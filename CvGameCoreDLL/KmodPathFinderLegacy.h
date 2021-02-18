#pragma once

#ifndef K_MOD_PATH_FINDER_LEGACY_H
#define K_MOD_PATH_FINDER_LEGACY_H

/*	<advc.test> This is the old (AdvCiv 0.98) version of KmodPathFinder.
	For verifying the correctness of the current, restructured version. */
#ifdef FASSERT_ENABLE
	#define VERIFY_PATHF 0
#else
	#define VERIFY_PATHF 0
#endif

#ifdef VERIFY_PATHF // </advc.test>

#include "FAStarNode.h"

class FAStarNodeMap
{
public:
	inline FAStarNodeMap(int iWidth, int iHeight)
	{
		m_data = new byte[iWidth * iHeight * sizeof(FAStarNode)];
		m_iWidth = iWidth;
		m_iHeight = iHeight;
		reset();
	}
	inline ~FAStarNodeMap()
	{
		SAFE_DELETE_ARRAY(m_data);
	}
	inline FAStarNode& get(int iX, int iY)
	{
		return reinterpret_cast<FAStarNode*>(m_data)[iY * m_iWidth + iX];
	}
	inline void reset()
	{
		memset(m_data, 0, sizeof(FAStarNode) * m_iWidth * m_iHeight);
	}
private:
	byte* m_data;
	int m_iWidth, m_iHeight;
};

struct CvPathSettings
{
	CvPathSettings(CvSelectionGroup const* pGroup = NULL,
			MovementFlags eFlags = NO_MOVEMENT_FLAGS,
			int iMaxPath = -1, int iHeuristicWeight = -1)
		: pGroup(pGroup), eFlags(eFlags),
		// advc.opt: Can avoid some branching this way
		iMaxPath(iMaxPath < 0 ? MAX_INT : iMaxPath),
		iHeuristicWeight(iHeuristicWeight)
	{}
	CvSelectionGroup const* pGroup;
	MovementFlags eFlags;
	int iMaxPath;
	int iHeuristicWeight;
};

class KmodPathFinderLegacy
{
public:
	static void InitHeuristicWeights();
	static int MinimumStepCost(int BaseMoves);

	KmodPathFinderLegacy();
	~KmodPathFinderLegacy();
	void Reset();
	inline void SetSettings(CvSelectionGroup const* pGroup,
		MovementFlags eFlags = NO_MOVEMENT_FLAGS,
		int iMaxPath = -1, int iHeuristicWeight = -1)
	{
		SetSettings(CvPathSettings(pGroup, eFlags, iMaxPath, iHeuristicWeight));
	}
	bool GeneratePath(int x1, int y1, int x2, int y2);
	bool GeneratePath(const CvPlot* pToPlot);
	bool IsPathComplete() const { return (end_node != NULL); }
	int GetPathTurns() const;
	int GetFinalMoves() const;
	CvPlot* GetPathFirstPlot() const;
	CvPlot* GetPathEndTurnPlot() const;

protected:
	void AddStartNode();
	void RecalculateHeuristics();
	bool ProcessNode();
	void ForwardPropagate(FAStarNode* head, int cost_delta);
	void SetSettings(CvPathSettings const& new_settings);
	typedef std::vector<FAStarNode*> OpenList_t;

	CvMap const& kMap;
	FAStarNodeMap* nodeMap;
	OpenList_t open_list;

	int dest_x, dest_y;
	int start_x, start_y;
	FAStarNode* end_node;
	CvPathSettings settings;

	static int admissible_base_weight;
	static int admissible_scaled_weight;
};

#endif // advc.test (VERIFY_PATHF)

#endif
