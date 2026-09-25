// AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
// (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

// <!-- custom: To validate or compare level-3 authoritative-RNG checkpoints, use /LLM_Helpers/compare_sasgamerecord_rng.py.
// /LLM_Helpers/examples/sasgamerecord_rng_compared.txt shows a maintained provenance-only divergence report. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->

#include "CvGameCoreDLL.h"
#include "SASGameRecordLog.h"
#include "CvGame.h" // <!-- custom: Needed for game-record turn, game-state, victory, RNG, and map-classification context rows. (GPT-5.5) -->
#include "CvDeal.h" // <!-- custom: Needed directly for canonical state-checkpoint deal identities and persisted trade-item lists; do not rely on CvGame headers to complete CvDeal transitively. (ChatGPT-5.6-Sol) -->
#include "CvCity.h" // <!-- custom: Needed by game-record city action/BFC rows; SASGameRecordLog.h only forward-declares CvCity. (GPT-5.5) -->
#include "CvCityAI.h" // <!-- custom: Needed only to read the existing Avoid Growth AI emphasis flag in city snapshots/aggregates; CvCity.h only forward-declares CvCityAI. This is a compile-time type dependency and does not alter AI state or gameplay. (ChatGPT-5.6-Sol) -->
#include "CvUnit.h" // <!-- custom: Needed by game-record battle rows; SASGameRecordLog.h only forward-declares CvUnit. (GPT-5.5) -->
#include "CombatOdds.h" // <!-- custom: Needed only for exact pre-combat odds on real level-2+ battle outcomes; AI candidate valuation remains untouched. (ChatGPT-5.6-Sol) -->
#include "CvUnitAI.h" // <!-- custom: Needed to inspect the head unit of large city groups and its UnitAI role; the base unit header only forward-declares CvUnitAI. (GPT-5.6-Sol) -->
#include "CityPlotIterator.h" // <!-- custom: Needed by compact game-record BFC composition rows. (ChatGPT-5.5) -->
#include "CitySiteEvaluator.h" // <!-- custom: Needed only for cold city-founding site-quality summaries; ordinary periodic SASGameRecord snapshots do not invoke settlement evaluation. (ChatGPT-5.6-Sol) -->
#include "CvPlot.h" // <!-- custom: Needed by game-record BFC and unit posture rows. (ChatGPT-5.5) -->
#include "CvInfo_Build.h" // <!-- custom: Needed for worker build-type names and build target classification in game-record rows. (ChatGPT-5.5) -->
#include "CvInfo_Command.h" // <!-- custom: Needed for mission-type names in worker/settler game-record rows. (ChatGPT-5.5) -->
#include "CvInfo_Building.h" // <!-- custom: Needed to classify city production in game-record city rows. (ChatGPT-5.5) -->
#include "CvInfo_Tech.h" // <!-- custom: Needed to bucket owned-tech counts by era in game-record rows. (ChatGPT-5.5) -->
#include "CvInfo_RandomEvent.h" // <!-- custom: Needed for stable EventTrigger/EventInfo identities and gameplay-owned unit-local random-event semantics in dedicated random-event lifecycle rows. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
#include "CvInfo_Terrain.h" // <!-- custom: Needed for terrain/feature/bonus type names in game-record context rows. (ChatGPT-5.5) -->
#include "CvInfo_Organization.h" // <!-- custom: Needed for religion/corporation type names in game-record action rows. (ChatGPT-5.5) -->
#include "CvInfo_Unit.h" // <!-- custom: Needed to classify unit composition and city production in game-record rows. (ChatGPT-5.5) -->
#include "CvInfo_Symbol.h" // <!-- custom: Needed for commerce-slider type names and actual assigned player-color/primary-color context; CvGlobals only forward-declares the related info classes. (GPT-5.5 + GPT-5.6-Sol) -->
#include "CvInfo_City.h" // <!-- custom: Needed for specialist and process type names in game-record city rows. (ChatGPT-5.5) -->
#include "CvInfo_Civics.h" // <!-- custom: Needed for policy/civic names in game-record advisor rows. (ChatGPT-5.5) -->
#include "CvInfo_Civilization.h" // <!-- custom: Needed to attribute player-wide extra happiness/health to traits instead of leaving effects from loaded-mod rules under an opaque `extra` label. (GPT-5.6-Sol) -->
#include "CvCivilization.h" // <!-- custom: Needed to resolve civilization-specific BuildingClass types in realized random-event building/city result rows; CvPlayer/CvCity only forward-declare the runtime CvCivilization wrapper. This is a compile-time dependency only. (ChatGPT-5.6-Sol) -->
#include "CvInfo_GameOption.h" // <!-- custom: Needed to log enabled game-option type names; CvGlobals only forward-declares CvGameOptionInfo. (GPT-5.5) -->
#include "CvInfo_Misc.h" // <!-- custom: Needed to log enabled graphics-option type names; CvGlobals only forward-declares CvGraphicOptionInfo. (GPT-5.6-Sol) -->
#include "CvMap.h" // <!-- custom: Needed to log map dimensions; CvGlobals only forward-declares CvMap. (GPT-5.5) -->
#include "CvSelectionGroup.h" // <!-- custom: Needed to inspect worker/settler mission queues in game-record rows. (ChatGPT-5.5) -->
#include "CvSelectionGroupAI.h" // <!-- custom: Needed for large city-group mission targets and MissionAI state; the base group header only forward-declares CvSelectionGroupAI. (GPT-5.6-Sol) -->
#include "CvPlotGroup.h" // <!-- custom: Needed to identify connected city networks in game-record city rows. (ChatGPT-5.5) -->
#include "CvArea.h" // <!-- custom: Needed for area-wide city happiness/health detail rows. (ChatGPT-5.5) -->
#include "CvPlayerAI.h" // <!-- custom: Needed for attitude/glance values in game-record advisor rows. (ChatGPT-5.5) -->
#include "AgentIterator.h" // <!-- custom: Needed directly for MemberIter in compact team-aware research-redirection context; do not rely on unrelated gameplay headers to provide the iterator transitively. (ChatGPT-5.6-Sol) -->
#include "CvTeamAI.h" // <!-- custom: Needed for team-level worst-enemy state in game-record diplomacy-status rows. (ChatGPT-5.5) -->
#include "CvGameAI.h" // <!-- custom: Complete CvGameAI is needed for getUWAI(). (ChatGPT-5.6-Sol) -->
#include "CvPythonCaller.h" // <!-- custom: Resolve map-option defaults and descriptions. (ChatGPT-5.6-Sol) -->
#include "CvStatistics.h" // <!-- custom: Needed for persistent player-record statistics in game-record benchmark rows. (GPT-5.5) -->
#include "CvGameCoreUtils.h" // <!-- custom: Needed for the shared DLL-process UTC identity used by BBAI and SASGameRecord. See KI#629. (GPT-5.6-Sol) -->
#include <time.h>
#include <psapi.h> // <!-- custom: Reuse AdvCiv's existing Windows process-memory API for compact game-record performance context. (GPT-5.6-Sol) -->
#include <algorithm> // <!-- custom: Needed to deduplicate buffered plot-change/map-revelation coordinates within each turn. (GPT-5.6-Sol) -->
#include <map> // <!-- custom: Level-3 city snapshot delta compression keeps small session-local previous-row state keyed by city identity/row family; disabled lower record levels never populate it. (ChatGPT-5.6-Sol) -->
#include <utility> // <!-- custom: Needed for Great Person odds pairs in game-record city rows. (ChatGPT-5.5) -->
#include <vector> // <!-- custom: Used for compact dynamic buckets in game-record known-area, BFC development, advisor, tech-era, worker/settler, and unit-composition rows. (ChatGPT-5.5) -->

static int getClampedSASGameRecordLogLevel(char const* szDefineName)
{
	const int iLevel = GC.getDefineINT(szDefineName);
	if (iLevel < 0)
		return 0;
	if (iLevel > 3)
		return 3;
	return iLevel;
}

// <!-- custom: Dedicated structured game-record log for autoplay comparison, general game analysis, and external LLM review.
// This is independent from SAS_BBAI_LOG_ENABLE because it is a run-report artifact rather than classic AI-decision diagnostics, and writes to SASGameRecord_*.log when enabled.
// Use ACTION rows rather than generic EVENT rows to avoid confusion with Civ4 random events; dedicated GAME_RECORD_RANDOM_EVENT_* rows are reserved specifically for actual EventTrigger/EventInfo lifecycle boundaries.
// Keep the recorder portable across Civ4 mods by enumerating loaded XML and using generic field meanings instead of hardcoding AdvCiv-SAS types or copying the full XML.
// Mod-specific rules can still be named in comments as concrete examples: TECH_DEPOPULATION currently applies negative player-wide health and happiness in AdvCiv-SAS, but the recorder attributes health/happiness from every loaded trait, civic and technology dynamically.
// The record describes the current format; do not add schema-version maintenance unless independently evolving consumers later require it. (ChatGPT-5.5 + GPT-5.5 + ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
// <!-- custom: Default to disabled during DLL/XML startup.
// CvXMLLoadUtility::SetGlobalDefines calls cacheSASGameRecordLogLevel only after base, SAS and modular GlobalDefines have all loaded, after which the widespread gGameRecordLogLevel gates are direct reads for the rest of the process.
// The old out-of-line static-local getter made every disabled hook potentially pay a function call in ordinary /O2 Release builds without /GL + /LTCG. (ChatGPT-5.6-Sol) -->
int g_iSASGameRecordLogLevel = 0;

void cacheSASGameRecordLogLevel()
{
	g_iSASGameRecordLogLevel = getClampedSASGameRecordLogLevel("SAS_GAME_RECORD_LOG_LEVEL");
}

static bool isSASGameRecordPerformanceMetricsEnabled()
{
	static const bool bEnabled = (GC.getDefineINT("SAS_GAME_RECORD_PERFORMANCE_METRICS_ENABLE") > 0);
	return bEnabled;
}

// <!-- custom: Foreign-Advisor market snapshots are intentionally level-3-only and can be disabled independently because enumerating target-specific resource/technology offerability and denial reasons is more expensive than ordinary factual player snapshots.
// Cache the switch so disabled runs perform no repeated XML lookup or market scan. (ChatGPT-5.6-Sol) -->
static bool isSASGameRecordTradeMarketEnabled()
{
	static const bool bEnabled = (GC.getDefineINT("SAS_GAME_RECORD_TRADE_MARKET_ENABLE") > 0);
	return bEnabled;
}

static bool isSASGameRecordTradeMarketBonusGPTQuotesEnabled()
{
	static const bool bEnabled = (GC.getDefineINT("SAS_GAME_RECORD_TRADE_MARKET_BONUS_GPT_QUOTES_ENABLE") > 0);
	return bEnabled;
}

static bool isSASGameRecordTradeMarketAITechValuesEnabled()
{
	static const bool bEnabled = (GC.getDefineINT("SAS_GAME_RECORD_TRADE_MARKET_AI_TECH_VALUES_ENABLE") > 0);
	return bEnabled;
}

static int getSASGameRecordSystemContextLevel()
{
	static const int iLevel = std::min(3, std::max(0, GC.getDefineINT("SAS_GAME_RECORD_SYSTEM_CONTEXT_LEVEL")));
	return iLevel;
}

int getSASGameRecordTurnInterval()
{
	// <!-- custom: Separate snapshot frequency from detail level. Level 0 disables the game-record rows; the interval is still clamped so modulo callers are safe. (ChatGPT-5.5) -->
	static const int iInterval = std::max(1, GC.getDefineINT("SAS_GAME_RECORD_INTERVAL_TURNS_UNSCALED_GAMESPEED"));
	return iInterval;
}

// <!-- custom: Cache the tunable preview bounds and horizontal character ratio like the other SASGameRecord settings; 0 for either bound lets level 3 keep its other detail while omitting all text-map layers. (GPT-5.6-Sol) -->
static int getSASGameRecordMapAsciiMaxWidth()
{
	static const int iMaxWidth = std::max(0, GC.getDefineINT("SAS_GAME_RECORD_MAP_ASCII_MAX_WIDTH"));
	return iMaxWidth;
}

static int getSASGameRecordMapAsciiMaxHeight()
{
	static const int iMaxHeight = std::max(0, GC.getDefineINT("SAS_GAME_RECORD_MAP_ASCII_MAX_HEIGHT"));
	return iMaxHeight;
}

static int getSASGameRecordMapAsciiHorizontalCharsPerCell()
{
	static const int iChars = std::min(4, std::max(1, GC.getDefineINT("SAS_GAME_RECORD_MAP_ASCII_HORIZONTAL_CHARS_PER_CELL")));
	return iChars;
}

// <!-- custom: Two initial geography overviews provide quick small/medium impressions before the authoritative full text maps. Clamp each percentage to 0..100 so malformed values cannot create oversized drawings; 0 disables that overview. (GPT-5.6-Sol) -->
static int getSASGameRecordMapAsciiOverview1ScalePercent()
{
	static const int iPercent = std::min(100, std::max(0, GC.getDefineINT("SAS_GAME_RECORD_MAP_ASCII_OVERVIEW_1_SCALE_PERCENT")));
	return iPercent;
}

static int getSASGameRecordMapAsciiOverview2ScalePercent()
{
	static const int iPercent = std::min(100, std::max(0, GC.getDefineINT("SAS_GAME_RECORD_MAP_ASCII_OVERVIEW_2_SCALE_PERCENT")));
	return iPercent;
}

static bool isSASGameRecordMapAsciiNativeGeographyEnabled()
{
	static const bool bEnabled = (GC.getDefineINT("SAS_GAME_RECORD_MAP_ASCII_NATIVE_GEOGRAPHY_ENABLE") > 0);
	return bEnabled;
}

// <!-- custom: Each text-map layer can be disabled independently to reduce SASGameRecord size while retaining the layers useful for a particular analysis.
// Cache these XML switches like the other recorder settings; width or height 0 remains a convenient master switch for all ASCII maps. (ChatGPT-5.6-Sol) -->
static bool isSASGameRecordMapAsciiGeographyEnabled()
{
	static const bool bEnabled = (GC.getDefineINT("SAS_GAME_RECORD_MAP_ASCII_GEOGRAPHY_ENABLE") > 0);
	return bEnabled;
}

static bool isSASGameRecordMapAsciiTerrainEnabled()
{
	static const bool bEnabled = (GC.getDefineINT("SAS_GAME_RECORD_MAP_ASCII_TERRAIN_ENABLE") > 0);
	return bEnabled;
}

static bool isSASGameRecordMapAsciiRiversEnabled()
{
	static const bool bEnabled = (GC.getDefineINT("SAS_GAME_RECORD_MAP_ASCII_RIVER_ENABLE") > 0);
	return bEnabled;
}

static bool isSASGameRecordMapAsciiBonusesEnabled()
{
	static const bool bEnabled = (GC.getDefineINT("SAS_GAME_RECORD_MAP_ASCII_BONUS_ENABLE") > 0);
	return bEnabled;
}

static bool isSASGameRecordMapAsciiFeaturesEnabled()
{
	static const bool bEnabled = (GC.getDefineINT("SAS_GAME_RECORD_MAP_ASCII_FEATURE_ENABLE") > 0);
	return bEnabled;
}

static bool isSASGameRecordMapAsciiPoliticalEnabled()
{
	static const bool bEnabled = (GC.getDefineINT("SAS_GAME_RECORD_MAP_ASCII_POLITICAL_ENABLE") > 0);
	return bEnabled;
}

static CvString g_szSASGameRecordLogTimestamp;
static int g_iSASGameRecordLogSequence = 0;
static CvString g_szSASGameRecordLogContext;
// <!-- custom: Structured row sequence and transaction IDs are recorder/session-local only; they never enter gameplay or save state.
// `seq` is assigned only when a GAME_RECORD_* row is actually emitted, so buffered initialization actions receive their canonical chronology at flush time rather than when first formatted.
// `tx` is attached when a row is formatted inside an active causal scope, so delayed emission cannot accidentally inherit a later unrelated transaction. Raw pipe-framed ASCII-map drawing rows intentionally remain undecorated. (ChatGPT-5.6-Sol) -->
static unsigned __int64 g_uiSASGameRecordSemanticSequence = 0;
static unsigned __int64 g_uiSASGameRecordNextTransaction = 0;
static unsigned __int64 g_uiSASGameRecordActiveTransaction = 0;
static CvString g_szSASGameRecordActiveTransactionKind;
static SASGameRecordPlotOwnerChangeCause g_eSASGameRecordPlotOwnerChangeCause = SAS_PLOT_OWNER_CAUSE_NONE;
// <!-- custom: Record snapshot UTC plus cumulative and per-interval wall time directly so benchmark duration is visible without external timestamp subtraction.
// Use the monotonic millisecond timer for useful precision and immunity to system-clock adjustments; wall time intentionally includes pauses and user interaction. (GPT-5.6-Sol) -->
static uint g_uiSASGameRecordSessionStartTime = 0;
static uint g_uiSASGameRecordPreviousSnapshotTime = 0;
static bool g_bSASGameRecordDisplayContextLogged = false;
// <!-- custom: New-game setup can emit many real ACTION rows before the EXE reports gameStart.
// Buffer only those actions so stable game/mod/map context stays at the top, then flush them in original order once initialization is complete. (ChatGPT-5.6-Sol) -->
static bool g_bSASGameRecordBufferInitializingActions = false;
static std::vector<std::pair<CvString, std::string> > g_aszSASGameRecordInitializingActions;

// <!-- custom: Level-3 periodic city-detail rows are among the largest/repetitive SASGameRecord families.
// Keep full rows as ordinary self-contained checkpoints, but between them emit only changed fields with an explicit previous/full-base turn chain.
// State is session-local and cleared on every new/load log roll, so a fresh file always starts each observed city/family with a full row; every tenth observation is full again to bound recovery after a truncated excerpt.
// The cache exists only for level-3-only city detail already being formatted/logged and therefore adds no work to level 0-2 gameplay. (ChatGPT-5.6-Sol; inspired by Claude Opus 5 and Claude Sonnet 5 feedback) -->
struct SASGameRecordCityDeltaField
{
	std::string szName;
	std::string szToken;
};
struct SASGameRecordCityDeltaState
{
	SASGameRecordCityDeltaState() : iPreviousTurn(-1), iFullBaseTurn(-1), iSnapshotsSinceFull(0) {}
	std::vector<SASGameRecordCityDeltaField> aFields;
	int iPreviousTurn;
	int iFullBaseTurn;
	int iSnapshotsSinceFull;
};
struct SASGameRecordCityIdentityState
{
	SASGameRecordCityIdentityState() : iGeneration(0) {}
	std::string szSignature;
	int iGeneration;
};
static std::map<std::string, SASGameRecordCityDeltaState> g_mapSASGameRecordCityDeltaStates;
static std::map<std::string, SASGameRecordCityIdentityState> g_mapSASGameRecordCityIdentityStates;
static const int SAS_GAME_RECORD_CITY_FULL_SNAPSHOT_CADENCE = 10;
static void flushSASGameRecordPendingCityBombard();
static bool g_bSASGameRecordFlushingCityBombard = false;

// <!-- custom: Level-3 reproducibility telemetry observes the two authoritative CvGame RNG streams without changing CvRandom's serialized 8-byte layout.
// RandLog intentionally remains the raw per-roll diagnostic; these trackers instead retain compact checkpoint-interval/session counts plus two order-sensitive FNV-1a fingerprints.
// Session fingerprints stay 64-bit; interval fingerprints deliberately use 32-bit FNV because every checkpoint also carries interval start/end state and counters plus both independent fingerprints. This halves duplicated 64-bit multiply work in Civ4's 32-bit hot RNG path while retaining a strong interval-local diagnostic signal.
// The stream fingerprint hashes the ordered abstract RNG operations needed to reproduce returned values: ROLL(requested upper bound) plus effective SEED_SET(new state) operations; a redundant same-state assignment is retained only in call provenance because it cannot alter any random value.
// With the same session-start state and CvRandom algorithm, it therefore remains useful across source builds even when logging labels move, and still stays meaningful if benchmark/Python code deliberately reseeds an authoritative stream mid-session.
// The richer call fingerprint additionally hashes a compact stable digest/length of the optional message, data1/data2 and EXE-wrapper origin for rolls, plus old/new state and reset-vs-reseed origin for seed sets. Each detailed operation is first reduced with cheap 32-bit FNV, then fed into the 64-bit session accumulator and native 32-bit interval accumulator, avoiding duplicated emulated 64-bit multiplies on Civ4's 32-bit build.
// Messages often contain CALL_LOC_STR source locations, so exact call-fingerprint comparison is intentionally strongest for runs using the same DLL/source build; states, counts and stream fingerprints remain independently useful across builds.
// As with the existing DLL FNV identifier, these are diagnostic divergence fingerprints rather than cryptographic proofs; independent seed/state/counter fields remain visible beside them.
// NULL-message calls are important: CvRandom shuffles and some iterator randomization advance synchronized state while intentionally producing no RandLog row.
// Async RNG is deliberately non-lockstep and can vary with client/UI activity (even though a few local human-interaction paths can use its result); local CvRandom helpers likewise do not advance CvGame's two authoritative streams.
// Ignore both by pointer identity so this fingerprint answers synchronized/map-stream reproducibility rather than conflating independent randomness with it.
// Python/third-party RNGs that do not advance these CvRandom objects, clocks and other external nondeterminism are likewise outside this layer; matching checkpoints are strong authoritative-RNG evidence, not a proof that all mutable game state is identical. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
bool g_bSASGameRecordRngTrackingActive = false;

struct SASGameRecordRngTracker
{
	SASGameRecordRngTracker() { clear(); }
	void clear()
	{
		bInitialized = false;
		bNextCallExternal = false;
		uiSessionStartState = uiIntervalStartState = 0;
		uiSessionCalls = uiIntervalCalls = 0;
		uiSessionNullMessageCalls = uiIntervalNullMessageCalls = 0;
		uiSessionExternalCalls = uiIntervalExternalCalls = 0;
		uiSessionDeterministicRangeCalls = uiIntervalDeterministicRangeCalls = 0;
		uiSessionSeedSets = uiIntervalSeedSets = 0;
		uiSessionStreamFingerprint = getOffsetBasis64();
		uiSessionCallFingerprint = getOffsetBasis64();
		uiIntervalStreamFingerprint = getOffsetBasis32();
		uiIntervalCallFingerprint = getOffsetBasis32();
	}
	void initialize(unsigned int uiState)
	{
		clear();
		bInitialized = true;
		uiSessionStartState = uiIntervalStartState = uiState;
	}
	void resetInterval(unsigned int uiState)
	{
		uiIntervalStartState = uiState;
		uiIntervalCalls = 0;
		uiIntervalNullMessageCalls = 0;
		uiIntervalExternalCalls = 0;
		uiIntervalDeterministicRangeCalls = 0;
		uiIntervalSeedSets = 0;
		uiIntervalStreamFingerprint = getOffsetBasis32();
		uiIntervalCallFingerprint = getOffsetBasis32();
	}
	static unsigned __int64 getOffsetBasis64() { return ((unsigned __int64)0xCBF29CE4 << 32) | 0x84222325; }
	static unsigned int getOffsetBasis32() { return 2166136261u; }
	bool bInitialized;
	bool bNextCallExternal;
	unsigned int uiSessionStartState;
	unsigned int uiIntervalStartState;
	unsigned __int64 uiSessionCalls;
	unsigned __int64 uiIntervalCalls;
	unsigned __int64 uiSessionNullMessageCalls;
	unsigned __int64 uiIntervalNullMessageCalls;
	unsigned __int64 uiSessionExternalCalls;
	unsigned __int64 uiIntervalExternalCalls;
	unsigned __int64 uiSessionDeterministicRangeCalls;
	unsigned __int64 uiIntervalDeterministicRangeCalls;
	unsigned __int64 uiSessionSeedSets;
	unsigned __int64 uiIntervalSeedSets;
	unsigned __int64 uiSessionStreamFingerprint;
	unsigned int uiIntervalStreamFingerprint;
	unsigned __int64 uiSessionCallFingerprint;
	unsigned int uiIntervalCallFingerprint;
};

static SASGameRecordRngTracker g_kSASGameRecordMapRng;
static SASGameRecordRngTracker g_kSASGameRecordSyncRng;
// <!-- custom: Cache the two authoritative object addresses at session initialization so the level-3 hot path needs only pointer comparisons, not GC/CvGame lookups, for every RNG advance. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
static CvRandom const* g_pSASGameRecordMapRng = NULL;
static CvRandom const* g_pSASGameRecordSyncRng = NULL;

static void updateSASGameRecordFNV1AByte(unsigned __int64& uiHash, unsigned char ucValue)
{
	static unsigned __int64 const uiPrime = ((unsigned __int64)0x00000100 << 32) | 0x000001B3;
	uiHash ^= ucValue;
	uiHash *= uiPrime;
}

static void updateSASGameRecordFNV1AUInt16(unsigned __int64& uiHash, unsigned short usValue)
{
	updateSASGameRecordFNV1AByte(uiHash, (unsigned char)(usValue & 0xFF));
	updateSASGameRecordFNV1AByte(uiHash, (unsigned char)(usValue >> 8));
}

static void updateSASGameRecordFNV1AUInt32(unsigned __int64& uiHash, unsigned int uiValue)
{
	for (int iShift = 0; iShift < 32; iShift += 8)
		updateSASGameRecordFNV1AByte(uiHash, (unsigned char)((uiValue >> iShift) & 0xFF));
}

static void updateSASGameRecordFNV1A32Byte(unsigned int& uiHash, unsigned char ucValue)
{
	uiHash ^= ucValue;
	uiHash *= 16777619u;
}

static void updateSASGameRecordFNV1A32UInt16(unsigned int& uiHash, unsigned short usValue)
{
	updateSASGameRecordFNV1A32Byte(uiHash, (unsigned char)(usValue & 0xFF));
	updateSASGameRecordFNV1A32Byte(uiHash, (unsigned char)(usValue >> 8));
}

static void updateSASGameRecordFNV1A32UInt32(unsigned int& uiHash, unsigned int uiValue)
{
	for (int iShift = 0; iShift < 32; iShift += 8)
		updateSASGameRecordFNV1A32Byte(uiHash, (unsigned char)((uiValue >> iShift) & 0xFF));
}

// <!-- custom: Level-3 semantic state fingerprints complement authoritative RNG checkpoints: equal RNG streams do not prove equal gameplay state, while component hashes can narrow a deterministic divergence before ordinary snapshots explain it in human-readable detail.
// Hash selected durable/core gameplay and AI-planning state in deterministic slot/free-list/map-index order; never hash raw object memory, pointers, padding, localized/user-entered strings, recorder state, RNG seeds, UI state, or incidental implementation caches whose differences are not gameplay-relevant.
// Serialized/AI-visible bookkeeping such as power/assets/maintenance is intentionally retained: divergence in such cached gameplay values can itself change later AI/economic behavior even when the underlying units/buildings still match.
// Deliberately omit very large per-plot-per-player culture/reveal arrays, per-build plot work-progress matrices and inactive per-city production inventories from this turn-by-turn CORE coverage; exact unit/group missions, city culture/buildings/current production, plot ownership/physical state, team/player state and ordinary SASGameRecord history still provide strong divergence sensitivity without turning each turn boundary into a full save/snapshot scan.
// Each object's fields are first reduced with a cheap native 32-bit ordered mix, then that fixed-width signature enters a 64-bit FNV-1a component hash. This keeps the 32-bit Civ4 hot-turn cost far below hashing every scalar with emulated 64-bit multiplication.
// The recordRevision identifies the exact recipe. These are diagnostic fingerprints rather than cryptographic proofs or savegame-equivalence guarantees. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
struct SASGameRecordStateFingerprints
{
	SASGameRecordStateFingerprints()
	{
		static unsigned __int64 const uiOffset = ((unsigned __int64)0xCBF29CE4 << 32) | 0x84222325;
		uiGame = uiTeams = uiPlayers = uiCities = uiUnits = uiGroups = uiPlots = uiDeals = uiCombined = uiOffset;
		iEverAliveTeamCount = iEverAlivePlayerCount = iCityCount = iUnitCount = iGroupCount = iDealCount = 0;
		iPlotCount = 0;
	}
	unsigned __int64 uiGame;
	unsigned __int64 uiTeams;
	unsigned __int64 uiPlayers;
	unsigned __int64 uiCities;
	unsigned __int64 uiUnits;
	unsigned __int64 uiGroups;
	unsigned __int64 uiPlots;
	unsigned __int64 uiDeals;
	unsigned __int64 uiCombined;
	int iEverAliveTeamCount;
	int iEverAlivePlayerCount;
	int iCityCount;
	int iUnitCount;
	int iGroupCount;
	int iPlotCount;
	int iDealCount;
};

struct SASGameRecordStateObjectHash
{
	SASGameRecordStateObjectHash() : uiPrimary(2166136261u), uiSecondary(0x9E3779B9u) {}
	unsigned int uiPrimary;
	unsigned int uiSecondary;
};

static void updateSASGameRecordStateValue(SASGameRecordStateObjectHash& kHash, int iValue)
{
	// <!-- custom: Two cheap native-32-bit ordered reducers retain substantially more divergence information per object before the 64-bit component hash, without paying an emulated 64-bit multiply for every scalar on Civ4's 32-bit build.
	// Cast preserves negative sentinel bit patterns deterministically. The secondary hash-combine-style reducer intentionally differs from the primary FNV-like multiply/xor path; together they remain diagnostic rather than cryptographic. (ChatGPT-5.6-Sol) -->
	unsigned int const uiValue = (unsigned int)iValue;
	kHash.uiPrimary ^= uiValue;
	kHash.uiPrimary *= 16777619u;
	kHash.uiSecondary ^= uiValue + 0x9E3779B9u + (kHash.uiSecondary << 6) + (kHash.uiSecondary >> 2);
}

static void updateSASGameRecordStateObject(unsigned __int64& uiComponentHash, SASGameRecordStateObjectHash const& kObjectHash)
{
	updateSASGameRecordFNV1AUInt32(uiComponentHash, kObjectHash.uiPrimary);
	updateSASGameRecordFNV1AUInt32(uiComponentHash, kObjectHash.uiSecondary);
}

static void updateSASGameRecordStateHash64(unsigned __int64& uiHash, unsigned __int64 uiValue)
{
	updateSASGameRecordFNV1AUInt32(uiHash, (unsigned int)(uiValue & 0xFFFFFFFF));
	updateSASGameRecordFNV1AUInt32(uiHash, (unsigned int)(uiValue >> 32));
}

static void updateSASGameRecordTriggeredEventState(SASGameRecordStateObjectHash& uiHash, EventTriggeredData const* pData)
{
	// <!-- custom: Event text is localized/user-facing and deliberately excluded; the numeric trigger target/state is durable gameplay provenance that can affect later event validity and choices. (ChatGPT-5.6-Sol) -->
	updateSASGameRecordStateValue(uiHash, pData != NULL);
	if (pData == NULL)
		return;
	updateSASGameRecordStateValue(uiHash, pData->m_iId);
	updateSASGameRecordStateValue(uiHash, pData->m_eTrigger);
	updateSASGameRecordStateValue(uiHash, pData->m_iTurn);
	updateSASGameRecordStateValue(uiHash, pData->m_ePlayer);
	updateSASGameRecordStateValue(uiHash, pData->m_iCityId);
	updateSASGameRecordStateValue(uiHash, pData->m_iPlotX);
	updateSASGameRecordStateValue(uiHash, pData->m_iPlotY);
	updateSASGameRecordStateValue(uiHash, pData->m_iUnitId);
	updateSASGameRecordStateValue(uiHash, pData->m_eOtherPlayer);
	updateSASGameRecordStateValue(uiHash, pData->m_iOtherPlayerCityId);
	updateSASGameRecordStateValue(uiHash, pData->m_eReligion);
	updateSASGameRecordStateValue(uiHash, pData->m_eCorporation);
	updateSASGameRecordStateValue(uiHash, pData->m_eBuilding);
}

static SASGameRecordStateObjectHash getSASGameRecordGameStateSignature(CvGame& kGame)
{
	SASGameRecordStateObjectHash uiHash;
	updateSASGameRecordStateValue(uiHash, kGame.getGameTurn());
	updateSASGameRecordStateValue(uiHash, kGame.getElapsedGameTurns());
	updateSASGameRecordStateValue(uiHash, kGame.getStartTurn());
	updateSASGameRecordStateValue(uiHash, kGame.getGameState());
	// <!-- custom: Do not hash getActivePlayer(): it is local client/UI context and can legitimately differ between peers whose synchronized gameplay state is identical. (GPT-5.6-Sol) -->
	updateSASGameRecordStateValue(uiHash, kGame.getWinner());
	updateSASGameRecordStateValue(uiHash, kGame.getVictory());
	updateSASGameRecordStateValue(uiHash, kGame.getCurrentEra());
	updateSASGameRecordStateValue(uiHash, kGame.getNumCities());
	updateSASGameRecordStateValue(uiHash, kGame.getTotalPopulation());
	updateSASGameRecordStateValue(uiHash, kGame.getNumDeals());
	// <!-- custom: Map dimensions/wrap topology are part of gameplay geometry; a flattened plot sequence alone cannot distinguish every possible geometry with the same number/content of plots. (ChatGPT-5.6-Sol) -->
	CvMap const& kMap = GC.getMap();
	updateSASGameRecordStateValue(uiHash, kMap.getGridWidth());
	updateSASGameRecordStateValue(uiHash, kMap.getGridHeight());
	updateSASGameRecordStateValue(uiHash, kMap.isWrapX());
	updateSASGameRecordStateValue(uiHash, kMap.isWrapY());
	updateSASGameRecordStateValue(uiHash, kMap.getOwnedPlots());
	updateSASGameRecordStateValue(uiHash, kMap.getNumAreas());
	updateSASGameRecordStateValue(uiHash, kGame.getGlobalWarmingIndex());
	updateSASGameRecordStateValue(uiHash, kGame.getNukesExploded());
	updateSASGameRecordStateValue(uiHash, kGame.getTradeRoutes());
	for (int iI = 0; iI < GC.getNumProjectInfos(); iI++)
		updateSASGameRecordStateValue(uiHash, kGame.getProjectCreatedCount((ProjectTypes)iI));
	for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		ReligionTypes const eReligion = (ReligionTypes)iI;
		updateSASGameRecordStateValue(uiHash, kGame.getReligionGameTurnFounded(eReligion));
		CvCity const* pHolyCity = kGame.getHolyCity(eReligion);
		updateSASGameRecordStateValue(uiHash, pHolyCity == NULL ? NO_PLAYER : pHolyCity->getOwner());
		updateSASGameRecordStateValue(uiHash, pHolyCity == NULL ? -1 : pHolyCity->getID());
	}
	for (int iI = 0; iI < GC.getNumCorporationInfos(); iI++)
	{
		CorporationTypes const eCorp = (CorporationTypes)iI;
		updateSASGameRecordStateValue(uiHash, kGame.getCorporationGameTurnFounded(eCorp));
		CvCity const* pHeadquarters = kGame.getHeadquarters(eCorp);
		updateSASGameRecordStateValue(uiHash, pHeadquarters == NULL ? NO_PLAYER : pHeadquarters->getOwner());
		updateSASGameRecordStateValue(uiHash, pHeadquarters == NULL ? -1 : pHeadquarters->getID());
	}
	// <!-- custom: Vote outcomes and source timers can diverge before a resolution visibly changes cities/players, so retain this small durable global decision state in CORE. (ChatGPT-5.6-Sol) -->
	for (int iI = 0; iI < GC.getNumVoteInfos(); iI++)
		updateSASGameRecordStateValue(uiHash, kGame.getVoteOutcome((VoteTypes)iI));
	for (int iI = 0; iI < GC.getNumVoteSourceInfos(); iI++)
	{
		VoteSourceTypes const eSource = (VoteSourceTypes)iI;
		updateSASGameRecordStateValue(uiHash, kGame.getSecretaryGeneralTimer(eSource));
		updateSASGameRecordStateValue(uiHash, kGame.getVoteTimer(eSource));
		updateSASGameRecordStateValue(uiHash, kGame.getDiploVoteCount(eSource));
		updateSASGameRecordStateValue(uiHash, kGame.getVoteSourceReligion(eSource));
	}
	return uiHash;
}

static SASGameRecordStateObjectHash getSASGameRecordTeamStateSignature(CvTeamAI const& kTeam)
{
	SASGameRecordStateObjectHash uiHash;
	updateSASGameRecordStateValue(uiHash, kTeam.getID());
	updateSASGameRecordStateValue(uiHash, kTeam.isEverAlive());
	if (!kTeam.isEverAlive())
		return uiHash;
	updateSASGameRecordStateValue(uiHash, kTeam.isAlive());
	updateSASGameRecordStateValue(uiHash, kTeam.isMinorCiv());
	updateSASGameRecordStateValue(uiHash, kTeam.getNumMembers());
	updateSASGameRecordStateValue(uiHash, kTeam.getAliveCount());
	updateSASGameRecordStateValue(uiHash, kTeam.getNumCities());
	updateSASGameRecordStateValue(uiHash, kTeam.getTotalPopulation(false));
	updateSASGameRecordStateValue(uiHash, kTeam.getTotalLand(false));
	updateSASGameRecordStateValue(uiHash, kTeam.getPower(false));
	updateSASGameRecordStateValue(uiHash, kTeam.getMasterTeam());
	updateSASGameRecordStateValue(uiHash, kTeam.isCapitulated());
	updateSASGameRecordStateValue(uiHash, kTeam.getVassalPower());
	updateSASGameRecordStateValue(uiHash, kTeam.getMasterPower());
	updateSASGameRecordStateValue(uiHash, kTeam.getEnemyWarWearinessModifier());
	updateSASGameRecordStateValue(uiHash, kTeam.getCurrentEra());
	updateSASGameRecordStateValue(uiHash, kTeam.AI_getWorstEnemy());
	for (int iI = 0; iI < GC.getNumTechInfos(); iI++)
	{
		TechTypes const eTech = (TechTypes)iI;
		updateSASGameRecordStateValue(uiHash, kTeam.isHasTech(eTech));
		updateSASGameRecordStateValue(uiHash, kTeam.getResearchProgress(eTech));
	}
	// <!-- custom: Forced resource revelation is persistent team state that can differ before the corresponding bonus becomes otherwise visible through technology or map knowledge. (ChatGPT-5.6-Sol) -->
	for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
		updateSASGameRecordStateValue(uiHash, kTeam.isForceRevealedBonus((BonusTypes)iI));
	for (int iI = 0; iI < GC.getNumProjectInfos(); iI++)
		updateSASGameRecordStateValue(uiHash, kTeam.getProjectCount((ProjectTypes)iI));
	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		TeamTypes const eOther = (TeamTypes)iI;
		updateSASGameRecordStateValue(uiHash, kTeam.isHasMet(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.isAtWar(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.isOpenBorders(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.isDefensivePact(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.isForcePeace(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.isPermanentWarPeace(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.getEspionagePointsAgainstTeam(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.getWarWeariness(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.getCounterespionageTurnsLeftAgainstTeam(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.getCounterespionageModAgainstTeam(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.AI_getWarPlan(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.AI_getWarPlanStateCounter(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.AI_getAtWarCounter(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.AI_getAtPeaceCounter(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.AI_getHasMetCounter(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.AI_getOpenBordersCounter(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.AI_getDefensivePactCounter(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.AI_getShareWarCounter(eOther));
		updateSASGameRecordStateValue(uiHash, kTeam.AI_getWarSuccess(eOther).round());
	}
	return uiHash;
}

// <!-- custom: Match AI_isDoStrategy's gameplay eligibility while treating AI Auto Play's disabled-human slot as AI-controlled.
// Dead, Barbarian, minor and ordinary-human players have no active AI strategy even if their internal hash contains bits; fingerprints/snapshots/transitions should therefore expose only strategy state that can actually drive AI behavior. (ChatGPT-5.6-Sol) -->
static bool isSASGameRecordAIStrategyPlayer(CvPlayerAI const& kPlayer)
{
	return (kPlayer.isAlive() && !kPlayer.isBarbarian() && !kPlayer.isMinorCiv() && (!kPlayer.isHuman() || kPlayer.isHumanDisabled()));
}

static bool isSASGameRecordAIStrategyActive(CvPlayerAI const& kPlayer, AIStrategy eStrategy)
{
	return kPlayer.AI_isDoStrategy(eStrategy, kPlayer.isHumanDisabled());
}

// <!-- custom: Victory-stage hashes are also maintained for ordinary humans because UWAI can inspect them, but GAME_RECORD_AI_VICTORY_* rows are AI-behavior history.
// Match the existing periodic-row policy: include AI Auto Play's disabled-human slot, but not an actively human-controlled player. (ChatGPT-5.6-Sol) -->
static bool isSASGameRecordAIVictoryStagePlayer(CvPlayerAI const& kPlayer)
{
	return (!kPlayer.isHuman() || kPlayer.isHumanDisabled());
}

static SASGameRecordStateObjectHash getSASGameRecordPlayerStateSignature(CvPlayerAI const& kPlayer)
{
	SASGameRecordStateObjectHash uiHash;
	updateSASGameRecordStateValue(uiHash, kPlayer.getID());
	updateSASGameRecordStateValue(uiHash, kPlayer.isEverAlive());
	if (!kPlayer.isEverAlive())
		return uiHash;
	updateSASGameRecordStateValue(uiHash, kPlayer.isAlive());
	updateSASGameRecordStateValue(uiHash, kPlayer.isHuman());
	updateSASGameRecordStateValue(uiHash, kPlayer.isMinorCiv());
	updateSASGameRecordStateValue(uiHash, kPlayer.getTeam());
	updateSASGameRecordStateValue(uiHash, kPlayer.getCivilizationType());
	updateSASGameRecordStateValue(uiHash, kPlayer.getLeaderType());
	updateSASGameRecordStateValue(uiHash, kPlayer.getPersonalityType());
	updateSASGameRecordStateValue(uiHash, kPlayer.getCurrentEra());
	updateSASGameRecordStateValue(uiHash, kPlayer.getParent());
	updateSASGameRecordStateValue(uiHash, kPlayer.getGold());
	updateSASGameRecordStateValue(uiHash, kPlayer.getGoldPerTurn());
	updateSASGameRecordStateValue(uiHash, kPlayer.getTotalMaintenanceTimes100());
	// <!-- custom: Effective inflation cheaply exposes the stored inflation modifier even when current maintenance happens to mask it; this is future-consequential economic state rather than a display-only derivative. (ChatGPT-5.6-Sol) -->
	updateSASGameRecordStateValue(uiHash, kPlayer.calculateInflationRate());
	updateSASGameRecordStateValue(uiHash, kPlayer.getTotalPopulation());
	updateSASGameRecordStateValue(uiHash, kPlayer.getTotalLand());
	updateSASGameRecordStateValue(uiHash, kPlayer.getAssets());
	updateSASGameRecordStateValue(uiHash, kPlayer.getPower());
	updateSASGameRecordStateValue(uiHash, kPlayer.getNumCities());
	updateSASGameRecordStateValue(uiHash, kPlayer.getNumUnits());
	updateSASGameRecordStateValue(uiHash, kPlayer.getGoldenAgeTurns());
	updateSASGameRecordStateValue(uiHash, kPlayer.getAnarchyTurns());
	updateSASGameRecordStateValue(uiHash, kPlayer.getStrikeTurns());
	updateSASGameRecordStateValue(uiHash, kPlayer.isStrike());
	updateSASGameRecordStateValue(uiHash, kPlayer.isTurnActive());
	updateSASGameRecordStateValue(uiHash, kPlayer.isAutoMoves());
	updateSASGameRecordStateValue(uiHash, kPlayer.isEndTurn());
	updateSASGameRecordStateValue(uiHash, kPlayer.isExtendedGame());
	updateSASGameRecordStateValue(uiHash, kPlayer.isFoundedFirstCity());
	updateSASGameRecordStateValue(uiHash, kPlayer.getStateReligion());
	updateSASGameRecordStateValue(uiHash, kPlayer.getLastStateReligion());
	updateSASGameRecordStateValue(uiHash, kPlayer.getRevolutionTimer());
	updateSASGameRecordStateValue(uiHash, kPlayer.getConversionTimer());
	updateSASGameRecordStateValue(uiHash, kPlayer.getOverflowResearch());
	updateSASGameRecordStateValue(uiHash, kPlayer.getCombatExperience());
	updateSASGameRecordStateValue(uiHash, kPlayer.getGreatPeopleCreated());
	updateSASGameRecordStateValue(uiHash, kPlayer.getGreatGeneralsCreated());
	updateSASGameRecordStateValue(uiHash, kPlayer.getCurrentResearch());
	updateSASGameRecordStateValue(uiHash, kPlayer.getExtraHealth());
	updateSASGameRecordStateValue(uiHash, kPlayer.getExtraHappiness());
	updateSASGameRecordStateValue(uiHash, kPlayer.getSpaceProductionModifier());
	updateSASGameRecordStateValue(uiHash, kPlayer.getBaseFreeUnits());
	updateSASGameRecordStateValue(uiHash, kPlayer.getBaseFreeMilitaryUnits());
	updateSASGameRecordStateValue(uiHash, kPlayer.getWarWearinessModifier());
	updateSASGameRecordStateValue(uiHash, kPlayer.getStateReligionUnitProductionModifier());
	updateSASGameRecordStateValue(uiHash, kPlayer.getStateReligionBuildingProductionModifier());
	updateSASGameRecordStateValue(uiHash, kPlayer.getStateReligionFreeExperience());
	CvCity const* pCapital = kPlayer.getCapital();
	updateSASGameRecordStateValue(uiHash, pCapital == NULL ? -1 : pCapital->getID());
	updateSASGameRecordStateValue(uiHash, kPlayer.getLengthResearchQueue());
	for (CLLNode<TechTypes>* pNode = kPlayer.headResearchQueueNode(); pNode != NULL; pNode = kPlayer.nextResearchQueueNode(pNode))
		updateSASGameRecordStateValue(uiHash, pNode->m_data);
	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
		updateSASGameRecordStateValue(uiHash, kPlayer.getCommercePercent((CommerceTypes)iI));
	for (int iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
		updateSASGameRecordStateValue(uiHash, kPlayer.getCivics((CivicOptionTypes)iI));
	for (int iI = 0; iI < MAX_TEAMS; iI++)
		updateSASGameRecordStateValue(uiHash, kPlayer.getEspionageSpendingWeightAgainstTeam((TeamTypes)iI));
	// <!-- custom: Pending/occurred/countdown random-event bookkeeping is small but future-consequential; identical visible economics/cities can otherwise hide a deterministic divergence in what events may fire next. (ChatGPT-5.6-Sol) -->
	for (int iI = 0; iI < GC.getNumEventTriggerInfos(); iI++)
		updateSASGameRecordStateValue(uiHash, kPlayer.isTriggerFired((EventTriggerTypes)iI));
	updateSASGameRecordStateValue(uiHash, kPlayer.getNumEventsTriggered());
	int iEventIter = 0;
	for (EventTriggeredData const* pEvent = kPlayer.firstEventTriggered(&iEventIter); pEvent != NULL; pEvent = kPlayer.nextEventTriggered(&iEventIter))
		updateSASGameRecordTriggeredEventState(uiHash, pEvent);
	for (int iI = 0; iI < GC.getNumEventInfos(); iI++)
	{
		EventTypes const eEvent = (EventTypes)iI;
		updateSASGameRecordTriggeredEventState(uiHash, kPlayer.getEventOccured(eEvent));
		updateSASGameRecordTriggeredEventState(uiHash, kPlayer.getEventCountdown(eEvent));
	}
	// <!-- custom: EventInfo and other gameplay paths can grant persistent free promotions for future matching units.
	// Existing-unit promotion flags alone cannot reveal this latent player state. (ChatGPT-5.6-Sol) -->
	for (int iI = 0; iI < GC.getNumPromotionInfos(); iI++)
	{
		PromotionTypes const ePromotion = (PromotionTypes)iI;
		for (int iJ = 0; iJ < GC.getNumUnitCombatInfos(); iJ++)
			updateSASGameRecordStateValue(uiHash, kPlayer.isFreePromotion((UnitCombatTypes)iJ, ePromotion));
		for (int iJ = 0; iJ < GC.getNumUnitClassInfos(); iJ++)
			updateSASGameRecordStateValue(uiHash, kPlayer.isFreePromotion((UnitClassTypes)iJ, ePromotion));
	}
	if (!kPlayer.isBarbarian())
	{
		// <!-- custom: The prototype failed to compile because raw AI_getStrategyHash is protected.
		// Hash each public strategy predicate through the recorder wrapper instead of widening CvPlayerAI solely for diagnostics; this also keeps AI Auto Play and ordinary-human eligibility consistent with readable strategy rows. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		for (int iStrategy = AI_DEFAULT_STRATEGY; iStrategy <= AI_STRATEGY_ESPIONAGE_ECONOMY; iStrategy <<= 1)
			updateSASGameRecordStateValue(uiHash, isSASGameRecordAIStrategyActive(kPlayer, (AIStrategy)iStrategy));
		updateSASGameRecordStateValue(uiHash, kPlayer.AI_getVictoryStageHash());
		updateSASGameRecordStateValue(uiHash, kPlayer.AI_getPeaceWeight());
		updateSASGameRecordStateValue(uiHash, kPlayer.AI_getEspionageWeight());
		updateSASGameRecordStateValue(uiHash, kPlayer.AI_getAttackOddsChange());
		updateSASGameRecordStateValue(uiHash, kPlayer.AI_getExtraGoldTarget());
		updateSASGameRecordStateValue(uiHash, kPlayer.AI_getCivicTimer());
		updateSASGameRecordStateValue(uiHash, kPlayer.AI_getReligionTimer());
		for (int iI = 0; iI < NUM_UNITAI_TYPES; iI++)
		{
			updateSASGameRecordStateValue(uiHash, kPlayer.AI_getNumAIUnits((UnitAITypes)iI));
			updateSASGameRecordStateValue(uiHash, kPlayer.AI_getNumTrainAIUnits((UnitAITypes)iI));
		}
		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			PlayerTypes const eOther = (PlayerTypes)iI;
			updateSASGameRecordStateValue(uiHash, kPlayer.AI_getAttitudeExtra(eOther));
			updateSASGameRecordStateValue(uiHash, kPlayer.AI_getSameReligionCounter(eOther));
			updateSASGameRecordStateValue(uiHash, kPlayer.AI_getDifferentReligionCounter(eOther));
			updateSASGameRecordStateValue(uiHash, kPlayer.AI_getFavoriteCivicCounter(eOther));
			updateSASGameRecordStateValue(uiHash, kPlayer.AI_getBonusTradeCounter(eOther));
			updateSASGameRecordStateValue(uiHash, kPlayer.AI_getPeacetimeTradeValue(eOther));
			updateSASGameRecordStateValue(uiHash, kPlayer.AI_getPeacetimeGrantValue(eOther));
			updateSASGameRecordStateValue(uiHash, kPlayer.AI_getGoldTradedTo(eOther));
			updateSASGameRecordStateValue(uiHash, kPlayer.AI_isFirstContact(eOther));
			for (int iJ = 0; iJ < NUM_MEMORY_TYPES; iJ++)
				updateSASGameRecordStateValue(uiHash, kPlayer.AI_getMemoryCount(eOther, (MemoryTypes)iJ));
			for (int iJ = 0; iJ < NUM_CONTACT_TYPES; iJ++)
				updateSASGameRecordStateValue(uiHash, kPlayer.AI_getContactTimer(eOther, (ContactTypes)iJ));
		}
	}
	return uiHash;
}

static SASGameRecordStateObjectHash getSASGameRecordCityStateSignature(CvCity const& kCity)
{
	SASGameRecordStateObjectHash uiHash;
	updateSASGameRecordStateValue(uiHash, kCity.getOwner());
	updateSASGameRecordStateValue(uiHash, kCity.getID());
	updateSASGameRecordStateValue(uiHash, kCity.getX());
	updateSASGameRecordStateValue(uiHash, kCity.getY());
	updateSASGameRecordStateValue(uiHash, kCity.getGameTurnFounded());
	updateSASGameRecordStateValue(uiHash, kCity.getGameTurnAcquired());
	updateSASGameRecordStateValue(uiHash, kCity.getPreviousOwner());
	updateSASGameRecordStateValue(uiHash, kCity.getOriginalOwner());
	updateSASGameRecordStateValue(uiHash, kCity.getPopulation());
	updateSASGameRecordStateValue(uiHash, kCity.getWorkingPopulation());
	updateSASGameRecordStateValue(uiHash, kCity.getSpecialistPopulation());
	updateSASGameRecordStateValue(uiHash, kCity.getFood());
	updateSASGameRecordStateValue(uiHash, kCity.getFoodKept());
	updateSASGameRecordStateValue(uiHash, kCity.getGreatPeopleProgress());
	updateSASGameRecordStateValue(uiHash, kCity.getProduction());
	updateSASGameRecordStateValue(uiHash, kCity.getOverflowProduction());
	updateSASGameRecordStateValue(uiHash, kCity.getFeatureProduction());
	updateSASGameRecordStateValue(uiHash, kCity.getMaintenanceTimes100());
	// <!-- custom: Base-yield and commerce arrays are direct cached gameplay state used by later economy/AI calculations.
	// Retain them cheaply so stale-cache divergences are visible even when their underlying plots/buildings still match. (ChatGPT-5.6-Sol) -->
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
		updateSASGameRecordStateValue(uiHash, kCity.getBaseYieldRate((YieldTypes)iI));
	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
		updateSASGameRecordStateValue(uiHash, kCity.getCommerceRateTimes100((CommerceTypes)iI));
	updateSASGameRecordStateValue(uiHash, kCity.getCultureLevel());
	updateSASGameRecordStateValue(uiHash, kCity.getOccupationTimer());
	updateSASGameRecordStateValue(uiHash, kCity.getCultureUpdateTimer());
	updateSASGameRecordStateValue(uiHash, kCity.getDefenseDamage());
	updateSASGameRecordStateValue(uiHash, kCity.getExtraHappiness());
	updateSASGameRecordStateValue(uiHash, kCity.getExtraHealth());
	updateSASGameRecordStateValue(uiHash, kCity.getHurryAngerTimer());
	updateSASGameRecordStateValue(uiHash, kCity.getConscriptAngerTimer());
	updateSASGameRecordStateValue(uiHash, kCity.getDefyResolutionAngerTimer());
	updateSASGameRecordStateValue(uiHash, kCity.getHappinessTimer());
	updateSASGameRecordStateValue(uiHash, kCity.isBombarded());
	updateSASGameRecordStateValue(uiHash, kCity.isDrafted());
	updateSASGameRecordStateValue(uiHash, kCity.isAirliftTargeted());
	updateSASGameRecordStateValue(uiHash, kCity.isPlundered());
	updateSASGameRecordStateValue(uiHash, kCity.isWeLoveTheKingDay());
	updateSASGameRecordStateValue(uiHash, kCity.isCitizensAutomated());
	updateSASGameRecordStateValue(uiHash, kCity.isProductionAutomated());
	CvPlot const* pRallyPlot = kCity.getRallyPlot();
	updateSASGameRecordStateValue(uiHash, pRallyPlot == NULL ? -1 : pRallyPlot->getX());
	updateSASGameRecordStateValue(uiHash, pRallyPlot == NULL ? -1 : pRallyPlot->getY());
	// <!-- custom: Current trade-city assignments are serialized and can affect commerce/blockade behavior before aggregate city yields visibly diverge.
	// Hash the fixed loaded-mod route slots by stable owner/city identity. (ChatGPT-5.6-Sol) -->
	int const iMaxTradeRoutes = GC.getDefineINT(CvGlobals::MAX_TRADE_ROUTES);
	updateSASGameRecordStateValue(uiHash, iMaxTradeRoutes);
	for (int iI = 0; iI < iMaxTradeRoutes; iI++)
	{
		CvCity const* pTradeCity = kCity.getTradeCity(iI);
		updateSASGameRecordStateValue(uiHash, pTradeCity == NULL ? NO_PLAYER : pTradeCity->getOwner());
		updateSASGameRecordStateValue(uiHash, pTradeCity == NULL ? -1 : pTradeCity->getID());
	}
	updateSASGameRecordStateValue(uiHash, kCity.getOrderQueueLength());
	for (CLLNode<OrderData> const* pNode = kCity.headOrderQueueNode(); pNode != NULL; pNode = kCity.nextOrderQueueNode(pNode))
	{
		OrderData const& kOrder = pNode->m_data;
		updateSASGameRecordStateValue(uiHash, kOrder.eOrderType);
		updateSASGameRecordStateValue(uiHash, kOrder.iData1);
		updateSASGameRecordStateValue(uiHash, kOrder.iData2);
		updateSASGameRecordStateValue(uiHash, kOrder.bSave);
	}
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
		updateSASGameRecordStateValue(uiHash, kCity.getCultureTimes100((PlayerTypes)iI));
	for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
		updateSASGameRecordStateValue(uiHash, kCity.isHasReligion((ReligionTypes)iI));
	for (int iI = 0; iI < GC.getNumCorporationInfos(); iI++)
		updateSASGameRecordStateValue(uiHash, kCity.isHasCorporation((CorporationTypes)iI));
	for (int iI = 0; iI < GC.getNumEventInfos(); iI++)
		updateSASGameRecordStateValue(uiHash, kCity.isEventOccured((EventTypes)iI));
	// <!-- custom: Worked BFC assignments, forced specialists and emphasis/automation choices are cheap durable decision state.
	// Hashing them can reveal a deterministic city-management divergence before population, food or production totals visibly separate. (ChatGPT-5.6-Sol) -->
	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
		updateSASGameRecordStateValue(uiHash, kCity.isWorkingPlot((CityPlotTypes)iI));
	for (int iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
	{
		SpecialistTypes const eSpecialist = (SpecialistTypes)iI;
		updateSASGameRecordStateValue(uiHash, kCity.getSpecialistCount(eSpecialist));
		updateSASGameRecordStateValue(uiHash, kCity.getForceSpecialistCount(eSpecialist));
		updateSASGameRecordStateValue(uiHash, kCity.getFreeSpecialistCount(eSpecialist));
	}
	CvCityAI const& kCityAI = kCity.AI();
	for (int iI = 0; iI < GC.getNumEmphasizeInfos(); iI++)
		updateSASGameRecordStateValue(uiHash, kCityAI.AI_isEmphasize((EmphasizeTypes)iI));
	updateSASGameRecordStateValue(uiHash, kCityAI.AI_isStrongEmphasis());
	for (int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		BuildingTypes const eBuilding = (BuildingTypes)iI;
		int const iReal = kCity.getNumRealBuilding(eBuilding);
		int const iFree = kCity.getNumFreeBuilding(eBuilding);
		updateSASGameRecordStateValue(uiHash, iReal);
		updateSASGameRecordStateValue(uiHash, iFree);
		// <!-- custom: Original owner/time can affect retained culture and time-based building effects; only existing buildings need these extra values. (ChatGPT-5.6-Sol) -->
		if (iReal > 0 || iFree > 0)
		{
			updateSASGameRecordStateValue(uiHash, kCity.getBuildingOriginalOwner(eBuilding));
			updateSASGameRecordStateValue(uiHash, kCity.getBuildingOriginalTime(eBuilding));
		}
	}
	// <!-- custom: BuildingClass modifiers are persistent city state and may remain latent while the affected building class is absent.
	// Preserve their exact stored values rather than relying on current building/output state to imply them. (ChatGPT-5.6-Sol) -->
	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingClassTypes const eClass = (BuildingClassTypes)iI;
		for (int iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
			updateSASGameRecordStateValue(uiHash, kCity.getBuildingYieldChange(eClass, (YieldTypes)iJ));
		for (int iJ = 0; iJ < NUM_COMMERCE_TYPES; iJ++)
			updateSASGameRecordStateValue(uiHash, kCity.getBuildingCommerceChange(eClass, (CommerceTypes)iJ));
		updateSASGameRecordStateValue(uiHash, kCity.getBuildingHappyChange(eClass));
		updateSASGameRecordStateValue(uiHash, kCity.getBuildingHealthChange(eClass));
	}
	return uiHash;
}

static SASGameRecordStateObjectHash getSASGameRecordUnitStateSignature(CvUnitAI const& kUnit)
{
	SASGameRecordStateObjectHash uiHash;
	updateSASGameRecordStateValue(uiHash, kUnit.getOwner());
	updateSASGameRecordStateValue(uiHash, kUnit.getID());
	updateSASGameRecordStateValue(uiHash, kUnit.getUnitType());
	updateSASGameRecordStateValue(uiHash, kUnit.getX());
	updateSASGameRecordStateValue(uiHash, kUnit.getY());
	if (kUnit.getUnitType() == NO_UNIT)
		return uiHash;
	updateSASGameRecordStateValue(uiHash, kUnit.getGameTurnCreated());
	updateSASGameRecordStateValue(uiHash, kUnit.getDamage());
	updateSASGameRecordStateValue(uiHash, kUnit.getMoves());
	updateSASGameRecordStateValue(uiHash, kUnit.getExperience());
	updateSASGameRecordStateValue(uiHash, kUnit.getLevel());
	updateSASGameRecordStateValue(uiHash, kUnit.getGroupID());
	updateSASGameRecordStateValue(uiHash, kUnit.getCargo());
	updateSASGameRecordStateValue(uiHash, kUnit.getImmobileTimer());
	updateSASGameRecordStateValue(uiHash, kUnit.getFortifyTurns());
	updateSASGameRecordStateValue(uiHash, kUnit.isMadeAttack());
	updateSASGameRecordStateValue(uiHash, kUnit.isMadeInterception());
	updateSASGameRecordStateValue(uiHash, kUnit.isBlockading());
	updateSASGameRecordStateValue(uiHash, kUnit.AI_getUnitAIType());
	updateSASGameRecordStateValue(uiHash, kUnit.AI_getBirthmark());
	CvUnit const* pTransport = kUnit.getTransportUnit();
	updateSASGameRecordStateValue(uiHash, pTransport == NULL ? NO_PLAYER : pTransport->getOwner());
	updateSASGameRecordStateValue(uiHash, pTransport == NULL ? -1 : pTransport->getID());
	for (int iI = 0; iI < GC.getNumPromotionInfos(); iI++)
		updateSASGameRecordStateValue(uiHash, kUnit.isHasPromotion((PromotionTypes)iI));
	return uiHash;
}

static SASGameRecordStateObjectHash getSASGameRecordGroupStateSignature(CvSelectionGroupAI const& kGroup)
{
	SASGameRecordStateObjectHash uiHash;
	updateSASGameRecordStateValue(uiHash, kGroup.getOwner());
	updateSASGameRecordStateValue(uiHash, kGroup.getID());
	updateSASGameRecordStateValue(uiHash, kGroup.getNumUnits());
	// <!-- custom: Preserve the group's ordered member list, not only each unit's group ID; selection/combat code can observe linked-list order before any unit position or membership changes. (ChatGPT-5.6-Sol) -->
	for (CLLNode<IDInfo> const* pNode = kGroup.headUnitNode(); pNode != NULL; pNode = kGroup.nextUnitNode(pNode))
	{
		updateSASGameRecordStateValue(uiHash, pNode->m_data.eOwner);
		updateSASGameRecordStateValue(uiHash, pNode->m_data.iID);
	}
	updateSASGameRecordStateValue(uiHash, kGroup.getActivityType());
	updateSASGameRecordStateValue(uiHash, kGroup.getAutomateType());
	updateSASGameRecordStateValue(uiHash, kGroup.getLengthMissionQueue());
	updateSASGameRecordStateValue(uiHash, kGroup.AI_getMissionAIType());
	updateSASGameRecordStateValue(uiHash, kGroup.AI_isForceSeparate());
	CvPlot const* pMissionPlot = kGroup.AI_getMissionAIPlot();
	updateSASGameRecordStateValue(uiHash, pMissionPlot == NULL ? -1 : pMissionPlot->getX());
	updateSASGameRecordStateValue(uiHash, pMissionPlot == NULL ? -1 : pMissionPlot->getY());
	CvUnitAI const* pMissionUnit = kGroup.AI_getMissionAIUnit();
	updateSASGameRecordStateValue(uiHash, pMissionUnit == NULL ? NO_PLAYER : pMissionUnit->getOwner());
	updateSASGameRecordStateValue(uiHash, pMissionUnit == NULL ? -1 : pMissionUnit->getID());
	for (CLLNode<MissionData>* pNode = kGroup.headMissionQueueNode(); pNode != NULL; pNode = kGroup.nextMissionQueueNode(pNode))
	{
		MissionData const& kMission = pNode->m_data;
		updateSASGameRecordStateValue(uiHash, kMission.eMissionType);
		updateSASGameRecordStateValue(uiHash, kMission.iData1);
		updateSASGameRecordStateValue(uiHash, kMission.iData2);
		updateSASGameRecordStateValue(uiHash, kMission.eFlags);
		updateSASGameRecordStateValue(uiHash, kMission.iPushTurn);
		updateSASGameRecordStateValue(uiHash, kMission.bModified);
	}
	return uiHash;
}

static SASGameRecordStateObjectHash getSASGameRecordPlotStateSignature(CvPlot const& kPlot, int iPlotIndex)
{
	SASGameRecordStateObjectHash uiHash;
	updateSASGameRecordStateValue(uiHash, iPlotIndex);
	// <!-- custom: Area partitioning is future-consequential for pathing and AI logic even when a plot's visible terrain/ownership has not changed. (ChatGPT-5.6-Sol) -->
	updateSASGameRecordStateValue(uiHash, kPlot.getArea().getID());
	updateSASGameRecordStateValue(uiHash, kPlot.getPlotType());
	updateSASGameRecordStateValue(uiHash, kPlot.getTerrainType());
	updateSASGameRecordStateValue(uiHash, kPlot.getFeatureType());
	updateSASGameRecordStateValue(uiHash, kPlot.getBonusType());
	updateSASGameRecordStateValue(uiHash, kPlot.getImprovementType());
	updateSASGameRecordStateValue(uiHash, kPlot.getRouteType());
	updateSASGameRecordStateValue(uiHash, kPlot.getOwner());
	CvCity const* pCity = kPlot.getPlotCity();
	// <!-- custom: AdvCiv contested-border ownership and forced-unowned countdowns can change future territory without changing the current primary owner yet.
	// During new-game/load rollover, the old plot can retain its city identifier after that city object no longer resolves; avoid inherited getSecondOwner's city-pointer dereference while fingerprinting the closing diagnostic session. See KI#382.2. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	PlayerTypes const eSecondOwner = (pCity != NULL ? pCity->getOwner() : (kPlot.isCity() ? NO_PLAYER : kPlot.getSecondOwner()));
	updateSASGameRecordStateValue(uiHash, eSecondOwner);
	updateSASGameRecordStateValue(uiHash, kPlot.getForceUnownedTimer());
	updateSASGameRecordStateValue(uiHash, kPlot.getOwnershipDuration());
	updateSASGameRecordStateValue(uiHash, kPlot.getImprovementDuration());
	updateSASGameRecordStateValue(uiHash, kPlot.getUpgradeProgress());
	updateSASGameRecordStateValue(uiHash, kPlot.isNOfRiver());
	updateSASGameRecordStateValue(uiHash, kPlot.getRiverNSDirection());
	updateSASGameRecordStateValue(uiHash, kPlot.isWOfRiver());
	updateSASGameRecordStateValue(uiHash, kPlot.getRiverWEDirection());
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
		updateSASGameRecordStateValue(uiHash, GC.getMap().getPlotExtraYield(kPlot, (YieldTypes)iI));
	updateSASGameRecordStateValue(uiHash, pCity == NULL ? NO_PLAYER : pCity->getOwner());
	updateSASGameRecordStateValue(uiHash, pCity == NULL ? -1 : pCity->getID());
	return uiHash;
}

static void updateSASGameRecordDealTradeState(SASGameRecordStateObjectHash& uiHash, CLinkList<TradeData> const& kList)
{
	updateSASGameRecordStateValue(uiHash, kList.getLength());
	for (CLLNode<TradeData> const* pNode = kList.head(); pNode != NULL; pNode = CLinkList<TradeData>::static_next(pNode))
	{
		TradeData const& kTrade = pNode->m_data;
		updateSASGameRecordStateValue(uiHash, kTrade.m_eItemType);
		updateSASGameRecordStateValue(uiHash, kTrade.m_iData);
		updateSASGameRecordStateValue(uiHash, kTrade.m_bOffering);
		updateSASGameRecordStateValue(uiHash, kTrade.m_bHidden);
	}
}

static SASGameRecordStateObjectHash getSASGameRecordDealStateSignature(CvDeal const& kDeal)
{
	SASGameRecordStateObjectHash uiHash;
	updateSASGameRecordStateValue(uiHash, kDeal.getID());
	updateSASGameRecordStateValue(uiHash, kDeal.getInitialGameTurn());
	updateSASGameRecordStateValue(uiHash, kDeal.getFirstPlayer());
	updateSASGameRecordStateValue(uiHash, kDeal.getSecondPlayer());
	updateSASGameRecordDealTradeState(uiHash, kDeal.getFirstList());
	updateSASGameRecordDealTradeState(uiHash, kDeal.getSecondList());
	return uiHash;
}

static SASGameRecordStateFingerprints getSASGameRecordStateFingerprints()
{
	SASGameRecordStateFingerprints kResult;
	CvGame& kGame = GC.getGame();
	updateSASGameRecordStateObject(kResult.uiGame, getSASGameRecordGameStateSignature(kGame));
	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		CvTeamAI const& kTeam = GET_TEAM((TeamTypes)iI);
		updateSASGameRecordStateObject(kResult.uiTeams, getSASGameRecordTeamStateSignature(kTeam));
		if (kTeam.isEverAlive())
			kResult.iEverAliveTeamCount++;
	}
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayerAI const& kPlayer = GET_PLAYER((PlayerTypes)iI);
		updateSASGameRecordStateObject(kResult.uiPlayers, getSASGameRecordPlayerStateSignature(kPlayer));
		if (!kPlayer.isEverAlive())
			continue;
		kResult.iEverAlivePlayerCount++;
		FOR_EACH_CITY(pCity, kPlayer)
		{
			updateSASGameRecordStateObject(kResult.uiCities, getSASGameRecordCityStateSignature(*pCity));
			kResult.iCityCount++;
		}
		FOR_EACH_UNITAI(pUnit, kPlayer)
		{
			updateSASGameRecordStateObject(kResult.uiUnits, getSASGameRecordUnitStateSignature(*pUnit));
			kResult.iUnitCount++;
		}
		FOR_EACH_GROUPAI(pGroup, kPlayer)
		{
			updateSASGameRecordStateObject(kResult.uiGroups, getSASGameRecordGroupStateSignature(*pGroup));
			kResult.iGroupCount++;
		}
	}
	CvMap const& kMap = GC.getMap();
	kResult.iPlotCount = (int)kMap.numPlots();
	for (int iI = 0; iI < kResult.iPlotCount; iI++)
		updateSASGameRecordStateObject(kResult.uiPlots, getSASGameRecordPlotStateSignature(kMap.getPlotByIndex(iI), iI));
	int iDealIter = 0;
	for (CvDeal const* pDeal = kGame.firstDeal(&iDealIter); pDeal != NULL; pDeal = kGame.nextDeal(&iDealIter))
	{
		updateSASGameRecordStateObject(kResult.uiDeals, getSASGameRecordDealStateSignature(*pDeal));
		kResult.iDealCount++;
	}
	updateSASGameRecordStateHash64(kResult.uiCombined, kResult.uiGame);
	updateSASGameRecordStateHash64(kResult.uiCombined, kResult.uiTeams);
	updateSASGameRecordStateHash64(kResult.uiCombined, kResult.uiPlayers);
	updateSASGameRecordStateHash64(kResult.uiCombined, kResult.uiCities);
	updateSASGameRecordStateHash64(kResult.uiCombined, kResult.uiUnits);
	updateSASGameRecordStateHash64(kResult.uiCombined, kResult.uiGroups);
	updateSASGameRecordStateHash64(kResult.uiCombined, kResult.uiPlots);
	updateSASGameRecordStateHash64(kResult.uiCombined, kResult.uiDeals);
	updateSASGameRecordFNV1AUInt32(kResult.uiCombined, (unsigned int)kResult.iEverAliveTeamCount);
	updateSASGameRecordFNV1AUInt32(kResult.uiCombined, (unsigned int)kResult.iEverAlivePlayerCount);
	updateSASGameRecordFNV1AUInt32(kResult.uiCombined, (unsigned int)kResult.iCityCount);
	updateSASGameRecordFNV1AUInt32(kResult.uiCombined, (unsigned int)kResult.iUnitCount);
	updateSASGameRecordFNV1AUInt32(kResult.uiCombined, (unsigned int)kResult.iGroupCount);
	updateSASGameRecordFNV1AUInt32(kResult.uiCombined, (unsigned int)kResult.iPlotCount);
	updateSASGameRecordFNV1AUInt32(kResult.uiCombined, (unsigned int)kResult.iDealCount);
	return kResult;
}

static void logSASGameRecordStateCheckpoint(int iGameTurn, char const* szReason)
{
#ifdef FASSERT_ENABLE
	// <!-- custom: These values are assertion-only; gate their reads too because Debug-opt/Release compile FAssertMsg away and treat the resulting unused-local warnings as errors. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	unsigned int const uiMapSeedBefore = GC.getGame().getMapRand().getSeed();
	unsigned int const uiSyncSeedBefore = GC.getGame().getSorenRand().getSeed();
#endif
	uint const uiStart = getSASMonotonicMilliseconds();
	SASGameRecordStateFingerprints const kState = getSASGameRecordStateFingerprints();
	uint const uiEnd = getSASMonotonicMilliseconds();
#ifdef FASSERT_ENABLE
	// <!-- custom: Fingerprinting must stay observational; all inputs above are direct/stable getters; this assertion guards future maintenance from accidentally introducing a helper that consumes authoritative RNG. (ChatGPT-5.6-Sol) -->
	FAssertMsg(uiMapSeedBefore == GC.getGame().getMapRand().getSeed() && uiSyncSeedBefore == GC.getGame().getSorenRand().getSeed(), "SASGameRecord state fingerprint consumed authoritative RNG");
#endif
	logSASGameRecord("GAME_RECORD_STATE_CHECKPOINT turn=%d reason=%s coverage=CORE everAliveTeamCount=%d everAlivePlayerCount=%d cityCount=%d unitCount=%d groupCount=%d plotCount=%d dealCount=%d gameFingerprint=FNV1A64:%016I64X teamsFingerprint=FNV1A64:%016I64X playersFingerprint=FNV1A64:%016I64X citiesFingerprint=FNV1A64:%016I64X unitsFingerprint=FNV1A64:%016I64X groupsFingerprint=FNV1A64:%016I64X plotsFingerprint=FNV1A64:%016I64X dealsFingerprint=FNV1A64:%016I64X combinedFingerprint=FNV1A64:%016I64X computeMilliseconds=%u",
		iGameTurn, szReason, kState.iEverAliveTeamCount, kState.iEverAlivePlayerCount, kState.iCityCount, kState.iUnitCount,
		kState.iGroupCount, kState.iPlotCount, kState.iDealCount, kState.uiGame, kState.uiTeams, kState.uiPlayers, kState.uiCities,
		kState.uiUnits, kState.uiGroups, kState.uiPlots, kState.uiDeals, kState.uiCombined, getSASElapsedMilliseconds(uiStart, uiEnd));
}

static unsigned int getSASGameRecordRandomMessageHash(TCHAR const* szLog, unsigned int& uiLength)
{
	unsigned int uiHash = 2166136261u;
	uiLength = 0;
	if (szLog == NULL)
		return uiHash;
	// <!-- custom: Source-location labels can be long and RNG calls are hot.
	// Reduce the message with cheap 32-bit FNV first instead of doing an emulated 64-bit multiply for every character on Civ4's 32-bit build. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (sizeof(TCHAR) == 1)
	{
		for (TCHAR const* pChar = szLog; *pChar != 0; pChar++, uiLength++)
			updateSASGameRecordFNV1A32Byte(uiHash, (unsigned char)*pChar);
	}
	else
	{
		for (TCHAR const* pChar = szLog; *pChar != 0; pChar++, uiLength++)
		{
			unsigned short const usChar = (unsigned short)*pChar;
			updateSASGameRecordFNV1A32Byte(uiHash, (unsigned char)(usChar & 0xFF));
			updateSASGameRecordFNV1A32Byte(uiHash, (unsigned char)(usChar >> 8));
		}
	}
	return uiHash;
}

static unsigned int getSASGameRecordRandomCallSignature(unsigned short usRange, TCHAR const* szLog, int iData1, int iData2, bool bExternal)
{
	// <!-- custom: Collapse each detailed call context with cheap 32-bit FNV first; feed that token into the 64-bit session fingerprint and native 32-bit interval fingerprint.
	// This keeps detailed provenance without duplicating the expensive 64-bit accumulator on Civ4's 32-bit build. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	unsigned int uiHash = 2166136261u;
	updateSASGameRecordFNV1A32Byte(uiHash, 0xA7); // <!-- custom: Detailed ROLL boundary/version marker. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	updateSASGameRecordFNV1A32UInt16(uiHash, usRange);
	updateSASGameRecordFNV1A32UInt32(uiHash, (unsigned int)iData1);
	updateSASGameRecordFNV1A32UInt32(uiHash, (unsigned int)iData2);
	updateSASGameRecordFNV1A32Byte(uiHash, bExternal ? 1 : 0);
	if (szLog == NULL)
		updateSASGameRecordFNV1A32Byte(uiHash, 0);
	else
	{
		updateSASGameRecordFNV1A32Byte(uiHash, 1);
		unsigned int uiMessageLength = 0;
		unsigned int const uiMessageHash = getSASGameRecordRandomMessageHash(szLog, uiMessageLength);
		updateSASGameRecordFNV1A32UInt32(uiHash, uiMessageLength);
		updateSASGameRecordFNV1A32UInt32(uiHash, uiMessageHash);
	}
	return uiHash;
}

static unsigned int getSASGameRecordRandomSeedSetCallSignature(unsigned int uiOldState, unsigned int uiNewState, bool bReseed)
{
	unsigned int uiHash = 2166136261u;
	updateSASGameRecordFNV1A32Byte(uiHash, 0xA8); // <!-- custom: Detailed SEED_SET boundary/version marker. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	updateSASGameRecordFNV1A32UInt32(uiHash, uiOldState);
	updateSASGameRecordFNV1A32UInt32(uiHash, uiNewState);
	updateSASGameRecordFNV1A32Byte(uiHash, bReseed ? 1 : 0);
	return uiHash;
}

// <!-- custom: CvRandom callers already pre-gate on active level-3 tracking; returning NULL here is a separate stream-identity filter that rejects async and temporary/local RNG objects while retaining only CvGame's authoritative map and synchronized streams. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
static SASGameRecordRngTracker* getSASGameRecordRngTracker(CvRandom const* pRandom)
{
	if (pRandom == g_pSASGameRecordMapRng)
		return &g_kSASGameRecordMapRng;
	if (pRandom == g_pSASGameRecordSyncRng)
		return &g_kSASGameRecordSyncRng;
	return NULL;
}

static void clearSASGameRecordRngTracking()
{
	g_bSASGameRecordRngTrackingActive = false;
	g_pSASGameRecordMapRng = NULL;
	g_pSASGameRecordSyncRng = NULL;
	g_kSASGameRecordMapRng.clear();
	g_kSASGameRecordSyncRng.clear();
}

void initializeSASGameRecordRngTracking()
{
	clearSASGameRecordRngTracking();
	CvGame& kGame = GC.getGame();
	g_pSASGameRecordMapRng = &kGame.getMapRand();
	g_pSASGameRecordSyncRng = &kGame.getSorenRand();
	g_kSASGameRecordMapRng.initialize(kGame.getMapRand().getSeed());
	g_kSASGameRecordSyncRng.initialize(kGame.getSorenRand().getSeed());
	g_bSASGameRecordRngTrackingActive = true;
}

void noteSASGameRecordExternalRandomCall(CvRandom const* pRandom)
{
	SASGameRecordRngTracker* pTracker = getSASGameRecordRngTracker(pRandom);
	if (pTracker == NULL || !pTracker->bInitialized)
		return;
	// <!-- custom: getExternal immediately calls get/getInt after this marker; retain EXE origin in both counters and that next call's fingerprint. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	pTracker->bNextCallExternal = true;
}

void noteSASGameRecordRandomCall(CvRandom const* pRandom, unsigned short usRange, TCHAR const* szLog, int iData1, int iData2)
{
	SASGameRecordRngTracker* pTracker = getSASGameRecordRngTracker(pRandom);
	if (pTracker == NULL || !pTracker->bInitialized)
		return;
	bool const bExternal = pTracker->bNextCallExternal;
	pTracker->bNextCallExternal = false;
	// <!-- custom: Hash the abstract stream operation directly, avoiding a nested signature. The requested range is intrinsically 16-bit, so hash exactly those two bytes rather than doing redundant work on two guaranteed-zero bytes.
	// Keep cumulative/session hashes 64-bit, but use native 32-bit FNV for interval copies; checkpoint state/counters plus both hashes make that cheaper signal sufficiently strong for interval-local diagnosis. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	updateSASGameRecordFNV1AByte(pTracker->uiSessionStreamFingerprint, 0x52);
	updateSASGameRecordFNV1AUInt16(pTracker->uiSessionStreamFingerprint, usRange);
	updateSASGameRecordFNV1A32Byte(pTracker->uiIntervalStreamFingerprint, 0x52);
	updateSASGameRecordFNV1A32UInt16(pTracker->uiIntervalStreamFingerprint, usRange);
	unsigned int const uiCallSignature = getSASGameRecordRandomCallSignature(usRange, szLog, iData1, iData2, bExternal);
	updateSASGameRecordFNV1AUInt32(pTracker->uiSessionCallFingerprint, uiCallSignature);
	updateSASGameRecordFNV1A32UInt32(pTracker->uiIntervalCallFingerprint, uiCallSignature);
	pTracker->uiSessionCalls++;
	pTracker->uiIntervalCalls++;
	if (szLog == NULL)
	{
		pTracker->uiSessionNullMessageCalls++;
		pTracker->uiIntervalNullMessageCalls++;
	}
	if (bExternal)
	{
		pTracker->uiSessionExternalCalls++;
		pTracker->uiIntervalExternalCalls++;
	}
	// <!-- custom: A low-level range <=1 still advances the seed despite yielding no entropy (the ordinary public range-0 wrapper returns before reaching here, so this is normally range 1). CvRandom::shuffle deliberately reaches range 1 on its final iteration.
	// Count rather than "optimizing" such calls away because their seed consumption is part of Civ4's synchronized RNG sequence and shifts every later result. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (usRange <= 1)
	{
		pTracker->uiSessionDeterministicRangeCalls++;
		pTracker->uiIntervalDeterministicRangeCalls++;
	}
}

void noteSASGameRecordRandomSeedSet(CvRandom const* pRandom, unsigned int uiOldState, unsigned int uiNewState, bool bReseed)
{
	SASGameRecordRngTracker* pTracker = getSASGameRecordRngTracker(pRandom);
	if (pTracker == NULL || !pTracker->bInitialized)
		return;
	pTracker->bNextCallExternal = false; // <!-- custom: A seed replacement cannot inherit a pending EXE-roll classification. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	// <!-- custom: A seed replacement is part of the authoritative RNG operation stream even though it consumes no roll. This is rare (notably benchmark Python can call CyRandom.init mid-session), so retain an explicit row too.
	// Hash it into the value-stream fingerprint only when it actually changes state: redundantly assigning the current seed changes call provenance but not any present/future random values, so it belongs only in the richer call fingerprint/counters below. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (uiOldState != uiNewState)
	{
		updateSASGameRecordFNV1AByte(pTracker->uiSessionStreamFingerprint, 0x53);
		updateSASGameRecordFNV1AUInt32(pTracker->uiSessionStreamFingerprint, uiNewState);
		updateSASGameRecordFNV1A32Byte(pTracker->uiIntervalStreamFingerprint, 0x53);
		updateSASGameRecordFNV1A32UInt32(pTracker->uiIntervalStreamFingerprint, uiNewState);
	}
	unsigned int const uiCallSignature = getSASGameRecordRandomSeedSetCallSignature(uiOldState, uiNewState, bReseed);
	updateSASGameRecordFNV1AUInt32(pTracker->uiSessionCallFingerprint, uiCallSignature);
	updateSASGameRecordFNV1A32UInt32(pTracker->uiIntervalCallFingerprint, uiCallSignature);
	pTracker->uiSessionSeedSets++;
	pTracker->uiIntervalSeedSets++;
	char const* szStream = (pTracker == &g_kSASGameRecordMapRng ? "MAP" : "SYNC");
	// <!-- custom: Position the rare replacement precisely within both the current checkpoint interval and recorder session.
	// This lets external tooling reconstruct seed progression around a mid-turn benchmark/Python reseed without per-roll SASGameRecord rows. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_RNG_SEED_SET turn=%d stream=%s operation=%s oldState=%u newState=%u intervalCalls=%I64u sessionCalls=%I64u intervalSeedSets=%I64u sessionSeedSets=%I64u",
		GC.getGame().getGameTurn(), szStream, bReseed ? "RESEED" : "RESET_OR_INIT", uiOldState, uiNewState, pTracker->uiIntervalCalls,
		pTracker->uiSessionCalls, pTracker->uiIntervalSeedSets, pTracker->uiSessionSeedSets);
}

// <!-- custom: Fixed checkpoint boundaries are an enum rather than caller-written strings, preventing silent spelling drift in rows consumed by cross-run comparison tooling.
// Convert that recorder-owned vocabulary to its stable schema text in one place. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
static char const* getSASGameRecordRngCheckpointReason(SASGameRecordRngCheckpointReason eReason)
{
	switch (eReason)
	{
	case SAS_RNG_CHECKPOINT_NEW_GAME_INITIALIZED: return "NEW_GAME_INITIALIZED";
	case SAS_RNG_CHECKPOINT_MAP_REGENERATION_BEGIN: return "MAP_REGENERATION_BEGIN";
	case SAS_RNG_CHECKPOINT_MAP_REGENERATION_END: return "MAP_REGENERATION_END";
	case SAS_RNG_CHECKPOINT_AUTOPLAY_BEGIN: return "AUTOPLAY_BEGIN";
	case SAS_RNG_CHECKPOINT_AUTOPLAY_END: return "AUTOPLAY_END";
	case SAS_RNG_CHECKPOINT_END_GAME_TURN: return "END_GAME_TURN";
	case SAS_RNG_CHECKPOINT_VICTORY: return "VICTORY";
	case SAS_RNG_CHECKPOINT_GAME_END: return "GAME_END";
	case SAS_RNG_CHECKPOINT_SAVE_LOADED: return "SAVE_LOADED";
	case SAS_RNG_CHECKPOINT_SESSION_FINALIZE: return "SESSION_FINALIZE";
	}
	FAssertMsg(false, "Unknown SASGameRecord RNG checkpoint reason");
	return "UNKNOWN";
}

// <!-- custom: Callers pre-gate on active tracking so disabled levels never enter this substantial snapshot path. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordRngCheckpoint(int iGameTurn, SASGameRecordRngCheckpointReason eReason)
{
	// <!-- custom: Flush the recorder's synthetic bombard sequence before sampling/resetting the RNG interval so future logging-side code cannot accidentally fall between the captured state and interval reset. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	flushSASGameRecordPendingCityBombard();
	CvGame& kGame = GC.getGame();
	SASGameRecordRngTracker& kMap = g_kSASGameRecordMapRng;
	SASGameRecordRngTracker& kSync = g_kSASGameRecordSyncRng;
	unsigned int const uiMapState = kGame.getMapRand().getSeed();
	unsigned int const uiSyncState = kGame.getSorenRand().getSeed();
	// <!-- custom: Snapshot the completed interval, then establish the next interval before writing the row.
	// If present or future logging unexpectedly consumes authoritative RNG, that consumption stays in the new interval instead of being counted and erased by a post-log reset. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	SASGameRecordRngTracker const kMapCompleted = kMap;
	SASGameRecordRngTracker const kSyncCompleted = kSync;
	kMap.resetInterval(uiMapState);
	kSync.resetInterval(uiSyncState);
	logSASGameRecord("GAME_RECORD_RNG_CHECKPOINT turn=%d reason=%s rngFingerprintSchema=1 mapSessionStartState=%u mapIntervalStartState=%u mapState=%u mapIntervalCalls=%I64u mapSessionCalls=%I64u mapIntervalNullMessageCalls=%I64u mapSessionNullMessageCalls=%I64u mapIntervalExternalCalls=%I64u mapSessionExternalCalls=%I64u mapIntervalDeterministicRangeCalls=%I64u mapSessionDeterministicRangeCalls=%I64u mapIntervalSeedSets=%I64u mapSessionSeedSets=%I64u mapIntervalStreamFingerprint=FNV1A32:%08X mapSessionStreamFingerprint=FNV1A64:%016I64X mapIntervalCallFingerprint=FNV1A32:%08X mapSessionCallFingerprint=FNV1A64:%016I64X syncSessionStartState=%u syncIntervalStartState=%u syncState=%u syncIntervalCalls=%I64u syncSessionCalls=%I64u syncIntervalNullMessageCalls=%I64u syncSessionNullMessageCalls=%I64u syncIntervalExternalCalls=%I64u syncSessionExternalCalls=%I64u syncIntervalDeterministicRangeCalls=%I64u syncSessionDeterministicRangeCalls=%I64u syncIntervalSeedSets=%I64u syncSessionSeedSets=%I64u syncIntervalStreamFingerprint=FNV1A32:%08X syncSessionStreamFingerprint=FNV1A64:%016I64X syncIntervalCallFingerprint=FNV1A32:%08X syncSessionCallFingerprint=FNV1A64:%016I64X",
		iGameTurn, getSASGameRecordRngCheckpointReason(eReason), kMapCompleted.uiSessionStartState, kMapCompleted.uiIntervalStartState,
		uiMapState, kMapCompleted.uiIntervalCalls, kMapCompleted.uiSessionCalls, kMapCompleted.uiIntervalNullMessageCalls,
		kMapCompleted.uiSessionNullMessageCalls, kMapCompleted.uiIntervalExternalCalls, kMapCompleted.uiSessionExternalCalls,
		kMapCompleted.uiIntervalDeterministicRangeCalls, kMapCompleted.uiSessionDeterministicRangeCalls, kMapCompleted.uiIntervalSeedSets,
		kMapCompleted.uiSessionSeedSets, kMapCompleted.uiIntervalStreamFingerprint, kMapCompleted.uiSessionStreamFingerprint,
		kMapCompleted.uiIntervalCallFingerprint, kMapCompleted.uiSessionCallFingerprint, kSyncCompleted.uiSessionStartState,
		kSyncCompleted.uiIntervalStartState, uiSyncState, kSyncCompleted.uiIntervalCalls, kSyncCompleted.uiSessionCalls,
		kSyncCompleted.uiIntervalNullMessageCalls, kSyncCompleted.uiSessionNullMessageCalls, kSyncCompleted.uiIntervalExternalCalls,
		kSyncCompleted.uiSessionExternalCalls, kSyncCompleted.uiIntervalDeterministicRangeCalls,
		kSyncCompleted.uiSessionDeterministicRangeCalls, kSyncCompleted.uiIntervalSeedSets, kSyncCompleted.uiSessionSeedSets,
		kSyncCompleted.uiIntervalStreamFingerprint, kSyncCompleted.uiSessionStreamFingerprint, kSyncCompleted.uiIntervalCallFingerprint,
		kSyncCompleted.uiSessionCallFingerprint);
	// <!-- custom: Reuse the exact same lifecycle boundary/reason for selected semantic gameplay state, so RNG-equal/state-different runs expose deterministic divergence immediately without adding another family of distant call sites. (ChatGPT-5.6-Sol) -->
	logSASGameRecordStateCheckpoint(iGameTurn, getSASGameRecordRngCheckpointReason(eReason));
}

// <!-- custom: Windows exposes focus separately from minimization. Find Civ4's visible, unowned top-level process window so background-visible and minimized snapshots remain distinguishable.
// Return -1 if no suitable window exists. (GPT-5.6-Sol) -->
struct SASGameRecordProcessWindowState
{
	SASGameRecordProcessWindowState() : bFound(false), bMinimized(false) {}
	bool bFound;
	bool bMinimized;
};

static BOOL CALLBACK findSASGameRecordProcessMainWindow(HWND hWindow, LPARAM lParam)
{
	DWORD uiProcessId = 0;
	GetWindowThreadProcessId(hWindow, &uiProcessId);
	if (uiProcessId != GetCurrentProcessId() || !IsWindowVisible(hWindow) || GetWindow(hWindow, GW_OWNER) != NULL)
		return TRUE;
	SASGameRecordProcessWindowState& kState = *(SASGameRecordProcessWindowState*)lParam;
	kState.bFound = true;
	kState.bMinimized = (IsIconic(hWindow) != FALSE);
	return FALSE;
}

static int getSASGameRecordProcessWindowMinimized()
{
	SASGameRecordProcessWindowState kState;
	EnumWindows(findSASGameRecordProcessMainWindow, (LPARAM)&kState);
	return kState.bFound ? (kState.bMinimized ? 1 : 0) : -1;
}

// <!-- custom: Native Win32 and Wine/Proton expose the same DLL APIs, but the distinction helps qualify support/performance results.
// Wine does not reliably expose whether its host is Linux or macOS, so do not guess beyond the compatibility layer. Preserve its own version string when available. (GPT-5.6-Sol + ChatGPT-5.6-Sol) -->
static FARPROC getSASGameRecordWineVersionProc()
{
	static HMODULE const hNtdll = GetModuleHandleA("ntdll.dll");
	static FARPROC const pWineVersion = (hNtdll == NULL ? NULL : GetProcAddress(hNtdll, "wine_get_version"));
	return pWineVersion;
}

static char const* getSASGameRecordWin32Runtime()
{
	return (getSASGameRecordWineVersionProc() == NULL ? "NATIVE_WINDOWS" : "WINE_OR_PROTON");
}

static CvString getSASGameRecordWineVersion()
{
	FARPROC const pProc = getSASGameRecordWineVersionProc();
	if (pProc == NULL)
		return CvString("-");
	typedef char const* (__cdecl* WineGetVersionFunction)();
	char const* const szVersion = ((WineGetVersionFunction)pProc)();
	return (szVersion == NULL || szVersion[0] == '\0' ? CvString("-") : CvString(szVersion));
}

struct SASGameRecordWindowsVersion
{
	SASGameRecordWindowsVersion() : iMajor(-1), iMinor(-1), iBuild(-1), iUpdateRevision(-1), iServicePackMajor(-1), iServicePackMinor(-1), szDisplayVersion("-") {}
	int iMajor;
	int iMinor;
	int iBuild;
	int iUpdateRevision;
	int iServicePackMajor;
	int iServicePackMinor;
	CvString szDisplayVersion;
};

struct SASGameRecordRegistryFunctions
{
	typedef LONG (WINAPI* RegOpenKeyExAFunction)(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
	typedef LONG (WINAPI* RegQueryValueExAFunction)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
	typedef LONG (WINAPI* RegCloseKeyFunction)(HKEY);
	SASGameRecordRegistryFunctions() : hAdvapi(LoadLibraryA("advapi32.dll")), pOpen(NULL), pQuery(NULL), pClose(NULL)
	{
		if (hAdvapi != NULL)
		{
			pOpen = (RegOpenKeyExAFunction)GetProcAddress(hAdvapi, "RegOpenKeyExA");
			pQuery = (RegQueryValueExAFunction)GetProcAddress(hAdvapi, "RegQueryValueExA");
			pClose = (RegCloseKeyFunction)GetProcAddress(hAdvapi, "RegCloseKey");
		}
	}
	HMODULE hAdvapi;
	RegOpenKeyExAFunction pOpen;
	RegQueryValueExAFunction pQuery;
	RegCloseKeyFunction pClose;
};

static SASGameRecordRegistryFunctions const& getSASGameRecordRegistryFunctions()
{
	static SASGameRecordRegistryFunctions const kFunctions;
	return kFunctions;
}

static bool openSASGameRecordWindowsVersionKey(HKEY& hKey)
{
	SASGameRecordRegistryFunctions const& kRegistry = getSASGameRecordRegistryFunctions();
	if (kRegistry.pOpen == NULL)
		return false;
	// <!-- custom: Prefer the native registry view even though Civ4 is a 32-bit process.
	// 0x0100 is KEY_WOW64_64KEY; use a literal so the old Civ4 SDK headers need not define it, then fall back for 32-bit Windows/Wine. (ChatGPT-5.6-Sol) -->
	if (kRegistry.pOpen(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE | 0x0100, &hKey) == ERROR_SUCCESS)
		return true;
	return (kRegistry.pOpen(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS);
}

static bool readSASGameRecordRegistryDword(HKEY hKey, char const* szName, int& iValue)
{
	SASGameRecordRegistryFunctions const& kRegistry = getSASGameRecordRegistryFunctions();
	if (kRegistry.pQuery == NULL)
		return false;
	DWORD uiType = 0;
	DWORD uiValue = 0;
	DWORD uiSize = sizeof(uiValue);
	if (kRegistry.pQuery(hKey, szName, NULL, &uiType, (BYTE*)&uiValue, &uiSize) != ERROR_SUCCESS || uiType != REG_DWORD || uiSize != sizeof(uiValue))
		return false;
	iValue = (int)uiValue;
	return true;
}

static bool readSASGameRecordRegistryString(HKEY hKey, char const* szName, CvString& szValue)
{
	SASGameRecordRegistryFunctions const& kRegistry = getSASGameRecordRegistryFunctions();
	if (kRegistry.pQuery == NULL)
		return false;
	DWORD uiType = 0;
	char szBuffer[128];
	DWORD uiSize = sizeof(szBuffer);
	if (kRegistry.pQuery(hKey, szName, NULL, &uiType, (BYTE*)szBuffer, &uiSize) != ERROR_SUCCESS || (uiType != REG_SZ && uiType != REG_EXPAND_SZ) || uiSize == 0)
		return false;
	szBuffer[sizeof(szBuffer) - 1] = '\0';
	szValue = szBuffer;
	return !szValue.empty();
}

// <!-- custom: Level-3 hardware context keeps only a coarse CPU vendor family, never the processor model/brand string.
// Read Windows' VendorIdentifier once and discard the raw value after classification; nativeArch remains available at level 2 even when hardware identity is disabled. (ChatGPT-5.6-Sol) -->
static char const* getSASGameRecordProcessorVendor()
{
	static CvString szVendorFamily;
	static bool bInitialized = false;
	if (bInitialized)
		return szVendorFamily.GetCString();
	bInitialized = true;
	szVendorFamily = "UNKNOWN";
	SASGameRecordRegistryFunctions const& kRegistry = getSASGameRecordRegistryFunctions();
	if (kRegistry.pOpen == NULL)
		return szVendorFamily.GetCString();
	HKEY hKey = NULL;
	if (kRegistry.pOpen(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
		return szVendorFamily.GetCString();
	CvString szVendor;
	if (readSASGameRecordRegistryString(hKey, "VendorIdentifier", szVendor))
	{
		char const* const szRaw = szVendor.GetCString();
		if (strstr(szRaw, "GenuineIntel") != NULL || strstr(szRaw, "Intel") != NULL) szVendorFamily = "INTEL";
		else if (strstr(szRaw, "AuthenticAMD") != NULL || strstr(szRaw, "AMD") != NULL) szVendorFamily = "AMD";
		else if (strstr(szRaw, "Qualcomm") != NULL || strstr(szRaw, "QUALCOMM") != NULL || strstr(szRaw, "QCOM") != NULL) szVendorFamily = "QUALCOMM";
		else if (strstr(szRaw, "Apple") != NULL || strstr(szRaw, "APPLE") != NULL) szVendorFamily = "APPLE";
		else if (strstr(szRaw, "ARM") != NULL || strstr(szRaw, "Arm") != NULL) szVendorFamily = "ARM";
		else if (strstr(szRaw, "Centaur") != NULL || strstr(szRaw, "VIA") != NULL) szVendorFamily = "VIA";
		else if (strstr(szRaw, "Hygon") != NULL || strstr(szRaw, "HYGON") != NULL) szVendorFamily = "HYGON";
		else if (strstr(szRaw, "Microsoft") != NULL || strstr(szRaw, "MICROSOFT") != NULL) szVendorFamily = "MICROSOFT";
		else szVendorFamily = "OTHER";
	}
	if (kRegistry.pClose != NULL)
		kRegistry.pClose(hKey);
	return szVendorFamily.GetCString();
}

static SASGameRecordWindowsVersion const& getSASGameRecordWindowsVersion()
{
	static SASGameRecordWindowsVersion kVersion;
	static bool bInitialized = false;
	if (bInitialized)
		return kVersion;
	bInitialized = true;

	// <!-- custom: GetVersionEx can be manifest/version-lied on modern Windows.
	// Resolve RtlGetVersion dynamically so archived logs retain the raw major/minor/build actually exposed by NT without adding a link dependency. (ChatGPT-5.6-Sol) -->
	HMODULE const hNtdll = GetModuleHandleA("ntdll.dll");
	if (hNtdll != NULL)
	{
		typedef LONG (WINAPI* RtlGetVersionFunction)(OSVERSIONINFOW*);
		RtlGetVersionFunction const pRtlGetVersion = (RtlGetVersionFunction)GetProcAddress(hNtdll, "RtlGetVersion");
		if (pRtlGetVersion != NULL)
		{
			OSVERSIONINFOEXW kVersionInfo;
			ZeroMemory(&kVersionInfo, sizeof(kVersionInfo));
			kVersionInfo.dwOSVersionInfoSize = sizeof(kVersionInfo);
			if (pRtlGetVersion((OSVERSIONINFOW*)&kVersionInfo) == 0)
			{
				kVersion.iMajor = (int)kVersionInfo.dwMajorVersion;
				kVersion.iMinor = (int)kVersionInfo.dwMinorVersion;
				kVersion.iBuild = (int)kVersionInfo.dwBuildNumber;
				kVersion.iServicePackMajor = (int)kVersionInfo.wServicePackMajor;
				kVersion.iServicePackMinor = (int)kVersionInfo.wServicePackMinor;
			}
		}
	}

	HKEY hKey = NULL;
	if (openSASGameRecordWindowsVersionKey(hKey))
	{
		readSASGameRecordRegistryDword(hKey, "UBR", kVersion.iUpdateRevision);
		if (!readSASGameRecordRegistryString(hKey, "DisplayVersion", kVersion.szDisplayVersion))
			readSASGameRecordRegistryString(hKey, "ReleaseId", kVersion.szDisplayVersion);
		SASGameRecordRegistryFunctions const& kRegistry = getSASGameRecordRegistryFunctions();
		if (kRegistry.pClose != NULL)
			kRegistry.pClose(hKey);
	}
	return kVersion;
}

static char const* getSASGameRecordArchitectureName(WORD uiArchitecture)
{
	switch (uiArchitecture)
	{
	case 0: return "X86";
	case 5: return "ARM";
	case 6: return "IA64";
	case 9: return "X64";
	case 12: return "ARM64";
	default: return "OTHER";
	}
}

static void getSASGameRecordNativeSystemInfo(SYSTEM_INFO& kSystemInfo)
{
	typedef VOID (WINAPI* GetNativeSystemInfoFunction)(LPSYSTEM_INFO);
	HMODULE const hKernel32 = GetModuleHandleA("kernel32.dll");
	GetNativeSystemInfoFunction const pGetNativeSystemInfo = (hKernel32 == NULL ? NULL : (GetNativeSystemInfoFunction)GetProcAddress(hKernel32, "GetNativeSystemInfo"));
	if (pGetNativeSystemInfo != NULL)
		pGetNativeSystemInfo(&kSystemInfo);
	else GetSystemInfo(&kSystemInfo);
}

static int getSASGameRecordWow64State()
{
	typedef BOOL (WINAPI* IsWow64ProcessFunction)(HANDLE, PBOOL);
	HMODULE const hKernel32 = GetModuleHandleA("kernel32.dll");
	IsWow64ProcessFunction const pIsWow64Process = (hKernel32 == NULL ? NULL : (IsWow64ProcessFunction)GetProcAddress(hKernel32, "IsWow64Process"));
	if (pIsWow64Process == NULL)
		return -1;
	BOOL bWow64 = FALSE;
	return (pIsWow64Process(GetCurrentProcess(), &bWow64) ? (bWow64 ? 1 : 0) : -1);
}

static char const* classifySASGameRecordDisplayVendor(DISPLAY_DEVICEA const& kDevice)
{
	char const* const szId = kDevice.DeviceID;
	char const* const szName = kDevice.DeviceString;
	if (strstr(szId, "VEN_8086") != NULL || strstr(szName, "Intel") != NULL || strstr(szName, "INTEL") != NULL) return "INTEL";
	if (strstr(szId, "VEN_10DE") != NULL || strstr(szName, "NVIDIA") != NULL) return "NVIDIA";
	if (strstr(szId, "VEN_1002") != NULL || strstr(szName, "AMD") != NULL || strstr(szName, "ATI") != NULL) return "AMD";
	if (strstr(szId, "VEN_1414") != NULL || strstr(szName, "Microsoft") != NULL) return "MICROSOFT";
	if (strstr(szId, "VEN_15AD") != NULL || strstr(szName, "VMware") != NULL) return "VMWARE";
	if (strstr(szId, "VEN_80EE") != NULL || strstr(szName, "VirtualBox") != NULL) return "VIRTUALBOX";
	if (strstr(szId, "VEN_1AF4") != NULL || strstr(szName, "Virtio") != NULL || strstr(szName, "virtio") != NULL) return "VIRTIO";
	return NULL;
}

struct SASGameRecordDisplayAdapterContext
{
	SASGameRecordDisplayAdapterContext() : iAdapters(0), iDesktopAdapters(0), iVendorFamilies(0), szPrimaryVendor("UNKNOWN") {}
	int iAdapters;
	int iDesktopAdapters;
	int iVendorFamilies;
	CvString szPrimaryVendor;
	CvString szVendors;
};

static SASGameRecordDisplayAdapterContext getSASGameRecordDisplayAdapterContext()
{
	SASGameRecordDisplayAdapterContext kContext;
	for (DWORD iDevice = 0; ; iDevice++)
	{
		DISPLAY_DEVICEA kDevice;
		ZeroMemory(&kDevice, sizeof(kDevice));
		kDevice.cb = sizeof(kDevice);
		if (!EnumDisplayDevicesA(NULL, iDevice, &kDevice, 0))
			break;
		// <!-- custom: EnumDisplayDevices(NULL, ...) enumerates display adapters rather than monitor children.
		// Ignore mirror-only pseudo-adapters, but retain inactive adapters that Windows still enumerates so hybrid/multi-GPU topology can remain visible in support records; this is not a physical-GPU inventory.
		// Multiple adapters/vendors can help explain graphics/performance anomalies from driver or device selection (e.g. a less-suitable GPU being used), while this topology remains support context and does not prove which Direct3D adapter Civ4 selected. (GPT-5.6-Sol + ChatGPT-5.6-Sol) -->
		if ((kDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) != 0)
			continue;
		kContext.iAdapters++;
		if ((kDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0)
			kContext.iDesktopAdapters++;
		char const* const szVendor = classifySASGameRecordDisplayVendor(kDevice);
		if ((kDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) != 0)
			kContext.szPrimaryVendor = (szVendor == NULL ? "UNKNOWN" : szVendor);
		if (szVendor == NULL || strstr(kContext.szVendors.GetCString(), szVendor) != NULL)
			continue;
		if (!kContext.szVendors.empty())
			kContext.szVendors += ",";
		kContext.szVendors += szVendor;
		kContext.iVendorFamilies++;
	}
	if (kContext.szVendors.empty())
		kContext.szVendors = "UNKNOWN";
	return kContext;
}

// <!-- custom: Resolve PSAPI only when enabled performance metrics first sample memory, so disabling them also avoids a mandatory runtime dependency. (GPT-5.6-Sol) -->
static bool getSASGameRecordProcessMemory(PROCESS_MEMORY_COUNTERS& kProcessMemory)
{
	typedef BOOL (WINAPI* GetProcessMemoryInfoFunction)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
	static HMODULE const hPsapi = LoadLibraryA("psapi.dll");
	static GetProcessMemoryInfoFunction const pGetProcessMemoryInfo = (hPsapi == NULL ? NULL : (GetProcessMemoryInfoFunction)GetProcAddress(hPsapi, "GetProcessMemoryInfo"));
	return (pGetProcessMemoryInfo != NULL && pGetProcessMemoryInfo(GetCurrentProcess(), &kProcessMemory, sizeof(kProcessMemory)) != FALSE);
}

struct SASGameRecordSystemSnapshot
{
	SASGameRecordSystemSnapshot() : iProcessForeground(-1), iProcessWindowMinimized(-1), iProcessWorkingSetKB(-1), iProcessPeakWorkingSetKB(-1), iProcessPagefileUsageKB(-1), iSystemMemoryLoadPercent(-1), iProcessAvailableVirtualMB(-1) {}
	int iProcessForeground;
	int iProcessWindowMinimized;
	int iProcessWorkingSetKB;
	int iProcessPeakWorkingSetKB;
	int iProcessPagefileUsageKB;
	int iSystemMemoryLoadPercent;
	int iProcessAvailableVirtualMB;
};

static SASGameRecordSystemSnapshot getSASGameRecordSystemSnapshot()
{
	SASGameRecordSystemSnapshot kSnapshot;
	if (!isSASGameRecordPerformanceMetricsEnabled())
		return kSnapshot;
	DWORD uiForegroundProcessId = 0;
	HWND const hForegroundWindow = GetForegroundWindow();
	if (hForegroundWindow != NULL) GetWindowThreadProcessId(hForegroundWindow, &uiForegroundProcessId);
	kSnapshot.iProcessForeground = (uiForegroundProcessId == GetCurrentProcessId() ? 1 : 0);
	kSnapshot.iProcessWindowMinimized = getSASGameRecordProcessWindowMinimized();
	PROCESS_MEMORY_COUNTERS kProcessMemory;
	ZeroMemory(&kProcessMemory, sizeof(kProcessMemory));
	kProcessMemory.cb = sizeof(kProcessMemory);
	if (getSASGameRecordProcessMemory(kProcessMemory))
	{
		kSnapshot.iProcessWorkingSetKB = (int)(kProcessMemory.WorkingSetSize / 1024);
		kSnapshot.iProcessPeakWorkingSetKB = (int)(kProcessMemory.PeakWorkingSetSize / 1024);
		kSnapshot.iProcessPagefileUsageKB = (int)(kProcessMemory.PagefileUsage / 1024);
	}
	MEMORYSTATUSEX kSystemMemory;
	ZeroMemory(&kSystemMemory, sizeof(kSystemMemory));
	kSystemMemory.dwLength = sizeof(kSystemMemory);
	if (GlobalMemoryStatusEx(&kSystemMemory) != FALSE)
	{
		kSnapshot.iSystemMemoryLoadPercent = (int)kSystemMemory.dwMemoryLoad;
		kSnapshot.iProcessAvailableVirtualMB = (int)(kSystemMemory.ullAvailVirtual / (1024 * 1024));
	}
	return kSnapshot;
}

static CvString getSASGameRecordLogTimestamp()
{
	if (g_szSASGameRecordLogTimestamp.empty())
		g_szSASGameRecordLogTimestamp = createSASUtcTimestamp();
	return g_szSASGameRecordLogTimestamp;
}

static bool isSASGameRecordTimestampedFilenameEnabled()
{
	static const bool bUseTimestampedFilename = (GC.getDefineINT("SAS_GAME_RECORD_LOG_USE_TIMESTAMPED_FILENAME") > 0);
	return bUseTimestampedFilename;
}

static CvString getSASGameRecordLogName()
{
	return getSASDiagnosticLogName("SASGameRecord", getSASGameRecordLogTimestamp(), g_szSASGameRecordLogContext, isSASGameRecordTimestampedFilenameEnabled());
}

static void rollSASGameRecordLog(const char* szContext)
{
	// <!-- custom: `seq`, `tx` and immediate plot-owner cause state are intentionally local to one timestamped record session; source/log identity distinguishes different new/load files. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	g_uiSASGameRecordSemanticSequence = 0;
	g_uiSASGameRecordNextTransaction = 0;
	g_uiSASGameRecordActiveTransaction = 0;
	g_szSASGameRecordActiveTransactionKind.clear();
	g_eSASGameRecordPlotOwnerChangeCause = SAS_PLOT_OWNER_CAUSE_NONE;
	time_t kSessionStartTime;
	time(&kSessionStartTime);
	g_uiSASGameRecordSessionStartTime = getSASMonotonicMilliseconds();
	g_uiSASGameRecordPreviousSnapshotTime = g_uiSASGameRecordSessionStartTime;
	g_bSASGameRecordDisplayContextLogged = false;
	// <!-- custom: Delta reconstruction never crosses SASGameRecord files/sessions; the first city-detail observation after every new/load roll must therefore be a full row. (ChatGPT-5.6-Sol) -->
	g_mapSASGameRecordCityDeltaStates.clear();
	g_mapSASGameRecordCityIdentityStates.clear();
	g_szSASGameRecordLogTimestamp = createSASUtcTimestamp(kSessionStartTime);
	if (isSASGameRecordTimestampedFilenameEnabled())
	{
		g_iSASGameRecordLogSequence++;
		g_szSASGameRecordLogContext.Format("%s%d", szContext, g_iSASGameRecordLogSequence);
	}
}

static bool isSASGameRecordStructuredRow(std::string const& szLine)
{
	return (szLine.find("GAME_RECORD_") == 0);
}

static void insertSASGameRecordFieldAfterRowType(std::string& szLine, char const* szField)
{
	if (!isSASGameRecordStructuredRow(szLine))
		return;
	size_t const iTypeEnd = szLine.find(' ');
	szLine.insert(iTypeEnd == std::string::npos ? szLine.length() : iTypeEnd, szField);
}


// <!-- custom: Parse only already-formatted level-3 city-detail rows, preserving each original key=value token byte-for-byte.
// Quoted diagnostic values can contain spaces and escaped quotes/backslashes, so tokenization must honor the shared getSASDiagnosticQuoted escaping rather than split blindly on spaces. (ChatGPT-5.6-Sol) -->
static bool parseSASGameRecordDeltaFields(std::string const& szLine, std::string& szType, std::vector<SASGameRecordCityDeltaField>& aFields)
{
	size_t const iTypeEnd = szLine.find(' ');
	if (iTypeEnd == std::string::npos)
		return false;
	szType = szLine.substr(0, iTypeEnd);
	size_t iPos = iTypeEnd + 1;
	while (iPos < szLine.length())
	{
		while (iPos < szLine.length() && szLine[iPos] == ' ')
			iPos++;
		if (iPos >= szLine.length())
			break;
		size_t const iFieldStart = iPos;
		size_t const iEquals = szLine.find('=', iFieldStart);
		if (iEquals == std::string::npos)
			return false;
		size_t iFieldEnd = iEquals + 1;
		if (iFieldEnd < szLine.length() && szLine[iFieldEnd] == '"')
		{
			bool bEscaped = false;
			iFieldEnd++;
			for (; iFieldEnd < szLine.length(); iFieldEnd++)
			{
				char const c = szLine[iFieldEnd];
				if (bEscaped)
				{
					bEscaped = false;
					continue;
				}
				if (c == '\\')
				{
					bEscaped = true;
					continue;
				}
				if (c == '"')
				{
					iFieldEnd++;
					break;
				}
			}
		}
		else
		{
			size_t const iSpace = szLine.find(' ', iFieldEnd);
			iFieldEnd = (iSpace == std::string::npos ? szLine.length() : iSpace);
		}
		if (iFieldEnd > szLine.length())
			return false;
		SASGameRecordCityDeltaField kField;
		kField.szName = szLine.substr(iFieldStart, iEquals - iFieldStart);
		kField.szToken = szLine.substr(iFieldStart, iFieldEnd - iFieldStart);
		aFields.push_back(kField);
		iPos = iFieldEnd;
	}
	return !aFields.empty();
}

static std::string getSASGameRecordDeltaFieldValue(std::vector<SASGameRecordCityDeltaField> const& aFields, char const* szName)
{
	for (size_t iI = 0; iI < aFields.size(); iI++)
	{
		if (aFields[iI].szName != szName)
			continue;
		size_t const iValue = aFields[iI].szToken.find('=');
		return (iValue == std::string::npos ? std::string() : aFields[iI].szToken.substr(iValue + 1));
	}
	return std::string();
}

static bool isSASGameRecordCityDeltaFamily(std::string const& szType)
{
	// <!-- custom: Trade-partner rows deliberately stay full: measured late-game records have only one payload field, so previous/full-base/change metadata can make a delta row larger than the original.
	// The five families below materially shrink in real level-3 records while retaining periodic full checkpoints. (ChatGPT-5.6-Sol) -->
	return (szType == "GAME_RECORD_CITY" || szType == "GAME_RECORD_CITY_DEVELOPMENT" ||
		szType == "GAME_RECORD_CITY_HAPPINESS" || szType == "GAME_RECORD_CITY_HEALTH" ||
		szType == "GAME_RECORD_CITY_BUILDINGS");
}

static bool hasSASGameRecordCityDeltaSchema(std::vector<SASGameRecordCityDeltaField> const& aCurrent, std::vector<SASGameRecordCityDeltaField> const& aPrevious)
{
	if (aCurrent.size() != aPrevious.size())
		return false;
	for (size_t iI = 0; iI < aCurrent.size(); iI++)
	{
		if (aCurrent[iI].szName != aPrevious[iI].szName)
			return false;
	}
	return true;
}

static void compactSASGameRecordCityDetailRow(std::string& szLine)
{
	std::string szType;
	std::vector<SASGameRecordCityDeltaField> aFields;
	if (!parseSASGameRecordDeltaFields(szLine, szType, aFields) || !isSASGameRecordCityDeltaFamily(szType))
		return;
	std::string const szTurn = getSASGameRecordDeltaFieldValue(aFields, "turn");
	std::string const szPlayer = getSASGameRecordDeltaFieldValue(aFields, "player");
	std::string const szCityId = getSASGameRecordDeltaFieldValue(aFields, "cityId");
	if (szTurn.empty() || szPlayer.empty() || szCityId.empty())
		return;
	int const iTurn = atoi(szTurn.c_str());
	CvString szIdentityKey;
	szIdentityKey.Format("%s:%s", szPlayer.c_str(), szCityId.c_str());

	// <!-- custom: (player, cityId) is the runtime key, but a slot can eventually be reused after city destruction.
	// The core CITY row arrives before its same-snapshot detail companions; track stable lifecycle identity there and advance a session-local generation whenever founding/acquisition/location identity changes, preventing a newly created/reacquired city from inheriting an older city's delta base. (ChatGPT-5.6-Sol) -->
	if (szType == "GAME_RECORD_CITY")
	{
		std::string const szSignature =
			getSASGameRecordDeltaFieldValue(aFields, "x") + "|" +
			getSASGameRecordDeltaFieldValue(aFields, "y") + "|" +
			getSASGameRecordDeltaFieldValue(aFields, "originalOwner") + "|" +
			getSASGameRecordDeltaFieldValue(aFields, "foundedTurn") + "|" +
			getSASGameRecordDeltaFieldValue(aFields, "acquiredTurn");
		std::map<std::string, SASGameRecordCityIdentityState>::iterator itIdentity = g_mapSASGameRecordCityIdentityStates.find(szIdentityKey.GetCString());
		if (itIdentity == g_mapSASGameRecordCityIdentityStates.end())
		{
			SASGameRecordCityIdentityState kIdentity;
			kIdentity.szSignature = szSignature;
			g_mapSASGameRecordCityIdentityStates[szIdentityKey.GetCString()] = kIdentity;
		}
		else if (itIdentity->second.szSignature != szSignature)
		{
			itIdentity->second.szSignature = szSignature;
			itIdentity->second.iGeneration++;
		}
	}
	int iGeneration = 0;
	std::map<std::string, SASGameRecordCityIdentityState>::const_iterator itIdentity = g_mapSASGameRecordCityIdentityStates.find(szIdentityKey.GetCString());
	if (itIdentity != g_mapSASGameRecordCityIdentityStates.end())
		iGeneration = itIdentity->second.iGeneration;

	CvString szStateKey;
	szStateKey.Format("%s|%s|%s|%d", szType.c_str(), szPlayer.c_str(), szCityId.c_str(), iGeneration);
	std::map<std::string, SASGameRecordCityDeltaState>::iterator itState = g_mapSASGameRecordCityDeltaStates.find(szStateKey.GetCString());
	bool const bHavePrevious = (itState != g_mapSASGameRecordCityDeltaStates.end());
	bool const bSchemaMatches = (bHavePrevious && hasSASGameRecordCityDeltaSchema(aFields, itState->second.aFields));
	bool const bPeriodicFull = (bHavePrevious && itState->second.iSnapshotsSinceFull >= SAS_GAME_RECORD_CITY_FULL_SNAPSHOT_CADENCE - 1);
	if (!bHavePrevious || !bSchemaMatches || bPeriodicFull)
	{
		SASGameRecordCityDeltaState& kState = g_mapSASGameRecordCityDeltaStates[szStateKey.GetCString()];
		kState.aFields = aFields;
		kState.iPreviousTurn = iTurn;
		kState.iFullBaseTurn = iTurn;
		kState.iSnapshotsSinceFull = 0;
		return;
	}

	SASGameRecordCityDeltaState& kState = itState->second;
	std::vector<int> aiChanged;
	for (size_t iI = 0; iI < aFields.size(); iI++)
	{
		std::string const& szName = aFields[iI].szName;
		if (szName == "turn" || szName == "player" || szName == "cityId")
			continue;
		if (aFields[iI].szToken != kState.aFields[iI].szToken)
			aiChanged.push_back((int)iI);
	}
	CvString szDeltaPrefix;
	szDeltaPrefix.Format("%s_DELTA turn=%d player=%s cityId=%s previousTurn=%d fullBaseTurn=%d changed=%d",
		szType.c_str(), iTurn, szPlayer.c_str(), szCityId.c_str(), kState.iPreviousTurn, kState.iFullBaseTurn, (int)aiChanged.size());
	szLine = szDeltaPrefix.GetCString();
	for (size_t iI = 0; iI < aiChanged.size(); iI++)
	{
		szLine += " ";
		szLine += aFields[aiChanged[iI]].szToken;
	}
	kState.aFields = aFields;
	kState.iPreviousTurn = iTurn;
	kState.iSnapshotsSinceFull++;
}

static void emitSASGameRecordLine(CvString const& szLogName, std::string szLine)
{
	// <!-- custom: Sequence only machine-readable GAME_RECORD_* rows.
	// Pipe-framed ASCII-map drawing lines remain uninterrupted pictures between their sequenced BEGIN/END metadata rows. (ChatGPT-5.6-Sol) -->
	if (isSASGameRecordStructuredRow(szLine))
	{
		CvString szSequence;
		szSequence.Format(" seq=%I64u", ++g_uiSASGameRecordSemanticSequence);
		insertSASGameRecordFieldAfterRowType(szLine, szSequence.GetCString());
	}
	// <!-- custom: Preserve any literal percent signs that survive producer formatting (including dynamic/free text) before crossing Civ4's printf-style EXE log boundary. See KI#375.3. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	logSASDiagnosticLiteralLine(szLogName.GetCString(), szLine.c_str());
}

static void logSASGameRecordFormattedLine(CvString const& szLogName, TCHAR* format, va_list args)
{
	std::string szLine;
	// <!-- custom: KI#161.2's explicit terminator stopped MSVC 7.1 truncation from leaving unsafe unterminated output, but the fixed 2048-byte buffer still silently discarded long structured rows such as late-game building, unit-type and promotion inventories.
	// Reuse CvString's grow-and-retry formatter so the complete machine-readable row reaches the log; abort the row if even that bounded formatter fails. See KI#375. (ChatGPT-5.5 + GPT-5.5; ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	bool const bFormatted = CvString::formatv(szLine, format, args);
	FAssertMsg(bFormatted, "SASGameRecord row formatting failed");
	if (!bFormatted)
		return;
	// <!-- custom: Delta-compaction runs only after a level-3 city-detail row has already paid its native snapshot computation/formatting cost; lower record levels never enter this parser/cache path.
	// Do this before transaction/sequence decoration so those session-local chronology fields cannot masquerade as city-state changes. (ChatGPT-5.6-Sol) -->
	if (g_iSASGameRecordLogLevel >= 3 && szLine.find("GAME_RECORD_CITY") == 0)
		compactSASGameRecordCityDetailRow(szLine);
	// <!-- custom: Capture causal membership before any buffering.
	// `seq` is deliberately deferred until actual emission, but `tx` must describe the operation active when the observation was produced. (ChatGPT-5.6-Sol) -->
	if (g_uiSASGameRecordActiveTransaction != 0 && isSASGameRecordStructuredRow(szLine))
	{
		CvString szTransaction;
		szTransaction.Format(" tx=%I64u", g_uiSASGameRecordActiveTransaction);
		insertSASGameRecordFieldAfterRowType(szLine, szTransaction.GetCString());
	}
	if (g_bSASGameRecordBufferInitializingActions && szLine.find("GAME_RECORD_ACTION ") == 0)
	{
		// <!-- custom: Successful initialization replaces this procedural transcript with compact finalized state. If initialization aborts, preserve event timing only on the raw fallback actions instead of adding a redundant timestamp to every normal row. (GPT-5.6-Sol) -->
		CvString szSessionWall;
		szSessionWall.Format(" sessionWallMilliseconds=%u", getSASElapsedMilliseconds(g_uiSASGameRecordSessionStartTime, getSASMonotonicMilliseconds()));
		szLine += szSessionWall.GetCString();
		g_aszSASGameRecordInitializingActions.push_back(std::make_pair(szLogName, szLine));
		return;
	}
	emitSASGameRecordLine(szLogName, szLine);
}

static uint getSASGameRecordSessionWallMilliseconds(uint uiNow)
{
	return getSASElapsedMilliseconds(g_uiSASGameRecordSessionStartTime, uiNow);
}

static uint getSASGameRecordSessionWallMilliseconds()
{
	return getSASGameRecordSessionWallMilliseconds(getSASMonotonicMilliseconds());
}

static void appendSASGameRecordType(CvString& szTypes, char const* szType)
{
	if (!szTypes.empty()) szTypes += ",";
	szTypes += szType;
}

static void flushSASGameRecordInitializingActions(bool bContextComplete)
{
	if (g_aszSASGameRecordInitializingActions.empty())
		return;
	logSASGameRecord("GAME_RECORD_INITIALIZATION_ACTIONS count=%d contextComplete=%d sessionWallMilliseconds=%u",
		(int)g_aszSASGameRecordInitializingActions.size(), bContextComplete, getSASGameRecordSessionWallMilliseconds());
	for (size_t iI = 0; iI < g_aszSASGameRecordInitializingActions.size(); iI++)
	{
		std::pair<CvString, std::string> const& kBuffered = g_aszSASGameRecordInitializingActions[iI];
		emitSASGameRecordLine(kBuffered.first, kBuffered.second);
	}
	g_aszSASGameRecordInitializingActions.clear();
}

void logSASGameRecord(TCHAR* format, ... )
{
	static const bool bEnabled = isSASGameRecordLogEnabled();
	if (!bEnabled)
		return;
	// <!-- custom: CITY_BOMBARD is buffered only across consecutive equivalent actions.
	// Flush it before the next ordinary GameRecord row so repeated siege clicks become one synthetic line without losing same-turn ordering relative to battles or other actions. (GPT-5.6 Thinking) -->
	if (!g_bSASGameRecordFlushingCityBombard)
		flushSASGameRecordPendingCityBombard();

	va_list args;
	va_start(args, format);
	logSASGameRecordFormattedLine(getSASGameRecordLogName(), format, args);
	va_end(args);
}

// <!-- custom: The first enabled scope owns a new session-local transaction; nested scopes join it so one synchronous causal chain stays one `tx`.
// BEGIN/END rows make transaction kind and completeness explicit, while every structured row emitted inside the scope receives the same tx field automatically. (ChatGPT-5.6-Sol) -->
void SASGameRecordTransactionScope::begin(char const* szKind)
{
	if (g_uiSASGameRecordActiveTransaction != 0)
		return;
	// <!-- custom: Flush an older synthetic bombard before arming the new transaction.
	// logSASGameRecord itself flushes bombard rows, but doing that after tx activation would falsely attach the previous operation to this scope. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (!g_bSASGameRecordFlushingCityBombard)
		flushSASGameRecordPendingCityBombard();
	g_uiSASGameRecordActiveTransaction = ++g_uiSASGameRecordNextTransaction;
	g_szSASGameRecordActiveTransactionKind = szKind;
	m_bOwnsTransaction = true;
	logSASGameRecord("GAME_RECORD_TRANSACTION_BEGIN turn=%d kind=%s", GC.getGame().getGameTurn(), szKind);
}

void SASGameRecordTransactionScope::end()
{
	FAssert(g_uiSASGameRecordActiveTransaction != 0);
	logSASGameRecord("GAME_RECORD_TRANSACTION_END turn=%d kind=%s", GC.getGame().getGameTurn(), g_szSASGameRecordActiveTransactionKind.GetCString());
	g_uiSASGameRecordActiveTransaction = 0;
	g_szSASGameRecordActiveTransactionKind.clear();
}

// <!-- custom: Nest immediate owner-change causes independently from the root transaction, temporarily overriding the outer mechanism and restoring it after the narrower setter chain returns. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void SASGameRecordPlotOwnerChangeCauseScope::begin(SASGameRecordPlotOwnerChangeCause eCause)
{
	FAssert(eCause != SAS_PLOT_OWNER_CAUSE_NONE);
	m_ePreviousCause = g_eSASGameRecordPlotOwnerChangeCause;
	g_eSASGameRecordPlotOwnerChangeCause = eCause;
	m_bActive = true;
}

void SASGameRecordPlotOwnerChangeCauseScope::end()
{
	FAssert(g_eSASGameRecordPlotOwnerChangeCause != SAS_PLOT_OWNER_CAUSE_NONE);
	g_eSASGameRecordPlotOwnerChangeCause = m_ePreviousCause;
}

void prepareSASGameRecordPlotOwnerChange()
{
	// <!-- custom: Any pending synthetic CITY_BOMBARD happened before the ownership mutation.
	// Flush it before CvPlot changes owner so its delayed row cannot observe half-mutated state or appear after the exact border transition. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (!g_bSASGameRecordFlushingCityBombard)
		flushSASGameRecordPendingCityBombard();
}

// <!-- custom: Free-text escaping is shared with other diagnostic logs in CvGameCoreUtils; keep only this city-specific missing-value wrapper local. (ChatGPT-5.6-Sol) -->
static CvWString getSASGameRecordQuotedCityName(CvCity const* pCity)
{
	return pCity == NULL ? L"-" : getSASDiagnosticQuoted(pCity->getName().GetCString());
}

// <!-- custom: Save-load context is recorded before Civ4 initializes graphics, which produced a misleading 0x0 resolution row.
// Defer the optional display context until graphics are ready, but write the display-disabled placeholder immediately because it needs no graphics queries. (GPT-5.6-Sol) -->
static void logSASGameRecordDisplayContext()
{
	if (g_bSASGameRecordDisplayContextLogged)
		return;
	int const iSystemContextLevel = getSASGameRecordSystemContextLevel();
	if (iSystemContextLevel < 1)
	{
		logSASGameRecord("GAME_RECORD_DISPLAY_CONTEXT systemContextLevel=0 resolution=-1x-1 graphicsInitialized=-1 fullscreen=-1 graphicOptions=- gpuAdapterCount=-1 gpuDesktopAdapterCount=-1 gpuVendorFamilyCount=-1 gpuPrimaryVendor=- gpuVendors=-");
		g_bSASGameRecordDisplayContextLogged = true;
		return;
	}
	if (!GC.IsGraphicsInitialized())
		return;
	CvString szGraphicOptions;
	FOR_EACH_ENUM(GraphicOption)
	{
		if (!gDLL->getGraphicOption(eLoopGraphicOption))
			continue;
		if (!szGraphicOptions.empty())
			szGraphicOptions += ",";
		szGraphicOptions += GC.getInfo(eLoopGraphicOption).getType();
	}
	if (szGraphicOptions.empty())
		szGraphicOptions = "-";
	CvGame const& kGame = GC.getGame();
	SASGameRecordDisplayAdapterContext kDisplayAdapters;
	if (iSystemContextLevel >= 3)
		kDisplayAdapters = getSASGameRecordDisplayAdapterContext();
	logSASGameRecord("GAME_RECORD_DISPLAY_CONTEXT systemContextLevel=%d resolution=%dx%d graphicsInitialized=1 fullscreen=%d graphicOptions=%s gpuAdapterCount=%d gpuDesktopAdapterCount=%d gpuVendorFamilyCount=%d gpuPrimaryVendor=%s gpuVendors=%s",
		iSystemContextLevel, kGame.getScreenWidth(), kGame.getScreenHeight(),
		gDLL->getGraphicOption(GRAPHICOPTION_FULLSCREEN), szGraphicOptions.GetCString(),
		iSystemContextLevel >= 3 ? kDisplayAdapters.iAdapters : -1, iSystemContextLevel >= 3 ? kDisplayAdapters.iDesktopAdapters : -1,
		iSystemContextLevel >= 3 ? kDisplayAdapters.iVendorFamilies : -1,
		iSystemContextLevel >= 3 ? kDisplayAdapters.szPrimaryVendor.GetCString() : "-",
		iSystemContextLevel >= 3 ? kDisplayAdapters.szVendors.GetCString() : "-");
	g_bSASGameRecordDisplayContextLogged = true;
}

// <!-- custom: Record every stored map-script option, including hidden values and current script defaults.
// Keep numeric values durable; readable descriptions can remain unresolved when the script is unavailable. (ChatGPT-5.6-Sol) -->
static void logSASGameRecordMapOptions(CvInitCore const& kInitCore)
{
	const int iNumOptions = kInitCore.getNumCustomMapOptions();
	const int iNumHiddenOptions = std::min(iNumOptions, std::max(0, kInitCore.getNumHiddenCustomMapOptions()));
	CvString const szMapScriptName(kInitCore.getMapScriptName());
	const bool bMapScriptAvailable = (!kInitCore.getWBMapScript() && gDLL->pythonMapExists(szMapScriptName.GetCString()));
	logSASGameRecord("GAME_RECORD_MAP_OPTIONS count=%d hidden=%d scriptAvailable=%d", iNumOptions, iNumHiddenOptions, bMapScriptAvailable);
	if (iNumOptions <= 0)
		return;
	CvPythonCaller const& kPython = *GC.getPythonCaller();
	for (int iOption = 0; iOption < iNumOptions; iOption++)
	{
		CustomMapOptionTypes const eValue = kInitCore.getCustomMapOption(iOption);
		CustomMapOptionTypes eDefault = NO_CUSTOM_MAPOPTION;
		CvWString szDescription;
		CvWString szDefaultDescription;
		if (bMapScriptAvailable)
		{
			eDefault = kPython.customMapOptionDefault(szMapScriptName.GetCString(), iOption);
			szDescription = kPython.customMapOptionDescription(szMapScriptName.GetCString(), iOption, eValue);
			if (eDefault >= 0)
				szDefaultDescription = kPython.customMapOptionDescription(szMapScriptName.GetCString(), iOption, eDefault);
		}
		CvWString const szQuotedDescription = (szDescription.empty() ? CvWString(L"-") : getSASDiagnosticQuoted(szDescription.GetCString()));
		CvWString const szQuotedDefaultDescription = (szDefaultDescription.empty() ? CvWString(L"-") : getSASDiagnosticQuoted(szDefaultDescription.GetCString()));
		const bool bHidden = (iOption >= iNumOptions - iNumHiddenOptions);
		const int iIsScriptDefault = (eDefault < 0 ? -1 : (eValue == eDefault ? 1 : 0));
		logSASGameRecord("GAME_RECORD_MAP_OPTION index=%d hidden=%d value=%d scriptDefault=%d isScriptDefault=%d description=%S scriptDefaultDescription=%S",
				iOption, bHidden, eValue, eDefault, iIsScriptDefault, szQuotedDescription.GetCString(), szQuotedDefaultDescription.GetCString());
	}
}

// <!-- custom: GAMEOPTION_AGGRESSIVE_AI is repurposed while AdvCiv selects UWAI versus legacy K-Mod logic.
// Record the resolved mode, the three defines that can change that interpretation, and SAS gameplay toggles that materially change UWAI behavior under the same source/settings context.
// Detailed SASGameRecord war-plan intent deliberately follows AdvCiv-SAS's supported foreground-UWAI gameplay; inherited K-Mod/Legacy modes stay identifiable here and through factual WAR_PLAN_CHANGED history, but are not promised equivalent causal provenance. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
static void logSASGameRecordWarAISettings(CvGame const& kGame)
{
	const bool bUWAI = getUWAI().isEnabled();
	const bool bUWAIBackground = getUWAI().isEnabled(true);
	const char* szUWAIMode = (bUWAI ? "FULL" : (bUWAIBackground ? "BACKGROUND" : "DISABLED"));

	// <!-- custom: These are cold setup/load reads; caching would add static state for negligible benefit. (ChatGPT-5.6-Sol) -->
	const int iUseKModAINonAggressive = GC.getDefineINT("USE_KMOD_AI_NONAGGRESSIVE");
	const int iDisableUWAI = GC.getDefineINT("DISABLE_UWAI");
	const int iUWAIInBackground = GC.getDefineINT("UWAI_IN_BACKGROUND");
	const int iNavalLogisticsDeploymentOptimize = GC.getDefineBOOL("SAS_UWAI_INVASION_GRAPH_NAVAL_LOGISTICS_DEPLOYMENT_OPTIMIZE");
	logSASGameRecord("GAME_RECORD_WAR_AI_SETTINGS warPeaceAI=%s uwaiMode=%s engineAggressiveAI=%d USE_KMOD_AI_NONAGGRESSIVE=%d DISABLE_UWAI=%d UWAI_IN_BACKGROUND=%d SAS_UWAI_INVASION_GRAPH_NAVAL_LOGISTICS_DEPLOYMENT_OPTIMIZE=%d",
			bUWAI ? "UWAI" : "KMOD_LEGACY", szUWAIMode, kGame.isOption(GAMEOPTION_AGGRESSIVE_AI),
			iUseKModAINonAggressive, iDisableUWAI, iUWAIInBackground, iNavalLogisticsDeploymentOptimize);
}

// <!-- custom: Use "row" wording for generic SASGameRecord row prefixes because Civ4 also has EventInfo/random events.
// Keep GAME_RECORD_ACTION only for chronological gameplay action rows. (GPT-5.5) -->
static void logSASGameRecordGameState(const char* szRowType)
{
	CvGame& kGame = GC.getGame();
	CvInitCore const& kInitCore = GC.getInitCore();
	const PlayerTypes eActivePlayer = kGame.getActivePlayer();
	const char* szActiveCivilization = "-";
	const char* szActiveHandicap = "-";
	if (eActivePlayer != NO_PLAYER)
	{
		CvPlayer const& kActivePlayer = GET_PLAYER(eActivePlayer);
		if (kActivePlayer.getCivilizationType() != NO_CIVILIZATION)
			szActiveCivilization = GC.getInfo(kActivePlayer.getCivilizationType()).getType();
		if (kActivePlayer.getHandicapType() != NO_HANDICAP)
			szActiveHandicap = GC.getInfo(kActivePlayer.getHandicapType()).getType();
	}
	CvString szGameOptions;
	FOR_EACH_ENUM(GameOption)
	{
		if (!kGame.isOption(eLoopGameOption))
			continue;
		if (!szGameOptions.empty())
			szGameOptions += ",";
		szGameOptions += GC.getInfo(eLoopGameOption).getType();
	}
	if (szGameOptions.empty())
		szGameOptions = "-";
	CvString szVictories;
	FOR_EACH_ENUM(Victory)
	{
		if (!kGame.isVictoryValid(eLoopVictory))
			continue;
		if (!szVictories.empty())
			szVictories += ",";
		szVictories += GC.getInfo(eLoopVictory).getType();
	}
	if (szVictories.empty())
		szVictories = "-";
	CvString szMPOptions;
	FOR_EACH_ENUM(MPOption)
	{
		if (!kGame.isMPOption(eLoopMPOption))
			continue;
		if (!szMPOptions.empty())
			szMPOptions += ",";
		szMPOptions += GC.getInfo(eLoopMPOption).getType();
	}
	if (szMPOptions.empty())
		szMPOptions = "-";
	const CvString szLogName = getSASGameRecordLogName();
	// <!-- custom: This session boundary is a useful timing anchor: on a new game it measures initialization, and on load it measures recorder setup before the finalized save context is available. (GPT-5.6-Sol) -->
	logSASGameRecord("%s processUtc=%s utc=%s logFile=%s turn=%d elapsed=%d year=%d scenario=%d activePlayer=%d activeCivilization=%s activeHandicap=%s playersDefined=%d playersAlive=%d playersEverAlive=%d humans=%d sessionWallMilliseconds=%u",
		szRowType, getSASProcessUtcTimestamp().GetCString(), getSASGameRecordLogTimestamp().GetCString(),
		getSASDiagnosticQuoted(szLogName.GetCString()).GetCString(), kGame.getGameTurn(), kGame.getElapsedGameTurns(),
		kGame.getGameTurnYear(), kGame.isScenario(), eActivePlayer, szActiveCivilization, szActiveHandicap, kInitCore.getNumDefinedPlayers(),
		kGame.countCivPlayersAlive(), kGame.countCivPlayersEverAlive(), kGame.getNumHumanPlayers(),
		getSASGameRecordSessionWallMilliseconds());
	// <!-- custom: Record the engine's durable launch/load and multiplayer identities so standalone logs distinguish ordinary games, scenarios, saves, replays, and supported network/turn modes.
	// The explicit booleans avoid requiring consumers to reproduce Civ4's non-obvious GameType groupings.
	// Simple Game versus Custom Game is not retained reliably after launch, so do not infer it from mutable player slots or options. (GPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_SESSION_CONTEXT gameType=%s gameMode=%s newGame=%d savedGame=%d scenario=%d gameMultiplayer=%d networkMultiplayer=%d hotseat=%d pbem=%d pitboss=%d simultaneousTeamTurns=%d mpOptions=%s",
		getSASGameType(kInitCore.getType()), getSASGameMode(kInitCore.getMode()), kInitCore.getNewGame(), kInitCore.getSavedGame(),
		kGame.isScenario(), kInitCore.getGameMultiplayer(), kGame.isNetworkMultiPlayer(), kGame.isHotSeat(), kGame.isPbem(),
		kGame.isPitboss(), kGame.isSimultaneousTeamTurns(), szMPOptions.GetCString());
	// <!-- custom: Enabled victories and their fixed turn/score limits determine which later victory-progress and AI-strategy rows are relevant.
	// Record this compact setup context instead of requiring external XML or save inspection. (GPT-5.6-Sol) -->
	// <!-- custom: Calendar, starting turn and starting year complete the time scale for scenarios and nonstandard calendars; ReplayInfo preserves the same context. (GPT-5.6-Sol) -->
	// <!-- custom: Record configured Advanced Start points separately from the boolean option, plus the current XML default for comparison.
	// Use -1 when ordinary Advanced Start is inactive or SPaH repurposes the setup value. (ChatGPT-5.6-Sol) -->
	const bool bAdvancedStart = kGame.isOption(GAMEOPTION_ADVANCED_START);
	const bool bStartPointsAsHandicap = kGame.isOption(GAMEOPTION_SPAH);
	const int iAdvancedStartConfiguredPoints = (bAdvancedStart && !bStartPointsAsHandicap ? kGame.getNumAdvancedStartPoints() : -1);
	int iAdvancedStartXmlDefaultPoints = -1;
	if (bAdvancedStart && !bStartPointsAsHandicap && kGame.getStartEra() != NO_ERA && kInitCore.getWorldSize() != NO_WORLDSIZE)
	{
		iAdvancedStartXmlDefaultPoints = GC.getInfo(kGame.getStartEra()).getAdvancedStartPoints();
		iAdvancedStartXmlDefaultPoints *= GC.getInfo(kInitCore.getWorldSize()).getAdvancedStartPointsMod();
		iAdvancedStartXmlDefaultPoints /= 100;
	}
	logSASGameRecord("GAME_RECORD_GAME_SETTINGS mapScript=%S map=%dx%d landHeavy=%d navalHeavy=%d world=%s climate=%s seaLevel=%s gameSpeed=%s startEra=%s calendar=%s startTurn=%d startYear=%d gameHandicap=%s maxTurns=%d targetScore=%d advancedStartConfiguredPoints=%d advancedStartXmlDefaultPoints=%d victories=%s options=%s",
		getSASDiagnosticQuoted(kInitCore.getMapScriptName().GetCString()).GetCString(), GC.getMap().getGridWidth(),
		GC.getMap().getGridHeight(), kGame.isLandHeavyMapnameCached(), kGame.isNavalHeavyMapnameCached(),
		GC.getInfo(kInitCore.getWorldSize()).getType(), GC.getInfo(kInitCore.getClimate()).getType(),
		GC.getInfo(kInitCore.getSeaLevel()).getType(), GC.getInfo(kGame.getGameSpeedType()).getType(),
		GC.getInfo(kGame.getStartEra()).getType(), getSASCalendarType(kGame.getCalendar()), kGame.getStartTurn(), kGame.getStartYear(),
		GC.getInfo(kGame.getHandicapType()).getType(), kGame.getMaxTurns(), kGame.getTargetScore(), iAdvancedStartConfiguredPoints,
		iAdvancedStartXmlDefaultPoints, szVictories.GetCString(), szGameOptions.GetCString());
	logSASGameRecordMapOptions(kInitCore);
	logSASGameRecordWarAISettings(kGame);
	// <!-- custom: Display settings can affect measured autoplay wall time.
	// Record the compact Civ4 context once, after graphics initialization, rather than copying unrelated CivilizationIV.ini settings. (GPT-5.6-Sol) -->
	logSASGameRecordDisplayContext();
	// <!-- custom: A Civ4 custom DLL is a Win32 binary even when Wine/Proton runs it on another host OS.
	// At the default privacy tier, retain raw Windows build/update identity and native architecture because OS/runtime changes can explain compatibility regressions while revealing far less than CPU/GPU models or absolute paths.
	// Level 3 keeps the more identifying coarse CPU/GPU vendor families plus processor-count/RAM capacity fields. Wine version is compatibility-layer identity only; do not guess its host OS. (GPT-5.6-Sol + ChatGPT-5.6-Sol) -->
	int const iSystemContextLevel = getSASGameRecordSystemContextLevel();
	if (iSystemContextLevel >= 2)
	{
		SASGameRecordWindowsVersion const& kWindowsVersion = getSASGameRecordWindowsVersion();
		SYSTEM_INFO kProcessSystemInfo;
		SYSTEM_INFO kNativeSystemInfo;
		ZeroMemory(&kProcessSystemInfo, sizeof(kProcessSystemInfo));
		ZeroMemory(&kNativeSystemInfo, sizeof(kNativeSystemInfo));
		GetSystemInfo(&kProcessSystemInfo);
		getSASGameRecordNativeSystemInfo(kNativeSystemInfo);
		CvString const szWineVersion = getSASGameRecordWineVersion();
		CvString const szWineVersionField = (szWineVersion == "-" ? CvString("-") : getSASDiagnosticQuoted(szWineVersion.GetCString()));
		CvString const szWindowsDisplayVersionField = (kWindowsVersion.szDisplayVersion == "-" ? CvString("-") : getSASDiagnosticQuoted(kWindowsVersion.szDisplayVersion.GetCString()));
		int iLogicalProcessors = -1;
		int iTotalPhysicalMemoryMB = -1;
		char const* szCpuVendor = "-";
		if (iSystemContextLevel >= 3)
		{
			szCpuVendor = getSASGameRecordProcessorVendor();
			iLogicalProcessors = (int)kNativeSystemInfo.dwNumberOfProcessors;
			MEMORYSTATUSEX kSystemMemory;
			ZeroMemory(&kSystemMemory, sizeof(kSystemMemory));
			kSystemMemory.dwLength = sizeof(kSystemMemory);
			if (GlobalMemoryStatusEx(&kSystemMemory)) iTotalPhysicalMemoryMB = (int)(kSystemMemory.ullTotalPhys / (1024 * 1024));
		}
		logSASGameRecord("GAME_RECORD_RUNTIME_CONTEXT systemContextLevel=%d win32Runtime=%s wineVersion=%s windowsMajor=%d windowsMinor=%d windowsBuild=%d windowsUpdateRevision=%d windowsServicePackMajor=%d windowsServicePackMinor=%d windowsDisplayVersion=%s pointerBits=%d processArch=%s nativeArch=%s wow64=%d cpuVendor=%s logicalProcessors=%d totalPhysicalMemoryMB=%d",
			iSystemContextLevel, getSASGameRecordWin32Runtime(), szWineVersionField.GetCString(), kWindowsVersion.iMajor, kWindowsVersion.iMinor, kWindowsVersion.iBuild,
			kWindowsVersion.iUpdateRevision, kWindowsVersion.iServicePackMajor, kWindowsVersion.iServicePackMinor, szWindowsDisplayVersionField.GetCString(),
			(int)(8 * sizeof(void*)), getSASGameRecordArchitectureName(kProcessSystemInfo.wProcessorArchitecture),
			getSASGameRecordArchitectureName(kNativeSystemInfo.wProcessorArchitecture), getSASGameRecordWow64State(), szCpuVendor, iLogicalProcessors, iTotalPhysicalMemoryMB);
	}
	else logSASGameRecord("GAME_RECORD_RUNTIME_CONTEXT systemContextLevel=%d win32Runtime=- wineVersion=- windowsMajor=-1 windowsMinor=-1 windowsBuild=-1 windowsUpdateRevision=-1 windowsServicePackMajor=-1 windowsServicePackMinor=-1 windowsDisplayVersion=- pointerBits=-1 processArch=- nativeArch=- wow64=-1 cpuVendor=- logicalProcessors=-1 totalPhysicalMemoryMB=-1",
		iSystemContextLevel);
	// <!-- custom: Keep the game's persisted initial seeds beside the current post-initialization/load RNG states.
	// Level-3 checkpoints add session-local consumption counts/fingerprints; this compact baseline remains useful at every enabled level. (GPT-5.6-Sol) -->
	std::pair<uint,uint> const kInitialRandSeed = kGame.getInitialRandSeed();
	logSASGameRecord("GAME_RECORD_GAME_RNG mapRandState=%u syncRandState=%u initialMapRandSeed=%u initialSyncRandSeed=%u",
		kGame.getMapRand().getSeed(), kGame.getSorenRand().getSeed(), kInitialRandSeed.first, kInitialRandSeed.second);
}

static void logSASGameRecordLogSettings()
{
	// <!-- custom: Read the popup define directly because this settings row is emitted only once per log; unlike repeatedly queried map settings, caching it would add state without avoiding repeat work. (GPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_LOG_SETTINGS SAS_GAME_RECORD_LOG_LEVEL=%d SAS_GAME_RECORD_INTERVAL_TURNS_UNSCALED_GAMESPEED=%d SAS_GAME_RECORD_LOG_USE_TIMESTAMPED_FILENAME=%d SAS_AIAUTOPLAY_AUTO_DISMISS_INFORMATIONAL_POPUPS_ENABLE=%d SAS_GAME_RECORD_MAP_ASCII_MAX_WIDTH=%d SAS_GAME_RECORD_MAP_ASCII_MAX_HEIGHT=%d SAS_GAME_RECORD_MAP_ASCII_HORIZONTAL_CHARS_PER_CELL=%d SAS_GAME_RECORD_MAP_ASCII_OVERVIEW_1_SCALE_PERCENT=%d SAS_GAME_RECORD_MAP_ASCII_OVERVIEW_2_SCALE_PERCENT=%d SAS_GAME_RECORD_MAP_ASCII_NATIVE_GEOGRAPHY_ENABLE=%d SAS_GAME_RECORD_MAP_ASCII_GEOGRAPHY_ENABLE=%d SAS_GAME_RECORD_MAP_ASCII_TERRAIN_ENABLE=%d SAS_GAME_RECORD_MAP_ASCII_RIVER_ENABLE=%d SAS_GAME_RECORD_MAP_ASCII_BONUS_ENABLE=%d SAS_GAME_RECORD_MAP_ASCII_FEATURE_ENABLE=%d SAS_GAME_RECORD_MAP_ASCII_POLITICAL_ENABLE=%d SAS_GAME_RECORD_TRADE_MARKET_ENABLE=%d SAS_GAME_RECORD_TRADE_MARKET_BONUS_GPT_QUOTES_ENABLE=%d SAS_GAME_RECORD_TRADE_MARKET_AI_TECH_VALUES_ENABLE=%d SAS_GAME_RECORD_PERFORMANCE_METRICS_ENABLE=%d SAS_GAME_RECORD_SYSTEM_CONTEXT_LEVEL=%d",
		getSASGameRecordLogLevel(), getSASGameRecordTurnInterval(), isSASGameRecordTimestampedFilenameEnabled(),
		GC.getDefineINT("SAS_AIAUTOPLAY_AUTO_DISMISS_INFORMATIONAL_POPUPS_ENABLE"), getSASGameRecordMapAsciiMaxWidth(),
		getSASGameRecordMapAsciiMaxHeight(), getSASGameRecordMapAsciiHorizontalCharsPerCell(),
		getSASGameRecordMapAsciiOverview1ScalePercent(), getSASGameRecordMapAsciiOverview2ScalePercent(),
		isSASGameRecordMapAsciiNativeGeographyEnabled(), isSASGameRecordMapAsciiGeographyEnabled(), isSASGameRecordMapAsciiTerrainEnabled(),
		isSASGameRecordMapAsciiRiversEnabled(), isSASGameRecordMapAsciiBonusesEnabled(), isSASGameRecordMapAsciiFeaturesEnabled(),
		isSASGameRecordMapAsciiPoliticalEnabled(), isSASGameRecordTradeMarketEnabled(), isSASGameRecordTradeMarketBonusGPTQuotesEnabled(),
		isSASGameRecordTradeMarketAITechValuesEnabled(), isSASGameRecordPerformanceMetricsEnabled(), getSASGameRecordSystemContextLevel());
}

// <!-- custom: Compact finalized team rows preserve which technologies each team owns and which diplomacy capabilities are active, but replacing setup-time TECH_ACQUIRED spam otherwise loses which technology grants each capability.
// Record the loaded XML mapping once for the whole session instead of repeating the same effect fields for every initial team-tech pair. (GPT-5.6-Sol) -->
static void logSASGameRecordTechCapabilitySources()
{
	CvString szMapTrading, szTechTrading, szGoldTrading, szOpenBordersTrading, szDefensivePactTrading, szPermanentAllianceTrading, szVassalStateTrading;
	FOR_EACH_ENUM(Tech)
	{
		CvTechInfo const& kTech = GC.getInfo(eLoopTech);
		if (kTech.isMapTrading()) appendSASGameRecordType(szMapTrading, kTech.getType());
		if (kTech.isTechTrading()) appendSASGameRecordType(szTechTrading, kTech.getType());
		if (kTech.isGoldTrading()) appendSASGameRecordType(szGoldTrading, kTech.getType());
		if (kTech.isOpenBordersTrading()) appendSASGameRecordType(szOpenBordersTrading, kTech.getType());
		if (kTech.isDefensivePactTrading()) appendSASGameRecordType(szDefensivePactTrading, kTech.getType());
		if (kTech.isPermanentAllianceTrading()) appendSASGameRecordType(szPermanentAllianceTrading, kTech.getType());
		if (kTech.isVassalStateTrading()) appendSASGameRecordType(szVassalStateTrading, kTech.getType());
	}
	logSASGameRecord("GAME_RECORD_TECH_CAPABILITY_SOURCES mapTrading=%s techTrading=%s goldTrading=%s openBordersTrading=%s defensivePactTrading=%s permanentAllianceTrading=%s vassalStateTrading=%s source=LOADED_XML",
		getSASDiagnosticOrDash(szMapTrading).GetCString(), getSASDiagnosticOrDash(szTechTrading).GetCString(),
		getSASDiagnosticOrDash(szGoldTrading).GetCString(), getSASDiagnosticOrDash(szOpenBordersTrading).GetCString(),
		getSASDiagnosticOrDash(szDefensivePactTrading).GetCString(), getSASDiagnosticOrDash(szPermanentAllianceTrading).GetCString(),
		getSASDiagnosticOrDash(szVassalStateTrading).GetCString());
}

// <!-- custom: Vassal freedom thresholds are loaded-rule context, not changing relationship state; emit them once per GameRecord session instead of repeating the same values on every periodic vassal row.
// This cold helper runs only when a new-game or loaded-save GameRecord file opens, so direct define lookups intentionally avoid persistent cache state for values that are read just once per session. (ChatGPT-5.6-Sol) -->
static void logSASGameRecordVassalRules()
{
	logSASGameRecord("GAME_RECORD_VASSAL_RULES freeVassalLandPercent=%d freeVassalPopulationPercent=%d ownLossesFactor=%d masterLossesFactor=%d vassalLandFloor=10 relativeSizeMasterLand=OWN_ONLY masterLossMasterLand=INCLUDING_VASSAL_CONTRIBUTION",
		GC.getDefineINT(CvGlobals::FREE_VASSAL_LAND_PERCENT), GC.getDefineINT(CvGlobals::FREE_VASSAL_POPULATION_PERCENT),
		GC.getDefineINT("VASSAL_REVOLT_OWN_LOSSES_FACTOR"), GC.getDefineINT("VASSAL_REVOLT_MASTER_LOSSES_FACTOR"));
}

static void resetSASGameRecordState();
static void logSASGameRecordInitialContext(bool bNewGame);
static void logSASGameRecordInitialPlayerIdentities();
static void logSASGameRecordFinalizedInitialState(int& iTeamStateRows, int& iTechRows, int& iDeals);
static void initializeSASGameRecordWarsFromLoadedSave();

// <!-- custom: Keep shared provenance behind the recorder's runtime gate so source resolution/DLL hashing never happens merely because an otherwise-disabled lifecycle hook was reached. (ChatGPT-5.6-Sol) -->
static void logSASGameRecordProvenanceContext()
{
	if (!isSASGameRecordLogEnabled())
		return;
	logSASGameRecord("GAME_RECORD_MOD_CONTEXT %s", getSASModContextFields().GetCString());
	// <!-- custom: Include the official recorder revision in SOURCE_CONTEXT: recordRevision tells copied implementations that SASGameRecord itself changed, while the remaining fields identify the exact running tree. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_SOURCE_CONTEXT recordRevision=%d %s", SAS_GAME_RECORD_REVISION, getSASSourceContextFields().GetCString());
	logSASGameRecord("GAME_RECORD_DLL_CONTEXT %s", getSASDllContextFields().GetCString());
	// <!-- custom: Fingerprint the actual Civ4 executable separately from install-location labeling: exact bytes remain the durable build/branch identity even when storefront branches share one directory. (ChatGPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_EXE_CONTEXT %s", getSASExeContextFields().GetCString());
	// <!-- custom: Keep install provenance privacy-tiered.
	// Level 2 records only a coarse storefront/distribution hint and numeric Steam AppID when available; level 3 explicitly opts into exact EXE/mod paths, which can contain a Windows username or custom folder names. (ChatGPT-5.6-Sol) -->
	int const iSystemContextLevel = getSASGameRecordSystemContextLevel();
	logSASGameRecord("GAME_RECORD_INSTALL_CONTEXT systemContextLevel=%d %s", iSystemContextLevel, getSASInstallContextFields(iSystemContextLevel).GetCString());
}

// <!-- custom: Persisted game-source history is separate from the current SOURCE_CONTEXT: it records where this save lineage began and each later runtime-source transition without storing noisy dirty-file lists in the save itself. (ChatGPT-5.6-Sol) -->
static void logSASGameRecordVersionHistory()
{
	CvGame const& kGame = GC.getGame();
	int const iEntries = kGame.getNumSASVersionHistoryEntries();
	logSASGameRecord("GAME_RECORD_SAVE_VERSION_HISTORY entries=%d", iEntries);
	for (int i = 0; i < iEntries; i++)
	{
		char const* szRole = (i == 0 ? "CREATION" : "TRANSITION");
		logSASGameRecord("GAME_RECORD_SAVE_VERSION_HISTORY_ENTRY index=%d role=%s turn=%d version=%s commit=%s dirty=%d",
			i, szRole, kGame.getSASVersionHistoryTurn(i), getSASDiagnosticQuoted(kGame.getSASVersionHistoryVersion(i)).GetCString(),
			getSASDiagnosticQuoted(kGame.getSASVersionHistoryCommitHash(i)).GetCString(), kGame.getSASVersionHistoryDirtyState(i));
	}
}

void startSASGameRecordLogForNewGame()
{
	rollSASGameRecordLog("new");
	resetSASGameRecordState();
	logSASGameRecord("GAME_RECORD_NEW_GAME_INITIALIZING processUtc=%s utc=%s logFile=%s sessionWallMilliseconds=0",
		getSASProcessUtcTimestamp().GetCString(), getSASGameRecordLogTimestamp().GetCString(),
		getSASDiagnosticQuoted(getSASGameRecordLogName().GetCString()).GetCString());
	// <!-- custom: Static mod/source/binary provenance is already final once this DLL is running, so keep it immediately after the lifecycle marker instead of burying it behind generated game context. (ChatGPT-5.6-Sol) -->
	logSASGameRecordProvenanceContext();
	logSASGameRecordLogSettings();
	logSASGameRecordTechCapabilitySources();
	logSASGameRecordVassalRules();
	// <!-- custom: Settings below are not final until map/player initialization finishes.
	// Buffer setup-generated actions only as a failure diagnostic: successful gameStart replaces the procedural transcript with authoritative finalized initial state, while an aborted initialization flushes the raw actions with contextComplete=0. (ChatGPT-5.6-Sol) -->
	g_bSASGameRecordBufferInitializingActions = true;
}

void logSASGameRecordNewGameStarted()
{
	logSASGameRecordGameState("GAME_RECORD_NEW_GAME_STARTED");
	// <!-- custom: gameStart has now established the persisted creation entry, so expose it immediately in the successful new-game record. (ChatGPT-5.6-Sol) -->
	logSASGameRecordVersionHistory();
	logSASGameRecordInitialPlayerIdentities();
	int iTeamStateRows = 0;
	int iTechRows = 0;
	int iDeals = 0;
	// <!-- custom: Finalized team/tech/deal expansion is level-2-only. Own that gate here so the helper never starts or validates work that its caller already knows is disabled. (ChatGPT-5.6-Sol) -->
	bool const bLogInitialDetails = (gGameRecordLogLevel >= 2);
	if (bLogInitialDetails) logSASGameRecordFinalizedInitialState(iTeamStateRows, iTechRows, iDeals);
	logSASGameRecordInitialContext(true);
	// <!-- custom: A successful initialization is represented by authoritative finalized state above, not by replaying the engine's procedural setup transcript.
	// Keep only the count as an audit clue; failed initialization still flushes every raw buffered action from finalizeSASGameRecordLogSession. (ChatGPT-5.6-Sol) -->
	int const iBufferedActionsNotReplayed = (int)g_aszSASGameRecordInitializingActions.size();
	g_bSASGameRecordBufferInitializingActions = false;
	g_aszSASGameRecordInitializingActions.clear();
	if (bLogInitialDetails || iBufferedActionsNotReplayed > 0)
		logSASGameRecord("GAME_RECORD_INITIAL_STATE_SUMMARY teamStateRows=%d techGroupRows=%d techTeamsCovered=%d %s bufferedActionsNotReplayed=%d source=FINALIZED_STATE sessionWallMilliseconds=%u",
			iTeamStateRows, iTechRows, iTeamStateRows, getSASInitialDealSummaryFields(bLogInitialDetails, iDeals).GetCString(),
			iBufferedActionsNotReplayed, getSASGameRecordSessionWallMilliseconds());
}

void startSASGameRecordLogForLoadedSave()
{
	rollSASGameRecordLog("load");
	resetSASGameRecordState();
	// <!-- custom: Loaded RNG state already exists when onAllGameDataRead starts this new recorder session, so use it directly as the level-3 baseline. Session counters intentionally restart at each timestamped load log.
	// GAMEOPTION_NEW_RANDOM_SEED is likewise applied during deserialization while old-session tracking is already finalized; its resulting seed is intentionally the new session baseline rather than a cross-session SEED_SET operation. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (gGameRecordLogLevel >= 3) initializeSASGameRecordRngTracking();
	// <!-- custom: The save contains no recorder-local war history. Begin partial observations for wars already in progress, with the loaded turn and current war success recorded explicitly as their observable baseline. (GPT-5.6-Sol) -->
	if (gGameRecordLogLevel >= 2) initializeSASGameRecordWarsFromLoadedSave();
	logSASGameRecordGameState("GAME_RECORD_SAVE_LOADED");
	// <!-- custom: Keep static mod/source/binary identity immediately after the load-session marker; it does not depend on the loaded save's generated/game state. (ChatGPT-5.6-Sol) -->
	logSASGameRecordProvenanceContext();
	// <!-- custom: onAllGameDataRead reconciles the persisted lineage before starting this log, so a load records any newly encountered source transition here. (ChatGPT-5.6-Sol) -->
	logSASGameRecordVersionHistory();
	logSASGameRecordLogSettings();
	logSASGameRecordTechCapabilitySources();
	logSASGameRecordVassalRules();
	logSASGameRecordInitialPlayerIdentities();
	logSASGameRecordInitialContext(false);
}

// <!-- custom: Game-record helpers keep output compact, stable, and machine-readable.
// They intentionally use XML type names instead of localized text where possible, so external tools can diff and parse autoplay runs reliably.
// The static state below is tiny and is only reset/updated through game-record call sites when the XML log level enables this feature; dynamic XML logging cannot be compiled out cleanly without losing normal runtime XML tuning. (ChatGPT-5.5) -->
static int g_aiSASGameRecordBattleWins[MAX_PLAYERS];
static int g_aiSASGameRecordBattleLosses[MAX_PLAYERS];
static int g_aiSASGameRecordCityBattleWins[MAX_PLAYERS];
static int g_aiSASGameRecordCityBattleLosses[MAX_PLAYERS];
static int g_aiSASGameRecordTotalBattleWins[MAX_PLAYERS];
static int g_aiSASGameRecordTotalBattleLosses[MAX_PLAYERS];
static int g_aiSASGameRecordTotalCityBattleWins[MAX_PLAYERS];
static int g_aiSASGameRecordTotalCityBattleLosses[MAX_PLAYERS];

// <!-- custom: Ordinary win/loss counts do not describe withdrawals, combat-limit attacks or how surprising binary outcomes were.
// Keep compact interval and recorder-session aggregates using Civ4's exact pre-combat odds; no individual level-2 battle rows are added. (ChatGPT-5.6-Sol) -->
struct SASGameRecordBattleQuality
{
	int iWithdrawals;
	int iEnemyWithdrawals;
	int iCombatLimitAttacks;
	int iCombatLimitDefenses;
	int iLuckEligibleBattles;
	int iLuckEligibleWins;
	int iExpectedWinsX1000;
	int iUpsetWins;
	int iUpsetLosses;
	int iLowestOddsWinPermille;
	int iHighestOddsLossPermille;
	void reset()
	{
		iWithdrawals = 0;
		iEnemyWithdrawals = 0;
		iCombatLimitAttacks = 0;
		iCombatLimitDefenses = 0;
		iLuckEligibleBattles = 0;
		iLuckEligibleWins = 0;
		iExpectedWinsX1000 = 0;
		iUpsetWins = 0;
		iUpsetLosses = 0;
		iLowestOddsWinPermille = -1;
		iHighestOddsLossPermille = -1;
	}
	bool hasAny() const
	{
		return (iWithdrawals > 0 || iEnemyWithdrawals > 0 || iCombatLimitAttacks > 0 || iCombatLimitDefenses > 0 || iLuckEligibleBattles > 0);
	}
};
static SASGameRecordBattleQuality g_akSASGameRecordBattleQuality[MAX_PLAYERS];
static SASGameRecordBattleQuality g_akSASGameRecordTotalBattleQuality[MAX_PLAYERS];

// <!-- custom: Session totals complement current-unit XP snapshots: veteran deaths, upgrades and captures no longer erase evidence of XP generated, promotion decisions made or veteran quality exchanged in combat.
// Promotion-type detail stays interval-only to avoid repeating a growing lifetime list. (ChatGPT-5.6-Sol) -->
struct SASGameRecordMilitaryQualityTotals
{
	int iExperienceGained;
	int iCombatExperienceGained;
	int iNonCombatExperienceGained;
	int iExperiencePreventedByCap;
	int iExperienceLostAdjustments;
	int iPromotionsChosen;
	int iLeaderPromotionApplications;
	int iEnemyExperienceDestroyed;
	int iOwnExperienceLost;
	void reset()
	{
		iExperienceGained = 0;
		iCombatExperienceGained = 0;
		iNonCombatExperienceGained = 0;
		iExperiencePreventedByCap = 0;
		iExperienceLostAdjustments = 0;
		iPromotionsChosen = 0;
		iLeaderPromotionApplications = 0;
		iEnemyExperienceDestroyed = 0;
		iOwnExperienceLost = 0;
	}
};
static SASGameRecordMilitaryQualityTotals g_akSASGameRecordMilitaryQualityTotals[MAX_PLAYERS];

// <!-- custom: Visible combat can delay combatResult for several frames after resolveCombat has already determined the fight.
// Keep only transient recorder context needed to join that later callback to the true attacker and pre-combat odds; this is never serialized into the save. (ChatGPT-5.6-Sol) -->
struct SASGameRecordCombatPending
{
	PlayerTypes eAttacker;
	PlayerTypes eDefender;
	int iAttackerUnitId;
	int iDefenderUnitId;
	int iX;
	int iY;
	int iAttackerFromX;
	int iAttackerFromY;
	int iRiverCrossingAttack;
	int iRiverAttackPenalty;
	int iAmphibiousAttack;
	int iAmphibiousAttackPenalty;
	int iAttackerCombatOddsPermille;
	bool bLuckEligible;
};
static std::vector<SASGameRecordCombatPending> g_aSASGameRecordCombatPending;

// <!-- custom: Naval blockades persist across turns but CivUnit stores only the current boolean state, not when the present blockade began.
// Keep tiny recorder-local start context so one rare end row can report observed duration and accumulated plunder without adding savegame fields or per-turn blockade spam. Mid-save sessions gracefully fall back to startKnown=0. (ChatGPT-5.6-Sol) -->
struct SASGameRecordBlockadeContext
{
	PlayerTypes ePlayer;
	int iUnitId;
	int iStartTurn;
	int iStartElapsedTurn;
	int iStartX;
	int iStartY;
	int iRangePlots;
	int iAffectedTeams;
	int iAffectedCities;
	CvString szRangePlots;
	CvString szAffectedTeams;
	CvString szAffectedCities;
	int iPlunderEvents;
	int iGoldPlundered;
	std::vector<std::pair<PlayerTypes,int> > aPlunderedCities;
};
static std::vector<SASGameRecordBlockadeContext> g_aSASGameRecordBlockades;

// <!-- custom: These counters reset whenever a new GameRecord log session begins, including after loading a save.
// Name them as logged observations rather than misleading lifetime totals. See KI#379. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
static int g_aiSASGameRecordLoggedGoldenAgeTurns[MAX_PLAYERS];
static int g_aiSASGameRecordLoggedAnarchyTurns[MAX_PLAYERS];
// <!-- custom: Observe each player's finalized research target once per player turn.
// This recorder-local state detects real incomplete-tech redirections without instrumenting every queue-mutating gameplay path or inventing a cause that the observation cannot prove.
// Repeat-tech counts distinguish a completed repeat from a true redirect. (ChatGPT-5.6-Sol) -->
struct SASGameRecordResearchPrevious
{
	bool bValid;
	TeamTypes eTeam;
	TechTypes eTech;
	int iTechCount;
	ResearchTargetChangeCause ePendingCause;
};
static SASGameRecordResearchPrevious g_akSASGameRecordResearchPrevious[MAX_PLAYERS];
// <!-- custom: CvPlayer::doResearch knows the exact split between this turn's modified research and previously stored unmodified overflow before both are combined into team progress.
// Retain that tiny level-2-only application context until an actual same-turn completion consumes it; this keeps RESEARCH_COMPLETED exact without widening generic gameplay research APIs for logging. (ChatGPT-5.6-Sol) -->
struct SASGameRecordResearchApplication
{
	bool bValid;
	int iGameTurn;
	TechTypes eTech;
	int iModifiedResearchRate;
	int iIncomingOverflowUnmodified;
	int iIncomingOverflowModified;
};
static SASGameRecordResearchApplication g_akSASGameRecordResearchApplication[MAX_PLAYERS];
// <!-- custom: Raze diagnostics use a tiny LIFO context stack rather than exposing recorder-specific state to CvPlayer.
// Python cityRazed callbacks can theoretically trigger nested gameplay, so a stack remains correct where one global pending slot would not. (ChatGPT-5.6-Sol) -->
struct SASGameRecordCityRazeContext
{
	PlayerTypes eRazer;
	TeamTypes eRazerTeam;
	PlayerTypes ePreviousOwner;
	TeamTypes ePreviousTeam;
	PlayerTypes eOriginalOwner;
	TeamTypes eOriginalTeam;
	int iGameTurn;
	int iCityId;
	CvWString szCityName;
	int iX;
	int iY;
	int iArea;
	CvString szRazeMode;
	int iPopulation;
	int iHighestPopulation;
	int iFoundedTurn;
	int iAcquiredTurn;
	int iOccupationTurns;
	int iRazerCulturePercent;
	int iPreviousCulturePercent;
	PlayerTypes eHighestCulturePlayer;
	int iHighestCulturePercent;
	int iMaintenanceTimes100;
	int iConnectedToCapital;
	int iCapitalDistance;
	int iCapitalSameArea;
	int iNearestRazerCityDistance;
	int iSameAreaRazerCitiesOther;
	int iNearestPreviousOwnerCityDistance;
	int iSameAreaPreviousOwnerCities;
	int iBuildings;
	int iRegularBuildings;
	int iNationalWonders;
	int iTeamWonders;
	int iWorldWonders;
	CvString szBuildings;
	CvString szReligions;
	CvString szHolyReligions;
	CvString szCorporations;
	CvString szHeadquarters;
	int iPlayerCitiesBefore;
	int iPlayerLandBefore;
	int iPlayerPopulationBefore;
	int iTeamCitiesBefore;
	int iTeamLandBefore;
	int iTeamPopulationBefore;
	int iWorldPopulationBefore;
	int iLandPctX100Before;
	int iPopPctX100Before;
	int iAIMaxVictoryStage;
	int iAIConquestStage;
	int iAIDominationStage;
	CvString szLandPopVictoryProgressBefore;
};
static std::vector<SASGameRecordCityRazeContext> g_aSASGameRecordCityRazeContexts;
static int g_aiSASGameRecordCitiesAcquired[MAX_PLAYERS];
static int g_aiSASGameRecordCitiesLost[MAX_PLAYERS];
static int g_aiSASGameRecordCitiesConquered[MAX_PLAYERS];
static int g_aiSASGameRecordCitiesLostByConquest[MAX_PLAYERS];
static int g_aiSASGameRecordCitiesTradedIn[MAX_PLAYERS];
static int g_aiSASGameRecordCitiesTradedOut[MAX_PLAYERS];
// <!-- custom: Recorder-local autoplay state makes each start/end row self-contained and counts active-player transfers without adding savegame fields.
// AI Auto Play is stopped when a save is loaded, so resetting this state with each log session matches the actual automation boundary. See KI#203. (GPT-5.6-Sol) -->
static int g_iSASGameRecordAutoPlayRequestId = 0;
static int g_iSASGameRecordAutoPlayRequestedTurns = 0;
static int g_iSASGameRecordAutoPlayStartTurn = -1;
static int g_iSASGameRecordAutoPlayStartElapsedTurn = -1;
static PlayerTypes g_eSASGameRecordAutoPlayStartPlayer = NO_PLAYER;
// <!-- custom: Track real per-request autoplay wall time with the same monotonic timer used by snapshot timing. (ChatGPT-5.6-Sol) -->
static uint g_uiSASGameRecordAutoPlayStartTime = 0;
static int g_iSASGameRecordAutoPlayPlayerChanges = 0;
static int g_iSASGameRecordTotalActivePlayerChanges = 0;
static int g_iSASGameRecordLastFullSnapshotTurn = -1;
// <!-- custom: Battle counters reset at actual snapshot boundaries, which need not match the configured periodic interval after loading or a victory flush.
// Track their real inclusive start like the newer flow buckets. See KI#378. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
static int g_iSASGameRecordBattleStartTurn = 0;
static int g_iSASGameRecordFlowStartTurn = 0;

// <!-- custom: Recorder-only production categories are shared by the interval transition matrix and exact target-change formatting.
// Keep the enum beside the flow state so its size is available before any serializer uses it. (ChatGPT-5.6-Sol) -->
enum SASGameRecordProductionKindIndex
{
	SAS_PRODUCTION_UNIT = 0,
	SAS_PRODUCTION_BUILDING,
	SAS_PRODUCTION_WONDER,
	SAS_PRODUCTION_PROJECT,
	SAS_PRODUCTION_PROCESS,
	NUM_SAS_PRODUCTION_KINDS
};

// <!-- custom: Level 3 preserves exact production, military and natural city-population evidence, but thousands of routine rows can distract broad analysis. Accumulate the same strategic totals into compact per-player interval rows for level 2+; dynamic type buckets use the loaded mod's XML rather than fixed BTS categories.
// `productionNeeded` is the loaded ruleset's production threshold at the action time, providing a consistent material-cost comparison; it is not a claim about raw hammers historically invested after production modifiers. (GPT-5.6-Sol + ChatGPT-5.6-Sol) -->
struct SASGameRecordPlayerFlow
{
	int iUnitsCompleted;
	int iUnitsConscripted;
	int iColonyFreeDefenders;
	int iUnitProductionNeeded;
	int iConscriptProductionNeeded;
	int iColonyFreeDefenderProductionNeeded;
	int iBuildingsCompleted;
	int iBuildingProductionNeeded;
	int iProjectsCompleted;
	int iProjectProductionNeeded;
	int iOverflowActions;
	int iRawModifiedOverflow;
	int iUnmodifiedOverflow;
	int iKeptOverflow;
	int iLostProduction;
	int iUnusedOverflowCapacity;
	int iOverflowGold;
	int iFailedInvestedProduction;
	int iFailGold;
	// <!-- custom: Natural city population flow is compacted into interval totals at level 2; level 3 additionally keeps exact city transitions.
	// Hurries, conscription, events, conquest/razing and other non-growth population changes retain their own existing/provenance-specific boundaries rather than being mixed into these natural-growth counters. (ChatGPT-5.6-Sol) -->
	int iCityGrowthEvents;
	int iPopulationGainedFromGrowth;
	int iCityGrowthPreventedEvents;
	int iFoodDiscardedByAvoidGrowth;
	int iCityStarvationEvents;
	int iPopulationLostToStarvation;
	// <!-- custom: AI production-target flow distinguishes ordinary completion->next selection from real active-target switching.
	// Stored production is parked rather than assumed wasted; separate decay/fail/overflow counters own actual mechanical loss. (ChatGPT-5.6-Sol) -->
	int iAIProductionTargetSwitches;
	int iAIProductionTargetClears;
	int iAIProductionInvestedTargetChanges;
	int iAIProductionParked;
	int iAIProductionTargetResumes;
	int iAIProductionResumed;
	int iProductionDecayActions;
	int iProductionDecayLost;
	int iProductionInvalidatedActions;
	int iProductionInvalidatedLost;
	int iProductionUpgradeTransfers;
	int iProductionUpgradeTransferred;
	int iProductionUpgradeOverwriteActions;
	int iProductionUpgradeOverwritten;
	int aiAIProductionTransitions[NUM_SAS_PRODUCTION_KINDS * NUM_SAS_PRODUCTION_KINDS];
	std::vector<std::pair<int,int> > aAIProductionTargetChangesByCity;
	int iUpgrades;
	int iUpgradeGold;
	int iScrapped;
	int iScrappedProductionNeeded;
	int iCaptured;
	int iCapturedProductionNeeded;
	int iCombatWins;
	int iCombatLosses;
	int iCityPlotWins;
	int iCityPlotLosses;
	int iEnemyProductionNeededDestroyed;
	int iOwnProductionNeededLost;
	int iExperienceGained;
	int iCombatExperienceGained;
	int iNonCombatExperienceGained;
	int iExperiencePreventedByCap;
	int iExperienceLostAdjustments;
	int iPromotionsChosen;
	int iLeaderPromotionApplications;
	int iEnemyExperienceDestroyed;
	int iOwnExperienceLost;
	std::vector<int> aiUnitTypes;
	std::vector<int> aiConscriptedUnitTypes;
	std::vector<int> aiColonyFreeDefenderUnitTypes;
	std::vector<int> aiBuildingTypes;
	std::vector<int> aiProjectTypes;
	std::vector<int> aiPromotionChoices;

	void reset()
	{
		iUnitsCompleted = 0;
		iUnitsConscripted = 0;
		iColonyFreeDefenders = 0;
		iUnitProductionNeeded = 0;
		iConscriptProductionNeeded = 0;
		iColonyFreeDefenderProductionNeeded = 0;
		iBuildingsCompleted = 0;
		iBuildingProductionNeeded = 0;
		iProjectsCompleted = 0;
		iProjectProductionNeeded = 0;
		iOverflowActions = 0;
		iRawModifiedOverflow = 0;
		iUnmodifiedOverflow = 0;
		iKeptOverflow = 0;
		iLostProduction = 0;
		iUnusedOverflowCapacity = 0;
		iOverflowGold = 0;
		iFailedInvestedProduction = 0;
		iFailGold = 0;
		iCityGrowthEvents = 0;
		iPopulationGainedFromGrowth = 0;
		iCityGrowthPreventedEvents = 0;
		iFoodDiscardedByAvoidGrowth = 0;
		iCityStarvationEvents = 0;
		iPopulationLostToStarvation = 0;
		iAIProductionTargetSwitches = 0;
		iAIProductionTargetClears = 0;
		iAIProductionInvestedTargetChanges = 0;
		iAIProductionParked = 0;
		iAIProductionTargetResumes = 0;
		iAIProductionResumed = 0;
		iProductionDecayActions = 0;
		iProductionDecayLost = 0;
		iProductionInvalidatedActions = 0;
		iProductionInvalidatedLost = 0;
		iProductionUpgradeTransfers = 0;
		iProductionUpgradeTransferred = 0;
		iProductionUpgradeOverwriteActions = 0;
		iProductionUpgradeOverwritten = 0;
		for (int iI = 0; iI < NUM_SAS_PRODUCTION_KINDS * NUM_SAS_PRODUCTION_KINDS; iI++) aiAIProductionTransitions[iI] = 0;
		aAIProductionTargetChangesByCity.clear();
		iUpgrades = 0;
		iUpgradeGold = 0;
		iScrapped = 0;
		iScrappedProductionNeeded = 0;
		iCaptured = 0;
		iCapturedProductionNeeded = 0;
		iCombatWins = 0;
		iCombatLosses = 0;
		iCityPlotWins = 0;
		iCityPlotLosses = 0;
		iEnemyProductionNeededDestroyed = 0;
		iOwnProductionNeededLost = 0;
		iExperienceGained = 0;
		iCombatExperienceGained = 0;
		iNonCombatExperienceGained = 0;
		iExperiencePreventedByCap = 0;
		iExperienceLostAdjustments = 0;
		iPromotionsChosen = 0;
		iLeaderPromotionApplications = 0;
		iEnemyExperienceDestroyed = 0;
		iOwnExperienceLost = 0;
		aiUnitTypes.assign(GC.getNumUnitInfos(), 0);
		aiConscriptedUnitTypes.assign(GC.getNumUnitInfos(), 0);
		aiColonyFreeDefenderUnitTypes.assign(GC.getNumUnitInfos(), 0);
		aiBuildingTypes.assign(GC.getNumBuildingInfos(), 0);
		aiProjectTypes.assign(GC.getNumProjectInfos(), 0);
		aiPromotionChoices.assign(GC.getNumPromotionInfos(), 0);
	}

	bool hasProduction() const
	{
		return (iUnitsCompleted > 0 || iUnitsConscripted > 0 || iColonyFreeDefenders > 0 || iBuildingsCompleted > 0 || iProjectsCompleted > 0 || iOverflowActions > 0 || iFailedInvestedProduction > 0 || iFailGold > 0 ||
			iAIProductionTargetSwitches > 0 || iAIProductionTargetClears > 0 || iAIProductionTargetResumes > 0 || iProductionDecayActions > 0 || iProductionInvalidatedActions > 0 || iProductionUpgradeTransfers > 0 || iProductionUpgradeOverwritten > 0);
	}

	bool hasMilitary() const
	{
		return (iUpgrades > 0 || iScrapped > 0 || iCaptured > 0 || iCombatWins > 0 || iCombatLosses > 0 || iExperienceGained > 0 || iExperiencePreventedByCap > 0 || iExperienceLostAdjustments > 0 || iPromotionsChosen > 0 || iLeaderPromotionApplications > 0 || iEnemyExperienceDestroyed > 0 || iOwnExperienceLost > 0);
	}

	bool hasCityPopulationFlow() const
	{
		return (iCityGrowthEvents > 0 || iCityGrowthPreventedEvents > 0 || iCityStarvationEvents > 0);
	}
};

static SASGameRecordPlayerFlow g_akSASGameRecordPlayerFlow[MAX_PLAYERS];

struct SASGameRecordPlotChangeGroup
{
	CvString szCategory;
	std::vector<std::pair<int,int> > aCoordinates;
};

struct SASGameRecordWorkerImprovementTransition
{
	ImprovementTypes eOldImprovement;
	ImprovementTypes eNewImprovement;
	int iCount;
	SASGameRecordWorkerImprovementTransition(ImprovementTypes eOld, ImprovementTypes eNew) : eOldImprovement(eOld), eNewImprovement(eNew), iCount(1) {}
};

struct SASGameRecordWorkerCompletedReplacement
{
	int iTurn;
	ImprovementTypes eOldImprovement;
	ImprovementTypes eNewImprovement;
	SASGameRecordWorkerCompletedReplacement() : iTurn(-1), eOldImprovement(NO_IMPROVEMENT), eNewImprovement(NO_IMPROVEMENT) {}
};

// <!-- custom: Keep Worker Build history compact at broad GameRecord level 2: one row per player/turn with completed work retains factual completed work and XML-driven carrier/maturation outcomes, while BBAI remains responsible for rejected candidates and numeric AI valuation. (GPT-5.6-Sol + ChatGPT-5.6-Sol) -->
struct SASGameRecordWorkerBuildTurn
{
	int iCompleted;
	int iImprovementChanges;
	int iNewImprovements;
	int iImprovementRemovals;
	int iImprovementReplacements;
	int iWorkedImprovementReplacements;
	int iUnworkedImprovementReplacements;
	int iDirectReversalsObserved;
	int iDirectReversalsWithin5TurnsObserved;
	int iRouteChanges;
	int iFeatureRemovals;
	int iBFC;
	int iOutsideBFC;
	int iBonus;
	int iBonusImprovementResults;
	int iIrrigationCarrierResults;
	int iIrrigatedCarrierResults;
	int iDryCarrierResults;
	int iIrrigationCarrierReplacements;
	int iIrrigationCarrierLosses;
	int iGrowthChainResults;
	int iGrowthChainReplacements;
	std::vector<int> aiBuilds;
	std::vector<int> aiFeaturesRemoved;
	std::vector<SASGameRecordWorkerImprovementTransition> aImprovementTransitions;

	SASGameRecordWorkerBuildTurn() { reset(); }
	void reset()
	{
		iCompleted = 0;
		iImprovementChanges = 0;
		iNewImprovements = 0;
		iImprovementRemovals = 0;
		iImprovementReplacements = 0;
		iWorkedImprovementReplacements = 0;
		iUnworkedImprovementReplacements = 0;
		iDirectReversalsObserved = 0;
		iDirectReversalsWithin5TurnsObserved = 0;
		iRouteChanges = 0;
		iFeatureRemovals = 0;
		iBFC = 0;
		iOutsideBFC = 0;
		iBonus = 0;
		iBonusImprovementResults = 0;
		iIrrigationCarrierResults = 0;
		iIrrigatedCarrierResults = 0;
		iDryCarrierResults = 0;
		iIrrigationCarrierReplacements = 0;
		iIrrigationCarrierLosses = 0;
		iGrowthChainResults = 0;
		iGrowthChainReplacements = 0;
		std::fill(aiBuilds.begin(), aiBuilds.end(), 0);
		std::fill(aiFeaturesRemoved.begin(), aiFeaturesRemoved.end(), 0);
		aImprovementTransitions.clear();
	}
};

static int g_iSASGameRecordPendingPlotTurn = -1;

// <!-- custom: Keep the city-bombard member named szMode. During the 6385 city-raze logging work it was accidentally renamed to szRazeMode while the existing bombard code still referenced szMode, causing MSVC C2039 compile errors.
// The separate city-raze context intentionally owns szRazeMode instead. (ChatGPT-5.6-Sol) -->
struct SASGameRecordCityBombardPending
{
	bool bValid;
	int iTurn;
	CvString szMode;
	PlayerTypes ePlayer;
	PlayerTypes eTargetPlayer;
	int iCityId;
	CvWString szCity;
	int iX;
	int iY;
	int iActions;
	int iBombardRateTotal;
	int iIgnoreBuildingDefenseActions;
	int iDefenseModifierBefore;
	int iDefenseModifierAfter;
	int iTotalDefense;
	int iDefenseDamageBefore;
	int iDefenseDamageAfter;
	int iDefenseDamageMax;
	std::vector<std::pair<CvString,int> > aUnitTypes;
	std::vector<std::pair<CvString,int> > aUnitAIs;
	SASGameRecordCityBombardPending() : bValid(false), iTurn(-1), ePlayer(NO_PLAYER), eTargetPlayer(NO_PLAYER), iCityId(-1), iX(-1), iY(-1), iActions(0), iBombardRateTotal(0), iIgnoreBuildingDefenseActions(0), iDefenseModifierBefore(-1), iDefenseModifierAfter(-1), iTotalDefense(-1), iDefenseDamageBefore(-1), iDefenseDamageAfter(-1), iDefenseDamageMax(-1) {}
};

static SASGameRecordCityBombardPending g_kSASGameRecordPendingCityBombard;
static std::vector<SASGameRecordPlotChangeGroup> g_aSASGameRecordPlotChanges;
static SASGameRecordWorkerBuildTurn g_akSASGameRecordWorkerBuildTurn[MAX_PLAYERS];
// <!-- custom: Remember only completed replacements observed in this log session.
// This measures realized A>B>A churn without copying BBAI's candidate scoring or pretending that pre-load history is known. (GPT-5.6-Sol) -->
static std::map<std::pair<int,int>, SASGameRecordWorkerCompletedReplacement> g_mapSASGameRecordWorkerCompletedReplacements;
static std::vector<std::pair<int,int> > g_aaSASGameRecordRevealedPlots[MAX_TEAMS];
static TeamTypes g_eSASGameRecordFullMapRevelationTeam = NO_TEAM;
static int g_iSASGameRecordFullMapRevealedBefore = 0;

struct SASGameRecordPlayerPrevious
{
	bool bValid;
	int iScore;
	int iCities;
	int iPopulation;
	int iLand;
	int iUnits;
	int iCombatUnits;
	int iMilitarySupportUnits;
	int iPower;
	int iGold;
	int iGoldRate;
	int iResearchRate;
	int iBonusTypes;
	int iBonusInstances;
	int iBonusImports;
	int iBonusExports;
	int iHistoryScore;
	int iHistoryEconomy;
	int iHistoryIndustry;
	int iHistoryAgriculture;
	int iHistoryPower;
	int iHistoryCulture;
	int iHistoryEspionage;
	int iEspionageRate;
	int iEspionagePercent;
	int iTeamEP;
	int iUnspentEP;
	int iDemoScore;
	int iDemoPopulation;
	int iDemoLand;
	int iDemoFood;
	int iDemoProduction;
	int iDemoCommerce;
	int iDemoResearch;
	int iDemoCulture;
	int iDemoEspionage;
	int iDemoGoldRate;
	int iDemoPower;
	int iUnitTotal;
	int iUnitMilitary;
	int iUnitWorkers;
	int iUnitSettlers;
	int iUnitFieldArmy;
	int iUnitCityDefenders;
	int iUnitEnemyUnitsInTerritory;
	int iUnitTotalExperience;
	int iUnitPromotionReady;
	int iWorkerWorkers;
	int iWorkerBuilding;
	int iWorkerIdle;
	int iWorkerMoving;
	int iWorkerWaiting;
	int iWorkerThreatened;
	int iTerritoryImprovedLand;
	int iTerritoryImprovedWater;
	int iTerritoryRoaded;
	int iTerritoryBFCDevelopmentLandRoaded;
	int iTerritorySuburbDevelopmentLandRoaded;
	int iTerritoryIrrigationCarriers;
	int iTerritoryIrrigatedCarriers;
	int iTerritoryDryCarriers;
	int iSettlerSettlers;
	int iSettlerFoundMission;
	int iSettlerFoundIntent;
	int iSettlerMoving;
	int iSettlerIdle;
	int iSettlerWaiting;
	int iSettlerThreatened;
	int iCityCount;
	int iCityConnectedToCapital;
	int iCityFoodSurplus;
	int iCityHappySurplus;
	int iCityHealthSurplus;
	int iCityFood;
	int iCityProduction;
	int iCityCommerce;
	int iCityTradeRoutes;
	int iCityTradeCommerce;
	int iCityAngryPopulation;
	int iCityWorkingPopulation;
	int iCityAssignedSpecialists;
	int iCityFreeSpecialistInstances;
	int iCityFreeSpecialistAllowance;
	int iCityGarrison;
};

struct SASGameRecordTeamPrevious
{
	bool bValid;
	bool bContactsValid;
	int iTechs;
	int iLand;
	int iLandPctX100;
	int iPopulation;
	int iPopPctX100;
	int iMetTeams;
};

struct SASGameRecordGlobalPrevious
{
	bool bValid;
	int iGlobalWarmingIndex;
	int iGlobalWarmingChances;
	int iOwnedLand;
	int iUnownedLand;
};

struct SASGameRecordPlotComposition
{
	int iPlots;
	int iLand;
	int iWater;
	int iHills;
	int iPeaks;
	int iRiverSide;
	int iFreshWater;
	int iCoastal;
	int iImproved;
	int iUnimprovedLand;
	int iRoaded;
	int iBonusImproved;
	int iBonusUnimproved;
	int iWorked;
	int iWorkedImproved;
	int iWorkedUnimproved;
	int iNatureFood;
	int iNatureProduction;
	int iNatureCommerce;
	int iCurrentFood;
	int iCurrentProduction;
	int iCurrentCommerce;
	std::vector<int> aiTerrains;
	std::vector<int> aiFeatures;
	std::vector<int> aiBonuses;
	std::vector<int> aiImprovements;
	std::vector<int> aiRoutes;

	SASGameRecordPlotComposition() : iPlots(0), iLand(0), iWater(0), iHills(0), iPeaks(0), iRiverSide(0), iFreshWater(0), iCoastal(0), iImproved(0), iUnimprovedLand(0), iRoaded(0), iBonusImproved(0), iBonusUnimproved(0), iWorked(0), iWorkedImproved(0), iWorkedUnimproved(0), iNatureFood(0), iNatureProduction(0), iNatureCommerce(0), iCurrentFood(0), iCurrentProduction(0), iCurrentCommerce(0), aiTerrains(GC.getNumTerrainInfos(), 0), aiFeatures(GC.getNumFeatureInfos(), 0), aiBonuses(GC.getNumBonusInfos(), 0), aiImprovements(GC.getNumImprovementInfos(), 0), aiRoutes(GC.getNumRouteInfos(), 0) {}
};

struct SASGameRecordTerritoryDevelopment
{
	SASGameRecordPlotComposition kOwned;
	std::vector<int> aiImprovedBonuses;
	std::vector<int> aiUnimprovedBonuses;
	std::vector<int> aiIrrigatedCarriers;
	std::vector<int> aiDryCarriers;
	std::vector<int> aiBFCDevelopmentRoutes;
	std::vector<int> aiSuburbDevelopmentRoutes;
	int iBFCPlots;
	int iSuburbPlots;
	int iDevelopmentLand;
	int iDevelopmentWater;
	int iImprovedLand;
	int iImprovedWater;
	int iBFCDevelopmentLand;
	int iBFCImprovedLand;
	int iBFCDevelopmentLandRoaded;
	int iBFCDevelopmentWater;
	int iBFCImprovedWater;
	int iSuburbDevelopmentLand;
	int iSuburbImprovedLand;
	int iSuburbDevelopmentLandRoaded;
	int iSuburbDevelopmentWater;
	int iSuburbImprovedWater;
	int iIrrigationCarriers;
	int iIrrigatedCarriers;
	int iDryCarriers;
	int iBonusIrrigationCarriers;
	int iIrrigatedBonusCarriers;
	int iDryBonusCarriers;
	int iBFCIrrigationCarriers;
	int iBFCIrrigatedCarriers;
	int iBFCDryCarriers;
	SASGameRecordTerritoryDevelopment() : aiImprovedBonuses(GC.getNumBonusInfos(), 0), aiUnimprovedBonuses(GC.getNumBonusInfos(), 0), aiIrrigatedCarriers(GC.getNumImprovementInfos(), 0), aiDryCarriers(GC.getNumImprovementInfos(), 0), aiBFCDevelopmentRoutes(GC.getNumRouteInfos(), 0), aiSuburbDevelopmentRoutes(GC.getNumRouteInfos(), 0), iBFCPlots(0), iSuburbPlots(0), iDevelopmentLand(0), iDevelopmentWater(0), iImprovedLand(0), iImprovedWater(0), iBFCDevelopmentLand(0), iBFCImprovedLand(0), iBFCDevelopmentLandRoaded(0), iBFCDevelopmentWater(0), iBFCImprovedWater(0), iSuburbDevelopmentLand(0), iSuburbImprovedLand(0), iSuburbDevelopmentLandRoaded(0), iSuburbDevelopmentWater(0), iSuburbImprovedWater(0), iIrrigationCarriers(0), iIrrigatedCarriers(0), iDryCarriers(0), iBonusIrrigationCarriers(0), iIrrigatedBonusCarriers(0), iDryBonusCarriers(0), iBFCIrrigationCarriers(0), iBFCIrrigatedCarriers(0), iBFCDryCarriers(0) {}
};

struct SASGameRecordCityNetwork
{
	std::vector<int> aiRoutes;
	CvString szCities;
	int iPlots;
	int iLandPlots;
	int iWaterPlots;
	int iRoutePlots;
	int iRiverNetworkPlots;
	int iLandTerrainNetworkPlots;
	int iWaterNetworkPlots;
	int iCities;
	int iOwnCities;
	int iForeignCities;
	bool bContainsCapital;
	SASGameRecordCityNetwork() : aiRoutes(GC.getNumRouteInfos(), 0), iPlots(0), iLandPlots(0), iWaterPlots(0), iRoutePlots(0), iRiverNetworkPlots(0), iLandTerrainNetworkPlots(0), iWaterNetworkPlots(0), iCities(0), iOwnCities(0), iForeignCities(0), bContainsCapital(false) {}
};

static SASGameRecordPlayerPrevious g_akSASGameRecordPlayerPrevious[MAX_PLAYERS];
static SASGameRecordTeamPrevious g_akSASGameRecordTeamPrevious[MAX_TEAMS];
static SASGameRecordGlobalPrevious g_kSASGameRecordGlobalPrevious;

// <!-- custom: A team-pair war is a natural historical unit that action rows otherwise force external tools to reconstruct across many turns.
// Keep this transient and recorder-local: loaded saves start an explicitly partial observation, while declarations in the current log retain their exact start/cause.
// Battles and conquered cities are attributed only to the two teams directly involved, so simultaneous wars do not contaminate one another. (GPT-5.6-Sol + GPT-5.6 Thinking) -->
struct SASGameRecordWarSummary
{
	TeamTypes eTeamA;
	TeamTypes eTeamB;
	TeamTypes eDeclarer;
	TeamTypes eTarget;
	WarPlanTypes eInitialWarPlan;
	CvString szStartCause;
	bool bStartKnown;
	bool bPrimary;
	int iStartTurn;
	int iObservedStartTurn;
	int iWarSuccessStartA;
	int iWarSuccessStartB;
	int iWarSuccessA;
	int iWarSuccessB;
	int iUnitsDestroyedByA;
	int iUnitsDestroyedByB;
	int iProductionDestroyedByA;
	int iProductionDestroyedByB;
	int iCityPlotWinsA;
	int iCityPlotWinsB;
	int iCitiesCapturedByA;
	int iCitiesCapturedByB;
	int iPopulationCapturedByA;
	int iPopulationCapturedByB;
	int iLastOngoingSummaryTurn;

	SASGameRecordWarSummary() : eTeamA(NO_TEAM), eTeamB(NO_TEAM), eDeclarer(NO_TEAM), eTarget(NO_TEAM), eInitialWarPlan(NO_WARPLAN), bStartKnown(false), bPrimary(false),
			iStartTurn(-1), iObservedStartTurn(-1), iWarSuccessStartA(0), iWarSuccessStartB(0), iWarSuccessA(0), iWarSuccessB(0),
			iUnitsDestroyedByA(0), iUnitsDestroyedByB(0), iProductionDestroyedByA(0), iProductionDestroyedByB(0), iCityPlotWinsA(0), iCityPlotWinsB(0),
			iCitiesCapturedByA(0), iCitiesCapturedByB(0), iPopulationCapturedByA(0), iPopulationCapturedByB(0), iLastOngoingSummaryTurn(-1) {}
};

static std::vector<SASGameRecordWarSummary> g_aSASGameRecordWars;

static bool getSASGameRecordWarPair(TeamTypes eFirst, TeamTypes eSecond, TeamTypes& eTeamA, TeamTypes& eTeamB)
{
	if (eFirst < 0 || eFirst >= MAX_CIV_TEAMS || eSecond < 0 || eSecond >= MAX_CIV_TEAMS || eFirst == eSecond)
		return false;
	eTeamA = (eFirst < eSecond ? eFirst : eSecond);
	eTeamB = (eFirst < eSecond ? eSecond : eFirst);
	return true;
}

static SASGameRecordWarSummary* findSASGameRecordWar(TeamTypes eFirst, TeamTypes eSecond)
{
	TeamTypes eTeamA;
	TeamTypes eTeamB;
	if (!getSASGameRecordWarPair(eFirst, eSecond, eTeamA, eTeamB))
		return NULL;
	for (size_t iI = 0; iI < g_aSASGameRecordWars.size(); iI++)
	{
		SASGameRecordWarSummary& kWar = g_aSASGameRecordWars[iI];
		if (kWar.eTeamA == eTeamA && kWar.eTeamB == eTeamB)
			return &kWar;
	}
	return NULL;
}

static void refreshSASGameRecordWarSuccess(SASGameRecordWarSummary& kWar)
{
	if (kWar.eTeamA < 0 || kWar.eTeamB < 0)
		return;
	kWar.iWarSuccessA = GET_TEAM(kWar.eTeamA).AI_getWarSuccess(kWar.eTeamB).round();
	kWar.iWarSuccessB = GET_TEAM(kWar.eTeamB).AI_getWarSuccess(kWar.eTeamA).round();
}

// <!-- custom: Release builds remove FAssert expressions entirely.
// Calling getSASGameRecordWarPair only from an assertion left both normalized teams at NO_TEAM and reproducibly crashed turn-110 reconciliation; validate in executed code and return NULL instead. (GPT-5.6-Sol) -->
static SASGameRecordWarSummary* addSASGameRecordWar(TeamTypes eFirst, TeamTypes eSecond, bool bStartKnown, TeamTypes eDeclarer, TeamTypes eTarget, WarPlanTypes eWarPlan, char const* szStartCause, bool bPrimary)
{
	TeamTypes eTeamA;
	TeamTypes eTeamB;
	if (!getSASGameRecordWarPair(eFirst, eSecond, eTeamA, eTeamB))
		return NULL;
	SASGameRecordWarSummary* pExisting = findSASGameRecordWar(eTeamA, eTeamB);
	if (pExisting != NULL)
		return pExisting;
	g_aSASGameRecordWars.push_back(SASGameRecordWarSummary());
	SASGameRecordWarSummary& kWar = g_aSASGameRecordWars.back();
	kWar.eTeamA = eTeamA;
	kWar.eTeamB = eTeamB;
	kWar.eDeclarer = eDeclarer;
	kWar.eTarget = eTarget;
	kWar.eInitialWarPlan = eWarPlan;
	kWar.szStartCause = szStartCause;
	kWar.bStartKnown = bStartKnown;
	kWar.bPrimary = bPrimary;
	kWar.iStartTurn = (bStartKnown ? GC.getGame().getGameTurn() : -1);
	kWar.iObservedStartTurn = GC.getGame().getGameTurn();
	refreshSASGameRecordWarSuccess(kWar);
	kWar.iWarSuccessStartA = kWar.iWarSuccessA;
	kWar.iWarSuccessStartB = kWar.iWarSuccessB;
	return &kWar;
}

static void initializeSASGameRecordWarsFromLoadedSave()
{
	for (int iA = 0; iA < MAX_CIV_TEAMS; iA++)
	{
		TeamTypes const eTeamA = (TeamTypes)iA;
		if (!GET_TEAM(eTeamA).isAlive())
			continue;
		for (int iB = iA + 1; iB < MAX_CIV_TEAMS; iB++)
		{
			TeamTypes const eTeamB = (TeamTypes)iB;
			if (GET_TEAM(eTeamB).isAlive() && GET_TEAM(eTeamA).isAtWar(eTeamB))
				addSASGameRecordWar(eTeamA, eTeamB, false, NO_TEAM, NO_TEAM, NO_WARPLAN, "PREEXISTING_ON_LOAD", false);
		}
	}
}

static void logSASGameRecordWarSummary(SASGameRecordWarSummary& kWar, char const* szStatus, char const* szTrigger, int iSummaryTurn, int iWarSuccessA, int iWarSuccessB, bool bCapitulate, TeamTypes eBroker, bool bRandomEvent, bool bReparations)
{
	kWar.iWarSuccessA = iWarSuccessA;
	kWar.iWarSuccessB = iWarSuccessB;
	bool const bEnded = (strcmp(szStatus, "ENDED") == 0);
	bool const bStateReconciliation = (strcmp(szTrigger, "STATE_RECONCILIATION") == 0);
	char const* szEndCause = (!bEnded ? "-" : (bStateReconciliation ? "STATE_CHANGE" : (bCapitulate ? "CAPITULATION" : (bRandomEvent ? "RANDOM_EVENT" : (eBroker != NO_TEAM ? "BROKERED_PEACE" : "PEACE")))));
	int const iElapsedTurns = (kWar.bStartKnown ? iSummaryTurn - kWar.iStartTurn : -1);
	int const iObservedTurns = iSummaryTurn - kWar.iObservedStartTurn;
	logSASGameRecord("GAME_RECORD_WAR_SUMMARY turn=%d status=%s trigger=%s teamA=%d teamB=%d startKnown=%d startTurn=%d observedStartTurn=%d endTurn=%d elapsedTurns=%d observedTurns=%d declarerTeam=%d targetTeam=%d startCause=%s initialWarPlan=%s primary=%d endCause=%s brokerTeam=%d reparations=%d warSuccessObservedStartA=%d warSuccessObservedStartB=%d warSuccessEndA=%d warSuccessEndB=%d warSuccessObservedGainA=%+d warSuccessObservedGainB=%+d unitsDestroyedByA=%d unitsDestroyedByB=%d unitProductionCostDestroyedByA=%d unitProductionCostDestroyedByB=%d cityPlotWinsA=%d cityPlotWinsB=%d citiesCapturedByA=%d citiesCapturedByB=%d populationCapturedByA=%d populationCapturedByB=%d",
		iSummaryTurn, szStatus, szTrigger, kWar.eTeamA, kWar.eTeamB, kWar.bStartKnown, kWar.iStartTurn, kWar.iObservedStartTurn,
		bEnded ? iSummaryTurn : -1, iElapsedTurns, iObservedTurns, kWar.eDeclarer, kWar.eTarget, kWar.szStartCause.GetCString(),
		getSASWarPlanType(kWar.eInitialWarPlan), kWar.bPrimary, szEndCause, eBroker, bReparations, kWar.iWarSuccessStartA,
		kWar.iWarSuccessStartB, kWar.iWarSuccessA, kWar.iWarSuccessB, kWar.iWarSuccessA - kWar.iWarSuccessStartA,
		kWar.iWarSuccessB - kWar.iWarSuccessStartB, kWar.iUnitsDestroyedByA, kWar.iUnitsDestroyedByB, kWar.iProductionDestroyedByA,
		kWar.iProductionDestroyedByB, kWar.iCityPlotWinsA, kWar.iCityPlotWinsB, kWar.iCitiesCapturedByA, kWar.iCitiesCapturedByB,
		kWar.iPopulationCapturedByA, kWar.iPopulationCapturedByB);
}

static void logSASGameRecordOngoingWarSummaries(char const* szReason)
{
	int const iGameTurn = GC.getGame().getGameTurn();
	for (size_t iI = 0; iI < g_aSASGameRecordWars.size(); iI++)
	{
		SASGameRecordWarSummary& kWar = g_aSASGameRecordWars[iI];
		if (kWar.iLastOngoingSummaryTurn == iGameTurn)
			continue;
		refreshSASGameRecordWarSuccess(kWar);
		logSASGameRecordWarSummary(kWar, "ONGOING", szReason, iGameTurn, kWar.iWarSuccessA, kWar.iWarSuccessB, false, NO_TEAM, false, false);
		kWar.iLastOngoingSummaryTurn = iGameTurn;
	}
}

static void reconcileSASGameRecordWars()
{
	for (size_t iI = 0; iI < g_aSASGameRecordWars.size();)
	{
		SASGameRecordWarSummary& kWar = g_aSASGameRecordWars[iI];
		if (GET_TEAM(kWar.eTeamA).isAtWar(kWar.eTeamB))
		{
			iI++;
			continue;
		}
		refreshSASGameRecordWarSuccess(kWar);
		logSASGameRecordWarSummary(kWar, "ENDED", "STATE_RECONCILIATION", GC.getGame().getGameTurn(), kWar.iWarSuccessA, kWar.iWarSuccessB, false, NO_TEAM, false, false);
		g_aSASGameRecordWars.erase(g_aSASGameRecordWars.begin() + iI);
	}
}

static int getSASGameRecordDelta(bool bValid, int iCurrent, int iPrevious)
{
	return bValid ? iCurrent - iPrevious : 0;
}

static void resetSASGameRecordState()
{
	// <!-- custom: Session rollover must stop the hot-path RNG hook until the new game's seeds or loaded save state establish a fresh level-3 baseline. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	clearSASGameRecordRngTracking();
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		g_aiSASGameRecordBattleWins[iI] = 0;
		g_aiSASGameRecordBattleLosses[iI] = 0;
		g_aiSASGameRecordCityBattleWins[iI] = 0;
		g_aiSASGameRecordCityBattleLosses[iI] = 0;
		g_aiSASGameRecordTotalBattleWins[iI] = 0;
		g_aiSASGameRecordTotalBattleLosses[iI] = 0;
		g_aiSASGameRecordTotalCityBattleWins[iI] = 0;
		g_aiSASGameRecordTotalCityBattleLosses[iI] = 0;
		g_akSASGameRecordBattleQuality[iI].reset();
		g_akSASGameRecordTotalBattleQuality[iI].reset();
		g_akSASGameRecordMilitaryQualityTotals[iI].reset();
		g_aiSASGameRecordLoggedGoldenAgeTurns[iI] = 0;
		g_aiSASGameRecordLoggedAnarchyTurns[iI] = 0;
		g_akSASGameRecordResearchPrevious[iI].bValid = false;
		g_akSASGameRecordResearchPrevious[iI].eTeam = NO_TEAM;
		g_akSASGameRecordResearchPrevious[iI].eTech = NO_TECH;
		g_akSASGameRecordResearchPrevious[iI].iTechCount = 0;
		g_akSASGameRecordResearchPrevious[iI].ePendingCause = RESEARCH_TARGET_CHANGE_UNKNOWN;
		g_akSASGameRecordResearchApplication[iI].bValid = false;
		g_akSASGameRecordResearchApplication[iI].iGameTurn = -1;
		g_akSASGameRecordResearchApplication[iI].eTech = NO_TECH;
		g_akSASGameRecordResearchApplication[iI].iModifiedResearchRate = 0;
		g_akSASGameRecordResearchApplication[iI].iIncomingOverflowUnmodified = 0;
		g_akSASGameRecordResearchApplication[iI].iIncomingOverflowModified = 0;
		g_aiSASGameRecordCitiesAcquired[iI] = 0;
		g_aiSASGameRecordCitiesLost[iI] = 0;
		g_aiSASGameRecordCitiesConquered[iI] = 0;
		g_aiSASGameRecordCitiesLostByConquest[iI] = 0;
		g_aiSASGameRecordCitiesTradedIn[iI] = 0;
		g_aiSASGameRecordCitiesTradedOut[iI] = 0;
		g_akSASGameRecordPlayerFlow[iI].reset();
		g_akSASGameRecordPlayerPrevious[iI].bValid = false;
		g_akSASGameRecordWorkerBuildTurn[iI].reset();
	}
	g_mapSASGameRecordWorkerCompletedReplacements.clear();
	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		g_akSASGameRecordTeamPrevious[iI].bValid = false;
		g_akSASGameRecordTeamPrevious[iI].bContactsValid = false;
	}
	g_kSASGameRecordGlobalPrevious.bValid = false;
	g_aSASGameRecordWars.clear();
	g_aSASGameRecordCityRazeContexts.clear();
	g_aSASGameRecordCombatPending.clear();
	g_aSASGameRecordBlockades.clear();
	g_iSASGameRecordLastFullSnapshotTurn = -1;
	g_iSASGameRecordBattleStartTurn = GC.getGame().getGameTurn();
	g_iSASGameRecordFlowStartTurn = GC.getGame().getGameTurn();
	g_iSASGameRecordPendingPlotTurn = -1;
	g_kSASGameRecordPendingCityBombard = SASGameRecordCityBombardPending();
	g_aSASGameRecordPlotChanges.clear();
	g_eSASGameRecordFullMapRevelationTeam = NO_TEAM;
	g_iSASGameRecordFullMapRevealedBefore = 0;
	// <!-- custom: AI Auto Play stops when a save is loaded, so clear its recorder-local request boundary and active-player transfer counters with the rest of the new log-session state. See KI#203. (GPT-5.6-Sol) -->
	g_iSASGameRecordAutoPlayRequestId = 0;
	g_iSASGameRecordAutoPlayRequestedTurns = 0;
	g_iSASGameRecordAutoPlayStartTurn = -1;
	g_iSASGameRecordAutoPlayStartElapsedTurn = -1;
	g_eSASGameRecordAutoPlayStartPlayer = NO_PLAYER;
	g_uiSASGameRecordAutoPlayStartTime = 0;
	g_iSASGameRecordAutoPlayPlayerChanges = 0;
	g_iSASGameRecordTotalActivePlayerChanges = 0;
	for (int iI = 0; iI < MAX_TEAMS; iI++)
		g_aaSASGameRecordRevealedPlots[iI].clear();
}

// <!-- custom: Team snapshots intentionally list living members, but CvTeam::addTeam reassigns every player slot on the absorbed team.
// Keep a separate exact helper for that rare structural boundary. (ChatGPT-5.6-Sol) -->
static CvString getSASGameRecordTeamAssignedPlayers(TeamTypes eTeam, int& iCount)
{
	iCount = 0;
	CvString szList;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		PlayerTypes const eLoopPlayer = (PlayerTypes)iI;
		if (GET_PLAYER(eLoopPlayer).getTeam() != eTeam)
			continue;
		appendSASDiagnosticIntListValue(szList, eLoopPlayer);
		iCount++;
	}
	return getSASDiagnosticOrDash(szList);
}

static CvString getSASGameRecordTeamMembers(TeamTypes eTeam)
{
	CvString szList;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes eLoopPlayer = (PlayerTypes)iI;
		CvPlayer const& kLoopPlayer = GET_PLAYER(eLoopPlayer);
		if (kLoopPlayer.isAlive() && kLoopPlayer.getTeam() == eTeam)
			appendSASDiagnosticIntListValue(szList, eLoopPlayer);
	}
	return getSASDiagnosticOrDash(szList);
}

static CvString getSASGameRecordWarTeams(TeamTypes eTeam)
{
	CvString szList;
	CvTeam const& kTeam = GET_TEAM(eTeam);
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		TeamTypes eLoopTeam = (TeamTypes)iI;
		if (eLoopTeam != eTeam && GET_TEAM(eLoopTeam).isAlive() && kTeam.isAtWar(eLoopTeam))
			appendSASDiagnosticIntListValue(szList, eLoopTeam);
	}
	return getSASDiagnosticOrDash(szList);
}

// <!-- custom: WAR_PLAN_CHANGED preserves exact transitions after the recorder begins, but a loaded save can already contain active/preparing plans.
// Keep the current team-level target/plan/age set in periodic state so strategic intent is reconstructible without replaying earlier logs or enabling UWAI diagnostics. (ChatGPT-5.6-Sol) -->
static CvString getSASGameRecordWarPlans(TeamTypes eTeam)
{
	CvString szList;
	CvTeamAI const& kTeam = GET_TEAM(eTeam);
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		TeamTypes const eLoopTeam = (TeamTypes)iI;
		if (eLoopTeam == eTeam || !GET_TEAM(eLoopTeam).isAlive() || GET_TEAM(eLoopTeam).isBarbarian())
			continue;
		WarPlanTypes const eWarPlan = kTeam.AI_getWarPlan(eLoopTeam);
		if (eWarPlan == NO_WARPLAN)
			continue;
		CvString szItem;
		szItem.Format(szList.empty() ? "%d:%s/%d" : ",%d:%s/%d", eLoopTeam, getSASWarPlanType(eWarPlan), kTeam.AI_getWarPlanStateCounter(eLoopTeam));
		szList += szItem;
	}
	return getSASDiagnosticOrDash(szList);
}

static CvString getSASGameRecordVassalTeams(TeamTypes eTeam)
{
	CvString szList;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		TeamTypes eLoopTeam = (TeamTypes)iI;
		if (eLoopTeam != eTeam && GET_TEAM(eLoopTeam).isAlive() && GET_TEAM(eLoopTeam).isVassal(eTeam))
			appendSASDiagnosticIntListValue(szList, eLoopTeam);
	}
	return getSASDiagnosticOrDash(szList);
}

static CvString getSASGameRecordMetTeams(TeamTypes eTeam)
{
	CvString szMetTeams;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		TeamTypes eLoopTeam = (TeamTypes)iI;
		if (eLoopTeam == eTeam || !GET_TEAM(eLoopTeam).isAlive() || GET_TEAM(eLoopTeam).isBarbarian())
			continue;
		if (GET_TEAM(eTeam).isHasMet(eLoopTeam))
			appendSASDiagnosticIntListValue(szMetTeams, eLoopTeam);
	}
	return getSASDiagnosticOrDash(szMetTeams);
}

static int getSASGameRecordMetTeamCount(TeamTypes eTeam)
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		TeamTypes eLoopTeam = (TeamTypes)iI;
		if (eLoopTeam != eTeam && GET_TEAM(eLoopTeam).isAlive() && !GET_TEAM(eLoopTeam).isBarbarian() && GET_TEAM(eTeam).isHasMet(eLoopTeam))
			iCount++;
	}
	return iCount;
}

static void logSASGameRecordTeamContacts(TeamTypes eTeam, int iGameTurn, const char* szReason)
{
	SASGameRecordTeamPrevious& kPrevious = g_akSASGameRecordTeamPrevious[eTeam];
	const int iMetTeams = getSASGameRecordMetTeamCount(eTeam);
	logSASGameRecord("GAME_RECORD_CONTACTS turn=%d reason=%s team=%d deltaValid=%d metCount=%d metCountDelta=%+d metTeams=%s",
		iGameTurn, szReason, eTeam, kPrevious.bContactsValid, iMetTeams,
		getSASGameRecordDelta(kPrevious.bContactsValid, iMetTeams, kPrevious.iMetTeams), getSASGameRecordMetTeams(eTeam).GetCString());
	kPrevious.bContactsValid = true;
	kPrevious.iMetTeams = iMetTeams;
}

static const char* getSASGameRecordTechType(TechTypes eTech)
{
	return (eTech == NO_TECH ? "-" : GC.getInfo(eTech).getType());
}

static const char* getSASGameRecordEventTriggerType(EventTriggerTypes eTrigger)
{
	return (eTrigger == NO_EVENTTRIGGER ? "-" : GC.getInfo(eTrigger).getType());
}

static const char* getSASGameRecordEventType(EventTypes eEvent)
{
	return (eEvent == NO_EVENT ? "-" : GC.getInfo(eEvent).getType());
}

static const char* getSASGameRecordGoodyType(GoodyTypes eGoody)
{
	return (eGoody == NO_GOODY ? "-" : GC.getInfo(eGoody).getType());
}

static const char* getSASGameRecordReligionType(ReligionTypes eReligion)
{
	return (eReligion == NO_RELIGION ? "-" : GC.getInfo(eReligion).getType());
}

static const char* getSASGameRecordCorporationType(CorporationTypes eCorporation)
{
	return (eCorporation == NO_CORPORATION ? "-" : GC.getInfo(eCorporation).getType());
}

static const char* getSASGameRecordBuildingType(BuildingTypes eBuilding)
{
	return (eBuilding == NO_BUILDING ? "-" : GC.getInfo(eBuilding).getType());
}

static const char* getSASGameRecordProjectType(ProjectTypes eProject)
{
	return (eProject == NO_PROJECT ? "-" : GC.getInfo(eProject).getType());
}

static const char* getSASGameRecordUnitType(UnitTypes eUnit)
{
	return (eUnit == NO_UNIT ? "-" : GC.getInfo(eUnit).getType());
}

static const char* getSASGameRecordBonusType(BonusTypes eBonus)
{
	return (eBonus == NO_BONUS ? "-" : GC.getInfo(eBonus).getType());
}

static const char* getSASGameRecordAttitudeType(AttitudeTypes eAttitude)
{
	return (eAttitude == NO_ATTITUDE ? "-" : GC.getInfo(eAttitude).getType());
}

static const char* getSASGameRecordTerrainType(TerrainTypes eTerrain)
{
	return (eTerrain == NO_TERRAIN ? "-" : GC.getInfo(eTerrain).getType());
}

static const char* getSASGameRecordFeatureType(FeatureTypes eFeature)
{
	return (eFeature == NO_FEATURE ? "-" : GC.getInfo(eFeature).getType());
}

static const char* getSASGameRecordImprovementType(ImprovementTypes eImprovement)
{
	return (eImprovement == NO_IMPROVEMENT ? "-" : GC.getInfo(eImprovement).getType());
}

static const char* getSASGameRecordRouteType(RouteTypes eRoute)
{
	return (eRoute == NO_ROUTE ? "-" : GC.getInfo(eRoute).getType());
}

static char const* getSASGameRecordPlotOwnerChangeCause()
{
	switch (g_eSASGameRecordPlotOwnerChangeCause)
	{
	case SAS_PLOT_OWNER_CAUSE_CITY_FOUNDING: return "CITY_FOUNDING";
	case SAS_PLOT_OWNER_CAUSE_CITY_ACQUISITION: return "CITY_ACQUISITION";
	case SAS_PLOT_OWNER_CAUSE_CULTURE_UPDATE: return "CULTURE_UPDATE";
	case SAS_PLOT_OWNER_CAUSE_WAR_BORDER: return "WAR_BORDER";
	case SAS_PLOT_OWNER_CAUSE_PEACE_BORDER: return "PEACE_BORDER";
	case SAS_PLOT_OWNER_CAUSE_WORLDBUILDER: return "WORLDBUILDER";
	case SAS_PLOT_OWNER_CAUSE_PYTHON_EXTERNAL: return "PYTHON_EXTERNAL";
	case SAS_PLOT_OWNER_CAUSE_NONE: break;
	}
	// <!-- custom: Root transaction identity is deliberately not reused as immediate mechanism provenance.
	// If no setter path proved a mechanism, say UNKNOWN so missing coverage stays visible instead of being masked by tx. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	return "UNKNOWN";
}

void logSASGameRecordPlotOwnerChanged(CvPlot const& kPlot, PlayerTypes eOldOwner, PlayerTypes eNewOwner, int iOwnershipDurationBefore, bool bOwnershipScoreBefore)
{
	FAssertMsg(g_eSASGameRecordPlotOwnerChangeCause != SAS_PLOT_OWNER_CAUSE_NONE, "Every current direct CvPlot::setOwner path should supply immediate SASGameRecord ownership provenance");
	TeamTypes const eOldTeam = (eOldOwner == NO_PLAYER ? NO_TEAM : GET_PLAYER(eOldOwner).getTeam());
	TeamTypes const eNewTeam = (eNewOwner == NO_PLAYER ? NO_TEAM : GET_PLAYER(eNewOwner).getTeam());
	int const iOldCulture = (eOldOwner == NO_PLAYER ? 0 : kPlot.getCulture(eOldOwner));
	int const iNewCulture = (eNewOwner == NO_PLAYER ? 0 : kPlot.getCulture(eNewOwner));
	int const iOldCulturePercent = (eOldOwner == NO_PLAYER ? 0 : kPlot.calculateCulturePercent(eOldOwner));
	int const iNewCulturePercent = (eNewOwner == NO_PLAYER ? 0 : kPlot.calculateCulturePercent(eNewOwner));
	PlayerTypes const eHighestCulturePlayer = kPlot.findHighestCulturePlayer();
	int const iHighestCulturePercent = (eHighestCulturePlayer == NO_PLAYER ? 0 : kPlot.calculateCulturePercent(eHighestCulturePlayer));
	// <!-- custom: Log only exact realized ownership transitions at the authoritative setter.
	// Culture pressure plus second-owner/strategic-tile context explains many flips without duplicating periodic political maps or every per-player plot-culture value. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_PLOT_OWNER_CHANGED turn=%d x=%d y=%d area=%d cause=%s oldOwner=%d oldTeam=%d newOwner=%d newTeam=%d secondOwner=%d water=%d ownershipDurationBefore=%d ownershipScoreBefore=%d forceUnownedTurns=%d oldOwnerCulture=%d oldOwnerCulturePercent=%d newOwnerCulture=%d newOwnerCulturePercent=%d highestCulturePlayer=%d highestCulturePercent=%d bonus=%s improvement=%s route=%s",
		GC.getGame().getGameTurn(), kPlot.getX(), kPlot.getY(), kPlot.getArea().getID(), getSASGameRecordPlotOwnerChangeCause(),
		eOldOwner, eOldTeam, eNewOwner, eNewTeam, kPlot.getSecondOwner(), kPlot.isWater(), iOwnershipDurationBefore, bOwnershipScoreBefore, kPlot.getForceUnownedTimer(),
		iOldCulture, iOldCulturePercent, iNewCulture, iNewCulturePercent, eHighestCulturePlayer, iHighestCulturePercent,
		getSASGameRecordBonusType(kPlot.getBonusType()), getSASGameRecordImprovementType(kPlot.getImprovementType()), getSASGameRecordRouteType(kPlot.getRouteType()));
}

SASGameRecordGoodyResult::SASGameRecordGoodyResult() :
	bFollowupOutcome(false), bUpgradeRoll(false), bUpgradeApplied(false), bAdditionalOutcomeAttempted(false),
	iGold(0), iNewlyRevealedPlots(0), iExperienceGained(0), iDamageHealed(0), eTech(NO_TECH), iTechRewardValue(0),
	iTechProgressBefore(-1), iTechProgressAfter(-1), iTechCost(-1), bTechCompleted(false), iFreePromotionsGranted(0)
{}

SASGameRecordPlotState::SASGameRecordPlotState(CvPlot const& kPlot) : eTerrain(kPlot.getTerrainType()), eFeature(kPlot.getFeatureType()), eBonus(kPlot.getBonusType()), eImprovement(kPlot.getImprovementType()), eRoute(kPlot.getRouteType())
{
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
		aiExtraYield[iI] = GC.getMap().getPlotExtraYield(kPlot, (YieldTypes)iI);
}

static bool isSASGameRecordPlotStateChanged(SASGameRecordPlotState const& kOldState, CvPlot const& kPlot)
{
	if (kOldState.eTerrain != kPlot.getTerrainType() || kOldState.eFeature != kPlot.getFeatureType() || kOldState.eBonus != kPlot.getBonusType() || kOldState.eImprovement != kPlot.getImprovementType() || kOldState.eRoute != kPlot.getRouteType())
		return true;
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		if (kOldState.aiExtraYield[iI] != GC.getMap().getPlotExtraYield(kPlot, (YieldTypes)iI))
			return true;
	}
	return false;
}

static void addSASGameRecordCoordinate(std::vector<std::pair<int,int> >& aCoordinates, CvPlot const& kPlot)
{
	std::pair<int,int> const kCoordinate(kPlot.getX(), kPlot.getY());
	if (std::find(aCoordinates.begin(), aCoordinates.end(), kCoordinate) == aCoordinates.end())
		aCoordinates.push_back(kCoordinate);
}

static void appendSASGameRecordCoordinateChunks(std::vector<CvString>& aszChunks, CvString& szChunk, char const* szCategory, std::vector<std::pair<int,int> > const& aCoordinates)
{
	for (size_t iI = 0; iI < aCoordinates.size(); iI++)
	{
		CvString szItem;
		if (iI == 0)
			szItem.Format("%s%s=(%d,%d)", szChunk.empty() ? "" : " ", szCategory, aCoordinates[iI].first, aCoordinates[iI].second);
		else szItem.Format(",(%d,%d)", aCoordinates[iI].first, aCoordinates[iI].second);
		if (!szChunk.empty() && szChunk.length() + szItem.length() > 1500)
		{
			aszChunks.push_back(szChunk);
			szChunk.clear();
			szItem.Format("%s=(%d,%d)", szCategory, aCoordinates[iI].first, aCoordinates[iI].second);
		}
		szChunk += szItem;
	}
}

static int getSASGameRecordRevealedPlotCount(TeamTypes eTeam)
{
	int iRevealed = 0;
	int iLoop = 0;
	for (CvArea const* pLoopArea = GC.getMap().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMap().nextArea(&iLoop))
		iRevealed += pLoopArea->getNumRevealedTiles(eTeam);
	return iRevealed;
}

void beginSASGameRecordFullMapRevelation(TeamTypes eTeam, TechTypes eTech)
{
	FAssert(g_eSASGameRecordFullMapRevelationTeam == NO_TEAM);
	FAssert(eTeam >= 0 && eTeam < MAX_CIV_TEAMS);
	FAssert(eTech != NO_TECH);
	g_eSASGameRecordFullMapRevelationTeam = eTeam;
	g_iSASGameRecordFullMapRevealedBefore = getSASGameRecordRevealedPlotCount(eTeam);
}

void endSASGameRecordFullMapRevelation(TeamTypes eTeam, TechTypes eTech)
{
	FAssert(g_eSASGameRecordFullMapRevelationTeam == eTeam);
	int const iRevealed = getSASGameRecordRevealedPlotCount(eTeam);
	int const iNewlyRevealed = iRevealed - g_iSASGameRecordFullMapRevealedBefore;
	int const iRevealedPctX100 = (10000 * iRevealed) / std::max(1, (int)GC.getMap().numPlots());
	logSASGameRecord("GAME_RECORD_MAP_REVELATION turn=%d team=%d cause=MAP_VISIBLE_TECH tech=%s revealMode=FULL_MAP newlyRevealedCount=%d revealedPlots=%d revealedPctX100=%d",
		GC.getGame().getGameTurn(), eTeam, getSASGameRecordTechType(eTech), iNewlyRevealed, iRevealed, iRevealedPctX100);
	g_eSASGameRecordFullMapRevelationTeam = NO_TEAM;
	g_iSASGameRecordFullMapRevealedBefore = 0;
}

void flushSASGameRecordTurnChanges(int iGameTurn)
{
	flushSASGameRecordPendingCityBombard();
	if (g_iSASGameRecordPendingPlotTurn < 0)
		return;
	FAssert(iGameTurn == g_iSASGameRecordPendingPlotTurn);
	int const iLoggedTurn = iGameTurn;
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
	{
		SASGameRecordWorkerBuildTurn& kWorkerBuild = g_akSASGameRecordWorkerBuildTurn[iPlayer];
		if (kWorkerBuild.iCompleted <= 0)
			continue;
		CvString szBuilds;
		for (size_t iI = 0; iI < kWorkerBuild.aiBuilds.size(); iI++)
		{
			if (kWorkerBuild.aiBuilds[iI] <= 0)
				continue;
			CvString szItem;
			szItem.Format(szBuilds.empty() ? "%s:%d" : ",%s:%d", GC.getInfo((BuildTypes)iI).getType(), kWorkerBuild.aiBuilds[iI]);
			szBuilds += szItem;
		}
		CvString szFeaturesRemoved;
		for (size_t iI = 0; iI < kWorkerBuild.aiFeaturesRemoved.size(); iI++)
		{
			if (kWorkerBuild.aiFeaturesRemoved[iI] <= 0)
				continue;
			CvString szItem;
			szItem.Format(szFeaturesRemoved.empty() ? "%s:%d" : ",%s:%d", getSASGameRecordFeatureType((FeatureTypes)iI), kWorkerBuild.aiFeaturesRemoved[iI]);
			szFeaturesRemoved += szItem;
		}
		CvString szImprovementTransitions;
		for (size_t iI = 0; iI < kWorkerBuild.aImprovementTransitions.size(); iI++)
		{
			SASGameRecordWorkerImprovementTransition const& kTransition = kWorkerBuild.aImprovementTransitions[iI];
			CvString szItem;
			szItem.Format(szImprovementTransitions.empty() ? "%s>%s:%d" : ",%s>%s:%d", getSASGameRecordImprovementType(kTransition.eOldImprovement), getSASGameRecordImprovementType(kTransition.eNewImprovement), kTransition.iCount);
			szImprovementTransitions += szItem;
		}
		logSASGameRecord("GAME_RECORD_WORKER_BUILDS_COMPLETED turn=%d player=%d completed=%d improvementChanges=%d newImprovements=%d improvementRemovals=%d improvementReplacements=%d workedImprovementReplacements=%d unworkedImprovementReplacements=%d directReversalsObserved=%d directReversalsWithin5TurnsObserved=%d routeChanges=%d featureRemovals=%d bfc=%d outsideBFC=%d bonus=%d bonusImprovementResults=%d irrigationCarrierResults=%d irrigatedCarrierResults=%d dryCarrierResults=%d irrigationCarrierReplacements=%d irrigationCarrierLosses=%d growthChainResults=%d growthChainReplacements=%d builds=%s improvementTransitions=%s featuresRemoved=%s",
			iLoggedTurn, iPlayer, kWorkerBuild.iCompleted, kWorkerBuild.iImprovementChanges, kWorkerBuild.iNewImprovements, kWorkerBuild.iImprovementRemovals,
			kWorkerBuild.iImprovementReplacements, kWorkerBuild.iWorkedImprovementReplacements, kWorkerBuild.iUnworkedImprovementReplacements,
			kWorkerBuild.iDirectReversalsObserved, kWorkerBuild.iDirectReversalsWithin5TurnsObserved,
			kWorkerBuild.iRouteChanges, kWorkerBuild.iFeatureRemovals,
			kWorkerBuild.iBFC, kWorkerBuild.iOutsideBFC, kWorkerBuild.iBonus, kWorkerBuild.iBonusImprovementResults,
			kWorkerBuild.iIrrigationCarrierResults, kWorkerBuild.iIrrigatedCarrierResults, kWorkerBuild.iDryCarrierResults,
			kWorkerBuild.iIrrigationCarrierReplacements, kWorkerBuild.iIrrigationCarrierLosses,
			kWorkerBuild.iGrowthChainResults, kWorkerBuild.iGrowthChainReplacements,
			getSASDiagnosticOrDash(szBuilds).GetCString(), getSASDiagnosticOrDash(szImprovementTransitions).GetCString(), getSASDiagnosticOrDash(szFeaturesRemoved).GetCString());
		kWorkerBuild.reset();
	}
	std::vector<CvString> aszPlotChunks;
	CvString szPlotChunk;
	for (size_t iI = 0; iI < g_aSASGameRecordPlotChanges.size(); iI++)
		appendSASGameRecordCoordinateChunks(aszPlotChunks, szPlotChunk, g_aSASGameRecordPlotChanges[iI].szCategory.GetCString(), g_aSASGameRecordPlotChanges[iI].aCoordinates);
	if (!szPlotChunk.empty())
		aszPlotChunks.push_back(szPlotChunk);
	for (size_t iI = 0; iI < aszPlotChunks.size(); iI++)
		logSASGameRecord("GAME_RECORD_PLOT_CHANGES turn=%d part=%d parts=%d changes=%s", iLoggedTurn, (int)iI + 1, (int)aszPlotChunks.size(), aszPlotChunks[iI].GetCString());
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		std::vector<std::pair<int,int> > const& aCoordinates = g_aaSASGameRecordRevealedPlots[iI];
		if (aCoordinates.empty())
			continue;
		std::vector<CvString> aszRevelationChunks;
		CvString szRevelationChunk;
		appendSASGameRecordCoordinateChunks(aszRevelationChunks, szRevelationChunk, "newlyRevealed", aCoordinates);
		if (!szRevelationChunk.empty())
			aszRevelationChunks.push_back(szRevelationChunk);
		// <!-- custom: CvMap::numPlots returns PlotNumTypes; casting it to int keeps MSVC 7.1 std::max template deduction unambiguous and fixed the compile error from adding map-revelation percentages. (GPT-5.6-Sol) -->
		int const iRevealedPctX100 = (10000 * getSASGameRecordRevealedPlotCount((TeamTypes)iI)) / std::max(1, (int)GC.getMap().numPlots());
		for (size_t iJ = 0; iJ < aszRevelationChunks.size(); iJ++)
			logSASGameRecord("GAME_RECORD_MAP_REVELATION turn=%d team=%d cause=INCREMENTAL revealMode=COORDINATES newlyRevealedCount=%d part=%d parts=%d revealedPctX100=%d %s",
				iLoggedTurn, iI, (int)aCoordinates.size(), (int)iJ + 1, (int)aszRevelationChunks.size(), iRevealedPctX100,
				aszRevelationChunks[iJ].GetCString());
	}
	g_iSASGameRecordPendingPlotTurn = -1;
	g_aSASGameRecordPlotChanges.clear();
	for (int iI = 0; iI < MAX_TEAMS; iI++)
		g_aaSASGameRecordRevealedPlots[iI].clear();
}

// <!-- custom: Session rollover previously reset pending city-bombard, plot-change and incremental-revelation observations without writing them.
// Flush while the old game/map still supply the matching turn and revelation totals, before CvGame::init or CvMap::read resets the corresponding state. See KI#382. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void finalizeSASGameRecordLogSession()
{
	// <!-- custom: If initialization aborts before gameStart, keep its setup actions even though authoritative context could not be completed. (ChatGPT-5.6-Sol) -->
	if (g_bSASGameRecordBufferInitializingActions)
	{
		g_bSASGameRecordBufferInitializingActions = false;
		flushSASGameRecordInitializingActions(false);
	}
	if (g_iSASGameRecordPendingPlotTurn >= 0)
		flushSASGameRecordTurnChanges(g_iSASGameRecordPendingPlotTurn);
	else flushSASGameRecordPendingCityBombard();
	// <!-- custom: A direct reset, new-game or load rollover can occur between ordinary end-turn checkpoints.
	// Preserve the last authoritative RNG state/consumption before the old session disappears, then disable the hot hook until the next session initializes its baseline. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (g_bSASGameRecordRngTrackingActive) logSASGameRecordRngCheckpoint(GC.getGame().getGameTurn(), SAS_RNG_CHECKPOINT_SESSION_FINALIZE);
	clearSASGameRecordRngTracking();
}

static void prepareSASGameRecordTurnChanges()
{
	int const iGameTurn = GC.getGame().getGameTurn();
	if (g_iSASGameRecordPendingPlotTurn >= 0 && g_iSASGameRecordPendingPlotTurn != iGameTurn)
		flushSASGameRecordTurnChanges(g_iSASGameRecordPendingPlotTurn);
	if (g_iSASGameRecordPendingPlotTurn < 0)
		g_iSASGameRecordPendingPlotTurn = iGameTurn;
}

static void bufferSASGameRecordPlotChangeCoordinate(CvPlot const& kPlot, char const* szCategory)
{
	prepareSASGameRecordTurnChanges();
	SASGameRecordPlotChangeGroup* pGroup = NULL;
	for (size_t iI = 0; iI < g_aSASGameRecordPlotChanges.size(); iI++)
	{
		if (g_aSASGameRecordPlotChanges[iI].szCategory == szCategory)
		{
			pGroup = &g_aSASGameRecordPlotChanges[iI];
			break;
		}
	}
	if (pGroup == NULL)
	{
		SASGameRecordPlotChangeGroup kGroup;
		kGroup.szCategory = szCategory;
		g_aSASGameRecordPlotChanges.push_back(kGroup);
		pGroup = &g_aSASGameRecordPlotChanges.back();
	}
	addSASGameRecordCoordinate(pGroup->aCoordinates, kPlot);
}

void recordSASGameRecordPlotChange(CvPlot const& kPlot, SASGameRecordPlotState const& kOldState, char const* szCategory, char const* szCause, bool bDetailed)
{
	if (GC.getGame().getElapsedGameTurns() <= 0 || !isSASGameRecordPlotStateChanged(kOldState, kPlot))
		return;
	bufferSASGameRecordPlotChangeCoordinate(kPlot, szCategory);
	if (bDetailed)
	{
		logSASGameRecord("GAME_RECORD_PLOT_CHANGE turn=%d cause=%s category=%s x=%d y=%d owner=%d terrainOld=%s terrainNew=%s featureOld=%s featureNew=%s bonusOld=%s bonusNew=%s improvementOld=%s improvementNew=%s routeOld=%s routeNew=%s extraFoodOld=%d extraFoodNew=%d extraProductionOld=%d extraProductionNew=%d extraCommerceOld=%d extraCommerceNew=%d",
				GC.getGame().getGameTurn(), szCause, szCategory, kPlot.getX(), kPlot.getY(), kPlot.getOwner(),
				getSASGameRecordTerrainType(kOldState.eTerrain), getSASGameRecordTerrainType(kPlot.getTerrainType()),
				getSASGameRecordFeatureType(kOldState.eFeature), getSASGameRecordFeatureType(kPlot.getFeatureType()),
				getSASGameRecordBonusType(kOldState.eBonus), getSASGameRecordBonusType(kPlot.getBonusType()),
				getSASGameRecordImprovementType(kOldState.eImprovement), getSASGameRecordImprovementType(kPlot.getImprovementType()),
				getSASGameRecordRouteType(kOldState.eRoute), getSASGameRecordRouteType(kPlot.getRouteType()),
				kOldState.aiExtraYield[YIELD_FOOD], GC.getMap().getPlotExtraYield(kPlot, YIELD_FOOD),
				kOldState.aiExtraYield[YIELD_PRODUCTION], GC.getMap().getPlotExtraYield(kPlot, YIELD_PRODUCTION),
				kOldState.aiExtraYield[YIELD_COMMERCE], GC.getMap().getPlotExtraYield(kPlot, YIELD_COMMERCE));
	}
}

// <!-- custom: Identify every member of an XML ImprovementUpgrade chain, including mature endpoints such as Town, so recorder summaries use the same generic concept as Worker replacement protection without naming one chain.
// Cache after XML load because completed Worker Builds can be frequent. (GPT-5.6-Sol) -->
static bool isSASGameRecordGrowthChainImprovement(ImprovementTypes eImprovement)
{
	if (eImprovement == NO_IMPROVEMENT)
		return false;
	static std::vector<bool> abGrowthChain;
	if (abGrowthChain.empty())
	{
		int const iNumImprovements = GC.getNumImprovementInfos();
		abGrowthChain.resize(iNumImprovements, false);
		for (int iStart = 0; iStart < iNumImprovements; iStart++)
		{
			ImprovementTypes eLoopImprovement = (ImprovementTypes)iStart;
			if (GC.getInfo(eLoopImprovement).getImprovementUpgrade() == NO_IMPROVEMENT)
				continue;
			std::vector<bool> abVisited(iNumImprovements, false);
			while (eLoopImprovement != NO_IMPROVEMENT && !abVisited[eLoopImprovement])
			{
				abVisited[eLoopImprovement] = true;
				abGrowthChain[eLoopImprovement] = true;
				eLoopImprovement = GC.getInfo(eLoopImprovement).getImprovementUpgrade();
			}
		}
	}
	return abGrowthChain[eImprovement];
}

void recordSASGameRecordWorkerBuild(CvPlot const& kPlot, SASGameRecordPlotState const& kOldState, PlayerTypes ePlayer, BuildTypes eBuild)
{
	if (GC.getGame().getElapsedGameTurns() <= 0 || !isSASGameRecordPlotStateChanged(kOldState, kPlot))
		return;
	recordSASGameRecordPlotChange(kPlot, kOldState, "workerBuilds", "WORKER_BUILD", false);
	FAssertBounds(0, MAX_PLAYERS, ePlayer);
	FAssertBounds(0, GC.getNumBuildInfos(), eBuild);
	prepareSASGameRecordTurnChanges();
	SASGameRecordWorkerBuildTurn& kWorkerBuild = g_akSASGameRecordWorkerBuildTurn[ePlayer];
	if ((int)kWorkerBuild.aiBuilds.size() != GC.getNumBuildInfos())
		kWorkerBuild.aiBuilds.resize(GC.getNumBuildInfos(), 0);
	if ((int)kWorkerBuild.aiFeaturesRemoved.size() != GC.getNumFeatureInfos())
		kWorkerBuild.aiFeaturesRemoved.resize(GC.getNumFeatureInfos(), 0);
	kWorkerBuild.iCompleted++;
	kWorkerBuild.aiBuilds[eBuild]++;
	CvCity const* pWorkingCity = kPlot.getWorkingCity();
	bool const bOwnedBFC = (pWorkingCity != NULL && pWorkingCity->getOwner() == ePlayer);
	if (bOwnedBFC)
		kWorkerBuild.iBFC++;
	else kWorkerBuild.iOutsideBFC++;
	BonusTypes const eBonus = kPlot.getBonusType();
	if (eBonus != NO_BONUS)
		kWorkerBuild.iBonus++;
	ImprovementTypes const eNewImprovement = kPlot.getImprovementType();
	if (kOldState.eImprovement != eNewImprovement)
	{
		kWorkerBuild.iImprovementChanges++;
		if (kOldState.eImprovement == NO_IMPROVEMENT)
			kWorkerBuild.iNewImprovements++;
		else if (eNewImprovement == NO_IMPROVEMENT)
			kWorkerBuild.iImprovementRemovals++;
		else
		{
			kWorkerBuild.iImprovementReplacements++;
			if (bOwnedBFC && pWorkingCity->isWorkingPlot(kPlot))
				kWorkerBuild.iWorkedImprovementReplacements++;
			else kWorkerBuild.iUnworkedImprovementReplacements++;
			// <!-- custom: Recent Worker-AI validation compared completed overwrites and short direct reversals, not merely changing candidate assignments.
			// Retain those compact factual outcomes here; exact plots and decision paths remain in BBAI. (GPT-5.6-Sol) -->
			std::pair<int,int> const kReplacementKey(ePlayer, kPlot.plotNum());
			std::map<std::pair<int,int>, SASGameRecordWorkerCompletedReplacement>::iterator itPrevious = g_mapSASGameRecordWorkerCompletedReplacements.find(kReplacementKey);
			if (itPrevious != g_mapSASGameRecordWorkerCompletedReplacements.end() && itPrevious->second.eOldImprovement == eNewImprovement && itPrevious->second.eNewImprovement == kOldState.eImprovement)
			{
				kWorkerBuild.iDirectReversalsObserved++;
				if (GC.getGame().getGameTurn() - itPrevious->second.iTurn <= 5)
					kWorkerBuild.iDirectReversalsWithin5TurnsObserved++;
			}
			SASGameRecordWorkerCompletedReplacement& kCompletedReplacement = g_mapSASGameRecordWorkerCompletedReplacements[kReplacementKey];
			kCompletedReplacement.iTurn = GC.getGame().getGameTurn();
			kCompletedReplacement.eOldImprovement = kOldState.eImprovement;
			kCompletedReplacement.eNewImprovement = eNewImprovement;
		}
		bool bFoundTransition = false;
		for (size_t iI = 0; iI < kWorkerBuild.aImprovementTransitions.size(); iI++)
		{
			SASGameRecordWorkerImprovementTransition& kTransition = kWorkerBuild.aImprovementTransitions[iI];
			if (kTransition.eOldImprovement == kOldState.eImprovement && kTransition.eNewImprovement == eNewImprovement)
			{
				kTransition.iCount++;
				bFoundTransition = true;
				break;
			}
		}
		if (!bFoundTransition)
			kWorkerBuild.aImprovementTransitions.push_back(SASGameRecordWorkerImprovementTransition(kOldState.eImprovement, eNewImprovement));
	}
	if (kOldState.eRoute != kPlot.getRouteType())
		kWorkerBuild.iRouteChanges++;
	if (kOldState.eFeature != NO_FEATURE && kOldState.eFeature != kPlot.getFeatureType())
	{
		kWorkerBuild.iFeatureRemovals++;
		kWorkerBuild.aiFeaturesRemoved[kOldState.eFeature]++;
	}
	bool const bOldCarrier = (kOldState.eImprovement != NO_IMPROVEMENT && GC.getInfo(kOldState.eImprovement).isCarriesIrrigation());
	bool const bNewCarrier = (eNewImprovement != NO_IMPROVEMENT && GC.getInfo(eNewImprovement).isCarriesIrrigation());
	bool const bImprovementChanged = (kOldState.eImprovement != eNewImprovement);
	if (bImprovementChanged && bNewCarrier)
	{
		kWorkerBuild.iIrrigationCarrierResults++;
		if (kPlot.isIrrigated())
			kWorkerBuild.iIrrigatedCarrierResults++;
		else kWorkerBuild.iDryCarrierResults++;
	}
	if (bOldCarrier && bImprovementChanged)
	{
		if (bNewCarrier)
			kWorkerBuild.iIrrigationCarrierReplacements++;
		else kWorkerBuild.iIrrigationCarrierLosses++;
	}
	bool const bOldGrowthChain = isSASGameRecordGrowthChainImprovement(kOldState.eImprovement);
	bool const bNewGrowthChain = isSASGameRecordGrowthChainImprovement(eNewImprovement);
	if (bImprovementChanged && bNewGrowthChain)
		kWorkerBuild.iGrowthChainResults++;
	if (bOldGrowthChain && bImprovementChanged)
		kWorkerBuild.iGrowthChainReplacements++;
	if (bImprovementChanged && eBonus != NO_BONUS && eNewImprovement != NO_IMPROVEMENT && GET_PLAYER(ePlayer).doesImprovementConnectBonus(eNewImprovement, eBonus))
		kWorkerBuild.iBonusImprovementResults++;
}

void logSASGameRecordRiverEdgeChanged(CvPlot const& kPlot, bool bOldSouthBoundary, bool bOldEastBoundary)
{
	bool const bNewSouthBoundary = kPlot.isNOfRiver();
	bool const bNewEastBoundary = kPlot.isWOfRiver();
	if (bOldSouthBoundary == bNewSouthBoundary && bOldEastBoundary == bNewEastBoundary) return;
	bufferSASGameRecordPlotChangeCoordinate(kPlot, "riverChanges");
	logSASGameRecord("GAME_RECORD_RIVER_EDGE_CHANGE turn=%d x=%d y=%d owner=%d southBoundaryOld=%d southBoundaryNew=%d eastBoundaryOld=%d eastBoundaryNew=%d",
			GC.getGame().getGameTurn(), kPlot.getX(), kPlot.getY(), kPlot.getOwner(), bOldSouthBoundary, bNewSouthBoundary, bOldEastBoundary, bNewEastBoundary);
}

void recordSASGameRecordPlotRevealed(CvPlot const& kPlot, TeamTypes eTeam)
{
	if (GC.getGame().getElapsedGameTurns() <= 0 || eTeam < 0 || eTeam >= MAX_CIV_TEAMS)
		return;
	if (eTeam == g_eSASGameRecordFullMapRevelationTeam)
		return;
	prepareSASGameRecordTurnChanges();
	// <!-- custom: setRevealed calls this only on false-to-true transitions, so the same team cannot add this plot twice without first losing permanent revelation; append directly instead of repeatedly searching a potentially large map-trade list. (GPT-5.6-Sol) -->
	g_aaSASGameRecordRevealedPlots[eTeam].push_back(std::make_pair(kPlot.getX(), kPlot.getY()));
}

void logSASGameRecordEnvironmentTurn(int iPollution, int iSustainabilityThreshold, int iLandDefense, int iIndexBefore, int iIndexBeforeRestoration, int iIndexEnd, int iWarmingChances, int iEventTally)
{
	// <!-- custom: Loaded-save context is written before graphics initialization; retry the one-shot display context at this already-level-2 per-turn boundary so short manual/autoplay sessions do not omit resolution/options/GPU vendor merely because they never reach a full snapshot interval. (ChatGPT-5.6-Sol) -->
	logSASGameRecordDisplayContext();
	logSASGameRecord("GAME_RECORD_ENVIRONMENT_TURN turn=%d pollution=%d sustainabilityThreshold=%d landDefense=%d totalDefense=%d indexBefore=%d indexBeforeRestoration=%d indexEnd=%d indexDelta=%+d warmingChances=%d eventTally=%d severityPercent=%d active=%d",
		GC.getGame().getGameTurn(), iPollution, iSustainabilityThreshold, iLandDefense, iSustainabilityThreshold + iLandDefense,
		iIndexBefore, iIndexBeforeRestoration, iIndexEnd, iIndexEnd - iIndexBefore, iWarmingChances, iEventTally,
		GC.getGame().calculateGwSeverityRating(), iIndexEnd > 0);
}

void logSASGameRecordBonusChanged(CvPlot const* pPlot, BonusTypes eOldBonus, BonusTypes eNewBonus)
{
	if (pPlot == NULL || eOldBonus == eNewBonus)
		return;
	SASGameRecordPlotState kOldState(*pPlot);
	kOldState.eBonus = eOldBonus;
	recordSASGameRecordPlotChange(*pPlot, kOldState, "resourceChanges", "RESOURCE_CHANGE", false);
	const char* szAction = (eOldBonus == NO_BONUS ? "appeared" : (eNewBonus == NO_BONUS ? "disappeared" : "changed"));
	CvCity const* pWorkingCity = pPlot->getWorkingCity();
	CvCity const* pPlotCity = pPlot->getPlotCity();
	// <!-- custom: Reproducible T129 crash dumps after adding this row failed in msvcr71!_output/_vsnprintf with an invalid read at 0x000003fc.
	// The original argument for area=%d was pPlot->getArea(), but CvPlot::getArea returns CvArea&, not an integer; passing that object reference through varargs corrupted the following formatter reads.
	// Logging the area ID explicitly fixed the crash in the next test run. (GPT-5.5) -->
	logSASGameRecord("GAME_RECORD_BONUS_CHANGE turn=%d elapsed=%d action=%s x=%d y=%d area=%d owner=%d oldBonus=%s newBonus=%s terrain=%s feature=%s improvement=%s route=%s water=%d hills=%d peak=%d riverSide=%d cityRadius=%d workingCity=%S workingCityId=%d plotCity=%S plotCityId=%d",
		GC.getGame().getGameTurn(), GC.getGame().getElapsedGameTurns(), szAction, pPlot->getX(), pPlot->getY(), pPlot->getArea().getID(),
		pPlot->getOwner(), getSASGameRecordBonusType(eOldBonus), getSASGameRecordBonusType(eNewBonus),
		getSASGameRecordTerrainType(pPlot->getTerrainType()), getSASGameRecordFeatureType(pPlot->getFeatureType()),
		getSASGameRecordImprovementType(pPlot->getImprovementType()), getSASGameRecordRouteType(pPlot->getRouteType()), pPlot->isWater(),
		pPlot->isHills(), pPlot->isPeak(), pPlot->isRiverSide(), pPlot->isCityRadius(),
		getSASGameRecordQuotedCityName(pWorkingCity).GetCString(), (pWorkingCity == NULL ? -1 : pWorkingCity->getID()),
		getSASGameRecordQuotedCityName(pPlotCity).GetCString(), (pPlotCity == NULL ? -1 : pPlotCity->getID()));
}

static const char* getSASGameRecordCommerceType(CommerceTypes eCommerce)
{
	return (eCommerce == NO_COMMERCE ? "-" : GC.getInfo(eCommerce).getType());
}

static const char* getSASGameRecordBuildType(BuildTypes eBuild)
{
	return (eBuild == NO_BUILD ? "-" : GC.getInfo(eBuild).getType());
}

static const char* getSASGameRecordMissionType(MissionTypes eMission)
{
	return (eMission == NO_MISSION ? "-" : GC.getInfo(eMission).getType());
}

static const char* getSASGameRecordEspionageMissionType(EspionageMissionTypes eMission)
{
	return (eMission == NO_ESPIONAGEMISSION ? "-" : GC.getInfo(eMission).getType());
}

static const char* getSASGameRecordUnitAIType(UnitAITypes eUnitAI)
{
	return (eUnitAI == NO_UNITAI ? "-" : GC.getInfo(eUnitAI).getType());
}

static const char* getSASGameRecordUnitCombatType(UnitCombatTypes eUnitCombat)
{
	return (eUnitCombat == NO_UNITCOMBAT ? "-" : GC.getInfo(eUnitCombat).getType());
}

static const char* getSASGameRecordPromotionType(PromotionTypes ePromotion)
{
	return (ePromotion == NO_PROMOTION ? "-" : GC.getInfo(ePromotion).getType());
}

static const char* getSASGameRecordSpecialistType(SpecialistTypes eSpecialist)
{
	return (eSpecialist == NO_SPECIALIST ? "-" : GC.getInfo(eSpecialist).getType());
}

static const char* getSASGameRecordProcessType(ProcessTypes eProcess)
{
	return (eProcess == NO_PROCESS ? "-" : GC.getInfo(eProcess).getType());
}

static const char* getSASGameRecordCivicType(CivicTypes eCivic)
{
	return (eCivic == NO_CIVIC ? "-" : GC.getInfo(eCivic).getType());
}

static const char* getSASGameRecordVoteSourceType(VoteSourceTypes eVoteSource)
{
	return (eVoteSource == NO_VOTESOURCE ? "-" : GC.getInfo(eVoteSource).getType());
}

static const char* getSASGameRecordVoteType(VoteTypes eVote)
{
	return (eVote == NO_VOTE ? "-" : GC.getInfo(eVote).getType());
}

static const char* getSASGameRecordEraType(EraTypes eEra)
{
	return (eEra == NO_ERA ? "-" : GC.getInfo(eEra).getType());
}

static void appendSASGameRecordTypeCount(CvString& szList, const char* szType, int iCount)
{
	if (iCount <= 0)
		return;
	CvString szItem;
	szItem.Format(szList.empty() ? "%s:%d" : ",%s:%d", szType, iCount);
	szList += szItem;
}

static void addSASGameRecordCityBombardTypeCount(std::vector<std::pair<CvString,int> >& aCounts, char const* szType)
{
	for (size_t iI = 0; iI < aCounts.size(); iI++)
	{
		if (aCounts[iI].first == szType)
		{
			aCounts[iI].second++;
			return;
		}
	}
	aCounts.push_back(std::make_pair(CvString(szType), 1));
}

static void flushSASGameRecordPendingCityBombard()
{
	if (!g_kSASGameRecordPendingCityBombard.bValid)
		return;
	CvString szUnitTypes;
	CvString szUnitAIs;
	for (size_t iI = 0; iI < g_kSASGameRecordPendingCityBombard.aUnitTypes.size(); iI++)
		appendSASGameRecordTypeCount(szUnitTypes, g_kSASGameRecordPendingCityBombard.aUnitTypes[iI].first.GetCString(), g_kSASGameRecordPendingCityBombard.aUnitTypes[iI].second);
	for (size_t iI = 0; iI < g_kSASGameRecordPendingCityBombard.aUnitAIs.size(); iI++)
		appendSASGameRecordTypeCount(szUnitAIs, g_kSASGameRecordPendingCityBombard.aUnitAIs[iI].first.GetCString(), g_kSASGameRecordPendingCityBombard.aUnitAIs[iI].second);
	if (szUnitTypes.empty()) szUnitTypes = "-";
	if (szUnitAIs.empty()) szUnitAIs = "-";
	g_bSASGameRecordFlushingCityBombard = true;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=CITY_BOMBARD mode=%s player=%d targetPlayer=%d cityId=%d city=%S x=%d y=%d actions=%d unitTypes=%s unitAI=%s bombardRateTotal=%d ignoreBuildingDefenseActions=%d defenseModifierBefore=%d defenseModifierAfter=%d defenseReduction=%d totalDefense=%d defenseDamageBefore=%d defenseDamageAfter=%d defenseDamageMax=%d",
		g_kSASGameRecordPendingCityBombard.iTurn, g_kSASGameRecordPendingCityBombard.szMode.GetCString(),
		g_kSASGameRecordPendingCityBombard.ePlayer, g_kSASGameRecordPendingCityBombard.eTargetPlayer,
		g_kSASGameRecordPendingCityBombard.iCityId, g_kSASGameRecordPendingCityBombard.szCity.GetCString(),
		g_kSASGameRecordPendingCityBombard.iX, g_kSASGameRecordPendingCityBombard.iY, g_kSASGameRecordPendingCityBombard.iActions,
		szUnitTypes.GetCString(), szUnitAIs.GetCString(), g_kSASGameRecordPendingCityBombard.iBombardRateTotal,
		g_kSASGameRecordPendingCityBombard.iIgnoreBuildingDefenseActions, g_kSASGameRecordPendingCityBombard.iDefenseModifierBefore,
		g_kSASGameRecordPendingCityBombard.iDefenseModifierAfter,
		std::max(0, g_kSASGameRecordPendingCityBombard.iDefenseModifierBefore - g_kSASGameRecordPendingCityBombard.iDefenseModifierAfter),
		g_kSASGameRecordPendingCityBombard.iTotalDefense, g_kSASGameRecordPendingCityBombard.iDefenseDamageBefore,
		g_kSASGameRecordPendingCityBombard.iDefenseDamageAfter, g_kSASGameRecordPendingCityBombard.iDefenseDamageMax);
	g_bSASGameRecordFlushingCityBombard = false;
	g_kSASGameRecordPendingCityBombard = SASGameRecordCityBombardPending();
}

static void appendSASGameRecordPositiveValue(CvString& szList, const char* szName, int iValue)
{
	if (iValue <= 0)
		return;
	CvString szItem;
	szItem.Format(szList.empty() ? "%s:%d" : ",%s:%d", szName, iValue);
	szList += szItem;
}

static void appendSASGameRecordValue(CvString& szList, const char* szName, int iValue)
{
	CvString szItem;
	szItem.Format(szList.empty() ? "%s:%d" : ",%s:%d", szName, iValue);
	szList += szItem;
}

static void appendSASGameRecordSignedValue(CvString& szList, const char* szName, int iValue)
{
	if (iValue == 0)
		return;
	CvString szItem;
	szItem.Format(szList.empty() ? "%s:%+d" : ",%s:%+d", szName, iValue);
	szList += szItem;
}

static void addSASGameRecordPlotComposition(SASGameRecordPlotComposition& kComposition, CvPlot const& kPlot, TeamTypes eTeam)
{
	kComposition.iPlots++;
	if (kPlot.isWater())
		kComposition.iWater++;
	else kComposition.iLand++;
	if (kPlot.isHills())
		kComposition.iHills++;
	if (kPlot.isPeak())
		kComposition.iPeaks++;
	if (kPlot.isRiverSide())
		kComposition.iRiverSide++;
	if (kPlot.isFreshWater())
		kComposition.iFreshWater++;
	if (kPlot.isCoastalLand())
		kComposition.iCoastal++;
	if (kPlot.getTerrainType() != NO_TERRAIN)
		kComposition.aiTerrains[kPlot.getTerrainType()]++;
	if (kPlot.getFeatureType() != NO_FEATURE)
		kComposition.aiFeatures[kPlot.getFeatureType()]++;
	ImprovementTypes eImprovement = kPlot.getImprovementType();
	if (eImprovement != NO_IMPROVEMENT)
	{
		kComposition.iImproved++;
		kComposition.aiImprovements[eImprovement]++;
	}
	else if (!kPlot.isWater())
	{
		kComposition.iUnimprovedLand++;
	}
	RouteTypes eRoute = kPlot.getRouteType();
	if (eRoute != NO_ROUTE)
	{
		kComposition.iRoaded++;
		kComposition.aiRoutes[eRoute]++;
	}
	BonusTypes eBonus = kPlot.getBonusType(eTeam);
	if (eBonus != NO_BONUS)
	{
		kComposition.aiBonuses[eBonus]++;
		if (eImprovement != NO_IMPROVEMENT)
			kComposition.iBonusImproved++;
		else kComposition.iBonusUnimproved++;
	}
	if (kPlot.isBeingWorked())
	{
		kComposition.iWorked++;
		if (eImprovement != NO_IMPROVEMENT)
			kComposition.iWorkedImproved++;
		else kComposition.iWorkedUnimproved++;
	}
	kComposition.iNatureFood += kPlot.calculateBestNatureYield(YIELD_FOOD, eTeam);
	kComposition.iNatureProduction += kPlot.calculateBestNatureYield(YIELD_PRODUCTION, eTeam);
	kComposition.iNatureCommerce += kPlot.calculateBestNatureYield(YIELD_COMMERCE, eTeam);
	kComposition.iCurrentFood += kPlot.calculateYield(YIELD_FOOD);
	kComposition.iCurrentProduction += kPlot.calculateYield(YIELD_PRODUCTION);
	kComposition.iCurrentCommerce += kPlot.calculateYield(YIELD_COMMERCE);
}

static int getSASGameRecordPercentX100(int iValue, int iTotal)
{
	return (iTotal <= 0 ? -1 : (10000 * iValue) / iTotal);
}

// <!-- custom: Add lightweight owned-territory counts to the map scan already used by the expansion record, rather than scanning every plot again or calculating unused plot yields.
// BFC means the plot is assigned to one of this player's cities; development land excludes city centers and peaks because Workers cannot add ordinary improvements there.
// Split routes on that same Worker-developable land between BFC and suburb plots, exposing whether route growth supports cities or consumes work outside them without diluting percentages with water, peaks or city centers.
// Count every XML bCarriesIrrigation improvement rather than hardcoding Farm, matching the Worker evaluator and preserving mod-added food/hammer/commerce carriers. (GPT-5.6-Sol) -->
static void addSASGameRecordTerritoryDevelopment(SASGameRecordTerritoryDevelopment& kDevelopment, CvPlot const& kPlot, PlayerTypes ePlayer, TeamTypes eTeam)
{
	SASGameRecordPlotComposition& kOwned = kDevelopment.kOwned;
	kOwned.iPlots++;
	if (kPlot.isWater())
		kOwned.iWater++;
	else kOwned.iLand++;
	if (kPlot.getTerrainType() != NO_TERRAIN)
		kOwned.aiTerrains[kPlot.getTerrainType()]++;
	if (kPlot.getFeatureType() != NO_FEATURE)
		kOwned.aiFeatures[kPlot.getFeatureType()]++;
	CvCity const* pWorkingCity = kPlot.getWorkingCity();
	bool const bBFC = (pWorkingCity != NULL && pWorkingCity->getOwner() == ePlayer);
	if (bBFC)
		kDevelopment.iBFCPlots++;
	else kDevelopment.iSuburbPlots++;
	ImprovementTypes const eImprovement = kPlot.getImprovementType();
	bool const bImproved = (eImprovement != NO_IMPROVEMENT);
	if (bImproved)
	{
		kOwned.iImproved++;
		kOwned.aiImprovements[eImprovement]++;
	}
	RouteTypes const eRoute = kPlot.getRouteType();
	if (eRoute != NO_ROUTE)
	{
		kOwned.iRoaded++;
		kOwned.aiRoutes[eRoute]++;
	}
	BonusTypes const eBonus = kPlot.getBonusType(eTeam);
	if (eBonus != NO_BONUS)
	{
		kOwned.aiBonuses[eBonus]++;
		if (bImproved)
		{
			kOwned.iBonusImproved++;
			kDevelopment.aiImprovedBonuses[eBonus]++;
		}
		else
		{
			kOwned.iBonusUnimproved++;
			kDevelopment.aiUnimprovedBonuses[eBonus]++;
		}
	}
	bool const bDevelopmentLand = (!kPlot.isWater() && !kPlot.isPeak() && !kPlot.isCity());
	if (bDevelopmentLand)
	{
		kDevelopment.iDevelopmentLand++;
		if (bImproved)
			kDevelopment.iImprovedLand++;
		if (bBFC)
		{
			kDevelopment.iBFCDevelopmentLand++;
			if (bImproved)
				kDevelopment.iBFCImprovedLand++;
			if (eRoute != NO_ROUTE)
			{
				kDevelopment.iBFCDevelopmentLandRoaded++;
				kDevelopment.aiBFCDevelopmentRoutes[eRoute]++;
			}
		}
		else
		{
			kDevelopment.iSuburbDevelopmentLand++;
			if (bImproved)
				kDevelopment.iSuburbImprovedLand++;
			if (eRoute != NO_ROUTE)
			{
				kDevelopment.iSuburbDevelopmentLandRoaded++;
				kDevelopment.aiSuburbDevelopmentRoutes[eRoute]++;
			}
		}
	}
	else if (kPlot.isWater() && (eBonus != NO_BONUS || bImproved))
	{
		// <!-- custom: Ordinary water cannot receive an improvement. Count only visible bonus water or an already improved water plot in the
		// development denominator, so seafood coverage is not diluted by unusable ocean. (GPT-5.6-Sol) -->
		kDevelopment.iDevelopmentWater++;
		if (bImproved)
			kDevelopment.iImprovedWater++;
		if (bBFC)
		{
			kDevelopment.iBFCDevelopmentWater++;
			if (bImproved)
				kDevelopment.iBFCImprovedWater++;
		}
		else
		{
			kDevelopment.iSuburbDevelopmentWater++;
			if (bImproved)
				kDevelopment.iSuburbImprovedWater++;
		}
	}
	if (!bImproved || !GC.getInfo(eImprovement).isCarriesIrrigation())
		return;
	kDevelopment.iIrrigationCarriers++;
	bool const bIrrigated = kPlot.isIrrigated();
	if (bIrrigated)
	{
		kDevelopment.iIrrigatedCarriers++;
		kDevelopment.aiIrrigatedCarriers[eImprovement]++;
	}
	else
	{
		kDevelopment.iDryCarriers++;
		kDevelopment.aiDryCarriers[eImprovement]++;
	}
	if (eBonus != NO_BONUS)
	{
		kDevelopment.iBonusIrrigationCarriers++;
		if (bIrrigated)
			kDevelopment.iIrrigatedBonusCarriers++;
		else kDevelopment.iDryBonusCarriers++;
	}
	if (bBFC)
	{
		kDevelopment.iBFCIrrigationCarriers++;
		if (bIrrigated)
			kDevelopment.iBFCIrrigatedCarriers++;
		else kDevelopment.iBFCDryCarriers++;
	}
}

static void getSASGameRecordImprovementRouteTypes(SASGameRecordPlotComposition const& kComposition, CvString& szImprovements, CvString& szRoutes)
{
	for (int iI = 0; iI < GC.getNumImprovementInfos(); iI++)
		appendSASGameRecordTypeCount(szImprovements, getSASGameRecordImprovementType((ImprovementTypes)iI), kComposition.aiImprovements[iI]);
	for (int iI = 0; iI < GC.getNumRouteInfos(); iI++)
		appendSASGameRecordTypeCount(szRoutes, getSASGameRecordRouteType((RouteTypes)iI), kComposition.aiRoutes[iI]);
}

static void getSASGameRecordLandscapeTypes(SASGameRecordPlotComposition const& kComposition, CvString& szTerrains, CvString& szFeatures, CvString& szBonuses)
{
	for (int iI = 0; iI < GC.getNumTerrainInfos(); iI++)
		appendSASGameRecordTypeCount(szTerrains, getSASGameRecordTerrainType((TerrainTypes)iI), kComposition.aiTerrains[iI]);
	for (int iI = 0; iI < GC.getNumFeatureInfos(); iI++)
		appendSASGameRecordTypeCount(szFeatures, getSASGameRecordFeatureType((FeatureTypes)iI), kComposition.aiFeatures[iI]);
	for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
		appendSASGameRecordTypeCount(szBonuses, getSASGameRecordBonusType((BonusTypes)iI), kComposition.aiBonuses[iI]);
}

static void logSASGameRecordTerritoryDevelopment(PlayerTypes ePlayer, int iGameTurn, SASGameRecordTerritoryDevelopment const& kDevelopment)
{
	SASGameRecordPlotComposition const& kOwned = kDevelopment.kOwned;
	SASGameRecordPlayerPrevious& kPrevious = g_akSASGameRecordPlayerPrevious[ePlayer];
	CvString szImprovements;
	CvString szRoutes;
	CvString szIrrigatedCarrierTypes;
	CvString szDryCarrierTypes;
	CvString szBFCDevelopmentRoutes;
	CvString szSuburbDevelopmentRoutes;
	getSASGameRecordImprovementRouteTypes(kOwned, szImprovements, szRoutes);
	for (int iI = 0; iI < GC.getNumImprovementInfos(); iI++)
	{
		appendSASGameRecordTypeCount(szIrrigatedCarrierTypes, getSASGameRecordImprovementType((ImprovementTypes)iI), kDevelopment.aiIrrigatedCarriers[iI]);
		appendSASGameRecordTypeCount(szDryCarrierTypes, getSASGameRecordImprovementType((ImprovementTypes)iI), kDevelopment.aiDryCarriers[iI]);
	}
	for (int iI = 0; iI < GC.getNumRouteInfos(); iI++)
	{
		appendSASGameRecordTypeCount(szBFCDevelopmentRoutes, getSASGameRecordRouteType((RouteTypes)iI), kDevelopment.aiBFCDevelopmentRoutes[iI]);
		appendSASGameRecordTypeCount(szSuburbDevelopmentRoutes, getSASGameRecordRouteType((RouteTypes)iI), kDevelopment.aiSuburbDevelopmentRoutes[iI]);
	}
	int const iDevelopmentPlots = kDevelopment.iDevelopmentLand + kDevelopment.iDevelopmentWater;
	int const iImprovedPlots = kDevelopment.iImprovedLand + kDevelopment.iImprovedWater;
	int const iUnimprovedPlots = iDevelopmentPlots - iImprovedPlots;
	int const iUnimprovedLand = kDevelopment.iDevelopmentLand - kDevelopment.iImprovedLand;
	int const iUnimprovedWater = kDevelopment.iDevelopmentWater - kDevelopment.iImprovedWater;
	int const iBFCUnimprovedLand = kDevelopment.iBFCDevelopmentLand - kDevelopment.iBFCImprovedLand;
	int const iBFCUnimprovedWater = kDevelopment.iBFCDevelopmentWater - kDevelopment.iBFCImprovedWater;
	int const iSuburbUnimprovedLand = kDevelopment.iSuburbDevelopmentLand - kDevelopment.iSuburbImprovedLand;
	int const iSuburbUnimprovedWater = kDevelopment.iSuburbDevelopmentWater - kDevelopment.iSuburbImprovedWater;
	int const iSuburbIrrigationCarriers = kDevelopment.iIrrigationCarriers - kDevelopment.iBFCIrrigationCarriers;
	int const iSuburbIrrigatedCarriers = kDevelopment.iIrrigatedCarriers - kDevelopment.iBFCIrrigatedCarriers;
	int const iSuburbDryCarriers = kDevelopment.iDryCarriers - kDevelopment.iBFCDryCarriers;
	// <!-- custom: Emit the cheap subtraction-derived backlog counts directly instead of requiring every human/LLM/parser to reconstruct them repeatedly.
	// These remain descriptive state, not Worker-AI legality/value judgments.
	// BFC/suburb water uses the same sparse seafood/actually-improved denominator as total development water. (ChatGPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_TERRITORY_DEVELOPMENT turn=%d player=%d deltaValid=%d ownedPlots=%d ownedLand=%d ownedWater=%d bfcPlots=%d suburbPlots=%d developmentPlots=%d improvedPlots=%d unimprovedPlots=%d improvedPercentX100=%d developmentLand=%d improvedLand=%d unimprovedLand=%d improvedLandDelta=%+d improvedLandPercentX100=%d developmentWater=%d improvedWater=%d unimprovedWater=%d improvedWaterDelta=%+d improvedWaterPercentX100=%d"
			" bfcDevelopmentLand=%d bfcImprovedLand=%d bfcUnimprovedLand=%d bfcImprovedLandPercentX100=%d bfcDevelopmentWater=%d bfcImprovedWater=%d bfcUnimprovedWater=%d bfcImprovedWaterPercentX100=%d suburbDevelopmentLand=%d suburbImprovedLand=%d suburbUnimprovedLand=%d suburbImprovedLandPercentX100=%d suburbDevelopmentWater=%d suburbImprovedWater=%d suburbUnimprovedWater=%d suburbImprovedWaterPercentX100=%d roaded=%d roadedDelta=%+d bfcDevelopmentLandRoaded=%d bfcDevelopmentLandRoadedDelta=%+d bfcDevelopmentLandRoadedPercentX100=%d suburbDevelopmentLandRoaded=%d suburbDevelopmentLandRoadedDelta=%+d suburbDevelopmentLandRoadedPercentX100=%d bfcDevelopmentRoutes=%s suburbDevelopmentRoutes=%s bonusImproved=%d bonusUnimproved=%d"
			" irrigationCarriers=%d irrigationCarriersDelta=%+d irrigatedCarriers=%d irrigatedCarriersDelta=%+d dryCarriers=%d dryCarriersDelta=%+d irrigatedCarrierPercentX100=%d dryCarrierPercentX100=%d bonusIrrigationCarriers=%d irrigatedBonusCarriers=%d dryBonusCarriers=%d bfcIrrigationCarriers=%d bfcIrrigatedCarriers=%d bfcDryCarriers=%d bfcIrrigatedCarrierPercentX100=%d suburbIrrigationCarriers=%d suburbIrrigatedCarriers=%d suburbDryCarriers=%d suburbIrrigatedCarrierPercentX100=%d irrigatedCarrierTypes=%s dryCarrierTypes=%s improvements=%s routes=%s",
		iGameTurn, ePlayer, kPrevious.bValid, kOwned.iPlots, kOwned.iLand, kOwned.iWater, kDevelopment.iBFCPlots, kDevelopment.iSuburbPlots,
		iDevelopmentPlots, iImprovedPlots, iUnimprovedPlots, getSASGameRecordPercentX100(iImprovedPlots, iDevelopmentPlots),
		kDevelopment.iDevelopmentLand, kDevelopment.iImprovedLand, iUnimprovedLand,
		getSASGameRecordDelta(kPrevious.bValid, kDevelopment.iImprovedLand, kPrevious.iTerritoryImprovedLand),
		getSASGameRecordPercentX100(kDevelopment.iImprovedLand, kDevelopment.iDevelopmentLand), kDevelopment.iDevelopmentWater,
		kDevelopment.iImprovedWater, iUnimprovedWater,
		getSASGameRecordDelta(kPrevious.bValid, kDevelopment.iImprovedWater, kPrevious.iTerritoryImprovedWater),
		getSASGameRecordPercentX100(kDevelopment.iImprovedWater, kDevelopment.iDevelopmentWater), kDevelopment.iBFCDevelopmentLand,
		kDevelopment.iBFCImprovedLand, iBFCUnimprovedLand,
		getSASGameRecordPercentX100(kDevelopment.iBFCImprovedLand, kDevelopment.iBFCDevelopmentLand), kDevelopment.iBFCDevelopmentWater,
		kDevelopment.iBFCImprovedWater, iBFCUnimprovedWater,
		getSASGameRecordPercentX100(kDevelopment.iBFCImprovedWater, kDevelopment.iBFCDevelopmentWater), kDevelopment.iSuburbDevelopmentLand,
		kDevelopment.iSuburbImprovedLand, iSuburbUnimprovedLand,
		getSASGameRecordPercentX100(kDevelopment.iSuburbImprovedLand, kDevelopment.iSuburbDevelopmentLand),
		kDevelopment.iSuburbDevelopmentWater, kDevelopment.iSuburbImprovedWater, iSuburbUnimprovedWater,
		getSASGameRecordPercentX100(kDevelopment.iSuburbImprovedWater, kDevelopment.iSuburbDevelopmentWater), kOwned.iRoaded,
		getSASGameRecordDelta(kPrevious.bValid, kOwned.iRoaded, kPrevious.iTerritoryRoaded), kDevelopment.iBFCDevelopmentLandRoaded,
		getSASGameRecordDelta(kPrevious.bValid, kDevelopment.iBFCDevelopmentLandRoaded, kPrevious.iTerritoryBFCDevelopmentLandRoaded),
		getSASGameRecordPercentX100(kDevelopment.iBFCDevelopmentLandRoaded, kDevelopment.iBFCDevelopmentLand), kDevelopment.iSuburbDevelopmentLandRoaded,
		getSASGameRecordDelta(kPrevious.bValid, kDevelopment.iSuburbDevelopmentLandRoaded, kPrevious.iTerritorySuburbDevelopmentLandRoaded),
		getSASGameRecordPercentX100(kDevelopment.iSuburbDevelopmentLandRoaded, kDevelopment.iSuburbDevelopmentLand),
		getSASDiagnosticOrDash(szBFCDevelopmentRoutes).GetCString(), getSASDiagnosticOrDash(szSuburbDevelopmentRoutes).GetCString(),
		kOwned.iBonusImproved, kOwned.iBonusUnimproved,
		kDevelopment.iIrrigationCarriers, getSASGameRecordDelta(kPrevious.bValid, kDevelopment.iIrrigationCarriers, kPrevious.iTerritoryIrrigationCarriers),
		kDevelopment.iIrrigatedCarriers,
		getSASGameRecordDelta(kPrevious.bValid, kDevelopment.iIrrigatedCarriers, kPrevious.iTerritoryIrrigatedCarriers), kDevelopment.iDryCarriers,
		getSASGameRecordDelta(kPrevious.bValid, kDevelopment.iDryCarriers, kPrevious.iTerritoryDryCarriers),
		getSASGameRecordPercentX100(kDevelopment.iIrrigatedCarriers, kDevelopment.iIrrigationCarriers),
		getSASGameRecordPercentX100(kDevelopment.iDryCarriers, kDevelopment.iIrrigationCarriers), kDevelopment.iBonusIrrigationCarriers,
		kDevelopment.iIrrigatedBonusCarriers, kDevelopment.iDryBonusCarriers, kDevelopment.iBFCIrrigationCarriers, kDevelopment.iBFCIrrigatedCarriers,
		kDevelopment.iBFCDryCarriers, getSASGameRecordPercentX100(kDevelopment.iBFCIrrigatedCarriers, kDevelopment.iBFCIrrigationCarriers), iSuburbIrrigationCarriers,
		iSuburbIrrigatedCarriers, iSuburbDryCarriers, getSASGameRecordPercentX100(iSuburbIrrigatedCarriers, iSuburbIrrigationCarriers),
		getSASDiagnosticOrDash(szIrrigatedCarrierTypes).GetCString(), getSASDiagnosticOrDash(szDryCarrierTypes).GetCString(),
		getSASDiagnosticOrDash(szImprovements).GetCString(), getSASDiagnosticOrDash(szRoutes).GetCString());
	if (gGameRecordLogLevel >= 3)
	{
		CvString szTerrains;
		CvString szFeatures;
		CvString szBonuses;
		CvString szImprovedBonuses;
		CvString szUnimprovedBonuses;
		getSASGameRecordLandscapeTypes(kOwned, szTerrains, szFeatures, szBonuses);
		for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
		{
			appendSASGameRecordTypeCount(szImprovedBonuses, getSASGameRecordBonusType((BonusTypes)iI), kDevelopment.aiImprovedBonuses[iI]);
			appendSASGameRecordTypeCount(szUnimprovedBonuses, getSASGameRecordBonusType((BonusTypes)iI), kDevelopment.aiUnimprovedBonuses[iI]);
		}
		logSASGameRecord("GAME_RECORD_TERRITORY_LANDSCAPE turn=%d player=%d terrains=%s features=%s bonuses=%s improvedBonuses=%s unimprovedBonuses=%s",
			iGameTurn, ePlayer, getSASDiagnosticOrDash(szTerrains).GetCString(), getSASDiagnosticOrDash(szFeatures).GetCString(),
			getSASDiagnosticOrDash(szBonuses).GetCString(), getSASDiagnosticOrDash(szImprovedBonuses).GetCString(),
			getSASDiagnosticOrDash(szUnimprovedBonuses).GetCString());
	}
	kPrevious.iTerritoryImprovedLand = kDevelopment.iImprovedLand;
	kPrevious.iTerritoryImprovedWater = kDevelopment.iImprovedWater;
	kPrevious.iTerritoryRoaded = kOwned.iRoaded;
	kPrevious.iTerritoryBFCDevelopmentLandRoaded = kDevelopment.iBFCDevelopmentLandRoaded;
	kPrevious.iTerritorySuburbDevelopmentLandRoaded = kDevelopment.iSuburbDevelopmentLandRoaded;
	kPrevious.iTerritoryIrrigationCarriers = kDevelopment.iIrrigationCarriers;
	kPrevious.iTerritoryIrrigatedCarriers = kDevelopment.iIrrigatedCarriers;
	kPrevious.iTerritoryDryCarriers = kDevelopment.iDryCarriers;
}

static void getSASGameRecordPlotCompositionTypes(SASGameRecordPlotComposition const& kComposition, CvString& szTerrains, CvString& szFeatures, CvString& szBonuses, CvString& szImprovements, CvString& szRoutes)
{
	getSASGameRecordLandscapeTypes(kComposition, szTerrains, szFeatures, szBonuses);
	getSASGameRecordImprovementRouteTypes(kComposition, szImprovements, szRoutes);
}

static void logSASGameRecordKnownArea(PlayerTypes ePlayer, const char* szReason)
{
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	TeamTypes eTeam = kPlayer.getTeam();
	if (eTeam == NO_TEAM)
		return;
	SASGameRecordPlotComposition kRevealed;
	SASGameRecordPlotComposition kVisible;
	CvMap const& kMap = GC.getMap();
	for (int iI = 0; iI < kMap.numPlots(); iI++)
	{
		CvPlot const& kPlot = kMap.getPlotByIndex(iI);
		if (kPlot.isRevealed(eTeam, false))
			addSASGameRecordPlotComposition(kRevealed, kPlot, eTeam);
		if (kPlot.isVisible(eTeam, false))
			addSASGameRecordPlotComposition(kVisible, kPlot, eTeam);
	}
	logSASGameRecord("GAME_RECORD_KNOWN_AREA turn=%d reason=%s player=%d team=%d revealedPlots=%d visiblePlots=%d revealedLand=%d visibleLand=%d revealedWater=%d visibleWater=%d revealedHills=%d visibleHills=%d revealedPeaks=%d visiblePeaks=%d revealedRiverSide=%d visibleRiverSide=%d revealedFreshWater=%d visibleFreshWater=%d revealedCoastal=%d visibleCoastal=%d revealedImproved=%d visibleImproved=%d revealedUnimprovedLand=%d visibleUnimprovedLand=%d revealedRoaded=%d visibleRoaded=%d revealedBonusImproved=%d visibleBonusImproved=%d revealedBonusUnimproved=%d visibleBonusUnimproved=%d revealedNatureFood=%d visibleNatureFood=%d revealedNatureProd=%d visibleNatureProd=%d revealedNatureCommerce=%d visibleNatureCommerce=%d revealedCurrentFood=%d visibleCurrentFood=%d revealedCurrentProd=%d visibleCurrentProd=%d revealedCurrentCommerce=%d visibleCurrentCommerce=%d",
			GC.getGame().getGameTurn(), szReason, ePlayer, eTeam,
			kRevealed.iPlots, kVisible.iPlots, kRevealed.iLand, kVisible.iLand, kRevealed.iWater, kVisible.iWater,
			kRevealed.iHills, kVisible.iHills, kRevealed.iPeaks, kVisible.iPeaks, kRevealed.iRiverSide, kVisible.iRiverSide,
			kRevealed.iFreshWater, kVisible.iFreshWater, kRevealed.iCoastal, kVisible.iCoastal, kRevealed.iImproved, kVisible.iImproved,
			kRevealed.iUnimprovedLand, kVisible.iUnimprovedLand, kRevealed.iRoaded, kVisible.iRoaded, kRevealed.iBonusImproved, kVisible.iBonusImproved,
			kRevealed.iBonusUnimproved, kVisible.iBonusUnimproved, kRevealed.iNatureFood, kVisible.iNatureFood, kRevealed.iNatureProduction, kVisible.iNatureProduction,
			kRevealed.iNatureCommerce, kVisible.iNatureCommerce, kRevealed.iCurrentFood, kVisible.iCurrentFood,
			kRevealed.iCurrentProduction, kVisible.iCurrentProduction, kRevealed.iCurrentCommerce, kVisible.iCurrentCommerce);
	CvString szRevealedTerrains;
	CvString szVisibleTerrains;
	CvString szRevealedFeatures;
	CvString szVisibleFeatures;
	CvString szRevealedBonuses;
	CvString szVisibleBonuses;
	CvString szRevealedImprovements;
	CvString szVisibleImprovements;
	CvString szRevealedRoutes;
	CvString szVisibleRoutes;
	getSASGameRecordPlotCompositionTypes(kRevealed, szRevealedTerrains, szRevealedFeatures, szRevealedBonuses, szRevealedImprovements, szRevealedRoutes);
	getSASGameRecordPlotCompositionTypes(kVisible, szVisibleTerrains, szVisibleFeatures, szVisibleBonuses, szVisibleImprovements, szVisibleRoutes);
	logSASGameRecord("GAME_RECORD_KNOWN_AREA_TYPES turn=%d reason=%s player=%d team=%d revealedTerrains=%s visibleTerrains=%s revealedFeatures=%s visibleFeatures=%s revealedBonuses=%s visibleBonuses=%s revealedImprovements=%s visibleImprovements=%s revealedRoutes=%s visibleRoutes=%s",
			GC.getGame().getGameTurn(), szReason, ePlayer, eTeam,
			getSASDiagnosticOrDash(szRevealedTerrains).GetCString(), getSASDiagnosticOrDash(szVisibleTerrains).GetCString(),
			getSASDiagnosticOrDash(szRevealedFeatures).GetCString(), getSASDiagnosticOrDash(szVisibleFeatures).GetCString(),
			getSASDiagnosticOrDash(szRevealedBonuses).GetCString(), getSASDiagnosticOrDash(szVisibleBonuses).GetCString(),
			getSASDiagnosticOrDash(szRevealedImprovements).GetCString(), getSASDiagnosticOrDash(szVisibleImprovements).GetCString(),
			getSASDiagnosticOrDash(szRevealedRoutes).GetCString(), getSASDiagnosticOrDash(szVisibleRoutes).GetCString());
}

static void logSASGameRecordStartingUnits(PlayerTypes ePlayer, const char* szReason)
{
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	if (GC.getGame().getElapsedGameTurns() > 1)
		return;
	int iLoop = 0;
	for (CvUnit const* pLoopUnit = kPlayer.firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = kPlayer.nextUnit(&iLoop))
	{
		CvPlot const& kPlot = pLoopUnit->getPlot();
		logSASGameRecord("GAME_RECORD_STARTING_UNIT turn=%d reason=%s player=%d unitId=%d unit=%s unitAI=%s unitCombat=%s x=%d y=%d damage=%d xp=%d level=%d movesLeft=%d plotOwner=%d plotTerrain=%s plotFeature=%s plotBonus=%s plotImprovement=%s plotRoute=%s",
			GC.getGame().getGameTurn(), szReason, ePlayer, pLoopUnit->getID(), getSASGameRecordUnitType(pLoopUnit->getUnitType()),
			getSASGameRecordUnitAIType(pLoopUnit->AI_getUnitAIType()),
			getSASGameRecordUnitCombatType(pLoopUnit->getUnitInfo().getUnitCombatType()), pLoopUnit->getX(), pLoopUnit->getY(),
			pLoopUnit->getDamage(), pLoopUnit->getExperience(), pLoopUnit->getLevel(), pLoopUnit->movesLeft(), kPlot.getOwner(),
			getSASGameRecordTerrainType(kPlot.getTerrainType()), getSASGameRecordFeatureType(kPlot.getFeatureType()),
			getSASGameRecordBonusType(kPlot.getBonusType(pLoopUnit->getTeam())), getSASGameRecordImprovementType(kPlot.getImprovementType()),
			getSASGameRecordRouteType(kPlot.getRouteType()));
	}
}

static void logSASGameRecordCityBFC(CvCity const& kCity, const char* szReason)
{
	CvString szTerrains;
	CvString szFeatures;
	CvString szBonuses;
	CvString szImprovements;
	CvString szRoutes;
	SASGameRecordPlotComposition kComposition;
	int iOwned = 0;
	TeamTypes eTeam = GET_PLAYER(kCity.getOwner()).getTeam();
	for (CityPlotIter it(kCity); it.hasNext(); ++it)
	{
		CvPlot const& kPlot = *it;
		if (kPlot.getOwner() == kCity.getOwner())
			iOwned++;
		addSASGameRecordPlotComposition(kComposition, kPlot, eTeam);
	}
	getSASGameRecordPlotCompositionTypes(kComposition, szTerrains, szFeatures, szBonuses, szImprovements, szRoutes);
	logSASGameRecord("GAME_RECORD_CITY_BFC turn=%d reason=%s player=%d cityId=%d city=%S x=%d y=%d plots=%d owned=%d land=%d water=%d hills=%d peaks=%d riverSide=%d freshWater=%d coastal=%d improved=%d unimprovedLand=%d roaded=%d bonusImproved=%d bonusUnimproved=%d worked=%d workedImproved=%d workedUnimproved=%d natureFood=%d natureProd=%d natureCommerce=%d currentFood=%d currentProd=%d currentCommerce=%d terrains=%s features=%s bonuses=%s improvements=%s routes=%s",
		GC.getGame().getGameTurn(), szReason, kCity.getOwner(), kCity.getID(), getSASGameRecordQuotedCityName(&kCity).GetCString(),
		kCity.getX(), kCity.getY(), kComposition.iPlots, iOwned, kComposition.iLand, kComposition.iWater, kComposition.iHills,
		kComposition.iPeaks, kComposition.iRiverSide, kComposition.iFreshWater, kComposition.iCoastal, kComposition.iImproved,
		kComposition.iUnimprovedLand, kComposition.iRoaded, kComposition.iBonusImproved, kComposition.iBonusUnimproved, kComposition.iWorked,
		kComposition.iWorkedImproved, kComposition.iWorkedUnimproved, kComposition.iNatureFood, kComposition.iNatureProduction,
		kComposition.iNatureCommerce, kComposition.iCurrentFood, kComposition.iCurrentProduction, kComposition.iCurrentCommerce,
		getSASDiagnosticOrDash(szTerrains).GetCString(), getSASDiagnosticOrDash(szFeatures).GetCString(),
		getSASDiagnosticOrDash(szBonuses).GetCString(), getSASDiagnosticOrDash(szImprovements).GetCString(),
		getSASDiagnosticOrDash(szRoutes).GetCString());
}

static SASGameRecordPlotComposition getSASGameRecordWorkedPlotComposition(CvCity const& kCity)
{
	SASGameRecordPlotComposition kComposition;
	const TeamTypes eTeam = GET_PLAYER(kCity.getOwner()).getTeam();
	// <!-- custom: Exclude the city center from worked-plot allocation records because it is always worked and would blur comparisons of citizen plot choices and improvement coverage between benchmark runs. (GPT-5.5) -->
	for (WorkingPlotIter it(kCity, false); it.hasNext(); ++it)
		addSASGameRecordPlotComposition(kComposition, *it, eTeam);
	return kComposition;
}

static void addSASGameRecordPlotComposition(SASGameRecordPlotComposition& kTarget, SASGameRecordPlotComposition const& kSource)
{
	kTarget.iPlots += kSource.iPlots;
	kTarget.iLand += kSource.iLand;
	kTarget.iWater += kSource.iWater;
	kTarget.iHills += kSource.iHills;
	kTarget.iPeaks += kSource.iPeaks;
	kTarget.iRiverSide += kSource.iRiverSide;
	kTarget.iFreshWater += kSource.iFreshWater;
	kTarget.iCoastal += kSource.iCoastal;
	kTarget.iImproved += kSource.iImproved;
	kTarget.iUnimprovedLand += kSource.iUnimprovedLand;
	kTarget.iRoaded += kSource.iRoaded;
	kTarget.iBonusImproved += kSource.iBonusImproved;
	kTarget.iBonusUnimproved += kSource.iBonusUnimproved;
	kTarget.iWorked += kSource.iWorked;
	kTarget.iWorkedImproved += kSource.iWorkedImproved;
	kTarget.iWorkedUnimproved += kSource.iWorkedUnimproved;
	kTarget.iNatureFood += kSource.iNatureFood;
	kTarget.iNatureProduction += kSource.iNatureProduction;
	kTarget.iNatureCommerce += kSource.iNatureCommerce;
	kTarget.iCurrentFood += kSource.iCurrentFood;
	kTarget.iCurrentProduction += kSource.iCurrentProduction;
	kTarget.iCurrentCommerce += kSource.iCurrentCommerce;
	for (int iI = 0; iI < GC.getNumTerrainInfos(); iI++)
		kTarget.aiTerrains[iI] += kSource.aiTerrains[iI];
	for (int iI = 0; iI < GC.getNumFeatureInfos(); iI++)
		kTarget.aiFeatures[iI] += kSource.aiFeatures[iI];
	for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
		kTarget.aiBonuses[iI] += kSource.aiBonuses[iI];
	for (int iI = 0; iI < GC.getNumImprovementInfos(); iI++)
		kTarget.aiImprovements[iI] += kSource.aiImprovements[iI];
	for (int iI = 0; iI < GC.getNumRouteInfos(); iI++)
		kTarget.aiRoutes[iI] += kSource.aiRoutes[iI];
}

static CvString getSASGameRecordTechEraCounts(TeamTypes eTeam)
{
	std::vector<int> aiEras(GC.getNumEraInfos(), 0);
	CvTeam const& kTeam = GET_TEAM(eTeam);
	FOR_EACH_ENUM(Tech)
	{
		if (!kTeam.isHasTech(eLoopTech))
			continue;
		EraTypes eEra = GC.getInfo(eLoopTech).getEra();
		if (eEra != NO_ERA)
			aiEras[eEra]++;
	}
	CvString szList;
	for (int iI = 0; iI < GC.getNumEraInfos(); iI++)
		appendSASGameRecordTypeCount(szList, getSASGameRecordEraType((EraTypes)iI), aiEras[iI]);
	return getSASDiagnosticOrDash(szList);
}

static void logSASGameRecordPlayerSetup(PlayerTypes ePlayer)
{
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	CvInitCore const& kInitCore = GC.getInitCore();
	const char* szCivType = (kPlayer.getCivilizationType() == NO_CIVILIZATION ? "-" : GC.getInfo(kPlayer.getCivilizationType()).getType());
	const char* szLeaderType = (kPlayer.getLeaderType() == NO_LEADER ? "-" : GC.getInfo(kPlayer.getLeaderType()).getType());
	const wchar* szLeaderName = (kPlayer.getLeaderType() == NO_LEADER ? L"-" : GC.getInfo(kPlayer.getLeaderType()).getDescription());
	// <!-- custom: During AI Auto Play, isHuman becomes false for the original human slot while isHumanDisabled becomes true.
	// Record both states explicitly so setup/load rows do not make the same player appear ambiguously human in one place and AI-controlled in another. (GPT-5.6-Sol) -->
	const bool bCurrentlyHumanControlled = kPlayer.isHuman();
	const bool bAutoplayControlled = kPlayer.isHumanDisabled();
	const bool bHumanSlot = (bCurrentlyHumanControlled || bAutoplayControlled);
	PlayerColorTypes const ePlayerColor = kPlayer.getPlayerColor();
	char const* szPlayerColor = "-";
	char const* szPrimaryColor = "-";
	int iPrimaryRed = -1;
	int iPrimaryGreen = -1;
	int iPrimaryBlue = -1;
	if (ePlayerColor != NO_PLAYERCOLOR)
	{
		CvPlayerColorInfo const& kPlayerColor = GC.getInfo(ePlayerColor);
		ColorTypes const ePrimaryColor = kPlayerColor.getColorTypePrimary();
		szPlayerColor = kPlayerColor.getType();
		if (ePrimaryColor != NO_COLOR)
		{
			NiColorA const& kPrimaryColor = GC.getInfo(ePrimaryColor).getColor();
			szPrimaryColor = GC.getInfo(ePrimaryColor).getType();
			iPrimaryRed = (int)(255 * kPrimaryColor.r);
			iPrimaryGreen = (int)(255 * kPrimaryColor.g);
			iPrimaryBlue = (int)(255 * kPrimaryColor.b);
		}
	}
	CvString szTraits;
	FOR_EACH_ENUM(Trait)
	{
		if (!kPlayer.hasTrait(eLoopTrait))
			continue;
		if (!szTraits.empty())
			szTraits += ",";
		szTraits += GC.getInfo(eLoopTrait).getType();
	}
	// <!-- custom: Leader traits and favorites are fixed but materially explain AI behavior and economic results.
	// Record them once per setup/load rather than repeating them in periodic player or policy snapshots. (GPT-5.6-Sol) -->
	// <!-- custom: Log the assigned PlayerColor rather than the civilization default because Civ4 can reassign duplicates.
	// The primary ColorInfo and RGB values help connect text records to maps and screenshots without requiring the source XML. (GPT-5.6-Sol) -->
	// <!-- custom: CvInitCore preserves whether civilization and leader were assigned through Random.
	// Older/imported saves can lack that provenance, so keep unknown distinct from a verified manual choice. (ChatGPT-5.6-Sol) -->
	bool const bCivLeaderChoiceKnown = kInitCore.isCivLeaderSetupKnown();
	logSASGameRecord("GAME_RECORD_PLAYER_SETUP turn=%d player=%d team=%d alive=%d everAlive=%d human=%d humanSlot=%d currentlyHumanControlled=%d autoplayControlled=%d slotStatus=%d civLeaderChoiceKnown=%d civChosenRandomly=%d leaderChosenRandomly=%d playerName=%S civType=%s civName=%S civShortName=%S leaderType=%s leaderName=%S playerColor=%s primaryColor=%s primaryColorRGB=%d,%d,%d traits=%s favoriteCivic=%s favoriteReligion=%s handicap=%s",
		GC.getGame().getGameTurn(), ePlayer, kPlayer.getTeam(), kPlayer.isAlive(), kPlayer.isEverAlive(), bCurrentlyHumanControlled,
		bHumanSlot, bCurrentlyHumanControlled, bAutoplayControlled, kInitCore.getSlotStatus(ePlayer), bCivLeaderChoiceKnown,
		bCivLeaderChoiceKnown ? kInitCore.wasCivRandomlyChosen(ePlayer) : -1,
		bCivLeaderChoiceKnown ? kInitCore.wasLeaderRandomlyChosen(ePlayer) : -1, getSASDiagnosticQuoted(kPlayer.getName(0)).GetCString(),
		szCivType, getSASDiagnosticQuoted(kPlayer.getCivilizationDescription(0)).GetCString(),
		getSASDiagnosticQuoted(kPlayer.getCivilizationShortDescription(0)).GetCString(), szLeaderType,
		getSASDiagnosticQuoted(szLeaderName).GetCString(), szPlayerColor, szPrimaryColor, iPrimaryRed, iPrimaryGreen, iPrimaryBlue,
		getSASDiagnosticOrDash(szTraits).GetCString(), getSASGameRecordCivicType(kPlayer.getFavoriteCivic()),
		getSASGameRecordReligionType(kPlayer.getFavoriteReligion()),
		kPlayer.getHandicapType() == NO_HANDICAP ? "-" : GC.getInfo(kPlayer.getHandicapType()).getType());
}

static void logSASGameRecordAttitudeLegend()
{
	const int iFuriousMax = GC.getDefineINT(CvGlobals::RELATIONS_THRESH_FURIOUS);
	const int iAnnoyedMax = GC.getDefineINT(CvGlobals::RELATIONS_THRESH_ANNOYED);
	const int iPleasedMin = GC.getDefineINT(CvGlobals::RELATIONS_THRESH_PLEASED);
	const int iFriendlyMin = GC.getDefineINT(CvGlobals::RELATIONS_THRESH_FRIENDLY);
	logSASGameRecord("GAME_RECORD_ATTITUDE_LEGEND valueFrom=AI_getAttitudeVal furious=<=%d annoyed=%d..%d cautious=%d..%d pleased=%d..%d friendly=>=%d",
			iFuriousMax, iFuriousMax + 1, iAnnoyedMax, iAnnoyedMax + 1, iPleasedMin - 1, iPleasedMin, iFriendlyMin - 1, iFriendlyMin);
}

struct SASGameRecordLandmassGeography
{
	SASGameRecordLandmassGeography() : iAreaId(-1), iAnchorX(-1), iAnchorY(-1), iPlots(0), iHabitablePlots(0), iImpassablePlots(0), iZeroNatureYieldPlots(0), iWaterBorderPlots(0), iSeaBorderPlots(0), iLakeBorderPlots(0), iIceBorderPlots(0), iSeaBorderEdges(0), iLakeBorderEdges(0), iIceBorderEdges(0), iHillsPlots(0), iPeakPlots(0), iRiverSidePlots(0), iFreshWaterPlots(0), iSumWrappedDX(0), iSumWrappedDY(0), iStartingPlayers(0), iNatureFood(0), iNatureProduction(0), iNatureCommerce(0), iBonusCount(0) {}
	int iAreaId;
	int iAnchorX;
	int iAnchorY;
	int iPlots;
	int iHabitablePlots;
	int iImpassablePlots;
	int iZeroNatureYieldPlots;
	int iWaterBorderPlots;
	int iSeaBorderPlots;
	int iLakeBorderPlots;
	int iIceBorderPlots;
	int iSeaBorderEdges;
	int iLakeBorderEdges;
	int iIceBorderEdges;
	int iHillsPlots;
	int iPeakPlots;
	int iRiverSidePlots;
	int iFreshWaterPlots;
	int iSumWrappedDX;
	int iSumWrappedDY;
	int iStartingPlayers;
	int iNatureFood;
	int iNatureProduction;
	int iNatureCommerce;
	int iBonusCount;
	CvString szStartingPlayers;
	std::vector<int> aiNoOceanConnectedAreas;
	std::vector<int> aiNoOceanNavigableConnectedAreas;
	std::vector<int> aiAdjacentLakeAreas;
	std::vector<int> aiTerrainCounts;
	std::vector<int> aiFeatureCounts;
	std::vector<int> aiBonusCounts;
	std::vector<CvString> aszBonusCoordinates;
	std::vector<int> aiEraRevealedRawFood;
	std::vector<int> aiEraRevealedRawProduction;
	std::vector<int> aiEraRevealedRawCommerce;
	std::vector<int> aiEraRevealedRaw321;
	std::vector<int> aiEraBonusPotentialFood;
	std::vector<int> aiEraBonusPotentialProduction;
	std::vector<int> aiEraBonusPotentialCommerce;
	std::vector<int> aiEraBonusPotential321;
};

static SASGameRecordLandmassGeography* getSASGameRecordLandmassGeography(std::vector<SASGameRecordLandmassGeography>& aLandmasses, int iAreaId)
{
	for (size_t iI = 0; iI < aLandmasses.size(); iI++)
	{
		if (aLandmasses[iI].iAreaId == iAreaId)
			return &aLandmasses[iI];
	}
	return NULL;
}

static void addSASGameRecordUniqueArea(std::vector<int>& aiAreas, int iAreaId)
{
	if (iAreaId < 0)
		return;
	if (std::find(aiAreas.begin(), aiAreas.end(), iAreaId) == aiAreas.end())
		aiAreas.push_back(iAreaId);
}

static CvString getSASGameRecordLandmassName(SASGameRecordLandmassGeography const& kLandmass)
{
	CvString szName;
	szName.Format("LAND_%d_%d_A%d", kLandmass.iAnchorX, kLandmass.iAnchorY, kLandmass.iAreaId);
	return szName;
}

static CvString getSASGameRecordLandmassList(std::vector<SASGameRecordLandmassGeography> const& aLandmasses, std::vector<int> const& aiAreas)
{
	CvString szList;
	for (size_t iI = 0; iI < aiAreas.size(); iI++)
	{
		for (size_t iJ = 0; iJ < aLandmasses.size(); iJ++)
		{
			if (aLandmasses[iJ].iAreaId != aiAreas[iI])
				continue;
			CvString szItem;
			CvString const szName = getSASGameRecordLandmassName(aLandmasses[iJ]);
			szItem.Format(szList.empty() ? "%s" : ",%s", szName.GetCString());
			szList += szItem;
			break;
		}
	}
	return getSASDiagnosticOrDash(szList);
}

static CvString getSASGameRecordLandmassTerrainPercentages(SASGameRecordLandmassGeography const& kLandmass)
{
	CvString szList;
	for (int iTerrain = 0; iTerrain < (int)kLandmass.aiTerrainCounts.size(); iTerrain++)
	{
		if (kLandmass.aiTerrainCounts[iTerrain] <= 0)
			continue;
		CvString szItem;
		szItem.Format(szList.empty() ? "%s:%d" : ",%s:%d", GC.getInfo((TerrainTypes)iTerrain).getType(), getSASGameRecordPercentX100(kLandmass.aiTerrainCounts[iTerrain], kLandmass.iPlots));
		szList += szItem;
	}
	return getSASDiagnosticOrDash(szList);
}

static CvString getSASGameRecordLandmassFeaturePercentages(SASGameRecordLandmassGeography const& kLandmass)
{
	CvString szList;
	for (int iFeature = 0; iFeature < (int)kLandmass.aiFeatureCounts.size(); iFeature++)
	{
		if (kLandmass.aiFeatureCounts[iFeature] <= 0)
			continue;
		CvString szItem;
		szItem.Format(szList.empty() ? "%s:%d" : ",%s:%d", GC.getInfo((FeatureTypes)iFeature).getType(), getSASGameRecordPercentX100(kLandmass.aiFeatureCounts[iFeature], kLandmass.iPlots));
		szList += szItem;
	}
	return getSASDiagnosticOrDash(szList);
}

static int getSASGameRecordAdjacentLakePlots(SASGameRecordLandmassGeography const& kLandmass)
{
	int iPlots = 0;
	for (size_t iI = 0; iI < kLandmass.aiAdjacentLakeAreas.size(); iI++)
	{
		CvArea const* pArea = GC.getMap().getArea(kLandmass.aiAdjacentLakeAreas[iI]);
		if (pArea != NULL && pArea->isLake())
			iPlots += pArea->getNumTiles();
	}
	return iPlots;
}

static bool isSASGameRecordTechAvailableByEra(TechTypes eTech, EraTypes eEra)
{
	return (eTech == NO_TECH || GC.getInfo(eTech).getEra() <= eEra);
}

static bool isSASGameRecordBuildAvailableByEra(CvPlot const& kPlot, BuildTypes eBuild, EraTypes eEra)
{
	CvBuildInfo const& kBuild = GC.getInfo(eBuild);
	if (kBuild.getImprovement() == NO_IMPROVEMENT || !isSASGameRecordTechAvailableByEra(kBuild.getTechPrereq(), eEra))
		return false;
	FeatureTypes const eFeature = kPlot.getFeatureType();
	if (eFeature != NO_FEATURE && kBuild.isFeatureRemove(eFeature) && !isSASGameRecordTechAvailableByEra(kBuild.getFeatureTech(eFeature), eEra))
		return false;
	return kPlot.canHaveImprovement(kBuild.getImprovement(), NO_TEAM, true, eBuild, false);
}

static bool isSASGameRecordIrrigationAvailableByEra(EraTypes eEra)
{
	static std::vector<char> abAvailable;
	if (abAvailable.empty())
	{
		abAvailable.assign(GC.getNumEraInfos(), 0);
		FOR_EACH_ENUM(Tech)
		{
			if (!GC.getInfo(eLoopTech).isIrrigation())
				continue;
			for (int iEra = GC.getInfo(eLoopTech).getEra(); iEra < GC.getNumEraInfos(); iEra++)
				abAvailable[iEra] = 1;
		}
	}
	return (eEra >= 0 && eEra < (int)abAvailable.size() && abAvailable[eEra] != 0);
}

static int getSASGameRecordImprovementTechYieldByEra(ImprovementTypes eImprovement, YieldTypes eYield, EraTypes eEra)
{
	static std::vector<int> aiYield;
	int const iNumEras = GC.getNumEraInfos();
	int const iNumImprovements = GC.getNumImprovementInfos();
	if (aiYield.empty())
	{
		aiYield.assign(iNumEras * iNumImprovements * NUM_YIELD_TYPES, 0);
		for (int iEra = 0; iEra < iNumEras; iEra++)
		{
			for (int iImprovement = 0; iImprovement < iNumImprovements; iImprovement++)
			{
				FOR_EACH_ENUM(Yield)
				{
					int iValue = 0;
					FOR_EACH_ENUM(Tech)
					{
						if (GC.getInfo(eLoopTech).getEra() <= iEra)
							iValue += GC.getInfo((ImprovementTypes)iImprovement).getTechYieldChanges(eLoopTech, eLoopYield);
					}
					aiYield[(iEra * iNumImprovements + iImprovement) * NUM_YIELD_TYPES + eLoopYield] = iValue;
				}
			}
		}
	}
	if (eEra < 0 || eEra >= iNumEras || eImprovement < 0 || eImprovement >= iNumImprovements || eYield < 0 || eYield >= NUM_YIELD_TYPES)
		return 0;
	return aiYield[(eEra * iNumImprovements + eImprovement) * NUM_YIELD_TYPES + eYield];
}

static void getSASGameRecordLandmassPlotPotential(CvPlot const& kPlot, EraTypes eEra, int& iRawFood, int& iRawProduction, int& iRawCommerce, int& iRaw321, int& iPotentialFood, int& iPotentialProduction, int& iPotentialCommerce, int& iPotential321)
{
	BonusTypes const eBonus = kPlot.getBonusType(NO_TEAM);
	bool const bBonusRevealed = (eBonus != NO_BONUS && isSASGameRecordTechAvailableByEra(GC.getInfo(eBonus).getTechReveal(), eEra));
	int aiRaw[NUM_YIELD_TYPES] = { 0 };
	FOR_EACH_ENUM(Yield)
	{
		aiRaw[eLoopYield] = kPlot.calculateNatureYield(eLoopYield, NO_TEAM);
		if (bBonusRevealed)
			aiRaw[eLoopYield] += GC.getInfo(eBonus).getYieldChange(eLoopYield);
	}
	iRawFood = aiRaw[YIELD_FOOD];
	iRawProduction = aiRaw[YIELD_PRODUCTION];
	iRawCommerce = aiRaw[YIELD_COMMERCE];
	iRaw321 = 3 * iRawFood + 2 * iRawProduction + iRawCommerce;
	iPotentialFood = iRawFood;
	iPotentialProduction = iRawProduction;
	iPotentialCommerce = iRawCommerce;
	iPotential321 = iRaw321;
	if (!bBonusRevealed || !isSASGameRecordTechAvailableByEra(GC.getInfo(eBonus).getTechCityTrade(), eEra))
		return;
	bool const bIrrigationPotential = (isSASGameRecordIrrigationAvailableByEra(eEra) && kPlot.canHavePotentialIrrigation());
	FOR_EACH_ENUM(Build)
	{
		if (!isSASGameRecordBuildAvailableByEra(kPlot, eLoopBuild, eEra))
			continue;
		CvBuildInfo const& kBuild = GC.getInfo(eLoopBuild);
		ImprovementTypes const eImprovement = kBuild.getImprovement();
		CvImprovementInfo const& kImprovement = GC.getInfo(eImprovement);
		// <!-- custom: Ordinary Farm/Cottage/Mine/etc. choices encode city specialization rather than intrinsic geography.
		// Only project an improvement when XML explicitly says that it connects this bonus, making the resource improvement a comparatively unambiguous part of the landmass's potential. (ChatGPT-5.6-Sol) -->
		if (!kImprovement.isImprovementBonusMakesValid(eBonus))
			continue;
		int aiYield[NUM_YIELD_TYPES] = { 0 };
		FOR_EACH_ENUM(Yield)
		{
			int iYield = kPlot.calculateNatureYield(eLoopYield, NO_TEAM, kPlot.getFeatureType() != NO_FEATURE && kBuild.isFeatureRemove(kPlot.getFeatureType()));
			iYield += GC.getInfo(eBonus).getYieldChange(eLoopYield);
			iYield += kImprovement.getYieldChange(eLoopYield);
			if (kPlot.isRiverSide())
				iYield += kImprovement.getRiverSideYieldChange(eLoopYield);
			if (kPlot.isHills())
				iYield += kImprovement.getHillsYieldChange(eLoopYield);
			if (bIrrigationPotential)
				iYield += kImprovement.getIrrigatedYieldChange(eLoopYield);
			iYield += getSASGameRecordImprovementTechYieldByEra(eImprovement, eLoopYield, eEra);
			iYield += kImprovement.getImprovementBonusYield(eBonus, eLoopYield);
			aiYield[eLoopYield] = std::max(0, iYield);
		}
		iPotentialFood = std::max(iPotentialFood, aiYield[YIELD_FOOD]);
		iPotentialProduction = std::max(iPotentialProduction, aiYield[YIELD_PRODUCTION]);
		iPotentialCommerce = std::max(iPotentialCommerce, aiYield[YIELD_COMMERCE]);
		iPotential321 = std::max(iPotential321, 3 * aiYield[YIELD_FOOD] + 2 * aiYield[YIELD_PRODUCTION] + aiYield[YIELD_COMMERCE]);
	}
}

static CvString getSASGameRecordEraLandmassYieldList(SASGameRecordLandmassGeography const& kLandmass, bool bBonusPotential)
{
	CvString szList;
	for (int iEra = 0; iEra < GC.getNumEraInfos(); iEra++)
	{
		if (kLandmass.iPlots <= 0)
			continue;
		std::vector<int> const& aiFood = (bBonusPotential ? kLandmass.aiEraBonusPotentialFood : kLandmass.aiEraRevealedRawFood);
		std::vector<int> const& aiProduction = (bBonusPotential ? kLandmass.aiEraBonusPotentialProduction : kLandmass.aiEraRevealedRawProduction);
		std::vector<int> const& aiCommerce = (bBonusPotential ? kLandmass.aiEraBonusPotentialCommerce : kLandmass.aiEraRevealedRawCommerce);
		std::vector<int> const& ai321 = (bBonusPotential ? kLandmass.aiEraBonusPotential321 : kLandmass.aiEraRevealedRaw321);
		CvString szItem;
		szItem.Format(szList.empty() ? "%s:%d/%d/%d/%d" : ",%s:%d/%d/%d/%d", GC.getInfo((EraTypes)iEra).getType(), (100 * aiFood[iEra]) / kLandmass.iPlots, (100 * aiProduction[iEra]) / kLandmass.iPlots, (100 * aiCommerce[iEra]) / kLandmass.iPlots, (100 * ai321[iEra]) / kLandmass.iPlots);
		szList += szItem;
	}
	return getSASDiagnosticOrDash(szList);
}

static CvString getSASGameRecordLandmassBonusTypes(SASGameRecordLandmassGeography const& kLandmass)
{
	CvString szTypes;
	for (int iBonus = 0; iBonus < (int)kLandmass.aiBonusCounts.size(); iBonus++)
	{
		if (kLandmass.aiBonusCounts[iBonus] <= 0)
			continue;
		appendSASGameRecordTypeCount(szTypes, getSASGameRecordBonusType((BonusTypes)iBonus), kLandmass.aiBonusCounts[iBonus]);
	}
	return getSASDiagnosticOrDash(szTypes);
}

static void logSASGameRecordLandmassBonusCoordinates(SASGameRecordLandmassGeography const& kLandmass, CvString const& szLandmassName)
{
	std::vector<CvString> aszParts;
	CvString szPart;
	for (size_t iI = 0; iI < kLandmass.aszBonusCoordinates.size(); iI++)
	{
		CvString const& szItem = kLandmass.aszBonusCoordinates[iI];
		if (!szPart.empty() && szPart.length() + szItem.length() + 1 > 1200)
		{
			aszParts.push_back(szPart);
			szPart = "";
		}
		if (!szPart.empty())
			szPart += ";";
		szPart += szItem;
	}
	if (!szPart.empty())
		aszParts.push_back(szPart);
	if (aszParts.empty())
	{
		logSASGameRecord("GAME_RECORD_LANDMASS_BONUS_COORDS turn=%d landmass=%s part=1 parts=1 bonuses=-", GC.getGame().getGameTurn(), szLandmassName.GetCString());
		return;
	}
	for (size_t iI = 0; iI < aszParts.size(); iI++)
		logSASGameRecord("GAME_RECORD_LANDMASS_BONUS_COORDS turn=%d landmass=%s part=%d parts=%d bonuses=%s",
			GC.getGame().getGameTurn(), szLandmassName.GetCString(), (int)iI + 1, (int)aszParts.size(), aszParts[iI].GetCString());
}

// <!-- custom: Record a map-geography snapshot once per new/load context so autoplay analysis can distinguish crowded continents, isolated islands, coast-connected landmasses, terrain/feature composition, lake/sea structure, map-resource distribution, and underlying land quality without reconstructing the map from later city history.
// Landmass names use a deterministic anchor coordinate plus the Civ4 area ID; center coordinates are wrap-aware approximations. "habitable" uses CvPlot::isHabitable, while impassable and zero-natural-yield plots remain separate.
// Nature yields exclude bonuses. Per-era raw value adds a bonus only from its reveal era; conservative potential improves only revealed/connectable bonus plots through an XML-valid resource improvement, leaving ordinary non-bonus specialization choices untouched.
// Routes, civics and improvement maturation remain excluded because they are not intrinsic geography. (ChatGPT-5.6-Sol); or because they are available everywhere, so counting them or not has no extra strategic information value so do not count them. -->
static void addSASGameRecordNoOceanConnections(CvMap const& kMap, TerrainTypes eOcean, bool bSkipImpassable, std::vector<SASGameRecordLandmassGeography>& aLandmasses)
{
	std::vector<char> abVisited(kMap.numPlots(), 0);
	for (int iI = 0; iI < kMap.numPlots(); iI++)
	{
		CvPlot const& kStart = kMap.getPlotByIndex(iI);
		if (abVisited[iI] || !kStart.isWater() || kStart.getTerrainType() == eOcean || (bSkipImpassable && kStart.isImpassable()))
			continue;
		std::vector<int> aiQueue;
		std::vector<int> aiBorderLandAreas;
		abVisited[iI] = 1;
		aiQueue.push_back(iI);
		for (size_t iQ = 0; iQ < aiQueue.size(); iQ++)
		{
			CvPlot const& kWater = kMap.getPlotByIndex(aiQueue[iQ]);
			FOR_EACH_ADJ_PLOT(kWater)
			{
				if (!pAdj->isWater())
				{
					addSASGameRecordUniqueArea(aiBorderLandAreas, pAdj->getArea().getID());
					continue;
				}
				if (pAdj->getTerrainType() == eOcean || (bSkipImpassable && pAdj->isImpassable()))
					continue;
				int const iAdjIndex = kMap.plotNum(pAdj->getX(), pAdj->getY());
				if (iAdjIndex < 0 || abVisited[iAdjIndex])
					continue;
				abVisited[iAdjIndex] = 1;
				aiQueue.push_back(iAdjIndex);
			}
		}
		for (size_t iA = 0; iA < aiBorderLandAreas.size(); iA++)
		{
			SASGameRecordLandmassGeography* pLandmass = getSASGameRecordLandmassGeography(aLandmasses, aiBorderLandAreas[iA]);
			if (pLandmass == NULL)
				continue;
			for (size_t iB = 0; iB < aiBorderLandAreas.size(); iB++)
			{
				if (iA == iB)
					continue;
				if (bSkipImpassable)
					addSASGameRecordUniqueArea(pLandmass->aiNoOceanNavigableConnectedAreas, aiBorderLandAreas[iB]);
				else addSASGameRecordUniqueArea(pLandmass->aiNoOceanConnectedAreas, aiBorderLandAreas[iB]);
			}
		}
	}
}

static void logSASGameRecordGeography()
{
	CvMap const& kMap = GC.getMap();
	std::vector<SASGameRecordLandmassGeography> aLandmasses;
	int iLandPlots = 0;
	int iWaterPlots = 0;
	int iSeaPlots = 0;
	int iLakePlots = 0;
	int iCoastSeaPlots = 0;
	int iOceanSeaPlots = 0;
	int iOtherSeaPlots = 0;
	int iIcePlots = 0;
	int iIceSeaPlots = 0;
	int iIceLakePlots = 0;
	int iIceCoastSeaPlots = 0;
	int iIceOceanSeaPlots = 0;
	int iWaterAreas = 0;
	int iSeaAreas = 0;
	int iLakeAreas = 0;
	TerrainTypes const eCoast = (TerrainTypes)GC.getInfoTypeForString("TERRAIN_COAST");
	TerrainTypes const eOcean = (TerrainTypes)GC.getInfoTypeForString("TERRAIN_OCEAN");
	FeatureTypes const eIce = (FeatureTypes)GC.getInfoTypeForString("FEATURE_ICE");
	int iLoop = 0;
	for (CvArea const* pLoopArea = kMap.firstArea(&iLoop); pLoopArea != NULL; pLoopArea = kMap.nextArea(&iLoop))
	{
		if (pLoopArea->isWater())
		{
			iWaterAreas++;
			if (pLoopArea->isLake())
				iLakeAreas++;
			else iSeaAreas++;
			continue;
		}
		SASGameRecordLandmassGeography kLandmass;
		kLandmass.iAreaId = pLoopArea->getID();
		kLandmass.aiTerrainCounts.assign(GC.getNumTerrainInfos(), 0);
		kLandmass.aiFeatureCounts.assign(GC.getNumFeatureInfos(), 0);
		kLandmass.aiBonusCounts.assign(GC.getNumBonusInfos(), 0);
		kLandmass.aiEraRevealedRawFood.assign(GC.getNumEraInfos(), 0);
		kLandmass.aiEraRevealedRawProduction.assign(GC.getNumEraInfos(), 0);
		kLandmass.aiEraRevealedRawCommerce.assign(GC.getNumEraInfos(), 0);
		kLandmass.aiEraRevealedRaw321.assign(GC.getNumEraInfos(), 0);
		kLandmass.aiEraBonusPotentialFood.assign(GC.getNumEraInfos(), 0);
		kLandmass.aiEraBonusPotentialProduction.assign(GC.getNumEraInfos(), 0);
		kLandmass.aiEraBonusPotentialCommerce.assign(GC.getNumEraInfos(), 0);
		kLandmass.aiEraBonusPotential321.assign(GC.getNumEraInfos(), 0);
		aLandmasses.push_back(kLandmass);
	}
	for (int iI = 0; iI < kMap.numPlots(); iI++)
	{
		CvPlot const& kPlot = kMap.getPlotByIndex(iI);
		if (kPlot.isWater())
		{
			iWaterPlots++;
			bool const bIce = (eIce != NO_FEATURE && kPlot.getFeatureType() == eIce);
			if (bIce)
				iIcePlots++;
			if (kPlot.isLake())
			{
				iLakePlots++;
				if (bIce)
					iIceLakePlots++;
				FOR_EACH_ADJ_PLOT(kPlot)
				{
					if (pAdj->isWater())
						continue;
					SASGameRecordLandmassGeography* pAdjacentLandmass = getSASGameRecordLandmassGeography(aLandmasses, pAdj->getArea().getID());
					if (pAdjacentLandmass != NULL)
						addSASGameRecordUniqueArea(pAdjacentLandmass->aiAdjacentLakeAreas, kPlot.getArea().getID());
				}
			}
			else
			{
				iSeaPlots++;
				if (bIce)
					iIceSeaPlots++;
				if (kPlot.getTerrainType() == eCoast)
				{
					iCoastSeaPlots++;
					if (bIce)
						iIceCoastSeaPlots++;
				}
				else if (kPlot.getTerrainType() == eOcean)
				{
					iOceanSeaPlots++;
					if (bIce)
						iIceOceanSeaPlots++;
				}
				else iOtherSeaPlots++;
			}
			continue;
		}
		iLandPlots++;
		SASGameRecordLandmassGeography* pLandmass = getSASGameRecordLandmassGeography(aLandmasses, kPlot.getArea().getID());
		if (pLandmass == NULL)
			continue;
		if (pLandmass->iPlots == 0)
		{
			pLandmass->iAnchorX = kPlot.getX();
			pLandmass->iAnchorY = kPlot.getY();
		}
		pLandmass->iPlots++;
		if (kPlot.getTerrainType() != NO_TERRAIN)
			pLandmass->aiTerrainCounts[kPlot.getTerrainType()]++;
		if (kPlot.getFeatureType() != NO_FEATURE)
			pLandmass->aiFeatureCounts[kPlot.getFeatureType()]++;
		if (kPlot.isHills())
			pLandmass->iHillsPlots++;
		if (kPlot.isPeak())
			pLandmass->iPeakPlots++;
		if (kPlot.isRiverSide())
			pLandmass->iRiverSidePlots++;
		if (kPlot.isFreshWater())
			pLandmass->iFreshWaterPlots++;
		pLandmass->iSumWrappedDX += kMap.dxWrap(kPlot.getX() - pLandmass->iAnchorX);
		pLandmass->iSumWrappedDY += kMap.dyWrap(kPlot.getY() - pLandmass->iAnchorY);
		if (kPlot.isHabitable())
			pLandmass->iHabitablePlots++;
		if (kPlot.isImpassable())
			pLandmass->iImpassablePlots++;
		int const iNatureFood = kPlot.calculateNatureYield(YIELD_FOOD, NO_TEAM);
		int const iNatureProduction = kPlot.calculateNatureYield(YIELD_PRODUCTION, NO_TEAM);
		int const iNatureCommerce = kPlot.calculateNatureYield(YIELD_COMMERCE, NO_TEAM);
		if (iNatureFood + iNatureProduction + iNatureCommerce <= 0)
			pLandmass->iZeroNatureYieldPlots++;
		pLandmass->iNatureFood += iNatureFood;
		pLandmass->iNatureProduction += iNatureProduction;
		pLandmass->iNatureCommerce += iNatureCommerce;
		BonusTypes const eBonus = kPlot.getBonusType(NO_TEAM);
		if (eBonus != NO_BONUS)
		{
			pLandmass->iBonusCount++;
			if (eBonus >= 0 && eBonus < (int)pLandmass->aiBonusCounts.size())
				pLandmass->aiBonusCounts[eBonus]++;
			CvString szBonus;
			szBonus.Format("%s@%d,%d", getSASGameRecordBonusType(eBonus), kPlot.getX(), kPlot.getY());
			pLandmass->aszBonusCoordinates.push_back(szBonus);
		}
		for (int iEra = 0; iEra < GC.getNumEraInfos(); iEra++)
		{
			int iRawFood = 0;
			int iRawProduction = 0;
			int iRawCommerce = 0;
			int iRaw321 = 0;
			int iPotentialFood = 0;
			int iPotentialProduction = 0;
			int iPotentialCommerce = 0;
			int iPotential321 = 0;
			getSASGameRecordLandmassPlotPotential(kPlot, (EraTypes)iEra, iRawFood, iRawProduction, iRawCommerce, iRaw321, iPotentialFood, iPotentialProduction, iPotentialCommerce, iPotential321);
			pLandmass->aiEraRevealedRawFood[iEra] += iRawFood;
			pLandmass->aiEraRevealedRawProduction[iEra] += iRawProduction;
			pLandmass->aiEraRevealedRawCommerce[iEra] += iRawCommerce;
			pLandmass->aiEraRevealedRaw321[iEra] += iRaw321;
			pLandmass->aiEraBonusPotentialFood[iEra] += iPotentialFood;
			pLandmass->aiEraBonusPotentialProduction[iEra] += iPotentialProduction;
			pLandmass->aiEraBonusPotentialCommerce[iEra] += iPotentialCommerce;
			pLandmass->aiEraBonusPotential321[iEra] += iPotential321;
		}
		bool bBordersWater = false;
		bool bBordersSea = false;
		bool bBordersLake = false;
		bool bBordersIce = false;
		FOR_EACH_ADJ_PLOT(kPlot)
		{
			if (!pAdj->isWater())
				continue;
			bBordersWater = true;
			if (pAdj->isLake())
			{
				bBordersLake = true;
				pLandmass->iLakeBorderEdges++;
			}
			else
			{
				bBordersSea = true;
				pLandmass->iSeaBorderEdges++;
			}
			if (eIce != NO_FEATURE && pAdj->getFeatureType() == eIce)
			{
				bBordersIce = true;
				pLandmass->iIceBorderEdges++;
			}
		}
		if (bBordersWater) pLandmass->iWaterBorderPlots++;
		if (bBordersSea) pLandmass->iSeaBorderPlots++;
		if (bBordersLake) pLandmass->iLakeBorderPlots++;
		if (bBordersIce) pLandmass->iIceBorderPlots++;
	}
	int iStartingPlayers = 0;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes eLoopPlayer = (PlayerTypes)iI;
		CvPlayer const& kPlayer = GET_PLAYER(eLoopPlayer);
		CvPlot const* pStart = kPlayer.getStartingPlot();
		if (!kPlayer.isEverAlive() || kPlayer.isBarbarian() || pStart == NULL || pStart->isWater())
			continue;
		SASGameRecordLandmassGeography* pLandmass = getSASGameRecordLandmassGeography(aLandmasses, pStart->getArea().getID());
		if (pLandmass == NULL)
			continue;
		iStartingPlayers++;
		pLandmass->iStartingPlayers++;
		CvString szItem;
		szItem.Format(pLandmass->szStartingPlayers.empty() ? "%d@%d,%d" : ";%d@%d,%d", eLoopPlayer, pStart->getX(), pStart->getY());
		pLandmass->szStartingPlayers += szItem;
	}
	if (eOcean != NO_TERRAIN)
	{
		// <!-- custom: Keep terrain-only coastal connectivity for shape/topology, then separately exclude impassable water (currently Ice) to capture what ordinary pre-ocean sea units can actually traverse. (ChatGPT-5.6-Sol) -->
		addSASGameRecordNoOceanConnections(kMap, eOcean, false, aLandmasses);
		addSASGameRecordNoOceanConnections(kMap, eOcean, true, aLandmasses);
	}

	int iLargestAreaId = -1;
	int iLargestAreaPlots = 0;
	for (size_t iI = 0; iI < aLandmasses.size(); iI++)
	{
		if (aLandmasses[iI].iPlots > iLargestAreaPlots)
		{
			iLargestAreaPlots = aLandmasses[iI].iPlots;
			iLargestAreaId = aLandmasses[iI].iAreaId;
		}
	}
	CvString szLargestLandmass = "-";
	SASGameRecordLandmassGeography* pLargestLandmass = getSASGameRecordLandmassGeography(aLandmasses, iLargestAreaId);
	if (pLargestLandmass != NULL)
		szLargestLandmass = getSASGameRecordLandmassName(*pLargestLandmass);
	int const iMapPlots = iLandPlots + iWaterPlots;

	// <!-- custom: Standardize the initial crowding context that otherwise has to be reconstructed from each landmass/start list for every map comparison.
	// Start spacing uses wrap-aware plotDistance and intentionally treats every starting civilization as map occupancy; current foreign/rival context is recorded separately in periodic expansion rows so later team merges cannot rewrite the meaning of these static start metrics. (ChatGPT-5.6-Sol) -->
	int iHabitableLandPlots = 0;
	int iInhabitedLandmasses = 0;
	int iInhabitedLandPlots = 0;
	int iInhabitedHabitablePlots = 0;
	int iUninhabitedLandmasses = 0;
	int iUninhabitedLandPlots = 0;
	int iUninhabitedHabitablePlots = 0;
	int iIsolatedLandmassStarts = 0;
	for (size_t iI = 0; iI < aLandmasses.size(); iI++)
	{
		SASGameRecordLandmassGeography const& kLandmass = aLandmasses[iI];
		iHabitableLandPlots += kLandmass.iHabitablePlots;
		if (kLandmass.iStartingPlayers > 0)
		{
			iInhabitedLandmasses++;
			iInhabitedLandPlots += kLandmass.iPlots;
			iInhabitedHabitablePlots += kLandmass.iHabitablePlots;
			if (kLandmass.iStartingPlayers == 1)
				iIsolatedLandmassStarts++;
		}
		else
		{
			iUninhabitedLandmasses++;
			iUninhabitedLandPlots += kLandmass.iPlots;
			iUninhabitedHabitablePlots += kLandmass.iHabitablePlots;
		}
	}
	int iStartPairs = 0;
	int iStartPairDistance = 0;
	int iSameLandmassPairs = 0;
	int iSameLandmassPairDistance = 0;
	int iNearestStartDistance = -1;
	int iNearestStartDistanceTotal = 0;
	int iNearestStartDistancePlayers = 0;
	int iMaxNearestStartDistance = -1;
	int iNearestSameLandmassStartDistance = -1;
	int iNearestSameLandmassStartDistanceTotal = 0;
	int iNearestSameLandmassStartDistancePlayers = 0;
	int iMaxNearestSameLandmassStartDistance = -1;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes const ePlayer = (PlayerTypes)iI;
		CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
		CvPlot const* pStart = kPlayer.getStartingPlot();
		if (!kPlayer.isEverAlive() || kPlayer.isBarbarian() || pStart == NULL || pStart->isWater())
			continue;
		int iPlayerNearest = -1;
		int iPlayerNearestSameLandmass = -1;
		for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
		{
			if (iJ == iI)
				continue;
			PlayerTypes const eOtherPlayer = (PlayerTypes)iJ;
			CvPlayer const& kOtherPlayer = GET_PLAYER(eOtherPlayer);
			CvPlot const* pOtherStart = kOtherPlayer.getStartingPlot();
			if (!kOtherPlayer.isEverAlive() || kOtherPlayer.isBarbarian() || pOtherStart == NULL || pOtherStart->isWater())
				continue;
			int const iDistance = plotDistance(pStart->getX(), pStart->getY(), pOtherStart->getX(), pOtherStart->getY());
			iPlayerNearest = (iPlayerNearest < 0 ? iDistance : std::min(iPlayerNearest, iDistance));
			bool const bSameLandmass = (pOtherStart->getArea().getID() == pStart->getArea().getID());
			if (bSameLandmass)
				iPlayerNearestSameLandmass = (iPlayerNearestSameLandmass < 0 ? iDistance : std::min(iPlayerNearestSameLandmass, iDistance));
			if (iJ <= iI)
				continue;
			iStartPairs++;
			iStartPairDistance += iDistance;
			if (bSameLandmass)
			{
				iSameLandmassPairs++;
				iSameLandmassPairDistance += iDistance;
			}
		}
		if (iPlayerNearest >= 0)
		{
			iNearestStartDistance = (iNearestStartDistance < 0 ? iPlayerNearest : std::min(iNearestStartDistance, iPlayerNearest));
			iNearestStartDistanceTotal += iPlayerNearest;
			iNearestStartDistancePlayers++;
			iMaxNearestStartDistance = std::max(iMaxNearestStartDistance, iPlayerNearest);
		}
		if (iPlayerNearestSameLandmass >= 0)
		{
			iNearestSameLandmassStartDistance = (iNearestSameLandmassStartDistance < 0 ? iPlayerNearestSameLandmass : std::min(iNearestSameLandmassStartDistance, iPlayerNearestSameLandmass));
			iNearestSameLandmassStartDistanceTotal += iPlayerNearestSameLandmass;
			iNearestSameLandmassStartDistancePlayers++;
			iMaxNearestSameLandmassStartDistance = std::max(iMaxNearestSameLandmassStartDistance, iPlayerNearestSameLandmass);
		}
	}
	logSASGameRecord("GAME_RECORD_START_DENSITY_SUMMARY turn=%d startingPlayers=%d inhabitedLandmasses=%d isolatedLandmassStarts=%d uninhabitedLandmasses=%d landPlotsPerStartX100=%d habitableLandPlots=%d habitableLandPlotsPerStartX100=%d inhabitedLandPlots=%d inhabitedLandPlotsPerStartX100=%d inhabitedHabitablePlots=%d inhabitedHabitablePlotsPerStartX100=%d uninhabitedLandPlots=%d uninhabitedHabitablePlots=%d startPairs=%d avgStartPairDistanceX100=%d sameLandmassPairs=%d avgSameLandmassPairDistanceX100=%d nearestStartDistance=%d avgNearestStartDistanceX100=%d maxNearestStartDistance=%d startsWithSameLandmassNeighbor=%d nearestSameLandmassStartDistance=%d avgNearestSameLandmassStartDistanceX100=%d maxNearestSameLandmassStartDistance=%d",
		GC.getGame().getGameTurn(), iStartingPlayers, iInhabitedLandmasses, iIsolatedLandmassStarts, iUninhabitedLandmasses,
		iStartingPlayers <= 0 ? -1 : (100 * iLandPlots) / iStartingPlayers, iHabitableLandPlots,
		iStartingPlayers <= 0 ? -1 : (100 * iHabitableLandPlots) / iStartingPlayers, iInhabitedLandPlots,
		iStartingPlayers <= 0 ? -1 : (100 * iInhabitedLandPlots) / iStartingPlayers, iInhabitedHabitablePlots,
		iStartingPlayers <= 0 ? -1 : (100 * iInhabitedHabitablePlots) / iStartingPlayers, iUninhabitedLandPlots, iUninhabitedHabitablePlots,
		iStartPairs, iStartPairs <= 0 ? -1 : (100 * iStartPairDistance) / iStartPairs, iSameLandmassPairs,
		iSameLandmassPairs <= 0 ? -1 : (100 * iSameLandmassPairDistance) / iSameLandmassPairs, iNearestStartDistance,
		iNearestStartDistancePlayers <= 0 ? -1 : (100 * iNearestStartDistanceTotal) / iNearestStartDistancePlayers, iMaxNearestStartDistance,
		iNearestSameLandmassStartDistancePlayers, iNearestSameLandmassStartDistance,
		iNearestSameLandmassStartDistancePlayers <= 0 ? -1 : (100 * iNearestSameLandmassStartDistanceTotal) / iNearestSameLandmassStartDistancePlayers,
		iMaxNearestSameLandmassStartDistance);
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes const ePlayer = (PlayerTypes)iI;
		CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
		CvPlot const* pStart = kPlayer.getStartingPlot();
		if (!kPlayer.isEverAlive() || kPlayer.isBarbarian() || pStart == NULL || pStart->isWater())
			continue;
		SASGameRecordLandmassGeography const* pLandmass = getSASGameRecordLandmassGeography(aLandmasses, pStart->getArea().getID());
		if (pLandmass == NULL)
			continue;
		PlayerTypes eNearestPlayer = NO_PLAYER;
		int iNearestDistance = -1;
		PlayerTypes eNearestSameLandmassPlayer = NO_PLAYER;
		int iNearestSameLandmassDistance = -1;
		int iSameLandmassDistance = 0;
		int iSameLandmassDistancePlayers = 0;
		for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
		{
			if (iJ == iI)
				continue;
			PlayerTypes const eOtherPlayer = (PlayerTypes)iJ;
			CvPlayer const& kOtherPlayer = GET_PLAYER(eOtherPlayer);
			CvPlot const* pOtherStart = kOtherPlayer.getStartingPlot();
			if (!kOtherPlayer.isEverAlive() || kOtherPlayer.isBarbarian() || pOtherStart == NULL || pOtherStart->isWater())
				continue;
			int const iDistance = plotDistance(pStart->getX(), pStart->getY(), pOtherStart->getX(), pOtherStart->getY());
			if (iNearestDistance < 0 || iDistance < iNearestDistance)
			{
				iNearestDistance = iDistance;
				eNearestPlayer = eOtherPlayer;
			}
			if (pOtherStart->getArea().getID() != pStart->getArea().getID())
				continue;
			iSameLandmassDistance += iDistance;
			iSameLandmassDistancePlayers++;
			if (iNearestSameLandmassDistance < 0 || iDistance < iNearestSameLandmassDistance)
			{
				iNearestSameLandmassDistance = iDistance;
				eNearestSameLandmassPlayer = eOtherPlayer;
			}
		}
		CvString const szLandmass = getSASGameRecordLandmassName(*pLandmass);
		logSASGameRecord("GAME_RECORD_PLAYER_START_DENSITY turn=%d player=%d currentTeam=%d start=%d,%d landmass=%s landmassStartingPlayers=%d landmassPlots=%d landmassHabitablePlots=%d landmassPlotsPerStartX100=%d landmassHabitablePlotsPerStartX100=%d nearestStartPlayer=%d nearestStartDistance=%d nearestSameLandmassStartPlayer=%d nearestSameLandmassStartDistance=%d avgSameLandmassStartDistanceX100=%d",
			GC.getGame().getGameTurn(), ePlayer, kPlayer.getTeam(), pStart->getX(), pStart->getY(), szLandmass.GetCString(),
			pLandmass->iStartingPlayers, pLandmass->iPlots, pLandmass->iHabitablePlots,
			pLandmass->iStartingPlayers <= 0 ? -1 : (100 * pLandmass->iPlots) / pLandmass->iStartingPlayers,
			pLandmass->iStartingPlayers <= 0 ? -1 : (100 * pLandmass->iHabitablePlots) / pLandmass->iStartingPlayers, eNearestPlayer,
			iNearestDistance, eNearestSameLandmassPlayer, iNearestSameLandmassDistance,
			iSameLandmassDistancePlayers <= 0 ? -1 : (100 * iSameLandmassDistance) / iSameLandmassDistancePlayers);
	}

	logSASGameRecord("GAME_RECORD_GEOGRAPHY_SUMMARY turn=%d landmasses=%d waterAreas=%d seaAreas=%d lakeAreas=%d mapPlots=%d landPlots=%d landPercentOfMapX100=%d waterPlots=%d waterPercentOfMapX100=%d seaPlots=%d seaPercentOfMapX100=%d coastSeaPlots=%d coastSeaPercentOfMapX100=%d oceanSeaPlots=%d oceanSeaPercentOfMapX100=%d otherSeaPlots=%d lakePlots=%d lakePercentOfMapX100=%d icePlots=%d icePercentOfMapX100=%d icePercentOfWaterX100=%d iceSeaPlots=%d icePercentOfSeaX100=%d iceCoastSeaPlots=%d iceOceanSeaPlots=%d iceLakePlots=%d largestLandmass=%s largestLandmassPlots=%d wrapX=%d wrapY=%d coastTerrain=%s oceanTerrain=%s iceFeature=%s",
		GC.getGame().getGameTurn(), (int)aLandmasses.size(), iWaterAreas, iSeaAreas, iLakeAreas, iMapPlots, iLandPlots,
		getSASGameRecordPercentX100(iLandPlots, iMapPlots), iWaterPlots, getSASGameRecordPercentX100(iWaterPlots, iMapPlots), iSeaPlots,
		getSASGameRecordPercentX100(iSeaPlots, iMapPlots), iCoastSeaPlots, getSASGameRecordPercentX100(iCoastSeaPlots, iMapPlots),
		iOceanSeaPlots, getSASGameRecordPercentX100(iOceanSeaPlots, iMapPlots), iOtherSeaPlots, iLakePlots,
		getSASGameRecordPercentX100(iLakePlots, iMapPlots), iIcePlots, getSASGameRecordPercentX100(iIcePlots, iMapPlots),
		getSASGameRecordPercentX100(iIcePlots, iWaterPlots), iIceSeaPlots, getSASGameRecordPercentX100(iIceSeaPlots, iSeaPlots),
		iIceCoastSeaPlots, iIceOceanSeaPlots, iIceLakePlots, szLargestLandmass.GetCString(), iLargestAreaPlots, kMap.isWrapX(),
		kMap.isWrapY(), eCoast == NO_TERRAIN ? "-" : GC.getInfo(eCoast).getType(), eOcean == NO_TERRAIN ? "-" : GC.getInfo(eOcean).getType(),
		eIce == NO_FEATURE ? "-" : GC.getInfo(eIce).getType());
	logSASGameRecord("GAME_RECORD_GEOGRAPHY_YIELD_LEGEND nature=terrain_feature_hills_river_without_bonus revealedRaw=nature_plus_bonus_base_yield_from_reveal_era bonusImprovedPotential=revealedRaw_plus_only_XML_valid_bonus_improvement_after_reveal_connection_and_build_tech nonBonusPlots=never_improvement_optimized routesCivicsMaturation=excluded potentialIrrigation=allowed_on_valid_bonus_improvement improvementTechYieldChanges=included score321=3F+2H+C averageScale=100 eraFormat=ERA:F/H/C/321");
	logSASGameRecord("GAME_RECORD_GEOGRAPHY_COMPOSITION_LEGEND percentScale=100 terrainPercentDenominator=landmassPlots featurePercentDenominator=landmassPlots featurelessPercent=plots_without_feature waterBorderLand=land_plots_adjacent_to_any_water seaBorderLand=land_plots_adjacent_to_nonlake_water lakeBorderLand=land_plots_adjacent_to_lake_water borderEdgesPerLandX100=adjacent_land_water_edges_per_land_plot_times_100 iceBorderLand=land_plots_adjacent_to_FEATURE_ICE adjacentLakePlots=unique_lake_area_tiles_touching_landmass adjacentLakePerLandPercentDenominator=landmassPlots noOceanConnected=non_ocean_water_topology_ignoring_impassable_features noOceanNavigable=non_ocean_water_excluding_impassable_plots sea=water_excluding_lakes coastSea=nonlake_TERRAIN_COAST oceanSea=nonlake_TERRAIN_OCEAN");
	for (size_t iI = 0; iI < aLandmasses.size(); iI++)
	{
		SASGameRecordLandmassGeography const& kLandmass = aLandmasses[iI];
		if (kLandmass.iPlots <= 0)
			continue;
		int iCenterX100 = kLandmass.iAnchorX * 100 + (100 * kLandmass.iSumWrappedDX) / kLandmass.iPlots;
		int iCenterY100 = kLandmass.iAnchorY * 100 + (100 * kLandmass.iSumWrappedDY) / kLandmass.iPlots;
		if (kMap.isWrapX())
		{
			int const iWidthX100 = kMap.getGridWidth() * 100;
			while (iCenterX100 < 0) iCenterX100 += iWidthX100;
			while (iCenterX100 >= iWidthX100) iCenterX100 -= iWidthX100;
		}
		if (kMap.isWrapY())
		{
			int const iHeightX100 = kMap.getGridHeight() * 100;
			while (iCenterY100 < 0) iCenterY100 += iHeightX100;
			while (iCenterY100 >= iHeightX100) iCenterY100 -= iHeightX100;
		}
		CvString const szName = getSASGameRecordLandmassName(kLandmass);
		logSASGameRecord("GAME_RECORD_LANDMASS turn=%d landmass=%s area=%d anchor=%d,%d centerX100=%d centerY100=%d plots=%d landSharePercentX100=%d habitablePlots=%d nonHabitablePlots=%d impassablePlots=%d zeroNatureYieldPlots=%d waterBorderPlots=%d seaBorderPlots=%d lakeBorderPlots=%d startingPlayers=%d starts=%s noOceanConnectedCount=%d noOceanConnectedTo=%s",
			GC.getGame().getGameTurn(), szName.GetCString(), kLandmass.iAreaId, kLandmass.iAnchorX, kLandmass.iAnchorY, iCenterX100,
			iCenterY100, kLandmass.iPlots, getSASGameRecordPercentX100(kLandmass.iPlots, iLandPlots), kLandmass.iHabitablePlots,
			kLandmass.iPlots - kLandmass.iHabitablePlots, kLandmass.iImpassablePlots, kLandmass.iZeroNatureYieldPlots,
			kLandmass.iWaterBorderPlots, kLandmass.iSeaBorderPlots, kLandmass.iLakeBorderPlots, kLandmass.iStartingPlayers,
			getSASDiagnosticOrDash(kLandmass.szStartingPlayers).GetCString(), (int)kLandmass.aiNoOceanConnectedAreas.size(),
			getSASGameRecordLandmassList(aLandmasses, kLandmass.aiNoOceanConnectedAreas).GetCString());
		std::vector<int> aiIceBlockedNoOceanAreas;
		for (size_t iConnection = 0; iConnection < kLandmass.aiNoOceanConnectedAreas.size(); iConnection++)
		{
			int const iAreaId = kLandmass.aiNoOceanConnectedAreas[iConnection];
			if (std::find(kLandmass.aiNoOceanNavigableConnectedAreas.begin(), kLandmass.aiNoOceanNavigableConnectedAreas.end(), iAreaId) == kLandmass.aiNoOceanNavigableConnectedAreas.end())
				aiIceBlockedNoOceanAreas.push_back(iAreaId);
		}
		logSASGameRecord("GAME_RECORD_LANDMASS_NAVIGATION turn=%d landmass=%s noOceanNavigableConnectedCount=%d iceBlockedNoOceanCount=%d iceBlockedNoOceanTo=%s",
			GC.getGame().getGameTurn(), szName.GetCString(), (int)kLandmass.aiNoOceanNavigableConnectedAreas.size(),
			(int)aiIceBlockedNoOceanAreas.size(), getSASGameRecordLandmassList(aLandmasses, aiIceBlockedNoOceanAreas).GetCString());
		int iFeaturePlots = 0;
		for (size_t iFeature = 0; iFeature < kLandmass.aiFeatureCounts.size(); iFeature++)
			iFeaturePlots += kLandmass.aiFeatureCounts[iFeature];
		int const iAdjacentLakePlots = getSASGameRecordAdjacentLakePlots(kLandmass);
		logSASGameRecord("GAME_RECORD_LANDMASS_COMPOSITION turn=%d landmass=%s hillsPercentX100=%d peakPercentX100=%d riverSidePercentX100=%d freshWaterPercentX100=%d waterBorderLandPercentX100=%d seaBorderLandPercentX100=%d lakeBorderLandPercentX100=%d iceBorderLandPercentX100=%d seaBorderEdgesPerLandX100=%d lakeBorderEdgesPerLandX100=%d iceBorderEdgesPerLandX100=%d featurelessPercentX100=%d adjacentLakePlots=%d adjacentLakePerLandPercentX100=%d terrainPercentX100=%s featurePercentX100=%s",
			GC.getGame().getGameTurn(), szName.GetCString(), getSASGameRecordPercentX100(kLandmass.iHillsPlots, kLandmass.iPlots),
			getSASGameRecordPercentX100(kLandmass.iPeakPlots, kLandmass.iPlots),
			getSASGameRecordPercentX100(kLandmass.iRiverSidePlots, kLandmass.iPlots),
			getSASGameRecordPercentX100(kLandmass.iFreshWaterPlots, kLandmass.iPlots),
			getSASGameRecordPercentX100(kLandmass.iWaterBorderPlots, kLandmass.iPlots),
			getSASGameRecordPercentX100(kLandmass.iSeaBorderPlots, kLandmass.iPlots),
			getSASGameRecordPercentX100(kLandmass.iLakeBorderPlots, kLandmass.iPlots),
			getSASGameRecordPercentX100(kLandmass.iIceBorderPlots, kLandmass.iPlots), (100 * kLandmass.iSeaBorderEdges) / kLandmass.iPlots,
			(100 * kLandmass.iLakeBorderEdges) / kLandmass.iPlots, (100 * kLandmass.iIceBorderEdges) / kLandmass.iPlots,
			getSASGameRecordPercentX100(kLandmass.iPlots - iFeaturePlots, kLandmass.iPlots), iAdjacentLakePlots,
			getSASGameRecordPercentX100(iAdjacentLakePlots, kLandmass.iPlots),
			getSASGameRecordLandmassTerrainPercentages(kLandmass).GetCString(),
			getSASGameRecordLandmassFeaturePercentages(kLandmass).GetCString());
		int const iFinalEra = GC.getNumEraInfos() - 1;
		int const iNature321 = 3 * kLandmass.iNatureFood + 2 * kLandmass.iNatureProduction + kLandmass.iNatureCommerce;
		logSASGameRecord("GAME_RECORD_LANDMASS_YIELDS turn=%d landmass=%s natureAvgF100=%d natureAvgH100=%d natureAvgC100=%d natureAvg321X100=%d revealedRawFinalAvgF100=%d revealedRawFinalAvgH100=%d revealedRawFinalAvgC100=%d revealedRawFinalAvg321X100=%d bonusImprovedPotentialFinalAvgF100=%d bonusImprovedPotentialFinalAvgH100=%d bonusImprovedPotentialFinalAvgC100=%d bonusImprovedPotentialFinalAvg321X100=%d eraRevealedRawAvgFHC321X100=%s eraBonusImprovedPotentialAvgFHC321X100=%s",
			GC.getGame().getGameTurn(), szName.GetCString(), (100 * kLandmass.iNatureFood) / kLandmass.iPlots,
			(100 * kLandmass.iNatureProduction) / kLandmass.iPlots, (100 * kLandmass.iNatureCommerce) / kLandmass.iPlots,
			(100 * iNature321) / kLandmass.iPlots, iFinalEra < 0 ? 0 : (100 * kLandmass.aiEraRevealedRawFood[iFinalEra]) / kLandmass.iPlots,
			iFinalEra < 0 ? 0 : (100 * kLandmass.aiEraRevealedRawProduction[iFinalEra]) / kLandmass.iPlots,
			iFinalEra < 0 ? 0 : (100 * kLandmass.aiEraRevealedRawCommerce[iFinalEra]) / kLandmass.iPlots,
			iFinalEra < 0 ? 0 : (100 * kLandmass.aiEraRevealedRaw321[iFinalEra]) / kLandmass.iPlots,
			iFinalEra < 0 ? 0 : (100 * kLandmass.aiEraBonusPotentialFood[iFinalEra]) / kLandmass.iPlots,
			iFinalEra < 0 ? 0 : (100 * kLandmass.aiEraBonusPotentialProduction[iFinalEra]) / kLandmass.iPlots,
			iFinalEra < 0 ? 0 : (100 * kLandmass.aiEraBonusPotentialCommerce[iFinalEra]) / kLandmass.iPlots,
			iFinalEra < 0 ? 0 : (100 * kLandmass.aiEraBonusPotential321[iFinalEra]) / kLandmass.iPlots,
			getSASGameRecordEraLandmassYieldList(kLandmass, false).GetCString(),
			getSASGameRecordEraLandmassYieldList(kLandmass, true).GetCString());
		logSASGameRecord("GAME_RECORD_LANDMASS_BONUS_SUMMARY turn=%d landmass=%s bonusCount=%d types=%s",
			GC.getGame().getGameTurn(), szName.GetCString(), kLandmass.iBonusCount,
			getSASGameRecordLandmassBonusTypes(kLandmass).GetCString());
		logSASGameRecordLandmassBonusCoordinates(kLandmass, szName);
	}
}

enum
{
	SAS_MAP_ASCII_GEOGRAPHY_SYMBOL_COUNT = 9,
	SAS_MAP_ASCII_RIVER_SYMBOL_COUNT = 6,
	SAS_MAP_ASCII_POLITICAL_SYMBOL_COUNT = 8
};

// <!-- custom: Parse and cache all ASCII-map symbols and related terrain-case settings together once per DLL session; callers therefore do not need separate static define caches.
// Keep each layer's palette independent. Ice deliberately has separately configurable Geography, Terrain-override, and Features symbols so a mod-mod can choose how it reads in each picture rather than inheriting one presentation everywhere. (GPT-5.6-Sol) -->
struct SASGameRecordMapAsciiPalette
{
	bool bValid;
	char acGeography[SAS_MAP_ASCII_GEOGRAPHY_SYMBOL_COUNT];
	char acRivers[SAS_MAP_ASCII_RIVER_SYMBOL_COUNT];
	char acPolitical[SAS_MAP_ASCII_POLITICAL_SYMBOL_COUNT];
	std::vector<char> acTerrain;
	std::vector<char> acFeatures;
	std::vector<char> acBonuses;
	int iTerrainUppercaseMinNatureFoodSurplus;
	int iFoodPerPopulation;
	CvString szPlayers;
	CvString szGeographyDefine;
	CvString szRiverDefine;
	CvString szPoliticalDefine;
	CvString szTerrainDefine;
	CvString szFeatureDefine;
	CvString szBonusDefine;
	CvString szError;
};

static bool parseSASGameRecordMapAsciiSymbol(CvString const& szToken, char& cSymbol)
{
	if (szToken == "SPACE") cSymbol = ' ';
	else if (szToken == "COMMA") cSymbol = ',';
	else if (szToken == "COLON") cSymbol = ':';
	else if (szToken == "SEMICOLON") cSymbol = ';';
	else if (szToken == "EQUALS") cSymbol = '=';
	else if (szToken.length() == 1) cSymbol = szToken[0];
	else return false;
	return (cSymbol >= 32 && cSymbol <= 126 && cSymbol != '"' && cSymbol != '\\' && cSymbol != '|');
}

static bool parseSASGameRecordMapAsciiPaletteDefine(char const* szDefineName, int iExpectedSymbols, char* acSymbols, CvString& szRaw, CvString& szError)
{
	szRaw = GC.getDefineSTRING(szDefineName);
	std::vector<CvString> aszTokens;
	szRaw.getTokens(",", aszTokens);
	if ((int)aszTokens.size() != iExpectedSymbols)
	{
		szError.Format("%s requires %d comma-separated symbols but has %d", szDefineName, iExpectedSymbols, (int)aszTokens.size());
		return false;
	}
	bool abUsed[127] = { false };
	for (int iSymbol = 0; iSymbol < iExpectedSymbols; iSymbol++)
	{
		if (!parseSASGameRecordMapAsciiSymbol(aszTokens[iSymbol], acSymbols[iSymbol]))
		{
			szError.Format("%s symbol %d must be one safe printable character or SPACE/COMMA/COLON/SEMICOLON/EQUALS", szDefineName, iSymbol);
			return false;
		}
		unsigned char const ucSymbol = (unsigned char)acSymbols[iSymbol];
		if (abUsed[ucSymbol])
		{
			szError.Format("%s repeats symbol ASCII %d", szDefineName, (int)ucSymbol);
			return false;
		}
		abUsed[ucSymbol] = true;
	}
	return true;
}

static char getSASGameRecordMapAsciiUppercaseSymbol(char cSymbol)
{
	return (cSymbol >= 'a' && cSymbol <= 'z' ? cSymbol - 'a' + 'A' : cSymbol);
}

static SASGameRecordMapAsciiPalette const& getSASGameRecordMapAsciiPalette()
{
	static SASGameRecordMapAsciiPalette kPalette;
	static bool bInitialized = false;
	if (bInitialized)
		return kPalette;
	bInitialized = true;
	kPalette.bValid = false;
	kPalette.acTerrain.resize(GC.getNumTerrainInfos() + 1);
	kPalette.acFeatures.resize(GC.getNumFeatureInfos() + 2);
	kPalette.acBonuses.resize(GC.getNumBonusInfos() + 3);
	kPalette.iTerrainUppercaseMinNatureFoodSurplus = GC.getDefineINT("SAS_GAME_RECORD_MAP_ASCII_TERRAIN_UPPERCASE_MIN_NATURE_FOOD_SURPLUS");
	kPalette.iFoodPerPopulation = GC.getFOOD_CONSUMPTION_PER_POPULATION();
	if (!parseSASGameRecordMapAsciiPaletteDefine("SAS_GAME_RECORD_MAP_ASCII_GEOGRAPHY_SYMBOLS", SAS_MAP_ASCII_GEOGRAPHY_SYMBOL_COUNT, kPalette.acGeography, kPalette.szGeographyDefine, kPalette.szError) ||
			!parseSASGameRecordMapAsciiPaletteDefine("SAS_GAME_RECORD_MAP_ASCII_RIVER_SYMBOLS", SAS_MAP_ASCII_RIVER_SYMBOL_COUNT, kPalette.acRivers, kPalette.szRiverDefine, kPalette.szError) ||
			!parseSASGameRecordMapAsciiPaletteDefine("SAS_GAME_RECORD_MAP_ASCII_POLITICAL_SYMBOLS", SAS_MAP_ASCII_POLITICAL_SYMBOL_COUNT, kPalette.acPolitical, kPalette.szPoliticalDefine, kPalette.szError) ||
			!parseSASGameRecordMapAsciiPaletteDefine("SAS_GAME_RECORD_MAP_ASCII_TERRAIN_SYMBOLS", (int)kPalette.acTerrain.size(), &kPalette.acTerrain[0], kPalette.szTerrainDefine, kPalette.szError) ||
			!parseSASGameRecordMapAsciiPaletteDefine("SAS_GAME_RECORD_MAP_ASCII_FEATURE_SYMBOLS", (int)kPalette.acFeatures.size(), &kPalette.acFeatures[0], kPalette.szFeatureDefine, kPalette.szError) ||
			!parseSASGameRecordMapAsciiPaletteDefine("SAS_GAME_RECORD_MAP_ASCII_BONUS_SYMBOLS", (int)kPalette.acBonuses.size(), &kPalette.acBonuses[0], kPalette.szBonusDefine, kPalette.szError))
		return kPalette;
	// <!-- custom: Uppercase is derived rather than configured separately.
	// Reject a base palette that already uses one of those derived symbols for another terrain, which would make the food distinction ambiguous. (GPT-5.6-Sol) -->
	for (int iTerrain = 0; iTerrain < GC.getNumTerrainInfos(); iTerrain++)
	{
		char const cUppercase = getSASGameRecordMapAsciiUppercaseSymbol(kPalette.acTerrain[iTerrain]);
		if (cUppercase == kPalette.acTerrain[iTerrain]) continue;
		for (int iSymbol = 0; iSymbol < (int)kPalette.acTerrain.size(); iSymbol++)
		{
			if (iSymbol == iTerrain || cUppercase != kPalette.acTerrain[iSymbol]) continue;
			kPalette.szError.Format("SAS_GAME_RECORD_MAP_ASCII_TERRAIN_SYMBOLS derived uppercase symbol %c for terrain %d reuses symbol %d", cUppercase, iTerrain, iSymbol);
			return kPalette;
		}
	}
	kPalette.szPlayers = GC.getDefineSTRING("SAS_GAME_RECORD_MAP_ASCII_PLAYER_SYMBOLS");
	if ((int)kPalette.szPlayers.length() != MAX_CIV_PLAYERS)
	{
		kPalette.szError.Format("SAS_GAME_RECORD_MAP_ASCII_PLAYER_SYMBOLS requires exactly MAX_CIV_PLAYERS=%d characters but has %d", MAX_CIV_PLAYERS, (int)kPalette.szPlayers.length());
		return kPalette;
	}
	bool abPlayerSymbols[127] = { false };
	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
	{
		unsigned char const ucSymbol = (unsigned char)kPalette.szPlayers[iPlayer];
		if (ucSymbol < 33 || ucSymbol > 126 || ucSymbol == '"' || ucSymbol == '\\' || ucSymbol == '|' || ucSymbol == ',' || ucSymbol == ';' || ucSymbol == '=')
		{
			kPalette.szError.Format("SAS_GAME_RECORD_MAP_ASCII_PLAYER_SYMBOLS player %d uses unsafe ASCII %d", iPlayer, (int)ucSymbol);
			return kPalette;
		}
		if (abPlayerSymbols[ucSymbol])
		{
			kPalette.szError.Format("SAS_GAME_RECORD_MAP_ASCII_PLAYER_SYMBOLS repeats symbol ASCII %d", (int)ucSymbol);
			return kPalette;
		}
		// <!-- custom: Separate layers may reuse intuitive symbols; only political symbols share a picture with player symbols. (GPT-5.6-Sol) -->
		for (int iSymbol = 0; iSymbol < SAS_MAP_ASCII_POLITICAL_SYMBOL_COUNT; iSymbol++)
		{
			if (ucSymbol == (unsigned char)kPalette.acPolitical[iSymbol])
			{
				kPalette.szError.Format("SAS_GAME_RECORD_MAP_ASCII_PLAYER_SYMBOLS player %d reuses political symbol ASCII %d", iPlayer, (int)ucSymbol);
				return kPalette;
			}
		}
		abPlayerSymbols[ucSymbol] = true;
	}
	kPalette.bValid = true;
	return kPalette;
}

static char getSASGameRecordMapAsciiPlayerSymbol(PlayerTypes ePlayer, SASGameRecordMapAsciiPalette const& kPalette)
{
	int const iPlayer = (int)ePlayer;
	return (iPlayer >= 0 && iPlayer < MAX_CIV_PLAYERS ? kPalette.szPlayers[iPlayer] : '?');
}

static void appendSASGameRecordMapAsciiSymbol(CvString& szRow, char cSymbol)
{
	char acSymbol[2] = { cSymbol, '\0' };
	szRow += acSymbol;
}

static CvString getSASGameRecordMapAsciiSymbolToken(char cSymbol)
{
	if (cSymbol == ' ') return "SPACE";
	if (cSymbol == ',') return "COMMA";
	if (cSymbol == ':') return "COLON";
	if (cSymbol == ';') return "SEMICOLON";
	if (cSymbol == '=') return "EQUALS";
	CvString szToken;
	szToken.Format("%c", cSymbol);
	return szToken;
}

static void appendSASGameRecordMapAsciiLegendEntry(CvString& szLegend, char cSymbol, char const* szType)
{
	CvString szEntry;
	szEntry.Format("%s=%s", getSASGameRecordMapAsciiSymbolToken(cSymbol).GetCString(), szType);
	if (!szLegend.empty()) szLegend += ";";
	szLegend += szEntry;
}

static void appendSASGameRecordMapAsciiRuntimeTypeLegendEntry(CvString& szLegend, char cSymbol, int iType, char const* szType)
{
	// <!-- custom: Runtime XML order can differ in another mod, so record the numeric ID with the stable type name and readable map symbol. (GPT-5.6-Sol) -->
	CvString szIdAndType;
	szIdAndType.Format("%d:%s", iType, szType);
	appendSASGameRecordMapAsciiLegendEntry(szLegend, cSymbol, szIdAndType.GetCString());
}

static CvString getSASGameRecordMapAsciiSymbolCounts(int const aiCounts[127])
{
	CvString szCounts;
	for (int iSymbol = 32; iSymbol <= 126; iSymbol++)
	{
		if (aiCounts[iSymbol] <= 0) continue;
		CvString szEntry;
		szEntry.Format("%s=%d", getSASGameRecordMapAsciiSymbolToken((char)iSymbol).GetCString(), aiCounts[iSymbol]);
		if (!szCounts.empty()) szCounts += ";";
		szCounts += szEntry;
	}
	return szCounts;
}

static CvString getSASGameRecordMapAsciiTerrainLegend(SASGameRecordMapAsciiPalette const& kPalette)
{
	CvString szLegend;
	for (int iTerrain = 0; iTerrain < GC.getNumTerrainInfos(); iTerrain++)
		appendSASGameRecordMapAsciiRuntimeTypeLegendEntry(szLegend, kPalette.acTerrain[iTerrain], iTerrain, getSASGameRecordTerrainType((TerrainTypes)iTerrain));
	appendSASGameRecordMapAsciiLegendEntry(szLegend, kPalette.acTerrain[GC.getNumTerrainInfos()], "FEATURE_ICE_OVERRIDE");
	return szLegend;
}

static CvString getSASGameRecordMapAsciiGeographyLegend(SASGameRecordMapAsciiPalette const& kPalette)
{
	static char const* const aszTypes[SAS_MAP_ASCII_GEOGRAPHY_SYMBOL_COUNT] = { "WATER", "LAKE", "FLAT_OR_MOSTLY_FLAT_LAND", "MOSTLY_HILLS", "MOSTLY_PEAKS", "MOSTLY_WATER_MIXED_LAND", "BALANCED_WATER_LAND", "MOSTLY_LAND_MIXED_WATER", "ICE" };
	CvString szLegend;
	for (int iType = 0; iType < SAS_MAP_ASCII_GEOGRAPHY_SYMBOL_COUNT; iType++)
		appendSASGameRecordMapAsciiLegendEntry(szLegend, kPalette.acGeography[iType], aszTypes[iType]);
	return szLegend;
}

static CvString getSASGameRecordMapAsciiTerrainUppercaseLegend(SASGameRecordMapAsciiPalette const& kPalette)
{
	CvString szLegend;
	for (int iTerrain = 0; iTerrain < GC.getNumTerrainInfos(); iTerrain++)
	{
		char const cUppercase = getSASGameRecordMapAsciiUppercaseSymbol(kPalette.acTerrain[iTerrain]);
		if (cUppercase != kPalette.acTerrain[iTerrain])
			appendSASGameRecordMapAsciiRuntimeTypeLegendEntry(szLegend, cUppercase, iTerrain, getSASGameRecordTerrainType((TerrainTypes)iTerrain));
	}
	if (szLegend.empty()) return "-";
	return szLegend;
}

static CvString getSASGameRecordMapAsciiRiverLegend(SASGameRecordMapAsciiPalette const& kPalette)
{
	static char const* const aszTypes[SAS_MAP_ASCII_RIVER_SYMBOL_COUNT] = { "NO_RIVER_EDGE_WATER", "NO_RIVER_EDGE_LAKE", "NO_RIVER_EDGE_LAND", "SOUTH_BOUNDARY_RIVER_EDGE", "EAST_BOUNDARY_RIVER_EDGE", "SOUTH_AND_EAST_BOUNDARY_RIVER_EDGES" };
	CvString szLegend;
	for (int iType = 0; iType < SAS_MAP_ASCII_RIVER_SYMBOL_COUNT; iType++)
		appendSASGameRecordMapAsciiLegendEntry(szLegend, kPalette.acRivers[iType], aszTypes[iType]);
	return szLegend;
}

static CvString getSASGameRecordMapAsciiFeatureLegend(SASGameRecordMapAsciiPalette const& kPalette)
{
	CvString szLegend;
	appendSASGameRecordMapAsciiLegendEntry(szLegend, kPalette.acFeatures[0], "NO_FEATURE_WATER");
	appendSASGameRecordMapAsciiLegendEntry(szLegend, kPalette.acFeatures[1], "NO_FEATURE_LAND");
	for (int iFeature = 0; iFeature < GC.getNumFeatureInfos(); iFeature++)
		appendSASGameRecordMapAsciiRuntimeTypeLegendEntry(szLegend, kPalette.acFeatures[iFeature + 2], iFeature, getSASGameRecordFeatureType((FeatureTypes)iFeature));
	return szLegend;
}

static CvString getSASGameRecordMapAsciiBonusLegend(SASGameRecordMapAsciiPalette const& kPalette)
{
	CvString szLegend;
	appendSASGameRecordMapAsciiLegendEntry(szLegend, kPalette.acBonuses[0], "NO_BONUS_WATER");
	appendSASGameRecordMapAsciiLegendEntry(szLegend, kPalette.acBonuses[1], "NO_BONUS_LAND");
	for (int iBonus = 0; iBonus < GC.getNumBonusInfos(); iBonus++)
		appendSASGameRecordMapAsciiRuntimeTypeLegendEntry(szLegend, kPalette.acBonuses[iBonus + 2], iBonus, getSASGameRecordBonusType((BonusTypes)iBonus));
	appendSASGameRecordMapAsciiLegendEntry(szLegend, kPalette.acBonuses[GC.getNumBonusInfos() + 2], "MULTIPLE_BONUS_TYPES");
	return szLegend;
}

static char getSASGameRecordMapAsciiGeographySymbol(CvMap const& kMap, SASGameRecordMapAsciiPalette const& kPalette, int iMinX, int iMaxX, int iMinY, int iMaxY)
{
	static FeatureTypes const eIce = (FeatureTypes)GC.getDefineINT("COLD_FEATURE");
	int iWater = 0;
	int iLake = 0;
	int iIce = 0;
	int iLand = 0;
	int iHills = 0;
	int iPeaks = 0;
	for (int iY = iMinY; iY < iMaxY; iY++)
	{
		for (int iX = iMinX; iX < iMaxX; iX++)
		{
			CvPlot const& kPlot = *kMap.plot(iX, iY);
			if (kPlot.isWater())
			{
				iWater++;
				if (kPlot.isLake()) iLake++;
				if (kPlot.getFeatureType() == eIce) iIce++;
			}
			else
			{
				iLand++;
				if (kPlot.isPeak()) iPeaks++;
				else if (kPlot.isHills()) iHills++;
			}
		}
	}
	// <!-- custom: Ice can form strategically important sea barriers in ordinary polar bands and in unconventional shapes on maps such as Peirce.
	// Exact previews mark every ice plot; when resampling, require an ice-majority water cell so one plot does not hide a much larger mixed area. (GPT-5.6-Sol) -->
	if (iIce > 0 && 2 * iIce >= iWater && iWater >= iLand) return kPalette.acGeography[8];
	if (iLand <= 0)
		return (iLake == iWater ? kPalette.acGeography[1] : kPalette.acGeography[0]);
	if (iWater > 0)
	{
		int const iPlots = iLand + iWater;
		if (3 * iLand < iPlots) return kPalette.acGeography[5];
		if (3 * iLand > 2 * iPlots) return kPalette.acGeography[7];
		return kPalette.acGeography[6];
	}
	if (2 * iPeaks >= iLand) return kPalette.acGeography[4];
	if (2 * iHills >= iLand) return kPalette.acGeography[3];
	return kPalette.acGeography[2];
}

static char getSASGameRecordMapAsciiTerrainSymbol(CvMap const& kMap, SASGameRecordMapAsciiPalette const& kPalette, int iMinX, int iMaxX, int iMinY, int iMaxY)
{
	static FeatureTypes const eIce = (FeatureTypes)GC.getDefineINT("COLD_FEATURE");
	static std::vector<int> aiCounts;
	static std::vector<int> aiNatureFood;
	aiCounts.assign(GC.getNumTerrainInfos(), 0);
	aiNatureFood.assign(GC.getNumTerrainInfos(), 0);
	int iIce = 0;
	for (int iY = iMinY; iY < iMaxY; iY++)
	{
		for (int iX = iMinX; iX < iMaxX; iX++)
		{
			CvPlot const& kPlot = *kMap.plot(iX, iY);
			if (kPlot.getFeatureType() == eIce) iIce++;
			TerrainTypes const eTerrain = kPlot.getTerrainType();
			if (eTerrain >= 0 && eTerrain < GC.getNumTerrainInfos())
			{
				aiCounts[eTerrain]++;
				aiNatureFood[eTerrain] += kPlot.calculateNatureYield(YIELD_FOOD, NO_TEAM);
			}
		}
	}
	// <!-- custom: Ice is technically a feature, but it is not ordinarily removable and blocks naval paths, so it reads as part of the strategic surface alongside Coast, land and peaks.
	// One character cannot show both ice and its underlying Coast/Ocean terrain; prefer ice because blocked access is the strategically decisive state, while the obscured Coast shape is normally inferable from adjacent Coast and land.
	// Show exact ice at native resolution and an ice-majority cell after resampling; geography and features retain it for their separate shape/history purposes. (GPT-5.6-Sol) -->
	int const iPlotCount = (iMaxX - iMinX) * (iMaxY - iMinY);
	if (iIce > 0 && 2 * iIce >= iPlotCount) return kPalette.acTerrain[GC.getNumTerrainInfos()];
	TerrainTypes eDominant = NO_TERRAIN;
	int iDominantCount = 0;
	for (int iTerrain = 0; iTerrain < GC.getNumTerrainInfos(); iTerrain++)
	{
		if (aiCounts[iTerrain] > iDominantCount)
		{
			eDominant = (TerrainTypes)iTerrain;
			iDominantCount = aiCounts[iTerrain];
		}
	}
	if (eDominant == NO_TERRAIN) return '?';
	char const cTerrain = kPalette.acTerrain[eDominant];
	// <!-- custom: Case adds the strategic food information that terrain identity alone loses: e.g. flat Grass versus a Grass hill, or bare Desert versus Flood Plains.
	// At reduced resolution, use the dominant terrain's average rather than letting one fertile plot capitalize a much larger poor cell. (GPT-5.6-Sol) -->
	int const iUppercaseMinNatureFood = kPalette.iFoodPerPopulation + kPalette.iTerrainUppercaseMinNatureFoodSurplus;
	if (aiNatureFood[eDominant] >= iUppercaseMinNatureFood * iDominantCount)
		return getSASGameRecordMapAsciiUppercaseSymbol(cTerrain);
	return cTerrain;
}

static char getSASGameRecordMapAsciiRiverSymbol(CvMap const& kMap, SASGameRecordMapAsciiPalette const& kPalette, int iMinX, int iMaxX, int iMinY, int iMaxY)
{
	// <!-- custom: Civ4's isNOfRiver flag means that this plot is north of a river on its southern boundary; isWOfRiver analogously records its eastern boundary.
	// These two bits therefore identify the orthogonal movement edges that cross rivers, unlike a riverside-plot marker. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	// <!-- custom: When no river edge replaces the background, blank ocean, visible lakes and dotted land keep each river attached to a recognizable landmass.
	// A distinct lake symbol avoids mistaking an inland body near the coast for an ocean inlet; the same majority rule preserves the broad outline after resampling. (GPT-5.6-Sol) -->
	bool bSouthBoundary = false;
	bool bEastBoundary = false;
	int iWater = 0;
	int iLake = 0;
	int iLand = 0;
	for (int iY = iMinY; iY < iMaxY; iY++)
	{
		for (int iX = iMinX; iX < iMaxX; iX++)
		{
			CvPlot const& kPlot = *kMap.plot(iX, iY);
			if (kPlot.isNOfRiver()) bSouthBoundary = true;
			if (kPlot.isWOfRiver()) bEastBoundary = true;
			if (kPlot.isWater())
			{
				iWater++;
				if (kPlot.isLake()) iLake++;
			}
			else iLand++;
		}
	}
	if (bSouthBoundary || bEastBoundary) return kPalette.acRivers[(bSouthBoundary && bEastBoundary) ? 5 : (bSouthBoundary ? 3 : 4)];
	if (iLand > iWater) return kPalette.acRivers[2];
	return kPalette.acRivers[iLake == iWater ? 1 : 0];
}

static char getSASGameRecordMapAsciiFeatureSymbol(CvMap const& kMap, SASGameRecordMapAsciiPalette const& kPalette, int iMinX, int iMaxX, int iMinY, int iMaxY)
{
	static std::vector<int> aiCounts;
	aiCounts.assign(GC.getNumFeatureInfos() + 2, 0);
	for (int iY = iMinY; iY < iMaxY; iY++)
	{
		for (int iX = iMinX; iX < iMaxX; iX++)
		{
			CvPlot const& kPlot = *kMap.plot(iX, iY);
			FeatureTypes const eFeature = kPlot.getFeatureType();
			aiCounts[eFeature == NO_FEATURE ? (kPlot.isWater() ? 0 : 1) : eFeature + 2]++;
		}
	}
	// <!-- custom: Distinguishing featureless land from featureless water preserves the continent silhouette, so this changing feature layer remains readable without mentally overlaying the static geography map. (GPT-5.6-Sol) -->
	int iDominant = 0;
	for (int iFeature = 1; iFeature < (int)aiCounts.size(); iFeature++)
	{
		if (aiCounts[iFeature] > aiCounts[iDominant]) iDominant = iFeature;
	}
	return kPalette.acFeatures[iDominant];
}

static char getSASGameRecordMapAsciiBonusSymbol(CvMap const& kMap, SASGameRecordMapAsciiPalette const& kPalette, int iMinX, int iMaxX, int iMinY, int iMaxY)
{
	int iNoBonusWater = 0;
	int iNoBonusLand = 0;
	BonusTypes eOnlyBonus = NO_BONUS;
	for (int iY = iMinY; iY < iMaxY; iY++)
	{
		for (int iX = iMinX; iX < iMaxX; iX++)
		{
			CvPlot const& kPlot = *kMap.plot(iX, iY);
			BonusTypes const eBonus = kPlot.getBonusType(NO_TEAM);
			if (eBonus == NO_BONUS)
			{
				if (kPlot.isWater()) iNoBonusWater++;
				else iNoBonusLand++;
			}
			else if (eOnlyBonus == NO_BONUS) eOnlyBonus = eBonus;
			else if (eOnlyBonus != eBonus) return kPalette.acBonuses[GC.getNumBonusInfos() + 2];
		}
	}
	// <!-- custom: Bonuses are sparse, so any bonus is more informative than the surrounding empty plots in a resampled cell.
	// Different bonus types collapse to the dedicated multiple marker; exact setup coordinates and later BONUS_CHANGE rows preserve every identity and location. (GPT-5.6-Sol) -->
	if (eOnlyBonus != NO_BONUS) return kPalette.acBonuses[eOnlyBonus + 2];
	return kPalette.acBonuses[iNoBonusWater >= iNoBonusLand ? 0 : 1];
}

static char getSASGameRecordMapAsciiPoliticalSymbol(CvMap const& kMap, SASGameRecordMapAsciiPalette const& kPalette, std::vector<PlayerTypes> const& aeStartingPlayers, int iMinX, int iMaxX, int iMinY, int iMaxY)
{
	// <!-- custom: Keep this layer purely political.
	// Coast or ice overriding a player symbol would hide owned water, while showing them only on unowned plots would make physical geography inconsistent as borders expand; Terrain/Geography/Features preserve that context separately. (GPT-5.6-Sol) -->
	int aiOwnerPlots[MAX_PLAYERS] = { 0 };
	PlayerTypes eStartingPlayer = NO_PLAYER;
	bool bMultipleStartingPlayers = false;
	bool bCivilizationCity = false;
	bool bBarbarianCity = false;
	int iWater = 0;
	int iLand = 0;
	for (int iY = iMinY; iY < iMaxY; iY++)
	{
		for (int iX = iMinX; iX < iMaxX; iX++)
		{
			CvPlot const& kPlot = *kMap.plot(iX, iY);
			if (kPlot.isWater()) iWater++;
			else iLand++;
			PlayerTypes const eOwner = kPlot.getOwner();
			if (eOwner >= 0 && eOwner < MAX_PLAYERS)
				aiOwnerPlots[eOwner]++;
			CvCity const* pCity = kPlot.getPlotCity();
			if (pCity != NULL)
			{
				if (pCity->isBarbarian()) bBarbarianCity = true;
				else bCivilizationCity = true;
			}
			PlayerTypes const ePlotStartingPlayer = (aeStartingPlayers.empty() ? NO_PLAYER : aeStartingPlayers[kMap.plotNum(iX, iY)]);
			if (ePlotStartingPlayer != NO_PLAYER)
			{
				if (eStartingPlayer == NO_PLAYER) eStartingPlayer = ePlotStartingPlayer;
				else if (eStartingPlayer != ePlotStartingPlayer) bMultipleStartingPlayers = true;
			}
		}
	}
	if (bCivilizationCity && bBarbarianCity) return kPalette.acPolitical[5];
	if (bBarbarianCity) return kPalette.acPolitical[4];
	if (bCivilizationCity) return kPalette.acPolitical[3];
	if (bMultipleStartingPlayers) return kPalette.acPolitical[6];
	if (eStartingPlayer != NO_PLAYER) return getSASGameRecordMapAsciiPlayerSymbol(eStartingPlayer, kPalette);
	PlayerTypes eDominantOwner = NO_PLAYER;
	int iDominantOwnerPlots = 0;
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
	{
		if (aiOwnerPlots[iPlayer] > iDominantOwnerPlots)
		{
			iDominantOwnerPlots = aiOwnerPlots[iPlayer];
			eDominantOwner = (PlayerTypes)iPlayer;
		}
	}
	if (eDominantOwner == BARBARIAN_PLAYER) return kPalette.acPolitical[7];
	if (eDominantOwner != NO_PLAYER) return getSASGameRecordMapAsciiPlayerSymbol(eDominantOwner, kPalette);
	if (iWater > 0 && iLand > 0) return kPalette.acPolitical[2];
	return (iWater > 0 ? kPalette.acPolitical[0] : kPalette.acPolitical[1]);
}

// <!-- custom: The bounded river picture can merge edges on oversized maps.
// Preserve every setup edge in compact coordinate chunks too, using S/E for the plot's south/east boundaries so an external tool can reconstruct exact orthogonal river crossings. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
static void logSASGameRecordRiverEdgeCoordinates()
{
	CvMap const& kMap = GC.getMap();
	std::vector<CvString> aszChunks;
	CvString szChunk;
	int iEdgePlots = 0;
	int iRiverEdges = 0;
	for (int iPlot = 0; iPlot < kMap.numPlots(); iPlot++)
	{
		CvPlot const& kPlot = *kMap.plotByIndex(iPlot);
		bool const bSouth = kPlot.isNOfRiver();
		bool const bEast = kPlot.isWOfRiver();
		if (!bSouth && !bEast) continue;
		char const* const szEdgeDirections = (bSouth && bEast ? "SE" : bSouth ? "S" : "E");
		iEdgePlots++;
		iRiverEdges += (bSouth ? 1 : 0) + (bEast ? 1 : 0);
		CvString szItem;
		szItem.Format("%s(%d,%d):%s", szChunk.empty() ? "" : ";", kPlot.getX(), kPlot.getY(), szEdgeDirections);
		if (!szChunk.empty() && szChunk.length() + szItem.length() > 1500)
		{
			aszChunks.push_back(szChunk);
			szChunk.clear();
			szItem.Format("(%d,%d):%s", kPlot.getX(), kPlot.getY(), szEdgeDirections);
		}
		szChunk += szItem;
	}
	if (!szChunk.empty()) aszChunks.push_back(szChunk);
	if (aszChunks.empty()) aszChunks.push_back("-");
	for (size_t iPart = 0; iPart < aszChunks.size(); iPart++)
		logSASGameRecord("GAME_RECORD_MAP_RIVER_EDGES turn=%d edgePlots=%d riverEdges=%d part=%d parts=%d edgeFormat=(x,y):D directionCodes=S/E/SE edges=%s",
			GC.getGame().getGameTurn(), iEdgePlots, iRiverEdges, (int)iPart + 1, (int)aszChunks.size(), aszChunks[iPart].GetCString());
}

// <!-- custom: Render all extra geography pictures through one direct source-grid path so overview and native symbols have identical semantics.
// The caller owns framing metadata and zero-initialized counts. (GPT-5.6-Sol) -->
static void logSASGameRecordMapAsciiGeographyRows(CvMap const& kMap, SASGameRecordMapAsciiPalette const& kPalette, int iPictureWidth, int iPictureHeight, int iHorizontalCharsPerCell, int* aiSymbolCounts)
{
	int const iSourceWidth = kMap.getGridWidth();
	int const iSourceHeight = kMap.getGridHeight();
	for (int iRow = 0; iRow < iPictureHeight; iRow++)
	{
		int const iPictureY = iPictureHeight - iRow - 1;
		int const iMinY = (iPictureY * iSourceHeight) / iPictureHeight;
		int const iMaxY = ((iPictureY + 1) * iSourceHeight) / iPictureHeight;
		CvString szPlots = "|";
		for (int iPictureX = 0; iPictureX < iPictureWidth; iPictureX++)
		{
			int const iMinX = (iPictureX * iSourceWidth) / iPictureWidth;
			int const iMaxX = ((iPictureX + 1) * iSourceWidth) / iPictureWidth;
			char const cSymbol = getSASGameRecordMapAsciiGeographySymbol(kMap, kPalette, iMinX, iMaxX, iMinY, iMaxY);
			aiSymbolCounts[(unsigned char)cSymbol]++;
			for (int iRepeat = 0; iRepeat < iHorizontalCharsPerCell; iRepeat++)
				appendSASGameRecordMapAsciiSymbol(szPlots, cSymbol);
		}
		szPlots += "|";
		logSASGameRecord("%s", szPlots.GetCString());
	}
}

// <!-- custom: The full multi-layer text map remains authoritative, but its geography picture is too large for a quick first impression. Add tunable initial overviews using the same aspect correction and geography palette.
// Derive only each overview's dimensions from the bounded full preview, then resample its cells directly from the original plot grid so the compact picture does not compound the full preview's aggregation loss. (GPT-5.6-Sol) -->
static void logSASGameRecordMapAsciiGeographyOverview(CvMap const& kMap, SASGameRecordMapAsciiPalette const& kPalette, int iFullPreviewWidth, int iFullPreviewHeight, int iHorizontalCharsPerCell, int iOverview, int iScalePercent, char const* szReason)
{
	int const iOverviewWidth = std::max(1, (iFullPreviewWidth * iScalePercent + 50) / 100);
	int const iOverviewHeight = std::max(1, (iFullPreviewHeight * iScalePercent + 50) / 100);
	int const iOutputWidth = iOverviewWidth * iHorizontalCharsPerCell;
	int aiSymbolCounts[127] = { 0 };
	logSASGameRecord("GAME_RECORD_MAP_ASCII_OVERVIEW_BEGIN turn=%d reason=%s overview=%d layer=GEOGRAPHY source=%dx%d fullPreviewCells=%dx%d overviewCells=%dx%d outputCharacters=%dx%d scalePercent=%d scaleBasis=bounded_full_preview resampledDirectlyFromSource=1 horizontalCharactersPerCell=%d aspectRatioPreserved=1 topRowFirst=1 rowFrame=PIPE informationScope=omniscient_actual_map",
		GC.getGame().getGameTurn(), szReason, iOverview, kMap.getGridWidth(), kMap.getGridHeight(), iFullPreviewWidth, iFullPreviewHeight,
		iOverviewWidth, iOverviewHeight, iOutputWidth, iOverviewHeight, iScalePercent, iHorizontalCharsPerCell);
	logSASGameRecordMapAsciiGeographyRows(kMap, kPalette, iOverviewWidth, iOverviewHeight, iHorizontalCharsPerCell, aiSymbolCounts);
	int const iOverviewCells = iOverviewWidth * iOverviewHeight;
	int const iDrawingCharacters = (iOutputWidth + 2) * iOverviewHeight;
	logSASGameRecord("GAME_RECORD_MAP_ASCII_OVERVIEW_END turn=%d reason=%s overview=%d layer=GEOGRAPHY overviewCells=%d drawingCharacters=%d overviewCellCounts=%s",
		GC.getGame().getGameTurn(), szReason, iOverview, iOverviewCells, iDrawingCharacters,
		getSASDiagnosticQuoted(getSASGameRecordMapAsciiSymbolCounts(aiSymbolCounts).GetCString()).GetCString());
}

// <!-- custom: Bounded previews can hide exact narrow passages and source-plot distances.
// Emit one initial native geography picture when requested, with one logical cell per source plot and explicit coordinate extents; omit it when the ordinary full preview is already native. (GPT-5.6-Sol) -->
static void logSASGameRecordMapAsciiNativeGeography(CvMap const& kMap, SASGameRecordMapAsciiPalette const& kPalette, int iHorizontalCharsPerCell, char const* szReason)
{
	int const iSourceWidth = kMap.getGridWidth();
	int const iSourceHeight = kMap.getGridHeight();
	int const iOutputWidth = iSourceWidth * iHorizontalCharsPerCell;
	int aiSymbolCounts[127] = { 0 };
	logSASGameRecord("GAME_RECORD_MAP_ASCII_NATIVE_BEGIN turn=%d reason=%s layer=GEOGRAPHY source=%dx%d nativeCells=%dx%d outputCharacters=%dx%d sourcePlotsPerCell=1 horizontalCharactersPerCell=%d aspectRatioPreserved=1 sourceXLeft=0 sourceXRight=%d sourceYTop=%d sourceYBottom=0 topRowFirst=1 rowFrame=PIPE informationScope=omniscient_actual_map",
		GC.getGame().getGameTurn(), szReason, iSourceWidth, iSourceHeight, iSourceWidth, iSourceHeight, iOutputWidth, iSourceHeight,
		iHorizontalCharsPerCell, iSourceWidth - 1, iSourceHeight - 1);
	logSASGameRecordMapAsciiGeographyRows(kMap, kPalette, iSourceWidth, iSourceHeight, iHorizontalCharsPerCell, aiSymbolCounts);
	int const iSourceCells = iSourceWidth * iSourceHeight;
	int const iDrawingCharacters = (iOutputWidth + 2) * iSourceHeight;
	logSASGameRecord("GAME_RECORD_MAP_ASCII_NATIVE_END turn=%d reason=%s layer=GEOGRAPHY nativeCells=%d drawingCharacters=%d nativeCellCounts=%s",
		GC.getGame().getGameTurn(), szReason, iSourceCells, iDrawingCharacters,
		getSASDiagnosticQuoted(getSASGameRecordMapAsciiSymbolCounts(aiSymbolCounts).GetCString()).GetCString());
}

// <!-- custom: A bounded text map gives external LLMs and text-only reviewers the broad spatial relationships that aggregate landmass statistics cannot show. Fit the generated map, rather than the selected XML world size, into the tunable box with one scale so Tiny maps remain exact while horizontal SAS_Longworld and possible vertical/tower maps preserve their shapes.
// Monospace characters are usually much taller than wide, so repeat each map cell horizontally by a tunable amount. Keep metadata outside the pipe-framed drawing rows so humans and LLMs can parse each layer as one uninterrupted picture.
// Geography, terrain, directional river edges, and bonuses are normally stable enough to record once at setup/load; structured map-change rows preserve later exceptions. Record features initially because jungle, forest, flood plains, oases, and ice affect settling, movement, health, and yields; repeat features and political borders at level-3 snapshots to show Forest/Jungle clearing and regrowth, fallout, expansion, conquest, and collapse. (GPT-5.6-Sol + ChatGPT-5.6-Sol) -->
static void logSASGameRecordMapAscii(bool bIncludeStaticLayers, char const* szReason)
{
	uint const uiMapAsciiStartTime = getSASMonotonicMilliseconds();
	bool const abLayerEnabled[6] =
	{
		bIncludeStaticLayers && isSASGameRecordMapAsciiGeographyEnabled(),
		bIncludeStaticLayers && isSASGameRecordMapAsciiTerrainEnabled(),
		bIncludeStaticLayers && isSASGameRecordMapAsciiRiversEnabled(),
		bIncludeStaticLayers && isSASGameRecordMapAsciiBonusesEnabled(),
		isSASGameRecordMapAsciiFeaturesEnabled(),
		isSASGameRecordMapAsciiPoliticalEnabled()
	};
	int iLayerCount = 0;
	for (int iLayer = 0; iLayer < 6; iLayer++)
		if (abLayerEnabled[iLayer]) iLayerCount++;
	if (iLayerCount <= 0)
		return;
	int const iMaxWidth = getSASGameRecordMapAsciiMaxWidth();
	int const iMaxHeight = getSASGameRecordMapAsciiMaxHeight();
	if (iMaxWidth <= 0 || iMaxHeight <= 0)
		return;
	SASGameRecordMapAsciiPalette const& kPalette = getSASGameRecordMapAsciiPalette();
	if (!kPalette.bValid)
	{
		logSASGameRecord("GAME_RECORD_MAP_ASCII_CONFIG_ERROR turn=%d error=%s", GC.getGame().getGameTurn(), getSASDiagnosticQuoted(kPalette.szError.GetCString()).GetCString());
		FAssertMsg(false, kPalette.szError.GetCString());
		return;
	}
	CvMap const& kMap = GC.getMap();
	int const iSourceWidth = kMap.getGridWidth();
	int const iSourceHeight = kMap.getGridHeight();
	if (iSourceWidth <= 0 || iSourceHeight <= 0)
		return;
	int const iHorizontalCharsPerCell = getSASGameRecordMapAsciiHorizontalCharsPerCell();
	int const iMaxCellWidth = std::max(1, iMaxWidth / iHorizontalCharsPerCell);
	int iPreviewWidth = iSourceWidth;
	int iPreviewHeight = iSourceHeight;
	if (iSourceWidth > iMaxCellWidth || iSourceHeight > iMaxHeight)
	{
		if (iSourceWidth * iMaxHeight >= iSourceHeight * iMaxCellWidth)
		{
			iPreviewWidth = iMaxCellWidth;
			iPreviewHeight = std::max(1, (iSourceHeight * iMaxCellWidth + iSourceWidth / 2) / iSourceWidth);
		}
		else
		{
			iPreviewHeight = iMaxHeight;
			iPreviewWidth = std::max(1, (iSourceWidth * iMaxHeight + iSourceHeight / 2) / iSourceHeight);
		}
	}
	int const iOutputWidth = iPreviewWidth * iHorizontalCharsPerCell;
	int iSourceSouthBoundaryRiverEdges = 0;
	int iSourceEastBoundaryRiverEdges = 0;
	if (abLayerEnabled[2])
	{
		for (int iPlot = 0; iPlot < kMap.numPlots(); iPlot++)
		{
			CvPlot const& kPlot = *kMap.plotByIndex(iPlot);
			if (kPlot.isNOfRiver()) iSourceSouthBoundaryRiverEdges++;
			if (kPlot.isWOfRiver()) iSourceEastBoundaryRiverEdges++;
		}
	}
	bool const bMarkStartingPlots = (abLayerEnabled[5] && GC.getGame().getElapsedGameTurns() == 0);
	std::vector<PlayerTypes> aeStartingPlayers;
	CvString szPlayerSymbols;
	if (bMarkStartingPlots) aeStartingPlayers.assign(kMap.numPlots(), NO_PLAYER);
	if (abLayerEnabled[5])
	{
		for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
		{
			PlayerTypes const ePlayer = (PlayerTypes)iPlayer;
			CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
			if (!kPlayer.isEverAlive())
				continue;
			if (bMarkStartingPlots && kPlayer.getStartingPlot() != NULL)
				aeStartingPlayers[kMap.plotNum(kPlayer.getStartingPlot()->getX(), kPlayer.getStartingPlot()->getY())] = ePlayer;
			CvString szPlayerSymbol;
			szPlayerSymbol.Format("%c=%d", getSASGameRecordMapAsciiPlayerSymbol(ePlayer, kPalette), iPlayer);
			if (!szPlayerSymbols.empty()) szPlayerSymbols += ";";
			szPlayerSymbols += szPlayerSymbol;
		}
	}
	CvString const szPlayerSymbolsQuoted = getSASDiagnosticQuoted(getSASDiagnosticOrDash(szPlayerSymbols).GetCString());
	int const iGameTurn = GC.getGame().getGameTurn();
	// <!-- custom: Keep one timing anchor at each text-map boundary.
	// Appending the same session time to every fixed-width drawing row obscured the frame, repeated no useful chronology and materially enlarged the log.
	// Repeat only `turn` on structured legend/layer framing rows so grep/parser slices remain self-locating while raw pipe-art rows stay compact and visually aligned. (GPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_MAP_ASCII_BEGIN turn=%d reason=%s layers=%d source=%dx%d previewCells=%dx%d outputCharacters=%dx%d maxCharacters=%dx%d horizontalCharactersPerCell=%d aspectRatioPreserved=1 resampled=%d wrapX=%d wrapY=%d topRowFirst=1 rowFrame=PIPE informationScope=omniscient_actual_map sessionWallMilliseconds=%u",
		iGameTurn, szReason, iLayerCount, iSourceWidth, iSourceHeight, iPreviewWidth, iPreviewHeight, iOutputWidth,
		iPreviewHeight, iMaxWidth, iMaxHeight, iHorizontalCharsPerCell, iPreviewWidth != iSourceWidth || iPreviewHeight != iSourceHeight,
		kMap.isWrapX(), kMap.isWrapY(), getSASGameRecordSessionWallMilliseconds(uiMapAsciiStartTime));
	if (abLayerEnabled[0])
		logSASGameRecord("GAME_RECORD_MAP_ASCII_LEGEND turn=%d layer=GEOGRAPHY symbolTypeCount=%d symbolTypes=%s sourcePlotTypeCount=%d sourcePlotTypes=\"%d:PLOT_PEAK;%d:PLOT_HILLS;%d:PLOT_LAND;%d:PLOT_OCEAN\" resampledCell=derived_plot_mix",
			iGameTurn, SAS_MAP_ASCII_GEOGRAPHY_SYMBOL_COUNT,
			getSASDiagnosticQuoted(getSASGameRecordMapAsciiGeographyLegend(kPalette).GetCString()).GetCString(), NUM_PLOT_TYPES, PLOT_PEAK,
			PLOT_HILLS, PLOT_LAND, PLOT_OCEAN);
	if (abLayerEnabled[1])
		logSASGameRecord("GAME_RECORD_MAP_ASCII_LEGEND turn=%d layer=TERRAIN runtimeTypeCount=%d runtimeTypeFormat=SYMBOL=ID:TYPE symbolTypes=%s uppercaseFoodSymbols=%s uppercaseMinNatureFoodSurplus=%d foodPerPopulation=%d foodIncludes=hills_features_lakes_rivers_permanent_plot_yields foodExcludes=improvements_bonuses resampledCell=dominant_type_with_average_nature_food_case",
			iGameTurn, GC.getNumTerrainInfos(), getSASDiagnosticQuoted(getSASGameRecordMapAsciiTerrainLegend(kPalette).GetCString()).GetCString(),
			getSASDiagnosticQuoted(getSASGameRecordMapAsciiTerrainUppercaseLegend(kPalette).GetCString()).GetCString(),
			kPalette.iTerrainUppercaseMinNatureFoodSurplus, kPalette.iFoodPerPopulation);
	if (abLayerEnabled[2])
		logSASGameRecord("GAME_RECORD_MAP_ASCII_LEGEND turn=%d layer=RIVERS palette=%s symbolTypes=%s storedEdgeSemantics=plot_south_and_east_boundaries resampledCell=any_source_edge_by_orientation",
			iGameTurn, getSASDiagnosticQuoted(kPalette.szRiverDefine.GetCString()).GetCString(),
			getSASDiagnosticQuoted(getSASGameRecordMapAsciiRiverLegend(kPalette).GetCString()).GetCString());
	// <!-- custom: getBonusType(NO_TEAM) intentionally records the actual map, including bonuses that no civilization has the technology to reveal yet.
	// State this explicitly so an analyst does not mistake diagnostic knowledge for contemporary AI knowledge. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (abLayerEnabled[3])
		logSASGameRecord("GAME_RECORD_MAP_ASCII_LEGEND turn=%d layer=BONUSES runtimeTypeCount=%d runtimeTypeFormat=SYMBOL=ID:TYPE symbolTypes=%s resampledCell=bonus_type_or_multiple includesUnrevealedBonuses=1",
			iGameTurn, GC.getNumBonusInfos(), getSASDiagnosticQuoted(getSASGameRecordMapAsciiBonusLegend(kPalette).GetCString()).GetCString());
	if (abLayerEnabled[4])
		logSASGameRecord("GAME_RECORD_MAP_ASCII_LEGEND turn=%d layer=FEATURES runtimeTypeCount=%d runtimeTypeFormat=SYMBOL=ID:TYPE symbolTypes=%s resampledCell=dominant_type",
			iGameTurn, GC.getNumFeatureInfos(), getSASDiagnosticQuoted(getSASGameRecordMapAsciiFeatureLegend(kPalette).GetCString()).GetCString());
	// <!-- custom: GAME_RECORD_PLAYER_SETUP already stores each player ID's quoted civilization and display name.
	// Keep this repeated map legend to unambiguous SYMBOL=PLAYER_ID pairs so user-controlled punctuation in names cannot break an embedded mini-format or duplicate long names at every snapshot. (ChatGPT-5.5 + GPT-5.6-Sol) -->
	if (abLayerEnabled[5])
		logSASGameRecord("GAME_RECORD_MAP_ASCII_LEGEND turn=%d layer=POLITICAL palette=%s order=unowned_water,unowned_land,mixed_unowned_water_land,civilization_city,Barbarian_city,civilization_and_Barbarian_cities,multiple_starting_players,Barbarian_territory playerSymbolFormat=SYMBOL=PLAYER_ID playerSymbols=%s playerDetailsRows=GAME_RECORD_PLAYER_SETUP startingPlotsMarked=%d",
				iGameTurn, getSASDiagnosticQuoted(kPalette.szPoliticalDefine.GetCString()).GetCString(), szPlayerSymbolsQuoted.GetCString(), bMarkStartingPlots);
	if (abLayerEnabled[0])
	{
		int const iOverview1ScalePercent = getSASGameRecordMapAsciiOverview1ScalePercent();
		int const iOverview2ScalePercent = getSASGameRecordMapAsciiOverview2ScalePercent();
		if (iOverview1ScalePercent > 0)
			logSASGameRecordMapAsciiGeographyOverview(kMap, kPalette, iPreviewWidth, iPreviewHeight, iHorizontalCharsPerCell, 1, iOverview1ScalePercent, szReason);
		// <!-- custom: Equal configured percentages would produce identical pictures, so keep the first and omit the duplicate second overview. (GPT-5.6-Sol) -->
		if (iOverview2ScalePercent > 0 && iOverview2ScalePercent != iOverview1ScalePercent)
			logSASGameRecordMapAsciiGeographyOverview(kMap, kPalette, iPreviewWidth, iPreviewHeight, iHorizontalCharsPerCell, 2, iOverview2ScalePercent, szReason);
		if (isSASGameRecordMapAsciiNativeGeographyEnabled() && (iPreviewWidth != iSourceWidth || iPreviewHeight != iSourceHeight))
			logSASGameRecordMapAsciiNativeGeography(kMap, kPalette, iHorizontalCharsPerCell, szReason);
	}
	for (int iLayer = 0; iLayer < 6; iLayer++)
	{
		if (!abLayerEnabled[iLayer]) continue;
		char const* szLayer = (iLayer == 0 ? "GEOGRAPHY" : iLayer == 1 ? "TERRAIN" : iLayer == 2 ? "RIVERS" : iLayer == 3 ? "BONUSES" : iLayer == 4 ? "FEATURES" : "POLITICAL");
		int aiSymbolCounts[127] = { 0 };
		logSASGameRecord("GAME_RECORD_MAP_ASCII_LAYER_BEGIN turn=%d layer=%s rows=%d columns=%d sourceYTop=%d sourceYBottom=0", iGameTurn, szLayer, iPreviewHeight, iOutputWidth, iSourceHeight - 1);
		for (int iRow = 0; iRow < iPreviewHeight; iRow++)
		{
			int const iPreviewY = iPreviewHeight - iRow - 1;
			int const iMinY = (iPreviewY * iSourceHeight) / iPreviewHeight;
			int const iMaxY = ((iPreviewY + 1) * iSourceHeight) / iPreviewHeight;
			CvString szPlots = "|";
			for (int iPreviewX = 0; iPreviewX < iPreviewWidth; iPreviewX++)
			{
				int const iMinX = (iPreviewX * iSourceWidth) / iPreviewWidth;
				int const iMaxX = ((iPreviewX + 1) * iSourceWidth) / iPreviewWidth;
				char cSymbol;
				if (iLayer == 0) cSymbol = getSASGameRecordMapAsciiGeographySymbol(kMap, kPalette, iMinX, iMaxX, iMinY, iMaxY);
				else if (iLayer == 1) cSymbol = getSASGameRecordMapAsciiTerrainSymbol(kMap, kPalette, iMinX, iMaxX, iMinY, iMaxY);
				else if (iLayer == 2) cSymbol = getSASGameRecordMapAsciiRiverSymbol(kMap, kPalette, iMinX, iMaxX, iMinY, iMaxY);
				else if (iLayer == 3) cSymbol = getSASGameRecordMapAsciiBonusSymbol(kMap, kPalette, iMinX, iMaxX, iMinY, iMaxY);
				else if (iLayer == 4) cSymbol = getSASGameRecordMapAsciiFeatureSymbol(kMap, kPalette, iMinX, iMaxX, iMinY, iMaxY);
				else cSymbol = getSASGameRecordMapAsciiPoliticalSymbol(kMap, kPalette, aeStartingPlayers, iMinX, iMaxX, iMinY, iMaxY);
				aiSymbolCounts[(unsigned char)cSymbol]++;
				for (int iRepeat = 0; iRepeat < iHorizontalCharsPerCell; iRepeat++)
					appendSASGameRecordMapAsciiSymbol(szPlots, cSymbol);
			}
			szPlots += "|";
			logSASGameRecord("%s", szPlots.GetCString());
		}
		int const iPreviewCells = iPreviewWidth * iPreviewHeight;
		int const iDrawingCharacters = (iOutputWidth + 2) * iPreviewHeight;
		if (iLayer == 2)
			logSASGameRecord("GAME_RECORD_MAP_ASCII_LAYER_END turn=%d layer=%s previewCells=%d drawingCharacters=%d previewCellCounts=%s sourceSouthBoundaryRiverEdges=%d sourceEastBoundaryRiverEdges=%d sourceRiverEdges=%d",
					iGameTurn, szLayer, iPreviewCells, iDrawingCharacters, getSASDiagnosticQuoted(getSASGameRecordMapAsciiSymbolCounts(aiSymbolCounts).GetCString()).GetCString(),
					iSourceSouthBoundaryRiverEdges, iSourceEastBoundaryRiverEdges, iSourceSouthBoundaryRiverEdges + iSourceEastBoundaryRiverEdges);
		else logSASGameRecord("GAME_RECORD_MAP_ASCII_LAYER_END turn=%d layer=%s previewCells=%d drawingCharacters=%d previewCellCounts=%s",
			iGameTurn, szLayer, iPreviewCells, iDrawingCharacters,
			getSASDiagnosticQuoted(getSASGameRecordMapAsciiSymbolCounts(aiSymbolCounts).GetCString()).GetCString());
	}
	uint const uiMapAsciiEndTime = getSASMonotonicMilliseconds();
	logSASGameRecord("GAME_RECORD_MAP_ASCII_END turn=%d reason=%s layers=%d rowsPerLayer=%d sessionWallMilliseconds=%u mapBlockWallMilliseconds=%u",
		iGameTurn, szReason, iLayerCount, iPreviewHeight, getSASGameRecordSessionWallMilliseconds(uiMapAsciiEndTime),
		getSASElapsedMilliseconds(uiMapAsciiStartTime, uiMapAsciiEndTime));
}

static void seedSASGameRecordTeamPreviousFromCurrentState(TeamTypes eTeam)
{
	CvGame const& kGame = GC.getGame();
	CvTeam const& kTeam = GET_TEAM(eTeam);
	SASGameRecordTeamPrevious& kPrevious = g_akSASGameRecordTeamPrevious[eTeam];
	int const iLand = kTeam.getTotalLand();
	int const iPopulation = kTeam.getTotalPopulation();
	kPrevious.bValid = true;
	kPrevious.iTechs = kTeam.getTechCount();
	kPrevious.iLand = iLand;
	kPrevious.iLandPctX100 = (10000 * iLand) / std::max(1, GC.getMap().getLandPlots());
	kPrevious.iPopulation = iPopulation;
	kPrevious.iPopPctX100 = (10000 * iPopulation) / std::max(1, kGame.getTotalPopulation());
	kPrevious.bContactsValid = true;
	kPrevious.iMetTeams = getSASGameRecordMetTeamCount(eTeam);
}

struct SASGameRecordInitialTechGroup
{
	CvString szTechFields;
	CvString szTeams;
	int iTeams;
};

// <!-- custom: Successful new-game initialization is best described by its authoritative result, not by the order in which Civ4 happened to call meet/declareWar/setHasTech/startTrade while constructing that result.
// Share the field semantics with BBAI and seed recorder deltas from the same finalized baseline.
// Group identical technology sets so a late-era start does not repeat the same long payload for every team; the explicit team lists keep arbitrary scenarios and mixed/modded setups exact. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
static void logSASGameRecordFinalizedInitialState(int& iTeamStateRows, int& iTechRows, int& iDeals)
{
	iTeamStateRows = 0;
	iTechRows = 0;
	iDeals = 0;
	// <!-- custom: Caller contract: logSASGameRecordNewGameStarted invokes this helper only at level 2+.
	// Keeping the gate outside prevents redundant checks and makes any future caller responsible for avoiding log-only setup work. (ChatGPT-5.6-Sol) -->
	std::vector<SASGameRecordInitialTechGroup> aTechGroups;
	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		TeamTypes const eTeam = (TeamTypes)iI;
		if (!GET_TEAM(eTeam).isEverAlive())
			continue;
		logSASGameRecord("GAME_RECORD_INITIAL_TEAM_STATE %s", getSASInitialTeamStateFields(eTeam).GetCString());
		CvString const szTechFields = getSASInitialTeamTechLevelFields(eTeam);
		SASGameRecordInitialTechGroup* pGroup = NULL;
		for (size_t iGroup = 0; iGroup < aTechGroups.size(); iGroup++)
		{
			if (aTechGroups[iGroup].szTechFields == szTechFields)
			{
				pGroup = &aTechGroups[iGroup];
				break;
			}
		}
		if (pGroup == NULL)
		{
			SASGameRecordInitialTechGroup kGroup;
			kGroup.szTechFields = szTechFields;
			kGroup.iTeams = 0;
			aTechGroups.push_back(kGroup);
			pGroup = &aTechGroups.back();
		}
		appendSASDiagnosticIntListValue(pGroup->szTeams, eTeam);
		pGroup->iTeams++;
		seedSASGameRecordTeamPreviousFromCurrentState(eTeam);
		iTeamStateRows++;
	}
	for (size_t iGroup = 0; iGroup < aTechGroups.size(); iGroup++)
	{
		SASGameRecordInitialTechGroup const& kGroup = aTechGroups[iGroup];
		logSASGameRecord("GAME_RECORD_INITIAL_TEAM_TECHS teams=%s teamCount=%d %s", kGroup.szTeams.GetCString(), kGroup.iTeams, kGroup.szTechFields.GetCString());
		iTechRows++;
	}
	int iLoop = 0;
	for (CvDeal const* pDeal = GC.getGame().firstDeal(&iLoop); pDeal != NULL; pDeal = GC.getGame().nextDeal(&iLoop))
	{
		if (isSASCollapsibleAdvancedStartPeaceDeal(*pDeal))
			continue;
		logSASGameRecord("GAME_RECORD_INITIAL_DEAL %s", getSASInitialDealStateFields(*pDeal).GetCString());
		iDeals++;
	}
}

// <!-- custom: Team-state rows identify numeric members exactly, but placing readable player/civilization identities only after hundreds of geography and text-map rows made the initial team and technology records needlessly hard to interpret.
// Emit fixed slot bounds and player identities before team relations; later map legends can still reference the same PLAYER_SETUP rows without repeating them. (GPT-5.6-Sol) -->
static void logSASGameRecordInitialPlayerIdentities()
{
	logSASGameRecord("GAME_RECORD_SLOT_CONSTANTS MAX_CIV_PLAYERS=%d MAX_PLAYERS=%d BARBARIAN_PLAYER=%d MAX_CIV_TEAMS=%d MAX_TEAMS=%d BARBARIAN_TEAM=%d NO_PLAYER=%d NO_TEAM=%d",
		MAX_CIV_PLAYERS, MAX_PLAYERS, BARBARIAN_PLAYER, MAX_CIV_TEAMS, MAX_TEAMS, BARBARIAN_TEAM, NO_PLAYER, NO_TEAM);
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes const eLoopPlayer = (PlayerTypes)iI;
		CvPlayer const& kLoopPlayer = GET_PLAYER(eLoopPlayer);
		if (kLoopPlayer.isEverAlive() && !kLoopPlayer.isBarbarian())
			logSASGameRecordPlayerSetup(eLoopPlayer);
	}
}

static void logSASGameRecordInitialContext(bool bNewGame)
{
	logSASGameRecordGeography();
	bool const bLogInitialDetails = (gGameRecordLogLevel >= 2);
	bool const bLogInitialVerboseDetails = (bLogInitialDetails && gGameRecordLogLevel >= 3);
	if (bLogInitialVerboseDetails)
	{
		logSASGameRecordMapAscii(true, "initialContext");
		logSASGameRecordRiverEdgeCoordinates();
	}
	if (!bLogInitialDetails)
		return;
	logSASGameRecordAttitudeLegend();
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes eLoopPlayer = (PlayerTypes)iI;
		CvPlayer const& kLoopPlayer = GET_PLAYER(eLoopPlayer);
		if (!kLoopPlayer.isAlive() || kLoopPlayer.isBarbarian())
			continue;
		logSASGameRecordKnownArea(eLoopPlayer, "setup");
		logSASGameRecordStartingUnits(eLoopPlayer, "setup");
		int iLoop = 0;
		for (CvCity const* pLoopCity = kLoopPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kLoopPlayer.nextCity(&iLoop))
			logSASGameRecordCityBFC(*pLoopCity, "setup");
	}
	// <!-- custom: Level-2+ new games already emitted authoritative INITIAL_TEAM_STATE metTeams and seeded the contact baseline.
	// Loaded saves have no finalized initial-team block in this session, so retain explicit setup contact rows for them. (ChatGPT-5.6-Sol) -->
	if (!bNewGame)
	{
		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			TeamTypes eLoopTeam = (TeamTypes)iI;
			if (GET_TEAM(eLoopTeam).isAlive() && !GET_TEAM(eLoopTeam).isBarbarian())
				logSASGameRecordTeamContacts(eLoopTeam, GC.getGame().getGameTurn(), "setup");
		}
	}
}

static void logSASGameRecordMapBonusTotals(int iGameTurn)
{
	CvString szBonuses;
	FOR_EACH_ENUM(Bonus)
		appendSASGameRecordTypeCount(szBonuses, getSASGameRecordBonusType(eLoopBonus), GC.getMap().getNumBonuses(eLoopBonus));
	logSASGameRecord("GAME_RECORD_MAP_BONUSES turn=%d total=%s", iGameTurn, getSASDiagnosticOrDash(szBonuses).GetCString());
}

static void logSASGameRecordTeamProjects(TeamTypes eTeam, int iGameTurn);

static void logSASGameRecordBattleBuckets(int iGameTurn)
{
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes eLoopPlayer = (PlayerTypes)iI;
		SASGameRecordBattleQuality& kQuality = g_akSASGameRecordBattleQuality[iI];
		if (g_aiSASGameRecordBattleWins[iI] != 0 || g_aiSASGameRecordBattleLosses[iI] != 0 || g_aiSASGameRecordCityBattleWins[iI] != 0 || g_aiSASGameRecordCityBattleLosses[iI] != 0 || kQuality.hasAny())
		{
			// <!-- custom: Expected wins are the sum of exact own pre-combat odds for the same binary battles counted by luckEligibleWins. luckDeltaX1000 is therefore observed minus expected wins in thousandths of a win.
			// Withdrawals/combat-limit outcomes remain separate. (ChatGPT-5.6-Sol) -->
			logSASGameRecord("GAME_RECORD_BATTLE_SUMMARY turn=%d range=%d-%d player=%d wins=%d losses=%d cityPlotWins=%d cityPlotLosses=%d withdrawals=%d enemyWithdrawals=%d combatLimitAttacks=%d combatLimitDefenses=%d luckEligibleBattles=%d luckEligibleWins=%d expectedWinsX1000=%d luckDeltaX1000=%+d upsetWins=%d upsetLosses=%d lowestOddsWinPermille=%d highestOddsLossPermille=%d",
				iGameTurn, g_iSASGameRecordBattleStartTurn, iGameTurn, eLoopPlayer, g_aiSASGameRecordBattleWins[iI],
				g_aiSASGameRecordBattleLosses[iI], g_aiSASGameRecordCityBattleWins[iI], g_aiSASGameRecordCityBattleLosses[iI],
				kQuality.iWithdrawals, kQuality.iEnemyWithdrawals, kQuality.iCombatLimitAttacks, kQuality.iCombatLimitDefenses,
				kQuality.iLuckEligibleBattles, kQuality.iLuckEligibleWins, kQuality.iExpectedWinsX1000,
				1000 * kQuality.iLuckEligibleWins - kQuality.iExpectedWinsX1000, kQuality.iUpsetWins, kQuality.iUpsetLosses,
				kQuality.iLowestOddsWinPermille, kQuality.iHighestOddsLossPermille);
		}
		g_aiSASGameRecordBattleWins[iI] = 0;
		g_aiSASGameRecordBattleLosses[iI] = 0;
		g_aiSASGameRecordCityBattleWins[iI] = 0;
		g_aiSASGameRecordCityBattleLosses[iI] = 0;
		kQuality.reset();
	}
	// <!-- custom: Advance from the actual reset boundary instead of fabricating the next range from the configured interval.
	// This preserves turn-0 combat and mid-interval load/victory flushes. See KI#378. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	g_iSASGameRecordBattleStartTurn = iGameTurn + 1;
}

static CvString getSASGameRecordAIProductionTransitions(SASGameRecordPlayerFlow const& kFlow)
{
	CvString szTransitions;
	static char const* const aszKinds[NUM_SAS_PRODUCTION_KINDS] = {"UNIT", "BUILDING", "WONDER", "PROJECT", "PROCESS"};
	for (int iOld = 0; iOld < NUM_SAS_PRODUCTION_KINDS; iOld++)
	{
		for (int iNew = 0; iNew < NUM_SAS_PRODUCTION_KINDS; iNew++)
		{
			int const iCount = kFlow.aiAIProductionTransitions[iOld * NUM_SAS_PRODUCTION_KINDS + iNew];
			if (iCount <= 0)
				continue;
			CvString szItem;
			szItem.Format(szTransitions.empty() ? "%s>%s:%d" : ",%s>%s:%d", aszKinds[iOld], aszKinds[iNew], iCount);
			szTransitions += szItem;
		}
	}
	return getSASDiagnosticOrDash(szTransitions);
}

static int getSASGameRecordMaxAIProductionTargetChangesOneCity(SASGameRecordPlayerFlow const& kFlow)
{
	int iMax = 0;
	for (size_t iI = 0; iI < kFlow.aAIProductionTargetChangesByCity.size(); iI++)
		iMax = std::max(iMax, kFlow.aAIProductionTargetChangesByCity[iI].second);
	return iMax;
}

static void logSASGameRecordFlowBuckets(int iGameTurn)
{
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes const ePlayer = (PlayerTypes)iI;
		SASGameRecordPlayerFlow& kFlow = g_akSASGameRecordPlayerFlow[iI];
		if (kFlow.hasProduction())
		{
			CvString szUnitTypes;
			CvString szConscriptedUnitTypes;
			CvString szColonyFreeDefenderUnitTypes;
			CvString szBuildingTypes;
			CvString szProjectTypes;
			FOR_EACH_ENUM(Unit)
			{
				appendSASGameRecordTypeCount(szUnitTypes, getSASGameRecordUnitType(eLoopUnit), kFlow.aiUnitTypes[eLoopUnit]);
				appendSASGameRecordTypeCount(szConscriptedUnitTypes, getSASGameRecordUnitType(eLoopUnit), kFlow.aiConscriptedUnitTypes[eLoopUnit]);
				appendSASGameRecordTypeCount(szColonyFreeDefenderUnitTypes, getSASGameRecordUnitType(eLoopUnit), kFlow.aiColonyFreeDefenderUnitTypes[eLoopUnit]);
			}
			FOR_EACH_ENUM(Building)
				appendSASGameRecordTypeCount(szBuildingTypes, getSASGameRecordBuildingType(eLoopBuilding), kFlow.aiBuildingTypes[eLoopBuilding]);
			FOR_EACH_ENUM(Project)
				appendSASGameRecordTypeCount(szProjectTypes, getSASGameRecordProjectType(eLoopProject), kFlow.aiProjectTypes[eLoopProject]);
			logSASGameRecord("GAME_RECORD_PRODUCTION_FLOW turn=%d range=%d-%d player=%d unitsProduced=%d unitProductionNeeded=%d unitTypes=%s unitsConscripted=%d conscriptProductionNeeded=%d conscriptedUnitTypes=%s colonyFreeDefenders=%d colonyFreeDefenderProductionNeeded=%d colonyFreeDefenderUnitTypes=%s buildingsCompleted=%d buildingProductionNeeded=%d buildingTypes=%s projectsCompleted=%d projectProductionNeeded=%d projectTypes=%s overflowActions=%d rawModifiedOverflow=%d unmodifiedOverflow=%d keptOverflow=%d lostProduction=%d unusedOverflowCapacity=%d overflowGold=%d failedInvestedProduction=%d failGold=%d aiTargetSwitches=%d aiTargetClears=%d aiInvestedTargetChanges=%d aiProductionParked=%d aiTargetResumes=%d aiProductionResumed=%d aiTargetChangedCities=%d aiMaxTargetChangesOneCity=%d aiTargetTransitions=%s productionDecayActions=%d productionDecayLost=%d productionInvalidatedActions=%d productionInvalidatedLost=%d productionUpgradeTransfers=%d productionUpgradeTransferred=%d productionUpgradeOverwriteActions=%d productionUpgradeOverwritten=%d",
				iGameTurn, g_iSASGameRecordFlowStartTurn, iGameTurn, ePlayer, kFlow.iUnitsCompleted, kFlow.iUnitProductionNeeded,
				getSASDiagnosticOrDash(szUnitTypes).GetCString(), kFlow.iUnitsConscripted, kFlow.iConscriptProductionNeeded,
				getSASDiagnosticOrDash(szConscriptedUnitTypes).GetCString(), kFlow.iColonyFreeDefenders, kFlow.iColonyFreeDefenderProductionNeeded,
				getSASDiagnosticOrDash(szColonyFreeDefenderUnitTypes).GetCString(), kFlow.iBuildingsCompleted, kFlow.iBuildingProductionNeeded,
				getSASDiagnosticOrDash(szBuildingTypes).GetCString(), kFlow.iProjectsCompleted, kFlow.iProjectProductionNeeded,
				getSASDiagnosticOrDash(szProjectTypes).GetCString(), kFlow.iOverflowActions, kFlow.iRawModifiedOverflow,
				kFlow.iUnmodifiedOverflow, kFlow.iKeptOverflow, kFlow.iLostProduction, kFlow.iUnusedOverflowCapacity, kFlow.iOverflowGold,
				kFlow.iFailedInvestedProduction, kFlow.iFailGold, kFlow.iAIProductionTargetSwitches, kFlow.iAIProductionTargetClears,
				kFlow.iAIProductionInvestedTargetChanges, kFlow.iAIProductionParked, kFlow.iAIProductionTargetResumes,
				kFlow.iAIProductionResumed, (int)kFlow.aAIProductionTargetChangesByCity.size(),
				getSASGameRecordMaxAIProductionTargetChangesOneCity(kFlow), getSASGameRecordAIProductionTransitions(kFlow).GetCString(),
				kFlow.iProductionDecayActions, kFlow.iProductionDecayLost, kFlow.iProductionInvalidatedActions,
				kFlow.iProductionInvalidatedLost, kFlow.iProductionUpgradeTransfers, kFlow.iProductionUpgradeTransferred,
				kFlow.iProductionUpgradeOverwriteActions, kFlow.iProductionUpgradeOverwritten);
		}
		if (kFlow.hasMilitary())
		{
			CvString szPromotionChoices;
			FOR_EACH_ENUM(Promotion)
				appendSASGameRecordTypeCount(szPromotionChoices, getSASGameRecordPromotionType(eLoopPromotion), kFlow.aiPromotionChoices[eLoopPromotion]);
			logSASGameRecord("GAME_RECORD_MILITARY_FLOW turn=%d range=%d-%d player=%d combatWins=%d combatLosses=%d cityPlotWins=%d cityPlotLosses=%d enemyProductionNeededDestroyed=%d ownProductionNeededLost=%d enemyXpDestroyed=%d ownXpLost=%d xpGained=%d combatXpGained=%d nonCombatXpGained=%d xpPreventedByCap=%d xpLostAdjustments=%d promotionsChosen=%d leaderPromotionApplications=%d promotionChoices=%s upgrades=%d upgradeGold=%d scrapped=%d scrappedProductionNeeded=%d captured=%d capturedProductionNeeded=%d",
				iGameTurn, g_iSASGameRecordFlowStartTurn, iGameTurn, ePlayer, kFlow.iCombatWins, kFlow.iCombatLosses, kFlow.iCityPlotWins,
				kFlow.iCityPlotLosses, kFlow.iEnemyProductionNeededDestroyed, kFlow.iOwnProductionNeededLost,
				kFlow.iEnemyExperienceDestroyed, kFlow.iOwnExperienceLost, kFlow.iExperienceGained, kFlow.iCombatExperienceGained,
				kFlow.iNonCombatExperienceGained, kFlow.iExperiencePreventedByCap, kFlow.iExperienceLostAdjustments, kFlow.iPromotionsChosen,
				kFlow.iLeaderPromotionApplications, getSASDiagnosticOrDash(szPromotionChoices).GetCString(), kFlow.iUpgrades,
				kFlow.iUpgradeGold, kFlow.iScrapped, kFlow.iScrappedProductionNeeded, kFlow.iCaptured, kFlow.iCapturedProductionNeeded);
		}
		if (kFlow.hasCityPopulationFlow())
		{
			logSASGameRecord("GAME_RECORD_CITY_POPULATION_FLOW turn=%d range=%d-%d player=%d growthEvents=%d populationGained=%d growthPreventedEvents=%d foodDiscardedByAvoidGrowth=%d starvationEvents=%d populationLost=%d netNaturalPopulationChange=%+d",
				iGameTurn, g_iSASGameRecordFlowStartTurn, iGameTurn, ePlayer, kFlow.iCityGrowthEvents, kFlow.iPopulationGainedFromGrowth,
				kFlow.iCityGrowthPreventedEvents, kFlow.iFoodDiscardedByAvoidGrowth, kFlow.iCityStarvationEvents,
				kFlow.iPopulationLostToStarvation, kFlow.iPopulationGainedFromGrowth - kFlow.iPopulationLostToStarvation);
		}
		kFlow.reset();
	}
	for (int iI = MAX_CIV_PLAYERS; iI < MAX_PLAYERS; iI++)
		g_akSASGameRecordPlayerFlow[iI].reset();
	g_iSASGameRecordFlowStartTurn = iGameTurn + 1;
}

// <!-- custom: Project completion rows did not show whether a project-based victory had its minimum/full component set or an active launch countdown.
// Build one compact shared state for periodic progress and the explicit launch action. (GPT-5.6-Sol) -->
static bool getSASGameRecordVictoryProjectState(TeamTypes eTeam, VictoryTypes eVictory, int& iPartsBuilt, int& iPartsMinimum, int& iPartsMaximum, bool& bMinimumComplete, CvString& szProjectParts)
{
	iPartsBuilt = 0;
	iPartsMinimum = 0;
	iPartsMaximum = 0;
	bMinimumComplete = true;
	szProjectParts.clear();
	CvTeam const& kTeam = GET_TEAM(eTeam);
	FOR_EACH_ENUM(Project)
	{
		CvProjectInfo const& kProject = GC.getInfo(eLoopProject);
		int const iMinimum = kProject.getVictoryMinThreshold(eVictory);
		int const iMaximum = kProject.getVictoryThreshold(eVictory);
		if (iMinimum <= 0 && iMaximum <= 0)
			continue;
		int const iBuilt = kTeam.getProjectCount(eLoopProject);
		iPartsBuilt += iBuilt;
		iPartsMinimum += iMinimum;
		iPartsMaximum += iMaximum;
		if (iBuilt < iMinimum)
			bMinimumComplete = false;
		CvString szItem;
		szItem.Format(szProjectParts.empty() ? "%s:%d/%d/%d" : ",%s:%d/%d/%d", getSASGameRecordProjectType(eLoopProject), iBuilt, iMinimum, iMaximum);
		szProjectParts += szItem;
	}
	return !szProjectParts.empty();
}

static char const* getSASGameRecordVictoryType(VictoryTypes eVictory)
{
	return eVictory == NO_VICTORY ? "-" : GC.getInfo(eVictory).getType();
}

typedef std::pair<int, CvCity const*> SASGameRecordCultureCity;

static bool compareSASGameRecordCultureCities(SASGameRecordCultureCity const& kFirst, SASGameRecordCultureCity const& kSecond)
{
	return kFirst.first > kSecond.first;
}

static CvString getSASGameRecordCultureVictoryCities(TeamTypes eTeam, int iRequired, int iThreshold, int& iComplete)
{
	std::vector<SASGameRecordCultureCity> aCities;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer const& kMember = GET_PLAYER((PlayerTypes)iI);
		if (!kMember.isAlive() || kMember.getTeam() != eTeam)
			continue;
		int iLoop = 0;
		for (CvCity const* pCity = kMember.firstCity(&iLoop); pCity != NULL; pCity = kMember.nextCity(&iLoop))
			aCities.push_back(std::make_pair(pCity->getCulture(pCity->getOwner()), pCity));
	}
	std::sort(aCities.begin(), aCities.end(), compareSASGameRecordCultureCities);
	iComplete = 0;
	for (int iI = 0; iI < (int)aCities.size(); iI++)
	{
		if (aCities[iI].first >= iThreshold)
			iComplete++;
	}
	CvString szCities;
	for (int iI = 0; iI < std::min(iRequired, (int)aCities.size()); iI++)
	{
		CvCity const& kCity = *aCities[iI].second;
		CvString szItem;
		szItem.Format(szCities.empty() ? "P%d:C%d@%d:%d=%d/%d" : ",P%d:C%d@%d:%d=%d/%d", kCity.getOwner(), kCity.getID(), kCity.getX(), kCity.getY(), aCities[iI].first, iThreshold);
		szCities += szItem;
	}
	return getSASDiagnosticOrDash(szCities);
}

// <!-- custom: Periodic AreaAI checkpoints preserve the effective post-UWAI theater posture without replaying every transition since game start/load.
// Neutral/uninitialized areas are omitted; absence therefore means no active non-neutral theater posture at this checkpoint. (ChatGPT-5.6-Sol) -->
static void logSASGameRecordAreaAISnapshot(TeamTypes eTeam, int iGameTurn)
{
	CvTeam const& kTeam = GET_TEAM(eTeam);
	if (!kTeam.isAlive())
		return;
	CvString szAreaStates;
	FOR_EACH_AREA(pLoopArea)
	{
		AreaAITypes const eAreaAI = pLoopArea->getAreaAIType(eTeam);
		if (eAreaAI == NO_AREAAI || eAreaAI == AREAAI_NEUTRAL)
			continue;
		CvString szItem;
		szItem.Format(szAreaStates.empty() ? "%d:%s" : ",%d:%s", pLoopArea->getID(), getSASAreaAIType(eAreaAI));
		szAreaStates += szItem;
	}
	logSASGameRecord("GAME_RECORD_AREA_AI turn=%d team=%d areaStates=%s",
		iGameTurn, eTeam, getSASDiagnosticOrDash(szAreaStates).GetCString());
}

// <!-- custom: Player+land-area target cities are persistent AI state consumed by stack movement, city production and UWAI alignment.
// Preserve a compact checkpoint so exact target changes remain reconstructible after load/truncation.
// Only non-null effective targets are listed; `timer` exposes K-Mod's refresh holdoff without invoking any target valuation. (ChatGPT-5.6-Sol) -->
static void logSASGameRecordAITargetCities(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	CvString szTargets;
	int iTargets = 0;
	FOR_EACH_AREA(pLoopArea)
	{
		CvCityAI const* pTargetCity = pLoopArea->AI_getTargetCity(ePlayer);
		if (pTargetCity == NULL)
			continue;
		CvString szItem;
		szItem.Format(szTargets.empty() ? "A%d=P%d:C%d@%d:%d" : ",A%d=P%d:C%d@%d:%d", pLoopArea->getID(),
			pTargetCity->getOwner(), pTargetCity->getID(), pTargetCity->getX(), pTargetCity->getY());
		szTargets += szItem;
		iTargets++;
	}
	logSASGameRecord("GAME_RECORD_AI_TARGET_CITIES turn=%d player=%d team=%d timer=%d targetCount=%d targets=%s",
		iGameTurn, ePlayer, kPlayer.getTeam(), kPlayer.AI_getCityTargetTimer(), iTargets, getSASDiagnosticOrDash(szTargets).GetCString());
}

// <!-- custom: Vassal freedom is threshold/state driven rather than a random "break chance".
// Keep the loaded relationship's stamped land values and current revolt-rule state in one compact periodic row so a truncated GameRecord can explain later independence without BBAI or UI tooltip reconstruction.
// Direct getters only; no diplomacy valuation or RNG. (ChatGPT-5.6-Sol) -->
static void logSASGameRecordVassalStatus(CvTeam const& kVassal, int iGameTurn)
{
	if (!kVassal.isAVassal())
		return;
	TeamTypes const eMaster = kVassal.getMasterTeam();
	if (eMaster == NO_TEAM)
		return;
	CvTeam const& kMaster = GET_TEAM(eMaster);
	int const iVassalLand = kVassal.getTotalLand(false);
	int const iVassalRuleLand = std::max(10, iVassalLand); // Native revolt rules apply this lower bound.
	// <!-- custom: CvTeam uses two different master-land views in the native freedom rules: relative-size checks use own land only, while the master-loss rule and its stamped baseline use getTotalLand() including vassal contribution.
	// Record both explicitly to avoid the same ambiguity that UI percentages can create. (ChatGPT-5.6-Sol) -->
	int const iMasterOwnLand = kMaster.getTotalLand(false);
	int const iMasterLossRuleLand = kMaster.getTotalLand();
	int const iVassalPop = kVassal.getTotalPopulation(false);
	int const iMasterPop = kMaster.getTotalPopulation(false);
	int const iOriginalVassalLand = kVassal.getVassalPower();
	int const iOriginalMasterLossRuleLand = kVassal.getMasterPower();
	bool const bRevoltByRelativeSize = kVassal.canVassalRevolt(eMaster, false);
	bool const bRevoltByLosses = kVassal.isLossesAllowRevolt(eMaster);
	bool const bCanVassalRevolt = kVassal.canVassalRevolt(eMaster, true);
	FAssert(bCanVassalRevolt == (bRevoltByRelativeSize || bRevoltByLosses));
	logSASGameRecord("GAME_RECORD_VASSAL_STATUS turn=%d vassalTeam=%d masterTeam=%d capitulated=%d vassalLand=%d vassalRuleLand=%d originalVassalLand=%d vassalRuleLandVsOriginalPercentX100=%d masterOwnLand=%d masterLossRuleLand=%d originalMasterLossRuleLand=%d masterLossRuleLandVsOriginalPercentX100=%d vassalRuleVsMasterOwnLandPercentX100=%d vassalPop=%d masterPop=%d vassalVsMasterPopPercentX100=%d revoltByRelativeSize=%d revoltByLosses=%d canVassalRevolt=%d",
		iGameTurn, kVassal.getID(), eMaster, kVassal.isCapitulated(), iVassalLand, iVassalRuleLand, iOriginalVassalLand,
		getSASGameRecordPercentX100(iVassalRuleLand, iOriginalVassalLand), iMasterOwnLand, iMasterLossRuleLand, iOriginalMasterLossRuleLand,
		getSASGameRecordPercentX100(iMasterLossRuleLand, iOriginalMasterLossRuleLand), getSASGameRecordPercentX100(iVassalRuleLand, iMasterOwnLand),
		iVassalPop, iMasterPop, getSASGameRecordPercentX100(iVassalPop, iMasterPop),
		bRevoltByRelativeSize, bRevoltByLosses, bCanVassalRevolt);
}

static void logSASGameRecordTeamSnapshot(TeamTypes eTeam, int iGameTurn)
{
	CvGame const& kGame = GC.getGame();
	CvTeam const& kTeam = GET_TEAM(eTeam);
	bool const bLogTeamDetails = (gGameRecordLogLevel >= 2);
	const int iLandPlots = std::max(1, GC.getMap().getLandPlots());
	const int iGamePopulation = std::max(1, kGame.getTotalPopulation());
	const int iTechs = kTeam.getTechCount();
	const int iLand = kTeam.getTotalLand();
	const int iLandPctX100 = (10000 * iLand) / iLandPlots;
	const int iPopulation = kTeam.getTotalPopulation();
	const int iPopPctX100 = (10000 * iPopulation) / iGamePopulation;
	SASGameRecordTeamPrevious& kPrevious = g_akSASGameRecordTeamPrevious[eTeam];
	TeamTypes const eMaster = (kTeam.isAVassal() ? kTeam.getMasterTeam() : NO_TEAM);
	logSASGameRecord("GAME_RECORD_TEAM turn=%d team=%d members=%s alive=%d deltaValid=%d techs=%d techsDelta=%+d techEraCounts=%s techTrading=%d goldTrading=%d land=%d landDelta=%+d landPctX100=%d landPctX100Delta=%+d pop=%d popDelta=%+d popPctX100=%d popPctX100Delta=%+d wars=%s warPlans=%s vassals=%s master=%d",
		iGameTurn, eTeam, getSASGameRecordTeamMembers(eTeam).GetCString(), kTeam.isAlive(), kPrevious.bValid, iTechs,
		getSASGameRecordDelta(kPrevious.bValid, iTechs, kPrevious.iTechs), getSASGameRecordTechEraCounts(eTeam).GetCString(),
		kTeam.isTechTrading(), kTeam.isGoldTrading(), iLand, getSASGameRecordDelta(kPrevious.bValid, iLand, kPrevious.iLand), iLandPctX100,
		getSASGameRecordDelta(kPrevious.bValid, iLandPctX100, kPrevious.iLandPctX100), iPopulation,
		getSASGameRecordDelta(kPrevious.bValid, iPopulation, kPrevious.iPopulation), iPopPctX100,
		getSASGameRecordDelta(kPrevious.bValid, iPopPctX100, kPrevious.iPopPctX100), getSASGameRecordWarTeams(eTeam).GetCString(),
		getSASGameRecordWarPlans(eTeam).GetCString(), getSASGameRecordVassalTeams(eTeam).GetCString(), eMaster);
	if (bLogTeamDetails)
	{
		logSASGameRecordTeamContacts(eTeam, iGameTurn, "snapshot");
		logSASGameRecordAreaAISnapshot(eTeam, iGameTurn);
		if (kTeam.isAVassal()) logSASGameRecordVassalStatus(kTeam, iGameTurn);
	}
	seedSASGameRecordTeamPreviousFromCurrentState(eTeam);

	VictoryTypes eScoreVictory = NO_VICTORY;
	VictoryTypes eTimeVictory = NO_VICTORY;
	VictoryTypes eConquestVictory = NO_VICTORY;
	VictoryTypes eCultureVictory = NO_VICTORY;
	VictoryTypes eDiplomaticVictory = NO_VICTORY;
	int iCultureCitiesRequired = 0;
	int iCultureThreshold = 0;
	FOR_EACH_ENUM(Victory)
	{
		if (!kGame.isVictoryValid(eLoopVictory))
			continue;
		CvVictoryInfo const& kVictory = GC.getInfo(eLoopVictory);
		if (kVictory.isTargetScore()) eScoreVictory = eLoopVictory;
		if (kVictory.isEndScore()) eTimeVictory = eLoopVictory;
		if (kVictory.isConquest()) eConquestVictory = eLoopVictory;
		if (kVictory.isDiploVote()) eDiplomaticVictory = eLoopVictory;
		if (kVictory.getCityCulture() != NO_CULTURELEVEL && kVictory.getNumCultureCities() > 0)
		{
			eCultureVictory = eLoopVictory;
			iCultureCitiesRequired = kVictory.getNumCultureCities();
			iCultureThreshold = kGame.getCultureThreshold((CultureLevelTypes)kVictory.getCityCulture());
		}
	}
	CvString szConquestRivals;
	int iConquestRivalCities = 0;
	if (eConquestVictory != NO_VICTORY)
	{
		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			TeamTypes const eRival = (TeamTypes)iI;
			CvTeam const& kRival = GET_TEAM(eRival);
			if (eRival == eTeam || !kRival.isAlive() || kRival.isBarbarian() || kRival.isVassal(eTeam) || kRival.getNumCities() <= 0)
				continue;
			appendSASDiagnosticIntListValue(szConquestRivals, eRival);
			iConquestRivalCities += kRival.getNumCities();
		}
	}
	int iBestRivalScore = -1;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		TeamTypes const eRival = (TeamTypes)iI;
		if (eRival != eTeam && GET_TEAM(eRival).isAlive() && !GET_TEAM(eRival).isBarbarian())
			iBestRivalScore = std::max(iBestRivalScore, kGame.getTeamScore(eRival));
	}
	int const iTeamScore = kGame.getTeamScore(eTeam);
	int const iTurnsRemaining = (kGame.getMaxTurns() <= 0 ? -1 : std::max(0, kGame.getMaxTurns() - kGame.getElapsedGameTurns()));
	int iCultureCitiesComplete = 0;
	CvString szCultureCities;
	if (eCultureVictory == NO_VICTORY) szCultureCities = "-";
	else szCultureCities = getSASGameRecordCultureVictoryCities(eTeam, iCultureCitiesRequired, iCultureThreshold, iCultureCitiesComplete);
	// <!-- custom: Domination and Space already have detailed per-victory rows, and diplomatic vote-source rows already contain exact vote thresholds.
	// Add one compact general row per team rather than one new row per missing victory, so Score/Time, Conquest, and Cultural progress become explicit without multiplying snapshot noise.
	// Culture lists only the required number of leading cities. (GPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_VICTORY_PROGRESS_GENERAL turn=%d team=%d scoreVictory=%s timeVictory=%s conquestVictory=%s culturalVictory=%s diplomaticVictory=%s teamScore=%d bestRivalScore=%d scoreLead=%+d targetScore=%d turnsRemaining=%d conquestRivals=%s conquestRivalCities=%d cultureCitiesComplete=%d cultureCitiesRequired=%d cultureThreshold=%d cultureCities=%s",
		iGameTurn, eTeam, getSASGameRecordVictoryType(eScoreVictory), getSASGameRecordVictoryType(eTimeVictory),
		getSASGameRecordVictoryType(eConquestVictory), getSASGameRecordVictoryType(eCultureVictory),
		getSASGameRecordVictoryType(eDiplomaticVictory), iTeamScore, iBestRivalScore,
		iBestRivalScore < 0 ? iTeamScore : iTeamScore - iBestRivalScore, kGame.getTargetScore(), iTurnsRemaining,
		getSASDiagnosticOrDash(szConquestRivals).GetCString(), iConquestRivalCities, iCultureCitiesComplete, iCultureCitiesRequired,
		iCultureThreshold, szCultureCities.GetCString());

	FOR_EACH_ENUM(Victory)
	{
		if (!kGame.isVictoryValid(eLoopVictory))
			continue;
		const int iLandNeed = kGame.getAdjustedLandPercent(eLoopVictory);
		const int iPopNeed = kGame.getAdjustedPopulationPercent(eLoopVictory);
		int iPartsBuilt = 0;
		int iPartsMinimum = 0;
		int iPartsMaximum = 0;
		bool bMinimumComplete = false;
		CvString szProjectParts;
		bool const bProjectVictory = getSASGameRecordVictoryProjectState(eTeam, eLoopVictory, iPartsBuilt, iPartsMinimum, iPartsMaximum, bMinimumComplete, szProjectParts);
		if (iLandNeed > 0 || iPopNeed > 0 || bProjectVictory)
		{
			int const iCountdown = kTeam.getVictoryCountdown(eLoopVictory);
			int const iTravelTurns = (bProjectVictory && bMinimumComplete ? kTeam.getVictoryDelay(eLoopVictory) : -1);
			logSASGameRecord("GAME_RECORD_VICTORY_PROGRESS turn=%d team=%d victory=%s landPctX100=%d landNeed=%d popPctX100=%d popNeed=%d projectVictory=%d launched=%d countdown=%d arrivalTurn=%d canLaunch=%d launchSuccessPercent=%d travelTurns=%d partsBuilt=%d partsMinimum=%d partsMaximum=%d projectParts=%s",
				iGameTurn, eTeam, GC.getInfo(eLoopVictory).getType(), iLandPctX100, iLandNeed, iPopPctX100, iPopNeed, bProjectVictory,
				bProjectVictory && iCountdown >= 0, iCountdown, iCountdown < 0 ? -1 : iGameTurn + iCountdown,
				bProjectVictory && kTeam.canLaunch(eLoopVictory), bProjectVictory ? kTeam.getLaunchSuccessRate(eLoopVictory) : -1,
				iTravelTurns, iPartsBuilt, iPartsMinimum, iPartsMaximum, bProjectVictory ? szProjectParts.GetCString() : "-");
		}
	}
	if (bLogTeamDetails) logSASGameRecordTeamProjects(eTeam, iGameTurn);
}



static CvString getSASGameRecordCivicList(CvPlayer const& kPlayer)
{
	CvString szList;
	FOR_EACH_ENUM(CivicOption)
	{
		CivicTypes eCivic = kPlayer.getCivics(eLoopCivicOption);
		if (eCivic == NO_CIVIC)
			continue;
		CvString szItem;
		szItem.Format(szList.empty() ? "%s:%s" : ",%s:%s", GC.getInfo(eLoopCivicOption).getType(), getSASGameRecordCivicType(eCivic));
		szList += szItem;
	}
	return getSASDiagnosticOrDash(szList);
}

static CvString getSASGameRecordPlayerCityReligions(CvPlayer const& kPlayer)
{
	std::vector<int> aiCounts(GC.getNumReligionInfos(), 0);
	int iLoop = 0;
	for (CvCity const* pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
	{
		FOR_EACH_ENUM(Religion)
		{
			if (pLoopCity->isHasReligion(eLoopReligion))
				aiCounts[eLoopReligion]++;
		}
	}
	CvString szList;
	FOR_EACH_ENUM(Religion)
		appendSASGameRecordTypeCount(szList, getSASGameRecordReligionType(eLoopReligion), aiCounts[eLoopReligion]);
	return getSASDiagnosticOrDash(szList);
}

static CvString getSASGameRecordPlayerCityCorporations(CvPlayer const& kPlayer)
{
	std::vector<int> aiCounts(GC.getNumCorporationInfos(), 0);
	int iLoop = 0;
	for (CvCity const* pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
	{
		FOR_EACH_ENUM(Corporation)
		{
			if (pLoopCity->isHasCorporation(eLoopCorporation))
				aiCounts[eLoopCorporation]++;
		}
	}
	CvString szList;
	FOR_EACH_ENUM(Corporation)
		appendSASGameRecordTypeCount(szList, getSASGameRecordCorporationType(eLoopCorporation), aiCounts[eLoopCorporation]);
	return getSASDiagnosticOrDash(szList);
}

// <!-- custom: City health/happiness rows previously combined player-wide modifiers under `extra`, hiding whether a loaded-mod rule caused a demographic change; for example, AdvCiv-SAS's TECH_DEPOPULATION currently applies negative health and happiness.
// Reconstruct all currently defined trait, civic and technology contributions once per player snapshot; preserve any event or other DLL adjustment as OTHER. (GPT-5.6-Sol) -->
static void getSASGameRecordPlayerExtraSources(CvPlayer const& kPlayer, CvString& szHealthSources, CvString& szHappinessSources)
{
	int iKnownHealth = 0;
	int iKnownHappiness = 0;
	FOR_EACH_ENUM(Trait)
	{
		if (!kPlayer.hasTrait(eLoopTrait))
			continue;
		CvTraitInfo const& kTrait = GC.getInfo(eLoopTrait);
		iKnownHealth += kTrait.getHealth();
		iKnownHappiness += kTrait.getHappiness();
		appendSASGameRecordSignedValue(szHealthSources, kTrait.getType(), kTrait.getHealth());
		appendSASGameRecordSignedValue(szHappinessSources, kTrait.getType(), kTrait.getHappiness());
	}
	FOR_EACH_ENUM(CivicOption)
	{
		CivicTypes const eCivic = kPlayer.getCivics(eLoopCivicOption);
		if (eCivic == NO_CIVIC)
			continue;
		CvCivicInfo const& kCivic = GC.getInfo(eCivic);
		iKnownHealth += kCivic.getExtraHealth();
		iKnownHappiness += kCivic.getExtraHappiness();
		appendSASGameRecordSignedValue(szHealthSources, kCivic.getType(), kCivic.getExtraHealth());
		appendSASGameRecordSignedValue(szHappinessSources, kCivic.getType(), kCivic.getExtraHappiness());
	}
	CvTeam const& kTeam = GET_TEAM(kPlayer.getTeam());
	FOR_EACH_ENUM(Tech)
	{
		if (!kTeam.isHasTech(eLoopTech))
			continue;
		CvTechInfo const& kTech = GC.getInfo(eLoopTech);
		iKnownHealth += kTech.getHealth();
		iKnownHappiness += kTech.getHappiness();
		appendSASGameRecordSignedValue(szHealthSources, kTech.getType(), kTech.getHealth());
		appendSASGameRecordSignedValue(szHappinessSources, kTech.getType(), kTech.getHappiness());
	}
	appendSASGameRecordSignedValue(szHealthSources, "OTHER", kPlayer.getExtraHealth() - iKnownHealth);
	appendSASGameRecordSignedValue(szHappinessSources, "OTHER", kPlayer.getExtraHappiness() - iKnownHappiness);
}

// <!-- custom: Periodic checkpoints make the complete final AI strategy state readable without replaying every transition since game start/load.
// Exclude AI_DEFAULT_STRATEGY because it is always present in a valid strategy hash; exact transition rows below cover only meaningful non-default flags. (ChatGPT-5.6-Sol) -->
static void logSASGameRecordAIStrategies(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	if (!isSASGameRecordAIStrategyPlayer(kPlayer))
		return;
	CvString szStrategies;
	for (int iStrategy = AI_STRATEGY_DAGGER; iStrategy <= AI_STRATEGY_ESPIONAGE_ECONOMY; iStrategy <<= 1)
	{
		AIStrategy const eStrategy = (AIStrategy)iStrategy;
		if (!isSASGameRecordAIStrategyActive(kPlayer, eStrategy))
			continue;
		if (!szStrategies.empty())
			szStrategies += ",";
		szStrategies += getSASAIStrategyType(eStrategy);
	}
	logSASGameRecord("GAME_RECORD_AI_STRATEGIES turn=%d player=%d team=%d strategies=%s",
		iGameTurn, ePlayer, kPlayer.getTeam(), getSASDiagnosticOrDash(szStrategies).GetCString());
}

// <!-- custom: Objective victory progress does not show which route currently guides AI strategy.
// Record the compact 0..4 route stages once per AI snapshot so city production and war choices can be interpreted without enabling detailed BBAI decisions. (GPT-5.6-Sol) -->
static void logSASGameRecordAIVictoryStages(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	if (!isSASGameRecordAIVictoryStagePlayer(kPlayer))
		return;
	AIVictoryStage const eStages = kPlayer.AI_getVictoryStageHash();
	int const iCultureStage = getSASCultureVictoryStageLevel(eStages);
	int const iSpaceStage = getSASSpaceVictoryStageLevel(eStages);
	int const iConquestStage = getSASConquestVictoryStageLevel(eStages);
	int const iDominationStage = getSASDominationVictoryStageLevel(eStages);
	int const iDiplomacyStage = getSASDiplomacyVictoryStageLevel(eStages);
	int const iPlayerMaxStage = std::max(std::max(iCultureStage, iSpaceStage), std::max(std::max(iConquestStage, iDominationStage), iDiplomacyStage));
	logSASGameRecord("GAME_RECORD_AI_VICTORY_STAGES turn=%d player=%d team=%d playerMaxStage=%d teamMaxStage=%d culture=%d space=%d conquest=%d domination=%d diplomacy=%d",
		iGameTurn, ePlayer, kPlayer.getTeam(), iPlayerMaxStage, getSASTeamMaxVictoryStage(kPlayer.getTeam()), iCultureStage, iSpaceStage,
		iConquestStage, iDominationStage, iDiplomacyStage);
}

// <!-- custom: Keep a compact periodic military-production pressure snapshot in the GameRecord so low/high army phases can be diagnosed even without detailed BBAI logging.
// The no-area maximum is a player-level reference; AI_chooseProduction can use a different city-area ceiling. (ChatGPT-5.6-Sol) -->
static void logSASGameRecordAIMilitaryProduction(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	if (kPlayer.isHuman() && !kPlayer.isHumanDisabled())
		return;
	CvTeamAI const& kTeam = GET_TEAM(kPlayer.getTeam());
	int const iPersonalityBuildProb = GC.getInfo(kPlayer.getPersonalityType()).getBuildUnitProb();
	int const iUnitSpending = kPlayer.AI_unitCostPerMil();
	int const iMaxUnitSpendingNoArea = kPlayer.AI_maxUnitCostPerMil();
	logSASGameRecord("GAME_RECORD_AI_MILITARY_PRODUCTION turn=%d player=%d personalityBuildProb=%d unitSpending=%d maxUnitSpendingNoArea=%d spendingGapNoArea=%d aggressiveAI=%d financialTrouble=%d economyFocus=%d getBetterUnits=%d focusWar=%d dagger=%d crush=%d alert1=%d alert2=%d turtle=%d lastStand=%d finalWar=%d fastMovers=%d landBlitz=%d airBlitz=%d nuclear=%d totalWarPlans=%d preparingTotalWarPlans=%d sneakPreparing=%d sneakReady=%d",
		iGameTurn, ePlayer, iPersonalityBuildProb, iUnitSpending, iMaxUnitSpendingNoArea, iMaxUnitSpendingNoArea - iUnitSpending,
		GC.getGame().isOption(GAMEOPTION_AGGRESSIVE_AI), kPlayer.AI_isFinancialTrouble(),
		isSASGameRecordAIStrategyActive(kPlayer, AI_STRATEGY_ECONOMY_FOCUS),
		isSASGameRecordAIStrategyActive(kPlayer, AI_STRATEGY_GET_BETTER_UNITS), kPlayer.AI_isFocusWar(),
		isSASGameRecordAIStrategyActive(kPlayer, AI_STRATEGY_DAGGER), isSASGameRecordAIStrategyActive(kPlayer, AI_STRATEGY_CRUSH),
		isSASGameRecordAIStrategyActive(kPlayer, AI_STRATEGY_ALERT1), isSASGameRecordAIStrategyActive(kPlayer, AI_STRATEGY_ALERT2),
		isSASGameRecordAIStrategyActive(kPlayer, AI_STRATEGY_TURTLE), isSASGameRecordAIStrategyActive(kPlayer, AI_STRATEGY_LAST_STAND),
		isSASGameRecordAIStrategyActive(kPlayer, AI_STRATEGY_FINAL_WAR), isSASGameRecordAIStrategyActive(kPlayer, AI_STRATEGY_FASTMOVERS),
		isSASGameRecordAIStrategyActive(kPlayer, AI_STRATEGY_LAND_BLITZ), isSASGameRecordAIStrategyActive(kPlayer, AI_STRATEGY_AIR_BLITZ),
		isSASGameRecordAIStrategyActive(kPlayer, AI_STRATEGY_OWABWNW), kTeam.AI_getNumWarPlans(WARPLAN_TOTAL),
		kTeam.AI_getNumWarPlans(WARPLAN_PREPARING_TOTAL), kTeam.AI_isSneakAttackPreparing(), kTeam.AI_isSneakAttackReady());
}

static void logSASGameRecordPolicies(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	CvString szExtraHealthSources;
	CvString szExtraHappinessSources;
	getSASGameRecordPlayerExtraSources(kPlayer, szExtraHealthSources, szExtraHappinessSources);
	logSASGameRecord("GAME_RECORD_POLICIES turn=%d player=%d civics=%s stateReligion=%s cityReligions=%s cityCorporations=%s playerExtraHealth=%d playerExtraHappiness=%d extraHealthSources=%s extraHappinessSources=%s",
		iGameTurn, ePlayer, getSASGameRecordCivicList(kPlayer).GetCString(), getSASGameRecordReligionType(kPlayer.getStateReligion()),
		getSASGameRecordPlayerCityReligions(kPlayer).GetCString(), getSASGameRecordPlayerCityCorporations(kPlayer).GetCString(),
		kPlayer.getExtraHealth(), kPlayer.getExtraHappiness(), getSASDiagnosticOrDash(szExtraHealthSources).GetCString(),
		getSASDiagnosticOrDash(szExtraHappinessSources).GetCString());
}


static CorporationTypes getSASGameRecordExecutiveCorporation(UnitTypes eUnit);

// <!-- custom: A corporation-spread MissionAI may belong either to the Executive's own group or to a sea transport carrying the Executive.
// Resolve the factual Executive from current group/cargo state without any AI target/value call. (ChatGPT-5.6-Sol) -->
static CvUnit const* getSASGameRecordCorporationMissionExecutive(CvSelectionGroup const* pGroup)
{
	if (pGroup == NULL)
		return NULL;
	CvUnit const* pHeadUnit = pGroup->getHeadUnit();
	if (pHeadUnit != NULL && pHeadUnit->AI_getUnitAIType() == UNITAI_MISSIONARY &&
		getSASGameRecordExecutiveCorporation(pHeadUnit->getUnitType()) != NO_CORPORATION)
	{
		return pHeadUnit;
	}
	CvPlot const* pPlot = pGroup->plot();
	if (pPlot == NULL)
		return NULL;
	FOR_EACH_UNIT_IN(pLoopUnit, *pPlot)
	{
		if (pLoopUnit->getOwner() != pGroup->getOwner() || pLoopUnit->AI_getUnitAIType() != UNITAI_MISSIONARY ||
			getSASGameRecordExecutiveCorporation(pLoopUnit->getUnitType()) == NO_CORPORATION)
		{
			continue;
		}
		CvUnit const* pTransport = pLoopUnit->getTransportUnit();
		if (pTransport != NULL && pTransport->getGroup() == pGroup)
			return pLoopUnit;
	}
	return NULL;
}

// <!-- custom: Corporation outcome rows say what ultimately spread; this periodic posture preserves long-lived Executive missions so a loaded-save analysis can still see where existing Executives are heading or whether they are already waiting in a target city.
// Only current unit/group/cargo state and cheap spread-cost/eligibility checks are used here; no corporation valuation, target search or pathfinding is repeated for logging. (ChatGPT-5.6-Sol) -->
static void logSASGameRecordCorporationPosture(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer).AI();
	// <!-- custom: isActiveCorporation means policy-enabled for this player, not actually founded/present.
	// Keep that useful rules-state separate from factual city presence so the posture row stays dormant before corporations exist. (ChatGPT-5.6-Sol) -->
	int iEnabledCorporations = 0;
	int iCorporationTypesPresent = 0;
	int iCorporationCityInstances = 0;
	int iTeamHeadquarters = 0;
	FOR_EACH_ENUM(Corporation)
	{
		if (kPlayer.isActiveCorporation(eLoopCorporation))
			iEnabledCorporations++;
		int iCorporationCities = 0;
		int iCityLoop = 0;
		for (CvCity const* pLoopCity = kPlayer.firstCity(&iCityLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iCityLoop))
		{
			if (pLoopCity->isHasCorporation(eLoopCorporation))
				iCorporationCities++;
		}
		if (iCorporationCities > 0)
			iCorporationTypesPresent++;
		iCorporationCityInstances += iCorporationCities;
		if (GET_TEAM(kPlayer.getTeam()).hasHeadquarters(eLoopCorporation))
			iTeamHeadquarters++;
	}
	int iExecutives = 0;
	int iExecutivesOnSpreadMission = 0;
	int iUnitLoop = 0;
	for (CvUnit const* pLoopUnit = kPlayer.firstUnit(&iUnitLoop); pLoopUnit != NULL; pLoopUnit = kPlayer.nextUnit(&iUnitLoop))
	{
		if (pLoopUnit->AI_getUnitAIType() == UNITAI_MISSIONARY && getSASGameRecordExecutiveCorporation(pLoopUnit->getUnitType()) != NO_CORPORATION)
			iExecutives++;
	}
	int iSpreadMissionGroups = 0;
	int iSpreadMissionGroupsAtTarget = 0;
	int iSpreadMissionGroupsWaitingGold = 0;
	int iSpreadMissionGroupsReady = 0;
	CvString szTargets;
	bool const bLogTargets = (gGameRecordLogLevel >= 3);
	int iGroupLoop = 0;
	for (CvSelectionGroup const* pLoopGroup = kPlayer.firstSelectionGroup(&iGroupLoop); pLoopGroup != NULL; pLoopGroup = kPlayer.nextSelectionGroup(&iGroupLoop))
	{
		if (pLoopGroup->AI().AI_getMissionAIType() != MISSIONAI_SPREAD_CORPORATION)
			continue;
		CvPlot const* pMissionPlot = pLoopGroup->AI().AI_getMissionAIPlot();
		CvUnit const* pExecutive = getSASGameRecordCorporationMissionExecutive(pLoopGroup);
		CorporationTypes const eCorporation = (pExecutive == NULL ? NO_CORPORATION : getSASGameRecordExecutiveCorporation(pExecutive->getUnitType()));
		if (pMissionPlot == NULL || pExecutive == NULL || eCorporation == NO_CORPORATION)
			continue;
		iSpreadMissionGroups++;
		iExecutivesOnSpreadMission++;
		CvCity const* pTargetCity = pMissionPlot->getPlotCity();
		bool const bAtTarget = pExecutive->at(*pMissionPlot);
		if (bAtTarget) iSpreadMissionGroupsAtTarget++;
		int iSpreadCost = -1;
		if (bAtTarget && pTargetCity != NULL && pExecutive->canSpreadCorporation(pMissionPlot, eCorporation, true))
		{
			iSpreadCost = pExecutive->spreadCorporationCost(eCorporation, pTargetCity);
			if (kPlayer.getGold() < iSpreadCost) iSpreadMissionGroupsWaitingGold++;
			else if (pExecutive->canSpreadCorporation(pMissionPlot, eCorporation)) iSpreadMissionGroupsReady++;
		}
		if (bLogTargets)
		{
			if (!szTargets.empty()) szTargets += ";";
			CvPlot const* pCurrentPlot = pLoopGroup->plot();
			CvUnit const* pTransport = pExecutive->getTransportUnit();
			CvString szTarget;
			szTarget.Format("%d:exec%d:transport%d:%s@(%d,%d)>(%d,%d):player%d:city%d:at%d:gold%d/cost%d",
				pLoopGroup->getID(), pExecutive->getID(), pTransport == NULL ? -1 : pTransport->getID(), getSASGameRecordCorporationType(eCorporation),
				pCurrentPlot == NULL ? -1 : pCurrentPlot->getX(), pCurrentPlot == NULL ? -1 : pCurrentPlot->getY(), pMissionPlot->getX(), pMissionPlot->getY(),
				pTargetCity == NULL ? NO_PLAYER : pTargetCity->getOwner(), pTargetCity == NULL ? -1 : pTargetCity->getID(), bAtTarget ? 1 : 0, kPlayer.getGold(), iSpreadCost);
			szTargets += szTarget;
		}
	}
	if (iCorporationTypesPresent <= 0 && iTeamHeadquarters <= 0 && iExecutives <= 0 && iSpreadMissionGroups <= 0)
		return;
	logSASGameRecord("GAME_RECORD_CORPORATION_POSTURE turn=%d player=%d team=%d enabledCorporations=%d corporationTypesPresent=%d corporationCityInstances=%d teamHeadquarters=%d executives=%d executivesOnSpreadMission=%d idleExecutives=%d spreadMissionGroups=%d spreadMissionGroupsAtTarget=%d spreadMissionGroupsWaitingGold=%d spreadMissionGroupsReady=%d targets=%s",
		iGameTurn, ePlayer, kPlayer.getTeam(), iEnabledCorporations, iCorporationTypesPresent, iCorporationCityInstances, iTeamHeadquarters,
		iExecutives, iExecutivesOnSpreadMission, std::max(0, iExecutives - iExecutivesOnSpreadMission), iSpreadMissionGroups,
		iSpreadMissionGroupsAtTarget, iSpreadMissionGroupsWaitingGold, iSpreadMissionGroupsReady,
		bLogTargets ? getSASDiagnosticOrDash(szTargets).GetCString() : "-");
}

static void logSASGameRecordEspionage(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	CvTeam const& kTeam = GET_TEAM(kPlayer.getTeam());
	SASGameRecordPlayerPrevious& kPrevious = g_akSASGameRecordPlayerPrevious[ePlayer];
	CvString szWeights;
	CvString szSpending;
	CvString szPoints;
	CvString szModifiers;
	// <!-- custom: EP totals alone do not show whether Spies are reaching rivals or remaining idle at home.
	// At periodic level-2 snapshots, summarize foreign deployment, city infiltration, stationary cost-reduction preparation, and both current and MissionAI rival targets without reproducing detailed movement/target valuation. (GPT-5.6-Sol + ChatGPT-5.6-Sol) -->
	CvString szSpyTargets;
	CvString szSpyMissionTargets;
	std::vector<int> aiSpiesAgainstPlayer(MAX_PLAYERS, 0);
	std::vector<int> aiSpiesTargetingPlayer(MAX_PLAYERS, 0);
	int iSpies = 0;
	int iGreatSpies = 0;
	int iSpiesInForeignTerritory = 0;
	int iSpiesInForeignCities = 0;
	int iStationarySpies = 0;
	int iMaxFortifyTurns = 0;
	int iAttackSpyIntent = 0;
	int iReconSpyIntent = 0;
	int iGuardSpyIntent = 0;
	int iUnitLoop = 0;
	for (CvUnit const* pLoopUnit = kPlayer.firstUnit(&iUnitLoop); pLoopUnit != NULL; pLoopUnit = kPlayer.nextUnit(&iUnitLoop))
	{
		UnitAITypes const eUnitAI = pLoopUnit->AI_getUnitAIType();
		if (!pLoopUnit->isSpy() && eUnitAI != UNITAI_GREAT_SPY)
			continue;
		iSpies++;
		if (eUnitAI == UNITAI_GREAT_SPY)
			iGreatSpies++;
		CvPlot const& kPlot = pLoopUnit->getPlot();
		// <!-- custom: Fortified ordinary Spies at home and fortified Great Spies inflated the stationary mission-discount diagnostic even though they were not preparing a valid foreign espionage mission.
		// Count fortify turns only for ordinary Spies on a structurally valid mission plot; bTestVisible deliberately ignores whether the mission button is usable at this exact snapshot. See KI#376. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if (pLoopUnit->isSpy() && pLoopUnit->getFortifyTurns() > 0 && pLoopUnit->canEspionage(&kPlot, true))
		{
			iStationarySpies++;
			iMaxFortifyTurns = std::max(iMaxFortifyTurns, pLoopUnit->getFortifyTurns());
		}
		PlayerTypes const ePlotOwner = kPlot.getOwner();
		if (ePlotOwner != NO_PLAYER && kPlot.getTeam() != kPlayer.getTeam())
		{
			iSpiesInForeignTerritory++;
			if (kPlot.isCity())
				iSpiesInForeignCities++;
			aiSpiesAgainstPlayer[ePlotOwner]++;
		}
		// <!-- custom: The player unit iterator exposes CvUnit, so access CvUnitAI through its AI facade when reading group MissionAI metadata. (GPT-5.6-Sol + ChatGPT-5.6-Sol) -->
		CvSelectionGroupAI const* pGroup = pLoopUnit->AI().AI_getGroup();
		MissionAITypes const eMissionAI = (pGroup == NULL ? NO_MISSIONAI : pGroup->AI_getMissionAIType());
		if (eMissionAI == MISSIONAI_ATTACK_SPY)
		{
			iAttackSpyIntent++;
			CvPlot const* pMissionPlot = pGroup->AI_getMissionAIPlot();
			PlayerTypes const eTargetPlayer = (pMissionPlot == NULL ? NO_PLAYER : pMissionPlot->getOwner());
			if (eTargetPlayer != NO_PLAYER && GET_PLAYER(eTargetPlayer).getTeam() != kPlayer.getTeam())
				aiSpiesTargetingPlayer[eTargetPlayer]++;
		}
		else if (eMissionAI == MISSIONAI_RECON_SPY)
			iReconSpyIntent++;
		else if (eMissionAI == MISSIONAI_GUARD_SPY)
			iGuardSpyIntent++;
	}
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (aiSpiesAgainstPlayer[iI] > 0)
		{
			CvString szItem;
			szItem.Format(szSpyTargets.empty() ? "%d:%d" : ",%d:%d", iI, aiSpiesAgainstPlayer[iI]);
			szSpyTargets += szItem;
		}
		if (aiSpiesTargetingPlayer[iI] > 0)
		{
			CvString szItem;
			szItem.Format(szSpyMissionTargets.empty() ? "%d:%d" : ",%d:%d", iI, aiSpiesTargetingPlayer[iI]);
			szSpyMissionTargets += szItem;
		}
	}
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		TeamTypes eLoopTeam = (TeamTypes)iI;
		if (eLoopTeam == kPlayer.getTeam() || !GET_TEAM(eLoopTeam).isAlive() || GET_TEAM(eLoopTeam).isBarbarian())
			continue;
		const int iWeight = kPlayer.getEspionageSpendingWeightAgainstTeam(eLoopTeam);
		const int iSpending = kTeam.isHasMet(eLoopTeam) ? kPlayer.getEspionageSpending(eLoopTeam) : -1;
		const int iPoints = kTeam.getEspionagePointsAgainstTeam(eLoopTeam);
		const int iModifier = kTeam.getEspionageModifier(eLoopTeam);
		if (iWeight > 0)
		{
			CvString szItem;
			szItem.Format(szWeights.empty() ? "%d:%d" : ",%d:%d", eLoopTeam, iWeight);
			szWeights += szItem;
		}
		if (iSpending > 0)
		{
			CvString szItem;
			szItem.Format(szSpending.empty() ? "%d:%d" : ",%d:%d", eLoopTeam, iSpending);
			szSpending += szItem;
		}
		if (iPoints > 0)
		{
			CvString szItem;
			szItem.Format(szPoints.empty() ? "%d:%d" : ",%d:%d", eLoopTeam, iPoints);
			szPoints += szItem;
		}
		if (iModifier != 0)
		{
			CvString szItem;
			szItem.Format(szModifiers.empty() ? "%d:%+d" : ",%d:%+d", eLoopTeam, iModifier);
			szModifiers += szItem;
		}
	}
	const int iEspionageRate = kPlayer.getCommerceRate(COMMERCE_ESPIONAGE);
	const int iEspionagePercent = kPlayer.getCommercePercent(COMMERCE_ESPIONAGE);
	const int iTeamEP = kTeam.getEspionagePointsEver();
	const int iUnspentEP = kTeam.getTotalUnspentEspionage();
	// <!-- custom: Weights show intent but not the rounded EP distribution that the game actually applies.
	// Record actual per-rival spending plus the two high-level espionage strategy flags; detailed reasons for enabling those strategies remain BBAI territory. (ChatGPT-5.6-Sol) -->
	const bool bBigEspionage = isSASGameRecordAIStrategyActive(kPlayer, AI_STRATEGY_BIG_ESPIONAGE);
	const bool bEspionageEconomy = isSASGameRecordAIStrategyActive(kPlayer, AI_STRATEGY_ESPIONAGE_ECONOMY);
	logSASGameRecord("GAME_RECORD_ESPIONAGE turn=%d player=%d team=%d espionageRate=%d espionagePercent=%d teamEP=%d unspentEP=%d weights=%s spending=%s pointsAgainst=%s modifiers=%s bigEspionage=%d espionageEconomy=%d spies=%d greatSpies=%d spiesInForeignTerritory=%d spiesInForeignCities=%d stationarySpies=%d maxFortifyTurns=%d spyTargets=%s attackSpyIntent=%d spyMissionTargets=%s reconSpyIntent=%d guardSpyIntent=%d",
		iGameTurn, ePlayer, kPlayer.getTeam(), iEspionageRate, iEspionagePercent, iTeamEP, iUnspentEP,
		getSASDiagnosticOrDash(szWeights).GetCString(), getSASDiagnosticOrDash(szSpending).GetCString(),
		getSASDiagnosticOrDash(szPoints).GetCString(), getSASDiagnosticOrDash(szModifiers).GetCString(), bBigEspionage, bEspionageEconomy,
		iSpies, iGreatSpies, iSpiesInForeignTerritory, iSpiesInForeignCities, iStationarySpies, iMaxFortifyTurns,
		getSASDiagnosticOrDash(szSpyTargets).GetCString(), iAttackSpyIntent, getSASDiagnosticOrDash(szSpyMissionTargets).GetCString(),
		iReconSpyIntent, iGuardSpyIntent);
	logSASGameRecord("GAME_RECORD_ESPIONAGE_DELTAS turn=%d player=%d deltaValid=%d espionageRateDelta=%+d espionagePercentDelta=%+d teamEPDelta=%+d unspentEPDelta=%+d",
		iGameTurn, ePlayer, kPrevious.bValid, getSASGameRecordDelta(kPrevious.bValid, iEspionageRate, kPrevious.iEspionageRate),
		getSASGameRecordDelta(kPrevious.bValid, iEspionagePercent, kPrevious.iEspionagePercent),
		getSASGameRecordDelta(kPrevious.bValid, iTeamEP, kPrevious.iTeamEP),
		getSASGameRecordDelta(kPrevious.bValid, iUnspentEP, kPrevious.iUnspentEP));
	kPrevious.iEspionageRate = iEspionageRate;
	kPrevious.iEspionagePercent = iEspionagePercent;
	kPrevious.iTeamEP = iTeamEP;
	kPrevious.iUnspentEP = iUnspentEP;
}

static CvString getSASGameRecordCommercePercents(CvPlayer const& kPlayer)
{
	CvString szList;
	FOR_EACH_ENUM(Commerce)
		appendSASGameRecordValue(szList, getSASGameRecordCommerceType(eLoopCommerce), kPlayer.getCommercePercent(eLoopCommerce));
	return getSASDiagnosticOrDash(szList);
}

static CvString getSASGameRecordCommerceRates(CvPlayer const& kPlayer)
{
	CvString szList;
	FOR_EACH_ENUM(Commerce)
		appendSASGameRecordValue(szList, getSASGameRecordCommerceType(eLoopCommerce), kPlayer.getCommerceRate(eLoopCommerce));
	return getSASDiagnosticOrDash(szList);
}

static CvString getSASGameRecordCommerceFlexible(CvPlayer const& kPlayer)
{
	CvString szList;
	FOR_EACH_ENUM(Commerce)
		appendSASGameRecordValue(szList, getSASGameRecordCommerceType(eLoopCommerce), kPlayer.isCommerceFlexible(eLoopCommerce));
	return getSASDiagnosticOrDash(szList);
}

static void logSASGameRecordEconomy(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	CvTeam const& kTeam = GET_TEAM(kPlayer.getTeam());
	TechTypes eResearch = kPlayer.getCurrentResearch();
	int const iResearchProgress = (eResearch == NO_TECH ? -1 : kTeam.getResearchProgress(eResearch));
	int const iResearchCost = (eResearch == NO_TECH ? -1 : kTeam.getResearchCost(eResearch));
	// <!-- custom: currentResearch=- does not mean that science is lost: CvPlayer::doResearch stores the nominal research rate as overflow until another technology can be selected.
	// Exact shared progress/cost makes partially researched technologies visible at ordinary snapshots instead of only when completion or redirection happens. (GPT-5.6-Sol + ChatGPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_ECONOMY turn=%d player=%d gold=%d goldRate=%d totalCommerce=%d sliders=%s commerceTypeRates=%s flexible=%s currentResearch=%s currentResearchTeamProgress=%d currentResearchCost=%d researchRate=%d researchOverflow=%d noResearchAvailable=%d researchTurns=%d",
		iGameTurn, ePlayer, kPlayer.getGold(), kPlayer.calculateGoldRate(), kPlayer.calculateTotalYield(YIELD_COMMERCE),
		getSASGameRecordCommercePercents(kPlayer).GetCString(), getSASGameRecordCommerceRates(kPlayer).GetCString(),
		getSASGameRecordCommerceFlexible(kPlayer).GetCString(), getSASGameRecordTechType(eResearch), iResearchProgress, iResearchCost,
		kPlayer.calculateResearchRate(eResearch), kPlayer.getOverflowResearch(), kPlayer.isNoResearchAvailable(),
		eResearch == NO_TECH ? -1 : kPlayer.getResearchTurnsLeft(eResearch, true));
}

static void logSASGameRecordStatistics(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	CvPlayerRecord const* pRecord = kPlayer.getPlayerRecord();
	const int iCitiesBuilt = (pRecord == NULL ? 0 : pRecord->getNumCitiesBuilt());
	const int iCitiesRazed = (pRecord == NULL ? 0 : pRecord->getNumCitiesRazed());
	// <!-- custom: Built/razed are persistent CyStatistics player-record values used by the Statistics tab.
	// Acquired/lost and all `logged*` military values are recorder-session observations, reset on load; keeping them local avoids save-format churn while exposing cumulative quality generation and combat luck alongside ordinary wins/losses. (GPT-5.5 + ChatGPT-5.6-Sol) -->
	SASGameRecordMilitaryQualityTotals const& kMilitary = g_akSASGameRecordMilitaryQualityTotals[ePlayer];
	SASGameRecordBattleQuality const& kBattleQuality = g_akSASGameRecordTotalBattleQuality[ePlayer];
	logSASGameRecord("GAME_RECORD_STATISTICS turn=%d player=%d currentCities=%d persistentCitiesBuilt=%d persistentCitiesRazed=%d loggedCitiesAcquired=%d loggedCitiesLost=%d loggedCitiesConquered=%d loggedCitiesLostByConquest=%d loggedCitiesTradedIn=%d loggedCitiesTradedOut=%d loggedCityNet=%+d loggedBattleWins=%d loggedBattleLosses=%d loggedCityBattleWins=%d loggedCityBattleLosses=%d loggedBattleNet=%+d loggedWithdrawals=%d loggedEnemyWithdrawals=%d loggedCombatLimitAttacks=%d loggedCombatLimitDefenses=%d loggedLuckEligibleBattles=%d loggedLuckEligibleWins=%d loggedExpectedWinsX1000=%d loggedLuckDeltaX1000=%+d loggedUpsetWins=%d loggedUpsetLosses=%d loggedLowestOddsWinPermille=%d loggedHighestOddsLossPermille=%d loggedXpGained=%d loggedCombatXpGained=%d loggedNonCombatXpGained=%d loggedXpPreventedByCap=%d loggedXpLostAdjustments=%d loggedPromotionsChosen=%d loggedLeaderPromotionApplications=%d loggedEnemyXpDestroyed=%d loggedOwnXpLost=%d",
		iGameTurn, ePlayer, kPlayer.getNumCities(), iCitiesBuilt, iCitiesRazed, g_aiSASGameRecordCitiesAcquired[ePlayer],
		g_aiSASGameRecordCitiesLost[ePlayer], g_aiSASGameRecordCitiesConquered[ePlayer], g_aiSASGameRecordCitiesLostByConquest[ePlayer],
		g_aiSASGameRecordCitiesTradedIn[ePlayer], g_aiSASGameRecordCitiesTradedOut[ePlayer],
		g_aiSASGameRecordCitiesAcquired[ePlayer] - g_aiSASGameRecordCitiesLost[ePlayer], g_aiSASGameRecordTotalBattleWins[ePlayer],
		g_aiSASGameRecordTotalBattleLosses[ePlayer], g_aiSASGameRecordTotalCityBattleWins[ePlayer],
		g_aiSASGameRecordTotalCityBattleLosses[ePlayer],
		g_aiSASGameRecordTotalBattleWins[ePlayer] - g_aiSASGameRecordTotalBattleLosses[ePlayer], kBattleQuality.iWithdrawals,
		kBattleQuality.iEnemyWithdrawals, kBattleQuality.iCombatLimitAttacks, kBattleQuality.iCombatLimitDefenses,
		kBattleQuality.iLuckEligibleBattles, kBattleQuality.iLuckEligibleWins, kBattleQuality.iExpectedWinsX1000,
		1000 * kBattleQuality.iLuckEligibleWins - kBattleQuality.iExpectedWinsX1000, kBattleQuality.iUpsetWins, kBattleQuality.iUpsetLosses,
		kBattleQuality.iLowestOddsWinPermille, kBattleQuality.iHighestOddsLossPermille, kMilitary.iExperienceGained,
		kMilitary.iCombatExperienceGained, kMilitary.iNonCombatExperienceGained, kMilitary.iExperiencePreventedByCap,
		kMilitary.iExperienceLostAdjustments, kMilitary.iPromotionsChosen, kMilitary.iLeaderPromotionApplications,
		kMilitary.iEnemyExperienceDestroyed, kMilitary.iOwnExperienceLost);
}

static CvString getSASGameRecordEliminatedPlayers()
{
	CvString szList;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes const eLoopPlayer = (PlayerTypes)iI;
		CvPlayer const& kLoopPlayer = GET_PLAYER(eLoopPlayer);
		if (kLoopPlayer.isEverAlive() && !kLoopPlayer.isAlive() && !kLoopPlayer.isBarbarian())
			appendSASDiagnosticIntListValue(szList, eLoopPlayer);
	}
	return getSASDiagnosticOrDash(szList);
}

static PlayerTypes getSASGameRecordTopScorePlayer()
{
	PlayerTypes eBestPlayer = NO_PLAYER;
	int iBestScore = MIN_INT;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes const eLoopPlayer = (PlayerTypes)iI;
		CvPlayer const& kLoopPlayer = GET_PLAYER(eLoopPlayer);
		if (!kLoopPlayer.isAlive() || kLoopPlayer.isBarbarian())
			continue;
		int const iScore = kLoopPlayer.calculateScore();
		if (eBestPlayer == NO_PLAYER || iScore > iBestScore)
		{
			eBestPlayer = eLoopPlayer;
			iBestScore = iScore;
		}
	}
	return eBestPlayer;
}

static PlayerTypes getSASGameRecordTopPowerPlayer()
{
	PlayerTypes eBestPlayer = NO_PLAYER;
	int iBestPower = MIN_INT;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes const eLoopPlayer = (PlayerTypes)iI;
		CvPlayer const& kLoopPlayer = GET_PLAYER(eLoopPlayer);
		if (!kLoopPlayer.isAlive() || kLoopPlayer.isBarbarian())
			continue;
		int const iPower = kLoopPlayer.getPower();
		if (eBestPlayer == NO_PLAYER || iPower > iBestPower)
		{
			eBestPlayer = eLoopPlayer;
			iBestPower = iPower;
		}
	}
	return eBestPlayer;
}

void logSASGameRecordRunStatus(char const* szReason)
{
	// <!-- custom: CvGame::getNumHumanPlayers is not const in the Civ4 SDK headers, so this local game reference cannot be const. (GPT-5.5) -->
	CvGame& kGame = GC.getGame();
	PlayerTypes const eTopScorePlayer = getSASGameRecordTopScorePlayer();
	PlayerTypes const eTopPowerPlayer = getSASGameRecordTopPowerPlayer();
	// <!-- custom: Compact run-status row gives autoplay/LLM review a single parse-friendly checkpoint for who is alive, eliminated, leading by score, and leading by power.
	// Victory already has its own action row; this row also works for ordinary stopped autoplays where no victory event fires. (GPT-5.5) -->
	logSASGameRecord("GAME_RECORD_RUN_STATUS turn=%d reason=%s elapsed=%d year=%d winnerTeam=%d victory=%s playersAlive=%d teamsAlive=%d playersEverAlive=%d humans=%d eliminatedPlayers=%s topScorePlayer=%d topScore=%d topPowerPlayer=%d topPower=%d totalCities=%d totalPopulation=%d",
		kGame.getGameTurn(), szReason == NULL ? "-" : szReason, kGame.getElapsedGameTurns(), kGame.getGameTurnYear(), kGame.getWinner(),
		kGame.getVictory() == NO_VICTORY ? "-" : GC.getInfo(kGame.getVictory()).getType(), kGame.countCivPlayersAlive(),
		kGame.countCivTeamsAlive(), kGame.countCivPlayersEverAlive(), kGame.getNumHumanPlayers(),
		getSASGameRecordEliminatedPlayers().GetCString(), eTopScorePlayer,
		eTopScorePlayer == NO_PLAYER ? 0 : GET_PLAYER(eTopScorePlayer).calculateScore(), eTopPowerPlayer,
		eTopPowerPlayer == NO_PLAYER ? 0 : GET_PLAYER(eTopPowerPlayer).getPower(), kGame.getNumCities(), kGame.getTotalPopulation());
}

static void logSASGameRecordDemographics(PlayerTypes ePlayer, int iGameTurn)
{
	CvGame const& kGame = GC.getGame();
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	SASGameRecordPlayerPrevious& kPrevious = g_akSASGameRecordPlayerPrevious[ePlayer];
	const int iScore = kPlayer.calculateScore();
	const int iPopulation = kPlayer.getTotalPopulation();
	const int iLand = kPlayer.getTotalLand();
	const int iFood = kPlayer.calculateTotalYield(YIELD_FOOD);
	const int iProduction = kPlayer.calculateTotalYield(YIELD_PRODUCTION);
	const int iCommerce = kPlayer.calculateTotalYield(YIELD_COMMERCE);
	const int iResearch = kPlayer.getCommerceRate(COMMERCE_RESEARCH);
	const int iCulture = kPlayer.getCommerceRate(COMMERCE_CULTURE);
	const int iEspionage = kPlayer.getCommerceRate(COMMERCE_ESPIONAGE);
	const int iGoldRate = kPlayer.calculateGoldRate();
	const int iPower = kPlayer.getPower();
	logSASGameRecord("GAME_RECORD_DEMOGRAPHICS turn=%d player=%d rank=%d score=%d population=%d land=%d food=%d production=%d commerce=%d research=%d culture=%d espionage=%d goldRate=%d power=%d",
			iGameTurn, ePlayer, kGame.getPlayerRank(ePlayer) + 1, iScore, iPopulation, iLand, iFood, iProduction, iCommerce, iResearch, iCulture, iEspionage, iGoldRate, iPower);
	logSASGameRecord("GAME_RECORD_DEMOGRAPHICS_DELTAS turn=%d player=%d deltaValid=%d scoreDelta=%+d populationDelta=%+d landDelta=%+d foodDelta=%+d productionDelta=%+d commerceDelta=%+d researchDelta=%+d cultureDelta=%+d espionageDelta=%+d goldRateDelta=%+d powerDelta=%+d",
		iGameTurn, ePlayer, kPrevious.bValid, getSASGameRecordDelta(kPrevious.bValid, iScore, kPrevious.iDemoScore),
		getSASGameRecordDelta(kPrevious.bValid, iPopulation, kPrevious.iDemoPopulation),
		getSASGameRecordDelta(kPrevious.bValid, iLand, kPrevious.iDemoLand),
		getSASGameRecordDelta(kPrevious.bValid, iFood, kPrevious.iDemoFood),
		getSASGameRecordDelta(kPrevious.bValid, iProduction, kPrevious.iDemoProduction),
		getSASGameRecordDelta(kPrevious.bValid, iCommerce, kPrevious.iDemoCommerce),
		getSASGameRecordDelta(kPrevious.bValid, iResearch, kPrevious.iDemoResearch),
		getSASGameRecordDelta(kPrevious.bValid, iCulture, kPrevious.iDemoCulture),
		getSASGameRecordDelta(kPrevious.bValid, iEspionage, kPrevious.iDemoEspionage),
		getSASGameRecordDelta(kPrevious.bValid, iGoldRate, kPrevious.iDemoGoldRate),
		getSASGameRecordDelta(kPrevious.bValid, iPower, kPrevious.iDemoPower));
	kPrevious.iDemoScore = iScore;
	kPrevious.iDemoPopulation = iPopulation;
	kPrevious.iDemoLand = iLand;
	kPrevious.iDemoFood = iFood;
	kPrevious.iDemoProduction = iProduction;
	kPrevious.iDemoCommerce = iCommerce;
	kPrevious.iDemoResearch = iResearch;
	kPrevious.iDemoCulture = iCulture;
	kPrevious.iDemoEspionage = iEspionage;
	kPrevious.iDemoGoldRate = iGoldRate;
	kPrevious.iDemoPower = iPower;
}

static void logSASGameRecordAttitudes(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	CvString szToward;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes eLoopPlayer = (PlayerTypes)iI;
		if (eLoopPlayer == ePlayer || !GET_PLAYER(eLoopPlayer).isAlive() || GET_PLAYER(eLoopPlayer).isBarbarian())
			continue;
		if (!GET_TEAM(kPlayer.getTeam()).isHasMet(GET_PLAYER(eLoopPlayer).getTeam()))
			continue;
		const int iValue = kPlayer.AI_getAttitudeVal(eLoopPlayer);
		CvString szItem;
		szItem.Format(szToward.empty() ? "%d:%+d" : ",%d:%+d", eLoopPlayer, iValue);
		szToward += szItem;
	}
	logSASGameRecord("GAME_RECORD_ATTITUDES turn=%d player=%d towardValues=%s", iGameTurn, ePlayer, getSASDiagnosticOrDash(szToward).GetCString());
}

static void appendSASGameRecordAttitudeComponent(CvString& szComponents, int& iComponentSum, char const* szName, int iValue)
{
	iComponentSum += iValue;
	if (iValue == 0)
		return;
	CvString szItem;
	szItem.Format(szComponents.empty() ? "%s=%+d" : ",%s=%+d", szName, iValue);
	szComponents += szItem;
}

static void logSASGameRecordDiplomaticAttitudes(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	CvTeam const& kTeam = GET_TEAM(kPlayer.getTeam());
	// <!-- custom: Level 3 already preserves individual memory counts/contributions.
	// Add the rest of the cached AI-attitude arithmetic at the same periodic boundary so totals such as "heathen religion", open borders, fair trade, shared war and close borders can be explained without logging every routine counter/decay tick.
	// Reuse the same memory scan for both rows; full additive breakdowns apply only to gameplay-active major AI players (including AI Auto Play), while the legacy memory row keeps its broader existing coverage. (GPT-5.6-Sol) -->
	bool const bLogBreakdowns = (!kPlayer.isMinorCiv() && (!kPlayer.isHuman() || kPlayer.isHumanDisabled()));
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes const eTowardPlayer = (PlayerTypes)iI;
		CvPlayerAI const& kTowardPlayer = GET_PLAYER(eTowardPlayer);
		if (eTowardPlayer == ePlayer || !kTowardPlayer.isAlive() || kTowardPlayer.isBarbarian() || !kTeam.isHasMet(kTowardPlayer.getTeam()))
			continue;

		CvString szComponents;
		int iComponentSum = 0;
		bool const bLogThisBreakdown = (bLogBreakdowns && !kTowardPlayer.isMinorCiv() && kPlayer.getTeam() != kTowardPlayer.getTeam());
		if (bLogThisBreakdown)
		{
			// <!-- custom: Mirror CvPlayerAI::AI_updateAttitude's additive order exactly; AI_getWarAttitude needs the full pre-war partial sum. (GPT-5.6-Sol) -->
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "firstImpression", kPlayer.AI_getFirstImpressionAttitude(eTowardPlayer));
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "teamSize", kPlayer.AI_getTeamSizeAttitude(eTowardPlayer));
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "rankDifference", kPlayer.AI_getRankDifferenceAttitude(eTowardPlayer));
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "closeBorders", kPlayer.AI_getCloseBordersAttitude(eTowardPlayer));
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "peace", kPlayer.AI_getPeaceAttitude(eTowardPlayer));
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "sameReligion", kPlayer.AI_getSameReligionAttitude(eTowardPlayer));
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "differentReligion", kPlayer.AI_getDifferentReligionAttitude(eTowardPlayer));
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "bonusTrade", kPlayer.AI_getBonusTradeAttitude(eTowardPlayer));
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "openBorders", kPlayer.AI_getOpenBordersAttitude(eTowardPlayer));
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "defensivePact", kPlayer.AI_getDefensivePactAttitude(eTowardPlayer));
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "rivalDefensivePact", kPlayer.AI_getRivalDefensivePactAttitude(eTowardPlayer));
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "rivalVassal", kPlayer.AI_getRivalVassalAttitude(eTowardPlayer));
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "expansionist", kPlayer.AI_getExpansionistAttitude(eTowardPlayer));
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "sharedWar", kPlayer.AI_getShareWarAttitude(eTowardPlayer));
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "favoriteCivic", kPlayer.AI_getFavoriteCivicAttitude(eTowardPlayer));
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "trade", kPlayer.AI_getTradeAttitude(eTowardPlayer));
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "rivalTrade", kPlayer.AI_getRivalTradeAttitude(eTowardPlayer));
		}

		CvString szMemories;
		int iMemoryAttitude = 0;
		for (int iJ = 0; iJ < NUM_MEMORY_TYPES; iJ++)
		{
			MemoryTypes const eMemory = (MemoryTypes)iJ;
			int const iCount = kPlayer.AI_getMemoryCount(eTowardPlayer, eMemory);
			if (iCount <= 0)
				continue;
			int const iAttitude = kPlayer.AI_getMemoryAttitude(eTowardPlayer, eMemory);
			iMemoryAttitude += iAttitude;
			CvString szItem;
			szItem.Format(szMemories.empty() ? "%s=%d/%+d" : ",%s=%d/%+d", getSASMemoryType(eMemory), iCount, iAttitude);
			szMemories += szItem;
		}
		if (bLogThisBreakdown)
		{
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "memories", iMemoryAttitude);
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "extra", kPlayer.AI_getAttitudeExtra(eTowardPlayer));
			int const iWarAttitude = kPlayer.AI_getWarAttitude(eTowardPlayer, iComponentSum);
			appendSASGameRecordAttitudeComponent(szComponents, iComponentSum, "war", iWarAttitude);
			int const iComponentValue = ::range(iComponentSum, -100, 100);
			int const iCachedRawValue = kPlayer.AI_getAttitudeVal(eTowardPlayer, false);
			int const iEffectiveValue = kPlayer.AI_getAttitudeVal(eTowardPlayer);
			// <!-- custom: componentSum is intentionally pre-clamp; componentValue applies the same -100..100 clamp as AI_updateAttitude, cachedRawValue exposes the actual cache, and effectiveValue additionally includes Civ4/AdvCiv forced vassal/master semantics.
			// Keeping all four makes stale-cache or forced-relation differences explicit rather than misattributing them to an additive component. (GPT-5.6-Sol) -->
			logSASGameRecord("GAME_RECORD_DIPLO_ATTITUDE_BREAKDOWN turn=%d player=%d toward=%d componentSum=%+d componentValue=%+d cachedRawValue=%+d effectiveValue=%+d forcedDelta=%+d components=%s",
				iGameTurn, ePlayer, eTowardPlayer, iComponentSum, iComponentValue, iCachedRawValue, iEffectiveValue, iEffectiveValue - iCachedRawValue,
				getSASDiagnosticOrDash(szComponents).GetCString());
		}
		if (!szMemories.empty())
		{
			// <!-- custom: Each item is MEMORY_TYPE=count/attitudeContribution; periodic snapshots avoid logging every routine memory decay. (GPT-5.6-Sol) -->
			logSASGameRecord("GAME_RECORD_DIPLO_MEMORIES turn=%d player=%d toward=%d attitudeValue=%+d memoryAttitude=%+d memories=%s",
				iGameTurn, ePlayer, eTowardPlayer, kPlayer.AI_getAttitudeVal(eTowardPlayer), iMemoryAttitude, szMemories.GetCString());
		}
	}
}

static void logSASGameRecordDiploStatus(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	CvTeam const& kTeam = GET_TEAM(kPlayer.getTeam());
	const TeamTypes eWorstEnemy = kTeam.AI().AI_getWorstEnemy();
	CvString szWorstEnemyPlayers;
	CvString szWorstEnemyOfTeams;
	CvString szAtWar;
	CvString szOpenBorders;
	CvString szDefensivePacts;
	CvString szForcePeace;
	CvString szCanContact;
	CvString szCanContactWilling;
	CvString szWontTalkTo;
	CvString szWontTalkFrom;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		TeamTypes eLoopTeam = (TeamTypes)iI;
		if (eLoopTeam == kPlayer.getTeam() || !GET_TEAM(eLoopTeam).isAlive() || GET_TEAM(eLoopTeam).isBarbarian())
			continue;
		if (!kTeam.isHasMet(eLoopTeam))
			continue;
		if (kTeam.isAtWar(eLoopTeam))
			appendSASDiagnosticIntListValue(szAtWar, eLoopTeam);
		if (kTeam.isOpenBorders(eLoopTeam))
			appendSASDiagnosticIntListValue(szOpenBorders, eLoopTeam);
		if (kTeam.isDefensivePact(eLoopTeam))
			appendSASDiagnosticIntListValue(szDefensivePacts, eLoopTeam);
		if (kTeam.isForcePeace(eLoopTeam))
			appendSASDiagnosticIntListValue(szForcePeace, eLoopTeam);
		if (GET_TEAM(eLoopTeam).AI().AI_getWorstEnemy() == kPlayer.getTeam())
			appendSASDiagnosticIntListValue(szWorstEnemyOfTeams, eLoopTeam);
	}
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes eLoopPlayer = (PlayerTypes)iI;
		if (eLoopPlayer == ePlayer || !GET_PLAYER(eLoopPlayer).isAlive() || GET_PLAYER(eLoopPlayer).isBarbarian())
			continue;
		if (!kTeam.isHasMet(GET_PLAYER(eLoopPlayer).getTeam()))
			continue;
		if (GET_PLAYER(eLoopPlayer).getTeam() == eWorstEnemy)
			appendSASDiagnosticIntListValue(szWorstEnemyPlayers, eLoopPlayer);
		if (kPlayer.canContact(eLoopPlayer, false))
			appendSASDiagnosticIntListValue(szCanContact, eLoopPlayer);
		if (kPlayer.canContact(eLoopPlayer, true))
			appendSASDiagnosticIntListValue(szCanContactWilling, eLoopPlayer);
		if (!kPlayer.AI_isWillingToTalk(eLoopPlayer))
			appendSASDiagnosticIntListValue(szWontTalkTo, eLoopPlayer);
		if (!GET_PLAYER(eLoopPlayer).AI_isWillingToTalk(ePlayer))
			appendSASDiagnosticIntListValue(szWontTalkFrom, eLoopPlayer);
	}
	logSASGameRecord("GAME_RECORD_DIPLO_STATUS turn=%d player=%d team=%d worstEnemyTeam=%d worstEnemyPlayers=%s worstEnemyOfTeams=%s atWar=%s openBorders=%s defensivePacts=%s forcePeace=%s canContact=%s canContactWilling=%s wontTalkTo=%s wontTalkFrom=%s",
		iGameTurn, ePlayer, kPlayer.getTeam(), eWorstEnemy, getSASDiagnosticOrDash(szWorstEnemyPlayers).GetCString(),
		getSASDiagnosticOrDash(szWorstEnemyOfTeams).GetCString(), getSASDiagnosticOrDash(szAtWar).GetCString(),
		getSASDiagnosticOrDash(szOpenBorders).GetCString(), getSASDiagnosticOrDash(szDefensivePacts).GetCString(),
		getSASDiagnosticOrDash(szForcePeace).GetCString(), getSASDiagnosticOrDash(szCanContact).GetCString(),
		getSASDiagnosticOrDash(szCanContactWilling).GetCString(), getSASDiagnosticOrDash(szWontTalkTo).GetCString(),
		getSASDiagnosticOrDash(szWontTalkFrom).GetCString());
}

// <!-- custom: Pairwise trade-market rows primarily mirror resolved information available through the active player's Foreign Advisor/diplomacy interface rather than speculative AI candidate reasoning.
// `NO_TALK` is kept separate from DenialTypes. Optional bonus GPT quotes and AI_techTradeVal fields add compact resolved valuation context under independent gates; they remain const observations and never submit an offer or alter trade state.
// All market observations use const query APIs, do not consume synchronized RNG, and do not mutate gameplay state. (ChatGPT-5.6-Sol) -->
static char const* getSASGameRecordDenialType(DenialTypes eDenial)
{
	return (eDenial == NO_DENIAL ? "NO_DENIAL" : GC.getInfo(eDenial).getType());
}

static void appendSASGameRecordMarketReason(CvString& szList, char const* szItem, char const* szReason)
{
	CvString szEntry;
	szEntry.Format(szList.empty() ? "%s=%s" : ",%s=%s", szItem, szReason);
	szList += szEntry;
}

static void appendSASGameRecordMarketValue(CvString& szList, char const* szItem, int iValue)
{
	CvString szEntry;
	szEntry.Format(szList.empty() ? "%s=%d" : ",%s=%d", szItem, iValue);
	szList += szEntry;
}

// <!-- custom: AI Auto Play temporarily changes the original human slot from isHuman() to isHumanDisabled(), so AI_goldForBonus's human-vs-AI assertion deliberately no longer applies even though a useful controller-to-controller market quote still exists.
// Reproduce AI_goldForBonus's AI-buyer conversion locally from its public valuation primitives instead of mutating human-control state or weakening the gameplay assertion.
// Ordinary human-vs-AI snapshots still call AI_goldForBonus directly and therefore retain exact Foreign Advisor hover semantics. (ChatGPT-5.6-Sol) -->
static int getSASGameRecordBonusGPTQuote(CvPlayerAI const& kBuyer, BonusTypes eBonus, PlayerTypes eSeller)
{
	CvPlayerAI const& kSeller = GET_PLAYER(eSeller);
	if (kBuyer.isHuman() != kSeller.isHuman())
		return kBuyer.AI_goldForBonus(eBonus, eSeller);
	if (kBuyer.isHuman())
		return 0; // <!-- custom: No AI quote is defined for human-vs-human trade. (ChatGPT-5.6-Sol) -->

	int const iTradeVal = kBuyer.AI_bonusTradeVal(eBonus, eSeller, 1);
	int const iMaxGPT = kBuyer.AI_maxGoldPerTurnTrade(eSeller);
	int iGPT = 0;
	while (iGPT < iMaxGPT && kBuyer.AI_goldPerTurnTradeVal(iGPT + 1) <= iTradeVal)
		iGPT++;
	return iGPT;
}

static char const* getSASGameRecordBonusUnavailableReason(CvPlayerAI const& kFrom, CvPlayerAI const& kTo, BonusTypes eBonus)
{
	CvTeam const& kFromTeam = GET_TEAM(kFrom.getTeam());
	CvTeam const& kToTeam = GET_TEAM(kTo.getTeam());
	if (kToTeam.isBonusObsolete(eBonus))
		return "RECIPIENT_OBSOLETE";
	if (kFromTeam.isBonusObsolete(eBonus))
		return "DONOR_OBSOLETE";
	if (kFrom.getNumTradeableBonuses(eBonus) <= 0)
		return "NO_TRADEABLE_COPY";
	return "OTHER";
}

static char const* getSASGameRecordTechUnavailableReason(CvPlayerAI const& kFrom, CvPlayerAI const& kTo, TechTypes eTech)
{
	if (!GC.getInfo(eTech).isTrade())
		return "XML_NOT_TRADEABLE";
	if (GET_TEAM(kFrom.getTeam()).isNoTradeTech(eTech))
		return "NO_TRADE_TECH";
	if (!kTo.canResearch(eTech, true))
		return "RECIPIENT_CANNOT_RESEARCH_VIA_TRADE";
	return "OTHER";
}

static void appendSASGameRecordMarketIntReason(CvString& szList, int iItem, char const* szReason)
{
	CvString szEntry;
	szEntry.Format(szList.empty() ? "%d=%s" : ",%d=%s", iItem, szReason);
	szList += szEntry;
}

static void appendSASGameRecordStrategicTradeStatus(CvString& szStatus, CvPlayerAI const& kFrom, PlayerTypes eTo, TradeableItems eItem)
{
	TradeData kTrade(eItem, 0);
	char const* szResult = "UNAVAILABLE";
	if (kFrom.canTradeItem(eTo, kTrade, false))
		szResult = getSASGameRecordDenialType(kFrom.getTradeDenial(eTo, kTrade));
	appendSASGameRecordMarketReason(szStatus, getSASTradeItemType(eItem), szResult);
}

// <!-- custom: Mirror the strategically important Foreign Advisor/trade-screen state that is derived only when the UI asks for it rather than stored as authoritative persistent state.
// Keep this active-viewer-relative and periodic: exact transition hooks do not exist for these derived willingness queries, and evaluating every possible viewer would turn the record into an unnecessary player^3 trade matrix.
// City ids map back to GAME_RECORD_CITY. Third-party war targets use team ids and mirror the Glance tab's "will declare war for trade" test, with exact denial reasons retained for the same evaluated targets. (ChatGPT-5.6-Sol) -->
static void logSASGameRecordStrategicTradeMarketPair(int iGameTurn, CvPlayerAI const& kViewer, CvPlayerAI const& kOther, bool bOtherWillingToTalk)
{
	PlayerTypes const eViewer = kViewer.getID();
	PlayerTypes const eOther = kOther.getID();
	CvString szStrategicStatus;
	appendSASGameRecordStrategicTradeStatus(szStrategicStatus, kOther, eViewer, TRADE_MAPS);
	appendSASGameRecordStrategicTradeStatus(szStrategicStatus, kOther, eViewer, TRADE_VASSAL);
	appendSASGameRecordStrategicTradeStatus(szStrategicStatus, kOther, eViewer, TRADE_SURRENDER);
	appendSASGameRecordStrategicTradeStatus(szStrategicStatus, kOther, eViewer, TRADE_OPEN_BORDERS);
	appendSASGameRecordStrategicTradeStatus(szStrategicStatus, kOther, eViewer, TRADE_DEFENSIVE_PACT);
	appendSASGameRecordStrategicTradeStatus(szStrategicStatus, kOther, eViewer, TRADE_PERMANENT_ALLIANCE);
	appendSASGameRecordStrategicTradeStatus(szStrategicStatus, kOther, eViewer, TRADE_DISENGAGE);

	CvString szCitiesWillCede;
	CvString szCityDenials;
	if (kOther.canPossiblyTradeItem(eViewer, TRADE_CITIES))
	{
		TradeData kCityTrade(TRADE_CITIES, 0);
		FOR_EACH_CITY(pCity, kOther)
		{
			kCityTrade.m_iData = pCity->getID();
			if (!kOther.canTradeItem(eViewer, kCityTrade, false))
				continue;
			DenialTypes const eDenial = kOther.getTradeDenial(eViewer, kCityTrade);
			if (eDenial == NO_DENIAL)
				appendSASDiagnosticIntListValue(szCitiesWillCede, pCity->getID());
			else appendSASGameRecordMarketIntReason(szCityDenials, pCity->getID(), getSASGameRecordDenialType(eDenial));
		}
	}

	CvString szGlanceWarTargets;
	CvString szGlanceWarTargetDenials;
	CvTeamAI const& kOtherTeam = GET_TEAM(kOther.getTeam());
	// <!-- custom: Match AttitudeUtil.getAttitudeText / CvGameTextMgr::parseWarTradesHelp rather than broadening this into every theoretical third-party trade combination.
	// The Glance fist is an AI-team willingness query relative to the active viewer; it intentionally does not require the pair to be willing to talk. (ChatGPT-5.6-Sol) -->
	if (kOther.getTeam() != kViewer.getTeam() && !kOtherTeam.isHuman())
	{
		for (TeamIter<MAJOR_CIV,NOT_SAME_TEAM_AS> itTarget(kOther.getTeam()); itTarget.hasNext(); ++itTarget)
		{
			TeamTypes const eTarget = itTarget->getID();
			if (eTarget == kViewer.getTeam() || kOtherTeam.isAtWar(eTarget))
				continue;
			DenialTypes const eDenial = kOtherTeam.AI_declareWarTrade(eTarget, kViewer.getTeam());
			if (eDenial == NO_DENIAL)
				appendSASDiagnosticIntListValue(szGlanceWarTargets, eTarget);
			else appendSASGameRecordMarketIntReason(szGlanceWarTargetDenials, eTarget, getSASGameRecordDenialType(eDenial));
		}
	}

	logSASGameRecord("GAME_RECORD_TRADE_STRATEGIC turn=%d viewer=%d other=%d viewerTeam=%d otherTeam=%d otherWillingToTalk=%d bilateral=%s citiesWillCede=%s cityDenials=%s glanceWarTargets=%s glanceWarTargetDenials=%s",
		iGameTurn, eViewer, eOther, kViewer.getTeam(), kOther.getTeam(), bOtherWillingToTalk,
		getSASDiagnosticOrDash(szStrategicStatus).GetCString(), getSASDiagnosticOrDash(szCitiesWillCede).GetCString(),
		getSASDiagnosticOrDash(szCityDenials).GetCString(), getSASDiagnosticOrDash(szGlanceWarTargets).GetCString(),
		getSASDiagnosticOrDash(szGlanceWarTargetDenials).GetCString());
}

static void logSASGameRecordTradeMarket(int iGameTurn)
{
	if (!isSASGameRecordTradeMarketEnabled())
		return;
	CvGame const& kGame = GC.getGame();
	PlayerTypes const eViewer = kGame.getActivePlayer();
	if (eViewer == NO_PLAYER)
		return;
	CvPlayerAI const& kViewer = GET_PLAYER(eViewer);
	if (!kViewer.isAlive() || kViewer.isBarbarian() || kViewer.isMinorCiv())
		return;
	CvTeam const& kViewerTeam = GET_TEAM(kViewer.getTeam());
	bool const bBonusGPTQuotesEnabled = isSASGameRecordTradeMarketBonusGPTQuotesEnabled();
	bool const bAITechValuesEnabled = isSASGameRecordTradeMarketAITechValuesEnabled();

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes const eOther = (PlayerTypes)iI;
		if (eOther == eViewer)
			continue;
		CvPlayerAI const& kOther = GET_PLAYER(eOther);
		if (!kOther.isAlive() || kOther.isBarbarian() || kOther.isMinorCiv())
			continue;
		CvTeam const& kOtherTeam = GET_TEAM(kOther.getTeam());
		if (!kViewerTeam.isHasMet(kOther.getTeam()))
			continue;

		bool const bOtherWillingToTalk = kOther.AI_isWillingToTalk(eViewer);
		bool const bTradeNetwork = kViewer.canTradeNetworkWith(eOther);
		bool const bGoldTrading = (kViewerTeam.isGoldTrading() || kOtherTeam.isGoldTrading());
		bool const bTechVisible = kViewer.canSeeTech(eOther);
		bool const bTechTradingPossible = kOther.canPossiblyTradeItem(eViewer, TRADE_TECHNOLOGIES);
		int const iOtherMaxGold = (bGoldTrading && bOtherWillingToTalk ? kOther.AI_maxGoldTrade(eViewer) : -1);
		int const iOtherMaxGPT = (bGoldTrading && bOtherWillingToTalk ? kOther.AI_maxGoldPerTurnTrade(eViewer) : -1);
		int const iViewerGPTBalance = kViewer.getGoldPerTurnByPlayer(eOther);
		char const* szBonusGPTQuoteMode = "DISABLED";
		if (bBonusGPTQuotesEnabled)
		{
			if (kViewer.isHuman() && !kOther.isHuman()) szBonusGPTQuoteMode = "FOREIGN_ADVISOR";
			else if (!kViewer.isHuman() && !kOther.isHuman()) szBonusGPTQuoteMode = (kViewer.isHumanDisabled() ? "AUTOPLAY_AI" : "AI_MARKET");
			else if (kViewer.isHuman() != kOther.isHuman()) szBonusGPTQuoteMode = "MIXED_AI_HUMAN";
			else szBonusGPTQuoteMode = "UNAVAILABLE";
		}

		CvString szViewerCanOfferBonuses;
		CvString szOtherBonusesForTrade;
		CvString szOtherBonusDenials;
		CvString szOtherBonusUnavailable;
		CvString szOtherBonusAskGPT;
		CvString szOtherBonusWillPayGPT;
		// <!-- custom: Resources cannot trade at all without a network, so keep that pair-wide fact in tradeNetwork instead of repeating NO_TRADE_NETWORK after every owned bonus. (ChatGPT-5.6-Sol) -->
		if (bTradeNetwork)
		{
			TradeData kBonusTrade(TRADE_RESOURCES, 0);
			TradeData const kOneGPTTrade(TRADE_GOLD_PER_TURN, 1);
			FOR_EACH_ENUM(Bonus)
			{
				int const iViewerAvailable = kViewer.getNumAvailableBonuses(eLoopBonus);
				int const iOtherAvailable = kOther.getNumAvailableBonuses(eLoopBonus);
				if (iViewerAvailable <= 0 && iOtherAvailable <= 0)
					continue;
				kBonusTrade.m_iData = eLoopBonus;
				if (iViewerAvailable > 0 && kViewer.canTradeItem(eOther, kBonusTrade, true))
				{
					appendSASGameRecordType(szViewerCanOfferBonuses, getSASGameRecordBonusType(eLoopBonus));
					// <!-- custom: Keep quote reconstruction independently optional because bonus trade valuation can perform substantial bonus/building work.
					// getSASGameRecordBonusGPTQuote preserves exact Foreign Advisor semantics in ordinary human play and switches to the current AI controller's equivalent quote during AI Auto Play. (ChatGPT-5.6-Sol) -->
					if (bBonusGPTQuotesEnabled && bOtherWillingToTalk && kOther.canTradeItem(eViewer, kOneGPTTrade, false))
					{
						int const iWillPayGPT = getSASGameRecordBonusGPTQuote(kOther, eLoopBonus, eViewer);
						if (iWillPayGPT > 0)
							appendSASGameRecordMarketValue(szOtherBonusWillPayGPT, getSASGameRecordBonusType(eLoopBonus), iWillPayGPT);
					}
				}

				if (iOtherAvailable <= 0)
					continue;
				if (!kOther.canTradeItem(eViewer, kBonusTrade, false))
				{
					appendSASGameRecordMarketReason(szOtherBonusUnavailable, getSASGameRecordBonusType(eLoopBonus), getSASGameRecordBonusUnavailableReason(kOther, kViewer, eLoopBonus));
					continue;
				}
				DenialTypes const eDenial = kOther.getTradeDenial(eViewer, kBonusTrade);
				if (eDenial != NO_DENIAL)
				{
					appendSASGameRecordMarketReason(szOtherBonusDenials, getSASGameRecordBonusType(eLoopBonus), getSASGameRecordDenialType(eDenial));
					continue;
				}
				if (!bOtherWillingToTalk)
				{
					appendSASGameRecordMarketReason(szOtherBonusDenials, getSASGameRecordBonusType(eLoopBonus), "NO_TALK");
					continue;
				}
				appendSASGameRecordType(szOtherBonusesForTrade, getSASGameRecordBonusType(eLoopBonus));
				if (bBonusGPTQuotesEnabled && kViewer.canTradeItem(eOther, kOneGPTTrade, false))
				{
					int const iAskGPT = getSASGameRecordBonusGPTQuote(kViewer, eLoopBonus, eOther);
					if (iAskGPT > 0)
						appendSASGameRecordMarketValue(szOtherBonusAskGPT, getSASGameRecordBonusType(eLoopBonus), iAskGPT);
				}
			}
		}

		CvString szViewerTechsForTrade;
		CvString szOtherTechsForTrade;
		CvString szOtherTechDenials;
		CvString szOtherTechUnavailable;
		CvString szViewerTechReceiveValues;
		CvString szOtherTechReceiveValues;
		// <!-- custom: canPossiblyTradeItem is pair-wide for technology trading; when false, techTradingPossible explains the whole tab and avoids per-tech work. (ChatGPT-5.6-Sol) -->
		if (bTechVisible && bTechTradingPossible)
		{
			TradeData kTechTrade(TRADE_TECHNOLOGIES, 0);
			FOR_EACH_ENUM(Tech)
			{
				bool const bViewerHasTech = kViewerTeam.isHasTech(eLoopTech);
				bool const bOtherHasTech = kOtherTeam.isHasTech(eLoopTech);
				if (bViewerHasTech == bOtherHasTech)
					continue;
				kTechTrade.m_iData = eLoopTech;
				if (bViewerHasTech)
				{
					if (kViewer.canTradeItem(eOther, kTechTrade, false))
					{
						if (bAITechValuesEnabled)
						{
							int const iReceiveValue = GET_TEAM(kOther.getTeam()).AI_techTradeVal( eLoopTech, kViewer.getTeam());
							appendSASGameRecordMarketValue(szOtherTechReceiveValues, getSASGameRecordTechType(eLoopTech), iReceiveValue);
						}
						if (kViewer.getTradeDenial(eOther, kTechTrade) == NO_DENIAL)
							appendSASGameRecordType(szViewerTechsForTrade, getSASGameRecordTechType(eLoopTech));
					}
					continue;
				}

				if (kOther.canTradeItem(eViewer, kTechTrade, false))
				{
					if (bAITechValuesEnabled)
					{
						int const iReceiveValue = GET_TEAM(kViewer.getTeam()).AI_techTradeVal( eLoopTech, kOther.getTeam());
						appendSASGameRecordMarketValue(szViewerTechReceiveValues, getSASGameRecordTechType(eLoopTech), iReceiveValue);
					}
					DenialTypes const eDenial = kOther.getTradeDenial(eViewer, kTechTrade);
					if (eDenial != NO_DENIAL)
						appendSASGameRecordMarketReason(szOtherTechDenials, getSASGameRecordTechType(eLoopTech), getSASGameRecordDenialType(eDenial));
					else if (!bOtherWillingToTalk)
						appendSASGameRecordMarketReason(szOtherTechDenials, getSASGameRecordTechType(eLoopTech), "NO_TALK");
					else appendSASGameRecordType(szOtherTechsForTrade, getSASGameRecordTechType(eLoopTech));
				}
				else if (kViewer.canResearch(eLoopTech, false))
				{
					appendSASGameRecordMarketReason(szOtherTechUnavailable, getSASGameRecordTechType(eLoopTech), getSASGameRecordTechUnavailableReason(kOther, kViewer, eLoopTech));
				}
			}
		}

		logSASGameRecordStrategicTradeMarketPair(iGameTurn, kViewer, kOther, bOtherWillingToTalk);

		// <!-- custom: `viewerGPTBalanceWithOther` uses CvPlayer's signed pair balance: positive means the viewer currently receives GPT from this player, negative means the viewer pays them.
		// Accepted/ended deal rows remain the canonical item lifecycles; this snapshot supplies the current Foreign-Advisor-style market without duplicating every active deal. (ChatGPT-5.6-Sol) -->
		logSASGameRecord("GAME_RECORD_TRADE_MARKET turn=%d viewer=%d other=%d viewerTeam=%d otherTeam=%d otherWillingToTalk=%d tradeNetwork=%d goldTrading=%d otherMaxGold=%d otherMaxGPT=%d viewerGPTBalanceWithOther=%+d techVisible=%d techTradingPossible=%d bonusGPTQuotesEnabled=%d bonusGPTQuoteMode=%s aiTechValuesEnabled=%d viewerCanOfferBonuses=%s otherBonusesForTrade=%s otherBonusDenials=%s otherBonusUnavailable=%s otherBonusAskGPT=%s otherBonusWillPayGPT=%s viewerTechsForTrade=%s otherTechsForTrade=%s otherTechDenials=%s otherTechUnavailable=%s viewerTechReceiveValues=%s otherTechReceiveValues=%s",
			iGameTurn, eViewer, eOther, kViewer.getTeam(), kOther.getTeam(), bOtherWillingToTalk, bTradeNetwork, bGoldTrading, iOtherMaxGold,
			iOtherMaxGPT, iViewerGPTBalance, bTechVisible, bTechTradingPossible, bBonusGPTQuotesEnabled, szBonusGPTQuoteMode,
			bAITechValuesEnabled, getSASDiagnosticOrDash(szViewerCanOfferBonuses).GetCString(),
			getSASDiagnosticOrDash(szOtherBonusesForTrade).GetCString(), getSASDiagnosticOrDash(szOtherBonusDenials).GetCString(),
			getSASDiagnosticOrDash(szOtherBonusUnavailable).GetCString(), getSASDiagnosticOrDash(szOtherBonusAskGPT).GetCString(),
			getSASDiagnosticOrDash(szOtherBonusWillPayGPT).GetCString(), getSASDiagnosticOrDash(szViewerTechsForTrade).GetCString(),
			getSASDiagnosticOrDash(szOtherTechsForTrade).GetCString(), getSASDiagnosticOrDash(szOtherTechDenials).GetCString(),
			getSASDiagnosticOrDash(szOtherTechUnavailable).GetCString(), getSASDiagnosticOrDash(szViewerTechReceiveValues).GetCString(),
			getSASDiagnosticOrDash(szOtherTechReceiveValues).GetCString());
	}
}

static void logSASGameRecordEnvironment(int iGameTurn)
{
	CvMap const& kMap = GC.getMap();
	std::vector<int> aiFeatures(GC.getNumFeatureInfos(), 0);
	std::vector<int> aiNegativeHealthFeatures(GC.getNumFeatureInfos(), 0);
	int iOwnedLand = 0;
	int iUnownedLand = 0;
	for (int iI = 0; iI < kMap.numPlots(); iI++)
	{
		CvPlot const& kPlot = kMap.getPlotByIndex(iI);
		if (!kPlot.isWater())
		{
			if (kPlot.isOwned())
				iOwnedLand++;
			else iUnownedLand++;
		}
		FeatureTypes eFeature = kPlot.getFeatureType();
		if (eFeature != NO_FEATURE)
		{
			aiFeatures[eFeature]++;
			if (GC.getInfo(eFeature).getHealthPercent() < 0)
				aiNegativeHealthFeatures[eFeature]++;
		}
	}
	CvString szFeatures;
	CvString szNegativeHealthFeatures;
	FOR_EACH_ENUM(Feature)
	{
		appendSASGameRecordTypeCount(szFeatures, getSASGameRecordFeatureType(eLoopFeature), aiFeatures[eLoopFeature]);
		appendSASGameRecordTypeCount(szNegativeHealthFeatures, getSASGameRecordFeatureType(eLoopFeature), aiNegativeHealthFeatures[eLoopFeature]);
	}
	const int iGlobalWarmingIndex = GC.getGame().getGlobalWarmingIndex();
	const int iGlobalWarmingChances = GC.getGame().getGlobalWarmingChances();
	logSASGameRecord("GAME_RECORD_ENVIRONMENT turn=%d globalWarmingIndex=%d globalWarmingChances=%d land=%d water=%d ownedLand=%d unownedLand=%d negativeHealthFeatures=%s features=%s",
		iGameTurn, iGlobalWarmingIndex, iGlobalWarmingChances, kMap.getLandPlots(), kMap.getWaterPlots(), iOwnedLand, iUnownedLand,
		getSASDiagnosticOrDash(szNegativeHealthFeatures).GetCString(), getSASDiagnosticOrDash(szFeatures).GetCString());
	logSASGameRecord("GAME_RECORD_ENVIRONMENT_DELTAS turn=%d deltaValid=%d globalWarmingIndexDelta=%+d globalWarmingChancesDelta=%+d ownedLandDelta=%+d unownedLandDelta=%+d",
			iGameTurn, g_kSASGameRecordGlobalPrevious.bValid,
			getSASGameRecordDelta(g_kSASGameRecordGlobalPrevious.bValid, iGlobalWarmingIndex, g_kSASGameRecordGlobalPrevious.iGlobalWarmingIndex),
			getSASGameRecordDelta(g_kSASGameRecordGlobalPrevious.bValid, iGlobalWarmingChances, g_kSASGameRecordGlobalPrevious.iGlobalWarmingChances),
			getSASGameRecordDelta(g_kSASGameRecordGlobalPrevious.bValid, iOwnedLand, g_kSASGameRecordGlobalPrevious.iOwnedLand),
			getSASGameRecordDelta(g_kSASGameRecordGlobalPrevious.bValid, iUnownedLand, g_kSASGameRecordGlobalPrevious.iUnownedLand));
	g_kSASGameRecordGlobalPrevious.bValid = true;
	g_kSASGameRecordGlobalPrevious.iGlobalWarmingIndex = iGlobalWarmingIndex;
	g_kSASGameRecordGlobalPrevious.iGlobalWarmingChances = iGlobalWarmingChances;
	g_kSASGameRecordGlobalPrevious.iOwnedLand = iOwnedLand;
	g_kSASGameRecordGlobalPrevious.iUnownedLand = iUnownedLand;
}

static void logSASGameRecordVoteSources(int iGameTurn)
{
	CvGame const& kGame = GC.getGame();
	FOR_EACH_ENUM(VoteSource)
	{
		CvCity const* pSourceCity = kGame.getVoteSourceCity(eLoopVoteSource, NO_TEAM, true);
		ReligionTypes eReligion = kGame.getVoteSourceReligion(eLoopVoteSource);
		TeamTypes eSecretary = kGame.getSecretaryGeneral(eLoopVoteSource);
		CvString szVotingTeams;
		CvString szFullTeams;
		CvString szVotes;
		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			TeamTypes eLoopTeam = (TeamTypes)iI;
			CvTeam const& kLoopTeam = GET_TEAM(eLoopTeam);
			if (!kLoopTeam.isAlive() || kLoopTeam.isBarbarian())
				continue;
			if (kLoopTeam.isVotingMember(eLoopVoteSource))
				appendSASDiagnosticIntListValue(szVotingTeams, eLoopTeam);
			if (kLoopTeam.isFullMember(eLoopVoteSource))
				appendSASDiagnosticIntListValue(szFullTeams, eLoopTeam);
			const int iVotes = kLoopTeam.getVotes(NO_VOTE, eLoopVoteSource);
			if (iVotes > 0)
			{
				CvString szItem;
				szItem.Format(szVotes.empty() ? "%d:%d" : ",%d:%d", eLoopTeam, iVotes);
				szVotes += szItem;
			}
		}
		CvString szVictoryVotes;
		FOR_EACH_ENUM(Vote)
		{
			if (!GC.getInfo(eLoopVote).isVoteSourceType(eLoopVoteSource) || !GC.getInfo(eLoopVote).isVictory())
				continue;
			CvString szItem;
			szItem.Format(szVictoryVotes.empty() ? "%s:required=%d,possible=%d" : ",%s:required=%d,possible=%d", getSASGameRecordVoteType(eLoopVote), kGame.getVoteRequired(eLoopVote, eLoopVoteSource), kGame.countPossibleVote(eLoopVote, eLoopVoteSource));
			szVictoryVotes += szItem;
		}
		if (pSourceCity == NULL && eReligion == NO_RELIGION && eSecretary == NO_TEAM && szVotingTeams.empty() && szVictoryVotes.empty())
			continue;
		logSASGameRecord("GAME_RECORD_DIPLO_VOTE_SOURCE turn=%d source=%s secretaryTeam=%d secretaryTimer=%d voteTimer=%d religion=%s sourceOwner=%d sourceCityId=%d sourceCity=%S sourceX=%d sourceY=%d votingTeams=%s fullTeams=%s votes=%s victoryVotes=%s",
			iGameTurn, getSASGameRecordVoteSourceType(eLoopVoteSource), eSecretary, kGame.getSecretaryGeneralTimer(eLoopVoteSource),
			kGame.getVoteTimer(eLoopVoteSource), getSASGameRecordReligionType(eReligion), pSourceCity == NULL ? -1 : pSourceCity->getOwner(),
			pSourceCity == NULL ? -1 : pSourceCity->getID(), getSASGameRecordQuotedCityName(pSourceCity).GetCString(),
			pSourceCity == NULL ? -1 : pSourceCity->getX(), pSourceCity == NULL ? -1 : pSourceCity->getY(),
			getSASDiagnosticOrDash(szVotingTeams).GetCString(), getSASDiagnosticOrDash(szFullTeams).GetCString(),
			getSASDiagnosticOrDash(szVotes).GetCString(), getSASDiagnosticOrDash(szVictoryVotes).GetCString());
	}
}

static void logSASGameRecordTeamProjects(TeamTypes eTeam, int iGameTurn)
{
	CvString szProjects;
	FOR_EACH_ENUM(Project)
		appendSASGameRecordTypeCount(szProjects, getSASGameRecordProjectType(eLoopProject), GET_TEAM(eTeam).getProjectCount(eLoopProject));
	if (!szProjects.empty())
		logSASGameRecord("GAME_RECORD_TEAM_PROJECTS turn=%d team=%d projects=%s", iGameTurn, eTeam, szProjects.GetCString());
}

static void logSASGameRecordPlayerBonuses(PlayerTypes ePlayer, int iGameTurn, SASGameRecordPlayerPrevious const& kPrevious)
{
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	CvString szAvailable;
	CvString szTradeable;
	CvString szImports;
	CvString szExports;
	int iBonusTypes = 0;
	int iBonusInstances = 0;
	int iBonusImports = 0;
	int iBonusExports = 0;
	FOR_EACH_ENUM(Bonus)
	{
		const int iAvailable = kPlayer.getNumAvailableBonuses(eLoopBonus);
		const int iTradeable = kPlayer.getNumTradeableBonuses(eLoopBonus);
		const int iImport = kPlayer.getBonusImport(eLoopBonus);
		const int iExport = kPlayer.getBonusExport(eLoopBonus);
		if (iAvailable > 0)
		{
			iBonusTypes++;
			iBonusInstances += iAvailable;
			appendSASGameRecordTypeCount(szAvailable, getSASGameRecordBonusType(eLoopBonus), iAvailable);
		}
		appendSASGameRecordTypeCount(szTradeable, getSASGameRecordBonusType(eLoopBonus), iTradeable);
		if (iImport > 0)
		{
			iBonusImports += iImport;
			appendSASGameRecordTypeCount(szImports, getSASGameRecordBonusType(eLoopBonus), iImport);
		}
		if (iExport > 0)
		{
			iBonusExports += iExport;
			appendSASGameRecordTypeCount(szExports, getSASGameRecordBonusType(eLoopBonus), iExport);
		}
	}
	logSASGameRecord("GAME_RECORD_BONUSES turn=%d player=%d deltaValid=%d bonusTypes=%d bonusTypesDelta=%+d bonusInstances=%d bonusInstancesDelta=%+d imports=%d importsDelta=%+d exports=%d exportsDelta=%+d",
		iGameTurn, ePlayer, kPrevious.bValid, iBonusTypes, getSASGameRecordDelta(kPrevious.bValid, iBonusTypes, kPrevious.iBonusTypes),
		iBonusInstances, getSASGameRecordDelta(kPrevious.bValid, iBonusInstances, kPrevious.iBonusInstances), iBonusImports,
		getSASGameRecordDelta(kPrevious.bValid, iBonusImports, kPrevious.iBonusImports), iBonusExports,
		getSASGameRecordDelta(kPrevious.bValid, iBonusExports, kPrevious.iBonusExports));
	logSASGameRecord("GAME_RECORD_BONUSES_AVAILABLE turn=%d player=%d available=%s", iGameTurn, ePlayer, getSASDiagnosticOrDash(szAvailable).GetCString());
	logSASGameRecord("GAME_RECORD_BONUSES_TRADEABLE turn=%d player=%d tradeable=%s", iGameTurn, ePlayer, getSASDiagnosticOrDash(szTradeable).GetCString());
	logSASGameRecord("GAME_RECORD_BONUSES_IMPORT_EXPORT turn=%d player=%d imported=%s exported=%s",
		iGameTurn, ePlayer, getSASDiagnosticOrDash(szImports).GetCString(), getSASDiagnosticOrDash(szExports).GetCString());
}

static bool isSASGameRecordMilitaryUnit(CvUnit const& kUnit)
{
	// <!-- custom: A failed NO_UNIT creation left an unplaced/reset object in the owner container, and the end-turn snapshot crashed while reading its combat state.
	// Unplaced units are not part of military posture; short-circuit before unit-info-backed checks. See KI#524.6. (GPT-5.6-Sol) -->
	CvPlot const* pPlot = kUnit.plot();
	return pPlot != NULL && (kUnit.canDefend(pPlot) || kUnit.baseCombatStr() > 0 || kUnit.airBaseCombatStr() > 0);
}

static bool isSASGameRecordWorkerUnit(CvUnit const& kUnit)
{
	UnitAITypes eUnitAI = kUnit.AI_getUnitAIType();
	return eUnitAI == UNITAI_WORKER || eUnitAI == UNITAI_WORKER_SEA || kUnit.workRate(true) > 0;
}

static bool isSASGameRecordSettlerUnit(CvUnit const& kUnit)
{
	return kUnit.AI_getUnitAIType() == UNITAI_SETTLE || kUnit.isFound();
}

static MissionTypes getSASGameRecordUnitMissionType(CvUnit const& kUnit)
{
	CvSelectionGroup const* pGroup = kUnit.getGroup();
	return pGroup == NULL ? NO_MISSION : pGroup->getMissionType(0);
}

static int getSASGameRecordBuildTurnsLeft(CvUnit const& kUnit, BuildTypes eBuild)
{
	return eBuild == NO_BUILD ? -1 : kUnit.getPlot().getBuildTurnsLeft(eBuild, kUnit.getOwner(), 0, 0);
}

static bool isSASGameRecordUnitGuarded(CvUnit const& kUnit)
{
	CvPlot const* pPlot = kUnit.plot();
	return pPlot != NULL && pPlot->getNumDefenders(kUnit.getOwner()) > 0;
}

static bool isSASGameRecordUnitThreatened(CvUnit const& kUnit)
{
	CvPlot const* pPlot = kUnit.plot();
	return pPlot != NULL && pPlot->isVisibleEnemyUnit(kUnit.getOwner());
}

// <!-- custom: A raw UNITAI_ASSAULT_SEA count does not show whether an empire can actually project troops across ordinary ocean.
// Test only the unit's persistent terrain rule plus the owning team's current passable tech; deliberately ignore the own-cultural-border exception so "open ocean" means lift that can leave local coastal waters.
// No pathfinding or target search is performed. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
static bool isSASGameRecordOpenOceanSeaUnit(CvUnit const& kUnit)
{
	if (kUnit.getDomainType() != DOMAIN_SEA || kUnit.getUnitType() == NO_UNIT)
		return false;
	if (kUnit.canMoveAllTerrain())
		return true;
	static TerrainTypes const eOcean = (TerrainTypes)GC.getInfoTypeForString("TERRAIN_OCEAN");
	FAssert(eOcean != NO_TERRAIN);
	if (eOcean == NO_TERRAIN)
		return false;
	if (!kUnit.getTerrainImpassable(eOcean))
		return true;
	TechTypes const ePassableTech = GC.getInfo(kUnit.getUnitType()).getTerrainPassableTech(eOcean);
	return ePassableTech != NO_TECH && GET_TEAM(kUnit.getTeam()).isHasTech(ePassableTech);
}

static void logSASGameRecordUnitPosture(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	TeamTypes eTeam = kPlayer.getTeam();
	SASGameRecordPlayerPrevious& kPrevious = g_akSASGameRecordPlayerPrevious[ePlayer];
	// <!-- custom: Promotion detail and exact assault-fleet mission destinations are level 3 only. Cache the immutable gates once instead of querying them for every unit/group. (ChatGPT-5.6-Sol) -->
	bool const bLogPromotionDetails = (gGameRecordLogLevel >= 3);
	bool const bLogNavalMissionTargets = (gGameRecordLogLevel >= 3);
	int iTotal = 0;
	int iMilitary = 0;
	int iLandMilitary = 0;
	int iSeaMilitary = 0;
	int iAirMilitary = 0;
	int iAttackAir = 0;
	int iDefenseAir = 0;
	int iCarrierAir = 0;
	int iMissileAir = 0;
	int iICBM = 0;
	int iCarrierSea = 0;
	int iMissileCarrierSea = 0;
	int iAirCargo = 0;
	int iCarrierAirCargo = 0;
	int iMissileCargo = 0;
	int iNukes = 0;
	int iBlockadingUnits = 0;
	int iUnitCombatTotal = 0;
	int iWorkers = 0;
	int iSettlers = 0;
	int iRecon = 0;
	int iCityDefenders = 0;
	int iFieldArmy = 0;
	int iOwnTerritory = 0;
	int iEnemyTerritory = 0;
	int iNeutralTerritory = 0;
	int iUnitsInCities = 0;
	int iEnemyUnitsInTerritory = 0;
	int iTotalExperience = 0;
	int iMaxExperience = 0;
	int iPromotionReady = 0;
	int iLevel2Plus = 0;
	int iLevel4Plus = 0;
	int iLevel6Plus = 0;
	// <!-- custom: Military-only quality complements all-unit totals. Keep health in percentX100 (10000 = full health) so averages remain precise without floating-point logging.
	// Promotion-instance scans remain level 3 only. (ChatGPT-5.6-Sol) -->
	int iMilitaryExperience = 0;
	int iMaxMilitaryExperience = 0;
	int iMilitaryLevelTotal = 0;
	int iMaxMilitaryLevel = 0;
	int iMilitaryPromotionReady = 0;
	int iGreatGeneralLedMilitary = 0;
	int iMilitaryXmlProductionCost = 0;
	int iMilitaryProductionNeeded = 0;
	int iMilitaryCostedUnits = 0;
	int iWoundedMilitary = 0;
	int iMilitaryHealthMeasured = 0;
	int iMilitaryHealthX100Total = 0;
	int iMinMilitaryHealthX100 = -1;
	int iMaxMilitaryHealthX100 = -1;
	int iMilitaryHealthFull = 0;
	int iMilitaryHealthHigh = 0;
	int iMilitaryHealthMedium = 0;
	int iMilitaryHealthLow = 0;
	int iPromotionInstances = (bLogPromotionDetails ? 0 : -1);
	int iMilitaryPromotionInstances = (bLogPromotionDetails ? 0 : -1);
	std::vector<int> aiUnitTypes(GC.getNumUnitInfos(), 0);
	std::vector<int> aiUnitAI(NUM_UNITAI_TYPES, 0);
	std::vector<int> aiUnitCombat(GC.getNumUnitCombatInfos(), 0);
	std::vector<int> aiPromotions(bLogPromotionDetails ? GC.getNumPromotionInfos() : 0, 0);
	std::vector<int> aiMilitaryPromotions(bLogPromotionDetails ? GC.getNumPromotionInfos() : 0, 0);
	// <!-- custom: Reuse this already-required unit census to preserve the broad state between "owns assault transports" and "successfully projects an army overseas."
	// Individual lift/cargo counters are collected here; assault-group state is classified once per group below. Keep per-target UWAI/InvasionGraph reasoning in BBAI rather than duplicating it into SASGameRecord. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	int iAssaultTransports = 0;
	int iCargoCapacity = 0;
	int iLoadedCargo = 0;
	int iOpenOceanTransports = 0;
	int iOpenOceanCapacity = 0;
	int iOpenOceanLoadedCargo = 0;
	int iTransportsEmpty = 0;
	int iTransportsPartial = 0;
	int iTransportsFull = 0;
	int iDamagedTransports = 0;
	int iLoadedCargoCanAttack = 0;
	int iLoadedCargoCannotAttack = 0;
	std::vector<int> aiAssaultTransportTypes(GC.getNumUnitInfos(), 0);
	std::vector<int> aiAssaultCargoTypes(GC.getNumUnitInfos(), 0);
	std::vector<int> aiAssaultCargoAI(NUM_UNITAI_TYPES, 0);
	std::vector<CvUnit*> apAssaultCargoUnits;
	int iLoop = 0;
	for (CvUnit const* pLoopUnit = kPlayer.firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = kPlayer.nextUnit(&iLoop))
	{
		iTotal++;
		if (pLoopUnit->getUnitType() != NO_UNIT)
			aiUnitTypes[pLoopUnit->getUnitType()]++;
		const int iExperience = pLoopUnit->getExperience();
		iTotalExperience += iExperience;
		iMaxExperience = std::max(iMaxExperience, iExperience);
		if (pLoopUnit->isPromotionReady())
			iPromotionReady++;
		if (pLoopUnit->getLevel() >= 2)
			iLevel2Plus++;
		if (pLoopUnit->getLevel() >= 4)
			iLevel4Plus++;
		if (pLoopUnit->getLevel() >= 6)
			iLevel6Plus++;
		CvPlot const* pPlot = pLoopUnit->plot();
		const bool bMilitary = isSASGameRecordMilitaryUnit(*pLoopUnit);
		if (bMilitary)
		{
			iMilitary++;
			iMilitaryExperience += iExperience;
			iMaxMilitaryExperience = std::max(iMaxMilitaryExperience, iExperience);
			iMilitaryLevelTotal += pLoopUnit->getLevel();
			iMaxMilitaryLevel = std::max(iMaxMilitaryLevel, pLoopUnit->getLevel());
			if (pLoopUnit->isPromotionReady()) iMilitaryPromotionReady++;
			if (pLoopUnit->getLeaderUnitType() != NO_UNIT) iGreatGeneralLedMilitary++;
			int const iXmlCost = (pLoopUnit->getUnitType() == NO_UNIT ? -1 : GC.getInfo(pLoopUnit->getUnitType()).getProductionCost());
			if (iXmlCost > 0)
			{
				iMilitaryCostedUnits++;
				iMilitaryXmlProductionCost += iXmlCost;
				iMilitaryProductionNeeded += kPlayer.getProductionNeeded(pLoopUnit->getUnitType());
			}
			if (pLoopUnit->getDamage() > 0) iWoundedMilitary++;
			int const iMaxHP = pLoopUnit->maxHitPoints();
			if (iMaxHP > 0)
			{
				iMilitaryHealthMeasured++;
				int const iHealthX100 = (10000 * pLoopUnit->currHitPoints()) / iMaxHP;
				iMilitaryHealthX100Total += iHealthX100;
				iMinMilitaryHealthX100 = (iMinMilitaryHealthX100 < 0 ? iHealthX100 : std::min(iMinMilitaryHealthX100, iHealthX100));
				iMaxMilitaryHealthX100 = std::max(iMaxMilitaryHealthX100, iHealthX100);
				if (iHealthX100 >= 10000) iMilitaryHealthFull++;
				else if (iHealthX100 > 6600) iMilitaryHealthHigh++;
				else if (iHealthX100 > 3300) iMilitaryHealthMedium++;
				else iMilitaryHealthLow++;
			}
			if (pLoopUnit->getDomainType() == DOMAIN_SEA)
				iSeaMilitary++;
			else if (pLoopUnit->getDomainType() == DOMAIN_AIR)
				iAirMilitary++;
			else iLandMilitary++;
			if (pPlot != NULL && pPlot->isCity() && pLoopUnit->canDefend(pPlot))
				iCityDefenders++;
			else iFieldArmy++;
		}
		UnitAITypes eUnitAI = pLoopUnit->AI_getUnitAIType();
		if (eUnitAI >= 0 && eUnitAI < NUM_UNITAI_TYPES)
		{
			aiUnitAI[eUnitAI]++;
			if (eUnitAI == UNITAI_ATTACK_AIR) iAttackAir++;
			else if (eUnitAI == UNITAI_DEFENSE_AIR) iDefenseAir++;
			else if (eUnitAI == UNITAI_CARRIER_AIR) iCarrierAir++;
			else if (eUnitAI == UNITAI_MISSILE_AIR) iMissileAir++;
			else if (eUnitAI == UNITAI_ICBM) iICBM++;
			else if (eUnitAI == UNITAI_CARRIER_SEA) iCarrierSea++;
			else if (eUnitAI == UNITAI_MISSILE_CARRIER_SEA) iMissileCarrierSea++;
		}
		if (eUnitAI == UNITAI_ASSAULT_SEA && pLoopUnit->cargoSpace() > 0)
		{
			iAssaultTransports++;
			iCargoCapacity += pLoopUnit->cargoSpace();
			iLoadedCargo += pLoopUnit->getCargo();
			if (pLoopUnit->getCargo() <= 0) iTransportsEmpty++;
			else if (pLoopUnit->getCargo() >= pLoopUnit->cargoSpace()) iTransportsFull++;
			else iTransportsPartial++;
			if (pLoopUnit->getUnitType() != NO_UNIT)
				aiAssaultTransportTypes[pLoopUnit->getUnitType()]++;
			apAssaultCargoUnits.clear();
			pLoopUnit->getCargoUnits(apAssaultCargoUnits);
			for (uint iCargo = 0; iCargo < apAssaultCargoUnits.size(); iCargo++)
			{
				CvUnit const& kCargo = *apAssaultCargoUnits[iCargo];
				if (kCargo.canAttack()) iLoadedCargoCanAttack++;
				else iLoadedCargoCannotAttack++;
				if (kCargo.getUnitType() != NO_UNIT)
					aiAssaultCargoTypes[kCargo.getUnitType()]++;
				UnitAITypes const eCargoUnitAI = kCargo.AI_getUnitAIType();
				if (eCargoUnitAI >= 0 && eCargoUnitAI < NUM_UNITAI_TYPES)
					aiAssaultCargoAI[eCargoUnitAI]++;
			}
			if (pLoopUnit->getDamage() > 0)
				iDamagedTransports++;
			if (isSASGameRecordOpenOceanSeaUnit(*pLoopUnit))
			{
				iOpenOceanTransports++;
				iOpenOceanCapacity += pLoopUnit->cargoSpace();
				iOpenOceanLoadedCargo += pLoopUnit->getCargo();
			}
		}
		if (pLoopUnit->getDomainType() == DOMAIN_AIR && pLoopUnit->isCargo())
		{
			iAirCargo++;
			if (eUnitAI == UNITAI_CARRIER_AIR) iCarrierAirCargo++;
			else if (eUnitAI == UNITAI_MISSILE_AIR) iMissileCargo++;
		}
		if (pLoopUnit->isNuke()) iNukes++;
		if (pLoopUnit->isBlockading()) iBlockadingUnits++;
		UnitCombatTypes eUnitCombat = pLoopUnit->getUnitCombatType();
		if (eUnitCombat != NO_UNITCOMBAT)
		{
			aiUnitCombat[eUnitCombat]++;
			iUnitCombatTotal++;
		}
		if (bLogPromotionDetails)
		{
			FOR_EACH_ENUM(Promotion)
			{
				if (pLoopUnit->isHasPromotion(eLoopPromotion))
				{
					aiPromotions[eLoopPromotion]++;
					iPromotionInstances++;
					if (bMilitary)
					{
						aiMilitaryPromotions[eLoopPromotion]++;
						iMilitaryPromotionInstances++;
					}
				}
			}
		}
		if (isSASGameRecordWorkerUnit(*pLoopUnit))
			iWorkers++;
		if (isSASGameRecordSettlerUnit(*pLoopUnit))
			iSettlers++;
		if (eUnitAI == UNITAI_EXPLORE || eUnitAI == UNITAI_EXPLORE_SEA)
			iRecon++;
		if (pPlot != NULL)
		{
			if (pPlot->isCity())
				iUnitsInCities++;
			if (pPlot->getOwner() == ePlayer)
				iOwnTerritory++;
			else if (pPlot->getTeam() != NO_TEAM && GET_TEAM(eTeam).isAtWar(pPlot->getTeam()))
				iEnemyTerritory++;
			else iNeutralTerritory++;
		}
	}
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
	{
		PlayerTypes eLoopPlayer = (PlayerTypes)iPlayer;
		CvPlayer const& kLoopPlayer = GET_PLAYER(eLoopPlayer);
		if (!kLoopPlayer.isAlive() || kLoopPlayer.getTeam() == eTeam || !GET_TEAM(eTeam).isAtWar(kLoopPlayer.getTeam()))
			continue;
		int iEnemyLoop = 0;
		for (CvUnit const* pLoopUnit = kLoopPlayer.firstUnit(&iEnemyLoop); pLoopUnit != NULL; pLoopUnit = kLoopPlayer.nextUnit(&iEnemyLoop))
		{
			CvPlot const* pPlot = pLoopUnit->plot();
			if (pPlot != NULL && pPlot->getOwner() == ePlayer)
				iEnemyUnitsInTerritory++;
		}
	}
	int iAssaultGroups = 0;
	int iGroupsEmpty = 0;
	int iGroupsPartial = 0;
	int iGroupsFull = 0;
	int iGroupsAtBase = 0;
	int iGroupsEmptyAtBase = 0;
	int iGroupsEmptyNoMissionAIAtBase = 0;
	int iGroupsLoadedAwayFromBase = 0;
	int iGroupsNoMissionAI = 0;
	int iGroupsAssault = 0;
	int iGroupsPickup = 0;
	int iGroupsReinforce = 0;
	int iGroupsOtherMissionAI = 0;
	int iGroupsMissionQueueNonempty = 0;
	int iGroupsHealing = 0;
	int iOpenOceanGroups = 0;
	int iGroupsWithSeaCombatSupport = 0;
	int iGroupedSeaCombatSupportUnits = 0;
	CvString szAssaultMissionTargets;
	int iGroupLoop = 0;
	for (CvSelectionGroup const* pLoopGroup = kPlayer.firstSelectionGroup(&iGroupLoop); pLoopGroup != NULL; pLoopGroup = kPlayer.nextSelectionGroup(&iGroupLoop))
	{
		int iGroupAssaultCapacity = 0;
		int iGroupAssaultCargo = 0;
		int iGroupSeaCombatSupport = 0;
		bool bOpenOceanGroup = true;
		FOR_EACH_UNIT_IN(pGroupUnit, *pLoopGroup)
		{
			if (pGroupUnit->getDomainType() != DOMAIN_SEA || !isSASGameRecordOpenOceanSeaUnit(*pGroupUnit))
				bOpenOceanGroup = false;
			if (pGroupUnit->AI_getUnitAIType() == UNITAI_ASSAULT_SEA && pGroupUnit->cargoSpace() > 0)
			{
				iGroupAssaultCapacity += pGroupUnit->cargoSpace();
				iGroupAssaultCargo += pGroupUnit->getCargo();
			}
			else if (pGroupUnit->getDomainType() == DOMAIN_SEA && isSASGameRecordMilitaryUnit(*pGroupUnit))
				iGroupSeaCombatSupport++;
		}
		if (iGroupAssaultCapacity <= 0)
			continue;
		iAssaultGroups++;
		if (iGroupAssaultCargo <= 0) iGroupsEmpty++;
		else if (iGroupAssaultCargo >= iGroupAssaultCapacity) iGroupsFull++;
		else iGroupsPartial++;
		CvPlot const* pGroupPlot = pLoopGroup->plot();
		// <!-- custom: Match the transport AI's own "in port" concept rather than city tiles only.
		// CvTeam::isBase also recognizes peacefully usable foreign cities and ActsAsCity improvements (e.g. forts), which take the same AI_assaultSeaMove branch. (ChatGPT-5.6-Sol) -->
		bool const bAtBase = (pGroupPlot != NULL && GET_TEAM(eTeam).isBase(*pGroupPlot));
		if (bAtBase)
		{
			iGroupsAtBase++;
			if (iGroupAssaultCargo <= 0) iGroupsEmptyAtBase++;
		}
		else if (iGroupAssaultCargo > 0) iGroupsLoadedAwayFromBase++;
		MissionAITypes const eMissionAI = pLoopGroup->AI().AI_getMissionAIType();
		if (eMissionAI == NO_MISSIONAI) iGroupsNoMissionAI++;
		else if (eMissionAI == MISSIONAI_ASSAULT) iGroupsAssault++;
		else if (eMissionAI == MISSIONAI_PICKUP) iGroupsPickup++;
		else if (eMissionAI == MISSIONAI_REINFORCE) iGroupsReinforce++;
		else iGroupsOtherMissionAI++;
		if (bLogNavalMissionTargets && eMissionAI != NO_MISSIONAI)
		{
			CvPlot const* pMissionPlot = pLoopGroup->AI().AI_getMissionAIPlot();
			CvCity const* pMissionCity = (pMissionPlot == NULL ? NULL : pMissionPlot->getPlotCity());
			CvString szMissionTarget;
			szMissionTarget.Format(szAssaultMissionTargets.empty() ? "%d:%d@(%d,%d)>(%d,%d):team%d:city%d:%d/%d:support%d:base%d:ocean%d" : ";%d:%d@(%d,%d)>(%d,%d):team%d:city%d:%d/%d:support%d:base%d:ocean%d",
				pLoopGroup->getID(), eMissionAI, (pGroupPlot == NULL ? -1 : pGroupPlot->getX()),
				(pGroupPlot == NULL ? -1 : pGroupPlot->getY()),
				(pMissionPlot == NULL ? -1 : pMissionPlot->getX()), (pMissionPlot == NULL ? -1 : pMissionPlot->getY()),
				(pMissionPlot == NULL ? NO_TEAM : pMissionPlot->getTeam()),
				(pMissionCity == NULL ? -1 : pMissionCity->getID()),
				iGroupAssaultCargo, iGroupAssaultCapacity, iGroupSeaCombatSupport, bAtBase, bOpenOceanGroup);
			szAssaultMissionTargets += szMissionTarget;
		}
		if (bAtBase && iGroupAssaultCargo <= 0 && eMissionAI == NO_MISSIONAI)
			iGroupsEmptyNoMissionAIAtBase++;
		if (pLoopGroup->getLengthMissionQueue() > 0) iGroupsMissionQueueNonempty++;
		if (pLoopGroup->getActivityType() == ACTIVITY_HEAL) iGroupsHealing++;
		if (bOpenOceanGroup) iOpenOceanGroups++;
		if (iGroupSeaCombatSupport > 0)
		{
			iGroupsWithSeaCombatSupport++;
			iGroupedSeaCombatSupportUnits += iGroupSeaCombatSupport;
		}
	}
	CvString szUnitTypes;
	CvString szUnitAI;
	CvString szUnitCombat;
	CvString szUnitCombatPercentX100;
	CvString szPromotions;
	CvString szMilitaryPromotions;
	// <!-- custom: UnitAI and combat class are useful but too coarse for game-record review: a Galley and Galleon can share naval transport roles, and a Camel Archer and Dragoon can sit in similar mounted/combat buckets despite very different strength and era impact.
	// Include actual unit-type counts so LLM/autoplay review can see army and navy quality without per-unit spam. (GPT-5.5) -->
	for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
		appendSASGameRecordTypeCount(szUnitTypes, getSASGameRecordUnitType((UnitTypes)iI), aiUnitTypes[iI]);
	for (int iI = 0; iI < NUM_UNITAI_TYPES; iI++)
		appendSASGameRecordTypeCount(szUnitAI, getSASGameRecordUnitAIType((UnitAITypes)iI), aiUnitAI[iI]);
	for (int iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
	{
		appendSASGameRecordTypeCount(szUnitCombat, getSASGameRecordUnitCombatType((UnitCombatTypes)iI), aiUnitCombat[iI]);
		if (aiUnitCombat[iI] > 0)
			appendSASGameRecordValue(szUnitCombatPercentX100, getSASGameRecordUnitCombatType((UnitCombatTypes)iI), getSASGameRecordPercentX100(aiUnitCombat[iI], iUnitCombatTotal));
	}
	CvString szAssaultTransportTypes;
	CvString szAssaultCargoTypes;
	CvString szAssaultCargoAI;
	for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
	{
		appendSASGameRecordTypeCount(szAssaultTransportTypes, getSASGameRecordUnitType((UnitTypes)iI), aiAssaultTransportTypes[iI]);
		appendSASGameRecordTypeCount(szAssaultCargoTypes, getSASGameRecordUnitType((UnitTypes)iI), aiAssaultCargoTypes[iI]);
	}
	for (int iI = 0; iI < NUM_UNITAI_TYPES; iI++)
		appendSASGameRecordTypeCount(szAssaultCargoAI, getSASGameRecordUnitAIType((UnitAITypes)iI), aiAssaultCargoAI[iI]);
	if (bLogPromotionDetails)
	{
		FOR_EACH_ENUM(Promotion)
		{
			appendSASGameRecordTypeCount(szPromotions, getSASGameRecordPromotionType(eLoopPromotion), aiPromotions[eLoopPromotion]);
			appendSASGameRecordTypeCount(szMilitaryPromotions, getSASGameRecordPromotionType(eLoopPromotion), aiMilitaryPromotions[eLoopPromotion]);
		}
	}
	// <!-- custom: Keep late-game air/missile/nuclear posture and current naval-blockade count on the existing unit row rather than adding repetitive snapshot rows.
	// Exact blockade unit/range history remains event-driven.
	// UnitAI-specific counts make carrier filling and missile/nuke inventories directly visible. (GPT-5.6 + ChatGPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_UNIT_POSTURE turn=%d player=%d total=%d military=%d landMilitary=%d seaMilitary=%d airMilitary=%d attackAir=%d defenseAir=%d carrierAir=%d missileAir=%d icbm=%d carrierSea=%d missileCarrierSea=%d airCargo=%d carrierAirCargo=%d missileCargo=%d nukes=%d blockadingUnits=%d workers=%d settlers=%d recon=%d cityDefenders=%d fieldArmy=%d ownTerritory=%d enemyTerritory=%d neutralTerritory=%d unitsInCities=%d enemyUnitsInTerritory=%d totalXP=%d avgXpX100=%d maxXP=%d promotionReady=%d level2Plus=%d level4Plus=%d level6Plus=%d promotionInstances=%d militaryXP=%d avgMilitaryXpX100=%d maxMilitaryXP=%d avgMilitaryLevelX100=%d maxMilitaryLevel=%d militaryPromotionReady=%d greatGeneralLedMilitary=%d militaryCostedUnits=%d militaryXmlProductionCost=%d militaryProductionNeeded=%d woundedMilitary=%d militaryHealthMeasured=%d avgMilitaryHealthX100=%d minMilitaryHealthX100=%d maxMilitaryHealthX100=%d militaryHealthFull=%d militaryHealthHigh=%d militaryHealthMedium=%d militaryHealthLow=%d militaryPromotionInstances=%d",
		iGameTurn, ePlayer, iTotal, iMilitary, iLandMilitary, iSeaMilitary, iAirMilitary, iAttackAir, iDefenseAir, iCarrierAir, iMissileAir,
		iICBM, iCarrierSea, iMissileCarrierSea, iAirCargo, iCarrierAirCargo, iMissileCargo, iNukes, iBlockadingUnits, iWorkers, iSettlers,
		iRecon, iCityDefenders, iFieldArmy, iOwnTerritory, iEnemyTerritory, iNeutralTerritory, iUnitsInCities, iEnemyUnitsInTerritory,
		iTotalExperience, iTotal == 0 ? 0 : (100 * iTotalExperience) / iTotal, iMaxExperience, iPromotionReady, iLevel2Plus, iLevel4Plus,
		iLevel6Plus, iPromotionInstances, iMilitaryExperience, iMilitary == 0 ? 0 : (100 * iMilitaryExperience) / iMilitary,
		iMaxMilitaryExperience, iMilitary == 0 ? 0 : (100 * iMilitaryLevelTotal) / iMilitary, iMaxMilitaryLevel, iMilitaryPromotionReady,
		iGreatGeneralLedMilitary, iMilitaryCostedUnits, iMilitaryXmlProductionCost, iMilitaryProductionNeeded, iWoundedMilitary,
		iMilitaryHealthMeasured, iMilitaryHealthMeasured == 0 ? -1 : iMilitaryHealthX100Total / iMilitaryHealthMeasured,
		iMinMilitaryHealthX100, iMaxMilitaryHealthX100, iMilitaryHealthFull, iMilitaryHealthHigh, iMilitaryHealthMedium, iMilitaryHealthLow,
		iMilitaryPromotionInstances);
	logSASGameRecord("GAME_RECORD_UNIT_POSTURE_DELTAS turn=%d player=%d deltaValid=%d totalDelta=%+d militaryDelta=%+d workersDelta=%+d settlersDelta=%+d fieldArmyDelta=%+d cityDefendersDelta=%+d enemyUnitsInTerritoryDelta=%+d totalXPDelta=%+d promotionReadyDelta=%+d",
		iGameTurn, ePlayer, kPrevious.bValid, getSASGameRecordDelta(kPrevious.bValid, iTotal, kPrevious.iUnitTotal),
		getSASGameRecordDelta(kPrevious.bValid, iMilitary, kPrevious.iUnitMilitary),
		getSASGameRecordDelta(kPrevious.bValid, iWorkers, kPrevious.iUnitWorkers),
		getSASGameRecordDelta(kPrevious.bValid, iSettlers, kPrevious.iUnitSettlers),
		getSASGameRecordDelta(kPrevious.bValid, iFieldArmy, kPrevious.iUnitFieldArmy),
		getSASGameRecordDelta(kPrevious.bValid, iCityDefenders, kPrevious.iUnitCityDefenders),
		getSASGameRecordDelta(kPrevious.bValid, iEnemyUnitsInTerritory, kPrevious.iUnitEnemyUnitsInTerritory),
		getSASGameRecordDelta(kPrevious.bValid, iTotalExperience, kPrevious.iUnitTotalExperience),
		getSASGameRecordDelta(kPrevious.bValid, iPromotionReady, kPrevious.iUnitPromotionReady));
	kPrevious.iUnitTotal = iTotal;
	kPrevious.iUnitMilitary = iMilitary;
	kPrevious.iUnitWorkers = iWorkers;
	kPrevious.iUnitSettlers = iSettlers;
	kPrevious.iUnitFieldArmy = iFieldArmy;
	kPrevious.iUnitCityDefenders = iCityDefenders;
	kPrevious.iUnitEnemyUnitsInTerritory = iEnemyUnitsInTerritory;
	kPrevious.iUnitTotalExperience = iTotalExperience;
	kPrevious.iUnitPromotionReady = iPromotionReady;
	// <!-- custom: Record UnitCombat shares alongside the raw counts already collected so army mix (e.g. siege-heavy vs. siege-light) is immediately comparable without LLM/manual summing.
	// PercentX100 uses only units with a real UnitCombat as the denominator, excluding Workers, Great People and other non-combat-class units. (GPT-5.6) -->
	logSASGameRecord("GAME_RECORD_UNIT_COMPOSITION turn=%d player=%d unitTypes=%s unitAI=%s unitCombatTotal=%d unitCombat=%s unitCombatPercentX100=%s",
		iGameTurn, ePlayer, getSASDiagnosticOrDash(szUnitTypes).GetCString(), getSASDiagnosticOrDash(szUnitAI).GetCString(),
		iUnitCombatTotal, getSASDiagnosticOrDash(szUnitCombat).GetCString(), getSASDiagnosticOrDash(szUnitCombatPercentX100).GetCString());
	// <!-- custom: One compact periodic naval-projection row preserves whether completed assault lift is usable and actually being loaded/mobilized without copying per-target UWAI/InvasionGraph diagnostics into SASGameRecord.
	// Capacity/cargo use only real UNITAI_ASSAULT_SEA transports, groups are counted once regardless of their head unit, openOceanGroups require every sea member in the current group to cross ordinary ocean, and existing empire-wide UnitAI/war-plan rows remain authoritative rather than being duplicated here.
	// MISSIONAI_LOAD_ASSAULT belongs to land cargo groups seeking a transport, not to the assault-sea group itself; transport-side pickup readiness is represented by groupsPickup instead.
	// At level 3, missionTargets keeps each assault group's already-stored MissionAI destination plus current/target coordinates, target team/city, cargo/capacity, sea-combat support, base state and open-ocean capability, with no target search or pathfinding. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	FAssert(iAssaultTransports == iTransportsEmpty + iTransportsPartial + iTransportsFull);
	FAssert(iLoadedCargo == iLoadedCargoCanAttack + iLoadedCargoCannotAttack);
	FAssert(iAssaultGroups == iGroupsEmpty + iGroupsPartial + iGroupsFull);
	FAssert(iAssaultGroups == iGroupsNoMissionAI + iGroupsAssault + iGroupsPickup + iGroupsReinforce + iGroupsOtherMissionAI);
	logSASGameRecord("GAME_RECORD_NAVAL_ASSAULT_POSTURE turn=%d player=%d assaultTransports=%d assaultTransportsTraining=%d cargoCapacity=%d loadedCargo=%d cargoUtilizationPercentX100=%d openOceanTransports=%d openOceanCapacity=%d openOceanLoadedCargo=%d openOceanGroups=%d transportsEmpty=%d transportsPartial=%d transportsFull=%d damagedTransports=%d assaultGroups=%d groupsEmpty=%d groupsPartial=%d groupsFull=%d groupsAtBase=%d groupsEmptyAtBase=%d groupsEmptyNoMissionAIAtBase=%d groupsLoadedAwayFromBase=%d groupsNoMissionAI=%d groupsAssault=%d groupsPickup=%d groupsReinforce=%d groupsOtherMissionAI=%d groupsMissionQueueNonempty=%d groupsHealing=%d groupsWithSeaCombatSupport=%d groupedSeaCombatSupportUnits=%d loadedCargoCanAttack=%d loadedCargoCannotAttack=%d transportUnitTypes=%s cargoUnitTypes=%s cargoUnitAI=%s missionTargets=%s",
		iGameTurn, ePlayer, iAssaultTransports, GET_PLAYER(ePlayer).AI_getNumTrainAIUnits(UNITAI_ASSAULT_SEA), iCargoCapacity, iLoadedCargo,
		getSASGameRecordPercentX100(iLoadedCargo, iCargoCapacity), iOpenOceanTransports, iOpenOceanCapacity, iOpenOceanLoadedCargo,
		iOpenOceanGroups, iTransportsEmpty, iTransportsPartial, iTransportsFull, iDamagedTransports, iAssaultGroups, iGroupsEmpty,
		iGroupsPartial, iGroupsFull, iGroupsAtBase, iGroupsEmptyAtBase, iGroupsEmptyNoMissionAIAtBase, iGroupsLoadedAwayFromBase,
		iGroupsNoMissionAI, iGroupsAssault, iGroupsPickup, iGroupsReinforce, iGroupsOtherMissionAI, iGroupsMissionQueueNonempty,
		iGroupsHealing, iGroupsWithSeaCombatSupport, iGroupedSeaCombatSupportUnits, iLoadedCargoCanAttack, iLoadedCargoCannotAttack,
		getSASDiagnosticOrDash(szAssaultTransportTypes).GetCString(), getSASDiagnosticOrDash(szAssaultCargoTypes).GetCString(),
		getSASDiagnosticOrDash(szAssaultCargoAI).GetCString(), getSASDiagnosticOrDash(szAssaultMissionTargets).GetCString());
	if (bLogPromotionDetails) logSASGameRecord("GAME_RECORD_UNIT_PROMOTIONS turn=%d player=%d promotions=%s militaryPromotions=%s",
		iGameTurn, ePlayer, getSASDiagnosticOrDash(szPromotions).GetCString(), getSASDiagnosticOrDash(szMilitaryPromotions).GetCString());
}

static void logSASGameRecordWorkers(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	TeamTypes eTeam = kPlayer.getTeam();
	SASGameRecordPlayerPrevious& kPrevious = g_akSASGameRecordPlayerPrevious[ePlayer];
	bool const bLogWorkerDetails = (gGameRecordLogLevel >= 3);
	int iWorkers = 0;
	int iSeaWorkers = 0;
	int iIdle = 0;
	int iBuilding = 0;
	int iBuildingImprovement = 0;
	int iBuildingRoute = 0;
	int iMoving = 0;
	int iWaiting = 0;
	int iOwnTerritory = 0;
	int iEnemyTerritory = 0;
	int iNeutralTerritory = 0;
	int iGuarded = 0;
	int iUnguarded = 0;
	int iThreatened = 0;
	std::vector<int> aiBuilds(GC.getNumBuildInfos(), 0);
	int iLoop = 0;
	for (CvUnit const* pLoopUnit = kPlayer.firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = kPlayer.nextUnit(&iLoop))
	{
		if (!isSASGameRecordWorkerUnit(*pLoopUnit))
			continue;
		iWorkers++;
		if (pLoopUnit->AI_getUnitAIType() == UNITAI_WORKER_SEA || pLoopUnit->getDomainType() == DOMAIN_SEA)
			iSeaWorkers++;
		CvPlot const* pPlot = pLoopUnit->plot();
		MissionTypes eMission = getSASGameRecordUnitMissionType(*pLoopUnit);
		BuildTypes eBuild = pLoopUnit->getBuildType();
		if (eBuild != NO_BUILD)
		{
			iBuilding++;
			aiBuilds[eBuild]++;
			if (GC.getInfo(eBuild).getImprovement() != NO_IMPROVEMENT)
				iBuildingImprovement++;
			if (GC.getInfo(eBuild).getRoute() != NO_ROUTE)
				iBuildingRoute++;
		}
		else if (eMission == MISSION_MOVE_TO || eMission == MISSION_ROUTE_TO || eMission == MISSION_MOVE_TO_UNIT)
			iMoving++;
		else if (pLoopUnit->canMove())
			iIdle++;
		else iWaiting++;
		if (pPlot != NULL)
		{
			if (pPlot->getOwner() == ePlayer)
				iOwnTerritory++;
			else if (pPlot->getTeam() != NO_TEAM && GET_TEAM(eTeam).isAtWar(pPlot->getTeam()))
				iEnemyTerritory++;
			else iNeutralTerritory++;
		}
		// <!-- custom: These two checks feed both the level-2 aggregate and level-3 detail row.
		// Compute them once per worker instead of repeating the plot queries for detail logging. (ChatGPT-5.6-Sol) -->
		bool const bGuarded = isSASGameRecordUnitGuarded(*pLoopUnit);
		bool const bThreatened = isSASGameRecordUnitThreatened(*pLoopUnit);
		if (bGuarded)
			iGuarded++;
		else iUnguarded++;
		if (bThreatened)
			iThreatened++;
		if (bLogWorkerDetails && pPlot != NULL)
		{
			logSASGameRecord("GAME_RECORD_WORKER turn=%d player=%d unitId=%d unit=%s unitAI=%s x=%d y=%d mission=%s build=%s buildTurnsLeft=%d plotOwner=%d plotTerrain=%s plotFeature=%s plotBonus=%s plotImprovement=%s plotRoute=%s guarded=%d threatened=%d",
				iGameTurn, ePlayer, pLoopUnit->getID(), getSASGameRecordUnitType(pLoopUnit->getUnitType()),
				getSASGameRecordUnitAIType(pLoopUnit->AI_getUnitAIType()), pLoopUnit->getX(), pLoopUnit->getY(),
				getSASGameRecordMissionType(eMission), getSASGameRecordBuildType(eBuild), getSASGameRecordBuildTurnsLeft(*pLoopUnit, eBuild),
				pPlot->getOwner(), getSASGameRecordTerrainType(pPlot->getTerrainType()),
				getSASGameRecordFeatureType(pPlot->getFeatureType()), getSASGameRecordBonusType(pPlot->getBonusType(pLoopUnit->getTeam())),
				getSASGameRecordImprovementType(pPlot->getImprovementType()), getSASGameRecordRouteType(pPlot->getRouteType()), bGuarded,
				bThreatened);
		}
	}
	CvString szBuilds;
	for (int iI = 0; iI < GC.getNumBuildInfos(); iI++)
		appendSASGameRecordTypeCount(szBuilds, getSASGameRecordBuildType((BuildTypes)iI), aiBuilds[iI]);
	logSASGameRecord("GAME_RECORD_WORKERS turn=%d player=%d workers=%d seaWorkers=%d idle=%d building=%d buildingImprovement=%d buildingRoute=%d moving=%d waiting=%d ownTerritory=%d enemyTerritory=%d neutralTerritory=%d guarded=%d unguarded=%d threatened=%d builds=%s",
		iGameTurn, ePlayer, iWorkers, iSeaWorkers, iIdle, iBuilding, iBuildingImprovement, iBuildingRoute, iMoving, iWaiting, iOwnTerritory,
		iEnemyTerritory, iNeutralTerritory, iGuarded, iUnguarded, iThreatened, getSASDiagnosticOrDash(szBuilds).GetCString());
	logSASGameRecord("GAME_RECORD_WORKERS_DELTAS turn=%d player=%d deltaValid=%d workersDelta=%+d buildingDelta=%+d idleDelta=%+d movingDelta=%+d waitingDelta=%+d threatenedDelta=%+d",
		iGameTurn, ePlayer, kPrevious.bValid, getSASGameRecordDelta(kPrevious.bValid, iWorkers, kPrevious.iWorkerWorkers),
		getSASGameRecordDelta(kPrevious.bValid, iBuilding, kPrevious.iWorkerBuilding),
		getSASGameRecordDelta(kPrevious.bValid, iIdle, kPrevious.iWorkerIdle),
		getSASGameRecordDelta(kPrevious.bValid, iMoving, kPrevious.iWorkerMoving),
		getSASGameRecordDelta(kPrevious.bValid, iWaiting, kPrevious.iWorkerWaiting),
		getSASGameRecordDelta(kPrevious.bValid, iThreatened, kPrevious.iWorkerThreatened));
	kPrevious.iWorkerWorkers = iWorkers;
	kPrevious.iWorkerBuilding = iBuilding;
	kPrevious.iWorkerIdle = iIdle;
	kPrevious.iWorkerMoving = iMoving;
	kPrevious.iWorkerWaiting = iWaiting;
	kPrevious.iWorkerThreatened = iThreatened;
}

// <!-- custom: City-site rows deliberately reuse CvPlayerAI's already-maintained shortlist and the found values stored on those shortlist plots.
// Do not call AI_updateCitySites, CitySiteEvaluator, pathfinding or getFoundValue on arbitrary plots solely for logging; this keeps SASGameRecord observational and cheap while still preserving the AI's broad expansion intent. (ChatGPT-5.6-Sol) -->
static int getSASGameRecordCitySiteRank(CvPlayerAI const& kPlayer, CvPlot const& kPlot, int& iFoundValue)
{
	iFoundValue = -1;
	for (int iI = 0; iI < kPlayer.AI_getNumCitySites(); iI++)
	{
		CvPlot const& kSite = kPlayer.AI_getCitySite(iI);
		if (&kSite != &kPlot)
			continue;
		iFoundValue = kSite.getFoundValue(kPlayer.getID());
		return iI + 1;
	}
	return -1;
}

static void getSASGameRecordCitySiteAlternative(CvPlayerAI const& kPlayer, CvPlot const* pExclude, int iAlternativeIndex, int& iRank, int& iX, int& iY, int& iFoundValue)
{
	iRank = -1;
	iX = -1;
	iY = -1;
	iFoundValue = -1;
	int iSeen = 0;
	for (int iI = 0; iI < kPlayer.AI_getNumCitySites(); iI++)
	{
		CvPlot const& kSite = kPlayer.AI_getCitySite(iI);
		if (pExclude != NULL && &kSite == pExclude)
			continue;
		if (iSeen++ != iAlternativeIndex)
			continue;
		iRank = iI + 1;
		iX = kSite.getX();
		iY = kSite.getY();
		iFoundValue = kSite.getFoundValue(kPlayer.getID());
		return;
	}
}

// <!-- custom: Summarize each actual CvPlotGroup containing one of the player's cities, so a record directly shows which cities share a trade/resource network instead of requiring consumers to join periodic city rows by plotGroupId.
// Reuse the expansion map scan and report the route/river/network-terrain composition already present in each component; those counts describe available conduits, not the exact conduit required by every city pair, which would require diagnostic-only graph reconstruction. (GPT-5.6-Sol) -->
static void addSASGameRecordCityNetworkPlot(std::map<int, SASGameRecordCityNetwork>& kNetworks, CvPlot const& kPlot, PlayerTypes ePlayer, TeamTypes eTeam)
{
	CvPlotGroup const* pPlotGroup = kPlot.getPlotGroup(ePlayer);
	if (pPlotGroup == NULL)
		return;
	std::map<int, SASGameRecordCityNetwork>::iterator it = kNetworks.find(pPlotGroup->getID());
	if (it == kNetworks.end())
		return;
	SASGameRecordCityNetwork& kNetwork = it->second;
	kNetwork.iPlots++;
	if (kPlot.isWater())
		kNetwork.iWaterPlots++;
	else kNetwork.iLandPlots++;
	RouteTypes const eRoute = kPlot.getRevealedRouteType(eTeam);
	if (eRoute != NO_ROUTE)
	{
		kNetwork.iRoutePlots++;
		kNetwork.aiRoutes[eRoute]++;
	}
	if (kPlot.isRiverNetwork(eTeam))
		kNetwork.iRiverNetworkPlots++;
	if (kPlot.isNetworkTerrain(eTeam))
	{
		if (kPlot.isWater())
			kNetwork.iWaterNetworkPlots++;
		else kNetwork.iLandTerrainNetworkPlots++;
	}
	CvCity const* pCity = kPlot.getPlotCity();
	if (pCity == NULL)
		return;
	CvString szCity;
	szCity.Format(kNetwork.szCities.empty() ? "%d:%d:%S" : ",%d:%d:%S", pCity->getOwner(), pCity->getID(), pCity->getName().GetCString());
	kNetwork.szCities += szCity;
	kNetwork.iCities++;
	if (pCity->getOwner() == ePlayer)
	{
		kNetwork.iOwnCities++;
		if (pCity->isCapital())
			kNetwork.bContainsCapital = true;
	}
	else kNetwork.iForeignCities++;
}

static void logSASGameRecordCityNetworks(std::map<int, SASGameRecordCityNetwork> const& kNetworks, PlayerTypes ePlayer, int iGameTurn)
{
	for (std::map<int, SASGameRecordCityNetwork>::const_iterator it = kNetworks.begin(); it != kNetworks.end(); ++it)
	{
		SASGameRecordCityNetwork const& kNetwork = it->second;
		if (kNetwork.iOwnCities <= 0)
			continue;
		CvString szRoutes;
		for (int iI = 0; iI < GC.getNumRouteInfos(); iI++)
			appendSASGameRecordTypeCount(szRoutes, getSASGameRecordRouteType((RouteTypes)iI), kNetwork.aiRoutes[iI]);
		logSASGameRecord("GAME_RECORD_CITY_NETWORK turn=%d player=%d plotGroupId=%d containsCapital=%d cities=%d ownCities=%d foreignCities=%d networkPlots=%d landPlots=%d waterPlots=%d routePlots=%d riverNetworkPlots=%d landTerrainNetworkPlots=%d waterNetworkPlots=%d cityList=%s routes=%s",
			iGameTurn, ePlayer, it->first, kNetwork.bContainsCapital ? 1 : 0, kNetwork.iCities, kNetwork.iOwnCities, kNetwork.iForeignCities,
			kNetwork.iPlots, kNetwork.iLandPlots, kNetwork.iWaterPlots, kNetwork.iRoutePlots, kNetwork.iRiverNetworkPlots,
			kNetwork.iLandTerrainNetworkPlots, kNetwork.iWaterNetworkPlots, getSASDiagnosticQuoted(kNetwork.szCities.GetCString()).GetCString(),
			getSASDiagnosticOrDash(szRoutes).GetCString());
	}
}

static void logSASGameRecordExpansion(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	CvPlayerAI const& kPlayerAI = kPlayer.AI();
	const TeamTypes eTeam = kPlayer.getTeam();
	int iRevealedLand = 0;
	int iVisibleLand = 0;
	int iRevealedUnownedLand = 0;
	int iVisibleUnownedLand = 0;
	int iRevealedForeignLand = 0;
	int iVisibleForeignLand = 0;
	int iRevealedOtherTeamLand = 0;
	int iVisibleOtherTeamLand = 0;
	CvCity const* pCapital = kPlayer.getCapitalCity();
	int const iCapitalArea = (pCapital == NULL ? -1 : pCapital->getArea().getID());
	int iNearestRevealedOtherTeamLandDistance = -1;
	int iNearestRevealedEnemyLandDistance = -1;
	SASGameRecordTerritoryDevelopment kTerritoryDevelopment;
	std::map<int, SASGameRecordCityNetwork> kCityNetworks;
	int iCitiesProducingSettlers = 0;
	int iCityLoop = 0;
	for (CvCity const* pLoopCity = kPlayer.firstCity(&iCityLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iCityLoop))
	{
		CvPlotGroup const* pPlotGroup = pLoopCity->plotGroup(ePlayer);
		if (pPlotGroup != NULL)
			kCityNetworks[pPlotGroup->getID()];
		UnitTypes const eProductionUnit = pLoopCity->getProductionUnit();
		if (eProductionUnit != NO_UNIT && GC.getInfo(eProductionUnit).getDefaultUnitAIType() == UNITAI_SETTLE)
			iCitiesProducingSettlers++;
	}
	CvMap const& kMap = GC.getMap();
	for (int iI = 0; iI < kMap.numPlots(); iI++)
	{
		CvPlot const& kPlot = kMap.getPlotByIndex(iI);
		if (kPlot.getOwner() == ePlayer)
			addSASGameRecordTerritoryDevelopment(kTerritoryDevelopment, kPlot, ePlayer, eTeam);
		addSASGameRecordCityNetworkPlot(kCityNetworks, kPlot, ePlayer, eTeam);
		if (kPlot.isWater())
			continue;
		if (kPlot.isRevealed(eTeam, false))
		{
			iRevealedLand++;
			if (kPlot.getOwner() == NO_PLAYER)
				iRevealedUnownedLand++;
			else if (kPlot.getOwner() != ePlayer)
				iRevealedForeignLand++;
			if (kPlot.getTeam() != NO_TEAM && kPlot.getTeam() != eTeam)
			{
				iRevealedOtherTeamLand++;
				if (pCapital != NULL)
				{
					int const iDistance = plotDistance(pCapital->getX(), pCapital->getY(), kPlot.getX(), kPlot.getY());
					iNearestRevealedOtherTeamLandDistance = (iNearestRevealedOtherTeamLandDistance < 0 ? iDistance : std::min(iNearestRevealedOtherTeamLandDistance, iDistance));
					if (GET_TEAM(eTeam).isAtWar(kPlot.getTeam()))
						iNearestRevealedEnemyLandDistance = (iNearestRevealedEnemyLandDistance < 0 ? iDistance : std::min(iNearestRevealedEnemyLandDistance, iDistance));
				}
			}
		}
		if (kPlot.isVisible(eTeam, false))
		{
			iVisibleLand++;
			if (kPlot.getOwner() == NO_PLAYER)
				iVisibleUnownedLand++;
			else if (kPlot.getOwner() != ePlayer)
				iVisibleForeignLand++;
			if (kPlot.getTeam() != NO_TEAM && kPlot.getTeam() != eTeam)
				iVisibleOtherTeamLand++;
		}
	}
	logSASGameRecordCityNetworks(kCityNetworks, ePlayer, iGameTurn);
	// <!-- custom: One periodic shortlist summary answers whether the AI currently sees worthwhile expansion opportunities without serializing every plot valuation.
	// The shortlist is already maintained by normal AI code; reading its primary entry and current threshold adds no map scan or candidate evaluation. (ChatGPT-5.6-Sol) -->
	int const iCitySites = kPlayerAI.AI_getNumCitySites();
	int const iMinFoundValue = kPlayerAI.AI_getMinFoundValue();
	int iPrimarySiteX = -1;
	int iPrimarySiteY = -1;
	int iPrimarySiteFoundValue = -1;
	CvString szRankedCitySites;
	if (iCitySites > 0)
	{
		CvPlot const& kPrimarySite = kPlayerAI.AI_getCitySite(0);
		iPrimarySiteX = kPrimarySite.getX();
		iPrimarySiteY = kPrimarySite.getY();
		iPrimarySiteFoundValue = kPrimarySite.getFoundValue(ePlayer);
		// <!-- custom: At level 3, preserve the whole already-maintained city-site shortlist rather than only its primary entry.
		// AI_updateCitySites keeps at most four sites by default, so this stays compact and adds no city-site evaluation, map scan, pathfinding or RNG. Rank is the cached AI order and each item stores x/y plus its current found value. (ChatGPT-5.6-Sol) -->
		if (gGameRecordLogLevel >= 3)
		{
			for (int iSite = 0; iSite < iCitySites; iSite++)
			{
				CvPlot const& kSite = kPlayerAI.AI_getCitySite(iSite);
				CvString szSite;
				szSite.Format(szRankedCitySites.empty() ? "%d@(%d,%d):%d" : ";%d@(%d,%d):%d",
						iSite + 1, kSite.getX(), kSite.getY(), kSite.getFoundValue(ePlayer));
				szRankedCitySites += szSite;
			}
		}
	}
	int iSettlers = 0;
	int iFoundMission = 0;
	int iFoundIntent = 0;
	int iNearestSettlerCityDistance = -1;
	int iTotalSettlerCityDistance = 0;
	int iSettlersWithCityDistance = 0;
	int iUnitLoop = 0;
	for (CvUnit const* pLoopUnit = kPlayer.firstUnit(&iUnitLoop); pLoopUnit != NULL; pLoopUnit = kPlayer.nextUnit(&iUnitLoop))
	{
		if (!isSASGameRecordSettlerUnit(*pLoopUnit))
			continue;
		iSettlers++;
		if (getSASGameRecordUnitMissionType(*pLoopUnit) == MISSION_FOUND)
			iFoundMission++;
		CvSelectionGroup const* pGroup = pLoopUnit->getGroup();
		if (pGroup != NULL && pGroup->AI().AI_getMissionAIType() == MISSIONAI_FOUND)
			iFoundIntent++;
		CvCity const* pNearestCity = kMap.findCity(pLoopUnit->getX(), pLoopUnit->getY(), ePlayer, NO_TEAM, false);
		if (pNearestCity == NULL)
			continue;
		const int iDistance = plotDistance(pLoopUnit->getX(), pLoopUnit->getY(), pNearestCity->getX(), pNearestCity->getY());
		iNearestSettlerCityDistance = (iNearestSettlerCityDistance < 0 ? iDistance : std::min(iNearestSettlerCityDistance, iDistance));
		iTotalSettlerCityDistance += iDistance;
		iSettlersWithCityDistance++;
	}
	const int iAvgSettlerCityDistanceX100 = (iSettlersWithCityDistance == 0 ? -1 : (100 * iTotalSettlerCityDistance) / iSettlersWithCityDistance);

	// <!-- custom: Add a knowledge-limited rival-distance complement to the already map-wide revealed land counts.
	// Reuse the existing plot scan for nearest revealed other-team/enemy territory and measure cities only from the current capital, avoiding an own-city x foreign-city scan; capital-area counts separately expose same-landmass competition. (ChatGPT-5.6-Sol) -->
	int iMetForeignPlayers = 0;
	int iForeignPlayersWithKnownCities = 0;
	int iKnownForeignCities = 0;
	int iCapitalAreaForeignPlayersWithKnownCities = 0;
	int iCapitalAreaKnownForeignCities = 0;
	PlayerTypes eNearestKnownForeignCityPlayer = NO_PLAYER;
	int iNearestKnownForeignCityId = -1;
	int iNearestKnownForeignCityDistance = -1;
	PlayerTypes eNearestCapitalAreaKnownForeignCityPlayer = NO_PLAYER;
	int iNearestCapitalAreaKnownForeignCityId = -1;
	int iNearestCapitalAreaKnownForeignCityDistance = -1;
	int iWarEnemyPlayers = 0;
	int iEnemyPlayersWithKnownCities = 0;
	int iKnownEnemyCities = 0;
	PlayerTypes eNearestKnownEnemyCityPlayer = NO_PLAYER;
	int iNearestKnownEnemyCityId = -1;
	int iNearestKnownEnemyCityDistance = -1;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes const eOtherPlayer = (PlayerTypes)iI;
		CvPlayer const& kOtherPlayer = GET_PLAYER(eOtherPlayer);
		if (!kOtherPlayer.isAlive() || kOtherPlayer.isBarbarian() || kOtherPlayer.getTeam() == eTeam)
			continue;
		bool const bMet = GET_TEAM(eTeam).isHasMet(kOtherPlayer.getTeam());
		bool const bEnemy = GET_TEAM(eTeam).isAtWar(kOtherPlayer.getTeam());
		if (bMet)
			iMetForeignPlayers++;
		if (bEnemy)
			iWarEnemyPlayers++;
		bool bHasKnownCity = false;
		bool bHasKnownCapitalAreaCity = false;
		bool bEnemyHasKnownCity = false;
		int iOtherCityLoop = 0;
		for (CvCity const* pOtherCity = kOtherPlayer.firstCity(&iOtherCityLoop); pOtherCity != NULL; pOtherCity = kOtherPlayer.nextCity(&iOtherCityLoop))
		{
			if (!pOtherCity->isRevealed(eTeam))
				continue;
			bHasKnownCity = true;
			iKnownForeignCities++;
			if (iCapitalArea >= 0 && pOtherCity->getArea().getID() == iCapitalArea)
			{
				bHasKnownCapitalAreaCity = true;
				iCapitalAreaKnownForeignCities++;
				if (pCapital != NULL)
				{
					int const iCapitalAreaDistance = plotDistance(pCapital->getX(), pCapital->getY(), pOtherCity->getX(), pOtherCity->getY());
					if (iNearestCapitalAreaKnownForeignCityDistance < 0 || iCapitalAreaDistance < iNearestCapitalAreaKnownForeignCityDistance)
					{
						iNearestCapitalAreaKnownForeignCityDistance = iCapitalAreaDistance;
						eNearestCapitalAreaKnownForeignCityPlayer = eOtherPlayer;
						iNearestCapitalAreaKnownForeignCityId = pOtherCity->getID();
					}
				}
			}
			if (bEnemy)
			{
				bEnemyHasKnownCity = true;
				iKnownEnemyCities++;
			}
			if (pCapital == NULL)
				continue;
			int const iDistance = plotDistance(pCapital->getX(), pCapital->getY(), pOtherCity->getX(), pOtherCity->getY());
			if (iNearestKnownForeignCityDistance < 0 || iDistance < iNearestKnownForeignCityDistance)
			{
				iNearestKnownForeignCityDistance = iDistance;
				eNearestKnownForeignCityPlayer = eOtherPlayer;
				iNearestKnownForeignCityId = pOtherCity->getID();
			}
			if (bEnemy && (iNearestKnownEnemyCityDistance < 0 || iDistance < iNearestKnownEnemyCityDistance))
			{
				iNearestKnownEnemyCityDistance = iDistance;
				eNearestKnownEnemyCityPlayer = eOtherPlayer;
				iNearestKnownEnemyCityId = pOtherCity->getID();
			}
		}
		if (bHasKnownCity)
			iForeignPlayersWithKnownCities++;
		if (bHasKnownCapitalAreaCity)
			iCapitalAreaForeignPlayersWithKnownCities++;
		if (bEnemyHasKnownCity)
			iEnemyPlayersWithKnownCities++;
	}
	logSASGameRecord("GAME_RECORD_EXPANSION turn=%d player=%d cities=%d targetCities=%d ownedLand=%d revealedLand=%d visibleLand=%d revealedUnownedLand=%d visibleUnownedLand=%d revealedForeignLand=%d visibleForeignLand=%d revealedOtherTeamLand=%d visibleOtherTeamLand=%d citySites=%d minFoundValue=%d primarySiteX=%d primarySiteY=%d primarySiteFoundValue=%d settlers=%d foundMission=%d foundIntent=%d citiesProducingSettlers=%d nearestSettlerCityDistance=%d avgSettlerCityDistanceX100=%d capitalArea=%d nearestRevealedOtherTeamLandFromCapitalDistance=%d nearestRevealedEnemyLandFromCapitalDistance=%d metForeignPlayers=%d foreignPlayersWithKnownCities=%d knownForeignCities=%d capitalAreaForeignPlayersWithKnownCities=%d capitalAreaKnownForeignCities=%d nearestKnownForeignCityPlayer=%d nearestKnownForeignCityId=%d nearestKnownForeignCityFromCapitalDistance=%d nearestCapitalAreaKnownForeignCityPlayer=%d nearestCapitalAreaKnownForeignCityId=%d nearestCapitalAreaKnownForeignCityFromCapitalDistance=%d warEnemyPlayers=%d enemyPlayersWithKnownCities=%d knownEnemyCities=%d nearestKnownEnemyCityPlayer=%d nearestKnownEnemyCityId=%d nearestKnownEnemyCityFromCapitalDistance=%d",
		iGameTurn, ePlayer, kPlayer.getNumCities(), GC.getInfo(kMap.getWorldSize()).getTargetNumCities(), kPlayer.getTotalLand(),
		iRevealedLand, iVisibleLand, iRevealedUnownedLand, iVisibleUnownedLand, iRevealedForeignLand, iVisibleForeignLand,
		iRevealedOtherTeamLand, iVisibleOtherTeamLand, iCitySites, iMinFoundValue, iPrimarySiteX, iPrimarySiteY, iPrimarySiteFoundValue,
		iSettlers, iFoundMission, iFoundIntent, iCitiesProducingSettlers, iNearestSettlerCityDistance, iAvgSettlerCityDistanceX100,
		iCapitalArea, iNearestRevealedOtherTeamLandDistance, iNearestRevealedEnemyLandDistance, iMetForeignPlayers,
		iForeignPlayersWithKnownCities, iKnownForeignCities, iCapitalAreaForeignPlayersWithKnownCities, iCapitalAreaKnownForeignCities,
		eNearestKnownForeignCityPlayer, iNearestKnownForeignCityId, iNearestKnownForeignCityDistance,
		eNearestCapitalAreaKnownForeignCityPlayer, iNearestCapitalAreaKnownForeignCityId, iNearestCapitalAreaKnownForeignCityDistance,
		iWarEnemyPlayers, iEnemyPlayersWithKnownCities, iKnownEnemyCities, eNearestKnownEnemyCityPlayer, iNearestKnownEnemyCityId,
		iNearestKnownEnemyCityDistance);
	if (gGameRecordLogLevel >= 3 && iCitySites > 0)
	{
		logSASGameRecord("GAME_RECORD_CITY_SITES turn=%d player=%d count=%d minFoundValue=%d rankedSites=%s",
				iGameTurn, ePlayer, iCitySites, iMinFoundValue, szRankedCitySites.GetCString());
	}
	logSASGameRecordTerritoryDevelopment(ePlayer, iGameTurn, kTerritoryDevelopment);
}

static void logSASGameRecordSettlers(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	TeamTypes eTeam = kPlayer.getTeam();
	SASGameRecordPlayerPrevious& kPrevious = g_akSASGameRecordPlayerPrevious[ePlayer];
	bool const bLogSettlerDetails = (gGameRecordLogLevel >= 3);
	int iSettlers = 0;
	int iFoundMission = 0;
	int iFoundIntent = 0;
	int iMoving = 0;
	int iIdle = 0;
	int iWaiting = 0;
	int iOwnTerritory = 0;
	int iEnemyTerritory = 0;
	int iNeutralTerritory = 0;
	int iGuarded = 0;
	int iUnguarded = 0;
	int iThreatened = 0;
	int iLoop = 0;
	for (CvUnit const* pLoopUnit = kPlayer.firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = kPlayer.nextUnit(&iLoop))
	{
		if (!isSASGameRecordSettlerUnit(*pLoopUnit))
			continue;
		iSettlers++;
		CvPlot const* pPlot = pLoopUnit->plot();
		CvSelectionGroup const* pGroup = pLoopUnit->getGroup();
		CvSelectionGroupAI const* pGroupAI = (pGroup == NULL ? NULL : &pGroup->AI());
		MissionAITypes const eMissionAI = (pGroupAI == NULL ? NO_MISSIONAI : pGroupAI->AI_getMissionAIType());
		CvPlot const* pMissionPlot = (pGroupAI == NULL ? NULL : pGroupAI->AI_getMissionAIPlot());
		MissionTypes eMission = getSASGameRecordUnitMissionType(*pLoopUnit);
		if (eMission == MISSION_FOUND)
			iFoundMission++;
		if (eMissionAI == MISSIONAI_FOUND)
			iFoundIntent++;
		if (eMission == MISSION_MOVE_TO || eMission == MISSION_ROUTE_TO || eMission == MISSION_MOVE_TO_UNIT)
			iMoving++;
		else if (pLoopUnit->canMove())
			iIdle++;
		else iWaiting++;
		if (pPlot != NULL)
		{
			if (pPlot->getOwner() == ePlayer)
				iOwnTerritory++;
			else if (pPlot->getTeam() != NO_TEAM && GET_TEAM(eTeam).isAtWar(pPlot->getTeam()))
				iEnemyTerritory++;
			else iNeutralTerritory++;
		}
		bool const bGuarded = isSASGameRecordUnitGuarded(*pLoopUnit);
		bool const bThreatened = isSASGameRecordUnitThreatened(*pLoopUnit);
		if (bGuarded)
			iGuarded++;
		else iUnguarded++;
		if (bThreatened)
			iThreatened++;
		if (bLogSettlerDetails && pPlot != NULL)
		{
			CvCity const* pNearestCity = GC.getMap().findCity(pLoopUnit->getX(), pLoopUnit->getY(), ePlayer, NO_TEAM, false);
			const int iNearestDistance = pNearestCity == NULL ? -1 : plotDistance(pLoopUnit->getX(), pLoopUnit->getY(), pNearestCity->getX(), pNearestCity->getY());
			int iTargetSiteFoundValue = -1;
			int iTargetSiteRank = -1;
			int iTargetX = -1;
			int iTargetY = -1;
			int iTargetArea = -1;
			int iTargetDistance = -1;
			if (pMissionPlot != NULL)
			{
				iTargetX = pMissionPlot->getX();
				iTargetY = pMissionPlot->getY();
				iTargetArea = pMissionPlot->getArea().getID();
				iTargetDistance = plotDistance(pLoopUnit->getX(), pLoopUnit->getY(), iTargetX, iTargetY);
				if (eMissionAI == MISSIONAI_FOUND)
					iTargetSiteRank = getSASGameRecordCitySiteRank(kPlayer.AI(), *pMissionPlot, iTargetSiteFoundValue);
			}
			logSASGameRecord("GAME_RECORD_SETTLER turn=%d player=%d unitId=%d unit=%s unitAI=%s groupId=%d x=%d y=%d mission=%s missionAI=%d targetX=%d targetY=%d targetArea=%d targetDistance=%d targetSiteListRank=%d targetCachedFoundValue=%d plotOwner=%d plotTerrain=%s plotFeature=%s plotBonus=%s plotImprovement=%s plotRoute=%s guarded=%d threatened=%d nearestCityId=%d nearestCity=%S nearestCityDistance=%d",
				iGameTurn, ePlayer, pLoopUnit->getID(), getSASGameRecordUnitType(pLoopUnit->getUnitType()),
				getSASGameRecordUnitAIType(pLoopUnit->AI_getUnitAIType()), (pGroup == NULL ? -1 : pGroup->getID()), pLoopUnit->getX(),
				pLoopUnit->getY(), getSASGameRecordMissionType(eMission), eMissionAI, iTargetX, iTargetY, iTargetArea, iTargetDistance,
				iTargetSiteRank, iTargetSiteFoundValue, pPlot->getOwner(), getSASGameRecordTerrainType(pPlot->getTerrainType()),
				getSASGameRecordFeatureType(pPlot->getFeatureType()), getSASGameRecordBonusType(pPlot->getBonusType(pLoopUnit->getTeam())),
				getSASGameRecordImprovementType(pPlot->getImprovementType()), getSASGameRecordRouteType(pPlot->getRouteType()), bGuarded,
				bThreatened, pNearestCity == NULL ? -1 : pNearestCity->getID(), getSASGameRecordQuotedCityName(pNearestCity).GetCString(),
				iNearestDistance);
		}
	}
	logSASGameRecord("GAME_RECORD_SETTLERS turn=%d player=%d settlers=%d foundMission=%d foundIntent=%d moving=%d idle=%d waiting=%d ownTerritory=%d enemyTerritory=%d neutralTerritory=%d guarded=%d unguarded=%d threatened=%d",
		iGameTurn, ePlayer, iSettlers, iFoundMission, iFoundIntent, iMoving, iIdle, iWaiting, iOwnTerritory, iEnemyTerritory,
		iNeutralTerritory, iGuarded, iUnguarded, iThreatened);
	logSASGameRecord("GAME_RECORD_SETTLERS_DELTAS turn=%d player=%d deltaValid=%d settlersDelta=%+d foundMissionDelta=%+d foundIntentDelta=%+d movingDelta=%+d idleDelta=%+d waitingDelta=%+d threatenedDelta=%+d",
		iGameTurn, ePlayer, kPrevious.bValid, getSASGameRecordDelta(kPrevious.bValid, iSettlers, kPrevious.iSettlerSettlers),
		getSASGameRecordDelta(kPrevious.bValid, iFoundMission, kPrevious.iSettlerFoundMission),
		getSASGameRecordDelta(kPrevious.bValid, iFoundIntent, kPrevious.iSettlerFoundIntent),
		getSASGameRecordDelta(kPrevious.bValid, iMoving, kPrevious.iSettlerMoving),
		getSASGameRecordDelta(kPrevious.bValid, iIdle, kPrevious.iSettlerIdle),
		getSASGameRecordDelta(kPrevious.bValid, iWaiting, kPrevious.iSettlerWaiting),
		getSASGameRecordDelta(kPrevious.bValid, iThreatened, kPrevious.iSettlerThreatened));
	kPrevious.iSettlerSettlers = iSettlers;
	kPrevious.iSettlerFoundMission = iFoundMission;
	kPrevious.iSettlerFoundIntent = iFoundIntent;
	kPrevious.iSettlerMoving = iMoving;
	kPrevious.iSettlerIdle = iIdle;
	kPrevious.iSettlerWaiting = iWaiting;
	kPrevious.iSettlerThreatened = iThreatened;
}

static CvString getSASGameRecordCitySpecialists(CvCity const& kCity, bool bFree)
{
	CvString szList;
	FOR_EACH_ENUM(Specialist)
	{
		const int iCount = (bFree ? kCity.getFreeSpecialistCount(eLoopSpecialist) : kCity.getSpecialistCount(eLoopSpecialist));
		appendSASGameRecordTypeCount(szList, getSASGameRecordSpecialistType(eLoopSpecialist), iCount);
	}
	return getSASDiagnosticOrDash(szList);
}

static CvString getSASGameRecordCityGPOdds(CvCity const& kCity)
{
	CvString szList;
	std::vector<std::pair<UnitTypes,int> > aeiProjection;
	kCity.GPProjection(aeiProjection);
	for (size_t iI = 0; iI < aeiProjection.size(); iI++)
		appendSASGameRecordTypeCount(szList, getSASGameRecordUnitType(aeiProjection[iI].first), aeiProjection[iI].second);
	return getSASDiagnosticOrDash(szList);
}

static CvString getSASGameRecordCityHappySources(CvCity const& kCity)
{
	CvString szList;
	CvPlayer const& kOwner = GET_PLAYER(kCity.getOwner());
	appendSASGameRecordPositiveValue(szList, "largestCity", std::max(0, kCity.getLargestCityHappiness()));
	appendSASGameRecordPositiveValue(szList, "military", std::max(0, kCity.getMilitaryHappiness()));
	appendSASGameRecordPositiveValue(szList, "stateReligion", std::max(0, kCity.getCurrentStateReligionHappiness()));
	appendSASGameRecordPositiveValue(szList, "building", std::max(0, kCity.getBuildingGoodHappiness()));
	appendSASGameRecordPositiveValue(szList, "extraBuilding", std::max(0, kCity.getExtraBuildingGoodHappiness()));
	appendSASGameRecordPositiveValue(szList, "surrounding", std::max(0, kCity.getSurroundingGoodHappiness()));
	appendSASGameRecordPositiveValue(szList, "bonus", std::max(0, kCity.getBonusGoodHappiness()));
	appendSASGameRecordPositiveValue(szList, "religion", std::max(0, kCity.getReligionGoodHappiness()));
	appendSASGameRecordPositiveValue(szList, "commerce", std::max(0, kCity.getCommerceHappiness()));
	appendSASGameRecordPositiveValue(szList, "areaBuilding", std::max(0, kCity.getArea().getBuildingHappiness(kCity.getOwner())));
	appendSASGameRecordPositiveValue(szList, "playerBuilding", std::max(0, kOwner.getBuildingHappiness()));
	appendSASGameRecordPositiveValue(szList, "extra", std::max(0, kCity.getExtraHappiness() + kOwner.getExtraHappiness()));
	appendSASGameRecordPositiveValue(szList, "handicap", std::max(0, GC.getInfo(kCity.getHandicapType()).getHappyBonus()));
	appendSASGameRecordPositiveValue(szList, "vassal", std::max(0, kCity.getVassalHappiness()));
	appendSASGameRecordPositiveValue(szList, "temporary", kCity.getHappinessTimer() > 0 ? GC.getDefineINT("TEMP_HAPPY") : 0);
	return getSASDiagnosticOrDash(szList);
}

static CvString getSASGameRecordCityFlatUnhappySources(CvCity const& kCity)
{
	CvString szList;
	CvPlayer const& kOwner = GET_PLAYER(kCity.getOwner());
	appendSASGameRecordPositiveValue(szList, "largestCity", -std::min(0, kCity.getLargestCityHappiness()));
	appendSASGameRecordPositiveValue(szList, "military", -std::min(0, kCity.getMilitaryHappiness()));
	appendSASGameRecordPositiveValue(szList, "stateReligion", -std::min(0, kCity.getCurrentStateReligionHappiness()));
	appendSASGameRecordPositiveValue(szList, "building", -std::min(0, kCity.getBuildingBadHappiness()));
	appendSASGameRecordPositiveValue(szList, "extraBuilding", -std::min(0, kCity.getExtraBuildingBadHappiness()));
	appendSASGameRecordPositiveValue(szList, "surrounding", -std::min(0, kCity.getSurroundingBadHappiness()));
	appendSASGameRecordPositiveValue(szList, "bonus", -std::min(0, kCity.getBonusBadHappiness()));
	appendSASGameRecordPositiveValue(szList, "religion", -std::min(0, kCity.getReligionBadHappiness()));
	appendSASGameRecordPositiveValue(szList, "commerce", -std::min(0, kCity.getCommerceHappiness()));
	appendSASGameRecordPositiveValue(szList, "areaBuilding", -std::min(0, kCity.getArea().getBuildingHappiness(kCity.getOwner())));
	appendSASGameRecordPositiveValue(szList, "playerBuilding", -std::min(0, kOwner.getBuildingHappiness()));
	appendSASGameRecordPositiveValue(szList, "extra", -std::min(0, kCity.getExtraHappiness() + kOwner.getExtraHappiness()));
	appendSASGameRecordPositiveValue(szList, "handicap", -std::min(0, GC.getInfo(kCity.getHandicapType()).getHappyBonus()));
	appendSASGameRecordPositiveValue(szList, "vassal", std::max(0, kCity.getVassalUnhappiness()));
	appendSASGameRecordPositiveValue(szList, "espionage", std::max(0, kCity.getEspionageHappinessCounter()));
	return getSASDiagnosticOrDash(szList);
}

static CvString getSASGameRecordCityAngerPercentSources(CvCity const& kCity)
{
	CvString szList;
	CvPlayer const& kOwner = GET_PLAYER(kCity.getOwner());
	int iCivicAnger = 0;
	FOR_EACH_ENUM(Civic)
		iCivicAnger += kOwner.getCivicPercentAnger(eLoopCivic);
	appendSASGameRecordPositiveValue(szList, "overcrowding", kCity.getOvercrowdingPercentAnger());
	appendSASGameRecordPositiveValue(szList, "noMilitary", kCity.getNoMilitaryPercentAnger());
	appendSASGameRecordPositiveValue(szList, "culture", kCity.getCulturePercentAnger());
	appendSASGameRecordPositiveValue(szList, "religion", kCity.getReligionPercentAnger());
	appendSASGameRecordPositiveValue(szList, "hurry", kCity.getHurryPercentAnger());
	appendSASGameRecordPositiveValue(szList, "conscript", kCity.getConscriptPercentAnger());
	appendSASGameRecordPositiveValue(szList, "defyResolution", kCity.getDefyResolutionPercentAnger());
	appendSASGameRecordPositiveValue(szList, "warWeariness", kCity.getWarWearinessPercentAnger());
	appendSASGameRecordPositiveValue(szList, "globalWarming", std::max(0, kOwner.getGwPercentAnger() * 10));
	appendSASGameRecordPositiveValue(szList, "civics", iCivicAnger);
	return getSASDiagnosticOrDash(szList);
}

static CvString getSASGameRecordCityHealthySources(CvCity const& kCity)
{
	CvString szList;
	CvPlayer const& kOwner = GET_PLAYER(kCity.getOwner());
	appendSASGameRecordPositiveValue(szList, "freshWater", std::max(0, kCity.getFreshWaterGoodHealth()));
	appendSASGameRecordPositiveValue(szList, "surrounding", std::max(0, kCity.getSurroundingGoodHealth()));
	appendSASGameRecordPositiveValue(szList, "power", std::max(0, kCity.getPowerGoodHealth()));
	appendSASGameRecordPositiveValue(szList, "bonus", std::max(0, kCity.getBonusGoodHealth()));
	appendSASGameRecordPositiveValue(szList, "building", std::max(0, kCity.totalGoodBuildingHealth()));
	appendSASGameRecordPositiveValue(szList, "extra", std::max(0, kCity.getExtraHealth() + kOwner.getExtraHealth()));
	appendSASGameRecordPositiveValue(szList, "handicap", std::max(0, GC.getInfo(kCity.getHandicapType()).getHealthBonus()));
	return getSASDiagnosticOrDash(szList);
}

static CvString getSASGameRecordCityUnhealthySources(CvCity const& kCity)
{
	CvString szList;
	CvPlayer const& kOwner = GET_PLAYER(kCity.getOwner());
	appendSASGameRecordPositiveValue(szList, "population", kCity.unhealthyPopulation());
	appendSASGameRecordPositiveValue(szList, "espionage", std::max(0, kCity.getEspionageHealthCounter()));
	appendSASGameRecordPositiveValue(szList, "freshWater", -std::min(0, kCity.getFreshWaterBadHealth()));
	appendSASGameRecordPositiveValue(szList, "surrounding", -std::min(0, kCity.getSurroundingBadHealth()));
	appendSASGameRecordPositiveValue(szList, "power", -std::min(0, kCity.getPowerBadHealth()));
	appendSASGameRecordPositiveValue(szList, "bonus", -std::min(0, kCity.getBonusBadHealth()));
	appendSASGameRecordPositiveValue(szList, "building", -std::min(0, kCity.totalBadBuildingHealth()));
	appendSASGameRecordPositiveValue(szList, "extra", -std::min(0, kCity.getExtraHealth() + kOwner.getExtraHealth()));
	appendSASGameRecordPositiveValue(szList, "handicap", -std::min(0, GC.getInfo(kCity.getHandicapType()).getHealthBonus()));
	return getSASDiagnosticOrDash(szList);
}

static const char* getSASGameRecordCityProductionKind(CvCity const& kCity)
{
	if (kCity.getProductionUnit() != NO_UNIT)
		return "UNIT";
	if (kCity.getProductionBuilding() != NO_BUILDING)
		return GC.getInfo(kCity.getProductionBuilding()).isLimited() ? "WONDER" : "BUILDING";
	if (kCity.getProductionProject() != NO_PROJECT)
		return "PROJECT";
	if (kCity.getProductionProcess() != NO_PROCESS)
		return "PROCESS";
	return "-";
}

static const char* getSASGameRecordCityProductionType(CvCity const& kCity)
{
	if (kCity.getProductionUnit() != NO_UNIT)
		return getSASGameRecordUnitType(kCity.getProductionUnit());
	if (kCity.getProductionBuilding() != NO_BUILDING)
		return getSASGameRecordBuildingType(kCity.getProductionBuilding());
	if (kCity.getProductionProject() != NO_PROJECT)
		return getSASGameRecordProjectType(kCity.getProductionProject());
	if (kCity.getProductionProcess() != NO_PROCESS)
		return getSASGameRecordProcessType(kCity.getProductionProcess());
	return "-";
}

// <!-- custom: Production churn is about the active head target, not every queued order mutation.
// Keep a compact recorder-only representation that survives AI_chooseProduction clearing/rebuilding its queue and distinguishes Wonders from ordinary buildings. (ChatGPT-5.6-Sol) -->
static int getSASGameRecordProductionKindIndex(OrderTypes eOrder, int iData1)
{
	switch (eOrder)
	{
	case ORDER_TRAIN: return SAS_PRODUCTION_UNIT;
	case ORDER_CONSTRUCT:
		return (iData1 >= 0 && iData1 < GC.getNumBuildingInfos() && GC.getInfo((BuildingTypes)iData1).isLimited() ? SAS_PRODUCTION_WONDER : SAS_PRODUCTION_BUILDING);
	case ORDER_CREATE: return SAS_PRODUCTION_PROJECT;
	case ORDER_MAINTAIN: return SAS_PRODUCTION_PROCESS;
	default: return -1;
	}
}

static const char* getSASGameRecordProductionKind(OrderTypes eOrder, int iData1)
{
	static char const* const aszKinds[NUM_SAS_PRODUCTION_KINDS] = {"UNIT", "BUILDING", "WONDER", "PROJECT", "PROCESS"};
	int const iKind = getSASGameRecordProductionKindIndex(eOrder, iData1);
	return (iKind < 0 ? "-" : aszKinds[iKind]);
}

static const char* getSASGameRecordProductionType(OrderTypes eOrder, int iData1)
{
	switch (eOrder)
	{
	case ORDER_TRAIN: return (iData1 >= 0 && iData1 < GC.getNumUnitInfos() ? getSASGameRecordUnitType((UnitTypes)iData1) : "-");
	case ORDER_CONSTRUCT: return (iData1 >= 0 && iData1 < GC.getNumBuildingInfos() ? getSASGameRecordBuildingType((BuildingTypes)iData1) : "-");
	case ORDER_CREATE: return (iData1 >= 0 && iData1 < GC.getNumProjectInfos() ? getSASGameRecordProjectType((ProjectTypes)iData1) : "-");
	case ORDER_MAINTAIN: return (iData1 >= 0 && iData1 < GC.getNumProcessInfos() ? getSASGameRecordProcessType((ProcessTypes)iData1) : "-");
	default: return "-";
	}
}

static void captureSASGameRecordProductionTarget(CvCity const& kCity, OrderTypes& eOrder, int& iData1, int& iStored, int& iNeeded, int& iTurnsLeft, int& iAccumulatedInactiveTurns)
{
	OrderData const kOrder = kCity.getOrderData(0);
	eOrder = kOrder.eOrderType;
	iData1 = kOrder.iData1;
	iStored = 0;
	iNeeded = 0;
	iTurnsLeft = -1;
	iAccumulatedInactiveTurns = -1;
	switch (eOrder)
	{
	case ORDER_TRAIN:
		iStored = kCity.getUnitProduction((UnitTypes)iData1);
		iNeeded = kCity.getProductionNeeded((UnitTypes)iData1);
		iTurnsLeft = kCity.getProductionTurnsLeft();
		if (iTurnsLeft == MAX_INT)
			iTurnsLeft = -1;
		iAccumulatedInactiveTurns = kCity.getUnitProductionTime((UnitTypes)iData1);
		break;
	case ORDER_CONSTRUCT:
		iStored = kCity.getBuildingProduction((BuildingTypes)iData1);
		iNeeded = kCity.getProductionNeeded((BuildingTypes)iData1);
		iTurnsLeft = kCity.getProductionTurnsLeft();
		if (iTurnsLeft == MAX_INT)
			iTurnsLeft = -1;
		iAccumulatedInactiveTurns = kCity.getBuildingProductionTime((BuildingTypes)iData1);
		break;
	case ORDER_CREATE:
		iStored = kCity.getProjectProduction((ProjectTypes)iData1);
		iNeeded = kCity.getProductionNeeded((ProjectTypes)iData1);
		iTurnsLeft = kCity.getProductionTurnsLeft();
		if (iTurnsLeft == MAX_INT)
			iTurnsLeft = -1;
		break;
	case ORDER_MAINTAIN:
		break;
	default:
		eOrder = NO_ORDER;
		iData1 = -1;
		break;
	}
}

static void noteSASGameRecordAIProductionTargetChangedCity(SASGameRecordPlayerFlow& kFlow, int iCityId)
{
	for (size_t iI = 0; iI < kFlow.aAIProductionTargetChangesByCity.size(); iI++)
	{
		if (kFlow.aAIProductionTargetChangesByCity[iI].first == iCityId)
		{
			kFlow.aAIProductionTargetChangesByCity[iI].second++;
			return;
		}
	}
	kFlow.aAIProductionTargetChangesByCity.push_back(std::make_pair(iCityId, 1));
}

// <!-- custom: SASGameRecord reports the broad all-city outcome at the control-path-aware turn boundary, while dedicated BBAI diagnostics explain exact chooser paths and legal-target context.
// Manual human cities are sampled before end-turn city processing so a normal newly completed item awaiting its popup is not misclassified.
// AI-controlled and production-automated cities are sampled afterward so their chooser and emergency-building rules get their opportunity first. The sole caller prevalidates the log level, city state and eligible civilization player. See KI#51. (GPT-5.6-Sol) -->
void logSASGameRecordCityProductionNoTarget(CvCity const& kCity, char const* szPhase)
{
	PlayerTypes const ePlayer = kCity.getOwner();
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=CITY_PRODUCTION_NO_TARGET player=%d cityId=%d city=%S phase=%s human=%d humanDisabled=%d productionAutomated=%d chooseProductionDirty=%d gameState=%d population=%d rawProduction=%d overflowProduction=%d anarchyTurns=%d occupation=%d occupationTimer=%d",
		GC.getGame().getGameTurn(), ePlayer, kCity.getID(), getSASGameRecordQuotedCityName(&kCity).GetCString(), szPhase,
		kPlayer.isHuman(), kPlayer.isHumanDisabled(), kCity.isProductionAutomated(), kCity.isChooseProductionDirty(), (int)GC.getGame().getGameState(), kCity.getPopulation(),
		kCity.getCurrentProductionDifference(false, false, true), kCity.getOverflowProduction(), kPlayer.getAnarchyTurns(), kCity.isOccupation(), kCity.getOccupationTimer());
}

// <!-- custom: Cold enabled path for the header-inline RAII wrapper.
// Keeping capture/finalization out of the header avoids expanding every CvCityAI includer while the disabled level-0/1 path remains tiny. (ChatGPT-5.6-Sol) -->
void SASGameRecordAIProductionChoiceScope::begin(CvCity const& kCity)
{
	m_pCity = &kCity;
	captureSASGameRecordProductionTarget(kCity, m_eOldOrder, m_iOldData1, m_iOldStored, m_iOldNeeded, m_iOldTurnsLeft, m_iOldAccumulatedInactiveTurns);
}

// <!-- custom: Called only when begin() armed m_pCity; the inline destructor retains the null guard, so this function can assume a valid enabled scope and perform the level-2+ final comparison directly. (ChatGPT-5.6-Sol) -->
void SASGameRecordAIProductionChoiceScope::end()
{
	OrderTypes eNewOrder = NO_ORDER;
	int iNewData1 = -1;
	int iNewStored = 0;
	int iNewNeeded = 0;
	int iNewTurnsLeft = -1;
	int iNewAccumulatedInactiveTurns = -1;
	captureSASGameRecordProductionTarget(*m_pCity, eNewOrder, iNewData1, iNewStored, iNewNeeded, iNewTurnsLeft, iNewAccumulatedInactiveTurns);
	bool const bSameTarget = (m_eOldOrder == eNewOrder && m_iOldData1 == iNewData1);
	if (bSameTarget)
		return;
	bool const bOldTarget = (m_eOldOrder != NO_ORDER);
	bool const bNewTarget = (eNewOrder != NO_ORDER);
	bool const bResume = (bNewTarget && iNewStored > 0);
	// <!-- custom: Normal completion -> fresh next target is not churn. Preserve only a real active-target change or selection that resumes previously stored production. (ChatGPT-5.6-Sol) -->
	if (!bOldTarget && !bResume)
		return;
	PlayerTypes const ePlayer = m_pCity->getOwner();
	if (ePlayer < 0 || ePlayer >= MAX_CIV_PLAYERS)
		return;
	SASGameRecordPlayerFlow& kFlow = g_akSASGameRecordPlayerFlow[ePlayer];
	if (bOldTarget)
	{
		if (bNewTarget)
			kFlow.iAIProductionTargetSwitches++;
		else kFlow.iAIProductionTargetClears++;
		if (m_iOldStored > 0)
		{
			kFlow.iAIProductionInvestedTargetChanges++;
			kFlow.iAIProductionParked += m_iOldStored;
		}
		noteSASGameRecordAIProductionTargetChangedCity(kFlow, m_pCity->getID());
		int const iOldKind = getSASGameRecordProductionKindIndex(m_eOldOrder, m_iOldData1);
		int const iNewKind = getSASGameRecordProductionKindIndex(eNewOrder, iNewData1);
		if (iOldKind >= 0 && iNewKind >= 0)
			kFlow.aiAIProductionTransitions[iOldKind * NUM_SAS_PRODUCTION_KINDS + iNewKind]++;
	}
	if (bResume)
	{
		kFlow.iAIProductionTargetResumes++;
		kFlow.iAIProductionResumed += iNewStored;
	}
	if (gGameRecordLogLevel >= 3)
	{
		char const* szChange = (bOldTarget ? (bNewTarget ? "SWITCH" : "CLEAR") : "RESUME");
		logSASGameRecord("GAME_RECORD_ACTION turn=%d type=AI_PRODUCTION_TARGET_CHANGED player=%d cityId=%d city=%S change=%s oldKind=%s oldTarget=%s oldStored=%d oldNeeded=%d oldTurnsLeft=%d oldAccumulatedInactiveTurns=%d newKind=%s newTarget=%s newStored=%d newNeeded=%d newTurnsLeft=%d newAccumulatedInactiveTurns=%d oldStoredParked=%d newStoredResumed=%d",
			GC.getGame().getGameTurn(), ePlayer, m_pCity->getID(), getSASGameRecordQuotedCityName(m_pCity).GetCString(), szChange,
			getSASGameRecordProductionKind(m_eOldOrder, m_iOldData1), getSASGameRecordProductionType(m_eOldOrder, m_iOldData1), m_iOldStored,
			m_iOldNeeded, m_iOldTurnsLeft, m_iOldAccumulatedInactiveTurns, getSASGameRecordProductionKind(eNewOrder, iNewData1),
			getSASGameRecordProductionType(eNewOrder, iNewData1), iNewStored, iNewNeeded, iNewTurnsLeft, iNewAccumulatedInactiveTurns,
			m_iOldStored > 0 ? m_iOldStored : 0, bResume ? iNewStored : 0);
	}
}

// <!-- custom: Production-pipeline aggregation is declared here because the helper that normalizes unavailable current-production cost is defined a little later with the other city-output helpers. (ChatGPT-5.6-Sol) -->
static int getSASGameRecordCityProductionNeeded(CvCity const& kCity);

// <!-- custom: Current city rows show only the active target, so repeated AI switches can leave a strategically important bank of partial production invisible.
// At periodic level-2 snapshots, enumerate stored non-current unit/building/project production once per city and summarize fragmentation; level 3 adds the exact parked inventory, where each unit/building @ value is the engine's accumulated inactive-turn counter (it pauses rather than resets when production resumes).
// Cheap isAnyProductionProgress guards skip each loaded-XML scan when that city has no stored production of the corresponding kind.
// Food-produced units such as Settlers/Workers use the same per-unit production bank, so split them as a useful subset rather than invent a separate parked-food quantity.
// AI unit/building production is not labeled "lost" because inherited K-Mod/AdvCiv CvCity::doDecay only reduces it for human cities. (ChatGPT-5.6-Sol) -->
static void logSASGameRecordProductionPipeline(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	bool const bLogParkedDetails = (gGameRecordLogLevel >= 3);
	int iActiveFiniteItems = 0;
	int iActiveStored = 0;
	int iActiveNeeded = 0;
	int iActiveProcesses = 0;
	int iActiveFoodProductionUnits = 0;
	int iActiveFoodProductionUnitStored = 0;
	int iParkedItems = 0;
	int iParkedStored = 0;
	int iParkedNeeded = 0;
	int iParkedUnitItems = 0;
	int iParkedUnitStored = 0;
	int iParkedFoodProductionUnitItems = 0;
	int iParkedFoodProductionUnitStored = 0;
	int iParkedBuildingItems = 0;
	int iParkedBuildingStored = 0;
	int iParkedWonderItems = 0;
	int iParkedWonderStored = 0;
	int iParkedProjectItems = 0;
	int iParkedProjectStored = 0;
	int iCitiesWithParked = 0;
	int iMaxParkedItemsOneCity = 0;
	int iMaxParkedStoredOneCity = 0;
	int iParkedHalfComplete = 0;
	int iParkedThreeQuarterComplete = 0;
	int iInactivityCounterItems = 0;
	int iAccumulatedInactiveTurnsTotal = 0;
	int iMaxAccumulatedInactiveTurns = 0;
	CvString szParked;
	int iCityLoop = 0;
	for (CvCity const* pCity = kPlayer.firstCity(&iCityLoop); pCity != NULL; pCity = kPlayer.nextCity(&iCityLoop))
	{
		UnitTypes const eCurrentUnit = pCity->getProductionUnit();
		BuildingTypes const eCurrentBuilding = pCity->getProductionBuilding();
		ProjectTypes const eCurrentProject = pCity->getProductionProject();
		ProcessTypes const eCurrentProcess = pCity->getProductionProcess();
		if (eCurrentUnit != NO_UNIT || eCurrentBuilding != NO_BUILDING || eCurrentProject != NO_PROJECT)
		{
			iActiveFiniteItems++;
			iActiveStored += pCity->getProduction();
			iActiveNeeded += getSASGameRecordCityProductionNeeded(*pCity);
			if (eCurrentUnit != NO_UNIT && pCity->isFoodProduction(eCurrentUnit))
			{
				iActiveFoodProductionUnits++;
				iActiveFoodProductionUnitStored += pCity->getUnitProduction(eCurrentUnit);
			}
		}
		else if (eCurrentProcess != NO_PROCESS) iActiveProcesses++;
		int iCityParkedItems = 0;
		int iCityParkedStored = 0;
		if (pCity->isAnyProductionProgress(ORDER_TRAIN))
		{
			FOR_EACH_ENUM(Unit)
			{
				int const iStored = pCity->getUnitProduction(eLoopUnit);
				if (iStored <= 0 || eLoopUnit == eCurrentUnit)
					continue;
				int const iNeeded = pCity->getProductionNeeded(eLoopUnit);
				int const iInactiveTurns = pCity->getUnitProductionTime(eLoopUnit);
				iParkedItems++; iParkedStored += iStored; iParkedNeeded += iNeeded;
				iParkedUnitItems++; iParkedUnitStored += iStored;
				if (pCity->isFoodProduction(eLoopUnit))
				{
					iParkedFoodProductionUnitItems++;
					iParkedFoodProductionUnitStored += iStored;
				}
				iCityParkedItems++; iCityParkedStored += iStored;
				if (iNeeded > 0 && 2 * iStored >= iNeeded) iParkedHalfComplete++;
				if (iNeeded > 0 && 4 * iStored >= 3 * iNeeded) iParkedThreeQuarterComplete++;
				iInactivityCounterItems++; iAccumulatedInactiveTurnsTotal += iInactiveTurns; iMaxAccumulatedInactiveTurns = std::max(iMaxAccumulatedInactiveTurns, iInactiveTurns);
				if (bLogParkedDetails)
				{
					CvString szItem;
					szItem.Format(szParked.empty() ? "%d:UNIT:%s:%d/%d@%d" : ",%d:UNIT:%s:%d/%d@%d", pCity->getID(), getSASGameRecordUnitType(eLoopUnit), iStored, iNeeded, iInactiveTurns);
					szParked += szItem;
				}
			}
		}
		if (pCity->isAnyProductionProgress(ORDER_CONSTRUCT))
		{
			FOR_EACH_ENUM(Building)
			{
				int const iStored = pCity->getBuildingProduction(eLoopBuilding);
				if (iStored <= 0 || eLoopBuilding == eCurrentBuilding)
					continue;
				int const iNeeded = pCity->getProductionNeeded(eLoopBuilding);
				int const iInactiveTurns = pCity->getBuildingProductionTime(eLoopBuilding);
				bool const bWonder = GC.getInfo(eLoopBuilding).isLimited();
				iParkedItems++; iParkedStored += iStored; iParkedNeeded += iNeeded;
				if (bWonder) { iParkedWonderItems++; iParkedWonderStored += iStored; }
				else { iParkedBuildingItems++; iParkedBuildingStored += iStored; }
				iCityParkedItems++; iCityParkedStored += iStored;
				if (iNeeded > 0 && 2 * iStored >= iNeeded) iParkedHalfComplete++;
				if (iNeeded > 0 && 4 * iStored >= 3 * iNeeded) iParkedThreeQuarterComplete++;
				iInactivityCounterItems++; iAccumulatedInactiveTurnsTotal += iInactiveTurns; iMaxAccumulatedInactiveTurns = std::max(iMaxAccumulatedInactiveTurns, iInactiveTurns);
				if (bLogParkedDetails)
				{
					CvString szItem;
					szItem.Format(szParked.empty() ? "%d:%s:%s:%d/%d@%d" : ",%d:%s:%s:%d/%d@%d", pCity->getID(), bWonder ? "WONDER" : "BUILDING", getSASGameRecordBuildingType(eLoopBuilding), iStored, iNeeded, iInactiveTurns);
					szParked += szItem;
				}
			}
		}
		if (pCity->isAnyProductionProgress(ORDER_CREATE))
		{
			FOR_EACH_ENUM(Project)
			{
				int const iStored = pCity->getProjectProduction(eLoopProject);
				if (iStored <= 0 || eLoopProject == eCurrentProject)
					continue;
				int const iNeeded = pCity->getProductionNeeded(eLoopProject);
				iParkedItems++; iParkedStored += iStored; iParkedNeeded += iNeeded;
				iParkedProjectItems++; iParkedProjectStored += iStored;
				iCityParkedItems++; iCityParkedStored += iStored;
				if (iNeeded > 0 && 2 * iStored >= iNeeded) iParkedHalfComplete++;
				if (iNeeded > 0 && 4 * iStored >= 3 * iNeeded) iParkedThreeQuarterComplete++;
				if (bLogParkedDetails)
				{
					CvString szItem;
					szItem.Format(szParked.empty() ? "%d:PROJECT:%s:%d/%d@-" : ",%d:PROJECT:%s:%d/%d@-", pCity->getID(), getSASGameRecordProjectType(eLoopProject), iStored, iNeeded);
					szParked += szItem;
				}
			}
		}
		if (iCityParkedItems > 0)
		{
			iCitiesWithParked++;
			iMaxParkedItemsOneCity = std::max(iMaxParkedItemsOneCity, iCityParkedItems);
			iMaxParkedStoredOneCity = std::max(iMaxParkedStoredOneCity, iCityParkedStored);
		}
	}
	logSASGameRecord("GAME_RECORD_PRODUCTION_PIPELINE turn=%d player=%d activeFiniteItems=%d activeStored=%d activeNeeded=%d activeProcesses=%d activeFoodProductionUnits=%d activeFoodProductionUnitStored=%d parkedItems=%d parkedStored=%d parkedNeeded=%d citiesWithParked=%d maxParkedItemsOneCity=%d maxParkedStoredOneCity=%d parkedHalfComplete=%d parkedThreeQuarterComplete=%d parkedUnitItems=%d parkedUnitStored=%d parkedFoodProductionUnitItems=%d parkedFoodProductionUnitStored=%d parkedBuildingItems=%d parkedBuildingStored=%d parkedWonderItems=%d parkedWonderStored=%d parkedProjectItems=%d parkedProjectStored=%d inactivityCounterItems=%d accumulatedInactiveTurnsTotal=%d maxAccumulatedInactiveTurns=%d",
		iGameTurn, ePlayer, iActiveFiniteItems, iActiveStored, iActiveNeeded, iActiveProcesses, iActiveFoodProductionUnits,
		iActiveFoodProductionUnitStored, iParkedItems, iParkedStored, iParkedNeeded, iCitiesWithParked, iMaxParkedItemsOneCity,
		iMaxParkedStoredOneCity, iParkedHalfComplete, iParkedThreeQuarterComplete, iParkedUnitItems, iParkedUnitStored,
		iParkedFoodProductionUnitItems, iParkedFoodProductionUnitStored, iParkedBuildingItems, iParkedBuildingStored, iParkedWonderItems,
		iParkedWonderStored, iParkedProjectItems, iParkedProjectStored, iInactivityCounterItems, iAccumulatedInactiveTurnsTotal,
		iMaxAccumulatedInactiveTurns);
	if (bLogParkedDetails && !szParked.empty())
		logSASGameRecord("GAME_RECORD_PRODUCTION_PARKED turn=%d player=%d items=%s", iGameTurn, ePlayer, szParked.GetCString());
}

// <!-- custom: Building-completion actions alone cannot reconstruct buildings inherited through conquest, granted for free, or already present when a log begins.
// At detail level, snapshot the exact owned buildings and compact regular/national/team/world-wonder totals for each city. (GPT-5.6-Sol) -->
static CvString getSASGameRecordCityBuildings(CvCity const& kCity, int& iTotal, int& iRegular, int& iNationalWonders, int& iTeamWonders, int& iWorldWonders)
{
	CvString szBuildings;
	iTotal = iRegular = iNationalWonders = iTeamWonders = iWorldWonders = 0;
	for (int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		BuildingTypes const eBuilding = (BuildingTypes)iI;
		int const iCount = kCity.getNumBuilding(eBuilding);
		if (iCount <= 0)
			continue;
		iTotal += iCount;
		CvBuildingInfo const& kBuilding = GC.getInfo(eBuilding);
		if (kBuilding.isWorldWonder())
			iWorldWonders += iCount;
		else if (kBuilding.isTeamWonder())
			iTeamWonders += iCount;
		else if (kBuilding.isNationalWonder())
			iNationalWonders += iCount;
		else iRegular += iCount;
		CvString szItem;
		szItem.Format(szBuildings.empty() ? "%s:%d" : ",%s:%d", getSASGameRecordBuildingType(eBuilding), iCount);
		szBuildings += szItem;
	}
	return getSASDiagnosticOrDash(szBuildings);
}

// <!-- custom: A PROCESS production name identifies Wealth/Research/Culture but not its actual gain.
// Record the exact production-to-commerce contribution in hundredths, matching CvCity::updateCommerce without rounding away fractional output. (GPT-5.6-Sol) -->
static CvString getSASGameRecordCityProductionConversion(CvCity const& kCity)
{
	CvString szConversion;
	// <!-- custom: CvCity::updateCommerce suppresses both ordinary commerce and production-to-commerce conversion during disorder.
	// Preserve the selected process elsewhere on the city row, but do not report output the city is not receiving. See KI#381. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (kCity.getProductionProcess() == NO_PROCESS || kCity.isDisorder())
		return "-";
	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		CommerceTypes const eCommerce = (CommerceTypes)iI;
		int const iRateX100 = kCity.getYieldRate(YIELD_PRODUCTION) * kCity.getProductionToCommerceModifier(eCommerce);
		if (iRateX100 > 0)
			appendSASGameRecordValue(szConversion, getSASGameRecordCommerceType(eCommerce), iRateX100);
	}
	return getSASDiagnosticOrDash(szConversion);
}

// <!-- custom: CvCity uses MAX_INT when no finite production amount or ETA exists, including processes, empty queues and zero production during disorder.
// Emit the GameRecord's ordinary unavailable-value sentinel instead of presenting 2147483647 as a real statistic. See KI#380. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
static int getSASGameRecordCityProductionTurns(CvCity const& kCity)
{
	int const iTurns = kCity.getProductionTurnsLeft();
	return iTurns == MAX_INT ? -1 : iTurns;
}

// <!-- custom: Apply the same unavailable-value contract to production cost because processes and empty queues have no finite amount needed. See KI#380. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
static int getSASGameRecordCityProductionNeeded(CvCity const& kCity)
{
	int const iNeeded = kCity.getProductionNeeded();
	return iNeeded == MAX_INT ? -1 : iNeeded;
}

static CvString getSASGameRecordCityTradePartners(CvCity const& kCity)
{
	CvString szList;
	for (int iI = 0; iI < kCity.getTradeRoutes(); iI++)
	{
		CvCity const* pTradeCity = kCity.getTradeCity(iI);
		if (pTradeCity == NULL)
			continue;
		CvString szItem;
		szItem.Format(szList.empty() ? "%d:%d:%S" : ",%d:%d:%S", pTradeCity->getOwner(), pTradeCity->getID(), pTradeCity->getName().GetCString());
		szList += szItem;
	}
	return szList.empty() ? "-" : getSASDiagnosticQuoted(szList.GetCString());
}

// <!-- custom: Settler unit-state helpers for event-based expansion diagnostics.
// Keep game-record rows descriptive: raw unit counts, visible enemy counts and combat/founding context, while BBAI logs carry the heavier AI-decision reasons; no gameplay behavior change. (ChatGPT-5.5) -->
struct SASGameRecordPlotUnitCounts
{
	int iUnits;
	int iMilitaryUnits;
	int iCivilianUnits;
	int iDefenders;
	int iHealthyDefenders;
	int iWoundedDefenders;
	int iSettlers;
	int iWorkers;
	int iAttackers;
	CvUnit const* pBestDefender;
	CvUnit const* pFirstSettler;
	SASGameRecordPlotUnitCounts() : iUnits(0), iMilitaryUnits(0), iCivilianUnits(0), iDefenders(0), iHealthyDefenders(0), iWoundedDefenders(0), iSettlers(0), iWorkers(0), iAttackers(0), pBestDefender(NULL), pFirstSettler(NULL) {}
};

static void collectSASGameRecordPlotUnitCounts(CvPlot const& kPlot, PlayerTypes ePlayer, SASGameRecordPlotUnitCounts& kCounts)
{
	for (CLLNode<IDInfo> const* pUnitNode = kPlot.headUnitNode(); pUnitNode != NULL; pUnitNode = kPlot.nextUnitNode(pUnitNode))
	{
		CvUnit const* pLoopUnit = ::getUnit(pUnitNode->m_data);
		if (pLoopUnit == NULL || pLoopUnit->getOwner() != ePlayer)
			continue;
		kCounts.iUnits++;
		if (pLoopUnit->baseCombatStr() > 0 || pLoopUnit->canAttack() || pLoopUnit->canDefend(&kPlot))
			kCounts.iMilitaryUnits++;
		else kCounts.iCivilianUnits++;
		if (isSASGameRecordSettlerUnit(*pLoopUnit))
		{
			kCounts.iSettlers++;
			if (kCounts.pFirstSettler == NULL)
				kCounts.pFirstSettler = pLoopUnit;
		}
		if (isSASGameRecordWorkerUnit(*pLoopUnit))
			kCounts.iWorkers++;
		if (pLoopUnit->canAttack())
			kCounts.iAttackers++;
		if (pLoopUnit->canDefend(&kPlot))
		{
			kCounts.iDefenders++;
			if (pLoopUnit->getDamage() <= 25)
				kCounts.iHealthyDefenders++;
			else kCounts.iWoundedDefenders++;
			if (kCounts.pBestDefender == NULL || pLoopUnit->baseCombatStr() > kCounts.pBestDefender->baseCombatStr() || (pLoopUnit->baseCombatStr() == kCounts.pBestDefender->baseCombatStr() && pLoopUnit->getDamage() < kCounts.pBestDefender->getDamage()))
				kCounts.pBestDefender = pLoopUnit;
		}
	}
}


static void logSASGameRecordWorkedPlots(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	SASGameRecordPlotComposition kComposition;
	int iLoop = 0;
	for (CvCity const* pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
		addSASGameRecordPlotComposition(kComposition, getSASGameRecordWorkedPlotComposition(*pLoopCity));
	CvString szTerrains;
	CvString szFeatures;
	CvString szBonuses;
	CvString szImprovements;
	CvString szRoutes;
	getSASGameRecordPlotCompositionTypes(kComposition, szTerrains, szFeatures, szBonuses, szImprovements, szRoutes);
	logSASGameRecord("GAME_RECORD_WORKED_PLOTS turn=%d player=%d cities=%d worked=%d improved=%d unimproved=%d land=%d water=%d hills=%d riverSide=%d freshWater=%d bonusImproved=%d bonusUnimproved=%d currentFood=%d currentProd=%d currentCommerce=%d natureFood=%d natureProd=%d natureCommerce=%d terrains=%s features=%s bonuses=%s improvements=%s routes=%s",
		iGameTurn, ePlayer, kPlayer.getNumCities(), kComposition.iWorked, kComposition.iWorkedImproved, kComposition.iWorkedUnimproved,
		kComposition.iLand, kComposition.iWater, kComposition.iHills, kComposition.iRiverSide, kComposition.iFreshWater,
		kComposition.iBonusImproved, kComposition.iBonusUnimproved, kComposition.iCurrentFood, kComposition.iCurrentProduction,
		kComposition.iCurrentCommerce, kComposition.iNatureFood, kComposition.iNatureProduction, kComposition.iNatureCommerce,
		getSASDiagnosticOrDash(szTerrains).GetCString(), getSASDiagnosticOrDash(szFeatures).GetCString(),
		getSASDiagnosticOrDash(szBonuses).GetCString(), getSASDiagnosticOrDash(szImprovements).GetCString(),
		getSASDiagnosticOrDash(szRoutes).GetCString());
}

static void appendSASGameRecordPlotCoordinate(CvString& szCoordinates, CvPlot const& kPlot)
{
	CvString szItem;
	szItem.Format(szCoordinates.empty() ? "(%d,%d)" : ",(%d,%d)", kPlot.getX(), kPlot.getY());
	szCoordinates += szItem;
}

static void appendSASGameRecordTypedPlotCoordinate(CvString& szCoordinates, char const* szType, CvPlot const& kPlot)
{
	CvString szItem;
	szItem.Format(szCoordinates.empty() ? "%s@(%d,%d)" : ",%s@(%d,%d)", szType, kPlot.getX(), kPlot.getY());
	szCoordinates += szItem;
}

// <!-- custom: Level-3 periodic city development is descriptive recorder state, deliberately not a Worker-AI diagnostic.
// Keep geometric radius counts separate from the owned plots actually assigned to this exact city through getWorkingCity(): the former exposes border/overlap context while the latter matches the territory row's BFC-development semantics and avoids double-counting shared radii.
// Development land excludes city centers/peaks; development water includes only visible bonus water or already improved water, so ordinary ocean is not mislabeled as a Worker backlog.
// Sparse coordinate/type lists stay bounded by one city radius and make persistent untouched Forest/Jungle/resource plots directly locatable in archived records without pathfinding, build-legality scans, yield valuation or BBAI reasoning. (ChatGPT-5.6-Sol) -->
static void logSASGameRecordCityDevelopment(CvCity const& kCity, int iGameTurn)
{
	PlayerTypes const ePlayer = kCity.getOwner();
	TeamTypes const eTeam = GET_PLAYER(ePlayer).getTeam();
	int iRadiusPlots = 0;
	int iRadiusLand = 0;
	int iRadiusWater = 0;
	int iOwnedRadiusPlots = 0;
	int iOwnedRadiusLand = 0;
	int iOwnedRadiusWater = 0;
	int iAssignedPlots = 0;
	int iAssignedLand = 0;
	int iAssignedWater = 0;
	int iDevelopmentLand = 0;
	int iImprovedLand = 0;
	int iDevelopmentWater = 0;
	int iImprovedWater = 0;
	int iBonusImproved = 0;
	int iBonusUnimproved = 0;
	std::vector<int> aiUnimprovedFeatures(GC.getNumFeatureInfos(), 0);
	std::vector<int> aiUnimprovedBonuses(GC.getNumBonusInfos(), 0);
	CvString szUnimprovedLandPlots;
	CvString szUnimprovedWaterPlots;
	CvString szUnimprovedFeaturePlots;
	CvString szUnimprovedBonusPlots;
	for (CityPlotIter it(kCity); it.hasNext(); ++it)
	{
		CvPlot const& kPlot = *it;
		iRadiusPlots++;
		if (kPlot.isWater())
			iRadiusWater++;
		else iRadiusLand++;
		if (kPlot.getOwner() != ePlayer)
			continue;
		iOwnedRadiusPlots++;
		if (kPlot.isWater())
			iOwnedRadiusWater++;
		else iOwnedRadiusLand++;
		if (kPlot.getWorkingCity() != &kCity)
			continue;
		iAssignedPlots++;
		if (kPlot.isWater())
			iAssignedWater++;
		else iAssignedLand++;
		ImprovementTypes const eImprovement = kPlot.getImprovementType();
		bool const bImproved = (eImprovement != NO_IMPROVEMENT);
		BonusTypes const eBonus = kPlot.getBonusType(eTeam);
		bool const bDevelopmentLand = (!kPlot.isWater() && !kPlot.isPeak() && !kPlot.isCity());
		bool const bDevelopmentWater = (kPlot.isWater() && (eBonus != NO_BONUS || bImproved));
		if (!bDevelopmentLand && !bDevelopmentWater)
			continue;
		if (eBonus != NO_BONUS)
		{
			if (bImproved)
				iBonusImproved++;
			else
			{
				iBonusUnimproved++;
				aiUnimprovedBonuses[eBonus]++;
				appendSASGameRecordTypedPlotCoordinate(szUnimprovedBonusPlots, getSASGameRecordBonusType(eBonus), kPlot);
			}
		}
		if (bDevelopmentLand)
		{
			iDevelopmentLand++;
			if (bImproved)
			{
				iImprovedLand++;
				continue;
			}
			appendSASGameRecordPlotCoordinate(szUnimprovedLandPlots, kPlot);
			FeatureTypes const eFeature = kPlot.getFeatureType();
			if (eFeature != NO_FEATURE)
			{
				aiUnimprovedFeatures[eFeature]++;
				appendSASGameRecordTypedPlotCoordinate(szUnimprovedFeaturePlots, getSASGameRecordFeatureType(eFeature), kPlot);
			}
		}
		else
		{
			iDevelopmentWater++;
			if (bImproved)
				iImprovedWater++;
			else appendSASGameRecordPlotCoordinate(szUnimprovedWaterPlots, kPlot);
		}
	}
	CvString szUnimprovedFeatures;
	CvString szUnimprovedBonuses;
	for (int iI = 0; iI < GC.getNumFeatureInfos(); iI++)
		appendSASGameRecordTypeCount(szUnimprovedFeatures, getSASGameRecordFeatureType((FeatureTypes)iI), aiUnimprovedFeatures[iI]);
	for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
		appendSASGameRecordTypeCount(szUnimprovedBonuses, getSASGameRecordBonusType((BonusTypes)iI), aiUnimprovedBonuses[iI]);
	int const iNotOwnedRadiusPlots = iRadiusPlots - iOwnedRadiusPlots;
	int const iOwnedUnassignedPlots = iOwnedRadiusPlots - iAssignedPlots;
	int const iDevelopmentPlots = iDevelopmentLand + iDevelopmentWater;
	int const iImprovedPlots = iImprovedLand + iImprovedWater;
	int const iUnimprovedPlots = iDevelopmentPlots - iImprovedPlots;
	int const iUnimprovedLand = iDevelopmentLand - iImprovedLand;
	int const iUnimprovedWater = iDevelopmentWater - iImprovedWater;
	logSASGameRecord("GAME_RECORD_CITY_DEVELOPMENT turn=%d player=%d cityId=%d city=%S x=%d y=%d radiusPlots=%d radiusLand=%d radiusWater=%d ownedRadiusPlots=%d ownedRadiusLand=%d ownedRadiusWater=%d notOwnedRadiusPlots=%d assignedPlots=%d assignedLand=%d assignedWater=%d ownedUnassignedPlots=%d developmentPlots=%d improvedPlots=%d unimprovedPlots=%d improvedPercentX100=%d developmentLand=%d improvedLand=%d unimprovedLand=%d improvedLandPercentX100=%d developmentWater=%d improvedWater=%d unimprovedWater=%d improvedWaterPercentX100=%d bonusImproved=%d bonusUnimproved=%d unimprovedFeatures=%s unimprovedBonuses=%s unimprovedLandPlots=%s unimprovedWaterPlots=%s unimprovedFeaturePlots=%s unimprovedBonusPlots=%s",
		iGameTurn, ePlayer, kCity.getID(), getSASGameRecordQuotedCityName(&kCity).GetCString(), kCity.getX(), kCity.getY(), iRadiusPlots,
		iRadiusLand, iRadiusWater, iOwnedRadiusPlots, iOwnedRadiusLand, iOwnedRadiusWater, iNotOwnedRadiusPlots, iAssignedPlots,
		iAssignedLand, iAssignedWater, iOwnedUnassignedPlots, iDevelopmentPlots, iImprovedPlots, iUnimprovedPlots,
		getSASGameRecordPercentX100(iImprovedPlots, iDevelopmentPlots), iDevelopmentLand, iImprovedLand, iUnimprovedLand,
		getSASGameRecordPercentX100(iImprovedLand, iDevelopmentLand), iDevelopmentWater, iImprovedWater, iUnimprovedWater,
		getSASGameRecordPercentX100(iImprovedWater, iDevelopmentWater), iBonusImproved, iBonusUnimproved,
		getSASDiagnosticOrDash(szUnimprovedFeatures).GetCString(), getSASDiagnosticOrDash(szUnimprovedBonuses).GetCString(),
		getSASDiagnosticOrDash(szUnimprovedLandPlots).GetCString(), getSASDiagnosticOrDash(szUnimprovedWaterPlots).GetCString(),
		getSASDiagnosticOrDash(szUnimprovedFeaturePlots).GetCString(), getSASDiagnosticOrDash(szUnimprovedBonusPlots).GetCString());
}

// <!-- custom: City snapshots use the same compact religion/corporation token builders later shared by city-removal provenance.
// Forward declarations keep the builders in their existing lifecycle section without duplicating list logic. (ChatGPT-5.6-Sol) -->
static CvString getSASGameRecordCityReligionList(CvCity const& kCity, bool bHolyOnly);
static CvString getSASGameRecordCityCorporationList(CvCity const& kCity, bool bHeadquartersOnly);

// <!-- custom: Private level-3-only helper; logSASGameRecordCities owns the single detail-level gate so this function does not repeat it for each city/subrow.
// Consequently the detailed trade-partner and large-garrison rows below intentionally have no local `gGameRecordLogLevel >= 3` checks; adding them back would only duplicate the caller gate once per city/subrow. (ChatGPT-5.6-Sol) -->
static void logSASGameRecordCityDetail(CvCity const& kCity, int iGameTurn)
{
	CvPlotGroup const* pPlotGroup = kCity.plotGroup(kCity.getOwner());
	int const iTradeRoutes = kCity.getTradeRoutes();
	int iDomesticTradeRoutes = 0;
	int iForeignTradeRoutes = 0;
	for (int iI = 0; iI < iTradeRoutes; iI++)
	{
		CvCity const* pTradeCity = kCity.getTradeCity(iI);
		if (pTradeCity == NULL)
			continue;
		if (pTradeCity->getOwner() == kCity.getOwner())
			iDomesticTradeRoutes++;
		else iForeignTradeRoutes++;
	}
	CvPlayer const& kOwner = GET_PLAYER(kCity.getOwner());
	const SASGameRecordPlotComposition kWorkedPlots = getSASGameRecordWorkedPlotComposition(kCity);
	SASGameRecordPlotUnitCounts kCityUnits;
	collectSASGameRecordPlotUnitCounts(kCity.getPlot(), kCity.getOwner(), kCityUnits);
	// <!-- custom: Keep the periodic city row self-contained enough to explain growth/starvation and current economic/cultural status without creating more per-turn rows.
	// Stored food/granary state, occupation/culture/maintenance and commerce-type output are cheap current-state getters; religion/corporation lists are small loaded-XML scans already used by city-removal provenance. (ChatGPT-5.6-Sol) -->
	CultureLevelTypes const eCultureLevel = kCity.getCultureLevel();
	PlayerTypes const eHighestCulturePlayer = kCity.findHighestCulture();
	// <!-- custom: Keep citizen accounting explicit for post-game city-management analysis.
	// Civ4's inherited specialist API names are unusually misleading: getSpecialistPopulation() is assigned population specialists; getNumGreatPeople() does not count born/present Great People here, but is the cached total changed by setFreeSpecialistCount() for actual typed free-specialist instances; totalFreeSpecialists() is instead the separate generic allowance that lets some assigned specialists be worked without consuming ordinary population.
	// Keep the recorder names explicit so these three mechanics are not conflated by future code/documentation cleanup.
	// Population changes only mark citizen assignment dirty, so a snapshot taken immediately after a severe population loss can temporarily retain more working/assigned citizens than the new population; these fields report observed state rather than promise an immediate arithmetic identity.
	// The by-type lists below use the same explicit naming. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	// <!-- custom: City-level commerce output/modifiers make each city's contribution to player-level gold/research/culture/espionage measurable; espionage defense remains a separate defensive modifier. (ChatGPT-5.6-Sol) -->
	// <!-- custom: Air-unit occupancy/capacity on the existing city row makes poor basing or saturated airbases visible without adding a separate late-game row.
	// Cargo aircraft are intentionally excluded by CvPlot::countNumAirUnits, matching actual base-capacity use. (GPT-5.6) -->
	// <!-- custom: City defense snapshots expose both the current post-bombard defense modifier and its undamaged ceiling.
	// DefenseDamage/MAX_CITY_DEFENSE_DAMAGE preserves the underlying bombardment state, while bombarded shows whether the city has already been hit this turn.
	// This lets broad game records be paired with the level-3 tactical bombardment actions below. (GPT-5.6) -->
	logSASGameRecord("GAME_RECORD_CITY turn=%d player=%d cityId=%d city=%S x=%d y=%d originalOwner=%d capital=%d foundedTurn=%d acquiredTurn=%d pop=%d highestPop=%d angryPopulation=%d workingPopulation=%d assignedSpecialists=%d freeSpecialistInstances=%d freeSpecialistAllowance=%d foodStored=%d foodKept=%d growthThreshold=%d maxFoodKeptPercent=%d avoidGrowth=%d foodSurplus=%d happySurplus=%d healthSurplus=%d food=%d prod=%d commerce=%d maintenanceTimes100=%d maintenanceModifier=%d occupationTurns=%d disorder=%d ownerCultureTimes100=%d cultureLevel=%s cultureLevelId=%d nextCultureThreshold=%d cultureUpdateTurns=%d ownerCulturePercent=%d highestCulturePlayer=%d highestCulturePercent=%d religions=%s holyReligions=%s corporations=%s headquarters=%s goldRate=%d researchRate=%d cultureRate=%d espionageRate=%d goldRateModifier=%d researchRateModifier=%d cultureRateModifier=%d espionageRateModifier=%d espionageDefenseModifier=%d defenseModifier=%d totalDefense=%d defenseDamage=%d defenseDamageMax=%d bombarded=%d airUnits=%d airCapacity=%d airSpaceAvailable=%d worked=%d workedImproved=%d workedUnimproved=%d workedFood=%d workedProd=%d workedCommerce=%d garrison=%d cityUnits=%d militaryUnits=%d civilianUnits=%d defenders=%d healthyDefenders=%d woundedDefenders=%d settlers=%d workers=%d attackers=%d connectedToCapital=%d plotGroupId=%d tradeRoutes=%d domesticTradeRoutes=%d foreignTradeRoutes=%d tradeFood=%d tradeProd=%d tradeCommerce=%d productionKind=%s production=%s productionUsesFood=%d productionTurns=%d productionStored=%d productionNeeded=%d overflowProduction=%d featureProduction=%d productionConversionX100=%s assignedSpecialistsByType=%s freeSpecialistsByType=%s gpProgress=%d gpThreshold=%d gpRate=%d gpTurnsLeft=%d gpOdds=%s",
		iGameTurn, kCity.getOwner(), kCity.getID(), getSASGameRecordQuotedCityName(&kCity).GetCString(), kCity.getX(), kCity.getY(),
		kCity.getOriginalOwner(), kCity.isCapital(), kCity.getGameTurnFounded(), kCity.getGameTurnAcquired(), kCity.getPopulation(),
		kCity.getHighestPopulation(), kCity.angryPopulation(), kCity.getWorkingPopulation(), kCity.getSpecialistPopulation(),
		kCity.getNumGreatPeople(), kCity.totalFreeSpecialists(), kCity.getFood(), kCity.getFoodKept(), kCity.growthThreshold(),
		kCity.getMaxFoodKeptPercent(), kCity.AI().AI_isEmphasizeAvoidGrowth() ? 1 : 0, kCity.foodDifference(),
		kCity.happyLevel() - kCity.unhappyLevel(), kCity.goodHealth() - kCity.badHealth(), kCity.getYieldRate(YIELD_FOOD),
		kCity.getYieldRate(YIELD_PRODUCTION), kCity.getYieldRate(YIELD_COMMERCE), kCity.getMaintenanceTimes100(),
		kCity.getMaintenanceModifier(), kCity.getOccupationTimer(), kCity.isDisorder() ? 1 : 0, kCity.getCultureTimes100(kCity.getOwner()),
		eCultureLevel == NO_CULTURELEVEL ? "-" : GC.getInfo(eCultureLevel).getType(), eCultureLevel, kCity.getCultureThreshold(),
		kCity.getCultureUpdateTimer(), kCity.calculateCulturePercent(kCity.getOwner()), eHighestCulturePlayer,
		eHighestCulturePlayer == NO_PLAYER ? 0 : kCity.calculateCulturePercent(eHighestCulturePlayer),
		getSASGameRecordCityReligionList(kCity, false).GetCString(), getSASGameRecordCityReligionList(kCity, true).GetCString(),
		getSASGameRecordCityCorporationList(kCity, false).GetCString(), getSASGameRecordCityCorporationList(kCity, true).GetCString(),
		kCity.getCommerceRate(COMMERCE_GOLD), kCity.getCommerceRate(COMMERCE_RESEARCH), kCity.getCommerceRate(COMMERCE_CULTURE),
		kCity.getCommerceRate(COMMERCE_ESPIONAGE), kCity.getTotalCommerceRateModifier(COMMERCE_GOLD),
		kCity.getTotalCommerceRateModifier(COMMERCE_RESEARCH), kCity.getTotalCommerceRateModifier(COMMERCE_CULTURE),
		kCity.getTotalCommerceRateModifier(COMMERCE_ESPIONAGE), kCity.getEspionageDefenseModifier(), kCity.getDefenseModifier(false),
		kCity.getTotalDefense(false), kCity.getDefenseDamage(), GC.getMAX_CITY_DEFENSE_DAMAGE(), kCity.isBombarded(),
		kCity.getPlot().countNumAirUnits(kCity.getTeam()), kCity.getAirUnitCapacity(kCity.getTeam()),
		kCity.getPlot().airUnitSpaceAvailable(kCity.getTeam()), kWorkedPlots.iWorked, kWorkedPlots.iWorkedImproved,
		kWorkedPlots.iWorkedUnimproved, kWorkedPlots.iCurrentFood, kWorkedPlots.iCurrentProduction, kWorkedPlots.iCurrentCommerce,
		kCity.plot()->getNumDefenders(kCity.getOwner()), kCityUnits.iUnits, kCityUnits.iMilitaryUnits, kCityUnits.iCivilianUnits,
		kCityUnits.iDefenders, kCityUnits.iHealthyDefenders, kCityUnits.iWoundedDefenders, kCityUnits.iSettlers, kCityUnits.iWorkers,
		kCityUnits.iAttackers, kCity.isConnectedToCapital(), pPlotGroup == NULL ? -1 : pPlotGroup->getID(), iTradeRoutes,
		iDomesticTradeRoutes, iForeignTradeRoutes, kCity.getTradeYield(YIELD_FOOD), kCity.getTradeYield(YIELD_PRODUCTION),
		kCity.getTradeYield(YIELD_COMMERCE), getSASGameRecordCityProductionKind(kCity), getSASGameRecordCityProductionType(kCity),
		kCity.isFoodProduction() ? 1 : 0, getSASGameRecordCityProductionTurns(kCity), kCity.getProduction(),
		getSASGameRecordCityProductionNeeded(kCity), kCity.getOverflowProduction(), kCity.getFeatureProduction(),
		getSASGameRecordCityProductionConversion(kCity).GetCString(), getSASGameRecordCitySpecialists(kCity, false).GetCString(),
		getSASGameRecordCitySpecialists(kCity, true).GetCString(), kCity.getGreatPeopleProgress(), kOwner.greatPeopleThreshold(false),
		kCity.getGreatPeopleRate(), kCity.GPTurnsLeft(), getSASGameRecordCityGPOdds(kCity).GetCString());
	logSASGameRecordCityDevelopment(kCity, iGameTurn);
	// <!-- custom: Source lists show the magnitude/origin of temporary happiness effects.
	// Retain their existing turn counters too so snapshots say how long whipping, drafting, defiance, temporary happiness and espionage unhappiness remain without logging per-turn timer decrements. (ChatGPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_CITY_HAPPINESS turn=%d player=%d cityId=%d happy=%d unhappy=%d surplus=%d hurryAngerTurns=%d conscriptAngerTurns=%d defyResolutionAngerTurns=%d temporaryHappinessTurns=%d espionageUnhappinessTurns=%d happySources=%s flatUnhappySources=%s angerPercentSources=%s",
		iGameTurn, kCity.getOwner(), kCity.getID(), kCity.happyLevel(), kCity.unhappyLevel(), kCity.happyLevel() - kCity.unhappyLevel(),
		kCity.getHurryAngerTimer(), kCity.getConscriptAngerTimer(), kCity.getDefyResolutionAngerTimer(), kCity.getHappinessTimer(),
		kCity.getEspionageHappinessCounter(), getSASGameRecordCityHappySources(kCity).GetCString(),
		getSASGameRecordCityFlatUnhappySources(kCity).GetCString(), getSASGameRecordCityAngerPercentSources(kCity).GetCString());
	// <!-- custom: Espionage unhealth is itself a decrementing duration counter, so preserve its remaining turns next to the existing unhealthy-source magnitude rather than emitting a row whenever the counter ticks down. (ChatGPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_CITY_HEALTH turn=%d player=%d cityId=%d goodHealth=%d badHealth=%d surplus=%d powered=%d dirtyPower=%d areaCleanPower=%d powerGoodHealth=%d powerBadHealth=%d espionageUnhealthTurns=%d healthySources=%s unhealthySources=%s",
			iGameTurn, kCity.getOwner(), kCity.getID(), kCity.goodHealth(), kCity.badHealth(), kCity.goodHealth() - kCity.badHealth(),
			kCity.isPower(), kCity.isDirtyPower(), kCity.isAreaCleanPower(), kCity.getPowerGoodHealth(), kCity.getPowerBadHealth(), kCity.getEspionageHealthCounter(),
			getSASGameRecordCityHealthySources(kCity).GetCString(), getSASGameRecordCityUnhealthySources(kCity).GetCString());
	int iBuildings, iRegularBuildings, iNationalWonders, iTeamWonders, iWorldWonders;
	CvString const szBuildings = getSASGameRecordCityBuildings(kCity, iBuildings, iRegularBuildings, iNationalWonders, iTeamWonders, iWorldWonders);
	logSASGameRecord("GAME_RECORD_CITY_BUILDINGS turn=%d player=%d cityId=%d total=%d regular=%d nationalWonders=%d teamWonders=%d worldWonders=%d buildings=%s",
		iGameTurn, kCity.getOwner(), kCity.getID(), iBuildings, iRegularBuildings, iNationalWonders, iTeamWonders, iWorldWonders, szBuildings.GetCString());
	logSASGameRecord("GAME_RECORD_CITY_TRADE_PARTNERS turn=%d player=%d cityId=%d partners=%s",
		iGameTurn, kCity.getOwner(), kCity.getID(), getSASGameRecordCityTradePartners(kCity).GetCString());
	// <!-- custom: Large city garrisons in autoplay logs did not reveal whether an army was one parked attack stack or many defensive/miscellaneous groups.
	// This helper is already level-3-only by caller contract, so only the six-military-unit threshold remains here; BBAI UNIT logging remains responsible for the groups' decision reasons. (GPT-5.6-Sol + ChatGPT-5.6-Sol) -->
	if (kCityUnits.iMilitaryUnits >= 6)
	{
		std::vector<int> aiUnitTypes(GC.getNumUnitInfos(), 0);
		std::vector<int> aiUnitAI(NUM_UNITAI_TYPES, 0);
		std::vector<int> aiGroupIds;
		CvSelectionGroup const* pLargestGroup = NULL;
		for (CLLNode<IDInfo> const* pUnitNode = kCity.getPlot().headUnitNode(); pUnitNode != NULL; pUnitNode = kCity.getPlot().nextUnitNode(pUnitNode))
		{
			CvUnit const* pLoopUnit = ::getUnit(pUnitNode->m_data);
			if (pLoopUnit == NULL || pLoopUnit->getOwner() != kCity.getOwner() || !isSASGameRecordMilitaryUnit(*pLoopUnit))
				continue;
			if (pLoopUnit->getUnitType() != NO_UNIT)
				aiUnitTypes[pLoopUnit->getUnitType()]++;
			UnitAITypes const eUnitAI = pLoopUnit->AI_getUnitAIType();
			if (eUnitAI >= 0 && eUnitAI < NUM_UNITAI_TYPES)
				aiUnitAI[eUnitAI]++;
			CvSelectionGroup const* pGroup = pLoopUnit->getGroup();
			if (pGroup == NULL)
				continue;
			bool bGroupAlreadyCounted = false;
			for (size_t iI = 0; iI < aiGroupIds.size(); iI++)
			{
				if (aiGroupIds[iI] == pGroup->getID())
				{
					bGroupAlreadyCounted = true;
					break;
				}
			}
			if (!bGroupAlreadyCounted)
				aiGroupIds.push_back(pGroup->getID());
			if (pLargestGroup == NULL || pGroup->getNumUnits() > pLargestGroup->getNumUnits())
				pLargestGroup = pGroup;
		}
		CvString szUnitTypes;
		CvString szUnitAI;
		for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
			appendSASGameRecordTypeCount(szUnitTypes, getSASGameRecordUnitType((UnitTypes)iI), aiUnitTypes[iI]);
		for (int iI = 0; iI < NUM_UNITAI_TYPES; iI++)
			appendSASGameRecordTypeCount(szUnitAI, getSASGameRecordUnitAIType((UnitAITypes)iI), aiUnitAI[iI]);
		CvSelectionGroupAI const* pLargestGroupAI = (pLargestGroup == NULL ? NULL : &pLargestGroup->AI());
		CvUnitAI const* pLargestGroupHead = (pLargestGroupAI == NULL ? NULL : pLargestGroupAI->AI_getHeadUnit());
		CvPlot const* pLargestGroupMissionPlot = (pLargestGroupAI == NULL ? NULL : pLargestGroupAI->AI_getMissionAIPlot());
		CvUnitAI const* pLargestGroupMissionUnit = (pLargestGroupAI == NULL ? NULL : pLargestGroupAI->AI_getMissionAIUnit());
		int iLargestGroupWounded = 0;
		if (pLargestGroup != NULL)
		{
			FOR_EACH_UNIT_IN(pLoopUnit, *pLargestGroup)
			{
				if (pLoopUnit->getDamage() > 0) iLargestGroupWounded++;
			}
		}
		// <!-- custom: Ordinary city-detail calculations above use CvPlayer, but incoming group-mission queries are available only through CvPlayerAI.
		// Keep the derived reference scoped to this level-3 diagnostic. (GPT-5.6-Sol) -->
		CvPlayerAI const& kOwnerAI = GET_PLAYER(kCity.getOwner());
		int const iLargestGroupIncomingJoiners = (pLargestGroupHead == NULL ? -1 : kOwnerAI.AI_unitTargetMissionAIs(*pLargestGroupHead, MISSIONAI_GROUP));
		// <!-- custom: A peaceful Aztec attack-city group grew to 86 of 137 military units but stopped appearing in ATTACK_CITY_PARKING, so its persistent state or an earlier return path was invisible.
		// At level 3, preserve the largest city group's activity, queued mission, MissionAI target, wounded count, and incoming joiners alongside composition; paired UNIT diagnostics trace AI_attackCityMove when it is actually entered. (GPT-5.6-Sol) -->
		logSASGameRecord("GAME_RECORD_CITY_UNIT_COMPOSITION turn=%d player=%d cityId=%d city=%S militaryUnits=%d groups=%d largestGroupId=%d largestGroupUnits=%d largestGroupHeadAI=%s largestGroupActivity=%d largestGroupMission=%s largestGroupMissionAI=%d largestGroupMissionPlot=(%d,%d) largestGroupMissionUnitOwner=%d largestGroupMissionUnitId=%d largestGroupMissionQueue=%d largestGroupWounded=%d largestGroupIncomingJoiners=%d unitTypes=%s unitAI=%s",
			iGameTurn, kCity.getOwner(), kCity.getID(), getSASGameRecordQuotedCityName(&kCity).GetCString(), kCityUnits.iMilitaryUnits,
			(int)aiGroupIds.size(), (pLargestGroup == NULL ? -1 : pLargestGroup->getID()),
			(pLargestGroup == NULL ? 0 : pLargestGroup->getNumUnits()),
			(pLargestGroupHead == NULL ? "-" : getSASGameRecordUnitAIType(pLargestGroupHead->AI_getUnitAIType())),
			(pLargestGroup == NULL ? NO_ACTIVITY : pLargestGroup->getActivityType()),
			(pLargestGroup == NULL ? "-" : getSASGameRecordMissionType(pLargestGroup->getMissionType(0))),
			(pLargestGroupAI == NULL ? NO_MISSIONAI : pLargestGroupAI->AI_getMissionAIType()),
			(pLargestGroupMissionPlot == NULL ? -1 : pLargestGroupMissionPlot->getX()),
			(pLargestGroupMissionPlot == NULL ? -1 : pLargestGroupMissionPlot->getY()),
			(pLargestGroupMissionUnit == NULL ? -1 : pLargestGroupMissionUnit->getOwner()),
			(pLargestGroupMissionUnit == NULL ? -1 : pLargestGroupMissionUnit->getID()),
			(pLargestGroup == NULL ? 0 : pLargestGroup->getLengthMissionQueue()), iLargestGroupWounded, iLargestGroupIncomingJoiners,
			getSASDiagnosticOrDash(szUnitTypes).GetCString(), getSASDiagnosticOrDash(szUnitAI).GetCString());
	}
}

static void logSASGameRecordCities(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	SASGameRecordPlayerPrevious& kPrevious = g_akSASGameRecordPlayerPrevious[ePlayer];
	bool const bLogCityDetails = (gGameRecordLogLevel >= 3);
	int iCities = 0;
	int iTotalFoodSurplus = 0;
	int iTotalHappySurplus = 0;
	int iTotalHealthSurplus = 0;
	int iTotalFoodYield = 0;
	int iTotalProductionYield = 0;
	int iTotalCommerceYield = 0;
	int iTotalFoodStored = 0;
	int iTotalFoodKept = 0;
	int iTotalMaintenanceTimes100 = 0;
	int iTotalTradeRoutes = 0;
	int iDomesticTradeRoutes = 0;
	int iForeignTradeRoutes = 0;
	int iTradeFood = 0;
	int iTradeProduction = 0;
	int iTradeCommerce = 0;
	int iConnectedToCapital = 0;
	int iUnhappyCities = 0;
	int iUnhealthyCities = 0;
	int iStarvingCities = 0;
	int iOccupiedCities = 0;
	int iDisorderCities = 0;
	int iAvoidGrowthCities = 0;
	int iAngryPopulation = 0;
	int iWorkingPopulation = 0;
	int iCitiesProducingUnits = 0;
	int iCitiesProducingMilitary = 0;
	int iCitiesProducingWorkers = 0;
	int iCitiesProducingSettlers = 0;
	int iCitiesProducingBuildings = 0;
	int iCitiesProducingWonders = 0;
	int iCitiesProducingProjects = 0;
	int iCitiesProducingProcesses = 0;
	int iAssignedSpecialists = 0;
	int iFreeSpecialistInstances = 0;
	int iFreeSpecialistAllowance = 0;
	int iGarrison = 0;
	int iCityUnits = 0;
	int iMilitaryUnitsInCities = 0;
	int iCivilianUnitsInCities = 0;
	int iDefendersInCities = 0;
	int iSettlersInCities = 0;
	int iWorkersInCities = 0;
	int iBestGPTurns = 1000000;
	CvCity const* pNextGPCity = NULL;
	int iLoop = 0;
	CvCity const* pCapital = kPlayer.getCapital();
	for (CvCity const* pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
	{
		iCities++;
		const int iFoodSurplus = pLoopCity->foodDifference();
		const int iHappySurplus = pLoopCity->happyLevel() - pLoopCity->unhappyLevel();
		const int iHealthSurplus = pLoopCity->goodHealth() - pLoopCity->badHealth();
		iTotalFoodSurplus += iFoodSurplus;
		iTotalHappySurplus += iHappySurplus;
		iTotalHealthSurplus += iHealthSurplus;
		iTotalFoodYield += pLoopCity->getYieldRate(YIELD_FOOD);
		iTotalProductionYield += pLoopCity->getYieldRate(YIELD_PRODUCTION);
		iTotalCommerceYield += pLoopCity->getYieldRate(YIELD_COMMERCE);
		iTotalFoodStored += pLoopCity->getFood();
		iTotalFoodKept += pLoopCity->getFoodKept();
		iTotalMaintenanceTimes100 += pLoopCity->getMaintenanceTimes100();
		int const iCityTradeRoutes = pLoopCity->getTradeRoutes();
		iTotalTradeRoutes += iCityTradeRoutes;
		iTradeFood += pLoopCity->getTradeYield(YIELD_FOOD);
		iTradeProduction += pLoopCity->getTradeYield(YIELD_PRODUCTION);
		iTradeCommerce += pLoopCity->getTradeYield(YIELD_COMMERCE);
		for (int iTrade = 0; iTrade < iCityTradeRoutes; iTrade++)
		{
			CvCity const* pTradeCity = pLoopCity->getTradeCity(iTrade);
			if (pTradeCity == NULL)
				continue;
			if (pTradeCity->getOwner() == ePlayer)
				iDomesticTradeRoutes++;
			else iForeignTradeRoutes++;
		}
		if (pLoopCity->isConnectedToCapital())
			iConnectedToCapital++;
		if (iHappySurplus < 0)
			iUnhappyCities++;
		if (iHealthSurplus < 0)
			iUnhealthyCities++;
		if (iFoodSurplus < 0)
			iStarvingCities++;
		if (pLoopCity->isOccupation())
			iOccupiedCities++;
		if (pLoopCity->isDisorder())
			iDisorderCities++;
		if (pLoopCity->AI().AI_isEmphasizeAvoidGrowth())
			iAvoidGrowthCities++;
		iAngryPopulation += pLoopCity->angryPopulation();
		iWorkingPopulation += pLoopCity->getWorkingPopulation();
		iAssignedSpecialists += pLoopCity->getSpecialistPopulation();
		iFreeSpecialistInstances += pLoopCity->getNumGreatPeople();
		iFreeSpecialistAllowance += pLoopCity->totalFreeSpecialists();
		iGarrison += pLoopCity->plot()->getNumDefenders(ePlayer);
		SASGameRecordPlotUnitCounts kCityUnits;
		collectSASGameRecordPlotUnitCounts(pLoopCity->getPlot(), ePlayer, kCityUnits);
		iCityUnits += kCityUnits.iUnits;
		iMilitaryUnitsInCities += kCityUnits.iMilitaryUnits;
		iCivilianUnitsInCities += kCityUnits.iCivilianUnits;
		iDefendersInCities += kCityUnits.iDefenders;
		iSettlersInCities += kCityUnits.iSettlers;
		iWorkersInCities += kCityUnits.iWorkers;
		const int iGPTurns = pLoopCity->GPTurnsLeft();
		if (iGPTurns >= 0 && iGPTurns < iBestGPTurns)
		{
			iBestGPTurns = iGPTurns;
			pNextGPCity = pLoopCity;
		}
		UnitTypes eProductionUnit = pLoopCity->getProductionUnit();
		BuildingTypes eProductionBuilding = pLoopCity->getProductionBuilding();
		if (eProductionUnit != NO_UNIT)
		{
			iCitiesProducingUnits++;
			UnitAITypes eUnitAI = GC.getInfo(eProductionUnit).getDefaultUnitAIType();
			if (GC.getInfo(eProductionUnit).isMilitaryProduction())
				iCitiesProducingMilitary++;
			if (eUnitAI == UNITAI_WORKER || eUnitAI == UNITAI_WORKER_SEA)
				iCitiesProducingWorkers++;
			if (eUnitAI == UNITAI_SETTLE)
				iCitiesProducingSettlers++;
		}
		else if (eProductionBuilding != NO_BUILDING)
		{
			iCitiesProducingBuildings++;
			if (GC.getInfo(eProductionBuilding).isLimited())
				iCitiesProducingWonders++;
		}
		else if (pLoopCity->getProductionProject() != NO_PROJECT)
			iCitiesProducingProjects++;
		else if (pLoopCity->getProductionProcess() != NO_PROCESS)
			iCitiesProducingProcesses++;
		if (bLogCityDetails) logSASGameRecordCityDetail(*pLoopCity, iGameTurn);
	}
	// <!-- custom: Aggregate citizen-state fields mirror the detailed city semantics: working/angry population, assigned population specialists, actual typed free-specialist instances, and the separate generic free-specialist allowance.
	// Keep these names explicit because the inherited APIs use historically ambiguous terminology. (ChatGPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_CITIES turn=%d player=%d cities=%d capitalId=%d capital=%S connectedToCapital=%d totalFoodSurplus=%d totalHappySurplus=%d totalHealthSurplus=%d totalFood=%d totalProd=%d totalCommerce=%d totalFoodStored=%d totalFoodKept=%d totalMaintenanceTimes100=%d tradeRoutes=%d domesticTradeRoutes=%d foreignTradeRoutes=%d tradeFood=%d tradeProd=%d tradeCommerce=%d unhappyCities=%d unhealthyCities=%d starvingCities=%d occupiedCities=%d disorderCities=%d avoidGrowthCities=%d angryPopulation=%d workingPopulation=%d assignedSpecialists=%d freeSpecialistInstances=%d freeSpecialistAllowance=%d garrison=%d cityUnits=%d militaryUnits=%d civilianUnits=%d defenders=%d settlers=%d workers=%d nextGPCityId=%d nextGPCity=%S nextGPTurns=%d nextGPRate=%d nextGPProgress=%d citiesProducingUnits=%d citiesProducingMilitary=%d citiesProducingWorkers=%d citiesProducingSettlers=%d citiesProducingBuildings=%d citiesProducingWonders=%d citiesProducingProjects=%d citiesProducingProcesses=%d",
		iGameTurn, ePlayer, iCities, pCapital == NULL ? -1 : pCapital->getID(), getSASGameRecordQuotedCityName(pCapital).GetCString(),
		iConnectedToCapital, iTotalFoodSurplus, iTotalHappySurplus, iTotalHealthSurplus, iTotalFoodYield, iTotalProductionYield,
		iTotalCommerceYield, iTotalFoodStored, iTotalFoodKept, iTotalMaintenanceTimes100, iTotalTradeRoutes, iDomesticTradeRoutes,
		iForeignTradeRoutes, iTradeFood, iTradeProduction, iTradeCommerce, iUnhappyCities, iUnhealthyCities, iStarvingCities,
		iOccupiedCities, iDisorderCities, iAvoidGrowthCities, iAngryPopulation, iWorkingPopulation, iAssignedSpecialists,
		iFreeSpecialistInstances, iFreeSpecialistAllowance, iGarrison, iCityUnits, iMilitaryUnitsInCities, iCivilianUnitsInCities,
		iDefendersInCities, iSettlersInCities, iWorkersInCities, pNextGPCity == NULL ? -1 : pNextGPCity->getID(),
		getSASGameRecordQuotedCityName(pNextGPCity).GetCString(), pNextGPCity == NULL ? -1 : iBestGPTurns,
		pNextGPCity == NULL ? 0 : pNextGPCity->getGreatPeopleRate(), pNextGPCity == NULL ? 0 : pNextGPCity->getGreatPeopleProgress(),
		iCitiesProducingUnits, iCitiesProducingMilitary, iCitiesProducingWorkers, iCitiesProducingSettlers, iCitiesProducingBuildings,
		iCitiesProducingWonders, iCitiesProducingProjects, iCitiesProducingProcesses);
	logSASGameRecord("GAME_RECORD_CITIES_DELTAS turn=%d player=%d deltaValid=%d citiesDelta=%+d connectedToCapitalDelta=%+d totalFoodSurplusDelta=%+d totalHappySurplusDelta=%+d totalHealthSurplusDelta=%+d totalFoodDelta=%+d totalProdDelta=%+d totalCommerceDelta=%+d tradeRoutesDelta=%+d tradeCommerceDelta=%+d angryPopulationDelta=%+d workingPopulationDelta=%+d assignedSpecialistsDelta=%+d freeSpecialistInstancesDelta=%+d freeSpecialistAllowanceDelta=%+d garrisonDelta=%+d",
		iGameTurn, ePlayer, kPrevious.bValid, getSASGameRecordDelta(kPrevious.bValid, iCities, kPrevious.iCityCount),
		getSASGameRecordDelta(kPrevious.bValid, iConnectedToCapital, kPrevious.iCityConnectedToCapital),
		getSASGameRecordDelta(kPrevious.bValid, iTotalFoodSurplus, kPrevious.iCityFoodSurplus),
		getSASGameRecordDelta(kPrevious.bValid, iTotalHappySurplus, kPrevious.iCityHappySurplus),
		getSASGameRecordDelta(kPrevious.bValid, iTotalHealthSurplus, kPrevious.iCityHealthSurplus),
		getSASGameRecordDelta(kPrevious.bValid, iTotalFoodYield, kPrevious.iCityFood),
		getSASGameRecordDelta(kPrevious.bValid, iTotalProductionYield, kPrevious.iCityProduction),
		getSASGameRecordDelta(kPrevious.bValid, iTotalCommerceYield, kPrevious.iCityCommerce),
		getSASGameRecordDelta(kPrevious.bValid, iTotalTradeRoutes, kPrevious.iCityTradeRoutes),
		getSASGameRecordDelta(kPrevious.bValid, iTradeCommerce, kPrevious.iCityTradeCommerce),
		getSASGameRecordDelta(kPrevious.bValid, iAngryPopulation, kPrevious.iCityAngryPopulation),
		getSASGameRecordDelta(kPrevious.bValid, iWorkingPopulation, kPrevious.iCityWorkingPopulation),
		getSASGameRecordDelta(kPrevious.bValid, iAssignedSpecialists, kPrevious.iCityAssignedSpecialists),
		getSASGameRecordDelta(kPrevious.bValid, iFreeSpecialistInstances, kPrevious.iCityFreeSpecialistInstances),
		getSASGameRecordDelta(kPrevious.bValid, iFreeSpecialistAllowance, kPrevious.iCityFreeSpecialistAllowance),
		getSASGameRecordDelta(kPrevious.bValid, iGarrison, kPrevious.iCityGarrison));
	kPrevious.iCityCount = iCities;
	kPrevious.iCityConnectedToCapital = iConnectedToCapital;
	kPrevious.iCityFoodSurplus = iTotalFoodSurplus;
	kPrevious.iCityHappySurplus = iTotalHappySurplus;
	kPrevious.iCityHealthSurplus = iTotalHealthSurplus;
	kPrevious.iCityFood = iTotalFoodYield;
	kPrevious.iCityProduction = iTotalProductionYield;
	kPrevious.iCityCommerce = iTotalCommerceYield;
	kPrevious.iCityTradeRoutes = iTotalTradeRoutes;
	kPrevious.iCityTradeCommerce = iTradeCommerce;
	kPrevious.iCityAngryPopulation = iAngryPopulation;
	kPrevious.iCityWorkingPopulation = iWorkingPopulation;
	kPrevious.iCityAssignedSpecialists = iAssignedSpecialists;
	kPrevious.iCityFreeSpecialistInstances = iFreeSpecialistInstances;
	kPrevious.iCityFreeSpecialistAllowance = iFreeSpecialistAllowance;
	kPrevious.iCityGarrison = iGarrison;
}

// <!-- custom: Ordinary civilization snapshots intentionally omit the Barbarian player because diplomacy, economy and victory-strategy rows do not meaningfully apply.
// Preserve the strategically useful Barbarian pressure instead through one compact summary, concise city rows, and level-3 unit positions. (GPT-5.6-Sol) -->
static void logSASGameRecordBarbarians(int iGameTurn)
{
	// <!-- custom: Barbarian AI uses the same AreaAI theater state as civilizations; checkpoint it here because the ordinary team-snapshot loop intentionally excludes the Barbarian team. (ChatGPT-5.6-Sol) -->
	logSASGameRecordAreaAISnapshot(BARBARIAN_TEAM, iGameTurn);
	CvPlayer const& kBarbarians = GET_PLAYER(BARBARIAN_PLAYER);
	std::vector<int> aiUnitTypes(GC.getNumUnitInfos(), 0);
	std::vector<int> aiUnitAI(NUM_UNITAI_TYPES, 0);
	std::vector<CvString> aszPositionChunks;
	CvString szPositionChunk;
	bool const bLogPositions = (gGameRecordLogLevel >= 3);
	int iUnits = 0;
	int iAnimals = 0;
	int iLandUnits = 0;
	int iSeaUnits = 0;
	int iCargoUnits = 0;
	int iUnitsInCities = 0;
	int iUnitsInBarbarianTerritory = 0;
	int iUnitsInUnownedTerritory = 0;
	int iUnitsInCivilizationTerritory = 0;
	int iWoundedUnits = 0;
	int iLoop = 0;
	for (CvUnit const* pLoopUnit = kBarbarians.firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = kBarbarians.nextUnit(&iLoop))
	{
		iUnits++;
		if (pLoopUnit->isAnimal()) iAnimals++;
		if (pLoopUnit->getDomainType() == DOMAIN_LAND) iLandUnits++;
		else if (pLoopUnit->getDomainType() == DOMAIN_SEA) iSeaUnits++;
		if (pLoopUnit->isCargo()) iCargoUnits++;
		if (pLoopUnit->getDamage() > 0) iWoundedUnits++;
		CvPlot const* pPlot = pLoopUnit->plot();
		if (pPlot != NULL)
		{
			if (pPlot->isCity()) iUnitsInCities++;
			if (pPlot->getOwner() == BARBARIAN_PLAYER) iUnitsInBarbarianTerritory++;
			else if (!pPlot->isOwned()) iUnitsInUnownedTerritory++;
			else iUnitsInCivilizationTerritory++;
		}
		if (pLoopUnit->getUnitType() != NO_UNIT) aiUnitTypes[pLoopUnit->getUnitType()]++;
		UnitAITypes const eUnitAI = pLoopUnit->AI_getUnitAIType();
		if (eUnitAI >= 0 && eUnitAI < NUM_UNITAI_TYPES) aiUnitAI[eUnitAI]++;
		if (bLogPositions && pPlot != NULL)
		{
			CvString szItem;
			szItem.Format("%s%s:%d:%s@(%d,%d)", szPositionChunk.empty() ? "" : ",", getSASGameRecordUnitType(pLoopUnit->getUnitType()), pLoopUnit->getID(), getSASGameRecordUnitAIType(eUnitAI), pPlot->getX(), pPlot->getY());
			if (!szPositionChunk.empty() && szPositionChunk.length() + szItem.length() > 1500)
			{
				aszPositionChunks.push_back(szPositionChunk);
				szPositionChunk.clear();
				szItem.Format("%s:%d:%s@(%d,%d)", getSASGameRecordUnitType(pLoopUnit->getUnitType()), pLoopUnit->getID(), getSASGameRecordUnitAIType(eUnitAI), pPlot->getX(), pPlot->getY());
			}
			szPositionChunk += szItem;
		}
	}
	if (!szPositionChunk.empty()) aszPositionChunks.push_back(szPositionChunk);
	CvString szUnitTypes;
	CvString szUnitAI;
	for (int iI = 0; iI < GC.getNumUnitInfos(); iI++) appendSASGameRecordTypeCount(szUnitTypes, getSASGameRecordUnitType((UnitTypes)iI), aiUnitTypes[iI]);
	for (int iI = 0; iI < NUM_UNITAI_TYPES; iI++) appendSASGameRecordTypeCount(szUnitAI, getSASGameRecordUnitAIType((UnitAITypes)iI), aiUnitAI[iI]);
	// <!-- custom: Barbarian research advances many technologies concurrently instead of selecting one current target.
	// Preserve only incomplete technologies with nonzero stored progress in the existing periodic row.
	// Completed technologies remain exact TECH_ACQUIRED source=BARBARIAN_RESEARCH actions. (ChatGPT-5.6-Sol) -->
	CvTeam const& kBarbarianTeam = GET_TEAM(kBarbarians.getTeam());
	CvString szPartialResearch;
	int iPartialResearchTechs = 0;
	FOR_EACH_ENUM(Tech)
	{
		if (kBarbarianTeam.isHasTech(eLoopTech))
			continue;
		int const iProgress = kBarbarianTeam.getResearchProgress(eLoopTech);
		if (iProgress <= 0)
			continue;
		CvString szItem;
		szItem.Format("%s%s:%d/%d", szPartialResearch.empty() ? "" : ",", getSASGameRecordTechType(eLoopTech), iProgress, kBarbarianTeam.getResearchCost(eLoopTech));
		szPartialResearch += szItem;
		iPartialResearchTechs++;
	}
	logSASGameRecord("GAME_RECORD_BARBARIAN_SUMMARY turn=%d cities=%d population=%d units=%d animals=%d nonAnimals=%d landUnits=%d seaUnits=%d cargoUnits=%d unitsInCities=%d unitsInBarbarianTerritory=%d unitsInUnownedTerritory=%d unitsInCivilizationTerritory=%d woundedUnits=%d unitTypes=%s unitAI=%s partialResearchTechs=%d partialResearch=%s",
		iGameTurn, kBarbarians.getNumCities(), kBarbarians.getTotalPopulation(), iUnits, iAnimals, iUnits - iAnimals, iLandUnits, iSeaUnits,
		iCargoUnits, iUnitsInCities, iUnitsInBarbarianTerritory, iUnitsInUnownedTerritory, iUnitsInCivilizationTerritory, iWoundedUnits,
		getSASDiagnosticOrDash(szUnitTypes).GetCString(), getSASDiagnosticOrDash(szUnitAI).GetCString(), iPartialResearchTechs,
		getSASDiagnosticOrDash(szPartialResearch).GetCString());
	int iCityLoop = 0;
	for (CvCity const* pLoopCity = kBarbarians.firstCity(&iCityLoop); pLoopCity != NULL; pLoopCity = kBarbarians.nextCity(&iCityLoop))
	{
		SASGameRecordPlotUnitCounts kCityUnits;
		collectSASGameRecordPlotUnitCounts(pLoopCity->getPlot(), BARBARIAN_PLAYER, kCityUnits);
		SASGameRecordPlotComposition const kWorkedPlots = getSASGameRecordWorkedPlotComposition(*pLoopCity);
		logSASGameRecord("GAME_RECORD_BARBARIAN_CITY turn=%d cityId=%d city=%S x=%d y=%d area=%d foundedTurn=%d age=%d pop=%d foodSurplus=%d prod=%d commerce=%d defenseModifier=%d totalDefense=%d cityUnits=%d defenders=%d healthyDefenders=%d woundedDefenders=%d workers=%d attackers=%d worked=%d workedImproved=%d workedUnimproved=%d productionKind=%s production=%s productionTurns=%d",
			iGameTurn, pLoopCity->getID(), getSASGameRecordQuotedCityName(pLoopCity).GetCString(), pLoopCity->getX(), pLoopCity->getY(),
			pLoopCity->getArea().getID(), pLoopCity->getGameTurnFounded(), iGameTurn - pLoopCity->getGameTurnFounded(),
			pLoopCity->getPopulation(), pLoopCity->foodDifference(), pLoopCity->getYieldRate(YIELD_PRODUCTION),
			pLoopCity->getYieldRate(YIELD_COMMERCE), pLoopCity->getDefenseModifier(false), pLoopCity->getTotalDefense(false),
			kCityUnits.iUnits, kCityUnits.iDefenders, kCityUnits.iHealthyDefenders, kCityUnits.iWoundedDefenders, kCityUnits.iWorkers,
			kCityUnits.iAttackers, kWorkedPlots.iWorked, kWorkedPlots.iWorkedImproved, kWorkedPlots.iWorkedUnimproved,
			getSASGameRecordCityProductionKind(*pLoopCity), getSASGameRecordCityProductionType(*pLoopCity),
			getSASGameRecordCityProductionTurns(*pLoopCity));
	}
	for (size_t iI = 0; iI < aszPositionChunks.size(); iI++)
		logSASGameRecord("GAME_RECORD_BARBARIAN_POSITIONS turn=%d part=%d parts=%d units=%s",
			iGameTurn, (int)iI + 1, (int)aszPositionChunks.size(), aszPositionChunks[iI].GetCString());
}

// <!-- custom: Complement the omniscient barbarian summary with the pressure that this player's team can actually know about.
// Visible-unit counts require both plot visibility and unit non-invisibility; barbarian cities distinguish revealed, currently visible, and actionable known state (AI players reuse K-Mod's existing city-deduction rule, humans require actual city revelation).
// UNITAI_EXPLORE / MISSIONAI_EXPLORE remain factual exploration state rather than being relabeled as fog-busting; explicit AdvCiv city-site fog-control assignments use MISSIONAI_GUARD_CITY and are tracked separately below and at their real assignment boundary.
// The existing AI barbarian-defense-focus predicate is also preserved without invoking barbarian target scoring. (ChatGPT-5.6-Sol) -->
static void logSASGameRecordBarbarianPressure(PlayerTypes ePlayer, int iGameTurn)
{
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer).AI();
	TeamTypes const eTeam = kPlayer.getTeam();
	CvPlayer const& kBarbarians = GET_PLAYER(BARBARIAN_PLAYER);
	int iVisibleUnits = 0;
	int iVisibleAnimals = 0;
	int iVisibleLandUnits = 0;
	int iVisibleSeaUnits = 0;
	int iVisibleUnitsInTerritory = 0;
	int iVisibleUnitsWithin3OfCity = 0;
	int iVisibleUnitsWithin6OfCity = 0;
	int iNearestVisibleUnitDistance = -1;
	int iNearestVisibleUnitId = -1;
	UnitTypes eNearestVisibleUnit = NO_UNIT;
	int iNearestVisibleUnitX = -1;
	int iNearestVisibleUnitY = -1;
	int iUnitLoop = 0;
	for (CvUnit const* pLoopUnit = kBarbarians.firstUnit(&iUnitLoop); pLoopUnit != NULL; pLoopUnit = kBarbarians.nextUnit(&iUnitLoop))
	{
		CvPlot const* pPlot = pLoopUnit->plot();
		if (pPlot == NULL || !pPlot->isVisible(eTeam, false) || pLoopUnit->isInvisible(eTeam, false))
			continue;
		iVisibleUnits++;
		if (pLoopUnit->isAnimal()) iVisibleAnimals++;
		if (pLoopUnit->getDomainType() == DOMAIN_LAND) iVisibleLandUnits++;
		else if (pLoopUnit->getDomainType() == DOMAIN_SEA) iVisibleSeaUnits++;
		if (pPlot->getOwner() == ePlayer) iVisibleUnitsInTerritory++;
		CvCity const* pNearestCity = GC.getMap().findCity(pPlot->getX(), pPlot->getY(), ePlayer, NO_TEAM, false);
		if (pNearestCity == NULL)
			continue;
		int const iDistance = plotDistance(pPlot->getX(), pPlot->getY(), pNearestCity->getX(), pNearestCity->getY());
		if (iDistance <= 3) iVisibleUnitsWithin3OfCity++;
		if (iDistance <= 6) iVisibleUnitsWithin6OfCity++;
		if (iNearestVisibleUnitDistance < 0 || iDistance < iNearestVisibleUnitDistance)
		{
			iNearestVisibleUnitDistance = iDistance;
			iNearestVisibleUnitId = pLoopUnit->getID();
			eNearestVisibleUnit = pLoopUnit->getUnitType();
			iNearestVisibleUnitX = pPlot->getX();
			iNearestVisibleUnitY = pPlot->getY();
		}
	}
	int iRevealedCities = 0;
	int iVisibleCities = 0;
	int iKnownCities = 0;
	int iNearestKnownCityDistance = -1;
	int iNearestKnownCityId = -1;
	int iNearestKnownCityX = -1;
	int iNearestKnownCityY = -1;
	int iCityLoop = 0;
	for (CvCity const* pLoopCity = kBarbarians.firstCity(&iCityLoop); pLoopCity != NULL; pLoopCity = kBarbarians.nextCity(&iCityLoop))
	{
		CvPlot const& kPlot = pLoopCity->getPlot();
		bool const bRevealed = pLoopCity->isRevealed(eTeam);
		if (bRevealed) iRevealedCities++;
		if (kPlot.isVisible(eTeam, false)) iVisibleCities++;
		bool const bKnown = (kPlayer.isHuman() ? bRevealed : kPlayer.AI_deduceCitySite(*pLoopCity));
		if (bKnown) iKnownCities++;
		if (!bKnown)
			continue;
		CvCity const* pNearestCity = GC.getMap().findCity(kPlot.getX(), kPlot.getY(), ePlayer, NO_TEAM, false);
		if (pNearestCity == NULL)
			continue;
		int const iDistance = plotDistance(kPlot.getX(), kPlot.getY(), pNearestCity->getX(), pNearestCity->getY());
		if (iNearestKnownCityDistance < 0 || iDistance < iNearestKnownCityDistance)
		{
			iNearestKnownCityDistance = iDistance;
			iNearestKnownCityId = pLoopCity->getID();
			iNearestKnownCityX = kPlot.getX();
			iNearestKnownCityY = kPlot.getY();
		}
	}
	int iExploreMissionGroups = 0;
	int iGuardCityMissionGroups = 0;
	int iCitySiteGuardGroups = 0;
	int iCitySiteGuardGroupsAtTarget = 0;
	int iCitySiteAdjacentGuardGroups = 0;
	CvString szCitySiteGuardTargets;
	bool const bLogCitySiteGuardTargets = (gGameRecordLogLevel >= 3);
	int const iCitySites = kPlayer.AI_getNumCitySites();
	int iGroupLoop = 0;
	for (CvSelectionGroup const* pLoopGroup = kPlayer.firstSelectionGroup(&iGroupLoop); pLoopGroup != NULL; pLoopGroup = kPlayer.nextSelectionGroup(&iGroupLoop))
	{
		MissionAITypes const eMissionAI = pLoopGroup->AI().AI_getMissionAIType();
		if (eMissionAI == MISSIONAI_EXPLORE)
			iExploreMissionGroups++;
		if (eMissionAI != MISSIONAI_GUARD_CITY)
			continue;
		iGuardCityMissionGroups++;
		CvPlot const* pMissionPlot = pLoopGroup->AI().AI_getMissionAIPlot();
		if (pMissionPlot == NULL || pMissionPlot->getPlotCity() != NULL)
			continue;
		int iMatchedSite = -1;
		int iMatchedDistance = 2;
		for (int iSite = 0; iSite < iCitySites; iSite++)
		{
			CvPlot const& kSite = kPlayer.AI_getCitySite(iSite);
			int const iDistance = plotDistance(pMissionPlot->getX(), pMissionPlot->getY(), kSite.getX(), kSite.getY());
			if (iDistance <= 1 && iDistance < iMatchedDistance)
			{
				iMatchedSite = iSite;
				iMatchedDistance = iDistance;
			}
		}
		if (iMatchedSite < 0)
			continue;
		iCitySiteGuardGroups++;
		if (iMatchedDistance > 0) iCitySiteAdjacentGuardGroups++;
		CvPlot const& kGroupPlot = pLoopGroup->getPlot();
		bool const bAtTarget = (&kGroupPlot == pMissionPlot);
		if (bAtTarget) iCitySiteGuardGroupsAtTarget++;
		if (bLogCitySiteGuardTargets)
		{
			if (!szCitySiteGuardTargets.empty()) szCitySiteGuardTargets += ";";
			CvPlot const& kSite = kPlayer.AI_getCitySite(iMatchedSite);
			CvString szTarget;
			szTarget.Format("%d:site%d@(%d,%d)>guard(%d,%d):current(%d,%d):adjacent%d:at%d",
				pLoopGroup->getID(),
				iMatchedSite + 1, kSite.getX(), kSite.getY(),
				pMissionPlot->getX(), pMissionPlot->getY(),
				kGroupPlot.getX(), kGroupPlot.getY(),
				iMatchedDistance > 0 ? 1 : 0, bAtTarget ? 1 : 0);
			szCitySiteGuardTargets += szTarget;
		}
	}
	int iBarbarianDefenseFocusAreas = -1;
	int iCapitalAreaBarbarianDefenseFocus = -1;
	int iBarbarianAttackersNeeded = -1;
	if (!kPlayer.isHuman())
	{
		iBarbarianDefenseFocusAreas = 0;
		std::vector<int> aiSettledAreaIds;
		int iOwnCityLoop = 0;
		for (CvCity const* pLoopCity = kPlayer.firstCity(&iOwnCityLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iOwnCityLoop))
		{
			int const iArea = pLoopCity->getArea().getID();
			if (std::find(aiSettledAreaIds.begin(), aiSettledAreaIds.end(), iArea) != aiSettledAreaIds.end())
				continue;
			aiSettledAreaIds.push_back(iArea);
			if (kPlayer.AI_isDefenseFocusOnBarbarians(pLoopCity->getArea()))
				iBarbarianDefenseFocusAreas++;
		}
		CvCity const* pCapital = kPlayer.getCapital();
		iCapitalAreaBarbarianDefenseFocus = (pCapital == NULL ? -1 : (kPlayer.AI_isDefenseFocusOnBarbarians(pCapital->getArea()) ? 1 : 0));
		iBarbarianAttackersNeeded = kPlayer.AI_neededCityAttackersVsBarbarians().ceil();
	}
	logSASGameRecord("GAME_RECORD_BARBARIAN_PRESSURE turn=%d player=%d team=%d human=%d barbarianCreationEra=%d visibleUnits=%d visibleAnimals=%d visibleNonAnimals=%d visibleLandUnits=%d visibleSeaUnits=%d visibleUnitsInTerritory=%d visibleUnitsWithin3OfCity=%d visibleUnitsWithin6OfCity=%d nearestVisibleUnitDistance=%d nearestVisibleUnitId=%d nearestVisibleUnit=%s nearestVisibleUnitX=%d nearestVisibleUnitY=%d revealedCities=%d visibleCities=%d knownCities=%d nearestKnownCityDistance=%d nearestKnownCityId=%d nearestKnownCityX=%d nearestKnownCityY=%d cityAttackRoleUnits=%d exploreRoleUnits=%d exploreMissionGroups=%d guardCityMissionGroups=%d citySiteGuardGroups=%d citySiteGuardGroupsAtTarget=%d citySiteAdjacentGuardGroups=%d citySiteGuardTargets=%s barbarianDefenseFocusAreas=%d capitalAreaBarbarianDefenseFocus=%d aiBarbarianAttackersNeeded=%d",
		iGameTurn, ePlayer, eTeam, kPlayer.isHuman() ? 1 : 0, GC.getGame().isBarbarianCreationEra() ? 1 : 0, iVisibleUnits, iVisibleAnimals,
		iVisibleUnits - iVisibleAnimals, iVisibleLandUnits, iVisibleSeaUnits, iVisibleUnitsInTerritory, iVisibleUnitsWithin3OfCity,
		iVisibleUnitsWithin6OfCity, iNearestVisibleUnitDistance, iNearestVisibleUnitId, getSASGameRecordUnitType(eNearestVisibleUnit),
		iNearestVisibleUnitX, iNearestVisibleUnitY, iRevealedCities, iVisibleCities, iKnownCities, iNearestKnownCityDistance,
		iNearestKnownCityId, iNearestKnownCityX, iNearestKnownCityY, kPlayer.AI_totalUnitAIs(UNITAI_ATTACK_CITY),
		kPlayer.AI_totalUnitAIs(UNITAI_EXPLORE), iExploreMissionGroups, iGuardCityMissionGroups, iCitySiteGuardGroups,
		iCitySiteGuardGroupsAtTarget, iCitySiteAdjacentGuardGroups, getSASDiagnosticOrDash(szCitySiteGuardTargets).GetCString(),
		iBarbarianDefenseFocusAreas, iCapitalAreaBarbarianDefenseFocus, iBarbarianAttackersNeeded);
}

static void logSASGameRecordPlayerSnapshot(PlayerTypes ePlayer, int iGameTurn)
{
	CvGame const& kGame = GC.getGame();
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	bool const bLogPlayerDetails = (gGameRecordLogLevel >= 2);
	// <!-- custom: Only query the level-3 threshold when level-2 detail logging is already active; disabled/level-1 snapshots pay one recorder-level read. (ChatGPT-5.6-Sol) -->
	bool const bLogPlayerVerboseDetails = (bLogPlayerDetails && gGameRecordLogLevel >= 3);
	CvTeam const& kTeam = GET_TEAM(kPlayer.getTeam());
	TechTypes eResearch = kPlayer.getCurrentResearch();
	const int iScore = kPlayer.calculateScore();
	const int iCities = kPlayer.getNumCities();
	const int iPopulation = kPlayer.getTotalPopulation();
	const int iLand = kPlayer.getTotalLand();
	const int iUnits = kPlayer.getNumUnits();
	const int iMilitarySupportUnits = kPlayer.getNumMilitaryUnits();
	// <!-- custom: CvPlayer::getNumMilitaryUnits counts XML bMilitarySupport, which can fall sharply when an army upgrades into combat units that intentionally do not pay military support.
	// Count actual combat-capable units with the same predicate used by GAME_RECORD_UNIT_POSTURE, and keep the raw Civ4 counter separately.
	// This scan runs only when a GameRecord player snapshot is already being generated. (ChatGPT-5.6-Sol) -->
	int iCombatUnits = 0;
	int iCombatLoop = 0;
	for (CvUnit const* pLoopUnit = kPlayer.firstUnit(&iCombatLoop); pLoopUnit != NULL; pLoopUnit = kPlayer.nextUnit(&iCombatLoop))
	{
		if (isSASGameRecordMilitaryUnit(*pLoopUnit)) ++iCombatUnits;
	}
	const int iPower = kPlayer.getPower();
	const int iGold = kPlayer.getGold();
	const int iGoldRate = kPlayer.calculateGoldRate();
	// <!-- custom: Keep nominal science visible when no target is selected because that science becomes stored research overflow rather than disappearing. (GPT-5.6-Sol) -->
	const int iResearchRate = kPlayer.calculateResearchRate(eResearch);
	const int iResearchTurns = (eResearch == NO_TECH ? -1 : kPlayer.getResearchTurnsLeft(eResearch, true));
	const int iHistoryScore = kPlayer.getHistorySafe(PLAYER_HISTORY_SCORE, iGameTurn);
	const int iHistoryEconomy = kPlayer.getHistorySafe(PLAYER_HISTORY_ECONOMY, iGameTurn);
	const int iHistoryIndustry = kPlayer.getHistorySafe(PLAYER_HISTORY_INDUSTRY, iGameTurn);
	const int iHistoryAgriculture = kPlayer.getHistorySafe(PLAYER_HISTORY_AGRICULTURE, iGameTurn);
	const int iHistoryPower = kPlayer.getHistorySafe(PLAYER_HISTORY_POWER, iGameTurn);
	const int iHistoryCulture = kPlayer.getHistorySafe(PLAYER_HISTORY_CULTURE, iGameTurn);
	const int iHistoryEspionage = kPlayer.getHistorySafe(PLAYER_HISTORY_ESPIONAGE, iGameTurn);
	SASGameRecordPlayerPrevious& kPrevious = g_akSASGameRecordPlayerPrevious[ePlayer];
	const char* szCiv = (kPlayer.getCivilizationType() == NO_CIVILIZATION ? "-" : GC.getInfo(kPlayer.getCivilizationType()).getType());
	const char* szLeader = (kPlayer.getLeaderType() == NO_LEADER ? "-" : GC.getInfo(kPlayer.getLeaderType()).getType());
	const char* szEra = (kPlayer.getCurrentEra() == NO_ERA ? "-" : GC.getInfo(kPlayer.getCurrentEra()).getType());
	const bool bCurrentlyHumanControlled = kPlayer.isHuman();
	const bool bAutoplayControlled = kPlayer.isHumanDisabled();
	const bool bHumanSlot = (bCurrentlyHumanControlled || bAutoplayControlled);
	// <!-- custom: These observations begin with the current log session rather than the civilization's lifetime; expose that scope in the player-row field names. See KI#379. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	// <!-- custom: Financial strike is player-wide, not a city disorder state.
	// `getStrikeTurns` is an inherited cumulative counter that is not reset when a strike ends, so expose it as `cumulativeStrikeTurns` rather than implying a current streak.
	// Exact strike turns are logged separately at level 2. (ChatGPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_PLAYER turn=%d player=%d team=%d civ=%s leader=%s isHuman=%d humanSlot=%d currentlyHumanControlled=%d autoplayControlled=%d rank=%d deltaValid=%d score=%d scoreDelta=%+d cities=%d citiesDelta=%+d pop=%d popDelta=%+d land=%d landDelta=%+d units=%d unitsDelta=%+d combatUnits=%d combatUnitsDelta=%+d militarySupportUnits=%d militarySupportUnitsDelta=%+d power=%d powerDelta=%+d gold=%d goldDelta=%+d gpt=%d gptDelta=%+d financialStrike=%d cumulativeStrikeTurns=%d researchRate=%d researchRateDelta=%+d researchPercent=%d currentResearch=%s researchOverflow=%d noResearchAvailable=%d researchTurns=%d era=%s stateReligion=%s techScorePercent=%d combatXP=%d greatPeopleCreated=%d greatGeneralsCreated=%d greatGeneralThreshold=%d goldenAgeTurns=%d loggedGoldenAgeTurns=%d anarchyTurns=%d loggedAnarchyTurns=%d revolutionTimer=%d conversionTimer=%d wars=%s",
		iGameTurn, ePlayer, kPlayer.getTeam(), szCiv, szLeader, bCurrentlyHumanControlled, bHumanSlot, bCurrentlyHumanControlled,
		bAutoplayControlled, kGame.getPlayerRank(ePlayer) + 1, kPrevious.bValid, iScore,
		getSASGameRecordDelta(kPrevious.bValid, iScore, kPrevious.iScore), iCities,
		getSASGameRecordDelta(kPrevious.bValid, iCities, kPrevious.iCities), iPopulation,
		getSASGameRecordDelta(kPrevious.bValid, iPopulation, kPrevious.iPopulation), iLand,
		getSASGameRecordDelta(kPrevious.bValid, iLand, kPrevious.iLand), iUnits,
		getSASGameRecordDelta(kPrevious.bValid, iUnits, kPrevious.iUnits), iCombatUnits,
		getSASGameRecordDelta(kPrevious.bValid, iCombatUnits, kPrevious.iCombatUnits), iMilitarySupportUnits,
		getSASGameRecordDelta(kPrevious.bValid, iMilitarySupportUnits, kPrevious.iMilitarySupportUnits), iPower,
		getSASGameRecordDelta(kPrevious.bValid, iPower, kPrevious.iPower), iGold,
		getSASGameRecordDelta(kPrevious.bValid, iGold, kPrevious.iGold), iGoldRate,
		getSASGameRecordDelta(kPrevious.bValid, iGoldRate, kPrevious.iGoldRate), kPlayer.isStrike(), kPlayer.getStrikeTurns(), iResearchRate,
		getSASGameRecordDelta(kPrevious.bValid, iResearchRate, kPrevious.iResearchRate), kPlayer.getCommercePercent(COMMERCE_RESEARCH),
		getSASGameRecordTechType(eResearch), kPlayer.getOverflowResearch(), kPlayer.isNoResearchAvailable(), iResearchTurns, szEra,
		getSASGameRecordReligionType(kPlayer.getStateReligion()), kTeam.getBestKnownTechScorePercent(), kPlayer.getCombatExperience(),
		kPlayer.getGreatPeopleCreated(), kPlayer.getGreatGeneralsCreated(), kPlayer.greatPeopleThreshold(true), kPlayer.getGoldenAgeTurns(),
		g_aiSASGameRecordLoggedGoldenAgeTurns[ePlayer], kPlayer.getAnarchyTurns(), g_aiSASGameRecordLoggedAnarchyTurns[ePlayer],
		kPlayer.getRevolutionTimer(), kPlayer.getConversionTimer(), getSASGameRecordWarTeams(kPlayer.getTeam()).GetCString());
	logSASGameRecord("GAME_RECORD_PLAYER_HISTORY turn=%d player=%d deltaValid=%d historyScore=%d historyScoreDelta=%+d historyEconomy=%d historyEconomyDelta=%+d historyIndustry=%d historyIndustryDelta=%+d historyAgriculture=%d historyAgricultureDelta=%+d historyPower=%d historyPowerDelta=%+d historyCulture=%d historyCultureDelta=%+d historyEspionage=%d historyEspionageDelta=%+d",
		iGameTurn, ePlayer, kPrevious.bValid, iHistoryScore, getSASGameRecordDelta(kPrevious.bValid, iHistoryScore, kPrevious.iHistoryScore),
		iHistoryEconomy, getSASGameRecordDelta(kPrevious.bValid, iHistoryEconomy, kPrevious.iHistoryEconomy), iHistoryIndustry,
		getSASGameRecordDelta(kPrevious.bValid, iHistoryIndustry, kPrevious.iHistoryIndustry), iHistoryAgriculture,
		getSASGameRecordDelta(kPrevious.bValid, iHistoryAgriculture, kPrevious.iHistoryAgriculture), iHistoryPower,
		getSASGameRecordDelta(kPrevious.bValid, iHistoryPower, kPrevious.iHistoryPower), iHistoryCulture,
		getSASGameRecordDelta(kPrevious.bValid, iHistoryCulture, kPrevious.iHistoryCulture), iHistoryEspionage,
		getSASGameRecordDelta(kPrevious.bValid, iHistoryEspionage, kPrevious.iHistoryEspionage));
	// <!-- custom: The environment row shows world pollution, but not which player produced it or whether buildings, bonuses, dirty power, or population caused it.
	// Keep these city scans behind record level 2, and derive the total from the four components rather than scanning a fifth time. (GPT-5.6-Sol) -->
	if (bLogPlayerDetails)
	{
		int const iBuildingPollution = kPlayer.calculatePollution(CvPlayer::POLLUTION_BUILDINGS);
		int const iBonusPollution = kPlayer.calculatePollution(CvPlayer::POLLUTION_BONUSES);
		int const iPowerPollution = kPlayer.calculatePollution(CvPlayer::POLLUTION_POWER);
		int const iPopulationPollution = kPlayer.calculatePollution(CvPlayer::POLLUTION_POPULATION);
		logSASGameRecord("GAME_RECORD_POLLUTION turn=%d player=%d total=%d buildings=%d bonuses=%d power=%d population=%d",
			iGameTurn, ePlayer, iBuildingPollution + iBonusPollution + iPowerPollution + iPopulationPollution, iBuildingPollution,
			iBonusPollution, iPowerPollution, iPopulationPollution);
	}
	if (bLogPlayerDetails)
	{
		logSASGameRecordPlayerBonuses(ePlayer, iGameTurn, kPrevious);
		logSASGameRecordAIStrategies(ePlayer, iGameTurn);
		logSASGameRecordAITargetCities(ePlayer, iGameTurn);
		logSASGameRecordAIVictoryStages(ePlayer, iGameTurn);
		logSASGameRecordAIMilitaryProduction(ePlayer, iGameTurn);
		logSASGameRecordPolicies(ePlayer, iGameTurn);
		logSASGameRecordCorporationPosture(ePlayer, iGameTurn);
		logSASGameRecordEconomy(ePlayer, iGameTurn);
		logSASGameRecordProductionPipeline(ePlayer, iGameTurn);
		logSASGameRecordStatistics(ePlayer, iGameTurn);
		logSASGameRecordEspionage(ePlayer, iGameTurn);
		logSASGameRecordDemographics(ePlayer, iGameTurn);
		logSASGameRecordAttitudes(ePlayer, iGameTurn);
		if (bLogPlayerVerboseDetails) logSASGameRecordDiplomaticAttitudes(ePlayer, iGameTurn);
		logSASGameRecordDiploStatus(ePlayer, iGameTurn);
		logSASGameRecordUnitPosture(ePlayer, iGameTurn);
		logSASGameRecordBarbarianPressure(ePlayer, iGameTurn);
		logSASGameRecordWorkers(ePlayer, iGameTurn);
		logSASGameRecordExpansion(ePlayer, iGameTurn);
		logSASGameRecordSettlers(ePlayer, iGameTurn);
		logSASGameRecordWorkedPlots(ePlayer, iGameTurn);
		logSASGameRecordCities(ePlayer, iGameTurn);
	}
	kPrevious.bValid = true;
	kPrevious.iScore = iScore;
	kPrevious.iCities = iCities;
	kPrevious.iPopulation = iPopulation;
	kPrevious.iLand = iLand;
	kPrevious.iUnits = iUnits;
	kPrevious.iCombatUnits = iCombatUnits;
	kPrevious.iMilitarySupportUnits = iMilitarySupportUnits;
	kPrevious.iPower = iPower;
	kPrevious.iGold = iGold;
	kPrevious.iGoldRate = iGoldRate;
	kPrevious.iResearchRate = iResearchRate;
	if (bLogPlayerDetails)
	{
		int iBonusTypes = 0;
		int iBonusInstances = 0;
		int iBonusImports = 0;
		int iBonusExports = 0;
		FOR_EACH_ENUM(Bonus)
		{
			const int iAvailable = kPlayer.getNumAvailableBonuses(eLoopBonus);
			if (iAvailable > 0)
			{
				iBonusTypes++;
				iBonusInstances += iAvailable;
			}
			iBonusImports += kPlayer.getBonusImport(eLoopBonus);
			iBonusExports += kPlayer.getBonusExport(eLoopBonus);
		}
		kPrevious.iBonusTypes = iBonusTypes;
		kPrevious.iBonusInstances = iBonusInstances;
		kPrevious.iBonusImports = iBonusImports;
		kPrevious.iBonusExports = iBonusExports;
	}
	kPrevious.iHistoryScore = iHistoryScore;
	kPrevious.iHistoryEconomy = iHistoryEconomy;
	kPrevious.iHistoryIndustry = iHistoryIndustry;
	kPrevious.iHistoryAgriculture = iHistoryAgriculture;
	kPrevious.iHistoryPower = iHistoryPower;
	kPrevious.iHistoryCulture = iHistoryCulture;
	kPrevious.iHistoryEspionage = iHistoryEspionage;
}

static void logSASGameRecordSnapshot(int iGameTurn, char const* szReason)
{
	CvGame const& kGame = GC.getGame();
	bool const bLogSnapshotDetails = (gGameRecordLogLevel >= 2);
	bool const bLogSnapshotVerboseDetails = (bLogSnapshotDetails && gGameRecordLogLevel >= 3);
	// <!-- custom: The initial save-load row is written before graphics initialization; the first full snapshot supplies the deferred display context. (GPT-5.6-Sol) -->
	logSASGameRecordDisplayContext();
	uint const uiSnapshotTime = getSASMonotonicMilliseconds();
	uint const uiSnapshotIntervalWallMilliseconds = getSASElapsedMilliseconds(g_uiSASGameRecordPreviousSnapshotTime, uiSnapshotTime);
	CvString const szSnapshotUtc = createSASUtcTimestampMilliseconds();
	// <!-- custom: Focus at this instant cannot prove how long Civ4 was foreground during the interval.
	// Repeated exact samples still help qualify wall-time comparisons without continuous monitoring. (GPT-5.6-Sol) -->
	// <!-- custom: Working set is Civ4's resident RAM, while page-file usage and available 32-bit virtual space help expose memory growth or address-space pressure.
	// Sample these only with the existing full snapshot, and skip all optional system calls when performance metrics are disabled. (GPT-5.6-Sol) -->
	SASGameRecordSystemSnapshot const kSystemSnapshot = getSASGameRecordSystemSnapshot();
	// <!-- custom: Team death and other state transitions can terminate a war without CvTeam::makePeace.
	// Reconcile before snapshots so such wars still receive one final synthetic summary. (GPT-5.6-Sol) -->
	if (bLogSnapshotDetails) reconcileSASGameRecordWars();
	// <!-- custom: This is primarily an autoplay/game-history row, so place its frequently scanned gameplay state first.
	// Keep wall-time and optional operating-system measurements afterward as supporting performance context. (GPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_TURN_BEGIN turn=%d reason=%s elapsed=%d year=%d playersAlive=%d teamsAlive=%d totalCities=%d totalPopulation=%d utc=%s sessionWallMilliseconds=%u snapshotIntervalWallMilliseconds=%u performanceMetricsEnabled=%d processForegroundAtSnapshot=%d processWindowMinimizedAtSnapshot=%d processWorkingSetKB=%d processPeakWorkingSetKB=%d processPagefileUsageKB=%d systemMemoryLoadPercent=%d processAvailableVirtualMB=%d",
		iGameTurn, szReason, kGame.getElapsedGameTurns(), kGame.getGameTurnYear(), kGame.countCivPlayersAlive(), kGame.countCivTeamsAlive(),
		kGame.getNumCities(), kGame.getTotalPopulation(), szSnapshotUtc.GetCString(),
		getSASGameRecordSessionWallMilliseconds(uiSnapshotTime), uiSnapshotIntervalWallMilliseconds,
		isSASGameRecordPerformanceMetricsEnabled(), kSystemSnapshot.iProcessForeground, kSystemSnapshot.iProcessWindowMinimized,
		kSystemSnapshot.iProcessWorkingSetKB, kSystemSnapshot.iProcessPeakWorkingSetKB, kSystemSnapshot.iProcessPagefileUsageKB,
		kSystemSnapshot.iSystemMemoryLoadPercent, kSystemSnapshot.iProcessAvailableVirtualMB);
	g_uiSASGameRecordPreviousSnapshotTime = uiSnapshotTime;
	logSASGameRecordRunStatus(szReason);
	if (bLogSnapshotVerboseDetails) logSASGameRecordMapAscii(false, szReason);
	if (bLogSnapshotDetails)
	{
		logSASGameRecordMapBonusTotals(iGameTurn);
		logSASGameRecordEnvironment(iGameTurn);
		logSASGameRecordVoteSources(iGameTurn);
	}
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes eLoopPlayer = (PlayerTypes)iI;
		if (GET_PLAYER(eLoopPlayer).isAlive() && !GET_PLAYER(eLoopPlayer).isBarbarian())
			logSASGameRecordPlayerSnapshot(eLoopPlayer, iGameTurn);
	}
	// <!-- custom: Reproduce the active player's resolved Foreign Advisor market only at level 3 and only when its independent switch is enabled; lower detail levels and disabled-market runs skip the entire pair/item scan. (ChatGPT-5.6-Sol) -->
	if (bLogSnapshotVerboseDetails && isSASGameRecordTradeMarketEnabled()) logSASGameRecordTradeMarket(iGameTurn);
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		TeamTypes eLoopTeam = (TeamTypes)iI;
		if (GET_TEAM(eLoopTeam).isAlive() && !GET_TEAM(eLoopTeam).isBarbarian())
			logSASGameRecordTeamSnapshot(eLoopTeam, iGameTurn);
	}
	if (bLogSnapshotDetails)
	{
		logSASGameRecordBarbarians(iGameTurn);
		logSASGameRecordBattleBuckets(iGameTurn);
		logSASGameRecordFlowBuckets(iGameTurn);
	}
	uint const uiSnapshotEndTime = getSASMonotonicMilliseconds();
	logSASGameRecord("GAME_RECORD_TURN_END turn=%d reason=%s sessionWallMilliseconds=%u snapshotWallMilliseconds=%u",
		iGameTurn, szReason, getSASGameRecordSessionWallMilliseconds(uiSnapshotEndTime),
		getSASElapsedMilliseconds(uiSnapshotTime, uiSnapshotEndTime));
	g_iSASGameRecordLastFullSnapshotTurn = iGameTurn;
}

void logSASGameRecordTurn(int iGameTurn)
{
	// <!-- custom: Victory now forces a full snapshot immediately. If it occurs on an ordinary snapshot turn, do not repeat the same large snapshot again at end-of-turn. (GPT-5.6-Sol) -->
	if (g_iSASGameRecordLastFullSnapshotTurn == iGameTurn)
		return;
	logSASGameRecordSnapshot(iGameTurn, "interval");
}

// <!-- custom: High-level queue-mutating paths call this only at SASGameRecord level 2+. Keep the latest authoritative cause until the player's next finalized research-target observation.
// If it produces no invested-tech redirection, the observer discards it rather than emitting a standalone/noisy action. (ChatGPT-5.6-Sol) -->
void noteSASGameRecordResearchTargetChangeCause(PlayerTypes ePlayer, ResearchTargetChangeCause eCause)
{
	if (ePlayer < 0 || ePlayer >= MAX_PLAYERS)
		return;
	g_akSASGameRecordResearchPrevious[ePlayer].ePendingCause = eCause;
}

// <!-- custom: Called only from the level-2-gated ordinary research-application path, using values gameplay already computes.
// Store the same-turn split so a completion row can distinguish fresh research from carried overflow without logging every non-completing research turn. (ChatGPT-5.6-Sol) -->
void noteSASGameRecordResearchApplication(PlayerTypes ePlayer, TechTypes eTech, int iModifiedResearchRate, int iIncomingOverflowUnmodified, int iIncomingOverflowModified)
{
	if (ePlayer < 0 || ePlayer >= MAX_PLAYERS)
		return;
	SASGameRecordResearchApplication& kApplication = g_akSASGameRecordResearchApplication[ePlayer];
	kApplication.bValid = true;
	kApplication.iGameTurn = GC.getGame().getGameTurn();
	kApplication.eTech = eTech;
	kApplication.iModifiedResearchRate = iModifiedResearchRate;
	kApplication.iIncomingOverflowUnmodified = iIncomingOverflowUnmodified;
	kApplication.iIncomingOverflowModified = iIncomingOverflowModified;
}

// <!-- custom: Accumulate Golden Age and anarchy turns observed in this GameRecord session only; loaded saves deliberately begin new logs rather than pretending these are persisted lifetime totals. See KI#379.
// This helper intentionally remains callable at every enabled log level because these duration counters feed lower-detail rows; the research branch below separately self-gates at level 2+. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void updateSASGameRecordPlayerTurnState(PlayerTypes ePlayer)
{
	if (ePlayer < 0 || ePlayer >= MAX_PLAYERS)
		return;
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	if (kPlayer.getGoldenAgeTurns() > 0)
		g_aiSASGameRecordLoggedGoldenAgeTurns[ePlayer]++;
	if (kPlayer.getAnarchyTurns() > 0)
		g_aiSASGameRecordLoggedAnarchyTurns[ePlayer]++;

	// <!-- custom: AI_doResearch has already finalized this turn's target before this hook, while CvPlayer::doResearch has not yet applied this turn's science.
	// Compare that stable boundary with the previous player turn and record only switches away from a still-incomplete technology; routine completed-tech queue progression is deliberately suppressed.
	// Cause comes only from explicit high-level queue-mutating hooks; unknown/uninstrumented paths stay UNKNOWN rather than being inferred from nearby events. (ChatGPT-5.6-Sol) -->
	if (gGameRecordLogLevel >= 2 && kPlayer.isAlive() && !kPlayer.isBarbarian())
	{
		SASGameRecordResearchPrevious& kPrevious = g_akSASGameRecordResearchPrevious[ePlayer];
		TeamTypes const eTeam = kPlayer.getTeam();
		CvTeam const& kTeam = GET_TEAM(eTeam);
		TechTypes const eResearch = kPlayer.getCurrentResearch();
		if (kPrevious.bValid && kPrevious.eTeam == eTeam && kPrevious.eTech != NO_TECH && kPrevious.eTech != eResearch)
		{
			TechTypes const eOldResearch = kPrevious.eTech;
			bool const bOldResearchCompleted = (GC.getInfo(eOldResearch).isRepeat() ?
				kTeam.getTechCount(eOldResearch) > kPrevious.iTechCount : kTeam.isHasTech(eOldResearch));
			if (!bOldResearchCompleted)
			{
				int const iOldProgress = kTeam.getResearchProgress(eOldResearch);
				// <!-- custom: A zero-progress target change wastes/parks no research and is common enough to be low-value noise.
				// Once progress exists, retain team-game context too: another teammate may still be researching the old technology, so this row must not imply that the team's investment was abandoned. (ChatGPT-5.6-Sol) -->
				if (iOldProgress > 0)
				{
					int iOldTeamResearchersAfter = 0;
					for (MemberIter it(eTeam); it.hasNext(); ++it)
					{
						if (it->getCurrentResearch() == eOldResearch)
							iOldTeamResearchersAfter++;
					}
					int const iOldCost = kTeam.getResearchCost(eOldResearch);
					int const iNewProgress = (eResearch == NO_TECH ? 0 : kTeam.getResearchProgress(eResearch));
					int const iNewCost = (eResearch == NO_TECH ? -1 : kTeam.getResearchCost(eResearch));
					logSASGameRecord("GAME_RECORD_ACTION turn=%d type=RESEARCH_TARGET_CHANGED player=%d team=%d reason=%s oldTech=%s oldTeamProgress=%d oldCost=%d oldTeamResearchersAfter=%d newTech=%s newTeamProgress=%d newCost=%d",
						GC.getGame().getGameTurn(), ePlayer, eTeam, getSASResearchTargetChangeCause(kPrevious.ePendingCause),
						getSASGameRecordTechType(eOldResearch), iOldProgress, iOldCost, iOldTeamResearchersAfter,
						getSASGameRecordTechType(eResearch), iNewProgress, iNewCost);
				}
			}
		}
		kPrevious.bValid = true;
		kPrevious.eTeam = eTeam;
		kPrevious.eTech = eResearch;
		kPrevious.iTechCount = (eResearch == NO_TECH ? 0 : kTeam.getTechCount(eResearch));
		// <!-- custom: Any tagged cause belongs only to mutations observed since the previous player-turn boundary.
		// If no invested-tech redirection resulted, discard it here so it cannot be misattributed to a later unrelated switch. (ChatGPT-5.6-Sol) -->
		kPrevious.ePendingCause = RESEARCH_TARGET_CHANGE_UNKNOWN;
	}
}


static void countSASGameRecordVisibleEnemiesNearPlot(CvPlot const& kCenter, PlayerTypes ePlayer, int iRange, int& iVisibleEnemies, int& iVisibleCombatEnemies, CvUnit const*& pNearestEnemy, int& iNearestEnemyDistance)
{
	iVisibleEnemies = 0;
	iVisibleCombatEnemies = 0;
	pNearestEnemy = NULL;
	iNearestEnemyDistance = -1;
	if (ePlayer == NO_PLAYER)
		return;
	TeamTypes const eTeam = GET_PLAYER(ePlayer).getTeam();
	for (int iDX = -iRange; iDX <= iRange; iDX++)
	{
		for (int iDY = -iRange; iDY <= iRange; iDY++)
		{
			CvPlot const* pLoopPlot = plotXY(kCenter.getX(), kCenter.getY(), iDX, iDY);
			if (pLoopPlot == NULL || !pLoopPlot->isVisible(eTeam, false))
				continue;
			for (CLLNode<IDInfo> const* pUnitNode = pLoopPlot->headUnitNode(); pUnitNode != NULL; pUnitNode = pLoopPlot->nextUnitNode(pUnitNode))
			{
				CvUnit const* pLoopUnit = ::getUnit(pUnitNode->m_data);
				// <!-- custom: isEnemy resolves plot-sensitive combat ownership and always-hostile rules, so testing every nearby unit against the center plot could misclassify the diagnostic.
				// Use the unit's actual loop plot. See KI#374. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				if (pLoopUnit == NULL || !pLoopUnit->isEnemy(eTeam, *pLoopPlot) || pLoopUnit->isInvisible(eTeam, false))
					continue;
				iVisibleEnemies++;
				if (pLoopUnit->baseCombatStr() > 0 || pLoopUnit->canAttack())
					iVisibleCombatEnemies++;
				int const iDistance = plotDistance(kCenter.getX(), kCenter.getY(), pLoopPlot->getX(), pLoopPlot->getY());
				if (iNearestEnemyDistance < 0 || iDistance < iNearestEnemyDistance)
				{
					iNearestEnemyDistance = iDistance;
					pNearestEnemy = pLoopUnit;
				}
			}
		}
	}
}

static void logSASGameRecordCityUnits(CvCity const& kCity, char const* szReason)
{
	SASGameRecordPlotUnitCounts kCounts;
	collectSASGameRecordPlotUnitCounts(kCity.getPlot(), kCity.getOwner(), kCounts);
	int iVisibleEnemies = 0;
	int iVisibleCombatEnemies = 0;
	int iNearestEnemyDistance = -1;
	CvUnit const* pNearestEnemy = NULL;
	countSASGameRecordVisibleEnemiesNearPlot(kCity.getPlot(), kCity.getOwner(), 2, iVisibleEnemies, iVisibleCombatEnemies, pNearestEnemy, iNearestEnemyDistance);
	CvCity const* pNearestOtherOwnCity = NULL;
	int iNearestOtherOwnCityDistance = -1;
	int iCityLoop = 0;
	for (CvCity const* pLoopCity = GET_PLAYER(kCity.getOwner()).firstCity(&iCityLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(kCity.getOwner()).nextCity(&iCityLoop))
	{
		if (pLoopCity->getID() == kCity.getID())
			continue;
		int const iDistance = plotDistance(kCity.getX(), kCity.getY(), pLoopCity->getX(), pLoopCity->getY());
		if (iNearestOtherOwnCityDistance < 0 || iDistance < iNearestOtherOwnCityDistance)
		{
			iNearestOtherOwnCityDistance = iDistance;
			pNearestOtherOwnCity = pLoopCity;
		}
	}
	logSASGameRecord("GAME_RECORD_CITY_UNITS turn=%d reason=%s player=%d cityId=%d city=%S x=%d y=%d pop=%d ownerUnits=%d militaryUnits=%d civilianUnits=%d defenders=%d healthyDefenders=%d woundedDefenders=%d settlers=%d workers=%d attackers=%d bestDefenderId=%d bestDefenderUnit=%s bestDefenderAI=%s bestDefenderDamage=%d visibleEnemiesR2=%d visibleCombatEnemiesR2=%d nearestEnemyPlayer=%d nearestEnemyUnit=%s nearestEnemyDist=%d nearestOtherOwnCityId=%d nearestOtherOwnCity=%S nearestOtherOwnCityDistance=%d",
		GC.getGame().getGameTurn(), szReason, kCity.getOwner(), kCity.getID(), getSASGameRecordQuotedCityName(&kCity).GetCString(),
		kCity.getX(), kCity.getY(), kCity.getPopulation(), kCounts.iUnits, kCounts.iMilitaryUnits, kCounts.iCivilianUnits,
		kCounts.iDefenders, kCounts.iHealthyDefenders, kCounts.iWoundedDefenders, kCounts.iSettlers, kCounts.iWorkers, kCounts.iAttackers,
		(kCounts.pBestDefender == NULL ? -1 : kCounts.pBestDefender->getID()),
		(kCounts.pBestDefender == NULL ? "-" : getSASGameRecordUnitType(kCounts.pBestDefender->getUnitType())),
		(kCounts.pBestDefender == NULL ? "-" : getSASGameRecordUnitAIType(kCounts.pBestDefender->AI_getUnitAIType())),
		(kCounts.pBestDefender == NULL ? -1 : kCounts.pBestDefender->getDamage()), iVisibleEnemies, iVisibleCombatEnemies,
		(pNearestEnemy == NULL ? -1 : pNearestEnemy->getOwner()),
		(pNearestEnemy == NULL ? "-" : getSASGameRecordUnitType(pNearestEnemy->getUnitType())), iNearestEnemyDistance,
		(pNearestOtherOwnCity == NULL ? -1 : pNearestOtherOwnCity->getID()),
		getSASGameRecordQuotedCityName(pNearestOtherOwnCity).GetCString(), iNearestOtherOwnCityDistance);
}

static bool logSASGameRecordSettlerCombatForPlot(CvUnit const* pWinner, CvUnit const* pLoser, CvPlot const* pPlot, PlayerTypes eSettlerOwner, bool bLoserWasSettler, bool bWinnerWasSettler)
{
	if (pWinner == NULL || pLoser == NULL || pPlot == NULL || eSettlerOwner == NO_PLAYER)
		return false;
	SASGameRecordPlotUnitCounts kCounts;
	collectSASGameRecordPlotUnitCounts(*pPlot, eSettlerOwner, kCounts);
	if (kCounts.iSettlers <= 0 && !bLoserWasSettler && !bWinnerWasSettler)
		return false;
	CvUnit const* pSettler = (bLoserWasSettler ? pLoser : (bWinnerWasSettler ? pWinner : kCounts.pFirstSettler));
	CvSelectionGroup const* pSettlerGroup = (pSettler == NULL ? NULL : pSettler->getGroup());
	int iGroupUnits = 0;
	int iGroupDefenders = 0;
	int iGroupSettlers = 0;
	if (pSettlerGroup != NULL)
	{
		FOR_EACH_UNIT_IN(pLoopUnit, *pSettlerGroup)
		{
			iGroupUnits++;
			if (isSASGameRecordSettlerUnit(*pLoopUnit))
				iGroupSettlers++;
			if (pLoopUnit->canDefend(pLoopUnit->plot()))
				iGroupDefenders++;
		}
	}
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=SETTLER_GROUP_ATTACKED settlerOwner=%d settlerId=%d settlerUnit=%s x=%d y=%d cityPlot=%d winnerPlayer=%d winnerUnitId=%d winnerUnit=%s winnerAI=%s winnerBaseStr=%d winnerDamage=%d loserPlayer=%d loserUnitId=%d loserUnit=%s loserAI=%s loserBaseStr=%d loserDamage=%d loserWasSettler=%d winnerWasSettler=%d ownerUnitsOnPlot=%d militaryUnitsOnPlot=%d civilianUnitsOnPlot=%d settlersOnPlot=%d defendersOnPlot=%d healthyDefendersOnPlot=%d workersOnPlot=%d settlerGroupId=%d settlerGroupUnits=%d settlerGroupSettlers=%d settlerGroupDefenders=%d",
		GC.getGame().getGameTurn(), eSettlerOwner, (pSettler == NULL ? -1 : pSettler->getID()),
		(pSettler == NULL ? "-" : getSASGameRecordUnitType(pSettler->getUnitType())), pPlot->getX(), pPlot->getY(), pPlot->isCity(),
		pWinner->getOwner(), pWinner->getID(), getSASGameRecordUnitType(pWinner->getUnitType()),
		getSASGameRecordUnitAIType(pWinner->AI_getUnitAIType()), pWinner->baseCombatStr(), pWinner->getDamage(), pLoser->getOwner(),
		pLoser->getID(), getSASGameRecordUnitType(pLoser->getUnitType()), getSASGameRecordUnitAIType(pLoser->AI_getUnitAIType()),
		pLoser->baseCombatStr(), pLoser->getDamage(), bLoserWasSettler, bWinnerWasSettler, kCounts.iUnits, kCounts.iMilitaryUnits,
		kCounts.iCivilianUnits, kCounts.iSettlers, kCounts.iDefenders, kCounts.iHealthyDefenders, kCounts.iWorkers,
		(pSettlerGroup == NULL ? -1 : pSettlerGroup->getID()), iGroupUnits, iGroupSettlers, iGroupDefenders);
	return true;
}

// <!-- custom: Add the actual battle target for Settler-group context.
// Using the losing unit's plot falsely treated a failed attack launched from a Settler stack as an attack against that stack. See KI#377. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
static void logSASGameRecordSettlerCombatIfNeeded(CvUnit const* pWinner, CvUnit const* pLoser, CvPlot const* pBattlePlot)
{
	if (pWinner == NULL || pLoser == NULL || pBattlePlot == NULL)
		return;
	bool const bLoserWasSettler = isSASGameRecordSettlerUnit(*pLoser);
	bool const bWinnerWasSettler = isSASGameRecordSettlerUnit(*pWinner);
	if (bLoserWasSettler && logSASGameRecordSettlerCombatForPlot(pWinner, pLoser, pBattlePlot, pLoser->getOwner(), true, bWinnerWasSettler))
		return;
	if (bWinnerWasSettler && logSASGameRecordSettlerCombatForPlot(pWinner, pLoser, pBattlePlot, pWinner->getOwner(), bLoserWasSettler, true))
		return;
	if (logSASGameRecordSettlerCombatForPlot(pWinner, pLoser, pBattlePlot, pLoser->getOwner(), false, false))
		return;
	logSASGameRecordSettlerCombatForPlot(pWinner, pLoser, pBattlePlot, pWinner->getOwner(), false, false);
}

// <!-- custom: GAME_RECORD_ACTION is narrower than a generic row: it records chronological gameplay happenings such as techs, city ownership, war state, Great People, unit upgrades, and victory.
// Do not rename this to GAME_RECORD_ROW; "row" is too generic because every log line is already a row. This keeps the row type useful without using "event", which can be confused with Civ4 EventInfo/random events. (GPT-5.5) -->
// <!-- custom: Ordinary unit-completion hooks do not see animals and other Barbarian units created directly from fog.
// Record those explicit spawn sites without instrumenting every unrelated CvPlayer::initUnit caller. (GPT-5.6-Sol) -->
void logSASGameRecordBarbarianSpawn(CvUnit const* pUnit, char const* szCause)
{
	if (pUnit == NULL || pUnit->getOwner() != BARBARIAN_PLAYER)
		return;
	CvPlot const* pPlot = pUnit->plot();
	int iRevealedMajorTeams = 0;
	int iVisibleMajorTeams = 0;
	PlayerTypes eNearestCivilizationPlayer = NO_PLAYER;
	int iNearestCivilizationCityId = -1;
	int iNearestCivilizationCityDistance = -1;
	if (pPlot != NULL)
	{
		for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
		{
			TeamTypes const eLoopTeam = (TeamTypes)iTeam;
			CvTeam const& kLoopTeam = GET_TEAM(eLoopTeam);
			if (!kLoopTeam.isAlive() || kLoopTeam.isBarbarian() || kLoopTeam.isMinorCiv())
				continue;
			if (pPlot->isRevealed(eLoopTeam, false)) iRevealedMajorTeams++;
			if (pPlot->isVisible(eLoopTeam, false)) iVisibleMajorTeams++;
		}
		for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
		{
			PlayerTypes const eLoopPlayer = (PlayerTypes)iPlayer;
			CvPlayer const& kLoopPlayer = GET_PLAYER(eLoopPlayer);
			if (!kLoopPlayer.isAlive() || kLoopPlayer.isBarbarian() || kLoopPlayer.isMinorCiv())
				continue;
			int iCityLoop = 0;
			for (CvCity const* pLoopCity = kLoopPlayer.firstCity(&iCityLoop); pLoopCity != NULL; pLoopCity = kLoopPlayer.nextCity(&iCityLoop))
			{
				int const iDistance = plotDistance(pPlot->getX(), pPlot->getY(), pLoopCity->getX(), pLoopCity->getY());
				if (iNearestCivilizationCityDistance < 0 || iDistance < iNearestCivilizationCityDistance)
				{
					eNearestCivilizationPlayer = eLoopPlayer;
					iNearestCivilizationCityId = pLoopCity->getID();
					iNearestCivilizationCityDistance = iDistance;
				}
			}
		}
	}
	// <!-- custom: Spawn context records whether the new barbarian appeared in known/visible space, its terrain/feature, and its nearest major-civilization city without rerunning spawn eligibility or pathfinding.
	// This complements later knowledge-limited per-player pressure snapshots. (ChatGPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=BARBARIAN_UNIT_SPAWNED cause=%s unitId=%d unit=%s unitAI=%s x=%d y=%d area=%d plotOwner=%d terrain=%s feature=%s water=%d hills=%d cargo=%d transportId=%d plotRevealedMajorTeams=%d plotVisibleMajorTeams=%d nearestCivilizationPlayer=%d nearestCivilizationCityId=%d nearestCivilizationCityDistance=%d",
			GC.getGame().getGameTurn(), szCause, pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()), getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()),
			pUnit->getX(), pUnit->getY(), pPlot == NULL ? -1 : pPlot->getArea().getID(),
			pPlot == NULL ? NO_PLAYER : pPlot->getOwner(),
			pPlot == NULL ? "-" : getSASGameRecordTerrainType(pPlot->getTerrainType()),
			pPlot == NULL ? "-" : getSASGameRecordFeatureType(pPlot->getFeatureType()),
			pPlot == NULL ? -1 : (pPlot->isWater() ? 1 : 0), pPlot == NULL ? -1 : (pPlot->isHills() ? 1 : 0),
			pUnit->isCargo(), pUnit->getTransportUnit() == NULL ? -1 : pUnit->getTransportUnit()->getID(),
			iRevealedMajorTeams, iVisibleMajorTeams, eNearestCivilizationPlayer, iNearestCivilizationCityId, iNearestCivilizationCityDistance);
}

static CvString getSASGameRecordGoodyUnits(std::vector<CvUnit const*> const& apUnits, bool bIncludePromotions)
{
	CvString szUnits;
	for (size_t iI = 0; iI < apUnits.size(); iI++)
	{
		CvUnit const* pUnit = apUnits[iI];
		if (pUnit == NULL)
			continue;
		CvString szPromotions;
		if (bIncludePromotions)
		{
			FOR_EACH_ENUM(Promotion)
			{
				if (!pUnit->isHasPromotion(eLoopPromotion))
					continue;
				if (!szPromotions.empty())
					szPromotions += "+";
				szPromotions += getSASGameRecordPromotionType(eLoopPromotion);
			}
		}
		CvString szItem;
		if (szPromotions.empty())
			szItem.Format("%s%s:%d@(%d,%d)", szUnits.empty() ? "" : ",", getSASGameRecordUnitType(pUnit->getUnitType()), pUnit->getID(), pUnit->getX(), pUnit->getY());
		else szItem.Format("%s%s:%d@(%d,%d)[%s]", szUnits.empty() ? "" : ",", getSASGameRecordUnitType(pUnit->getUnitType()), pUnit->getID(), pUnit->getX(), pUnit->getY(), szPromotions.GetCString());
		szUnits += szItem;
	}
	return getSASDiagnosticOrDash(szUnits);
}

// <!-- custom: Log the resolved goody result rather than only the XML label. AdvCiv goodies can randomize gold/research, reveal a variable map area, upgrade free units, spawn variable hostile units, or roll a same-sign follow-up outcome.
// Generic TECH_ACQUIRED and map-revelation rows remain complementary chronology; this rare level-2 row ties those downstream effects back to the hut that caused them. (ChatGPT-5.6-Sol) -->
void logSASGameRecordGoodyReceived(PlayerTypes ePlayer, CvPlot const* pPlot, CvUnit const* pTriggerUnit, GoodyTypes eGoody, SASGameRecordGoodyResult const& kResult)
{
	if (pPlot == NULL || ePlayer < 0 || ePlayer >= MAX_PLAYERS || eGoody == NO_GOODY)
		return;
	int const iTechProgressAdded = (kResult.iTechProgressBefore < 0 || kResult.iTechProgressAfter < 0 ? -1 : kResult.iTechProgressAfter - kResult.iTechProgressBefore);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=GOODY_RECEIVED player=%d team=%d x=%d y=%d area=%d triggerUnitId=%d triggerUnit=%s goody=%s bad=%d followup=%d upgradeRoll=%d upgradeApplied=%d additionalOutcomeAttempted=%d gold=%d newlyRevealedPlots=%d experienceGained=%d damageHealed=%d tech=%s techRewardValue=%d techStoredProgressBefore=%d techStoredProgressAdded=%d techStoredProgressAfter=%d techCost=%d techCompleted=%d freeUnitCount=%d freePromotionsGranted=%d freeUnits=%s barbarianUnitCount=%d barbarianUnits=%s",
		GC.getGame().getGameTurn(), ePlayer, GET_PLAYER(ePlayer).getTeam(), pPlot->getX(), pPlot->getY(), pPlot->getArea().getID(),
		pTriggerUnit == NULL ? -1 : pTriggerUnit->getID(),
		pTriggerUnit == NULL ? "-" : getSASGameRecordUnitType(pTriggerUnit->getUnitType()), getSASGameRecordGoodyType(eGoody),
		GC.getInfo(eGoody).isBad(), kResult.bFollowupOutcome, kResult.bUpgradeRoll, kResult.bUpgradeApplied,
		kResult.bAdditionalOutcomeAttempted, kResult.iGold, kResult.iNewlyRevealedPlots, kResult.iExperienceGained, kResult.iDamageHealed,
		getSASGameRecordTechType(kResult.eTech), kResult.iTechRewardValue, kResult.iTechProgressBefore, iTechProgressAdded,
		kResult.iTechProgressAfter, kResult.iTechCost, kResult.bTechCompleted, (int)kResult.apFreeUnits.size(),
		kResult.iFreePromotionsGranted, getSASGameRecordGoodyUnits(kResult.apFreeUnits, true).GetCString(),
		(int)kResult.apBarbarianUnits.size(), getSASGameRecordGoodyUnits(kResult.apBarbarianUnits, false).GetCString());
}

// <!-- custom: A hut can exhaust NUM_DO_GOODY_ATTEMPTS without finding an eligible result.
// Preserve that rare factual no-outcome boundary so native DLL-resolved hut removals remain explainable, including failed AdvCiv follow-up rolls. (ChatGPT-5.6-Sol) -->
void logSASGameRecordGoodyNoOutcome(PlayerTypes ePlayer, CvPlot const* pPlot, CvUnit const* pTriggerUnit, GoodyTypes eTaboo, int iAttempts)
{
	if (pPlot == NULL || ePlayer < 0 || ePlayer >= MAX_PLAYERS)
		return;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=GOODY_NO_OUTCOME player=%d team=%d x=%d y=%d area=%d triggerUnitId=%d triggerUnit=%s followup=%d taboo=%s attempts=%d",
		GC.getGame().getGameTurn(), ePlayer, GET_PLAYER(ePlayer).getTeam(), pPlot->getX(), pPlot->getY(), pPlot->getArea().getID(),
		pTriggerUnit == NULL ? -1 : pTriggerUnit->getID(),
		pTriggerUnit == NULL ? "-" : getSASGameRecordUnitType(pTriggerUnit->getUnitType()), eTaboo != NO_GOODY,
		getSASGameRecordGoodyType(eTaboo), iAttempts);
}

// <!-- custom: Keep random-event narration compact: summarize only broad EventInfo effect families and gameplay-relevant Python hooks, not every static XML magnitude or AI candidate value.
// These helpers run only from already level-2-gated lifecycle loggers. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
static CvString getSASGameRecordRandomEventEffects(CvEventInfo const& kEvent)
{
	CvString szEffects;
	if (kEvent.getGold() != 0 || kEvent.getRandomGold() != 0 || kEvent.getTechCostPercent() != 0)
		appendSASGameRecordType(szEffects, "GOLD");
	if (kEvent.getTechCostPercent() != 0)
		appendSASGameRecordType(szEffects, "TECH_COST");
	if (kEvent.getEspionagePoints() != 0)
		appendSASGameRecordType(szEffects, "ESPIONAGE_POINTS");
	if (kEvent.getTechPercent() != 0)
		appendSASGameRecordType(szEffects, "TECH_PROGRESS");
	if (kEvent.isGoldenAge())
		appendSASGameRecordType(szEffects, "GOLDEN_AGE");
	if (kEvent.getFreeUnitSupport() != 0 || kEvent.getInflationModifier() != 0 || kEvent.getSpaceProductionModifier() != 0)
		appendSASGameRecordType(szEffects, "PLAYER_MODIFIER");
	if (kEvent.isDeclareWar())
		appendSASGameRecordType(szEffects, "DECLARE_WAR");
	if (kEvent.getBonusGift() != NO_BONUS)
		appendSASGameRecordType(szEffects, "BONUS_GIFT");
	if (kEvent.getHappy() != 0)
		appendSASGameRecordType(szEffects, "HAPPINESS");
	if (kEvent.getHealth() != 0)
		appendSASGameRecordType(szEffects, "HEALTH");
	if (kEvent.getHurryAnger() != 0 || kEvent.getHappyTurns() != 0)
		appendSASGameRecordType(szEffects, "TEMPORARY_MOOD");
	if (kEvent.getFood() != 0 || kEvent.getFoodPercent() != 0)
		appendSASGameRecordType(szEffects, "FOOD");
	if (kEvent.getPopulationChange() != 0)
		appendSASGameRecordType(szEffects, "POPULATION");
	if (kEvent.getRevoltTurns() > 0)
		appendSASGameRecordType(szEffects, "REVOLT");
	if (kEvent.getCulture() != 0)
		appendSASGameRecordType(szEffects, "CULTURE");
	if (kEvent.getMaxPillage() > 0)
		appendSASGameRecordType(szEffects, "PILLAGE");
	bool bFreeSpecialists = false;
	FOR_EACH_ENUM(Specialist)
	{
		if (kEvent.getFreeSpecialistCount(eLoopSpecialist) != 0)
		{
			bFreeSpecialists = true;
			break;
		}
	}
	if (bFreeSpecialists)
		appendSASGameRecordType(szEffects, "FREE_SPECIALISTS");
	if (kEvent.getUnitClass() != NO_UNITCLASS && kEvent.getNumUnits() > 0)
		appendSASGameRecordType(szEffects, "FREE_UNITS");
	if (kEvent.getBuildingClass() != NO_BUILDINGCLASS && kEvent.getBuildingChange() != 0)
		appendSASGameRecordType(szEffects, "BUILDING_CHANGE");
	if (kEvent.getBuildingYieldChange().isAnyNonDefault() || kEvent.getBuildingCommerceChange().isAnyNonDefault() ||
		kEvent.getBuildingHappyChange().isAnyNonDefault() || kEvent.getBuildingHealthChange().isAnyNonDefault())
	{
		appendSASGameRecordType(szEffects, "BUILDING_MODIFIER");
	}
	bool bPlotYield = false;
	FOR_EACH_ENUM(Yield)
	{
		if (kEvent.getPlotExtraYield(eLoopYield) != 0)
		{
			bPlotYield = true;
			break;
		}
	}
	if (kEvent.getFeatureChange() != 0 || kEvent.getImprovementChange() != 0 || kEvent.getBonusChange() != 0 || kEvent.getRouteChange() != 0 || bPlotYield)
		appendSASGameRecordType(szEffects, "PLOT_CHANGE");
	bool bFreePromotions = false;
	FOR_EACH_ENUM(UnitCombat)
	{
		if (kEvent.getUnitCombatPromotion(eLoopUnitCombat) != NO_PROMOTION)
		{
			bFreePromotions = true;
			break;
		}
	}
	if (!bFreePromotions)
	{
		FOR_EACH_ENUM(UnitClass)
		{
			if (kEvent.getUnitClassPromotion(eLoopUnitClass) != NO_PROMOTION)
			{
				bFreePromotions = true;
				break;
			}
		}
	}
	if (bFreePromotions)
		appendSASGameRecordType(szEffects, "FREE_PROMOTION");
	if (kEvent.getBonusRevealed() != NO_BONUS)
		appendSASGameRecordType(szEffects, "BONUS_REVEAL");
	if (kEvent.getConvertOwnCities() > 0 || kEvent.getConvertOtherCities() > 0)
		appendSASGameRecordType(szEffects, "RELIGION_SPREAD");
	if (kEvent.getOurAttitudeModifier() != 0 || kEvent.getAttitudeModifier() != 0 || kEvent.getTheirEnemyAttitudeModifier() != 0)
		appendSASGameRecordType(szEffects, "DIPLO_ATTITUDE");
	if (kEvent.hasUnitLocalEffect())
		appendSASGameRecordType(szEffects, "UNIT_LOCAL");
	bool bFollowup = false;
	bool bClear = false;
	FOR_EACH_ENUM(Event)
	{
		if (kEvent.getAdditionalEventChance(eLoopEvent) > 0 || kEvent.getAdditionalEventTime(eLoopEvent) != 0)
			bFollowup = true;
		if (kEvent.getClearEventChance(eLoopEvent) > 0)
			bClear = true;
	}
	if (bFollowup)
		appendSASGameRecordType(szEffects, "FOLLOWUP_EVENT");
	if (bClear)
		appendSASGameRecordType(szEffects, "CLEAR_EVENT");
	char const* szPythonCallback = kEvent.getPythonCallback();
	if (szPythonCallback != NULL && szPythonCallback[0] != '\0')
		appendSASGameRecordType(szEffects, "PYTHON_CALLBACK");
	return getSASDiagnosticOrDash(szEffects);
}

static char const* getSASGameRecordRandomEventNormalSelectionMode(CvEventTriggerInfo const& kTrigger)
{
	// <!-- custom: Mirror CvPlayer::doEvents/getEventTriggerWeight semantics exactly. Weight -1 is auto-fired whenever eligible.
	// Values below -1 are excluded from the ordinary weighted/forced loop and are reserved for direct/special firing (e.g. the -2 Partisans trigger), while weight 0 can only reach the delivery boundary through a direct/special caller.
	// The row describes normal engine selection semantics, not an unverifiable claim about this instance's actual caller. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	int const iWeight = kTrigger.getProbability();
	if (iWeight == -1)
		return "FORCED_WHEN_ELIGIBLE";
	if (iWeight < -1)
		return "DIRECT_OR_SPECIAL_ONLY";
	if (iWeight == 0)
		return "ZERO_WEIGHT_DIRECT_ONLY";
	return "WEIGHTED_RANDOM";
}

static CvString getSASGameRecordRandomEventTriggerPrereqs(CvEventTriggerInfo const& kTrigger)
{
	CvString szPrereqs;
	for (int i = 0; i < kTrigger.getNumPrereqEvents(); i++)
		appendSASGameRecordType(szPrereqs, getSASGameRecordEventType((EventTypes)kTrigger.getPrereqEvent(i)));
	return getSASDiagnosticOrDash(szPrereqs);
}

static CvString getSASGameRecordRandomEventTriggerPythonHooks(CvEventTriggerInfo const& kTrigger)
{
	CvString szHooks;
	if (kTrigger.getPythonCallback() != NULL && kTrigger.getPythonCallback()[0] != '\0') appendSASGameRecordType(szHooks, "CALLBACK");
	if (kTrigger.getPythonCanDo() != NULL && kTrigger.getPythonCanDo()[0] != '\0') appendSASGameRecordType(szHooks, "CAN_DO");
	if (kTrigger.getPythonCanDoCity() != NULL && kTrigger.getPythonCanDoCity()[0] != '\0') appendSASGameRecordType(szHooks, "CAN_DO_CITY");
	if (kTrigger.getPythonCanDoUnit() != NULL && kTrigger.getPythonCanDoUnit()[0] != '\0') appendSASGameRecordType(szHooks, "CAN_DO_UNIT");
	return getSASDiagnosticOrDash(szHooks);
}

static CvString getSASGameRecordRandomEventPythonHooks(CvEventInfo const& kEvent)
{
	CvString szHooks;
	if (kEvent.getPythonCallback() != NULL && kEvent.getPythonCallback()[0] != '\0') appendSASGameRecordType(szHooks, "CALLBACK");
	if (kEvent.getPythonCanDo() != NULL && kEvent.getPythonCanDo()[0] != '\0') appendSASGameRecordType(szHooks, "CAN_DO");
	if (kEvent.getPythonExpireCheck() != NULL && kEvent.getPythonExpireCheck()[0] != '\0') appendSASGameRecordType(szHooks, "EXPIRE_CHECK");
	return getSASDiagnosticOrDash(szHooks);
}

// <!-- custom: Civ4 EventInfo/random-event rows use dedicated RANDOM_EVENT_* row names so they cannot be confused with generic GAME_RECORD_ACTION chronology or Python/CvEventReporter callbacks.
// Target existence is sampled only at actual trigger/reply boundaries. A -1 existence value means that no concrete target of that kind was stored; 0 means an ID/coordinate was stored but no longer resolves, which is important for stale-popup diagnostics such as KI#809/KI#810. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
struct SASGameRecordRandomEventTargets
{
	SASGameRecordRandomEventTargets(CvPlayer const& kPlayer, EventTriggeredData const* pData, EventTypes eEvent)
	: iCityId(-1), iCityExists(-1), eOtherPlayer(NO_PLAYER), iOtherPlayerAlive(-1), iOtherCityId(-1), iOtherCityExists(-1),
	  iUnitId(-1), iUnitExists(-1), iUnitCanApply(-1), szUnit("-"), iPlotX(INVALID_PLOT_COORD), iPlotY(INVALID_PLOT_COORD), iPlotExists(-1), iPlotOwner(NO_PLAYER),
	  eReligion(NO_RELIGION), eCorporation(NO_CORPORATION), eBuilding(NO_BUILDING), iBuildingPresentInCity(-1)
	{
		if (pData == NULL)
			return;
		iCityId = pData->m_iCityId;
		if (iCityId >= 0)
			iCityExists = (kPlayer.getCity(iCityId) != NULL);
		eOtherPlayer = pData->m_eOtherPlayer;
		if (eOtherPlayer != NO_PLAYER && eOtherPlayer >= 0 && eOtherPlayer < MAX_PLAYERS)
		{
			iOtherPlayerAlive = GET_PLAYER(eOtherPlayer).isAlive();
			iOtherCityId = pData->m_iOtherPlayerCityId;
			if (iOtherCityId >= 0)
				iOtherCityExists = (GET_PLAYER(eOtherPlayer).getCity(iOtherCityId) != NULL);
		}
		else iOtherCityId = pData->m_iOtherPlayerCityId;
		iUnitId = pData->m_iUnitId;
		bool const bHasEvent = (eEvent != NO_EVENT);
		bool const bRequiresConcreteUnit = (bHasEvent && GC.getInfo(eEvent).hasUnitLocalEffect());
		if (iUnitId >= 0)
		{
			CvUnit const* pUnit = kPlayer.getUnit(iUnitId);
			iUnitExists = (pUnit != NULL);
			if (pUnit != NULL)
			{
				szUnit = getSASGameRecordUnitType(pUnit->getUnitType());
				if (bHasEvent)
					iUnitCanApply = pUnit->canApplyEvent(eEvent);
			}
			else if (bHasEvent) iUnitCanApply = 0;
		}
		else if (bRequiresConcreteUnit)
		{
			iUnitExists = 0;
			iUnitCanApply = 0;
		}
		iPlotX = pData->m_iPlotX;
		iPlotY = pData->m_iPlotY;
		if (iPlotX != INVALID_PLOT_COORD && iPlotY != INVALID_PLOT_COORD)
		{
			CvPlot const* pPlot = GC.getMap().plot(iPlotX, iPlotY);
			iPlotExists = (pPlot != NULL);
			if (pPlot != NULL)
				iPlotOwner = pPlot->getOwner();
		}
		eReligion = pData->m_eReligion;
		eCorporation = pData->m_eCorporation;
		eBuilding = pData->m_eBuilding;
		if (eBuilding != NO_BUILDING)
		{
			CvCity const* pCity = (iCityId < 0 ? NULL : kPlayer.getCity(iCityId));
			iBuildingPresentInCity = (pCity != NULL && pCity->getNumRealBuilding(eBuilding) > 0);
		}
	}
	int iCityId;
	int iCityExists;
	PlayerTypes eOtherPlayer;
	int iOtherPlayerAlive;
	int iOtherCityId;
	int iOtherCityExists;
	int iUnitId;
	int iUnitExists;
	int iUnitCanApply;
	char const* szUnit;
	int iPlotX;
	int iPlotY;
	int iPlotExists;
	PlayerTypes iPlotOwner;
	ReligionTypes eReligion;
	CorporationTypes eCorporation;
	BuildingTypes eBuilding;
	int iBuildingPresentInCity;
};

static char const* getSASGameRecordRandomEventApplyPath(CvPlayer const& kPlayer, EventTypes eEvent, int iTriggeredId, bool bUpdateTrigger, int& iCountdownDueTurn)
{
	iCountdownDueTurn = -1;
	if (!bUpdateTrigger)
		return "ADDITIONAL_IMMEDIATE";
	EventTriggeredData const* pCountdown = kPlayer.getEventCountdown(eEvent);
	if (pCountdown != NULL && pCountdown->m_iId == iTriggeredId && GC.getGame().getGameTurn() >= pCountdown->m_iTurn)
	{
		iCountdownDueTurn = pCountdown->m_iTurn;
		return "ADDITIONAL_COUNTDOWN";
	}
	return "PRIMARY_REPLY_OR_DIRECT";
}

// <!-- custom: Record only a trigger instance that has actually reached CvPlayer::trigger after target construction/Python trigger mutation and weighted selection.
// Candidate getEventTriggerWeight/initTriggeredData searches remain intentionally absent; the row describes the concrete stored instance that will be delivered to a human popup or immediately resolved by AI. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordRandomEventTriggered(CvPlayer const& kPlayer, EventTriggeredData const& kTriggeredData, char const* szDeliveryPath)
{
	CvEventTriggerInfo const& kTrigger = GC.getInfo(kTriggeredData.m_eTrigger);
	SASGameRecordRandomEventTargets const kTargets(kPlayer, &kTriggeredData, NO_EVENT);
	logSASGameRecord("GAME_RECORD_RANDOM_EVENT_TRIGGERED turn=%d player=%d team=%d triggeredId=%d trigger=%s delivery=%s triggerTurn=%d normalSelectionMode=%s recurring=%d global=%d teamScope=%d singlePlayer=%d plotEvent=%d triggerFiredBeforeDelivery=%d prereqEvents=%s prereqEventCity=%d pythonHooks=%s cityId=%d cityExists=%d otherPlayer=%d otherPlayerAlive=%d otherCityId=%d otherCityExists=%d unitId=%d unitExists=%d unit=%s plot=%d,%d plotExists=%d plotOwner=%d religion=%s corporation=%s building=%s buildingPresentInCity=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), kTriggeredData.m_iId,
		getSASGameRecordEventTriggerType(kTriggeredData.m_eTrigger), szDeliveryPath, kTriggeredData.m_iTurn,
		getSASGameRecordRandomEventNormalSelectionMode(kTrigger), kTrigger.isRecurring(), kTrigger.isGlobal(), kTrigger.isTeam(),
		kTrigger.isSinglePlayer(), kTrigger.isPlotEventTrigger(), kPlayer.isTriggerFired(kTriggeredData.m_eTrigger),
		getSASGameRecordRandomEventTriggerPrereqs(kTrigger).GetCString(), kTrigger.isPrereqEventCity(),
		getSASGameRecordRandomEventTriggerPythonHooks(kTrigger).GetCString(), kTargets.iCityId, kTargets.iCityExists, kTargets.eOtherPlayer,
		kTargets.iOtherPlayerAlive, kTargets.iOtherCityId, kTargets.iOtherCityExists, kTargets.iUnitId, kTargets.iUnitExists,
		kTargets.szUnit, kTargets.iPlotX, kTargets.iPlotY, kTargets.iPlotExists, kTargets.iPlotOwner,
		getSASGameRecordReligionType(kTargets.eReligion), getSASGameRecordCorporationType(kTargets.eCorporation),
		getSASGameRecordBuildingType(kTargets.eBuilding), kTargets.iBuildingPresentInCity);
}

// <!-- custom: An AI can reach the real trigger-delivery boundary yet find no currently legal EventInfo after its normal canDoEvent/AI_eventValue search.
// Record only that final NO_EVENT resolution, never the candidate values/search itself, so a delivered trigger cannot silently disappear from lifecycle history. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordRandomEventNoSelection(CvPlayer const& kPlayer, EventTriggeredData const& kTriggeredData, char const* szResolution)
{
	logSASGameRecord("GAME_RECORD_RANDOM_EVENT_NO_SELECTION turn=%d player=%d team=%d triggeredId=%d trigger=%s resolution=%s triggerTurn=%d ageTurns=%d",
			GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), kTriggeredData.m_iId, getSASGameRecordEventTriggerType(kTriggeredData.m_eTrigger), szResolution,
			kTriggeredData.m_iTurn, GC.getGame().getGameTurn() - kTriggeredData.m_iTurn);
}

// <!-- custom: This is the synchronized/random-event transaction diagnostic rather than a static EventInfo dump.
// For rejected replies, the row is emitted before triggered-data deletion; ACCEPTED is emitted immediately after setEventOccured and before downstream payload/RNG effects, while specialized result rows preserve realized outcomes.
// That boundary makes KI#809 visible as requiresConcreteUnit=1 + unitExists=0 + canDoEvent=1, and KI#810 as REJECTED_CAN_DO with triggerFiredBefore=0/triggerFiredAfter=1 while eventOccurredAfter remains 0, without repairing either defect here. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordRandomEventApply(CvPlayer const& kPlayer, EventTypes eEvent, int iTriggeredId, EventTriggeredData const* pTriggeredData, bool bUpdateTrigger, char const* szDisposition, int iCanDoEvent, int iTriggerFiredBefore, int iEventOccurredBefore)
{
	CvEventInfo const& kEvent = GC.getInfo(eEvent);
	EventTriggerTypes const eTrigger = (pTriggeredData == NULL ? NO_EVENTTRIGGER : pTriggeredData->m_eTrigger);
	CvEventTriggerInfo const* pTrigger = (eTrigger == NO_EVENTTRIGGER ? NULL : &GC.getInfo(eTrigger));
	SASGameRecordRandomEventTargets const kTargets(kPlayer, pTriggeredData, eEvent);
	int iCountdownDueTurn = -1;
	char const* szApplyPath = getSASGameRecordRandomEventApplyPath(kPlayer, eEvent, iTriggeredId, bUpdateTrigger, iCountdownDueTurn);
	int const iTriggerTurn = (pTriggeredData == NULL ? -1 : pTriggeredData->m_iTurn);
	int const iReplyAgeTurns = (iTriggerTurn < 0 ? -1 : GC.getGame().getGameTurn() - iTriggerTurn);
	int const iTriggerFiredAfter = (eTrigger == NO_EVENTTRIGGER ? -1 : kPlayer.isTriggerFired(eTrigger));
	int const iEventOccurredAfter = (kPlayer.getEventOccured(eEvent) != NULL);
	logSASGameRecord("GAME_RECORD_RANDOM_EVENT_APPLY turn=%d player=%d team=%d triggeredId=%d trigger=%s event=%s applyPath=%s bUpdateTrigger=%d disposition=%s canDoEvent=%d triggerTurn=%d replyAgeTurns=%d countdownDueTurn=%d triggerNormalSelectionMode=%s triggerRecurring=%d triggerGlobal=%d triggerTeam=%d triggerSinglePlayer=%d triggerPlotEvent=%d triggerPrereqEvents=%s triggerPrereqEventCity=%d triggerPythonHooks=%s eventQuest=%d eventGlobal=%d eventTeam=%d eventCityEffect=%d eventOtherCityEffect=%d eventEffects=%s eventPythonHooks=%s triggerFiredBefore=%d triggerFiredAfter=%d eventOccurredBefore=%d eventOccurredAfter=%d requiresConcreteUnit=%d unitId=%d unitExists=%d unit=%s unitCanApply=%d cityId=%d cityExists=%d otherPlayer=%d otherPlayerAlive=%d otherCityId=%d otherCityExists=%d plot=%d,%d plotExists=%d plotOwner=%d religion=%s corporation=%s building=%s buildingPresentInCity=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), iTriggeredId, getSASGameRecordEventTriggerType(eTrigger),
		getSASGameRecordEventType(eEvent), szApplyPath, bUpdateTrigger, szDisposition, iCanDoEvent, iTriggerTurn, iReplyAgeTurns,
		iCountdownDueTurn, pTrigger == NULL ? "-" : getSASGameRecordRandomEventNormalSelectionMode(*pTrigger),
		pTrigger == NULL ? -1 : pTrigger->isRecurring(), pTrigger == NULL ? -1 : pTrigger->isGlobal(),
		pTrigger == NULL ? -1 : pTrigger->isTeam(), pTrigger == NULL ? -1 : pTrigger->isSinglePlayer(),
		pTrigger == NULL ? -1 : pTrigger->isPlotEventTrigger(),
		pTrigger == NULL ? "-" : getSASGameRecordRandomEventTriggerPrereqs(*pTrigger).GetCString(),
		pTrigger == NULL ? -1 : pTrigger->isPrereqEventCity(),
		pTrigger == NULL ? "-" : getSASGameRecordRandomEventTriggerPythonHooks(*pTrigger).GetCString(), kEvent.isQuest(), kEvent.isGlobal(),
		kEvent.isTeam(), kEvent.isCityEffect(), kEvent.isOtherPlayerCityEffect(), getSASGameRecordRandomEventEffects(kEvent).GetCString(),
		getSASGameRecordRandomEventPythonHooks(kEvent).GetCString(), iTriggerFiredBefore, iTriggerFiredAfter, iEventOccurredBefore,
		iEventOccurredAfter, kEvent.hasUnitLocalEffect(), kTargets.iUnitId, kTargets.iUnitExists, kTargets.szUnit, kTargets.iUnitCanApply,
		kTargets.iCityId, kTargets.iCityExists, kTargets.eOtherPlayer, kTargets.iOtherPlayerAlive, kTargets.iOtherCityId,
		kTargets.iOtherCityExists, kTargets.iPlotX, kTargets.iPlotY, kTargets.iPlotExists, kTargets.iPlotOwner,
		getSASGameRecordReligionType(kTargets.eReligion), getSASGameRecordCorporationType(kTargets.eCorporation),
		getSASGameRecordBuildingType(kTargets.eBuilding), kTargets.iBuildingPresentInCity);
}

// <!-- custom: EventInfo gold can come from fixed/random gold or a dynamically selected technology-cost percentage.
// Reuse the exact already-computed cost endpoints/result from CvPlayer::applyEvent so logging captures the realized treasury transaction without additional RNG, tech selection, or event-cost calculation. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordRandomEventGoldResult(CvPlayer const& kPlayer, EventTypes eEvent, int iTriggeredId, int iRangeLow, int iRangeHigh, int iPlayerGoldDelta, PlayerTypes eOtherPlayer, bool bGoldToPlayer)
{
	int const iOtherGoldDelta = (bGoldToPlayer && eOtherPlayer != NO_PLAYER ? -iPlayerGoldDelta : 0);
	logSASGameRecord("GAME_RECORD_RANDOM_EVENT_GOLD_RESULT turn=%d player=%d team=%d triggeredId=%d event=%s rangeLow=%d rangeHigh=%d playerGoldDelta=%d otherPlayer=%d otherGoldDelta=%d goldToPlayer=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), iTriggeredId, getSASGameRecordEventType(eEvent), iRangeLow,
		iRangeHigh, iPlayerGoldDelta, eOtherPlayer, iOtherGoldDelta, bGoldToPlayer);
}

// <!-- custom: A random EventInfo can dynamically choose a research target and apply only partial progress.
// Record the already-selected tech and actual signed beaker result after gameplay applies it; TECH_ACQUIRED remains canonical if the event completes the technology.
// No extra tech search or RNG is performed for logging. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordRandomEventTechResult(CvPlayer const& kPlayer, EventTypes eEvent, int iTriggeredId, TechTypes eTech, int iTechPercent, int iResearchBefore, int iBeakersApplied, int iResearchAfter, int iTechCost, int iCompleted)
{
	logSASGameRecord("GAME_RECORD_RANDOM_EVENT_TECH_RESULT turn=%d player=%d team=%d triggeredId=%d event=%s tech=%s techPercent=%d researchBefore=%d beakersApplied=%d researchAfter=%d techCost=%d completed=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), iTriggeredId, getSASGameRecordEventType(eEvent),
		getSASGameRecordTechType(eTech), iTechPercent, iResearchBefore, iBeakersApplied, iResearchAfter, iTechCost, iCompleted);
}

SASGameRecordRandomEventCityState::SASGameRecordRandomEventCityState(CvCity const& kCity, EventTypes eEvent) :
		iPopulation(kCity.getPopulation()), iFood(kCity.getFood()), iFoodYield(kCity.getYieldRate(YIELD_FOOD)),
		iProductionYield(kCity.getYieldRate(YIELD_PRODUCTION)), iCommerceYield(kCity.getYieldRate(YIELD_COMMERCE)),
		iGoldRate(kCity.getCommerceRate(COMMERCE_GOLD)), iResearchRate(kCity.getCommerceRate(COMMERCE_RESEARCH)),
		iCultureRate(kCity.getCommerceRate(COMMERCE_CULTURE)), iEspionageRate(kCity.getCommerceRate(COMMERCE_ESPIONAGE)),
		iOwnerCultureTimes100(kCity.getCultureTimes100(kCity.getOwner())), iOccupationTurns(kCity.getOccupationTimer()),
		iCultureUpdateTurns(kCity.getCultureUpdateTimer()), iExtraHappiness(kCity.getExtraHappiness()),
		iExtraHealth(kCity.getExtraHealth()), iHurryAngerTurns(kCity.getHurryAngerTimer()),
		iHappinessTurns(kCity.getHappinessTimer()), iAngryPopulation(kCity.angryPopulation()), iHappyLevel(kCity.happyLevel()),
		iUnhappyLevel(kCity.unhappyLevel()), iGoodHealth(kCity.goodHealth()), iBadHealth(kCity.badHealth()),
		iSpaceProductionModifier(kCity.getSpaceProductionModifier()), iFreeSpecialistInstances(kCity.getNumGreatPeople()),
		eBuilding(NO_BUILDING), iRealBuildingCount(-1)
{
	CvEventInfo const& kEvent = GC.getInfo(eEvent);
	BuildingClassTypes const eBuildingClass = (BuildingClassTypes)kEvent.getBuildingClass();
	if (eBuildingClass != NO_BUILDINGCLASS)
	{
		eBuilding = kCity.getCivilization().getBuilding(eBuildingClass);
		if (eBuilding != NO_BUILDING)
			iRealBuildingCount = kCity.getNumRealBuilding(eBuilding);
	}
}

// <!-- custom: Keep deterministic EventInfo city consequences compact and realized: record before/after state only when something actually changed.
// Include immediate yield/commerce output so building modifiers do not disappear until a later periodic snapshot; plot pillage, free units, gold and tech retain their specialized result rows. (ChatGPT-5.6-Sol) -->
void logSASGameRecordRandomEventCityResult(PlayerTypes ePlayer, PlayerTypes eAffectedPlayer, int iTriggeredId, EventTypes eEvent, char const* szScope, CvCity const& kCity, SASGameRecordRandomEventCityState const& kBefore, SASGameRecordRandomEventCityState const& kAfter)
{
	bool const bChanged = (kBefore.iPopulation != kAfter.iPopulation || kBefore.iFood != kAfter.iFood ||
			kBefore.iFoodYield != kAfter.iFoodYield || kBefore.iProductionYield != kAfter.iProductionYield ||
			kBefore.iCommerceYield != kAfter.iCommerceYield || kBefore.iGoldRate != kAfter.iGoldRate ||
			kBefore.iResearchRate != kAfter.iResearchRate || kBefore.iCultureRate != kAfter.iCultureRate ||
			kBefore.iEspionageRate != kAfter.iEspionageRate || kBefore.iOwnerCultureTimes100 != kAfter.iOwnerCultureTimes100 ||
			kBefore.iOccupationTurns != kAfter.iOccupationTurns || kBefore.iCultureUpdateTurns != kAfter.iCultureUpdateTurns ||
			kBefore.iExtraHappiness != kAfter.iExtraHappiness || kBefore.iExtraHealth != kAfter.iExtraHealth ||
			kBefore.iHurryAngerTurns != kAfter.iHurryAngerTurns || kBefore.iHappinessTurns != kAfter.iHappinessTurns ||
			kBefore.iAngryPopulation != kAfter.iAngryPopulation || kBefore.iHappyLevel != kAfter.iHappyLevel ||
			kBefore.iUnhappyLevel != kAfter.iUnhappyLevel || kBefore.iGoodHealth != kAfter.iGoodHealth ||
			kBefore.iBadHealth != kAfter.iBadHealth || kBefore.iSpaceProductionModifier != kAfter.iSpaceProductionModifier ||
			kBefore.iFreeSpecialistInstances != kAfter.iFreeSpecialistInstances || kBefore.eBuilding != kAfter.eBuilding ||
			kBefore.iRealBuildingCount != kAfter.iRealBuildingCount);
	if (!bChanged)
		return;
	logSASGameRecord("GAME_RECORD_RANDOM_EVENT_CITY_RESULT turn=%d player=%d team=%d affectedPlayer=%d affectedTeam=%d triggeredId=%d event=%s scope=%s cityId=%d city=%S x=%d y=%d populationBefore=%d populationAfter=%d populationDelta=%+d foodBefore=%d foodAfter=%d foodDelta=%+d foodYieldBefore=%d foodYieldAfter=%d productionYieldBefore=%d productionYieldAfter=%d commerceYieldBefore=%d commerceYieldAfter=%d goldRateBefore=%d goldRateAfter=%d researchRateBefore=%d researchRateAfter=%d cultureRateBefore=%d cultureRateAfter=%d espionageRateBefore=%d espionageRateAfter=%d ownerCultureTimes100Before=%d ownerCultureTimes100After=%d ownerCultureTimes100Delta=%+d occupationTurnsBefore=%d occupationTurnsAfter=%d cultureUpdateTurnsBefore=%d cultureUpdateTurnsAfter=%d extraHappinessBefore=%d extraHappinessAfter=%d extraHealthBefore=%d extraHealthAfter=%d hurryAngerTurnsBefore=%d hurryAngerTurnsAfter=%d happinessTurnsBefore=%d happinessTurnsAfter=%d angryPopulationBefore=%d angryPopulationAfter=%d happyLevelBefore=%d happyLevelAfter=%d unhappyLevelBefore=%d unhappyLevelAfter=%d goodHealthBefore=%d goodHealthAfter=%d badHealthBefore=%d badHealthAfter=%d spaceProductionModifierBefore=%d spaceProductionModifierAfter=%d freeSpecialistInstancesBefore=%d freeSpecialistInstancesAfter=%d building=%s realBuildingCountBefore=%d realBuildingCountAfter=%d",
		GC.getGame().getGameTurn(), ePlayer, (ePlayer == NO_PLAYER ? NO_TEAM : GET_PLAYER(ePlayer).getTeam()), eAffectedPlayer,
		(eAffectedPlayer == NO_PLAYER ? NO_TEAM : GET_PLAYER(eAffectedPlayer).getTeam()), iTriggeredId, getSASGameRecordEventType(eEvent),
		szScope, kCity.getID(), getSASGameRecordQuotedCityName(&kCity).GetCString(), kCity.getX(), kCity.getY(), kBefore.iPopulation,
		kAfter.iPopulation, kAfter.iPopulation - kBefore.iPopulation, kBefore.iFood, kAfter.iFood, kAfter.iFood - kBefore.iFood,
		kBefore.iFoodYield, kAfter.iFoodYield, kBefore.iProductionYield, kAfter.iProductionYield, kBefore.iCommerceYield,
		kAfter.iCommerceYield, kBefore.iGoldRate, kAfter.iGoldRate, kBefore.iResearchRate, kAfter.iResearchRate, kBefore.iCultureRate,
		kAfter.iCultureRate, kBefore.iEspionageRate, kAfter.iEspionageRate, kBefore.iOwnerCultureTimes100, kAfter.iOwnerCultureTimes100,
		kAfter.iOwnerCultureTimes100 - kBefore.iOwnerCultureTimes100, kBefore.iOccupationTurns, kAfter.iOccupationTurns,
		kBefore.iCultureUpdateTurns, kAfter.iCultureUpdateTurns, kBefore.iExtraHappiness, kAfter.iExtraHappiness, kBefore.iExtraHealth,
		kAfter.iExtraHealth, kBefore.iHurryAngerTurns, kAfter.iHurryAngerTurns, kBefore.iHappinessTurns, kAfter.iHappinessTurns,
		kBefore.iAngryPopulation, kAfter.iAngryPopulation, kBefore.iHappyLevel, kAfter.iHappyLevel, kBefore.iUnhappyLevel,
		kAfter.iUnhappyLevel, kBefore.iGoodHealth, kAfter.iGoodHealth, kBefore.iBadHealth, kAfter.iBadHealth,
		kBefore.iSpaceProductionModifier, kAfter.iSpaceProductionModifier, kBefore.iFreeSpecialistInstances, kAfter.iFreeSpecialistInstances,
		getSASGameRecordBuildingType(kAfter.eBuilding != NO_BUILDING ? kAfter.eBuilding : kBefore.eBuilding), kBefore.iRealBuildingCount,
		kAfter.iRealBuildingCount);
}


// <!-- custom: Building-modifier EventInfos can create durable latent state even when no current building/output changes.
// Keep one compact realized operation row per configured modifier: selected-city modifiers retain their stored after value, empire yield/commerce modifiers name the current-city scope, and empire happiness/health modifiers retain the resolved player-level building modifier before/after. (ChatGPT-5.6-Sol) -->
void logSASGameRecordRandomEventBuildingModifierResults(CvPlayer const& kPlayer, EventTypes eEvent, int iTriggeredId, char const* szScope, CvCity const* pCity)
{
	CvEventInfo const& kEvent = GC.getInfo(eEvent);
	bool const bCityScope = (pCity != NULL);
	PlayerTypes const eAffectedPlayer = (bCityScope ? pCity->getOwner() : kPlayer.getID());
	int const iCityId = (bCityScope ? pCity->getID() : -1);
	int const iAffectedCityCount = (bCityScope ? 1 : kPlayer.getNumCities());

	FOR_EACH_NON_DEFAULT_PAIR(kEvent.getBuildingYieldChange(), BuildingClass, YieldChangeMap)
	{
		FOR_EACH_NON_DEFAULT_PAIR(perBuildingClassVal.second, Yield, int)
		{
			BuildingTypes const eBuilding = (bCityScope ? pCity->getCivilization().getBuilding(perBuildingClassVal.first) :
					kPlayer.getCivilization().getBuilding(perBuildingClassVal.first));
			int iBefore = -1;
			int iAfter = -1;
			if (bCityScope)
			{
				iAfter = pCity->getBuildingYieldChange(perBuildingClassVal.first, perYieldVal.first);
				iBefore = iAfter - perYieldVal.second;
			}
			logSASGameRecord("GAME_RECORD_RANDOM_EVENT_BUILDING_MODIFIER_RESULT turn=%d player=%d team=%d triggeredId=%d event=%s scope=%s target=%s affectedPlayer=%d affectedTeam=%d cityId=%d affectedCityCount=%d modifier=YIELD buildingClass=%s building=%s subType=%s operation=ADD configuredValue=%+d valueBefore=%d valueAfter=%d",
					GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), iTriggeredId, getSASGameRecordEventType(eEvent), szScope,
					bCityScope ? "CITY_STORED_MODIFIER" : "CURRENT_CITIES_STORED_MODIFIER", eAffectedPlayer,
					GET_PLAYER(eAffectedPlayer).getTeam(), iCityId, iAffectedCityCount,
					GC.getInfo(perBuildingClassVal.first).getType(), getSASGameRecordBuildingType(eBuilding), GC.getInfo(perYieldVal.first).getType(),
					perYieldVal.second, iBefore, iAfter);
		}
	}
	FOR_EACH_NON_DEFAULT_PAIR(kEvent.getBuildingCommerceChange(), BuildingClass, CommerceChangeMap)
	{
		FOR_EACH_NON_DEFAULT_PAIR(perBuildingClassVal.second, Commerce, int)
		{
			BuildingTypes const eBuilding = (bCityScope ? pCity->getCivilization().getBuilding(perBuildingClassVal.first) :
					kPlayer.getCivilization().getBuilding(perBuildingClassVal.first));
			int iBefore = -1;
			int iAfter = -1;
			if (bCityScope)
			{
				iAfter = pCity->getBuildingCommerceChange(perBuildingClassVal.first, perCommerceVal.first);
				iBefore = iAfter - perCommerceVal.second;
			}
			logSASGameRecord("GAME_RECORD_RANDOM_EVENT_BUILDING_MODIFIER_RESULT turn=%d player=%d team=%d triggeredId=%d event=%s scope=%s target=%s affectedPlayer=%d affectedTeam=%d cityId=%d affectedCityCount=%d modifier=COMMERCE buildingClass=%s building=%s subType=%s operation=ADD configuredValue=%+d valueBefore=%d valueAfter=%d",
					GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), iTriggeredId, getSASGameRecordEventType(eEvent), szScope,
					bCityScope ? "CITY_STORED_MODIFIER" : "CURRENT_CITIES_STORED_MODIFIER", eAffectedPlayer,
					GET_PLAYER(eAffectedPlayer).getTeam(), iCityId, iAffectedCityCount,
					GC.getInfo(perBuildingClassVal.first).getType(), getSASGameRecordBuildingType(eBuilding), getSASGameRecordCommerceType(perCommerceVal.first),
					perCommerceVal.second, iBefore, iAfter);
		}
	}
	FOR_EACH_NON_DEFAULT_PAIR(kEvent.getBuildingHappyChange(), BuildingClass, int)
	{
		BuildingTypes const eBuilding = (bCityScope ? pCity->getCivilization().getBuilding(perBuildingClassVal.first) :
				kPlayer.getCivilization().getBuilding(perBuildingClassVal.first));
		int iBefore = -1;
		int iAfter = -1;
		char const* szTarget = "CITY_STORED_MODIFIER";
		char const* szOperation = "SET";
		if (bCityScope)
			iAfter = pCity->getBuildingHappyChange(perBuildingClassVal.first);
		else
		{
			szTarget = "PLAYER_BUILDING_MODIFIER";
			szOperation = "ADD";
			if (eBuilding != NO_BUILDING)
			{
				iAfter = kPlayer.getExtraBuildingHappiness(eBuilding);
				iBefore = iAfter - perBuildingClassVal.second;
			}
		}
		logSASGameRecord("GAME_RECORD_RANDOM_EVENT_BUILDING_MODIFIER_RESULT turn=%d player=%d team=%d triggeredId=%d event=%s scope=%s target=%s affectedPlayer=%d affectedTeam=%d cityId=%d affectedCityCount=%d modifier=HAPPINESS buildingClass=%s building=%s subType=- operation=%s configuredValue=%+d valueBefore=%d valueAfter=%d",
			GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), iTriggeredId, getSASGameRecordEventType(eEvent), szScope,
			szTarget, eAffectedPlayer, GET_PLAYER(eAffectedPlayer).getTeam(), iCityId, bCityScope ? 1 : -1,
			GC.getInfo(perBuildingClassVal.first).getType(), getSASGameRecordBuildingType(eBuilding), szOperation,
			perBuildingClassVal.second, iBefore, iAfter);
	}
	FOR_EACH_NON_DEFAULT_PAIR(kEvent.getBuildingHealthChange(), BuildingClass, int)
	{
		BuildingTypes const eBuilding = (bCityScope ? pCity->getCivilization().getBuilding(perBuildingClassVal.first) :
				kPlayer.getCivilization().getBuilding(perBuildingClassVal.first));
		int iBefore = -1;
		int iAfter = -1;
		char const* szTarget = "CITY_STORED_MODIFIER";
		char const* szOperation = "SET";
		if (bCityScope)
			iAfter = pCity->getBuildingHealthChange(perBuildingClassVal.first);
		else
		{
			szTarget = "PLAYER_BUILDING_MODIFIER";
			szOperation = "ADD";
			if (eBuilding != NO_BUILDING)
			{
				iAfter = kPlayer.getExtraBuildingHealth(eBuilding);
				iBefore = iAfter - perBuildingClassVal.second;
			}
		}
		logSASGameRecord("GAME_RECORD_RANDOM_EVENT_BUILDING_MODIFIER_RESULT turn=%d player=%d team=%d triggeredId=%d event=%s scope=%s target=%s affectedPlayer=%d affectedTeam=%d cityId=%d affectedCityCount=%d modifier=HEALTH buildingClass=%s building=%s subType=- operation=%s configuredValue=%+d valueBefore=%d valueAfter=%d",
			GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), iTriggeredId, getSASGameRecordEventType(eEvent), szScope,
			szTarget, eAffectedPlayer, GET_PLAYER(eAffectedPlayer).getTeam(), iCityId, bCityScope ? 1 : -1,
			GC.getInfo(perBuildingClassVal.first).getType(), getSASGameRecordBuildingType(eBuilding), szOperation,
			perBuildingClassVal.second, iBefore, iAfter);
	}
}

SASGameRecordRandomEventUnitState::SASGameRecordRandomEventUnitState() :
		iExists(0), iUnitId(-1), eUnit(NO_UNIT), eUnitAI(NO_UNITAI), iX(INVALID_PLOT_COORD), iY(INVALID_PLOT_COORD),
		iDamage(-1), iExperience(-1), iImmobileTurns(-1), ePromotion(NO_PROMOTION), iHasPromotion(-1)
{}

SASGameRecordRandomEventUnitState::SASGameRecordRandomEventUnitState(CvUnit const& kUnit, EventTypes eEvent) :
		iExists(1), iUnitId(kUnit.getID()), eUnit(kUnit.getUnitType()), eUnitAI(kUnit.AI_getUnitAIType()), iX(kUnit.getX()), iY(kUnit.getY()),
		iDamage(kUnit.getDamage()), iExperience(kUnit.getExperience()), iImmobileTurns(kUnit.getImmobileTimer()),
		ePromotion((PromotionTypes)GC.getInfo(eEvent).getUnitPromotion()),
		iHasPromotion(ePromotion == NO_PROMOTION ? -1 : kUnit.isHasPromotion(ePromotion))
{}

// <!-- custom: Preserve the concrete stored-unit result after CvUnit::applyEvent rather than only the EventInfo's UNIT_LOCAL label.
// A disbanded unit is represented by existsAfter=0 and sentinel after-state values; no dead object is dereferenced after the original kill. (ChatGPT-5.6-Sol) -->
void logSASGameRecordRandomEventUnitResult(PlayerTypes ePlayer, int iTriggeredId, EventTypes eEvent, SASGameRecordRandomEventUnitState const& kBefore, SASGameRecordRandomEventUnitState const& kAfter)
{
	CvEventInfo const& kEvent = GC.getInfo(eEvent);
	if (!kBefore.iExists)
		return;
	CvWString const szUnitNameKey(kEvent.getUnitNameKey());
	bool const bRenameApplied = !szUnitNameKey.empty();
	bool const bChanged = (!kAfter.iExists || kBefore.iDamage != kAfter.iDamage || kBefore.iExperience != kAfter.iExperience ||
			kBefore.iImmobileTurns != kAfter.iImmobileTurns || kBefore.iHasPromotion != kAfter.iHasPromotion ||
			kEvent.isDisbandUnit() || bRenameApplied);
	if (!bChanged)
		return;
	logSASGameRecord("GAME_RECORD_RANDOM_EVENT_UNIT_RESULT turn=%d player=%d team=%d triggeredId=%d event=%s unitId=%d unit=%s unitAI=%s x=%d y=%d existsBefore=%d existsAfter=%d damageBefore=%d damageAfter=%d damageDelta=%+d experienceBefore=%d experienceAfter=%d experienceDelta=%+d immobileTurnsBefore=%d immobileTurnsAfter=%d immobileTurnsDelta=%+d promotion=%s hadPromotionBefore=%d hasPromotionAfter=%d unitNameKey=%S renameApplied=%d disbanded=%d",
			GC.getGame().getGameTurn(), ePlayer, ePlayer == NO_PLAYER ? NO_TEAM : GET_PLAYER(ePlayer).getTeam(), iTriggeredId, getSASGameRecordEventType(eEvent),
			kBefore.iUnitId, getSASGameRecordUnitType(kBefore.eUnit), getSASGameRecordUnitAIType(kBefore.eUnitAI), kBefore.iX, kBefore.iY,
			kBefore.iExists, kAfter.iExists, kBefore.iDamage, kAfter.iDamage,
			(kAfter.iDamage < 0 ? -1 : kAfter.iDamage - kBefore.iDamage), kBefore.iExperience, kAfter.iExperience,
			(kAfter.iExperience < 0 ? -1 : kAfter.iExperience - kBefore.iExperience), kBefore.iImmobileTurns, kAfter.iImmobileTurns,
			(kAfter.iImmobileTurns < 0 ? -1 : kAfter.iImmobileTurns - kBefore.iImmobileTurns), getSASGameRecordPromotionType(kBefore.ePromotion),
			kBefore.iHasPromotion, kAfter.iHasPromotion, bRenameApplied ? kEvent.getUnitNameKey() : L"-",
			bRenameApplied, (kEvent.isDisbandUnit() && !kAfter.iExists));
}

SASGameRecordRandomEventPlayerState::SASGameRecordRandomEventPlayerState(CvPlayer const& kPlayer, EventTypes eEvent, PlayerTypes eOtherPlayer) :
		iExtraHappiness(kPlayer.getExtraHappiness()), iExtraHealth(kPlayer.getExtraHealth()), iBaseFreeUnits(kPlayer.getBaseFreeUnits()),
		iSpaceProductionModifier(kPlayer.getSpaceProductionModifier()), iInflationRate(kPlayer.calculateInflationRate()),
		iEspionagePointsAgainstOther(eOtherPlayer == NO_PLAYER ? -1 : GET_TEAM(kPlayer.getTeam()).getEspionagePointsAgainstTeam(GET_PLAYER(eOtherPlayer).getTeam())),
		eBonusRevealed((BonusTypes)GC.getInfo(eEvent).getBonusRevealed()),
		iForceRevealedBonus(eBonusRevealed == NO_BONUS ? -1 : GET_TEAM(kPlayer.getTeam()).isForceRevealedBonus(eBonusRevealed))
{}

// <!-- custom: Keep native player-level EventInfo consequences tied to the selected event without duplicating gold, tech, Golden Age or war rows.
// `inflationRate` is the public realized rate rather than the private raw modifier; configured modifier deltas are included only for the three direct player-modifier XML fields. (ChatGPT-5.6-Sol) -->
void logSASGameRecordRandomEventPlayerResult(CvPlayer const& kPlayer, EventTypes eEvent, int iTriggeredId, PlayerTypes eOtherPlayer, SASGameRecordRandomEventPlayerState const& kBefore, SASGameRecordRandomEventPlayerState const& kAfter)
{
	CvEventInfo const& kEvent = GC.getInfo(eEvent);
	bool const bChanged = (kBefore.iExtraHappiness != kAfter.iExtraHappiness || kBefore.iExtraHealth != kAfter.iExtraHealth ||
			kBefore.iBaseFreeUnits != kAfter.iBaseFreeUnits || kBefore.iSpaceProductionModifier != kAfter.iSpaceProductionModifier ||
			kBefore.iInflationRate != kAfter.iInflationRate || kBefore.iEspionagePointsAgainstOther != kAfter.iEspionagePointsAgainstOther ||
			kBefore.iForceRevealedBonus != kAfter.iForceRevealedBonus || kEvent.getInflationModifier() != 0);
	if (!bChanged)
		return;
	logSASGameRecord("GAME_RECORD_RANDOM_EVENT_PLAYER_RESULT turn=%d player=%d team=%d triggeredId=%d event=%s otherPlayer=%d otherTeam=%d playerExtraHappinessBefore=%d playerExtraHappinessAfter=%d playerExtraHealthBefore=%d playerExtraHealthAfter=%d baseFreeUnitsBefore=%d baseFreeUnitsAfter=%d configuredFreeUnitSupport=%+d spaceProductionModifierBefore=%d spaceProductionModifierAfter=%d configuredSpaceProductionModifier=%+d inflationRateBefore=%d inflationRateAfter=%d configuredInflationModifier=%+d espionagePointsAgainstOtherBefore=%d espionagePointsAgainstOtherAfter=%d espionagePointsDelta=%+d bonusRevealed=%s forceRevealedBefore=%d forceRevealedAfter=%d",
			GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), iTriggeredId, getSASGameRecordEventType(eEvent), eOtherPlayer,
			eOtherPlayer == NO_PLAYER ? NO_TEAM : GET_PLAYER(eOtherPlayer).getTeam(), kBefore.iExtraHappiness, kAfter.iExtraHappiness,
			kBefore.iExtraHealth, kAfter.iExtraHealth, kBefore.iBaseFreeUnits, kAfter.iBaseFreeUnits, kEvent.getFreeUnitSupport(),
			kBefore.iSpaceProductionModifier, kAfter.iSpaceProductionModifier, kEvent.getSpaceProductionModifier(), kBefore.iInflationRate, kAfter.iInflationRate,
			kEvent.getInflationModifier(), kBefore.iEspionagePointsAgainstOther, kAfter.iEspionagePointsAgainstOther,
			(kBefore.iEspionagePointsAgainstOther < 0 || kAfter.iEspionagePointsAgainstOther < 0 ? -1 : kAfter.iEspionagePointsAgainstOther - kBefore.iEspionagePointsAgainstOther),
			getSASGameRecordBonusType(kAfter.eBonusRevealed != NO_BONUS ? kAfter.eBonusRevealed : kBefore.eBonusRevealed), kBefore.iForceRevealedBonus, kAfter.iForceRevealedBonus);
}

// <!-- custom: EventInfos can grant a persistent free promotion to a whole UnitCombat or UnitClass while also updating existing matching units immediately.
// Record one realized scope row with the newly promoted existing-unit count instead of one row per unit. (ChatGPT-5.6-Sol) -->
void logSASGameRecordRandomEventFreePromotionResult(CvPlayer const& kPlayer, EventTypes eEvent, int iTriggeredId, char const* szScope, int iScopeId, PromotionTypes ePromotion, int iExistingUnitsNewlyPromoted, int iFreePromotionBefore, int iFreePromotionAfter)
{
	char const* szScopeType = "-";
	if (strcmp(szScope, "UNIT_COMBAT") == 0 && iScopeId >= 0 && iScopeId < GC.getNumUnitCombatInfos())
		szScopeType = getSASGameRecordUnitCombatType((UnitCombatTypes)iScopeId);
	else if (strcmp(szScope, "UNIT_CLASS") == 0 && iScopeId >= 0 && iScopeId < GC.getNumUnitClassInfos())
		szScopeType = GC.getInfo((UnitClassTypes)iScopeId).getType();
	logSASGameRecord("GAME_RECORD_RANDOM_EVENT_FREE_PROMOTION_RESULT turn=%d player=%d team=%d triggeredId=%d event=%s scope=%s scopeType=%s promotion=%s existingUnitsNewlyPromoted=%d freePromotionBefore=%d freePromotionAfter=%d",
			GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), iTriggeredId, getSASGameRecordEventType(eEvent), szScope, szScopeType,
			getSASGameRecordPromotionType(ePromotion), iExistingUnitsNewlyPromoted, iFreePromotionBefore, iFreePromotionAfter);
}

// <!-- custom: EventInfos can create civilization-specific free units directly rather than through city production.
// Record the already resolved unit class/type, requested versus successfully created count, and actual spawn city/location; no extra unit construction or scan is performed. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordRandomEventFreeUnitsResult(PlayerTypes ePlayer, PlayerTypes eAffectedPlayer, EventTypes eEvent, int iTriggeredId, UnitClassTypes eUnitClass, UnitTypes eUnit, int iRequestedCount, int iCreatedCount, CvCity const* pSpawnCity)
{
	logSASGameRecord("GAME_RECORD_RANDOM_EVENT_FREE_UNITS_RESULT turn=%d player=%d team=%d affectedPlayer=%d triggeredId=%d event=%s unitClass=%s unit=%s requested=%d created=%d cityId=%d plot=%d,%d",
		GC.getGame().getGameTurn(), ePlayer, (ePlayer == NO_PLAYER ? NO_TEAM : GET_PLAYER(ePlayer).getTeam()), eAffectedPlayer, iTriggeredId,
		getSASGameRecordEventType(eEvent), eUnitClass == NO_UNITCLASS ? "-" : GC.getInfo(eUnitClass).getType(),
		getSASGameRecordUnitType(eUnit), iRequestedCount, iCreatedCount, pSpawnCity == NULL ? -1 : pSpawnCity->getID(),
		pSpawnCity == NULL ? INVALID_PLOT_COORD : pSpawnCity->getX(), pSpawnCity == NULL ? INVALID_PLOT_COORD : pSpawnCity->getY());
}

// <!-- custom: Successful ClearEventChance rolls are durable EventInfo lifecycle changes, not candidate diagnostics.
// Record only the realized clear transaction after the existing player/team/global reset loop has run, including how many scoped occurrences actually existed and were cleared. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordRandomEventOccurrenceCleared(CvPlayer const& kPlayer, EventTypes eSourceEvent, EventTypes eClearedEvent, int iTriggeredId, int iClearChance, char const* szScope, TeamTypes eScopeTeam, int iScopePlayerSlots, int iScopeEverAlivePlayers, int iClearedOccurrences)
{
	logSASGameRecord("GAME_RECORD_RANDOM_EVENT_OCCURRENCE_CLEARED turn=%d player=%d team=%d triggeredId=%d sourceEvent=%s clearedEvent=%s clearChance=%d scope=%s scopeTeam=%d scopePlayerSlots=%d scopeEverAlivePlayers=%d clearedOccurrences=%d",
			GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), iTriggeredId, getSASGameRecordEventType(eSourceEvent), getSASGameRecordEventType(eClearedEvent),
			iClearChance, szScope, eScopeTeam, iScopePlayerSlots, iScopeEverAlivePlayers, iClearedOccurrences);
}

// <!-- custom: checkExpireEvent can end a previously applied quest/event turns after its original trigger.
// Record the exact existing branch that caused meaningful expiry plus the post-removal stored-target validity; routine NON_QUEST_TIMEOUT housekeeping is intentionally suppressed by the caller. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordRandomEventExpired(CvPlayer const& kPlayer, EventTypes eEvent, EventTriggeredData const& kTriggeredData, char const* szReason)
{
	CvEventInfo const& kEvent = GC.getInfo(eEvent);
	CvEventTriggerInfo const& kTrigger = GC.getInfo(kTriggeredData.m_eTrigger);
	SASGameRecordRandomEventTargets const kTargets(kPlayer, &kTriggeredData, eEvent);
	logSASGameRecord("GAME_RECORD_RANDOM_EVENT_EXPIRED turn=%d player=%d team=%d triggeredId=%d trigger=%s event=%s reason=%s quest=%d triggerTurn=%d ageTurns=%d prereqEvents=%s prereqEventCity=%d cityId=%d cityExists=%d otherPlayer=%d otherPlayerAlive=%d otherCityId=%d otherCityExists=%d unitId=%d unitExists=%d unit=%s unitCanApply=%d plot=%d,%d plotExists=%d plotOwner=%d religion=%s corporation=%s building=%s buildingPresentInCity=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), kTriggeredData.m_iId,
		getSASGameRecordEventTriggerType(kTriggeredData.m_eTrigger), getSASGameRecordEventType(eEvent), szReason, kEvent.isQuest(),
		kTriggeredData.m_iTurn, GC.getGame().getGameTurn() - kTriggeredData.m_iTurn,
		getSASGameRecordRandomEventTriggerPrereqs(kTrigger).GetCString(), kTrigger.isPrereqEventCity(), kTargets.iCityId,
		kTargets.iCityExists, kTargets.eOtherPlayer, kTargets.iOtherPlayerAlive, kTargets.iOtherCityId, kTargets.iOtherCityExists,
		kTargets.iUnitId, kTargets.iUnitExists, kTargets.szUnit, kTargets.iUnitCanApply, kTargets.iPlotX, kTargets.iPlotY,
		kTargets.iPlotExists, kTargets.iPlotOwner, getSASGameRecordReligionType(kTargets.eReligion),
		getSASGameRecordCorporationType(kTargets.eCorporation), getSASGameRecordBuildingType(kTargets.eBuilding),
		kTargets.iBuildingPresentInCity);
}

// <!-- custom: KI#736 is already repaired in gameplay: both pillage scopes roll MinPillage..MaxPillage inclusively.
// Log the realized attempt/destruction counts only after the existing loops, proving endpoint reachability while keeping exact plot destruction in canonical GAME_RECORD_PLOT_CHANGE rows. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordRandomEventPillageResult(char const* szScope, PlayerTypes ePlayer, PlayerTypes eAffectedPlayer, int iCityId, int iTriggeredId, EventTypes eEvent, int iMinPillage, int iMaxPillage, int iAttempts, int iDestroyed)
{
	logSASGameRecord("GAME_RECORD_RANDOM_EVENT_PILLAGE_RESULT turn=%d scope=%s player=%d affectedPlayer=%d cityId=%d triggeredId=%d event=%s minPillage=%d maxPillage=%d attempts=%d destroyed=%d failedAttempts=%d",
		GC.getGame().getGameTurn(), szScope, ePlayer, eAffectedPlayer, iCityId, iTriggeredId, getSASGameRecordEventType(eEvent), iMinPillage,
		iMaxPillage, iAttempts, iDestroyed, std::max(0, iAttempts - iDestroyed));
}

// <!-- custom: Delayed AdditionalEvent outcomes are actual scheduled lifecycle state, unlike speculative candidate/chance evaluation.
// Record the due turn only after the existing chance roll and earliest-countdown merge have resolved. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordRandomEventCountdownScheduled(CvPlayer const& kPlayer, EventTypes eSourceEvent, EventTypes eFollowupEvent, int iTriggeredId, int iRequestedDueTurn, int iPreviousDueTurn, int iScheduledDueTurn)
{
	logSASGameRecord("GAME_RECORD_RANDOM_EVENT_COUNTDOWN_SCHEDULED turn=%d player=%d team=%d triggeredId=%d sourceEvent=%s followupEvent=%s requestedDueTurn=%d previousDueTurn=%d scheduledDueTurn=%d delayTurns=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), iTriggeredId, getSASGameRecordEventType(eSourceEvent),
		getSASGameRecordEventType(eFollowupEvent), iRequestedDueTurn, iPreviousDueTurn, iScheduledDueTurn,
		iScheduledDueTurn - GC.getGame().getGameTurn());
}

void logSASGameRecordUnitCompleted(CvCity const* pCity, CvUnit const* pUnit, SASGameRecordUnitCompletionSource eSource, int iRawModifiedOverflow, int iUnmodifiedOverflow, int iKeptOverflow, int iLostProduction, int iUnusedOverflowCapacity, int iOverflowGold)
{
	if (pCity == NULL || pUnit == NULL)
		return;
	PlayerTypes const ePlayer = pUnit->getOwner();
	if (ePlayer < 0 || ePlayer >= MAX_PLAYERS)
		return;
	SASGameRecordPlayerFlow& kFlow = g_akSASGameRecordPlayerFlow[ePlayer];
	int const iProductionNeeded = GET_PLAYER(ePlayer).getProductionNeeded(pUnit->getUnitType());
	char const* szSource = NULL;
	switch (eSource)
	{
	case SAS_UNIT_COMPLETION_PRODUCTION:
		kFlow.iUnitsCompleted++;
		kFlow.iUnitProductionNeeded += iProductionNeeded;
		kFlow.aiUnitTypes[pUnit->getUnitType()]++;
		szSource = "PRODUCTION";
		break;
	case SAS_UNIT_COMPLETION_CONSCRIPT:
		kFlow.iUnitsConscripted++;
		kFlow.iConscriptProductionNeeded += iProductionNeeded;
		kFlow.aiConscriptedUnitTypes[pUnit->getUnitType()]++;
		szSource = "CONSCRIPT";
		break;
	case SAS_UNIT_COMPLETION_COLONY_FREE_DEFENDER:
		kFlow.iColonyFreeDefenders++;
		kFlow.iColonyFreeDefenderProductionNeeded += iProductionNeeded;
		kFlow.aiColonyFreeDefenderUnitTypes[pUnit->getUnitType()]++;
		szSource = "COLONY_FREE_DEFENDER";
		break;
	default:
		FAssert(false);
		return;
	}
	if (gGameRecordLogLevel >= 3)
	{
		logSASGameRecord("GAME_RECORD_ACTION turn=%d type=UNIT_COMPLETED player=%d cityId=%d city=%S unitId=%d unit=%s unitAI=%s source=%s productionNeeded=%d rawModifiedOverflow=%d unmodifiedOverflow=%d keptOverflow=%d lostProduction=%d unusedOverflowCapacity=%d overflowGold=%d",
			GC.getGame().getGameTurn(), ePlayer, pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(), pUnit->getID(),
			getSASGameRecordUnitType(pUnit->getUnitType()), getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()), szSource,
			iProductionNeeded, iRawModifiedOverflow, iUnmodifiedOverflow, iKeptOverflow, iLostProduction, iUnusedOverflowCapacity, iOverflowGold);
	}
}

// <!-- custom: Keep exact research-overflow arithmetic separate from TECH_ACQUIRED because only ordinary research completion has meaningful progress/overflow conversion.
// The threshold caller supplies its exact arithmetic while recorder-local same-turn application context supplies the fresh-research/carried-overflow split without widening generic research APIs. (ChatGPT-5.6-Sol) -->
void logSASGameRecordResearchCompleted(TechTypes eTech, TeamTypes eTeam, PlayerTypes ePlayer, int iProgressBefore, int iProgressBeforeClamp, int iResearchModifier, int iUnmodifiedOverflow)
{
	CvTeam const& kTeam = GET_TEAM(eTeam);
	int const iResearchCost = kTeam.getResearchCost(eTech);
	int const iProgressAdded = iProgressBeforeClamp - iProgressBefore;
	int const iRawModifiedOverflow = std::max(0, iProgressBeforeClamp - iResearchCost);
	SASGameRecordResearchApplication& kApplication = g_akSASGameRecordResearchApplication[ePlayer];
	bool const bApplicationKnown = (kApplication.bValid && kApplication.iGameTurn == GC.getGame().getGameTurn() && kApplication.eTech == eTech);
	int const iModifiedResearchRate = (bApplicationKnown ? kApplication.iModifiedResearchRate : -1);
	int const iIncomingOverflowUnmodified = (bApplicationKnown ? kApplication.iIncomingOverflowUnmodified : -1);
	int const iIncomingOverflowModified = (bApplicationKnown ? kApplication.iIncomingOverflowModified : -1);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=RESEARCH_COMPLETED player=%d team=%d tech=%s researchCost=%d teamProgressBefore=%d applicationBreakdownKnown=%d modifiedResearchRateApplied=%d incomingOverflowUnmodified=%d incomingOverflowModifiedApplied=%d modifiedProgressAdded=%d teamProgressBeforeClamp=%d researchModifier=%d rawModifiedOverflow=%d outgoingOverflowUnmodified=%d playerOverflowAfter=%d teamStoredProgressAfter=%d",
		GC.getGame().getGameTurn(), ePlayer, eTeam, getSASGameRecordTechType(eTech), iResearchCost, iProgressBefore,
		bApplicationKnown ? 1 : 0, iModifiedResearchRate, iIncomingOverflowUnmodified, iIncomingOverflowModified, iProgressAdded,
		iProgressBeforeClamp, iResearchModifier, iRawModifiedOverflow, iUnmodifiedOverflow, GET_PLAYER(ePlayer).getOverflowResearch(),
		kTeam.getResearchProgress(eTech));
	kApplication.bValid = false;
}

// <!-- custom: Added eCause to write the acquisition source supplied by gameplay code instead of inferring it from ambiguous announcement/first-discovery flags. (GPT-5.6-Sol + GPT-5.6 Thinking) -->
void logSASGameRecordTechAcquired(TechTypes eType, TeamTypes eTeam, PlayerTypes ePlayer, TechAcquisitionCause eCause)
{
	CvTechInfo const& kTech = GC.getInfo(eType);
	// <!-- custom: The acquisition turn already gives the exact chronology.
	// Mark technologies that enable tech or gold trading, while team snapshots state whether each capability is currently available. (GPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=TECH_ACQUIRED player=%d team=%d tech=%s source=%s enablesTechTrading=%d enablesGoldTrading=%d",
		GC.getGame().getGameTurn(), ePlayer, eTeam, getSASGameRecordTechType(eType), getSASTechAcquisitionCause(eCause),
		kTech.isTechTrading(), kTech.isGoldTrading());
}

void logSASGameRecordBarbarianCitySiteChoice(bool bSkipCivAreas, int iProbModifierPercent, int iTargetCitiesMultiplier, int iDiscouragedRange, CvPlot const* const apPlots[], int const aiRawValues[], int const aiAreaValues[], int const aiFinalValues[], int const aiRandomPercents[], int iCandidateCount)
{
	CvPlot const* pChosen = (iCandidateCount > 0 ? apPlots[0] : NULL);
	CvPlot const* pAlt1 = (iCandidateCount > 1 ? apPlots[1] : NULL);
	CvPlot const* pAlt2 = (iCandidateCount > 2 ? apPlots[2] : NULL);
	// <!-- custom: Unlike normal-civ settling, Barbarian cities are selected directly inside CvGame::createBarbarianCity from a full gameplay-required scan.
	// Preserve that exact chooser-time winner and two runner-ups here; the caller passes values it already computed, so SASGameRecord performs no additional evaluation, RNG or map scan. (ChatGPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_BARBARIAN_CITY_SITE_CHOICE turn=%d skipCivAreas=%d probabilityModifierPercent=%d targetCitiesMultiplier=%d discouragedRange=%d chosenX=%d chosenY=%d chosenArea=%d chosenRawValue=%d chosenAreaAdjustedValue=%d chosenFinalValue=%d chosenRandomPercent=%d alt1X=%d alt1Y=%d alt1Area=%d alt1RawValue=%d alt1AreaAdjustedValue=%d alt1FinalValue=%d alt1RandomPercent=%d alt2X=%d alt2Y=%d alt2Area=%d alt2RawValue=%d alt2AreaAdjustedValue=%d alt2FinalValue=%d alt2RandomPercent=%d",
		GC.getGame().getGameTurn(), bSkipCivAreas, iProbModifierPercent, iTargetCitiesMultiplier, iDiscouragedRange,
		pChosen == NULL ? -1 : pChosen->getX(), pChosen == NULL ? -1 : pChosen->getY(), pChosen == NULL ? -1 : pChosen->getArea().getID(),
		iCandidateCount > 0 ? aiRawValues[0] : -1, iCandidateCount > 0 ? aiAreaValues[0] : -1, iCandidateCount > 0 ? aiFinalValues[0] : -1,
		iCandidateCount > 0 ? aiRandomPercents[0] : -1, pAlt1 == NULL ? -1 : pAlt1->getX(), pAlt1 == NULL ? -1 : pAlt1->getY(),
		pAlt1 == NULL ? -1 : pAlt1->getArea().getID(), iCandidateCount > 1 ? aiRawValues[1] : -1, iCandidateCount > 1 ? aiAreaValues[1] : -1,
		iCandidateCount > 1 ? aiFinalValues[1] : -1, iCandidateCount > 1 ? aiRandomPercents[1] : -1, pAlt2 == NULL ? -1 : pAlt2->getX(),
		pAlt2 == NULL ? -1 : pAlt2->getY(), pAlt2 == NULL ? -1 : pAlt2->getArea().getID(), iCandidateCount > 2 ? aiRawValues[2] : -1,
		iCandidateCount > 2 ? aiAreaValues[2] : -1, iCandidateCount > 2 ? aiFinalValues[2] : -1,
		iCandidateCount > 2 ? aiRandomPercents[2] : -1);
}

void logSASGameRecordCityFoundingSite(CvPlayer const& kPlayer, CvPlot const& kPlot)
{
	if (kPlayer.isBarbarian())
		return;
	CvPlayerAI const& kPlayerAI = kPlayer.AI();
	int iChosenFoundValue = -1;
	int const iChosenRank = getSASGameRecordCitySiteRank(kPlayerAI, kPlot, iChosenFoundValue);
	int iAlt1Rank = -1;
	int iAlt1X = -1;
	int iAlt1Y = -1;
	int iAlt1FoundValue = -1;
	int iAlt2Rank = -1;
	int iAlt2X = -1;
	int iAlt2Y = -1;
	int iAlt2FoundValue = -1;
	getSASGameRecordCitySiteAlternative(kPlayerAI, &kPlot, 0, iAlt1Rank, iAlt1X, iAlt1Y, iAlt1FoundValue);
	getSASGameRecordCitySiteAlternative(kPlayerAI, &kPlot, 1, iAlt2Rank, iAlt2X, iAlt2Y, iAlt2FoundValue);

	// <!-- custom: Founding is a rare level-2 event, so preserve a compact player-known versus diagnostic-omniscient quality summary for the site that actually became a city.
	// This reuses AIFoundValue's existing 1/2/3/6/10/14 plot-distribution work in one evaluation per perspective; periodic snapshots do no settlement rescoring.
	// It complements BBAI candidate/sniping diagnostics without copying rejected-candidate spam into the broad GameRecord. (ChatGPT-5.6-Sol) -->
	bool const bFirstCity = (kPlayer.getNumCities() <= 0);
	int aiKnownCoreSums[6] = {0, 0, 0, 0, 0, 0};
	int iKnownPositivePlots = 0;
	int iKnownSustainableProductivePlotValue = 0;
	CitySiteEvaluator kKnownEvaluator(kPlayerAI, -1, bFirstCity);
	// <!-- custom: Match the existing BBAI chosen-site replay: ignore the maintained city-site list during this founding-time rescore.
	// Those sites were not all present when this plot was originally selected; leaving them active can reject/distort the founded plot itself. Diagnostic only. (ChatGPT-5.6-Sol) -->
	kKnownEvaluator.setDebug(true);
	if (bFirstCity)
		kKnownEvaluator.setAllSeeing(false);
	int const iKnownRescoredFoundValue = kKnownEvaluator.evaluateWithPlotValueDistribution(kPlot, aiKnownCoreSums, iKnownPositivePlots, iKnownSustainableProductivePlotValue);

	int aiOmniscientCoreSums[6] = {0, 0, 0, 0, 0, 0};
	int iOmniscientPositivePlots = 0;
	int iOmniscientSustainableProductivePlotValue = 0;
	CitySiteEvaluator kOmniscientEvaluator(kPlayerAI, -1, bFirstCity);
	kOmniscientEvaluator.setDebug(true);
	kOmniscientEvaluator.setDiagnosticOmniscience(true);
	int const iOmniscientRescoredFoundValue = kOmniscientEvaluator.evaluateWithPlotValueDistribution(kPlot, aiOmniscientCoreSums, iOmniscientPositivePlots, iOmniscientSustainableProductivePlotValue);
	FAssert(iKnownSustainableProductivePlotValue == iOmniscientSustainableProductivePlotValue);

	int const iMinWaterSizeForOcean = GC.getDefineINT(CvGlobals::MIN_WATER_SIZE_FOR_OCEAN);
	bool const bCoastal = kPlot.isCoastalLand(iMinWaterSizeForOcean);
	int const iKnownWaterBonuses = (bCoastal ? CitySiteEvaluator::countWaterBonuses(kPlot, kPlayer.getTeam(), false) : 0);
	int const iOmniscientWaterBonuses = (bCoastal ? CitySiteEvaluator::countWaterBonuses(kPlot, kPlayer.getTeam(), true) : 0);
	int const iAreaUnownedTiles = kPlot.getArea().getNumUnownedTiles();
	int const iAreaSelectionFactor = std::min(NUM_CITY_PLOTS * 2, iAreaUnownedTiles + 1);
	CvCity const* pNearestOwnCity = GC.getMap().findCity(kPlot.getX(), kPlot.getY(), kPlayer.getID(), NO_TEAM, false);
	int const iNearestOwnCityId = (pNearestOwnCity == NULL ? -1 : pNearestOwnCity->getID());
	int const iNearestOwnCityX = (pNearestOwnCity == NULL ? -1 : pNearestOwnCity->getX());
	int const iNearestOwnCityY = (pNearestOwnCity == NULL ? -1 : pNearestOwnCity->getY());
	int const iNearestOwnCityDistance = (pNearestOwnCity == NULL ? -1 : plotDistance(&kPlot, pNearestOwnCity->plot()));

	// <!-- custom: `chosenCachedFoundValue=-1` means the founded plot is not in the current cached strategic shortlist, not that its true CitySiteEvaluator value is -1.
	// This distinction is useful in itself: a travelling settler can legitimately found a formerly selected site after the live shortlist has changed. (ChatGPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_CITY_SITE_CHOICE turn=%d player=%d human=%d x=%d y=%d area=%d plotOwner=%d citySites=%d minFoundValue=%d chosenSiteListRank=%d chosenCachedFoundValue=%d alt1SiteListRank=%d alt1X=%d alt1Y=%d alt1CachedFoundValue=%d alt2SiteListRank=%d alt2X=%d alt2Y=%d alt2CachedFoundValue=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.isHuman(), kPlot.getX(), kPlot.getY(), kPlot.getArea().getID(),
		kPlot.getOwner(), kPlayerAI.AI_getNumCitySites(), kPlayerAI.AI_getMinFoundValue(), iChosenRank, iChosenFoundValue, iAlt1Rank, iAlt1X,
		iAlt1Y, iAlt1FoundValue, iAlt2Rank, iAlt2X, iAlt2Y, iAlt2FoundValue);
	// <!-- custom: Keep the heavier chosen-site quality in its own rare row rather than widening the established shortlist row; rejected-candidate causes remain BBAI Found diagnostics.
	// `*RescoredFoundValue` is an explicit founding-time diagnostic replay, not the cached value that originally selected the site; GAME_RECORD_CITY_SITE_CHOICE preserves that strategic shortlist value separately. (ChatGPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_CITY_SITE_QUALITY turn=%d player=%d firstCity=%d x=%d y=%d coastal=%d knownWaterBonuses=%d omniscientWaterBonuses=%d areaUnownedTiles=%d areaSelectionFactor=%d nearestOwnCityId=%d nearestOwnCityX=%d nearestOwnCityY=%d nearestOwnCityDistance=%d sustainableProductivePlotValue=%d knownRescoredFoundValue=%d knownBest1Sum=%d knownBest2Sum=%d knownBest3Sum=%d knownBest6Sum=%d knownBest10Sum=%d knownBest14Sum=%d knownPositivePlots=%d omniscientRescoredFoundValue=%d omniscientBest1Sum=%d omniscientBest2Sum=%d omniscientBest3Sum=%d omniscientBest6Sum=%d omniscientBest10Sum=%d omniscientBest14Sum=%d omniscientPositivePlots=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), bFirstCity, kPlot.getX(), kPlot.getY(), bCoastal, iKnownWaterBonuses, iOmniscientWaterBonuses,
		iAreaUnownedTiles, iAreaSelectionFactor, iNearestOwnCityId, iNearestOwnCityX, iNearestOwnCityY, iNearestOwnCityDistance, iKnownSustainableProductivePlotValue,
		iKnownRescoredFoundValue, aiKnownCoreSums[0], aiKnownCoreSums[1], aiKnownCoreSums[2], aiKnownCoreSums[3], aiKnownCoreSums[4],
		aiKnownCoreSums[5], iKnownPositivePlots, iOmniscientRescoredFoundValue, aiOmniscientCoreSums[0], aiOmniscientCoreSums[1],
		aiOmniscientCoreSums[2], aiOmniscientCoreSums[3], aiOmniscientCoreSums[4], aiOmniscientCoreSums[5], iOmniscientPositivePlots);
}

void logSASGameRecordCityBuilt(CvCity const* pCity)
{
	if (pCity == NULL)
		return;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=CITY_BUILT player=%d cityId=%d city=%S x=%d y=%d pop=%d",
		GC.getGame().getGameTurn(), pCity->getOwner(), pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(), pCity->getX(),
		pCity->getY(), pCity->getPopulation());
	if (gGameRecordLogLevel >= 2)
	{
		logSASGameRecordCityBFC(*pCity, "built");
		logSASGameRecordCityUnits(*pCity, "built");
	}
}

// <!-- custom: Land/population victory thresholds can change with world state, so serialize every enabled victory that actually uses either criterion rather than assuming one XML name such as VICTORY_DOMINATION.
// L/P values are current/required percentages multiplied by 100; M is whether all applicable land/population criteria are met at this exact boundary. (ChatGPT-5.6-Sol) -->
static CvString getSASGameRecordLandPopulationVictoryProgress(TeamTypes eTeam)
{
	CvGame const& kGame = GC.getGame();
	CvTeam const& kTeam = GET_TEAM(eTeam);
	int const iLandPlots = std::max(1, GC.getMap().getLandPlots());
	int const iWorldPopulation = std::max(1, kGame.getTotalPopulation());
	int const iLandPctX100 = (10000 * kTeam.getTotalLand()) / iLandPlots;
	int const iPopPctX100 = (10000 * kTeam.getTotalPopulation()) / iWorldPopulation;
	CvString szResult;
	FOR_EACH_ENUM(Victory)
	{
		if (!kGame.isVictoryValid(eLoopVictory))
			continue;
		int const iLandNeed = kGame.getAdjustedLandPercent(eLoopVictory);
		int const iPopNeed = kGame.getAdjustedPopulationPercent(eLoopVictory);
		if (iLandNeed <= 0 && iPopNeed <= 0)
			continue;
		bool const bLandMet = (iLandNeed <= 0 || 100 * kTeam.getTotalLand() >= GC.getMap().getLandPlots() * iLandNeed);
		bool const bPopMet = (iPopNeed <= 0 || 100 * kTeam.getTotalPopulation() >= kGame.getTotalPopulation() * iPopNeed);
		CvString szItem;
		szItem.Format(szResult.empty() ? "%s:L%d/%d:P%d/%d:M%d" : ";%s:L%d/%d:P%d/%d:M%d",
				GC.getInfo(eLoopVictory).getType(), iLandNeed <= 0 ? -1 : iLandPctX100, iLandNeed <= 0 ? -1 : 100 * iLandNeed,
				iPopNeed <= 0 ? -1 : iPopPctX100, iPopNeed <= 0 ? -1 : 100 * iPopNeed, bLandMet && bPopMet);
		szResult += szItem;
	}
	return getSASDiagnosticOrDash(szResult);
}

static CvString getSASGameRecordCityReligionList(CvCity const& kCity, bool bHolyOnly)
{
	CvString szResult;
	FOR_EACH_ENUM(Religion)
	{
		if ((bHolyOnly && !kCity.isHolyCity(eLoopReligion)) || (!bHolyOnly && !kCity.isHasReligion(eLoopReligion)))
			continue;
		CvString szItem;
		szItem.Format(szResult.empty() ? "%s" : ",%s", getSASGameRecordReligionType(eLoopReligion));
		szResult += szItem;
	}
	return getSASDiagnosticOrDash(szResult);
}

static CvString getSASGameRecordCityCorporationList(CvCity const& kCity, bool bHeadquartersOnly)
{
	CvString szResult;
	FOR_EACH_ENUM(Corporation)
	{
		if ((bHeadquartersOnly && !kCity.isHeadquarters(eLoopCorporation)) || (!bHeadquartersOnly && !kCity.isHasCorporation(eLoopCorporation)))
			continue;
		CvString szItem;
		szItem.Format(szResult.empty() ? "%s" : ",%s", getSASGameRecordCorporationType(eLoopCorporation));
		szResult += szItem;
	}
	return getSASDiagnosticOrDash(szResult);
}

static void getSASGameRecordRazeCityDistances(CvCity const& kCity, PlayerTypes eRazer, PlayerTypes ePreviousOwner, int& iCapitalDistance, int& iCapitalSameArea, int& iNearestRazerCityDistance, int& iSameAreaRazerCitiesOther, int& iNearestPreviousOwnerCityDistance, int& iSameAreaPreviousOwnerCities)
{
	CvPlayer const& kRazer = GET_PLAYER(eRazer);
	CvCity const* pCapital = kRazer.getCapitalCity();
	iCapitalDistance = (pCapital == NULL ? -1 : plotDistance(kCity.getX(), kCity.getY(), pCapital->getX(), pCapital->getY()));
	iCapitalSameArea = (pCapital == NULL ? -1 : pCapital->getArea().getID() == kCity.getArea().getID());
	iNearestRazerCityDistance = -1;
	iSameAreaRazerCitiesOther = 0;
	int iLoop = 0;
	for (CvCity const* pLoopCity = kRazer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kRazer.nextCity(&iLoop))
	{
		if (pLoopCity->getID() == kCity.getID())
			continue;
		int const iDistance = plotDistance(kCity.getX(), kCity.getY(), pLoopCity->getX(), pLoopCity->getY());
		if (iNearestRazerCityDistance < 0 || iDistance < iNearestRazerCityDistance)
			iNearestRazerCityDistance = iDistance;
		if (pLoopCity->getArea().getID() == kCity.getArea().getID())
			iSameAreaRazerCitiesOther++;
	}
	iNearestPreviousOwnerCityDistance = -1;
	iSameAreaPreviousOwnerCities = 0;
	if (ePreviousOwner >= 0 && ePreviousOwner < MAX_PLAYERS)
	{
		CvPlayer const& kPreviousOwner = GET_PLAYER(ePreviousOwner);
		iLoop = 0;
		for (CvCity const* pLoopCity = kPreviousOwner.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPreviousOwner.nextCity(&iLoop))
		{
			int const iDistance = plotDistance(kCity.getX(), kCity.getY(), pLoopCity->getX(), pLoopCity->getY());
			if (iNearestPreviousOwnerCityDistance < 0 || iDistance < iNearestPreviousOwnerCityDistance)
				iNearestPreviousOwnerCityDistance = iDistance;
			if (pLoopCity->getArea().getID() == kCity.getArea().getID())
				iSameAreaPreviousOwnerCities++;
		}
	}
}

void beginSASGameRecordCityRaze(CvCity const* pCity, PlayerTypes ePlayer)
{
	if (pCity == NULL || ePlayer < 0 || ePlayer >= MAX_PLAYERS)
		return;
	CvPlayerAI const& kRazer = GET_PLAYER(ePlayer);
	SASGameRecordCityRazeContext kContext;
	kContext.eRazer = ePlayer;
	kContext.eRazerTeam = kRazer.getTeam();
	kContext.ePreviousOwner = pCity->getPreviousOwner();
	kContext.ePreviousTeam = (kContext.ePreviousOwner >= 0 && kContext.ePreviousOwner < MAX_PLAYERS ? GET_PLAYER(kContext.ePreviousOwner).getTeam() : NO_TEAM);
	kContext.eOriginalOwner = pCity->getOriginalOwner();
	kContext.eOriginalTeam = (kContext.eOriginalOwner >= 0 && kContext.eOriginalOwner < MAX_PLAYERS ? GET_PLAYER(kContext.eOriginalOwner).getTeam() : NO_TEAM);
	kContext.iGameTurn = GC.getGame().getGameTurn();
	kContext.iCityId = pCity->getID();
	kContext.szCityName = getSASGameRecordQuotedCityName(pCity);
	kContext.iX = pCity->getX();
	kContext.iY = pCity->getY();
	kContext.iArea = pCity->getArea().getID();
	kContext.szRazeMode = (pCity->isAutoRaze() ? "AUTO_RAZE" : (kRazer.isHuman() ? "HUMAN" : "AI"));
	kContext.iPopulation = pCity->getPopulation();
	kContext.iHighestPopulation = pCity->getHighestPopulation();
	kContext.iFoundedTurn = pCity->getGameTurnFounded();
	kContext.iAcquiredTurn = pCity->getGameTurnAcquired();
	kContext.iOccupationTurns = pCity->getOccupationTimer();
	kContext.iRazerCulturePercent = pCity->calculateTeamCulturePercent(kContext.eRazerTeam);
	kContext.iPreviousCulturePercent = (kContext.ePreviousTeam == NO_TEAM ? -1 : pCity->calculateTeamCulturePercent(kContext.ePreviousTeam));
	kContext.eHighestCulturePlayer = pCity->findHighestCulture();
	kContext.iHighestCulturePercent = (kContext.eHighestCulturePlayer == NO_PLAYER ? -1 : pCity->calculateCulturePercent(kContext.eHighestCulturePlayer));
	kContext.iMaintenanceTimes100 = pCity->getMaintenanceTimes100();
	kContext.iConnectedToCapital = pCity->isConnectedToCapital();
	getSASGameRecordRazeCityDistances(*pCity, ePlayer, kContext.ePreviousOwner, kContext.iCapitalDistance, kContext.iCapitalSameArea, kContext.iNearestRazerCityDistance, kContext.iSameAreaRazerCitiesOther, kContext.iNearestPreviousOwnerCityDistance, kContext.iSameAreaPreviousOwnerCities);
	kContext.szBuildings = getSASGameRecordCityBuildings(*pCity, kContext.iBuildings, kContext.iRegularBuildings, kContext.iNationalWonders, kContext.iTeamWonders, kContext.iWorldWonders);
	kContext.szReligions = getSASGameRecordCityReligionList(*pCity, false);
	kContext.szHolyReligions = getSASGameRecordCityReligionList(*pCity, true);
	kContext.szCorporations = getSASGameRecordCityCorporationList(*pCity, false);
	kContext.szHeadquarters = getSASGameRecordCityCorporationList(*pCity, true);
	kContext.iPlayerCitiesBefore = kRazer.getNumCities();
	kContext.iPlayerLandBefore = kRazer.getTotalLand();
	kContext.iPlayerPopulationBefore = kRazer.getTotalPopulation();
	kContext.iTeamCitiesBefore = GET_TEAM(kContext.eRazerTeam).getNumCities();
	kContext.iTeamLandBefore = GET_TEAM(kContext.eRazerTeam).getTotalLand();
	kContext.iTeamPopulationBefore = GET_TEAM(kContext.eRazerTeam).getTotalPopulation();
	kContext.iWorldPopulationBefore = GC.getGame().getTotalPopulation();
	kContext.iLandPctX100Before = (10000 * kContext.iTeamLandBefore) / std::max(1, GC.getMap().getLandPlots());
	kContext.iPopPctX100Before = (10000 * kContext.iTeamPopulationBefore) / std::max(1, GC.getGame().getTotalPopulation());
	if (kRazer.isHuman() && !kRazer.isHumanDisabled())
	{
		kContext.iAIMaxVictoryStage = -1;
		kContext.iAIConquestStage = -1;
		kContext.iAIDominationStage = -1;
	}
	else
	{
		AIVictoryStage const eStages = kRazer.AI_getVictoryStageHash();
		kContext.iAIConquestStage = getSASConquestVictoryStageLevel(eStages);
		kContext.iAIDominationStage = getSASDominationVictoryStageLevel(eStages);
		kContext.iAIMaxVictoryStage = std::max(getSASCultureVictoryStageLevel(eStages), std::max(getSASSpaceVictoryStageLevel(eStages), std::max(kContext.iAIConquestStage, std::max(kContext.iAIDominationStage, getSASDiplomacyVictoryStageLevel(eStages)))));
	}
	kContext.szLandPopVictoryProgressBefore = getSASGameRecordLandPopulationVictoryProgress(kContext.eRazerTeam);
	g_aSASGameRecordCityRazeContexts.push_back(kContext);
}

void endSASGameRecordCityRaze(PlayerTypes ePlayer)
{
	if (g_aSASGameRecordCityRazeContexts.empty())
		return;
	// <!-- custom: LIFO matches nested razes triggered synchronously from Python callbacks; only consume a matching context. (ChatGPT-5.6-Sol) -->
	int iContext = (int)g_aSASGameRecordCityRazeContexts.size() - 1;
	if (g_aSASGameRecordCityRazeContexts[iContext].eRazer != ePlayer)
		return;
	SASGameRecordCityRazeContext const kContext = g_aSASGameRecordCityRazeContexts[iContext];
	g_aSASGameRecordCityRazeContexts.pop_back();
	CvPlayer const& kRazer = GET_PLAYER(ePlayer);
	CvTeam const& kTeam = GET_TEAM(kContext.eRazerTeam);
	int const iPlayerCitiesAfter = kRazer.getNumCities();
	int const iPlayerLandAfter = kRazer.getTotalLand();
	int const iPlayerPopulationAfter = kRazer.getTotalPopulation();
	int const iTeamCitiesAfter = kTeam.getNumCities();
	int const iTeamLandAfter = kTeam.getTotalLand();
	int const iTeamPopulationAfter = kTeam.getTotalPopulation();
	int const iWorldPopulationAfter = GC.getGame().getTotalPopulation();
	int const iLandPctX100After = (10000 * iTeamLandAfter) / std::max(1, GC.getMap().getLandPlots());
	int const iPopPctX100After = (10000 * iTeamPopulationAfter) / std::max(1, iWorldPopulationAfter);
	CvString const szVictoryProgressAfter = getSASGameRecordLandPopulationVictoryProgress(kContext.eRazerTeam);
	CvPlot const& kRazedPlot = GC.getMap().getPlot(kContext.iX, kContext.iY);
	PlayerTypes const eCityPlotOwnerAfter = kRazedPlot.getOwner();
	TeamTypes const eCityPlotTeamAfter = kRazedPlot.getTeam();
	// <!-- custom: One rare level-2 action now preserves both the destroyed city's intrinsic/contextual value and the exact empire/victory effect of deleting it.
	// CITY_ACQUIRED + CITY_BFC/CITY_UNITS already preserve capture/BFC/tactical state, so do not duplicate those large rows here. (ChatGPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=CITY_RAZED razer=%d razerTeam=%d razeMode=%s previousOwner=%d previousTeam=%d originalOwner=%d originalTeam=%d cityId=%d city=%S x=%d y=%d area=%d pop=%d highestPop=%d foundedTurn=%d cityAge=%d acquiredTurn=%d turnsHeld=%d occupationTurns=%d razerCulturePercent=%d previousCulturePercent=%d highestCulturePlayer=%d highestCulturePercent=%d connectedToCapital=%d capitalDistance=%d capitalSameArea=%d nearestRazerCityDistance=%d sameAreaRazerCitiesOther=%d nearestPreviousOwnerCityDistance=%d sameAreaPreviousOwnerCities=%d maintenanceTimes100=%d buildings=%d regularBuildings=%d nationalWonders=%d teamWonders=%d worldWonders=%d buildingTypes=%s religions=%s holyReligions=%s corporations=%s headquarters=%s cityPlotOwnerAfter=%d cityPlotTeamAfter=%d playerCitiesBefore=%d playerCitiesAfter=%d playerLandBefore=%d playerLandAfter=%d playerLandDelta=%+d playerPopBefore=%d playerPopAfter=%d playerPopDelta=%+d teamCitiesBefore=%d teamCitiesAfter=%d teamLandBefore=%d teamLandAfter=%d teamLandDelta=%+d landPctX100Before=%d landPctX100After=%d landPctX100Delta=%+d teamPopBefore=%d teamPopAfter=%d teamPopDelta=%+d worldPopBefore=%d worldPopAfter=%d popPctX100Before=%d popPctX100After=%d popPctX100Delta=%+d aiMaxVictoryStage=%d aiConquestStage=%d aiDominationStage=%d landPopVictoryProgressBefore=%s landPopVictoryProgressAfter=%s",
		kContext.iGameTurn, kContext.eRazer, kContext.eRazerTeam, kContext.szRazeMode.GetCString(), kContext.ePreviousOwner,
		kContext.ePreviousTeam, kContext.eOriginalOwner, kContext.eOriginalTeam, kContext.iCityId, kContext.szCityName.GetCString(),
		kContext.iX, kContext.iY, kContext.iArea, kContext.iPopulation, kContext.iHighestPopulation, kContext.iFoundedTurn,
		kContext.iFoundedTurn < 0 ? -1 : kContext.iGameTurn - kContext.iFoundedTurn, kContext.iAcquiredTurn,
		kContext.iAcquiredTurn < 0 ? -1 : kContext.iGameTurn - kContext.iAcquiredTurn, kContext.iOccupationTurns,
		kContext.iRazerCulturePercent, kContext.iPreviousCulturePercent, kContext.eHighestCulturePlayer, kContext.iHighestCulturePercent,
		kContext.iConnectedToCapital, kContext.iCapitalDistance, kContext.iCapitalSameArea, kContext.iNearestRazerCityDistance,
		kContext.iSameAreaRazerCitiesOther, kContext.iNearestPreviousOwnerCityDistance, kContext.iSameAreaPreviousOwnerCities,
		kContext.iMaintenanceTimes100, kContext.iBuildings, kContext.iRegularBuildings, kContext.iNationalWonders, kContext.iTeamWonders,
		kContext.iWorldWonders, kContext.szBuildings.GetCString(), kContext.szReligions.GetCString(), kContext.szHolyReligions.GetCString(),
		kContext.szCorporations.GetCString(), kContext.szHeadquarters.GetCString(), eCityPlotOwnerAfter, eCityPlotTeamAfter,
		kContext.iPlayerCitiesBefore, iPlayerCitiesAfter, kContext.iPlayerLandBefore, iPlayerLandAfter,
		iPlayerLandAfter - kContext.iPlayerLandBefore, kContext.iPlayerPopulationBefore, iPlayerPopulationAfter,
		iPlayerPopulationAfter - kContext.iPlayerPopulationBefore, kContext.iTeamCitiesBefore, iTeamCitiesAfter, kContext.iTeamLandBefore,
		iTeamLandAfter, iTeamLandAfter - kContext.iTeamLandBefore, kContext.iLandPctX100Before, iLandPctX100After,
		iLandPctX100After - kContext.iLandPctX100Before, kContext.iTeamPopulationBefore, iTeamPopulationAfter,
		iTeamPopulationAfter - kContext.iTeamPopulationBefore, kContext.iWorldPopulationBefore, iWorldPopulationAfter,
		kContext.iPopPctX100Before, iPopPctX100After, iPopPctX100After - kContext.iPopPctX100Before, kContext.iAIMaxVictoryStage,
		kContext.iAIConquestStage, kContext.iAIDominationStage, kContext.szLandPopVictoryProgressBefore.GetCString(),
		szVictoryProgressAfter.GetCString());
}

void logSASGameRecordCityAcquired(PlayerTypes eOldOwner, PlayerTypes eNewOwner, CvCity const* pCity, bool bConquest, bool bTrade)
{
	if (pCity == NULL)
		return;
	if (eNewOwner >= 0 && eNewOwner < MAX_PLAYERS)
	{
		g_aiSASGameRecordCitiesAcquired[eNewOwner]++;
		if (bConquest)
			g_aiSASGameRecordCitiesConquered[eNewOwner]++;
		if (bTrade)
			g_aiSASGameRecordCitiesTradedIn[eNewOwner]++;
	}
	if (eOldOwner >= 0 && eOldOwner < MAX_PLAYERS)
	{
		g_aiSASGameRecordCitiesLost[eOldOwner]++;
		if (bConquest)
			g_aiSASGameRecordCitiesLostByConquest[eOldOwner]++;
		if (bTrade)
			g_aiSASGameRecordCitiesTradedOut[eOldOwner]++;
	}
	// <!-- custom: Attribute a conquest to its active team-pair war so the final summary distinguishes territorial results from battle losses and abstract war success. (GPT-5.6-Sol) -->
	if (bConquest && eOldOwner >= 0 && eOldOwner < MAX_PLAYERS && eNewOwner >= 0 && eNewOwner < MAX_PLAYERS)
	{
		TeamTypes const eOldTeam = GET_PLAYER(eOldOwner).getTeam();
		TeamTypes const eNewTeam = GET_PLAYER(eNewOwner).getTeam();
		SASGameRecordWarSummary* pWar = findSASGameRecordWar(eOldTeam, eNewTeam);
		if (pWar != NULL)
		{
			if (eNewTeam == pWar->eTeamA)
			{
				pWar->iCitiesCapturedByA++;
				pWar->iPopulationCapturedByA += pCity->getPopulation();
			}
			else
			{
				pWar->iCitiesCapturedByB++;
				pWar->iPopulationCapturedByB += pCity->getPopulation();
			}
			refreshSASGameRecordWarSuccess(*pWar);
		}
	}
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=CITY_ACQUIRED oldOwner=%d newOwner=%d cityId=%d city=%S x=%d y=%d pop=%d conquest=%d trade=%d",
		GC.getGame().getGameTurn(), eOldOwner, eNewOwner, pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(), pCity->getX(),
		pCity->getY(), pCity->getPopulation(), bConquest, bTrade);
	if (gGameRecordLogLevel >= 2)
	{
		logSASGameRecordCityBFC(*pCity, "acquired");
		logSASGameRecordCityUnits(*pCity, "acquired");
	}
}

void logSASGameRecordWarStarted(TeamTypes eDeclarer, TeamTypes eTarget, WarPlanTypes eWarPlan, bool bPrimaryDoW, bool bNewDiplo, PlayerTypes eSponsor, bool bRandomEvent, WarDeclarationCause eCause)
{
	CvTeam const& kDeclarer = GET_TEAM(eDeclarer);
	CvTeam const& kTarget = GET_TEAM(eTarget);
	CvTeamAI const& kTargetAI = GET_TEAM(eTarget);
	char const* szCause = (bRandomEvent ? "RANDOM_EVENT" : (eSponsor != NO_PLAYER ? "SPONSORED_WAR" : getSASWarDeclarationCause(eCause)));
	addSASGameRecordWar(eDeclarer, eTarget, true, eDeclarer, eTarget, eWarPlan, szCause, bPrimaryDoW);
	// <!-- custom: `cause=DIRECT` identifies how war began, not why the AI selected that rival.
	// Preserve the target's exact victory state at declaration time so archived records show whether victory denial was relevant without falsely claiming it was the sole strategic motive. (GPT-5.6-Sol) -->
	int const iTargetMaxVictoryStage = getSASTeamMaxVictoryStage(eTarget);
	int const iTargetSpaceVictoryStage = getSASTeamSpaceVictoryStage(eTarget);
	int const iTargetSpaceshipParts = getSASTeamSpaceshipPartsBuilt(eTarget);
	int const iTargetSpaceshipPartsPercent = getSASTeamSpaceshipPartsPercent(eTarget);
	int const iTargetVictoryCountdown = kTargetAI.AI_getLowestVictoryCountdown();
	bool const bVictoryDenialContext = (iTargetVictoryCountdown >= 0 || iTargetMaxVictoryStage >= 4 || isSASTeamStage3SpaceVictoryThreat(eTarget));
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=WAR_STARTED declarerTeam=%d targetTeam=%d cause=%s primary=%d newDiplo=%d warPlan=%s sponsorPlayer=%d sponsorTeam=%d randomEvent=%d declarerMaster=%d targetMaster=%d declarerWarsAfter=%d targetWarsAfter=%d victoryDenialContext=%d targetMaxVictoryStage=%d targetSpaceVictoryStage=%d targetSpaceshipParts=%d targetSpaceshipPartsPercent=%d targetVictoryCountdown=%d",
		GC.getGame().getGameTurn(), eDeclarer, eTarget, szCause, bPrimaryDoW, bNewDiplo, getSASWarPlanType(eWarPlan), eSponsor,
		eSponsor == NO_PLAYER ? NO_TEAM : GET_PLAYER(eSponsor).getTeam(), bRandomEvent,
		kDeclarer.isAVassal() ? kDeclarer.getMasterTeam() : NO_TEAM, kTarget.isAVassal() ? kTarget.getMasterTeam() : NO_TEAM,
		kDeclarer.getNumWars(false), kTarget.getNumWars(false), bVictoryDenialContext, iTargetMaxVictoryStage, iTargetSpaceVictoryStage,
		iTargetSpaceshipParts, iTargetSpaceshipPartsPercent, iTargetVictoryCountdown);
}

// <!-- custom: Added final war-success and peace-context parameters because those details are no longer recoverable after CvTeam::makePeace finishes. (GPT-5.6-Sol) -->
void logSASGameRecordWarEnded(TeamTypes eTeam, TeamTypes eOtherTeam, int iTeamAWarSuccess, int iTeamBWarSuccess, bool bCapitulate, TeamTypes eBroker, bool bRandomEvent, bool bReparations)
{
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=WAR_ENDED teamA=%d teamB=%d teamAWarsAfter=%d teamBWarsAfter=%d capitulation=%d brokerTeam=%d randomEvent=%d reparations=%d teamAWarSuccess=%d teamBWarSuccess=%d",
		GC.getGame().getGameTurn(), eTeam, eOtherTeam, GET_TEAM(eTeam).getNumWars(false), GET_TEAM(eOtherTeam).getNumWars(false),
		bCapitulate, eBroker, bRandomEvent, bReparations, iTeamAWarSuccess, iTeamBWarSuccess);
	SASGameRecordWarSummary* pWar = findSASGameRecordWar(eTeam, eOtherTeam);
	if (pWar == NULL)
		pWar = addSASGameRecordWar(eTeam, eOtherTeam, false, NO_TEAM, NO_TEAM, NO_WARPLAN, "UNOBSERVED_START", false);
	if (pWar == NULL)
		return;
	int const iWarSuccessA = (eTeam == pWar->eTeamA ? iTeamAWarSuccess : iTeamBWarSuccess);
	int const iWarSuccessB = (eTeam == pWar->eTeamA ? iTeamBWarSuccess : iTeamAWarSuccess);
	logSASGameRecordWarSummary(*pWar, "ENDED", "MAKE_PEACE", GC.getGame().getGameTurn(), iWarSuccessA, iWarSuccessB, bCapitulate, eBroker, bRandomEvent, bReparations);
	for (size_t iI = 0; iI < g_aSASGameRecordWars.size(); iI++)
	{
		if (&g_aSASGameRecordWars[iI] == pWar)
		{
			g_aSASGameRecordWars.erase(g_aSASGameRecordWars.begin() + iI);
			break;
		}
	}
}

static char const* getSASGameRecordUWAIWarPlanDecisionReason(SASGameRecordUWAIWarPlanDecisionReason eReason)
{
	switch (eReason)
	{
	case SAS_UWAI_WAR_PLAN_ILLEGAL_TARGET: return "ILLEGAL_TARGET";
	case SAS_UWAI_WAR_PLAN_VICTORY_DENIAL_DIRECT: return "VICTORY_DENIAL_DIRECT";
	case SAS_UWAI_WAR_PLAN_IMMINENT_NEGATIVE_UTILITY: return "IMMINENT_NEGATIVE_UTILITY";
	case SAS_UWAI_WAR_PLAN_IMMINENT_TIMEOUT: return "IMMINENT_TIMEOUT";
	case SAS_UWAI_WAR_PLAN_PREPARATION_DEADLINE_REACHED: return "PREPARATION_DEADLINE_REACHED";
	case SAS_UWAI_WAR_PLAN_PREPARATION_DEADLINE_NEGATIVE_UTILITY: return "PREPARATION_DEADLINE_NEGATIVE_UTILITY";
	case SAS_UWAI_WAR_PLAN_SEVERE_NEGATIVE_UTILITY: return "SEVERE_NEGATIVE_UTILITY";
	case SAS_UWAI_WAR_PLAN_TARGET_SWITCH: return "TARGET_SWITCH";
	case SAS_UWAI_WAR_PLAN_ATTACKED_RECENT_MATURED: return "ATTACKED_RECENT_MATURED";
	case SAS_UWAI_WAR_PLAN_ACTIVE_TYPE_SWITCH: return "ACTIVE_TYPE_SWITCH";
	case SAS_UWAI_WAR_PLAN_DIRECT_UTILITY_THRESHOLD: return "DIRECT_UTILITY_THRESHOLD";
	default: return "UNKNOWN";
	}
}

static char const* getSASGameRecordUWAIWarPlanDecisionAction(SASGameRecordUWAIWarPlanDecisionReason eReason)
{
	switch (eReason)
	{
	case SAS_UWAI_WAR_PLAN_ILLEGAL_TARGET:
	case SAS_UWAI_WAR_PLAN_IMMINENT_NEGATIVE_UTILITY:
	case SAS_UWAI_WAR_PLAN_IMMINENT_TIMEOUT:
	case SAS_UWAI_WAR_PLAN_PREPARATION_DEADLINE_NEGATIVE_UTILITY:
	case SAS_UWAI_WAR_PLAN_SEVERE_NEGATIVE_UTILITY:
		return "CANCEL";
	case SAS_UWAI_WAR_PLAN_VICTORY_DENIAL_DIRECT: return "DECLARE";
	case SAS_UWAI_WAR_PLAN_PREPARATION_DEADLINE_REACHED:
	case SAS_UWAI_WAR_PLAN_DIRECT_UTILITY_THRESHOLD:
		return "CONCLUDE";
	case SAS_UWAI_WAR_PLAN_TARGET_SWITCH: return "SWITCH_TARGET";
	case SAS_UWAI_WAR_PLAN_ATTACKED_RECENT_MATURED:
	case SAS_UWAI_WAR_PLAN_ACTIVE_TYPE_SWITCH:
		return "SWITCH_TYPE";
	default: return "UNKNOWN";
	}
}

// <!-- custom: Generic WAR_PLAN_CHANGED preserves authoritative before/after state, while this compact foreground-UWAI row preserves the already-computed causal decision that immediately triggered it.
// N/A comparison/decision fields remain -1/NO_TEAM; their meaning is reason-specific and no war evaluation or RNG is repeated here. (ChatGPT-5.6-Sol) -->
void logSASGameRecordUWAIWarPlanDecision(TeamTypes eAgent, TeamTypes eTarget, SASGameRecordUWAIWarPlanDecisionReason eReason, WarPlanTypes eOldWarPlan, WarPlanTypes eNewWarPlan, int iUtility, int iStateCounter, int iPrepTurnsRemaining, TeamTypes eComparisonTarget, int iComparisonUtility, int iDecisionValue, int iDecisionThreshold, int iVictoryDenialBoost)
{
	if (eAgent < 0 || eAgent >= MAX_TEAMS || eTarget < 0 || eTarget >= MAX_TEAMS)
		return;
	logSASGameRecord("GAME_RECORD_AI_WAR_PLAN_DECISION turn=%d planner=UWAI action=%s reason=%s agentTeam=%d targetTeam=%d oldWarPlan=%s newWarPlan=%s utility=%d stateCounter=%d prepTurnsRemaining=%d comparisonTargetTeam=%d comparisonUtility=%d decisionValue=%d decisionThreshold=%d victoryDenialBoost=%d",
		GC.getGame().getGameTurn(), getSASGameRecordUWAIWarPlanDecisionAction(eReason), getSASGameRecordUWAIWarPlanDecisionReason(eReason),
		eAgent, eTarget, getSASWarPlanType(eOldWarPlan), getSASWarPlanType(eNewWarPlan), iUtility, iStateCounter, iPrepTurnsRemaining,
		eComparisonTarget, iComparisonUtility, iDecisionValue, iDecisionThreshold, iVictoryDenialBoost);
}

void logSASGameRecordWarPlanChanged(TeamTypes eTeam, TeamTypes eTarget, WarPlanTypes eOldWarPlan, WarPlanTypes eNewWarPlan, bool bWar, int iOldStateCounter)
{
	if (eTeam < 0 || eTeam >= MAX_TEAMS || eTarget < 0 || eTarget >= MAX_TEAMS)
		return;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=WAR_PLAN_CHANGED team=%d targetTeam=%d oldWarPlan=%s newWarPlan=%s bWar=%d atWar=%d oldStateCounter=%d ourWars=%d targetWars=%d",
		GC.getGame().getGameTurn(), eTeam, eTarget, getSASWarPlanType(eOldWarPlan), getSASWarPlanType(eNewWarPlan), bWar,
		GET_TEAM(eTeam).isAtWar(eTarget), iOldStateCounter, GET_TEAM(eTeam).getNumWars(true, true), GET_TEAM(eTarget).getNumWars(true, true));
}

// <!-- custom: CvTeam::addTeam is the authoritative team-merge boundary. Log both pre-merge member lists while the absorbed team still owns its players.
// Periodic team snapshots can then describe the resulting state without forcing a consumer to infer the exact merge turn. (ChatGPT-5.6-Sol) -->
void logSASGameRecordTeamMerged(TeamTypes eSurvivingTeam, TeamTypes eAbsorbedTeam)
{
	if (eSurvivingTeam < 0 || eSurvivingTeam >= MAX_TEAMS || eAbsorbedTeam < 0 || eAbsorbedTeam >= MAX_TEAMS || eSurvivingTeam == eAbsorbedTeam)
		return;
	int iSurvivingPlayerCount = 0;
	int iAbsorbedPlayerCount = 0;
	CvString const szSurvivingPlayers = getSASGameRecordTeamAssignedPlayers(eSurvivingTeam, iSurvivingPlayerCount);
	CvString const szAbsorbedPlayers = getSASGameRecordTeamAssignedPlayers(eAbsorbedTeam, iAbsorbedPlayerCount);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=TEAM_MERGED survivingTeam=%d absorbedTeam=%d survivingPlayersBefore=%s absorbedPlayers=%s survivingPlayerCountBefore=%d absorbedPlayerCount=%d resultingPlayerCount=%d",
			GC.getGame().getGameTurn(), eSurvivingTeam, eAbsorbedTeam, szSurvivingPlayers.GetCString(),
			szAbsorbedPlayers.GetCString(), iSurvivingPlayerCount, iAbsorbedPlayerCount, iSurvivingPlayerCount + iAbsorbedPlayerCount);
}

void logSASGameRecordTeamMet(TeamTypes eTeam, TeamTypes eOtherTeam, bool bNewDiplo, int iX1, int iY1, int iX2, int iY2, CvPlot const* pTeamContactPlot, CvPlot const* pOtherContactPlot)
{
	bool const bMeetDataPlot1Valid = (iX1 >= 0 && iY1 >= 0 && iX1 < GC.getMap().getGridWidth() && iY1 < GC.getMap().getGridHeight());
	bool const bMeetDataPlot2Valid = (iX2 >= 0 && iY2 >= 0 && iX2 < GC.getMap().getGridWidth() && iY2 < GC.getMap().getGridHeight());
	// <!-- custom: FirstContactData may leave coordinates meaningless when the corresponding validity test fails.
	// Preserve the validity flag, but serialize invalid pairs canonically as -1,-1 instead of leaking uninitialized values into the log. (ChatGPT-5.6-Sol) -->
	int const iLoggedX1 = (bMeetDataPlot1Valid ? iX1 : -1);
	int const iLoggedY1 = (bMeetDataPlot1Valid ? iY1 : -1);
	int const iLoggedX2 = (bMeetDataPlot2Valid ? iX2 : -1);
	int const iLoggedY2 = (bMeetDataPlot2Valid ? iY2 : -1);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=TEAM_MET team=%d otherTeam=%d bNewDiplo=%d teamMembers=%s otherMembers=%s meetDataPlot1=%d,%d meetDataPlot1Valid=%d meetDataPlot2=%d,%d meetDataPlot2Valid=%d teamContactPlot=%d,%d otherTeamContactPlot=%d,%d",
		GC.getGame().getGameTurn(), eTeam, eOtherTeam, bNewDiplo, getSASGameRecordTeamMembers(eTeam).GetCString(),
		getSASGameRecordTeamMembers(eOtherTeam).GetCString(), iLoggedX1, iLoggedY1, bMeetDataPlot1Valid, iLoggedX2, iLoggedY2,
		bMeetDataPlot2Valid, pTeamContactPlot == NULL ? -1 : pTeamContactPlot->getX(),
		pTeamContactPlot == NULL ? -1 : pTeamContactPlot->getY(), pOtherContactPlot == NULL ? -1 : pOtherContactPlot->getX(),
		pOtherContactPlot == NULL ? -1 : pOtherContactPlot->getY());
}

void logSASGameRecordPlayerGoldTrade(PlayerTypes eFromPlayer, PlayerTypes eToPlayer, int iAmount)
{
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=GOLD_TRADE from=%d to=%d amount=%d", GC.getGame().getGameTurn(), eFromPlayer, eToPlayer, iAmount);
}

// <!-- custom: Recorder policy, not diplomacy AI: identify the DiploEvent values whose generic pre-event row is replaced by a richer post-resolution state-delta row.
// Keep the taxonomy with the recorder schema instead of teaching CvPlayer gameplay code how SASGameRecord groups events. (ChatGPT-5.6-Sol) -->
bool isSASGameRecordResolvedDiploInteraction(DiploEventTypes eDiploEvent)
{
	switch (eDiploEvent)
	{
	case DIPLOEVENT_GIVE_HELP:
	case DIPLOEVENT_REFUSED_HELP:
	case DIPLOEVENT_ACCEPT_DEMAND:
	case DIPLOEVENT_REJECTED_DEMAND:
	case DIPLOEVENT_CONVERT:
	case DIPLOEVENT_NO_CONVERT:
	case DIPLOEVENT_REVOLUTION:
	case DIPLOEVENT_NO_REVOLUTION:
	case DIPLOEVENT_JOIN_WAR:
	case DIPLOEVENT_NO_JOIN_WAR:
	case DIPLOEVENT_STOP_TRADING:
	case DIPLOEVENT_NO_STOP_TRADING:
	case DIPLOEVENT_ASK_HELP:
	case DIPLOEVENT_MADE_DEMAND:
		return true;
	default:
		return false;
	}
}

void captureSASGameRecordDiploRelationState(PlayerTypes eActor, PlayerTypes eOther, SASGameRecordDiploRelationState& kState)
{
	CvPlayerAI const& kActor = GET_PLAYER(eActor);
	CvPlayerAI const& kOther = GET_PLAYER(eOther);
	kState.iActorAttitude = kActor.AI_getAttitudeVal(eOther);
	kState.iOtherAttitude = kOther.AI_getAttitudeVal(eActor);
	for (int iMemory = 0; iMemory < NUM_MEMORY_TYPES; iMemory++)
	{
		MemoryTypes const eMemory = (MemoryTypes)iMemory;
		kState.aiActorMemory[iMemory] = kActor.AI_getMemoryCount(eOther, eMemory);
		kState.aiOtherMemory[iMemory] = kOther.AI_getMemoryCount(eActor, eMemory);
	}
	kState.eActorWarPlan = GET_TEAM(kActor.getTeam()).AI_getWarPlan(kOther.getTeam());
	kState.eOtherWarPlan = GET_TEAM(kOther.getTeam()).AI_getWarPlan(kActor.getTeam());
	kState.bAtWar = GET_TEAM(kActor.getTeam()).isAtWar(kOther.getTeam());
}

// <!-- custom: These semantic labels are SASGameRecord schema, not reusable gameplay enums.
// Keep the private mapping beside the row formatter; CvGameCoreUtils retains only generic raw enum-token helpers such as getSASDiploEventType. (ChatGPT-5.6-Sol) -->
static char const* getSASGameRecordDiploInteractionType(DiploEventTypes eDiploEvent)
{
	switch (eDiploEvent)
	{
	case DIPLOEVENT_GIVE_HELP:
	case DIPLOEVENT_REFUSED_HELP:
	case DIPLOEVENT_ASK_HELP:
		return "HELP";
	case DIPLOEVENT_ACCEPT_DEMAND:
	case DIPLOEVENT_REJECTED_DEMAND:
	case DIPLOEVENT_MADE_DEMAND:
		return "DEMAND";
	case DIPLOEVENT_CONVERT:
	case DIPLOEVENT_NO_CONVERT:
		return "RELIGION";
	case DIPLOEVENT_REVOLUTION:
	case DIPLOEVENT_NO_REVOLUTION:
		return "CIVIC";
	case DIPLOEVENT_JOIN_WAR:
	case DIPLOEVENT_NO_JOIN_WAR:
		return "JOIN_WAR";
	case DIPLOEVENT_STOP_TRADING:
	case DIPLOEVENT_NO_STOP_TRADING:
		return "STOP_TRADING";
	default:
		return "-";
	}
}

static char const* getSASGameRecordDiploInteractionOutcome(DiploEventTypes eDiploEvent, int iData1)
{
	switch (eDiploEvent)
	{
	case DIPLOEVENT_GIVE_HELP:
	case DIPLOEVENT_ACCEPT_DEMAND:
	case DIPLOEVENT_CONVERT:
	case DIPLOEVENT_REVOLUTION:
	case DIPLOEVENT_JOIN_WAR:
	case DIPLOEVENT_STOP_TRADING:
		return "ACCEPTED";
	case DIPLOEVENT_REFUSED_HELP:
	case DIPLOEVENT_REJECTED_DEMAND:
	case DIPLOEVENT_NO_CONVERT:
	case DIPLOEVENT_NO_REVOLUTION:
	case DIPLOEVENT_NO_JOIN_WAR:
	case DIPLOEVENT_NO_STOP_TRADING:
		return "REFUSED";
	case DIPLOEVENT_ASK_HELP:
	case DIPLOEVENT_MADE_DEMAND:
		return (iData1 > 0 ? "ACCEPTED" : "REFUSED");
	default:
		return "-";
	}
}

static CvString getSASGameRecordDiploInteractionSubject(PlayerTypes eActor, DiploEventTypes eDiploEvent, int iData1)
{
	switch (eDiploEvent)
	{
	case DIPLOEVENT_CONVERT:
	case DIPLOEVENT_NO_CONVERT:
		return GET_PLAYER(eActor).getStateReligion() == NO_RELIGION ? CvString("-") : CvString(GC.getInfo(GET_PLAYER(eActor).getStateReligion()).getType());
	case DIPLOEVENT_REVOLUTION:
	case DIPLOEVENT_NO_REVOLUTION:
		return GET_PLAYER(eActor).getFavoriteCivic() == NO_CIVIC ? CvString("-") : CvString(GC.getInfo(GET_PLAYER(eActor).getFavoriteCivic()).getType());
	case DIPLOEVENT_JOIN_WAR:
	case DIPLOEVENT_NO_JOIN_WAR:
	case DIPLOEVENT_STOP_TRADING:
	case DIPLOEVENT_NO_STOP_TRADING:
		return getSASTeamDiagnosticText((TeamTypes)iData1);
	default:
		return CvString("-");
	}
}

static CvString getSASGameRecordDiploMemoryChanges(PlayerTypes eActor, PlayerTypes eOther, SASGameRecordDiploRelationState const& kBefore, SASGameRecordDiploRelationState const& kAfter)
{
	CvString szChanges;
	for (int iMemory = 0; iMemory < NUM_MEMORY_TYPES; iMemory++)
	{
		MemoryTypes const eMemory = (MemoryTypes)iMemory;
		if (kBefore.aiActorMemory[iMemory] != kAfter.aiActorMemory[iMemory])
		{
			CvString szItem;
			szItem.Format(szChanges.empty() ? "P%d:%s:%d>%d" : ",P%d:%s:%d>%d", eActor, getSASMemoryType(eMemory), kBefore.aiActorMemory[iMemory], kAfter.aiActorMemory[iMemory]);
			szChanges += szItem;
		}
		if (kBefore.aiOtherMemory[iMemory] != kAfter.aiOtherMemory[iMemory])
		{
			CvString szItem;
			szItem.Format(szChanges.empty() ? "P%d:%s:%d>%d" : ",P%d:%s:%d>%d", eOther, getSASMemoryType(eMemory), kBefore.aiOtherMemory[iMemory], kAfter.aiOtherMemory[iMemory]);
			szChanges += szItem;
		}
	}
	return getSASDiagnosticOrDash(szChanges);
}

void logSASGameRecordResolvedDiploInteraction(PlayerTypes eActor, DiploEventTypes eDiploEvent, PlayerTypes eOther, int iData1, SASGameRecordDiploRelationState const& kBefore, SASGameRecordDiploRelationState const& kAfter)
{
	bool const bOtherRequested = (eDiploEvent == DIPLOEVENT_ASK_HELP || eDiploEvent == DIPLOEVENT_MADE_DEMAND);
	PlayerTypes const eRequester = (bOtherRequested ? eOther : eActor);
	PlayerTypes const eResponder = (bOtherRequested ? eActor : eOther);
	int const iRequesterAttitudeBefore = (bOtherRequested ? kBefore.iOtherAttitude : kBefore.iActorAttitude);
	int const iRequesterAttitudeAfter = (bOtherRequested ? kAfter.iOtherAttitude : kAfter.iActorAttitude);
	int const iResponderAttitudeBefore = (bOtherRequested ? kBefore.iActorAttitude : kBefore.iOtherAttitude);
	int const iResponderAttitudeAfter = (bOtherRequested ? kAfter.iActorAttitude : kAfter.iOtherAttitude);
	WarPlanTypes const eRequesterWarPlanBefore = (bOtherRequested ? kBefore.eOtherWarPlan : kBefore.eActorWarPlan);
	WarPlanTypes const eRequesterWarPlanAfter = (bOtherRequested ? kAfter.eOtherWarPlan : kAfter.eActorWarPlan);
	WarPlanTypes const eResponderWarPlanBefore = (bOtherRequested ? kBefore.eActorWarPlan : kBefore.eOtherWarPlan);
	WarPlanTypes const eResponderWarPlanAfter = (bOtherRequested ? kAfter.eActorWarPlan : kAfter.eOtherWarPlan);

	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=DIPLO_INTERACTION requester=%d responder=%d interaction=%s outcome=%s subject=%s event=%s requesterAttitudeValue=%d>%d responderAttitudeValue=%d>%d memoryChanges=%s requesterWarPlan=%s>%s responderWarPlan=%s>%s atWar=%d>%d",
		GC.getGame().getGameTurn(), eRequester, eResponder, getSASGameRecordDiploInteractionType(eDiploEvent),
		getSASGameRecordDiploInteractionOutcome(eDiploEvent, iData1),
		getSASGameRecordDiploInteractionSubject(eActor, eDiploEvent, iData1).GetCString(), getSASDiploEventType(eDiploEvent),
		iRequesterAttitudeBefore, iRequesterAttitudeAfter, iResponderAttitudeBefore, iResponderAttitudeAfter,
		getSASGameRecordDiploMemoryChanges(eActor, eOther, kBefore, kAfter).GetCString(), getSASWarPlanType(eRequesterWarPlanBefore),
		getSASWarPlanType(eRequesterWarPlanAfter), getSASWarPlanType(eResponderWarPlanBefore), getSASWarPlanType(eResponderWarPlanAfter),
		kBefore.bAtWar ? 1 : 0, kAfter.bAtWar ? 1 : 0);
}

// <!-- custom: CvPlayer::AI_considerOfferExternal is the EXE's submitted human->AI offer boundary rather than a speculative internal valuation call.
// Preserve rejected packages too; accepted packages will additionally produce the existing DIPLO_DEAL row. (ChatGPT-5.6-Sol) -->
void logSASGameRecordDiploOfferEvaluated(PlayerTypes eProposer, PlayerTypes eResponder, CLinkList<TradeData> const& kProposerGives, CLinkList<TradeData> const& kResponderGives, int iChange, bool bAccepted, SASGameRecordDiploRelationState const& kBefore, SASGameRecordDiploRelationState const& kAfter)
{
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=DIPLO_OFFER_EVALUATED proposer=%d responder=%d outcome=%s change=%d proposerGives=%s responderGives=%s proposerAttitudeValue=%d>%d responderAttitudeValue=%d>%d memoryChanges=%s atWar=%d",
		GC.getGame().getGameTurn(), eProposer, eResponder, bAccepted ? "ACCEPTED" : "REJECTED", iChange,
		getSASTradeListText(kProposerGives, eProposer).GetCString(), getSASTradeListText(kResponderGives, eResponder).GetCString(),
		kBefore.iOtherAttitude, kAfter.iOtherAttitude, kBefore.iActorAttitude, kAfter.iActorAttitude,
		getSASGameRecordDiploMemoryChanges(eResponder, eProposer, kBefore, kAfter).GetCString(), kBefore.bAtWar ? 1 : 0);
}

// <!-- custom: The EXE counterproposal wrapper exposes the exact submitted package plus the additions selected by the AI.
// Log only this resolved boundary, not the many internal candidate/value calculations used to construct it. (ChatGPT-5.6-Sol) -->
void logSASGameRecordDiploCounterProposal(PlayerTypes eProposer, PlayerTypes eResponder, CLinkList<TradeData> const& kOriginalProposerGives, CLinkList<TradeData> const& kOriginalResponderGives, CLinkList<TradeData> const& kProposerAdds, CLinkList<TradeData> const& kResponderAdds, bool bProposed)
{
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=DIPLO_COUNTERPROPOSAL proposer=%d responder=%d outcome=%s originalProposerGives=%s originalResponderGives=%s proposerAdds=%s responderAdds=%s",
		GC.getGame().getGameTurn(), eProposer, eResponder, bProposed ? "PROPOSED" : "NONE",
		getSASTradeListText(kOriginalProposerGives, eProposer).GetCString(),
		getSASTradeListText(kOriginalResponderGives, eResponder).GetCString(), getSASTradeListText(kProposerAdds, eProposer).GetCString(),
		getSASTradeListText(kResponderAdds, eResponder).GetCString());
}

// <!-- custom: The unmoddable EXE does not expose a clean DLL rejection callback carrying AI->human ordinary trade items.
// BUG's resolved DealRejected UI event does, so log that exact package here without importing Python/localized formatting into the schema. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIToHumanOfferRejected(PlayerTypes eProposer, PlayerTypes eResponder, CLinkList<TradeData> const& kProposerGives, CLinkList<TradeData> const& kResponderGives)
{
	CvPlayerAI const& kProposer = GET_PLAYER(eProposer);
	CvPlayerAI const& kResponder = GET_PLAYER(eResponder);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=DIPLO_OFFER_REJECTED proposer=%d responder=%d proposerGives=%s responderGives=%s proposerAttitudeValue=%d responderAttitudeValue=%d atWar=%d",
		GC.getGame().getGameTurn(), eProposer, eResponder, getSASTradeListText(kProposerGives, eProposer).GetCString(),
		getSASTradeListText(kResponderGives, eResponder).GetCString(), kProposer.AI_getAttitudeVal(eResponder),
		kResponder.AI_getAttitudeVal(eProposer), GET_TEAM(kProposer.getTeam()).isAtWar(kResponder.getTeam()) ? 1 : 0);
}

// <!-- custom: This is the compact missing first link before existing offer/deal/interaction rows: record only a realized proactive contact/deal, not every failed AI_contactRoll or discarded candidate.
// All package/subject inputs are already final live state; no trade valuation, contact roll, RNG or candidate search is repeated for logging. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIDiploContactIntent(CvPlayerAI const& kPlayer, PlayerTypes eTarget, ContactTypes eContact, TradeData const* pSubject, CLinkList<TradeData> const* pAIGives, CLinkList<TradeData> const* pAIReceives)
{
	CvString const szSubject = (pSubject == NULL ? CvString("-") : getSASTradeDataText(*pSubject, kPlayer.getID()));
	CvString const szAIGives = (pAIGives == NULL ? CvString("-") : getSASTradeListText(*pAIGives, kPlayer.getID()));
	CvString const szAIReceives = (pAIReceives == NULL ? CvString("-") : getSASTradeListText(*pAIReceives, eTarget));
	CvPlayerAI const& kTarget = GET_PLAYER(eTarget);
	bool const bTargetHuman = kTarget.isHuman();
	logSASGameRecord("GAME_RECORD_AI_DIPLO_CONTACT turn=%d player=%d team=%d targetPlayer=%d targetTeam=%d targetHuman=%d contact=%s delivery=%s attitudeValue=%d subject=%s aiGives=%s aiReceives=%s",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), eTarget, kTarget.getTeam(), bTargetHuman ? 1 : 0,
		getSASContactType(eContact), bTargetHuman ? "HUMAN_CONTACT" : "AI_DEAL", kPlayer.AI_getAttitudeVal(eTarget),
		szSubject.GetCString(), szAIGives.GetCString(), szAIReceives.GetCString());
}

// <!-- custom: Help/tribute origin is caller state that disappears once the shared request helper succeeds; keep the vocabulary recorder-local and leave revision-100 contact rows authoritative for the final package. (ChatGPT-5.6-Sol) -->
static char const* getSASGameRecordAIHumanRequestOrigin(SASGameRecordAIHumanRequestOrigin eOrigin)
{
	switch (eOrigin)
	{
	case SAS_AI_HUMAN_REQUEST_AI_DO_DIPLO: return "AI_DO_DIPLO";
	case SAS_AI_HUMAN_REQUEST_UWAI_AMEND_TENSIONS: return "UWAI_AMEND_TENSIONS";
	default: return "UNKNOWN";
	}
}

static char const* getSASGameRecordAIDemandType(AIDemandTypes eDemand)
{
	switch (eDemand)
	{
	case DEMAND_GOLD: return "DEMAND_GOLD";
	case DEMAND_MAP: return "DEMAND_MAP";
	case DEMAND_TECH: return "DEMAND_TECH";
	case DEMAND_BONUS: return "DEMAND_BONUS";
	case DEMAND_GOLD_PER_TURN: return "DEMAND_GOLD_PER_TURN";
	case DEMAND_CITY: return "DEMAND_CITY";
	default: return "UNKNOWN";
	}
}

// <!-- custom: Preserve the city-vs-tech chooser behind one realized AI help request.
// The generic CONTACT_ASK_FOR_HELP row remains authoritative for the final package; no chooser valuation or RNG is repeated here. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIHelpRequest(CvPlayerAI const& kPlayer, PlayerTypes eHuman, SASGameRecordAIHelpRequestContext const& kContext)
{
	CvPlayerAI const& kHuman = GET_PLAYER(eHuman);
	TradeData const kSelected(kContext.eSelectedItemType, kContext.iSelectedItemData);
	logSASGameRecord("GAME_RECORD_AI_HELP_REQUEST turn=%d player=%d team=%d human=%d humanTeam=%d origin=%s attitudeValue=%d humanAssets=%d aiTeamAssets=%d bestTech=%s techValue=%d bestCityId=%d cityValue=%d bestCityLiberation=%d cityChoiceRoll=%d selected=%s:%s",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), eHuman, kHuman.getTeam(), getSASGameRecordAIHumanRequestOrigin(kContext.eOrigin),
		kPlayer.AI_getAttitudeVal(eHuman), kHuman.getAssets(), GET_TEAM(kPlayer.getTeam()).getAssets(), getSASGameRecordTechType(kContext.eBestTech), kContext.iTechValue,
		kContext.iBestCityId, kContext.iCityValue, kContext.bBestCityLiberation ? 1 : 0, kContext.iCityChoiceRoll, getSASTradeItemType(kContext.eSelectedItemType),
		getSASTradeDataText(kSelected, eHuman).GetCString());
}

// <!-- custom: Preserve the realized tribute-request route and final live threshold comparison, plus only the technology/map chooser state that revision-100 contact packages cannot reconstruct.
// Revision 87's bonus-demand row remains authoritative for resource-candidate detail. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAITributeRequest(CvPlayerAI const& kPlayer, PlayerTypes eHuman, SASGameRecordAITributeRequestContext const& kContext)
{
	CvPlayerAI const& kHuman = GET_PLAYER(eHuman);
	logSASGameRecord("GAME_RECORD_AI_TRIBUTE_REQUEST turn=%d player=%d team=%d human=%d humanTeam=%d origin=%s attitudeValue=%d demand=%s minValueX100=%d finalDealValue=%d mostUsefulTech=%s selectedTech=%s selectedTechSortScore=%d selectedTechTradeValue=%d mapTradeValue=%d mapAdded=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), eHuman, kHuman.getTeam(), getSASGameRecordAIHumanRequestOrigin(kContext.eOrigin),
		kPlayer.AI_getAttitudeVal(eHuman), getSASGameRecordAIDemandType(kContext.eDemand), kContext.iMinValueX100, kContext.iFinalDealValue,
		getSASGameRecordTechType(kContext.eMostUsefulTech), getSASGameRecordTechType(kContext.eSelectedTech), kContext.iSelectedTechSortScore,
		kContext.iSelectedTechTradeValue, kContext.iMapTradeValue, kContext.bMapAdded ? 1 : 0);
}

static char const* getSASGameRecordAIGiveHelpOrigin(SASGameRecordAIGiveHelpOrigin eOrigin)
{
	switch (eOrigin)
	{
	case SAS_AI_GIVE_HELP_RELATION_RESOURCE: return "RELATION_RESOURCE";
	case SAS_AI_GIVE_HELP_RELATION_TECH: return "RELATION_TECH";
	case SAS_AI_GIVE_HELP_PROACTIVE_TECH: return "PROACTIVE_TECH";
	default: return "UNKNOWN";
	}
}

// <!-- custom: Preserve which live GIVE_HELP algorithm produced a realized gift and the chooser/gate values that disappear after the generic revision-100 package is emitted.
// Relationship flags and cheap winner facts are derived only after the caller's level-2 gate; no candidate scan, valuation or synchronized RNG is repeated. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIGiveHelpDecision(CvPlayerAI const& kPlayer, PlayerTypes eTarget, SASGameRecordAIGiveHelpOrigin eOrigin, BonusTypes eSelectedBonus, TechTypes eSelectedTech, int iSelectionScore, int iTechScoreRatioX1000, int iGiftProbX1000, int iContactProbMultX1000)
{
	CvPlayerAI const& kTarget = GET_PLAYER(eTarget);
	TeamTypes const eOurTeam = kPlayer.getTeam();
	TeamTypes const eTargetTeam = kTarget.getTeam();
	bool const bSameTeam = (eOurTeam == eTargetTeam);
	bool const bTargetVassal = (!bSameTeam && GET_TEAM(eTargetTeam).isVassal(eOurTeam));
	bool const bGiverVassal = (!bSameTeam && GET_TEAM(eOurTeam).isVassal(eTargetTeam));
	int const iTargetHadBonus = (eSelectedBonus == NO_BONUS ? -1 : (kTarget.getNumAvailableBonuses(eSelectedBonus) > 0 ? 1 : 0));
	int const iSelectedTechCost = (eSelectedTech == NO_TECH ? -1 : GC.getInfo(eSelectedTech).getResearchCost());
	bool const bProactive = (eOrigin == SAS_AI_GIVE_HELP_PROACTIVE_TECH);
	// <!-- custom: attitudeValue is chooser provenance only for proactive gifts. RELATION_RESOURCE does not use attitude here, while RELATION_TECH can deliberately use AI_getAttitude(..., false); the normal forced-attitude lookup would therefore be misleading for those origins.
	// Keep this lookup proactive-only (-1 otherwise), matching the proactive-only threshold/assets fields and avoiding an unnecessary recorder lookup. (ChatGPT-5.6-Sol) -->
	int const iAttitudeValue = (bProactive ? kPlayer.AI_getAttitudeVal(eTarget) : -1);
	AttitudeTypes const eAttitude = (bProactive ? CvPlayerAI::AI_getAttitudeFromValue(iAttitudeValue) : NO_ATTITUDE);
	AttitudeTypes const eAttitudeThreshold = (bProactive ? (AttitudeTypes)GC.getInfo(kPlayer.getPersonalityType()).getNoGiveHelpAttitudeThreshold() : NO_ATTITUDE);
	int const iOurAssets = (bProactive ? GET_TEAM(eOurTeam).getAssets() : -1);
	int const iTargetAssets = (bProactive ? GET_TEAM(eTargetTeam).getAssets() : -1);
	logSASGameRecord("GAME_RECORD_AI_GIVE_HELP_DECISION turn=%d player=%d team=%d targetPlayer=%d targetTeam=%d targetHuman=%d origin=%s sameTeam=%d targetVassal=%d giverVassal=%d attitudeValue=%d proactiveAttitude=%s proactiveAttitudeThreshold=%s giverAssets=%d targetAssets=%d selectedBonus=%s targetHadBonus=%d selectedTech=%s selectedTechCost=%d selectionScore=%d techScoreRatioX1000=%d giftProbX1000=%d contactProbMultX1000=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), eOurTeam, eTarget, eTargetTeam, kTarget.isHuman() ? 1 : 0,
		getSASGameRecordAIGiveHelpOrigin(eOrigin), bSameTeam ? 1 : 0, bTargetVassal ? 1 : 0, bGiverVassal ? 1 : 0,
		iAttitudeValue, getSASGameRecordAttitudeType(eAttitude), getSASGameRecordAttitudeType(eAttitudeThreshold),
		iOurAssets, iTargetAssets, getSASGameRecordBonusType(eSelectedBonus), iTargetHadBonus, getSASGameRecordTechType(eSelectedTech),
		iSelectedTechCost, iSelectionScore, iTechScoreRatioX1000, iGiftProbX1000, iContactProbMultX1000);
}

static char const* getSASGameRecordAITechTradeOrigin(SASGameRecordAITechTradeOrigin eOrigin)
{
	switch (eOrigin)
	{
	case SAS_AI_TECH_TRADE_RANDOM_DIRECT: return "RANDOM_TECH_DIRECT";
	case SAS_AI_TECH_TRADE_RANDOM_COUNTERPROPOSE: return "RANDOM_TECH_COUNTERPROPOSE";
	case SAS_AI_TECH_TRADE_PROGRESS_GOLD: return "PROGRESS_TECH_GOLD";
	default: return "UNKNOWN";
	}
}

// <!-- custom: Preserve the selected random receive-tech score, value-matched give-tech result and AdvC progress-purchase alternative only for a realized CONTACT_TRADE_TECH.
// The revision-100 contact row remains authoritative for the final package (including any counterproposal additions); this formatter performs no candidate scan, trade valuation or synchronized RNG. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAITechTradeDecision(CvPlayerAI const& kPlayer, PlayerTypes eTarget, SASGameRecordAITechTradeOrigin eOrigin, int iContactProbMultX1000, int iBestKnownTechScorePercent, TechTypes eRandomReceiveTech, int iRandomReceiveScore, bool bRandomReceiveLocusSuppressed, TechTypes eProgressTech, int iProgressResearchPoints, bool bProgressLocusSuppressed, TechTypes eGiveTech, int iOurReceiveTechValue, int iTargetReceiveTechValue, int iGiveMatchDeltaValue, int iProgressReceiveTechValue, int iProgressMaxGold)
{
	CvPlayerAI const& kTarget = GET_PLAYER(eTarget);
	int const iProgressResearchPointsLogged = (eProgressTech == NO_TECH ? -1 : iProgressResearchPoints);
	logSASGameRecord("GAME_RECORD_AI_TECH_TRADE_DECISION turn=%d player=%d team=%d targetPlayer=%d targetTeam=%d targetHuman=%d origin=%s contactProbMultX1000=%d bestKnownTechScorePercent=%d randomReceiveTech=%s randomReceiveScore=%d randomReceiveLocusSuppressed=%d progressTech=%s progressResearchPoints=%d progressLocusSuppressed=%d giveTech=%s ourReceiveTechValue=%d targetReceiveTechValue=%d giveMatchDeltaValue=%d progressReceiveTechValue=%d progressMaxGold=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), eTarget, kTarget.getTeam(), kTarget.isHuman() ? 1 : 0,
		getSASGameRecordAITechTradeOrigin(eOrigin), iContactProbMultX1000, iBestKnownTechScorePercent,
		getSASGameRecordTechType(eRandomReceiveTech), (eRandomReceiveTech == NO_TECH ? -1 : iRandomReceiveScore), bRandomReceiveLocusSuppressed ? 1 : 0,
		getSASGameRecordTechType(eProgressTech), iProgressResearchPointsLogged, bProgressLocusSuppressed ? 1 : 0, getSASGameRecordTechType(eGiveTech),
		iOurReceiveTechValue, iTargetReceiveTechValue, iGiveMatchDeltaValue, iProgressReceiveTechValue, iProgressMaxGold);
}

static char const* getSASGameRecordAIDealCancellationReason(SASGameRecordAIDealCancellationReason eReason)
{
	switch (eReason)
	{
	case SAS_AI_DEAL_CANCEL_OFFER_REJECTED_RENEGOTIATE: return "OFFER_REJECTED_RENEGOTIATE";
	case SAS_AI_DEAL_CANCEL_DUAL_DENIAL: return "DUAL_DENIAL";
	case SAS_AI_DEAL_CANCEL_AI_RESOURCE_DENIAL: return "AI_RESOURCE_DENIAL";
	case SAS_AI_DEAL_CANCEL_OTHER_RESOURCE_JOKING: return "OTHER_RESOURCE_JOKING";
	case SAS_AI_DEAL_CANCEL_GPT_LIMIT: return "GPT_LIMIT";
	default: return "UNKNOWN";
	}
}

// <!-- custom: Preserve only the realized AI_doDeals cancellation boundary. The authoritative DIPLO_DEAL_ENDED action still owns the deal payload; this row keeps the otherwise-lost decision cause and already-computed denial/GPT-cap context.
// Trigger formatting and the static dual-denial probability are recorder-only and run only after the caller's level-2 gate; no AI_considerOffer, trade denial, bonus valuation or RNG is repeated. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIDealCancellationDecision(CvPlayerAI const& kPlayer, PlayerTypes eOther, CvDeal const& kDeal, SASGameRecordAIDealCancellationReason eReason, TradeData const* pTriggerItem, bool bTriggerFromAI, DenialTypes eDenial, int iGptOverdraftBefore, int iDealGpt, int iGptOverdraftAfter)
{
	CvPlayerAI const& kOther = GET_PLAYER(eOther);
	CvString szTriggerItem("-");
	char const* szTriggerSide = "-";
	if (pTriggerItem != NULL)
	{
		szTriggerSide = (bTriggerFromAI ? "AI_GIVES" : "OTHER_GIVES");
		CvString const szData = getSASTradeDataText(*pTriggerItem, bTriggerFromAI ? kPlayer.getID() : eOther);
		szTriggerItem.Format("%s:%s", getSASTradeItemType(pTriggerItem->m_eItemType), szData.GetCString());
	}
	char const* szDenial = (eDenial == NO_DENIAL ? "-" : getSASGameRecordDenialType(eDenial));
	int iDualCancelChanceX1000 = -1;
	if (eReason == SAS_AI_DEAL_CANCEL_DUAL_DENIAL && pTriggerItem != NULL)
	{
		iDualCancelChanceX1000 =
				(pTriggerItem->m_eItemType == TRADE_DEFENSIVE_PACT && eDenial == DENIAL_JOKING ? 1000 :
				(eDenial == DENIAL_WORST_ENEMY ? 400 : 200));
	}
	logSASGameRecord("GAME_RECORD_AI_DEAL_CANCELLATION_DECISION turn=%d player=%d team=%d otherPlayer=%d otherTeam=%d otherHuman=%d dealId=%d dealAge=%d vassalDeal=%d reason=%s triggerSide=%s triggerItem=%s denial=%s dualCancelChanceX1000=%d gptOverdraftBefore=%d dealGpt=%d gptOverdraftAfter=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), eOther, kOther.getTeam(), kOther.isHuman() ? 1 : 0,
		kDeal.getID(), kDeal.getAge(), kDeal.isVassalDeal() ? 1 : 0, getSASGameRecordAIDealCancellationReason(eReason),
		szTriggerSide, szTriggerItem.GetCString(), szDenial, iDualCancelChanceX1000, iGptOverdraftBefore, iDealGpt, iGptOverdraftAfter);
}

static char const* getSASGameRecordDealInvalidationReason(SASGameRecordDealInvalidationReason eReason)
{
	switch (eReason)
	{
	case SAS_DEAL_INVALID_RESOURCE_SUPPLY_DEFICIT: return "RESOURCE_SUPPLY_DEFICIT";
	case SAS_DEAL_INVALID_TRADE_NETWORK_LOST: return "TRADE_NETWORK_LOST";
	case SAS_DEAL_INVALID_GIVER_BONUS_OBSOLETE: return "GIVER_BONUS_OBSOLETE";
	case SAS_DEAL_INVALID_RECIPIENT_BONUS_OBSOLETE: return "RECIPIENT_BONUS_OBSOLETE";
	case SAS_DEAL_INVALID_GPT_INSOLVENT: return "GPT_INSOLVENT";
	case SAS_DEAL_INVALID_PEACE_TREATY_EXPIRED: return "PEACE_TREATY_EXPIRED";
	default: return "UNKNOWN";
	}
}

// <!-- custom: Preserve CvDeal::verify's realized first-failure cause immediately before native teardown.
// DIPLO_DEAL_ENDED remains authoritative for the full payload; this formatter only serializes already-captured context and performs no validity query itself. (ChatGPT-5.6-Sol) -->
void logSASGameRecordDealInvalidation(CvDeal const& kDeal, SASGameRecordDealInvalidationContext const& kContext)
{
	CvString szTriggerItem("-");
	int iGptDue = -1;
	if (kContext.pTriggerItem != NULL)
	{
		CvString const szData = getSASTradeDataText(*kContext.pTriggerItem, kContext.eGiver);
		szTriggerItem.Format("%s:%s", getSASTradeItemType(kContext.pTriggerItem->m_eItemType), szData.GetCString());
		if (kContext.pTriggerItem->m_eItemType == TRADE_GOLD_PER_TURN)
			iGptDue = kContext.pTriggerItem->m_iData;
	}
	int const iGiverTeam = (kContext.eGiver == NO_PLAYER ? -1 : GET_PLAYER(kContext.eGiver).getTeam());
	int const iRecipientTeam = (kContext.eRecipient == NO_PLAYER ? -1 : GET_PLAYER(kContext.eRecipient).getTeam());
	logSASGameRecord("GAME_RECORD_DEAL_INVALIDATION turn=%d dealId=%d dealAge=%d first=%d second=%d reason=%s giver=%d giverTeam=%d recipient=%d recipientTeam=%d triggerItem=%s tradeableBonusCount=%d goldPercent=%d gold=%d gptDue=%d goldRate=%d",
		GC.getGame().getGameTurn(), kDeal.getID(), kDeal.getAge(), kDeal.getFirstPlayer(), kDeal.getSecondPlayer(), getSASGameRecordDealInvalidationReason(kContext.eReason),
		kContext.eGiver, iGiverTeam, kContext.eRecipient, iRecipientTeam, szTriggerItem.GetCString(), kContext.iTradeableBonusCount, kContext.iGoldPercent, kContext.iGold, iGptDue, kContext.iGoldRate);
}

// <!-- custom: Preserve the live reason for a realized AI request that a human join one of its wars.
// The generic CONTACT_JOIN_WAR row remains authoritative for the request subject; this row derives only cheap current counters/static context and never repeats contact/peace/target RNG or UWAI denial evaluation. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIJointWarRequest(CvPlayerAI const& kPlayer, PlayerTypes eHuman, TeamTypes eTarget, int iTargetScore, int iMeanAtWarTurnsX1000)
{
	CvTeamAI const& kOurTeam = GET_TEAM(kPlayer.getTeam());
	// <!-- custom: CvPlayerAI.cpp includes CoreAI.h, whose GET_TEAM overload accepts PlayerTypes and whose CvGamePlay dependency provides TEAMID; this recorder translation unit deliberately does not include those broad gameplay wrappers.
	// Spell out the player-to-team conversion here instead of copying that file-local shorthand or widening SASGameRecordLog's include dependencies just for one lookup. See also KI#436. (ChatGPT-5.6-Sol) -->
	CvTeamAI const& kHumanTeam = GET_TEAM(GET_PLAYER(eHuman).getTeam());
	int const iTargetAtWarCounter = kOurTeam.AI_getAtWarCounter(eTarget);
	int const iHumanPeaceCounter = kHumanTeam.AI_getAtPeaceCounter(eTarget);
	int const iTargetRandomScore = iTargetScore - std::min(20, iTargetAtWarCounter) * 1000;
	int const iContactRand = GC.getInfo(kPlayer.getPersonalityType()).getContactRand(CONTACT_JOIN_WAR);
	logSASGameRecord("GAME_RECORD_AI_JOINT_WAR_REQUEST turn=%d player=%d team=%d human=%d humanTeam=%d attitudeValue=%d meanAtWarTurnsX1000=%d contactRand=%d targetTeam=%d targetScore=%d targetRandomScore=%d targetAtWarCounter=%d humanPeaceCounter=%d uwai=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), eHuman, kHumanTeam.getID(), kPlayer.AI_getAttitudeVal(eHuman),
		iMeanAtWarTurnsX1000, iContactRand, eTarget, iTargetScore, iTargetRandomScore, iTargetAtWarCounter, iHumanPeaceCounter, getUWAI().isEnabled() ? 1 : 0);
}

// <!-- custom: Preserve why a realized STOP_TRADING request exists; the generic revision-100 contact row remains authoritative for the actual embargo subject.
// Redirect-only fields describe the already-selected joint-war target and cached UWAI denial; direct embargo contacts use explicit sentinel/NA values and no chooser/RNG work is repeated. (ChatGPT-5.6-Sol) -->
static char const* getSASGameRecordAIEmbargoRequestOrigin(SASGameRecordAIEmbargoRequestOrigin eOrigin)
{
	switch (eOrigin)
	{
	case SAS_AI_EMBARGO_DIRECT_CONTACT: return "DIRECT_CONTACT";
	case SAS_AI_EMBARGO_JOINT_WAR_ATTITUDE_REDIRECT: return "JOINT_WAR_ATTITUDE_REDIRECT";
	case SAS_AI_EMBARGO_JOINT_WAR_UWAI_REDIRECT: return "JOINT_WAR_UWAI_REDIRECT";
	default: return "UNKNOWN";
	}
}

void logSASGameRecordAIEmbargoRequest(CvPlayerAI const& kPlayer, PlayerTypes eHuman, SASGameRecordAIEmbargoRequestOrigin eOrigin, TeamTypes eJointWarTarget, int iJointWarTargetScore, DenialTypes eJointWarDenial)
{
	CvTeamAI const& kOurTeam = GET_TEAM(kPlayer.getTeam());
	// <!-- custom: This narrow recorder translation unit does not inherit CoreAI.h's PlayerTypes-aware GET_TEAM shorthand; spell out the player-to-team conversion as for the revision-107 joint-war logger. See also KI#436. (ChatGPT-5.6-Sol) -->
	TeamTypes const eHumanTeam = GET_PLAYER(eHuman).getTeam();
	TeamTypes const eEmbargoTarget = kOurTeam.AI_getWorstEnemy();
	// <!-- custom: A successful AI_proposeEmbargo necessarily selected the current worst enemy; assert that invariant before recording its realized target. (ChatGPT-5.6-Sol) -->
	FAssert(eEmbargoTarget != NO_TEAM);
	int const iTeamAttitudeHuman = kOurTeam.AI_getAttitudeVal(eHumanTeam);
	int const iTeamAttitudeTarget = kOurTeam.AI_getAttitudeVal(eEmbargoTarget);
	int const iJointWarAtWarCounter = (eJointWarTarget == NO_TEAM ? -1 : kOurTeam.AI_getAtWarCounter(eJointWarTarget));
	int const iJointWarRandomScore = (eJointWarTarget == NO_TEAM ? -1 : iJointWarTargetScore - std::min(20, iJointWarAtWarCounter) * 1000);
	char const* szJointWarDenial = (eOrigin == SAS_AI_EMBARGO_JOINT_WAR_UWAI_REDIRECT ? getSASGameRecordDenialType(eJointWarDenial) : "-");
	logSASGameRecord("GAME_RECORD_AI_EMBARGO_REQUEST turn=%d player=%d team=%d human=%d humanTeam=%d attitudeValue=%d origin=%s embargoTargetTeam=%d teamAttitudeHuman=%d teamAttitudeTarget=%d attitudeGap=%d jointWarTargetTeam=%d jointWarTargetScore=%d jointWarTargetRandomScore=%d jointWarTargetAtWarCounter=%d jointWarTargetIsEmbargoTarget=%d jointWarDenial=%s",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), eHuman, eHumanTeam, kPlayer.AI_getAttitudeVal(eHuman), getSASGameRecordAIEmbargoRequestOrigin(eOrigin),
		eEmbargoTarget, iTeamAttitudeHuman, iTeamAttitudeTarget, iTeamAttitudeHuman - iTeamAttitudeTarget, eJointWarTarget, iJointWarTargetScore, iJointWarRandomScore,
		iJointWarAtWarCounter, (eJointWarTarget != NO_TEAM && eJointWarTarget == eEmbargoTarget) ? 1 : 0, szJointWarDenial);
}

// <!-- custom: Preserve the realized origin of AI-driven vassalage/surrender before downstream deal/vassal actions erase whether this came from ordinary voluntary diplomacy, UWAI protector-seeking, or legacy/UWAI capitulation.
// Final package serialization is necessary for capitulation-to-human counterproposals; detailed rejected-master/capitulation utility reasoning remains in BBAI. (ChatGPT-5.6-Sol) -->
static char const* getSASGameRecordAIVassalageOrigin(SASGameRecordAIVassalageOrigin eOrigin)
{
	switch (eOrigin)
	{
	case SAS_AI_VASSALAGE_VOLUNTARY_CONTACT: return "VOLUNTARY_CONTACT";
	case SAS_AI_VASSALAGE_UWAI_FIND_MASTER: return "UWAI_FIND_MASTER";
	case SAS_AI_VASSALAGE_LEGACY_CAPITULATION: return "LEGACY_CAPITULATION";
	case SAS_AI_VASSALAGE_UWAI_CAPITULATION: return "UWAI_CAPITULATION";
	default: return "UNKNOWN";
	}
}

void logSASGameRecordAIVassalageDecision(CvPlayerAI const& kPlayer, PlayerTypes eTarget, SASGameRecordAIVassalageOrigin eOrigin, TeamTypes eEnemyTeam, int iContactProbMultX1000, int iTargetRank, int iAliveCivs, int iCounterProposal, CLinkList<TradeData> const& kAIGives, CLinkList<TradeData> const& kAIReceives)
{
	CvPlayerAI const& kTarget = GET_PLAYER(eTarget);
	int iEnemyAtWarCounter = -1;
	if (eEnemyTeam != NO_TEAM && GET_TEAM(kPlayer.getTeam()).isAtWar(eEnemyTeam))
		iEnemyAtWarCounter = GET_TEAM(kPlayer.getTeam()).AI_getAtWarCounter(eEnemyTeam);
	logSASGameRecord("GAME_RECORD_AI_VASSALAGE_DECISION turn=%d player=%d team=%d targetPlayer=%d targetTeam=%d targetHuman=%d origin=%s enemyTeam=%d enemyAtWarCounter=%d contactProbMultX1000=%d targetRank=%d aliveCivs=%d ourWars=%d counterProposal=%d aiGives=%s aiReceives=%s",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), eTarget, kTarget.getTeam(), kTarget.isHuman() ? 1 : 0,
		getSASGameRecordAIVassalageOrigin(eOrigin), eEnemyTeam, iEnemyAtWarCounter, iContactProbMultX1000, iTargetRank, iAliveCivs,
		GET_TEAM(kPlayer.getTeam()).getNumWars(), iCounterProposal, getSASTradeListText(kAIGives, kPlayer.getID()).GetCString(),
		getSASTradeListText(kAIReceives, eTarget).GetCString());
}

// <!-- custom: The colony transfer itself is already visible through player/vassal/city/unit actions, but those consequences cannot recover why this area was chosen or the pre-split aggregate AI_splitEmpireValue.
// Serialize only the committed broad decision and cheap pre-transfer scale context; detailed city-value components remain BBAI/source-analysis territory. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIColonySplitDecision(CvPlayerAI const& kPlayer, CvArea const& kArea, int iAreaValue, bool bForce)
{
	logSASGameRecord("GAME_RECORD_AI_COLONY_SPLIT_DECISION turn=%d player=%d team=%d origin=%s areaId=%d areaValue=%d areaCities=%d areaPopulation=%d playerCitiesBefore=%d playerPopulationBefore=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(),
		bForce ? "UWAI_FORCED_PRE_CAPITULATION" : "NORMAL_POSITIVE_VALUE",
		kArea.getID(), iAreaValue, kArea.getCitiesPerPlayer(kPlayer.getID()), kArea.getPopulationPerPlayer(kPlayer.getID()),
		kPlayer.getNumCities(), kPlayer.getTotalPopulation());
}

// <!-- custom: The factual SPACESHIP_LAUNCHED action preserves the post-launch countdown/components but cannot recover why the AI launched now.
// Reuse AI_launch's caller-supplied nearest-rival and live success context; no rival/project scan, value calculation or RNG is repeated in the serializer. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAISpaceshipLaunchDecision(CvPlayerAI const& kPlayer, VictoryTypes eVictory, int iLaunchSuccessPercent, TeamTypes eNearestRivalTeam, int iNearestRivalCountdown, bool bUrgentRival)
{
	if (eVictory == NO_VICTORY)
		return;
	char const* szReason = (iLaunchSuccessPercent >= 100 ? "FULL_SUCCESS" : "RIVAL_DEADLINE");
	// <!-- custom: A sub-100% launch is only reachable through the native urgent-rival override. (ChatGPT-5.6-Sol) -->
	FAssert(iLaunchSuccessPercent >= 0 && iLaunchSuccessPercent <= 100);
	FAssert(iLaunchSuccessPercent >= 100 || bUrgentRival);
	logSASGameRecord("GAME_RECORD_AI_SPACESHIP_LAUNCH_DECISION turn=%d player=%d team=%d victory=%s reason=%s launchSuccessPercent=%d nearestRivalTeam=%d nearestRivalCountdown=%d rivalAtOrBeforeOurArrival=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), getSASGameRecordVictoryType(eVictory), szReason,
		iLaunchSuccessPercent, eNearestRivalTeam, iNearestRivalCountdown, bUrgentRival ? 1 : 0);
}


// <!-- custom: MAPS <-> MAPS loses its pre-exchange valuation asymmetry as soon as the deal reveals both maps. Serialize only the already-live proposer/target values and threshold; the generic CONTACT_TRADE_MAP row remains authoritative for delivery/package. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIMapTradeDecision(CvPlayerAI const& kPlayer, PlayerTypes eTarget, int iOurReceiveValue, int iTargetReceiveValue, int iTargetReceiveMinExclusive)
{
	if (eTarget == NO_PLAYER)
		return;
	CvPlayerAI const& kTarget = GET_PLAYER(eTarget);
	logSASGameRecord("GAME_RECORD_AI_MAP_TRADE_DECISION turn=%d player=%d team=%d targetPlayer=%d targetTeam=%d targetHuman=%d ourReceiveMapValue=%d targetReceiveMapValue=%d ourValueAdvantage=%d targetReceiveMinExclusive=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), eTarget, kTarget.getTeam(), kTarget.isHuman() ? 1 : 0,
		iOurReceiveValue, iTargetReceiveValue, iOurReceiveValue - iTargetReceiveValue, iTargetReceiveMinExclusive);
}

// <!-- custom: The native vassal tribute chooser already evaluated every eligible resource before selecting the master's highest-value missing bonus.
// Serialize those retained scalars before delivery; the following DIPLO_DEAL remains authoritative for an AI-vassal transfer, while HUMAN_POPUP records a request that the human vassal may still decline. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIVassalResourceTributeDecision(CvPlayerAI const& kMaster, PlayerTypes eVassal, bool bHumanPopup, BonusTypes eSelectedBonus, int iSelectedValue, BonusTypes eRunnerUpBonus, int iRunnerUpValue, int iCandidateCount)
{
	if (eVassal == NO_PLAYER || eSelectedBonus == NO_BONUS)
		return;
	CvPlayerAI const& kVassal = GET_PLAYER(eVassal);
	int const iRunnerUpValueLogged = (eRunnerUpBonus == NO_BONUS ? -1 : iRunnerUpValue);
	logSASGameRecord("GAME_RECORD_AI_VASSAL_RESOURCE_TRIBUTE turn=%d master=%d masterTeam=%d vassal=%d vassalTeam=%d vassalHuman=%d vassalCapitulated=%d delivery=%s selectedBonus=%s selectedValue=%d runnerUpBonus=%s runnerUpValue=%d selectedAdvantage=%d candidateCount=%d",
		GC.getGame().getGameTurn(), kMaster.getID(), kMaster.getTeam(), eVassal, kVassal.getTeam(), kVassal.isHuman() ? 1 : 0,
		GET_TEAM(kVassal.getTeam()).isCapitulated() ? 1 : 0, bHumanPopup ? "HUMAN_POPUP" : "AI_DEAL", getSASGameRecordBonusType(eSelectedBonus), iSelectedValue,
		getSASGameRecordBonusType(eRunnerUpBonus), iRunnerUpValueLogged, eRunnerUpBonus == NO_BONUS ? -1 : iSelectedValue - iRunnerUpValue, iCandidateCount);
}

static char const* getSASGameRecordAIWarTradePaymentPath(SASGameRecordAIWarTradePaymentPath ePaymentPath)
{
	switch (ePaymentPath)
	{
	case SAS_AI_WAR_TRADE_TECH_GOLD: return "TECH_GOLD";
	case SAS_AI_WAR_TRADE_CITY_COUNTERPROPOSE: return "CITY_COUNTERPROPOSE";
	default: return "UNKNOWN";
	}
}

// <!-- custom: Preserve the live reason for a realized AI-to-AI joint-war hire immediately before the existing deal bridge.
// The generic CONTACT_JOIN_WAR row remains authoritative for the final package; this row reuses already-computed target/price/payment state and performs no extra target valuation, trade balancing or RNG. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIWarTradeIntent(CvPlayerAI const& kPlayer, PlayerTypes eHireling, SASGameRecordAIWarTradePaymentPath ePaymentPath, bool bUWAI, int iMinAtWarCounter, int iOfferDenominator, TeamTypes eTargetTeam, int iUWAITargetValue, int iLegacyTargetScore, int iHirePrice, TechTypes eCandidateTech1, TechTypes eCandidateTech2, int iWarSuccessRating, int iCityId, int iCityFitness, int iFinalWarSideValue, int iFinalPaymentSideValue)
{
	CvPlayerAI const& kHireling = GET_PLAYER(eHireling);
	logSASGameRecord("GAME_RECORD_AI_WAR_TRADE_INTENT turn=%d player=%d team=%d hireling=%d hirelingTeam=%d source=%s minAtWarCounter=%d offerDenominator=%d targetTeam=%d uwaiTargetValue=%d legacyTargetScore=%d hirePrice=%d paymentPath=%s candidateTech1=%s candidateTech2=%s warSuccessRating=%d cityId=%d cityFitness=%d finalWarSideValue=%d finalPaymentSideValue=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), eHireling, kHireling.getTeam(),
		bUWAI ? "UWAI" : "LEGACY", iMinAtWarCounter, iOfferDenominator, eTargetTeam,
		iUWAITargetValue, iLegacyTargetScore, iHirePrice, getSASGameRecordAIWarTradePaymentPath(ePaymentPath),
		getSASGameRecordTechType(eCandidateTech1), getSASGameRecordTechType(eCandidateTech2),
		iWarSuccessRating, iCityId, iCityFitness, iFinalWarSideValue, iFinalPaymentSideValue);
}

static char const* getSASGameRecordAICityTradeFormation(SASGameRecordAICityTradeFormation eFormation)
{
	switch (eFormation)
	{
	case SAS_AI_CITY_TRADE_FREE_LIBERATION: return "FREE_LIBERATION";
	case SAS_AI_CITY_TRADE_FREE_CITY_TO_HUMAN: return "FREE_CITY_TO_HUMAN";
	case SAS_AI_CITY_TRADE_OUR_SIDE_COUNTERPROPOSE: return "OUR_SIDE_COUNTERPROPOSE";
	case SAS_AI_CITY_TRADE_TARGET_SIDE_COUNTERPROPOSE: return "TARGET_SIDE_COUNTERPROPOSE";
	case SAS_AI_CITY_TRADE_SAME_TEAM_OVERRIDE: return "SAME_TEAM_OVERRIDE";
	default: return "UNKNOWN";
	}
}

// <!-- custom: Preserve the selected AI_proposeCityTrade boundary only after its live cede/counterproposal logic has formed a real human contact or immediate AI deal.
// Candidate rank/count and signed initial value gap reuse the function's existing sorted pair state; final trade lists are already formed. No city valuation, cede test, counterproposal search or RNG is repeated for logging. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAICityTradeIntent(CvPlayerAI const& kPlayer, PlayerTypes eTarget, SASGameRecordAICityTradeFormation eFormation, int iCandidateRank, int iCandidateCount, int iInitialValueGap, bool bInverseGapFallback, int iOurCityId, int iTheirCityId, bool bLiberation, bool bEvacuating, bool bSameTeam, bool bNegotiable, CLinkList<TradeData> const& kAIGives, CLinkList<TradeData> const& kAIReceives)
{
	CvPlayerAI const& kTarget = GET_PLAYER(eTarget);
	bool const bTargetHuman = kTarget.isHuman();
	logSASGameRecord("GAME_RECORD_AI_CITY_TRADE_INTENT turn=%d player=%d team=%d targetPlayer=%d targetTeam=%d targetHuman=%d delivery=%s attitudeValue=%d formation=%s candidateRank=%d candidates=%d initialValueGap=%d inverseGapFallback=%d ourCityId=%d theirCityId=%d liberation=%d evacuating=%d sameTeam=%d negotiable=%d aiGives=%s aiReceives=%s",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), eTarget, kTarget.getTeam(), bTargetHuman ? 1 : 0,
		bTargetHuman ? "HUMAN_CONTACT" : "AI_DEAL", kPlayer.AI_getAttitudeVal(eTarget), getSASGameRecordAICityTradeFormation(eFormation),
		iCandidateRank, iCandidateCount, iInitialValueGap, bInverseGapFallback ? 1 : 0, iOurCityId, iTheirCityId,
		bLiberation ? 1 : 0, bEvacuating ? 1 : 0, bSameTeam ? 1 : 0, bNegotiable ? 1 : 0,
		getSASTradeListText(kAIGives, kPlayer.getID()).GetCString(), getSASTradeListText(kAIReceives, eTarget).GetCString());
}


// <!-- custom: Record the resolved shared peace-negotiation boundary, including the compact end-war-value imbalance and reparations package that explains whether peace was blocked, deferred to a human offer, or implemented.
// `provisional*` is the reparations selected by AI_negotiatePeace before any human counterproposal; `aiGives`/`aiReceives` serialize the final lists when they exist.
// Exhaustive UWAI peace utility/reluctance/probability reasoning remains BBAI. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIPeaceDecision(PlayerTypes ePlayer, PlayerTypes eOther, int iAtWarTurns, bool bUWAI, int iInitialOurBenefit, int iInitialTheirBenefit, int iFinalOurBenefit, int iFinalTheirBenefit, int iGiveGold, int iReceiveGold, TechTypes eGiveTech, TechTypes eReceiveTech, int iGiveCityId, int iReceiveCityId, bool bCounterProposal, char const* szOutcome, CLinkList<TradeData> const* pWeGive, CLinkList<TradeData> const* pTheyGive)
{
	CvString const szAIGives = (pWeGive == NULL ? CvString("-") : getSASTradeListText(*pWeGive, ePlayer));
	CvString const szAIReceives = (pTheyGive == NULL ? CvString("-") : getSASTradeListText(*pTheyGive, eOther));
	logSASGameRecord("GAME_RECORD_AI_PEACE_DECISION turn=%d player=%d team=%d targetPlayer=%d targetTeam=%d humanTarget=%d source=%s atWarTurns=%d initialOurBenefit=%d initialTheirBenefit=%d finalOurBenefit=%d finalTheirBenefit=%d provisionalGiveGold=%d provisionalReceiveGold=%d provisionalGiveTech=%s provisionalReceiveTech=%s provisionalGiveCityId=%d provisionalReceiveCityId=%d counterProposal=%d outcome=%s aiGives=%s aiReceives=%s",
			GC.getGame().getGameTurn(), ePlayer, GET_PLAYER(ePlayer).getTeam(), eOther,
			GET_PLAYER(eOther).getTeam(), GET_PLAYER(eOther).isHuman() ? 1 : 0, bUWAI ? "UWAI" : "LEGACY", iAtWarTurns,
			iInitialOurBenefit, iInitialTheirBenefit, iFinalOurBenefit, iFinalTheirBenefit, iGiveGold, iReceiveGold,
			getSASGameRecordTechType(eGiveTech), getSASGameRecordTechType(eReceiveTech), iGiveCityId, iReceiveCityId,
			bCounterProposal ? 1 : 0, szOutcome, szAIGives.GetCString(), szAIReceives.GetCString());
}


// <!-- custom: Executives are UNITAI_MISSIONARY units whose XML carries one or more corporation-spread entries.
// Resolve the concrete corporation from the selected unit without invoking any AI value/search routine.
// Standard/SAS executives are one-corporation units; returning the first positive spread keeps the recorder factual and cheap if a modmod ever adds more. (ChatGPT-5.6-Sol) -->
static CorporationTypes getSASGameRecordExecutiveCorporation(UnitTypes eUnit)
{
	if (eUnit == NO_UNIT)
		return NO_CORPORATION;
	CvUnitInfo const& kUnit = GC.getInfo(eUnit);
	FOR_EACH_ENUM(Corporation)
	{
		if (kUnit.getCorporationSpreads(eLoopCorporation) > 0)
			return eLoopCorporation;
	}
	return NO_CORPORATION;
}

// <!-- custom: Log only after the normal production chooser has actually committed to the selected spread unit.
// Religious Missionaries share the same chooser and are intentionally ignored here; this row is corporation operational intent only. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIExecutiveProduction(CvCity const* pCity, UnitTypes eUnit, int iExecutiveValue, int iThreshold, char const* szStage)
{
	if (pCity == NULL || eUnit == NO_UNIT)
		return;
	CorporationTypes const eCorporation = getSASGameRecordExecutiveCorporation(eUnit);
	if (eCorporation == NO_CORPORATION)
		return;
	CvPlayerAI const& kPlayer = GET_PLAYER(pCity->getOwner()).AI();
	int const iProductionNeeded = kPlayer.getProductionNeeded(eUnit);
	logSASGameRecord("GAME_RECORD_AI_CORPORATION_DECISION turn=%d player=%d team=%d kind=EXECUTIVE_PRODUCTION cityId=%d city=%S x=%d y=%d corporation=%s unit=%s stage=%s executiveValue=%d threshold=%d valueMargin=%d productionNeeded=%d teamHasHQ=%d cityHasCorporation=%d ownerCorporationCities=%d",
		GC.getGame().getGameTurn(), pCity->getOwner(), pCity->getTeam(), pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(),
		pCity->getX(), pCity->getY(), getSASGameRecordCorporationType(eCorporation), getSASGameRecordUnitType(eUnit),
		szStage == NULL ? "-" : szStage, iExecutiveValue, iThreshold, iExecutiveValue - iThreshold, iProductionNeeded,
		GET_TEAM(kPlayer.getTeam()).hasHeadquarters(eCorporation) ? 1 : 0, pCity->isHasCorporation(eCorporation) ? 1 : 0,
		kPlayer.getHasCorporationCount(eCorporation));
}

// <!-- custom: Preserve one new/retargeted Executive spread destination using only target/value/path components already produced by AI_spreadCorporation.
// The realized mission attempt remains authoritative in CORPORATION_SPREAD_ATTEMPT; this row explains the operational choice that led the Executive there. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAICorporationTarget(CvUnit const* pUnit, CorporationTypes eCorporation, PlayerTypes ePreferredPlayer, int iExecutiveSpreadValue, CvCity const* pTargetCity, int iHqBaseValue, int iLocalCorporationValue, int iCompetingCorporationAdjustment, int iPopulationBonus, int iPreferredPlayerMultiplier, int iPathTurns, int iTargetScore, char const* szAction)
{
	if (pUnit == NULL || eCorporation == NO_CORPORATION || pTargetCity == NULL)
		return;
	CvPlayerAI const& kPlayer = GET_PLAYER(pUnit->getOwner()).AI();
	int const iSpreadCost = pUnit->spreadCorporationCost(eCorporation, pTargetCity);
	int const iGold = kPlayer.getGold();
	logSASGameRecord("GAME_RECORD_AI_CORPORATION_DECISION turn=%d player=%d team=%d kind=SPREAD_TARGET unitId=%d unit=%s unitAI=%s groupId=%d corporation=%s teamHasHQ=%d preferredPlayer=%d targetIsPreferredPlayer=%d executiveSpreadValue=%d targetPlayer=%d targetTeam=%d cityId=%d city=%S x=%d y=%d hqBaseValue=%d localCorporationValue=%d competingCorporationAdjustment=%d populationBonus=%d preferredPlayerMultiplier=%d pathTurns=%d targetScore=%d gold=%d spreadCost=%d canAfford=%d atTarget=%d action=%s",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getTeam(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()),
		getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()), pUnit->getGroupID(), getSASGameRecordCorporationType(eCorporation),
		GET_TEAM(kPlayer.getTeam()).hasHeadquarters(eCorporation) ? 1 : 0, ePreferredPlayer,
		pTargetCity->getOwner() == ePreferredPlayer ? 1 : 0, iExecutiveSpreadValue, pTargetCity->getOwner(), pTargetCity->getTeam(),
		pTargetCity->getID(), getSASGameRecordQuotedCityName(pTargetCity).GetCString(), pTargetCity->getX(), pTargetCity->getY(),
		iHqBaseValue, iLocalCorporationValue, iCompetingCorporationAdjustment, iPopulationBonus, iPreferredPlayerMultiplier, iPathTurns,
		iTargetScore, iGold, iSpreadCost, iGold >= iSpreadCost ? 1 : 0, pUnit->at(pTargetCity->getPlot()) ? 1 : 0,
		szAction == NULL ? "-" : szAction);
}

// <!-- custom: Preserve the compact selected Missionary destination boundary without duplicating AI_spreadReligion's city/player scoring internals.
// Ordinary land rows are emitted only for a new/retargeted MISSIONAI_SPREAD destination; airlift rows preserve the realized redirect. The eventual RELIGION_SPREAD_ATTEMPT remains authoritative for the actual spread outcome. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIReligionSpreadTarget(CvUnit const* pUnit, ReligionTypes eReligion, CvCity const* pTargetCity, int iPlayerMultiplierPercent, int iPathTurns, int iTargetScore, char const* szAction)
{
	if (pUnit == NULL || eReligion == NO_RELIGION || pTargetCity == NULL)
		return;
	CvPlayer const& kOwner = GET_PLAYER(pUnit->getOwner());
	char const* szTargetScope = (pTargetCity->getOwner() == pUnit->getOwner() ? "SELF" :
			(pTargetCity->getTeam() == pUnit->getTeam() ? "TEAM" : "FOREIGN"));
	logSASGameRecord("GAME_RECORD_AI_RELIGION_SPREAD_TARGET turn=%d player=%d team=%d ownerHuman=%d unitId=%d unit=%s unitAI=%s groupId=%d religion=%s teamHasHolyCity=%d targetScope=%s targetPlayer=%d targetTeam=%d cityId=%d city=%S x=%d y=%d playerMultiplierPercent=%d pathTurns=%d targetScore=%d atTarget=%d action=%s",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getTeam(), kOwner.isHuman() ? 1 : 0, pUnit->getID(),
		getSASGameRecordUnitType(pUnit->getUnitType()), getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()), pUnit->getGroupID(),
		getSASGameRecordReligionType(eReligion), GET_TEAM(pUnit->getTeam()).hasHolyCity(eReligion) ? 1 : 0, szTargetScope,
		pTargetCity->getOwner(), pTargetCity->getTeam(), pTargetCity->getID(), getSASGameRecordQuotedCityName(pTargetCity).GetCString(),
		pTargetCity->getX(), pTargetCity->getY(), iPlayerMultiplierPercent, iPathTurns, iTargetScore,
		pUnit->at(pTargetCity->getPlot()) ? 1 : 0, szAction == NULL ? "-" : szAction);
}


// <!-- custom: Airlift and sea-transport routing can materially redirect an Executive before its ordinary land spread target is chosen.
// Record only the already-selected destination/score/path state; no extra corporation valuation, city search or pathfinding is performed for this transit row. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAICorporationTransit(CvUnit const* pExecutive, CvUnit const* pTransport, CorporationTypes eCorporation, int iEligibleCorporations, CvCity const* pTargetCity, CvPlot const* pMovePlot, int iPathTurns, int iTargetScore, char const* szRoute)
{
	if (pExecutive == NULL || eCorporation == NO_CORPORATION || pTargetCity == NULL)
		return;
	CvPlot const* pFromPlot = pExecutive->plot();
	CvPlayerAI const& kPlayer = GET_PLAYER(pExecutive->getOwner()).AI();
	logSASGameRecord("GAME_RECORD_AI_CORPORATION_DECISION turn=%d player=%d team=%d kind=TRANSIT_TARGET route=%s executiveId=%d executive=%s unitAI=%s transportId=%d transport=%s corporation=%s eligibleCorporations=%d teamHasHQ=%d fromX=%d fromY=%d moveX=%d moveY=%d targetPlayer=%d targetTeam=%d cityId=%d city=%S x=%d y=%d pathTurns=%d targetScore=%d",
		GC.getGame().getGameTurn(), pExecutive->getOwner(), pExecutive->getTeam(), szRoute == NULL ? "-" : szRoute, pExecutive->getID(),
		getSASGameRecordUnitType(pExecutive->getUnitType()), getSASGameRecordUnitAIType(pExecutive->AI_getUnitAIType()),
		pTransport == NULL ? -1 : pTransport->getID(), pTransport == NULL ? "-" : getSASGameRecordUnitType(pTransport->getUnitType()),
		getSASGameRecordCorporationType(eCorporation), iEligibleCorporations,
		GET_TEAM(kPlayer.getTeam()).hasHeadquarters(eCorporation) ? 1 : 0, pFromPlot == NULL ? -1 : pFromPlot->getX(),
		pFromPlot == NULL ? -1 : pFromPlot->getY(), pMovePlot == NULL ? -1 : pMovePlot->getX(), pMovePlot == NULL ? -1 : pMovePlot->getY(),
		pTargetCity->getOwner(), pTargetCity->getTeam(), pTargetCity->getID(), getSASGameRecordQuotedCityName(pTargetCity).GetCString(),
		pTargetCity->getX(), pTargetCity->getY(), iPathTurns, iTargetScore);
}


// <!-- custom: Serialize level-3 AI_bestTech alternatives as candidateTech:immediateValue/pathValue>aimTech.
// The list is already ordered by the live chooser's distinct returned-tech preference; no valuation is repeated here. (ChatGPT-5.6-Sol) -->
static CvString getSASGameRecordTechCandidatePaths(SASTechChoiceContext const* pChoice)
{
	if (pChoice == NULL || !pChoice->bCollectCandidates || pChoice->aCandidates.empty())
		return CvString("-");
	CvString szResult;
	for (size_t i = 0; i < pChoice->aCandidates.size(); i++)
	{
		SASTechChoiceCandidate const& kCandidate = pChoice->aCandidates[i];
		CvString szItem;
		szItem.Format(i == 0 ? "%s:%d/%d>%s" : ",%s:%d/%d>%s", getSASGameRecordTechType(kCandidate.eTech), kCandidate.iImmediateValue, kCandidate.iPathValue, getSASGameRecordTechType(kCandidate.eAimTech));
		szResult += szItem;
	}
	return szResult;
}

// <!-- custom: Record one actual AI research/free-tech commitment.
// Team coordination and Python overrides are identified honestly without fabricated scores; AI_bestTech contributes only the path data its real chooser already computed. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIResearchDecision(CvPlayerAI const& kPlayer, char const* szKind, char const* szSource, TechTypes eRequestedTech, int iResearchDepth, PlayerTypes eCoordinatingPlayer, SASTechChoiceContext const* pChoice)
{
	TechTypes const eCurrentResearch = (strcmp(szKind, "RESEARCH") == 0 ? kPlayer.getCurrentResearch() : NO_TECH);
	TechTypes const eAimTech = (pChoice == NULL ? NO_TECH : pChoice->eAimTech);
	TechTypes const eRunnerUpTech = (pChoice == NULL ? NO_TECH : pChoice->eRunnerUpTech);
	TechTypes const eRunnerUpAimTech = (pChoice == NULL ? NO_TECH : pChoice->eRunnerUpAimTech);
	int const iBestImmediateValue = (pChoice == NULL ? -1 : pChoice->iBestImmediateValue);
	int const iBestPathValue = (pChoice == NULL ? -1 : pChoice->iBestPathValue);
	int const iRunnerUpImmediateValue = (pChoice == NULL ? -1 : pChoice->iRunnerUpImmediateValue);
	int const iRunnerUpPathValue = (pChoice == NULL ? -1 : pChoice->iRunnerUpPathValue);
	CvString const szCandidates = getSASGameRecordTechCandidatePaths(pChoice);
	logSASGameRecord("GAME_RECORD_AI_RESEARCH_DECISION turn=%d player=%d team=%d kind=%s source=%s requestedTech=%s currentResearchAfter=%s researchDepth=%d coordinatingPlayer=%d aimTech=%s immediateValue=%d pathValue=%d runnerUpTech=%s runnerUpAimTech=%s runnerUpImmediateValue=%d runnerUpPathValue=%d pathValueMargin=%d selectedPathIndex=%d depth0Evaluated=%d evaluatedTechs=%d evaluatedPaths=%d queueLength=%d candidatePaths=%s",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), szKind, szSource, getSASGameRecordTechType(eRequestedTech),
		getSASGameRecordTechType(eCurrentResearch), iResearchDepth, eCoordinatingPlayer, getSASGameRecordTechType(eAimTech),
		iBestImmediateValue, iBestPathValue, getSASGameRecordTechType(eRunnerUpTech), getSASGameRecordTechType(eRunnerUpAimTech),
		iRunnerUpImmediateValue, iRunnerUpPathValue, (eRunnerUpTech == NO_TECH ? -1 : iBestPathValue - iRunnerUpPathValue),
		pChoice == NULL ? -1 : pChoice->iSelectedPathIndex, pChoice == NULL ? -1 : pChoice->iImmediateCandidateCount,
		pChoice == NULL ? -1 : pChoice->iEvaluatedTechCount, pChoice == NULL ? -1 : pChoice->iPathCount, kPlayer.getLengthResearchQueue(),
		szCandidates.GetCString());
}

// <!-- custom: Resource-choice winner facts are deliberately cheap/factual and gathered only when an actual resource proposal/demand row is emitted.
// They complement, rather than rerun or decompose, AI_bonusTradeVal. (ChatGPT-5.6-Sol) -->
// <!-- custom: Preserve the live AI_doCivics hysteresis decision without repeating civic valuation.
// The recorder gathers broader strategic context only after the caller has crossed the level gate, keeping ordinary gameplay free of these diagnostic lookups. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAICivicCandidate(PlayerTypes ePlayer, char const* szStatus, CivicOptionTypes eCivicOption, CivicTypes eOldCivic, CivicTypes eNewCivic, int iCurrentValue, int iBestValue, int iTestAnarchy, int iCurrentBundleAnarchy, int iThreshold, int iSlack, bool bPassPercent, bool bPassSlack, bool bFirstPass)
{
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer).AI();
	CvTeamAI const& kTeam = GET_TEAM(kPlayer.getTeam());
	int const iAnarchyDelta = std::max(0, iTestAnarchy - iCurrentBundleAnarchy);
	logSASGameRecord("GAME_RECORD_AI_CIVIC_DECISION turn=%d player=%d team=%d status=%s option=%s oldCivic=%s newCivic=%s currentValue=%d bestValue=%d delta=%d anarchyTest=%d currentBundleAnarchy=%d anarchyDelta=%d threshold=%d slack=%d passPercent=%d passSlack=%d firstPass=%d civicTimer=%d goldenAgeTurns=%d maxAnarchyTurns=%d favoriteCivic=%s newIsFavorite=%d financialTrouble=%d wars=%d anyWarPlan=%d gold=%d goldRate=%d",
		GC.getGame().getGameTurn(), ePlayer, kPlayer.getTeam(), szStatus, GC.getInfo(eCivicOption).getType(),
		getSASGameRecordCivicType(eOldCivic), getSASGameRecordCivicType(eNewCivic), iCurrentValue, iBestValue, iBestValue - iCurrentValue,
		iTestAnarchy, iCurrentBundleAnarchy, iAnarchyDelta, iThreshold, iSlack, bPassPercent ? 1 : 0, bPassSlack ? 1 : 0, bFirstPass ? 1 : 0,
		kPlayer.AI_getCivicTimer(), kPlayer.getGoldenAgeTurns(), kPlayer.getMaxAnarchyTurns(),
		getSASGameRecordCivicType(kPlayer.getFavoriteCivic()), eNewCivic == kPlayer.getFavoriteCivic() ? 1 : 0,
		kPlayer.AI_isFinancialTrouble() ? 1 : 0, kTeam.getNumWars(), kTeam.AI_isAnyWarPlan() ? 1 : 0, kPlayer.getGold(),
		kPlayer.calculateGoldRate());
}

static CvString getSASGameRecordCivicChanges(std::vector<std::pair<CivicTypes, CivicTypes> > const& aeChanges)
{
	if (aeChanges.empty())
		return CvString("-");
	CvString szChanges;
	for (size_t i = 0; i < aeChanges.size(); i++)
	{
		CivicTypes const eOldCivic = aeChanges[i].first;
		CivicTypes const eNewCivic = aeChanges[i].second;
		CivicTypes const eOptionCivic = (eNewCivic != NO_CIVIC ? eNewCivic : eOldCivic);
		CvString szItem;
		if (eOptionCivic == NO_CIVIC)
			szItem = "UNKNOWN:-";
		else
			szItem.Format("%s:%s>%s", GC.getInfo(GC.getInfo(eOptionCivic).getCivicOptionType()).getType(), getSASGameRecordCivicType(eOldCivic), getSASGameRecordCivicType(eNewCivic));
		if (!szChanges.empty())
			szChanges += ",";
		szChanges += szItem;
	}
	return szChanges;
}

// <!-- custom: Final civic outcome records only a meaningful accepted bundle: an actual revolution or a concrete reason that the accepted bundle was postponed/blocked.
// Pending/final civic pairs come from the already-mutated local AI_doCivics bundle; no civic valuation is repeated here. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAICivicOutcome(PlayerTypes ePlayer, char const* szOutcome, std::vector<std::pair<CivicTypes, CivicTypes> > const& aeChanges, int iAnarchyLength, int iCivicTimerAfter, TechTypes eResearch, int iResearchTurns, CivicTypes eWaitCivic, int iWaitValue, int iWaitCurrentValue, int iGoldNeeded, int iCanRevolution)
{
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer).AI();
	CvTeamAI const& kTeam = GET_TEAM(kPlayer.getTeam());
	CvString const szChanges = getSASGameRecordCivicChanges(aeChanges);
	// <!-- custom: getGoldPerTurn is net diplomatic deal GPT; AI_doCivics uses it directly for the paid-anarchy cash-reserve gate, while calculateGoldRate is broader economy context. (ChatGPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_AI_CIVIC_OUTCOME turn=%d player=%d team=%d outcome=%s changes=%s changeCount=%d plannedAnarchy=%d civicTimerBefore=%d civicTimerAfter=%d research=%s researchTurns=%d waitCivic=%s waitValue=%d waitCurrentValue=%d gold=%d goldRate=%d dealGoldPerTurn=%d goldNeeded=%d strikeTurns=%d canRevolution=%d goldenAgeTurns=%d maxAnarchyTurns=%d financialTrouble=%d wars=%d anyWarPlan=%d",
		GC.getGame().getGameTurn(), ePlayer, kPlayer.getTeam(), szOutcome, szChanges.GetCString(), (int)aeChanges.size(), iAnarchyLength,
		kPlayer.AI_getCivicTimer(), iCivicTimerAfter, getSASGameRecordTechType(eResearch), iResearchTurns,
		getSASGameRecordCivicType(eWaitCivic), iWaitValue, iWaitCurrentValue, kPlayer.getGold(), kPlayer.calculateGoldRate(),
		kPlayer.getGoldPerTurn(), iGoldNeeded, kPlayer.getStrikeTurns(), iCanRevolution, kPlayer.getGoldenAgeTurns(),
		kPlayer.getMaxAnarchyTurns(), kPlayer.AI_isFinancialTrouble() ? 1 : 0, kTeam.getNumWars(), kTeam.AI_isAnyWarPlan() ? 1 : 0);
}

struct SASGameRecordBonusChoiceFacts
{
	int iBuyerEra;
	int iBuyerAvailableBefore;
	int iSellerTradeableCopies;
	int iBonusHealth;
	int iBonusHappiness;
	int iBonusAIObjective;
	int iBuyerHasMetal;
	int iBuyerHasMount;
	int iBuyerHasElephants;
	int iMasterVassalCluster;
};

static void getSASGameRecordBonusChoiceFacts(CvPlayerAI const& kBuyer, CvPlayerAI const& kSeller, BonusTypes eBonus, SASGameRecordBonusChoiceFacts& kFacts)
{
	kFacts.iBuyerEra = kBuyer.getCurrentEra();
	kFacts.iBuyerAvailableBefore = (eBonus == NO_BONUS ? -1 : kBuyer.getNumAvailableBonuses(eBonus));
	kFacts.iSellerTradeableCopies = (eBonus == NO_BONUS ? -1 : kSeller.getNumTradeableBonuses(eBonus));
	kFacts.iBonusHealth = (eBonus == NO_BONUS ? 0 : GC.getInfo(eBonus).getHealth());
	kFacts.iBonusHappiness = (eBonus == NO_BONUS ? 0 : GC.getInfo(eBonus).getHappiness());
	kFacts.iBonusAIObjective = (eBonus == NO_BONUS ? 0 : GC.getInfo(eBonus).getAIObjective());
	static const BonusTypes eCopper = (BonusTypes)GC.getInfoTypeForString(GC.getDefineSTRING("SAS_KEY_STRATEGIC_METAL_BONUS_NAME_1"), true);
	static const BonusTypes eIron = (BonusTypes)GC.getInfoTypeForString(GC.getDefineSTRING("SAS_KEY_STRATEGIC_METAL_BONUS_NAME_2"), true);
	static const BonusTypes eHorse = (BonusTypes)GC.getInfoTypeForString(GC.getDefineSTRING("SAS_MOUNTED_UNITS_BONUS_NAME_1"), true);
	static const BonusTypes eCamel = (BonusTypes)GC.getInfoTypeForString(GC.getDefineSTRING("SAS_MOUNTED_UNITS_BONUS_NAME_2"), true);
	static const BonusTypes eElephants = (BonusTypes)GC.getInfoTypeForString(GC.getDefineSTRING("SAS_MOUNTED_UNITS_BONUS_NAME_3"), true);
	kFacts.iBuyerHasMetal = ((eCopper != NO_BONUS && kBuyer.getNumAvailableBonuses(eCopper) > 0) || (eIron != NO_BONUS && kBuyer.getNumAvailableBonuses(eIron) > 0) ? 1 : 0);
	kFacts.iBuyerHasMount = ((eHorse != NO_BONUS && kBuyer.getNumAvailableBonuses(eHorse) > 0) || (eCamel != NO_BONUS && kBuyer.getNumAvailableBonuses(eCamel) > 0) ? 1 : 0);
	kFacts.iBuyerHasElephants = (eElephants != NO_BONUS && kBuyer.getNumAvailableBonuses(eElephants) > 0 ? 1 : 0);
	CvTeamAI const& kBuyerTeam = GET_TEAM(kBuyer.getTeam());
	TeamTypes const eAnchor = (kBuyerTeam.isAVassal() ? kBuyerTeam.getMasterTeam() : kBuyer.getTeam());
	TeamTypes const eSellerTeam = kSeller.getTeam();
	kFacts.iMasterVassalCluster = (eSellerTeam == eAnchor || GET_TEAM(eSellerTeam).isVassal(eAnchor) ? 1 : 0);
}

// <!-- custom: Preserve the real proactive resource chooser's winning/runner-up receive and give candidates, including its existing gate/random score, then join those candidates to the final counterproposal terms.
// Logging occurs before an AI-AI implementDeal can mutate holdings. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIBonusTradeDecision(PlayerTypes ePlayer, PlayerTypes eOther, char const* szAnchor, SASGameRecordBonusTradeSide const& kReceive, SASGameRecordBonusTradeSide const& kGive, CLinkList<TradeData> const& kWeGive, CLinkList<TradeData> const& kTheyGive)
{
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer).AI();
	CvPlayerAI const& kOther = GET_PLAYER(eOther).AI();
	SASGameRecordBonusChoiceFacts kReceiveFacts;
	SASGameRecordBonusChoiceFacts kGiveFacts;
	getSASGameRecordBonusChoiceFacts(kPlayer, kOther, kReceive.eBestBonus, kReceiveFacts);
	getSASGameRecordBonusChoiceFacts(kOther, kPlayer, kGive.eBestBonus, kGiveFacts);
	CvString const szWeGive = getSASTradeListText(kWeGive, ePlayer);
	CvString const szTheyGive = getSASTradeListText(kTheyGive, eOther);
	logSASGameRecord("GAME_RECORD_AI_BONUS_TRADE_DECISION turn=%d player=%d team=%d other=%d otherTeam=%d otherHuman=%d anchor=%s outcome=%s receiveBonus=%s receiveBuyerTradeValue=%d receiveSellerKeepValue=%d receiveBias=%d receiveGatePermille=%d receiveRandom=%d receiveScore=%d receiveRunnerUpBonus=%s receiveRunnerUpBuyerTradeValue=%d receiveRunnerUpSellerKeepValue=%d receiveRunnerUpBias=%d receiveRunnerUpGatePermille=%d receiveRunnerUpRandom=%d receiveRunnerUpScore=%d receiveEvaluated=%d receiveGatePassed=%d receiveNoDenial=%d receiveBuyerEra=%s receiveBuyerAvailableBefore=%d receiveSellerTradeableCopies=%d receiveBonusHealth=%d receiveBonusHappiness=%d receiveBonusAIObjective=%d receiveBuyerHasMetal=%d receiveBuyerHasMount=%d receiveBuyerHasElephants=%d receiveMasterVassalCluster=%d giveBonus=%s giveBuyerTradeValue=%d giveSellerKeepValue=%d giveBias=%d giveGatePermille=%d giveRandom=%d giveScore=%d giveRunnerUpBonus=%s giveRunnerUpBuyerTradeValue=%d giveRunnerUpSellerKeepValue=%d giveRunnerUpBias=%d giveRunnerUpGatePermille=%d giveRunnerUpRandom=%d giveRunnerUpScore=%d giveEvaluated=%d giveGatePassed=%d giveNoDenial=%d giveBuyerEra=%s giveBuyerAvailableBefore=%d giveSellerTradeableCopies=%d giveBonusHealth=%d giveBonusHappiness=%d giveBonusAIObjective=%d giveBuyerHasMetal=%d giveBuyerHasMount=%d giveBuyerHasElephants=%d giveMasterVassalCluster=%d weGive=%s theyGive=%s",
		GC.getGame().getGameTurn(), ePlayer, kPlayer.getTeam(), eOther, kOther.getTeam(), kOther.isHuman() ? 1 : 0, szAnchor,
		kOther.isHuman() ? "CONTACT_HUMAN" : "IMPLEMENT_AI_DEAL", getSASGameRecordBonusType(kReceive.eBestBonus),
		kReceive.iBestBuyerTradeValue, kReceive.iBestSellerKeepValue, kReceive.iBestBias, kReceive.iBestGatePermille, kReceive.iBestRandom,
		kReceive.iBestScore, getSASGameRecordBonusType(kReceive.eRunnerUpBonus), kReceive.iRunnerUpBuyerTradeValue,
		kReceive.iRunnerUpSellerKeepValue, kReceive.iRunnerUpBias, kReceive.iRunnerUpGatePermille, kReceive.iRunnerUpRandom,
		kReceive.iRunnerUpScore, kReceive.iEvaluated, kReceive.iGatePassed, kReceive.iNoDenial,
		getSASGameRecordEraType((EraTypes)kReceiveFacts.iBuyerEra), kReceiveFacts.iBuyerAvailableBefore,
		kReceiveFacts.iSellerTradeableCopies, kReceiveFacts.iBonusHealth, kReceiveFacts.iBonusHappiness, kReceiveFacts.iBonusAIObjective,
		kReceiveFacts.iBuyerHasMetal, kReceiveFacts.iBuyerHasMount, kReceiveFacts.iBuyerHasElephants, kReceiveFacts.iMasterVassalCluster,
		getSASGameRecordBonusType(kGive.eBestBonus), kGive.iBestBuyerTradeValue, kGive.iBestSellerKeepValue, kGive.iBestBias,
		kGive.iBestGatePermille, kGive.iBestRandom, kGive.iBestScore, getSASGameRecordBonusType(kGive.eRunnerUpBonus),
		kGive.iRunnerUpBuyerTradeValue, kGive.iRunnerUpSellerKeepValue, kGive.iRunnerUpBias, kGive.iRunnerUpGatePermille,
		kGive.iRunnerUpRandom, kGive.iRunnerUpScore, kGive.iEvaluated, kGive.iGatePassed, kGive.iNoDenial,
		getSASGameRecordEraType((EraTypes)kGiveFacts.iBuyerEra), kGiveFacts.iBuyerAvailableBefore, kGiveFacts.iSellerTradeableCopies,
		kGiveFacts.iBonusHealth, kGiveFacts.iBonusHappiness, kGiveFacts.iBonusAIObjective, kGiveFacts.iBuyerHasMetal,
		kGiveFacts.iBuyerHasMount, kGiveFacts.iBuyerHasElephants, kGiveFacts.iMasterVassalCluster, szWeGive.GetCString(),
		szTheyGive.GetCString());
}

// <!-- custom: Bonus tribute uses a different chooser from proactive resource trading.
// Preserve the already-sorted winner/runner-up, the 0.6 non-surplus sort context, selected set/threshold and final ordinary deal valuation without duplicating AI_bonusTradeVal or its random factor. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIBonusDemandDecision(PlayerTypes ePlayer, PlayerTypes eHuman, SASGameRecordBonusDemandContext const& kContext, CLinkList<TradeData> const& kHumanGives)
{
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer).AI();
	CvPlayerAI const& kHuman = GET_PLAYER(eHuman).AI();
	SASGameRecordBonusChoiceFacts kBestFacts;
	getSASGameRecordBonusChoiceFacts(kPlayer, kHuman, kContext.eBestBonus, kBestFacts);
	CvString const szDemanded = getSASTradeListText(kHumanGives, eHuman);
	logSASGameRecord("GAME_RECORD_AI_BONUS_DEMAND_DECISION turn=%d player=%d team=%d human=%d humanTeam=%d bestBonus=%s bestSortValueX100=%d bestHumanTradeableCopies=%d bestNonSurplusSort=%d runnerUpBonus=%s runnerUpSortValueX100=%d runnerUpHumanTradeableCopies=%d runnerUpNonSurplusSort=%d candidates=%d maxSelectable=%d selected=%d selectedTotalValueX100=%d minValueX100=%d finalDealValue=%d buyerEra=%s buyerAvailableBefore=%d sellerTradeableCopies=%d bonusHealth=%d bonusHappiness=%d bonusAIObjective=%d buyerHasMetal=%d buyerHasMount=%d buyerHasElephants=%d masterVassalCluster=%d demanded=%s",
		GC.getGame().getGameTurn(), ePlayer, kPlayer.getTeam(), eHuman, kHuman.getTeam(), getSASGameRecordBonusType(kContext.eBestBonus),
		kContext.iBestSortValueX100, kContext.iBestHumanTradeableCopies, kContext.iBestNonSurplusSort,
		getSASGameRecordBonusType(kContext.eRunnerUpBonus), kContext.iRunnerUpSortValueX100, kContext.iRunnerUpHumanTradeableCopies,
		kContext.iRunnerUpNonSurplusSort, kContext.iCandidateCount, kHuman.getCurrentEra() + 2, kContext.iSelectedCount,
		kContext.iSelectedTotalValueX100, kContext.iMinValueX100, kContext.iDealValue,
		getSASGameRecordEraType((EraTypes)kBestFacts.iBuyerEra), kBestFacts.iBuyerAvailableBefore, kBestFacts.iSellerTradeableCopies,
		kBestFacts.iBonusHealth, kBestFacts.iBonusHappiness, kBestFacts.iBonusAIObjective, kBestFacts.iBuyerHasMetal,
		kBestFacts.iBuyerHasMount, kBestFacts.iBuyerHasElephants, kBestFacts.iMasterVassalCluster, szDemanded.GetCString());
}

// <!-- custom: Record the final authoritative non-default strategy-bit changes after AI_updateStrategyHash has completed all local strategy decisions and final validity cleanup.
// The old/new hashes are already-computed gameplay state; this bridge performs no strategy evaluation, RNG or pathfinding. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIStrategyChanges(CvPlayerAI const& kPlayer, AIStrategy eOldStrategies, AIStrategy eNewStrategies)
{
	if (!isSASGameRecordAIStrategyPlayer(kPlayer))
		return;
	for (int iStrategy = AI_STRATEGY_DAGGER; iStrategy <= AI_STRATEGY_ESPIONAGE_ECONOMY; iStrategy <<= 1)
	{
		AIStrategy const eStrategy = (AIStrategy)iStrategy;
		bool const bWasActive = ((eOldStrategies & eStrategy) != 0);
		bool const bIsActive = ((eNewStrategies & eStrategy) != 0);
		if (bWasActive == bIsActive)
			continue;
		logSASGameRecord("GAME_RECORD_AI_STRATEGY_CHANGE turn=%d player=%d team=%d strategy=%s activeAfter=%d",
			GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), getSASAIStrategyType(eStrategy), bIsActive);
	}
}

static void logSASGameRecordAIVictoryStageChange(CvPlayerAI const& kPlayer, char const* szRoute, int iOldStage, int iNewStage)
{
	if (iOldStage == iNewStage)
		return;
	logSASGameRecord("GAME_RECORD_AI_VICTORY_STAGE_CHANGE turn=%d player=%d team=%d route=%s oldStage=%d newStage=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), szRoute, iOldStage, iNewStage);
}

// <!-- custom: Record route-level 0..4 transitions only after AI_updateVictoryStageHash reaches its final state, including early resets for invalid/capitulated/no-capital players.
// Comparing its already-computed old/new bitfields adds no victory-stage evaluation, RNG or pathfinding; periodic GAME_RECORD_AI_VICTORY_STAGES remains the checkpoint for loaded/truncated records. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIVictoryStageChanges(CvPlayerAI const& kPlayer, AIVictoryStage eOldStages, AIVictoryStage eNewStages)
{
	if (!isSASGameRecordAIVictoryStagePlayer(kPlayer) || eOldStages == eNewStages)
		return;
	logSASGameRecordAIVictoryStageChange(kPlayer, "CULTURE", getSASCultureVictoryStageLevel(eOldStages), getSASCultureVictoryStageLevel(eNewStages));
	logSASGameRecordAIVictoryStageChange(kPlayer, "SPACE", getSASSpaceVictoryStageLevel(eOldStages), getSASSpaceVictoryStageLevel(eNewStages));
	logSASGameRecordAIVictoryStageChange(kPlayer, "CONQUEST", getSASConquestVictoryStageLevel(eOldStages), getSASConquestVictoryStageLevel(eNewStages));
	logSASGameRecordAIVictoryStageChange(kPlayer, "DOMINATION", getSASDominationVictoryStageLevel(eOldStages), getSASDominationVictoryStageLevel(eNewStages));
	logSASGameRecordAIVictoryStageChange(kPlayer, "DIPLOMACY", getSASDiplomacyVictoryStageLevel(eOldStages), getSASDiplomacyVictoryStageLevel(eNewStages));
}

// <!-- custom: AI_updateWorstEnemy can reevaluate once after decaying old enemy-trade memories; its recursive correction commits the final state, while the outer pass returns before assignment.
// Log that one authoritative replacement together with the selection pass's already-computed enmity values and whether this was the unique recursive corrective pass; periodic GAME_RECORD_DIPLO_STATUS worstEnemyTeam remains the loaded-save/truncated-log checkpoint. (ChatGPT-5.6-Sol) -->
void logSASGameRecordWorstEnemyChanged(CvTeamAI const& kTeam, TeamTypes eOldEnemy, TeamTypes eNewEnemy, int iOldEnmity, int iNewEnmity, bool bRecursiveRecheck)
{
	if (eOldEnemy == eNewEnemy)
		return;
	logSASGameRecord("GAME_RECORD_WORST_ENEMY_CHANGE turn=%d team=%d oldEnemyTeam=%d newEnemyTeam=%d oldEnmity=%d newEnmity=%d recursiveRecheck=%d",
		GC.getGame().getGameTurn(), kTeam.getID(), eOldEnemy, eNewEnemy, iOldEnmity, iNewEnmity, bRecursiveRecheck ? 1 : 0);
}

// <!-- custom: AreaAI has only two authoritative writers: CvTeamAI's ordinary calculation and UWAI's later alignAreaAI override.
// Record both changes with source provenance; ordering makes a same-turn UWAI row explicitly supersede the calculated row, while periodic GAME_RECORD_AREA_AI preserves the final checkpoint.
// Initial NO_AREAAI assignment is setup rather than a gameplay transition and is intentionally left to the checkpoint. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAreaAIChanged(CvTeamAI const& kTeam, CvArea const& kArea, AreaAITypes eOldType, AreaAITypes eNewType, char const* szSource)
{
	if (eOldType == NO_AREAAI || eOldType == eNewType)
		return;
	logSASGameRecord("GAME_RECORD_AREA_AI_CHANGE turn=%d team=%d area=%d source=%s oldType=%s newType=%s teamCities=%d totalCities=%d wars=%d anyWarPlan=%d",
		GC.getGame().getGameTurn(), kTeam.getID(), kArea.getID(), szSource, getSASAreaAIType(eOldType), getSASAreaAIType(eNewType),
		kTeam.countNumCitiesByArea(kArea), kArea.getNumCities(), kTeam.getNumWars(), kTeam.AI_isAnyWarPlan() ? 1 : 0);
}

static char const* getSASGameRecordAITargetCityChangeSource(SASGameRecordAITargetCityChangeSource eSource)
{
	switch (eSource)
	{
	case SAS_AI_TARGET_CITY_AREA_SEARCH: return "AREA_SEARCH";
	case SAS_AI_TARGET_CITY_RANDOM_CLEAR: return "RANDOM_CLEAR";
	case SAS_AI_TARGET_CITY_DIPLO_COORDINATION: return "DIPLO_COORDINATION";
	case SAS_AI_TARGET_CITY_AREA_REASSIGN_CLEAR: return "AREA_REASSIGN_CLEAR";
	case SAS_AI_TARGET_CITY_CITY_REMOVED: return "CITY_REMOVED";
	default: return "UNKNOWN";
	}
}

// <!-- custom: Target-city state is changed by periodic AI search/random clearing, diplomacy coordination and area reassignment; deleting the referenced city also makes the stored IDInfo resolve to NULL without a setter.
// Record those authoritative/effective changes only from their real paths. `selectionValue` is the already-computed randomized winning AI_targetCityValue from AREA_SEARCH and is -1 for non-search causes; no target valuation is repeated for logging. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAITargetCityChanged(CvPlayerAI const& kPlayer, CvArea const& kArea, CvCity const* pOldCity, CvCity const* pNewCity, SASGameRecordAITargetCityChangeSource eSource, int iSelectionValue)
{
	if (pOldCity == pNewCity)
		return;
	PlayerTypes const eOldOwner = (pOldCity == NULL ? NO_PLAYER : pOldCity->getOwner());
	TeamTypes const eOldTeam = (pOldCity == NULL ? NO_TEAM : pOldCity->getTeam());
	int const iOldCity = (pOldCity == NULL ? -1 : pOldCity->getID());
	int const iOldX = (pOldCity == NULL ? -1 : pOldCity->getX());
	int const iOldY = (pOldCity == NULL ? -1 : pOldCity->getY());
	PlayerTypes const eNewOwner = (pNewCity == NULL ? NO_PLAYER : pNewCity->getOwner());
	TeamTypes const eNewTeam = (pNewCity == NULL ? NO_TEAM : pNewCity->getTeam());
	int const iNewCity = (pNewCity == NULL ? -1 : pNewCity->getID());
	int const iNewX = (pNewCity == NULL ? -1 : pNewCity->getX());
	int const iNewY = (pNewCity == NULL ? -1 : pNewCity->getY());
	CvTeamAI const& kTeam = GET_TEAM(kPlayer.getTeam());
	WarPlanTypes const eNewWarPlan = (eNewTeam == NO_TEAM ? NO_WARPLAN : kTeam.AI_getWarPlan(eNewTeam));
	bool const bAtWarWithNewTarget = (eNewTeam != NO_TEAM && kTeam.isAtWar(eNewTeam));
	logSASGameRecord("GAME_RECORD_AI_TARGET_CITY_CHANGE turn=%d player=%d team=%d area=%d source=%s oldTargetPlayer=%d oldTargetTeam=%d oldTargetCity=%d oldTargetX=%d oldTargetY=%d newTargetPlayer=%d newTargetTeam=%d newTargetCity=%d newTargetX=%d newTargetY=%d selectionValue=%d areaAI=%s atWarWithNewTarget=%d newTargetWarPlan=%s",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), kArea.getID(), getSASGameRecordAITargetCityChangeSource(eSource),
		eOldOwner, eOldTeam, iOldCity, iOldX, iOldY, eNewOwner, eNewTeam, iNewCity, iNewX, iNewY, iSelectionValue,
		getSASAreaAIType(kArea.getAreaAIType(kPlayer.getTeam())), bAtWarWithNewTarget ? 1 : 0, getSASWarPlanType(eNewWarPlan));
}

static char const* getSASGameRecordAIConquerCityOutcome(SASGameRecordAIConquerCityOutcome eOutcome)
{
	switch (eOutcome)
	{
	case SAS_AI_CONQUER_CITY_KEEP: return "KEEP";
	case SAS_AI_CONQUER_CITY_RAZE: return "RAZE";
	case SAS_AI_CONQUER_CITY_LIBERATE: return "LIBERATE";
	default: return "UNKNOWN";
	}
}

static char const* getSASGameRecordAIConquerCityReason(SASGameRecordAIConquerCityReason eReason)
{
	switch (eReason)
	{
	case SAS_AI_CONQUER_CITY_CANNOT_RAZE: return "CANNOT_RAZE";
	case SAS_AI_CONQUER_CITY_DOMINATION3_PRIMARY_AREA_KEEP: return "DOMINATION3_PRIMARY_AREA_KEEP";
	case SAS_AI_CONQUER_CITY_CULTURE_VICTORY: return "CULTURE_VICTORY";
	case SAS_AI_CONQUER_CITY_UNLIKELY_LONG_TERM_BENEFIT: return "SAS_UNLIKELY_LONG_TERM_BENEFIT";
	case SAS_AI_CONQUER_CITY_EARLY_REMOTE_BARB: return "SAS_EARLY_REMOTE_BARB";
	case SAS_AI_CONQUER_CITY_EARLY_REMOTE_NONBARB: return "SAS_EARLY_REMOTE_NONBARB";
	case SAS_AI_CONQUER_CITY_BARBARIAN_VALUE: return "BARBARIAN_VALUE";
	case SAS_AI_CONQUER_CITY_NORMAL_VALUE: return "NORMAL_VALUE";
	case SAS_AI_CONQUER_CITY_LIBERATION: return "LIBERATION";
	case SAS_AI_CONQUER_CITY_LIBERATION_WITHHELD_HOSTAGE: return "LIBERATION_WITHHELD_HOSTAGE";
	default: return "UNKNOWN";
	}
}

// <!-- custom: Compact realized captured-city disposition. Normal AdvCiv valuation reuses the component boundaries and random draw gameplay already computed; forced rules and no-raze exits deliberately leave valueValid=0 rather than reconstructing a hypothetical value.
// Barbarian `barbarianRollPassed` preserves the inherited percentage-roll result separately from the final positive-value threshold, making that path auditable without consuming another RNG draw. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIConquerCityDecision(CvPlayerAI const& kPlayer, CvCity const& kCity, SASGameRecordAIConquerCityOutcome eOutcome, SASGameRecordAIConquerCityReason eReason, bool bEverOwned, int iCloseness, bool bValueValid, int iRazeValueBeforeRandom, int iRazeRandom, int iRazeValue, bool bComponentsValid, int iDistanceAndLocalPower, int iMaintenanceDelta, int iPopulationDelta, int iPersonalityDominationDelta, int iOtherDelta, int iFinancialTrouble, int iBarbarianRollPassed, PlayerTypes eLiberationPlayer)
{
	PlayerTypes const ePreviousOwner = kCity.getPreviousOwner();
	TeamTypes const ePreviousTeam = (ePreviousOwner == NO_PLAYER ? NO_TEAM : GET_PLAYER(ePreviousOwner).getTeam());
	logSASGameRecord("GAME_RECORD_AI_CONQUER_CITY_DECISION turn=%d player=%d team=%d cityId=%d city=%S x=%d y=%d area=%d previousOwner=%d previousTeam=%d originalOwner=%d pop=%d everOwned=%d outcome=%s reason=%s valueValid=%d razeValueBeforeRandom=%d random=%d razeValue=%d threshold=0 componentsValid=%d distanceAndLocalPower=%d maintenanceDelta=%+d populationDelta=%+d personalityDominationDelta=%+d otherDelta=%+d financialTrouble=%d closeness=%d barbarianRollPassed=%d liberationPlayer=%d",
		GC.getGame().getGameTurn(), kPlayer.getID(), kPlayer.getTeam(), kCity.getID(), kCity.getName().GetCString(), kCity.getX(),
		kCity.getY(), kCity.getArea().getID(), ePreviousOwner, ePreviousTeam, kCity.getOriginalOwner(), kCity.getPopulation(),
		bEverOwned ? 1 : 0, getSASGameRecordAIConquerCityOutcome(eOutcome), getSASGameRecordAIConquerCityReason(eReason),
		bValueValid ? 1 : 0, iRazeValueBeforeRandom, iRazeRandom, iRazeValue, bComponentsValid ? 1 : 0, iDistanceAndLocalPower,
		iMaintenanceDelta, iPopulationDelta, iPersonalityDominationDelta, iOtherDelta, iFinancialTrouble, iCloseness, iBarbarianRollPassed,
		eLiberationPlayer);
}

static char const* getSASGameRecordAIGreatPersonAction(SASGameRecordAIGreatPersonAction eAction)
{
	switch (eAction)
	{
	case SAS_AI_GREAT_PERSON_DISCOVER_TECH: return "DISCOVER_TECH";
	case SAS_AI_GREAT_PERSON_TRADE_MISSION: return "TRADE_MISSION";
	case SAS_AI_GREAT_PERSON_MOVE_TO_TRADE_MISSION: return "MOVE_TO_TRADE_MISSION";
	case SAS_AI_GREAT_PERSON_GREAT_WORK: return "GREAT_WORK";
	case SAS_AI_GREAT_PERSON_MOVE_TO_GREAT_WORK: return "MOVE_TO_GREAT_WORK";
	case SAS_AI_GREAT_PERSON_GOLDEN_AGE: return "GOLDEN_AGE";
	case SAS_AI_GREAT_PERSON_JOIN_CITY: return "JOIN_CITY";
	case SAS_AI_GREAT_PERSON_MOVE_TO_JOIN_CITY: return "MOVE_TO_JOIN_CITY";
	case SAS_AI_GREAT_PERSON_CONSTRUCT_BUILDING: return "CONSTRUCT_BUILDING";
	case SAS_AI_GREAT_PERSON_MOVE_TO_CONSTRUCT_BUILDING: return "MOVE_TO_CONSTRUCT_BUILDING";
	case SAS_AI_GREAT_PERSON_HURRY_BUILDING: return "HURRY_BUILDING";
	case SAS_AI_GREAT_PERSON_MOVE_TO_HURRY_BUILDING: return "MOVE_TO_HURRY_BUILDING";
	case SAS_AI_GREAT_PERSON_DANGER_DISCOVER_TECH: return "DANGER_DISCOVER_TECH";
	case SAS_AI_GREAT_PERSON_RECON_SPY: return "RECON_SPY";
	case SAS_AI_GREAT_PERSON_RETREAT: return "RETREAT";
	case SAS_AI_GREAT_PERSON_STRANDED: return "STRANDED";
	case SAS_AI_GREAT_PERSON_SAFETY: return "SAFETY";
	case SAS_AI_GREAT_PERSON_SKIP: return "SKIP";
	default: return "UNKNOWN";
	}
}

// <!-- custom: Compact all-type Great Person action provenance.
// The five action scores and slow candidate metadata are the exact values produced by AI_greatPersonMove before sorting; target is the already-selected current waypoint/action plot and previousMission* preserves continuity from the group's pre-decision state.
// Fallback actions deliberately use choiceRank/selectedValue=-1: danger discovery, recon, retreat, stranded handling and safety are later fallback helpers, not winners of the earlier sorted score comparison. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIGreatPersonDecision(CvUnitAI const& kUnit, CvPlot const* pDecisionPlot, SASGameRecordAIGreatPersonAction eAction, int iChoiceRank, int iSelectedValue, int iScoreThreshold, int iSlowValue, int iSlowBaseValue, int iSlowPathTurns, MissionAITypes eSlowMissionAI, CvCity const* pSlowCity, SpecialistTypes eSpecialist, BuildingTypes eBuilding, int iDiscoverValue, TechTypes eDiscoverTech, int iGoldenAgeValue, int iTradeValue, int iCultureValue, CvPlot const* pTargetPlot, MissionAITypes ePreviousMissionAI, CvPlot const* pPreviousMissionPlot)
{
	CvGame const& kGame = GC.getGame();
	int const iAge = kGame.getGameTurn() - kUnit.getGameTurnCreated();
	int const iAgeNormal = 100 * iAge / std::max(1, kGame.getSpeedPercent());
	logSASGameRecord("GAME_RECORD_AI_GREAT_PERSON_DECISION turn=%d player=%d team=%d unitId=%d unit=%s unitAI=%s x=%d y=%d age=%d ageNormal=%d action=%s choiceRank=%d selectedValue=%d threshold=%d slow=%d slowBaseValue=%d slowPathTurns=%d slowMissionAI=%d slowCityId=%d slowCity=%S slowCityX=%d slowCityY=%d specialist=%s building=%s discover=%d discoverTech=%s goldenAge=%d trade=%d culture=%d targetX=%d targetY=%d previousMissionAI=%d previousTargetX=%d previousTargetY=%d",
		kGame.getGameTurn(), kUnit.getOwner(), kUnit.getTeam(), kUnit.getID(), getSASGameRecordUnitType(kUnit.getUnitType()),
		getSASGameRecordUnitAIType(kUnit.AI_getUnitAIType()), (pDecisionPlot == NULL ? kUnit.getX() : pDecisionPlot->getX()),
		(pDecisionPlot == NULL ? kUnit.getY() : pDecisionPlot->getY()), iAge, iAgeNormal,
		getSASGameRecordAIGreatPersonAction(eAction), iChoiceRank, iSelectedValue, iScoreThreshold, iSlowValue, iSlowBaseValue,
		(iSlowPathTurns == MAX_INT ? -1 : iSlowPathTurns), eSlowMissionAI, (pSlowCity == NULL ? -1 : pSlowCity->getID()),
		(pSlowCity == NULL ? L"-" : pSlowCity->getName().GetCString()), (pSlowCity == NULL ? -1 : pSlowCity->getX()),
		(pSlowCity == NULL ? -1 : pSlowCity->getY()), getSASGameRecordSpecialistType(eSpecialist), getSASGameRecordBuildingType(eBuilding),
		iDiscoverValue, getSASGameRecordTechType(eDiscoverTech), iGoldenAgeValue, iTradeValue, iCultureValue,
		(pTargetPlot == NULL ? -1 : pTargetPlot->getX()), (pTargetPlot == NULL ? -1 : pTargetPlot->getY()), ePreviousMissionAI,
		(pPreviousMissionPlot == NULL ? -1 : pPreviousMissionPlot->getX()), (pPreviousMissionPlot == NULL ? -1 : pPreviousMissionPlot->getY()));
}

static char const* getSASGameRecordAIGreatGeneralStage(SASGameRecordAIGreatGeneralStage eStage)
{
	switch (eStage)
	{
	case SAS_AI_GREAT_GENERAL_PREFERRED_INSTRUCTOR: return "PREFERRED_INSTRUCTOR";
	case SAS_AI_GREAT_GENERAL_FIRST_ACADEMY: return "FIRST_ACADEMY";
	case SAS_AI_GREAT_GENERAL_FIRST_INSTRUCTOR: return "FIRST_INSTRUCTOR";
	case SAS_AI_GREAT_GENERAL_DANGER_LEAD: return "DANGER_LEAD";
	case SAS_AI_GREAT_GENERAL_OFFENSE_LEAD_ATTACK_CITY: return "OFFENSE_LEAD_ATTACK_CITY";
	case SAS_AI_GREAT_GENERAL_OFFENSE_LEAD_ATTACK: return "OFFENSE_LEAD_ATTACK";
	case SAS_AI_GREAT_GENERAL_JOIN_LIMIT_2: return "JOIN_LIMIT_2";
	case SAS_AI_GREAT_GENERAL_ACADEMY_LIMIT_2: return "ACADEMY_LIMIT_2";
	case SAS_AI_GREAT_GENERAL_JOIN_LIMIT_4: return "JOIN_LIMIT_4";
	case SAS_AI_GREAT_GENERAL_RANDOM_CONSTRUCT: return "RANDOM_CONSTRUCT";
	case SAS_AI_GREAT_GENERAL_FINAL_JOIN: return "FINAL_JOIN";
	case SAS_AI_GREAT_GENERAL_RETREAT: return "RETREAT";
	case SAS_AI_GREAT_GENERAL_STRANDED: return "STRANDED";
	case SAS_AI_GREAT_GENERAL_SAFETY: return "SAFETY";
	case SAS_AI_GREAT_GENERAL_SKIP: return "SKIP";
	default: return "UNKNOWN";
	}
}

static char const* getSASGameRecordAIGreatGeneralAction(SASGameRecordAIGreatGeneralAction eAction)
{
	switch (eAction)
	{
	case SAS_AI_GREAT_GENERAL_ACTION_JOIN: return "JOIN";
	case SAS_AI_GREAT_GENERAL_ACTION_CONSTRUCT: return "CONSTRUCT";
	case SAS_AI_GREAT_GENERAL_ACTION_LEAD: return "LEAD";
	case SAS_AI_GREAT_GENERAL_ACTION_RETREAT: return "RETREAT";
	case SAS_AI_GREAT_GENERAL_ACTION_STRANDED: return "STRANDED";
	case SAS_AI_GREAT_GENERAL_ACTION_SAFETY: return "SAFETY";
	case SAS_AI_GREAT_GENERAL_ACTION_SKIP: return "SKIP";
	default: return "UNKNOWN";
	}
}

// <!-- custom: Compact Great-General policy provenance. Unlike ordinary Great People, AI_generalMove uses an ordered fallback chain rather than a cross-action score; stage therefore records the live policy branch and the optional helper context preserves only the target/value/path the selected helper already computed. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIGreatGeneralDecision(CvUnitAI const& kUnit, SASGreatGeneralChoiceContext const& kChoice)
{
	CvGame const& kGame = GC.getGame();
	CvPlayerAI const& kOwner = GET_PLAYER(kUnit.getOwner());
	int const iAge = kGame.getGameTurn() - kUnit.getGameTurnCreated();
	int const iAgeNormal = 100 * iAge / std::max(1, kGame.getSpeedPercent());
	CvCity const* pTargetCity = kChoice.pTargetCity;
	CvUnit const* pTargetUnit = kChoice.pTargetUnit;
	CvPlot const* pTargetPlot = kChoice.pTargetPlot;
	CvPlot const* pWaypointPlot = kChoice.pWaypointPlot;
	CvPlot const& kDecisionPlot = (kChoice.pDecisionPlot == NULL ? kUnit.getPlot() : *kChoice.pDecisionPlot);
	logSASGameRecord("GAME_RECORD_AI_GREAT_GENERAL_DECISION turn=%d player=%d team=%d unitId=%d unit=%s unitAI=%s x=%d y=%d area=%d areaAI=%d age=%d ageNormal=%d action=%s stage=%s move=%d selectedValue=%d limit=%d valueThreshold=%d minStrength=%d minHealing=%d era=%d preferInstructorFirst=%d preferThroughEra=%d randomConstructRoll=%d specialist=%s building=%s targetCityId=%d targetCity=%S targetUnitId=%d targetUnit=%s targetUnitAI=%s targetStrengthScore=%d targetHealing=%d leadByHealing=%d targetX=%d targetY=%d waypointX=%d waypointY=%d previousMissionAI=%d previousTargetX=%d previousTargetY=%d",
		kGame.getGameTurn(), kUnit.getOwner(), kUnit.getTeam(), kUnit.getID(), getSASGameRecordUnitType(kUnit.getUnitType()), getSASGameRecordUnitAIType(kUnit.AI_getUnitAIType()),
		kDecisionPlot.getX(), kDecisionPlot.getY(), kDecisionPlot.getArea().getID(), kDecisionPlot.getArea().getAreaAIType(kUnit.getTeam()), iAge, iAgeNormal,
		getSASGameRecordAIGreatGeneralAction(kChoice.eAction), getSASGameRecordAIGreatGeneralStage(kChoice.eStage), (int)kChoice.bMove, kChoice.iSelectedValue,
		(kChoice.iPolicyLimit == MAX_INT ? -1 : kChoice.iPolicyLimit), kChoice.iValueThreshold, kChoice.iMinStrength, kChoice.iMinHealing,
		kOwner.getCurrentEra(), kChoice.bPreferInstructorFirst, kChoice.iPreferThroughEra, kChoice.iRandomConstructRoll,
		getSASGameRecordSpecialistType(kChoice.eSpecialist), getSASGameRecordBuildingType(kChoice.eBuilding),
		(pTargetCity == NULL ? -1 : pTargetCity->getID()), (pTargetCity == NULL ? L"-" : pTargetCity->getName().GetCString()),
		(pTargetUnit == NULL ? -1 : pTargetUnit->getID()), (pTargetUnit == NULL ? "-" : getSASGameRecordUnitType(pTargetUnit->getUnitType())),
		(pTargetUnit == NULL ? "-" : getSASGameRecordUnitAIType(pTargetUnit->AI_getUnitAIType())), kChoice.iTargetStrengthScore, kChoice.iTargetHealing,
		kChoice.iLeadByHealing, (pTargetPlot == NULL ? -1 : pTargetPlot->getX()), (pTargetPlot == NULL ? -1 : pTargetPlot->getY()),
		(pWaypointPlot == NULL ? -1 : pWaypointPlot->getX()), (pWaypointPlot == NULL ? -1 : pWaypointPlot->getY()), kChoice.ePreviousMissionAI,
		(kChoice.pPreviousMissionPlot == NULL ? -1 : kChoice.pPreviousMissionPlot->getX()), (kChoice.pPreviousMissionPlot == NULL ? -1 : kChoice.pPreviousMissionPlot->getY()));
}


// <!-- custom: Serialize only scores produced by the real AI_bestReligion loop; the vector is built only at GameRecord level 3 and formatted only when AI_doReligion reaches a meaningful switch/spread-block decision. (ChatGPT-5.6-Sol) -->
static CvString getSASGameRecordReligionCandidateScores(std::vector<std::pair<ReligionTypes, int> > const* paCandidateValues)
{
	if (paCandidateValues == NULL || paCandidateValues->empty())
		return CvString("-");
	CvString szResult;
	for (size_t i = 0; i < paCandidateValues->size(); i++)
	{
		CvString szItem;
		szItem.Format(i == 0 ? "%s:%d" : ",%s:%d", getSASGameRecordReligionType((*paCandidateValues)[i].first), (*paCandidateValues)[i].second);
		szResult += szItem;
	}
	return szResult;
}

// <!-- custom: Record only meaningful AI religion-switch decisions. `evaluatedBest` is the pre-spread-gate winner using AI_bestReligion's real post-bias score; `selectedReligion` is the post-gate/fallback target actually considered by AI_doReligion.
// Raw values are present only when gameplay itself needed them for the conversion probability. `stateReligionAfter` confirms realized conversion without adding another decision evaluation. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIReligionDecision(PlayerTypes ePlayer, ReligionTypes eCurrentReligion, ReligionTypes eEvaluatedBest, ReligionTypes eSelectedReligion, ReligionTypes eRunnerUp, int iBestValue, int iRunnerUpValue, int iCurrentScore, int iCurrentRawValue, int iSelectedRawValue, int iConvertProbabilityPercent, int iRollSuccess, char const* szOutcome, std::vector<std::pair<ReligionTypes, int> > const* paCandidateValues)
{
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer).AI();
	int iBestSpreadPercent = -1;
	int iBestHolyCityOwned = -1;
	if (eEvaluatedBest != NO_RELIGION)
	{
		iBestSpreadPercent = kPlayer.getHasReligionCount(eEvaluatedBest) * 100 /
				std::min(kPlayer.getNumCities() + 1, (GC.getInfo(GC.getMap().getWorldSize()).getTargetNumCities() * 3) / 2 + 1);
		CvCity const* pHolyCity = GC.getGame().getHolyCity(eEvaluatedBest);
		iBestHolyCityOwned = (pHolyCity != NULL && pHolyCity->getTeam() == kPlayer.getTeam() ? 1 : 0);
	}
	CvString const szCandidateScores = getSASGameRecordReligionCandidateScores(paCandidateValues);
	logSASGameRecord("GAME_RECORD_AI_RELIGION_DECISION turn=%d player=%d team=%d currentReligion=%s evaluatedBestReligion=%s selectedReligion=%s stateReligionAfter=%s runnerUpReligion=%s bestScore=%d runnerUpScore=%d currentScore=%d bestMinusCurrentScore=%d currentRawValue=%d selectedRawValue=%d favoriteReligion=%s missionaryStrategy=%d religionFlavor=%d bestSpreadPercent=%d bestHolyCityOwned=%d anarchyLength=%d convertProbabilityPercent=%d rollSuccess=%d outcome=%s candidateScores=%s",
		GC.getGame().getGameTurn(), ePlayer, kPlayer.getTeam(), getSASGameRecordReligionType(eCurrentReligion),
		getSASGameRecordReligionType(eEvaluatedBest), getSASGameRecordReligionType(eSelectedReligion),
		getSASGameRecordReligionType(kPlayer.getStateReligion()), getSASGameRecordReligionType(eRunnerUp), iBestValue, iRunnerUpValue,
		iCurrentScore, (iCurrentScore < 0 ? -1 : iBestValue - iCurrentScore), iCurrentRawValue, iSelectedRawValue,
		getSASGameRecordReligionType(kPlayer.getFavoriteReligion()), isSASGameRecordAIStrategyActive(kPlayer, AI_STRATEGY_MISSIONARY) ? 1 : 0,
		kPlayer.AI_getFlavorValue(FLAVOR_RELIGION), iBestSpreadPercent, iBestHolyCityOwned, kPlayer.getReligionAnarchyLength(),
		iConvertProbabilityPercent, iRollSuccess, szOutcome, szCandidateScores.GetCString());
}


// <!-- custom: Vote helpers below are recorder schema, not gameplay abstractions: they translate native vote enums/flags into stable factual history tokens while leaving AI valuation in BBAI. (ChatGPT-5.6-Sol) -->
static CvString getSASGameRecordPlayerVoteChoice(PlayerVoteTypes eChoice)
{
	CvString szChoice;
	if (eChoice >= 0 && eChoice < MAX_CIV_TEAMS)
	{
		szChoice.Format("TEAM_%d", eChoice);
		return szChoice;
	}
	switch (eChoice)
	{
	case NO_PLAYER_VOTE_CHECKED: return CvString("UNCHECKED");
	case PLAYER_VOTE_NEVER: return CvString("NEVER");
	case PLAYER_VOTE_ABSTAIN: return CvString("ABSTAIN");
	case PLAYER_VOTE_NO: return CvString("NO");
	case PLAYER_VOTE_YES: return CvString("YES");
	case NO_PLAYER_VOTE: return CvString("NONE");
	default:
		szChoice.Format("VALUE_%d", eChoice);
		return szChoice;
	}
}

static CvString getSASGameRecordVoteEffects(VoteTypes eVote)
{
	if (eVote == NO_VOTE)
		return CvString("-");
	CvVoteInfo const& kVote = GC.getInfo(eVote);
	CvString szEffects;
	CvString szItem;
	if (kVote.isSecretaryGeneral())
		szEffects = "SECRETARY_GENERAL";
	if (kVote.isVictory())
	{
		if (!szEffects.empty()) szEffects += ",";
		szEffects += "DIPLOMATIC_VICTORY";
	}
	if (kVote.getTradeRoutes() != 0)
	{
		szItem.Format(szEffects.empty() ? "TRADE_ROUTES:%+d" : ",TRADE_ROUTES:%+d", kVote.getTradeRoutes());
		szEffects += szItem;
	}
	if (kVote.isFreeTrade())
		szEffects += (szEffects.empty() ? "FREE_TRADE" : ",FREE_TRADE");
	if (kVote.isNoNukes())
		szEffects += (szEffects.empty() ? "NO_NUKES" : ",NO_NUKES");
	FOR_EACH_NON_DEFAULT_KEY(kVote.isForceCivic(), Civic)
	{
		szItem.Format(szEffects.empty() ? "FORCE_CIVIC:%s" : ",FORCE_CIVIC:%s", getSASGameRecordCivicType(eLoopCivic));
		szEffects += szItem;
	}
	if (kVote.isOpenBorders())
		szEffects += (szEffects.empty() ? "OPEN_BORDERS" : ",OPEN_BORDERS");
	if (kVote.isDefensivePact())
		szEffects += (szEffects.empty() ? "DEFENSIVE_PACT" : ",DEFENSIVE_PACT");
	if (kVote.isForcePeace())
		szEffects += (szEffects.empty() ? "FORCE_PEACE" : ",FORCE_PEACE");
	if (kVote.isForceNoTrade())
		szEffects += (szEffects.empty() ? "EMBARGO" : ",EMBARGO");
	if (kVote.isForceWar())
		szEffects += (szEffects.empty() ? "FORCE_WAR" : ",FORCE_WAR");
	if (kVote.isAssignCity())
		szEffects += (szEffects.empty() ? "ASSIGN_CITY" : ",ASSIGN_CITY");
	return getSASDiagnosticOrDash(szEffects);
}

static bool isSASGameRecordPersistentVote(VoteTypes eVote)
{
	if (eVote == NO_VOTE)
		return false;
	CvVoteInfo const& kVote = GC.getInfo(eVote);
	return (kVote.getTradeRoutes() != 0 || kVote.isFreeTrade() || kVote.isNoNukes() || kVote.isForceCivic().isAnyNonDefault());
}

static CvString getSASGameRecordVotePlayerMask(qword uiPlayers)
{
	CvString szPlayers;
	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
	{
		if ((uiPlayers & ((qword)1 << iPlayer)) != 0)
			appendSASDiagnosticIntListValue(szPlayers, iPlayer);
	}
	return getSASDiagnosticOrDash(szPlayers);
}

static CvString getSASGameRecordVoteEligibleTeams(VoteTriggeredData const& kTriggered)
{
	CvGame const& kGame = GC.getGame();
	if (!kGame.isTeamVote(kTriggered.kVoteOption.eVote))
		return CvString("-");
	CvString szTeams;
	for (TeamIter<MAJOR_CIV> itTeam; itTeam.hasNext(); ++itTeam)
	{
		if (kGame.isTeamVoteEligible(itTeam->getID(), kTriggered.eVoteSource))
			appendSASDiagnosticIntListValue(szTeams, itTeam->getID());
	}
	return getSASDiagnosticOrDash(szTeams);
}

static CvString getSASGameRecordVoteBallots(VoteTriggeredData const& kTriggered)
{
	CvGame const& kGame = GC.getGame();
	CvString szBallots;
	for (PlayerIter<MAJOR_CIV> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		if (!itPlayer->isVotingMember(kTriggered.eVoteSource))
			continue;
		PlayerVoteTypes const eChoice = kGame.getPlayerVote(itPlayer->getID(), kTriggered.getID());
		CvString szItem;
		szItem.Format(szBallots.empty() ? "P%d@T%d:%s:%d" : ",P%d@T%d:%s:%d",
				itPlayer->getID(), itPlayer->getTeam(), getSASGameRecordPlayerVoteChoice(eChoice).GetCString(),
				itPlayer->getVotes(kTriggered.kVoteOption.eVote, kTriggered.eVoteSource));
		szBallots += szItem;
	}
	return getSASDiagnosticOrDash(szBallots);
}

static void getSASGameRecordVoteTarget(VoteSelectionSubData const& kVoteOption, PlayerTypes& eTargetPlayer, TeamTypes& eTargetTeam, CvCity const*& pTargetCity, PlayerTypes& eOtherPlayer, TeamTypes& eOtherTeam)
{
	eTargetPlayer = kVoteOption.ePlayer;
	eTargetTeam = (eTargetPlayer == NO_PLAYER ? NO_TEAM : GET_PLAYER(eTargetPlayer).getTeam());
	pTargetCity = NULL;
	if (eTargetPlayer != NO_PLAYER && kVoteOption.iCityId >= 0)
		pTargetCity = GET_PLAYER(eTargetPlayer).getCity(kVoteOption.iCityId);
	eOtherPlayer = kVoteOption.eOtherPlayer;
	eOtherTeam = (eOtherPlayer == NO_PLAYER ? NO_TEAM : GET_PLAYER(eOtherPlayer).getTeam());
}

static char const* getSASGameRecordAIDiploVoteReason(SASGameRecordAIDiploVoteReason eReason)
{
	switch (eReason)
	{
	case SAS_AI_DIPLO_VOTE_TEAM_SELF_ELIGIBLE: return "TEAM_SELF_ELIGIBLE";
	case SAS_AI_DIPLO_VOTE_TEAM_VASSAL_MASTER: return "TEAM_VASSAL_MASTER";
	case SAS_AI_DIPLO_VOTE_TEAM_OWN_DIPLO_VICTORY_ABSTAIN: return "TEAM_OWN_DIPLO_VICTORY_ABSTAIN";
	case SAS_AI_DIPLO_VOTE_TEAM_ATTITUDE_TIE_ABSTAIN: return "TEAM_ATTITUDE_TIE_ABSTAIN";
	case SAS_AI_DIPLO_VOTE_TEAM_BEST_ATTITUDE: return "TEAM_BEST_ATTITUDE";
	case SAS_AI_DIPLO_VOTE_SECRETARY_SELF_OR_MASTER: return "SECRETARY_SELF_OR_MASTER";
	case SAS_AI_DIPLO_VOTE_FRIENDLY_SECRETARY: return "FRIENDLY_SECRETARY";
	case SAS_AI_DIPLO_VOTE_FORCE_CIVIC: return "FORCE_CIVIC";
	case SAS_AI_DIPLO_VOTE_TRADE_ROUTES: return "TRADE_ROUTES";
	case SAS_AI_DIPLO_VOTE_NO_NUKES: return "NO_NUKES";
	case SAS_AI_DIPLO_VOTE_FREE_TRADE: return "FREE_TRADE";
	case SAS_AI_DIPLO_VOTE_OPEN_BORDERS: return "OPEN_BORDERS";
	case SAS_AI_DIPLO_VOTE_DEFENSIVE_PACT: return "DEFENSIVE_PACT";
	case SAS_AI_DIPLO_VOTE_FORCE_PEACE: return "FORCE_PEACE";
	case SAS_AI_DIPLO_VOTE_EMBARGO_RECENT_DEAL: return "EMBARGO_RECENT_DEAL";
	case SAS_AI_DIPLO_VOTE_EMBARGO_UNMET_ABSTAIN: return "EMBARGO_UNMET_ABSTAIN";
	case SAS_AI_DIPLO_VOTE_EMBARGO: return "EMBARGO";
	case SAS_AI_DIPLO_VOTE_FORCE_WAR: return "FORCE_WAR";
	case SAS_AI_DIPLO_VOTE_ASSIGN_CITY: return "ASSIGN_CITY";
	case SAS_AI_DIPLO_VOTE_DEFAULT: return "DEFAULT";
	default: return "UNKNOWN";
	}
}

static void logSASGameRecordAIDiploVoteDecisionImpl(CvPlayerAI const& kPlayer, VoteSelectionSubData const& kVoteData, VoteSourceTypes eVoteSource, int iTriggeredVoteId, PlayerVoteTypes eChoice, SASGameRecordAIDiploVoteReason eReason, bool bHasDecisionMetrics, int iDecisionValue, int iDecisionThreshold, int iRandomRoll)
{
	CvGame const& kGame = GC.getGame();
	TeamTypes const eSecretaryTeam = kGame.getSecretaryGeneral(eVoteSource);
	CvString szSecretaryAttitude = "-";
	if (eSecretaryTeam != NO_TEAM && eSecretaryTeam != kPlayer.getTeam())
		szSecretaryAttitude.Format("%d", GET_TEAM(kPlayer.getTeam()).AI_getAttitudeVal(eSecretaryTeam));
	PlayerTypes eTargetPlayer;
	TeamTypes eTargetTeam;
	CvCity const* pTargetCity;
	PlayerTypes eOtherPlayer;
	TeamTypes eOtherTeam;
	getSASGameRecordVoteTarget(kVoteData, eTargetPlayer, eTargetTeam, pTargetCity, eOtherPlayer, eOtherTeam);
	CvString szDecisionValue = "-";
	CvString szDecisionThreshold = "-";
	CvString szRandomRoll = "-";
	if (bHasDecisionMetrics)
	{
		szDecisionValue.Format("%d", iDecisionValue);
		szDecisionThreshold.Format("%d", iDecisionThreshold);
		if (iRandomRoll >= 0)
			szRandomRoll.Format("%d", iRandomRoll);
	}
	logSASGameRecord("GAME_RECORD_AI_DIPLO_VOTE turn=%d triggeredId=%d player=%d team=%d source=%s vote=%s choice=%s reason=%s effects=%s repeal=%d secretaryTeam=%d secretaryAttitude=%s decisionValue=%s decisionThreshold=%s randomRoll=%s targetPlayer=%d targetTeam=%d targetCityId=%d targetCity=%S targetX=%d targetY=%d otherPlayer=%d otherTeam=%d",
		kGame.getGameTurn(), iTriggeredVoteId, kPlayer.getID(), kPlayer.getTeam(), getSASGameRecordVoteSourceType(eVoteSource),
		getSASGameRecordVoteType(kVoteData.eVote), getSASGameRecordPlayerVoteChoice(eChoice).GetCString(),
		getSASGameRecordAIDiploVoteReason(eReason), getSASGameRecordVoteEffects(kVoteData.eVote).GetCString(),
		kGame.getVoteOutcome(kVoteData.eVote) == PLAYER_VOTE_YES ? 1 : 0, eSecretaryTeam, szSecretaryAttitude.GetCString(),
		szDecisionValue.GetCString(), szDecisionThreshold.GetCString(), szRandomRoll.GetCString(), eTargetPlayer, eTargetTeam,
		kVoteData.iCityId, getSASGameRecordQuotedCityName(pTargetCity).GetCString(), pTargetCity == NULL ? -1 : pTargetCity->getX(),
		pTargetCity == NULL ? -1 : pTargetCity->getY(), eOtherPlayer, eOtherTeam);
}

void logSASGameRecordAIDiploVoteDecision(CvPlayerAI const& kPlayer, VoteSelectionSubData const& kVoteData, VoteSourceTypes eVoteSource, int iTriggeredVoteId, PlayerVoteTypes eChoice, SASGameRecordAIDiploVoteReason eReason)
{
	logSASGameRecordAIDiploVoteDecisionImpl(kPlayer, kVoteData, eVoteSource, iTriggeredVoteId, eChoice, eReason, false, 0, 0, -1);
}

void logSASGameRecordAIDiploVoteDecision(CvPlayerAI const& kPlayer, VoteSelectionSubData const& kVoteData, VoteSourceTypes eVoteSource, int iTriggeredVoteId, PlayerVoteTypes eChoice, SASGameRecordAIDiploVoteReason eReason, int iDecisionValue, int iDecisionThreshold, int iRandomRoll)
{
	logSASGameRecordAIDiploVoteDecisionImpl(kPlayer, kVoteData, eVoteSource, iTriggeredVoteId, eChoice, eReason, true, iDecisionValue, iDecisionThreshold, iRandomRoll);
}

void logSASGameRecordAIElectionChoice(CvTeamAI const& kTeam, VoteSelectionData const& kSelection, int iSelectedIndex, int iSelectedValue, int iSelectedRandomValue, int iSelectedVictoryBoost, int iValidOptions, int iRejectedOptions, int iVictoryBoostedOptions)
{
	VoteSelectionSubData const* pSelected = NULL;
	if (iSelectedIndex >= 0 && iSelectedIndex < (int)kSelection.aVoteOptions.size())
		pSelected = &kSelection.aVoteOptions[iSelectedIndex];
	PlayerTypes eTargetPlayer = NO_PLAYER;
	TeamTypes eTargetTeam = NO_TEAM;
	CvCity const* pTargetCity = NULL;
	PlayerTypes eOtherPlayer = NO_PLAYER;
	TeamTypes eOtherTeam = NO_TEAM;
	if (pSelected != NULL)
		getSASGameRecordVoteTarget(*pSelected, eTargetPlayer, eTargetTeam, pTargetCity, eOtherPlayer, eOtherTeam);
	logSASGameRecord("GAME_RECORD_AI_ELECTION_CHOICE turn=%d selectionId=%d team=%d secretaryPlayer=%d source=%s outcome=%s optionCount=%d validOptions=%d rejectedOptions=%d victoryBoostedOptions=%d selectedIndex=%d selectedVote=%s selectedEffects=%s selectedValue=%d selectedRandomValue=%d selectedVictoryBoost=%d targetPlayer=%d targetTeam=%d targetCityId=%d targetCity=%S targetX=%d targetY=%d otherPlayer=%d otherTeam=%d",
		GC.getGame().getGameTurn(), kSelection.getID(), kTeam.getID(), kTeam.getSecretaryID(), getSASGameRecordVoteSourceType(kSelection.eVoteSource),
		pSelected == NULL ? "NO_SELECTION" : "SELECTED", (int)kSelection.aVoteOptions.size(), iValidOptions, iRejectedOptions,
		iVictoryBoostedOptions, iSelectedIndex, pSelected == NULL ? "-" : getSASGameRecordVoteType(pSelected->eVote),
		pSelected == NULL ? "-" : getSASGameRecordVoteEffects(pSelected->eVote).GetCString(), iSelectedValue, iSelectedRandomValue,
		iSelectedVictoryBoost, eTargetPlayer, eTargetTeam, pSelected == NULL ? -1 : pSelected->iCityId,
		getSASGameRecordQuotedCityName(pTargetCity).GetCString(), pTargetCity == NULL ? -1 : pTargetCity->getX(),
		pTargetCity == NULL ? -1 : pTargetCity->getY(), eOtherPlayer, eOtherTeam);
}

void logSASGameRecordVoteTriggered(VoteTriggeredData const* pVoteTriggered)
{
	if (pVoteTriggered == NULL || pVoteTriggered->kVoteOption.eVote == NO_VOTE)
		return;
	CvGame const& kGame = GC.getGame();
	VoteTypes const eVote = pVoteTriggered->kVoteOption.eVote;
	CvVoteInfo const& kVote = GC.getInfo(eVote);
	TeamTypes const eSecretaryTeam = kGame.getSecretaryGeneral(pVoteTriggered->eVoteSource);
	PlayerTypes eProposer = NO_PLAYER;
	if (!kVote.isSecretaryGeneral() && eSecretaryTeam != NO_TEAM)
		eProposer = GET_TEAM(eSecretaryTeam).getSecretaryID();
	PlayerTypes eTargetPlayer;
	TeamTypes eTargetTeam;
	CvCity const* pTargetCity;
	PlayerTypes eOtherPlayer;
	TeamTypes eOtherTeam;
	getSASGameRecordVoteTarget(pVoteTriggered->kVoteOption, eTargetPlayer, eTargetTeam, pTargetCity, eOtherPlayer, eOtherTeam);
	bool const bPreviouslyPassed = kGame.isVotePassed(eVote);
	char const* szContext = (kVote.isSecretaryGeneral() ? "AUTOMATIC_SECRETARY_ELECTION" : (bPreviouslyPassed ? "ACTIVE_RESOLUTION_RECONSIDERATION" : "SECRETARY_PROPOSAL"));
	BuildingTypes const eSourceBuilding = kGame.getVoteSourceBuilding(pVoteTriggered->eVoteSource);
	CvCity const* pSourceCity = kGame.getVoteSourceCity(pVoteTriggered->eVoteSource, NO_TEAM);

	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=DIPLO_VOTE_TRIGGERED triggeredId=%d source=%s vote=%s context=%s sourceReligion=%s sourceBuilding=%s sourceOwner=%d sourceCityId=%d sourceCity=%S sourceX=%d sourceY=%d secretaryTeamBefore=%d proposerPlayer=%d teamVote=%d secretaryElection=%d victoryVote=%d previousOutcome=%s previouslyPassed=%d effects=%s eligibleTeams=%s requiredVotes=%d possibleVotes=%d targetPlayer=%d targetTeam=%d targetCityId=%d targetCity=%S targetX=%d targetY=%d otherPlayer=%d otherTeam=%d",
		kGame.getGameTurn(), pVoteTriggered->getID(), getSASGameRecordVoteSourceType(pVoteTriggered->eVoteSource),
		getSASGameRecordVoteType(eVote), szContext, getSASGameRecordReligionType(kGame.getVoteSourceReligion(pVoteTriggered->eVoteSource)),
		getSASGameRecordBuildingType(eSourceBuilding), pSourceCity == NULL ? -1 : pSourceCity->getOwner(),
		pSourceCity == NULL ? -1 : pSourceCity->getID(), getSASGameRecordQuotedCityName(pSourceCity).GetCString(),
		pSourceCity == NULL ? -1 : pSourceCity->getX(), pSourceCity == NULL ? -1 : pSourceCity->getY(), eSecretaryTeam, eProposer,
		kGame.isTeamVote(eVote) ? 1 : 0, kVote.isSecretaryGeneral() ? 1 : 0, kVote.isVictory() ? 1 : 0,
		getSASGameRecordPlayerVoteChoice(kGame.getVoteOutcome(eVote)).GetCString(), bPreviouslyPassed ? 1 : 0,
		getSASGameRecordVoteEffects(eVote).GetCString(), getSASGameRecordVoteEligibleTeams(*pVoteTriggered).GetCString(),
		kGame.getVoteRequired(eVote, pVoteTriggered->eVoteSource), kGame.countPossibleVote(eVote, pVoteTriggered->eVoteSource),
		eTargetPlayer, eTargetTeam, pVoteTriggered->kVoteOption.iCityId, getSASGameRecordQuotedCityName(pTargetCity).GetCString(),
		pTargetCity == NULL ? -1 : pTargetCity->getX(), pTargetCity == NULL ? -1 : pTargetCity->getY(), eOtherPlayer, eOtherTeam);
}

void logSASGameRecordVoteResult(VoteTriggeredData const* pVoteTriggered, bool bThresholdPassed, bool bPassed, bool bCancelled, qword uiDefaultedAbstain, qword uiDefiers, qword uiEndorsers)
{
	if (pVoteTriggered == NULL || pVoteTriggered->kVoteOption.eVote == NO_VOTE)
		return;
	CvGame const& kGame = GC.getGame();
	VoteTypes const eVote = pVoteTriggered->kVoteOption.eVote;
	CvVoteInfo const& kVote = GC.getInfo(eVote);
	bool const bTeamVote = kGame.isTeamVote(eVote);
	bool const bPreviouslyPassed = kGame.isVotePassed(eVote);
	TeamTypes eWinningTeam = NO_TEAM;
	int iWinningVotes = -1;
	if (bTeamVote && !bCancelled)
	{
		eWinningTeam = kGame.findHighestVoteTeam(*pVoteTriggered);
		if (eWinningTeam != NO_TEAM)
			iWinningVotes = kGame.countVote(*pVoteTriggered, (PlayerVoteTypes)eWinningTeam);
	}
	int const iYesVotes = (bTeamVote || bCancelled ? -1 : kGame.countVote(*pVoteTriggered, PLAYER_VOTE_YES));
	int const iNoVotes = (bTeamVote || bCancelled ? -1 : kGame.countVote(*pVoteTriggered, PLAYER_VOTE_NO));
	int const iAbstainVotes = (bCancelled ? -1 : kGame.countVote(*pVoteTriggered, PLAYER_VOTE_ABSTAIN));
	int const iNeverVotes = (bTeamVote || bCancelled ? -1 : kGame.countVote(*pVoteTriggered, PLAYER_VOTE_NEVER));
	char const* szResult = (bCancelled ? "CANCELLED" : (bPassed ? "PASSED" : (bThresholdPassed && uiDefiers != 0 ? "DEFIED" : "FAILED")));
	char const* szOperation = "NO_CHANGE";
	if (bCancelled)
		szOperation = "CANCELLED";
	else if (kVote.isSecretaryGeneral())
		szOperation = (bPassed ? "ELECT" : (bPreviouslyPassed ? "CLEAR_SECRETARY" : "NO_ELECTION"));
	else if (kVote.isVictory())
		szOperation = (bPassed ? "DIPLOMATIC_VICTORY" : "NO_VICTORY");
	else if (isSASGameRecordPersistentVote(eVote))
	{
		if (bPreviouslyPassed && !bPassed) szOperation = "DEACTIVATE";
		else if (!bPreviouslyPassed && bPassed) szOperation = "ACTIVATE";
		else if (bPreviouslyPassed && bPassed) szOperation = "KEEP_ACTIVE";
	}
	else if (bPassed)
		szOperation = "APPLY_ONE_SHOT";
	PlayerTypes eTargetPlayer;
	TeamTypes eTargetTeam;
	CvCity const* pTargetCity;
	PlayerTypes eOtherPlayer;
	TeamTypes eOtherTeam;
	getSASGameRecordVoteTarget(pVoteTriggered->kVoteOption, eTargetPlayer, eTargetTeam, pTargetCity, eOtherPlayer, eOtherTeam);
	TeamTypes const eSecretaryTeam = kGame.getSecretaryGeneral(pVoteTriggered->eVoteSource);

	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=DIPLO_VOTE_RESULT triggeredId=%d source=%s vote=%s result=%s operation=%s sourceReligion=%s secretaryTeamBefore=%d teamVote=%d secretaryElection=%d victoryVote=%d previousOutcome=%s previouslyPassed=%d thresholdPassed=%d resolutionPassed=%d winningTeam=%d winningVotes=%d yesVotes=%d noVotes=%d abstainVotes=%d neverVotes=%d requiredVotes=%d possibleVotes=%d defaultedAbstain=%s defiers=%s endorsers=%s effects=%s ballots=%s targetPlayer=%d targetTeam=%d targetCityId=%d targetCity=%S targetX=%d targetY=%d otherPlayer=%d otherTeam=%d",
		kGame.getGameTurn(), pVoteTriggered->getID(), getSASGameRecordVoteSourceType(pVoteTriggered->eVoteSource),
		getSASGameRecordVoteType(eVote), szResult, szOperation,
		getSASGameRecordReligionType(kGame.getVoteSourceReligion(pVoteTriggered->eVoteSource)), eSecretaryTeam, bTeamVote ? 1 : 0,
		kVote.isSecretaryGeneral() ? 1 : 0, kVote.isVictory() ? 1 : 0,
		getSASGameRecordPlayerVoteChoice(kGame.getVoteOutcome(eVote)).GetCString(), bPreviouslyPassed ? 1 : 0, bThresholdPassed ? 1 : 0,
		bPassed ? 1 : 0, eWinningTeam, iWinningVotes, iYesVotes, iNoVotes, iAbstainVotes, iNeverVotes,
		kGame.getVoteRequired(eVote, pVoteTriggered->eVoteSource), kGame.countPossibleVote(eVote, pVoteTriggered->eVoteSource),
		getSASGameRecordVotePlayerMask(uiDefaultedAbstain).GetCString(), getSASGameRecordVotePlayerMask(uiDefiers).GetCString(),
		getSASGameRecordVotePlayerMask(uiEndorsers).GetCString(), getSASGameRecordVoteEffects(eVote).GetCString(),
		getSASGameRecordVoteBallots(*pVoteTriggered).GetCString(), eTargetPlayer, eTargetTeam, pVoteTriggered->kVoteOption.iCityId,
		getSASGameRecordQuotedCityName(pTargetCity).GetCString(), pTargetCity == NULL ? -1 : pTargetCity->getX(),
		pTargetCity == NULL ? -1 : pTargetCity->getY(), eOtherPlayer, eOtherTeam);
}

void logSASGameRecordReligionFounded(ReligionTypes eReligion, PlayerTypes ePlayer)
{
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=RELIGION_FOUNDED player=%d religion=%s", GC.getGame().getGameTurn(), ePlayer, getSASGameRecordReligionType(eReligion));
}

void logSASGameRecordCorporationFounded(CorporationTypes eCorporation, PlayerTypes ePlayer)
{
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=CORPORATION_FOUNDED player=%d corporation=%s",
		GC.getGame().getGameTurn(), ePlayer, getSASGameRecordCorporationType(eCorporation));
}

void logSASGameRecordCityGrowthPrevented(CvCity const* pCity, int iFoodDiscarded)
{
	if (pCity == NULL)
		return;
	PlayerTypes const ePlayer = pCity->getOwner();
	if (ePlayer < 0 || ePlayer >= MAX_CIV_PLAYERS)
		return;
	// <!-- custom: Avoid Growth can repeatedly cap a city at its threshold and discard excess food.
	// Keep only interval totals so AI growth suppression is measurable without one action row per capped city turn. (ChatGPT-5.6-Sol) -->
	SASGameRecordPlayerFlow& kFlow = g_akSASGameRecordPlayerFlow[ePlayer];
	kFlow.iCityGrowthPreventedEvents++;
	kFlow.iFoodDiscardedByAvoidGrowth += std::max(0, iFoodDiscarded);
}

void logSASGameRecordCityPopulationChanged(CvCity const* pCity, bool bGrowth, int iPopulationBefore, int iFoodDifference, int iFoodBefore, int iFoodAfterDifference, int iFoodKeptBefore, int iFoodKeptBeforePopulationChange, int iGrowthThresholdBefore)
{
	if (pCity == NULL)
		return;
	PlayerTypes const ePlayer = pCity->getOwner();
	if (ePlayer < 0 || ePlayer >= MAX_CIV_PLAYERS)
		return;
	int const iPopulationAfter = pCity->getPopulation();
	int const iPopulationDelta = iPopulationAfter - iPopulationBefore;
	if ((bGrowth && iPopulationDelta <= 0) || (!bGrowth && iPopulationDelta >= 0))
		return;
	SASGameRecordPlayerFlow& kFlow = g_akSASGameRecordPlayerFlow[ePlayer];
	if (bGrowth)
	{
		kFlow.iCityGrowthEvents++;
		kFlow.iPopulationGainedFromGrowth += iPopulationDelta;
	}
	else
	{
		kFlow.iCityStarvationEvents++;
		kFlow.iPopulationLostToStarvation += -iPopulationDelta;
	}
	// <!-- custom: Level 2 keeps only interval natural-population totals; level 3 preserves the exact city transition and the food/granary states immediately before and after CvCity::doGrowth resolves it.
	// This hook is observation-only and is called after the existing population/food mutations, before Python's successful-growth event can add unrelated side effects. (ChatGPT-5.6-Sol) -->
	if (gGameRecordLogLevel >= 3)
	{
		logSASGameRecord("GAME_RECORD_ACTION turn=%d type=CITY_POPULATION_CHANGED cause=%s player=%d cityId=%d city=%S x=%d y=%d populationBefore=%d populationAfter=%d populationDelta=%+d foodDifference=%+d foodBefore=%d foodAfterDifference=%d foodAfter=%d foodKeptBefore=%d foodKeptBeforePopulationChange=%d foodKeptAfter=%d growthThresholdBefore=%d growthThresholdAfter=%d maxFoodKeptPercent=%d",
			GC.getGame().getGameTurn(), bGrowth ? "GROWTH" : "STARVATION", ePlayer, pCity->getID(),
			getSASGameRecordQuotedCityName(pCity).GetCString(), pCity->getX(), pCity->getY(), iPopulationBefore, iPopulationAfter,
			iPopulationDelta, iFoodDifference, iFoodBefore, iFoodAfterDifference, pCity->getFood(), iFoodKeptBefore,
			iFoodKeptBeforePopulationChange, pCity->getFoodKept(), iGrowthThresholdBefore, pCity->growthThreshold(),
			pCity->getMaxFoodKeptPercent());
	}
}

void logSASGameRecordCityCultureExpanded(CvCity const* pCity)
{
	if (pCity == NULL || pCity->getCultureLevel() == NO_CULTURELEVEL)
		return;
	CultureLevelTypes const eCultureLevel = pCity->getCultureLevel();
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=CITY_CULTURE_EXPANDED player=%d cityId=%d city=%S x=%d y=%d cultureLevel=%s cultureLevelId=%d ownerCultureTimes100=%d nextCultureThreshold=%d defenseModifier=%d totalDefense=%d",
		GC.getGame().getGameTurn(), pCity->getOwner(), pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(), pCity->getX(),
		pCity->getY(), GC.getInfo(eCultureLevel).getType(), eCultureLevel, pCity->getCultureTimes100(pCity->getOwner()),
		pCity->getCultureThreshold(), pCity->getDefenseModifier(false), pCity->getTotalDefense(false));
}

// <!-- custom: AI_doDraft's final conscription action preserves the unit but loses which live trigger released bWait and the enabling value/population context.
// Keep this serializer factual about the already-resolved trigger; rejected city turns remain intentionally unlogged. (ChatGPT-5.6-Sol) -->
static char const* getSASGameRecordAIDraftReason(SASGameRecordAIDraftReason eReason)
{
	switch (eReason)
	{
	case SAS_AI_DRAFT_FORCED_CALLER: return "FORCED_CALLER";
	case SAS_AI_DRAFT_TURTLE_STRATEGY: return "TURTLE_STRATEGY";
	case SAS_AI_DRAFT_LOCAL_DANGER: return "LOCAL_DANGER";
	case SAS_AI_DRAFT_NONCRITICAL_RANDOM_VALUE: return "NONCRITICAL_RANDOM_VALUE";
	default: return "UNKNOWN";
	}
}

void logSASGameRecordAIDraftDecision(CvCity const& kCity, SASGameRecordAIDraftReason eReason, UnitTypes eConscriptUnit, int iConscriptPopulation, int iDanger, int iLandWar, int iGoodValue, int iTooMuchPop, int iPoorPlots, int iHappyDiff, int iUnitCostPerMil, int iLocalDefense, int iLocalEnemyOffense, int iBuildUnitProb)
{
	logSASGameRecord("GAME_RECORD_AI_DRAFT_DECISION turn=%d player=%d cityId=%d city=%S reason=%s unit=%s population=%d highestPopulation=%d conscriptPopulation=%d conscriptAngerTimer=%d danger=%d landWar=%d goodValue=%d tooMuchPop=%d poorPlots=%d happyDiff=%d unitCostPerMil=%d localDefense=%d localEnemyOffense=%d buildUnitProb=%d",
		GC.getGame().getGameTurn(), kCity.getOwner(), kCity.getID(), getSASGameRecordQuotedCityName(&kCity).GetCString(), getSASGameRecordAIDraftReason(eReason),
		getSASGameRecordUnitType(eConscriptUnit), kCity.getPopulation(), kCity.getHighestPopulation(), iConscriptPopulation, kCity.getConscriptAngerTimer(),
		iDanger, iLandWar, iGoodValue, iTooMuchPop, iPoorPlots, iHappyDiff, iUnitCostPerMil, iLocalDefense, iLocalEnemyOffense, iBuildUnitProb);
}

// <!-- custom: AI_doHurry has several mutually exclusive realized exits whose live value/cost comparison is lost as soon as hurry mutates production, population and treasury.
// Keep this serializer factual about the already-resolved reason; rejected hurry types remain intentionally unlogged. (ChatGPT-5.6-Sol) -->
static char const* getSASGameRecordAIHurryReason(SASGameRecordAIHurryReason eReason)
{
	switch (eReason)
	{
	case SAS_AI_HURRY_FORCED_PANIC: return "FORCED_PANIC";
	case SAS_AI_HURRY_UNHAPPINESS_RELIEF: return "UNHAPPINESS_RELIEF";
	case SAS_AI_HURRY_FOOD_LOSS_RELIEF: return "FOOD_LOSS_RELIEF";
	case SAS_AI_HURRY_UNIT_VALUE: return "UNIT_VALUE";
	case SAS_AI_HURRY_BUILDING_VALUE: return "BUILDING_VALUE";
	default: return "UNKNOWN";
	}
}

void logSASGameRecordAIHurryDecision(CvCity const& kCity, HurryTypes eHurry, SASGameRecordAIHurryReason eReason, UnitAITypes eUnitAI, int iHappyBalance, int iHappyDiff, int iFoodDifference, int iPopCost, int iGoldCost, int iDecisionValue, int iOverflowValue, int iDecisionMargin)
{
	logSASGameRecord("GAME_RECORD_AI_HURRY_DECISION turn=%d player=%d cityId=%d city=%S ownerHuman=%d productionAutomated=%d reason=%s hurry=%s targetKind=%s target=%s unitAI=%s happyBalance=%d happyDiff=%d foodDifference=%d subjectivePopCost=%d subjectiveGoldCost=%d decisionValue=%d overflowValue=%d decisionMargin=%d",
		GC.getGame().getGameTurn(), kCity.getOwner(), kCity.getID(), getSASGameRecordQuotedCityName(&kCity).GetCString(), kCity.isHuman() ? 1 : 0,
		kCity.isProductionAutomated() ? 1 : 0, getSASGameRecordAIHurryReason(eReason), GC.getInfo(eHurry).getType(),
		getSASGameRecordCityProductionKind(kCity), getSASGameRecordCityProductionType(kCity), getSASGameRecordUnitAIType(eUnitAI),
		iHappyBalance, iHappyDiff, iFoodDifference, iPopCost, iGoldCost, iDecisionValue, iOverflowValue, iDecisionMargin);
}

void logSASGameRecordCityHurry(CvCity const* pCity, HurryTypes eHurry, int iProductionBefore, int iProductionAdded, int iGoldCost, int iPopulationCost, int iHurryAngerAdded, int iGoldBefore, int iPopulationBefore, int iHurryAngerBefore)
{
	if (pCity == NULL || eHurry == NO_HURRY)
		return;
	PlayerTypes const ePlayer = pCity->getOwner();
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=CITY_HURRIED player=%d cityId=%d city=%S x=%d y=%d hurry=%s targetKind=%s target=%s productionBefore=%d productionNeeded=%d productionAdded=%d productionAfter=%d goldCost=%d goldBefore=%d goldAfter=%d populationCost=%d populationBefore=%d populationAfter=%d hurryAngerAdded=%d hurryAngerBefore=%d hurryAngerAfter=%d",
		GC.getGame().getGameTurn(), ePlayer, pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(), pCity->getX(),
		pCity->getY(), GC.getInfo(eHurry).getType(), getSASGameRecordCityProductionKind(*pCity), getSASGameRecordCityProductionType(*pCity),
		iProductionBefore, getSASGameRecordCityProductionNeeded(*pCity), iProductionAdded, pCity->getProduction(), iGoldCost, iGoldBefore,
		kPlayer.getGold(), iPopulationCost, iPopulationBefore, pCity->getPopulation(), iHurryAngerAdded, iHurryAngerBefore,
		pCity->getHurryAngerTimer());
}

void logSASGameRecordPillage(CvUnit const* pUnit, SASGameRecordPlotState const& kOldPlotState, PlayerTypes eVictimPlayer, int iGoldGained)
{
	if (pUnit == NULL)
		return;
	CvPlot const& kPlot = pUnit->getPlot();
	CvCity const* pWorkingCity = kPlot.getWorkingCity();
	char const* szStructure = (kOldPlotState.eRoute != kPlot.getRouteType() ? "ROUTE" :
			(kOldPlotState.eImprovement != kPlot.getImprovementType() ? "IMPROVEMENT" : "-"));
	TeamTypes const eVictimTeam = (eVictimPlayer == NO_PLAYER ? NO_TEAM : GET_PLAYER(eVictimPlayer).getTeam());
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=UNIT_PILLAGE player=%d team=%d unitId=%d unit=%s unitAI=%s x=%d y=%d victimPlayer=%d victimTeam=%d structure=%s improvementOld=%s improvementNew=%s routeOld=%s routeNew=%s bonus=%s workingCityId=%d workingCity=%S goldGained=%d hiddenNationality=%d alwaysHostile=%d",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getTeam(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()),
		getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()), kPlot.getX(), kPlot.getY(), eVictimPlayer, eVictimTeam, szStructure,
		getSASGameRecordImprovementType(kOldPlotState.eImprovement), getSASGameRecordImprovementType(kPlot.getImprovementType()),
		getSASGameRecordRouteType(kOldPlotState.eRoute), getSASGameRecordRouteType(kPlot.getRouteType()),
		getSASGameRecordBonusType(kOldPlotState.eBonus), pWorkingCity == NULL ? -1 : pWorkingCity->getID(),
		getSASGameRecordQuotedCityName(pWorkingCity).GetCString(), iGoldGained, pUnit->getUnitInfo().isHiddenNationality() ? 1 : 0,
		pUnit->isAlwaysHostile(kPlot) ? 1 : 0);
}

static int getSASGameRecordBlockadeContextIndex(PlayerTypes ePlayer, int iUnitId)
{
	for (size_t iI = 0; iI < g_aSASGameRecordBlockades.size(); iI++)
	{
		if (g_aSASGameRecordBlockades[iI].ePlayer == ePlayer && g_aSASGameRecordBlockades[iI].iUnitId == iUnitId)
			return (int)iI;
	}
	return -1;
}

static bool hasSASGameRecordCityReference(std::vector<std::pair<PlayerTypes,int> > const& aCities, PlayerTypes ePlayer, int iCityId)
{
	for (size_t iI = 0; iI < aCities.size(); iI++)
	{
		if (aCities[iI].first == ePlayer && aCities[iI].second == iCityId)
			return true;
	}
	return false;
}

static void captureSASGameRecordBlockadeContext(CvUnit const& kUnit, SASGameRecordBlockadeContext& kContext, bool bCheckCanPlunder)
{
	kContext.ePlayer = kUnit.getOwner();
	kContext.iUnitId = kUnit.getID();
	kContext.iStartTurn = GC.getGame().getGameTurn();
	kContext.iStartElapsedTurn = GC.getGame().getElapsedGameTurns();
	kContext.iStartX = kUnit.getX();
	kContext.iStartY = kUnit.getY();
	kContext.iRangePlots = 0;
	kContext.iAffectedTeams = 0;
	kContext.iAffectedCities = 0;
	kContext.szRangePlots.clear();
	kContext.szAffectedTeams.clear();
	kContext.szAffectedCities.clear();
	kContext.iPlunderEvents = 0;
	kContext.iGoldPlundered = 0;
	kContext.aPlunderedCities.clear();

	std::vector<CvPlot*> apRange;
	// <!-- custom: START mirrors updatePlunder's legal range exactly.
	// A fallback END without recorder start context can request the physical range with legality disabled because the unit may be ending precisely after becoming unable to plunder. (ChatGPT-5.6-Sol) -->
	kUnit.blockadeRange(apRange, 0, bCheckCanPlunder);
	kContext.iRangePlots = (int)apRange.size();
	for (size_t iI = 0; iI < apRange.size(); iI++)
	{
		CvString szItem;
		szItem.Format(kContext.szRangePlots.empty() ? "%d,%d" : ";%d,%d", apRange[iI]->getX(), apRange[iI]->getY());
		kContext.szRangePlots += szItem;
	}

	bool abAffectedTeams[MAX_TEAMS];
	for (int iTeam = 0; iTeam < MAX_TEAMS; iTeam++)
		abAffectedTeams[iTeam] = false;
	// <!-- custom: Mirror CvUnit::updatePlunder's exact team iterator/admission test so the logged team/city set describes teams this unit actually contributes blockade counts against, including hidden-nationality behavior. (ChatGPT-5.6-Sol) -->
	for (TeamIter<ALIVE,KNOWN_POTENTIAL_ENEMY_OF> it(kUnit.getTeam()); it.hasNext(); ++it)
	{
		CvTeam const& kTeam = *it;
		if (!kUnit.isEnemy(kTeam.getID()))
			continue;
		abAffectedTeams[kTeam.getID()] = true;
		CvString szItem;
		szItem.Format(kContext.szAffectedTeams.empty() ? "%d" : ",%d", kTeam.getID());
		kContext.szAffectedTeams += szItem;
		kContext.iAffectedTeams++;
	}

	std::vector<std::pair<PlayerTypes,int> > aCities;
	for (size_t iI = 0; iI < apRange.size(); iI++)
	{
		FOR_EACH_ADJ_PLOT(*apRange[iI])
		{
			CvCity const* pCity = pAdj->getPlotCity();
			if (pCity == NULL || pCity->getTeam() < 0 || pCity->getTeam() >= MAX_TEAMS || !abAffectedTeams[pCity->getTeam()] ||
				hasSASGameRecordCityReference(aCities, pCity->getOwner(), pCity->getID()))
			{
				continue;
			}
			aCities.push_back(std::make_pair(pCity->getOwner(), pCity->getID()));
			CvString szItem;
			szItem.Format(kContext.szAffectedCities.empty() ? "P%d:C%d@%d,%d" : ";P%d:C%d@%d,%d",
					pCity->getOwner(), pCity->getID(), pCity->getX(), pCity->getY());
			kContext.szAffectedCities += szItem;
		}
	}
	kContext.iAffectedCities = (int)aCities.size();
}

void logSASGameRecordBlockadeChanged(CvUnit const* pUnit, bool bStarting)
{
	if (pUnit == NULL)
		return;
	int const iExisting = getSASGameRecordBlockadeContextIndex(pUnit->getOwner(), pUnit->getID());
	if (bStarting)
	{
		if (iExisting >= 0)
			g_aSASGameRecordBlockades.erase(g_aSASGameRecordBlockades.begin() + iExisting);
		SASGameRecordBlockadeContext kContext;
		captureSASGameRecordBlockadeContext(*pUnit, kContext, true);
		g_aSASGameRecordBlockades.push_back(kContext);
		logSASGameRecord("GAME_RECORD_ACTION turn=%d type=NAVAL_BLOCKADE_STARTED player=%d team=%d unitId=%d unit=%s unitAI=%s x=%d y=%d rangePlots=%d plots=%s affectedTeams=%d teams=%s affectedCities=%d cities=%s canPlunder=%d hiddenNationality=%d alwaysHostile=%d",
			GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getTeam(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()),
			getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()), pUnit->getX(), pUnit->getY(), kContext.iRangePlots,
			kContext.szRangePlots.empty() ? "-" : kContext.szRangePlots.GetCString(), kContext.iAffectedTeams,
			kContext.szAffectedTeams.empty() ? "-" : kContext.szAffectedTeams.GetCString(), kContext.iAffectedCities,
			kContext.szAffectedCities.empty() ? "-" : kContext.szAffectedCities.GetCString(), pUnit->canPlunder(pUnit->getPlot()) ? 1 : 0,
			pUnit->getUnitInfo().isHiddenNationality() ? 1 : 0, pUnit->isAlwaysHostile(pUnit->getPlot()) ? 1 : 0);
		return;
	}

	SASGameRecordBlockadeContext kContext;
	bool const bStartKnown = (iExisting >= 0);
	if (bStartKnown)
		kContext = g_aSASGameRecordBlockades[iExisting];
	else
	{
		captureSASGameRecordBlockadeContext(*pUnit, kContext, false);
		kContext.iStartTurn = -1;
		kContext.iStartElapsedTurn = -1;
		kContext.iStartX = -1;
		kContext.iStartY = -1;
	}
	int const iDurationTurns = (bStartKnown ? GC.getGame().getGameTurn() - kContext.iStartTurn : -1);
	int const iDurationElapsedTurns = (bStartKnown ? GC.getGame().getElapsedGameTurns() - kContext.iStartElapsedTurn : -1);
	SASGameRecordBlockadeContext kEndContext;
	captureSASGameRecordBlockadeContext(*pUnit, kEndContext, false);
	bool const bScopeChanged = (bStartKnown &&
			(kContext.szAffectedTeams != kEndContext.szAffectedTeams || kContext.szAffectedCities != kEndContext.szAffectedCities));
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=NAVAL_BLOCKADE_ENDED player=%d team=%d unitId=%d unit=%s unitAI=%s startKnown=%d rangeSource=%s startTurn=%d startX=%d startY=%d endX=%d endY=%d durationTurns=%d durationElapsedTurns=%d rangePlots=%d plots=%s affectedTeams=%d teams=%s affectedCities=%d cities=%s endAffectedTeams=%d endTeams=%s endAffectedCities=%d endCities=%s scopeChanged=%d plunderEvents=%d goldPlundered=%d uniquePlunderedCities=%d canPlunderAtEnd=%d",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getTeam(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()),
		getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()), bStartKnown ? 1 : 0, bStartKnown ? "START" : "END_FALLBACK",
		kContext.iStartTurn, kContext.iStartX, kContext.iStartY, pUnit->getX(), pUnit->getY(), iDurationTurns, iDurationElapsedTurns,
		kContext.iRangePlots, kContext.szRangePlots.empty() ? "-" : kContext.szRangePlots.GetCString(), kContext.iAffectedTeams,
		kContext.szAffectedTeams.empty() ? "-" : kContext.szAffectedTeams.GetCString(), kContext.iAffectedCities,
		kContext.szAffectedCities.empty() ? "-" : kContext.szAffectedCities.GetCString(), kEndContext.iAffectedTeams,
		kEndContext.szAffectedTeams.empty() ? "-" : kEndContext.szAffectedTeams.GetCString(), kEndContext.iAffectedCities,
		kEndContext.szAffectedCities.empty() ? "-" : kEndContext.szAffectedCities.GetCString(), bScopeChanged ? 1 : 0,
		kContext.iPlunderEvents, kContext.iGoldPlundered, (int)kContext.aPlunderedCities.size(), pUnit->canPlunder(pUnit->getPlot()) ? 1 : 0);
	if (iExisting >= 0)
		g_aSASGameRecordBlockades.erase(g_aSASGameRecordBlockades.begin() + iExisting);
}

void logSASGameRecordBlockadePlunder(CvUnit const* pUnit, CvCity const* pCity, int iGold, int iTradeRoutes, int iProfitPerRoute)
{
	if (pUnit == NULL || pCity == NULL || iGold <= 0)
		return;
	int const iContext = getSASGameRecordBlockadeContextIndex(pUnit->getOwner(), pUnit->getID());
	int iStartTurn = -1;
	int iAgeTurns = -1;
	if (iContext >= 0)
	{
		SASGameRecordBlockadeContext& kContext = g_aSASGameRecordBlockades[iContext];
		kContext.iPlunderEvents++;
		kContext.iGoldPlundered += iGold;
		if (!hasSASGameRecordCityReference(kContext.aPlunderedCities, pCity->getOwner(), pCity->getID()))
			kContext.aPlunderedCities.push_back(std::make_pair(pCity->getOwner(), pCity->getID()));
		iStartTurn = kContext.iStartTurn;
		iAgeTurns = GC.getGame().getGameTurn() - kContext.iStartTurn;
	}
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=NAVAL_BLOCKADE_PLUNDER player=%d team=%d unitId=%d unit=%s unitAI=%s unitX=%d unitY=%d victimPlayer=%d victimTeam=%d cityId=%d city=%S cityX=%d cityY=%d gold=%d tradeRoutes=%d profitPerRoute=%d blockadeStartKnown=%d blockadeStartTurn=%d blockadeAgeTurns=%d",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getTeam(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()),
		getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()), pUnit->getX(), pUnit->getY(), pCity->getOwner(), pCity->getTeam(),
		pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(), pCity->getX(), pCity->getY(), iGold, iTradeRoutes,
		iProfitPerRoute, iContext >= 0 ? 1 : 0, iStartTurn, iAgeTurns);
}

void logSASGameRecordUnitGifted(CvUnit const* pUnit, PlayerTypes eGiftingPlayer, CvPlot const* pPlotLocation)
{
	if (pUnit == NULL || eGiftingPlayer < 0 || eGiftingPlayer >= MAX_PLAYERS)
		return;
	PlayerTypes const eReceiver = pUnit->getOwner();
	TeamTypes const eGiverTeam = GET_PLAYER(eGiftingPlayer).getTeam();
	TeamTypes const eReceiverTeam = (eReceiver == NO_PLAYER ? NO_TEAM : GET_PLAYER(eReceiver).getTeam());
	CvPlot const* pPlot = (pPlotLocation == NULL ? pUnit->plot() : pPlotLocation);
	int iPromotions = 0;
	FOR_EACH_ENUM(Promotion)
	{
		if (pUnit->isHasPromotion(eLoopPromotion))
			iPromotions++;
	}
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=UNIT_GIFTED giverPlayer=%d giverTeam=%d receiverPlayer=%d receiverTeam=%d unitId=%d unit=%s unitAI=%s x=%d y=%d area=%d experience=%d level=%d promotions=%d damage=%d productionNeeded=%d canCombat=%d cargo=%d transportId=%d",
		GC.getGame().getGameTurn(), eGiftingPlayer, eGiverTeam, eReceiver, eReceiverTeam, pUnit->getID(),
		getSASGameRecordUnitType(pUnit->getUnitType()), getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()),
		pPlot == NULL ? -1 : pPlot->getX(), pPlot == NULL ? -1 : pPlot->getY(), pPlot == NULL ? -1 : pPlot->getArea().getID(),
		pUnit->getExperience(), pUnit->getLevel(), iPromotions, pUnit->getDamage(),
		eReceiver == NO_PLAYER ? -1 : GET_PLAYER(eReceiver).getProductionNeeded(pUnit->getUnitType()), pUnit->canCombat() ? 1 : 0,
		pUnit->isCargo() ? 1 : 0, pUnit->getTransportUnit() == NULL ? -1 : pUnit->getTransportUnit()->getID());
}

void logSASGameRecordReligionChanged(ReligionTypes eReligion, PlayerTypes ePlayer, CvCity const* pCity, bool bAdded)
{
	if (eReligion == NO_RELIGION || pCity == NULL)
		return;
	CvGame const& kGame = GC.getGame();
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=%s player=%d team=%d cityId=%d city=%S x=%d y=%d religion=%s holyCity=%d effectiveStateReligion=%s religionsInCity=%d",
		kGame.getGameTurn(), bAdded ? "RELIGION_SPREAD" : "RELIGION_REMOVED", ePlayer, kPlayer.getTeam(), pCity->getID(),
		getSASGameRecordQuotedCityName(pCity).GetCString(), pCity->getX(), pCity->getY(), getSASGameRecordReligionType(eReligion),
		kGame.getHolyCity(eReligion) == pCity ? 1 : 0, getSASGameRecordReligionType(kPlayer.getStateReligion()), pCity->getReligionCount());
}

void logSASGameRecordReligionSpreadAttempt(CvUnit const* pUnit, ReligionTypes eReligion, CvCity const* pCity, int iDirectSpreadChance, bool bSuccess, ReligionTypes eDisplacedReligion)
{
	if (pUnit == NULL || eReligion == NO_RELIGION || pCity == NULL)
		return;
	char const* szOutcome = (bSuccess ? (eDisplacedReligion == NO_RELIGION ? "SPREAD" : "SPREAD_AND_DISPLACE") : "FAILED");
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=RELIGION_SPREAD_ATTEMPT player=%d team=%d unitId=%d unit=%s unitAI=%s religion=%s targetPlayer=%d targetTeam=%d cityId=%d city=%S x=%d y=%d directSpreadChance=%d outcome=%s displacedReligion=%s religionsBefore=%d",
			GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getTeam(), pUnit->getID(),
			getSASGameRecordUnitType(pUnit->getUnitType()), getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()), getSASGameRecordReligionType(eReligion),
			pCity->getOwner(), pCity->getTeam(), pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(), pCity->getX(), pCity->getY(),
			iDirectSpreadChance, szOutcome, getSASGameRecordReligionType(eDisplacedReligion), pCity->getReligionCount());
}

void logSASGameRecordCorporationChanged(CorporationTypes eCorporation, PlayerTypes ePlayer, CvCity const* pCity, bool bAdded)
{
	if (eCorporation == NO_CORPORATION || pCity == NULL)
		return;
	CvGame const& kGame = GC.getGame();
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=%s player=%d team=%d cityId=%d city=%S x=%d y=%d corporation=%s headquarters=%d corporationsInCity=%d",
		kGame.getGameTurn(), bAdded ? "CORPORATION_SPREAD" : "CORPORATION_REMOVED", ePlayer, kPlayer.getTeam(), pCity->getID(),
		getSASGameRecordQuotedCityName(pCity).GetCString(), pCity->getX(), pCity->getY(), getSASGameRecordCorporationType(eCorporation),
		kGame.getHeadquarters(eCorporation) == pCity ? 1 : 0, pCity->getCorporationCount());
}

void logSASGameRecordCorporationSpreadAttempt(CvUnit const* pUnit, CorporationTypes eCorporation, CvCity const* pCity, int iSpreadChance, int iGoldCost, int iGoldBefore, bool bSuccess)
{
	if (pUnit == NULL || eCorporation == NO_CORPORATION || pCity == NULL)
		return;
	CvPlayer const& kPlayer = GET_PLAYER(pUnit->getOwner());
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=CORPORATION_SPREAD_ATTEMPT player=%d team=%d unitId=%d unit=%s unitAI=%s corporation=%s targetPlayer=%d targetTeam=%d cityId=%d city=%S x=%d y=%d spreadChance=%d outcome=%s goldCost=%d goldBefore=%d goldAfter=%d corporationsBefore=%d",
			GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getTeam(), pUnit->getID(),
			getSASGameRecordUnitType(pUnit->getUnitType()), getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()), getSASGameRecordCorporationType(eCorporation),
			pCity->getOwner(), pCity->getTeam(), pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(), pCity->getX(), pCity->getY(),
			iSpreadChance, bSuccess ? "SPREAD" : "FAILED", iGoldCost, iGoldBefore, kPlayer.getGold(), pCity->getCorporationCount());
}

void logSASGameRecordCircumnavigated(TeamTypes eTeam, int iFreeSeaMoves, bool bBonusApplied, int iSeaExtraMovesBefore, int iSeaExtraMovesAfter)
{
	if (eTeam == NO_TEAM)
		return;
	CvMap const& kMap = GC.getMap();
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=CIRCUMNAVIGATION_COMPLETED team=%d members=%s wrapX=%d wrapY=%d freeSeaMoves=%d bonusApplied=%d seaExtraMovesBefore=%d seaExtraMovesAfter=%d",
			GC.getGame().getGameTurn(), eTeam, getSASGameRecordTeamMembers(eTeam).GetCString(), kMap.isWrapX() ? 1 : 0, kMap.isWrapY() ? 1 : 0,
			iFreeSeaMoves, bBonusApplied ? 1 : 0, iSeaExtraMovesBefore, iSeaExtraMovesAfter);
}

// <!-- custom: Financial strikes are rare but can begin, force unit disbands and end entirely between periodic snapshots.
// Record each realized strike turn from values already produced by CvPlayer::doGold. (ChatGPT-5.6-Sol) -->
void logSASGameRecordFinancialStrikeTurn(PlayerTypes ePlayer, int iGoldBefore, int iCalculatedGoldRate, int iGoldAfterClamp, int iCumulativeStrikeTurns, int iUnitsBeforeDisband, int iUnitsAfterDisband)
{
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=FINANCIAL_STRIKE_TURN player=%d team=%d goldBefore=%d calculatedGoldRate=%d projectedGoldBeforeClamp=%d goldAfterClamp=%d cumulativeStrikeTurns=%d unitsBeforeDisband=%d unitsAfterDisband=%d unitsDisbanded=%d",
			GC.getGame().getGameTurn(), ePlayer, kPlayer.getTeam(), iGoldBefore, iCalculatedGoldRate, iGoldBefore + iCalculatedGoldRate, iGoldAfterClamp, iCumulativeStrikeTurns,
			iUnitsBeforeDisband, iUnitsAfterDisband, std::max(0, iUnitsBeforeDisband - iUnitsAfterDisband));
}


// <!-- custom: Keep Golden Age/anarchy action rows consistent with the player snapshot by labeling recorder-local observations as logged turns. See KI#379. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordGoldenAge(PlayerTypes ePlayer, bool bStart)
{
	if (ePlayer < 0 || ePlayer >= MAX_PLAYERS)
		return;
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=%s player=%d goldenAgeTurns=%d loggedGoldenAgeTurns=%d anarchyTurns=%d loggedAnarchyTurns=%d",
			GC.getGame().getGameTurn(), bStart ? "GOLDEN_AGE_STARTED" : "GOLDEN_AGE_ENDED", ePlayer, kPlayer.getGoldenAgeTurns(),
			g_aiSASGameRecordLoggedGoldenAgeTurns[ePlayer], kPlayer.getAnarchyTurns(), g_aiSASGameRecordLoggedAnarchyTurns[ePlayer]);
}

void logSASGameRecordGoldenAgeTurnsChanged(PlayerTypes ePlayer, int iChange, int iOldGoldenAgeTurns, int iNewGoldenAgeTurns)
{
	if (ePlayer < 0 || ePlayer >= MAX_PLAYERS)
		return;
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=GOLDEN_AGE_TURNS_CHANGED player=%d change=%+d oldGoldenAgeTurns=%d newGoldenAgeTurns=%d goldenAgeTurns=%d loggedGoldenAgeTurns=%d anarchyTurns=%d loggedAnarchyTurns=%d",
			GC.getGame().getGameTurn(), ePlayer, iChange, iOldGoldenAgeTurns, iNewGoldenAgeTurns, kPlayer.getGoldenAgeTurns(),
			g_aiSASGameRecordLoggedGoldenAgeTurns[ePlayer], kPlayer.getAnarchyTurns(), g_aiSASGameRecordLoggedAnarchyTurns[ePlayer]);
}

void logSASGameRecordAnarchy(PlayerTypes ePlayer, bool bStart)
{
	if (ePlayer < 0 || ePlayer >= MAX_PLAYERS)
		return;
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=%s player=%d anarchyTurns=%d loggedAnarchyTurns=%d goldenAgeTurns=%d loggedGoldenAgeTurns=%d revolutionTimer=%d conversionTimer=%d",
		GC.getGame().getGameTurn(), bStart ? "ANARCHY_STARTED" : "ANARCHY_ENDED", ePlayer, kPlayer.getAnarchyTurns(),
		g_aiSASGameRecordLoggedAnarchyTurns[ePlayer], kPlayer.getGoldenAgeTurns(), g_aiSASGameRecordLoggedGoldenAgeTurns[ePlayer],
		kPlayer.getRevolutionTimer(), kPlayer.getConversionTimer());
}

void logSASGameRecordCivicChanged(PlayerTypes ePlayer, CivicOptionTypes eCivicOption, CivicTypes eOldCivic, CivicTypes eNewCivic, ReligionTypes eOldEffectiveStateReligion, ReligionTypes eNewEffectiveStateReligion)
{
	if (ePlayer < 0 || ePlayer >= MAX_PLAYERS || eCivicOption == NO_CIVICOPTION)
		return;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=CIVIC_CHANGED player=%d civicOption=%s oldCivic=%s newCivic=%s oldEffectiveStateReligion=%s newEffectiveStateReligion=%s anarchyTurns=%d",
		GC.getGame().getGameTurn(), ePlayer, GC.getInfo(eCivicOption).getType(), getSASGameRecordCivicType(eOldCivic), getSASGameRecordCivicType(eNewCivic),
		getSASGameRecordReligionType(eOldEffectiveStateReligion), getSASGameRecordReligionType(eNewEffectiveStateReligion), GET_PLAYER(ePlayer).getAnarchyTurns());
}

void logSASGameRecordLastStateReligionChanged(PlayerTypes ePlayer, ReligionTypes eOldReligion, ReligionTypes eNewReligion)
{
	if (ePlayer < 0 || ePlayer >= MAX_PLAYERS)
		return;
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	ReligionTypes const eOldEffectiveReligion = (kPlayer.isStateReligion() ? eOldReligion : NO_RELIGION);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=LAST_STATE_RELIGION_CHANGED player=%d oldLastStateReligion=%s newLastStateReligion=%s oldEffectiveStateReligion=%s newEffectiveStateReligion=%s anarchyTurns=%d",
		GC.getGame().getGameTurn(), ePlayer, getSASGameRecordReligionType(eOldReligion), getSASGameRecordReligionType(eNewReligion),
		getSASGameRecordReligionType(eOldEffectiveReligion), getSASGameRecordReligionType(kPlayer.getStateReligion()),
		kPlayer.getAnarchyTurns());
}

void logSASGameRecordBuildingCompletedByProduction(CvCity const* pCity, BuildingTypes eBuilding, int iRawModifiedOverflow, int iUnmodifiedOverflow, int iKeptOverflow, int iLostProduction, int iUnusedOverflowCapacity, int iOverflowGold)
{
	if (pCity == NULL || eBuilding == NO_BUILDING)
		return;
	PlayerTypes const ePlayer = pCity->getOwner();
	SASGameRecordPlayerFlow& kFlow = g_akSASGameRecordPlayerFlow[ePlayer];
	int const iProductionNeeded = GET_PLAYER(ePlayer).getProductionNeeded(eBuilding);
	kFlow.iBuildingsCompleted++;
	kFlow.iBuildingProductionNeeded += iProductionNeeded;
	kFlow.aiBuildingTypes[eBuilding]++;
	// <!-- custom: At level 3, also keep production-completed limited buildings here: WONDER_BUILT remains the rare strategic marker, while BUILDING_COMPLETED now owns the exact production/overflow result like units and projects. (ChatGPT-5.6-Sol) -->
	if (gGameRecordLogLevel >= 3)
		logSASGameRecord("GAME_RECORD_ACTION turn=%d type=BUILDING_COMPLETED player=%d cityId=%d city=%S building=%s productionNeeded=%d rawModifiedOverflow=%d unmodifiedOverflow=%d keptOverflow=%d lostProduction=%d unusedOverflowCapacity=%d overflowGold=%d",
			GC.getGame().getGameTurn(), ePlayer, pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(), getSASGameRecordBuildingType(eBuilding), iProductionNeeded,
			iRawModifiedOverflow, iUnmodifiedOverflow, iKeptOverflow, iLostProduction, iUnusedOverflowCapacity, iOverflowGold);
}

void logSASGameRecordBuildingBuilt(CvCity const* pCity, BuildingTypes eBuilding)
{
	if (pCity == NULL || eBuilding == NO_BUILDING || !GC.getInfo(eBuilding).isLimited())
		return;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=WONDER_BUILT player=%d cityId=%d city=%S building=%s",
		GC.getGame().getGameTurn(), pCity->getOwner(), pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(),
		getSASGameRecordBuildingType(eBuilding));
}

void logSASGameRecordProjectBuilt(CvCity const* pCity, ProjectTypes eProject, int iRawModifiedOverflow, int iUnmodifiedOverflow, int iKeptOverflow, int iLostProduction, int iUnusedOverflowCapacity, int iOverflowGold)
{
	if (pCity == NULL || eProject == NO_PROJECT)
		return;
	PlayerTypes const ePlayer = pCity->getOwner();
	SASGameRecordPlayerFlow& kFlow = g_akSASGameRecordPlayerFlow[ePlayer];
	int const iProductionNeeded = GET_PLAYER(ePlayer).getProductionNeeded(eProject);
	kFlow.iProjectsCompleted++;
	kFlow.iProjectProductionNeeded += iProductionNeeded;
	kFlow.aiProjectTypes[eProject]++;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=PROJECT_BUILT player=%d cityId=%d city=%S project=%s productionNeeded=%d rawModifiedOverflow=%d unmodifiedOverflow=%d keptOverflow=%d lostProduction=%d unusedOverflowCapacity=%d overflowGold=%d",
		GC.getGame().getGameTurn(), ePlayer, pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(), getSASGameRecordProjectType(eProject), iProductionNeeded,
		iRawModifiedOverflow, iUnmodifiedOverflow, iKeptOverflow, iLostProduction, iUnusedOverflowCapacity, iOverflowGold);
}

void logSASGameRecordProductionOverflow(CvCity const* pCity, int iRawModifiedOverflow, int iUnmodifiedOverflow, int iKeptOverflow, int iLostProduction, int iUnusedCapacity, int iGold)
{
	if (pCity == NULL)
		return;
	SASGameRecordPlayerFlow& kFlow = g_akSASGameRecordPlayerFlow[pCity->getOwner()];
	kFlow.iOverflowActions++;
	kFlow.iRawModifiedOverflow += iRawModifiedOverflow;
	kFlow.iUnmodifiedOverflow += iUnmodifiedOverflow;
	kFlow.iKeptOverflow += iKeptOverflow;
	kFlow.iLostProduction += iLostProduction;
	kFlow.iUnusedOverflowCapacity += iUnusedCapacity;
	kFlow.iOverflowGold += iGold;
	// <!-- custom: Level 3 already carries these exact values on the corresponding production-completion row, so avoid a duplicate action.
	// At level 2, PROJECT_BUILT already owns its exact overflow. Otherwise keep strategically exceptional loss/gold and every Barbarian overflow because ordinary unit/building completion rows and Barbarian production-flow summaries are unavailable there. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (gGameRecordLogLevel == 2 && !pCity->isProductionProject() && (pCity->isBarbarian() || iLostProduction > 0 || iGold > 0))
		logSASGameRecord("GAME_RECORD_ACTION turn=%d type=PRODUCTION_OVERFLOW player=%d cityId=%d city=%S productionKind=%s production=%s rawModifiedOverflow=%d unmodifiedOverflow=%d keptOverflow=%d lostProduction=%d unusedOverflowCapacity=%d gold=%d",
			GC.getGame().getGameTurn(), pCity->getOwner(), pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(),
			getSASGameRecordCityProductionKind(*pCity), getSASGameRecordCityProductionType(*pCity), iRawModifiedOverflow,
			iUnmodifiedOverflow, iKeptOverflow, iLostProduction, iUnusedCapacity, iGold);
}

void logSASGameRecordProductionFailed(CvCity const* pCity, int iOrderData, bool bProject, int iInvestedProduction, int iGold)
{
	if (pCity == NULL)
		return;
	SASGameRecordPlayerFlow& kFlow = g_akSASGameRecordPlayerFlow[pCity->getOwner()];
	kFlow.iFailedInvestedProduction += iInvestedProduction;
	kFlow.iFailGold += iGold;
	char const* szProduction = (bProject ? GC.getInfo((ProjectTypes)iOrderData).getType() : getSASGameRecordBuildingType((BuildingTypes)iOrderData));
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=PRODUCTION_FAILED_TO_GOLD player=%d cityId=%d city=%S productionKind=%s production=%s investedProduction=%d gold=%d",
		GC.getGame().getGameTurn(), pCity->getOwner(), pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(),
		bProject ? "PROJECT" : "BUILDING", szProduction, iInvestedProduction, iGold);
}

void logSASGameRecordProductionDecay(CvCity const* pCity, OrderTypes eOrder, int iData1, int iBefore, int iAfter, int iInactiveTurns)
{
	if (pCity == NULL || iAfter >= iBefore)
		return;
	int const iLost = iBefore - iAfter;
	SASGameRecordPlayerFlow& kFlow = g_akSASGameRecordPlayerFlow[pCity->getOwner()];
	kFlow.iProductionDecayActions++;
	kFlow.iProductionDecayLost += iLost;
	if (gGameRecordLogLevel >= 3)
	{
		logSASGameRecord("GAME_RECORD_ACTION turn=%d type=PRODUCTION_DECAY player=%d cityId=%d city=%S productionKind=%s production=%s storedBefore=%d storedAfter=%d lost=%d accumulatedInactiveTurns=%d",
			GC.getGame().getGameTurn(), pCity->getOwner(), pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(),
			getSASGameRecordProductionKind(eOrder, iData1), getSASGameRecordProductionType(eOrder, iData1), iBefore, iAfter, iLost,
			iInactiveTurns);
	}
}

void logSASGameRecordProductionInvalidated(CvCity const* pCity, OrderTypes eOrder, int iData1, int iStoredLost, bool bActiveTarget, bool bQueued)
{
	if (pCity == NULL || iStoredLost <= 0)
		return;
	SASGameRecordPlayerFlow& kFlow = g_akSASGameRecordPlayerFlow[pCity->getOwner()];
	kFlow.iProductionInvalidatedActions++;
	kFlow.iProductionInvalidatedLost += iStoredLost;
	char const* szReason = (eOrder == ORDER_TRAIN ? "MAXED_UNIT_CLASS" : (eOrder == ORDER_CONSTRUCT ? "MAXED_BUILDING_CLASS" : (eOrder == ORDER_CREATE ? "MAXED_PROJECT" : "UNKNOWN")));
	// <!-- custom: This is actual stored production erased by inherited maxed-class/project cleanup, not strategic target switching.
	// Emit the rare loss at level 2 so parked production cannot disappear between snapshots without provenance. (ChatGPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=PRODUCTION_INVALIDATED player=%d cityId=%d city=%S productionKind=%s production=%s reason=%s storedLost=%d activeTarget=%d queued=%d",
		GC.getGame().getGameTurn(), pCity->getOwner(), pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(),
		getSASGameRecordProductionKind(eOrder, iData1), getSASGameRecordProductionType(eOrder, iData1), szReason, iStoredLost,
		bActiveTarget ? 1 : 0, bQueued ? 1 : 0);
}

void logSASGameRecordProductionUpgraded(CvCity const* pCity, UnitTypes eOldUnit, UnitTypes eNewUnit, int iProductionTransferred, int iDestinationProductionBefore)
{
	if (pCity == NULL || (iProductionTransferred <= 0 && iDestinationProductionBefore <= 0))
		return;
	SASGameRecordPlayerFlow& kFlow = g_akSASGameRecordPlayerFlow[pCity->getOwner()];
	if (iProductionTransferred > 0)
	{
		kFlow.iProductionUpgradeTransfers++;
		kFlow.iProductionUpgradeTransferred += iProductionTransferred;
	}
	// <!-- custom: CvCity::upgradeProduction assigns rather than adds at the destination.
	// Any pre-existing destination production is therefore overwritten; preserve that separately as a possible mechanical loss instead of misclassifying it as target churn. (ChatGPT-5.6-Sol) -->
	if (iDestinationProductionBefore > 0)
		kFlow.iProductionUpgradeOverwriteActions++;
	kFlow.iProductionUpgradeOverwritten += std::max(0, iDestinationProductionBefore);
	if (gGameRecordLogLevel >= 3 || iDestinationProductionBefore > 0)
	{
		logSASGameRecord("GAME_RECORD_ACTION turn=%d type=PRODUCTION_UPGRADED player=%d cityId=%d city=%S oldUnit=%s newUnit=%s productionTransferred=%d newProductionBefore=%d newProductionAfter=%d overwrittenDestinationProduction=%d",
			GC.getGame().getGameTurn(), pCity->getOwner(), pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(),
			getSASGameRecordUnitType(eOldUnit), getSASGameRecordUnitType(eNewUnit), iProductionTransferred, iDestinationProductionBefore,
			iProductionTransferred, std::max(0, iDestinationProductionBefore));
	}
}

void logSASGameRecordVictoryLaunched(PlayerTypes ePlayer, VictoryTypes eVictory)
{
	if (ePlayer < 0 || ePlayer >= MAX_PLAYERS || eVictory == NO_VICTORY)
		return;
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	CvTeam const& kTeam = GET_TEAM(kPlayer.getTeam());
	int iPartsBuilt = 0;
	int iPartsMinimum = 0;
	int iPartsMaximum = 0;
	bool bMinimumComplete = false;
	CvString szProjectParts;
	bool const bProjectVictory = getSASGameRecordVictoryProjectState(kPlayer.getTeam(), eVictory, iPartsBuilt, iPartsMinimum, iPartsMaximum, bMinimumComplete, szProjectParts);
	int const iCountdown = kTeam.getVictoryCountdown(eVictory);
	// <!-- custom: PROJECT_BUILT rows could only imply a spaceship launch.
	// Record the actual launch and its exact arrival state so a Space victory no longer has to be reconstructed from component timing. (GPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=SPACESHIP_LAUNCHED player=%d team=%d victory=%s countdown=%d arrivalTurn=%d travelTurns=%d launchSuccessPercent=%d partsBuilt=%d partsMinimum=%d partsMaximum=%d projectParts=%s",
			GC.getGame().getGameTurn(), ePlayer, kPlayer.getTeam(), GC.getInfo(eVictory).getType(),
			iCountdown, iCountdown < 0 ? -1 : GC.getGame().getGameTurn() + iCountdown,
			bProjectVictory && bMinimumComplete ? kTeam.getVictoryDelay(eVictory) : -1,
			kTeam.getLaunchSuccessRate(eVictory), iPartsBuilt, iPartsMinimum, iPartsMaximum, bProjectVictory ? szProjectParts.GetCString() : "-");
}

static void logSASGameRecordVictoryProgressRemoved(TeamTypes eTeam, VictoryTypes eVictory, char const* szAction, char const* szCause, int iLaunchSuccessPercent, CvCity const* pCapital)
{
	CvTeam const& kTeam = GET_TEAM(eTeam);
	int iPartsBuilt = 0;
	int iPartsMinimum = 0;
	int iPartsMaximum = 0;
	bool bMinimumComplete = false;
	CvString szProjectParts;
	bool const bProjectVictory = getSASGameRecordVictoryProjectState(eTeam, eVictory, iPartsBuilt, iPartsMinimum, iPartsMaximum, bMinimumComplete, szProjectParts);
	int const iCountdown = kTeam.getVictoryCountdown(eVictory);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=%s team=%d victory=%s cause=%s countdown=%d arrivalTurn=%d launchSuccessPercent=%d capitalPlayer=%d capitalCityId=%d capital=%S capitalX=%d capitalY=%d projectVictory=%d partsBuilt=%d partsMinimum=%d partsMaximum=%d projectParts=%s",
			GC.getGame().getGameTurn(), szAction, eTeam, getSASGameRecordVictoryType(eVictory), szCause, iCountdown, iCountdown < 0 ? -1 : GC.getGame().getGameTurn() + iCountdown,
			iLaunchSuccessPercent, pCapital == NULL ? NO_PLAYER : pCapital->getOwner(), pCapital == NULL ? -1 : pCapital->getID(),
			getSASGameRecordQuotedCityName(pCapital).GetCString(), pCapital == NULL ? -1 : pCapital->getX(), pCapital == NULL ? -1 : pCapital->getY(),
			bProjectVictory, iPartsBuilt, iPartsMinimum, iPartsMaximum, bProjectVictory ? szProjectParts.GetCString() : "-");
}

void logSASGameRecordVictoryProgressResetForCapital(CvCity const* pCapital)
{
	if (pCapital == NULL || GC.getGame().getGameState() != GAMESTATE_ON)
		return;
	TeamTypes const eTeam = pCapital->getTeam();
	CvTeam const& kTeam = GET_TEAM(eTeam);
	FOR_EACH_ENUM(Victory)
	{
		if (kTeam.getVictoryCountdown(eLoopVictory) >= 0)
			logSASGameRecordVictoryProgressRemoved(eTeam, eLoopVictory, "VICTORY_PROGRESS_RESET", "CAPITAL_LOST", kTeam.getLaunchSuccessRate(eLoopVictory), pCapital);
	}
}

void logSASGameRecordSpaceshipFailed(TeamTypes eTeam, VictoryTypes eVictory, int iLaunchSuccessPercent)
{
	if (eTeam == NO_TEAM || eVictory == NO_VICTORY)
		return;
	// <!-- custom: A failed arrival roll previously erased the countdown and spaceship projects without an explicit event.
	// Preserve the losing launch state immediately before resetVictoryProgress removes it. (GPT-5.6-Sol) -->
	logSASGameRecordVictoryProgressRemoved(eTeam, eVictory, "SPACESHIP_FAILED", "LAUNCH_ROLL_FAILED", iLaunchSuccessPercent, NULL);
}

void logSASGameRecordVassalState(TeamTypes eMaster, TeamTypes eVassal, bool bVassal)
{
	int const iGameTurn = GC.getGame().getGameTurn();
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=%s master=%d vassal=%d", iGameTurn, bVassal ? "VASSALAGE_STARTED" : "VASSALAGE_ENDED", eMaster, eVassal);
	// <!-- custom: Capture the relationship's stamped land baselines immediately after vassalage starts; periodic level-2 team snapshots then show movement toward any revolt threshold. (ChatGPT-5.6-Sol) -->
	if (bVassal && eVassal != NO_TEAM)
		logSASGameRecordVassalStatus(GET_TEAM(eVassal), iGameTurn);
}

void logSASGameRecordVictory(TeamTypes eWinner, VictoryTypes eVictory)
{
	bool const bLogVictoryDetails = (gGameRecordLogLevel >= 2);
	// <!-- custom: Victory can be reported before the ordinary end-turn hook.
	// Flush this turn's buffered map history first so the final snapshot does not precede its last plot changes or map revelation.
	// Reuse the same level-2 gate for final war reconciliation below. (GPT-5.6-Sol + ChatGPT-5.6-Sol) -->
	if (bLogVictoryDetails) flushSASGameRecordTurnChanges(GC.getGame().getGameTurn());
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=VICTORY team=%d victory=%s",
		GC.getGame().getGameTurn(), eWinner, eVictory == NO_VICTORY ? "-" : GC.getInfo(eVictory).getType());
	// <!-- custom: ReplayInfo preserves only the selected player's final and normalized scores.
	// Record both for every civilization once at victory so benchmark review can compare the complete final field without reconstructing Civ4's final-score formula. (GPT-5.6-Sol) -->
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes const ePlayer = (PlayerTypes)iI;
		CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
		if (!kPlayer.isEverAlive())
			continue;
		bool const bWinner = (kPlayer.getTeam() == eWinner);
		logSASGameRecord("GAME_RECORD_FINAL_SCORE turn=%d player=%d team=%d alive=%d winner=%d score=%d normalizedScore=%d",
			GC.getGame().getGameTurn(), ePlayer, kPlayer.getTeam(), kPlayer.isAlive(), bWinner, kPlayer.calculateScore(),
			kPlayer.calculateScore(true, bWinner));
	}
	// <!-- custom: A victory can end the run with wars still active; preserve their observed results without falsely marking them as completed wars. (GPT-5.6-Sol) -->
	if (bLogVictoryDetails)
	{
		reconcileSASGameRecordWars();
		logSASGameRecordOngoingWarSummaries("VICTORY");
	}
	// <!-- custom: Periodic snapshots could stop several turns before victory, leaving every civilization's exact final state unknown.
	// Force one complete marked snapshot now; the ordinary end-turn hook suppresses a duplicate on the same turn. (GPT-5.6-Sol) -->
	logSASGameRecordSnapshot(GC.getGame().getGameTurn(), "victory");
}

void logSASGameRecordPlayerEliminated(PlayerTypes ePlayer)
{
	if (ePlayer < 0 || ePlayer >= MAX_PLAYERS)
		return;
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=PLAYER_ELIMINATED player=%d team=%d civ=%s leader=%s cities=%d units=%d score=%d power=%d playersAlive=%d teamsAlive=%d eliminatedPlayers=%s",
		GC.getGame().getGameTurn(), ePlayer, kPlayer.getTeam(),
		kPlayer.getCivilizationType() == NO_CIVILIZATION ? "-" : GC.getInfo(kPlayer.getCivilizationType()).getType(),
		kPlayer.getLeaderType() == NO_LEADER ? "-" : GC.getInfo(kPlayer.getLeaderType()).getType(), kPlayer.getNumCities(),
		kPlayer.getNumUnits(), kPlayer.calculateScore(), kPlayer.getPower(), GC.getGame().countCivPlayersAlive(),
		GC.getGame().countCivTeamsAlive(), getSASGameRecordEliminatedPlayers().GetCString());
	// <!-- custom: If this eliminated the team's last player, close its active war summaries on the exact elimination turn instead of waiting for the next periodic snapshot. (GPT-5.6-Sol) -->
	reconcileSASGameRecordWars();
	logSASGameRecordRunStatus("playerEliminated");
}

void logSASGameRecordPlayerAliveChanged(PlayerTypes ePlayer, bool bRevived)
{
	if (ePlayer < 0 || ePlayer >= MAX_PLAYERS)
		return;
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=%s player=%d team=%d civ=%s leader=%s cities=%d units=%d score=%d power=%d playersAlive=%d teamsAlive=%d playersEverAlive=%d",
		GC.getGame().getGameTurn(), bRevived ? "PLAYER_REVIVED" : "PLAYER_APPEARED", ePlayer, kPlayer.getTeam(),
		kPlayer.getCivilizationType() == NO_CIVILIZATION ? "-" : GC.getInfo(kPlayer.getCivilizationType()).getType(),
		kPlayer.getLeaderType() == NO_LEADER ? "-" : GC.getInfo(kPlayer.getLeaderType()).getType(), kPlayer.getNumCities(),
		kPlayer.getNumUnits(), kPlayer.calculateScore(), kPlayer.getPower(), GC.getGame().countCivPlayersAlive(),
		GC.getGame().countCivTeamsAlive(), GC.getGame().countCivPlayersEverAlive());
	logSASGameRecordRunStatus(bRevived ? "playerRevived" : "playerAppeared");
}

// <!-- custom: Debug mode is game-level state; record only successful transitions, not rejected toggle attempts. (ChatGPT-5.6-Sol) -->
void logSASGameRecordDebugModeChanged(bool bOldDebugMode, bool bNewDebugMode)
{
	if (bOldDebugMode == bNewDebugMode)
		return;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=DEBUG_MODE_CHANGED old=%d new=%d activePlayer=%d",
		GC.getGame().getGameTurn(), bOldDebugMode, bNewDebugMode, GC.getGame().getActivePlayer());
}

void logSASGameRecordAutoPlayChanged(int iOldValue, int iNewValue, bool bChangePlayerStatus, SASAutoPlayEndCause eEndCause)
{
	if (iOldValue == iNewValue)
		return;
	CvGame const& kGame = GC.getGame();
	bool const bStarted = (iOldValue <= 0 && iNewValue > 0);
	bool const bEnded = (iOldValue > 0 && iNewValue <= 0);
	char const* szAction = (bStarted ? "AUTOPLAY_STARTED" : (bEnded ? "AUTOPLAY_ENDED" : "AUTOPLAY_CHANGED"));
	PlayerTypes const eActivePlayer = kGame.getActivePlayer();
	uint const uiAutoPlayTime = getSASMonotonicMilliseconds();
	if (bStarted)
	{
		g_iSASGameRecordAutoPlayRequestId++;
		g_iSASGameRecordAutoPlayRequestedTurns = iNewValue;
		g_iSASGameRecordAutoPlayStartTurn = kGame.getGameTurn();
		g_iSASGameRecordAutoPlayStartElapsedTurn = kGame.getElapsedGameTurns();
		g_eSASGameRecordAutoPlayStartPlayer = eActivePlayer;
		g_uiSASGameRecordAutoPlayStartTime = uiAutoPlayTime;
		g_iSASGameRecordAutoPlayPlayerChanges = 0;
	}
	if (bEnded && eEndCause == SAS_AUTOPLAY_END_UNSPECIFIED)
		eEndCause = (kGame.getWinner() != NO_TEAM ? SAS_AUTOPLAY_END_VICTORY : (eActivePlayer != NO_PLAYER && !GET_PLAYER(eActivePlayer).isAlive() ? SAS_AUTOPLAY_END_ACTIVE_PLAYER_DEFEATED : SAS_AUTOPLAY_END_OTHER));
	int const iCompletedTurns = (!bEnded || g_iSASGameRecordAutoPlayRequestedTurns <= 0 ? 0 : (eEndCause == SAS_AUTOPLAY_END_SCHEDULED ? g_iSASGameRecordAutoPlayRequestedTurns : std::max(0, g_iSASGameRecordAutoPlayRequestedTurns - iOldValue)));
	int const iElapsedGameTurns = (g_iSASGameRecordAutoPlayStartElapsedTurn < 0 ? 0 : std::max(0, kGame.getElapsedGameTurns() - g_iSASGameRecordAutoPlayStartElapsedTurn));
	int const iAutoPlayWallMilliseconds = (g_iSASGameRecordAutoPlayStartTurn < 0 ? -1 : (int)getSASElapsedMilliseconds(g_uiSASGameRecordAutoPlayStartTime, uiAutoPlayTime));
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=%s oldTurnsLeft=%d newTurnsLeft=%d activePlayer=%d changePlayerStatus=%d requestId=%d requestedTurns=%d completedTurns=%d elapsedGameTurns=%d sessionWallMilliseconds=%u autoplayWallMilliseconds=%d startTurn=%d startElapsed=%d startPlayer=%d activePlayerChanges=%d totalActivePlayerChanges=%d endCause=%s",
		kGame.getGameTurn(), szAction, iOldValue, iNewValue, eActivePlayer, bChangePlayerStatus, g_iSASGameRecordAutoPlayRequestId,
		g_iSASGameRecordAutoPlayRequestedTurns, iCompletedTurns, iElapsedGameTurns, getSASGameRecordSessionWallMilliseconds(uiAutoPlayTime),
		iAutoPlayWallMilliseconds, g_iSASGameRecordAutoPlayStartTurn, g_iSASGameRecordAutoPlayStartElapsedTurn,
		g_eSASGameRecordAutoPlayStartPlayer, g_iSASGameRecordAutoPlayPlayerChanges, g_iSASGameRecordTotalActivePlayerChanges,
		getSASAutoPlayEndCause(eEndCause));
	// <!-- custom: Treat only actual autoplay start/end as rare level-3 RNG boundaries, not the ordinary per-turn countdown changes. This isolates the benchmark/autoplay random-consumption window even when it begins or ends partway through a game turn.
	// A benchmark seed entered in AIAutoPlay.py is applied immediately before AUTOPLAY_STARTED, so AUTOPLAY_BEGIN opens from that newly assigned authoritative state. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (g_bSASGameRecordRngTrackingActive && (bStarted || bEnded)) logSASGameRecordRngCheckpoint(kGame.getGameTurn(), bStarted ? SAS_RNG_CHECKPOINT_AUTOPLAY_BEGIN : SAS_RNG_CHECKPOINT_AUTOPLAY_END);
	// <!-- custom: Manual or scheduled autoplay completion is also a useful record boundary even when the game and its wars continue. (GPT-5.6-Sol) -->
	if (gGameRecordLogLevel >= 2 && bEnded)
	{
		reconcileSASGameRecordWars();
		logSASGameRecordOngoingWarSummaries("AUTOPLAY_ENDED");
	}
	logSASGameRecordRunStatus(szAction);
	if (bEnded)
	{
		g_iSASGameRecordAutoPlayRequestedTurns = 0;
		g_iSASGameRecordAutoPlayStartTurn = -1;
		g_iSASGameRecordAutoPlayStartElapsedTurn = -1;
		g_eSASGameRecordAutoPlayStartPlayer = NO_PLAYER;
		g_uiSASGameRecordAutoPlayStartTime = 0;
		g_iSASGameRecordAutoPlayPlayerChanges = 0;
	}
}

void logSASGameRecordActivePlayerChanged(PlayerTypes eOldPlayer, PlayerTypes eNewPlayer)
{
	g_iSASGameRecordTotalActivePlayerChanges++;
	bool const bDuringAutoPlay = (GC.getGame().getAIAutoPlay() > 0 && g_iSASGameRecordAutoPlayRequestedTurns > 0);
	if (bDuringAutoPlay) g_iSASGameRecordAutoPlayPlayerChanges++;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=ACTIVE_PLAYER_CHANGED oldPlayer=%d newPlayer=%d autoplayActive=%d autoplayTurnsLeft=%d requestId=%d activePlayerChanges=%d totalActivePlayerChanges=%d",
		GC.getGame().getGameTurn(), eOldPlayer, eNewPlayer, bDuringAutoPlay, GC.getGame().getAIAutoPlay(),
		bDuringAutoPlay ? g_iSASGameRecordAutoPlayRequestId : -1, bDuringAutoPlay ? g_iSASGameRecordAutoPlayPlayerChanges : 0,
		g_iSASGameRecordTotalActivePlayerChanges);
}

void logSASGameRecordAutoPlayPopupDismissed(char const* szPopupKind)
{
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=AUTOPLAY_INFORMATIONAL_POPUP_DISMISSED popupKind=%s autoplayTurnsLeft=%d requestId=%d",
			GC.getGame().getGameTurn(), szPopupKind, GC.getGame().getAIAutoPlay(), g_iSASGameRecordAutoPlayRequestedTurns > 0 ? g_iSASGameRecordAutoPlayRequestId : -1);
}

void logSASGameRecordGreatPersonBorn(CvUnit const* pUnit, PlayerTypes ePlayer, CvCity const* pCity)
{
	// <!-- custom: Keep the newborn unit ID and exact spawn plot so one Great Person, especially a Great General, can be followed directly from birth through BBAI decisions to join/construct/attach/death rows instead of correlating only by player and turn. (ChatGPT-5.6-Sol) -->
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=GREAT_PERSON_BORN player=%d cityId=%d city=%S unitId=%d unit=%s x=%d y=%d combatXP=%d greatPeopleCreated=%d greatGeneralsCreated=%d greatGeneralThreshold=%d",
			GC.getGame().getGameTurn(), ePlayer, pCity == NULL ? -1 : pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(),
			pUnit == NULL ? -1 : pUnit->getID(), pUnit == NULL ? "-" : getSASGameRecordUnitType(pUnit->getUnitType()),
			pUnit == NULL ? -1 : pUnit->getX(), pUnit == NULL ? -1 : pUnit->getY(),
			ePlayer == NO_PLAYER ? 0 : GET_PLAYER(ePlayer).getCombatExperience(), ePlayer == NO_PLAYER ? 0 : GET_PLAYER(ePlayer).getGreatPeopleCreated(),
			ePlayer == NO_PLAYER ? 0 : GET_PLAYER(ePlayer).getGreatGeneralsCreated(), ePlayer == NO_PLAYER ? 0 : GET_PLAYER(ePlayer).greatPeopleThreshold(true));
}

void logSASGameRecordGreatPersonJoined(CvUnit const* pUnit, CvCity const* pCity, SpecialistTypes eSpecialist)
{
	if (pUnit == NULL || pCity == NULL)
		return;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=GREAT_PERSON_JOINED_CITY player=%d unitId=%d unit=%s cityId=%d city=%S specialist=%s freeSpecialists=%d",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()), pCity->getID(),
		getSASGameRecordQuotedCityName(pCity).GetCString(), eSpecialist == NO_SPECIALIST ? "-" : GC.getInfo(eSpecialist).getType(),
		pCity->getFreeSpecialistCount(eSpecialist));
}

// <!-- custom: Great Person births and city joining were already recorded, but other completed Great Person missions disappeared from the record when the unit was consumed.
// Record the rare completed outcome and its concrete gain without logging AI candidate values or reasoning. (GPT-5.6-Sol) -->
void logSASGameRecordGreatPersonConstructed(CvUnit const* pUnit, CvCity const* pCity, BuildingTypes eBuilding)
{
	if (pUnit == NULL || pCity == NULL || eBuilding == NO_BUILDING)
		return;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=GREAT_PERSON_USED use=CONSTRUCT_BUILDING player=%d unitId=%d unit=%s cityId=%d city=%S building=%s",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()), pCity->getID(),
		getSASGameRecordQuotedCityName(pCity).GetCString(), getSASGameRecordBuildingType(eBuilding));
}

void logSASGameRecordGreatPersonDiscovered(CvUnit const* pUnit, TechTypes eTech, int iResearch)
{
	if (pUnit == NULL || eTech == NO_TECH)
		return;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=GREAT_PERSON_USED use=DISCOVER_TECH player=%d unitId=%d unit=%s x=%d y=%d tech=%s research=%d",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()), pUnit->getX(),
		pUnit->getY(), getSASGameRecordTechType(eTech), iResearch);
}

void logSASGameRecordGreatPersonHurried(CvUnit const* pUnit, CvCity const* pCity, BuildingTypes eBuilding, int iProduction)
{
	if (pUnit == NULL || pCity == NULL || eBuilding == NO_BUILDING)
		return;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=GREAT_PERSON_USED use=HURRY_BUILDING player=%d unitId=%d unit=%s cityId=%d city=%S building=%s production=%d",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()), pCity->getID(),
		getSASGameRecordQuotedCityName(pCity).GetCString(), getSASGameRecordBuildingType(eBuilding), iProduction);
}

void logSASGameRecordGreatPersonTradeMission(CvUnit const* pUnit, CvCity const* pCity, int iGold)
{
	if (pUnit == NULL || pCity == NULL)
		return;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=GREAT_PERSON_USED use=TRADE_MISSION player=%d unitId=%d unit=%s targetPlayer=%d cityId=%d city=%S gold=%d",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()), pCity->getOwner(),
		pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(), iGold);
}

void logSASGameRecordGreatPersonGreatWork(CvUnit const* pUnit, CvCity const* pCity, int iCulture)
{
	if (pUnit == NULL || pCity == NULL)
		return;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=GREAT_PERSON_USED use=GREAT_WORK player=%d unitId=%d unit=%s cityId=%d city=%S culture=%d",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()), pCity->getID(),
		getSASGameRecordQuotedCityName(pCity).GetCString(), iCulture);
}

void logSASGameRecordGreatPersonInfiltrated(CvUnit const* pUnit, CvCity const* pCity, int iEspionage)
{
	if (pUnit == NULL || pCity == NULL)
		return;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=GREAT_PERSON_USED use=INFILTRATE player=%d unitId=%d unit=%s targetPlayer=%d targetTeam=%d cityId=%d city=%S espionage=%d",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()), pCity->getOwner(),
		pCity->getTeam(), pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(), iEspionage);
}

void logSASGameRecordGreatPersonGoldenAgeConsumed(CvUnit const* pUnit)
{
	if (pUnit == NULL)
		return;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=GREAT_PERSON_USED use=GOLDEN_AGE player=%d unitId=%d unit=%s x=%d y=%d",
			GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()), pUnit->getX(), pUnit->getY());
}

// <!-- custom: Add an optional explicit death plot because combat supplies its target while a dead attacker still reports its origin; other death paths retain the unit's current location. See KI#377. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordGreatPersonDied(CvUnit const* pUnit, PlayerTypes eResponsiblePlayer, char const* szCause, CvPlot const* pDeathPlot)
{
	if (pUnit == NULL || (!pUnit->isGoldenAge() && pUnit->getUnitInfo().getLeaderExperience() <= 0))
		return;
	CvPlot const* pPlot = (pDeathPlot == NULL ? pUnit->plot() : pDeathPlot);
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=GREAT_PERSON_DIED player=%d unitId=%d unit=%s x=%d y=%d cause=%s responsiblePlayer=%d",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()),
		pPlot == NULL ? -1 : pPlot->getX(), pPlot == NULL ? -1 : pPlot->getY(), szCause, eResponsiblePlayer);
}

// <!-- custom: Completed espionage and mission-phase interceptions share one decoder so the same eMission/iExtraData and pre-mission destructible-target context always produce identical readable target tokens. (ChatGPT-5.6-Sol) -->
static void getSASGameRecordEspionageTarget(EspionageMissionTypes eMission, int iExtraData, ImprovementTypes eTargetImprovement, RouteTypes eTargetRoute, UnitTypes eTargetUnit, char const*& szTargetKind, char const*& szTargetType)
{
	szTargetKind = "-";
	szTargetType = "-";
	if (eMission == NO_ESPIONAGEMISSION)
		return;
	CvEspionageMissionInfo const& kMission = GC.getInfo(eMission);
	if (kMission.isDestroyImprovement())
	{
		if (eTargetImprovement != NO_IMPROVEMENT)
		{
			szTargetKind = "improvement";
			szTargetType = getSASGameRecordImprovementType(eTargetImprovement);
		}
		else if (eTargetRoute != NO_ROUTE)
		{
			szTargetKind = "route";
			szTargetType = getSASGameRecordRouteType(eTargetRoute);
		}
	}
	else if (kMission.getDestroyBuildingCostFactor() > 0)
	{
		szTargetKind = "building";
		szTargetType = getSASGameRecordBuildingType((BuildingTypes)iExtraData);
	}
	else if (kMission.getDestroyProjectCostFactor() > 0)
	{
		szTargetKind = "project";
		szTargetType = getSASGameRecordProjectType((ProjectTypes)iExtraData);
	}
	else if (kMission.getDestroyUnitCostFactor() > 0 || kMission.getBuyUnitCostFactor() > 0)
	{
		szTargetKind = "unit";
		szTargetType = getSASGameRecordUnitType(eTargetUnit);
	}
	else if (kMission.getBuyTechCostFactor() > 0)
	{
		szTargetKind = "tech";
		szTargetType = getSASGameRecordTechType((TechTypes)iExtraData);
	}
	else if (kMission.getSwitchCivicCostFactor() > 0)
	{
		szTargetKind = "civic";
		szTargetType = getSASGameRecordCivicType((CivicTypes)iExtraData);
	}
	else if (kMission.getSwitchReligionCostFactor() > 0)
	{
		szTargetKind = "religion";
		szTargetType = getSASGameRecordReligionType((ReligionTypes)iExtraData);
	}
}

// <!-- custom: Before execution, resolve the live selected/runner-up target using the same plot/data the chooser just scored.
// Destructive target types are safe to inspect here because nothing has executed yet; completed/intercepted rows continue using their pre-mission captured target identity. (ChatGPT-5.6-Sol) -->
static void getSASGameRecordLiveEspionageTarget(CvUnitAI const* pUnit, PlayerTypes eTargetPlayer, SASEspionageCandidateContext const& kCandidate, char const*& szTargetKind, char const*& szTargetType)
{
	ImprovementTypes eTargetImprovement = NO_IMPROVEMENT;
	RouteTypes eTargetRoute = NO_ROUTE;
	UnitTypes eTargetUnit = NO_UNIT;
	if (pUnit != NULL && kCandidate.eMission != NO_ESPIONAGEMISSION)
	{
		CvPlot const* pPlot = pUnit->plot();
		CvEspionageMissionInfo const& kMission = GC.getInfo(kCandidate.eMission);
		if (pPlot != NULL && kMission.isDestroyImprovement())
		{
			eTargetImprovement = pPlot->getImprovementType();
			eTargetRoute = pPlot->getRouteType();
		}
		else if (eTargetPlayer != NO_PLAYER && kCandidate.iData >= 0 &&
			(kMission.getDestroyUnitCostFactor() > 0 || kMission.getBuyUnitCostFactor() > 0))
		{
			CvUnit const* pTargetUnit = GET_PLAYER(eTargetPlayer).getUnit(kCandidate.iData);
			if (pTargetUnit != NULL)
				eTargetUnit = pTargetUnit->getUnitType();
		}
	}
	getSASGameRecordEspionageTarget(kCandidate.eMission, kCandidate.iData, eTargetImprovement, eTargetRoute, eTargetUnit, szTargetKind, szTargetType);
}

// <!-- custom: The real AI mission selector contributes only its already-computed winner/runner-up arithmetic at the actual MISSION_ESPIONAGE commit boundary.
// No mission is revalued and no chooser RNG is repeated; completed effects and interception outcomes remain separate so decision -> attempt risk -> realized consequence can be reconciled compactly. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIEspionageDecision(CvUnitAI const* pUnit, PlayerTypes eTargetPlayer, SASEspionageChoiceContext const& kChoice)
{
	if (pUnit == NULL || kChoice.kBest.eMission == NO_ESPIONAGEMISSION)
		return;
	CvPlot const* pPlot = pUnit->plot();
	CvCity const* pCity = (pPlot == NULL ? NULL : pPlot->getPlotCity());
	char const* szTargetKind;
	char const* szTargetType;
	getSASGameRecordLiveEspionageTarget(pUnit, eTargetPlayer, kChoice.kBest, szTargetKind, szTargetType);
	char const* szRunnerUpTargetKind;
	char const* szRunnerUpTargetType;
	getSASGameRecordLiveEspionageTarget(pUnit, eTargetPlayer, kChoice.kRunnerUp, szRunnerUpTargetKind, szRunnerUpTargetType);
	logSASGameRecord("GAME_RECORD_AI_ESPIONAGE_DECISION turn=%d player=%d team=%d spyId=%d spy=%s spyAI=%s targetPlayer=%d targetTeam=%d targetGold=%d cityId=%d city=%S x=%d y=%d mission=%s targetKind=%s target=%s extraData=%d rawValue=%d randomPercent=%d randomizedValue=%d overhead=%d costPenalty=%d cost=%d finalValue=%d runnerUpMission=%s runnerUpTargetKind=%s runnerUpTarget=%s runnerUpData=%d runnerUpRawValue=%d runnerUpRandomPercent=%d runnerUpRandomizedValue=%d runnerUpOverhead=%d runnerUpCostPenalty=%d runnerUpCost=%d runnerUpValue=%d valueMargin=%d estimatedBaseInterceptPercent=%d estimatedSpyValue=%d estimatedEscapeCost=%d teamEP=%d espionageRate=%d bigEspionage=%d espionageEconomy=%d fortifyTurns=%d",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getTeam(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()),
		getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()), eTargetPlayer,
		eTargetPlayer == NO_PLAYER ? NO_TEAM : GET_PLAYER(eTargetPlayer).getTeam(),
		eTargetPlayer == NO_PLAYER ? -1 : GET_PLAYER(eTargetPlayer).getGold(), pCity == NULL ? -1 : pCity->getID(),
		getSASGameRecordQuotedCityName(pCity).GetCString(), pUnit->getX(), pUnit->getY(),
		getSASGameRecordEspionageMissionType(kChoice.kBest.eMission), szTargetKind, szTargetType, kChoice.kBest.iData,
		kChoice.kBest.iRawValue, kChoice.kBest.iRandomPercent, kChoice.kBest.iRandomizedValue, kChoice.kBest.iOverhead,
		kChoice.kBest.iCostPenalty, kChoice.kBest.iCost, kChoice.kBest.iFinalValue,
		getSASGameRecordEspionageMissionType(kChoice.kRunnerUp.eMission), szRunnerUpTargetKind, szRunnerUpTargetType,
		kChoice.kRunnerUp.iData, kChoice.kRunnerUp.iRawValue, kChoice.kRunnerUp.iRandomPercent, kChoice.kRunnerUp.iRandomizedValue,
		kChoice.kRunnerUp.iOverhead, kChoice.kRunnerUp.iCostPenalty, kChoice.kRunnerUp.iCost, kChoice.kRunnerUp.iFinalValue,
		kChoice.kBest.iFinalValue - kChoice.kRunnerUp.iFinalValue, kChoice.iEstimatedBaseInterceptPercent, kChoice.iSpyValue,
		kChoice.iEscapeCost, kChoice.iEspionagePoints, kChoice.iEspionageRate, kChoice.bBigEspionage, kChoice.bEspionageEconomy,
		pUnit->getFortifyTurns());
}

// <!-- custom: AI_revoltCitySpy is a separate K-Mod tactical commit path that intentionally bypasses the scored general espionage chooser.
// Record its factual city-defense gate and chosen revolt mission without fabricating winner/runner-up values or repeating any tactical/strength evaluation. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAITacticalEspionageDecision(CvUnitAI const* pUnit, CvCity const* pCity, EspionageMissionTypes eMission)
{
	if (pUnit == NULL || pCity == NULL || eMission == NO_ESPIONAGEMISSION)
		return;
	int const iMaxDefenseDamage = GC.getMAX_CITY_DEFENSE_DAMAGE();
	int const iRemainingDefensePercent = 100 * (iMaxDefenseDamage - pCity->getDefenseDamage()) / std::max(1, iMaxDefenseDamage);
	logSASGameRecord("GAME_RECORD_AI_ESPIONAGE_TACTICAL_DECISION turn=%d player=%d team=%d spyId=%d spy=%s spyAI=%s targetPlayer=%d targetTeam=%d cityId=%d city=%S x=%d y=%d mission=%s kind=CITY_REVOLT defenseDamage=%d maxDefenseDamage=%d remainingDefensePercent=%d fortifyTurns=%d",
			GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getTeam(),
			pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()), getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()),
			pCity->getOwner(), pCity->getTeam(), pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(), pCity->getX(), pCity->getY(),
			getSASGameRecordEspionageMissionType(eMission), pCity->getDefenseDamage(), iMaxDefenseDamage, iRemainingDefensePercent, pUnit->getFortifyTurns());
}

// <!-- custom: Periodic espionage totals showed investment against each rival but not what those points accomplished.
// Record completed missions plus every real mission-phase interception check; target decoding keeps stolen technologies and sabotaged buildings/projects/units readable.
// AI mission-choice provenance is emitted separately at the live chooser commit boundary above. (GPT-5.6-Sol + ChatGPT-5.6-Sol) -->
void logSASGameRecordEspionageMission(CvUnit const* pUnit, EspionageMissionTypes eMission, PlayerTypes eTargetPlayer, CvPlot const* pPlot, int iExtraData, int iCost, int iEPBefore, int iEPAfter, ImprovementTypes eTargetImprovement, RouteTypes eTargetRoute, UnitTypes eTargetUnit, int iEffectValue, char const* szEffectKind)
{
	if (pUnit == NULL || eMission == NO_ESPIONAGEMISSION)
		return;
	char const* szTargetKind;
	char const* szTargetType;
	getSASGameRecordEspionageTarget(eMission, iExtraData, eTargetImprovement, eTargetRoute, eTargetUnit, szTargetKind, szTargetType);
	CvCity const* pCity = (pPlot == NULL ? NULL : pPlot->getPlotCity());
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=ESPIONAGE_MISSION player=%d spyId=%d spy=%s spyAI=%s targetPlayer=%d targetTeam=%d mission=%s cost=%d epBefore=%d epAfter=%d cityId=%d city=%S x=%d y=%d targetKind=%s target=%s effectKind=%s effectValue=%d extraData=%d fortifyTurns=%d",
			GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()), getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()),
			eTargetPlayer, eTargetPlayer == NO_PLAYER ? NO_TEAM : GET_PLAYER(eTargetPlayer).getTeam(), getSASGameRecordEspionageMissionType(eMission), iCost, iEPBefore, iEPAfter,
			pCity == NULL ? -1 : pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(), pPlot == NULL ? -1 : pPlot->getX(), pPlot == NULL ? -1 : pPlot->getY(),
			szTargetKind, szTargetType, szEffectKind, iEffectValue, iExtraData, pUnit->getFortifyTurns());
}

void logSASGameRecordSpyInterceptionCheck(CvUnit const* pUnit, PlayerTypes eTargetPlayer, char const* szPhase, int iModifier, int iBaseInterceptPercent, int iInterceptChanceX100, int iInterceptRoll, bool bIntercepted, int iCounterespionageMod, bool bCounterSpyDefenseAtPlot, int iTargetSpiesOnPlot, int iTargetCounterSpyUnitsOnPlot, int iAttackerSpiesOnPlot, int iCityEspionageDefenseModifier, bool bRecentMissionBonusApplies, EspionageMissionTypes eMission, int iExtraData, ImprovementTypes eTargetImprovement, RouteTypes eTargetRoute, UnitTypes eTargetUnit)
{
	if (pUnit == NULL)
		return;
	char const* szTargetKind;
	char const* szTargetType;
	getSASGameRecordEspionageTarget(eMission, iExtraData, eTargetImprovement, eTargetRoute, eTargetUnit, szTargetKind, szTargetType);
	CvCity const* pCity = pUnit->getPlot().getPlotCity();
	logSASGameRecord("GAME_RECORD_SPY_INTERCEPTION_CHECK turn=%d player=%d team=%d spyId=%d spy=%s spyAI=%s targetPlayer=%d targetTeam=%d phase=%s mission=%s targetKind=%s target=%s extraData=%d x=%d y=%d cityId=%d city=%S modifier=%d baseInterceptPercent=%d interceptChanceX100=%d roll=%d intercepted=%d counterespionageMod=%d counterSpyDefenseAtPlot=%d targetSpiesOnPlot=%d targetCounterSpyUnitsOnPlot=%d attackerSpiesOnPlot=%d cityEspionageDefenseModifier=%d recentMissionBonusApplies=%d fortifyTurns=%d",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getTeam(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()),
		getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()), eTargetPlayer,
		eTargetPlayer == NO_PLAYER ? NO_TEAM : GET_PLAYER(eTargetPlayer).getTeam(), szPhase, getSASGameRecordEspionageMissionType(eMission),
		szTargetKind, szTargetType, iExtraData, pUnit->getX(), pUnit->getY(), pCity == NULL ? -1 : pCity->getID(),
		getSASGameRecordQuotedCityName(pCity).GetCString(), iModifier, iBaseInterceptPercent, iInterceptChanceX100, iInterceptRoll,
		bIntercepted, iCounterespionageMod, bCounterSpyDefenseAtPlot, iTargetSpiesOnPlot, iTargetCounterSpyUnitsOnPlot, iAttackerSpiesOnPlot,
		iCityEspionageDefenseModifier, bRecentMissionBonusApplies, pUnit->getFortifyTurns());
}

void logSASGameRecordSpyIntercepted(CvUnit const* pUnit, PlayerTypes eTargetPlayer, char const* szPhase, int iModifier, int iInterceptChanceX100, EspionageMissionTypes eMission, int iExtraData, ImprovementTypes eTargetImprovement, RouteTypes eTargetRoute, UnitTypes eTargetUnit)
{
	if (pUnit == NULL)
		return;
	char const* szTargetKind;
	char const* szTargetType;
	getSASGameRecordEspionageTarget(eMission, iExtraData, eTargetImprovement, eTargetRoute, eTargetUnit, szTargetKind, szTargetType);
	CvCity const* pCity = pUnit->getPlot().getPlotCity();
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=SPY_INTERCEPTED player=%d spyId=%d spy=%s spyAI=%s targetPlayer=%d targetTeam=%d phase=%s mission=%s targetKind=%s target=%s extraData=%d x=%d y=%d cityId=%d city=%S modifier=%d interceptChanceX100=%d fortifyTurns=%d",
			GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()), getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()),
			eTargetPlayer, eTargetPlayer == NO_PLAYER ? NO_TEAM : GET_PLAYER(eTargetPlayer).getTeam(), szPhase, getSASGameRecordEspionageMissionType(eMission),
			szTargetKind, szTargetType, iExtraData, pUnit->getX(), pUnit->getY(), pCity == NULL ? -1 : pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(),
			iModifier, iInterceptChanceX100, pUnit->getFortifyTurns());
}

void logSASGameRecordGreatGeneralAttached(CvUnit const* pGreatGeneral, CvUnit const* pTargetUnit, PromotionTypes ePromotion)
{
	if (pGreatGeneral == NULL || pTargetUnit == NULL)
		return;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=GREAT_GENERAL_ATTACHED player=%d generalUnitId=%d generalUnit=%s targetUnitId=%d targetUnit=%s targetUnitAI=%s x=%d y=%d promotion=%s targetXP=%d targetLevel=%d",
			GC.getGame().getGameTurn(), pGreatGeneral->getOwner(), pGreatGeneral->getID(), getSASGameRecordUnitType(pGreatGeneral->getUnitType()),
			pTargetUnit->getID(), getSASGameRecordUnitType(pTargetUnit->getUnitType()), getSASGameRecordUnitAIType(pTargetUnit->AI_getUnitAIType()),
			pTargetUnit->getX(), pTargetUnit->getY(), ePromotion == NO_PROMOTION ? "-" : GC.getInfo(ePromotion).getType(),
			pTargetUnit->getExperience(), pTargetUnit->getLevel());
}


void logSASGameRecordUnitScrapped(CvUnit const* pUnit)
{
	if (pUnit == NULL)
		return;
	PlayerTypes const ePlayer = pUnit->getOwner();
	SASGameRecordPlayerFlow& kFlow = g_akSASGameRecordPlayerFlow[ePlayer];
	kFlow.iScrapped++;
	kFlow.iScrappedProductionNeeded += GET_PLAYER(ePlayer).getProductionNeeded(pUnit->getUnitType());
	if (gGameRecordLogLevel >= 3)
	{
		logSASGameRecord("GAME_RECORD_ACTION turn=%d type=UNIT_SCRAPPED player=%d unitId=%d unit=%s unitAI=%s x=%d y=%d damage=%d xp=%d level=%d age=%d cargo=%d cargoSpace=%d",
			GC.getGame().getGameTurn(), ePlayer, pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()), getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()),
			pUnit->getX(), pUnit->getY(), pUnit->getDamage(), pUnit->getExperience(), pUnit->getLevel(), GC.getGame().getGameTurn() - pUnit->getGameTurnCreated(),
			pUnit->getCargo(), pUnit->cargoSpace());
	}
}

void logSASGameRecordUnitUpgraded(CvUnit const* pOldUnit, CvUnit const* pNewUnit, int iCost)
{
	if (pOldUnit == NULL || pNewUnit == NULL)
		return;
	PlayerTypes const ePlayer = pNewUnit->getOwner();
	SASGameRecordPlayerFlow& kFlow = g_akSASGameRecordPlayerFlow[ePlayer];
	kFlow.iUpgrades++;
	kFlow.iUpgradeGold += iCost;
	if (gGameRecordLogLevel >= 3)
	{
		logSASGameRecord("GAME_RECORD_ACTION turn=%d type=UNIT_UPGRADED player=%d oldUnitId=%d newUnitId=%d fromUnit=%s toUnit=%s unitAI=%s x=%d y=%d cost=%d oldXP=%d newXP=%d oldLevel=%d newLevel=%d",
			GC.getGame().getGameTurn(), ePlayer, pOldUnit->getID(), pNewUnit->getID(), getSASGameRecordUnitType(pOldUnit->getUnitType()),
			getSASGameRecordUnitType(pNewUnit->getUnitType()), getSASGameRecordUnitAIType(pNewUnit->AI_getUnitAIType()),
			pNewUnit->getX(), pNewUnit->getY(), iCost, pOldUnit->getExperience(),
			pNewUnit->getExperience(), pOldUnit->getLevel(), pNewUnit->getLevel());
	}
}

void logSASGameRecordUnitCaptured(PlayerTypes eOldOwner, UnitTypes eOldUnitType, CvUnit const* pNewUnit)
{
	if (pNewUnit == NULL)
		return;
	PlayerTypes const eNewOwner = pNewUnit->getOwner();
	SASGameRecordPlayerFlow& kFlow = g_akSASGameRecordPlayerFlow[eNewOwner];
	kFlow.iCaptured++;
	kFlow.iCapturedProductionNeeded += GET_PLAYER(eNewOwner).getProductionNeeded(pNewUnit->getUnitType());
	// <!-- custom: Captures are rare and strategically distinct, so retain the exact captured type and location at level 2 in addition to the interval aggregate. (GPT-5.6-Sol + GPT-5.6 Thinking) -->
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=UNIT_CAPTURED oldOwner=%d newOwner=%d oldUnit=%s newUnitId=%d newUnit=%s newUnitAI=%s x=%d y=%d",
		GC.getGame().getGameTurn(), eOldOwner, eNewOwner, getSASGameRecordUnitType(eOldUnitType), pNewUnit->getID(),
		getSASGameRecordUnitType(pNewUnit->getUnitType()), getSASGameRecordUnitAIType(pNewUnit->AI_getUnitAIType()),
		pNewUnit->getX(), pNewUnit->getY());
}

void logSASGameRecordCityBombard(CvUnit const* pUnit, CvCity const* pCity, char const* szMode, int iBombardRate, bool bIgnoreBuildingDefense, int iDefenseModifierBefore, int iDefenseDamageBefore)
{
	if (pUnit == NULL || pCity == NULL)
		return;
	prepareSASGameRecordTurnChanges();
	const int iGameTurn = GC.getGame().getGameTurn();
	const int iDefenseModifierAfter = pCity->getDefenseModifier(false);
	const int iDefenseDamageAfter = pCity->getDefenseDamage();
	// <!-- custom: Consecutive bombard actions against the same city are synthetic history, not five nearly identical rows for five Trebuchets.
	// Keep sequences separate when attacker/mode/city changes or defense continuity breaks, and the generic writer flushes a pending sequence before the next unrelated GameRecord row so battle-vs-bombard order remains observable. (GPT-5.6 Thinking) -->
	const bool bContinueSequence = (g_kSASGameRecordPendingCityBombard.bValid && g_kSASGameRecordPendingCityBombard.iTurn == iGameTurn && g_kSASGameRecordPendingCityBombard.szMode == szMode && g_kSASGameRecordPendingCityBombard.ePlayer == pUnit->getOwner() && g_kSASGameRecordPendingCityBombard.eTargetPlayer == pCity->getOwner() && g_kSASGameRecordPendingCityBombard.iCityId == pCity->getID() && g_kSASGameRecordPendingCityBombard.iDefenseModifierAfter == iDefenseModifierBefore && g_kSASGameRecordPendingCityBombard.iDefenseDamageAfter == iDefenseDamageBefore);
	if (!bContinueSequence)
	{
		flushSASGameRecordPendingCityBombard();
		g_kSASGameRecordPendingCityBombard.bValid = true;
		g_kSASGameRecordPendingCityBombard.iTurn = iGameTurn;
		g_kSASGameRecordPendingCityBombard.szMode = szMode;
		g_kSASGameRecordPendingCityBombard.ePlayer = pUnit->getOwner();
		g_kSASGameRecordPendingCityBombard.eTargetPlayer = pCity->getOwner();
		g_kSASGameRecordPendingCityBombard.iCityId = pCity->getID();
		g_kSASGameRecordPendingCityBombard.szCity = getSASGameRecordQuotedCityName(pCity);
		g_kSASGameRecordPendingCityBombard.iX = pCity->getX();
		g_kSASGameRecordPendingCityBombard.iY = pCity->getY();
		g_kSASGameRecordPendingCityBombard.iDefenseModifierBefore = iDefenseModifierBefore;
		g_kSASGameRecordPendingCityBombard.iDefenseDamageBefore = iDefenseDamageBefore;
	}
	g_kSASGameRecordPendingCityBombard.iActions++;
	g_kSASGameRecordPendingCityBombard.iBombardRateTotal += iBombardRate;
	if (bIgnoreBuildingDefense) g_kSASGameRecordPendingCityBombard.iIgnoreBuildingDefenseActions++;
	g_kSASGameRecordPendingCityBombard.iDefenseModifierAfter = iDefenseModifierAfter;
	g_kSASGameRecordPendingCityBombard.iTotalDefense = pCity->getTotalDefense(false);
	g_kSASGameRecordPendingCityBombard.iDefenseDamageAfter = iDefenseDamageAfter;
	g_kSASGameRecordPendingCityBombard.iDefenseDamageMax = GC.getMAX_CITY_DEFENSE_DAMAGE();
	addSASGameRecordCityBombardTypeCount(g_kSASGameRecordPendingCityBombard.aUnitTypes, getSASGameRecordUnitType(pUnit->getUnitType()));
	addSASGameRecordCityBombardTypeCount(g_kSASGameRecordPendingCityBombard.aUnitAIs, getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()));
}

void logSASGameRecordAirStrike(CvUnit const* pUnit, CvUnit const* pDefender, int iDefenderDamageBefore, int iDefenderDamageAfter)
{
	if (pUnit == NULL || pDefender == NULL)
		return;
	CvPlot const* pTargetPlot = pDefender->plot();
	CvCity const* pCity = (pTargetPlot == NULL ? NULL : pTargetPlot->getPlotCity());
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=AIR_STRIKE player=%d unitId=%d unit=%s unitAI=%s fromX=%d fromY=%d targetPlayer=%d targetUnitId=%d targetUnit=%s targetUnitAI=%s x=%d y=%d cityPlot=%d cityId=%d city=%S attackerAirBaseStr=%d defenderBaseStr=%d defenderDamageBefore=%d defenderDamageAfter=%d damageDealt=%d airCombatLimit=%d",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()),
		getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()), pUnit->getX(), pUnit->getY(), pDefender->getOwner(), pDefender->getID(),
		getSASGameRecordUnitType(pDefender->getUnitType()), getSASGameRecordUnitAIType(pDefender->AI_getUnitAIType()),
		pTargetPlot == NULL ? -1 : pTargetPlot->getX(), pTargetPlot == NULL ? -1 : pTargetPlot->getY(), pCity != NULL,
		pCity == NULL ? -1 : pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(), pUnit->airBaseCombatStr(),
		pDefender->baseCombatStr(), iDefenderDamageBefore, iDefenderDamageAfter, std::max(0, iDefenderDamageAfter - iDefenderDamageBefore),
		pUnit->airCombatLimit());
}

void logSASGameRecordAirInterception(CvUnit const* pAttacker, CvUnit const* pInterceptor, CvPlot const* pTargetPlot, int iAttackerDamageTaken, int iInterceptorDamageTaken)
{
	if (pAttacker == NULL || pInterceptor == NULL || pTargetPlot == NULL)
		return;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=AIR_INTERCEPTION attackerPlayer=%d attackerUnitId=%d attackerUnit=%s attackerUnitAI=%s interceptorPlayer=%d interceptorUnitId=%d interceptorUnit=%s interceptorUnitAI=%s x=%d y=%d attackerDamageTaken=%d interceptorDamageTaken=%d attackerDead=%d interceptorDead=%d attackerIsAir=%d",
		GC.getGame().getGameTurn(), pAttacker->getOwner(), pAttacker->getID(), getSASGameRecordUnitType(pAttacker->getUnitType()),
		getSASGameRecordUnitAIType(pAttacker->AI_getUnitAIType()), pInterceptor->getOwner(), pInterceptor->getID(),
		getSASGameRecordUnitType(pInterceptor->getUnitType()), getSASGameRecordUnitAIType(pInterceptor->AI_getUnitAIType()),
		pTargetPlot->getX(), pTargetPlot->getY(), iAttackerDamageTaken, iInterceptorDamageTaken, pAttacker->isDead(), pInterceptor->isDead(),
		pAttacker->getDomainType() == DOMAIN_AIR);
}

void logSASGameRecordAirBombPlot(CvUnit const* pUnit, CvPlot const* pTargetPlot, char const* szTargetKind, char const* szTarget, bool bSuccess)
{
	if (pUnit == NULL || pTargetPlot == NULL)
		return;
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=AIR_BOMB_PLOT player=%d unitId=%d unit=%s unitAI=%s fromX=%d fromY=%d targetOwner=%d x=%d y=%d targetKind=%s target=%s success=%d",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()),
		getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()), pUnit->getX(), pUnit->getY(), pTargetPlot->getOwner(), pTargetPlot->getX(),
		pTargetPlot->getY(), szTargetKind, szTarget, bSuccess);
}

// <!-- custom: Record the launch after its interception roll while the nuke unit and pre-detonation target still exist.
// CvUnit::nuke passes its already-computed affected-team flags, so this helper only formats them. (ChatGPT-5.6-Sol) -->
void logSASGameRecordNukeLaunched(CvUnit const* pUnit, CvPlot const* pTargetPlot, bool const* pabAffectedTeams, bool bIntercepted, TeamTypes eBestInterceptorTeam, int iInterceptionChance)
{
	if (pUnit == NULL || pTargetPlot == NULL || pabAffectedTeams == NULL)
		return;
	CvString szAffectedTeams;
	for (int iTeam = 0; iTeam < MAX_TEAMS; iTeam++)
	{
		if (pabAffectedTeams[iTeam])
			appendSASDiagnosticIntListValue(szAffectedTeams, iTeam);
	}
	CvCity const* pTargetCity = pTargetPlot->getPlotCity();
	logSASGameRecord("GAME_RECORD_ACTION turn=%d type=NUKE_LAUNCHED player=%d team=%d unitId=%d unit=%s x=%d y=%d plotOwner=%d plotTeam=%d targetCityId=%d targetCity=%S targetCityOwner=%d targetCityTeam=%d targetCityPopulation=%d affectedTeams=%s intercepted=%d bestInterceptorTeam=%d interceptionChance=%d",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getTeam(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()),
		pTargetPlot->getX(), pTargetPlot->getY(), pTargetPlot->getOwner(), pTargetPlot->getTeam(),
		pTargetCity == NULL ? -1 : pTargetCity->getID(), getSASGameRecordQuotedCityName(pTargetCity).GetCString(),
		pTargetCity == NULL ? NO_PLAYER : pTargetCity->getOwner(), pTargetCity == NULL ? NO_TEAM : pTargetCity->getTeam(),
		pTargetCity == NULL ? -1 : pTargetCity->getPopulation(), getSASDiagnosticOrDash(szAffectedTeams).GetCString(), bIntercepted,
		eBestInterceptorTeam, iInterceptionChance);
}

// <!-- custom: CvPlot::nukeExplosion already accumulates the real post-random damage effects for player messages.
// Reuse only those counters here, plus exact fallout/citizen totals gathered in the same loop, so the recorder adds no second map/unit scan. (ChatGPT-5.6-Sol) -->
void logSASGameRecordNukeEffects(CvUnit const* pUnit, CvPlot const* pTargetPlot, int iFalloutPlotsCreated, int iImprovementsDestroyed, int iFeaturesDestroyed, int iUnitsDamaged, int iUnitsKilled, int iBuildingsDestroyed, int iCitiesAffected, int iPopulationKilled)
{
	if (pUnit == NULL || pTargetPlot == NULL)
		return;
	CvCity const* pTargetCity = pTargetPlot->getPlotCity();
	logSASGameRecord("GAME_RECORD_NUKE_EFFECTS turn=%d player=%d team=%d unitId=%d unit=%s x=%d y=%d targetCityId=%d targetCity=%S targetCityOwner=%d targetCityTeam=%d targetCityPopulationAfter=%d falloutPlotsCreated=%d improvementsDestroyed=%d featuresDestroyed=%d unitsDamaged=%d unitsKilled=%d buildingsDestroyed=%d citiesAffected=%d populationKilled=%d nukesExplodedAfter=%d",
		GC.getGame().getGameTurn(), pUnit->getOwner(), pUnit->getTeam(), pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()),
		pTargetPlot->getX(), pTargetPlot->getY(), pTargetCity == NULL ? -1 : pTargetCity->getID(),
		getSASGameRecordQuotedCityName(pTargetCity).GetCString(), pTargetCity == NULL ? NO_PLAYER : pTargetCity->getOwner(),
		pTargetCity == NULL ? NO_TEAM : pTargetCity->getTeam(), pTargetCity == NULL ? -1 : pTargetCity->getPopulation(),
		iFalloutPlotsCreated, iImprovementsDestroyed, iFeaturesDestroyed, iUnitsDamaged, iUnitsKilled, iBuildingsDestroyed, iCitiesAffected,
		iPopulationKilled, GC.getGame().getNukesExploded());
}


// <!-- custom: Preserve each city caught in an actual unit-launched blast, including cases where defenses reduce realized losses to zero.
// The building list is assembled only from buildings the existing destruction loop actually removed; no city/building rescan or RNG is added. (ChatGPT-5.6-Sol) -->
void logSASGameRecordNukeCityEffect(CvUnit const* pNukeUnit, CvCity const* pCity, int iPopulationBefore, int iNukeModifier, std::vector<BuildingTypes> const& aeBuildingsDestroyed)
{
	if (pNukeUnit == NULL || pCity == NULL)
		return;
	CvString szBuildingsDestroyed;
	for (size_t i = 0; i < aeBuildingsDestroyed.size(); i++)
		appendSASGameRecordType(szBuildingsDestroyed, getSASGameRecordBuildingType(aeBuildingsDestroyed[i]));
	int const iPopulationAfter = pCity->getPopulation();
	logSASGameRecord("GAME_RECORD_NUKE_CITY_EFFECT turn=%d player=%d team=%d nukeUnitId=%d nukeUnit=%s affectedPlayer=%d affectedTeam=%d cityId=%d city=%S x=%d y=%d nukeModifier=%d populationBefore=%d populationAfter=%d populationKilled=%d buildingsDestroyedCount=%d buildingsDestroyed=%s",
			GC.getGame().getGameTurn(), pNukeUnit->getOwner(), pNukeUnit->getTeam(), pNukeUnit->getID(), getSASGameRecordUnitType(pNukeUnit->getUnitType()),
			pCity->getOwner(), pCity->getTeam(), pCity->getID(), getSASGameRecordQuotedCityName(pCity).GetCString(), pCity->getX(), pCity->getY(), iNukeModifier,
			iPopulationBefore, iPopulationAfter, std::max(0, iPopulationBefore - iPopulationAfter), (int)aeBuildingsDestroyed.size(),
			getSASDiagnosticOrDash(szBuildingsDestroyed).GetCString());
}

// <!-- custom: Level-3 nuke unit rows retain tactical identity before the existing damage/kill operation can remove the object.
// Direct combat damage records exact before/after damage; indirect cargo and noncombat death use damageAfter=-1 rather than inventing a damage value that gameplay never assigned. (ChatGPT-5.6-Sol) -->
void logSASGameRecordNukeUnitEffect(CvUnit const* pNukeUnit, CvUnit const* pAffectedUnit, CvPlot const* pPlot, int iDamageBefore, int iDamageAfter, bool bKilled, char const* szCause)
{
	if (pNukeUnit == NULL || pAffectedUnit == NULL || pPlot == NULL)
		return;
	CvUnit const* pTransport = pAffectedUnit->getTransportUnit();
	int const iDamageDelta = (iDamageBefore >= 0 && iDamageAfter >= 0 ? iDamageAfter - iDamageBefore : -1);
	logSASGameRecord("GAME_RECORD_NUKE_UNIT_EFFECT turn=%d player=%d team=%d nukeUnitId=%d nukeUnit=%s affectedPlayer=%d affectedTeam=%d unitId=%d unit=%s unitAI=%s x=%d y=%d damageBefore=%d damageAfter=%d damageDelta=%d killed=%d cause=%s cargo=%d transportPlayer=%d transportId=%d",
		GC.getGame().getGameTurn(), pNukeUnit->getOwner(), pNukeUnit->getTeam(), pNukeUnit->getID(),
		getSASGameRecordUnitType(pNukeUnit->getUnitType()), pAffectedUnit->getOwner(), pAffectedUnit->getTeam(), pAffectedUnit->getID(),
		getSASGameRecordUnitType(pAffectedUnit->getUnitType()), getSASGameRecordUnitAIType(pAffectedUnit->AI_getUnitAIType()), pPlot->getX(),
		pPlot->getY(), iDamageBefore, iDamageAfter, iDamageDelta, bKilled, szCause, pAffectedUnit->isCargo(),
		pTransport == NULL ? NO_PLAYER : pTransport->getOwner(), pTransport == NULL ? -1 : pTransport->getID());
}

// <!-- custom: Only ordinary civilization-vs-civilization battles with no attacker withdrawal chance and a lethal combat limit form a true binary win/loss sample.
// Siege/combat-limit fights, withdrawals and Barbarian free-win rules are recorded separately rather than contaminating expected-vs-observed luck. (ChatGPT-5.6-Sol) -->
static bool isSASGameRecordLuckEligible(CvUnit const& kAttacker, CvUnit const& kDefender)
{
	PlayerTypes const eAttacker = kAttacker.getOwner();
	PlayerTypes const eDefender = kDefender.getOwner();
	return (eAttacker >= 0 && eAttacker < MAX_CIV_PLAYERS && eDefender >= 0 && eDefender < MAX_CIV_PLAYERS && !kAttacker.isBarbarian() && !kDefender.isBarbarian() && kAttacker.withdrawalProbability() <= 0 && kAttacker.combatLimit() >= kDefender.maxHitPoints());
}

static bool popSASGameRecordCombatPending(CvUnit const* pUnitA, CvUnit const* pUnitB, CvPlot const* pBattlePlot, SASGameRecordCombatPending& kResult)
{
	if (pUnitA == NULL || pUnitB == NULL)
		return false;
	for (int iI = (int)g_aSASGameRecordCombatPending.size() - 1; iI >= 0; iI--)
	{
		SASGameRecordCombatPending const& kPending = g_aSASGameRecordCombatPending[iI];
		bool const bAIsAttacker = (pUnitA->getOwner() == kPending.eAttacker && pUnitA->getID() == kPending.iAttackerUnitId && pUnitB->getOwner() == kPending.eDefender && pUnitB->getID() == kPending.iDefenderUnitId);
		bool const bBIsAttacker = (pUnitB->getOwner() == kPending.eAttacker && pUnitB->getID() == kPending.iAttackerUnitId && pUnitA->getOwner() == kPending.eDefender && pUnitA->getID() == kPending.iDefenderUnitId);
		if (!bAIsAttacker && !bBIsAttacker)
			continue;
		if (pBattlePlot != NULL && (pBattlePlot->getX() != kPending.iX || pBattlePlot->getY() != kPending.iY))
			continue;
		kResult = kPending;
		g_aSASGameRecordCombatPending.erase(g_aSASGameRecordCombatPending.begin() + iI);
		return true;
	}
	return false;
}

void noteSASGameRecordCombatStarted(CvUnit const* pAttacker, CvUnit const* pDefender, CvPlot const* pBattlePlot)
{
	if (pAttacker == NULL || pDefender == NULL || pBattlePlot == NULL)
		return;
	bool const bLuckEligible = isSASGameRecordLuckEligible(*pAttacker, *pDefender);
	bool const bLogExactBattle = (gGameRecordLogLevel >= 3);
	bool const bCaptureCombatContext = (bLogExactBattle || bLuckEligible);
	// <!-- custom: Level 2 needs transient context only for the exact-odds statistical sample.
	// Level 3 also keeps attacker identity for every exact battle row, including Barbarian fights whose special free-win semantics intentionally leave odds unknown here.
	// Name the positive requirement explicitly, then early-return to keep the expensive capture path unnested. (ChatGPT-5.6-Sol) -->
	if (!bCaptureCombatContext)
		return;
	SASGameRecordCombatPending kPending;
	kPending.eAttacker = pAttacker->getOwner();
	kPending.eDefender = pDefender->getOwner();
	kPending.iAttackerUnitId = pAttacker->getID();
	kPending.iDefenderUnitId = pDefender->getID();
	kPending.iX = pBattlePlot->getX();
	kPending.iY = pBattlePlot->getY();
	// <!-- custom: Exact level-3 battle rows should say whether the attacker actually crossed a river or attacked from water, not force later analysis to reconstruct transient origin state from map snapshots.
	// Mirror CvUnit::maxCombatStr's penalty conditions without changing combat; level-2 luck-only pending context keeps these fields unknown to avoid extra work. (ChatGPT-5.6-Sol) -->
	kPending.iAttackerFromX = -1;
	kPending.iAttackerFromY = -1;
	kPending.iRiverCrossingAttack = -1;
	kPending.iRiverAttackPenalty = -1;
	kPending.iAmphibiousAttack = -1;
	kPending.iAmphibiousAttackPenalty = -1;
	if (bLogExactBattle)
	{
		CvPlot const& kFrom = pAttacker->getPlot();
		bool const bAdjacent = (stepDistance(&kFrom, pBattlePlot) == 1);
		bool const bRiverCrossingAttack = (bAdjacent && kFrom.isRiverCrossing(directionXY(kFrom, *pBattlePlot)));
		bool const bAmphibiousAttack = (!pBattlePlot->isWater() && kFrom.isWater());
		kPending.iAttackerFromX = kFrom.getX();
		kPending.iAttackerFromY = kFrom.getY();
		kPending.iRiverCrossingAttack = bRiverCrossingAttack;
		kPending.iRiverAttackPenalty = (bRiverCrossingAttack && !pAttacker->isRiver());
		kPending.iAmphibiousAttack = bAmphibiousAttack;
		kPending.iAmphibiousAttackPenalty = (bAmphibiousAttack && !pAttacker->isAmphib());
	}
	kPending.bLuckEligible = bLuckEligible;
	bool const bCivilizationBattle = (pAttacker->getOwner() >= 0 && pAttacker->getOwner() < MAX_CIV_PLAYERS && pDefender->getOwner() >= 0 && pDefender->getOwner() < MAX_CIV_PLAYERS && !pAttacker->isBarbarian() && !pDefender->isBarbarian());
	kPending.iAttackerCombatOddsPermille = ((bLuckEligible || (bLogExactBattle && bCivilizationBattle)) ? calculateCombatOdds(*pAttacker, *pDefender) : -1);
	g_aSASGameRecordCombatPending.push_back(kPending);
}

static void recordSASGameRecordBattleLuck(PlayerTypes ePlayer, int iOwnOddsPermille, bool bWon)
{
	if (ePlayer < 0 || ePlayer >= MAX_CIV_PLAYERS || iOwnOddsPermille < 0 || iOwnOddsPermille > 1000)
		return;
	SASGameRecordBattleQuality* apQuality[2] = { &g_akSASGameRecordBattleQuality[ePlayer], &g_akSASGameRecordTotalBattleQuality[ePlayer] };
	for (int iI = 0; iI < 2; iI++)
	{
		SASGameRecordBattleQuality& kQuality = *apQuality[iI];
		kQuality.iLuckEligibleBattles++;
		kQuality.iExpectedWinsX1000 += iOwnOddsPermille;
		if (bWon)
		{
			kQuality.iLuckEligibleWins++;
			if (iOwnOddsPermille < 500) kQuality.iUpsetWins++;
			if (kQuality.iLowestOddsWinPermille < 0 || iOwnOddsPermille < kQuality.iLowestOddsWinPermille) kQuality.iLowestOddsWinPermille = iOwnOddsPermille;
		}
		else
		{
			if (iOwnOddsPermille > 500) kQuality.iUpsetLosses++;
			if (iOwnOddsPermille > kQuality.iHighestOddsLossPermille) kQuality.iHighestOddsLossPermille = iOwnOddsPermille;
		}
	}
}

void logSASGameRecordNonlethalCombat(CvUnit const* pAttacker, CvUnit const* pDefender, CvPlot const* pBattlePlot, bool bCombatLimitReached)
{
	if (pAttacker == NULL || pDefender == NULL || pBattlePlot == NULL)
		return;
	PlayerTypes const eAttacker = pAttacker->getOwner();
	PlayerTypes const eDefender = pDefender->getOwner();
	if (eAttacker >= 0 && eAttacker < MAX_CIV_PLAYERS)
	{
		SASGameRecordBattleQuality* apQuality[2] = { &g_akSASGameRecordBattleQuality[eAttacker], &g_akSASGameRecordTotalBattleQuality[eAttacker] };
		for (int iI = 0; iI < 2; iI++)
		{
			if (bCombatLimitReached) apQuality[iI]->iCombatLimitAttacks++;
			else apQuality[iI]->iWithdrawals++;
		}
	}
	if (eDefender >= 0 && eDefender < MAX_CIV_PLAYERS)
	{
		SASGameRecordBattleQuality* apQuality[2] = { &g_akSASGameRecordBattleQuality[eDefender], &g_akSASGameRecordTotalBattleQuality[eDefender] };
		for (int iI = 0; iI < 2; iI++)
		{
			if (bCombatLimitReached) apQuality[iI]->iCombatLimitDefenses++;
			else apQuality[iI]->iEnemyWithdrawals++;
		}
	}
	SASGameRecordCombatPending kPending;
	bool const bPending = popSASGameRecordCombatPending(pAttacker, pDefender, pBattlePlot, kPending);
	if (gGameRecordLogLevel >= 3)
	{
		logSASGameRecord("GAME_RECORD_BATTLE_NONLETHAL turn=%d attacker=%d defender=%d attackerUnit=%s attackerUnitId=%d defenderUnit=%s defenderUnitId=%d reason=%s x=%d y=%d cityPlot=%d attackerFromX=%d attackerFromY=%d riverCrossingAttack=%d riverAttackPenalty=%d amphibiousAttack=%d amphibiousAttackPenalty=%d attackerBaseStr=%d defenderBaseStr=%d attackerDamage=%d defenderDamage=%d attackerCombatLimit=%d attackerWithdrawal=%d attackerCombatOddsPermille=%d attackerXP=%d attackerLevel=%d defenderXP=%d defenderLevel=%d",
			GC.getGame().getGameTurn(), eAttacker, eDefender, getSASGameRecordUnitType(pAttacker->getUnitType()), pAttacker->getID(),
			getSASGameRecordUnitType(pDefender->getUnitType()), pDefender->getID(), bCombatLimitReached ? "COMBAT_LIMIT" : "WITHDRAWAL",
			pBattlePlot->getX(), pBattlePlot->getY(), pBattlePlot->isCity(),
			bPending ? kPending.iAttackerFromX : -1, bPending ? kPending.iAttackerFromY : -1,
			bPending ? kPending.iRiverCrossingAttack : -1, bPending ? kPending.iRiverAttackPenalty : -1,
			bPending ? kPending.iAmphibiousAttack : -1, bPending ? kPending.iAmphibiousAttackPenalty : -1,
			pAttacker->baseCombatStr(), pDefender->baseCombatStr(), pAttacker->getDamage(), pDefender->getDamage(), pAttacker->combatLimit(), pAttacker->withdrawalProbability(),
			bPending ? kPending.iAttackerCombatOddsPermille : -1, pAttacker->getExperience(), pAttacker->getLevel(), pDefender->getExperience(), pDefender->getLevel());
	}
}

void logSASGameRecordExperienceChange(CvUnit const* pUnit, int iAdjustedChange, int iActualChange, bool bFromCombat)
{
	if (pUnit == NULL)
		return;
	PlayerTypes const ePlayer = pUnit->getOwner();
	if (ePlayer < 0 || ePlayer >= MAX_PLAYERS)
		return;
	int const iGained = std::max(0, iActualChange);
	int const iLostAdjustment = std::max(0, -iActualChange);
	int const iPreventedByCap = (iAdjustedChange > 0 ? std::max(0, iAdjustedChange - iGained) : 0);
	if (iGained <= 0 && iLostAdjustment <= 0 && iPreventedByCap <= 0)
		return;
	SASGameRecordPlayerFlow& kFlow = g_akSASGameRecordPlayerFlow[ePlayer];
	kFlow.iExperienceGained += iGained;
	if (bFromCombat) kFlow.iCombatExperienceGained += iGained;
	else kFlow.iNonCombatExperienceGained += iGained;
	kFlow.iExperiencePreventedByCap += iPreventedByCap;
	kFlow.iExperienceLostAdjustments += iLostAdjustment;
	SASGameRecordMilitaryQualityTotals& kTotal = g_akSASGameRecordMilitaryQualityTotals[ePlayer];
	kTotal.iExperienceGained += iGained;
	if (bFromCombat) kTotal.iCombatExperienceGained += iGained;
	else kTotal.iNonCombatExperienceGained += iGained;
	kTotal.iExperiencePreventedByCap += iPreventedByCap;
	kTotal.iExperienceLostAdjustments += iLostAdjustment;
}

void logSASGameRecordUnitPromoted(CvUnit const* pUnit, PromotionTypes ePromotion)
{
	if (pUnit == NULL || ePromotion == NO_PROMOTION)
		return;
	PlayerTypes const ePlayer = pUnit->getOwner();
	if (ePlayer < 0 || ePlayer >= MAX_PLAYERS)
		return;
	bool const bLeaderPromotion = GC.getInfo(ePromotion).isLeader();
	SASGameRecordPlayerFlow& kFlow = g_akSASGameRecordPlayerFlow[ePlayer];
	SASGameRecordMilitaryQualityTotals& kTotal = g_akSASGameRecordMilitaryQualityTotals[ePlayer];
	if (bLeaderPromotion)
	{
		kFlow.iLeaderPromotionApplications++;
		kTotal.iLeaderPromotionApplications++;
		return; // <!-- custom: GREAT_GENERAL_ATTACHED already provides the exact level-2 action with both source and target units. (ChatGPT-5.6-Sol) -->
	}
	kFlow.iPromotionsChosen++;
	kTotal.iPromotionsChosen++;
	int const iPromotion = (int)ePromotion;
	if (iPromotion >= 0 && iPromotion < (int)kFlow.aiPromotionChoices.size())
		kFlow.aiPromotionChoices[iPromotion]++;
	if (gGameRecordLogLevel >= 3)
	{
		logSASGameRecord("GAME_RECORD_ACTION turn=%d type=UNIT_PROMOTED player=%d unitId=%d unit=%s unitAI=%s promotion=%s x=%d y=%d xp=%d level=%d",
			GC.getGame().getGameTurn(), ePlayer, pUnit->getID(), getSASGameRecordUnitType(pUnit->getUnitType()),
			getSASGameRecordUnitAIType(pUnit->AI_getUnitAIType()), getSASGameRecordPromotionType(ePromotion),
			pUnit->getX(), pUnit->getY(), pUnit->getExperience(), pUnit->getLevel());
	}
}

// <!-- custom: Add the combat target supplied by CvUnit because a defeated attacker still occupies its origin at this callback.
// Deriving the target from pLoser made failed attacks wrong across coordinates, city-battle counters and dependent action rows. See KI#377.
// CvEventReporter owns the level-2 caller gate, so this helper intentionally does not repeat it before aggregate work. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordCombatResult(CvUnit const* pWinner, CvUnit const* pLoser, CvPlot const* pBattlePlot)
{
	if (pWinner == NULL || pLoser == NULL || pBattlePlot == NULL)
		return;
	bool const bLogExactBattle = (gGameRecordLogLevel >= 3);
	logSASGameRecordSettlerCombatIfNeeded(pWinner, pLoser, pBattlePlot);
	PlayerTypes eWinner = pWinner->getOwner();
	PlayerTypes eLoser = pLoser->getOwner();
	CvPlot const* pPlot = pBattlePlot;
	const bool bCityPlot = (pPlot != NULL && pPlot->isCity());
	SASGameRecordCombatPending kPending;
	bool const bPendingCombat = popSASGameRecordCombatPending(pWinner, pLoser, pPlot, kPending);
	if (bPendingCombat && kPending.bLuckEligible && kPending.iAttackerCombatOddsPermille >= 0)
	{
		bool const bAttackerWon = (pWinner->getOwner() == kPending.eAttacker && pWinner->getID() == kPending.iAttackerUnitId);
		recordSASGameRecordBattleLuck(kPending.eAttacker, kPending.iAttackerCombatOddsPermille, bAttackerWon);
		recordSASGameRecordBattleLuck(kPending.eDefender, 1000 - kPending.iAttackerCombatOddsPermille, !bAttackerWon);
	}
	// <!-- custom: Aggregate battle rows omit the Barbarian player, and Settler-defense rows cover only one special case.
	// At level 3, retain exact ordinary Barbarian/animal combat so spawned pressure can be followed through its actual outcome. (GPT-5.6-Sol) -->
	if (bLogExactBattle && (pWinner->getOwner() == BARBARIAN_PLAYER || pLoser->getOwner() == BARBARIAN_PLAYER))
	{
		logSASGameRecord("GAME_RECORD_ACTION turn=%d type=BARBARIAN_COMBAT winnerPlayer=%d winnerUnitId=%d winnerUnit=%s winnerAI=%s winnerDamage=%d loserPlayer=%d loserUnitId=%d loserUnit=%s loserAI=%s loserDamage=%d x=%d y=%d cityPlot=%d",
			GC.getGame().getGameTurn(), pWinner->getOwner(), pWinner->getID(), getSASGameRecordUnitType(pWinner->getUnitType()),
			getSASGameRecordUnitAIType(pWinner->AI_getUnitAIType()), pWinner->getDamage(), pLoser->getOwner(), pLoser->getID(),
			getSASGameRecordUnitType(pLoser->getUnitType()), getSASGameRecordUnitAIType(pLoser->AI_getUnitAIType()), pLoser->getDamage(),
			pPlot == NULL ? -1 : pPlot->getX(), pPlot == NULL ? -1 : pPlot->getY(), bCityPlot);
	}
	int const iLoserProductionNeeded = (eLoser >= 0 && eLoser < MAX_PLAYERS ? GET_PLAYER(eLoser).getProductionNeeded(pLoser->getUnitType()) : 0);
	if (eWinner >= 0 && eWinner < MAX_PLAYERS)
	{
		g_aiSASGameRecordBattleWins[eWinner]++;
		g_aiSASGameRecordTotalBattleWins[eWinner]++;
		SASGameRecordPlayerFlow& kWinnerFlow = g_akSASGameRecordPlayerFlow[eWinner];
		kWinnerFlow.iCombatWins++;
		if (eLoser >= 0 && eLoser < MAX_PLAYERS)
		{
			kWinnerFlow.iEnemyProductionNeededDestroyed += iLoserProductionNeeded;
			kWinnerFlow.iEnemyExperienceDestroyed += pLoser->getExperience();
			g_akSASGameRecordMilitaryQualityTotals[eWinner].iEnemyExperienceDestroyed += pLoser->getExperience();
		}
		if (bCityPlot)
		{
			g_aiSASGameRecordCityBattleWins[eWinner]++;
			g_aiSASGameRecordTotalCityBattleWins[eWinner]++;
			kWinnerFlow.iCityPlotWins++;
		}
	}
	if (eLoser >= 0 && eLoser < MAX_PLAYERS)
	{
		g_aiSASGameRecordBattleLosses[eLoser]++;
		g_aiSASGameRecordTotalBattleLosses[eLoser]++;
		SASGameRecordPlayerFlow& kLoserFlow = g_akSASGameRecordPlayerFlow[eLoser];
		kLoserFlow.iCombatLosses++;
		kLoserFlow.iOwnProductionNeededLost += iLoserProductionNeeded;
		kLoserFlow.iOwnExperienceLost += pLoser->getExperience();
		g_akSASGameRecordMilitaryQualityTotals[eLoser].iOwnExperienceLost += pLoser->getExperience();
		if (bCityPlot)
		{
			g_aiSASGameRecordCityBattleLosses[eLoser]++;
			g_aiSASGameRecordTotalCityBattleLosses[eLoser]++;
			kLoserFlow.iCityPlotLosses++;
		}
	}
	// <!-- custom: Keep battle aggregates scoped to the active war between the combatants' teams; this avoids charging third-party or Barbarian losses to another simultaneous war. (GPT-5.6-Sol) -->
	if (eWinner >= 0 && eWinner < MAX_PLAYERS && eLoser >= 0 && eLoser < MAX_PLAYERS)
	{
		TeamTypes const eWinnerTeam = GET_PLAYER(eWinner).getTeam();
		TeamTypes const eLoserTeam = GET_PLAYER(eLoser).getTeam();
		SASGameRecordWarSummary* pWar = findSASGameRecordWar(eWinnerTeam, eLoserTeam);
		if (pWar != NULL)
		{
			if (eWinnerTeam == pWar->eTeamA)
			{
				pWar->iUnitsDestroyedByA++;
				pWar->iProductionDestroyedByA += iLoserProductionNeeded;
				if (bCityPlot) pWar->iCityPlotWinsA++;
			}
			else
			{
				pWar->iUnitsDestroyedByB++;
				pWar->iProductionDestroyedByB += iLoserProductionNeeded;
				if (bCityPlot) pWar->iCityPlotWinsB++;
			}
			refreshSASGameRecordWarSuccess(*pWar);
		}
	}
	if (pLoser->getLeaderUnitType() != NO_UNIT)
	{
		logSASGameRecord("GAME_RECORD_ACTION turn=%d type=GREAT_GENERAL_UNIT_DIED player=%d unitId=%d unit=%s attachedGreatGeneral=%s winnerPlayer=%d winnerUnitId=%d winnerUnit=%s x=%d y=%d",
			GC.getGame().getGameTurn(), eLoser, pLoser->getID(), getSASGameRecordUnitType(pLoser->getUnitType()),
			getSASGameRecordUnitType(pLoser->getLeaderUnitType()), eWinner, pWinner->getID(),
			getSASGameRecordUnitType(pWinner->getUnitType()), pPlot->getX(), pPlot->getY());
	}
	logSASGameRecordGreatPersonDied(pLoser, eWinner, "COMBAT", pPlot);
	if (bLogExactBattle)
	{
		// <!-- custom: Include exact unit IDs so WAR_ATTACK_ORDER attacker selections can be joined to the resulting battle even when several units of the same type fight on the same turn.
		// The transient start context additionally preserves true attacker identity and pre-combat odds after visible-combat delay. (ChatGPT-5.6-Sol) -->
		int const iWinnerOddsPermille = (!bPendingCombat || kPending.iAttackerCombatOddsPermille < 0 ? -1 : (pWinner->getOwner() == kPending.eAttacker && pWinner->getID() == kPending.iAttackerUnitId ? kPending.iAttackerCombatOddsPermille : 1000 - kPending.iAttackerCombatOddsPermille));
		logSASGameRecord("GAME_RECORD_BATTLE turn=%d winner=%d loser=%d winnerUnit=%s winnerUnitId=%d loserUnit=%s loserUnitId=%d attacker=%d attackerUnitId=%d attackerCombatOddsPermille=%d winnerCombatOddsPermille=%d luckEligible=%d x=%d y=%d cityPlot=%d attackerFromX=%d attackerFromY=%d riverCrossingAttack=%d riverAttackPenalty=%d amphibiousAttack=%d amphibiousAttackPenalty=%d winnerBaseStr=%d loserBaseStr=%d winnerDamage=%d loserDamage=%d winnerXP=%d winnerLevel=%d loserXP=%d loserLevel=%d winnerLeaderUnit=%s loserLeaderUnit=%s",
			GC.getGame().getGameTurn(), eWinner, eLoser, getSASGameRecordUnitType(pWinner->getUnitType()), pWinner->getID(),
			getSASGameRecordUnitType(pLoser->getUnitType()), pLoser->getID(), bPendingCombat ? kPending.eAttacker : NO_PLAYER,
			bPendingCombat ? kPending.iAttackerUnitId : -1, bPendingCombat ? kPending.iAttackerCombatOddsPermille : -1, iWinnerOddsPermille,
			bPendingCombat && kPending.bLuckEligible, pPlot->getX(), pPlot->getY(), bCityPlot,
			bPendingCombat ? kPending.iAttackerFromX : -1, bPendingCombat ? kPending.iAttackerFromY : -1,
			bPendingCombat ? kPending.iRiverCrossingAttack : -1, bPendingCombat ? kPending.iRiverAttackPenalty : -1,
			bPendingCombat ? kPending.iAmphibiousAttack : -1, bPendingCombat ? kPending.iAmphibiousAttackPenalty : -1,
			pWinner->baseCombatStr(), pLoser->baseCombatStr(), pWinner->getDamage(), pLoser->getDamage(), pWinner->getExperience(), pWinner->getLevel(),
			pLoser->getExperience(), pLoser->getLevel(), getSASGameRecordUnitType(pWinner->getLeaderUnitType()),
			getSASGameRecordUnitType(pLoser->getLeaderUnitType()));
	}
}
