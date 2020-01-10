#pragma once

#ifndef CIV4_CITY_LIST_H
#define CIV4_CITY_LIST_H

#include "FFreeListTrashArray.h"
#include "CvCityAI.h"

// advc.003u: Wrapper for FLTA
class CvCityList : public FFreeListTrashArray<CvCityAI> {};

// <advc.003s> See CvUnitList.h about these macros
#define FOR_EACH_CITY(pCity, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvCity const* pCity = (kOwner).firstCity(&LOOPCOUNTERNAME); pCity != NULL; pCity = (kOwner).nextCity(&LOOPCOUNTERNAME))
#define FOR_EACH_CITY_VAR(pCity, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvCity* pCity = (kOwner).firstCity(&LOOPCOUNTERNAME); pCity != NULL; pCity = (kOwner).nextCity(&LOOPCOUNTERNAME))
// advc.003u: AI versions
#define FOR_EACH_CITYAI_VAR(pCity, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvCityAI* pCity = (kOwner).AI_firstCity(&LOOPCOUNTERNAME); pCity != NULL; pCity = (kOwner).AI_nextCity(&LOOPCOUNTERNAME))
#define FOR_EACH_CITYAI(pCity, kOwner) \
	int LOOPCOUNTERNAME; \
	for(CvCityAI const* pCity = (kOwner).AI_firstCity(&LOOPCOUNTERNAME); pCity != NULL; pCity = (kOwner).AI_nextCity(&LOOPCOUNTERNAME))
// </advc.003s>
#endif
