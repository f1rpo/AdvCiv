// advc.enum: From "We the People" (Nightinggale); see EnumMap.h.

#include "CvGameCoreDLL.h"

//#define _TEST_ENUM_MAP // advc: Uncomment to enable the tests
//#define _TEST_SPARSE_ENUM_MAP // advc.enum

void TestEnumMap()
{
	// advc: Moved into the function body
	#if /*defined(_DEBUG) &&*/ defined(_TEST_ENUM_MAP)
	{
		EnumMap<RouteTypes, int> test;
		RouteTypes var = static_cast<RouteTypes>(0);

		FAssert(test.get(var) == 0);
		FAssert(!test.hasContent());
		FAssert(!test.isAllocated());
		test.set(var, 1);
		FAssert(test.isAllocated());
		FAssert(test.get(var) == 1);
		FAssert(test.hasContent());
		test.set(var, 0);
		FAssert(test.get(var) == 0);
		FAssert(test.isAllocated());
		FAssert(!test.hasContent());
		test.set(var, 1);
		test.reset();
		FAssert(test.get(var) == 0);
		FAssert(!test.isAllocated());
		FAssert(!test.hasContent());
	}

	{	// advc: Changed this test from RouteTypes to YieldTypes
		EnumMap<YieldTypes, bool> test;
		YieldTypes var = static_cast<YieldTypes>(0);

		FAssert(!test.get(var));
		FAssert(!test.hasContent());
		test.set(var, true);
		FAssert(test.get(var));
		FAssert(test.hasContent());
		test.set(var, false);
		FAssert(!test.get(var));
		FAssert(!test.hasContent());
		test.set(var, true);
		test.reset();
		FAssert(!test.get(var));
		FAssert(!test.hasContent());
	}

	{
		EnumMap<RouteTypes, PlayerTypes> test;
		RouteTypes var = static_cast<RouteTypes>(0);

		FAssert(test.get(var) == NO_PLAYER);
		FAssert(!test.hasContent());
		test.set(var, FIRST_PLAYER);
		FAssert(test.get(var) == FIRST_PLAYER);
		FAssert(test.hasContent());
		test.set(var, NO_PLAYER);
		FAssert(test.get(var) == NO_PLAYER);
		FAssert(!test.hasContent());
		test.set(var, FIRST_PLAYER);
		test.reset();
		FAssert(test.get(var) == NO_PLAYER);
		FAssert(!test.hasContent());
	}
	#endif
	#if defined(_TEST_SPARSE_ENUM_MAP)
	{
		int count = 0; // to prevent optimizer from discarding unused calls
		SparseEnumMap<PlayerTypes,char> test;
		test.set((PlayerTypes)3,3);
		test.set((PlayerTypes)3,4);
		char c = test.get((PlayerTypes)3);
		FAssert(c == 4);
		count += c;
		test.add((PlayerTypes)3,2);
		FAssert(test.get((PlayerTypes)3) == 6);
		test.add((PlayerTypes)1,-1);
		FAssert(test.get((PlayerTypes)1) == -1);
		std::vector<PlayerTypes> testKeys;
		std::vector<int> testVals;
		FOR_EACH_NON_DEFAULT_PAIR(test, Player, int)
		{
			testKeys.push_back(perPlayerVal.first);
			testVals.push_back(perPlayerVal.second);
		}
		FAssert(testKeys[0] == 1);
		FAssert(testKeys[1] == 3);
		FAssert(testVals[0] == -1);
		FAssert(testVals[1] == 6);
		test.add((PlayerTypes)3,-6);
		test.set((PlayerTypes)1,0);
		FAssert(!test.isAnyNonDefault());
		test.add((PlayerTypes)3,-6);
		FAssert(test.isAnyNonDefault());
		test.reset();
		FAssert(!test.isAnyNonDefault());
		test.set((PlayerTypes)3,3);
		test.set((PlayerTypes)7,7);
		// insert in middle
		test.set((PlayerTypes)5,5);
		test.set((PlayerTypes)4,4);
		test.set((PlayerTypes)6,6);
		if (count == -362728) // not going to happen
		{
			FAssert(false);
			exit(0);
		}
	}
	{
		SparseEnumMap<PlayerTypes,ImprovementTypes> test;
		test.set((PlayerTypes)0,(ImprovementTypes)0);
		FAssert(test.isAnyNonDefault());
	}
	{
		SparseEnumMap2D<PlayerTypes,ImprovementTypes,char> test;
		test.set((PlayerTypes)0,(ImprovementTypes)0, 0);
		FAssert(!test.isAnyNonDefault());
	}
	{
		SparseEnumMap2D<PlayerTypes,DomainTypes,short> test;
		test.set((PlayerTypes)3,(DomainTypes)0,3);
		test.set((PlayerTypes)3,(DomainTypes)1,4);
		FAssert(test.get((PlayerTypes)3,(DomainTypes)1) == 4);
		test.add((PlayerTypes)3,(DomainTypes)0, 2);
		FAssert(test.get((PlayerTypes)3, (DomainTypes)0) == 5);
		test.set((PlayerTypes)4,(DomainTypes)2,1);
		test.add((PlayerTypes)3,(DomainTypes)0,-5);
		test.set((PlayerTypes)4,(DomainTypes)2,0);
		FAssert(test.isAnyNonDefault());
		test.set((PlayerTypes)3,(DomainTypes)1,0);
		FAssert(!test.isAnyNonDefault());
		test.reset();
		FAssert(!test.isAnyNonDefault());
		test.set((PlayerTypes)3,(DomainTypes)0,3);
		test.set((PlayerTypes)5,(DomainTypes)0,5);
		test.set((PlayerTypes)4,(DomainTypes)0,4);
	}
	#endif
}
