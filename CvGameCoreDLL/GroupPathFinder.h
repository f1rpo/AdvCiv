#pragma once

#ifndef GROUP_PATH_FINDER_H
#define GROUP_PATH_FINDER_H

#include "KmodPathFinder.h"
#include "CvMap.h" // for inlining heuristicStepCost
// <advc.tmp>
#ifdef FASSERT_ENABLE
#include "KmodPathFinderLegacy.h"
#endif // </advc.tmp>

/*	advc.pf: New header for classes implementing an A* path finder for
	selection groups of units. Mostly pre-AdvCiv code, just organized differently. */

class GroupPathNode : public PathNodeBase<GroupPathNode>
{
private:
	/*	Number of moves remaining to the unit with the fewest moves in the group.
		Corresponds to FAStarNode::m_iData1 in K-Mod. */
	int m_iMoves; // (short would suffice - but wouldn't help currently b/c of padding)
public:
	__forceinline int getMoves() const
	{
		return m_iMoves;
	}
	__forceinline void setMoves(int iMoves)
	{
		m_iMoves = iMoves;
	}
	// Give more specific meaning to the inherited path length data member
	__forceinline int getPathTurns() const
	{
		return m_iPathLength;
	}
	__forceinline void setPathTurns(int iPathTurns)
	{
		m_iPathLength = iPathTurns;
	}
	/*	I've added the same function to FAStarNode.
		To make GroupPathNode and FAStarNode compatible as template parameters
		in code shared by FAStar and GroupPathFinder. */
	inline CvPlot& getPlot() const
	{	/*	Not nice to access a global object here - which, if we weren't sure
			that there is only one map, could be inconsistent with the map used
			by GroupPathFinder. */
		return GC.getMap().getPlotByIndex(m_ePlot);
	}
};

class CvSelectionGroup;
#define PATH_MOVEMENT_WEIGHT (1000)

class GroupStepMetric : public StepMetricBase<GroupPathNode>
{
public:
/*	static interface so that GroupStepMetric can share code with the
	FAStar path finder in the EXE */
	static bool isValidStep(CvPlot const& kFrom, CvPlot const& kTo,
			CvSelectionGroup const& kGroup, MovementFlags eFlags);	
	static bool canStepThrough(CvPlot const& kFrom, CvSelectionGroup const& kGroup,
			MovementFlags eFlags, int iMoves, int iPathTurns);
	static bool isValidDest(CvPlot const& kPlot, CvSelectionGroup const& kGroup,
			MovementFlags eFlags);
	static int cost(CvPlot const& kFrom, CvPlot const& kTo,
			CvSelectionGroup const& kGroup, MovementFlags eFlags,
			int iCurrMoves, bool bAtStart);
	static inline int heuristicStepCost(int iFromX, int iFromY, int iToX, int iToY)
	{
		return stepDistance(iFromX, iFromY, iToX, iToY) * PATH_MOVEMENT_WEIGHT;
	}
	/*	The K-Mod code for updating path data is pretty intrusive; needs to
		have access to the node objects. */
	template<class Node> // GroupPathNode or FAStarNode
	static bool updatePathData(Node& kNode, Node const& kParent,
			CvSelectionGroup const& kGroup, MovementFlags eFlags);
	static int initialMoves(CvSelectionGroup const& kGroup, MovementFlags eFlags);

// Non-static interface ...
	GroupStepMetric(CvSelectionGroup const* pGroup = NULL,
		MovementFlags eFlags = NO_MOVEMENT_FLAGS, int iMaxPath = -1,
		int iHeuristicWeight = -1)
	:	StepMetricBase<GroupPathNode>(iMaxPath), m_pGroup(pGroup),
		m_eFlags(eFlags), m_iHeuristicWeight(iHeuristicWeight)
	{}
	inline CvSelectionGroup const* getGroup() const
	{
		return m_pGroup;
	}
	inline MovementFlags getFlags() const
	{
		return m_eFlags;
	}
	inline int getHeuristicWeight() const
	{
		return m_iHeuristicWeight;
	}
	inline bool isValidStep(CvPlot const& kFrom, CvPlot const& kTo) const
	{
		return isValidStep(kFrom, kTo, *m_pGroup, m_eFlags);
	}
	inline bool canStepThrough(CvPlot const& kPlot, GroupPathNode const& kNode) const
	{
		return canStepThrough(kPlot, *m_pGroup, m_eFlags,
				kNode.getMoves(), kNode.getPathTurns());
	}
	inline bool isValidDest(CvPlot const& kStart, CvPlot const& kDest) const
	{
		return isValidDest(kDest, *m_pGroup, m_eFlags);
	}
	inline int cost(CvPlot const& kFrom, CvPlot const& kTo,
		GroupPathNode const& kParentNode) const
	{
		return cost(kFrom, kTo, *m_pGroup, m_eFlags,
				kParentNode.getMoves(), kParentNode.m_iKnownCost != 0);
	}
	inline int heuristicCost(CvPlot const& kFrom, CvPlot const& kTo) const
	{
		return heuristicStepCost(kFrom.getX(), kFrom.getY(), kTo.getX(), kTo.getY()) *
				m_iHeuristicWeight;
	}
	inline bool updatePathData(GroupPathNode& kNode, GroupPathNode const& kParent) const
	{
		return updatePathData(kNode, kParent, *m_pGroup, m_eFlags);
	}
	inline void initializePathData(GroupPathNode& kNode) const
	{
		StepMetricBase<GroupPathNode>::initializePathData(kNode);
		kNode.setMoves(initialMoves(*m_pGroup, m_eFlags));
	}
	bool canReuseInitialPathData(GroupPathNode const& kStart) const;

protected:
	CvSelectionGroup const* m_pGroup;
	MovementFlags m_eFlags;
	int m_iHeuristicWeight;
};


class GroupPathFinder : public KmodPathFinder<GroupStepMetric, GroupPathNode>
{
public:
	static void InitHeuristicWeights();
	static int MinimumStepCost(int iBaseMoves);
	void invalidateGroup(CvSelectionGroup const& kGroup);
	// Keep the K-Mod function names b/c there are a great many call locations
	void SetSettings(CvSelectionGroup const& kGroup,
			MovementFlags eFlags = NO_MOVEMENT_FLAGS,
			int iMaxPath = -1, int iHeuristicWeight = -1);
	bool GeneratePath(CvPlot const& kTo);
	#ifndef FASSERT_ENABLE // advc.tmp
	inline int GetPathTurns() const
	{
		return getPathLength();
	}
	__forceinline void Reset() { resetNodes(); }
	// <advc.tmp>
	#else
	int GetPathTurns() const
	{
		int r= getPathLength(); FAssert(r==leg.GetPathTurns()); return r;
	}
	inline void Reset() { resetNodes(); leg.Reset(); }
	bool generatePath(int iStartX, int iStartY, int iDestX, int iDestY)
	{
		bool r=KmodPathFinder<GroupStepMetric,GroupPathNode>::generatePath(iStartX,iStartY,iDestX,iDestY);
		FAssert(r==leg.GeneratePath(iStartX,iStartY,iDestX,iDestY));
		return r;
	}
	#endif
	#ifndef FASSERT_ENABLE // </advc.tmp>
	__forceinline CvPlot* GetPathFirstPlot() { return getPathFirstPlot(); }
	// <advc.tmp>
	#else
	CvPlot* GetPathFirstPlot() { CvPlot* r= getPathFirstPlot(); FAssert(r ==leg.GetPathFirstPlot()); return r;}
	#endif // </advc.tmp>
	CvPlot* GetPathEndTurnPlot() const; //tbd.: could return CvPlot&
	__forceinline bool IsPathComplete() { return isPathComplete(); }
	int GetFinalMoves() const
	{
		if (m_pEndNode == NULL)
		{
			FAssert(m_pEndNode != NULL);
			return 0;
		}
		return m_pEndNode->getMoves();
	}
	/*	advc (tbd.): Remove this function so that GroupPathNode is fully encapasulated.
		Cf. comment in CvUnitAI::AI_considerPathDOW (the only call location). */
	GroupPathNode* GetEndNode() const
	{	// Note: the returned pointer becomes invalid if the pathfinder is destroyed.
		FAssert(m_pEndNode != NULL);
		return m_pEndNode;
	}
private:
	static int iAdmissibleBaseWeight;
	static int iAdmissibleScaledWeight;
	// <advc.tmp>
	#ifdef FASSERT_ENABLE
	KmodPathFinderLegacy leg;
	#endif // </advc.tmp>
};

#endif
