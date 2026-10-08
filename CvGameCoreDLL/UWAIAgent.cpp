#include "CvGameCoreDLL.h"
#include "UWAIAgent.h"
#include "WarEvaluator.h"
#include "WarEvalParameters.h"
#include "MilitaryBranch.h"
#include "CvInfo_GameOption.h"
#include "CvInfo_Building.h" // Just for vote-related info
#include "CoreAI.h"
#include "CvCityAI.h"
#include "CvDiploParameters.h"
#include "CvGameCoreUtils.h" // <!-- custom: Shared raw WarPlanTypes token text and victory-stage helpers for structured war diagnostics. (GPT-5.5) -->
#include "CvUnit.h" // <!-- custom: Required by WAR target-opportunity diagnostics that inspect candidate-team military locations. (GPT-5.6-Sol) -->
#include "TeamPathFinder.h"
#include "CvArea.h"
#include "RiseFall.h" // advc.705
#include "BBAILog.h" // <!-- custom: Dedicated SAS war diagnostics log UWAI target utility and distance context separately from broad TEAM logging. (GPT-5.5) -->
#include "SASGameRecordLog.h" // <!-- custom: Preserve compact foreground-UWAI war-target intent and realized war-plan lifecycle causes in the broad game record; exhaustive candidate/review reasoning remains in BBAI. (ChatGPT-5.6-Sol) -->

using std::vector;
using std::set;

namespace
{
	int const iMaxReparationUtility = 25;
	int const iWarTradeUtilityThresh = -37;
	// AI payments for peace (with human or AI enemy)
	scaled const rReparationsModifierAI = fixp(0.5);
	/*  Modifier for human payments for peace, i.e what the AI asks a human to pay
		(no modifier for brokering, i.e. 100%). */
	scaled const rReparationsModifierHuman = fixp(0.75);

	int getSASBBAINearestCityDistance(TeamTypes eFrom, TeamTypes eTo)
	{
		int iBestDistance = MAX_INT;
		for (MemberAIIter itFrom(eFrom); itFrom.hasNext(); ++itFrom)
		{
			FOR_EACH_CITY(pFromCity, *itFrom)
			{
				for (MemberAIIter itTo(eTo); itTo.hasNext(); ++itTo)
				{
					FOR_EACH_CITY(pToCity, *itTo)
					{
						iBestDistance = std::min(iBestDistance, plotDistance(pFromCity->getX(), pFromCity->getY(), pToCity->getX(), pToCity->getY()));
					}
				}
			}
		}
		return (iBestDistance == MAX_INT ? -1 : iBestDistance);
	}

	int getSASBBAITargetPowerPercent(CvTeamAI const& kAgent, TeamTypes eTarget)
	{
		return (100 * GET_TEAM(eTarget).getDefensivePower(kAgent.getID())) / std::max(1, kAgent.getPower(true));
	}

	void getSASBBAITeamVictoryStages(TeamTypes eTeam, int& iCultureStage, int& iSpaceStage, int& iConquestStage, int& iDominationStage, int& iDiplomacyStage)
	{
		iCultureStage = 0;
		iSpaceStage = 0;
		iConquestStage = 0;
		iDominationStage = 0;
		iDiplomacyStage = 0;
		for (MemberAIIter it(eTeam); it.hasNext(); ++it)
		{
			AIVictoryStage const eVictoryStageHash = it->AI_getVictoryStageHash();
			iCultureStage = std::max(iCultureStage, getSASCultureVictoryStageLevel(eVictoryStageHash));
			iSpaceStage = std::max(iSpaceStage, getSASSpaceVictoryStageLevel(eVictoryStageHash));
			iConquestStage = std::max(iConquestStage, getSASConquestVictoryStageLevel(eVictoryStageHash));
			iDominationStage = std::max(iDominationStage, getSASDominationVictoryStageLevel(eVictoryStageHash));
			iDiplomacyStage = std::max(iDiplomacyStage, getSASDiplomacyVictoryStageLevel(eVictoryStageHash));
		}
	}

	int getSASBBAIVictoryDenialUtilityBoost(TeamTypes eTarget, int iTargetMaxVictoryStage)
	{
		static const bool bEnable = GC.getDefineBOOL("SAS_UWAI_VICTORY_DENIAL_ENABLE");
		if (!bEnable)
			return 0;
		int iBoost = 0;
		int const iCountdown = GET_TEAM(eTarget).AI_getLowestVictoryCountdown();
		static const int iMaxCountdownBoostNormal = GC.getDefineINT("SAS_UWAI_VICTORY_DENIAL_MAX_COUNTDOWN_TURNS_NORMAL_GAMESPEED_BOOST");
		int const iMaxCountdownBoost = getSASVictoryDelayTurnsFromNormalGameSpeed(iMaxCountdownBoostNormal);
		if (iCountdown >= 0 && iCountdown <= iMaxCountdownBoost)
		{
			static const int iCountdownUtilityBoost = GC.getDefineINT("SAS_UWAI_VICTORY_DENIAL_COUNTDOWN_UTILITY_BOOST");
			iBoost += (iCountdownUtilityBoost * (iMaxCountdownBoost + 1 - iCountdown)) / std::max(1, iMaxCountdownBoost + 1);
		}
		if (iTargetMaxVictoryStage >= 4)
		{
			static const int iStage4UtilityBoost = GC.getDefineINT("SAS_UWAI_VICTORY_DENIAL_STAGE4_UTILITY_BOOST");
			iBoost += iStage4UtilityBoost;
		}
		if (isSASTeamStage3SpaceVictoryThreat(eTarget))
		{
			static const int iStage3SpaceUtilityBoost = GC.getDefineINT("SAS_UWAI_VICTORY_DENIAL_STAGE3_SPACE_UTILITY_BOOST");
			iBoost += iStage3SpaceUtilityBoost;
		}
		return iBoost;
	}

	void logSASBBAIWarTargetVictoryContext(CvTeamAI const& kAgent, TeamTypes eTarget, char const* szRow, WarPlanTypes eWarPlan, int iUtility, int iDrivePercent, int iTargetRank, int iCandidateCount, bool bBackground)
	{
		int iTargetCultureStage, iTargetSpaceStage, iTargetConquestStage, iTargetDominationStage, iTargetDiplomacyStage;
		int iAgentCultureStage, iAgentSpaceStage, iAgentConquestStage, iAgentDominationStage, iAgentDiplomacyStage;
		getSASBBAITeamVictoryStages(eTarget, iTargetCultureStage, iTargetSpaceStage, iTargetConquestStage, iTargetDominationStage, iTargetDiplomacyStage);
		getSASBBAITeamVictoryStages(kAgent.getID(), iAgentCultureStage, iAgentSpaceStage, iAgentConquestStage, iAgentDominationStage, iAgentDiplomacyStage);
		if (std::max(std::max(iTargetCultureStage, iTargetSpaceStage), std::max(std::max(iTargetConquestStage, iTargetDominationStage), iTargetDiplomacyStage)) < 3 && GET_TEAM(eTarget).AI_getLowestVictoryCountdown() < 0)
			return;
		// <!-- custom: Map 450 showed Lincoln winning Space Race despite stronger teams evaluating him as weak and reachable only a few turns earlier.
		// Log near-victory target context directly beside UWAI target rows so future runs show whether the AI noticed a rival victory threat, whether the target was weak/near enough, and whether normal war selection acted too late. (GPT-5.5) -->
		logBBAI("WAR_TARGET_VICTORY_PRESSURE turn=%d row=%s background=%d agentTeam=%d targetTeam=%d warPlan=%s utility=%d drivePercent=%d targetRank=%d candidateCount=%d targetCultureStage=%d targetSpaceStage=%d targetConquestStage=%d targetDominationStage=%d targetDiplomacyStage=%d targetVictoryCountdown=%d targetSpaceshipParts=%d targetSpaceshipPartsPercent=%d targetSpaceLeaderPartGap=%d agentCultureStage=%d agentSpaceStage=%d agentConquestStage=%d agentDominationStage=%d agentDiplomacyStage=%d agentVictoryCountdown=%d attitude=%d attitudeValue=%d closeness=%d nearestCityDistance=%d targetPowerPercent=%d ourCities=%d targetCities=%d ourWars=%d targetWars=%d",
			GC.getGame().getGameTurn(), szRow, bBackground, kAgent.getID(), eTarget, getSASWarPlanType(eWarPlan), iUtility, iDrivePercent,
			iTargetRank, iCandidateCount, iTargetCultureStage, iTargetSpaceStage, iTargetConquestStage, iTargetDominationStage,
			iTargetDiplomacyStage, GET_TEAM(eTarget).AI_getLowestVictoryCountdown(), getSASTeamSpaceshipPartsBuilt(eTarget),
			getSASTeamSpaceshipPartsPercent(eTarget), getSASTeamStage3SpaceLeaderPartGap(eTarget), iAgentCultureStage, iAgentSpaceStage,
			iAgentConquestStage, iAgentDominationStage, iAgentDiplomacyStage, kAgent.AI_getLowestVictoryCountdown(),
			kAgent.AI_getAttitude(eTarget), kAgent.AI_getAttitudeVal(eTarget), kAgent.AI_teamCloseness(eTarget),
			getSASBBAINearestCityDistance(kAgent.getID(), eTarget), getSASBBAITargetPowerPercent(kAgent, eTarget), kAgent.getNumCities(),
			GET_TEAM(eTarget).getNumCities(), kAgent.getNumWars(true, true), GET_TEAM(eTarget).getNumWars(true, true));
	}

	bool isSASBBAIPreferredLocalWarTarget(CvTeamAI const& kAgent, TeamTypes eTarget, scaled rDrive)
	{
		static const bool bEnable = GC.getDefineBOOL("SAS_UWAI_LOCAL_WAR_TARGET_PREFERENCE_ENABLE");
		if (!bEnable)
			return false;
		// <!-- custom: UWAI sorts by drive, then tests targets probabilistically. BBAI war-target logs showed this can fall through to a farther target even when a closer weak/disliked land target is available, splitting armies away from the core and inviting opportunistic invasions.
		// Mark only mutually land-relevant local targets here; island/naval choices remain ordinary UWAI or future naval-specific logic. The later skip is intentionally narrow so ordinary UWAI target choice is not flattened into always attacking the weakest neighbor. Uses team power because UWAI chooses team war plans. See KI#182. (GPT-5.5) -->
		if (!kAgent.AI_isLandTarget(eTarget) || !GET_TEAM(eTarget).AI_isLandTarget(kAgent.getID()))
			return false;
		static const int iMinDrivePercent = GC.getDefineINT("SAS_UWAI_LOCAL_WAR_TARGET_MIN_DRIVE_PERCENT");
		static const int iMaxDistance = GC.getDefineINT("SAS_UWAI_LOCAL_WAR_TARGET_MAX_DISTANCE");
		static const int iMaxTargetPowerPercent = GC.getDefineINT("SAS_UWAI_LOCAL_WAR_TARGET_MAX_TARGET_POWER_PERCENT");
		static const int iMaxAttitudeValue = GC.getDefineINT("SAS_UWAI_LOCAL_WAR_TARGET_MAX_ATTITUDE_VALUE");
		static const int iMinCloseness = GC.getDefineINT("SAS_UWAI_LOCAL_WAR_TARGET_MIN_CLOSENESS");
		return (rDrive.getPercent() >= iMinDrivePercent && getSASBBAINearestCityDistance(kAgent.getID(), eTarget) <= iMaxDistance && getSASBBAITargetPowerPercent(kAgent, eTarget) <= iMaxTargetPowerPercent && kAgent.AI_getAttitudeVal(eTarget) <= iMaxAttitudeValue && kAgent.AI_teamCloseness(eTarget) >= iMinCloseness);
	}

	bool shouldSASBBAISkipForPreferredLocalWarTarget(CvTeamAI const& kAgent, TeamTypes eTarget, TeamTypes ePreferredTarget)
	{
		if (ePreferredTarget == NO_TEAM || eTarget == ePreferredTarget)
			return false;
		static const int iMinDistanceAdvantage = GC.getDefineINT("SAS_UWAI_LOCAL_WAR_TARGET_MIN_DISTANCE_ADVANTAGE");
		return (getSASBBAINearestCityDistance(kAgent.getID(), eTarget) >= getSASBBAINearestCityDistance(kAgent.getID(), ePreferredTarget) + iMinDistanceAdvantage);
	}

	bool isSASBBAIBetterPreferredLocalWarTarget(CvTeamAI const& kAgent, TeamTypes eCandidate, scaled rCandidateDrive, TeamTypes eCurrent, scaled rCurrentDrive)
	{
		if (eCurrent == NO_TEAM)
			return true;
		int const iCandidateDistance = getSASBBAINearestCityDistance(kAgent.getID(), eCandidate);
		int const iCurrentDistance = getSASBBAINearestCityDistance(kAgent.getID(), eCurrent);
		if (iCandidateDistance != iCurrentDistance)
			return (iCandidateDistance < iCurrentDistance);
		int const iCandidatePowerPercent = getSASBBAITargetPowerPercent(kAgent, eCandidate);
		int const iCurrentPowerPercent = getSASBBAITargetPowerPercent(kAgent, eCurrent);
		if (iCandidatePowerPercent != iCurrentPowerPercent)
			return (iCandidatePowerPercent < iCurrentPowerPercent);
		int const iCandidateAttitudeValue = kAgent.AI_getAttitudeVal(eCandidate);
		int const iCurrentAttitudeValue = kAgent.AI_getAttitudeVal(eCurrent);
		if (iCandidateAttitudeValue != iCurrentAttitudeValue)
			return (iCandidateAttitudeValue < iCurrentAttitudeValue);
		int const iCandidateCloseness = kAgent.AI_teamCloseness(eCandidate);
		int const iCurrentCloseness = kAgent.AI_teamCloseness(eCurrent);
		if (iCandidateCloseness != iCurrentCloseness)
			return (iCandidateCloseness > iCurrentCloseness);
		return (rCandidateDrive.getPercent() > rCurrentDrive.getPercent());
	}

	// <!-- custom: A rival can be an attractive target because its power is spread across too many cities or because its army is committed to another war. Count that context only inside WAR level-2 diagnostics so testing can measure these opportunities before changing UWAI target valuation. Reuse SASGameRecord's military-unit definition for consistent comparison. (GPT-5.6-Sol) -->
	void getSASBBAITeamMilitaryPosture(TeamTypes eTarget, int& iMilitary, int& iOwnTerritory, int& iOutsideOwnTerritory, int& iEnemyTerritory, int& iInCities)
	{
		iMilitary = 0;
		iOwnTerritory = 0;
		iOutsideOwnTerritory = 0;
		iEnemyTerritory = 0;
		iInCities = 0;
		for (MemberAIIter itMember(eTarget); itMember.hasNext(); ++itMember)
		{
			int iLoop = 0;
			for (CvUnit const* pLoopUnit = itMember->firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = itMember->nextUnit(&iLoop))
			{
				CvPlot const* pPlot = pLoopUnit->plot();
				// <!-- custom: A failed inherited Partisans initUnit(NO_UNIT) call left a reset/unplaced object in the owner container.
				// It is not part of deployable military posture, and reading its unit-info-backed combat state crashed WAR level-2 diagnostics; reject it first. See KI#524.3 and KI#524.6. (GPT-5.6-Sol) -->
				if (pPlot == NULL)
					continue;
				if (!pLoopUnit->canDefend(pPlot) && pLoopUnit->baseCombatStr() <= 0 && pLoopUnit->airBaseCombatStr() <= 0)
					continue;
				iMilitary++;
				if (pPlot->isCity())
					iInCities++;
				if (pPlot->getTeam() == eTarget)
					iOwnTerritory++;
				else
				{
					iOutsideOwnTerritory++;
					if (pPlot->getTeam() != NO_TEAM && GET_TEAM(eTarget).isAtWar(pPlot->getTeam()))
						iEnemyTerritory++;
				}
			}
		}
	}

	void logSASBBAIWarTargetEval(CvTeamAI const& kAgent, TeamTypes eTarget, WarPlanTypes eWarPlan, int iUtility, int iLimitedU, int iTotalU, bool bLimitedNaval, bool bTotalNaval, int iLimitedPrepTime, int iTotalPrepTime, bool bShortWork, bool bBackground)
	{
		CvTeamAI const& kTarget = GET_TEAM(eTarget);
		const PlayerTypes eAgentLeader = kAgent.getLeaderID();
		const PlayerTypes eTargetLeader = kTarget.getLeaderID();
		const int iAttitude = kAgent.AI_getAttitude(eTarget);
		const int iAttitudeValue = kAgent.AI_getAttitudeVal(eTarget);
		const int iOurPower = std::max(1, kAgent.getPower(true));
		const int iTargetTotalPower = kTarget.getPower(true);
		const int iTargetPower = kTarget.getDefensivePower(kAgent.getID());
		const int iNearestCityDistance = getSASBBAINearestCityDistance(kAgent.getID(), eTarget);
		int iOurMilitary, iOurMilitaryOwnTerritory, iOurMilitaryOutsideOwnTerritory, iOurMilitaryEnemyTerritory, iOurMilitaryInCities;
		int iTargetMilitary, iTargetMilitaryOwnTerritory, iTargetMilitaryOutsideOwnTerritory, iTargetMilitaryEnemyTerritory, iTargetMilitaryInCities;
		getSASBBAITeamMilitaryPosture(kAgent.getID(), iOurMilitary, iOurMilitaryOwnTerritory, iOurMilitaryOutsideOwnTerritory, iOurMilitaryEnemyTerritory, iOurMilitaryInCities);
		getSASBBAITeamMilitaryPosture(eTarget, iTargetMilitary, iTargetMilitaryOwnTerritory, iTargetMilitaryOutsideOwnTerritory, iTargetMilitaryEnemyTerritory, iTargetMilitaryInCities);
		logBBAI("WAR_TARGET_EVAL turn=%d background=%d agentTeam=%d agentLeader=%d targetTeam=%d targetLeader=%d warPlan=%s utility=%d limitedUtility=%d totalUtility=%d limitedNaval=%d totalNaval=%d limitedPrepTurns=%d totalPrepTurns=%d shortWork=%d avoidWar=%d forcedPeaceTurns=%d attitude=%d attitudeValue=%d closeness=%d landTarget=%d targetLandTarget=%d nearestCityDistance=%d ourPower=%d targetTotalPower=%d targetDefensivePower=%d targetPowerPercent=%d ourCities=%d targetCities=%d ourPowerPerCityX100=%d targetPowerPerCityX100=%d ourWars=%d targetWars=%d targetEnemyPowerPercent=%d ourMilitary=%d ourMilitaryOwnTerritory=%d ourMilitaryOutsideOwnTerritory=%d ourMilitaryEnemyTerritory=%d ourMilitaryInCities=%d targetMilitary=%d targetMilitaryOwnTerritory=%d targetMilitaryOutsideOwnTerritory=%d targetMilitaryEnemyTerritory=%d targetMilitaryInCities=%d",
			GC.getGame().getGameTurn(), bBackground, kAgent.getID(), eAgentLeader, eTarget, eTargetLeader, getSASWarPlanType(eWarPlan),
			iUtility, iLimitedU, iTotalU, bLimitedNaval, bTotalNaval, iLimitedPrepTime, iTotalPrepTime, bShortWork,
			kAgent.AI_isAvoidWar(eTarget, true), kAgent.turnsOfForcedPeaceRemaining(eTarget), iAttitude, iAttitudeValue,
			kAgent.AI_teamCloseness(eTarget), kAgent.AI_isLandTarget(eTarget), kTarget.AI_isLandTarget(kAgent.getID()), iNearestCityDistance,
			iOurPower, iTargetTotalPower, iTargetPower, (100 * iTargetPower) / iOurPower, kAgent.getNumCities(), kTarget.getNumCities(),
			(100 * iOurPower) / std::max(1, kAgent.getNumCities()), (100 * iTargetTotalPower) / std::max(1, kTarget.getNumCities()),
			kAgent.getNumWars(true, true), kTarget.getNumWars(true, true), kTarget.AI_getEnemyPowerPercent(true), iOurMilitary,
			iOurMilitaryOwnTerritory, iOurMilitaryOutsideOwnTerritory, iOurMilitaryEnemyTerritory, iOurMilitaryInCities, iTargetMilitary,
			iTargetMilitaryOwnTerritory, iTargetMilitaryOutsideOwnTerritory, iTargetMilitaryEnemyTerritory, iTargetMilitaryInCities);
		logSASBBAIWarTargetVictoryContext(kAgent, eTarget, "EVAL", eWarPlan, iUtility, -1, -1, -1, bBackground);
	}

	void logSASBBAIWarTargetDrive(CvTeamAI const& kAgent, TeamTypes eTarget, WarPlanTypes eWarPlan, int iUtility, scaled rDrive, bool bShortWork, bool bBackground)
	{
		CvTeamAI const& kTarget = GET_TEAM(eTarget);
		logBBAI("WAR_TARGET_DRIVE turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s utility=%d drivePercent=%d shortWork=%d avoidWar=%d forcedPeaceTurns=%d attitude=%d attitudeValue=%d closeness=%d nearestCityDistance=%d targetPowerPercent=%d ourCities=%d targetCities=%d ourWars=%d targetWars=%d",
			GC.getGame().getGameTurn(), bBackground, kAgent.getID(), eTarget, getSASWarPlanType(eWarPlan), iUtility, rDrive.getPercent(),
			bShortWork, kAgent.AI_isAvoidWar(eTarget, true), kAgent.turnsOfForcedPeaceRemaining(eTarget), kAgent.AI_getAttitude(eTarget),
			kAgent.AI_getAttitudeVal(eTarget), kAgent.AI_teamCloseness(eTarget), getSASBBAINearestCityDistance(kAgent.getID(), eTarget),
			getSASBBAITargetPowerPercent(kAgent, eTarget), kAgent.getNumCities(), kTarget.getNumCities(), kAgent.getNumWars(true, true),
			kTarget.getNumWars(true, true));
		logSASBBAIWarTargetVictoryContext(kAgent, eTarget, "DRIVE", eWarPlan, iUtility, rDrive.getPercent(), -1, -1, bBackground);
	}
}


UWAI::Team::Team()
:	m_eAgent(NO_TEAM), m_bInBackground(false)
{}


UWAI::Team::~Team()
{}


void UWAI::Team::reset()
{
	m_bInBackground = getUWAI().isEnabled(true);
}


void UWAI::Team::init(TeamTypes eTeam)
{
	m_eAgent = eTeam;
	reset();
}


void UWAI::Team::write(FDataStreamBase* pStream) const
{
	pStream->Write(m_eAgent);
}


void UWAI::Team::read(FDataStreamBase* pStream)
{
	pStream->Read((int*)&m_eAgent);
	reset();
}


void UWAI::Team::addTeam(PlayerTypes eOtherLeader)
{
	for (MemberAIIter it(m_eAgent); it.hasNext(); ++it)
		it->uwai().getCache().addTeam(eOtherLeader);
}


// <!-- custom: Paid-war sponsorship is player-cache state, so clear it for every living member when a lifecycle path cannot perform full war-history evaluation. See KI#562. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void UWAI::Team::clearWarSponsorship(TeamTypes eEnemy)
{
	for (MemberAIIter it(m_eAgent); it.hasNext(); ++it)
		it->uwai().getCache().clearWarSponsorship(eEnemy);
}


void UWAI::Team::reportWarEnding(TeamTypes eEnemy, CLinkList<TradeData> const* pWeReceive, CLinkList<TradeData> const* pWeGive)
{
	// <!-- custom: Human-capitulation readiness authorizes surrender only for the war in which considerPeace set it.
	// Expire that authorization at peace so a later war cannot reuse the old AI-turn decision. See KI#519. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (GET_TEAM(eEnemy).isHuman())
		leaderCache().setReadyToCapitulate(eEnemy, false);
	/*  This isn't team-level data b/c each member can have its
		own interpretation of whether the war was successful. */
	for (MemberAIIter it(m_eAgent); it.hasNext(); ++it)
		it->uwai().getCache().reportWarEnding(eEnemy, pWeReceive, pWeGive);
}


void UWAI::Team::turnPre()
{
	/*  Causes Player::turnPre to be called before CvPlayerAI::AI_turnPre.
		That's OK b/c AI_turnPre doesn't do anything crucial for UWAI::Player.
		Need to call Player::turnPre already during the team turn b/c
		the update to UWAICache is important for war planning. */
	for (MemberAIIter it(m_eAgent); it.hasNext(); ++it)
		it->uwai().turnPre();
}


void UWAI::Team::doWar()
{
	if (!getUWAI().isReady())
		return;
	CvTeamAI& kAgent = GET_TEAM(m_eAgent);
	if (!kAgent.isAlive() || !kAgent.isMajorCiv())
		return;
	FAssertMsg(!kAgent.isAVassal() || kAgent.getNumWars() > 0 ||
			kAgent.AI_getNumWarPlans(WARPLAN_DOGPILE) +
			kAgent.AI_getNumWarPlans(WARPLAN_LIMITED) +
			kAgent.AI_getNumWarPlans(WARPLAN_TOTAL) <= 0,
			"Vassals shouldn't have non-preparatory war plans unless at war");
	// <!-- custom: The Team now owns only this tiny nested mute-depth state. Assert that every prior evaluation balanced its push/pop pairs before starting another UWAI turn. See KI#505.3. (ChatGPT-5.6-Sol) -->
	FAssert(!m_kLogMuteState.isMuted());
	if (kAgent.isHuman() || kAgent.isAVassal())
	{
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("%S is %s", GET_TEAM(kAgent.getID()).getName().GetCString(),
				(kAgent.isHuman() ? "human" : "a vassal"));
		for (TeamIter<MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> it(kAgent.getID());
			it.hasNext(); ++it)
		{
			TeamTypes const eTarget = it->getID();
			WarPlanTypes const eWP = kAgent.AI_getWarPlan(eTarget);
			if (eWP == WARPLAN_ATTACKED_RECENT)
				considerPlanTypeChange(eTarget, 0);
			/*  Non-human vassals abandon human-instructed war prep. after 20 turns.
				Humans can have war preparations from AI Auto Play that they should
				also abandon (but not immediately b/c AI Auto Play could resume.) */
			// <!-- custom: The inherited condition negated the human-master test, so it timed out AI-master preparations while leaving the human-instructed preparations named above indefinitely.
			// Match the documented/manual-instruction case. See KI#516. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			if ((eWP == WARPLAN_PREPARING_LIMITED || eWP == WARPLAN_PREPARING_TOTAL) &&
				(kAgent.isHuman() || GET_TEAM(kAgent.getMasterTeam()).isHuman()) &&
				kAgent.AI_getWarPlanStateCounter(eTarget) > 20)
			{
				considerAbandonPreparations(eTarget, -1, 0);
			}
			if (kAgent.isAVassal())
			{
				/*  Make sure we match our master's war plan.
					CvTeamAI::AI_setWarPlan mostly handles this, but
					doesn't align vassal's plans after signing the vassal agreement.
					(Could do that in CvTeam::setVassal I guess.) */
				CvTeamAI const& kMaster = GET_TEAM(kAgent.getMasterTeam());
				if (kMaster.getID() == eTarget)
					continue;
				if (kMaster.isAtWar(eTarget))
					kAgent.AI_setWarPlan(eTarget, WARPLAN_DOGPILE);
				else if (kMaster.AI_getWarPlan(eTarget) != NO_WARPLAN &&
					!kMaster.isHuman()) // Human master will have to instruct us
				{
					kAgent.AI_setWarPlan(eTarget, WARPLAN_PREPARING_LIMITED);
				}
			}
		}
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Nothing more to do for this team");
		FAssert(!m_kLogMuteState.isMuted());
		return;
	}
	UWAICache& kCache = leaderCache();
	set<TeamTypes> aeChangedWarPlanTargets;
	if (reviewWarPlans(aeChangedWarPlanTargets))
	{
		scheme(aeChangedWarPlanTargets);
		for (TeamIter<CIV_ALIVE,KNOWN_POTENTIAL_ENEMY_OF> itTarget(kAgent.getID());
			itTarget.hasNext(); ++itTarget)
		{
			if (kAgent.AI_isSneakAttackPreparing(itTarget->getID()))
				kCache.setCanBeHiredAgainst(itTarget->getID(), true);
		}
	}
	else
	{
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("No scheming b/c clearly busy with current wars");
		for (TeamIter<CIV_ALIVE> it; it.hasNext(); ++it)
			kCache.setCanBeHiredAgainst(it->getID(), false);
	}
	FAssert(!m_kLogMuteState.isMuted());
}


UWAI::Player const& UWAI::Team::leaderUWAI() const
{
	return GET_PLAYER(GET_TEAM(m_eAgent).getLeaderID()).uwai();
}

UWAI::Player& UWAI::Team::leaderUWAI()
{
	return GET_PLAYER(GET_TEAM(m_eAgent).getLeaderID()).uwai();
}


scaled UWAI::Team::utilityToTradeVal(scaled rUtility) const
{
	scaled r;
	MemberAIIter itMember(m_eAgent);
	for(; itMember.hasNext(); ++itMember)
		r += itMember->uwai().utilityToTradeVal(rUtility);
	return r / itMember.nextIndex();
}


scaled UWAI::Team::tradeValToUtility(scaled rTradeVal) const
{
	scaled r;
	MemberAIIter itMember(m_eAgent);
	for(; itMember.hasNext(); ++itMember)
		r += itMember->uwai().tradeValToUtility(rTradeVal);
	return r / itMember.nextIndex();
}


namespace
{
	struct PlanData
	{
		PlanData(int iU, TeamTypes eTarget, int iPrepTurns, bool bNaval)
		:	iU(iU), eTarget(eTarget), iPrepTurns(iPrepTurns), bNaval(bNaval)
		{}
		bool operator<(PlanData const& kOther) { return iU < kOther.iU; }
		int iU;
		TeamTypes eTarget;
		int iPrepTurns;
		bool bNaval;
	};
}

bool UWAI::Team::reviewWarPlans(set<TeamTypes>& aeChangedTargets)
{
	CvTeamAI& kAgent = GET_TEAM(m_eAgent);
	if (!kAgent.AI_isAnyWarPlan())
	{
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("%S has no war plans to review",
				GET_TEAM(kAgent.getID()).getName().GetCString());
		return true;
	}
	bool bScheme = true;
	if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("%S reviews its war plans",
			GET_TEAM(kAgent.getID()).getName().GetCString());
	EagerEnumMap<TeamTypes,bool> abTargetDone;
	bool bPlanChanged = false;
	bool bAllNaval = true, bAnyNaval = false;
	bool bAllLand = true, bAnyLand = false;
	do
	{
		vector<PlanData> aPlans;
		for (TeamAIIter<MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itTarget(kAgent.getID());
			itTarget.hasNext(); ++itTarget)
		{
			CvTeamAI const& kTarget = *itTarget;
			TeamTypes const eTarget = kTarget.getID();
			if (abTargetDone.get(eTarget))
				continue;
			if (kTarget.isHuman()) // considerCapitulation may set ReadyToCapitulate
				leaderCache().setReadyToCapitulate(eTarget, false);
			WarPlanTypes const eWP = kAgent.AI_getWarPlan(eTarget);
			if (eWP == NO_WARPLAN)
			{
				// As good a place as any to make sure of this
				FAssert(kAgent.AI_getWarPlanStateCounter(eTarget) == 0);
				continue;
			}
			if (kTarget.isAVassal())
				continue;
			WarEvalParameters params(kAgent.getID(), eTarget, m_kLogMuteState);
			WarEvaluator eval(params);
			if (gWarLogLevel >= 2 && kAgent.isAtWar(eTarget)) eval.enableSASBBAISuspiciousPeaceLog();
			int iU = eval.evaluate(eWP);
			// <!-- custom: doScheme adds victory-denial urgency when selecting a target. Preserve that value during later reviews too; otherwise UWAI can select a rival for being close to victory and cancel the same preparation after evaluating it without the urgency boost. Save-file 452 reproduced this repeatedly against India. See KI#189. (GPT-5.6-Sol) -->
			if (!kAgent.isAtWar(eTarget))
				iU += getSASBBAIVictoryDenialUtilityBoost(eTarget, getSASTeamMaxVictoryStage(eTarget));
			// 'evaluate' sets preparation time and isNaval in params
			aPlans.push_back(PlanData(iU, eTarget, params.getPreparationTime(),
					params.isNaval()));
			/*  Skip scheming when in a very bad war. Very unlikely that another war
				could help then. And I worry that, in rare situations, when the
				outcome of a war is close, but potentially disastrous, that an
				additional war could produce a more favorable simulation outcome. */
			if (iU < -100 && kAgent.isAtWar(eTarget))
				bScheme = false;
		}
		std::sort(aPlans.begin(), aPlans.end());
		TeamTypes ePreferredEmergencyPeaceTarget = NO_TEAM;
		int iPreferredEmergencyPeaceUtility = 0;
		int iPreferredEmergencyPeaceReluctance = MIN_INT;
		int const iMajorWars = kAgent.getNumWars(true, true);
		int const iEnemyPowerPercent = (iMajorWars >= 2 ? kAgent.AI_getEnemyPowerPercent(true) : 0);
		static int const iEmergencyPeacePowerThreshold = GC.getDefineINT("SAS_UWAI_EMERGENCY_PEACE_ENEMY_POWER_THRESHOLD");
		static int const iExtraWarThreatPercent = GC.getDefineINT("SAS_UWAI_EMERGENCY_PEACE_EXTRA_WAR_THREAT_PERCENT");
		int const iAdjustedEnemyPowerPercent = iEnemyPowerPercent * (100 + std::max(0, iMajorWars - 2) * iExtraWarThreatPercent) / 100;
		bool const bEmergencyPeaceMode = (iMajorWars >= 2 && iAdjustedEnemyPowerPercent > iEmergencyPeacePowerThreshold);
		if (bEmergencyPeaceMode)
		{
			bool const bAgentVictoryThreat = isSASUWAIVictoryDenialPeaceThreat(kAgent.getID());
			TeamTypes eLowestUtilityTarget = NO_TEAM;
			int iLowestUtility = 0;
			for (size_t i = 0; i < aPlans.size(); i++)
			{
				TeamTypes const eTarget = aPlans[i].eTarget;
				if (!kAgent.isAtWar(eTarget) || !kAgent.canChangeWarPeace(eTarget))
					continue;
				// <!-- custom: Capitulation remains possible, but ordinary peace is intentionally unavailable while either side is a configured victory threat. Skip those wars when choosing where the emergency peace override can actually help. (GPT-5.6-Sol) -->
				if (bAgentVictoryThreat || isSASUWAIVictoryDenialPeaceThreat(eTarget))
					continue;
				if (eLowestUtilityTarget == NO_TEAM)
				{
					eLowestUtilityTarget = eTarget;
					iLowestUtility = aPlans[i].iU;
					// <!-- custom: A newly declared dangerous war cannot negotiate immediately. Preserve the other wars briefly so the AI first tries the preferred opponent instead of abandoning an easy conquest on the declaration turn. (GPT-5.6-Sol) -->
					if (kAgent.AI_getAtWarCounter(eTarget) <= 2)
						break;
				}
				int const iReluctance = GET_TEAM(eTarget).uwai().reluctanceToPeace(kAgent.getID(), false);
				if (iReluctance > iMaxReparationUtility)
					continue;
				ePreferredEmergencyPeaceTarget = eTarget;
				iPreferredEmergencyPeaceUtility = aPlans[i].iU;
				iPreferredEmergencyPeaceReluctance = iReluctance;
				break;
			}
			if (ePreferredEmergencyPeaceTarget == NO_TEAM && eLowestUtilityTarget != NO_TEAM && kAgent.AI_getAtWarCounter(eLowestUtilityTarget) <= 2)
			{
				ePreferredEmergencyPeaceTarget = eLowestUtilityTarget;
				iPreferredEmergencyPeaceUtility = iLowestUtility;
				iPreferredEmergencyPeaceReluctance = MIN_INT;
			}
			if (gWarLogLevel >= 1) logBBAI("WAR_EMERGENCY_PEACE_TARGET turn=%d background=%d agentTeam=%d preferredTarget=%d majorWars=%d enemyPowerPercent=%d adjustedEnemyPowerPercent=%d emergencyPowerThreshold=%d preferredUtility=%d preferredReluctance=%d",
				GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), ePreferredEmergencyPeaceTarget, iMajorWars, iEnemyPowerPercent,
				iAdjustedEnemyPowerPercent, iEmergencyPeacePowerThreshold, iPreferredEmergencyPeaceUtility,
				iPreferredEmergencyPeaceReluctance);
		}
		bPlanChanged = false;

		for (size_t i = 0; i < aPlans.size(); i++)
		{
			bPlanChanged = !reviewPlan(aPlans[i].eTarget, aPlans[i].iU, aPlans[i].iPrepTurns, aPlans[i].bNaval,
					iMajorWars, iEnemyPowerPercent, iAdjustedEnemyPowerPercent, ePreferredEmergencyPeaceTarget, iPreferredEmergencyPeaceReluctance);
			if (bPlanChanged)
			{
				aeChangedTargets.insert(aPlans[i].eTarget);
				abTargetDone.set(aPlans[i].eTarget, true);
				if (abTargetDone.numNonDefault() < (int)aPlans.size())
				{
					if (gUWAIAgentLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("War plan against %S has changed, repeating review",
							GET_TEAM(aPlans[i].eTarget).getName().GetCString());
				}
				break;
			}
			// Ignore isNaval if we're not sure about the war (low utility)
			else if (aPlans[i].iU > 5)
			{
				if (aPlans[i].bNaval)
				{
					bAnyNaval = true;
					bAllLand = false;
				}
				else
				{
					bAnyLand = true;
					bAllNaval = false;
				}
			}
		}
	} while(bPlanChanged);
	/*  As CvTeamAI::AI_updateAreaStrategies is called before CvTeamAI::AI_doWar,
		this is going to be the last word on AreaAI. */
	if (bAllNaval && bAnyNaval)
		alignAreaAI(true);
	if (bAllLand && bAnyLand)
		alignAreaAI(false);
	return bScheme;
}


void UWAI::Team::alignAreaAI(bool bNaval)
{
	PROFILE_FUNC();
	set<int> areasToAlign;
	set<int> areasNotToAlign;
	for (MemberAIIter itMember(m_eAgent); itMember.hasNext(); ++itMember)
	{
		CvPlayerAI const& kMember = *itMember;
		CvCity const* pCapital = kMember.getCapital();
		if (pCapital == NULL)
			continue;
		CvArea& kArea = pCapital->getArea();
		CvCity const* pTargetCity = kArea.AI_getTargetCity(kMember.getID());
		bool bAlign = true;
		if (bNaval)
		{
			if (pTargetCity!= NULL &&
				(GET_TEAM(pTargetCity->getTeam()).AI_isPrimaryArea(kArea) ||
				3 * kArea.getCitiesPerPlayer(pTargetCity->getOwner()) >
				kArea.getCitiesPerPlayer(kMember.getID())))
			{
				WarPlanTypes eWP = GET_TEAM(m_eAgent).AI_getWarPlan(
						pTargetCity->getTeam());
				if (!GET_TEAM(m_eAgent).AI_isPushover(pTargetCity->getTeam()) ||
					(eWP != WARPLAN_TOTAL && eWP != WARPLAN_PREPARING_TOTAL))
				{
					// Make sure there isn't an easily reachable target in the capital area
					TeamPathFinder<TeamPath::LAND> pf(GET_TEAM(m_eAgent),
							&GET_TEAM(pTargetCity->getTeam()), 8);
					if (pf.generatePath(pCapital->getPlot(), pTargetCity->getPlot()))
						bAlign = false;
				}
			}
		}
		else
		{
			// Make sure some city can be attacked in the capital area
			if (pTargetCity == NULL)
			{
				// Target city is sometimes (randomly) set to NULL
				pTargetCity = kMember.AI_findTargetCity(kArea);
			}
			if (pTargetCity == NULL)
				bAlign = false;
			else
			{
				UWAICache::City* pCacheCity = kMember.uwai().getCache().
						lookupCity(pTargetCity->plotNum());
				if (pCacheCity == NULL || !pCacheCity->canReachByLandFromCapital())
					bAlign = false;
			}
		}
		if (bAlign)
			areasToAlign.insert(kArea.getID());
		else areasNotToAlign.insert(kArea.getID());
	}
	set<int> diff;
	std::set_difference(
			areasToAlign.begin(), areasToAlign.end(),
			areasNotToAlign.begin(), areasNotToAlign.end(),
			std::inserter(diff, diff.begin()));
	CvMap& kMap = GC.getMap();
	for(set<int>::const_iterator it = diff.begin(); it != diff.end(); ++it)
	{
		CvArea& kArea = *kMap.getArea(*it);
		AreaAITypes const eOldAreaAI = kArea.getAreaAIType(m_eAgent);
		AreaAITypes eNewAreaAI = eOldAreaAI;
		if (bNaval)
		{
			if (eOldAreaAI == AREAAI_MASSING)
				eNewAreaAI = AREAAI_ASSAULT_MASSING;
			else if (eOldAreaAI == AREAAI_OFFENSIVE)
				eNewAreaAI = AREAAI_ASSAULT;
		}
		else
		{
			if (eOldAreaAI == AREAAI_ASSAULT_MASSING)
				eNewAreaAI = AREAAI_MASSING;
			else if (eOldAreaAI == AREAAI_ASSAULT)
				eNewAreaAI = AREAAI_OFFENSIVE;
		}
		if (eNewAreaAI != eOldAreaAI)
		{
			kArea.setAreaAIType(m_eAgent, eNewAreaAI);
			if (gGameRecordLogLevel >= 2) logSASGameRecordAreaAIChanged(GET_TEAM(m_eAgent), kArea, eOldAreaAI, eNewAreaAI, bNaval ? "UWAI_ALIGN_NAVAL" : "UWAI_ALIGN_LAND");
		}
	}
}


// <!-- custom: Added bNaval so a reviewed preparation retains the initial target evaluation's land/naval restriction when checking whether victory denial justifies immediate war. Added the emergency-peace measurements, preferred target and cached reluctance so multi-war danger is computed once and first seeks feasible peace in the least valuable war instead of forcing peace against every opponent. (GPT-5.6-Sol) -->
bool UWAI::Team::reviewPlan(TeamTypes eTarget, int iU, int iPrepTurns, bool bNaval, int iMajorWars, int iEnemyPowerPercent, int iAdjustedEnemyPowerPercent, TeamTypes ePreferredEmergencyPeaceTarget, int iPreferredEmergencyPeaceReluctance)
{
	CvTeamAI& kAgent = GET_TEAM(m_eAgent);
	WarPlanTypes const eWP = kAgent.AI_getWarPlan(eTarget);
	FAssert(eWP != NO_WARPLAN);
	bool bAtWar = kAgent.isAtWar(eTarget);
	int iWPAge = kAgent.AI_getWarPlanStateCounter(eTarget);
	FAssert(iWPAge >= 0);
	if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Reviewing war plan \"%s\" (age: %d turns) against %S (%su=%d)",
			getSASWarPlanType(eWP), iWPAge, GET_TEAM(eTarget).getName().GetCString(),
			(bAtWar ? "at war; " : ""), iU);
	if (bAtWar)
	{
		FAssert(eWP != WARPLAN_PREPARING_LIMITED && eWP != WARPLAN_PREPARING_TOTAL);
		if (!considerPeace(eTarget, iU, iMajorWars, iEnemyPowerPercent, iAdjustedEnemyPowerPercent, ePreferredEmergencyPeaceTarget, iPreferredEmergencyPeaceReluctance))
			return false;
		considerPlanTypeChange(eTarget, iU);
		/*  Changing between attacked_recent, limited and total is unlikely
			to affect our other war plans, so ignore the return value of
			considerPlanTypeChange and report "no changes" to the caller. */
		return true;
	}
	else
	{
		// <!-- custom: The initial target evaluation included victory-denial urgency, but inherited UWAI reviews omitted it and could immediately discard or redirect the preparation. Reapply the same boost during reviews, retain the unboosted value for diagnostics, and pass the boost into the direct-war comparison below. See KI#189. (GPT-5.6-Sol) -->
		int const iTargetMaxVictoryStage = getSASTeamMaxVictoryStage(eTarget);
		int const iVictoryDenialBoost = getSASBBAIVictoryDenialUtilityBoost(eTarget, iTargetMaxVictoryStage);
		int const iOriginalU = iU - iVictoryDenialBoost;
		if (gWarLogLevel >= 1 && iVictoryDenialBoost > 0) logBBAI("WAR_PREPARATION_VICTORY_DENIAL_ADJUST turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s originalUtility=%d adjustedUtility=%d boost=%d targetMaxVictoryStage=%d targetVictoryCountdown=%d distance=%d targetPowerPercent=%d",
			GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), iOriginalU, iU,
			iVictoryDenialBoost, iTargetMaxVictoryStage, GET_TEAM(eTarget).AI_getLowestVictoryCountdown(),
			getSASBBAINearestCityDistance(kAgent.getID(), eTarget), getSASBBAITargetPowerPercent(kAgent, eTarget));
		// <!-- custom: Save-file 452 diagnostics showed 152 war preparations but only 17 becoming wars, while 130 were canceled or switched.
		// Keep each real/simulated review and exact cancellation/conclusion cause visible while correcting inherited UWAI behavior. (GPT-5.6-Sol) -->
		if (gWarLogLevel >= 2 && (eWP == WARPLAN_PREPARING_LIMITED || eWP == WARPLAN_PREPARING_TOTAL)) logBBAI("WAR_PREPARATION_REVIEW turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s utility=%d stateCounter=%d prepTurnsRemaining=%d warPlanCount=%d wars=%d forcedPeaceTurns=%d distance=%d targetVictoryCountdown=%d targetPowerPercent=%d",
			GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), iU, iWPAge, iPrepTurns,
			kAgent.AI_countWarPlans(), kAgent.getNumWars(true, true), kAgent.turnsOfForcedPeaceRemaining(eTarget),
			getSASBBAINearestCityDistance(kAgent.getID(), eTarget), GET_TEAM(eTarget).AI_getLowestVictoryCountdown(),
			getSASBBAITargetPowerPercent(kAgent, eTarget));
		if (!canSchemeAgainst(eTarget, true))
		{
			if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("War plan \"%s\" canceled b/c %S is no longer a legal target",
					getSASWarPlanType(eWP), GET_TEAM(eTarget).getName().GetCString());
			if (gWarLogLevel >= 1) logBBAI("WAR_PREPARATION_CANCEL turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s reason=illegal_target utility=%d stateCounter=%d prepTurnsRemaining=%d",
					GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), iU, iWPAge, iPrepTurns);
			if (!isInBackground())
			{
				if (gGameRecordLogLevel >= 2) logSASGameRecordUWAIWarPlanDecision(kAgent.getID(), eTarget, SAS_UWAI_WAR_PLAN_ILLEGAL_TARGET, eWP, NO_WARPLAN, iU, iWPAge, iPrepTurns);
				kAgent.AI_setWarPlan(eTarget, NO_WARPLAN);
				showWarPlanAbandonedMsg(eTarget);
			}
			return false;
		}
		// <!-- custom: A victory threat can become close and weak enough for direct war after preparation began. Use the same narrow distance, power and naval gates as doScheme, then declare immediately instead of letting the old review logic cancel or delay the emergency plan. See KI#189. (GPT-5.6-Sol) -->
		if (iVictoryDenialBoost > 0 && isSASVictoryDenialDirectWarAllowed(eTarget, iTargetMaxVictoryStage, bNaval, getSASBBAINearestCityDistance(kAgent.getID(), eTarget)) && kAgent.canDeclareWar(eTarget))
		{
			WarPlanTypes const eDirectWP = (eWP == WARPLAN_PREPARING_TOTAL || eWP == WARPLAN_TOTAL ? WARPLAN_TOTAL : WARPLAN_LIMITED);
			if (gWarLogLevel >= 1) logBBAI("WAR_PREPARATION_VICTORY_DENIAL_DECLARE turn=%d background=%d agentTeam=%d targetTeam=%d oldWarPlan=%s newWarPlan=%s utility=%d originalUtility=%d boost=%d targetMaxVictoryStage=%d targetVictoryCountdown=%d distance=%d targetPowerPercent=%d",
				GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), getSASWarPlanType(eDirectWP),
				iU, iOriginalU, iVictoryDenialBoost, iTargetMaxVictoryStage, GET_TEAM(eTarget).AI_getLowestVictoryCountdown(),
				getSASBBAINearestCityDistance(kAgent.getID(), eTarget), getSASBBAITargetPowerPercent(kAgent, eTarget));
			if (!isInBackground())
			{
				if (gGameRecordLogLevel >= 2) logSASGameRecordUWAIWarPlanDecision(kAgent.getID(), eTarget, SAS_UWAI_WAR_PLAN_VICTORY_DENIAL_DIRECT, eWP, eDirectWP, iU, iWPAge, iPrepTurns, NO_TEAM, -1, -1, -1, iVictoryDenialBoost);
				kAgent.declareWar(eTarget, false, eDirectWP);
			}
			return false;
		}
		if (eWP != WARPLAN_PREPARING_LIMITED && eWP != WARPLAN_PREPARING_TOTAL)
		{
			FAssert(eWP == WARPLAN_LIMITED || eWP == WARPLAN_TOTAL ||
					// UWAI doesn't use dogpile war plans
					(isInBackground() && eWP == WARPLAN_DOGPILE));
			if (iU < 0)
			{
				if (gUWAIAgentLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("Imminent war canceled; no longer worthwhile");
				if (gWarLogLevel >= 1) logBBAI("WAR_PREPARATION_CANCEL turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s reason=imminent_negative_utility utility=%d stateCounter=%d prepTurnsRemaining=%d",
						GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), iU, iWPAge, iPrepTurns);
				if (!isInBackground())
				{
					if (gGameRecordLogLevel >= 2) logSASGameRecordUWAIWarPlanDecision(kAgent.getID(), eTarget, SAS_UWAI_WAR_PLAN_IMMINENT_NEGATIVE_UTILITY, eWP, NO_WARPLAN, iU, iWPAge, iPrepTurns);
					kAgent.AI_setWarPlan(eTarget, NO_WARPLAN);
					showWarPlanAbandonedMsg(eTarget);
				}
				return false;
			}
			else
			{
				// <!-- custom: Once target switching itself requires a clear deterministic advantage, a separate random roll only makes the AI overlook the best target unpredictably. Always perform the comparison; the temporary plan change remains necessary because the inherited helper expects a preparation plan. See KI#189. (GPT-5.6-Sol) -->
				// <!-- custom: Preserve TOTAL vs LIMITED intent when temporarily converting an imminent plan for target comparison. Otherwise the helper evaluates every alternative as LIMITED and permanently assigns that type after a successful switch. See KI#511. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				WarPlanTypes const ePreparingWP = (eWP == WARPLAN_TOTAL ? WARPLAN_PREPARING_TOTAL : WARPLAN_PREPARING_LIMITED);
				kAgent.AI_setWarPlanNoUpdate(eTarget, ePreparingWP);
				bool const bSwitch = !considerSwitchTarget(eTarget, iU, 0);
				/*  If we do switch, then considerSwitchTarget has
					already reset the war plan against targetId --
					except when running in the background. */
				if (!bSwitch || isInBackground())
					kAgent.AI_setWarPlanNoUpdate(eTarget, eWP);
				if (bSwitch)
					return false;
			}
			CvMap const& kMap = GC.getMap();
			// 12 turns for a (AdvCiv) Standard-size map, 9 on Small.
			int iTimeout = std::max(kMap.getGridWidth(), kMap.getGridHeight()) / 7;
			// Checking missions is a bit costly, don't do it if timeout isn't near.
			if (iWPAge > iTimeout)
			{
				// Akin to code in CvTeamAI::AI_endWarVal
				for(MemberAIIter itMember(kAgent.getID()); itMember.hasNext(); ++itMember)
				{
					if (itMember->AI_isAnyEnemyTargetMission(eTarget))
					{
						iTimeout *= 2;
						break;
					}
				}
			}
			if (iWPAge > iTimeout)
			{
				if (gUWAIAgentLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("Imminent war canceled b/c of timeout (%d turns)",
						iTimeout);
				if (gWarLogLevel >= 1) logBBAI("WAR_PREPARATION_CANCEL turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s reason=imminent_timeout utility=%d stateCounter=%d timeout=%d",
						GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), iU, iWPAge, iTimeout);
				if (!isInBackground())
				{
					if (gGameRecordLogLevel >= 2) logSASGameRecordUWAIWarPlanDecision(kAgent.getID(), eTarget, SAS_UWAI_WAR_PLAN_IMMINENT_TIMEOUT, eWP, NO_WARPLAN, iU, iWPAge, iPrepTurns, NO_TEAM, -1, iWPAge, iTimeout);
					kAgent.AI_setWarPlan(eTarget, NO_WARPLAN);
					showWarPlanAbandonedMsg(eTarget);
				}
				return false;
			}
			if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("War remains imminent (%d turns until timeout)",
					1 + iTimeout - iWPAge);
		}
		else
		{
			if (!considerConcludePreparations(eTarget, iU, iPrepTurns, iVictoryDenialBoost))
				return false;
			if (!considerAbandonPreparations(eTarget, iU, iPrepTurns))
				return false;
			if (!considerSwitchTarget(eTarget, iU, iPrepTurns))
				return false;
		}
	}
	return true;
}


bool UWAI::Team::considerPeace(TeamTypes eTarget, int iU, int iMajorWars, int iEnemyPowerPercent, int iAdjustedEnemyPowerPercent, TeamTypes ePreferredEmergencyPeaceTarget, int iPreferredEmergencyPeaceReluctance)
{
	CvTeamAI& kAgent = GET_TEAM(m_eAgent);
	int const iInitialU = iU;
	if (!kAgent.canChangeWarPeace(eTarget))
	{
		if (gWarLogLevel >= 2) logBBAI("WAR_PEACE_DECISION turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s initialUtility=%d decisionUtility=%d sought=0 reason=cannot_change_war_peace",
				GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(kAgent.AI_getWarPlan(eTarget)), iInitialU, iU);
		return true;
	}
	CvTeamAI& kTarget = GET_TEAM(eTarget);
	scaled rPeaceThresh = peaceThreshold(eTarget);
	bool const bHuman = kTarget.isHuman();
	if (bHuman)
	{
		/*	They might pay us for peace (whereas, for AI-AI deals, the side
			that expects to be paid waits for the other side to sue for peace). */
		rPeaceThresh += 10;
	}

	// <!-- custom: we have an issue of hatshepsut ai being the military leader with a strong army at turn 150 and then being badly dogpiled on and dying/been defeated before turn 200. While we need to fix the reasons why wars happened that were detrimental for Hatshepsut ai as well, for now and first i mean here, add pre-checks to enforce/emergency exit multi-wars past a certain count and try to seek peace no matter what, as more war ennemies (not including barbarians hopefully) can only be detrimental to us, code provided by chatgpt 5, check if accurate; see also known issue as of now 65 for details; results of these "emergency peace" changes: great!!! Now hatshepsut ai seemingly does not die anymore, makes military gains and makes peace many times based on chatgpt 5 reading of the event log as i had not read it myself at first but then i saw same results looking at event log ingame with multiple peace treaties made by hatshepsut ai quite shortly after war, and hatshepsut ai is still strongest player at turn 200 (although by smaller margin but did very great i would say)!! See known issue as of now 65 for details -->
	// 
	// Emergency rule: if we're at war with 3+ major civs, force the negotiation path.
	// With those two placements, Hatshepsut (or anyone) at war with 3+ major civs will reliably try to negotiate peace now, instead of riding the dogpile into the ground.
	//
	// Count current wars vs major civs (ignore barbs & minors; also ignore vassal “duplicates")
	// const int iMajorWars = kAgent.getNumWars(/*bIgnoreMinors=*/true, /*bIgnoreVassals=*/true);

	// Combined enemy power vs us (100 = parity)
	// const int iEnemyPowPct = kAgent.AI_getEnemyPowerPercent(true);

	// <!-- custom: avoid as of now max 3 wars or even if 2 wars if our opponents are strong enough treat it the same. This allows to be versatile enough (3 wars are fine if a few targets are weak, so don't over-peace which would be bit boring too if i may say or waste potential) but also safe enough (even 2 wars are already dangerous if one or both of these rivals are strong enough to combined ravage us xd so treat it as an emergency) -->
	// const bool bEmergencyPeaceMode = ((iMajorWars >= 3) || (iMajorWars >= 2 && iEnemyPowPct > 160));
	// <!-- custom: KI#65's SAS rule treated any three wars as an emergency, so save-file 450 Carthage abandoned a four-turn war against two-city Holy Rome despite having 16 cities and all three enemies together contributing only 70% effective power. Multiply actual combined enemy power by a tunable pressure for each front beyond two instead: several meaningful enemies remain dangerous, but any number of nearly powerless enemies cannot force peace. The values were computed once in reviewWarPlans and passed through reviewPlan. (GPT-5.6-Sol) -->
	static int const iEmergencyPeacePowerThreshold = GC.getDefineINT("SAS_UWAI_EMERGENCY_PEACE_ENEMY_POWER_THRESHOLD");
	const bool bEmergencyPeaceMode = (iMajorWars >= 2 && iAdjustedEnemyPowerPercent > iEmergencyPeacePowerThreshold);
	// <!-- custom: The old KI#65 override applied independently to every war. Fresh Pangaea runs showed Arabia abandoning a profitable war against a 34%-power rival and Shaka making peace with a one-city 16%-power rival because a more dangerous new enemy had appeared.
	// reviewWarPlans already sorts wars from lowest to highest continuation utility. After briefly giving a new preferred war time to become negotiable, force emergency peace only for the lowest-utility opponent willing to accept and preserve the better wars; if peace succeeds, the repeated review recalculates whether another emergency remains. (GPT-5.6-Sol) -->
	const bool bEmergencyPeace = (bEmergencyPeaceMode && eTarget == ePreferredEmergencyPeaceTarget);
	// <!-- custom: Save files 450 and 452 showed 25 of 44 completed wars ending after only two turns, often after the attacker captured just one city or none. Log the inherited UWAI peace valuation and our KI#65 emergency override together so we can distinguish rational retreats from premature peace that lets a weak rival survive and recover.
	// Target thinness, simultaneous enemies and army location also show whether continuing can exploit a weak/distracted rival or would merely prolong a stalemate. Keep the unit scan inside the level-2 gate. (GPT-5.6-Sol) -->
	if (gWarLogLevel >= 2)
	{
		const int iOurPower = std::max(1, kAgent.getPower(true));
		const int iTargetTotalPower = kTarget.getPower(true);
		const int iTargetDefensivePower = kTarget.getDefensivePower(kAgent.getID());
		const int iOurCities = kAgent.getNumCities();
		const int iTargetCities = kTarget.getNumCities();
		const int iOurWarSuccess = kAgent.AI_getWarSuccess(eTarget).round();
		const int iTargetWarSuccess = kTarget.AI_getWarSuccess(kAgent.getID()).round();
		int iTargetMilitary, iTargetMilitaryOwnTerritory, iTargetMilitaryOutsideOwnTerritory, iTargetMilitaryEnemyTerritory, iTargetMilitaryInCities;
		getSASBBAITeamMilitaryPosture(eTarget, iTargetMilitary, iTargetMilitaryOwnTerritory, iTargetMilitaryOutsideOwnTerritory, iTargetMilitaryEnemyTerritory, iTargetMilitaryInCities);
		logBBAI("WAR_PEACE_REVIEW turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s initialUtility=%d peaceThreshold=%d atWarCounter=%d majorWars=%d enemyPowerPercent=%d adjustedEnemyPowerPercent=%d emergencyPowerThreshold=%d emergencyPeaceMode=%d preferredEmergencyPeaceTarget=%d emergencyPeace=%d ourPower=%d targetTotalPower=%d targetDefensivePower=%d targetPowerPercent=%d ourCities=%d targetCities=%d ourPowerPerCityX100=%d targetPowerPerCityX100=%d ourWarSuccess=%d targetWarSuccess=%d warSuccessDelta=%d targetWars=%d targetEnemyPowerPercent=%d targetMilitary=%d targetMilitaryOwnTerritory=%d targetMilitaryOutsideOwnTerritory=%d targetMilitaryEnemyTerritory=%d targetMilitaryInCities=%d nearestCityDistance=%d attitude=%d attitudeValue=%d",
			GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(kAgent.AI_getWarPlan(eTarget)),
			iInitialU, rPeaceThresh.round(), kAgent.AI_getAtWarCounter(eTarget), iMajorWars, iEnemyPowerPercent, iAdjustedEnemyPowerPercent,
			iEmergencyPeacePowerThreshold, bEmergencyPeaceMode, ePreferredEmergencyPeaceTarget, bEmergencyPeace, iOurPower,
			iTargetTotalPower, iTargetDefensivePower, (100 * iTargetDefensivePower) / iOurPower, iOurCities, iTargetCities,
			(100 * iOurPower) / std::max(1, iOurCities), (100 * iTargetTotalPower) / std::max(1, iTargetCities), iOurWarSuccess,
			iTargetWarSuccess, iOurWarSuccess - iTargetWarSuccess, kTarget.getNumWars(true, true), kTarget.AI_getEnemyPowerPercent(true),
			iTargetMilitary, iTargetMilitaryOwnTerritory, iTargetMilitaryOutsideOwnTerritory, iTargetMilitaryEnemyTerritory,
			iTargetMilitaryInCities, getSASBBAINearestCityDistance(kAgent.getID(), eTarget), kAgent.AI_getAttitude(eTarget),
			kAgent.AI_getAttitudeVal(eTarget));
	}
	if (bEmergencyPeaceMode && ePreferredEmergencyPeaceTarget != NO_TEAM && !bEmergencyPeace)
	{
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Preserving this war while emergency peace is sought against the lower-utility target");
		if (gWarLogLevel >= 1) logBBAI("WAR_PEACE_DECISION turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s initialUtility=%d decisionUtility=%d peaceThreshold=%d majorWars=%d enemyPowerPercent=%d adjustedEnemyPowerPercent=%d emergencyPeaceMode=1 preferredEmergencyPeaceTarget=%d sought=0 reason=preserve_better_war",
			GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(kAgent.AI_getWarPlan(eTarget)),
			iInitialU, iU, rPeaceThresh.round(), iMajorWars, iEnemyPowerPercent, iAdjustedEnemyPowerPercent, ePreferredEmergencyPeaceTarget);
		return true;
	}

	if (bEmergencyPeace)
	{
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Emergency peace mode: %d simultaneous wars vs majors — forcing negotiation.", iMajorWars);
		// Make sure the code doesn't early-out on "utility above threshold":
		// push iU clearly below the threshold so we go to the negotiation block.
		// <!-- custom: Emergency-peace thresholds are signed and normally nonpositive for inter-AI wars; uround asserts on negative input and rounds it with the wrong contract.
		// Use signed round before forcing utility below the threshold. See KI#514. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		iU = std::min(iU, rPeaceThresh.round() - 100);
	}
	// keep the existing log (or adjust) after this
	//
	if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Threshold for seeking peace: %d", rPeaceThresh.round());
	TeamTypes eImminentWarTarget = NO_TEAM;
	if (iU >= rPeaceThresh)
	{
		/*  Peace so we can free our hands for a different war.
			(The "distraction" war utility aspect also deals with this,
			but it's normally not enough to get the AI to stop a successful
			war before starting one that looks even more worthwhile.) */
		for (TeamIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itOther(kAgent.getID());
			itOther.hasNext(); ++itOther)
		{
			TeamTypes const eOther = itOther->getID();
			if (!kAgent.AI_isSneakAttackReady(eOther))
				continue;
			eImminentWarTarget = eOther;
			FAssert(eOther != eTarget);
			if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Considering peace with %S to focus on"
					" imminent war against %S; evaluating two-front war:",
					GET_TEAM(eTarget).getName().GetCString(), GET_TEAM(eOther).getName().GetCString());
			WarEvalParameters params(kAgent.getID(), eOther, m_kLogMuteState, true);
			params.addExtraTarget(eTarget);
			/*  We're sure that we want to attack otherId.
				Only consider peace with the ExtraTarget. */
			params.setNotConsideringPeace();
			WarEvaluator eval(params);
			static int const iUWAI_MULTI_WAR_RELUCTANCE = GC.getDefineINT("UWAI_MULTI_WAR_RELUCTANCE");
			/*  Check both limited and total war instead of kAgent.AI_getWarPlan(eOther)
				-- to avoid a preference for the two-front case on account of
				greater military build-up. */
			int uLim = eval.evaluate(WARPLAN_LIMITED, 0) - iUWAI_MULTI_WAR_RELUCTANCE;
			int uTot = eval.evaluate(WARPLAN_TOTAL, 0) - iUWAI_MULTI_WAR_RELUCTANCE;
			iU = std::min(uLim, uTot);
			// Tbd.: If the war plan against otherId is TOTAL ...
			if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Utility of a two-front war compared with a war "
					"only against %S: %d", GET_TEAM(eOther).getName().GetCString(), iU);
			break; // Only one war can be imminent at a time
		}
		if (iU >= rPeaceThresh)
		{
			if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("No peace sought b/c war utility is above the peace threshold");
			if (gWarLogLevel >= 2) logBBAI("WAR_PEACE_DECISION turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s initialUtility=%d decisionUtility=%d peaceThreshold=%d imminentWarTarget=%d emergencyPeace=%d sought=0 reason=utility_above_threshold",
				GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(kAgent.AI_getWarPlan(eTarget)),
				iInitialU, iU, rPeaceThresh.round(), eImminentWarTarget, bEmergencyPeace);
			return true;
		}
	}
	// We refuse to talk for 1 turn
	if (kAgent.AI_getAtWarCounter(eTarget) <= 1)
	{
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Too early to consider peace");
		if (gWarLogLevel >= 2) logBBAI("WAR_PEACE_DECISION turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s initialUtility=%d decisionUtility=%d peaceThreshold=%d atWarCounter=%d emergencyPeace=%d sought=0 reason=minimum_war_age",
			GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(kAgent.AI_getWarPlan(eTarget)),
			iInitialU, iU, rPeaceThresh.round(), kAgent.AI_getAtWarCounter(eTarget), bEmergencyPeace);
		return true;
	}
	CvPlayerAI& kTargetPlayer = GET_PLAYER(kTarget.getRandomMemberAlive(true));
	CvPlayerAI& kAgentPlayer = GET_PLAYER(kAgent.getRandomMemberAlive(false));
	if (!kAgentPlayer.canContact(kTargetPlayer.getID(), true))
	{
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Can't talk to %S about peace",
				GET_PLAYER(kTargetPlayer.getID()).getName(0));
		if (gWarLogLevel >= 2) logBBAI("WAR_PEACE_DECISION turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s initialUtility=%d decisionUtility=%d peaceThreshold=%d atWarCounter=%d emergencyPeace=%d sought=0 reason=cannot_contact",
			GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(kAgent.AI_getWarPlan(eTarget)),
			iInitialU, iU, rPeaceThresh.round(), kAgent.AI_getAtWarCounter(eTarget), bEmergencyPeace);
		return true; // Can't contact them for capitulation either
	}
	scaled rPeaceProb = 0;
	bool bOfferPeace = true;
	// <!-- custom: Emergency selection already had to compute the preferred target's costly willingness to negotiate; reuse it here instead of evaluating the same peace twice. (GPT-5.6-Sol) -->
	int iTheirReluct = (bEmergencyPeace ? iPreferredEmergencyPeaceReluctance : MIN_INT);
	// <!-- custom: CvDeal's final victory-denial guard formerly rejected the treaty after AI_negotiatePeace returned success, so UWAI repeated an ineffective peace deal every turn. Skip ordinary peace here while retaining the later capitulation check; surrender is intentionally exempt from victory-denial refusal. (GPT-5.6-Sol) -->
	bool const bVictoryDenialPeaceBlocked = (isSASUWAIVictoryDenialPeaceThreat(kAgent.getID()) || isSASUWAIVictoryDenialPeaceThreat(eTarget));
	if (bVictoryDenialPeaceBlocked)
	{
		bOfferPeace = false;
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Ordinary peace blocked while either side remains a configured victory threat");
		if (gWarLogLevel >= 1) logBBAI("WAR_PEACE_DECISION turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s initialUtility=%d decisionUtility=%d peaceThreshold=%d atWarCounter=%d emergencyPeace=%d sought=0 reason=victory_denial_treaty_blocked",
			GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(kAgent.AI_getWarPlan(eTarget)),
			iInitialU, iU, rPeaceThresh.round(), kAgent.AI_getAtWarCounter(eTarget), bEmergencyPeace);
	}
	else if (bHuman)
	{
		int const iContactDelay = kAgentPlayer.
				AI_getContactTimer(kTargetPlayer.getID(), CONTACT_PEACE_TREATY);
		int const iContactRand = GC.getInfo(kAgentPlayer.
				getPersonalityType()).getContactRand(CONTACT_PEACE_TREATY);
		int iAtWarCounter = kAgent.AI_getAtWarCounter(eTarget);
		if (iContactDelay > 0 || iContactRand <= 0 ||
			kAgent.AI_getWarPlan(eTarget) == WARPLAN_ATTACKED_RECENT ||
			kAgentPlayer.AI_refuseToTalkTurns(kTargetPlayer.getID()) > iAtWarCounter)
		{
			if (iContactDelay > 0)
			{
				if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("No peace with human sought b/c of contact delay: %d",
						iContactDelay);
			}
			else if (iContactRand <= 0)
			{
				if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("No peace sought b/c %S never seeks peace",
						GET_PLAYER(kAgentPlayer.getID()).getName(0));
			}
			else if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("No peace sought b/c war too recent: %d turns", iAtWarCounter);
			rPeaceProb = 0; // Don't return; capitulation always needs to be checked.
			bOfferPeace = false;
		}
		else
		{
			/*	(Going through CvPlayerAI::AI_contactRoll gets too complicated here,
				and wouldn't matter b/c we don't speed-adjust peace rolls.) */
			rPeaceProb = scaled(1, iContactRand); // 5 to 10%
			// Adjust probability based on whether peace looks like win-win or zero-sum
			if (iTheirReluct == MIN_INT)
				iTheirReluct = kTarget.uwai().reluctanceToPeace(kAgent.getID(), false);
			scaled rWinWinFactor = scaled(iTheirReluct + iU, -15);
			if (rWinWinFactor < 0)
			{
				rWinWinFactor.flipSign();
				rWinWinFactor.decreaseTo(fixp(2.5));
				rWinWinFactor.flipFraction();
			}
			else rWinWinFactor.decreaseTo(fixp(2.5));
			/*  Tbd.: (5*2.5)% seems a bit much for leaders that should be reluctant
				to sue for peace. Exponentiate rPeaceProb? Subtract a percentage point
				or two before applying the rWinWinFactor? */
			rPeaceProb *= rWinWinFactor;
			if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Win-win factor: %d percent", rWinWinFactor.getPercent());
		}
	}
	else
	{
		FAssert(iU < rPeaceThresh);
		// Base AdvCiv: rPeaceProb = (rPeaceThresh - iU).sqrt() * fixp(0.03);
		// <!-- custom: Base AdvCiv gave every negative AI-vs-AI review a random chance to initiate peace. Repeated marginal rolls increasingly risked abandoning useful pressure, while clearly bad wars could wait several turns before seeking peace; volatile one-turn utility also made the outcome depend on whether a roll happened during a temporary dip.
		// In save file 452, a 15-point prototype immediately ended Egypt's advantageous Japan war at -32. With a 40-point margin, utility rebounded to +87 one turn later, Egypt captured population-7 Tokyo, then accepted Japan's peace request on turn 175 after costly attrition. Mali likewise held a -7 dip, but accepted India's request one turn later, so the margin did not prevent later rational peace.
		// Treat a smaller deficit as insufficient reason to initiate peace, but seek peace immediately once it reaches the tunable decisive margin. The opponent can still propose an acceptable treaty, and emergency peace remains immediate; human contact retains personality-based pacing. (GPT-5.6-Sol) -->
		static int const iDecisivePeaceMargin = GC.getDefineINT("SAS_UWAI_AI_PEACE_DECISIVE_UTILITY_MARGIN");
		scaled const rPeaceUtilityDeficit = rPeaceThresh - iU;
		if (rPeaceUtilityDeficit >= iDecisivePeaceMargin)
			rPeaceProb = fixp(1);
		else
		{
			bOfferPeace = false;
			if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("No AI peace initiated b/c utility deficit %d is below decisive margin %d", rPeaceUtilityDeficit.round(), iDecisivePeaceMargin);
			if (gWarLogLevel >= 2) logBBAI("WAR_PEACE_DECISION turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s initialUtility=%d decisionUtility=%d peaceThreshold=%d utilityDeficit=%d decisiveMargin=%d emergencyPeace=%d sought=0 reason=utility_deficit_below_decisive_margin",
				GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(kAgent.AI_getWarPlan(eTarget)),
				iInitialU, iU, rPeaceThresh.round(), rPeaceUtilityDeficit.round(), iDecisivePeaceMargin, bEmergencyPeace);
		}
	}

	// <!-- custom: add emergency peace in multi wars as part of our fix as well, code provided by chatgpt 5, check if accurate, and see also known issue as of now 65 for details -->
	if (bEmergencyPeace && bOfferPeace)
	{
		rPeaceProb = fixp(1); // 100%
	}

	if (bOfferPeace)
	{
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Probability for peace negotiation: %d percent",
				rPeaceProb.getPercent());
		bool const bRandomlySkipped = (rPeaceProb < 1 && SyncRandSuccess(1 - rPeaceProb));
		if (gWarLogLevel >= 2) logBBAI("WAR_PEACE_NEGOTIATION_CHECK turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s initialUtility=%d decisionUtility=%d peaceThreshold=%d atWarCounter=%d majorWars=%d enemyPowerPercent=%d adjustedEnemyPowerPercent=%d emergencyPeace=%d imminentWarTarget=%d peaceProbabilityPercent=%d randomlySkipped=%d",
			GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(kAgent.AI_getWarPlan(eTarget)),
			iInitialU, iU, rPeaceThresh.round(), kAgent.AI_getAtWarCounter(eTarget), iMajorWars, iEnemyPowerPercent,
			iAdjustedEnemyPowerPercent, bEmergencyPeace, eImminentWarTarget, rPeaceProb.getPercent(), bRandomlySkipped);
		if (bRandomlySkipped)
		{
			if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Peace negotiation randomly skipped");
			if (!bHuman)
			{
				// Don't consider capitulation to AI w/o having tried peace negotiation
				return true;
			}
			bOfferPeace = false;
		}
	}
	if (iTheirReluct == MIN_INT)
		iTheirReluct = kTarget.uwai().reluctanceToPeace(kAgent.getID(), false);
	if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Their reluctance to peace: %d", iTheirReluct);
	if (bOfferPeace)
	{
		if (iTheirReluct <= iMaxReparationUtility)
		{
			int iTradeVal = 0;
			int iDemandVal = 0; // (Demand only from humans)
			if (bHuman)
			{
				iTradeVal = endWarVal(eTarget) - kTarget.uwai().endWarVal(kAgent.getID());
				// A bit higher than the K-Mod discounts (advc.134a)
				scaled const rDiscountFactor = fixp(1.3);
				if (iTradeVal < 0)
				{
					iDemandVal = -iTradeVal;
					// Offer a square deal when it's close
					if (iDemandVal < kTargetPlayer.uwai().utilityToTradeVal(fixp(4.25)))
						iDemandVal = 0;
					else iDemandVal = (iDemandVal / rDiscountFactor).uround();
					if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Seeking reparations with a trade value of %d", iDemandVal);
					iTradeVal = 0;
				}
				else iTradeVal = (iTradeVal * rDiscountFactor).uround();
			}
			else
			{
				// Base the reparations they demand on their economy
				iTradeVal = (kTarget.uwai().utilityToTradeVal(
						std::max(0, iTheirReluct))).uround();
				/*  Reduce the trade value b/c the war isn't completely off the table;
					could continue after 10 turns. */
				iTradeVal = (iTradeVal * rReparationsModifierAI).uround();
			}
			if (iTradeVal > 0 || iDemandVal == 0)
			{
				if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Trying to offer reparations with a trade value of %d",
						iTradeVal);
			}
			bool bPeace = false;
			// <!-- custom: AI_negotiatePeace clears the war plan, counter and war-success state when peace succeeds. Cache the pre-negotiation values so the result row describes the decision that ended the war instead of logging reset zeros.
			// A fresh Pangaea diagnostic run also showed that the victory-denial deal guard can reject a treaty after AI_negotiatePeace returns true. Log the post-call war state separately so blocked treaties are not mistaken for completed peace. (GPT-5.6-Sol) -->
			WarPlanTypes eLoggedWarPlan = NO_WARPLAN;
			int iLoggedAtWarCounter = -1, iLoggedOurPower = -1, iLoggedTargetDefensivePower = -1, iLoggedTargetPowerPercent = -1;
			int iLoggedOurCities = -1, iLoggedTargetCities = -1, iLoggedOurWarSuccess = -1, iLoggedTargetWarSuccess = -1;
			if (gWarLogLevel >= 1)
			{
				eLoggedWarPlan = kAgent.AI_getWarPlan(eTarget);
				iLoggedAtWarCounter = kAgent.AI_getAtWarCounter(eTarget);
				iLoggedOurPower = std::max(1, kAgent.getPower(true));
				iLoggedTargetDefensivePower = kTarget.getDefensivePower(kAgent.getID());
				iLoggedTargetPowerPercent = (100 * iLoggedTargetDefensivePower) / iLoggedOurPower;
				iLoggedOurCities = kAgent.getNumCities();
				iLoggedTargetCities = kTarget.getNumCities();
				iLoggedOurWarSuccess = kAgent.AI_getWarSuccess(eTarget).round();
				iLoggedTargetWarSuccess = kTarget.AI_getWarSuccess(kAgent.getID()).round();
			}
			if (!isInBackground())
			{
				bPeace = kAgentPlayer.AI_negotiatePeace(kTargetPlayer.getID(),
						iDemandVal, iTradeVal);
			}
			if (bHuman)
			{
				if (gUWAIAgentLogLevel >= 1 && !m_kLogMuteState.isMuted())
				{
					if (bPeace)
						logBBAI("Peace offer sent");
					else logBBAI("Failed to find a peace offer");
				}
			}
			else if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Peace negotiation %s", (bPeace ? "succeeded" : "failed"));
			bool const bWarEnded = !kAgent.isAtWar(eTarget);
			if (gWarLogLevel >= 2 || (bPeace && gWarLogLevel >= 1)) logBBAI("WAR_PEACE_NEGOTIATION_RESULT turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s initialUtility=%d decisionUtility=%d peaceThreshold=%d atWarCounter=%d majorWars=%d enemyPowerPercent=%d adjustedEnemyPowerPercent=%d emergencyPeace=%d imminentWarTarget=%d peaceProbabilityPercent=%d theirReluctance=%d maxReparationUtility=%d tradeValue=%d demandValue=%d negotiationReturnedSuccess=%d warEnded=%d ourPower=%d targetDefensivePower=%d targetPowerPercent=%d ourCities=%d targetCities=%d ourWarSuccess=%d targetWarSuccess=%d",
				GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eLoggedWarPlan), iInitialU, iU,
				rPeaceThresh.round(), iLoggedAtWarCounter, iMajorWars, iEnemyPowerPercent, iAdjustedEnemyPowerPercent, bEmergencyPeace,
				eImminentWarTarget, rPeaceProb.getPercent(), iTheirReluct, iMaxReparationUtility, iTradeVal, iDemandVal, bPeace, bWarEnded,
				iLoggedOurPower, iLoggedTargetDefensivePower, iLoggedTargetPowerPercent, iLoggedOurCities, iLoggedTargetCities,
				iLoggedOurWarSuccess, iLoggedTargetWarSuccess);
			return !bPeace;
		}
		else
		{
			if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("No peace negotiation attempted; they're too reluctant");
			if (gWarLogLevel >= 2) logBBAI("WAR_PEACE_NEGOTIATION_RESULT turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s initialUtility=%d decisionUtility=%d peaceThreshold=%d atWarCounter=%d majorWars=%d enemyPowerPercent=%d adjustedEnemyPowerPercent=%d emergencyPeace=%d imminentWarTarget=%d peaceProbabilityPercent=%d theirReluctance=%d maxReparationUtility=%d negotiationReturnedSuccess=0 warEnded=0 reason=target_reluctant",
				GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(kAgent.AI_getWarPlan(eTarget)),
				iInitialU, iU, rPeaceThresh.round(), kAgent.AI_getAtWarCounter(eTarget), iMajorWars, iEnemyPowerPercent,
				iAdjustedEnemyPowerPercent, bEmergencyPeace, eImminentWarTarget, rPeaceProb.getPercent(), iTheirReluct,
				iMaxReparationUtility);
		}
	}
	if (considerCapitulation(eTarget, iU, iTheirReluct))
		return true; // No surrender
	int const iCities = kAgent.getNumCities();
	/*  Otherwise, considerCapitulation guarantees that surrender (to AI)
		is possible; before we do it, one last attempt to find help: */
	if (iCities > 1 && !tryFindingMaster(eTarget))
		return false; // Have become a vassal, or awaiting human response.
	// Liberate any colonies (to leave sth. behind, or just to spite the enemy)
	if (!isInBackground())
	{
		for (MemberAIIter it(kAgent.getID()); it.hasNext(); ++it)
			it->AI_doSplit(true);
	}
	if (kAgent.getNumCities() != iCities)
	{
		if (gUWAIAgentLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("Empire split");
		return false; // Leads to re-evaluation of war plans; may yet capitulate.
	}
	if (kAgentPlayer.AI_getContactTimer(kTargetPlayer.getID(), CONTACT_PEACE_TREATY) <= 0)
	{
		if (gUWAIAgentLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("%s capitulation to %S", bHuman ? "Offering" : "Implementing",
				GET_PLAYER(kTargetPlayer.getID()).getName(0));
		if (!isInBackground())
		{
			kAgentPlayer.AI_offerCapitulation(kTargetPlayer.getID(), true);
			return false;
		}
	}
	return true;
}


bool UWAI::Team::considerCapitulation(TeamTypes eMaster, int iAgentWarUtility, int iMasterReluctancePeace)
{
	{
		int const iUtilityThresh = -75;
		if (iAgentWarUtility * 4 > iUtilityThresh)
		{
			if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Don't compute capitulation utility b/c probably"
					" not low enough (%d>%d)", iAgentWarUtility, iUtilityThresh / 4);
			return true;
		}
		if (iAgentWarUtility > iUtilityThresh)
		{
			int iCapitulationUtility = iAgentWarUtility;
			if (GET_TEAM(m_eAgent).getNumWars(true, true) > 1)
			{
				/*	Looks like war utility is low, but not low enough. Perhaps
					this is b/c we haven't yet accounted for the protection that
					the master grants us from third parties.
					NB: Ideally, considerCapitulation should not rely on iAgentWarUtility
					at all when there are multiple (free) war enemies, but that's
					now difficult to change at the call site. */
				if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Computing war utility of capitulation (%d>%d)",
						iAgentWarUtility, iUtilityThresh);
				WarEvalParameters params(m_eAgent, eMaster, m_kLogMuteState, false,
						NO_PLAYER, eMaster);
				WarEvaluator eval(params);
				iCapitulationUtility = eval.evaluate(GET_TEAM(m_eAgent).AI_getWarPlan(eMaster));
			}
			if (iCapitulationUtility > iUtilityThresh)
			{
				if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("No capitulation b/c utility not low enough (%d>%d)",
						iCapitulationUtility, iUtilityThresh);
				return true;
			}
		}
	}
	scaled rSkipProb;
	int const iAgentCities = GET_TEAM(m_eAgent).getNumCities();
	if (iAgentCities > 1) // Can't afford to wait with just 1 city left
	{
		/*  Low reluctance to peace can just mean that there isn't much left for
			them to conquer; doesn't have to mean that they'll soon offer peace.
			Probability test to ensure that we eventually capitulate even if
			master's reluctance remains low. */
		rSkipProb = 1 - (iMasterReluctancePeace * fixp(0.015) + fixp(0.3));
		rSkipProb.clamp(0,
				// Reduce maximal waiting time in the late game
				fixp(0.87) - fixp(0.04) * GET_TEAM(eMaster).AI_getCurrEraFactor());
		if (iAgentCities <= 2)
			rSkipProb -= fixp(0.25);
	}
	if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("%d percent probability to delay capitulation based on master's "
			"reluctance to peace (%d)", rSkipProb.getPercent(), iMasterReluctancePeace);
	if (SyncRandSuccess(rSkipProb))
	{
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("No capitulation this turn");
		return true;
	}
	if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted() && rSkipProb.isPositive()) logBBAI("Not skipped");
	/*  Since capitulation trade denial is decided at the team level, it doesn't matter
		which team members are used. */
	CvPlayerAI const& kAgentLeader = GET_PLAYER(GET_TEAM(m_eAgent).getLeaderID());
	CvTeamAI const& kMaster = GET_TEAM(eMaster);
	if (!kAgentLeader.canTradeItem(kMaster.getLeaderID(), TradeData(TRADE_SURRENDER)))
	{
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Capitulation to %S impossible", GET_TEAM(eMaster).getName().GetCString());
		return true;
	}
	bool const bHumanMaster = GET_TEAM(eMaster).isHuman();
	/*  Make master team accept if it's not sure about continuing the war. Note that,
		due to change advc.130v, gaining a vassal can't really hurt the master. */
	bool const bCheckAccept = (!bHumanMaster && iMasterReluctancePeace >= 15);
	if (!bCheckAccept && !bHumanMaster)
	{
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Master accepts capitulation b/c of low reluctance to peace (%d)",
				iMasterReluctancePeace);
	}
	if (bHumanMaster)
	{
		// This allows AI_surrenderTrade to return true (for a human master)
		leaderCache().setReadyToCapitulate(eMaster, true);
	}
	// Checks our willingness and that of the master
	DenialTypes eDenial = GET_TEAM(m_eAgent).AI_surrenderTrade(
			eMaster, CvTeamAI::VASSAL_POWER_MOD_SURRENDER, bCheckAccept);
	if (eDenial != NO_DENIAL)
	{
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Not ready to capitulate%s; denial code: %d",
				bCheckAccept ? " (or master refuses)" : "", (int)eDenial);
		if (bHumanMaster)
		{
			/*  To ensure that the capitulation decision is made on an AI turn;
				so that tryFindingMaster and AI_doSplit are called by considerPeace. */
			leaderCache().setReadyToCapitulate(eMaster, false);
		}
		return true;
	}
	if (gUWAIAgentLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("%S ready to capitulate to %S", GET_TEAM(m_eAgent).getName().GetCString(),
			GET_TEAM(eMaster).getName().GetCString());
	return false;
}


bool UWAI::Team::tryFindingMaster(TeamTypes eEnemy)
{
	CvPlayerAI& kAgentPlayer = GET_PLAYER(GET_TEAM(m_eAgent).getRandomMemberAlive(false));
	for (TeamAIRandIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itMaster(syncRand(), m_eAgent);
		itMaster.hasNext(); ++itMaster)
	{
		CvTeamAI& kMaster = *itMaster;
		if (kMaster.isAtWar(m_eAgent) ||
			kMaster.isAtWar(eEnemy)) // No point if they're already our ally
		{
			continue;
		}
		CvPlayerAI& kMasterPlayer = GET_PLAYER(kMaster.getRandomMemberAlive(true));
		if (!kAgentPlayer.canContact(kMasterPlayer.getID(), true))
			continue;
		// Based on code in CvPlayerAI::AI_doDiplo
		TradeData item(TRADE_VASSAL);
		/*  Test Denial separately b/c it can cause the master to evaluate war
			against enemyId, which is costly. */
		if (!kAgentPlayer.canTradeItem(kMasterPlayer.getID(), item))
			continue;
		// Don't nag them (especially not humans)
		if (kAgentPlayer.AI_getContactTimer(kMasterPlayer.getID(),
			// Same contact memory for alliance and vassal agreement
			CONTACT_PERMANENT_ALLIANCE) != 0)
		{
			if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("%S not asked for protection b/c recently contacted",
					GET_PLAYER(kMasterPlayer.getID()).getName(0));
			continue;
		}
		// Checks both our and master's willingness
		if (kAgentPlayer.getTradeDenial(kMasterPlayer.getID(), item) != NO_DENIAL)
			continue;
		if (kMaster.isHuman())
		{
			if (gUWAIAgentLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("Asking human %S for vassal agreement",
					GET_PLAYER(kMasterPlayer.getID()).getName(0));
		}
		else if (gUWAIAgentLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("Signing vassal agreement with %S",
				GET_TEAM(kMaster.getID()).getName().GetCString());
		if (!isInBackground())
{
			CLinkList<TradeData> ourList, theirList;
			ourList.insertAtEnd(item);
			// <!-- custom: This third-party protector path otherwise collapses into an ordinary TRADE_VASSAL action; preserve only the realized origin/current enemy/package and leave rejected-master reasoning in BBAI. (ChatGPT-5.6-Sol) -->
			if (gGameRecordLogLevel >= 2) logSASGameRecordAIVassalageDecision(kAgentPlayer, kMasterPlayer.getID(), SAS_AI_VASSALAGE_UWAI_FIND_MASTER,
				eEnemy, -1, -1, -1, -1, ourList, theirList);
			if (kMaster.isHuman())
			{
				kAgentPlayer.AI_changeContactTimer(kMasterPlayer.getID(),
						CONTACT_PERMANENT_ALLIANCE,
						kAgentPlayer.AI_getContactDelay(CONTACT_PERMANENT_ALLIANCE));
				CvDiploParameters* pDiplo = new CvDiploParameters(kAgentPlayer.getID());
				pDiplo->setDiploComment(GC.getAIDiploCommentType("OFFER_VASSAL"));
				pDiplo->setAIContact(true);
				pDiplo->setOurOfferList(theirList);
				pDiplo->setTheirOfferList(ourList);
				gDLL->beginDiplomacy(pDiplo, kMasterPlayer.getID());
			}
			else
			{
				GC.getGame().implementDeal(kAgentPlayer.getID(),
						kMasterPlayer.getID(), ourList, theirList);
			}
		}
		return false;
	}
	if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("No partner for a voluntary vassal agreement found");
	return true;
}


bool UWAI::Team::considerPlanTypeChange(TeamTypes eTarget, int iU)
{
	CvTeamAI& kAgent = GET_TEAM(m_eAgent);
	FAssert(kAgent.isAtWar(eTarget));
	WarPlanTypes const eWP = kAgent.AI_getWarPlan(eTarget);
	int const iWPAge = kAgent.AI_getWarPlanStateCounter(eTarget);
	WarPlanTypes eAltWP = NO_WARPLAN;
	switch (eWP)
	{
	case WARPLAN_ATTACKED_RECENT:
		// Same as BBAI in CvTeamAI::AI_doWar, but switch after 8 turns (not 10).
		if (iWPAge >= 4)
		{
			if (!GET_TEAM(eTarget).AI_isLandTarget(kAgent.getID()) || iWPAge >= 8)
			{
				if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Switching to war plan \"attacked\" after %d turns", iWPAge);
				if (!isInBackground())
				{
					if (gGameRecordLogLevel >= 2) logSASGameRecordUWAIWarPlanDecision(kAgent.getID(), eTarget, SAS_UWAI_WAR_PLAN_ATTACKED_RECENT_MATURED, eWP, WARPLAN_ATTACKED, iU, iWPAge, -1);
					kAgent.AI_setWarPlan(eTarget, WARPLAN_ATTACKED);
					// Don't reset wpAge
					kAgent.AI_setWarPlanStateCounter(eTarget, iWPAge);
				}
				return false;
			}
		}
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Too early to switch to \"attacked\" war plan");
		break;
	// Treat these three as limited wars, and consider switching to total.
	case WARPLAN_ATTACKED:
	case WARPLAN_DOGPILE:
	case WARPLAN_LIMITED:
		eAltWP = WARPLAN_TOTAL;
		break;
	case WARPLAN_TOTAL:
		eAltWP = WARPLAN_LIMITED;
		break;
	default: FErrorMsg("Unsuitable war plan type");
	}
	if (eAltWP == NO_WARPLAN)
		return true;
	// <!-- custom: This alternative is gameplay input for the outer decision, but its full nested diagnostic trace would only duplicate that outer review. (ChatGPT-5.6-Sol) -->
	UWAILogMuteState silentLogMuteState(true);
	WarEvalParameters params(kAgent.getID(), eTarget, silentLogMuteState);
	WarEvaluator eval(params);
	int iAltU = eval.evaluate(eAltWP);
	if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Utility of alt. war plan (%s): %d",
			getSASWarPlanType(eAltWP), iAltU);
	scaled rSwitchProb;
	if (iAltU > iU)
	{
		// Increase both utility values if iU is close to 0
		int iPadding = 0;
		if (iU < 20)
			iPadding += 20 - iU;
		rSwitchProb = scaled(iAltU + iPadding, 4 * (iU + iPadding));
	}
	if (rSwitchProb.isPositive())
	{
		scaled const rLimitedWarWeight = limitedWarWeight();
		if (eAltWP == WARPLAN_LIMITED)
			rSwitchProb *= rLimitedWarWeight;
		else
		{
			rSwitchProb = (rLimitedWarWeight.isPositive() ?
					rSwitchProb / rLimitedWarWeight : 1);
		}
		if (rLimitedWarWeight != 1)
		{
			if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Bias for/against limited war: %d percent",
					rLimitedWarWeight.getPercent());
		}
	}
	if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Probability of switching: %d percent", rSwitchProb.getPercent());
	if (!rSwitchProb.isPositive())
		return true;
	if (SyncRandSuccess(rSwitchProb))
	{
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Switching to war plan \"%s\"", getSASWarPlanType(eAltWP));
		if (!isInBackground())
		{
			if (gGameRecordLogLevel >= 2) logSASGameRecordUWAIWarPlanDecision(kAgent.getID(), eTarget, SAS_UWAI_WAR_PLAN_ACTIVE_TYPE_SWITCH, eWP, eAltWP, iU, iWPAge, -1, NO_TEAM, iAltU, rSwitchProb.getPercent());
			kAgent.AI_setWarPlan(eTarget, eAltWP);
			kAgent.AI_setWarPlanStateCounter(eTarget, iWPAge); // Don't reset wpAge
		}
		return false;
	}
	if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("War plan not switched; still \"%s\"", getSASWarPlanType(eWP));
	return true;
}


bool UWAI::Team::considerAbandonPreparations(TeamTypes eTarget, int iU, int iTurnsRemaining)
{
	CvTeamAI& kAgent = GET_TEAM(m_eAgent);
	WarPlanTypes const eWP = kAgent.AI_getWarPlan(eTarget);
	if (kAgent.AI_countWarPlans() > kAgent.getNumWars(true, true) + 1)
	{
		/*  Only one war should be imminent or in preparation at a time.
			(Otherwise WarEvaluator will ignore all but one plan).
			Too many plans can occur here only if UWAI was running
			in the background at some point. */
		if (gWarLogLevel >= 1) logBBAI("WAR_PREPARATION_CANCEL turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s reason=too_many_plans utility=%d stateCounter=%d prepTurnsRemaining=%d warPlanCount=%d wars=%d",
			GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), iU,
			kAgent.AI_getWarPlanStateCounter(eTarget), iTurnsRemaining, kAgent.AI_countWarPlans(), kAgent.getNumWars(true, true));
		if (!isInBackground())
		{
			kAgent.AI_setWarPlan(eTarget, NO_WARPLAN);
			showWarPlanAbandonedMsg(eTarget);
		}
		if (gUWAIAgentLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("More than one war in preparation, canceling the one against %S",
				GET_TEAM(eTarget).getName().GetCString());
		return false;
	}
	if (iU >= 0)
		return true;
	if (iTurnsRemaining <= 0)
	{
		if (gUWAIAgentLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("Time limit for preparations reached; plan abandoned");
		if (gWarLogLevel >= 1) logBBAI("WAR_PREPARATION_CANCEL turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s reason=preparation_deadline_negative_utility utility=%d stateCounter=%d prepTurnsRemaining=%d",
				GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), iU, kAgent.AI_getWarPlanStateCounter(eTarget), iTurnsRemaining);
		if (!isInBackground())
		{
			if (gGameRecordLogLevel >= 2) logSASGameRecordUWAIWarPlanDecision(kAgent.getID(), eTarget, SAS_UWAI_WAR_PLAN_PREPARATION_DEADLINE_NEGATIVE_UTILITY, eWP, NO_WARPLAN, iU, kAgent.AI_getWarPlanStateCounter(eTarget), iTurnsRemaining);
			kAgent.AI_setWarPlan(eTarget, NO_WARPLAN);
			showWarPlanAbandonedMsg(eTarget);
		}
		return false;
	}
	// <!-- custom: The deterministic severity gate initially canceled newly selected plans after only one review when transient UWAI/AreaAI changes briefly drove utility negative. Save-file 452 then reselected 7 of those targets on the next turn and 11 within three turns. Give a new preparation two full reviews to settle before severity alone may cancel it; hard deadline and legality failures remain immediate. See KI#189. (GPT-5.6-Sol) -->
	static int const iMinAge = GC.getDefineINT("SAS_UWAI_PREPARATION_ABANDON_MIN_AGE_TURNS_UNSCALED_GAMESPEED");
	static int const iMinAbandonSeverityPercent = GC.getDefineINT("SAS_UWAI_PREPARATION_ABANDON_MIN_SEVERITY_PERCENT");
	int const iAge = kAgent.AI_getWarPlanStateCounter(eTarget);
	if (iAge < iMinAge)
	{
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Preparation abandonment deferred until age %d (current age %d)", iMinAge, iAge);
		if (gWarLogLevel >= 2) logBBAI("WAR_PREPARATION_ABANDON_CHECK turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s utility=%d stateCounter=%d prepTurnsRemaining=%d reason=minimum_age minAbandonAge=%d warRand=-1 abandonSeverityPercent=-1 minAbandonSeverityPercent=%d abandoned=0 distance=%d targetVictoryCountdown=%d targetPowerPercent=%d",
			GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), iU, iAge, iTurnsRemaining,
			iMinAge, iMinAbandonSeverityPercent, getSASBBAINearestCityDistance(kAgent.getID(), eTarget),
			GET_TEAM(eTarget).AI_getLowestVictoryCountdown(), getSASBBAITargetPowerPercent(kAgent, eTarget));
		return true;
	}
	WarPlanTypes eEvaluationWP = eWP;
	if (isInBackground() && eEvaluationWP == WARPLAN_DOGPILE)
		eEvaluationWP = WARPLAN_PREPARING_LIMITED;
	int iWarRand = -1;
	if (eEvaluationWP == WARPLAN_PREPARING_LIMITED)
		iWarRand = kAgent.AI_limitedWarRand();
	if (eEvaluationWP == WARPLAN_PREPARING_TOTAL)
		iWarRand = kAgent.AI_maxWarRand();
	FAssert(iWarRand >= 0);
	// WarRand is between 40 (aggro) and 400 (chilled)
	// <!-- custom: UWAI named this value rAbandonProb and rolled it on every negative review, so even a mildly doubtful unchanged preparation was eventually canceled. Rename it rAbandonSeverity because the same personality/game-speed-adjusted value is now compared with a deterministic threshold and only clearly bad plans are abandoned; save-file 452 showed this would reduce 69 random cancellations to the 38 checks that reached the default 100% severity. See KI#189. (GPT-5.6-Sol) -->
	scaled rAbandonSeverity(-iU * iWarRand, 7500);
	// Slight adjustment to training speed
	rAbandonSeverity *= 2;
	rAbandonSeverity /= per100(GC.getInfo(GC.getGame().getGameSpeedType()).
			getTrainPercent()) + 1;
	rAbandonSeverity.decreaseTo(1);
	bool const bAbandon = (rAbandonSeverity.getPercent() >= iMinAbandonSeverityPercent);
	if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Preparation abandonment severity %d percent; threshold %d (warRand=%d)", rAbandonSeverity.getPercent(), iMinAbandonSeverityPercent, iWarRand);
	if (gWarLogLevel >= 2) logBBAI("WAR_PREPARATION_ABANDON_CHECK turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s utility=%d stateCounter=%d prepTurnsRemaining=%d warRand=%d abandonSeverityPercent=%d minAbandonSeverityPercent=%d abandoned=%d distance=%d targetVictoryCountdown=%d targetPowerPercent=%d",
		GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), iU, iAge, iTurnsRemaining, iWarRand,
		rAbandonSeverity.getPercent(), iMinAbandonSeverityPercent, bAbandon, getSASBBAINearestCityDistance(kAgent.getID(), eTarget),
		GET_TEAM(eTarget).AI_getLowestVictoryCountdown(), getSASBBAITargetPowerPercent(kAgent, eTarget));
	if (bAbandon)
	{
		if (gUWAIAgentLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("Preparations abandoned");
		if (gWarLogLevel >= 1) logBBAI("WAR_PREPARATION_CANCEL turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s reason=severe_negative_utility utility=%d stateCounter=%d prepTurnsRemaining=%d warRand=%d abandonSeverityPercent=%d minAbandonSeverityPercent=%d",
			GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), iU, iAge, iTurnsRemaining,
			iWarRand, rAbandonSeverity.getPercent(), iMinAbandonSeverityPercent);
		if (!isInBackground())
		{
			if (gGameRecordLogLevel >= 2) logSASGameRecordUWAIWarPlanDecision(kAgent.getID(), eTarget, SAS_UWAI_WAR_PLAN_SEVERE_NEGATIVE_UTILITY, eWP, NO_WARPLAN, iU, iAge, iTurnsRemaining, NO_TEAM, -1, rAbandonSeverity.getPercent(), iMinAbandonSeverityPercent);
			kAgent.AI_setWarPlan(eTarget, NO_WARPLAN);
			showWarPlanAbandonedMsg(eTarget);
		}
		return false;
	}
	else if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Preparations not abandoned");
	return true;
}


bool UWAI::Team::considerSwitchTarget(TeamTypes eTarget, int iU, int iTurnsRemaining)
{
	CvTeamAI& kAgent = GET_TEAM(m_eAgent);
	WarPlanTypes const eWP = kAgent.AI_getWarPlan(eTarget);
	TeamTypes eBestAltTarget = NO_TEAM;
	int iBestUtility = 0;
	bool const bQualms = (iTurnsRemaining > 0 && kAgent.AI_isAvoidWar(eTarget, true));
	bool bAltQualms = false; // Qualms about attacking the alt. target
	for (TeamIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itAlt(kAgent.getID());
		itAlt.hasNext(); ++itAlt)
	{
		TeamTypes const eAltTarget = itAlt->getID();
		if (!canSchemeAgainst(eAltTarget, false) ||
			kAgent.turnsOfForcedPeaceRemaining(eAltTarget) > iTurnsRemaining)
		{
			continue;
		}
		bool bLoopQualms = kAgent.AI_isAvoidWar(eAltTarget, true);
		if (bLoopQualms && !bQualms)
			continue;
		UWAILogMuteState silentLogMuteState(true);
		WarEvalParameters params(kAgent.getID(), eAltTarget, silentLogMuteState, true);
		WarEvaluator eval(params);
		int iAltU = eval.evaluate(eWP, iTurnsRemaining);
		// <!-- custom: Compare targets using the same victory-denial value used when they are first selected and later reviewed. (GPT-5.6-Sol) -->
		iAltU += getSASBBAIVictoryDenialUtilityBoost(eAltTarget, getSASTeamMaxVictoryStage(eAltTarget));
		if (iAltU > std::max(iBestUtility, bQualms ? 0 : iU))
		{
			eBestAltTarget = eAltTarget;
			iBestUtility = iAltU;
			bAltQualms = bLoopQualms;
		}
	}
	if (eBestAltTarget == NO_TEAM)
	{
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("No other promising target for war preparations found");
		return true;
	}
	// <!-- custom: UWAI named this result rSwitchProb and rolled it on every review. Preserve its relative-target calculation but rename it rSwitchAdvantage, then compare it with a deterministic threshold so a clearly better target is reliably selected and a small fluctuation never redirects an established preparation. Save-file 452's default threshold accepts 15 of 71 comparisons instead of producing 22 roll-dependent switches. See KI#189. (GPT-5.6-Sol) -->
	int iPadding = 0;
	if (std::min(iU, iBestUtility) < 20)
		iPadding += 20 - std::min(iU, iBestUtility);
	scaled rSwitchAdvantage = fixp(0.75) * (1 - scaled(iU + iPadding, iBestUtility + iPadding));
	// Slight adjustment to training speed
	if (rSwitchAdvantage.isPositive())
	{
		rSwitchAdvantage *= 2;
		rSwitchAdvantage /= per100(GC.getInfo(GC.getGame().getGameSpeedType()).
				getTrainPercent()) + 1;
	}
	if (bQualms && !bAltQualms)
		rSwitchAdvantage += fixp(1.8);
	static int const iMinSwitchAdvantagePercent = GC.getDefineINT("SAS_UWAI_PREPARATION_TARGET_SWITCH_MIN_ADVANTAGE_PERCENT");
	bool const bSwitch = (rSwitchAdvantage.getPercent() >= iMinSwitchAdvantagePercent);
	if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Best alternative target %S (u=%d) has advantage score %d percent; threshold %d",
			GET_TEAM(eBestAltTarget).getName().GetCString(), iBestUtility, rSwitchAdvantage.getPercent(), iMinSwitchAdvantagePercent);
	if (gWarLogLevel >= 1) logBBAI("WAR_TARGET_SWITCH_CHECK turn=%d background=%d agentTeam=%d oldTargetTeam=%d newTargetTeam=%d warPlan=%s oldUtility=%d newUtility=%d prepTurnsRemaining=%d stateCounter=%d switchAdvantagePercent=%d minSwitchAdvantagePercent=%d switched=%d oldQualms=%d newQualms=%d oldDistance=%d newDistance=%d oldAttitude=%d newAttitude=%d oldAttitudeValue=%d newAttitudeValue=%d oldTargetPowerPercent=%d newTargetPowerPercent=%d",
		GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, eBestAltTarget, getSASWarPlanType(eWP), iU, iBestUtility,
		iTurnsRemaining, kAgent.AI_getWarPlanStateCounter(eTarget), rSwitchAdvantage.getPercent(), iMinSwitchAdvantagePercent, bSwitch,
		bQualms, bAltQualms, getSASBBAINearestCityDistance(kAgent.getID(), eTarget),
		getSASBBAINearestCityDistance(kAgent.getID(), eBestAltTarget), kAgent.AI_getAttitude(eTarget), kAgent.AI_getAttitude(eBestAltTarget),
		kAgent.AI_getAttitudeVal(eTarget), kAgent.AI_getAttitudeVal(eBestAltTarget), getSASBBAITargetPowerPercent(kAgent, eTarget),
		getSASBBAITargetPowerPercent(kAgent, eBestAltTarget));
	if (!bSwitch)
	{
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Target not switched");
		return true;
	}
	if (gUWAIAgentLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("Target switched");
	if (gWarLogLevel >= 1) logBBAI("WAR_TARGET_SWITCHED turn=%d background=%d agentTeam=%d oldTargetTeam=%d newTargetTeam=%d warPlan=%s oldUtility=%d newUtility=%d prepTurnsRemaining=%d stateCounter=%d oldDistance=%d newDistance=%d oldAttitudeValue=%d newAttitudeValue=%d oldTargetPowerPercent=%d newTargetPowerPercent=%d",
		GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, eBestAltTarget, getSASWarPlanType(eWP), iU, iBestUtility,
		iTurnsRemaining, kAgent.AI_getWarPlanStateCounter(eTarget), getSASBBAINearestCityDistance(kAgent.getID(), eTarget),
		getSASBBAINearestCityDistance(kAgent.getID(), eBestAltTarget), kAgent.AI_getAttitudeVal(eTarget),
		kAgent.AI_getAttitudeVal(eBestAltTarget), getSASBBAITargetPowerPercent(kAgent, eTarget),
		getSASBBAITargetPowerPercent(kAgent, eBestAltTarget));
	if (!isInBackground())
	{
		int iWPAge = kAgent.AI_getWarPlanStateCounter(eTarget);
		if (gGameRecordLogLevel >= 2) logSASGameRecordUWAIWarPlanDecision(kAgent.getID(), eTarget, SAS_UWAI_WAR_PLAN_TARGET_SWITCH, eWP, eWP, iU, iWPAge, iTurnsRemaining, eBestAltTarget, iBestUtility, rSwitchAdvantage.getPercent(), iMinSwitchAdvantagePercent);
		kAgent.AI_setWarPlan(eTarget, NO_WARPLAN);
		showWarPlanAbandonedMsg(eTarget);
		kAgent.AI_setWarPlan(eBestAltTarget, eWP);
		showWarPrepStartedMsg(eBestAltTarget);
		kAgent.AI_setWarPlanStateCounter(eBestAltTarget, iWPAge);
	}
	return false;
}


// <!-- custom: Added iVictoryDenialBoost so an existing preparation keeps the same victory-denial value when its preparation and immediate-war utilities are compared. (GPT-5.6-Sol) -->
bool UWAI::Team::considerConcludePreparations(TeamTypes eTarget, int iU, int iTurnsRemaining, int iVictoryDenialBoost)
{
	CvTeamAI& kAgent = GET_TEAM(m_eAgent);
	if (kAgent.AI_countWarPlans() > kAgent.getNumWars(true, true) + 1)
	{
		// More than 1 war in preparation; let considerAbandonPreparations handle it.
		return true;
	}
	int const iTurnsOfPeace = kAgent.turnsOfForcedPeaceRemaining(eTarget);
	if (iTurnsOfPeace > 3)
	{
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Can't finish preparations b/c of peace treaty (%d turns"
				" to cancel)", iTurnsOfPeace);
		if (gWarLogLevel >= 2) logBBAI("WAR_PREPARATION_CONCLUDE_CHECK turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s utility=%d stateCounter=%d prepTurnsRemaining=%d reason=forced_peace forcedPeaceTurns=%d concluded=0",
			GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(kAgent.AI_getWarPlan(eTarget)), iU,
			kAgent.AI_getWarPlanStateCounter(eTarget), iTurnsRemaining, iTurnsOfPeace);
		return true;
	}
	WarPlanTypes const eWP = kAgent.AI_getWarPlan(eTarget);
	WarPlanTypes eDirectWP = WARPLAN_LIMITED;
	if (eWP == WARPLAN_PREPARING_TOTAL)
		eDirectWP = WARPLAN_TOTAL;
	bool bConclude = false;
	if (iTurnsRemaining <= 0)
	{
		if (iU < 0)
		{
			if (gWarLogLevel >= 2) logBBAI("WAR_PREPARATION_CONCLUDE_CHECK turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s utility=%d stateCounter=%d prepTurnsRemaining=%d reason=deadline_negative_utility concluded=0",
					GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), iU, kAgent.AI_getWarPlanStateCounter(eTarget), iTurnsRemaining);
			return true; // Let considerAbandonPreparations handle it
		}
		if (gUWAIAgentLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("Time limit for preparation reached; adopting direct war plan");
		if (gWarLogLevel >= 2) logBBAI("WAR_PREPARATION_CONCLUDE_CHECK turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s utility=%d stateCounter=%d prepTurnsRemaining=%d reason=deadline_reached concluded=1",
				GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), iU, kAgent.AI_getWarPlanStateCounter(eTarget), iTurnsRemaining);
		if (gGameRecordLogLevel >= 2 && !isInBackground()) logSASGameRecordUWAIWarPlanDecision(kAgent.getID(), eTarget, SAS_UWAI_WAR_PLAN_PREPARATION_DEADLINE_REACHED, eWP, eDirectWP, iU, kAgent.AI_getWarPlanStateCounter(eTarget), iTurnsRemaining, NO_TEAM, -1, -1, -1, iVictoryDenialBoost);
		bConclude = true;
	}
	else
	{
		UWAILogMuteState silentLogMuteState(true);
		WarEvalParameters params(kAgent.getID(), eTarget, silentLogMuteState);
		WarEvaluator eval(params);
		int iDirectU = eval.evaluate(eDirectWP) + iVictoryDenialBoost;
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Utility of immediate switch to direct war plan: %d", iDirectU);
		if (iDirectU > 0)
		{
			// <!-- custom: UWAI randomly selected a new threshold on every review, so an unchanged preparation could unpredictably conclude or continue. Keep the inherited random lines and original explanation below commented out for reference; the active midpoint preserves the intended time/readiness progression deterministically. See KI#189. (GPT-5.6-Sol) -->
			// scaled rRandWeight = SyncRandFract(scaled);
			/*  The more time remains, the longer we'd still have to wait in order
				to achieve utility iU. Therefore use a low threshold
				if iTurnsRemaining is high.
				Example: 10 turns remaining, iU=80: thresh between 32 and 80.
						 8 turns later, if still iU=80: thresh between 64 and 80. */
			scaled rRandPortion(iTurnsRemaining, 10); 
			rRandPortion.decreaseTo(fixp(0.4));
			// scaled rThresh = iU * ((1 - rRandPortion) + rRandWeight * rRandPortion);
			scaled rThresh = iU * ((1 - rRandPortion) + fixp(0.5) * rRandPortion);
			if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Utility threshold for direct war plan: %d", rThresh.round());
			if (iDirectU >= rThresh)
			{
				if (gGameRecordLogLevel >= 2 && !isInBackground()) logSASGameRecordUWAIWarPlanDecision(kAgent.getID(), eTarget, SAS_UWAI_WAR_PLAN_DIRECT_UTILITY_THRESHOLD, eWP, eDirectWP, iU, kAgent.AI_getWarPlanStateCounter(eTarget), iTurnsRemaining, NO_TEAM, -1, iDirectU, rThresh.round(), iVictoryDenialBoost);
				bConclude = true;
			}
			if (gUWAIAgentLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("%sirect war plan adopted", (bConclude ? "D" : "No d"));
			if (gWarLogLevel >= 2) logBBAI("WAR_PREPARATION_CONCLUDE_CHECK turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s directWarPlan=%s utility=%d directUtility=%d threshold=%d stateCounter=%d prepTurnsRemaining=%d reason=direct_utility_threshold concluded=%d",
				GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), getSASWarPlanType(eDirectWP),
				iU, iDirectU, rThresh.round(), kAgent.AI_getWarPlanStateCounter(eTarget), iTurnsRemaining, bConclude);
		}
		else if (gWarLogLevel >= 2) logBBAI("WAR_PREPARATION_CONCLUDE_CHECK turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s directWarPlan=%s utility=%d directUtility=%d stateCounter=%d prepTurnsRemaining=%d reason=direct_utility_nonpositive concluded=0",
			GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), getSASWarPlanType(eDirectWP), iU,
			iDirectU, kAgent.AI_getWarPlanStateCounter(eTarget), iTurnsRemaining);
	}
	if (bConclude)
	{
		if (gWarLogLevel >= 1)
		{
			logBBAI("WAR_PLAN_DIRECT_ADOPTED turn=%d background=%d agentTeam=%d targetTeam=%d oldWarPlan=%s newWarPlan=%s utility=%d turnsRemaining=%d forcedPeaceTurns=%d distance=%d targetVictoryCountdown=%d targetPowerPercent=%d",
				GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), getSASWarPlanType(eDirectWP),
				iU, iTurnsRemaining, kAgent.turnsOfForcedPeaceRemaining(eTarget), getSASBBAINearestCityDistance(kAgent.getID(), eTarget),
				GET_TEAM(eTarget).AI_getLowestVictoryCountdown(), getSASBBAITargetPowerPercent(kAgent, eTarget));
			logSASBBAIWarTargetVictoryContext(kAgent, eTarget, "DIRECT_ADOPTED", eDirectWP, iU, -1, -1, -1, isInBackground());
		}
		if (!isInBackground())
		{
			// Don't AI_setWarPlanStateCounter, i.e. let CvTeamAI reset it to 0.
			kAgent.AI_setWarPlan(eTarget, eDirectWP);
			if (gWarLogLevel >= 1)
			{
				logBBAI("WAR_PLAN_DIRECT_SET turn=%d agentTeam=%d targetTeam=%d oldWarPlan=%s newWarPlan=%s utility=%d turnsRemaining=%d forcedPeaceTurns=%d distance=%d targetVictoryCountdown=%d targetPowerPercent=%d stateCounter=%d",
					GC.getGame().getGameTurn(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), getSASWarPlanType(eDirectWP), iU,
					iTurnsRemaining, kAgent.turnsOfForcedPeaceRemaining(eTarget), getSASBBAINearestCityDistance(kAgent.getID(), eTarget),
					GET_TEAM(eTarget).AI_getLowestVictoryCountdown(), getSASBBAITargetPowerPercent(kAgent, eTarget),
					kAgent.AI_getWarPlanStateCounter(eTarget));
				logSASBBAIWarTargetVictoryContext(kAgent, eTarget, "DIRECT_SET", eDirectWP, iU, -1, -1, -1, false);
			}
		}
		return false;
	}
	return true;
}


int UWAI::Team::peaceThreshold(TeamTypes eTarget) const
{
	// Computation is similar to BtS CvPlayerAI::AI_isWillingToTalk
	CvTeamAI const& kAgent = GET_TEAM(m_eAgent);
	if (kAgent.isHuman())
		return 0;
	CvTeamAI const& kTarget = GET_TEAM(eTarget);
	scaled r = fixp(-7.5); // To give it time
	// Give it more time then; also more bitterness I guess.
	if (kAgent.AI_getWarPlan(eTarget) == WARPLAN_TOTAL)
		r -= 5;
	// If they're attacked or att. recent (they could switch to total war eventually)
	if (!kTarget.AI_isChosenWar(kAgent.getID()))
		r -= 5;
	{
		/*	Personality is handled at the player level, but all members
			need to count here. Therefore pass the team's MakePeaceRand. */
		scaled rPrideRating = leaderUWAI().prideRating(kAgent.AI_makePeaceRand());
		// (This puts the addend between -30 and 10)
		r += (1 - rPrideRating) * 40 - 30;
	}
	// <!-- custom: The inherited 2021 UWAI rewrite indexed the target's WarSuccess map with its own team ID, normally reading zero instead of the target's success against the agent.
	// Restore the reciprocal opponent index used before that regression. See KI#508. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	r += scaled::min(15, kAgent.AI_getAtWarCounter(eTarget) +
				(2 * kAgent.AI_getWarSuccess(eTarget) +
				4 * kTarget.AI_getWarSuccess(kAgent.getID())) /
				GC.getWAR_SUCCESS_CITY_CAPTURING());
	int iR = r.round();
	if (!kTarget.isHuman())
	{
		/* Never set the threshold above 0 for inter-AI peace
			(let _them_ sue for peace instead) */
		iR = std::min(0, iR);
	}
	return iR;
}


int UWAI::Team::uJointWar(TeamTypes eTarget, TeamTypes eAlly) const
{
	// Only log about inter-AI war trades
	bool bMuteLog = (GET_TEAM(m_eAgent).isHuman() || GET_TEAM(eAlly).isHuman());
	/*  Joint war against vassal/ master: This function then gets called twice.
		The war evaluation is the same though; so disable logging for one of the calls. */
	if (GET_TEAM(eTarget).isAVassal())
	{
		bMuteLog = true;
		eTarget = GET_TEAM(eTarget).getMasterTeam();
	}
	UWAILogMuteState logMuteState(bMuteLog);
	if (!bMuteLog)
	{
		if (gUWAIAgentLogLevel >= 2 && !logMuteState.isMuted()) logBBAI("Negotiation of joint war");
		if (gUWAIAgentLogLevel >= 2 && !logMuteState.isMuted()) logBBAI("%S is evaluating the utility of %S joining the war"
				" against %S\n", GET_TEAM(m_eAgent).getName().GetCString(),
				GET_TEAM(eAlly).getName().GetCString(), GET_TEAM(eTarget).getName().GetCString());
	}
	WarEvalParameters params(m_eAgent, eTarget, logMuteState);
	params.addWarAlly(eAlly);
	params.setImmediateDoW(true);
	WarPlanTypes eWP = WARPLAN_LIMITED;
	if (GET_TEAM(m_eAgent).isAtWar(eTarget)) // (Currently always guarenteed by caller)
	{
		params.setNotConsideringPeace();
		eWP = NO_WARPLAN; // Evaluate the current plan
	}
	WarEvaluator eval(params);
	int iU = eval.evaluate(eWP);
	/*  Military analysis might conclude that the ally is going to send some ships,
		enough to tip the scales. Highly unlikely to actually happen in the
		first half of the game. */
	if (!GET_TEAM(eAlly).uwai().isLandTarget(eTarget) &&
		GET_TEAM(eAlly).getCurrentEra() < CvEraInfo::AI_getAgeOfExploration())
	{
		iU = std::min(0, iU);
	}
	return iU;
}

int UWAI::Team::tradeValJointWar(TeamTypes eTarget, TeamTypes eAlly) const
{
	PROFILE_FUNC();
	/*  This function could handle a human ally, but the AI isn't supposed to
		pay humans for war (and I've no plans for changing that). */
	FAssert(!GET_TEAM(eAlly).isHuman());
	int iU = uJointWar(eTarget, eAlly); // Compares joint war with solo war
	/*  Low u suggests that we're not sure that we'll need help. Also,
		war evaluation doesn't account for MEMORY_HIRED_WAR_ALLY and
		CvTeam::makeUnwillingToTalk (advc.104o). */
	if (iU < 5 + 9 * scaled::hash(GC.getGame().getGameTurn(),
		GET_TEAM(m_eAgent).getLeaderID()))
	{
		return 0;
	}
	// NB: declareWarTrade applies an additional threshold
	return utilityToTradeVal(std::min(iU, -iWarTradeUtilityThresh)).uround();
}


int UWAI::Team::reluctanceToPeace(TeamTypes eEnemy, bool bNonNegative) const
{
	int iR = -uEndWar(eEnemy) - std::min(0, peaceThreshold(eEnemy));
	if (bNonNegative)
		return std::max(0, iR);
	return iR;
}


bool UWAI::Team::canSchemeAgainst(TeamTypes eTarget, bool bAssumeNoWarPlan, bool bCheckDefensivePacts) const
{
	CvTeamAI const& kAgent = GET_TEAM(m_eAgent);
	if (eTarget == NO_TEAM || eTarget == BARBARIAN_TEAM || eTarget == kAgent.getID())
		return false;
	// Vassals don't scheme
	if (kAgent.isAVassal())
		return false;
	CvTeam const& kTarget = GET_TEAM(eTarget);
	/*  advc.130o: Shouldn't attack right after peace from demand; therefore
		don't plan war during the peace treaty. */
	if (kAgent.isForcePeace(eTarget) &&
		kAgent.AI_getMemoryCount(eTarget, MEMORY_ACCEPT_DEMAND) > 0)
	{
		return false;
	}
	if (!(kTarget.isAlive() && !kTarget.isMinorCiv() && kAgent.isHasMet(eTarget) &&
		!kTarget.isAVassal() && kTarget.getNumCities() > 0 &&
		(bAssumeNoWarPlan || kAgent.AI_getWarPlan(eTarget) == NO_WARPLAN) &&
		kAgent.canEventuallyDeclareWar(eTarget)))
	{
		return false;
	}
	/*	Important not to scheme against a faraway member of a DP b/c that
		may delay our DoW considerably, may even require transports. CvUnitAI
		is only going to target cities of the target team and its vassals.
		The initial DoW matters for diplo penalties, but that's a less important
		consideration. */
	if (bCheckDefensivePacts)
	{
		for (TeamIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itAlly(eTarget);
			itAlly.hasNext(); ++itAlly)
		{
			if (itAlly->isDefensivePact(eTarget) &&
				/*	(Should come up rarely, so the performance of these checks
					shouldn't matter much. Closeness also gets cached, however,
					we may not be in a synchronized context, so we can't
					necessarily benefit from caching.) */
				canSchemeAgainst(itAlly->getID(), bAssumeNoWarPlan, false))
			{
				int iAllyCloseness = kAgent.AI_teamCloseness(itAlly->getID(),
						DEFAULT_PLAYER_CLOSENESS, false, true);
				for (TeamIter<MAJOR_CIV,VASSAL_OF> itVassal(itAlly->getID());
					itVassal.hasNext(); ++itVassal)
				{
					iAllyCloseness = std::max(iAllyCloseness,
							kAgent.AI_teamCloseness(itVassal->getID(),
							DEFAULT_PLAYER_CLOSENESS, false, true));
				}
				int iTargetCloseness = kAgent.AI_teamCloseness(eTarget,
						DEFAULT_PLAYER_CLOSENESS, false, true);
				for (TeamIter<MAJOR_CIV,VASSAL_OF> itVassal(eTarget);
					itVassal.hasNext(); ++itVassal)
				{
					iTargetCloseness = std::max(iTargetCloseness,
							kAgent.AI_teamCloseness(itVassal->getID(),
							DEFAULT_PLAYER_CLOSENESS, false, true));
				}
				if (2 * iAllyCloseness > 3 * iTargetCloseness)
					return false;
			}
		}
	}
	return true;
}


scaled UWAI::Team::limitedWarWeight() const
{
	int const iLimitedWarRand = GET_TEAM(m_eAgent).AI_limitedWarRand();
	if (iLimitedWarRand <= 0)
		return 0;
	/*  The higher rExp, the greater the impact of personal preference for
		limited or total war. */
	scaled const rExp = fixp(0.75);
	// Bias for total war b/c the WarRand values are biased toward limited war
	scaled const rBase = fixp(0.8) * GET_TEAM(m_eAgent).AI_maxWarRand() / iLimitedWarRand;
	FAssert(rBase >= 0);
	return rBase.pow(rExp);
}


namespace
{
	struct TargetData
	{
		// <!-- custom: Preserve original/boosted utility and victory-denial flags after sorting.
		// Later selection needs them to reduce avoid-war hesitation, avoid local-target vetoes, choose direct vs preparation war, and retain the already-evaluated naval/preparation context for compact selected-target logging. See KI#184. (GPT-5.5 + ChatGPT-5.6-Sol) -->
		TargetData(scaled rDrive, TeamTypes eTeam, bool bTotal, bool bDirect, bool bNaval, int iPreparationTime, int iOriginalU, int iU, int iVictoryDenialBoost, int iTargetMaxVictoryStage, bool bShortWork)
		:	rDrive(rDrive), eTeam(eTeam), bTotal(bTotal), bDirect(bDirect), bNaval(bNaval), iPreparationTime(iPreparationTime), iOriginalU(iOriginalU), iU(iU), iVictoryDenialBoost(iVictoryDenialBoost), iTargetMaxVictoryStage(iTargetMaxVictoryStage), bShortWork(bShortWork)
		{}
		bool operator<(TargetData const& kOther) { return rDrive < kOther.rDrive; }
		scaled rDrive;
		TeamTypes eTeam;
		bool bTotal;
		bool bDirect;
		bool bNaval;
		int iPreparationTime;
		int iOriginalU;
		int iU;
		int iVictoryDenialBoost;
		int iTargetMaxVictoryStage;
		bool bShortWork;
	};
}

// <!-- custom: Added aeChangedTargets so a canceled or redirected preparation cannot be recreated against the same target later in this team turn; other targets remain eligible. (GPT-5.6-Sol) -->
void UWAI::Team::scheme(set<TeamTypes> const& aeChangedTargets)
{
	CvTeamAI& kAgent = GET_TEAM(m_eAgent);
	if (kAgent.AI_countWarPlans() > kAgent.getNumWars(true, true))
	{
		// <!-- custom: Save-file 449 ended with India launching on turn 299 while the strongest rival continued preparing an unrelated Ottoman war and did not evaluate a new anti-Space war before India won on turn 311.
		// When an existing non-war plan blocks all new scheming, identify each stage-3+, countdown, or launched-victory threat and the plan occupying the single slot.
		// Include direct plans because a preparation can become one without starting a war. This is diagnostic only and is gated by War logging. (GPT-5.6-Sol) -->
		if (gWarLogLevel >= 1)
		{
			TeamTypes eBlockingTarget = NO_TEAM;
			WarPlanTypes eBlockingPlan = NO_WARPLAN;
			for (TeamAIIter<CIV_ALIVE> itLoop; itLoop.hasNext(); ++itLoop)
			{
				WarPlanTypes const eLoopPlan = kAgent.AI_getWarPlan(itLoop->getID());
				if (eLoopPlan == NO_WARPLAN || kAgent.isAtWar(itLoop->getID())) continue;
				eBlockingTarget = itLoop->getID();
				eBlockingPlan = eLoopPlan;
				break;
			}
			for (TeamIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itThreat(kAgent.getID()); itThreat.hasNext(); ++itThreat)
			{
				TeamTypes const eThreat = itThreat->getID();
				CvTeamAI const& kThreat = GET_TEAM(eThreat);
				int const iThreatStage = getSASTeamMaxVictoryStage(eThreat);
				int const iThreatCountdown = kThreat.AI_getLowestVictoryCountdown();
				if (iThreatStage < 3 && iThreatCountdown < 0) continue;
				int const iNearestCityDistance = getSASBBAINearestCityDistance(kAgent.getID(), eThreat);
				logBBAI("WAR_TARGET_VICTORY_DENIAL_SCHEME_BLOCKED turn=%d background=%d agentTeam=%d threatTeam=%d threatMaxVictoryStage=%d threatVictoryCountdown=%d threatSpaceshipParts=%d threatSpaceshipPartsPercent=%d blockingTargetTeam=%d blockingWarPlan=%s blockingStateCounter=%d warPlanCount=%d wars=%d canSchemeAssumingNoPlan=%d canEventuallyDeclare=%d canDeclareNow=%d forcedPeaceTurns=%d nearestCityDistance=%d targetPowerPercent=%d mutualLandTarget=%d directAllowedAsLand=%d directAllowedAsNaval=%d attitude=%d attitudeValue=%d",
					GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eThreat, iThreatStage, iThreatCountdown,
					getSASTeamSpaceshipPartsBuilt(eThreat), getSASTeamSpaceshipPartsPercent(eThreat), eBlockingTarget,
					getSASWarPlanType(eBlockingPlan), (eBlockingTarget == NO_TEAM ? -1 : kAgent.AI_getWarPlanStateCounter(eBlockingTarget)),
					kAgent.AI_countWarPlans(), kAgent.getNumWars(true, true), canSchemeAgainst(eThreat, true, false),
					kAgent.canEventuallyDeclareWar(eThreat), kAgent.canDeclareWar(eThreat), kAgent.turnsOfForcedPeaceRemaining(eThreat),
					iNearestCityDistance, getSASBBAITargetPowerPercent(kAgent, eThreat),
					(kAgent.AI_isLandTarget(eThreat) && kThreat.AI_isLandTarget(kAgent.getID())),
					isSASVictoryDenialDirectWarAllowed(eThreat, iThreatStage, false, iNearestCityDistance),
					isSASVictoryDenialDirectWarAllowed(eThreat, iThreatStage, true, iNearestCityDistance), kAgent.AI_getAttitude(eThreat),
					kAgent.AI_getAttitudeVal(eThreat));
			}
		}
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("No scheming b/c already a war in preparation");
		return;
	}
	for (TeamAIIter<CIV_ALIVE> itMinor; itMinor.hasNext(); ++itMinor)
	{
		if (!itMinor->isMinorCiv())
			continue;
		int iCloseness = kAgent.AI_teamCloseness(itMinor->getID());
		if (iCloseness >= 40 && itMinor->getPower(false) * 3 > kAgent.getPower(true) &&
			itMinor->AI_isLandTarget(kAgent.getID()) &&
			kAgent.AI_isLandTarget(itMinor->getID()))
		{
			if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("No scheming b/c busy fighting minor civ %S at closeness %d",
					GET_TEAM(itMinor->getID()).getName().GetCString(), iCloseness);
			return;
		}
	}
	vector<TargetData> aTargets;
	scaled rTotalDrive;
	UWAICache& kCache = leaderCache();
	for (TeamIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itTarget(kAgent.getID());
		itTarget.hasNext(); ++itTarget)
	{
		TeamTypes const eTarget = itTarget->getID();
		// <!-- custom: Save-file 452 still had four cases where review canceled a plan and scheme selected the same target again later in the same team turn because evaluating an existing plan and starting it anew produced contradictory utilities. Wait for the next turn before reconsidering that target, without blocking other targets. See KI#189. (GPT-5.6-Sol) -->
		if (aeChangedTargets.count(eTarget) > 0)
		{
			kCache.setCanBeHiredAgainst(eTarget, false);
			if (gWarLogLevel >= 1) logBBAI("WAR_TARGET_SAME_TURN_RECONSIDERATION_BLOCKED turn=%d background=%d agentTeam=%d targetTeam=%d currentWarPlan=%s",
					GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(kAgent.AI_getWarPlan(eTarget)));
			continue;
		}
		bool const bCanSchemeIgnoringPlanAndDP = canSchemeAgainst(eTarget, true, false);
		bool const bCanSchemeNow = canSchemeAgainst(eTarget, false);
		if (gWarLogLevel >= 3) logBBAI("WAR_TARGET_SCHEME_GATE turn=%d background=%d agentTeam=%d targetTeam=%d canSchemeIgnoringPlanAndDP=%d canSchemeNow=%d existingWarPlan=%s canEventuallyDeclare=%d canDeclareNow=%d forcedPeaceTurns=%d avoidWar=%d attitude=%d attitudeValue=%d closeness=%d nearestCityDistance=%d targetPowerPercent=%d ourPower=%d targetPower=%d ourCities=%d targetCities=%d targetVassal=%d",
			GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, bCanSchemeIgnoringPlanAndDP, bCanSchemeNow,
			getSASWarPlanType(kAgent.AI_getWarPlan(eTarget)), kAgent.canEventuallyDeclareWar(eTarget), kAgent.canDeclareWar(eTarget),
			kAgent.turnsOfForcedPeaceRemaining(eTarget), kAgent.AI_isAvoidWar(eTarget, true), kAgent.AI_getAttitude(eTarget),
			kAgent.AI_getAttitudeVal(eTarget), kAgent.AI_teamCloseness(eTarget), getSASBBAINearestCityDistance(kAgent.getID(), eTarget),
			getSASBBAITargetPowerPercent(kAgent, eTarget), kAgent.getPower(true), GET_TEAM(eTarget).getDefensivePower(kAgent.getID()),
			kAgent.getNumCities(), GET_TEAM(eTarget).getNumCities(), GET_TEAM(eTarget).isAVassal());
		if (!bCanSchemeIgnoringPlanAndDP) kCache.setCanBeHiredAgainst(eTarget, false);
		if (!bCanSchemeNow) continue;
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Scheming against %S", GET_TEAM(eTarget).getName().GetCString());
		bool bShortWork = kAgent.AI_isPushover(eTarget);
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted() && bShortWork) logBBAI("Target assumed to be short work");
		bool bSkipTotal = (kAgent.AI_isAnyWarPlan() || bShortWork);
		/*  Skip scheming entirely if already in a total war? Probably too
			restrictive in the lategame. Perhaps have reviewWarPlans compute the
			smallest utility among current war plans, and skip scheming if that
			minimum is, say, -55 or less. */
		m_kLogMuteState.pushMute();
		WarEvalParameters params(kAgent.getID(), eTarget, m_kLogMuteState);
		WarEvaluator eval(params);
		int iTotalU = INT_MIN;
		bool bTotalNaval = false;
		int iTotalPrepTime = -1;
		if (!bSkipTotal)
		{
			iTotalU = eval.evaluate(WARPLAN_PREPARING_TOTAL);
			bTotalNaval = params.isNaval();
			iTotalPrepTime = params.getPreparationTime();
		}
		int const iLimitedU = eval.evaluate(WARPLAN_PREPARING_LIMITED, bShortWork ? 0 : -1);
		bool const bLimitedNaval = params.isNaval();
		int const iLimitedPrepTime = params.getPreparationTime();
		bool bTotal = false;
		// <!-- custom: A neutral limited-war evaluation must not borrow a positive total-war utility and then install a limited plan.
		// Include the zero boundary while preserving personality weighting when both choices are positive. See KI#512. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if (iLimitedU <= 0 && iTotalU > 0)
			bTotal = true;
		if (iLimitedU < 0 && iTotalU < 0) // Only relevant for logging
			bTotal = (iTotalU > iLimitedU);
		if (iLimitedU > 0 && iTotalU > 0)
		{
			scaled const rLimitedWarWeight = limitedWarWeight();
			if (rLimitedWarWeight != 1)
			{
				if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Bias for/against limited war: %d percent",
						rLimitedWarWeight.uround());
			}
			int const iPadding = SyncRandNum(40);
			bTotal = (iTotalU + iPadding > (iPadding + iLimitedU) * rLimitedWarWeight);
		}
		int iU = std::max(iLimitedU, iTotalU);
		// <!-- custom: If a rival is close to winning, ordinary UWAI reluctance can leave the game passive until the victory fires. Save-file 450 BBAI testing showed Egypt/Ramesses winning Space Race at turn 290 while multiple rivals evaluated the threat every turn but never selected a real war plan.
		// Boost all imminent victory types, not only Space Race; short countdowns can also skip preparation when the target is close and not too strong. (GPT-5.5) -->
		int const iOriginalU = iU;
		int const iTargetMaxVictoryStage = getSASTeamMaxVictoryStage(eTarget);
		int const iVictoryDenialBoost = getSASBBAIVictoryDenialUtilityBoost(eTarget, iTargetMaxVictoryStage);
		int const iNearestCityDistance = (iVictoryDenialBoost > 0 ? getSASBBAINearestCityDistance(kAgent.getID(), eTarget) : -1);
		// <!-- custom: KI#184 marked qualifying victory-denial targets as direct even during force peace, then installed a LIMITED/TOTAL plan when its guarded declaration failed.
		// Keep the urgency boost and treaty-time target selection, but use a direct plan only when war can actually be declared now; otherwise the target correctly enters preparation. See KI#515. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		bool const bVictoryDenialDirect = (iVictoryDenialBoost > 0 && kAgent.canDeclareWar(eTarget) && isSASVictoryDenialDirectWarAllowed(eTarget, iTargetMaxVictoryStage, bTotal ? bTotalNaval : bLimitedNaval, iNearestCityDistance));
		if (iVictoryDenialBoost > 0)
		{
			iU += iVictoryDenialBoost;
			if (gWarLogLevel >= 1) logBBAI("WAR_TARGET_VICTORY_DENIAL_ADJUST turn=%d agentTeam=%d targetTeam=%d originalUtility=%d adjustedUtility=%d boost=%d direct=%d targetMaxVictoryStage=%d targetVictoryCountdown=%d targetSpaceshipParts=%d targetSpaceshipPartsPercent=%d targetSpaceLeaderPartGap=%d attitude=%d attitudeValue=%d closeness=%d nearestCityDistance=%d targetPowerPercent=%d",
				GC.getGame().getGameTurn(), kAgent.getID(), eTarget, iOriginalU, iU, iVictoryDenialBoost, bVictoryDenialDirect,
				iTargetMaxVictoryStage, GET_TEAM(eTarget).AI_getLowestVictoryCountdown(), getSASTeamSpaceshipPartsBuilt(eTarget),
				getSASTeamSpaceshipPartsPercent(eTarget), getSASTeamStage3SpaceLeaderPartGap(eTarget), kAgent.AI_getAttitude(eTarget),
				kAgent.AI_getAttitudeVal(eTarget), kAgent.AI_teamCloseness(eTarget), iNearestCityDistance,
				getSASBBAITargetPowerPercent(kAgent, eTarget));
		}
		m_kLogMuteState.popMute();
		// <!-- custom: Base AdvCiv used hidden utility thresholds before repeating the chosen evaluation solely for report detail.
		// SAS uses explicit UWAI subsystem levels instead: level 1 stays compact, while level 2+ requests an observation-only selected-scenario rerun that bypasses WarEvaluator caches and cannot feed gameplay utility. (ChatGPT-5.6-Sol) -->
		bool const bDetailedLog = (!m_kLogMuteState.isMuted() &&
			(gUWAIWarUtilityLogLevel >= 2 || gUWAIMilitaryAnalystLogLevel >= 2 ||
			gUWAIInvasionGraphLogLevel >= 2 || gUWAIArmamentForecastLogLevel >= 2));
		if (bDetailedLog)
		{
			if (bTotal) eval.evaluateForDiagnostics(WARPLAN_PREPARING_TOTAL, bTotalNaval, iTotalPrepTime);
			else eval.evaluateForDiagnostics(WARPLAN_PREPARING_LIMITED, bLimitedNaval, iLimitedPrepTime);
		}
		else
		{
			if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("%s %s war has %d utility", bTotal ? "total" : "limited",
					((bTotal && bTotalNaval) || (!bTotal && bLimitedNaval)) ?
					"naval" : "", iU);
		}
		if (gWarLogLevel >= 2) logSASBBAIWarTargetEval(kAgent, eTarget, bTotal ? WARPLAN_PREPARING_TOTAL : WARPLAN_PREPARING_LIMITED, iU, iLimitedU, iTotalU, bLimitedNaval, bTotalNaval, iLimitedPrepTime, iTotalPrepTime, bShortWork, isInBackground());
		bool const bCanHireOld = kCache.canBeHiredAgainst(eTarget);
		kCache.updateCanBeHiredAgainst(eTarget, iU, iWarTradeUtilityThresh);
		bool const bCanHireNew = kCache.canBeHiredAgainst(eTarget);
		if (bCanHireOld != bCanHireNew && (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()))
		{
			if (bCanHireNew)
				logBBAI("Can now (possibly) be hired for war");
			else logBBAI("Can no longer be hired for war");
		}
		if (iU <= 0)
			continue;
		scaled rDrive = iU;
		/*  WarRand of e.g. Alexander and Montezuma 50 for total war, else 40;
			Gandhi and Mansa Musa 400 for total, else 200.
			I.e. peaceful leaders hesitate longer before starting war preparations;
			warlike leaders have more drive.
			(Could easily use DogpileWarRand here when the target is already
			in a war, but I think I've found a better use for DogpileWarRand
			in warConfidenceAllies. Less dogpiling on weak targets that way.) */
		scaled rDiv = (bTotal ? kAgent.AI_maxWarRand() : kAgent.AI_limitedWarRand());
		FAssert(rDiv >= 0);
		// Let's make the AI a bit less patient
		// Especially the peaceful types; this maps e.g. 400 to 296 and 40 to 33.
		//rDiv.exponentiate(fixp(0.95));
		/*	Not a good idea after all. Overall, UWAI tends to make peaceful leaders
			rather too utilitarian with their war plans, I've come to think.
			So let's adjust only linearly. */
		rDiv *= fixp(0.85);
		if (rDiv <= 0)
			rDrive = 0;
		else rDrive /= rDiv;
		/*  Delay preparations probabilistically (by lowering drive) when there's
			still a peace treaty */
		scaled rPeacePortionRemaining(kAgent.turnsOfForcedPeaceRemaining(eTarget),
				// +1.0 b/c I don't want 0 drive at this point
				GC.getDefineINT(CvGlobals::PEACE_TREATY_LENGTH) + 1);
		rPeacePortionRemaining.decreaseTo(fixp(0.95));
		// (Let's try it w/o exponentiation)
		rDrive *= (1 - rPeacePortionRemaining)/*.pow(fixp(1.5))*/;
		aTargets.push_back(TargetData(rDrive, eTarget, bTotal, bVictoryDenialDirect, bTotal ? bTotalNaval : bLimitedNaval, bTotal ? iTotalPrepTime : iLimitedPrepTime, iOriginalU, iU, iVictoryDenialBoost, iTargetMaxVictoryStage, bShortWork));
		rTotalDrive += rDrive;
	}
	// Descending by drive
	std::sort(aTargets.rbegin(), aTargets.rend());
	vector<scaled> aAdjustedDrives;
	aAdjustedDrives.reserve(aTargets.size());
	TeamTypes ePreferredLocalTarget = NO_TEAM;
	scaled rPreferredLocalDrive;
	int iPreferredLocalRank = -1;
	// <!-- custom: Reuse the same adjusted drive that the normal target loop uses after AI_isAvoidWar hesitation, then pick at most one close/weak/disliked land-war target for the narrow faraway-target guard. This prepass does not itself start a war; it only identifies the local target that can block clearly farther alternatives below. See KI#182. (GPT-5.5) -->
	for (size_t i = 0; i < aTargets.size(); i++)
	{
		TeamTypes const eTarget = aTargets[i].eTeam;
		scaled rDrive = aTargets[i].rDrive;
		// Conscientious hesitation
		if (kAgent.AI_isAvoidWar(eTarget, true))
		{
			// <!-- custom: Avoid-war personality hesitation is useful for ordinary wars, but save-file 450 showed it could prevent any response to an imminent Space win. Keep the hesitation tunable for victory-denial targets instead of removing it entirely. See KI#184. (GPT-5.5) -->
			// rDrive -= rTotalDrive / 2;
			scaled rAvoidWarHesitation = rTotalDrive / 2;
			if (aTargets[i].iVictoryDenialBoost > 0)
			{
				static const int iVictoryDenialAvoidWarPercent = GC.getDefineINT("SAS_UWAI_VICTORY_DENIAL_AVOID_WAR_HESITATION_PERCENT");
				rAvoidWarHesitation *= iVictoryDenialAvoidWarPercent;
				rAvoidWarHesitation /= 100;
			}
			rDrive -= rAvoidWarHesitation;
		}
		aAdjustedDrives.push_back(rDrive);
		if (rDrive > 0 && isSASBBAIPreferredLocalWarTarget(kAgent, eTarget, rDrive) && isSASBBAIBetterPreferredLocalWarTarget(kAgent, eTarget, rDrive, ePreferredLocalTarget, rPreferredLocalDrive))
		{
			ePreferredLocalTarget = eTarget;
			rPreferredLocalDrive = rDrive;
			iPreferredLocalRank = (int)i + 1;
		}
	}
	if (gWarLogLevel >= 1 && ePreferredLocalTarget != NO_TEAM) logBBAI("WAR_TARGET_LOCAL_PREFERRED turn=%d agentTeam=%d preferredTargetTeam=%d preferredDrivePercent=%d preferredRank=%d candidateCount=%d attitude=%d attitudeValue=%d closeness=%d nearestCityDistance=%d targetPowerPercent=%d",
		GC.getGame().getGameTurn(), kAgent.getID(), ePreferredLocalTarget, rPreferredLocalDrive.getPercent(), iPreferredLocalRank,
		(int)aTargets.size(), kAgent.AI_getAttitude(ePreferredLocalTarget), kAgent.AI_getAttitudeVal(ePreferredLocalTarget),
		kAgent.AI_teamCloseness(ePreferredLocalTarget), getSASBBAINearestCityDistance(kAgent.getID(), ePreferredLocalTarget),
		getSASBBAITargetPowerPercent(kAgent, ePreferredLocalTarget));
	// <!-- custom: BBAI testing showed China reject a nearby target with 68% final drive and then select a distant third-ranked target with only 5% drive. The inherited independent rolls mix the chance to prepare any war with target choice.
	// When enabled, identify the highest final drive after all eligibility/local-target guards, roll only that rival below, and begin no preparation if it fails. This preserves uncertain timing without randomly substituting a worse target. (GPT-5.6-Sol) -->
	static bool const bOnlyRollBestEligibleTarget = GC.getDefineBOOL("SAS_UWAI_ONLY_ROLL_BEST_ELIGIBLE_WAR_TARGET_ENABLE");
	TeamTypes eBestEligibleTarget = NO_TEAM;
	scaled rBestEligibleDrive;
	int iBestEligibleTargetIndex = -1;
	if (bOnlyRollBestEligibleTarget)
	{
		for (size_t i = 0; i < aTargets.size(); i++)
		{
			TeamTypes const eTarget = aTargets[i].eTeam;
			scaled const rDrive = aAdjustedDrives[i];
			if (rDrive <= 0 || (aTargets[i].iVictoryDenialBoost <= 0 && shouldSASBBAISkipForPreferredLocalWarTarget(kAgent, eTarget, ePreferredLocalTarget)))
				continue;
			if (iBestEligibleTargetIndex < 0 || rDrive > rBestEligibleDrive)
			{
				eBestEligibleTarget = eTarget;
				rBestEligibleDrive = rDrive;
				iBestEligibleTargetIndex = (int)i;
			}
		}
	}
	int iEligibleRank = 0;
	int iHigherRankRollFailures = 0;
	for (size_t i = 0; i < aTargets.size(); i++)
	{
		TeamTypes const eTarget = aTargets[i].eTeam;
		scaled rDrive = aAdjustedDrives[i];
		if (rDrive <= 0)
			continue;
		// <!-- custom: The preferred-local-target guard fixes ordinary faraway-war blunders, but must not veto emergency victory denial. A close-to-win target can be strategically mandatory even if a cleaner local conquest target exists. See KI#184. (GPT-5.5) -->
		if (aTargets[i].iVictoryDenialBoost <= 0 && shouldSASBBAISkipForPreferredLocalWarTarget(kAgent, eTarget, ePreferredLocalTarget))
		{
			// <!-- custom: Only block targets that are clearly farther than the preferred local land target. Equal-distance and closer targets still use ordinary UWAI, keeping this a narrow faraway-war blunder fix rather than a broad war-target rewrite. (GPT-5.5) -->
			if (gWarLogLevel >= 1) logBBAI("WAR_TARGET_LOCAL_PREFERRED_SKIP turn=%d agentTeam=%d skippedTargetTeam=%d preferredTargetTeam=%d skippedDrivePercent=%d preferredDrivePercent=%d skippedRank=%d preferredRank=%d candidateCount=%d skippedAttitude=%d skippedAttitudeValue=%d preferredAttitude=%d preferredAttitudeValue=%d skippedCloseness=%d preferredCloseness=%d skippedDistance=%d preferredDistance=%d skippedTargetPowerPercent=%d preferredTargetPowerPercent=%d",
				GC.getGame().getGameTurn(), kAgent.getID(), eTarget, ePreferredLocalTarget, rDrive.getPercent(),
				rPreferredLocalDrive.getPercent(), (int)i + 1, iPreferredLocalRank, (int)aTargets.size(), kAgent.AI_getAttitude(eTarget),
				kAgent.AI_getAttitudeVal(eTarget), kAgent.AI_getAttitude(ePreferredLocalTarget),
				kAgent.AI_getAttitudeVal(ePreferredLocalTarget), kAgent.AI_teamCloseness(eTarget),
				kAgent.AI_teamCloseness(ePreferredLocalTarget), getSASBBAINearestCityDistance(kAgent.getID(), eTarget),
				getSASBBAINearestCityDistance(kAgent.getID(), ePreferredLocalTarget), getSASBBAITargetPowerPercent(kAgent, eTarget),
				getSASBBAITargetPowerPercent(kAgent, ePreferredLocalTarget));
			continue;
		}
		iEligibleRank++;
		if (eBestEligibleTarget == NO_TEAM)
		{
			eBestEligibleTarget = eTarget;
			rBestEligibleDrive = rDrive;
		}
		// <!-- custom: Victory-denial direct-war candidates have already passed declaration, power, distance and naval gates.
		// Convert only those from preparation to immediate limited/total war so ordinary UWAI preparation behavior stays intact. See KI#184 and KI#515. (GPT-5.5 + ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		WarPlanTypes const eWP = (aTargets[i].bTotal ? (aTargets[i].bDirect ? WARPLAN_TOTAL : WARPLAN_PREPARING_TOTAL) : (aTargets[i].bDirect ? WARPLAN_LIMITED : WARPLAN_PREPARING_LIMITED));
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Drive for %s against %S: %d percent", aTargets[i].bDirect ? "direct war" : "war preparations", GET_TEAM(eTarget).getName().GetCString(), rDrive.getPercent());
		if (gWarLogLevel >= 2) logSASBBAIWarTargetDrive(kAgent, eTarget, eWP, aTargets[i].iU, rDrive, aTargets[i].bShortWork, isInBackground());
		// <!-- custom: Keep diagnostics for every eligible rival, but when KI#191 is enabled, only the highest final-drive candidate reaches the roll. This prevents a failed best-target roll from falling through to a rival the AI rated worse. (GPT-5.6-Sol) -->
		if (bOnlyRollBestEligibleTarget && (int)i != iBestEligibleTargetIndex)
			continue;
		// <!-- custom: Log the exact eligible order and best-target comparison. With the new rule enabled, only the highest final-drive candidate reaches this roll; disabling it restores inherited independent rolls and possible fall-through to lower-ranked targets. (GPT-5.6-Sol) -->
		bool const bSelected = SyncRandSuccess(rDrive);
		if (gWarLogLevel >= 2) logBBAI("WAR_TARGET_SELECTION_ROLL turn=%d background=%d bestOnly=%d agentTeam=%d targetTeam=%d warPlan=%s utility=%d drivePercent=%d selected=%d targetRank=%d eligibleRank=%d candidateCount=%d higherRankRollFailures=%d bestEligibleTargetTeam=%d bestEligibleDrivePercent=%d candidateDistance=%d bestEligibleDistance=%d candidateTargetPowerPercent=%d bestEligibleTargetPowerPercent=%d candidateAttitude=%d candidateAttitudeValue=%d bestEligibleAttitude=%d bestEligibleAttitudeValue=%d",
			GC.getGame().getGameTurn(), isInBackground(), bOnlyRollBestEligibleTarget, kAgent.getID(), eTarget, getSASWarPlanType(eWP),
			aTargets[i].iU, rDrive.getPercent(), bSelected, (int)i + 1, iEligibleRank, (int)aTargets.size(), iHigherRankRollFailures,
			eBestEligibleTarget, rBestEligibleDrive.getPercent(), getSASBBAINearestCityDistance(kAgent.getID(), eTarget),
			getSASBBAINearestCityDistance(kAgent.getID(), eBestEligibleTarget), getSASBBAITargetPowerPercent(kAgent, eTarget),
			getSASBBAITargetPowerPercent(kAgent, eBestEligibleTarget), kAgent.AI_getAttitude(eTarget), kAgent.AI_getAttitudeVal(eTarget),
			kAgent.AI_getAttitude(eBestEligibleTarget), kAgent.AI_getAttitudeVal(eBestEligibleTarget));
		// <!-- custom: SASGameRecord keeps only the real foreground target decision, not the full UWAI candidate/evaluation trace.
		// A successful roll is preserved immediately before its plan/declaration mutation.
		// At level 3, the single best-only failed roll is also retained as compact evidence that war was considered but deferred.
		// Check the recorder level before the background flag so disabled logging adds no logging-only foreground test. (ChatGPT-5.6-Sol) -->
		if ((bSelected || bOnlyRollBestEligibleTarget) && gGameRecordLogLevel >= (bSelected ? 2 : 3) && !isInBackground()) logSASGameRecord("GAME_RECORD_AI_WAR_TARGET_CHOICE turn=%d outcome=%s agentTeam=%d targetTeam=%d warPlan=%s utility=%d originalUtility=%d victoryDenialBoost=%d direct=%d naval=%d evaluatedPrepTurns=%d forcedPeaceTurns=%d targetMaxVictoryStage=%d targetVictoryCountdown=%d drivePercent=%d shortWork=%d targetRank=%d eligibleRank=%d candidateCount=%d bestOnly=%d attitude=%d attitudeValue=%d closeness=%d nearestCityDistance=%d targetPowerPercent=%d",
			GC.getGame().getGameTurn(), bSelected ? "SELECTED" : "DEFERRED_ROLL", kAgent.getID(), eTarget, getSASWarPlanType(eWP),
			aTargets[i].iU, aTargets[i].iOriginalU, aTargets[i].iVictoryDenialBoost, aTargets[i].bDirect, aTargets[i].bNaval,
			aTargets[i].iPreparationTime, kAgent.turnsOfForcedPeaceRemaining(eTarget), aTargets[i].iTargetMaxVictoryStage,
			GET_TEAM(eTarget).AI_getLowestVictoryCountdown(), rDrive.getPercent(), aTargets[i].bShortWork, (int)i + 1, iEligibleRank,
			(int)aTargets.size(), bOnlyRollBestEligibleTarget, kAgent.AI_getAttitude(eTarget), kAgent.AI_getAttitudeVal(eTarget),
			kAgent.AI_teamCloseness(eTarget), getSASBBAINearestCityDistance(kAgent.getID(), eTarget),
			getSASBBAITargetPowerPercent(kAgent, eTarget));
		if (bSelected)
		{
			if (gWarLogLevel >= 1)
			{
				// <!-- custom: CHOSEN can be emitted during UWAI background evaluation, where no real war plan is assigned. Keep it for probabilistic target-choice context, but log PLAN_SET below only after the non-background AI_setWarPlan call so victory-pressure audits can distinguish simulated choice from actual action. (GPT-5.5) -->
				logBBAI("WAR_TARGET_CHOSEN turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s utility=%d originalUtility=%d victoryDenialBoost=%d direct=%d targetMaxVictoryStage=%d drivePercent=%d shortWork=%d targetRank=%d candidateCount=%d attitude=%d attitudeValue=%d closeness=%d nearestCityDistance=%d targetPowerPercent=%d",
					GC.getGame().getGameTurn(), isInBackground(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), aTargets[i].iU,
					aTargets[i].iOriginalU, aTargets[i].iVictoryDenialBoost, aTargets[i].bDirect, aTargets[i].iTargetMaxVictoryStage,
					rDrive.getPercent(), aTargets[i].bShortWork, (int)i + 1, (int)aTargets.size(), kAgent.AI_getAttitude(eTarget),
					kAgent.AI_getAttitudeVal(eTarget), kAgent.AI_teamCloseness(eTarget),
					getSASBBAINearestCityDistance(kAgent.getID(), eTarget), getSASBBAITargetPowerPercent(kAgent, eTarget));
				logSASBBAIWarTargetVictoryContext(kAgent, eTarget, "CHOSEN", eWP, aTargets[i].iU, rDrive.getPercent(), (int)i + 1, (int)aTargets.size(), isInBackground());
			}
			if (!isInBackground())
			{
				// <!-- custom: `AI_setWarPlan(WARPLAN_LIMITED/TOTAL)` was not sufficient in the Lincoln retests; the near-finished spaceship target could still win before a real declaration happened. Declare immediately for the narrow victory-denial direct-war case. See KI#184. (GPT-5.5) -->
				if (aTargets[i].bDirect && aTargets[i].iVictoryDenialBoost > 0 && kAgent.canDeclareWar(eTarget))
				{
					kAgent.declareWar(eTarget, false, eWP);
					if (gWarLogLevel >= 1)
					{
						logBBAI("WAR_TARGET_VICTORY_DENIAL_DECLARE turn=%d agentTeam=%d targetTeam=%d warPlan=%s utility=%d originalUtility=%d victoryDenialBoost=%d targetMaxVictoryStage=%d targetVictoryCountdown=%d targetPowerPercent=%d nearestCityDistance=%d",
							GC.getGame().getGameTurn(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), aTargets[i].iU,
							aTargets[i].iOriginalU, aTargets[i].iVictoryDenialBoost, aTargets[i].iTargetMaxVictoryStage,
							GET_TEAM(eTarget).AI_getLowestVictoryCountdown(), getSASBBAITargetPowerPercent(kAgent, eTarget),
							getSASBBAINearestCityDistance(kAgent.getID(), eTarget));
						logSASBBAIWarTargetVictoryContext(kAgent, eTarget, "DECLARE", eWP, aTargets[i].iU, rDrive.getPercent(), (int)i + 1, (int)aTargets.size(), false);
					}
				}
				else kAgent.AI_setWarPlan(eTarget, eWP);
				if (gWarLogLevel >= 1)
				{
					logBBAI("WAR_TARGET_PLAN_SET turn=%d agentTeam=%d targetTeam=%d warPlan=%s utility=%d originalUtility=%d victoryDenialBoost=%d direct=%d targetMaxVictoryStage=%d drivePercent=%d targetRank=%d candidateCount=%d stateCounter=%d targetVictoryCountdown=%d targetPowerPercent=%d nearestCityDistance=%d",
						GC.getGame().getGameTurn(), kAgent.getID(), eTarget, getSASWarPlanType(eWP), aTargets[i].iU, aTargets[i].iOriginalU,
						aTargets[i].iVictoryDenialBoost, aTargets[i].bDirect, aTargets[i].iTargetMaxVictoryStage, rDrive.getPercent(),
						(int)i + 1, (int)aTargets.size(), kAgent.AI_getWarPlanStateCounter(eTarget),
						GET_TEAM(eTarget).AI_getLowestVictoryCountdown(), getSASBBAITargetPowerPercent(kAgent, eTarget),
						getSASBBAINearestCityDistance(kAgent.getID(), eTarget));
					logSASBBAIWarTargetVictoryContext(kAgent, eTarget, "PLAN_SET", eWP, aTargets[i].iU, rDrive.getPercent(), (int)i + 1, (int)aTargets.size(), false);
				}
				// <!-- custom: Direct victory-denial wars are already declared, so the preparation-started message would be misleading. Keep it only for actual preparation plans. See KI#184. (GPT-5.5) -->
				if (!aTargets[i].bDirect)
					showWarPrepStartedMsg(eTarget);
			}
			if (gUWAIAgentLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("War plan initiated (%s)", getSASWarPlanType(eWP));
			break; // Prepare only one war at a time
		}
		iHigherRankRollFailures++;
		if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("No preparations begun this turn");
		if (GET_TEAM(eTarget).isHuman() && aTargets[i].iU <= 23)
		{
			PlayerTypes eTargetPlayer = GET_TEAM(eTarget).getRandomMemberAlive(true);
			CvPlayerAI& kAgentPlayer = GET_PLAYER(kAgent.getRandomMemberAlive(false));
			if (kAgentPlayer.canContact(eTargetPlayer, true))
			{
				if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Trying to amend tensions with human %S",
						GET_TEAM(eTarget).getName().GetCString());
				if (!isInBackground())
				{
					bool const bAmendedTensions = kAgentPlayer.uwai().amendTensions(eTargetPlayer);
					if (gUWAIAgentLogLevel >= 1 && !m_kLogMuteState.isMuted())
					{
						if (bAmendedTensions)
							logBBAI("Diplo message sent");
						else logBBAI("No diplo message sent");
					}
				}
			}
			else
			{
				if (gUWAIAgentLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Can't amend tension b/c can't contact %S",
						GET_PLAYER(eTargetPlayer).getName(0));
			}
		}
	}
}


DenialTypes UWAI::Team::declareWarTrade(TeamTypes eTarget, TeamTypes eSponsor) const
{
	if (!canReach(eTarget))
		return DENIAL_NO_GAIN;
	CvTeamAI const& kAgent = GET_TEAM(m_eAgent);
	CvTeam const& kSponsor = GET_TEAM(eSponsor);
	/*  Check canBeHiredAgainst only in large games (to reduce the number of
		war trade alerts seen by humans) */
	bool bInsufficientPayment = false;
	if (!kSponsor.isHuman() || kSponsor.getHasMetCivCount() < 8 ||
		leaderCache().canBeHiredAgainst(eTarget))
	{
		int iUtilityThresh = iWarTradeUtilityThresh + 2;
		UWAILogMuteState silentLogMuteState(true);
		WarEvalParameters params(kAgent.getID(), eTarget, silentLogMuteState,
				false, kSponsor.getLeaderID());
		WarEvaluator eval(params, true);
		// Has to be negative; we can't be hired for free.
		int iU = std::min(-1, eval.evaluate(WARPLAN_LIMITED));
		if (iU > iUtilityThresh)
		{
			if (GET_TEAM(eSponsor).isHuman())
			{
				// Don't return NO_DENIAL if human can't pay enough
				int iHumanTradeVal = -1;
				/*	Would be nice if eSponsor were a player, but that seems
					difficult to change ... */
				for (MemberIter itSponsorMember(eSponsor);
					itSponsorMember.hasNext(); ++itSponsorMember)
				{
					for (MemberAIIter itAgentMember(kAgent.getID());
						itAgentMember.hasNext(); ++itAgentMember)
					{
						int iMemberTradeVal=-1;
						itAgentMember->uwai().canTradeAssets(
								utilityToTradeVal(-iUtilityThresh).round(),
								itSponsorMember->getID(), &iMemberTradeVal);
						iHumanTradeVal = std::max(iHumanTradeVal, iMemberTradeVal);
					}
				}
				iUtilityThresh = std::max(iUtilityThresh,
						- (tradeValToUtility(iHumanTradeVal) +
						// For gold that the human might be able to procure
						((GET_TEAM(eSponsor).isGoldTrading() ||
						kAgent.isGoldTrading() ||
						// Or they could ask nicely
						(GET_TEAM(eSponsor).isAtWar(eTarget) &&
						kAgent.AI_getAttitude(eSponsor) >= ATTITUDE_PLEASED)) ?
						(GC.getGame().isOption(GAMEOPTION_NO_TECH_TRADING) ? 6 : 4) : 0) +
						// For tech that the human might get access to soon
						(GET_TEAM(eSponsor).isTechTrading() ||
						kAgent.isTechTrading() ? 4 : 0)).round());
			}
			if (iU > iUtilityThresh)
				return NO_DENIAL;
			else bInsufficientPayment = true;
		}
		/* "Maybe we'll change our mind" when it's (very) close?
			No, don't provide this info after all. */
		/*if (iU > utilityThresh - 5)
			return DENIAL_RECENT_CANCEL;*/
	}
	// We don't know why utility is so small; can only guess.
	if (!bInsufficientPayment &&
		4 * kAgent.getPower(true) +
		(kSponsor.isAtWar(eTarget) ? 2 * kSponsor.getPower(true) : 0) <
		3 * GET_TEAM(eTarget).getPower(true))
	{
		return DENIAL_POWER_THEM;
	}
	if (kAgent.AI_anyMemberAtVictoryStage(AI_VICTORY_CULTURE4 | AI_VICTORY_SPACE4))
		return DENIAL_VICTORY;
	// "Too much on our hands" can mean anything
	return DENIAL_TOO_MANY_WARS;
}


int UWAI::Team::declareWarTradeVal(TeamTypes eTarget, TeamTypes eSponsor) const
{
	CvTeamAI const& kAgent = GET_TEAM(m_eAgent);
	bool const bMuteLog = GET_TEAM(eSponsor).isHuman();
	UWAILogMuteState logMuteState(bMuteLog);
	if (!bMuteLog)
	{
		if (gUWAIAgentLogLevel >= 2 && !logMuteState.isMuted()) logBBAI("Considering sponsored war");
		if (gUWAIAgentLogLevel >= 2 && !logMuteState.isMuted()) logBBAI("%S is considering to declare war on %S at the request of %S",
				GET_TEAM(kAgent.getID()).getName().GetCString(), GET_TEAM(eTarget).getName().GetCString(),
				GET_TEAM(eSponsor).getName().GetCString());
		/*  Will see the above lines multiple times in the log when this team
			agrees to declare war b/c CvGame::implementDeal causes the dealValue
			to be recomputed twice for diplomatic consequences ("traded with enemy",
			"fair and forthright"). */
	}
	CvTeamAI const& kSponsor = GET_TEAM(eSponsor);
	// Don't log details of war evaluation
	UWAILogMuteState silentLogMuteState(true);
	WarEvalParameters params(kAgent.getID(), eTarget, silentLogMuteState, false,
			kSponsor.getLeaderID());
	WarEvaluator eval(params);
	int iU = eval.evaluate(WARPLAN_LIMITED);
	/*  Sponsored war results in a peace treaty with the sponsor. Don't check if
		we're planning war against the sponsor - too easy to read (b/c there are
		just two possibilities). Instead check war utility against the sponsor. */
	if (canSchemeAgainst(eSponsor, true, false))
	{
		WarEvalParameters paramsVsSponsor(kAgent.getID(), eSponsor, silentLogMuteState);
		WarEvaluator evalVsSponsor(paramsVsSponsor);
		int iUtilityVsSponsor = evalVsSponsor.evaluate(WARPLAN_LIMITED, 3);
		if (iUtilityVsSponsor > 0)
			iU -= (fixp(2/3.) * iUtilityVsSponsor).round();
	}
	/*  Don't trust utility completely; human sponsor will try to pick the time
		when we're most willing. Need to be conservative. Also, apparently the
		sponsor gets sth. out of the DoW, and therefore we should always ask for
		a decent price, even if we don't mind declaring war. */
	int iLowerBound = -2;
	if (!kSponsor.isAtWar(eTarget))
		iLowerBound -= 4;
	// War utility is especially unreliable when things get desperate
	{
		int iWarSuccessRating = kAgent.AI_getWarSuccessRating();
		if (iWarSuccessRating < 0)
			iLowerBound += iWarSuccessRating / 10;
	}
	iU = std::min(iLowerBound, iU);
	scaled rPriceOurEconomy = utilityToTradeVal(-iU);
	scaled rPriceSponsorEconomy = kSponsor.uwai().utilityToTradeVal(-iU);
	/*  If the sponsor has the bigger economy, use the mean of the price based on
		our economy and his, otherwise, base it only on our economy. */
	scaled rPrice = (rPriceOurEconomy +
			std::max(rPriceOurEconomy, rPriceSponsorEconomy)) / 2;
	if (gUWAIAgentLogLevel >= 2 && !logMuteState.isMuted()) logBBAI("War utility: %d, base price: %d", iU, rPrice.round());
	/*  Adjust the price based on our attitude and obscure it so that humans
		can't learn how willing we are exactly */
	AttitudeTypes const eTowardSponsor = kAgent.AI_getAttitude(eSponsor);
	// 0.25 if pleased, 0.5 cautious, 1 furious
	scaled rAttitudeModifier(ATTITUDE_FRIENDLY - eTowardSponsor,
			ATTITUDE_FRIENDLY);
	// Mates' rates, but will still obscure the price (modifier != 0).
	if (eTowardSponsor == ATTITUDE_FRIENDLY)
		rAttitudeModifier = fixp(-0.25);
	/*  Put our money where our mouth is: discount for war on our worst enemy.
		(Other than that, our attitude toward the target is sufficiently reflected
		by war utility.) */
	if (kAgent.AI_getWorstEnemy() == eTarget)
	{
		rAttitudeModifier -= fixp(0.25);
		if (rAttitudeModifier == 0) // Avoid 0 for the sake of obscurity
			rAttitudeModifier -= fixp(0.25);
	}
	vector<int> aiInputs;
	/*  Allow hash to change w/e the target's rank or our attitude toward the
		sponsor changes */
	aiInputs.push_back(GC.getGame().getRankTeam(eTarget));
	aiInputs.push_back(eTowardSponsor);
	scaled rModifierWeight = fixp(0.6) * scaled::hash(aiInputs, kAgent.getLeaderID());
	scaled rObscuredPrice = rPrice * (1 + rAttitudeModifier * rModifierWeight);
	int iR = rObscuredPrice.roundToMultiple(10); // Makes gold cost a multiple of 5
	if (gUWAIAgentLogLevel >= 2 && !logMuteState.isMuted()) logBBAI("Obscured price: %d (attitude modifier: %d percent)", iR,
			rAttitudeModifier.getPercent());
	return iR;
}


DenialTypes UWAI::Team::makePeaceTrade(TeamTypes eEnemy, TeamTypes eBroker) const
{
	// Broker can't be involved in the war (not checked by caller; BtS allows it).
	if (GET_TEAM(eBroker).isAtWar(eEnemy))
		return DENIAL_JOKING;
	// (Willingness to talk is essentially a team-level decision; can use any members.)
	bool const bEnemyWillTalk = GET_PLAYER(GET_TEAM(m_eAgent).getLeaderID()).
			canContact(GET_TEAM(eEnemy).getLeaderID(), true);
	if (!bEnemyWillTalk && !gDLL->isDiplomacy())
	{
		// Don't waste time with a more specific answer if human won't read it anyway
		return DENIAL_RECENT_CANCEL;
	}
	int const iAgentReluct = reluctanceToPeace(eEnemy);
	int const iEnemyReluct = GET_TEAM(eEnemy).uwai().reluctanceToPeace(m_eAgent);
	bool bNoDenial = false; // Will still have to check bEnemyWillTalk then
	if (iEnemyReluct <= 0)
	{
		if (iAgentReluct < 55)
			bNoDenial = true;
		else
		{
			CvGameAI const& kGame = GC.AI_getGame();
			scaled rScoreRatio(kGame.getTeamScore(m_eAgent),
					kGame.getTeamScore(kGame.getRankTeam((TeamTypes)0)));
			scaled const rGameEra = kGame.AI_getCurrEraFactor();
			// <!-- custom: AdvCiv introduced this low-score relaxation but returned DENIAL_VICTORY immediately after setting bNoDenial, making the successful branch unreachable.
			// Let success reach the existing NO_DENIAL gate; retain the denial when the score exception fails. See KI#507. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			if (rGameEra > 0 &&
				rScoreRatio < ((rGameEra - 1) / rGameEra + fixp(2/3.)) / 2)
			{
				// We're not doing well in score; the broker might be doing much better.
				if (iAgentReluct < 70)
					bNoDenial = true;
				else return DENIAL_TOO_MUCH;
			}
			else return DENIAL_VICTORY;
		}
	}
	if (!bEnemyWillTalk)
	{
		if (iAgentReluct <= 0 && iEnemyReluct > 0 && iEnemyReluct - iAgentReluct >= 15)
		{
			/*	They'll refuse with "not right now", so it's a bit pointless to
				"contact them", but if _we_ say "not right now" it'll be misleading
				b/c we probably won't be able to make peace even once they become
				willing to talk. */
			return DENIAL_CONTACT_THEM;
		}
		return DENIAL_RECENT_CANCEL;
	}
	if (bNoDenial)
		return NO_DENIAL;
	/*  Unusual case: both sides want to continue, so both would have to be paid
		by the broker, which is too complicated. "Not right now" is true enough -
		will probably change soon. */
	if (iAgentReluct > 0)
		return DENIAL_RECENT_CANCEL;
	return DENIAL_CONTACT_THEM;
}


int UWAI::Team::makePeaceTradeVal(TeamTypes eEnemy, TeamTypes eBroker) const
{
	int const iAgentReluct = reluctanceToPeace(eEnemy);
	// Demand at least a token payment
	scaled r = utilityToTradeVal(std::max(3, iAgentReluct));
	CvTeamAI const& kAgent = GET_TEAM(m_eAgent);
	AttitudeTypes eTowardBroker = kAgent.AI_getAttitude(eBroker);
	// Make it a bit easier to broker peace as part of a peace treaty
	if (kAgent.isAtWar(eBroker) && eTowardBroker < ATTITUDE_CAUTIOUS)
		eTowardBroker = ATTITUDE_CAUTIOUS;
	/*  Between 400% and 77%. 100% when we're pleased with both.
		We prefer to get even with our enemy, so letting the broker pay 1 for 1 is
		already a concession. */
	scaled rAttitudeModifier(10, 1 + 2 * eTowardBroker + GET_TEAM(eEnemy).AI_getAttitude(eBroker));
	rAttitudeModifier.decreaseTo(4);
	int const iWarDuration = kAgent.AI_getAtWarCounter(eEnemy);
	// <!-- custom: AdvCiv assumed the at-war counter was at least 1, but third-party peace brokerage can be valued on the declaration turn while it is 0.
	// The formula already supports that state through sqrt(iWarDuration + 1); assert the real nonnegative domain without changing its deliberately high early-war markup. See KI#520. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	FAssert(iWarDuration >= 0);
	scaled rTimeModifier = (fixp(5.5) - kAgent.AI_getCurrEraFactor() / 2) / scaled(iWarDuration + 1).sqrt();
	rTimeModifier.increaseTo(fixp(0.75));
	r *= rAttitudeModifier * rTimeModifier;
	return kAgent.AI_roundTradeVal(r.round());
}


int UWAI::Team::endWarVal(TeamTypes eEnemy) const
{
	bool const bAgentHuman = GET_TEAM(m_eAgent).isHuman();
	FAssertMsg(bAgentHuman || GET_TEAM(eEnemy).isHuman(),
			"This should only be called for human-AI peace");
	CvTeamAI const& kHuman = (bAgentHuman ? GET_TEAM(m_eAgent) : GET_TEAM(eEnemy));
	CvTeamAI const& kAI =  (bAgentHuman ? GET_TEAM(eEnemy) : GET_TEAM(m_eAgent));
	int iAIReluct = kAI.uwai().reluctanceToPeace(kHuman.getID(), false);
	if (iAIReluct <= 0)
	{
		bool bCanTrade = false;
		for (MemberIter itHumanMember(kHuman.getID());
			itHumanMember.hasNext(); ++itHumanMember)
		{
			for (MemberIter itAIMember(kAI.getID());
				itAIMember.hasNext(); ++itAIMember)
			{
				if (itHumanMember->canPossiblyTradeItem(
					itAIMember->getID(), TRADE_GOLD) ||
					itHumanMember->canPossiblyTradeItem(
					itAIMember->getID(), TRADE_TECHNOLOGIES) ||
					// <!-- custom: AdvCiv's early no-payment shortcut checked only Gold and Technologies even though its live peace constructor can use a legal city independently of those trade options.
					// Include Cities in this cheap capability test; downstream legality, valuation and selection remain unchanged. See KI#509. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
					itHumanMember->canPossiblyTradeItem(
					itAIMember->getID(), TRADE_CITIES))
				{
					bCanTrade = true;
					break;
				}
			}
		}
		if (!bCanTrade)
			return 0;
	}
	// Really just utility given how peaceThreshold is computed for humans
	int iHumanUtility = kHuman.uwai().reluctanceToPeace(kAI.getID(), false);
	/*	Neither side pays if both want peace and the AI wants it
		more badly than the human - but not far more badly. */
	if (iHumanUtility <= 0 && iAIReluct < iHumanUtility &&
		iAIReluct >= 2 * iHumanUtility)
	{
		return 0;
	}
	scaled r;
	// Only AI wants to end the war or AI wants to end it much more badly
	if (iAIReluct < 0 && (iHumanUtility >= 0 || iAIReluct < 2 * iHumanUtility))
	{
		// Human pays nothing if AI pays
		if (bAgentHuman)
			return 0;
		if (iHumanUtility >= 0)
		{
			// Rely more on war utility of the AI side than on human war utility
			r = fixp(0.5) * (std::min(-iAIReluct, iHumanUtility) - iAIReluct);
		}
		/*	Both negative: A rather symbolic payment -
			unless the war is disastrous for the AI. */
		else r = iHumanUtility - fixp(0.5) * iAIReluct;
		/*  What if human declares war, but never sends units, although we believe
			that it would hurt us and benefit them (all things considered)?
			Then we're probably wrong somewhere and shouldn't trust our
			utility computations. */
		if (kAI.AI_getMemoryCount(kHuman.getID(), MEMORY_DECLARED_WAR) > 0 &&
			kAI.getNumCities() > 0)
		{
			scaled rWSDelta = scaled::max(0,
					kHuman.AI_getWarSuccess(kAI.getID())
					-kAI.AI_getWarSuccess(kHuman.getID()));
			scaled rWSAdjustment = (4 * rWSDelta) /
					(GC.getWAR_SUCCESS_CITY_CAPTURING() * kAI.getNumCities());
			rWSAdjustment.decreaseTo(1);
			r *= rWSAdjustment;
		}
		r = kAI.uwai().reparationsToHuman(r) * rReparationsModifierAI;
	}
	else
	{
		// AI pays nothing if human pays
		if (!bAgentHuman)
			return 0;
		// Don't demand payment if willing to capitulate to human
		if (kAI.AI_surrenderTrade(
			kHuman.getID(), CvTeamAI::VASSAL_POWER_MOD_SURRENDER, false) == NO_DENIAL)
		{
			return 0;
		}
		// (No limit on human reparations)
		if (iAIReluct > 0) // This is enough to make peace worthwhile for the AI
			r = kAI.uwai().utilityToTradeVal(iAIReluct);
		/*  But if human wants to end the war more badly than AI, AI also tries
			to take advantage of that. */
		if (iHumanUtility < 0)
		{
			/*  How much we try to squeeze the human. Not much b/c trade values
				go a lot higher now than they do in BtS. 5 utility can easily
				correspond to 1000 gold in the midgame. The AI evaluation of human
				utility isn't too reliable (may well assume that the human starts
				some costly but futile offensive) and fluctuates a lot from turn
				to turn, whereas peace terms mustn't fluctuate too much.
				And at least if iAIReluct < 0, we do want peace negotiations to
				succeed. */
			scaled rGreedFactor = fixp(0.05);
			if (iAIReluct > 0)
				rGreedFactor += fixp(0.1);
			int const iDelta = std::min(0, iAIReluct) - iHumanUtility;
			if (iDelta == 0)
				return 0;
			FAssert(iDelta > 0);
			// Conversion based on human's economy
			r += rGreedFactor * kHuman.uwai().utilityToTradeVal(iDelta);
			r *= rReparationsModifierHuman;
			/*  Demand less if human has too little. Akin to the
				tech/gold trading clause higher up.
				(Too lazy to implement this check properly for team games.) */
			if (iAIReluct < 0 && kHuman.getNumMembers() == 1)
			{
				int iMaxHumanCanPay = -1;
				kAI.uwai().leaderUWAI().canTradeAssets(
						r.round(), kHuman.getLeaderID(),
						&iMaxHumanCanPay);
				if (iMaxHumanCanPay < r)
				{
					/*  This means that the human player may want to make peace
						right away when the AI becomes willing to talk b/c the
						AI could change its mind again on the next turn. */
					if (scaled::hash(
						/*  Integer division to avoid flickering when the right side
							of the inequality is near 0.5. Don't just hash the
							game turn b/c that would mean that the AI can change its mind
							only every so many turns - too predictable. */
						(GC.getGame().getGameTurn() - kAI.AI_getWarSuccessRating()) / 8,
						kAI.getLeaderID()) < scaled(iAIReluct, -40))
					{
						r = iMaxHumanCanPay;
					}
				}
				// Demand more if they have a lot. Also to avoid asking for a pittance.
				else if (r > 0)
				{
					scaled const rMaxExtraMult = fixp(1.5);
					scaled const rAssetsImpact = fixp(0.25);
					// Recompute because earlier calculation had stopped at the current r
					kAI.uwai().leaderUWAI().canTradeAssets(
							(r * rMaxExtraMult / rAssetsImpact).ceil(),
							kHuman.getLeaderID(), &iMaxHumanCanPay, true);
					if (iMaxHumanCanPay > r)
					{
						r *= 1 + scaled::min(rMaxExtraMult,
								rAssetsImpact * iMaxHumanCanPay / r);
					}
				}
			}
		}
	}
	return r.round();
}


int UWAI::Team::uEndAllWars(VoteSourceTypes eVS) const
{
	vector<TeamTypes> aeWarEnemies;
	for (TeamIter<FREE_MAJOR_CIV,ENEMY_OF> itEnemy(m_eAgent);
		itEnemy.hasNext(); ++itEnemy)
	{
		if (eVS == NO_VOTESOURCE || itEnemy->isVotingMember(eVS))
			aeWarEnemies.push_back(itEnemy->getID());
	}
	if (aeWarEnemies.empty())
	{
		FAssert(!aeWarEnemies.empty());
		return 0;
	}
	UWAILogMuteState logMuteState;
	if (gUWAIAgentLogLevel >= 2) logBBAI("Peace vote");
	if (gUWAIAgentLogLevel >= 2) logBBAI("%S is evaluating the utility of war against %S in order to "
			"decide whether to vote for peace between self and everyone",
			GET_TEAM(m_eAgent).getName().GetCString(), GET_TEAM(aeWarEnemies[0]).getName().GetCString());
	WarEvalParameters params(m_eAgent, aeWarEnemies[0], logMuteState);
	for (size_t i = 1; i < aeWarEnemies.size(); i++)
	{
		params.addExtraTarget(aeWarEnemies[i]);
		if (gUWAIAgentLogLevel >= 2 && !logMuteState.isMuted()) logBBAI("War enemy: %S", GET_TEAM(aeWarEnemies[i]).getName().GetCString());
	}
	WarEvaluator eval(params);
	int iR = -eval.evaluate();
	return iR;
}


int UWAI::Team::uJointWar(TeamTypes eTarget, VoteSourceTypes eVS) const
{
	UWAILogMuteState logMuteState;
	if (gUWAIAgentLogLevel >= 2) logBBAI("War vote");
	if (gUWAIAgentLogLevel >= 2) logBBAI("%S is evaluating the utility of war against %S through diplo vote",
			GET_TEAM(m_eAgent).getName().GetCString(), GET_TEAM(eTarget).getName().GetCString());
	vector<TeamTypes> aeAllies;
	for(PlayerIter<FREE_MAJOR_CIV,POTENTIAL_ENEMY_OF> itAlly(eTarget);
		itAlly.hasNext(); ++itAlly)
	{
		if (itAlly->isVotingMember(eVS) && itAlly->getTeam() != m_eAgent &&
			!GET_TEAM(itAlly->getTeam()).isAtWar(eTarget))
		{
			if (gUWAIAgentLogLevel >= 2 && !logMuteState.isMuted()) logBBAI("%S would join as a war ally",
					GET_PLAYER(itAlly->getID()).getName(0));
			aeAllies.push_back(itAlly->getTeam());
		}
	}
	WarEvalParameters params(m_eAgent, eTarget, logMuteState);
	for (size_t i = 0; i < aeAllies.size(); i++)
		params.addWarAlly(aeAllies[i]);
	params.setImmediateDoW(true);
	WarPlanTypes eWP = WARPLAN_LIMITED;
	// (CvPlayerAI::AI_diploVote actually rules this out)
	if (GET_TEAM(m_eAgent).isAtWar(eTarget))
	{
		params.setNotConsideringPeace();
		eWP = NO_WARPLAN; // evaluate the current plan
	}
	WarEvaluator eval(params);
	int iR = eval.evaluate(eWP);
	return iR;
}


int UWAI::Team::uEndWar(TeamTypes eEnemy) const
{
	UWAILogMuteState silentLogMuteState(true);
	WarEvalParameters params(m_eAgent, eEnemy, silentLogMuteState);
	WarEvaluator eval(params);
	return -eval.evaluate();
}


scaled UWAI::Team::reparationsToHuman(scaled rUtility) const
{
	/*  iMaxReparationUtility is the upper limit for inter-AI peace;
		be less generous with humans */
	scaled const rTop(4 * iMaxReparationUtility, 5);
	/*  If utility for reparations is above the cap, we become less
		willing to pay b/c we don't believe that the peace can last. */
	if (rUtility > rTop)
	{
		scaled const rBottom = rTop / 4;
		scaled const rGradient(-1, 6);
		scaled rDelta = (rUtility - rTop);
		/*	Decreasing linearly from top to bottom seems a bit too
			steep for small delta. Choose an exponent and divisor
			so that the delta for which rUtility reaches the bottom
			remains unchanged (fixpoint). */
		scaled const rDeltaBottom = (-1 / rGradient) * (rTop - rBottom);
		int const iDiv = 3;
		// Tbd.: Add a logarithm function to ScaledNum
		double const dDeltaBottom = rDeltaBottom.getDouble();
		double const dExponent = std::log(dDeltaBottom * iDiv) /
				std::log(dDeltaBottom);
		scaled const rExponent = scaled::fromDouble(dExponent);
		rDelta.exponentiate(rExponent);
		rDelta /= iDiv;
		rUtility = std::max(rBottom, rTop + rDelta * rGradient);
	}
	return utilityToTradeVal(rUtility); // Trade value based on our economy
}


void UWAI::Team::respondToRebuke(TeamTypes eTarget, bool bPrepare)
{
	/*  Caveat: Mustn't use RNG here b/c this is called from both async (bPrepare=false)
		and sync (bPrepare=true) contexts */
	CvTeamAI& kAgent = GET_TEAM(m_eAgent);
	if (!canSchemeAgainst(eTarget, true) || (bPrepare ?
		kAgent.AI_isSneakAttackPreparing(eTarget) :
		kAgent.AI_isSneakAttackReady(eTarget)))
	{
		return;
	}
	if (!bPrepare && !kAgent.canDeclareWar(eTarget))
		return;
	FAssert(GET_TEAM(eTarget).isHuman());
	UWAILogMuteState silentLogMuteState(true);
	WarEvalParameters params(kAgent.getID(), eTarget, silentLogMuteState);
	WarEvaluator eval(params);
	int const iU = eval.evaluate(bPrepare ? WARPLAN_PREPARING_LIMITED : WARPLAN_LIMITED);
	if (iU < 0)
		return;
	if (bPrepare)
		kAgent.AI_setWarPlan(eTarget, WARPLAN_PREPARING_LIMITED);
	else kAgent.AI_setWarPlan(eTarget, WARPLAN_LIMITED);
}


DenialTypes UWAI::Team::acceptVassal(TeamTypes eVassal) const
{
	CvTeamAI const& kAgent = GET_TEAM(m_eAgent);
	vector<TeamTypes> aeWarEnemies; // Just the new ones
	for (TeamIter<FREE_MAJOR_CIV,ENEMY_OF> itEnemy(eVassal);
		itEnemy.hasNext(); ++itEnemy)
	{
		if (!itEnemy->isAtWar(kAgent.getID()))
		{
			aeWarEnemies.push_back(itEnemy->getID());
			FAssert(kAgent.isHasMet(itEnemy->getID())); // eVassal shouldn't ask us then
		}
	}
	if (aeWarEnemies.empty())
	{
		FAssert(!aeWarEnemies.empty());
		return NO_DENIAL;
	}
	/*  Ideally, WarEvaluator would have a mode for assuming a vassal agreement,
		and would do everything this function needs. I didn't think of this early
		enough. As it is, WarEvaluator can handle the resulting wars well enough,
		but won't account for the utility of us gaining a vassal, nor for any
		altruistic desire to protect the vassal. (Assistance::evaluate isn't
		altruistic.)
		GreedForVassals::evaluate has quite sophisticated code for evaluating
		vassals, but can't be easily separated from the context of WarEvaluator.
		I'm using only the cached part of that computation. */
	bool const bMuteLog = kAgent.isHuman();
	UWAILogMuteState logMuteState(bMuteLog);
	if (!bMuteLog)
	{
		if (gUWAIAgentLogLevel >= 2 && !logMuteState.isMuted()) logBBAI("Considering war to accept vassal");
		if (gUWAIAgentLogLevel >= 2 && !logMuteState.isMuted()) logBBAI("%S is considering to accept %S as its vassal; implied DoW on:",
				GET_TEAM(kAgent.getID()).getName().GetCString(), GET_TEAM(eVassal).getName().GetCString());
		if (gUWAIAgentLogLevel >= 2 && !logMuteState.isMuted())
		{
			for (size_t i = 0; i < aeWarEnemies.size(); i++)
				logBBAI("%S", GET_TEAM(aeWarEnemies[i]).getName().GetCString());
		}
	}
	int iResourceScore = 0;
	int iTechScore = 0;
	for (MemberIter itVassalMember(eVassal);
		itVassalMember.hasNext(); ++itVassalMember)
	{
		iResourceScore += leaderCache().vassalResourceScore(
				itVassalMember->getID());
		iTechScore += leaderCache().vassalTechScore(
				itVassalMember->getID());
	}
	// resourceScore is already utility
	scaled rVassalUtility = tradeValToUtility(iTechScore) + iResourceScore;
	if (gUWAIAgentLogLevel >= 2 && !logMuteState.isMuted()) logBBAI("%d utility from vassal resources, %d from tech", iResourceScore,
			(rVassalUtility - iResourceScore).round());
	rVassalUtility += scaled(GET_TEAM(eVassal).getNumCities() * 30,
			kAgent.getNumCities() + 1);
	if (kAgent.AI_anyMemberAtVictoryStage(
		AI_VICTORY_DIPLOMACY4 | AI_VICTORY_CONQUEST4))
	{
		rVassalUtility *= 2;
	}
	else if (kAgent.AI_anyMemberAtVictoryStage(
		AI_VICTORY_DIPLOMACY3 | AI_VICTORY_CONQUEST3))
	{
		rVassalUtility *= fixp(1.4);
	}
	/*  If the war will go badly for us, we'll likely not be able to protect
		the vassal. rVassalUtility therefore mustn't be so high that it could
		compensate for a lost war; should only compensate for bad diplo and
		military build-up.
		Except, maybe, if we're Friendly toward the vassal (see below). */
	rVassalUtility.decreaseTo(25);
	if (gUWAIAgentLogLevel >= 2 && !logMuteState.isMuted()) logBBAI("Utility after adding vassal cities: %d", rVassalUtility.round());
	/*  CvTeamAI::AI_vassalTrade already does an attitude check - we know we don't
		_dislike_ the vassal */
	if (kAgent.AI_getAttitude(eVassal) >= ATTITUDE_FRIENDLY)
	{
		rVassalUtility += 5;
		if (gUWAIAgentLogLevel >= 2 && !logMuteState.isMuted()) logBBAI("Utility increased b/c of attitude");
	}
	//UWAILogMuteState silentLogMuteState(true); // use this one for fewer details
	WarEvalParameters params(kAgent.getID(), aeWarEnemies[0], logMuteState);
	for (size_t i = 1; i < aeWarEnemies.size(); i++)
		params.addExtraTarget(aeWarEnemies[i]);
	params.setImmediateDoW(true);
	WarEvaluator eval(params);
	int iWarUtility = eval.evaluate(WARPLAN_LIMITED);
	if (gUWAIAgentLogLevel >= 2 && !logMuteState.isMuted()) logBBAI("War utility: %d", iWarUtility);
	int iTotalUtility = rVassalUtility.round() + iWarUtility;
	if (iTotalUtility > 0)
	{
		if (gUWAIAgentLogLevel >= 1 && !logMuteState.isMuted()) logBBAI("Accepting vassal");
		return NO_DENIAL;
	}
	if (gUWAIAgentLogLevel >= 2 && !logMuteState.isMuted()) logBBAI("Vassal not accepted");
	// Doesn't matter which denial; no one gets to read this.
	return DENIAL_POWER_THEM;
}


bool UWAI::Team::isLandTarget(TeamTypes eTeam) const
{
	PROFILE_FUNC();
	bool bHasCoastalCity = false;
	bool bCanReachAnyByLand = false;
	for (MemberAIIter itMember(m_eAgent); itMember.hasNext(); ++itMember)
	{
		UWAICache const& kCache = itMember->uwai().getCache();
		// <!-- custom: Deep-sea cargo capability belongs to this player's cache.
		// Derive its land-distance exemption per member so an earlier teammate without cargo cannot widen every later member's limit. See KI#513. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		int const iDistLimit = (kCache.canTrainDeepSeaCargo() ?
			getUWAI().maxLandDist() : MAX_INT);
		for (int j = 0; j < kCache.numCities(); j++)
		{
			UWAICache::City& kCacheCity = kCache.cityAt(j);
			if (kCacheCity.city().getTeam() != eTeam)
				continue;
			if (kCacheCity.city().isCoastal())
				bHasCoastalCity = true;
			if (kCacheCity.canReachByLand())
			{
				bCanReachAnyByLand = true;
				// <!-- custom: Compare the land-target limit with the qualifying land route, not a faster mixed/sea duration. See KI#591. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				if (kCacheCity.getDistanceByLand() <= iDistLimit)
					return true;
			}
		}
	}
	/*  Tactical AI can't do naval assaults on inland cities. Assume that landlocked
		civs are land targets even if they're too far away; better than treating
		them as entirely unreachable. */
	return (!bHasCoastalCity && bCanReachAnyByLand);
}


bool UWAI::Team::canReach(TeamTypes eTarget) const
{
	for (MemberIter itTarget(eTarget); itTarget.hasNext(); ++itTarget)
	{
		for (MemberAIIter itAgent(m_eAgent); itAgent.hasNext(); ++itAgent)
		{	// (Don't call UWAI::Player::canReach - to avoid the call overhead.)
			if (itAgent->uwai().getCache().
				numReachableCities(itTarget->getID()) > 0)
			{
				return true;
			}
		}
	}
	return false;
}


// <!-- custom: Save-file 450 showed Lincoln reaching 11 spaceship parts before the victory countdown started, then later reporting stage 3 while countdown still showed only a few turns left. The later Arabia branch showed direct war at countdown 4 was mechanically correct but still too late. Save-file 452 then showed raw part count could still fire too late in a faster Space race. Allow direct war for hard countdown emergencies, weak/near stage-4 threats, or configured stage-3 Space threats, so UWAI does not wait until the disruption window is almost gone. (GPT-5.5) -->
// <!-- custom: Save-file 449 then showed the normal 3-turn contact limit assigning about -100000 utility even when this policy approved nearby, stronger Celts and Aztecs for direct war against India's launched spaceship. Keep the policy on UWAI::Team so target selection and the contact guard cannot drift apart. The caller supplies nearest-city plot distance for selection or cached path turns for the contact guard. (GPT-5.6-Sol) -->
bool UWAI::Team::isSASVictoryDenialDirectWarAllowed(TeamTypes eTarget, int iTargetMaxVictoryStage, bool bNaval, int iDistance) const
{
	CvTeamAI const& kAgent = GET_TEAM(m_eAgent);
	static const bool bEnable = GC.getDefineBOOL("SAS_UWAI_VICTORY_DENIAL_ENABLE");
	if (!bEnable)
		return false;
	int const iCountdown = GET_TEAM(eTarget).AI_getLowestVictoryCountdown();
	static const int iMaxCountdownDirectWarNormal = GC.getDefineINT("SAS_UWAI_VICTORY_DENIAL_MAX_COUNTDOWN_TURNS_NORMAL_GAMESPEED_DIRECT_WAR");
	int const iMaxCountdownDirectWar = getSASVictoryDelayTurnsFromNormalGameSpeed(iMaxCountdownDirectWarNormal);
	static const bool bDirectStage4Enable = GC.getDefineBOOL("SAS_UWAI_VICTORY_DENIAL_DIRECT_STAGE4_ENABLE");
	static const bool bDirectStage3SpaceEnable = GC.getDefineBOOL("SAS_UWAI_VICTORY_DENIAL_DIRECT_STAGE3_SPACE_ENABLE");
	bool const bCountdownDirect = (iCountdown >= 0 && iCountdown <= iMaxCountdownDirectWar);
	bool const bStage4Direct = (bDirectStage4Enable && iTargetMaxVictoryStage >= 4);
	bool const bStage3SpaceDirect = (bDirectStage3SpaceEnable && isSASTeamStage3SpaceVictoryThreat(eTarget));
	if (!bCountdownDirect && !bStage4Direct && !bStage3SpaceDirect)
		return false;
	static const int iMaxTargetPowerPercent = GC.getDefineINT("SAS_UWAI_VICTORY_DENIAL_DIRECT_MAX_TARGET_POWER_PERCENT");
	int const iTargetPowerPercent = 100 * GET_TEAM(eTarget).getDefensivePower(kAgent.getID()) / std::max(1, kAgent.getPower(true));
	if (iTargetPowerPercent > iMaxTargetPowerPercent)
		return false;
	static const int iMaxDistance = GC.getDefineINT("SAS_UWAI_VICTORY_DENIAL_DIRECT_MAX_DISTANCE");
	if (iDistance < 0 || iDistance > iMaxDistance)
		return false;
	return (!bNaval || kAgent.AI_isLandTarget(eTarget));
}


bool UWAI::Team::isCloseToAdoptingAnyWarPlan() const
{
	for (TeamIter<MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itRival(m_eAgent);
		itRival.hasNext(); ++itRival)
	{
		if (canSchemeAgainst(itRival->getID(), false) &&
			leaderCache().warUtilityIgnoringDistraction(itRival->getID()) >= -20)
		{
			return true;
		}
	}
	return false;
}


void UWAI::Team::showWarPrepStartedMsg(TeamTypes eTarget)
{
	showWarPlanMsg(eTarget, "TXT_KEY_WAR_PREPARATION_STARTED");
}


void UWAI::Team::showWarPlanAbandonedMsg(TeamTypes eTarget)
{
	showWarPlanMsg(eTarget, "TXT_KEY_WAR_PLAN_ABANDONED");
}


void UWAI::Team::showWarPlanMsg(TeamTypes eTarget, char const* szKey)
{
	// <!-- custom: make these static const for performance optimization as advised by chatgpt 5 too. -->
	static const bool bUWAI_SPECTATOR_ENABLED = GC.getDefineBOOL("UWAI_SPECTATOR_ENABLED");

	CvPlayer& kActivePlayer = GET_PLAYER(GC.getGame().getActivePlayer());
	if (!kActivePlayer.isSpectator() || !bUWAI_SPECTATOR_ENABLED)
		return;
	CvWString szBuffer = gDLL->getText(szKey,
			GET_TEAM(m_eAgent).getName().GetCString(),
			GET_TEAM(eTarget).getName().GetCString());
	gDLL->UI().addMessage(kActivePlayer.getID(), false, -1, szBuffer,
			0, MESSAGE_TYPE_MAJOR_EVENT,
			/* <advc.127b> */ NULL, NO_COLOR,
			GET_TEAM(m_eAgent).getCapitalX(kActivePlayer.getTeam(), true),
			GET_TEAM(m_eAgent).getCapitalY(kActivePlayer.getTeam(), true));
			// </advc.127b>
}

UWAICache& UWAI::Team::leaderCache()
{
	return GET_PLAYER(GET_TEAM(m_eAgent).getLeaderID()).uwai().getCache();
}

UWAICache const& UWAI::Team::leaderCache() const
{
	// Duplicate code; see also UWAICache::leaderCache.
	return GET_PLAYER(GET_TEAM(m_eAgent).getLeaderID()).uwai().getCache();
}


bool UWAI::Team::isWarEvalNeeded(TeamTypes eTeam) const
{
	return canSchemeAgainst(eTeam, true) || (!GET_TEAM(eTeam).isAVassal() &&
			!GET_TEAM(m_eAgent).isAVassal() && GET_TEAM(eTeam).isAtWar(m_eAgent));
}


void UWAI::Team::doWarReport()
{
	if (!getUWAI().isEnabled())
		return;
	// <!-- custom: The manual diagnostic helper previously switched only UWAI's global background state.
	// Team-side mutation guards use this cached state instead, so `doWar` could change real war plans and diplomacy while producing diagnostics.
	// Keep both views synchronized for the whole diagnostic run, then restore both independently. See KI#517. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	bool const bGlobalInBackground = getUWAI().isEnabled(true);
	bool const bTeamInBackground = m_bInBackground;
	getUWAI().setInBackground(true);
	m_bInBackground = true;
	doWar();
	m_bInBackground = bTeamInBackground;
	getUWAI().setInBackground(bGlobalInBackground);
}


UWAI::Player::Player() : m_eAgent(NO_PLAYER) {}


void UWAI::Player::init(PlayerTypes ePlayer)
{
	m_eAgent = ePlayer;
	m_cache.init(ePlayer);
}


void UWAI::Player::uninit()
{
	m_cache.uninit();
}


void UWAI::Player::turnPre()
{
	m_cache.update();
}


void UWAI::Player::write(FDataStreamBase* pStream) const
{
	pStream->Write(GET_PLAYER(m_eAgent).getID());
	m_cache.write(pStream);
}


void UWAI::Player::read(FDataStreamBase* pStream)
{
	pStream->Read((int*)&m_eAgent);
	m_cache.read(pStream);
}


bool UWAI::Player::considerDemand(PlayerTypes eDemandPlayer, int iTradeVal) const
{
	// When furious, they'll have to take it from our cold dead hands.
	if (GET_PLAYER(m_eAgent).AI_getAttitude(eDemandPlayer) <= ATTITUDE_FURIOUS)
		return false;
	/*  (I don't think the interface even allows demanding tribute when there's
		a peace treaty) */
	if (!GET_TEAM(eDemandPlayer).canDeclareWar(TEAMID(m_eAgent)))
		return false;
	UWAILogMuteState silentLogMuteState(true);
	WarEvalParameters ourParams(TEAMID(m_eAgent), TEAMID(eDemandPlayer), silentLogMuteState);
	WarEvaluator ourEval(ourParams);
	scaled rOurUtility = ourEval.evaluate(WARPLAN_LIMITED);
	// Add -5 to 40 for self-assertion
	rOurUtility += 45 * prideRating() - 5;
	/*  The more prior demands we remember, the more recalcitrant we become
		(does not count the current demand). */
	int const iMemoryDemand = GET_PLAYER(m_eAgent).AI_getMemoryCount(eDemandPlayer,
			MEMORY_MADE_DEMAND);
	if (iMemoryDemand > 0)
		rOurUtility += SQR(iMemoryDemand);
	if (rOurUtility >= 0) // Bring it on!
		return false;
	WarEvalParameters theirParams(TEAMID(eDemandPlayer),
			TEAMID(m_eAgent), silentLogMuteState);
	WarEvaluator theirEval(theirParams);
	/*  If they don't intend to attack soon, the peace treaty from tribute
		won't help us. Total war scares us more than limited. */
	int iTheirUtility = theirEval.evaluate(WARPLAN_TOTAL);
	if (iTheirUtility < 0)
		return false; // Call their bluff
	// Willing to pay at most this much
	scaled rMaxTradeVal = GET_TEAM(m_eAgent).uwai().reparationsToHuman(
			// Interpret theirUtility as a probability of attack
			(-rOurUtility * 2 * (4 + iTheirUtility)) / 100);
	return (iTradeVal <= rMaxTradeVal);
	/*  Some randomness? None in the BtS code (AI_considerOffer) either.
		Would have to use scaled::hash. */
}


bool UWAI::Player::considerPlea(PlayerTypes ePleaPlayer, int iTradeVal) const
{
	/*  Check only war utility and peace treaty here; all other preconditions
		are handled by CvPlayerAI::AI_considerOffer. */
	if (GET_TEAM(m_eAgent).isForcePeace(TEAMID(ePleaPlayer)) &&
		GET_PLAYER(m_eAgent).AI_getMemoryAttitude(ePleaPlayer, MEMORY_GIVE_HELP) <= 0)
	{
		return false;
	}
	// If war not possible, might as well sign a peace treaty.
	if (!GET_TEAM(m_eAgent).canDeclareWar(TEAMID(ePleaPlayer)) ||
		!GET_TEAM(ePleaPlayer).canDeclareWar(TEAMID(m_eAgent)))
	{
		return true;
	}
	/*  Accept probabilistically regardless of war utility (so long
		as we're not planning war yet, which the caller ensures).
		Probability to accept is 45% for Gandhi, 0% for Tokugawa. */
	scaled rAcceptProb = fixp(0.5) - GET_PLAYER(m_eAgent).AI_prDenyHelp();
	// Can't use sync'd RNG here, but don't want the outcome to change after reload.
	vector<int> aiInputs;
	aiInputs.push_back(GC.getGame().getGameTurn());
	aiInputs.push_back(iTradeVal);
	if (scaled::hash(aiInputs, m_eAgent) < rAcceptProb)
		return true;
	// Probably won't want to attack ePleaPlayer then
	if (GET_TEAM(m_eAgent).AI_isSneakAttackReady())
	{	// (Ruled out by caller)
		FAssert(!GET_TEAM(m_eAgent).AI_isSneakAttackReady(TEAMID(ePleaPlayer)));
		return true;
	}
	UWAILogMuteState silentLogMuteState(true);
	WarEvalParameters params(TEAMID(m_eAgent), TEAMID(ePleaPlayer), silentLogMuteState);
	WarEvaluator eval(params);
	int iU = eval.evaluate(WARPLAN_LIMITED, 5) - 5; // minus 5 for goodwill
	if (iU >= 0)
		return false;
	scaled rThresh = utilityToTradeVal(-iU);
	return (rThresh >= iTradeVal);
}


bool UWAI::Player::amendTensions(PlayerTypes eHuman)
{
	FAssert(GET_PLAYER(eHuman).isHuman());
	// <!-- custom: AdvCiv deliberately changed war diplomacy to select a random alive team member but retained the old leader-only assertion here.
	// Assert the surviving caller contract without rejecting valid nonleader diplomats. See KI#518. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	FAssert(GET_PLAYER(m_eAgent).isAlive());
	// Lower contact probabilities in later eras
	scaled const rEra = GET_PLAYER(m_eAgent).AI_getCurrEraFactor();
	CvLeaderHeadInfo const& kPersonality = GC.getInfo(GET_PLAYER(m_eAgent).
			getPersonalityType());
	if (GET_PLAYER(m_eAgent).AI_getAttitude(eHuman) <=
		kPersonality.getDemandTributeAttitudeThreshold())
	{
		// <!-- custom: Preserve only the UWAI caller origin when level-2 request provenance is enabled; AI_demandTribute fills and emits the realized chooser state. (ChatGPT-5.6-Sol) -->
		SASGameRecordAITributeRequestContext kSASTributeRequest;
		SASGameRecordAITributeRequestContext* pSASTributeRequest = NULL;
		if (gGameRecordLogLevel >= 2)
		{
			kSASTributeRequest.eOrigin = SAS_AI_HUMAN_REQUEST_UWAI_AMEND_TENSIONS;
			pSASTributeRequest = &kSASTributeRequest;
		}
		// <!-- custom: AdvC practical 2075 explicitly randomized tribute-demand type order in AI_doDiplo, but this parallel UWAI path retained fixed enum order and therefore favored earlier types before its first successful return.
		// Apply the same synchronized random ordering here; contact/helper logic is unchanged apart from removing that inherited positional bias. See KI#518.2. (ChatGPT-5.6-Sol) -->
		FOR_EACH_ENUM_RAND(AIDemand, syncRand())
		{
			if (GET_PLAYER(m_eAgent).AI_contactRoll(CONTACT_DEMAND_TRIBUTE,
				(fixp(8.5) - rEra) / 2) &&
				GET_PLAYER(m_eAgent).AI_demandTribute(eHuman, eLoopAIDemand, pSASTributeRequest))
			{
				return true;
			}
		}
	}
	else
	{
		// <!-- custom: Same origin-only recorder bridge for UWAI help requests; NULL leaves ordinary gameplay unchanged when detailed recording is off. (ChatGPT-5.6-Sol) -->
		SASGameRecordAIHelpRequestContext kSASHelpRequest;
		SASGameRecordAIHelpRequestContext* pSASHelpRequest = NULL;
		if (gGameRecordLogLevel >= 2)
		{
			kSASHelpRequest.eOrigin = SAS_AI_HUMAN_REQUEST_UWAI_AMEND_TENSIONS;
			pSASHelpRequest = &kSASHelpRequest;
		}
		if (GET_PLAYER(m_eAgent).AI_contactRoll(CONTACT_ASK_FOR_HELP,
			(fixp(5.5) - rEra) / fixp(1.25)) &&
			GET_PLAYER(m_eAgent).AI_askHelp(eHuman, pSASHelpRequest))
		{
			return true;
		}
	}
	if (kPersonality.getContactRand(CONTACT_RELIGION_PRESSURE) <=
		kPersonality.getContactRand(CONTACT_CIVIC_PRESSURE))
	{
		if (GET_PLAYER(m_eAgent).AI_contactRoll(CONTACT_RELIGION_PRESSURE,
			8 - rEra) &&
			GET_PLAYER(m_eAgent).AI_contactReligion(eHuman))
		{
			return true;
		}
	} 
	else
	{
		if (GET_PLAYER(m_eAgent).AI_contactRoll(CONTACT_CIVIC_PRESSURE,
			fixp(2.5)) &&
			GET_PLAYER(m_eAgent).AI_contactCivics(eHuman))
		{
			return true;
		}
	}
	// Embargo request - too unlikely to succeed I think.
	/*if (GET_PLAYER(m_eAgent).AI_contactRoll(CONTACT_STOP_TRADING, ?) &&
		GET_PLAYER(m_eAgent).AI_proposeEmbargo(eHuman))
	{
		return true;
	}*/
	return false;
}

// Wrapper that handles the war evaluator cache
int UWAI::Player::willTalk(PlayerTypes eToPlayer, int iAtWarCounter, bool bUseCache) const
{
	if (bUseCache)
		WarEvaluator::enableCache();
	int iR = willTalk(eToPlayer, iAtWarCounter);
	if (bUseCache)
		WarEvaluator::disableCache();
	return iR;
}


int UWAI::Player::willTalk(PlayerTypes eToPlayer, int iAtWarCounter) const
{
	if (GET_TEAM(m_eAgent).AI_surrenderTrade(TEAMID(eToPlayer)) == NO_DENIAL)
		return 1;
	// 1 turn RTT and let the team leader handle peace negotiation
	if (iAtWarCounter <= 1 || GET_TEAM(m_eAgent).getLeaderID() != m_eAgent)
		return -1;
	// bValid=true: want to return 1, but still need to check for DECLARED_WAR_RECENT.
	bool bValid = false;
	/*  Checking for a possible peace deal only serves as a convenience for
		human players; no need to do it for AI-AI peace. */
	if (!GET_PLAYER(eToPlayer).isHuman())
		bValid = true;
	else
	{
		bValid = (GET_TEAM(m_eAgent).AI_surrenderTrade(TEAMID(eToPlayer)) == NO_DENIAL ||
				isPeaceDealPossible(eToPlayer));
	}
	if (GET_PLAYER(m_eAgent).AI_getMemoryCount(eToPlayer, MEMORY_DECLARED_WAR_RECENT) > 0)
	{
		if (bValid)
			return 0;
		return -1;
	}
	return (bValid ? 1 : -1);
}


bool UWAI::Player::isPeaceDealPossible(PlayerTypes eHuman) const
{
	/*  Could simply call CvPlayerAI::AI_counterPropose, but I think there are rare
		situations when a deal is possible but AI_counterPropose doesn't find it.
		It would also be slower. */
	// <advc.705>
	CvGame const& kGame = GC.getGame();
	if (kGame.isOption(GAMEOPTION_RISE_FALL) &&
		kGame.getRiseFall().isCooperationRestricted(m_eAgent) &&
		GET_TEAM(m_eAgent).uwai().reluctanceToPeace(TEAMID(eHuman)) >= 20)
	{
		return false;
	} // </advc.705>
	int iTargetTradeVal = GET_TEAM(eHuman).uwai().endWarVal(TEAMID(m_eAgent));
	if (iTargetTradeVal <= 0)
		return true;
	return canTradeAssets(iTargetTradeVal, eHuman);
}


// (advc.ctr: Now unused b/c the AI will always accept cities as payment) <!-- custom: hoisted from multiline signature between `piAvailableTradeVal` and `bIgnoreCities` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
bool UWAI::Player::canTradeAssets(int iTargetTradeVal, PlayerTypes eHuman, int* piAvailableTradeVal, bool bIgnoreCities) const
{
	if (piAvailableTradeVal != NULL)
		*piAvailableTradeVal = iTargetTradeVal;
	int iTotalTradeVal = 0;
	CvPlayerAI const& kHuman = GET_PLAYER(eHuman);
	if (kHuman.canTradeItem(m_eAgent, TradeData(TRADE_GOLD, kHuman.getGold()), true))
		iTotalTradeVal += kHuman.getGold();
	if (iTotalTradeVal >= iTargetTradeVal)
		return true;
	FOR_EACH_ENUM(Tech)
	{
		if (kHuman.canTradeItem(m_eAgent, TradeData(TRADE_TECHNOLOGIES, eLoopTech), true))
		{
			iTotalTradeVal += GET_TEAM(m_eAgent).AI_techTradeVal(eLoopTech,
					kHuman.getTeam(), true, true);
			if (iTotalTradeVal >= iTargetTradeVal)
				return true;
		}
	}
	if (!bIgnoreCities)
	{
		int const iCityLimit = intdiv::uceil(kHuman.getNumCities(), 6);
		// <!-- custom: AdvCiv applied its intended one-sixth city-reparation cap to arbitrary city iteration order and could miss a later valuable city that satisfies the deal alone.
		// Collect every legal city value, then sum only the best values permitted by the unchanged quantity cap. See KI#510. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		vector<int> aiCityTradeValues;
		FOR_EACH_CITYAI(pCity, kHuman)
		{
			if(kHuman.canTradeItem(
				m_eAgent, TradeData(TRADE_CITIES, pCity->getID()), true))
				aiCityTradeValues.push_back(GET_PLAYER(m_eAgent).AI_cityTradeVal(*pCity));
		}
		std::sort(aiCityTradeValues.begin(), aiCityTradeValues.end());
		for (int i = 0; i < iCityLimit && i < (int)aiCityTradeValues.size(); i++)
		{
			iTotalTradeVal += aiCityTradeValues[aiCityTradeValues.size() - i - 1];
			if (iTotalTradeVal >= iTargetTradeVal)
				return true;
		}
	}
	FAssert(iTotalTradeVal < iTargetTradeVal);
	if (piAvailableTradeVal != NULL)
		*piAvailableTradeVal = iTotalTradeVal;
	return false;
}


scaled UWAI::Player::utilityToTradeVal(scaled rUtility) const
{
	return rUtility / tradeValUtilityConversionRate();
}


scaled UWAI::Player::tradeValToUtility(scaled rTradeVal) const
{
	return rTradeVal * tradeValUtilityConversionRate();
}


scaled UWAI::Player::tradeValUtilityConversionRate() const
{
	// Based on how long it would take us to produce as much trade value
	scaled rSpeedFactor = 1;
	int const iTrainPercent = GC.getInfo(GC.getGame().getGameSpeedType()).
			getTrainPercent();
	if (iTrainPercent > 0)
		rSpeedFactor = scaled(100, iTrainPercent);
	return std::max(scaled::epsilon(), (3 * rSpeedFactor) /
			(scaled::max(10,
			GET_TEAM(m_eAgent).AI_estimateYieldRate(m_eAgent, YIELD_COMMERCE))
			+ 2 * scaled::max(1,
			GET_TEAM(m_eAgent).AI_estimateYieldRate(m_eAgent, YIELD_PRODUCTION))));
	/*  Note that change advc.004s excludes espionage and culture from the
		Economy history, and estimateYieldRate(YIELD_COMMERCE) doesn't account
		for these yields either. Not a problem for culture, I think, which is
		usually produced in addition to gold and research, but the economic output
		of civs running the Big Espionage strategy will be underestimated.
		Still better than just adding up all commerce types. */
}


scaled UWAI::Player::amortizationMultiplier() const
{
	// 25 turns delay b/c war planning is generally about a medium-term future
	return GET_PLAYER(m_eAgent).AI_amortizationMultiplier(25);
}


scaled UWAI::Player::buildUnitProb() const
{
	scaled r;
	if (GET_PLAYER(m_eAgent).isHuman())
		r = humanBuildUnitProb();
	else r = per100(GC.getInfo(GET_PLAYER(m_eAgent).getPersonalityType()).
			getBuildUnitProb());
	// Accounting for advc.253
	r *= GET_PLAYER(m_eAgent).AI_trainUnitSpeedAdustment();
	return r;
}

// (Sort of duplicated in UWAI::Team::canReach)
bool UWAI::Player::canReach(PlayerTypes eTarget) const
{
	return (getCache().numReachableCities(eTarget) > 0);
}

/*	This player makes the prediction; the prediction is _about_ ePlayer.
	Like CvTeamAI::AI_estimateYieldRate (and other AI code), this function
	currently cheats by not checking whether demographics are visible.
	Would, in any case, make more sense as a team-level function,
	but I want to keep it together with buildUnitProb for now. */
scaled UWAI::Player::estimateBuildUpRate(PlayerTypes ePlayer, int iTurns) const
{
	iTurns *= GC.getInfo(GC.getGame().getGameSpeedType()).getTrainPercent();
	iTurns /= 100;
	return estimateDemographicGrowthRate(ePlayer, PLAYER_HISTORY_POWER, iTurns);
}


scaled UWAI::Player::estimateDemographicGrowthRate(PlayerTypes ePlayer, PlayerHistoryTypes eDemographic, int iTurns) const
{
	if (GC.getGame().getElapsedGameTurns() < iTurns + 1)
		return 0;
	int iGameTurn = GC.getGame().getGameTurn();
	int iPastValue = std::max(1, GET_PLAYER(ePlayer).getHistory(
			eDemographic, iGameTurn - 1 - iTurns));
	int iDelta = GET_PLAYER(ePlayer).getHistory(eDemographic, iGameTurn - 1)
			- iPastValue;
	return scaled::max(0, scaled(iDelta, iPastValue));
}


scaled UWAI::Player::humanBuildUnitProb() const
{
	scaled r = fixp(0.25); // 30 is about average, Gandhi 15
	if (GET_PLAYER(m_eAgent).getCurrentEra() == 0)
		r += fixp(0.1);
	if(GC.getGame().isOption(GAMEOPTION_RAGING_BARBARIANS) &&
		GET_PLAYER(m_eAgent).AI_getCurrEraFactor() <= 2)
	{
		r += fixp(0.05);
	}
	return r;
}


scaled UWAI::Player::confidenceFromWarSuccess(TeamTypes eTarget) const
{
	/*	Currently this function works entirely at the team level,
		but it could take into account the personality of this UWAI::Player
		in the future. */
	/*  Need to be careful not to overestimate
		early successes (often from a surprise attack). Bound the war success
		ratio based on how long the war lasts and the extent of successes. */
	CvTeamAI const& kAgent = GET_TEAM(m_eAgent);
	CvTeamAI const& kTarget = GET_TEAM(eTarget);
	int const iTurnsAtWar = kAgent.AI_getAtWarCounter(eTarget);
	/*  Can differ by 1 b/c of turn difference. Can apparently also differ by 2
		somehow, which I don't understand, but it's not a problem really. */
	FAssert(std::abs(iTurnsAtWar - kTarget.AI_getAtWarCounter(kAgent.getID())) <= 2);
	if (iTurnsAtWar <= 0)
		return -1;
	scaled const rAgentSuccess = std::max(scaled::epsilon(),
			kAgent.AI_getWarSuccess(eTarget));
	scaled const rTargetSuccess = std::max(scaled::epsilon(),
			kTarget.AI_getWarSuccess(kAgent.getID()));
	scaled rSuccessRatio = rAgentSuccess / rTargetSuccess;
	scaled const rFixedBound = fixp(0.5);
	// Reaches rFixedBound after 20 turns
	scaled rTimeBasedBound = (100 - fixp(2.5) * iTurnsAtWar) / 100;
	/*  Bound based on total war success: Becomes relevant once a war lasts long; e.g.
		after 25 turns, in the Industrial era, will need a total war success of 250
		in order to reach fixedBound. Neither side should feel confident if there
		isn't much action. */
	scaled rTotalBasedBound;
	{
		scaled rProgressFactor = 11 - kAgent.AI_getCurrEraFactor() * fixp(1.5);
		rProgressFactor.increaseTo(3);
		rTotalBasedBound = (100 - (rProgressFactor *
				(rAgentSuccess + rTargetSuccess)) / iTurnsAtWar) / 100;
	}
	scaled r = rSuccessRatio;
	r.clamp(rFixedBound, 2 - rFixedBound);
	r.clamp(rTimeBasedBound, 2 - rTimeBasedBound);
	r.clamp(rTotalBasedBound, 2 - rTotalBasedBound);
	return r;
}


scaled UWAI::Player::confidenceFromPastWars(TeamTypes eTarget) const
{
	scaled rPastWarScore = per100(m_cache.pastWarScore(eTarget));
	int iSign = (rPastWarScore < 0 ? -1 : 1);
	// -15% for the first lost war, less from further wars.
	scaled r = 1 + iSign * rPastWarScore.abs().sqrt() * fixp(0.15);
	r.clamp(fixp(0.5), fixp(1.5));
	return r;
}


scaled UWAI::Player::distrustRating() const
{
	if (GET_PLAYER(m_eAgent).isHuman())
		return 1;
	int iR = GC.getInfo(GET_PLAYER(m_eAgent).getPersonalityType()).
			getEspionageWeight() - 10;
	if (m_cache.hasDefensiveTrait())
		iR += 30;
	return scaled(std::max(0, iR), 100);
}


scaled UWAI::Player::warConfidencePersonal(bool bNaval, bool bTotal, PlayerTypes eTarget) const
{
	/*	AI assumes that human confidence depends on difficulty. NB: This doesn't
		mean that the AI thinks that humans are good at warfare - this is handled
		by confidenceAgainstHuman. Here, the AI thinks that humans think that
		humans are good at war, and that humans may therefore attack despite
		having little power. */
	if (GET_PLAYER(m_eAgent).isHuman())
	{
		// e.g. 0.62 at Settler, 1.59 at Deity
		return GET_PLAYER(m_eAgent).trainingModifierFromHandicap() /
				GET_PLAYER(eTarget).trainingModifierFromHandicap();
	}
	CvLeaderHeadInfo const& kPersonality = GC.getInfo(GET_PLAYER(m_eAgent).
			getPersonalityType());
	int const iMaxWarNearbyPR = kPersonality.getMaxWarNearbyPowerRatio();
	int const iMaxWarDistPR = kPersonality.getMaxWarDistantPowerRatio();
	int const iLimWarPR = kPersonality.getLimitedWarPowerRatio();
	scaled r = // Montezuma: 1.3; Elizabeth: 0.85
			//scaled(iMaxWarNearbyPR + iLimWarPR, 200);
		/*	Limited and total mostly affect the military build-up, not how the war
			is conducted. So it may not make much sense for a leader to be e.g.
			more optimistic about limited than total war. But the difference between
			the PowerRatio values should somehow matter. Perhaps some leaders think
			e.g. that they can't use large stacks so effectively ... */
			(bTotal ? iMaxWarNearbyPR : iLimWarPR);
			// (Or perhaps use a weighted mean as a compromise?)
	if (bNaval)
	{
		/*  distantWar refers to intercontinental war. The values in LeaderHead are
			between 0 (Sitting Bull) and 100 (Isabella), i.e. almost everyone is
			reluctant to fight cross-ocean wars. That reluctance is now covered
			elsewhere (e.g. army power reduced based on cargo capacity in
			simulations); hence the +35. This puts the return value between 0.35 and
			1.35. Exceeding the PR for land war is dangerous though; could cause
			the AI to plan for naval war when ships aren't needed at all. */
		r = scaled::min(r + 3, iMaxWarDistPR + 35);
	}
	return r / 100;
}


scaled UWAI::Player::warConfidenceLearned(PlayerTypes eTarget, bool bIgnoreDefOnly) const
{

	scaled rFromWarSuccess = confidenceFromWarSuccess(TEAMID(eTarget));
	scaled rFromPastWars = confidenceFromPastWars(TEAMID(eTarget));
	if (bIgnoreDefOnly == (rFromPastWars > 1))
		rFromPastWars = 1;
	scaled r = 1;
	if (rFromWarSuccess > 0)
		r += rFromWarSuccess - 1;
	/*  Very high/low rFromWarSuccess suggests relatively high reliability
		(long war and high total war successes); disregard the experience
		from past wars in this case. */
	if (rFromPastWars > 0 && r > fixp(0.6) && r < fixp(1.4))
		r += rFromPastWars - 1;
	r.decreaseTo(fixp(1.5));
	return r;
	// Tbd.: Consider using statistics (e.g. units killed/ lost) in addition
}


scaled UWAI::Player::warConfidenceAllies() const
{
	// AI assumes that humans have normal confidence
	if (GET_PLAYER(m_eAgent).isHuman())
		return fixp(0.8);
	int const iDogpileWarRand = GC.getInfo(GET_PLAYER(m_eAgent).
			getPersonalityType()).getDogpileWarRand();
	if (iDogpileWarRand <= 0)
		return 0;
	/*	iDogpileWarRand is between 20 (DeGaulle, high confidence) and
		150 (Lincoln, low confidence). These values are too far apart to convert
		them proportionally. Hence the square root. The result is normally between
		1 and 0.23. */
	scaled r(30, iDogpileWarRand);
	r = r.sqrt() - fixp(0.22);
	r.clamp(0, fixp(1.8));
	/*  Should have much greater confidence in civs on our team, but
		can't tell in this function who the ally is. Hard to rewrite
		InvasionGraph such that each ally is evaluated individually; wasn't
		written with team games in mind. As a kludge, just generally
		increase confidence when part of a team: */
	if (GET_TEAM(m_eAgent).getNumMembers() > 1)
	{
		scaled rConfTeam = r * 2;
		rConfTeam.clamp(fixp(0.6), fixp(1.2));
		r.increaseTo(rConfTeam);
	}
	return r;
}

// (See comment in header)
//scaled UWAI::Player::confidenceAgainstHuman() const {
	/*  How hesitant should the AI be to engage humans?
		This will have to be set based on experimentation. 90% is moderate
		discouragement against wars vs. humans. Perhaps unneeded, perhaps needs to
		be lower than 90%. Could set it based on the difficulty setting, however,
		while a Settler player can be expected to be easy prey, the AI arguably
		shouldn't exploit this, and while a Deity player will be difficult to
		defeat, the AI should arguably still try.
		The learning-which-civs-are-dangerous approach in warConfidenceLearned
		is more elgant, but won't prevent an AI-on-human dogpile in the early game. */
	//return (GET_PLAYER(m_eAgent).isHuman() ? 1 : fixp(0.9));
//}


int UWAI::Player::vengefulness() const
{
	CvPlayerAI const& kAgent = GET_PLAYER(m_eAgent);
	/*	AI assumes that humans are mostly calculating.
		But player feedback has shown that most humans are at least
		a little bit vengeful, casual players a bit more so.
		On the bottom line, this is relevant mainly for the reparations
		that the AI is willing to pay. */
	if (kAgent.isHuman())
	{
		if (GC.getGame().isOption(GAMEOPTION_RISE_FALL))
			return 1; // Difficulty not so telling in R&F
		int const iDifficulty = GC.getInfo(kAgent.getHandicapType()).getDifficulty();
		if (iDifficulty >= 50)
			return 1;
		if (iDifficulty > 20)
			return 2;
		return 3;
	}
	/*  RefuseToTalkWarThreshold loses its original meaning because UWAI
		doesn't sulk. It fits pretty well for carrying a grudge. Sitting Bull
		has the highest value (12) and De Gaulle the lowest (5).
		BasePeaceWeight (between 0 and 10) now has a dual use; continues to be
		used for inter-AI diplo modifiers. */
	CvLeaderHeadInfo const& kPersonality = GC.getInfo(kAgent.getPersonalityType());
	return std::max(0, kPersonality.getRefuseToTalkWarThreshold()
			- kPersonality.getBasePeaceWeight());
}


scaled UWAI::Player::protectiveInstinct() const
{
	if (GET_PLAYER(m_eAgent).isHuman())
		return 1;
	/*  DogPileWarRand is not a good indicator; that's more about backstabbing.
		Willingness to sign DP makes some sense. E.g. Roosevelt and the
		Persian leaders do that at Cautious, while Pleased is generally more common.
		Subtract WarMongerRespect to sort out the ones that just like DP
		because they're fearful, e.g. Boudica or de Gaulle. */
	CvLeaderHeadInfo const& kPersonality = GC.getInfo(GET_PLAYER(m_eAgent).
			getPersonalityType());
	int iDPVal = 2 * (ATTITUDE_FRIENDLY - kPersonality.
			getDefensivePactRefuseAttitudeThreshold());
	int iWarmongerRespect = kPersonality.getWarmongerRespect();
	return fixp(0.9) + scaled(iDPVal - SQR(iWarmongerRespect), 10);
}


scaled UWAI::Player::diploWeight() const
{
	if (GET_PLAYER(m_eAgent).isHuman())
		return 0;
	CvLeaderHeadInfo const& kPersonality = GC.getInfo(GET_PLAYER(m_eAgent).
			getPersonalityType());
	int const iCR = kPersonality.getContactRand(CONTACT_TRADE_TECH);
	if (iCR <= 0 || iCR > 15)
		return fixp(0.25);
	if (iCR <= 1)
		return fixp(1.75);
	if (iCR <= 3)
		return fixp(1.5);
	if (iCR <= 7)
		return 1;
	return fixp(0.5);
}


scaled UWAI::Player::prideRating(int iMakePeaceRand) const
{
	if (GET_PLAYER(m_eAgent).isHuman())
		return 0;
	if (iMakePeaceRand < 0)
	{
		iMakePeaceRand = GC.getInfo(GET_PLAYER(m_eAgent).getPersonalityType()).
				getMakePeaceRand();
	}
	scaled r(iMakePeaceRand, 110);
	r -= fixp(0.09);
	r.clamp(0, 1);
	return r;
}
