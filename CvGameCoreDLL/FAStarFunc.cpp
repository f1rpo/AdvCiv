#include "CvGameCoreDLL.h"
#include "FAStarFunc.h"
#include "FAStarNode.h"
#include "CvSelectionGroupAI.h"
#include "CvUnit.h"
#include "CoreAI.h"
#include "CvInfo_Terrain.h" // for terrain damage

// advc.pf: New implementation file; see comment in header.

/*	advc.pf: I've mainly wanted to increase PATH_STEP_WEIGHT.
	Symmetry breaking and PATH_STRAIGHT_WEIGHT can add up to a total of 6 per step.
	STEP_WEIGHT 7 still isn't always enough to trump that,
	so I'm adding 1 more later on for human units.
	Don't want to scale everything up too much, i.e. not too close to 1000.
	(Though I guess one could just increase PATH_MOVEMENT_WEIGHT as well, e.g. to 2048.) */

//#define PATH_MOVEMENT_WEIGHT    (1000) // advc.inl: Had to move this one to the header
// advc.pf: Was 20 in K-Mod, 100 in BtS.
#define PATH_RIVER_WEIGHT         (32) // river crossing penalty
// advc.pf: Was 200 in K-Mod, 100 in BtS.
#define PATH_CITY_WEIGHT         (225) // K-Mod
// advc.pf: Was 4 in K-Mod, 10 in BtS.
#define PATH_DEFENSE_WEIGHT        (7) // defence bonus
// advc.pf: Was 5 in K-Mod, 3 in BtS.
#define PATH_TERRITORY_WEIGHT      (9)
#define PATH_DOW_WEIGHT	           (7) // advc.082
// advc.pf: Was 4 in K-Mod, 2 in BtS.
#define PATH_STEP_WEIGHT           (7)
#define PATH_STRAIGHT_WEIGHT       (2) // K-Mod: was 1
//#define PATH_ASYMMETRY_WEIGHT    (1) // K-Mod

// #define PATH_DAMAGE_WEIGHT      (500) // K-Mod (disabled because it isn't used)
// advc.pf: Was 300 in K-Mod
#define PATH_COMBAT_WEIGHT         (350) // K-Mod. penalty for having to fight along the way.
// Note: there will also be other combat penalties added, for example from defence weight and city weight.

// (edited by K-Mod)
BOOL pathDestValid(int iToX, int iToY, void const* pointer, FAStar* finder)
{	// <advc.pf>
	return pathDestValid(GC.getMap().getPlot(iToX, iToY), *(CvSelectionGroup*)pointer,
			(MovementFlags)gDLL->getFAStarIFace()->GetInfo(finder));
}


bool pathDestValid(CvPlot const& kTo, CvSelectionGroup const& kGroup, MovementFlags eFlags)
{	// </advc.pf>
	PROFILE_FUNC();

	if (kGroup.at(kTo))
		return true;

	if (kGroup.getDomainType() == DOMAIN_IMMOBILE)
		return false;

	bool const bAIControl = kGroup.AI_isControlled();

	if (bAIControl)
	{	/*  BETTER_BTS_AI_MOD, Efficiency, 11/04/09, jdog5000: START
			switch order as AI_isAnyPlotDanger is more expensive */
		if (kGroup.getDomainType() == DOMAIN_LAND)
		{
			CvArea const& kGroupArea = *kGroup.area();
			if (!kTo.isArea(kGroupArea) &&
				!kGroup.canMoveAllTerrain() &&
				!kTo.isAdjacentToArea(kGroupArea))
			{
				return false;
			}
		}
		if (!(eFlags & MOVE_IGNORE_DANGER))
		{
			if (!kGroup.canFight() && !kGroup.alwaysInvisible())
			{
				if (GET_PLAYER(kGroup.getHeadOwner()).AI_isAnyPlotDanger(kTo))
					return false;
			}
		}
		// BETTER_BTS_AI_MOD: END
	}

	if (bAIControl || kTo.isRevealed(kGroup.getHeadTeam()))
	{
		if (kGroup.isAmphibPlot(&kTo))
		{
			for (CLLNode<IDInfo> const* pUnitNode1 = kGroup.headUnitNode();
				pUnitNode1 != NULL; pUnitNode1 = kGroup.nextUnitNode(pUnitNode1))
			{
				CvUnit const* pLoopUnit1 = ::getUnit(pUnitNode1->m_data);
				if (pLoopUnit1->getCargo() > 0 && pLoopUnit1->domainCargo() == DOMAIN_LAND)
				{
					bool bValid = false;
					for (CLLNode<IDInfo> const* pUnitNode2 = pLoopUnit1->getPlot().headUnitNode();
						pUnitNode2 != NULL; pUnitNode2 = pLoopUnit1->getPlot().nextUnitNode(pUnitNode2))
					{
						CvUnit const* pLoopUnit2 = ::getUnit(pUnitNode2->m_data);
						if (pLoopUnit2->getTransportUnit() == pLoopUnit1)
						{
							if (pLoopUnit2->isGroupHead())
							{
								if (pLoopUnit2->getGroup()->canMoveOrAttackInto(kTo,
									//kGroup.AI().AI_isDeclareWar(kTo) || (eFlags & MOVE_DECLARE_WAR)))
									// K-Mod. The new AI must be explicit about declaring war.
									eFlags & MOVE_DECLARE_WAR, false, bAIControl))
								{
									bValid = true;
									break;
								}
							}
						}
					}
					if (bValid)
						return true;
				}
			}

			return false;
		}
		else
		{
			if (!kGroup.canMoveOrAttackInto(kTo,
				//pSelectionGroup->AI_isDeclareWar(pToPlot) || (eFlags & MOVE_DECLARE_WAR))
				// K-Mod. The new AI must be explicit about declaring war.
				eFlags & MOVE_DECLARE_WAR, false, bAIControl))
			{
				return false;
			}
		}
	}

	return true;
}

/*	This function has been completely rewritten for K-Mod.
	(the rewrite includes some bug fixes as well as some new features) */
int pathCost(FAStarNode* parent, FAStarNode* node,
	int data, // advc (note): unused
	void const* pointer, FAStar* finder)
{	// <advc.pf>
	return pathCost(
			GC.getMap().getPlot(parent->m_iX, parent->m_iY),
			GC.getMap().getPlot(node->m_iX, node->m_iY),
			*(CvSelectionGroup*)pointer,
			(MovementFlags)gDLL->getFAStarIFace()->GetInfo(finder),
			parent->m_iData1, parent->m_iKnownCost);
}


int pathCost(CvPlot const& kFrom, CvPlot const& kTo,
	CvSelectionGroup const& kGroup, MovementFlags eFlags,
	// Not sure if I've named these suitably
	int iCurrMovesLeft, int iKnownCost)
{	// </advc.pf>
	//PROFILE_FUNC(); // advc.003o

	int iWorstCost = 0;
	int iWorstMovesLeft = MAX_INT;
	int iWorstMaxMoves = MAX_INT;

	TeamTypes const eTeam = kGroup.getHeadTeam();
	// <advc.035>
	int const iFlipModifierDiv = 7;
	int iFlipModifier = iFlipModifierDiv;
	// </advc.035>
	// K-Mod
	int iExploreModifier = 3; // (in thirds)
	if (!kTo.isRevealed(eTeam))
	{
		if (kGroup.getAutomateType() == AUTOMATE_EXPLORE ||
			(!kGroup.isHuman() &&
			(kGroup.getHeadUnitAIType() == UNITAI_EXPLORE ||
			kGroup.getHeadUnitAIType() == UNITAI_EXPLORE_SEA)))
		{
			iExploreModifier = 2; // lower cost to encourage exploring unrevealed areas
		}
		else if (!kFrom.isRevealed(eTeam))
		{
			iExploreModifier = 4; // higher cost to discourage pathfinding deep into the unknown
		}
	}
	// K-Mod end
	// <advc.035>
	else if(GC.getDefineBOOL(CvGlobals::OWN_EXCLUSIVE_RADIUS) &&
		(eFlags & MOVE_DECLARE_WAR) && eTeam != BARBARIAN_TEAM)
	{
		PlayerTypes const eSecondOwner = kTo.getSecondOwner();
		PlayerTypes const eFirstOwner = kTo.getOwner();
		if(eSecondOwner != NO_PLAYER && eFirstOwner != NO_PLAYER &&
			((kGroup.getDomainType() == DOMAIN_SEA) == kTo.isWater()))
		{	// Avoid tiles that flip from us to the enemy upon DoW
			if(TEAMID(eFirstOwner) == eTeam && (GET_TEAM(eTeam).isHuman() ?
				(!GET_TEAM(eTeam).isFriendlyTerritory(TEAMID(eSecondOwner)) &&
				!GET_TEAM(eTeam).isAtWar(TEAMID(eSecondOwner))) :
				GET_TEAM(eTeam).AI_isSneakAttackReady(TEAMID(eSecondOwner))))
			{
				iFlipModifier++;
			}
			// Seek out enemy tiles that will flip to us upon DoW
			if(TEAMID(eSecondOwner) == eTeam && (GET_TEAM(eTeam).isHuman() ?
				(!GET_TEAM(eTeam).isFriendlyTerritory(TEAMID(eFirstOwner)) &&
				!GET_TEAM(eTeam).isAtWar(TEAMID(eFirstOwner))) :
				GET_TEAM(eTeam).AI_isSneakAttackReady(TEAMID(eFirstOwner))))
			{
				iFlipModifier--;
			}
			/*  This could be done much more accurately, taking into account
				vassal agreements, defensive pacts, and going through the entire
				selection group, but I worry about the performance, and it's OK
				if it doesn't always work correctly. */
		}
	} // </advc.035>
	{
		for (CLLNode<IDInfo> const* pUnitNode = kGroup.headUnitNode();
			pUnitNode != NULL; pUnitNode = kGroup.nextUnitNode(pUnitNode))
		{
			CvUnit const* pLoopUnit = ::getUnit(pUnitNode->m_data);
			FAssert(pLoopUnit->getDomainType() != DOMAIN_AIR);

			int iMaxMoves = (iCurrMovesLeft > 0 ? iCurrMovesLeft : pLoopUnit->maxMoves());
			int iMoveCost = kTo.movementCost(pLoopUnit, &kFrom,
					false); // advc.001i
			int iMovesLeft = std::max(0, iMaxMoves - iMoveCost);

			iWorstMovesLeft = std::min(iWorstMovesLeft, iMovesLeft);
			iWorstMaxMoves = std::min(iWorstMaxMoves, iMaxMoves);

			int iCost = PATH_MOVEMENT_WEIGHT * (iMovesLeft == 0 ? iMaxMoves : iMoveCost);
			iCost = (iCost * iExploreModifier) / 3;
			iCost = (iCost * iFlipModifier) / iFlipModifierDiv; // advc.035
			if (iCost > iWorstCost)
			{
				iWorstCost = iCost;
				iWorstMovesLeft = iMovesLeft;
				iWorstMaxMoves = iMaxMoves;
			}
		}
	}

	iWorstCost += PATH_STEP_WEIGHT;

	/*	symmetry breaking. This is meant to prevent two paths from having equal cost.
		(If two paths have equal cost, sometimes the interface shows one path
		and the units follow the other. This is bad.) */
	/* original K-Mod symmetry breaking. (extra cost for turning a corner)
	if (parent->m_pParent)
	{
		const int map_width = GC.getMap().getGridWidth();
		const int map_height = GC.getMap().getGridHeight();

		#define WRAP_X(x) ((x) - ((x) > map_width/2 ? map_width : 0) + ((x) < -map_width/2 ? map_width : 0))
		#define WRAP_Y(y) ((y) - ((y) > map_height/2 ? map_height : 0) + ((y) < -map_height/2 ? map_height : 0))

		int start_x = parent->m_pParent->m_iX;
		int start_y = parent->m_pParent->m_iY;

		int dx1 = WRAP_X(pFromPlot->getX() - start_x);
		int dy1 = WRAP_Y(pFromPlot->getY() - start_y);
		int dx2 = WRAP_X(pToPlot->getX() - start_x);
		int dy2 = WRAP_Y(pToPlot->getY() - start_y);

		// cross product. (greater than zero => sin(angle) > 0 => angle > 0)
		int cross = dx1 * dy2 - dx2 * dy1;
		if (cross > 0)
			iWorstCost += PATH_ASYMMETRY_WEIGHT; // turning left
		else if (cross < 0)
			iWorstCost -= PATH_ASYMMETRY_WEIGHT; // turning right

		// woah - hang on. Does that say /minus/ asym weight?
		// Doesn't this guy know that bad things happen if the total weight is negative?
		// ...
		// take a breath.
		#if PATH_STEP_WEIGHT < PATH_ASYMMETRY_WEIGHT
		#error "I'm sorry, but I must demand that step weight be greater than asym weight."
		#endif
				// I think we're going to be ok.

		#undef WRAP_X
		#undef WRAP_Y
	} */

	/*	Unfortunately, the above code is not sufficient to fix the symmetry problem.
		Here's a new method, which combines symmetry breaking
		with the old "straight path" effect.
		Note: symmetry breaking is not important for AI controlled units. */

	/*	It's actually marginally better strategy to move diagonally - for mapping reasons.
		So let the AI prefer diagonal movement.
		However, diagonal zig-zags will probably seem unnatural and weird to humans
		who are just trying to move in a straight line.
		So let the pathfinding for human groups prefer cardinal movement. */
	bool const bAIControl = kGroup.AI_isControlled();
	if (bAIControl)
	{
		if (kFrom.getX() == kTo.getX() || kFrom.getY() == kTo.getY())
			iWorstCost += PATH_STRAIGHT_WEIGHT;
	}
	else
	{
		if (kFrom.getX() != kTo.getX() && kFrom.getY() != kTo.getY())
		{
			iWorstCost += PATH_STRAIGHT_WEIGHT *
					(1 + ((kTo.getX() + kTo.getY()) % 2));
		}
		iWorstCost += (kTo.getX() + kTo.getY() + 1) % 3;
		iWorstCost++; // advc.pf: Essentially 1 extra
	}
	/*	unfortunately, this simple method may have problems at the world-wrap boundaries.
		It's difficult to tell when to correct for wrap effects and when not to,
		because as soon as the unit starts moving, the start position of the path changes,
		and so it's no longer possible to tell whether or not the unit started
		on the other side of the boundary. Drat. */

	// end symmetry breaking.

	if (!kTo.isRevealed(eTeam))
		return iWorstCost;

	// the cost of battle...
	if (eFlags & MOVE_ATTACK_STACK)
	{
		FAssert(bAIControl); // only the AI uses MOVE_ATTACK_STACK
		FAssert(kGroup.getDomainType() == DOMAIN_LAND);

		int iEnemyDefence = 0;

		if (kTo.isVisible(eTeam))
		{	/*  <advc.001> In the rare case that the AI plans war while animals
				still roam the map, the DefenceStrength computation will crash
				when it gets to the point where the UnitCombatType is accessed.
				(Actually, not so exotic b/c advc.300 allows animals to survive
				in continents w/o civ cities.) */
			CvUnit* pUnit = kTo.headUnit();
			if (pUnit != NULL && !pUnit->isAnimal()) // </advc.001>
			{
				iEnemyDefence = GET_PLAYER(kGroup.getOwner()).
						AI_localDefenceStrength(&kTo, NO_TEAM,
						kGroup.getDomainType(), 0, true, false,
						kGroup.isHuman());
			}
		}
		else
		{
			// plot not visible. use memory
			iEnemyDefence = GET_TEAM(eTeam).AI_getStrengthMemory(kTo.getX(), kTo.getY());
		}

		if (iEnemyDefence > 0)
		{
			iWorstCost += PATH_COMBAT_WEIGHT;
			int iAttackRatio = std::max(10,
					100 * kGroup.AI().AI_sumStrength(&kTo) / iEnemyDefence);
			/*	Note. I half intend to have pathValid return false whenever the
				above ratio is less than 100. I just haven't done that yet,
				mostly because I'm worried about performance. */
			if (iAttackRatio < 400)
			{
				iWorstCost += PATH_MOVEMENT_WEIGHT * GC.getMOVE_DENOMINATOR() *
						(400-iAttackRatio) / std::min(150, iAttackRatio);
			}
			// else, don't worry about it too much.
		}
	} //

	// <advc.082>
	TeamTypes eToPlotTeam = kTo.getTeam();
	/*  The AVOID_ENEMY code in the no-moves-left branch below doesn't stop the AI
		from trying to move _through_ enemy territory and thus declaring war
		earlier than necessary */
	if(bAIControl && (eFlags & MOVE_DECLARE_WAR) && eToPlotTeam != NO_TEAM &&
		eToPlotTeam != eTeam && GET_TEAM(eTeam).AI_isSneakAttackReady(eToPlotTeam))
	{
		iWorstCost += PATH_DOW_WEIGHT;
	} // </advc.082>
	if (iWorstMovesLeft <= 0)
	{
		if (eToPlotTeam != eTeam)
			iWorstCost += PATH_TERRITORY_WEIGHT;

		// Damage caused by features (for mods)
		if (GC.getDefineINT(CvGlobals::PATH_DAMAGE_WEIGHT) != 0)
		{
			if (kTo.isFeature())
			{
				iWorstCost += (GC.getDefineINT(CvGlobals::PATH_DAMAGE_WEIGHT) *
						std::max(0, GC.getInfo(kTo.getFeatureType()).getTurnDamage())) /
						GC.getMAX_HIT_POINTS();
			}

			if (kTo.getExtraMovePathCost() > 0)
				iWorstCost += (PATH_MOVEMENT_WEIGHT * kTo.getExtraMovePathCost());
		}

		// defence modifiers
		int iDefenceMod = 0;
		int iDefenceCount = 0;
		int iFromDefenceMod = 0; // defence bonus for our attacking units left behind
		int iAttackWeight = 0;
		int iAttackCount = 0;
		int const iEnemies = kTo.getNumVisibleEnemyDefenders(kGroup.getHeadUnit());

		for (CLLNode<IDInfo> const* pUnitNode = kGroup.headUnitNode();
			pUnitNode != NULL; pUnitNode = kGroup.nextUnitNode(pUnitNode))
		{
			CvUnit const* pLoopUnit = ::getUnit(pUnitNode->m_data);
			if (!pLoopUnit->canFight())
				continue; // advc
			iDefenceCount++;
			if (pLoopUnit->canDefend(&kTo))
			{
				iDefenceMod += pLoopUnit->noDefensiveBonus() ? 0 :
						//pToPlot->defenseModifier(eTeam, false);
						GET_TEAM(eTeam).AI_plotDefense(kTo); // advc.012
			}
			else iDefenceMod -= 100; // we don't want to be here.

			// K-Mod note. the above code doesn't count all defensive bonuses, unfortunately.
			// We could count everything like this:
			/*CombatDetails combat_details;
			pLoopUnit->maxCombatStr(pToPlot, NULL, &combat_details);
			iDefenceMod += combat_details.iModifierTotal;*/
			// but that seems like overkill. I'm worried it would be too slow.

			// defence for units who stay behind after attacking an enemy.
			if (iEnemies > 0)
			{
				/*	For human-controlled units, only apply the following effects
					for multi-step moves. (otherwise this might prevent the user
					from attacking from where they want to attack from.) */
				if (bAIControl || iKnownCost != 0 || eFlags & MOVE_HAS_STEPPED)
				{
					iAttackCount++;
					if (!pLoopUnit->noDefensiveBonus())
						iFromDefenceMod += kFrom.defenseModifier(eTeam, false);

					if (!kFrom.isCity())
					{
						iAttackWeight += PATH_CITY_WEIGHT;
						/*	it's done this way around rather than subtracting when in a city
							so that the overall adjustment can't be negative. */
					}
					if (pLoopUnit->canAttack() && !pLoopUnit->isRiver() &&
						kFrom.isRiverCrossing(directionXY(kFrom, kTo)))
					{
						iAttackWeight -= PATH_RIVER_WEIGHT *
								// Note, river modifier will be negative.
								GC.getDefineINT(CvGlobals::RIVER_ATTACK_MODIFIER);
						//iAttackMod -= (PATH_MOVEMENT_WEIGHT * iMovesLeft);
					}
				}
				/*	If this is a direct attack move from a human player,
					make sure it is the best value move possible.
					(This allows humans to choose which plot they attack from.)
					(note: humans have no way of ordering units to attack units en-route,
					so the fact that this is an attack move means we are at the destination.) */
				else if (pLoopUnit->canAttack()) // iKnownCost == 0 && !(eFlags & MOVE_HAS_STEPPED) && !bAIControl
					return PATH_STEP_WEIGHT; // DONE!
			}
		}
		//
		if (iAttackCount > 0)
		{
			// scale attack weights down if not all our units will need to fight.
			iAttackWeight *= std::min(iAttackCount, iEnemies);
			iAttackWeight /= iAttackCount;
			iFromDefenceMod *= std::min(iAttackCount, iEnemies);
			iFromDefenceMod /= iAttackCount;
			iAttackCount = std::min(iAttackCount, iEnemies);
		}
		//
		iWorstCost += PATH_DEFENSE_WEIGHT * std::max(0,
				(iDefenceCount * 200 - iDefenceMod) / std::max(1, iDefenceCount));
		iWorstCost += PATH_DEFENSE_WEIGHT * std::max(0,
				(iAttackCount * 200 - iFromDefenceMod) / std::max(1, iAttackCount));
		iWorstCost += std::max(0, iAttackWeight) / std::max(1, iAttackCount);
		/*	if we're in enemy territory, consider
			the sum of our defensive bonuses as well as the average */
		if (kTo.isOwned() && atWar(eToPlotTeam, eTeam))
		{
			iWorstCost += PATH_DEFENSE_WEIGHT * std::max(0,
					(iDefenceCount * 200 - iDefenceMod) / 5);
			iWorstCost += PATH_DEFENSE_WEIGHT * std::max(0,
					(iAttackCount * 200 - iFromDefenceMod) / 5);
			iWorstCost += std::max(0, iAttackWeight) / 5;
		}

		// additional cost for ending turn in or adjacent to enemy territory based on flags (based on BBAI)
		if (eFlags & (MOVE_AVOID_ENEMY_WEIGHT_2 | MOVE_AVOID_ENEMY_WEIGHT_3))
		{
			if (kTo.isOwned() && GET_TEAM(eTeam).AI_getWarPlan(eToPlotTeam) != NO_WARPLAN)
				iWorstCost *= ((eFlags & MOVE_AVOID_ENEMY_WEIGHT_3) ? 3 : 2);
			else
			{
				FOR_EACH_ENUM(Direction)
				{
					CvPlot const* pAdj = plotDirection(kTo.getX(), kTo.getY(), eLoopDirection);
					if (pAdj == NULL)
						continue;
					if (pAdj->isOwned() &&
						GET_TEAM(pAdj->getTeam()).isAtWar(kGroup.getHeadTeam()))
					{
						if (eFlags & MOVE_AVOID_ENEMY_WEIGHT_3)
						{
							iWorstCost *= 3;
							iWorstCost /= 2;
						}
						else
						{
							iWorstCost *= 4;
							iWorstCost /= 3;
						}
					}
				}
			}
		}
	}

	FAssert(iWorstCost > 0);

	return iWorstCost;
}


/*	advc.inl (note): These two functions are ultimately only called in the EXE,
	so there's no point in trying to get them inlined. */
int stepHeuristic(int iFromX, int iFromY, int iToX, int iToY)
{
	return stepDistance(iFromX, iFromY, iToX, iToY);
}

int stepCost(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder)
{
	return 1;
}


BOOL pathValid(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder)
{
	PROFILE_FUNC();

	if(parent == NULL)
		return TRUE;
	// advc: Was unused (apart from an assertion)
	/*CvPlot* pFromPlot = ...;
	CvPlot* pToPlot = ...; */
	//pSelectionGroup = ((CvSelectionGroup *)pointer);
	// K-Mod
	/*CvSelectionGroup const* pSelectionGroup = finder ? (CvSelectionGroup*)pointer :
			((CvPathSettings*)pointer)->pGroup;
	int iFlags = finder ? gDLL->getFAStarIFace()->GetInfo(finder) :
			((CvPathSettings*)pointer)->iFlags;*/
	// K-Mod end
	/*	<advc.pf> KmodPathFinder doesn't call this function, so it
		doesn't have to be able to handle those arguments. */
	CvPlot const& kFrom = GC.getMap().getPlot(parent->m_iX, parent->m_iY);
	CvPlot const& kTo = GC.getMap().getPlot(node->m_iX, node->m_iY);
	CvSelectionGroup const& kGroup = *(CvSelectionGroup*)pointer;
	MovementFlags eFlags = (MovementFlags)gDLL->getFAStarIFace()->GetInfo(finder);
	// </advc.pf>
	if (!pathValid_join(kFrom, kTo, kGroup, eFlags))
		return FALSE;
	if (!pathValid_source(kFrom, kGroup, eFlags, // K-Mod
		parent->m_iData1, parent->m_iData2)) // advc.pf
	{
		return FALSE;
	}
	return TRUE;
}


bool pathValid_join(CvPlot const& kFrom, CvPlot const& kTo, CvSelectionGroup const& kGroup, MovementFlags eFlags)
{
	return (kGroup.getDomainType() != DOMAIN_SEA ||
			!GC.getMap().isSeparatedByIsthmus(kFrom, kTo) ||
			kGroup.canMoveAllTerrain());
}


bool pathValid_source(CvPlot const& kFrom, CvSelectionGroup const& kGroup, MovementFlags eFlags,
	int iMovesLeft, int iTurns) // advc.pf (instead of passing the node pointer)
{
	//PROFILE_FUNC(); // advc.003o

	if (kGroup.atPlot(&kFrom))
		return true;

	if (eFlags & MOVE_SAFE_TERRITORY)
	{
		if (kFrom.isOwned() && kFrom.getTeam() != kGroup.getHeadTeam())
			return false;

		if (!kFrom.isRevealed(kGroup.getHeadTeam()))
			return false;
	}
	// <advc.pf> No new AI routes in human territory (but upgrade to railroad OK)
	if(eFlags & MOVE_ROUTE_TO)
	{
		if(kFrom.getRevealedRouteType(kGroup.getHeadTeam()) == NO_ROUTE &&
			!kGroup.isHuman())
		{
			PlayerTypes eOwner = kFrom.getOwner();
			if(eOwner != NO_PLAYER && GET_PLAYER(eOwner).isHuman())
				return false;
		}
	} // </advc.pf>

	if ((eFlags & MOVE_NO_ENEMY_TERRITORY) && kFrom.isOwned() &&
		GET_TEAM(kFrom.getTeam()).isAtWar(kGroup.getHeadTeam()))
	{
		return false;
	}
	bool const bAIControl = kGroup.AI_isControlled();
	if (bAIControl)
	{
		if (iTurns > 1 || iMovesLeft == 0)
		{
			if (!(eFlags & MOVE_IGNORE_DANGER))
			{
				if (!kGroup.canFight() && !kGroup.alwaysInvisible())
				{
					if (GET_PLAYER(kGroup.getHeadOwner()).AI_isAnyPlotDanger(kFrom))
						return false;
				}
			}
		}
	}

	if (bAIControl || kFrom.isRevealed(kGroup.getHeadTeam()))
	{
		if (eFlags & (MOVE_THROUGH_ENEMY /* K-Mod: */ | MOVE_ATTACK_STACK))
		{
			if (!kGroup.canMoveOrAttackInto(kFrom,
				eFlags & MOVE_DECLARE_WAR && !kGroup.isHuman())) // K-Mod
			{
				return false;
			}
		}
		else
		{
			if (!kGroup.canMoveThrough(kFrom,
				// K-Mod
				(eFlags & MOVE_DECLARE_WAR) && !kGroup.isHuman(),
				(eFlags & MOVE_ASSUME_VISIBLE) || !kGroup.isHuman()))
				// K-Mod end
			{
				return false;
			}
		}
	}
	/*	K-Mod. Note: it's currently difficult to extract the
		vision-cheating part of this AI, because the AI needs to cheat
		inside canMoveOrAttackInto for its other cheating parts to work...
		.. anyway, here is the beginnings of what the code
		might look like without the cheats. (it's unfinished) */
	#if 0
	if (kFrom.isRevealed(kGroup.getHeadTeam(), false))
	{
		PROFILE("pathValid move through");
		CvTeamAI& kTeam = GET_TEAM(kGroup.getHeadTeam());

		int iEnemyDefence;
		if (kFrom.isVisible(kGroup.getHeadTeam(), false))
		{
			iEnemyDefence = GET_PLAYER(kGroup.getOwner()).AI_localDefenceStrength(pToPlot, NO_TEAM, kGroup.getDomainType(), 0, true, false, kGroup.isHuman());
		}
		else
		{
			iEnemyDefence = kTeam.AI_getStrengthMemory(&kFrom);
		}

		if (kTeam.AI_getStrengthMemory(&kFrom) > 0 && eFlags & (MOVE_THROUGH_ENEMY | MOVE_ATTACK_STACK))
		{
			if (!kGroup.canMoveOrAttackInto(kFrom) ||
				(eFlags & MOVE_ATTACK_STACK && kGroup.AI_sumStrength(&kFrom) < iEnemyDefence))
			{
				return false;
			}
		}
		else
		{
			if (!kGroup.canMoveThrough(kFrom))
			{
				return false;
			}
		}
	}
	#endif
	// K-Mod end

	return true;
}


BOOL pathAdd(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder)
{	// <advc.pf>
	CvSelectionGroup const& kGroup = *(CvSelectionGroup*)pointer;
	MovementFlags eFlags = (MovementFlags)gDLL->getFAStarIFace()->GetInfo(finder);
	if (data == ASNC_INITIALADD)
	{
		node->m_iData1 = pathInitialAdd(kGroup, eFlags);
		node->m_iData2 = 1;
	}
	else pathAdd(*parent, *node, kGroup, eFlags);
	return TRUE;
}


int pathInitialAdd(CvSelectionGroup const& kGroup, MovementFlags eFlags)
{
	// (This used to be a branch in pathAdd)
	FAssert(kGroup.getNumUnits() > 0);
	// K-Mod: Moved into separate functions
	return ((eFlags & MOVE_MAX_MOVES) ? kGroup.maxMoves() : kGroup.movesLeft());
}


void pathAdd(FAStarNode const& kParent, FAStarNode& kNode, CvSelectionGroup const& kGroup, MovementFlags eFlags)
{	// </advc.pf>
	//PROFILE_FUNC(); // advc.003o
	FAssert(kGroup.getNumUnits() > 0);
	CvMap const& kMap = GC.getMap();
	CvPlot const& kFrom = kMap.getPlot(kParent.m_iX, kParent.m_iY);
	CvPlot const& kTo = kMap.getPlot(kNode.m_iX, kNode.m_iY);
	int iStartMoves = kParent.m_iData1;
	int iTurns = kParent.m_iData2;
	int iMoves = MAX_INT;
	/*if (iStartMoves == 0)
		iTurns++;
	for (CLLNode<IDInfo>* pUnitNode = kGroup.headUnitNode(); pUnitNode != NULL;
			pUnitNode = kGroup.nextUnitNode(pUnitNode)) {
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		int iUnitMoves = (iStartMoves == 0 ? pLoopUnit->maxMoves() : iStartMoves);
		iUnitMoves -= kTo.movementCost(pLoopUnit, &kFrom);
		iUnitMoves = std::max(iUnitMoves, 0);
		iMoves = std::min(iMoves, iUnitMoves);
	}*/ // BtS
	/*K-Mod. The original code would give incorrect results for groups
		where one unit had more moves but also had higher move cost.
		(the most obvious example is when a group with 1-move units and 2-move units
		is moving on a railroad. - In this situation, the original code would consistently
		underestimate the remaining moves at every step.) */
	{
		bool bNewTurn = (iStartMoves == 0);
		if (bNewTurn)
		{
			iTurns++;
			iStartMoves = kGroup.maxMoves();
		}
	}
	CLLNode<IDInfo> const* pUnitNode = kGroup.headUnitNode();
	int iMoveCost = kTo.movementCost(::getUnit(pUnitNode->m_data), &kFrom,
			false); // advc.001i
	bool bUniformCost = true;
	for (pUnitNode = kGroup.nextUnitNode(pUnitNode);
		bUniformCost && pUnitNode != NULL; pUnitNode = kGroup.nextUnitNode(pUnitNode))
	{
		CvUnit const* pLoopUnit = ::getUnit(pUnitNode->m_data);
		int iLoopCost = kTo.movementCost(pLoopUnit, &kFrom,
				false); // advc.001i
		if (iLoopCost != iMoveCost)
			bUniformCost = false;
	}
	if (bUniformCost)
	{
		// the simple, normal case
		iMoves = std::max(0, iStartMoves - iMoveCost);
	}
	else
	{
		//PROFILE("pathAdd - non-uniform cost"); // advc.003o
		/*	Move costs are uneven for units in this group.
			To be sure of the true movement cost for the group,
			we need to calculate the movement cost for each unit
			for every step in this turn. */
		std::vector<CvPlot const*> plot_list; // will be traversed in reverse order
		plot_list.push_back(&kTo);
		plot_list.push_back(&kFrom);
		FAStarNode const* pStartNode = &kParent;
		while (pStartNode->m_iData2 == iTurns && pStartNode->m_pParent != NULL)
		{
			pStartNode = pStartNode->m_pParent;
			plot_list.push_back(kMap.plotSoren(pStartNode->m_iX, pStartNode->m_iY));
		}
		iMoves = MAX_INT;
		bool const bMaxMoves = (pStartNode->m_iData1 == 0 || eFlags & MOVE_MAX_MOVES);
		for (pUnitNode = kGroup.headUnitNode(); pUnitNode != NULL;
			pUnitNode = kGroup.nextUnitNode(pUnitNode))
		{
			CvUnit const* pLoopUnit = ::getUnit(pUnitNode->m_data);
			int iUnitMoves = (bMaxMoves ? pLoopUnit->maxMoves() : pLoopUnit->movesLeft());
			for (size_t i = plot_list.size() - 1; i > 0; i--)
			{
				iUnitMoves -= plot_list[i-1]->movementCost(pLoopUnit, plot_list[i],
						false); // advc.001i
				FAssert(iUnitMoves > 0 || i == 1);
			}

			iUnitMoves = std::max(iUnitMoves, 0);
			iMoves = std::min(iMoves, iUnitMoves);
		}
	}
	// K-Mod end

	FAssert(iMoves >= 0);
	kNode.m_iData1 = iMoves;
	kNode.m_iData2 = iTurns;
}


int stepDestValid(int iToX, int iToY, void const* pointer, FAStar* finder)
{
	PROFILE_FUNC();

	CvPlot const& kFrom = GC.getMap().getPlot(
			gDLL->getFAStarIFace()->GetStartX(finder), gDLL->getFAStarIFace()->GetStartY(finder));
	CvPlot const& kTo = GC.getMap().getPlot(iToX, iToY);
	if (!kFrom.sameArea(kTo))
		return FALSE;

	return TRUE;
}

// <advc.104b> Rule out (basically) no destinations; let teamStepValid_advc decide
int stepDestValid_advc(int iToX, int iToY, void const* pointer, FAStar* finder)
{
	CvPlot* pTo = GC.getMap().plotSoren(iToX, iToY);
	if(pTo == NULL || pTo->isImpassable())
		return FALSE;
	return TRUE;
}

// Can handle sea paths. Based on teamStepValid.
BOOL teamStepValid_advc(FAStarNode* parent, FAStarNode* node,
	int data, // advc (note): unused
	void const* pointer, FAStar* finder)
{
	//PROFILE_FUNC(); // advc.003o
	/*if(parent == NULL) // I don't think this can happen
		return TRUE;*/
	CvMap const& kMap = GC.getMap();
	CvPlot const& kTo = kMap.getPlot(node->m_iX, node->m_iY);
	if(kTo.isImpassable())
		return FALSE;
	CvPlot const& kFrom = kMap.getPlot(parent->m_iX, parent->m_iY);
	if(kMap.isSeparatedByIsthmus(kFrom, kTo))
		return FALSE;
	TeamTypes const ePlotTeam = kTo.getTeam();
	int* const v = (int*)pointer;
	int const iMaxPath = v[5];
	/*  As far as I understand the code, node (the pToPlot) is still set to 0
		cost if it's visited for the first time, so we should look at parent
		(pFromPlot) when enforcing the upper bound (iMaxPath). But it doesn't
		hurt to check node's cost too. */
	if(iMaxPath > 0 && (parent->m_iHeuristicCost + parent->m_iKnownCost > iMaxPath ||
		node->m_iHeuristicCost + node->m_iKnownCost > iMaxPath))
	{
		return FALSE;
	}
	TeamTypes const eTeam = (TeamTypes)v[0]; // The team that computes the path
	TeamTypes const eTargetTeam = (TeamTypes)v[1];
	DomainTypes eDom = (DomainTypes)v[2];
	// Check domain legality:
	if(eDom == DOMAIN_LAND && kTo.isWater())
		return FALSE;
	/*  <advc.033> Naval blockades (Barbarian eTeam) are allowed to reach a city
		but mustn't pass through */
	if(eTeam == BARBARIAN_TEAM && eDom != DOMAIN_LAND && kFrom.isCity() &&
		kFrom.getTeam() != BARBARIAN_TEAM)
	{
		return FALSE;
	} // </advc.033>
	bool const bEnterCityFromCoast = (eDom != DOMAIN_LAND && kTo.isCity(true) &&
			kTo.isCoastalLand());
	bool const bDestination = kTo.at(v[3], v[4]);
	// Use DOMAIN_IMMOBILE to encode sea units with impassable terrain
	bool bImpassableTerrain = false;
	if(eDom == DOMAIN_IMMOBILE)
	{
		bImpassableTerrain = true;
		eDom = DOMAIN_SEA;
	}
	if(eDom == DOMAIN_SEA && !bEnterCityFromCoast && !kTo.isWater() &&
		!bDestination) // Allow non-city land tile as cargo destination
	{
		return FALSE;
	}
	if(!bEnterCityFromCoast && !bDestination && ePlotTeam != eTeam && bImpassableTerrain &&
		/*  This handles only Coast and no other water terrain types that a mod-mod 
			might make passable */
		kTo.getTerrainType() != GC.getWATER_TERRAIN(true))
	{
		return FALSE;
	}
	// Don't check isRevealed; caller ensures that destination city is deducible.
	if(ePlotTeam == NO_TEAM)
		return TRUE;
	if(GET_TEAM(ePlotTeam).getMasterTeam() == GET_TEAM(eTargetTeam).getMasterTeam())
		return TRUE;
	CvTeamAI const& kTeam = GET_TEAM(eTeam);
	if(kTeam.canPeacefullyEnter(ePlotTeam))
		return TRUE;
	// A war plan isn't enough; war against eTargetTeam could supplant that plan.
	if(kTeam.isAtWar(ePlotTeam) &&
		/*  Units can't just move through an enemy city, but they can conquer
			it. Even ships can when part of a naval assault. They can't really
			conquer forts though. */
		(eDom == DOMAIN_LAND || !bEnterCityFromCoast || kTo.isCity()))
	{
		return TRUE;
	}
	return FALSE;
} // </advc.104b>


BOOL stepValid(FAStarNode* parent, FAStarNode* node,
	int data, // advc (note): unused
	void const* pointer, FAStar* finder)
{
	if (parent == NULL)
		return TRUE;

	CvPlot const& kTo = GC.getMap().getPlot(node->m_iX, node->m_iY);
	if (kTo.isImpassable())
		return FALSE;

	CvPlot const& kFrom = GC.getMap().getPlot(parent->m_iX, parent->m_iY);
	if (!kFrom.sameArea(kTo))
		return FALSE;

	// BETTER_BTS_AI_MOD, Bugfix, 12/12/08, jdog5000: START
	if (GC.getMap().isSeparatedByIsthmus(kFrom, kTo)) // (advc: Moved into new function)
		return FALSE; // BETTER_BTS_AI_MOD: END

	return TRUE;
}

/*  BETTER_BTS_AI_MOD, 02/02/09, jdog5000:
	Find paths that a team's units could follow without declaring war */
// advc (comment): Actually does assume a DoW on pointer[1] (eTargetTeam)
BOOL teamStepValid(FAStarNode* parent, FAStarNode* node,
	int data, // advc (note): unused
	void const* pointer, FAStar* finder)
{
	/*if (parent == NULL)
		return TRUE;*/ // advc: I don't think this can happen

	CvMap const& kMap = GC.getMap();
	CvPlot const& kTo = GC.getMap().getPlot(node->m_iX, node->m_iY);

	if (kTo.isImpassable())
		return FALSE;

	CvPlot const& kFrom = GC.getMap().getPlot(parent->m_iX, parent->m_iY);
	if (!kFrom.sameArea(kTo))
		return FALSE;

	if (kMap.isSeparatedByIsthmus(kFrom, kTo)) // advc: Moved into new function
		return FALSE;

	TeamTypes ePlotTeam = kTo.getTeam();
	if (ePlotTeam == NO_TEAM)
		return TRUE;

	std::vector<TeamTypes> teamVec = *((std::vector<TeamTypes>*)pointer);
	TeamTypes eTeam = teamVec[0];
	TeamTypes eTargetTeam = teamVec[1];
	CvTeamAI& kTeam = GET_TEAM(eTeam);
	// advc.001: Was just ePlotTeam == eTargetTeam; anticipate DoW on/ by vassals.
	if(eTargetTeam != NO_TEAM &&
		GET_TEAM(ePlotTeam).getMasterTeam() == GET_TEAM(eTargetTeam).getMasterTeam())
	{
		return TRUE;
	}
	if (kTeam.canPeacefullyEnter(ePlotTeam) ||
		kTeam.isDisengage(ePlotTeam)) // advc.034
	{
		return TRUE;
	}

	if (kTeam.AI_getWarPlan(ePlotTeam) != NO_WARPLAN)
		return TRUE;

	return FALSE;
}


BOOL stepAdd(FAStarNode* parent, FAStarNode* node,
	int data, // advc (note): unused
	void const* pointer, FAStar* finder)
{
	if (data == ASNC_INITIALADD)
		node->m_iData1 = 0;
	else node->m_iData1 = (parent->m_iData1 + 1);
	FAssertMsg(node->m_iData1 >= 0, "invalid Index");
	return TRUE;
}


BOOL routeValid(FAStarNode* parent, FAStarNode* node,
	int data, // advc (note): unused
	void const* pointer, FAStar* finder)
{
	if (parent == NULL)
		return TRUE;

	CvPlot const& kNewPlot = GC.getMap().getPlot(node->m_iX, node->m_iY);
	PlayerTypes ePlayer = (PlayerTypes)gDLL->getFAStarIFace()->GetInfo(finder);
	if (!kNewPlot.isOwned() || kNewPlot.getTeam() == TEAMID(ePlayer))
	{
		if (kNewPlot.getRouteType() == GET_PLAYER(ePlayer).getBestRoute(&kNewPlot))
			return TRUE;
	}
	return FALSE;
}


BOOL borderValid(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder)
{
	if (parent == NULL)
		return TRUE;

	PlayerTypes ePlayer = (PlayerTypes)gDLL->getFAStarIFace()->GetInfo(finder);
	if (GC.getMap().getPlot(node->m_iX, node->m_iY).getTeam() == TEAMID(ePlayer))
		return TRUE;

	return FALSE;
}


BOOL areaValid(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder)
{
	if (parent == NULL)
		return TRUE;

	return ((GC.getMap().getPlot(parent->m_iX, parent->m_iY).isWater() ==
			GC.getMap().getPlot(node->m_iX, node->m_iY).isWater()) ? TRUE : FALSE);
	// (advc.030 takes care of this)
	// BETTER_BTS_AI_MOD, General AI, 10/02/09, jdog5000
	// BBAI TODO: Why doesn't this work to break water and ice into separate area?
	/*if (GC.getMap().plotSoren(parent->m_iX, parent->m_iY)->isWater() != GC.getMap().plotSoren(node->m_iX, node->m_iY)->isWater())
	return FALSE;
	// Ice blocks become their own area
	if (GC.getMap().plotSoren(parent->m_iX, parent->m_iY)->isWater() && GC.getMap().plotSoren(node->m_iX, node->m_iY)->isWater()) {
		if (GC.getMap().plotSoren(parent->m_iX, parent->m_iY)->isImpassable() != GC.getMap().plotSoren(node->m_iX, node->m_iY)->isImpassable())
			return FALSE;
	}
	return TRUE;*/
}


BOOL joinArea(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder)
{
	if (data == ASNL_ADDCLOSED)
	{
		CvMap const& kMap = GC.getMap();
		kMap.getPlot(node->m_iX, node->m_iY).setArea(
				kMap.getArea(gDLL->getFAStarIFace()->GetInfo(finder)));
	}
	return TRUE;
}


BOOL plotGroupValid(FAStarNode* parent, FAStarNode* node,
	int data, // advc (note): unused
	void const* pointer, FAStar* finder)
{
	//PROFILE_FUNC(); // advc.003o
	if (parent == NULL)
		return TRUE;

	CvPlot const& kOldPlot = GC.getMap().getPlot(parent->m_iX, parent->m_iY);
	CvPlot const& kNewPlot = GC.getMap().getPlot(node->m_iX, node->m_iY);

	PlayerTypes const ePlayer = (PlayerTypes)gDLL->getFAStarIFace()->GetInfo(finder);
	TeamTypes const eTeam = TEAMID(ePlayer);
	if (kOldPlot.isSamePlotGroup(kNewPlot, ePlayer) &&
		kNewPlot.isTradeNetwork(eTeam) &&
		kNewPlot.isTradeNetworkConnected(kOldPlot, eTeam))
	{
		return TRUE;
	}
	return FALSE;
}


BOOL countPlotGroup(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder)
{
	if (data == ASNL_ADDCLOSED)
		(*((int*)pointer))++;
	return TRUE;
}


BOOL potentialIrrigation(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder)
{
	if (parent == NULL)
		return TRUE;
	return (GC.getMap().getPlot(node->m_iX, node->m_iY).isPotentialIrrigation() ? TRUE : FALSE);
}


BOOL checkFreshWater(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder)
{
	if (data == ASNL_ADDCLOSED)
	{
		if (GC.getMap().getPlot(node->m_iX, node->m_iY).isFreshWater())
			*((bool *)pointer) = true;
	}
	return TRUE;
}


BOOL changeIrrigated(FAStarNode* parent, FAStarNode* node, int data, void const* pointer, FAStar* finder)
{
	if (data == ASNL_ADDCLOSED)
		GC.getMap().getPlot(node->m_iX, node->m_iY).setIrrigated(*((bool*)pointer));
	return TRUE;
}
