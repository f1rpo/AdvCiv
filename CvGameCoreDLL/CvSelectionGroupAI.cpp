#include "CvGameCoreDLL.h"
#include "CvGame.h" // <!-- custom: WAR_ATTACK_ORDER diagnostics need CvGame::getGameTurn for unit-age logging. (GPT-5.6 Thinking) -->
#include "CvSelectionGroupAI.h"
#include "CvUnitAI.h"
// <advc.004c> for AI_bestUnitForMission
#include "CvUnit.h"
#include "CvCityAI.h" // </advc.004c>
#include "CvPlayerAI.h"
#include "CvTeamAI.h"
#include "AgentIterator.h"
#include "CvMap.h"
#include "BBAILog.h" // <!-- custom: WAR_ATTACK_ORDER diagnostics log the selected and final stack attacker. (GPT-5.6 Thinking) -->


CvSelectionGroupAI::CvSelectionGroupAI()
{
	AI_reset();
}


CvSelectionGroupAI::~CvSelectionGroupAI()
{
	AI_uninit();
}


void CvSelectionGroupAI::AI_init()
{
	AI_reset();
}


void CvSelectionGroupAI::AI_uninit() {}


void CvSelectionGroupAI::AI_reset()
{
	AI_uninit();

	m_iMissionAIX = INVALID_PLOT_COORD;
	m_iMissionAIY = INVALID_PLOT_COORD;

	m_bForceSeparate = false;

	m_eMissionAIType = NO_MISSIONAI;

	m_missionAIUnit.reset();

	m_bGroupAttack = false;
	m_iGroupAttackX = -1;
	m_iGroupAttackY = -1;
}

// these separate function have been tweaked by K-Mod and bbai.
void CvSelectionGroupAI::AI_separate()
{
	FOR_EACH_UNIT_VAR_IN(pUnit, *this)
	{
		pUnit->joinGroup(NULL);
	}
}


void CvSelectionGroupAI::AI_separateNonAI(UnitAITypes eUnitAI)
{
	FOR_EACH_UNIT_VAR_IN(pUnit, *this)
	{
		if (pUnit->AI_getUnitAIType() != eUnitAI)
			pUnit->joinGroup(NULL);
	}
}


void CvSelectionGroupAI::AI_separateAI(UnitAITypes eUnitAI)
{
	FOR_EACH_UNIT_VAR_IN(pUnit, *this)
	{
		if (pUnit->AI_getUnitAIType() == eUnitAI)
			pUnit->joinGroup(NULL);
	}
}


bool CvSelectionGroupAI::AI_separateImpassable()
{
	CvPlayerAI& kPlayer = GET_PLAYER(getOwner());
	bool bSeparated = false;
	FOR_EACH_UNIT_VAR_IN(pUnit, *this)
	{
		if (kPlayer.AI_isAnyImpassable(pUnit->getUnitType()))
		{
			pUnit->joinGroup(NULL);
			bSeparated = true;
		}
	}
	return bSeparated;
}


bool CvSelectionGroupAI::AI_separateEmptyTransports()
{
	bool bSeparated = false;
	FOR_EACH_UNIT_VAR_IN(pUnit, *this)
	{
		if (pUnit->AI_getUnitAIType() == UNITAI_ASSAULT_SEA &&
			!pUnit->hasCargo())
		{
			pUnit->joinGroup(NULL);
			bSeparated = true;
		}
	}
	return bSeparated;
} // bbai / K-Mod end

// Returns true if the group has become busy
bool CvSelectionGroupAI::AI_update()
{
	PROFILE_FUNC();

	FAssert(getOwner() != NO_PLAYER);

	if (!isAIControlled())
		return false;

	if (getNumUnits() == 0)
		return false;

	// <!-- custom: K-Mod 846's sole force-update producer already cancels every old queued attack at turn start.
	// If a group attack now coexists with that marker, an amphibious command queued it later and must take precedence.
	// Remove K-Mod 846's redundant entry cancellation so the loop below clears the stale marker while preserving the continuation. See KI#535. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->

	//FAssert(!(GET_PLAYER(getOwner()).isAutoMoves())); // (no longer true in K-Mod)

	//int iTempHack = 0; // XXX
	// <advc.001y> Will keep this permanently as a fallback
	int iAttempts = 0;
	int iMaxAttempts = 6 * (GET_PLAYER(getOwner()).getCurrentEra() + 1) +
			std::max(getNumUnits(), 4);
#ifdef _DEBUG
	iMaxAttempts += 4; // Extra iterations for debugging
#endif
	// </advc.001y>
	bool bDead = false;
	bool bFailedAlreadyFighting = false;
	//while ((m_bGroupAttack && !bFailedAlreadyFighting) || readyToMove())
	while ((AI_isGroupAttack() && !isBusy()) || readyToMove()) // K-Mod
	{
		// K-Mod. Force update just means we should get into this loop at least once.
		setForceUpdate(false);
		iAttempts++;
		/*  <advc.001y> Moved out of the block below so I can see what the loop does
			before it terminates. Debugger stops in CvSelectionGroup::pushMission,
			startMission and in CvUnitAI::AI_update have been helpful to me. */

		// <!-- custom: The SAS no-progress tripwire fixed an inherited Scout/Explorer spin, but originally compared only head geometry/moves and queue length.
		// Snapshot head identity and role too, because legitimate AI_setUnitAIType transitions detach the acting head and leave another same-tile unit in this group. See KI#319. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		CvUnitAI const* pHeadBefore = AI_getHeadUnit();
		const int iHeadIDBefore = (pHeadBefore == NULL ? -1 : pHeadBefore->getID());
		const UnitAITypes eHeadAIBefore = (pHeadBefore == NULL ? NO_UNITAI : pHeadBefore->AI_getUnitAIType());
		const int iHeadXBefore = (pHeadBefore == NULL ? -1 : pHeadBefore->getX());
		const int iHeadYBefore = (pHeadBefore == NULL ? -1 : pHeadBefore->getY());
		const int iHeadMovesBefore = (pHeadBefore == NULL ? -1 : pHeadBefore->movesLeft());
		const int iMissionQueueLengthBefore = getLengthMissionQueue();
		// <!-- custom: Missionless transport unloading changes group cargo, and AdvCiv can detach a non-head bombard unit while leaving every existing head snapshot unchanged. Both are real progress that must re-enter inherited group evaluation instead of receiving MISSION_SKIP. See KI#522 and KI#524. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		const int iNumUnitsBefore = getNumUnits();
		const int iCargoBefore = getCargo();
		// <!-- custom: Consuming a queued group attack and spending a non-head member's movement are also legitimate progress while every head-centric field can remain unchanged.
		// Snapshot both so the KI#319 fallback does not suppress inherited same-turn reassessment. See KI#525 and KI#526. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		const bool bGroupAttackBefore = AI_isGroupAttack();
		int iGroupMovesBefore = 0;
		FOR_EACH_UNIT_IN(pLoopUnit, *this)
			iGroupMovesBefore += pLoopUnit->movesLeft();
	#ifdef _DEBUG
		iMaxAttempts -= 4; // Trigger assert early
	#endif
		// <!-- custom: Keep the richer stuck-group diagnostic lazy, but preserve it in every assertion-enabled build rather than `_DEBUG` alone. The early trigger and restoration remain separate because only Debug lowers the attempt limit. See KI#596. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		// FAssertMsg(iAttempts != iMaxAttempts, "Unit stuck in a loop");
	#ifdef FASSERT_ENABLE
		const bool bAssertCondition = (iAttempts != iMaxAttempts);
		if (!bAssertCondition)
		{
			CvUnitAI const* u = AI_getHeadUnit();
			int x = -1, y = -1, moves = -1, uid = -1, uai = -1, dom = -1;
			const wchar* udesc = L"NONE";
			if (u)
			{
				x    = u->getX(); 
				y    = u->getY();
				moves= u->movesLeft();
				uid  = u->getID();
				uai  = u->AI_getUnitAIType();
				dom  = u->getDomainType();
				udesc= GC.getUnitInfo(u->getUnitType()).getDescription();
			}
			FAssertMsg(bAssertCondition, CvString::format(
				"Unit stuck in a loop | grp=%d owner=%d at=(%d,%d) unit=%S id=%d AI=%d dom=%d "
				"moves=%d ready=%d busy=%d queue=%d forceUpd=%d grpAtk=%d forceSep=%d attempts=%d max=%d",
				getID(), getOwner(), x, y, udesc, uid, uai, dom, moves,
				(int)readyToMove(), (int)isBusy(), getLengthMissionQueue(),
				(int)isForceUpdate(), (int)AI_isGroupAttack(), (int)AI_isForceSeparate(),
				iAttempts, iMaxAttempts
			).c_str());
			// <!-- custom: now we have more info thanks chatgpt 5 and thanks to me too i guess i mean if i may say as well -->
			// Assert Failed
			// File:  ..\.\CvSelectionGroupAI.cpp
			// Line:  180
			// Func:  CvSelectionGroupAI::AI_update
			// Expression:  iAttempts != iMaxAttempts
			// Message:  Unit stuck in a loop | grp=73732 owner=8 at=(61,28) unit=Scout id=57348 AI=13 dom=2 moves=120 ready=1 busy=0 queue=0 forceUpd=0 grpAtk=0 forceSep=0 attempts=16 max=16
			//
			// <!-- custom: note: happens many times after first one, with various owner's units, but udesc is seemingly always a scout unit in all of these. -->
			//
		}
	#endif
	#ifdef _DEBUG
		iMaxAttempts += 4; // Restore extra iterations
	#endif
		if (iAttempts >= iMaxAttempts) // was > 100 </advc.001y>
		{
			CvUnit* pHeadUnit = getHeadUnit();
			if (pHeadUnit != NULL)
			{	// <advc.001y>
			#ifndef _DEBUG
				if (iAttempts == iMaxAttempts) // Don't spam the log </advc.004y>
					GC.getLogger().logUnitStuck(*pHeadUnit); // advc.003t
			#endif
				pHeadUnit->finishMoves();
			}
			break;
		}

		// if we want to force the group to attack, force another attack
		if (AI_isGroupAttack())
		{
			AI_cancelGroupAttack();
			groupAttack(m_iGroupAttackX, m_iGroupAttackY,
					MOVE_DIRECT_ATTACK, bFailedAlreadyFighting);
		}
		// else pick AI action
		else
		{
			CvUnitAI* pHeadUnit = AI_getHeadUnit();
			//if (pHeadUnit == NULL || pHeadUnit->isDelayedDeath())
			if (pHeadUnit == NULL || pHeadUnit->doDelayedDeath()) // K-Mod
				break;

			//resetPath();
			if (pHeadUnit->AI_update())
			{	// AI_update returns true when we should abort the loop and wait until next slice
				FAssert(!pHeadUnit->isDelayedDeath());
				break;
			}
		}

		// <!-- custom: Consume the turn only when the same head unit and group made no observable progress.
		// Head replacement, role conversion, membership/cargo change, queued-attack consumption or any member spending movement must let the remaining group continue its normal update. See KI#319, KI#522, KI#524, KI#525 and KI#526. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		CvUnitAI const* pHeadAfter = AI_getHeadUnit();
		int iGroupMovesAfter = 0;
		FOR_EACH_UNIT_IN(pLoopUnit, *this)
			iGroupMovesAfter += pLoopUnit->movesLeft();
		if (!isBusy() && readyToMove(true) && pHeadAfter != NULL &&
			pHeadAfter->getID() == iHeadIDBefore && pHeadAfter->AI_getUnitAIType() == eHeadAIBefore &&
			pHeadAfter->getX() == iHeadXBefore && pHeadAfter->getY() == iHeadYBefore &&
			pHeadAfter->movesLeft() == iHeadMovesBefore && getLengthMissionQueue() == iMissionQueueLengthBefore &&
			getNumUnits() == iNumUnitsBefore && getCargo() == iCargoBefore &&
			AI_isGroupAttack() == bGroupAttackBefore && iGroupMovesAfter == iGroupMovesBefore)
		{
			pushMission(MISSION_SKIP); // finishes moves cleanly
			break;
		}

		if (doDelayedDeath())
		{
			bDead = true;
			break;
		}

		/*	if no longer group attacking, and force separate is true,
			then bail, decide what to do after group is split up */
		// (UnitAI of head unit may have changed)
		if (!AI_isGroupAttack() && AI_isForceSeparate())
		{
			AI_separate();	// pointers could become invalid...
			//return true;
			return false; // K-Mod
		}
	}

	if (!bDead)
	{
		// K-Mod. this is how we deal with force update when some group members can't move.
		if (isForceUpdate())
		{
			setForceUpdate(false);
			AI_cancelGroupAttack();
			setActivityType(ACTIVITY_AWAKE);
		}
		// K-Mod end
		if (!isHuman())
		{
			bool bFollow = false;
			// <k146>
			// if we're not group attacking, then check for 'follow' action
			if (!AI_isGroupAttack() && readyToMove(true))
			{
				/*  What we do here might split the group. So to avoid problems,
					lets make a list of our units. */
				std::vector<IDInfo> originalGroup;
				for(CLLNode<IDInfo> const* pUnitNode = headUnitNode(); pUnitNode != NULL;
					pUnitNode = nextUnitNode(pUnitNode))
 				{
					originalGroup.push_back(pUnitNode->m_data);
				}
				FAssert(originalGroup.size() == getNumUnits());
				bool bFirst = true;
				resetPath();
				for (std::vector<IDInfo>::iterator it = originalGroup.begin();
					it != originalGroup.end(); ++it)
				{
					CvUnitAI* pLoopUnit = ::AI_getUnit(*it);
					if (pLoopUnit && pLoopUnit->getGroupID() == getID() &&
						pLoopUnit->canMove())
					{
						if (pLoopUnit->AI_follow(bFirst))
						{
							bFollow = true;
							bFirst = true; // let the next unit start fresh.
							resetPath();
							if (!readyToMove(true))
								break;
						}
						else bFirst = false;
					}
				} // </k146>
			}

			if (doDelayedDeath())
				bDead = true;

			if (!bDead)
			{
				if (!bFollow && readyToMove(true))
					pushMission(MISSION_SKIP);
			}
		}
	}
	// <advc.test>
	/*if(GC.getRandLogging() && !GC.getGame().checkInSync()) {
		CvUnit* pHeadUnit = getHeadUnit(); // for inspection in debugger
		FAssert(false);
	}*/ // </advc.test>

	if (bDead)
	{	//return true;
		return false; // K-Mod
	}

	return (isBusy() || isCargoBusy());
}

// Returns attack odds out of 100 (the higher, the better...)
int CvSelectionGroupAI::AI_attackOdds(const CvPlot* pPlot, bool bPotentialEnemy) const
{
	PROFILE_FUNC();

	FAssert(getOwner() != NO_PLAYER);
	//if (pPlot->getBestDefender(NO_PLAYER, getOwner(), NULL, !bPotentialEnemy, bPotentialEnemy) == NULL)
	// BETTER_BTS_AI_MOD, Efficiency, Lead From Behind (UncutDragon), 02/21/10, jdog5000:
	if (!pPlot->hasDefender(false, NO_PLAYER, getOwner(),
		NULL, !bPotentialEnemy, bPotentialEnemy))
	{
		return 100;
	}
	int iOdds=-1; // (advc: Was 0. Shouldn't matter.)
	CvUnit* pAttacker = AI_getBestGroupAttacker(pPlot, bPotentialEnemy, iOdds);
	if (pAttacker == NULL)
		return 0;

	return iOdds;
}

/*	K-Mod. A new odds-adjusting function to replace CvUnitAI::AI_finalOddsThreshold.
	(note: I would like to put this in CvSelectionGroupAI ... but - well -
	I don't need to say it, right?)
	advc.003u: I think CvUnitAI::AI_getGroup solves karadoc's problem; so - moved. */
int CvSelectionGroupAI::AI_getWeightedOdds(CvPlot const* pPlot, bool bPotentialEnemy)
{
	PROFILE_FUNC();
	int iOdds=-1;
	CvUnitAI const* pAttacker = AI_getBestGroupAttacker(pPlot, bPotentialEnemy, iOdds);
	if (pAttacker == NULL)
		return 0;
	CvPlot::DefenderFilters defFilters(getOwner(), pAttacker,
			!bPotentialEnemy, bPotentialEnemy,
			true, false); // advc.028, advc.089 (same as in CvUnitAI::AI_attackOdds)
	CvUnit const* pDefender = pPlot->getBestDefender(NO_PLAYER, defFilters);
	if (pDefender == NULL)
		return 100;

	/*	<advc.114b> We shouldn't adjust the odds based on an optimistic estimate
		(increased by AttackOddsChange) b/c that leads to Warriors attacking Tanks -
		high difference in production cost and non-negligible optimistic odds.
		I'm subtracting the AttackOddsChange temporarily;
		adding them back in after the adjustments are done.
		(A more elaborate fix would avoid adding them in the first place.) */
	int const iAttackOddsChange = GET_PLAYER(getOwner()).AI_getAttackOddsChange();
	// <!-- custom: Subtraction remains exact away from the public clamp boundaries. At 1 or 99, recompute the selected attacker's unadjusted odds because the clamp may have discarded a true 0% or 100% endpoint.
	// This implements AdvCiv's suggested elaborate fix without repeating combat-odds work for ordinary values. See KI#523. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (iOdds <= 1 || iOdds >= 99)
		iOdds = pAttacker->AI_attackOdds(pPlot, bPotentialEnemy, true);
	else iOdds -= iAttackOddsChange;
	/*	Require a stack of at least 3 if actual odds are below 1%. Should
		matter mostly for Barbarians, hence only this primitive condition
		(not checking if the other units could actually attack etc.). */
	if(iOdds == 0 && getNumUnits() < 3)
		return 0;
	// </advc.114b>
	// advc: The bulk of the computation is still in CvUnitAI
	int iAdjustedOdds = pAttacker->AI_opportuneOdds(iOdds, *pDefender);

	/*  one more thing... unfortunately, the sea AI isn't evolved enough
		to do without something as painful as this... */
	if (getDomainType() == DOMAIN_SEA && !hasCargo())
	{
		// I'm sorry about this. I really am. I'll try to make it better one day...
		int iDefenders = pAttacker->AI_countEnemyDefenders(*pPlot);
		iAdjustedOdds *= 2 + getNumUnits();
		iAdjustedOdds /= 3 + std::min(iDefenders / 2, getNumUnits());
	}

	iAdjustedOdds += iAttackOddsChange; // advc.114b
	return range(iAdjustedOdds, 1, 99);
}


// <advc.048> <!-- custom: hoisted from multiline signature between `bNoBlitz` and `bSacrifice` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
CvUnitAI* CvSelectionGroupAI::AI_getBestGroupAttacker(const CvPlot* pPlot, bool bPotentialEnemy, int& iUnitOdds, bool bForce, bool bNoBlitz, bool bSacrifice, bool bMaxSurvival, bool bPreferLowPower) const
{
	int const iOddsThresh = 68; // Should this be lower if bHuman?
	FAssert(!bMaxSurvival || !bSacrifice); // </advc.048>
	PROFILE_FUNC();

	int iBestValue = 0;
	int iBestOdds = 0;
	int iBestXP = 0;
	int iBestHealthRank = 0;
	CvUnitAI* pBestUnit = NULL;
	CLLNode<IDInfo> const* pUnitNode = headUnitNode();
	bool const bHuman = (pUnitNode == NULL ? true :
			GET_PLAYER(::getUnit(pUnitNode->m_data)->getOwner()).isHuman());
	static const bool bSAS_AI_GETBESTGROUPATTACKER_LOW_POWER_ATTACK_ORDER_OPTIMIZE = GC.getDefineBOOL("SAS_AI_GETBESTGROUPATTACKER_LOW_POWER_ATTACK_ORDER_OPTIMIZE");
	// <!-- custom: SAS's low-power rank uses ordinary land/sea effective strength, while aircraft store their real strength in iAirCombat.
	// Keep aircraft on the air-aware selection path below instead of asserting in Debug or collapsing their rank to 1 in Release. See KI#532. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	bool const bUseLowPower = (bPreferLowPower && !bHuman && getDomainType() != DOMAIN_AIR && bSAS_AI_GETBESTGROUPATTACKER_LOW_POWER_ATTACK_ORDER_OPTIMIZE);
	if (bUseLowPower)
	{
		iBestValue = 1 << 30;
		iBestXP = 1 << 30;
		iBestHealthRank = 1 << 30;
	}
	FAssert(!bMaxSurvival || bHuman); // advc.048
	while (pUnitNode != NULL)
	{
		CvUnitAI& kLoopUnit = *::AI_getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		if (kLoopUnit.isDead())
			continue;

		bool bCanAttack = false;
		if (kLoopUnit.getDomainType() == DOMAIN_AIR)
			bCanAttack = kLoopUnit.canAirAttack();
		else
		{
			bCanAttack = kLoopUnit.canAttack();
			if (bCanAttack && bNoBlitz && kLoopUnit.isBlitz() &&
				kLoopUnit.isMadeAttack())
			{
				bCanAttack = false;
			}
		}
		if (!bCanAttack || (!bForce && !kLoopUnit.canMove()))
			continue;

		if (!bForce && !kLoopUnit.canMoveInto(*pPlot, true, bPotentialEnemy))
			continue;

		// <!-- custom: For AI stack attacks, spend expendable units first to preserve elite finishers.
		// This is economically efficient: older/weaker units cost upkeep but scale poorly, while elite units are costly to lose
		// and can secure the fight if early attacks go badly; keeping them as finishers preserves flexibility and escape odds.
		// Once bombard is done and we have decided to attack, siege/collateral units go first because they are less useful on defense
		// and have already contributed their main value; this also front-loads collateral damage to soften the defenders.
		// Order by lowest effective power, then lowest XP; among healthy units (>= SAS_*_MIN_HEALTH_PERCENT), lower health first. (GPT-5.2-Codex) -->
		if (bUseLowPower)
		{
			int const iOdds = kLoopUnit.AI_attackOdds(pPlot, bPotentialEnemy);
			bool const bCollateral = (kLoopUnit.collateralDamage() > 0);
			int iBaseCollateral = (bCollateral ? estimateCollateralWeight(pPlot, getTeam()) : 0);
			int const iEffectiveStr = kLoopUnit.AI_currEffectiveStr(pPlot, &kLoopUnit,
					bCollateral, iBaseCollateral, true);
			int iValue = iEffectiveStr;
			if (kLoopUnit.bombardRate() > 0 || bCollateral)
				iValue -= 1000000;
			int const iXP = kLoopUnit.getExperience();
			int const iMaxHP = std::max(1, kLoopUnit.maxHitPoints());
			static const int iSAS_AI_GETBESTGROUPATTACKER_LOW_POWER_ATTACK_ORDER_MIN_HEALTH_PERCENT = GC.getDefineINT("SAS_AI_GETBESTGROUPATTACKER_LOW_POWER_ATTACK_ORDER_MIN_HEALTH_PERCENT");
			int const iHealthPercent = (100 * (iMaxHP - kLoopUnit.getDamage())) / iMaxHP;
			int iHealthRank = (iHealthPercent >= iSAS_AI_GETBESTGROUPATTACKER_LOW_POWER_ATTACK_ORDER_MIN_HEALTH_PERCENT
					? iHealthPercent : 100);
			if (pBestUnit == NULL || iValue < iBestValue ||
				(iValue == iBestValue && iXP < iBestXP) ||
				(iValue == iBestValue && iXP == iBestXP && iHealthRank < iBestHealthRank))
			{
				iBestValue = iValue;
				iBestOdds = iOdds;
				iBestXP = iXP;
				iBestHealthRank = iHealthRank;
				pBestUnit = &kLoopUnit;
			}
			continue;
		}

		// BETTER_BTS_AI_MOD, Lead From Behind (UncutDragon), 02/21/10, jdog5000: START
		// <!-- custom: Inherited LFB constructs ordinary-combat ranks and therefore treats iAirCombat aircraft as zero-strength attackers.
		// Route aircraft through the existing air-aware AI_attackOdds branch while preserving LFB for land and sea units. See KI#533. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if (kLoopUnit.getDomainType() != DOMAIN_AIR &&
			GC.getDefineBOOL(CvGlobals::LFB_ENABLE) &&
			GC.getDefineBOOL(CvGlobals::LFB_USECOMBATODDS) &&
			!bMaxSurvival) // advc.048
		{
			kLoopUnit.LFBgetBetterAttacker(&pBestUnit, pPlot, bPotentialEnemy, iBestOdds,
					iBestValue); // K-Mod.
		}
		else
		{
			int iOdds = kLoopUnit.AI_attackOdds(pPlot, bPotentialEnemy);
			int iValue = iOdds;
			FAssert(iValue > 0);
			if (kLoopUnit.collateralDamage() > 0 && /* advc.048: */ !bMaxSurvival)
			{
				int iPossibleTargets = std::min(
						pPlot->getNumVisibleEnemyDefenders(&kLoopUnit) - 1,
						kLoopUnit.collateralDamageMaxUnits());
				if (iPossibleTargets > 0)
				{
					iValue *= 100 + ((kLoopUnit.//collateralDamage()
							AI_collateralDmgFactor() * // advc.159
							iPossibleTargets)) / 5;
					iValue /= 100;
				}
			}
			/*  if non-human, prefer the last unit that has the best value
				(so as to avoid splitting the group) */
			if (iValue > iBestValue ||
				(!bHuman && iValue > 0 && iValue == iBestValue) ||
				/*  <advc.048> For human, use sacrifice value to break ties
					in order to match the choice made in the !bMaxSurvival branch above
					and the bSacrifice branch below. */
				(bHuman && iValue == iBestValue &&
				(pBestUnit == NULL || kLoopUnit.AI_sacrificeValue(pPlot) >
				pBestUnit->AI_sacrificeValue(pPlot)))) // </advc.048>
			{
				iBestValue = iValue;
				iBestOdds = iOdds;
				pBestUnit = &kLoopUnit;
			}
		}
		// BETTER_BTS_AI_MOD: END
	}
	CvUnitAI* const pLowPowerSelected = (bUseLowPower ? pBestUnit : NULL);
	int const iLowPowerSelectedOdds = (bUseLowPower ? iBestOdds : -1);
	CvUnitAI* pInheritedBestSacrifice = NULL;
	CvUnitAI* pSASObsoleteSacrifice = NULL;
	int iInheritedBestSacrificeOdds = -1;
	int iSASObsoleteSacrificeOdds = -1;
	iUnitOdds = iBestOdds;
	// <advc.048> Cut from CvSelectionGroup::groupAttack
	if(bSacrifice && iUnitOdds < iOddsThresh)
	{
		CvUnitAI* pBestSacrifice = AI_getBestGroupSacrifice(pPlot,
				bPotentialEnemy, bForce, /* advc.164: */ bNoBlitz);
		pInheritedBestSacrifice = pBestSacrifice;
		if(pBestSacrifice != NULL)
		{
			// <!-- custom: When the inherited sacrifice choice would spend a current unit at risky odds, prefer an obsolete sacrifice that retains enough inherited tactical value. Obsolete units remain useful as deliberate expenditures: losing them reduces support/upgrade pressure while preserving current units for future wars.
			// A current unit with very high odds is allowed to attack because it is unlikely to be lost; once the current choice is itself in sacrifice territory, the normal odds-deficit limit can be bypassed and sacrifice value/tactical role become the safeguards. Do not protect obsolete units merely for XP. See KI#200. (GPT-5.6 Thinking) -->
			static const bool bSAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_OPTIMIZE = GC.getDefineBOOL("SAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_OPTIMIZE");
			static const int iSAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_CURRENT_SAFE_ODDS_PERCENT = GC.getDefineINT("SAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_CURRENT_SAFE_ODDS_PERCENT");
			static const int iSAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_ODDS_DEFICIT_BYPASS_MAX_CURRENT_ODDS_PERCENT = GC.getDefineINT("SAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_ODDS_DEFICIT_BYPASS_MAX_CURRENT_ODDS_PERCENT");
			static const int iSAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_MAX_ODDS_DEFICIT_PERCENT = GC.getDefineINT("SAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_MAX_ODDS_DEFICIT_PERCENT");
			static const int iSAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_MIN_VALUE_PERCENT_OF_CURRENT = GC.getDefineINT("SAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_MIN_VALUE_PERCENT_OF_CURRENT");
			iInheritedBestSacrificeOdds = pBestSacrifice->AI_attackOdds(pPlot, bPotentialEnemy);
			TechTypes const eInheritedObsoleteTech = pBestSacrifice->getUnitInfo().getObsoleteTech();
			bool const bInheritedObsolete = (eInheritedObsoleteTech != NO_TECH && GET_TEAM(pBestSacrifice->getTeam()).isHasTech(eInheritedObsoleteTech));
			if (bUseLowPower && bSAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_OPTIMIZE && !bInheritedObsolete && iInheritedBestSacrificeOdds < iSAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_CURRENT_SAFE_ODDS_PERCENT)
			{
				int const iInheritedSacrificeValue = pBestSacrifice->AI_sacrificeValue(pPlot);
				bool const bInheritedTacticalSacrifice = (pBestSacrifice->bombardRate() > 0 || pBestSacrifice->collateralDamage() > 0);
				int iBestObsoleteSacrificeValue = -1;
				for (CLLNode<IDInfo> const* pObsoleteNode = headUnitNode(); pObsoleteNode != NULL; pObsoleteNode = nextUnitNode(pObsoleteNode))
				{
					CvUnitAI* pObsoleteUnit = ::AI_getUnit(pObsoleteNode->m_data);
					if (pObsoleteUnit->isDead()) continue;
					bool bCanAttack = (pObsoleteUnit->getDomainType() == DOMAIN_AIR ? pObsoleteUnit->canAirAttack() : pObsoleteUnit->canAttack());
					if (bCanAttack && pObsoleteUnit->getDomainType() != DOMAIN_AIR && bNoBlitz && pObsoleteUnit->isBlitz() && pObsoleteUnit->isMadeAttack()) bCanAttack = false;
					if (!bCanAttack || (!bForce && !pObsoleteUnit->canMove()) || (!bForce && !pObsoleteUnit->canMoveInto(*pPlot, true))) continue;
					TechTypes const eObsoleteTech = pObsoleteUnit->getUnitInfo().getObsoleteTech();
					if (eObsoleteTech == NO_TECH || !GET_TEAM(pObsoleteUnit->getTeam()).isHasTech(eObsoleteTech)) continue;
					if (bInheritedTacticalSacrifice && pObsoleteUnit->bombardRate() <= 0 && pObsoleteUnit->collateralDamage() <= 0) continue;
					int const iObsoleteOdds = pObsoleteUnit->AI_attackOdds(pPlot, bPotentialEnemy);
					// <!-- custom: Once the inherited current unit is itself in sacrifice territory, do not reject obsolete candidates merely for having still lower victory odds; the sacrifice-value and tactical-role gates remain the quality safeguards. Above that risk threshold, retain the normal maximum odds-deficit limit. See KI#200. (GPT-5.6 Thinking) -->
					if (iInheritedBestSacrificeOdds > iSAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_ODDS_DEFICIT_BYPASS_MAX_CURRENT_ODDS_PERCENT && iObsoleteOdds + iSAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_MAX_ODDS_DEFICIT_PERCENT < iInheritedBestSacrificeOdds) continue;
					int const iObsoleteSacrificeValue = pObsoleteUnit->AI_sacrificeValue(pPlot);
					if (100 * iObsoleteSacrificeValue < iSAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_MIN_VALUE_PERCENT_OF_CURRENT * iInheritedSacrificeValue) continue;
					if (pSASObsoleteSacrifice == NULL || iObsoleteSacrificeValue >= iBestObsoleteSacrificeValue)
					{
						pSASObsoleteSacrifice = pObsoleteUnit;
						iSASObsoleteSacrificeOdds = iObsoleteOdds;
						iBestObsoleteSacrificeValue = iObsoleteSacrificeValue;
					}
				}
				if (pSASObsoleteSacrifice != NULL) pBestSacrifice = pSASObsoleteSacrifice;
			}
			pBestUnit = pBestSacrifice;
			/*  I.e. caller mustn't use these odds. Don't want to compute them here
				if the caller doesn't need them. */
			iUnitOdds = -1;
		}
	} // </advc.048>
	if (gWarLogLevel >= 3 && pSASObsoleteSacrifice != NULL && pInheritedBestSacrifice != NULL)
	{
		int const iOddsDeficitBypassMaxCurrentOdds = GC.getDefineINT("SAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_ODDS_DEFICIT_BYPASS_MAX_CURRENT_ODDS_PERCENT");
		logBBAI("WAR_ATTACK_ORDER_OBSOLETE_EXPEND_OVERRIDE turn=%d player=%d group=%d target=(%d,%d) inheritedUnit=%s inheritedUnitId=%d inheritedOdds=%d inheritedSacrificeValue=%d obsoleteUnit=%s obsoleteUnitId=%d obsoleteOdds=%d obsoleteSacrificeValue=%d currentSafeOdds=%d oddsDeficitBypassMaxCurrentOdds=%d oddsDeficitBypassed=%d maxOddsDeficit=%d minValuePercentOfCurrent=%d",
			GC.getGame().getGameTurn(), pSASObsoleteSacrifice->getOwner(), getID(), pPlot->getX(), pPlot->getY(),
			GC.getInfo(pInheritedBestSacrifice->getUnitType()).getType(), pInheritedBestSacrifice->getID(), iInheritedBestSacrificeOdds,
			pInheritedBestSacrifice->AI_sacrificeValue(pPlot), GC.getInfo(pSASObsoleteSacrifice->getUnitType()).getType(),
			pSASObsoleteSacrifice->getID(), iSASObsoleteSacrificeOdds, pSASObsoleteSacrifice->AI_sacrificeValue(pPlot),
			GC.getDefineINT("SAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_CURRENT_SAFE_ODDS_PERCENT"), iOddsDeficitBypassMaxCurrentOdds,
			(iInheritedBestSacrificeOdds <= iOddsDeficitBypassMaxCurrentOdds ? 1 : 0),
			GC.getDefineINT("SAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_MAX_ODDS_DEFICIT_PERCENT"),
			GC.getDefineINT("SAS_AI_GETBESTGROUPATTACKER_OBSOLETE_SACRIFICE_MIN_VALUE_PERCENT_OF_CURRENT"));
	}
	// <!-- custom: Diagnostic only: verify whether the low-power stack ordering spends old/obsolete units, whether the inherited sacrifice fallback replaces that choice, and when the SAS obsolete-expenditure refinement changes the inherited sacrifice.
	// Compare the exact best obsolete/non-obsolete sacrifice-value and attack-odds alternatives so tuning can remain evidence-based.
	// Age is intentionally logged rather than used for selection; the low-power stage still ranks bombard/collateral first, then low effective power, XP and health. Gate all extra diagnostic candidate scans behind WAR level 3. (GPT-5.6 Thinking) -->
	if (gWarLogLevel >= 3 && bUseLowPower && pLowPowerSelected != NULL && pBestUnit != NULL)
	{
		int iEligibleAttackers = 0;
		int iSacrificeEligibleAttackers = 0;
		int iObsoleteEligible = 0;
		int iObsoleteSacrificeEligible = 0;
		int iObsoleteSacrificeAtOrAboveOddsThreshold = 0;
		int iOldestAge = -1;
		CvUnitAI const* pOldestEligible = NULL;
		CvUnitAI const* pBestObsoleteSacrifice = NULL;
		CvUnitAI const* pBestNonObsoleteSacrifice = NULL;
		CvUnitAI const* pBestObsoleteOdds = NULL;
		CvUnitAI const* pBestNonObsoleteOdds = NULL;
		int iBestObsoleteSacrificeValue = -1;
		int iBestObsoleteSacrificeOdds = -1;
		int iBestNonObsoleteSacrificeValue = -1;
		int iBestNonObsoleteSacrificeOdds = -1;
		int iBestObsoleteOdds = -1;
		int iBestObsoleteOddsSacrificeValue = -1;
		int iBestNonObsoleteOdds = -1;
		int iBestNonObsoleteOddsSacrificeValue = -1;
		for (CLLNode<IDInfo> const* pLogNode = headUnitNode(); pLogNode != NULL; pLogNode = nextUnitNode(pLogNode))
		{
			CvUnitAI const& kLogUnit = *::AI_getUnit(pLogNode->m_data);
			if (kLogUnit.isDead()) continue;
			bool bCanAttack = (kLogUnit.getDomainType() == DOMAIN_AIR ? kLogUnit.canAirAttack() : kLogUnit.canAttack());
			if (bCanAttack && kLogUnit.getDomainType() != DOMAIN_AIR && bNoBlitz && kLogUnit.isBlitz() && kLogUnit.isMadeAttack()) bCanAttack = false;
			if (!bCanAttack || (!bForce && !kLogUnit.canMove()) || (!bForce && !kLogUnit.canMoveInto(*pPlot, true, bPotentialEnemy))) continue;
			iEligibleAttackers++;
			TechTypes const eObsoleteTech = kLogUnit.getUnitInfo().getObsoleteTech();
			bool const bObsolete = (eObsoleteTech != NO_TECH && GET_TEAM(kLogUnit.getTeam()).isHasTech(eObsoleteTech));
			if (bObsolete) iObsoleteEligible++;
			int const iAge = GC.getGame().getGameTurn() - kLogUnit.getGameTurnCreated();
			if (iAge > iOldestAge)
			{
				iOldestAge = iAge;
				pOldestEligible = &kLogUnit;
			}
			// Match AI_getBestGroupSacrifice's own canMoveInto test for the alternative-candidate summaries below.
			if (!bForce && !kLogUnit.canMoveInto(*pPlot, true)) continue;
			iSacrificeEligibleAttackers++;
			int const iSacrificeValue = kLogUnit.AI_sacrificeValue(pPlot);
			int const iOdds = kLogUnit.AI_attackOdds(pPlot, bPotentialEnemy);
			if (bObsolete)
			{
				iObsoleteSacrificeEligible++;
				if (iOdds >= iOddsThresh) iObsoleteSacrificeAtOrAboveOddsThreshold++;
				if (iSacrificeValue >= iBestObsoleteSacrificeValue)
				{
					pBestObsoleteSacrifice = &kLogUnit;
					iBestObsoleteSacrificeValue = iSacrificeValue;
					iBestObsoleteSacrificeOdds = iOdds;
				}
				if (iOdds > iBestObsoleteOdds || (iOdds == iBestObsoleteOdds && iSacrificeValue >= iBestObsoleteOddsSacrificeValue))
				{
					pBestObsoleteOdds = &kLogUnit;
					iBestObsoleteOdds = iOdds;
					iBestObsoleteOddsSacrificeValue = iSacrificeValue;
				}
			}
			else
			{
				if (iSacrificeValue >= iBestNonObsoleteSacrificeValue)
				{
					pBestNonObsoleteSacrifice = &kLogUnit;
					iBestNonObsoleteSacrificeValue = iSacrificeValue;
					iBestNonObsoleteSacrificeOdds = iOdds;
				}
				if (iOdds > iBestNonObsoleteOdds || (iOdds == iBestNonObsoleteOdds && iSacrificeValue >= iBestNonObsoleteOddsSacrificeValue))
				{
					pBestNonObsoleteOdds = &kLogUnit;
					iBestNonObsoleteOdds = iOdds;
					iBestNonObsoleteOddsSacrificeValue = iSacrificeValue;
				}
			}
		}
		int const iLowPowerHealth = (100 * (std::max(1, pLowPowerSelected->maxHitPoints()) - pLowPowerSelected->getDamage())) / std::max(1, pLowPowerSelected->maxHitPoints());
		int const iFinalHealth = (100 * (std::max(1, pBestUnit->maxHitPoints()) - pBestUnit->getDamage())) / std::max(1, pBestUnit->maxHitPoints());
		TechTypes const eLowPowerObsoleteTech = pLowPowerSelected->getUnitInfo().getObsoleteTech();
		TechTypes const eFinalObsoleteTech = pBestUnit->getUnitInfo().getObsoleteTech();
		bool const bLowPowerObsolete = (eLowPowerObsoleteTech != NO_TECH && GET_TEAM(pLowPowerSelected->getTeam()).isHasTech(eLowPowerObsoleteTech));
		bool const bFinalObsolete = (eFinalObsoleteTech != NO_TECH && GET_TEAM(pBestUnit->getTeam()).isHasTech(eFinalObsoleteTech));
		int const iFinalActualOdds = (pBestUnit == pLowPowerSelected ? iLowPowerSelectedOdds : pBestUnit->AI_attackOdds(pPlot, bPotentialEnemy));
		int const iLowPowerSacrificeValue = pLowPowerSelected->AI_sacrificeValue(pPlot);
		int const iFinalSacrificeValue = pBestUnit->AI_sacrificeValue(pPlot);
		logBBAI("WAR_ATTACK_ORDER turn=%d player=%d group=%d target=(%d,%d) eligible=%d obsoleteEligible=%d lowPowerUnit=%s lowPowerUnitId=%d lowPowerUnitAI=%d lowPowerAge=%d lowPowerXP=%d lowPowerHealth=%d lowPowerOdds=%d lowPowerSacrificeValue=%d lowPowerObsolete=%d finalUnit=%s finalUnitId=%d finalUnitAI=%d finalAge=%d finalXP=%d finalHealth=%d finalOdds=%d finalActualOdds=%d finalSacrificeValue=%d finalObsolete=%d sacrificeOverride=%d oldestUnit=%s oldestUnitAI=%d oldestAge=%d oldestXP=%d oldestObsolete=%d",
			GC.getGame().getGameTurn(), pBestUnit->getOwner(), getID(), pPlot->getX(), pPlot->getY(), iEligibleAttackers, iObsoleteEligible,
			GC.getInfo(pLowPowerSelected->getUnitType()).getType(), pLowPowerSelected->getID(), pLowPowerSelected->AI_getUnitAIType(),
			GC.getGame().getGameTurn() - pLowPowerSelected->getGameTurnCreated(), pLowPowerSelected->getExperience(), iLowPowerHealth,
			iLowPowerSelectedOdds, iLowPowerSacrificeValue, bLowPowerObsolete, GC.getInfo(pBestUnit->getUnitType()).getType(),
			pBestUnit->getID(), pBestUnit->AI_getUnitAIType(), GC.getGame().getGameTurn() - pBestUnit->getGameTurnCreated(),
			pBestUnit->getExperience(), iFinalHealth, iUnitOdds, iFinalActualOdds, iFinalSacrificeValue, bFinalObsolete,
			pBestUnit != pLowPowerSelected, (pOldestEligible == NULL ? "-" : GC.getInfo(pOldestEligible->getUnitType()).getType()),
			(pOldestEligible == NULL ? NO_UNITAI : pOldestEligible->AI_getUnitAIType()), iOldestAge,
			(pOldestEligible == NULL ? -1 : pOldestEligible->getExperience()),
			(pOldestEligible == NULL ? 0 :
				(pOldestEligible->getUnitInfo().getObsoleteTech() != NO_TECH &&
				GET_TEAM(pOldestEligible->getTeam()).isHasTech(pOldestEligible->getUnitInfo().getObsoleteTech()))));
		if (iObsoleteEligible > 0)
		{
			logBBAI("WAR_ATTACK_ORDER_CANDIDATE_SUMMARY turn=%d player=%d group=%d target=(%d,%d) sacrificeOddsThreshold=%d attackEligible=%d sacrificeEligible=%d obsoleteAttackEligible=%d obsoleteSacrificeEligible=%d obsoleteSacrificeAtOrAboveOddsThreshold=%d",
				GC.getGame().getGameTurn(), pBestUnit->getOwner(), getID(), pPlot->getX(), pPlot->getY(), iOddsThresh, iEligibleAttackers,
				iSacrificeEligibleAttackers, iObsoleteEligible, iObsoleteSacrificeEligible, iObsoleteSacrificeAtOrAboveOddsThreshold);
			CvUnitAI const* apCandidates[4] = { pBestObsoleteSacrifice, pBestNonObsoleteSacrifice, pBestObsoleteOdds, pBestNonObsoleteOdds };
			char const* aszRoles[4] = { "BEST_OBSOLETE_SACRIFICE", "BEST_NONOBSOLETE_SACRIFICE", "BEST_OBSOLETE_ODDS", "BEST_NONOBSOLETE_ODDS" };
			int aiOdds[4] = { iBestObsoleteSacrificeOdds, iBestNonObsoleteSacrificeOdds, iBestObsoleteOdds, iBestNonObsoleteOdds };
			int aiSacrificeValues[4] = { iBestObsoleteSacrificeValue, iBestNonObsoleteSacrificeValue, iBestObsoleteOddsSacrificeValue, iBestNonObsoleteOddsSacrificeValue };
			for (int i = 0; i < 4; i++)
			{
				CvUnitAI const* pCandidate = apCandidates[i];
				if (pCandidate == NULL) continue;
				int const iCandidateMaxHP = std::max(1, pCandidate->maxHitPoints());
				int const iCandidateHealth = (100 * (iCandidateMaxHP - pCandidate->getDamage())) / iCandidateMaxHP;
				bool const bCandidateCollateral = (pCandidate->collateralDamage() > 0);
				int const iCandidateBaseCollateral = (bCandidateCollateral ? estimateCollateralWeight(pPlot, getTeam()) : 0);
				int const iCandidateEffectiveStr = pCandidate->AI_currEffectiveStr(pPlot, pCandidate, bCandidateCollateral, iCandidateBaseCollateral, true);
				TechTypes const eCandidateObsoleteTech = pCandidate->getUnitInfo().getObsoleteTech();
				bool const bCandidateObsolete = (eCandidateObsoleteTech != NO_TECH && GET_TEAM(pCandidate->getTeam()).isHasTech(eCandidateObsoleteTech));
				CvPlot::DefenderFilters kDefenderFilters(pCandidate->getOwner(), pCandidate, !bPotentialEnemy, bPotentialEnemy, true, false);
				CvUnit const* pCandidateDefender = pPlot->getBestDefender(NO_PLAYER, kDefenderFilters);
				int const iCandidateDefenderHealth = (pCandidateDefender == NULL ? -1 : (100 * (std::max(1, pCandidateDefender->maxHitPoints()) - pCandidateDefender->getDamage())) / std::max(1, pCandidateDefender->maxHitPoints()));
				logBBAI("WAR_ATTACK_ORDER_CANDIDATE turn=%d player=%d group=%d target=(%d,%d) role=%s unit=%s unitId=%d unitAI=%d obsolete=%d age=%d xp=%d health=%d odds=%d sacrificeValue=%d effectiveStr=%d productionCost=%d bombard=%d collateral=%d combatLimit=%d withdrawal=%d leaderUnit=%d sameTileHeal=%d adjacentTileHeal=%d lfbRelativeValue=%d defenderOwner=%d defenderUnit=%s defenderUnitId=%d defenderHealth=%d defenderBaseStr=%d isLowPower=%d isFinal=%d",
					GC.getGame().getGameTurn(), pBestUnit->getOwner(), getID(), pPlot->getX(), pPlot->getY(), aszRoles[i],
					GC.getInfo(pCandidate->getUnitType()).getType(), pCandidate->getID(), pCandidate->AI_getUnitAIType(), bCandidateObsolete,
					GC.getGame().getGameTurn() - pCandidate->getGameTurnCreated(), pCandidate->getExperience(), iCandidateHealth, aiOdds[i],
					aiSacrificeValues[i], iCandidateEffectiveStr, pCandidate->getUnitInfo().getProductionCost(),
					pCandidate->bombardRate() > 0, bCandidateCollateral, pCandidate->combatLimit(), pCandidate->withdrawalProbability(),
					pCandidate->getLeaderUnitType() != NO_UNIT, pCandidate->getSameTileHeal(), pCandidate->getAdjacentTileHeal(),
					pCandidate->LFBgetRelativeValueRating(), (pCandidateDefender == NULL ? NO_PLAYER : pCandidateDefender->getOwner()),
					(pCandidateDefender == NULL ? "-" : GC.getInfo(pCandidateDefender->getUnitType()).getType()),
					(pCandidateDefender == NULL ? -1 : pCandidateDefender->getID()), iCandidateDefenderHealth,
					(pCandidateDefender == NULL ? -1 : pCandidateDefender->baseCombatStr()), pCandidate == pLowPowerSelected,
					pCandidate == pBestUnit);
			}
		}
	}
	return pBestUnit;
}


CvUnitAI* CvSelectionGroupAI::AI_getBestGroupSacrifice(const CvPlot* pPlot, bool bPotentialEnemy, bool bForce, bool bNoBlitz) const
{
	int iBestValue = -1; // advc.048: was 0
	CvUnitAI* pBestUnit = NULL;

	CLLNode<IDInfo> const* pUnitNode = headUnitNode();
	// <advc.048> Copied from AI_getBestGroupAttacker
	bool bHuman = (pUnitNode == NULL ? true :
			GET_PLAYER(::getUnit(pUnitNode->m_data)->getOwner()).isHuman());
	// </advc.048>
	while (pUnitNode != NULL)
	{
		CvUnitAI* pLoopUnit = ::AI_getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		if (!pLoopUnit->isDead())
		{
			bool bCanAttack = false;
			if (pLoopUnit->getDomainType() == DOMAIN_AIR)
				bCanAttack = pLoopUnit->canAirAttack();
			else
			{
				bCanAttack = pLoopUnit->canAttack();
				if (bCanAttack && bNoBlitz && pLoopUnit->isBlitz() &&
					pLoopUnit->isMadeAttack())
				{
					bCanAttack = false;
				}
			}
			if (bCanAttack)
			{
				if (bForce || pLoopUnit->canMove())
				{
					if (bForce || pLoopUnit->canMoveInto(*pPlot, true))
					{
						int iValue = pLoopUnit->AI_sacrificeValue(pPlot);
						/* advc.006: > 0 not guaranteed if unit has no
						   production cost; changed to >= 0. */
						FAssert(iValue >= 0);

						/*	we want to pick the last unit of highest value,
							so pick the last unit with a good value */
						//if (iValue >= iBestValue)
						// advc.048: As in AI_getBestGroupAttacker
						if (iValue > iBestValue || (!bHuman && iValue == iBestValue))
						{
							iBestValue = iValue;
							pBestUnit = pLoopUnit;
						}
					}
				}
			}
		}
	}
	return pBestUnit;
}

/*	Returns ratio of strengths of stacks times 100
	(so 100 is an even ratio, numbers over 100 mean that
	this group is more powerful than the stack on a plot) */
int CvSelectionGroupAI::AI_compareStacks(const CvPlot* pPlot, bool bCheckCanAttack, bool bConstCache) const // advc.001n
{
	FAssert(pPlot != NULL);

	DomainTypes eDomainType = getDomainType();
	/*	if not aircraft, then choose based on the plot,
		not the head unit (mainly for transport carried units) */
	if (eDomainType != DOMAIN_AIR)
	{
		if (pPlot->isWater())
			eDomainType = DOMAIN_SEA;
		else eDomainType = DOMAIN_LAND;
	}

	int iCompareRatio = AI_sumStrength(pPlot, eDomainType, bCheckCanAttack);
	iCompareRatio *= 100;

	PlayerTypes eOwner = getOwner();
	if (eOwner == NO_PLAYER)
		eOwner = getHeadOwner();

	FAssert(eOwner != NO_PLAYER);

	// K-Mod. Note. This function currently does not support bPotentialEnemy == false.
	//FAssert(bPotentialEnemy);
	int iDefenderSum = pPlot->isVisible(getHeadTeam()) ?
			GET_PLAYER(eOwner).AI_localDefenceStrength(pPlot, NO_TEAM, eDomainType, 0,
			true, false, bConstCache) : // advc.001n
			GET_TEAM(getHeadTeam()).AI_strengthMemory().get(*pPlot);
	// K-Mod end
	iCompareRatio /= std::max(1, iDefenderSum);

	/*	K-Mod. If there are more defenders than we have attacks,
		but yet the ratio is still greater than 100,
		then inflate the ratio futher to account for the fact that we
		are going to do significantly more damage to them than they to us.
		The purpose of this is to give the AI extra encouragement
		to attack when its units are better than the defender's units. */
	/*if (compareRatio > 100) {
		FAssert(getHeadUnit() && getNumUnits() > 0);
		int iDefenders = pPlot->getNumVisibleEnemyDefenders(getHeadUnit());
		if (iDefenders > getNumUnits())
			compareRatio += (compareRatio - 100) * (iDefenders - getNumUnits()) / getNumUnits();
	}*/ // (currently disabled)
	// K-Mod end

	return iCompareRatio;
}

/*  K-Mod. I've removed bCheckMove, and changed bCheckCanAttack to include checks
	for moves, and for hasAlreadyAttacked / blitz */
/*  advc.159: No longer simply a sum of combat strength values; see the comment
	above CvPlayerAI::AI_localDefenceStrength. */
int CvSelectionGroupAI::AI_sumStrength(const CvPlot* pAttackedPlot, DomainTypes eDomainType, bool bCheckCanAttack) const
{
	FAssert(eDomainType != DOMAIN_AIR && eDomainType != DOMAIN_IMMOBILE); // advc: Air combat strength isn't counted
	// <K-Mod>
	bool const bDefenders = (pAttackedPlot ?
			pAttackedPlot->isVisibleEnemyUnit(getHeadOwner()) : false);
	bool const bCountCollateral = (pAttackedPlot && pAttackedPlot != plot()); // </K-Mod>
	int const iBaseCollateral = (bCountCollateral ?
			estimateCollateralWeight(pAttackedPlot, getTeam()) : 0);
	int	iSum = 0;
	FOR_EACH_UNITAI_IN(pUnit, *this)
	{
		if (pUnit->isDead() ||
			// advc.opt: (If we want to count air units, then this'll have to be removed.)
			!pUnit->canFight()) 
		{
			continue;
		}
		if (eDomainType != NO_DOMAIN && pUnit->getDomainType() != eDomainType)
			continue; // advc: Moved up
		// K-Mod. (original checks deleted.)
		if (bCheckCanAttack)
		{
			// advc.opt: currEffectiveStr is 0 for air units anyway
			/*if (pUnit->getDomainType() == DOMAIN_AIR)
			{
				if (!pUnit->canAirAttack() || !pUnit->canMove() ||
					(pAttackedPlot != NULL && bDefenders &&
					!pUnit->canMoveInto(*pAttackedPlot, true, true)))
				{
					continue; // can't attack.
				}
			}
			else*/
			if (!pUnit->canAttack() || !pUnit->canMove() ||
				(pAttackedPlot && bDefenders &&
				!pUnit->canMoveInto(*pAttackedPlot, true, true)) ||
				//(!pUnit->isBlitz() && pUnit->isMadeAttack())
				pUnit->isMadeAllAttacks()) // advc.164
			{
				continue; // can't attack.
			}
		} // K-Mod end

		// iSum += pLoopUnit->currEffectiveStr(pAttackedPlot, pLoopUnit);
		/*	K-Mod estimate the value of first strike
			and the attack power of collateral units.
			(cf with calculation in CvPlayerAI::AI_localAttackStrength) */
		/*  <advc.159> Call AI_currEffectiveStr instead of currEffectiveStr.
			Adjustments for first strikes and collateral damage moved into
			that new function. */
		int const iUnitStr = pUnit->AI_currEffectiveStr(pAttackedPlot, pUnit,
				bCountCollateral, iBaseCollateral, bCheckCanAttack);
		// </advc.159>
		iSum += iUnitStr;
		// K-Mod end
	}
	return iSum;
}

// advc.004c: Auxiliary function for AI_bestUnitForMission
namespace
{
	scaled overallUnitValue(CvUnit const& kUnit)
	{	// Crude ...
		return kUnit.getUnitInfo().getProductionCost() *
				(1 + per100(6) * kUnit.getExperience());
	}
}

/*	advc.004c: (Not const b/c it needs to return a non-const unit.
	Ideally, there would be a const version returning a const unit,
	but that would lead to a lot of duplicate code.) */
CvUnit* CvSelectionGroupAI::AI_bestUnitForMission(MissionTypes eMission, CvPlot const* pMissionPlot, std::vector<int> const* pUnitsToSkip)
{
	PROFILE_FUNC(); // advc (neither frequently called nor expensive)
	CvPlot const& kAt = getPlot();
	bool bEasyCityCapture = false;
	CvCity const* pTargetCity = (pMissionPlot == NULL ? NULL :
			pMissionPlot->getPlotCity());
	int iDefenders = -1;
	if (eMission == MISSION_BOMBARD)
	{
		FOR_EACH_UNIT_IN(pUnit, *this)
		{
			pTargetCity = pUnit->bombardTarget(kAt);
			if (pTargetCity != NULL)
				break;
		}
		if (pTargetCity != NULL)
		{
			pMissionPlot = pTargetCity->plot();
			// <!-- custom: Count units that can actually defend the city. The inherited raw predicate also counted loaded combat cargo, although primary-defender selection excludes cargo. See KI#531. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			iDefenders = 0;
			FOR_EACH_UNIT_IN(pLoopUnit, *pMissionPlot)
			{
				if (!pLoopUnit->isCargo() &&
					PUF_canDefendEnemy(pLoopUnit, getOwner(), false))
				{
					iDefenders++;
				}
			}
			if (!isHuman())
			{	// Visibility cheat, but saves time.
				bEasyCityCapture = pTargetCity->AI().AI_isEvacuating();
			}
			else
			{
				// <!-- custom: Human Bombard is legal against an undefended city; canBombard and isBombardable deliberately impose no defender requirement, and the later ratio already guards zero through std::max. Remove the stale human-path assertion rather than restricting valid gameplay. See KI#521. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				int iAttackers = 0;
				FOR_EACH_UNIT_IN(pUnit, kAt)
				{
					// <!-- custom: Count only the human owner's units that can participate in the immediate city take. The inherited entry test admitted peaceful foreign units and noncombat units such as Spies, distorting the smart-Bombard decision.
					// KI#530 consequently required attack entry, but that excludes every legal capturer when the city has no defender; use capture entry in that state, which still rejects no-capture units. See KI#530 and KI#1040. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
					if (pUnit->getOwner() == getOwner() && pUnit->canMove() && pUnit->canAttack() &&
						!pUnit->canBombard(kAt) && pUnit->canMoveInto(*pMissionPlot, iDefenders > 0))
					{
						iAttackers++;
						if (iAttackers >= 2 * iDefenders)
							break;
					}
				}
				if (iAttackers >= iDefenders)
				{
					scaled rStackCmp = per100(AI_compareStacks(pMissionPlot, true, true));
					if (rStackCmp > fixp(1.5) &&
						/*	NB: If iAtt==iDef, odds needs to be very favorable
							for an immediate conquest. */
						scaled(iAttackers, std::max(iDefenders, 1)) * rStackCmp > fixp(2.5))
					{	// (Assuming that the city gets bombarded to 0)
						bEasyCityCapture = true;
					}
				}
			}
		}
	}
	CvUnit* pBestUnit = NULL;
	scaled rMaxPriority = scaled::MIN;
	FOR_EACH_UNITAI_VAR_IN(pUnit, *this)
	{
		if (!pUnit->canMove() ||
			(pUnitsToSkip != NULL &&
			std::find(pUnitsToSkip->begin(), pUnitsToSkip->end(),
			pUnit->getID()) != pUnitsToSkip->end()))
		{
			continue;
		}
		scaled rPriority;
		switch (eMission)
		{
		case MISSION_PILLAGE:
		{	// K-Mod code cut from startMission
			/*	K-Mod. Let fast units carry out the pillage action first.
				(This is based on the idea from BBAI, which had a buggy implementation.) */
			if (!pUnit->canPillage(kAt))
				continue;
			rPriority = 3;
			if (pUnit->bombardRate() > 0)
				rPriority--;
			if (pUnit->isMadeAttack())
				rPriority++;
			if (pUnit->isHurt() && !pUnit->hasMoved())
				rPriority--;
			// <advc.004c>
			rPriority *= 10000;
			rPriority -= overallUnitValue(*pUnit).round();
			// </advc.004c>
			//iPriority = (3 + iPriority) * pUnit->movesLeft() / 3;
			// advc.004c: Add 3 upfront. Don't see what good the division would do.
			rPriority *= pUnit->movesLeft();
			break;
		}
		case MISSION_BOMBARD:
		{
			if (!pUnit->canBombard(kAt))
				continue;
			/*	Some baseline to avoid precision problem when getting
				too close to 0 through divisions and multiplications */
			rPriority = 1000;
			if (bEasyCityCapture)
				rPriority *= per100(pUnit->currHitPoints());
			int const iBombard = pUnit->damageToBombardTarget(kAt);
			// bIgnoreBuilding=false b/c iBombard already reflects that
			int const iCurrDefense = pTargetCity->getDefenseModifier(false);
			int const iWaste = std::max(0, iBombard - iCurrDefense);
			if (isHuman())
			{	// Derive human intent from promotions
				scaled rDeltaBombard = (pUnit->getExtraBombardRate() - iWaste) -
						(pUnit->getExtraCollateralDamage() +
						pUnit->getExtraCityAttackPercent()) / 5;
				rPriority *= 1 + scaled::clamp(5 * rDeltaBombard, -90, 100) / 100;
			}
			rPriority *= std::max(1, 15 + iBombard - iWaste);
			// <!-- custom: Undefended cities are legal Bombard targets, but their conventional 100% attack odds erased every intended Bombard priority.
			// Apply defender odds only when a defender exists. See KI#528. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			if (iDefenders > 0)
			{
				scaled const rOdds = per100(pUnit->AI_attackOdds(pMissionPlot, false));
				rPriority *= (1 - rOdds);
			}
			rPriority /= 1 + per100(pUnit->AI_collateralDmgFactor());
			rPriority /= 15 + std::min(iDefenders, pUnit->collateralDamageMaxUnits());
			/*	(CollateralDamageLimit gets ignored by all AI code so far,
				so I'm not going to bother with it here either.) */
			break;
		}
		case MISSION_AIRBOMB:
		{
			// <!-- custom: Rank only aircraft that can execute Air Bomb at this target. AdvCiv used generic ability for cities and no target test for improvements, so an invalid first choice could abort a valid group order. See KI#394. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			if (pMissionPlot == NULL || !pUnit->canAirBombAt(*pMissionPlot))
				continue;
			if (pMissionPlot->isCity())
			{
				int iWasted = 0;
				int const iDamage = pUnit->airBombDefenseDamage(*pTargetCity);
				if (iDamage > 0)
				{
					iWasted = iDamage - pTargetCity->getDefenseModifier(false);
					iWasted = std::max(0, iWasted);
				}
				rPriority = std::max(0, iDamage - iWasted) * 1000 - iWasted * 100;
				rPriority -= overallUnitValue(*pUnit);
			}
			else
			{
				rPriority = pUnit->airBombCurrRate();
				rPriority *= 10000;
				rPriority -= overallUnitValue(*pUnit);
			}
			break;
		}
		case MISSION_PARADROP:
		{	/*	The group can be split between two plots here.
				Therefore don't check kAt. */
			if (!pUnit->canParadropAt(pUnit->plot(),
				pMissionPlot->getX(), pMissionPlot->getY()))
			{
				continue;
			}
			/*	I don't think it makes sense to maximize the evasion chance.
				When moving a stack of paratroopers, getting just one through
				isn't usually the goal. Want to send in the least valuable units
				first to draw out interceptors. */
			rPriority = -overallUnitValue(*pUnit);
			break;
		}
		default: FErrorMsg("Mission type not supported by bestUnitForMission");
		}
		if (rPriority > rMaxPriority)
		{
			rMaxPriority = rPriority;
			pBestUnit = pUnit;
		}
	}
	return pBestUnit;
}


void CvSelectionGroupAI::AI_queueGroupAttack(int iX, int iY)
{
	m_bGroupAttack = true;

	m_iGroupAttackX = iX;
	m_iGroupAttackY = iY;
}


bool CvSelectionGroupAI::AI_isDeclareWar(CvPlot const& kPlot) const // advc: param no longer optional
{
	FAssert(getHeadUnit() != NULL);

	if (isHuman())
		return false;
	// K-Mod
	if (AI_getMissionAIType() == MISSIONAI_EXPLORE)
		return false;
	// K-Mod end

	bool bLimitedWar = false;
	TeamTypes ePlotTeam = kPlot.getTeam();
	if (ePlotTeam != NO_TEAM)
	{
		WarPlanTypes eWarplan = GET_TEAM(getTeam()).AI_getWarPlan(
				GET_TEAM(ePlotTeam).getMasterTeam()); // advc.104j
		if (eWarplan == WARPLAN_LIMITED)
			bLimitedWar = true;
	}

	CvUnit const* pHeadUnit = getHeadUnit();
	if (pHeadUnit == NULL)
		return false;

	switch (pHeadUnit->AI_getUnitAIType())
	{
	case UNITAI_UNKNOWN:
	case UNITAI_ANIMAL:
	case UNITAI_SETTLE:
	case UNITAI_WORKER:
		return false;
	case UNITAI_ATTACK_CITY:
	case UNITAI_ATTACK_CITY_LEMMING:
		return true;
	case UNITAI_ATTACK:
	case UNITAI_COLLATERAL:
	case UNITAI_PILLAGE:
		return bLimitedWar;
	case UNITAI_PARADROP:
	case UNITAI_RESERVE:
	case UNITAI_COUNTER:
	case UNITAI_CITY_DEFENSE:
	case UNITAI_CITY_COUNTER:
	case UNITAI_CITY_SPECIAL:
	case UNITAI_EXPLORE:
	case UNITAI_MISSIONARY:
	case UNITAI_GREAT_PROPHET:
	case UNITAI_GREAT_ARTIST:
	case UNITAI_GREAT_SCIENTIST:
	case UNITAI_GREAT_GENERAL:
	case UNITAI_GREAT_MERCHANT:
	case UNITAI_GREAT_ENGINEER:
	case UNITAI_GREAT_SPY: // K-Mod
	case UNITAI_SPY:
	case UNITAI_ICBM:
	case UNITAI_WORKER_SEA:
		return false;
	case UNITAI_ATTACK_SEA:
	case UNITAI_RESERVE_SEA:
	case UNITAI_ESCORT_SEA:
		return bLimitedWar;
	case UNITAI_EXPLORE_SEA:
		return false;
	case UNITAI_ASSAULT_SEA:
		// <!-- custom: K-Mod's total-cargo proxy could authorize war for an assault transport carrying only a rescued Missionary or Great Person.
		// Require actual attack-capable cargo so declaration intent matches the invasion the group can execute. See KI#529. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		return (getCargoThatCanAttack() > 0);
	case UNITAI_SETTLER_SEA:
	case UNITAI_MISSIONARY_SEA:
	case UNITAI_SPY_SEA:
	case UNITAI_CARRIER_SEA:
	case UNITAI_MISSILE_CARRIER_SEA:
	case UNITAI_PIRATE_SEA:
	case UNITAI_ATTACK_AIR:
	case UNITAI_DEFENSE_AIR:
	case UNITAI_CARRIER_AIR:
	case UNITAI_MISSILE_AIR:
		return false;
	default:
		FAssert(false);
		return false;
	}
}

/*	BETTER_BTS_AI_MOD, 08/19/09 and 03/30/10, jdog5000 (General AI): START
	(advc: Moved from CvSelectionGroup) */
// Approximate how many turns this group would take to reduce pCity's defense to zero
int CvSelectionGroupAI::AI_getBombardTurns(CvCity const* pCity) const
{
	PROFILE_FUNC();
	bool const bHasBomber = (getOwner() != NO_PLAYER ?
			GET_PLAYER(getOwner()).AI_isDomainBombard(DOMAIN_AIR) : false);
	int iTotalBombardRate = (bHasBomber ? 16 : 0);
	bool bIgnoreBuildingDefense = bHasBomber;
	int iUnitBombardRate = 0;
	FOR_EACH_UNIT_IN(pUnit, *this)
	{
		if (pUnit->bombardRate() <= 0)
			continue;
		iUnitBombardRate = pUnit->bombardRate();
		if (pUnit->ignoreBuildingDefense())
			bIgnoreBuildingDefense = true;
		else
		{
			iUnitBombardRate *= std::max(25, 100 - pCity->getBuildingBombardDefense());
			iUnitBombardRate /= 100;
		}
		iTotalBombardRate += iUnitBombardRate;
	}
	// advc (minor bugfix?): BBAI had not passed bIgnoreBuildingDefense consistently
	int const iTotalDefense = pCity->getTotalDefense(bIgnoreBuildingDefense);
	if (iTotalDefense <= 0)
		return 0;
	int const iHP = GC.getMAX_CITY_DEFENSE_DAMAGE() - pCity->getDefenseDamage();
	if (iHP <= 0)
		return 0;
	int iBombardTurns = intdiv::uceil(iHP * iTotalDefense,
			std::max(1, GC.getMAX_CITY_DEFENSE_DAMAGE() * iTotalBombardRate));
	//if (gUnitLogLevel > 2) logBBAI("      Bombard of %S will take %d turns at rate %d and current damage %d with bombard def %d", pCity->getName().GetCString(), iBombardTurns, iTotalBombardRate, pCity->getDefenseDamage(), (bIgnoreBuildingDefense ? 0 : pCity->getBuildingBombardDefense()));
	return iBombardTurns;
}

// advc: Param renamed from bIgnoreMinors b/c it also causes Barbarians to be ignored
bool CvSelectionGroupAI::AI_isHasPathToAreaEnemyCity(bool bMajorOnly, MovementFlags eFlags, int iMaxPathTurns) const
{
	PROFILE_FUNC();
	//int iPass = 0; // advc: unused
	for (PlayerIter<ALIVE> it; it.hasNext(); ++it)
	{
		if (bMajorOnly && !it->isMajorCiv())
			continue;
		if (GET_TEAM(getTeam()).AI_mayAttack(it->getTeam()) &&
			AI_isHasPathToAreaPlayerCity(it->getID(), eFlags, iMaxPathTurns))
		{
			return true;
		}
	}
	return false;
}


bool CvSelectionGroupAI::AI_isHasPathToAreaPlayerCity(PlayerTypes ePlayer, MovementFlags eFlags, int iMaxPathTurns) const
{
	PROFILE_FUNC();
	// <advc> Instead of relying on the area checks to fail when the group has no area
	if (getNumUnits() <= 0)
		return false; // </advc>
	FOR_EACH_CITY(pLoopCity, GET_PLAYER(ePlayer))
	{
		if (pLoopCity->isArea(*area()))
		{
			int iPathTurns;
			if (generatePath(getPlot(), pLoopCity->getPlot(), eFlags, true,
				&iPathTurns, iMaxPathTurns))
			{
				if (iMaxPathTurns < 0 || iPathTurns <= iMaxPathTurns)
					return true;
			}
		}
	}
	return false;
}


bool CvSelectionGroupAI::AI_isStranded() const
{
	/*PROFILE_FUNC();
	if (!m_bIsStrandedCacheValid){
		m_bIsStrandedCache = calculateIsStranded();
		m_bIsStrandedCacheValid = true;
	}
	return m_bIsStrandedCache; */
	return (AI_getMissionAIType() == MISSIONAI_STRANDED); // K-Mod
} // BETTER_BTS_AI_MOD: END


CvPlot* CvSelectionGroupAI::AI_getMissionAIPlot() const
{
	return GC.getMap().plotSoren(m_iMissionAIX, m_iMissionAIY);
}


bool CvSelectionGroupAI::AI_isForceSeparate() const
{
	return m_bForceSeparate;
}


void CvSelectionGroupAI::AI_setMissionAI(MissionAITypes eNewMissionAI, CvPlot const* pNewPlot, CvUnit const* pNewUnit)
{
	//PROFILE_FUNC();

	m_eMissionAIType = eNewMissionAI;

	if (pNewPlot != NULL)
	{
		m_iMissionAIX = pNewPlot->getX();
		m_iMissionAIY = pNewPlot->getY();
	}
	else
	{
		m_iMissionAIX = INVALID_PLOT_COORD;
		m_iMissionAIY = INVALID_PLOT_COORD;
	}

	if (pNewUnit != NULL)
		m_missionAIUnit = pNewUnit->getIDInfo();
	else m_missionAIUnit.reset();
}


CvUnitAI* CvSelectionGroupAI::AI_getMissionAIUnit() const
{
	/*	advc (note): Could possibly return an incorrect unit if the correct
		one has been killed and the same FFreeList ID has gotten assigned to
		a new unit. I.e. this data member is not getting reset by CvUnit::kill. */
	return ::AI_getUnit(m_missionAIUnit);
}


bool CvSelectionGroupAI::AI_isFull()
{
	if(getNumUnits() <= 0)
		return false;

	UnitAITypes eUnitAI = getHeadUnitAIType();
	// do two passes, the first pass, we ignore units with speical cargo.
	int iSpecialCargoCount = 0;
	int iCargoCount = 0;

	// first pass, count but ignore special cargo units.
	FOR_EACH_UNIT_IN(pUnit, *this)
	{
		if (pUnit->AI_getUnitAIType() != eUnitAI)
			continue;
		if (pUnit->cargoSpace() > 0)
			iCargoCount++;
		if (pUnit->specialCargo() != NO_SPECIALUNIT)
			iSpecialCargoCount++;
		else if (!pUnit->isFull())
			return false;
	}
	/*	if every unit in the group has special cargo, then check those,
		otherwise, consider ourselves full. */
	if (iSpecialCargoCount >= iCargoCount)
	{
		FOR_EACH_UNIT_IN(pUnit, *this)
		{
			if (pUnit->AI_getUnitAIType() != eUnitAI)
				continue;
			if (!pUnit->isFull())
				return false;
		}
	}
	return true;
}


// <!-- custom: Keep the defender-ranking plot const-correct because it supplies combat context but is never modified. See KI#701. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
CvUnitAI* CvSelectionGroupAI::AI_ejectBestDefender(CvPlot const* pDefendPlot)
{
	CvUnitAI* pBestUnit = NULL;
	int iBestUnitValue = 0;
	FOR_EACH_UNITAI_VAR_IN(pUnit, *this)
	{
		//if (pUnit->noDefensiveBonus()) continue;
		// commented out by K-Mod. The noDefBonus thing is already taken into account.
		/*  advc.159: Call AI_currEffectiveStr instead of currEffectiveStr
			And reduce the precision multiplier from 100 to 20. */
		int iValue = pUnit->AI_currEffectiveStr(pDefendPlot) * 20;
		//if (pDefendPlot->isCity(true, getTeam())))
		if (GET_TEAM(getTeam()).isCityDefense(*pDefendPlot)) // advc
		{
			iValue *= 100 + pUnit->cityDefenseModifier();
			iValue /= 100;
		}
		iValue *= 100;
		//iValue /= (100 + pUnit->cityAttackModifier() + pUnit->getExtraCityAttackPercent());
		// advc.mnai: (Note that cityAttackModifier includes ExtraCityAttackPercent)
		iValue = intdiv::uceil(iValue, 100 + std::max(-50, 2 * pUnit->cityAttackModifier()));
		iValue = intdiv::uceil(iValue, 2 + (pUnit->getLevel() *
				// advc.mnai:
				(pUnit->AI_getUnitAIType() == UNITAI_ATTACK_CITY ? 2 : 1)));
		if (iValue > iBestUnitValue)
		{
			iBestUnitValue = iValue;
			pBestUnit = pUnit;
		}
	}
	if (pBestUnit != NULL && getNumUnits() > 1)
		pBestUnit->joinGroup(NULL);
	return pBestUnit;
}

// <advc.003u> Based on CvSelectionGroup::getHeadUnit
CvUnitAI const* CvSelectionGroupAI::AI_getHeadUnit() const
{
	CLLNode<IDInfo> const* pNode = headUnitNode();
	return (pNode != NULL ? ::AI_getUnit(pNode->m_data) : NULL);
}


CvUnitAI* CvSelectionGroupAI::AI_getHeadUnit()
{
	CLLNode<IDInfo>* pNode = headUnitNode();
	return (pNode != NULL ? ::AI_getUnit(pNode->m_data) : NULL);
} // </advc.003u>


void CvSelectionGroupAI::read(FDataStreamBase* pStream)
{
	CvSelectionGroup::read(pStream);

	uint uiFlag=0;
	pStream->Read(&uiFlag);

	pStream->Read(&m_iMissionAIX);
	pStream->Read(&m_iMissionAIY);

	pStream->Read(&m_bForceSeparate);

	pStream->Read((int*)&m_eMissionAIType);

	pStream->Read((int*)&m_missionAIUnit.eOwner);
	m_missionAIUnit.validateOwner(); // advc.opt
	pStream->Read(&m_missionAIUnit.iID);

	pStream->Read(&m_bGroupAttack);
	pStream->Read(&m_iGroupAttackX);
	pStream->Read(&m_iGroupAttackY);
}


void CvSelectionGroupAI::write(FDataStreamBase* pStream)
{
	CvSelectionGroup::write(pStream);

	uint uiFlag=0;
	pStream->Write(uiFlag);
	REPRO_TEST_BEGIN_WRITE(CvString::format("SelGroupAI(%d,%d,%d)", getID(), getX(), getY()));
	pStream->Write(m_iMissionAIX);
	pStream->Write(m_iMissionAIY);

	pStream->Write(m_bForceSeparate);

	pStream->Write(m_eMissionAIType);

	pStream->Write(m_missionAIUnit.eOwner);
	pStream->Write(m_missionAIUnit.iID);

	pStream->Write(m_bGroupAttack);
	pStream->Write(m_iGroupAttackX);
	pStream->Write(m_iGroupAttackY);
	REPRO_TEST_END_WRITE();
}
