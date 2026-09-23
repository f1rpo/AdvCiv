#include "CvGameCoreDLL.h"
#include "BBAILog.h"
// <advc.133>
#include "CvGameTextMgr.h"
#include "CvGamePlay.h" // </advc.133>
#include "CvInfo_GameOption.h" // <!-- custom: Needed to log enabled game-option type names; CvGlobals only forward-declares CvGameOptionInfo. (GPT-5.5) -->
#include "CvMap.h" // <!-- custom: Needed to log map dimensions; CvGlobals only forward-declares CvMap. (GPT-5.5) -->
#include "CvGameCoreUtils.h" // <!-- custom: Needed for the shared DLL-process UTC identity used by BBAI and SASGameRecord. See KI#629. (GPT-5.6-Sol) -->

// AI decision making logging

static int getClampedSASBBAILogLevel(char const* szDefineName)
{
	const int iLevel = GC.getDefineINT(szDefineName);
	if (iLevel < 0)
		return 0;
	if (iLevel > 3)
		return 3;
	return iLevel;
}

// <!-- custom: Zero initialization keeps BBAI disabled during DLL/XML startup. CvXMLLoadUtility::SetGlobalDefines fills this once after every base/SAS/modular override has loaded, so the hundreds of category gates in ordinary gameplay are direct field reads rather than out-of-line cached-getter calls.
// Runtime Define mutation was already unsupported because the old getters cached their first resolved values. (ChatGPT-5.6-Sol) -->
SASBBAILogSettings gSASBBAILogSettings;

void cacheSASBBAILogSettings()
{
	SASBBAILogSettings& kSettings = gSASBBAILogSettings;
	bool const bMasterEnabled = (GC.getDefineINT("SAS_BBAI_LOG_ENABLE") > 0);
	kSettings.bMasterEnabled = bMasterEnabled;
	kSettings.iPlayerLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_PLAYER_LOG_LEVEL") : 0);
	kSettings.iTeamLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_TEAM_LOG_LEVEL") : 0);
	kSettings.iWarLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_WAR_LOG_LEVEL") : 0);
	kSettings.iCityLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_CITY_LOG_LEVEL") : 0);
	kSettings.iProductionNoTargetLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_PRODUCTION_NO_TARGET_LOG_LEVEL") : 0);
	kSettings.iMilitaryProductionLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_MILITARY_PRODUCTION_LOG_LEVEL") : 0);
	kSettings.iSpaceProductionLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_SPACE_PRODUCTION_LOG_LEVEL") : 0);
	kSettings.iLimitedProjectProductionLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_LIMITED_PROJECT_PRODUCTION_LOG_LEVEL") : 0);
	kSettings.iBuildingProductionLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_BUILDING_PRODUCTION_LOG_LEVEL") : 0);
	kSettings.iCitizenLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_CITIZEN_LOG_LEVEL") : 0);
	kSettings.iUnitLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_UNIT_LOG_LEVEL") : 0);
	kSettings.iOverseasTransportLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_OVERSEAS_TRANSPORT_LOG_LEVEL") : 0);
	kSettings.iGreatGeneralLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_GREAT_GENERAL_LOG_LEVEL") : 0);
	kSettings.iSettlerLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_SETTLER_LOG_LEVEL") : 0);
	kSettings.iFoundLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_FOUND_LOG_LEVEL") : 0);
	kSettings.iEvacuationLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_EVACUATION_LOG_LEVEL") : 0);
	kSettings.iWorkerLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_WORKER_LOG_LEVEL") : 0);
	kSettings.iWorkerSeaLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_WORKER_SEA_LOG_LEVEL") : 0);
	kSettings.iMapLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_MAP_LOG_LEVEL") : 0);
	kSettings.iDealCancelLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_DEAL_CANCEL_LOG_LEVEL") : 0);
	kSettings.iCultureLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel("SAS_BBAI_CULTURE_LOG_LEVEL") : 0);
	kSettings.bEnabled = (bMasterEnabled &&
		(kSettings.iPlayerLogLevel > 0 || kSettings.iTeamLogLevel > 0 || kSettings.iWarLogLevel > 0 || kSettings.iCityLogLevel > 0 || kSettings.iProductionNoTargetLogLevel > 0 || kSettings.iMilitaryProductionLogLevel > 0 || kSettings.iSpaceProductionLogLevel > 0 || kSettings.iLimitedProjectProductionLogLevel > 0 || kSettings.iBuildingProductionLogLevel > 0 || kSettings.iCitizenLogLevel > 0 ||
		kSettings.iUnitLogLevel > 0 || kSettings.iOverseasTransportLogLevel > 0 || kSettings.iGreatGeneralLogLevel > 0 || kSettings.iSettlerLogLevel > 0 || kSettings.iFoundLogLevel > 0 || kSettings.iEvacuationLogLevel > 0 || kSettings.iWorkerLogLevel > 0 || kSettings.iWorkerSeaLogLevel > 0 || kSettings.iMapLogLevel > 0 || kSettings.iDealCancelLogLevel > 0 || kSettings.iCultureLogLevel > 0));
}

int getSASBBAIScoreLogInterval()
{
	// <!-- custom: This is reached only inside enabled PLAYER logging; keep the modulo divisor at least 1. (GPT-5.5? + ChatGPT-5.6-Sol) -->
	static const int iInterval = (isSASBBAILogMasterEnabled() ? std::max(1, GC.getDefineINT("SAS_BBAI_SCORE_LOG_INTERVAL_TURNS_UNSCALED_GAMESPEED")) : 1);
	return iInterval;
}

static CvString g_szSASBBAILogTimestamp;
static int g_iSASBBAILogSequence = 0;
static CvString g_szSASBBAILogContext;

static CvString getSASBBAILogTimestamp()
{
	if (g_szSASBBAILogTimestamp.empty())
		g_szSASBBAILogTimestamp = createSASUtcTimestamp();
	return g_szSASBBAILogTimestamp;
}

static bool isSASBBAILogTimestampedFilenameEnabled()
{
	static const bool bUseTimestampedFilename = (GC.getDefineINT("SAS_BBAI_LOG_USE_TIMESTAMPED_FILENAME") > 0);
	return bUseTimestampedFilename;
}

// <!-- custom: When timestamped filenames are enabled, use a new file for every new game and loaded save.
// The new/load context and shared chronological sequence keep files distinct and show their order even when multiple new/load actions occur within the same UTC second. (GPT-5.5) -->
static CvString getSASBBAILogName()
{
	return getSASDiagnosticLogName("BBAI", getSASBBAILogTimestamp(), g_szSASBBAILogContext, isSASBBAILogTimestampedFilenameEnabled());
}

static void rollSASBBAILog(const char* szContext)
{
	// <!-- custom: Refresh session identity for both filename modes; fixed-name logs intentionally share a file, but each new/load header still needs its own current UTC. See KI#629. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	g_szSASBBAILogTimestamp = createSASUtcTimestamp();
	if (isSASBBAILogTimestampedFilenameEnabled())
	{
		g_iSASBBAILogSequence++;
		g_szSASBBAILogContext.Format("%s%d", szContext, g_iSASBBAILogSequence);
	}
}

static void logSASBBAIGameState(const char* szRowType)
{
	CvGame& kGame = GC.getGame();
	CvInitCore const& kInitCore = GC.getInitCore();
	const PlayerTypes eActivePlayer = kGame.getActivePlayer();
	const char* szActiveCivilization = "-";
	const char* szActiveHandicap = "-";
	if (eActivePlayer != NO_PLAYER)
	{
		CvPlayer const& kActivePlayer = GET_PLAYER(eActivePlayer);
		if (kActivePlayer.getCivilizationType() != NO_CIVILIZATION)
			szActiveCivilization = GC.getInfo(kActivePlayer.getCivilizationType()).getType();
		if (kActivePlayer.getHandicapType() != NO_HANDICAP)
			szActiveHandicap = GC.getInfo(kActivePlayer.getHandicapType()).getType();
	}
	CvString szGameOptions;
	FOR_EACH_ENUM(GameOption)
	{
		if (!kGame.isOption(eLoopGameOption))
			continue;
		if (!szGameOptions.empty())
			szGameOptions += ",";
		szGameOptions += GC.getInfo(eLoopGameOption).getType();
	}
	if (szGameOptions.empty())
		szGameOptions = "-";
	const CvString szLogName = getSASBBAILogName();
	logBBAI("%s processUtc=%s utc=%s logFile=%s turn=%d elapsed=%d year=%d scenario=%d activePlayer=%d activeCivilization=%s activeHandicap=%s playersDefined=%d playersAlive=%d playersEverAlive=%d humans=%d",
		szRowType, getSASProcessUtcTimestamp().GetCString(), getSASBBAILogTimestamp().GetCString(),
		getSASDiagnosticQuoted(szLogName.GetCString()).GetCString(), kGame.getGameTurn(), kGame.getElapsedGameTurns(),
		kGame.getGameTurnYear(), kGame.isScenario(), eActivePlayer, szActiveCivilization, szActiveHandicap, kInitCore.getNumDefinedPlayers(),
		kGame.countCivPlayersAlive(), kGame.countCivPlayersEverAlive(), kGame.getNumHumanPlayers());
	// <!-- custom: Log the actual cached DLL map classification rather than requiring tests to infer it from the map-script name. (GPT-5.5) -->
	logBBAI("BBAI_GAME_SETTINGS mapScript=%S map=%dx%d landHeavy=%d navalHeavy=%d world=%s climate=%s seaLevel=%s gameSpeed=%s startEra=%s gameHandicap=%s options=%s",
		kInitCore.getMapScriptName().GetCString(), GC.getMap().getGridWidth(), GC.getMap().getGridHeight(), kGame.isLandHeavyMapnameCached(),
		kGame.isNavalHeavyMapnameCached(), GC.getInfo(kInitCore.getWorldSize()).getType(), GC.getInfo(kInitCore.getClimate()).getType(),
		GC.getInfo(kInitCore.getSeaLevel()).getType(), GC.getInfo(kGame.getGameSpeedType()).getType(),
		GC.getInfo(kGame.getStartEra()).getType(), GC.getInfo(kGame.getHandicapType()).getType(), szGameOptions.GetCString());
	logBBAI("BBAI_GAME_RNG mapRandState=%u syncRandState=%u", kGame.getMapRand().getSeed(), kGame.getSorenRand().getSeed());
}

// <!-- custom: Static mod/source/binary provenance is shared with SASGameRecord so AI-decision logs can be tied to the exact source and candidate DLL that produced them.
// The common helpers also keep quoting and field semantics identical between logs. (ChatGPT-5.6-Sol) -->
static void logSASBBAIProvenanceContext()
{
	if (!isSASBBAILogEnabled())
		return;
	logBBAI("BBAI_MOD_CONTEXT %s", getSASModContextFields().GetCString());
	logBBAI("BBAI_SOURCE_CONTEXT %s", getSASSourceContextFields().GetCString());
	logBBAI("BBAI_DLL_CONTEXT %s", getSASDllContextFields().GetCString());
}

// <!-- custom: Record the effective BBAI diagnostic profile in each new/load file so test runs with different category levels are not compared as if they contained the same diagnostics. (GPT-5.5) -->
static void logSASBBAILogSettings()
{
	logBBAI("BBAI_LOG_SETTINGS SAS_BBAI_LOG_ENABLE=%d SAS_BBAI_LOG_USE_TIMESTAMPED_FILENAME=%d SAS_BBAI_PLAYER_LOG_LEVEL=%d SAS_BBAI_TEAM_LOG_LEVEL=%d SAS_BBAI_WAR_LOG_LEVEL=%d SAS_BBAI_CITY_LOG_LEVEL=%d SAS_BBAI_PRODUCTION_NO_TARGET_LOG_LEVEL=%d SAS_BBAI_MILITARY_PRODUCTION_LOG_LEVEL=%d SAS_BBAI_SPACE_PRODUCTION_LOG_LEVEL=%d SAS_BBAI_LIMITED_PROJECT_PRODUCTION_LOG_LEVEL=%d SAS_BBAI_BUILDING_PRODUCTION_LOG_LEVEL=%d SAS_BBAI_CITIZEN_LOG_LEVEL=%d SAS_BBAI_UNIT_LOG_LEVEL=%d SAS_BBAI_OVERSEAS_TRANSPORT_LOG_LEVEL=%d SAS_BBAI_GREAT_GENERAL_LOG_LEVEL=%d SAS_BBAI_SETTLER_LOG_LEVEL=%d SAS_BBAI_FOUND_LOG_LEVEL=%d SAS_BBAI_EVACUATION_LOG_LEVEL=%d SAS_BBAI_WORKER_LOG_LEVEL=%d SAS_BBAI_WORKER_SEA_LOG_LEVEL=%d SAS_BBAI_MAP_LOG_LEVEL=%d SAS_BBAI_DEAL_CANCEL_LOG_LEVEL=%d SAS_BBAI_CULTURE_LOG_LEVEL=%d SAS_BBAI_SCORE_LOG_INTERVAL_TURNS_UNSCALED_GAMESPEED=%d",
		isSASBBAILogMasterEnabled(), isSASBBAILogTimestampedFilenameEnabled(), gPlayerLogLevel, gTeamLogLevel, gWarLogLevel, gCityLogLevel,
		gProductionNoTargetLogLevel, gMilitaryProductionLogLevel, gSpaceProductionLogLevel, gLimitedProjectProductionLogLevel,
		gBuildingProductionLogLevel, gCitizenLogLevel, gUnitLogLevel, gOverseasTransportLogLevel, gGreatGeneralLogLevel, gSettlerLogLevel,
		gFoundLogLevel, gEvacuationLogLevel, gWorkerLogLevel, gWorkerSeaLogLevel, gMapLogLevel, gDealCancelLogLevel, gCultureLogLevel,
		gScoreLogInterval);
}

// <!-- custom: Replace setup-time tech/diplomacy construction chatter with one authoritative finalized state shared with SASGameRecord.
// Preserve the inherited TEAM/WAR detail boundaries while making the successful-start representation compact and deterministic. (ChatGPT-5.6-Sol) -->
static void logSASBBAIInitialState()
{
	int const iTeamLogLevel = gTeamLogLevel;
	int const iWarLogLevel = gWarLogLevel;
	if (iTeamLogLevel < 1 && iWarLogLevel < 1)
		return;
	bool const bDealDetailEnabled = (iTeamLogLevel >= 2 || iWarLogLevel >= 2);
	int iTeamStateRows = 0;
	int iTechRows = 0;
	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		TeamTypes const eTeam = (TeamTypes)iI;
		if (!GET_TEAM(eTeam).isEverAlive())
			continue;
		logBBAI("BBAI_INITIAL_TEAM_STATE %s", getSASInitialTeamStateFields(eTeam).GetCString());
		iTeamStateRows++;
		if (iTeamLogLevel >= 2)
		{
			logBBAI("BBAI_INITIAL_TEAM_TECHS %s", getSASInitialTeamTechFields(eTeam).GetCString());
			iTechRows++;
		}
	}
	int iLoggedDealRows = 0;
	if (bDealDetailEnabled)
	{
		int iLoop = 0;
		for (CvDeal const* pDeal = GC.getGame().firstDeal(&iLoop); pDeal != NULL; pDeal = GC.getGame().nextDeal(&iLoop))
		{
			if (isSASCollapsibleAdvancedStartPeaceDeal(*pDeal))
				continue;
			logBBAI("BBAI_INITIAL_DEAL %s", getSASInitialDealStateFields(*pDeal).GetCString());
			iLoggedDealRows++;
		}
	}
	logBBAI("BBAI_INITIAL_STATE_SUMMARY teamStateRows=%d techRows=%d %s source=FINALIZED_STATE",
		iTeamStateRows, iTechRows, getSASInitialDealSummaryFields(bDealDetailEnabled, iLoggedDealRows).GetCString());
}

// <!-- custom: Roll over before new-game initialization can emit map-generation or starting-position diagnostics.
// The complete metadata is logged later from CvEventReporter::gameStart, once the generated game state exists. (GPT-5.5) -->
void startSASBBAILogForNewGame()
{
	rollSASBBAILog("new");
	logBBAI("BBAI_NEW_GAME_INITIALIZING processUtc=%s utc=%s logFile=%s",
		getSASProcessUtcTimestamp().GetCString(), getSASBBAILogTimestamp().GetCString(),
		getSASDiagnosticQuoted(getSASBBAILogName().GetCString()).GetCString());
	logSASBBAIProvenanceContext();
	logSASBBAILogSettings();
}

void logSASBBAINewGameStarted()
{
	logSASBBAIGameState("BBAI_NEW_GAME_STARTED");
	logSASBBAIInitialState();
}

// <!-- custom: Civ4 does not expose the source save filename to the DLL or Python OnLoad event.
// Start a distinct BBAI file after all save data is read and identify the loaded state through UTC, turn/year, map/game settings, player counts, and read-only RNG states instead. This removes the need to restart Civ4 between repeated save-file tests. (GPT-5.5) -->
void startSASBBAILogForLoadedSave()
{
	rollSASBBAILog("load");
	logSASBBAIGameState("BBAI_SAVE_LOADED");
	logSASBBAIProvenanceContext();
	logSASBBAILogSettings();
}

void logBBAI(TCHAR* format, ... )
{
	// <!-- custom: Defensive backstop only; callers should pre-gate so logging-only arguments are never evaluated while BBAI diagnostics are disabled. (ChatGPT-5.6-Sol) -->
	if (!gLogBBAI)
		return;

	std::string szLine;
	va_list args;
	va_start(args, format);
	// <!-- custom: Structured BBAI diagnostics can now contain long finalized-state/deal rows.
	// Reuse CvString's grow-and-retry formatter instead of silently truncating at the inherited 2048-byte buffer; this supersedes KI#161.2's fixed-buffer size/forced-terminator workaround. (ChatGPT-5.5; ChatGPT-5.6-Sol) -->
	bool const bFormatted = CvString::formatv(szLine, format, args);
	va_end(args);
	FAssertMsg(bFormatted, "BBAI row formatting failed");
	if (!bFormatted)
		return;
	CvString const szLogName = getSASBBAILogName();
	// <!-- custom: The EXE logger applies a second printf-style interpretation to this already-formatted line.
	// Preserve literal percent signs and prevent bogus vararg reads through the shared final-boundary escape. See KI#375.3. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	logSASDiagnosticLiteralLine(szLogName.GetCString(), szLine.c_str());
}

// advc.133:
void logBBAICancel(CvDeal const& d, PlayerTypes eCancelPlayer, wchar const* szReason)
{
	if (gDealCancelLogLevel <= 0)
		return;

	CvWStringBuffer szTmpBuffer;
	GAMETEXT.getDealString(szTmpBuffer, d, eCancelPlayer, false);
	CvWString szBuffer;
	szBuffer.Format(L"    %s cancels deal (%s): %s", GET_PLAYER(eCancelPlayer).getName(0),
			szReason, szTmpBuffer.getCString());
	// Leave it to logBBAI to narrow the string
	logBBAI("%S", szBuffer.GetCString());
}
