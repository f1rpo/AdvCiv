#pragma once

#ifndef CV_DLL_LOGGER_H
#define CV_DLL_LOGGER_H

/*  advc: New class. Encapsulates calls to messageControlLog (CvDLLUtilityIFaceBase).
	The log is enabled through "MessageLog" in CivilizationIV.ini. "LoggingEnabled"
	doesn't matter -- and this probably can't be changed b/c the DLL can't check if
	LoggingEnabled is set. (CvGlobals::m_bLogging only says whether MessageLog is set.) */
class CvDLLLogger : private boost::noncopyable
{
public:
	CvDLLLogger(bool bEnabled, bool bRandEnabled);
	// Requires "RandLog" to be set in addition to "MessageLog"
	void logRandomNumber(const TCHAR* szMsg, unsigned short usNum, unsigned long ulSeed,
			int iData1, int iData2);
	void logTurnActive(PlayerTypes ePlayer);
	void logCityBuilt(CvCity const& kCity);
	void logCombat(CvUnit const& kAttacker, CvUnit const& kDefender);
	void logUnitStuck(CvUnit const& kUnit);
	void logMapStats(); // advc.mapstat

private:
	bool m_bEnabled;
	bool m_bRandEnabled;

	inline bool isEnabled(bool bRand = false) const
	{
		return bRand ? m_bRandEnabled : m_bEnabled;
	}
};

#endif
