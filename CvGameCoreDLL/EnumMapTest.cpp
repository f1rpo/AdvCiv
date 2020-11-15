// advc.enum: From "We the People" (Nightinggale); see EnumMap.h.

#include "CvGameCoreDLL.h"

//#define _TEST_ENUM_MAP // advc: Uncomment to enable the tests

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
}
