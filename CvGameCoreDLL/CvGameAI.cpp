// gameAI.cpp

#include "CvGameCoreDLL.h"
#include "CvGameAI.h"
#include "CvGamePlay.h"
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
	AI_sortOutWPAIOptions(false); // advc.104
}

// <advc.104u>
/*  Parts of the AI don't seem to get properly initialized in scenarios. Not
	sure if this has always been the case, if it has to do with K-Mod changes to
	the turn order (team turns vs. player turns) or is a problem I introduced.
	Amendment: */
void CvGameAI::AI_initScenario()
{
	// Citizens not properly assigned
	for(int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)i);
		if(!kPlayer.isAlive())
			continue;

		FOR_EACH_CITYAI_VAR(c, kPlayer)
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
	// Ensure UWAI initialization
	for(int i = 0; i < MAX_CIV_TEAMS; i++)
	{
		CvTeamAI& t = GET_TEAM((TeamTypes)i);
		if(t.isAlive())
			t.AI_doTurnPre();
	}
} // </advc.104u>

/*  <advc.104> I'm repurposing the Aggressive AI option so that it enables the
	(legacy) war/peace behavior from K-Mod in addition to the option's normal effect.
	A bit of a hack, but less invasive than changing all the
	isOption(AGGRESSIVE_AI) checks. And I don't want two separate options because
	the new war/peace AI implies Aggressive AI. */
void CvGameAI::AI_sortOutWPAIOptions(bool bFromSaveGame)
{
	if(GC.getDefineINT("USE_KMOD_AI_NONAGGRESSIVE"))
	{
		m_wpai.setUseKModAI(true);
		setOption(GAMEOPTION_AGGRESSIVE_AI, false);
		return;
	}
	if(GC.getDefineINT("DISABLE_UWAI"))
	{
		m_wpai.setUseKModAI(true);
		setOption(GAMEOPTION_AGGRESSIVE_AI, true);
		return;
	}
	m_wpai.setInBackground(GC.getDefineINT("UWAI_IN_BACKGROUND") > 0);
	if(bFromSaveGame)
	{
		if(m_wpai.isEnabled() || m_wpai.isEnabled(true))
			setOption(GAMEOPTION_AGGRESSIVE_AI, true);
		return;
	}
	// If still not returned: settings according to Custom Game screen
	bool bUseKModAI = isOption(GAMEOPTION_AGGRESSIVE_AI);
	m_wpai.setUseKModAI(bUseKModAI);
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
	m_wpai.read(pStream);
	AI_sortOutWPAIOptions(true);
	// </advc.104>
}


void CvGameAI::write(FDataStreamBase* pStream)
{
	CvGame::write(pStream);

	uint uiFlag=0;
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iPad);

	m_wpai.write(pStream); // advc.104
}
