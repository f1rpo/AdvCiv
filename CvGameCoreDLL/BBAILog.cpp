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

// <!-- custom: Keep each BBAI category Define paired with its cached field in one iterable registry.
// Startup loading, any-category enabled detection and BBAI_LOG_SETTINGS profile output all walk this list, while widespread g*LogLevel caller guards remain direct field reads. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
struct SASBBAILogCategoryDescriptor
{
	char const* szDefineName;
	int SASBBAILogSettings::* pLogLevel;
};

static SASBBAILogCategoryDescriptor const aSASBBAILogCategories[] =
{
	{ "SAS_BBAI_PLAYER_LOG_LEVEL", &SASBBAILogSettings::iPlayerLogLevel },
	{ "SAS_BBAI_TEAM_LOG_LEVEL", &SASBBAILogSettings::iTeamLogLevel },
	{ "SAS_BBAI_WAR_LOG_LEVEL", &SASBBAILogSettings::iWarLogLevel },
	{ "SAS_BBAI_CITY_LOG_LEVEL", &SASBBAILogSettings::iCityLogLevel },
	{ "SAS_BBAI_PRODUCTION_NO_TARGET_LOG_LEVEL", &SASBBAILogSettings::iProductionNoTargetLogLevel },
	{ "SAS_BBAI_MILITARY_PRODUCTION_LOG_LEVEL", &SASBBAILogSettings::iMilitaryProductionLogLevel },
	{ "SAS_BBAI_SPACE_PRODUCTION_LOG_LEVEL", &SASBBAILogSettings::iSpaceProductionLogLevel },
	{ "SAS_BBAI_LIMITED_PROJECT_PRODUCTION_LOG_LEVEL", &SASBBAILogSettings::iLimitedProjectProductionLogLevel },
	{ "SAS_BBAI_BUILDING_PRODUCTION_LOG_LEVEL", &SASBBAILogSettings::iBuildingProductionLogLevel },
	{ "SAS_BBAI_CITIZEN_LOG_LEVEL", &SASBBAILogSettings::iCitizenLogLevel },
	{ "SAS_BBAI_UNIT_LOG_LEVEL", &SASBBAILogSettings::iUnitLogLevel },
	{ "SAS_BBAI_OVERSEAS_TRANSPORT_LOG_LEVEL", &SASBBAILogSettings::iOverseasTransportLogLevel },
	{ "SAS_BBAI_GREAT_GENERAL_LOG_LEVEL", &SASBBAILogSettings::iGreatGeneralLogLevel },
	{ "SAS_BBAI_SETTLER_LOG_LEVEL", &SASBBAILogSettings::iSettlerLogLevel },
	{ "SAS_BBAI_FOUND_LOG_LEVEL", &SASBBAILogSettings::iFoundLogLevel },
	{ "SAS_BBAI_EVACUATION_LOG_LEVEL", &SASBBAILogSettings::iEvacuationLogLevel },
	{ "SAS_BBAI_WORKER_LOG_LEVEL", &SASBBAILogSettings::iWorkerLogLevel },
	{ "SAS_BBAI_WORKER_SEA_LOG_LEVEL", &SASBBAILogSettings::iWorkerSeaLogLevel },
	{ "SAS_BBAI_MAP_LOG_LEVEL", &SASBBAILogSettings::iMapLogLevel },
	{ "SAS_BBAI_DEAL_CANCEL_LOG_LEVEL", &SASBBAILogSettings::iDealCancelLogLevel },
	{ "SAS_BBAI_BONUS_LOG_LEVEL", &SASBBAILogSettings::iBonusLogLevel },
	{ "SAS_BBAI_CULTURE_LOG_LEVEL", &SASBBAILogSettings::iCultureLogLevel },
	{ "SAS_BBAI_UWAI_AGENT_LOG_LEVEL", &SASBBAILogSettings::iUWAIAgentLogLevel },
	{ "SAS_BBAI_UWAI_MILITARY_ANALYST_LOG_LEVEL", &SASBBAILogSettings::iUWAIMilitaryAnalystLogLevel },
	{ "SAS_BBAI_UWAI_INVASION_GRAPH_LOG_LEVEL", &SASBBAILogSettings::iUWAIInvasionGraphLogLevel },
	{ "SAS_BBAI_UWAI_ARMAMENT_FORECAST_LOG_LEVEL", &SASBBAILogSettings::iUWAIArmamentForecastLogLevel },
	{ "SAS_BBAI_UWAI_WAR_UTILITY_LOG_LEVEL", &SASBBAILogSettings::iUWAIWarUtilityLogLevel }
};

static int getSASBBAILogCategoryCount()
{
	return (int)(sizeof(aSASBBAILogCategories) / sizeof(aSASBBAILogCategories[0]));
}

// <!-- custom: Zero initialization keeps BBAI disabled during DLL/XML startup.
// CvXMLLoadUtility::SetGlobalDefines fills this once after every base/SAS/modular override has loaded, so the hundreds of category gates in ordinary gameplay are direct field reads rather than out-of-line cached-getter calls.
// Runtime Define mutation was already unsupported because the old getters cached their first resolved values. (ChatGPT-5.6-Sol) -->
SASBBAILogSettings gSASBBAILogSettings;

void cacheSASBBAILogSettings()
{
	SASBBAILogSettings& kSettings = gSASBBAILogSettings;
	bool const bMasterEnabled = (GC.getDefineINT("SAS_BBAI_LOG_ENABLE") > 0);
	kSettings.bMasterEnabled = bMasterEnabled;
	bool bAnyCategoryEnabled = false;
	int const iCategoryCount = getSASBBAILogCategoryCount();
	for (int iI = 0; iI < iCategoryCount; iI++)
	{
		SASBBAILogCategoryDescriptor const& kCategory = aSASBBAILogCategories[iI];
		int& iLogLevel = kSettings.*(kCategory.pLogLevel);
		iLogLevel = (bMasterEnabled ? getClampedSASBBAILogLevel(kCategory.szDefineName) : 0);
		if (iLogLevel > 0)
			bAnyCategoryEnabled = true;
	}
	kSettings.bEnabled = (bMasterEnabled && bAnyCategoryEnabled);
}

// <!-- custom: Keep each supporting option's XML lookup and normalization in one reader, shared by its runtime cache and the session profiles.
// These are not category levels: the score interval is clamped to at least one, the timestamp/Food-support switches are booleans, and the high-war-utility threshold remains an integer. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
static int getSASBBAIScoreLogIntervalConfigured()
{
	return std::max(1, GC.getDefineINT("SAS_BBAI_SCORE_LOG_INTERVAL_TURNS_UNSCALED_GAMESPEED"));
}

static int getSASBBAITimestampedFilenameConfigured()
{
	return GC.getDefineBOOL("SAS_BBAI_LOG_USE_TIMESTAMPED_FILENAME");
}

static int getSASBBAIFoodSupportAuditConfigured()
{
	return GC.getDefineBOOL("SAS_BBAI_FOUND_LATER_CITY_FOOD_SUPPORT_AUDIT_ENABLE");
}

// <!-- custom: The high-war-utility threshold controls potentially large WAR level-3 scenario/component output; zero disables it independently of the category.
// Share this reader with the diagnostic profile while preserving the evaluator's first-use cache and threshold semantics. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
static int getSASBBAIHighWarUtilityThresholdConfigured()
{
	return GC.getDefineINT("SAS_UWAI_HIGH_UTILITY_LOG_THRESHOLD");
}

int getSASBBAIHighWarUtilityLogThreshold()
{
	static int const iThreshold = getSASBBAIHighWarUtilityThresholdConfigured();
	return iThreshold;
}

// <!-- custom: Preserve the audit's first-use cache, but callers now check Found level 2 before entering this getter so disabled diagnostics do not initialize it. See KI#144.4. (GPT-6.1-Sol) -->
bool isSASBBAILaterCityFoodSupportAuditEnabled()
{
	static bool const bEnabled = (getSASBBAIFoodSupportAuditConfigured() > 0);
	return bEnabled;
}

int getSASBBAIScoreLogInterval()
{
	// <!-- custom: This is reached only inside enabled PLAYER logging; keep the modulo divisor at least 1. (GPT-5.5? + ChatGPT-5.6-Sol) -->
	static const int iInterval = (isSASBBAILogMasterEnabled() ? getSASBBAIScoreLogIntervalConfigured() : 1);
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
	static const bool bUseTimestampedFilename = (getSASBBAITimestampedFilenameConfigured() > 0);
	return bUseTimestampedFilename;
}

// <!-- custom: These three no-argument activity callbacks are a session-profile exception to gating before helper entry: the registry calls them only during enabled BBAI/SASGameRecord new/load logging, never from the gameplay loop.
// Their category checks stay inside so the registry can report inactive options too; short-circuit && skips the cached option getter when its category is disabled, and no caller arguments require preparation.
// This retains a few session-start callback calls; ordinary gameplay logging must still pre-gate before calling helpers or evaluating logging-only arguments. (GPT-6.1-Sol) -->
static bool isSASBBAIScoreIntervalActive()
{
	return (gPlayerLogLevel > 0);
}

static bool isSASBBAIFoodSupportAuditArmed()
{
	return (gFoundLogLevel >= 2 && isSASBBAILaterCityFoodSupportAuditEnabled());
}

static bool isSASBBAIHighWarUtilityLoggingArmed()
{
	return (gWarLogLevel >= 3 && getSASBBAIHighWarUtilityLogThreshold() > 0);
}

// <!-- custom: A small supporting-options group shares configured readers and activity policies between the BBAI and SASGameRecord session profiles.
// Keep those policies explicit: PLAYER activates the score interval, Found level 2 plus its switch arms the food audit, and WAR level 3 plus a positive threshold arms high-utility logging; site/scenario-specific exclusions still apply.
// Iteration and formatting occur only in caller-gated session logging; gameplay consumers retain their existing first-use caches. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
struct SASBBAILogSupportingOptionDescriptor
{
	char const* szConfiguredField;
	int (*getConfigured)();
	char const* szActivityField;
	bool (*isActive)();
};

// <!-- custom: Names without () below store function pointers; the formatter invokes them through kOption.getConfigured() / kOption.isActive().
// A reader can therefore serve both a direct cache-initialization call and this registry even when searching for its name followed by ( finds only one direct call. (GPT-6.1-Sol) -->
static SASBBAILogSupportingOptionDescriptor const aSASBBAILogSupportingOptions[] =
{
	{ "bbaiTimestampedFilename", getSASBBAITimestampedFilenameConfigured, NULL, NULL },
	{ "bbaiScoreIntervalConfigured", getSASBBAIScoreLogIntervalConfigured, "bbaiScoreIntervalActive", isSASBBAIScoreIntervalActive },
	{ "foundLaterCityFoodSupportAuditConfigured", getSASBBAIFoodSupportAuditConfigured, "foundLaterCityFoodSupportAuditArmed", isSASBBAIFoodSupportAuditArmed },
	{ "uwaiHighUtilityThresholdConfigured", getSASBBAIHighWarUtilityThresholdConfigured, "uwaiHighUtilityLoggingArmed", isSASBBAIHighWarUtilityLoggingArmed }
};

static CvString getSASBBAILogSupportingProfileFields()
{
	// <!-- custom: Emit the same session sequence in both logs, including fixed-filename mode where the filename does not identify each new/load session. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	CvString szFields = CvString::format("bbaiSessionSequence=%d", getSASBBAILogSessionSequence());
	int const iCount = (int)(sizeof(aSASBBAILogSupportingOptions) / sizeof(aSASBBAILogSupportingOptions[0]));
	for (int i = 0; i < iCount; i++)
	{
		SASBBAILogSupportingOptionDescriptor const& kOption = aSASBBAILogSupportingOptions[i];
		appendSASDiagnosticListValue(szFields, CvString::format("%s=%d", kOption.szConfiguredField, kOption.getConfigured()).GetCString(), " ");
		if (kOption.isActive != NULL)
			appendSASDiagnosticListValue(szFields, CvString::format("%s=%d", kOption.szActivityField, kOption.isActive()).GetCString(), " ");
	}
	return szFields;
}

static CvString getSASBBAILogCategoryToken(char const* szDefineName)
{
	CvString szToken(szDefineName == NULL ? "" : szDefineName);
	size_t const iPrefix = strlen("SAS_BBAI_");
	size_t const iSuffix = szToken.rfind("_LOG_LEVEL");
	if (szToken.length() > iPrefix && iSuffix != CvString::npos && iSuffix > iPrefix)
		szToken = szToken.substr(iPrefix, iSuffix - iPrefix);
	return szToken;
}

CvString getSASBBAILogProfileFields()
{
	// <!-- custom: BBAI's cached category fields intentionally store effective levels only, becoming zero when the master switch is disabled.
	// Re-read the same registered Defines once per SASGameRecord session so the profile preserves what was configured as well as whether any category can actually run. (ChatGPT-5.6-Sol) -->
	CvString szConfiguredLevels;
	CvString szEffectiveLevels;
	int const iCategoryCount = getSASBBAILogCategoryCount();
	for (int iI = 0; iI < iCategoryCount; iI++)
	{
		SASBBAILogCategoryDescriptor const& kCategory = aSASBBAILogCategories[iI];
		CvString const szToken = getSASBBAILogCategoryToken(kCategory.szDefineName);
		int const iConfiguredLevel = getClampedSASBBAILogLevel(kCategory.szDefineName);
		int const iEffectiveLevel = gSASBBAILogSettings.*(kCategory.pLogLevel);
		appendSASDiagnosticListValue(szConfiguredLevels, CvString::format("%s:%d", szToken.GetCString(), iConfiguredLevel).GetCString());
		if (iEffectiveLevel > 0)
			appendSASDiagnosticListValue(szEffectiveLevels, CvString::format("%s:%d", szToken.GetCString(), iEffectiveLevel).GetCString());
	}
	CvString szFields;
	szFields.Format("bbaiMasterEnabled=%d bbaiAnyCategoryEffective=%d bbaiConfiguredLevels=%s bbaiEffectiveLevels=%s %s",
		isSASBBAILogMasterEnabled(), isSASBBAILogEnabled(), (szConfiguredLevels.empty() ? "-" : szConfiguredLevels.GetCString()),
		(szEffectiveLevels.empty() ? "-" : szEffectiveLevels.GetCString()), getSASBBAILogSupportingProfileFields().GetCString());
	return szFields;
}

// <!-- custom: When timestamped filenames are enabled, use a new file for every new game and loaded save.
// The new/load context and shared chronological sequence keep files distinct and show their order even when multiple new/load actions occur within the same UTC second. (GPT-5.5) -->
static CvString getSASBBAILogName()
{
	return getSASDiagnosticLogName("BBAI", getSASBBAILogTimestamp(), g_szSASBBAILogContext, isSASBBAILogTimestampedFilenameEnabled());
}

static void rollSASBBAILog(const char* szContext)
{
	// <!-- custom: Refresh session identity for both filename modes; the sequence also invalidates diagnostic deduplication caches on every new/load, even when fixed-name logs share one file; only filename context formatting depends on timestamped filenames; each header still needs its own current UTC. See KI#629. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	g_szSASBBAILogTimestamp = createSASUtcTimestamp();
	g_iSASBBAILogSequence++;
	if (isSASBBAILogTimestampedFilenameEnabled())
		g_szSASBBAILogContext.Format("%s%d", szContext, g_iSASBBAILogSequence);
}

int getSASBBAILogSessionSequence()
{
	return g_iSASBBAILogSequence;
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
		appendSASDiagnosticListValue(szGameOptions, GC.getInfo(eLoopGameOption).getType());
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
	CvString szCategorySettings;
	int const iCategoryCount = getSASBBAILogCategoryCount();
	for (int iI = 0; iI < iCategoryCount; iI++)
	{
		SASBBAILogCategoryDescriptor const& kCategory = aSASBBAILogCategories[iI];
		szCategorySettings += CvString::format(" %s=%d", kCategory.szDefineName,
				gSASBBAILogSettings.*(kCategory.pLogLevel));
	}
	logBBAI("BBAI_LOG_SETTINGS SAS_BBAI_LOG_ENABLE=%d SAS_BBAI_LOG_USE_TIMESTAMPED_FILENAME=%d%s SAS_BBAI_SCORE_LOG_INTERVAL_TURNS_UNSCALED_GAMESPEED=%d",
		isSASBBAILogMasterEnabled(), isSASBBAILogTimestampedFilenameEnabled(), szCategorySettings.GetCString(), gScoreLogInterval);
	// <!-- custom: Reuse the same supporting-option fields as SASGameRecord while preserving the existing BBAI_LOG_SETTINGS row. (GPT-6.1-Sol) -->
	logBBAI("BBAI_LOG_SUPPORTING_SETTINGS %s", getSASBBAILogSupportingProfileFields().GetCString());
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

// <!-- custom: The three startup entry points below are caller-pre-gated: CvGame checks isSASBBAILogEnabled() before new-game/load initialization, and CvEventReporter checks it before the finalized new-game report.
// This protects every nested logging call and its argument/setup work, including settings-string construction; individual calls do not need duplicate master-enable checks. (GPT-6.1-Sol) -->
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
// Start a distinct BBAI file after all save data is read and identify the loaded state through UTC, turn/year, map/game settings, player counts, and read-only RNG states instead.
// This removes the need to restart Civ4 between repeated save-file tests. (GPT-5.5) -->
void startSASBBAILogForLoadedSave()
{
	rollSASBBAILog("load");
	logSASBBAIGameState("BBAI_SAVE_LOADED");
	logSASBBAIProvenanceContext();
	logSASBBAILogSettings();
}

void logBBAI(TCHAR* format, ... )
{
	// <!-- custom: Defensive sink backstop only; callers still pre-gate by category so disabled diagnostics do not evaluate logging-only arguments or setup. (ChatGPT-5.6-Sol) -->
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
	// <!-- custom: Submit the already-formatted row promptly through the shared DLL-owned writer; direct file emission preserves literal percent signs without Civ4's former second printf pass. See KI#375.3. (ChatGPT-5.6-Sol + GPT-5.6-Sol + GPT-6.1-Sol) -->
	logSASDiagnosticLiteralLine(szLogName.GetCString(), szLine.c_str(), SAS_DIAGNOSTIC_LOG_BBAI);
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
