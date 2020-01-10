#pragma once

#ifndef CIV4_UNIT_LIST_H
#define CIV4_UNIT_LIST_H

#include "FFreeListTrashArray.h"
#include "CvUnitAI.h"

// advc.003u: Wrapper for FLTA
class CvUnitList : public FFreeListTrashArray<CvUnitAI> {};

// <advc.003s> 

/*  All FOR_EACH macros need to be listed in cpp.hint so that Visual Studio can
	parse them properly! Otherwise, the navigation bar will show "Global Scope"
	for any functions that use the macros. */

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
// advc.003u: AI versions
#define FOR_EACH_UNITAI(pUnit, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvUnitAI const* pUnit = (kOwner).AI_firstUnit(&LOOPCOUNTERNAME); pUnit != NULL; pUnit = (kOwner).AI_nextUnit(&LOOPCOUNTERNAME))
#define FOR_EACH_UNITAI_VAR(pUnit, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvUnitAI* pUnit = (kOwner).AI_firstUnit(&LOOPCOUNTERNAME); pUnit != NULL; pUnit = (kOwner).AI_nextUnit(&LOOPCOUNTERNAME))
// </advc.003s>
#endif
