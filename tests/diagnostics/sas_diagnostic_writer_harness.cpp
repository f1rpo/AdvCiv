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
typedef wchar_t wchar;
#include "CvString.h"

// <!-- custom: Only the writer's loss-notice formatter needs this game-string method; all buffering, path and I/O logic below comes from the current source. (GPT-6.1-Sol) -->
void CvString::Format(LPCSTR format, ...)
{
	char buffer[2048];
	va_list args;
	va_start(args, format);
	int const length = _vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	assert(length >= 0 && length < sizeof(buffer));
	assign(buffer, length);
}

struct TestGlobals
{
	TestGlobals() : overwrite(true) {}
	bool overwrite;
	bool isOverwriteLogs() const { return overwrite; }
} GC;

static bool g_shellFolderUnavailable = false;
static bool g_failOpen = false;
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
#include "writer_under_test.inc"
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

int main(int argc, char** argv)
{
	wchar_t temporary[MAX_PATH];
	require(GetEnvironmentVariableW(L"SAS_DIAGNOSTIC_TEST_ROOT", temporary, MAX_PATH) > 0, "test root is provided");
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
		logSASDiagnosticLiteralLine("SASGameRecord_early_abort.log", "initialization ended before BUG");
		require(!flushSASDiagnosticLogs(true) && g_openCalls == 0, "rollover still preserves unresolved output");
		require(flushSASDiagnosticLogs(true, true), "terminal flush resolves fallback without a BUG handoff");
		require(getSASDiagnosticLogBuffer("SASGameRecord_early_abort.log").hFile == INVALID_HANDLE_VALUE, "terminal flush closes its handle");
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
		logSASDiagnosticLiteralLine("BBAI_crash.log", "BBAI last evidence 75%");
		logSASDiagnosticLiteralLine("SASGameRecord_crash.log", "SGR last evidence 75%");
		logSASDiagnosticLiteralLine("BBAI_crash.log", "final BBAI tail");
		logSASDiagnosticLiteralLine("SASGameRecord_crash.log", "final SGR tail");
		require(g_iSASDiagnosticBufferedBytes == 0, "successful rows do not wait for a turn checkpoint");
		TerminateProcess(GetCurrentProcess(), 73);
		require(false, "abrupt termination must not return");
	}
	std::wstring const root = std::wstring(temporary) + L"\\Unicode_\x0416\x6f22";
	require(CreateDirectoryW(root.c_str(), NULL) != FALSE, "create Unicode root outside the ANSI alphabet");
	require(CreateDirectoryW((root + L"\\Logs").c_str(), NULL) != FALSE, "create log directory");
	int const beforeRoot = g_openCalls;
	require(flushSASDiagnosticLogs(false), "empty writer does not resolve a fallback root");
	logSASDiagnosticLiteralLine("SASGameRecord_pre_root.log", "before BUG initializes");
	require(!flushSASDiagnosticLogs(true) && g_openCalls == beforeRoot, "early native rollover waits for authoritative root");
	std::string const startupRow(1024 * 1024, 's');
	for (int startup = 0; startup < 18; startup++)
		logSASDiagnosticLiteralLine("SASGameRecord_startup_budget.log", startupRow.c_str());
	require(g_openCalls == beforeRoot && !g_bSASDiagnosticLogRootResolved, "pre-BUG overflow never auto-binds a fallback root");
	require(g_iSASDiagnosticBufferedBytes <= SAS_DIAGNOSTIC_MAX_BUFFERED_BYTES && getSASDiagnosticLogBuffer("SASGameRecord_startup_budget.log").uiDroppedRows > 0, "pre-BUG overflow is bounded and accounted");
	setSASDiagnosticLogRootDirectory((root + L"\\").c_str());
	require(g_szSASDiagnosticLogRootDirectory == root, "root is Unicode and trailing separator is trimmed");
	require(flushSASDiagnosticLogs(false), "pre-root batch flushes after BUG supplies root");
	require(readFile(root + L"\\Logs\\SASGameRecord_pre_root.log") == "before BUG initializes\r\n", "early batch uses authoritative root");

	int const initialOpens = g_openCalls;
	require(flushSASDiagnosticLogs(false) && g_openCalls == initialOpens, "empty/disabled writer does no I/O");
	seedFile(root + L"\\Logs\\BBAI.log");
	g_inspectWhileWriting = true;
	logSASDiagnosticLiteralLine("BBAI.log", "literal=75% %n %%");
	require(readFile(root + L"\\Logs\\BBAI.log") == "literal=75% %n %%\r\n", "ordinary row is visible before any checkpoint");
	require(CopyFileW((root + L"\\Logs\\BBAI.log").c_str(), (root + L"\\BBAI_live_copy.log").c_str(), FALSE) != FALSE, "Windows copy succeeds while the writer handle remains open");
	require(readFile(root + L"\\BBAI_live_copy.log") == "literal=75% %n %%\r\n", "live copy includes the completed row");
	int const afterFirstRow = g_openCalls;
	require(getSASDiagnosticLogBuffer("BBAI.log").hFile != INVALID_HANDLE_VALUE, "successful row retains shared handle");
	require(flushSASDiagnosticLogs(false), "first checkpoint succeeds");
	require(readFile(root + L"\\Logs\\BBAI.log") == "literal=75% %n %%\r\n", "first overwrite and literal percent preservation");
	logSASDiagnosticLiteralLine("BBAI.log", "next");
	require(g_openCalls == afterFirstRow, "later rows reuse the open handle");
	require(flushSASDiagnosticLogs(true), "session-boundary checkpoint succeeds");
	require(readFile(root + L"\\Logs\\BBAI.log") == "literal=75% %n %%\r\nnext\r\n", "later checkpoint appends");
	require(getSASDiagnosticLogBuffer("BBAI.log").szData.capacity() == std::string().capacity(), "drained peak capacity released");
	require(getSASDiagnosticLogBuffer("BBAI.log").hFile == INVALID_HANDLE_VALUE, "session release closes persistent handle");
	logSASDiagnosticLiteralLine("BBAI.log", "after rollover");
	require(readFile(root + L"\\Logs\\BBAI.log") == "literal=75% %n %%\r\nnext\r\nafter rollover\r\n", "reopening fixed name after release does not truncate prior rows");
	setSASDiagnosticLogRootDirectory(temporary);
	require(g_szSASDiagnosticLogRootDirectory == root, "successful output root remains stable");
	GC.overwrite = false;
	seedFile(root + L"\\Logs\\BBAI_append.log");
	logSASDiagnosticLiteralLine("BBAI_append.log", "append");
	require(flushSASDiagnosticLogs(false), "append-only checkpoint succeeds");
	require(readFile(root + L"\\Logs\\BBAI_append.log") == "old\r\nappend\r\n", "OverwriteLogs false preserves existing bytes");

	g_maxWrite = 5;
	g_failPartialWrite = true;
	logSASDiagnosticLiteralLine("SASGameRecord_partial.log", "partial row survives");
	require(getSASDiagnosticLogBuffer("SASGameRecord_partial.log").bAutoFlushBlocked, "failed immediate partial write blocks automatic retries");
	require(readFile(root + L"\\Logs\\SASGameRecord_partial.log") == "parti", "acknowledged prefix exists once");
	g_failPartialWrite = false;
	require(flushSASDiagnosticLogs(false), "partial write resumes");
	require(readFile(root + L"\\Logs\\SASGameRecord_partial.log") == "partial row survives\r\n", "retry does not duplicate acknowledged bytes");
	g_maxWrite = 0xffffffff;
	g_zeroWrite = true;
	logSASDiagnosticLiteralLine("SASGameRecord_zero.log", "zero progress");
	int const beforeZero = g_writeCalls;
	require(getSASDiagnosticLogBuffer("SASGameRecord_zero.log").bAutoFlushBlocked, "zero-progress immediate write is blocked");
	require(!flushSASDiagnosticLogs(false) && g_writeCalls == beforeZero + 1, "zero progress stops without spinning");
	g_zeroWrite = false;
	require(flushSASDiagnosticLogs(true), "zero-progress retry recovers");

	g_failOpen = true;
	std::string const largeRow(1024 * 1024, 'x');
	for (int i = 0; i < 4; i++)
		logSASDiagnosticLiteralLine("BBAI_blocked.log", largeRow.c_str());
	int const afterFailedOpen = g_openCalls;
	for (int i = 0; i < 80; i++)
		logSASDiagnosticLiteralLine("BBAI_blocked.log", largeRow.c_str());
	require(g_openCalls == afterFailedOpen, "failed append does not retry on every row");
	for (int i = 0; i < 80; i++)
		logSASDiagnosticLiteralLine("SASGameRecord_blocked.log", largeRow.c_str());
	require(g_iSASDiagnosticBufferedBytes <= SAS_DIAGNOSTIC_MAX_BUFFERED_BYTES, "payload cap is shared across files");
	require(getSASDiagnosticLogBuffer("BBAI_blocked.log").uiDroppedRows > 0, "overflow counts rejected rows");
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
	logSASDiagnosticLiteralLine("Z_budget_holder.log", fullBudgetRow.c_str());
	logSASDiagnosticLiteralLine("A_loss_order.log", "first dropped row exceeds the remaining thirty-two bytes of storage");
	logSASDiagnosticLiteralLine("A_loss_order.log", "dropped second");
	require(getSASDiagnosticLogBuffer("A_loss_order.log").szData.empty() && getSASDiagnosticLogBuffer("A_loss_order.log").uiDroppedRows == 2, "full shared budget leaves only loss counters in another file");
	g_failOpen = false;
	require(!flushSASDiagnosticLogs(false), "earlier file cannot queue its notice before the later budget holder drains");
	require(g_iSASDiagnosticBufferedBytes == 0 && getSASDiagnosticLogBuffer("A_loss_order.log").uiDroppedRows == 2, "later file frees the budget while the earlier loss notice remains outstanding");
	logSASDiagnosticLiteralLine("A_loss_order.log", "after recovery");
	std::string const orderedLoss = readFile(root + L"\\Logs\\A_loss_order.log");
	require(orderedLoss.find("SAS_DIAGNOSTIC_DATA_LOSS droppedRows=2") == 0, "cross-file recovery publishes the gap before the next accepted row");
	require(orderedLoss.find("\r\nafter recovery\r\n") != std::string::npos, "later row follows the complete loss notice");
	require(flushSASDiagnosticLogs(true), "cross-file recovery closes and releases drained buffers");

	std::string const oversized(SAS_DIAGNOSTIC_MAX_BUFFERED_BYTES + 1, 'z');
	logSASDiagnosticLiteralLine("SASGameRecord_oversized.log", oversized.c_str());
	require(g_iSASDiagnosticBufferedBytes == 0, "oversized row is not copied into the pending buffer");
	g_maxWrite = 5;
	g_failPartialWrite = true;
	require(!flushSASDiagnosticLogs(false), "loss-notice partial failure is reported");
	require(getSASDiagnosticLogBuffer("SASGameRecord_oversized.log").uiDroppedRows == 0, "queued notice owns the loss counters");
	g_failPartialWrite = false;
	g_maxWrite = 0xffffffff;
	require(flushSASDiagnosticLogs(true), "oversized-row notice resumes after failure");
	std::string const loss = readFile(root + L"\\Logs\\SASGameRecord_oversized.log");
	require(loss.find("SAS_DIAGNOSTIC_DATA_LOSS droppedRows=1") == 0, "loss notice is complete without a duplicate prefix");
	puts("PASS C++ diagnostic writer: Unicode/sharing, literal text, overwrite/append, partial/zero writes, bounded failures, recovery notices and capacity release");
	return 0;
}
