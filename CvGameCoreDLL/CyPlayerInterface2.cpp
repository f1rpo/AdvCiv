#include "CvGameCoreDLL.h"
#include "CyPlayer.h"
#include "CvPlayer.h" // advc.enum: for PollutionFlags
#include "CySelectionGroup.h"
#include "CyArea.h"

//
// published python interface for CyPlayer
//

void CyPlayerPythonInterface2(python::class_<CyPlayer>& x)
{
	printToConsole("Python Extension Module - CyPlayerPythonInterface2\n");

	// set the docstring of the current module scope
	python::scope().attr("__doc__") = "Civilization IV Player Class";
	x
		.def("AI_updateFoundValues", &CyPlayer::AI_updateFoundValues, "void (bool bStartingLoc)")
		.def("AI_foundValue", &CyPlayer::AI_foundValue, "int (int, int, int, bool)")
		.def("AI_isFinancialTrouble", &CyPlayer::AI_isFinancialTrouble, "bool ()")
		.def("AI_isWillingToTalk", &CyPlayer::AI_isWillingToTalk, "bool (int /*PlayerTypes*/)") // K-Mod
		.def("AI_demandRebukedWar", &CyPlayer::AI_demandRebukedWar, "bool (int /*PlayerTypes*/)")
		.def("AI_getAttitude", &CyPlayer::AI_getAttitude, "AttitudeTypes (int /*PlayerTypes*/) - Gets the attitude of the player towards the player passed in")
		.def("AI_unitValue", &CyPlayer::AI_unitValue, "int (int /*UnitTypes*/ eUnit, int /*UnitAITypes*/ eUnitAI, CyArea* pArea)")
		.def("AI_civicValue", &CyPlayer::AI_civicValue, "int (int /*CivicTypes*/ eCivic)")
		.def("AI_totalUnitAIs", &CyPlayer::AI_totalUnitAIs, "int (int /*UnitAITypes*/ eUnitAI)")
		.def("AI_totalAreaUnitAIs", &CyPlayer::AI_totalAreaUnitAIs, "int (CyArea* pArea, int /*UnitAITypes*/ eUnitAI)")
		.def("AI_totalWaterAreaUnitAIs", &CyPlayer::AI_totalWaterAreaUnitAIs, "int (CyArea* pArea, int /*UnitAITypes*/ eUnitAI)")
		.def("AI_getNumAIUnits", &CyPlayer::AI_getNumAIUnits, "int (UnitAIType) - Returns # of UnitAITypes the player current has of UnitAIType")
		.def("AI_getAttitudeExtra", &CyPlayer::AI_getAttitudeExtra, "int (int /*PlayerTypes*/ eIndex) - Returns the extra attitude for this player - usually scenario specific")
		.def("AI_setAttitudeExtra", &CyPlayer::AI_setAttitudeExtra, "void (int /*PlayerTypes*/ eIndex, int iNewValue) - Sets the extra attitude for this player - usually scenario specific")
		.def("AI_changeAttitudeExtra", &CyPlayer::AI_changeAttitudeExtra, "void (int /*PlayerTypes*/ eIndex, int iChange) - Changes the extra attitude for this player - usually scenario specific")
		.def("AI_getMemoryCount", &CyPlayer::AI_getMemoryCount, "int (/*PlayerTypes*/ eIndex1, /*MemoryTypes*/ eIndex2)")
		.def("AI_changeMemoryCount", &CyPlayer::AI_changeMemoryCount, "void (/*PlayerTypes*/ eIndex1, /*MemoryTypes*/ eIndex2, int iChange)")
		.def("AI_getExtraGoldTarget", &CyPlayer::AI_getExtraGoldTarget, "int ()")
		.def("AI_setExtraGoldTarget", &CyPlayer::AI_setExtraGoldTarget, "void (int)")

		.def("getScoreHistory", &CyPlayer::getScoreHistory, "int (int iTurn)")
		.def("getEconomyHistory", &CyPlayer::getEconomyHistory, "int (int iTurn)")
		.def("getIndustryHistory", &CyPlayer::getIndustryHistory, "int (int iTurn)")
		.def("getAgricultureHistory", &CyPlayer::getAgricultureHistory, "int (int iTurn)")
		.def("getPowerHistory", &CyPlayer::getPowerHistory, "int (int iTurn)")
		.def("getCultureHistory", &CyPlayer::getCultureHistory, "int (int iTurn)")
		.def("getEspionageHistory", &CyPlayer::getEspionageHistory, "int (int iTurn)")

		.def("getScriptData", &CyPlayer::getScriptData, "str () - Get stored custom data (via pickle)")
		.def("setScriptData", &CyPlayer::setScriptData, "void (str) - Set stored custom data (via pickle)")

		.def("chooseTech", &CyPlayer::chooseTech, "void (int iDiscover, wstring szText, bool bFront)")

		.def("AI_maxGoldTrade", &CyPlayer::AI_maxGoldTrade, "int (int)")
		.def("AI_maxGoldPerTurnTrade", &CyPlayer::AI_maxGoldPerTurnTrade, "int (int)")

		.def("splitEmpire", &CyPlayer::splitEmpire, "bool (int iAreaId)")
		.def("canSplitEmpire", &CyPlayer::canSplitEmpire, "bool ()")
		.def("canSplitArea", &CyPlayer::canSplitArea, "bool (int)")
		.def("canHaveTradeRoutesWith", &CyPlayer::canHaveTradeRoutesWith, "bool (int)")
		.def("forcePeace", &CyPlayer::forcePeace, "void (int)")
		// advc.038:
		.def("estimateYieldRate", &CyPlayer::estimateYieldRate, "int (int /*YieldTypes*/ yield)")
		// advc.210:
		.def("checkAlert", &CyPlayer::checkAlert, "void (int alertId, bool silent)")
		// advc.210e:
		.def("AI_corporationBonusVal", &CyPlayer::AI_corporationBonusVal, "int (int)")
		// <advc.085>
		.def("setScoreboardExpanded", &CyPlayer::setScoreboardExpanded, "void (bool)")
		.def("isScoreboardExpanded", &CyPlayer::isScoreboardExpanded, "bool ()")
		// </advc.085> <advc.190c>
		.def("wasCivRandomlyChosen", &CyPlayer::wasCivRandomlyChosen, "bool ()")
		.def("wasLeaderRandomlyChosen", &CyPlayer::wasLeaderRandomlyChosen, "bool ()")
		// </advc.190c>
		;

	/*	K-Mod, 5/jan/11: pollution flags (advc.enum: Moved from CyEnumsInterface
		b/c it's no longer a global type within the DLL) */
	// (advc.enum: Keep the name "Types" in Python although it's now "Flags" in the DLL)
	python::enum_<int>("PollutionTypes")
		.value("POLLUTION_POPULATION", CvPlayer::POLLUTION_POPULATION)
		.value("POLLUTION_BUILDINGS", CvPlayer::POLLUTION_BUILDINGS)
		.value("POLLUTION_BONUSES", CvPlayer::POLLUTION_BONUSES)
		.value("POLLUTION_POWER", CvPlayer::POLLUTION_POWER)
		.value("POLLUTION_ALL", CvPlayer::POLLUTION_ALL)
		;
}
