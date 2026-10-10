#pragma once

#ifndef CIV4_RANDOM_H
#define CIV4_RANDOM_H

class CvRandom
{
public:
	DllExport CvRandom();
	DllExport virtual ~CvRandom(); // advc (comment): Can't make this non-virtual b/c of DllExport

	DllExport void init(unsigned long ulSeed);
	void reset(unsigned int uiSeed = 0);
	// for serialization
	virtual void read(FDataStreamBase* pStream); // advc.007c: virtual
	virtual void write(FDataStreamBase* pStream); // advc.007c: virtual
	/*	Returns a value from the half-open interval [0, iNum).
		advc.006: iNum taken as an int but needs to be in [0, 65535].
		Will return 0 for iNum=0 (and also for iNum=1). */
	// <advc.001n>
	unsigned short get(int iNum, TCHAR const* szMsg = NULL, int iData1 = MIN_INT, int iData2 = MIN_INT) { return getInt(iNum, szMsg, iData1, iData2); } // advc.128 // New name to avoid issues in CyRandomPythonInterface
	unsigned short getInt(int iNum, TCHAR const* szMsg, int iData1, int iData2 = MIN_INT)
	{	
		// <!-- custom: this assert fails sometimes, so fixing the issue as recommended by chatgpt 5, minimally fixed here so that we don't have to tediously check every place where it may be called or not and if it is done correctly (patched the places throwing this failed assert as well but others may remain in other functions maybe, as i didn't check them all only the ones that fired these failed asserts ingame).
		// Sanity check maybe as well and possibly future proof as chatgpt 5 said and as i would also think too before that xd thanks i mean but check if accurate, see known issue as of now 72 for details, and check if accurate i mean too -->
		// Assert Failed
		// File: c:\program files (x86)\steam\steamapps\common\sid meier's civilization iv beyond the sword\beyond the sword\mods\advciv-sas\cvgamecoredll\CvRandom.h
		// Line: 31
		// Func: CvRandom::getInt
		// Expression: static_cast<int>(iNum) >= static_cast<int>(0)
		// Message: Index expected to be >= 0. (value: -4)
		//
		// Good catch. This assert means someone called the RNG with a negative range. Your dump shows:
		// CvRandom::getInt
		// → CvGame::getSorenRandNum
		// → CvCityAI::AI_chooseProduction
		// → CvCity::popOrder → CvCity::doProduction → …
		// So a branch in AI_chooseProduction computed iNum = -4 and passed it to getSorenRandNum(iNum, …).
		// Safety net (tiny, future-proof)
		// Make CvRandom::getInt(int …) tolerant so you can keep playing while you hunt the caller:
		if (iNum <= 0)
		{
			FAssertMsg(iNum >= 0, CvString::format(
				"getInt: range<0 (%d) msg=%s d1=%d d2=%d",
				iNum, (szMsg ? szMsg : ""), iData1, iData2).c_str());
			// preserve old semantics: return 0 when range is 0/invalid
			return 0;
		}
		/*	</advc.001n>  <advc.006> The compiler doesn't warn when an int variable gets
			passed as a short int. It does warn when a large int literal is passed, but
			I think that's not quite good enough. (At the least, the wrapper functions at
			CvGame would have to take their param as unsigned short too.)*/
		FAssertBounds(0, getRange() + 1, iNum);
		/*	If the upper bound above is exceeded, then e.g. iNum=66868 gets cast to 1332,
			which is 66868-MAX_UNSIGNED_SHORT+1. Don't know how this works in general. */
		return getInt(static_cast<unsigned short>(iNum), szMsg, iData1, iData2);
	}
	static int getRange() { return MAX_UNSIGNED_SHORT; } // Client code may want to check
	// </advc.006>
	// advc.190c: Exported through .def file
	unsigned short getExternal(unsigned short usNum, TCHAR const* szMsg = NULL);
	DllExport float getFloat() { return get(MAX_UNSIGNED_SHORT) / (float)MAX_UNSIGNED_SHORT; }

	void reseed(unsigned int uiNewValue);
	unsigned int getSeed();

	/*	<advc> Shuffle functions moved from CvGameCoreUtils
		(as it's also done in Civ4Col). */
	int* shuffle(int iNum) // Caller deletes the returned array
	{
		int* piShuffle = new int[iNum];
		shuffle(piShuffle, iNum);
		return piShuffle;
	}
	// renamed from "shuffleArray"
	void shuffle(int* piShuffle, int iNum); // Caller allocates and frees the array
	// </advc>  advc.enum:
	void shuffle(std::vector<int>& kIndices); // Caller sets the vector size
	// advc.304 (may find other uses too):
	template<class ItemType>
	// NULL means uniform weights. Uniform choice isn't what this function is for, but it's convenient to have as an option.
	// <!-- custom: hoisted from multiline signature between `kItems` and `pWeights` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	ItemType* weightedChoice(std::vector<ItemType*> const& kItems, std::vector<int> const* pWeights = NULL);

protected:
	// advc.007c
	// <!-- custom: hoisted from multiline signature between `usNum` and `iData1` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	virtual void printToLog(TCHAR const* szMsg, unsigned short usNum, int iData1, int iData2); // advc.001n

	unsigned int m_uiRandomSeed;
	// advc.001n, advc.006:
	unsigned short getInt(unsigned short usNum, TCHAR const* szMsg, int iData1, int iData2);
};

// advc.003k: Gets instantiated (also) externally; size mustn't change.
BOOST_STATIC_ASSERT(sizeof(CvRandom) == 8);

/*	advc.007c: Since I can't store a log file name at CvRandom,
	let's make a class that'll only get instantiated in the DLL. */
class CvRandomExtended : public CvRandom
{
public:
	/*	Caveat: Calling this on a serialized CvRandom object will break
		savegame compatibility unless the owning class increases the uiFlag constant
		in its write function. */
	void setLogFileName(CvString szName);
	void read(FDataStreamBase* pStream); // override
	void write(FDataStreamBase* pStream); // override
protected:
	// override:
	void printToLog(TCHAR const* szMsg, unsigned short usNum, int iData1, int iData2);
	CvString m_szFileName;
};

// advc: Moved from CvGameCoreUtils, exported through .def file.
int* shuffleExternal(int iNum, CvRandom& kRand) { return kRand.shuffle(iNum); }

// advc.304:
template<class ItemType>
ItemType* CvRandom::weightedChoice(std::vector<ItemType*> const& kItems, std::vector<int> const* pWeights)
{
	std::vector<int> aiLowPrecisionWeights;
	int iTotal = 0;
	if (pWeights == NULL)
		iTotal = (int)kItems.size();
	else
	{
		FAssert(pWeights->size() == kItems.size());
		for (size_t i = 0; i < pWeights->size(); i++)
			iTotal += (*pWeights)[i];
		FAssertMsg(iTotal >= 0, "overflow?");
		// This we'll need to be able to recover from I think:
		int const iRange = safeIntCast<int>(getRange());
		if (iTotal > iRange)
		{
			scaled rDownScaleFactor(iRange, iTotal);
			rDownScaleFactor.decreaseTo(1 - scaled::epsilon());
			aiLowPrecisionWeights = *pWeights; // copy
			pWeights = &aiLowPrecisionWeights;
			iTotal = 0; // recompute
			for (size_t i = 0; i < aiLowPrecisionWeights.size(); i++)
			{
				aiLowPrecisionWeights[i] = (aiLowPrecisionWeights[i] *
						rDownScaleFactor).floor();
				iTotal += aiLowPrecisionWeights[i];
			}
			FAssert(iTotal > 0 && iTotal <= iRange);
		}
	}
	if (iTotal == 0)
		return NULL;
	int iRoll = get(iTotal, "weightedChoice");
	if (pWeights == NULL)
		return kItems[iRoll];
	for (size_t i = 0; i < kItems.size(); i++)
	{
		iRoll -= (*pWeights)[i];
		if (iRoll < 0)
			return kItems[i];
	}
	FErrorMsg("Negative weights?");
	return NULL;
}

#endif
