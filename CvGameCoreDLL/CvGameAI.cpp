// gameAI.cpp

#include "CvGameCoreDLL.h"
#include "CvGameAI.h"
#include "CvPlayerAI.h"
#include "CvTeamAI.h"
#include "CvGlobals.h"
#include "CvInfos.h"
// advc.137:
#include "CvInitCore.h"

// Public Functions...

CvGameAI::CvGameAI()
{
	AI_reset();
}


CvGameAI::~CvGameAI()
{
	AI_uninit();
}


void CvGameAI::AI_init()
{
	AI_reset();

	//--------------------------------
	// Init other game data

	sortOutWPAIOptions(false); // advc.104
}

// <advc.104>
WarAndPeaceAI& CvGameAI::warAndPeaceAI() {

	return wpai;
}

void CvGameAI::sortOutWPAIOptions(bool fromSaveGame) {

	if(GC.getDefineINT("USE_KMOD_AI_NONAGGRESSIVE")) {
		wpai.setUseKModAI(true);
		setOption(GAMEOPTION_AGGRESSIVE_AI, false);
		return;
	}
	if(GC.getDefineINT("DISABLE_UWAI")) {
		wpai.setUseKModAI(true);
		setOption(GAMEOPTION_AGGRESSIVE_AI, true);
		return;
	}
	wpai.setInBackground(GC.getDefineINT("UWAI_IN_BACKGROUND") > 0);
	if(fromSaveGame) {
		if(wpai.isEnabled() || wpai.isEnabled(true))
			setOption(GAMEOPTION_AGGRESSIVE_AI, true);
		return;
	}
	// If still not returned: settings according to Custom Game screen
	bool useKModAI = isOption(GAMEOPTION_AGGRESSIVE_AI);
	wpai.setUseKModAI(useKModAI);
	if(!useKModAI)
		setOption(GAMEOPTION_AGGRESSIVE_AI, true);
} // </advc.104>

void CvGameAI::AI_uninit()
{
}


void CvGameAI::AI_reset()
{
	AI_uninit();

	m_iPad = 0;
}


void CvGameAI::AI_makeAssignWorkDirty()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).AI_makeAssignWorkDirty();
		}
	}
}


void CvGameAI::AI_updateAssignWork()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (GET_TEAM(kLoopPlayer.getTeam()).isHuman() && kLoopPlayer.isAlive())
		{
			kLoopPlayer.AI_updateAssignWork();
		}
	}
}


int CvGameAI::AI_combatValue(UnitTypes eUnit) const
{
	int iValue;

	iValue = 100;

	if (GC.getUnitInfo(eUnit).getDomainType() == DOMAIN_AIR)
	{
		iValue *= GC.getUnitInfo(eUnit).getAirCombat();
	}
	else
	{
		iValue *= GC.getUnitInfo(eUnit).getCombat();

		iValue *= ((((GC.getUnitInfo(eUnit).getFirstStrikes() * 2) + GC.getUnitInfo(eUnit).getChanceFirstStrikes()) * (GC.getCOMBAT_DAMAGE() / 5)) + 100);
		iValue /= 100;
	}

	iValue /= getBestLandUnitCombat();

	return iValue;
}


int CvGameAI::AI_turnsPercent(int iTurns, int iPercent)
{
	FAssert(iPercent > 0);
	if (iTurns != MAX_INT)
	{
		iTurns *= (iPercent);
		iTurns /= 100;
	}

	return std::max(1, iTurns);
}


void CvGameAI::read(FDataStreamBase* pStream)
{
	CvGame::read(pStream);

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iPad);
	// <advc.104>
	wpai.read(pStream);
	sortOutWPAIOptions(true);
	// </advc.104>
}


void CvGameAI::write(FDataStreamBase* pStream)
{
	CvGame::write(pStream);

	uint uiFlag=0;
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iPad);

	wpai.write(pStream); // advc.104
}

// Protected Functions...

// Private Functions...
