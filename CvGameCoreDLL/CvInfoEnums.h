#pragma once

#ifndef CV_INFO_ENUMS_H
#define CV_INFO_ENUMS_H

/*  advc.enum: New header; mostly for macros dealing with CvInfo classes and their
	associated enum types. */

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
// For accessing an info object in a loop over an info enum (tentative and unused)
#define _kLoop_(TypeName) \
	Cv##TypeName##Info const& kLoop##TypeName = GC.getInfo(eLoop##TypeName)

// Type lists ...

/*  Number of instances not known at compile time, but it's safe to say, even in
	mod-mods, that it's not going to be greater than MAX_CHAR. (CvXMLLoadUtility
	will verify this too.) */
#define DO_FOR_EACH_SMALL_DYN_INFO_TYPE(DO) \
	/* getNumInfos function exported */ \
	DO(Route, ROUTE) \
	/* getInfo function and getNumInfos function exported */ \
	DO(Climate, CLIMATE) \
	DO(SeaLevel, SEALEVEL) \
	DO(Terrain, TERRAIN) \
	DO(Feature, FEATURE) \
	DO(TurnTimer, TURNTIMER) \
	DO(Improvement, IMPROVEMENT) \
	DO(Handicap, HANDICAP) \
	DO(GameSpeed, GAMESPEED) \
	DO(Era, ERA) \
	DO(Victory, VICTORY) \
	/* internal only */ \
	/*DO(Camera, CAMERAANIMATION)*/ /* advc.003j: unused */ \
	DO(Advisor, ADVISOR) \
	DO(Emphasize, EMPHASIZE) \
	DO(BonusClass, BONUSCLASS) \
	DO(River, RIVER) \
	DO(Goody, GOODY) \
	DO(Trait, TRAIT) \
	DO(Process, PROCESS) \
	DO(Season, SEASON) \
	DO(Month, MONTH) \
	DO(UnitCombat, UNITCOMBAT) \
	DO(Invisible, INVISIBLE) \
	DO(VoteSource, VOTESOURCE) \
	DO(Specialist, SPECIALIST) \
	DO(Religion, RELIGION) \
	DO(Corporation, CORPORATION) \
	DO(Hurry, HURRY) \
	DO(Upkeep, UPKEEP) \
	DO(CultureLevel, CULTURELEVEL) \
	DO(CivicOption, CIVICOPTION)

// Number of instances not known at compile time; might be greater than MAX_CHAR.
#define DO_FOR_EACH_BIG_DYN_INFO_TYPE(DO) \
	/* getInfo function exported */ \
	DO(Color, COLOR) \
	DO(AnimationCategory, ANIMCAT) \
	DO(EntityEvent, ENTITYEVENT) \
	DO(Effect, EFFECT) \
	DO(Attachable, ATTACHABLE) \
	DO(Build, BUILD) \
	/* both exported */ \
	DO(PlayerColor, PLAYERCOLOR) \
	DO(Bonus, BONUS) \
	DO(Civilization, CIVILIZATION) \
	DO(LeaderHead, LEADER) \
	DO(Cursor, CURSOR) \
	/* internal only */ \
	DO(BuildingClass, BUILDINGCLASS) \
	DO(Building, BUILDING) \
	DO(SpecialBuilding, SPECIALBUILDING) \
	DO(Project, PROJECT) \
	DO(Vote, VOTE) \
	DO(Concept, CONCEPT) \
	DO(NewConcept, NEW_CONCEPT) \
	DO(UnitClass, UNITCLASS) \
	DO(Unit, UNIT) \
	DO(SpecialUnit, SPECIALUNIT) \
	DO(Promotion, PROMOTION) \
	DO(Tech, TECH) \
	DO(Civic, CIVIC) \
	DO(Event, EVENT) \
	DO(EventTrigger, EVENTTRIGGER) \
	DO(EspionageMission, ESPIONAGEMISSION) \
	DO(UnitArtStyle, UNIT_ARTSTYLE)

// Number of instances not known at compile time
#define DO_FOR_EACH_DYN_INFO_TYPE(DO) \
	DO_FOR_EACH_SMALL_DYN_INFO_TYPE(DO) \
	DO_FOR_EACH_BIG_DYN_INFO_TYPE(DO)

/*  Number of instances not known at compile time; no associated enum type.
	(I've enter upper-case prefixes though in case that enums are desired at a
	later time.) */
#define DO_FOR_EACH_INT_INFO_TYPE(DO) \
	/* getInfo function exported */ \
	DO(ThroneRoomCamera, THRONEROOMCAM) \
	DO(MainMenu, MAINMENU) \
	DO(WaterPlane, WATERPLANE) \
	DO(Landscape, LANDSCAPE) \
	/* getNumInfos function exported */ \
	DO(CameraOverlay, CAMERAOVERLAY) \
	DO(UnitFormation, UNITFORMATION) \
	/* both exported */ \
	DO(RouteModel, ROUTEMODEL) \
	DO(RiverModel, RIVERMODEL) \
	DO(TerrainPlane, TERRAINPLANE) \
	DO(ThroneRoom, THRONEROOM) \
	DO(ThroneRoomStyle, THRONEROOMSTYLE) \
	DO(SlideShow, SLIDESHOW) \
	DO(SlideShowRandom, SLIDESHOWRAND) \
	DO(WorldPicker, WORLDPICKER) \
	DO(SpaceShip, SPACESHIP) \
	DO(Action, ACTION) \
	DO(Hint, HINT) \
	/* internal only */ \
	/* DO(Quest, QUEST) */ /* advc.003j: unused */ \
	DO(Diplomacy, DIPLOMACY) \
	DO(Tutorial, TUTORIAL)

#undef DOMAIN // defined in math.h
// Number of instances hardcoded in CvEnums.h
#define DO_FOR_EACH_STATIC_INFO_TYPE(DO) \
	/* exported */ \
	DO(AnimationPath, ANIMATIONPATH) \
	DO(InterfaceMode, INTERFACEMODE) \
	DO(GameOption, GAMEOPTION) \
	DO(MPOption, MPOPTION) \
	DO(PlayerOption, PLAYEROPTION) \
	DO(GraphicOption, GRAPHICOPTION) \
	DO(ForceControl, FORCECONTROL) \
	DO(Mission, MISSION) \
	/* internal only */ \
	DO(Yield, YIELD) \
	DO(Commerce, COMMERCE) \
	DO(Control, CONTROL) \
	DO(Command, COMMAND) \
	DO(Automate, AUTOMATE) \
	DO(Domain, DOMAIN) \
	DO(Attitude, ATTITUDE) \
	DO(Memory, MEMORY) \
	DO(CityTab, CITYTAB) \
	DO(Calendar, CALENDAR) \
	DO(UnitAI, UNITAI) \
	DO(Denial, DENIAL)

#define DO_FOR_EACH_INFO_TYPE(DO) \
	DO_FOR_EACH_DYN_INFO_TYPE(DO) \
	DO_FOR_EACH_STATIC_INFO_TYPE(DO) \
	DO_FOR_EACH_INT_INFO_TYPE(DO)

/*  These don't have a dedicated CvInfo class, and the macros for generating
	getter functions can't deal with that. (typedef would make it impossible
	to forward declare them in CvGlobals.h.) */
#define CvHintInfo CvInfoBase
#define CvConceptInfo CvInfoBase
#define CvNewConceptInfo CvInfoBase
#define CvSeasonInfo CvInfoBase
#define CvMonthInfo CvInfoBase
#define CvUnitCombatInfo CvInfoBase
#define CvInvisibleInfo CvInfoBase
#define CvUnitCombatInfo CvInfoBase
#define CvDomainInfo CvInfoBase
#define CvAttitudeInfo CvInfoBase
#define CvMemoryInfo CvInfoBase
#define CvCityTabInfo CvInfoBase
#define CvCalendarInfo CvInfoBase
#define CvUnitAIInfo CvInfoBase
#define CvDenialInfo CvInfoBase
// This one just has an irregular, exported name
#define CvThroneRoomCameraInfo CvThroneRoomCamera

// Macros for generating enum definitions and getEnumLength functions (CvEnums, CvGlobals) ...

#define NUM_ENUM_TYPES(INFIX) NUM_##INFIX##_TYPES
#define NO_ENUM_TYPE(SUFFIX) NO_##SUFFIX = -1

// (See SET_NONXML_ENUM_LENGTH in EnumMap.h about the bAllowForEach parameter)
#define SET_ENUM_LENGTH_STATIC(Name, INFIX) \
	__forceinline Name##Types getEnumLength(Name##Types, bool bAllowForEach = true) \
	{ \
		return NUM_ENUM_TYPES(INFIX); \
	}
/*  This gets used in CvGlobals.h. (I wanted to do it in MAKE_INFO_ENUM, which is
	used in CvEnums.h, but that lead to a circular dependency.) */
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

// Macros for generating CvInfo accessor functions (CvGlobals) ...
	
#define MAKE_INFO_ACCESSORS_DYN(Name, Dummy) \
	inline int getNum##Name##Infos() const \
	{ \
		return m_pa##Name##Info.size(); \
	} \
	inline Cv##Name##Info& getInfo(Name##Types e##Name) const \
	{ \
		FASSERT_BOUNDS(0, getNum##Name##Infos(), e##Name, "CvGlobals::getInfo("#Name"Types)"); \
		return *m_pa##Name##Info[e##Name]; \
	} \
	/* Deprecated: */ \
	inline Cv##Name##Info& get##Name##Info(Name##Types e##Name) const \
	{ \
		return getInfo(e##Name); \
	}
#define MAKE_INFO_ACCESSORS_INT(Name, Dummy) \
	inline int getNum##Name##Infos() const \
	{ \
		return m_pa##Name##Info.size(); \
	} \
	inline Cv##Name##Info& get##Name##Info(int i##Name) const \
	{ \
		FASSERT_BOUNDS(0, getNum##Name##Infos(), i##Name, "CvGlobals::get"#Name"Info"); \
		return *m_pa##Name##Info[i##Name]; \
	}
#define MAKE_INFO_ACCESSORS_STATIC(Name, INFIX) \
	inline Cv##Name##Info& getInfo(Name##Types e##Name) const \
	{ \
		FASSERT_BOUNDS(0, NUM_ENUM_TYPES(INFIX), e##Name, "CvGlobals::getInfo("#Name"Types)"); \
		return *m_pa##Name##Info[e##Name]; \
	} \
	/* Deprecated: */ \
	inline Cv##Name##Info& get##Name##Info(Name##Types e##Name) const \
	{ \
		return getInfo(e##Name); \
	}

#endif
