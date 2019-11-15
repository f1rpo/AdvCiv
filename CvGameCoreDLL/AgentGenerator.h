#pragma once

#ifndef AGENT_GENERATOR_H
#define AGENT_GENERATOR_H

/*  advc.agent: New file. Hierarchy of generators of
	agent (CvTeam, CvPlayer) sequences. So that AgentIterator can obtain
	the most efficient generator for the given predicates. The generators
	are accessed through CvAgents::getAgentGenerator. */

#include "AgentPredicates.h"


template<class AgentType>
class AgentGenerator
{
public:
	/*  Reusable object.
		Derived classes that call this function need to carry out their own initialization
		first so that generateNext is called on a fully initialized object. */
	virtual void init(std::vector<AgentType*> const* pCachedSeq)
	{
		m_iPos = 0;
		m_pNext = NULL;
		m_pCache = pCachedSeq;

		generateNext();
	}

	__forceinline bool hasNext() const
	{
		return (m_pNext != NULL);
	}

	__forceinline AgentType& get() const
	{
		return *m_pNext;
	}

	virtual void generateNext() = 0;

protected:
	std::vector<AgentType*> const* m_pCache;
	int m_iPos;
	AgentType* m_pNext;
};

/*  The constructors are only to be called from CvAgent::GeneratorPool
	(friend declarations to that effect are difficult to write) */

template<class AgentType>
class FullyCachedAgentGenerator : public AgentGenerator<AgentType>
{
public:
	void generateNext();
	FullyCachedAgentGenerator() {}
};


template<class AgentType>
class BarbarianAppendingAgentGenerator : public AgentGenerator<AgentType>
{
public:
	void generateNext();
	BarbarianAppendingAgentGenerator() {}
};


template<class AgentType>
class FilteringAgentGenerator : public AgentGenerator<AgentType>
{
public:
	void init(std::vector<AgentType*> const* pCachedSeq, bool bInclBarbarians,
		AgentStatusPredicate eStatus, AgentRelationPredicate eRelation,
		TeamTypes eTeam, PlayerTypes ePlayer);
	void generateNext();
	FilteringAgentGenerator() {}

protected:
	std::vector<AgentType*> const* m_pCache;
	int m_iPos;
	AgentType* m_pNext;
	bool m_bIncludeBarbarians;

	bool passFilters(AgentType const& kAgent) const;
	// The remaining private data should only be read in passFilters
	AgentStatusPredicate m_eStatus;
	AgentRelationPredicate m_eRelation;
	TeamTypes m_eTeam;
	PlayerTypes m_ePlayer;
};

#endif
