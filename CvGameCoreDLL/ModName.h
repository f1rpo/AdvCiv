#pragma once
#ifndef MOD_NAME_H
#define MOD_NAME_H

/*	advc.106i: Small class for caching the mod's name and install location.
	(Could be expanded to also modify the name stored by the EXE; see the
	ModName class in the Taurus mod.) */
// <!-- custom: AdvCiv-SAS also uses this existing central object for branded display identity and lazily resolved source/version provenance, keeping UI and logs on one resolver. (ChatGPT-5.6-Sol) -->

class ModName
{
public:
	// <!-- custom: Initialize the AdvCiv-SAS source/version cache alongside the legacy ModName path cache. (ChatGPT-5.6-Sol) -->
	ModName();
	/*	The first param (and our m_sFullPath) is for the result of
		CvDLLUtilityIFaceBase::getExternalModName with bFullPath=true,
		the second (our m_sPathInRoot) is the result for bFullPath=false.
		I'm always seeing the same result, namely a path relative to the
		folder containing the Mods folder (I'll refer to that folder as "root"):
		"Mods\AdvCiv\"
		I don't know that the bFullPath parameter will never matter, so
		it seems safer to store both strings. Maybe the full path could
		be longer than what I've seen, maybe the non-full path could be
		just the actual mod name. We will, in any case, extract the actual
		mod name into m_sName. */
	void update(char const* szFullPath, char const* szPathInRoot);
	char const* getFullPath() const { return m_sFullPath.c_str(); }
	char const* getPathInRoot() const { return m_sPathInRoot.c_str(); }
	// <!-- custom: Keep getName() as the actual loaded mod-folder name detected by AdvCiv/BtS; it may differ from the branded display name after a user renames the installed folder. (ChatGPT-5.6-Sol) -->
	char const* getName() const { return m_sName.c_str(); }
	// <!-- custom: Return the branded/project name configured by SAS_MOD_DISPLAY_NAME, falling back to the detected folder name.
	// Call only after GlobalDefines have been loaded. (ChatGPT-5.6-Sol) -->
	char const* getDisplayName() const;

	// <!-- custom: Resolve source/version metadata lazily from either an exported archive marker or the exact Git checkout rooted at this loaded mod folder.
	// A shallow Git checkout keeps the exact SHA but intentionally omits the misleading incomplete commit-count version. (ChatGPT-5.6-Sol) -->
	char const* getVersion() const;
	char const* getCommitHash() const;
	char const* getBranch() const;
	char const* getCommitDate() const;
	char const* getSourceMetadataType() const;
	// <!-- custom: Tri-state dirty status: -1 = unavailable/no inspectable Git worktree, 0 = verified clean tracked tree, 1 = verified tracked changes. (ChatGPT-5.6-Sol) -->
	int getSourceDirtyState() const;
	int getSourceDirtyFileCount() const;
	char const* getSourceDirtyFiles() const;
	// <!-- custom: Fingerprint tracked gameplay code/rules/scripts relative to HEAD so logs can distinguish different runtime-relevant edits without hashing docs, LLM helpers, or the separately fingerprinted loaded DLL. (ChatGPT-5.6-Sol) -->
	__int64 getSourceCodeRulesDiffBytes() const;
	char const* getSourceCodeRulesDiffFingerprint() const;

private:
	// <!-- custom: Internal source/version resolver helpers; resolution is lazy and reset only if update() changes the loaded mod path. (ChatGPT-5.6-Sol) -->
	void resetSourceDetails();
	void resolveSourceDetails() const;
	CvString getAbsoluteModPath() const;

	CvString m_sFullPath;
	CvString m_sPathInRoot;
	CvString m_sName;
	// <!-- custom: Cache only authoritative source/version facts.
	// Derived presentation values such as a short SHA are intentionally computed by callers from these primary fields.
	// mutable allows const UI/log getters to populate the cache once without making source resolution part of ModName's logical identity mutation. (ChatGPT-5.6-Sol) -->
	mutable bool m_bSourceDetailsResolved;
	mutable int m_iSourceDirtyState;
	mutable int m_iSourceDirtyFileCount;
	mutable __int64 m_iSourceCodeRulesDiffBytes;
	mutable CvString m_sVersion;
	mutable CvString m_sCommitHash;
	mutable CvString m_sBranch;
	mutable CvString m_sCommitDate;
	mutable CvString m_sSourceMetadataType;
	mutable CvString m_sSourceDirtyFiles;
	mutable CvString m_sSourceCodeRulesDiffFingerprint;
};

#endif
