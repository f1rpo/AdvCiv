#include "CvGameCoreDLL.h"
#include "CyGameCoreUtils.h"
// <!-- custom: CvGameCoreUtils declares the existing Win32 UTC sampler used by Python diagnostic timestamps; the adapter below needs its callable declaration. (GPT-6.1-Sol) -->
#include "CvGameCoreUtils.h"

//
// Python interface for CvgameCoreUtils.h.
//

// <!-- custom: Supply current compact UTC (YYYYMMDDTHHMMSS.mmmZ) to Python diagnostics without relying on the embedded Python wall clock, which stayed fixed in the measured game trial.
// SASPythonLogging requires a rebuilt DLL exporting this binding; the bridge supplies reliable wall time, not a measured speed improvement.
// Return std::string for the existing Boost.Python converter rather than exposing the derived CvString type; keep this binding-only adapter private to this file. (GPT-6.1-Sol) -->
static std::string getSASDiagnosticUtcTimestamp()
{
	return createSASUtcTimestampMilliseconds();
}

void CyGameCoreUtilsPythonInterface()
{
	printToConsole("Python Extension Module - CyGameCoreUtilsPythonInterface\n");

	// <!-- custom: Python's optional diagnostic prefixes need a fresh Windows UTC reading even when its embedded time/datetime readings do not advance. (GPT-6.1-Sol) -->
	python::def("getSASDiagnosticUtcTimestamp", getSASDiagnosticUtcTimestamp, "str () - current UTC as YYYYMMDDTHHMMSS.mmmZ");

	python::def("cyIntRange", cyIntRange,"int (int iNum, int iLow, int iHigh)");
	python::def("cyFloatRange", cyFloatRange,"float (float fNum, float fLow, float fHigh)");
	python::def("dxWrap", cyDxWrap,"int (int iDX)");
	python::def("dyWrap", cyDyWrap,"int (int iDY)");
	python::def("plotDistance", cyPlotDistance,"int (int iX1, int iY1, int iX2, int iY2)");
	python::def("stepDistance", cyStepDistance,"int (int iX1, int iY1, int iX2, int iY2)");
	python::def("plotDirection", cyPlotDirection, python::return_value_policy<python::manage_new_object>(), "CyPlot* (int iX, int iY, DirectionTypes eDirection)");
	python::def("plotCardinalDirection", cyPlotCardinalDirection, python::return_value_policy<python::manage_new_object>(), "CyPlot* (int iX, int iY, CardinalDirectionTypes eCardDirection)");
	python::def("splotCardinalDirection", cysPlotCardinalDirection, python::return_value_policy<python::reference_existing_object>(), "CyPlot* (int iX, int iY, CardinalDirectionTypes eCardDirection)");
	python::def("plotXY", cyPlotXY, python::return_value_policy<python::manage_new_object>(), "CyPlot* (int iX, int iY, int iDX, int iDY)");
	python::def("splotXY", cysPlotXY, python::return_value_policy<python::reference_existing_object>(), "CyPlot* (int iX, int iY, int iDX, int iDY)");
	python::def("directionXY", cyDirectionXYFromInt,"DirectionTypes (int iDX, int iDY)");
	python::def("directionXYFromPlot", cyDirectionXYFromPlot,"DirectionTypes (CyPlot* pFromPlot, CyPlot* pToPlot)");
	python::def("plotCity", cyPlotCity, python::return_value_policy<python::manage_new_object>(), "CyPlot* (int iX, int iY, int iIndex)");
	python::def("plotCityXY", cyPlotCityXYFromInt,"int (int iDX, int iDY)");
	python::def("plotCityXYFromCity", cyPlotCityXYFromCity,"int (CyCity* pCity, CyPlot* pPlot)");
	python::def("getOppositeCardinalDirection", cyGetOppositeCardinalDirection,"CardinalDirectionTypes (CardinalDirectionTypes eDir)");
	python::def("cardinalDirectionToDirection", cyCardinalDirectionToDirection, "DirectionTypes (CardinalDirectionTypes eDir) - converts a CardinalDirectionType to the corresponding DirectionType");

	python::def("isCardinalDirection", cyIsCardinalDirection,"bool (DirectionTypes eDirection)");
	python::def("estimateDirection", cyEstimateDirection, "DirectionTypes (int iDX, int iDY)");

	python::def("atWar", cyAtWar,"bool (int eTeamA, int eTeamB)");
	// advc: No longer exposed
	//python::def("isPotentialEnemy", cyIsPotentialEnemy,"bool (int eOurTeam, int eTheirTeam)");

	python::def("getCity", cyGetCity, python::return_value_policy<python::manage_new_object>(), "CyPlot* (IDInfo city)");
	python::def("getUnit", cyGetUnit, python::return_value_policy<python::manage_new_object>(), "CyUnit* (IDInfo unit)");

	/*python::def("getPopulationAsset", cyGetPopulationAsset, "int (int iPopulation)");
	python::def("getLandPlotsAsset", cyGetLandPlotsAsset, "int (int iLandPlots)");
	python::def("getPopulationPower", cyGetPopulationPower, "int (int iPopulation)");
	python::def("getPopulationScore", cyGetPopulationScore, "int (int iPopulation)");
	python::def("getLandPlotsScore", cyGetLandPlotsScore, "int (int iPopulation)");
	python::def("getTechScore", cyGetTechScore, "int (int eTech)");
	python::def("getWonderScore", cyGetWonderScore, "int (int eWonderClass)");*/ // advc: No longer exposed

	python::def("isPromotionValid", cyIsPromotionValid, "bool (int /*PromotionTypes*/ ePromotion, int /*UnitTypes*/ eUnit, bool bLeader)");
	python::def("finalImprovementUpgrade", cyFinalImprovementUpgrade, "int /*ImprovementTypes*/ (int /*ImprovementTypes*/ eImprovement)");

	//python::def("getWorldSizeMaxConscript", cyGetWorldSizeMaxConscript, "int (int /*CivicTypes*/ eCivic)"); // advc: No longer exposed
	python::def("isReligionTech", cyIsReligionTech, "int (int /*TechTypes*/ eTech)");

	python::def("isTechRequiredForUnit", cyIsTechRequiredForUnit, "bool (int /*TechTypes*/ eTech, int /*UnitTypes*/ eUnit)");
	python::def("isTechRequiredForBuilding", cyIsTechRequiredForBuilding, "bool (int /*TechTypes*/ eTech, int /*BuildingTypes*/ eBuilding)");
	python::def("isTechRequiredForProject", cyIsTechRequiredForProject, "bool (int /*TechTypes*/ eTech, int /*ProjectTypes*/ eProject)");
	python::def("isWorldUnitClass", cyIsWorldUnitClass, "bool (int /*UnitClassTypes*/ eUnitClass)");
	python::def("isTeamUnitClass", cyIsTeamUnitClass, "bool (int /*UnitClassTypes*/ eUnitClass)");
	python::def("isNationalUnitClass", cyIsNationalUnitClass, "bool (int /*UnitClassTypes*/ eUnitClass)");
	python::def("isLimitedUnitClass", cyIsLimitedUnitClass, "bool (int /*UnitClassTypes*/ eUnitClass)");
	python::def("isWorldWonderClass", cyIsWorldWonderClass, "bool (int /*BuildingClassTypes*/ eBuildingClass)");
	python::def("isTeamWonderClass", cyIsTeamWonderClass, "bool (int /*BuildingClassTypes*/ eBuildingClass)");
	python::def("isNationalWonderClass", cyIsNationalWonderClass, "bool (int /*BuildingClassTypes*/ eBuildingClass)");
	python::def("isLimitedWonderClass", cyIsLimitedWonderClass, "bool (int /*BuildingClassTypes*/ eBuildingClass)");
	python::def("isWorldProject", cyIsWorldProject, "bool (int /*ProjectTypes*/ eProject)");
	python::def("isTeamProject", cyIsTeamProject, "bool (int /*ProjectTypes*/ eProject)");
	python::def("isLimitedProject", cyIsLimitedProject, "bool (int /*ProjectTypes*/ eProject)");
	python::def("getCombatOdds", cyGetCombatOdds, "int (CyUnit* pAttacker, CyUnit* pDefender)");
	python::def("getEspionageModifier", cyGetEspionageModifier, "int (int /*TeamTypes*/ iOurTeam, int /*TeamTypes*/ iTargetTeam)");

	python::def("cyUpdatePlotIndicatorSize", cyUpdatePlotIndicatorSize, "void (void)"); // advc.092b
}
