// AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
// (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

// <!-- custom: Manual Windows harness for the actual diagnostic writer, with fault-injecting file open/write wrappers and a shell-folder-unavailable fixture.
// Run through tests/diagnostics/run_writer_tests.py, which extracts the writer from CvGameCoreUtils.cpp and compiles it with this fixture into a temporary executable using the Visual C++ Toolkit 2003 (VC2003) compiler.
// The fixture supplies minimal game scaffolding; Civ4 never loads this executable, and no game DLL is built or installed.
// This exercises real filesystem sharing and Unicode paths; game lifecycle and autoplay performance remain separate runtime checks. (GPT-6.1-Sol) -->
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0500
// <!-- custom: Win32 declarations exercise the real Unicode/sharing APIs; standard headers supply writer containers/algorithms, assertions, byte/string operations and the varargs formatter fixture. (GPT-6.1-Sol) -->
#include <windows.h>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
// <!-- custom: Standalone CvString.h compilation requires the vector/static-assert/delete helpers normally supplied by the game PCH; VC2003 reported C3861/C2039/C2501 without them. (GPT-6.1-Sol) -->
#include <vector>
#include <boost/static_assert.hpp>
template<class T> static void harnessDeleteArray(T*& value) { delete[] value; value = NULL; }
#define SAFE_DELETE_ARRAY(value) harnessDeleteArray(value)
// <!-- custom: Runtime timing uses the game PCH's uint alias; standalone VC2003 compilation failed with C2146/C2501 until the fixture supplied the same unsigned type. (GPT-6.1-Sol) -->
typedef unsigned int uint;
typedef wchar_t wchar;
#include "CvString.h"

// <!-- custom: The writer's loss-notice and health-field formatters need this game-string method; all buffering, path and I/O logic below comes from the current source. (GPT-6.1-Sol) -->
void CvString::Format(LPCSTR format, ...)
{
	// <!-- custom: The indented runtime snapshot includes timing and identity fields; this fixture formatter must accommodate it like production CvString rather than truncate at the old 2 KiB writer-row limit. (GPT-6.1-Sol) -->
	char buffer[8192];
	va_list args;
	va_start(args, format);
	int const length = _vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	assert(length >= 0 && length < sizeof(buffer));
	assign(buffer, length);
}

// <!-- custom: Minimal read-only game/map state lets the separate runtime-status publisher run without Civ4; Define lookup accepts only its own independent enable switch. (GPT-6.1-Sol) -->
// <!-- custom: Production GameStateTypes comes from the Civ4 headers; the standalone fixture needs only the values inspected by runtime status. (ChatGPT-5.6-Sol) -->
// <!-- custom: Minimal player/civilization stubs exercise overlay ownership without including gameplay code in this standalone test. (GPT-6.1-Sol) -->
typedef int PlayerTypes;
struct TestPlayer { int getCivilizationType() const { return 4; } };
static TestPlayer g_runtimePlayer;
#define GET_PLAYER(PLAYER) g_runtimePlayer
enum GameStateTypes { GAMESTATE_ON = 0, GAMESTATE_OVER, GAMESTATE_EXTENDED };
static int g_worldBuilderModeReads = 0;
static int g_advancedStartReads = 0;
struct CvGame
{
	CvGame() : ready(false), autoplay(0), state(GAMESTATE_ON), advancedStart(false), gameTurn(9), activePlayer(0) {}
	bool ready;
	int autoplay;
	GameStateTypes state;
	bool advancedStart;
	// <!-- custom: Mutable observation fields reproduce delayed metadata writes after the game turn or active player changes; normal fixture defaults remain turn 9/player 0. (GPT-6.1-Sol) -->
	int gameTurn;
	int activePlayer;
	bool isInAdvancedStart() const { ++g_advancedStartReads; return advancedStart; }
	bool isFinalInitialized() const { return ready; }
	GameStateTypes getGameState() const { return state; }
	int getGameTurn() const { return gameTurn; }
	int getElapsedGameTurns() const { return gameTurn; }
	int getActivePlayer() const { return activePlayer; }
	int getAIAutoPlay() const { return autoplay; }
	int getGameSpeedType() const { return 2; }
	bool isScenario() const { return false; }
	int getSASLoadedSaveCheckpointTurn() const { return 7; }
	int getSASLoadedSaveCheckpointElapsedTurn() const { return 7; }
	char const* getSASLoadedSaveCheckpointUtc() const { return "20261018T000000.000Z"; }
};
struct TestMap
{
	int getGridWidth() const { return 120; }
	int getGridHeight() const { return 84; }
	int getWorldSize() const { return 5; }
};
struct TestGlobals
{
	TestGlobals() : overwrite(true), runtimeStatus(false) {}
	bool overwrite;
	bool runtimeStatus;
	CvGame game;
	TestMap map;
	bool isOverwriteLogs() const { return overwrite; }
	int getDefineINT(char const* name) const { assert(strcmp(name, "SAS_RUNTIME_STATUS_ENABLE") == 0); return runtimeStatus ? 1 : 0; }
	CvGame const& getGame() const { return game; }
	TestMap const& getMap() const { return map; }
} GC;

// <!-- custom: Compile the actual WorldBuilder Python bridge with observable native getters; disabled cases must skip both mode queries and the Advanced Start scan. (GPT-6.1-Sol) -->
struct RuntimeTestDLL
{
	RuntimeTestDLL() : worldBuilder(false) {}
	bool worldBuilder;
	bool GetWorldBuilderMode() const { ++g_worldBuilderModeReads; return worldBuilder; }
} g_runtimeTestDLL;
static RuntimeTestDLL* gDLL = &g_runtimeTestDLL;
class CyGame
{
public:
	CyGame(CvGame& game) : m_kGame(game) {}
	void updateSASRuntimeStatusWorldBuilderScreen(bool bVisible);
	bool isSASRuntimeStatusRecordingEnabled() const;
	void updateSASRuntimeStatusMainMenuState(bool bInMainMenu);
	void updateSASRuntimeStatusPreGameStart();
	void updateSASRuntimeStatusCivilopedia(bool bVisible, bool bInMainMenu);
private:
	CvGame& m_kGame;
};

// <!-- custom: Supply a stable DLL-load label; process creation time and status UTC still use their actual production Win32 implementations. (GPT-6.1-Sol) -->
CvString const& getSASProcessUtcTimestamp() { static CvString const stamp("20261018T000000Z"); return stamp; }

static bool g_shellFolderUnavailable = false;
static bool g_failOpen = false;
static bool g_failClose = false;
static bool g_failPartialWrite = false;
static bool g_zeroWrite = false;
static bool g_inspectWhileWriting = false;
static DWORD g_maxWrite = 0xffffffff;
static int g_openCalls = 0;
static int g_writeCalls = 0;
static std::wstring g_openPath;

static void require(bool condition, char const* message)
{
	if (!condition)
	{
		fprintf(stderr, "FAIL C++ diagnostic writer: %s (Win32 error %lu)\n", message, GetLastError());
		exit(1);
	}
}

static HANDLE WINAPI testCreateFileW(LPCWSTR path, DWORD access, DWORD sharing, LPSECURITY_ATTRIBUTES attributes, DWORD disposition, DWORD flags, HANDLE templateFile)
{
	g_openCalls++;
	g_openPath = path;
	if (g_failOpen)
	{
		SetLastError(ERROR_SHARING_VIOLATION);
		return INVALID_HANDLE_VALUE;
	}
	return ::CreateFileW(path, access, sharing, attributes, disposition, flags, templateFile);
}

// <!-- custom: Close the real handle but report failure once, exercising the drained-file recovery path without leaking fixture handles. (GPT-6.1-Sol) -->
static BOOL WINAPI testCloseHandle(HANDLE file)
{
	BOOL const success = ::CloseHandle(file);
	if (g_failClose)
	{
		g_failClose = false;
		SetLastError(ERROR_WRITE_FAULT);
		return FALSE;
	}
	return success;
}

static BOOL WINAPI testWriteFile(HANDLE file, LPCVOID bytes, DWORD count, LPDWORD written, LPOVERLAPPED overlapped)
{
	g_writeCalls++;
	if (g_zeroWrite)
	{
		*written = 0;
		return TRUE;
	}
	if (g_inspectWhileWriting)
	{
		HANDLE const reader = ::CreateFileW(g_openPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
		require(reader != INVALID_HANDLE_VALUE, "reader can open during append");
		CloseHandle(reader);
	}
	BOOL const success = ::WriteFile(file, bytes, std::min(count, g_maxWrite), written, overlapped);
	if (success && g_failPartialWrite)
	{
		SetLastError(ERROR_DISK_FULL);
		return FALSE;
	}
	return success;
}

// <!-- custom: Simulate an unavailable shell-folder API only for the isolated shutdown case; USERPROFILE then points into the temporary fixture so fallback testing never writes to the real user Logs directory. (GPT-6.1-Sol) -->
static HMODULE WINAPI testLoadLibraryW(LPCWSTR name)
{
	if (g_shellFolderUnavailable && wcscmp(name, L"shell32.dll") == 0)
		return NULL;
	return ::LoadLibraryW(name);
}

#define FAssert(condition) assert(condition)
#define LoadLibraryW testLoadLibraryW
#define CreateFileW testCreateFileW
#define WriteFile testWriteFile
#define CloseHandle testCloseHandle
#include "writer_under_test.inc"
// <!-- custom: Compile the production binary context and JSON formatter against controlled fixture metadata; this generated include comes from the same runner as the writer extraction. (GPT-6.1-Sol) -->
#define SAS_DLL_BUILD_CONFIGURATION 2
#include "runtime_binary_context_under_test.inc"
// <!-- custom: Count binary-context accesses and supply a controllable clock so lifecycle tests verify lazy formatting and exact elapsed times independently of real UTC progression. (GPT-6.1-Sol) -->
static int g_dllIdentityLookups = 0;
static unsigned int g_runtimeClock = 0;
unsigned int getSASMonotonicMilliseconds() { return g_runtimeClock; }
unsigned int getSASElapsedMilliseconds(unsigned int start, unsigned int end) { return end - start; }
static SASBinaryDiagnosticContext const& getSASDllDiagnosticContext()
{
	static SASBinaryDiagnosticContext context;
	++g_dllIdentityLookups;
	context.bModuleFound = true;
	context.bFileReadable = true;
	context.iFileSize = 123456;
	context.uiFingerprint = 0x123456789ABCDEF0ui64;
	context.szLastWriteUtc = "20261018T000000.000Z";
	context.uiPETimestamp = 42;
	return context;
}
// <!-- custom: Compile the separate current-state publisher with the same real Win32 wrappers and explicit game fixture so JSON/lifecycle failures are exercised alongside writer I/O. (GPT-6.1-Sol) -->
#include "runtime_status_under_test.inc"
// <!-- custom: Extract the production CyGame notification body so callback ordering and pre-gating are tested at the real bridge, not just at its lower-level helpers. (GPT-6.1-Sol) -->
#include "runtime_world_builder_bridge_under_test.inc"
#undef CloseHandle
#undef WriteFile
#undef CreateFileW
#undef LoadLibraryW

static std::string readFile(std::wstring const& path)
{
	HANDLE const file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return std::string();
	std::string result;
	char bytes[4096];
	DWORD count = 0;
	while (ReadFile(file, bytes, sizeof(bytes), &count, NULL) && count > 0)
		result.append(bytes, count);
	CloseHandle(file);
	return result;
}

static void seedFile(std::wstring const& path)
{
	HANDLE const file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, 0, NULL);
	require(file != INVALID_HANDLE_VALUE, "create fixture file");
	DWORD written = 0;
	require(WriteFile(file, "old\r\n", 5, &written, NULL) && written == 5, "seed fixture file");
	CloseHandle(file);
}

// <!-- custom: Preserve each overwritten state as a fixture snapshot so Python's JSON parser checks the actual emitted document, rather than reproducing its formatter. (GPT-6.1-Sol) -->
static void snapshotStatus(std::wstring const& root, std::wstring const& snapshots, wchar_t const* name)
{
	require(CopyFileW((root + L"\\Logs\\SASRuntimeStatus.json").c_str(), (snapshots + L"\\" + name + L".json").c_str(), FALSE) != FALSE, "copy runtime snapshot");
}

// <!-- custom: Isolate recorder observation stubs from the runtime publisher fixture; the included finalizer and open-session declaration come directly from production.
// Health writes use the real sink, while counters verify that aborted setup, plot observations and RNG cleanup survive the no-session health guard. (GPT-6.1-Sol) -->
namespace recordFinalizationFixture
{
	static bool g_bSASGameRecordBufferInitializingActions = false;
	static int g_iSASGameRecordPendingPlotTurn = -1;
	static bool g_bSASGameRecordRngTrackingActive = false;
	static int const SAS_RNG_CHECKPOINT_SESSION_FINALIZE = 1;
	static int setupFlushes = 0, plotFlushes = 0, rngCheckpoints = 0, healthRows = 0;
	static void flushSASGameRecordInitializingActions(bool complete) { assert(!complete); ++setupFlushes; }
	static void flushSASGameRecordTurnChanges(int turn) { ++plotFlushes; g_iSASGameRecordPendingPlotTurn = -1; }
	static void flushSASGameRecordPendingCityBombard() {}
	static void logSASGameRecordRngCheckpoint(int turn, int reason) { ++rngCheckpoints; }
	static void clearSASGameRecordRngTracking() { g_bSASGameRecordRngTrackingActive = false; }
	static void logSASGameRecordDiagnosticWriterHealth(int turn, char const* reason)
	{
		++healthRows;
		logSASDiagnosticLiteralLine("record_finalize.log", reason, SAS_DIAGNOSTIC_LOG_GAME_RECORD);
	}
	// <!-- custom: Use the extracted production function rather than copying its lifecycle guard into this test. (GPT-6.1-Sol) -->
	#include "record_finalization_under_test.inc"
}

// <!-- custom: Keep WM_CLOSE observable without letting the fixture window actually disappear; production only observes and chains the message, while this test WndProc suppresses destruction so two close-ordering scenarios can run in one process. (ChatGPT-5.6-Sol) -->
// <!-- custom: Handle SC_CLOSE directly in this fixture, without generating WM_CLOSE, to test the gap left by watching only direct WM_CLOSE delivery. (GPT-6.1-Sol) -->
static LRESULT CALLBACK runtimeCloseTestWindowProc(HWND hWindow, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
	if (uiMessage == WM_CLOSE || (uiMessage == WM_SYSCOMMAND && (wParam & 0xFFF0) == SC_CLOSE))
		return 0;
	return DefWindowProcW(hWindow, uiMessage, wParam, lParam);
}

int main(int argc, char** argv)
{
	wchar_t temporary[MAX_PATH];
	require(GetEnvironmentVariableW(L"SAS_DIAGNOSTIC_TEST_ROOT", temporary, MAX_PATH) > 0, "test root is provided");
	if (argc == 2 && strcmp(argv[1], "--record-finalization") == 0)
	{
		using namespace recordFinalizationFixture;
		std::wstring const root = std::wstring(temporary) + L"\\record_finalization";
		require(CreateDirectoryW(root.c_str(), NULL) != FALSE, "create recorder finalization fixture root");
		setSASDiagnosticLogRootDirectory(root.c_str());
		finalizeSASGameRecordLogSession();
		require(healthRows == 0 && readFile(root + L"\\Logs\\record_finalize.log").empty(), "startup reset creates no recorder health file");
		g_failOpen = true;
		logSASDiagnosticLiteralLine("BBAI_pending.log", "pending before session", SAS_DIAGNOSTIC_LOG_BBAI);
		require(g_iSASDiagnosticBufferedBytes > 0, "fixture holds pending shared output");
		g_failOpen = false;
		finalizeSASGameRecordLogSession();
		require(g_iSASDiagnosticBufferedBytes == 0 && readFile(root + L"\\Logs\\BBAI_pending.log").find("pending before session") != std::string::npos, "no-session reset still retries shared output");
		g_bSASGameRecordSessionOpen = true;
		g_bSASGameRecordBufferInitializingActions = true;
		g_iSASGameRecordPendingPlotTurn = 0;
		g_bSASGameRecordRngTrackingActive = true;
		finalizeSASGameRecordLogSession();
		require(setupFlushes == 1 && plotFlushes == 1 && rngCheckpoints == 1 && healthRows == 1, "aborted initialization preserves buffered observations and final health");
		finalizeSASGameRecordLogSession();
		require(healthRows == 1 && rngCheckpoints == 1, "repeated reset does not duplicate final rows");
		g_bSASGameRecordSessionOpen = true;
		finalizeSASGameRecordLogSession();
		require(healthRows == 2 && !g_bSASGameRecordSessionOpen, "next recording session gets its own final health");
		puts("PASS recorder finalization: startup, retry, aborted initialization, repeated reset and next session");
		return 0;
	}
	if (argc == 2 && strcmp(argv[1], "--runtime-status-disabled") == 0)
	{
		require(!shouldPublishAutoplayChange(0, 5, true), "disabled hook skips explicit autoplay changes");
		setSASDiagnosticLogRootDirectory(temporary);
		updateSASRuntimeStatusSessionStarted(false);
		updateSASRuntimeStatusSessionReady();
		updateSASRuntimeStatusTurnCompleted(3);
		updateSASRuntimeStatusAutoPlay(SAS_RUNTIME_STATUS_AUTOPLAY_STARTED);
		CyGame bridge(GC.game);
		require(!bridge.isSASRuntimeStatusRecordingEnabled(), "Python recording gate is disabled");
		bridge.updateSASRuntimeStatusMainMenuState(true);
		bridge.updateSASRuntimeStatusCivilopedia(true, true);
		bridge.updateSASRuntimeStatusPreGameStart();
		bridge.updateSASRuntimeStatusWorldBuilderScreen(true);
		bridge.updateSASRuntimeStatusWorldBuilderScreen(false);
		require(g_worldBuilderModeReads == 0 && g_advancedStartReads == 0, "disabled Python bridge skips native mode and Advanced Start queries");
		updateSASRuntimeStatusGameEnded();
		updateSASRuntimeStatusRequest(SAS_RUNTIME_STATUS_EXIT_REQUESTED);
		updateSASRuntimeStatusCleanExit();
		require(g_openCalls == 0 && g_dllIdentityLookups == 0 && g_uiSASRuntimeStatusWriteFailures == 0 && g_uiSASRuntimeActivitySerial == 0 && g_uiSASRuntimeStatusSerial == 0 && g_uiSASRuntimeSessionSerial == 0, "disabled status does not publish or accumulate session state");
		require(g_uiSASRuntimeHistorySessionMetadataCaptured == 0 && g_szSASRuntimeHistoryPendingSessionMetadata.empty(), "disabled status does not capture pending metadata");
		require(g_hSASRuntimeWindowMessageHook == NULL && g_hSASRuntimeWindowQueueHook == NULL && g_hSASRuntimeObservedWindow == NULL, "disabled status installs no window-message or queue hook");
		return 0;
	}
	// <!-- custom: A blocked output path must not stretch creation time until the next successful write; retain the observed interval and expose a missing history row through counters/serials. (GPT-6.1-Sol) -->
	if (argc == 2 && strcmp(argv[1], "--runtime-status-output-failure") == 0)
	{
		GC.runtimeStatus = true;
		std::wstring const root = std::wstring(temporary) + L"\\status_failure";
		require(CreateDirectoryW(root.c_str(), NULL) != FALSE, "create failed-output fixture root");
		// <!-- custom: A fully written run-metadata row with an unconfirmed close must not be appended again on the next update; failure counters preserve the uncertainty without duplicating identity. (ChatGPT-5.6-Sol) -->
		g_failClose = true;
		setSASRuntimeStatusRootDirectory(root);
		require(g_uiSASRuntimeHistoryRowsWritten == 1 && g_uiSASRuntimeHistoryWriteFailures == 1, "unconfirmed run metadata is not counted as confirmed but the following transition still publishes");
		g_runtimeClock = 100;
		g_failOpen = true;
		updateSASRuntimeStatusSessionStarted(false);
		g_failOpen = false;
		require(g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_CREATING_GAME && g_uiSASRuntimeHistoryWriteFailures == 2, "failed publication retains creation start and discloses lost transition");
		g_runtimeClock = 5100;
		GC.game.ready = true;
		updateSASRuntimeStatusSessionReady();
		require(g_iSASRuntimePreviousDurationMilliseconds == 5000, "creation time uses observed start despite failed output");
		require(CopyFileW((root + L"\\Logs\\SASRuntimeStatus.json").c_str(), (std::wstring(temporary) + L"\\runtime_failed_output.json").c_str(), FALSE) != FALSE, "retain recovered failure snapshot");
		std::wstring const recoveredHistoryName(g_szSASRuntimeHistoryFilename.begin(), g_szSASRuntimeHistoryFilename.end());
		std::string const recoveredHistory = readFile(root + L"\\Logs\\" + recoveredHistoryName);
		std::string const runRecord = "\"record\":\"run\"";
		size_t const firstRunRecord = recoveredHistory.find(runRecord);
		require(firstRunRecord != std::string::npos && recoveredHistory.find(runRecord, firstRunRecord + 1) == std::string::npos, "unconfirmed run metadata is not duplicated on recovery");
		// <!-- custom: A fully written history row can remain readable after close reports failure; counters mean confirmed successful appends, not a physical line count. (ChatGPT (review) + GPT-6.1-Sol) -->
		GC.game.autoplay = 5;
		g_failClose = true;
		updateSASRuntimeStatusAutoPlay(SAS_RUNTIME_STATUS_AUTOPLAY_STARTED);
		require(g_uiSASRuntimeHistoryRowsWritten == 3 && g_uiSASRuntimeHistoryWriteFailures == 3, "unconfirmed history close does not count as a confirmed append");
		std::wstring const historyName(g_szSASRuntimeHistoryFilename.begin(), g_szSASRuntimeHistoryFilename.end());
		require(readFile(root + L"\\Logs\\" + historyName).find("AUTOPLAY_STARTED") != std::string::npos, "row can exist after reported history close failure");
		// <!-- custom: Exercise each status-only failure path without creating more history transitions; recovery must expose failures even when a write acknowledged every byte before reporting failure. (GPT-6.1-Sol) -->
		g_failClose = true;
		updateSASRuntimeStatusTurnCompleted(9);
		g_failPartialWrite = true;
		updateSASRuntimeStatusTurnCompleted(9);
		g_failPartialWrite = false;
		g_zeroWrite = true;
		updateSASRuntimeStatusTurnCompleted(9);
		g_zeroWrite = false;
		setSASRuntimeStatusRootDirectory(root + L"\\missing_parent\\missing_child");
		setSASRuntimeStatusRootDirectory(root);
		require(g_uiSASRuntimeStatusWriteFailures == 5, "status failures include open, close, failed/zero write and missing root");
		require(CopyFileW((root + L"\\Logs\\SASRuntimeStatus.json").c_str(), (std::wstring(temporary) + L"\\runtime_failed_status.json").c_str(), FALSE) != FALSE, "retain recovered status-failure snapshot");
		return 0;
	}
	// <!-- custom: Failed first-ready output previously resampled a later turn/player as the original ready context.
	// Exercise recovery during play and after reset; the frozen session evidence must survive both while the fixed snapshot continues to reflect current state. (GPT-6.1-Sol) -->
	if (argc == 2 && strcmp(argv[1], "--runtime-session-metadata-retry") == 0)
	{
		GC.runtimeStatus = true;
		std::wstring const root = std::wstring(temporary) + L"\\session_metadata_retry";
		require(CreateDirectoryW(root.c_str(), NULL) != FALSE, "create session retry root");
		g_failOpen = true;
		setSASRuntimeStatusRootDirectory(root);
		updateSASRuntimeStatusSessionStarted(false);
		GC.game.ready = true;
		updateSASRuntimeStatusSessionReady();
		require(!g_bSASRuntimeHistoryMetadataWritten && !g_szSASRuntimeHistoryPendingSessionMetadata.empty(), "failed ready publication retains context even before run metadata can be written");
		g_failOpen = false;
		GC.game.gameTurn = 10;
		GC.game.activePlayer = 1;
		updateSASRuntimeStatusTurnCompleted(9);
		require(g_szSASRuntimeHistoryPendingSessionMetadata.empty(), "successful retry releases pending context");
		require(CopyFileW((root + L"\\Logs\\SASRuntimeStatus.json").c_str(), (std::wstring(temporary) + L"\\runtime_session_retry.json").c_str(), FALSE) != FALSE, "retain current-state retry snapshot");
		unsigned int const beforeTurnRows = g_uiSASRuntimeHistoryRowsWritten;
		updateSASRuntimeStatusTurnCompleted(9);
		require(g_uiSASRuntimeHistoryRowsWritten == beforeTurnRows, "published session context is not recreated on later turns");
		updateSASRuntimeStatusNoGame();
		updateSASRuntimeStatusSessionStarted(true);
		GC.game.gameTurn = 20;
		GC.game.activePlayer = 2;
		g_failOpen = true;
		updateSASRuntimeStatusSessionReady();
		g_failOpen = false;
		GC.game.ready = false;
		updateSASRuntimeStatusNoGame();
		require(g_szSASRuntimeHistoryPendingSessionMetadata.empty(), "menu/reset can publish the last pending ready context without querying live gameplay");
		updateSASRuntimeStatusSessionStarted(false);
		GC.game.ready = true;
		GC.game.gameTurn = 30;
		GC.game.activePlayer = 3;
		g_failClose = true;
		updateSASRuntimeStatusSessionReady();
		require(g_szSASRuntimeHistoryPendingSessionMetadata.empty(), "complete session bytes with an uncertain close are not queued again");
		unsigned int const beforeLaterTurnRows = g_uiSASRuntimeHistoryRowsWritten;
		GC.game.gameTurn = 31;
		GC.game.activePlayer = 4;
		updateSASRuntimeStatusTurnCompleted(30);
		require(g_uiSASRuntimeHistoryRowsWritten == beforeLaterTurnRows, "uncertain session close does not cause a duplicate on the next turn");
		require(CopyFileW((root + L"\\Logs\\SASRuntimeStatus.json").c_str(), (std::wstring(temporary) + L"\\runtime_session_uncertain.json").c_str(), FALSE) != FALSE, "retain uncertain-session accounting snapshot");
		return 0;
	}
	// <!-- custom: A full history write with an unconfirmed close can leave physical evidence while confirmed-row count remains zero; CvAltRoot migration must still carry that possible evidence forward. (ChatGPT-5.6-Sol) -->
	if (argc == 2 && strcmp(argv[1], "--runtime-history-uncertain-migration") == 0)
	{
		std::wstring const oldRoot = std::wstring(temporary) + L"\\history_uncertain_old";
		std::wstring const newRoot = std::wstring(temporary) + L"\\history_uncertain_new";
		require(CreateDirectoryW(oldRoot.c_str(), NULL) != FALSE, "create uncertain old root");
		require(CreateDirectoryW(newRoot.c_str(), NULL) != FALSE, "create uncertain new root");
		require(CreateDirectoryW((oldRoot + L"\\Logs").c_str(), NULL) != FALSE, "create uncertain old Logs");
		require(CreateDirectoryW((newRoot + L"\\Logs").c_str(), NULL) != FALSE, "create uncertain new Logs");
		g_szSASRuntimeHistoryFilename = "SASRuntimeHistory_uncertain.jsonl";
		g_szSASRuntimeHistoryRoot = oldRoot;
		g_failClose = true;
		bool bContentComplete = false;
		require(!recordSASRuntimeHistoryRow("{\"record\":\"uncertain\"}\r\n", &bContentComplete), "unconfirmed close is not a confirmed history append");
		require(bContentComplete && g_uiSASRuntimeHistoryRowsWritten == 0 && g_uiSASRuntimeHistoryWriteFailures == 1 && g_bSASRuntimeHistoryMayContainRows, "full unconfirmed row retains migration evidence despite zero confirmed rows");
		require(prepareSASRuntimeHistory(newRoot) && g_bSASRuntimeHistoryAtCurrentRoot, "uncertain history migrates to authoritative root");
		require(readFile(newRoot + L"\\Logs\\SASRuntimeHistory_uncertain.jsonl").find("uncertain") != std::string::npos, "uncertain physical history row survives root migration");
		return 0;
	}

	// <!-- custom: Exercise passive presentation/lifecycle observations separately from the baseline timing fixture so new boundaries do not rewrite its established duration expectations. (ChatGPT-5.6-Sol) -->
	if (argc == 2 && strcmp(argv[1], "--runtime-presentation") == 0)
	{
		GC.runtimeStatus = true;
		std::wstring const root = std::wstring(temporary) + L"\\runtime_presentation";
		require(CreateDirectoryW(root.c_str(), NULL) != FALSE, "create runtime-presentation fixture root");
		setSASRuntimeStatusRootDirectory(root);

		CyGame bridge(GC.game);
		g_runtimeClock = 100;
		unsigned int before = g_uiSASRuntimeStatusSerial;
		updateSASRuntimeStatusMainMenuSceneRequested();
		require(g_szSASRuntimeMainMenuSceneRequestedUtc != "-" && g_uiSASRuntimeStatusSerial == before + 1, "first main-menu scene request is observed");
		before = g_uiSASRuntimeStatusSerial;
		updateSASRuntimeStatusMainMenuSceneRequested();
		require(g_uiSASRuntimeStatusSerial == before, "duplicate main-menu scene requests are suppressed");

		require(bridge.isSASRuntimeStatusRecordingEnabled(), "Python recording gate is enabled");
		g_runtimeClock = 140;
		before = g_uiSASRuntimeStatusSerial;
		bridge.updateSASRuntimeStatusMainMenuState(false);
		require(g_uiSASRuntimeStatusSerial == before && g_szSASRuntimeMainMenuStateObservedUtc == "-", "negative main-menu state sample is not published");
		bridge.updateSASRuntimeStatusCivilopedia(true, false);
		require(g_uiSASRuntimeStatusSerial == before && !g_bSASRuntimeCivilopediaVisible, "in-game overlay needs a ready game context");
		g_runtimeClock = 150;
		bridge.updateSASRuntimeStatusMainMenuState(true);
		require(g_szSASRuntimeMainMenuStateObservedUtc != "-" && g_uiSASRuntimeStatusSerial == before + 1, "positive main-menu state sample is observed");
		g_runtimeClock = 160;
		bridge.updateSASRuntimeStatusCivilopedia(true, true);
		require(g_bSASRuntimeCivilopediaVisible, "confirmed main-menu Civilopedia show is observed");
		before = g_uiSASRuntimeStatusSerial;
		bridge.updateSASRuntimeStatusCivilopedia(true, true);
		require(g_uiSASRuntimeStatusSerial == before, "duplicate main-menu Civilopedia shows are suppressed");
		g_runtimeClock = 180;
		bridge.updateSASRuntimeStatusCivilopedia(false, false);
		require(!g_bSASRuntimeCivilopediaVisible, "main-menu Civilopedia close is observed");

		g_runtimeClock = 1000;
		updateSASRuntimeStatusSessionStarted(false);
		CvString const creationStartedUtc = g_szSASRuntimeActivityStartedUtc;
		require(g_szSASRuntimeMainMenuSceneRequestedUtc == "-" && g_szSASRuntimeMainMenuStateObservedUtc == "-" && g_szSASRuntimeLoadingScreenAssetRequestedUtc == "-" && g_szSASRuntimePythonPreGameStartUtc == "-" && !g_bSASRuntimeCivilopediaVisible, "session start resets presentation observations");
		g_runtimeClock = 1250;
		before = g_uiSASRuntimeStatusSerial;
		updateSASRuntimeStatusLoadingScreenAssetRequested();
		require(g_szSASRuntimeLoadingScreenAssetRequestedUtc != "-" && g_uiSASRuntimeStatusSerial == before + 1 && g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_CREATING_GAME, "first loading asset request preserves creation activity");
		require(g_szSASRuntimeActivityStartedUtc == creationStartedUtc && g_uiSASRuntimeActivityStartedMilliseconds == 1000, "loading presentation does not restart creation timing");
		before = g_uiSASRuntimeStatusSerial;
		updateSASRuntimeStatusLoadingScreenAssetRequested();
		require(g_uiSASRuntimeStatusSerial == before, "duplicate loading asset requests are suppressed");
		g_runtimeClock = 1500;
		bridge.updateSASRuntimeStatusPreGameStart();
		require(g_szSASRuntimePythonPreGameStartUtc != "-" && g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_CREATING_GAME && g_uiSASRuntimeActivityStartedMilliseconds == 1000, "Python PreGameStart preserves creation activity timing");
		before = g_uiSASRuntimeStatusSerial;
		bridge.updateSASRuntimeStatusPreGameStart();
		require(g_uiSASRuntimeStatusSerial == before, "duplicate Python PreGameStart observations are suppressed");

		GC.game.ready = true;
		GC.game.state = GAMESTATE_ON;
		g_runtimeClock = 2000;
		updateSASRuntimeStatusSessionReady();
		require(g_iSASRuntimePreviousDurationMilliseconds == 1000, "creation completion retains its whole interval across loading presentation");
		bridge.updateSASRuntimeStatusCivilopedia(true, false);
		require(g_bSASRuntimeCivilopediaVisible && g_eSASRuntimeCivilopediaContext == SAS_RUNTIME_CIVILOPEDIA_IN_GAME && g_iSASRuntimeCivilopediaActivePlayer == 0 && g_iSASRuntimeCivilopediaActiveCivilization == 4, "in-game Civilopedia records active player and civilization");
		bridge.updateSASRuntimeStatusCivilopedia(false, false);
		require(!g_bSASRuntimeCivilopediaVisible && g_iSASRuntimeCivilopediaActivePlayer == 0, "hide retains the observed overlay owner");
		g_runtimeTestDLL.worldBuilder = true;
		GC.game.advancedStart = true;
		before = g_uiSASRuntimeStatusSerial;
		bridge.updateSASRuntimeStatusWorldBuilderScreen(true);
		require(g_uiSASRuntimeStatusSerial == before && !g_bSASRuntimeWorldBuilderMode, "real bridge rejects Advanced Start even when EXE editor mode is enabled");
		GC.game.advancedStart = false;
		g_runtimeClock = 2200;
		updateSASRuntimeStatusWorldBuilderMode(true);
		require(g_bSASRuntimeWorldBuilderMode && !g_bSASRuntimeWorldBuilderScreenVisible && g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_PLAYING, "native WorldBuilder mode is distinct from visible editor activity");
		g_runtimeClock = 2300;
		bridge.updateSASRuntimeStatusWorldBuilderScreen(true);
		require(g_bSASRuntimeWorldBuilderScreenVisible && g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_WORLD_BUILDER, "visible WorldBuilder screen starts editor activity");
		g_runtimeClock = 4300;
		updateSASRuntimeStatusWorldBuilderMode(false);
		require(!g_bSASRuntimeWorldBuilderMode && g_bSASRuntimeWorldBuilderScreenVisible && g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_PLAYING && g_iSASRuntimePreviousDurationMilliseconds == 2000, "native WorldBuilder exit ends editor activity without erasing the independent screen observation");
		g_runtimeClock = 4400;
		g_runtimeTestDLL.worldBuilder = false;
		bridge.updateSASRuntimeStatusWorldBuilderScreen(false);
		require(!g_bSASRuntimeWorldBuilderScreenVisible && g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_PLAYING, "later WorldBuilder hide records presentation closure after mode exit");
		require(g_uiSASRuntimeActivityStartedMilliseconds == 4300 && g_iSASRuntimePreviousDurationMilliseconds == 2000, "hide-only observation preserves current play start and completed editor duration");
		before = g_uiSASRuntimeStatusSerial;
		GC.game.advancedStart = true;
		bridge.updateSASRuntimeStatusWorldBuilderScreen(true);
		require(g_uiSASRuntimeStatusSerial == before && !g_bSASRuntimeWorldBuilderScreenVisible, "Advanced Start screen reuse is ignored without native WorldBuilder mode");
		GC.game.advancedStart = false;

		GC.game.state = GAMESTATE_OVER;
		g_runtimeClock = 5000;
		updateSASRuntimeStatusGameEnded();
		require(g_eSASRuntimeGameState == SAS_RUNTIME_GAME_GAME_ENDED, "victory enters ended runtime state");
		GC.game.state = GAMESTATE_EXTENDED;
		g_runtimeClock = 5200;
		updateSASRuntimeStatusExtendedGameStarted();
		require(g_eSASRuntimeGameState == SAS_RUNTIME_GAME_PLAYING && g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_PLAYING, "extended game restores active play");
		require(CopyFileW((root + L"\\Logs\\SASRuntimeStatus.json").c_str(), (std::wstring(temporary) + L"\\runtime_presentation.json").c_str(), FALSE) != FALSE, "retain runtime-presentation status snapshot");
		std::wstring const historyName(g_szSASRuntimeHistoryFilename.begin(), g_szSASRuntimeHistoryFilename.end());
		require(CopyFileW((root + L"\\Logs\\" + historyName).c_str(), (std::wstring(temporary) + L"\\runtime_presentation.jsonl").c_str(), FALSE) != FALSE, "retain runtime-presentation history");
		// <!-- custom: Exercise Python-first mode/show plus screen-hide-before-native-mode-off, complementing the native-first ordering above. (GPT-6.1-Sol) -->
		g_runtimeClock = 6000;
		g_runtimeTestDLL.worldBuilder = true;
		bridge.updateSASRuntimeStatusWorldBuilderScreen(true);
		require(g_bSASRuntimeWorldBuilderMode && g_bSASRuntimeWorldBuilderScreenVisible, "Python-first show observes mode and visibility together");
		before = g_uiSASRuntimeStatusSerial;
		updateSASRuntimeStatusWorldBuilderMode(true);
		require(g_uiSASRuntimeStatusSerial == before, "later native mode notification deduplicates Python-first observation");
		g_runtimeClock = 7200;
		g_runtimeTestDLL.worldBuilder = false;
		bridge.updateSASRuntimeStatusWorldBuilderScreen(false);
		require(!g_bSASRuntimeWorldBuilderMode && !g_bSASRuntimeWorldBuilderScreenVisible && g_iSASRuntimePreviousDurationMilliseconds == 1200, "screen-first hide preserves the full editor interval through subsequent mode observation");
		g_runtimeClock = 7300;
		bridge.updateSASRuntimeStatusWorldBuilderScreen(true);
		require(!g_bSASRuntimeWorldBuilderMode && g_bSASRuntimeWorldBuilderScreenVisible && g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_PLAYING, "show arriving before native mode is retained without inventing editor activity");
		g_runtimeClock = 7400;
		g_runtimeTestDLL.worldBuilder = true;
		updateSASRuntimeStatusWorldBuilderMode(true);
		require(g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_WORLD_BUILDER, "later native mode completes the previously observed editor visibility");
		g_runtimeClock = 7600;
		g_runtimeTestDLL.worldBuilder = false;
		bridge.updateSASRuntimeStatusWorldBuilderScreen(false);
		require(g_iSASRuntimePreviousDurationMilliseconds == 200, "editor duration begins only after both observations are present");
		g_runtimeClock = 8000;
		g_runtimeTestDLL.worldBuilder = true;
		bridge.updateSASRuntimeStatusWorldBuilderScreen(true);
		updateSASRuntimeStatusRequest(SAS_RUNTIME_STATUS_EXIT_REQUESTED);
		updateSASRuntimeStatusTurnCompleted(9);
		require(g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_EXIT_REQUESTED, "late callbacks cannot replace an exit request with a still-visible editor");
		return 0;
	}
	if (argc == 2 && strcmp(argv[1], "--runtime-window-close") == 0)
	{
		GC.runtimeStatus = true;
		std::wstring const root = std::wstring(temporary) + L"\\runtime_window_close";
		require(CreateDirectoryW(root.c_str(), NULL) != FALSE, "create runtime-window-close fixture root");
		// <!-- custom: First menu status can precede window visibility.
		// Arm before creating the fixture HWND, then require a close to bind it without another lifecycle update. (GPT-6.1-Sol) -->
		setSASRuntimeStatusRootDirectory(root);
		require(g_hSASRuntimeWindowMessageHook != NULL && g_hSASRuntimeWindowQueueHook != NULL && g_hSASRuntimeObservedWindow == NULL, "early status arms both hooks before a top-level window exists");
		WNDCLASSW kClass;
		ZeroMemory(&kClass, sizeof(kClass));
		kClass.lpfnWndProc = runtimeCloseTestWindowProc;
		kClass.hInstance = GetModuleHandleW(NULL);
		kClass.lpszClassName = L"SASRuntimeCloseFixtureWindow";
		require(RegisterClassW(&kClass) != 0, "register runtime close fixture window");
		HWND const hWindow = CreateWindowW(kClass.lpszClassName, L"SAS runtime close fixture", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 320, 200, NULL, NULL, kClass.hInstance, NULL);
		require(hWindow != NULL, "create runtime close fixture window");
		ShowWindow(hWindow, SW_SHOW);
		UpdateWindow(hWindow);
		unsigned int const uiBeforeUnrelatedMessage = g_uiSASRuntimeActivitySerial;
		SendMessageW(hWindow, WM_NULL, 0, 0);
		require(g_uiSASRuntimeActivitySerial == uiBeforeUnrelatedMessage && g_szSASRuntimeWindowCloseRequestObservedUtc == "-", "ordinary messages perform no status publication");
		SendMessageW(hWindow, WM_SYSCOMMAND, SC_CLOSE | 3, 0);
		require(g_hSASRuntimeObservedWindow == hWindow && g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_EXIT_REQUESTED, "close command binds the later window and records its request even without a WM_CLOSE");
		updateSASRuntimeStatusNoGame();

		g_runtimeClock = 100;
		updateSASRuntimeStatusSessionStarted(false);
		GC.game.ready = true;
		g_runtimeClock = 200;
		updateSASRuntimeStatusSessionReady();
		GC.game.autoplay = 5;
		g_runtimeClock = 300;
		updateSASRuntimeStatusAutoPlay(SAS_RUNTIME_STATUS_AUTOPLAY_STARTED);
		g_runtimeClock = 400;
		SendMessageW(hWindow, WM_CLOSE, 0, 0);
		require(g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_EXIT_REQUESTED && g_szSASRuntimeWindowCloseRequestObservedUtc != "-", "WM_CLOSE during autoplay records an observed exit request without consuming the message");

		GC.game.autoplay = 0;
		GC.game.ready = false;
		g_runtimeClock = 500;
		updateSASRuntimeStatusNoGame();
		g_runtimeClock = 600;
		updateSASRuntimeStatusRequest(SAS_RUNTIME_STATUS_EXIT_REQUESTED);
		uint const uiExplicitExitStart = g_uiSASRuntimeActivityStartedMilliseconds;
		g_runtimeClock = 700;
		SendMessageW(hWindow, WM_CLOSE, 0, 0);
		require(g_eSASRuntimeActivity == SAS_RUNTIME_ACTIVITY_EXIT_REQUESTED && g_uiSASRuntimeActivityStartedMilliseconds == uiExplicitExitStart, "WM_CLOSE after an explicit UI exit remains observation-only and does not restart the exit interval");

		// <!-- custom: A game message loop may remove a close and exit without calling DispatchMessage; the old SendMessage-only fixture could not expose that blind spot.
		// PM_NOREMOVE inspection must not publish, and the observer must leave the actual retrieved MSG unchanged. (GPT-6.1-Sol) -->
		MSG kQueued;
		require(PostMessageW(hWindow, WM_CLOSE, 0, 0) != FALSE, "post close for an undispatched message-loop case");
		unsigned int const uiBeforePeek = g_uiSASRuntimeActivitySerial;
		require(PeekMessageW(&kQueued, hWindow, WM_CLOSE, WM_CLOSE, PM_NOREMOVE) != FALSE, "peek queued close without removing it");
		require(PeekMessageW(&kQueued, hWindow, WM_CLOSE, WM_CLOSE, PM_NOREMOVE) != FALSE && g_uiSASRuntimeActivitySerial == uiBeforePeek, "repeated peeks create no duplicate observation");
		require(PeekMessageW(&kQueued, hWindow, WM_CLOSE, WM_CLOSE, PM_REMOVE) != FALSE, "remove close without dispatching it");
		require(kQueued.hwnd == hWindow && kQueued.message == WM_CLOSE && g_uiSASRuntimeActivitySerial == uiBeforePeek + 1, "undispatched close is observed without modifying its message");
		require(g_uiSASRuntimeActivityStartedMilliseconds == uiExplicitExitStart, "queued observation preserves the existing exit interval");
		require(PostMessageW(hWindow, WM_SYSCOMMAND, SC_CLOSE | 3, 0) != FALSE, "post close system command with reserved low bits");
		require(PeekMessageW(&kQueued, hWindow, WM_SYSCOMMAND, WM_SYSCOMMAND, PM_REMOVE) != FALSE, "remove close system command");
		require(kQueued.wParam == (SC_CLOSE | 3), "system command remains unchanged for Civ4");
		DispatchMessageW(&kQueued);
		require(g_uiSASRuntimeActivitySerial == uiBeforePeek + 2 && g_uiSASRuntimeActivityStartedMilliseconds == uiExplicitExitStart, "posted commands are observed at queue removal even when DispatchMessage bypasses the sent-message hook");

		// <!-- custom: Exercise the stale-HWND recovery path without adding another close transition: Civ4 normally keeps one main window, but an observer must not remain pinned to a destroyed handle if the EXE recreates it. (ChatGPT-5.6-Sol) -->
		require(DestroyWindow(hWindow) != FALSE, "destroy original runtime close fixture window");
		HWND const hReplacement = CreateWindowW(kClass.lpszClassName, L"SAS runtime close replacement", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 320, 200, NULL, NULL, kClass.hInstance, NULL);
		require(hReplacement != NULL, "create replacement runtime close fixture window");
		ShowWindow(hReplacement, SW_SHOW);
		UpdateWindow(hReplacement);
		ensureSASRuntimeWindowMessageHook();
		require(g_hSASRuntimeWindowMessageHook != NULL && g_hSASRuntimeObservedWindow == hReplacement, "observer re-arms on a replacement top-level window after the original HWND disappears");

		updateSASRuntimeStatusCleanExit();
		require(g_hSASRuntimeWindowMessageHook == NULL && g_hSASRuntimeWindowQueueHook == NULL && g_hSASRuntimeObservedWindow == NULL, "orderly shutdown releases both window observers after final publication");
		require(CopyFileW((root + L"\\Logs\\SASRuntimeStatus.json").c_str(), (std::wstring(temporary) + L"\\runtime_window_close.json").c_str(), FALSE) != FALSE, "retain window-close status snapshot");
		std::wstring const historyName(g_szSASRuntimeHistoryFilename.begin(), g_szSASRuntimeHistoryFilename.end());
		require(CopyFileW((root + L"\\Logs\\" + historyName).c_str(), (std::wstring(temporary) + L"\\runtime_window_close.jsonl").c_str(), FALSE) != FALSE, "retain window-close runtime history");
		DestroyWindow(hReplacement);
		UnregisterClassW(kClass.lpszClassName, kClass.hInstance);
		return 0;
	}
	if (argc == 2 && strcmp(argv[1], "--runtime-status") == 0)
	{
		GC.runtimeStatus = true;
		require(!shouldPublishAutoplayChange(5, 4, false), "ordinary countdown uses only the turn-completion update");
		require(shouldPublishAutoplayChange(0, 5, true) && shouldPublishAutoplayChange(5, 0, false) && shouldPublishAutoplayChange(5, 8, true), "explicit start/adjustment and automatic end publish immediately");
		GC.overwrite = false;
		std::wstring const fallback = std::wstring(temporary) + L"\\status_fallback";
		std::wstring const root = std::wstring(temporary) + L"\\status_\x0416\x6f22";
		std::wstring const snapshots = std::wstring(temporary) + L"\\runtime_snapshots";
		require(CreateDirectoryW(fallback.c_str(), NULL) && CreateDirectoryW(root.c_str(), NULL) && CreateDirectoryW(snapshots.c_str(), NULL), "create runtime fixture directories");
		setSASDiagnosticLogRootDirectory(fallback.c_str());
		logSASDiagnosticLiteralLine("BBAI_root_lock.log", "historical root stays fixed", SAS_DIAGNOSTIC_LOG_BBAI);
		g_failOpen = true;
		setSASDiagnosticLogRootDirectory(root.c_str());
		require(g_szSASDiagnosticLogRootDirectory == fallback && g_szSASRuntimeStatusRootDirectory == root, "status migration is independent of committed historical handles");
		require(GetFileAttributesW((fallback + L"\\Logs\\SASRuntimeStatus.json").c_str()) != INVALID_FILE_ATTRIBUTES, "failed migration preserves fallback");
		std::wstring const anotherFailedRoot = root + L"_failed";
		require(CreateDirectoryW(anotherFailedRoot.c_str(), NULL) != FALSE, "create second failed handoff root");
		setSASRuntimeStatusRootDirectory(anotherFailedRoot);
		g_failOpen = false;
		g_failClose = true;
		setSASRuntimeStatusRootDirectory(root);
		require(GetFileAttributesW((fallback + L"\\Logs\\SASRuntimeStatus.json").c_str()) != INVALID_FILE_ATTRIBUTES, "failed close preserves fallback");
		setSASRuntimeStatusRootDirectory(root);
		require(GetFileAttributesW((fallback + L"\\Logs\\SASRuntimeStatus.json").c_str()) == INVALID_FILE_ATTRIBUTES, "successful migration removes obsolete fallback");
		// <!-- custom: BUG/native root spellings can differ only by case; migration cleanup must not delete the same physical JSON that was just rewritten. (GPT-6.1-Sol) -->
		std::wstring sameRoot = root;
		// <!-- custom: ASCII case changes are sufficient for this regression; CharUpperBuffW caused LNK2019 because the standalone fixture does not link User32. (GPT-6.1-Sol) -->
		for (size_t i = 0; i < sameRoot.size(); i++)
		{
			if (sameRoot[i] >= L'a' && sameRoot[i] <= L'z')
				sameRoot[i] -= L'a' - L'A';
		}
		setSASRuntimeStatusRootDirectory(sameRoot);
		require(GetFileAttributesW((root + L"\\Logs\\SASRuntimeStatus.json").c_str()) != INVALID_FILE_ATTRIBUTES, "same root with different casing retains current JSON");
		for (size_t i = 0; i < sameRoot.size(); i++)
		{
			if (sameRoot[i] == L'\\')
				sameRoot[i] = L'/';
		}
		setSASRuntimeStatusRootDirectory(sameRoot);
		require(GetFileAttributesW((root + L"\\Logs\\SASRuntimeStatus.json").c_str()) != INVALID_FILE_ATTRIBUTES, "same root with slash/case changes retains current JSON");
		snapshotStatus(root, snapshots, L"no_game");
		GC.game.ready = true;
		g_runtimeClock = 100;
		updateSASRuntimeStatusSessionStarted(false);
		snapshotStatus(root, snapshots, L"starting");
		g_runtimeClock = 15900;
		updateSASRuntimeStatusSessionReady();
		snapshotStatus(root, snapshots, L"ready");
		g_runtimeClock = 19100;
		GC.game.autoplay = 5;
		updateSASRuntimeStatusAutoPlay(SAS_RUNTIME_STATUS_AUTOPLAY_STARTED);
		snapshotStatus(root, snapshots, L"autoplay");
		g_runtimeClock = 25000;
		unsigned int const beforeTurnRows = g_uiSASRuntimeHistoryRowsWritten;
		updateSASRuntimeStatusTurnCompleted(4);
		require(g_uiSASRuntimeHistoryRowsWritten == beforeTurnRows, "ordinary turn does not append another activity interval");
		snapshotStatus(root, snapshots, L"turn");
		g_runtimeClock = 35100;
		GC.game.autoplay = 0;
		updateSASRuntimeStatusAutoPlay(SAS_RUNTIME_STATUS_AUTOPLAY_ENDED, SAS_AUTOPLAY_END_USER_INTERRUPTED);
		snapshotStatus(root, snapshots, L"interrupted");
		g_runtimeClock = 35200;
		updateSASRuntimeStatusRequest(SAS_RUNTIME_STATUS_RETURN_TO_MENU_REQUESTED);
		snapshotStatus(root, snapshots, L"menu_request");
		g_runtimeClock = 39000;
		GC.game.ready = false;
		updateSASRuntimeStatusNoGame();
		snapshotStatus(root, snapshots, L"menu_reset");
		g_runtimeClock = 39200;
		GC.game.ready = true;
		updateSASRuntimeStatusSessionStarted(true);
		snapshotStatus(root, snapshots, L"load_starting");
		g_runtimeClock = 42400;
		updateSASRuntimeStatusSessionReady();
		snapshotStatus(root, snapshots, L"loaded");
		updateSASRuntimeStatusGameEnded();
		snapshotStatus(root, snapshots, L"ended");
		updateSASRuntimeStatusNoGame();
		snapshotStatus(root, snapshots, L"reset");
		updateSASRuntimeStatusRequest(SAS_RUNTIME_STATUS_EXIT_REQUESTED);
		snapshotStatus(root, snapshots, L"exit_request");
		updateSASRuntimeStatusCleanExit();
		snapshotStatus(root, snapshots, L"exit");
		std::wstring const historyName(g_szSASRuntimeHistoryFilename.begin(), g_szSASRuntimeHistoryFilename.end());
		std::wstring const historyPath = root + L"\\Logs\\" + historyName;
		std::string const beforeFailure = readFile(historyPath);
		g_maxWrite = 7;
		g_failPartialWrite = true;
		require(!appendSASRuntimeHistory("{\"failedProbe\":true}\r\n"), "partial history write reports failure");
		g_failPartialWrite = false;
		g_maxWrite = 0xffffffff;
		require(readFile(historyPath) == beforeFailure, "partial history write rolls back without harming prior intervals");
		require(CopyFileW(historyPath.c_str(), (snapshots + L"\\timeline.jsonl").c_str(), FALSE) != FALSE, "retain timeline fixture for JSON parsing");
		require(g_dllIdentityLookups == 2, "runtime DLL identity formats each whitespace representation once across lifecycle snapshots");
		return 0;
	}
	// <!-- custom: A separate process preserves genuine unresolved-root startup state while exercising terminal fallback after an early initialization abort. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	if (argc == 2 && strcmp(argv[1], "--terminal-fallback") == 0)
	{
		g_shellFolderUnavailable = true;
		require(SetEnvironmentVariableW(L"USERPROFILE", temporary) != FALSE, "redirect fallback profile into fixture");
		std::wstring const documents = std::wstring(temporary) + L"\\Documents";
		std::wstring const games = documents + L"\\My Games";
		std::wstring const fallback = games + L"\\" + getSASDiagnosticExecutableFolder();
		require(CreateDirectoryW(documents.c_str(), NULL) != FALSE, "create fallback documents fixture");
		require(CreateDirectoryW(games.c_str(), NULL) != FALSE, "create fallback games fixture");
		require(CreateDirectoryW(fallback.c_str(), NULL) != FALSE, "create fallback game root fixture");
		logSASDiagnosticLiteralLine("SASGameRecord_early_abort.log", "initialization ended before BUG", SAS_DIAGNOSTIC_LOG_GAME_RECORD);
		require(!flushSASDiagnosticLogs(true) && g_openCalls == 0, "rollover still preserves unresolved output");
		require(flushSASDiagnosticLogs(true, true), "terminal flush resolves fallback without a BUG handoff");
		require(getSASDiagnosticLogBuffer("SASGameRecord_early_abort.log", SAS_DIAGNOSTIC_LOG_GAME_RECORD).hFile == INVALID_HANDLE_VALUE, "terminal flush closes its handle");
		require(g_szSASDiagnosticLogRootDirectory == fallback, "terminal output uses the isolated fallback root");
		require(readFile(fallback + L"\\Logs\\SASGameRecord_early_abort.log") == "initialization ended before BUG\r\n", "terminal fallback preserves pending startup diagnostics");
		puts("PASS C++ diagnostic writer: terminal pre-BUG fallback preserves early-abort output");
		return 0;
	}
	// <!-- custom: Terminate without CRT/static destructors or an explicit checkpoint; the parent verifies that both logs retain the final successfully submitted row. (GPT-6.1-Sol) -->
	if (argc == 2 && strcmp(argv[1], "--abrupt-tail") == 0)
	{
		std::wstring const crashRoot = std::wstring(temporary) + L"\\abrupt";
		require(CreateDirectoryW(crashRoot.c_str(), NULL) != FALSE, "create abrupt-termination fixture root");
		setSASDiagnosticLogRootDirectory(crashRoot.c_str());
		logSASDiagnosticLiteralLine("BBAI_crash.log", "BBAI last evidence 75%", SAS_DIAGNOSTIC_LOG_BBAI);
		logSASDiagnosticLiteralLine("SASGameRecord_crash.log", "SGR last evidence 75%", SAS_DIAGNOSTIC_LOG_GAME_RECORD);
		logSASDiagnosticLiteralLine("BBAI_crash.log", "final BBAI tail", SAS_DIAGNOSTIC_LOG_BBAI);
		logSASDiagnosticLiteralLine("SASGameRecord_crash.log", "final SGR tail", SAS_DIAGNOSTIC_LOG_GAME_RECORD);
		require(g_iSASDiagnosticBufferedBytes == 0, "successful rows do not wait for a turn checkpoint");
		TerminateProcess(GetCurrentProcess(), 73);
		require(false, "abrupt termination must not return");
	}
	std::wstring const root = std::wstring(temporary) + L"\\Unicode_\x0416\x6f22";
	require(CreateDirectoryW(root.c_str(), NULL) != FALSE, "create Unicode root outside the ANSI alphabet");
	require(CreateDirectoryW((root + L"\\Logs").c_str(), NULL) != FALSE, "create log directory");
	int const beforeRoot = g_openCalls;
	require(flushSASDiagnosticLogs(false), "empty writer does not resolve a fallback root");
	logSASDiagnosticLiteralLine("SASGameRecord_pre_root.log", "before BUG initializes", SAS_DIAGNOSTIC_LOG_GAME_RECORD);
	require(!flushSASDiagnosticLogs(true) && g_openCalls == beforeRoot, "early native rollover waits for authoritative root");
	std::string const startupRow(1024 * 1024, 's');
	for (int startup = 0; startup < 18; startup++)
		logSASDiagnosticLiteralLine("SASGameRecord_startup_budget.log", startupRow.c_str(), SAS_DIAGNOSTIC_LOG_GAME_RECORD);
	require(g_openCalls == beforeRoot && !g_bSASDiagnosticLogRootResolved, "pre-BUG overflow never auto-binds a fallback root");
	require(g_iSASDiagnosticBufferedBytes <= SAS_DIAGNOSTIC_MAX_BUFFERED_BYTES && getSASDiagnosticLogBuffer("SASGameRecord_startup_budget.log", SAS_DIAGNOSTIC_LOG_GAME_RECORD).uiDroppedRows > 0, "pre-BUG overflow is bounded and accounted");
	setSASDiagnosticLogRootDirectory((root + L"\\").c_str());
	require(g_szSASDiagnosticLogRootDirectory == root, "root is Unicode and trailing separator is trimmed");
	require(flushSASDiagnosticLogs(false), "pre-root batch flushes after BUG supplies root");
	require(readFile(root + L"\\Logs\\SASGameRecord_pre_root.log") == "before BUG initializes\r\n", "early batch uses authoritative root");

	int const initialOpens = g_openCalls;
	require(flushSASDiagnosticLogs(false) && g_openCalls == initialOpens, "empty/disabled writer does no I/O");
	seedFile(root + L"\\Logs\\BBAI.log");
	g_inspectWhileWriting = true;
	logSASDiagnosticLiteralLine("BBAI.log", "literal=75% %n %%", SAS_DIAGNOSTIC_LOG_BBAI);
	require(readFile(root + L"\\Logs\\BBAI.log") == "literal=75% %n %%\r\n", "ordinary row is visible before any checkpoint");
	require(CopyFileW((root + L"\\Logs\\BBAI.log").c_str(), (root + L"\\BBAI_live_copy.log").c_str(), FALSE) != FALSE, "Windows copy succeeds while the writer handle remains open");
	require(readFile(root + L"\\BBAI_live_copy.log") == "literal=75% %n %%\r\n", "live copy includes the completed row");
	int const afterFirstRow = g_openCalls;
	require(getSASDiagnosticLogBuffer("BBAI.log", SAS_DIAGNOSTIC_LOG_BBAI).hFile != INVALID_HANDLE_VALUE, "successful row retains shared handle");
	require(g_uiSASDiagnosticBBAIWrittenBytes == strlen("literal=75% %n %%\r\n"), "BBAI written bytes count accepted output");
	require(g_uiSASDiagnosticWrittenBytes == g_uiSASDiagnosticBBAIWrittenBytes + g_uiSASDiagnosticGameRecordWrittenBytes + g_uiSASDiagnosticOtherWrittenBytes, "process written bytes equal all families including startup output");
	require(getSASDiagnosticWriterHealthFields().find("bbaiProcessWrittenBytes=") != std::string::npos, "health fields expose per-log-family written bytes");
	require(flushSASDiagnosticLogs(false), "first checkpoint succeeds");
	require(readFile(root + L"\\Logs\\BBAI.log") == "literal=75% %n %%\r\n", "first overwrite and literal percent preservation");
	logSASDiagnosticLiteralLine("BBAI.log", "next", SAS_DIAGNOSTIC_LOG_BBAI);
	require(g_openCalls == afterFirstRow, "later rows reuse the open handle");
	// <!-- custom: Explicit families must survive renamed files and misleading prefixes; filename-based classification would misattribute both fixtures. (GPT-6.1-Sol) -->
	unsigned __int64 const bbaiBeforeRename = g_uiSASDiagnosticBBAIWrittenBytes;
	unsigned __int64 const otherBeforePrefix = g_uiSASDiagnosticOtherWrittenBytes;
	logSASDiagnosticLiteralLine("renamed_decisions.log", "renamed BBAI", SAS_DIAGNOSTIC_LOG_BBAI);
	logSASDiagnosticLiteralLine("BBAI_unrelated.log", "unrelated producer", SAS_DIAGNOSTIC_LOG_OTHER);
	require(g_uiSASDiagnosticBBAIWrittenBytes == bbaiBeforeRename + strlen("renamed BBAI\r\n"), "renamed BBAI retains its explicit family");
	require(g_uiSASDiagnosticOtherWrittenBytes == otherBeforePrefix + strlen("unrelated producer\r\n"), "BBAI prefix does not classify an unrelated producer");

	require(flushSASDiagnosticLogs(true), "session-boundary checkpoint succeeds");
	require(readFile(root + L"\\Logs\\BBAI.log") == "literal=75% %n %%\r\nnext\r\n", "later checkpoint appends");
	require(getSASDiagnosticLogBuffer("BBAI.log", SAS_DIAGNOSTIC_LOG_BBAI).szData.capacity() == std::string().capacity(), "drained peak capacity released");
	require(getSASDiagnosticLogBuffer("BBAI.log", SAS_DIAGNOSTIC_LOG_BBAI).hFile == INVALID_HANDLE_VALUE, "session release closes persistent handle");
	logSASDiagnosticLiteralLine("BBAI.log", "after rollover", SAS_DIAGNOSTIC_LOG_BBAI);
	require(readFile(root + L"\\Logs\\BBAI.log") == "literal=75% %n %%\r\nnext\r\nafter rollover\r\n", "reopening fixed name after release does not truncate prior rows");
	setSASDiagnosticLogRootDirectory(temporary);
	require(g_szSASDiagnosticLogRootDirectory == root, "successful output root remains stable");
	GC.overwrite = false;
	seedFile(root + L"\\Logs\\BBAI_append.log");
	logSASDiagnosticLiteralLine("BBAI_append.log", "append", SAS_DIAGNOSTIC_LOG_BBAI);
	require(flushSASDiagnosticLogs(false), "append-only checkpoint succeeds");
	require(readFile(root + L"\\Logs\\BBAI_append.log") == "old\r\nappend\r\n", "OverwriteLogs false preserves existing bytes");

	g_maxWrite = 5;
	g_failPartialWrite = true;
	logSASDiagnosticLiteralLine("SASGameRecord_partial.log", "partial row survives", SAS_DIAGNOSTIC_LOG_GAME_RECORD);
	require(getSASDiagnosticLogBuffer("SASGameRecord_partial.log", SAS_DIAGNOSTIC_LOG_GAME_RECORD).bAutoFlushBlocked, "failed immediate partial write blocks automatic retries");
	require(g_uiSASDiagnosticOutputFailures > 0 && g_iSASDiagnosticBlockedFiles > 0, "writer health counts failed output and blocked files");
	require(readFile(root + L"\\Logs\\SASGameRecord_partial.log") == "parti", "acknowledged prefix exists once");
	g_failPartialWrite = false;
	unsigned __int64 const retriesBeforePartial = g_uiSASDiagnosticRetryAttempts;
	require(flushSASDiagnosticLogs(false), "partial write resumes");
	require(g_uiSASDiagnosticRetryAttempts == retriesBeforePartial + 1 && g_uiSASDiagnosticRetrySuccesses > 0, "writer health counts retry attempt and recovery");
	require(g_iSASDiagnosticBlockedFiles == 0, "successful retry clears the live blocked-file count");
	require(readFile(root + L"\\Logs\\SASGameRecord_partial.log") == "partial row survives\r\n", "retry does not duplicate acknowledged bytes");
	g_maxWrite = 0xffffffff;
	g_zeroWrite = true;
	logSASDiagnosticLiteralLine("SASGameRecord_zero.log", "zero progress", SAS_DIAGNOSTIC_LOG_GAME_RECORD);
	int const beforeZero = g_writeCalls;
	require(getSASDiagnosticLogBuffer("SASGameRecord_zero.log", SAS_DIAGNOSTIC_LOG_GAME_RECORD).bAutoFlushBlocked, "zero-progress immediate write is blocked");
	require(!flushSASDiagnosticLogs(false) && g_writeCalls == beforeZero + 1, "zero progress stops without spinning");
	g_zeroWrite = false;
	require(flushSASDiagnosticLogs(true), "zero-progress retry recovers");

	// <!-- custom: A failure can leave no queued bytes; the checkpoint must still recover the blocked state rather than permanently reporting a dead file. (GPT-6.1-Sol) -->
	g_failPartialWrite = true;
	logSASDiagnosticLiteralLine("accepted_but_failed.log", "fully acknowledged", SAS_DIAGNOSTIC_LOG_OTHER);
	require(g_iSASDiagnosticBufferedBytes == 0 && g_iSASDiagnosticBlockedFiles == 1, "fully acknowledged failed write is blocked without pending text");
	g_failPartialWrite = false;
	require(flushSASDiagnosticLogs(true) && g_iSASDiagnosticBlockedFiles == 0, "empty failed write recovers at a checkpoint");
	logSASDiagnosticLiteralLine("close_failure.log", "completed row", SAS_DIAGNOSTIC_LOG_OTHER);
	g_failClose = true;
	require(!flushSASDiagnosticLogs(true) && g_iSASDiagnosticBlockedFiles == 1, "failed close is counted after all text drains");
	require(flushSASDiagnosticLogs(true) && g_iSASDiagnosticBlockedFiles == 0, "empty failed close recovers at the next checkpoint");


	g_failOpen = true;
	std::string const largeRow(1024 * 1024, 'x');
	for (int i = 0; i < 4; i++)
		logSASDiagnosticLiteralLine("BBAI_blocked.log", largeRow.c_str(), SAS_DIAGNOSTIC_LOG_BBAI);
	int const afterFailedOpen = g_openCalls;
	for (int i = 0; i < 80; i++)
		logSASDiagnosticLiteralLine("BBAI_blocked.log", largeRow.c_str(), SAS_DIAGNOSTIC_LOG_BBAI);
	require(g_openCalls == afterFailedOpen, "failed append does not retry on every row");
	for (int i = 0; i < 80; i++)
		logSASDiagnosticLiteralLine("SASGameRecord_blocked.log", largeRow.c_str(), SAS_DIAGNOSTIC_LOG_GAME_RECORD);
	require(g_iSASDiagnosticBufferedBytes <= SAS_DIAGNOSTIC_MAX_BUFFERED_BYTES, "payload cap is shared across files");
	require(getSASDiagnosticLogBuffer("BBAI_blocked.log", SAS_DIAGNOSTIC_LOG_BBAI).uiDroppedRows > 0, "overflow counts rejected rows");
	require(g_uiSASDiagnosticDroppedRows > 0 && g_uiSASDiagnosticDroppedBytes > 0, "writer health retains cumulative dropped output");
	require(g_iSASDiagnosticPeakBufferedBytes >= g_iSASDiagnosticBufferedBytes && g_iSASDiagnosticPeakBufferedBytes > 0, "writer health tracks peak pending text");
	require(!flushSASDiagnosticLogs(false), "checkpoint reports ongoing open failure");
	g_failOpen = false;
	require(flushSASDiagnosticLogs(true), "blocked output recovers");
	require(g_iSASDiagnosticBufferedBytes == 0, "shared accounting drains after recovery");
	require(readFile(root + L"\\Logs\\BBAI_blocked.log").find("SAS_DIAGNOSTIC_DATA_LOSS droppedRows=") != std::string::npos, "recovered log discloses losses");
	for (SASDiagnosticLogBuffers::const_iterator it = g_aSASDiagnosticLogBuffers.begin(); it != g_aSASDiagnosticLogBuffers.end(); ++it)
		require(it->second.szData.capacity() == std::string().capacity(), "session releases every drained file's capacity");

	// <!-- custom: ChatGPT's review identified a cross-file recovery gap: an earlier filename can have only loss counters while a later filename holds the entire shared budget.
	// Its checkpoint cannot queue the notice until the later file drains; the next accepted row must follow that notice, not precede it. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	g_failOpen = true;
	// <!-- custom: Leave enough room for a small later row but not its gap notice; accepting that row would conceal the loss position even before checkpoint recovery. (GPT-6.1-Sol) -->
	std::string const fullBudgetRow(SAS_DIAGNOSTIC_MAX_BUFFERED_BYTES - 34, 'b');
	logSASDiagnosticLiteralLine("Z_budget_holder.log", fullBudgetRow.c_str(), SAS_DIAGNOSTIC_LOG_OTHER);
	logSASDiagnosticLiteralLine("A_loss_order.log", "first dropped row exceeds the remaining thirty-two bytes of storage", SAS_DIAGNOSTIC_LOG_OTHER);
	logSASDiagnosticLiteralLine("A_loss_order.log", "dropped second", SAS_DIAGNOSTIC_LOG_OTHER);
	require(getSASDiagnosticLogBuffer("A_loss_order.log", SAS_DIAGNOSTIC_LOG_OTHER).szData.empty() && getSASDiagnosticLogBuffer("A_loss_order.log", SAS_DIAGNOSTIC_LOG_OTHER).uiDroppedRows == 2, "full shared budget leaves only loss counters in another file");
	g_failOpen = false;
	require(!flushSASDiagnosticLogs(false), "earlier file cannot queue its notice before the later budget holder drains");
	require(g_iSASDiagnosticBufferedBytes == 0 && getSASDiagnosticLogBuffer("A_loss_order.log", SAS_DIAGNOSTIC_LOG_OTHER).uiDroppedRows == 2, "later file frees the budget while the earlier loss notice remains outstanding");
	logSASDiagnosticLiteralLine("A_loss_order.log", "after recovery", SAS_DIAGNOSTIC_LOG_OTHER);
	std::string const orderedLoss = readFile(root + L"\\Logs\\A_loss_order.log");
	require(orderedLoss.find("SAS_DIAGNOSTIC_DATA_LOSS droppedRows=2") == 0, "cross-file recovery publishes the gap before the next accepted row");
	require(orderedLoss.find("\r\nafter recovery\r\n") != std::string::npos, "later row follows the complete loss notice");
	require(flushSASDiagnosticLogs(true), "cross-file recovery closes and releases drained buffers");

	std::string const oversized(SAS_DIAGNOSTIC_MAX_BUFFERED_BYTES + 1, 'z');
	logSASDiagnosticLiteralLine("SASGameRecord_oversized.log", oversized.c_str(), SAS_DIAGNOSTIC_LOG_GAME_RECORD);
	require(g_iSASDiagnosticBufferedBytes == 0, "oversized row is not copied into the pending buffer");
	g_maxWrite = 5;
	g_failPartialWrite = true;
	require(!flushSASDiagnosticLogs(false), "loss-notice partial failure is reported");
	require(getSASDiagnosticLogBuffer("SASGameRecord_oversized.log", SAS_DIAGNOSTIC_LOG_GAME_RECORD).uiDroppedRows == 0, "queued notice owns the loss counters");
	g_failPartialWrite = false;
	g_maxWrite = 0xffffffff;
	require(flushSASDiagnosticLogs(true), "oversized-row notice resumes after failure");
	std::string const loss = readFile(root + L"\\Logs\\SASGameRecord_oversized.log");
	require(loss.find("SAS_DIAGNOSTIC_DATA_LOSS droppedRows=1") == 0, "loss notice is complete without a duplicate prefix");
	puts("PASS C++ diagnostic writer: Unicode/sharing, literal text, overwrite/append, partial/zero writes, bounded failures, recovery notices and capacity release");
	return 0;
}
