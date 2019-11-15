// advc.agent: New file; see comment in header file.

#include "CvGameCoreDLL.h"
#include "CvAgents.h"
#include "AgentIterator.h"
#include "CvAI.h"

CvAgents::CvAgents(int iMaxPlayers, int iMaxTeams)
{
	// Tbd.: Instead create the CvPlayerAI and CvTeamAI objects here
	for (int i = 0; i < iMaxTeams; i++)
	{
		CvTeamAI* pTeam = &GET_TEAM((TeamTypes)i);
		m_allTeams.push_back(pTeam);
	}
	for (int i = 0; i < iMaxPlayers; i++)
	{
		CvPlayerAI* pPlayer = &GET_PLAYER((PlayerTypes)i);
		m_allPlayers.push_back(pPlayer);
	}
	m_playersPerTeam.resize(iMaxPlayers);
	m_playersAlivePerTeam.resize(iMaxPlayers);
	m_vassalTeamsAlivePerTeam.resize(iMaxPlayers);
	m_vassalPlayersAlivePerTeam.resize(iMaxPlayers);
	PlayerIter::m_pAgents = this;
	TeamIter::m_pAgents = this;
}

// <advc.tmp>
//#define AGENT_ITERATOR_TEST
#ifdef AGENT_ITERATOR_TEST
#include "TSCProfiler.h"
namespace
{
	int speedTest()
	{
		int r = 0;
		{
			TSC_PROFILE("MAJOR_CIV TeamIter");
			for (TeamIter it(MAJOR_CIV); it.hasNext(); ++it)
			{
				CvTeam& kTeam = *it;
				r += kTeam.getID();
			}
		}
		{
			TSC_PROFILE("MAJOR_CIV MAX_CIV_TEAMS");
			for (int i = 0; i < MAX_CIV_TEAMS; i++)
			{
				CvTeam& kTeam = GET_TEAM((TeamTypes)i);
				if (!kTeam.isAlive() || kTeam.isMinorCiv())
					continue;
				r += kTeam.getID();
			}
		}
		{
			TSC_PROFILE("MAJOR_CIV vector<CvTeamAI*>");
			CvAgents const& kAgents = GC.getAgents();
			for (int i = 0; i < kAgents.majorTeams(); i++)
			{
				CvTeam& kTeam = kAgents.majorTeam(i);
				r += kTeam.getID();
			}
		}
		{
			TSC_PROFILE("FREE_MAJOR TeamIter");
			for (TeamIter it(FREE_MAJOR); it.hasNext(); ++it)
			{
				CvTeam& kTeam = *it;
				r += kTeam.getID();
			}
		}
		{
			TSC_PROFILE("FREE_MAJOR MAX_CIV_TEAMS");
			for (int i = 0; i < MAX_CIV_TEAMS; i++)
			{
				CvTeam& kTeam = GET_TEAM((TeamTypes)i);
				if (!kTeam.isAlive() || kTeam.isMinorCiv() || kTeam.isAVassal())
					continue;
				r += kTeam.getID();
			}
		}
		{
			TSC_PROFILE("FREE_MAJOR vector<CvTeamAI*>");
			CvAgents const& kAgents = GC.getAgents();
			for (int i = 0; i < kAgents.majorTeams(); i++)
			{
				CvTeam& kTeam = kAgents.majorTeam(i);
				if (kTeam.isAVassal())
					continue;
				r += kTeam.getID();
			}
		}
		return r;
	}
}
#endif
// </advc.tmp>

void CvAgents::gameStart(bool bFromSaveGame)
{
	m_bNoBarbarians = GC.getGame().isOption(GAMEOPTION_NO_BARBARIANS);
	updateAllCachedSequences();
	// <advc.tmp>
	#ifdef AGENT_ITERATOR_TEST
	if (bFromSaveGame)
	{
		int iNonsense = 0;
		for (int i = 0; i < 100; i++)
			iNonsense += speedTest();
		if (iNonsense == -1) // Just to make sure that the whole thing doesn't somehow get optimized away
			updateAllCachedSequences();
	}
	#endif
	// </advc.tmp>
}

void CvAgents::updateAllCachedSequences()
{
	m_civPlayersEverAlive.clear();
	m_civPlayersAlive.clear();
	m_majorPlayersAlive.clear();
	m_civTeamsEverAlive.clear();
	m_civTeamsAlive.clear();
	m_majorTeamsAlive.clear();
	for (size_t i = 0; i < m_allTeams.size(); i++)
	{
		m_playersPerTeam[i].clear();
		m_playersAlivePerTeam[i].clear();
		m_vassalPlayersAlivePerTeam[i].clear();
		m_vassalTeamsAlivePerTeam[i].clear();
	}

	for (size_t i = 0; i < m_allTeams.size(); i++)
	{
		CvTeamAI* pTeam = m_allTeams[i];
		if (pTeam->isBarbarian() || !pTeam->isEverAlive())
			continue;
		m_civTeamsEverAlive.push_back(pTeam);
		if (!pTeam->isAlive())
			continue;
		m_civTeamsAlive.push_back(pTeam);
		if (!pTeam->isMinorCiv())
			m_majorTeamsAlive.push_back(pTeam);
	}
	for (size_t i = 0; i < m_allPlayers.size(); i++)
	{
		CvPlayerAI* pPlayer = m_allPlayers[i];
		TeamTypes eTeam = pPlayer->getTeam();
		m_playersPerTeam[eTeam].push_back(pPlayer);
		if (!pPlayer->isEverAlive())
			continue;
		if (!pPlayer->isBarbarian())
			m_civPlayersEverAlive.push_back(pPlayer);
		if (!pPlayer->isAlive())
			continue;
		m_playersAlivePerTeam[eTeam].push_back(pPlayer);
		if(!pPlayer->isMinorCiv())
			m_majorPlayersAlive.push_back(pPlayer);
	}
}

namespace
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
	eraseFromVector(m_civPlayersAlive, eDeadPlayer);
	if (!GET_PLAYER(eDeadPlayer).isMinorCiv())
		eraseFromVector(m_majorPlayersAlive, eDeadPlayer);
	TeamTypes eTeam = TEAMID(eDeadPlayer);
	eraseFromVector(m_playersAlivePerTeam[eTeam], eDeadPlayer);
	TeamTypes eMasterTeam = GET_TEAM(eTeam).getMasterTeam();
	if (eMasterTeam != eTeam)
		eraseFromVector(m_vassalPlayersAlivePerTeam[eTeam], eDeadPlayer);
	if (!GET_TEAM(eTeam).isAlive())
	{
		eraseFromVector(m_civTeamsAlive, eTeam);
		if (GET_TEAM(eTeam).isMinorCiv())
			eraseFromVector(m_majorTeamsAlive, eTeam, false);
		if (eMasterTeam != eTeam)
			eraseFromVector(m_vassalTeamsAlivePerTeam[eMasterTeam], eTeam, false);
	}
}


void CvAgents::colonyCreated(PlayerTypes eNewPlayer)
{
	CvPlayerAI* pPlayer = &GET_PLAYER(eNewPlayer);
	CvTeamAI* pTeam = &GET_TEAM(eNewPlayer);
	insertIntoVector(m_civTeamsEverAlive, pTeam);
	insertIntoVector(m_civTeamsAlive, pTeam);
	insertIntoVector(m_majorTeamsAlive, pTeam);
	insertIntoVector(m_civPlayersEverAlive, pPlayer);
	insertIntoVector(m_civPlayersAlive, pPlayer);
	insertIntoVector(m_majorPlayersAlive, pPlayer);
	PlayerVector const& members = m_playersPerTeam[pTeam->getID()];
	FAssert(std::find(members.begin(), members.end(), pPlayer) != members.end());
	PlayerVector& membersAlive = m_playersAlivePerTeam[pTeam->getID()];
	insertIntoVector(membersAlive, pPlayer);
}


void CvAgents::updateVassal(TeamTypes eVassal, TeamTypes eMaster, bool bVassal)
{
	FASSERT_BOUNDS(0, (int)m_allTeams.size(), eMaster, "CvAgents::updateVassal");
	TeamVector& vassalTeams = m_vassalTeamsAlivePerTeam[eMaster];
	PlayerVector& vassalPlayers = m_vassalPlayersAlivePerTeam[eMaster];
	for (size_t i = 0; i < m_playersPerTeam[eVassal].size(); i++)
	{
		CvPlayerAI* pMember = m_playersPerTeam[eVassal][i];
		if (bVassal)
			insertIntoVector(vassalPlayers, pMember);
		else eraseFromVector(vassalPlayers, pMember->getID());
	}
	CvTeamAI* pVassalTeam = &GET_TEAM(eVassal);
	if (bVassal)
		insertIntoVector(vassalTeams, pVassalTeam);
	else eraseFromVector(vassalTeams, pVassalTeam->getID());
}


template<class AgentType> AgentGenerator<AgentType>* CvAgents
::getAgentGenerator(AgentStatusPredicate eStatus, AgentRelationPredicate eRelation,
	TeamTypes eTeam, PlayerTypes ePlayer)
{
	std::vector<AgentType*> const* pCachedSeq = NULL;
	bool bIncludeBarbarians = false;
	bool bFilter = false;
	switch(eRelation)
	{
	case ANY_AGENT_RELATION:
		switch(eStatus)
		{
			case ANY_AGENT_STATUS:
				pCachedSeq = getAllAgents<AgentType>();
				bIncludeBarbarians = true;
				break;
			case EVER_ALIVE:
				pCachedSeq = getCivAgentsEverAlive<AgentType>();
				bIncludeBarbarians = true;
				break;
			case ALIVE:
				pCachedSeq = getCivAgentsAlive<AgentType>();
				bIncludeBarbarians = true;
				break;
			case NON_BARBARIAN:
				pCachedSeq = getCivAgentsAlive<AgentType>();
				break;
			case MAJOR_CIV:
				pCachedSeq = getMajorAgentsAlive<AgentType>();
				break;
			default:
				pCachedSeq = getMajorAgentsAlive<AgentType>();
				bFilter = true;
		}
		break;

	case SAME_TEAM_AS:
		switch(eStatus)
		{
		case ANY_AGENT_STATUS:
			pCachedSeq = getAgentsPerTeam<AgentType>(eTeam);
			break;
		case EVER_ALIVE:
			pCachedSeq = getAgentsPerTeam<AgentType>(eTeam);
			bFilter = true;
		case ALIVE:
			pCachedSeq = getAgentsAlivePerTeam<AgentType>(eTeam);
			break;
		default:
			pCachedSeq = getAgentsAlivePerTeam<AgentType>(eTeam);
			bFilter = true;
		}
		break;

	case VASSAL_OF:
		switch(eStatus)
		{
		case ANY_AGENT_STATUS:
			pCachedSeq = getAllAgents<AgentType>();
			bFilter = true;
			break;
		case EVER_ALIVE:
			pCachedSeq = getCivAgentsEverAlive<AgentType>();
			bFilter = true;
			eStatus = ANY_AGENT_STATUS; // No need for generator to check status
			break;
		case ALIVE:
			pCachedSeq = getVassalAgentsAlivePerTeam<AgentType>(eTeam);
			eStatus = ANY_AGENT_STATUS;
			break;
		default:
			pCachedSeq = getVassalAgentsAlivePerTeam<AgentType>(eTeam);
			bFilter = true;
		}
		break;

	case POTENTIAL_ENEMY_OF:
	case KNOWN_TO:
	case KNOWN_POTENTIAL_ENEMY_OF:
	case ENEMY_OF: // Applicable to minor civs
		bFilter = true;
		bIncludeBarbarians = includeBarbarians(eStatus, eRelation);
		if (eStatus >= MAJOR_CIV)
		{
			pCachedSeq = getMajorAgentsAlive<AgentType>();
			if (eStatus == MAJOR_CIV)
				eStatus = ANY_AGENT_STATUS;
		}
		else
		{
			pCachedSeq = getCivAgentsAlive<AgentType>();
			if (eStatus == ALIVE)
				eStatus = ANY_AGENT_STATUS;
		}
		break;

	default:
		bFilter = true;
		bIncludeBarbarians = includeBarbarians(eStatus, eRelation);
		pCachedSeq = getMajorAgentsAlive<AgentType>();
		if (eStatus == MAJOR_CIV)
			eStatus = ANY_AGENT_STATUS;
	}
	return getAgentGenerator<AgentType>(bIncludeBarbarians && !m_bNoBarbarians, bFilter,
			pCachedSeq, eStatus, eRelation, eTeam, ePlayer);
}

// Explicit instantiations; needed by the linker.
template AgentGenerator<CvPlayerAI>* CvAgents::getAgentGenerator<CvPlayerAI>(
	AgentStatusPredicate, AgentRelationPredicate, TeamTypes, PlayerTypes);
template AgentGenerator<CvTeamAI>* CvAgents::getAgentGenerator<CvTeamAI>(
	AgentStatusPredicate, AgentRelationPredicate, TeamTypes, PlayerTypes);

bool CvAgents::includeBarbarians(AgentStatusPredicate eStatus, AgentRelationPredicate eRelation)
{
	return (eStatus < NON_BARBARIAN && eRelation != VASSAL_OF &&
			eRelation != NOT_A_RIVAL_OF && eRelation != KNOWN_TO);
}

template<>
AgentGenerator<CvPlayerAI>* CvAgents::getAgentGenerator<CvPlayerAI>(
	bool bFilter, bool bInclBarbarians, PlayerVector const* pCachedSeq,
	AgentStatusPredicate eStatus, AgentRelationPredicate eRelation,
	TeamTypes eTeam, PlayerTypes ePlayer) const
{
	if (bFilter)
	{
		FilteringAgentGenerator<CvPlayerAI>& kFiltering = m_filteringPlayerGen.getNext();
		kFiltering.init(pCachedSeq, bInclBarbarians, eStatus, eRelation, eTeam, ePlayer);
		return &kFiltering;
	}
	if (bInclBarbarians)
	{
		BarbarianAppendingAgentGenerator<CvPlayerAI>& kBarbAppending = m_barbAppendingPlayerGen.getNext();
		kBarbAppending.init(pCachedSeq);
		return &kBarbAppending;
	}
	FullyCachedAgentGenerator<CvPlayerAI>& kFullyCached = m_fullyCachedPlayerGen.getNext();
	kFullyCached.init(pCachedSeq);
	return &kFullyCached;
}

template<>
AgentGenerator<CvTeamAI>* CvAgents::getAgentGenerator<CvTeamAI>(
	bool bFilter, bool bInclBarbarians, TeamVector const* pCachedSeq,
	AgentStatusPredicate eStatus, AgentRelationPredicate eRelation,
	TeamTypes eTeam, PlayerTypes ePlayer) const
{
	if (bFilter)
	{
		FilteringAgentGenerator<CvTeamAI>& kFiltering = m_filteringTeamGen.getNext();
		kFiltering.init(pCachedSeq, bInclBarbarians, eStatus, eRelation, eTeam, ePlayer);
		return &kFiltering;
	}
	if (bInclBarbarians)
	{
		BarbarianAppendingAgentGenerator<CvTeamAI>& kBarbAppending = m_barbAppendingTeamGen.getNext();
		kBarbAppending.init(pCachedSeq);
		return &kBarbAppending;
	}
	FullyCachedAgentGenerator<CvTeamAI>& kFullyCached = m_fullyCachedTeamGen.getNext();
	kFullyCached.init(pCachedSeq);
	return &kFullyCached;
}

#define NO_GENERIC_IMPLEMENTATION() \
	FAssertMsg(false, "No generic implementation"); \
	return NULL

template<class AgentType>
AgentGenerator<AgentType>* CvAgents::getAgentGenerator(bool,bool,std::vector<AgentType*>const*,AgentStatusPredicate,AgentRelationPredicate,TeamTypes,PlayerTypes) const
{
	NO_GENERIC_IMPLEMENTATION();
}


template<>
std::vector<CvTeamAI*> const* CvAgents::getAgentsPerTeam<CvTeamAI>(TeamTypes) const
{
	FAssertMsg(false, "Cannot generate 'teams per team'");
	return NULL;
}

template<>
std::vector<CvTeamAI*> const* CvAgents::getAgentsAlivePerTeam<CvTeamAI>(TeamTypes) const
{
	FAssertMsg(false, "Cannot generate 'teams alive per team'");
	return NULL;
}

template<class AgentType>
std::vector<AgentType*> const* CvAgents::getAllAgents() const
{
	NO_GENERIC_IMPLEMENTATION();
}

template<class AgentType>
std::vector<AgentType*> const* CvAgents::getCivAgentsEverAlive() const
{
	NO_GENERIC_IMPLEMENTATION();
}

template<class AgentType>
std::vector<AgentType*> const* CvAgents::getCivAgentsAlive() const
{
	NO_GENERIC_IMPLEMENTATION();
}

template<class AgentType>
std::vector<AgentType*> const* CvAgents::getMajorAgentsAlive() const
{
	NO_GENERIC_IMPLEMENTATION();
}

template<class AgentType>
std::vector<AgentType*> const* CvAgents::getVassalAgentsAlivePerTeam(TeamTypes) const
{
	NO_GENERIC_IMPLEMENTATION();
}

template<class AgentType>
std::vector<AgentType*> const* CvAgents::getAgentsPerTeam(TeamTypes) const
{
	NO_GENERIC_IMPLEMENTATION();
}

template<class AgentType>
std::vector<AgentType*> const* CvAgents::getAgentsAlivePerTeam(TeamTypes) const
{
	NO_GENERIC_IMPLEMENTATION();
}

template<class AgentType>
std::vector<AgentType*> const* CvAgents::getNoAgents() const
{
	NO_GENERIC_IMPLEMENTATION();
}
