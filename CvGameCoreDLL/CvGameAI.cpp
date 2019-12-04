// gameAI.cpp

#include "CvGameCoreDLL.h"
#include "CvGameAI.h"
#include "CvAI.h"
#include "UWAIAgent.h"
#include "CvInfo_Unit.h"


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
	AI_sortOutUWAIOptions(false); // advc.104
}

// <advc.104u>
/*  Parts of the AI don't seem to get properly initialized in scenarios. Not
	sure if this has always been the case, if it has to do with K-Mod changes to
	the turn order (team turns vs. player turns) or is a problem I introduced.
	Amendment: */
void CvGameAI::AI_initScenario()
{
	// Citizens not properly assigned
	for (PlayerIter<ALIVE> it; it.hasNext(); ++it)
	{
		FOR_EACH_CITYAI_VAR(c, *it)
		{
			/*  Added after getting failed assertions in CvCity::doTurn in the
				Europe1000AD scenario (I'm guessing due to production from Apostolic Palace). */
			for(int j = 0; j < NUM_YIELD_TYPES; j++)
			{
				YieldTypes y = (YieldTypes)j;
				c->setBaseYieldRate(y, c->calculateBaseYieldRate(y));
			}
			c->AI_assignWorkingPlots();
		}
	}
	for (TeamIter<MAJOR_CIV> it; it.hasNext(); ++it)
		it->uwai().turnPre();
} // </advc.104u>

/*  <advc.104> I'm repurposing the Aggressive AI option so that it disables UWAI
	in addition to the option's normal effect. A bit of a hack, but less invasive
	than changing all the isOption(AGGRESSIVE_AI) checks. Don't want two separate
	options because UWAI implies Aggressive AI. */
void CvGameAI::AI_sortOutUWAIOptions(bool bFromSaveGame)
{
	if(GC.getDefineINT("USE_KMOD_AI_NONAGGRESSIVE"))
	{
		m_uwai.setUseKModAI(true);
		setOption(GAMEOPTION_AGGRESSIVE_AI, false);
		return;
	}
	if(GC.getDefineINT("DISABLE_UWAI"))
	{
		m_uwai.setUseKModAI(true);
		setOption(GAMEOPTION_AGGRESSIVE_AI, true);
		return;
	}
	m_uwai.setInBackground(GC.getDefineINT("UWAI_IN_BACKGROUND") > 0);
	if(bFromSaveGame)
	{
		if(m_uwai.isEnabled() || m_uwai.isEnabled(true))
			setOption(GAMEOPTION_AGGRESSIVE_AI, true);
		return;
	}
	// If still not returned: settings according to Custom Game screen
	bool bUseKModAI = isOption(GAMEOPTION_AGGRESSIVE_AI);
	m_uwai.setUseKModAI(bUseKModAI);
	if(!bUseKModAI)
		setOption(GAMEOPTION_AGGRESSIVE_AI, true);
} // </advc.104>


void CvGameAI::AI_uninit() {}


void CvGameAI::AI_reset()
{
	AI_uninit();

	m_iPad = 0;
}


void CvGameAI::AI_makeAssignWorkDirty()
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).AI_makeAssignWorkDirty();
		}
	}
}


void CvGameAI::AI_updateAssignWork()
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (GET_TEAM(kLoopPlayer.getTeam()).isHuman() && kLoopPlayer.isAlive())
		{
			kLoopPlayer.AI_updateAssignWork();
		}
	}
}


int CvGameAI::AI_combatValue(UnitTypes eUnit) /* K-Mod: */ const
{
	int iValue = 100;

	if (GC.getInfo(eUnit).getDomainType() == DOMAIN_AIR)
	{
		iValue *= GC.getInfo(eUnit).getAirCombat();
	}
	else
	{
		iValue *= GC.getInfo(eUnit).getCombat();

		iValue *= ((((GC.getInfo(eUnit).getFirstStrikes() * 2) + GC.getInfo(eUnit).getChanceFirstStrikes()) * (GC.getCOMBAT_DAMAGE() / 5)) + 100);
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
	m_uwai.read(pStream);
	AI_sortOutUWAIOptions(true);
	// </advc.104>
}


void CvGameAI::write(FDataStreamBase* pStream)
{
	CvGame::write(pStream);

	uint uiFlag=0;
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iPad);

	m_uwai.write(pStream); // advc.104
}
