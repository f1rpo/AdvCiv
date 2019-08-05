#pragma once

#ifndef CIV4_SELECTIONGROUP_LIST_H
#define CIV4_SELECTIONGROUP_LIST_H

#include "FLTALoopCounter.h"
#include "FFreeListTrashArray.h"
#include "CvSelectionGroupAI.h"

// advc.003u: Wrapper for FLTA
class CvSelectionGroupList : public FFreeListTrashArray<CvSelectionGroupAI> {};

// <advc.003s> See CvUnitList.h about these macros
#define FOR_EACH_GROUP(pGroup, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvSelectionGroup const* pGroup = (kOwner).firstSelectionGroup(&LOOPCOUNTERNAME); pGroup != NULL; pGroup = (kOwner).nextSelectionGroup(&LOOPCOUNTERNAME))
#define FOR_EACH_GROUP_VAR(pGroup, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvSelectionGroup* pGroup = (kOwner).firstSelectionGroup(&LOOPCOUNTERNAME); pGroup != NULL; pGroup = (kOwner).nextSelectionGroup(&LOOPCOUNTERNAME))
// advc.003u: AI versions
#define FOR_EACH_GROUPAI(pGroup, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvSelectionGroupAI const* pGroup = (kOwner).AI_firstSelectionGroup(&LOOPCOUNTERNAME); pGroup != NULL; pGroup = (kOwner).AI_nextSelectionGroup(&LOOPCOUNTERNAME))
#define FOR_EACH_GROUPAI_VAR(pGroup, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvSelectionGroupAI* pGroup = (kOwner).AI_firstSelectionGroup(&LOOPCOUNTERNAME); pGroup != NULL; pGroup = (kOwner).AI_nextSelectionGroup(&LOOPCOUNTERNAME))
// </advc.003s>

#endif
