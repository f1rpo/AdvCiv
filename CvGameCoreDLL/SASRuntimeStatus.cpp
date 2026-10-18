// AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
// (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

#include "CvGameCoreDLL.h" // <!-- custom: Required first for this DLL's precompiled game/Win32 declarations. (GPT-6.1-Sol) -->
#include "CvGameCoreUtils.h" // <!-- custom: Declares typed status reasons plus the shared clocks and user-root services used by this publisher. (GPT-6.1-Sol) -->
// <!-- custom: Keep this publisher separate from the BBAI/SASGameRecord writer so its standalone I/O fixture has explicit gameplay dependencies. (GPT-6.1-Sol) -->
#include "CvGame.h" // <!-- custom: Game/autoplay/save-checkpoint getters require the complete CvGame type, not a forward declaration. (GPT-6.1-Sol) -->
#include "CvMap.h" // <!-- custom: Map dimension/world-size getters require the complete CvMap type. (GPT-6.1-Sol) -->

// <!-- custom: Finite process/game/session states use enums rather than heap-backed strings; the same local rows generate their immutable JSON names.
// Update-reason rows are shared with the public enum so callers cannot invent misspelled reasons; serialize names only when publishing JSON.
// Like building focus, each expansion generates one representation and then undefines its temporary macro to avoid leaking it into later code; this is compile-time work only. (GPT-6.1-Sol) -->
// <!-- custom: Both files come from this runtime recorder, so one implementation revision identifies their paired observations and refers readers to its revision history.
// Compile the label into the publisher: an edited XML value must not make an older loaded DLL report a newer implementation; this counter is independent of SASGameRecord and binary-save versions. (GPT-6.1-Sol) -->
enum SASRuntimeRecorderRevision
{
	SAS_RUNTIME_REVISION = 1
};

#define SAS_RUNTIME_PROCESS_STATES(DO) DO(RUNNING) DO(EXITED_CLEANLY)
enum SASRuntimeProcessState
{
#define SAS_RUNTIME_PROCESS_ENUM(NAME) SAS_RUNTIME_PROCESS_##NAME,
	SAS_RUNTIME_PROCESS_STATES(SAS_RUNTIME_PROCESS_ENUM)
#undef SAS_RUNTIME_PROCESS_ENUM
};
static char const* const aSASRuntimeProcessStateNames[] =
{
#define SAS_RUNTIME_PROCESS_NAME(NAME) #NAME,
	SAS_RUNTIME_PROCESS_STATES(SAS_RUNTIME_PROCESS_NAME)
#undef SAS_RUNTIME_PROCESS_NAME
};
#undef SAS_RUNTIME_PROCESS_STATES

#define SAS_RUNTIME_GAME_STATES(DO) DO(NO_GAME) DO(SESSION_STARTING) DO(PLAYING) DO(GAME_ENDED)
enum SASRuntimeGameState
{
#define SAS_RUNTIME_GAME_ENUM(NAME) SAS_RUNTIME_GAME_##NAME,
	SAS_RUNTIME_GAME_STATES(SAS_RUNTIME_GAME_ENUM)
#undef SAS_RUNTIME_GAME_ENUM
};
static char const* const aSASRuntimeGameStateNames[] =
{
#define SAS_RUNTIME_GAME_NAME(NAME) #NAME,
	SAS_RUNTIME_GAME_STATES(SAS_RUNTIME_GAME_NAME)
#undef SAS_RUNTIME_GAME_NAME
};
#undef SAS_RUNTIME_GAME_STATES

#define SAS_RUNTIME_SESSION_STATES(DO) DO(NONE) DO(NEW_GAME) DO(LOAD_GAME)
enum SASRuntimeSessionState
{
#define SAS_RUNTIME_SESSION_ENUM(NAME) SAS_RUNTIME_SESSION_##NAME,
	SAS_RUNTIME_SESSION_STATES(SAS_RUNTIME_SESSION_ENUM)
#undef SAS_RUNTIME_SESSION_ENUM
};
static char const* const aSASRuntimeSessionStateNames[] =
{
#define SAS_RUNTIME_SESSION_NAME(NAME) #NAME,
	SAS_RUNTIME_SESSION_STATES(SAS_RUNTIME_SESSION_NAME)
#undef SAS_RUNTIME_SESSION_NAME
};
#undef SAS_RUNTIME_SESSION_STATES

static char const* const aSASRuntimeStatusReasonNames[] =
{
#define SAS_RUNTIME_STATUS_REASON(NAME) #NAME,
// <!-- custom: Reuse the public enum's catalog at the serialization boundary; no per-event allocation or string search is needed. (GPT-6.1-Sol) -->
#include "SASRuntimeStatusReasons.h"
#undef SAS_RUNTIME_STATUS_REASON
};
BOOST_STATIC_ASSERT(sizeof(aSASRuntimeStatusReasonNames) / sizeof(aSASRuntimeStatusReasonNames[0]) == NUM_SAS_RUNTIME_STATUS_REASONS);

// <!-- custom: Activity is separate from game lifecycle: initialized play can be manual or autoplay, while creation and save loading need distinct timed intervals.
// Requests such as returning to the menu are observed boundaries, not claims that the EXE has displayed that menu. (GPT-6.1-Sol) -->
#define SAS_RUNTIME_ACTIVITIES(DO) DO(NONE) DO(NO_GAME) DO(CREATING_GAME) DO(LOADING_SAVE) DO(PLAYING) DO(AUTOPLAY) DO(GAME_ENDED) DO(RETURNING_TO_MENU) DO(EXIT_REQUESTED) DO(SHUTTING_DOWN)
enum SASRuntimeActivity
{
#define SAS_RUNTIME_ACTIVITY_ENUM(NAME) SAS_RUNTIME_ACTIVITY_##NAME,
	SAS_RUNTIME_ACTIVITIES(SAS_RUNTIME_ACTIVITY_ENUM)
#undef SAS_RUNTIME_ACTIVITY_ENUM
};
static char const* const aSASRuntimeActivityNames[] =
{
#define SAS_RUNTIME_ACTIVITY_NAME(NAME) #NAME,
	SAS_RUNTIME_ACTIVITIES(SAS_RUNTIME_ACTIVITY_NAME)
#undef SAS_RUNTIME_ACTIVITY_NAME
};
#undef SAS_RUNTIME_ACTIVITIES
static SASRuntimeActivity g_eSASRuntimeActivity = SAS_RUNTIME_ACTIVITY_NONE;
static SASRuntimeActivity g_eSASRuntimePreviousActivity = SAS_RUNTIME_ACTIVITY_NONE;
static CvString g_szSASRuntimeActivityStartedUtc = "-";
static CvString g_szSASRuntimePreviousStartedUtc = "-";
static CvString g_szSASRuntimePreviousEndedUtc = "-";
static uint g_uiSASRuntimeActivityStartedMilliseconds = 0;
static __int64 g_iSASRuntimePreviousDurationMilliseconds = -1;
static unsigned int g_uiSASRuntimeActivitySerial = 0;
static unsigned int g_uiSASRuntimeActivitySession = 0;
static int g_iSASRuntimeActivityStartTurn = -1;
static int g_iSASRuntimeActivityRequestedTurns = 0;
static SASAutoPlayEndCause g_eSASRuntimeLastAutoplayEndCause = SAS_AUTOPLAY_END_UNSPECIFIED;
static CvString g_szSASRuntimeHistoryFilename;
static std::wstring g_szSASRuntimeHistoryRoot;
static unsigned int g_uiSASRuntimeHistoryRowsWritten = 0;
static unsigned int g_uiSASRuntimeHistoryWriteFailures = 0;
static bool g_bSASRuntimeHistoryAtCurrentRoot = false;

// <!-- custom: Keep the external-agent runtime status independent from the structured game-record and AI-diagnostic enablement.
// It is default-off and transition-driven; the fixed JSON is current state, with completed activity intervals retained in a companion timeline.
// Session/load counters live for the DLL process so repeated new/load actions are distinguishable without retaining a growing history in memory. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
static int g_iSASRuntimeStatusEnabled = -1;
static unsigned int g_uiSASRuntimeStatusSerial = 0;
// <!-- custom: Recovered snapshots disclose failed earlier publications even to readers without an older statusSerial; status and history failures describe separate outputs. (ChatGPT (review) + GPT-6.1-Sol) -->
static unsigned int g_uiSASRuntimeStatusWriteFailures = 0;
static unsigned int g_uiSASRuntimeSessionSerial = 0;
static unsigned int g_uiSASRuntimeNewGameCount = 0;
static unsigned int g_uiSASRuntimeLoadCount = 0;
static int g_iSASRuntimeLastCompletedTurn = -1;
static SASRuntimeProcessState g_eSASRuntimeProcessState = SAS_RUNTIME_PROCESS_RUNNING;
static SASRuntimeGameState g_eSASRuntimeGameState = SAS_RUNTIME_GAME_NO_GAME;
static SASRuntimeSessionState g_eSASRuntimeSessionKind = SAS_RUNTIME_SESSION_NONE;
static CvString g_szSASRuntimeSessionStartedUtc = "-";
static CvString g_szSASRuntimeNativeReadyUtc = "-";
static std::wstring g_szSASRuntimeStatusRootDirectory;
static std::wstring g_szSASRuntimeStatusSupersededPath;
static bool g_bSASRuntimeStatusRootResolved = false;
// <!-- custom: Track the last successfully published root, not merely the last attempted root; several failed handoffs must retain the actual fallback copy until a replacement succeeds. (GPT-6.1-Sol) -->
static std::wstring g_szSASRuntimeStatusLastPublishedRoot;

// <!-- custom: Read XML once on first use, then reuse the setting until restart: -1 means unread, 0 means cached off, and 1 means cached on.
// Shutdown checks the stored value directly; if it was never read, skip status publication rather than performing a first XML lookup after XML caches may have been released. (GPT-6.1-Sol) -->
bool isSASRuntimeStatusEnabled()
{
	if (g_iSASRuntimeStatusEnabled < 0)
		g_iSASRuntimeStatusEnabled = (GC.getDefineINT("SAS_RUNTIME_STATUS_ENABLE") > 0 ? 1 : 0);
	return g_iSASRuntimeStatusEnabled > 0;
}

static bool writeSASRuntimeStatus(SASRuntimeStatusReason eReason);

static bool resolveSASRuntimeStatusDefaultRootDirectory()
{
	if (g_bSASRuntimeStatusRootResolved)
		return !g_szSASRuntimeStatusRootDirectory.empty();
	if (!getSASNativeDefaultRootDirectory(g_szSASRuntimeStatusRootDirectory))
		return false;
	g_bSASRuntimeStatusRootResolved = true;
	return true;
}

// <!-- custom: A case-only Windows root handoff previously wrote and then deleted the same JSON through its old spelling; the native fixture reproduced the missing file.
// Compare normalized slash spellings with Windows Unicode case folding so an equivalent path is never treated as obsolete. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
static bool isSASRuntimeStatusSamePath(std::wstring szLeft, std::wstring szRight)
{
	for (size_t i = 0; i < szLeft.size(); i++)
	{
		if (szLeft[i] == L'/')
			szLeft[i] = L'\\';
	}
	for (size_t i = 0; i < szRight.size(); i++)
	{
		if (szRight[i] == L'/')
			szRight[i] = L'\\';
	}
	return CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, szLeft.c_str(), -1, szRight.c_str(), -1) == CSTR_EQUAL;
}

// <!-- custom: Accept BUG's already-trimmed Unicode root independently of historical log handles; status is replaceable current state, not append history. (GPT-6.1-Sol) -->
void setSASRuntimeStatusRootDirectory(std::wstring const& szRoot)
{
	if (!isSASRuntimeStatusEnabled())
		return;
	// <!-- custom: An enabled session transition can publish status through the native default root before BUG supplies CvAltRoot.
	// Preserve that last good fallback copy until an authoritative-root rewrite succeeds; if a live reader blocks deletion, later successful updates retry cleanup. (ChatGPT-5.6-Sol) -->
	if (!g_szSASRuntimeStatusLastPublishedRoot.empty() && !isSASRuntimeStatusSamePath(g_szSASRuntimeStatusLastPublishedRoot, szRoot))
		g_szSASRuntimeStatusSupersededPath = g_szSASRuntimeStatusLastPublishedRoot + L"\\Logs\\SASRuntimeStatus.json";
	g_szSASRuntimeStatusRootDirectory = szRoot;
	g_bSASRuntimeStatusRootResolved = true;
	writeSASRuntimeStatus(SAS_RUNTIME_STATUS_ROOT_READY);
}

// <!-- custom: Cache each whitespace representation once for enabled output; the shared binary context already caches module discovery/hashing, so neither formatting style repeats that I/O. (GPT-6.1-Sol) -->
static CvString const& getSASRuntimeDllIdentity(bool bIndented = false)
{
	if (bIndented)
	{
		static CvString const szIndentedIdentity = getSASDllRuntimeIdentityJson(true);
		return szIndentedIdentity;
	}
	static CvString const szCompactIdentity = getSASDllRuntimeIdentityJson(false);
	return szCompactIdentity;
}

// <!-- custom: Keep the timing history append-only and independent from optional SASGameRecord logging; retaining only the latest snapshot lost earlier new/load/autoplay intervals.
// Each transition stores the completed interval and the newly entered activity, so idle periods survive without a frame timer or an in-memory history that grows throughout the process.
// State durations use the shared monotonic clock, not UTC subtraction; UTC still locates intervals in external evidence.
// A loaded-session row retains known serialization checkpoints for later matching; these do not identify the current filesystem name after a save was renamed or copied. (GPT-6.1-Sol) -->
static bool prepareSASRuntimeHistory(std::wstring const& szRoot)
{
	if (g_szSASRuntimeHistoryFilename.empty())
		g_szSASRuntimeHistoryFilename.Format("SASRuntimeHistory_%s_%lu.jsonl", getSASExecutableProcessStartUtc().GetCString(), (unsigned long)GetCurrentProcessId());
	std::wstring const szName(g_szSASRuntimeHistoryFilename.begin(), g_szSASRuntimeHistoryFilename.end());
	if (g_uiSASRuntimeHistoryRowsWritten != 0 && !g_szSASRuntimeHistoryRoot.empty() && !isSASRuntimeStatusSamePath(g_szSASRuntimeHistoryRoot, szRoot))
	{
		// <!-- custom: Copy the small timeline when BUG changes the root; never delete the old evidence, and continue there if migration fails rather than silently splitting the history. (GPT-6.1-Sol) -->
		std::wstring const szOldPath = g_szSASRuntimeHistoryRoot + L"\\Logs\\" + szName;
		std::wstring const szNewPath = szRoot + L"\\Logs\\" + szName;
		if (CopyFileW(szOldPath.c_str(), szNewPath.c_str(), FALSE))
			g_szSASRuntimeHistoryRoot = szRoot;
	}
	else g_szSASRuntimeHistoryRoot = szRoot;
	g_bSASRuntimeHistoryAtCurrentRoot = isSASRuntimeStatusSamePath(g_szSASRuntimeHistoryRoot, szRoot);
	return !g_szSASRuntimeHistoryRoot.empty();
}

static bool appendSASRuntimeHistory(CvString const& szRow)
{
	if (g_szSASRuntimeHistoryRoot.empty())
		return false;
	std::wstring const szName(g_szSASRuntimeHistoryFilename.begin(), g_szSASRuntimeHistoryFilename.end());
	std::wstring const szPath = g_szSASRuntimeHistoryRoot + L"\\Logs\\" + szName;
	HANDLE const hFile = CreateFileW(szPath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	LARGE_INTEGER kZero, kStart;
	kZero.QuadPart = 0;
	bool bComplete = SetFilePointerEx(hFile, kZero, &kStart, FILE_END) != FALSE;
	size_t iWrittenTotal = 0;
	while (bComplete && iWrittenTotal < szRow.size())
	{
		DWORD iWritten = 0;
		BOOL const bWritten = WriteFile(hFile, szRow.GetCString() + iWrittenTotal, (DWORD)(szRow.size() - iWrittenTotal), &iWritten, NULL);
		iWrittenTotal += iWritten;
		bComplete = (bWritten != FALSE && iWritten != 0);
	}
	// <!-- custom: A failed partial append must not join a torn JSON row to the next transition; roll back this row while retaining earlier complete intervals. (GPT-6.1-Sol) -->
	if (!bComplete && iWrittenTotal != 0)
	{
		if (SetFilePointerEx(hFile, kStart, NULL, FILE_BEGIN))
			SetEndOfFile(hFile);
	}
	BOOL const bClosed = CloseHandle(hFile);
	return bComplete && bClosed != FALSE;
}

static SASRuntimeActivity getSASRuntimeActivity(SASRuntimeStatusReason eReason, int iAutoPlayRemaining)
{
	if (g_eSASRuntimeProcessState == SAS_RUNTIME_PROCESS_EXITED_CLEANLY)
		return SAS_RUNTIME_ACTIVITY_SHUTTING_DOWN;
	if (eReason == SAS_RUNTIME_STATUS_RETURN_TO_MENU_REQUESTED)
		return SAS_RUNTIME_ACTIVITY_RETURNING_TO_MENU;
	if (eReason == SAS_RUNTIME_STATUS_EXIT_REQUESTED)
		return SAS_RUNTIME_ACTIVITY_EXIT_REQUESTED;
	if (g_eSASRuntimeGameState == SAS_RUNTIME_GAME_NO_GAME)
		return SAS_RUNTIME_ACTIVITY_NO_GAME;
	if (g_eSASRuntimeGameState == SAS_RUNTIME_GAME_SESSION_STARTING)
		return (g_eSASRuntimeSessionKind == SAS_RUNTIME_SESSION_NEW_GAME ? SAS_RUNTIME_ACTIVITY_CREATING_GAME : SAS_RUNTIME_ACTIVITY_LOADING_SAVE);
	if (g_eSASRuntimeGameState == SAS_RUNTIME_GAME_GAME_ENDED)
		return SAS_RUNTIME_ACTIVITY_GAME_ENDED;
	// <!-- custom: A completed-turn callback can arrive after an exit request; preserve the pending request until a reset/new session or shutdown actually changes lifecycle. (GPT-6.1-Sol) -->
	if (g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_RETURNING_TO_MENU || g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_EXIT_REQUESTED)
		return g_eSASRuntimeActivity;
	return (iAutoPlayRemaining > 0 ? SAS_RUNTIME_ACTIVITY_AUTOPLAY : SAS_RUNTIME_ACTIVITY_PLAYING);
}

static void updateSASRuntimeActivity(SASRuntimeStatusReason eReason, CvString const& szNowUtc, uint uiNowMilliseconds, int iGameTurn, int iAutoPlayRemaining, int iLoadedCheckpointTurn, int iLoadedCheckpointElapsedTurn, char const* szLoadedCheckpointUtc)
{
	SASRuntimeActivity const eActivity = getSASRuntimeActivity(eReason, iAutoPlayRemaining);
	if (eActivity == g_eSASRuntimeActivity && eReason != SAS_RUNTIME_STATUS_AUTOPLAY_CHANGED && g_uiSASRuntimeActivitySession == g_uiSASRuntimeSessionSerial)
		return;
	g_eSASRuntimePreviousActivity = g_eSASRuntimeActivity;
	g_szSASRuntimePreviousStartedUtc = g_szSASRuntimeActivityStartedUtc;
	g_szSASRuntimePreviousEndedUtc = (g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_NONE ? CvString("-") : szNowUtc);
	g_iSASRuntimePreviousDurationMilliseconds = (g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_NONE ? -1 : (__int64)getSASElapsedMilliseconds(g_uiSASRuntimeActivityStartedMilliseconds, uiNowMilliseconds));
	CvString szRow;
	CvString const& szDllIdentity = getSASRuntimeDllIdentity();
	// <!-- custom: Keep one JSONL record on each output line; adjacent C++ literals let the source show one field per line without inserting output newlines. (GPT-6.1-Sol) -->
	szRow.Format("{\"revision\":%d,"
		"\"stateSerial\":%u,"
		"\"processId\":%lu,"
		"\"processStartUtc\":\"%s\","
		"\"dllIdentity\":%s,"
		"\"previousState\":\"%s\","
		"\"previousStartedUtc\":\"%s\","
		"\"previousEndedUtc\":\"%s\","
		"\"previousDurationMilliseconds\":%I64d,"
		"\"previousSessionSerial\":%u,"
		"\"previousStartTurn\":%d,"
		"\"previousRequestedAutoplayTurns\":%d,"
		"\"recordedUtc\":\"%s\","
		"\"state\":\"%s\","
		"\"sessionSerial\":%u,"
		"\"gameTurn\":%d,"
		"\"autoplayRemainingTurns\":%d,"
		"\"reason\":\"%s\","
		"\"autoplayEndCause\":\"%s\","
		"\"loadedSaveCheckpointTurn\":%d,"
		"\"loadedSaveCheckpointElapsedTurn\":%d,"
		"\"loadedSaveCheckpointUtc\":\"%s\"}\r\n",
		SAS_RUNTIME_REVISION, ++g_uiSASRuntimeActivitySerial, (unsigned long)GetCurrentProcessId(), getSASExecutableProcessStartUtc().GetCString(), szDllIdentity.GetCString(),
		aSASRuntimeActivityNames[g_eSASRuntimePreviousActivity], g_szSASRuntimePreviousStartedUtc.GetCString(), g_szSASRuntimePreviousEndedUtc.GetCString(), g_iSASRuntimePreviousDurationMilliseconds,
		g_uiSASRuntimeActivitySession, g_iSASRuntimeActivityStartTurn, g_iSASRuntimeActivityRequestedTurns, szNowUtc.GetCString(), aSASRuntimeActivityNames[eActivity], g_uiSASRuntimeSessionSerial, iGameTurn, iAutoPlayRemaining,
		aSASRuntimeStatusReasonNames[eReason], (eReason == SAS_RUNTIME_STATUS_AUTOPLAY_ENDED ? getSASAutoPlayEndCause(g_eSASRuntimeLastAutoplayEndCause) : "-"), iLoadedCheckpointTurn, iLoadedCheckpointElapsedTurn, szLoadedCheckpointUtc);
	if (appendSASRuntimeHistory(szRow))
		g_uiSASRuntimeHistoryRowsWritten++;
	else g_uiSASRuntimeHistoryWriteFailures++;
	g_eSASRuntimeActivity = eActivity;
	g_szSASRuntimeActivityStartedUtc = szNowUtc;
	g_uiSASRuntimeActivityStartedMilliseconds = uiNowMilliseconds;
	g_uiSASRuntimeActivitySession = g_uiSASRuntimeSessionSerial;
	g_iSASRuntimeActivityStartTurn = iGameTurn;
	g_iSASRuntimeActivityRequestedTurns = iAutoPlayRemaining;
}

// <!-- custom: Write one small current-state JSON file for external agents.
// This snapshot stays overwrite-only; the companion JSONL retains timed activity transitions without rewriting an ever-growing history each turn.
// Readers may momentarily catch the short CREATE_ALWAYS rewrite and should retry malformed JSON; avoiding rename/delete semantics keeps ordinary readers from blocking updates.
// Its fixed name is a discovery endpoint; the timestamped runtime timeline retains coarse state intervals, while SASGameRecord owns detailed game/action evidence.
// The dedicated enable flag controls direct writes independently of LoggingEnabled/MessageLog/RandLog/SynchLog; OverwriteLogs does not apply to a current-state snapshot.
// No FlushFileBuffers is requested. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
static bool publishSASRuntimeStatus(SASRuntimeStatusReason eReason)
{
	if (!isSASRuntimeStatusEnabled())
		return true;
	FAssert(eReason >= 0 && eReason < NUM_SAS_RUNTIME_STATUS_REASONS);
	if (eReason < 0 || eReason >= NUM_SAS_RUNTIME_STATUS_REASONS)
		return false;
	// <!-- custom: Sample the observed boundary before root discovery or file I/O, so slow publication does not redefine when creation/loading or autoplay changed state. (GPT-6.1-Sol) -->
	CvString const szUpdatedUtc = createSASUtcTimestampMilliseconds();
	uint const uiNowMilliseconds = getSASMonotonicMilliseconds();
	bool const bRootReady = resolveSASRuntimeStatusDefaultRootDirectory();
	std::wstring const szLogDirectory = g_szSASRuntimeStatusRootDirectory + L"\\Logs";
	bool const bDirectoryReady = bRootReady && (CreateDirectoryW(szLogDirectory.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS);
	std::wstring const szPath = szLogDirectory + L"\\SASRuntimeStatus.json";

	CvGame const& kGame = GC.getGame();
	// <!-- custom: During load/reset, final-initialized state can belong to an earlier lifecycle boundary; SESSION_STARTING/NO_GAME must not publish ready-game fields. (GPT-6.1-Sol) -->
	bool const bGameLoaded = (g_eSASRuntimeGameState != SAS_RUNTIME_GAME_NO_GAME && g_eSASRuntimeGameState != SAS_RUNTIME_GAME_SESSION_STARTING && kGame.isFinalInitialized());
	int const iGameTurn = (bGameLoaded ? kGame.getGameTurn() : -1);
	int const iElapsedGameTurns = (bGameLoaded ? kGame.getElapsedGameTurns() : -1);
	int const iActivePlayer = (bGameLoaded ? kGame.getActivePlayer() : -1);
	int const iAutoPlayRemaining = (bGameLoaded ? std::max(0, kGame.getAIAutoPlay()) : 0);
	int const iMapWidth = (bGameLoaded ? GC.getMap().getGridWidth() : -1);
	int const iMapHeight = (bGameLoaded ? GC.getMap().getGridHeight() : -1);
	int const iWorldSize = (bGameLoaded ? (int)GC.getMap().getWorldSize() : -1);
	int const iGameSpeed = (bGameLoaded ? (int)kGame.getGameSpeedType() : -1);
	int const iScenario = (bGameLoaded ? (int)kGame.isScenario() : 0);
	int const iLoadedCheckpointTurn = (bGameLoaded && g_eSASRuntimeSessionKind == SAS_RUNTIME_SESSION_LOAD_GAME ? kGame.getSASLoadedSaveCheckpointTurn() : -1);
	int const iLoadedCheckpointElapsedTurn = (bGameLoaded && g_eSASRuntimeSessionKind == SAS_RUNTIME_SESSION_LOAD_GAME ? kGame.getSASLoadedSaveCheckpointElapsedTurn() : -1);
	char const* szLoadedCheckpointUtc = "-";
	if (bGameLoaded && g_eSASRuntimeSessionKind == SAS_RUNTIME_SESSION_LOAD_GAME && kGame.getSASLoadedSaveCheckpointUtc() != NULL && kGame.getSASLoadedSaveCheckpointUtc()[0] != '\0')
		szLoadedCheckpointUtc = kGame.getSASLoadedSaveCheckpointUtc();
	// <!-- custom: Failed output must not postpone the observed transition or inflate the next interval; retain timing in memory and disclose unsuccessful history appends when publication resumes. (GPT-6.1-Sol) -->
	if (bDirectoryReady) prepareSASRuntimeHistory(g_szSASRuntimeStatusRootDirectory);
	else g_bSASRuntimeHistoryAtCurrentRoot = false;
	updateSASRuntimeActivity(eReason, szUpdatedUtc, uiNowMilliseconds, iGameTurn, iAutoPlayRemaining, iLoadedCheckpointTurn, iLoadedCheckpointElapsedTurn, szLoadedCheckpointUtc);
	if (!bDirectoryReady)
		return false;
	g_uiSASRuntimeStatusSerial++;
	// <!-- custom: Cache DLL identity only after the status gate; disabled runs do no provenance work, and enabled turn updates never rehash the binary or rescan build outputs. (GPT-6.1-Sol) -->
	CvString const& szDllIdentity = getSASRuntimeDllIdentity(true);
	CvString szJson;
	// <!-- custom: Indent the current-state snapshot for direct inspection as well as agent parsing; whitespace does not change its JSON fields. (GPT-6.1-Sol) -->
	szJson.Format("{\r\n"
		"  \"revision\": %d,\r\n"
		"  \"dllIdentity\": %s,\r\n"
		"  \"statusSerial\": %u,\r\n"
		"  \"statusWriteFailures\": %u,\r\n"
		"  \"processId\": %lu,\r\n"
		"  \"processStartUtc\": \"%s\",\r\n"
		"  \"dllLoadUtc\": \"%s\",\r\n"
		"  \"updatedUtc\": \"%s\",\r\n"
		"  \"processState\": \"%s\",\r\n"
		"  \"gameState\": \"%s\",\r\n"
		"  \"activity\": \"%s\",\r\n"
		"  \"activitySerial\": %u,\r\n"
		"  \"activityStartedUtc\": \"%s\",\r\n"
		"  \"activityElapsedMilliseconds\": %u,\r\n"
		"  \"previousActivity\": \"%s\",\r\n"
		"  \"previousActivityStartedUtc\": \"%s\",\r\n"
		"  \"previousActivityEndedUtc\": \"%s\",\r\n"
		"  \"previousActivityDurationMilliseconds\": %I64d,\r\n"
		"  \"historyFilename\": \"%s\",\r\n"
		"  \"historyAtCurrentRoot\": %d,\r\n"
		"  \"historyRowsWritten\": %u,\r\n"
		"  \"historyWriteFailures\": %u,\r\n"
		"  \"updateReason\": \"%s\",\r\n"
		"  \"sessionSerial\": %u,\r\n"
		"  \"newGameCount\": %u,\r\n"
		"  \"loadCount\": %u,\r\n"
		"  \"sessionKind\": \"%s\",\r\n"
		"  \"sessionStartedUtc\": \"%s\",\r\n"
		"  \"nativeReadyUtc\": \"%s\",\r\n"
		"  \"gameTurn\": %d,\r\n"
		"  \"elapsedGameTurns\": %d,\r\n"
		"  \"lastCompletedTurn\": %d,\r\n"
		"  \"activePlayer\": %d,\r\n"
		"  \"autoplayActive\": %d,\r\n"
		"  \"autoplayRemainingTurns\": %d,\r\n"
		"  \"mapWidth\": %d,\r\n"
		"  \"mapHeight\": %d,\r\n"
		"  \"worldSize\": %d,\r\n"
		"  \"gameSpeed\": %d,\r\n"
		"  \"scenario\": %d,\r\n"
		"  \"loadedSaveCheckpointTurn\": %d,\r\n"
		"  \"loadedSaveCheckpointElapsedTurn\": %d,\r\n"
		"  \"loadedSaveCheckpointUtc\": \"%s\"\r\n"
		"}\r\n",
		SAS_RUNTIME_REVISION, szDllIdentity.GetCString(), g_uiSASRuntimeStatusSerial, g_uiSASRuntimeStatusWriteFailures, (unsigned long)GetCurrentProcessId(), getSASExecutableProcessStartUtc().GetCString(), getSASProcessUtcTimestamp().GetCString(), szUpdatedUtc.GetCString(),
		aSASRuntimeProcessStateNames[g_eSASRuntimeProcessState], aSASRuntimeGameStateNames[g_eSASRuntimeGameState],
		aSASRuntimeActivityNames[g_eSASRuntimeActivity], g_uiSASRuntimeActivitySerial, g_szSASRuntimeActivityStartedUtc.GetCString(), getSASElapsedMilliseconds(g_uiSASRuntimeActivityStartedMilliseconds, uiNowMilliseconds),
		aSASRuntimeActivityNames[g_eSASRuntimePreviousActivity], g_szSASRuntimePreviousStartedUtc.GetCString(), g_szSASRuntimePreviousEndedUtc.GetCString(), g_iSASRuntimePreviousDurationMilliseconds,
		g_szSASRuntimeHistoryFilename.GetCString(), g_bSASRuntimeHistoryAtCurrentRoot, g_uiSASRuntimeHistoryRowsWritten, g_uiSASRuntimeHistoryWriteFailures,
		aSASRuntimeStatusReasonNames[eReason], g_uiSASRuntimeSessionSerial, g_uiSASRuntimeNewGameCount, g_uiSASRuntimeLoadCount,
		aSASRuntimeSessionStateNames[g_eSASRuntimeSessionKind], g_szSASRuntimeSessionStartedUtc.GetCString(), g_szSASRuntimeNativeReadyUtc.GetCString(), iGameTurn, iElapsedGameTurns, g_iSASRuntimeLastCompletedTurn, iActivePlayer,
		(iAutoPlayRemaining > 0 ? 1 : 0), iAutoPlayRemaining, iMapWidth, iMapHeight, iWorldSize, iGameSpeed, iScenario,
		iLoadedCheckpointTurn, iLoadedCheckpointElapsedTurn, szLoadedCheckpointUtc);

	HANDLE const hFile = CreateFileW(szPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	size_t iWrittenTotal = 0;
	bool bWriteSucceeded = true;
	while (iWrittenTotal < szJson.size())
	{
		DWORD iWritten = 0;
		DWORD const iChunk = (DWORD)std::min(szJson.size() - iWrittenTotal, (size_t)0xffffffff);
		BOOL const bWritten = WriteFile(hFile, szJson.GetCString() + iWrittenTotal, iChunk, &iWritten, NULL);
		iWrittenTotal += iWritten;
		if (!bWritten || iWritten == 0)
		{
			bWriteSucceeded = false;
			break;
		}
	}
	// <!-- custom: A reported failed write can acknowledge the entire byte count; require successful write/close results before retiring the fallback copy. (GPT-6.1-Sol) -->
	BOOL const bClosed = CloseHandle(hFile);
	bool const bComplete = (bWriteSucceeded && iWrittenTotal == szJson.size() && bClosed != FALSE);
	if (bComplete)
	{
		g_szSASRuntimeStatusLastPublishedRoot = g_szSASRuntimeStatusRootDirectory;
		if (!g_szSASRuntimeStatusSupersededPath.empty())
		{
			if (isSASRuntimeStatusSamePath(g_szSASRuntimeStatusSupersededPath, szPath) || DeleteFileW(g_szSASRuntimeStatusSupersededPath.c_str()) || GetLastError() == ERROR_FILE_NOT_FOUND)
				g_szSASRuntimeStatusSupersededPath.clear();
		}
	}
	return bComplete;
}

// <!-- custom: Count every failed publication once, including root/open/write/close failures; the next successful snapshot reports the cumulative count without retaining a failure history. (ChatGPT (review) + GPT-6.1-Sol) -->
static bool writeSASRuntimeStatus(SASRuntimeStatusReason eReason)
{
	bool const bPublished = publishSASRuntimeStatus(eReason);
	if (!bPublished)
		g_uiSASRuntimeStatusWriteFailures++;
	return bPublished;
}

// <!-- custom: Reset always publishes GAME_RESET; removing the free-form label keeps the fixed event catalog authoritative. (GPT-6.1-Sol) -->
void updateSASRuntimeStatusNoGame()
{
	if (!isSASRuntimeStatusEnabled())
		return;
	g_eSASRuntimeGameState = SAS_RUNTIME_GAME_NO_GAME;
	g_eSASRuntimeSessionKind = SAS_RUNTIME_SESSION_NONE;
	g_szSASRuntimeSessionStartedUtc = "-";
	g_szSASRuntimeNativeReadyUtc = "-";
	g_iSASRuntimeLastCompletedTurn = -1;
	writeSASRuntimeStatus(SAS_RUNTIME_STATUS_GAME_RESET);
}

void updateSASRuntimeStatusSessionStarted(bool bLoadedSave)
{
	if (!isSASRuntimeStatusEnabled())
		return;
	g_uiSASRuntimeSessionSerial++;
	if (bLoadedSave)
	{
		g_uiSASRuntimeLoadCount++;
		g_eSASRuntimeSessionKind = SAS_RUNTIME_SESSION_LOAD_GAME;
	}
	else
	{
		g_uiSASRuntimeNewGameCount++;
		g_eSASRuntimeSessionKind = SAS_RUNTIME_SESSION_NEW_GAME;
	}
	g_eSASRuntimeGameState = SAS_RUNTIME_GAME_SESSION_STARTING;
	g_szSASRuntimeSessionStartedUtc = createSASUtcTimestampMilliseconds();
	g_szSASRuntimeNativeReadyUtc = "-";
	g_iSASRuntimeLastCompletedTurn = -1;
	writeSASRuntimeStatus(bLoadedSave ? SAS_RUNTIME_STATUS_LOAD_GAME_STARTED : SAS_RUNTIME_STATUS_NEW_GAME_STARTED);
}

void updateSASRuntimeStatusSessionReady()
{
	if (!isSASRuntimeStatusEnabled())
		return;
	g_eSASRuntimeGameState = SAS_RUNTIME_GAME_PLAYING;
	g_szSASRuntimeNativeReadyUtc = createSASUtcTimestampMilliseconds();
	writeSASRuntimeStatus(g_eSASRuntimeSessionKind == SAS_RUNTIME_SESSION_LOAD_GAME ? SAS_RUNTIME_STATUS_LOAD_GAME_NATIVE_READY : SAS_RUNTIME_STATUS_NEW_GAME_NATIVE_READY);
}

void updateSASRuntimeStatusTurnCompleted(int iCompletedTurn)
{
	if (!isSASRuntimeStatusEnabled())
		return;
	g_iSASRuntimeLastCompletedTurn = iCompletedTurn;
	writeSASRuntimeStatus(SAS_RUNTIME_STATUS_TURN_COMPLETED);
}

// <!-- custom: Accept the caller's typed start/end/adjustment reason and existing completion-cause enum, so history distinguishes interruption from scheduled completion without parsing SASGameRecord. (GPT-6.1-Sol) -->
void updateSASRuntimeStatusAutoPlay(SASRuntimeStatusReason eReason, SASAutoPlayEndCause eEndCause)
{
	if (!isSASRuntimeStatusEnabled())
		return;
	g_eSASRuntimeLastAutoplayEndCause = eEndCause;
	writeSASRuntimeStatus(eReason);
}

// <!-- custom: Preserve explicit UI navigation/exit requests separately from observed reset/shutdown completion. (GPT-6.1-Sol) -->
void updateSASRuntimeStatusRequest(SASRuntimeStatusReason eReason)
{
	if (!isSASRuntimeStatusEnabled())
		return;
	writeSASRuntimeStatus(eReason);
}

void updateSASRuntimeStatusGameEnded()
{
	if (!isSASRuntimeStatusEnabled())
		return;
	g_eSASRuntimeGameState = SAS_RUNTIME_GAME_GAME_ENDED;
	writeSASRuntimeStatus(SAS_RUNTIME_STATUS_GAME_ENDED);
}

void updateSASRuntimeStatusCleanExit()
{
	// <!-- custom: Do not resolve/query XML for the first time during global teardown; any enabled status session initialized this cache earlier. (ChatGPT-5.6-Sol) -->
	if (g_iSASRuntimeStatusEnabled <= 0)
		return;
	g_eSASRuntimeProcessState = SAS_RUNTIME_PROCESS_EXITED_CLEANLY;
	writeSASRuntimeStatus(SAS_RUNTIME_STATUS_PROCESS_EXIT);
}
