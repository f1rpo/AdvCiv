#pragma once

#ifndef AGENT_PREDICATES_H
#define AGENT_PREDICATES_H

/*  advc.agent: New file. Predicates for specifying sets of agents
	(CvTeam or CvPlayer objects). */

/*  Any predicates that are added to this enum need to satisfy the assumptions
	stated in comments. Otherwise, client code may break; e.g. code that relies on
	eStatus >= ALIVE implying that dead agents are excluded. */
enum AgentStatusPredicate
{
	ANY_AGENT_STATUS,
	EVER_ALIVE,
	//NEVER_ALIVE, // Iterating over _only_ dead agents should rarely be needed
	//DEFEATED,
	ALIVE,
	// The rest are assumed to imply ALIVE
	NON_BARBARIAN, // The above are assumed to include Barbarians
	//MINOR_CIV,
	// The rest are assumed to exclude minor civs and Barbarians
	MAJOR_CIV, 
	VASSAL,
	FREE_MAJOR,
	FREE_MAJOR_AI,
	HUMAN,
}; // Default: ALIVE

enum AgentRelationPredicate // Relative to some given second agent
{
	ANY_AGENT_RELATION,
	SAME_TEAM_AS,
	NOT_SAME_TEAM_AS,
	VASSAL_OF,
	NOT_A_RIVAL_OF, // Same team or some vassal/ master relation
	POTENTIAL_ENEMY_OF, // incl. current war enemies
	//OPEN_BORDERS_WITH, // Or rather CAN_ENTER_BORDERS_OF
	// The rest include minor civs and Barbarians unless ruled out AgentStatusPredicate
	KNOWN_TO,
	KNOWN_POTENTIAL_ENEMY_OF,
	ENEMY_OF,
}; // Default: SAME_TEAM_AS when a second agent is given; otherwise ANY_AGENT_RELATION.

#endif
