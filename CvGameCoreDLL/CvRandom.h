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
	//  Returns a value from the half-open interval [0, usNum)
	DllExport unsigned short get(unsigned short usNum, const TCHAR* pszLog = NULL)
	{	// <advc.001n>
		return getInt(usNum, pszLog, MIN_INT);
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
