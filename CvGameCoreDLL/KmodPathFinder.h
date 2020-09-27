#pragma once

#ifndef KMOD_PATHFINDER_H
#define KMOD_PATHFINDER_H

class CvSelectionGroup;
class FAStarNode;
class CvMap;
class FAStarNodeMap;

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

class KmodPathFinder
{
public:
	static void InitHeuristicWeights();
	static int MinimumStepCost(int BaseMoves);

	KmodPathFinder();
	~KmodPathFinder();
	void Reset();
	void SetSettings(const CvPathSettings& new_settings);
	inline void SetSettings(CvSelectionGroup const* pGroup,
		MovementFlags eFlags = NO_MOVEMENT_FLAGS,
		int iMaxPath = -1, int iHeuristicWeight = -1)
	{
		SetSettings(CvPathSettings(pGroup, eFlags, iMaxPath, iHeuristicWeight));
	}
	bool GeneratePath(int x1, int y1, int x2, int y2);
	bool GeneratePath(const CvPlot* pToPlot); // just a wrapper for convenience
	bool IsPathComplete() const { return (end_node != NULL); }
	int GetPathTurns() const;
	int GetFinalMoves() const;
	CvPlot* GetPathFirstPlot() const;
	CvPlot* GetPathEndTurnPlot() const;
	/*	advc (tbd.): Get rid of this function so that FAStarNode is fully encapasulated.
		Cf. comment in CvUnitAI::AI_considerPathDOW (the only call location). */
	FAStarNode* GetEndNode() const
	{	// Note: the returned pointer becomes invalid if the pathfinder is destroyed.
		FAssert(end_node != NULL);
		return end_node;
	}

protected:
	void AddStartNode();
	void RecalculateHeuristics();
	bool ProcessNode();
	void ForwardPropagate(FAStarNode* head, int cost_delta);
	// advc.pf: Moved into FAStarNodeMap
	//FAStarNode& GetNode(int x, int y) { return node_data[y * map_width + x]; }
	// advc.pf: Not needed anymore
	//bool ValidateNodeMap(); // (Called when SetSettings is used.)
	typedef std::vector<FAStarNode*> OpenList_t;

	struct OpenList_sortPred
	{
		bool operator()(const FAStarNode* &left, const FAStarNode* &right);
	};

	CvMap const& kMap; // advc.pf
	/*	advc.pf: Wrapper for raw array. Historical note: Before K-Mod 1.45,
		stdext::hash_map<int,boost::shared_ptr<FAStarNode> > had been used. */
	FAStarNodeMap& nodeMap;
	OpenList_t open_list;

	int dest_x, dest_y;
	int start_x, start_y;
	FAStarNode* end_node;
	CvPathSettings settings;

	static int admissible_scaled_weight;
	static int admissible_base_weight;
};

#endif
