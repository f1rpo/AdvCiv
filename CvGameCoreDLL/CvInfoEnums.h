#pragma once

#ifndef CV_ENUM_HELPERS_H
#define CV_ENUM_HELPERS_H

// advc.enum: New header for inclusion in CvEnums.h

/*  operator++ functions copied from "We the People" (original author: Nightinggale).
	For iterating over an enum when FOR_EACH_ENUM isn't applicable. Also used by EnumMap. */
template <class T>
static inline T& operator++(T& c)
{
	c = static_cast<T>(c + 1);
	return c;
}

template <class T>
static inline T operator++(T& c, int)
{
	T cache = c;
	c = static_cast<T>(c + 1);
	return cache;
}

#define FOR_EACH_ENUM(TypeName) \
	for (TypeName##Types eLoop##TypeName = (TypeName##Types)0; \
			eLoop##TypeName != getEnumLength(eLoop##TypeName); \
			eLoop##TypeName = (TypeName##Types)(eLoop##TypeName + 1))

#define NUM_ENUM_TYPES(INFIX) NUM_##INFIX##_TYPES
#define NO_ENUM_TYPE(SUFFIX) NO_##SUFFIX = -1

// (See SET_NONXML_ENUM_LENGTH in EnumMap.h about the bAllowForEach parameter)
#define SET_ENUM_LENGTH_STATIC(Name, INFIX) \
	__forceinline Name##Types getEnumLength(Name##Types, bool bAllowForEach = true) \
	{ \
		return NUM_ENUM_TYPES(INFIX); \
	}
/*  This gets used in CvGlobals.h. (I wanted to do it here in MAKE_INFO_ENUM, but
	that lead to a circular dependency.) */
#define SET_ENUM_LENGTH(Name, PREFIX) \
	__forceinline Name##Types getEnumLength(Name##Types, bool bAllowForEach = true) \
	{ \
		return static_cast<Name##Types>(gGlobals.getNum##Name##Infos()); \
	}

#define MAKE_INFO_ENUM(Name, PREFIX) \
enum Name##Types \
{ \
	NO_ENUM_TYPE(PREFIX), \
};

/*  No variadic macros in MSVC03, so, without using an external code generator,
	this is all I can do: */
#define ENUM_START(Name, PREFIX) \
enum Name##Types \
{ \
	NO_ENUM_TYPE(PREFIX),

#define ENUM_END(Name, PREFIX) \
	NUM_ENUM_TYPES(PREFIX) \
}; \
SET_ENUM_LENGTH_STATIC(Name, PREFIX)
// For enumerators that are supposed to be excluded from iteration
#define ENUM_END_HIDDEN(Name, PREFIX) \
}; \
SET_ENUM_LENGTH_STATIC(Name, PREFIX)
// (Let's worry about #ifdef _USRDLL only when the source of the EXE is released, i.e. probably never.)

#define DO_FOR_EACH_INFO_TYPE(DO) \
	DO(Color, COLOR) \
	DO(PlayerColor, PLAYERCOLOR) \
	DO(Effect, EFFECT) \
	DO(Attachable, ATTACHABLE) \
	DO(Climate, CLIMATE) \
	DO(SeaLevel, SEALEVEL) \
	DO(Terrain, TERRAIN) \
	DO(Advisor, ADVISOR) \
	DO(Emphasize, EMPHASIZE) \
	DO(Victory, VICTORY) \
	DO(Feature, FEATURE) \
	DO(Bonus, BONUS) \
	DO(BonusClass, BONUSCLASS) \
	DO(Improvement, IMPROVEMENT) \
	DO(Route, ROUTE) \
	DO(River, RIVER) \
	DO(Goody, GOODY) \
	DO(Build, BUILD) \
	DO(Handicap, HANDICAP) \
	DO(GameSpeed, GAMESPEED) \
	DO(TurnTimer, TURNTIMER) \
	DO(Era, ERA) \
	DO(Civilization, CIVILIZATION) \
	DO(LeaderHead, LEADER) \
	DO(Trait, TRAIT) \
	DO(BuildingClass, BUILDINGCLASS) \
	DO(Building, BUILDING) \
	DO(SpecialBuilding, SPECIALBUILDING) \
	DO(Project, PROJECT) \
	DO(Process, PROCESS) \
	DO(Vote, VOTE) \
	DO(Concept, CONCEPT) \
	DO(NewConcept, NEW_CONCEPT) \
	DO(Season, SEASON) \
	DO(Month, MONTH) \
	DO(UnitClass, UNITCLASS) \
	DO(Unit, UNIT) \
	DO(SpecialUnit, SPECIALUNIT) \
	DO(UnitCombat, UNITCOMBAT) \
	DO(Invisible, INVISIBLE) \
	DO(VoteSource, VOTESOURCE) \
	DO(Promotion, PROMOTION) \
	DO(Tech, TECH) \
	DO(Specialist, SPECIALIST) \
	DO(Religion, RELIGION) \
	DO(Corporation, CORPORATION) \
	DO(Hurry, HURRY) \
	DO(Upkeep, UPKEEP) \
	DO(CultureLevel, CULTURELEVEL) \
	DO(CivicOption, CIVICOPTION) \
	DO(Civic, CIVIC) \
	DO(Cursor, CURSOR) \
	DO(Event, EVENT) \
	DO(EventTrigger, EVENTTRIGGER) \
	DO(EspionageMission, ESPIONAGEMISSION)

#endif
