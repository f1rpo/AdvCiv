#pragma once

// random.h

#ifndef CIV4_RANDOM_H
#define CIV4_RANDOM_H

class CvRandom
{
public:
	DllExport CvRandom();
	DllExport virtual ~CvRandom(); // advc (comment): Can't make this non-virtual b/c of DllExport

	DllExport void init(unsigned long ulSeed);
	void reset(unsigned long ulSeed = 0);
	/*	Returns a value from the half-open interval [0, iNum).
		advc.006: iNum taken as an int but needs to be in [0, 65535].
		Will return 0 for iNum=0 (and also for iNum=1). */
	DllExport inline unsigned short get(int iNum, TCHAR const* szLog = NULL,
			int iData1 = MIN_INT, int iData2 = MIN_INT)
	{	// <advc.001n>
		return getInt(iNum, szLog, MIN_INT);
	} // New name to avoid issues in CyRandomPythonInterface
	inline unsigned short getInt(int iNum, TCHAR const* szLog, int iData1, int iData2 = MIN_INT)
	{	/*	</advc.001n>  <advc.006> The compiler doesn't warn when an int variable gets
			passed as a short int. It does warn when a large int literal is passed, but
			I think that's not quite good enough. (At the least, the wrapper functions at
			CvGame would have to take their param as unsigned short too.)*/
		FAssertBounds(0, getRange() + 1, iNum);
		/*	If the upper bound above is exceeded, then e.g. iNum=66868 gets cast to 1332,
			which is 66868-MAX_UNSIGNED_SHORT+1. Don't know how this works in general. */
		return getInt(static_cast<unsigned short>(iNum), szLog, iData1, iData2);
	}
	static inline int getRange() { return MAX_UNSIGNED_SHORT; } // Client code may want to check
	// </advc.006>
	DllExport float getFloat();

	void reseed(unsigned long ulNewValue);
	unsigned long getSeed();

	// for serialization
	void read(FDataStreamBase* pStream);
	void write(FDataStreamBase* pStream);

protected:
	unsigned long m_ulRandomSeed;
	// advc.001n, advc.006:
	unsigned short getInt(unsigned short usNum, TCHAR const* szLog, int iData1, int iData2);
};

#endif
