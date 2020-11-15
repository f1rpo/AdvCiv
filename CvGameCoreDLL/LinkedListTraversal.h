#pragma once

#ifndef LINKED_LIST_TRAVERSAL_H
#define LINKED_LIST_TRAVERSAL_H

/*	advc.003s: Macros for traversing certain kinds of CLinkList (LinkedList.h)
	See FreeListTraversal.h for further comments (similar approach).
	advc.003u: Variants that provide a pointer to an AI object */

#define ORIGINALLISTLENGTHNAME CONCATVARNAME(pAnonOrigListLength_, __LINE__)
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
	#define DECL_ORIGINAL_LIST_LEN(kUnitListWrapper) \
		int ORIGINALLISTLENGTHNAME = (kUnitListWrapper).getNumUnits()
#else
	#define ASSERT_UNIT_LIST_LENGTH(kUnitListWrapper, iLength) 0
	#define DECL_ORIGINAL_LIST_LEN(kUnitListWrapper)
#endif


#define LISTNODENAME CONCATVARNAME(pAnonListNode_, __LINE__)

#define FOR_EACH_UNIT_IN_HELPER(pUnit, kUnitListWrapper, CvUnitType, getUnitGlobalFunc) \
	DECL_ORIGINAL_LIST_LEN(kUnitListWrapper); \
	CLLNode<IDInfo> const* LISTNODENAME = (kUnitListWrapper).headUnitNode(); \
	/* Really want to declare pUnit in the header to avoid name clashes */ \
	for (CvUnitType const* pUnit; \
		LISTNODENAME != NULL \
		/* I think this is the only way to avoid making a 2nd NULL check */ \
		&& ((pUnit = ::getUnitGlobalFunc(LISTNODENAME->m_data)) \
		/* Only care about the side-effect of pUnit=... Don't want to branch on it. */ \
		, true); \
		ASSERT_UNIT_LIST_LENGTH(kUnitListWrapper, ORIGINALLISTLENGTHNAME), \
		LISTNODENAME = (kUnitListWrapper).nextUnitNode(LISTNODENAME))

#define FOR_EACH_UNIT_VAR_IN_HELPER(pUnit, kUnitListWrapper, CvUnitType, getUnitGlobalFunc) \
	CLLNode<IDInfo>* LISTNODENAME = (kUnitListWrapper).headUnitNode(); \
	for (CvUnitType* pUnit; \
		LISTNODENAME != NULL && \
		((pUnit = ::getUnitGlobalFunc(LISTNODENAME->m_data)), true) && \
		/* Update the node right after the termination check */ \
		/* so that it's safe to delete pUnit and its node in the loop body. */ \
		((LISTNODENAME = (kUnitListWrapper).nextUnitNode(LISTNODENAME)), true); \
		)

/*	kUnitListWrapper can be of type CvPlot or CvSelectionGroup
	(or whichever other class has the proper interface). Will require either the
	CvPlot or CvSelectionGroup header and, in any case, the CvPlayer header for
	looking up the unit IDInfo. */
#define FOR_EACH_UNIT_IN(pUnit, kUnitListWrapper) \
	FOR_EACH_UNIT_IN_HELPER(pUnit, kUnitListWrapper, CvUnit, getUnit)
#define FOR_EACH_UNITAI_IN(pUnit, kUnitListWrapper) \
	FOR_EACH_UNIT_IN_HELPER(pUnit, kUnitListWrapper, CvUnitAI, AI_getUnit)

#define FOR_EACH_UNIT_VAR_IN(pUnit, kUnitListWrapper) \
	FOR_EACH_UNIT_VAR_IN_HELPER(pUnit, kUnitListWrapper, CvUnit, getUnit)
#define FOR_EACH_UNITAI_VAR_IN(pUnit, kUnitListWrapper) \
	FOR_EACH_UNIT_VAR_IN_HELPER(pUnit, kUnitListWrapper, CvUnitAI, AI_getUnit)

// itemList is of type CLinkList<TradeData>
#define FOR_EACH_TRADE_ITEM(pItemVar, itemList) \
	CLLNode<TradeData> const* LISTNODENAME; \
	for (TradeData const* pItemVar = \
		((LISTNODENAME = (itemList).head()), NULL); \
		LISTNODENAME != NULL \
		&& ((pItemVar = &LISTNODENAME->m_data), true); \
		LISTNODENAME = (itemList).next(LISTNODENAME))

#endif
