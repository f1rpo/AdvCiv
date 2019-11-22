#pragma once

#ifndef AGENT_ITERATOR_H
#define AGENT_ITERATOR_H

/*  advc.agent: New file. Iterators over sequences of CvTeam or CvPlayer objects.
	The concrete iterator classes are defined at the end of the file. */

#include "AgentPredicates.h"
#include "CvAgents.h"

class CvTeam;
class CvPlayer;


/*  Non-generic base class that allows all instantiations of AgentIterator
	to share static variables */
class AgentIteratorBase
{
public:
	static void setAgentsCache(CvAgents const* pAgents);
protected:
	static CvAgents const* m_pAgents;
};

/*  This helper base class gets explicitly instantiated for all combinations
	of parameters in AgentIterator.cpp. This allows me to implement the
	passFilters function outside of the header file. passFilters requires
	CvPlayerAI.h and CvTeamAI.h; I don't want to include these in AgentIterator.h. */
template<class AgentType, AgentStatusPredicate eSTATUS, AgentRelationPredicate eRELATION>
class ExplicitAgentIterator : protected AgentIteratorBase
{
protected:
	inline ExplicitAgentIterator(TeamTypes eTeam = NO_TEAM) : m_eTeam(eTeam) {}
	bool passFilters(AgentType const& kAgent) const;
	TeamTypes m_eTeam;

	// These two variables are needed in passFilters:

	/*  Cache that is either an exact match for the required predicates or
		(Barbarians to be added) a subset. (Named after the LaTeX command /subseteq.) */
	static CvAgents::AgentSeqCache const eCACHE_SUBSETEQ = (
			(eSTATUS == ANY_AGENT_STATUS && eRELATION == ANY_AGENT_RELATION) ?
			CvAgents::ALL :
			(eSTATUS == ANY_AGENT_STATUS && eRELATION == MEMBER_OF) ?
			CvAgents::MEMBER :
			(eSTATUS == EVER_ALIVE && eRELATION == ANY_AGENT_RELATION) ?
			CvAgents::CIV_EVER_ALIVE :
			((eSTATUS == ALIVE || eSTATUS == NON_BARB) && eRELATION == ANY_AGENT_RELATION) ?
			CvAgents::CIV_ALIVE :
			(eSTATUS == ALIVE && eRELATION == MEMBER_OF) ?
			CvAgents::MEMBER_ALIVE :
			(eSTATUS == MAJOR_CIV && eRELATION == ANY_AGENT_RELATION) ?
			CvAgents::MAJOR_ALIVE :
			(eSTATUS >= ALIVE && eSTATUS <= MAJOR_CIV && eRELATION == VASSAL_OF) ?
			CvAgents::VASSAL_ALIVE :
			CvAgents::NO_CACHE);
	/*  Cache that is a super set of the required agents. Will have to filter out those
		that don't match the predicates. */
	static CvAgents::AgentSeqCache const eCACHE_SUPER = (
			eRELATION == ANY_AGENT_RELATION ?
			CvAgents::MAJOR_ALIVE :
			eRELATION == MEMBER_OF ?
				(eSTATUS == EVER_ALIVE ? CvAgents::MEMBER : CvAgents::MEMBER_ALIVE) :
			eRELATION == VASSAL_OF ?
				(eSTATUS == ANY_AGENT_STATUS ? CvAgents::ALL :
				(eSTATUS == EVER_ALIVE ? CvAgents::CIV_EVER_ALIVE : CvAgents::VASSAL_ALIVE)) :
			// These four can apply to minor civs
			(eRELATION == POTENTIAL_ENEMY_OF || eRELATION == KNOWN_TO ||
			eRELATION == KNOWN_POTENTIAL_ENEMY_OF || eRELATION == ENEMY_OF) ?
				(eSTATUS >= MAJOR_CIV ? CvAgents::MAJOR_ALIVE : CvAgents::CIV_ALIVE) :
			CvAgents::MAJOR_ALIVE);
};

/*  It seems that the compiler doesn't remove unreachable branches if I define
	bAPPLY_FILTERS as a static bool const member. For 'eCACHE_... == CvAgents::NO_CACHE',
	it seems to work. */
#define bAPPLY_FILTERS (eCACHE_SUBSETEQ == CvAgents::NO_CACHE)
#define _bADD_BARBARIANS(eCACHE) (eCACHE != CvAgents::ALL && eCACHE != CvAgents::MEMBER && \
		eCACHE != CvAgents::MEMBER_ALIVE && eCACHE != CvAgents::VASSAL_ALIVE)
#define bADD_BARBARIANS ((bAPPLY_FILTERS && _bADD_BARBARIANS(eCACHE_SUPER)) || \
		(!bAPPLY_FILTERS && _bADD_BARBARIANS(eCACHE_SUBSETEQ)))

template<class AgentType, AgentStatusPredicate eSTATUS, AgentRelationPredicate eRELATION>
class AgentIterator : ExplicitAgentIterator<AgentType, eSTATUS, eRELATION>
{
public:
	__forceinline bool hasNext() const
	{
		return (m_pNext != NULL);
	}

	__forceinline AgentType& operator*()
	{
		return *m_pNext;
	}

protected:
	AgentIterator(TeamTypes eTeam = NO_TEAM) : ExplicitAgentIterator<AgentType,eSTATUS,eRELATION>(eTeam)
	{
		/*  ExplicitAgentIterator gets instantiated for all combinations of template parameters,
			so these static assertions would fail in that class. In this derived class, they'll
			only fail when an offending AgentIterator object is declared.
			See comments in AgentPredicates.h about the specific assertions. */
		BOOST_STATIC_ASSERT(eSTATUS != ANY_AGENT_STATUS || eRELATION <= NOT_SAME_TEAM_AS);
		BOOST_STATIC_ASSERT(eRELATION != VASSAL_OF || eSTATUS < FREE_MAJOR_CIV);

		FAssert(eRELATION == ANY_AGENT_RELATION || (eTeam > NO_TEAM && eTeam < MAX_TEAMS));
		if (bAPPLY_FILTERS)
		{
			/*  (This if/else and the one below could be avoided by laying out all caches
				sequentially so that there is only one get...Cache function) */
			if (eCACHE_SUPER < CvAgents::NUM_STATUS_CACHES)
				m_pCache = m_pAgents->getAgentSeqCache<AgentType>(eCACHE_SUPER);
			else m_pCache = m_pAgents->getPerTeamSeqCache<AgentType>(eCACHE_SUPER, eTeam);
		}
		else
		{
			if (eCACHE_SUBSETEQ < CvAgents::NUM_STATUS_CACHES)
				m_pCache = m_pAgents->getAgentSeqCache<AgentType>(eCACHE_SUBSETEQ);
			else m_pCache = m_pAgents->getPerTeamSeqCache<AgentType>(eCACHE_SUBSETEQ, eTeam);
		}
		m_iPos = 0;
		// Cache the cache size (std::vector computes it as 'end' minus 'begin')
		m_iCacheSize = m_pCache->size();
		generateNext();
	}

	AgentIterator& operator++()
	{
		FAssertMsg(false, "Derived classes should define their own operator++ function");
		// This is what derived classes should do (force-inlined, arguably):
		generateNext();
		return *this;
	}

	/*  Tbd.(?): In the !bAPPLY_FILTERS&&!bADD_BARBARIANS case, it could be more efficient
		to split generateNext between hasNext (check bounds), operator++ (increment m_iPos)
		and operator* (access m_pCache). */
	void generateNext()
	{
		if (bADD_BARBARIANS)
		{
			int const iRemaining = m_iCacheSize - m_iPos;
			switch(iRemaining)
			{
			case 0:
				m_pNext = getBarbarianAgent();
				m_iPos++;
				// Currently all status predicates are already handled by bADD_BARBARIANS
				if (eRELATION != ANY_AGENT_RELATION && !passFilters(*m_pNext))
					m_pNext = NULL;
				return;
			case -1:
				m_pNext = NULL;
				return;
			default: m_pNext = (*m_pCache)[m_iPos];
			}
		}
		else
		{
			if (m_iPos < m_iCacheSize)
				m_pNext = (*m_pCache)[m_iPos];
			else
			{
				m_pNext = NULL;
				return;
			}
		}
		m_iPos++;
		if (bAPPLY_FILTERS)
		{
			if (!passFilters(*m_pNext))
			{
				m_pNext = NULL;
				generateNext(); // tail recursion
			}
		}
	}

private:
	std::vector<AgentType*> const* m_pCache;
	short m_iCacheSize;
	short m_iPos;
	AgentType* m_pNext;

	// Don't want to assume that BARBARIAN_PLAYER==BARBARIAN_TEAM
	__forceinline AgentType* getBarbarianAgent() const
	{
		return _getBarbarianAgent<AgentType>();
	}
	template<class T>
	T* _getBarbarianAgent() const;
	template<>
	__forceinline CvPlayerAI* _getBarbarianAgent<CvPlayerAI>() const
	{
		// Don't want to include an AI header for GET_PLAYER
		return (*m_pAgents->getAgentSeqCache<CvPlayerAI>(CvAgents::ALL))[BARBARIAN_PLAYER];
	}
	template<>
	__forceinline CvTeamAI* _getBarbarianAgent<CvTeamAI>() const
	{
		return (*m_pAgents->getAgentSeqCache<CvTeamAI>(CvAgents::ALL))[BARBARIAN_TEAM];
	}
};
#undef bAPPLY_FILTERS
#undef bADD_BARBARIANS
#undef _bADD_BARBARIANS

// For lack of C++11 alias declarations:

template<AgentStatusPredicate eSTATUS = ALIVE, AgentRelationPredicate eRELATION = ANY_AGENT_RELATION>
class PlayerIter : public AgentIterator<CvPlayerAI, eSTATUS, eRELATION>
{
public:
	explicit PlayerIter(TeamTypes eTeam = NO_TEAM) : AgentIterator<CvPlayerAI,eSTATUS,eRELATION>(eTeam) {}
	__forceinline PlayerIter& operator++()
	{
		generateNext();
		return *this;
	}
};

template<AgentStatusPredicate eSTATUS = ALIVE, AgentRelationPredicate eRELATION = ANY_AGENT_RELATION>
class TeamIter : public AgentIterator<CvTeamAI, eSTATUS, eRELATION>
{
public:
	explicit TeamIter(TeamTypes eTeam = NO_TEAM) : AgentIterator<CvTeamAI,eSTATUS,eRELATION>(eTeam)
	{
		// Can't loop over all "teams that are member of eTeam"
		BOOST_STATIC_ASSERT(eRELATION != MEMBER_OF);
	}
	__forceinline TeamIter& operator++()
	{
		generateNext();
		return *this;
	}
};

// A bit nicer than PlayerIter</*...*/,MEMBER_OF>
template<AgentStatusPredicate eSTATUS = ALIVE>
class MemberIter : public PlayerIter<eSTATUS, MEMBER_OF>
{
public:
	explicit MemberIter(TeamTypes eTeam) : PlayerIter<eSTATUS,MEMBER_OF>(eTeam) {}
	__forceinline MemberIter& operator++()
	{
		generateNext();
		return *this;
	}
};


#endif
