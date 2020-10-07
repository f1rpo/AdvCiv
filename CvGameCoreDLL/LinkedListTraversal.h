#pragma once

#ifndef LINKED_LIST_TRAVERSAL_H
#define LINKED_LIST_TRAVERSAL_H

/*	advc.003s: Macros for traversing certain kinds of CLinkList (LinkedList.h)
	See FreeListTraversal.h for further comments (similar approach).
	advc.003u: Variants that provide a pointer to an AI object */

#define pANON_ORIG_LIST_LENGTH CONCATVARNAME(pAnonOrigListLength_, __LINE__)
/*	Accidental changes to a list during traversal can happen pretty easily.
	That said, I don't think such errors are usually difficult to debug;
	so this assertion is perhaps overkill (even in debug builds). */
#if 0//#ifdef _DEBUG
	#define ASSERT_UNIT_LIST_LENGTH(kUnitListWrapper, iLength) \
		assertUnitListLength((kUnitListWrapper).getNumUnits(), iLength)
	void assertUnitListLength(int iActualLength, int iDesiredLength)
	{	// FAssertMsg(...) is not an expression; hence this wrapper function.
		FAssertMsg(iActualLength == iDesiredLength,
				"List length has changed during FOR_EACH_UNIT_IN traversal");
	}
	#define SET_ORIGINAL_LIST_LEN(kUnitListWrapper) \
		int pANON_ORIG_LIST_LENGTH = (kUnitListWrapper).getNumUnits()
#else
	#define ASSERT_UNIT_LIST_LENGTH(kUnitListWrapper, iLength) 0
	#define SET_ORIGINAL_LIST_LEN(kUnitListWrapper)
#endif


#define pANON_LIST_NODE CONCATVARNAME(pAnonListNode_, __LINE__)

// const
#define FOR_EACH_UNIT_IN_HELPER(pUnit, kUnitListWrapper, CvUnitType, getUnitGlobalFunc) \
	SET_ORIGINAL_LIST_LEN(kUnitListWrapper); \
	CLLNode<IDInfo> const* pANON_LIST_NODE = (kUnitListWrapper).headUnitNode(); \
	for (CvUnitType const* pUnit; \
		pANON_LIST_NODE != NULL \
		/* I think this is the only way to avoid making a 2nd NULL check */ \
		&& ((pUnit = ::getUnitGlobalFunc(pANON_LIST_NODE->m_data)) \
		/* Only care about the side-effect of pUnit=... Don't want to branch on it. */ \
		, true); \
		ASSERT_UNIT_LIST_LENGTH(kUnitListWrapper, pANON_ORIG_LIST_LENGTH), \
		pANON_LIST_NODE = (kUnitListWrapper).nextUnitNode(pANON_LIST_NODE))

// non-const (i.e. VARiable)
#define FOR_EACH_UNIT_VAR_IN_HELPER(pUnit, kUnitListWrapper, CvUnitType, getUnitGlobalFunc) \
	CLLNode<IDInfo>* pANON_LIST_NODE = (kUnitListWrapper).headUnitNode(); \
	for (CvUnitType* pUnit; \
		pANON_LIST_NODE != NULL && \
		((pUnit = ::getUnitGlobalFunc(pANON_LIST_NODE->m_data)), true) && \
		/* Update the node right after the termination check */ \
		/* so that it's safe to delete pUnit and its node in the loop body. */ \
		((pANON_LIST_NODE = (kUnitListWrapper).nextUnitNode(pANON_LIST_NODE)), true); \
		)

/*	kUnitListWrapper can be of type CvPlot or CvSelectionGroup
	(or whichever other class has the proper interface). Will require either the
	CvPlot or CvSelectionGroup header and, in any case, the CvPlayer header for
	looking up the unit's IDInfo. */
#define FOR_EACH_UNIT_IN(pUnit, kUnitListWrapper) \
	FOR_EACH_UNIT_IN_HELPER(pUnit, kUnitListWrapper, CvUnit, getUnit)
#define FOR_EACH_UNITAI_IN(pUnit, kUnitListWrapper) \
	FOR_EACH_UNIT_IN_HELPER(pUnit, kUnitListWrapper, CvUnitAI, AI_getUnit)

#define FOR_EACH_UNIT_VAR_IN(pUnit, kUnitListWrapper) \
	FOR_EACH_UNIT_VAR_IN_HELPER(pUnit, kUnitListWrapper, CvUnit, getUnit)
#define FOR_EACH_UNITAI_VAR_IN(pUnit, kUnitListWrapper) \
	FOR_EACH_UNIT_VAR_IN_HELPER(pUnit, kUnitListWrapper, CvUnitAI, AI_getUnit)

// kItemList is of type CLinkList<TradeData>
#define FOR_EACH_TRADE_ITEM(pItemVar, kItemList) \
	CLLNode<TradeData> const* pANON_LIST_NODE; \
	for (TradeData const* pItemVar = \
		((pANON_LIST_NODE = (kItemList).head()), NULL); \
		pANON_LIST_NODE != NULL \
		&& ((pItemVar = &pANON_LIST_NODE->m_data), true); \
		pANON_LIST_NODE = (kItemList).next(pANON_LIST_NODE))

#endif
