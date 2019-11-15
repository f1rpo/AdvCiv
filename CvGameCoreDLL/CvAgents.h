#pragma once

#ifndef CV_AGENTS_H
#define CV_AGENTS_H

/*  advc.agent: New class for access (via AgentIterator) to sequences of
	agent (CvTeam, CvPlayer) objects. Caches frequently needed sequences. */

#include "AgentGenerator.h"

class CvPlayerAI;
class CvTeamAI;


class CvAgents
{
public:
	/*  To be initialized from CvGame (replacing the initStatics calls
		in CvGlobals::init) */
	CvAgents(int iMaxPlayers, int iMaxTeams);
	void gameStart(bool bFromSaveGame);
	void playerDefeated(PlayerTypes ePlayer);
	void colonyCreated(PlayerTypes eNewPlayer);
	void teamCapitulated(TeamTypes eVassal, TeamTypes eMaster)
	{
		updateVassal(eVassal, eMaster, true); // sufficient for now
	}
	void voluntaryVassalAgreementSigned(TeamTypes eVassal, TeamTypes eMaster)
	{
		updateVassal(eVassal, eMaster, true);
	}
	void vassalFreed(TeamTypes eVassal, TeamTypes eMaster)
	{
		updateVassal(eVassal, eMaster, false);
	}
	void allianceFormed() // Tbd.: Not currently called
	{
		updateAllCachedSequences();
	}
	// Might be needed in the future
	//void bordersOpened(TeamTypes eFirst, TeamTypes eSecond);
	//void bordersClosed(TeamTypes eFirst, TeamTypes eSecond);
	//void warDeclared(TeamTypes eFirst, TeamTypes eSecond);
	//void peaceMade(TeamTypes eFirst, TeamTypes eSecond);
	//void teamsMet(TeamTypes eFirst, TeamTypes eSecond);
	//void setHuman(PlayerTypes ePlayer, bool bNewValue);

	template<class AgentType>
	AgentGenerator<AgentType>* getAgentGenerator(
			AgentStatusPredicate eStatus = ALIVE,
			AgentRelationPredicate eRelation = ANY_AGENT_RELATION,
			TeamTypes eTeam = NO_TEAM, PlayerTypes ePlayer = NO_PLAYER);
	// <advc.tmp> For speedTest in CvAgents.cpp
	int majorTeams() const { return (int)m_majorTeamsAlive.size(); }
	CvTeamAI& majorTeam(int iAt) const { return *m_majorTeamsAlive[iAt]; }
	// </advc.tmp>
	// Implement functions like CvGame::countCivPlayersAlive (also: CvTeam)
	// here. Either get the size of a vector (add 1 for barbs) or step
	// through a generator with a counter

private:
	void updateAllCachedSequences();
	void updateVassal(TeamTypes eVassal, TeamTypes eMaster, bool bVassal);
	static bool includeBarbarians(AgentStatusPredicate eStatus, AgentRelationPredicate eRelation);

	bool m_bNoBarbarians;
	typedef std::vector<CvPlayerAI*> PlayerVector;
	typedef std::vector<CvTeamAI*> TeamVector;
	PlayerVector m_allPlayers;
	PlayerVector m_civPlayersEverAlive;
	PlayerVector m_civPlayersAlive;
	PlayerVector m_majorPlayersAlive;
	PlayerVector m_noPlayers; // empty
	TeamVector m_allTeams;
	TeamVector m_civTeamsEverAlive;
	TeamVector m_civTeamsAlive;
	TeamVector m_majorTeamsAlive;
	TeamVector m_noTeams; // empty
	std::vector<PlayerVector> m_playersPerTeam;
	std::vector<PlayerVector> m_playersAlivePerTeam;
	std::vector<PlayerVector> m_vassalPlayersAlivePerTeam;
	std::vector<TeamVector> m_vassalTeamsAlivePerTeam;

	// For accessing agent vectors from function templates
	template<class AgentType> std::vector<AgentType*> const* getAllAgents() const;
	template<class AgentType> std::vector<AgentType*> const* getCivAgentsEverAlive() const;
	template<class AgentType> std::vector<AgentType*> const* getCivAgentsAlive() const;
	template<class AgentType> std::vector<AgentType*> const* getMajorAgentsAlive() const;
	template<class AgentType> std::vector<AgentType*> const* getVassalAgentsAlivePerTeam(TeamTypes) const;
	template<class AgentType> std::vector<AgentType*> const* getAgentsPerTeam(TeamTypes) const;
	template<class AgentType> std::vector<AgentType*> const* getAgentsAlivePerTeam(TeamTypes) const;
	template<class AgentType> std::vector<AgentType*> const* getNoAgents() const;
	template<>
	__forceinline PlayerVector const* getAllAgents<CvPlayerAI>() const
	{
		return &m_allPlayers;
	}
	template<>
	__forceinline PlayerVector const* getCivAgentsEverAlive<CvPlayerAI>() const
	{
		return &m_civPlayersEverAlive;
	}
	template<>
	__forceinline PlayerVector const* getCivAgentsAlive<CvPlayerAI>() const
	{
		return &m_civPlayersAlive;
	}
	template<>
	__forceinline PlayerVector const* getMajorAgentsAlive<CvPlayerAI>() const
	{
		return &m_majorPlayersAlive;
	}
	template<>
	__forceinline PlayerVector const* getVassalAgentsAlivePerTeam<CvPlayerAI>(TeamTypes eMaster) const
	{
		return &m_vassalPlayersAlivePerTeam[eMaster];
	}
	template<>
	__forceinline PlayerVector const* getAgentsPerTeam<CvPlayerAI>(TeamTypes eTeam) const
	{
		return &m_playersPerTeam[eTeam];
	}
	template<>
	__forceinline PlayerVector const* getAgentsAlivePerTeam<CvPlayerAI>(TeamTypes eTeam) const
	{
		return &m_playersAlivePerTeam[eTeam];
	}
	template<>
	__forceinline PlayerVector const* getNoAgents<CvPlayerAI>() const
	{
		return &m_noPlayers;
	}
	template<>
	__forceinline TeamVector const* getAllAgents<CvTeamAI>() const
	{
		return &m_allTeams;
	}
	template<>
	__forceinline TeamVector const* getCivAgentsEverAlive<CvTeamAI>() const
	{
		return &m_allTeams;
	}
	template<>
	__forceinline TeamVector const* getCivAgentsAlive<CvTeamAI>() const
	{
		return &m_allTeams;
	}
	template<>
	__forceinline TeamVector const* getMajorAgentsAlive<CvTeamAI>() const
	{
		return &m_majorTeamsAlive;
	}
	template<>
	__forceinline TeamVector const* getVassalAgentsAlivePerTeam<CvTeamAI>(TeamTypes eMaster) const
	{
		return &m_vassalTeamsAlivePerTeam[eMaster];
	}
	template<>
	TeamVector const* getAgentsPerTeam<CvTeamAI>(TeamTypes eTeam) const;
	template<>
	TeamVector const* getAgentsAlivePerTeam<CvTeamAI>(TeamTypes eTeam) const;
	template<>
	__forceinline TeamVector const* getNoAgents<CvTeamAI>() const
	{
		return &m_noTeams;
	}

	template<class AgentGeneratorType>
	class AgentGeneratorPool
	{
	public:
		AgentGeneratorPool() : m_iPos(0) {}
		inline AgentGeneratorType& getNext()
		{
			AgentGeneratorType& r = m_aGenerators[m_iPos];
			m_iPos = (m_iPos + 1) % m_iSize;
			return r;
		}
	private:
		static int const m_iSize = 10;
		AgentGeneratorType m_aGenerators[m_iSize];
		int m_iPos;
	};

	mutable AgentGeneratorPool<FullyCachedAgentGenerator<CvPlayerAI> > m_fullyCachedPlayerGen;
	mutable AgentGeneratorPool<BarbarianAppendingAgentGenerator<CvPlayerAI> > m_barbAppendingPlayerGen;
	mutable AgentGeneratorPool<FilteringAgentGenerator<CvPlayerAI> > m_filteringPlayerGen;
	mutable AgentGeneratorPool<FullyCachedAgentGenerator<CvTeamAI> > m_fullyCachedTeamGen;
	mutable AgentGeneratorPool<BarbarianAppendingAgentGenerator<CvTeamAI> > m_barbAppendingTeamGen;
	mutable AgentGeneratorPool<FilteringAgentGenerator<CvTeamAI> > m_filteringTeamGen;

	template<class AgentType>
	AgentGenerator<AgentType>* getAgentGenerator(bool,bool,std::vector<AgentType*>const*,AgentStatusPredicate,AgentRelationPredicate,TeamTypes,PlayerTypes) const;
	template<>
	AgentGenerator<CvPlayerAI>* getAgentGenerator<CvPlayerAI>(bool bFilter, bool bInclBarbarians,
		PlayerVector const* pCachedSeq, AgentStatusPredicate eStatus, AgentRelationPredicate eRelation,
		TeamTypes eTeam, PlayerTypes ePlayer) const;
	template<>
	AgentGenerator<CvTeamAI>* getAgentGenerator<CvTeamAI>(bool bFilter, bool bInclBarbarians,
		TeamVector const* pCachedSeq, AgentStatusPredicate eStatus, AgentRelationPredicate eRelation,
		TeamTypes eTeam, PlayerTypes ePlayer) const;
};

#endif
