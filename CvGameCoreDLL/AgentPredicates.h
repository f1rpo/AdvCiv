#pragma once

#ifndef AGENT_PREDICATES_H
#define AGENT_PREDICATES_H

/*  advc.agent: New file. Predicates for specifying sets of agents
	(CvTeam or CvPlayer objects). */

/*  Any predicates that are added to these two enums need to satisfy the assumptions
	stated in comments. Otherwise, client code may break; e.g. code that relies on
	eStatus >= ALIVE implying that dead agents are excluded. */

enum AgentStatusPredicate
{
	ANY_AGENT_STATUS,
	ANY_STATUS = ANY_AGENT_STATUS, // ANY_AGENT_STATUS is too verbose after all
	EVER_ALIVE,
	//NEVER_ALIVE, // Iterating over _only_ dead agents should rarely be needed
	//DEFEATED,
	ALIVE,
	// The rest are assumed to imply ALIVE and non-Barbarian
	CIV_ALIVE,
	//MINOR_CIV,
	// The rest are assumed to exclude minor civs and Barbarians
	MAJOR_CIV,
	//VASSAL,
	FREE_MAJOR_CIV,
	//FREE_MAJOR_AI_CIV,
	HUMAN,
};

enum AgentRelationPredicate // Relative to some given second agent
{
	ANY_AGENT_RELATION,
	MEMBER_OF, // First agent has to be a player
	NOT_SAME_TEAM_AS,
	/*  The rest should imply that the agents are alive; shouldn't rely on
		relationships other than team membership being correct for dead players. */
	VASSAL_OF, // Currently assumed to imply non-human
	NOT_A_RIVAL_OF, // Same team or some vassal/ master relation
	POTENTIAL_ENEMY_OF, // incl. current war enemies
	//OPEN_BORDERS_WITH, // Or rather CAN_ENTER_BORDERS_OF
	// The rest include minor civs and Barbarians unless ruled out AgentStatusPredicate
	KNOWN_TO, // incl. the first agent (agents know themselves)
	KNOWN_POTENTIAL_ENEMY_OF,
	ENEMY_OF, // At war; war in preparation isn't enough.
};

#endif
