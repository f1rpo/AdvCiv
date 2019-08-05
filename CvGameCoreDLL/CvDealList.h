#pragma once

#ifndef CIV4_DEAL_LIST_H
#define CIV4_DEAL_LIST_H

#include "FLTALoopCounter.h"
#include "FFreeListTrashArray.h"
#include "CvDeal.h"

// advc.003u: Wrapper for FLTA
class CvDealList : public FFreeListTrashArray<CvDeal> {};

// advc.003s: See also CvUnitList.h about the FOR_EACH macros

// See FreeListTraversal_Map.h about this
#define GAMEVARNAME CONCATVARNAME(kGame_, __LINE__)

#define FOR_EACH_DEAL(pDeal) \
	int LOOPCOUNTERNAME; CvGame const& GAMEVARNAME = GC.getGame(); \
	for(CvDeal const* pDeal = GAMEVARNAME.firstDeal(&LOOPCOUNTERNAME); pDeal != NULL; pDeal = GAMEVARNAME.nextDeal(&LOOPCOUNTERNAME))
#define FOR_EACH_DEAL_VAR(pDeal) \
	int LOOPCOUNTERNAME; CvGame const& GAMEVARNAME = GC.getGame(); \
	for(CvDeal* pDeal = GAMEVARNAME.firstDeal(&LOOPCOUNTERNAME); pDeal != NULL; pDeal = GAMEVARNAME.nextDeal(&LOOPCOUNTERNAME))

#endif
