// random.cpp

#include "CvGameCoreDLL.h"
#include "CvRandom.h"
#include "CvGlobals.h"
#include "CyArgsList.h"
#include "CvGameAI.h"

#define RANDOM_A      (1103515245)
#define RANDOM_C      (12345)
#define RANDOM_SHIFT  (16)

CvRandom::CvRandom()
{ 
	reset();
}


CvRandom::~CvRandom()
{ 
	uninit();
}


void CvRandom::init(unsigned long ulSeed)
{
	//--------------------------------
	// Init saved data
	reset(ulSeed);

	//--------------------------------
	// Init non-saved data
}


void CvRandom::uninit()
{
}


// FUNCTION: reset()
// Initializes data members that are serialized.
void CvRandom::reset(unsigned long ulSeed)
{
	//--------------------------------
	// Uninit class
	uninit();

	m_ulRandomSeed = ulSeed;
}


unsigned short CvRandom::get(unsigned short usNum, const TCHAR* pszLog)
{
	if (pszLog != NULL)
	{
		if (GC.getLogging() && GC.getRandLogging())
		{
			CvGame const& g = GC.getGameINLINE(); // advc.003
			if (g.getTurnSlice() > 0)
			{
				TCHAR szOut[1024];
				sprintf(szOut, "Rand = %ul / %hu (%s) on %d\n", getSeed(), usNum, pszLog, GC.getGameINLINE().getTurnSlice());
				// <advc.007>
				if(GC.getPER_PLAYER_MESSAGE_CONTROL_LOG() > 0 &&
						g.isNetworkMultiPlayer()) {
					CvString logName = CvString::format("MPLog%d.log",
							(int)g.getActivePlayer());
					gDLL->logMsg(logName.c_str(), szOut, false, false);
				}
				else // </advc.007>
					gDLL->messageControlLog(szOut);
			}
		}
	}

	m_ulRandomSeed = ((RANDOM_A * m_ulRandomSeed) + RANDOM_C);

	unsigned short us = ((unsigned short)((((m_ulRandomSeed >> RANDOM_SHIFT) & MAX_UNSIGNED_SHORT) * ((unsigned long)usNum)) / (MAX_UNSIGNED_SHORT + 1)));

	return us;
}


float CvRandom::getFloat()
{
	return (((float)(get(MAX_UNSIGNED_SHORT))) / ((float)MAX_UNSIGNED_SHORT));
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
