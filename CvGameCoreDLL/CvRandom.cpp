#include "CvGameCoreDLL.h"
#include "CvRandom.h"
#include "CvGame.h"
#include "SASGameRecordLog.h" // <!-- custom: Level-3 SASGameRecord counts/fingerprints authoritative map/sync RNG advances without emitting one row per roll. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->


#define RANDOM_A      (1103515245)
#define RANDOM_C      (12345)
#define RANDOM_SHIFT  (16)

unsigned short CvRandom::getInt(unsigned short usNum, TCHAR const* szLog, int iData1, int iData2) // advc.001n
{	// <advc.003t>
	if (GC.getLogger().isEnabledRand() && szLog != NULL) printToLog(szLog, usNum, iData1, iData2); // </advc.003t>
	// <!-- custom: Count/fingerprint the real RNG consumption before advancing the seed.
	// Unlike RandLog, this deliberately includes NULL-message calls such as shuffles.
	// The caller-side boolean keeps levels 0-2 and pre-session/main-menu randomness at a single cheap branch; the helper itself ignores async and local CvRandom objects by identity. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (g_bSASGameRecordRngTrackingActive) noteSASGameRecordRandomCall(this, usNum, szLog, iData1, iData2);
	m_uiRandomSeed = (RANDOM_A * m_uiRandomSeed) + RANDOM_C;
	unsigned short r = (unsigned short)
			((((m_uiRandomSeed >> RANDOM_SHIFT) & MAX_UNSIGNED_SHORT) *
			((unsigned int)usNum)) / (MAX_UNSIGNED_SHORT + 1));
	return r;
}


// <!-- custom: Initialize before reset() because level-3 diagnostics can observe reset calls while unrelated/local CvRandom objects are constructed during an active game; their pointer is ignored, but the old-state argument must still be defined before helper dispatch. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
CvRandom::CvRandom() :
	m_uiRandomSeed(0)
{
	reset();
}


CvRandom::~CvRandom() {}


void CvRandom::init(unsigned long ulSeed)
{
	reset(ulSeed);
}

// Initializes data members that are serialized
void CvRandom::reset(unsigned int uiSeed)
{
	// <!-- custom: Level-3 RNG reproducibility must observe explicit mid-session seed replacement too (e.g. benchmark Python calls CyRandom.init).
	// Constructor/new-game/load resets occur while tracking is inactive or before authoritative identities are registered.
	// Assign gameplay state first, then report the already-known old/new values: diagnostics should never run while the RNG object is half-mutated, and any future logging-side RNG would then correctly occur after the SEED_SET operation. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	unsigned int const uiOldSeed = m_uiRandomSeed;
	m_uiRandomSeed = uiSeed;
	if (g_bSASGameRecordRngTrackingActive) noteSASGameRecordRandomSeedSet(this, uiOldSeed, uiSeed, false);
}

/*	advc.190c: Separate function for calls from the EXE. So that the DLL can figure out
	which options were set to "Random" during game setup. */
unsigned short CvRandom::getExternal(unsigned short usNum, TCHAR const* szLog)
{
	GC.getInitCore().externalRNGCall(usNum, this);
	// <!-- custom: Preserve how many authoritative RNG advances originated through the EXE-facing wrapper.
	// The following get() performs/counts the actual advance; this marker only classifies that next call.
	// Range 0 is rejected by the public int wrapper without advancing the RNG, so do not leave a stale EXE-origin marker for a later real call. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (g_bSASGameRecordRngTrackingActive && usNum > 0) noteSASGameRecordExternalRandomCall(this);
	return get(usNum, szLog);
}


void CvRandom::reseed(unsigned int uiNewValue)
{
	// <!-- custom: Keep explicit reseeds in the same level-3 authoritative stream provenance as ordinary reset/init seed assignments.
	// Assign first for the same observer-safety/order reason as reset(). (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	unsigned int const uiOldSeed = m_uiRandomSeed;
	m_uiRandomSeed = uiNewValue;
	if (g_bSASGameRecordRngTrackingActive) noteSASGameRecordRandomSeedSet(this, uiOldSeed, uiNewValue, true);
}


unsigned int CvRandom::getSeed()
{
	return m_uiRandomSeed;
}


void CvRandom::read(FDataStreamBase* pStream)
{
	reset();
	pStream->Read(&m_uiRandomSeed);
}


void CvRandom::write(FDataStreamBase* pStream)
{
	pStream->Write(m_uiRandomSeed);
}

// advc: Moved from CvGameCoreUtils
void CvRandom::shuffle(int* piShuffle, int iNum)
{
	for (int i = 0; i < iNum; i++)
		piShuffle[i] = i;
	for (int i = 0; i < iNum; i++)
	{
		int j = (get(iNum - i, NULL) + i);
		if (i != j)
		{
			int iTemp = piShuffle[i];
			piShuffle[i] = piShuffle[j];
			piShuffle[j] = iTemp;
		}
	}
} // </advc>

// advc.enum:
void CvRandom::shuffle(std::vector<int>& aiIndices)
{
	sequtil::iota(aiIndices.begin(), aiIndices.end(), 0);
	int const iSize = (int)aiIndices.size();
	FAssertMsg(iSize > 0, "Shuffling empty vector; intended?");
	for (int i = 0; i < iSize; i++)
		std::swap(aiIndices[i], aiIndices[get(iSize - i, NULL) + i]);
}

// <advc.007c>
/*	Two function calls that won't get inlined, but it doesn't really matter -
	only gets called if the log is enabled. */
void CvRandom::printToLog(TCHAR const* szMsg, unsigned short usNum, int iData1, int iData2) // advc.001n
{	// advc.003t:
	GC.getLogger().logRandomNumber(szMsg, usNum, m_uiRandomSeed, iData1, iData2);
}


void CvRandomExtended::printToLog(TCHAR const* szMsg, unsigned short usNum, int iData1, int iData2) // advc.001n
{
	GC.getLogger().logRandomNumber(szMsg, usNum, m_uiRandomSeed, iData1, iData2,
			&m_szFileName);
}


void CvRandomExtended::setLogFileName(CvString szName)
{
	m_szFileName = szName;
}


void CvRandomExtended::read(FDataStreamBase* pStream)
{
	CvRandom::read(pStream);
	m_szFileName.clear();
	pStream->ReadString(m_szFileName);
}


void CvRandomExtended::write(FDataStreamBase* pStream)
{
	CvRandom::write(pStream);
	pStream->WriteString(m_szFileName);
}
// </advc.007c>
