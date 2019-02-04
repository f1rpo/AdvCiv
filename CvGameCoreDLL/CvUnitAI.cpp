// unitAI.cpp

#include "CvGameCoreDLL.h"
#include "CvUnitAI.h"
#include "CvMap.h"
#include "CvArea.h"
#include "CvPlot.h"
#include "CvGlobals.h"
#include "CvTeamAI.h"
#include "CvPlayerAI.h"
#include "CvGameCoreUtils.h"
#include "CvRandom.h"
#include "CyUnit.h"
#include "CyArgsList.h"
#include "CvDLLPythonIFaceBase.h"
#include "CvInfos.h"
#include "FProfiler.h"
#include "FAStarNode.h"

// interface uses
#include "CvDLLInterfaceIFaceBase.h"
#include "CvDLLFAStarIFaceBase.h"

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/02/09                                jdog5000      */
/*                                                                                              */
/* AI logging                                                                                   */
/************************************************************************************************/
#include "BetterBTSAI.h"
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

#define FOUND_RANGE				(7)

// Public Functions...

CvUnitAI::CvUnitAI()
{
	AI_reset();
}


CvUnitAI::~CvUnitAI()
{
	AI_uninit();
}


void CvUnitAI::AI_init(UnitAITypes eUnitAI)
{
	AI_reset(eUnitAI);

	//--------------------------------
	// Init other game data
	AI_setBirthmark(GC.getGameINLINE().getSorenRandNum(10000, "AI Unit Birthmark"));

	FAssertMsg(AI_getUnitAIType() != NO_UNITAI, "AI_getUnitAIType() is not expected to be equal with NO_UNITAI");
	area()->changeNumAIUnits(getOwnerINLINE(), AI_getUnitAIType(), 1);
	GET_PLAYER(getOwnerINLINE()).AI_changeNumAIUnits(AI_getUnitAIType(), 1);
}


void CvUnitAI::AI_uninit()
{
}


void CvUnitAI::AI_reset(UnitAITypes eUnitAI)
{
	AI_uninit();

	m_iBirthmark = 0;

	m_eUnitAIType = eUnitAI;
	
	m_iAutomatedAbortTurn = -1;
	m_iSearchRangeRandPercent = 100; // advc.128
}

// AI_update returns true when we should abort the loop and wait until next slice
bool CvUnitAI::AI_update()
{
	PROFILE_FUNC();

	CvUnit* pTransportUnit;

	FAssertMsg(canMove(), "canMove is expected to be true");
	FAssertMsg(isGroupHead(), "isGroupHead is expected to be true"); // XXX is this a good idea???
	// allow python to handle it
	if (GC.getUSE_AI_UNIT_UPDATE_CALLBACK()) // K-Mod. block unused python callbacks
	{
		CyUnit* pyUnit = new CyUnit(this);
		CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyUnit));	// pass in unit class
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "AI_unitUpdate", argsList.makeFunctionArgs(), &lResult);
		delete pyUnit;	// python fxn must not hold on to this pointer
		if (lResult == 1)
		{
			return false;
		}
	} // <advc.128>
	CvString const msg = CvString::format("advc.128 (%d,%d)", getX_INLINE(),
			getY_INLINE());
	m_iSearchRangeRandPercent = GC.getGame().getSorenRandNum(101, msg.c_str());
	// </advc.128>
	if (getDomainType() == DOMAIN_LAND)
	{
		if (plot()->isWater() && !canMoveAllTerrain())
		{
			getGroup()->pushMission(MISSION_SKIP);
			return false;
		}
		else
		{
			pTransportUnit = getTransportUnit();

			if (pTransportUnit != NULL)
			{
				//if (pTransportUnit->getGroup()->hasMoved() || (pTransportUnit->getGroup()->headMissionQueueNode() != NULL))
				// K-Mod. Note: transport units with cargo always have their turn before the cargo does - so... well... I've changed the skip condition.
				if (pTransportUnit->getGroup()->headMissionQueueNode() != NULL ||
					(pTransportUnit->getGroup()->AI_getMissionAIPlot() && !atPlot(pTransportUnit->getGroup()->AI_getMissionAIPlot())))
				// K-Mod end
				{
					getGroup()->pushMission(MISSION_SKIP);
					return false;
				}
			}
		}
	}

	if (AI_afterAttack())
	{
		return false;
	}

	if (getGroup()->isAutomated() && isHuman())
	{
		switch (getGroup()->getAutomateType())
		{
		case AUTOMATE_BUILD:
			if (AI_getUnitAIType() == UNITAI_WORKER)
			{
				AI_workerMove();
			}
			else if (AI_getUnitAIType() == UNITAI_WORKER_SEA)
			{
				AI_workerSeaMove();
			}
			else
			{
				FAssert(false);
			}
			break;

		case AUTOMATE_NETWORK:
			AI_networkAutomated();
			// XXX else wake up???
			break;

		case AUTOMATE_CITY:
			AI_cityAutomated();
			// XXX else wake up???
			break;

		case AUTOMATE_EXPLORE:
			switch (getDomainType())
			{
			case DOMAIN_SEA:
				AI_exploreSeaMove();
				break;

			case DOMAIN_AIR:
				// if we are cargo (on a carrier), hold if the carrier is not done moving yet
				pTransportUnit = getTransportUnit();
				if (pTransportUnit != NULL)
				{
					if (pTransportUnit->isAutomated() && pTransportUnit->canMove() && pTransportUnit->getGroup()->getActivityType() != ACTIVITY_HOLD)
					{
						getGroup()->pushMission(MISSION_SKIP);
						break;
					}
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/12/09                                jdog5000      */
/*                                                                                              */
/* Player Interface                                                                             */
/************************************************************************************************/
				// Have air units explore like AI units do
				AI_exploreAirMove();
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/		
				break;

			case DOMAIN_LAND:
				AI_exploreMove();
				break;

			default:
				FAssert(false);
				break;
			}
			
			// if we have air cargo (we are a carrier), and we done moving, explore with the aircraft as well
			if (hasCargo() && domainCargo() == DOMAIN_AIR && (!canMove() || getGroup()->getActivityType() == ACTIVITY_HOLD))
			{
				std::vector<CvUnit*> aCargoUnits;
				getCargoUnits(aCargoUnits);
				for (uint i = 0; i < aCargoUnits.size() && isAutomated(); ++i)
				{
					CvUnit* pCargoUnit = aCargoUnits[i];
					if (pCargoUnit->getDomainType() == DOMAIN_AIR)
					{
						if (pCargoUnit->canMove())
						{
							pCargoUnit->getGroup()->setAutomateType(AUTOMATE_EXPLORE);
							pCargoUnit->getGroup()->setActivityType(ACTIVITY_AWAKE);
						}
					}
				}
			}
			break;

		case AUTOMATE_RELIGION:
			if (AI_getUnitAIType() == UNITAI_MISSIONARY)
			{
				AI_missionaryMove();
			}
			break;

		default:
			FAssert(false);
			break;
		}
	
		// if no longer automated, then we want to bail
		return !getGroup()->isAutomated();
	}
	else
	{
		// <advc.139>
		UnitAITypes const uai = AI_getUnitAIType();
		int const nEvacAITypes = 9;
		UnitAITypes evacAITypes[nEvacAITypes] = {
			/*  The other AI types are obscure or already have routines for
				escaping untenable cities. */
			UNITAI_ATTACK, UNITAI_ATTACK_CITY, UNITAI_COLLATERAL, UNITAI_PILLAGE,
			UNITAI_RESERVE, UNITAI_COUNTER, UNITAI_CITY_DEFENSE, UNITAI_CITY_COUNTER,
			UNITAI_CITY_SPECIAL,
		};
		bool isEvacAI = false;
		for(int i = 0; i < nEvacAITypes; i++) {
			if(uai == evacAITypes[i]) {
				isEvacAI = true;
				break;
			}
		}
		if(plot()->isCity() && TEAMID(plot()->getOwner()) == getTeam() &&
				!isBarbarian() && isEvacAI && AI_evacuateCity())
			return false;
		switch(uai) // </advc.139>
		{
		case UNITAI_UNKNOWN:
			getGroup()->pushMission(MISSION_SKIP);
			break;

		case UNITAI_ANIMAL:
			AI_animalMove();
			break;

		case UNITAI_SETTLE:
			AI_settleMove();
			break;

		case UNITAI_WORKER:
			AI_workerMove();
			break;

		case UNITAI_ATTACK:
			if (isBarbarian())
			{
				AI_barbAttackMove();
			}
			else
			{
				AI_attackMove();
			}
			break;

		case UNITAI_ATTACK_CITY:
			AI_attackCityMove();
			break;

		case UNITAI_COLLATERAL:
			AI_collateralMove();
			break;

		case UNITAI_PILLAGE:
			AI_pillageMove();
			break;

		case UNITAI_RESERVE:
			AI_reserveMove();
			break;

		case UNITAI_COUNTER:
			AI_counterMove();
			break;

		case UNITAI_PARADROP:
			AI_paratrooperMove();
			break;

		case UNITAI_CITY_DEFENSE:
			AI_cityDefenseMove();
			break;

		case UNITAI_CITY_COUNTER:
		case UNITAI_CITY_SPECIAL:
			AI_cityDefenseExtraMove();
			break;

		case UNITAI_EXPLORE:
			AI_exploreMove();
			break;

		case UNITAI_MISSIONARY:
			AI_missionaryMove();
			break;

		case UNITAI_PROPHET:
			AI_prophetMove();
			break;

		case UNITAI_ARTIST:
			AI_artistMove();
			break;

		case UNITAI_SCIENTIST:
			AI_scientistMove();
			break;

		case UNITAI_GENERAL:
			AI_generalMove();
			break;

		case UNITAI_MERCHANT:
			AI_merchantMove();
			break;

		case UNITAI_ENGINEER:
			AI_engineerMove();
			break;

		// K-Mod
		case UNITAI_GREAT_SPY:
			AI_greatSpyMove();
			break;
		// K-Mod end

		case UNITAI_SPY:
			AI_spyMove();
			break;

		case UNITAI_ICBM:
			AI_ICBMMove();
			break;

		case UNITAI_WORKER_SEA:
			AI_workerSeaMove();
			break;

		case UNITAI_ATTACK_SEA:
			if (isBarbarian())
			{
				AI_barbAttackSeaMove();
			}
			else
			{
				AI_attackSeaMove();
			}
			break;

		case UNITAI_RESERVE_SEA:
			AI_reserveSeaMove();
			break;

		case UNITAI_ESCORT_SEA:
			AI_escortSeaMove();
			break;

		case UNITAI_EXPLORE_SEA:
			AI_exploreSeaMove();
			break;

		case UNITAI_ASSAULT_SEA:
			AI_assaultSeaMove();
			break;

		case UNITAI_SETTLER_SEA:
			AI_settlerSeaMove();
			break;

		case UNITAI_MISSIONARY_SEA:
			AI_missionarySeaMove();
			break;

		case UNITAI_SPY_SEA:
			AI_spySeaMove();
			break;

		case UNITAI_CARRIER_SEA:
			AI_carrierSeaMove();
			break;

		case UNITAI_MISSILE_CARRIER_SEA:
			AI_missileCarrierSeaMove();
			break;

		case UNITAI_PIRATE_SEA:
			AI_pirateSeaMove();
			break;

		case UNITAI_ATTACK_AIR:
			AI_attackAirMove();
			break;

		case UNITAI_DEFENSE_AIR:
			AI_defenseAirMove();
			break;

		case UNITAI_CARRIER_AIR:
			AI_carrierAirMove();
			break;

		case UNITAI_MISSILE_AIR:
			AI_missileAirMove();
			break;

		case UNITAI_ATTACK_CITY_LEMMING:
			AI_attackCityLemmingMove();
			break;

		default:
			FAssert(false);
			break;
		}
	}

	return false;
}


// Returns true if took an action or should wait to move later...
// K-Mod. I've basically rewritten this function.
// bFirst should be "true" if this is the first unit in the group to use this follow function.
// the point is that there are some calculations and checks in here which only depend on the group, not the unit
// so for efficiency, we should only check them once.
bool CvUnitAI::AI_follow(bool bFirst)
{
	FAssert(getDomainType() != DOMAIN_AIR);

	if (AI_followBombard())
		return true;

	if (bFirst && getGroup()->getHeadUnitAI() == UNITAI_ATTACK_CITY)
	{
		// note: AI_stackAttackCity will check which of our units can attack when comparing stacks;
		// and it will issue the attack order using MOVE_DIRECT ATTACK, which will execute without waiting for the entire group to have movement points.
		if (AI_stackAttackCity()) // automatic threshold
			return true;
	}

	// I've changed attack-follow code so that it will only attack with a single unit, not the whole group.
	if (bFirst && AI_cityAttack(1, 65, 0, true))
		return true;
	if (bFirst)
	{
		bool bMoveGroup = false; // to large groups to leave some units behind.
		if (getGroup()->getNumUnits() >= 16)
		{
			int iCanMove = 0;
			CLLNode<IDInfo>* pEntityNode = getGroup()->headUnitNode();
			while (pEntityNode)
			{
				CvUnit* pLoopUnit = ::getUnit(pEntityNode->m_data);
				pEntityNode = getGroup()->nextUnitNode(pEntityNode);
				iCanMove += (pLoopUnit->canMove() ? 1 : 0);
			}
			bMoveGroup = 5 * iCanMove >= 4 * getGroup()->getNumUnits() || iCanMove >= 20; // if 4/5 of our group can still move.
		}
		if (AI_anyAttack(1, isEnemy(plot()->getTeam()) ? 65 : 70, 0, bMoveGroup ? 0 : 2, true, true))
			return true;
	}
	//

	if (isEnemy(plot()->getTeam()))
	{
		if (canPillage(plot()))
		{
			getGroup()->pushMission(MISSION_PILLAGE);
			return true;
		}
	}

	// K-Mod. AI_foundRange is bad AI. It doesn't always found when we want to, and it has the potential to found when we don't!
	// So I've replaced it.
	if (AI_foundFollow())
		return true;

	return false;
}

// K-Mod. This function has been completely rewritten to improve efficiency and intelligence.
void CvUnitAI::AI_upgrade()
{
	PROFILE_FUNC();

	FAssert(!isHuman());
	FAssert(AI_getUnitAIType() != NO_UNITAI);

	if (!isReadyForUpgrade())
		return;

	const CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());
	const CvCivilizationInfo& kCivInfo = GC.getCivilizationInfo(kPlayer.getCivilizationType());
	UnitAITypes eUnitAI = AI_getUnitAIType();
	CvArea* pArea = area();

	int iBestValue = kPlayer.AI_unitValue(getUnitType(), eUnitAI, pArea) * 100;
	UnitTypes eBestUnit = NO_UNIT;

	// Note: the original code did two passes, presumably for speed reasons.
	// In the first pass, they checked only units which were flagged with the right unitAI.
	// Then, only if no such units were found, they checked all other units.
	//
	// I'm just jumping straight to the second (slower) pass, because most of the time no upgrades are available at all and so both passes would be used anyway.
	//
	// I've reversed the order of iteration because the stronger units are typically later in the list
	for (UnitClassTypes i = (UnitClassTypes)(GC.getNumUnitClassInfos()-1); i >= 0; i=(UnitClassTypes)(i-1))
	{
		UnitTypes eLoopUnit = (UnitTypes)kCivInfo.getCivilizationUnits(i);

		if (eLoopUnit != NO_UNIT)
		{
			int iValue = kPlayer.AI_unitValue(eLoopUnit, eUnitAI, pArea);
			// use a random factor. less than 100, so that the upgrade must be better than the current unit.
			iValue *= 80 + GC.getGameINLINE().getSorenRandNum(21, "AI Upgrade");

			// (believe it or not, AI_unitValue is faster than canUpgrade.)
			if (iValue > iBestValue && canUpgrade(eLoopUnit))
			{
				iBestValue = iValue;
				eBestUnit = eLoopUnit;
			}
		}
	}

	if (eBestUnit != NO_UNIT)
	{
		/* original bts code
		upgrade(eBestUnit);
		doDelayedDeath(); */

		// K-Mod. Ungroup the unit, so that we don't cause the whole group to miss their turn.
		CvUnit* pUpgradeUnit = upgrade(eBestUnit);
		doDelayedDeath();

		if (pUpgradeUnit != this)
		{
			CvSelectionGroup* pGroup = pUpgradeUnit->getGroup();
			if (pGroup->getHeadUnit() != pUpgradeUnit)
			{
				pUpgradeUnit->joinGroup(NULL);
				// indicate that the unit intends to rejoin the old group (although it might not actually do so...)
				pUpgradeUnit->getGroup()->AI_setMissionAI(MISSIONAI_GROUP, 0, pGroup->getHeadUnit());
			}
		}
	}
}


void CvUnitAI::AI_promote()
{
	PROFILE_FUNC();

	// K-Mod. A quick check to see if we can rule out all promotions in one hit, before we go through them one by one.
	if (!isReadyForPromotion()) // advc.002e
		return; // can't get any normal promotions. (see CvUnit::canPromote)
	// K-Mod end

	int iBestValue = 0;
	PromotionTypes eBestPromotion = NO_PROMOTION;

	for (int iI = 0; iI < GC.getNumPromotionInfos(); iI++)
	{
		if (canPromote((PromotionTypes)iI, -1))
		{
			int iValue = AI_promotionValue((PromotionTypes)iI);

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				eBestPromotion = ((PromotionTypes)iI);
			}
		}
	}

	if (eBestPromotion != NO_PROMOTION)
	{
		promote(eBestPromotion, -1);
		AI_promote();
	}
}


int CvUnitAI::AI_groupFirstVal()
{
	switch (AI_getUnitAIType())
	{
	case UNITAI_UNKNOWN:
	case UNITAI_ANIMAL:
		FAssert(false);
		break;

	case UNITAI_SETTLE:
		return 21;

	case UNITAI_WORKER:
		return 20;

	case UNITAI_ATTACK:
		if (collateralDamage() > 0)
		{
			return 15; // was 17
		}
		if (withdrawalProbability() > 0)
		{
			return 14; // was 15
		}
		return 13;

	case UNITAI_ATTACK_CITY:
		if (bombardRate() > 0)
		{
			return 19;
		}
		if (collateralDamage() > 0)
		{
			return 18;
		}
		if (withdrawalProbability() > 0)
		{
			return 17; // was 16
		}
		return 16; // was 14

	case UNITAI_COLLATERAL:
		return 7;

	case UNITAI_PILLAGE:
		return 12;

	case UNITAI_RESERVE:
		return 6;

	case UNITAI_COUNTER:
		return 5;

	case UNITAI_CITY_DEFENSE:
		return 3;

	case UNITAI_CITY_COUNTER:
		return 2;

	case UNITAI_CITY_SPECIAL:
		return 1;

	case UNITAI_PARADROP:
		return 4;

	case UNITAI_EXPLORE:
		return 8;

	case UNITAI_MISSIONARY:
		return 10;

	case UNITAI_PROPHET:
	case UNITAI_ARTIST:
	case UNITAI_SCIENTIST:
	case UNITAI_GENERAL:
	case UNITAI_MERCHANT:
	case UNITAI_ENGINEER:
	case UNITAI_GREAT_SPY: // K-Mod
		return 11;

	case UNITAI_SPY:
		return 9;

	case UNITAI_ICBM:
		break;

	case UNITAI_WORKER_SEA:
		return 8;

	case UNITAI_ATTACK_SEA:
		return 3;

	case UNITAI_RESERVE_SEA:
		return 2;

	case UNITAI_ESCORT_SEA:
		return 1;

	case UNITAI_EXPLORE_SEA:
		return 5;

	case UNITAI_ASSAULT_SEA:
		return 11;

	case UNITAI_SETTLER_SEA:
		return 9;

	case UNITAI_MISSIONARY_SEA:
		return 9;

	case UNITAI_SPY_SEA:
		return 10;

	case UNITAI_CARRIER_SEA:
		return 7;

	case UNITAI_MISSILE_CARRIER_SEA:
		return 6;

	case UNITAI_PIRATE_SEA:
		return 4;

	case UNITAI_ATTACK_AIR:
	case UNITAI_DEFENSE_AIR:
	case UNITAI_CARRIER_AIR:
	case UNITAI_MISSILE_AIR:
		return 0;

	case UNITAI_ATTACK_CITY_LEMMING:
		return 1;

	default:
		FAssert(false);
	}

	return 0;
}


int CvUnitAI::AI_groupSecondVal()
{
	return ((getDomainType() == DOMAIN_AIR) ? airBaseCombatStr() : baseCombatStr());
}


// Returns attack odds out of 100 (the higher, the better...)
// Withdrawal odds included in returned value
int CvUnitAI::AI_attackOdds(const CvPlot* pPlot, bool bPotentialEnemy) const
{
	PROFILE_FUNC();

	CvUnit* pDefender;
	int iOurStrength;
	int iTheirStrength;
	int iOurFirepower;
	int iTheirFirepower;
	int iBaseOdds;
	int iStrengthFactor;
	int iDamageToUs;
	int iDamageToThem;
	int iNeededRoundsUs;
	int iNeededRoundsThem;
	int iHitLimitThem;

	pDefender = pPlot->getBestDefender(NO_PLAYER, getOwnerINLINE(), this, !bPotentialEnemy, bPotentialEnemy);

	if (pDefender == NULL)
	{
		return 100;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/21/10                                jdog5000      */
/*                                                                                              */
/* Efficiency, Lead From Behind                                                                 */
/************************************************************************************************/
	// From Lead From Behind by UncutDragon
	if (GC.getLFBEnable() && GC.getLFBUseCombatOdds())
	{
		// Combat odds are out of 1000 - we need odds out of 100
		int iOdds = (getCombatOdds(this, pDefender) + 5) / 10;
		iOdds += GET_PLAYER(getOwnerINLINE()).AI_getAttackOddsChange();

		return std::max(1, std::min(iOdds, 99));
	}

	iOurStrength = ((getDomainType() == DOMAIN_AIR) ? airCurrCombatStr(NULL) : currCombatStr(NULL, NULL));
	iOurFirepower = ((getDomainType() == DOMAIN_AIR) ? iOurStrength : currFirepower(NULL, NULL));

	if (iOurStrength == 0)
	{
		return 1;
	}

	iTheirStrength = pDefender->currCombatStr(pPlot, this);
	iTheirFirepower = pDefender->currFirepower(pPlot, this);


	FAssert((iOurStrength + iTheirStrength) > 0);
	FAssert((iOurFirepower + iTheirFirepower) > 0);

	iBaseOdds = (100 * iOurStrength) / (iOurStrength + iTheirStrength);
	if (iBaseOdds == 0)
	{
		return 1;
	}

	iStrengthFactor = ((iOurFirepower + iTheirFirepower + 1) / 2);

	// UncutDragon
	// original
	//iDamageToUs = std::max(1,((GC.getDefineINT("COMBAT_DAMAGE") * (iTheirFirepower + iStrengthFactor)) / (iOurFirepower + iStrengthFactor)));
	//iDamageToThem = std::max(1,((GC.getDefineINT("COMBAT_DAMAGE") * (iOurFirepower + iStrengthFactor)) / (iTheirFirepower + iStrengthFactor)));
	// modified
	iDamageToUs = std::max(1,((GC.getCOMBAT_DAMAGE() * (iTheirFirepower + iStrengthFactor)) / (iOurFirepower + iStrengthFactor)));
	iDamageToThem = std::max(1,((GC.getCOMBAT_DAMAGE() * (iOurFirepower + iStrengthFactor)) / (iTheirFirepower + iStrengthFactor)));
	// /UncutDragon
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	iHitLimitThem = pDefender->maxHitPoints() - combatLimit();

	iNeededRoundsUs = (std::max(0, pDefender->currHitPoints() - iHitLimitThem) + iDamageToThem - 1 ) / iDamageToThem;
	iNeededRoundsThem = (std::max(0, currHitPoints()) + iDamageToUs - 1 ) / iDamageToUs;

	if (getDomainType() != DOMAIN_AIR)
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/30/09                      Mongoose & jdog5000     */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
		// From Mongoose SDK
		if (!pDefender->immuneToFirstStrikes()) {
			iNeededRoundsUs   -= ((iBaseOdds * firstStrikes()) + ((iBaseOdds * chanceFirstStrikes()) / 2)) / 100;
		}
		if (!immuneToFirstStrikes()) {
			iNeededRoundsThem -= (((100 - iBaseOdds) * pDefender->firstStrikes()) + (((100 - iBaseOdds) * pDefender->chanceFirstStrikes()) / 2)) / 100;
		}
		iNeededRoundsUs   = std::max(1, iNeededRoundsUs);
		iNeededRoundsThem = std::max(1, iNeededRoundsThem);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	}

	int iRoundsDiff = iNeededRoundsUs - iNeededRoundsThem;
	if (iRoundsDiff > 0)
	{
		iTheirStrength *= (1 + iRoundsDiff);
	}
	else
	{
		iOurStrength *= (1 - iRoundsDiff);
	}

	int iOdds = (((iOurStrength * 100) / (iOurStrength + iTheirStrength)));
	iOdds += ((100 - iOdds) * withdrawalProbability()) / 100;
	iOdds += GET_PLAYER(getOwnerINLINE()).AI_getAttackOddsChange();

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/30/09                      Mongoose & jdog5000     */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	// From Mongoose SDK
	return range(iOdds, 1, 99);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
}


// Returns true if the unit found a build for this city...
bool CvUnitAI::AI_bestCityBuild(CvCity* pCity, CvPlot** ppBestPlot, BuildTypes* peBestBuild, CvPlot* pIgnorePlot, CvUnit* pUnit)
{
	PROFILE_FUNC();

	int iBestValue = 0;
	BuildTypes eBestBuild = NO_BUILD;
	CvPlot* pBestPlot = NULL;

	// K-Mod. hack: For the AI, I want to use the standard pathfinder, CvUnit::generatePath.
	// but this function is also used to give action recommendations for the player
	// - and for that I do not want to disrupt the standard pathfinder. (because I'm paranoid about OOS bugs.)
	KmodPathFinder alt_finder;
	KmodPathFinder& pathFinder = getGroup()->AI_isControlled() ? CvSelectionGroup::path_finder : alt_finder;
	if (getGroup()->AI_isControlled())
	{
		// standard settings. cf. CvUnit::generatePath
		pathFinder.SetSettings(getGroup(), 0);
	}
	else
	{
		// like I said - this is only for action recommendations. It can be rough.
		pathFinder.SetSettings(getGroup(), 0, 5, GC.getMOVE_DENOMINATOR());
	}
	// K-Mod end

	for (int iPass = 0; iPass < 2; iPass++)
	{
		for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			CvPlot* pLoopPlot = plotCity(pCity->getX_INLINE(), pCity->getY_INLINE(), iI);

			//if (pLoopPlot != NULL)
			if (pLoopPlot && pLoopPlot != pIgnorePlot && pLoopPlot->getWorkingCity() == pCity && AI_plotValid(pLoopPlot)) // K-Mod
			{
				if (pLoopPlot->getImprovementType() == NO_IMPROVEMENT ||
					!GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_SAFE_AUTOMATION) ||
					pLoopPlot->getImprovementType() == GC.getDefineINT("RUINS_IMPROVEMENT"))
				{
					int iValue = pCity->AI_getBestBuildValue(iI);

					if (iValue > iBestValue)
					{
						BuildTypes eBuild = pCity->AI_getBestBuild(iI);
						FAssertMsg(eBuild < GC.getNumBuildInfos(), "Invalid Build");

						//if (eBuild != NO_BUILD)
						if (eBuild != NO_BUILD && canBuild(pLoopPlot, eBuild)) // K-Mod
						{
							if (0 == iPass)
							{
								iBestValue = iValue;
								pBestPlot = pLoopPlot;
								eBestBuild = eBuild;
							}
							else //if (canBuild(pLoopPlot, eBuild))
							{
								if (!(pLoopPlot->isVisibleEnemyUnit(this)))
								{
									/* original bts code
									int iPathTurns;
									if (generatePath(pLoopPlot, 0, true, &iPathTurns))
									{
										// XXX take advantage of range (warning... this could lead to some units doing nothing...)
										int iMaxWorkers = 1;
										if (getPathLastNode()->m_iData1 == 0)
										{
											iPathTurns++;
										}
										else if (iPathTurns <= 1)
										{
											iMaxWorkers = AI_calculatePlotWorkersNeeded(pLoopPlot, eBuild);
										}
										if (pUnit != NULL)
										{
											if (pUnit->plot()->isCity() && iPathTurns == 1 && getPathLastNode()->m_iData1 > 0)
											{
												iMaxWorkers += 10;
											}
										} */
									// K-Mod. basically the same thing, but using pathFinder.
									if (pathFinder.GeneratePath(pLoopPlot))
									{
										int iPathTurns = pathFinder.GetPathTurns() + (pathFinder.GetFinalMoves() == 0 ? 1 : 0);
										int iMaxWorkers = iPathTurns > 1 ? 1 : AI_calculatePlotWorkersNeeded(pLoopPlot, eBuild);
										if (pUnit && pUnit->plot()->isCity() && iPathTurns == 1)
											iMaxWorkers += 10;
									// K-Mod end
										if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BUILD, getGroup()) < iMaxWorkers)
										{
											//XXX this could be improved greatly by
											//looking at the real build time and other factors
											//when deciding whether to stack.
											iValue /= iPathTurns;

											iBestValue = iValue;
											pBestPlot = pLoopPlot;
											eBestBuild = eBuild;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		
		if (0 == iPass)
		{
			if (eBestBuild != NO_BUILD)
			{
				FAssert(pBestPlot != NULL);
				/* original bts code
				int iPathTurns;
				if ((generatePath(pBestPlot, 0, true, &iPathTurns)) && canBuild(pBestPlot, eBestBuild)
					&& !(pBestPlot->isVisibleEnemyUnit(this)))
				{
					int iMaxWorkers = 1;
					if (pUnit != NULL)
					{
						if (pUnit->plot()->isCity())
						{
							iMaxWorkers += 10;
						}
					}	
					if (getPathLastNode()->m_iData1 == 0)
					{
						iPathTurns++;
					}
					else if (iPathTurns <= 1)
					{
						iMaxWorkers = AI_calculatePlotWorkersNeeded(pBestPlot, eBestBuild);										
					} */
				// K-Mod. basically the same thing, but using pathFinder.
				if (pathFinder.GeneratePath(pBestPlot))
				{
					int iPathTurns = pathFinder.GetPathTurns() + (pathFinder.GetFinalMoves() == 0 ? 1 : 0);
					int iMaxWorkers = iPathTurns > 1 ? 1 : AI_calculatePlotWorkersNeeded(pBestPlot, eBestBuild);
					if (pUnit && pUnit->plot()->isCity() && iPathTurns == 1)
						iMaxWorkers += 10;
				// K-Mod end
					int iWorkerCount = GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pBestPlot, MISSIONAI_BUILD, getGroup());
					if (iWorkerCount < iMaxWorkers)
					{
						//Good to go.
						break;
					}
				}
				eBestBuild = NO_BUILD;
				iBestValue = 0;
			}			
		}
	}
	
	if (NO_BUILD != eBestBuild)
	{
		FAssert(NULL != pBestPlot);
		if (ppBestPlot != NULL)
		{
			*ppBestPlot = pBestPlot;
		}
		if (peBestBuild != NULL)
		{
			*peBestBuild = eBestBuild;
		}
	}


	return (NO_BUILD != eBestBuild);
}


bool CvUnitAI::AI_isCityAIType() const
{
	return (AI_getUnitAIType() == UNITAI_CITY_DEFENSE ||
		AI_getUnitAIType() == UNITAI_CITY_COUNTER ||
		AI_getUnitAIType() == UNITAI_CITY_SPECIAL ||
		AI_getUnitAIType() == UNITAI_RESERVE ||
		//advc.rom (Afforess): count units on guard mission as city defenders
		getGroup()->AI_getMissionAIType() == MISSIONAI_GUARD_CITY);
}


void CvUnitAI::AI_setBirthmark(int iNewValue)
{
	m_iBirthmark = iNewValue;
	if (AI_getUnitAIType() == UNITAI_EXPLORE_SEA)
	{
		if (GC.getGame().circumnavigationAvailable())
		{
			m_iBirthmark -= m_iBirthmark % 4;
			int iExplorerCount = GET_PLAYER(getOwnerINLINE()).AI_getNumAIUnits(UNITAI_EXPLORE_SEA);
			iExplorerCount += getOwnerINLINE() % 4;
			if (GC.getMap().isWrapX())
			{
				if ((iExplorerCount % 2) == 1)
				{
					m_iBirthmark += 1;
				}
			}
			if (GC.getMap().isWrapY())
			{
				if (!GC.getMap().isWrapX())
				{
					iExplorerCount *= 2;
				}
					
				if (((iExplorerCount >> 1) % 2) == 1)
				{
					m_iBirthmark += 2;
				}
			}
		}
	}
}


UnitAITypes CvUnitAI::AI_getUnitAIType() const
{
	return m_eUnitAIType;
}


// XXX make sure this gets called...
void CvUnitAI::AI_setUnitAIType(UnitAITypes eNewValue)
{
	FAssertMsg(eNewValue != NO_UNITAI, "NewValue is not assigned a valid value");

	if (AI_getUnitAIType() != eNewValue)
	{
		area()->changeNumAIUnits(getOwnerINLINE(), AI_getUnitAIType(), -1);
		GET_PLAYER(getOwnerINLINE()).AI_changeNumAIUnits(AI_getUnitAIType(), -1);

		m_eUnitAIType = eNewValue;

		area()->changeNumAIUnits(getOwnerINLINE(), AI_getUnitAIType(), 1);
		GET_PLAYER(getOwnerINLINE()).AI_changeNumAIUnits(AI_getUnitAIType(), 1);

		joinGroup(NULL);
	}
}

int CvUnitAI::AI_sacrificeValue(const CvPlot* pPlot) const
{
    //int iValue;
	long iValue; // K-Mod. (the int will overflow)
    int iCollateralDamageValue = 0;
    if (pPlot != NULL)
    {
        int iPossibleTargets = std::min((pPlot->getNumVisibleEnemyDefenders(this) - 1), collateralDamageMaxUnits());

        if (iPossibleTargets > 0)
        {
            iCollateralDamageValue = collateralDamage();
            iCollateralDamageValue += std::max(0, iCollateralDamageValue - 100);
            iCollateralDamageValue *= iPossibleTargets;
            iCollateralDamageValue /= 5;
        }
    }

	if (getDomainType() == DOMAIN_AIR) 
	{
		iValue = 128 * (100 + currInterceptionProbability());
		if (m_pUnitInfo->getNukeRange() != -1)
		{
			iValue += 25000;
		}
		//iValue /= std::max(1, (1 + m_pUnitInfo->getProductionCost()));
		iValue /= m_pUnitInfo->getProductionCost() > 0 ? m_pUnitInfo->getProductionCost() : 180; // K-Mod
		iValue *= (maxHitPoints() - getDamage());
		iValue /= 100;
	} 
	else 
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/14/10                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
/* 
// original bts code
		iValue  = 128 * (currEffectiveStr(pPlot, ((pPlot == NULL) ? NULL : this)));
		iValue *= (100 + iCollateralDamageValue);
		iValue /= (100 + cityDefenseModifier());
		iValue *= (100 + withdrawalProbability());	
		iValue /= std::max(1, (1 + m_pUnitInfo->getProductionCost()));
		iValue /= (10 + getExperience());
*/
		iValue  = 128 * (currEffectiveStr(pPlot, ((pPlot == NULL) ? NULL : this)));
		iValue *= (100 + iCollateralDamageValue);
		iValue /= (100 + cityDefenseModifier());
		iValue *= (100 + withdrawalProbability());
		iValue /= 100; // K-Mod

		// Experience and medics now better handled in LFB
		if( !GC.getLFBEnable() )
		{
			iValue *= 10; // K-Mod
			iValue /= (10 + getExperience()); // K-Mod - moved from out of the if.
			iValue *= 10;
			iValue /= (10 + getSameTileHeal() + getAdjacentTileHeal());
		}

		// Value units which can't kill units later, also combat limits mean higher survival odds
		/* original bbai code
		if (combatLimit() < 100)
		{
			iValue *= 150;
			iValue /= 100;

			iValue *= 100;
			iValue /= std::max(1, combatLimit());
		} */
		// K-Mod. The above code is way too extreme.
		// I'm going to replace it with something more meaningful, and less severe.
		iValue *= 100 + 5 * (2 * firstStrikes() + chanceFirstStrikes()) / 2 + (immuneToFirstStrikes() ? 20 : 0) + (combatLimit() < 100 ? 20 : 0);
		iValue /= 100;
		// K-Mod end

		//iValue /= std::max(1, (1 + m_pUnitInfo->getProductionCost()));
		iValue /= m_pUnitInfo->getProductionCost() > 0 ? m_pUnitInfo->getProductionCost() : 180; // K-Mod
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	}

	// From Lead From Behind by UncutDragon
	if (GC.getLFBEnable())
	{
		// Reduce the value of sacrificing 'valuable' units - based on great general, limited, healer, experience
		/* bbai code
		iValue *= 100;
		int iRating = LFBgetRelativeValueRating();
		if (iRating > 0)
		{
			iValue /= (1 + 3*iRating);
		} */
		// K-Mod. cf. LFBgetValueAdjustedOdds
		iValue *= 1000;
		iValue /= std::max(1, 1000 + 1000 * LFBgetRelativeValueRating() * GC.getLFBAdjustNumerator() / GC.getLFBAdjustDenominator());
		// roughly speaking, the second part of the denominator is the odds adjustment from LFBgetValueAdjustedOdds.
		// It might be more natural to subtract it from the numerator, but then we can't guarantee a positive value.
		// K-Mod end
	}

	//return iValue;
	return std::min((long)MAX_INT, iValue); // K-Mod
}

// Protected Functions...

/*  K-Mod - test if we should declare war before moving to the target plot.
	(originally, DOW were made inside the unit movement mechanics.
	To me, that seems like a really dumb idea.) */
bool CvUnitAI::AI_considerDOW(CvPlot* pPlot)
{
	CvTeamAI& kOurTeam = GET_TEAM(getTeam());
	TeamTypes ePlotTeam = pPlot->getTeam();

	//if (!canEnterArea(ePlotTeam, pPlot->area(), true))
	/*  Note: We might be a transport ship which ignores borders, but with escorts
		and cargo who don't ignore borders.
		So, we should check that the whole group can enter the borders.
		(There are faster ways to check, but this is good enough.)
		If it's an amphibious landing, lets just assume that our cargo will need a DoW! */
	if (!getGroup()->canEnterArea(ePlotTeam, pPlot->area(), true) || getGroup()->isAmphibPlot(pPlot))
	{
		if (ePlotTeam != NO_TEAM && kOurTeam.AI_isSneakAttackReady(ePlotTeam))
		{	/*  advc.163: If the tile that we're on has flipped to the war target,
				the DoW is going to bump us out. Could catch this in AI_attackCityMove
				and other functions, and compute a new path. However, bumping ourselves
				may actually be the fastest way to reach the target, so I'm just
				going to go through with the DoW. */
			/*FAssert(!plot()->isOwned() || GET_TEAM(plot()->getTeam()).
					getMasterTeam() != ePlotTeam)*/
			if (kOurTeam.canDeclareWar(ePlotTeam))
			{
				if (gUnitLogLevel > 0) logBBAI("    %S declares war on %S with AI_considerDOW (%S - %S).", kOurTeam.getName().GetCString(), GET_TEAM(ePlotTeam).getName().GetCString(), getName(0).GetCString(), GC.getUnitAIInfo(AI_getUnitAIType()).getDescription());
				kOurTeam.declareWar(ePlotTeam, true, NO_WARPLAN);
				getPathFinder().Reset();
				return true;
			}
		}
	}
	return false;
}

/*  AI_considerPathDOW checks each plot on the path until the end of the turn.
	Sometimes the end plot is in friendly territory, but we need to declare war
	to actually get there. This situation is very rare, but unfortunately we
	have to check for it every time - because otherwise, when it happens,
	the AI will just get stuck. */
bool CvUnitAI::AI_considerPathDOW(CvPlot* pPlot, int iFlags)
{
	PROFILE_FUNC();

	if (!(iFlags & MOVE_DECLARE_WAR))
		return false;

	if (!generatePath(pPlot, iFlags, true))
	{
		FAssertMsg(false, "AI_considerPathDOW didn't find a path.");
		return false;
	}

	bool bDOW = false;
	FAStarNode* pNode = getPathFinder().GetEndNode(); // TODO: rewrite so that GetEndNode isn't used.
	while (!bDOW && pNode)
	{	// advc.003:
		CvPlot* pLoopPlot = GC.getMapINLINE().plotSorenINLINE(pNode->m_iX, pNode->m_iY);
		/*  we need to check DOW even for moves several turns away -
			otherwise the actual move mission may fail to find a path.
			however, I would consider it irresponsible to call this function for multi-move missions.
			(note: amphibious landings may say 2 turns, even though it is really only 1...) */
		FAssert(pNode->m_iData2 <= 1 ||
				(pNode->m_iData2 == 2 && getGroup()->isAmphibPlot(pLoopPlot)));
		bDOW = AI_considerDOW(pLoopPlot);
		pNode = pNode->m_pParent;
	}

	return bDOW;
}
// K-Mod end

void CvUnitAI::AI_animalMove()
{
	PROFILE_FUNC();

	if (GC.getGameINLINE().getSorenRandNum(100, "Animal Attack") < GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAnimalAttackProb())
	{
		if (AI_anyAttack(1, 0))
		{
			return;
		}
	}

	if (AI_heal())
	{
		return;
	}

	if (AI_patrol())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_settleMove()
{
	PROFILE_FUNC();
	CvGame const& g = GC.getGameINLINE(); // advc.003
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod
	int iMoveFlags = MOVE_NO_ENEMY_TERRITORY; // K-Mod

	if (kOwner.getNumCities() == 0)
	{	// advc.108: Merged this from the Better BUG AI mod
/************************************************************************************************/
/* Afforess & Fuyu	                  Start      09/18/10                                       */
/*                                                                                              */
/* Check for Good City Sites Near Starting Location                                             */
/************************************************************************************************/
		// <advc>
		CvGameSpeedInfo const& gsi = GC.getGameSpeedInfo(g.getGameSpeedType());
		/*  Earlier exploreMove may have revealed more tiles. Don't set bStartingLoc;
			that setting rules out e.g. plots with a goody hut or at the edge of a
			flat map. I've added some getNumCities()<=0 checks to AI_foundValue. */
		kOwner.AI_updateFoundValues(false); // </advc>
		int iGameSpeedPercent = ( (2 * gsi.getTrainPercent())
				+ gsi.getConstructPercent() + gsi.getResearchPercent() ) / 4;
		int iMaxFoundTurn = (iGameSpeedPercent + 50) / 150; //quick 0, normal/epic 1, marathon 2
		if(!g.isScenario() && /* advc: Let the creator of the scenario decide where
								 the AI settles */
				canMove() && !kOwner.AI_isPlotCitySite(*plot()) &&
				g.getElapsedGameTurns() <= iMaxFoundTurn)
		{
			int iBestValue = 0;
			int iBestFoundTurn = 0;
			CvPlot* pBestPlot = NULL;

			for (int iCitySite = 0; iCitySite < kOwner.AI_getNumCitySites(); iCitySite++)
			{
				CvPlot* pCitySite = kOwner.AI_getCitySite(iCitySite);
				if(pCitySite->getArea() != getArea() && !canMoveAllTerrain())
					continue;
				//int iPlotValue = kOwner.AI_foundValue(pCitySite->getX_INLINE(), pCitySite->getY_INLINE());
				int iPlotValue = pCitySite->getFoundValue(kOwner.getID());
				if(iPlotValue <= iBestValue)
					continue;
				//Can this unit reach the plot this turn? (getPathLastNode()->m_iData2 == 1)
				//Will this unit still have movement points left to found the city the same turn? (getPathLastNode()->m_iData1 > 0))
				if (generatePath(pCitySite))
				{
					int iFoundTurn = g.getElapsedGameTurns() +
							/*getPathLastNode()->m_iData2 -
							(getPathLastNode()->m_iData1 > 0 ? 1 : 0);*/
							// advc: Adapted to K-Mod pathfinder
							getPathFinder().GetPathTurns() -
							(getPathFinder().GetFinalMoves() > 0 ? 1 : 0);
					if (iFoundTurn <= iMaxFoundTurn)
					{
						iPlotValue *= 100; //more precision
						/*  the slower the game speed, the less penality the plotvalue
							gets for long walks towards it.
							On normal it's -18% per turn */
						/*  advc: 18% seems a bit much; try 15%. K-Mod found values
							aren't quite on the same scale as BBAI. */
						iPlotValue *= 100 - std::min(100, ((1500/
								std::max(1, iGameSpeedPercent)) * iFoundTurn));
						iPlotValue /= 100;
						if (iPlotValue > iBestValue)
						{
							iBestValue = iPlotValue;
							iBestFoundTurn = iFoundTurn;
							pBestPlot = pCitySite;
						}
					}
				}
			}
			if (pBestPlot != NULL)
			{
				//Don't give up coast or river, don't settle on bonus with food
				/*if ( (plot()->isRiver() && !pBestPlot->isRiver())
					|| (plot()->isCoastalLand(GC.getMIN_WATER_SIZE_FOR_OCEAN()) && !pBestPlot->isCoastalLand(GC.getMIN_WATER_SIZE_FOR_OCEAN()))
					|| (pBestPlot->getBonusType(NO_TEAM) != NO_BONUS && pBestPlot->calculateNatureYield(YIELD_FOOD, getTeam(), true) > 0) )*/
				// advc: I think AI_foundValue can handle the other stuff
				if(plot()->isFreshWater() && !pBestPlot->isFreshWater()) 
				{
					pBestPlot = NULL;
				}
			}

			if (pBestPlot != NULL)
			{
				if( gUnitLogLevel >= 2 )
				{
					logBBAI("    Settler not founding in place but moving %d, %d to nearby city site at %d, %d (%d turns away) with value %d)", (pBestPlot->getX_INLINE() - plot()->getX_INLINE()), (pBestPlot->getY_INLINE() - plot()->getY_INLINE()), pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), iBestFoundTurn, iBestValue);
				}
				getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_SAFE_TERRITORY, false, false, MISSIONAI_FOUND, pBestPlot);
				return;
			}
		}
/************************************************************************************************/
/* Afforess & Fuyu	                     END                                                    */
/************************************************************************************************/
		if (canFound(plot()))
		{
			if( gUnitLogLevel >= 2 ) logBBAI("    Settler founding in place");
			getGroup()->pushMission(MISSION_FOUND);
			return;
		}
	}

	/* original bts code
	int iDanger = kOwner.AI_getPlotDanger(plot(), 3);
	
	if (iDanger > 0)
	{
		if ((plot()->getOwnerINLINE() == getOwnerINLINE()) || (iDanger > 2)) */
	// K-Mod
	if (kOwner.AI_getAnyPlotDanger(plot()))
	{
		//int iOurDefence = getGroup()->AI_sumStrength(0); // not counting defensive bonuses
		//int iEnemyAttack = kOwner.AI_localAttackStrength(plot(), NO_TEAM, getDomainType(), 2, true);
		if (!getGroup()->canDefend() ||
				100 * kOwner.AI_localAttackStrength(plot(), NO_TEAM) >
				80 * getGroup()->AI_sumStrength(0))
	// K-Mod end
		{	// flee
			joinGroup(NULL);
			if(AI_retreatToCity())
				return;
			/* original bts code
			if(AI_safety())
				return;
			getGroup()->pushMission(MISSION_SKIP); */
			// fallthrough. There might be something useful we can do. eg. AI_handleStranded!
		}
	}

	int iAreaBestFoundValue = 0;
	int iOtherBestFoundValue = 0;

	for (int iI = 0; iI < kOwner.AI_getNumCitySites(); iI++)
	{
		CvPlot* pCitySitePlot = kOwner.AI_getCitySite(iI);
		if ((pCitySitePlot->getArea() == getArea() || canMoveAllTerrain()) &&
				// UNOFFICIAL_PATCH: Only count city sites we can get to
				generatePath(pCitySitePlot, iMoveFlags, true))
		{
			if (plot() == pCitySitePlot)
			{
				if (canFound(plot()))
				{
					if( gUnitLogLevel >= 2 ) logBBAI("    Settler founding in place since it's at a city site %d, %d", getX_INLINE(), getY_INLINE());
					getGroup()->pushMission(MISSION_FOUND);
					return;					
				}
			}
			// K-Mod. If we are already heading to this site, then keep going.
			// (disabled. This is no longer required - I hope.)
			/*else {
				CvPlot* pMissionPlot = getGroup()->AI_getMissionAIPlot();
				if (pMissionPlot == pCitySitePlot && getGroup()->AI_getMissionAIType() == MISSIONAI_FOUND)
				{	// safety check. (cf. conditions in AI_found)
					if (getGroup()->canDefend() || kOwner.AI_plotTargetMissionAIs(pMissionPlot, MISSIONAI_GUARD_CITY) > 0)
					{
						if( gUnitLogLevel >= 2 ) logBBAI("    Settler continuing mission to %d, %d", pCitySitePlot->getX_INLINE(), pCitySitePlot->getY_INLINE());
						CvPlot* pEndTurnPlot = getPathEndTurnPlot();
						getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX(), pEndTurnPlot->getY(), MOVE_SAFE_TERRITORY, false, false, MISSIONAI_FOUND, pCitySitePlot);
						return;
					}
				}
			}*/
			// K-Mod end
			iAreaBestFoundValue = std::max(iAreaBestFoundValue,
					pCitySitePlot->getFoundValue(getOwnerINLINE()));

		}
		else
		{
			iOtherBestFoundValue = std::max(iOtherBestFoundValue,
					pCitySitePlot->getFoundValue(getOwnerINLINE()));
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/16/09                                jdog5000      */
/*                                                                                              */
/* Gold AI                                                                                      */
/************************************************************************************************/
	// No new settling of colonies when AI is in financial trouble
	if( plot()->isCity() && (plot()->getOwnerINLINE() == getOwnerINLINE()) )
	{
		if( kOwner.AI_isFinancialTrouble() )
		{
			iOtherBestFoundValue = 0;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (iAreaBestFoundValue == 0 && iOtherBestFoundValue == 0)
	{
		if (GC.getGame().getGameTurn() - getGameTurnCreated() > 20)
		{
			if (NULL != getTransportUnit())
			{
				getTransportUnit()->unloadAll();
			}

			if (NULL == getTransportUnit())
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      11/30/08                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
/* original bts code
				//may seem wasteful, but settlers confuse the AI.
				scrap();
				return;
*/
				if( kOwner.AI_unitTargetMissionAIs(getGroup()->getHeadUnit(), MISSIONAI_PICKUP) == 0 )
				{
					//may seem wasteful, but settlers confuse the AI.
					scrap();
					return;
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			}
		}
	}
	bool bMoveToCoast = false; // advc.040
	if (iOtherBestFoundValue * 100 > iAreaBestFoundValue * 110)
	{
		if (plot()->getOwnerINLINE() == getOwnerINLINE())
		{
			if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, NO_UNITAI,
					-1, -1, -1, 0, iMoveFlags))
				return;
			// <advc.040>
			else {
				CvCity* pCity = plot()->getPlotCity();
				if(pCity != NULL && !pCity->isCoastal() &&
						getGroup()->getNumUnits() <= 3 && area()->getNumAIUnits(
						getOwnerINLINE(), UNITAI_SETTLER_SEA) > 0)
					bMoveToCoast = true; // Check MaxCityElimination first
			} // </advc.040>
		}
	}
	
	/* original bts code
	if ((iAreaBestFoundValue > 0) && plot()->isBestAdjacentFound(getOwnerINLINE()))
	{
		if (canFound(plot()))
		{
			if( gUnitLogLevel >= 2 ) logBBAI("    Settler founding in place due to best adjacent found");
			getGroup()->pushMission(MISSION_FOUND);
			return;
		}
	} */ // disabled by K-Mod. We go to a lot of trouble to pick good city sites. Don't let this mess it up for us!

	/* original bts code
	if (!GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE) && !GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) && !getGroup()->canDefend())
	{
		if (AI_retreatToCity())
			return;
	} */ // disabled by K-Mod. Let them risk moving an undefended settler.. there are other checks in place to help them.

	if (plot()->isCity() && (plot()->getOwnerINLINE() == getOwnerINLINE()))
	{
		if (kOwner.AI_getAnyPlotDanger(plot())
			&& (GC.getGameINLINE().getMaxCityElimination() > 0))
		{
			if (getGroup()->getNumUnits() < 3)
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
		}
	}
	// <advc.040>
	if(bMoveToCoast && AI_moveSettlerToCoast())
		return; // </advc.040>
	if (iAreaBestFoundValue > 0)
	{
		if (AI_found(iMoveFlags))
		{
			return;
		}
	}

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, NO_UNITAI, -1, -1, -1, 0, iMoveFlags))
		{
			return;
		}
		// BBAI TODO: Go to a good city (like one with a transport) ...
		// advc.: ^Now adressed in AI_moveSettlerToCoast
	}

	// K-Mod: sometimes an unescorted settlers will join up with an escort mid-mission..
	{	int iLoop;
		for (CvSelectionGroup* pLoopSelectionGroup = kOwner.firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = kOwner.nextSelectionGroup(&iLoop))
		{
			if (pLoopSelectionGroup != getGroup())
			{
				if (pLoopSelectionGroup->AI_getMissionAIUnit() == this && pLoopSelectionGroup->AI_getMissionAIType() == MISSIONAI_GROUP)
				{
					int iPathTurns = MAX_INT;

					generatePath(pLoopSelectionGroup->plot(), iMoveFlags, true, &iPathTurns, 2);
					if (iPathTurns <= 2)
					{
						CvPlot* pEndTurnPlot = getPathEndTurnPlot();
						if (atPlot(pEndTurnPlot))
						{
							//getGroup()->pushMission(MISSION_SKIP, 0, 0, 0, false, false, MISSIONAI_GROUP, pEndTurnPlot);
							pLoopSelectionGroup->mergeIntoGroup(getGroup());
							FAssert(getGroup()->getNumUnits() > 1);
							FAssert(getGroup()->getHeadUnitAI() == UNITAI_SETTLE);
						}
						else
						{
							// if we were on our way to a site, keep the current mission plot.
							if (getGroup()->AI_getMissionAIType() == MISSIONAI_FOUND && getGroup()->AI_getMissionAIPlot() != NULL)
							{
								getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX_INLINE(), pEndTurnPlot->getY_INLINE(), iMoveFlags, false, false, MISSIONAI_FOUND, getGroup()->AI_getMissionAIPlot());
							}
							else
							{
								getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX_INLINE(), pEndTurnPlot->getY_INLINE(), iMoveFlags, false, false, MISSIONAI_GROUP, 0, pLoopSelectionGroup->getHeadUnit());
							}
						}
						return;
					}
				}
			}
		}
	}
	// K-Mod end
	if(AI_retreatToCity())
		return;
	// K-Mod
	if (AI_handleStranded())
		return;
	// K-Mod end
	if (AI_safety())
		return;
	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_workerMove()
{
	PROFILE_FUNC();
	
	bool bCanRoute = canBuildRoute();
	bool bNextCity = false;

	CvPlayerAI const& kOwner = GET_PLAYER(getOwnerINLINE());
	// XXX could be trouble...
	if (plot()->getOwnerINLINE() != getOwnerINLINE())
	{
		if (AI_retreatToCity())
		{
			return;
		}
	}
	if (!isHuman())
	{
		if (plot()->getOwnerINLINE() == getOwnerINLINE())
		{
			if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_SETTLE,
					2, -1, -1, 0, MOVE_SAFE_TERRITORY))
				return;
		}
	}

    if (!getGroup()->canDefend())
	{
		if (kOwner.AI_isPlotThreatened(plot(), 2))
		{
			if (AI_retreatToCity()) // XXX maybe not do this??? could be working productively somewhere else...
			{
				return;
			}
		}
	}

	if (bCanRoute)
	{
		if (plot()->getOwnerINLINE() == getOwnerINLINE()) // XXX team???
		{
			BonusTypes eNonObsoleteBonus = plot()->getNonObsoleteBonusType(getTeam());
			if (NO_BONUS != eNonObsoleteBonus)
			{
				if (!plot()->isConnectedToCapital())
				{
					ImprovementTypes eImprovement = plot()->getImprovementType();
					//if (NO_IMPROVEMENT != eImprovement && GC.getImprovementInfo(eImprovement).isImprovementBonusTrade(eNonObsoleteBonus))
					if (kOwner.doesImprovementConnectBonus(eImprovement, eNonObsoleteBonus))
					{
						if (AI_connectPlot(plot()))
						{
							return;
						}
					}
				}
			}
		}
	}

	/*  advc.117, advc.121: A measure of how busy we are. Compute it here and pass
		it to subroutines in order to avoid computing it multiple times. */
	int iNeededWorkersInArea = kOwner.AI_neededWorkers(area()) -
			kOwner.AI_totalAreaUnitAIs(area(), UNITAI_WORKER);
	
	/*CvPlot* pBestBonusPlot = NULL;
	BuildTypes eBestBonusBuild = NO_BUILD;
	int iBestBonusValue = 0; 
    if (AI_improveBonus(25, &pBestBonusPlot, &eBestBonusBuild, &iBestBonusValue)) */
	if (AI_improveBonus( // K-Mod
			iNeededWorkersInArea)) // advc.121
		return;

	if (bCanRoute && !isBarbarian())
	{
		if (AI_connectCity())
		{
			return;
		}
	}

	CvCity* pCity = NULL;

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		pCity = plot()->getPlotCity();
		if (pCity == NULL)
		{
			pCity = plot()->getWorkingCity();
		}
	}
	
	
/*	if (pCity != NULL) {
		bool bMoreBuilds = false;
		for (iI = 0; iI < NUM_CITY_PLOTS; iI++) {
			CvPlot* pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);
			if ((iI != CITY_HOME_PLOT) && (pLoopPlot != NULL)) {
				if (pLoopPlot->getWorkingCity() == pCity) {
					if (pLoopPlot->isBeingWorked()) {
						if (pLoopPlot->getImprovementType() == NO_IMPROVEMENT) {
							if (pCity->AI_getBestBuildValue(iI) > 0) {
								ImprovementTypes eImprovement;
								eImprovement = (ImprovementTypes)GC.getBuildInfo((BuildTypes)pCity->AI_getBestBuild(iI)).getImprovement();
								if (eImprovement != NO_IMPROVEMENT) {
									bMoreBuilds = true;
									break;
		}}}}}}}
		if (bMoreBuilds) {
			if (AI_improveCity(pCity))
				return;
		}}*/
	if (pCity != NULL)
	{
		/* original bts code (is it just me, or did they get this backwards?)
		if ((pCity->AI_getWorkersNeeded() > 0) && (plot()->isCity() || (pCity->AI_getWorkersNeeded() < ((1 + pCity->AI_getWorkersHave() * 2) / 3)))) */
		// K-Mod. Note: this worker is currently at pCity, and so we're probably counted in AI_getWorkersHave.
		if (pCity->AI_getWorkersNeeded() > 0 && (plot()->isCity() ||
				pCity->AI_getWorkersHave()-1 <=
				(1 + pCity->AI_getWorkersNeeded() * 2) / 3))
		// K-Mod end
		{
			if (AI_improveCity(pCity))
				return;
		}
	}

	/* original bts code
	if (AI_improveLocalPlot(2, pCity))
		return;	*/ // Moved by K-Mod

	bool bBuildFort = false;

	if (GC.getGame().getSorenRandNum(5, "AI Worker build Fort with Priority")
			== 0) // advc.001: Was > 0; why should a Fort be given priority 80% of the time?
	{
		//bool bCanal = ((100 * area()->getNumCities()) / std::max(1, GC.getGame().getNumCities()) < 85);
		// K-Mod. The current AI for canals doesn't work anyway; so lets skip it to save time.
		bool bCanal = false;
		bool bAirbase = false;
		bAirbase = (kOwner.AI_totalUnitAIs(UNITAI_PARADROP) || kOwner.AI_totalUnitAIs(UNITAI_ATTACK_AIR) || kOwner.AI_totalUnitAIs(UNITAI_MISSILE_AIR));
		
		if (bCanal || bAirbase)
		{
			if (AI_fortTerritory(bCanal, bAirbase))
			{
				return;
			}
		}
		bBuildFort = true;
	}


	if (bCanRoute && isBarbarian())
	{
		if (AI_connectCity())
		{
			return;
		}
	}

	if(!isBarbarian() && // advc.300
		(pCity == NULL || pCity->AI_getWorkersNeeded() == 0 ||
		(pCity->AI_getWorkersHave() > pCity->AI_getWorkersNeeded() + 1)))
	{	/*if ((pBestBonusPlot != NULL) && (iBestBonusValue >= 15)) {
			if (AI_improvePlot(pBestBonusPlot, eBestBonusBuild))
				return;
		}*/ // disabled by K-Mod. (this did nothing - ever - because of a bug.)

		/*if (pCity == NULL)
			pCity = GC.getMapINLINE().findCity(getX_INLINE(), getY_INLINE(), getOwnerINLINE());*/ // XXX do team???

		if (AI_nextCityToImprove(pCity))
		{
			return;
		}

		bNextCity = true;
	}

	/* if (pBestBonusPlot != NULL) {
		if (AI_improvePlot(pBestBonusPlot, eBestBonusBuild))
			return;
	} */ // K-Mod

	if (pCity != NULL)
	{
		if (AI_improveCity(pCity))
			return;
	}
	// K-Mod. (moved from higher up)
	if(AI_improveLocalPlot(2, pCity,
			iNeededWorkersInArea)) // advc.117
		return;
	//
	// <advc.300> None of the stuff below seems relevant for Barbarian workers
	if(isBarbarian()) {
		if(!AI_retreatToCity(false, true))
			getGroup()->pushMission(MISSION_SKIP);
		return;
	} // </advc.300>

	if (!bNextCity)
	{
		if (AI_nextCityToImprove(pCity))
		{
			return;
		}
	}

	if (bCanRoute)
	{
		if (AI_routeTerritory(true))
		{
			return;
		}

		if (AI_connectBonus(false))
		{
			return;
		}

		if (AI_routeCity())
		{
			return;
		}
	}

	if (AI_irrigateTerritory())
	{
		return;
	}

	if (!bBuildFort)
	{
		//bool bCanal = ((100 * area()->getNumCities()) / std::max(1, GC.getGame().getNumCities()) < 85);
		bool bCanal = false; // K-Mod. The current AI for canals doesn't work anyway; so lets skip it to save time.
		bool bAirbase = false;
		bAirbase = (kOwner.AI_totalUnitAIs(UNITAI_PARADROP) || kOwner.AI_totalUnitAIs(UNITAI_ATTACK_AIR) || kOwner.AI_totalUnitAIs(UNITAI_MISSILE_AIR));
		
		if (bCanal || bAirbase)
		{
			if (AI_fortTerritory(bCanal, bAirbase))
			{
				return;
			}
		}
	}

	if (bCanRoute)
	{
		if (AI_routeTerritory())
		{
			return;
		}
	}

	if (!isHuman() || (isAutomated() && GET_TEAM(getTeam()).getAtWarCount(true) == 0))
	{
		if (!isHuman() || (getGameTurnCreated() < GC.getGame().getGameTurn()))
		{
			if (AI_nextCityToImproveAirlift())
			{
				return;
			}
		}
		if (!isHuman())
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/14/09                                jdog5000      */
/* Worker AI                                                                                    */
/************************************************************************************************/
			/*if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY))
				return; */
			// Fill up boats which already have workers
			if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_WORKER,
					-1, -1, -1, -1, MOVE_SAFE_TERRITORY))
				return;
			// Avoid filling a galley which has just a settler in it, reduce chances for other ships
			if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, NO_UNITAI,
					-1, 2, -1, -1, MOVE_SAFE_TERRITORY))
				return;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		}
	}

	if(AI_improveLocalPlot(3, NULL,
			iNeededWorkersInArea)) // advc.117
		return;

	if (!isHuman() && AI_getUnitAIType() == UNITAI_WORKER)
	{	/*if (GC.getGameINLINE().getElapsedGameTurns() > 10) {
			if (GET_PLAYER(getOwnerINLINE()).AI_totalUnitAIs(UNITAI_WORKER) > GET_PLAYER(getOwnerINLINE()).getNumCities()) */
		// K-Mod
		if (GC.getGameINLINE().getElapsedGameTurns() > GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getResearchPercent()/6)
		{
			if (kOwner.AI_totalUnitAIs(UNITAI_WORKER) > std::max(GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities(), kOwner.getNumCities()*3/2) &&
				area()->getNumAIUnits(getOwnerINLINE(), UNITAI_WORKER) > kOwner.AI_neededWorkers(area())*3/2)
		// K-Mod end
			{
				if (kOwner.calculateUnitCost() > 0)
				{
					scrap();
					return;
				}
			}
		}
	}

	if (AI_retreatToCity(false, true))
	{
		return;
	}

	/*if (AI_retreatToCity())
		return; */ // disabled by K-Mod (redundant)

	// K-Mod
	if(AI_handleStranded())
		return;
	// K-Mod end

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_barbAttackMove()
{
	PROFILE_FUNC();
	CvGame& g = GC.getGameINLINE();
	if (AI_guardCity(false, true, 1))
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/15/10                                jdog5000      */
/*                                                                                              */
/* Barbarian AI                                                                                 */
/************************************************************************************************/
	if (plot()->isGoody())
	{
		if (AI_anyAttack(1, 90))
		{
			return;
		}

		if (plot()->plotCount(PUF_isUnitAIType, UNITAI_ATTACK, -1, getOwnerINLINE()) == 1 && getGroup()->getNumUnits() == 1)
		{
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/		

	if (g.getSorenRandNum(2, "AI Barb") == 0)
	{
		if (AI_pillageRange(1))
		{
			return;
		}
	}

	if (AI_anyAttack(1, 20))
	{
		return;
	}

  // <advc.300>
  // See CvTeamAI::AI_calculateAreaAIType
  if(area()->getAreaAIType(getTeam()) != AREAAI_ASSAULT) {
	  int iCivsInArea = area()->countCivs(true);
	  int iCivCitiesInArea = area()->countCivCities();
	  int iBabarianCitiesInArea = area()->getNumCities() - iCivCitiesInArea;
	  int iCivCities = g.getNumCivCities();
	  int iCivs = g.countCivPlayersAlive();
	  // </advc.300>
	  if(g.isOption(GAMEOPTION_RAGING_BARBARIANS) &&
			// <advc.300>
			/*  On slower than Normal game speed, don't start to rage until 3 in 5
				civs have founded a second city. */
			((iCivsInArea > 1 ?
			(3 * iCivCitiesInArea > 5 * iCivsInArea) :
			(2 * iCivCities > 3 * iCivs)) ||
			GC.getGameSpeedInfo(g.getGameSpeedType()).getBarbPercent() <= 100))
			// </advc.300>
	{
		if (AI_pillageRange(4))
		{
			return;
		}

		if (AI_cityAttack(3, 10))
		{
			return;
		}

		if (area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE)
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/15/10                                jdog5000      */
/*                                                                                              */
/* Barbarian AI                                                                                 */
/************************************************************************************************/
			if (AI_groupMergeRange(UNITAI_ATTACK, 1, true, true, true))
			{
				return;
			}

			if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 3, true, true, true))
			{
				return;
			}

			if (AI_goToTargetCity(MOVE_ATTACK_STACK, 12))
			{
				return;
			}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/		
		}
	}
	/* <advc.300> Now checked per area unless there is only one civ (to avoid an
	   isolated human civ deliberately steering Barbarian activity in its area).
	   Barbarian cities no longer count, but threshold lowered to 2.5. */
	else if(iCivsInArea > 1 ?
			(2 * iCivCitiesInArea > 5 * iCivsInArea) :
			// The BtS condition: // </advc.300>
			(iCivCities > iCivs * 3))
	{
		if (AI_cityAttack(1, 15))
		{
			return;
		}

		if (AI_pillageRange(3))
		{
			return;
		}

		if (AI_cityAttack(2, 10))
		{
			return;
		}

		if (area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE)
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/15/10                                jdog5000      */
/*                                                                                              */
/* Barbarian AI                                                                                 */
/************************************************************************************************/
			if (AI_groupMergeRange(UNITAI_ATTACK, 1, true, true, true))
			{
				return;
			}

			if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 3, true, true, true))
			{
				return;
			}

			if (AI_goToTargetCity(MOVE_ATTACK_STACK, 12))
			{
				return;
			}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/		
		}
	}
	// <advc.300>
	else if(iCivsInArea > 1 ?
		(iCivCitiesInArea > 2 * iCivsInArea ||
		// For continents that have only room for 2 or 3 cities
		(iBabarianCitiesInArea <= 0 && iCivCities > 3 * iCivs)) : // </advc.300>
		(iCivCities > iCivs * 2))
	{
		if(AI_pillageRange(2))
			return;
		if(AI_cityAttack(1, 10))
			return;
	}
  }
	
	if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 1))
	{
		return;
	}

	if (AI_heal())
	{
		return;
	}

	if (AI_guardCity(false, true, 2))
	{
		return;
	}

	if (AI_patrol())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}

// This function has been heavily edited by K-Mod
void CvUnitAI::AI_attackMove()
{
	PROFILE_FUNC();
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod
	bool bDanger = (kOwner.AI_getAnyPlotDanger(plot(), 3));
	bool bLandWar = kOwner.AI_isLandWar(area()); // K-Mod

	// K-Mod note. We'll split the group up later if we need to. (bbai group splitting code deleted.)
	FAssert(getGroup()->countNumUnitAIType(UNITAI_ATTACK_CITY) == 0); // K-Mod. (I'm pretty sure this can't happen.)

	// Attack choking units
	// K-Mod (bbai code deleted)
	if (plot()->getTeam() == getTeam() && (bDanger || area()->getAreaAIType(getTeam()) != AREAAI_NEUTRAL))
	{
		if (bDanger && plot()->isCity())
		{
			if (AI_leaveAttack(2, 55, 105))
				return;
		}
		else
		{
			if (AI_defendTerritory(70, 0, 2, true))
				return;
		}
	}
	// K-Mod end

	{
		PROFILE("CvUnitAI::AI_attackMove() 1");

		// Guard a city we're in if it needs it
		if (AI_guardCity(true))
		{
			return;
		}

		/* if( !(plot()->isOwned()) )
		{
			// Group with settler after naval drop
			if( AI_groupMergeRange(UNITAI_SETTLE, 2, true, false, false) )
			{
				return;
			}
		} */ // disabled by K-Mod. This is redundant.

		if(!plot()->isOwned() || plot()->getOwnerINLINE() == getOwnerINLINE())
		{
			if(area()->getCitiesPerPlayer(getOwnerINLINE()) > kOwner.AI_totalAreaUnitAIs(area(), UNITAI_CITY_DEFENSE))
			{
				// Defend colonies in new world
				//if (AI_guardCity(true, true, 3))
				if (getGroup()->getNumUnits() == 1 ? AI_guardCityMinDefender(true) : AI_guardCity(true, true, 3)) // K-Mod
				{
					return;
				}
			}
		}

		if (AI_heal(30, 1))
		{
			return;
		}
		
		/* original bts code (with omniGroup subbed in.)
		if (!bDanger)
		{
			//if (AI_group(UNITAI_SETTLE, 1, -1, -1, false, false, false, 3, true))
			if (AI_omniGroup(UNITAI_SETTLE, 1, -1, false, 0, 3, true, false))
			{
				return;
			}

			//if (AI_group(UNITAI_SETTLE, 2, -1, -1, false, false, false, 3, true))
			if (AI_omniGroup(UNITAI_SETTLE, 2, -1, false, 0, 3, false, false))
			{
				return;
			}
		} */
		// K-Mod
		if (AI_omniGroup(UNITAI_SETTLE, 2, -1, false, 0, 3, false, false, false, false, false))
			return;
		// K-Mod end

		if (AI_guardCityAirlift())
		{
			return;
		}

		if (AI_guardCity(false, true, 1))
		{
			return;
		}

		//join any city attacks in progress
		/* original bts code
		if (plot()->isOwned() && plot()->getOwnerINLINE() != getOwnerINLINE())
		{
			if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 1, true, true))
			{
				return;
			}
		} */
		// K-Mod
		if (isEnemy(plot()->getTeam()))
		{
			if (AI_omniGroup(UNITAI_ATTACK_CITY, -1, -1, true, 0, 2, true, false))
			{
				return;
			}
		}
		// K-Mod end
		
		AreaAITypes eAreaAIType = area()->getAreaAIType(getTeam());
        if (plot()->isCity())
        {
            if (plot()->getTeam() == getTeam()) // cdtw.9
            {
                if ((eAreaAIType == AREAAI_ASSAULT) || (eAreaAIType == AREAAI_ASSAULT_ASSIST))
                {
                    if (AI_offensiveAirlift())
                    {
                        return;
                    }
                }
            }
        }
		
		if (bDanger)
		{
			// K-Mod
			if (getGroup()->getNumUnits() > 1 && AI_stackVsStack(3, 110, 65, 0))
				return;
			// K-Mod end

			/* original bts code
			if (AI_cityAttack(1, 55))
			{
				return;
			}

			if (AI_anyAttack(1, 65))
			{
				return;
			} */

			if (collateralDamage() > 0)
			{
				if (AI_anyAttack(1, 45, 0, 3))
				{
					return;
				}
			}
		}
		// K-Mod (moved from below, and replacing the disabled stuff above)
		if (AI_anyAttack(1, 70))
		{
			return;
		}
		// K-Mod end

		if (!noDefensiveBonus())
		{
			if (AI_guardCity(false, false))
			{
				return;
			}
		}

		if (!bDanger)
		{
			if (plot()->getTeam() == getTeam()) // cdtw.9
			{
				bool bAssault = ((eAreaAIType == AREAAI_ASSAULT) || (eAreaAIType == AREAAI_ASSAULT_MASSING) || (eAreaAIType == AREAAI_ASSAULT_ASSIST));
				if ( bAssault )
				{
					if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, UNITAI_ATTACK_CITY, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
					{
						return;
					}		
				}

				if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_SETTLE, -1, -1, -1, 1, MOVE_SAFE_TERRITORY, 3))
				{
					return;
				}

				//bool bLandWar = ((eAreaAIType == AREAAI_OFFENSIVE) || (eAreaAIType == AREAAI_DEFENSIVE) || (eAreaAIType == AREAAI_MASSING));
				if (!bLandWar)
				{
					// Fill transports before starting new one, but not just full of our unit ai
					if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, 1, -1, -1, 1, MOVE_SAFE_TERRITORY, 4))
					{
						return;
					}

					// Pick new transport which has space for other unit ai types to join
					if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, 2, -1, -1, MOVE_SAFE_TERRITORY, 4))
					{
						return;
					}
				}

				if (kOwner.AI_unitTargetMissionAIs(this, MISSIONAI_GROUP) > 0)
				{
					getGroup()->pushMission(MISSION_SKIP);
					return;
				}
			}
		}

		// Allow larger groups if outside territory
		if( getGroup()->getNumUnits() < 3 )
		{
			if( plot()->isOwned() && GET_TEAM(getTeam()).isAtWar(plot()->getTeam()) )
			{
				//if (AI_groupMergeRange(UNITAI_ATTACK, 1, true, true, true))
				if (AI_omniGroup(UNITAI_ATTACK, 3, -1, false, 0, 1, true, false, true, false, false))
				{
					return;
				}
			}
		}

		if (AI_goody(3))
		{
			return;
		}

		/* moved up
		if (AI_anyAttack(1, 70))
		{
			return;
		} */
	}

	{
		PROFILE("CvUnitAI::AI_attackMove() 2");

		if (bDanger)
		{
			// K-Mod. This block has been rewritten. (original code deleted)

			// slightly more reckless than last time
			if (getGroup()->getNumUnits() > 1 && AI_stackVsStack(3, 90, 40, 0))
				return;

			bool bAggressive = area()->getAreaAIType(getTeam()) != AREAAI_DEFENSIVE || getGroup()->getNumUnits() > 1 || plot()->getTeam() != getTeam();

			if (bAggressive && AI_pillageRange(1, 10))
				return;

			if (plot()->getTeam() == getTeam())
			{
				if (AI_defendTerritory(55, 0, 2, true))
				{
					return;
				}
			}
			else if (AI_anyAttack(1, 45))
			{
				return;
			}

			if (bAggressive && AI_pillageRange(3, 10))
			{
				return;
			}

			if (getGroup()->getNumUnits() < 4 && isEnemy(plot()->getTeam()))
			{
				if (AI_choke(1))
				{
					return;
				}
			}

			if (bAggressive && AI_anyAttack(3, 40))
				return;
		}

		if (!isEnemy(plot()->getTeam()))
		{
			if (AI_heal())
			{
				return;
			}
		}

		//if ((kOwner.AI_getNumAIUnits(UNITAI_CITY_DEFENSE) > 0) || (GET_TEAM(getTeam()).getAtWarCount(true) > 0))
		if (!plot()->isCity() || plot()->plotCount(PUF_isUnitAIType, UNITAI_CITY_DEFENSE, -1, getOwnerINLINE()) > 0) // K-Mod
		{
			// BBAI TODO: If we're fast, maybe shadow an attack city stack and pillage off of it

			bool bIgnoreFaster = false;
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_LAND_BLITZ))
			{
				if (area()->getAreaAIType(getTeam()) != AREAAI_ASSAULT)
				{
					bIgnoreFaster = true;
				}
			}

			//if (AI_group(UNITAI_ATTACK_CITY, 1, 1, -1, bIgnoreFaster, true, true, 5))
			// K-Mod
			bool bAttackCity = bLandWar && (area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE || (AI_getBirthmark() + GC.getGameINLINE().getGameTurn()/8)%5 <= 1);
			if (bAttackCity)
			{
				// strong merge strategy
				if (AI_omniGroup(UNITAI_ATTACK_CITY, -1, -1, true, 0, 5, true, getGroup()->getNumUnits() < 2, bIgnoreFaster, false, false))
					return;
			}
			else
			{
				// weak merge strategy
				if (AI_omniGroup(UNITAI_ATTACK_CITY, -1, 2, true, 0, 5, true, false, bIgnoreFaster, false, false))
					return;
			}
			// K-Mod end

			//if (AI_group(UNITAI_ATTACK, 1, 1, -1, true, true, false, 4))
			if (AI_omniGroup(UNITAI_ATTACK, 2, -1, false, 0, 4, true, true, true, true, false))
			{
				return;
			}
			
			// BBAI TODO: Need group to be fast, need to ignore slower groups
			//if (GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_FASTMOVERS))
			//{
			//	if (AI_group(UNITAI_ATTACK, /*iMaxGroup*/ 4, /*iMaxOwnUnitAI*/ 1, -1, true, false, false, /*iMaxPath*/ 3))
			//	{
			//		return;
			//	}
			//}

			//if (AI_group(UNITAI_ATTACK, 1, 1, -1, true, false, false, 1))
			if (AI_omniGroup(UNITAI_ATTACK, 1, 1, false, 0, 1, true, true, false, false, false))
			{
				return;
			}

			// K-Mod. If we're feeling aggressive, then try to get closer to the enemy.
			if (bAttackCity && getGroup()->getNumUnits() > 1)
			{
				/*  advc.001t (Tbd.?): Maybe check CvSelectionGroupAI::AI_isDeclareWar
					and pass MOVE_DECLARE_WAR (instead of 0) if true */
				if (AI_goToTargetCity(0, 12))
					return;
			}
			// K-Mod end
		}

		/* original bts code
		if (area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE)
		{
			if (getGroup()->getNumUnits() > 1)
			{
				//if (AI_targetCity())
				if (AI_goToTargetCity(0, 12))
				{
					return;
				}
			}
		} */ // disabled by K-Mod. (moved / changed)
		/* BBAI code
		else if( area()->getAreaAIType(getTeam()) != AREAAI_DEFENSIVE )
		{
			if (area()->getCitiesPerPlayer(BARBARIAN_PLAYER) > 0)
			{
				if (getGroup()->getNumUnits() >= GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getBarbarianInitialDefenders())
				{
					if (AI_goToTargetBarbCity(10))
					{
						return;
					}
				}
			}
		} */ // disabled by K-Mod. attack groups are currently limited to 2 units anyway. This test will never pass.

		if (AI_guardCity(false, true, 3))
		{
			return;
		}

		if ((kOwner.getNumCities() > 1) && (getGroup()->getNumUnits() == 1))
		{
			if (area()->getAreaAIType(getTeam()) != AREAAI_DEFENSIVE)
			{
				if (area()->getNumUnrevealedTiles(getTeam()) > 0)
				{
					if (kOwner.AI_areaMissionAIs(area(), MISSIONAI_EXPLORE, getGroup()) < (kOwner.AI_neededExplorers(area()) + 1))
					{
						if (AI_exploreRange(3))
						{
							return;
						}

						if (AI_explore())
						{
							return;
						}
					}
				}
			}
		}

		//if (AI_protect(35, 0, 5))
		if (AI_defendTerritory(45, 0, 7)) // K-Mod
		{
			return;
		}

		if (AI_offensiveAirlift())
		{
			return;
		}

		if (!bDanger && (area()->getAreaAIType(getTeam()) != AREAAI_DEFENSIVE))
		{
			if (plot()->getTeam() == getTeam()) // cdtw.9
			{
				if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, 1, -1, -1, 1, MOVE_SAFE_TERRITORY, 4))
				{
					return;
				}

				if( (GET_TEAM(getTeam()).getAtWarCount(true) > 0) && !(getGroup()->isHasPathToAreaEnemyCity()) )
				{
					if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
					{
						return;
					}
				}
			}
		}

		// K-Mod
		if (getGroup()->getNumUnits() >= 4 && plot()->getTeam() == getTeam())
		{
			CvSelectionGroup *pSplitGroup, *pRemainderGroup = NULL;
			pSplitGroup = getGroup()->splitGroup(2, 0, &pRemainderGroup);
			if (pSplitGroup)
				pSplitGroup->pushMission(MISSION_SKIP);
			if (pRemainderGroup)
			{
				if (pRemainderGroup->AI_isForceSeparate())
					pRemainderGroup->AI_separate();
				else
					pRemainderGroup->pushMission(MISSION_SKIP);
			}
			return;
		}
		// K-Mod end

		if (AI_defend())
		{
			return;
		}

		if (AI_travelToUpgradeCity())
		{
			return;
		}

		// K-Mod
		if (AI_handleStranded())
			return;
		// K-Mod end

		/* if( !bDanger && !isHuman() && plot()->isCoastalLand() && kOwner.AI_unitTargetMissionAIs(this, MISSIONAI_PICKUP) > 0 )
		{
			// If no other desirable actions, wait for pickup
			getGroup()->pushMission(MISSION_SKIP);
			return;
		} */ // disabled by K-Mod. We don't need this.

		if (AI_patrol())
		{
			return;
		}

		if (AI_retreatToCity())
		{
			return;
		}

		if (AI_safety())
		{
			return;
		}
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_paratrooperMove()
{
	PROFILE_FUNC();

	bool bHostile = (plot()->isOwned() && isPotentialEnemy(plot()->getTeam()));
	if (!bHostile)
	{
		if (AI_guardCity(true))
		{
			return;
		}
		
		if (plot()->getTeam() == getTeam())
		{
			if (plot()->isCity())
			{
				if (AI_heal(30, 1))
				{
					return;
				}
			}
			
			//AreaAITypes eAreaAIType = area()->getAreaAIType(getTeam());
			//bool bLandWar = ((eAreaAIType == AREAAI_OFFENSIVE) || (eAreaAIType == AREAAI_DEFENSIVE) || (eAreaAIType == AREAAI_MASSING));		
			bool bLandWar = GET_PLAYER(getOwnerINLINE()).AI_isLandWar(area()); // K-Mod
			if (!bLandWar)
			{
				if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, 0, MOVE_SAFE_TERRITORY, 4))
				{
					return;
				}
			}
		}

		if (AI_guardCity(false, true, 1))
		{
			return;
		}
	}

	if (AI_cityAttack(1, 45))
	{
		return;
	}
	
	if (AI_anyAttack(1, 55))
	{
		return;
	}
	
	if (!bHostile)
	{
		if (AI_paradrop(getDropRange()))
		{
			return;
		}
	
		if (AI_offensiveAirlift())
		{
			return;
		}

		if (AI_moveToStagingCity())
		{
			return;
		}
		
		if (AI_guardFort(true))
		{
			return;
		}

		if (AI_guardCityAirlift())
		{
			return;
		}
	}

	/* if (collateralDamage() > 0)
	{
		if (AI_anyAttack(1, 45, 0, 3))
		{
			return;
		}
	} */ // disabled by K-Mod. (redundant)

	if (AI_pillageRange(1, 15))
	{
		return;
	}
	
	if (bHostile)
	{
		if (AI_choke(1))
		{
			return;
		}
	}
	
	if (AI_heal())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_defendTerritory(55, 0, 5)) // K-Mod
	{
		return;
	}

	// K-Mod
	if (AI_handleStranded())
		return;
	// K-Mod end

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}

// This function has been heavily edited by K-Mod and by BBAI
void CvUnitAI::AI_attackCityMove()
{
	PROFILE_FUNC();
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod

	AreaAITypes eAreaAI = area()->getAreaAIType(getTeam());
    //bool bLandWar = !isBarbarian() && ((eAreaAIType == AREAAI_OFFENSIVE) || (eAreaAIType == AREAAI_DEFENSIVE) || (eAreaAIType == AREAAI_MASSING));
	bool bLandWar = !isBarbarian() && kOwner.AI_isLandWar(area()); // K-Mod
	bool bAssault = !isBarbarian() && (eAreaAI == AREAAI_ASSAULT ||
			eAreaAI == AREAAI_ASSAULT_ASSIST || eAreaAI == AREAAI_ASSAULT_MASSING);

	bool bTurtle = kOwner.AI_isDoStrategy(AI_STRATEGY_TURTLE);
	bool bAlert1 = kOwner.AI_isDoStrategy(AI_STRATEGY_ALERT1);
	bool bIgnoreFaster = false;
	if (kOwner.AI_isDoStrategy(AI_STRATEGY_LAND_BLITZ))
	{
		if (!bAssault && area()->getCitiesPerPlayer(getOwnerINLINE()) > 0)
		{
			bIgnoreFaster = true;
		}
	}

	bool bInCity = plot()->isCity();

	if( bInCity &&
			plot()->getTeam() == getTeam()) // cdtw.9
	{
		// force heal if we in our own city and damaged
		// can we remove this or call AI_heal here?
		if (getGroup()->getNumUnits() == 1 && getDamage() > 0)
		{
			getGroup()->pushMission(MISSION_HEAL);
			return;
		}

		if( bIgnoreFaster )
		{
			// BBAI TODO: split out slow units ... will need to test to make sure this doesn't cause loops
		}

		if ((GC.getGame().getGameTurn() - plot()->getPlotCity()->getGameTurnAcquired()) <= 1
				// cdtw.9: (comment from Dave_uk) only do this in our own cities though
				&& plot()->getOwnerINLINE() == getOwnerINLINE())
		{
			CvSelectionGroup* pOldGroup = getGroup();

			pOldGroup->AI_separateNonAI(UNITAI_ATTACK_CITY);

			if (pOldGroup != getGroup())
			{
				return;
			}
		}

		if (AI_guardCity(false)) // note. this will eject a unit to defend the city rather then using the whole group
		{
			return;
		}

		//if ((eAreaAIType == AREAAI_ASSAULT) || (eAreaAIType == AREAAI_ASSAULT_ASSIST))
		if (bAssault) // K-Mod
		{
		    if (AI_offensiveAirlift())
		    {
		        return;
		    }
		}
	}

	bool bAtWar = isEnemy(plot()->getTeam());

	bool bHuntBarbs = false;
	bool bReadyToAttack = false;
	// <advc.300>
	int iOurEra = GET_PLAYER(getOwnerINLINE()).getCurrentEra();
	int iBarbarianEra = GET_PLAYER(BARBARIAN_PLAYER).getCurrentEra();
	int iBarbarianGarrison = 2 + iBarbarianEra;
	if(!isBarbarian() && !bTurtle && ((eAreaAI != AREAAI_DEFENSIVE && eAreaAI != AREAAI_OFFENSIVE &&
			!bAlert1) || iBarbarianGarrison < 2 * iOurEra))
		bHuntBarbs = true;
	/*  Don't yet know if we'll actually target a Barbarian city, so it's hard to
		decide on the proper size of the attack stack. But if there is nothing else
		to attack, it's easy. */
	bool bHuntOnlyBarbs = (bHuntBarbs && !GET_TEAM(getTeam()).AI_isSneakAttackReady() &&
			GET_TEAM(getTeam()).getAtWarCount() <= 0);
	if(!bTurtle) {
		int iGroupSz = getGroup()->getNumUnits();
		if(!bHuntOnlyBarbs && iGroupSz >= AI_stackOfDoomExtra())
			bReadyToAttack = true;
		else if(bHuntOnlyBarbs &&
			iGroupSz + iOurEra >= 1.25 * iBarbarianGarrison + iBarbarianEra &&
				/*  Don't send a giant stack. (Tbd.: Should perhaps
					split the group up then.) */
				iGroupSz < 3 * iBarbarianGarrison)
			bReadyToAttack = true;
	} // </advc.300>

	if( isBarbarian() )
	{
		bLandWar = (area()->getNumCities() - area()->getCitiesPerPlayer(BARBARIAN_PLAYER) > 0);
		bReadyToAttack = (getGroup()->getNumUnits() >= 3);
	}

	if( bReadyToAttack )
	{
		// Check that stack has units which can capture cities
		// (K-Mod, I've edited this section to distinguish between 'no capture' and 'combat limit < 100')
		bReadyToAttack = false;
		int iNoCombatLimit = 0;
		int iCityCapture = 0;
		CvSelectionGroup* pGroup = getGroup();

		CLLNode<IDInfo>* pUnitNode = pGroup->headUnitNode();
		while (pUnitNode != NULL && !bReadyToAttack)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = pGroup->nextUnitNode(pUnitNode);

			//if( !pLoopUnit->isOnlyDefensive() )
			if (pLoopUnit->canAttack() // K-Mod
					// advc.315:
					&& !::isMostlyDefensive(pLoopUnit->getUnitInfo()))
			{
				iCityCapture += pLoopUnit->isNoCapture() ? 0 : 1;
				iNoCombatLimit += pLoopUnit->combatLimit() < 100 ? 0 : 1;

				//if( iCityCaptureCount > 5 || 3*iCityCaptureCount > getGroup()->getNumUnits() )

				if ((iCityCapture >= 3 || 2*iCityCapture > pGroup->getNumUnits()) &&
					(iNoCombatLimit >= 6 || 3*iNoCombatLimit > pGroup->getNumUnits()))
				{
					bReadyToAttack = true;
				}
			}
		}
	}

	// K-Mod. Try to be consistent in our usage of move flags, so that we don't cause unnecessary pathfinder resets.
	int iMoveFlags = MOVE_AVOID_ENEMY_WEIGHT_2 | (bReadyToAttack ? MOVE_ATTACK_STACK | MOVE_DECLARE_WAR : 0);

	// Barbarian stacks should be reckless and unpredictable.
	if (isBarbarian())
	{
		int iThreshold = GC.getGameINLINE().getSorenRandNum(150, "barb attackCity stackVsStack threshold") + 20;
		if (AI_stackVsStack(1, iThreshold, 0, iMoveFlags))
			return;
	}
	// K-Mod end

	//if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 0, true, true, bIgnoreFaster))
	if (AI_omniGroup(UNITAI_ATTACK_CITY, -1, -1, true, iMoveFlags, 0, true, false, bIgnoreFaster))
	{
		return;
	}
	
	CvCity* pTargetCity = NULL;
	if( isBarbarian() )
	{
		pTargetCity = AI_pickTargetCity(iMoveFlags, 10); // was 12 (K-Mod)
	}
	else
	{
		//pTargetCity = AI_pickTargetCity(iMoveFlags, MAX_INT, bHuntBarbs);
		// K-Mod. Try to avoid picking a target city in cases where we clearly aren't ready. (just for efficiency.)
		if (bReadyToAttack || bAtWar || (!plot()->isCity() && getGroup()->getNumUnits() > 1))
			pTargetCity = AI_pickTargetCity(iMoveFlags, MAX_INT, bHuntBarbs);
	}

	bool bTargetTooStrong = false; // K-Mod. This is used to prevent the AI from oscillating between moving to attack moving to pillage.
	int iStepDistToTarget = MAX_INT;
	// Note. I've rearranged some parts of the code below, sometimes without comment.
	if (pTargetCity != NULL)
	{
		int iAttackRatio = GC.getBBAI_ATTACK_CITY_STACK_RATIO();
		int iAttackRatioSkipBombard = GC.getBBAI_SKIP_BOMBARD_MIN_STACK_RATIO();
		iStepDistToTarget = stepDistance(pTargetCity->getX_INLINE(), pTargetCity->getY_INLINE(), getX_INLINE(), getY_INLINE());

		// K-Mod - I'm going to scale the attack ratio based on our war strategy
		if (isBarbarian())
		{
			iAttackRatio = 80;
		}
		else
		{
			int iAdjustment = 5;
			iAdjustment += GET_TEAM(getTeam()).AI_getWarPlan(pTargetCity->getTeam()) == WARPLAN_LIMITED ? 10 : 0;			
			iAdjustment += kOwner.AI_isDoStrategy(AI_STRATEGY_CRUSH) ? -10 : 0;
			iAdjustment += iAdjustment >= 0 && pTargetCity == area()->getTargetCity(getOwnerINLINE()) ? -10 : 0;
			iAdjustment += range((GET_TEAM(getTeam()).AI_getEnemyPowerPercent(true)-100)/12, -10, 0);
			iAdjustment += iStepDistToTarget <= 1 &&
					pTargetCity->isOccupation() ?
					range(111-(iAttackRatio+iAdjustment), -10, 0) : 0; // k146
			iAttackRatio += iAdjustment;
			iAttackRatioSkipBombard += iAdjustment;
			FAssert(iAttackRatioSkipBombard >= iAttackRatio);
			FAssert(iAttackRatio >= 100);
		}
		// K-Mod end

		int iComparePostBombard = getGroup()->AI_compareStacks(pTargetCity->plot(), true);
		int iBombardTurns = getGroup()->getBombardTurns(pTargetCity);
		// K-Mod note: AI_compareStacks will try to use the AI memory if it can't see.
		{
			// K-Mod
			// The defense modifier is counted in AI_compareStacks. So if we add it again, we'd be double counting.
			// I'm going to subtract defence, but unfortunately this will reduce based on the total rather than the base.
			int iDefenseModifier = pTargetCity->getDefenseModifier(false);
			int iReducedModifier = iDefenseModifier;
			iReducedModifier *= std::min(20, iBombardTurns);
			iReducedModifier /= 20;
			int iBase = 210 + (pTargetCity->plot()->isHills() ? GC.getHILLS_EXTRA_DEFENSE() : 0);
			// advc.003: Make sure we don't get an overflow here
			double mult = iBase / (double)std::max(1,
					iBase + iReducedModifier - iDefenseModifier); // def. mod. < 200. I promise.
			iComparePostBombard = ::round(mult * iComparePostBombard);
			// iBase > 100 is to offset the over-reduction from compounding.
			// With iBase == 200, bombarding a defence bonus of 100% will reduce effective defence by 50%
		}

		bTargetTooStrong = iComparePostBombard < iAttackRatio;

		if (iStepDistToTarget <= 2)
		{
			// K-Mod. I've rearranged and rewritten most of this section - removing the bbai code.
			if (bTargetTooStrong)
			{
				if (AI_stackVsStack(2, iAttackRatio, 80, iMoveFlags))
					return;

				FAssert(getDomainType() == DOMAIN_LAND);
				int iOurOffense = kOwner.AI_localAttackStrength(plot(), getTeam(), DOMAIN_LAND, 1, false);
				int iEnemyOffense = kOwner.AI_localAttackStrength(plot(), NO_TEAM, DOMAIN_LAND, 2, false);

				// If in danger, seek defensive ground
				if (4*iOurOffense < 3*iEnemyOffense)
				{
					if (AI_omniGroup(UNITAI_ATTACK_CITY, -1, -1, true, iMoveFlags, 3, true, false, bIgnoreFaster, false, false)) // including smaller groups
						return;

					if (iAttackRatio/2 > iComparePostBombard && 4*iEnemyOffense/5 > kOwner.AI_localDefenceStrength(plot(), getTeam()))
					{
						// we don't have anywhere near enough attack power, and we are in serious danger.
						// unfortunately, if we are "bReadyToAttack", we'll probably end up coming straight back here...
						if (!bReadyToAttack && AI_retreatToCity())
							return;
					}
					if (getGroup()->AI_getMissionAIType() == MISSIONAI_PILLAGE &&
							// advc.012:
							AI_plotDefense() > 0)
							//plot()->defenseModifier(getTeam(), false) > 0)
					{
						if (isEnemy(plot()->getTeam()) && canPillage(plot()))
						{
							getGroup()->pushMission(MISSION_PILLAGE, -1, -1, 0, false, false, MISSIONAI_PILLAGE, plot());
							return;
						}
					}

					if (AI_choke(2, true, iMoveFlags))
					{
						return;
					}
				}
				else
				{
					if (AI_omniGroup(UNITAI_ATTACK_CITY, -1, -1, true, iMoveFlags, 3, true, false, bIgnoreFaster)) // bigger groups only
						return;

					if (canBombard(plot()))
					{
						getGroup()->pushMission(MISSION_BOMBARD, -1, -1, 0, false, false, MISSIONAI_ASSAULT, pTargetCity->plot());
						return;
					}

					if (AI_omniGroup(UNITAI_ATTACK_CITY, -1, -1, true, iMoveFlags, 3, true, false, bIgnoreFaster, false, false)) // any size
						return;
				}
			}

			if (iStepDistToTarget == 1)
			{
				// Consider getting into a better position for attack.
				if (iComparePostBombard < GC.getBBAI_SKIP_BOMBARD_BASE_STACK_RATIO() && // only if we don't already have overwhelming force
					(iComparePostBombard < iAttackRatioSkipBombard ||
					pTargetCity->getDefenseDamage() < GC.getMAX_CITY_DEFENSE_DAMAGE()/ 2 ||
					plot()->isRiverCrossing(directionXY(plot(), pTargetCity->plot()))))
				{
					// Only move into attack position if we have a chance.
					// Without this check, the AI can get stuck alternating between this, and pillage.
					// I've tried to roughly take into account how much our ratio would improve by removing a river penalty.
					if ((getGroup()->canBombard(plot()) && iBombardTurns > 2) ||
						(plot()->isRiverCrossing(directionXY(plot(), pTargetCity->plot())) && 150 * iComparePostBombard >= (150 + GC.getRIVER_ATTACK_MODIFIER()) * iAttackRatio))
					{
						if (AI_goToTargetCity(iMoveFlags, 2, pTargetCity))
							return;
					}
					// Note: bombard may skip if stack is powerful enough
					if (AI_bombardCity())
						return;
				}
				else if (iComparePostBombard >= iAttackRatio && AI_bombardCity()) // we're satisfied with our position already. But we still want to consider bombarding.
					return;

				if (iComparePostBombard >= iAttackRatio)
				{
					// in position; and no desire to bombard.  So attack!
					if (AI_stackAttackCity(iAttackRatio))
						return;
				}
			}

			if (iComparePostBombard >= iAttackRatio && AI_goToTargetCity(iMoveFlags, 4, pTargetCity))
				return;
			// K-Mod end
		}
	}

	// K-Mod. Lets have some slightly smarter stack vs. stack AI.
	// it would be nice to have some personality effection here...
	// eg. protective leaders have a lower risk threshold.   -- Maybe later.
	// Note. This stackVsStack stuff used to be a bit lower, after the group and the heal stuff.
	if (getGroup()->getNumUnits() > 1)
	{
		if (bAtWar) // recall that "bAtWar" just means we are in enemy territory.
		{
			// note. if we are 2 steps from the target city, this check here is redundant. (see code above)
			if (AI_stackVsStack(1, 160, 95, iMoveFlags))
				return;
		}
		else
		{
			/*  <advc.300> Replacing literal 4 with 'searchRange'. Reduced
				b/c (New World) barbs who can't find a target city
				shouldn't be aggressive. */
			int searchRange = 4;
			if(isBarbarian() && area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT)
				searchRange = 1; // </advc.300>
			if (eAreaAI == AREAAI_DEFENSIVE && plot()->getOwnerINLINE() == getOwnerINLINE())
			{
				if (AI_stackVsStack(searchRange, 110, 55, iMoveFlags))
					return;
				if (AI_stackVsStack(searchRange, 180, 30, iMoveFlags))
					return;
			}
			else if (AI_stackVsStack(searchRange, 130, 60, iMoveFlags))
				return;
		}
	}

	/*  K-Mod. The loading of units for assault needs to be before the following
		omnigroup - otherwise the units may leave the boat to join their friends. */
	if (bAssault && (pTargetCity == NULL || pTargetCity->area() != area()))
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, iMoveFlags, 6)) // was 4 max-turns
		{
			return;
		}
	}
	// K-Mod end

	//if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 2, true, true, bIgnoreFaster))
	if (AI_omniGroup(UNITAI_ATTACK_CITY, -1, -1, true, iMoveFlags, 2, true, false, bIgnoreFaster))
	{
		return;
	}

	if (AI_heal(30, 1))
	{
		return;
	}

	/* original bts code
	if (collateralDamage() > 0 && plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if (AI_anyAttack(1, 45, iMoveFlags, 3, false, false))
		{
			return;
		}

		if( !bReadyToAttack )
		{
			if (AI_anyAttack(1, 25, iMoveFlags, 5, false))
			{
				return;
			}
		}
	} */

	//if (AI_anyAttack(1, 60, iMoveFlags, 0, false))
	if (AI_anyAttack(1, 60, iMoveFlags | MOVE_SINGLE_ATTACK)) // K-Mod (changed to allow cities, and to only use a single unit, but it is still a questionable move)
	{
		return;
	}

	// K-Mod - replacing some stuff I moved / removed from the BBAI code
	if (pTargetCity && bTargetTooStrong && iStepDistToTarget <= (bReadyToAttack ? 3 : 2))
	{
		// Pillage around enemy city
		if (generatePath(pTargetCity->plot(), iMoveFlags, true, 0, 5))
		{
			// the above path check is just for efficiency.
			// Otherwise we'd be checking every surrounding tile.
			if (AI_pillageAroundCity(pTargetCity, 11, iMoveFlags, 2)) // was 3 turns
				return;

			if (AI_pillageAroundCity(pTargetCity, 0, iMoveFlags, 4)) // was 5 turns
				return;
		}

		// choke the city.
		if (iStepDistToTarget <= 2 && AI_choke(1, false, iMoveFlags))
			return;

		// if we're already standing right next to the city, then goToTargetCity can fail
		// - and we might end up doing something stupid instead. So try again to choke.
		if (iStepDistToTarget <= 1 && AI_choke(3, false, iMoveFlags))
			return;
	}

	// one more thing. Sometimes a single step can cause the AI to change its target city;
	// and when it changes the target - and so sometimes they can get stuck in a loop where
	// they step towards their target, change their mind, step back to pillage something, ... repeat.
	// Here I've made a kludge to break that cycle:
	if (getGroup()->AI_getMissionAIType() == MISSIONAI_PILLAGE)
	{
		CvPlot* pMissionPlot = getGroup()->AI_getMissionAIPlot();
		if (pMissionPlot && canPillage(pMissionPlot) && isEnemy(pMissionPlot->getTeam(), pMissionPlot))
		{
			if (atPlot(pMissionPlot))
			{
				getGroup()->pushMission(MISSION_PILLAGE, -1, -1, 0, false, false, MISSIONAI_PILLAGE, pMissionPlot);
				return;
			}
			if (generatePath(pMissionPlot, iMoveFlags, true, 0, 6))
			{
				/*  the max path turns is arbitrary, but it should be at least as
					big as the pillage sections higher up. */
				CvPlot* pEndTurnPlot = getPathEndTurnPlot();
				FAssert(!atPlot(pEndTurnPlot));
				// warning: this command may attack something. We haven't checked!
				getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX_INLINE(), pEndTurnPlot->getY_INLINE(), iMoveFlags, false, false, MISSIONAI_PILLAGE, pMissionPlot);
				return;
			}
		}
	}
	// K-Mod end

	if (bAtWar && (bTargetTooStrong || getGroup()->getNumUnits() <= 2))
	{
		if (AI_pillageRange(3, 11, iMoveFlags))
		{
			return;
		}

		if (AI_pillageRange(1, 0, iMoveFlags))
		{
			return;
		}
	}

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		/* original bts code
		if (!bLandWar)
		{
			if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, iMoveFlags, 4))
			{
				return;
			}
		} */ // I've moved this to be above the omniGroup stuff, otherwise it just causes AI confusion.

		if( bReadyToAttack )
		{
			// Wait for units about to join our group
			/* original BBAI code
			MissionAITypes eMissionAIType = MISSIONAI_GROUP;
			int iJoiners = kOwner.AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 2);

			if( (iJoiners*5) > getGroup()->getNumUnits() )
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}*/

			// K-Mod. If the target city is close, be less likely to wait for backup.
			int iPathTurns = 10;
			int iMaxWaitTurns = 3;
			if (pTargetCity && generatePath(pTargetCity->plot(), iMoveFlags, true, &iPathTurns, iPathTurns))
				iMaxWaitTurns = (iPathTurns+1) / 3;

			MissionAITypes eMissionAIType = MISSIONAI_GROUP;
			int iJoiners = iMaxWaitTurns > 0 ? kOwner.AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), iMaxWaitTurns) : 0;

			if (iJoiners*range(iPathTurns-1, 2, 5) > getGroup()->getNumUnits())
			{
				getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GROUP); // (the mission is just for debug feedback)
				return;
			}
			// K-Mod end
		}
		else
		{
			if (bTurtle)
			{
				// K-Mod
				if (AI_leaveAttack(1, 51, 100))
					return;

				if (AI_defendTerritory(70, iMoveFlags, 3))
					return;
				// K-Mod end
				if (AI_guardCity(false, true, 7, iMoveFlags))
				{
					return;
				}
			}
			else if (!isBarbarian() && eAreaAI == AREAAI_DEFENSIVE)
			{
				// Use smaller attack city stacks on defense
				// K-Mod
				if (AI_defendTerritory(65, iMoveFlags, 3))
					return;
				// K-Mod end

				if (AI_guardCity(false, true, 3, iMoveFlags))
				{
					return;
				}
			}

			int iTargetCount = kOwner.AI_unitTargetMissionAIs(this, MISSIONAI_GROUP);
			if ((iTargetCount * 5) > getGroup()->getNumUnits())
			{
				MissionAITypes eMissionAIType = MISSIONAI_GROUP;
				int iJoiners = kOwner.AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 2);
				
				if( (iJoiners*5) > getGroup()->getNumUnits() )
				{
					//getGroup()->pushMission(MISSION_SKIP);
					getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GROUP); // K-Mod (for debug feedback)
					return;
				}

				if (AI_moveToStagingCity())
				{
					return;
				}
			}
		}
	}

	if (AI_heal(50, 3))
	{
		return;
	}

	if (!bAtWar)
	{
		if (AI_heal())
		{
			return;
		}

		if ((getGroup()->getNumUnits() == 1) && (getTeam() != plot()->getTeam()))
		{
			if (AI_retreatToCity())
			{
				return;
			}
		}
	}

	if (!bReadyToAttack && !noDefensiveBonus())
	{
		if (AI_guardCity(false, false, MAX_INT, iMoveFlags))
		{
			return;
		}
	}

	if (bReadyToAttack)
	{	// advc.003b: Moved into the bReadyToAttack branch
		bool bAnyWarPlan = (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0);
		/* BBAI code
		if( isBarbarian() )
		{
			if (AI_goToTargetCity(iMoveFlags, 12))
			{
				return;
			}

			if (AI_pillageRange(3, 11, iMoveFlags))
			{
				return;
			}

			if (AI_pillageRange(1, 0, iMoveFlags))
			{
				return;
			}
		}
		else if (bHuntBarbs && AI_goToTargetBarbCity((bAnyWarPlan ? 7 : 12)))
		{
			return;
		}
		else if (bLandWar && pTargetCity != NULL) */
		// K-Mod
		if (isBarbarian())
		{
			if (pTargetCity && AI_goToTargetCity(iMoveFlags, 12, pTargetCity)) // target city has already been calculated.
				return;
			// <advc.300> See previous comment.
			int searchRange = 3;
			if(area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT)
				searchRange = 1; // </advc.300>
			if (AI_pillageRange(searchRange, 0, iMoveFlags))
				return;
		}
		else if (pTargetCity)
		// K-Mod end
		{
			// Before heading out, check whether to wait to allow unit upgrades
			if( bInCity && plot()->getOwnerINLINE() == getOwnerINLINE() )
			{
				//if( !(GET_PLAYER(getOwnerINLINE()).AI_isFinancialTrouble()) )
				if (!kOwner.AI_isFinancialTrouble() && !pTargetCity->isBarbarian())
				{
					// Check if stack has units which can upgrade
					int iNeedUpgradeCount = 0;

					CLLNode<IDInfo>* pUnitNode = getGroup()->headUnitNode();
					while (pUnitNode != NULL)
					{
						CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = getGroup()->nextUnitNode(pUnitNode);

						if( pLoopUnit->getUpgradeCity(false) != NULL )
						{
							iNeedUpgradeCount++;

							if( 5*iNeedUpgradeCount > getGroup()->getNumUnits() ) // was 8*
							{
								getGroup()->pushMission(MISSION_SKIP);
								return;
							}
						}
					}
				}
			}

			// K-Mod. (original bloated code deleted)
			// Estimate the number of turns required.
			int iPathTurns;
			if (!generatePath(pTargetCity->plot(), iMoveFlags, true, &iPathTurns))
			{
				//FAssertMsg(false, "failed to find path to target city."); // AI_pickTargetCity now allows boat-only paths, so this assertion no longer holds.
				iPathTurns = 100;
			}

			if (!pTargetCity->isBarbarian() || iPathTurns < (bAnyWarPlan ? 7 : 12)) // don't bother with long-distance barb attacks
			{
				// See if we can get there faster by boat..
				if (iPathTurns > 5)// && !pTargetCity->isBarbarian())
				{
				/*  note: if the only land path to our target happens to go
					through a tough line of defence...
					we probably want to take the boat even if our iPathTurns is
					low. Here's one way to account for that:
					iPathTurns = std::max(iPathTurns, getPathLastNode()->
					m_iTotalCost / (2000*GC.getMOVE_DENOMINATOR()));
					Unfortunately, that "2000"... well I think you know what the
					problem is. So maybe next time. */
					int iLoadTurns = std::max(3, iPathTurns/3 - 1); // k146
					int iMaxTransportTurns = iPathTurns - iLoadTurns - 2;

					if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, iMoveFlags, iLoadTurns, iMaxTransportTurns))
						return;
				}
				// We have to walk.
				if (AI_goToTargetCity(iMoveFlags, MAX_INT, pTargetCity))
				{
					return;
				}

				if (bAnyWarPlan)
				{
					// We're at war, but we failed to walk to the target. Before we start wigging out, lets just check one more thing...
					if (bTargetTooStrong && iStepDistToTarget == 1)
					{
						// we're standing outside the city already, but we can't capture it and we can't pillage or choke it.
						// I guess we'll just wait for reinforcements to arrive.
						if (AI_safety())
							return;
						getGroup()->pushMission(MISSION_SKIP);
						return;
					}

					//CvCity* pTargetCity =
					/*  advc.003: This shadows the pTargetCity that the rest of this
						function cares about (and which isn't necessarily in this
						unit's area). I think it doesn't really matter if we reassign
						pTargetCity here or use a new variable b/c the only remaining
						code that cares about pTargetCity has to do with airlifts, and
						we already know that AI_safety fails ... */
					CvCity* pAreaTargetCity =
							area()->getTargetCity(getOwnerINLINE());
					if (pAreaTargetCity != NULL)
					{
						/*  this is a last resort. I don't expect that we'll ever actually need it.
							(it's a pretty ugly function, so I /hope/ we don't need it.) */
						FAssertMsg(false, "AI_attackCityMove is resorting to AI_solveBlockageProblem");
						if (AI_solveBlockageProblem(pAreaTargetCity->plot(),
								(GET_TEAM(getTeam()).getAtWarCount(true) == 0)))
						{
							return;
						}
						// advc.006:
						FAssertMsg(false, "AI_solveBlockageProblem returned false");
					}
				}
			}
			// K-Mod end
		}
	}
	else
	{
		/* original code
		int iTargetCount = kOwner.AI_unitTargetMissionAIs(this, MISSIONAI_GROUP);
		if( ((iTargetCount * 4) > getGroup()->getNumUnits()) || ((getGroup()->getNumUnits() + iTargetCount) >= (bHuntBarbs ? 3 : AI_stackOfDoomExtra())) )
		{
			MissionAITypes eMissionAIType = MISSIONAI_GROUP;
			int iJoiners = kOwner.AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 2);
			
			if (6*iJoiners > getGroup()->getNumUnits())
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}*/
		// K-Mod
		int iTargetCount = kOwner.AI_unitTargetMissionAIs(this, MISSIONAI_GROUP);
		if (6*iTargetCount > getGroup()->getNumUnits())
		{
			MissionAITypes eMissionAIType = MISSIONAI_GROUP;
			int iNearbyJoiners = kOwner.AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 2);

			if (4*iNearbyJoiners > getGroup()->getNumUnits())
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
		// K-Mod end

		if ((bombardRate() > 0) && noDefensiveBonus())
		{
			// BBAI Notes: Add this stack lead by bombard unit to stack probably not lead by a bombard unit
			// BBAI TODO: Some sense of minimum stack size?  Can have big stack moving 10 turns to merge with tiny stacks
			//if (AI_group(UNITAI_ATTACK_CITY, -1, -1, -1, bIgnoreFaster, true, true, /*iMaxPath*/ 10, /*bAllowRegrouping*/ true))
			if (AI_omniGroup(UNITAI_ATTACK_CITY, -1, -1, true, iMoveFlags, 10, true, getGroup()->getNumUnits() < 2, bIgnoreFaster, true, true))
			{
				return;
			}
		}
		else
		{
			//if (AI_group(UNITAI_ATTACK_CITY, AI_stackOfDoomExtra() * 2, -1, -1, bIgnoreFaster, true, true, /*iMaxPath*/ 10, /*bAllowRegrouping*/ false))
			if (AI_omniGroup(UNITAI_ATTACK_CITY, AI_stackOfDoomExtra() * 2, -1, true, iMoveFlags, 10, true, getGroup()->getNumUnits() < 2, bIgnoreFaster, false, true))
			{
				return;
			}
		}
	}

	if (plot()->getOwnerINLINE() == getOwnerINLINE() && bLandWar)
	{
		if( (GET_TEAM(getTeam()).getAtWarCount(true) > 0) )
		{
			// if no land path to enemy cities, try getting there another way
			if (AI_offensiveAirlift())
			{
				return;
			}

			if( pTargetCity == NULL )
			{
				if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, iMoveFlags, 4))
				{
					return;
				}
			}
		}
	}

	// K-Mod
	if (AI_defendTerritory(70, iMoveFlags, 1, true))
		return;
	// K-Mod end

	if (AI_moveToStagingCity())
	{
		return;
	}

	if (AI_offensiveAirlift())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	// K-Mod
	if (AI_handleStranded())
		return;
	// K-Mod end

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}

void CvUnitAI::AI_attackCityLemmingMove()
{
	if (AI_cityAttack(1, 80)) 
	{ 
		return;
	} 

	if (AI_bombardCity())
	{ 
		return;
	} 

	if (AI_cityAttack(1, 40)) 
	{ 
		return;
	} 

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/29/10                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if (AI_goToTargetCity(MOVE_THROUGH_ENEMY))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	{ 
		return;
	} 

	if (AI_anyAttack(1, 70)) 
	{ 
		return;
	} 

	if (AI_anyAttack(1, 0)) 
	{ 
		return;
	} 

	getGroup()->pushMission(MISSION_SKIP);
}


void CvUnitAI::AI_collateralMove()
{
	PROFILE_FUNC();

	// K-Mod!
	if (AI_defensiveCollateral(51, 3))
		return;
	// K-Mod end
	
	if (AI_leaveAttack(1, 30, 100)) // was 20
	{
		return;
	}

	if (AI_guardCity(false, true, 1))
	{
		return;
	}

	if (AI_heal(30, 1))
	{
		return;
	}

	if (AI_cityAttack(1, 35))
	{
		return;
	}

	/*if (AI_anyAttack(1, 45, 0, 3))
	{
		return;
	}*/

	if (AI_anyAttack(1, 55, 0, 2))
	{
		return;
	}

	if (AI_anyAttack(1, 35, 0, 3))
	{
		return;
	}

	/* original bts code
	if (AI_anyAttack(1, 30, 0, 4))
	{
		return;
	}

	if (AI_anyAttack(1, 20, 5))
	{
		return;
	} */

	// K-Mod
	{
		// count our collateral damage units on this plot
		int iTally = 0;
		CLLNode<IDInfo>* pUnitNode = plot()->headUnitNode();
		while (pUnitNode != NULL)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);

			if (DOMAIN_LAND == pLoopUnit->getDomainType() && pLoopUnit->getOwner() == getOwner()
				&& pLoopUnit->canMove() && pLoopUnit->collateralDamage() > 0)
			{
				iTally++;
			}

			pUnitNode = plot()->nextUnitNode(pUnitNode);
		}
		FAssert(iTally > 0);
		FAssert(collateralDamageMaxUnits() > 0);

		int iDangerModifier = 100;
		do
		{
			int iMinOdds = 80 / (3 + iTally);
			iMinOdds *= 100;
			iMinOdds /= iDangerModifier;
			if (AI_anyAttack(1, iMinOdds, 0, std::min(2*collateralDamageMaxUnits(), collateralDamageMaxUnits() + iTally - 1)))
			{
				return;
			}
			// Try again with just half the units, just in case our only problem is that we can't find a big enough target stack.
			iTally = (iTally-1)/2;
		} while (iTally > 1);
	}
	// K-Mod end

	if (AI_heal())
	{
		return;
	}

	/*if (!noDefensiveBonus())
	{
		if (AI_guardCity(false, false))
		{
			return;
		}
	}*/ // redundant

	if (AI_anyAttack(2, 55, 0, 3))
	{
		return;
	}

	if (AI_cityAttack(2, 50))
	{
		return;
	}

	/* original bts code. (check again with a stricter threshold -> a waste of time)
	if (AI_anyAttack(2, 60))
	{
		return;
	}*/
	// K-Mod
	if (area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE)
	{
		const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
		// if more than a third of our floating defenders are collateral units, convert this one to city attack
		if (3 * kOwner.AI_totalAreaUnitAIs(area(), UNITAI_COLLATERAL) > kOwner.AI_getTotalFloatingDefenders(area()))
		{
			if (kOwner.AI_unitValue(getUnitType(), UNITAI_ATTACK_CITY, area()) > 0)
			{
				AI_setUnitAIType(UNITAI_ATTACK_CITY);
				return; // no mission pushed.
			}
		}
	}
	// K-Mod end

	//if (AI_protect(50))
	if (AI_defendTerritory(55, 0, 6)) // K-Mod
	{
		return;
	}

	if (AI_guardCity(false, true, 8)) // was 3
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	// K-Mod
	if (AI_handleStranded())
		return;
	// K-Mod end

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}

void CvUnitAI::AI_pillageMove()
{
	PROFILE_FUNC();

	if (AI_guardCity(false, true, 2)) // was 1
	{
		return;
	}

	if (AI_heal(30, 1))
	{
		return;
	}

	// BBAI TODO: Shadow ATTACK_CITY stacks and pillage

	//join any city attacks in progress
	if (plot()->isOwned() && plot()->getOwnerINLINE() != getOwnerINLINE())
	{
		if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 1, true, true))
		{
			return;
		}
	}
	
	/*if (AI_cityAttack(1, 55))
	{
		return;
	}*/

	// K-Mod. Pillage units should focus on pillaging, when possible.
	// note: having 2 moves doesn't necessarily mean we can move & pillage in the same turn, but it's a good enough approximation.
	if (AI_pillageRange(getGroup()->baseMoves() > 1 ? 1 : 0, 11))
	{
		return;
	}
	// K-Mod end
	
	if (AI_anyAttack(1, 65))
	{
		return;
	}

	if (!noDefensiveBonus())
	{
		if (AI_guardCity(false, false))
		{
			return;
		}
	}

	if (AI_pillageRange(3, 11))
	{
		return;
	}
	
	if (AI_choke(1))
	{
		return;
	}

	if (AI_pillageRange(1))
	{
		return;
	}

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, UNITAI_ATTACK, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
		{
			return;
		}
	}

	if (AI_heal(50, 3))
	{
		return;
	}

	if (!isEnemy(plot()->getTeam()))
	{
		if (AI_heal())
		{
			return;
		}
	}

	//if (AI_group(UNITAI_PILLAGE, /*iMaxGroup*/ 1, /*iMaxOwnUnitAI*/ 1, -1, /*bIgnoreFaster*/ true, false, false, /*iMaxPath*/ 3))
	if (AI_group(UNITAI_PILLAGE, /*iMaxGroup*/ 2, /*iMaxOwnUnitAI*/ 1, -1, /*bIgnoreFaster*/ true, false, false, /*iMaxPath*/ 3)) // K-Mod. (later, I might tell counter units to join up.)
	{
		return;
	}

	if ((area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE) || isEnemy(plot()->getTeam()))
	{
		if (AI_pillage(20))
		{
			return;
		}
	}

	if (AI_heal())
	{
		return;
	}

	if (AI_guardCity(false, true, 3))
	{
		return;
	}

	if (AI_offensiveAirlift())
	{
		return;
	}

	if (AI_travelToUpgradeCity())
	{
		return;
	}

	if( !isHuman() && plot()->isCoastalLand() &&
			/*  advc.046: SKIP w/o setting eMissionAI would make the group forget
				that it's stranded, and then AI_pickupStranded won't find it. */ 
			!getGroup()->isStranded() &&
			GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_PICKUP) > 0 )
	{
		getGroup()->pushMission(MISSION_SKIP);
		return;
	}

	// K-Mod
	if (plot()->getTeam() == getTeam() && AI_defendTerritory(55, 0, 3, true))
		return;

	if (AI_handleStranded())
		return;
	// K-Mod end

	if (AI_patrol())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_reserveMove()
{
	PROFILE_FUNC();

	// K-Mod
	if (AI_guardCityOnlyDefender())
		return;
	// K-Mod end

	bool bDanger = (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 3));

	/* original bts code
	if (bDanger && AI_leaveAttack(2, 55, 130))
	{
		return;
	} */
	// K-Mod
	if (plot()->getTeam() == getTeam() && (bDanger || area()->getAreaAIType(getTeam()) != AREAAI_NEUTRAL))
	{
		if (bDanger && plot()->isCity())
		{
			if (AI_leaveAttack(1, 55, 110))
				return;
		}
		else
		{
			if (AI_defendTerritory(65, 0, 2, true))
				return;
		}
	}
	else
	{
		if (AI_anyAttack(1, 65))
			return;
	}
	// K-Mod end

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_SETTLE, -1, -1, 1, -1, MOVE_SAFE_TERRITORY))
		{
			return;
		}
		if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_WORKER, -1, -1, 1, -1, MOVE_SAFE_TERRITORY))
		{
			return;
		}
	}

	if (!bDanger || !plot()->isOwned()) // K-Mod
	{
		if (AI_group(UNITAI_SETTLE, 2, -1, -1, false, false, false, 3, true))
		{
			return;
		}
	}
	// <advc.314> Can be important for huts near colonies
	if(!bDanger && AI_goody(3))
		return; // </advc.314>
	if (AI_guardCity(true))
	{
		return;
	}

	if (!noDefensiveBonus())
	{
		if (AI_guardFort(false))
		{
			return;
		}
	}

	if (AI_guardCityAirlift())
	{
		return;
	}

	if (AI_guardCity(false, true, 2)) // was 1
	{
		return;
	}

	// <advc.300> Protect high-yield tiles from Barbarians
	if(GET_PLAYER(getOwnerINLINE()).AI_isDefenseFocusOnBarbarians(
			area()->getID()) && AI_guardYield())
		return;
	// Moved up. Shouldn't travel to future city site while badly injured.
	if(AI_heal(30, 1))
		return;
	// </advc.300>

	if (AI_guardCitySite())
	{
		return;
	}

	if (!noDefensiveBonus())
	{
		if (AI_guardFort(true))
		{
			return;
		}

		if (AI_guardBonus(15))
		{
			return;
		}
	}

	// advc.300: Moved up
	/*if (AI_heal(30, 1))
	{
		return;
	}*/

	/*if (bDanger)
	{
		if (AI_cityAttack(1, 55))
		{
			return;
		}

		if (AI_anyAttack(1, 60))
		{
			return;
		}
	}

	if (!noDefensiveBonus())
	{
		if (AI_guardCity(false, false))
		{
			return;
		}
	}*/ // disabled by K-Mod. (redundant)
	
	if (bDanger)
	{
		/*if (AI_cityAttack(3, 45))
		{
			return;
		}*/

		if (AI_anyAttack(3, 50))
		{
			return;
		}
	}

	//if (AI_protect(45))
	if (AI_defendTerritory(45, 0, 8)) // K-Mod
	{
		return;
	}

	//if (AI_guardCity(false, true, 3))
	if (AI_guardCity(false, true)) // K-Mod
	{
		return;
	}

	if (AI_defend())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	// K-Mod
	if (AI_handleStranded())
		return;
	// K-Mod end

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_counterMove()
{
	PROFILE_FUNC();

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/03/10                                jdog5000      */
/*                                                                                              */
/* Unit AI, Settler AI                                                                          */
/************************************************************************************************/
	// Should never have group lead by counter unit
	if( getGroup()->getNumUnits() > 1 )
	{
		UnitAITypes eGroupAI = getGroup()->getHeadUnitAI();
		if( eGroupAI == AI_getUnitAIType() )
		{
			if( plot()->isCity() && plot()->getOwnerINLINE() == getOwnerINLINE() )
			{
				//FAssert(false); // just interested in when this happens, not a problem
				getGroup()->AI_separate(); // will change group
				return;
			}
		}
	}

	if( !(plot()->isOwned()) )
	{
		if( AI_groupMergeRange(UNITAI_SETTLE, 2, true, false, false) )
		{
			return;
		}
	}

	// K-Mod
	bool bDanger = GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 3);
	if (bDanger && plot()->getTeam() == getTeam())
	{
		if (plot()->isCity())
		{
			if (AI_leaveAttack(1, 65, 115))
				return;
		}
		else
		{
			if (AI_defendTerritory(70, 0, 2, true))
				return;
		}
	}
	// K-Mod end

	//if (AI_guardCity(false, true, 1))
	if (AI_guardCity(false, true, 2)) // K-Mod
	{
		return;
	}
	
	if (getSameTileHeal() > 0)
	{
		if (!canAttack())
		{
			// Don't restrict to groups carrying cargo ... does this apply to any units in standard bts anyway?
			if (AI_shadow(UNITAI_ATTACK_CITY, -1, 21, false, false, 4))
			{
				return;
			}
		}
	}

	AreaAITypes eAreaAIType = area()->getAreaAIType(getTeam());

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if( !bDanger )
		{
			if (plot()->isCity())
			{
				if ((eAreaAIType == AREAAI_ASSAULT) || (eAreaAIType == AREAAI_ASSAULT_ASSIST))
				{
					if (AI_offensiveAirlift())
					{
						return;
					}
				}
			}
		
			if( (eAreaAIType == AREAAI_ASSAULT) || (eAreaAIType == AREAAI_ASSAULT_ASSIST) || (eAreaAIType == AREAAI_ASSAULT_MASSING) )
			{
				if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, UNITAI_ATTACK_CITY, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
				{
					return;
				}

				if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, UNITAI_ATTACK, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
				{
					return;
				}
			}
		}

		/*if (!noDefensiveBonus())
		{
			if (AI_guardCity(false, false))
			{
				return;
			}
		} */ // disabled by K-Mod. This is redundant.
	}

	//join any city attacks in progress
	if (plot()->getOwnerINLINE() != getOwnerINLINE())
	{
		if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 1, true, true))
		{
			return;
		}
	}

	if (bDanger)
	{
		/*if (AI_cityAttack(1, 35))
		{
			return;
		}*/ // disabled by K-Mod

		if (AI_anyAttack(1, 40))
		{
			return;
		}
	}
	
	bool bIgnoreFasterStacks = false;
	if (GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_LAND_BLITZ))
	{
		if (area()->getAreaAIType(getTeam()) != AREAAI_ASSAULT)
		{
			bIgnoreFasterStacks = true;
		}
	}

	if (AI_group(UNITAI_ATTACK_CITY, /*iMaxGroup*/ -1, 2, -1, bIgnoreFasterStacks, /*bIgnoreOwnUnitType*/ true, /*bStackOfDoom*/ true, /*iMaxPath*/ 6))
	{
		return;
	}
	
	bool bFastMovers = (GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_FASTMOVERS));
	if (AI_group(UNITAI_ATTACK, /*iMaxGroup*/ 2, -1, -1, bFastMovers, /*bIgnoreOwnUnitType*/ true, /*bStackOfDoom*/ true, /*iMaxPath*/ 5))
	{
		return;
	}

	// BBAI TODO: merge with nearby pillage
	
	//if (AI_guardCity(false, true, 3))
	if (AI_guardCity(true, true, 5)) // K-Mod
	{
		return;
	}

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if( !bDanger )
		{
			if( (eAreaAIType != AREAAI_DEFENSIVE) )
			{
				if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, UNITAI_ATTACK_CITY, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
				{
					return;
				}

				if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, UNITAI_ATTACK, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
				{
					return;
				}
			}
		}
	}

	if (AI_heal())
	{
		return;
	}

	if (AI_offensiveAirlift())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	// K-Mod
	if (AI_handleStranded())
		return;
	// K-Mod end

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
}


void CvUnitAI::AI_cityDefenseMove()
{
	PROFILE_FUNC();
	
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
	//bool bDanger = (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 3) > 0);
	bool bDanger = (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 3));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Settler AI                                                                                   */
/************************************************************************************************/
	if( !(plot()->isOwned()) )
	{
		if (AI_group(UNITAI_SETTLE, 1, -1, -1, false, false, false, 2, true))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (bDanger)
	{
		if (AI_leaveAttack(1, 70, 140)) // was ,,175
		{
			return;
		}

		if (AI_chokeDefend())
		{
			return;
		}
	}

	if (AI_guardCityBestDefender())
	{
		return;
	}
	
	if (!bDanger)
	{
		if (plot()->getOwnerINLINE() == getOwnerINLINE())
		{
			if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_SETTLE, -1, -1, 1, -1, MOVE_SAFE_TERRITORY, 1))
			{
				return;
			}
		}
	}
	
	if (AI_guardCityMinDefender(true))
	{
		return;
	}

	if (AI_guardCity(true))
	{
		return;
	}
	
	if (!bDanger)
	{
		if (AI_group(UNITAI_SETTLE, /*iMaxGroup*/ 1, -1, -1, false, false, false, /*iMaxPath*/ 2, /*bAllowRegrouping*/ true))
		{
			return;
		}

		if (AI_group(UNITAI_SETTLE, /*iMaxGroup*/ 2, -1, -1, false, false, false, /*iMaxPath*/ 2, /*bAllowRegrouping*/ true))
		{
			return;
		}

		if (plot()->getOwnerINLINE() == getOwnerINLINE())
		{
			if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_SETTLE, -1, -1, 1, -1, MOVE_SAFE_TERRITORY))
			{
				return;
			}
		}
	}
	
	AreaAITypes eAreaAI = area()->getAreaAIType(getTeam());
	if ((eAreaAI == AREAAI_ASSAULT) || (eAreaAI == AREAAI_ASSAULT_MASSING) || (eAreaAI == AREAAI_ASSAULT_ASSIST))
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, UNITAI_ATTACK_CITY, -1, -1, -1, 0, MOVE_SAFE_TERRITORY))
		{
			return;
		}
	}

	if ((AI_getBirthmark() % 4) == 0)
	{
		if (AI_guardFort())
		{
			return;
		}
	}

	if (AI_guardCityAirlift())
	{
		return;
	}

	if (AI_guardCity(false, true, 1))
	{
		return;
	}

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_SETTLE, 3, -1, -1, -1, MOVE_SAFE_TERRITORY))
		{
			// will enter here if in danger
			return;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/02/10                                jdog5000      */
/*                                                                                              */
/* City AI                                                                                      */
/************************************************************************************************/
	//join any city attacks in progress
	/*if (plot()->getOwnerINLINE() != getOwnerINLINE())
	{
		if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 1, true, true))
		{
			return;
		}
	}*/ // disabled by K-Mod (how often do you think this is going to help us?)
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_guardCity(false, true))
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/04/10                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if (!isBarbarian() && ((area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE) || (area()->getAreaAIType(getTeam()) == AREAAI_MASSING)))
	{
		bool bIgnoreFaster = false;
		if (GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_LAND_BLITZ))
		{
			if (area()->getAreaAIType(getTeam()) != AREAAI_ASSAULT)
			{
				bIgnoreFaster = true;
			}
		}

		if (AI_group(UNITAI_ATTACK_CITY, -1, 2, 4, bIgnoreFaster))
		{
			return;
		}
	}
	
	if (area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT)
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, UNITAI_ATTACK_CITY, 2, -1, -1, 1, MOVE_SAFE_TERRITORY))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_retreatToCity())
	{
		return;
	}

	// K-Mod
	if (AI_handleStranded())
		return;
	// K-Mod end

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_cityDefenseExtraMove()
{
	PROFILE_FUNC();

	CvCity* pCity;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Settler AI                                                                                   */
/************************************************************************************************/
	if( !(plot()->isOwned()) )
	{
		if (AI_group(UNITAI_SETTLE, 1, -1, -1, false, false, false, 1, true))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_leaveAttack(2, 55, 150))
	{
		return;
	}
	
	if (AI_chokeDefend())
	{
		return;
	}

	if (AI_guardCityBestDefender())
	{
		return;
	}

	if (AI_guardCity(true))
	{
		return;
	}

	if (AI_group(UNITAI_SETTLE, /*iMaxGroup*/ 1, -1, -1, false, false, false, /*iMaxPath*/ 2, /*bAllowRegrouping*/ true))
	{
		return;
	}

	if (AI_group(UNITAI_SETTLE, /*iMaxGroup*/ 2, -1, -1, false, false, false, /*iMaxPath*/ 2, /*bAllowRegrouping*/ true))
	{
		return;
	}

	pCity = plot()->getPlotCity();

	if ((pCity != NULL) && (pCity->getOwnerINLINE() == getOwnerINLINE())) // XXX check for other team?
	{
		if (plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isUnitAIType, AI_getUnitAIType()) == 1)
		{
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	}

	if (AI_guardCityAirlift())
	{
		return;
	}

	if (AI_guardCity(false, true, 1))
	{
		return;
	}

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_SETTLE, 3, -1, -1, -1, MOVE_SAFE_TERRITORY, 3))
		{
			return;
		}
	}

	if (AI_guardCity(false, true))
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_exploreMove()
{
	PROFILE_FUNC();

	if (!isHuman() && canAttack())
	{
		/*if (AI_cityAttack(1, 60))
		{
			return;
		}*/ // disabled by K-Mod

		if (AI_anyAttack(1, 70))
		{
			return;
		}
	}

	if (getDamage() > 0)
	{
		if ((plot()->getFeatureType() == NO_FEATURE) || (GC.getFeatureInfo(plot()->getFeatureType()).getTurnDamage() == 0))
		{
			getGroup()->pushMission(MISSION_HEAL);
			return;
		}
	}

	if (!isHuman())
	{
		//if (AI_pillageRange(1))
		if (AI_pillageRange(3, 10)) // K-Mod
		{
			return;
		}

		if (AI_cityAttack(3, 80))
		{
			return;
		}
	}

	if (AI_goody(4))
	{
		return;
	}

	if (AI_exploreRange(3))
	{
		return;
	}

	if (!isHuman())
	{
		if (AI_pillageRange(3))
		{
			return;
		}
	}

	if (AI_explore())
	{
		return;
	}

	if (!isHuman())
	{
		if (AI_pillage())
		{
			return;
		}
	}

	if (!isHuman())
	{
		if (AI_travelToUpgradeCity())
		{
			return;
		}
	} // <advc.315> Idle explorers can help guard city sites
	if(!isHuman() && !GC.getGameINLINE().isOption(GAMEOPTION_NO_BARBARIANS) &&
			(m_pUnitInfo->getCombat() * (100 + barbarianCombatModifier())) / 100 >=
			(GET_PLAYER(BARBARIAN_PLAYER).getCurrentEra() + 1) * 2) {
		if(AI_guardCitySite())
			return; 
	} // </advc.315>
	if (!isHuman() && (AI_getUnitAIType() == UNITAI_EXPLORE))
	{
		if (GET_PLAYER(getOwnerINLINE()).AI_totalAreaUnitAIs(area(), UNITAI_EXPLORE) > GET_PLAYER(getOwnerINLINE()).AI_neededExplorers(area()))
		{
			if (GET_PLAYER(getOwnerINLINE()).calculateUnitCost() > 0)
			{
				// K-Mod. Maybe we can still use this unit.
				if (GET_PLAYER(getOwnerINLINE()).AI_unitValue(getUnitType(), UNITAI_ATTACK, area()) > 0)
				{
					AI_setUnitAIType(UNITAI_ATTACK);
				}
				else
				// K-Mod end
				{
					scrap();
				}
				return;
			}
		}
	}
	// K-Mod
	if (AI_handleStranded())
		return;
	// K-Mod end

	if (AI_patrol())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_missionaryMove()
{
	PROFILE_FUNC();

	// K-Mod. Split up groups of automated missionaries - automate them individually.
	if (getGroup()->getNumUnits() > 1)
	{
		AutomateTypes eAutomate = getGroup()->getAutomateType();
		FAssert(isHuman() && eAutomate != NO_AUTOMATE);

		CLLNode<IDInfo>* pEntityNode = getGroup()->headUnitNode();
		while (pEntityNode)
		{
			CvUnit* pLoopUnit = ::getUnit(pEntityNode->m_data);
			pEntityNode = getGroup()->nextUnitNode(pEntityNode);

			if (pLoopUnit->canAutomate(eAutomate))
			{
				pLoopUnit->joinGroup(0, true);
				pLoopUnit->automate(eAutomate);
			}
		}
		return;
	}
	// K-Mod end

	if (AI_spreadReligion())
	{
		return;
	}

	if (AI_spreadCorporation())
	{
		return;
	}

	if (!isHuman() || (isAutomated() && GET_TEAM(getTeam()).getAtWarCount(true) == 0))
	{
		if (!isHuman() || (getGameTurnCreated() < GC.getGame().getGameTurn()))
		{
			if (AI_spreadReligionAirlift())
			{
				return;
			}
			if (AI_spreadCorporationAirlift())
			{
				return;
			}
		}
		
		if (!isHuman())
		{
			/*if (AI_load(UNITAI_MISSIONARY_SEA, MISSIONAI_LOAD_SPECIAL, NO_UNITAI, -1, -1, -1, 0, MOVE_SAFE_TERRITORY))
			{
				return;
			}*/

			if (AI_load(UNITAI_MISSIONARY_SEA, MISSIONAI_LOAD_SPECIAL, NO_UNITAI, -1, -1, -1, 0, MOVE_NO_ENEMY_TERRITORY))
			{
				return;
			}
		}
	}

	//if (AI_retreatToCity())
	if (AI_retreatToCity(false, true)) // K-Mod
	{
		return;
	}

	// K-Mod
	if (AI_handleStranded())
		return;
	// K-Mod end

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_prophetMove()
{
	PROFILE_FUNC();

	/* original bts code
	if (AI_construct(1))
	{
		return;
	}

	if (AI_discover(true, true))
	{
		return;
	}

	if (AI_construct(3))
	{
		return;
	}
	
	int iGoldenAgeValue = (GET_PLAYER(getOwnerINLINE()).AI_calculateGoldenAgeValue() / (GET_PLAYER(getOwnerINLINE()).unitsRequiredForGoldenAge()));
	int iDiscoverValue = std::max(1, getDiscoverResearch(NO_TECH));

	if (((iGoldenAgeValue * 100) / iDiscoverValue) > 60)
	{
        if (AI_goldenAge())
        {
            return;
        }

        if (iDiscoverValue > iGoldenAgeValue)
        {
            if (AI_discover())
            {
                return;
            }
            if (GET_PLAYER(getOwnerINLINE()).getUnitClassCount(getUnitClassType()) > 1)
            {
                if (AI_join())
                {
                    return;
                }
            }
        }
	}
	else
	{
		if (AI_discover())
		{
			return;
		}

		if (AI_join())
		{
			return;
		}
	} */
	// K-Mod
	if (AI_greatPersonMove())
		return;

	/* original bts code
	if ((GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2)) ||
		(getGameTurnCreated() < (GC.getGameINLINE().getGameTurn() - 25))) */
	if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2)) // K-Mod (there are good reasons for saving a great person)
	{
		if (AI_discover())
		{
			return;
		}
	}

	if (AI_retreatToCity())
	{
		return;
	}

	// K-Mod
	if (AI_handleStranded())
		return;
	// K-Mod end

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_artistMove()
{
	PROFILE_FUNC();

	/* original bts code
	if (AI_artistCultureVictoryMove())
	{
	    return;
	}

	if (AI_construct())
	{
		return;
	}

	if (AI_discover(true, true))
	{
		return;
	}

	if (AI_greatWork())
	{
		return;
	}

	int iGoldenAgeValue = (GET_PLAYER(getOwnerINLINE()).AI_calculateGoldenAgeValue() / (GET_PLAYER(getOwnerINLINE()).unitsRequiredForGoldenAge()));
	int iDiscoverValue = std::max(1, getDiscoverResearch(NO_TECH));

	if (((iGoldenAgeValue * 100) / iDiscoverValue) > 60)
	{
        if (AI_goldenAge())
        {
            return;
        }

        if (iDiscoverValue > iGoldenAgeValue)
        {
            if (AI_discover())
            {
                return;
            }
            if (GET_PLAYER(getOwnerINLINE()).getUnitClassCount(getUnitClassType()) > 1)
            {
                if (AI_join())
                {
                    return;
                }
            }
        }
	}
	else
	{
		if (AI_discover())
		{
			return;
		}

		if (AI_join())
		{
			return;
		}
	} */

	/* original bts code
	if ((GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2)) ||
		(getGameTurnCreated() < (GC.getGameINLINE().getGameTurn() - 25))) */

	// K-Mod
	if (AI_greatPersonMove())
		return;

	if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2))
	{
		if (AI_discover())
		{
			return;
		}
	}
	// K-Mod end

	if (AI_retreatToCity())
	{
		return;
	}

	// K-Mod
	if (AI_handleStranded())
		return;
	// K-Mod end

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_scientistMove()
{
	PROFILE_FUNC();

	/* original bts code
	if (AI_discover(true, true))
	{
		return;
	}

	if (AI_construct(MAX_INT, 1))
	{
		return;
	}
	if (GET_PLAYER(getOwnerINLINE()).getCurrentEra() < 3)
	{
		if (AI_join(2))
		{
			return;
		}		
	}
	
	if (GET_PLAYER(getOwnerINLINE()).getCurrentEra() <= (GC.getNumEraInfos() / 2))
	{
		if (AI_construct())
		{
			return;
		}
	}

	int iGoldenAgeValue = (GET_PLAYER(getOwnerINLINE()).AI_calculateGoldenAgeValue() / (GET_PLAYER(getOwnerINLINE()).unitsRequiredForGoldenAge()));
	int iDiscoverValue = std::max(1, getDiscoverResearch(NO_TECH));

	if (((iGoldenAgeValue * 100) / iDiscoverValue) > 60)
	{
        if (AI_goldenAge())
        {
            return;
        }

        if (iDiscoverValue > iGoldenAgeValue)
        {
            if (AI_discover())
            {
                return;
            }
            if (GET_PLAYER(getOwnerINLINE()).getUnitClassCount(getUnitClassType()) > 1)
            {
                if (AI_join())
                {
                    return;
                }
            }
        }
	}
	else
	{
		if (AI_discover())
		{
			return;
		}

		if (AI_join())
		{
			return;
		}
	}

	if ((GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2)) ||
		(getGameTurnCreated() < (GC.getGameINLINE().getGameTurn() - 25)))
	{
		if (AI_discover())
		{
			return;
		}
	} */
	// K-Mod
	if (AI_greatPersonMove())
		return;

	if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2))
	{
		if (AI_discover())
		{
			return;
		}
	}
	// K-Mod end

	if (AI_retreatToCity())
	{
		return;
	}

	// K-Mod
	if (AI_handleStranded())
		return;
	// K-Mod end

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_generalMove()
{
	PROFILE_FUNC();

	std::vector<UnitAITypes> aeUnitAITypes;
	int iDanger = GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 2);
	
	bool bOffenseWar = (area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE);
	
	
	if (iDanger > 0)
	{
		aeUnitAITypes.clear();
		aeUnitAITypes.push_back(UNITAI_ATTACK);
		aeUnitAITypes.push_back(UNITAI_COUNTER);
		if (AI_lead(aeUnitAITypes))
		{
			return;
		}
	}
	
	if (AI_construct(1))
	{
		return;
	}
	if (AI_join(1))
	{
		return;
	}
	
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/14/10                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if (bOffenseWar && (AI_getBirthmark() % 2 == 0))
	{
		aeUnitAITypes.clear();
		aeUnitAITypes.push_back(UNITAI_ATTACK_CITY);
		if (AI_lead(aeUnitAITypes))
		{
			return;
		}

		aeUnitAITypes.clear();
		aeUnitAITypes.push_back(UNITAI_ATTACK);
		if (AI_lead(aeUnitAITypes))
		{
			return;
		}
	}
	
	if (AI_join(2))
	{
		return;
	}
	
	if (AI_construct(2))
	{
		return;
	}
	if (AI_join(4))
	{
		return;
	}
	
	if (GC.getGameINLINE().getSorenRandNum(3, "AI General Construct") == 0)
	{
		if (AI_construct())
		{
			return;
		}
	}

	if (AI_join())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	// K-Mod
	if (AI_handleStranded())
		return;
	// K-Mod end
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_merchantMove()
{
	PROFILE_FUNC();

	/* original bts code
	if (AI_construct())
	{
		return;
	}

	if (AI_discover(true, true))
	{
		return;
	}

	int iGoldenAgeValue = (GET_PLAYER(getOwnerINLINE()).AI_calculateGoldenAgeValue() / (GET_PLAYER(getOwnerINLINE()).unitsRequiredForGoldenAge()));
	int iDiscoverValue = std::max(1, getDiscoverResearch(NO_TECH));

	if (AI_trade(iGoldenAgeValue * 2))
	{
	    return;
	}

	if (((iGoldenAgeValue * 100) / iDiscoverValue) > 60)
	{
        if (AI_goldenAge())
        {
            return;
        }

        if (AI_trade(iGoldenAgeValue))
        {
            return;
        }

        if (iDiscoverValue > iGoldenAgeValue)
        {
            if (AI_discover())
            {
                return;
            }
            if (GET_PLAYER(getOwnerINLINE()).getUnitClassCount(getUnitClassType()) > 1)
            {
                if (AI_join())
                {
                    return;
                }
            }
        }
	}
	else
	{
		if (AI_discover())
		{
			return;
		}

		if (AI_join())
		{
			return;
		}
	} */
	// K-Mod
	if (AI_greatPersonMove())
		return;

	/* original bts code
	if ((GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2)) ||
		(getGameTurnCreated() < (GC.getGameINLINE().getGameTurn() - 25))) */
	if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2)) // K-Mod (there are good reasons for saving a great person)
	{
		if (AI_discover())
		{
			return;
		}
	}

	if (AI_retreatToCity())
	{
		return;
	}

	// K-Mod
	if (AI_handleStranded())
		return;
	// K-Mod end

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_engineerMove()
{
	PROFILE_FUNC();

	/* original bts code
	if (AI_construct())
	{
		return;
	}

	if (AI_switchHurry())
	{
		return;
	}

	if (AI_hurry())
	{
		return;
	}

	if (AI_discover(true, true))
	{
		return;
	}

	int iGoldenAgeValue = (GET_PLAYER(getOwnerINLINE()).AI_calculateGoldenAgeValue() / (GET_PLAYER(getOwnerINLINE()).unitsRequiredForGoldenAge()));
	int iDiscoverValue = std::max(1, getDiscoverResearch(NO_TECH));

	if (((iGoldenAgeValue * 100) / iDiscoverValue) > 60)
	{
        if (AI_goldenAge())
        {
            return;
        }

        if (iDiscoverValue > iGoldenAgeValue)
        {
            if (AI_discover())
            {
                return;
            }
            if (GET_PLAYER(getOwnerINLINE()).getUnitClassCount(getUnitClassType()) > 1)
            {
                if (AI_join())
                {
                    return;
                }
            }
        }
	}
	else
	{
		if (AI_discover())
		{
			return;
		}

		if (AI_join())
		{
			return;
		}
	} */
	// K-Mod
	if (AI_greatPersonMove())
		return;

	/* original bts code
	if ((GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2)) ||
		(getGameTurnCreated() < (GC.getGameINLINE().getGameTurn() - 25))) */
	if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2)) // K-Mod (there are good reasons for saving a great person)
	{
		if (AI_discover())
		{
			return;
		}
	}

	if (AI_retreatToCity())
	{
		return;
	}

	// K-Mod
	if (AI_handleStranded())
		return;
	// K-Mod end

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}

// K-Mod, the previously missing great spy ai function...
void CvUnitAI::AI_greatSpyMove()
{
	if (AI_greatPersonMove())
		return;

	// Note: spies can't be seen, and can't be attacked. So we don't need to worry about retreating to safety.
	FAssert(alwaysInvisible());

	if (area()->getNumCities() > area()->getCitiesPerPlayer(getOwner()))
	{
		if (AI_reconSpy(5))
		{
			return;
		}
	}

	if (AI_handleStranded())
		return;

	getGroup()->pushMission(MISSION_SKIP);
	return;
}

// K-Mod. For most great people, the AI needs to do similar checks and calculations.
// I've made this general function to do those calculations for all types of great people.
bool CvUnitAI::AI_greatPersonMove()
{
	const CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());

	enum
	{
		GP_SLOW,
		GP_DISCOVER,
		GP_GOLDENAGE,
		GP_TRADE,
		GP_CULTURE
	};
	std::vector<std::pair<int, int> > missions; // (value, mission)
	// 1) Add possible missions to the mission vector.
	// 2) Sort them.
	// 3) Attempt to carry out missions, starting with the highest value.

	CvPlot* pBestPlot = NULL;
	SpecialistTypes eBestSpecialist = NO_SPECIALIST;
	BuildingTypes eBestBuilding = NO_BUILDING;
	int iBestValue = 1;
	int iBestPathTurns = MAX_INT; // just used as a tie-breaker.
	int iMoveFlags = alwaysInvisible() ? 0 : MOVE_NO_ENEMY_TERRITORY;
	bool bCanHurry = m_pUnitInfo->getBaseHurry() > 0 || m_pUnitInfo->getHurryMultiplier() > 0;

	int iLoop;
	for (CvCity* pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
	{
		if (pLoopCity->area() == area())
		{
			int iPathTurns;
			if (generatePath(pLoopCity->plot(), iMoveFlags, true, &iPathTurns))
			{
				// Join
				for (int iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
				{
					SpecialistTypes eSpecialist = (SpecialistTypes)iI;
					if (canJoin(pLoopCity->plot(), eSpecialist))
					{
						// Note, specialistValue is roughly 400x the commerce it provides. So /= 4 to make it 100x.
						int iValue = pLoopCity->AI_permanentSpecialistValue(eSpecialist)/4;
						if (iValue > iBestValue || (iValue == iBestValue && iPathTurns < iBestPathTurns))
						{
							iBestValue = iValue;
							pBestPlot = getPathEndTurnPlot();
							eBestSpecialist = eSpecialist;
							eBestBuilding = NO_BUILDING;
						}
					}
				}
				// Construct
				for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
				{
					BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI);

					if (eBuilding != NO_BUILDING)
					{
						if ((m_pUnitInfo->getForceBuildings(eBuilding) || m_pUnitInfo->getBuildings(eBuilding)) &&
							canConstruct(pLoopCity->plot(), eBuilding))
						{
							// Note, building value is roughly 4x the value of the commerce it provides.
							// so we * 25 to match the scale of specialist value.
							int iValue = pLoopCity->AI_buildingValue(eBuilding) * 25;

							if (iValue > iBestValue || (iValue == iBestValue && iPathTurns < iBestPathTurns))
							{
								iBestValue = iValue;
								pBestPlot = getPathEndTurnPlot();
								eBestBuilding = eBuilding;
								eBestSpecialist = NO_SPECIALIST;
							}
						}
						else if (bCanHurry && isWorldWonderClass((BuildingClassTypes)iI) && pLoopCity->canConstruct(eBuilding))
						{
							// maybe we can hurry a wonder...
							int iCost = pLoopCity->getProductionNeeded(eBuilding);
							int iHurryProduction = getMaxHurryProduction(pLoopCity);
							int iProgress = pLoopCity->getBuildingProduction(eBuilding);

							int iProductionRate = iCost > iHurryProduction + iProgress ? pLoopCity->getProductionDifference(iCost, iProgress, pLoopCity->getProductionModifier(eBuilding), false, 0) : 0;
							// note: currently, it is impossible for a building to be "food production".
							// also note that iProductionRate will return 0 if the city is in disorder. This may mess up our great person's decision - but it's a non-trivial problem to fix.

							if (pLoopCity->getProductionBuilding() == eBuilding)
							{
								iProgress += iProductionRate * iPathTurns;
							}

							FAssert(iHurryProduction > 0);
							int iFraction = 100 * std::min(iHurryProduction, iCost-iProgress) / std::max(1, iCost);

							if (iFraction > 40) // arbitary, and somewhat unneccessary.
							{
								FAssert(iFraction <= 100);
								int iValue = pLoopCity->AI_buildingValue(eBuilding) * 25 * iFraction / 100;

								if (iProgress + iHurryProduction < iCost)
								{
									// decrease the value, because we might still miss out!
									FAssert(iProductionRate > 0 || pLoopCity->isDisorder());
									iValue *= 12;
									iValue /= 12 + std::min(30, pLoopCity->getProductionTurnsLeft(iCost, iProgress, iProductionRate, iProductionRate));
								}

								if (iValue > iBestValue || (iValue == iBestValue && iPathTurns < iBestPathTurns))
								{
									iBestValue = iValue;
									pBestPlot = getPathEndTurnPlot();
									iBestPathTurns = iPathTurns;
									eBestBuilding = eBuilding;
									eBestSpecialist = NO_SPECIALIST;
								}
							}
						}
					}
				}
			} // end safe move possible
		} // end this area
	} // end city loop.

	// Golden age
	int iGoldenAgeValue = 0;
	if (isGoldenAge())
	{
		iGoldenAgeValue = GET_PLAYER(getOwnerINLINE()).AI_calculateGoldenAgeValue() / GET_PLAYER(getOwnerINLINE()).unitsRequiredForGoldenAge();
		iGoldenAgeValue *= (75 + kPlayer.AI_getStrategyRand(0) % 51);
		iGoldenAgeValue /= 100;
		missions.push_back(std::pair<int, int>(iGoldenAgeValue, GP_GOLDENAGE));
	}
	//

	// Discover ("bulb tech")
	int iDiscoverValue = 0;
	TechTypes eDiscoverTech = getDiscoveryTech();
	if (eDiscoverTech != NO_TECH)
	{
		iDiscoverValue = getDiscoverResearch(eDiscoverTech);
		// if this isn't going to immediately help our research, it isn't worth as much.
		if (iDiscoverValue < GET_TEAM(getTeam()).getResearchLeft(eDiscoverTech) && kPlayer.getCurrentResearch() != eDiscoverTech)
		{
			iDiscoverValue *= 2;
			iDiscoverValue /= 3;
		}
		if (kPlayer.AI_isFirstTech(eDiscoverTech)) // founding religions / free techs / free great people
		{
			iDiscoverValue *= 2;
		}
		// amplify the 'undiscovered' bonus based on how likely we are to try to trade the tech.
		iDiscoverValue *= 100 + (200 - GC.getLeaderHeadInfo(kPlayer.getPersonalityType()).getTechTradeKnownPercent())*GET_TEAM(getTeam()).AI_knownTechValModifier(eDiscoverTech)/100;
		iDiscoverValue /= 100;
		if(GET_PLAYER(getOwnerINLINE()).AI_isFocusWar()) // advc.105
		//if (GET_TEAM(getTeam()).getAnyWarPlanCount(true) || kPlayer.AI_isDoStrategy(AI_STRATEGY_ALERT2))
		{
			iDiscoverValue *= (area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE ? 5 : 4);
			iDiscoverValue /= 3;
		}

		iDiscoverValue *= (75 + kPlayer.AI_getStrategyRand(3) % 51);
		iDiscoverValue /= 100;
		missions.push_back(std::pair<int, int>(iDiscoverValue, GP_DISCOVER));
	}

	// SlowValue is meant to be a rough estimation of how much value we'll get from doing the best join / build mission.
	// To give this estimate, I'm going to do a rough personality-based calculation of how many turns to count.
	// Note that "iBestValue" is roughly 100x commerce per turn for our best join or build mission.
	// Also note that the commerce per turn is likely to increase as we improve our city infrastructure and so on.
	int iSlowValue = iBestValue;
	if (iSlowValue > 0)
	{
		// multiply by the full number of turns remaining
		iSlowValue *= GC.getGameINLINE().getEstimateEndTurn() - GC.getGameINLINE().getGameTurn();

		// construct a modifier based on what victory we might like to aim for with our personality & situation
		const CvLeaderHeadInfo& kLeader = GC.getLeaderHeadInfo(kPlayer.getPersonalityType());
		int iModifier =
			2 * std::max(kLeader.getSpaceVictoryWeight(), kPlayer.AI_isDoVictoryStrategy(AI_VICTORY_SPACE1) ? 35 : 0) +
			1 * std::max(kLeader.getCultureVictoryWeight(), kPlayer.AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1) ? 35 : 0) +
			//0 * kLeader.getDiplomacyVictoryWeight() +
			-1 * std::max(kLeader.getDominationVictoryWeight(), kPlayer.AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION1) ? 35 : 0) +
			-2 * std::max(kLeader.getConquestVictoryWeight(), kPlayer.AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST1) ? 35 : 0);
		// If we're small, then slow & steady progress might be our best hope to keep up. So increase the modifier for small civs. (think avg. cities / our cities)
		iModifier += range(40 * GC.getGameINLINE().getNumCivCities() / std::max(1, GC.getGameINLINE().countCivPlayersAlive()*kPlayer.getNumCities()) - 50, 0, 50);

		// convert the modifier into some percentage of the remaining turns
		iModifier = range(30 + iModifier/2, 20, 80);
		// apply it
		iSlowValue *= iModifier;
		iSlowValue /= 10000; // (also removing excess factor of 100)

		// half the value if anyone we know is up to stage 4. (including us)
		for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
		{
			const CvPlayerAI& kLoopPlayer = GET_PLAYER(i);
			if (kLoopPlayer.isAlive() && kLoopPlayer.AI_isDoVictoryStrategyLevel4() && GET_TEAM(getTeam()).isHasMet(kLoopPlayer.getTeam()))
			{
				iSlowValue /= 2;
				break; // just once.
			}
		}
		//if (gUnitLogLevel > 2) logBBAI("    %S GP slow modifier: %d, value: %d", GET_PLAYER(getOwnerINLINE()).getCivilizationDescription(0), range(30 + iModifier/2, 20, 80), iSlowValue);
		iSlowValue *= (75 + kPlayer.AI_getStrategyRand(6) % 51);
		iSlowValue /= 100;
		missions.push_back(std::pair<int, int>(iSlowValue, GP_SLOW));
	}

	// Trade mission
	CvPlot* pBestTradePlot;
	int iTradeValue = AI_tradeMissionValue(pBestTradePlot, iDiscoverValue / 2);
	// make it roughly comparable to research points
	if (pBestTradePlot != NULL)
	{
		iTradeValue *= kPlayer.AI_commerceWeight(COMMERCE_GOLD);
		iTradeValue /= 100;
		iTradeValue *= kPlayer.AI_averageCommerceMultiplier(COMMERCE_RESEARCH);
		iTradeValue /= kPlayer.AI_averageCommerceMultiplier(COMMERCE_GOLD);
		// gold can be targeted where it is needed, but it's benefits typically aren't instant. (cf AI_knownTechValModifier)
		iTradeValue *= 130;
		iTradeValue /= 100;
		if (getGroup()->AI_getMissionAIType() == MISSIONAI_TRADE && plot()->getOwner() != getOwner())
		{
			// if we are part way through a trade mission, prefer not to turn back.
			iTradeValue *= 120;
			iTradeValue /= 100;
		}
		iTradeValue *= (75 + kPlayer.AI_getStrategyRand(9) % 51);
		iTradeValue /= 100;
		missions.push_back(std::pair<int, int>(iTradeValue, GP_TRADE));
	}

	// Great works (culture bomb)
	CvPlot* pBestCulturePlot;
	int iCultureValue = AI_greatWorkValue(pBestCulturePlot, iDiscoverValue / 2);
	if (pBestCulturePlot != 0)
	{
		missions.push_back(std::pair<int, int>(iCultureValue, GP_CULTURE));
	}

	// Sort the list!
	std::sort(missions.begin(), missions.end(), std::greater<std::pair<int, int> >());
	std::vector<std::pair<int, int> >::iterator it;

	int iChoice = 1;
	int iScoreThreshold = 0;
	for (it = missions.begin(); it != missions.end(); ++it)
	{
		if (it->first < iScoreThreshold)
			break;

		switch (it->second)
		{
		case GP_DISCOVER:
			if (canDiscover(plot()))
			{
                getGroup()->pushMission(MISSION_DISCOVER);
				if (gUnitLogLevel > 2) logBBAI("    %S chooses 'discover' (%S) with their %S (value: %d, choice #%d)", GET_PLAYER(getOwnerINLINE()).getCivilizationDescription(0), GC.getTechInfo(eDiscoverTech).getDescription(), getName(0).GetCString(), iDiscoverValue, iChoice);
				return true;
			}
			break;

		case GP_TRADE:
			{
				MissionAITypes eOldMission = getGroup()->AI_getMissionAIType(); // just used for the log message below
				if (AI_doTradeMission(pBestTradePlot))
				{
					if (gUnitLogLevel > 2) logBBAI("    %S %s 'trade mission' with their %S (value: %d, choice #%d)", GET_PLAYER(getOwnerINLINE()).getCivilizationDescription(0), eOldMission == MISSIONAI_TRADE?"continues" :"chooses", getName(0).GetCString(), iTradeValue, iChoice);
					return true;
				}
				break;
			}

		case GP_CULTURE:
			{
				MissionAITypes eOldMission = getGroup()->AI_getMissionAIType(); // just used for the log message below
				if (AI_doGreatWork(pBestCulturePlot))
				{
					if (gUnitLogLevel > 2) logBBAI("    %S %s 'great work' with their %S (value: %d, choice #%d)", GET_PLAYER(getOwnerINLINE()).getCivilizationDescription(0), eOldMission == MISSIONAI_TRADE?"continues" :"chooses", getName(0).GetCString(), iCultureValue, iChoice);
					return true;
				}
				break;
			}

		case GP_GOLDENAGE:
			if (AI_goldenAge())
			{
				if (gUnitLogLevel > 2) logBBAI("    %S chooses 'golden age' with their %S (value: %d, choice #%d)", GET_PLAYER(getOwnerINLINE()).getCivilizationDescription(0), getName(0).GetCString(), iGoldenAgeValue, iChoice);
				return true;
			}
			else if (kPlayer.AI_totalUnitAIs(AI_getUnitAIType()) < 2)
			{
				// Do we want to wait for another great person? How long will it take?
				int iGpThreshold = kPlayer.greatPeopleThreshold();
				int iMinTurns = INT_MAX;
				//int iPercentOther; // chance of it being a different GP.
				// unfortunately, it's non-trivial to calculate the GP type probabilies. So I'm leaving it out.
				for (CvCity* pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
				{
					int iGpRate = pLoopCity->getGreatPeopleRate();
					if (iGpRate > 0)
					{
						int iGpProgress = pLoopCity->getGreatPeopleProgress();
						int iTurns = (iGpThreshold - iGpProgress + iGpRate - 1) / iGpRate;
						if (iTurns < iMinTurns)
							iMinTurns = iTurns;
					}
				}

				if (iMinTurns != INT_MAX)
				{
					int iRelativeWaitTime = iMinTurns + (GC.getGameINLINE().getGameTurn() - getGameTurnCreated());
					iRelativeWaitTime *= 100;
					iRelativeWaitTime /= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getVictoryDelayPercent();
					// lets say 1% per turn.
					iScoreThreshold = std::max(iScoreThreshold, it->first * (100 - iRelativeWaitTime) / 100);
				}
			}
			break;

		case GP_SLOW:
			// no dedicated function for this.
			if (pBestPlot != NULL)
			{
				if (eBestSpecialist != NO_SPECIALIST)
				{
					if (gUnitLogLevel > 2) logBBAI("    %S %s 'join' with their %S (value: %d, choice #%d)", GET_PLAYER(getOwnerINLINE()).getCivilizationDescription(0), getGroup()->AI_getMissionAIType() == MISSIONAI_JOIN_CITY?"continues" :"chooses", getName(0).GetCString(), iSlowValue, iChoice);
					if (atPlot(pBestPlot))
					{
						getGroup()->pushMission(MISSION_JOIN, eBestSpecialist);
						return true;
					}
					else
					{
						getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), iMoveFlags, false, false, MISSIONAI_JOIN_CITY);
						return true;
					}
				}

				if (eBestBuilding != NO_BUILDING)
				{
					MissionAITypes eMissionAI = canConstruct(pBestPlot, eBestBuilding) ? MISSIONAI_CONSTRUCT : MISSIONAI_HURRY;

					if (gUnitLogLevel > 2) logBBAI("    %S %s 'build' (%S) with their %S (value: %d, choice #%d)", GET_PLAYER(getOwnerINLINE()).getCivilizationDescription(0), getGroup()->AI_getMissionAIType() == eMissionAI?"continues" :"chooses", GC.getBuildingInfo(eBestBuilding).getDescription(), getName(0).GetCString(), iSlowValue, iChoice);
					if (atPlot(pBestPlot))
					{
						if (eMissionAI == MISSIONAI_CONSTRUCT)
						{
							getGroup()->pushMission(MISSION_CONSTRUCT, eBestBuilding);
						}
						else
						{
							// switch and hurry.
							CvCity* pCity = pBestPlot->getPlotCity();
							FAssert(pCity);

							if (pCity->getProductionBuilding() != eBestBuilding)
								pCity->pushOrder(ORDER_CONSTRUCT, eBestBuilding);

							if (pCity->getProductionBuilding() == eBestBuilding && canHurry(plot()))
							{
								getGroup()->pushMission(MISSION_HURRY);
							}
							else
							{
								FAssertMsg(false, "great person cannot hurry what it intended to hurry.");
								return false;
							}
						}
						return true;
					}
					else
					{
						getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), iMoveFlags, false, false, eMissionAI);
						return true;
					}
				}
			}
			break;
		default:
			FAssertMsg(false, "Unhandled great person mission");
			break;
		}
		iChoice++;
	}
	FAssert(iScoreThreshold > 0);
	if (gUnitLogLevel > 2) logBBAI("    %S chooses 'wait' with their %S (value: %d, dead time: %d)", GET_PLAYER(getOwnerINLINE()).getCivilizationDescription(0), getName(0).GetCString(), iScoreThreshold, GC.getGameINLINE().getGameTurn() - getGameTurnCreated());
	return false;
}
// K-Mod end

// Edited heavily for K-Mod
void CvUnitAI::AI_spyMove()
{
	PROFILE_FUNC();

	const CvTeamAI& kTeam = GET_TEAM(getTeam());
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());

	// First, let us finish any missions that we were part way through doing
	{
		CvPlot* pMissionPlot = getGroup()->AI_getMissionAIPlot();
		if (pMissionPlot != NULL)
		{
			switch (getGroup()->AI_getMissionAIType())
			{
			case MISSIONAI_GUARD_SPY:
				if (pMissionPlot->getOwner() == getOwner())
				{
					if (atPlot(pMissionPlot))
					{
						// stay here for a few turns.
						if (hasMoved())
						{
							getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_SPY, pMissionPlot);
							return;
						}
						if (GC.getGame().getSorenRandNum(6, "AI Spy continue guarding") > 0)
						{
							getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_SPY, pMissionPlot);
							return;
						}
					}
					else
					{
						// continue to the destination
						if (generatePath(pMissionPlot, 0, true))
						{
							CvPlot* pEndTurnPlot = getPathEndTurnPlot();
							getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX_INLINE(), pEndTurnPlot->getY_INLINE(), 0, false, false, MISSIONAI_GUARD_SPY, pMissionPlot);
							return;
						}
					}
				}
				break;
			case MISSIONAI_ATTACK_SPY:
				if (pMissionPlot->getTeam() != getTeam())
				{
					if (!atPlot(pMissionPlot))
					{
						// continue to the destination
						if (generatePath(pMissionPlot, 0, true))
						{
							CvPlot* pEndTurnPlot = getPathEndTurnPlot();
							getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX_INLINE(), pEndTurnPlot->getY_INLINE(), 0, false, false, MISSIONAI_ATTACK_SPY, pMissionPlot);
							return;
						}
					}
					else if (hasMoved())
					{
						getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY);
						return;
					}
				}
				break;
			case MISSIONAI_EXPLORE:
				/*if (atPlot(pMissionPlot))
				{
					getGroup()->AI_setMissionAI(NO_MISSIONAI, 0, 0);
				}*/
				break;
			case MISSIONAI_LOAD_SPECIAL:
				if (AI_load(UNITAI_SPY_SEA, MISSIONAI_LOAD_SPECIAL))
					return;

			default:
				break;
			}
		}
	}

	if (plot()->isOwned() && plot()->getTeam() != getTeam()
			&& !plot()->isBarbarian()) // advc.003n
	{
		int iSpontaneousChance = 0;
		switch (GET_PLAYER(getOwnerINLINE()).AI_getAttitude(plot()->getOwnerINLINE()))
		{
		case ATTITUDE_FURIOUS:
			iSpontaneousChance = 100;
			break;

		case ATTITUDE_ANNOYED:
			iSpontaneousChance = 50;
			break;

		case ATTITUDE_CAUTIOUS: // advc.019: was ?30:10
			iSpontaneousChance = (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 20 : 10);
			break;

		case ATTITUDE_PLEASED: // advc.019: was ?20:0
			iSpontaneousChance = (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 15 : 0);
			break;

		case ATTITUDE_FRIENDLY:
			iSpontaneousChance = 0;
			break;

		default:
			FAssert(false);
			break;
		}
		
		WarPlanTypes eWarPlan = kTeam.AI_getWarPlan(plot()->getTeam());
		if (eWarPlan != NO_WARPLAN)
		{
			if (eWarPlan == WARPLAN_LIMITED)
			{
				iSpontaneousChance += 50;
			}
			else
			{
				iSpontaneousChance += 20;
			}
		}

		if (plot()->isCity())
		{
			bool bTargetCity = false;

			// would we have more power if enemy defenses were down?
			/* original BBAI code
			int iOurPower = kOwner.AI_getOurPlotStrength(plot(),1,false,true);
			int iEnemyPower = kOwner.AI_getEnemyPlotStrength(plot(),0,false,false); // original BBAI code */
			// K-Mod note: telling AI_getEnemyPlotStrength to not count defensive bonuses is not what we want.
			// That would make it not count hills and city defence promotions; and instead count collateral damage power.
			// Instead, I'm going to count the defensive bonuses, and then try to approximately remove the city part.
			int iOurPower = kOwner.AI_localAttackStrength(plot(), getTeam(), DOMAIN_LAND, 1, true, true);
			int iEnemyPower = kOwner.AI_localDefenceStrength(plot(), NO_TEAM, DOMAIN_LAND, 0);
			{
				int iBase = 235 + (plot()->isHills() ? GC.getHILLS_EXTRA_DEFENSE() : 0);
				iEnemyPower *= iBase - plot()->getPlotCity()->getDefenseModifier(false);
				iEnemyPower /= iBase;
			}
			// cf. approximation used in AI_attackCityMove. (here we are slightly more pessimistic)

			//if( 5*iOurPower > 6*iEnemyPower && eWarPlan != NO_WARPLAN )
			if (95*iOurPower > GC.getBBAI_ATTACK_CITY_STACK_RATIO()*iEnemyPower && eWarPlan != NO_WARPLAN
					&& iOurPower < 8 * iEnemyPower) // advc.120b
			{
				bTargetCity = true;

				if (AI_revoltCitySpy())
				{
					return;
				}

				if (GC.getGame().getSorenRandNum(6, "AI Spy Skip Turn") > 0)
				{
					getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY, plot());
					return;
				}

				if (plot()->plotCount(PUF_isSpy, -1, -1, getOwner()) > 2)
				{
					if (AI_cityOffenseSpy(5, plot()->getPlotCity()))
					{
						return;
					}
				}
			}

			// I think this spontaneous thing is bad. I'm leaving it in, but with greatly diminished probability.
			// scale for game speed
			iSpontaneousChance *= 100;
			iSpontaneousChance /= GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getVictoryDelayPercent();
			if (GC.getGameINLINE().getSorenRandNum(1500, "AI Spy Espionage") < iSpontaneousChance)
			{
				if (AI_espionageSpy())
				{
					return;
				}
			}

			if( kOwner.AI_plotTargetMissionAIs(plot(), MISSIONAI_ASSAULT, getGroup()) > 0 )
			{
				bTargetCity = true;

				getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY, plot());
				return;
			}
			
			if( !bTargetCity )
			{
				// normal city handling

				if (getFortifyTurns() >= GC.getDefineINT("MAX_FORTIFY_TURNS"))
				{
					if (AI_espionageSpy())
					{
						return;
					}
				}
				// advc.034: Can take our time during disengagement
				else if(isIntruding())
				{
					// If we think we'll get caught soon, then do the mission early.
					int iInterceptChance = getSpyInterceptPercent(plot()->getTeam(), false);
					iInterceptChance *= 100 + (GET_TEAM(getTeam()).isOpenBorders(plot()->getTeam())
						? GC.getDefineINT("ESPIONAGE_SPY_NO_INTRUDE_INTERCEPT_MOD")
						: GC.getDefineINT("ESPIONAGE_SPY_INTERCEPT_MOD"));
					iInterceptChance /= 100;
					if (GC.getGame().getSorenRandNum(100, "AI Spy early attack") < iInterceptChance + getFortifyTurns())
					{
						if (AI_espionageSpy())
							return;
					}
				}

				if (GC.getGame().getSorenRandNum(100, "AI Spy Skip Turn") > 5)
				{
					// don't wait forever
					getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY, plot());
					return;
				}
			}
		}
	}

	// Do with have enough points on anyone for an attack mission to be useful?
	int iAttackChance = 0;
	int iTransportChance = 0;
	{
		int iScale = 100 * (kOwner.getCurrentEra() + 1);
		int iAttackSpies = kOwner.AI_areaMissionAIs(area(), MISSIONAI_ATTACK_SPY);
		int iLocalPoints = 0;
		int iTotalPoints = 0;

		if (kOwner.AI_isDoStrategy(AI_STRATEGY_ESPIONAGE_ECONOMY))
		{	// advc.120b: was 50*
			iScale += 80 * kOwner.getCurrentEra() * (kOwner.getCurrentEra()+1);
		}

		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			int iPoints = kTeam.getEspionagePointsAgainstTeam((TeamTypes)iI);
			iTotalPoints += iPoints;

			if (iI != getTeam() && GET_TEAM((TeamTypes)iI).isAlive() && kTeam.isHasMet((TeamTypes)iI) &&
				GET_TEAM((TeamTypes)iI).countNumCitiesByArea(area()) > 0)
			{
				int x = 100 * iPoints + iScale;
				x /= iPoints + (1 + iAttackSpies) * iScale;
				iAttackChance = std::max(iAttackChance, x);

				iLocalPoints += iPoints;
			}
		}
		iAttackChance /=
				!GET_PLAYER(getOwnerINLINE()).AI_isFocusWar(area()) // advc.105
				//kTeam.getAnyWarPlanCount(true) == 0
				? 3 : 1;
		iAttackChance /= plot()->area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE ? 2 : 1;
		iAttackChance /= (kOwner.AI_isDoVictoryStrategy(AI_VICTORY_SPACE4) || kOwner.AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3)) ? 2 : 1;
		iAttackChance *= GC.getLeaderHeadInfo(kOwner.getPersonalityType()).getEspionageWeight();
		iAttackChance /= 100;
		// scale for game speed
		iAttackChance *= 100;
		iAttackChance /= GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getVictoryDelayPercent();

		iTransportChance = (100 * iTotalPoints - 130 * iLocalPoints) / std::max(1, iTotalPoints);
	}
	
	if (plot()->getTeam() == getTeam())
	{
		if (GC.getGame().getSorenRandNum(100, "AI Spy guard / transport") >= iAttackChance)
		{
			if (GC.getGame().getSorenRandNum(100, "AI Spy transport") < iTransportChance)
			{
				if (AI_load(UNITAI_SPY_SEA, MISSIONAI_LOAD_SPECIAL, NO_UNITAI, -1, -1, -1, 0, 0, 8))
					return;
			}

			if (AI_guardSpy(0))
			{
				return;
			}
		}

		if (!kOwner.AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE) && 
				//GET_TEAM(getTeam()).getAtWarCount(true) > 0 &&
				// advc.105: Replacing the above
				GET_PLAYER(getOwnerINLINE()).AI_isFocusWar(area()) &&
				GC.getGame().getSorenRandNum(100, "AI Spy pillage improvement") <
				(kOwner.AI_getStrategyRand(5) % 36))
		{
			if (AI_bonusOffenseSpy(6))
			{
				return;
			}
		}
		else
		{
			if (AI_cityOffenseSpy(10))
			{
				return;
			}
		}
	}
	
	if (getGroup()->AI_getMissionAIType() == MISSIONAI_ATTACK_SPY && plot()->getNonObsoleteBonusType(getTeam(), true) != NO_BONUS
		&& plot()->isOwned() && kOwner.AI_isMaliciousEspionageTarget(plot()->getOwner()))
	{
		// assume this is the target of our destroy improvement mission.
		if (getFortifyTurns() >= GC.getDefineINT("MAX_FORTIFY_TURNS"))
		{
			if (AI_espionageSpy())
			{
				return;
			}
		}

		if (GC.getGame().getSorenRandNum(10, "AI Spy skip turn at improvement") > 0)
		{
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY, plot());
			return;
		}
	}

	if( area()->getNumCities() > area()->getCitiesPerPlayer(getOwnerINLINE()) )
	{
		if (kOwner.AI_areaMissionAIs(area(), MISSIONAI_RECON_SPY) <= kOwner.AI_areaMissionAIs(area(), MISSIONAI_GUARD_SPY)+1 &&
			(getGroup()->AI_getMissionAIType() == MISSIONAI_RECON_SPY
			|| GC.getGame().getSorenRandNum(3, "AI Spy Choose Movement") > 0))
		{
			if (AI_reconSpy(3))
			{
				return;
			}
		}
		else
		{
			if (GC.getGame().getSorenRandNum(100, "AI Spy defense (2)") >= iAttackChance)
			{
				if (AI_guardSpy(0))
				{
					return;
				}
			}

			if (AI_cityOffenseSpy(20))
			{
				return;
			}
		}
	}
	
	//if (AI_load(UNITAI_SPY_SEA, MISSIONAI_LOAD_SPECIAL, NO_UNITAI, -1, -1, -1, 0, MOVE_NO_ENEMY_TERRITORY))
	if (AI_load(UNITAI_SPY_SEA, MISSIONAI_LOAD_SPECIAL))
		return;

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}

void CvUnitAI::AI_ICBMMove()
{
	PROFILE_FUNC();

//	CvCity* pCity = plot()->getPlotCity();

//	if (pCity != NULL)
//	{
//		if (pCity->AI_isDanger())
//		{
//			if (!(pCity->AI_isDefended()))
//			{
//				if (AI_airCarrier())
//				{
//					return;
//				}
//			}
//		}
//	}
	
	if (airRange() > 0)
	{
		if (AI_nukeRange(airRange()))
		{
			return;
		}
	}
	else if (AI_nuke())
	{
		return;
	}

	if (isCargo())
	{
		getGroup()->pushMission(MISSION_SKIP);
		return;
	}
	
	if (airRange() > 0)
	{
		if (AI_missileLoad(UNITAI_MISSILE_CARRIER_SEA, 2, true))
		{
			return;
		}
		
		if (AI_missileLoad(UNITAI_MISSILE_CARRIER_SEA, 1, false))
		{
			return;
		}
		
		if (AI_getBirthmark() % 3 == 0)
		{
			if (AI_missileLoad(UNITAI_ATTACK_SEA, 0, false))
			{
				return;
			}
		}
		
		if (AI_airOffensiveCity())
		{
			return;
		}
	}

	getGroup()->pushMission(MISSION_SKIP);
}


void CvUnitAI::AI_workerSeaMove()
{
	PROFILE_FUNC();

	if (!getGroup()->canDefend())
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
		//if (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot()) > 0)
		if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot()))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		{
			if (AI_retreatToCity())
			{
				return;
			}
		}
	}

	/* if (AI_improveBonus(0,20))
	{
		return;
	}

	if (AI_improveBonus(0,10))
	{
		return;
	} */ // disabled by K-Mod. .. obviously redundant.

	if (AI_improveBonus())
	{
		return;
	}
	
	if (isHuman())
	{
		FAssert(isAutomated());
		if (plot()->getBonusType() != NO_BONUS)
		{
			if ((plot()->getOwnerINLINE() == getOwnerINLINE()) || (!plot()->isOwned()))
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
		}
		
		for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
		{
			CvPlot* pLoopPlot = plotDirection(getX_INLINE(), getY_INLINE(), (DirectionTypes)iI);
			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->getBonusType() != NO_BONUS)
				{
					if (pLoopPlot->isValidDomainForLocation(*this))
					{
						getGroup()->pushMission(MISSION_SKIP);
						return;						
					}					
				}
			}
		}
	}
	
	if (!isHuman() && AI_getUnitAIType() == UNITAI_WORKER_SEA)
	{
		CvCity* pCity = plot()->getPlotCity();

		if (pCity != NULL)
		{
			if (pCity->getOwnerINLINE() == getOwnerINLINE())
			{
				if (pCity->AI_neededSeaWorkers() == 0)
				{
					if (GC.getGameINLINE().getElapsedGameTurns() > 10)
					{
						if (GET_PLAYER(getOwnerINLINE()).calculateUnitCost() > 0)
						{
							scrap();
							return;
						}
					}
				}
				else
				{
					//Probably icelocked since it can't perform actions.
					scrap();
					return;
				}	
			}
		}
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_barbAttackSeaMove()
{
	PROFILE_FUNC();

	/* original BTS code
	if (GC.getGameINLINE().getSorenRandNum(2, "AI Barb") == 0)
	{
		if (AI_pillageRange(1))
		{
			return;
		}
	}

	if (AI_anyAttack(2, 25))
	{
		return;
	}

	if (AI_pillageRange(4))
	{
		return;
	}

	if (AI_heal())
	{
		return;
	}
	*/

	// <advc.306> Assault mode until cargo delivered
	if(hasCargo()) {
		// Opportunistic attack on Worker or empty city
		if(AI_barbAmphibiousCapture())
			return;
		if(AI_assaultSeaTransport())
			return;
		// Dump cargo anywhere if no assault target:
		if(plot()->isAdjacentToLand()) {
			CvPlot* dest = plot()->getNearestLandPlot();
			if(dest != NULL && AI_transportGoTo(plot(), dest, 0, MISSIONAI_ASSAULT))
				return;
		}
	} // </advc.306>

	// K-Mod
	if (AI_anyAttack(1, 51)) // safe attack
		return;

	if (AI_pillageRange(1)) // near pillage
		return;

	if (AI_heal())
		return;

	if (AI_anyAttack(1, 30)) // reckless attack
		return;

	if (GC.getGameINLINE().getSorenRandNum(10, "AI barb attack sea pillage") < 4 && AI_pillageRange(3)) // long pillage
		return;

	if (GC.getGameINLINE().getSorenRandNum(16, "AI barb attack sea chase") < 15 && AI_anyAttack(2, 45)) // chase
		return;

	// Barb ships will often hang out for a little while blockading before moving on (BBAI)
	if( (GC.getGame().getGameTurn() + AI_getBirthmark())%12 > 5 )
	{
		if( AI_pirateBlockade())
		{
			return;
		}
	}

	// (trap checking from BBAI)
	if( GC.getGameINLINE().getSorenRandNum(3, "AI Check trapped") == 0 )
	{
		// If trapped in small hole in ice or around tiny island, disband to allow other units to be generated
		bool bScrap = true;
		int iMaxRange = baseMoves() + 2;
		for (int iDX = -(iMaxRange); iDX <= iMaxRange; iDX++)
		{
			for (int iDY = -(iMaxRange); iDY <= iMaxRange; iDY++)
			{
				if( bScrap )
				{
					CvPlot* pLoopPlot = plotXY(plot()->getX_INLINE(), plot()->getY_INLINE(), iDX, iDY);
					
					if (pLoopPlot != NULL && AI_plotValid(pLoopPlot))
					{
						int iPathTurns;
						if (generatePath(pLoopPlot, 0, true, &iPathTurns))
						{
							if( iPathTurns > 1 )
							{
								bScrap = false;
							}
						}
					}
				}
			}
		}

		if( bScrap )
		{
			scrap();
			return;
		}
	}
	// K-Mod / BBAI end

	/*  <advc.306> Have ships retreat more often, so that they can receive cargo.
		Should also resolve an issue where barb ships are indefinitely stuck
		patrolling an unowned stretch surrounded by borders. (Patrolling barbs
		never enter borders.) */
	if((::bernoulliSuccess(0.2, "advc.306") ||
			getGroup()->getMissionType(0) == MISSION_MOVE_TO) && AI_safety())
		return; // </advc.306>

	if (AI_patrol())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/23/10                                jdog5000      */
/*                                                                                              */
/* Pirate AI                                                                                    */
/************************************************************************************************/
void CvUnitAI::AI_pirateSeaMove()
{
	PROFILE_FUNC();

	// heal in defended, unthreatened forts and cities
	if (plot()->isCity(true) && (GET_PLAYER(getOwnerINLINE()).AI_localDefenceStrength(plot(), getTeam()) > 0) && !(GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2, false)) )
	{
		if (AI_heal())
		{
			return;
		}
	}

	if (plot()->isOwned() && (plot()->getTeam() == getTeam()))
	{
		if (AI_anyAttack(2, 40))
		{
			return;			
		}
		
		//if (AI_protect(30))
		if (AI_defendTerritory(45, 0, 3, true)) // K-Mod
		{
			return;
		}
		

		if (((AI_getBirthmark() / 8) % 2) == 0)
		{
			// Previously code actually blocked grouping
			if (AI_group(UNITAI_PIRATE_SEA, -1, 1, -1, true, false, false, 8))
			{
				return;
			}
		}
	}
	else
	{
		if (AI_anyAttack(2, 51))
		{
			return;
		}
	}

	
	if (GC.getGame().getSorenRandNum(10, "AI Pirate Explore") == 0)
	{
		CvArea* pWaterArea = plot()->waterArea();

		if (pWaterArea != NULL)
		{
			if (pWaterArea->getNumUnrevealedTiles(getTeam()) > 0)
			{
				if (GET_PLAYER(getOwnerINLINE()).AI_areaMissionAIs(pWaterArea, MISSIONAI_EXPLORE, getGroup()) < (GET_PLAYER(getOwnerINLINE()).AI_neededExplorers(pWaterArea)))
				{
					if (AI_exploreRange(2))
					{
						return;
					}
				}
			}
		}
	}

	if (GC.getGame().getSorenRandNum(11, "AI Pirate Pillage") == 0)
	{
		if (AI_pillageRange(1))
		{
			return;
		}
	}
	
	//Includes heal and retreat to sea routines.
	if (AI_pirateBlockade())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


void CvUnitAI::AI_attackSeaMove()
{
	PROFILE_FUNC();
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						06/14/09	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true))
	{
		if(AI_isThreatenedFromLand()) // advc.139: Code moved into subroutine
		{
			if (AI_anyAttack(2, 50))
			{
				return;
			}

			if (AI_shadow(UNITAI_ASSAULT_SEA, 4, 34, false, true, baseMoves()))
			{
				return;
			}

			//if (AI_protect(35, 0, 3))
			if (AI_defendTerritory(45, 0, 3, true)) // K-Mod
			{
				return;
			}
			// <advc.017b>
			CvArea* a = plot()->waterArea();
			if(a != NULL) {
				if(m_pUnitInfo->getDefaultUnitAIType() == UNITAI_EXPLORE_SEA &&
						kOwner.AI_totalWaterAreaUnitAIs(a, UNITAI_EXPLORE_SEA) <
						kOwner.AI_neededExplorers(a))
					AI_setUnitAIType(UNITAI_EXPLORE_SEA);
			} // </advc.017b>
			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	if (AI_heal(30, 1))
	{
		return;
	}
	
	if (AI_anyAttack(1, 35))
	{
		return;
	}
	
	if (AI_anyAttack(2, 40))
	{
		return;
	}
	
	if (AI_seaBombardRange(6))
	{
		return;
	}
	
	if (AI_heal(50, 3))
	{
		return;
	}

	if (AI_heal())
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/10/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
	// BBAI TODO: Turn this into a function, have docked escort ships do it to
	CvCity* pCity = plot()->getPlotCity();

	if( pCity != NULL )
	{
		if( pCity->isBlockaded() )
		{
			// City under blockade
			// Attacker has low odds since anyAttack checks above passed, try to break if sufficient numbers

			int iAttackers = plot()->plotCount(PUF_isUnitAIType, UNITAI_ATTACK_SEA, -1, NO_PLAYER, getTeam());
			// advc.114a: Why count only group heads? Need to count all attackers!
					//PUF_isGroupHead, -1, -1);
			int iBlockaders = kOwner.AI_getWaterDanger(plot(), 4);
			//if( iAttackers > (iBlockaders + 2) )
			// advc.114a: Replacing the above
			if(2 * iAttackers >= 3 * iBlockaders && iBlockaders > 0)
			{
				if( iAttackers > GC.getGameINLINE().getSorenRandNum(2*iBlockaders + 1, "AI - Break blockade") )
				{
					// BBAI TODO: Make odds scale by # of blockaders vs number of attackers
					/*  <advc.114a>: Exactly. Attack regardless of odds when
						outnumbering them 5:1; 1% at 3:1; 22% at 2:1; 33% at 1.5:1.
						Those are chancy odds, but don't want CvCityAI to build any
						more (outdated) ships than necessary; they won't have much
						of a future use. Also, blockading units can't usually heal;
						not imperative to destroy them in one turn. In fact, damaging
						them may be enough to drive them off. */
					double attackerRatio = iAttackers / (double)iBlockaders;
					int oddsThresh = 0;
					if(attackerRatio < 5 && attackerRatio >= 3)
						oddsThresh = 1;
					if(attackerRatio < 3)
						oddsThresh = ::round(std::pow(5 - attackerRatio, 2.8));
					if(AI_anyAttack(1, oddsThresh))
					//if (AI_anyAttack(1, 15)) // </advc.114a>
					{
						return;
					}
				}
			}
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	
	if (AI_group(UNITAI_CARRIER_SEA, /*iMaxGroup*/ 4, 1, -1, true, false, false, /*iMaxPath*/ 5))
	{
		return;
	}
	
	if (AI_group(UNITAI_ATTACK_SEA, /*iMaxGroup*/ 1, -1, -1, true, false, false, /*iMaxPath*/ 3))
	{
		return;
	}
	
	if (!plot()->isOwned() || !isEnemy(plot()->getTeam()))
	{
		/* original bts code
		if (AI_shadow(UNITAI_ASSAULT_SEA, 4, 34))
		{
			return;
		}
		
		if (AI_shadow(UNITAI_CARRIER_SEA, 4, 51))
		{
			return;
		}

		if (AI_group(UNITAI_ASSAULT_SEA, -1, 4, -1, false, false, false))
		{
			return;
		}
	}
	
	if (AI_group(UNITAI_CARRIER_SEA, -1, 1, -1, false, false, false))
	{
		return;
	} */
		// K-Mod / BBAI. I've changed the order of group / shadow.
		// What I'd really like is to join the assault group if the group needs escorts, but shadow if it doesn't.

		// Get at least one shadow per assault group.
		if (AI_shadow(UNITAI_ASSAULT_SEA, 1, -1, true, false, 4))
		{
			return;
		}

		// Allow several attack_sea with large flotillas 
		if (AI_group(UNITAI_ASSAULT_SEA, -1, 4, 4, false, false, false, 4, false, true, false))
		{
			return;
		}

		// allow just a couple with small asault teams
		if (AI_group(UNITAI_ASSAULT_SEA, -1, 2, -1, false, false, false, 5, false, true, false))
		{
			return;
		}

		// Otherwise, try to shadow.
		if (AI_shadow(UNITAI_ASSAULT_SEA, 4, 34, true, false, 4))
		{
			return;
		}

		if (AI_shadow(UNITAI_CARRIER_SEA, 4, 51, true, false, 5))
		{
			return;
		}
	}
	
	if (AI_group(UNITAI_CARRIER_SEA, -1, 1, -1, false, false, false, 10))
	{
		return;
	}
	// K-Mod / BBAI end
	
	if (plot()->isOwned() && isEnemy(plot()->getTeam())
			// advc.033: Don't blockade Barbs
			&& plot()->getTeam() != BARBARIAN_TEAM)
	{
		if (AI_blockade())
		{
			return;
		}
	}

	if (AI_pillageRange(4))
	{
		return;
	}
	
	//if (AI_protect(35))
	if (AI_defendTerritory(40, 0, 8)) // K-Mod
	{
		return;
	}

	if (AI_travelToUpgradeCity())
	{
		return;
	}

	// K-Mod
	if (AI_guardBonus(10))
		return;

	if (AI_getBirthmark()%2 == 0 && AI_guardCoast()) // I want some attackSea units to just patrol the area.
		return;
	// K-Mod end

	if (AI_patrol())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_reserveSeaMove()
{
	PROFILE_FUNC();
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						06/14/09	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true))
	{
		if(AI_isThreatenedFromLand()) // advc.139: Code moved into subroutine
		{
			if (AI_anyAttack(2, 60))
			{
				return;
			}

			//if (AI_protect(40))
			if (AI_defendTerritory(45, 0, 3, true)) // K-Mod
			{
				return;
			}

			if (AI_shadow(UNITAI_SETTLER_SEA, 2, -1, false, true, baseMoves()))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	/*  advc.017b: Defend bonus if it's threatened, otherwise, consider a bunch of
		other activities first. (K-Mod's AI_guardBonus(15) moved down instead.) */
	if(kOwner.AI_getWaterDanger(plot(), std::min(maxMoves(), DANGER_RANGE), false) > 0 &&
			AI_guardBonus(10))
		return; // </advc.017b>

	if (AI_heal(30, 1))
	{
		return;
	}

	if (AI_anyAttack(1, 55))
	{
		return;
	}
	
	if (AI_seaBombardRange(6))
	{
		return;
	}
	
	//if (AI_protect(40))
	if (AI_defendTerritory(45, 0, 5)) // K-Mod
	{
		return;
	}
	
	/* original bts code
	if (AI_shadow(UNITAI_SETTLER_SEA, 1, -1, true))
	{
		return;
	}

	if (AI_group(UNITAI_RESERVE_SEA, 1))
	{
		return;
	}
	
	if (bombardRate() > 0)
	{
		if (AI_shadow(UNITAI_ASSAULT_SEA, 2, 30, true))
		{
			return;
		}
	} */
	// Shadow any nearby settler sea transport out at sea
	if (AI_shadow(UNITAI_SETTLER_SEA, 2, -1, false, true, 5))
	{
		return;
	}
	
	if (AI_group(UNITAI_RESERVE_SEA, 1, -1, -1, false, false, false, 8))
	{
		return;
	}
	
	if (bombardRate() > 0)
	{
		if (AI_shadow(UNITAI_ASSAULT_SEA, 2, 30, true, false, 8))
		{
			return;
		}
	}

	if (AI_heal(50, 3))
	{
		return;
	}

	//if (AI_protect(40))
	if (AI_defendTerritory(45, 0, -1)) // K-Mod
	{
		return;
	}

	if (AI_anyAttack(3, 45))
	{
		return;
	}

	if (AI_heal())
	{
		return;
	}

	if (!isNeverInvisible())
	{
		if (AI_anyAttack(5, 35))
		{
			return;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/03/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                      */
/************************************************************************************************/
	// Shadow settler transport with cargo 
	if (AI_shadow(UNITAI_SETTLER_SEA, 1, -1, true, false, 10))
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	// advc.017b: Moved this down
	if (AI_guardBonus(15)) // K-Mod (note: this will defend seafood when we have exactly 1 of them)
	{
		return;
	}

	if (AI_travelToUpgradeCity())
	{
		return;
	}

	// K-Mod
	if (AI_guardBonus(10))
		return;

	if (AI_guardCoast())
		return;
	// K-Mod end

	if (AI_patrol())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_escortSeaMove()
{
	PROFILE_FUNC();
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod

//	// if we have cargo, possibly convert to UNITAI_ASSAULT_SEA (this will most often happen with galleons)
//	// note, this should not happen when we are not the group head, so escort galleons are fine joining a group, just not as head
//	if (hasCargo() && (getUnitAICargo(UNITAI_ATTACK_CITY) > 0 || getUnitAICargo(UNITAI_ATTACK) > 0))
//	{
//		// non-zero AI_unitValue means that UNITAI_ASSAULT_SEA is valid for this unit (that is the check used everywhere)
//		if (GET_PLAYER(getOwnerINLINE()).AI_unitValue(getUnitType(), UNITAI_ASSAULT_SEA, NULL) > 0)
//		{
//			// save old group, so we can merge it back in
//			CvSelectionGroup* pOldGroup = getGroup();
//
//			// this will remove this unit from the current group
//			AI_setUnitAIType(UNITAI_ASSAULT_SEA);
//
//			// merge back the rest of the group into the new group
//			CvSelectionGroup* pNewGroup = getGroup();
//			if (pOldGroup != pNewGroup)
//			{
//				pOldGroup->mergeIntoGroup(pNewGroup);
//			}
//
//			// perform assault sea action
//			AI_assaultSeaMove();
//			return;
//		}
//	}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						06/14/09	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true)) //prioritize getting outta there
	{
		if(AI_isThreatenedFromLand()) // advc.139: Code moved into subroutine
		{
			if (AI_anyAttack(1, 60))
			{
				return;
			}

			if (AI_group(UNITAI_ASSAULT_SEA, -1, /*iMaxOwnUnitAI*/ 1, -1, /*bIgnoreFaster*/ true, false, false, /*iMaxPath*/ getMoves()))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	if (AI_heal(30, 1))
	{
		return;
	}

	if (AI_anyAttack(1, 55))
	{
		return;
	}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						9/14/08			jdog5000		*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	// Galleons can get stuck with this AI type since they don't upgrade to any escort unit
	// Galleon escorts are much less useful once Frigates or later are available
	if (!isHuman() && !isBarbarian())
	{
		if( getCargo() > 0 && (GC.getUnitInfo(getUnitType()).getSpecialCargo() == NO_SPECIALUNIT) )
		{
			//Obsolete?
			int iValue = kOwner.AI_unitValue(getUnitType(), AI_getUnitAIType(), area());
			int iBestValue = kOwner.AI_bestAreaUnitAIValue(AI_getUnitAIType(), area());
			
			if (iValue < iBestValue)
			{
				if (kOwner.AI_unitValue(getUnitType(), UNITAI_ASSAULT_SEA, area()) > 0)
				{
					AI_setUnitAIType(UNITAI_ASSAULT_SEA);
					return;
				}

				if (kOwner.AI_unitValue(getUnitType(), UNITAI_SETTLER_SEA, area()) > 0)
				{
					AI_setUnitAIType(UNITAI_SETTLER_SEA);
					return;
				}

				scrap();
				return; // advc.001: Always return after scrap
			}
		}
	}	
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/
	
	if (AI_group(UNITAI_CARRIER_SEA, -1, /*iMaxOwnUnitAI*/ 0, -1, /*bIgnoreFaster*/ true))
	{
		return;
	}
		
	if (AI_group(UNITAI_ASSAULT_SEA, -1, /*iMaxOwnUnitAI*/ 0, -1, /*bIgnoreFaster*/ true, false, false, /*iMaxPath*/ 3))
	{
		return;
	}

	if (AI_heal(50, 3))
	{
		return;
	}

	if (AI_pillageRange(2))
	{
		return;
	}
	
	//if (AI_group(UNITAI_MISSILE_CARRIER_SEA, 1, 1, true))
	if (AI_group(UNITAI_MISSILE_CARRIER_SEA, 1, 1, -1, true)) // K-Mod. (presumably this is what they meant)
	{
		return;
	}

	if (AI_group(UNITAI_ASSAULT_SEA, 1, /*iMaxOwnUnitAI*/ 0, /*iMinUnitAI*/ -1, /*bIgnoreFaster*/ true))
	{
		return;
	}
	
	if (AI_group(UNITAI_ASSAULT_SEA, -1, /*iMaxOwnUnitAI*/ 2, /*iMinUnitAI*/ -1, /*bIgnoreFaster*/ true))
	{
		return;
	}
	
	if (AI_group(UNITAI_CARRIER_SEA, -1, /*iMaxOwnUnitAI*/ 2, /*iMinUnitAI*/ -1, /*bIgnoreFaster*/ true))
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/01/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                      */
/************************************************************************************************/
/* original bts code
	if (AI_group(UNITAI_ASSAULT_SEA, -1, 4, -1, true))
	{
		return;
	}
*/
	// Group only with large flotillas first
	if (AI_group(UNITAI_ASSAULT_SEA, -1, /*iMaxOwnUnitAI*/ 4, /*iMinUnitAI*/ 3, /*bIgnoreFaster*/ true))
	{
		return;
	}

	if (AI_shadow(UNITAI_SETTLER_SEA, 2, -1, false, true, 4))
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/	

	if (AI_heal())
	{
		return;
	}

	if (AI_travelToUpgradeCity())
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/18/10                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
	// If nothing else useful to do, escort nearby large flotillas even if they're faster
	// Gives Caravel escorts something to do during the Galleon/pre-Frigate era
	if (AI_group(UNITAI_ASSAULT_SEA, -1, /*iMaxOwnUnitAI*/ 4, /*iMinUnitAI*/ 3, /*bIgnoreFaster*/ false, false, false, 4, false, true))
	{
		return;
	}

	if (AI_group(UNITAI_ASSAULT_SEA, -1, /*iMaxOwnUnitAI*/ 2, /*iMinUnitAI*/ -1, /*bIgnoreFaster*/ false, false, false, 1, false, true))
	{
		return;
	}

	// Pull back to primary area if it's not too far so primary area cities know you exist
	// and don't build more, unnecessary escorts
	/* original code
	if (AI_retreatToCity(true,false,6))
	{
		return;
	} */
	// K-Mod. We don't want to actually end our turn inside the city...
	if (AI_guardCoast(true))
		return;
	// K-Mod end
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_exploreSeaMove()
{
	PROFILE_FUNC();
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/21/08	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true)) //prioritize getting outta there
	{
		if(AI_isThreatenedFromLand()) // advc.139: Code moved into subroutine
		{
			if (!isHuman())
			{
				if (AI_anyAttack(1, 60))
				{
					return;
				}
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	CvArea* pWaterArea = plot()->waterArea();

	if (!isHuman())
	{
		if (AI_anyAttack(1, 60))
		{
			return;
		}
	}
	// advc.017b: Moved the transform/ scrap block down; try nearby exploration first.

	if (getDamage() > 0)
	{
		if ((plot()->getFeatureType() == NO_FEATURE) || (GC.getFeatureInfo(plot()->getFeatureType()).getTurnDamage() == 0))
		{
			getGroup()->pushMission(MISSION_HEAL);
			return;
		}
	}

	if (!isHuman())
	{
		if (AI_pillageRange(1))
		{
			return;
		}
	}

	if (AI_exploreRange(4))
	{
		return;
	}

	if (!isHuman())
	{
		if (AI_pillageRange(4))
		{
			return;
		}
	}
	// advc.017b: Moved this chunk of code down
	bool bExcessExplorers = false;
	if (!isHuman() && !isBarbarian()) //XXX move some of this into a function? maybe useful elsewhere
	{	// <advc.017b>
		bool bTransform = false;
		// Don't be too quick to decide that there are too many explorers
		if(::bernoulliSuccess(0.13, "advc.017b")) {
			/*  Be careful not to convert or scrap a unit that CvPlayerAI thinks we need
				(b/c otherwise cities will keep training explorers and they'll keep
				getting converted to other AI types) */
			CvArea* pSecondWaterArea = NULL;
			if(pWaterArea == NULL)
				pSecondWaterArea = plot()->secondWaterArea();
			else FAssert(pWaterArea != NULL);
			// Subtract explorers still being trained
			int iInTraining = kOwner.AI_getNumTrainAIUnits(UNITAI_EXPLORE_SEA);
			bool bTransform = ((pWaterArea == NULL ||
					kOwner.AI_neededExplorers(pWaterArea) <
					kOwner.AI_totalWaterAreaUnitAIs(pWaterArea, UNITAI_EXPLORE_SEA) -
					iInTraining) && (pSecondWaterArea == NULL ||
					kOwner.AI_neededExplorers(pSecondWaterArea) <
					kOwner.AI_totalWaterAreaUnitAIs(pSecondWaterArea, UNITAI_EXPLORE_SEA) -
					kOwner.AI_getNumTrainAIUnits(UNITAI_EXPLORE_SEA)));
			bExcessExplorers = bTransform;
		}
		if(!bTransform &&
				/*  In the early game, it'll often take a better explorer (Galley
					vs Work Boat) too long to reach an unexplored area; better to
					let the outdated explorer continue. */
				kOwner.getCurrentEra() > 1) {
			int iValue = kOwner.AI_unitValue(getUnitType(), UNITAI_EXPLORE_SEA, pWaterArea);
			for(int i = 0; i < GC.getNumUnitClassInfos(); i++) {
				UnitClassTypes uct = (UnitClassTypes)i;
				/*  Will eventually build some Caravels as attackers even if
					explorers are maxed out. Could alternatively check
					kOwner.canTrain(ut), but that's slightly more expensive. */
				if(kOwner.getUnitClassCount(uct) <= 0)
					continue;
				UnitTypes ut = (UnitTypes)GC.getCivilizationInfo(kOwner.
						getCivilizationType()).getCivilizationUnits(uct);
				int iLoopValue = kOwner.AI_unitValue(ut, UNITAI_EXPLORE_SEA, pWaterArea);
				if(75 * iLoopValue > 100 * iValue) {
					bTransform = true;
					break;
				}
			}
		} // Moved the obsoletion test; now only required for scrapping
		if(bTransform) { // </advc.017b>
			// <advc.003> Made this more concise (original code deleted)
			std::vector<UnitAITypes> transformTypes;
			transformTypes.push_back(UNITAI_WORKER_SEA);
			transformTypes.push_back(UNITAI_PIRATE_SEA);
			AreaAITypes aai = area()->getAreaAIType(getTeam());
			if(aai == AREAAI_ASSAULT || aai == AREAAI_ASSAULT_ASSIST ||
					aai == AREAAI_ASSAULT_MASSING ||
					kOwner.AI_totalUnitAIs(UNITAI_SETTLE) <= 0 ||
					kOwner.AI_totalUnitAIs(UNITAI_SETTLER_SEA) > kOwner.getCurrentEra() / 2) {
				transformTypes.push_back(UNITAI_ASSAULT_SEA);
				transformTypes.push_back(UNITAI_SETTLER_SEA);
			}
			else {
				transformTypes.push_back(UNITAI_SETTLER_SEA);
				transformTypes.push_back(UNITAI_ASSAULT_SEA);
			}
			// <advc.017b> Instead of always trying MISSIONARY_SEA before RESERVE_SEA
			if((kOwner.AI_totalUnitAIs(UNITAI_MISSIONARY) > 0 ||
					kOwner.AI_isDoStrategy(AI_STRATEGY_MISSIONARY)) &&
					kOwner.AI_totalUnitAIs(UNITAI_MISSIONARY_SEA) <= 1) {
				transformTypes.push_back(UNITAI_MISSIONARY_SEA);
				transformTypes.push_back(UNITAI_RESERVE_SEA);
			}
			else {
				transformTypes.push_back(UNITAI_RESERVE_SEA);
				transformTypes.push_back(UNITAI_MISSIONARY_SEA);
			} // </advc.017b>
			for(size_t i = 0; i < transformTypes.size(); i++) {
				if(m_pUnitInfo->getUnitAIType(transformTypes[i]) &&
						kOwner.AI_unitValue(getUnitType(), transformTypes[i], pWaterArea) > 0) {
					// Before transforming a Work Boat, check if there is sth. to improve.
					if(transformTypes[i] == UNITAI_WORKER_SEA &&
							/*  Costly (checks all tiles on the map), but this is the
								last check before changing the AI type. And exploring
								Work Boats are an early-game thing. */
							kOwner.AI_countUnimprovedBonuses(pWaterArea, plot(), 5) <= 0)
						continue;
					AI_setUnitAIType(transformTypes[i]);
					return;
				}
			} // </advc.003>
			// <advc.017b>
			int iValue = kOwner.AI_unitValue(getUnitType(), AI_getUnitAIType(), pWaterArea);
			int iBestValue = kOwner.AI_bestAreaUnitAIValue(AI_getUnitAIType(), pWaterArea);
			if (iValue < iBestValue) {
				scrap();
				return;
			}
		} // </advc.017b>
	}

	if (AI_explore())
	{
		return;
	}

	if (!isHuman())
	{
		if (AI_pillage())
		{
			return;
		}
	}
	
	if (!isHuman())
	{
		if (AI_travelToUpgradeCity())
		{
			return;
		}
	}

	if (!(isHuman()) && (AI_getUnitAIType() == UNITAI_EXPLORE_SEA))
	{
		pWaterArea = plot()->waterArea();

		if (pWaterArea != NULL)
		{
			if(bExcessExplorers) // advc.017b
			{
				if (kOwner.calculateUnitCost() > 0)
				{
					scrap();
					return;
				}
			}
		}
	}

	if (AI_patrol())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/18/10                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
void CvUnitAI::AI_assaultSeaMove()
{
	PROFILE_FUNC();
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod

	FAssert(AI_getUnitAIType() == UNITAI_ASSAULT_SEA);

	bool bEmpty = !getGroup()->hasCargo();
	bool bFull = getGroup()->getCargo() > 0 && getGroup()->AI_isFull();

	if (plot()->isCity(true))
	{
		// K-Mod
		int iOurDefense = kOwner.AI_localDefenceStrength(plot(), getTeam(), DOMAIN_LAND, 0);
		int iEnemyOffense = kOwner.AI_localAttackStrength(plot(), NO_TEAM, DOMAIN_LAND, 2);
		// K-Mod end

		if( getDamage() > 0 )	// extra risk to leaving when wounded
		{
			iOurDefense *= 2;
		}

		if (iEnemyOffense > iOurDefense/4) // was 1 vs 1/8
		{
			if (iEnemyOffense > iOurDefense/2) // was 1 vs 1/4
			{
				if( !bEmpty )
				{
					getGroup()->unloadAll();
				}

				if (AI_anyAttack(1, 65))
				{
					return;
				}

				// Retreat to primary area first
				if (AI_retreatToCity(true))
				{
					return;
				}

				if (AI_retreatToCity())
				{
					return;
				}

				if (AI_safety())
				{
					return;
				}
			}

			if( !bFull && !bEmpty )
			{
				getGroup()->unloadAll();
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
		}
	}

	if (bEmpty)
	{
		if (AI_anyAttack(1, 65))
		{
			return;
		}
		/*if (AI_anyAttack(1, 45))
		{
			return;
		}*/ // disabled by K-Mod. (redundant)
	}

	bool bReinforce = false;
	bool bAttack = false;
	bool bNoWarPlans = (GET_TEAM(getTeam()).getAnyWarPlanCount(true) == 0);
	bool bAttackBarbarian = false;
	//bool bLandWar = false;
	bool bBarbarian = isBarbarian();
	
	// Count forts as cities
	bool bCity = plot()->isCity(true);

	// Cargo if already at war
	//int iTargetReinforcementSize = (bBarbarian ? AI_stackOfDoomExtra() : 2);
	int iTargetReinforcementSize = (bBarbarian ? 2 : AI_stackOfDoomExtra()); // K-Mod. =\

	// Cargo to launch a new invasion
	int iTargetInvasionSize = 2*iTargetReinforcementSize;

	// K-Mod. If we are already en route for invasion, decrease the threshold.
	// (One reason for this decrease is that the threshold may actually increase midway through the journey. We don't want to turn back because of that!)
	if (getGroup()->AI_getMissionAIType() == MISSIONAI_ASSAULT)
	{
		iTargetReinforcementSize = iTargetReinforcementSize*2/3;
		iTargetInvasionSize = iTargetInvasionSize*2/3;
	}
	// K-Mod end

	int iCargo = getGroup()->getCargo();
	int iEscorts = getGroup()->countNumUnitAIType(UNITAI_ESCORT_SEA) + getGroup()->countNumUnitAIType(UNITAI_ATTACK_SEA);

	AreaAITypes eAreaAIType = area()->getAreaAIType(getTeam());
	//bLandWar = !bBarbarian && ((eAreaAIType == AREAAI_OFFENSIVE) || (eAreaAIType == AREAAI_DEFENSIVE) || (eAreaAIType == AREAAI_MASSING));
	bool bLandWar = !bBarbarian && kOwner.AI_isLandWar(area()); // K-Mod

	// Plot danger case handled above

	if( hasCargo() && (getUnitAICargo(UNITAI_SETTLE) > 0 || getUnitAICargo(UNITAI_WORKER) > 0) )
	{
		// Dump inappropriate load at first oppurtunity after pick up
		if( bCity && (plot()->getOwnerINLINE() == getOwnerINLINE()) )
		{		
			getGroup()->unloadAll();
			/* original code
			getGroup()->pushMission(MISSION_SKIP);
			return; */
			iCargo = 0; // K-Mod. I see no need to skip.
		}
		else
		{
			if( !isFull() )
			{
				if(AI_pickupStranded(NO_UNITAI, 1))
				{
					return;
				}
			}

			if (AI_retreatToCity(true))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}
		}
	}

	if (bCity)
	{
		CvCity* pCity = plot()->getPlotCity();

		if( pCity != NULL && (plot()->getOwnerINLINE() == getOwnerINLINE()) ) 
		{
			// split out galleys from stack of ocean capable ships
			if( kOwner.AI_unitImpassableCount(getUnitType()) == 0 && getGroup()->getNumUnits() > 1 )
			{
				//getGroup()->AI_separateImpassable();
				// K-Mod
				if (getGroup()->AI_separateImpassable())
				{
					// recalculate cached variables.
					bEmpty = !getGroup()->hasCargo();
					bFull = getGroup()->getCargo() > 0 && getGroup()->AI_isFull();
					iCargo = getGroup()->getCargo();
					iEscorts = getGroup()->countNumUnitAIType(UNITAI_ESCORT_SEA) + getGroup()->countNumUnitAIType(UNITAI_ATTACK_SEA);
				}
				// K-Mod end
			}

			// galleys with upgrade available should get that ASAP
			if( kOwner.AI_unitImpassableCount(getUnitType()) > 0 )
			{
				CvCity* pUpgradeCity = getUpgradeCity(false);
				if( pUpgradeCity != NULL && pUpgradeCity == pCity )
				{
					// Wait for upgrade, this unit is top upgrade priority
					getGroup()->pushMission(MISSION_SKIP);
					return;
				}
			}
		}

		if( (iCargo > 0) )
		{
			if( pCity != NULL )
			{
				if( (GC.getGameINLINE().getGameTurn() - pCity->getGameTurnAcquired()) <= 1 )
				{
					if( pCity->getPreviousOwner() != NO_PLAYER )
					{
						// Just captured city, probably from naval invasion.  If area targets, drop cargo and leave so as to not to be lost in quick counter attack
						if( GET_TEAM(getTeam()).countEnemyPowerByArea(plot()->area()) > 0 )
						{
							getGroup()->unloadAll();

							if( iEscorts > 2 )
							{
								if( getGroup()->countNumUnitAIType(UNITAI_ESCORT_SEA) > 1 && getGroup()->countNumUnitAIType(UNITAI_ATTACK_SEA) > 0 )
								{
									getGroup()->AI_separateAI(UNITAI_ATTACK_SEA);
									getGroup()->AI_separateAI(UNITAI_RESERVE_SEA);

									iEscorts = getGroup()->countNumUnitAIType(UNITAI_ESCORT_SEA);
								}
							}
							iCargo = getGroup()->getCargo();
						}
					}
				}
			}
		}

		if( (iCargo > 0) && (iEscorts == 0) )
		{
			if (AI_group(UNITAI_ASSAULT_SEA,-1,-1,-1,/*bIgnoreFaster*/true,false,false,/*iMaxPath*/1,false,/*bCargoOnly*/true,false,MISSIONAI_ASSAULT))
			{
				return;
			}

			if( plot()->plotCount(PUF_isUnitAIType, UNITAI_ESCORT_SEA, -1, getOwnerINLINE(), NO_TEAM, PUF_isGroupHead, -1, -1) > 0 )
			{
				// Loaded but with no escort, wait for escorts in plot to join us
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}

			MissionAITypes eMissionAIType = MISSIONAI_GROUP;
			if( (kOwner.AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 3) > 0) || (kOwner.AI_getWaterDanger(plot(), 4, false) > 0) )
			{
				// Loaded but with no escort, wait for others joining us soon or avoid dangerous waters
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
		}

		if (bLandWar)
		{
			if ( iCargo > 0 )
			{
				if( (eAreaAIType == AREAAI_DEFENSIVE) || (pCity != NULL && pCity->AI_isDanger()))
				{
					// Unload cargo when on defense or if small load of troops and can reach enemy city over land (generally less risky)
					getGroup()->unloadAll();
					getGroup()->pushMission(MISSION_SKIP);
					return;
				}
			}

			if (iCargo >= iTargetReinforcementSize)
			{
				getGroup()->AI_separateEmptyTransports();

				if( !(getGroup()->hasCargo()) )
				{
					// this unit was empty group leader
					//getGroup()->pushMission(MISSION_SKIP);
					//return;
					// K-Mod. (and I've made a second if iCargo > thing)
					FAssert(getGroup()->getNumUnits() == 1);
					iCargo = 0;
					iEscorts = 0;
				}
			}
			if (iCargo >= iTargetReinforcementSize)
			{
				// Send ready transports
				if (AI_assaultSeaReinforce(false))
				{
					return;
				}

				//if( iCargo >= iTargetInvasionSize )
				// Disabled by K-Mod. (otherwise groups trying to take a short-cut by boat will get stuck.)
				{
					if (AI_assaultSeaTransport(false, true))
					{
						return;
					}
				}
			}
		}
		else
		{
			if ( (eAreaAIType == AREAAI_ASSAULT) )
			{
				if( iCargo >= iTargetInvasionSize )
				{
					bAttack = true;
				}
			}
			
			if( (eAreaAIType == AREAAI_ASSAULT) || (eAreaAIType == AREAAI_ASSAULT_ASSIST) )
			{
				if( (bFull && iCargo > cargoSpace()) || (iCargo >= iTargetReinforcementSize) )
				{
					bReinforce = true;
				}
			}
		}

		/* original bbai code
		if( !bAttack && !bReinforce && (plot()->getTeam() == getTeam()) )
		{
			if( iEscorts > 3 && iEscorts > (2*getGroup()->countNumUnitAIType(UNITAI_ASSAULT_SEA)) )
			{
				// If we have too many escorts, try freeing some for others
				getGroup()->AI_separateAI(UNITAI_ATTACK_SEA);
				getGroup()->AI_separateAI(UNITAI_RESERVE_SEA);

				iEscorts = getGroup()->countNumUnitAIType(UNITAI_ESCORT_SEA);
				if( iEscorts > 3 && iEscorts > (2*getGroup()->countNumUnitAIType(UNITAI_ASSAULT_SEA)) )
				{
					getGroup()->AI_separateAI(UNITAI_ESCORT_SEA);
				}
			}
		} */
		// K-Mod, same purpose, different implementation.
		// keep ungrouping escort units until we don't have too many.
		if (!bAttack && !bReinforce && plot()->getTeam() == getTeam())
		{
			int iAssaultUnits = getGroup()->countNumUnitAIType(UNITAI_ASSAULT_SEA);
			CLLNode<IDInfo>* pEntityNode = getGroup()->headUnitNode();
			while (iEscorts > 3 && iEscorts > 2*iAssaultUnits && iEscorts > 2*iCargo && pEntityNode != NULL)
			{
				CvUnit* pLoopUnit = ::getUnit(pEntityNode->m_data);
				pEntityNode = getGroup()->nextUnitNode(pEntityNode);
				// (maybe we should adjust this to ungroup "escorts" last?)
				if (!pLoopUnit->hasCargo())
				{
					switch (pLoopUnit->AI_getUnitAIType())
					{
					case UNITAI_ATTACK_SEA:
					case UNITAI_RESERVE_SEA:
					case UNITAI_ESCORT_SEA:
						pLoopUnit->joinGroup(NULL);
						iEscorts--;
						break;
					default:
						break;
					}
				}
			}
			FAssert(!(iEscorts > 3 && iEscorts > 2*iAssaultUnits && iEscorts > 2*iCargo));
		}
		// K-Mod end

		MissionAITypes eMissionAIType = MISSIONAI_GROUP;
		if( kOwner.AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 1) > 0 )
		{
			// Wait for units which are joining our group this turn
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}

		if( !bFull )
		{
			if( bAttack )
			{
				eMissionAIType = MISSIONAI_LOAD_ASSAULT;
				if( kOwner.AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 1) > 0 )
				{
					// Wait for cargo which will load this turn
					getGroup()->pushMission(MISSION_SKIP);
					return;
				}
			}
			else if( kOwner.AI_unitTargetMissionAIs(this, MISSIONAI_LOAD_ASSAULT) > 0 )
			{
				// Wait for cargo which is on the way
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
		}

		if( !bAttack && !bReinforce )
		{
			if ( iCargo > 0 )
			{
				if (AI_group(UNITAI_ASSAULT_SEA,-1,-1,-1,/*bIgnoreFaster*/true,false,false,/*iMaxPath*/5,false,/*bCargoOnly*/true,false,MISSIONAI_ASSAULT))
				{
					return;
				}
			}
			/* original code
			else if (plot()->getTeam() == getTeam() && getGroup()->getNumUnits() > 1)
			{
				CvCity* pCity = plot()->getPlotCity();
				if( pCity != NULL && (GC.getGameINLINE().getGameTurn() - pCity->getGameTurnAcquired()) > 10 )
				{
					if( pCity->plot()->plotCount(PUF_isAvailableUnitAITypeGroupie, UNITAI_ATTACK_CITY, -1, getOwnerINLINE()) < iTargetReinforcementSize )
					{
						// Not attacking, no cargo so release any escorts, attack ships, etc and split transports
						getGroup()->AI_makeForceSeparate();
					}
				}
			} */ // moved by K-Mod
		}
	}
	
	if (!bCity)
	{
		if( iCargo >= iTargetInvasionSize )
		{
			bAttack = true;
		}

		if ((iCargo >= iTargetReinforcementSize) || (bFull && iCargo > cargoSpace()))
		{
			bReinforce = true;
		}

		CvPlot* pAdjacentPlot = NULL;
		for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
		{
			pAdjacentPlot = plotDirection(getX_INLINE(), getY_INLINE(), ((DirectionTypes)iI));
			if( pAdjacentPlot != NULL )
			{
				if( iCargo > 0 )
				{
					CvCity* pAdjacentCity = pAdjacentPlot->getPlotCity();
					if( pAdjacentCity != NULL && pAdjacentCity->getOwner() == getOwnerINLINE() && pAdjacentCity->getPreviousOwner() != NO_PLAYER )
					{
						if( (GC.getGameINLINE().getGameTurn() - pAdjacentCity->getGameTurnAcquired()) < 5 )
						{
							// If just captured city and we have some cargo, dump units in city
							getGroup()->pushMission(MISSION_MOVE_TO, pAdjacentPlot->getX_INLINE(), pAdjacentPlot->getY_INLINE(), 0, false, false, NO_MISSIONAI, pAdjacentPlot);
							// K-Mod note: this use to use missionAI_assault. I've changed it because assault would suggest to the troops that they should stay onboard.
							return;
						}
					}
				}
				else 
				{
					if (pAdjacentPlot->isOwned() && isEnemy(pAdjacentPlot->getTeam()))
					{
						if( pAdjacentPlot->getNumDefenders(getOwnerINLINE()) > 2 )
						{
							// if we just made a dropoff in enemy territory, release sea bombard units to support invaders
							if ((getGroup()->countNumUnitAIType(UNITAI_ATTACK_SEA) + getGroup()->countNumUnitAIType(UNITAI_RESERVE_SEA)) > 0)
							{
								bool bMissionPushed = false;
								
								if (AI_seaBombardRange(1))
								{
									bMissionPushed = true;
								}

								CvSelectionGroup* pOldGroup = getGroup();

								//Release any Warships to finish the job.
								getGroup()->AI_separateAI(UNITAI_ATTACK_SEA);
								getGroup()->AI_separateAI(UNITAI_RESERVE_SEA);

								// Fixed bug in next line with checking unit type instead of unit AI
								if (pOldGroup == getGroup() && AI_getUnitAIType() == UNITAI_ASSAULT_SEA)
								{
									// Need to be sure all units can move
									if( getGroup()->canAllMove() )
									{
										if (AI_retreatToCity(true))
										{
											bMissionPushed = true;
										}
									}
								}

								if (bMissionPushed)
								{
									return;
								}
							}
						}
					}
				}
			}
		}

		if(iCargo > 0)
		{
			MissionAITypes eMissionAIType = MISSIONAI_GROUP;
			if( kOwner.AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 1) > 0 )
			{
				if( iEscorts < kOwner.AI_getWaterDanger(plot(), 2, false) )
				{
					// Wait for units which are joining our group this turn (hopefully escorts)
					getGroup()->pushMission(MISSION_SKIP);
					return;
				}
			}
		}
	}

	if (bBarbarian)
	{
		if (getGroup()->isFull() || iCargo > iTargetInvasionSize)
		{
			if (AI_assaultSeaTransport(false))
			{
				return;
			}
		}
		else
		{
			if (AI_pickup(UNITAI_ATTACK_CITY, true, 5))
			{
				return;
			}

			if (AI_pickup(UNITAI_ATTACK, true, 5))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if( !(getGroup()->getCargo()) )
			{
				AI_barbAttackSeaMove();
				return;
			}

			if( AI_safety() )
			{
				return;
			}

			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	}
	else
	{
		if (bAttack || bReinforce)
		{
			if( bCity )
			{
				getGroup()->AI_separateEmptyTransports();
			}

			if( !(getGroup()->hasCargo()) )
			{
				// this unit was empty group leader
				//getGroup()->pushMission(MISSION_SKIP);
				//return;
				bAttack = bReinforce = false; // K-Mod
				iCargo = 0;
			}
		}
		if (bAttack || bReinforce) // K-Mod
		{
			FAssert(getGroup()->hasCargo());

			//BBAI TODO: Check that group has escorts, otherwise usually wait

			if( bAttack )
			{
				if( bReinforce && (AI_getBirthmark()%2 == 0) )
				{
					if (AI_assaultSeaReinforce())
					{
						return;
					}
					bReinforce = false;
				}

				if (AI_assaultSeaTransport())
				{
					return;
				}
			}

			// If not enough troops for own invasion, 
			if( bReinforce )
			{
				if (AI_assaultSeaReinforce())
				{
					return;
				}	
			}
		}

		if( bNoWarPlans && (iCargo >= iTargetReinforcementSize) )
		{
			bAttackBarbarian = true;

			getGroup()->AI_separateEmptyTransports();

			if( !(getGroup()->hasCargo()) )
			{
				// this unit was empty group leader
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}

			FAssert(getGroup()->hasCargo());
			if (AI_assaultSeaReinforce(bAttackBarbarian))
			{
				return;
			}

			FAssert(getGroup()->hasCargo());
			if (AI_assaultSeaTransport(bAttackBarbarian))
			{
				return;
			}
		}
	} // <advc.046>
	bool bHasCargo = getGroup()->hasCargo(); // Moved up
	/*  If we have room, or are in a city where we could unload, check if there is
		a good stranded target. This is more important than drawing units together
		when no naval attack is planned (!bAttack). */
	bool const bGoodCity = (plot()->isCity(false, getTeam()) &&
			GET_TEAM(getTeam()).AI_isPrimaryArea(plot()->area()));
	if((bGoodCity || !bHasCargo) && !bAttack &&
			AI_pickupStranded())
		return; // </advc.046>
	if ((bFull || bReinforce) && !bAttack)
	{
		// Group with nearby transports with units on board
		/* original code
		if (AI_group(UNITAI_ASSAULT_SEA, -1, -1, -1, true, false, false, 2, false, true, false, MISSIONAI_ASSAULT))
		{
			return;
		} */ // disabled by K-Mod. This is redundant.

		//if (AI_group(UNITAI_ASSAULT_SEA, -1, -1, -1, true, false, false, 10, false, true, false, MISSIONAI_ASSAULT))
		if (AI_omniGroup(UNITAI_ASSAULT_SEA, -1, -1, false, 0, 10, true, true, true, false, false, -1, true, true))
		{
			return;
		}
	}
	else if( !bFull )
	{
		bool bHasOneLoad = (getGroup()->getCargo() >= cargoSpace());

		if (AI_pickup(UNITAI_ATTACK_CITY, !bHasCargo, (bHasOneLoad ? 3 : 7)))
		{
			return;
		}

		if (AI_pickup(UNITAI_ATTACK, !bHasCargo, (bHasOneLoad ? 3 : 7)))
		{
			return;
		}
		
		if (AI_pickup(UNITAI_COUNTER, !bHasCargo, (bHasOneLoad ? 3 : 7)))
		{
			return;
		}

		if (AI_pickup(UNITAI_ATTACK_CITY, !bHasCargo))
		{
			return;
		}

		if( !bHasCargo )
		{
			if(AI_pickupStranded(UNITAI_ATTACK_CITY))
			{
				return;
			}

			if(AI_pickupStranded(UNITAI_ATTACK))
			{
				return;
			}

			if(AI_pickupStranded(UNITAI_COUNTER))
			{
				return;
			}

			if( (getGroup()->countNumUnitAIType(AI_getUnitAIType()) == 1) )
			{
				// Try picking up any thing
				if(AI_pickupStranded())
				{
					return;
				}
			}
		}
	}

	//if (bCity && bLandWar && getGroup()->hasCargo())
	if (bCity)
	{
		FAssert(iCargo == getGroup()->getCargo());
		if (bLandWar && iCargo > 0)
		{
			// Enemy units in this player's territory
			if (kOwner.AI_countNumAreaHostileUnits(area(),true,false,false,false) > iCargo/2)
			{
				getGroup()->unloadAll();
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
		}
		// K-Mod. (moved from way higher up)
		if (iCargo == 0 && plot()->getTeam() == getTeam() && getGroup()->getNumUnits() > 1)
		{
			getGroup()->AI_separate();
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	}
	
	if (AI_retreatToCity(true))
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


void CvUnitAI::AI_settlerSeaMove()
{
	PROFILE_FUNC();
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod
	
	bool bEmpty = !getGroup()->hasCargo();

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/21/08	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true))
	{
		if(AI_isThreatenedFromLand()) // advc.139: Code moved into subroutine
		{
			if( bEmpty )
			{
				if (AI_anyAttack(1, 65))
				{
					return;
				}
			}

			// Retreat to primary area first
			if (AI_retreatToCity(true))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	if (bEmpty)
	{
		if (AI_anyAttack(1, 65))
		{
			return;
		}
		if (AI_anyAttack(1, 40))
		{
			return;
		}		
	}
	
	int iSettlerCount = getUnitAICargo(UNITAI_SETTLE);
	int iWorkerCount = getUnitAICargo(UNITAI_WORKER);

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/07/08                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
	if( hasCargo() && (iSettlerCount == 0) && (iWorkerCount == 0))
	{
		// Dump troop load at first oppurtunity after pick up
		if( plot()->isCity() && plot()->getOwnerINLINE() == getOwnerINLINE() )
		{
			getGroup()->unloadAll();
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
		else
		{
			if( !(isFull()) )
			{
				if(AI_pickupStranded(NO_UNITAI, 1))
				{
					return;
				}
			}

			if (AI_retreatToCity(true))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/02/09                                jdog5000      */
/*                                                                                              */
/* Settler AI                                                                                   */
/************************************************************************************************/
	// Don't send transport with settler and no defense
	if( (iSettlerCount > 0) && (iSettlerCount + iWorkerCount == cargoSpace()) )
	{
		// No defenders for settler
		if( plot()->isCity() && plot()->getOwnerINLINE() == getOwnerINLINE() )
		{
			getGroup()->unloadAll();
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	} 

	if ((iSettlerCount > 0) && (isFull() ||
			((getUnitAICargo(UNITAI_CITY_DEFENSE) > 0) &&
			 (kOwner.AI_unitTargetMissionAIs(this, MISSIONAI_LOAD_SETTLER) == 0))))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	{
		if (AI_settlerSeaTransport())
		{
			return;
		}
	}
	else if ((getTeam() != plot()->getTeam()) && bEmpty)
	{
		if (AI_pillageRange(3))
		{
			return;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
	if (plot()->isCity() && plot()->getOwnerINLINE() == getOwnerINLINE() && !hasCargo())
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	{
		AreaAITypes eAreaAI = area()->getAreaAIType(getTeam());
		if ((eAreaAI == AREAAI_ASSAULT) || (eAreaAI == AREAAI_ASSAULT_MASSING))
		{
			CvArea* pWaterArea = plot()->waterArea();
			FAssert(pWaterArea != NULL);
			if (pWaterArea != NULL)
			{
				if (kOwner.AI_totalWaterAreaUnitAIs(pWaterArea, UNITAI_SETTLER_SEA) > 1
						// <advc.017b>
						+ kOwner.getCurrentEra() / 2
						/*  Also convert if no colonies (which may need Workers)
							and no Settler on the horizon. */
						|| (kOwner.AI_totalUnitAIs(UNITAI_SETTLE) <= 0 &&
						// </advc.017b>
						kOwner.getNumCities() == area()->getCitiesPerPlayer(getOwnerINLINE()) &&
						kOwner.AI_totalUnitAIs(UNITAI_ASSAULT_SEA) <= 5))
				{
					if (kOwner.AI_unitValue(getUnitType(), UNITAI_ASSAULT_SEA, pWaterArea) > 0)
					{
						AI_setUnitAIType(UNITAI_ASSAULT_SEA);
						AI_assaultSeaMove();
						return;
					}
				}
			}
		}
	}
	
	if ((iWorkerCount > 0)
		&& kOwner.AI_unitTargetMissionAIs(this, MISSIONAI_LOAD_SETTLER) == 0)
	{
		if (isFull() || (iSettlerCount == 0))
		{
			if (AI_settlerSeaFerry())
			{
				return;
			}
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Settler AI                                                                                   */
/************************************************************************************************/
/* original bts code
	if (AI_pickup(UNITAI_SETTLE))
	{
		return;
	}
*/
	if( !(getGroup()->hasCargo()) )
	{
		if(AI_pickupStranded(UNITAI_SETTLE))
		{
			return;
		}
	}

	if( !(getGroup()->isFull()) )
	{
		if( kOwner.AI_unitTargetMissionAIs(this, MISSIONAI_LOAD_SETTLER) > 0 )
		{
			// Wait for units on the way
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}

		if( iSettlerCount > 0 )
		{
			if (AI_pickup(UNITAI_CITY_DEFENSE))
			{
				return;
			}
		}
		else if( cargoSpace() - 2 >= getCargo() + iWorkerCount )
		{
			if (AI_pickup(UNITAI_SETTLE, true))
			{
				return;
			}
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/	
	
	//if ((GC.getGame().getGameTurn() - getGameTurnCreated()) < 8)
	if (GC.getGame().getGameTurn() - getGameTurnCreated() < 8 && plot()->waterArea() &&
		kOwner.AI_areaMissionAIs(plot()->waterArea(), MISSIONAI_EXPLORE, getGroup()) < kOwner.AI_neededExplorers(plot()->waterArea())) // K-Mod
	{
		if ((plot()->getPlotCity() == NULL) || kOwner.AI_totalAreaUnitAIs(plot()->area(), UNITAI_SETTLE) == 0)
		{
			if (AI_explore())
			{
				return;
			}
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
/* original bts code
	if (AI_pickup(UNITAI_WORKER))
	{
		return;
	}
*/
	if( !getGroup()->hasCargo() )
	{
		// Rescue stranded non-settlers
		if(AI_pickupStranded())
		{
			return;
		}
	}
	
	//if( cargoSpace() - 2 < getCargo() + iWorkerCount )
	// advc.rom: (Koshling); relevant excerpt of his comment:
	/*	old condition here was broken for transports with a max capacity of 1 [...],
		and (after reading the old code) I think more generally anyway. [...] */
	if( iWorkerCount > 0 && cargoSpace() > 1 && cargoSpace() - getCargo() < 2 )
	{
		// If full of workers and not going anywhere, dump them if a settler is available
		if( (iSettlerCount == 0) && (plot()->plotCount(PUF_isAvailableUnitAITypeGroupie, UNITAI_SETTLE, -1, getOwnerINLINE(), NO_TEAM, PUF_isFiniteRange) > 0) )
		{
			getGroup()->unloadAll();

			if (AI_pickup(UNITAI_SETTLE, true))
			{
				return;
			}

			return;
		}
	}
	
	if( !(getGroup()->isFull()) )
	{
		if (AI_pickup(UNITAI_WORKER))
		{
			return;
		}
	}

	// Carracks cause problems for transport upgrades, galleys can't upgrade to them and they can't
	// upgrade to galleons.  Scrap galleys, switch unit AI for stuck Carracks.
	if( plot()->isCity() && plot()->getOwnerINLINE() == getOwnerINLINE() )
	{
		//
		{
			UnitTypes eBestSettlerTransport = NO_UNIT;
			kOwner.AI_bestCityUnitAIValue(AI_getUnitAIType(), NULL, &eBestSettlerTransport);
			if( eBestSettlerTransport != NO_UNIT )
			{
				if( eBestSettlerTransport != getUnitType() && kOwner.AI_unitImpassableCount(eBestSettlerTransport) == 0 )
				{
					UnitClassTypes ePotentialUpgradeClass = (UnitClassTypes)GC.getUnitInfo(eBestSettlerTransport).getUnitClassType();
					if( !upgradeAvailable(getUnitType(), ePotentialUpgradeClass) )
					{
						getGroup()->unloadAll();

						if( kOwner.AI_unitImpassableCount(getUnitType()) > 0 )
						{
							scrap();
							return;
						}
						else
						{
							CvArea* pWaterArea = plot()->waterArea();
							FAssert(pWaterArea != NULL);
							if (pWaterArea != NULL)
							{
								if( kOwner.AI_totalUnitAIs(UNITAI_EXPLORE_SEA) == 0 )
								{
									if (kOwner.AI_unitValue(getUnitType(), UNITAI_EXPLORE_SEA, pWaterArea) > 0)
									{
										AI_setUnitAIType(UNITAI_EXPLORE_SEA);
										AI_exploreSeaMove();
										return;
									}
								}

								if( kOwner.AI_totalUnitAIs(UNITAI_SPY_SEA) == 0 )
								{
									if (kOwner.AI_unitValue(getUnitType(), UNITAI_SPY_SEA, area()) > 0)
									{
										AI_setUnitAIType(UNITAI_SPY_SEA);
										AI_spySeaMove();
										return;
									}
								}

								if( kOwner.AI_totalUnitAIs(UNITAI_MISSIONARY_SEA) == 0 )
								{
									if (kOwner.AI_unitValue(getUnitType(), UNITAI_MISSIONARY_SEA, area()) > 0)
									{
										AI_setUnitAIType(UNITAI_MISSIONARY_SEA);
										AI_missionarySeaMove();
										return;
									}
								}

								if (kOwner.AI_unitValue(getUnitType(), UNITAI_ATTACK_SEA, pWaterArea) > 0)
								{
									AI_setUnitAIType(UNITAI_ATTACK_SEA);
									AI_attackSeaMove();
									return;
								}
							}
						}
					}
				}
			}
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		
	if (AI_retreatToCity(true))
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_missionarySeaMove()
{
	PROFILE_FUNC();
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/21/08	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true))
	{
		if(AI_isThreatenedFromLand()) // advc.139: Code moved into subroutine
		{
			// Retreat to primary area first
			if (AI_retreatToCity(true))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	if (getUnitAICargo(UNITAI_MISSIONARY) > 0)
	{
		if (AI_specialSeaTransportMissionary())
		{
			return;
		}
	}
	else if (!(getGroup()->hasCargo()))
	{
		if (AI_pillageRange(4))
		{
			return;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/14/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
	if( !(getGroup()->isFull()) )
	{
		if( kOwner.AI_unitTargetMissionAIs(this, MISSIONAI_LOAD_SPECIAL) > 0 )
		{
			// Wait for units on the way
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	}

	if (AI_pickup(UNITAI_MISSIONARY, true))
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/	
	
	if (plot()->waterArea() && kOwner.AI_areaMissionAIs(plot()->waterArea(), MISSIONAI_EXPLORE, getGroup()) < kOwner.AI_neededExplorers(plot()->waterArea())) // K-Mod
	{
		if (AI_explore())
			return;
	}

	if (AI_retreatToCity(true))
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_spySeaMove()
{
	PROFILE_FUNC();
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/21/08	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true))
	{
		if(AI_isThreatenedFromLand()) // advc.139: Code moved into subroutine
		{
			// Retreat to primary area first
			if (AI_retreatToCity(true))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	if (getUnitAICargo(UNITAI_SPY) > 0)
	{
		if (AI_specialSeaTransportSpy())
		{
			return;
		}

		CvCity* pCity = plot()->getPlotCity();

		if (pCity != NULL)
		{
			if (pCity->getOwnerINLINE() == getOwnerINLINE())
			{
				getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY, pCity->plot());
				return;
			}
		}
	}
	else if (!(getGroup()->hasCargo()))
	{
		if (AI_pillageRange(5))
		{
			return;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/14/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
	if( !(getGroup()->isFull()) )
	{
		if( kOwner.AI_unitTargetMissionAIs(this, MISSIONAI_LOAD_SPECIAL) > 0 )
		{
			// Wait for units on the way
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	}

	if (AI_pickup(UNITAI_SPY, true))
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/	

	if (AI_retreatToCity(true))
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_carrierSeaMove()
{
	PROFILE_FUNC();
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/21/08	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true))
	{
		if(AI_isThreatenedFromLand()) // advc.139: Code moved into subroutine
		{
			if (AI_retreatToCity(true))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/
	
	if (AI_heal(50))
	{
		return;
	}

	if (!isEnemy(plot()->getTeam()))
	{
		if (kOwner.AI_unitTargetMissionAIs(this, MISSIONAI_GROUP) > 0)
		{
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	}
	else
	{
		if (AI_seaBombardRange(1))
		{
			return;
		}
	}
	
	if (AI_group(UNITAI_CARRIER_SEA, -1, /*iMaxOwnUnitAI*/ 1))
	{
		return;
	}

	if (getGroup()->countNumUnitAIType(UNITAI_ATTACK_SEA) + getGroup()->countNumUnitAIType(UNITAI_ESCORT_SEA) == 0)
	{
		if (plot()->isCity() && plot()->getOwnerINLINE() == getOwnerINLINE())
		{
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
		if (AI_retreatToCity())
		{
			return;
		}
	}
		
	if (getCargo() > 0)
	{
		if (AI_carrierSeaTransport())
		{
			return;
		}

		if (AI_blockade())
		{
			return;
		}

		if (AI_shadow(UNITAI_ASSAULT_SEA))
		{
			return;
		}
	}
	
	if (AI_travelToUpgradeCity())
	{
		return;
	}

	if (AI_retreatToCity(true))
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_missileCarrierSeaMove()
{
	PROFILE_FUNC();

	bool bStealth = (getInvisibleType() != NO_INVISIBLE);

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						06/14/09	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true))
	{
		if(AI_isThreatenedFromLand()) // advc.139: Code moved into subroutine
		{
			if (AI_shadow(UNITAI_ASSAULT_SEA, 1, 50, false, true, baseMoves()))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/
	
	if (plot()->isCity() && plot()->getTeam() == getTeam())
	{
		if (AI_heal())
		{
			return;
		}
	}
	
	if (((plot()->getTeam() != getTeam()) && getGroup()->hasCargo()) || getGroup()->AI_isFull())
	{
		if (bStealth)
		{
			if (AI_carrierSeaTransport())
			{
				return;
			}
		}
		else
		{
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						06/14/09		jdog5000		*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
			if (AI_shadow(UNITAI_ASSAULT_SEA, 1, 50, true, false, 12))
			{
				return;
			}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/
		
			if (AI_carrierSeaTransport())
			{
				return;
			}
		}
	}
//	if (AI_pickup(UNITAI_ICBM))
//	{
//		return;
//	}
//	
//	if (AI_pickup(UNITAI_MISSILE_AIR))
//	{
//		return;
//	}
	if (AI_retreatToCity())
	{
		return;
	}
	
	getGroup()->pushMission(MISSION_SKIP);
}


void CvUnitAI::AI_attackAirMove()
{
	PROFILE_FUNC();
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/21/08	Solver & jdog5000	*/
/* 																			*/
/* 	Air AI																	*/
/********************************************************************************/
	CvCity* pCity = plot()->getPlotCity();
	//bool bSkiesClear = true;
	//int iDX, iDY;

	// Check for sufficient defenders to stay
	int iDefenders = plot()->plotCount(PUF_canDefend, -1, -1, plot()->getOwner(),
			NO_TEAM, PUF_isDomainType, DOMAIN_LAND); // advc.001s

	int iAttackAirCount = plot()->plotCount(PUF_canAirAttack, -1, -1, NO_PLAYER, getTeam());
	iAttackAirCount += 2 * plot()->plotCount(PUF_isUnitAIType, UNITAI_ICBM, -1, NO_PLAYER, getTeam());

	if( plot()->isCoastalLand(GC.getMIN_WATER_SIZE_FOR_OCEAN()) )
	{
		iDefenders -= 1;
	}

	if( pCity != NULL )
	{
		if( pCity->getDefenseModifier(true) < 40 )
		{
			iDefenders -= 1;
		}

		if( pCity->getOccupationTimer() > 1 )
		{
			iDefenders -= 1;
		}
	}

	if( iAttackAirCount > iDefenders )
	{
		if (AI_airOffensiveCity())
		{
			return;
		}
	}

	// Check for direct threat to current base
	if (plot()->isCity(true))
	{
		// K-Mod
		int iOurDefense = kOwner.AI_localDefenceStrength(plot(), getTeam(), DOMAIN_LAND, 0);
		int iEnemyOffense = kOwner.AI_localAttackStrength(plot(), NO_TEAM, DOMAIN_LAND, 2);
		// K-Mod end

		if (iEnemyOffense > iOurDefense || iOurDefense == 0)
		{
			// Too risky, pull back
			if (AI_airOffensiveCity())
			{
				return;
			}

			if( canAirDefend() )
			{
				if (AI_airDefensiveCity())
				{
					return;
				}
			}
		}
		else if( iEnemyOffense > iOurDefense/3 )
		{
			if( getDamage() == 0 )
			{
				if( collateralDamage() == 0 && canAirDefend() )
				{
					if (pCity != NULL)
					{
						// Check for whether city needs this unit to air defend
						if( !(pCity->AI_isAirDefended(true,-1)) )
						{
							getGroup()->pushMission(MISSION_AIRPATROL);
							return;
						}
					}
				}

				// Attack the invaders!
				if (AI_defendBaseAirStrike())
				{
					return;
				}
				
				/*if (AI_defensiveAirStrike())
				{
					return;
				} */

				if (AI_airStrike())
				{
					return;
				}

				// If no targets, no sense staying in risky place
				if (AI_airOffensiveCity())
				{
					return;
				}

				if( canAirDefend() )
				{
					if (AI_airDefensiveCity())
					{
						return;
					}
				}
			}

			if( healTurns(plot()) > 1 )
			{
				// If very damaged, no sense staying in risky place
				if (AI_airOffensiveCity())
				{
					return;
				}

				if( canAirDefend() )
				{
					if (AI_airDefensiveCity())
					{
						return;
					}
				}
			}
			
		}
	}

	if( getDamage() > 0 )
	{
		//if (((100*currHitPoints()) / maxHitPoints()) < 40)
		{
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
		/* original bts / BBAI code. Disabled by K-Mod because it's time consuming and doesn't help much
		else
		{
			CvPlot *pLoopPlot;
			int iSearchRange = airRange();
			for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
			{
				if (!bSkiesClear) break;
				for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
				{
					pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

					if (pLoopPlot != NULL)
					{
						if (bestInterceptor(pLoopPlot) != NULL)
						{
							bSkiesClear = false;
							break;
						}
					}
				}
			}

			if (!bSkiesClear)
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
		} */
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());
	CvArea* pArea = area();
	int iAttackValue = kPlayer.AI_unitValue(getUnitType(), UNITAI_ATTACK_AIR, pArea);
	int iCarrierValue = kPlayer.AI_unitValue(getUnitType(), UNITAI_CARRIER_AIR, pArea);
	if (iCarrierValue > 0)
	{
		int iCarriers = kPlayer.AI_totalUnitAIs(UNITAI_CARRIER_SEA);
		if (iCarriers > 0)
		{
			UnitTypes eBestCarrierUnit = NO_UNIT;  
			kPlayer.AI_bestAreaUnitAIValue(UNITAI_CARRIER_SEA, NULL, &eBestCarrierUnit);
			if (eBestCarrierUnit != NO_UNIT)
			{
				int iCarrierAirNeeded = iCarriers * GC.getUnitInfo(eBestCarrierUnit).getCargoSpace();
				if (kPlayer.AI_totalUnitAIs(UNITAI_CARRIER_AIR) < iCarrierAirNeeded)
				{
					AI_setUnitAIType(UNITAI_CARRIER_AIR);
					getGroup()->pushMission(MISSION_SKIP);
					return;		
				}
			}
		}
	}
	
	int iDefenseValue = kPlayer.AI_unitValue(getUnitType(), UNITAI_DEFENSE_AIR, pArea);
	if (iDefenseValue > iAttackValue)
	{
		if (kPlayer.AI_bestAreaUnitAIValue(UNITAI_ATTACK_AIR, pArea) > iAttackValue)
		{
			AI_setUnitAIType(UNITAI_DEFENSE_AIR);
			getGroup()->pushMission(MISSION_SKIP);
			return;	
		}
	}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/6/08			jdog5000		*/
/* 																			*/
/* 	Air AI																	*/
/********************************************************************************/
	/* original BTS code
	if (AI_airBombDefenses())
	{
		return;
	}

	if (GC.getGameINLINE().getSorenRandNum(4, "AI Air Attack Move") == 0)
	{
		if (AI_airBombPlots())
		{
			return;
		}
	}

	if (AI_airStrike())
	{
		return;
	}
	
	if (canAirAttack())
	{
		if (AI_airOffensiveCity())
		{
			return;
		}
	}
	
	if (canRecon(plot()))
	{
		if (AI_exploreAir())
		{
			return;
		}
	}

	if (canAirDefend())
	{
		getGroup()->pushMission(MISSION_AIRPATROL);
		return;
	}
	*/
	bool bDefensive = false;
	if( pArea != NULL )
	{
		bDefensive = pArea->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE;
	}

	/* if (GC.getGameINLINE().getSorenRandNum(bDefensive ? 3 : 6, "AI Air Attack Move") == 0)
	{
		if( AI_defensiveAirStrike() )
		{
			return;
		}
	} */ // disabled by K-Mod

	if (GC.getGameINLINE().getSorenRandNum(4, "AI Air Attack Move") == 0)
	{
		// only moves unit in a fort
		if (AI_travelToUpgradeCity())
		{
			return;
		}
	}

	// Support ground attacks
	/* original bts code
	if (AI_airBombDefenses())
	{
		return;
	}

	if (GC.getGameINLINE().getSorenRandNum(bDefensive ? 6 : 4, "AI Air Attack Move") == 0)
	{
		if (AI_airBombPlots())
		{
			return;
		}
	}

	if (AI_airStrike())
	{
		return;
	} */
	// K-Mod
	if (AI_airStrike())
	{
		return;
	}
	// switched probabilities from original bts. If we're on the offense, we don't want to smash up too many improvements...
	// soon they will be _our_ improvements.
	if (GC.getGameINLINE().getSorenRandNum(bDefensive ? 4 : 6, "AI Air Attack Move") == 0)
	{
		if (AI_airBombPlots())
		{
			return;
		}
	}

	if (canAirAttack())
	{
		if (AI_airOffensiveCity())
		{
			return;
		}
	}
	else
	{
		if( canAirDefend() )
		{
			if (AI_airDefensiveCity())
			{
				return;
			}
		}
	}

	// BBAI TODO: Support friendly attacks on common enemies, if low risk?

	if (canAirDefend())
	{
		if( bDefensive || GC.getGameINLINE().getSorenRandNum(2, "AI Air Attack Move") == 0 )
		{
			getGroup()->pushMission(MISSION_AIRPATROL);
			return;
		}
	}
	
	if (canRecon(plot()))
	{
		if (AI_exploreAir())
		{
			return;
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/
	

	getGroup()->pushMission(MISSION_SKIP);
	return;
}

// This function has been rewritten for K-Mod. (The new version is much simplier.)
void CvUnitAI::AI_defenseAirMove()
{
	PROFILE_FUNC();

	if (!plot()->isCity(true))
	{
		//FAssertMsg(GC.getGame().getGameTurn() - getGameTurnCreated() > 1, "defenseAir units are expected to stay in cities/forts");
		if (AI_airDefensiveCity())
			return;
	}

	CvCity* pCity = plot()->getPlotCity();

	int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_localAttackStrength(plot(), NO_TEAM);
	int iOurDefense = GET_PLAYER(getOwnerINLINE()).AI_localDefenceStrength(plot(), getTeam());

	if (iEnemyOffense > 2*iOurDefense || iOurDefense == 0)
	{
		// Too risky, pull out
		if (AI_airDefensiveCity())
		{
			return;
		}
	}

	int iDefNeeded = pCity ? pCity->AI_neededAirDefenders() : 0;
	int iDefHere = plot()->plotCount(PUF_isAirIntercept, -1, -1, NO_PLAYER, getTeam()) - (PUF_isAirIntercept(this, -1, -1) ? 1 : 0);
	FAssert(iDefHere >= 0);

	if (canAirDefend() && iEnemyOffense < iOurDefense && iDefHere < iDefNeeded/2)
	{
		getGroup()->pushMission(MISSION_AIRPATROL);
		return;
	}

	if (iEnemyOffense > (getDamage() == 0 ? iOurDefense/3 : iOurDefense))
	{
		// Attack the invaders!
		if (AI_defendBaseAirStrike())
		{
			return;
		}
	}

	if (getDamage() > maxHitPoints()/3)
	{
		getGroup()->pushMission(MISSION_SKIP);
		return;
	}

	if (iEnemyOffense == 0 && GC.getGameINLINE().getSorenRandNum(4, "AI Air Defense Move") == 0)
	{
		if (AI_travelToUpgradeCity())
		{
			return;
		}
	}

	bool bDefensive = area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE;
	bool bOffensive = area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE;

	bool bTriedAirStrike = false;
	if (GC.getGameINLINE().getSorenRandNum(3, "AI_defenseAirMove airstrike") <= (bOffensive ? 1 : 0) - (bDefensive ? 1 : 0))
	{
		if (AI_airStrike())
			return;
		bTriedAirStrike = true;
	}

	if (canAirDefend() && iDefHere < iDefNeeded*2/3)
	{
		getGroup()->pushMission(MISSION_AIRPATROL);
		return;
	}

	if (!bTriedAirStrike && GC.getGameINLINE().getSorenRandNum(3, "AI_defenseAirMove airstrike2") <= (bOffensive ? 1 : 0) - (bDefensive ? 1 : 0))
	{
		if (AI_airStrike())
			return;
		bTriedAirStrike = true;
	}

	// <advc.651>
	if(GET_PLAYER(getOwner()).AI_isDangerFromSubmarines() && plot()->isCoastalLand() &&
			::bernoulliSuccess(0.38, "advc.651")) {
		/* Would be better to check for matching Invisible Types (modded aircraft
		   may not be able to see invisible units). Also, isCoastalLand is a bit
		   narrow -- can often scout the seas from landlocked cities. */
		if(AI_exploreAir2())
			return;
	} // </advc.651>

	if (AI_airDefensiveCity()) // check if there's a better city to be in
	{
		return;
	}

	if (canAirDefend() && iDefHere < iDefNeeded)
	{
		getGroup()->pushMission(MISSION_AIRPATROL);
		return;
	}

	if (canRecon(plot()))
	{
		if (GC.getGame().getSorenRandNum(bDefensive ? 6 : 3, "AI defensive air recon") == 0)
		{
			if (AI_exploreAir())
			{
				return;
			}
		}
	}

	if (canAirDefend())
	{
		getGroup()->pushMission(MISSION_AIRPATROL);
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_carrierAirMove()
{
	PROFILE_FUNC();

	// XXX maybe protect land troops?

	if (getDamage() > 0)
	{
		getGroup()->pushMission(MISSION_SKIP);
		return;
	}

	if (isCargo())
	{
		/* original bts code
		int iRand = GC.getGameINLINE().getSorenRandNum(3, "AI Air Carrier Move");

		if (iRand == 2 && canAirDefend())
		{
			getGroup()->pushMission(MISSION_AIRPATROL);
			return;
		}
		else if (AI_airBombDefenses())
		{
			return;
		}
		else if (iRand == 1)
		{
			if (AI_airBombPlots())
			{
				return;
			}
			
			if (AI_airStrike())
			{
				return;
			}
		}
		else
		{
			if (AI_airStrike())
			{
				return;
			}
			
			if (AI_airBombPlots())
			{
				return;
			}
		} */
		// K-Mod
		if (canAirDefend())
		{
			int iActiveInterceptors = plot()->plotCount(PUF_isAirIntercept, -1, -1, getOwnerINLINE());
			if (GC.getGameINLINE().getSorenRandNum(16, "AI Air Carrier Move") < 4 - std::min(3, iActiveInterceptors))
			{
				getGroup()->pushMission(MISSION_AIRPATROL);
				return;
			}
		}
		if (AI_airStrike())
		{
			return;
		}
		if (AI_airBombPlots())
		{
			return;
		}
		// K-Mod end

		if (AI_travelToUpgradeCity())
		{
			return;
		}

		if (canAirDefend())
		{
			getGroup()->pushMission(MISSION_AIRPATROL);
			return;
		}
		getGroup()->pushMission(MISSION_SKIP);
		return;
	}

	if (AI_airCarrier())
	{
		return;
	}

	if (AI_airDefensiveCity())
	{
		return;
	}

	if (canAirDefend())
	{
		getGroup()->pushMission(MISSION_AIRPATROL);
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_missileAirMove()
{
	PROFILE_FUNC();

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/21/08	Solver & jdog5000	*/
/* 																			*/
/* 	Air AI																	*/
/********************************************************************************/
	// includes forts
	if (!isCargo() && plot()->isCity(true))
	{
		// K-Mod
		int iOurDefense = kOwner.AI_localDefenceStrength(plot(), getTeam(), DOMAIN_LAND, 0);
		int iEnemyOffense = kOwner.AI_localAttackStrength(plot(), NO_TEAM, DOMAIN_LAND, 2);
		// K-Mod end
		
		if (iEnemyOffense > (iOurDefense/2) || iOurDefense == 0)
		{
			if (AI_airOffensiveCity())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/
	
	if (isCargo())
	{
		int iRand = GC.getGameINLINE().getSorenRandNum(3, "AI Air Missile plot bombing");
		if (iRand != 0)
		{
			if (AI_airBombPlots())
			{
				return;
			}
		}

		/* original bts code
		iRand = GC.getGameINLINE().getSorenRandNum(3, "AI Air Missile Carrier Move");
		if (iRand == 0)
		{
			if (AI_airBombDefenses())
			{
				return;
			}
			
			if (AI_airStrike())
			{
				return;
			}
		}
		else
		{	
			if (AI_airStrike())
			{
				return;
			}
			
			if (AI_airBombDefenses())
			{
				return;
			}
		} */
		// K-Mod
		if (AI_airStrike())
		{
			return;
		}
		// K-Mod end
		
		if (AI_airBombPlots())
		{
			return;
		}
		
		getGroup()->pushMission(MISSION_SKIP);
		return;
	}
	
	if (AI_airStrike())
	{
		return;
	}
	
	if (AI_missileLoad(UNITAI_MISSILE_CARRIER_SEA))
	{
		return;
	}
	
	if (AI_missileLoad(UNITAI_RESERVE_SEA, 1))
	{
		return;
	}
	
	if (AI_missileLoad(UNITAI_ATTACK_SEA, 1))
	{
		return;
	}

	/* if (AI_airBombDefenses())
	{
		return;
	} */ // disabled by K-Mod

	if (!isCargo())
	{
		if (AI_airOffensiveCity())
		{
			return;
		}
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_networkAutomated()
{
	FAssertMsg(canBuildRoute(), "canBuildRoute is expected to be true");

	if (!getGroup()->canDefend())
	{
/***************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09           jdog5000      */
/* Unit AI, Efficiency                                                     */
/***************************************************************************/
		//if (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot()) > 0)
		if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot()))
/****************************************************************************/
/* BETTER_BTS_AI_MOD                       END                              */
/****************************************************************************/
		{
			if (AI_retreatToCity()) // XXX maybe not do this??? could be working productively somewhere else...
			{
				return;
			}
		}
	}

	/* if (AI_improveBonus(0,20))
		return;
	if (AI_improveBonus(o,10))
		return;*/
	// K-Mod
	if (AI_improveBonus())
		return;
	// K-Mod end. (I don't think AI_connectBonus() is useful either, but I haven't looked closely enough to remove it.)

	if (AI_connectBonus())
	{
		return;
	}

	if (AI_connectCity())
	{
		return;
	}

	/* if (AI_improveBonus())
		return; */ // disabled by K-Mod

	if (AI_routeTerritory(true))
	{
		return;
	}

	if (AI_connectBonus(false))
	{
		return;
	}

	if (AI_routeCity())
	{
		return;
	}

	if (AI_routeTerritory())
	{
		return;
	}
	
	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_cityAutomated()
{
	CvCity* pCity;

	if (!(getGroup()->canDefend()))
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
		//if (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot()) > 0)
		if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot()))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		{
			if (AI_retreatToCity()) // XXX maybe not do this??? could be working productively somewhere else...
			{
				return;
			}
		}
	}

	pCity = NULL;

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		pCity = plot()->getWorkingCity();
	}

	if (pCity == NULL)
	{
		pCity = GC.getMapINLINE().findCity(getX_INLINE(), getY_INLINE(), getOwnerINLINE()); // XXX do team???
	}

	if (pCity != NULL)
	{
		if (AI_improveCity(pCity))
		{
			return;
		}
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


// XXX make sure we include any new UnitAITypes...
int CvUnitAI::AI_promotionValue(PromotionTypes ePromotion)
{
	int iValue;
	int iTemp;
	int iExtra;
	//int iI;

	iValue = 0;

	if (GC.getPromotionInfo(ePromotion).isLeader())
	{
		// Don't consume the leader as a regular promotion
		return 0;
	}

	//if (GC.getPromotionInfo(ePromotion).isBlitz())
	// <advc.164>
	int iBlitz = GC.getPromotionInfo(ePromotion).getBlitz();
	if(iBlitz != 0) {
		if(iBlitz < 0)
			iBlitz = 3;
		int iExtraAttacks = std::min(iBlitz, baseMoves() - 1 +
				(getDropRange() > 0 ? 1 : 0));
		if(iExtraAttacks > 0) { // </advc.164>
			if ((AI_getUnitAIType() == UNITAI_RESERVE  && baseMoves() > 1) || 
				AI_getUnitAIType() == UNITAI_PARADROP)
			{
				//iValue += 10;
				iValue += 8 * iExtraAttacks; // advc.164
			}
			else
			{
				//iValue += 2;
				iValue += iExtraAttacks; // advc.164
			}
		}
	}

	if (GC.getPromotionInfo(ePromotion).isAmphib())
	{
		if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
			  (AI_getUnitAIType() == UNITAI_ATTACK_CITY))
		{
			iValue += 5;
		}
		else
		{
			iValue++;
		}
	}

	if (GC.getPromotionInfo(ePromotion).isRiver())
	{
		if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
			  (AI_getUnitAIType() == UNITAI_ATTACK_CITY))
		{
			iValue += 5;
		}
		else
		{
			iValue++;
		}
	}

	if (GC.getPromotionInfo(ePromotion).isEnemyRoute())
	{
		if (AI_getUnitAIType() == UNITAI_PILLAGE)
		{
			iValue += 40;
		}
		else if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
			       (AI_getUnitAIType() == UNITAI_ATTACK_CITY))
		{
			iValue += 20;
		}
		else if (AI_getUnitAIType() == UNITAI_PARADROP)
		{
			iValue += 10;
		}
		else
		{
			iValue += 4;
		}
	}

	if (GC.getPromotionInfo(ePromotion).isAlwaysHeal())
	{
		if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
			  (AI_getUnitAIType() == UNITAI_ATTACK_CITY) ||
				(AI_getUnitAIType() == UNITAI_PILLAGE) ||
				(AI_getUnitAIType() == UNITAI_COUNTER) ||
				(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
				(AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
				(AI_getUnitAIType() == UNITAI_ESCORT_SEA) ||
				(AI_getUnitAIType() == UNITAI_PARADROP))
		{
			iValue += 10;
		}
		else
		{
			iValue += 8;
		}
	}

	if (GC.getPromotionInfo(ePromotion).isHillsDoubleMove())
	{
		if (AI_getUnitAIType() == UNITAI_EXPLORE)
		{
			iValue += 20;
		}
		else
		{
			iValue += 10;
		}
	}

	if (GC.getPromotionInfo(ePromotion).isImmuneToFirstStrikes() 
		&& !immuneToFirstStrikes())
	{
		if ((AI_getUnitAIType() == UNITAI_ATTACK_CITY))
		{
			iValue += 12;			
		}
		else if ((AI_getUnitAIType() == UNITAI_ATTACK))
		{
			iValue += 8;
		}
		else
		{
			iValue += 4;
		}
	}

	iTemp = GC.getPromotionInfo(ePromotion).getVisibilityChange();
	if ((AI_getUnitAIType() == UNITAI_EXPLORE_SEA) || 
		(AI_getUnitAIType() == UNITAI_EXPLORE))
	{
		iValue += (iTemp * 40);
	}
	else if (AI_getUnitAIType() == UNITAI_PIRATE_SEA) 
	{
		iValue += (iTemp * 20);
	}

	iTemp = GC.getPromotionInfo(ePromotion).getMovesChange();
	if ((AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
		(AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
		  (AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
		  (AI_getUnitAIType() == UNITAI_ESCORT_SEA) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
			(AI_getUnitAIType() == UNITAI_ASSAULT_SEA) ||
			(AI_getUnitAIType() == UNITAI_SETTLER_SEA) ||
			(AI_getUnitAIType() == UNITAI_PILLAGE) ||
			(AI_getUnitAIType() == UNITAI_ATTACK) ||
			(AI_getUnitAIType() == UNITAI_PARADROP))
	{
		iValue += (iTemp * 20);
	}
	else
	{
		iValue += (iTemp * 4);
	}

	iTemp = GC.getPromotionInfo(ePromotion).getMoveDiscountChange();
	if (AI_getUnitAIType() == UNITAI_PILLAGE)
	{
		iValue += (iTemp * 10);
	}
	else
	{
		iValue += (iTemp * 2);
	}

	iTemp = GC.getPromotionInfo(ePromotion).getAirRangeChange();
	if (AI_getUnitAIType() == UNITAI_ATTACK_AIR ||
		AI_getUnitAIType() == UNITAI_CARRIER_AIR)
	{
		iValue += (iTemp * 20);
	}
	else if (AI_getUnitAIType() == UNITAI_DEFENSE_AIR)
	{
		iValue += (iTemp * 10);
	}

	iTemp = GC.getPromotionInfo(ePromotion).getInterceptChange();
	if (AI_getUnitAIType() == UNITAI_DEFENSE_AIR)
	{
		iValue += (iTemp * 3);
	}
	else if (AI_getUnitAIType() == UNITAI_CITY_SPECIAL || AI_getUnitAIType() == UNITAI_CARRIER_AIR)
	{
		iValue += (iTemp * 2);
	}
	else
	{
		iValue += (iTemp / 10);
	}

	iTemp = GC.getPromotionInfo(ePromotion).getEvasionChange();
	if (AI_getUnitAIType() == UNITAI_ATTACK_AIR || AI_getUnitAIType() == UNITAI_CARRIER_AIR)
	{
		iValue += (iTemp * 3);
	}
	else
	{
		iValue += (iTemp / 10);
	}

	iTemp = GC.getPromotionInfo(ePromotion).getFirstStrikesChange() * 2;
	iTemp += GC.getPromotionInfo(ePromotion).getChanceFirstStrikesChange();
	if ((AI_getUnitAIType() == UNITAI_RESERVE) ||
		  (AI_getUnitAIType() == UNITAI_COUNTER) ||
			(AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
			(AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
			(AI_getUnitAIType() == UNITAI_CITY_SPECIAL) ||
			(AI_getUnitAIType() == UNITAI_ATTACK))
	{
		iTemp *= 8;
		iExtra = getExtraChanceFirstStrikes() + getExtraFirstStrikes() * 2;
		iTemp *= 100 + iExtra * 15;
		iTemp /= 100;
		iValue += iTemp;
	}
	else
	{
		iValue += (iTemp * 5);
	}


	iTemp = GC.getPromotionInfo(ePromotion).getWithdrawalChange();
	if (iTemp != 0)
	{
		iExtra = (m_pUnitInfo->getWithdrawalProbability() + (getExtraWithdrawal() * 4));
		iTemp *= (100 + iExtra);
		iTemp /= 100;
		if ((AI_getUnitAIType() == UNITAI_ATTACK_CITY))
		{
			iValue += (iTemp * 4) / 3;
		}
		else if ((AI_getUnitAIType() == UNITAI_COLLATERAL) ||
			  (AI_getUnitAIType() == UNITAI_RESERVE) ||
			  (AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
			  getLeaderUnitType() != NO_UNIT)
		{
			iValue += iTemp * 1;
		}
		else
		{
			iValue += (iTemp / 4);
		}
	}

	iTemp = GC.getPromotionInfo(ePromotion).getCollateralDamageChange();
	if (iTemp != 0)
	{
		iExtra = (getExtraCollateralDamage());//collateral has no strong synergy (not like retreat)
		iTemp *= (100 + iExtra);
		iTemp /= 100;
		
		if (AI_getUnitAIType() == UNITAI_COLLATERAL)
		{
			iValue += (iTemp * 1);
		}
		else if (AI_getUnitAIType() == UNITAI_ATTACK_CITY)
		{
			iValue += ((iTemp * 2) / 3);
		}
		else
		{
			iValue += (iTemp / 8);
		}
	}

	iTemp = GC.getPromotionInfo(ePromotion).getBombardRateChange();
	if (AI_getUnitAIType() == UNITAI_ATTACK_CITY)
	{
		iValue += (iTemp * 2);
	}
	else
	{
		iValue += (iTemp / 8);
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/26/10                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	iTemp = GC.getPromotionInfo(ePromotion).getEnemyHealChange();	
	if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
		(AI_getUnitAIType() == UNITAI_PILLAGE) ||
		(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
		(AI_getUnitAIType() == UNITAI_PARADROP) ||
		(AI_getUnitAIType() == UNITAI_PIRATE_SEA))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	{
		iValue += (iTemp / 4);
	}
	else
	{
		iValue += (iTemp / 8);
	}

	iTemp = GC.getPromotionInfo(ePromotion).getNeutralHealChange();
	iValue += (iTemp / 8);

	iTemp = GC.getPromotionInfo(ePromotion).getFriendlyHealChange();
	if ((AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
		  (AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
		  (AI_getUnitAIType() == UNITAI_CITY_SPECIAL))
	{
		iValue += (iTemp / 4);
	}
	else
	{
		iValue += (iTemp / 8);
	}

	// BBAI / K-Mod
    if (getDamage() > 0 || ((AI_getBirthmark() % 8 == 0) && (AI_getUnitAIType() == UNITAI_COUNTER || 
															AI_getUnitAIType() == UNITAI_PILLAGE ||
															AI_getUnitAIType() == UNITAI_ATTACK_CITY ||
															AI_getUnitAIType() == UNITAI_RESERVE ||
															AI_getUnitAIType() == UNITAI_PIRATE_SEA ||
															AI_getUnitAIType() == UNITAI_RESERVE_SEA ||
															AI_getUnitAIType() == UNITAI_ASSAULT_SEA)) )
    {
	// BBAI / K-Mod
        iTemp = GC.getPromotionInfo(ePromotion).getSameTileHealChange() + getSameTileHeal();
        iExtra = getSameTileHeal();
        
        iTemp *= (100 + iExtra * 5);
        iTemp /= 100;
        
        if (iTemp > 0)
        {
            if (healRate(plot(), false, true) < iTemp)
            {
                iValue += iTemp * ((getGroup()->getNumUnits() > 4) ? 4 : 2);
            }
            else
            {
                iValue += (iTemp / 8);
            }
        }

        iTemp = GC.getPromotionInfo(ePromotion).getAdjacentTileHealChange();
        iExtra = getAdjacentTileHeal();
        iTemp *= (100 + iExtra * 5);
        iTemp /= 100;
        if (getSameTileHeal() >= iTemp)
        {
            iValue += (iTemp * ((getGroup()->getNumUnits() > 9) ? 4 : 2));
        }
        else
        {
            iValue += (iTemp / 4);
        }
    }

	// try to use Warlords to create super-medic units
	if (GC.getPromotionInfo(ePromotion).getAdjacentTileHealChange() > 0 || GC.getPromotionInfo(ePromotion).getSameTileHealChange() > 0)
	{
		/* original bts code PromotionTypes eLeader = NO_PROMOTION;
		for (iI = 0; iI < GC.getNumPromotionInfos(); iI++)
		{
			if (GC.getPromotionInfo((PromotionTypes)iI).isLeader())
			{
				eLeader = (PromotionTypes)iI;
			}
		}
		
		if (isHasPromotion(eLeader) && eLeader != NO_PROMOTION)
		{
			iValue += GC.getPromotionInfo(ePromotion).getAdjacentTileHealChange() + GC.getPromotionInfo(ePromotion).getSameTileHealChange();
		} */
		// K-Mod, I've changed the way we work out if we are a leader or not.
		// The original method would break if there was more than one "leader" promotion)
		for (int iI = 0; iI < GC.getNumPromotionInfos(); iI++)
		{
			if (GC.getPromotionInfo((PromotionTypes)iI).isLeader() && isHasPromotion((PromotionTypes)iI))
			{
				iValue += GC.getPromotionInfo(ePromotion).getAdjacentTileHealChange() + GC.getPromotionInfo(ePromotion).getSameTileHealChange();
				break;
			}
		}
		// K-Mod end
	}

	iTemp = GC.getPromotionInfo(ePromotion).getCombatPercent();
	// kmodx: Removed redundant clauses
	if (AI_getUnitAIType() == UNITAI_ATTACK ||
		AI_getUnitAIType() == UNITAI_COUNTER ||
		AI_getUnitAIType() == UNITAI_CITY_COUNTER ||
		AI_getUnitAIType() == UNITAI_ATTACK_SEA ||
		AI_getUnitAIType() == UNITAI_PARADROP ||
	    AI_getUnitAIType() == UNITAI_PIRATE_SEA ||
		AI_getUnitAIType() == UNITAI_RESERVE_SEA ||
		AI_getUnitAIType() == UNITAI_ESCORT_SEA ||
		AI_getUnitAIType() == UNITAI_CARRIER_SEA ||
		AI_getUnitAIType() == UNITAI_ATTACK_AIR ||
		AI_getUnitAIType() == UNITAI_CARRIER_AIR)
	{
		iValue += (iTemp * 2);
	}
	else
	{
		iValue += (iTemp * 1);
	}

	iTemp = GC.getPromotionInfo(ePromotion).getCityAttackPercent();
	if (iTemp != 0)
	{
		if (m_pUnitInfo->getUnitAIType(UNITAI_ATTACK) || m_pUnitInfo->getUnitAIType(UNITAI_ATTACK_CITY) || m_pUnitInfo->getUnitAIType(UNITAI_ATTACK_CITY_LEMMING))
		{
			iExtra = (m_pUnitInfo->getCityAttackModifier() + (getExtraCityAttackPercent() * 2));
			iTemp *= (100 + iExtra);
			iTemp /= 100;
			if (AI_getUnitAIType() == UNITAI_ATTACK_CITY)
			{
				iValue += (iTemp * 1);
			}
			else 
			{
				iValue -= iTemp / 4;
			}
		}
	}

	iTemp = GC.getPromotionInfo(ePromotion).getCityDefensePercent();
	if (iTemp != 0)
	{
		if ((AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
			  (AI_getUnitAIType() == UNITAI_CITY_SPECIAL))
		{
			iExtra = m_pUnitInfo->getCityDefenseModifier() + (getExtraCityDefensePercent() * 2);
			iValue += ((iTemp * (100 + iExtra)) / 100);
		}
		else
		{
			iValue += (iTemp / 4);
		}
	}

	iTemp = GC.getPromotionInfo(ePromotion).getHillsAttackPercent();
	if (iTemp != 0)
	{
		iExtra = getExtraHillsAttackPercent();
		iTemp *= (100 + iExtra * 2);
		iTemp /= 100;
		if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
			(AI_getUnitAIType() == UNITAI_COUNTER))
		{
			iValue += (iTemp / 4);
		}
		else
		{
			iValue += (iTemp / 16);
		}
	}

	iTemp = GC.getPromotionInfo(ePromotion).getHillsDefensePercent();
	if (iTemp != 0)
	{
		iExtra = (m_pUnitInfo->getHillsDefenseModifier() + (getExtraHillsDefensePercent() * 2)); 
		iTemp *= (100 + iExtra);
		iTemp /= 100;
		if (AI_getUnitAIType() == UNITAI_CITY_DEFENSE)
		{
			if (plot()->isCity() && plot()->isHills())
			{
				iValue += (iTemp * 4) / 3;
			}
		}
		else if (AI_getUnitAIType() == UNITAI_COUNTER)
		{
			if (plot()->isHills())
			{
				iValue += (iTemp / 4);
			}
			else
			{
				iValue++;
			}
		}
		else
		{
			iValue += (iTemp / 16);
		}
	}
	// advc.099e: Commented out
	/*iTemp = GC.getPromotionInfo(ePromotion).getRevoltProtection();
	if ((AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
		(AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
		(AI_getUnitAIType() == UNITAI_CITY_SPECIAL))
	{
		if (iTemp > 0)
		{
			PlayerTypes eOwner = plot()->calculateCulturalOwner();
			if (eOwner != NO_PLAYER && GET_PLAYER(eOwner).getTeam() != GET_PLAYER(getOwnerINLINE()).getTeam())
			{
				iValue += (iTemp / 2);
			}
		}
	}*/

	iTemp = GC.getPromotionInfo(ePromotion).getCollateralDamageProtection();
	if ((AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
		(AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
		(AI_getUnitAIType() == UNITAI_CITY_SPECIAL))
	{
		iValue += (iTemp / 3);
	}
	else if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
		(AI_getUnitAIType() == UNITAI_COUNTER))
	{
		iValue += (iTemp / 4);
	}
	else
	{
		iValue += (iTemp / 8);
	}

	iTemp = GC.getPromotionInfo(ePromotion).getPillageChange();
	if (AI_getUnitAIType() == UNITAI_PILLAGE ||
		AI_getUnitAIType() == UNITAI_ATTACK_SEA ||
		AI_getUnitAIType() == UNITAI_PIRATE_SEA)
	{
		iValue += (iTemp / 4);
	}
	else
	{
		iValue += (iTemp / 16);
	}

	iTemp = GC.getPromotionInfo(ePromotion).getUpgradeDiscount();
	iValue += (iTemp / 16);

	iTemp = GC.getPromotionInfo(ePromotion).getExperiencePercent();
	if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
		(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
		(AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
		(AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
		(AI_getUnitAIType() == UNITAI_ESCORT_SEA) ||
		(AI_getUnitAIType() == UNITAI_CARRIER_SEA) ||
		(AI_getUnitAIType() == UNITAI_MISSILE_CARRIER_SEA))
	{
		iValue += (iTemp * 1);
	}
	else
	{
		iValue += (iTemp / 2);
	}

	iTemp = GC.getPromotionInfo(ePromotion).getKamikazePercent();
	if (AI_getUnitAIType() == UNITAI_ATTACK_CITY)
	{
		iValue += (iTemp / 16);
	}
	else
	{
		iValue += (iTemp / 64);
	}

	for (int iI = 0; iI < GC.getNumTerrainInfos(); iI++)
	{
		iTemp = GC.getPromotionInfo(ePromotion).getTerrainAttackPercent(iI);
		if (iTemp != 0)
		{
			iExtra = getExtraTerrainAttackPercent((TerrainTypes)iI);
			iTemp *= (100 + iExtra * 2);
			iTemp /= 100;
			if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
				(AI_getUnitAIType() == UNITAI_COUNTER))
			{
				iValue += (iTemp / 4);
			}
			else
			{
				iValue += (iTemp / 16);
			}
		}

		iTemp = GC.getPromotionInfo(ePromotion).getTerrainDefensePercent(iI);
		if (iTemp != 0)
		{
			iExtra =  getExtraTerrainDefensePercent((TerrainTypes)iI);
			iTemp *= (100 + iExtra);
			iTemp /= 100;
			if (AI_getUnitAIType() == UNITAI_COUNTER)
			{
				if (plot()->getTerrainType() == (TerrainTypes)iI)
				{
					iValue += (iTemp / 4);
				}
				else
				{
					iValue++;
				}
			}
			else
			{
				iValue += (iTemp / 16);
			}
		}

		if (GC.getPromotionInfo(ePromotion).getTerrainDoubleMove(iI))
		{
			if (AI_getUnitAIType() == UNITAI_EXPLORE)
			{
				iValue += 20;
			}
			else if ((AI_getUnitAIType() == UNITAI_ATTACK) || (AI_getUnitAIType() == UNITAI_PILLAGE))
			{
				iValue += 10;
			}
			else
			{
			    iValue += 1;
			}
		}
	}

	for (int iI = 0; iI < GC.getNumFeatureInfos(); iI++)
	{
		iTemp = GC.getPromotionInfo(ePromotion).getFeatureAttackPercent(iI);
		if (iTemp != 0)
		{
			iExtra = getExtraFeatureAttackPercent((FeatureTypes)iI);
			iTemp *= (100 + iExtra * 2);
			iTemp /= 100;
			if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
				(AI_getUnitAIType() == UNITAI_COUNTER))
			{
				iValue += (iTemp / 4);
			}
			else
			{
				iValue += (iTemp / 16);
			}
		}

		iTemp = GC.getPromotionInfo(ePromotion).getFeatureDefensePercent(iI);
		if (iTemp != 0)
		{
			iExtra = getExtraFeatureDefensePercent((FeatureTypes)iI);
			iTemp *= (100 + iExtra * 2);
			iTemp /= 100;
			
			if (!noDefensiveBonus())
			{
				if (AI_getUnitAIType() == UNITAI_COUNTER)
				{
					if (plot()->getFeatureType() == (FeatureTypes)iI)
					{
						iValue += (iTemp / 4);
					}
					else
					{
						iValue++;
					}
				}
				else
				{
					iValue += (iTemp / 16);
				}
			}
		}

		if (GC.getPromotionInfo(ePromotion).getFeatureDoubleMove(iI))
		{
			if (AI_getUnitAIType() == UNITAI_EXPLORE)
			{
				iValue += 20;
			}
			else if ((AI_getUnitAIType() == UNITAI_ATTACK) || (AI_getUnitAIType() == UNITAI_PILLAGE))
			{
				iValue += 10;
			}
			else
			{
			    iValue += 1;
			}
		}
	}

    int iOtherCombat = 0; 
    int iSameCombat = 0;
    
    for (int iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
    {
        if ((UnitCombatTypes)iI == getUnitCombatType())
        {
            iSameCombat += unitCombatModifier((UnitCombatTypes)iI);
        }
        else
        {
            iOtherCombat += unitCombatModifier((UnitCombatTypes)iI);
        }
    }

	for (int iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
	{
		iTemp = GC.getPromotionInfo(ePromotion).getUnitCombatModifierPercent(iI);
		int iCombatWeight = 0;
        //Fighting their own kind
        if ((UnitCombatTypes)iI == getUnitCombatType())
        {
            if (iSameCombat >= iOtherCombat)
            {
                iCombatWeight = 70;//"axeman takes formation"
            }
            else
            {
                iCombatWeight = 30;
            }
        }
        else
        {
            //fighting other kinds
            if (unitCombatModifier((UnitCombatTypes)iI) > 10)
            {
                iCombatWeight = 70;//"spearman takes formation"
            }
            else
            {
                iCombatWeight = 30;
            }
        }

		iCombatWeight *= GET_PLAYER(getOwnerINLINE()).AI_getUnitCombatWeight((UnitCombatTypes)iI);
		iCombatWeight /= 100;		
		
		if ((AI_getUnitAIType() == UNITAI_COUNTER) || (AI_getUnitAIType() == UNITAI_CITY_COUNTER))
		{
		    iValue += (iTemp * iCombatWeight) / 50;
		}
		else if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
			       (AI_getUnitAIType() == UNITAI_RESERVE))
		{
			iValue += (iTemp * iCombatWeight) / 100;
		}
		else
		{
			iValue += (iTemp * iCombatWeight) / 200;
		}
	}

	for (int iI = 0; iI < NUM_DOMAIN_TYPES; iI++)
	{
		//WTF? why float and cast to int?
		//iTemp = ((int)((GC.getPromotionInfo(ePromotion).getDomainModifierPercent(iI) + getExtraDomainModifier((DomainTypes)iI)) * 100.0f));
		iTemp = GC.getPromotionInfo(ePromotion).getDomainModifierPercent(iI);
		if (AI_getUnitAIType() == UNITAI_COUNTER)
		{
			iValue += (iTemp * 1);
		}
		else if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
			       (AI_getUnitAIType() == UNITAI_RESERVE))
		{
			iValue += (iTemp / 2);
		}
		else
		{
			iValue += (iTemp / 8);
		}
	}

	if (iValue > 0)
	{
		iValue += GC.getGameINLINE().getSorenRandNum(15, "AI Promote");
	}

	return iValue;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_shadow(UnitAITypes eUnitAI, int iMax, int iMaxRatio, bool bWithCargoOnly, bool bOutsideCityOnly, int iMaxPath)
{
	PROFILE_FUNC();

	CvUnit* pLoopUnit;
	CvUnit* pBestUnit;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iLoop;

	iBestValue = 0;
	pBestUnit = NULL;

	for(pLoopUnit = GET_PLAYER(getOwnerINLINE()).firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = GET_PLAYER(getOwnerINLINE()).nextUnit(&iLoop))
	{
		if (pLoopUnit != this)
		{
			if (AI_plotValid(pLoopUnit->plot()))
			{
				if (pLoopUnit->isGroupHead())
				{
					if (!(pLoopUnit->isCargo()))
					{
						if (pLoopUnit->AI_getUnitAIType() == eUnitAI)
						{
							if (pLoopUnit->getGroup()->baseMoves() <= getGroup()->baseMoves())
							{
								if (!bWithCargoOnly || pLoopUnit->getGroup()->hasCargo())
								{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/08/08                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
									if( bOutsideCityOnly && pLoopUnit->plot()->isCity() )
									{
										continue;
									}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

									int iShadowerCount = GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(pLoopUnit, MISSIONAI_SHADOW, getGroup());
									if (((-1 == iMax) || (iShadowerCount < iMax)) &&
										 ((-1 == iMaxRatio) || (iShadowerCount == 0) || (((100 * iShadowerCount) / std::max(1, pLoopUnit->getGroup()->countNumUnitAIType(eUnitAI))) <= iMaxRatio)))
									{
										if (!(pLoopUnit->plot()->isVisibleEnemyUnit(this)))
										{
											if (generatePath(pLoopUnit->plot(), 0, true, &iPathTurns, iMaxPath))
											{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/08/08                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
/* original bts code
												//if (iPathTurns <= iMaxPath) XXX
*/
												if (iPathTurns <= iMaxPath)
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
												{
													iValue = 1 + pLoopUnit->getGroup()->getCargo();
													iValue *= 1000;
													iValue /= 1 + iPathTurns;

													if (iValue > iBestValue)
													{
														iBestValue = iValue;
														pBestUnit = pLoopUnit;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestUnit != NULL)
	{
		if (atPlot(pBestUnit->plot()))
		{
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_SHADOW, NULL, pBestUnit);
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO_UNIT, pBestUnit->getOwnerINLINE(), pBestUnit->getID(), 0, false, false, MISSIONAI_SHADOW, NULL, pBestUnit);
			return true;
		}
	}

	return false;
}

// K-Mod. One group function to rule them all.
bool CvUnitAI::AI_omniGroup(UnitAITypes eUnitAI, int iMaxGroup, int iMaxOwnUnitAI, bool bStackOfDoom, int iFlags, int iMaxPath, bool bMergeGroups, bool bSafeOnly, bool bIgnoreFaster, bool bIgnoreOwnUnitType, bool bBiggerOnly, int iMinUnitAI, bool bWithCargoOnly, bool bIgnoreBusyTransports)
{
	PROFILE_FUNC();

	iFlags &= ~MOVE_DECLARE_WAR; // Don't consider war when we just want to group

	if (isCargo())
		return false;

	if (!AI_canGroupWithAIType(eUnitAI))
		return false;

	if (getDomainType() == DOMAIN_LAND && !canMoveAllTerrain())
	{
		if (area()->getNumAIUnits(getOwnerINLINE(), eUnitAI) == 0)
		{
			return false;
		}
	}

	int iOurImpassableCount = 0;
	CLLNode<IDInfo>* pUnitNode = getGroup()->headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pImpassUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = getGroup()->nextUnitNode(pUnitNode);

		iOurImpassableCount = std::max(iOurImpassableCount, GET_PLAYER(getOwnerINLINE()).AI_unitImpassableCount(pImpassUnit->getUnitType()));
	}

	CvUnit* pBestUnit = NULL;
	int iBestValue = MAX_INT;
	int iLoop;
	CvSelectionGroup* pLoopGroup = NULL;
	for (pLoopGroup = GET_PLAYER(getOwnerINLINE()).firstSelectionGroup(&iLoop); pLoopGroup != NULL; pLoopGroup = GET_PLAYER(getOwnerINLINE()).nextSelectionGroup(&iLoop))
	{
		CvUnit* pLoopUnit = pLoopGroup->getHeadUnit();
		if (pLoopUnit == NULL)
			continue;

		CvPlot* pPlot = pLoopUnit->plot();
		if (AI_plotValid(pPlot))
		{
			if (iMaxPath != 0 || pPlot == plot())
			{	
				if (getDomainType() != DOMAIN_LAND || canMoveAllTerrain() ||
						// advc.030: Replacing the clause below
						canEnterArea(*pPlot->area()))
						//area() == pPlot->area())
				{
					if (AI_allowGroup(pLoopUnit, eUnitAI))
					{
						// K-Mod. I've restructed this wad of conditions so that it is easier for me to read.
						// ((removed ((heaps) of parentheses) (etc)).)
						// also, I've rearranged the order to be slightly faster for failed checks.
						// Note: the iMaxGroups & OwnUnitAI check is apparently off-by-one. This is for backwards compatibility for the original code.
						if (true
							&& (!bSafeOnly || !isEnemy(pPlot->getTeam()))
							&& (!bWithCargoOnly || pLoopUnit->getGroup()->hasCargo())
							&& (!bBiggerOnly || !bMergeGroups || pLoopGroup->getNumUnits() >= getGroup()->getNumUnits())
							&& (!bIgnoreFaster || pLoopGroup->baseMoves() <= baseMoves())
							&& (!bIgnoreOwnUnitType || pLoopUnit->getUnitType() != getUnitType())
							&& (!bIgnoreBusyTransports || !pLoopGroup->hasCargo() || (pLoopGroup->AI_getMissionAIType() != MISSIONAI_ASSAULT && pLoopGroup->AI_getMissionAIType() != MISSIONAI_REINFORCE))
							&& (iMinUnitAI == -1 || pLoopGroup->countNumUnitAIType(eUnitAI) >= iMinUnitAI)
							&& (iMaxOwnUnitAI == -1 || (bMergeGroups ? std::max(0, getGroup()->countNumUnitAIType(AI_getUnitAIType()) - 1) : 0) + pLoopGroup->countNumUnitAIType(AI_getUnitAIType()) <= iMaxOwnUnitAI + (bStackOfDoom ? AI_stackOfDoomExtra() : 0))
							&& (iMaxGroup == -1 || (bMergeGroups ? getGroup()->getNumUnits() - 1 : 0) + pLoopGroup->getNumUnits() + GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(pLoopUnit, MISSIONAI_GROUP, getGroup()) <= iMaxGroup + (bStackOfDoom ? AI_stackOfDoomExtra() : 0))
							&& (pLoopGroup->AI_getMissionAIType() != MISSIONAI_GUARD_CITY || !pLoopGroup->plot()->isCity() || pLoopGroup->plot()->plotCount(PUF_isMissionAIType, MISSIONAI_GUARD_CITY, -1, getOwnerINLINE()) > pLoopGroup->plot()->getPlotCity()->AI_minDefenders())
							)
						{
							FAssert(!pPlot->isVisibleEnemyUnit(this))
							if (iOurImpassableCount > 0 || AI_getUnitAIType() == UNITAI_ASSAULT_SEA)
							{
								int iTheirImpassableCount = 0;
								pUnitNode = pLoopGroup->headUnitNode();
								while (pUnitNode != NULL)
								{
									CvUnit* pImpassUnit = ::getUnit(pUnitNode->m_data);
									pUnitNode = pLoopGroup->nextUnitNode(pUnitNode);

									iTheirImpassableCount = std::max(iTheirImpassableCount, GET_PLAYER(getOwnerINLINE()).AI_unitImpassableCount(pImpassUnit->getUnitType()));
								}

								if( iOurImpassableCount != iTheirImpassableCount )
								{
									continue;
								}
							}

							int iPathTurns = 0;
							if (atPlot(pPlot) || generatePath(pPlot, iFlags, true, &iPathTurns, iMaxPath))
							{
								int iCost = 100 * (iPathTurns * iPathTurns + 1);
								iCost *= 4 + pLoopGroup->getCargo();
								iCost /= 2 + pLoopGroup->getNumUnits();
								/*int iSizeMod = 10*std::max(getGroup()->getNumUnits(), pLoopGroup->getNumUnits());
								iSizeMod /= std::min(getGroup()->getNumUnits(), pLoopGroup->getNumUnits());
								iCost *= iSizeMod * iSizeMod;
								iCost /= 1000; */

								if (iCost < iBestValue)
								{
									iBestValue = iCost;
									pBestUnit = pLoopUnit;
								}
							}
						}
					}
				}
			}
		}
	}
	
	if (pBestUnit != NULL)
	{
		if (!atPlot(pBestUnit->plot()))
		{
			if (!bMergeGroups && getGroup()->getNumUnits() > 1)
				joinGroup(NULL); // might as well leave our current group behind, since they won't be merging anyway.
			getGroup()->pushMission(MISSION_MOVE_TO_UNIT, pBestUnit->getOwnerINLINE(), pBestUnit->getID(), iFlags, false, false, MISSIONAI_GROUP, NULL, pBestUnit);
		}
		if (atPlot(pBestUnit->plot()))
		{
			if (bMergeGroups)
				getGroup()->mergeIntoGroup(pBestUnit->getGroup());
			else
				joinGroup(pBestUnit->getGroup());
		}
		return true;
	}

	return false;
}
// K-Mod end

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/22/10                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
// Added new options to aid transport grouping
// Returns true if a group was joined or a mission was pushed...
bool CvUnitAI::AI_group(UnitAITypes eUnitAI, int iMaxGroup, int iMaxOwnUnitAI, int iMinUnitAI, bool bIgnoreFaster, bool bIgnoreOwnUnitType, bool bStackOfDoom, int iMaxPath, bool bAllowRegrouping, bool bWithCargoOnly, bool bInCityOnly, MissionAITypes eIgnoreMissionAIType)
{
	// K-Mod. I've completely gutted this function. It's now basically just a wrapper for AI_omniGroup.
	// This is part of the process of phasing the function out.

	// unsupported features:
	FAssert(!bInCityOnly);
	FAssert(eIgnoreMissionAIType == NO_MISSIONAI || (eUnitAI == UNITAI_ASSAULT_SEA && eIgnoreMissionAIType == MISSIONAI_ASSAULT));
	// .. and now the function.

	if (!bAllowRegrouping)
	{
		if (getGroup()->getNumUnits() > 1)
		{
			return false;
		}
	}

	return AI_omniGroup(eUnitAI, iMaxGroup, iMaxOwnUnitAI, bStackOfDoom, 0, iMaxPath, true, true, bIgnoreFaster, bIgnoreOwnUnitType, false, iMinUnitAI, bWithCargoOnly, eIgnoreMissionAIType == MISSIONAI_ASSAULT);
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

bool CvUnitAI::AI_groupMergeRange(UnitAITypes eUnitAI, int iMaxRange, bool bBiggerOnly, bool bAllowRegrouping, bool bIgnoreFaster)
{
	// K-Mod. I've completely gutted this function. It's now basically just a wrapper for AI_omniGroup.
	// This is part of the process of phasing the function out.

	if (isCargo())
	{
		return false;
	}

    if (!bAllowRegrouping)
	{
		if (getGroup()->getNumUnits() > 1)
		{
			return false;
		}
	}

	// approximate max path based on range.
	int iMaxPath = 1;
	while (AI_searchRange(iMaxPath) < iMaxRange)
		iMaxPath++;

	return AI_omniGroup(eUnitAI, -1, -1, false, 0, iMaxPath, true, false, bIgnoreFaster, false, bBiggerOnly);
}

/*  K-Mod
	Look for the nearest suitable transport. Return a pointer to the transport unit.
	(the bulk of this function was moved straight out of AI_load.
	I've fixed it up a bit, but I didn't write most of it.) */
CvUnit* CvUnitAI::AI_findTransport(UnitAITypes eUnitAI, int iFlags, int iMaxPath,
		UnitAITypes eTransportedUnitAI, int iMinCargo, int iMinCargoSpace,
		int iMaxCargoSpace, int iMaxCargoOurUnitAI)
{
	/*if (getDomainType() == DOMAIN_LAND && !canMoveAllTerrain()) {
		if (area()->getNumAIUnits(getOwnerINLINE(), eUnitAI) == 0)
			return false;
	}*/ // disabled, because this would exclude boats sailing on the coast.

	// K-Mod
	if (eUnitAI != NO_UNITAI && GET_PLAYER(getOwnerINLINE()).AI_getNumAIUnits(eUnitAI) == 0)
		return NULL; // kmodx: was "false"
	// K-Mod end

	int iBestValue = MAX_INT;
	CvUnit* pBestUnit = 0;
	const int iLoadMissionAICount = 4;
	MissionAITypes aeLoadMissionAI[iLoadMissionAICount] = {
			MISSIONAI_LOAD_ASSAULT, MISSIONAI_LOAD_SETTLER,
			MISSIONAI_LOAD_SPECIAL, MISSIONAI_ATTACK_SPY};
	int iCurrentGroupSize = getGroup()->getNumUnits();
	CvPlayerAI const& kOwner = GET_PLAYER(getOwnerINLINE()); int iLoop=-1;
	for (CvUnit* pLoopUnit = kOwner.firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = kOwner.nextUnit(&iLoop))
	{	// K-Mod
		if (pLoopUnit->cargoSpace() <= 0 || (pLoopUnit->getArea() != getArea() &&
				!pLoopUnit->plot()->isAdjacentToArea(getArea())) ||
				!canLoadUnit(pLoopUnit, pLoopUnit->plot())) 
			continue; // K-Mod end

		UnitAITypes eLoopUnitAI = pLoopUnit->AI_getUnitAIType();
		if (eUnitAI == NO_UNITAI || eLoopUnitAI == eUnitAI)
		{
			int iCargoSpaceAvailable = pLoopUnit->cargoSpaceAvailable(getSpecialUnitType(), getDomainType());
			iCargoSpaceAvailable -= kOwner.AI_unitTargetMissionAIs(pLoopUnit,
					aeLoadMissionAI, iLoadMissionAICount, getGroup());
			if (iCargoSpaceAvailable > 0)
			{
				if ((eTransportedUnitAI == NO_UNITAI ||
						pLoopUnit->getUnitAICargo(eTransportedUnitAI) > 0) &&
						(iMinCargo == -1 || pLoopUnit->getCargo() >= iMinCargo))
				{	// <advc.040> Leave space for Settler and protection
					if(eLoopUnitAI == UNITAI_SETTLER_SEA && eUnitAI == UNITAI_SETTLER_SEA &&
							(eTransportedUnitAI == UNITAI_WORKER ||
							AI_getUnitAIType() == UNITAI_WORKER) &&
							pLoopUnit->cargoSpace() -
							pLoopUnit->getUnitAICargo(UNITAI_WORKER) <= 2)
						continue;
					// </advc.040>
					// Use existing count of cargo space available
					if ((iMinCargoSpace == -1 || iCargoSpaceAvailable >= iMinCargoSpace) &&
							(iMaxCargoSpace == -1 || iCargoSpaceAvailable <= iMaxCargoSpace))
					{
						if (iMaxCargoOurUnitAI == -1 ||
								pLoopUnit->getUnitAICargo(AI_getUnitAIType()) <= iMaxCargoOurUnitAI)
						{	// <advc.046> Don't join a pickup-stranded mission
							CvUnit* u = pLoopUnit->getGroup()->AI_getMissionAIUnit();
							if(u != NULL && u->plot()->getTeam() != getTeam() &&
									u->plot() != plot())
								continue; // </advc.046>
							if (!pLoopUnit->plot()->isVisibleEnemyUnit(this))
							{
								CvPlot* pUnitTargetPlot = pLoopUnit->getGroup()->AI_getMissionAIPlot();
								if (pUnitTargetPlot == NULL || pUnitTargetPlot->getTeam() == getTeam() ||
										(!pUnitTargetPlot->isOwned() ||
										!isPotentialEnemy(pUnitTargetPlot->getTeam(), pUnitTargetPlot)))
								{
									int iPathTurns = 0;
									if (atPlot(pLoopUnit->plot()) || generatePath(pLoopUnit->plot(), iFlags, true, &iPathTurns, iMaxPath))
									{
										// prefer a transport that can hold as much of our group as possible 
										int iValue = 5*std::max(0, iCurrentGroupSize - iCargoSpaceAvailable) + iPathTurns;

										if (iValue < iBestValue)
										{
											iBestValue = iValue;
											pBestUnit = pLoopUnit;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return pBestUnit;
}
// K-Mod end

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/18/10                                jdog5000      */
/*                                                                                              */
/* War tactics AI, Unit AI                                                                      */
/************************************************************************************************/
// Returns true if we loaded onto a transport or a mission was pushed...
bool CvUnitAI::AI_load(UnitAITypes eUnitAI, MissionAITypes eMissionAI,
		UnitAITypes eTransportedUnitAI, int iMinCargo, int iMinCargoSpace,
		int iMaxCargoSpace, int iMaxCargoOurUnitAI, int iFlags, int iMaxPath,
		int iMaxTransportPath)
{
	PROFILE_FUNC();

	if (getCargo() > 0)
		return false;

	if (isCargo())
	{
		getGroup()->pushMission(MISSION_SKIP);
		return true;
	}
	// K-Mod
	CvUnit* pBestUnit = AI_findTransport(eUnitAI, iFlags, iMaxPath,
			eTransportedUnitAI, iMinCargo, iMinCargoSpace, iMaxCargoSpace,
			iMaxCargoOurUnitAI);  // K-Mod end
	if (pBestUnit != NULL && iMaxTransportPath < MAX_INT &&
			(eUnitAI == UNITAI_ASSAULT_SEA || eUnitAI == UNITAI_SPY_SEA)) // K-Mod
	{
		// Can transport reach enemy in requested time
		bool bFoundEnemyPlotInRange = false;
		int iPathTurns;
		int iRange = iMaxTransportPath * pBestUnit->baseMoves();
		CvPlot* pAdjacentPlot = NULL;
		// K-Mod. use a separate pathfinder for the transports, so that we don't reset our current path data.
		KmodPathFinder temp_finder;
		temp_finder.SetSettings(CvPathSettings(pBestUnit->getGroup(), iFlags & MOVE_DECLARE_WAR, iMaxTransportPath, GC.getMOVE_DENOMINATOR()));
		//

		for( int iDX = -iRange; (iDX <= iRange && !bFoundEnemyPlotInRange); iDX++ )
		{
			for( int iDY = -iRange; (iDY <= iRange && !bFoundEnemyPlotInRange); iDY++ )
			{
				//CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);
				CvPlot* pLoopPlot = plotXY(pBestUnit->getX_INLINE(), pBestUnit->getY_INLINE(), iDX, iDY); // K-Mod

				if( pLoopPlot != NULL )
				{
					if( pLoopPlot->isCoastalLand() )
					{
						if( pLoopPlot->isOwned() )
						{
							if( isPotentialEnemy(pLoopPlot->getTeam(), pLoopPlot) && !isBarbarian() )
							{
								if( pLoopPlot->area()->getCitiesPerPlayer(pLoopPlot->getOwnerINLINE()) > 0 )
								{
									// Transport cannot enter land plot without cargo, so generate path only works properly if
									// land units are already loaded
									
									for( int iI = 0; (iI < NUM_DIRECTION_TYPES && !bFoundEnemyPlotInRange); iI++ )
									{
										pAdjacentPlot = plotDirection(getX_INLINE(), getY_INLINE(), (DirectionTypes)iI);
										if (pAdjacentPlot != NULL)
										{
											if( pAdjacentPlot->isWater() )
											{
												//if( pBestUnit->generatePath(pAdjacentPlot, 0, true, &iPathTurns, iMaxTransportPath) )
												if (temp_finder.GeneratePath(pAdjacentPlot))
												{
													/* if (pBestUnit->getPathLastNode()->m_iData1 == 0)
													{
														iPathTurns++;
													}*/
													iPathTurns = temp_finder.GetPathTurns() + (temp_finder.GetFinalMoves() == 0 ? 1 : 0); // K-Mod

													if( iPathTurns <= iMaxTransportPath )
													{
														bFoundEnemyPlotInRange = true;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}

		if( !bFoundEnemyPlotInRange )
		{
			pBestUnit = NULL;
		}
	}

	if (pBestUnit != NULL)
	{
		if (atPlot(pBestUnit->plot()))
		{
			CvSelectionGroup* pRemainderGroup = NULL; // K-Mod renamed from 'pOtherGroup'
			getGroup()->setTransportUnit(pBestUnit, &pRemainderGroup); // XXX is this dangerous (not pushing a mission...) XXX air units?

			// If part of large group loaded, then try to keep loading the rest
			if( eUnitAI == UNITAI_ASSAULT_SEA && eMissionAI == MISSIONAI_LOAD_ASSAULT )
			{
				if( pRemainderGroup != NULL && pRemainderGroup->getNumUnits() > 0 )
				{
					if( pRemainderGroup->getHeadUnitAI() == AI_getUnitAIType() )
					{
						if (pRemainderGroup->getHeadUnit()->AI_load( eUnitAI, eMissionAI, eTransportedUnitAI, iMinCargo, iMinCargoSpace, iMaxCargoSpace, iMaxCargoOurUnitAI, iFlags, 0, iMaxTransportPath ))
							pRemainderGroup->AI_setForceSeparate(false); // K-Mod
					}
					else if( eTransportedUnitAI == NO_UNITAI && iMinCargo < 0 && iMinCargoSpace < 0 && iMaxCargoSpace < 0 && iMaxCargoOurUnitAI < 0 )
					{
						if (pRemainderGroup->getHeadUnit()->AI_load( eUnitAI, eMissionAI, NO_UNITAI, -1, -1, -1, -1, iFlags, 0, iMaxTransportPath ))
							pRemainderGroup->AI_setForceSeparate(false); // K-Mod
					}
				}
			}
			// K-Mod - just for efficiency, I'll take care of the force separate stuff here.
			if (pRemainderGroup && pRemainderGroup->AI_isForceSeparate())
				pRemainderGroup->AI_separate();
			// K-Mod end

			return true;
		}
		else
		{
			// BBAI TODO: To split or not to split?
			// K-Mod. How about we this:
			// Split the group only if it is going to take more than 1 turn to get to the transport.
			if (generatePath(pBestUnit->plot(), iFlags, true, 0, 1))
			{
				// only 1 turn. Don't split.
				getGroup()->pushMission(MISSION_MOVE_TO_UNIT, pBestUnit->getOwnerINLINE(), pBestUnit->getID(), iFlags, false, false, eMissionAI, NULL, pBestUnit);
				return true;
			}
			else
			{
				// (bbai code. split the group)
				int iCargoSpaceAvailable = pBestUnit->cargoSpaceAvailable(getSpecialUnitType(), getDomainType());
				FAssertMsg(iCargoSpaceAvailable > 0, "best unit has no space");

				// split our group to fit on the transport
				CvSelectionGroup* pRemainderGroup = NULL;
				CvSelectionGroup* pSplitGroup = getGroup()->splitGroup(iCargoSpaceAvailable, this, &pRemainderGroup);			
				FAssertMsg(pSplitGroup, "splitGroup failed");
				FAssertMsg(getGroupID() == pSplitGroup->getID(), "splitGroup failed to put head unit in the new group");

				if (pSplitGroup != NULL)
				{
					CvPlot* pOldPlot = pSplitGroup->plot();
					pSplitGroup->pushMission(MISSION_MOVE_TO_UNIT, pBestUnit->getOwnerINLINE(), pBestUnit->getID(), iFlags, false, false, eMissionAI, NULL, pBestUnit);
					/* bool bMoved = (pSplitGroup->plot() != pOldPlot);
					if (!bMoved && pOtherGroup != NULL)
					{
						joinGroup(pOtherGroup);
					}
					return bMoved;
					*/ // K-Mod. (that block is obsolete)
					// K-Mod - just for efficiency, I'll take care of the force separate stuff here.
					if (pRemainderGroup && pRemainderGroup->AI_isForceSeparate())
						pRemainderGroup->AI_separate();
					// K-Mod end
					return true;
				}
			}
			// K-Mod end
		}
	}

	return false;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


// Returns true if a mission was pushed...
bool CvUnitAI::AI_guardCityBestDefender()
{
	CvCity* pCity;
	CvPlot* pPlot;

	pPlot = plot();
	pCity = pPlot->getPlotCity();

	if (pCity != NULL)
	{
		if (pCity->getOwnerINLINE() == getOwnerINLINE())
		{
			if (pPlot->getBestDefender(getOwnerINLINE()) == this)
			{
				getGroup()->pushMission(isFortifyable() ? MISSION_FORTIFY : MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_CITY, NULL);
				return true;
			}
		}
	}

	return false;
}

// K-Mod
bool CvUnitAI::AI_guardCityOnlyDefender()
{
	FAssert(getGroup()->getNumUnits() == 1);

	CvCity* pPlotCity = plot()->getPlotCity();
	if (pPlotCity && pPlotCity->getOwnerINLINE() == getOwnerINLINE())
	{
		if (plot()->plotCount(PUF_isMissionAIType, MISSIONAI_GUARD_CITY, -1, getOwnerINLINE()) <= (getGroup()->AI_getMissionAIType() == MISSIONAI_GUARD_CITY ? 1 : 0))
		{
			getGroup()->pushMission(isFortifyable() ? MISSION_FORTIFY : MISSION_SKIP, -1, -1, 0, false, false, noDefensiveBonus() ? NO_MISSIONAI : MISSIONAI_GUARD_CITY, 0);
			return true;
		}
	}
	return false;
}
// K-Mod end

bool CvUnitAI::AI_guardCityMinDefender(bool bSearch)
{
	PROFILE_FUNC();

	CvCity* pPlotCity = plot()->getPlotCity();
	if ((pPlotCity != NULL) && (pPlotCity->getOwnerINLINE() == getOwnerINLINE()))
	{
		/* original bts code
		int iCityDefenderCount = pPlotCity->plot()->plotCount(PUF_isUnitAIType, UNITAI_CITY_DEFENSE, -1, getOwnerINLINE());
		if ((iCityDefenderCount - 1) < pPlotCity->AI_minDefenders())
		{
			if ((iCityDefenderCount <= 2) || (GC.getGame().getSorenRandNum(5, "AI shuffle defender") != 0))
			{
				getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_CITY, NULL);
				return true;
			}
		}*/

		// K-mod
		// Note. For this check, we only count UNITAI_CITY_DEFENSE. But in the bSearch case, we count all guard_city units.
		int iDefendersHave = plot()->plotCount(PUF_isMissionAIType, MISSIONAI_GUARD_CITY, -1, getOwnerINLINE(), NO_TEAM, AI_getUnitAIType() == UNITAI_CITY_DEFENSE ? PUF_isUnitAIType : 0, UNITAI_CITY_DEFENSE);

		if (getGroup()->AI_getMissionAIType() == MISSIONAI_GUARD_CITY)
			iDefendersHave--;

		if (iDefendersHave < pPlotCity->AI_minDefenders())
		{
			if (iDefendersHave <= 1 || GC.getGame().getSorenRandNum(area()->getNumAIUnits(getOwnerINLINE(), UNITAI_CITY_DEFENSE) + 5, "AI shuffle defender") > 1)
			{
				getGroup()->pushMission(isFortifyable() ? MISSION_FORTIFY : MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_CITY, NULL);
				return true;
			}
		}
		// K-Mod end
	}

	if (bSearch)
	{
		int iBestValue = 0;
		CvPlot* pBestPlot = NULL;
		CvPlot* pBestGuardPlot = NULL;
		
		int iLoop;
		for(CvCity* pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
		{
			if (AI_plotValid(pLoopCity->plot()))
			{
				//int iDefendersHave = pLoopCity->plot()->plotCount(PUF_isUnitAIType, UNITAI_CITY_DEFENSE, -1, getOwnerINLINE());
				// K-Mod
				int iDefendersHave = pLoopCity->plot()->plotCount(PUF_isMissionAIType, MISSIONAI_GUARD_CITY, -1, getOwnerINLINE());
				if (pPlotCity == pLoopCity && getGroup()->AI_getMissionAIType() == MISSIONAI_GUARD_CITY)
					iDefendersHave--;
				// K-Mod end
				int iDefendersNeed = pLoopCity->AI_minDefenders();

				if (iDefendersHave < iDefendersNeed)
				{
					if (!pLoopCity->plot()->isVisibleEnemyUnit(this))
					{
						iDefendersHave += GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_GUARD_CITY, getGroup());
						if (iDefendersHave < iDefendersNeed + 1)
						{
							int iPathTurns;
							//if (!atPlot(pLoopCity->plot()) && generatePath(pLoopCity->plot(), 0, true, &iPathTurns))
							if (generatePath(pLoopCity->plot(), 0, true, &iPathTurns, 10)) // K-Mod. (also deleted "if (iPathTurns < 10)")
							{
								/* original bts code
								int iValue = (iDefendersNeed - iDefendersHave) * 20;
								iValue += 2 * std::min(15, iCurrentTurn - pLoopCity->getGameTurnAcquired());
								if (pLoopCity->isOccupation())
								{
									iValue += 5;
								}

								iValue -= iPathTurns;
								*/
								// K-Mod
								int iValue = (iDefendersNeed - iDefendersHave) * 10;
								iValue += iDefendersHave <= 0 ? 10 : 0;

								iValue += 2 * pLoopCity->getCultureLevel();
								iValue += pLoopCity->getPopulation() / 3;
								iValue += pLoopCity->isOccupation() ? 8 : 0;
								iValue -= iPathTurns;
								// K-Mod end

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = getPathEndTurnPlot();
									pBestGuardPlot = pLoopCity->plot();
								}
							}
						}
					}
				}
			}
		}
		if (pBestPlot != NULL)
		{
			if (atPlot(pBestGuardPlot))
			{
				FAssert(pBestGuardPlot == pBestPlot);
				getGroup()->pushMission(isFortifyable() ? MISSION_FORTIFY : MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_CITY, NULL);
				return true;
			}
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_GUARD_CITY, pBestGuardPlot);
			return true;
		}
	}
	
	return false;
}

// Returns true if a mission was pushed...
// K-Mod. This function was so full of useless cruft and duplicated code and double-counting mistakes...
// I've deleted the bulk of the old code, and rewritten it to be much much simpler - and also better.
bool CvUnitAI::AI_guardCity(bool bLeave, bool bSearch, int iMaxPath, int iFlags)
{
	PROFILE_FUNC();

	FAssert(getDomainType() == DOMAIN_LAND);
	FAssert(canDefend());

	CvPlot* pEndTurnPlot = NULL;
	CvPlot* pBestGuardPlot = NULL;


	CvPlot* pPlot = plot();
	CvCity* pCity = pPlot->getPlotCity(); // advc.003
	if (pCity != NULL && pCity->getOwnerINLINE() == getOwnerINLINE())
	{
		int iExtra; // additional defenders needed.
		if (bLeave && !pCity->AI_isDanger())
		{
			iExtra = -1;
		}
		else
		{
			iExtra = bSearch ? 0 : GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(pPlot, 2);
		}

		if (pPlot->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(), NO_TEAM, AI_isCityAIType() ? PUF_isCityAIType : 0)
					< pCity->AI_neededDefenders() + 1 + iExtra) // +1 because this unit is being counted as a defender.
		{
			// don't bother searching. We're staying here.
			bSearch = false;
			pEndTurnPlot = plot();
			pBestGuardPlot = plot();
		}
	}

	if (bSearch)
	{
		int iBestValue = 0;

		int iLoop;
		for (CvCity* pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
		{
			if (AI_plotValid(pLoopCity->plot()))
			{
				// BBAI efficiency: check area for land units
				if( (getDomainType() == DOMAIN_LAND) && (pLoopCity->area() != area()) && !(getGroup()->canMoveAllTerrain()) )
					continue;

				//if (!pLoopCity->AI_isDefended((!AI_isCityAIType() ? pLoopCity->plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isNotCityAIType) : 0)))
				// K-Mod
				int iDefendersNeeded = pLoopCity->AI_neededDefenders(true);
				int iDefendersHave = pLoopCity->plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(), NO_TEAM, AI_isCityAIType() ? PUF_isCityAIType : 0);
				if (pCity == pLoopCity)
					iDefendersHave-=getGroup()->getNumUnits();
				// K-Mod end
				/*  <advc.139> Reinforce city despite evac if group large enough.
					Don't want cities to be abandoned unnecessarily just b/c few
					units were garrisoned when the enemy stack arrived, but don't
					want them to move in and out either; tricky to get right.
					CvCityAI::updateEvacuating already checks for potential defenders
					within 3 tiles of the city. If this stack can be farther away
					than that, it'll probably not arrive in time to save the city,
					but it might, or could quickly retake the city. */
				int delta = iDefendersNeeded - iDefendersHave;
				if(delta <= 0)
					continue; // No functional change from BtS
				bool evac = pLoopCity->AI_isEvacuating();
				if(!evac || delta <= ::round(0.75 * getGroup()->getNumUnits())) {
				// </advc.139>
					if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
					{
						if ((GC.getGame().getGameTurn() - pLoopCity->getGameTurnAcquired() < 10) || GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_GUARD_CITY, getGroup()) < 2)
						{
							int iPathTurns;
							if (!atPlot(pLoopCity->plot()) && generatePath(pLoopCity->plot(), iFlags, true, &iPathTurns, iMaxPath))
							{
								if (iPathTurns <= iMaxPath)
								{
									int iValue = 1000 * (1 + iDefendersNeeded - iDefendersHave);
									iValue /= 1 + iPathTurns + iDefendersHave;
									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pEndTurnPlot = getPathEndTurnPlot();
										pBestGuardPlot = pLoopCity->plot();
										FAssert(!atPlot(pEndTurnPlot));
										if (iMaxPath == 1 || iBestValue >= 500)
											break; // we found a good city. No need to waste any more time looking.
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if (pEndTurnPlot != NULL && pBestGuardPlot != NULL)
	{
		CvSelectionGroup* pOldGroup = getGroup();
		CvUnit* pEjectedUnit = getGroup()->AI_ejectBestDefender(pPlot);

		if (!pEjectedUnit)
		{
			FAssertMsg(false, "AI_ejectBestDefender failed to choose a candidate for AI_guardCity.");
			pEjectedUnit = this;
			if (getGroup()->getNumUnits() > 0)
				joinGroup(0);
		}

		FAssert(pEjectedUnit);

		// If the unit is not suited for defense, do not use MISSIONAI_GUARD_CITY.
		MissionAITypes eMissionAI = pEjectedUnit->noDefensiveBonus() ? NO_MISSIONAI : MISSIONAI_GUARD_CITY;
		if (atPlot(pBestGuardPlot))
		{
			pEjectedUnit->getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, eMissionAI, 0);
		}
		else
		{
			FAssert(bSearch);
			FAssert(!atPlot(pEndTurnPlot));
			pEjectedUnit->getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX_INLINE(), pEndTurnPlot->getY_INLINE(), iFlags, false, false, eMissionAI, pBestGuardPlot);
		}

		if (pEjectedUnit->getGroup() == pOldGroup || pEjectedUnit == this)
			return true;
		else
			return false;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_guardCityAirlift()
{
	PROFILE_FUNC();

	CvCity* pCity;
	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	int iValue;
	int iBestValue;
	int iLoop;

	if (getGroup()->getNumUnits() > 1)
	{
		return false;
	}

	pCity = plot()->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (pCity->getMaxAirlift() == 0)
	{
		return false;
	}

	iBestValue = 0;
	pBestPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (pLoopCity != pCity)
		{
			if (canAirliftAt(pCity->plot(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE()))
			{
				if (!(pLoopCity->AI_isDefended((!AI_isCityAIType()) ? pLoopCity->plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isNotCityAIType) : 0)))	// XXX check for other team's units?
				{
					iValue = pLoopCity->getPopulation();

					if (pLoopCity->AI_isDanger())
					{
						iValue *= 2;
					}

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestPlot = pLoopCity->plot();
						FAssert(pLoopCity != pCity);
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_AIRLIFT, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}

// K-Mod.
// This function will use our naval unit to block the coast outside our cities.
bool CvUnitAI::AI_guardCoast(bool bPrimaryOnly, int iFlags, int iMaxPath)
{
	PROFILE_FUNC();

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());

	CvPlot* pBestCityPlot = 0;
	CvPlot* pEndTurnPlot = 0;
	int iBestValue = 0;

	int iLoop;
	for (CvCity* pLoopCity = kOwner.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kOwner.nextCity(&iLoop))
	{
		if (!pLoopCity->isCoastal() || (bPrimaryOnly && !kOwner.AI_isPrimaryArea(pLoopCity->area())))
			continue;

		int iCoastPlots = 0;

		for (DirectionTypes i = (DirectionTypes)0; i < NUM_DIRECTION_TYPES; i=(DirectionTypes)(i+1))
		{
			CvPlot* pAdjacentPlot = plotDirection(pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), i);

			if (pAdjacentPlot && pAdjacentPlot->isWater() && pAdjacentPlot->area()->getNumTiles() >= GC.getMIN_WATER_SIZE_FOR_OCEAN())
				iCoastPlots++;
		}

		int iBaseValue = iCoastPlots > 0
			? 1000 * pLoopCity->AI_neededDefenders() / (iCoastPlots + 3) // arbitrary units (AI_cityValue is a bit slower)
			: 0;

		iBaseValue /= kOwner.AI_isLandWar(pLoopCity->area()) ? 2 : 1;

		if (iBaseValue <= iBestValue)
			continue;

		iBaseValue *= 4;
		iBaseValue /= 4 + kOwner.AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_GUARD_COAST, getGroup(), 1);

		if (iBaseValue <= iBestValue)
			continue;

		for (DirectionTypes i = (DirectionTypes)0; i < NUM_DIRECTION_TYPES; i=(DirectionTypes)(i+1))
		{
			CvPlot* pAdjacentPlot = plotDirection(pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), i);

			if (!pAdjacentPlot || !pAdjacentPlot->isWater() || pAdjacentPlot->area()->getNumTiles() < GC.getMIN_WATER_SIZE_FOR_OCEAN() || pAdjacentPlot->getTeam() != getTeam())
				continue;

			int iValue = iBaseValue;

			iValue *= 2;
			iValue /= pAdjacentPlot->getBonusType(getTeam()) == NO_BONUS ? 3 : 2;
			iValue *= 3;
			iValue /= std::max(3, (atPlot(pAdjacentPlot) ? 1-getGroup()->getNumUnits() : 1)+pAdjacentPlot->plotCount(PUF_isMissionAIType, MISSIONAI_GUARD_COAST, -1, getOwnerINLINE()));

			int iPathTurns;
			if (iValue > iBestValue && generatePath(pAdjacentPlot, iFlags, true, &iPathTurns, iMaxPath))
			{
				iValue *= 4;
				iValue /= 3 + iPathTurns;

				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					pBestCityPlot = pLoopCity->plot();
					pEndTurnPlot = getPathEndTurnPlot();
				}
			}
		}
	}

	if (pEndTurnPlot)
	{
		if (atPlot(pEndTurnPlot))
		{
			getGroup()->pushMission(canSeaPatrol(pEndTurnPlot) ? MISSION_SEAPATROL : MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_COAST, pBestCityPlot);
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX_INLINE(), pEndTurnPlot->getY_INLINE(), iFlags, false, false, MISSIONAI_GUARD_COAST, pBestCityPlot);
		}
		return true;
	}

	return false;
}
// K-Mod end


// Returns true if a mission was pushed...
bool CvUnitAI::AI_guardBonus(int iMinValue)
{
	PROFILE_FUNC();
	// <advc.107> No defenders to spare for bonuses
	if(GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_TURTLE) ||
			(area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE &&
			GET_TEAM(getTeam()).AI_getWarSuccessRating() < -80))
		return false; // </advc.107>
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestGuardPlot;
	//ImprovementTypes eImprovement; // K-Mod made this obsolete
	BonusTypes eNonObsoleteBonus;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iI;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestGuardPlot = NULL;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
		// <advc.003b> Might make this expensive loop a tad cheaper
		if(pLoopPlot->getOwnerINLINE() != getOwnerINLINE())
			continue; // </advc.003b>
		if (AI_plotValid(pLoopPlot))
		{
			//if (pLoopPlot->getOwnerINLINE() == getOwnerINLINE())
			{
				eNonObsoleteBonus = pLoopPlot->getNonObsoleteBonusType(getTeam(), true);

				//if (eNonObsoleteBonus != NO_BONUS)
				if (eNonObsoleteBonus != NO_BONUS && pLoopPlot->isValidDomainForAction(*this)) // K-Mod. (boats shouldn't defend forts!)
				{
					//iValue = GET_PLAYER(getOwnerINLINE()).AI_bonusVal(eNonObsoleteBonus);
					iValue = GET_PLAYER(getOwnerINLINE()).AI_bonusVal(eNonObsoleteBonus, 0); // K-Mod

					iValue += std::max(0, 200 * GC.getBonusInfo(eNonObsoleteBonus).getAIObjective());

					if (pLoopPlot->getPlotGroupConnectedBonus(getOwnerINLINE(), eNonObsoleteBonus) == 1)
					{
						iValue *= 2;
					}

					if (iValue > iMinValue)
					{
						if (!(pLoopPlot->isVisibleEnemyUnit(this)))
						{
							//if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_GUARD_BONUS, getGroup()) == 0)
							// K-Mod
							iValue *= 2;
							iValue /= 2 + GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_GUARD_BONUS, getGroup());
							if (iValue > iMinValue)
							// K-Mod end
							{
								if (generatePath(pLoopPlot, 0, true, &iPathTurns))
								{
									iValue *= 1000;

									iValue /= iPathTurns + 4; // was +1

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
										pBestGuardPlot = pLoopPlot;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestGuardPlot != NULL))
	{
		if (atPlot(pBestGuardPlot))
		{
			//getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_BONUS, pBestGuardPlot);
			getGroup()->pushMission(canSeaPatrol(pBestGuardPlot) ? MISSION_SEAPATROL : (isFortifyable() ? MISSION_FORTIFY : MISSION_SKIP), -1, -1, 0, false, false, MISSIONAI_GUARD_BONUS, pBestGuardPlot); // K-Mod
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_GUARD_BONUS, pBestGuardPlot);
			return true;
		}
	}

	return false;
}

// <advc.300> Structure adopted from AI_guardBonus
bool CvUnitAI::AI_guardYield() {

	// <advc.107>
	if(GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_TURTLE))
		return false; // </advc.107>
	CvCity* c = plot()->getPlotCity();
	// For now, only consider nearby city if already guarding bonus.
	MissionTypes stayPut = isFortifyable() ? MISSION_FORTIFY : MISSION_SKIP;
	MissionAITypes const GB = MISSIONAI_GUARD_BONUS;
	if(c == NULL && getGroup()->AI_getMissionAIType() == GB)
		c = plot()->getWorkingCity();
	if(c == NULL || c->getOwnerINLINE() != getOwnerINLINE())
		return false;
	int bestVal = 6;
	if(!GC.getGameINLINE().isOption(GAMEOPTION_RAGING_BARBARIANS))
		bestVal = 8;
	CvPlot* bestPlot = NULL;
	for(int i = 0; i < NUM_CITY_PLOTS; i++) {
		CvPlot* pp = plotCity(plot()->getX_INLINE(), plot()->getY_INLINE(), i);
		if(pp == NULL || !AI_plotValid(pp)) continue; CvPlot const& p = *pp;
		if(p.getOwnerINLINE() != getOwnerINLINE() ||
				p.getImprovementType() == NO_IMPROVEMENT ||
				p.getPlotCity() != NULL || p.isWater() || p.isUnit())
			continue;
		int pathTurns;
		/*  Must be reachable in one hop, so that we can hurry back to the city
			when it's in danger. */
		if(!generatePath(pp, 0, true, &pathTurns) || pathTurns > 1)
			continue;
		int val = 0;
		// Example: Plains Hill Mine in the outer ring. Score 8.
		for(int j = 0; j < NUM_YIELD_TYPES; j++) {
			YieldTypes y = (YieldTypes)j;
			int yieldScore = p.getYield(y);
			if(y != YIELD_COMMERCE)
				yieldScore *= 2;
			val += yieldScore;
		}
		/*  BARBARIAN_TEAM: It's more about the defense barbs will have if
			allowed to enter the tile than our unit's defense. */
		/*  Example cont.: +2 from the Hill (25/10 rounded down).
			I.e. it's worth protecting (barely). */
		val += p.defenseModifier(BARBARIAN_TEAM, true, getTeam()) / 10; // advc.012
		if(val <= bestVal) // Will only decrease from here
			continue;
		// Guard only tiles near invisible regions (where barbs might appear)
		CvPlot const* ni = p.nearestInvisiblePlot(true, 5, getTeam());
		if(ni == NULL)
			continue;
		val -= ::plotDistance(ni, pp);
		if(val > bestVal) {
			bestVal = val;
			bestPlot = pp;
		}
	}
	if(bestPlot == NULL)
		return false;
	if(atPlot(bestPlot))
		 getGroup()->pushMission(stayPut, -1, -1, 0, false, false, GB, bestPlot);
	else getGroup()->pushMission(MISSION_MOVE_TO, bestPlot->getX_INLINE(),
			bestPlot->getY_INLINE(), 0, false, false, GB, bestPlot);
	return true;
} // </advc.300>

// <advc.306>
bool CvUnitAI::AI_barbAmphibiousCapture() {

	bool targetWithinOwnBorders = true;
	CvPlot* dest = NULL;
	MissionAITypes mission = MISSIONAI_PILLAGE;
	for(int i = 0; i < GC.getNumDirections(); i++) {
		DirectionTypes dir = (DirectionTypes)i;
		CvPlot* pp = ::plotDirection(plot()->getX_INLINE(), plot()->getY_INLINE(), dir);
		if(pp == NULL) continue; CvPlot const& p = *pp;
		// <advc.306>
		if(p.getTeam() != NO_TEAM && p.area()->isBorderObstacle(p.getTeam()))
			continue; // </advc.306>
		/*  Undefended city (perhaps unnecessary; not sure if the assault routine
			gets this right, or if it would drop units next to the city) */
		CvCity* c = p.getPlotCity();
		if(c != NULL && !c->isBarbarian() && !p.isVisibleEnemyDefender(this)) {
			dest = pp;
			// Sudden attacks on undefended cities are OK (see below)
			targetWithinOwnBorders = false; // Not a good variable name
			mission = MISSIONAI_ASSAULT;
			break;
		}
		if(!p.isUnit() || p.isWater())
			continue;
		/*  Don't attack stacks of AI civilians. Can't be so lenient with humans;
			could be exploited by having threatened Workers huddle together. */
		if(p.getNumUnits() > 2 && p.getOwnerINLINE() != NO_PLAYER &&
				!GET_PLAYER(p.getOwnerINLINE()).isHuman())
			continue;
		CvUnit* head = p.getUnitByIndex(0);
		if(head->getOwner() != getOwner() && !head->isCombat()) {
			bool withinOwnBorders = (p.getOwnerINLINE() == head->getOwnerINLINE());
			// Prefer target outside its borders (see below)
			if(dest == NULL || (targetWithinOwnBorders && !withinOwnBorders)) {
				dest = pp;
				targetWithinOwnBorders = withinOwnBorders;
				if(!targetWithinOwnBorders)
					break;
			}
		}
	}
	if(dest == NULL)
		return false;
	/*  When borders don't reach out onto sea, it's possible that barbs enter
		visibility with enough moves left to kill a Worker.
		Don't want such attacks "out of the blue".
		Exception: Undefended non-combatants outside their owner's borders
		are always fair game. */
	if(targetWithinOwnBorders && plot()->getOwner() != getOwner() &&
			plot()->getOwner() != dest->getOwner())
		return false;
	if(AI_transportGoTo(plot(), dest, 0, mission))
		return true;
	return false;
} // </advc.306>


int CvUnitAI::AI_getPlotDefendersNeeded(CvPlot* pPlot, int iExtra)
{
	int iNeeded = iExtra;
	BonusTypes eNonObsoleteBonus = pPlot->getNonObsoleteBonusType(getTeam());
	if (eNonObsoleteBonus != NO_BONUS)
	{
		iNeeded += (GET_PLAYER(getOwnerINLINE()).AI_bonusVal(eNonObsoleteBonus, 1) + 10) / 19;
	}

	int iDefense = pPlot->defenseModifier(getTeam(), true);

	iNeeded += (iDefense + 25) / 50;

	if (iNeeded == 0)
	{
		return 0;
	}

	iNeeded += GET_PLAYER(getOwnerINLINE()).AI_getPlotAirbaseValue(pPlot) / 50;

	int iNumHostiles = 0;
	int iNumPlots = 0;

	int iRange = 2;
	for (int iX = -iRange; iX <= iRange; iX++)
	{
		for (int iY = -iRange; iY <= iRange; iY++)
		{
			CvPlot* pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iX, iY);
			if (pLoopPlot != NULL)
			{
				iNumHostiles += pLoopPlot->getNumVisibleEnemyDefenders(this);
				if ((pLoopPlot->getTeam() != getTeam()) || pLoopPlot->isCoastalLand())
				{
				    iNumPlots++;
                    if (isEnemy(pLoopPlot->getTeam()))
                    {
                        iNumPlots += 4;
                    }
				}
			}
		}
	}

	if ((iNumHostiles == 0) && (iNumPlots < 4))
	{
		if (iNeeded > 1)
		{
			iNeeded = 1;
		}
		else
		{
			iNeeded = 0;
		}
	}

	return iNeeded;
}

bool CvUnitAI::AI_guardFort(bool bSearch)
{
	PROFILE_FUNC();
	// <advc.107> Don't guard forts when cities are falling
	if(GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_TURTLE))
		return false; // </advc.107>
	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		ImprovementTypes eImprovement = plot()->getImprovementType();
		if (eImprovement != NO_IMPROVEMENT)
		{
			if (GC.getImprovementInfo(eImprovement).isActsAsCity())
			{
				if (plot()->plotCount(PUF_isCityAIType, -1, -1, getOwnerINLINE()) <= AI_getPlotDefendersNeeded(plot(), 0))
				{
					getGroup()->pushMission(isFortifyable() ? MISSION_FORTIFY : MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_BONUS, plot());
					return true;
				}
			}
		}
	}
	
	if (!bSearch)
	{
		return false;
	}

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestGuardPlot = NULL;

	for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot) && !atPlot(pLoopPlot))
		{
			if (pLoopPlot->getOwnerINLINE() == getOwnerINLINE())
			{
				ImprovementTypes eImprovement = pLoopPlot->getImprovementType();
				if (eImprovement != NO_IMPROVEMENT)
				{
					if (GC.getImprovementInfo(eImprovement).isActsAsCity())
					{
						int iValue = AI_getPlotDefendersNeeded(pLoopPlot, 0);

						if (iValue > 0)
						{
							if (!(pLoopPlot->isVisibleEnemyUnit(this)))
							{
								if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_GUARD_BONUS, getGroup()) < iValue)
								{
									int iPathTurns;
									if (generatePath(pLoopPlot, 0, true, &iPathTurns))
									{
										iValue *= 1000;

										iValue /= (iPathTurns + 2);

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = getPathEndTurnPlot();
											pBestGuardPlot = pLoopPlot;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestGuardPlot != NULL))
	{
		if (atPlot(pBestGuardPlot))
		{
			getGroup()->pushMission(isFortifyable() ? MISSION_FORTIFY : MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_BONUS, pBestGuardPlot);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_GUARD_BONUS, pBestGuardPlot);
			return true;
		}
	}

	return false;
}
// Returns true if a mission was pushed...
bool CvUnitAI::AI_guardCitySite()
{
	PROFILE_FUNC();

	int iPathTurns=-1;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestGuardPlot = NULL;
	CvPlayerAI const& kOwner = GET_PLAYER(getOwnerINLINE()); // advc.003
	//iBestValue = 0; // <advc.300>
	// Don't guard any ole tile with a positive found value; only actual city sites. */
	int iBestValue = kOwner.AI_getMinFoundValue() - 1;
	CvMap const& m = GC.getMapINLINE(); // </advc.300>
	for (int iI = 0; iI < kOwner.AI_getNumCitySites(); iI++)
	{
		CvPlot* pLoopPlot = kOwner.AI_getCitySite(iI);
		//if (owner.AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_GUARD_CITY, getGroup()) == 0)
		// <advc.300> Need to check the adjacent tiles too
		bool bValid = true;
		for(int dx = -1; dx <= 1; dx++)
		for(int dy = -1; dy <= 1; dy++) {
			CvPlot* adj = m.plot(pLoopPlot->getX_INLINE() + dx,
					pLoopPlot->getY_INLINE() + dy);
			if(adj != NULL && kOwner.AI_plotTargetMissionAIs(adj,
					MISSIONAI_GUARD_CITY, getGroup()) > 0) {
				bValid = false;
				dx = dy = 2; // break x2
			}
		}
		if(bValid) // </advc.300>
		{
			// K-Mod. I've switched the order of the following two if statements, for efficiency.
			int iValue = pLoopPlot->getFoundValue(kOwner.getID());
			if (iValue > iBestValue)
			{
				if (generatePath(pLoopPlot, 0, true, &iPathTurns))
				{
					iBestValue = iValue;
					pBestPlot = getPathEndTurnPlot();
					pBestGuardPlot = pLoopPlot;
				}
			}
		}
	}
	// <advc.300> Guard an adjacent plot if it's better for fogbusting
	if(pBestGuardPlot != NULL) {
		int iBestGuardVal = 0;
		CvPlot* pBetterGuardPlot = pBestGuardPlot;
		for(int dx = -1; dx <= 1; dx++) {
			for(int dy = -1; dy <= 1; dy++) {
				CvPlot* adj = m.plot(pBestGuardPlot->getX_INLINE() + dx,
						pBestGuardPlot->getY_INLINE() + dy);
				if(adj == NULL || adj->isImpassable())
					continue;
				int iGuardVal = adj->defenseModifier(getTeam(), true);
				iGuardVal += adj->seeFromLevel(getTeam()) * 30;
				if(adj == pBestGuardPlot)
					iGuardVal += 1; // Tiebreaker
				if(iGuardVal > iBestGuardVal) {
					iBestGuardVal = iGuardVal;
					pBetterGuardPlot = adj;
				}
			}
		}
		if(pBetterGuardPlot != pBestGuardPlot && 
				generatePath(pBetterGuardPlot, 0, true, &iPathTurns)) {
			pBestPlot = getPathEndTurnPlot();
			pBestGuardPlot = pBetterGuardPlot;
		}
	} // </advc.300>
	if ((pBestPlot != NULL) && (pBestGuardPlot != NULL))
	{

		if (atPlot(pBestGuardPlot))
		{
			getGroup()->pushMission(isFortifyable() ? MISSION_FORTIFY : MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_CITY, pBestGuardPlot);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_GUARD_CITY, pBestGuardPlot);
			return true;
		}
	}

	return false;
}



// Returns true if a mission was pushed...
bool CvUnitAI::AI_guardSpy(int iRandomPercent)
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	CvPlot* pBestGuardPlot;
	int iValue;
	int iBestValue;
	int iLoop;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestGuardPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (AI_plotValid(pLoopCity->plot()))
		{
			// if (!(pLoopCity->plot()->isVisibleEnemyUnit(this))) // disabled by K-Mod. This isn't required for spies...
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
				// BBAI efficiency: check area for land units
				if( (getDomainType() == DOMAIN_LAND) && (pLoopCity->area() != area()) && !(getGroup()->canMoveAllTerrain()) )
				{
					continue;
				}

				iValue = 0;

				if( GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_SPACE4) )
				{
					if( pLoopCity->isCapital() )
					{
						iValue += 30;
					}
					else if( pLoopCity->isProductionProject() )
					{
						iValue += 5;
					}
				}

				if( GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) )
				{
					if( pLoopCity->getCultureLevel() >= (GC.getNumCultureLevelInfos() - 2))
					{
						iValue += 10;
					}
				}
				
				if (pLoopCity->isProductionUnit())
				{
					if (isLimitedUnitClass((UnitClassTypes)(GC.getUnitInfo(pLoopCity->getProductionUnit()).getUnitClassType())))
					{
						iValue += 4;
					}
				}
				else if (pLoopCity->isProductionBuilding())
				{
					if (isLimitedWonderClass((BuildingClassTypes)(GC.getBuildingInfo(pLoopCity->getProductionBuilding()).getBuildingClassType())))
					{
						iValue += 5;
					}
				}
				else if (pLoopCity->isProductionProject())
				{
					if (isLimitedProject(pLoopCity->getProductionProject()))
					{
						iValue += 6;
					}
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

				if (iValue > 0)
				{
					if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_GUARD_SPY, getGroup()) == 0)
					{
						int iPathTurns;
						if (generatePath(pLoopCity->plot(), 0, true, &iPathTurns))
						{
							iValue *= 100 + GC.getGameINLINE().getSorenRandNum(iRandomPercent, "AI Guard Spy");
							//iValue /= 100;
							iValue /= iPathTurns + 1;

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = getPathEndTurnPlot();
								pBestGuardPlot = pLoopCity->plot();
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestGuardPlot != NULL))
	{
		if (atPlot(pBestGuardPlot))
		{
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_SPY, pBestGuardPlot);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_GUARD_SPY, pBestGuardPlot);
			return true;
		}
	}

	return false;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/25/09                                jdog5000      */
/*                                                                                              */
/* Espionage AI                                                                                 */
/************************************************************************************************/					
/*
// Never used BTS functions ... 

// Returns true if a mission was pushed...
bool CvUnitAI::AI_destroySpy()
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvCity* pBestCity;
	CvPlot* pBestPlot;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestCity = NULL;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam())
			{
				if (GET_PLAYER(getOwnerINLINE()).AI_getAttitude((PlayerTypes)iI) <= ATTITUDE_ANNOYED)
				{
					for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
					{
						if (AI_plotValid(pLoopCity->plot()))
						{
							if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_ATTACK_SPY, getGroup()) == 0)
							{
								if (generatePath(pLoopCity->plot(), 0, true))
								{
									iValue = (pLoopCity->getPopulation() * 2);

									iValue += pLoopCity->getYieldRate(YIELD_PRODUCTION);

									if (atPlot(pLoopCity->plot()))
									{
										iValue *= 4;
										iValue /= 3;
									}

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
										pBestCity = pLoopCity;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestCity != NULL))
	{
		if (atPlot(pBestCity->plot()))
		{
			if (canDestroy(pBestCity->plot()))
			{
				if (pBestCity->getProduction() > ((pBestCity->getProductionNeeded() * 2) / 3))
				{
					if (pBestCity->isProductionUnit())
					{
						if (isLimitedUnitClass((UnitClassTypes)(GC.getUnitInfo(pBestCity->getProductionUnit()).getUnitClassType())))
						{
							getGroup()->pushMission(MISSION_DESTROY);
							return true;
						}
					}
					else if (pBestCity->isProductionBuilding())
					{
						if (isLimitedWonderClass((BuildingClassTypes)(GC.getBuildingInfo(pBestCity->getProductionBuilding()).getBuildingClassType())))
						{
							getGroup()->pushMission(MISSION_DESTROY);
							return true;
						}
					}
					else if (pBestCity->isProductionProject())
					{
						if (isLimitedProject(pBestCity->getProductionProject()))
						{
							getGroup()->pushMission(MISSION_DESTROY);
							return true;
						}
					}
				}
			}

			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY, pBestCity->plot());
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_ATTACK_SPY, pBestCity->plot());
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_sabotageSpy()
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestSabotagePlot;
	bool abPlayerAngry[MAX_PLAYERS];
	ImprovementTypes eImprovement;
	BonusTypes eNonObsoleteBonus;
	int iValue;
	int iBestValue;
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		abPlayerAngry[iI] = false;

		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam())
			{
				if (GET_PLAYER(getOwnerINLINE()).AI_getAttitude((PlayerTypes)iI) <= ATTITUDE_ANNOYED)
				{
					abPlayerAngry[iI] = true;
				}
			}
		}
	}

	iBestValue = 0;
	pBestPlot = NULL;
	pBestSabotagePlot = NULL;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot))
		{
			if (pLoopPlot->isOwned())
			{
				if (pLoopPlot->getTeam() != getTeam())
				{
					if (abPlayerAngry[pLoopPlot->getOwnerINLINE()])
					{
						eNonObsoleteBonus = pLoopPlot->getNonObsoleteBonusType(pLoopPlot->getTeam());

						if (eNonObsoleteBonus != NO_BONUS)
						{
							eImprovement = pLoopPlot->getImprovementType();

							if ((eImprovement != NO_IMPROVEMENT) && GC.getImprovementInfo(eImprovement).isImprovementBonusTrade(eNonObsoleteBonus))
							{
								if (canSabotage(pLoopPlot))
								{
									iValue = GET_PLAYER(pLoopPlot->getOwnerINLINE()).AI_bonusVal(eNonObsoleteBonus);

									if (pLoopPlot->isConnectedToCapital() && (pLoopPlot->getPlotGroupConnectedBonus(pLoopPlot->getOwnerINLINE(), eNonObsoleteBonus) == 1))
									{
										iValue *= 3;
									}

									if (iValue > 25)
									{
										if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_ATTACK_SPY, getGroup()) == 0)
										{
											if (generatePath(pLoopPlot, 0, true))
											{
												if (iValue > iBestValue)
												{
													iBestValue = iValue;
													pBestPlot = getPathEndTurnPlot();
													pBestSabotagePlot = pLoopPlot;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestSabotagePlot != NULL))
	{
		if (atPlot(pBestSabotagePlot))
		{
			getGroup()->pushMission(MISSION_SABOTAGE);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_ATTACK_SPY, pBestSabotagePlot);
			return true;
		}
	}

	return false;
}


bool CvUnitAI::AI_pickupTargetSpy()
{
	PROFILE_FUNC();

	CvCity* pCity;
	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	CvPlot* pBestPickupPlot;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iLoop;

	pCity = plot()->getPlotCity();

	if (pCity != NULL)
	{
		if (pCity->getOwnerINLINE() == getOwnerINLINE())
		{
			if (pCity->isCoastal(GC.getMIN_WATER_SIZE_FOR_OCEAN()))
			{
				getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY, pCity->plot());
				return true;
			}
		}
	}

	iBestValue = MAX_INT;
	pBestPlot = NULL;
	pBestPickupPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (AI_plotValid(pLoopCity->plot()))
		{
			if (pLoopCity->isCoastal(GC.getMIN_WATER_SIZE_FOR_OCEAN()))
			{
				if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
				{
					if (!atPlot(pLoopCity->plot()) && generatePath(pLoopCity->plot(), 0, true, &iPathTurns))
					{
						iValue = iPathTurns;

						if (iValue < iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = getPathEndTurnPlot();
							pBestPickupPlot = pLoopCity->plot();
							FAssert(!atPlot(pBestPlot));
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestPickupPlot != NULL))
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_ATTACK_SPY, pBestPickupPlot);
		return true;
	}

	return false;
}
*/
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


// Returns true if a mission was pushed...
bool CvUnitAI::AI_chokeDefend()
{
	FAssert(AI_isCityAIType());

	// XXX what about amphib invasions?

	CvCity* pCity = plot()->getPlotCity();
	if (pCity != NULL)
	{
		if (pCity->getOwnerINLINE() == getOwnerINLINE())
		{
			if (pCity->AI_neededDefenders() > 1)
			{
				if (pCity->AI_isDefended(pCity->plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isNotCityAIType)))
				{
					int iPlotDanger = GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 3);
					if (iPlotDanger <= 4)
					{
						//if (AI_anyAttack(1, 65, 0, std::max(0, (iPlotDanger - 1))))
						if (AI_anyAttack(1, 65, 0, iPlotDanger > 1 ? 2 : 0)) // K-Mod
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_heal(int iDamagePercent, int iMaxPath)
{
	PROFILE_FUNC();
	
	if (plot()->getFeatureType() != NO_FEATURE)
	{
		if (GC.getFeatureInfo(plot()->getFeatureType()).getTurnDamage() != 0)
		{
			//Pass through
			//(actively seeking a safe spot may result in unit getting stuck)
			return false;
		}
	}
	
	CvSelectionGroup* pGroup = getGroup();
	
	if (iDamagePercent == 0)
	{
	    iDamagePercent = 10;	    
	}	

	// <advc.306>
	// isAlwaysHeal check moved up
    if (getGroup()->getNumUnits() == 1 && !isAlwaysHeal() && getDamage() > 0)
	{
		bool bHeal = false;
		if(plot()->isCity() || healTurns(plot()) == 1) // Like in BtS
			bHeal = true;
		else if(isBarbarian()) {
			int iHeal = healRate(plot());
			double div = (getDomainType() == DOMAIN_SEA ? 17.5 : 22.5);
			if(iHeal >= 5 && ::bernoulliSuccess(iHeal / div, "advc.306 (heal)"))
				bHeal = true;
		}
		if(bHeal) {
			getGroup()->pushMission(MISSION_HEAL, -1, -1, 0, false, false, MISSIONAI_HEAL);
			return true;
		} // </advc.306>
        return false;
	}
	
	iMaxPath = std::min(iMaxPath, 2);

	CLLNode<IDInfo>* pEntityNode = getGroup()->headUnitNode();

    int iTotalDamage = 0;
    int iTotalHitpoints = 0;
    int iHurtUnitCount = 0;
	std::vector<CvUnit*> aeDamagedUnits;
	while (pEntityNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pEntityNode->m_data);
		FAssert(pLoopUnit != NULL);
		pEntityNode = pGroup->nextUnitNode(pEntityNode);

		int iDamageThreshold = (pLoopUnit->maxHitPoints() * iDamagePercent) / 100;

		if (NO_UNIT != getLeaderUnitType())
		{
			iDamageThreshold /= 2;
		}
		
		if (pLoopUnit->getDamage() > 0)
		{
		    iHurtUnitCount++;
		}
		iTotalDamage += pLoopUnit->getDamage();
		iTotalHitpoints += pLoopUnit->maxHitPoints();
		

		if (pLoopUnit->getDamage() > iDamageThreshold)
		{
			if (!pLoopUnit->hasMoved())
			{
				if (!pLoopUnit->isAlwaysHeal())
				{
					if (pLoopUnit->healTurns(pLoopUnit->plot()) <= iMaxPath)
					{
					    aeDamagedUnits.push_back(pLoopUnit);
					}
				}
			}
		}
	}
	if (iHurtUnitCount == 0)
	{
	    return false;
	}
	
	bool bPushedMission = false;
    if (plot()->isCity() && (plot()->getOwnerINLINE() == getOwnerINLINE()))
	{
		FAssertMsg(((int) aeDamagedUnits.size()) <= iHurtUnitCount, "damaged units array is larger than our hurt unit count");

		for (unsigned int iI = 0; iI < aeDamagedUnits.size(); iI++)
		{
			CvUnit* pUnitToHeal = aeDamagedUnits[iI];
			pUnitToHeal->joinGroup(NULL);
			pUnitToHeal->getGroup()->pushMission(MISSION_HEAL, -1, -1, 0, false, false, MISSIONAI_HEAL);

			// note, removing the head unit from a group will force the group to be completely split if non-human
			if (pUnitToHeal == this)
			{
				bPushedMission = true;
			}

			iHurtUnitCount--;
		}
	}
	
	if (iHurtUnitCount * 2 > pGroup->getNumUnits())
	{
		FAssertMsg(pGroup->getNumUnits() > 0, "group now has zero units");
	
	    if (AI_moveIntoCity(2))
		{
			return true;
		}
		else if (healRate(plot()) > 10)
	    {
            pGroup->pushMission(MISSION_HEAL, -1, -1, 0, false, false, MISSIONAI_HEAL);
            return true;
	    }
	}
	
	return bPushedMission;
}

/*  <advc.139> Mostly cut and pasted from AI_escortSeaMove, AI_attackSeaMove,
	AI_reserveSeaMove, AI_exploreSeaMove, AI_settlerSeaMove, AI_missionarySeaMove,
	AI_spySeaMove and AI_missileCarrierSeaMove (duplicate code).
	AI_assaultSeaMove, AI_attackAirMove, AI_defenseAirMove and AI_missileAirMove
	still contain similar code. */
bool CvUnitAI::AI_isThreatenedFromLand() const {

	FAssert(getDomainType() != DOMAIN_LAND);
	bool bDamaged = (getDamage() > 0);
	if(!bDamaged) { // Can save some computing time then
		CvCity* pPlotCity = plot()->getPlotCity();
		if(pPlotCity != NULL)
			return !pPlotCity->AI_isSafe();
	}
	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD						06/14/09	Solver & jdog5000		*/
	/* 																				*/
	/* 	Naval AI																	*/
	/********************************************************************************/
	// (with K-Mod adjustments)
	CvPlayerAI const& kOwner = GET_PLAYER(getOwnerINLINE());
	int iOurDefense = kOwner.AI_localDefenceStrength(plot(), getTeam(), DOMAIN_LAND, 0);
	int iEnemyOffense = kOwner.AI_localAttackStrength(plot(), NO_TEAM, DOMAIN_LAND, 2);
	// (was based on AI_getOurPlotStrength in BBAI)
	if(bDamaged) // extra risk to leaving when wounded
		iOurDefense *= 2;
	return (iEnemyOffense > iOurDefense/2); // (was 1 vs 1/4 in BBAI)
	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD						END									*/
	/********************************************************************************/
} // </advc.139>

// Returns true if a mission was pushed...
bool CvUnitAI::AI_afterAttack()
{
	if (!isMadeAttack())
	{
		return false;
	}
	
	if (!canFight())
	{
		return false;
	}

	if (isBlitz())
	{
		return false;
	}

	// K-Mod. Large groups may still have important stuff to do!
	if (getGroup()->getNumUnits() > 2)
		return false;
	// K-Mod end

	if (getDomainType() == DOMAIN_LAND)
	{
		if (AI_guardCity(false, true, 1))
		{
			return true;
		}

		// K-Mod. We might be able to capture an undefended city, or at least a worker. (think paratrooper)
		// (note: it's also possible that we are asking our group partner to attack something.)
		// (also, note that AI_anyAttack will favour undefended cities over workers.)
		if (AI_anyAttack(1, 65))
		{
			return true;
		}
		// K-Mod end
	}

	if (AI_pillageRange(1))
	{
		return true;
	}

	if (AI_retreatToCity(false, false, 1))
	{
		return true;
	}

	if (AI_hide())
	{
		return true;
	}

	if (AI_goody(1))
	{
		return true;
	}

	if (AI_pillageRange(2))
	{
		return true;
	}

	if (AI_defend())
	{
		return true;
	}

	if (AI_safety())
	{
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_goldenAge()
{
	if (canGoldenAge(plot()))
	{
		getGroup()->pushMission(MISSION_GOLDEN_AGE);
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
// This function has been edited for K-Mod
bool CvUnitAI::AI_spreadReligion()
{
	PROFILE_FUNC();

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod

	bool bCultureVictory = kOwner.AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2);

	ReligionTypes eReligion = NO_RELIGION;

	if (kOwner.getStateReligion() != NO_RELIGION)
	{
		if (m_pUnitInfo->getReligionSpreads(kOwner.getStateReligion()) > 0)
		{
			eReligion = kOwner.getStateReligion();
		}
	}

	if (eReligion == NO_RELIGION)
	{
		for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
		{
			//if (bCultureVictory || GET_TEAM(getTeam()).hasHolyCity((ReligionTypes)iI))
			{
				if (m_pUnitInfo->getReligionSpreads((ReligionTypes)iI) > 0)
				{
					eReligion = ((ReligionTypes)iI);
					break;
				}
			}
		}
	}

	if (eReligion == NO_RELIGION)
	{
		return false;
	}

	bool bHasHolyCity = GET_TEAM(getTeam()).hasHolyCity(eReligion);
	bool bHasAnyHolyCity = bHasHolyCity;
	if (!bHasAnyHolyCity)
	{
		for (int iI = 0; !bHasAnyHolyCity && iI < GC.getNumReligionInfos(); iI++)
		{
			bHasAnyHolyCity = GET_TEAM(getTeam()).hasHolyCity((ReligionTypes)iI);
		}
	}

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestSpreadPlot = NULL;

	// BBAI TODO: Could also use CvPlayerAI::AI_missionaryValue to determine which player to target ...
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		const CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);

		if (kLoopPlayer.isAlive())
		{
		    int iPlayerMultiplierPercent = 0;

			if (kLoopPlayer.getTeam() != getTeam() && canEnterTerritory(kLoopPlayer.getTeam()))
			{
				if (bHasHolyCity)
				{
					iPlayerMultiplierPercent = 100;
					if (!bCultureVictory || (eReligion == kOwner.getStateReligion()))
					{
						if (kLoopPlayer.getStateReligion() == NO_RELIGION)
						{
							if (0 == (kLoopPlayer.getNonStateReligionHappiness()))
							{
								iPlayerMultiplierPercent += 600;
							}
						}
						else if (kLoopPlayer.getStateReligion() == eReligion)
						{
							iPlayerMultiplierPercent += 300;
						}
						else
						{
							if (kLoopPlayer.hasHolyCity(kLoopPlayer.getStateReligion()))
							{
								iPlayerMultiplierPercent += 50;
							}
							else
							{
								iPlayerMultiplierPercent += 300;
							}
						}
						
						int iReligionCount = kLoopPlayer.countTotalHasReligion();
						//int iCityCount = kOwner.getNumCities();
						int iCityCount = kLoopPlayer.getNumCities(); // K-Mod!
						//magic formula to produce normalized adjustment factor based on religious infusion
						int iAdjustment = (100 * (iCityCount + 1));
						iAdjustment /= ((iCityCount + 1) + iReligionCount);
						iAdjustment = (((iAdjustment - 25) * 4) / 3);
						
						iAdjustment = std::max(10, iAdjustment);
						
						iPlayerMultiplierPercent *= iAdjustment;
						iPlayerMultiplierPercent /= 100;
					}
				}
			}
			else if (iI == getOwnerINLINE())
			{
				iPlayerMultiplierPercent = (bCultureVictory ? 1600 : 400) + (kOwner.getStateReligion() == eReligion ? 100 : 0);
			}
			else if (bHasHolyCity && kLoopPlayer.getTeam() == getTeam())
			{
				iPlayerMultiplierPercent = kLoopPlayer.getStateReligion() == eReligion ? 600 : 300;
			}
			
			if (iPlayerMultiplierPercent > 0)
			{
				int iLoop;
				for (CvCity* pLoopCity = kLoopPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kLoopPlayer.nextCity(&iLoop))
				{
					if (AI_plotValid(pLoopCity->plot()) && pLoopCity->area() == area())
					{
						//if (canSpread(pLoopCity->plot(), eReligion))
						if (kOwner.AI_deduceCitySite(pLoopCity) && canSpread(pLoopCity->plot(), eReligion))
						{
							if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
							{
								if (kOwner.AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_SPREAD, getGroup()) == 0)
								{
									int iPathTurns;
									if (generatePath(pLoopCity->plot(), MOVE_NO_ENEMY_TERRITORY, true, &iPathTurns))
									{
										int iValue = 16 + pLoopCity->getPopulation() * 4; // was 7 +

										iValue *= iPlayerMultiplierPercent;
										iValue /= 100;
										
										int iCityReligionCount = pLoopCity->getReligionCount();
										int iReligionCountFactor = iCityReligionCount;

										if (kLoopPlayer.getTeam() == kOwner.getTeam())
										{
											// count cities with no religion the same as cities with 2 religions
											// prefer a city with exactly 1 religion already
											if (iCityReligionCount == 0)
											{
												iReligionCountFactor = 2;
											}
											else if (iCityReligionCount == 1)
											{
												iValue *= 2;
											}
										}
										else
										{
											// absolutely prefer cities with zero religions
											if (iCityReligionCount == 0)
											{
												iValue *= 2;
											}

											// not our city, so prefer the lowest number of religions (increment so no divide by zero)
											iReligionCountFactor++;
										}

										iValue /= iReligionCountFactor;

										FAssert(iPathTurns > 0);
										
										bool bForceMove = false;
										if (isHuman())
										{
											//If human, prefer to spread to the player where automated from.
											if (plot()->getOwnerINLINE() == pLoopCity->getOwnerINLINE())
											{
												iValue *= 10;
												if (pLoopCity->isRevealed(getTeam(), false))
												{
													bForceMove = true;
												}
											}
										}

										iValue *= 1000;

										if (iPathTurns > 0)
											iValue /= (iPathTurns + 2);

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = bForceMove ? pLoopCity->plot() : getPathEndTurnPlot();
											pBestSpreadPlot = pLoopCity->plot();
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestSpreadPlot != NULL))
	{
		if (atPlot(pBestSpreadPlot))
		{
			getGroup()->pushMission(MISSION_SPREAD, eReligion);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_NO_ENEMY_TERRITORY, false, false, MISSIONAI_SPREAD, pBestSpreadPlot);
			return true;
		}
	}

	return false;
}


// K-Mod: I've basically rewritten this whole function.
// Returns true if a mission was pushed...
bool CvUnitAI::AI_spreadCorporation()
{
	PROFILE_FUNC();

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());

	CorporationTypes eCorporation = NO_CORPORATION;	

	for (int iI = 0; iI < GC.getNumCorporationInfos(); ++iI)
	{
		if (m_pUnitInfo->getCorporationSpreads((CorporationTypes)iI) > 0)
		{
			eCorporation = ((CorporationTypes)iI);
			break;
		}
	}

	//if (NO_CORPORATION == eCorporation)
	if (NO_CORPORATION == eCorporation || !kOwner.isActiveCorporation(eCorporation))
	{
		return false;
	}
	bool bHasHQ = GET_TEAM(getTeam()).hasHeadquarters(eCorporation);

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestSpreadPlot = NULL;

	// K-Mod
	// first, if we are already doing a spread mission, continue that.
	if (getGroup()->AI_getMissionAIType() == MISSIONAI_SPREAD_CORPORATION)
	{
		CvPlot* pMissionPlot = getGroup()->AI_getMissionAIPlot();
		if (pMissionPlot != NULL &&
			pMissionPlot->getPlotCity() != NULL &&
			canSpreadCorporation(pMissionPlot, eCorporation, true) && // don't check gold here
			!pMissionPlot->isVisibleEnemyUnit(this) &&
			generatePath(pMissionPlot, MOVE_NO_ENEMY_TERRITORY, true))
		{
			pBestPlot = getPathEndTurnPlot();
			pBestSpreadPlot = pMissionPlot;
		}
	}

	if (pBestSpreadPlot == NULL)
	{
		PlayerTypes eTargetPlayer = NO_PLAYER;

		if (isHuman())
			eTargetPlayer = plot()->isOwned() ? plot()->getOwnerINLINE() : getOwnerINLINE();

		if (eTargetPlayer == NO_PLAYER ||
			GET_PLAYER(eTargetPlayer).isNoCorporations() || GET_PLAYER(eTargetPlayer).isNoForeignCorporations() ||
			GET_PLAYER(eTargetPlayer).countCorporations(eCorporation, area()) >= area()->getCitiesPerPlayer(eTargetPlayer))
		{
			if (kOwner.AI_executiveValue(area(), eCorporation, &eTargetPlayer, true) <= 0)
				return false; // corp is not worth spreading in this region.
		}

		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			const CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
			if (kLoopPlayer.isAlive() && canEnterTerritory(kLoopPlayer.getTeam()) && area()->getCitiesPerPlayer((PlayerTypes)iI) > 0)
			{	// advc.003: Unused. Attitude should matter though ... tbd.
				//AttitudeTypes eAttitude = GET_TEAM(getTeam()).AI_getAttitude(kLoopPlayer.getTeam());
				if (iI == eTargetPlayer || getTeam() == kLoopPlayer.getTeam())
				{
					int iLoop;
					for (CvCity* pLoopCity = kLoopPlayer.firstCity(&iLoop); pLoopCity; pLoopCity = kLoopPlayer.nextCity(&iLoop))
					{
						if (AI_plotValid(pLoopCity->plot()) &&
							pLoopCity->getArea() == getArea() &&
							kOwner.AI_deduceCitySite(pLoopCity) &&
							canSpreadCorporation(pLoopCity->plot(), eCorporation) &&
							!pLoopCity->plot()->isVisibleEnemyUnit(this) &&
							kOwner.AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_SPREAD_CORPORATION, getGroup()) == 0)
						{
							int iPathTurns;
							if (generatePath(pLoopCity->plot(), MOVE_NO_ENEMY_TERRITORY, true, &iPathTurns))
							{
								int iValue = 0;
								// we should probably calculate the true HqValue, but I couldn't be bothered right now.
								iValue += bHasHQ ? 1000 : 0;
								if (pLoopCity->getTeam() == getTeam())
								{
									const CvPlayerAI& kCityOwner = GET_PLAYER(pLoopCity->getOwnerINLINE());
									iValue += kCityOwner.AI_corporationValue(eCorporation, pLoopCity);

									for (CorporationTypes i = (CorporationTypes)0; i < GC.getNumCorporationInfos(); i=(CorporationTypes)(i+1))
									{
										if (pLoopCity->isHasCorporation(i) && GC.getGameINLINE().isCompetingCorporation(i, eCorporation))
										{
											iValue -= kCityOwner.AI_corporationValue(i, pLoopCity) + (GET_TEAM(getTeam()).hasHeadquarters(i) ? 1100 : 100);
											// cf. iValue before AI_corporationValue is added.
										}
									}
								}
								if (iValue < 0)
									continue;

								iValue += 10 + pLoopCity->getPopulation() * 2;
								if (iI == eTargetPlayer)
									iValue *= 2;

								iValue *= 1000;

								iValue /= (iPathTurns + 1);

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = isHuman() ? pLoopCity->plot() : getPathEndTurnPlot();
									pBestSpreadPlot = pLoopCity->plot();
								}
							}
						}
					}
				}
			}
		}
	}
	// K-Mod end

	// (original code deleted)

	if (pBestPlot != NULL && pBestSpreadPlot != NULL)
	{
		if (atPlot(pBestSpreadPlot))
		{
			if (canSpreadCorporation(pBestSpreadPlot, eCorporation))
			{
				getGroup()->pushMission(MISSION_SPREAD_CORPORATION, eCorporation);
				return true;
			}
			//else
			else if (GET_PLAYER(getOwnerINLINE()).getGold() < spreadCorporationCost(eCorporation, pBestSpreadPlot->getPlotCity()))
			{
				// wait for more money
				getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_SPREAD_CORPORATION, pBestSpreadPlot);
				return true;
			}
			// FAssertMsg(false, "AI_spreadCorporation has taken us to a bogus pBestSpreadPlot");
			// this can happen from time to time. For example, when the player loses their only corp resources while the exec is en route.
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_NO_ENEMY_TERRITORY, false, false, MISSIONAI_SPREAD_CORPORATION, pBestSpreadPlot);
			return true;
		}
	}

	return false;
}

bool CvUnitAI::AI_spreadReligionAirlift()
{
	PROFILE_FUNC();

	int iI;

	if (getGroup()->getNumUnits() > 1)
	{
		return false;
	}

	CvCity* pCity = plot()->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (pCity->getMaxAirlift() == 0)
	{
		return false;
	}

	//bool bCultureVictory = GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_CULTURE2);
	ReligionTypes eReligion = NO_RELIGION;
	//if (eReligion == NO_RELIGION) // advc.003: redundant
	{
		if (GET_PLAYER(getOwnerINLINE()).getStateReligion() != NO_RELIGION)
		{
			if (m_pUnitInfo->getReligionSpreads(GET_PLAYER(getOwnerINLINE()).getStateReligion()) > 0)
			{
				eReligion = GET_PLAYER(getOwnerINLINE()).getStateReligion();
			}
		}
	}

	if (eReligion == NO_RELIGION)
	{
		for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
		{
			//if (bCultureVictory || GET_TEAM(getTeam()).hasHolyCity((ReligionTypes)iI))
			{
				if (m_pUnitInfo->getReligionSpreads((ReligionTypes)iI) > 0)
				{
					eReligion = ((ReligionTypes)iI);
					break;
				}
			}
		}
	}

	if (eReligion == NO_RELIGION)
	{
		return false;
	}

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.isAlive() && (getTeam() == kLoopPlayer.getTeam()))
		{
			int iLoop;
			for (CvCity* pLoopCity = kLoopPlayer.firstCity(&iLoop); NULL != pLoopCity; pLoopCity = kLoopPlayer.nextCity(&iLoop))
			{
				if (canAirliftAt(pCity->plot(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE()))
				{
					if (canSpread(pLoopCity->plot(), eReligion))
					{
						if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_SPREAD, getGroup()) == 0)
						{
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       08/04/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
							// Don't airlift where there's already one of our unit types (probably just airlifted)
							if( pLoopCity->plot()->plotCount(PUF_isUnitType, getUnitType(), -1, getOwnerINLINE()) > 0 )
							{
								continue;
							}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
							int iValue = (7 + (pLoopCity->getPopulation() * 4));
							
							int iCityReligionCount = pLoopCity->getReligionCount();
							int iReligionCountFactor = iCityReligionCount;

							// count cities with no religion the same as cities with 2 religions
							// prefer a city with exactly 1 religion already
							if (iCityReligionCount == 0)
							{
								iReligionCountFactor = 2;
							}
							else if (iCityReligionCount == 1)
							{
								iValue *= 2;
							}
							
							iValue /= iReligionCountFactor;
							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pLoopCity->plot();
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_AIRLIFT, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_SPREAD, pBestPlot);
		return true;
	}

	return false;	
}

bool CvUnitAI::AI_spreadCorporationAirlift()
{
	PROFILE_FUNC();
	
	if (getGroup()->getNumUnits() > 1)
	{
		return false;
	}

	CvCity* pCity = plot()->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (pCity->getMaxAirlift() == 0)
	{
		return false;
	}
	
	CorporationTypes eCorporation = NO_CORPORATION;	

	for (int iI = 0; iI < GC.getNumCorporationInfos(); ++iI)
	{
		if (m_pUnitInfo->getCorporationSpreads((CorporationTypes)iI) > 0)
		{
			eCorporation = ((CorporationTypes)iI);
			break;
		}
	}

	if (NO_CORPORATION == eCorporation)
	{
		return false;
	}

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.isAlive() && (getTeam() == kLoopPlayer.getTeam()))
		{
			int iLoop;
			for (CvCity* pLoopCity = kLoopPlayer.firstCity(&iLoop); NULL != pLoopCity; pLoopCity = kLoopPlayer.nextCity(&iLoop))
			{
				if (canAirliftAt(pCity->plot(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE()))
				{
					if (canSpreadCorporation(pLoopCity->plot(), eCorporation))
					{
						if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_SPREAD_CORPORATION, getGroup()) == 0)
						{
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       08/04/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
							// Don't airlift where there's already one of our unit types (probably just airlifted)
							if( pLoopCity->plot()->plotCount(PUF_isUnitType, getUnitType(), -1, getOwnerINLINE()) > 0 )
							{
								continue;
							}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/

							int iValue = (pLoopCity->getPopulation() * 4);

							if (pLoopCity->getOwnerINLINE() == getOwnerINLINE())
							{
								iValue *= 4;
							}
							else
							{
								iValue *= 3;
							}
							
							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pLoopCity->plot();
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_AIRLIFT, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_SPREAD, pBestPlot);
		return true;
	}

	return false;	
}
	
// Returns true if a mission was pushed...
bool CvUnitAI::AI_discover(bool bThisTurnOnly, bool bFirstResearchOnly)
{
	if(!canDiscover(plot()))
		return false;

	TechTypes eDiscoverTech = getDiscoveryTech();
	bool bFirstTech = (GET_PLAYER(getOwnerINLINE()).AI_isFirstTech(eDiscoverTech));

	if (bFirstResearchOnly && !bFirstTech)
	{
		return false;
	}

	int iPercentWasted = (100 - ((getDiscoverResearch(eDiscoverTech) * 100) / getDiscoverResearch(NO_TECH)));
	FAssert(((iPercentWasted >= 0) && (iPercentWasted <= 100)));


	if (getDiscoverResearch(eDiscoverTech) >= GET_TEAM(getTeam()).getResearchLeft(eDiscoverTech))
	{
		if ((iPercentWasted < 51) && bFirstResearchOnly && bFirstTech)
		{
			getGroup()->pushMission(MISSION_DISCOVER);
			return true;
		}

		if (iPercentWasted < (bFirstTech ? 31 : 11))
		{
			//I need a good way to assess if the tech is actually valuable...
			//but don't have one.
			getGroup()->pushMission(MISSION_DISCOVER);
			return true;
		}
	}
	else if (bThisTurnOnly)
	{
		return false;
	}

	if (iPercentWasted <= 11)
	{
		if (GET_PLAYER(getOwnerINLINE()).getCurrentResearch() == eDiscoverTech)
		{
			getGroup()->pushMission(MISSION_DISCOVER);
			return true;
		}
	}

	return false;
}

// Returns true if a mission was pushed...
bool CvUnitAI::AI_lead(std::vector<UnitAITypes>& aeUnitAITypes)
{
	PROFILE_FUNC();

	FAssertMsg(!isHuman(), "isHuman did not return false as expected");
	FAssertMsg(AI_getUnitAIType() != NO_UNITAI, "AI_getUnitAIType() is not expected to be equal with NO_UNITAI");
	FAssert(NO_PLAYER != getOwnerINLINE());

	CvPlayer& kOwner = GET_PLAYER(getOwnerINLINE());

	bool bNeedLeader = false;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iI);
		if (isEnemy((TeamTypes)iI))
		{
			if (kLoopTeam.countNumUnitsByArea(area()) > 0)
			{
				bNeedLeader = true;
				break;
			}
		}
	}

	CvUnit* pBestUnit = NULL;
	CvPlot* pBestPlot = NULL;

	// AI may use Warlords to create super-medic units
	CvUnit* pBestStrUnit = NULL;
	CvPlot* pBestStrPlot = NULL;

	CvUnit* pBestHealUnit = NULL;
	CvPlot* pBestHealPlot = NULL;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/14/10                                jdog5000      */
/*                                                                                              */
/* Great People AI, Unit AI                                                                     */
/************************************************************************************************/
	if (bNeedLeader)
	{
		int iBestStrength = 0;
		int iBestHealing = 0;
		int iLoop;
		for (CvUnit* pLoopUnit = kOwner.firstUnit(&iLoop); pLoopUnit; pLoopUnit = kOwner.nextUnit(&iLoop))
		{
			bool bValid = isWorldUnitClass(pLoopUnit->getUnitClassType());

			if( !bValid )
			{
				for (uint iI = 0; iI < aeUnitAITypes.size(); iI++)
				{
					if (pLoopUnit->AI_getUnitAIType() == aeUnitAITypes[iI] || NO_UNITAI == aeUnitAITypes[iI])
					{
						bValid = true;
						break;
					}
				}
			}

			if( bValid )
			{
				if (canLead(pLoopUnit->plot(), pLoopUnit->getID()) > 0)
				{
					if (AI_plotValid(pLoopUnit->plot()))
					{
						if (!(pLoopUnit->plot()->isVisibleEnemyUnit(this)))
						{
							if( pLoopUnit->combatLimit() == 100 )
							{
								if (generatePath(pLoopUnit->plot(), MOVE_AVOID_ENEMY_WEIGHT_3, true))
								{
									// pick the unit with the highest current strength
									int iCombatStrength = pLoopUnit->currCombatStr(NULL, NULL);

									iCombatStrength *= 30 + pLoopUnit->getExperience();
									iCombatStrength /= 30;

									if( GC.getUnitClassInfo(pLoopUnit->getUnitClassType()).getMaxGlobalInstances() > -1 )
									{
										iCombatStrength *= 1 + GC.getUnitClassInfo(pLoopUnit->getUnitClassType()).getMaxGlobalInstances();
										iCombatStrength /= std::max(1, GC.getUnitClassInfo(pLoopUnit->getUnitClassType()).getMaxGlobalInstances());
									}

									if (iCombatStrength > iBestStrength)
									{
										iBestStrength = iCombatStrength;
										pBestStrUnit = pLoopUnit;
										pBestStrPlot = getPathEndTurnPlot();
									}
									
									// or the unit with the best healing ability
									int iHealing = pLoopUnit->getSameTileHeal() + pLoopUnit->getAdjacentTileHeal();
									if (iHealing > iBestHealing)
									{
										iBestHealing = iHealing;
										pBestHealUnit = pLoopUnit;
										pBestHealPlot = getPathEndTurnPlot();
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if( AI_getBirthmark() % 3 == 0 && pBestHealUnit != NULL )
	{
		pBestPlot = pBestHealPlot;
		pBestUnit = pBestHealUnit;
	}
	else
	{
		pBestPlot = pBestStrPlot;
		pBestUnit = pBestStrUnit;
	}

	if (pBestPlot)
	{
		if (atPlot(pBestPlot) && pBestUnit)
		{
			if( gUnitLogLevel > 2 )
			{
				CvWString szString;
				getUnitAIString(szString, pBestUnit->AI_getUnitAIType());

				logBBAI("      Great general %d for %S chooses to lead %S with UNITAI %S", getID(), GET_PLAYER(getOwnerINLINE()).getCivilizationDescription(0), pBestUnit->getName(0).GetCString(), szString.GetCString());
			}
			getGroup()->pushMission(MISSION_LEAD, pBestUnit->getID());
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_AVOID_ENEMY_WEIGHT_3);
			return true;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	return false;
}

// Returns true if a mission was pushed... 
// iMaxCounts = 1 would mean join a city if there's no existing joined GP of that type.
bool CvUnitAI::AI_join(int iMaxCount)
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	SpecialistTypes eBestSpecialist;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;
	int iCount;

	iBestValue = 0;
	pBestPlot = NULL;
	eBestSpecialist = NO_SPECIALIST;
	iCount = 0;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
		// BBAI efficiency: check same area
		if ( /* advc.030: Replacing the clause below (although it won't
				make a functional difference) */
				canEnterArea(*pLoopCity->area())
				//(pLoopCity->area() == area())
				&& AI_plotValid(pLoopCity->plot()))
		{
			if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
			{
				if (generatePath(pLoopCity->plot(), MOVE_SAFE_TERRITORY, true))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				{
					for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
					{
						bool bDoesJoin = false;
						SpecialistTypes eSpecialist = (SpecialistTypes)iI;
						if (m_pUnitInfo->getGreatPeoples(eSpecialist))
						{
							bDoesJoin = true;
						}
						if (bDoesJoin)
						{
							iCount += pLoopCity->getSpecialistCount(eSpecialist);
							if (iCount >= iMaxCount)
							{
								return false;
							}
						}
						
						if (canJoin(pLoopCity->plot(), ((SpecialistTypes)iI)))
						{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
							//if (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(pLoopCity->plot(), 2) == 0)
							if ( !(GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(pLoopCity->plot(), 2)) )
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
							{
								//iValue = pLoopCity->AI_specialistValue(((SpecialistTypes)iI), pLoopCity->AI_avoidGrowth(), false);
								iValue = pLoopCity->AI_permanentSpecialistValue((SpecialistTypes)iI); // K-Mod
								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = getPathEndTurnPlot();
									eBestSpecialist = ((SpecialistTypes)iI);
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (eBestSpecialist != NO_SPECIALIST))
	{
		if (atPlot(pBestPlot))
		{
			getGroup()->pushMission(MISSION_JOIN, eBestSpecialist);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/09/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_SAFE_TERRITORY);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			return true;
		}
	}

	return false;
}

// Returns true if a mission was pushed... 
// iMaxCount = 1 would mean construct only if there are no existing buildings 
//   constructed by this GP type.
bool CvUnitAI::AI_construct(int iMaxCount, int iMaxSingleBuildingCount, int iThreshold)
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	CvPlot* pBestConstructPlot;
	BuildingTypes eBestBuilding;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;
	int iCount;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestConstructPlot = NULL;
	eBestBuilding = NO_BUILDING;
	iCount = 0;
	
	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (AI_plotValid(pLoopCity->plot()) && pLoopCity->area() == area())
		{
			if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
			{
				//if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_CONSTRUCT, getGroup()) == 0)
				// above line disabled by K-Mod, because there are different types of buildings to construct...
				{
					for (iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
					{
						BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI);

						if (NO_BUILDING != eBuilding)
						{
							bool bDoesBuild = false;
							if ((m_pUnitInfo->getForceBuildings(eBuilding))
								|| (m_pUnitInfo->getBuildings(eBuilding)))
							{
								bDoesBuild = true;
							}
								
							if (bDoesBuild && (pLoopCity->getNumBuilding(eBuilding) > 0))
							{
								iCount++;
								if (iCount >= iMaxCount)
								{
									return false;
								}
							}
								
							if (bDoesBuild && GET_PLAYER(getOwnerINLINE()).getBuildingClassCount((BuildingClassTypes)GC.getBuildingInfo(eBuilding).getBuildingClassType()) < iMaxSingleBuildingCount)
							{
								//if (canConstruct(pLoopCity->plot(), eBuilding))
								if (canConstruct(pLoopCity->plot(), eBuilding) && generatePath(pLoopCity->plot(), MOVE_NO_ENEMY_TERRITORY, true))
								{
									iValue = pLoopCity->AI_buildingValue(eBuilding);

									if ((iValue > iThreshold) && (iValue > iBestValue))
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
										pBestConstructPlot = pLoopCity->plot();
										eBestBuilding = eBuilding;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestConstructPlot != NULL) && (eBestBuilding != NO_BUILDING))
	{
		if (atPlot(pBestConstructPlot))
		{
			getGroup()->pushMission(MISSION_CONSTRUCT, eBestBuilding);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/09/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_NO_ENEMY_TERRITORY, false, false, MISSIONAI_CONSTRUCT, pBestConstructPlot);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
// advc.003j: Obsoleted by K-Mod (AI_greatPersonMove)
#if 0
bool CvUnitAI::AI_switchHurry()
{
	CvCity* pCity = plot()->getPlotCity();

	if ((pCity == NULL) || (pCity->getOwnerINLINE() != getOwnerINLINE()))
	{
		return false;
	}

	int iBestValue = 0;
	BuildingTypes eBestBuilding = NO_BUILDING;

	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		if (isWorldWonderClass((BuildingClassTypes)iI))
		{
			BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI);

			if (NO_BUILDING != eBuilding)
			{
				if (pCity->canConstruct(eBuilding))
				{
					if (pCity->getBuildingProduction(eBuilding) == 0)
					{
						if (getMaxHurryProduction(pCity) >= pCity->getProductionNeeded(eBuilding))
						{
							int iValue = pCity->AI_buildingValue(eBuilding);

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								eBestBuilding = eBuilding;
							}
						}
					}
				}
			}
		}
	}

	if (eBestBuilding != NO_BUILDING)
	{
		pCity->pushOrder(ORDER_CONSTRUCT, eBestBuilding);

		if (pCity->getProductionBuilding() == eBestBuilding)
		{
			if (canHurry(plot()))
			{
				getGroup()->pushMission(MISSION_HURRY);
				return true;
			}
		}

		FAssert(false);
	}

	return false;
}

// Returns true if a mission was pushed...
bool CvUnitAI::AI_hurry()
{
	PROFILE_FUNC();

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestHurryPlot = NULL;

	int iLoop;
	for (CvCity* pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
		// BBAI efficiency: check same area
		if ((pLoopCity->area() == area()) && AI_plotValid(pLoopCity->plot()))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		{
			if ( canHurry(pLoopCity->plot()))
			{
				if (!pLoopCity->plot()->isVisibleEnemyUnit(this))
				{
					if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_HURRY, getGroup()) == 0)
					{
						int iPathTurns;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/03/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
						if (generatePath(pLoopCity->plot(), MOVE_NO_ENEMY_TERRITORY, true, &iPathTurns))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
						{
							bool bHurry = false;

							if (pLoopCity->isProductionBuilding())
							{
								if (isWorldWonderClass((BuildingClassTypes)(GC.getBuildingInfo(pLoopCity->getProductionBuilding()).getBuildingClassType())))
								{
									bHurry = true;
								}
							}

							if (bHurry)
							{
								int iTurnsLeft = pLoopCity->getProductionTurnsLeft();

								iTurnsLeft -= iPathTurns;

								if (iTurnsLeft > 8)
								{
									int iValue = iTurnsLeft;

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
										pBestHurryPlot = pLoopCity->plot();
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestHurryPlot != NULL))
	{
		if (atPlot(pBestHurryPlot))
		{
			getGroup()->pushMission(MISSION_HURRY);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/09/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_NO_ENEMY_TERRITORY, false, false, MISSIONAI_HURRY, pBestHurryPlot);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			return true;
		}
	}

	return false;
}
#endif // advc.003j

// Returns true if a mission was pushed...
/* disabled by K-Mod. obsolete.
bool CvUnitAI::AI_greatWork()
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	CvPlot* pBestGreatWorkPlot;
	int iValue;
	int iBestValue;
	int iLoop;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestGreatWorkPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		// BBAI efficiency: check same area
		if ((pLoopCity->area() == area()) && AI_plotValid(pLoopCity->plot()))
		{
			if (canGreatWork(pLoopCity->plot()))
			{
				if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
				{
					if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_GREAT_WORK, getGroup()) == 0)
					{
						if (generatePath(pLoopCity->plot(), MOVE_NO_ENEMY_TERRITORY, true))
						{
							iValue = pLoopCity->AI_calculateCulturePressure(true);
							iValue -= ((100 * pLoopCity->getCulture(pLoopCity->getOwnerINLINE())) / std::max(1, getGreatWorkCulture(pLoopCity->plot())));
							if (iValue > 0)
							{
								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = getPathEndTurnPlot();
									pBestGreatWorkPlot = pLoopCity->plot();
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestGreatWorkPlot != NULL))
	{
		if (atPlot(pBestGreatWorkPlot))
		{
			getGroup()->pushMission(MISSION_GREAT_WORK);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_NO_ENEMY_TERRITORY, false, false, MISSIONAI_GREAT_WORK, pBestGreatWorkPlot);
			return true;
		}
	}

	return false;
} */


// Returns true if a mission was pushed...
bool CvUnitAI::AI_offensiveAirlift()
{
	PROFILE_FUNC();

	CvCity* pCity;
	CvCity* pTargetCity;
	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	int iValue;
	int iBestValue;
	int iLoop;

	if (getGroup()->getNumUnits() > 1)
	{
		return false;
	}

	if (area()->getTargetCity(getOwnerINLINE()) != NULL)
	{
		return false;
	}

	pCity = plot()->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (pCity->getMaxAirlift() == 0)
	{
		return false;
	}

	iBestValue = 0;
	pBestPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (pLoopCity->area() != pCity->area())
		{
			if (canAirliftAt(pCity->plot(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE()))
			{
				pTargetCity = pLoopCity->area()->getTargetCity(getOwnerINLINE());

				if (pTargetCity != NULL)
				{
					/* AreaAITypes eAreaAIType = pTargetCity->area()->getAreaAIType(getTeam());
					if (((eAreaAIType == AREAAI_OFFENSIVE) || (eAreaAIType == AREAAI_DEFENSIVE) || (eAreaAIType == AREAAI_MASSING))
						|| pTargetCity->AI_isDanger()) */
					if (GET_PLAYER(getOwnerINLINE()).AI_isLandWar(pTargetCity->area()) || pTargetCity->AI_isDanger()) // K-Mod
					{
						iValue = 10000;

						iValue *= (GET_PLAYER(getOwnerINLINE()).AI_militaryWeight(pLoopCity->area()) + 10);
						iValue /= (GET_PLAYER(getOwnerINLINE()).AI_totalAreaUnitAIs(pLoopCity->area(), AI_getUnitAIType()) + 10);

						iValue += std::max(1, ((GC.getMapINLINE().maxStepDistance() * 2) - GC.getMapINLINE().calculatePathDistance(pLoopCity->plot(), pTargetCity->plot())));
						
						if (AI_getUnitAIType() == UNITAI_PARADROP)
						{
							CvCity* pNearestEnemyCity = GC.getMapINLINE().findCity(pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), NO_PLAYER, NO_TEAM, false, false, getTeam());

							if (pNearestEnemyCity != NULL)
							{
								int iDistance = plotDistance(pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), pNearestEnemyCity->getX_INLINE(), pNearestEnemyCity->getY_INLINE());
								if (iDistance <= getDropRange())
								{
									iValue *= 5;
								}
							}
						}

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopCity->plot();
							FAssert(pLoopCity != pCity);
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_AIRLIFT, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_paradrop(int iRange)
{
	PROFILE_FUNC();

	if (getGroup()->getNumUnits() > 1)
	{
		return false;
	}
	int iParatrooperCount = plot()->plotCount(PUF_isUnitAIType, UNITAI_PARADROP, -1, getOwnerINLINE());
	FAssert(iParatrooperCount > 0);

	CvPlot* pPlot = plot();

	if (!canParadrop(pPlot))
	{
		return false;
	}

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;

	int iSearchRange = AI_searchRange(iRange);

	for (int iDX = -iSearchRange; iDX <= iSearchRange; ++iDX)
	{
		for (int iDY = -iSearchRange; iDY <= iSearchRange; ++iDY)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (isPotentialEnemy(pLoopPlot->getTeam(), pLoopPlot))
				{
					if (canParadropAt(pPlot, pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
					{
						int iValue = 0;

						PlayerTypes eTargetPlayer = pLoopPlot->getOwnerINLINE();
						FAssert(NO_PLAYER != eTargetPlayer);
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       08/01/08                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original BTS code
						if (NO_BONUS != pLoopPlot->getBonusType())
						{
							iValue += GET_PLAYER(eTargetPlayer).AI_bonusVal(pLoopPlot->getBonusType()) - 10;
						}
*/
						// Bonus values for bonuses the AI has are less than 10 for non-strategic resources... since this is
						// in the AI territory they probably have it
						if (NO_BONUS != pLoopPlot->getNonObsoleteBonusType(getTeam()))
						{
							iValue += std::max(1,GET_PLAYER(eTargetPlayer).AI_bonusVal(pLoopPlot->getBonusType(), 0) - 10);
						}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/

						for (int i = -1; i <= 1; ++i)
						{
							for (int j = -1; j <= 1; ++j)
							{
								CvPlot* pAdjacentPlot = plotXY(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), i, j);
								if (NULL != pAdjacentPlot)
								{
									CvCity* pAdjacentCity = pAdjacentPlot->getPlotCity();

									if (NULL != pAdjacentCity)
									{
										if (pAdjacentCity->getOwnerINLINE() == eTargetPlayer)
										{
											int iAttackerCount = GET_PLAYER(getOwnerINLINE()).AI_adjacentPotentialAttackers(pAdjacentPlot, true);
											int iDefenderCount = pAdjacentPlot->getNumVisibleEnemyDefenders(this);
											iValue += 20 * (AI_attackOdds(pAdjacentPlot, true) - ((50 * iDefenderCount) / (iParatrooperCount + iAttackerCount)));
										}
									}
								}
							}
						}
						
						if (iValue > 0)
						{
							/*  advc.012: Whether our unit ignores building defense
								(i.e.  is a gunpowder unit) shouldn't matter b/c we
								can't drop into an enemy city. */
							iValue += AI_plotDefense(pLoopPlot);
							//iValue += pLoopPlot->defenseModifier(getTeam(), ignoreBuildingDefense());

							CvUnit* pInterceptor = bestInterceptor(pLoopPlot);
							if (NULL != pInterceptor)
							{
								int iInterceptProb = isSuicide() ? 100 : pInterceptor->currInterceptionProbability();

								iInterceptProb *= std::max(0, (100 - evasionProbability()));
								iInterceptProb /= 100;

								iValue *= std::max(0, 100 - iInterceptProb / 2);
								iValue /= 100;
							}
						}

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopPlot;

							FAssert(pBestPlot != pPlot);
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_PARADROP, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}

#if 0
/*  advc.003j: This function was apparently replaced by K-Mod's
	AI_defendTerritory.
	The iFlags and iMaxPathTurns parameters of AI_protect were switched in
	this file; I've corrected this although the function is unused; also in
	the commented-out code at the call locations. */
// Returns true if a mission was pushed...
bool CvUnitAI::AI_protect(int iOddsThreshold, int iFlags, int iMaxPathTurns)
{
	PROFILE_FUNC();

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;

	for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (pLoopPlot->getOwnerINLINE() == getOwnerINLINE())
		{
			if (AI_plotValid(pLoopPlot))
			{
				if (pLoopPlot->isVisibleEnemyUnit(this))
				{		
					if (!atPlot(pLoopPlot)) 
					{
						// BBAI efficiency: Check area for land units
						if( (getDomainType() != DOMAIN_LAND) || (pLoopPlot->area() == area()) || getGroup()->canMoveAllTerrain() )
						{
							// BBAI efficiency: Most of the time, path will exist and odds will be checked anyway.  When path doesn't exist, checking path
							// takes longer.  Therefore, check odds first.
							//iValue = getGroup()->AI_attackOdds(pLoopPlot, true);
							int iValue = AI_getWeightedOdds(pLoopPlot, false); // K-Mod
							BonusTypes eBonus = pLoopPlot->getNonObsoleteBonusType(getTeam()); // K-Mod

							//if ((iValue >= AI_finalOddsThreshold(pLoopPlot, iOddsThreshold)) && (iValue*50 > iBestValue))
							if (iValue >= iOddsThreshold && (eBonus != NO_BONUS || iValue*50 > iBestValue)) // K-Mod
							{
								int iPathTurns;
								if( generatePath(pLoopPlot, iFlags, true, &iPathTurns, iMaxPathTurns) )
								{
									// BBAI TODO: Other units targeting this already (if path turns > 1 or 0)?
									if( iPathTurns <= iMaxPathTurns )
									{
										iValue *= 100;

										iValue /= (2 + iPathTurns);

										// K-Mod
										if (eBonus != NO_BONUS)
										{
											iValue *= 10 + GET_PLAYER(getOwnerINLINE()).AI_bonusVal(eBonus, 0);
											iValue /= 10;
										}
										// K-Mod end
									
										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = getPathEndTurnPlot();
											FAssert(!atPlot(pBestPlot));
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), iFlags);
		return true;
	}

	return false;
}
#endif // advc.003j

// Returns true if a mission was pushed...
bool CvUnitAI::AI_patrol()
{
	PROFILE_FUNC();

	CvPlot* pAdjacentPlot;
	CvPlot* pBestPlot;
	int iValue;
	int iBestValue;
	int iI;

	iBestValue = 0;
	pBestPlot = NULL;
	// <advc.102>
	CvPlot* facedPlot = plotDirection(getX_INLINE(), getY_INLINE(),
			getFacingDirection(true));
	int facedX = -100, facedY = -100;
	if(facedPlot != NULL) {
		facedX = facedPlot->getX_INLINE();
		facedY = facedPlot->getY_INLINE();
	} // </advc.102>

	for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		pAdjacentPlot = plotDirection(getX_INLINE(), getY_INLINE(), ((DirectionTypes)iI));

		if (pAdjacentPlot != NULL)
		{
			if (AI_plotValid(pAdjacentPlot))
			{
				if (!(pAdjacentPlot->isVisibleEnemyUnit(this)))
				{
					if (generatePath(pAdjacentPlot, 0, true))
					{
						//  advc.001: generatePath doesn't seem to rule this out.
						if(atPlot(getPathEndTurnPlot())) continue;
						iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Patrol"));
						// <advc.309>
						if(isAnimal()) {
							CvPlot& p = *pAdjacentPlot;
							CvUnitInfo& u = *m_pUnitInfo;
							// Same check as in CvGame::createAnimals
							if((p.getFeatureType() != NO_FEATURE) ? 
									u.getFeatureNative(p.getFeatureType()) :
									u.getTerrainNative(p.getTerrainType()))
								iValue += GC.getGameINLINE().getSorenRandNum(
										10000, "advc.309");
						} else { // Animals shouldn't follow a consistent direction
						// </advc.309>
							/*  <advc.102> Prefer facedPlot or a plot that's
								orthogonally adjacent to facedPlot. */
							int x = pAdjacentPlot->getX_INLINE();
							int y = pAdjacentPlot->getY_INLINE();
							int delta = ::abs(facedX - x) + ::abs(facedY - y);
							if(delta <= 1)
								iValue += GC.getGameINLINE().getSorenRandNum(
										10000, "advc.102");
							/*  Prefer to stay/ get out of foreign borders: could be
								human borders, and AI patrols inside human borders
								are annoying when "Show ...Moves" is enabled. */
							PlayerTypes adjOwner = pAdjacentPlot->getOwnerINLINE();
							if(adjOwner != getOwnerINLINE() && adjOwner != NO_PLAYER)
								iValue -= 4000;
							// </advc.102>
						}
						if (isBarbarian()
							/*  advc.306: Patrolling barb ships should pretty much
								move about randomly, neither seeking nor avoiding
								owned tiles. */
							&& !plot()->isWater()
							)
						{
							if (!(pAdjacentPlot->isOwned()))
							{
								iValue += 20000;
							}

							if (!(pAdjacentPlot->isAdjacentOwned()))
							{
								iValue += 10000;
							}
						}
						else
						{
							if (pAdjacentPlot->isRevealedGoody(getTeam()))
							{
								iValue += 100000;
							}

							/* advc.102: Don't prefer own tiles
							if (pAdjacentPlot->getOwnerINLINE() == getOwnerINLINE())
							{
								iValue += 10000;
							}*/
						}

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = getPathEndTurnPlot();
							FAssert(!atPlot(pBestPlot));
						}
					}
				}
			}
		}
	}

	if(pBestPlot != NULL
		/*  <advc.102> Non-barb AI should only patrol tiles it doesn't own
			(owned tiles have visibility anyway). In order to get to
			unowned tiles, however, units may have to traverse owned tiles,
			so, they need to be allowed to patrol into owned tiles at least
			under some circumstances. */
			&& (pBestPlot->getOwner() != getOwner() || isBarbarian() ||
			// Make sure not to hamper early exploration (perhaps not an issue)
			GC.getGameINLINE().getElapsedGameTurns() < 25 ||
			(::bernoulliSuccess(0.1, "advc.102") && !pBestPlot->isUnit()))) {
			// </advc.102>
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_PATROL);
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_defend()
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	if (AI_defendPlot(plot()))
	{
		getGroup()->pushMission(MISSION_SKIP);
		return true;
	}

	iSearchRange = AI_searchRange(1);

	iBestValue = 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot))
				{
					if (AI_defendPlot(pLoopPlot))
					{
						if (!(pLoopPlot->isVisibleEnemyUnit(this)))
						{
							if (!atPlot(pLoopPlot) && generatePath(pLoopPlot, 0, true, &iPathTurns, 1))
							{
								if (iPathTurns <= 1)
								{
									iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Defend"));

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = pLoopPlot;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/06/08                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
		if( !(pBestPlot->isCity()) && (getGroup()->getNumUnits() > 1) )
		{
			//getGroup()->AI_makeForceSeparate();
			joinGroup(0); // K-Mod. (AI_makeForceSeparate is a complete waste of time here.)
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_DEFEND);
		return true;
	}

	return false;
}


// This function has been edited for K-Mod
bool CvUnitAI::AI_safety()
{
	PROFILE_FUNC();

	int iSearchRange = AI_searchRange(1);

	int iBestValue = 0;
	CvPlot* pBestPlot = 0;
	bool bEnemyTerritory = isEnemy(plot()->getTeam());
	bool bIgnoreDanger = false;

	//for (iPass = 0; iPass < 2; iPass++)
	do // K-Mod. What's the point of the first pass if it is just ignored? (see break condition at the end)
	{
		for (int iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
		{
			for (int iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
			{
				CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

				if (pLoopPlot && AI_plotValid(pLoopPlot) && !pLoopPlot->isVisibleEnemyUnit(this))
				{
					/*  <advc.306> Consider only unobserved plots (and cities) safe
						for Barbarian sea units. This should help them receive spawned
						cargo. */
					if(isBarbarian() && pLoopPlot->isWater() &&
							pLoopPlot->isVisibleToCivTeam())
						continue; // </advc.306>
					int iPathTurns;
					if (generatePath(pLoopPlot, bIgnoreDanger ? MOVE_IGNORE_DANGER : 0, true, &iPathTurns, 1))
					{
						int iCount = 0;

						CLLNode<IDInfo>* pUnitNode = pLoopPlot->headUnitNode();

						while (pUnitNode != NULL)
						{
							CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
							pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

							if (pLoopUnit->getOwnerINLINE() == getOwnerINLINE())
							{
								if (pLoopUnit->canDefend())
								{
									CvUnit* pHeadUnit = pLoopUnit->getGroup()->getHeadUnit();
									FAssert(pHeadUnit != NULL);
									FAssert(getGroup()->getHeadUnit() == this);

									if (pHeadUnit != this)
									{
										if (pHeadUnit->isWaiting() || !(pHeadUnit->canMove()))
										{
											FAssert(pLoopUnit != this);
											FAssert(pHeadUnit != getGroup()->getHeadUnit());
											iCount++;
										}
									}
								}
							}
						}

						int iValue = (iCount * 100);
						iValue += AI_plotDefense(pLoopPlot); // advc.012
						//iValue += pLoopPlot->defenseModifier(getTeam(), false);

						// K-Mod
						iValue += (bEnemyTerritory ? !isEnemy(pLoopPlot->getTeam(), pLoopPlot) : pLoopPlot->getTeam() == getTeam()) ? 30 : 0;
						iValue += pLoopPlot->isValidRoute(this, false /* advc.001i */)
								? 25 : 0;
						// K-Mod end

						if (atPlot(pLoopPlot))
						{
							iValue += 50;
						}
						else
						{
							iValue += GC.getGameINLINE().getSorenRandNum(50, "AI Safety");
						}

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopPlot;
						}
					}
				}
			}
		}
		// K-Mod
		if (!pBestPlot)
		{
			if (bIgnoreDanger)
				break; // no suitable plot, even when ignoring danger
			else
				bIgnoreDanger = true; // try harder next time
		}
		// K-Mod end
	} while (!pBestPlot);

	if (pBestPlot != NULL)
	{
		if (atPlot(pBestPlot))
		{
			getGroup()->pushMission(MISSION_SKIP);
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), bIgnoreDanger ? MOVE_IGNORE_DANGER : 0);
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_hide()
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvUnit* pHeadUnit;
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	bool bValid;
	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iCount;
	int iDX, iDY;
	int iI;

	if (getInvisibleType() == NO_INVISIBLE)
	{
		return false;
	}

	iSearchRange = AI_searchRange(1);

	iBestValue = 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot))
				{
					bValid = true;

					for (iI = 0; iI < MAX_TEAMS; iI++)
					{
						if (GET_TEAM((TeamTypes)iI).isAlive())
						{
							if (pLoopPlot->isInvisibleVisible(((TeamTypes)iI), getInvisibleType()))
							{
								bValid = false;
								break;
							}
						}
					}

					if (bValid)
					{
						if (!(pLoopPlot->isVisibleEnemyUnit(this)))
						{
							if (generatePath(pLoopPlot, 0, true, &iPathTurns, 1))
							{
								if (iPathTurns <= 1)
								{
									iCount = 1;

									pUnitNode = pLoopPlot->headUnitNode();

									while (pUnitNode != NULL)
									{
										pLoopUnit = ::getUnit(pUnitNode->m_data);
										pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

										if (pLoopUnit->getOwnerINLINE() == getOwnerINLINE())
										{
											if (pLoopUnit->canDefend())
											{
												pHeadUnit = pLoopUnit->getGroup()->getHeadUnit();
												FAssert(pHeadUnit != NULL);
												FAssert(getGroup()->getHeadUnit() == this);

												if (pHeadUnit != this)
												{
													if (pHeadUnit->isWaiting() || !(pHeadUnit->canMove()))
													{
														FAssert(pLoopUnit != this);
														FAssert(pHeadUnit != getGroup()->getHeadUnit());
														iCount++;
													}
												}
											}
										}
									}

									iValue = (iCount * 100);
									iValue += AI_plotDefense(pLoopPlot); // advc.012
									//iValue += pLoopPlot->defenseModifier(getTeam(), false);

									if (atPlot(pLoopPlot))
									{
										iValue += 50;
									}
									else
									{
										iValue += GC.getGameINLINE().getSorenRandNum(50, "AI Hide");
									}

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = pLoopPlot;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		if (atPlot(pBestPlot))
		{
			getGroup()->pushMission(MISSION_SKIP);
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_goody(int iRange)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
//	CvPlot* pAdjacentPlot;
	CvPlot* pBestPlot;
	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;
//	int iI;

	if (isBarbarian())
	{
		return false;
	}

	iSearchRange = AI_searchRange(iRange);

	iBestValue = 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot))
				{
					if (pLoopPlot->isRevealedGoody(getTeam()))
					{
						if (!(pLoopPlot->isVisibleEnemyUnit(this)))
						{
							if (!atPlot(pLoopPlot) && generatePath(pLoopPlot, 0, true, &iPathTurns, iRange))
							{
								if (iPathTurns <= iRange)
								{
									iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Goody"));

									iValue /= (iPathTurns + 1);

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_explore()
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pAdjacentPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestExplorePlot;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iI, iJ;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestExplorePlot = NULL;
	
	bool bNoContact = (GC.getGameINLINE().countCivTeamsAlive() > GET_TEAM(getTeam()).getHasMetCivCount(true));
	const CvTeam& kTeam = GET_TEAM(getTeam()); // K-Mod

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		PROFILE("AI_explore 1");

		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot))
		{
			iValue = 0;

			if (pLoopPlot->isRevealedGoody(getTeam()))
			{
				iValue += 100000;
			}

			if (iValue > 0 || GC.getGameINLINE().getSorenRandNum(4, "AI make explore faster ;)") == 0)
			{
				if (!(pLoopPlot->isRevealed(getTeam(), false)))
				{
					iValue += 10000;
				}
				// XXX is this too slow?
				for (iJ = 0; iJ < NUM_DIRECTION_TYPES; iJ++)
				{
					PROFILE("AI_explore 2");

					pAdjacentPlot = plotDirection(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), ((DirectionTypes)iJ));

					if (pAdjacentPlot != NULL)
					{
						if (!(pAdjacentPlot->isRevealed(getTeam(), false)))
						{
							iValue += 1000;
						}
						/* original bts code
						else if (bNoContact)
						{
							if (pAdjacentPlot->getRevealedTeam(getTeam(), false) != pAdjacentPlot->getTeam())
							{
								iValue += 100;
							}
						} */
						// K-Mod. Not only is the original code cheating, it also doesn't help us meet anyone!
						// The goal here is to try to meet teams which we have already seen through map trading.
						if (bNoContact && pAdjacentPlot->getRevealedOwner(kTeam.getID(), false) != NO_PLAYER) // note: revealed team can be set before the plot is actually revealed.
						{
							if (!kTeam.isHasMet(pAdjacentPlot->getRevealedTeam(kTeam.getID(), false)))
								iValue += 100;
						}
						// K-Mod end
					}
				}

				if (iValue > 0)
				{
					if (!(pLoopPlot->isVisibleEnemyUnit(this)))
					{
						if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_EXPLORE, getGroup(), 3) == 0)
						{
							if (!atPlot(pLoopPlot) && generatePath(pLoopPlot, MOVE_NO_ENEMY_TERRITORY, true, &iPathTurns))
							{
								iValue += GC.getGameINLINE().getSorenRandNum(250 * abs(xDistance(getX_INLINE(), pLoopPlot->getX_INLINE())) + abs(yDistance(getY_INLINE(), pLoopPlot->getY_INLINE())), "AI explore");

								if (pLoopPlot->isAdjacentToLand())
								{
									iValue += 10000;
								}

								if (pLoopPlot->isOwned())
								{
									iValue += 5000;
								}

								iValue /= 3 + std::max(1, iPathTurns);

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = pLoopPlot->isRevealedGoody(getTeam()) ? getPathEndTurnPlot() : pLoopPlot;
									pBestExplorePlot = pLoopPlot;
								}
							}
						}
					}
				}
			}		
		}
	}

	if ((pBestPlot != NULL) && (pBestExplorePlot != NULL))
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_NO_ENEMY_TERRITORY, false, false, MISSIONAI_EXPLORE, pBestExplorePlot);
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_exploreRange(int iRange)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pAdjacentPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestExplorePlot;
	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;
	int iI;

	iSearchRange = AI_searchRange(iRange);

	iBestValue = 0;
	pBestPlot = NULL;
	pBestExplorePlot = NULL;

	int iImpassableCount = GET_PLAYER(getOwnerINLINE()).AI_unitImpassableCount(getUnitType());

	const CvTeam& kTeam = GET_TEAM(getTeam()); // K-Mod

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			PROFILE("AI_exploreRange 1");

			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot))
				{
					iValue = 0;

					if (pLoopPlot->isRevealedGoody(getTeam()))
					{
						iValue += 100000;
					}

					if (!(pLoopPlot->isRevealed(getTeam(), false)))
					{
						iValue += 10000;
					}

					// K-Mod. Try to meet teams that we have seen through map trading
					if (pLoopPlot->getRevealedOwner(kTeam.getID(), false) != NO_PLAYER && !kTeam.isHasMet(pLoopPlot->getRevealedTeam(kTeam.getID(), false)))
						iValue += 1000;
					// K-Mod end

					for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
					{
						PROFILE("AI_exploreRange 2");

						pAdjacentPlot = plotDirection(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), ((DirectionTypes)iI));

						if (pAdjacentPlot != NULL)
						{
							if (!(pAdjacentPlot->isRevealed(getTeam(), false)))
							{
								iValue += 1000;
							}
						}
					}

					if (iValue > 0)
					{
						if (!(pLoopPlot->isVisibleEnemyUnit(this)))
						{
							PROFILE("AI_exploreRange 3");

							if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_EXPLORE, getGroup(), 3) == 0)
							{
								PROFILE("AI_exploreRange 4");

								if (!atPlot(pLoopPlot) && generatePath(pLoopPlot, MOVE_NO_ENEMY_TERRITORY, true, &iPathTurns, iRange))
								{
									if (iPathTurns <= iRange)
									{
										iValue += GC.getGameINLINE().getSorenRandNum(10000, "AI Explore");

										if (pLoopPlot->isAdjacentToLand())
										{
											iValue += 10000;
										}

										if (pLoopPlot->isOwned())
										{
											iValue += 5000;
										}
										
										if (!isHuman())
										{
											int iDirectionModifier = 100;

											if (AI_getUnitAIType() == UNITAI_EXPLORE_SEA && iImpassableCount == 0)
											{
												iDirectionModifier += (50 * (abs(iDX) + abs(iDY))) / iSearchRange;
												if (GC.getGame().circumnavigationAvailable())
												{
													if (GC.getMap().isWrapX())
													{
														if ((iDX * ((AI_getBirthmark() % 2 == 0) ? 1 : -1)) > 0)
														{
															iDirectionModifier *= 150 + ((iDX * 100) / iSearchRange);
														}
														else
														{
															iDirectionModifier /= 2;
														}
													}
													if (GC.getMap().isWrapY())
													{
														if ((iDY * (((AI_getBirthmark() >> 1) % 2 == 0) ? 1 : -1)) > 0)
														{
															iDirectionModifier *= 150 + ((iDY * 100) / iSearchRange);
														}
														else
														{
															iDirectionModifier /= 2;
														}
													}
												}
												iValue *= iDirectionModifier;
												iValue /= 100;
											}
										}

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											if (getDomainType() == DOMAIN_LAND)
											{
												pBestPlot = getPathEndTurnPlot();
											}
											else
											{
												pBestPlot = pLoopPlot;
											}
											pBestExplorePlot = pLoopPlot;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestExplorePlot != NULL))
	{
		PROFILE("AI_exploreRange 5");

		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_NO_ENEMY_TERRITORY, false, false, MISSIONAI_EXPLORE, pBestExplorePlot);
		return true;
	}

	return false;
}

// Returns target city
// This function has been heavily edited for K-Mod (and I got sick of putting "K-Mod" tags all over the place)
CvCity* CvUnitAI::AI_pickTargetCity(int iFlags, int iMaxPathTurns, bool bHuntBarbs)
{
	PROFILE_FUNC();

	CvCity* pBestCity = NULL;
	int iBestValue = 0;
	// K-Mod
	CvPlayerAI const& kOwner = GET_PLAYER(getOwnerINLINE());
	int iOurOffence = -1; // We calculate this for the first city only.
	CvUnit* pBestTransport = NULL;
	// iLoadTurns < 0 implies we should look for a transport; otherwise, it is the number of turns to reach the transport.
	// Also, we only consider using transports if we aren't in enemy territory.
	int iLoadTurns = isEnemy(plot()->getTeam()) ? MAX_INT : -1;
	KmodPathFinder transport_path;
	// K-Mod end

	CvCity* pTargetCity = area()->getTargetCity(getOwnerINLINE());

	for (int iI = 0; iI < (bHuntBarbs ? MAX_PLAYERS : MAX_CIV_PLAYERS); iI++)
	{
		CvPlayer const& kTargetPlayer = GET_PLAYER((PlayerTypes)iI);
		if(!kTargetPlayer.isAlive() ||
				!::isPotentialEnemy(getTeam(), kTargetPlayer.getTeam()))
			continue; // advc.003
		int iLoop;
		for (CvCity* pLoopCity = kTargetPlayer.firstCity(&iLoop); pLoopCity != NULL;
				pLoopCity = kTargetPlayer.nextCity(&iLoop))
		{
			if(!AI_plotValid(pLoopCity->plot()) || pLoopCity->area() != area())
				continue; // advc.003
			if(!AI_potentialEnemy(kTargetPlayer.getTeam(), pLoopCity->plot()))
				continue;
			/*  <advc.300> Assault barbs shouldn't target cities in areas where
				they already have the upper hand. */
			if(isBarbarian() && pLoopCity->area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT)
				continue; // </advc.300>
			if (kOwner.AI_deduceCitySite(pLoopCity))
			{
				// K-Mod. Look for either a direct land path, or a sea transport path.
				int iPathTurns = MAX_INT;
				bool bLandPath = generatePath(pLoopCity->plot(), iFlags, true,
						&iPathTurns, iMaxPathTurns);
				if (pLoopCity->isCoastal() && (pBestTransport || iLoadTurns < 0))
				{
					// add a random bias in favour of land paths, so that not all stacks try to use boats.
					//int iLandBias =AI_getBirthmark()%6 +(AI_getBirthmark() % (bLandPath ? 3 : 6) ? 6 : 1);
					/*  <advc.003> That's equivalent to this (I've checked through
						the assertion below): */
					int iLandBias = (AI_getBirthmark() % 6) +
							(((AI_getBirthmark() % (bLandPath ? 3 : 6)) > 0 ? 6 : 1));
					//FAssert(iLandBias == (AI_getBirthmark()%6 +(AI_getBirthmark() % (bLandPath ? 3 : 6) ? 6 : 1)));
					/*  But does it make sense? It seems that the expected value is
						higher if bLand is false, namely 2.5+6*5/6+1/5 > 2.5+6*2/3+1/3.
						Should it be '==0' instead of '>0'?
						Also: "so that not all stacks try to use boats" --
						but we're only picking a target city here; the choice
						how to move there is made later. */
					// </advc.003>
					if (!pBestTransport && iPathTurns > iLandBias + 2)
					{
						pBestTransport = AI_findTransport(UNITAI_ASSAULT_SEA, iFlags,
								std::min(iMaxPathTurns, iPathTurns));
						if (pBestTransport)
						{
							generatePath(pBestTransport->plot(), iFlags, true, &iLoadTurns);
							FAssert(iLoadTurns > 0 && iLoadTurns < MAX_INT);
							iLoadTurns += iLandBias;
							FAssert(iLoadTurns > 0);
						}
						// just to indicate the we shouldn't look for a transport again.
						else iLoadTurns = MAX_INT;
					}
					int iMaxTransportTurns = std::min(iMaxPathTurns, iPathTurns) - iLoadTurns;
					if (pBestTransport && iMaxTransportTurns > 0)
					{
						transport_path.SetSettings(pBestTransport->getGroup(),
								iFlags & MOVE_DECLARE_WAR, iMaxTransportTurns,
								GC.getMOVE_DENOMINATOR());
						if (transport_path.GeneratePath(pLoopCity->plot()))
						{
							// faster by boat
							FAssert(transport_path.GetPathTurns() + iLoadTurns <= iPathTurns);
							iPathTurns = transport_path.GetPathTurns() + iLoadTurns;
						}
					}
				}

				if (iPathTurns < iMaxPathTurns)
				{
					// If city is visible and our force already in position is dominantly powerful or we have a huge force
					// already on the way, pick a different target
					int iEnemyDefence = -1; // used later.
					int iOffenceEnRoute = kOwner.AI_cityTargetStrengthByPath(
							pLoopCity, getGroup(), iPathTurns);
					if (pLoopCity->isVisible(getTeam(), false))
					{
						iEnemyDefence = kOwner.AI_localDefenceStrength(
								pLoopCity->plot(), NO_TEAM, DOMAIN_LAND, true,
								iPathTurns > 1 ? 2 : 0);
						if (iPathTurns > 2)
						{
							int iAttackRatio = ((GC.getMAX_CITY_DEFENSE_DAMAGE() -
									pLoopCity->getDefenseDamage()) *
									GC.getBBAI_SKIP_BOMBARD_BASE_STACK_RATIO() +
									pLoopCity->getDefenseDamage() *
									GC.getBBAI_SKIP_BOMBARD_MIN_STACK_RATIO()) /
									std::max(1, GC.getMAX_CITY_DEFENSE_DAMAGE());
							if (100 * iOffenceEnRoute > iAttackRatio * iEnemyDefence)
								continue;
						}
					}
					if (iOurOffence == -1)
					{
						/*  note: with bCheckCanAttack == false, AI_sumStrength should be
							roughly the same regardless of which city we are targeting.
							... except if lots of our units have a hills-attack promotion
							or something like that. */
						iOurOffence = getGroup()->AI_sumStrength(pLoopCity->plot());
					}
					FAssert(iOurOffence > 0);
					int iTotalOffence = iOurOffence + iOffenceEnRoute;

					int iValue = 0;
					if (AI_getUnitAIType() == UNITAI_ATTACK_CITY) //lemming?
					{
						iValue = kOwner.AI_targetCityValue(pLoopCity, false, false);
					}
					else
					{
						iValue = kOwner.AI_targetCityValue(pLoopCity, true, true);
					}
					// adjust value based on defensive bonuses
					{
						int iMod =
								std::min(8, getGroup()->getBombardTurns(pLoopCity)) *
								pLoopCity->getDefenseModifier(false) / 8
								+ (pLoopCity->plot()->isHills() ?
								GC.getHILLS_EXTRA_DEFENSE() : 0);
						iValue *= std::max(25, 125 - iMod);
						iValue /= 25; // the denominator is arbitrary, and unimportant.
						/*  note: the value reduction from high defences which are bombardable should not
							be more than the value reduction from simply having higher iPathTurns. */
					}
					// prefer cities which are close to the main target.
					if (pLoopCity == pTargetCity)
					{
						iValue *= 2;
					}
					else if (pTargetCity != NULL)
					{
						int iStepsFromTarget = stepDistance(
							pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(),
							pTargetCity->getX_INLINE(), pTargetCity->getY_INLINE());

						iValue *= 124 - 2*std::min(12, iStepsFromTarget);
						iValue /= 100;
					}

					if (area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE)
					{
						iValue *= 100 + pLoopCity->calculateCulturePercent(getOwnerINLINE()); // was 50
						iValue /= 125; // was 50 (unimportant)
					}

					/*  boost value if we can see that the city is poorly defended,
						or if our existing armies need help there */
					if (pLoopCity->isVisible(getTeam(), false) && iPathTurns < 6)
					{
						FAssert(iEnemyDefence != -1);
						if (iOffenceEnRoute > iEnemyDefence/3 && iOffenceEnRoute < iEnemyDefence)
						{
							iValue *= 100 + (9 * iTotalOffence > 10 * iEnemyDefence ? 30 : 15);
							iValue /= 100;
						}
						else if (iOurOffence > iEnemyDefence)
						{
							// dont' boost it by too much, otherwise human players will exploit us. :(
							int iCap = 100 + 100 * (6 - iPathTurns) / 5;
							iValue *= std::min(iCap, 100 * iOurOffence / std::max(1, iEnemyDefence));
							iValue /= 100;
							// an additional bonus if we're already adjacent
							// (we can afford to be generous with this bonus, because the enemy has no time to bring in reinforcements)
							if (iPathTurns <= 1)
							{
								iValue *= std::min(300, 150 * iOurOffence / std::max(1, iEnemyDefence));
								iValue /= 100;
							}
						}
					}
					// Reduce the value if we can see, or remember, that the city is well defended.
					// Note. This adjustment can be more heavy handed because it is harder to feign strong defence than weak defence.
					iEnemyDefence = GET_TEAM(getTeam()).AI_getStrengthMemory(pLoopCity->plot());
					if (iEnemyDefence > iTotalOffence)
					{
						// a more sensitive adjustment than usual (w/ modifier on the denominator), so as not to be too deterred before bombarding.
						iEnemyDefence *= 130;
						iEnemyDefence /= 130 + (bombardRate() > 0 ? pLoopCity->getDefenseModifier(false) : 0);
						WarPlanTypes eWarPlan = GET_TEAM(kOwner.getTeam()).AI_getWarPlan(pLoopCity->getTeam());
						// If we aren't fully committed to the war, then focus on taking easy cities - but try not to be completely predictable.
						bool bCherryPick = eWarPlan == WARPLAN_LIMITED || eWarPlan == WARPLAN_PREPARING_LIMITED || eWarPlan == WARPLAN_DOGPILE;
						bCherryPick = bCherryPick && (AI_unitBirthmarkHash(GC.getGameINLINE().getElapsedGameTurns()/4) % 4);

						int iBase = bCherryPick ? 100 : 110;
						if (100 * iEnemyDefence > iBase * iTotalOffence) // an uneven comparison, just in case we can get some air support or other help somehow.
						{
							iValue *= bCherryPick ?
									std::max(20, (3 * iBase * iTotalOffence - iEnemyDefence) / (2*iEnemyDefence)) :
									std::max(33, iBase * iTotalOffence / iEnemyDefence);
							iValue /= 100;
						}
					}
					// A const-random component, so that the AI doesn't always go for the same city.
					iValue *= 80 + AI_unitPlotHash(pLoopCity->plot()) % 41;
					iValue /= 100;

					iValue *= 1000;

					// If city is minor civ, less interesting
					if (GET_PLAYER(pLoopCity->getOwnerINLINE()).isMinorCiv() || GET_PLAYER(pLoopCity->getOwnerINLINE()).isBarbarian())
					{
						//iValue /= 2;
						iValue /= 3; // K-Mod
					}
					// If stack has poor bombard, direct towards lower defense cities
					//iPathTurns += std::min(12, getGroup()->getBombardTurns(pLoopCity)/4);
					//iPathTurns += bombardRate() > 0 ? std::min(5, getGroup()->getBombardTurns(pLoopCity)/3) : 0; // K-Mod
					// (already taken into account.)

					iValue /= 8 + iPathTurns*iPathTurns; // was 4+

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestCity = pLoopCity;
					}
				}
			} // end if revealed.
			// K-Mod. If no city in the area is revealed,
			// then assume the AI is able to deduce the position of the closest city.
			else if (iBestValue == 0 && !pLoopCity->isBarbarian() && (!pBestCity ||
				stepDistance(getX_INLINE(), getY_INLINE(), pBestCity->getX_INLINE(), pBestCity->getY_INLINE()) >
				stepDistance(getX_INLINE(), getY_INLINE(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE())))
			{
				if (generatePath(pLoopCity->plot(), iFlags, true, 0, iMaxPathTurns))
					pBestCity = pLoopCity;
			}
			// K-Mod end
		}
	}

	return pBestCity;
}

// Returns true if a mission was pushed...
bool CvUnitAI::AI_goToTargetCity(int iFlags, int iMaxPathTurns, CvCity* pTargetCity )
{
	PROFILE_FUNC();

	if( pTargetCity == NULL )
	{
		pTargetCity = AI_pickTargetCity(iFlags, iMaxPathTurns);
	}
	if(pTargetCity == NULL)
		return false;

	CvPlot* pEndTurnPlot = NULL; // K-Mod
	int iPathTurns;
	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	if (0 == (iFlags & MOVE_THROUGH_ENEMY)
		/*  advc.123b: Could also pass MOVE_THROUGH_ENEMY in AI_barbAttackMove,
			but only want barbarians to disregard defenders blocking the
			tiles around the city; no need to ignore obstacles along the
			way; hence keep using MOVE_ATTACK_STACK.
			The cleaner solution would be to address the TODO in the loop.
			(I've only seen barbarians unable to goToTargetCity, but I'm not
			positive that other units can't have that problem.) */
			&& !isBarbarian())
	{
		
		for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
		{
			CvPlot* pAdjacentPlot = plotDirection(pTargetCity->getX_INLINE(), pTargetCity->getY_INLINE(), ((DirectionTypes)iI));
			if (pAdjacentPlot != NULL)
			{
				if (AI_plotValid(pAdjacentPlot))
				{
					// K-Mod TODO: consider fighting for the best plot.
					if (!pAdjacentPlot->isVisibleEnemyUnit(this))
					{
						if (generatePath(pAdjacentPlot, iFlags, true, &iPathTurns, iMaxPathTurns))
						{
							if(iPathTurns <= iMaxPathTurns)
							{
								int iValue = std::max(0, 
										AI_plotDefense(pAdjacentPlot) // advc.012
										//pAdjacentPlot->defenseModifier(getTeam(), false)
										+ 100);

								if (!pAdjacentPlot->isRiverCrossing(directionXY(pAdjacentPlot, pTargetCity->plot())))
								{
									iValue += (12 * -(GC.getRIVER_ATTACK_MODIFIER()));
								}

								if (!isEnemy(pAdjacentPlot->getTeam(), pAdjacentPlot))
								{
									iValue += 100;                                
								}

								if( atPlot(pAdjacentPlot) )
								{
									iValue += 50;
								}

								iValue = std::max(1, iValue);

								iValue *= 1000;

								iValue /= (iPathTurns + 1);

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									//pBestPlot = getPathEndTurnPlot();
									// K-Mod
									pBestPlot = pAdjacentPlot;
									pEndTurnPlot = getPathEndTurnPlot();
									// K-Mod end
								}
							}
						}
					}
				}
			}
		}
	}
	else
	{
		pBestPlot =  pTargetCity->plot();
		// K-mod. As far as I know, nothing actually uses this flag here.. but that doesn't mean we should let the code be wrong.
		if (!generatePath(pBestPlot, iFlags, true, &iPathTurns, iMaxPathTurns) || iPathTurns > iMaxPathTurns)
			return false;
		pEndTurnPlot = getPathEndTurnPlot();
		// K-mod end
	}

	if (pBestPlot != NULL)
	{
		FAssertMsg(
				isBarbarian() || // advc.123b
				!pTargetCity->at(pEndTurnPlot) || 0 != (iFlags & MOVE_THROUGH_ENEMY), "no suicide missions...");
		if (!atPlot(pEndTurnPlot))
		{	/*  <advc.001t> Needed for the call in AI_attackMove. These stacks
				aren't supposed to declare war, and they shouldn't move into
				enemy cities when war is imminent. */
			if(!(iFlags & MOVE_DECLARE_WAR) && GET_TEAM(getTeam()).
					AI_isSneakAttackReady(pTargetCity->getTeam())) {
				TeamTypes eBestPlotTeam = pBestPlot->getTeam();
				if(eBestPlotTeam != NO_TEAM && GET_TEAM(eBestPlotTeam).
						getMasterTeam() == GET_TEAM(pTargetCity->getTeam()).
						getMasterTeam())
					return false;
			} // </advc.001t>
			//getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), iFlags);
			// K-Mod
			if (AI_considerPathDOW(pEndTurnPlot, iFlags))
			{	// <advc.163>
				if(!canMove())
					return true; // </advc.163>
				/*  regenerate the path, just in case we want to take a different route after the DOW
					(but don't bother recalculating the best destination)
					Note. if the best destination happens to be on the border,
					and has a stack of defenders on it, this will make us attack them.
					That's bad. I'll try to fix that in the future. */
				if (!generatePath(pBestPlot, iFlags, false))
					return false;
				CvPlot* pEnemyPlot = pEndTurnPlot; // advc.001t
				pEndTurnPlot = getPathEndTurnPlot();
				// <advc.139> Don't move through city that is about to be lost
				if(pEndTurnPlot->getPlotCity() != NULL &&
						pEndTurnPlot->getPlotCity()->AI_isEvacuating())
					return false; // </advc.139>
				// <advc.001t>
				if(!isEnemy(pEndTurnPlot->getTeam())) {
					FAssertMsg(isEnemy(pEndTurnPlot->getTeam()),
						"Known issue: AI may change its mind about the path to"
						" the target city after declaring war; temporary fix:"
						" stick to the original path.");
					if(isEnemy(pEnemyPlot->getTeam()))
						pEndTurnPlot = pEnemyPlot;
					else FAssert(isEnemy(pEnemyPlot->getTeam()));
					/*  If the else... assert fails, it's probably b/c the stack
						has multiple moves and there is an intermediate tile that
						requires a DoW. So this can be fine. Could check this through
						getPathFinder().GetEndNode() like it's done in
						AI_considerPathDOW -- tbd.? */
				} // </advc.001t>
			}
			// I'm going to use MISSIONAI_ASSAULT signal to our spies and other units that we're attacking this city.
			getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX_INLINE(), pEndTurnPlot->getY_INLINE(), iFlags, false, false, MISSIONAI_ASSAULT, pTargetCity->plot());
			// K-Mod end
			return true;
		}
	}

	return false;
}

bool CvUnitAI::AI_pillageAroundCity(CvCity* pTargetCity, int iBonusValueThreshold, int iFlags, int iMaxPathTurns)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestPillagePlot;
	int iPathTurns;
	int iValue;
	int iBestValue;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestPillagePlot = NULL;

	// K-Mod
	if (!isEnemy(pTargetCity->getTeam()) && !getGroup()->AI_isDeclareWar(pTargetCity->plot()))
	{
		return false;
	}
	// K-Mod end

	for( int iI = 0; iI < NUM_CITY_PLOTS; iI++ )
	{
		pLoopPlot = pTargetCity->getCityIndexPlot(iI);

		if (pLoopPlot != NULL)
		{
			if (AI_plotValid(pLoopPlot) && !(pLoopPlot->isBarbarian()))
			{
				if (potentialWarAction(pLoopPlot) && (pLoopPlot->getTeam() == pTargetCity->getTeam()))
				{
                    if (canPillage(pLoopPlot))
                    {
                        if (!(pLoopPlot->isVisibleEnemyUnit(this)))
                        {
                            if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_PILLAGE, getGroup()) == 0)
                            {
                                if (generatePath(pLoopPlot, iFlags, true, &iPathTurns, iMaxPathTurns))
                                {
									if (getPathFinder().GetFinalMoves() == 0)
                                    {
                                        iPathTurns++;
                                    }

                                    if ( iPathTurns <= iMaxPathTurns )
                                    {
                                        iValue = AI_pillageValue(pLoopPlot, iBonusValueThreshold);
										iValue *= (1000 + 30 *
												/*  advc.012: This seems to be about a single unit, so
													noDefensiveBonus should be checked.
													A hill bias might make sense b/c of Iron and Copper,
													but this sth. AI_pillageValue needs to handle. */
												(noDefensiveBonus() ? 0 : AI_plotDefense(pLoopPlot)));
												//(pLoopPlot->defenseModifier(getTeam(),false));

                                        //iValue /= (iPathTurns + 1);
										iValue /= std::max(1, iPathTurns); // K-Mod

										// if not at war with this plot owner, then devalue plot if we already inside this owner's borders
										// (because declaring war will pop us some unknown distance away)
										if (!isEnemy(pLoopPlot->getTeam()) && plot()->getTeam() == pLoopPlot->getTeam())
										{
											iValue /= 10;
										}

                                        if (iValue > iBestValue)
                                        {
                                            iBestValue = iValue;
                                            pBestPlot = getPathEndTurnPlot();
                                            pBestPillagePlot = pLoopPlot;
                                        }
                                    }
                                }
                            }
                        }
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestPillagePlot != NULL))
	{
		/* original code
		if (atPlot(pBestPillagePlot) && !isEnemy(pBestPillagePlot->getTeam()))
		{
			//getGroup()->groupDeclareWar(pBestPillagePlot, true);
			// rather than declare war, just find something else to do, since we may already be deep in enemy territory
			return false;
		} */ // disabled by K-Mod. (also see new code at top.)
		// K-Mod
		FAssert(getGroup()->AI_isDeclareWar());
		if (AI_considerPathDOW(pBestPlot, iFlags))
		{	// <advc.163>
			if(!canMove())
				return true; // </advc.163>
			if (!generatePath(pBestPillagePlot, iFlags, true, &iPathTurns))
				return false;
			pBestPlot = getPathEndTurnPlot();
		}
		// K-Mod end
		
		if (atPlot(pBestPillagePlot))
		{
			//if (isEnemy(pBestPillagePlot->getTeam()))
			FAssert(isEnemy(pBestPillagePlot->getTeam())); // K-Mod
			{
				getGroup()->pushMission(MISSION_PILLAGE, -1, -1, 0, false, false, MISSIONAI_PILLAGE, pBestPillagePlot);
				return true;
			}
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), iFlags, false, false, MISSIONAI_PILLAGE, pBestPillagePlot);
			return true;
		}
	}

	return false;
}

// Returns true if a mission was pushed...
// This function has been completely rewritten (and greatly simplified) for K-Mod
bool CvUnitAI::AI_bombardCity()
{
	// check if we need to declare war before bombarding!
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		CvPlot* pLoopPlot = plotDirection(plot()->getX_INLINE(), plot()->getY_INLINE(), ((DirectionTypes)iI));

		if (pLoopPlot != NULL && pLoopPlot->isCity())
		{
			if(AI_considerDOW(pLoopPlot)) { // <advc.163>
				if(!canMove())
					return true;
			} // </advc.163>
			break; // assume there can only be one city adjacent to us.
		}
	}

	if (!canBombard(plot()))
		return false;

	CvCity* pBombardCity = bombardTarget(plot());

	FAssertMsg(pBombardCity != NULL, "BombardCity is not assigned a valid value");

	int iAttackOdds = getGroup()->AI_attackOdds(pBombardCity->plot(), true);
	int iBase = GC.getBBAI_SKIP_BOMBARD_BASE_STACK_RATIO();
	int iMin = GC.getBBAI_SKIP_BOMBARD_MIN_STACK_RATIO();
	int iBombardTurns = getGroup()->getBombardTurns(pBombardCity);
	// <advc.004c>
	if(iBombardTurns == 0)
		return false; // </advc.004c>
	iBase = (iBase * (GC.getMAX_CITY_DEFENSE_DAMAGE()-pBombardCity->getDefenseDamage()) + iMin * pBombardCity->getDefenseDamage())/std::max(1, GC.getMAX_CITY_DEFENSE_DAMAGE());
	int iThreshold = (iBase * (100 - iAttackOdds) + (1 + iBombardTurns/2) * iMin * iAttackOdds) / (100 + (iBombardTurns/2) * iAttackOdds);
	int iComparison = getGroup()->AI_compareStacks(pBombardCity->plot(), true);

	if (iComparison > iThreshold)
	{
		if( gUnitLogLevel > 2 ) logBBAI("      Stack skipping bombard of %S with compare %d, starting odds %d, bombard turns %d, threshold %d", pBombardCity->getName().GetCString(), iComparison, iAttackOdds, iBombardTurns, iThreshold);
		return false;
	}

	//getGroup()->pushMission(MISSION_BOMBARD);
	getGroup()->pushMission(MISSION_BOMBARD, -1, -1, 0, false, false, MISSIONAI_ASSAULT, pBombardCity->plot()); // K-Mod
	return true;
}

// Returns true if a mission was pushed...
// This function has been been heavily edited for K-Mod.
bool CvUnitAI::AI_cityAttack(int iRange, int iOddsThreshold, int iFlags, bool bFollow)
{
	PROFILE_FUNC();

	FAssert(canMove());

	int iSearchRange = bFollow ? 1 : AI_searchRange(iRange);
	bool bDeclareWar = iFlags & MOVE_DECLARE_WAR;

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;

	for (int iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (int iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot))
				{
					if (pLoopPlot->isCity() && (bDeclareWar ? AI_potentialEnemy(pLoopPlot->getTeam(), pLoopPlot) : isEnemy(pLoopPlot->getTeam(), pLoopPlot)))
					{
						int iPathTurns;
						if (!atPlot(pLoopPlot) && (bFollow ? canMoveOrAttackInto(pLoopPlot, bDeclareWar) : generatePath(pLoopPlot, iFlags, true, &iPathTurns, iRange)))
						{
							int iValue = pLoopPlot->getNumVisiblePotentialEnemyDefenders(this) == 0 ? 100 : AI_getWeightedOdds(pLoopPlot, true);

							if (iValue >= iOddsThreshold)
							{
								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = ((bFollow) ? pLoopPlot : getPathEndTurnPlot());
									FAssert(!atPlot(pBestPlot));
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		// K-Mod
		if (AI_considerPathDOW(pBestPlot, iFlags))
		{	// <advc.163>
			if(!canMove())
				return true; // </advc.163>
			// after DOW, we might not be able to get to our target this turn... but try anyway.
			if (!generatePath(pBestPlot, iFlags, false))
				return false;
			if (bFollow && pBestPlot != getPathEndTurnPlot())
				return false;
			pBestPlot = getPathEndTurnPlot();
		}
		if (bFollow && pBestPlot->getNumVisiblePotentialEnemyDefenders(this) == 0)
		{
			FAssert(pBestPlot->getPlotCity() != 0);
			// we need to ungroup this unit so that we can move into the city.
			joinGroup(0);
			bFollow = false;
		}
		// K-Mod end
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), iFlags | (bFollow ? MOVE_DIRECT_ATTACK | MOVE_SINGLE_ATTACK : 0));
		return true;
	}

	return false;
}

// Returns true if a mission was pushed...
// This function has been been written for K-Mod. (it started getting messy, so I deleted most of the old code)
// bFollow implies AI_follow conditions - ie. not everyone in the group can move, and this unit might not be the group leader.
bool CvUnitAI::AI_anyAttack(int iRange, int iOddsThreshold, int iFlags, int iMinStack, bool bAllowCities, bool bFollow)
{
	PROFILE_FUNC();

	FAssert(canMove());

	if (AI_rangeAttack(iRange))
	{
		return true;
	}

	int iSearchRange = bFollow ? 1 : AI_searchRange(iRange);
	bool bDeclareWar = iFlags & MOVE_DECLARE_WAR;

	/*  advc.128: Within this range, the AI is able see to units on hidden tiles.
		The random number can't be generated here b/c AI_anyAttack can be called
		multiple times for the same unit within one turn. (Typically it's called
		twice.) A new random number for each call would give a unit multiple
		chances to spot a target. */
	int searchRangeRand = ::round((iSearchRange * m_iSearchRangeRandPercent) / 100.0);

	CvPlot* pBestPlot = NULL;

	for (int iDX = -iSearchRange; iDX <= iSearchRange; iDX++)
	{
		for (int iDY = -iSearchRange; iDY <= iSearchRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot == NULL || !AI_plotValid(pLoopPlot))
				continue;

			if (!bAllowCities && pLoopPlot->isCity())
				continue;
			// <advc.128>
			if((std::abs(iDX) > searchRangeRand || std::abs(iDY) > searchRangeRand)
					&& !pLoopPlot->isVisible(getTeam(), false))
				continue; // </advc.128>
			if (bDeclareWar
				? !pLoopPlot->isVisiblePotentialEnemyUnit(getOwnerINLINE()) && !(pLoopPlot->isCity() && AI_potentialEnemy(pLoopPlot->getPlotCity()->getTeam(), pLoopPlot))
				: !pLoopPlot->isVisibleEnemyUnit(this) && !pLoopPlot->isEnemyCity(*this))
			{
				continue;
			}

			int iEnemyDefenders = bDeclareWar ? pLoopPlot->getNumVisiblePotentialEnemyDefenders(this) : pLoopPlot->getNumVisibleEnemyDefenders(this);
			// <advc.033>
			if(isAlwaysHostile(pLoopPlot)) {
				std::pair<int,int> defendersAll = AI_countPiracyTargets(*pLoopPlot);
				if(defendersAll.second <= 0)
					continue;
				iEnemyDefenders = defendersAll.first;
			} // </advc.033>
			if (iEnemyDefenders < iMinStack)
				continue;

			if (!atPlot(pLoopPlot) && (bFollow ? getGroup()->canMoveOrAttackInto(pLoopPlot, bDeclareWar, true) : generatePath(pLoopPlot, iFlags, true, 0, iRange)))
			{
				// 101 for cities, because that's a better thing to capture.
				int iOdds = (iEnemyDefenders == 0 ?
						(pLoopPlot->isCity() ? 101 : 100) :
						AI_getWeightedOdds(pLoopPlot, false));
				if (iOdds >= iOddsThreshold)
				{
					iOddsThreshold = iOdds;
					pBestPlot = bFollow ? pLoopPlot : getPathEndTurnPlot();
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		// K-Mod
		if (AI_considerPathDOW(pBestPlot, iFlags))
		{	// <advc.163>
			if(!canMove())
				return true; // </advc.163>
			// after DOW, we might not be able to get to our target this turn... but try anyway.
			if (!generatePath(pBestPlot, iFlags))
				return false;
			if (bFollow && pBestPlot != getPathEndTurnPlot())
				return false;
			pBestPlot = getPathEndTurnPlot();
		}
		if (bFollow && pBestPlot->getNumVisiblePotentialEnemyDefenders(this) == 0)
		{
			// we need to ungroup to capture the undefended unit / city. (because not everyone in our group can move)
			joinGroup(0);
			bFollow = false;
		}
		// K-Mod end
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), iFlags | (bFollow ? MOVE_DIRECT_ATTACK | MOVE_SINGLE_ATTACK : 0));
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_rangeAttack(int iRange)
{
	PROFILE_FUNC();

	FAssert(canMove());

	if (!canRangeStrike())
	{
		return false;
	}

	int iSearchRange = AI_searchRange(iRange);

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;

	for (int iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (int iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				//if (pLoopPlot->isVisibleEnemyUnit(this) || (pLoopPlot->isCity() && AI_potentialEnemy(pLoopPlot->getTeam())))
				if (pLoopPlot->isVisibleEnemyUnit(this)) // K-Mod
				{
					if (!atPlot(pLoopPlot) && canRangeStrikeAt(plot(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
					{
						int iValue = getGroup()->AI_attackOdds(pLoopPlot, true);

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopPlot;
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		// K-Mod note: no AI_considerDOW here.
		getGroup()->pushMission(MISSION_RANGE_ATTACK, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0);
		return true;
	}

	return false;
}

// (heavily edited for K-Mod)
bool CvUnitAI::AI_leaveAttack(int iRange, int iOddsThreshold, int iStrengthThreshold)
{
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod

	FAssert(canMove());

	int iSearchRange = iRange;

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;

	CvCity* pCity = plot()->getPlotCity();

	if ((pCity != NULL) && (pCity->getOwnerINLINE() == getOwnerINLINE()))
	{
		/*int iOurStrength = GET_PLAYER(getOwnerINLINE()).AI_getOurPlotStrength(plot(), 0, false, false);
		int iEnemyStrength = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(), 2, false, false);*/
		// K-Mod
		int iOurDefence = kOwner.AI_localDefenceStrength(plot(), getTeam());
		int iEnemyStrength = kOwner.AI_localAttackStrength(plot(), NO_TEAM, DOMAIN_LAND, 2);
		// K-Mod end
		if (iEnemyStrength > 0)
		{
			if (iOurDefence * 100 / iEnemyStrength < iStrengthThreshold)
			{
				// K-Mod.
				// We should only heed to the threshold if we either we have enough defence to hold the city,
				// or we don't have enough attack force to wipe the enemy out.
				// (otherwise, we are better off attacking than defending.)
				if (iEnemyStrength < iOurDefence
					|| kOwner.AI_localAttackStrength(plot(), getTeam(), DOMAIN_LAND, 0, false, false, true)
					 < kOwner.AI_localDefenceStrength(plot(), NO_TEAM, DOMAIN_LAND, 2, false))
				// K-Mod end
					return false;
			}
			if (plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(),
					NO_TEAM, PUF_isDomainType, DOMAIN_LAND) // advc.001s
					<= getGroup()->getNumUnits())
			{
				return false;
			}
		}
	}

	for (int iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (int iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot == NULL || !AI_plotValid(pLoopPlot))
				continue;

			/*if (pLoopPlot->isVisibleEnemyUnit(this) || (pLoopPlot->isCity() && AI_potentialEnemy(pLoopPlot->getTeam(), pLoopPlot)))
			{
				//if (pLoopPlot->getNumVisibleEnemyDefenders(this) > 0) */
			if (pLoopPlot->isVisibleEnemyDefender(this)) // K-Mod
			{
				if (!atPlot(pLoopPlot) && generatePath(pLoopPlot, 0, true, 0, iRange))
				{
					//iValue = getGroup()->AI_attackOdds(pLoopPlot, true);
					int iValue = AI_getWeightedOdds(pLoopPlot, false); // K-Mod

					//if (iValue >= AI_finalOddsThreshold(pLoopPlot, iOddsThreshold))
					if (iValue >= iOddsThreshold) // K-mod
					{
						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = getPathEndTurnPlot();
							FAssert(!atPlot(pBestPlot));
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		// K-Mod note: no AI_considerDOW here.
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_COUNTER_ATTACK);
		return true;
	}

	return false;
}

// K-Mod. Defend nearest city against invading attack stacks.
bool CvUnitAI::AI_defensiveCollateral(int iThreshold, int iSearchRange)
{
	PROFILE_FUNC();
	FAssert(collateralDamage() > 0);

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());

	CvPlot* pDefencePlot = 0;
	
	if (plot()->isCity(false, getTeam()))
		pDefencePlot = plot();
	else
	{
		int iClosest = MAX_INT;
		for (int iDX = -iSearchRange; iDX <= iSearchRange; iDX++)
		{
			for (int iDY = -iSearchRange; iDY <= iSearchRange; iDY++)
			{
				CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

				if (pLoopPlot && pLoopPlot->isCity(false, getTeam()))
				{
					if (kOwner.AI_getAnyPlotDanger(pLoopPlot))
					{
						pDefencePlot = pLoopPlot;
						break;
					}

					int iDist = std::max(std::abs(iDX), std::abs(iDY));
					if (iDist < iClosest)
					{
						iClosest = iDist;
						pDefencePlot = pLoopPlot;
					}
				}
			}
		}
	}

	if (pDefencePlot == NULL)
		return false;

	int iEnemyAttack = kOwner.AI_localAttackStrength(pDefencePlot, NO_TEAM, getDomainType(), iSearchRange);
	int iOurDefence = kOwner.AI_localDefenceStrength(pDefencePlot, getTeam(), getDomainType(), 0);
	bool bDanger = iEnemyAttack > iOurDefence;

	CvPlot* pBestPlot = NULL;

	for (int iDX = -iSearchRange; iDX <= iSearchRange; iDX++)
	{
		for (int iDY = -iSearchRange; iDY <= iSearchRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);
			if (pLoopPlot && AI_plotValid(pLoopPlot) && !atPlot(pLoopPlot))
			{
				int iEnemies = pLoopPlot->getNumVisibleEnemyDefenders(this);
				int iPathTurns;
				if (iEnemies > 0 && generatePath(pLoopPlot, 0, true, &iPathTurns, 1))
				{
					//int iValue = getGroup()->AI_attackOdds(pLoopPlot, false);
					int iValue = AI_getWeightedOdds(pLoopPlot);

					if (iValue > 0 && iEnemies >= std::min(4, collateralDamageMaxUnits()))
					{
						int iOurAttack = kOwner.AI_localAttackStrength(pLoopPlot, getTeam(), getDomainType(), iSearchRange, true, true, true);
						int iEnemyDefence = kOwner.AI_localDefenceStrength(pLoopPlot, NO_TEAM, getDomainType(), 0);

						iValue += std::max(0, (bDanger ? 75 : 45) * (3 * iOurAttack - iEnemyDefence) / std::max(1, 3*iEnemyDefence));
						// note: the scale is choosen to be around +50% when attack == defence, while in danger.
						if (bDanger && std::max(std::abs(iDX), std::abs(iDY)) <= 1)
						{
							// enemy is ready to attack, and strong enough to win. We might as well hit them.
							iValue += 20;
						}
					}

					if (iValue >= iThreshold)
					{
						iThreshold = iValue;
						pBestPlot = getPathEndTurnPlot();
					}
				}
			}
		} // dy
	} // dx

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}

// <advc.139>
bool CvUnitAI::AI_evacuateCity() {

	if(!plot()->getPlotCity()->AI_isEvacuating())
		return false;
	double prEvac = 1;
	/*  Units that don't receive def. modifiers should always evacuate.
		AI_defensiveCollateral can still happen, but not when the threat ratio is
		this high. */
	if(m_pUnitInfo->getCombat() > 0 && !m_pUnitInfo->isNoDefensiveBonus()) {
		if(AI_getUnitAIType() == UNITAI_CITY_DEFENSE)
			prEvac = 0;
		else {
			prEvac = 1.8 - currHitPoints() / (maxHitPoints()+0.001);
			int defenseMod = fortifyModifier() + plot()->defenseModifier(getTeam(),
					GC.getGameINLINE().getCurrentEra() > 3) + cityDefenseModifier() +
					(plot()->isHills() ? hillsDefenseModifier() : 0);
			prEvac -= defenseMod / 100.0;
		}
	}
	/*  retreatToCity isn't perfect for this; selects the city based on plot danger.
		Hopefully sufficient most of the time. */
	if(::bernoulliSuccess(prEvac, "advc.139"))
		return AI_retreatToCity();
	return false;
} // </advc.139>


// K-Mod.
// bLocal is just to help with the efficiency of this function for short-range checks. It means that we should look only in nearby plots.
// the default (bLocal == false) is to look at every plot on the map!
bool CvUnitAI::AI_defendTerritory(int iThreshold, int iFlags, int iMaxPathTurns, bool bLocal)
{
	PROFILE_FUNC();

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());

	CvPlot* pEndTurnPlot = NULL;
	int iBestValue = 0;

	//for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	// I'm going to use a loop equivalent to the above when !bLocal; and a loop in a square around our unit if bLocal.
	int i = 0;
	int iRange = bLocal ? AI_searchRange(iMaxPathTurns) : 0;
	int iPlots = bLocal ? (2*iRange+1)*(2*iRange+1) : GC.getMapINLINE().numPlotsINLINE();
	if (bLocal && iPlots >= GC.getMapINLINE().numPlotsINLINE())
	{
		bLocal = false;
		iRange = 0;
		iPlots = GC.getMapINLINE().numPlotsINLINE();
		// otherwise it's just silly.
	}
	FAssert(!bLocal || iRange > 0);
	while (i < iPlots)
	{
		CvPlot* pLoopPlot = bLocal
			? plotXY(getX_INLINE(), getY_INLINE(), -iRange + i % (2*iRange+1), -iRange + i / (2*iRange+1))
			: GC.getMapINLINE().plotByIndexINLINE(i);
		i++; // for next cycle.

		if (pLoopPlot && pLoopPlot->getTeam() == getTeam() && AI_plotValid(pLoopPlot))
		{
			if (pLoopPlot->isVisibleEnemyUnit(this))
			{	// <advc.033>
				if(isAlwaysHostile(pLoopPlot) && !AI_isAnyPiracyTarget(*pLoopPlot))
					continue;
				/*  This doesn't guarantee that the best defender will be a
					PiracyTarget, but at least we're going to attack a unit that
					is hanging out with a target. */ // </advc.033>
				int iPathTurns;
				if (generatePath(pLoopPlot, iFlags, true, &iPathTurns, iMaxPathTurns))
				{
					int iOdds = AI_getWeightedOdds(pLoopPlot);
					int iValue = iOdds;

					if (iOdds > 0 && iOdds < 100 && iThreshold > 0)
					{
						int iOurAttack = kOwner.AI_localAttackStrength(pLoopPlot, getTeam(), getDomainType(), 2, true, true, true);
						int iEnemyDefence = kOwner.AI_localDefenceStrength(pLoopPlot, NO_TEAM, getDomainType(), 0);

						if (iOurAttack > iEnemyDefence && iEnemyDefence > 0)
						{
							/*int iBonus = 100 - iOdds;
							iBonus -= iBonus * 4*iBonus / (4*iBonus + 100*(iOurAttack-iEnemyDefence)/iEnemyDefence);

							FAssert(iBonus >= 0);
							FAssert(iBonus <= 100 - iOdds);

							iValue += iBonus;*/
							iValue += 100 * (iOdds+15) * (iOurAttack-iEnemyDefence)/((iThreshold+100) * iEnemyDefence);
						}
					}

					if (iValue >= iThreshold)
					{
						BonusTypes eBonus = pLoopPlot->getNonObsoleteBonusType(getTeam());
						iValue *= 100 + (eBonus != NO_BONUS ? 3*kOwner.AI_bonusVal(eBonus, 0)/2 : 0) + (pLoopPlot->getWorkingCity() ? 20 : 0);

						if (pLoopPlot->getOwnerINLINE() != getOwnerINLINE())
							iValue = 2*iValue/3;

						if (iPathTurns > 1)
							iValue /= iPathTurns + 2;

						if (iOdds >= iThreshold)
							iValue = 4*iValue/3;

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pEndTurnPlot = getPathEndTurnPlot();
						}
					}
				}
			}
		}
	}

	if (pEndTurnPlot != NULL)
	{
		FAssert(!atPlot(pEndTurnPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX_INLINE(), pEndTurnPlot->getY_INLINE(), iFlags, false, false, MISSIONAI_DEFEND);
		return true;
	}

	return false;
}
// K-Mod end

// iAttackThreshold is the minimum ratio for our attack / their defence.
// iRiskThreshold is the minimum ratio for their attack / our defence adjusted for stack size
// note: iSearchRange is /not/ the number of turns. It is the number of steps. iSearchRange < 1 means 'automatic'
// Only 1-turn moves are considered here.
bool CvUnitAI::AI_stackVsStack(int iSearchRange, int iAttackThreshold, int iRiskThreshold, int iFlags)
{
	PROFILE_FUNC();

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());

	if (iSearchRange < 1)
	{
		iSearchRange = AI_searchRange(1);
	}

	//int iOurDefence = kOwner.AI_localDefenceStrength(plot(), getTeam());
	int iOurDefence = getGroup()->AI_sumStrength(0); // not counting defensive bonuses

	CvPlot* pBestPlot = NULL;
	int iBestValue = 0;

	for (int iDX = -iSearchRange; iDX <= iSearchRange; iDX++)
	{
		for (int iDY = -iSearchRange; iDY <= iSearchRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);
			if (pLoopPlot && AI_plotValid(pLoopPlot) && !atPlot(pLoopPlot))
			{
				int iEnemies = pLoopPlot->getNumVisibleEnemyDefenders(this);
				int iPathTurns;
				if (iEnemies > 0 && generatePath(pLoopPlot, iFlags, true, &iPathTurns, 1))
				{
					int iEnemyAttack = kOwner.AI_localAttackStrength(pLoopPlot, NO_TEAM, getDomainType(), 0, false);

					int iRiskRatio = 100 * iEnemyAttack / std::max(1, iOurDefence);
					// adjust risk ratio based on the relative numbers of units.
					iRiskRatio *= 50 + 50 * (getGroup()->getNumUnits()+3) / std::min(iEnemies+3, getGroup()->getNumUnits()+3);
					iRiskRatio /= 100;
					//
					if (iRiskRatio < iRiskThreshold)
						continue;

					int iAttackRatio = getGroup()->AI_compareStacks(pLoopPlot, true);
					if (iAttackRatio < iAttackThreshold)
						continue;

					int iValue = iAttackRatio * iRiskRatio;

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestPlot = pLoopPlot;
						FAssert(pBestPlot == getPathEndTurnPlot());
					}
				}
			}
		} // dy
	} // dx

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		if (gUnitLogLevel >= 2)
		{
			logBBAI("    Stack for player %d (%S) uses StackVsStack attack with value %d", getOwner(), GET_PLAYER(getOwnerINLINE()).getCivilizationDescription(0), iBestValue);
		}
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), iFlags, false, false, MISSIONAI_COUNTER_ATTACK, pBestPlot);
		return true;
	}

	return false;
}
// K-Moe end

// Returns true if a mission was pushed...
bool CvUnitAI::AI_blockade()
{
	PROFILE_FUNC();

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestBlockadePlot = NULL;

	int iFlags = MOVE_DECLARE_WAR; // K-Mod

	for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot))
		{
			if (potentialWarAction(pLoopPlot))
			{
				CvCity* pCity = pLoopPlot->getWorkingCity();

				if (pCity != NULL)
				{
					if (pCity->isCoastal())
					{
						if (!(pCity->isBarbarian()))
						{
							FAssert(isEnemy(pCity->getTeam()) || GET_TEAM(getTeam()).AI_getWarPlan(pCity->getTeam()) != NO_WARPLAN);

							if (!(pLoopPlot->isVisibleEnemyUnit(this)) && canPlunder(pLoopPlot))
							{
								if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BLOCKADE, getGroup(), 2) == 0)
								{
									int iPathTurns;
									if (generatePath(pLoopPlot, iFlags, true, &iPathTurns))
									{
										int iValue = 1;

										iValue += std::min(pCity->getPopulation(), pCity->countNumWaterPlots());

										iValue += GET_PLAYER(getOwnerINLINE()).AI_adjacentPotentialAttackers(pCity->plot());

										iValue += (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pCity->plot(), MISSIONAI_ASSAULT, getGroup(), 2) * 3);

										if (canBombard(pLoopPlot))
										{
											iValue *= 2;
										}

										iValue *= 1000;

										iValue /= (iPathTurns + 1);
										
										if (iPathTurns == 1)
										{
											//Prefer to have movement remaining to Bombard + Plunder
											iValue *= 1 + std::min(2, getPathFinder().GetFinalMoves());
										}

										// if not at war with this plot owner, then devalue plot if we already inside this owner's borders
										// (because declaring war will pop us some unknown distance away)
										if (!isEnemy(pLoopPlot->getTeam()) && plot()->getTeam() == pLoopPlot->getTeam())
										{
											iValue /= 10;
										}

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = getPathEndTurnPlot();
											pBestBlockadePlot = pLoopPlot;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestBlockadePlot != NULL))
	{
		FAssert(canPlunder(pBestBlockadePlot));
		if (atPlot(pBestBlockadePlot) && !isEnemy(pBestBlockadePlot->getTeam(), pBestBlockadePlot))
		{
			//getGroup()->groupDeclareWar(pBestBlockadePlot, true);
			if(AI_considerPathDOW(pBestBlockadePlot, iFlags)) { // K-Mod
				// <advc.163>
				if(!canMove())
					return true;
			} // </advc.163>
		}

		if (atPlot(pBestBlockadePlot))
		{
			if (canBombard(plot()))
			{
				getGroup()->pushMission(MISSION_BOMBARD, -1, -1, 0, false, false, MISSIONAI_BLOCKADE, pBestBlockadePlot);
			}

			//getGroup()->pushMission(MISSION_PLUNDER, -1, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BLOCKADE, pBestBlockadePlot);
			getGroup()->pushMission(MISSION_PLUNDER, -1, -1, 0, true, false, MISSIONAI_BLOCKADE, pBestBlockadePlot); // K-Mod
			
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), iFlags, false, false, MISSIONAI_BLOCKADE, pBestBlockadePlot);
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
// K-Mod todo: this function is very slow on large maps. Consider rewriting it!
// k146, advc.003b (comment): Performance might be OK now
bool CvUnitAI::AI_pirateBlockade()
{
	PROFILE_FUNC();
	
	int iPathTurns;
	
	/*  <k146> Removed the loop that computed the vector 'aiDeathZone'
		("computationally expensive, and not particularly effective").
		advc: I'm re-using parts of the body of that loop for a more limited
		danger check. */
	bool bInDanger = false;
	int iRange = m_pUnitInfo->getMoves();
	int iCurrEffStr = currEffectiveStr(plot(), NULL, NULL);
	for(int dx = -iRange; dx <= iRange; dx++) {
		for(int dy = -iRange; dy <= iRange; dy++) {
			if(dx == 0 && dy == 0)
				continue;
			CvPlot const* pp = plotXY(getX_INLINE(), getY_INLINE(), dx, dy);
			if(pp == NULL) continue; CvPlot const& p = *pp;
			if(!p.isVisible(getTeam(), false) || (p.area() != area() && !p.isCity()))
				continue;
			if(p.getNumUnits() > 20) // Make sure we're not spending too much time
				continue;
			CLLNode<IDInfo>* pNode = p.headUnitNode();
			while(pNode != NULL) {
				CvUnit const& u = *::getUnit(pNode->m_data);
				pNode = p.nextUnitNode(pNode);
				if(u.getDomainType() == DOMAIN_SEA && u.canFight() &&
						!::isMostlyDefensive(u.getUnitInfo()) &&
						isEnemy(u.getTeam(), pp) &&
						!u.isInvisible(getTeam(), false) &&
						u.currEffectiveStr(NULL, NULL, NULL) > iCurrEffStr + 50) {
					bInDanger = true;
					goto terminateOuter;
				}
			}
		}
	} terminateOuter:
	// </k146>
	if (!bInDanger)
	{
		if (getDamage() > 0)
		{
			if (!plot()->isOwned() && !plot()->isAdjacentOwned())
			{
				if (AI_retreatToCity(false, false, 1 + getDamage() / 20))
				{
					return true;
				}
				getGroup()->pushMission(MISSION_SKIP);
				return true;
			}
		}
	}
	
	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestBlockadePlot = NULL;
	bool bBestIsForceMove = false;
	bool bBestIsMove = false;
	int turnNumberSalt = GC.getGameINLINE().getGameTurn() % 7; // advc.003b
	for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
		// <advc.003>
		if(!AI_plotValid(pLoopPlot) ||
				!pLoopPlot->isRevealed(getTeam(), false) || // advc.003b
				pLoopPlot->isVisibleEnemyUnit(this) || !canPlunder(pLoopPlot) ||
				//GC.getGame().getSorenRandNum(4, "AI Pirate Blockade") != 0 ||
				/*  advc.033: Replacing the above. Should make Privateers a bit
					more stationary. */
				::hash(iI + turnNumberSalt, getOwnerINLINE()) > 0.25f ||
				GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(
				pLoopPlot, MISSIONAI_BLOCKADE, getGroup(), 3) != 0 ||
				// advc.003b:
				(!pLoopPlot->isOwned() && pLoopPlot->isAdjacentOwned()))
			continue; // </advc.003>
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/17/09                                jdog5000      */
/*                                                                                              */
/* Pirate AI                                                                                    */
/************************************************************************************************/
/* original bts code
		if (generatePath(pLoopPlot, 0, true, &iPathTurns))
*/		// <advc.003>
		if(!generatePath(pLoopPlot, MOVE_AVOID_ENEMY_WEIGHT_3, true, &iPathTurns))
			continue; // </advc.003>
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		int iBlockadedCount = 0;
		int iPopulationValue = 0;
		int iRange = GC.getDefineINT("SHIP_BLOCKADE_RANGE") - 1;
		for (int iX = -iRange; iX <= iRange; iX++)
		{
			for (int iY = -iRange; iY <= iRange; iY++)
			{
				CvPlot* pRangePlot = plotXY(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), iX, iY);
				// <advc.003>
				if(pRangePlot == NULL
						|| pRangePlot->isBarbarian()) // advc.033
					continue; // </advc.003>
				bool bPlotBlockaded = false;
				if (pRangePlot->isWater() && pRangePlot->isOwned() && isEnemy(pRangePlot->getTeam(), pLoopPlot))
				{
					bPlotBlockaded = true;
					iBlockadedCount += pRangePlot->getBlockadedCount(pRangePlot->getTeam());
				} // <advc.003>
				if(bPlotBlockaded)
					continue; // </advc.003>
				CvCity* pPlotCity = pRangePlot->getPlotCity();
				if (pPlotCity != NULL &&
					/*  advc.003: Note that isEnemy checks isAlwaysHostile;
						so the owner of pPlotCity does not have to be a war enemy
						of this unit. In fact, plundering war enemies is
						discouraged below; doesn't yield gold. */
						isEnemy(pPlotCity->getTeam(), pLoopPlot)
						&& !pPlotCity->isBarbarian()) // advc.123e
				{
					int iCityValue = 3 + pPlotCity->getPopulation();
					// <advc.033>
					if(GET_TEAM(pPlotCity->getTeam()).isVassal(getTeam()) ||
							GET_TEAM(getTeam()).isVassal(pPlotCity->getTeam()))
						iCityValue = 0;
					if(!isBarbarian()) {
						int attitudeFactor = std::max(1, 10 - ::round(
								std::pow((double)GET_PLAYER(getOwnerINLINE()).
								AI_getAttitude(pPlotCity->getOwnerINLINE()), 1.5)));
						iCityValue *= attitudeFactor;
						TechTypes techReq = (TechTypes)m_pUnitInfo->getPrereqAndTech();
						int ourEra = (techReq == NO_TECH ?
								GET_PLAYER(getOwnerINLINE()).getCurrentEra() :
								GC.getTechInfo(techReq).getEra());
						int theirEra = GET_PLAYER(pPlotCity->getOwnerINLINE()).
								getCurrentEra();
						double eraFactor = 1.5;
						if(theirEra > ourEra)
							eraFactor = 0.5;
						if(ourEra > theirEra)
							eraFactor = 2.5;
						/*  Era alone is too coarse. A civ in early Renaissance
							is a fine target for Privateers.
							Tbd.: Should check sth. like isTechTrading first --
							we might not know whether they have the tech. */
						if(techReq != NO_TECH && GET_TEAM(pPlotCity->getTeam()).
								isHasTech(techReq))
							eraFactor /= 2;
						iCityValue = ::round(iCityValue * eraFactor);
					} // </advc.033>
					iCityValue *= (atWar(getTeam(), pPlotCity->getTeam()) ? 1 : 3);
					if (GET_PLAYER(pPlotCity->getOwnerINLINE()).isNoForeignTrade())
					{
						iCityValue /= 2;
					}
					iPopulationValue += ::round(iCityValue
							/*  advc.033: Normalize to keep the scale as it was
								and avoid overflows */
							/ (isBarbarian() ? 1.0 : 7.0));
				}
			}
		} // advc.003: Declaration moved
		int iValue = iPopulationValue;

		iValue *= 1000;

		iValue /= 16 + iBlockadedCount;

		bool bMove = getPathFinder().GetPathTurns() == 1 && getPathFinder().GetFinalMoves() > 0;
		if (atPlot(pLoopPlot))
		{
			iValue *= 3;
		}
		else if (bMove)
		{
			iValue *= 2;
		}
		bool bForceMove = false;
		// k146: Some bInDanger code deleted
		if (bInDanger && iPathTurns <= 2 && 0 == iPopulationValue &&
				// advc.003:
				getPathFinder().GetFinalMoves() == 0)
				// advc.003b: AdjacentOwned now guaranteed
				//&& !pLoopPlot->isAdjacentOwned()
		{
			int iRand = GC.getGame().getSorenRandNum(2500, "AI Pirate Retreat");
			iValue += iRand;
			if (iRand > 1000)
			{
				iValue += GC.getGame().getSorenRandNum(2500, "AI Pirate Retreat");
				bForceMove = true;
			}
		}

		if (!bForceMove)
		{
			iValue /= iPathTurns + 1;
		}

		if (iValue > iBestValue)
		{
			iBestValue = iValue;
			pBestPlot = bForceMove ? pLoopPlot : getPathEndTurnPlot();
			pBestBlockadePlot = pLoopPlot;
			bBestIsForceMove = bForceMove;
			bBestIsMove = bMove;
		}
	}

	if (pBestPlot != NULL && pBestBlockadePlot != NULL)
	{
		FAssert(canPlunder(pBestBlockadePlot));

		if (atPlot(pBestBlockadePlot))
		{
			//getGroup()->pushMission(MISSION_PLUNDER, -1, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BLOCKADE, pBestBlockadePlot);
			getGroup()->pushMission(MISSION_PLUNDER, -1, -1, 0, true, false, MISSIONAI_BLOCKADE, pBestBlockadePlot); // K-Mod
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			if (bBestIsForceMove)
			{
				getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_AVOID_ENEMY_WEIGHT_3);
				return true;
			}
			else
			{
				getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_AVOID_ENEMY_WEIGHT_3, false, false, MISSIONAI_BLOCKADE, pBestBlockadePlot);
				if (bBestIsMove)
				{
					//getGroup()->pushMission(MISSION_PLUNDER, -1, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BLOCKADE, pBestBlockadePlot);
					getGroup()->pushMission(MISSION_PLUNDER, -1, -1, 0, true, false, MISSIONAI_BLOCKADE, pBestBlockadePlot); // K-Mod
				}
				return true;
			}
		}
	}
	return false;
}

// Returns true if a mission was pushed...
bool CvUnitAI::AI_seaBombardRange(int iMaxRange)
{
	PROFILE_FUNC();
	
	// cached values
	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());
	CvPlot* pPlot = plot();
	CvSelectionGroup* pGroup = getGroup();
	
	// can any unit in this group bombard?
	bool bHasBombardUnit = false;
	bool bBombardUnitCanBombardNow = false;
	CLLNode<IDInfo>* pUnitNode = pGroup->headUnitNode();
	while (pUnitNode != NULL && !bBombardUnitCanBombardNow)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = pGroup->nextUnitNode(pUnitNode);

		if (pLoopUnit->bombardRate() > 0)
		{
			bHasBombardUnit = true;

			if (pLoopUnit->canMove() && !pLoopUnit->isMadeAttack())
			{
				bBombardUnitCanBombardNow = true;
			}
		}
	}

	if (!bHasBombardUnit)
	{
		return false;
	}

	// best match
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestBombardPlot = NULL;
	int iBestValue = 0;
	
	// iterate over plots at each range
	for (int iDX = -(iMaxRange); iDX <= iMaxRange; iDX++)
	{
		for (int iDY = -(iMaxRange); iDY <= iMaxRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);
			
			if (pLoopPlot != NULL && AI_plotValid(pLoopPlot))
			{
				CvCity* pBombardCity = bombardTarget(pLoopPlot);
				/*  <advc.004c> Don't bombard cities at 0 (even if there is nothing
					better to do b/c it spams the message log) */
				if(pBombardCity != NULL && (pBombardCity->getDefenseModifier(false) <= 0 ||
						// advc.033:
						(pBombardCity->isBarbarian() && pBombardCity->getDefenseModifier(true) <= 0)))
					pBombardCity = NULL; // </advc.004c>
				if (pBombardCity != NULL && isEnemy(pBombardCity->getTeam(), pLoopPlot) && pBombardCity->getDefenseDamage() < GC.getMAX_CITY_DEFENSE_DAMAGE())
				{
					int iPathTurns;
					if (generatePath(pLoopPlot, 0, true, &iPathTurns, 1 + iMaxRange/baseMoves()))
					{
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						6/24/08				jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
						// Loop construction doesn't guarantee we can get there anytime soon, could be on other side of narrow continent
						if( iPathTurns <= (1 + iMaxRange/baseMoves()) )
						{
							// Check only for supporting our own ground troops first, if none will look for another target
							int iValue = (kPlayer.AI_plotTargetMissionAIs(pBombardCity->plot(), MISSIONAI_ASSAULT, NULL, 2) * 3);
							iValue += (kPlayer.AI_adjacentPotentialAttackers(pBombardCity->plot(), true));
							
							if (iValue > 0)
							{
								iValue *= 1000;

								iValue /= (iPathTurns + 1);
								
								if (iPathTurns == 1)
								{
									//Prefer to have movement remaining to Bombard + Plunder
									iValue *= 1 + std::min(2, getPathFinder().GetFinalMoves());
								}

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = getPathEndTurnPlot();
									pBestBombardPlot = pLoopPlot;
								}
							}
						}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD							END							*/
/********************************************************************************/
					}
				}
			}
		}
	}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						6/24/08				jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	// If no troops of ours to support, check for other bombard targets
	if( (pBestPlot == NULL) && (pBestBombardPlot == NULL) )
	{
		if( (AI_getUnitAIType() != UNITAI_ASSAULT_SEA) )
		{
			for (int iDX = -(iMaxRange); iDX <= iMaxRange; iDX++)
			{
				for (int iDY = -(iMaxRange); iDY <= iMaxRange; iDY++)
				{
					CvPlot* pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);
					
					if (pLoopPlot != NULL && AI_plotValid(pLoopPlot))
					{
						CvCity* pBombardCity = bombardTarget(pLoopPlot);

						// Consider city even if fully bombarded, causes ship to camp outside blockading instead of twitching between
						// cities after bombarding to 0
						if (pBombardCity != NULL && isEnemy(pBombardCity->getTeam(), pLoopPlot) && pBombardCity->getTotalDefense(false) > 0
								/*  advc.033: Barbs normally have only building defense.
									If that's the case, don't sea-bombard them. */
								&& (!pBombardCity->isBarbarian() || pBombardCity->getTotalDefense(true) > 0))
						{
							int iPathTurns;
							if (generatePath(pLoopPlot, 0, true, &iPathTurns, 1 + iMaxRange/baseMoves()))
							{	
								// Loop construction doesn't guarantee we can get there anytime soon, could be on other side of narrow continent
								if( iPathTurns <= 1 + iMaxRange/baseMoves() )
								{
									int iValue = std::min(20,pBombardCity->getDefenseModifier(false)/2); 

									// Inclination to support attacks by others
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
									//if( GET_PLAYER(pBombardCity->getOwnerINLINE()).AI_getPlotDanger(pBombardCity->plot(), 2, false) > 0 )
									if( GET_PLAYER(pBombardCity->getOwnerINLINE()).AI_getAnyPlotDanger(pBombardCity->plot(), 2, false) )
									{
										iValue += 60;
									}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

									// Inclination to bombard a different nearby city to extend the reach of blockade
									if( GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pBombardCity->plot(), MISSIONAI_BLOCKADE, getGroup(), 3) == 0 )
									{
										iValue += 35 + pBombardCity->getPopulation();
									}

									// Small inclination to bombard area target, not too large so as not to tip our hand
									if( pBombardCity == pBombardCity->area()->getTargetCity(getOwnerINLINE()) )
									{
										iValue += 10;
									}
										
									if (iValue > 0)
									{
										iValue *= 1000;

										iValue /= (iPathTurns + 1);
										
										if (iPathTurns == 1)
										{
											//Prefer to have movement remaining to Bombard + Plunder
											iValue *= 1 + std::min(2, getPathFinder().GetFinalMoves());
										}

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = getPathEndTurnPlot();
											pBestBombardPlot = pLoopPlot;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD							END							*/
	/********************************************************************************/
	
	if ((pBestPlot != NULL) && (pBestBombardPlot != NULL))
	{
		if (atPlot(pBestBombardPlot))
		{
			// if we are at the plot from which to bombard, and we have a unit that can bombard this turn, do it
			if (bBombardUnitCanBombardNow && pGroup->canBombard(pBestBombardPlot))
			{
				getGroup()->pushMission(MISSION_BOMBARD, -1, -1, 0, false, false, MISSIONAI_BLOCKADE, pBestBombardPlot);

				// if city bombarded enough, wake up any units that were waiting to bombard this city
				CvCity* pBombardCity = bombardTarget(pBestBombardPlot); // is NULL if city cannot be bombarded any more

				if (pBombardCity == NULL || pBombardCity->getDefenseDamage() < ((GC.getMAX_CITY_DEFENSE_DAMAGE()*5)/6))
				{
					kPlayer.AI_wakePlotTargetMissionAIs(pBestBombardPlot, MISSIONAI_BLOCKADE, getGroup());
				}
			}
			// otherwise, skip until next turn, when we will surely bombard
			else if (canPlunder(pBestBombardPlot))
			{
				getGroup()->pushMission(MISSION_PLUNDER, -1, -1, 0, false, false, MISSIONAI_BLOCKADE, pBestBombardPlot);
			}
			else
			{
				getGroup()->pushMission(MISSION_SKIP);
			}

			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BLOCKADE, pBestBombardPlot);
			return true;
		}
	}

	return false;
}



// Returns true if a mission was pushed...
bool CvUnitAI::AI_pillage(int iBonusValueThreshold, int iFlags)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestPillagePlot;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iI;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestPillagePlot = NULL;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot) && !(pLoopPlot->isBarbarian()))
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/22/10                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
			//if (potentialWarAction(pLoopPlot))
			if( pLoopPlot->isOwned() && isEnemy(pLoopPlot->getTeam(),pLoopPlot) )
			{
			    CvCity * pWorkingCity = pLoopPlot->getWorkingCity();

			    if (pWorkingCity != NULL)
			    {
                    if (!(pWorkingCity == area()->getTargetCity(getOwnerINLINE())) && canPillage(pLoopPlot))
                    {
                        if (!(pLoopPlot->isVisibleEnemyUnit(this)))
                        {
                            if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_PILLAGE, getGroup(), 1) == 0)
                            {
								iValue = AI_pillageValue(pLoopPlot, iBonusValueThreshold);
								iValue *= 1000;

								// if not at war with this plot owner, then devalue plot if we already inside this owner's borders
								// (because declaring war will pop us some unknown distance away)
								if (!isEnemy(pLoopPlot->getTeam()) && plot()->getTeam() == pLoopPlot->getTeam())
								{
									iValue /= 10;
								}

								if( iValue > iBestValue )
								{
									if (generatePath(pLoopPlot, iFlags, true, &iPathTurns))
									{
										iValue /= (iPathTurns + 1);

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = getPathEndTurnPlot();
											pBestPillagePlot = pLoopPlot;
										}
									}
                                }
							}
                        }
                    }
			    }
			}
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	}

	if ((pBestPlot != NULL) && (pBestPillagePlot != NULL))
	{
		if (atPlot(pBestPillagePlot) && !isEnemy(pBestPillagePlot->getTeam()))
		{
			//getGroup()->groupDeclareWar(pBestPillagePlot, true);
			// rather than declare war, just find something else to do, since we may already be deep in enemy territory
			return false;
		}
		
		if (atPlot(pBestPillagePlot))
		{
			if (isEnemy(pBestPillagePlot->getTeam()))
			{
				getGroup()->pushMission(MISSION_PILLAGE, -1, -1, 0, false, false, MISSIONAI_PILLAGE, pBestPillagePlot);
				return true;
			}
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), iFlags, false, false, MISSIONAI_PILLAGE, pBestPillagePlot);
			return true;
		}
	}

	return false;
}

/*  advc.003j: This Vanilla Civ 4 function was, apparently, never used.
	I think it says that it's OK to pillage unowned tiles when they don't
	belong to the trade network of a non-hostile civ. Seems sensible.
	The current AI code (see AI_pillage above) never pillages unowned tiles. */
/*bool CvUnitAI::AI_canPillage(CvPlot& kPlot) const
{
	if (isEnemy(kPlot.getTeam(), &kPlot))
	{
		return true;
	}

	if (!kPlot.isOwned())
	{
		bool bPillageUnowned = true;

		for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS && bPillageUnowned; ++iPlayer)
		{
			int iIndx;
			CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
			if (!isEnemy(kLoopPlayer.getTeam(), &kPlot))
			{
				for (CvCity* pCity = kLoopPlayer.firstCity(&iIndx); NULL != pCity; pCity = kLoopPlayer.nextCity(&iIndx))
				{
					if (kPlot.getPlotGroup((PlayerTypes)iPlayer) == pCity->plot()->getPlotGroup((PlayerTypes)iPlayer))
					{
						bPillageUnowned = false;
						break;
					}

				}
			}
		}

		if (bPillageUnowned)
		{
			return true;
		}
	}

	return false;
}*/


// Returns true if a mission was pushed...
bool CvUnitAI::AI_pillageRange(int iRange, int iBonusValueThreshold, int iFlags)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestPillagePlot;
	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	iSearchRange = AI_searchRange(iRange);

	iBestValue = 0;
	pBestPlot = NULL;
	pBestPillagePlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);
			// <advc.003>
			if (pLoopPlot == NULL || !AI_plotValid(pLoopPlot) ||
					pLoopPlot->isBarbarian() || !potentialWarAction(pLoopPlot))
				continue; // </advc.003>
			// <advc.033>
			if(isAlwaysHostile(pLoopPlot) && pLoopPlot->isOwned() &&
					!GET_PLAYER(getOwnerINLINE()).AI_isPiracyTarget(
					pLoopPlot->getOwnerINLINE()))
				continue; // </advc.033>
			CvCity * pWorkingCity = pLoopPlot->getWorkingCity();
			// <advc.003>
			if(pWorkingCity == NULL)
				continue; // </advc.003>
			if ((pWorkingCity != area()->getTargetCity(getOwnerINLINE())
				/*  advc.001: Barbarians perhaps shouldn't have a target city
					at all. At any rate, they should not exclude that city from
					pillaging. Barbarians are supposed to be destructive and
					shortsighted. More importantly, it's inexplicable for most
					human players when the Barbarians repeatedly spare improvements. */
					|| isBarbarian())
					&& canPillage(pLoopPlot)) { // <advc.003>
				if(pLoopPlot->isVisibleEnemyUnit(this) ||
						!GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(
						pLoopPlot, MISSIONAI_PILLAGE, getGroup()) == 0 ||
						!generatePath(pLoopPlot, iFlags, true, &iPathTurns, iRange))
					continue; // </advc.003>
				if (getPathFinder().GetFinalMoves() == 0)
				{
					iPathTurns++;
				}

				if (iPathTurns <= iRange)
				{
					iValue = AI_pillageValue(pLoopPlot, iBonusValueThreshold);

					iValue *= 1000;

					iValue /= (iPathTurns + 1);

					// if not at war with this plot owner, then devalue plot if we already inside this owner's borders
					// (because declaring war will pop us some unknown distance away)
					if (!isEnemy(pLoopPlot->getTeam()) && plot()->getTeam() == pLoopPlot->getTeam())
					{
						iValue /= 10;
					}

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestPlot = getPathEndTurnPlot();
						pBestPillagePlot = pLoopPlot;
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestPillagePlot != NULL))
	{
		if (atPlot(pBestPillagePlot) && !isEnemy(pBestPillagePlot->getTeam()))
		{
			//getGroup()->groupDeclareWar(pBestPillagePlot, true);
			// rather than declare war, just find something else to do, since we may already be deep in enemy territory
			return false;
		}
		
		if (atPlot(pBestPillagePlot))
		{
			if (isEnemy(pBestPillagePlot->getTeam()))
			{
				getGroup()->pushMission(MISSION_PILLAGE, -1, -1, 0, false, false, MISSIONAI_PILLAGE, pBestPillagePlot);
				return true;
			}
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), iFlags, false, false, MISSIONAI_PILLAGE, pBestPillagePlot);
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_found(int iFlags)
{
	PROFILE_FUNC();
//	advc.003: Most of the original code deleted
//	CvPlot* pLoopPlot;
//	...
//	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
//	{
//		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
//		...
//	}

	int iPathTurns;
	int iValue;
	int iBestFoundValue = 0;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestFoundPlot = NULL;
	// <advc.052>
	double plusMinus = 0;
	if(!isHuman() && GC.getGameINLINE().isScenario())
		plusMinus = 0.04; // </advc.052>
	for (int iI = 0; iI < GET_PLAYER(getOwnerINLINE()).AI_getNumCitySites(); iI++)
	{
		CvPlot* pCitySitePlot = GET_PLAYER(getOwnerINLINE()).AI_getCitySite(iI);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/23/09                                jdog5000      */
/*                                                                                              */
/* Settler AI                                                                                   */
/************************************************************************************************/
/* orginal BTS code
		if (pCitySitePlot->getArea() == getArea())
*/
		if (pCitySitePlot->getArea() == getArea() || canMoveAllTerrain())
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		{
			if (canFound(pCitySitePlot))
			{
				if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pCitySitePlot, MISSIONAI_FOUND, getGroup()) == 0)
				{
					if (getGroup()->canDefend() || GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pCitySitePlot, MISSIONAI_GUARD_CITY) > 0)
					{
						if (generatePath(pCitySitePlot, iFlags, true, &iPathTurns))
						{
							if (!pCitySitePlot->isVisible(getTeam(), false) || !pCitySitePlot->isVisibleEnemyUnit(this) || (iPathTurns > 1 && getGroup()->canDefend())) // K-Mod
							{
								iValue = pCitySitePlot->getFoundValue(getOwnerINLINE());
								// <advc.052>
								double randMult = 1 - plusMinus + 2 * plusMinus *
										::hash(m_iBirthmark);
								iValue = ::round(iValue * randMult);
								// </advc.052>
								iValue *= 1000;
								//iValue /= (iPathTurns + 1);
								iValue /= iPathTurns + (getGroup()->canDefend() ? 4 : 1); // K-Mod
								if (iValue > iBestFoundValue)
								{
									iBestFoundValue = iValue;
									pBestPlot = getPathEndTurnPlot();
									pBestFoundPlot = pCitySitePlot;
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestFoundPlot != NULL))
	{
		if (atPlot(pBestFoundPlot))
		{
			if( gUnitLogLevel >= 2 )
			{
				logBBAI("    Settler founding at site %d, %d", pBestFoundPlot->getX_INLINE(), pBestFoundPlot->getY_INLINE());
			}
			getGroup()->pushMission(MISSION_FOUND, -1, -1, 0, false, false, MISSIONAI_FOUND, pBestFoundPlot);
			return true;
		}
		else
		{
			if( gUnitLogLevel >= 2 )
			{
				logBBAI("    Settler heading for site %d, %d", pBestFoundPlot->getX_INLINE(), pBestFoundPlot->getY_INLINE());
			}
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), iFlags, false, false, MISSIONAI_FOUND, pBestFoundPlot);
			return true;
		}
	}

	return false;
}


/* original bts code - disabled by K-Mod
// Returns true if a mission was pushed...
bool CvUnitAI::AI_foundRange(int iRange, bool bFollow)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestFoundPlot;
	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	iSearchRange = AI_searchRange(iRange);

	iBestValue = 0;
	pBestPlot = NULL;
	pBestFoundPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot) && (pLoopPlot != plot() || GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(pLoopPlot, 1) <= pLoopPlot->plotCount(PUF_canDefend, -1, -1, getOwnerINLINE())))
				{
					if (canFound(pLoopPlot))
					{
						iValue = pLoopPlot->getFoundValue(getOwnerINLINE());

						if (iValue > iBestValue)
						{
							if (!(pLoopPlot->isVisibleEnemyUnit(this)))
							{
								if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_FOUND, getGroup(), 3) == 0)
								{
									if (generatePath(pLoopPlot, MOVE_SAFE_TERRITORY, true, &iPathTurns))
									{
										if (iPathTurns <= iRange)
										{
											iBestValue = iValue;
											pBestPlot = getPathEndTurnPlot();
											pBestFoundPlot = pLoopPlot;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestFoundPlot != NULL))
	{
		if (atPlot(pBestFoundPlot))
		{
			getGroup()->pushMission(MISSION_FOUND, -1, -1, 0, false, false, MISSIONAI_FOUND, pBestFoundPlot);
			return true;
		}
		else if (!bFollow)
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_SAFE_TERRITORY, false, false, MISSIONAI_FOUND, pBestFoundPlot);
			return true;
		}
	}

	return false;
} */

// K-Mod: this function simply checks if we are standing at our target destination
// and if we are, we issue the found command and return true.
// I've disabled (badly flawed) AI_foundRange, which was previously used for 'follow' AI.
bool CvUnitAI::AI_foundFollow()
{
	if (canFound(plot()) && getGroup()->AI_getMissionAIPlot() == plot() && getGroup()->AI_getMissionAIType() == MISSIONAI_FOUND)
	{
		if( gUnitLogLevel >= 2 )
		{
			logBBAI("    Settler founding at plot %d, %d (follow)", getX_INLINE(), getY_INLINE());
		}
		getGroup()->pushMission(MISSION_FOUND);
		return true;
	}

	return false;
}

// K-Mod. helper function for AI_assaultSeaTransport. (just to avoid code duplication)
static int estimateAndCacheCityDefence(CvPlayerAI& kPlayer, CvCity* pCity, std::map<CvCity*, int>& city_defence_cache)
{
	// calculate the city's defences, or read from the cache if we've already done it.
	std::map<CvCity*, int>::iterator city_it = city_defence_cache.find(pCity);
	int iDefenceStrength = -1;
	if (city_it == city_defence_cache.end())
	{
		if (pCity->plot()->isVisible(kPlayer.getTeam(), false))
		{
			iDefenceStrength = kPlayer.AI_localDefenceStrength(pCity->plot(), NO_TEAM);
		}
		else
		{
			// If we don't have vision of the city, we should try to estimate its strength based the expected number of defenders.
			int iUnitStr = GET_PLAYER(pCity->getOwnerINLINE()).getTypicalUnitValue(UNITAI_CITY_DEFENSE, DOMAIN_LAND) * GC.getGameINLINE().getBestLandUnitCombat() / 100;
			iDefenceStrength = std::max(GET_TEAM(kPlayer.getTeam()).AI_getStrengthMemory(pCity->plot()), pCity->AI_neededDefenders()*iUnitStr);
		}
		city_defence_cache[pCity] = iDefenceStrength;
	}
	else
	{
		// use the cached value
		iDefenceStrength = city_it->second;
	}
	return iDefenceStrength;
}
// K-Mod end

// Returns true if a mission was pushed...
// This function has been mostly rewritten for K-Mod.
bool CvUnitAI::AI_assaultSeaTransport(bool bAttackBarbs, bool bLocal)
{
	PROFILE_FUNC();

	//bool bAttackCity = (getUnitAICargo(UNITAI_ATTACK_CITY) > 0);

	FAssert(getGroup()->hasCargo());
	//FAssert(bAttackCity || getGroup()->getUnitAICargo(UNITAI_ATTACK) > 0);

	/* original bts code
	if (!canCargoAllMove())
	{
		return false;
	} */ // disabled by K-Mod. (this is now checked in AI_assaultGoTo)

	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());

	int iLimitedAttackers = 0;
	int iAmphibiousAttackers = 0;
	int iAmphibiousAttackStrength = 0;
	int iLandedAttackStrength = 0;
	int iCollateralDamageScale = estimateCollateralWeight(0, getTeam());
	std::map<CvCity*, int> city_defence_cache;

	std::vector<CvUnit*> aGroupCargo;
	CLLNode<IDInfo>* pUnitNode = plot()->headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = plot()->nextUnitNode(pUnitNode);
		CvUnit* pTransport = pLoopUnit->getTransportUnit();
		if (pTransport != NULL && pTransport->getGroup() == getGroup())
		{
			aGroupCargo.push_back(pLoopUnit);
			// K-Mod. Gather some data for later...
			iLimitedAttackers += (pLoopUnit->combatLimit() < 100 ? 1 : 0);
			iAmphibiousAttackers += (pLoopUnit->isAmphib() ? 1 : 0);

			// Estimate attack strength, both for landed assaults and amphibious assaults.
			//
			// Unfortunately, we can't use AI_localAttackStrength because that may miscount
			// depending on whether there is another group on this plot and things like that,
			// and we can't use AI_sumStrength because that currently only works for groups.
			// What we have here is a list of cargo units rather than a group.
			if (pLoopUnit->canAttack())
			{
				int iUnitStr = pLoopUnit->currEffectiveStr(NULL, NULL);

				iUnitStr *= 100 + 4 * pLoopUnit->firstStrikes() + 2 * pLoopUnit->chanceFirstStrikes();
				iUnitStr /= 100;

				if (pLoopUnit->collateralDamage() > 0)
					iUnitStr += pLoopUnit->baseCombatStr() * iCollateralDamageScale * pLoopUnit->collateralDamage() * pLoopUnit->collateralDamageMaxUnits() / 10000;

				iLandedAttackStrength += iUnitStr;

				if (pLoopUnit->combatLimit() >= 100 && pLoopUnit->canMove() && (!pLoopUnit->isMadeAttack() || pLoopUnit->isBlitz()))
				{
					if (!pLoopUnit->isAmphib())
						iUnitStr += iUnitStr * GC.getAMPHIB_ATTACK_MODIFIER() / 100;

					iAmphibiousAttackStrength += iUnitStr;
				}
			}
			// K-Mod end
		}
	}

	int iFlags = MOVE_AVOID_ENEMY_WEIGHT_3 | MOVE_DECLARE_WAR; // K-Mod
	int iCargo = getGroup()->getCargo();
	FAssert(iCargo > 0);
	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestAssaultPlot = NULL;

	// K-Mod note: I've restructured and rewritten this section for efficiency, clarity, and sometimes even to improve the AI!
	// Most of the original code has been deleted.
	for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
		if (!pLoopPlot->isRevealed(getTeam(), false))
			continue;
		if (!pLoopPlot->isOwned())
			continue;
		if (!bAttackBarbs && pLoopPlot->isBarbarian() && !kOwner.isMinorCiv())
			continue;
		if (!pLoopPlot->isCoastalLand())
			continue;
		if (!isPotentialEnemy(pLoopPlot->getTeam(), pLoopPlot))
			continue;
		// <advc.306>
		if(isBarbarian() && pLoopPlot->getTeam() != NO_TEAM &&
				pLoopPlot->area()->isBorderObstacle(pLoopPlot->getTeam()))
			continue; // </advc.306>

		// Note: currently these condtions mean we will never land to invade land-locked enemies

		int iTargetCities = pLoopPlot->area()->getCitiesPerPlayer(pLoopPlot->getOwnerINLINE());
		if (iTargetCities == 0)
			continue;

		int iPathTurns;
		if (!generatePath(pLoopPlot, iFlags, true, &iPathTurns))
			continue;

		CvCity* pCity = pLoopPlot->getPlotCity();
		// If the plot can't be seen, then just roughly estimate what the AI might think is there...
		int iEnemyDefenders = (pLoopPlot->isVisible(getTeam(), false) || GET_TEAM(getTeam()).AI_getStrengthMemory(pLoopPlot))
			? pLoopPlot->getNumVisiblePotentialEnemyDefenders(this)
			: (pCity ? pCity->AI_neededDefenders() : 0);

		int iBaseValue = 10 + std::min(9, 3*iTargetCities);
		int iValueMultiplier = 100;

		// if there are defenders, we should decide whether or not it is worth attacking them amphibiously.
		if (iEnemyDefenders > 0)
		{
			if ((iLimitedAttackers > 0 || iAmphibiousAttackers < iCargo/2) && iEnemyDefenders*3 > 2*(iCargo-iLimitedAttackers))
				continue;

			int iDefenceStrength = -1;
			if (pCity)
				iDefenceStrength = estimateAndCacheCityDefence(kOwner, pCity, city_defence_cache);
			else
				iDefenceStrength = pLoopPlot->isVisible(getTeam(), false) ? kOwner.AI_localDefenceStrength(pLoopPlot, NO_TEAM) : GET_TEAM(kOwner.getTeam()).AI_getStrengthMemory(pLoopPlot);
			// Note: the amphibious attack modifier is already taken into account by AI_localAttackStrength,
			// but I'm going to apply a similar penality again just to discourage the AI from attacking amphibiously when they don't need to.
			iDefenceStrength -= iDefenceStrength * GC.getAMPHIB_ATTACK_MODIFIER() * (iCargo - iAmphibiousAttackers) / (100*iCargo);

			if (iAmphibiousAttackStrength * 100 < iDefenceStrength * GC.getBBAI_ATTACK_CITY_STACK_RATIO())
				continue;

			if (pCity == NULL)
				iValueMultiplier = iValueMultiplier * (iAmphibiousAttackStrength - iDefenceStrength*std::min(iCargo-iLimitedAttackers, iEnemyDefenders)/iEnemyDefenders) / iAmphibiousAttackStrength;
		}

		if (pCity == NULL)
		{
			// consider landing on strategic resources
			iBaseValue += AI_pillageValue(pLoopPlot, 15);

			int iModifier = 0;
			// prefer to land on a defensive plot, but not with a river between us and the city
			// advc.001: Moved. That's a consideration for an adjacent city.
			/*if (pCity && pLoopPlot->isRiverCrossing(directionXY(pLoopPlot, pCity->plot())))
				iModifier += GC.getRIVER_ATTACK_MODIFIER()/10;*/
			
			//iModifier += pLoopPlot->defenseModifier(getTeam(), false) / 10;
			// advc.012: Replacing the above
			iModifier += AI_plotDefense(pLoopPlot) / 10;
			// advc.001: See the comment above. This will have to move too.
			//iValueMultiplier = (100+iModifier)*iValueMultiplier / 100;

			// Look look for adjacent cities.
			// advc.001: Better to use a separate variable for this
			CvCity* pAdjacentCity = NULL;
			for (DirectionTypes dir = (DirectionTypes)0; dir < NUM_DIRECTION_TYPES; dir=(DirectionTypes)(dir+1))
			{
				CvPlot* pAdjacentPlot = plotDirection(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), dir);
				if (pAdjacentPlot == NULL)
					continue;

				pAdjacentCity = pAdjacentPlot->getPlotCity();
				if (pAdjacentCity != NULL)
				{
					if (pAdjacentCity->getOwnerINLINE() == pLoopPlot->getOwnerINLINE())
						break;
					pAdjacentCity = NULL;
				}
			} // <advc.001>
			if(pAdjacentCity != NULL) {
				pCity = pAdjacentCity;
				// Copied from above
				if(pLoopPlot->isRiverCrossing(directionXY(pLoopPlot, pCity->plot())))
					iModifier += GC.getRIVER_ATTACK_MODIFIER()/10;
			} // Also copied from above
			iValueMultiplier *= (100 + iModifier);
			iValueMultiplier /= 100;
			// </advc.001>
		}


		if (pCity != NULL)
		{
			int iDefenceStrength = estimateAndCacheCityDefence(kOwner, pCity, city_defence_cache);

			FAssert(isPotentialEnemy(pCity->getTeam(), pLoopPlot));
			iBaseValue += kOwner.AI_targetCityValue(pCity, false, false); // maybe false, true?

			if (pCity->plot() == pLoopPlot)
				iValueMultiplier*=pLoopPlot->isVisible(getTeam(), false) ? 5 : 2; // apparently we can take the city amphibiously
			else
			{
				// prefer to join existing assaults. (maybe we should calculate the actual attack strength here and roll it into the strength comparison modifier below)
				int iModifier = std::min(kOwner.AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_ASSAULT, getGroup()) + kOwner.AI_adjacentPotentialAttackers(pCity->plot()), 2*iCargo) * 100 / iCargo;
				iValueMultiplier = (100+iModifier)*iValueMultiplier / 100;

				// Prefer to target cities that we can defeat.
				// However, keep in mind that if we often won't be able to see the city to gauge their defenses.


				if (iDefenceStrength > 0 || pLoopPlot->isVisible(getTeam(), false)) // otherwise, assume we have no idea what's there.
				{
					if (pLoopPlot->isVisible(getTeam(), false))
						iModifier = std::min(100, 125*iLandedAttackStrength / std::max(1, iDefenceStrength) - 25);
					else
						iModifier = std::min(50, 75*iLandedAttackStrength / std::max(1, iDefenceStrength) - 25);
				}

				iValueMultiplier = (100+iModifier)*iValueMultiplier / 100;
			}
		}

		// Continue attacking in area we have already captured cities
		if (pLoopPlot->area()->getCitiesPerPlayer(getOwnerINLINE()) > 0)
		{
			if (pCity != NULL && (bLocal || pCity->AI_playerCloseness(getOwnerINLINE()) > 5))
			{
				iValueMultiplier = iValueMultiplier*3/2;
			}
		}
		else if (bLocal)
		{
			iValueMultiplier = iValueMultiplier*2/3;
		}

		// K-Mod note: It would be nice to use the pathfinder again here
		// to make sure we aren't landing a long way from any enemy cities;
		// otherwise the AI might get into a loop of contantly dropping
		// off units which just walk back to the city to be dropped off again.
		// (maybe some other time)

		FAssert(iPathTurns > 0);

		/* some old bts code. The ideas here are worth remembering, but the execution is not suitable.
		if (iPathTurns == 1)
		{
			if (pCity != NULL)
			{
				if (pCity->area()->getNumCities() > 1)
				{
					iValue *= 2;
				}
			}
		}

		iValue *= 1000;

		if (iTargetCities <= iAssaultsHere)
		{
			iValue /= 2;
		}

		if (iTargetCities == 1)
		{
			if (iCargo > 7)
			{
				iValue *= 3;
				iValue /= iCargo - 4;
			}
		}

		if (pLoopPlot->isCity())
		{
			if (iEnemyDefenders * 3 > iCargo)
			{
				iValue /= 10;
			}
			else
			{
				iValue *= iCargo;
				iValue /= std::max(1, (iEnemyDefenders * 3));
			}
		}
		else
		{
			if (0 == iEnemyDefenders)
			{
				iValue *= 4;
				iValue /= 3;
			}
			else
			{
				iValue /= iEnemyDefenders;
			}
		}

		// if more than 3 turns to get there, then put some randomness into our preference of distance
		// +/- 33%
		if (iPathTurns > 3)
		{
			int iPathAdjustment = GC.getGameINLINE().getSorenRandNum(67, "AI Assault Target");

			iPathTurns *= 66 + iPathAdjustment;
			iPathTurns /= 100;
		}

		iValue /= (iPathTurns + 1); */
		// K-Mod. A bit of randomness is good, but if it's random every turn then it will lead to inconsistent decisions.
		// So.. if we're already en-route somewhere, try to keep going there.
		if (pCity && getGroup()->AI_getMissionAIPlot() && stepDistance(getGroup()->AI_getMissionAIPlot(), pCity->plot()) <= 1)
		{
			iValueMultiplier *= 150;
			iValueMultiplier /= 100;
		}
		else if (iPathTurns > 2)
		{
			//iValue *= 60 + GC.getGameINLINE().getSorenRandNum(81, "AI Assault Target");
			iValueMultiplier *= 60 + (AI_unitPlotHash(pLoopPlot, getGroup()->getNumUnits()) % 81);
			iValueMultiplier /= 100;
		}
		int iValue = iBaseValue * iValueMultiplier / (iPathTurns +2);
		// K-Mod end

		if (iValue > iBestValue)
		{
			iBestValue = iValue;
			pBestPlot = getPathEndTurnPlot();
			pBestAssaultPlot = pLoopPlot;
		}
	}

	if (pBestPlot != NULL && pBestAssaultPlot != NULL)
	{
		return AI_transportGoTo(pBestPlot, pBestAssaultPlot, iFlags, MISSIONAI_ASSAULT);
	}

	return false;
}

// Returns true if a mission was pushed...
// This function has been heavily edited and restructured by K-Mod. It includes some bbai changes.
bool CvUnitAI::AI_assaultSeaReinforce(bool bAttackBarbs)
{
	PROFILE_FUNC();

	bool bAttackCity = (getUnitAICargo(UNITAI_ATTACK_CITY) > 0);

	FAssert(getGroup()->hasCargo());
	FAssert(getGroup()->canAllMove()); // K-Mod (replacing a BBAI check that I'm sure is unnecessary.)

	std::vector<CvUnit*> aGroupCargo;
	CLLNode<IDInfo>* pUnitNode = plot()->headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = plot()->nextUnitNode(pUnitNode);
		CvUnit* pTransport = pLoopUnit->getTransportUnit();
		if (pTransport != NULL && pTransport->getGroup() == getGroup())
		{
			aGroupCargo.push_back(pLoopUnit);
		}
	}

	// bool bCity = plot()->isCity(true,getTeam());
	// K-Mod note: bCity was used in a few places, but it should not be. If we make a decision based on being in a city, then we'll just change our mind as soon as we leave the city!

	int iCargo = getGroup()->getCargo();
	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestAssaultPlot = NULL;
	CvArea* pWaterArea = plot()->waterArea();
	bool bCanMoveAllTerrain = getGroup()->canMoveAllTerrain();
	int iFlags = MOVE_AVOID_ENEMY_WEIGHT_3; // K-Mod. (no declare war)

	// Loop over nearby plots for groups in enemy territory to reinforce
	int iRange = 2*maxMoves();
	for (int iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (int iDY = -(iRange); iDY <= iRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot && isEnemy(pLoopPlot->getTeam(), pLoopPlot))
			{
				if ( bCanMoveAllTerrain || (pWaterArea != NULL && pLoopPlot->isAdjacentToArea(pWaterArea)) )
				{
					int iTargetCities = pLoopPlot->area()->getCitiesPerPlayer(pLoopPlot->getOwnerINLINE());

					if (iTargetCities > 0)
					{
						int iOurFightersHere = pLoopPlot->getNumDefenders(getOwnerINLINE());

						if( iOurFightersHere > 2 )
						{
							int iPathTurns;
							if (generatePath(pLoopPlot, iFlags, true, &iPathTurns, 2))
							{
								CvPlot* pEndTurnPlot = getPathEndTurnPlot();

								int iValue = 10*iTargetCities;
								iValue += 8*iOurFightersHere;
								iValue += 3*GET_PLAYER(getOwnerINLINE()).AI_adjacentPotentialAttackers(pLoopPlot);

								iValue *= 100;

								iValue /= (iPathTurns + 1);

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = pEndTurnPlot;
									pBestAssaultPlot = pLoopPlot;
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot && pBestAssaultPlot)
		return AI_transportGoTo(pBestPlot, pBestAssaultPlot, iFlags, MISSIONAI_REINFORCE);

	// Loop over other transport groups, looking for synchronized landing
	int iLoop;
	for(CvSelectionGroup* pLoopSelectionGroup = GET_PLAYER(getOwnerINLINE()).firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = GET_PLAYER(getOwnerINLINE()).nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup != getGroup())
		{
			//if (pLoopSelectionGroup->AI_getMissionAIType() == MISSIONAI_ASSAULT)
			if (pLoopSelectionGroup->AI_getMissionAIType() == MISSIONAI_ASSAULT && pLoopSelectionGroup->getHeadUnitAI() == UNITAI_ASSAULT_SEA) // K-Mod. (b/c assault is also used for ground units)
			{
				CvPlot* pLoopPlot = pLoopSelectionGroup->AI_getMissionAIPlot();

				if (pLoopPlot && isPotentialEnemy(pLoopPlot->getTeam(), pLoopPlot))
				{
					if ( bCanMoveAllTerrain || (pWaterArea != NULL && pLoopPlot->isAdjacentToArea(pWaterArea)) )
					{
						int iTargetCities = pLoopPlot->area()->getCitiesPerPlayer(pLoopPlot->getOwnerINLINE());
						if (iTargetCities <= 0)
							continue;

						int iAssaultsHere = pLoopSelectionGroup->getCargo();

						if (iAssaultsHere < 3)
							continue;

						int iPathTurns;
						if (generatePath(pLoopPlot, iFlags, true, &iPathTurns))
						{
							CvPlot* pEndTurnPlot = getPathEndTurnPlot();

							int iOtherPathTurns = MAX_INT;
							//if (pLoopSelectionGroup->generatePath(pLoopSelectionGroup->plot(), pLoopPlot, iFlags, true, &iOtherPathTurns))
							// K-Mod. Use a different pathfinder, so that we don't clear our path data.
							KmodPathFinder loop_path;
							loop_path.SetSettings(pLoopSelectionGroup, iFlags, iPathTurns);
							if (loop_path.GeneratePath(pLoopPlot))
								// K-Mod end
							{
								//iOtherPathTurns += 1;
								iOtherPathTurns = loop_path.GetPathTurns(); // (K-Mod note: I'm not convinced the +1 thing is a good idea.)
							}
							else
							{
								continue;
							}

							FAssert(iOtherPathTurns <= iPathTurns);
							if (iPathTurns >= iOtherPathTurns + 5)
								continue;

							int iEnemyDefenders = pLoopPlot->getNumVisibleEnemyDefenders(this);
							if (iEnemyDefenders > 0 || pLoopPlot->isCity())
							{
								bool bCanCargoAllUnload = true;
								for (uint i = 0; i < aGroupCargo.size(); ++i)
								{
									CvUnit* pAttacker = aGroupCargo[i];
									if (!pLoopPlot->hasDefender(true, NO_PLAYER, pAttacker->getOwnerINLINE(), pAttacker, true))
									{
										bCanCargoAllUnload = false;
										break;
									}
									else if( pLoopPlot->isCity() && !(pLoopPlot->isVisible(getTeam(),false)) )
									{
										// Artillery can't naval invade, so don't try
										if( pAttacker->combatLimit() < 100 )
										{
											bCanCargoAllUnload = false;
											break;
										}
									}
								}
							}

							int iValue = (iAssaultsHere * 5);
							iValue += iTargetCities*10;

							iValue *= 100;

							/* original bts code
							// if more than 3 turns to get there, then put some randomness into our preference of distance
							// +/- 33%
							if (iPathTurns > 3)
							{
								int iPathAdjustment = GC.getGameINLINE().getSorenRandNum(67, "AI Assault Target");

								iPathTurns *= 66 + iPathAdjustment;
								iPathTurns /= 100;
							}
							iValue /= (iPathTurns + 1);
							*/
							// K-Mod. More consistent randomness, to prevent decisions from oscillating.
							iValue *= 70 + (AI_unitPlotHash(pLoopPlot, getGroup()->getNumUnits()) % 61);
							iValue /= 100;

							iValue /= (iPathTurns + 2);
							// K-Mod end

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pEndTurnPlot;
								pBestAssaultPlot = pLoopPlot;
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot && pBestAssaultPlot)
		return AI_transportGoTo(pBestPlot, pBestAssaultPlot, iFlags, MISSIONAI_REINFORCE);

	// Reinforce our cities in need
	for (CvCity* pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if( bCanMoveAllTerrain || (pWaterArea != NULL && (pLoopCity->waterArea(true) == pWaterArea || pLoopCity->secondWaterArea() == pWaterArea)) )
		{
			int iValue = 0;
			if(pLoopCity->area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE)
			{
				iValue = 3;
			}
			else if(pLoopCity->area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE)
			{
				iValue = 2;
			}
			else if(pLoopCity->area()->getAreaAIType(getTeam()) == AREAAI_MASSING)
			{
				iValue = 1;
			}
			else if( bAttackBarbs && (pLoopCity->area()->getCitiesPerPlayer(BARBARIAN_PLAYER) > 0) )
			{
				iValue = 1;
			}
			else
				continue;

			bool bCityDanger = pLoopCity->AI_isDanger();
			//if( (bCity && pLoopCity->area() != area()) || bCityDanger || ((GC.getGameINLINE().getGameTurn() - pLoopCity->getGameTurnAcquired()) < 10 && pLoopCity->getPreviousOwner() != NO_PLAYER) )
			if (pLoopCity->area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE || bCityDanger || (GC.getGameINLINE().getGameTurn() - pLoopCity->getGameTurnAcquired() < 10 && pLoopCity->getPreviousOwner() != NO_PLAYER)) // K-Mod
			{
				int iOurPower = std::max(1, pLoopCity->area()->getPower(getOwnerINLINE()));
				// Enemy power includes barb power
				int iEnemyPower = GET_TEAM(getTeam()).countEnemyPowerByArea(pLoopCity->area());

				// Don't send troops to areas we are dominating already
				// Don't require presence of enemy cities, just a dangerous force
				if( iOurPower < (3*iEnemyPower) )
				{
					int iPathTurns;
					if (generatePath(pLoopCity->plot(), iFlags, true, &iPathTurns))
					{
						iValue *= 10*pLoopCity->AI_cityThreat();

						iValue += 20 * GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_ASSAULT, getGroup());

						iValue *= std::min(iEnemyPower, 3*iOurPower);
						iValue /= iOurPower;

						iValue *= 100;

						// if more than 3 turns to get there, then put some randomness into our preference of distance
						// +/- 33%
						if (iPathTurns > 3)
						{
							int iPathAdjustment = GC.getGameINLINE().getSorenRandNum(67, "AI Assault Target");

							iPathTurns *= 66 + iPathAdjustment;
							iPathTurns /= 100;
						}

						iValue /= (iPathTurns + 6);

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							//pBestPlot = (bCityDanger ? getPathEndTurnPlot() : pLoopCity->plot());
							pBestPlot = getPathEndTurnPlot(); // K-Mod (why did they have that other stuff?)
							pBestAssaultPlot = pLoopCity->plot();
						}
					}
				}
			}
		}
	}

	if (pBestPlot && pBestAssaultPlot)
		return AI_transportGoTo(pBestPlot, pBestAssaultPlot, iFlags, MISSIONAI_REINFORCE);

	// assist master in attacking
	if( GET_TEAM(getTeam()).isAVassal() )
	{
		TeamTypes eMasterTeam = NO_TEAM;

		for( int iI = 0; iI < MAX_CIV_TEAMS; iI++ )
		{
			if( GET_TEAM(getTeam()).isVassal((TeamTypes)iI) )
			{
				eMasterTeam = (TeamTypes)iI;
			}
		}

		if( (eMasterTeam != NO_TEAM) && GET_TEAM(getTeam()).isOpenBorders(eMasterTeam) )
		{
			for( int iI = 0; iI < MAX_CIV_PLAYERS; iI++ )
			{
				if( GET_PLAYER((PlayerTypes)iI).getTeam() == eMasterTeam )
				{
					for (CvCity* pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
					{
						if (pLoopCity->area() == area())
							continue; // otherwise we can just walk there. (probably)

						int iValue = 0;
						switch (pLoopCity->area()->getAreaAIType(eMasterTeam))
						{
						case AREAAI_OFFENSIVE:
							iValue = 2;
							break;

						case AREAAI_MASSING:
							iValue = 1;
							break;

						default:
							continue; // not an appropriate area.
						}

						if( bCanMoveAllTerrain || (pWaterArea != NULL && (pLoopCity->waterArea(true) == pWaterArea || pLoopCity->secondWaterArea() == pWaterArea)) )
						{
							int iOurPower = std::max(1, pLoopCity->area()->getPower(getOwnerINLINE()));
							iOurPower += GET_TEAM(eMasterTeam).countPowerByArea(pLoopCity->area());
							// Enemy power includes barb power
							int iEnemyPower = GET_TEAM(eMasterTeam).countEnemyPowerByArea(pLoopCity->area());

							// Don't send troops to areas we are dominating already
							// Don't require presence of enemy cities, just a dangerous force
							if( iOurPower < (2*iEnemyPower) )
							{
								int iPathTurns;
								if (generatePath(pLoopCity->plot(), iFlags, true, &iPathTurns))
								{
									iValue *= pLoopCity->AI_cityThreat();

									iValue += 10 * GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_ASSAULT, getGroup());

									iValue *= std::min(iEnemyPower, 3*iOurPower);
									iValue /= iOurPower;

									iValue *= 100;

									// if more than 3 turns to get there, then put some randomness into our preference of distance
									// +/- 33%
									if (iPathTurns > 3)
									{
										int iPathAdjustment = GC.getGameINLINE().getSorenRandNum(67, "AI Assault Target");

										iPathTurns *= 66 + iPathAdjustment;
										iPathTurns /= 100;
									}

									iValue /= (iPathTurns + 1);

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
										pBestAssaultPlot = pLoopCity->plot();
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot && pBestAssaultPlot)
		return AI_transportGoTo(pBestPlot, pBestAssaultPlot, iFlags, MISSIONAI_REINFORCE);

	return false;
}

// K-Mod. General function for moving assault groups - to reduce code duplication.
bool CvUnitAI::AI_transportGoTo(CvPlot* pEndTurnPlot, CvPlot* pTargetPlot, int iFlags, MissionAITypes eMissionAI)
{
	FAssert(pEndTurnPlot && pTargetPlot);
	FAssert(!pEndTurnPlot->isImpassable());

	if (getGroup()->AI_getMissionAIType() != eMissionAI)
	{
		// Cancel missions of all those coming to join departing transport
		int iLoop = 0;
		CvPlayer& kOwner = GET_PLAYER(getOwnerINLINE());

		for (CvSelectionGroup* pLoopGroup = kOwner.firstSelectionGroup(&iLoop); pLoopGroup != NULL; pLoopGroup = kOwner.nextSelectionGroup(&iLoop))
		{
			if (pLoopGroup != getGroup())
			{
				if (pLoopGroup->AI_getMissionAIType() == MISSIONAI_GROUP)
				{
					CvUnit* pMissionUnit = pLoopGroup->AI_getMissionAIUnit();

					if (pMissionUnit && pMissionUnit->getGroup() == getGroup() && !pLoopGroup->isFull())
					{
						pLoopGroup->clearMissionQueue();
						pLoopGroup->AI_setMissionAI(NO_MISSIONAI, 0, 0);
					}
				}
			}
		}
	}

	if (atPlot(pTargetPlot))
	{
		getGroup()->unloadAll(); // XXX is this dangerous (not pushing a mission...) XXX air units?
		getGroup()->setActivityType(ACTIVITY_AWAKE); // K-Mod
		return true;
	}
	else
	{
		if (getGroup()->isAmphibPlot(pTargetPlot))
		{
			// If target is actually an amphibious landing from pEndTurnPlot, then set pEndTurnPlot = pTargetPlot so that we can land this turn.
			if (pTargetPlot != pEndTurnPlot && stepDistance(pTargetPlot, pEndTurnPlot) == 1)
			{
				pEndTurnPlot = pTargetPlot;
			}

			// if our cargo isn't going to be ready to land, just wait.
			if (pTargetPlot == pEndTurnPlot && !getGroup()->canCargoAllMove())
			{
				getGroup()->pushMission(MISSION_SKIP, -1, -1, iFlags, false, false, eMissionAI, pTargetPlot);
				return true;
			}
		}

		// declare war if we need to. (Note: AI_considerPathDOW checks for the declare war flag.)
		if (AI_considerPathDOW(pEndTurnPlot, iFlags))
		{	// <advc.163>
			if(!canMove())
				return true; // </advc.163>
			if (!generatePath(pTargetPlot, iFlags, false))
				return false;
			pEndTurnPlot = getPathEndTurnPlot();
		}

		// Group all moveable land units together before landing,
		// this will help the AI to think more clearly about attacking on the next turn.
		if (pEndTurnPlot == pTargetPlot && !pTargetPlot->isWater() && !pTargetPlot->isCity())
		{
			CvSelectionGroup* pCargoGroup = NULL;
			CLLNode<IDInfo>* pUnitNode = getGroup()->headUnitNode();

			while (pUnitNode != NULL)
			{
				CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = getGroup()->nextUnitNode(pUnitNode);

				std::vector<CvUnit*> cargo_units;
				pLoopUnit->getCargoUnits(cargo_units);

				for (size_t i = 0; i < cargo_units.size(); i++)
				{
					if (cargo_units[i]->getGroup() != pCargoGroup &&
						cargo_units[i]->getDomainType() == DOMAIN_LAND && cargo_units[i]->canMove())
					{
						if (pCargoGroup)
						{
							cargo_units[i]->joinGroup(pCargoGroup);
						}
						else
						{
							if (!cargo_units[i]->getGroup()->canAllMove())
							{
								cargo_units[i]->joinGroup(NULL); // separate from units that can't move.
							}
							pCargoGroup = cargo_units[i]->getGroup();
						}
					}
				}
			}
		}
		//
		getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX_INLINE(), pEndTurnPlot->getY_INLINE(), iFlags, false, false, eMissionAI, pTargetPlot);
		return true;
	}
}
// K-Mod end

// Returns true if a mission was pushed...
bool CvUnitAI::AI_settlerSeaTransport()
{
	PROFILE_FUNC();

	FAssert(getCargo() > 0);
	FAssert(getUnitAICargo(UNITAI_SETTLE) > 0);

	if (!getGroup()->canCargoAllMove())
	{
		return false;
	}

	//New logic should allow some new tricks like 
	//unloading settlers when a better site opens up locally
	//and delivering settlers
	//to inland sites

	FAssertMsg(plot()->waterArea() != NULL, "Ship out of water?");

	CvUnit* pSettlerUnit = NULL;
	CvPlot* pPlot = plot();
	CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();

	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = pPlot->nextUnitNode(pUnitNode);

		if (pLoopUnit->getTransportUnit() == this)
		{
			if (pLoopUnit->AI_getUnitAIType() == UNITAI_SETTLE)
			{
				pSettlerUnit = pLoopUnit;
				break;
			}
		}
	}
	
	FAssert(pSettlerUnit != NULL);
	
	int iAreaBestFoundValue = 0;
	CvPlot* pAreaBestPlot = NULL;

	int iOtherAreaBestFoundValue = 0;
	CvPlot* pOtherAreaBestPlot = NULL;

	KmodPathFinder land_path;
	land_path.SetSettings(pSettlerUnit->getGroup(), MOVE_SAFE_TERRITORY);

	for (int iI = 0; iI < GET_PLAYER(getOwnerINLINE()).AI_getNumCitySites(); iI++)
	{
		CvPlot* pCitySitePlot = GET_PLAYER(getOwnerINLINE()).AI_getCitySite(iI);
		if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pCitySitePlot, MISSIONAI_FOUND, getGroup()) == 0)
		{
			int iValue = pCitySitePlot->getFoundValue(getOwnerINLINE());
			//if (pCitySitePlot->getArea() == getArea())
			if (pCitySitePlot->getArea() == getArea() && land_path.GeneratePath(pCitySitePlot)) // K-Mod
			{
				if (iValue > iAreaBestFoundValue)
				{
					iAreaBestFoundValue = iValue;
					pAreaBestPlot = pCitySitePlot;
				}
			}
			else
			{
				if (iValue > iOtherAreaBestFoundValue)
				{
					iOtherAreaBestFoundValue = iValue;
					pOtherAreaBestPlot = pCitySitePlot;
				}
			}
		}
	}
	if (iAreaBestFoundValue == 0 && iOtherAreaBestFoundValue == 0)
	{
		return false;
	}
	
	if (iAreaBestFoundValue > iOtherAreaBestFoundValue)
	{
		//let the settler walk.
		getGroup()->unloadAll();
		getGroup()->pushMission(MISSION_SKIP);
		return true;
	}
	
	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestFoundPlot = NULL;

	for (int iI = 0; iI < GET_PLAYER(getOwnerINLINE()).AI_getNumCitySites(); iI++)
	{
		CvPlot* pCitySitePlot = GET_PLAYER(getOwnerINLINE()).AI_getCitySite(iI);
		if (!(pCitySitePlot->isVisibleEnemyUnit(this)))
		{
			if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pCitySitePlot, MISSIONAI_FOUND, getGroup(), 4) == 0)
			{
				int iPathTurns;
				// BBAI TODO: Nearby plots too if much shorter (settler walk from there)
				// also, if plots are in area player already has cities, then may not be coastal ... (see Earth 1000 AD map for Inca)
				if (generatePath(pCitySitePlot, 0, true, &iPathTurns))
				{
					int iValue = pCitySitePlot->getFoundValue(getOwnerINLINE());
					iValue *= 1000;
					iValue /= (2 + iPathTurns);
					
					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestPlot = getPathEndTurnPlot();
						pBestFoundPlot = pCitySitePlot;
					}
				}
			}
		}
	}

	if (pBestPlot != NULL && pBestFoundPlot != NULL)
	{
		FAssert(!(pBestPlot->isImpassable()));

		if ((pBestPlot == pBestFoundPlot) || (stepDistance(pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), pBestFoundPlot->getX_INLINE(), pBestFoundPlot->getY_INLINE()) == 1))
		{
			if (atPlot(pBestFoundPlot))
			{
				unloadAll(); // XXX is this dangerous (not pushing a mission...) XXX air units?
				getGroup()->setActivityType(ACTIVITY_AWAKE); // K-Mod
				return true;
			}
			else
			{
				getGroup()->pushMission(MISSION_MOVE_TO, pBestFoundPlot->getX_INLINE(), pBestFoundPlot->getY_INLINE(), 0, false, false, MISSIONAI_FOUND, pBestFoundPlot);
				return true;
			}
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_FOUND, pBestFoundPlot);
			return true;
		}
	}

	//Try original logic
	//(sometimes new logic breaks)
	pPlot = plot();

	iBestValue = 0;
	pBestPlot = NULL;
	pBestFoundPlot = NULL;
	
	int iMinFoundValue = GET_PLAYER(getOwnerINLINE()).AI_getMinFoundValue();

	for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		//if (pLoopPlot->isCoastalLand())
		// K-Mod. Only consider areas we have explored, and only land if we know there is something we want to settle.
		int iAreaBest; // (currently unused)
		if (pLoopPlot->isCoastalLand() && pLoopPlot->isRevealed(getTeam(), false) && GET_PLAYER(getOwnerINLINE()).AI_getNumAreaCitySites(pLoopPlot->getArea(), iAreaBest) > 0)
		// K-Mod end
		{
			int iValue = pLoopPlot->getFoundValue(getOwnerINLINE());

			if ((iValue > iBestValue) && (iValue >= iMinFoundValue))
			{
				bool bValid = false;

				pUnitNode = pPlot->headUnitNode();

				while (pUnitNode != NULL)
				{
					CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
					pUnitNode = pPlot->nextUnitNode(pUnitNode);

					if (pLoopUnit->getTransportUnit() == this)
					{
						if (pLoopUnit->canFound(pLoopPlot))
						{
							bValid = true;
							break;
						}
					}
				}

				if (bValid)
				{
					if (!(pLoopPlot->isVisibleEnemyUnit(this)))
					{
						if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_FOUND, getGroup(), 4) == 0)
						{
							if (generatePath(pLoopPlot, 0, true))
							{
								iBestValue = iValue;
								pBestPlot = getPathEndTurnPlot();
								pBestFoundPlot = pLoopPlot;
							}
						}
					}
				}
			}
		}
	}
	
	if ((pBestPlot != NULL) && (pBestFoundPlot != NULL))
	{
		FAssert(!(pBestPlot->isImpassable()));

		if ((pBestPlot == pBestFoundPlot) || (stepDistance(pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), pBestFoundPlot->getX_INLINE(), pBestFoundPlot->getY_INLINE()) == 1))
		{
			if (atPlot(pBestFoundPlot))
			{
				unloadAll(); // XXX is this dangerous (not pushing a mission...) XXX air units?
				getGroup()->setActivityType(ACTIVITY_AWAKE); // K-Mod
				return true;
			}
			else
			{
				getGroup()->pushMission(MISSION_MOVE_TO, pBestFoundPlot->getX_INLINE(), pBestFoundPlot->getY_INLINE(), 0, false, false, MISSIONAI_FOUND, pBestFoundPlot);
				return true;
			}
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_FOUND, pBestFoundPlot);
			return true;
		}
	}
	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_settlerSeaFerry()
{
	PROFILE_FUNC();

	FAssert(getCargo() > 0);
	FAssert(getUnitAICargo(UNITAI_WORKER) > 0);

	if (!getGroup()->canCargoAllMove())
		return false;

	CvArea* pWaterArea = plot()->waterArea();
	FAssertMsg(pWaterArea != NULL, "Ship out of water?");
	CvPlayerAI const& kOwner = GET_PLAYER(getOwnerINLINE());

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	int iLoop;
	for (CvCity* pLoopCity = kOwner.firstCity(&iLoop); pLoopCity != NULL;
			pLoopCity = kOwner.nextCity(&iLoop))
	{
		int iValue = pLoopCity->AI_getWorkersNeeded();
		if(iValue <= 0)
			continue;

		iValue -= kOwner.AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_FOUND, getGroup());
		if (iValue <= 0)
			continue;

		int iPathTurns;
		if (!generatePath(pLoopCity->plot(), 0, true, &iPathTurns))
			continue;

		iValue += std::max(0, kOwner.AI_neededWorkers(pLoopCity->area()) -
				kOwner.AI_totalAreaUnitAIs(pLoopCity->area(), UNITAI_WORKER));
		iValue *= 1000;
		iValue /= 4 + iPathTurns;
		if (atPlot(pLoopCity->plot()))
			iValue += 100;
		else iValue += GC.getGame().getSorenRandNum(100, "AI settler sea ferry");
		if (iValue > iBestValue)
		{
			iBestValue = iValue;
			pBestPlot = pLoopCity->plot();							
		}
	}
	// <advc.040>
	// Need a handle to a Worker so we can check if any improvement can actually be built
	CvUnit* pWorker = NULL;
	std::vector<CvUnit*> aCargoUnits;
	getCargoUnits(aCargoUnits);
	for(size_t i = 0; i < aCargoUnits.size(); i++) {
		if(aCargoUnits[i]->AI_getUnitAIType() == UNITAI_WORKER) {
			pWorker = aCargoUnits[i];
			break;
		}
	}
	FAssert(pWorker != NULL);
	if(iBestValue < 500 && pWorker != NULL) {
		CvUnitAI const& kWorker = pWorker->AI();
		CvMap const& m = GC.getMapINLINE();
		for(int i = 0; i < m.numPlotsINLINE(); i++) {
			CvPlot& kPlot = *m.plotByIndexINLINE(i);
			if(kPlot.isWater() || kPlot.getOwnerINLINE() != kOwner.getID() ||
					kPlot.area()->getCitiesPerPlayer(kOwner.getID()) > 0 ||
					kOwner.AI_neededWorkers(kPlot.area()) <= 0 ||
					kOwner.AI_totalAreaUnitAIs(kPlot.area(), UNITAI_WORKER) > 0)
				continue;
			CvCity* pWorkingCity = kPlot.getWorkingCity();
			BonusTypes eBonus = kPlot.getNonObsoleteBonusType(kOwner.getTeam());
			if(pWorkingCity == NULL && eBonus == NO_BONUS)
				continue;
			// Do a computation similar to CvCityAI::AI_neededWorkers on the fly
			ImprovementTypes eImpr = kPlot.getImprovementType();
			// Don't bother sending a Worker if there's already a good improvement
			if(eImpr != NO_IMPROVEMENT && (eBonus == NO_BONUS ||
					kOwner.doesImprovementConnectBonus(eImpr, eBonus)))
				continue;
			if(pWorkingCity != NULL) {
				CvCityAI const& kWorkingCity = pWorkingCity->AI();
				BuildTypes eBestBuild = kWorkingCity.AI_getBestBuild(kWorkingCity.
						getCityPlotIndex(&kPlot));
				if(eBestBuild == NO_BUILD || !kWorker.canBuild(&kPlot, eBestBuild))
					continue;
				ImprovementTypes eBestImpr = (ImprovementTypes)GC.getBuildInfo(eBestBuild).getImprovement();
				if(eBestImpr == NO_IMPROVEMENT) // Don't go there just to chop
					continue;
				// Not going to build forts on workable tiles
				if(GC.getImprovementInfo(eBestImpr).isActsAsCity())
					continue;
			}
			else {
				bool bValid = false;
				for(int j = 0; j < GC.getNumBuildInfos(); j++) {
					BuildTypes eBuild = (BuildTypes)j;
					if(kWorker.AI_canConnectBonus(kPlot, eBuild)) {
						bValid = true;
						break;
					}
				}
				if(!bValid)
					continue;
			}
			int iValue = 0;
			if(kPlot.getBonusType() != NO_BONUS) { // Could be obsolete
				iValue++;
				if(eBonus != NO_BONUS) // Non-obsolete
					iValue++;
			}
			if(pWorkingCity != NULL) { // Can be worked
				iValue++;
				if(kPlot.isBeingWorked())
					iValue++;
			}
			FAssert(iValue > 0);
			// Akin to the city loop above
			int iPathTurns;
			if(!generatePath(&kPlot, 0, true, &iPathTurns, 8))
				continue;
			// Check this last b/c it goes through all selection groups
			if(kOwner.AI_areaMissionAIs(kPlot.area(), MISSIONAI_FOUND, getGroup()) > 0)
				continue;
			/*  For cities, it's 4+... in the divisor. 1 extra to deprioritize islands.
				(Or we could say it's the extra turn for unloading outside a city.) */
			iValue = (1000 * iValue) / (5 + iPathTurns);
			iValue += GC.getGame().getSorenRandNum(100, "advc.040");
 			if(iValue > iBestValue) {
				iBestValue = iValue;
				pBestPlot = &kPlot;							
			}
		}
	} // </advc.040>

	if (pBestPlot != NULL)
	{
		if (atPlot(pBestPlot))
		{
			unloadAll(); // XXX is this dangerous (not pushing a mission...) XXX air units?
			getGroup()->setActivityType(ACTIVITY_AWAKE); // K-Mod
			return true;
		}
		else
		{	// <advc.040> Unload all but 1 Worker before going to an island
			if(!pBestPlot->isCity()) {
				int iWorkersFound = 0;
				for(size_t i = 0; i < aCargoUnits.size(); i++) {
					if(aCargoUnits[i]->AI_getUnitAIType() == UNITAI_WORKER) {
						iWorkersFound++;
						if(iWorkersFound > 1)
							aCargoUnits[i]->unload();
					}
					else aCargoUnits[i]->unload();
				}
			} // </advc.040>
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(),
					pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_FOUND, pBestPlot);
			return true;
		}
	}
	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_specialSeaTransportMissionary()
{
	//PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvCity* pCity;
	CvUnit* pMissionaryUnit;
	CvUnit* pLoopUnit;
	CvPlot* pLoopPlot;
	CvPlot* pPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestSpreadPlot;
	int iPathTurns;
	int iValue;
	int iCorpValue;
	int iBestValue;
	int iI, iJ;
	bool bExecutive = false;

	FAssert(getCargo() > 0);
	FAssert(getUnitAICargo(UNITAI_MISSIONARY) > 0);

	if (!getGroup()->canCargoAllMove())
	{
		return false;
	}

	pPlot = plot();

	pMissionaryUnit = NULL;

	pUnitNode = pPlot->headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = pPlot->nextUnitNode(pUnitNode);

		if (pLoopUnit->getTransportUnit() == this)
		{
			if (pLoopUnit->AI_getUnitAIType() == UNITAI_MISSIONARY)
			{
				pMissionaryUnit = pLoopUnit;
				break;
			}
		}
	}

	if (pMissionaryUnit == NULL)
	{
		return false;
	}

	iBestValue = 0;
	pBestPlot = NULL;
	pBestSpreadPlot = NULL;

	// XXX what about non-coastal cities?
	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (pLoopPlot->isCoastalLand())
		{
			pCity = pLoopPlot->getPlotCity();

			if (pCity != NULL)
			{
				iValue = 0;
				iCorpValue = 0;

				for (iJ = 0; iJ < GC.getNumReligionInfos(); iJ++)
				{
					if (pMissionaryUnit->canSpread(pLoopPlot, ((ReligionTypes)iJ)))
					{
						if (GET_PLAYER(getOwnerINLINE()).getStateReligion() == ((ReligionTypes)iJ))
						{
							iValue += 3;
						}

						if (GET_PLAYER(getOwnerINLINE()).hasHolyCity((ReligionTypes)iJ))
						{
							iValue++;
						}
					}
				}

				for (iJ = 0; iJ < GC.getNumCorporationInfos(); iJ++)
				{
					if (pMissionaryUnit->canSpreadCorporation(pLoopPlot, ((CorporationTypes)iJ)))
					{
						if (GET_PLAYER(getOwnerINLINE()).hasHeadquarters((CorporationTypes)iJ))
						{
							iCorpValue += 3;
						}
					}
				}

				if (iValue > 0)
				{
					if (!(pLoopPlot->isVisibleEnemyUnit(this)))
					{
						if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_SPREAD, getGroup()) == 0)
						{
							if (generatePath(pLoopPlot, 0, true, &iPathTurns))
							{
								iValue *= pCity->getPopulation();

								if (pCity->getOwnerINLINE() == getOwnerINLINE())
								{
									iValue *= 4;
								}
								else if (pCity->getTeam() == getTeam())
								{
									iValue *= 3;
								}

								if (pCity->getReligionCount() == 0)
								{
									iValue *= 2;
								}

								iValue /= (pCity->getReligionCount() + 1);

								FAssert(iPathTurns > 0);

								if (iPathTurns == 1)
								{
									iValue *= 2;
								}

								iValue *= 1000;

								iValue /= (iPathTurns + 1);

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = getPathEndTurnPlot();
									pBestSpreadPlot = pLoopPlot;
									bExecutive = false;
								}
							}
						}
					}
				}

				if (iCorpValue > 0)
				{
					if (!(pLoopPlot->isVisibleEnemyUnit(this)))
					{
						if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_SPREAD_CORPORATION, getGroup()) == 0)
						{
							if (generatePath(pLoopPlot, 0, true, &iPathTurns))
							{
								iCorpValue *= pCity->getPopulation();

								FAssert(iPathTurns > 0);

								if (iPathTurns == 1)
								{
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       02/22/10                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original bts code
									iValue *= 2;
*/
									iCorpValue *= 2;
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
								}

								iCorpValue *= 1000;

								iCorpValue /= (iPathTurns + 1);

								if (iCorpValue > iBestValue)
								{
									iBestValue = iCorpValue;
									pBestPlot = getPathEndTurnPlot();
									pBestSpreadPlot = pLoopPlot;
									bExecutive = true;
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestSpreadPlot != NULL))
	{
		FAssert(!(pBestPlot->isImpassable()) || canMoveImpassable());

		if ((pBestPlot == pBestSpreadPlot) || (stepDistance(pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), pBestSpreadPlot->getX_INLINE(), pBestSpreadPlot->getY_INLINE()) == 1))
		{
			if (atPlot(pBestSpreadPlot))
			{
				unloadAll(); // XXX is this dangerous (not pushing a mission...) XXX air units?
				getGroup()->setActivityType(ACTIVITY_AWAKE); // K-Mod
				return true;
			}
			else
			{
				getGroup()->pushMission(MISSION_MOVE_TO, pBestSpreadPlot->getX_INLINE(), pBestSpreadPlot->getY_INLINE(), 0, false, false, bExecutive ? MISSIONAI_SPREAD_CORPORATION : MISSIONAI_SPREAD, pBestSpreadPlot);
				return true;
			}
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, bExecutive ? MISSIONAI_SPREAD_CORPORATION : MISSIONAI_SPREAD, pBestSpreadPlot);
			return true;
		}
	}

	return false;
}


// The body of this function has been completely deleted and rewritten for K-Mod
bool CvUnitAI::AI_specialSeaTransportSpy()
{
	const CvTeamAI& kOurTeam = GET_TEAM(getTeam());
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());

	std::vector<int> base_value(MAX_CIV_PLAYERS);

	int iTotalPoints = kOurTeam.getTotalUnspentEspionage();

	int iBestValue = 0;
	PlayerTypes eBestTarget = NO_PLAYER;

	for (int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		const CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)i);

		if (kLoopPlayer.getTeam() == getTeam() || !kOurTeam.isHasMet(kLoopPlayer.getTeam()))
		{
			base_value[i] = 0;
		}
		else
		{
			int iValue = 1000 * kOurTeam.getEspionagePointsAgainstTeam(kLoopPlayer.getTeam()) / std::max(1, iTotalPoints);

			if (kOwner.AI_isMaliciousEspionageTarget((PlayerTypes)i))
				iValue = 3*iValue/2;

			if (kOurTeam.isAtWar(kLoopPlayer.getTeam()) && !isInvisible(kLoopPlayer.getTeam(), false))
			{
				iValue /= 3; // it might be too risky.
			}

			if (kOurTeam.AI_hasCitiesInPrimaryArea(kLoopPlayer.getTeam()))
				iValue /= 6;

			iValue *= 100 - kOurTeam.AI_getAttitudeWeight(kLoopPlayer.getTeam())/2;

			base_value[i] = iValue; // of order 1000 * percentage of espionage. (~20000)

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				eBestTarget = (PlayerTypes)i;
			}
		}
	}

	if (eBestTarget == NO_PLAYER)
		return false;

	iBestValue = 0; // was best player value, now it is best plot value
	CvPlot* pTargetPlot = 0;
	CvPlot* pEndTurnPlot = 0;

	for (int i = 0; i < GC.getMapINLINE().numPlotsINLINE(); i++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(i);
		PlayerTypes ePlotOwner = pLoopPlot->getRevealedOwner(getTeam(), false);

		// only consider coast plots, owned by civ teams, with base value greater than the current best
		if (ePlotOwner == NO_PLAYER || ePlotOwner >= MAX_CIV_PLAYERS || !pLoopPlot->isCoastalLand() || iBestValue >= base_value[ePlotOwner] || pLoopPlot->area()->getCitiesPerPlayer(ePlotOwner) == 0)
			continue;

		//FAssert(pLoopPlot->isRevealed(getTeam(), false)); // otherwise, how do we have a revealed owner?
		// Actually, the owner gets revealed when any of the adjacent plots are visable - so this assert is not always true. I think it's fair to consider this plot anyway.

		int iValue = base_value[ePlotOwner];

		iValue *= 2;
		iValue /= 2 + kOwner.AI_totalAreaUnitAIs(pLoopPlot->area(), UNITAI_SPY);

		CvCity* pPlotCity = pLoopPlot->getPlotCity();
		if (pPlotCity && !kOurTeam.isAtWar(GET_PLAYER(ePlotOwner).getTeam())) // don't go directly to cities if we are at war.
		{
			iValue *= 100;
			iValue /= std::max(100, 3 * kOwner.getEspionageMissionCostModifier(NO_ESPIONAGEMISSION, ePlotOwner, pLoopPlot));
		}
		else
		{
			iValue /= 5;
		}

		if (GET_PLAYER(ePlotOwner).AI_isDoVictoryStrategyLevel4() && !GET_PLAYER(ePlotOwner).AI_isPrimaryArea(pLoopPlot->area()))
		{
			iValue /= 4;
		}

		FAssert(iValue <= base_value[ePlotOwner]);
		int iPathTurns;
		if (iValue > iBestValue && generatePath(pLoopPlot, 0, true, &iPathTurns))
		{
			iValue *= 10;
			iValue /= 3 + iPathTurns;
			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				pTargetPlot = pLoopPlot;
				pEndTurnPlot = getPathEndTurnPlot();
			}
		}
	}

	if (pTargetPlot)
	{
		if (atPlot(pTargetPlot))
		{
			getGroup()->unloadAll();
			getGroup()->setActivityType(ACTIVITY_AWAKE);
			return true; // no actual mission pushed, but we need to rethink our next move.
		}
		else
		{
			if (canMoveInto(pEndTurnPlot) || getGroup()->canCargoAllMove()) // (without this, we could get into an infinite loop when the cargo isn't ready to move)
			{
				if (gUnitLogLevel > 2 && pTargetPlot->getOwnerINLINE() != NO_PLAYER && generatePath(pTargetPlot, 0, true, 0, 1))
				{
					logBBAI("      %S lands sea-spy in %S territory. (%d percent of unspent points)", // apparently it's impossible to actually use a % sign in this Microsoft version of vsnprintf. madness
						kOurTeam.getName().GetCString(), GET_PLAYER(pTargetPlot->getOwnerINLINE()).getCivilizationDescription(0), kOurTeam.getEspionagePointsAgainstTeam(pTargetPlot->getTeam())*100/iTotalPoints);
				}

				getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX_INLINE(), pEndTurnPlot->getY_INLINE(), 0, false, false, MISSIONAI_ATTACK_SPY, pTargetPlot);
				return true;
			}
			else
			{
				// need to wait for our cargo to be ready
				getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY, pTargetPlot);
				return true;
			}
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_carrierSeaTransport()
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pLoopPlotAir;
	CvPlot* pBestPlot;
	CvPlot* pBestCarrierPlot;
	int iMaxAirRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;
	int iI;

	iMaxAirRange = 0;

	std::vector<CvUnit*> aCargoUnits;
	getCargoUnits(aCargoUnits);
	for (uint i = 0; i < aCargoUnits.size(); ++i)
	{
		iMaxAirRange = std::max(iMaxAirRange, aCargoUnits[i]->airRange());
	}

	if (iMaxAirRange == 0)
	{
		return false;
	}

	iBestValue = 0;
	pBestPlot = NULL;
	pBestCarrierPlot = NULL;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/22/10                                jdog5000      */
/*                                                                                              */
/* Naval AI, War tactics, Efficiency                                                            */
/************************************************************************************************/
	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot))
		{
			if (pLoopPlot->isAdjacentToLand())
			{
				if (!(pLoopPlot->isVisibleEnemyUnit(this)))
				{
					iValue = 0;

					for (iDX = -(iMaxAirRange); iDX <= iMaxAirRange; iDX++)
					{
						for (iDY = -(iMaxAirRange); iDY <= iMaxAirRange; iDY++)
						{
							pLoopPlotAir = plotXY(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), iDX, iDY);

							if (pLoopPlotAir != NULL)
							{
								if (plotDistance(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), pLoopPlotAir->getX_INLINE(), pLoopPlotAir->getY_INLINE()) <= iMaxAirRange)
								{
									if (!(pLoopPlotAir->isBarbarian()))
									{
										if (potentialWarAction(pLoopPlotAir))
										{
											if (pLoopPlotAir->isCity())
											{
												iValue += 3;

												// BBAI: Support invasions
												iValue += (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlotAir, MISSIONAI_ASSAULT, getGroup(), 2) * 6);
											}

											if (pLoopPlotAir->getImprovementType() != NO_IMPROVEMENT)
											{
												iValue += 2;
											}

											if (plotDistance(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), pLoopPlotAir->getX_INLINE(), pLoopPlotAir->getY_INLINE()) <= iMaxAirRange/2)
											{
												// BBAI: Support/air defense for land troops
												iValue += pLoopPlotAir->plotCount(PUF_canDefend, -1, -1, getOwnerINLINE());
												// advc.001s (comment): Also counts ships, but I guess that's OK.
											}
										}
									}
								}
							}
						}
					}

					if( iValue > 0 )
					{
						iValue *= 1000;

						for (int iDirection = 0; iDirection < NUM_DIRECTION_TYPES; iDirection++)
						{
							CvPlot* pDirectionPlot = plotDirection(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), (DirectionTypes)iDirection);
							if (pDirectionPlot != NULL)
							{
								if (pDirectionPlot->isCity() && isEnemy(pDirectionPlot->getTeam(), pLoopPlot))
								{
									iValue /= 2;
									break;
								}
							}
						}

						if (iValue > iBestValue)
						{
							bool bStealth = (getInvisibleType() != NO_INVISIBLE);
							if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_CARRIER, getGroup(), bStealth ? 5 : 3) <= (bStealth ? 0 : 3))
							{
								if (generatePath(pLoopPlot, 0, true, &iPathTurns))
								{
									iValue /= (iPathTurns + 1);

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
										pBestCarrierPlot = pLoopPlot;
									}
								}
							}
						}
					}
				}
			}
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if ((pBestPlot != NULL) && (pBestCarrierPlot != NULL))
	{
		if (atPlot(pBestCarrierPlot))
		{
			if (getGroup()->hasCargo())
			{
				CvPlot* pPlot = plot();

				int iNumUnits = pPlot->getNumUnits();

				for (int i = 0; i < iNumUnits; ++i)
				{
					bool bDone = true;
					CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();
					while (pUnitNode != NULL)
					{
						CvUnit* pCargoUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = pPlot->nextUnitNode(pUnitNode);
					
						if (pCargoUnit->isCargo())
						{
							FAssert(pCargoUnit->getTransportUnit() != NULL);
							if (pCargoUnit->getOwnerINLINE() == getOwnerINLINE() && (pCargoUnit->getTransportUnit()->getGroup() == getGroup()) && (pCargoUnit->getDomainType() == DOMAIN_AIR))
							{
								if (pCargoUnit->canMove() && pCargoUnit->isGroupHead())
								{
									// careful, this might kill the cargo group
									if (pCargoUnit->getGroup()->AI_update())
									{
										bDone = false;
										break;
									}
								}
							}
						}
					}

					if (bDone)
					{
						break;
					}
				}
			}

			if (canPlunder(pBestCarrierPlot))
			{
				getGroup()->pushMission(MISSION_PLUNDER, -1, -1, 0, false, false, MISSIONAI_CARRIER, pBestCarrierPlot);
			}
			else
			{
				getGroup()->pushMission(MISSION_SKIP);
			}
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_CARRIER, pBestCarrierPlot);
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_connectPlot(CvPlot* pPlot, int iRange)
{
	PROFILE_FUNC();

	FAssert(canBuildRoute());

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
	// BBAI efficiency: check area for land units before generating paths
	if( (getDomainType() == DOMAIN_LAND) && (pPlot->area() != area()) && !(getGroup()->canMoveAllTerrain()) )
	{
		return false;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	CvCity* pLoopCity;
	int iLoop;

	if (!pPlot->isVisibleEnemyUnit(this))
	{
		if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pPlot, MISSIONAI_BUILD, getGroup(), iRange) == 0)
		{
			if (generatePath(pPlot, MOVE_SAFE_TERRITORY
					| MOVE_ROUTE_TO, // advc.049
					true))
			{
				/* <advc.300> Barb behavior should just be to put roads on the
				   bonuses adjacent to their cities. */
				if(isBarbarian()) {
					CvCity* c = pPlot->getWorkingCity();
					if(c == NULL || !atPlot(pPlot) || pPlot->isConnectedTo(c))
						return false;
					getGroup()->pushMission(MISSION_ROUTE_TO,
							c->getX_INLINE(), c->getY_INLINE(),
							MOVE_SAFE_TERRITORY, false, false, MISSIONAI_BUILD,
							pPlot);
					return true;
				} // </advc.300>
				for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
				{
					if (!pPlot->isConnectedTo(pLoopCity))
					{
						FAssertMsg(pPlot->getPlotCity() != pLoopCity, "pPlot->getPlotCity() is not expected to be equal with pLoopCity");

						if (plot()->getPlotGroup(getOwnerINLINE()) == pLoopCity->plot()->getPlotGroup(getOwnerINLINE()))
						{
							getGroup()->pushMission(MISSION_ROUTE_TO,
									pPlot->getX_INLINE(),
									pPlot->getY_INLINE(),
									MOVE_SAFE_TERRITORY
									| MOVE_ROUTE_TO, // advc.049
									false, false, MISSIONAI_BUILD, pPlot);
							return true;
						}
					}
				}

				for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
				{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
					// BBAI efficiency: check same area
					if( (pLoopCity->area() != pPlot->area()) )
					{
						continue;
					}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

					if (!(pPlot->isConnectedTo(pLoopCity)))
					{
						FAssertMsg(pPlot->getPlotCity() != pLoopCity, "pPlot->getPlotCity() is not expected to be equal with pLoopCity");

						if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
						{
							if (generatePath(pLoopCity->plot(), MOVE_SAFE_TERRITORY
									| MOVE_ROUTE_TO, // advc.049
									true))
							{
								if (atPlot(pPlot)) // need to test before moving...
								{
									getGroup()->pushMission(MISSION_ROUTE_TO,
											pLoopCity->getX_INLINE(),
											pLoopCity->getY_INLINE(),
											MOVE_SAFE_TERRITORY
											| MOVE_ROUTE_TO, // advc.049
											false, false, MISSIONAI_BUILD, pPlot);
								}
								else
								{
									getGroup()->pushMission(MISSION_ROUTE_TO,
											pLoopCity->getX_INLINE(),
											pLoopCity->getY_INLINE(),
											MOVE_SAFE_TERRITORY
											| MOVE_ROUTE_TO, // advc.049
											false, false, MISSIONAI_BUILD, pPlot);
									//getGroup()->pushMission(MISSION_ROUTE_TO, pPlot->getX_INLINE(), pPlot->getY_INLINE(), MOVE_SAFE_TERRITORY, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BUILD, pPlot);
									getGroup()->pushMission(MISSION_ROUTE_TO,
											pPlot->getX_INLINE(),
											pPlot->getY_INLINE(),
											MOVE_SAFE_TERRITORY
											| MOVE_ROUTE_TO, // advc.049
											true, false, MISSIONAI_BUILD, pPlot); // K-Mod
								}

								return true;
							}
						}
					}
				}
			}
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_improveCity(CvCity* pCity)
{
	PROFILE_FUNC();

	CvPlot* pBestPlot;
	BuildTypes eBestBuild;
	MissionTypes eMission;

	if (AI_bestCityBuild(pCity, &pBestPlot, &eBestBuild, NULL, this))
	{
		FAssertMsg(pBestPlot != NULL, "BestPlot is not assigned a valid value");
		FAssertMsg(eBestBuild != NO_BUILD, "BestBuild is not assigned a valid value");
		FAssertMsg(eBestBuild < GC.getNumBuildInfos(), "BestBuild is assigned a corrupt value");
		if ((plot()->getWorkingCity() != pCity) || (GC.getBuildInfo(eBestBuild).getRoute() != NO_ROUTE))
		{
			eMission = MISSION_ROUTE_TO;
		}
		else
		{
			eMission = MISSION_MOVE_TO;
			if (pBestPlot && generatePath(pBestPlot) && getPathFinder().GetPathTurns() == 1 && getPathFinder().GetFinalMoves() == 0)
			{
				if (pBestPlot->getRouteType() != NO_ROUTE)
				{
					eMission = MISSION_ROUTE_TO;
				}
			}
			else if (plot()->getRouteType() == NO_ROUTE)
			{
				int iPlotMoveCost = 0;
				iPlotMoveCost = ((plot()->getFeatureType() == NO_FEATURE) ? GC.getTerrainInfo(plot()->getTerrainType()).getMovementCost() : GC.getFeatureInfo(plot()->getFeatureType()).getMovementCost());

				if (plot()->isHills())
				{
					iPlotMoveCost += GC.getHILLS_EXTRA_MOVEMENT();
				}
				if (iPlotMoveCost > 1)
				{
					eMission = MISSION_ROUTE_TO;
				}
			}
		}
		
		eBestBuild = AI_betterPlotBuild(pBestPlot, eBestBuild);

		getGroup()->pushMission(eMission, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pBestPlot);
		//getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BUILD, pBestPlot);
		getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, true, false, MISSIONAI_BUILD, pBestPlot); // K-Mod

		return true;
	}

	return false;
}

bool CvUnitAI::AI_improveLocalPlot(int iRange, CvCity* pIgnoreCity,
		int iNeededWorkersInArea) // advc.117
{
	PROFILE_FUNC();

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	BuildTypes eBestBuild = NO_BUILD;
	bool bChop = false; // advc.117

	for (int iX = -iRange; iX <= iRange; iX++)
	{
		for (int iY = -iRange; iY <= iRange; iY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iX, iY);
			// <advc.117> Chop plots outside of city radii
			int iValue = 0;
			CvPlayerAI& kOwner = GET_PLAYER(getOwner());
			if(pLoopPlot != NULL && !pLoopPlot->isCityRadius() &&
					pLoopPlot->getWorkingCity() == NULL &&
					!isBarbarian() &&
					AI_plotValid(pLoopPlot) &&
					iNeededWorkersInArea <= 0 &&
					!kOwner.isOption(PLAYEROPTION_LEAVE_FORESTS) &&
					kOwner.getGwPercentAnger() <= 0 &&
					pLoopPlot->getImprovementType() == NO_IMPROVEMENT &&
					pLoopPlot->getFeatureType() != NO_FEATURE &&
					// Don't chop near planned cities
					!kOwner.AI_isAdjacentCitySite(*pLoopPlot, false)) {
				for(int i = 0; i < GC.getNumBuildInfos(); i++) {
					BuildTypes eBuild = (BuildTypes)i;
					if(eBuild == NO_BUILD)
						continue;
					CvBuildInfo& kBuild = GC.getBuildInfo(eBuild);
					if(kBuild.getImprovement() != NO_IMPROVEMENT)
						continue;
					if(!canBuild(pLoopPlot, eBuild))
						continue;
					iValue = kBuild.getFeatureProduction(pLoopPlot->getFeatureType());
					if(iValue <= iBestValue)
						continue;
					int iPathTurns;
					if(generatePath(pLoopPlot, 0, true, &iPathTurns, 5))
					{	/*  Doesn't need to be on the same scale as non-chop values;
							only want to chop when there is nothing else to do. */
						iBestValue = (iValue * 100) / (1 + iPathTurns);
						eBestBuild = eBuild;
						pBestPlot = pLoopPlot;
						bChop = true;
						break;
					}
				}
				if(bChop)
					continue;
			} // </advc.117>
			// K-Mod note: I've turn the all-encompassing if blocks into !if continues.
			if (pLoopPlot == NULL || !pLoopPlot->isCityRadius())
				continue;

			CvCity* pCity = pLoopPlot->getWorkingCity(); // advc.117
			if (pCity == NULL || pCity->getOwnerINLINE() != getOwnerINLINE())
				continue;

			if (pIgnoreCity != NULL && pCity == pIgnoreCity)
				continue;

			if (!AI_plotValid(pLoopPlot))
				continue;

			int iIndex = pCity->getCityPlotIndex(pLoopPlot); // advc.117
			if (iIndex == CITY_HOME_PLOT || pCity->AI_getBestBuild(iIndex) == NO_BUILD)
				continue;

			if (pIgnoreCity != NULL && pCity->AI_getWorkersHave()-(plot()->getWorkingCity() == pCity ? 1 : 0) >= (1 + pCity->AI_getWorkersNeeded() * 2) / 3)
				continue;
			// K-Mod note. This was the original condition for the rest of the block:
			//if (((NULL == pIgnoreCity) || ((pCity->AI_getWorkersNeeded() > 0) && (pCity->AI_getWorkersHave() < (1 + pCity->AI_getWorkersNeeded() * 2 / 3)))) && (pCity->AI_getBestBuild(iIndex) != NO_BUILD))

			if (!canBuild(pLoopPlot, pCity->AI_getBestBuild(iIndex)))
				continue;

			if (GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_SAFE_AUTOMATION))
			{
				if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT && pLoopPlot->getImprovementType() != GC.getDefineINT("RUINS_IMPROVEMENT"))
					continue;
			}
			/* original bts code
			if (bAllowed) {
				if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT && GC.getBuildInfo(pCity->AI_getBestBuild(iIndex)).getImprovement() != NO_IMPROVEMENT)
					bAllowed = false;
			} */ /* K-Mod. I don't think it's a good idea to disallow improvement changes here.
					So I'm changing it to have a cutoff value instead. */
			if (pCity->AI_getBestBuildValue(iIndex) <= 1)
				continue;
			// advc.117: iValue now declared earlier
			iValue = pCity->AI_getBestBuildValue(iIndex);
		
			int iPathTurns;
			if (generatePath(pLoopPlot, 0, true, &iPathTurns))
			{
				int iMaxWorkers = 1;
				if (plot() == pLoopPlot)
				{
					iValue *= 3;
					iValue /= 2;
				}
				else if (getPathFinder().GetFinalMoves() == 0)
				{
					iPathTurns++;
				}
				else if (iPathTurns <= 1)
				{
					iMaxWorkers = AI_calculatePlotWorkersNeeded(pLoopPlot, pCity->AI_getBestBuild(iIndex));
				}

				if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BUILD, getGroup()) < iMaxWorkers)
				{
					iValue *= 1000;
					iValue /= 1 + iPathTurns;
					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestPlot = pLoopPlot;
						eBestBuild = pCity->AI_getBestBuild(iIndex);
						bChop = false; // advc.117
					}
				}
			}
		}
	}
	
	if (pBestPlot != NULL)
	{
	    FAssertMsg(eBestBuild != NO_BUILD, "BestBuild is not assigned a valid value");
	    FAssertMsg(eBestBuild < GC.getNumBuildInfos(), "BestBuild is assigned a corrupt value");

		// advc.117: No longer guaranteed
		//FAssert(pBestPlot->getWorkingCity() != NULL);
		if (NULL != pBestPlot->getWorkingCity())
		{
			pBestPlot->getWorkingCity()->AI_changeWorkersHave(+1);

			if (plot()->getWorkingCity() != NULL)
			{
				plot()->getWorkingCity()->AI_changeWorkersHave(-1);
			}
		}
		MissionTypes eMission = MISSION_MOVE_TO;

		int iPathTurns;
		if (generatePath(pBestPlot, 0, true, &iPathTurns) && getPathFinder().GetPathTurns() == 1 && getPathFinder().GetFinalMoves() == 0)
		{
			if (pBestPlot->getRouteType() != NO_ROUTE)
			{
				eMission = MISSION_ROUTE_TO;
			}				
		}
		else if (plot()->getRouteType() == NO_ROUTE)
		{
			int iPlotMoveCost = 0;
			iPlotMoveCost = ((plot()->getFeatureType() == NO_FEATURE) ? GC.getTerrainInfo(plot()->getTerrainType()).getMovementCost() : GC.getFeatureInfo(plot()->getFeatureType()).getMovementCost());

			if (plot()->isHills())
			{
				iPlotMoveCost += GC.getHILLS_EXTRA_MOVEMENT();
			}
			if (iPlotMoveCost > 1)
			{
				eMission = MISSION_ROUTE_TO;
			}
		}
		if(!bChop) /* advc.117: betterPlotBuild will only suggest Farms or Forts 
				     or who knows what -- stick to the chopping plan */
			eBestBuild = AI_betterPlotBuild(pBestPlot, eBestBuild);

		getGroup()->pushMission(eMission, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pBestPlot);
		//getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BUILD, pBestPlot);
		getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, true, false, MISSIONAI_BUILD, pBestPlot); // K-Mod
		return true;
	}
	
	return false;
}

// Returns true if a mission was pushed...
bool CvUnitAI::AI_nextCityToImprove(CvCity* pCity)
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvPlot* pPlot;
	CvPlot* pBestPlot;
	BuildTypes eBuild;
	BuildTypes eBestBuild;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iLoop;

	iBestValue = 0;
	eBestBuild = NO_BUILD;
	pBestPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (pLoopCity != pCity)
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/22/10                                jdog5000      */
/*                                                                                              */
/* Worker AI, Efficiency                                                                        */
/************************************************************************************************/
			// BBAI efficiency: check area for land units before path generation
			if( (getDomainType() == DOMAIN_LAND) && (pLoopCity->area() != area()) && !(getGroup()->canMoveAllTerrain()) )
			{
				continue;
			}

			//iValue = pLoopCity->AI_totalBestBuildValue(area());
			int iWorkersNeeded = pLoopCity->AI_getWorkersNeeded();
			int iWorkersHave = pLoopCity->AI_getWorkersHave();
			
			iValue = std::max(0, iWorkersNeeded - iWorkersHave) * 100;
			iValue += iWorkersNeeded * 10;
			iValue *= (iWorkersNeeded + 1);
			iValue /= (iWorkersHave + 1);

			if (iValue > 0)
			{
				if (AI_bestCityBuild(pLoopCity, &pPlot, &eBuild, NULL, this))
				{
					FAssert(pPlot != NULL);
					FAssert(eBuild != NO_BUILD);

					if( AI_plotValid(pPlot) )
					{
						iValue *= 1000;

						if (pLoopCity->isCapital())
						{
							iValue *= 2;
						}

						if( iValue > iBestValue )
						{
							if( generatePath(pPlot, 0, true, &iPathTurns) )
							{
								iValue /= (iPathTurns + 1);

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									eBestBuild = eBuild;
									pBestPlot = pPlot;
									//FAssert(!atPlot(pBestPlot) || NULL == pCity || pCity->AI_getWorkersNeeded() == 0 || pCity->AI_getWorkersHave() > pCity->AI_getWorkersNeeded() + 1);
								}
							}
						}
					}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
	    FAssertMsg(eBestBuild != NO_BUILD, "BestBuild is not assigned a valid value");
	    FAssertMsg(eBestBuild < GC.getNumBuildInfos(), "BestBuild is assigned a corrupt value");
	    if (plot()->getWorkingCity() != NULL)
	    {
			plot()->getWorkingCity()->AI_changeWorkersHave(-1);
	    }

		FAssert(pBestPlot->getWorkingCity() != NULL || GC.getBuildInfo(eBestBuild).getImprovement() == NO_IMPROVEMENT);
		if (NULL != pBestPlot->getWorkingCity())
		{
			pBestPlot->getWorkingCity()->AI_changeWorkersHave(+1);
		}
		
		eBestBuild = AI_betterPlotBuild(pBestPlot, eBestBuild);

		getGroup()->pushMission(MISSION_ROUTE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pBestPlot);
		//getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BUILD, pBestPlot);
		getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, true, false, MISSIONAI_BUILD, pBestPlot); // K-Mod
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_nextCityToImproveAirlift()
{
	PROFILE_FUNC();

	CvCity* pCity;
	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	int iValue;
	int iBestValue;
	int iLoop;

	if (getGroup()->getNumUnits() > 1)
	{
		return false;
	}

	pCity = plot()->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (pCity->getMaxAirlift() == 0)
	{
		return false;
	}

	iBestValue = 0;
	pBestPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (pLoopCity != pCity)
		{
			if (canAirliftAt(pCity->plot(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE()))
			{
				iValue = pLoopCity->AI_totalBestBuildValue(pLoopCity->area());

				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					pBestPlot = pLoopCity->plot();
					FAssert(pLoopCity != pCity);
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_AIRLIFT, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_irrigateTerritory()
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	int iPathTurns;
	int iI, iJ;

	int iBestValue = 0;
	BuildTypes eBestBuild = NO_BUILD;
	CvPlot* pBestPlot = NULL;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
		// advc.003: Some changes to reduce indentation
		if (!AI_plotValid(pLoopPlot) ||
				pLoopPlot->getOwnerINLINE() != getOwnerINLINE() || // XXX team???
				pLoopPlot->getWorkingCity() != NULL)
			continue;
		ImprovementTypes eImprovement = pLoopPlot->getImprovementType();
		if(eImprovement != NO_IMPROVEMENT &&
				GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_SAFE_AUTOMATION) &&
				eImprovement != (GC.getDefineINT("RUINS_IMPROVEMENT")))
			continue;
		if(eImprovement != NO_IMPROVEMENT &&
				GC.getImprovementInfo(eImprovement).isCarriesIrrigation())
			continue;
		BonusTypes eNonObsoleteBonus = pLoopPlot->getNonObsoleteBonusType(getTeam());
		//if ((eImprovement == NO_IMPROVEMENT) || (eNonObsoleteBonus == NO_BONUS) || !(GC.getImprovementInfo(eImprovement).isImprovementBonusTrade(eNonObsoleteBonus)))
		if (eImprovement == NO_IMPROVEMENT || eNonObsoleteBonus == NO_BONUS ||
				!GET_PLAYER(getOwnerINLINE()).doesImprovementConnectBonus(
				eImprovement, eNonObsoleteBonus))
		{
			if (pLoopPlot->isIrrigationAvailable(true))
			{
				int iBestTempBuildValue = MAX_INT;
				BuildTypes eBestTempBuild = NO_BUILD;
				for (iJ = 0; iJ < GC.getNumBuildInfos(); iJ++)
				{
					BuildTypes eBuild = ((BuildTypes)iJ);
					FAssertMsg(eBuild < GC.getNumBuildInfos(), "Invalid Build");
					if (GC.getBuildInfo(eBuild).getImprovement() != NO_IMPROVEMENT)
					{ /* advc.121: Same problems as in AI_improveBonus (see there).
						 Hopefully fixed now, though no adverse consequences either way
						 so long as there is only one improvement that carries irrigation. */
						ImprovementTypes impId = (ImprovementTypes)(GC.getBuildInfo(eBuild).getImprovement());
						if (GC.getImprovementInfo(impId).isCarriesIrrigation())
						{
							if (canBuild(pLoopPlot, eBuild)
									|| impId == pLoopPlot->getImprovementType()) // advc.121
							{
								/* int iValue = 10000;
								iValue /= (GC.getBuildInfo(eBuild).getTime() + 1);*/
								// <advc.121> Replacing the above
								int iValue = ((impId == pLoopPlot->getImprovementType()) ?
										0 : GC.getBuildInfo(eBuild).getTime());
								// </advc.121>
								// XXX feature production???
								if (iValue < iBestTempBuildValue)
								{
									iBestTempBuildValue = iValue;
									eBestTempBuild = eBuild;
								}
							}
						}
					}
				} // <advc.121>
				if(eBestTempBuild != NO_BUILD &&
						!canBuild(pLoopPlot, eBestTempBuild))
					eBestTempBuild = NO_BUILD; // </advc.121>
				if(eBestTempBuild != NO_BUILD)
				{
					bool bValid = true;
					// K-Mod. (I didn't want it to be this way...)
					/* original bts code
					if (GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_LEAVE_FORESTS))
					{
						if (pLoopPlot->getFeatureType() != NO_FEATURE)
						{
							if (GC.getBuildInfo(eBestTempBuild).isFeatureRemove(pLoopPlot->getFeatureType()))
							{
								if (GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_PRODUCTION) > 0)
								{
									bValid = false;
								}
							}
						}
					} */
					if (pLoopPlot->getFeatureType() != NO_FEATURE)
					{
						if (GC.getBuildInfo(eBestTempBuild).isFeatureRemove(pLoopPlot->getFeatureType()))
						{
							const CvFeatureInfo& kFeatureInfo = GC.getFeatureInfo(pLoopPlot->getFeatureType());
							if ((GC.getGame().getGwEventTally() >= 0 && kFeatureInfo.getWarmingDefense() > 0) ||
								(GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_LEAVE_FORESTS) && kFeatureInfo.getYieldChange(YIELD_PRODUCTION) > 0))
							{
								bValid = false;
							}
						}
					}
					// K-Mod end

					if (bValid)
					{
						if (!pLoopPlot->isVisibleEnemyUnit(this))
						{
							if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BUILD, getGroup(), 1) == 0)
							{
								if (generatePath(pLoopPlot, 0, true, &iPathTurns)) // XXX should this actually be at the top of the loop? (with saved paths and all...)
								{
									int iValue = 10000;

									iValue /= (iPathTurns + 1);

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										eBestBuild = eBestTempBuild;
										pBestPlot = pLoopPlot;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssertMsg(eBestBuild != NO_BUILD, "BestBuild is not assigned a valid value");
		FAssertMsg(eBestBuild < GC.getNumBuildInfos(), "BestBuild is assigned a corrupt value");

		getGroup()->pushMission(MISSION_ROUTE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pBestPlot);
		//getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BUILD, pBestPlot);
		getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, true, false, MISSIONAI_BUILD, pBestPlot); // K-Mod

		return true;
	}

	return false;
}

bool CvUnitAI::AI_fortTerritory(bool bCanal, bool bAirbase)
{
	PROFILE_FUNC();

	// K-Mod. This function currently only handles canals and airbases. So if we want neither, just abort.
	if (!bCanal && !bAirbase)
		return false;
	// K-Mod end

	int iBestValue = 0;
	BuildTypes eBestBuild = NO_BUILD;
	CvPlot* pBestPlot = NULL;

	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
		// advc.003: Some refactoring to reduce indentation
		if(!AI_plotValid(pLoopPlot) ||
				pLoopPlot->getOwnerINLINE() == getOwnerINLINE()) // XXX team???
			continue;
		if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT
				|| pLoopPlot->isContestedByRival()) // advc.035
			continue;
		int iValue = 0;
		iValue += (bCanal ? kOwner.AI_getPlotCanalValue(pLoopPlot) : 0);
		iValue += (bAirbase ? kOwner.AI_getPlotAirbaseValue(pLoopPlot) : 0);
		if(iValue <= 0)
			continue;
		int iBestTempBuildValue = MAX_INT;
		BuildTypes eBestTempBuild = NO_BUILD;

		/*  K-Mod note: the following code may choose the improvement poorly if there are
			multiple fort options to choose from. I don't want to spend time fixing it now,
			because K-Mod only has one type of fort anyway. */
		for (int iJ = 0; iJ < GC.getNumBuildInfos(); iJ++)
		{
			BuildTypes eBuild = ((BuildTypes)iJ);
			FAssertMsg(eBuild < GC.getNumBuildInfos(), "Invalid Build");

			if (GC.getBuildInfo(eBuild).getImprovement() != NO_IMPROVEMENT)
			{ /* advc.121: Same problems as in AI_improveBonus, but there's
				 only one type of Fort anyway (see also the K-Mod comment above). */
				ImprovementTypes impId = (ImprovementTypes)(GC.getBuildInfo(eBuild).getImprovement());
				if(GC.getImprovementInfo(impId).isActsAsCity() && 
						GC.getImprovementInfo(impId).getDefenseModifier() > 0)
				{
					if (canBuild(pLoopPlot, eBuild)
							// advc.121:
							|| impId == pLoopPlot->getImprovementType())
					{
						/* int iValue = 10000;
						iValue /= (GC.getBuildInfo(eBuild).getTime() + 1);*/
						// <advc.121> Replacing the above
						int iTempBuildValue = (impId == pLoopPlot->getImprovementType() ?
								0 : GC.getBuildInfo(eBuild).getTime());
						// </advc.121>
						if (iTempBuildValue < iBestTempBuildValue)
						{
							iBestTempBuildValue = iTempBuildValue;
							eBestTempBuild = eBuild;
						}
					}
				}
			}
		}
		// <advc.121>
		if(eBestTempBuild != NO_BUILD &&
				!canBuild(pLoopPlot, eBestTempBuild))
			eBestTempBuild = NO_BUILD; // </advc.121>
		if(eBestTempBuild != NO_BUILD)
		{
			if(!pLoopPlot->isVisibleEnemyUnit(this))
			{
				bool bValid = true;
				if (GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_LEAVE_FORESTS) &&
						pLoopPlot->getFeatureType() != NO_FEATURE &&
						GC.getBuildInfo(eBestTempBuild).isFeatureRemove(pLoopPlot->getFeatureType()) &&
						GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_PRODUCTION) > 0)
					bValid = false;
				if (bValid)
				{
					if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BUILD, getGroup(), 3) == 0)
					{
						int iPathTurns;
						if (generatePath(pLoopPlot, 0, true, &iPathTurns))
						{
							iValue *= 1000;
							iValue /= (iPathTurns + 1);

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								eBestBuild = eBestTempBuild;
								pBestPlot = pLoopPlot;
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssertMsg(eBestBuild != NO_BUILD, "BestBuild is not assigned a valid value");
		FAssertMsg(eBestBuild < GC.getNumBuildInfos(), "BestBuild is assigned a corrupt value");

		getGroup()->pushMission(MISSION_ROUTE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pBestPlot);
		//getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BUILD, pBestPlot);
		getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, true, false, MISSIONAI_BUILD, pBestPlot); // K-Mod

		return true;
	}
	return false;
}

// Returns true if a mission was pushed...
//bool CvUnitAI::AI_improveBonus(int iMinValue, CvPlot** ppBestPlot, BuildTypes* peBestBuild, int* piBestValue)
bool CvUnitAI::AI_improveBonus( // K-Mod. (all that junk wasn't being used anyway.)
		int iNeededWorkersInArea) // advc.121
{
	PROFILE_FUNC();

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	bool bBestBuildIsRoute = false;
	int iBestValue = 0;
	//int iBestResourceValue = 0; // advc.003: no longer used
	BuildTypes eBestBuild = NO_BUILD;
	CvPlot* pBestPlot = NULL;
	bool bCanRoute = canBuildRoute();
	for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
		if(pLoopPlot->getOwnerINLINE() != getOwnerINLINE() ||
				!AI_plotValid(pLoopPlot))
			continue;
		// <advc.300> Barbarian workers mustn't improve bonuses around far-away cities
		if(isBarbarian() && (pLoopPlot->getWorkingCity() == NULL ||
				pLoopPlot->getWorkingCity() != plot()->getWorkingCity()))
			continue; // </advc.300>

		bool bCanImprove = (pLoopPlot->area() == area());
		if (!bCanImprove)
		{
			if (DOMAIN_SEA == getDomainType() && pLoopPlot->isWater() &&
					plot()->isAdjacentToArea(pLoopPlot->area()))
				bCanImprove = true;
		}
		if(!bCanImprove)
			continue;

		BonusTypes eNonObsoleteBonus = pLoopPlot->getNonObsoleteBonusType(getTeam());
		if(eNonObsoleteBonus == NO_BONUS)
			continue;

		bool bConnected = pLoopPlot->isConnectedToCapital(getOwnerINLINE());
		if(pLoopPlot->getWorkingCity() == NULL && !bConnected && !bCanRoute)
			continue;

		/* original bts code
		ImprovementTypes eImprovement = pLoopPlot->getImprovementType();
		bool bDoImprove = false;
		if (eImprovement == NO_IMPROVEMENT)
			bDoImprove = true;
		else if (GC.getImprovementInfo(eImprovement).isActsAsCity() || GC.getImprovementInfo(eImprovement).isImprovementBonusTrade(eNonObsoleteBonus))
			bDoImprove = false;
		else if (eImprovement == (ImprovementTypes)(GC.getDefineINT("RUINS_IMPROVEMENT")))
			bDoImprove = true;
		else if (!GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_SAFE_AUTOMATION))
			bDoImprove = true;
		int iBestTempBuildValue = MAX_INT;
		BuildTypes eBestTempBuild = NO_BUILD; */
		// K-Mod. Simpler, and better.
		bool bDoImprove = true;
		ImprovementTypes eImprovement = pLoopPlot->getImprovementType();
		CvCity* pWorkingCity = pLoopPlot->getWorkingCity();
		BuildTypes eBestTempBuild = NO_BUILD;

		if (eImprovement != NO_IMPROVEMENT &&
				((kOwner.isOption(PLAYEROPTION_SAFE_AUTOMATION) &&
				eImprovement != GC.getDefineINT("RUINS_IMPROVEMENT")) ||
				(kOwner.doesImprovementConnectBonus(eImprovement, eNonObsoleteBonus)
			/*  advc.121: This should give replacement of Fort with another
				connecting improvement a higher priority when Workers have time. */
				&& pWorkingCity == NULL && iNeededWorkersInArea > 0)))
			bDoImprove = false;
		else if (pWorkingCity)
		{
			// Let "best build" handle improvement replacements near cities.
			BuildTypes eBuild = pWorkingCity->AI_getBestBuild(plotCityXY(
					pWorkingCity, pLoopPlot));
			if (eBuild != NO_BUILD && kOwner.doesImprovementConnectBonus(
					(ImprovementTypes)GC.getBuildInfo(eBuild).getImprovement(),
					eNonObsoleteBonus) && canBuild(pLoopPlot, eBuild))
			{
				bDoImprove = true;
				eBestTempBuild = eBuild;
			}
			else bDoImprove = false;
		} // K-Mod end
		//if (bDoImprove)
		if (bDoImprove && eBestTempBuild == NO_BUILD) // K-Mod
		{
			int iBestTempBuildValue = MAX_INT; // K-Mod
			for (int iJ = 0; iJ < GC.getNumBuildInfos(); iJ++)
			{
				BuildTypes eLoopBuild = (BuildTypes)iJ;
				// <advc.121> Moved into subroutines
				if(!AI_canConnectBonus(*pLoopPlot, eLoopBuild))
					continue;
				int iValue = AI_connectBonusCost(*pLoopPlot, eLoopBuild,
						iNeededWorkersInArea);
				// </advc.121>
				if (iValue < iBestTempBuildValue)
				{
					iBestTempBuildValue = iValue;
					eBestTempBuild = eLoopBuild;
				}
			}
		}
		/*  <advc.121> canBuild no longer guaranteed b/c eBestTempBuild could
			already be present. */
		if(eBestTempBuild != NO_BUILD && !canBuild(pLoopPlot, eBestTempBuild))
			eBestTempBuild = NO_BUILD; // </advc.121>
		if(eBestTempBuild == NO_BUILD)
			bDoImprove = false;

		if(eBestTempBuild == NO_BUILD && (!bCanRoute || bConnected))
			continue;

		int iPathTurns;
		if(!generatePath(pLoopPlot, 0, true, &iPathTurns))
			continue;

		int iValue = kOwner.AI_bonusVal(eNonObsoleteBonus, 1);
		if (bDoImprove)
		{
			eImprovement = (ImprovementTypes)GC.getBuildInfo(eBestTempBuild).getImprovement();
			FAssert(eImprovement != NO_IMPROVEMENT);
			//iValue += (GC.getImprovementInfo((ImprovementTypes) GC.getBuildInfo(eBestTempBuild).getImprovement()))
			iValue += 5 * pLoopPlot->calculateImprovementYieldChange(eImprovement, YIELD_FOOD, getOwnerINLINE(), false);
			iValue += 5 * pLoopPlot->calculateNatureYield(YIELD_FOOD, getTeam(), (pLoopPlot->getFeatureType() == NO_FEATURE) ? true : GC.getBuildInfo(eBestTempBuild).isFeatureRemove(pLoopPlot->getFeatureType()));
		}
		iValue += std::max(0, 100 * GC.getBonusInfo(eNonObsoleteBonus).getAIObjective());

		if(kOwner.getNumTradeableBonuses(eNonObsoleteBonus) == 0)
			iValue *= 2;

		int iMaxWorkers = 1;
		if (eBestTempBuild != NO_BUILD && !GC.getBuildInfo(eBestTempBuild).isKill())
		{ //allow teaming.
			iMaxWorkers = AI_calculatePlotWorkersNeeded(pLoopPlot, eBestTempBuild);
			if (getPathFinder().GetFinalMoves() == 0)
			{
				iMaxWorkers = std::min((iMaxWorkers + 1) / 2,
						1 + kOwner.AI_baseBonusVal(eNonObsoleteBonus) / 20);
			}
		}
		if (kOwner.AI_plotTargetMissionAIs(
				pLoopPlot, MISSIONAI_BUILD, getGroup()) < iMaxWorkers &&
				(!bDoImprove || pLoopPlot->getBuildTurnsLeft(eBestTempBuild,
				/* advc.251: */ kOwner.getID(), 0, 0) > iPathTurns * 2 - 1))
		{
			if (bDoImprove)
			{
				iValue *= 1000;
				if(iPathTurns == 1 && getPathFinder().GetFinalMoves() != 0)
					iValue *= 2;
				iValue /= (iPathTurns + 1);
				if(pLoopPlot->isCityRadius())
					iValue *= 2;
				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					eBestBuild = eBestTempBuild;
					pBestPlot = pLoopPlot;
					bBestBuildIsRoute = false;
					//iBestResourceValue = iValue; // advc.003: no longer used
				}
			}
			else
			{
				FAssert(bCanRoute && !bConnected);
				eImprovement = pLoopPlot->getImprovementType();
				//if ((eImprovement != NO_IMPROVEMENT) && (GC.getImprovementInfo(eImprovement).isImprovementBonusTrade(eNonObsoleteBonus)))
				if (kOwner.doesImprovementConnectBonus(eImprovement, eNonObsoleteBonus))
				{
					iValue *= 1000;
					iValue /= (iPathTurns + 1);
					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						eBestBuild = NO_BUILD;
						pBestPlot = pLoopPlot;
						bBestBuildIsRoute = true;
					}
				}
			}
		}
	}
	/* original bts code
	if ((iBestValue < iMinValue) && (NULL != ppBestPlot))
	{
		FAssert(NULL != peBestBuild);
		FAssert(NULL != piBestValue);

		*ppBestPlot = pBestPlot;
		*peBestBuild = eBestBuild;
		*piBestValue = iBestResourceValue;
	} */ // disabled by K-Mod. This is clearly wrong. But even if it was fixed it isn't useful anyway, so I'm removing it.

	if (pBestPlot == NULL)
		return false;

	if (eBestBuild != NO_BUILD)
	{
		FAssertMsg(!bBestBuildIsRoute, "BestBuild should not be a route");
		FAssertMsg(eBestBuild < GC.getNumBuildInfos(), "BestBuild is assigned a corrupt value");
		MissionTypes eBestMission = MISSION_MOVE_TO;
		// <advc.001y> Sea workers can't route
		if(getGroup()->canDoMission(MISSION_ROUTE_TO, 
				getX_INLINE(), getY_INLINE(), plot(), false, false)) { // </advc.001y>
			if (pBestPlot->getWorkingCity() == NULL ||
					!pBestPlot->getWorkingCity()->isConnectedToCapital())
				eBestMission = MISSION_ROUTE_TO;
			else
			{
				int iDistance = stepDistance(getX_INLINE(), getY_INLINE(),
						pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
				int iPathTurns;
				if (generatePath(pBestPlot, 0, false, &iPathTurns))
				{
					if (iPathTurns >= iDistance)
						eBestMission = MISSION_ROUTE_TO;
				}
			}
		}
		eBestBuild = AI_betterPlotBuild(pBestPlot, eBestBuild);
		getGroup()->pushMission(eBestMission, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(),
				0, false, false, MISSIONAI_BUILD, pBestPlot);
		getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0,
				//(getGroup()->getLengthMissionQueue() > 0),
				true, // K-Mod
				false, MISSIONAI_BUILD, pBestPlot); 
		return true;
	}
	else if (bBestBuildIsRoute)
	{
		if (AI_connectPlot(pBestPlot))
		{
			return true;
		}
		/*else {
			// the plot may be connected, but not connected to capital, if capital is not on same area, or if civ has no capital (like barbarians)
			FAssertMsg(false, "Expected that a route could be built to eBestPlot");
		}*/
	}
	else FAssert(false);
	return false;
}

//returns true if a mission is pushed
//if eBuild is NO_BUILD, assumes a route is desired.
// advc.003j (comment): Not currently used (b/c of K-Mod changes in this file)
bool CvUnitAI::AI_improvePlot(CvPlot* pPlot, BuildTypes eBuild)
{
	FAssert(pPlot != NULL);
	
	if (eBuild != NO_BUILD)
	{
		FAssertMsg(eBuild < GC.getNumBuildInfos(), "BestBuild is assigned a corrupt value");
		
		eBuild = AI_betterPlotBuild(pPlot, eBuild);
		if (!atPlot(pPlot))
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pPlot->getX_INLINE(), pPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pPlot);
		}
		//getGroup()->pushMission(MISSION_BUILD, eBuild, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BUILD, pPlot);
		getGroup()->pushMission(MISSION_BUILD, eBuild, -1, 0, true, false, MISSIONAI_BUILD, pPlot); // K-Mod

		return true;
	}
	else if (canBuildRoute())
	{
		if (AI_connectPlot(pPlot))
		{
			return true;
		}
	}
	
	return false;
	
}

BuildTypes CvUnitAI::AI_betterPlotBuild(CvPlot* pPlot, BuildTypes eBuild)
{
	FAssert(pPlot != NULL);
	FAssert(eBuild != NO_BUILD);
	bool bBuildRoute = false;
	bool bClearFeature = false;
	
	FeatureTypes eFeature = pPlot->getFeatureType();
	
	CvBuildInfo& kOriginalBuildInfo = GC.getBuildInfo(eBuild);
	
	if (kOriginalBuildInfo.getRoute() != NO_ROUTE)
	{
		return eBuild;
	}
	
	int iWorkersNeeded = AI_calculatePlotWorkersNeeded(pPlot, eBuild);
	
	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD						7/31/08				jdog5000	*/
	/* 																			*/
	/* 	Bugfix																	*/
	/********************************************************************************/
	//if ((pPlot->getBonusType() == NO_BONUS) && (pPlot->getWorkingCity() != NULL))
	if ((pPlot->getNonObsoleteBonusType(getTeam()) == NO_BONUS) && (pPlot->getWorkingCity() != NULL))
	{
		iWorkersNeeded = std::max(1, std::min(iWorkersNeeded, pPlot->getWorkingCity()->AI_getWorkersHave()));
	}
	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD						END								*/
	/********************************************************************************/

	if (eFeature != NO_FEATURE)
	{
		CvFeatureInfo& kFeatureInfo = GC.getFeatureInfo(eFeature);
		if (kOriginalBuildInfo.isFeatureRemove(eFeature))
		{
			if ((kOriginalBuildInfo.getImprovement() == NO_IMPROVEMENT) || (!pPlot->isBeingWorked() || (kFeatureInfo.getYieldChange(YIELD_FOOD) + kFeatureInfo.getYieldChange(YIELD_PRODUCTION)) <= 0))
			{
				bClearFeature = true;
			}
		}
		
		if ((kFeatureInfo.getMovementCost() > 1) && (iWorkersNeeded > 1))
		{
			bBuildRoute = true;
		}
	}
	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD						7/31/08				jdog5000	*/
	/* 																			*/
	/* 	Bugfix																	*/
	/********************************************************************************/
	//if (pPlot->getBonusType() != NO_BONUS)
	if (pPlot->getNonObsoleteBonusType(getTeam()) != NO_BONUS)
	{
		bBuildRoute = true;
	}
	else if (pPlot->isHills())
	{
		if ((GC.getHILLS_EXTRA_MOVEMENT() > 0) && (iWorkersNeeded > 1))
		{
			bBuildRoute = true;
		}
	}
	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD						END								*/
	/********************************************************************************/
	
	if (pPlot->getRouteType() != NO_ROUTE)
	{
		bBuildRoute = false;
	}
	
	BuildTypes eBestBuild = NO_BUILD;
	int iBestValue = 0;
	for (int iBuild = 0; iBuild < GC.getNumBuildInfos(); iBuild++)
	{
		BuildTypes eBuild = ((BuildTypes)iBuild);
		CvBuildInfo& kBuildInfo = GC.getBuildInfo(eBuild);
		
		
		RouteTypes eRoute = (RouteTypes)kBuildInfo.getRoute();
		if ((bBuildRoute && (eRoute != NO_ROUTE)) || (bClearFeature && kBuildInfo.isFeatureRemove(eFeature)))
		{
			if (canBuild(pPlot, eBuild))
			{
				int iValue = 10000;
				
				if (bBuildRoute && (eRoute != NO_ROUTE))
				{
					iValue *= (1 + GC.getRouteInfo(eRoute).getValue());
					iValue /= 2;
					
					/********************************************************************************/
					/* 	BETTER_BTS_AI_MOD						7/31/08				jdog5000	*/
					/* 																			*/
					/* 	Bugfix																	*/
					/********************************************************************************/
					//if if (pPlot->getBonusType() != NO_BONUS)
					if (pPlot->getNonObsoleteBonusType(getTeam()) != NO_BONUS)
					{
						iValue *= 2;
					}
					/********************************************************************************/
					/* 	BETTER_BTS_AI_MOD						END								*/
					/********************************************************************************/
					
					if (pPlot->getWorkingCity() != NULL)
					{
						iValue *= 2 + iWorkersNeeded + ((pPlot->isHills() && (iWorkersNeeded > 1)) ? 2 * GC.getHILLS_EXTRA_MOVEMENT() : 0);
						iValue /= 3;
					}
					ImprovementTypes eImprovement = (ImprovementTypes)kOriginalBuildInfo.getImprovement();
					if (eImprovement != NO_IMPROVEMENT)
					{
						int iRouteMultiplier = ((GC.getImprovementInfo(eImprovement).getRouteYieldChanges(eRoute, YIELD_FOOD)) * 100);
						iRouteMultiplier += ((GC.getImprovementInfo(eImprovement).getRouteYieldChanges(eRoute, YIELD_PRODUCTION)) * 100);
						iRouteMultiplier += ((GC.getImprovementInfo(eImprovement).getRouteYieldChanges(eRoute, YIELD_COMMERCE)) * 60);
						iValue *= 100 + iRouteMultiplier;
						iValue /= 100;
					}
					
					int iPlotGroupId = -1;
					for (int iDirection = 0; iDirection < NUM_DIRECTION_TYPES; iDirection++)
					{
						CvPlot* pLoopPlot = plotDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), (DirectionTypes)iDirection);
						if (pLoopPlot != NULL)
						{
							if (pPlot->isRiver() || (pLoopPlot->getRouteType() != NO_ROUTE))
							{
								CvPlotGroup* pLoopGroup = pLoopPlot->getPlotGroup(getOwnerINLINE());
								if (pLoopGroup != NULL)
								{
									if (pLoopGroup->getID() != -1)
									{
										if (pLoopGroup->getID() != iPlotGroupId)
										{
											//This plot bridges plot groups, so route it.
											iValue *= 4;
											break;
										}
										else
										{
											iPlotGroupId = pLoopGroup->getID();
										}
									}
								}
							}
						}
					}	
				}

				iValue /= (kBuildInfo.getTime() + 1);

				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					eBestBuild = eBuild;
				}
			}
		}
	}
	
	if (eBestBuild == NO_BUILD)
	{
		return eBuild;
	}
	return eBestBuild;	
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_connectBonus(bool bTestTrade)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	BonusTypes eNonObsoleteBonus;
	int iI;

	// XXX how do we make sure that we can build roads???

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot))
		{
			if (pLoopPlot->getOwnerINLINE() == getOwnerINLINE()) // XXX team???
			{
				eNonObsoleteBonus = pLoopPlot->getNonObsoleteBonusType(getTeam());

				if (eNonObsoleteBonus != NO_BONUS)
				{
					if (!(pLoopPlot->isConnectedToCapital()))
					{
						//if (!bTestTrade || ((pLoopPlot->getImprovementType() != NO_IMPROVEMENT) && (GC.getImprovementInfo(pLoopPlot->getImprovementType()).isImprovementBonusTrade(eNonObsoleteBonus))))
						if (!bTestTrade || GET_PLAYER(getOwnerINLINE()).doesImprovementConnectBonus(pLoopPlot->getImprovementType(), eNonObsoleteBonus))
						{
							if (AI_connectPlot(pLoopPlot))
							{
								return true;
							}
						}
					}
				}
			}
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_connectCity()
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	int iLoop;

	// XXX how do we make sure that we can build roads???

    pLoopCity = plot()->getWorkingCity();
    if (pLoopCity != NULL)
    {
        if (AI_plotValid(pLoopCity->plot()))
        {
            if (!(pLoopCity->isConnectedToCapital()))
            {
                if (AI_connectPlot(pLoopCity->plot(), 1))
                {
                    return true;
                }
            }
        }
    }
	// <advc.300>
	if(isBarbarian())
		return false; // </advc.300>

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (AI_plotValid(pLoopCity->plot()))
		{
			if (!(pLoopCity->isConnectedToCapital()))
			{
				if (AI_connectPlot(pLoopCity->plot(), 1))
				{
					return true;
				}
			}
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_routeCity()
{
	PROFILE_FUNC();

	CvCity* pRouteToCity;
	CvCity* pLoopCity;
	int iLoop;

	FAssert(canBuildRoute());

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (AI_plotValid(pLoopCity->plot()))
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/22/10                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
			// BBAI efficiency: check area for land units before generating path
			if( (getDomainType() == DOMAIN_LAND) && (pLoopCity->area() != area()) && !(getGroup()->canMoveAllTerrain()) )
			{
				continue;
			}

			pRouteToCity = pLoopCity->AI_getRouteToCity();

			if (pRouteToCity != NULL)
			{
				if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
				{
					if (!(pRouteToCity->plot()->isVisibleEnemyUnit(this)))
					{
						if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pRouteToCity->plot(), MISSIONAI_BUILD, getGroup()) == 0)
						{
							if (generatePath(pLoopCity->plot(), MOVE_SAFE_TERRITORY
									| MOVE_ROUTE_TO, // advc.049
									true))
							{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
								if (generatePath(pRouteToCity->plot(), MOVE_SAFE_TERRITORY
										| MOVE_ROUTE_TO, // advc.049
										true))
								{
									getGroup()->pushMission(MISSION_ROUTE_TO,
											pLoopCity->getX_INLINE(),
											pLoopCity->getY_INLINE(),
											MOVE_SAFE_TERRITORY
											| MOVE_ROUTE_TO, // advc.049
											false, false, MISSIONAI_BUILD, pRouteToCity->plot());
									//getGroup()->pushMission(MISSION_ROUTE_TO, pRouteToCity->getX_INLINE(), pRouteToCity->getY_INLINE(), MOVE_SAFE_TERRITORY, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BUILD, pRouteToCity->plot());
									getGroup()->pushMission(MISSION_ROUTE_TO,
											pRouteToCity->getX_INLINE(),
											pRouteToCity->getY_INLINE(),
											MOVE_SAFE_TERRITORY
											| MOVE_ROUTE_TO, // advc.049
											true, false, MISSIONAI_BUILD, pRouteToCity->plot()); // K-Mod

									return true;
								}
							}
						}
					}
				}
			}
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_routeTerritory(bool bImprovementOnly)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	ImprovementTypes eImprovement;
	RouteTypes eBestRoute;
	bool bValid;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iI, iJ;

	// XXX how do we make sure that we can build roads???

	FAssert(canBuildRoute());

	iBestValue = 0;
	pBestPlot = NULL;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot))
		{
			if (pLoopPlot->getOwnerINLINE() == getOwnerINLINE()) // XXX team???
			{
				eBestRoute = GET_PLAYER(getOwnerINLINE()).getBestRoute(pLoopPlot);

				if (eBestRoute != NO_ROUTE)
				{
					if (eBestRoute != pLoopPlot->getRouteType())
					{
						if (bImprovementOnly)
						{
							bValid = false;

							eImprovement = pLoopPlot->getImprovementType();

							if (eImprovement != NO_IMPROVEMENT)
							{
								for (iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
								{
									if (GC.getImprovementInfo(eImprovement).getRouteYieldChanges(eBestRoute, iJ) > 0)
									{
										bValid = true;
										break;
									}
								}
							}
						}
						else
						{
							bValid = true;
						}

						if (bValid)
						{
							if (!(pLoopPlot->isVisibleEnemyUnit(this)))
							{
								if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BUILD, getGroup(), 1) == 0)
								{
									if (generatePath(pLoopPlot, MOVE_SAFE_TERRITORY
											| MOVE_ROUTE_TO, // advc.049
											true, &iPathTurns))
									{
										iValue = 10000;

										iValue /= (iPathTurns + 1);

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = pLoopPlot;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_ROUTE_TO, pBestPlot->getX_INLINE(),
				pBestPlot->getY_INLINE(), MOVE_SAFE_TERRITORY
				| MOVE_ROUTE_TO, // advc.049
				false, false, MISSIONAI_BUILD, pBestPlot);
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_travelToUpgradeCity()
{
	PROFILE_FUNC();

	// is there a city which can upgrade us?
	CvCity* pUpgradeCity = getUpgradeCity(/*bSearch*/ true);
	if (pUpgradeCity != NULL)
	{
		// cache some stuff
		CvPlot* pPlot = plot();
		bool bSeaUnit = (getDomainType() == DOMAIN_SEA);
		bool bCanAirliftUnit = (getDomainType() == DOMAIN_LAND);
		bool bShouldSkipToUpgrade = (getDomainType() != DOMAIN_AIR);

		// if we are at the upgrade city, stop, wait to get upgraded
		if (pUpgradeCity->plot() == pPlot)
		{
			if (!bShouldSkipToUpgrade)
			{
				return false;
			}
			
			getGroup()->pushMission(MISSION_SKIP);
			return true;
		}

		if (DOMAIN_AIR == getDomainType())
		{
			FAssert(!atPlot(pUpgradeCity->plot()));
			getGroup()->pushMission(MISSION_MOVE_TO, pUpgradeCity->getX_INLINE(), pUpgradeCity->getY_INLINE());
			return true;
		}

		// find the closest city
		CvCity* pClosestCity = pPlot->getPlotCity();
		bool bAtClosestCity = (pClosestCity != NULL);
		if (pClosestCity == NULL)
		{
			pClosestCity = pPlot->getWorkingCity();
		}
		if (pClosestCity == NULL)
		{
			pClosestCity = GC.getMapINLINE().findCity(getX_INLINE(), getY_INLINE(), NO_PLAYER, getTeam(), true, bSeaUnit);
		}

		// can we path to the upgrade city?
		int iUpgradeCityPathTurns;
		CvPlot* pThisTurnPlot = NULL;
		bool bCanPathToUpgradeCity = generatePath(pUpgradeCity->plot(), 0, true, &iUpgradeCityPathTurns);
		if (bCanPathToUpgradeCity)
		{
			pThisTurnPlot = getPathEndTurnPlot();
		}
		
		// if we close to upgrade city, head there 
		if (NULL != pThisTurnPlot && NULL != pClosestCity && (pClosestCity == pUpgradeCity || iUpgradeCityPathTurns < 4))
		{
			FAssert(!atPlot(pThisTurnPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pThisTurnPlot->getX_INLINE(), pThisTurnPlot->getY_INLINE());
			return true;
		}
		
		// check for better airlift choice
		if (bCanAirliftUnit && NULL != pClosestCity && pClosestCity->getMaxAirlift() > 0)
		{
			// if we at the closest city, then do the airlift, or wait
			if (bAtClosestCity)
			{
				// can we do the airlift this turn?
				if (canAirliftAt(pClosestCity->plot(), pUpgradeCity->getX_INLINE(), pUpgradeCity->getY_INLINE()))
				{
					getGroup()->pushMission(MISSION_AIRLIFT, pUpgradeCity->getX_INLINE(), pUpgradeCity->getY_INLINE());
					return true;
				}
				// wait to do it next turn
				else
				{
					getGroup()->pushMission(MISSION_SKIP);
					return true;
				}
			}
			
			int iClosestCityPathTurns;
			CvPlot* pThisTurnPlotForAirlift = NULL;
			bool bCanPathToClosestCity = generatePath(pClosestCity->plot(), 0, true, &iClosestCityPathTurns);
			if (bCanPathToClosestCity)
			{
				pThisTurnPlotForAirlift = getPathEndTurnPlot();
			}
			
			// is the closest city closer pathing? If so, move toward closest city
			if (NULL != pThisTurnPlotForAirlift && (!bCanPathToUpgradeCity || iClosestCityPathTurns < iUpgradeCityPathTurns))
			{
				FAssert(!atPlot(pThisTurnPlotForAirlift));
				getGroup()->pushMission(MISSION_MOVE_TO, pThisTurnPlotForAirlift->getX_INLINE(), pThisTurnPlotForAirlift->getY_INLINE(), 0, false, false, MISSIONAI_UPGRADE);
				return true;
			}
		}
		
		// did not have better airlift choice, go ahead and path to the upgrade city
		if (NULL != pThisTurnPlot)
		{
			FAssert(!atPlot(pThisTurnPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pThisTurnPlot->getX_INLINE(), pThisTurnPlot->getY_INLINE(), 0, false, false, MISSIONAI_UPGRADE);
			return true;
		}
	}

	return false;
}

// Returns true if a mission was pushed...
// K-Mod. The bAirlift parameter now means that airlift cities will be priotised, but other cities are still accepted.
bool CvUnitAI::AI_retreatToCity(bool bPrimary, bool bPrioritiseAirlift, int iMaxPath)
{
	PROFILE_FUNC();

	//int iCurrentDanger = GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot());
	int iCurrentDanger = getGroup()->alwaysInvisible() ? 0 : GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot()); // K-Mod

	CvCity* pCity = plot()->getPlotCity();

	if (0 == iCurrentDanger)
	{
		if (pCity != NULL)
		{
			if (pCity->getOwnerINLINE() == getOwnerINLINE())
			{
				if (!bPrimary || GET_PLAYER(getOwnerINLINE()).AI_isPrimaryArea(pCity->area()))
				{
					if (!bPrioritiseAirlift || (pCity->getMaxAirlift() > 0))
					{	//if (!(pCity->plot()->isVisibleEnemyUnit(this)))
						{
							getGroup()->pushMission(MISSION_SKIP);
							return true;
						}
					}
				}
			}
		}
	}

	//for (iPass = 0; iPass < 4; iPass++)
	/*  K-Mod. originally; pass 0 required the dest to have less plot danger
		unless the unit could fight;
		pass 1 was just an ordinary move; 
		pass 2 was a 1 turn move with "ignore plot danger" and
		pass 3 was the full iMaxPath with ignore plot danger.
		I've changed it so that if the unit can fight, the pass 0 just skipped
		(because it's the same as the pass 1) and
		pass 2 is always skipped because it's a useless test.
		-- and I've renumbered the passes. */
	CvPlot* pBestPlot = NULL;
	int iShortestPath = MAX_INT;
	// <advc.139>
	bool bEvac = (pCity != NULL && pCity->AI_isEvacuating());
	bool bSafe = (pCity != NULL && pCity->AI_isSafe());
	// </advc.139>
	// <advc.003>
	int iPass = 0; // Used after the loop
	CvPlayerAI const& kOwner = GET_PLAYER(getOwnerINLINE()); // </advc.003>
	for (iPass = ((getGroup()->canDefend()
			&& getDomainType() == DOMAIN_LAND) // advc.001s
			? 1 : 0); iPass < 3; iPass++)
	{	int iLoop=-1;
		bool bNeedsAirlift = false;
		for(CvCity* pLoopCity = kOwner.firstCity(&iLoop); pLoopCity != NULL;
				pLoopCity = kOwner.nextCity(&iLoop)) {
			if (!AI_plotValid(pLoopCity->plot()))
				continue;
			if (bPrimary && !kOwner.AI_isPrimaryArea(pLoopCity->area()))
				continue;
			if (bNeedsAirlift && pLoopCity->getMaxAirlift() == 0)
				continue;
			// <advc.139>
			/*  When evacuating, exclude other cities that also evacuate
				(and exclude the current city). */
			if(bEvac && pLoopCity->AI_isEvacuating())
				continue;
			/*  Avoid path and danger computation if we already know that we're safer
				where we are. */
			if(!pLoopCity->AI_isSafe() && (bSafe || iCurrentDanger <= 0 ||
					/*  Even when threatened at sea, a ship won't seek refuge in
						an unsafe city. */
					getDomainType() != DOMAIN_LAND))
				continue;
			// </advc.139>
			int iPathTurns=-1;
			if (generatePath(pLoopCity->plot(), (iPass >= 2 ? MOVE_IGNORE_DANGER : 0), true, &iPathTurns, iMaxPath)) // was iPass >= 3
			{/* (comment by jdog5000, 08/19/09)
				Water units can't defend a city
				Any unthreatened city acceptable on 0th pass, solves problem where sea units
				would oscillate in and out of threatened city because they had iCurrentDanger = 0
				on turns outside city */
				if (iPass > 0 || kOwner.AI_getPlotDanger(pLoopCity->plot()) <= iCurrentDanger)
				{
					// If this is the first viable air-lift city, then reset iShortestPath.
					if (bPrioritiseAirlift && !bNeedsAirlift && pLoopCity->getMaxAirlift() > 0)
					{
						bNeedsAirlift = true;
						iShortestPath = MAX_INT;
					}

					if (iPathTurns < iShortestPath
						/*  <advc.139> Don't want to be ambushed while evacuating.
							Since I'm handling evacuation on a per-unit basis, it's
							impossible to say how much danger along the path
							is tolerable. Don't just want to use MOVE_IGNORE_DANGER
							b/c then a single enemy could stop a dozen units from
							evacuating. Using era isn't much better than this ...
							Moreover, I'm only considering the fastest path,
							not any detours that could be safer.
							Adding a maxDanger parameter to generatePath
							(or generalizing MOVE_IGNORE_DANGER) could help. */
						&& (!bEvac || kOwner.AI_getPlotDanger(getPathEndTurnPlot())
						<= kOwner.getCurrentEra())
						// </advc.139>
						)
					{
						iShortestPath = iPathTurns;
						pBestPlot = getPathEndTurnPlot();
					}
				}
			}
		}

		if (pBestPlot != NULL)
		{
			break;
		}

		if (getGroup()->alwaysInvisible())
		{
			break;
		}
	}

	if (pBestPlot != NULL)
	{
		if (atPlot(pBestPlot))
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_RETREAT);
		else
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(),
					iPass >= 2 ? MOVE_IGNORE_DANGER : 0, // was iPass >= 3
					false, false, MISSIONAI_RETREAT);
		return true;
	}

	if (pCity != NULL)
	{
		if (pCity->getTeam() == getTeam())
		{
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_RETREAT);
			return true;
		}
	}

	return false;
}

// K-Mod
// Decide whether or not this group is stranded.
// If they are stranded, try to walk towards the coast.
// If we're on the coast, wait to be rescued!
bool CvUnitAI::AI_handleStranded(int iFlags)
{
	PROFILE_FUNC();

	// <advc.001> No place to go
	if(GET_PLAYER(getOwnerINLINE()).getNumCities() <= 0) {
		getGroup()->pushMission(MISSION_SKIP);
		return true;
	} // </advc.001>

	if (isCargo())
	{	// This is possible, in some rare cases, but I'm currently trying to pin down precisely what those cases are.
		//FAssertMsg(false, "AI_handleStranded: this unit is already cargo."); // advc.006
		getGroup()->pushMission(MISSION_SKIP);
		return true;
	}

	if (isHuman())
		return false;

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());

	// return false if the group is not stranded.
	int iDummy;
	if (area()->getNumAIUnits(getOwnerINLINE(), UNITAI_SETTLE) > 0 && kOwner.AI_getNumAreaCitySites(getArea(), iDummy) > 0)
	{
		return false;
	}

	if (area()->getNumCities() > 0)
	{
		if (plot()->getTeam() == getTeam())
			return false;

		if (getGroup()->isHasPathToAreaPlayerCity(getOwnerINLINE(), iFlags))
		{
			return false;
		}

		if ((canFight() || isSpy()) && getGroup()->isHasPathToAreaEnemyCity(true, iFlags))
		{
			return false;
		}
	}

	// ok.. so the group is stranded.
	// Try to get to the coast.
	if (!plot()->isCoastalLand())
	{
		// maybe we were already on our way?
		CvPlot* pMissionPlot = 0;
		CvPlot* pEndTurnPlot = 0;
		if (getGroup()->AI_getMissionAIType() == MISSIONAI_STRANDED)
		{
			pMissionPlot = getGroup()->AI_getMissionAIPlot();
			if (pMissionPlot && pMissionPlot->isCoastalLand() && !pMissionPlot->isVisibleEnemyUnit(this) && generatePath(pMissionPlot, iFlags, true))
			{
				// The current mission looks good enough. Don't bother searching for a better option.
				pEndTurnPlot = getPathEndTurnPlot();
			}
			else
			{
				// the current mission plot is not suitable. We'll have to search.
				pMissionPlot = 0;
			}
		}
		if (!pMissionPlot)
		{
			// look for the clostest coastal plot in this area
			int iShortestPath = MAX_INT;

			for (int i = 0; i < GC.getMapINLINE().numPlotsINLINE(); i++)
			{
				CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(i);

				if (pLoopPlot->getArea() == getArea() && pLoopPlot->isCoastalLand())
				{
					// TODO: check that the water isnt' blocked by ice.
					// advc.030 (comment): ^Should be guaranteed by pLoopPlot->getArea() == getArea() now
					int iPathTurns;
					if (generatePath(pLoopPlot, iFlags, true, &iPathTurns, iShortestPath))
					{
						FAssert(iPathTurns <= iShortestPath);
						iShortestPath = iPathTurns;
						pEndTurnPlot = getPathEndTurnPlot();
						pMissionPlot = pLoopPlot;
						if (iPathTurns <= 1)
							break;
					}
				}
			}
		}

		if (pMissionPlot)
		{
			FAssert(pEndTurnPlot);
			getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX_INLINE(), pEndTurnPlot->getY_INLINE(), iFlags, false, false, MISSIONAI_STRANDED, pMissionPlot);
			return true;
		}
	}

	// Hopefully we're on the coast. (but we might not be - if we couldn't find a path to the coast)
	// try to load into a passing boat
	// Calling AI_load will check all of our boats; so before we do that, I'm going to just see if there are any boats on adjacent plots.
	for (int i = NO_DIRECTION; i < NUM_DIRECTION_TYPES; i++)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX_INLINE(), getY_INLINE(), (DirectionTypes)i);

		if (pAdjacentPlot && canLoad(pAdjacentPlot))
		{
			// ok. there is something we can load into - but lets use the (slow) official function to actually issue the load command.
			if (AI_load(NO_UNITAI, NO_MISSIONAI, NO_UNITAI, -1, -1, -1, -1, iFlags, 1))
				return true;
			else // if that didn't do it, nothing will
				break;
		}
	}

	// raise the 'stranded' flag, and wait to be rescued.
	getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_STRANDED, plot());
	return true;
}
// K-Mod end

// Returns true if a mission was pushed...
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/15/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
/* original bts code
bool CvUnitAI::AI_pickup(UnitAITypes eUnitAI)
*/
bool CvUnitAI::AI_pickup(UnitAITypes eUnitAI, bool bCountProduction, int iMaxPath)
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
{
	PROFILE_FUNC();

	CvCity* pCity;
	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	CvPlot* pBestPickupPlot;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iLoop;

	FAssert(cargoSpace() > 0);
	if (0 == cargoSpace())
	{
		return false;
	}

	pCity = plot()->getPlotCity();

	if (pCity != NULL)
	{
		if (pCity->getOwnerINLINE() == getOwnerINLINE())
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/23/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
/* original bts code
			if (pCity->plot()->plotCount(PUF_isUnitAIType, eUnitAI, -1, getOwnerINLINE()) > 0)
			{
				if ((AI_getUnitAIType() != UNITAI_ASSAULT_SEA) || pCity->AI_isDefended(-1))
				{
*/
			if( (GC.getGameINLINE().getGameTurn() - pCity->getGameTurnAcquired()) > 15 || (GET_TEAM(getTeam()).countEnemyPowerByArea(pCity->area()) == 0) )
			{
				bool bConsider = false;

				if(AI_getUnitAIType() == UNITAI_ASSAULT_SEA)
				{
					// Improve island hopping
					if( pCity->area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE )
					{
						bConsider = false;
					}
					else if( eUnitAI == UNITAI_ATTACK_CITY && !(pCity->AI_isDanger()) )
					{
						bConsider = (pCity->plot()->plotCount(PUF_canDefend, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isDomainType, DOMAIN_LAND) > pCity->AI_neededDefenders());
					}
					else
					{
						bConsider = pCity->AI_isDefended(-1);
					}
				}
				else if(AI_getUnitAIType() == UNITAI_SETTLER_SEA)
				{
					if( eUnitAI == UNITAI_CITY_DEFENSE )
					{
						bConsider = (pCity->plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isCityAIType) > 1);
					}
					else
					{
						bConsider = true;
					}
				}
				else
				{
					bConsider = true;
				}
				
				if ( bConsider )
				{
					// only count units which are available to load 
					int iCount = pCity->plot()->plotCount(PUF_isAvailableUnitAITypeGroupie, eUnitAI, -1, getOwnerINLINE(), NO_TEAM, PUF_isFiniteRange);
					
					if (bCountProduction && (pCity->getProductionUnitAI() == eUnitAI))
					{
						if( pCity->getProductionTurnsLeft() < 4 )
						{
							CvUnitInfo& kUnitInfo = GC.getUnitInfo(pCity->getProductionUnit());
							if ((kUnitInfo.getDomainType() != DOMAIN_AIR) || kUnitInfo.getAirRange() > 0)
							{
								iCount++;
							}
						}
					}

					if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pCity->plot(), MISSIONAI_PICKUP, getGroup()) < ((iCount + (cargoSpace() - 1)) / cargoSpace()))
					{
						getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_PICKUP, pCity->plot());
						return true;
					}
				}
			}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		}
	}

	iBestValue = 0;
	pBestPlot = NULL;
	pBestPickupPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (AI_plotValid(pLoopCity->plot()))
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/23/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
			if( (GC.getGameINLINE().getGameTurn() - pLoopCity->getGameTurnAcquired()) > 15 || (GET_TEAM(getTeam()).countEnemyPowerByArea(pLoopCity->area()) == 0) )
			{
				bool bConsider = false;

				if(AI_getUnitAIType() == UNITAI_ASSAULT_SEA)
				{
					if( pLoopCity->area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE )
					{
						bConsider = false;
					}
					else if( eUnitAI == UNITAI_ATTACK_CITY && !(pLoopCity->AI_isDanger()) )
					{
						// Improve island hopping
						bConsider = (pLoopCity->plot()->plotCount(PUF_canDefend, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isDomainType, DOMAIN_LAND) > pLoopCity->AI_neededDefenders());
					}
					else
					{
						bConsider = pLoopCity->AI_isDefended(-1);
					}
				}
				else if(AI_getUnitAIType() == UNITAI_SETTLER_SEA)
				{
					if( eUnitAI == UNITAI_CITY_DEFENSE )
					{
						bConsider = (pLoopCity->plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isCityAIType) > 1);
					}
					else
					{
						bConsider = true;
					}
				}
				else
				{
					bConsider = true;
				}

				if ( bConsider )
				{
					// only count units which are available to load, have had a chance to move since being built
					int iCount = pLoopCity->plot()->plotCount(PUF_isAvailableUnitAITypeGroupie, eUnitAI, -1, getOwnerINLINE(), NO_TEAM, (bCountProduction ? PUF_isFiniteRange : PUF_isFiniteRangeAndNotJustProduced));

					iValue = iCount * 10;
					
					if (bCountProduction && (pLoopCity->getProductionUnitAI() == eUnitAI))
					{
						CvUnitInfo& kUnitInfo = GC.getUnitInfo(pLoopCity->getProductionUnit());
						if ((kUnitInfo.getDomainType() != DOMAIN_AIR) || kUnitInfo.getAirRange() > 0)
						{
							iValue++;
							iCount++;
						}
					}

					if (iValue > 0)
					{
						iValue += pLoopCity->getPopulation();

						if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
						{
							if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_PICKUP, getGroup()) < ((iCount + (cargoSpace() - 1)) / cargoSpace()))
							{
								if( !(pLoopCity->AI_isDanger()) )
								{
									if (!atPlot(pLoopCity->plot()) && generatePath(pLoopCity->plot(), MOVE_AVOID_ENEMY_WEIGHT_3, true, &iPathTurns))
									{
										if( AI_getUnitAIType() == UNITAI_ASSAULT_SEA )
										{
											if( pLoopCity->area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT )
											{
												iValue *= 4;
											}
											else if( pLoopCity->area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT_ASSIST )
											{
												iValue *= 2;
											}
										}

										iValue *= 1000;

										iValue /= (iPathTurns + 3);

										if( (iValue > iBestValue) && (iPathTurns <= iMaxPath) )
										{
											iBestValue = iValue;
											// Do one turn along path, then reevaluate
											// Causes update of destination based on troop movement
											//pBestPlot = pLoopCity->plot();
											pBestPlot = getPathEndTurnPlot();
											pBestPickupPlot = pLoopCity->plot();

											if( pBestPlot == NULL || atPlot(pBestPlot) )
											{
												//FAssert(false);
												pBestPlot = pBestPickupPlot;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if ((pBestPlot != NULL) && (pBestPickupPlot != NULL))
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_AVOID_ENEMY_WEIGHT_3, false, false, MISSIONAI_PICKUP, pBestPickupPlot);
		return true;
	}

	return false;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/22/10                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
// Returns true if a mission was pushed...
// (this function has been significantly edited for K-Mod)
bool CvUnitAI::AI_pickupStranded(UnitAITypes eUnitAI, int iMaxPath)
{
	PROFILE_FUNC();

	FAssert(cargoSpace() > 0);
	if (0 == cargoSpace())
	{
		return false;
	}

	if( isBarbarian() )
	{
		return false;
	}

	int iBestValue = 0;
	CvUnit* pBestUnit = 0;

	CvPlot* pEndTurnPlot = 0;
	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());

	int iLoop;
	for(CvSelectionGroup* pLoopGroup = kPlayer.firstSelectionGroup(&iLoop); pLoopGroup != NULL; pLoopGroup = kPlayer.nextSelectionGroup(&iLoop))
	{
		if( pLoopGroup->isStranded() )
		{
			CvUnit* pHeadUnit = pLoopGroup->getHeadUnit();
			if( pHeadUnit == NULL )
			{
				continue;
			}

			if( (eUnitAI != NO_UNITAI) && (pHeadUnit->AI_getUnitAIType() != eUnitAI) )
			{
				continue;
			}

			//pLoopPlot = pHeadUnit->plot();
			CvPlot* pPickupPlot = pLoopGroup->AI_getMissionAIPlot(); // K-Mod

			if( pPickupPlot == NULL  )
			{
				continue;
			}

			if( !(pPickupPlot->isCoastalLand())  && !canMoveAllTerrain() )
			{
				continue;
			}

			// Units are stranded, attempt rescue

			int iCount = pLoopGroup->getNumUnits();
			
			if( 1000*iCount > iBestValue )
			{
				CvPlot* pTargetPlot = 0;
				int iPathTurns = MAX_INT;

				for (int iI = NO_DIRECTION; iI < NUM_DIRECTION_TYPES; iI++)
				{
					CvPlot* pAdjacentPlot = plotDirection(pPickupPlot->getX_INLINE(), pPickupPlot->getY_INLINE(), ((DirectionTypes)iI));

					if (pAdjacentPlot && (atPlot(pAdjacentPlot) || canMoveInto(pAdjacentPlot)) && generatePath(pAdjacentPlot, 0, true, &iPathTurns, iMaxPath))
					{
						pTargetPlot = getPathEndTurnPlot();
						break;
					}
				}

				if (pTargetPlot)
				{
					FAssert(iMaxPath < 0 || iPathTurns <= iMaxPath);

					MissionAITypes eMissionAIType = MISSIONAI_PICKUP;
					iCount -= GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(pHeadUnit, &eMissionAIType, 1, getGroup(), iPathTurns) * cargoSpace(); // estimate

					int iValue = 1000*iCount;

					iValue /= (iPathTurns + 1);

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestUnit = pHeadUnit;
						pEndTurnPlot = pTargetPlot;
					}
				}
			}
		}
	}

	if (pBestUnit)
	{	/*  <advc.046> Previously, the caller ensured that we have space. Now
			ensures that we have space or can unload. */
		int iCargo = getGroup()->getCargo();
		if(iCargo > 0) {
			/*  Only unload the current cargo if the stranded units aren't too few
				or too far away. */
			if(iCargo * 150 > iBestValue || atPlot(pEndTurnPlot))
				return false;
			FAssert(plot()->isCity(false, getTeam()));
			getGroup()->unloadAll();
			if(getGroup()->hasCargo())
				return false;
		} // </advc.046>
		FAssert(pEndTurnPlot);
		if (atPlot(pEndTurnPlot))
		{
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_PICKUP, 0, pBestUnit);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestUnit->plot()));
			//getGroup()->pushMission(MISSION_MOVE_TO_UNIT, pBestUnit->getOwnerINLINE(), pBestUnit->getID(), MOVE_AVOID_ENEMY_WEIGHT_3, false, false, MISSIONAI_PICKUP, NULL, pBestUnit);
			getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX_INLINE(), pEndTurnPlot->getY_INLINE(), 0, false, false, MISSIONAI_PICKUP, 0, pBestUnit);
			return true;
		}
	}

	return false;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


// Returns true if a mission was pushed...
bool CvUnitAI::AI_airOffensiveCity()
{
	//PROFILE_FUNC();

	CvPlot* pBestPlot;
	int iValue;
	int iBestValue;
	int iI;

	FAssert(canAirAttack() || nukeRange() >= 0);

	iBestValue = 0;
	pBestPlot = NULL;

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						04/25/08			jdog5000		*/
/* 																				*/
/* 	Air AI																		*/
/********************************************************************************/
	/* original BTS code

	*/
	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		// Limit to cities and forts, true for any city but only this team's forts
		if (pLoopPlot->isCity(true, getTeam()))
		{
			if (pLoopPlot->getTeam() == getTeam() || (pLoopPlot->isOwned() && GET_TEAM(pLoopPlot->getTeam()).isVassal(getTeam())))
			{
				if (atPlot(pLoopPlot) || canMoveInto(pLoopPlot))
				{
					iValue = AI_airOffenseBaseValue( pLoopPlot );

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestPlot = pLoopPlot;
					}
				}
			}
		}
	}
	
	if (pBestPlot != NULL)
	{
		if (!atPlot(pBestPlot))
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_SAFE_TERRITORY);
			return true;
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END									*/
/********************************************************************************/

	return false;
}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						04/25/10			jdog5000		*/
/* 																				*/
/* 	Air AI																		*/
/********************************************************************************/
// Function for ranking the value of a plot as a base for offensive air units
int CvUnitAI::AI_airOffenseBaseValue( CvPlot* pPlot )
{
	if( pPlot == NULL || pPlot->area() == NULL )
	{
		return 0;
	}

	/*int iOurOffense; // advc.003: unused
	int iEnemyDefense;*/

	int iAttackAirCount = pPlot->plotCount(PUF_canAirAttack, -1, -1, NO_PLAYER, getTeam());
	iAttackAirCount += 2 * pPlot->plotCount(PUF_isUnitAIType, UNITAI_ICBM, -1, NO_PLAYER, getTeam());
	if (atPlot(pPlot))
	{
		iAttackAirCount += canAirAttack() ? -1 : 0;
		iAttackAirCount += (nukeRange() >= 0) ? -2 : 0;
	}
	int iDefenders = pPlot->plotCount(PUF_canDefend, -1, -1, pPlot->getOwner(),
			NO_TEAM, PUF_isDomainType, DOMAIN_LAND); // advc.001s
	if( pPlot->isCoastalLand(GC.getMIN_WATER_SIZE_FOR_OCEAN()) )
	{
		iDefenders -= 1;
	}
	CvCity* pCity = pPlot->getPlotCity();
	if(pCity != NULL)
	{
		if( pCity->getDefenseModifier(true) < 40 )
		{
			iDefenders -= 1;
		}

		if( pCity->getOccupationTimer() > 1 )
		{
			iDefenders -= 1;
		}
	}

	// Consider threat from nearby enemy territory
	int iRange = 1;
	int iBorderDanger = 0;

	for (int iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (int iDY = -(iRange); iDY <= iRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->area() == pPlot->area() && pLoopPlot->isOwned())
				{	// <advc.001i>
					if(!pLoopPlot->isRevealed(getTeam(), false))
						continue; // </advc.001i>
				    int iDistance = stepDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
				    if(pLoopPlot->getTeam() != getTeam() &&
							!GET_TEAM(pLoopPlot->getTeam()).isVassal(getTeam()) )
					{
						if( iDistance == 1 )
						{
							iBorderDanger++;
						}

						if (atWar(pLoopPlot->getTeam(), getTeam()))
						{
							if (iDistance == 1)
							{
								iBorderDanger += 2;
							}
							else if (iDistance == 2 && pLoopPlot->//isRoute()
									// advc.001i:
									getRevealedRouteType(getTeam(), false))
								iBorderDanger += 2;
						}
					}
				}
			}
		}
	}

	iDefenders -= std::min(2,(iBorderDanger + 1)/3);
	
	// Don't put more attack air units on plot than effective land defenders ... too large a risk
	if (iAttackAirCount >= (iDefenders) || iDefenders <= 0)
	{
		return 0;
	}
	
	bool bAnyWar = (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0);

	int iValue = 0;

	if( bAnyWar )
	{
		// Don't count assault assist, don't want to weight defending colonial coasts when homeland might be under attack
		bool bAssault = (pPlot->area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT) || (pPlot->area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT_MASSING);

		// Loop over operational range
		iRange = airRange();

		for (int iDX = -(iRange); iDX <= iRange; iDX++)
		{
			for (int iDY = -(iRange); iDY <= iRange; iDY++)
			{
				CvPlot* pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);
				
				if ((pLoopPlot != NULL && pLoopPlot->area() != NULL))
				{
					int iDistance = plotDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());

					if( iDistance <= iRange )
					{
						// Value system is based around 1 enemy military unit in our territory = 10 pts
						int iTempValue = 0;					

						if( pLoopPlot->isWater() )
						{
							if( pLoopPlot->isVisible(getTeam(),false) && !pLoopPlot->area()->isLake()  )
							{
								// Defend ocean
								iTempValue = 1;
								
								if( pLoopPlot->isOwned() )
								{
									if( pLoopPlot->getTeam() == getTeam() )
									{
										iTempValue += 1;
									}
									else if ((pLoopPlot->getTeam() != getTeam()) && GET_TEAM(getTeam()).AI_getWarPlan(pLoopPlot->getTeam()) != NO_WARPLAN)
									{
										iTempValue += 1;
									}
								}

								// Low weight for visible ships cause they will probably move
								iTempValue += 2*pLoopPlot->getNumVisibleEnemyDefenders(this);

								if( bAssault )
								{
									iTempValue *= 2;
								}
							}
						}
						else 
						{
							if( !(pLoopPlot->isOwned()) )
							{
								if( iDistance < (iRange - 2) )
								{
									// Target enemy troops in neutral territory
									iTempValue += 4*pLoopPlot->getNumVisibleEnemyDefenders(this);
								}
							}
							else if( pLoopPlot->getTeam() == getTeam() )
							{
								iTempValue = 0;

								if( iDistance < (iRange - 2) )
								{
									// Target enemy troops in our territory
									iTempValue += 5*pLoopPlot->getNumVisibleEnemyDefenders(this);

									if( pLoopPlot->getOwnerINLINE() == getOwnerINLINE() )
									{
										if( GET_PLAYER(getOwnerINLINE()).AI_isPrimaryArea(pLoopPlot->area()) )
										{
											iTempValue *= 3;
										}
										else
										{
											iTempValue *= 2;
										}
									}
									// advc.003: unused
									//bool bOffensive = pLoopPlot->area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE;
									bool bDefensive = (pLoopPlot->area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE);
									if( bDefensive )
									{
										iTempValue *= 2;
									}
								}
							}
							else if ((pLoopPlot->getTeam() != getTeam()) && GET_TEAM(getTeam()).AI_getWarPlan(pLoopPlot->getTeam()) != NO_WARPLAN)
							{
								// Attack opponents land territory
								iTempValue = 3;

								CvCity* pLoopCity = pLoopPlot->getPlotCity();

								if (pLoopCity != NULL)
								{
									// Target enemy cities
									iTempValue += (3*pLoopCity->getPopulation() + 30);

									//if( canAirBomb(pPlot) && pLoopCity->isBombardable(this) )
									if (canAirBombAt(pPlot, pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE())) // K-Mod
									{
										iTempValue *= 2;
									}

									if( pLoopPlot->area()->getTargetCity(getOwnerINLINE()) == pLoopCity )
									{
										iTempValue *= 2;
									}

									if( pLoopCity->AI_isDanger() )
									{
										// Multiplier for nearby troops, ours, teammate's, and any other enemy of city
										iTempValue *= 3;
									}
								}
								else
								{
									if( iDistance < (iRange - 2) )
									{
										// Support our troops in enemy territory
										iTempValue += 15*pLoopPlot->getNumDefenders(getOwnerINLINE());

										// Target enemy troops adjacent to our territory
										if( pLoopPlot->isAdjacentTeam(getTeam(),true) )
										{
											iTempValue += 7*pLoopPlot->getNumVisibleEnemyDefenders(this);
										}
									}

									// Weight resources
									if (canAirBombAt(pPlot, pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
									{
										if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
										{
											iTempValue += 8*std::max(2, GET_PLAYER(pLoopPlot->getOwnerINLINE()).AI_bonusVal(pLoopPlot->getBonusType(getTeam()),0)/10);
										}
									}
								}

								if( (pLoopPlot->area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE) )
								{
									// Extra weight for enemy territory in offensive areas
									iTempValue *= 2;
								}

								if( GET_PLAYER(getOwnerINLINE()).AI_isPrimaryArea(pLoopPlot->area()) )
								{
									iTempValue *= 3;
									iTempValue /= 2;
								}

								if( pLoopPlot->isBarbarian() )
								{
									iTempValue /= 2;
								}
							}
						}

						iValue += iTempValue;
					}
				}
			}
		}

		// Consider available defense, direct threat to potential base
		// K-Mod
		int iOurDefense = GET_PLAYER(getOwnerINLINE()).AI_localDefenceStrength(pPlot, getTeam(), DOMAIN_LAND, 0);
		int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_localAttackStrength(pPlot, NO_TEAM, DOMAIN_LAND, 2);
		// K-Mod end

		if( 3*iEnemyOffense > iOurDefense || iOurDefense == 0 )
		{
			iValue *= iOurDefense;
			iValue /= std::max(1,3*iEnemyOffense);
		}

		// Value forts less, they are generally riskier bases
		if( pCity == NULL )
		{
			iValue *= 2;
			iValue /= 3;
		}
	}
	else
	{
		if( pPlot->getOwnerINLINE() != getOwnerINLINE() )
		{
			// Keep planes at home when not in real wars
			return 0;
		}

		// If no wars, use prior logic with added value to keeping planes safe from sneak attack
		if (pCity != NULL)
		{
			/* original bts code
			iValue = (pCity->getPopulation() + 20);
			iValue += pCity->AI_cityThreat(); */

			// K-Mod. Try not to waste airspace which we need for air defenders; but use the needed air defenders as a proxy for good offense placement.
			// AI_cityThreat has arbitrary scale, so it should not be added to population like that.
			// (the rest of this function still needs some work, but this bit was particularly problematic.)
			// advc.003: Removed unnecessary static_cast to CvCityAI*
			int iDefNeeded = pCity->AI_neededAirDefenders();
			int iDefHere = pPlot->plotCount(PUF_isAirIntercept, -1, -1, NO_PLAYER, getTeam()) - (atPlot(pPlot) && PUF_isAirIntercept(this, -1, -1) ? 1 : 0);
			int iSpace = pPlot->airUnitSpaceAvailable(getTeam()) + (atPlot(pPlot) ? 1 : 0);
			iValue = pCity->getPopulation() + 20;
			iValue *= std::min(iDefNeeded+1, iDefHere+iSpace);
			if (iDefNeeded > iSpace+iDefHere)
			{
				FAssert(iDefNeeded > 0);
				// drop value to zero if we can't even fit half of the air defenders we need here.
				iValue *= 2*(iSpace+iDefHere) - iDefNeeded;
				iValue /= iDefNeeded;
			}
			// K-Mod end
		}
		else
		{
			if( iDefenders > 0 )
			{
				iValue = (pCity != NULL) ? 0 : GET_PLAYER(getOwnerINLINE()).AI_getPlotAirbaseValue(pPlot);
				iValue /= 6;
			}
		}

		iValue += std::min(24, 3*(iDefenders - iAttackAirCount));

		if( GET_PLAYER(getOwnerINLINE()).AI_isPrimaryArea(pPlot->area()) )
		{
			iValue *= 4;
			iValue /= 3;
		}

		// No real enemies, check for minor civ or barbarian cities where attacks could be supported
		CvCity* pNearestEnemyCity = GC.getMapINLINE().findCity(pPlot->getX_INLINE(), pPlot->getY_INLINE(), NO_PLAYER, NO_TEAM, false, false, getTeam());

		if (pNearestEnemyCity != NULL)
		{
			int iDistance = plotDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pNearestEnemyCity->getX_INLINE(), pNearestEnemyCity->getY_INLINE());
			if (iDistance > airRange())
			{
				iValue /= 10 * (2 + airRange());
			}
			else
			{
				iValue /= 2 + iDistance;
			}
		}
	}
	
	if (pPlot->getOwnerINLINE() == getOwnerINLINE())
	{
		// Bases in our territory better than teammate's
		iValue *= 2;
	}
	else if( pPlot->getTeam() == getTeam() )
	{
		// Our team's bases are better than vassal plots
		iValue *= 3;
		iValue /= 2;
	}

	return iValue;
}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

// Returns true if a mission was pushed...
// Most of this function has been rewritten for K-Mod, using bbai as the base version. (old code deleted.)
bool CvUnitAI::AI_airDefensiveCity()
{
	PROFILE_FUNC();

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod

	FAssert(getDomainType() == DOMAIN_AIR);
	FAssert(canAirDefend());

	if (canAirDefend() && getDamage() == 0)
	{
		CvCity* pCity = plot()->getPlotCity();

		if (pCity && pCity->getOwnerINLINE() == getOwnerINLINE())
		{
			int iExistingAirDefenders = plot()->plotCount(PUF_isAirIntercept, -1, -1, getOwnerINLINE());
			if (PUF_isAirIntercept(this, -1, -1))
				iExistingAirDefenders--;
			int iNeedAirDefenders = pCity->AI_neededAirDefenders();

			if (iExistingAirDefenders < iNeedAirDefenders/2 && iExistingAirDefenders < 3)
			{
				// Be willing to defend with a couple of planes even if it means their doom.
				getGroup()->pushMission(MISSION_AIRPATROL);
				return true;
			}

			if (iExistingAirDefenders < iNeedAirDefenders)
			{
				// Stay if city is threatened or if we're well short of our target, but not if capture is imminent.
				int iEnemyOffense = kOwner.AI_localAttackStrength(plot(), NO_TEAM, DOMAIN_LAND, 2);

				if (iEnemyOffense > 0 || iExistingAirDefenders < iNeedAirDefenders/2)
				{
					int iOurDefense = kOwner.AI_localDefenceStrength(plot(), getTeam());
					if (iEnemyOffense < iOurDefense)
					{
						getGroup()->pushMission(MISSION_AIRPATROL);
						return true;
					}
				}
			}
		}
	}
	
	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;

	int iLoop;
	for (CvCity* pLoopCity = kOwner.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kOwner.nextCity(&iLoop))
	{
		if (!canAirDefend(pLoopCity->plot()))
			continue;

		if (!atPlot(pLoopCity->plot()) && !canMoveInto(pLoopCity->plot()))
			continue;

		bool bCurrentPlot = atPlot(pLoopCity->plot());

		int iExistingAirDefenders = pLoopCity->plot()->plotCount(PUF_isAirIntercept, -1, -1, pLoopCity->getOwnerINLINE());
		if (bCurrentPlot && PUF_isAirIntercept(this, -1, -1))
			iExistingAirDefenders--;
		int iNeedAirDefenders = pLoopCity->AI_neededAirDefenders();
		int iAirSpaceAvailable = pLoopCity->plot()->airUnitSpaceAvailable(kOwner.getTeam()) + (bCurrentPlot ? 1 : 0);

		if (iNeedAirDefenders > iExistingAirDefenders || iAirSpaceAvailable > 1)
		{
			/* int iValue = pLoopCity->getPopulation() + pLoopCity->AI_cityThreat();
			iValue *= 100;
			iValue *= std::max(1, 3 + iNeedAirDefenders - iExistingAirDefenders); */
			// K-Mod note: AI_cityThreat is too expensive for this stuff, and it's already taken into account by AI_neededAirDefenders anyway

			int iOurDefense = kOwner.AI_localDefenceStrength(pLoopCity->plot(), getTeam(), DOMAIN_LAND, 0);
			int iEnemyOffense = kOwner.AI_localAttackStrength(pLoopCity->plot(), NO_TEAM, DOMAIN_LAND, 2);

			int iValue = 10 + iAirSpaceAvailable;
			iValue *= 10 * std::max(0, iNeedAirDefenders - iExistingAirDefenders) + 1;

			if (bCurrentPlot && iAirSpaceAvailable > 1)
				iValue = iValue * 4/3;

			if( kOwner.AI_isPrimaryArea(pLoopCity->area()) )
			{
				iValue *= 4;
				iValue /= 3;
			}

			if (pLoopCity->getPreviousOwner() != getOwnerINLINE())
			{
				iValue *= (GC.getGameINLINE().getGameTurn() - pLoopCity->getGameTurnAcquired() < 20 ? 3 : 4);
				iValue /= 5;
			}

			// Reduce value of endangered city, it may be too late to help
			if (iEnemyOffense > 0)
			{
				if (iOurDefense == 0)
					iValue = 0;
				else if (iEnemyOffense*4 > iOurDefense*3)
				{
					// note: this will drop to zero when iEnemyOffense = 1.5 * iOurDefence.
					iValue *= 6*iOurDefense - 4*iEnemyOffense;
					iValue /= 3*iOurDefense;
				}
			}

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				pBestPlot = pLoopCity->plot();
			}
		}
	}

	if (pBestPlot != NULL && !atPlot(pBestPlot))
	{
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_airCarrier()
{
	//PROFILE_FUNC();
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod

	CvUnit* pLoopUnit;
	CvUnit* pBestUnit;
	int iValue;
	int iBestValue;
	int iLoop;

	if (getCargo() > 0)
	{
		return false;
	}

	if (isCargo())
	{
		if (canAirDefend())
		{
			getGroup()->pushMission(MISSION_AIRPATROL);
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_SKIP);
			return true;
		}
	}

	iBestValue = 0;
	pBestUnit = NULL;

	for(pLoopUnit = kOwner.firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = kOwner.nextUnit(&iLoop))
	{
		if (canLoadUnit(pLoopUnit, pLoopUnit->plot()))
		{
			iValue = 10;

			if (!(pLoopUnit->plot()->isCity()))
			{
				iValue += 20;
			}

			if (pLoopUnit->plot()->isOwned())
			{
				if (isEnemy(pLoopUnit->plot()->getTeam(), pLoopUnit->plot()))
				{
					iValue += 20;
				}
			}
			else
			{
				iValue += 10;
			}

			iValue /= (pLoopUnit->getCargo() + 1);

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				pBestUnit = pLoopUnit;
			}
		}
	}

	if (pBestUnit != NULL)
	{
		if (atPlot(pBestUnit->plot()))
		{
			setTransportUnit(pBestUnit); // XXX is this dangerous (not pushing a mission...) XXX air units?
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestUnit->getX_INLINE(), pBestUnit->getY_INLINE());
			return true;
		}
	}

	return false;
}

bool CvUnitAI::AI_missileLoad(UnitAITypes eTargetUnitAI, int iMaxOwnUnitAI, bool bStealthOnly)
{
	//PROFILE_FUNC();

	CvUnit* pBestUnit = NULL;
	int iBestValue = 0;
	int iLoop;
	CvUnit* pLoopUnit;
	for(pLoopUnit = GET_PLAYER(getOwnerINLINE()).firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = GET_PLAYER(getOwnerINLINE()).nextUnit(&iLoop))
	{
		if (!bStealthOnly || pLoopUnit->getInvisibleType() != NO_INVISIBLE)
		{
			if (pLoopUnit->AI_getUnitAIType() == eTargetUnitAI)
			{
				if ((iMaxOwnUnitAI == -1) || (pLoopUnit->getUnitAICargo(AI_getUnitAIType()) <= iMaxOwnUnitAI))
				{
					if (canLoadUnit(pLoopUnit, pLoopUnit->plot()))
					{
						int iValue = 100;
						
						iValue += GC.getGame().getSorenRandNum(100, "AI missile load");

						iValue *= 1 + pLoopUnit->getCargo();

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestUnit = pLoopUnit;
						}
					}
				}
			}
		}
	}

	if (pBestUnit != NULL)
	{
		if (atPlot(pBestUnit->plot()))
		{
			setTransportUnit(pBestUnit); // XXX is this dangerous (not pushing a mission...) XXX air units?
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestUnit->getX_INLINE(), pBestUnit->getY_INLINE());
			setTransportUnit(pBestUnit);
			return true;
		}
	}

	return false;	
	
}


// Returns true if a mission was pushed...
// K-Mod. I'm rewritten this function so that it now considers bombarding city defences and bombing improvements
// as well as air strikes against enemy troops. Also, it now prefers to hit targets that are in our territory.
bool CvUnitAI::AI_airStrike(int iThreshold)
{
	PROFILE_FUNC();

	int iSearchRange = airRange();

	int iBestValue = iThreshold + isSuicide() && m_pUnitInfo->getProductionCost() > 0 ? m_pUnitInfo->getProductionCost() * 5 / 6 : 0;
	CvPlot* pBestPlot = NULL;
	bool bBombard = false; // K-Mod. bombard (city / improvement), rather than air strike (damage)

	for (int iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (int iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				int iStrikeValue = 0;
				int iBombValue = 0;
				int iAdjacentAttackers = 0; // (only count adjacent units if we can air-strike)
				int iAssaultEnRoute = pLoopPlot->isCity() ? GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_ASSAULT, getGroup(), 1) : 0;

				// TODO: consider changing the evaluation system so that instead of simply counting units, it counts attack / defence power.

				// air strike (damage)
				if (canMoveInto(pLoopPlot, true))
				{
					iAdjacentAttackers += GET_PLAYER(getOwnerINLINE()).AI_adjacentPotentialAttackers(pLoopPlot);
					//if (pLoopPlot->isWater() || (iPotentialAttackers > 0) || pLoopPlot->isAdjacentTeam(getTeam()))
					{
						CvUnit* pDefender = pLoopPlot->getBestDefender(NO_PLAYER, getOwnerINLINE(), this, true);

						FAssert(pDefender != NULL);
						FAssert(pDefender->canDefend());

						int iDamage = airCombatDamage(pDefender);
						int iDefenders = pLoopPlot->getNumVisibleEnemyDefenders(this);

						iStrikeValue = std::max(0, std::min(pDefender->getDamage() + iDamage, airCombatLimit()) - pDefender->getDamage());

						iStrikeValue += iDamage * collateralDamage() * std::min(iDefenders - 1, collateralDamageMaxUnits()) / 200;

						iStrikeValue *= (3 + iAdjacentAttackers + iAssaultEnRoute / 2);
						iStrikeValue /= (iAdjacentAttackers + iAssaultEnRoute > 0 ? 4 : 6) + std::min(iAdjacentAttackers + iAssaultEnRoute / 2, iDefenders)/2;

						if (pLoopPlot->isCity(true, pDefender->getTeam()))
						{
							// units heal more easily in a city / fort
							iStrikeValue *= 3;
							iStrikeValue /= 4;
						}
						if (pLoopPlot->isWater() && (iAdjacentAttackers > 0 || pLoopPlot->getTeam() == getTeam()))
						{
							iStrikeValue *= 3;
						}
						else if (pLoopPlot->isAdjacentTeam(getTeam())) // prefer defensive strikes
						{
							iStrikeValue *= 2;
						}
					}
				}
				// bombard (destroy improvement / city defences)
				if (canAirBombAt(plot(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
				{
					if (pLoopPlot->isCity())
					{
						const CvCity* pCity = pLoopPlot->getPlotCity();
						if(pCity->getDefenseModifier(true) > 0) { // advc.004c
							iBombValue = std::max(0, std::min(pCity->getDefenseDamage() + airBombCurrRate(), GC.getMAX_CITY_DEFENSE_DAMAGE()) - pCity->getDefenseDamage());
							iBombValue *= iAdjacentAttackers + 2*iAssaultEnRoute + (area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE ? 5 : 1);
							iBombValue /= 2;
						}
					}
					else
					{
						BonusTypes eBonus = pLoopPlot->getNonObsoleteBonusType(getTeam(), true);
						if (eBonus != NO_BONUS && pLoopPlot->isOwned() && canAirBombAt(plot(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
						{
							iBombValue = GET_PLAYER(pLoopPlot->getOwner()).AI_bonusVal(eBonus, -1);
							iBombValue += GET_PLAYER(pLoopPlot->getOwner()).AI_bonusVal(eBonus, 0);
						}
					}
				}
				// factor in air defenses but try to avoid using bestInterceptor, because that's a slow function.
				if (iBombValue > iBestValue || iStrikeValue > iBestValue) // values only decreased from here on.
				{
					if (isSuicide())
					{
						iStrikeValue /= 2;
						iBombValue /= 2;
					}
					else if (!canAirDefend()) // assume that air defenders are strong.. and that they are willing to fight
					{
						CvUnit* pInterceptor = bestInterceptor(pLoopPlot);

						if (pInterceptor != NULL)
						{
							int iInterceptProb = pInterceptor->currInterceptionProbability();

							iInterceptProb *= std::max(0, (100 - evasionProbability()));
							iInterceptProb /= 100;

							iStrikeValue *= std::max(0, 100 - iInterceptProb / 2);
							iStrikeValue /= 100;
							iBombValue *= std::max(0, 100 - iInterceptProb / 2);
							iBombValue /= 100;
						}
					}

					if (iStrikeValue > iBestValue || iBombValue > iBestValue)
					{
						bBombard = iBombValue > iStrikeValue;
						iBestValue = std::max(iBombValue, iStrikeValue);
						pBestPlot = pLoopPlot;
						FAssert(!atPlot(pBestPlot));
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		if (bBombard)
			getGroup()->pushMission(MISSION_AIRBOMB, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		else
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}

// Air strike around base city
// Returns true if a mission was pushed...
bool CvUnitAI::AI_defendBaseAirStrike()
{
	PROFILE_FUNC();

	CvUnit* pDefender;
	CvUnit* pInterceptor;
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iDamage;
	int iInterceptProb;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	// Only search around base
	int iSearchRange = 2;

	iBestValue = (isSuicide() && m_pUnitInfo->getProductionCost() > 0) ? (15 * m_pUnitInfo->getProductionCost()) : 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (canMoveInto(pLoopPlot, true) && !pLoopPlot->isWater()) // Only true of plots this unit can airstrike
				{
					if( plot()->area() == pLoopPlot->area() )
					{
						iValue = 0;

						pDefender = pLoopPlot->getBestDefender(NO_PLAYER, getOwnerINLINE(), this, true);

						FAssert(pDefender != NULL);
						FAssert(pDefender->canDefend());

						iDamage = airCombatDamage(pDefender);

						iValue = std::max(0, (std::min((pDefender->getDamage() + iDamage), airCombatLimit()) - pDefender->getDamage()));

						iValue += ((iDamage * collateralDamage()) * std::min((pLoopPlot->getNumVisibleEnemyDefenders(this) - 1), collateralDamageMaxUnits())) / (2*100);

						// Weight towards stronger units
						iValue *= (pDefender->currCombatStr(NULL,NULL,NULL) + 2000);
						iValue /= 2000;

						// Weight towards adjacent stacks
						if( plotDistance(plot()->getX_INLINE(), plot()->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()) == 1 )
						{
							iValue *= 5;
							iValue /= 4;
						}

						pInterceptor = bestInterceptor(pLoopPlot);

						if (pInterceptor != NULL)
						{
							iInterceptProb = isSuicide() ? 100 : pInterceptor->currInterceptionProbability();

							iInterceptProb *= std::max(0, (100 - evasionProbability()));
							iInterceptProb /= 100;

							iValue *= std::max(0, 100 - iInterceptProb / 2);
							iValue /= 100;
						}

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopPlot;
							FAssert(!atPlot(pBestPlot));
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

bool CvUnitAI::AI_airBombPlots()
{
	//PROFILE_FUNC();

	CvUnit* pInterceptor;
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iSearchRange;
	int iInterceptProb;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	iSearchRange = airRange();

	iBestValue = 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (!pLoopPlot->isCity() && pLoopPlot->isOwned() && pLoopPlot != plot())
				{
					if (canAirBombAt(plot(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
					{
						iValue = 0;

						if (pLoopPlot->getBonusType(pLoopPlot->getTeam()) != NO_BONUS)
						{
							iValue += AI_pillageValue(pLoopPlot, 15);

							iValue += GC.getGameINLINE().getSorenRandNum(10, "AI Air Bomb");
						}
						else if (isSuicide())
						{
							//This should only be reached when the unit is desperate to die
							iValue += AI_pillageValue(pLoopPlot);
							// Guided missiles lean towards destroying resource-producing tiles as opposed to improvements like Towns
							if (pLoopPlot->getBonusType(pLoopPlot->getTeam()) != NO_BONUS)
							{
								//and even more so if it's a resource
								iValue += GET_PLAYER(pLoopPlot->getOwnerINLINE()).AI_bonusVal(pLoopPlot->getBonusType(pLoopPlot->getTeam()), 0);
							}
						}

						if (iValue > 0)
						{

							/* original bts code
							pInterceptor = bestInterceptor(pLoopPlot);

							if (pInterceptor != NULL)
							{
								iInterceptProb = isSuicide() ? 100 : pInterceptor->currInterceptionProbability();

								iInterceptProb *= std::max(0, (100 - evasionProbability()));
								iInterceptProb /= 100;

								iValue *= std::max(0, 100 - iInterceptProb / 2);
								iValue /= 100;
							} */
							// K-Mod. Try to avoid using bestInterceptor... because that's a slow function.
							if (isSuicide())
							{
								iValue /= 2;
							}
							else if (!canAirDefend()) // assume that air defenders are strong.. and that they are willing to fight
							{
								pInterceptor = bestInterceptor(pLoopPlot);

								if (pInterceptor != NULL)
								{
									iInterceptProb = pInterceptor->currInterceptionProbability();

									iInterceptProb *= std::max(0, (100 - evasionProbability()));
									iInterceptProb /= 100;

									iValue *= std::max(0, 100 - iInterceptProb / 2);
									iValue /= 100;
								}
							}
							// K-Mod end

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pLoopPlot;
								FAssert(!atPlot(pBestPlot));
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_AIRBOMB, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}	

/* disabled by K-Mod - this is now handled in AI_airStrike.
bool CvUnitAI::AI_airBombDefenses()
{
	//PROFILE_FUNC();

	CvCity* pCity;
	CvUnit* pInterceptor;
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iSearchRange;
	int iPotentialAttackers;
	int iInterceptProb;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	iSearchRange = airRange();

	iBestValue = 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				pCity = pLoopPlot->getPlotCity();
				if (pCity != NULL)
				{
					iValue = 0;

					if (canAirBombAt(plot(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
					{
						iPotentialAttackers = GET_PLAYER(getOwnerINLINE()).AI_adjacentPotentialAttackers(pLoopPlot);
						iPotentialAttackers += std::max(0, GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pCity->plot(), NO_MISSIONAI, getGroup(), 2) - 4);

						if (iPotentialAttackers > 1)
						{
							iValue += std::max(0, (std::min((pCity->getDefenseDamage() + airBombCurrRate()), GC.getMAX_CITY_DEFENSE_DAMAGE()) - pCity->getDefenseDamage()));

							iValue *= 4 + iPotentialAttackers;

							if (pCity->AI_isDanger())
							{
								iValue *= 2;
							}

							if (pCity == pCity->area()->getTargetCity(getOwnerINLINE()))
							{
								iValue *= 2;
							}
						}

						if (iValue > 0)
						{
							pInterceptor = bestInterceptor(pLoopPlot);

							if (pInterceptor != NULL)
							{
								iInterceptProb = isSuicide() ? 100 : pInterceptor->currInterceptionProbability();

								iInterceptProb *= std::max(0, (100 - evasionProbability()));
								iInterceptProb /= 100;

								iValue *= std::max(0, 100 - iInterceptProb / 2);
								iValue /= 100;
							}

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pLoopPlot;
								FAssert(!atPlot(pBestPlot));
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_AIRBOMB, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;	
	
} */

bool CvUnitAI::AI_exploreAir()
{
	PROFILE_FUNC();
	
	CvPlayer& kPlayer = GET_PLAYER(getOwnerINLINE());
	CvPlot* pBestPlot = NULL;
	int iBestValue = 0;
	
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive() && !GET_PLAYER((PlayerTypes)iI).isBarbarian())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam())
			{
				int iLoop;
				for (CvCity* pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
				{
					if (!pLoopCity->isVisible(getTeam(), false))
					{
						if (canReconAt(plot(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE()))
						{
							int iValue = 1 + GC.getGame().getSorenRandNum(15, "AI explore air");
							if (isEnemy(GET_PLAYER((PlayerTypes)iI).getTeam()))
							{
								iValue += 10;
								iValue += std::min(10,  pLoopCity->area()->getNumAIUnits(getOwnerINLINE(), UNITAI_ATTACK_CITY));
								iValue += 10 * kPlayer.AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_ASSAULT);
							}
							
							iValue *= plotDistance(getX_INLINE(), getY_INLINE(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE());
							
							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pLoopCity->plot();								
							}
						}
					}
				}
			}
		}
	}
	
	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_RECON, pBestPlot->getX(), pBestPlot->getY());
		return true;
	}
	
	return false;	
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/02/09                                jdog5000      */
/*                                                                                              */
/* Player Interface                                                                             */
/************************************************************************************************/
int CvUnitAI::AI_exploreAirPlotValue( CvPlot* pPlot )
{
	int iValue = 0;
	if (pPlot->isVisible(getTeam(), false))
	{
		iValue++;

		if (!pPlot->isOwned())
		{
			iValue++;
		}

		if (!pPlot->isImpassable())
		{
			iValue *= 4;

			if (pPlot->isWater() || pPlot->getArea() == getArea())
			{
				iValue *= 2;
			}
		}
	}

	return iValue;
}

bool CvUnitAI::AI_exploreAir2()
{
	PROFILE_FUNC();
	
	CvPlot* pBestPlot = NULL;
	int iBestValue = 0;
	int iSearchRange = airRange();
	for (int iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (int iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if( pLoopPlot != NULL )
			{
				if( !pLoopPlot->isVisible(getTeam(),false) )
				{
					if (canReconAt(plot(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
					{
						int iValue = AI_exploreAirPlotValue( pLoopPlot );

						for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
						{
							DirectionTypes eDirection = (DirectionTypes) iI;
							CvPlot* pAdjacentPlot = plotDirection(getX_INLINE(), getY_INLINE(), eDirection);
							if (pAdjacentPlot != NULL)
							{
								if( !pAdjacentPlot->isVisible(getTeam(),false) )
								{
									iValue += AI_exploreAirPlotValue( pAdjacentPlot );
								}
							}
						}

						iValue += GC.getGame().getSorenRandNum(25, "AI explore air");
						iValue *= std::min(7, plotDistance(getX_INLINE(), getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()));
		
						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopPlot;								
						}
					}
				}
			}
		}
	}
	
	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_RECON, pBestPlot->getX(), pBestPlot->getY());
		return true;
	}
	
	return false;	
}

void CvUnitAI::AI_exploreAirMove()
{
	if( AI_exploreAir() )
	{
		return;
	}

	if( AI_exploreAir2() )
	{
		return;
	}

	if( canAirDefend() )
	{
		getGroup()->pushMission(MISSION_AIRPATROL);
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


// Returns true if a mission was pushed...
// This function has been completely rewritten for K-Mod.
bool CvUnitAI::AI_nuke()
{
	PROFILE_FUNC();

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	const CvTeamAI& kTeam = GET_TEAM(kOwner.getTeam());

	bool bDanger = kOwner.AI_getAnyPlotDanger(plot(), 2); // consider changing this to something smarter
	int iWarRating = kTeam.AI_getWarSuccessRating();
	// iBaseWeight is the civ-independant part of the weight for civilian damage evaluation
	int iBaseWeight = 10;
	iBaseWeight += kOwner.AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3) || GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI)
			? 10 : 0; // advc.019: was ?20:0
	iBaseWeight += kOwner.AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST4) ? 20 : 0;
	iBaseWeight += std::max(0, -iWarRating);
	iBaseWeight -= std::max(0, iWarRating - 50); // don't completely destroy them if we want to keep their land.

	CvPlot* pBestTarget = 0;
	// the initial value of iBestValue is the threshold for action. (cf. units of AI_nukeValue)
	int iBestValue = std::max(0, 4 * getUnitInfo().getProductionCost());
	iBestValue += bDanger || kOwner.AI_isDoStrategy(AI_STRATEGY_DAGGER) ? 20 : 100;
	iBestValue *= std::max(1, kOwner.getNumNukeUnits() + 2 * kOwner.getNumCities());
	iBestValue /= std::max(1, 2 * kOwner.getNumNukeUnits() + (bDanger ? 2 : 1) * kOwner.getNumCities());
	iBestValue *= 150 + iWarRating;
	iBestValue /= 150;

	for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i = (PlayerTypes)(i+1))
	{
		const CvPlayer& kLoopPlayer = GET_PLAYER(i);
		if (kLoopPlayer.isAlive() && isEnemy(kLoopPlayer.getTeam()))
		{
			int iDestructionWeight = iBaseWeight - kOwner.AI_getAttitudeWeight(i) / 2 + std::min(60,
				//2 * // advc.130j: Cancels out with the new way of counting memory
				kOwner.AI_getMemoryCount(i, MEMORY_NUKED_FRIEND) +
				//5 * // advc.130j: Make it 3 *
				3 * kOwner.AI_getMemoryCount(i, MEMORY_NUKED_US));
			int iLoop;
			for (CvCity* pLoopCity = kLoopPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kLoopPlayer.nextCity(&iLoop))
			{
				// note: we could use "AI_deduceCitySite" here, but if we can't see the city, then we can't properly judge its target value anyway.
				if (pLoopCity->isRevealed(getTeam(), false) && canNukeAt(plot(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE()))
				{
					CvPlot* pTarget;
					int iValue = AI_nukeValue(pLoopCity->plot(), nukeRange(), pTarget, iDestructionWeight);
					iValue /= (kTeam.AI_getWarPlan(pLoopCity->getTeam()) == WARPLAN_LIMITED && iWarRating > -10) ? 2 : 1;

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestTarget = pTarget;
					}
				}
			}
		}
	}

	if (pBestTarget)
	{
		FAssert(canNukeAt(plot(), pBestTarget->getX_INLINE(), pBestTarget->getY_INLINE()));
		getGroup()->pushMission(MISSION_NUKE, pBestTarget->getX_INLINE(), pBestTarget->getY_INLINE());
		return true;
	}

	return false;
}

// this function has been completely rewritten for K-Mod
bool CvUnitAI::AI_nukeRange(int iRange)
{
	PROFILE_FUNC();

	int iThresholdValue = 60 + std::max(0, 3 * getUnitInfo().getProductionCost());
	if (!GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), DANGER_RANGE))
		iThresholdValue = iThresholdValue * 3/2;

	CvPlot* pTargetPlot = 0;
	int iNukeValue = AI_nukeValue(plot(), iRange, pTargetPlot);
	if (iNukeValue > iThresholdValue)
	{
		FAssert(pTargetPlot && canNukeAt(plot(), pTargetPlot->getX_INLINE(), pTargetPlot->getY_INLINE()));
		getGroup()->pushMission(MISSION_NUKE, pTargetPlot->getX_INLINE(), pTargetPlot->getY_INLINE());
		return true;
	}
	return false;
}

#if 0 // old code disabled by K-Mod
bool CvUnitAI::AI_nukeRange(int iRange)
{
	CvPlot* pBestPlot = NULL;
	int iBestValue = 0;
	for (int iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (int iDY = -(iRange); iDY <= iRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);
			
			if (pLoopPlot != NULL)
			{
				if (canNukeAt(plot(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
				{
					int iValue = -99;
					
					for (int iDX2 = -(nukeRange()); iDX2 <= nukeRange(); iDX2++)
					{
						for (int iDY2 = -(nukeRange()); iDY2 <= nukeRange(); iDY2++)
						{
							CvPlot* pLoopPlot2 = plotXY(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), iDX2, iDY2);

							if (pLoopPlot2 != NULL)
							{
								int iEnemyCount = 0;
								int iTeamCount = 0;
								int iNeutralCount = 0;
								int iDamagedEnemyCount = 0;
								
								CLLNode<IDInfo>* pUnitNode;
								CvUnit* pLoopUnit;
								pUnitNode = pLoopPlot2->headUnitNode();
								while (pUnitNode != NULL)
								{
									pLoopUnit = ::getUnit(pUnitNode->m_data);
									pUnitNode = pLoopPlot2->nextUnitNode(pUnitNode);
									
									if (!pLoopUnit->isNukeImmune())
									{
										if (pLoopUnit->getTeam() == getTeam())
										{
											iTeamCount++;
										}
										else if (!pLoopUnit->isInvisible(getTeam(), false))
										{
											if (isEnemy(pLoopUnit->getTeam()))
											{
												iEnemyCount++;
												if (pLoopUnit->getDamage() * 2 > pLoopUnit->maxHitPoints())
												{
													iDamagedEnemyCount++;
												}
											}
											else
											{
												iNeutralCount++;
											}
										}
									}
								}
								
								//iValue += (iEnemyCount + iDamagedEnemyCount) * (pLoopPlot2->isWater() ? 25 : 12);
								iValue += (iEnemyCount + std::min(0, iEnemyCount/2-iDamagedEnemyCount)) * (pLoopPlot2->isWater() ? 25 : 12); // K-Mod
								iValue -= iTeamCount * 15;
								iValue -= iNeutralCount * 20;
								
								
								int iMultiplier = 1;
								if (pLoopPlot2->getTeam() == getTeam())
								{
									iMultiplier = -2;
								}
								else if (isEnemy(pLoopPlot2->getTeam()))
								{
									iMultiplier = 1;
								}
								else if (!pLoopPlot2->isOwned())
								{
									iMultiplier = 0;
								}
								else
								{
									iMultiplier = -10;
								}
								
								if (pLoopPlot2->getImprovementType() != NO_IMPROVEMENT)
								{
									iValue += iMultiplier * 10;
								}

								/********************************************************************************/
								/* 	BETTER_BTS_AI_MOD						7/31/08				jdog5000	*/
								/* 																			*/
								/* 	Bugfix																	*/
								/********************************************************************************/
								// This could also have been considered a minor AI cheat
								//if (pLoopPlot2->getBonusType() != NO_BONUS)
								if (pLoopPlot2->getNonObsoleteBonusType(getTeam()) != NO_BONUS)
								{
									iValue += iMultiplier * 20;
								}
								/********************************************************************************/
								/* 	BETTER_BTS_AI_MOD						END								*/
								/********************************************************************************/
								
								if (pLoopPlot2->isCity())
								{
									iValue += std::max(0, iMultiplier * (-20 + 15 * pLoopPlot2->getPlotCity()->getPopulation()));
								}
							}
						}
					}
					
					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestPlot = pLoopPlot;
					}
				}
			}
		}
	}
	
	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_NUKE, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}
	
	return false;
}
#endif // end old AI_nukeRnage code.

// K-Mod. Get the best trade mission value.
// Note. The iThreshold parameter is only there to improve efficiency.
int CvUnitAI::AI_tradeMissionValue(CvPlot*& pBestPlot, int iThreshold)
{
	pBestPlot = NULL;

	int iBestValue = 0;
	int iBestPathTurns = INT_MAX;
	int iLoop;

	if (getUnitInfo().getBaseTrade() <= 0 && getUnitInfo().getTradeMultiplier() <= 0)
		return 0;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			for (CvCity* pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
			{
				if (AI_plotValid(pLoopCity->plot()) && !pLoopCity->plot()->isVisibleEnemyUnit(this))
				{
					int iValue = getTradeGold(pLoopCity->plot());
					int iPathTurns;

					if (iValue >= iThreshold && canTrade(pLoopCity->plot()))
					{
						if (generatePath(pLoopCity->plot(), MOVE_NO_ENEMY_TERRITORY, true, &iPathTurns))
						{
							if (iValue / (4 + iPathTurns) > iBestValue / (4 + iBestPathTurns))
							{
								iBestValue = iValue;
								iBestPathTurns = iPathTurns;
								pBestPlot = getPathEndTurnPlot();
								iThreshold = std::max(iThreshold, iBestValue * 4 / (4 + iBestPathTurns));
							}
						}
					}
				}
			}
		}
	}

	return iBestValue;
}

// K-Mod. Move to destination for a trade mission. (pTradePlot is either the target city, or the end-turn plot.)
bool CvUnitAI::AI_doTradeMission(CvPlot* pTradePlot)
{
	if (pTradePlot != NULL)
	{
		if (atPlot(pTradePlot))
		{
			FAssert(canTrade(pTradePlot));
			if (canTrade(pTradePlot))
			{
				getGroup()->pushMission(MISSION_TRADE);
				return true;
			}
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pTradePlot->getX_INLINE(), pTradePlot->getY_INLINE(), MOVE_NO_ENEMY_TERRITORY, false, false, MISSIONAI_TRADE);
			return true;
		}
	}

	return false;
}

// find the best place to do create a great work (culture bomb)
int CvUnitAI::AI_greatWorkValue(CvPlot*& pBestPlot, int iThreshold)
{
	pBestPlot = NULL;

	int iBestValue = 0;
	int iBestPathTurns = INT_MAX;
	int iLoop;

	if (getUnitInfo().getGreatWorkCulture() == 0)
		return 0;

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	for (CvCity* pLoopCity = kOwner.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kOwner.nextCity(&iLoop))
	{
		if (AI_plotValid(pLoopCity->plot()) && !pLoopCity->plot()->isVisibleEnemyUnit(this))
		{
			int iValue = getGreatWorkCulture(pLoopCity->plot()) * kOwner.AI_commerceWeight(COMMERCE_CULTURE, pLoopCity) / 100;
			// commerceWeight takes into account culture pressure and cultural victory strategy.
			// However, it is intended to be used for evaluating steady culture rates rather than bursts.
			// Therefore the culture-pressure side of it is probably under-rated, and the cultural
			// victory part is based on current culture rather than turns to legendary.
			// Also, it doesn't take into account the possibility of flipping enemy cities.
			// ... But it's a good start.
			int iPathTurns;

			if (iValue >= iThreshold && canGreatWork(pLoopCity->plot()))
			{
				if (generatePath(pLoopCity->plot(), MOVE_NO_ENEMY_TERRITORY, true, &iPathTurns))
				{
					if (iValue / (4 + iPathTurns) > iBestValue / (4 + iBestPathTurns))
					{
						iBestValue = iValue;
						iBestPathTurns = iPathTurns;
						pBestPlot = getPathEndTurnPlot();
						iThreshold = std::max(iThreshold, iBestValue * 4 / (4 + iBestPathTurns));
					}
				}
			}
		}
	}

	return iBestValue;
}

// create great work if we're at pCulturePlot, otherwise just move towards pCulturePlot.
bool CvUnitAI::AI_doGreatWork(CvPlot* pCulturePlot)
{
	if (pCulturePlot != NULL)
	{
		if (atPlot(pCulturePlot))
		{
			FAssert(canGreatWork(pCulturePlot));
			if (canGreatWork(pCulturePlot))
			{
				getGroup()->pushMission(MISSION_GREAT_WORK);
				return true;
			}
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pCulturePlot->getX_INLINE(), pCulturePlot->getY_INLINE(), MOVE_NO_ENEMY_TERRITORY, false, false, MISSIONAI_GREAT_WORK);
			return true;
		}
	}

	return false;
}

// K-Mod end

/*  advc.003j (comment): Unused - i.e. the AI doesn't use the Infiltrate mission.
	There is a call commented out in the BtS version of this file (CvUnitAI::AI_spyMove). */
bool CvUnitAI::AI_infiltrate()
{
	PROFILE_FUNC();

	if (canInfiltrate(plot()))
	{
		getGroup()->pushMission(MISSION_INFILTRATE);
		return true;
	}

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if ((GET_PLAYER((PlayerTypes)iI).isAlive()) && GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam())
		{
			int iLoop;
			for (CvCity* pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
			{
				if (canInfiltrate(pLoopCity->plot()))
				{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/22/10                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
					// BBAI efficiency: check area for land units before generating path
					if( (getDomainType() == DOMAIN_LAND) && (pLoopCity->area() != area()) && !(getGroup()->canMoveAllTerrain()) )
					{
						continue;
					}

					int iValue = getEspionagePoints(pLoopCity->plot());
					
					if (iValue > iBestValue)
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
					{
						int iPathTurns;
						if (generatePath(pLoopCity->plot(), 0, true, &iPathTurns))
						{
							FAssert(iPathTurns > 0);
							
							if (getPathFinder().GetFinalMoves() == 0)
							{
								iPathTurns++;
							}

							iValue /= 1 + iPathTurns;

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pLoopCity->plot();
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL))
	{
		if (atPlot(pBestPlot))
		{
			getGroup()->pushMission(MISSION_INFILTRATE);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
			//getGroup()->pushMission(MISSION_INFILTRATE, -1, -1, 0, (getGroup()->getLengthMissionQueue() > 0));
			getGroup()->pushMission(MISSION_INFILTRATE, -1, -1, 0, true); // K-Mod
			return true;
		}
	}

	return false;
}

bool CvUnitAI::AI_reconSpy(int iRange)
{
	PROFILE_FUNC();
	CvPlot* pLoopPlot;
	int iX, iY;
	
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestTargetPlot = NULL;
	int iBestValue = 0;
	
	int iSearchRange = AI_searchRange(iRange);

	for (iX = -iSearchRange; iX <= iSearchRange; iX++)
	{
		for (iY = -iSearchRange; iY <= iSearchRange; iY++)
		{
			pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iX, iY);
			int iDistance = stepDistance(0, 0, iX, iY);
			if ((iDistance > 0) && (pLoopPlot != NULL) && AI_plotValid(pLoopPlot))
			{
				int iValue = 0;
				if (pLoopPlot->getPlotCity() != NULL)
				{
					iValue += GC.getGameINLINE().getSorenRandNum(2400, "AI Spy Scout City"); // was 4000
				}
				
				if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
				{
					iValue += GC.getGameINLINE().getSorenRandNum(800, "AI Spy Recon Bonus"); // was 1000
				}
				
				for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
				{
					CvPlot* pAdjacentPlot = plotDirection(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), ((DirectionTypes)iI));

					if (pAdjacentPlot != NULL)
					{
						if (!pAdjacentPlot->isRevealed(getTeam(), false))
						{
							iValue += 500;
						}
						else if (!pAdjacentPlot->isVisible(getTeam(), false))
						{
							iValue += 200;
						}
					}
				}
				// K-Mod
				if (pLoopPlot->getTeam() == getTeam())
					iValue /= 4;
				else if (isPotentialEnemy(pLoopPlot->getTeam(), pLoopPlot))
					iValue *= 2;
				// K-Mod end


				if (iValue > 0)
				{
					int iPathTurns;
					if (generatePath(pLoopPlot, 0, true, &iPathTurns, iRange))
					{
						if (iPathTurns <= iRange)
						{
							// don't give each and every plot in range a value before generating the patch (performance hit)
							iValue += GC.getGameINLINE().getSorenRandNum(250, "AI Spy Scout Best Plot");

							iValue *= iDistance;

							/* Can no longer perform missions after having moved
							if (getPathLastNode()->m_iData2 == 1)
							{
								if (getPathLastNode()->m_iData1 > 0)
								{
									//Prefer to move and have movement remaining to perform a kill action.
									iValue *= 2;
								}
							} */

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestTargetPlot = getPathEndTurnPlot();
								pBestPlot = pLoopPlot;
							}
						}
					}
				}
			}
		}
	}
	
	if ((pBestPlot != NULL) && (pBestTargetPlot != NULL))
	{
		if (atPlot(pBestTargetPlot))
		{
			getGroup()->pushMission(MISSION_SKIP);
			return true;
		}
		else
		{
			/* original bts code
			getGroup()->pushMission(MISSION_MOVE_TO, pBestTargetPlot->getX_INLINE(), pBestTargetPlot->getY_INLINE());
			getGroup()->pushMission(MISSION_SKIP); */
			// K-Mod. (skip turn after each step of a recon mission? strange)
			getGroup()->pushMission(MISSION_MOVE_TO, pBestTargetPlot->getX_INLINE(), pBestTargetPlot->getY_INLINE(), 0, false, false, MISSIONAI_RECON_SPY, pBestPlot);
			// K-Mod end
			return true;
		}
	}

	return false;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/25/09                               jdog5000        */
/*                                                                                               */
/* Espionage AI                                                                                  */
/************************************************************************************************/
/// \brief Spy decision on whether to cause revolt in besieged city
///
/// Have spy breakdown city defenses if we have troops in position to capture city this turn.
bool CvUnitAI::AI_revoltCitySpy()
{
	PROFILE_FUNC();

	CvCity* pCity = plot()->getPlotCity();

	FAssert(pCity != NULL);

	if( pCity == NULL )
	{
		return false;
	}

	if( !(GET_TEAM(getTeam()).isAtWar(pCity->getTeam())) )
	{
		return false;
	}

	if( pCity->isDisorder() )
	{
		return false;
	}

	// K-Mod
	if (100 * (GC.getMAX_CITY_DEFENSE_DAMAGE() - pCity->getDefenseDamage())/std::max(1, GC.getMAX_CITY_DEFENSE_DAMAGE()) < 15)
		return false;
	// K-Mod end

	/* original BBAI code
	int iOurPower = GET_PLAYER(getOwnerINLINE()).AI_getOurPlotStrength(plot(),1,false,true);
	int iEnemyDefensePower = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),0,true,false);
	int iEnemyPostPower = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),0,false,false);

	if( iOurPower > 2*iEnemyDefensePower )
	{
		return false;
	}

	if( iOurPower < iEnemyPostPower )
	{
		return false;
	}

	if( 10*iEnemyDefensePower < 11*iEnemyPostPower )
	{
		return false;
	} */ // Disabled by K-Mod. The power comparisons are done in AI_spyMove().

	for (int iMission = 0; iMission < GC.getNumEspionageMissionInfos(); ++iMission)
	{
		CvEspionageMissionInfo& kMissionInfo = GC.getEspionageMissionInfo((EspionageMissionTypes)iMission);
		if ((kMissionInfo.getCityRevoltCounter() > 0) || (kMissionInfo.getPlayerAnarchyCounter() > 0))
		{
			/* if (!GET_PLAYER(getOwnerINLINE()).canDoEspionageMission((EspionageMissionTypes)iMission, pCity->getOwnerINLINE(), pCity->plot(), -1, this))
			{
				continue;
			}
			
			if (!espionage((EspionageMissionTypes)iMission, -1))
			{
				continue;
			}

			return true; */
			// K-Mod
			if (GET_PLAYER(getOwnerINLINE()).canDoEspionageMission((EspionageMissionTypes)iMission, pCity->getOwnerINLINE(), pCity->plot(), -1, this))
			{
				if (gUnitLogLevel > 2) logBBAI("      %S uses city revolt at %S.", GET_PLAYER(getOwnerINLINE()).getCivilizationDescription(0), pCity->getName().GetCString());
				getGroup()->pushMission(MISSION_ESPIONAGE, iMission);
				return true;
			}
			// K-Mod end
		}
	}

	return false;
}

// K-Mod, I've moved the pathfinding check out of this function.
//int CvUnitAI::AI_getEspionageTargetValue(CvPlot* pPlot, int iMaxPath)
int CvUnitAI::AI_getEspionageTargetValue(CvPlot* pPlot)
{
	PROFILE_FUNC();

	int iValue = 0;

	if (pPlot->isOwned() && pPlot->getTeam() != getTeam() && !GET_TEAM(getTeam()).isVassal(pPlot->getTeam()))
	{
		if (AI_plotValid(pPlot))
		{
			CvCity* pCity = pPlot->getPlotCity();
			if (pCity != NULL)
			{
				iValue += pCity->getPopulation();
				iValue += pCity->plot()->calculateCulturePercent(getOwnerINLINE())/8;

				int iRand = GC.getGame().getSorenRandNum(6, "AI spy choose city");
				iValue += iRand * iRand;

				if( area()->getTargetCity(getOwnerINLINE()) == pCity )
				{
					iValue += 30;
				}

				if( GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pPlot, MISSIONAI_ASSAULT, getGroup()) > 0 )
				{
					iValue += 30;
				}
				// K-Mod. Dilute the effect of population, and take cost modifiers into account.
				iValue += 10;
				iValue *= 100;
				iValue /= GET_PLAYER(getOwnerINLINE()).getEspionageMissionCostModifier(NO_ESPIONAGEMISSION, pCity->getOwner(), pPlot);
				// K-Mod end.
			}
			else
			{
				BonusTypes eBonus = pPlot->getNonObsoleteBonusType(getTeam(), true);
				if (eBonus != NO_BONUS)
				{
					iValue += GET_PLAYER(pPlot->getOwnerINLINE()).AI_baseBonusVal(eBonus) - 10;
				}
			}
			/* original bts code (moved out of this function)
			int iPathTurns;
			if (generatePath(pPlot, 0, true, &iPathTurns))
			{
				if (iPathTurns <= iMaxPath)
				{
					if (kTeam.AI_getWarPlan(pPlot->getTeam()) == NO_WARPLAN)
					{
						iValue *= 1;
					}
					else if (kTeam.AI_isSneakAttackPreparing(pPlot->getTeam()))
					{
						iValue *= (pPlot->isCity()) ? 15 : 10;						
					}
					else
					{
						iValue *= 3;
					}

					iValue *= 3;
					iValue /= (3 + GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pPlot, MISSIONAI_ATTACK_SPY, getGroup()));
				}
			} */
		}
	}

	return iValue;
}

// heavily edited for K-Mod
bool CvUnitAI::AI_cityOffenseSpy(int iMaxPath, CvCity* pSkipCity)
{
	PROFILE_FUNC();

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	CvPlot* pEndTurnPlot = NULL;

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	const CvTeamAI& kTeam = GET_TEAM(getTeam());

	const int iEra = kOwner.getCurrentEra();
	int iBaselinePoints = 50 * iEra * (iEra+1); // cf the "big espionage" minimum value.
	int iAverageUnspentPoints;
	{
		int iTeamCount = 0;
		int iTotalUnspentPoints = 0;
		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			const CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
			if (iI != getTeam() && kLoopTeam.isAlive() && !kTeam.isVassal((TeamTypes)iI))
			{
				iTotalUnspentPoints += kTeam.getEspionagePointsAgainstTeam((TeamTypes)iI);
				iTeamCount++;
			}
		}
		iAverageUnspentPoints = iTotalUnspentPoints /= std::max(1, iTeamCount);
	}


	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kLoopPlayer.isAlive() && kLoopPlayer.getTeam() != getTeam() && !GET_TEAM(getTeam()).isVassal(kLoopPlayer.getTeam()))
		{
			int iTeamWeight = 1000;
			iTeamWeight *= kTeam.getEspionagePointsAgainstTeam(kLoopPlayer.getTeam());
			iTeamWeight /= std::max(1, iAverageUnspentPoints+iBaselinePoints);

			iTeamWeight *= 400 - kTeam.AI_getAttitudeWeight(kLoopPlayer.getTeam());
			iTeamWeight /= 500;
			/*  <advc.120> The "else" is the only functional change; don't want to
				communicate too loudly that we're preparing war. */
			if(kTeam.AI_getWarPlan(kLoopPlayer.getTeam()) != NO_WARPLAN)
				iTeamWeight *= 2;
			else if(kTeam.AI_isSneakAttackPreparing(kLoopPlayer.getTeam()))
				iTeamWeight *= 2; // </advc.120>
			iTeamWeight *= kOwner.AI_isMaliciousEspionageTarget((PlayerTypes)iPlayer) ? 3 : 2;
			iTeamWeight /= 2;
			// <advc.130v>
			if(GET_TEAM(kLoopPlayer.getTeam()).isCapitulated())
				iTeamWeight /= 2; // </advc.130v>
			if (iTeamWeight < 200 && GC.getGame().getSorenRandNum(10, "AI team target saving throw") != 0)
			{
				// low weight. Probably friendly attitude and below average points.
				// don't target this team.
				continue;
			}

			int iLoop;
			for (CvCity* pLoopCity = kLoopPlayer.firstCity(&iLoop); NULL != pLoopCity; pLoopCity = kLoopPlayer.nextCity(&iLoop))
			{
				if (pLoopCity == pSkipCity || !kOwner.AI_deduceCitySite(pLoopCity))
				{
					continue;
				}

				if (pLoopCity->area() == area() || canMoveAllTerrain())
				{
					CvPlot* pLoopPlot = pLoopCity->plot();
					if (AI_plotValid(pLoopPlot))
					{
						int iPathTurns;
						if (generatePath(pLoopPlot, 0, true, &iPathTurns, iMaxPath) && iPathTurns <= iMaxPath)
						{
							int iValue = AI_getEspionageTargetValue(pLoopPlot);

							iValue *= 5;
							iValue /= (5 + GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_ATTACK_SPY, getGroup()));
							iValue *= iTeamWeight;
							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pLoopPlot;
								pEndTurnPlot = getPathEndTurnPlot();
							}
						}
					}
				}
			}
		}
	}
	
	if (pBestPlot != NULL)
	{
		if (atPlot(pBestPlot))
		{
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY);
		}
		else
		{
			FAssert(pEndTurnPlot != NULL);
			getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX(), pEndTurnPlot->getY(), 0, false, false, MISSIONAI_ATTACK_SPY, pBestPlot);
		}
		return true;
	}
	
	return false;
}

bool CvUnitAI::AI_bonusOffenseSpy(int iRange)
{
	PROFILE_FUNC();

	CvPlot* pBestPlot = NULL;
	CvPlot* pEndTurnPlot = NULL;

	int iBestValue = 10;

	int iSearchRange = AI_searchRange(iRange);

	for (int iX = -iSearchRange; iX <= iSearchRange; iX++)
	{
		for (int iY = -iSearchRange; iY <= iSearchRange; iY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iX, iY);

			if (NULL != pLoopPlot && pLoopPlot->getNonObsoleteBonusType(getTeam(), true) != NO_BONUS)
			{
				if( pLoopPlot->isOwned() && pLoopPlot->getTeam() != getTeam() )
				{
					/* original code
					// Only move to plots where we will run missions
					if (GET_PLAYER(getOwnerINLINE()).AI_getAttitudeWeight(pLoopPlot->getOwner()) < (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 51 : 1)
						|| GET_TEAM(getTeam()).AI_getWarPlan(pLoopPlot->getTeam()) != NO_WARPLAN )
					{
						int iValue = AI_getEspionageTargetValue(pLoopPlot, iRange);
						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopPlot;								
						}
					}*/

					// K-Mod. I think this is only worthwhile when at war...
					//if (kOwner.AI_isMaliciousEspionageTarget(pLoopPlot->getOwner()))
					if (GET_TEAM(getTeam()).isAtWar(pLoopPlot->getTeam()))
					{
						int iPathTurns;
						if (generatePath(pLoopPlot, 0, true, &iPathTurns, iRange) && iPathTurns <= iRange)
						{
							int iValue = AI_getEspionageTargetValue(pLoopPlot);
							//iValue *= GET_TEAM(getTeam()).AI_getWarPlan(pLoopPlot->getTeam()) != NO_WARPLAN ? 3: 1;
							//iValue *= GET_TEAM(getTeam()).AI_isSneakAttackPreparing(pLoopPlot->getTeam()) ? 2 : 1;

							iValue *= 4;
							iValue /= (4 + GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_ATTACK_SPY, getGroup()));
							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pLoopPlot;
								pEndTurnPlot = getPathEndTurnPlot();
							}
						}

					}
					// K-Mod end
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		if (atPlot(pBestPlot))
		{	
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY);
			return true;
		}
		else
		{
			/* original code
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_ATTACK_SPY);			
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY); */
			// K-Mod
			FAssert(pEndTurnPlot != NULL);
			getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX(), pEndTurnPlot->getY(), 0, false, false, MISSIONAI_ATTACK_SPY, pBestPlot);
			// K-Mod end
			return true;
		}
	}

	return false;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

//Returns true if the spy performs espionage.
bool CvUnitAI::AI_espionageSpy()
{
	PROFILE_FUNC();

	if (!canEspionage(plot()))
	{
		return false;
	}
	
	EspionageMissionTypes eBestMission = NO_ESPIONAGEMISSION;
	CvPlot* pTargetPlot = NULL;
	PlayerTypes eTargetPlayer = NO_PLAYER;
	int iExtraData = -1;
	
	//eBestMission = GET_PLAYER(getOwnerINLINE()).AI_bestPlotEspionage(plot(), eTargetPlayer, pTargetPlot, iExtraData);
	eBestMission = AI_bestPlotEspionage(eTargetPlayer, pTargetPlot, iExtraData);
	if (NO_ESPIONAGEMISSION == eBestMission)
	{
		return false;
	}

	if (!GET_PLAYER(getOwnerINLINE()).canDoEspionageMission(eBestMission, eTargetPlayer, pTargetPlot, iExtraData, this))
	{
		return false;
	}
	
	/* original bts code
	if (!espionage(eBestMission, iExtraData))
	{
		return false;
	} */
	// K-Mod
	getGroup()->pushMission(MISSION_ESPIONAGE, eBestMission, iExtraData);
	
	return true;
}

// K-Mod edition. (This use to be a CvPlayerAI:: function.)
EspionageMissionTypes CvUnitAI::AI_bestPlotEspionage(PlayerTypes& eTargetPlayer, CvPlot*& pPlot, int& iData) const
{
	PROFILE_FUNC();

	CvPlot* pSpyPlot = plot();
	const CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());
	bool bBigEspionage = kPlayer.AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE);

	FAssert(pSpyPlot != NULL);

	int iSpyValue = 3*kPlayer.getProductionNeeded(getUnitType()) + 60;
	if (kPlayer.getCapitalCity() != NULL)
	{
		iSpyValue += stepDistance(getX(), getY(), kPlayer.getCapitalCity()->getX(), kPlayer.getCapitalCity()->getY()) / 2;
	}
	
	pPlot = NULL;
	iData = -1;

	EspionageMissionTypes eBestMission = NO_ESPIONAGEMISSION;
	int iBestValue = 0;

	int iEspionageRate = kPlayer.getCommerceRate(COMMERCE_ESPIONAGE);
	
	if (pSpyPlot->isOwned())
	{
		TeamTypes eTargetTeam = pSpyPlot->getTeam();

		if (eTargetTeam != getTeam())
		{
			int iEspPoints = GET_TEAM(getTeam()).getEspionagePointsAgainstTeam(eTargetTeam);

			// estimate risk cost of losing the spy while trying to escape
			int iBaseIntercept = 0;
			{
				int iTargetTotal = GET_TEAM(eTargetTeam).getEspionagePointsEver();
				int iOurTotal = GET_TEAM(getTeam()).getEspionagePointsEver();
				iBaseIntercept += (GC.getDefineINT("ESPIONAGE_INTERCEPT_SPENDING_MAX") * iTargetTotal) / std::max(1, iTargetTotal + iOurTotal);

				if (GET_TEAM(eTargetTeam).getCounterespionageModAgainstTeam(getTeam()) > 0)
					iBaseIntercept += GC.getDefineINT("ESPIONAGE_INTERCEPT_COUNTERESPIONAGE_MISSION");
			}
			int iEscapeCost = 2*iSpyValue * iBaseIntercept * (100+GC.getDefineINT("ESPIONAGE_SPY_MISSION_ESCAPE_MOD")) / 10000;

			// One espionage mission loop to rule them all.
			for (int iMission = 0; iMission < GC.getNumEspionageMissionInfos(); ++iMission)
			{
				CvEspionageMissionInfo& kMissionInfo = GC.getEspionageMissionInfo((EspionageMissionTypes)iMission);
				int iTestData = 1;
				if (kMissionInfo.getBuyTechCostFactor() > 0)
				{
					iTestData = GC.getNumTechInfos();
				}
				else if (kMissionInfo.getDestroyProjectCostFactor() > 0)
				{
					iTestData = GC.getNumProjectInfos();
				}
				else if (kMissionInfo.getDestroyBuildingCostFactor() > 0)
				{
					iTestData = GC.getNumBuildingInfos();
				}

				// estimate the risk cost of losing the spy.
				int iOverhead = iEscapeCost + iSpyValue * iBaseIntercept * (100 + kMissionInfo.getDifficultyMod()) / 10000;

				for (iTestData-- ; iTestData >= 0; iTestData--)
				{
					int iCost = kPlayer.getEspionageMissionCost((EspionageMissionTypes)iMission, pSpyPlot->getOwner(), pSpyPlot, iTestData, this);
					if (iCost < 0 || (iCost <= iEspPoints && !kPlayer.canDoEspionageMission((EspionageMissionTypes)iMission, pSpyPlot->getOwnerINLINE(), pSpyPlot, iTestData, this)))
						continue; // we can't do the mission, and cost is not the limiting factor.

					int iValue = kPlayer.AI_espionageVal(pSpyPlot->getOwner(), (EspionageMissionTypes)iMission, pSpyPlot, iTestData);
					iValue *= 80 + GC.getGameINLINE().getSorenRandNum(60, "AI best espionage mission");
					iValue /= 100;
					iValue -= iOverhead;
					iValue -= iCost * (bBigEspionage ? 2 : 1) * iCost / std::max(1, iCost + GET_TEAM(getTeam()).getEspionagePointsAgainstTeam(eTargetTeam));

					// If we can't do the mission yet, don't completely give up. It might be worth saving points for.
					if (!kPlayer.canDoEspionageMission((EspionageMissionTypes)iMission, pSpyPlot->getOwner(), pSpyPlot, iTestData, this))
					{
						// Is cost is the reason we can't do the mission?
						if (GET_TEAM(getTeam()).isHasTech((TechTypes)kMissionInfo.getTechPrereq()))
						{
							FAssert(iCost > iEspPoints); // (see condition at the top of the loop)
							// Scale the mission value based on how long we think it will take to get the points.

							int iTurns = (iCost - iEspPoints) / std::max(1, iEspionageRate);
							iTurns *= bBigEspionage? 1 : 2;
							// The number of turns is approximated (poorly) by assuming our entire esp rate is targeting eTargetTeam.
							iValue *= 3;
							iValue /= iTurns + 3;
							// eg, 1 turn left -> 3/4. 2 turns -> 3/5, 3 turns -> 3/6. Etc.
						}
						else
						{
							// Ok. Now it's time to give up. (Even if we're researching the prereq tech right now - too bad.)
							iValue = 0;
						}
					}

					// Block small missions when using "big espionage", unless the mission is really good value.
					if (bBigEspionage
						&& iValue < 50*kPlayer.getCurrentEra()*(kPlayer.getCurrentEra()+1) // 100, 300, 600, 1000, 1500, ...
						&& iValue < (kPlayer.AI_isDoStrategy(AI_STRATEGY_ESPIONAGE_ECONOMY) ? 4 : 2)*iCost)
					{
						iValue = 0;
					}

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						eBestMission = (EspionageMissionTypes)iMission;
						eTargetPlayer = pSpyPlot->getOwner();
						pPlot = pSpyPlot;
						iData = iTestData;
					}
				}
			}
			// K-Mod end
		}
	}
	if (gUnitLogLevel > 2 && eBestMission != NO_ESPIONAGEMISSION)
	{
		// The following assert isn't a problem or a bug. I just want to know when it happens, for testing purposes.
		//FAssertMsg(!kPlayer.AI_isDoStrategy(AI_STRATEGY_ESPIONAGE_ECONOMY) || GC.getEspionageMissionInfo(eBestMission).getBuyTechCostFactor() > 0 || GC.getEspionageMissionInfo(eBestMission).getDestroyProjectCostFactor() > 0, "Potentially wasteful AI use of espionage.");
		//
		logBBAI("      %S chooses %S as their best%s espionage mission (value: %d, cost: %d).", GET_PLAYER(getOwnerINLINE()).getCivilizationDescription(0), GC.getEspionageMissionInfo(eBestMission).getText(), bBigEspionage?" (big)":"", iBestValue, kPlayer.getEspionageMissionCost(eBestMission, eTargetPlayer, pPlot, iData, this));
	}

	return eBestMission;
}

bool CvUnitAI::AI_moveToStagingCity()
{
	PROFILE_FUNC();

	CvPlot* pStagingPlot = NULL;
	CvPlot* pEndTurnPlot = NULL;

	int iBestValue = 0;

	int iWarCount = 0;
	TeamTypes eTargetTeam = NO_TEAM;
	CvTeam& kTeam = GET_TEAM(getTeam());
	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		if ((iI != getTeam()) && GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (kTeam.AI_isSneakAttackPreparing((TeamTypes)iI))
			{
				eTargetTeam = (TeamTypes)iI;
				iWarCount++;
			}			
		}		
	}

	if (iWarCount > 1)
	{
		eTargetTeam = NO_TEAM;
	}

	int iLoop;
	for (CvCity* pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if ((pLoopCity->area() == area()) && AI_plotValid(pLoopCity->plot()))
		{
			// BBAI TODO: Need some knowledge of whether this is a good city to attack from ... only get that
			// indirectly from threat.
			int iValue = pLoopCity->AI_cityThreat();

			// Have attack stacks in assault areas move to coastal cities for faster loading
			if( (area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT) || (area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT_MASSING) )
			{
				CvArea* pWaterArea = pLoopCity->waterArea();
				if( pWaterArea != NULL && GET_TEAM(getTeam()).AI_isWaterAreaRelevant(pWaterArea) )
				{
					// BBAI TODO:  Need a better way to determine which cities should serve as invasion launch locations

					// Inertia so units don't just chase transports around the map
					iValue = iValue/2;
					if( pLoopCity->area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT )
					{
						// If in assault, transports may be at sea ... tend to stay where they left from
						// to speed reinforcement
						iValue += pLoopCity->plot()->plotCount(PUF_isAvailableUnitAITypeGroupie, UNITAI_ATTACK_CITY, -1, getOwnerINLINE());
					}

					// Attraction to cities which are serving as launch/pickup points
					iValue += 3*pLoopCity->plot()->plotCount(PUF_isUnitAIType, UNITAI_ASSAULT_SEA, -1, getOwnerINLINE());
					iValue += 2*pLoopCity->plot()->plotCount(PUF_isUnitAIType, UNITAI_ESCORT_SEA, -1, getOwnerINLINE());
					iValue += 5*GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_PICKUP);
				}
				else
				{
					iValue = iValue/8;
				}
			}

			if (iValue*200 > iBestValue)
			{
				int iPathTurns;
				if (generatePath(pLoopCity->plot(), MOVE_AVOID_ENEMY_WEIGHT_3, true, &iPathTurns))
				{
					iValue *= 1000;
					iValue /= (5 + iPathTurns);
					if ((pLoopCity->plot() != plot()) && pLoopCity->isVisible(eTargetTeam, false))
					{
						iValue /= 2;
					}

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pStagingPlot = pLoopCity->plot();
						pEndTurnPlot = getPathEndTurnPlot();
					}
				}
			}
		}
	}

	if (pStagingPlot != NULL)
	{
		if (atPlot(pStagingPlot))
		{
			getGroup()->pushMission(MISSION_SKIP);
			return true;
		}
		else
		{
			//getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
			getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX_INLINE(), pEndTurnPlot->getY_INLINE(), MOVE_AVOID_ENEMY_WEIGHT_3, false, false, MISSIONAI_GROUP, pStagingPlot); // K-Mod
			return true;
		}
	}

	return false;
}

/*
bool CvUnitAI::AI_seaRetreatFromCityDanger()
{
	if (plot()->isCity(true) && GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 2) > 0) //prioritize getting outta there
	{
		if (AI_anyAttack(2, 40))
		{
			return true;
		}

		if (AI_anyAttack(4, 50))
		{
			return true;
		}

		if (AI_retreatToCity())
		{
			return true;
		}

		if (AI_safety())
		{
			return true;
		}
	}
	return false;
}

bool CvUnitAI::AI_airRetreatFromCityDanger()
{
	if (plot()->isCity(true))
	{
		CvCity* pCity = plot()->getPlotCity();
		if (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 2) > 0 || (pCity != NULL && !pCity->AI_isDefended()))
		{
			if (AI_airOffensiveCity())
			{
				return true;
			}

			if (canAirDefend() && AI_airDefensiveCity())
			{
				return true;
			}
		}
	}
	return false;
}

bool CvUnitAI::AI_airAttackDamagedSkip()
{
	if (getDamage() == 0)
	{
		return false;
	}

	bool bSkip = (currHitPoints() * 100 / maxHitPoints() < 40);
	if (!bSkip)
	{
		int iSearchRange = airRange();
		bool bSkiesClear = true;
		for (int iDX = -iSearchRange; iDX <= iSearchRange && bSkiesClear; iDX++)
		{
			for (int iDY = -iSearchRange; iDY <= iSearchRange && bSkiesClear; iDY++)
			{
				CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);	
				if (pLoopPlot != NULL)
				{
					if (bestInterceptor(pLoopPlot) != NULL)
					{
						bSkiesClear = false;
						break;
					}
				}
			}
		}
		bSkip = !bSkiesClear;
	}

	if (bSkip)
	{
		getGroup()->pushMission(MISSION_SKIP);
		return true;
	}

	return false;
}
*/

// Returns true if a mission was pushed or we should wait for another unit to bombard...
bool CvUnitAI::AI_followBombard()
{
	/*CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pAdjacentPlot1;
	CvPlot* pAdjacentPlot2;
	int iI, iJ; */ // K-mod disabled

	if (canBombard(plot()))
	{
		getGroup()->pushMission(MISSION_BOMBARD);
		return true;
	}

	// K-Mod note: I've disabled the following code because it seems like a timewaster with very little benefit.
	// The code checks if we are standing next to a city, and then checks if we have any other readyToMove group
	// next to the same city which can bombard... if so, return true.
	// I suppose the point of the code is to block our units from issuing a follow-attack order if we still have
	// some bombarding to do. -- But in my opinion, such checks, if we want them, should be done by the attack code.
	/* original bts code
	if (getDomainType() == DOMAIN_LAND)
	{
		for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
		{
			pAdjacentPlot1 = plotDirection(getX_INLINE(), getY_INLINE(), ((DirectionTypes)iI));

			if (pAdjacentPlot1 != NULL)
			{
				if (pAdjacentPlot1->isCity())
				{
					if (AI_potentialEnemy(pAdjacentPlot1->getTeam(), pAdjacentPlot1))
					{
						for (iJ = 0; iJ < NUM_DIRECTION_TYPES; iJ++)
						{
							pAdjacentPlot2 = plotDirection(pAdjacentPlot1->getX_INLINE(), pAdjacentPlot1->getY_INLINE(), ((DirectionTypes)iJ));

							if (pAdjacentPlot2 != NULL)
							{
								pUnitNode = pAdjacentPlot2->headUnitNode();

								while (pUnitNode != NULL)
								{
									pLoopUnit = ::getUnit(pUnitNode->m_data);
									pUnitNode = pAdjacentPlot2->nextUnitNode(pUnitNode);

									if (pLoopUnit->getOwnerINLINE() == getOwnerINLINE())
									{
										if (pLoopUnit->canBombard(pAdjacentPlot2))
										{
											if (pLoopUnit->isGroupHead())
											{
												if (pLoopUnit->getGroup() != getGroup())
												{
													if (pLoopUnit->getGroup()->readyToMove())
													{
														return true;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	} */

	return false;
}


// Returns true if the unit has found a potential enemy...
bool CvUnitAI::AI_potentialEnemy(TeamTypes eTeam, const CvPlot* pPlot)
{
	PROFILE_FUNC();

	if (getGroup()->AI_isDeclareWar(pPlot))
	{
		return isPotentialEnemy(eTeam, pPlot);
	}
	else
	{
		return isEnemy(eTeam, pPlot);
	}
}

// <advc.033> Returns a pair (defenders,all)
std::pair<int,int> CvUnitAI::AI_countPiracyTargets(CvPlot const& p,
		bool bStopIfAnyTarget) const {

	std::pair<int,int> r = std::make_pair<int,int>(0, 0);
	if(!isAlwaysHostile(&p))
			// This is handled by searchRange
			//|| !p.isVisible(getTeam(), false))
		return r;
	for(int i = 0; i < p.getNumUnits(); i++) {
		CvUnit* up = p.getUnitByIndex(i);
		if(up == NULL) continue; CvUnit const& u = *up;
		if(u.isInvisible(getTeam(), false))
			continue;
		if(!GET_PLAYER(getOwnerINLINE()).AI_isPiracyTarget(u.getOwnerINLINE()))
			continue;
		r.second++;
		if(bStopIfAnyTarget)
			return r;
		if(u.canDefend())
			r.first++;
	}
	return r;
}

bool CvUnitAI::AI_isAnyPiracyTarget(CvPlot const& p) const {

	return AI_countPiracyTargets(p, true).second > 0;
}// </advc.033>

// Returns true if this plot needs some defense...
bool CvUnitAI::AI_defendPlot(CvPlot* pPlot)
{
	CvCity* pCity;

	if (!canDefend(pPlot))
	{
		return false;
	}

	pCity = pPlot->getPlotCity();

	if (pCity != NULL)
	{
		if (pCity->getOwnerINLINE() == getOwnerINLINE())
		{
			if (pCity->AI_isDanger())
			{
				return true;
			}
		}
	}
	else
	{
		if (pPlot->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(),
				// advc.001s: Want up to 1 defender per domain type
				NO_TEAM, PUF_isDomainType, getDomainType())
				<= ((atPlot(pPlot)) ? 1 : 0))
		{
			if (pPlot->plotCount(PUF_cannotDefend, -1, -1, getOwnerINLINE(),
					/*  advc.001s: A land unit can defend non-land units in a Fort,
						but not vice versa. */
					NO_TEAM, getDomainType() == DOMAIN_LAND ? NULL : PUF_isDomainType, getDomainType())
					> 0)
			{
				return true;
			}

//			if (pPlot->defenseModifier(getTeam(), false) >= 50 && pPlot->isRoute() && pPlot->getTeam() == getTeam())
//			{
//				return true;
//			}
		}
	}

	return false;
}

int CvUnitAI::AI_pillageValue(CvPlot* pPlot, int iBonusValueThreshold)
{
	FAssert(canPillage(pPlot) || canAirBombAt(plot(), pPlot->getX_INLINE(), pPlot->getY_INLINE()) || (getGroup()->getCargo() > 0));

	if (!pPlot->isOwned())
	{
		return 0;
	}

	int iValue = 0;

	int iBonusValue = 0;
	// eNonObsoleteBonus = pPlot->getNonObsoleteBonusType(pPlot->getTeam(), true);
	BonusTypes eNonObsoleteBonus = pPlot->isRevealed(getTeam(), false) ? pPlot->getNonObsoleteBonusType(pPlot->getTeam(), true) : NO_BONUS; // K-Mod
	if (eNonObsoleteBonus != NO_BONUS)
	{
		//iBonusValue = (GET_PLAYER(pPlot->getOwnerINLINE()).AI_bonusVal(eNonObsoleteBonus));
		iBonusValue = GET_PLAYER(pPlot->getOwnerINLINE()).AI_bonusVal(eNonObsoleteBonus, 0); // K-Mod
	}
	
	if (iBonusValueThreshold > 0)
	{
		if (eNonObsoleteBonus == NO_BONUS)
		{
			return 0;
		}
		else if (iBonusValue < iBonusValueThreshold)
		{
			return 0;
		}
	}

	if (getDomainType() != DOMAIN_AIR)
	{
		if (pPlot->//isRoute()
				// advc.001i:
				getRevealedRouteType(getTeam(), false) != NO_ROUTE)
		{
			iValue++;
			if (eNonObsoleteBonus != NO_BONUS)
			{
				//iValue += iBonusValue * 4;
				iValue += iBonusValue; // K-Mod. (many more iBonusValues will be added again later anyway)
			}

			for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
			{
				//pAdjacentPlot = plotDirection(getX_INLINE(), getY_INLINE(), ((DirectionTypes)iI));
				CvPlot* pAdjacentPlot = plotDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), (DirectionTypes)iI); // K-Mod, bugfix

				if (pAdjacentPlot != NULL && pAdjacentPlot->getTeam() == pPlot->getTeam())
				{
					if (pAdjacentPlot->isCity())
					{
						iValue += 10;
					}

					//if (!pAdjacentPlot->isRoute())
					// advc.001i:
					if(pAdjacentPlot->getRevealedRouteType(getTeam(), false) == NO_ROUTE)
					{
						if (!pAdjacentPlot->isWater() &&
								!pAdjacentPlot->isImpassable())
							iValue += 2;
					}
				}
			}
		}
	}

	/*if (pPlot->getImprovementDuration() > ((pPlot->isWater()) ? 20 : 5))
	{
		eImprovement = pPlot->getImprovementType();
	}
	else
	{
		eImprovement = pPlot->getRevealedImprovementType(getTeam(), false);
	}*/
	ImprovementTypes eImprovement = pPlot->getImprovementDuration() > 20 ?
			pPlot->getImprovementType() :
			pPlot->getRevealedImprovementType(getTeam(), false);
	if (eImprovement != NO_IMPROVEMENT)
	{
		if (pPlot->getWorkingCity() != NULL)
		{
			iValue += (pPlot->calculateImprovementYieldChange(eImprovement,
					YIELD_FOOD, pPlot->getOwnerINLINE()) * 5);
			iValue += (pPlot->calculateImprovementYieldChange(eImprovement,
					YIELD_PRODUCTION, pPlot->getOwnerINLINE()) * 4);
			iValue += (pPlot->calculateImprovementYieldChange(eImprovement,
					YIELD_COMMERCE, pPlot->getOwnerINLINE()) * 3);
		}

		if (getDomainType() != DOMAIN_AIR)
		{
			iValue += GC.getImprovementInfo(eImprovement).getPillageGold();
		}

		if (eNonObsoleteBonus != NO_BONUS)
		{
			//if (GC.getImprovementInfo(eImprovement).isImprovementBonusTrade(eNonObsoleteBonus))
			if (GET_PLAYER(pPlot->getOwnerINLINE()).doesImprovementConnectBonus(eImprovement, eNonObsoleteBonus)) // K-Mod
			{
				int iTempValue = iBonusValue * 4;

				if (pPlot->isConnectedToCapital() && (pPlot->getPlotGroupConnectedBonus(pPlot->getOwnerINLINE(), eNonObsoleteBonus) == 1))
				{
					iTempValue *= 2;
				}

				iValue += iTempValue;
			}
		}
	}

	return iValue;
}

// K-Mod.
// Return the value of the best nuke target in the range specified, and set pBestTarget to be the specific target plot.
// The return value is roughly in units of production.
int CvUnitAI::AI_nukeValue(CvPlot* pCenterPlot, int iSearchRange, CvPlot*& pBestTarget, int iCivilianTargetWeight) const
{
	PROFILE_FUNC();

	FAssert(pCenterPlot);

	int iMilitaryTargetWeight = 100;

	typedef std::map<CvPlot*, int> plotMap_t;
	plotMap_t affected_plot_values;
	int iBestValue = 0; // note: value is divided by 100 at the end
	pBestTarget = 0;

	for (int iX = -iSearchRange; iX <= iSearchRange; iX++)
	{
		for (int iY = -iSearchRange; iY <= iSearchRange; iY++)
		{
			CvPlot* pLoopTarget = plotXY(pCenterPlot, iX, iY);
			if (!pLoopTarget || !canNukeAt(plot(), pLoopTarget->getX_INLINE(), pLoopTarget->getY_INLINE()))
				continue;

			bool bValid = true;
			// Note: canNukeAt checks that we aren't hitting any 3rd party targets, so we don't have to worry about checking that elsewhere

			int iTargetValue = 0;

			for (int jX = -nukeRange(); bValid && jX <= nukeRange(); jX++)
			{
				for (int jY = -nukeRange(); bValid && jY <= nukeRange(); jY++)
				{
					CvPlot* pLoopPlot = plotXY(pLoopTarget, jX, jY);
					if (!pLoopPlot)
						continue;

					plotMap_t::iterator plot_it = affected_plot_values.find(pLoopPlot);
					if (plot_it != affected_plot_values.end())
					{
						if (plot_it->second == INT_MIN)
							bValid = false;
						else
							iTargetValue += plot_it->second;
						continue;
					}
					// plot evaluation:
					int iPlotValue = 0;

					// value for improvements / bonuses etc.
					if (bValid && pLoopPlot->isOwned())
					{
						bool bEnemy = isEnemy(pLoopPlot->getTeam(), pLoopPlot);
						FAssert(bEnemy || pLoopPlot->getTeam() == getTeam()); // it is owned, and we aren't allowed to nuke neutrals; so it is either enemy or ours.

						ImprovementTypes eImprovement = pLoopPlot->getRevealedImprovementType(getTeam(), false);
						BonusTypes eBonus = pLoopPlot->getNonObsoleteBonusType(getTeam());
						const CvPlayerAI& kPlotOwner = GET_PLAYER(pLoopPlot->getOwnerINLINE());

						if (eImprovement != NO_IMPROVEMENT)
						{
							const CvImprovementInfo& kImprovement = GC.getImprovementInfo(eImprovement);
							if (!kImprovement.isPermanent())
							{
								// arbitrary values, sorry.
								iPlotValue += 8 * (bEnemy ? iCivilianTargetWeight : -50);
								if (kImprovement.getImprovementPillage() != NO_IMPROVEMENT)
								{
									iPlotValue += (kImprovement.getImprovementUpgrade() == NO_IMPROVEMENT ? 32 : 16) * (bEnemy ? iCivilianTargetWeight : -50);
								}
							}
						}
						if (eBonus != NO_BONUS)
						{
							iPlotValue += 8 * (bEnemy ? iCivilianTargetWeight : -50);
							if (kPlotOwner.doesImprovementConnectBonus(eImprovement, eBonus))
							{
								// assume that valueable bonuses are military targets, because the enemy might be using the bonus to build weapons.
								iPlotValue += kPlotOwner.AI_bonusVal(eBonus, 0) * (bEnemy ? iMilitaryTargetWeight : -100);
							}
						}
					}

					// consider military units if the plot is visible. (todo: increase value of military units that we can chase down this turn, maybe.)
					if (bValid && pLoopPlot->isVisible(getTeam(), false))
					{
						CLLNode<IDInfo>* pUnitNode = pLoopPlot->headUnitNode();
						while (pUnitNode)
						{
							CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
							pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

							if (!pLoopUnit->isInvisible(getTeam(), false, true)) // I'm going to allow the AI to cheat here by seeing cargo units. (Human players can usually guess when a ship is loaded...)
							{
								if (pLoopUnit->isEnemy(getTeam(), pLoopPlot))
								{
									int iUnitValue = std::max(1, pLoopUnit->getUnitInfo().getProductionCost());
									// decrease the value for wounded units. (it might be nice to only do this if we are in a position to attack with ground forces...)
									int x = 100 * (pLoopUnit->maxHitPoints() - pLoopUnit->currHitPoints()) / std::max(1, pLoopUnit->maxHitPoints());
									iUnitValue -= iUnitValue*x*x/10000;
									iPlotValue += iMilitaryTargetWeight * iUnitValue;
								}
								else // non enemy unit
								{
									if (pLoopUnit->getTeam() == getTeam())
									{
										// nuking our own units... sometimes acceptable
										int x = pLoopUnit->getUnitInfo().getProductionCost();
										if (x > 0)
											iPlotValue -= iMilitaryTargetWeight * x;
										else
											bValid = false; // assume this is a special unit.
									}
									// dlph.7: Commented out
									/*else
									{
										FAssertMsg(false, "3rd party unit being considered for nuking.");
									}*/
								}
							}
						} // end unit loop
					} // end plot visible

					if (bValid && pLoopPlot->isCity() && pLoopPlot->getPlotCity()->isRevealed(getTeam(), false))
					{
						CvCity* pLoopCity = pLoopPlot->getPlotCity(); // I can imagine some cases where this actually isn't pCity. Can you?

						// it might even be one of our own cities, so be careful!
						if (!isEnemy(pLoopCity->getTeam(), pLoopPlot))
						{
							bValid = false;
						}
						else
						{
							// the values used here are quite arbitrary.
							iPlotValue += iCivilianTargetWeight * 2 * (pLoopCity->getCultureLevel() + 2) * pLoopCity->getPopulation();

							// note, it is possible to see which buildings the city has by looking at the map. This is not secret information.
							for (BuildingTypes i = (BuildingTypes)0; i < GC.getNumBuildingInfos(); i=(BuildingTypes)(i+1))
							{
								if (pLoopCity->getNumRealBuilding(i) > 0)
								{
									const CvBuildingInfo& kBuildingInfo = GC.getBuildingInfo(i);
									if (!kBuildingInfo.isNukeImmune())
										iPlotValue += iCivilianTargetWeight * pLoopCity->getNumRealBuilding(i) * std::max(0, kBuildingInfo.getProductionCost());
								}
							}

							// if we don't have vision of the city, just assume that there are at least a couple of defenders, and count that into our evaluation.
							if (!pLoopPlot->isVisible(getTeam(), false))
							{
								UnitTypes eBasicUnit = pLoopCity->getConscriptUnit();
								int iBasicCost = std::max(10, eBasicUnit != NO_UNIT ? GC.getUnitInfo(eBasicUnit).getProductionCost() : 0);
								int iExpectedUnits = 1 + ((1 + pLoopCity->getCultureLevel()) * pLoopCity->getPopulation() + pLoopCity->getHighestPopulation()/2) / std::max(1, pLoopCity->getHighestPopulation());

								iPlotValue += iMilitaryTargetWeight * iExpectedUnits * iBasicCost;
							}
						}
					}
					// end of plot evaluation
					affected_plot_values[pLoopPlot] = bValid ? iPlotValue : INT_MIN;
					iTargetValue += iPlotValue;
				}
			}

			if (bValid && iTargetValue > iBestValue)
			{
				pBestTarget = pLoopTarget;
				iBestValue = iTargetValue;
			}
		}
	}
	return iBestValue / 100;
}

// <advc.121>
int CvUnitAI::AI_connectBonusCost(CvPlot const& p, BuildTypes eBuild,
		int iNeededWorkersInArea) const {

	PROFILE_FUNC();
	// BtS code (originally in AI_improveBonus):
	/*int iValue = 10000;
	iValue /= (GC.getBuildInfo(eBuild).getTime() + 1);*/
	/*  <advc.121> The above means that the longer a build takes, the smaller is
		its (cost) value. So Forts are always preferred over cheaper Plantations etc.
		(There are similar issues in AI_irrigateTerritory and AI_fortifyTerritory,
		but w/o harmful consequences. CvCityAI::AI_getImprovementValue also divides
		by the build time, but that function gets it right as it computes a maximum.) */

	// Ad-hoc heuristic for Fort building:
	ImprovementTypes eImpr = (ImprovementTypes)GC.getBuildInfo(eBuild).getImprovement();
	CvImprovementInfo& kImpr = GC.getImprovementInfo(eImpr);
	int iDefenseValue = kImpr.getDefenseModifier();
	// The AI isn't going to station units on an island without cities
	if(p.area()->getCitiesPerPlayer(getOwnerINLINE()) <= 0 ||
			// <advc.035>
			p.isContestedByRival())
		iDefenseValue = 0; // </advc.035>
	/*  Prioritize Forts on tiles with high natural defense and on important
		resources that may later be guarded. */
	if(iDefenseValue > 0) {
		iDefenseValue += p.defenseModifier(getTeam(), true);
		BonusTypes eBonus = p.getBonusType(getTeam());
		if(eBonus == NO_BONUS) {
			FAssert(eBonus != NO_BONUS);
			return -1;
		}
		/*  bonusVal is usually just a single digit; small double digit if it's
			an important strategic resource. */
		iDefenseValue += GET_PLAYER(getOwnerINLINE()).AI_bonusVal(eBonus, 0);
		/*  (Not much of a point in checking p.isAdjacentToPlayer for all rivals.
			This function is only called for unworkable tiles, which are usually
			near a border.) */
	}
	int iCost = GC.getBuildInfo(eBuild).getTime();
	// No cost for leaving an existing improvement alone
	if(eImpr == p.getImprovementType())
		iCost = 0;
	// Time is more dear when workers are busy
	int iMultiplier = 2 * (std::max(0, iNeededWorkersInArea) + 1);
	// Halve the cost when there's nothing to do
	if(iNeededWorkersInArea == 0)
		iMultiplier = 1;
	// Account for having to replace a Fort later
	if(kImpr.isActsAsCity() && GET_PLAYER(getOwnerINLINE()).
		AI_isAdjacentCitySite(p, true))
			iMultiplier++;
	iCost *= iMultiplier;
	iCost /= 2;
	int const iDefenseWeight = 20;
	int r = iCost - iDefenseWeight * iDefenseValue;
	if(kImpr.isActsAsCity() && (p.isConnectSea() || 
			/*  If no cities in area, only a Fort will connect the bonus.
				(Unless workable, see advc.124, but p isn't workable.) */
			GET_TEAM(getTeam()).countNumCitiesByArea(p.area()) <= 0)) {
		/*  That means, eBuild is a very good build. But note that the build time
			component of iCost is on a times-100 scale, so -1000 doesn't guarantee
			that a Fort is built. */
		r -= 1000;
	}
	return r;
	// XXX feature production???
	/*  As for the Firaxis comment above:
		Feature production is handled elsewhere (see advc.117). That said, once
		a Fort is built, I'm not sure if Workers will still consider chopping.
		If they do, then the defense bonus from a Forest shouldn't be counted fully
		here. (Tbd., but not important.) */
}

/*  Function needed for advc.040. In part copied from AI_connectBonus.
	I guess taking into account safe automation makes this an AI function. */
bool CvUnitAI::AI_canConnectBonus(CvPlot const& p, BuildTypes eBuild) const
{	// Some old BtS code
	//if (GC.getBuildInfo(eBuild).getImprovement() != NO_IMPROVEMENT)
	//if (GC.getImprovementInfo((ImprovementTypes) GC.getBuildInfo(eBuild).getImprovement()).isImprovementBonusTrade(eNonObsoleteBonus) || (!pLoopPlot->isCityRadius() && GC.getImprovementInfo((ImprovementTypes) GC.getBuildInfo(eBuild).getImprovement()).isActsAsCity()))

	CvPlayer const& kOwner = GET_PLAYER(getOwnerINLINE());
	BonusTypes eBonus = p.getNonObsoleteBonusType(kOwner.getTeam());
	if(eBonus == NO_BONUS)
		return false;
	ImprovementTypes eLoopImpr = (ImprovementTypes)GC.getBuildInfo(eBuild).getImprovement();
	if(eLoopImpr == NO_IMPROVEMENT ||
			!kOwner.doesImprovementConnectBonus(eLoopImpr, eBonus)) // K-Mod
		return false;
	/*  Important for AI_connectBonus that true is returned if the current improvement
		already connects the resource */
	if(p.getImprovementType() == eLoopImpr)
		return true;
	if(!canBuild(&p, eBuild))
		return false;
	if(p.getFeatureType() != NO_FEATURE &&
			GC.getBuildInfo(eBuild).isFeatureRemove(p.getFeatureType()) &&
			kOwner.isOption(PLAYEROPTION_LEAVE_FORESTS))
		return false;
	return true;
} // </advc.121>


int CvUnitAI::AI_searchRange(int iRange)
{
	if (iRange == 0)
	{
		return 0;
	}
	if (flatMovementCost() || (getDomainType() == DOMAIN_SEA))
	{
		return (iRange * baseMoves());
	}
	else
	{
		return ((iRange + 1) * (baseMoves() + 1));
	}
}


// XXX at some point test the game with and without this function...
bool CvUnitAI::AI_plotValid(CvPlot* pPlot)
{	/*  advc.003o: This function is called extremely often and the total time
		spent is significant, but profiling it over and over won't help. */
	//PROFILE_FUNC();
	if (m_pUnitInfo->isNoRevealMap() && willRevealByMove(pPlot))
	{
		return false;
	}

	switch (getDomainType())
	{
	case DOMAIN_SEA:
		if (pPlot->isWater() || canMoveAllTerrain())
		{
			return true;
		}
		else if (pPlot->isFriendlyCity(*this, true) && pPlot->isCoastalLand())
		{
			return true;
		}
		break;

	case DOMAIN_AIR:
		FAssert(false);
		break;

	case DOMAIN_LAND:
		if (//pPlot->getArea() == getArea()
			/*  advc.030: Replacing the above. Wouldn't hurt to do that for
				DOMAIN_SEA as well, but no need. For DOMAIN_LAND, the change is
				only important if a land unit is given canMoveImpassable. */
				canEnterArea(*pPlot->area())
				|| canMoveAllTerrain())
		{
			return true;
		}
		break;

	case DOMAIN_IMMOBILE:
		FAssert(false);
		break;

	default:
		FAssert(false);
		break;
	}

	return false;
}

#if 0
int CvUnitAI::AI_finalOddsThreshold(CvPlot* pPlot, int iOddsThreshold)
{
// K-Mod note: This functions is trash.
// This is what makes the AI suicide huge stacks of units against a small group of powerful defenders.
// Imagine 2 units with defence and drill promotions, standing in a hills-forest fort...

// So... I intend to change the AI to never use this function.
	PROFILE_FUNC();

	CvCity* pCity;

	int iFinalOddsThreshold;

	iFinalOddsThreshold = iOddsThreshold;

	pCity = pPlot->getPlotCity();

	if (pCity != NULL)
	{
		if (pCity->getDefenseDamage() < ((GC.getMAX_CITY_DEFENSE_DAMAGE() * 3) / 4))
		{
			iFinalOddsThreshold += std::max(0, (pCity->getDefenseDamage() - pCity->getLastDefenseDamage() - (GC.getDefineINT("CITY_DEFENSE_DAMAGE_HEAL_RATE") * 2)));
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/29/10                                jdog5000      */
/*                                                                                              */
/* War tactics AI                                                                               */
/************************************************************************************************/
/* original bts code
	if (pPlot->getNumVisiblePotentialEnemyDefenders(this) == 1)
	{
		if (pCity != NULL)
		{
			iFinalOddsThreshold *= 2;
			iFinalOddsThreshold /= 3;
		}
		else
		{
			iFinalOddsThreshold *= 7;
			iFinalOddsThreshold /= 8;
		}
	}

	if ((getDomainType() == DOMAIN_SEA) && !getGroup()->hasCargo())
	{
		iFinalOddsThreshold *= 3;
		iFinalOddsThreshold /= 2 + getGroup()->getNumUnits();
	}
	else
	{
		iFinalOddsThreshold *= 6;
		iFinalOddsThreshold /= (3 + GET_PLAYER(getOwnerINLINE()).AI_adjacentPotentialAttackers(pPlot, true) + ((stepDistance(getX_INLINE(), getY_INLINE(), pPlot->getX_INLINE(), pPlot->getY_INLINE()) > 1) ? 1 : 0) + ((AI_isCityAIType()) ? 2 : 0));
	}
*/
	int iDefenders = pPlot->getNumVisiblePotentialEnemyDefenders(this);

	// More aggressive if only one enemy defending city
	if (iDefenders == 1 && pCity != NULL)
	{
		iFinalOddsThreshold *= 2;
		iFinalOddsThreshold /= 3;
	}

	if ((getDomainType() == DOMAIN_SEA) && !getGroup()->hasCargo())
	{
		iFinalOddsThreshold *= 3 + (iDefenders/2);
		iFinalOddsThreshold /= 2 + getGroup()->getNumUnits();
	}
	else
	{
		iFinalOddsThreshold *= 6 + (iDefenders/((pCity != NULL) ? 1 : 2));
		int iDivisor = 3;
		iDivisor += GET_PLAYER(getOwnerINLINE()).AI_adjacentPotentialAttackers(pPlot, true);
		iDivisor += ((stepDistance(getX_INLINE(), getY_INLINE(), pPlot->getX_INLINE(), pPlot->getY_INLINE()) > 1) ? getGroup()->getNumUnits() : 0);
		iDivisor += (AI_isCityAIType() ? 2 : 0);
		iFinalOddsThreshold /= iDivisor;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	return range(iFinalOddsThreshold, 1, 99);
}
#endif

// K-Mod. A new odds-adjusting function to replace the seriously flawed CvUnitAI::AI_finalOddsThreshold function.
// (note: I would like to put this in CvSelectionGroupAI ... but - well - I don't need to say it, right?
int CvUnitAI::AI_getWeightedOdds(CvPlot* pPlot, bool bPotentialEnemy)
{
	PROFILE_FUNC();
	int iOdds;
	CvUnit* pAttacker = getGroup()->AI_getBestGroupAttacker(pPlot, bPotentialEnemy, iOdds);
	if (!pAttacker)
		return 0;
	CvUnit* pDefender = pPlot->getBestDefender(NO_PLAYER, getOwnerINLINE(), pAttacker, !bPotentialEnemy, bPotentialEnemy);

	if (!pDefender)
		return 100;

	/* <advc.114b>: We shouldn't adjust the odds based on an optimistic estimate
	   (increased by AttackOddsChange). It leads to Warriors attacking Tanks
	   because the optimistic odds are considerably above zero and the
	   difference in production cost is great. I'm subtracting the AttackOddsChange
	   temporarily; adding them back in after the adjustments are done.
	   (A more elaborate fix would avoid adding them in the first place.) */
	int attackOddsChange = GET_PLAYER(getOwner()).AI_getAttackOddsChange();
	iOdds -= attackOddsChange;
	/* Require a stack of at least 3 if actual odds are below 1%. Should
	   matter mostly for barbarians, hence only this primitive condition
	   (not checking if the other units could actually attack etc.). */
	if(iOdds == 0 && getGroup()->getNumUnits() < 3)
		return 0;
	// </advc.114b>
	int iAdjustedOdds = iOdds;

	// adjust the values based on the relative production cost of the units.
	{
		int iOurCost = pAttacker->getUnitInfo().getProductionCost();
		int iTheirCost = pDefender->getUnitInfo().getProductionCost();
		if (iOurCost > 0 && iTheirCost > 0 && iOurCost != iTheirCost)
		{
			//iAdjustedOdds += iOdds * (100 - iOdds) * 2 * iTheirCost / (iOurCost + iTheirCost) / 100;
			//iAdjustedOdds -= iOdds * (100 - iOdds) * 2 * iOurCost / (iOurCost + iTheirCost) / 100;
			int x = iOdds * (100 - iOdds) * 2 / (iOurCost + iTheirCost + 20);
			iAdjustedOdds += x * (iTheirCost - iOurCost) / 100;
		}
	}
	// similarly, adjust based on the LFB value (slightly diluted)
	{
		int iDilution = GC.getLFBBasedOnExperience() + GC.getLFBBasedOnHealer() + ROUND_DIVIDE(10 * GC.getLFBBasedOnExperience() * (GC.getGameINLINE().getCurrentEra() - GC.getGameINLINE().getStartEra() + 1), std::max(1, GC.getNumEraInfos() - GC.getGameINLINE().getStartEra()));
		int iOurValue = pAttacker->LFBgetRelativeValueRating() + iDilution;
		int iTheirValue = pDefender->LFBgetRelativeValueRating() + iDilution;

		int x = iOdds * (100 - iOdds) * 2 / std::max(1, iOurValue + iTheirValue);
		iAdjustedOdds += x * (iTheirValue - iOurValue) / 100;
	}

	// adjust down if the enemy is on a defensive tile - we'd prefer to attack them on open ground.
	if (!pDefender->noDefensiveBonus())
	{
		iAdjustedOdds -= (100 - iOdds) * pPlot->defenseModifier(pDefender->getTeam(), false
			, getTeam() // advc.012
			) / (getDomainType() == DOMAIN_SEA ? 100 : 300);
	}

	// adjust the odds up if the enemy is wounded. We want to attack them now before they heal.
	iAdjustedOdds += iOdds * (100 - iOdds) * pDefender->getDamage() / (100 * pDefender->maxHitPoints());
	// adjust the odds down if our attacker is wounded - but only if healing is viable.
	if (pAttacker->isHurt() && pAttacker->healRate(pAttacker->plot()) > 10)
		iAdjustedOdds -= iOdds * (100 - iOdds) * pAttacker->getDamage() / (100 * pAttacker->maxHitPoints());

	// We're extra keen to take cites when we can...
	if (pPlot->isCity() && pPlot->getNumVisiblePotentialEnemyDefenders(pAttacker) == 1)
	{
		iAdjustedOdds += (100 - iOdds) / 3;
	}	

	// one more thing... unfortunately, the sea AI isn't evolved enough to do without something as painful as this...
	if (getDomainType() == DOMAIN_SEA && !getGroup()->hasCargo())
	{
		// I'm sorry about this. I really am. I'll try to make it better one day...
		int iDefenders = pPlot->getNumVisiblePotentialEnemyDefenders(pAttacker);
		iAdjustedOdds *= 2 + getGroup()->getNumUnits();
		iAdjustedOdds /= 3 + std::min(iDefenders/2, getGroup()->getNumUnits());
	}

	iAdjustedOdds += attackOddsChange; // advc.114b
	return range(iAdjustedOdds, 1, 99);
}

// A simple hash of the unit's birthmark.
// This is to be used for getting a 'random' number which depends on the unit but which does not vary from turn to turn.
unsigned CvUnitAI::AI_unitBirthmarkHash(int iExtra) const
{
	unsigned iHash = AI_getBirthmark() + iExtra;
	iHash *= 2654435761; // golden ratio of 2^32;
	return iHash;
}

// another 'random' hash, but which depends on a particular plot
unsigned CvUnitAI::AI_unitPlotHash(const CvPlot* pPlot, int iExtra) const
{
	return AI_unitBirthmarkHash(GC.getMapINLINE().plotNumINLINE(pPlot->getX_INLINE(), pPlot->getY_INLINE()) + iExtra);
}
// K-Mod end

int CvUnitAI::AI_stackOfDoomExtra() const
{
	//return ((AI_getBirthmark() % (1 + GET_PLAYER(getOwnerINLINE()).getCurrentEra())) + 4);
	// K-Mod
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	int iFlavourExtra = kOwner.AI_getFlavorValue(FLAVOR_MILITARY)/2 +
			(kOwner.AI_getFlavorValue(FLAVOR_MILITARY) > 0 ?
			4 : 2); // <advc.104p> was 8:4
	/*  Would be best to use the era of the target, but stacks aren't formed
		against a particular target. Game era is still better than using
		our era: If we're more advanced than our rivals, it doesn't mean that
		we need larger stacks than theirs. */
	int iEra = GC.getGameINLINE().getCurrentEra(); // </advc.104p>
	// 4 base. then rand between 0 and ... (1 or 2 + iEra + flavour * era ratio)
	// <advc.104p> Put that era ratio in a variable and round modulus to nearest
	double eraRatio = (iEra+1.0)/std::max(1, GC.getNumEraInfos());
	int modulus = ::round( // </advc.104p>
			((kOwner.AI_isDoStrategy(AI_STRATEGY_CRUSH) ? 2 : 1) +
			eraRatio * iFlavourExtra +
			// <advc.104p> Half of iEra factored into the non-random portion
			iEra/2));
	int r = (iEra+1)/2 + // </advc.104p>
			4 + (AI_getBirthmark() % modulus);
	// K-Mod end
	// <advc.104p>
	double mult = 1;
	/*  Bigger AI stacks on higher difficulty (b/c units are cheaper to train then,
		not b/c bigger stacks are generally smarter) */
	if(!kOwner.isHuman() && !isBarbarian()) {
		// e.g. 85% on Emperor, 80% on Immortal, 60% on Deity (raise that to 75%)
		double trainMod = GC.getHandicapInfo(GC.getGameINLINE().
				getHandicapType()).getAITrainPercent() / 100.0;
		mult /= std::max(0.75, trainMod);
	}
	CvTeamAI const& ourTeam = GET_TEAM(kOwner.getTeam());
	// A little extra for naval assault
	if(area()->getAreaAIType(ourTeam.getID()) == AREAAI_ASSAULT)
		mult += 0.225;
	if(ourTeam.getWarPlanCount(WARPLAN_TOTAL) <= 0)
		mult *= 0.85;
	r = ::round(mult * r); // </advc.104p>
	return r;
}

// This function has been significantly modified for K-Mod
bool CvUnitAI::AI_stackAttackCity(int iPowerThreshold)
{
    PROFILE_FUNC();

	FAssert(canMove());

	CvPlot* pCityPlot = NULL;
	int iSearchRange = 1;
	for (int iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (int iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL && AI_plotValid(pLoopPlot))
			{
				//if (pLoopPlot->isCity() || (pLoopPlot->isCity(true) && pLoopPlot->isVisibleEnemyUnit(this)))
				if (pLoopPlot->isCity()) // K-Mod. We want to attack a city. We don't care about guarded forts!
				{
					if (AI_potentialEnemy(pLoopPlot->getTeam(), pLoopPlot))
					{
						//if (!atPlot(pLoopPlot) && ((bFollow) ? canMoveInto(pLoopPlot, /*bAttack*/ true, /*bDeclareWar*/ true) : (generatePath(pLoopPlot, 0, true, &iPathTurns) && (iPathTurns <= iRange))))
						if (!atPlot(pLoopPlot) && getGroup()->canMoveOrAttackInto(pLoopPlot, true, true))
						{
							// K-Mod
							if (iPowerThreshold < 0)
							{
								// basic threshold calculation.
								CvCity* pCity = pLoopPlot->getPlotCity();
								// This automatic threshold calculation is used by AI_follow; and so we can't assume this unit is the head of the group.
								// ... But I think it's fair to assume that if our group has any bombard, it the head unit will have it.
								if (getGroup()->getHeadUnit()->bombardRate() > 0)
								{
									// if we can bombard, then we should do a rough calculation to give us a 'skip bombard' threshold.
									iPowerThreshold = ((GC.getMAX_CITY_DEFENSE_DAMAGE()-pCity->getDefenseDamage()) * GC.getBBAI_SKIP_BOMBARD_BASE_STACK_RATIO() + pCity->getDefenseDamage() * GC.getBBAI_SKIP_BOMBARD_MIN_STACK_RATIO()) / std::max(1, GC.getMAX_CITY_DEFENSE_DAMAGE());
								}
								else
								{
									// if we have no bombard ability - just use the minimum threshold
									iPowerThreshold = GC.getBBAI_SKIP_BOMBARD_MIN_STACK_RATIO();
								}
								FAssert(iPowerThreshold >= GC.getBBAI_ATTACK_CITY_STACK_RATIO());
							}
							// K-Mod end

							if (getGroup()->AI_compareStacks(pLoopPlot, true) >= iPowerThreshold)
							{
								pCityPlot = pLoopPlot;
							}
						}
					}
					break; // there can only be one.
				}
			}
		}
	}

	if (pCityPlot != NULL)
	{
		if( gUnitLogLevel >= 1 && pCityPlot->getPlotCity() != NULL )
		{
			logBBAI("    Stack for player %d (%S) decides to attack city %S with stack ratio %d", getOwner(), GET_PLAYER(getOwnerINLINE()).getCivilizationDescription(0), pCityPlot->getPlotCity()->getName(0).GetCString(), getGroup()->AI_compareStacks(pCityPlot, true) );
			logBBAI("    City %S has defense modifier %d, %d with ignore building", pCityPlot->getPlotCity()->getName(0).GetCString(), pCityPlot->getPlotCity()->getDefenseModifier(false), pCityPlot->getPlotCity()->getDefenseModifier(true) );
		}

		FAssert(!atPlot(pCityPlot));
		if(AI_considerDOW(pCityPlot)) {	// <advc.163>
			if(!canMove())
				return true;
		} // </advc.163>
		getGroup()->pushMission(MISSION_MOVE_TO, pCityPlot->getX_INLINE(), pCityPlot->getY_INLINE(), pCityPlot->isVisibleEnemyDefender(this) ? MOVE_DIRECT_ATTACK : 0);
		return true;
	}

	return false;
}

bool CvUnitAI::AI_moveIntoCity(int iRange)
{
    PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iSearchRange = iRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	FAssert(canMove());

	iBestValue = 0;
	pBestPlot = NULL;
	
	if (plot()->isCity())
	{
	    return false;
	}

	iSearchRange = AI_searchRange(iRange);

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);
			
			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot) && (!isEnemy(pLoopPlot->getTeam(), pLoopPlot)))
				{
					if (pLoopPlot->isCity() || (pLoopPlot->isCity(true)))
					{
                        if (canMoveInto(pLoopPlot, false) && (generatePath(pLoopPlot, 0, true, &iPathTurns, 1) && (iPathTurns <= 1)))
                        {
                            iValue = 1;
                            if (pLoopPlot->getPlotCity() != NULL)
                            {
                                 iValue += pLoopPlot->getPlotCity()->getPopulation();                                
                            }
                            
                            if (iValue > iBestValue)
                            {
                                iBestValue = iValue;
                                pBestPlot = getPathEndTurnPlot();
                                FAssert(!atPlot(pBestPlot));
                            }
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}

//bolsters the culture of the weakest city.
//returns true if a mission is pushed.
/* disabled by K-Mod. (not currently used)
bool CvUnitAI::AI_artistCultureVictoryMove()
{
    bool bGreatWork = false;
    bool bJoin = true;

    if (!(GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1)))
    {
        return false;        
    }
    
    if (GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3))
    {
        //Great Work
        bGreatWork = true;
    }

	int iCultureCitiesNeeded = GC.getGameINLINE().culturalVictoryNumCultureCities();
	FAssertMsg(iCultureCitiesNeeded > 0, "CultureVictory Strategy should not be true");

	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	CvCity* pBestCity;
	SpecialistTypes eBestSpecialist;
	int iLoop, iValue, iBestValue;

	pBestPlot = NULL;
	eBestSpecialist = NO_SPECIALIST;

	pBestCity = NULL;
	
	iBestValue = 0;
	iLoop = 0;
	
	int iTargetCultureRank = iCultureCitiesNeeded;
	while (iTargetCultureRank > 0 && pBestCity == NULL)
	{
		for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
		{
			// BBAI efficiency: check same area
			if ((pLoopCity->area() == area()) && AI_plotValid(pLoopCity->plot()))
			{
				// instead of commerce rate rank should use the culture on tile...
				if (pLoopCity->findCommerceRateRank(COMMERCE_CULTURE) == iTargetCultureRank)
				{
					// if the city is a fledgling, probably building culture, try next higher city
					if (pLoopCity->getCultureLevel() < 2)
					{
						break;
					}
					
					// if we cannot path there, try the next higher culture city
					if (!generatePath(pLoopCity->plot(), 0, true))
					{
						break;
					}

					pBestCity = pLoopCity;
					pBestPlot = pLoopCity->plot();
					if (bGreatWork)
					{
						if (canGreatWork(pBestPlot))
						{
							break;
						}
					}

					for (int iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
					{
						if (canJoin(pBestPlot, ((SpecialistTypes)iI)))
						{
							//iValue = pLoopCity->AI_specialistValue(((SpecialistTypes)iI), pLoopCity->AI_avoidGrowth(), false);
							iValue = pLoopCity->AI_permanentSpecialistValue((SpecialistTypes)iI); // K-Mod

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								eBestSpecialist = ((SpecialistTypes)iI);
							}
						}
					}

					if (eBestSpecialist == NO_SPECIALIST)
					{
						bJoin = false;
						if (canGreatWork(pBestPlot))
						{
							bGreatWork = true; 
							break;                        
						}
						bGreatWork = false;
					}
					break;
				}
			}
		}
	
		iTargetCultureRank--;
	}


	FAssertMsg(bGreatWork || bJoin, "This wasn't a Great Artist");
	
	if (pBestCity == NULL)
	{
	    //should try to airlift there...
	    return false;
	}


    if (atPlot(pBestPlot))
    {
        if (bGreatWork)
        {
            getGroup()->pushMission(MISSION_GREAT_WORK);
            return true;
        }
        if (bJoin)
        {
            getGroup()->pushMission(MISSION_JOIN, eBestSpecialist);
            return true;
        }
        FAssert(false);
        return false;		    
    }
    else
    {
        FAssert(!atPlot(pBestPlot));
        getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
        return true;
    }
} */

// advc.003j (comment): This is never called (AI Worker stealing)
bool CvUnitAI::AI_poach()
{
	CvPlot* pLoopPlot;
	int iX, iY;
	
	int iBestPoachValue = 0;
	CvPlot* pBestPoachPlot = NULL;
	TeamTypes eBestPoachTeam = NO_TEAM;
	
	if (!GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI))
	{
		return false;
	}
	
	if (GET_TEAM(getTeam()).getNumMembers() > 1)
	{
		return false;
	}
	
	int iNoPoachRoll = GET_PLAYER(getOwnerINLINE()).AI_totalUnitAIs(UNITAI_WORKER);
	iNoPoachRoll += GET_PLAYER(getOwnerINLINE()).getNumCities();
	iNoPoachRoll = std::max(0, (iNoPoachRoll - 1) / 2);
	if (GC.getGameINLINE().getSorenRandNum(iNoPoachRoll, "AI Poach") > 0)
	{
		return false;
	}

	if(GET_PLAYER(getOwnerINLINE()).AI_isFocusWar(area())) // advc.105
	//if (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0)
	{
		return false;
	}
	
	FAssert(canAttack());
	
	
	
	int iRange = 1;
	//Look for a unit which is non-combat
	//and has a capture unit type
	for (iX = -iRange; iX <= iRange; iX++)
	{
		for (iY = -iRange; iY <= iRange; iY++)
		{
			if (iX != 0 && iY != 0)
			{
				pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iX, iY);
				if ((pLoopPlot != NULL) && (pLoopPlot->getTeam() != getTeam()) && pLoopPlot->isVisible(getTeam(), false))
				{
					int iPoachCount = 0;
					int iDefenderCount = 0;
					CvUnit* pPoachUnit = NULL;
					CLLNode<IDInfo>* pUnitNode = pLoopPlot->headUnitNode();
					while (pUnitNode != NULL)
					{
						CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);
						if ((pLoopUnit->getTeam() != getTeam())
							&& GET_TEAM(getTeam()).canDeclareWar(pLoopUnit->getTeam()))
						{
							if (!pLoopUnit->canDefend())
							{
								if (pLoopUnit->getCaptureUnitType(getCivilizationType()) != NO_UNIT)
								{
									iPoachCount++;
									pPoachUnit = pLoopUnit;						
								}
							}
							else
							{
								iDefenderCount++;
							}
						}
					}
					
					if (pPoachUnit != NULL)
					{
						if (iDefenderCount == 0)
						{
							int iValue = iPoachCount * 100;
							iValue -= iNoPoachRoll * 25;
							if (iValue > iBestPoachValue)
							{
								iBestPoachValue = iValue;
								pBestPoachPlot = pLoopPlot;
								eBestPoachTeam = pPoachUnit->getTeam();
							}
						}
					}
				}
			}
		}
	}
	
	if (pBestPoachPlot != NULL)
	{
		//No war roll.
		if (!GET_TEAM(getTeam()).AI_performNoWarRolls(eBestPoachTeam))
		{
			GET_TEAM(getTeam()).declareWar(eBestPoachTeam, true, WARPLAN_LIMITED);
			
			FAssert(!atPlot(pBestPoachPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPoachPlot->getX_INLINE(), pBestPoachPlot->getY_INLINE(), MOVE_DIRECT_ATTACK);
			return true;
		}
		
	}
	
	return false;
}

// K-Mod. I've rewritten most of this function.
bool CvUnitAI::AI_choke(int iRange, bool bDefensive, int iFlags)
{
	PROFILE_FUNC();

	int iPercentDefensive;
	{
		int iDefCount = 0;
		CLLNode<IDInfo>* pUnitNode = getGroup()->headUnitNode();
		CvUnit* pLoopUnit = NULL;

		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			iDefCount += pLoopUnit->noDefensiveBonus() ? 0 : 1;

			pUnitNode = getGroup()->nextUnitNode(pUnitNode);
		}
		iPercentDefensive = 100 * iDefCount / getGroup()->getNumUnits();
	}

	CvPlot* pBestPlot = 0;
	CvPlot* pEndTurnPlot = 0;
	int iBestValue = 0;
	for (int iX = -iRange; iX <= iRange; iX++)
	{
		for (int iY = -iRange; iY <= iRange; iY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iX, iY);
			if (pLoopPlot && isEnemy(pLoopPlot->getTeam()) && !pLoopPlot->isVisibleEnemyUnit(this))
			{
				int iPathTurns;
				if (pLoopPlot->getWorkingCity() && generatePath(pLoopPlot, iFlags, true, &iPathTurns, iRange))
				{
					FAssert(pLoopPlot->getWorkingCity()->getTeam() == pLoopPlot->getTeam());
					//int iValue = (bDefensive ? pLoopPlot->defenseModifier(getTeam(), false) : -15);
					int iValue = bDefensive ?
						AI_plotDefense(pLoopPlot) // advc.012
						//pLoopPlot->defenseModifier(getTeam(), false) // K-Mod
						- 15 : 0;
					if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
					{
						iValue = GET_PLAYER(pLoopPlot->getOwnerINLINE()).AI_bonusVal(pLoopPlot->getBonusType(), 0);
					}
					
					iValue += pLoopPlot->getYield(YIELD_PRODUCTION) * 9; // was 10
					iValue += pLoopPlot->getYield(YIELD_FOOD) * 12; // was 10
					iValue += pLoopPlot->getYield(YIELD_COMMERCE) * 5;

					if (atPlot(pLoopPlot) && canPillage(pLoopPlot))
					{
						iValue += AI_pillageValue(pLoopPlot, 0) / (bDefensive ? 2 : 1);
					}

					if (iValue > 0)
					{
						iValue *= (bDefensive ? 25 : 50) + iPercentDefensive *
							AI_plotDefense(pLoopPlot) // advc.012
							//pLoopPlot->defenseModifier(getTeam(), false)
							/ 100;

						if (bDefensive)
						{
							// for defensive, we care a lot about path turns
							iValue *= 10;
							iValue /= std::max(1, iPathTurns);
						}
						else
						{
							// otherwise we just want to block as many tiles as possible
							iValue *= 10;
							iValue /= std::max(1, pLoopPlot->getNumDefenders(getOwnerINLINE()) + (pLoopPlot == plot() ? 0 : getGroup()->getNumUnits()));
						}

						if (iValue > iBestValue)
						{
							pBestPlot = pLoopPlot;
							pEndTurnPlot = getPathEndTurnPlot();
							iBestValue = iValue;
						}
					}
				}
			}
		}
	}
	if (pBestPlot)
	{
		FAssert(pBestPlot->getWorkingCity());
		CvPlot* pChokedCityPlot = pBestPlot->getWorkingCity()->plot();
		if (atPlot(pBestPlot))
		{
			FAssert(atPlot(pEndTurnPlot));
			if (canPillage(plot()))
				getGroup()->pushMission(MISSION_PILLAGE, -1, -1, iFlags, false, false, MISSIONAI_CHOKE, pChokedCityPlot);
			else
				getGroup()->pushMission(MISSION_SKIP, -1, -1, iFlags, false, false, MISSIONAI_CHOKE, pChokedCityPlot);
			return true;
		}
		else
		{
			FAssert(!atPlot(pEndTurnPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pEndTurnPlot->getX(), pEndTurnPlot->getY(), iFlags, false, false, MISSIONAI_CHOKE, pChokedCityPlot);
			return true;
		}
	}
	
	return false;
}

// <advc.012>
int CvUnitAI::AI_plotDefense(CvPlot const* p) const {

	// Don't check noDefensiveBonus here b/c this unit can be part of a stack
	//if(noDefensiveBonus()) return 0;
	if(p == NULL)
		p = plot();
	return GET_TEAM(getTeam()).AI_plotDefense(*p);
} // </advc.012>

bool CvUnitAI::AI_solveBlockageProblem(CvPlot* pDestPlot, bool bDeclareWar)
{
	PROFILE_FUNC();

	FAssert(pDestPlot != NULL);
	

	if (pDestPlot != NULL)
	{
		FAStarNode* pStepNode;
		
		CvPlot* pSourcePlot = plot();

		if (gDLL->getFAStarIFace()->GeneratePath(&GC.getStepFinder(), pSourcePlot->getX_INLINE(), pSourcePlot->getY_INLINE(), pDestPlot->getX_INLINE(), pDestPlot->getY_INLINE(), false, 0, true))
		{
			pStepNode = gDLL->getFAStarIFace()->GetLastNode(&GC.getStepFinder());

			while (pStepNode != NULL)
			{
				CvPlot* pStepPlot = GC.getMapINLINE().plotSorenINLINE(pStepNode->m_iX, pStepNode->m_iY);
				if (canMoveOrAttackInto(pStepPlot) && generatePath(pStepPlot, 0, true))
				{
					if (bDeclareWar && pStepNode->m_pPrev != NULL)
					{
						CvPlot* pPlot = GC.getMapINLINE().plotSorenINLINE(pStepNode->m_pPrev->m_iX, pStepNode->m_pPrev->m_iY);
						if (pPlot->getTeam() != NO_TEAM)
						{
							if (!canMoveInto(pPlot, true, true))
							{
								if (!isPotentialEnemy(pPlot->getTeam(), pPlot))
								{									
									CvTeamAI& kTeam = GET_TEAM(getTeam());
									if (kTeam.canDeclareWar(pPlot->getTeam()))
									{
										WarPlanTypes eWarPlan = WARPLAN_LIMITED;
										WarPlanTypes eExistingWarPlan = kTeam.AI_getWarPlan(pDestPlot->getTeam());
										if (eExistingWarPlan != NO_WARPLAN)
										{
											if ((eExistingWarPlan == WARPLAN_TOTAL) || (eExistingWarPlan == WARPLAN_PREPARING_TOTAL))
											{
												eWarPlan = WARPLAN_TOTAL;
											}

											if (!kTeam.isAtWar(pDestPlot->getTeam()))
											{
												kTeam.AI_setWarPlan(pDestPlot->getTeam(), NO_WARPLAN);
											}
										}
										kTeam.AI_setWarPlan(pPlot->getTeam(), eWarPlan, true);
										//return (AI_targetCity());
										return AI_goToTargetCity(MOVE_AVOID_ENEMY_WEIGHT_2 | MOVE_DECLARE_WAR); // K-Mod / BBAI
									}
								}
							}
						}
					}
					if (pStepPlot->isVisibleEnemyUnit(this))
					{
						FAssert(canAttack());
						CvPlot* pBestPlot = pStepPlot;
						//To prevent puppeteering attempt to barge through
						//if quite close
						if (getPathFinder().GetPathTurns() > 3)
						{
							pBestPlot = getPathEndTurnPlot();
						}

						FAssert(!atPlot(pBestPlot));
						getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_DIRECT_ATTACK);
						return true;						
					}
				}
				pStepNode = pStepNode->m_pParent;
			}
		}
	}

	return false;
}

int CvUnitAI::AI_calculatePlotWorkersNeeded(CvPlot* pPlot, BuildTypes eBuild)
{
	int iBuildTime = pPlot->getBuildTime(eBuild, /* advc.251: */ getOwnerINLINE())
			- pPlot->getBuildProgress(eBuild);
	int iWorkRate = workRate(true);
	
	if (iWorkRate <= 0)
	{
		FAssert(false);
		return 1;
	}
	int iTurns = iBuildTime / iWorkRate;
	
	if (iBuildTime > (iTurns * iWorkRate))
	{
		iTurns++;
	}
	
	int iNeeded = std::max(1, (iTurns + 2) / 3);
	
	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD						7/31/08				jdog5000	*/
	/* 																			*/
	/* 	Bugfix																	*/
	/********************************************************************************/
	//if (pPlot->getBonusType() != NO_BONUS)
	if (pPlot->getNonObsoleteBonusType(getTeam()) != NO_BONUS)
	{
		iNeeded *= 2;		
	}
	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD						END								*/
	/********************************************************************************/

	return iNeeded;
	
}

bool CvUnitAI::AI_canGroupWithAIType(UnitAITypes eUnitAI) const
{
	if (eUnitAI != AI_getUnitAIType())
	{
		switch (eUnitAI)
		{
		case (UNITAI_ATTACK_CITY):
			if (plot()->isCity() && (GC.getGame().getGameTurn() - plot()->getPlotCity()->getGameTurnAcquired()) <= 1)
			{
				return false;
			}
			break;
		default:
			break;
		}
	}

	return true;
}



bool CvUnitAI::AI_allowGroup(const CvUnit* pUnit, UnitAITypes eUnitAI) const
{
	CvSelectionGroup* pGroup = pUnit->getGroup();
	CvPlot* pPlot = pUnit->plot();

	if (pUnit == this)
	{
		return false;
	}

	if (!pUnit->isGroupHead())
	{
		return false;
	}

	if (pGroup == getGroup())
	{
		return false;
	}

	if (pUnit->isCargo())
	{
		return false;
	}

	if (pUnit->AI_getUnitAIType() != eUnitAI)
	{
		return false;
	}

	switch (pGroup->AI_getMissionAIType())
	{
	case MISSIONAI_GUARD_CITY:
		// do not join groups that are guarding cities
		// intentional fallthrough
	case MISSIONAI_LOAD_SETTLER:
	case MISSIONAI_LOAD_ASSAULT:
	case MISSIONAI_LOAD_SPECIAL:
		// do not join groups that are loading into transports (we might not fit and get stuck in loop forever)
		return false;
		break;
	default:
		break;
	}

	if (pGroup->getActivityType() == ACTIVITY_HEAL)
	{
		// do not attempt to join groups which are healing this turn
		// (healing is cleared every turn for automated groups, so we know we pushed a heal this turn)
		return false;
	}

	if (!canJoinGroup(pPlot, pGroup))
	{
		return false;
	}

	if (eUnitAI == UNITAI_SETTLE)
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
		//if (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(pPlot, 3) > 0)
		if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(pPlot, 3))
		{
			return false;
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	}
	else if (eUnitAI == UNITAI_ASSAULT_SEA)
	{
		if (!pGroup->hasCargo())
		{
			return false;
		}
	}

	if ((getGroup()->getHeadUnitAI() == UNITAI_CITY_DEFENSE))
	{
		if (plot()->isCity() && (plot()->getTeam() == getTeam()) && plot()->getBestDefender(getOwnerINLINE())->getGroup() == getGroup())
		{
			return false;
		}
	}

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		CvPlot* pTargetPlot = pGroup->AI_getMissionAIPlot();

		if (pTargetPlot != NULL)
		{
			if (pTargetPlot->isOwned())
			{
				if (isPotentialEnemy(pTargetPlot->getTeam(), pTargetPlot))
				{
					//Do not join groups which have debarked on an offensive mission
					return false;
				}
			}
		}
	}

	if (pUnit->getInvisibleType() != NO_INVISIBLE)
	{
		if (getInvisibleType() == NO_INVISIBLE)
		{
			return false;
		}
	}

	return true;
}

// <advc.040>
bool CvUnitAI::AI_moveSettlerToCoast(int iMaxPathTurns) {

	CvPlayer const& owner = GET_PLAYER(getOwnerINLINE()); int foo=-1;
	CvCity* currentCity = plot()->getPlotCity();
	if(currentCity == NULL)
		return false;
	int const iGroupSz = getGroupSize();
	CvCity* pBest = NULL;
	CvPlot* pEndPlot = NULL;
	int iBest = 0;
	for(CvCity* c = owner.firstCity(&foo); c != NULL; c = owner.nextCity(&foo)) {
		CvPlot& cp = *c->plot();
		if(c == currentCity || c->area() != area() || !c->isCoastal() ||
				!AI_plotValid(&cp) || c->AI_isEvacuating() ||
				owner.AI_getPlotDanger(&cp) > iGroupSz - 1)
			continue;
		int iPathTurns=-1;
		if(generatePath(&cp, 0, true, &iPathTurns, iMaxPathTurns)) {
			int iValue = 5 + iMaxPathTurns - iPathTurns;
			if(cp.plotCheck(PUF_isUnitAIType, UNITAI_SETTLER_SEA, -1, owner.getID()) != NULL)
				iValue += 100;
			if(iValue > iBest) {
				iBest = iValue;
				pBest = c;
				pEndPlot = getPathEndTurnPlot();
			}
		}
	}
	if(pBest == NULL)
		return false;
	getGroup()->pushMission(MISSION_MOVE_TO, pEndPlot->getX_INLINE(),
			pEndPlot->getY_INLINE(), (iGroupSz > 1 ? 0 : MOVE_SAFE_TERRITORY),
			false, false, MISSIONAI_LOAD_SETTLER, pBest->plot());
	return true;
} // </advc.040>

void CvUnitAI::read(FDataStreamBase* pStream)
{
	CvUnit::read(pStream);

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iBirthmark);

	pStream->Read((int*)&m_eUnitAIType);
	pStream->Read(&m_iAutomatedAbortTurn);
}


void CvUnitAI::write(FDataStreamBase* pStream)
{
	CvUnit::write(pStream);

	uint uiFlag=0;
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iBirthmark);

	pStream->Write(m_eUnitAIType);
	pStream->Write(m_iAutomatedAbortTurn);
}

// Private Functions...

// Lead From Behind, by UncutDragon, edited for K-Mod
void CvUnitAI::LFBgetBetterAttacker(CvUnit** ppAttacker, const CvPlot* pPlot, bool bPotentialEnemy, int& iAIAttackOdds, int& iAttackerValue)
{
	CvUnit* pDefender = pPlot->getBestDefender(NO_PLAYER, getOwnerINLINE(), this, !bPotentialEnemy, bPotentialEnemy);

	int iOdds;
	int iValue = LFBgetAttackerRank(pDefender, iOdds);

	// Combat odds are out of 1000, but the AI routines need odds out of 100, and when called from AI_getBestGroupAttacker
	// we return this value. Note that I'm not entirely sure if/how that return value is actually used ... but just in case I
	// want to make sure I'm returning something consistent with what was there before
	int iAIOdds = (iOdds + 5) / 10;
	iAIOdds += GET_PLAYER(getOwnerINLINE()).AI_getAttackOddsChange();
	iAIOdds = std::max(1, std::min(iAIOdds, 99));

	if (collateralDamage() > 0)
	{
		int iPossibleTargets = std::min((pPlot->getNumVisibleEnemyDefenders(this) - 1), collateralDamageMaxUnits());

		if (iPossibleTargets > 0)
		{
			iValue *= (100 + ((collateralDamage() * iPossibleTargets) / 5));
			iValue /= 100;
		}
	}

	// Nothing to compare against - we're obviously better
	if (!(*ppAttacker))
	{
		(*ppAttacker) = this;
		iAIAttackOdds = iAIOdds;
		iAttackerValue = iValue;
		return;
	}

	// Compare our adjusted value with the current best
	if (iValue > iAttackerValue)
	{
		(*ppAttacker) = this;
		iAIAttackOdds = iAIOdds;
		iAttackerValue = iValue;
	}
}


