#pragma once

#ifndef FREE_LIST_TRAVERSAL_H
#define FREE_LIST_TRAVERSAL_H

// advc.003s: Macros for traversing FFreeListTrashArray
// advc.003u: Added variants that provide a pointer to an AI object (e.g. FOR_EACH_UNITAI).

/*  All FOR_EACH macros need to be listed in cpp.hint so that Visual Studio can
	parse them properly! Otherwise, the navigation bar will show "Global Scope"
	for any functions that use the macros. (At least in VS2010; perhaps not an issue
	with more recent versions.) */

// ('iLoopCounter_##__LINE__' won't work)
#define LOOPCOUNTERNAME CONCATVARNAME(iAnonLoopCounter_, __LINE__)

/*  pointerType argument to allow the caller to choose AI or non-AI type and
	const or non-const. -- Not intuitive enough; create a separate macro for every
	combination instead. */
/*#define FOR_EACH_UNIT(pointerType, pUnit, kOwner) \
	int LOOPCOUNTERNAME; \
	for(pointerType* pUnit = (kOwner).AI_firstUnit(&LOOPCOUNTERNAME); pCity != NULL; pCity = (kOwner).AI_nextUnit(&LOOPCOUNTERNAME))*/

// Make const the default (shortest macro name)
#define FOR_EACH_UNIT(pUnit, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvUnit const* pUnit = (kOwner).firstUnit(&LOOPCOUNTERNAME); pUnit != NULL; pUnit = (kOwner).nextUnit(&LOOPCOUNTERNAME))
#define FOR_EACH_UNIT_VAR(pUnit, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvUnit* pUnit = (kOwner).firstUnit(&LOOPCOUNTERNAME); pUnit != NULL; pUnit = (kOwner).nextUnit(&LOOPCOUNTERNAME))
#define FOR_EACH_UNITAI(pUnit, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvUnitAI const* pUnit = (kOwner).AI_firstUnit(&LOOPCOUNTERNAME); pUnit != NULL; pUnit = (kOwner).AI_nextUnit(&LOOPCOUNTERNAME))
#define FOR_EACH_UNITAI_VAR(pUnit, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvUnitAI* pUnit = (kOwner).AI_firstUnit(&LOOPCOUNTERNAME); pUnit != NULL; pUnit = (kOwner).AI_nextUnit(&LOOPCOUNTERNAME))

#define FOR_EACH_CITY(pCity, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvCity const* pCity = (kOwner).firstCity(&LOOPCOUNTERNAME); pCity != NULL; pCity = (kOwner).nextCity(&LOOPCOUNTERNAME))
#define FOR_EACH_CITY_VAR(pCity, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvCity* pCity = (kOwner).firstCity(&LOOPCOUNTERNAME); pCity != NULL; pCity = (kOwner).nextCity(&LOOPCOUNTERNAME))
#define FOR_EACH_CITYAI_VAR(pCity, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvCityAI* pCity = (kOwner).AI_firstCity(&LOOPCOUNTERNAME); pCity != NULL; pCity = (kOwner).AI_nextCity(&LOOPCOUNTERNAME))
#define FOR_EACH_CITYAI(pCity, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvCityAI const* pCity = (kOwner).AI_firstCity(&LOOPCOUNTERNAME); pCity != NULL; pCity = (kOwner).AI_nextCity(&LOOPCOUNTERNAME))

#define FOR_EACH_GROUP(pGroup, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvSelectionGroup const* pGroup = (kOwner).firstSelectionGroup(&LOOPCOUNTERNAME); pGroup != NULL; pGroup = (kOwner).nextSelectionGroup(&LOOPCOUNTERNAME))
#define FOR_EACH_GROUP_VAR(pGroup, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvSelectionGroup* pGroup = (kOwner).firstSelectionGroup(&LOOPCOUNTERNAME); pGroup != NULL; pGroup = (kOwner).nextSelectionGroup(&LOOPCOUNTERNAME))
#define FOR_EACH_GROUPAI(pGroup, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvSelectionGroupAI const* pGroup = (kOwner).AI_firstSelectionGroup(&LOOPCOUNTERNAME); pGroup != NULL; pGroup = (kOwner).AI_nextSelectionGroup(&LOOPCOUNTERNAME))
#define FOR_EACH_GROUPAI_VAR(pGroup, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvSelectionGroupAI* pGroup = (kOwner).AI_firstSelectionGroup(&LOOPCOUNTERNAME); pGroup != NULL; pGroup = (kOwner).AI_nextSelectionGroup(&LOOPCOUNTERNAME))

/*  To avoid calling GC.getMap() in every iteration; probably makes no difference,
	but don't want to rely on inlining too much. */
#define MAPVARNAME CONCATVARNAME(kMap_, __LINE__)

#define FOR_EACH_AREA(pArea) \
	int LOOPCOUNTERNAME; CvMap const& MAPVARNAME = GC.getMap(); \
	for(CvArea const* pArea = MAPVARNAME.firstArea(&LOOPCOUNTERNAME); pArea != NULL; pArea = MAPVARNAME.nextArea(&LOOPCOUNTERNAME))
#define FOR_EACH_AREA_VAR(pArea) \
	int LOOPCOUNTERNAME; CvMap const& MAPVARNAME = GC.getMap(); \
	for(CvArea* pArea = MAPVARNAME.firstArea(&LOOPCOUNTERNAME); pArea != NULL; pArea = MAPVARNAME.nextArea(&LOOPCOUNTERNAME))

#define GAMEVARNAME CONCATVARNAME(kGame_, __LINE__)

#define FOR_EACH_DEAL(pDeal) \
	int LOOPCOUNTERNAME; CvGame const& GAMEVARNAME = GC.getGame(); \
	for(CvDeal const* pDeal = GAMEVARNAME.firstDeal(&LOOPCOUNTERNAME); pDeal != NULL; pDeal = GAMEVARNAME.nextDeal(&LOOPCOUNTERNAME))
#define FOR_EACH_DEAL_VAR(pDeal) \
	int LOOPCOUNTERNAME; CvGame const& GAMEVARNAME = GC.getGame(); \
	for(CvDeal* pDeal = GAMEVARNAME.firstDeal(&LOOPCOUNTERNAME); pDeal != NULL; pDeal = GAMEVARNAME.nextDeal(&LOOPCOUNTERNAME))

#endif
