#pragma once

#ifndef CIV4_AREA_LIST_H
#define CIV4_AREA_LIST_H

#include "FFreeListTrashArray.h"
#include "CvArea.h"

// advc.003u: Wrapper for FLTA
class CvAreaList : public FFreeListTrashArray<CvArea> {};

// advc.003s: See also CvUnitList.h about the FOR_EACH macros

/*  To avoid calling GC.getMap() in every iteration; probably makes no difference,
	but don't want to trust inlining more than I have to. */
#define MAPVARNAME CONCATVARNAME(kMap_, __LINE__)

/*  Const-correctness for CvArea isn't normally achievable b/c functions always
	take CvArea* as a variable parameter, but I want to be consistent with the
	other FOR_EACH_... macros, so FOR_EACH_AREA uses a const CvArea*. */
#define FOR_EACH_AREA(pArea) \
	int LOOPCOUNTERNAME; CvMap const& MAPVARNAME = GC.getMap(); \
	for(CvArea const* pArea = MAPVARNAME.firstArea(&LOOPCOUNTERNAME); pArea != NULL; pArea = MAPVARNAME.nextArea(&LOOPCOUNTERNAME))
#define FOR_EACH_AREA_VAR(pArea) \
	int LOOPCOUNTERNAME; CvMap const& MAPVARNAME = GC.getMap(); \
	for(CvArea* pArea = MAPVARNAME.firstArea(&LOOPCOUNTERNAME); pArea != NULL; pArea = MAPVARNAME.nextArea(&LOOPCOUNTERNAME))

#endif
