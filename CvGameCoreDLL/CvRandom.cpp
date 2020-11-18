#include "CvGameCoreDLL.h"
#include "CvRandom.h"
#include "CvGame.h"


#define RANDOM_A      (1103515245)
#define RANDOM_C      (12345)
#define RANDOM_SHIFT  (16)

unsigned short CvRandom::getInt(unsigned short usNum, TCHAR const* szLog,
	int iData1, int iData2) // advc.001n
{	// <advc.003t>
	if (GC.getLogger().isEnabledRand() && szLog != NULL)
		printToLog(szLog, usNum, iData1, iData2); // </advc.003t>
	m_ulRandomSeed = (RANDOM_A * m_ulRandomSeed) + RANDOM_C;
	unsigned short r = (unsigned short)
			((((m_ulRandomSeed >> RANDOM_SHIFT) & MAX_UNSIGNED_SHORT) *
			((unsigned long)usNum)) / (MAX_UNSIGNED_SHORT + 1));
	return r;
}


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

/*	advc.190c: Separate function for calls from the EXE. So that the DLL can figure out
	which options were set to "Random" during game setup. */
unsigned short CvRandom::getExternal(unsigned short usNum, TCHAR const* szLog)
{
	GC.getInitCore().externalRNGCall(usNum, this);
	return get(usNum, szLog);
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

// <advc.007b>
/*	Two function calls that won't get inlined, but it doesn't really matter -
	only gets called if the log is enabled. */
void CvRandom::printToLog(TCHAR const* szMsg, unsigned short usNum,
	int iData1, int iData2) // advc.001n
{	// advc.003t:
	GC.getLogger().logRandomNumber(szMsg, usNum, m_ulRandomSeed, iData1, iData2);
}


void CvRandomExtended::printToLog(TCHAR const* szMsg, unsigned short usNum,
	int iData1, int iData2) // advc.001n
{
	GC.getLogger().logRandomNumber(szMsg, usNum, m_ulRandomSeed, iData1, iData2,
			&m_szFileName);
}

// Initializes data members that are serialized.
void CvRandomExtended::reset(unsigned long ulSeed)
{
	CvRandom::reset(ulSeed);
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
// </advc.007b>
