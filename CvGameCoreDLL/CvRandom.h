#pragma once

// random.h

#ifndef CIV4_RANDOM_H
#define CIV4_RANDOM_H

class CvRandom
{

public:

	DllExport CvRandom();
	DllExport virtual ~CvRandom();

	DllExport void init(unsigned long ulSeed);
	void uninit();
	void reset(unsigned long ulSeed = 0);

	DllExport unsigned short get(unsigned short usNum, const TCHAR* pszLog = NULL) {  //  Returns value from 0 to num-1 inclusive.
		// <advc.001n>
		return getInt(usNum, pszLog, MIN_INT, MIN_INT);
	} // New name to avoid issues in CyRandomPythonInterface
	unsigned short getInt(unsigned short usNum, const TCHAR* pszLog, int iData1, int iData2 = MIN_INT);
	// </advc.001n>
	DllExport float getFloat();

	void reseed(unsigned long ulNewValue);
	unsigned long getSeed();

	// for serialization
	void read(FDataStreamBase* pStream);
	void write(FDataStreamBase* pStream);

protected:

	unsigned long m_ulRandomSeed;
};

#endif
