// advc.agent: New file; see comment in header file.

#include "CvGameCoreDLL.h"
#include "AgentGenerator.h"
#include "CvPlayerAI.h"
#include "CvTeamAI.h"


namespace
{
	template<class AgentType>
	AgentType& getBarbarianAgent()
	{
		FAssertMsg(false, "No generic implementation");
		return GET_PLAYER(BARBARIAN_PLAYER);
	}
	template<>
	CvPlayerAI& getBarbarianAgent<CvPlayerAI>()
	{
		return GET_PLAYER(BARBARIAN_PLAYER);
	}
	template<>
	CvTeamAI& getBarbarianAgent<CvTeamAI>()
	{
		return GET_TEAM(BARBARIAN_TEAM);
	}
	TeamTypes getTeam(CvPlayerAI const& kPlayer)
	{
		return kPlayer.getTeam();
	}
	TeamTypes getTeam(CvTeamAI const& kTeam)
	{
		return kTeam.getID();
	}
}

template<class AgentType>
void FullyCachedAgentGenerator<AgentType>::generateNext()
{
	if (m_iPos < (int)m_pCache->size())
		m_pNext = (*m_pCache)[m_iPos];
	else m_pNext = NULL;
	m_iPos++;
}

template<class AgentType>
void BarbarianAppendingAgentGenerator<AgentType>::generateNext()
{
	if (m_iPos == (int)m_pCache->size())
		m_pNext = &getBarbarianAgent<AgentType>();
	else if (m_iPos < (int)m_pCache->size())
		m_pNext = (*m_pCache)[m_iPos];
	else m_pNext = NULL;
	m_iPos++;
}

template<class AgentType>
void FilteringAgentGenerator<AgentType>::
init(std::vector<AgentType*> const* pCachedSeq, bool bInclBarbarians,
	AgentStatusPredicate eStatus, AgentRelationPredicate eRelation,
	TeamTypes eTeam, PlayerTypes ePlayer)
{
	m_bIncludeBarbarians = bInclBarbarians;
	m_eStatus = eStatus;
	m_eRelation = eRelation;
	m_eTeam = eTeam;
	m_ePlayer = ePlayer;
	if (m_eTeam == NO_TEAM && m_ePlayer != NO_PLAYER)
		m_eTeam = GET_PLAYER(m_ePlayer).getTeam();
	AgentGenerator<AgentType>::init(pCachedSeq);
}

template<class AgentType>
void FilteringAgentGenerator<AgentType>::generateNext()
{
	if (!m_bIncludeBarbarians)
	{
		if (m_iPos < (int)m_pCache->size())
			m_pNext = (*m_pCache)[m_iPos];
		else m_pNext = NULL;
	}
	else
	{
		if (m_iPos == (int)m_pCache->size())
			m_pNext = &getBarbarianAgent<AgentType>();
		else if (m_iPos < (int)m_pCache->size())
			m_pNext = (*m_pCache)[m_iPos];
		else m_pNext = NULL;
	}
	if (m_pNext != NULL && !passFilters(*m_pNext))
		m_pNext = NULL;
	m_iPos++;
}


template<class AgentType>
bool FilteringAgentGenerator<AgentType>::passFilters(AgentType const& kAgent) const
{
	switch(m_eStatus)
	{
	case ANY_AGENT_STATUS:
	case NON_BARBARIAN: // Handled by m_bInclBarbarians
		break;
	case EVER_ALIVE:
		if (!kAgent.isEverAlive())
			return false;
		break;
	case ALIVE:
		if (!kAgent.isAlive())
			return false;
		break;
	case MAJOR_CIV:
		if (kAgent.isMinorCiv() || !kAgent.isAlive())
			return false;
		break;
	case VASSAL:
		if (!kAgent.isAVassal() || !kAgent.isAlive())
			return false;
		break;
	case FREE_MAJOR:
		if (kAgent.isAVassal() || !kAgent.isAlive() || kAgent.isMinorCiv())
			return false;
		break;
	case FREE_MAJOR_AI:
		if (kAgent.isAVassal() || kAgent.isHuman() ||
				!kAgent.isAlive() || kAgent.isMinorCiv())
			return false;
		break;
	case HUMAN:
		if (!kAgent.isHuman() || !kAgent.isAlive())
			return false;
		break;
	default:
		FAssertMsg(false, "Unknown agent status type");
		return false;
	}
	if (m_eTeam == NO_TEAM)
		return true;
	switch(m_eRelation)
	{
	case ANY_AGENT_RELATION:
		return true;
	case SAME_TEAM_AS:
		return getTeam(kAgent) == m_eTeam;
	case NOT_SAME_TEAM_AS:
		return getTeam(kAgent) != m_eTeam;
	case VASSAL_OF:
		return GET_TEAM(getTeam(kAgent)).isVassal(m_eTeam);
	case NOT_A_RIVAL_OF:
		return (getTeam(kAgent) == m_eTeam || kAgent.getMasterTeam() == GET_TEAM(m_eTeam).getMasterTeam());
	case POTENTIAL_ENEMY_OF:
		return (getTeam(kAgent) != m_eTeam && kAgent.getMasterTeam() != GET_TEAM(m_eTeam).getMasterTeam());
	case KNOWN_TO:
		return GET_TEAM(getTeam(kAgent)).isHasMet(m_eTeam);
	case KNOWN_POTENTIAL_ENEMY_OF:
		return (GET_TEAM(getTeam(kAgent)).isHasMet(m_eTeam) && getTeam(kAgent) != m_eTeam &&
			kAgent.getMasterTeam() != GET_TEAM(m_eTeam).getMasterTeam());
	case ENEMY_OF:
			return GET_TEAM(getTeam(kAgent)).isAtWar(m_eTeam);
	default:
		FAssertMsg(false, "Unknown agent relation type");
		return false;
	}
}

// All possible instantiations; needed by the linker.
template class FullyCachedAgentGenerator<CvPlayerAI>;
template class BarbarianAppendingAgentGenerator<CvPlayerAI>;
template class FilteringAgentGenerator<CvPlayerAI>;
template class FullyCachedAgentGenerator<CvTeamAI>;
template class BarbarianAppendingAgentGenerator<CvTeamAI>;
template class FilteringAgentGenerator<CvTeamAI>;
