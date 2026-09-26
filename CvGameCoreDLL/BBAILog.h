#pragma once

#ifndef BBAI_LOG_H
#define BBAI_LOG_H

// AI decision making logging

// Log levels:
// 0 - None
// 1 - Important decisions only
// 2 - Many decisions
// 3 - All logging
// <!-- custom: Base AdvCiv required recompiling the DLL to toggle BBAI logging through LOG_AI / LOG_FOUND_VALUE.
// AdvCiv-SAS exposes the existing log levels through XML defines instead and caches their effective values once after all GlobalDefines/module overrides load.
// Keep the widespread g*LogLevel hot gates as direct reads; call sites should still gate before evaluating logging-only arguments because logBBAI cannot undo argument work already performed by its caller. (GPT-5.5? + ChatGPT-5.6-Sol) -->
struct SASBBAILogSettings
{
	bool bEnabled;
	bool bMasterEnabled;
	int iPlayerLogLevel;
	int iTeamLogLevel;
	int iWarLogLevel;
	int iCityLogLevel;
	int iProductionNoTargetLogLevel;
	int iMilitaryProductionLogLevel;
	int iSpaceProductionLogLevel;
	int iLimitedProjectProductionLogLevel;
	int iBuildingProductionLogLevel;
	int iCitizenLogLevel;
	int iUnitLogLevel;
	int iOverseasTransportLogLevel;
	int iGreatGeneralLogLevel;
	int iSettlerLogLevel;
	int iFoundLogLevel;
	int iEvacuationLogLevel;
	int iWorkerLogLevel;
	int iWorkerSeaLogLevel;
	int iMapLogLevel;
	int iDealCancelLogLevel;
	int iBonusLogLevel;
	int iCultureLogLevel;
};
extern SASBBAILogSettings gSASBBAILogSettings;
void cacheSASBBAILogSettings();
__forceinline bool isSASBBAILogEnabled() { return gSASBBAILogSettings.bEnabled; }
__forceinline bool isSASBBAILogMasterEnabled() { return gSASBBAILogSettings.bMasterEnabled; }
int getSASBBAIScoreLogInterval();
void startSASBBAILogForNewGame(); // <!-- custom: Roll to a new timestamped BBAI diagnostic file before new-game map generation can log. (GPT-5.5 + GPT-5.5) -->
void logSASBBAINewGameStarted(); // <!-- custom: Log complete new-game identification after map and player initialization. (GPT-5.5) -->
void startSASBBAILogForLoadedSave(); // <!-- custom: Roll and identify a loaded save after its complete game state is read. (GPT-5.5) -->
int getSASBBAILogSessionSequence(); // <!-- custom: Let high-volume diagnostic categories discard change-dedup state whenever a new/load BBAI session begins. (GPT-5.6-Sol) -->
#define gLogBBAI (gSASBBAILogSettings.bEnabled) // advc.007: So that BBAI logging can be checked in FAssert; <!-- custom: startup-cached direct read. (ChatGPT-5.6-Sol) -->
#define gPlayerLogLevel (gSASBBAILogSettings.iPlayerLogLevel)
#define gScoreLogInterval getSASBBAIScoreLogInterval() // advc.007: was hardcoded to 25 in CvPlayer::onTurnLogging; only reached when PLAYER logging is enabled.
#define gTeamLogLevel (gSASBBAILogSettings.iTeamLogLevel)
#define gWarLogLevel (gSASBBAILogSettings.iWarLogLevel) // <!-- custom: Separate war-plan and war-target diagnostics from broad TEAM logging. (GPT-5.5) -->
#define gCityLogLevel (gSASBBAILogSettings.iCityLogLevel)
#define gProductionNoTargetLogLevel (gSASBBAILogSettings.iProductionNoTargetLogLevel) // <!-- custom: Dedicated all-city no-production boundary and AI chooser-path diagnostics without broad CITY or military-production logging. See KI#51. (GPT-5.6-Sol) -->
#define gMilitaryProductionLogLevel (gSASBBAILogSettings.iMilitaryProductionLogLevel) // <!-- custom: Dedicated AI city military-production diagnostics without enabling broad CITY logging. (ChatGPT-5.6-Sol) -->
#define gSpaceProductionLogLevel (gSASBBAILogSettings.iSpaceProductionLogLevel) // <!-- custom: Dedicated spaceship-project valuation, selection, and production-switch diagnostics without broad PLAYER/CITY logging. (ChatGPT-5.6-Sol) -->
#define gLimitedProjectProductionLogLevel (gSASBBAILogSettings.iLimitedProjectProductionLogLevel) // <!-- custom: Dedicated one-copy non-spaceship Project continuity/selection diagnostics without broad PLAYER/CITY logging. (ChatGPT-5.6-Sol) -->
#define gBuildingProductionLogLevel (gSASBBAILogSettings.iBuildingProductionLogLevel) // <!-- custom: Dedicated building-vs-unit production diagnostics without broad CITY or military-production logging. (ChatGPT-5.6-Sol) -->
#define gCitizenLogLevel (gSASBBAILogSettings.iCitizenLogLevel) // <!-- custom: Separate citizen-assignment and plot-allocation diagnostics from general CITY logging. (GPT-5.5) -->
#define gUnitLogLevel (gSASBBAILogSettings.iUnitLogLevel)
#define gOverseasTransportLogLevel (gSASBBAILogSettings.iOverseasTransportLogLevel) // <!-- custom: Separate overseas military-cargo and Settler-transport diagnostics from broad CITY and UNIT logging. (GPT-5.6-Sol) -->
#define gGreatGeneralLogLevel (gSASBBAILogSettings.iGreatGeneralLogLevel) // <!-- custom: Separate Great General action, Military Instructor, and Warlord attachment diagnostics from general UNIT logging. (GPT-5.5) -->
#define gSettlerLogLevel (gSASBBAILogSettings.iSettlerLogLevel) // <!-- custom: Separate Settler diagnostics from general UNIT logging so each can be inspected independently. (GPT-5.5) -->
#define gFoundLogLevel (gSASBBAILogSettings.iFoundLogLevel) // advc.031c
#define gEvacuationLogLevel (gSASBBAILogSettings.iEvacuationLogLevel) // <!-- custom: Separate evacuation/retreat diagnostics from general CITY and UNIT logging. (GPT-5.5) -->
#define gWorkerLogLevel (gSASBBAILogSettings.iWorkerLogLevel) // <!-- custom: Separate land-Worker diagnostics from general UNIT logging. (ChatGPT-5.5 + GPT-5.5 review) -->
#define gWorkerSeaLogLevel (gSASBBAILogSettings.iWorkerSeaLogLevel) // <!-- custom: Separate Work Boat / WORKER_SEA diagnostics from general UNIT and land-Worker logging. (ChatGPT-5.5 + GPT-5.5 review) -->
#define gMapLogLevel (gSASBBAILogSettings.iMapLogLevel) // K-Mod
#define gDealCancelLogLevel (gSASBBAILogSettings.iDealCancelLogLevel) // advc.133
#define gBonusLogLevel (gSASBBAILogSettings.iBonusLogLevel) // <!-- custom: Shared bonus valuation, trade, and strategic-resource diagnostics independent of their Worker/Settler consumers. (GPT-5.6-Sol) -->
#define gCultureLogLevel (gSASBBAILogSettings.iCultureLogLevel) // <!-- custom: Separate culture-victory diagnostics from general PLAYER and CITY logging. (ChatGPT-5.5) -->

void logBBAI(TCHAR* format, ... );
// <advc.133>
class CvDeal;
void logBBAICancel(CvDeal const& d, PlayerTypes eCancelPlayer, wchar const* szReason);
// </advc.133>

#endif  //BBAI_LOG_H
