#include "CvGameCoreDLL.h"
#include "CvEventReporter.h"
#include "CvGame.h"
#include "CvPlayer.h"
#include "CvCity.h" // <!-- custom: Needed to log nearest/working city context for Worker deaths. (GPT-5.5) -->
#include "CvPlot.h" // <!-- custom: Needed to log plot context for Worker deaths. (GPT-5.5) -->
#include "CvUnit.h" // <!-- custom: Needed to log killed Worker and killer unit context. (GPT-5.5) -->
#include "CvUnitAI.h" // <!-- custom: Needed to inspect killed Worker AI group state. (GPT-5.5) -->
#include "CvPlayerAI.h" // <!-- custom: Needed to log killed Worker's current plot danger. (GPT-5.5) -->
#include "CvMap.h" // <!-- custom: Needed for nearest-city distance in Worker-death diagnostics. (GPT-5.5) -->
#include "PlotRadiusIterator.h" // <!-- custom: Needed to count visible enemies around Worker-death plots. (GPT-5.5) -->
#include "CvInfo_Terrain.h" // <!-- custom: Needed for terrain/feature/bonus/improvement/route names in Worker-death diagnostics. (GPT-5.5) -->
#include "CvSelectionGroupAI.h" // <!-- custom: Needed to log Worker-death mission AI context in BBAI Worker diagnostics. (GPT-5.5) -->
#include "CvGameCoreUtils.h" // <!-- custom: Needed for BBAI Worker-death unit-AI and mission-AI strings. (GPT-5.5) -->
#include "CvDLLPythonIFaceBase.h" // advc
#include "BBAILog.h" // <!-- custom: Needed to complete the BBAI identity header. (GPT-5.5 + ChatGPT-5.5) -->
#include "SASGameRecordLog.h" // <!-- custom: Structured run-summary action rows are separate from BBAI diagnostics. (GPT-5.5) -->

//
// static, singleton accessor
//
CvEventReporter& CvEventReporter::getInstance()
{
	static CvEventReporter gEventReporter;
	return gEventReporter;
}


void CvEventReporter::resetStatistics()
{
	m_kStatistics.reset();
}

// advc.106l: Explicit constructor added, so I can initialize my booleans.
CvEventReporter::CvEventReporter() : m_bPreAutoSave(false), m_bPreQuickSave(false) {}

static bool isSASBBAIWorkerDeathUnit(CvUnit const& kUnit)
{
	return kUnit.isWorker() || kUnit.AI_getUnitAIType() == UNITAI_WORKER || kUnit.AI_getUnitAIType() == UNITAI_WORKER_SEA;
}

static CvCity const* getSASBBAINearestOwnedCity(CvPlot const& kPlot, PlayerTypes ePlayer, int& iDistance)
{
	iDistance = -1;
	if (ePlayer == NO_PLAYER)
		return NULL;
	CvCity const* pBestCity = NULL;
	int iLoop = 0;
	for (CvCity const* pLoopCity = GET_PLAYER(ePlayer).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(ePlayer).nextCity(&iLoop))
	{
		const int iLoopDistance = plotDistance(kPlot.getX(), kPlot.getY(), pLoopCity->getX(), pLoopCity->getY());
		if (iDistance < 0 || iLoopDistance < iDistance)
		{
			iDistance = iLoopDistance;
			pBestCity = pLoopCity;
		}
	}
	return pBestCity;
}

static void countSASBBAIVisibleEnemiesNearWorkerDeath(CvPlot const& kPlot, PlayerTypes ePlayer, int& iVisibleEnemies, int& iVisibleCombatEnemies)
{
	iVisibleEnemies = 0;
	iVisibleCombatEnemies = 0;
	if (ePlayer == NO_PLAYER)
		return;
	for (SquareIter it(kPlot, 2); it.hasNext(); ++it)
	{
		iVisibleEnemies += it->plotCount(PUF_isEnemy, ePlayer, false, NO_PLAYER, NO_TEAM, PUF_isVisible, ePlayer);
		iVisibleCombatEnemies += it->plotCount(PUF_canDefendEnemy, ePlayer, false, NO_PLAYER, NO_TEAM, PUF_isVisible, ePlayer);
	}
}

static void logSASBBAIWorkerDeathContext(CvUnit const* pWinner, CvUnit const* pLoser)
{
	if (pWinner == NULL || pLoser == NULL || !isSASBBAIWorkerDeathUnit(*pLoser))
		return;
	CvPlot const& kPlot = pLoser->getPlot();
	PlayerTypes const eLoser = pLoser->getOwner();
	CvSelectionGroupAI const* pGroup = pLoser->AI().AI_getGroup();
	CvPlot const* pMissionPlot = (pGroup == NULL ? NULL : pGroup->AI_getMissionAIPlot());
	CvWString szLoserAI; getUnitAIString(szLoserAI, pLoser->AI_getUnitAIType());
	CvWString szWinnerAI; getUnitAIString(szWinnerAI, pWinner->AI_getUnitAIType());
	CvWString szMissionAI; getMissionAIString(szMissionAI, pGroup == NULL ? NO_MISSIONAI : pGroup->AI_getMissionAIType());
	int iVisibleEnemies = 0;
	int iVisibleCombatEnemies = 0;
	countSASBBAIVisibleEnemiesNearWorkerDeath(kPlot, eLoser, iVisibleEnemies, iVisibleCombatEnemies);
	int iNearestOwnedCityDistance = -1;
	CvCity const* pNearestOwnedCity = getSASBBAINearestOwnedCity(kPlot, eLoser, iNearestOwnedCityDistance);
	CvCity const* pWorkingCity = kPlot.getWorkingCity();
	const int iOwnerUnits = kPlot.plotCount(PUF_isPlayer, eLoser);
	const int iOwnerDefenders = kPlot.plotCount(PUF_canDefend, -1, -1, eLoser);
	const int iOwnerWorkers = kPlot.plotCount(PUF_isUnitAIType, UNITAI_WORKER, -1, eLoser) + kPlot.plotCount(PUF_isUnitAIType, UNITAI_WORKER_SEA, -1, eLoser);
	// <!-- custom: Worker death counts dropped from 40 to 17 by t200 in the save-file-450 test after the current-danger fixes, but the remaining deaths still need classification.
	// Log only Worker/Work Boat deaths in BBAI Worker diagnostics with enough context to distinguish unavoidable war-front losses from bad movement or missing retreat; no gameplay behavior change. (GPT-5.5) -->
	logBBAI("    WORKER_DEATH_CONTEXT turn=%d loser=%d %S loserUnitId=%d loserUnit=%S loserAI=%S winner=%d winnerUnitId=%d winnerUnit=%S winnerAI=%S plot=(%d,%d) owner=%d cityPlot=%d currentDanger=%d missionAI=%S missionTarget=(%d,%d) missionQueue=%d activity=%d movesSpent=%d movesLeft=%d visibleEnemiesR2=%d visibleCombatEnemiesR2=%d ownerUnitsOnPlot=%d ownerDefendersOnPlot=%d ownerWorkersOnPlot=%d nearestOwnedCity=%S nearestOwnedCityId=%d nearestOwnedCityDistance=%d workingCity=%S workingCityId=%d terrain=%S feature=%S bonus=%S improvement=%S route=%S",
		GC.getGame().getGameTurn(), eLoser, GET_PLAYER(eLoser).getCivilizationDescription(0), pLoser->getID(),
		GC.getInfo(pLoser->getUnitType()).getDescription(), szLoserAI.GetCString(), pWinner->getOwner(), pWinner->getID(),
		GC.getInfo(pWinner->getUnitType()).getDescription(), szWinnerAI.GetCString(), kPlot.getX(), kPlot.getY(), kPlot.getOwner(),
		kPlot.isCity(), GET_PLAYER(eLoser).AI_getPlotDanger(kPlot), szMissionAI.GetCString(),
		(pMissionPlot == NULL ? -1 : pMissionPlot->getX()), (pMissionPlot == NULL ? -1 : pMissionPlot->getY()),
		(pGroup == NULL ? -1 : pGroup->getLengthMissionQueue()), (pGroup == NULL ? NO_ACTIVITY : pGroup->getActivityType()),
		pLoser->getMoves(), pLoser->movesLeft(), iVisibleEnemies, iVisibleCombatEnemies, iOwnerUnits, iOwnerDefenders, iOwnerWorkers,
		(pNearestOwnedCity == NULL ? L"-" : pNearestOwnedCity->getName().GetCString()),
		(pNearestOwnedCity == NULL ? -1 : pNearestOwnedCity->getID()), iNearestOwnedCityDistance,
		(pWorkingCity == NULL ? L"-" : pWorkingCity->getName().GetCString()), (pWorkingCity == NULL ? -1 : pWorkingCity->getID()),
		GC.getInfo(kPlot.getTerrainType()).getDescription(),
		(kPlot.getFeatureType() == NO_FEATURE ? L"-" : GC.getInfo(kPlot.getFeatureType()).getDescription()),
		(kPlot.getBonusType(pLoser->getTeam()) == NO_BONUS ? L"-" : GC.getInfo(kPlot.getBonusType(pLoser->getTeam())).getDescription()),
		(kPlot.getImprovementType() == NO_IMPROVEMENT ? L"-" : GC.getInfo(kPlot.getImprovementType()).getDescription()),
		(kPlot.getRouteType() == NO_ROUTE ? L"-" : GC.getInfo(kPlot.getRouteType()).getDescription()));
}

// advc.003y: Just pass the call along
void CvEventReporter::initPythonCallbackGuards()
{
	m_kPythonEventMgr.initCallbackGuards();
}

// Returns true if the event is consumed by Python
bool CvEventReporter::mouseEvent(int evt, int iCursorX, int iCursorY, bool bInterfaceConsumed)
{
	return m_kPythonEventMgr.reportMouseEvent(evt, iCursorX, iCursorY, bInterfaceConsumed);
}

// Returns true if the event is consumed by Python
bool CvEventReporter::kbdEvent(int evt, int key, int iCursorX, int iCursorY)
{
	return m_kPythonEventMgr.reportKbdEvent(evt, key, iCursorX, iCursorY);
}

void CvEventReporter::genericEvent(const char* szEventName, void *pyArgs)
{
	m_kPythonEventMgr.reportGenericEvent(szEventName, pyArgs);
}


void CvEventReporter::newGame()
{
	/*	This will only be called if statistics are being reported!
		Called at the launch of a game (new or loaded) */

	// Report initial stats for the game
	m_kStatistics.setMapName(CvString(GC.getInitCore().getMapScriptName()).GetCString());
	m_kStatistics.setEra(GC.getInitCore().getEra());
}

void CvEventReporter::newPlayer(PlayerTypes ePlayer)
{
	/*	This will only be called if statistics are being reported!
		Called at the launch of a game (new or loaded) */

	// Report initial stats for this player
	m_kStatistics.setLeader(ePlayer, GET_PLAYER(ePlayer).getLeaderType());
}

void CvEventReporter::reportModNetMessage(int iData1, int iData2, int iData3, int iData4, int iData5)
{
	m_kPythonEventMgr.reportModNetMessage(iData1, iData2, iData3, iData4, iData5);
}

void CvEventReporter::init()
{
	m_kPythonEventMgr.reportInit();
}

void CvEventReporter::update(float fDeltaTime)
{
	m_kPythonEventMgr.reportUpdate(fDeltaTime);
}

void CvEventReporter::unInit()
{
	m_kPythonEventMgr.reportUnInit();
}

void CvEventReporter::gameStart()
{
	// <!-- custom: Capture source provenance only once final new-game/scenario initialization has established the real starting turn, before diagnostic/UI consumers read the persistent history. (ChatGPT-5.6-Sol) -->
	GC.getGame().initializeSASVersionHistoryForNewGame();
	// <!-- custom: Complete the new-game BBAI identity header after map generation and player initialization.
	// Caller-gated to avoid entering logging helpers when BBAI is disabled. (GPT-5.5) -->
	if (isSASBBAILogEnabled()) logSASBBAINewGameStarted();
	if (isSASGameRecordLogEnabled()) logSASGameRecordNewGameStarted();
	m_kPythonEventMgr.reportGameStart();
	// <!-- custom: End the level-3 initialization RNG interval only after Python gameStart too, so map/setup consumption has one complete baseline before normal gameplay begins. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (g_bSASGameRecordRngTrackingActive) logSASGameRecordRngCheckpoint(GC.getGame().getGameTurn(), SAS_RNG_CHECKPOINT_NEW_GAME_INITIALIZED);
}

void CvEventReporter::gameEnd()
{
	m_kPythonEventMgr.reportGameEnd();
}

void CvEventReporter::beginGameTurn(int iGameTurn)
{
	m_kPythonEventMgr.reportBeginGameTurn(iGameTurn);
}

void CvEventReporter::endGameTurn(int iGameTurn)
{
	m_kPythonEventMgr.reportEndGameTurn(iGameTurn);
	// <!-- custom: Intentional raw-level cache exception: this boundary genuinely needs two different thresholds (level 2+ turn-change flushing and level 1+ periodic snapshots).
	// One integer read is cheaper/cleaner here than rereading the global merely to manufacture two booleans. (ChatGPT-5.6-Sol) -->
	int const iGameRecordLogLevel = gGameRecordLogLevel;
	// <!-- custom: Plot changes and permanent map revelation are collected during the turn so SASGameRecord writes compact coordinate lists instead of one row per plot.
	// Flush after the Python turn event so its changes are included too. (GPT-5.6-Sol) -->
	if (iGameRecordLogLevel >= 2) flushSASGameRecordTurnChanges(iGameTurn);
	// <!-- custom: Periodic game-record snapshots are separate from normal BBAI diagnostics and mainly serve autoplay comparison / external review. (ChatGPT-5.5) -->
	if (iGameRecordLogLevel > 0 && iGameTurn > 0 && (iGameTurn % gGameRecordTurnInterval) == 0) logSASGameRecordTurn(iGameTurn);
}

void CvEventReporter::beginPlayerTurn(int iGameTurn, PlayerTypes ePlayer)
{
	m_kPythonEventMgr.reportBeginPlayerTurn(iGameTurn, ePlayer);
}

void CvEventReporter::endPlayerTurn(int iGameTurn, PlayerTypes ePlayer)
{
	m_kPythonEventMgr.reportEndPlayerTurn(iGameTurn, ePlayer);
}

void CvEventReporter::firstContact(TeamTypes eTeamID1, TeamTypes eTeamID2)
{
	m_kPythonEventMgr.reportFirstContact(eTeamID1, eTeamID2);
}

// <!-- custom: Add the combat target captured by CvUnit instead of reconstructing it from whichever unit lost. See KI#377. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void CvEventReporter::combatResult(CvUnit* pWinner, CvUnit* pLoser, CvPlot const* pBattlePlot)
{
	if (gGameRecordLogLevel >= 2) logSASGameRecordCombatResult(pWinner, pLoser, pBattlePlot);
	if (gWorkerLogLevel >= 2) logSASBBAIWorkerDeathContext(pWinner, pLoser);
	m_kPythonEventMgr.reportCombatResult(pWinner, pLoser);
}

// advc: Cut from CvUnit::resolveCombat
void CvEventReporter::combatLogHit(CombatDetails const& kAttackerDetails, CombatDetails const& kDefenderDetails, int iDamage, bool bAttackerTakesHit)
{
	CyArgsList pyArgs;
	pyArgs.add(gDLL->getPythonIFace()->makePythonObject(&kAttackerDetails));
	pyArgs.add(gDLL->getPythonIFace()->makePythonObject(&kDefenderDetails));
	pyArgs.add(bAttackerTakesHit ? 1 : 0);
	pyArgs.add(iDamage);
	genericEvent("combatLogHit", pyArgs.makeFunctionArgs());
}

void CvEventReporter::improvementBuilt(int iImprovementType, int iX, int iY)
{
	m_kPythonEventMgr.reportImprovementBuilt(iImprovementType, iX, iY);
}

void CvEventReporter::improvementDestroyed(int iImprovementType, int iPlayer, int iX, int iY)
{
	m_kPythonEventMgr.reportImprovementDestroyed(iImprovementType, iPlayer, iX, iY);
}

void CvEventReporter::routeBuilt(int iRouteType, int iX, int iY)
{
	m_kPythonEventMgr.reportRouteBuilt(iRouteType, iX, iY);
}

void CvEventReporter::plotRevealed(CvPlot *pPlot, TeamTypes eTeam)
{
	m_kPythonEventMgr.reportPlotRevealed(pPlot, eTeam);
}

void CvEventReporter::plotFeatureRemoved(CvPlot *pPlot, FeatureTypes eFeature, CvCity* pCity)
{
	m_kPythonEventMgr.reportPlotFeatureRemoved(pPlot, eFeature, pCity);
}

void CvEventReporter::plotPicked(CvPlot *pPlot)
{
	m_kPythonEventMgr.reportPlotPicked(pPlot);
}

void CvEventReporter::nukeExplosion(CvPlot *pPlot, CvUnit* pNukeUnit)
{
	m_kPythonEventMgr.reportNukeExplosion(pPlot, pNukeUnit);
}

void CvEventReporter::gotoPlotSet(CvPlot *pPlot, PlayerTypes ePlayer)
{
	m_kPythonEventMgr.reportGotoPlotSet(pPlot, ePlayer);
}

void CvEventReporter::cityBuilt(CvCity *pCity)
{
	if (gGameRecordLogLevel >= 2) logSASGameRecordCityBuilt(pCity);
	m_kPythonEventMgr.reportCityBuilt(pCity);
	m_kStatistics.cityBuilt(pCity);
}

void CvEventReporter::cityRazed(CvCity *pCity, PlayerTypes ePlayer)
{
	// <!-- custom: SASGameRecord now brackets CvPlayer::raze directly so its single CITY_RAZED row can include exact post-disband land/population/victory deltas.
	// Keep this reporter focused on Python/statistics while the city is still alive. (ChatGPT-5.6-Sol) -->
	m_kPythonEventMgr.reportCityRazed(pCity, ePlayer);
	m_kStatistics.cityRazed(pCity, ePlayer);
}

void CvEventReporter::cityAcquired(PlayerTypes eOldOwner, PlayerTypes iPlayer, CvCity* pCity, bool bConquest, bool bTrade)
{
	if (gGameRecordLogLevel >= 2) logSASGameRecordCityAcquired(eOldOwner, iPlayer, pCity, bConquest, bTrade);
	m_kPythonEventMgr.reportCityAcquired(eOldOwner, iPlayer, pCity, bConquest, bTrade);
}

void CvEventReporter::cityAcquiredAndKept(PlayerTypes iPlayer, CvCity* pCity)
{
	m_kPythonEventMgr.reportCityAcquiredAndKept(iPlayer, pCity);
}

void CvEventReporter::cityLost( CvCity *pCity)
{
	m_kPythonEventMgr.reportCityLost(pCity);
}

void CvEventReporter::cultureExpansion(CvCity *pCity, PlayerTypes ePlayer)
{
	// <!-- custom: Border expansion is a rare consequential city-state transition (workable radius/defense) that can occur between periodic snapshots.
	// Preserve the realized new culture level at level 2+; no additional gameplay calculation or RNG is introduced. (ChatGPT-5.6-Sol) -->
	if (gGameRecordLogLevel >= 2) logSASGameRecordCityCultureExpanded(pCity);
	m_kPythonEventMgr.reportCultureExpansion(pCity, ePlayer);
}

void CvEventReporter::cityGrowth(CvCity *pCity, PlayerTypes ePlayer)
{
	m_kPythonEventMgr.reportCityGrowth(pCity, ePlayer);
}

void CvEventReporter::cityDoTurn(CvCity *pCity, PlayerTypes ePlayer)
{
	m_kPythonEventMgr.reportCityProduction(pCity, ePlayer);
}

void CvEventReporter::cityBuildingUnit(CvCity* pCity, UnitTypes eUnitType)
{
	m_kPythonEventMgr.reportCityBuildingUnit(pCity, eUnitType);
}

void CvEventReporter::cityBuildingBuilding(CvCity* pCity, BuildingTypes eBuildingType)
{
	m_kPythonEventMgr.reportCityBuildingBuilding(pCity, eBuildingType);
}

void CvEventReporter::cityRename(CvCity* pCity)
{
	m_kPythonEventMgr.reportCityRename(pCity);
}

void CvEventReporter::cityHurry(CvCity* pCity, HurryTypes eHurry)
{
	m_kPythonEventMgr.reportCityHurry(pCity, eHurry);
}

void CvEventReporter::selectionGroupPushMission(CvSelectionGroup* pSelectionGroup, MissionTypes eMission)
{
	m_kPythonEventMgr.reportSelectionGroupPushMission(pSelectionGroup, eMission);
}

void CvEventReporter::unitMove(CvPlot* pPlot, CvUnit* pUnit, CvPlot* pOldPlot)
{
	m_kPythonEventMgr.reportUnitMove(pPlot, pUnit, pOldPlot);
}

void CvEventReporter::unitSetXY(CvPlot* pPlot, CvUnit* pUnit)
{
	m_kPythonEventMgr.reportUnitSetXY(pPlot, pUnit);
}

void CvEventReporter::unitCreated(CvUnit *pUnit)
{
	m_kPythonEventMgr.reportUnitCreated(pUnit);
}

void CvEventReporter::unitBuilt(CvCity *pCity, CvUnit *pUnit)
{
	m_kPythonEventMgr.reportUnitBuilt(pCity, pUnit);
	m_kStatistics.unitBuilt(pUnit);
}

void CvEventReporter::unitKilled(CvUnit *pUnit, PlayerTypes eAttacker)
{
	m_kPythonEventMgr.reportUnitKilled(pUnit, eAttacker);
	m_kStatistics.unitKilled(pUnit, eAttacker);
}

void CvEventReporter::unitLost(CvUnit *pUnit)
{
	m_kPythonEventMgr.reportUnitLost(pUnit);
}

void CvEventReporter::unitCaptured(PlayerTypes eOldOwner, UnitTypes eOldUnitType, CvUnit* pNewUnit)
{
	// <!-- custom: forward actual capture creation to Python so battle history records captured workers/civilians instead of inferring from the earlier combatResult. (GPT-5.5) -->
	if (gGameRecordLogLevel >= 2) logSASGameRecordUnitCaptured(eOldOwner, eOldUnitType, pNewUnit);
	m_kPythonEventMgr.reportUnitCaptured(eOldOwner, eOldUnitType, pNewUnit);
}

void CvEventReporter::unitPromoted(CvUnit *pUnit, PromotionTypes ePromotion)
{
	// <!-- custom: Record authoritative promotion applications after CvUnit::promote succeeds.
	// The recorder separates ordinary player/AI choices from fixed Great-General leader promotions and aggregates the common case instead of emitting level-2 per-unit spam. (ChatGPT-5.6-Sol) -->
	if (gGameRecordLogLevel >= 2) logSASGameRecordUnitPromoted(pUnit, ePromotion);
	m_kPythonEventMgr.reportUnitPromoted(pUnit, ePromotion);
}

void CvEventReporter::unitSelected( CvUnit *pUnit)
{
	m_kPythonEventMgr.reportUnitSelected(pUnit);
}

void CvEventReporter::unitRename(CvUnit* pUnit)
{
	m_kPythonEventMgr.reportUnitRename(pUnit);
}

void CvEventReporter::unitPillage(CvUnit* pUnit, ImprovementTypes eImprovement, RouteTypes eRoute, PlayerTypes ePlayer)
{
	m_kPythonEventMgr.reportUnitPillage(pUnit, eImprovement, eRoute, ePlayer);
}

void CvEventReporter::unitSpreadReligionAttempt(CvUnit* pUnit, ReligionTypes eReligion, bool bSuccess)
{
	m_kPythonEventMgr.reportUnitSpreadReligionAttempt(pUnit, eReligion, bSuccess);
}

void CvEventReporter::unitGifted(CvUnit* pUnit, PlayerTypes eGiftingPlayer, CvPlot* pPlotLocation)
{
	// <!-- custom: A gifted unit is recreated for its recipient before this authoritative reporter boundary.
	// Preserve the realized giver->recipient transfer at level 2+; AI gift valuation remains outside factual SASGameRecord history. (ChatGPT-5.6-Sol) -->
	if (gGameRecordLogLevel >= 2) logSASGameRecordUnitGifted(pUnit, eGiftingPlayer, pPlotLocation);
	m_kPythonEventMgr.reportUnitGifted(pUnit, eGiftingPlayer, pPlotLocation);
}

void CvEventReporter::unitBuildImprovement(CvUnit* pUnit, BuildTypes eBuild, bool bFinished)
{
	m_kPythonEventMgr.reportUnitBuildImprovement(pUnit, eBuild, bFinished);
}

void CvEventReporter::goodyReceived(PlayerTypes ePlayer, CvPlot *pGoodyPlot, CvUnit *pGoodyUnit, GoodyTypes eGoodyType)
{
	m_kPythonEventMgr.reportGoodyReceived(ePlayer, pGoodyPlot, pGoodyUnit, eGoodyType);
}

void CvEventReporter::greatPersonBorn(CvUnit *pUnit, PlayerTypes ePlayer, CvCity *pCity)
{
	if (gGameRecordLogLevel >= 2) logSASGameRecordGreatPersonBorn(pUnit, ePlayer, pCity);
	m_kPythonEventMgr.reportGreatPersonBorn( pUnit, ePlayer, pCity);
	m_kStatistics.unitBuilt(pUnit);
}

void CvEventReporter::buildingBuilt(CvCity *pCity, BuildingTypes eBuilding)
{
	if (gGameRecordLogLevel >= 2) logSASGameRecordBuildingBuilt(pCity, eBuilding);
	m_kPythonEventMgr.reportBuildingBuilt(pCity, eBuilding);
	m_kStatistics.buildingBuilt(pCity, eBuilding);
}

void CvEventReporter::projectBuilt(CvCity *pCity, ProjectTypes eProject)
{
	// <!-- custom: SASGameRecord logs production-built projects in CvCity::popOrder so PROJECT_BUILT can carry the exact overflow result; this reporter remains the unchanged Python event path. (ChatGPT-5.6-Sol) -->
	m_kPythonEventMgr.reportProjectBuilt(pCity, eProject);
}

// <!-- custom: Added eCause only for SASGameRecord provenance; the Python tech-acquired event keeps its original signature below. (GPT-5.6-Sol + GPT-5.6 Thinking) -->
void CvEventReporter::techAcquired(TechTypes eType, TeamTypes eTeam, PlayerTypes ePlayer, bool bAnnounce, TechAcquisitionCause eCause)
{
	if (gGameRecordLogLevel >= 2) logSASGameRecordTechAcquired(eType, eTeam, ePlayer, eCause);
	m_kPythonEventMgr.reportTechAcquired(eType, eTeam, ePlayer, bAnnounce);
}

void CvEventReporter::techSelected(TechTypes eTech, PlayerTypes ePlayer)
{
	m_kPythonEventMgr.reportTechSelected(eTech, ePlayer);
}

void CvEventReporter::religionFounded(ReligionTypes eType, PlayerTypes ePlayer)
{
	if (gGameRecordLogLevel >= 2) logSASGameRecordReligionFounded(eType, ePlayer);
	m_kPythonEventMgr.reportReligionFounded(eType, ePlayer);
	m_kStatistics.religionFounded(eType, ePlayer);
}

void CvEventReporter::religionSpread(ReligionTypes eType, PlayerTypes ePlayer, CvCity* pSpreadCity)
{
	// <!-- custom: Successful city religion membership changes are strategically relevant to diplomacy, happiness and vote-source membership.
	// Preserve only the realized change, not missionary/AI candidate reasoning. (ChatGPT-5.6-Sol) -->
	if (gGameRecordLogLevel >= 2) logSASGameRecordReligionChanged(eType, ePlayer, pSpreadCity, true);
	m_kPythonEventMgr.reportReligionSpread(eType, ePlayer, pSpreadCity);
}

void CvEventReporter::religionRemove(ReligionTypes eType, PlayerTypes ePlayer, CvCity* pSpreadCity)
{
	if (gGameRecordLogLevel >= 2) logSASGameRecordReligionChanged(eType, ePlayer, pSpreadCity, false);
	m_kPythonEventMgr.reportReligionRemove(eType, ePlayer, pSpreadCity);
}

void CvEventReporter::corporationFounded(CorporationTypes eType, PlayerTypes ePlayer)
{
	if (gGameRecordLogLevel >= 2) logSASGameRecordCorporationFounded(eType, ePlayer);
	m_kPythonEventMgr.reportCorporationFounded(eType, ePlayer);
}

void CvEventReporter::corporationSpread(CorporationTypes eType, PlayerTypes ePlayer, CvCity* pSpreadCity)
{
	// <!-- custom: Corporation membership can materially change city yields, maintenance and resource consumption.
	// Keep the authoritative successful spread/removal in factual history while leaving executive AI reasoning elsewhere. (ChatGPT-5.6-Sol) -->
	if (gGameRecordLogLevel >= 2) logSASGameRecordCorporationChanged(eType, ePlayer, pSpreadCity, true);
	m_kPythonEventMgr.reportCorporationSpread(eType, ePlayer, pSpreadCity);
}

void CvEventReporter::corporationRemove(CorporationTypes eType, PlayerTypes ePlayer, CvCity* pSpreadCity)
{
	if (gGameRecordLogLevel >= 2) logSASGameRecordCorporationChanged(eType, ePlayer, pSpreadCity, false);
	m_kPythonEventMgr.reportCorporationRemove(eType, ePlayer, pSpreadCity);
}

void CvEventReporter::goldenAge(PlayerTypes ePlayer)
{
	if (gGameRecordLogLevel >= 2) logSASGameRecordGoldenAge(ePlayer, true);
	m_kPythonEventMgr.reportGoldenAge(ePlayer);
	m_kStatistics.goldenAge(ePlayer);
}

void CvEventReporter::endGoldenAge(PlayerTypes ePlayer)
{
	if (gGameRecordLogLevel >= 2) logSASGameRecordGoldenAge(ePlayer, false);
	m_kPythonEventMgr.reportEndGoldenAge(ePlayer);
}

void CvEventReporter::changeWar(bool bWar, TeamTypes eTeam, TeamTypes eOtherTeam)
{
	// <!-- custom: Completed-war summaries are logged in CvTeam::makePeace because this later callback runs after AI_postMakePeace has erased both teams' final war-success values. (GPT-5.6-Sol) -->
	m_kPythonEventMgr.reportChangeWar(bWar, eTeam, eOtherTeam);
}

void CvEventReporter::setPlayerAlive(PlayerTypes ePlayerID, bool bNewValue)
{
	m_kPythonEventMgr.reportSetPlayerAlive(ePlayerID, bNewValue);
}

void CvEventReporter::playerChangeStateReligion(PlayerTypes ePlayerID, ReligionTypes eNewReligion, ReligionTypes eOldReligion)
{
	if (gGameRecordLogLevel >= 2 && GC.getGame().isFinalInitialized()) logSASGameRecordLastStateReligionChanged(ePlayerID, eOldReligion, eNewReligion);
	m_kPythonEventMgr.reportPlayerChangeStateReligion(ePlayerID, eNewReligion, eOldReligion);
}

void CvEventReporter::playerGoldTrade(PlayerTypes eFromPlayer, PlayerTypes eToPlayer, int iAmount)
{
	if (gGameRecordLogLevel >= 3) logSASGameRecordPlayerGoldTrade(eFromPlayer, eToPlayer, iAmount);
	m_kPythonEventMgr.reportPlayerGoldTrade(eFromPlayer, eToPlayer, iAmount);
}

/*	advc.make: To get rid of the K-Mod friend declaration in the header.
	Not const because CvStatistics performs lazy initialization of player records. */
CvPlayerRecord const* CvEventReporter::getPlayerRecord(PlayerTypes ePlayer)
{
	return m_kStatistics.getPlayerRecord(ePlayer);
}

void CvEventReporter::chat(CvWString szString)
{
	m_kPythonEventMgr.reportChat(szString);
}

void CvEventReporter::victory(TeamTypes eWinner, VictoryTypes eVictory)
{
	if (gGameRecordLogLevel > 0) logSASGameRecordVictory(eWinner, eVictory);
	m_kPythonEventMgr.reportVictory(eWinner, eVictory);
	m_kStatistics.setVictory(eWinner, eVictory);

	// Set all human player's final total time played
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (GET_PLAYER((PlayerTypes)i).isEverAlive())
		{
			m_kStatistics.setTimePlayed((PlayerTypes)i, GET_PLAYER((PlayerTypes)i).getTotalTimePlayed());
		}
	}
	gDLL->reportStatistics(); // automatically report MP stats on victory
}

void CvEventReporter::vassalState(TeamTypes eMaster, TeamTypes eVassal, bool bVassal)
{
	if (gGameRecordLogLevel >= 2) logSASGameRecordVassalState(eMaster, eVassal, bVassal);
	m_kPythonEventMgr.reportVassalState(eMaster, eVassal, bVassal);
}

void CvEventReporter::preSave()
{
	m_kPythonEventMgr.preSave();
	/*  <advc.106l> The original "saving" messages are disabled through game text XML
		b/c the EXE displays them for too long. Will show replacement messages here. */
	bool bAutoSave = m_bPreAutoSave;
	bool bQuickSave = m_bPreQuickSave;
	m_bPreAutoSave = m_bPreQuickSave = false;
	FAssertMsg(bAutoSave || !GC.getGame().isInBetweenTurns() ||
			GC.getInitCore().getPbem() || GC.getGame().isNetworkMultiPlayer(),
			"Quicksave in between turns?");
	char const* szDefineName = "";
	CvWString szMsgTag;
	if(bAutoSave)
	{
		szDefineName = "AUTO_SAVING_MESSAGE_TIME";
		szMsgTag = L"TXT_KEY_AUTOSAVING2";
	}
	else if(bQuickSave)
	{
		szDefineName = "QUICK_SAVING_MESSAGE_TIME";
		szMsgTag = L"TXT_KEY_QUICK_SAVING2";
	}
	else
	{
		szDefineName = "SAVING_MESSAGE_TIME";
		szMsgTag = L"TXT_KEY_SAVING_GAME2";
	}
	int iLength = GC.getDefineINT(szDefineName);
	if(iLength <= 0)
		return;
	PlayerTypes eActivePlayer = GC.getGame().getActivePlayer();
	if(eActivePlayer == NO_PLAYER)
	{
		FAssert(eActivePlayer != NO_PLAYER);
		return;
	}
	gDLL->UI().addMessage(eActivePlayer, true,
			iLength, gDLL->getText(szMsgTag), NULL, MESSAGE_TYPE_DISPLAY_ONLY);
}

void CvEventReporter::preAutoSave()
{
	/*  Can detect failed auto-saves here, but only if AutoSaveInterval=1 in the INI.
		Can't test in the DLL if that's the case. (Can't parse the INI file either;
		it could be any file passed to the EXE at startup through ini="...") */
	//FAssertMsg(!m_bPreAutoSave || GC.getGame().isNetworkMultiPlayer(), "Should've been reset by preSave");
	m_bPreAutoSave = true;
}

void CvEventReporter::preQuickSave()
{
	//FAssertMsg(!m_bPreAutoSave || GC.getGame().isNetworkMultiPlayer(), "Should've been reset by preSave");
	m_bPreQuickSave = true;
} // </advc.106l>

void CvEventReporter::windowActivation(bool bActive)
{
	m_kPythonEventMgr.reportWindowActivation(bActive);
}

void CvEventReporter::getGameStatistics(std::vector<CvStatBase*>& aStats)
{
	aStats.clear();
	aStats.push_back(new CvStatString("mapname", m_kStatistics.getMapName()));
	aStats.push_back(new CvStatInt("era", m_kStatistics.getEra()));

	// Report game params governing some server-side loops
	aStats.push_back(new CvStatInt("numplayers", MAX_CIV_PLAYERS));
	aStats.push_back(new CvStatInt("numunittypes", GC.getNumUnitInfos()));
	aStats.push_back(new CvStatInt("numbuildingtypes", GC.getNumBuildingInfos()));
	aStats.push_back(new CvStatInt("numreligiontypes", GC.getNumReligionInfos()));
}

void CvEventReporter::getPlayerStatistics(PlayerTypes ePlayer, std::vector<CvStatBase*>& aStats)
{
	aStats.clear();
	CvPlayerRecord* pRecord = m_kStatistics.getPlayerRecord(ePlayer);
	if (pRecord != NULL)
	{
		aStats.push_back(new CvStatInt("victorytype", pRecord->getVictory()));
		aStats.push_back(new CvStatInt("timeplayed", pRecord->getMinutesPlayed()));
		aStats.push_back(new CvStatInt("leader", pRecord->getLeader()-1));  // -1 because index 0 is barb
		aStats.push_back(new CvStatInt("citiesbuilt", pRecord->getNumCitiesBuilt()));
		aStats.push_back(new CvStatInt("citiesrazed", pRecord->getNumCitiesRazed()));
		aStats.push_back(new CvStatInt("goldenages", pRecord->getNumGoldenAges()));

		CvString strKey;
		FOR_EACH_ENUM(Unit)
		{
			strKey.format("unit_%d_built", eLoopUnit);
			aStats.push_back(new CvStatInt(strKey,
					pRecord->getNumUnitsBuilt(eLoopUnit)));
			strKey.format("unit_%d_killed", eLoopUnit);
			aStats.push_back(new CvStatInt(strKey,
					pRecord->getNumUnitsKilled(eLoopUnit)));
			strKey.format("unit_%d_lost", eLoopUnit);
			aStats.push_back(new CvStatInt(strKey,
					pRecord->getNumUnitsWasKilled(eLoopUnit)));
		}

		FOR_EACH_ENUM(Building)
		{
			strKey.format("building_%d_built", eLoopBuilding);
			aStats.push_back(new CvStatInt(strKey,
					pRecord->getNumBuildingsBuilt(eLoopBuilding)));
		}

		FOR_EACH_ENUM(Religion)
		{
			strKey.format("religion_%d_founded", eLoopReligion);
			aStats.push_back(new CvStatInt(strKey,
					pRecord->getReligionFounded(eLoopReligion)));
		}
	}
}

void CvEventReporter::readStatistics(FDataStreamBase* pStream)
{
	m_kStatistics.reset();
	m_kStatistics.read(pStream);
	GC.getGame().onAllGameDataRead(); // advc
}

void CvEventReporter::writeStatistics(FDataStreamBase* pStream)
{
	PROFILE_FUNC(); // advc
	REPRO_TEST_BEGIN_WRITE("Statistics");
	m_kStatistics.write(pStream);
	REPRO_TEST_FINAL_WRITE();
}
