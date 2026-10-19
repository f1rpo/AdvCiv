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
	SAS_RUNTIME_REVISION = 4
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
#define SAS_RUNTIME_ACTIVITIES(DO) DO(NONE) DO(NO_GAME) DO(CREATING_GAME) DO(LOADING_SAVE) DO(PLAYING) DO(AUTOPLAY) DO(WORLD_BUILDER) DO(GAME_ENDED) DO(RETURNING_TO_MENU) DO(EXIT_REQUESTED) DO(SHUTTING_DOWN)
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
// <!-- custom: stateSerial orders transition observations only; run/session metadata contributes to confirmed historyRowsWritten without consuming a transition serial. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
static unsigned int g_uiSASRuntimeActivitySerial = 0;
static unsigned int g_uiSASRuntimeActivitySession = 0;
static int g_iSASRuntimeActivityStartTurn = -1;
static int g_iSASRuntimeActivityRequestedTurns = 0;
static SASAutoPlayEndCause g_eSASRuntimeLastAutoplayEndCause = SAS_AUTOPLAY_END_UNSPECIFIED;
static CvString g_szSASRuntimeHistoryFilename;
static CvString g_szSASRuntimeHistoryRunId;
static std::wstring g_szSASRuntimeHistoryRoot;
static unsigned int g_uiSASRuntimeHistoryRowsWritten = 0;
static unsigned int g_uiSASRuntimeHistoryWriteFailures = 0;
// <!-- custom: Retain one bounded ready-session payload for retry, not an in-memory history.
// A failed publication at turn 9/player 0 was retried with turn 10/player 1 mislabeled as the ready state; capture before I/O and retain that observation while failure counters remain live.
// A later ready session supersedes any still-unpublished context; this best-effort archive does not queue failed sessions indefinitely. (GPT-6.1-Sol) -->
static unsigned int g_uiSASRuntimeHistorySessionMetadataCaptured = 0;
static CvString g_szSASRuntimeHistoryPendingSessionMetadata;
static bool g_bSASRuntimeHistoryMetadataWritten = false;
static bool g_bSASRuntimeHistoryMayContainRows = false;
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
// <!-- custom: These are passive presentation observations, not claims that an EXE-rendered frame reached the user; exported info getters and Python screen callbacks provide event-driven boundaries without frame polling. (ChatGPT-5.6-Sol) -->
static CvString g_szSASRuntimeMainMenuSceneRequestedUtc = "-";
static CvString g_szSASRuntimeLoadingScreenAssetRequestedUtc = "-";
static bool g_bSASRuntimeWorldBuilderMode = false;
static bool g_bSASRuntimeWorldBuilderScreenVisible = false;
static std::wstring g_szSASRuntimeStatusRootDirectory;
static std::wstring g_szSASRuntimeStatusSupersededPath;
static bool g_bSASRuntimeStatusRootResolved = false;
// <!-- custom: Track the last successfully published root, not merely the last attempted root; several failed handoffs must retain the actual fallback copy until a replacement succeeds. (GPT-6.1-Sol) -->
static std::wstring g_szSASRuntimeStatusLastPublishedRoot;
// <!-- custom: Observe Windows close messages without replacing Civ4's WndProc: thread-specific hooks watch queue removal and sent-message delivery, always chaining onward.
// Keep the hook entirely inside the default-off runtime recorder; normal gameplay with status disabled never enumerates windows or installs a hook.
// The observation identifies a Windows close message/command, not whether it came from the title-bar X, Alt+F4, automation, or another sender. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
static HHOOK g_hSASRuntimeWindowMessageHook = NULL;
// <!-- custom: Window-procedure delivery alone missed the first real autoplay/red-cross test although the observer was armed.
// Observe queue removal too: an EXE can handle a retrieved message without dispatching it to its WndProc; both hooks remain read-only and thread-specific. (GPT-6.1-Sol) -->
static HHOOK g_hSASRuntimeWindowQueueHook = NULL;
static DWORD g_uiSASRuntimeWindowHookThreadId = 0;
static HWND g_hSASRuntimeObservedWindow = NULL;
static bool g_bSASRuntimeHandlingWindowClose = false;
static bool g_bSASRuntimeWindowCloseObservationArmed = false;
// <!-- custom: Both WM_CLOSE and SC_CLOSE observations update this request timestamp; the field names the shared meaning rather than suggesting only WM_CLOSE is covered. (GPT-6.1-Sol) -->
static CvString g_szSASRuntimeWindowCloseRequestObservedUtc = "-";

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

// <!-- custom: Locate Civ4's visible, unowned top-level window only during enabled lifecycle observations.
// Queue and sent-message hooks observe its thread without subclassing, consuming, or altering the EXE's messages; when no window is visible yet, arm on the lifecycle caller's thread. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
struct SASRuntimeProcessWindow
{
	SASRuntimeProcessWindow() : hWindow(NULL), uiThreadId(0) {}
	HWND hWindow;
	DWORD uiThreadId;
};

static BOOL CALLBACK findSASRuntimeProcessWindow(HWND hWindow, LPARAM lParam)
{
	DWORD uiProcessId = 0;
	DWORD const uiThreadId = GetWindowThreadProcessId(hWindow, &uiProcessId);
	if (uiProcessId != GetCurrentProcessId() || uiThreadId == 0 || !IsWindowVisible(hWindow) || GetWindow(hWindow, GW_OWNER) != NULL)
		return TRUE;
	SASRuntimeProcessWindow& kWindow = *(SASRuntimeProcessWindow*)lParam;
	kWindow.hWindow = hWindow;
	kWindow.uiThreadId = uiThreadId;
	return FALSE;
}

// <!-- custom: Keep close evidence typed by both message and observation boundary.
// Queue removal proves retrieval, not window-procedure delivery or completed exit; keep a later delivery observation without restarting its exit interval. (GPT-6.1-Sol) -->
static bool isSASRuntimeWindowCloseReason(SASRuntimeStatusReason eReason)
{
	switch (eReason)
	{
	case SAS_RUNTIME_STATUS_WINDOW_CLOSE_MESSAGE_OBSERVED:
	case SAS_RUNTIME_STATUS_WINDOW_CLOSE_COMMAND_OBSERVED:
	case SAS_RUNTIME_STATUS_WINDOW_CLOSE_MESSAGE_DEQUEUED:
	case SAS_RUNTIME_STATUS_WINDOW_CLOSE_COMMAND_DEQUEUED:
		return true;
	default:
		return false;
	}
}

// <!-- custom: Reject unrelated messages before querying windows, formatting JSON or writing files; enabled hooks incur only this small filter for ordinary messages.
// SC_CLOSE can precede WM_CLOSE and may be handled directly by the EXE.
// Windows reserves the low four command bits, so mask them as documented for WM_SYSCOMMAND.
// The first real autoplay/red-cross retest observed SC_CLOSE without WM_CLOSE; the queue hook separately covers posted requests reproduced by the standalone fixture. (GPT-6.1-Sol) -->
static void observeSASRuntimeWindowClose(HWND hWindow, UINT uiMessage, WPARAM wParam, bool bDequeued)
{
	bool const bCloseMessage = (uiMessage == WM_CLOSE);
	if (!bCloseMessage && !(uiMessage == WM_SYSCOMMAND && (wParam & 0xFFF0) == SC_CLOSE))
		return;
	if (g_bSASRuntimeHandlingWindowClose || g_eSASRuntimeProcessState == SAS_RUNTIME_PROCESS_EXITED_CLEANLY)
		return;
	if (g_hSASRuntimeObservedWindow == NULL)
	{
		// <!-- custom: Civ4's first menu-scene observation occurred before a visible window was discoverable.
		// An observer armed on that thread can bind a later close target without frame polling or requiring a game to have started. (GPT-6.1-Sol) -->
		DWORD uiProcessId = 0;
		DWORD const uiThreadId = GetWindowThreadProcessId(hWindow, &uiProcessId);
		if (uiProcessId != GetCurrentProcessId() || uiThreadId != g_uiSASRuntimeWindowHookThreadId || GetWindow(hWindow, GW_OWNER) != NULL)
			return;
		g_hSASRuntimeObservedWindow = hWindow;
	}
	if (hWindow != g_hSASRuntimeObservedWindow)
		return;
	SASRuntimeStatusReason const eReason = (bCloseMessage ?
		(bDequeued ? SAS_RUNTIME_STATUS_WINDOW_CLOSE_MESSAGE_DEQUEUED : SAS_RUNTIME_STATUS_WINDOW_CLOSE_MESSAGE_OBSERVED) :
		(bDequeued ? SAS_RUNTIME_STATUS_WINDOW_CLOSE_COMMAND_DEQUEUED : SAS_RUNTIME_STATUS_WINDOW_CLOSE_COMMAND_OBSERVED));
	g_bSASRuntimeHandlingWindowClose = true;
	writeSASRuntimeStatus(eReason);
	g_bSASRuntimeHandlingWindowClose = false;
}

static LRESULT CALLBACK observeSASRuntimeWindowMessage(int iCode, WPARAM wParam, LPARAM lParam)
{
	if (iCode >= 0 && lParam != 0)
	{
		CWPSTRUCT const& kMessage = *(CWPSTRUCT const*)lParam;
		observeSASRuntimeWindowClose(kMessage.hwnd, kMessage.message, kMessage.wParam, false);
	}
	return CallNextHookEx(g_hSASRuntimeWindowMessageHook, iCode, wParam, lParam);
}

// <!-- custom: WH_GETMESSAGE runs before GetMessage/PeekMessage returns to the EXE, covering messages consumed by its message loop before DispatchMessage.
// Ignore PM_NOREMOVE peeks so repeatedly inspecting one queued request does not repeatedly record it.
// Never modify MSG or suppress its normal return to the EXE. (GPT-6.1-Sol) -->
static LRESULT CALLBACK observeSASRuntimeWindowQueue(int iCode, WPARAM wParam, LPARAM lParam)
{
	if (iCode >= 0 && wParam == PM_REMOVE && lParam != 0)
	{
		MSG const& kMessage = *(MSG const*)lParam;
		observeSASRuntimeWindowClose(kMessage.hwnd, kMessage.message, kMessage.wParam, true);
	}
	return CallNextHookEx(g_hSASRuntimeWindowQueueHook, iCode, wParam, lParam);
}

static void releaseSASRuntimeWindowMessageHook()
{
	if (g_hSASRuntimeWindowMessageHook != NULL)
		UnhookWindowsHookEx(g_hSASRuntimeWindowMessageHook);
	if (g_hSASRuntimeWindowQueueHook != NULL)
		UnhookWindowsHookEx(g_hSASRuntimeWindowQueueHook);
	g_hSASRuntimeWindowMessageHook = NULL;
	g_hSASRuntimeWindowQueueHook = NULL;
	g_hSASRuntimeObservedWindow = NULL;
	g_uiSASRuntimeWindowHookThreadId = 0;
}

static void ensureSASRuntimeWindowMessageHook()
{
	if (g_hSASRuntimeWindowMessageHook != NULL && g_hSASRuntimeWindowQueueHook != NULL &&
		g_hSASRuntimeObservedWindow != NULL && IsWindow(g_hSASRuntimeObservedWindow))
		return;
	SASRuntimeProcessWindow kWindow;
	EnumWindows(findSASRuntimeProcessWindow, (LPARAM)&kWindow);
	// <!-- custom: Before the main window is visible, observe the thread making this lifecycle callback; bind its top-level close target when one is actually observed.
	// Later lifecycle observations can move the hooks to the discovered window's thread if it differs.
	// No turn/frame polling is added. (GPT-6.1-Sol) -->
	if (kWindow.uiThreadId == 0)
		kWindow.uiThreadId = GetCurrentThreadId();
	if (g_hSASRuntimeWindowMessageHook != NULL && g_hSASRuntimeWindowQueueHook != NULL && g_uiSASRuntimeWindowHookThreadId == kWindow.uiThreadId)
	{
		g_hSASRuntimeObservedWindow = kWindow.hWindow;
		return;
	}
	releaseSASRuntimeWindowMessageHook();
	HHOOK const hMessageHook = SetWindowsHookExW(WH_CALLWNDPROC, observeSASRuntimeWindowMessage, NULL, kWindow.uiThreadId);
	if (hMessageHook == NULL)
		return;
	HHOOK const hQueueHook = SetWindowsHookExW(WH_GETMESSAGE, observeSASRuntimeWindowQueue, NULL, kWindow.uiThreadId);
	if (hQueueHook == NULL)
	{
		// <!-- custom: Arming means both observation paths were installed; roll back a partial installation so later lifecycle callbacks can retry the complete pair. (GPT-6.1-Sol) -->
		UnhookWindowsHookEx(hMessageHook);
		return;
	}
	g_hSASRuntimeObservedWindow = kWindow.hWindow;
	g_uiSASRuntimeWindowHookThreadId = kWindow.uiThreadId;
	g_hSASRuntimeWindowMessageHook = hMessageHook;
	g_hSASRuntimeWindowQueueHook = hQueueHook;
	g_bSASRuntimeWindowCloseObservationArmed = true;
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
// Runtime revision 3 stores constant process/DLL identity once in a typed run record and ready-game context once per session; transition rows keep a compact run reference plus their local timing/failure evidence.
// A copied transition identifies its run, but full DLL/map interpretation needs its metadata records; the fixed current-state snapshot remains self-contained, and missing metadata does not suppress later transition evidence.
// State durations use the shared monotonic clock, not UTC subtraction; UTC still locates intervals in external evidence. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
static CvString const& getSASRuntimeHistoryRunId()
{
	if (g_szSASRuntimeHistoryRunId.empty())
		g_szSASRuntimeHistoryRunId.Format("%s_%lu", getSASExecutableProcessStartUtc().GetCString(), (unsigned long)GetCurrentProcessId());
	return g_szSASRuntimeHistoryRunId;
}

static bool prepareSASRuntimeHistory(std::wstring const& szRoot)
{
	if (g_szSASRuntimeHistoryFilename.empty())
		g_szSASRuntimeHistoryFilename.Format("SASRuntimeHistory_%s.jsonl", getSASRuntimeHistoryRunId().GetCString());
	std::wstring const szName(g_szSASRuntimeHistoryFilename.begin(), g_szSASRuntimeHistoryFilename.end());
	if (g_bSASRuntimeHistoryMayContainRows && !g_szSASRuntimeHistoryRoot.empty() && !isSASRuntimeStatusSamePath(g_szSASRuntimeHistoryRoot, szRoot))
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

// <!-- custom: Distinguish complete row bytes from confirmed write/close success through pbContentComplete.
// Metadata can avoid duplicate retries after an uncertain close while confirmed-row counters still require the successful close result. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
static bool appendSASRuntimeHistory(CvString const& szRow, bool* pbContentComplete = NULL)
{
	if (pbContentComplete != NULL)
		*pbContentComplete = false;
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
	if (pbContentComplete != NULL)
		*pbContentComplete = bComplete;
	BOOL const bClosed = CloseHandle(hFile);
	return bComplete && bClosed != FALSE;
}

// <!-- custom: Account for all three record types in one place; physical-row evidence and confirmed-success counts are deliberately separate after an uncertain close.
// Separate row writes keep each retry decision tied to that record's outcome rather than an ambiguous context/transition batch. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
static bool recordSASRuntimeHistoryRow(CvString const& szRow, bool* pbContentComplete = NULL)
{
	bool bContentComplete = false;
	bool const bConfirmed = appendSASRuntimeHistory(szRow, &bContentComplete);
	if (pbContentComplete != NULL)
		*pbContentComplete = bContentComplete;
	// <!-- custom: A full WriteFile followed by an unconfirmed close can leave a physical row even though it is not counted as confirmed; remember that possibility so a later CvAltRoot migration does not silently abandon the old history file. (ChatGPT-5.6-Sol) -->
	if (bConfirmed || bContentComplete)
		g_bSASRuntimeHistoryMayContainRows = true;
	if (bConfirmed)
	{
		g_uiSASRuntimeHistoryRowsWritten++;
		return true;
	}
	g_uiSASRuntimeHistoryWriteFailures++;
	return false;
}

// <!-- custom: Normally publish one run record before transitions so constant revision/process/DLL identity is not repeated on every line.
// If this metadata write fails, transition rows still carry revision + run ID and prior failure counters; open/partial failures retry later, while a full row with unconfirmed close is not duplicated. (ChatGPT-5.6-Sol) -->
static bool ensureSASRuntimeHistoryMetadata()
{
	if (g_bSASRuntimeHistoryMetadataWritten)
		return true;
	CvString szRow;
	CvString const& szDllIdentity = getSASRuntimeDllIdentity();
	szRow.Format("{\"record\":\"run\","
		"\"revision\":%d,"
		"\"run\":\"%s\","
		"\"processId\":%lu,"
		"\"processStartUtc\":\"%s\","
		"\"dllLoadUtc\":\"%s\","
		"\"dllIdentity\":%s,"
		"\"priorStatusWriteFailures\":%u,"
		"\"priorHistoryWriteFailures\":%u}\r\n",
		SAS_RUNTIME_REVISION, getSASRuntimeHistoryRunId().GetCString(), (unsigned long)GetCurrentProcessId(), getSASExecutableProcessStartUtc().GetCString(), getSASProcessUtcTimestamp().GetCString(), szDllIdentity.GetCString(),
		g_uiSASRuntimeStatusWriteFailures, g_uiSASRuntimeHistoryWriteFailures);
	bool bContentComplete = false;
	bool const bConfirmed = recordSASRuntimeHistoryRow(szRow, &bContentComplete);
	// <!-- custom: Do not duplicate a fully written row after an unconfirmed close; historyWriteFailures already marks that its physical presence is uncertain.
	// Retry only when the row bytes themselves did not complete. (ChatGPT-5.6-Sol) -->
	g_bSASRuntimeHistoryMetadataWritten = bConfirmed || bContentComplete;
	return g_bSASRuntimeHistoryMetadataWritten;
}

// <!-- custom: Capture first-ready context once per session instead of repeating it on transitions or resampling a later turn during retries.
// Cache the serialized stable fields independently of directory availability or metadata-write success; recordedUtc locates that observation, while publication-failure counters are appended at each write attempt. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
static void captureSASRuntimeHistorySessionMetadata(CvString const& szNowUtc, int iReadyGameTurn, int iReadyActivePlayer, int iMapWidth, int iMapHeight, int iWorldSize, int iGameSpeed, int iScenario, int iLoadedCheckpointTurn, int iLoadedCheckpointElapsedTurn, char const* szLoadedCheckpointUtc)
{
	if (g_uiSASRuntimeSessionSerial == 0 || g_uiSASRuntimeHistorySessionMetadataCaptured == g_uiSASRuntimeSessionSerial)
		return;
	g_szSASRuntimeHistoryPendingSessionMetadata.Format("{\"record\":\"session\","
		"\"revision\":%d,"
		"\"run\":\"%s\","
		"\"recordedUtc\":\"%s\","
		"\"sessionSerial\":%u,"
		"\"sessionKind\":\"%s\","
		"\"sessionStartedUtc\":\"%s\","
		"\"nativeReadyUtc\":\"%s\","
		"\"newGameCount\":%u,"
		"\"loadCount\":%u,"
		"\"readyGameTurn\":%d,"
		"\"readyActivePlayer\":%d,"
		"\"mapWidth\":%d,"
		"\"mapHeight\":%d,"
		"\"worldSize\":%d,"
		"\"gameSpeed\":%d,"
		"\"scenario\":%d,"
		"\"loadedSaveCheckpointTurn\":%d,"
		"\"loadedSaveCheckpointElapsedTurn\":%d,"
		"\"loadedSaveCheckpointUtc\":\"%s\",",
		SAS_RUNTIME_REVISION, getSASRuntimeHistoryRunId().GetCString(), szNowUtc.GetCString(), g_uiSASRuntimeSessionSerial, aSASRuntimeSessionStateNames[g_eSASRuntimeSessionKind], g_szSASRuntimeSessionStartedUtc.GetCString(), g_szSASRuntimeNativeReadyUtc.GetCString(),
		g_uiSASRuntimeNewGameCount, g_uiSASRuntimeLoadCount, iReadyGameTurn, iReadyActivePlayer, iMapWidth, iMapHeight, iWorldSize, iGameSpeed, iScenario,
		iLoadedCheckpointTurn, iLoadedCheckpointElapsedTurn, szLoadedCheckpointUtc);
	g_uiSASRuntimeHistorySessionMetadataCaptured = g_uiSASRuntimeSessionSerial;
}

// <!-- custom: Retry the frozen session payload independently of live gameplay so a menu/reset update can still recover the last ready observation.
// Open/partial failures leave it pending; fully written metadata with an uncertain close is not duplicated, and keeping the capture serial prevents later turn updates from recreating it. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
static void ensureSASRuntimeHistorySessionMetadata()
{
	if (!g_bSASRuntimeHistoryMetadataWritten || g_szSASRuntimeHistoryPendingSessionMetadata.empty())
		return;
	CvString szRow;
	szRow.Format("%s\"priorStatusWriteFailures\":%u,\"priorHistoryWriteFailures\":%u}\r\n",
		g_szSASRuntimeHistoryPendingSessionMetadata.GetCString(), g_uiSASRuntimeStatusWriteFailures, g_uiSASRuntimeHistoryWriteFailures);
	bool bContentComplete = false;
	bool const bConfirmed = recordSASRuntimeHistoryRow(szRow, &bContentComplete);
	if (bConfirmed || bContentComplete)
		g_szSASRuntimeHistoryPendingSessionMetadata.clear();
}

static SASRuntimeActivity getSASRuntimeActivity(SASRuntimeStatusReason eReason, int iAutoPlayRemaining)
{
	if (g_eSASRuntimeProcessState == SAS_RUNTIME_PROCESS_EXITED_CLEANLY)
		return SAS_RUNTIME_ACTIVITY_SHUTTING_DOWN;
	if (eReason == SAS_RUNTIME_STATUS_RETURN_TO_MENU_REQUESTED)
		return SAS_RUNTIME_ACTIVITY_RETURNING_TO_MENU;
	if (eReason == SAS_RUNTIME_STATUS_EXIT_REQUESTED || isSASRuntimeWindowCloseReason(eReason))
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
	// <!-- custom: A still-visible editor must not overwrite an already observed menu/exit request when another callback arrives before its hide notification. (GPT-6.1-Sol) -->
	if (g_bSASRuntimeWorldBuilderMode && g_bSASRuntimeWorldBuilderScreenVisible)
		return SAS_RUNTIME_ACTIVITY_WORLD_BUILDER;
	return (iAutoPlayRemaining > 0 ? SAS_RUNTIME_ACTIVITY_AUTOPLAY : SAS_RUNTIME_ACTIVITY_PLAYING);
}

// <!-- custom: Save-checkpoint/map context now belongs to frozen session metadata; this formatter takes only the current transition's time, turn and autoplay values. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
static void updateSASRuntimeActivity(SASRuntimeStatusReason eReason, CvString const& szNowUtc, uint uiNowMilliseconds, int iGameTurn, int iAutoPlayRemaining)
{
	SASRuntimeActivity const eActivity = getSASRuntimeActivity(eReason, iAutoPlayRemaining);
	// <!-- custom: Keep presentation observations in the timeline without splitting the underlying activity interval.
	// The native fixture's creation from 1000 to 2000 ms previously became 250 ms plus 750 ms after its loading-asset request; preserving the interval start restores the full 1000 ms completion duration. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	bool const bObservationBoundary = (
		eReason == SAS_RUNTIME_STATUS_MAIN_MENU_SCENE_REQUESTED ||
		eReason == SAS_RUNTIME_STATUS_LOADING_SCREEN_ASSET_REQUESTED ||
		eReason == SAS_RUNTIME_STATUS_WORLD_BUILDER_MODE_ENTERED ||
		eReason == SAS_RUNTIME_STATUS_WORLD_BUILDER_MODE_EXITED ||
		eReason == SAS_RUNTIME_STATUS_WORLD_BUILDER_SCREEN_SHOWN ||
		eReason == SAS_RUNTIME_STATUS_WORLD_BUILDER_SCREEN_HIDDEN ||
		isSASRuntimeWindowCloseReason(eReason));
	bool const bIntervalStarted = (eActivity != g_eSASRuntimeActivity || eReason == SAS_RUNTIME_STATUS_AUTOPLAY_CHANGED || g_uiSASRuntimeActivitySession != g_uiSASRuntimeSessionSerial);
	if (!bIntervalStarted && !bObservationBoundary)
		return;
	unsigned int const uiPreviousSession = (bIntervalStarted ? g_uiSASRuntimeActivitySession : 0);
	int const iPreviousStartTurn = (bIntervalStarted ? g_iSASRuntimeActivityStartTurn : -1);
	int const iPreviousRequestedTurns = (bIntervalStarted ? g_iSASRuntimeActivityRequestedTurns : 0);
	if (bIntervalStarted)
	{
		g_eSASRuntimePreviousActivity = g_eSASRuntimeActivity;
		g_szSASRuntimePreviousStartedUtc = g_szSASRuntimeActivityStartedUtc;
		g_szSASRuntimePreviousEndedUtc = (g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_NONE ? CvString("-") : szNowUtc);
		g_iSASRuntimePreviousDurationMilliseconds = (g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_NONE ? -1 : (__int64)getSASElapsedMilliseconds(g_uiSASRuntimeActivityStartedMilliseconds, uiNowMilliseconds));
		g_eSASRuntimeActivity = eActivity;
		g_szSASRuntimeActivityStartedUtc = szNowUtc;
		g_uiSASRuntimeActivityStartedMilliseconds = uiNowMilliseconds;
		g_uiSASRuntimeActivitySession = g_uiSASRuntimeSessionSerial;
		g_iSASRuntimeActivityStartTurn = iGameTurn;
		g_iSASRuntimeActivityRequestedTurns = iAutoPlayRemaining;
	}
	CvString szRow;
	// <!-- custom: Keep one JSONL record on each output line.
	// Constant binary/process identity lives in the run record; revision + run ID stay on transitions so copied/grepped lines still identify their recorder/run.
	// Observation-only rows do not repeat a completed previous interval; the fixed snapshot still retains the most recently completed activity.
	// Prior publication counters disclose missing earlier evidence without making every row repeat the fixed snapshot or session metadata. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
	szRow.Format("{\"record\":\"transition\","
		"\"revision\":%d,"
		"\"run\":\"%s\","
		"\"stateSerial\":%u,"
		"\"intervalStarted\":%d,"
		"\"previousState\":\"%s\","
		"\"previousStartedUtc\":\"%s\","
		"\"previousEndedUtc\":\"%s\","
		"\"previousDurationMilliseconds\":%I64d,"
		"\"previousSessionSerial\":%u,"
		"\"previousStartTurn\":%d,"
		"\"previousRequestedAutoplayTurns\":%d,"
		"\"recordedUtc\":\"%s\","
		"\"state\":\"%s\","
		"\"stateStartedUtc\":\"%s\","
		"\"stateElapsedMilliseconds\":%u,"
		"\"sessionSerial\":%u,"
		"\"gameTurn\":%d,"
		"\"autoplayRemainingTurns\":%d,"
		"\"reason\":\"%s\","
		"\"autoplayEndCause\":\"%s\","
		"\"priorStatusWriteFailures\":%u,"
		"\"priorHistoryWriteFailures\":%u}\r\n",
		SAS_RUNTIME_REVISION, getSASRuntimeHistoryRunId().GetCString(), ++g_uiSASRuntimeActivitySerial, bIntervalStarted ? 1 : 0,
		aSASRuntimeActivityNames[bIntervalStarted ? g_eSASRuntimePreviousActivity : SAS_RUNTIME_ACTIVITY_NONE], bIntervalStarted ? g_szSASRuntimePreviousStartedUtc.GetCString() : "-", bIntervalStarted ? g_szSASRuntimePreviousEndedUtc.GetCString() : "-", bIntervalStarted ? g_iSASRuntimePreviousDurationMilliseconds : (__int64)-1,
		uiPreviousSession, iPreviousStartTurn, iPreviousRequestedTurns, szNowUtc.GetCString(), aSASRuntimeActivityNames[eActivity], g_szSASRuntimeActivityStartedUtc.GetCString(), getSASElapsedMilliseconds(g_uiSASRuntimeActivityStartedMilliseconds, uiNowMilliseconds), g_uiSASRuntimeSessionSerial, iGameTurn, iAutoPlayRemaining,
		aSASRuntimeStatusReasonNames[eReason], (eReason == SAS_RUNTIME_STATUS_AUTOPLAY_ENDED ? getSASAutoPlayEndCause(g_eSASRuntimeLastAutoplayEndCause) : "-"), g_uiSASRuntimeStatusWriteFailures, g_uiSASRuntimeHistoryWriteFailures);
	recordSASRuntimeHistoryRow(szRow);
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
	// <!-- custom: Do not retry window discovery on ordinary turn heartbeats; startup/menu/session/request boundaries provide repeated low-frequency opportunities to arm the observer if the window did not exist yet. (ChatGPT-5.6-Sol) -->
	if (eReason != SAS_RUNTIME_STATUS_TURN_COMPLETED && eReason != SAS_RUNTIME_STATUS_PROCESS_EXIT && !isSASRuntimeWindowCloseReason(eReason))
		ensureSASRuntimeWindowMessageHook();
	// <!-- custom: Sample the observed boundary before root discovery or file I/O, so slow publication does not redefine when creation/loading or autoplay changed state. (GPT-6.1-Sol) -->
	CvString const szUpdatedUtc = createSASUtcTimestampMilliseconds();
	if (isSASRuntimeWindowCloseReason(eReason))
		g_szSASRuntimeWindowCloseRequestObservedUtc = szUpdatedUtc;
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
	int const iExtendedGame = (bGameLoaded && kGame.getGameState() == GAMESTATE_EXTENDED ? 1 : 0);
	int const iLoadedCheckpointTurn = (bGameLoaded && g_eSASRuntimeSessionKind == SAS_RUNTIME_SESSION_LOAD_GAME ? kGame.getSASLoadedSaveCheckpointTurn() : -1);
	int const iLoadedCheckpointElapsedTurn = (bGameLoaded && g_eSASRuntimeSessionKind == SAS_RUNTIME_SESSION_LOAD_GAME ? kGame.getSASLoadedSaveCheckpointElapsedTurn() : -1);
	char const* szLoadedCheckpointUtc = "-";
	if (bGameLoaded && g_eSASRuntimeSessionKind == SAS_RUNTIME_SESSION_LOAD_GAME && kGame.getSASLoadedSaveCheckpointUtc() != NULL && kGame.getSASLoadedSaveCheckpointUtc()[0] != '\0')
		szLoadedCheckpointUtc = kGame.getSASLoadedSaveCheckpointUtc();
	if (bGameLoaded)
		captureSASRuntimeHistorySessionMetadata(szUpdatedUtc, iGameTurn, iActivePlayer, iMapWidth, iMapHeight, iWorldSize, iGameSpeed, iScenario, iLoadedCheckpointTurn, iLoadedCheckpointElapsedTurn, szLoadedCheckpointUtc);
	// <!-- custom: Failed output must not postpone the observed transition or inflate the next interval; retain timing in memory and disclose unsuccessful history appends when publication resumes. (GPT-6.1-Sol) -->
	if (bDirectoryReady)
	{
		prepareSASRuntimeHistory(g_szSASRuntimeStatusRootDirectory);
		ensureSASRuntimeHistoryMetadata();
		// <!-- custom: Normally session context precedes its ready transition; after I/O failure, retry the captured context even if this update reports a menu/reset boundary. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
		ensureSASRuntimeHistorySessionMetadata();
	}
	else g_bSASRuntimeHistoryAtCurrentRoot = false;
	updateSASRuntimeActivity(eReason, szUpdatedUtc, uiNowMilliseconds, iGameTurn, iAutoPlayRemaining);
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
		"  \"windowCloseObservationArmed\": %d,\r\n"
		"  \"windowCloseRequestObservedUtc\": \"%s\",\r\n"
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
		"  \"mainMenuSceneRequestedUtc\": \"%s\",\r\n"
		"  \"loadingScreenAssetRequestedUtc\": \"%s\",\r\n"
		"  \"worldBuilderMode\": %d,\r\n"
		"  \"worldBuilderScreenVisible\": %d,\r\n"
		"  \"extendedGame\": %d,\r\n"
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
		aSASRuntimeProcessStateNames[g_eSASRuntimeProcessState], (g_bSASRuntimeWindowCloseObservationArmed ? 1 : 0), g_szSASRuntimeWindowCloseRequestObservedUtc.GetCString(),
		aSASRuntimeGameStateNames[g_eSASRuntimeGameState],
		aSASRuntimeActivityNames[g_eSASRuntimeActivity], g_uiSASRuntimeActivitySerial, g_szSASRuntimeActivityStartedUtc.GetCString(), getSASElapsedMilliseconds(g_uiSASRuntimeActivityStartedMilliseconds, uiNowMilliseconds),
		aSASRuntimeActivityNames[g_eSASRuntimePreviousActivity], g_szSASRuntimePreviousStartedUtc.GetCString(), g_szSASRuntimePreviousEndedUtc.GetCString(), g_iSASRuntimePreviousDurationMilliseconds,
		g_szSASRuntimeHistoryFilename.GetCString(), g_bSASRuntimeHistoryAtCurrentRoot, g_uiSASRuntimeHistoryRowsWritten, g_uiSASRuntimeHistoryWriteFailures,
		aSASRuntimeStatusReasonNames[eReason], g_uiSASRuntimeSessionSerial, g_uiSASRuntimeNewGameCount, g_uiSASRuntimeLoadCount,
		aSASRuntimeSessionStateNames[g_eSASRuntimeSessionKind], g_szSASRuntimeSessionStartedUtc.GetCString(), g_szSASRuntimeNativeReadyUtc.GetCString(),
		g_szSASRuntimeMainMenuSceneRequestedUtc.GetCString(), g_szSASRuntimeLoadingScreenAssetRequestedUtc.GetCString(), g_bSASRuntimeWorldBuilderMode, g_bSASRuntimeWorldBuilderScreenVisible, iExtendedGame,
		iGameTurn, iElapsedGameTurns, g_iSASRuntimeLastCompletedTurn, iActivePlayer,
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
	g_szSASRuntimeMainMenuSceneRequestedUtc = "-";
	g_szSASRuntimeLoadingScreenAssetRequestedUtc = "-";
	g_bSASRuntimeWorldBuilderMode = false;
	g_bSASRuntimeWorldBuilderScreenVisible = false;
	g_iSASRuntimeLastCompletedTurn = -1;
	writeSASRuntimeStatus(SAS_RUNTIME_STATUS_GAME_RESET);
}

// <!-- custom: CvMainMenuInfo scene getters are DllExports consumed by the EXE and have no local caller; observing an existing request avoids calling side-effectful soundtrack code or polling CyInterface().
// Record this only while no game is represented and once per no-game interval.
// It is presentation evidence, not proof that a rendered frame reached the user. (ChatGPT-5.6-Sol) -->
void updateSASRuntimeStatusMainMenuSceneRequested()
{
	if (!isSASRuntimeStatusEnabled() || g_eSASRuntimeGameState != SAS_RUNTIME_GAME_NO_GAME || g_szSASRuntimeMainMenuSceneRequestedUtc != "-")
		return;
	g_szSASRuntimeMainMenuSceneRequestedUtc = createSASUtcTimestampMilliseconds();
	writeSASRuntimeStatus(SAS_RUNTIME_STATUS_MAIN_MENU_SCENE_REQUESTED);
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
	g_szSASRuntimeMainMenuSceneRequestedUtc = "-";
	g_szSASRuntimeLoadingScreenAssetRequestedUtc = "-";
	g_bSASRuntimeWorldBuilderMode = false;
	g_bSASRuntimeWorldBuilderScreenVisible = false;
	g_iSASRuntimeLastCompletedTurn = -1;
	writeSASRuntimeStatus(bLoadedSave ? SAS_RUNTIME_STATUS_LOAD_GAME_STARTED : SAS_RUNTIME_STATUS_NEW_GAME_STARTED);
}

// <!-- custom: getLoading/getLoadingSlideshow are passive EXE-facing asset requests.
// Preserve the first request inside each native new/load interval as a loading-presentation proxy without calling either getter ourselves. (ChatGPT-5.6-Sol) -->
void updateSASRuntimeStatusLoadingScreenAssetRequested()
{
	if (!isSASRuntimeStatusEnabled() || g_eSASRuntimeGameState != SAS_RUNTIME_GAME_SESSION_STARTING || g_szSASRuntimeLoadingScreenAssetRequestedUtc != "-")
		return;
	g_szSASRuntimeLoadingScreenAssetRequestedUtc = createSASUtcTimestampMilliseconds();
	writeSASRuntimeStatus(SAS_RUNTIME_STATUS_LOADING_SCREEN_ASSET_REQUESTED);
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

void updateSASRuntimeStatusWorldBuilderMode(bool bEnabled)
{
	if (!isSASRuntimeStatusEnabled() || g_bSASRuntimeWorldBuilderMode == bEnabled)
		return;
	g_bSASRuntimeWorldBuilderMode = bEnabled;
	// <!-- custom: Keep the Python screen observation independent from native mode so either exit ordering remains visible: mode-off ends editor activity immediately, while the later hide callback records presentation closure. (ChatGPT-5.6-Sol) -->
	writeSASRuntimeStatus(bEnabled ? SAS_RUNTIME_STATUS_WORLD_BUILDER_MODE_ENTERED : SAS_RUNTIME_STATUS_WORLD_BUILDER_MODE_EXITED);
}

void updateSASRuntimeStatusWorldBuilderScreen(bool bVisible)
{
	if (!isSASRuntimeStatusEnabled() || g_eSASRuntimeGameState != SAS_RUNTIME_GAME_PLAYING || g_bSASRuntimeWorldBuilderScreenVisible == bVisible)
		return;
	// <!-- custom: The Python bridge filters Advanced Start before reaching this helper; retain a genuine show observation even if the EXE has not exposed its mode toggle yet.
	// Editor activity starts only once both mode and visibility are observed, so callback order cannot discard a visible screen or invent active editor time from mode alone. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	g_bSASRuntimeWorldBuilderScreenVisible = bVisible;
	writeSASRuntimeStatus(bVisible ? SAS_RUNTIME_STATUS_WORLD_BUILDER_SCREEN_SHOWN : SAS_RUNTIME_STATUS_WORLD_BUILDER_SCREEN_HIDDEN);
}

void updateSASRuntimeStatusExtendedGameStarted()
{
	if (!isSASRuntimeStatusEnabled())
		return;
	g_eSASRuntimeGameState = SAS_RUNTIME_GAME_PLAYING;
	g_bSASRuntimeWorldBuilderMode = false;
	g_bSASRuntimeWorldBuilderScreenVisible = false;
	writeSASRuntimeStatus(SAS_RUNTIME_STATUS_EXTENDED_GAME_STARTED);
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
	g_bSASRuntimeWorldBuilderMode = false;
	g_bSASRuntimeWorldBuilderScreenVisible = false;
	writeSASRuntimeStatus(SAS_RUNTIME_STATUS_GAME_ENDED);
}

void updateSASRuntimeStatusCleanExit()
{
	// <!-- custom: Do not resolve/query XML for the first time during global teardown; any enabled status session initialized this cache earlier. (ChatGPT-5.6-Sol) -->
	if (g_iSASRuntimeStatusEnabled <= 0)
		return;
	g_eSASRuntimeProcessState = SAS_RUNTIME_PROCESS_EXITED_CLEANLY;
	writeSASRuntimeStatus(SAS_RUNTIME_STATUS_PROCESS_EXIT);
	// <!-- custom: Release the thread-local message observer after the final orderly snapshot; the snapshot keeps the cumulative observation-armed flag, while abrupt process termination is cleaned up by Windows. (ChatGPT-5.6-Sol) -->
	releaseSASRuntimeWindowMessageHook();
}
