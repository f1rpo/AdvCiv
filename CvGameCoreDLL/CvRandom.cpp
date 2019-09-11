// random.cpp

#include "CvGameCoreDLL.h"
#include "CvRandom.h"
#include "CvGame.h"

#define RANDOM_A      (1103515245)
#define RANDOM_C      (12345)
#define RANDOM_SHIFT  (16)

CvRandom::CvRandom()
{
	reset();
}


CvRandom::~CvRandom() {}


void CvRandom::init(unsigned long ulSeed)
{
	reset(ulSeed);
}

// Initializes data members that are serialized.
void CvRandom::reset(unsigned long ulSeed)
{
	m_ulRandomSeed = ulSeed;
}


unsigned short CvRandom::getInt(unsigned short usNum, const TCHAR* pszLog,
		int iData1, int iData2) // advc.001n
{
	if (pszLog != NULL)
		GC.getLogger().logRandomNumber(pszLog, usNum, m_ulRandomSeed, iData1, iData2); // advc.003t

	m_ulRandomSeed = (RANDOM_A * m_ulRandomSeed) + RANDOM_C;
	unsigned short r = (unsigned short)
		((((m_ulRandomSeed >> RANDOM_SHIFT) & MAX_UNSIGNED_SHORT) *
		((unsigned long)usNum)) / (MAX_UNSIGNED_SHORT + 1));
	return r;
}


float CvRandom::getFloat()
{
	return get(MAX_UNSIGNED_SHORT) / (float)MAX_UNSIGNED_SHORT;
}


void CvRandom::reseed(unsigned long ulNewValue)
{
	m_ulRandomSeed = ulNewValue;
}


unsigned long CvRandom::getSeed()
{
	return m_ulRandomSeed;
}


void CvRandom::read(FDataStreamBase* pStream)
{
	reset();
	pStream->Read(&m_ulRandomSeed);
}


void CvRandom::write(FDataStreamBase* pStream)
{
	pStream->Write(m_ulRandomSeed);
}
