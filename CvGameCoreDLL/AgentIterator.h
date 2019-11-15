#pragma once

#ifndef AGENT_ITERATOR_H
#define AGENT_ITERATOR_H

// advc.agent: New file. Iterators over sequences of CvTeam or CvPlayer objects.

#include "CvAgents.h"

class CvTeam;
class CvPlayer;

template<class AgentType>
class AgentIterator
{
public:
	AgentIterator(AgentStatusPredicate eStatus = ALIVE)
	{	
		m_pGenerator = m_pAgents->getAgentGenerator<AgentType>(eStatus, ANY_AGENT_RELATION, NO_TEAM);
	}
	AgentIterator(AgentStatusPredicate eStatus, TeamTypes eTeam)
	{
		m_pGenerator = m_pAgents->getAgentGenerator(eStatus, SAME_TEAM_AS, eTeam);
	}
	AgentIterator(TeamTypes eTeam)
	{
		m_pGenerator = m_pAgents->getAgentGenerator(ALIVE, SAME_TEAM_AS, eTeam);
	}
	AgentIterator(AgentRelationPredicate eRelation, TeamTypes eTeam)
	{
		m_pGenerator = m_pAgents->getAgentGenerator(ALIVE, eRelation, eTeam);
	}
	AgentIterator(AgentRelationPredicate eRelation, CvTeam const& kTeam)
	{
		m_pGenerator = m_pAgents->getAgentGenerator(ALIVE, eRelation, kTeam.getID());
	}
	AgentIterator(AgentRelationPredicate eRelation, PlayerTypes ePlayer)
	{
		m_pGenerator = m_pAgents->getAgentGenerator(ALIVE, eRelation, NO_TEAM, ePlayer);
	}
	AgentIterator(AgentRelationPredicate eRelation, CvPlayer const& kPlayer)
	{
		m_pGenerator = m_pAgents->getAgentGenerator(ALIVE, eRelation, NO_TEAM, kPlayer.getID());
	}
	AgentIterator(AgentStatusPredicate eStatus, AgentRelationPredicate eRelation, CvTeam const& kTeam)
	{
		m_pGenerator = m_pAgents->getAgentGenerator(eStatus, eRelation, kTeam.getID());
	}
	AgentIterator(AgentStatusPredicate eStatus, AgentRelationPredicate eRelation, PlayerTypes ePlayer)
	{
		m_pGenerator = m_pAgents->getAgentGenerator(eStatus, eRelation, NO_TEAN, ePlayer);
	}
	AgentIterator(AgentStatusPredicate eStatus, AgentRelationPredicate eRelation, CvPlayer const& kPlayer)
	{
		m_pGenerator = m_pAgents->getAgentGenerator(eStatus, eRelation, NO_TEAN, kPlayer.getID());
	}

	__forceinline bool hasNext() const
	{
		return m_pGenerator->hasNext();
	}
	__forceinline AgentIterator& operator++()
	{
		m_pGenerator->generateNext();
		return *this;
	}
	__forceinline AgentType& operator*()
	{
		return m_pGenerator->get();
	}

private:
	AgentGenerator<AgentType>* m_pGenerator;
	friend CvAgents::CvAgents(int,int);
	static CvAgents* m_pAgents;
};

template<class AgentType>
CvAgents* AgentIterator<AgentType>::m_pAgents = NULL;

typedef AgentIterator<CvPlayerAI> PlayerIter;
typedef AgentIterator<CvTeamAI> TeamIter;

#endif
