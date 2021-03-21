#pragma once

#ifndef CIV4_RANDOM_H
#define CIV4_RANDOM_H

class CvRandom
{
public:
	DllExport CvRandom();
	DllExport virtual ~CvRandom(); // advc (comment): Can't make this non-virtual b/c of DllExport

	DllExport void init(unsigned long ulSeed);
	virtual void reset(unsigned long ulSeed = 0); // advc.007b: virtual
	// for serialization
	virtual void read(FDataStreamBase* pStream); // advc.007b: virtual
	virtual void write(FDataStreamBase* pStream); // advc.007b: virtual
	/*	Returns a value from the half-open interval [0, iNum).
		advc.006: iNum taken as an int but needs to be in [0, 65535].
		Will return 0 for iNum=0 (and also for iNum=1). */
	inline unsigned short get(int iNum, TCHAR const* szMsg = NULL,
			int iData1 = MIN_INT, int iData2 = MIN_INT) // advc.128
	{	// <advc.001n>
		return getInt(iNum, szMsg, MIN_INT);
	} // New name to avoid issues in CyRandomPythonInterface
	inline unsigned short getInt(int iNum, TCHAR const* szMsg,
		int iData1, int iData2 = MIN_INT)
	{	/*	</advc.001n>  <advc.006> The compiler doesn't warn when an int variable gets
			passed as a short int. It does warn when a large int literal is passed, but
			I think that's not quite good enough. (At the least, the wrapper functions at
			CvGame would have to take their param as unsigned short too.)*/
		FAssertBounds(0, getRange() + 1, iNum);
		/*	If the upper bound above is exceeded, then e.g. iNum=66868 gets cast to 1332,
			which is 66868-MAX_UNSIGNED_SHORT+1. Don't know how this works in general. */
		return getInt(static_cast<unsigned short>(iNum), szMsg, iData1, iData2);
	}
	static inline int getRange() { return MAX_UNSIGNED_SHORT; } // Client code may want to check
	// </advc.006>
	// advc.190c: Exported through .def file
	unsigned short getExternal(unsigned short usNum, TCHAR const* szMsg = NULL);
	DllExport float getFloat() // advc.inl
	{
		return get(MAX_UNSIGNED_SHORT) / (float)MAX_UNSIGNED_SHORT;
	}

	void reseed(unsigned long ulNewValue);
	unsigned long getSeed();

protected:
	virtual void printToLog(TCHAR const* szMsg, unsigned short usNum, // advc.007b
			int iData1, int iData2); // advc.001n

	unsigned long m_ulRandomSeed;
	// advc.001n, advc.006:
	unsigned short getInt(unsigned short usNum, TCHAR const* szMsg, int iData1, int iData2);
};

// advc.003k: Gets instantiated (also) externally; size mustn't change.
BOOST_STATIC_ASSERT(sizeof(CvRandom) == 8);

// <advc.007b>
/*	Since I can't store a log file name at CvRandom,
	let's make a class that'll only get instantiated in the DLL. */
class CvRandomExtended : public CvRandom
{
public:
	/*	Caveat: Calling this on a serialized CvRandom object will break
		savegame compatibility unless the owning class increases the uiFlag constant
		in its write function. */
	void setLogFileName(CvString szName);
	// The rest are overrides
	void reset(unsigned long ulSeed = 0);
	void read(FDataStreamBase* pStream);
	void write(FDataStreamBase* pStream);
protected:
	// Override:
	void printToLog(TCHAR const* szMsg, unsigned short usNum, int iData1, int iData2);
	CvString m_szFileName;
};

/*	Some macros that should make the logging aspect less tedious.
	Still unsure if I want to use them. Unused (and untested) so far. */

// May well be useful elsewhere, but let's put it where it gets used for now.
#define STRINGIFY_HELPER2(x) #x
#define STRINGIFY_HELPER1(x) STRINGIFY_HELPER2(x)
#define CALL_LOC_STR __FUNCTION__ "(" __FILE__ "):" STRINGIFY_HELPER1(__LINE__)

// ASyncRand gets used so little in the DLL; don't need a macro.
/*#define AsyncRandNum(iNumOutcomes) \
	GC.getASyncRand().get((iNumOutcomes), CALL_LOC_STR)*/

// The rest will require the CvGame header:

/*	Implementation files that include the CvRandom header can re-define this
	to use a different CvGame instance */
#define CVGAME_INSTANCE_FOR_RNG GC.getGame()

#define SyncRandNum(iNumOutcomes) \
	CVGAME_INSTANCE_FOR_RNG.getSRandNum((iNumOutcomes), CALL_LOC_STR)
#define MapRandNum(iNumOutcomes) \
	CVGAME_INSTANCE_FOR_RNG.getMapRandNum((iNumOutcomes), CALL_LOC_STR)
/*	These take a ScaledNum instance or instantiation as parameter
	and will therefore require the ScaledNum header. */
#define SyncRandFract(T) \
	T::rand(CVGAME_INSTANCE_FOR_RNG.getSRand(), CALL_LOC_STR)
#define SyncRandSuccess(rSuccessProbability) \
	(rSuccessProbability).bernoulliSuccess( \
	CVGAME_INSTANCE_FOR_RNG.getSRand(), CALL_LOC_STR)
// </advc.007b>

#endif
