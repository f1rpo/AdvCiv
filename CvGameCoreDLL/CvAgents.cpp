// advc.agent: New file; see comment in header file.

#include "CvGameCoreDLL.h"
#include "CvAgents.h"
#include "AgentIterator.h"
#include "CvAI.h"

CvAgents::CvAgents(int iMaxPlayers, int iMaxTeams)
{
	// Tbd.: Instead create the CvPlayerAI and CvTeamAI objects here

	m_playerSeqCache.resize(NUM_STATUS_CACHES);
	m_teamSeqCache.resize(NUM_STATUS_CACHES);
	m_memberSeqCache.resize(NUM_RELATION_CACHES);
	m_teamPerTeamSeqCache.resize(NUM_RELATION_CACHES);
	for (int i = 0; i < NUM_RELATION_CACHES; i++)
	{
		m_memberSeqCache[i].resize(iMaxTeams);
		m_teamPerTeamSeqCache[i].resize(iMaxTeams);
	}
	for (int i = 0; i < iMaxTeams; i++)
	{
		CvTeamAI* pTeam = &GET_TEAM((TeamTypes)i);
		teamSeqCache(ALL).push_back(pTeam);
	}
	for (int i = 0; i < iMaxPlayers; i++)
	{
		CvPlayerAI* pPlayer = &GET_PLAYER((PlayerTypes)i);
		playerSeqCache(ALL).push_back(pPlayer);
	}
	AgentIteratorBase::setAgentsCache(this);
}


void CvAgents::gameStart(bool bFromSaveGame)
{
	updateAllCachedSequences();
	// <advc.test>
	if (bFromSaveGame)
	{
		int TestAgentIterator(int);
		int iNonsense = TestAgentIterator(100);
		if (iNonsense == -1) // Just to make sure that the whole thing doesn't somehow get optimized away
		{
			FAssert(false);
			updateAllCachedSequences();
		}
	} // </advc.test>
}

void CvAgents::updateAllCachedSequences()
{
	for (int i = ALL + 1; i < NUM_STATUS_CACHES; i++)
	{
		AgentSeqCache eCacheID = (AgentSeqCache)i;
		playerSeqCache(eCacheID).clear();
		teamSeqCache(eCacheID).clear();
	}
	int const iTeams =  (int)teamSeqCache(ALL).size();
	for (int i = NUM_STATUS_CACHES; i < NUM_CACHES; i++)
	{
		AgentSeqCache eCacheID = (AgentSeqCache)i;
		for (int j = 0; j < iTeams; j++)
		{
			TeamTypes eTeam = (TeamTypes)i;
			memberSeqCache(eCacheID, eTeam).clear();
			teamPerTeamSeqCache(eCacheID, eTeam).clear();
		}
	}
	for (int i = 0; i < iTeams; i++)
	{
		CvTeamAI* pTeam = teamSeqCache(ALL)[i];
		if (pTeam->isBarbarian() || !pTeam->isEverAlive())
			continue;
		teamSeqCache(CIV_EVER_ALIVE).push_back(pTeam);
		if (!pTeam->isAlive())
			continue;
		teamSeqCache(CIV_ALIVE).push_back(pTeam);
		if (!pTeam->isMinorCiv())
			teamSeqCache(MAJOR_ALIVE).push_back(pTeam);
	}
	int const iPlayers = (int)playerSeqCache(ALL).size();
	for (int i = 0; i < iPlayers; i++)
	{
		CvPlayerAI* pPlayer = playerSeqCache(ALL)[i];
		TeamTypes eTeam = pPlayer->getTeam();
		memberSeqCache(MEMBER, eTeam).push_back(pPlayer);
		if (!pPlayer->isEverAlive())
			continue;
		if (!pPlayer->isBarbarian())
			playerSeqCache(CIV_EVER_ALIVE).push_back(pPlayer);
		if (!pPlayer->isAlive())
			continue;
		memberSeqCache(MEMBER_ALIVE, eTeam).push_back(pPlayer);
		if (pPlayer->isBarbarian())
			continue;
		playerSeqCache(CIV_ALIVE).push_back(pPlayer);
		if(!pPlayer->isMinorCiv())
			playerSeqCache(MAJOR_ALIVE).push_back(pPlayer);
	}
}

namespace // These functions aren't frequently called; don't need to be fast.
{
	template<class AgentType>
	void eraseFromVector(std::vector<AgentType*>& v, int iAgentID, bool bAssertSuccess = true)
	{
		for (std::vector<AgentType*>::iterator it = v.begin(); it != v.end(); ++it)
		{
			if ((*it)->getID() == iAgentID)
			{
				v.erase(it);
				return;
			}
		}
		FAssert(!bAssertSuccess);
	}

	template<class AgentType>
	void insertIntoVector(std::vector<AgentType*>& v, AgentType* pAgent, bool bAssertSuccess = true)
	{
		int iAgentID = pAgent->getID();
		std::vector<AgentType*>::iterator pos;
		for (pos = v.begin(); pos != v.end(); ++pos)
		{
			int const iPosID = (*pos)->getID();
			FAssert (!bAssertSuccess || iPosID != iAgentID);
			if (iPosID > iAgentID)
				break;
		}
		v.insert(pos, pAgent);
	}
}


void CvAgents::playerDefeated(PlayerTypes eDeadPlayer)
{
	eraseFromVector(playerSeqCache(CIV_ALIVE), eDeadPlayer);
	if (!GET_PLAYER(eDeadPlayer).isMinorCiv())
		eraseFromVector(playerSeqCache(MAJOR_ALIVE), eDeadPlayer);
	TeamTypes eTeam = TEAMID(eDeadPlayer);
	eraseFromVector(memberSeqCache(MEMBER_ALIVE, eTeam), eDeadPlayer);
	TeamTypes eMasterTeam = GET_TEAM(eTeam).getMasterTeam();
	if (eMasterTeam != eTeam)
		eraseFromVector(memberSeqCache(VASSAL_ALIVE, eTeam), eDeadPlayer);
	if (!GET_TEAM(eTeam).isAlive())
	{
		eraseFromVector(teamSeqCache(CIV_ALIVE), eTeam);
		if (GET_TEAM(eTeam).isMinorCiv())
			eraseFromVector(teamSeqCache(MAJOR_ALIVE), eTeam, false);
		if (eMasterTeam != eTeam)
			eraseFromVector(teamPerTeamSeqCache(VASSAL_ALIVE, eMasterTeam), eTeam, false);
	}
}


void CvAgents::colonyCreated(PlayerTypes eNewPlayer)
{
	CvPlayerAI* pPlayer = &GET_PLAYER(eNewPlayer);
	CvTeamAI* pTeam = &GET_TEAM(eNewPlayer);
	insertIntoVector(teamSeqCache(CIV_EVER_ALIVE), pTeam);
	insertIntoVector(teamSeqCache(CIV_ALIVE), pTeam);
	insertIntoVector(teamSeqCache(MAJOR_ALIVE), pTeam);
	insertIntoVector(playerSeqCache(CIV_EVER_ALIVE), pPlayer);
	insertIntoVector(playerSeqCache(CIV_ALIVE), pPlayer);
	insertIntoVector(playerSeqCache(MAJOR_ALIVE), pPlayer);
	PlayerVector const& members = memberSeqCache(MEMBER, pTeam->getID());
	FAssert(std::find(members.begin(), members.end(), pPlayer) != members.end());
	PlayerVector& membersAlive = memberSeqCache(MEMBER_ALIVE, pTeam->getID());
	insertIntoVector(membersAlive, pPlayer);
}


void CvAgents::updateVassal(TeamTypes eVassal, TeamTypes eMaster, bool bVassal)
{
	FASSERT_BOUNDS(0, (int)playerSeqCache(ALL).size(), eMaster, "CvAgents::updateVassal");
	TeamVector& vassalTeams = teamPerTeamSeqCache(VASSAL_ALIVE, eMaster);
	PlayerVector& vassalPlayers = memberSeqCache(VASSAL_ALIVE, eMaster);
	for (size_t i = 0; i < memberSeqCache(MEMBER, eVassal).size(); i++)
	{
		CvPlayerAI* pMember = memberSeqCache(MEMBER, eVassal)[i];
		if (bVassal)
			insertIntoVector(vassalPlayers, pMember);
		else eraseFromVector(vassalPlayers, pMember->getID());
	}
	CvTeamAI* pVassalTeam = &GET_TEAM(eVassal);
	if (bVassal)
		insertIntoVector(vassalTeams, pVassalTeam);
	else eraseFromVector(vassalTeams, pVassalTeam->getID());
}


#define NO_GENERIC_IMPLEMENTATION() \
	FAssertMsg(false, "No generic implementation"); \
	return NULL

template<class AgentType>
std::vector<AgentType*> const* CvAgents::getAgentSeqCache(AgentSeqCache eCacheID) const
{
	NO_GENERIC_IMPLEMENTATION();
}

template<class AgentType>
std::vector<AgentType*> const* CvAgents::getPerTeamSeqCache(AgentSeqCache eCacheID, TeamTypes) const
{
	NO_GENERIC_IMPLEMENTATION();
}

template<class AgentType>
std::vector<AgentType*> const* CvAgents::getNoAgents() const
{
	NO_GENERIC_IMPLEMENTATION();
}
