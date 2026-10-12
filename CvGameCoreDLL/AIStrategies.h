#pragma once

#ifndef AI_STRATEGIES_H // advc: Renamed this file from AI_Defines.h
#define AI_STRATEGIES_H

// BETTER_BTS_AI_MOD, 03/08/10, jdog5000: START
// Could increase this value now that player closeness is fixed
#define DEFAULT_PLAYER_CLOSENESS 7

#define AI_DAGGER_THRESHOLD 100  // higher is a lower chance
// <!-- custom: The removed MAX_AI_STRATEGIES was a 32-bit scan bound, not the number of defined strategies; it brute-forced nonexistent bit values and provided no canonical iterable value/name registry for shared callers.
// Iterable UI/diagnostic callers now use the shared SASAIStrategyDescriptor table, whose exact enum coverage and raw names are enforced by CI. (GPT-5.6-Sol) -->
// <!-- custom: Generate AdvCiv's AIStrategy enum and SAS's iterable raw-name descriptors from one list instead of synchronizing two declarations.
// Preserve all bit positions and descriptor ordering, including AI_DEFAULT_STRATEGY; NO_AI_STRATEGY remains the separate zero sentinel.
// The unchanged inherited strategy notes below are hoisted from the former enum body; named headings retain their association with individual rows.
// Gameplay still reads enum constants directly; list expansion is compile-time and existing text/iteration helpers keep their interfaces. (GPT-6.1-Sol) -->
// <!-- custom: Retain the former BETTER_BTS_AI_MOD / K-Mod enum notes verbatim below, with explicit strategy-name headings after moving them out of generated declarations.
// Military strategies based on power, war status
// AI_STRATEGY_DAGGER:
// Aggressive early game
// AI_STRATEGY_CRUSH:
// Convert units to City Attack
// AI_STRATEGY_ALERT1:
// Likely attack from neighbor
// AI_STRATEGY_ALERT2:
// Seemingly imminent attack from enemy
// AI_STRATEGY_TURTLE:
// Defensive shell
// Military strategies based on analysis of trainable units
// AI_STRATEGY_OWABWNW:
// "Our words are backed with nuclear weapons"
// Domestic strategies
// AI_STRATEGY_ECONOMY_FOCUS:
// K-Mod - catch up in tech, at the expense of military
// AI_STRATEGY_ESPIONAGE_ECONOMY:
// K-Mod - run high espionage slider to steal techs at a discount. (GPT-6.1-Sol) -->
#define SAS_AI_STRATEGY_ROWS(DO) \
	DO(AI_DEFAULT_STRATEGY, 0) \
	DO(AI_STRATEGY_DAGGER, 1) \
	DO(AI_STRATEGY_CRUSH, 2) \
	DO(AI_STRATEGY_ALERT1, 3) \
	DO(AI_STRATEGY_ALERT2, 4) \
	DO(AI_STRATEGY_TURTLE, 5) \
	DO(AI_STRATEGY_LAST_STAND, 6) \
	DO(AI_STRATEGY_FINAL_WAR, 7) \
	DO(AI_STRATEGY_GET_BETTER_UNITS, 8) \
	DO(AI_STRATEGY_FASTMOVERS, 9) \
	DO(AI_STRATEGY_LAND_BLITZ, 10) \
	DO(AI_STRATEGY_AIR_BLITZ, 11) \
	DO(AI_STRATEGY_OWABWNW, 12) \
	DO(AI_STRATEGY_PRODUCTION, 13) \
	DO(AI_STRATEGY_MISSIONARY, 14) \
	DO(AI_STRATEGY_BIG_ESPIONAGE, 15) \
	DO(AI_STRATEGY_ECONOMY_FOCUS, 16) \
	DO(AI_STRATEGY_ESPIONAGE_ECONOMY, 17)

enum AIStrategy // advc.enum: To avoid mixup with victory strategies (was #define)
{
	NO_AI_STRATEGY = 0, // advc.enum
#define SAS_AI_STRATEGY_ENUM(NAME, BIT) NAME = (1 << BIT),
	SAS_AI_STRATEGY_ROWS(SAS_AI_STRATEGY_ENUM)
#undef SAS_AI_STRATEGY_ENUM
};

// AI victory stages
enum AIVictoryStage // advc: replacing preprocessor defines
{
	NO_AI_VICTORY_STAGE				=	0, // advc.enum
	AI_DEFAULT_VICTORY_STAGE		=	(1 << 0),
	AI_VICTORY_SPACE1				=	(1 << 1),
	AI_VICTORY_SPACE2				=	(1 << 2),
	AI_VICTORY_SPACE3				=	(1 << 3),
	AI_VICTORY_SPACE4				=	(1 << 4),
	AI_VICTORY_CONQUEST1			=	(1 << 5),
	AI_VICTORY_CONQUEST2			=	(1 << 6),
	AI_VICTORY_CONQUEST3			=	(1 << 7),
	AI_VICTORY_CONQUEST4			=	(1 << 8),
	AI_VICTORY_CULTURE1				=	(1 << 9),   //religions and wonders
	AI_VICTORY_CULTURE2				=	(1 << 10),  //mass culture buildings
	AI_VICTORY_CULTURE3				=	(1 << 11),  //culture slider
	AI_VICTORY_CULTURE4				=	(1 << 12),
	AI_VICTORY_DOMINATION1			=	(1 << 13),
	AI_VICTORY_DOMINATION2			=	(1 << 14),
	AI_VICTORY_DOMINATION3			=	(1 << 15),
	AI_VICTORY_DOMINATION4			=	(1 << 16),
	AI_VICTORY_DIPLOMACY1			=	(1 << 17),
	AI_VICTORY_DIPLOMACY2			=	(1 << 18),
	AI_VICTORY_DIPLOMACY3			=	(1 << 19),
	AI_VICTORY_DIPLOMACY4			=	(1 << 20),
	AI_VICTORY_MILITARY1			=	AI_VICTORY_DOMINATION1 | AI_VICTORY_CONQUEST1,
	AI_VICTORY_MILITARY2			=	AI_VICTORY_DOMINATION2 | AI_VICTORY_CONQUEST2,
	AI_VICTORY_MILITARY3			=	AI_VICTORY_DOMINATION3 | AI_VICTORY_CONQUEST3,
	AI_VICTORY_MILITARY4			=	AI_VICTORY_DOMINATION4 | AI_VICTORY_CONQUEST4,
};
// BETTER_BTS_AI_MOD: END
// advc: Added by the BtS expansion; has (sadly perhaps) remained unused.
/*enum AICityRole
{
	AI_CITY_ROLE_VALID              =	(1 <<  1),	//zero is bad
	AI_CITY_ROLE_BIG_CULTURE        =	(1 <<  2),	//culture victory, probably
	AI_CITY_ROLE_BIG_PRODUCTION     =	(1 <<  3),	//don't build girly NW's
	AI_CITY_ROLE_BIG_MILITARY       =	(1 <<  4),	//stick with military stuff
	AI_CITY_ROLE_SCIENCE            =	(1 <<  5),	//
	AI_CITY_ROLE_GOLD               =	(1 <<  6),	//
	AI_CITY_ROLE_PRODUCTION         =	(1 <<  7),	//
	AI_CITY_ROLE_SPECIALIST         =	(1 <<  8),	//
	AI_CITY_ROLE_FISHING            =	(1 <<  9),	//
	AI_CITY_ROLE_STAGING            =	(1 << 10),	//send troops here
	AI_CITY_ROLE_LINCHPIN            =	(1 << 11),	//this city must not fall
};
// <advc.enum>
OVERLOAD_BITWISE_OPERATORS(AICityRole)*/
OVERLOAD_BITWISE_OPERATORS(AIStrategy)
OVERLOAD_BITWISE_OPERATORS(AIVictoryStage)
// </advc.enum>

#endif // AI_DEFINES_H
