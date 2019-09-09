#include "CvGameCoreDLL.h"
#include "CvPlayerAI.h"
#include "CvTeamAI.h"
#include "CvGameAI.h"
#include "CvSelectionGroupAI.h"

/*  advc.003u: New file for dummy/ adapter implementations of AI_... functions
	that shouldn't have been made virtual, but need to be preserved for the EXE.
	When a function prototype is changed in one of the AI classes, it may be
	necessary to change the call location here; the compiler will point this out. */

// CvPlayer ...

/*void CvPlayer::AI_initExternal() {
	AI().AI_init();
}
void CvPlayer::AI_resetExternal(bool bConstructor) {
	AI().AI_reset(bConstructor);
}
void CvPlayer::AI_doTurnPreExternal() {
	AI().AI_doTurnPre();
}*/
void CvPlayer::AI_doTurnPostExternal() {
	AI().AI_doTurnPost();
}
void CvPlayer::AI_doTurnUnitsPreExternal() {
	AI().AI_doTurnUnitsPre();
}
void CvPlayer::AI_doTurnUnitsPostExternal() {
	AI().AI_doTurnUnitsPost();
}
void CvPlayer::AI_updateFoundValuesExternal(bool bStartingLoc) {
	AI().AI_updateFoundValues(bStartingLoc);
}
void CvPlayer::AI_unitUpdateExternal() {
	AI().AI_unitUpdate();
}
void CvPlayer::AI_makeAssignWorkDirtyExternal() {
	AI().AI_makeAssignWorkDirty();
}
void CvPlayer::AI_assignWorkingPlotsExternal() {
	AI().AI_assignWorkingPlots();
}
void CvPlayer::AI_updateAssignWorkExternal() {
	AI().AI_updateAssignWork();
}
void CvPlayer::AI_makeProductionDirtyExternal() {
	AI().AI_makeProductionDirty();
}
void CvPlayer::AI_conquerCityExternal(CvCity* pCity) {
	AI().AI_conquerCity(*reinterpret_cast<CvCityAI*>(pCity)); // advc
}
int CvPlayer::AI_foundValueExternal(int iX, int iY, int iMinUnitRange, bool bStartingLoc) {
	return AI().AI_foundValue(iX, iY, iMinUnitRange, bStartingLoc);
}
bool CvPlayer::AI_isCommercePlotExternal(CvPlot* pPlot) {
	return AI().AI_isCommercePlot(pPlot);
}
int CvPlayer::AI_getPlotDangerExternal(CvPlot* pPlot, int iRange, bool bTestMoves) {
	return AI().AI_getPlotDanger(*pPlot, iRange, bTestMoves); // advc: CvPlot* -> CvPlot&
}
bool CvPlayer::AI_isFinancialTroubleExternal() {
	return AI().AI_isFinancialTrouble();
}
TechTypes CvPlayer::AI_bestTechExternal(int iMaxPathLength, bool bIgnoreCost, bool bAsync, TechTypes eIgnoreTech, AdvisorTypes eIgnoreAdvisor) {
	return AI().AI_bestTech(iMaxPathLength, bIgnoreCost, bAsync, eIgnoreTech, eIgnoreAdvisor);
}
void CvPlayer::AI_chooseFreeTechExternal() {
	AI().AI_chooseFreeTech();
}
void CvPlayer::AI_chooseResearchExternal() {
	AI().AI_chooseResearch();
}
bool CvPlayer::AI_isWillingToTalkExternal(PlayerTypes ePlayer) {
	return AI().AI_isWillingToTalk(ePlayer);
}
bool CvPlayer::AI_demandRebukedSneakExternal(PlayerTypes ePlayer) {
	return AI().AI_demandRebukedSneak(ePlayer);
}
bool CvPlayer::AI_demandRebukedWarExternal(PlayerTypes ePlayer) {
	return AI().AI_demandRebukedWar(ePlayer);
}
AttitudeTypes CvPlayer::AI_getAttitudeExternal(PlayerTypes ePlayer, bool bForced) {
	return AI().AI_getAttitude(ePlayer, bForced);
}
PlayerVoteTypes CvPlayer::AI_diploVoteExternal(VoteSelectionSubData& kVoteData, VoteSourceTypes eVoteSource, bool bPropose) {
	return AI().AI_diploVote(kVoteData, eVoteSource, bPropose);
}
int CvPlayer::AI_dealValExternal(PlayerTypes ePlayer, CLinkList<TradeData>* pList, bool bIgnoreAnnual, int iExtra) {
	return AI().AI_dealVal(ePlayer, pList, bIgnoreAnnual, iExtra);
}
bool CvPlayer::AI_considerOfferExternal(PlayerTypes ePlayer, CLinkList<TradeData>* pTheirList, CLinkList<TradeData>* pOurList, int iChange) {
	return AI().AI_considerOffer(ePlayer, *pTheirList, *pOurList, iChange); // advc: The list params are now references
}
bool CvPlayer::AI_counterProposeExternal(PlayerTypes ePlayer, CLinkList<TradeData>* pTheirList, CLinkList<TradeData>* pOurList, CLinkList<TradeData>* pTheirInventory, CLinkList<TradeData>* pOurInventory, CLinkList<TradeData>* pTheirCounter, CLinkList<TradeData>* pOurCounter) {
	return AI().AI_counterPropose(ePlayer, pTheirList, pOurList, pTheirInventory, pOurInventory, pTheirCounter, pOurCounter);
}
int CvPlayer::AI_bonusValExternal(BonusTypes eBonus, int iChange) {
	return AI().AI_bonusVal(eBonus, iChange);
}
int CvPlayer::AI_bonusTradeValExternal(BonusTypes eBonus, PlayerTypes ePlayer, int iChange) {
	return AI().AI_bonusTradeVal(eBonus, ePlayer, iChange);
}
DenialTypes CvPlayer::AI_bonusTradeExternal(BonusTypes eBonus, PlayerTypes ePlayer) {
	return AI().AI_bonusTrade(eBonus, ePlayer);
}
int CvPlayer::AI_cityTradeValExternal(CvCity* pCity) {
	return AI().AI_cityTradeVal(*reinterpret_cast<CvCityAI*>(pCity)); // advc
}
DenialTypes CvPlayer::AI_cityTradeExternal(CvCity* pCity, PlayerTypes ePlayer) {
	return AI().AI_cityTrade(*reinterpret_cast<CvCityAI*>(pCity), ePlayer); // advc
}
DenialTypes CvPlayer::AI_stopTradingTradeExternal(TeamTypes eTradeTeam, PlayerTypes ePlayer) {
	return AI().AI_stopTradingTrade(eTradeTeam, ePlayer);
}
DenialTypes CvPlayer::AI_civicTradeExternal(CivicTypes eCivic, PlayerTypes ePlayer) {
	return AI().AI_civicTrade(eCivic, ePlayer);
}
DenialTypes CvPlayer::AI_religionTradeExternal(ReligionTypes eReligion, PlayerTypes ePlayer) {
	return AI().AI_religionTrade(eReligion, ePlayer);
}
int CvPlayer::AI_unitValueExternal(UnitTypes eUnit, UnitAITypes eUnitAI, CvArea* pArea) {
	return AI().AI_unitValue(eUnit, eUnitAI, pArea);
}
int CvPlayer::AI_totalUnitAIsExternal(UnitAITypes eUnitAI) {
	return AI().AI_totalUnitAIs(eUnitAI);
}
int CvPlayer::AI_totalAreaUnitAIsExternal(CvArea* pArea, UnitAITypes eUnitAI) {
	return AI().AI_totalAreaUnitAIs(pArea, eUnitAI);
}
int CvPlayer::AI_totalWaterAreaUnitAIsExternal(CvArea* pArea, UnitAITypes eUnitAI) {
	return AI().AI_totalWaterAreaUnitAIs(pArea, eUnitAI);
}
int CvPlayer::AI_plotTargetMissionAIsExternal(CvPlot* pPlot, MissionAITypes eMissionAI, CvSelectionGroup* pSkipSelectionGroup, int iRange) {
	return AI().AI_plotTargetMissionAIs(pPlot, eMissionAI, pSkipSelectionGroup, iRange);
}
int CvPlayer::AI_unitTargetMissionAIsExternal(CvUnit* pUnit, MissionAITypes eMissionAI, CvSelectionGroup* pSkipSelectionGroup) {
	return AI().AI_unitTargetMissionAIs(pUnit, eMissionAI, pSkipSelectionGroup);
}
int CvPlayer::AI_civicValueExternal(CivicTypes eCivic) {
	return AI().AI_civicValue(eCivic);
}
int CvPlayer::AI_getNumAIUnitsExternal(UnitAITypes eIndex) {
	return AI().AI_getNumAIUnits(eIndex);
}
void CvPlayer::AI_changePeacetimeTradeValueExternal(PlayerTypes eIndex, int iChange) {
	AI().AI_processPeacetimeTradeValue(eIndex, iChange); // advc.130p (renamed)
}
void CvPlayer::AI_changePeacetimeGrantValueExternal(PlayerTypes eIndex, int iChange) {
	AI().AI_processPeacetimeGrantValue(eIndex, iChange); // advc.130p (renamed)
}
int CvPlayer::AI_getAttitudeExtraExternal(PlayerTypes eIndex) {
	return AI().AI_getAttitudeExtra(eIndex);
}
void CvPlayer::AI_setAttitudeExtraExternal(PlayerTypes eIndex, int iNewValue) {
	AI().AI_setAttitudeExtra(eIndex, iNewValue);
}
void CvPlayer::AI_changeAttitudeExtraExternal(PlayerTypes eIndex, int iChange) {
	AI().AI_changeAttitudeExtra(eIndex, iChange);
}
void CvPlayer::AI_setFirstContactExternal(PlayerTypes eIndex, bool bNewValue) {
	AI().AI_setFirstContact(eIndex, bNewValue);
}
int CvPlayer::AI_getMemoryCountExternal(PlayerTypes eIndex1, MemoryTypes eIndex2) {
	return AI().AI_getMemoryCount(eIndex1, eIndex2);
}
void CvPlayer::AI_changeMemoryCountExternal(PlayerTypes eIndex1, MemoryTypes eIndex2, int iChange) {
	AI().AI_changeMemoryCount(eIndex1, eIndex2, iChange);
}
void CvPlayer::AI_doCommerceExternal() {
	AI().AI_doCommerce();
}
EventTypes CvPlayer::AI_chooseEventExternal(int iTriggeredId) {
	return AI().AI_chooseEvent(iTriggeredId);
}
void CvPlayer::AI_launchExternal(VictoryTypes eVictory) {
	AI().AI_launch(eVictory);
}
void CvPlayer::AI_doAdvancedStartExternal(bool bNoExit) {
	AI().AI_doAdvancedStart(bNoExit);
}
void CvPlayer::AI_updateBonusValueExternal() {
	AI().AI_updateBonusValue();
}
void CvPlayer::AI_updateBonusValueExternal(BonusTypes eBonus) { 
	AI().AI_updateBonusValue(eBonus);
}
ReligionTypes CvPlayer::AI_chooseReligionExternal() {
	return AI().AI_chooseReligion();
}
int CvPlayer::AI_getExtraGoldTargetExternal() {
	return AI().AI_getExtraGoldTarget();
}
void CvPlayer::AI_setExtraGoldTargetExternal(int iNewValue) {
	AI().AI_setExtraGoldTarget(iNewValue);
}
int CvPlayer::AI_maxGoldPerTurnTradeExternal(PlayerTypes ePlayer) {
	return AI().AI_maxGoldPerTurnTrade(ePlayer);
}
// Called when a human player adds AI gold to the trade table
int CvPlayer::AI_maxGoldTradeExternal(PlayerTypes ePlayer) {
	return AI().AI_maxGoldTrade(ePlayer);
}
void CvPlayer::readExternal(FDataStreamBase* pStream) {
	read(pStream);
}
void CvPlayer::writeExternal(FDataStreamBase* pStream) {
	write(pStream);
}

// CvTeam ...

/*void CvTeam::AI_initExternal() {
	AI().AI_init();
}
void CvTeam::AI_resetExternal(bool bConstructor) {
	AI().AI_reset(bConstructor);
}
void CvTeam::AI_doTurnPreExternal() {
	AI().AI_doTurnPre();
}*/
void CvTeam::AI_doTurnPostExternal() {
	AI().AI_doTurnPost();
}
void CvTeam::AI_makeAssignWorkDirtyExternal() {
	AI().AI_makeAssignWorkDirty();
}
void CvTeam::AI_updateAreaStrategiesExternal(bool bTargets) {
	AI().AI_updateAreaStrategies(bTargets);
}
bool CvTeam::AI_shareWarExternal(TeamTypes eTeam) {
	return AI().AI_shareWar(eTeam);
}
void CvTeam::AI_updateWorstEnemyExternal() {
	AI().AI_updateWorstEnemy();
}
int CvTeam::AI_getAtWarCounterExternal(TeamTypes eIndex) {
	return AI().AI_getAtWarCounter(eIndex);
}
void CvTeam::AI_setAtWarCounterExternal(TeamTypes eIndex, int iNewValue) {
	AI().AI_setAtWarCounter(eIndex, iNewValue);
}
int CvTeam::AI_getAtPeaceCounterExternal(TeamTypes eIndex) {
	return AI().AI_getAtPeaceCounter(eIndex);
}
void CvTeam::AI_setAtPeaceCounterExternal(TeamTypes eIndex, int iNewValue) {
	AI().AI_setAtPeaceCounter(eIndex, iNewValue);
}
int CvTeam::AI_getHasMetCounterExternal(TeamTypes eIndex) {
	return AI().AI_getHasMetCounter(eIndex);
}
void CvTeam::AI_setHasMetCounterExternal(TeamTypes eIndex, int iNewValue) {
	AI().AI_setHasMetCounter(eIndex, iNewValue);
}
int CvTeam::AI_getOpenBordersCounterExternal(TeamTypes eIndex) {
	return AI().AI_getOpenBordersCounter(eIndex);
}
void CvTeam::AI_setOpenBordersCounterExternal(TeamTypes eIndex, int iNewValue) {
	AI().AI_setOpenBordersCounter(eIndex, iNewValue);
}
int CvTeam::AI_getDefensivePactCounterExternal(TeamTypes eIndex) {
	return AI().AI_getDefensivePactCounter(eIndex);
}
void CvTeam::AI_setDefensivePactCounterExternal(TeamTypes eIndex, int iNewValue) {
	AI().AI_setDefensivePactCounter(eIndex, iNewValue);
}
int CvTeam::AI_getShareWarCounterExternal(TeamTypes eIndex) {
	return AI().AI_getShareWarCounter(eIndex);
}
void CvTeam::AI_setShareWarCounterExternal(TeamTypes eIndex, int iNewValue) {
	AI().AI_setShareWarCounter(eIndex, iNewValue);
}
int CvTeam::AI_getWarSuccessExternal(TeamTypes eIndex) {
	return AI().AI_getWarSuccess(eIndex);
}
void CvTeam::AI_setWarSuccessExternal(TeamTypes eIndex, int iNewValue) {
	AI().AI_setWarSuccess(eIndex, iNewValue);
}
void CvTeam::AI_changeWarSuccessExternal(TeamTypes eIndex, int iChange) {
	AI().AI_changeWarSuccess(eIndex, iChange);
}
int CvTeam::AI_getEnemyPeacetimeTradeValueExternal(TeamTypes eIndex) {
	return AI().AI_getEnemyPeacetimeTradeValue(eIndex);
}
void CvTeam::AI_setEnemyPeacetimeTradeValueExternal(TeamTypes eIndex, int iNewValue) {
	AI().AI_setEnemyPeacetimeTradeValue(eIndex, iNewValue);
}
int CvTeam::AI_getEnemyPeacetimeGrantValueExternal(TeamTypes eIndex) {
	return AI().AI_getEnemyPeacetimeGrantValue(eIndex);
}
void CvTeam::AI_setEnemyPeacetimeGrantValueExternal(TeamTypes eIndex, int iNewValue) {
	AI().AI_setEnemyPeacetimeGrantValue(eIndex, iNewValue);
}
WarPlanTypes CvTeam::AI_getWarPlanExternal(TeamTypes eIndex) {
	return AI().AI_getWarPlan(eIndex);
}
bool CvTeam::AI_isChosenWarExternal(TeamTypes eIndex) {
	return AI().AI_isChosenWar(eIndex);
}
bool CvTeam::AI_isSneakAttackPreparingExternal(TeamTypes eIndex) {
	return AI().AI_isSneakAttackPreparing(eIndex);
}
bool CvTeam::AI_isSneakAttackReadyExternal(TeamTypes eIndex) {
	return AI().AI_isSneakAttackReady(eIndex);
}
void CvTeam::AI_setWarPlanExternal(TeamTypes eIndex, WarPlanTypes eNewValue, bool bWar) {
	AI().AI_setWarPlan(eIndex, eNewValue, bWar);
}
void CvTeam::readExternal(FDataStreamBase* pStream) {
	read(pStream);
}
void CvTeam::writeExternal(FDataStreamBase* pStream) {
	write(pStream);
}

// CvGame ...

/*void CvGame::AI_initExternal() {
	AI().AI_init();
}*/
void CvGame::AI_resetExternal() {
	AI().AI_reset();
}
void CvGame::AI_makeAssignWorkDirtyExternal() {
	AI().AI_makeAssignWorkDirty();
}
void CvGame::AI_updateAssignWorkExternal() {
	AI().AI_updateAssignWork();
}
int CvGame::AI_combatValueExternal(UnitTypes eUnit) {
	return AI().AI_combatValue(eUnit);
}

// CvSelectionGroup ...

/*void CvSelectionGroup::AI_initExternal() {
	AI().AI_init();
}*/
void CvSelectionGroup::AI_resetExternal() {
	AI().AI_reset();
}
void CvSelectionGroup::AI_separateExternal() {
	AI().AI_separate();
}
bool CvSelectionGroup::AI_updateExternal() {
	return AI().AI_update();
}
int CvSelectionGroup::AI_attackOddsExternal(CvPlot* pPlot, bool bPotentialEnemy) {
	return AI().AI_attackOdds(pPlot, bPotentialEnemy);
}
CvUnit* CvSelectionGroup::AI_getBestGroupAttackerExternal(CvPlot* pPlot, bool bPotentialEnemy, int& iUnitOdds, bool bForce, bool bNoBlitz) {
	return AI().AI_getBestGroupAttacker(pPlot, bPotentialEnemy, iUnitOdds, bForce, bNoBlitz);
}
CvUnit* CvSelectionGroup::AI_getBestGroupSacrificeExternal(CvPlot* pPlot, bool bPotentialEnemy, bool bForce, bool bNoBlitz) {
	return AI().AI_getBestGroupSacrifice(pPlot, bPotentialEnemy, bForce, bNoBlitz);
}
// (Not called:)
int CvSelectionGroup::AI_compareStacksExternal(CvPlot* pPlot, bool bPotentialEnemy, bool bCheckCanAttack, bool bCheckCanMove) {
	return AI().AI_compareStacks(pPlot, bCheckCanAttack); // K-Mod has removed bPotentialEnemy and bCheckCanMove
}
int CvSelectionGroup::AI_sumStrengthExternal(CvPlot* pAttackedPlot, DomainTypes eDomainType, bool bCheckCanAttack, bool bCheckCanMove) {
	return AI().AI_sumStrength(pAttackedPlot, eDomainType, bCheckCanAttack); // K-Mod has removed bCheckCanMove
}
void CvSelectionGroup::AI_queueGroupAttackExternal(int iX, int iY) {
	AI().AI_queueGroupAttack(iX, iY);
}
void CvSelectionGroup::AI_cancelGroupAttackExternal() {
	AI().AI_cancelGroupAttack();
}
bool CvSelectionGroup::AI_isGroupAttackExternal() {
	return AI().AI_isGroupAttack();
}
bool CvSelectionGroup::AI_isControlledExternal() {
	return AI().AI_isControlled();
}
bool CvSelectionGroup::AI_isDeclareWarExternal(CvPlot* pPlot) {
	return AI().AI_isDeclareWar(pPlot);
}
CvPlot* CvSelectionGroup::AI_getMissionAIPlotExternal() {
	return AI().AI_getMissionAIPlot();
}
bool CvSelectionGroup::AI_isForceSeparateExternal() {
	return AI().AI_isForceSeparate();
}
void CvSelectionGroup::AI_makeForceSeparateExternal() {
	AI().AI_setForceSeparate(); // Call K-Mod replacement
}
MissionAITypes CvSelectionGroup::AI_getMissionAITypeExternal() {
	return AI().AI_getMissionAIType();
}
void CvSelectionGroup::AI_setMissionAIExternal(MissionAITypes eNewMissionAI, CvPlot* pNewPlot, CvUnit* pNewUnit) {
	AI().AI_setMissionAI(eNewMissionAI, pNewPlot, pNewUnit);
}
CvUnit* CvSelectionGroup::AI_getMissionAIUnitExternal() {
	return AI().AI_getMissionAIUnit();
}
CvUnit* CvSelectionGroup::AI_ejectBestDefenderExternal(CvPlot* pTargetPlot) {
	return AI().AI_ejectBestDefender(pTargetPlot);
}
void CvSelectionGroup::AI_separateNonAIExternal(UnitAITypes eUnitAI) {
	AI().AI_separateNonAI(eUnitAI);
}
void CvSelectionGroup::AI_separateAIExternal(UnitAITypes eUnitAI) {
	AI().AI_separateAI(eUnitAI);
}
// Not called:
/*bool CvSelectionGroup::AI_isFullExternal() {
	return AI().AI_isFull();
}*/
