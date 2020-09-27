#pragma once
//  *****************   FIRAXIS GAME ENGINE   ********************
//  FILE:    FAStarNode.h
//  AUTHOR:  Casey O'Toole  --  8/21/2002
//  PURPOSE: A* Pathfinding - based off of A* Explorer from "AI Game Programming Wisdom"
//  Copyright (c) 2002 Firaxis Games, Inc. All rights reserved.
#ifndef FASTARNODE_H
#define FASTARNODE_H

#define ASNL_ADDOPEN		0
#define ASNL_STARTOPEN		1
#define ASNL_DELETEOPEN		2
#define ASNL_ADDCLOSED		3

#define ASNC_INITIALADD		0
#define ASNC_OPENADD_UP		1
#define ASNC_CLOSEDADD_UP	2
#define ASNC_PARENTADD_UP	3
#define ASNC_NEWADD			4

enum FAStarListType
{
	NO_FASTARLIST = -1,
	FASTARLIST_OPEN,
	FASTARLIST_CLOSED,
	NUM_FASTARLIST_TYPES
};

// Used by FAStar pathfinding class
class FAStarNode
{
	/*	advc.pf: This gets initialized through memset by the KmodPathFinder and,
		apparently, also by the BtS path finder. To save time, presumably.
		I've removed the constructor to make this clearer. Note that the
		constructor had set m_iX, m_iY and m_eFAStarListType to -1 (not 0). */
	FAStarNode();
public:
	/*FAStarNode()
	{
		m_iX = -1;
		m_iY = -1;
		clear();
	}
	void clear()
	{
		m_eFAStarListType = NO_FASTARLIST;
		m_iTotalCost = 0;
		m_iKnownCost = 0;
		m_iHeuristicCost = 0;
		m_iNumChildren = 0;
		m_iData1 = 0;
		m_iData2 = 0;
		m_bOnStack = false;
		m_pParent = NULL;
		m_pNext = NULL;
		m_pPrev = NULL;
		m_pStack = NULL;
		for (int i = 0; i < 8; i++)
			m_apChildren[i] = NULL;
	}*/

	int m_iX, m_iY;         // Coordinate position
	int m_iTotalCost;		// Fitness (f)
	int m_iKnownCost;		// Goal (g)
	int m_iHeuristicCost;   // Heuristic (h)
	int m_iNumChildren;
	int m_iData1;
	int m_iData2;

	bool m_bOnStack;

	FAStarListType m_eFAStarListType;

	FAStarNode* m_pParent;
	FAStarNode* m_pNext;	// For Open and Closed lists
	FAStarNode* m_pPrev;	// For Open and Closed lists
	FAStarNode* m_pStack;	// For Push/Pop Stack

	FAStarNode* m_apChildren[8];
};
/*	advc: I think the EXE allocates memory for FAStarNodes based on the struct's size
	at the time that the EXE was compiled. I haven't tested it, but I expect that the
	the size needs to remain unchanged. */
BOOST_STATIC_ASSERT(sizeof(FAStarNode) == 88);

// advc.pf: Based on code cut from KmodPathFinder
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

#endif
