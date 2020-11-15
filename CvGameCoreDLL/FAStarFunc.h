#pragma once

#ifndef F_ASTAR_FUNC_H
#define F_ASTAR_FUNC_H

/*	advc.pf: New header for path finding functions (previously in CvGameCoreUtils.h).
	Most of these functions get passed to CvDLLFAStarIFaceBase, so they have to
	remain compatible with that interface. Also note that those functions get called
	from the EXE despite not being exported. I've replaced return type int with
	BOOL (which is a typedef of int) where appropritate. Those functions that
	KmodPathFinder used to call directly, I've turned into wrappers so that
	KmodPathFinder can use a more intuitive interface. */

#include "CvMap.h" // advc.inl: for pathHeuristic

class FAStarNode;
class FAStar;
class CvPlot;
class CvSelectionGroup;

BOOL pathDestValid(int iToX, int iToY, void const* pointer, FAStar* finder);
// advc.pf:
bool pathDestValid(CvPlot const& kTo, CvSelectionGroup const& kGroup, MovementFlags eFlags);
/*	<advc.inl> Inlining isn't going to help the pathfinder in the EXE,
	but I've verified (disassembly) that it helps KmodPathFinder. */
#define PATH_MOVEMENT_WEIGHT (1000)
// (NB: The linker requires the inline keyword here.)
inline int pathHeuristic(int iFromX, int iFromY, int iToX, int iToY)
{
	return stepDistance(iFromX, iFromY, iToX, iToY) * PATH_MOVEMENT_WEIGHT;
} // </advc.inl>
int pathCost(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);
// advc.pf:
int pathCost(CvPlot const& kFrom, CvPlot const& kTo, CvSelectionGroup const& kGroup, MovementFlags eFlags,
		int iCurrMovesLeft, int iKnownCost);
int stepHeuristic(int iFromX, int iFromY, int iToX, int iToY);
int stepCost(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);
BOOL pathValid(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);
// <K-Mod> (advc.pf: changed param lists so that the caller handles the plot lookup)
bool pathValid_join(CvPlot const& kFrom, CvPlot const& kTo, CvSelectionGroup const& kGroup, MovementFlags eFlags);
bool pathValid_source(CvPlot const& kFrom, CvSelectionGroup const& kGroup, MovementFlags eFlags,
		int iMovesLeft, int iTurns); // advc.pf
// </K-Mod>
BOOL pathAdd(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder);
// <advc.pf>
void pathAdd(FAStarNode const& kParent, FAStarNode& kNode, CvSelectionGroup const& kGroup, MovementFlags eFlags);
int pathInitialAdd(CvSelectionGroup const& kGroup, MovementFlags eFlags); // </advc.pf>
int stepDestValid(int iToX, int iToY, void const* pointer, FAStar* finder);
// advc.104b:
int stepDestValid_advc(int iToX, int iToY, void const* pointer, FAStar* finder);
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
