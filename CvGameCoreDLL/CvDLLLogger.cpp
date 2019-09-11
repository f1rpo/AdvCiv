// advc.003t: New class; see CvDLLLogger.h.

#include "CvGameCoreDLL.h"
#include "CvGame.h"
#include "CvPlayer.h"
#include "CvCity.h"
#include "CvUnit.h"


CvDLLLogger::CvDLLLogger(bool bEnabled, bool bRandEnabled)
	: m_bEnabled(bEnabled), m_bRandEnabled(bRandEnabled) {}

// Cut from CvRandom::getInt
void CvDLLLogger::logRandomNumber(const TCHAR* szMsg, unsigned short usNum,
		unsigned long ulSeed, int iData1, int iData2)
{
	if (!isEnabled(true))
		return;
	int const iTurnSlice = GC.getGame().getTurnSlice();
	if (iTurnSlice <= 0)
		return;
	TCHAR szOut[1024];
	// <advc.007>
	CvString szData;
	if(iData1 > MIN_INT)
	{
		if(iData2 == MIN_INT)
			szData.Format(" (%d)", iData1);
		else szData.Format(" (%d, %d)", iData1, iData2);
	}
	bool bNetworkMP = GC.getGame().isNetworkMultiPlayer();
	/*  Don't show iTurnSlice in singleplayer b/c that makes it harder to
		compare log files. */
	int iOn = iTurnSlice;
	if (!bNetworkMP)
		iOn = GC.getGame().getGameTurn(); // (any more useful info to put here?)
	// The second and last %s are new
	std::sprintf(szOut, "Rand = %ul / %hu (%s%s) on %s%d\n", ulSeed, usNum,
			szMsg, szData.c_str(), bNetworkMP ? "" : "t", iOn);
	if (GC.getDefineBOOL(CvGlobals::PER_PLAYER_MESSAGE_CONTROL_LOG) && bNetworkMP)
	{
		CvString logName = CvString::format("MPLog%d.log",
				(int)GC.getGame().getActivePlayer());
		gDLL->logMsg(logName.c_str(), szOut, false, false);
	}
	else // </advc.007>
		gDLL->messageControlLog(szOut);
}

// Cut from CvPlayer::setTurnActive
void CvDLLLogger::logTurnActive(PlayerTypes ePlayer)
{
	if (!isEnabled()) // || gDLL->getChtLvl() <= 0) // advc.007
		return;
	TCHAR szOut[1024];
	std::sprintf(szOut, "Player %d Turn ON\n", ePlayer);
	gDLL->messageControlLog(szOut);
}

// Cut from CvCity::init
void CvDLLLogger::logCityBuilt(CvCity const& kCity)
{
	if (!isEnabled()) //|| gDLL->getChtLvl() <= 0) // advc.007
		return;
	TCHAR szOut[1024];
	std::sprintf(szOut, "Player %d City %d built at %d:%d\n", kCity.getOwner(),
			kCity.getID(), kCity.getX(), kCity.getY());
	gDLL->messageControlLog(szOut);
}

// Cut from CvUnit::setCombatUnit
void CvDLLLogger::logCombat(CvUnit const& kAttacker, CvUnit const& kDefender)
{
	if (!isEnabled()) //|| gDLL->getChtLvl() <= 0) // advc.007
		return;
	char szOut[1024];
	std::sprintf( szOut, "*** KOMBAT!\n     ATTACKER: Player %d Unit %d (%S's %S), CombatStrength=%d\n"
		"     DEFENDER: Player %d Unit %d (%S's %S), CombatStrength=%d\n",
			kAttacker.getOwner(), kAttacker.getID(), GET_PLAYER(kAttacker.getOwner()).getName(), kAttacker.getName().GetCString(), kAttacker.currCombatStr(NULL, NULL),
			kDefender.getOwner(), kDefender.getID(), GET_PLAYER(kDefender.getOwner()).getName(), kDefender.getName().GetCString(), kDefender.currCombatStr(kDefender.plot(), &kAttacker));
	gDLL->messageControlLog(szOut);
}

// Cut from CvSelectionGroupAI::AI_update
void CvDLLLogger::logUnitStuck(CvUnit const& kUnit)
{
	if (!isEnabled())
		return;
	TCHAR szOut[1024];
	CvWString szTempString;
	::getUnitAIString(szTempString, kUnit.AI_getUnitAIType());
	std::sprintf(szOut, "Unit stuck in loop: %S(%S)[%d, %d] (%S)\n",
			kUnit.getName().GetCString(), GET_PLAYER(kUnit.getOwner()).getName(),
			kUnit.getX(), kUnit.getY(), szTempString.GetCString());
	gDLL->messageControlLog(szOut);
}
