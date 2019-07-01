#include "CvGameCoreDLL.h"
#include "CyCity.h"

// dlph.34/advc: Added in order to reduce the size of CyCityInterface1.cpp

void CyCityPythonInterface2(python::class_<CyCity>& x)
{
	OutputDebugString("Python Extension Module - CyCityPythonInterface2\n");

	x	/*  advc: Arbitrarily moved these from CyCityInterface1.cpp so
			that nothing breaks if a few more functions are added there. */
		.def("getBuildingYieldChange", &CyCity::getBuildingYieldChange, "int (int /*BuildingClassTypes*/ eBuildingClass, int /*YieldTypes*/ eYield)")
		.def("setBuildingYieldChange", &CyCity::setBuildingYieldChange, "void (int /*BuildingClassTypes*/ eBuildingClass, int /*YieldTypes*/ eYield, int iChange)")
		.def("getBuildingCommerceChange", &CyCity::getBuildingCommerceChange, "int (int /*BuildingClassTypes*/ eBuildingClass, int /*CommerceTypes*/ eCommerce)")
		.def("setBuildingCommerceChange", &CyCity::setBuildingCommerceChange, "void (int /*BuildingClassTypes*/ eBuildingClass, int /*CommerceTypes*/ eCommerce, int iChange)")
		.def("getBuildingHappyChange", &CyCity::getBuildingHappyChange, "int (int /*BuildingClassTypes*/ eBuildingClass)")
		.def("setBuildingHappyChange", &CyCity::setBuildingHappyChange, "void (int /*BuildingClassTypes*/ eBuildingClass, int iChange)")
		.def("getBuildingHealthChange", &CyCity::getBuildingHealthChange, "int (int /*BuildingClassTypes*/ eBuildingClass)")
		.def("setBuildingHealthChange", &CyCity::setBuildingHealthChange, "void (int /*BuildingClassTypes*/ eBuildingClass, int iChange)")

		.def("getLiberationPlayer", &CyCity::getLiberationPlayer, "int ()")
		.def("liberate", &CyCity::liberate, "void ()")
		;
}
