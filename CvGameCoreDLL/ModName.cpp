#include "CvGameCoreDLL.h"
#include "ModName.h"
#include <fstream> // <!-- custom: Read the small exported source-metadata marker used by archive/runtime version resolution. (ChatGPT-5.6-Sol) -->
#include <string.h> // <!-- custom: Supply memcpy for the mutable CreateProcess command buffer used by the one-time Git resolver. (ChatGPT-5.6-Sol) -->

// advc.106i: New implementation file; see comment in header.

namespace
{
	// <!-- custom: Private source/version helpers stay local to ModName.
	// Dirty-file display limits protect log readability/buffers only; the uncapped total dirty-file count is still recorded, so these are implementation safety limits rather than player-tunable settings. (ChatGPT-5.6-Sol) -->
	char const* const SAS_VERSION_METADATA_RELATIVE_PATH = "Assets\\SASModVersion.txt";
	int const SAS_VERSION_DIRTY_FILE_DISPLAY_LIMIT = 32; // maximum changed paths written into dirtyFiles
	int const SAS_VERSION_DIRTY_FILES_DISPLAY_CHAR_LIMIT = 1200; // maximum aggregate dirtyFiles text; bounds diagnostic-row size/readability

	// <!-- custom: Small normalization/validation helpers below belong only to the source-version resolver; keeping them private avoids adding generic-looking utilities to the wider DLL API. (ChatGPT-5.6-Sol) -->
	CvString trimSourceValue(CvString value)
	{
		while (!value.empty() && (value[0] == ' ' || value[0] == '\t' || value[0] == '\r' || value[0] == '\n'))
			value.erase(0, 1);
		while (!value.empty())
		{
			char const c = value[value.length() - 1];
			if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
				break;
			value.erase(value.length() - 1, 1);
		}
		return value;
	}

	// <!-- custom: Git short-status uses a leading space as its unstaged-status column, so command output cleanup must preserve leading whitespace and remove only trailing CR/LF line endings. (ChatGPT-5.6-Sol) -->
	CvString trimSourceCommandOutput(CvString value)
	{
		while (!value.empty() && (value[value.length() - 1] == '\r' || value[value.length() - 1] == '\n'))
			value.erase(value.length() - 1, 1);
		return value;
	}

	bool isDecimalSourceValue(CvString const& value)
	{
		if (value.empty())
			return false;
		for (int i = 0; i < (int)value.length(); i++)
		{
			if (value[i] < '0' || value[i] > '9')
				return false;
		}
		return true;
	}

	bool isHexSourceValue(CvString const& value, size_t iExpectedLength)
	{
		if (value.length() != iExpectedLength)
			return false;
		for (int i = 0; i < (int)value.length(); i++)
		{
			char const c = value[i];
			if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
				return false;
		}
		return true;
	}

	CvString normalizedPathForCompare(CvString value)
	{
		value = trimSourceValue(value);
		for (size_t i = 0; i < value.length(); i++)
		{
			if (value[i] == '/')
				value[i] = '\\';
			else if (value[i] >= 'A' && value[i] <= 'Z')
				value[i] = (char)(value[i] - 'A' + 'a');
		}
		while (value.length() > 3 && value[value.length() - 1] == '\\')
			value.erase(value.length() - 1, 1);
		return value;
	}

	// <!-- custom: Run Git without a console-window flash, capture stdout through a pipe, and discard stderr so warnings cannot contaminate parsed metadata/status rows.
	// This is used only for a real development checkout; exported archives normally resolve the marker first and never spawn Git. (ChatGPT-5.6-Sol) -->
	bool runHiddenSourceCommand(CvString const& commandLine, CvString& output)
	{
		output.clear();
		SECURITY_ATTRIBUTES kSecurity;
		ZeroMemory(&kSecurity, sizeof(kSecurity));
		kSecurity.nLength = sizeof(kSecurity);
		kSecurity.bInheritHandle = TRUE;
		HANDLE hRead = NULL;
		HANDLE hWrite = NULL;
		if (!CreatePipe(&hRead, &hWrite, &kSecurity, 0))
			return false;
		if (!SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0))
		{
			CloseHandle(hRead);
			CloseHandle(hWrite);
			return false;
		}

		HANDLE hNull = CreateFileA("NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &kSecurity, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hNull == INVALID_HANDLE_VALUE)
		{
			CloseHandle(hRead);
			CloseHandle(hWrite);
			return false;
		}

		STARTUPINFOA kStartup;
		PROCESS_INFORMATION kProcess;
		ZeroMemory(&kStartup, sizeof(kStartup));
		ZeroMemory(&kProcess, sizeof(kProcess));
		kStartup.cb = sizeof(kStartup);
		kStartup.dwFlags = STARTF_USESTDHANDLES;
		kStartup.hStdInput = hNull;
		kStartup.hStdOutput = hWrite;
		kStartup.hStdError = hNull;

		std::vector<char> command(commandLine.length() + 1);
		memcpy(&command[0], commandLine.c_str(), commandLine.length() + 1);
		BOOL const bCreated = CreateProcessA(NULL, &command[0], NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &kStartup, &kProcess);
		CloseHandle(hNull);
		CloseHandle(hWrite);
		if (!bCreated)
		{
			CloseHandle(hRead);
			return false;
		}

		char buffer[2048];
		DWORD iRead = 0;
		while (ReadFile(hRead, buffer, sizeof(buffer), &iRead, NULL) && iRead > 0)
			output.append(buffer, iRead);
		CloseHandle(hRead);
		WaitForSingleObject(kProcess.hProcess, INFINITE);
		DWORD iExitCode = 1;
		GetExitCodeProcess(kProcess.hProcess, &iExitCode);
		CloseHandle(kProcess.hThread);
		CloseHandle(kProcess.hProcess);
		output = trimSourceCommandOutput(output);
		return (iExitCode == 0);
	}

	CvString sourceGitCommand(CvString const& modPath, char const* szArguments)
	{
		return CvString::format("git.exe -C \"%s\" %s", modPath.c_str(), szArguments);
	}

	void splitFirstSourceLine(CvString const& value, CvString& first, CvString& rest)
	{
		size_t const iBreak = value.find('\n');
		if (iBreak == CvString::npos)
		{
			first = trimSourceValue(value);
			rest.clear();
			return;
		}
		first = trimSourceValue(value.substr(0, iBreak));
		rest = trimSourceValue(value.substr(iBreak + 1));
	}

	CvString summarizeTrackedSourceStatus(CvString const& rawStatus, int& iFileCount)
	{
		iFileCount = 0;
		int iDisplayedFileCount = 0;
		CvString result;
		size_t iStart = 0;
		while (iStart < rawStatus.length())
		{
			size_t iEnd = rawStatus.find('\n', iStart);
			if (iEnd == CvString::npos)
				iEnd = rawStatus.length();
			CvString line = trimSourceValue(rawStatus.substr(iStart, iEnd - iStart));
			if (!line.empty())
			{
				iFileCount++;
				if (iDisplayedFileCount < SAS_VERSION_DIRTY_FILE_DISPLAY_LIMIT)
				{
					// <!-- custom: Preserve Git's two status columns, including staged-vs-unstaged distinction. (ChatGPT-5.6-Sol) -->
					CvString displayLine = rawStatus.substr(iStart, iEnd - iStart);
					if (!displayLine.empty() && displayLine[displayLine.length() - 1] == '\r')
						displayLine.erase(displayLine.length() - 1, 1);
					for (size_t i = 0; i < displayLine.length(); i++)
					{
						if (displayLine[i] == '"')
							displayLine[i] = '\'';
					}
					if (displayLine.length() >= 3)
						displayLine = "[" + displayLine.substr(0, 2) + "] " + displayLine.substr(3);
					size_t const iSeparatorLength = (result.empty() ? 0 : 2);
					if (result.length() + iSeparatorLength + displayLine.length() <= SAS_VERSION_DIRTY_FILES_DISPLAY_CHAR_LIMIT)
					{
						if (!result.empty())
							result += "; ";
						result += displayLine;
						iDisplayedFileCount++;
					}
				}
			}
			iStart = iEnd + 1;
		}
		if (iFileCount > iDisplayedFileCount)
		{
			if (!result.empty())
				result += "; ";
			result += CvString::format("... (+%d more)", iFileCount - iDisplayedFileCount);
		}
		return result;
	}


	// <!-- custom: Parse SAS_VERSION_ANCHOR_<base>[-<distance>-g<sha>] from Git describe/export-subst.
	// Git documents the distance as the number of commits in anchor..target, so base practical version + distance exactly reconstructs git rev-list --count for descendants of a correctly named immutable anchor, including merges. (ChatGPT-5.6-Sol) -->
	bool resolveVersionFromAnchorDescribe(CvString const& describeValue, CvString& version)
	{
		char const* const szPrefix = "SAS_VERSION_ANCHOR_";
		CvString const prefix(szPrefix);
		CvString const value = trimSourceValue(describeValue);
		if (value.find(prefix) != 0)
			return false;
		size_t const iVersionStart = prefix.length();
		size_t const iDash = value.find('-', iVersionStart);
		CvString const baseText = value.substr(iVersionStart, (iDash == CvString::npos ? value.length() : iDash) - iVersionStart);
		if (!isDecimalSourceValue(baseText))
			return false;

		int iBase = 0;
		for (int i = 0; i < (int)baseText.length(); i++)
			iBase = iBase * 10 + (baseText[i] - '0');
		int iDistance = 0;
		if (iDash != CvString::npos)
		{
			size_t const iGitHash = value.find("-g", iDash + 1);
			if (iGitHash == CvString::npos)
				return false;
			CvString const distanceText = value.substr(iDash + 1, iGitHash - iDash - 1);
			if (!isDecimalSourceValue(distanceText) || iGitHash + 2 >= value.length())
				return false;
			for (int i = 0; i < (int)distanceText.length(); i++)
				iDistance = iDistance * 10 + (distanceText[i] - '0');
		}
		version = CvString::format("%d", iBase + iDistance);
		return true;
	}

	// <!-- custom: Hash the tracked gameplay code/rules/script diff relative to HEAD.
	// This intentionally scopes out docs/LLM helpers and the compiled DLL (fingerprinted separately), while covering CvGameCoreDLL sources plus Config/Python/XML/PrivateMaps inputs. (ChatGPT-5.6-Sol) -->
	CvString fingerprintTrackedCodeRulesDiff(CvString const& diff)
	{
		unsigned __int64 uiHash = ((unsigned __int64)0xCBF29CE4 << 32) | 0x84222325;
		unsigned __int64 const uiPrime = ((unsigned __int64)0x00000100 << 32) | 0x000001B3;
		// <!-- custom: Visual C++ 2003 reported C2666 for CvString::operator[] with a size_t index because CvString exposes an int subscript plus a const char* conversion.
		// Use int to select the member overload unambiguously. (GPT-5.6-Sol) -->
		for (int i = 0; i < (int)diff.length(); i++)
		{
			uiHash ^= (unsigned char)diff[i];
			uiHash *= uiPrime;
		}
		CvString fingerprint;
		fingerprint.Format("FNV1A64:%016I64X", uiHash);
		return fingerprint;
	}

	// <!-- custom: Read the tiny export-subst marker used when no live Git worktree ships with the mod.
	// Exact SHA validates the marker; the optional anchor describe string reconstructs the practical numeric version. (ChatGPT-5.6-Sol) -->
	bool readExportedSourceMetadata(CvString const& modPath, CvString& version, CvString& commitHash, CvString& commitDate)
	{
		CvString path = modPath;
		if (!path.empty() && path[path.length() - 1] != '\\' && path[path.length() - 1] != '/')
			path += "\\";
		path += SAS_VERSION_METADATA_RELATIVE_PATH;
		std::ifstream input(path.c_str());
		if (!input.is_open())
			return false;
		std::string rawLine;
		while (std::getline(input, rawLine))
		{
			CvString line = trimSourceValue(rawLine.c_str());
			size_t const iEquals = line.find('=');
			if (iEquals == CvString::npos)
				continue;
			CvString const key = trimSourceValue(line.substr(0, iEquals));
			CvString const value = trimSourceValue(line.substr(iEquals + 1));
			if (key == "versionDescribe" && value.find("$Format:") == CvString::npos)
				resolveVersionFromAnchorDescribe(value, version);
			else if (key == "commit" && isHexSourceValue(value, 40))
				commitHash = value;
			else if (key == "commitDate" && value.find("$Format:") == CvString::npos)
				commitDate = value;
		}
		// <!-- custom: Exact SHA is the trust anchor for exported metadata; an unsubstituted/hand-edited marker must not be treated as valid source provenance. (ChatGPT-5.6-Sol) -->
		if (commitHash.empty())
		{
			version.clear();
			commitDate.clear();
			return false;
		}
		return true;
	}
}

// <!-- custom: Initialize the lazy source/version cache explicitly; ModName exists before any source-details getter is necessarily called. (ChatGPT-5.6-Sol) -->
ModName::ModName()
{
	resetSourceDetails();
}

// <!-- custom: Invalidate every cached source field together.
// update() calls this when BtS refreshes the loaded mod path, so no provenance from a previous path can survive. (ChatGPT-5.6-Sol) -->
void ModName::resetSourceDetails()
{
	m_bSourceDetailsResolved = false;
	m_iSourceDirtyState = -1;
	m_iSourceDirtyFileCount = -1;
	m_iSourceCodeRulesDiffBytes = -1;
	m_sVersion.clear();
	m_sCommitHash.clear();
	m_sBranch.clear();
	m_sCommitDate.clear();
	m_sSourceMetadataType = "unknown";
	m_sSourceDirtyFiles.clear();
	m_sSourceCodeRulesDiffFingerprint = "-";
}

void ModName::update(char const* szFullPath, char const* szPathInRoot)
{
	m_sFullPath = szFullPath;
	m_sPathInRoot = szPathInRoot;
	m_sName = m_sPathInRoot;
	size_t posMods = m_sName.find("Mods");
	if (posMods != CvString::npos)
	{
		/*	Skip over "Mods" plus the path separator.
			And chop off the separator at the end. */
		m_sName = m_sName.substr(posMods + 5, m_sName.length() - posMods - 6);
	}
	// <!-- custom: A refreshed loaded-mod path invalidates any source metadata cached for the previous path. (ChatGPT-5.6-Sol) -->
	resetSourceDetails();
}

// <!-- custom: Central branded/project identity.
// The actual folder/path remains the EXE-derived data cached by update(). (ChatGPT-5.6-Sol) -->
char const* ModName::getDisplayName() const
{
	char const* szDisplayName = GC.getDefineSTRING("SAS_MOD_DISPLAY_NAME");
	if (szDisplayName != NULL && szDisplayName[0] != '\0')
		return szDisplayName;
	return getName();
}

// <!-- custom: Convert AdvCiv/BtS's cached mod path into an absolute filesystem path for the metadata-file/.git checks without changing the public loaded-folder identity. (ChatGPT-5.6-Sol) -->
CvString ModName::getAbsoluteModPath() const
{
	CvString path = m_sFullPath;
	bool const bAbsoluteDrivePath = (path.length() >= 3 && path[1] == ':' && (path[2] == '\\' || path[2] == '/'));
	bool const bAbsoluteUNCPath = (path.length() >= 2 && ((path[0] == '\\' && path[1] == '\\') || (path[0] == '/' && path[1] == '/')));
	if (!bAbsoluteDrivePath && !bAbsoluteUNCPath)
	{
		char szExePath[MAX_PATH + 1];
		DWORD const iLength = GetModuleFileNameA(NULL, szExePath, MAX_PATH);
		if (iLength == 0 || iLength >= MAX_PATH)
			return "";
		szExePath[iLength] = '\0';
		CvString exePath(szExePath);
		size_t const iSlash = exePath.find_last_of("\\/");
		if (iSlash == CvString::npos)
			return "";
		path = exePath.substr(0, iSlash + 1) + m_sPathInRoot;
	}
	while (path.length() > 3 && (path[path.length() - 1] == '\\' || path[path.length() - 1] == '/'))
		path.erase(path.length() - 1, 1);
	return path;
}

// <!-- custom: Resolve source provenance lazily once per loaded mod path, then cache it for the rest of the process.
// Archives normally cost one tiny metadata-file read; real Git checkouts run a small set of hidden Git commands once, after which every public getter is an in-memory lookup. (ChatGPT-5.6-Sol) -->
void ModName::resolveSourceDetails() const
{
	if (m_bSourceDetailsResolved)
		return;
	m_bSourceDetailsResolved = true;
	m_iSourceDirtyState = -1;
	m_iSourceDirtyFileCount = -1;
	m_iSourceCodeRulesDiffBytes = -1;
	m_sVersion.clear();
	m_sCommitHash.clear();
	m_sBranch.clear();
	m_sCommitDate.clear();
	m_sSourceMetadataType = "unknown";
	m_sSourceDirtyFiles.clear();
	m_sSourceCodeRulesDiffFingerprint = "-";

	CvString const modPath = getAbsoluteModPath();
	if (modPath.empty())
		return;

	// <!-- custom: GitHub/git-archive downloads can reconstruct the exact practical version from one immutable SAS_VERSION_ANCHOR_<exact-count> tag plus Git describe distance, while export-subst supplies exact commit/date.
	// No per-commit VERSION update is needed.
	// Archives still cannot inspect post-extraction local edits, so dirty remains -1. (ChatGPT-5.6-Sol) -->
	if (readExportedSourceMetadata(modPath, m_sVersion, m_sCommitHash, m_sCommitDate))
	{
		m_sSourceMetadataType = (m_sVersion.empty() ? "gitArchive" : "gitArchiveVersioned");
		return;
	}

	// <!-- custom: If this is a plain copied/exported folder whose metadata marker was not substituted, avoid spawning Git merely to discover an unrelated parent repository.
	// Git worktrees may use either a .git directory or file, so existence is enough here; the exact top-level path is still verified below. (ChatGPT-5.6-Sol) -->
	CvString const gitMarker = modPath + "\\.git";
	if (GetFileAttributesA(gitMarker.c_str()) == INVALID_FILE_ATTRIBUTES)
		return;

	CvString output;
	if (!runHiddenSourceCommand(sourceGitCommand(modPath, "rev-parse --show-toplevel"), output))
		return;
	if (normalizedPathForCompare(output) != normalizedPathForCompare(modPath))
		return;

	// <!-- custom: %cI is Git's strict ISO-8601 committer date with the timezone offset stored in the commit.
	// Keep that canonical commit metadata rather than rewriting it to the runtime log's UTC event-time convention. (ChatGPT-5.6-Sol) -->
	CvString commitAndDate;
	if (!runHiddenSourceCommand(sourceGitCommand(modPath, "show -s --format=%H%n%cI HEAD"), commitAndDate))
		return;
	splitFirstSourceLine(commitAndDate, m_sCommitHash, m_sCommitDate);
	if (!isHexSourceValue(m_sCommitHash, 40))
	{
		m_sCommitHash.clear();
		m_sCommitDate.clear();
		return;
	}

	runHiddenSourceCommand(sourceGitCommand(modPath, "rev-parse --abbrev-ref HEAD"), m_sBranch);
	CvString shallow;
	bool const bShallowKnown = runHiddenSourceCommand(sourceGitCommand(modPath, "rev-parse --is-shallow-repository"), shallow);
	bool const bShallow = (bShallowKnown && trimSourceValue(shallow) == "true");
	bool const bFullHistory = (bShallowKnown && trimSourceValue(shallow) == "false");
	if (bFullHistory)
	{
		CvString version;
		if (runHiddenSourceCommand(sourceGitCommand(modPath, "rev-list --count HEAD"), version) && isDecimalSourceValue(version))
			m_sVersion = version;
	}

	CvString status;
	if (runHiddenSourceCommand(sourceGitCommand(modPath, "status --short --untracked-files=no"), status))
	{
		m_sSourceDirtyFiles = summarizeTrackedSourceStatus(status, m_iSourceDirtyFileCount);
		m_iSourceDirtyState = (m_iSourceDirtyFileCount > 0 ? 1 : 0);
		// <!-- custom: Fingerprint only tracked code/rules/scripts that can affect the mod/runtime: C++ build inputs plus Config/Python/XML/PrivateMaps.
		// git diff HEAD includes staged and unstaged working-tree content; untracked files are deliberately not claimed here, and the loaded DLL has its own exact-byte fingerprint. (ChatGPT-5.6-Sol) -->
		CvString diff;
		if (runHiddenSourceCommand(sourceGitCommand(modPath, "diff --binary --full-index --no-ext-diff --no-textconv --no-renames --no-color HEAD -- CvGameCoreDLL Assets/Config Assets/Python Assets/XML PrivateMaps"), diff))
		{
			m_iSourceCodeRulesDiffBytes = (__int64)diff.length();
			m_sSourceCodeRulesDiffFingerprint = fingerprintTrackedCodeRulesDiff(diff);
		}
	}
	m_sSourceMetadataType = (bShallow ? "gitShallow" : (bFullHistory ? "git" : "gitHistoryUnknown"));
}

// <!-- custom: Thin public source-detail accessors all trigger the same lazy resolver.
// Only the first accessor after construction/update performs filesystem/Git work; subsequent calls return cached values. (ChatGPT-5.6-Sol) -->
char const* ModName::getVersion() const
{
	resolveSourceDetails();
	return m_sVersion.c_str();
}

char const* ModName::getCommitHash() const
{
	resolveSourceDetails();
	return m_sCommitHash.c_str();
}

char const* ModName::getBranch() const
{
	resolveSourceDetails();
	return m_sBranch.c_str();
}

char const* ModName::getCommitDate() const
{
	resolveSourceDetails();
	return m_sCommitDate.c_str();
}

char const* ModName::getSourceMetadataType() const
{
	resolveSourceDetails();
	return m_sSourceMetadataType.c_str();
}

int ModName::getSourceDirtyState() const
{
	resolveSourceDetails();
	return m_iSourceDirtyState;
}

int ModName::getSourceDirtyFileCount() const
{
	resolveSourceDetails();
	return m_iSourceDirtyFileCount;
}

char const* ModName::getSourceDirtyFiles() const
{
	resolveSourceDetails();
	return m_sSourceDirtyFiles.c_str();
}

// <!-- custom: Fingerprint tracked gameplay code/rules/scripts relative to HEAD so logs can distinguish different runtime-relevant edits without hashing docs, LLM helpers, or the separately fingerprinted loaded DLL. (ChatGPT-5.6-Sol) -->
__int64 ModName::getSourceCodeRulesDiffBytes() const
{
	resolveSourceDetails();
	return m_iSourceCodeRulesDiffBytes;
}

char const* ModName::getSourceCodeRulesDiffFingerprint() const
{
	resolveSourceDetails();
	return m_sSourceCodeRulesDiffFingerprint.c_str();
}
