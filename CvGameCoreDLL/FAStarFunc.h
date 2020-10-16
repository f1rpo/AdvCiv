#pragma once

#ifndef F_ASTAR_FUNC_H
#define F_ASTAR_FUNC_H

/*	advc.pf: New header for path finding functions (previously in CvGameCoreUtils.h).
	Most of these functions get passed to CvDLLFAStarIFaceBase, so they have to
	remain compatible with that interface. Also note that those functions get called
	from the EXE despite not being exported. I've replaced return type int with
	BOOL (which is a typedef of int) where appropritate. Those functions that
	KmodPathFinder used to call directly, I've turned into wrappers so that
	GroupPathFinder can use a more intuitive interface. */

class FAStarNode;
class FAStar;
class CvPlot;
class CvSelectionGroup;

BOOL pathDestValid(int iToX, int iToY, void const* pointer, FAStar* finder);
int pathHeuristic(int iFromX, int iFromY, int iToX, int iToY);
int pathCost(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);
int stepHeuristic(int iFromX, int iFromY, int iToX, int iToY);
int stepCost(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);
BOOL pathValid(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);
BOOL pathAdd(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);

BOOL stepDestValid(int iToX, int iToY, void const* pointer, FAStar* finder);
// advc.104b:
BOOL stepDestValid_advc(int iToX, int iToY, void const* pointer, FAStar* finder);
BOOL stepValid(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);
BOOL stepAdd(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);
// BETTER_BTS_AI_MOD, 11/30/08, jdog5000:
BOOL teamStepValid(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);
// advc.104b:
BOOL teamStepValid_advc(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);

BOOL routeValid(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);
BOOL borderValid(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);
BOOL areaValid(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);
BOOL joinArea(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);
BOOL plotGroupValid(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);
BOOL countPlotGroup(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);

BOOL potentialIrrigation(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);
BOOL checkFreshWater(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);
BOOL changeIrrigated(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);

#endif
