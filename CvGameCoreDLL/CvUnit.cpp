// unit.cpp

#include "CvGameCoreDLL.h"
#include "CvUnit.h"
#include "CvArea.h"
#include "CvPlot.h"
#include "CvCity.h"
#include "CvGlobals.h"
#include "CvGameCoreUtils.h"
#include "CvGameAI.h"
#include "CvMap.h"
#include "CvPlayerAI.h"
#include "CvRandom.h"
#include "CvTeamAI.h"
#include "CvGameCoreUtils.h"
#include "CyUnit.h"
#include "CyArgsList.h"
#include "CyPlot.h"
#include "CvDLLEntityIFaceBase.h"
#include "CvDLLInterfaceIFaceBase.h"
#include "CvDLLEngineIFaceBase.h"
#include "CvEventReporter.h"
#include "CvDLLPythonIFaceBase.h"
#include "CvDLLFAStarIFaceBase.h"
#include "CvInfos.h"
#include "FProfiler.h"
#include "CvPopupInfo.h"
#include "CvArtFileMgr.h"

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/24/10                                jdog5000      */
/*                                                                                              */
/* AI logging                                                                                   */
/************************************************************************************************/
#include "BetterBTSAI.h"
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

// Public Functions...


CvUnit::CvUnit()
{
	m_aiExtraDomainModifier = new int[NUM_DOMAIN_TYPES];

	m_pabHasPromotion = NULL;

	m_paiTerrainDoubleMoveCount = NULL;
	m_paiFeatureDoubleMoveCount = NULL;
	m_paiExtraTerrainAttackPercent = NULL;
	m_paiExtraTerrainDefensePercent = NULL;
	m_paiExtraFeatureAttackPercent = NULL;
	m_paiExtraFeatureDefensePercent = NULL;
	m_paiExtraUnitCombatModifier = NULL;

	CvDLLEntity::createUnitEntity(this);		// create and attach entity to unit

	reset(0, NO_UNIT, NO_PLAYER, true);
}


CvUnit::~CvUnit()
{
	if (!gDLL->GetDone() && GC.IsGraphicsInitialized())						// don't need to remove entity when the app is shutting down, or crash can occur
	{
		gDLL->getEntityIFace()->RemoveUnitFromBattle(this);
		CvDLLEntity::removeEntity();		// remove entity from engine
	}

	CvDLLEntity::destroyEntity();			// delete CvUnitEntity and detach from us

	uninit();

	SAFE_DELETE_ARRAY(m_aiExtraDomainModifier);
}

void CvUnit::reloadEntity()
{
	//destroy old entity
	if (!gDLL->GetDone() && GC.IsGraphicsInitialized())						// don't need to remove entity when the app is shutting down, or crash can occur
	{
		gDLL->getEntityIFace()->RemoveUnitFromBattle(this);
		CvDLLEntity::removeEntity();		// remove entity from engine
	}
	
	CvDLLEntity::destroyEntity();			// delete CvUnitEntity and detach from us

	//creat new one
	CvDLLEntity::createUnitEntity(this);		// create and attach entity to unit
	setupGraphical();
}


void CvUnit::init(int iID, UnitTypes eUnit, UnitAITypes eUnitAI, PlayerTypes eOwner, int iX, int iY, DirectionTypes eFacingDirection)
{
	CvWString szBuffer;
	//int iUnitName; // advc.003
	int iI, iJ;

	FAssert(NO_UNIT != eUnit);

	//--------------------------------
	// Init saved data
	reset(iID, eUnit, eOwner);

	if(eFacingDirection == NO_DIRECTION)
		m_eFacingDirection = DIRECTION_SOUTH;
	else
		m_eFacingDirection = eFacingDirection;

	//--------------------------------
	// Init containers

	//--------------------------------
	// Init pre-setup() data
	setXY(iX, iY, false, false);

	//--------------------------------
	// Init non-saved data
	setupGraphical();

	//--------------------------------
	// Init other game data
	plot()->updateCenterUnit();

	plot()->setFlagDirty(true);
	// <advc.003>
	CvGame& g = GC.getGameINLINE();
	// Was called iUnitName, which doesn't make any sense to me.
	int iCreated = g.getUnitCreatedCount(getUnitType()); // </advc.003>
	int iNumNames = m_pUnitInfo->getNumUnitNames();
	if (iCreated < iNumNames)
	{	// <advc.005b>
		/*  Skip every step'th name on average. Basic assumption: About half of
			the names are used in a long (space-race) game with 7 civs; therefore, skip every
			step=2 then. Adjust this to the (current) number of civs. */
		int alive = g.countCivPlayersAlive();
		if(alive <= 0) {
			FAssert(alive > 0);
			alive = 7;
		}
		int step = std::max(1, ::round(14.0 / alive));
		// This gives iRand an expected value of step
		int iRand = GC.getGameINLINE().getSorenRandNum(2 * step + 1, "advc.005b");
		/*  The index of the most recently used name isn't available; instead,
			take step times the number of previously used names in order to
			pick up roughly where we left off. */
		int iOffset = step * iCreated + iRand;
		// That's +8 in Medieval, +16 in Renaissance and +24 in Industrial or later
		iOffset += std::min(24, 8 * std::max(0, GC.getGame().getStartEra() - 1));
		bool nameSet = false;
		// If we run out of names, search backward
		if(iOffset >= iNumNames) {
			/*  The first couple are still somewhat random, but then just 
				pick them chronologically. */
			for(iI = iNumNames - 1 - iRand; iI >= 0; iI--) {
				CvWString szName = gDLL->getText(m_pUnitInfo->getUnitNames(iI));
				// Copied from below
				if(!GC.getGameINLINE().isGreatPersonBorn(szName)) {
					setName(szName);
					nameSet = true;
					GC.getGameINLINE().addGreatPersonBornName(szName);
					break;
				}
			} // Otherwise, search forward
		} if(!nameSet) // </advc.005b>
		for (iI = 0; iI < iNumNames; iI++)
		{
			int iIndex = (iI + iOffset) % iNumNames;
			CvWString szName = gDLL->getText(m_pUnitInfo->getUnitNames(iIndex));
			if (!GC.getGameINLINE().isGreatPersonBorn(szName))
			{
				setName(szName);
				GC.getGameINLINE().addGreatPersonBornName(szName);
				break;
			}
		}
	}

	setGameTurnCreated(GC.getGameINLINE().getGameTurn());

	GC.getGameINLINE().incrementUnitCreatedCount(getUnitType());

	GC.getGameINLINE().incrementUnitClassCreatedCount((UnitClassTypes)(m_pUnitInfo->getUnitClassType()));
	GET_TEAM(getTeam()).changeUnitClassCount(((UnitClassTypes)(m_pUnitInfo->getUnitClassType())), 1);
	GET_PLAYER(getOwnerINLINE()).changeUnitClassCount(((UnitClassTypes)(m_pUnitInfo->getUnitClassType())), 1);

	GET_PLAYER(getOwnerINLINE()).changeExtraUnitCost(m_pUnitInfo->getExtraCost());

	if (m_pUnitInfo->getNukeRange() != -1)
	{
		GET_PLAYER(getOwnerINLINE()).changeNumNukeUnits(1);
	}

	if (m_pUnitInfo->isMilitarySupport())
	{
		GET_PLAYER(getOwnerINLINE()).changeNumMilitaryUnits(1);
	}

	GET_PLAYER(getOwnerINLINE()).changeAssets(m_pUnitInfo->getAssetValue());

	GET_PLAYER(getOwnerINLINE()).changePower(m_pUnitInfo->getPowerValue()
		/ (m_pUnitInfo->getDomainType() == DOMAIN_SEA ? 2 : 1) // advc.104e
	);

	// advc.104: To enable more differentiated tracking of power values
	GET_PLAYER(getOwnerINLINE()).warAndPeaceAI().getCache().reportUnitCreated(
			*m_pUnitInfo);

	for (iI = 0; iI < GC.getNumPromotionInfos(); iI++)
	{
		if (m_pUnitInfo->getFreePromotions(iI))
		{
			setHasPromotion(((PromotionTypes)iI), true);
		}
	}

	FAssertMsg((GC.getNumTraitInfos() > 0), "GC.getNumTraitInfos() is less than or equal to zero but is expected to be larger than zero in CvUnit::init");
	for (iI = 0; iI < GC.getNumTraitInfos(); iI++)
	{
		if (GET_PLAYER(getOwnerINLINE()).hasTrait((TraitTypes)iI))
		{
			for (iJ = 0; iJ < GC.getNumPromotionInfos(); iJ++)
			{
				if (GC.getTraitInfo((TraitTypes) iI).isFreePromotion(iJ))
				{
					if ((getUnitCombatType() != NO_UNITCOMBAT) && GC.getTraitInfo((TraitTypes) iI).isFreePromotionUnitCombat(getUnitCombatType()))
					{
						setHasPromotion(((PromotionTypes)iJ), true);
					}
				}
			}
		}
	}

	if (NO_UNITCOMBAT != getUnitCombatType())
	{
		for (iJ = 0; iJ < GC.getNumPromotionInfos(); iJ++)
		{
			if (GET_PLAYER(getOwnerINLINE()).isFreePromotion(getUnitCombatType(), (PromotionTypes)iJ))
			{
				setHasPromotion(((PromotionTypes)iJ), true);
			}
		}
	}

	if (NO_UNITCLASS != getUnitClassType())
	{
		for (iJ = 0; iJ < GC.getNumPromotionInfos(); iJ++)
		{
			if (GET_PLAYER(getOwnerINLINE()).isFreePromotion(getUnitClassType(), (PromotionTypes)iJ))
			{
				setHasPromotion(((PromotionTypes)iJ), true);
			}
		}
	}

	if (getDomainType() == DOMAIN_LAND)
	{
		if (baseCombatStr() > 0)
		{
			if ((GC.getGameINLINE().getBestLandUnit() == NO_UNIT) || (baseCombatStr() > GC.getGameINLINE().getBestLandUnitCombat()))
			{
				GC.getGameINLINE().setBestLandUnit(getUnitType());
			}
		} 
	}

	if (getOwnerINLINE() == GC.getGameINLINE().getActivePlayer())
	{
		gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
	}

	if (isWorldUnitClass((UnitClassTypes)(m_pUnitInfo->getUnitClassType())))
	{
		for (iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_TEAM(getTeam()).isHasMet(GET_PLAYER((PlayerTypes)iI).getTeam()))
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_CREATED_UNIT", GET_PLAYER(getOwnerINLINE()).getNameKey(), getNameKey());
					gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_WONDER_UNIT_BUILD", MESSAGE_TYPE_MAJOR_EVENT, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_UNIT_TEXT"), getX_INLINE(), getY_INLINE(), true, true);
				}
				else
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_UNKNOWN_CREATED_UNIT", getNameKey());
					gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_WONDER_UNIT_BUILD", MESSAGE_TYPE_MAJOR_EVENT, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_UNIT_TEXT"));
				}
			}
		}

		szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_CREATED_UNIT", GET_PLAYER(getOwnerINLINE()).getNameKey(), getNameKey());
		GC.getGameINLINE().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getOwnerINLINE(), szBuffer, getX_INLINE(), getY_INLINE(), (ColorTypes)GC.getInfoTypeForString("COLOR_UNIT_TEXT"));
	}

	AI_init(eUnitAI);

	CvEventReporter::getInstance().unitCreated(this);
}


void CvUnit::uninit()
{
	SAFE_DELETE_ARRAY(m_pabHasPromotion);

	SAFE_DELETE_ARRAY(m_paiTerrainDoubleMoveCount);
	SAFE_DELETE_ARRAY(m_paiFeatureDoubleMoveCount);
	SAFE_DELETE_ARRAY(m_paiExtraTerrainAttackPercent);
	SAFE_DELETE_ARRAY(m_paiExtraTerrainDefensePercent);
	SAFE_DELETE_ARRAY(m_paiExtraFeatureAttackPercent);
	SAFE_DELETE_ARRAY(m_paiExtraFeatureDefensePercent);
	SAFE_DELETE_ARRAY(m_paiExtraUnitCombatModifier);
}


// FUNCTION: reset()
// Initializes data members that are serialized.
void CvUnit::reset(int iID, UnitTypes eUnit, PlayerTypes eOwner, bool bConstructorCall)
{
	int iI;

	//--------------------------------
	// Uninit class
	uninit();

	m_iID = iID;
	m_iGroupID = FFreeList::INVALID_INDEX;
	m_iHotKeyNumber = -1;
	m_iX = INVALID_PLOT_COORD;
	m_iY = INVALID_PLOT_COORD;
	m_iLastMoveTurn = 0;
	m_iReconX = INVALID_PLOT_COORD;
	m_iReconY = INVALID_PLOT_COORD;
	m_iGameTurnCreated = 0;
	m_iDamage = 0;
	m_iMoves = 0;
	m_iExperience = 0;
	m_iLevel = 1;
	m_iCargo = 0;
	m_iAttackPlotX = INVALID_PLOT_COORD;
	m_iAttackPlotY = INVALID_PLOT_COORD;
	m_iCombatTimer = 0;
	m_iCombatFirstStrikes = 0;
	m_iFortifyTurns = 0;
	m_iBlitzCount = 0;
	m_iAmphibCount = 0;
	m_iRiverCount = 0;
	m_iEnemyRouteCount = 0;
	m_iAlwaysHealCount = 0;
	m_iHillsDoubleMoveCount = 0;
	m_iImmuneToFirstStrikesCount = 0;
	m_iExtraVisibilityRange = 0;
	m_iExtraMoves = 0;
	m_iExtraMoveDiscount = 0;
	m_iExtraAirRange = 0;
	m_iExtraIntercept = 0;
	m_iExtraEvasion = 0;
	m_iExtraFirstStrikes = 0;
	m_iExtraChanceFirstStrikes = 0;
	m_iExtraWithdrawal = 0;
	m_iExtraCollateralDamage = 0;
	m_iExtraBombardRate = 0;
	m_iExtraEnemyHeal = 0;
	m_iExtraNeutralHeal = 0;
	m_iExtraFriendlyHeal = 0;
	m_iSameTileHeal = 0;
	m_iAdjacentTileHeal = 0;
	m_iExtraCombatPercent = 0;
	m_iExtraCityAttackPercent = 0;
	m_iExtraCityDefensePercent = 0;
	m_iExtraHillsAttackPercent = 0;
	m_iExtraHillsDefensePercent = 0;
	m_iRevoltProtection = 0;
	m_iCollateralDamageProtection = 0;
	m_iPillageChange = 0;
	m_iUpgradeDiscount = 0;
	m_iExperiencePercent = 0;
	m_iKamikazePercent = 0;
	m_eFacingDirection = DIRECTION_SOUTH;
	m_iImmobileTimer = 0;

	m_bMadeAttack = false;
	m_bMadeInterception = false;
	m_bPromotionReady = false;
	m_bDeathDelay = false;
	m_bCombatFocus = false;
	m_bInfoBarDirty = false;
	m_bBlockading = false;
	m_bAirCombat = false;

	m_eOwner = eOwner;
	m_eCapturingPlayer = NO_PLAYER;
	m_eUnitType = eUnit;
	m_pUnitInfo = (NO_UNIT != m_eUnitType) ? &GC.getUnitInfo(m_eUnitType) : NULL;
	m_iBaseCombat = (NO_UNIT != m_eUnitType) ? m_pUnitInfo->getCombat() : 0;
	m_eLeaderUnitType = NO_UNIT;
	m_iCargoCapacity = (NO_UNIT != m_eUnitType) ? m_pUnitInfo->getCargoSpace() : 0;

	m_combatUnit.reset();
	m_transportUnit.reset();

	for (iI = 0; iI < NUM_DOMAIN_TYPES; iI++)
	{
		m_aiExtraDomainModifier[iI] = 0;
	}

	m_szName.clear();
	m_szScriptData ="";
	initiallyVisible = true; // advc.102
	m_iLastReconTurn = -1; // advc.029

	if (!bConstructorCall)
	{
		FAssertMsg((0 < GC.getNumPromotionInfos()), "GC.getNumPromotionInfos() is not greater than zero but an array is being allocated in CvUnit::reset");
		m_pabHasPromotion = new bool[GC.getNumPromotionInfos()];
		for (iI = 0; iI < GC.getNumPromotionInfos(); iI++)
		{
			m_pabHasPromotion[iI] = false;
		}

		FAssertMsg((0 < GC.getNumTerrainInfos()), "GC.getNumTerrainInfos() is not greater than zero but a float array is being allocated in CvUnit::reset");
		m_paiTerrainDoubleMoveCount = new int[GC.getNumTerrainInfos()];
		m_paiExtraTerrainAttackPercent = new int[GC.getNumTerrainInfos()];
		m_paiExtraTerrainDefensePercent = new int[GC.getNumTerrainInfos()];
		for (iI = 0; iI < GC.getNumTerrainInfos(); iI++)
		{
			m_paiTerrainDoubleMoveCount[iI] = 0;
			m_paiExtraTerrainAttackPercent[iI] = 0;
			m_paiExtraTerrainDefensePercent[iI] = 0;
		}

		FAssertMsg((0 < GC.getNumFeatureInfos()), "GC.getNumFeatureInfos() is not greater than zero but a float array is being allocated in CvUnit::reset");
		m_paiFeatureDoubleMoveCount = new int[GC.getNumFeatureInfos()];
		m_paiExtraFeatureDefensePercent = new int[GC.getNumFeatureInfos()];
		m_paiExtraFeatureAttackPercent = new int[GC.getNumFeatureInfos()];
		for (iI = 0; iI < GC.getNumFeatureInfos(); iI++)
		{
			m_paiFeatureDoubleMoveCount[iI] = 0;
			m_paiExtraFeatureAttackPercent[iI] = 0;
			m_paiExtraFeatureDefensePercent[iI] = 0;
		}

		FAssertMsg((0 < GC.getNumUnitCombatInfos()), "GC.getNumUnitCombatInfos() is not greater than zero but an array is being allocated in CvUnit::reset");
		m_paiExtraUnitCombatModifier = new int[GC.getNumUnitCombatInfos()];
		for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
		{
			m_paiExtraUnitCombatModifier[iI] = 0;
		}

		AI_reset();
	}
}


//////////////////////////////////////
// graphical only setup
//////////////////////////////////////
void CvUnit::setupGraphical()
{
	if (!GC.IsGraphicsInitialized())
	{
		return;
	}

	CvDLLEntity::setup();

	if (getGroup()->getActivityType() == ACTIVITY_INTERCEPT)
	{
		airCircle(true);
	}
}


void CvUnit::convert(CvUnit* pUnit)
{
	CvPlot* pPlot = plot();

	for (int iI = 0; iI < GC.getNumPromotionInfos(); iI++)
	{
		setHasPromotion(((PromotionTypes)iI), (pUnit->isHasPromotion((PromotionTypes)iI) || m_pUnitInfo->getFreePromotions(iI)));
	}

	setGameTurnCreated(pUnit->getGameTurnCreated());
	setDamage(pUnit->getDamage());
	setMoves(pUnit->getMoves());

	setLevel(pUnit->getLevel());
	int iOldModifier = std::max(1, 100 + GET_PLAYER(pUnit->getOwnerINLINE()).getLevelExperienceModifier());
	int iOurModifier = std::max(1, 100 + GET_PLAYER(getOwnerINLINE()).getLevelExperienceModifier());
	setExperience(std::max(0, (pUnit->getExperience() * iOurModifier) / iOldModifier));

	setName(pUnit->getNameNoDesc());
	setLeaderUnitType(pUnit->getLeaderUnitType());

	CvUnit* pTransportUnit = pUnit->getTransportUnit();
	/* if (pTransportUnit != NULL)
	{
		pUnit->setTransportUnit(NULL);
		setTransportUnit(pTransportUnit);
	} */
	// K-Mod
	if (pTransportUnit != NULL)
		pUnit->setTransportUnit(NULL);
	setTransportUnit(pTransportUnit);
	// K-Mod end

	std::vector<CvUnit*> aCargoUnits;
	pUnit->getCargoUnits(aCargoUnits);
	for (uint i = 0; i < aCargoUnits.size(); ++i)
	{
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       10/30/09                     Mongoose & jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original BTS code
		aCargoUnits[i]->setTransportUnit(this);
*/
		// From Mongoose SDK
		// Check cargo types and capacity when upgrading transports
		if (cargoSpaceAvailable(aCargoUnits[i]->getSpecialUnitType(), aCargoUnits[i]->getDomainType()) > 0)
		{
			aCargoUnits[i]->setTransportUnit(this);
		}
		else
		{
			aCargoUnits[i]->setTransportUnit(NULL);
			aCargoUnits[i]->jumpToNearestValidPlot();
		}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
	}

	pUnit->kill(true);
}


// K-Mod. I've made some structural change to this function, for efficiency, clarity, and sanity.
void CvUnit::kill(bool bDelay, PlayerTypes ePlayer)
{
	PROFILE_FUNC();
	
	CvPlot* pPlot = plot();
	FAssert(pPlot);

	CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();
	while (pUnitNode)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = pPlot->nextUnitNode(pUnitNode);

		FAssert(pLoopUnit);
		if (pLoopUnit->getTransportUnit() == this)
		{
			if (pPlot->isValidDomainForLocation(*pLoopUnit))
			{
				pLoopUnit->setCapturingPlayer(NO_PLAYER);
			}

			pLoopUnit->kill(false, ePlayer);
		}
	}

	if (ePlayer != NO_PLAYER)
	{
		CvEventReporter::getInstance().unitKilled(this, ePlayer);

		if (NO_UNIT != getLeaderUnitType())
		{
			CvWString szBuffer;
			//szBuffer = gDLL->getText("TXT_KEY_MISC_GENERAL_KILLED", getNameKey());
			// <advc.004u> szBuffer now set inside the loop
			PlayerTypes ownerId = getOwner();
			// Don't see how the GG could have no owner, but let's be certain.
			if(ownerId != NO_PLAYER) { // </advc.004u>
				for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1)) // advc.003: Exclude barbs
				{	// <advc.003>
					if(!GET_PLAYER(i).isAlive())
						continue; // </advc.003>
					ColorTypes col = NO_COLOR;
					// <advc.004u>
					FAssert(ownerId != ePlayer);
					if(i == ownerId) {
						szBuffer = gDLL->getText("TXT_KEY_MISC_YOUR_GENERAL_KILLED", getNameKey(),
								GET_PLAYER(ePlayer).getCivilizationShortDescription());
						col = (ColorTypes)GC.getInfoTypeForString("COLOR_RED");
					}
					else if(i == ePlayer) {
						szBuffer = gDLL->getText("TXT_KEY_MISC_GENERAL_KILLED_BY_YOU", getNameKey(),
								GET_PLAYER(ownerId).getCivilizationShortDescription());
						col = (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN");
					}
					else if(TEAMREF(i).isHasMet(TEAMID(ownerId)) // advc.004u
							|| GET_PLAYER(i).isSpectator()) { // advc.127
						szBuffer = gDLL->getText("TXT_KEY_MISC_GENERAL_KILLED", getNameKey(),
								GET_PLAYER(ownerId).getCivilizationShortDescription(),
								GET_PLAYER(ePlayer).getCivilizationShortDescription());
						col = (ColorTypes)GC.getInfoTypeForString("COLOR_UNIT_TEXT");
					}
					bool isRev = plot()->isRevealed(TEAMID(i), false);
					// </advc.004u>
					gDLL->getInterfaceIFace()->addHumanMessage(i, false, GC.getEVENT_MESSAGE_TIME(), szBuffer,
							// advc.004u: Play the original sound when it does fit
							i == ownerId ? GC.getEraInfo(GET_PLAYER(i).getCurrentEra()).getAudioUnitDefeatScript() :
							"AS2D_INTERCEPTED", MESSAGE_TYPE_MAJOR_EVENT, // K-Mod (the other sound is not appropriate for most civs receiving the message.)
							// <advc.004u> Indicate location on map
							getButton(), col, isRev ? plot()->getX() : -1,
							isRev ? plot()->getY() : -1, isRev, isRev);
							// </advc.004u>
				}
			}
		}
	}

	finishMoves();

	if (bDelay)
	{
		startDelayedDeath();
		return;
	}

	if (isMadeAttack() && nukeRange() != -1)
	{
		CvPlot* pTarget = getAttackPlot();
		if (pTarget)
		{
			pTarget->nukeExplosion(nukeRange(), this);
			setAttackPlot(NULL, false);
		}
	}

	if (IsSelected())
	{
		if (gDLL->getInterfaceIFace()->getLengthSelectionList() == 1)
		{
			if (!(gDLL->getInterfaceIFace()->isFocused()) && !(gDLL->getInterfaceIFace()->isCitySelection()) && !(gDLL->getInterfaceIFace()->isDiploOrPopupWaiting()))
			{
				GC.getGameINLINE().updateSelectionList();
			}

			if (IsSelected())
			{
				GC.getGameINLINE().cycleSelectionGroups_delayed(1, false);
			}
			else
			{
				gDLL->getInterfaceIFace()->setDirty(SelectionCamera_DIRTY_BIT, true);
			}
		}
	}

	gDLL->getInterfaceIFace()->removeFromSelectionList(this);

	// XXX this is NOT a hack, without it, the game crashes.
	gDLL->getEntityIFace()->RemoveUnitFromBattle(this);

	//FAssertMsg(!isCombat(), "isCombat did not return false as expected");
	FAssert(!isFighting()); // K-Mod. With simultaneous turns, a unit can be captured while trying to execute an attack order. (eg. a bomber)

	setTransportUnit(NULL);

	setReconPlot(NULL);
	setBlockading(false);

	//FAssertMsg(getAttackPlot() == NULL, "The current unit instance's attack plot is expected to be NULL");
	FAssertMsg(getCombatUnit() == NULL, "The current unit instance's combat unit is expected to be NULL");

	GET_TEAM(getTeam()).changeUnitClassCount((UnitClassTypes)m_pUnitInfo->getUnitClassType(), -1);
	GET_PLAYER(getOwnerINLINE()).changeUnitClassCount((UnitClassTypes)m_pUnitInfo->getUnitClassType(), -1);

	GET_PLAYER(getOwnerINLINE()).changeExtraUnitCost(-(m_pUnitInfo->getExtraCost()));

	if (m_pUnitInfo->getNukeRange() != -1)
	{
		GET_PLAYER(getOwnerINLINE()).changeNumNukeUnits(-1);
	}

	if (m_pUnitInfo->isMilitarySupport())
	{
		GET_PLAYER(getOwnerINLINE()).changeNumMilitaryUnits(-1);
	}

	GET_PLAYER(getOwnerINLINE()).changeAssets(-(m_pUnitInfo->getAssetValue()));

	GET_PLAYER(getOwnerINLINE()).changePower(-(m_pUnitInfo->getPowerValue()
		/ (m_pUnitInfo->getDomainType() == DOMAIN_SEA ? 2 : 1) // advc.104e
		));

	// advc.104: To enable more differentiated tracking of power values
	GET_PLAYER(getOwnerINLINE()).warAndPeaceAI().getCache().reportUnitDestroyed(
		 *m_pUnitInfo);

	GET_PLAYER(getOwnerINLINE()).AI_changeNumAIUnits(AI_getUnitAIType(), -1);

	PlayerTypes eOwner = getOwnerINLINE();
	PlayerTypes eCapturingPlayer = getCapturingPlayer();
	UnitTypes eCaptureUnitType = ((eCapturingPlayer != NO_PLAYER) ? getCaptureUnitType(GET_PLAYER(eCapturingPlayer).getCivilizationType()) : NO_UNIT);

	setXY(INVALID_PLOT_COORD, INVALID_PLOT_COORD, true);

	joinGroup(NULL, false, false);

	CvEventReporter::getInstance().unitLost(this);

	GET_PLAYER(getOwnerINLINE()).deleteUnit(getID());

	if ((eCapturingPlayer != NO_PLAYER) && (eCaptureUnitType != NO_UNIT) && !(GET_PLAYER(eCapturingPlayer).isBarbarian()))
	{
		if (GET_PLAYER(eCapturingPlayer).isHuman() || GET_PLAYER(eCapturingPlayer).AI_captureUnit(eCaptureUnitType, pPlot) || 0 == GC.getDefineINT("AI_CAN_DISBAND_UNITS"))
		{
			CvUnit* pkCapturedUnit = GET_PLAYER(eCapturingPlayer).initUnit(eCaptureUnitType, pPlot->getX_INLINE(), pPlot->getY_INLINE());

			if (pkCapturedUnit != NULL)
			{
				CvWString szBuffer;
				szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_CAPTURED_UNIT", GC.getUnitInfo(eCaptureUnitType).getTextKeyWide());
				gDLL->getInterfaceIFace()->addHumanMessage(eCapturingPlayer, true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_UNITCAPTURE", MESSAGE_TYPE_INFO, pkCapturedUnit->getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pPlot->getX_INLINE(), pPlot->getY_INLINE());

				// Add a captured mission
				if (pPlot->isActiveVisible(false)) // K-Mod
				{
					CvMissionDefinition kMission;
					kMission.setMissionTime(GC.getMissionInfo(MISSION_CAPTURED).getTime() * gDLL->getSecsPerTurn());
					kMission.setUnit(BATTLE_UNIT_ATTACKER, pkCapturedUnit);
					kMission.setUnit(BATTLE_UNIT_DEFENDER, NULL);
					kMission.setPlot(pPlot);
					kMission.setMissionType(MISSION_CAPTURED);
					gDLL->getEntityIFace()->AddMission(&kMission);
				}

				pkCapturedUnit->finishMoves();

				if (!GET_PLAYER(eCapturingPlayer).isHuman())
				{
					CvPlot* pPlot = pkCapturedUnit->plot();
					if (pPlot && !pPlot->isCity(false))
					{
						if (GET_PLAYER(eCapturingPlayer).AI_getPlotDanger(pPlot) && GC.getDefineINT("AI_CAN_DISBAND_UNITS"))
						{
							//pkCapturedUnit->kill(false);
							pkCapturedUnit->scrap(); // K-Mod. roughly the same thing, but this is more appropriate.
						}
					}
				}
			}
		}
	}
}


void CvUnit::NotifyEntity(MissionTypes eMission)
{
	gDLL->getEntityIFace()->NotifyEntity(getUnitEntity(), eMission);
}


void CvUnit::doTurn()
{
	PROFILE("CvUnit::doTurn()")

	FAssertMsg(!isDead(), "isDead did not return false as expected");
	FAssertMsg(getGroup() != NULL, "getGroup() is not expected to be equal with NULL");

	testPromotionReady();

	if (isBlockading())
	{
		collectBlockadeGold();
	}

	if (isSpy() && isIntruding() && !isCargo())
	{
		TeamTypes eTeam = plot()->getTeam();
		if (NO_TEAM != eTeam)
		{
			if (GET_TEAM(getTeam()).isOpenBorders(eTeam))
			{
				testSpyIntercepted(plot()->getOwnerINLINE(), false, GC.getDefineINT("ESPIONAGE_SPY_NO_INTRUDE_INTERCEPT_MOD"));
			}
			else
			{
				testSpyIntercepted(plot()->getOwnerINLINE(), false, GC.getDefineINT("ESPIONAGE_SPY_INTERCEPT_MOD"));
			}
		}
	}

	if (baseCombatStr() > 0)
	{
		FeatureTypes eFeature = plot()->getFeatureType();
		if (NO_FEATURE != eFeature)
		{
			if (0 != GC.getFeatureInfo(eFeature).getTurnDamage())
			{
				changeDamage(GC.getFeatureInfo(eFeature).getTurnDamage(), NO_PLAYER);
			}
		}
	}

	if (hasMoved())
	{
		if (isAlwaysHeal())
		{
			doHeal();
		}
	}
	else
	{
		if (isHurt())
		{
			doHeal();
		}

		if (!isCargo())
		{
			changeFortifyTurns(1);
		}
	}

	changeImmobileTimer(-1);

	setMadeAttack(false);
	setMadeInterception(false);

	//setReconPlot(NULL); // advc.029: Handled at end of turn now

	setMoves(0);
}

// <advc.029>
void CvUnit::doTurnPost() {

	if(GC.getGameINLINE().getGameTurn() > m_iLastReconTurn)
		setReconPlot(NULL);
} // </advc.029>


void CvUnit::updateAirStrike(CvPlot* pPlot, bool bQuick, bool bFinish)
{
	bool bVisible = false;

	if (!bFinish)
	{
		if (isFighting())
		{
			return;
		}

		if (!bQuick)
		{
			bVisible = isCombatVisible(NULL);
		}

		if (!airStrike(pPlot))
		{
			return;
		}

		if (bVisible)
		{
			CvAirMissionDefinition kAirMission;
			kAirMission.setMissionType(MISSION_AIRSTRIKE);
			kAirMission.setUnit(BATTLE_UNIT_ATTACKER, this);
			kAirMission.setUnit(BATTLE_UNIT_DEFENDER, NULL);
			kAirMission.setDamage(BATTLE_UNIT_DEFENDER, 0);
			kAirMission.setDamage(BATTLE_UNIT_ATTACKER, 0);
			kAirMission.setPlot(pPlot);
			setCombatTimer(GC.getMissionInfo(MISSION_AIRSTRIKE).getTime());
			GC.getGameINLINE().incrementTurnTimer(getCombatTimer());
			kAirMission.setMissionTime(getCombatTimer() * gDLL->getSecsPerTurn());

			if (pPlot->isActiveVisible(false))
			{
				gDLL->getEntityIFace()->AddMission(&kAirMission);
			}

			return;
		}
	}

	CvUnit *pDefender = getCombatUnit();
	if (pDefender != NULL)
	{
		pDefender->setCombatUnit(NULL);
	}
	setCombatUnit(NULL);
	setAttackPlot(NULL, false);

	getGroup()->clearMissionQueue();

	if (isSuicide() && !isDead())
	{
		kill(true);
	}
}

void CvUnit::resolveAirCombat(CvUnit* pInterceptor, CvPlot* pPlot, CvAirMissionDefinition& kBattle)
{
	CvWString szBuffer;

	int iTheirStrength = (DOMAIN_AIR == pInterceptor->getDomainType() ? pInterceptor->airCurrCombatStr(this) : pInterceptor->currCombatStr(NULL, NULL));
	int iOurStrength = (DOMAIN_AIR == getDomainType() ? airCurrCombatStr(pInterceptor) : currCombatStr(NULL, NULL));
	int iTotalStrength = iOurStrength + iTheirStrength;
	if (0 == iTotalStrength)
	{
		FAssert(false);
		return;
	}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/19/08	Roland J & jdog5000	*/
/* 																			*/
/* 	Combat mechanics														*/
/********************************************************************************/
	/*
	int iOurOdds = (100 * iOurStrength) / std::max(1, iTotalStrength);

	int iOurRoundDamage = (pInterceptor->currInterceptionProbability() * GC.getDefineINT("MAX_INTERCEPTION_DAMAGE")) / 100;
	int iTheirRoundDamage = (currInterceptionProbability() * GC.getDefineINT("MAX_INTERCEPTION_DAMAGE")) / 100;
	if (getDomainType() == DOMAIN_AIR)
	{
		iTheirRoundDamage = std::max(GC.getDefineINT("MIN_INTERCEPTION_DAMAGE"), iTheirRoundDamage);
	}

	//original BTS code
	int iTheirDamage = 0;
	int iOurDamage = 0;

	for (int iRound = 0; iRound < GC.getDefineINT("INTERCEPTION_MAX_ROUNDS"); ++iRound)
	*/
	// For air v air, more rounds and factor in strength for per round damage
	int iOurOdds = (100 * iOurStrength) / std::max(1, iTotalStrength);
	int iMaxRounds = 0;
	int iOurRoundDamage = 0;
	int iTheirRoundDamage = 0;

	// Air v air is more like standard comabt
	// Round damage in this case will now depend on strength and interception probability
	if( GC.getBBAI_AIR_COMBAT() && (DOMAIN_AIR == pInterceptor->getDomainType() && DOMAIN_AIR == getDomainType()) )
	{
		int iBaseDamage = GC.getDefineINT("AIR_COMBAT_DAMAGE");
		int iOurFirepower = ((airMaxCombatStr(pInterceptor) + iOurStrength + 1) / 2);
		int iTheirFirepower = ((pInterceptor->airMaxCombatStr(this) + iTheirStrength + 1) / 2);

		int iStrengthFactor = ((iOurFirepower + iTheirFirepower + 1) / 2);

		int iTheirInterception = std::max(pInterceptor->maxInterceptionProbability(),2*GC.getDefineINT("MIN_INTERCEPTION_DAMAGE"));
		int iOurInterception = std::max(maxInterceptionProbability(),2*GC.getDefineINT("MIN_INTERCEPTION_DAMAGE"));

		iOurRoundDamage = std::max(1, ((iBaseDamage * (iTheirFirepower + iStrengthFactor) * iTheirInterception) / ((iOurFirepower + iStrengthFactor) * 100)));
		iTheirRoundDamage = std::max(1, ((iBaseDamage * (iOurFirepower + iStrengthFactor) * iOurInterception) / ((iTheirFirepower + iStrengthFactor) * 100)));

		iMaxRounds = 2*GC.getDefineINT("INTERCEPTION_MAX_ROUNDS") - 1;
	}
	else
	{
		iOurRoundDamage = (pInterceptor->currInterceptionProbability() * GC.getDefineINT("MAX_INTERCEPTION_DAMAGE")) / 100;
		iTheirRoundDamage = (currInterceptionProbability() * GC.getDefineINT("MAX_INTERCEPTION_DAMAGE")) / 100;
		if (getDomainType() == DOMAIN_AIR)
		{
			iTheirRoundDamage = std::max(GC.getDefineINT("MIN_INTERCEPTION_DAMAGE"), iTheirRoundDamage);
		}

		iMaxRounds = GC.getDefineINT("INTERCEPTION_MAX_ROUNDS");
	}

	int iTheirDamage = 0;
	int iOurDamage = 0;

	for (int iRound = 0; iRound < iMaxRounds; ++iRound)
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/
	{
		if (GC.getGameINLINE().getSorenRandNum(100, "Air combat") < iOurOdds)
		{
			if (DOMAIN_AIR == pInterceptor->getDomainType())
			{
				iTheirDamage += iTheirRoundDamage;
				pInterceptor->changeDamage(iTheirRoundDamage, getOwnerINLINE());
				if (pInterceptor->isDead())
				{
					break;
				}
			}
		}
		else
		{
			iOurDamage += iOurRoundDamage;
			changeDamage(iOurRoundDamage, pInterceptor->getOwnerINLINE());
			if (isDead())
			{
				break;
			}
		}
	}

	if (isDead())
	{
		if (iTheirRoundDamage > 0)
		{
			int iExperience = attackXPValue();
			iExperience = (iExperience * iOurStrength) / std::max(1, iTheirStrength);
			iExperience = range(iExperience, GC.getDefineINT("MIN_EXPERIENCE_PER_COMBAT"), GC.getDefineINT("MAX_EXPERIENCE_PER_COMBAT"));
			pInterceptor->changeExperience(iExperience, maxXPValue(), true, pPlot->getOwnerINLINE() == pInterceptor->getOwnerINLINE(), !isBarbarian());
		}
	}
	else if (pInterceptor->isDead())
	{
		int iExperience = pInterceptor->defenseXPValue();
		iExperience = (iExperience * iTheirStrength) / std::max(1, iOurStrength);
		iExperience = range(iExperience, GC.getDefineINT("MIN_EXPERIENCE_PER_COMBAT"), GC.getDefineINT("MAX_EXPERIENCE_PER_COMBAT"));
		changeExperience(iExperience, pInterceptor->maxXPValue(), true, pPlot->getOwnerINLINE() == getOwnerINLINE(), !pInterceptor->isBarbarian());
	}
	else if (iOurDamage > 0)
	{
		if (iTheirRoundDamage > 0)
		{
			pInterceptor->changeExperience(GC.getDefineINT("EXPERIENCE_FROM_WITHDRAWL"), maxXPValue(), true, pPlot->getOwnerINLINE() == pInterceptor->getOwnerINLINE(), !isBarbarian());
		}
	}
	else if (iTheirDamage > 0)
	{
		changeExperience(GC.getDefineINT("EXPERIENCE_FROM_WITHDRAWL"), pInterceptor->maxXPValue(), true, pPlot->getOwnerINLINE() == getOwnerINLINE(), !pInterceptor->isBarbarian());
	}

	kBattle.setDamage(BATTLE_UNIT_ATTACKER, iOurDamage);
	kBattle.setDamage(BATTLE_UNIT_DEFENDER, iTheirDamage);
}


void CvUnit::updateAirCombat(bool bQuick)
{
	CvUnit* pInterceptor = NULL;
	bool bFinish = false;

	FAssert(getDomainType() == DOMAIN_AIR || getDropRange() > 0);

	if (getCombatTimer() > 0)
	{
		changeCombatTimer(-1);

		if (getCombatTimer() > 0)
		{
			return;
		}
		else
		{
			bFinish = true;
		}
	}

	CvPlot* pPlot = getAttackPlot();
	if (pPlot == NULL)
	{
		return;
	}

	if (bFinish)
	{
		pInterceptor = getCombatUnit();
	}
	else
	{
		pInterceptor = bestInterceptor(pPlot);
	}


	if (pInterceptor == NULL)
	{
		setAttackPlot(NULL, false);
		setCombatUnit(NULL);

		getGroup()->clearMissionQueue();

		return;
	}

	//check if quick combat
	bool bVisible = false;
	if (!bQuick)
	{
		bVisible = isCombatVisible(pInterceptor);
	}

	//if not finished and not fighting yet, set up combat damage and mission
	if (!bFinish)
	{
		if (!isFighting())
		{
			//if (plot()->isFighting() || pPlot->isFighting())
			// K-Mod. I don't think it matters if the plot we're on is fighting already - but the interceptor needs to be available to fight!
			if (pPlot->isFighting() || pInterceptor->isFighting())
			{
				return;
			}

			setMadeAttack(true);

			setCombatUnit(pInterceptor, true);
			pInterceptor->setCombatUnit(this, false);
		}

		FAssertMsg(pInterceptor != NULL, "Defender is not assigned a valid value");

		FAssertMsg(plot()->isFighting(), "Current unit instance plot is not fighting as expected");
		FAssertMsg(pInterceptor->plot()->isFighting(), "pPlot is not fighting as expected");

		CvAirMissionDefinition kAirMission;
		if (DOMAIN_AIR != getDomainType())
		{
			kAirMission.setMissionType(MISSION_PARADROP);
		}
		else
		{
			kAirMission.setMissionType(MISSION_AIRSTRIKE);
		}
		kAirMission.setUnit(BATTLE_UNIT_ATTACKER, this);
		kAirMission.setUnit(BATTLE_UNIT_DEFENDER, pInterceptor);

		resolveAirCombat(pInterceptor, pPlot, kAirMission);

		if (!bVisible)
		{
			bFinish = true;
		}
		else
		{
			kAirMission.setPlot(pPlot);
			kAirMission.setMissionTime(GC.getMissionInfo(MISSION_AIRSTRIKE).getTime() * gDLL->getSecsPerTurn());
			setCombatTimer(GC.getMissionInfo(MISSION_AIRSTRIKE).getTime());
			GC.getGameINLINE().incrementTurnTimer(getCombatTimer());

			if (pPlot->isActiveVisible(false))
			{
				gDLL->getEntityIFace()->AddMission(&kAirMission);
			}
		}

		changeMoves(GC.getMOVE_DENOMINATOR());
		if (DOMAIN_AIR != pInterceptor->getDomainType())
		{
			pInterceptor->setMadeInterception(true);
		}

		if (isDead())
		{
			CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_SHOT_DOWN_ENEMY", pInterceptor->getNameKey(), getNameKey(), getVisualCivAdjective(pInterceptor->getTeam()));
			gDLL->getInterfaceIFace()->addHumanMessage(pInterceptor->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_INTERCEPT", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true, true);

			szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_UNIT_SHOT_DOWN", getNameKey(), pInterceptor->getNameKey());
			gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_INTERCEPTED", MESSAGE_TYPE_INFO, pInterceptor->getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE());
		}
		else if (kAirMission.getDamage(BATTLE_UNIT_ATTACKER) > 0)
		{
			CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_HURT_ENEMY_AIR", pInterceptor->getNameKey(), getNameKey(),
				    kAirMission.getDamage(BATTLE_UNIT_ATTACKER), // advc.004g
					getVisualCivAdjective(pInterceptor->getTeam()));
			gDLL->getInterfaceIFace()->addHumanMessage(pInterceptor->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_INTERCEPT", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true, true);

			szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_AIR_UNIT_HURT", getNameKey(), pInterceptor->getNameKey(),
				    kAirMission.getDamage(BATTLE_UNIT_ATTACKER)); // advc.004g
			gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_INTERCEPTED", MESSAGE_TYPE_INFO, pInterceptor->getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE());
		}

		if (pInterceptor->isDead())
		{
			CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_SHOT_DOWN_ENEMY", getNameKey(), pInterceptor->getNameKey(), pInterceptor->getVisualCivAdjective(getTeam()));
			gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_INTERCEPT", MESSAGE_TYPE_INFO, pInterceptor->getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true, true);

			szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_UNIT_SHOT_DOWN", pInterceptor->getNameKey(), getNameKey());
			gDLL->getInterfaceIFace()->addHumanMessage(pInterceptor->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_INTERCEPTED", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE());
		}
		else if (kAirMission.getDamage(BATTLE_UNIT_DEFENDER) > 0)
		{
			CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_DAMAGED_ENEMY_AIR", getNameKey(), pInterceptor->getNameKey(),
				    kAirMission.getDamage(BATTLE_UNIT_DEFENDER), // advc.004g
					pInterceptor->getVisualCivAdjective(getTeam()));
			gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_INTERCEPT", MESSAGE_TYPE_INFO, pInterceptor->getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true, true);

			szBuffer = gDLL->getText("TXT_KEY_MISC_YOUR_AIR_UNIT_DAMAGED", pInterceptor->getNameKey(), getNameKey(),
				    kAirMission.getDamage(BATTLE_UNIT_DEFENDER)); // advc.004g
			gDLL->getInterfaceIFace()->addHumanMessage(pInterceptor->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_INTERCEPTED", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE());
		}

		if (0 == kAirMission.getDamage(BATTLE_UNIT_ATTACKER) + kAirMission.getDamage(BATTLE_UNIT_DEFENDER))
		{
			CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_ABORTED_ENEMY_AIR", pInterceptor->getNameKey(), getNameKey(), getVisualCivAdjective(getTeam()));
			gDLL->getInterfaceIFace()->addHumanMessage(pInterceptor->getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_INTERCEPT", MESSAGE_TYPE_INFO, pInterceptor->getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true, true);

			szBuffer = gDLL->getText("TXT_KEY_MISC_YOUR_AIR_UNIT_ABORTED", getNameKey(), pInterceptor->getNameKey());
			gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_INTERCEPTED", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE());
		}
	}

	if (bFinish)
	{
		setAttackPlot(NULL, false);
		setCombatUnit(NULL);
		pInterceptor->setCombatUnit(NULL);

		if (!isDead() && isSuicide())
		{
			kill(true);
		}
	}
}

//#define LOG_COMBAT_OUTCOMES // K-Mod -- this makes the game log the odds and outcomes of every battle, to help verify the accuracy of the odds calculation.

// K-Mod. I've edited this function so that it handles the battle planning internally rather than feeding details back to the caller.
void CvUnit::resolveCombat(CvUnit* pDefender, CvPlot* pPlot, bool bVisible)
{
#ifdef LOG_COMBAT_OUTCOMES
	int iLoggedOdds = getCombatOdds(this, pDefender);
	iLoggedOdds += (1000 - iLoggedOdds)*withdrawalProbability()/100;
#endif

	// K-Mod. Initialize battle info.
	// Note: kBattle is only relevant if we are going to show the battle animation.
	CvBattleDefinition kBattle;
	if (bVisible)
	{
		kBattle.setUnit(BATTLE_UNIT_ATTACKER, this);
		kBattle.setUnit(BATTLE_UNIT_DEFENDER, pDefender);
		kBattle.setDamage(BATTLE_UNIT_ATTACKER, BATTLE_TIME_BEGIN, getDamage());
		kBattle.setDamage(BATTLE_UNIT_DEFENDER, BATTLE_TIME_BEGIN, pDefender->getDamage());
	}
	std::vector<int> combat_log; // positive number for attacker hitting the defender, negative numbers for defender hitting the attacker.
	// K-Mod end

	CombatDetails cdAttackerDetails;
	CombatDetails cdDefenderDetails;

	int iAttackerStrength = currCombatStr(NULL, NULL, &cdAttackerDetails);
	int iAttackerFirepower = currFirepower(NULL, NULL);
	int iDefenderStrength;
	int iAttackerDamage;
	int iDefenderDamage;
	int iDefenderOdds;

	getDefenderCombatValues(*pDefender, pPlot, iAttackerStrength, iAttackerFirepower, iDefenderOdds, iDefenderStrength, iAttackerDamage, iDefenderDamage, &cdDefenderDetails);
	int iAttackerKillOdds = iDefenderOdds * (100 - withdrawalProbability()) / 100;

	if (isHuman() || pDefender->isHuman())
	{
		//Added ST
		CyArgsList pyArgsCD;
		pyArgsCD.add(gDLL->getPythonIFace()->makePythonObject(&cdAttackerDetails));
		pyArgsCD.add(gDLL->getPythonIFace()->makePythonObject(&cdDefenderDetails));
		pyArgsCD.add(getCombatOdds(this, pDefender));
		CvEventReporter::getInstance().genericEvent("combatLogCalc", pyArgsCD.makeFunctionArgs());
	}

	collateralCombat(pPlot, pDefender);

	while (true)
	{
		if (GC.getGameINLINE().getSorenRandNum(GC.getCOMBAT_DIE_SIDES(), "Combat") < iDefenderOdds)
		{
			if (getCombatFirstStrikes() == 0)
			{
				if (getDamage() + iAttackerDamage >= maxHitPoints() && GC.getGameINLINE().getSorenRandNum(100, "Withdrawal") < withdrawalProbability())
				{
					flankingStrikeCombat(pPlot, iAttackerStrength, iAttackerFirepower, iAttackerKillOdds, iDefenderDamage, pDefender);

					changeExperience(GC.getDefineINT("EXPERIENCE_FROM_WITHDRAWL"), pDefender->maxXPValue(), true, pPlot->getOwnerINLINE() == getOwnerINLINE(), !pDefender->isBarbarian());
					combat_log.push_back(0); // K-Mod
					break;
				}

				changeDamage(iAttackerDamage, pDefender->getOwnerINLINE());
				combat_log.push_back(-iAttackerDamage); // K-Mod

				/* if (pDefender->getCombatFirstStrikes() > 0 && pDefender->isRanged())
				{
					kBattle.addFirstStrikes(BATTLE_UNIT_DEFENDER, 1);
					kBattle.addDamage(BATTLE_UNIT_ATTACKER, BATTLE_TIME_RANGED, iAttackerDamage);
				} */
				// K-Mod. (I don't think this stuff is actually used, but I want to do it my way, just in case.)
				if (pDefender->getCombatFirstStrikes() > 0)
					kBattle.addFirstStrikes(BATTLE_UNIT_DEFENDER, 1);
				// K-Mod end

				cdAttackerDetails.iCurrHitPoints = currHitPoints();

				if (isHuman() || pDefender->isHuman())
				{
					CyArgsList pyArgs;
					pyArgs.add(gDLL->getPythonIFace()->makePythonObject(&cdAttackerDetails));
					pyArgs.add(gDLL->getPythonIFace()->makePythonObject(&cdDefenderDetails));
					pyArgs.add(1);
					pyArgs.add(iAttackerDamage);
					CvEventReporter::getInstance().genericEvent("combatLogHit", pyArgs.makeFunctionArgs());
				}
			}
			// K-Mod. Track the free-strike misses, to use for choreographing the battle animation.
			else if (bVisible && !combat_log.empty())
				combat_log.push_back(0);
			// K-Mod end
		}
		else
		{
			if (pDefender->getCombatFirstStrikes() == 0)
			{
				if (std::min(GC.getMAX_HIT_POINTS(), pDefender->getDamage() + iDefenderDamage) > combatLimit())
				{
					changeExperience(GC.getDefineINT("EXPERIENCE_FROM_WITHDRAWL"), pDefender->maxXPValue(), true, pPlot->getOwnerINLINE() == getOwnerINLINE(), !pDefender->isBarbarian());
					combat_log.push_back(combatLimit() - pDefender->getDamage()); // K-Mod
					pDefender->setDamage(combatLimit(), getOwnerINLINE());
					break;
				}

				pDefender->changeDamage(iDefenderDamage, getOwnerINLINE());
				combat_log.push_back(iDefenderDamage); // K-Mod

				/* if (getCombatFirstStrikes() > 0 && isRanged())
				{
					kBattle.addFirstStrikes(BATTLE_UNIT_ATTACKER, 1);
					kBattle.addDamage(BATTLE_UNIT_DEFENDER, BATTLE_TIME_RANGED, iDefenderDamage);
				} */
				// K-Mod
				if (getCombatFirstStrikes() > 0)
					kBattle.addFirstStrikes(BATTLE_UNIT_ATTACKER, 1);
				// K-Mod end

				cdDefenderDetails.iCurrHitPoints=pDefender->currHitPoints();

				if (isHuman() || pDefender->isHuman())
				{
					CyArgsList pyArgs;
					pyArgs.add(gDLL->getPythonIFace()->makePythonObject(&cdAttackerDetails));
					pyArgs.add(gDLL->getPythonIFace()->makePythonObject(&cdDefenderDetails));
					pyArgs.add(0);
					pyArgs.add(iDefenderDamage);
					CvEventReporter::getInstance().genericEvent("combatLogHit", pyArgs.makeFunctionArgs());
				}
			}
			// K-Mod
			else if (bVisible && !combat_log.empty())
				combat_log.push_back(0);
			// K-Mod end
		}

		if (getCombatFirstStrikes() > 0)
		{
			changeCombatFirstStrikes(-1);
		}

		if (pDefender->getCombatFirstStrikes() > 0)
		{
			pDefender->changeCombatFirstStrikes(-1);
		}

		if (isDead() || pDefender->isDead())
		{
			if (isDead())
			{
				int iExperience = defenseXPValue();
				iExperience = ((iExperience * iAttackerStrength) / iDefenderStrength);
				iExperience = range(iExperience, GC.getDefineINT("MIN_EXPERIENCE_PER_COMBAT"), GC.getDefineINT("MAX_EXPERIENCE_PER_COMBAT")
						- (isBarbarian() ? 4 : 0)); // advc.312
				pDefender->changeExperience(iExperience, maxXPValue(), true, pPlot->getOwnerINLINE() == pDefender->getOwnerINLINE(), !isBarbarian());
			}
			else
			{
				flankingStrikeCombat(pPlot, iAttackerStrength, iAttackerFirepower, iAttackerKillOdds, iDefenderDamage, pDefender);

				int iExperience = pDefender->attackXPValue();
				iExperience = ((iExperience * iDefenderStrength) / iAttackerStrength);
				iExperience = range(iExperience, GC.getDefineINT("MIN_EXPERIENCE_PER_COMBAT"), GC.getDefineINT("MAX_EXPERIENCE_PER_COMBAT")
						/ (pDefender->isBarbarian() ? 2 : 1)); // advc.312
				changeExperience(iExperience, pDefender->maxXPValue(), true, pPlot->getOwnerINLINE() == getOwnerINLINE(), !pDefender->isBarbarian());
			}

			break;
		}
	}

	// K-Mod. Finalize battle info and start the animation.
	if (bVisible)
	{
		kBattle.setDamage(BATTLE_UNIT_ATTACKER, BATTLE_TIME_END, getDamage());
		kBattle.setDamage(BATTLE_UNIT_DEFENDER, BATTLE_TIME_END, pDefender->getDamage());
		kBattle.setAdvanceSquare(canAdvance(pPlot, 1));

		// note: BATTLE_TIME_RANGED damage is now set inside planBattle; (not that it actually does anything...)

		int iTurns = planBattle(kBattle, combat_log);
		kBattle.setMissionTime(iTurns * gDLL->getSecsPerTurn());
		setCombatTimer(iTurns);

		GC.getGameINLINE().incrementTurnTimer(getCombatTimer()); // additional time for multiplayer turn timer.

		if (pPlot->isActiveVisible(false))
		{
			ExecuteMove(0.5f, true);
			gDLL->getEntityIFace()->AddMission(&kBattle);
		}
	}

#ifdef LOG_COMBAT_OUTCOMES
	if (!isBarbarian() && !pDefender->isBarbarian()) // don't log barb battles, because they have special rules.
	{
		TCHAR message[20];
		_snprintf(message, 20, "%.2f\t%d\n", (float)iLoggedOdds/1000, isDead() ? 0 : 1);
		gDLL->logMsg("combat.txt", message ,false, false);
	}
#endif
}


void CvUnit::updateCombat(bool bQuick)
{
	CvWString szBuffer;

	bool bFinish = false;
	bool bVisible = false;

	if (getCombatTimer() > 0)
	{
		FAssert(getCombatUnit() && getCombatUnit()->getAttackPlot() == NULL); // K-Mod
		changeCombatTimer(-1);

		if (getCombatTimer() > 0)
		{
			return;
		}
		else
		{
			bFinish = true;
		}
	}

	CvPlot* pPlot = getAttackPlot();

	if (pPlot == NULL)
	{
		return;
	}

	if (getDomainType() == DOMAIN_AIR)
	{
		updateAirStrike(pPlot, bQuick, bFinish);
		return;
	}

	CvUnit* pDefender = NULL;
	if (bFinish)
	{
		pDefender = getCombatUnit();
	}
	else
	{
		FAssert(!isFighting());
		if (plot()->isFighting() || pPlot->isFighting())
		{
			// K-Mod. we need to wait for our turn to attack - so don't bother looking for a defender yet.
			return;
		}
		pDefender = pPlot->getBestDefender(NO_PLAYER, getOwnerINLINE(), this, true);
	}

	if (pDefender == NULL)
	{
		setAttackPlot(NULL, false);
		setCombatUnit(NULL);

		//getGroup()->groupMove(pPlot, true, ((canAdvance(pPlot, 0)) ? this : NULL));
		// K-Mod
		if (bFinish)
		{
			FAssertMsg(false, "Cannot 'finish' combat with NULL defender");
			return;
		}
		else
			getGroup()->groupMove(pPlot, true, canAdvance(pPlot, 0) ? this : NULL, true);
		// K-Mod end

		getGroup()->clearMissionQueue();

		return;
	}

	//check if quick combat
	if (!bQuick)
	{
		bVisible = isCombatVisible(pDefender);
	}

	//FAssertMsg((pPlot == pDefender->plot()), "There is not expected to be a defender or the defender's plot is expected to be pPlot (the attack plot)");

	//if not finished and not fighting yet, set up combat damage and mission
	if (!bFinish)
	{
		if (!isFighting())
		{
			if (plot()->isFighting() || pPlot->isFighting())
			{
				return;
			}

			setMadeAttack(true);

			//rotate to face plot
			DirectionTypes newDirection = estimateDirection(this->plot(), pDefender->plot());
			if (newDirection != NO_DIRECTION)
			{
				setFacingDirection(newDirection);
			}

			//rotate enemy to face us
			newDirection = estimateDirection(pDefender->plot(), this->plot());
			if (newDirection != NO_DIRECTION)
			{
				pDefender->setFacingDirection(newDirection);
			}

			setCombatUnit(pDefender, true);
			pDefender->setCombatUnit(this, false);

			pDefender->setAttackPlot(NULL, false); // K-Mod (to prevent weirdness from simultanious attacks)
			pDefender->getGroup()->clearMissionQueue();

			bool bFocused = (bVisible && isCombatFocus() && gDLL->getInterfaceIFace()->isCombatFocus());

			if (bFocused)
			{
				DirectionTypes directionType = directionXY(plot(), pPlot);
				//								N			NE				E				SE					S				SW					W				NW
				NiPoint2 directions[8] = {NiPoint2(0, 1), NiPoint2(1, 1), NiPoint2(1, 0), NiPoint2(1, -1), NiPoint2(0, -1), NiPoint2(-1, -1), NiPoint2(-1, 0), NiPoint2(-1, 1)};
				NiPoint3 attackDirection = NiPoint3(directions[directionType].x, directions[directionType].y, 0);
				float plotSize = GC.getPLOT_SIZE();
				NiPoint3 lookAtPoint(plot()->getPoint().x + plotSize / 2 * attackDirection.x, plot()->getPoint().y + plotSize / 2 * attackDirection.y, (plot()->getPoint().z + pPlot->getPoint().z) / 2);
				attackDirection.Unitize();
				gDLL->getInterfaceIFace()->lookAt(lookAtPoint, (((getOwnerINLINE() != GC.getGameINLINE().getActivePlayer()) || gDLL->getGraphicOption(GRAPHICOPTION_NO_COMBAT_ZOOM)) ? CAMERALOOKAT_BATTLE : CAMERALOOKAT_BATTLE_ZOOM_IN), attackDirection);
			}
			else
			{
				PlayerTypes eAttacker = getVisualOwner(pDefender->getTeam());
				CvWString szMessage;
				if (BARBARIAN_PLAYER != eAttacker)
				{
					szMessage = gDLL->getText("TXT_KEY_MISC_YOU_UNITS_UNDER_ATTACK", GET_PLAYER(getOwnerINLINE()).getNameKey());
				}
				else
				{
					szMessage = gDLL->getText("TXT_KEY_MISC_YOU_UNITS_UNDER_ATTACK_UNKNOWN");
				}

				gDLL->getInterfaceIFace()->addHumanMessage(pDefender->getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szMessage, "AS2D_COMBAT", MESSAGE_TYPE_DISPLAY_ONLY, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true);
			}
		}

		FAssertMsg(pDefender != NULL, "Defender is not assigned a valid value");

		FAssertMsg(plot()->isFighting(), "Current unit instance plot is not fighting as expected");
		FAssertMsg(pPlot->isFighting(), "pPlot is not fighting as expected");

		if (!pDefender->canDefend())
		{
			if (!bVisible)
			{
				bFinish = true;
			}
			else
			{
				CvMissionDefinition kMission;
				kMission.setMissionTime(getCombatTimer() * gDLL->getSecsPerTurn());
				kMission.setMissionType(MISSION_SURRENDER);
				kMission.setUnit(BATTLE_UNIT_ATTACKER, this);
				kMission.setUnit(BATTLE_UNIT_DEFENDER, pDefender);
				kMission.setPlot(pPlot);
				gDLL->getEntityIFace()->AddMission(&kMission);

				// Surrender mission
				setCombatTimer(GC.getMissionInfo(MISSION_SURRENDER).getTime());

				GC.getGameINLINE().incrementTurnTimer(getCombatTimer());
			}

			// Kill them!
			pDefender->setDamage(GC.getMAX_HIT_POINTS());
		}
		else
		{
			resolveCombat(pDefender, pPlot, bVisible);

			FAssert(!bVisible || getCombatTimer() > 0);
			if (!bVisible)
				bFinish = true;

			// Note: K-Mod has moved the bulk of this block into resolveCombat.
		}
	}

	if (bFinish)
	{
		if (bVisible)
		{
			if (isCombatFocus() && gDLL->getInterfaceIFace()->isCombatFocus())
			{
				if (getOwnerINLINE() == GC.getGameINLINE().getActivePlayer())
				{
					gDLL->getInterfaceIFace()->releaseLockedCamera();
				}
			}
		}

		//end the combat mission if this code executes first
		gDLL->getEntityIFace()->RemoveUnitFromBattle(this);
		gDLL->getEntityIFace()->RemoveUnitFromBattle(pDefender);
		setAttackPlot(NULL, false);
		setCombatUnit(NULL);
		pDefender->setCombatUnit(NULL);
		NotifyEntity(MISSION_DAMAGE);
		pDefender->NotifyEntity(MISSION_DAMAGE);
		PlayerTypes plotOwner = pPlot->getOwnerINLINE(); // advc.130m
		if (isDead())
		{
			if (isBarbarian())
			{
				GET_PLAYER(pDefender->getOwnerINLINE()).changeWinsVsBarbs(1);
			}

			if (!m_pUnitInfo->isHiddenNationality() && !pDefender->getUnitInfo().isHiddenNationality())
			{
				GET_TEAM(getTeam()).changeWarWeariness(pDefender->getTeam(), *pPlot, GC.getDefineINT("WW_UNIT_KILLED_ATTACKING"));
				GET_TEAM(pDefender->getTeam()).changeWarWeariness(getTeam(), *pPlot, GC.getDefineINT("WW_KILLED_UNIT_DEFENDING"));
			}
			// <advc.130m>
			int const ws = GC.getDefineINT("WAR_SUCCESS_DEFENDING");
			// No functional change:
			GET_TEAM(pDefender->getTeam()).AI_changeWarSuccess(getTeam(), ws);
			if(plotOwner != NO_PLAYER) {
				TeamTypes plotMasterId = TEAMREF(plotOwner).getMasterTeam();
				/*  Success vs. barbs isn't normally counted. Do count it if
					it happens in another civ's borders. Also Privateers.
					Success within their borders against a shared war enemy is
					already counted by changeWarSuccess, but count it again
					for added weight. */
				if(plotMasterId != GET_TEAM(pDefender->getTeam()).getMasterTeam() &&
						// Plot owner not happy if his own Privateer killed
						plotMasterId != GET_TEAM(getTeam()).getMasterTeam() &&
						(isBarbarian() || m_pUnitInfo->isHiddenNationality() ||
						GET_TEAM(getTeam()).isAtWar(pPlot->getTeam())))
					TEAMREF(plotOwner).reportSharedWarSuccess(
							ws, pDefender->getTeam(), getTeam(), true);
				// Same for the owner of the dead unit
				if(plotMasterId != GET_TEAM(pDefender->getTeam()).getMasterTeam() &&
						plotMasterId != GET_TEAM(getTeam()).getMasterTeam() &&
						(pDefender->isBarbarian() ||
						pDefender->m_pUnitInfo->isHiddenNationality() ||
						GET_TEAM(pDefender->getTeam()).isAtWar(pPlot->getTeam())))
					TEAMREF(plotOwner).reportSharedWarSuccess(
							ws, getTeam(), pDefender->getTeam(), true);
			} // <advc.130m>

			szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_UNIT_DIED_ATTACKING", getNameKey(), pDefender->getNameKey());
			gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, GC.getEraInfo(GC.getGameINLINE().getCurrentEra()).getAudioUnitDefeatScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE());
			szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_KILLED_ENEMY_UNIT", pDefender->getNameKey(), getNameKey(), getVisualCivAdjective(pDefender->getTeam()));
			gDLL->getInterfaceIFace()->addHumanMessage(pDefender->getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, GC.getEraInfo(GC.getGameINLINE().getCurrentEra()).getAudioUnitVictoryScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pPlot->getX_INLINE(), pPlot->getY_INLINE());

			// report event to Python, along with some other key state
			CvEventReporter::getInstance().combatResult(pDefender, this);
		}
		else if (pDefender->isDead())
		{
			if (pDefender->isBarbarian())
			{
				GET_PLAYER(getOwnerINLINE()).changeWinsVsBarbs(1);
			}

			if (!m_pUnitInfo->isHiddenNationality() && !pDefender->getUnitInfo().isHiddenNationality())
			{
				GET_TEAM(pDefender->getTeam()).changeWarWeariness(getTeam(), *pPlot, GC.getDefineINT("WW_UNIT_KILLED_DEFENDING"));
				GET_TEAM(getTeam()).changeWarWeariness(pDefender->getTeam(), *pPlot, GC.getDefineINT("WW_KILLED_UNIT_ATTACKING"));
			}
			// <advc.130m>
			int const ws = GC.getDefineINT("WAR_SUCCESS_ATTACKING");
			// No functional change:
			GET_TEAM(getTeam()).AI_changeWarSuccess(pDefender->getTeam(), ws);
			// As above
			if(plotOwner != NO_PLAYER) {
				TeamTypes plotMasterId = TEAMREF(plotOwner).getMasterTeam();
				if(plotMasterId != GET_TEAM(pDefender->getTeam()).getMasterTeam() &&
						plotMasterId != GET_TEAM(getTeam()).getMasterTeam() &&
						(isBarbarian() || m_pUnitInfo->isHiddenNationality() ||
						GET_TEAM(getTeam()).isAtWar(pPlot->getTeam())))
					TEAMREF(plotOwner).reportSharedWarSuccess(
							ws, pDefender->getTeam(), getTeam(), true);
				if(plotMasterId != GET_TEAM(pDefender->getTeam()).getMasterTeam() &&
						plotMasterId != GET_TEAM(getTeam()).getMasterTeam() &&
						(pDefender->isBarbarian() ||
						pDefender->m_pUnitInfo->isHiddenNationality() ||
						GET_TEAM(pDefender->getTeam()).isAtWar(pPlot->getTeam())))
					TEAMREF(plotOwner).reportSharedWarSuccess(
							ws, getTeam(), pDefender->getTeam(), true);
			} // <advc.130m>

			szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_UNIT_DESTROYED_ENEMY", getNameKey(), pDefender->getNameKey());
			gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, GC.getEraInfo(GC.getGameINLINE().getCurrentEra()).getAudioUnitVictoryScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pPlot->getX_INLINE(), pPlot->getY_INLINE());
			if (getVisualOwner(pDefender->getTeam()) != getOwnerINLINE())
			{
				szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_UNIT_WAS_DESTROYED_UNKNOWN", pDefender->getNameKey(), getNameKey());
			}
			else
			{
				szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_UNIT_WAS_DESTROYED", pDefender->getNameKey(), getNameKey(), getVisualCivAdjective(pDefender->getTeam()));
			}
			gDLL->getInterfaceIFace()->addHumanMessage(pDefender->getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer,GC.getEraInfo(GC.getGameINLINE().getCurrentEra()).getAudioUnitDefeatScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE());

			// report event to Python, along with some other key state
			CvEventReporter::getInstance().combatResult(this, pDefender);

			bool bAdvance = false;

			if (isSuicide())
			{
				kill(true);

				pDefender->kill(false);
				pDefender = NULL;
			}
			else
			{
				bAdvance = canAdvance(pPlot, ((pDefender->canDefend()) ? 1 : 0));

				if (bAdvance)
				{
					if (!isNoCapture())
					{
						pDefender->setCapturingPlayer(getOwnerINLINE());
					}
				}


				pDefender->kill(false);
				pDefender = NULL;

				if (!bAdvance)
				{
					changeMoves(std::max(GC.getMOVE_DENOMINATOR(), pPlot->movementCost(this, plot())));
					checkRemoveSelectionAfterAttack();
				}
			}

			if (pPlot->getNumVisibleEnemyDefenders(this) == 0)
			{
				//getGroup()->groupMove(pPlot, true, ((bAdvance) ? this : NULL));
				getGroup()->groupMove(pPlot, true, bAdvance ? this : NULL, true); // K-Mod
			}

			// This is is put before the plot advancement, the unit will always try to walk back
			// to the square that they came from, before advancing.
			getGroup()->clearMissionQueue();
		}
		else
		{
			szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_UNIT_WITHDRAW", getNameKey(), pDefender->getNameKey());
			gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_OUR_WITHDRAWL", MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pPlot->getX_INLINE(), pPlot->getY_INLINE());
			szBuffer = gDLL->getText("TXT_KEY_MISC_ENEMY_UNIT_WITHDRAW", getNameKey(), pDefender->getNameKey());
			gDLL->getInterfaceIFace()->addHumanMessage(pDefender->getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_THEIR_WITHDRAWL", MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE());

			changeMoves(std::max(GC.getMOVE_DENOMINATOR(), pPlot->movementCost(this, plot())));
			checkRemoveSelectionAfterAttack();

			getGroup()->clearMissionQueue();
		}
	}
}

void CvUnit::checkRemoveSelectionAfterAttack()
{
	if (!canMove() || !isBlitz())
	{
		if (IsSelected())
		{
			if (gDLL->getInterfaceIFace()->getLengthSelectionList() > 1)
			{
				gDLL->getInterfaceIFace()->removeFromSelectionList(this);
			}
		}
	}
}


bool CvUnit::isActionRecommended(int iAction)
{
	//CvCity* pWorkingCity;
	//CvPlot* pPlot;
	//ImprovementTypes eImprovement;
	//ImprovementTypes eFinalImprovement;
	//BuildTypes eBuild;
	//RouteTypes eRoute;
	//BonusTypes eBonus;
	//int iIndex;
	/*  advc.002e: Can't find another DLL function that is called by the EXE after
		loading a savegame and after calling CvUnit::isPromotionReady */
	gDLL->getEntityIFace()->showPromotionGlow(getUnitEntity(), isReadyForPromotion());
	if (getOwnerINLINE() != GC.getGameINLINE().getActivePlayer())
	{
		return false;
	}

	/* advc.004h: When the go-to tile changes, the EXE appears to call
	   isActionRecommended before coloring the 1-tile radius around the
	   go-to tile (if a Settler is selected). Perhaps some DLL function
	   is called before displaying the map - then this call should be moved
	   into that function - but I doubt it. This also means that the 1-tile
	   radius can't be cleared from the DLL. What might work is temporarily
	   changing the player color to sth. transparent, or some property of
	   the go-to tile so that the EXE decides not to color it.
	   I've tried unrevealing the tile, but that didn't stop the coloring.
	   Since it doesn't work as intended (both radi are drawn), showCityCross
	   currently does nothing. */
	if(gDLL->getInterfaceIFace()->canSelectionListFound()) showCityCross();

	if (GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_NO_UNIT_RECOMMENDATIONS))
	{
		return false;
	}

	CyUnit* pyUnit = new CyUnit(this);
	CyArgsList argsList;
	argsList.add(gDLL->getPythonIFace()->makePythonObject(pyUnit));	// pass in unit class
	argsList.add(iAction);
	long lResult=0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "isActionRecommended", argsList.makeFunctionArgs(), &lResult);
	delete pyUnit;	// python fxn must not hold on to this pointer 
	if (lResult == 1)
	{
		return true;
	}

	CvPlot* pPlot = gDLL->getInterfaceIFace()->getGotoPlot();

	if (pPlot == NULL)
	{
		if (GC.shiftKey())
		{
			pPlot = getGroup()->lastMissionPlot();
		}
	}

	if (pPlot == NULL)
	{
		pPlot = plot();
	}

	if (GC.getActionInfo(iAction).getMissionType() == MISSION_FORTIFY)
	{
		if (pPlot->isCity(true, getTeam()))
		{
			if (canDefend(pPlot))
			{
				if (pPlot->getNumDefenders(getOwnerINLINE()) < ((atPlot(pPlot)) ? 2 : 1))
				{
					return true;
				}
			}
		}
	}

	if (GC.getActionInfo(iAction).getMissionType() == MISSION_HEAL)
	{
		if (isHurt())
		{
			if (!hasMoved())
			{
				if ((pPlot->getTeam() == getTeam()) || (healTurns(pPlot) < 4))
				{
					return true;
				}
			}
		}
	}

	if (GC.getActionInfo(iAction).getMissionType() == MISSION_FOUND)
	{
		if (canFound(pPlot))
		{
			if (pPlot->isBestAdjacentFound(getOwnerINLINE()))
			{
				return true;
			}
		}
	}

	if (GC.getActionInfo(iAction).getMissionType() == MISSION_BUILD)
	{
		if (pPlot->getOwnerINLINE() == getOwnerINLINE())
		{
			BuildTypes eBuild = (BuildTypes)GC.getActionInfo(iAction).getMissionData();
			FAssert(eBuild != NO_BUILD);
			FAssertMsg(eBuild < GC.getNumBuildInfos(), "Invalid Build");

			if (canBuild(pPlot, eBuild))
			{
				/// K-Mod
				if (pPlot->getBuildProgress(eBuild) > 0)
					return true;
				// K-Mod end

				ImprovementTypes eImprovement = (ImprovementTypes)GC.getBuildInfo(eBuild).getImprovement();
				RouteTypes eRoute = (RouteTypes)(GC.getBuildInfo(eBuild).getRoute());
				//eBonus = pPlot->getBonusType(getTeam());
				BonusTypes eBonus = pPlot->getNonObsoleteBonusType(getTeam()); // K-Mod
				CvCity* pWorkingCity = pPlot->getWorkingCity();

				// if (pPlot->getImprovementType() == NO_IMPROVEMENT) // Disabled by K-Mod (this looks like a bug to me)
				{
					BuildTypes eBestBuild = NO_BUILD; // K-Mod. (I use this again later.)
					if (pWorkingCity)
					{
						int iIndex = pWorkingCity->getCityPlotIndex(pPlot);
						FAssert(iIndex != -1); // K-Mod. this use to be an if statement in the release code

						eBestBuild = pWorkingCity->AI_getBestBuild(iIndex);

						if (eBestBuild == eBuild)
							return true;
					}

					if (eImprovement != NO_IMPROVEMENT)
					{
						/* original code
						if (eBonus != NO_BONUS)
						{
							if (GC.getImprovementInfo(eImprovement).isImprovementBonusTrade(eBonus))
							{
								return true;
							}
						} */

						// K-Mod
						if (eBonus != NO_BONUS &&
							!GET_PLAYER(getOwnerINLINE()).doesImprovementConnectBonus(pPlot->getImprovementType(), eBonus) &&
							GET_PLAYER(getOwnerINLINE()).doesImprovementConnectBonus(eImprovement, eBonus) &&
							(eBestBuild == NO_BUILD || !GET_PLAYER(getOwnerINLINE()).doesImprovementConnectBonus((ImprovementTypes)GC.getBuildInfo(eBestBuild).getImprovement(), eBonus)))
						{
							return true;
						}

						if (pPlot->getImprovementType() == NO_IMPROVEMENT && eBonus == NO_BONUS && pWorkingCity == NULL)
						{
							if (pPlot->getFeatureType() == NO_FEATURE || !GC.getBuildInfo(eBuild).isFeatureRemove((FeatureTypes)pPlot->getFeatureType()))
							{
								if (GC.getImprovementInfo(eImprovement).isCarriesIrrigation() && !pPlot->isIrrigated() && pPlot->isIrrigationAvailable(true))
									return true;

								if (pPlot->getFeatureType() != NO_FEATURE && GC.getImprovementInfo(eImprovement).getFeatureGrowthProbability() > 0)
									return true;
							}
						}
						// K-Mod end

						/* original bts code
						if (pPlot->getImprovementType() == NO_IMPROVEMENT)
						{
							if (!(pPlot->isIrrigated()) && pPlot->isIrrigationAvailable(true))
							{
								if (GC.getImprovementInfo(eImprovement).isCarriesIrrigation())
								{
									return true;
								}
							}

							if (pWorkingCity != NULL)
							{
								if (GC.getImprovementInfo(eImprovement).getYieldChange(YIELD_FOOD) > 0)
								{
									return true;
								}

								if (pPlot->isHills())
								{
									if (GC.getImprovementInfo(eImprovement).getYieldChange(YIELD_PRODUCTION) > 0)
									{
										return true;
									}
								}
								else
								{
									if (GC.getImprovementInfo(eImprovement).getYieldChange(YIELD_COMMERCE) > 0)
									{
										return true;
									}
								}
							}
						} */ 
					}
				}

				if (eRoute != NO_ROUTE)
				{
					if (!(pPlot->isRoute()))
					{
						if (eBonus != NO_BONUS)
						{
							return true;
						}

						if (pWorkingCity != NULL)
						{
							if (pPlot->isRiver())
							{
								return true;
							}
						}
					}

					/* original bts code
					eFinalImprovement = eImprovement;

					if (eFinalImprovement == NO_IMPROVEMENT)
					{
						eFinalImprovement = pPlot->getImprovementType();
					} */
					// K-Mod
					ImprovementTypes eFinalImprovement = finalImprovementUpgrade(eImprovement != NO_IMPROVEMENT ? eImprovement : pPlot->getImprovementType());
					// K-Mod end

					if (eFinalImprovement != NO_IMPROVEMENT)
					{
						if ((GC.getImprovementInfo(eFinalImprovement).getRouteYieldChanges(eRoute, YIELD_FOOD) > 0) ||
							(GC.getImprovementInfo(eFinalImprovement).getRouteYieldChanges(eRoute, YIELD_PRODUCTION) > 0) ||
							(GC.getImprovementInfo(eFinalImprovement).getRouteYieldChanges(eRoute, YIELD_COMMERCE) > 0))
						{
							return true;
						}
					}
				}
			}
		}
	}

	if (GC.getActionInfo(iAction).getCommandType() == COMMAND_PROMOTION)
	{
		return true;
	}

	return false;
}

// <advc.004h>
void CvUnit::showCityCross() const {

	// Uncomment in order to enable change 004h:
	return;
	// Both needed?
	gDLL->getEngineIFace()->clearColoredPlots(PLOT_LANDSCAPE_LAYER_BASE);
	gDLL->getEngineIFace()->clearAreaBorderPlots(
			AREA_BORDER_LAYER_CITY_RADIUS);
	CvPlot* goToPlot = gDLL->getInterfaceIFace()->getGotoPlot();
	CvPlot* center;
	if(goToPlot == NULL)
		center = plot();
	else center = goToPlot;
	if(center == NULL || !center->isRevealed(TEAMID(getOwner()), false) ||
			!(atPlot(center) && !canMoveInto(center)))
		return;
	int colorId = GC.getPlayerColorInfo(GET_PLAYER(getOwner()).
			getPlayerColor()).getColorTypePrimary();
	const NiColorA& col = GC.getColorInfo((ColorTypes)colorId).getColor();
	NiColorA blank(GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_WHITE")).getColor());
	blank.a = 0.f;
	for(int i = 0; i < GC.getMapINLINE().numPlots(); i++) {
		CvPlot const& p = *GC.getMapINLINE().plotByIndexINLINE(i);
		if(::plotDistance(center, &p) <= CITY_PLOTS_RADIUS) {
			gDLL->getEngineIFace()->fillAreaBorderPlot(p.getX_INLINE(),
					p.getY_INLINE(), col, AREA_BORDER_LAYER_CITY_RADIUS);
		}
	}
	// Needed?
	gDLL->getInterfaceIFace()->setDirty(ColoredPlots_DIRTY_BIT, true);
} // </advc.004h>

// From Lead From Behind by UncutDragon
// original
//bool CvUnit::isBetterDefenderThan(const CvUnit* pDefender, const CvUnit* pAttacker) const
// modified (with extra parameter)
bool CvUnit::isBetterDefenderThan(const CvUnit* pDefender, const CvUnit* pAttacker, int* pBestDefenderRank) const
{
	int iOurDefense;
	int iTheirDefense;

	TeamTypes eAttackerTeam = NO_TEAM;
	if (NULL != pAttacker)
	{
		eAttackerTeam = pAttacker->getTeam();
	}

	// <advc.028>
	bool invisible = (eAttackerTeam != NO_TEAM && isInvisible(eAttackerTeam, false));
	// Only pick invisible unit as defender once attack is underway ...
	if(invisible && pAttacker->getAttackPlot() == NULL)
		return false;
	/*  and if there is some visible team unit that could get attacked otherwise
		(better: check if our team has the best visible defender; tbd.): */
	if(invisible) {
		bool found = false;
		CLLNode<IDInfo>* node = plot()->headUnitNode();
		while(node != NULL) {
			CvUnit* u = ::getUnit(node->m_data);
			node = plot()->nextUnitNode(node);
			if(u->getTeam() == getTeam() && !u->isInvisible(eAttackerTeam, false))
				found = true;
		}
		if(!found)
			return false;
	}
	// Moved down: // </advc.028>
	if (pDefender == NULL)
	{
		return true;
	}
	// <advc.028>
	if(pDefender->getTeam() != getTeam() && invisible)
		return false; // </advc.028>
	if (canCoexistWithEnemyUnit(eAttackerTeam))
	{
		return false;
	}

	if (!canDefend())
	{
		return false;
	}

	if (canDefend() && !(pDefender->canDefend()))
	{
		return true;
	}

	if (pAttacker)
	{
		if (isTargetOf(*pAttacker) && !pDefender->isTargetOf(*pAttacker))
		{
			return true;
		}

		if (!isTargetOf(*pAttacker) && pDefender->isTargetOf(*pAttacker))
		{
			return false;
		}

		if (pAttacker->canAttack(*pDefender) && !pAttacker->canAttack(*this))
		{
			return false;
		}

		if (pAttacker->canAttack(*this) && !pAttacker->canAttack(*pDefender))
		{
			return true;
		}
	}

	// UncutDragon
	// To cut down on changes to existing code, we just short-circuit the method
	// and this point and call our own version instead
	if (GC.getLFBEnable())
		return LFBisBetterDefenderThan(pDefender, pAttacker, pBestDefenderRank);
	// /UncutDragon

	iOurDefense = currCombatStr(plot(), pAttacker);
	if (::isWorldUnitClass(getUnitClassType()))
	{
		iOurDefense /= 2;
	}

	if (NULL == pAttacker)
	{
		if (pDefender->collateralDamage() > 0)
		{
			iOurDefense *= (100 + pDefender->collateralDamage());
			iOurDefense /= 100;
		}

		if (pDefender->currInterceptionProbability() > 0)
		{
			iOurDefense *= (100 + pDefender->currInterceptionProbability());
			iOurDefense /= 100;
		}
	}
	else
	{
		if (!(pAttacker->immuneToFirstStrikes()))
		{
			iOurDefense *= 100 + (firstStrikes() * 2 + chanceFirstStrikes()) * GC.getCOMBAT_DAMAGE() * 2 / 5;
			iOurDefense /= 100;
		}

		if (immuneToFirstStrikes())
		{
			iOurDefense *= 100 + (pAttacker->firstStrikes() * 2 + pAttacker->chanceFirstStrikes()) * GC.getCOMBAT_DAMAGE() * 2 / 5;
			iOurDefense /= 100;
		}
	}

	int iAssetValue = std::max(1, getUnitInfo().getAssetValue());
	int iCargoAssetValue = 0;
	std::vector<CvUnit*> aCargoUnits;
	getCargoUnits(aCargoUnits);
	for (uint i = 0; i < aCargoUnits.size(); ++i)
	{
		iCargoAssetValue += aCargoUnits[i]->getUnitInfo().getAssetValue();
	}
	iOurDefense = iOurDefense * iAssetValue / std::max(1, iAssetValue + iCargoAssetValue);

	iTheirDefense = pDefender->currCombatStr(plot(), pAttacker);
	if (::isWorldUnitClass(pDefender->getUnitClassType()))
	{
		iTheirDefense /= 2;
	}

	if (NULL == pAttacker)
	{
		if (collateralDamage() > 0)
		{
			iTheirDefense *= (100 + collateralDamage());
			iTheirDefense /= 100;
		}

		if (currInterceptionProbability() > 0)
		{
			iTheirDefense *= (100 + currInterceptionProbability());
			iTheirDefense /= 100;
		}
	}
	else
	{
		if (!(pAttacker->immuneToFirstStrikes()))
		{
			iTheirDefense *= 100 + (pDefender->firstStrikes() * 2 + pDefender->chanceFirstStrikes()) * GC.getCOMBAT_DAMAGE() * 2 / 5;
			iTheirDefense /= 100;
		}

		if (pDefender->immuneToFirstStrikes())
		{
			iTheirDefense *= 100 + (pAttacker->firstStrikes() * 2 + pAttacker->chanceFirstStrikes()) * GC.getCOMBAT_DAMAGE() * 2 / 5;
			iTheirDefense /= 100;
		}
	}

	iAssetValue = std::max(1, pDefender->getUnitInfo().getAssetValue());
	iCargoAssetValue = 0;
	pDefender->getCargoUnits(aCargoUnits);
	for (uint i = 0; i < aCargoUnits.size(); ++i)
	{
		iCargoAssetValue += aCargoUnits[i]->getUnitInfo().getAssetValue();
	}
	iTheirDefense = iTheirDefense * iAssetValue / std::max(1, iAssetValue + iCargoAssetValue);

	if (iOurDefense == iTheirDefense)
	{
		if (NO_UNIT == getLeaderUnitType() && NO_UNIT != pDefender->getLeaderUnitType())
		{
			++iOurDefense;
		}
		else if (NO_UNIT != getLeaderUnitType() && NO_UNIT == pDefender->getLeaderUnitType())
		{
			++iTheirDefense;
		}
		else if (isBeforeUnitCycle(this, pDefender))
		{
			++iOurDefense;
		}
	}

	return (iOurDefense > iTheirDefense);
}

bool CvUnit::canDoCommand(CommandTypes eCommand, int iData1, int iData2, bool bTestVisible, bool bTestBusy)
{
	CvUnit* pUnit;

	if (bTestBusy && getGroup()->isBusy())
	{
		return false;
	}

	switch (eCommand)
	{
	case COMMAND_PROMOTION:
		if (canPromote((PromotionTypes)iData1, iData2))
		{
			return true;
		}
		break;

	case COMMAND_UPGRADE:
		if (canUpgrade(((UnitTypes)iData1), bTestVisible))
		{
			return true;
		}
		break;

	case COMMAND_AUTOMATE:
		if (canAutomate((AutomateTypes)iData1))
		{
			return true;
		}
		break;

	case COMMAND_WAKE:
		if (!isAutomated() && isWaiting())
		{
			return true;
		}
		break;

	case COMMAND_CANCEL:
	case COMMAND_CANCEL_ALL:
		if (!isAutomated() && (getGroup()->getLengthMissionQueue() > 0))
		{
			return true;
		}
		break;

	case COMMAND_STOP_AUTOMATION:
		if (isAutomated())
		{
			return true;
		}
		break;

	case COMMAND_DELETE:
		if (canScrap())
		{
			return true;
		}
		break;

	case COMMAND_GIFT:
		if (canGift(bTestVisible))
		{
			return true;
		}
		break;

	case COMMAND_LOAD:
		if (canLoad(plot(),
			/*  advc.123c: If a unit is moved onto a transport, canLoad gets
				checked before the command can be issued, and again after the unit
				has moved. Want to check whether the unit has moves left only in
				the first check (otherwise the unit moves onto the water tile,
				but isn't actually loaded).
				bTestVisible is (apparently ...) true before the command gets issued
				and false afterwards. */
			bTestVisible))
		{
			return true;
		}
		break;

	case COMMAND_LOAD_UNIT:
		pUnit = ::getUnit(IDInfo(((PlayerTypes)iData1), iData2));
		if (pUnit != NULL)
		{
			if (canLoadUnit(pUnit, plot()))
			{
				return true;
			}
		}
		break;

	case COMMAND_UNLOAD:
		if (canUnload())
		{
			return true;
		}
		break;

	case COMMAND_UNLOAD_ALL:
		if (canUnloadAll())
		{
			return true;
		}
		break;

	case COMMAND_HOTKEY:
		if (isGroupHead())
		{
			return true;
		}
		break;

	default:
		FAssert(false);
		break;
	}

	return false;
}


void CvUnit::doCommand(CommandTypes eCommand, int iData1, int iData2)
{
	CvUnit* pUnit;
	bool bCycle;

	bCycle = false;

	FAssert(getOwnerINLINE() != NO_PLAYER);

	if (canDoCommand(eCommand, iData1, iData2))
	{
		switch (eCommand)
		{
		case COMMAND_PROMOTION:
			promote((PromotionTypes)iData1, iData2);
			break;

		case COMMAND_UPGRADE:
			upgrade((UnitTypes)iData1);
			bCycle = true;
			break;

		case COMMAND_AUTOMATE:
			automate((AutomateTypes)iData1);
			bCycle = true;
			break;

		case COMMAND_WAKE:
			getGroup()->setActivityType(ACTIVITY_AWAKE);
			break;

		case COMMAND_CANCEL:
			getGroup()->popMission();
			break;

		case COMMAND_CANCEL_ALL:
			getGroup()->clearMissionQueue();
			break;

		case COMMAND_STOP_AUTOMATION:
			getGroup()->setAutomateType(NO_AUTOMATE);
			break;

		case COMMAND_DELETE:
			scrap();
			bCycle = true;
			break;

		case COMMAND_GIFT:
			gift();
			bCycle = true;
			break;

		case COMMAND_LOAD:
			load();
			bCycle = true;
			break;

		case COMMAND_LOAD_UNIT:
			pUnit = ::getUnit(IDInfo(((PlayerTypes)iData1), iData2));
			if (pUnit != NULL)
			{
				loadUnit(pUnit);
				bCycle = true;
			}
			break;

		case COMMAND_UNLOAD:
			unload();
			bCycle = true;
			break;

		case COMMAND_UNLOAD_ALL:
			unloadAll();
			bCycle = true;
			break;

		case COMMAND_HOTKEY:
			setHotKeyNumber(iData1);
			break;

		default:
			FAssert(false);
			break;
		}
	}

	if (bCycle)
	{
		if (IsSelected())
		{
			GC.getGameINLINE().cycleSelectionGroups_delayed(1, false);
		}
	}

	getGroup()->doDelayedDeath();
}

// Disabled by K-Mod. (This function is deprecated.)
/* FAStarNode* CvUnit::getPathLastNode() const
{
	return getGroup()->getPathLastNode();
} */


CvPlot* CvUnit::getPathEndTurnPlot() const
{
	return getGroup()->getPathEndTurnPlot();
}


bool CvUnit::generatePath(const CvPlot* pToPlot, int iFlags, bool bReuse, int* piPathTurns, int iMaxPath) const
{
	return getGroup()->generatePath(plot(), pToPlot, iFlags, bReuse, piPathTurns, iMaxPath);
}

// K-Mod. Return the standard pathfinder, for extracting path information.
KmodPathFinder& CvUnit::getPathFinder() const
{
	return CvSelectionGroup::path_finder;
}
// K-Mod end

bool CvUnit::canEnterTerritory(TeamTypes eTeam, bool bIgnoreRightOfPassage) const
{
	if (GET_TEAM(getTeam()).isFriendlyTerritory(eTeam))
	{
		return true;
	}

	if (eTeam == NO_TEAM)
	{
		return true;
	}

	if (isEnemy(eTeam))
	{
		return true;
	}

	if (isRivalTerritory())
	{
		return true;
	}

	if (alwaysInvisible())
	{
		return true;
	}

	if (m_pUnitInfo->isHiddenNationality())
	{
		return true;
	}

	if (!bIgnoreRightOfPassage)
	{
		if (GET_TEAM(getTeam()).isOpenBorders(eTeam)
				|| GET_TEAM(getTeam()).isDisengage(eTeam)) // advc.034
		{
			return true;
		}
	}
	return false;
}


bool CvUnit::canEnterArea(TeamTypes eTeam, const CvArea* pArea, bool bIgnoreRightOfPassage) const
{
	if (!canEnterTerritory(eTeam, bIgnoreRightOfPassage))
	{
		return false;
	}

	if (isBarbarian() && DOMAIN_LAND == getDomainType())
	{
		if (eTeam != NO_TEAM && eTeam != getTeam())
		{
			if (pArea && pArea->isBorderObstacle(eTeam))
			{
				return false;
			}
		}
	}

	return true;
}


// Returns the ID of the team to declare war against
TeamTypes CvUnit::getDeclareWarMove(const CvPlot* pPlot) const
{
	CvUnit* pUnit;
	TeamTypes eRevealedTeam;

	FAssert(isHuman());

	if (getDomainType() != DOMAIN_AIR)
	{
		eRevealedTeam = pPlot->getRevealedTeam(getTeam(), false);

		if (eRevealedTeam != NO_TEAM)
		{
			if (!canEnterArea(eRevealedTeam, pPlot->area()) || (getDomainType() == DOMAIN_SEA && !canCargoEnterArea(eRevealedTeam, pPlot->area(), false) && getGroup()->isAmphibPlot(pPlot)))
			{
				if (GET_TEAM(getTeam()).canDeclareWar(pPlot->getTeam()))
				{
					return eRevealedTeam;
				}
			}
		}
		else
		{
			if (pPlot->isActiveVisible(false))
			{
				//if (canMoveInto(pPlot, true, true, true))
				// K-Mod. Don't give the "declare war" popup unless we need war to move into the plot.
				if (canMoveInto(pPlot, true, true, true, false) && !canMoveInto(pPlot, false, false, false, false))
				// K-Mod end
				{
					pUnit = pPlot->plotCheck(PUF_canDeclareWar, getOwnerINLINE(), isAlwaysHostile(pPlot), NO_PLAYER, NO_TEAM, PUF_isVisible, getOwnerINLINE());

					if (pUnit != NULL)
					{
						return pUnit->getTeam();
					}
				}
			}
		}
	}

	return NO_TEAM;
}

bool CvUnit::willRevealByMove(const CvPlot* pPlot) const
{
	int iRange = visibilityRange() + 1;
	for (int i = -iRange; i <= iRange; ++i)
	{
		for (int j = -iRange; j <= iRange; ++j)
		{
			CvPlot* pLoopPlot = ::plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), i, j);
			if (NULL != pLoopPlot)
			{
				if (!pLoopPlot->isRevealed(getTeam(), false) && pPlot->canSeePlot(pLoopPlot, getTeam(), visibilityRange(), NO_DIRECTION))
				{
					return true;
				}
			}
		}
	}

	return false;
}

TeamTypes CvUnit::measuringDistance = NO_TEAM; // advc.104b

// K-Mod. I've rearranged a few things to make the function slightly faster, and added "bAssumeVisible" which signals that we should check for units on the plot regardless of whether we can actually see.
bool CvUnit::canMoveInto(const CvPlot* pPlot, bool bAttack, bool bDeclareWar, bool bIgnoreLoad, bool bAssumeVisible) const
{
	PROFILE_FUNC();

	FAssertMsg(pPlot != NULL, "Plot is not assigned a valid value");

	if (atPlot(pPlot))
	{
		return false;
	}

	if (!m_pUnitInfo->isCanMoveImpassable() && pPlot->isImpassable())
	{
		return false;
	}

	// Cannot move around in unrevealed land freely
	if (m_pUnitInfo->isNoRevealMap() && willRevealByMove(pPlot))
	{
		return false;
	}

	if (m_pUnitInfo->isSpy() && GC.getUSE_SPIES_NO_ENTER_BORDERS())
	{
		if (pPlot->getOwnerINLINE() != NO_PLAYER && !GET_PLAYER(getOwnerINLINE()).canSpiesEnterBorders(pPlot->getOwnerINLINE()))
		{
			return false;
		}
	}

	CvArea *pPlotArea = pPlot->area();
	TeamTypes ePlotTeam = pPlot->getTeam();
	bool bCanEnterArea = canEnterArea(ePlotTeam, pPlotArea);
	if (bCanEnterArea)
	{
		if (pPlot->getFeatureType() != NO_FEATURE)
		{
			if (m_pUnitInfo->getFeatureImpassable(pPlot->getFeatureType()))
			{
				TechTypes eTech = (TechTypes)m_pUnitInfo->getFeaturePassableTech(pPlot->getFeatureType());
				if (NO_TECH == eTech || !GET_TEAM(getTeam()).isHasTech(eTech))
				{
					if (DOMAIN_SEA != getDomainType() || pPlot->getTeam() != getTeam())  // sea units can enter impassable in own cultural borders
					{
						return false;
					}
				}
			}
		}

		if (m_pUnitInfo->getTerrainImpassable(pPlot->getTerrainType()))
		{
			TechTypes eTech = (TechTypes)m_pUnitInfo->getTerrainPassableTech(pPlot->getTerrainType());
			if (NO_TECH == eTech || !GET_TEAM(getTeam()).isHasTech(eTech))
			{
				if (DOMAIN_SEA != getDomainType() || pPlot->getTeam() != getTeam())  // sea units can enter impassable in own cultural borders
				{
					if (bIgnoreLoad || !canLoad(pPlot))
					{
						return false;
					}
				}
			}
		}
	}

	switch (getDomainType())
	{
	case DOMAIN_SEA:
		if (!pPlot->isWater() && !canMoveAllTerrain())
		{
			if (!pPlot->isFriendlyCity(*this, true) || !pPlot->isCoastalLand()) 
			{
				return false;
			}
		}
		break;

	case DOMAIN_AIR:
		if (!bAttack)
		{
			bool bValid = false;

			if (pPlot->isFriendlyCity(*this, true))
			{
				bValid = true;

				if (m_pUnitInfo->getAirUnitCap() > 0)
				{
					if (pPlot->airUnitSpaceAvailable(getTeam()) <= 0)
					{
						bValid = false;
					}
				}
			}

			if (!bValid)
			{
				if (bIgnoreLoad || !canLoad(pPlot))
				{
					return false;
				}
			}
		}

		break;

	case DOMAIN_LAND:
		if (pPlot->isWater() && !canMoveAllTerrain())
		{
			if (!pPlot->isCity() || 0 == GC.getDefineINT("LAND_UNITS_CAN_ATTACK_WATER_CITIES"))
			{
				//if (bIgnoreLoad || !isHuman() || plot()->isWater() || !canLoad(pPlot))
				if (bIgnoreLoad || plot()->isWater() || !canLoad(pPlot)) // K-Mod. (AI might want to load into a boat on the coast)
				{
					return false;
				}
			}
		}
		break;

	case DOMAIN_IMMOBILE:
		return false;
		break;

	default:
		FAssert(false);
		break;
	}

	if (isAnimal())
	{
		if (pPlot->isOwned())
		{
			return false;
		}

		if (!bAttack)
		{
			if (pPlot->getBonusType() != NO_BONUS)
			{
				return false;
			}

			if (pPlot->getImprovementType() != NO_IMPROVEMENT)
			{
				return false;
			}

			if (pPlot->getNumUnits() > 0)
			{
				return false;
			}
		}
	}

	if (isNoCapture())
	{
		if (!bAttack)
		{
			if (pPlot->isEnemyCity(*this))
			{
				return false;
			}
		}
		// K-Mod. Don't let noCapture units attack defenceless cities. (eg. cities with a worker in them)
		/*if (pPlot->isEnemyCity(*this))
		{
			if (!bAttack || !pPlot->isVisibleEnemyDefender(this))
				return false;
 		} */
		// K-Mod end
		/* advc.001: I think my (pre-K-Mod 1.45) fix in CvSelectionGroup::groupMove
		   might be better - at least I know it (mostly) works. */
	}

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       07/23/09                                jdog5000      */
/*                                                                                              */
/* Consistency                                                                                  */
/************************************************************************************************/
/* original bts code
	if (bAttack)
	{
		if (isMadeAttack() && !isBlitz())
		{
			return false;
		}
	}
*/
	// The following change makes capturing an undefended city like a attack action, it
	// cannot be done after another attack or a paradrop
	/*
	if (bAttack || (pPlot->isEnemyCity(*this) && !canCoexistWithEnemyUnit(NO_TEAM)) )
	{
		if (isMadeAttack() && !isBlitz())
		{
			return false;
		}
	}
	*/

	// The following change makes it possible to capture defenseless units after having 
	// made a previous attack or paradrop
	if (bAttack)
	{
		if (isMadeAttack() && !isBlitz() && pPlot->isVisibleEnemyDefender(this))
		{
			return false;
		}
	}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/

	if (getDomainType() == DOMAIN_AIR)
	{
		if (bAttack)
		{
			if (!canAirStrike(pPlot))
			{
				return false;
			}
		}
	}
	else
	{
		if (canAttack())
		{
			if (bAttack || !canCoexistWithEnemyUnit(NO_TEAM))
			{
				//if (!isHuman() || (pPlot->isVisible(getTeam(), false)))
				if (bAssumeVisible || pPlot->isVisible(getTeam(), false))
				{
					if (pPlot->isVisibleEnemyUnit(this) != bAttack)
					{
						//FAssertMsg(isHuman() || (!bDeclareWar || (pPlot->isVisibleOtherUnit(getOwnerINLINE()) != bAttack)), "hopefully not an issue, but tracking how often this is the case when we dont want to really declare war");
						/* original bts code
						if (!bDeclareWar || (pPlot->isVisibleOtherUnit(getOwnerINLINE()) != bAttack && !(bAttack && pPlot->getPlotCity() && !isNoCapture()))) */
						// K-Mod. I'm not entirely sure I understand what they were trying to do here. But I'm pretty sure it's wrong.
						// I think the rule should be that bAttack means we have to actually fight an enemy unit. Capturing an undefended city doesn't not count.
						// (there is no "isVisiblePotentialEnemyUnit" function, so I just wrote the code directly.)
						if (!bAttack || !bDeclareWar || !pPlot->isVisiblePotentialEnemyUnit(getOwnerINLINE()))
						// K-Mod end
						{
							return false;
						}
					}
				}
			}

			if (bAttack)
			{
				/* CvUnit* pDefender = pPlot->getBestDefender(NO_PLAYER, getOwnerINLINE(), this, true);
				if (NULL != pDefender)
				{
					if (!canAttack(*pDefender))
					{
						return false;
					}
				} */
				// K-Mod. (this is much faster.)
				if (!pPlot->hasDefender(true, NO_PLAYER, getOwnerINLINE(), this, true))
					return false;
				// K-Mod end
			}
		}
		else
		{
			if (bAttack)
			{
				return false;
			}

			if (!canCoexistWithEnemyUnit(NO_TEAM))
			{
				//if (!isHuman() || pPlot->isVisible(getTeam(), false))
				if (bAssumeVisible || pPlot->isVisible(getTeam(), false)) // K-Mod
				{
					if (pPlot->isEnemyCity(*this))
					{
						return false;
					}

					if (pPlot->isVisibleEnemyUnit(this))
					{
						return false;
					}
				}
			}
		}

		if (isHuman()) // (should this be !bAssumeVisible? It's a bit different to the other isHuman() checks)
		{
			ePlotTeam = pPlot->getRevealedTeam(getTeam(), false);
			bCanEnterArea = canEnterArea(ePlotTeam, pPlotArea);
		}

		if (!bCanEnterArea)
		{
			FAssert(ePlotTeam != NO_TEAM);
			// <advc.104b> Can't move across closed border of third-party
			if(measuringDistance != NO_TEAM && measuringDistance != ePlotTeam)
				return false; // </advc.104b>
			if (!(GET_TEAM(getTeam()).canDeclareWar(ePlotTeam))
				 /* advc.104b: Otherwise, distances can't be measured
				    by vassals or while there is a peace treaty. */
				&& measuringDistance == NO_TEAM
				)
			{
				return false;
			}

			/* original bts code
			if (isHuman())
			{
				if (!bDeclareWar)
				{
					return false;
				}
			}
			else
			{
				if (GET_TEAM(getTeam()).AI_isSneakAttackReady(ePlotTeam))
				{
					if (!(getGroup()->AI_isDeclareWar(pPlot)))
					{
						return false;
					}
				}
				else
				{
					return false;
				}
			} */
			// K-Mod. Rather than allowing the AI to move in forbidden territory when it is planning war.
			// I'm going to disallow it from doing so when _not_ planning war.
			if (!bDeclareWar)
			{
				return false;
			}
			else if (!isHuman()
				&& measuringDistance == NO_TEAM // advc.104b
			)
			{
				if (!GET_TEAM(getTeam()).AI_isSneakAttackReady(ePlotTeam) || !getGroup()->AI_isDeclareWar(pPlot))
				{
					return false;
				}
			}
			// K-Mod end
		}
	}

	if (GC.getUSE_UNIT_CANNOT_MOVE_INTO_CALLBACK())
	{
		// Python Override
		CyArgsList argsList;
		argsList.add(getOwnerINLINE());	// Player ID
		argsList.add(getID());	// Unit ID
		argsList.add(pPlot->getX());	// Plot X
		argsList.add(pPlot->getY());	// Plot Y
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "unitCannotMoveInto", argsList.makeFunctionArgs(), &lResult);

		if (lResult != 0)
		{
			return false;
		}
	}

	return true;
}


bool CvUnit::canMoveOrAttackInto(const CvPlot* pPlot, bool bDeclareWar) const
{
	return (canMoveInto(pPlot, false, bDeclareWar) || canMoveInto(pPlot, true, bDeclareWar));
}


/* bool CvUnit::canMoveThrough(const CvPlot* pPlot, bool bDeclareWar) const
{
	return canMoveInto(pPlot, false, bDeclareWar, true);
}*/

// <advc.030>
bool CvUnit::canEnterArea(CvArea const& a) const {

	return a.canBeEntered(*area(), this);
} // </advc.030>


void CvUnit::attack(CvPlot* pPlot, bool bQuick)
{
	/**
	*** Note: this assertion could fail in certain situations involving sea-patrol - Karadoc
	**/
	FAssert(canMoveInto(pPlot, true));
	FAssert(getCombatTimer() == 0);

	setAttackPlot(pPlot, false);

	updateCombat(bQuick);
}

void CvUnit::fightInterceptor(const CvPlot* pPlot, bool bQuick)
{
	FAssert(getCombatTimer() == 0);

	setAttackPlot(pPlot, true);

	updateAirCombat(bQuick);
}

void CvUnit::attackForDamage(CvUnit *pDefender, int attackerDamageChange, int defenderDamageChange)
{
	FAssert(getCombatTimer() == 0);
	FAssert(pDefender != NULL);
	FAssert(!isFighting());

	if(pDefender == NULL)
	{
		return;
	}

	setAttackPlot(pDefender->plot(), false);

	CvPlot* pPlot = getAttackPlot();
	if (pPlot == NULL)
	{
		return;
	}

	//rotate to face plot
	DirectionTypes newDirection = estimateDirection(this->plot(), pDefender->plot());
	if(newDirection != NO_DIRECTION)
	{
		setFacingDirection(newDirection);
	}

	//rotate enemy to face us
	newDirection = estimateDirection(pDefender->plot(), this->plot());
	if(newDirection != NO_DIRECTION)
	{
		pDefender->setFacingDirection(newDirection);
	}

	//check if quick combat
	bool bVisible = isCombatVisible(pDefender);

	//if not finished and not fighting yet, set up combat damage and mission
	if (!isFighting())
	{
		if (plot()->isFighting() || pPlot->isFighting())
		{
			return;
		}

		setCombatUnit(pDefender, true);
		pDefender->setCombatUnit(this, false);

		pDefender->getGroup()->clearMissionQueue();

		bool bFocused = (bVisible && isCombatFocus() && gDLL->getInterfaceIFace()->isCombatFocus());

		if (bFocused)
		{
			DirectionTypes directionType = directionXY(plot(), pPlot);
			//								N			NE				E				SE					S				SW					W				NW
			NiPoint2 directions[8] = {NiPoint2(0, 1), NiPoint2(1, 1), NiPoint2(1, 0), NiPoint2(1, -1), NiPoint2(0, -1), NiPoint2(-1, -1), NiPoint2(-1, 0), NiPoint2(-1, 1)};
			NiPoint3 attackDirection = NiPoint3(directions[directionType].x, directions[directionType].y, 0);
			float plotSize = GC.getPLOT_SIZE();
			NiPoint3 lookAtPoint(plot()->getPoint().x + plotSize / 2 * attackDirection.x, plot()->getPoint().y + plotSize / 2 * attackDirection.y, (plot()->getPoint().z + pPlot->getPoint().z) / 2);
			attackDirection.Unitize();
			gDLL->getInterfaceIFace()->lookAt(lookAtPoint, (((getOwnerINLINE() != GC.getGameINLINE().getActivePlayer()) || gDLL->getGraphicOption(GRAPHICOPTION_NO_COMBAT_ZOOM)) ? CAMERALOOKAT_BATTLE : CAMERALOOKAT_BATTLE_ZOOM_IN), attackDirection);
		}
		else
		{
			PlayerTypes eAttacker = getVisualOwner(pDefender->getTeam());
			CvWString szMessage;
			if (BARBARIAN_PLAYER != eAttacker)
			{
				szMessage = gDLL->getText("TXT_KEY_MISC_YOU_UNITS_UNDER_ATTACK", GET_PLAYER(getOwnerINLINE()).getNameKey());
			}
			else
			{
				szMessage = gDLL->getText("TXT_KEY_MISC_YOU_UNITS_UNDER_ATTACK_UNKNOWN");
			}

			gDLL->getInterfaceIFace()->addHumanMessage(pDefender->getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szMessage, "AS2D_COMBAT", MESSAGE_TYPE_DISPLAY_ONLY, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true);
		}
	}

	FAssertMsg(plot()->isFighting(), "Current unit instance plot is not fighting as expected");
	FAssertMsg(pPlot->isFighting(), "pPlot is not fighting as expected");

	//setup battle object
	CvBattleDefinition kBattle;
	kBattle.setUnit(BATTLE_UNIT_ATTACKER, this);
	kBattle.setUnit(BATTLE_UNIT_DEFENDER, pDefender);
	kBattle.setDamage(BATTLE_UNIT_ATTACKER, BATTLE_TIME_BEGIN, getDamage());
	kBattle.setDamage(BATTLE_UNIT_DEFENDER, BATTLE_TIME_BEGIN, pDefender->getDamage());

	changeDamage(attackerDamageChange, pDefender->getOwnerINLINE());
	pDefender->changeDamage(defenderDamageChange, getOwnerINLINE());

	if (bVisible)
	{
		kBattle.setDamage(BATTLE_UNIT_ATTACKER, BATTLE_TIME_END, getDamage());
		kBattle.setDamage(BATTLE_UNIT_DEFENDER, BATTLE_TIME_END, pDefender->getDamage());
		kBattle.setAdvanceSquare(canAdvance(pPlot, 1));

		/* original bts code
		kBattle.addDamage(BATTLE_UNIT_ATTACKER, BATTLE_TIME_RANGED, kBattle.getDamage(BATTLE_UNIT_ATTACKER, BATTLE_TIME_BEGIN));
		kBattle.addDamage(BATTLE_UNIT_DEFENDER, BATTLE_TIME_RANGED, kBattle.getDamage(BATTLE_UNIT_DEFENDER, BATTLE_TIME_BEGIN)); */
		// K-Mod. (the original code looks wrong to me.)
		kBattle.setDamage(BATTLE_UNIT_ATTACKER, BATTLE_TIME_RANGED, kBattle.getDamage(BATTLE_UNIT_ATTACKER, BATTLE_TIME_BEGIN));
		kBattle.setDamage(BATTLE_UNIT_DEFENDER, BATTLE_TIME_RANGED, kBattle.getDamage(BATTLE_UNIT_DEFENDER, BATTLE_TIME_BEGIN));
		// K-Mod end

		//int iTurns = planBattle( kBattle);
		// K-Mod
		std::vector<int> combat_log;
		combat_log.push_back(defenderDamageChange);
		combat_log.push_back(-attackerDamageChange);
		int iTurns = planBattle(kBattle, combat_log);
		// K-Mod end
		kBattle.setMissionTime(iTurns * gDLL->getSecsPerTurn());
		setCombatTimer(iTurns);

		GC.getGameINLINE().incrementTurnTimer(getCombatTimer());

		if (pPlot->isActiveVisible(false))
		{
			ExecuteMove(0.5f, true);
			gDLL->getEntityIFace()->AddMission(&kBattle);
		}
	}
	else
	{
		setCombatTimer(1);
	}
}


void CvUnit::move(CvPlot* pPlot, bool bShow)
{
	FAssert(canMoveOrAttackInto(pPlot) || isMadeAttack());

	CvPlot* pOldPlot = plot();

	changeMoves(pPlot->movementCost(this, plot()));

	setXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), true, true, bShow && pPlot->isVisibleToWatchingHuman(), bShow);

	//change feature
	FeatureTypes featureType = pPlot->getFeatureType();
	if(featureType != NO_FEATURE)
	{
		CvString featureString(GC.getFeatureInfo(featureType).getOnUnitChangeTo());
		if(!featureString.IsEmpty())
		{
			FeatureTypes newFeatureType = (FeatureTypes) GC.getInfoTypeForString(featureString);
			pPlot->setFeatureType(newFeatureType);
		}
	}

	if (getOwnerINLINE() == GC.getGameINLINE().getActivePlayer())
	{
		if (!(pPlot->isOwned()))
		{
			//spawn birds if trees present - JW
			if (featureType != NO_FEATURE)
			{
				if (GC.getASyncRand().get(100) < GC.getFeatureInfo(featureType).getEffectProbability())
				{
					EffectTypes eEffect = (EffectTypes)GC.getInfoTypeForString(GC.getFeatureInfo(featureType).getEffectType());
					gDLL->getEngineIFace()->TriggerEffect(eEffect, pPlot->getPoint(), (float)(GC.getASyncRand().get(360)));
					gDLL->getInterfaceIFace()->playGeneralSound("AS3D_UN_BIRDS_SCATTER", pPlot->getPoint());
				}
			}
		}
	}

	CvEventReporter::getInstance().unitMove(pPlot, this, pOldPlot);
}

// false if unit is killed
// K-Mod, added bForceMove and bGroup
bool CvUnit::jumpToNearestValidPlot(bool bGroup, bool bForceMove)
{
	CvCity* pNearestCity;
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iValue;
	int iBestValue;
	int iI;

	FAssertMsg(!isAttacking(), "isAttacking did not return false as expected");
	FAssertMsg(!isFighting(), "isFighting did not return false as expected");

	pNearestCity = GC.getMapINLINE().findCity(getX_INLINE(), getY_INLINE(), getOwnerINLINE());

	iBestValue = MAX_INT;
	pBestPlot = NULL;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (pLoopPlot->isValidDomainForLocation(*this))
		{
			if (canMoveInto(pLoopPlot))
			{
				if (canEnterArea(pLoopPlot->getTeam(), pLoopPlot->area()) && !isEnemy(pLoopPlot->getTeam(), pLoopPlot))
				{
					FAssertMsg(!atPlot(pLoopPlot), "atPlot(pLoopPlot) did not return false as expected");

					if ((getDomainType() != DOMAIN_AIR) || pLoopPlot->isFriendlyCity(*this, true))
					{
						if (pLoopPlot->isRevealed(getTeam(), false))
						{
							iValue = (plotDistance(getX_INLINE(), getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()) * 2);
							// K-mod, 2/jan/11, karadoc - bForceMove functionality
							if (bForceMove && iValue == 0)
								continue;
							// K-Mod end

							if (pNearestCity != NULL)
							{
								iValue += plotDistance(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE());
							}

							if (getDomainType() == DOMAIN_SEA && !plot()->isWater())
							{
								if (!pLoopPlot->isWater() || !pLoopPlot->isAdjacentToArea(area()))
								{
									iValue *= 3;
								}
							}
							else
							{
								if (pLoopPlot->area() != area())
								{
									iValue *= 3;
								}
							}

							if (iValue < iBestValue)
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

	bool bValid = true;
	if (pBestPlot != NULL)
	{
		// K-Mod. If a unit is bumped, we should clear their mission queue
		if (pBestPlot != plot())
			getGroup()->clearMissionQueue();
		// K-Mod end
		setXY(pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), bGroup);
	}
	else
	{
		kill(false);
		bValid = false;
	}

	return bValid;
}


bool CvUnit::canAutomate(AutomateTypes eAutomate) const
{
	if (eAutomate == NO_AUTOMATE)
	{
		return false;
	}

	/*if (!isGroupHead())
	{
		return false;
	}*/ // disabled by K-Mod

	switch (eAutomate)
	{
	case AUTOMATE_BUILD:
		if ((AI_getUnitAIType() != UNITAI_WORKER) && (AI_getUnitAIType() != UNITAI_WORKER_SEA))
		{
			return false;
		}
		break;

	case AUTOMATE_NETWORK:
		if ((AI_getUnitAIType() != UNITAI_WORKER) || !canBuildRoute())
		{
			return false;
		}
		break;

	case AUTOMATE_CITY:
		if (AI_getUnitAIType() != UNITAI_WORKER)
		{
			return false;
		}
		break;

	case AUTOMATE_EXPLORE:
		/* original bts code
		if ((!canFight() && (getDomainType() != DOMAIN_SEA)) || (getDomainType() == DOMAIN_AIR) || (getDomainType() == DOMAIN_IMMOBILE))
		{
			return false;
		}
		break; */
		switch (getDomainType())
		{
		case DOMAIN_IMMOBILE:
			return false;
		case DOMAIN_LAND:
			return canFight() || isSpy() || alwaysInvisible();
		case DOMAIN_AIR:
			return canRecon(NULL);
		default: // sea
			return true;
		}
		break;

	case AUTOMATE_RELIGION:
		if (AI_getUnitAIType() != UNITAI_MISSIONARY)
		{
			return false;
		}
		break;

	default:
		FAssert(false);
		break;
	}

	return true;
}


void CvUnit::automate(AutomateTypes eAutomate)
{
	if (!canAutomate(eAutomate))
	{
		return;
	}

	getGroup()->setAutomateType(eAutomate);
	// K-Mod. I'd like for the unit to automate immediately after the command is given, just so that the UI seems responsive. But doing so is potentially problematic.
	if (isGroupHead() && GET_PLAYER(getOwnerINLINE()).isTurnActive() && getGroup()->readyToMove())
	{
		// unfortunately, CvSelectionGroup::AI_update can kill the unit, and CvUnit::automate currently isn't allowed to kill the unit.
		// CvUnit::AI_update should be ok; but using it here makes me a bit uncomfortable because it doesn't include all the checks and conditions of the group update.
		// ...too bad.
		AI_update();
	}
	// K-Mod end
}


bool CvUnit::canScrap() const
{
	if (plot()->isFighting())
	{
		return false;
	}

	return true;
}


void CvUnit::scrap()
{
	if (!canScrap())
	{
		return;
	}

	kill(true);
}


bool CvUnit::canGift(bool bTestVisible, bool bTestTransport)
{
	CvPlot* pPlot = plot();
	CvUnit* pTransport = getTransportUnit();

	if (!(pPlot->isOwned()))
	{
		return false;
	}

	if (pPlot->getOwnerINLINE() == getOwnerINLINE())
	{
		return false;
	}
	// <advc.143b>
	if(m_pUnitInfo->getNukeRange() >= 0) // -1 for non-nukes
		return false; // </advc.143b>
	// <advc.034>
	if(!TEAMREF(getOwnerINLINE()).isOpenBorders(TEAMID(pPlot->getOwnerINLINE())))
		return false; // </advc.034>

	if (pPlot->isVisibleEnemyUnit(this))
	{
		return false;
	}

	if (pPlot->isVisibleEnemyUnit(pPlot->getOwnerINLINE()))
	{
		return false;
	}

	if (!pPlot->isValidDomainForLocation(*this) && NULL == pTransport)
	{
		return false;
	}

	for (int iCorp = 0; iCorp < GC.getNumCorporationInfos(); ++iCorp)
	{
		if (m_pUnitInfo->getCorporationSpreads(iCorp) > 0)
		{
			return false;
		}
	}

	if (bTestTransport)
	{
		if (pTransport && pTransport->getTeam() != pPlot->getTeam())
		{
			return false;
		}
	}

	if (!bTestVisible)
	{
		if (GET_TEAM(pPlot->getTeam()).isUnitClassMaxedOut(getUnitClassType(), GET_TEAM(pPlot->getTeam()).getUnitClassMaking(getUnitClassType())))
		{
			return false;
		}

		if (GET_PLAYER(pPlot->getOwnerINLINE()).isUnitClassMaxedOut(getUnitClassType(), GET_PLAYER(pPlot->getOwnerINLINE()).getUnitClassMaking(getUnitClassType())))
		{
			return false;
		}

		if (!(GET_PLAYER(pPlot->getOwnerINLINE()).AI_acceptUnit(this)))
		{
			return false;
		}
	}

	/* <advc.123a> Disallow gifting of missionaries that don't match the
	   (intolerant) state religion. */
	CvPlayer& benef = GET_PLAYER(pPlot->getOwnerINLINE());
	ReligionTypes benefR = benef.getStateReligion();
	if(benefR != NO_RELIGION && AI_getUnitAIType() == UNITAI_MISSIONARY &&
	        benef.isNoNonStateReligionSpread() &&
			benefR != GC.getUnitInfo(getUnitType()).getReligionType())
		return false;
	// <dlph.4>
	std::vector<CvUnit*> aCargoUnits;
	getCargoUnits(aCargoUnits);
	for(size_t i = 0; i < aCargoUnits.size(); i++)
		if(!aCargoUnits[i]->canGift(false, false))
			return false; // </dlph.4>
	// </advc.123a>
	// <advc.705>
	CvGame const& g = GC.getGameINLINE();
	if(g.isOption(GAMEOPTION_RISE_FALL) && isHuman() &&
			g.getRiseFall().isCooperationRestricted(pPlot->getOwnerINLINE()))
		return false; // </advc.705>
	return !atWar(pPlot->getTeam(), getTeam());
}


void CvUnit::gift(bool bTestTransport)
{
	if (!canGift(false, bTestTransport))
	{
		return;
	}

	std::vector<CvUnit*> aCargoUnits;
	getCargoUnits(aCargoUnits);
	for (uint i = 0; i < aCargoUnits.size(); ++i)
	{
		aCargoUnits[i]->gift(false);
	}

	FAssertMsg(plot()->getOwnerINLINE() != NO_PLAYER, "plot()->getOwnerINLINE() is not expected to be equal with NO_PLAYER");
	CvPlayerAI& kRecievingPlayer = GET_PLAYER(plot()->getOwnerINLINE()); // K-Mod

	CvUnit* pGiftUnit = kRecievingPlayer.initUnit(getUnitType(), getX_INLINE(), getY_INLINE(), AI_getUnitAIType());
	FAssertMsg(pGiftUnit != NULL, "GiftUnit is not assigned a valid value");

	pGiftUnit->convert(this);

	PlayerTypes eOwner = getOwnerINLINE();

	//GET_PLAYER(pGiftUnit->getOwnerINLINE()).AI_changePeacetimeGrantValue(eOwner, (pGiftUnit->getUnitInfo().getProductionCost() / 5));
	// K-Mod
	// advc.141 (commented out):
	/*if (pGiftUnit->isGoldenAge())
	{
		kRecievingPlayer.AI_changeMemoryCount(eOwner, MEMORY_GIVE_HELP, 1);
	}*/
	// Note: I'm not currently considering special units with < 0 production cost.
	if (pGiftUnit->canCombat()) // dlph.8: was isCombat
	{
		int iEffectiveWarRating = plot()->area()->getAreaAIType(kRecievingPlayer.getTeam()) != AREAAI_NEUTRAL
			? GET_TEAM(kRecievingPlayer.getTeam()).AI_getWarSuccessRating()
			: 60 - (kRecievingPlayer.AI_isDoStrategy(AI_STRATEGY_ALERT1) ? 20 : 0) - (kRecievingPlayer.AI_isDoStrategy(AI_STRATEGY_ALERT2) ? 20 : 0);

		int iUnitValue = std::max(0, kRecievingPlayer.AI_unitValue(pGiftUnit->getUnitType(), pGiftUnit->AI_getUnitAIType(), plot()->area()));
		int iBestValue = kRecievingPlayer.AI_bestAreaUnitAIValue(pGiftUnit->AI_getUnitAIType(), plot()->area());

		int iGiftValue = pGiftUnit->getUnitInfo().getProductionCost() * 4 * std::min(300, 100*iUnitValue/std::max(1, iBestValue)) / 100;
		iGiftValue *= 100;
		iGiftValue /= std::max(20, 110 + 3*iEffectiveWarRating);

		if (iUnitValue <= iBestValue && kRecievingPlayer.AI_unitCostPerMil() > kRecievingPlayer.AI_maxUnitCostPerMil(plot()->area()))
		{
			iGiftValue /= 2;
		}
		kRecievingPlayer.AI_changePeacetimeGrantValue(eOwner, iGiftValue);
		// TODO: It would nice if there was some way this could also reduce "you refused to help us during war time", and stuff like that.
		//       But I think that would probably require some additional AI memory.
	}
	else
	{
		kRecievingPlayer.AI_changePeacetimeGrantValue(eOwner, pGiftUnit->getUnitInfo().getProductionCost() / 2);
	}
	// K-Mod end

	CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_GIFTED_UNIT_TO_YOU", GET_PLAYER(eOwner).getNameKey(), pGiftUnit->getNameKey());
	gDLL->getInterfaceIFace()->addHumanMessage(pGiftUnit->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_UNITGIFTED", MESSAGE_TYPE_INFO, pGiftUnit->getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"), pGiftUnit->getX_INLINE(), pGiftUnit->getY_INLINE(), true, true);

	// Python Event
	CvEventReporter::getInstance().unitGifted(pGiftUnit, getOwnerINLINE(), plot());
}


bool CvUnit::canLoadUnit(const CvUnit* pUnit, const CvPlot* pPlot
	, bool checkMoves // advc.123c
	) const
{
	FAssert(pUnit != NULL);
	FAssert(pPlot != NULL);

	if (pUnit == this)
	{
		return false;
	}

	if (pUnit->getTeam() != getTeam())
	{
		return false;
	}

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       06/23/10                     Mongoose & jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
	// From Mongoose SDK
	if (isCargo() && getTransportUnit() == pUnit)
	{
		return false;
	}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/

	if (getCargo() > 0)
	{
		return false;
	}

	if (pUnit->isCargo())
	{
		return false;
	}

	if (!(pUnit->cargoSpaceAvailable(getSpecialUnitType(), getDomainType())))
	{
		return false;
	}

	if (!(pUnit->atPlot(pPlot)))
	{
		return false;
	}

	if (!m_pUnitInfo->isHiddenNationality() && pUnit->getUnitInfo().isHiddenNationality())
	{
		return false;
	}

	if (NO_SPECIALUNIT != getSpecialUnitType())
	{
		if (GC.getSpecialUnitInfo(getSpecialUnitType()).isCityLoad())
		{
			if (!pPlot->isCity(true, getTeam()))
			{
				return false;
			}
		}
	}
	// <advc.123c>
	if(checkMoves && getDomainType() != DOMAIN_AIR && movesLeft() <= 0)
		return false; // </advc.123>
	return true;
}


void CvUnit::loadUnit(CvUnit* pUnit)
{
	if (!canLoadUnit(pUnit, plot()))
	{
		return;
	}

	setTransportUnit(pUnit);
}

bool CvUnit::shouldLoadOnMove(const CvPlot* pPlot) const
{
	if (isCargo())
	{
		return false;
	}

	switch (getDomainType())
	{
	case DOMAIN_LAND:
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       10/30/09                     Mongoose & jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original bts code
		if (pPlot->isWater())
*/
		// From Mongoose SDK
		if (pPlot->isWater() && !canMoveAllTerrain())
		{
			return true;
		}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
		break;
	case DOMAIN_AIR:
		if (!pPlot->isFriendlyCity(*this, true))
		{
			return true;
		}

		if (m_pUnitInfo->getAirUnitCap() > 0)
		{
			if (pPlot->airUnitSpaceAvailable(getTeam()) <= 0)
			{
				return true;
			}
		}
		break;
	default:
		break;
	}

	if (m_pUnitInfo->getTerrainImpassable(pPlot->getTerrainType())) 
	{ 
		TechTypes eTech = (TechTypes)m_pUnitInfo->getTerrainPassableTech(pPlot->getTerrainType()); 
		if (NO_TECH == eTech || !GET_TEAM(getTeam()).isHasTech(eTech)) 
		{ 
			return true; 
		} 
	} 

	return false;
}


bool CvUnit::canLoad(const CvPlot* pPlot
	, bool checkMoves // advc.123c
	) const
{
	PROFILE_FUNC();

	FAssert(pPlot != NULL);

	CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = pPlot->nextUnitNode(pUnitNode);

		if (canLoadUnit(pLoopUnit, pPlot
			, checkMoves // advc.123c
			))
		{
			return true;
		}
	}

	return false;
}


void CvUnit::load()
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pPlot;
	int iPass;

	if (!canLoad(plot()))
	{
		return;
	}

	pPlot = plot();

	for (iPass = 0; iPass < 2; iPass++)
	{
		pUnitNode = pPlot->headUnitNode();

		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = pPlot->nextUnitNode(pUnitNode);

			if (canLoadUnit(pLoopUnit, pPlot))
			{
				if ((iPass == 0) ? (pLoopUnit->getOwnerINLINE() == getOwnerINLINE()) : (pLoopUnit->getTeam() == getTeam()))
				{
					setTransportUnit(pLoopUnit);
					break;
				}
			}
		}

		if (isCargo())
		{
			break;
		}
	}
}


bool CvUnit::canUnload() const
{
	CvPlot& kPlot = *(plot());

	if (getTransportUnit() == NULL)
	{
		return false;
	}

	if (!kPlot.isValidDomainForLocation(*this))
	{
		return false;
	}

	if (getDomainType() == DOMAIN_AIR)
	{
		if (kPlot.isFriendlyCity(*this, true))
		{
			int iNumAirUnits = kPlot.countNumAirUnits(getTeam());
			CvCity* pCity = kPlot.getPlotCity();
			if (NULL != pCity)
			{
				if (iNumAirUnits >= pCity->getAirUnitCapacity(getTeam()))
				{
					return false;
				}
			}
			else
			{
				if (iNumAirUnits >= GC.getDefineINT("CITY_AIR_UNIT_CAPACITY"))
				{
					return false;
				}
			}
		}
	}

	return true;
}


void CvUnit::unload()
{
	if (!canUnload())
	{
		return;
	}

	setTransportUnit(NULL);
}


bool CvUnit::canUnloadAll() const
{
	if (getCargo() == 0)
	{
		return false;
	}
	/*  advc.123c: Not sure if unloading at sea causes problems, but doesn't
		accomplish anything (at least not with the change to canLoadUnit) */
	if(plot()->isWater()) return false;
	return true;
}


void CvUnit::unloadAll()
{
	if (!canUnloadAll())
	{
		return;
	}

	std::vector<CvUnit*> aCargoUnits;
	getCargoUnits(aCargoUnits);
	for (uint i = 0; i < aCargoUnits.size(); ++i)
	{
		CvUnit* pCargo = aCargoUnits[i];
		if (pCargo->canUnload())
		{
			pCargo->setTransportUnit(NULL);
		}
		else
		{
			FAssert(isHuman() || pCargo->getDomainType() == DOMAIN_AIR);
			pCargo->getGroup()->setActivityType(ACTIVITY_AWAKE);
		}
	}
}


bool CvUnit::canHold(const CvPlot* pPlot) const
{
	return true;
}


bool CvUnit::canSleep(const CvPlot* pPlot) const
{
	if (isFortifyable())
	{
		return false;
	}

	//if (isWaiting())
	if (getGroup()->getActivityType() == ACTIVITY_SLEEP) // K-Mod
	{
		return false;
	}

	return true;
}


bool CvUnit::canFortify(const CvPlot* pPlot) const
{
	if (!isFortifyable())
	{
		return false;
	}

	//if (isWaiting())
	if (getGroup()->getActivityType() == ACTIVITY_SLEEP) // K-Mod
	{
		return false;
	}

	return true;
}


bool CvUnit::canAirPatrol(const CvPlot* pPlot) const
{
	if (getDomainType() != DOMAIN_AIR)
	{
		return false;
	}

	if (!canAirDefend(pPlot))
	{
		return false;
	}

	//if (isWaiting())
	if (getGroup()->getActivityType() == ACTIVITY_INTERCEPT) // K-Mod
	{
		return false;
	}

	return true;
}


bool CvUnit::canSeaPatrol(const CvPlot* pPlot) const
{
	if (!pPlot->isWater())
	{
		return false;
	}

	if (getDomainType() != DOMAIN_SEA)
	{
		return false;
	}

	if (!canFight() || isOnlyDefensive())
	{
		return false;
	}

	//if (isWaiting())
	if (getGroup()->getActivityType() == ACTIVITY_PATROL) // K-Mod
	{
		return false;
	}

	return true;
}


void CvUnit::airCircle(bool bStart)
{
	if (!GC.IsGraphicsInitialized())
	{
		return;
	}

	if ((getDomainType() != DOMAIN_AIR) || (maxInterceptionProbability() == 0))
	{
		return;
	}

	//cancel previos missions
	gDLL->getEntityIFace()->RemoveUnitFromBattle( this );

	if (bStart)
	{
		CvAirMissionDefinition kDefinition;
		kDefinition.setPlot(plot());
		kDefinition.setUnit(BATTLE_UNIT_ATTACKER, this);
		kDefinition.setUnit(BATTLE_UNIT_DEFENDER, NULL);
		kDefinition.setMissionType(MISSION_AIRPATROL);
		kDefinition.setMissionTime(1.0f); // patrol is indefinite - time is ignored

		gDLL->getEntityIFace()->AddMission( &kDefinition );
	}
}


bool CvUnit::canHeal(const CvPlot* pPlot) const
{
	if (!isHurt())
		return false;

	//if (isWaiting())
	if (getGroup()->getActivityType() == ACTIVITY_HEAL) // K-Mod
		return false;

	if (healTurns(pPlot) == MAX_INT)
		return false;

	return true;
}


bool CvUnit::canSentry(const CvPlot* pPlot) const
{
	if (!canDefend(pPlot))
	{
		return false;
	}

	//if (isWaiting())
	if (getGroup()->getActivityType() == ACTIVITY_SENTRY) // K-Mod
	{
		return false;
	}

	return true;
}


int CvUnit::healRate(const CvPlot* pPlot, bool bLocation, bool bUnits) const
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvCity* pCity;
	CvUnit* pLoopUnit;
	CvPlot* pLoopPlot;
	int iTotalHeal;
	int iHeal;
	int iBestHeal;
	int iI;

	pCity = pPlot->getPlotCity();

	iTotalHeal = 0;

	if (bLocation) // K-Mod
	{
		if (pPlot->isCity(true, getTeam()))
		{
			iTotalHeal += GC.getDefineINT("CITY_HEAL_RATE") + (GET_TEAM(getTeam()).isFriendlyTerritory(pPlot->getTeam()) ? getExtraFriendlyHeal() : getExtraNeutralHeal());
			if (pCity && !pCity->isOccupation())
			{
				iTotalHeal += pCity->getHealRate();
			}
		}
		else
		{
			if (!GET_TEAM(getTeam()).isFriendlyTerritory(pPlot->getTeam()))
			{
				if (isEnemy(pPlot->getTeam(), pPlot))
				{
					iTotalHeal += (GC.getDefineINT("ENEMY_HEAL_RATE") + getExtraEnemyHeal());
				}
				else
				{
					iTotalHeal += (GC.getDefineINT("NEUTRAL_HEAL_RATE") + getExtraNeutralHeal());
				}
			}
			else
			{
				iTotalHeal += (GC.getDefineINT("FRIENDLY_HEAL_RATE") + getExtraFriendlyHeal());
			}
		}
	}

	if (bUnits) // K-Mod
	{
		// XXX optimize this (save it?)
		iBestHeal = 0;

		pUnitNode = pPlot->headUnitNode();

		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = pPlot->nextUnitNode(pUnitNode);

			if (pLoopUnit->getTeam() == getTeam()) // XXX what about alliances?
			{
				iHeal = pLoopUnit->getSameTileHeal();

				if (iHeal > iBestHeal)
				{
					iBestHeal = iHeal;
				}
			}
		}

		for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
		{
			pLoopPlot = plotDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), ((DirectionTypes)iI));

			if (pLoopPlot != NULL)
			{	// advc.030: Instead check domain type below
				//if (pLoopPlot->area() == pPlot->area())
				{
					pUnitNode = pLoopPlot->headUnitNode();

					while (pUnitNode != NULL)
					{
						pLoopUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

						if (	// <advc.030>
								pLoopUnit->getDomainType() == getDomainType() &&
								!pLoopUnit->isCargo() && // </advc.030>
								pLoopUnit->getTeam() == getTeam()) // XXX what about alliances?
						{
							iHeal = pLoopUnit->getAdjacentTileHeal();

							if (iHeal > iBestHeal)
							{
								iBestHeal = iHeal;
							}
						}
					}
				}
			}
		}

		iTotalHeal += iBestHeal;
		// XXX
	}

	return iTotalHeal;
}


int CvUnit::healTurns(const CvPlot* pPlot) const
{
	if (!isHurt())
		return 0;

	int iHeal = healRate(pPlot);

/*************************************************************************************************/
/* UNOFFICIAL_PATCH                       06/02/10                           LunarMongoose       */
/*                                                                                               */
/* Bugfix                                                                                        */
/*************************************************************************************************/
	// Mongoose FeatureDamageFix
	FeatureTypes eFeature = pPlot->getFeatureType();
	if (eFeature != NO_FEATURE)
	{
		iHeal -= GC.getFeatureInfo(eFeature).getTurnDamage();
	}
/*************************************************************************************************/
/* UNOFFICIAL_PATCH                         END                                                  */
/*************************************************************************************************/

	if (iHeal > 0)
	{
		/*iTurns = (getDamage() / iHeal);

		if ((getDamage() % iHeal) != 0)
		{
			iTurns++;
		}

		return iTurns; */
		return (getDamage() + iHeal-1) / iHeal; // K-Mod (same, but faster)
	}
	else
	{
		return MAX_INT;
	}
}


void CvUnit::doHeal()
{
	changeDamage(-(healRate(plot())));
}


bool CvUnit::canAirlift(const CvPlot* pPlot) const
{
	CvCity* pCity;

	if (getDomainType() != DOMAIN_LAND)
	{
		return false;
	}

	if (hasMoved())
	{
		return false;
	}

	pCity = pPlot->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (pCity->getCurrAirlift() >= pCity->getMaxAirlift())
	{
		return false;
	}

	if (pCity->getTeam() != getTeam())
	{
		return false;
	}

	return true;
}


bool CvUnit::canAirliftAt(const CvPlot* pPlot, int iX, int iY) const
{
	CvPlot* pTargetPlot;
	CvCity* pTargetCity;

	if (!canAirlift(pPlot))
	{
		return false;
	}

	pTargetPlot = GC.getMapINLINE().plotINLINE(iX, iY);

	// canMoveInto use to be here

	pTargetCity = pTargetPlot->getPlotCity();

	if (pTargetCity == NULL)
	{
		return false;
	}

	if (pTargetCity->isAirliftTargeted())
	{
		return false;
	}

	if (pTargetCity->getTeam() != getTeam() && !GET_TEAM(pTargetCity->getTeam()).isVassal(getTeam()))
	{
		return false;
	}

	if (!canMoveInto(pTargetPlot)) // moved by K-Mod
	{
		return false;
	}

	return true;
}


bool CvUnit::airlift(int iX, int iY)
{
	CvCity* pCity;
	CvCity* pTargetCity;
	CvPlot* pTargetPlot;

	if (!canAirliftAt(plot(), iX, iY))
	{
		return false;
	}

	pCity = plot()->getPlotCity();
	FAssert(pCity != NULL);
	pTargetPlot = GC.getMapINLINE().plotINLINE(iX, iY);
	FAssert(pTargetPlot != NULL);
	pTargetCity = pTargetPlot->getPlotCity();
	FAssert(pTargetCity != NULL);
	FAssert(pCity != pTargetCity);

	pCity->changeCurrAirlift(1);
	if (pTargetCity->getMaxAirlift() == 0)
	{
		pTargetCity->setAirliftTargeted(true);
	}

	finishMoves();

	AutomateTypes eAuto = getGroup()->getAutomateType(); // K-Mod
	setXY(pTargetPlot->getX_INLINE(), pTargetPlot->getY_INLINE());
	getGroup()->setAutomateType(eAuto); // K-Mod. (automated workers sometimes airlift themselves. They should stay automated.)

	return true;
}


bool CvUnit::isNukeVictim(const CvPlot* pPlot, TeamTypes eTeam) const
{
	CvPlot* pLoopPlot;
	int iDX, iDY;

	if (!(GET_TEAM(eTeam).isAlive()))
	{
		return false;
	}

	if (eTeam == getTeam())
	{
		return false;
	}

	for (iDX = -(nukeRange()); iDX <= nukeRange(); iDX++)
	{
		for (iDY = -(nukeRange()); iDY <= nukeRange(); iDY++)
		{
			pLoopPlot	= plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->getTeam() == eTeam)
				{
					return true;
				}

				if (pLoopPlot->plotCheck(PUF_isCombatTeam, eTeam, getTeam()) != NULL
					&& isEnemy(eTeam) // dlph.7
					)
				{
					return true;
				}
			}
		}
	}

	return false;
}


bool CvUnit::canNuke(const CvPlot* pPlot) const
{
	if (nukeRange() == -1)
	{
		return false;
	}

	return true;
}


bool CvUnit::canNukeAt(const CvPlot* pPlot, int iX, int iY) const
{
	CvPlot* pTargetPlot;
	int iI;

	if (!canNuke(pPlot))
	{
		return false;
	}

	int iDistance = plotDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iX, iY);
	if (iDistance <= nukeRange())
	{
		return false;
	}

	if (airRange() > 0 && iDistance > airRange())
	{
		return false;
	}

	pTargetPlot = GC.getMapINLINE().plotINLINE(iX, iY);

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (isNukeVictim(pTargetPlot, ((TeamTypes)iI)))
		{
			if (!isEnemy((TeamTypes)iI, pPlot))
			{
				return false;
			}
		}
	}

	return true;
}


bool CvUnit::nuke(int iX, int iY)
{
	CvPlot* pPlot;
	CvWString szBuffer;
	int abTeamsAffected[MAX_TEAMS]; // advc.130q: was bool
	TeamTypes eBestTeam;
	int iBestInterception;
	int iI, iJ, iK;

	if (!canNukeAt(plot(), iX, iY))
	{
		return false;
	}

	pPlot = GC.getMapINLINE().plotINLINE(iX, iY);

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		abTeamsAffected[iI] = isNukeVictim(pPlot, ((TeamTypes)iI));
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (abTeamsAffected[iI])
		{
			if (!isEnemy((TeamTypes)iI))
			{
				GET_TEAM(getTeam()).declareWar(((TeamTypes)iI), false, WARPLAN_LIMITED);
			}
		}
	}

	iBestInterception = 0;
	eBestTeam = NO_TEAM;

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (abTeamsAffected[iI])
		{
			if (GET_TEAM((TeamTypes)iI).getNukeInterception() > iBestInterception)
			{
				iBestInterception = GET_TEAM((TeamTypes)iI).getNukeInterception();
				eBestTeam = ((TeamTypes)iI);
			}
			// <advc.143b>
			CvTeam const& affected = GET_TEAM((TeamTypes)iI);
			if(affected.isAVassal()) {
				int masterChance = GET_TEAM(affected.getMasterTeam()).
						getNukeInterception();
				if(masterChance > iBestInterception) {
					iBestInterception = masterChance;
					eBestTeam = affected.getMasterTeam();
				}
			} // </advc.143b>
		}
	}

	iBestInterception *= (100 - m_pUnitInfo->getEvasionProbability());
	iBestInterception /= 100;

	setReconPlot(pPlot);

	if (GC.getGameINLINE().getSorenRandNum(100, "Nuke") < iBestInterception)
	{
		for (iI = 0; iI < MAX_PLAYERS; iI++)
		{
			//if (GET_PLAYER((PlayerTypes)iI).isAlive())
			// K-Mod. Only show the message to players who have met the teams involved!
			const CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
			if (kLoopPlayer.isAlive() && (GET_TEAM(kLoopPlayer.getTeam()).isHasMet(getTeam()) && GET_TEAM(kLoopPlayer.getTeam()).isHasMet(eBestTeam)
					|| kLoopPlayer.isSpectator())) // advc.127
			// K-Mod end
			{
				szBuffer = gDLL->getText("TXT_KEY_MISC_NUKE_INTERCEPTED", GET_PLAYER(getOwnerINLINE()).getNameKey(), getNameKey(), GET_TEAM(eBestTeam).getName().GetCString());
				gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), (((PlayerTypes)iI) == getOwnerINLINE()), GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_NUKE_INTERCEPTED", MESSAGE_TYPE_MAJOR_EVENT, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true, true);
			}
		}

		if (pPlot->isActiveVisible(false))
		{
			// Nuke entity mission
			CvMissionDefinition kDefiniton;
			kDefiniton.setMissionTime(GC.getMissionInfo(MISSION_NUKE).getTime() * gDLL->getSecsPerTurn());
			kDefiniton.setMissionType(MISSION_NUKE);
			kDefiniton.setPlot(pPlot);
			kDefiniton.setUnit(BATTLE_UNIT_ATTACKER, this);
			kDefiniton.setUnit(BATTLE_UNIT_DEFENDER, this);

			// Add the intercepted mission (defender is not NULL)
			gDLL->getEntityIFace()->AddMission(&kDefiniton);
		}

		kill(true);
		return true; // Intercepted!!! (XXX need special event for this...)
	}

	if (pPlot->isActiveVisible(false))
	{
		// Nuke entity mission
		CvMissionDefinition kDefiniton;
		kDefiniton.setMissionTime(GC.getMissionInfo(MISSION_NUKE).getTime() * gDLL->getSecsPerTurn());
		kDefiniton.setMissionType(MISSION_NUKE);
		kDefiniton.setPlot(pPlot);
		kDefiniton.setUnit(BATTLE_UNIT_ATTACKER, this);
		kDefiniton.setUnit(BATTLE_UNIT_DEFENDER, NULL);

		// Add the non-intercepted mission (defender is NULL)
		gDLL->getEntityIFace()->AddMission(&kDefiniton);
	}

	setMadeAttack(true);
	setAttackPlot(pPlot, false);

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (abTeamsAffected[iI])
		{
			GET_TEAM((TeamTypes)iI).changeWarWeariness(getTeam(), 100 * GC.getDefineINT("WW_HIT_BY_NUKE"));
			GET_TEAM(getTeam()).changeWarWeariness(((TeamTypes)iI), 100 * GC.getDefineINT("WW_ATTACKED_WITH_NUKE"));
			GET_TEAM(getTeam()).AI_changeWarSuccess(((TeamTypes)iI), GC.getDefineINT("WAR_SUCCESS_NUKE"));
		}
	}
	// <advc.130q>
	for(int i = 0; i < MAX_TEAMS; i++) {
		TeamTypes tId = (TeamTypes)i;
		// We already know if the team is affected at all
		if(abTeamsAffected[tId] == 0)
			continue;
		// How badly is it affected?
		double score = 0;
		for(int dx = -nukeRange(); dx <= nukeRange(); dx++)
		for(int dy = -nukeRange(); dy <= nukeRange(); dy++) {
			CvPlot* p = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), dx, dy);
			if(p == NULL || !p->isCity())
				continue;
			CvCity const& c = *p->getPlotCity();
			if(c.getTeam() != tId)
				continue;
			score += GET_PLAYER(c.getOwnerINLINE()).razeAngerRating(c);
		}
		if(score >= 1)
			abTeamsAffected[tId] = 2;
		if(score > 8)
			abTeamsAffected[tId] = 3;
	} // </advc.130q>
	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (iI != getTeam())
			{
				/*  advc.130q: Moved the > 0 case to a separate loop b/c it's now
					important to process the nuked-friend penalties first */
				if(abTeamsAffected[iI] <= 0)
				{
					for (iJ = 0; iJ < MAX_TEAMS; iJ++)
					{
						if (GET_TEAM((TeamTypes)iJ).isAlive())
						{
							// <advc.130q> was just if(abTeamsAffected[iJ])
							if(abTeamsAffected[iJ] > 1 && // Don't hate them for striking back
									GET_TEAM(getTeam()).AI_getMemoryCount((TeamTypes)iJ, MEMORY_NUKED_US)
									<= GET_TEAM((TeamTypes)iJ).AI_getMemoryCount(getTeam(), MEMORY_NUKED_US))
									// </advc.130q>
							{
								if (GET_TEAM((TeamTypes)iI).isHasMet((TeamTypes)iJ))
								{
									if (GET_TEAM((TeamTypes)iI).AI_getAttitude((TeamTypes)iJ) >= ATTITUDE_CAUTIOUS)
									{
										for (iK = 0; iK < MAX_PLAYERS; iK++)
										{
											if (GET_PLAYER((PlayerTypes)iK).isAlive())
											{
												if (GET_PLAYER((PlayerTypes)iK).getTeam() == ((TeamTypes)iI))
												{   // advc.130j:
													GET_PLAYER((PlayerTypes)iK).AI_rememberEvent(getOwnerINLINE(), MEMORY_NUKED_FRIEND);
												}
											}
										}
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	// <advc.130q> The nuked-us loop (refactored)
	for(iI = 0; iI < MAX_TEAMS; iI++) {
		CvTeamAI& affectedTeam = GET_TEAM((TeamTypes)iI);
		if(!affectedTeam.isAlive() || affectedTeam.getID() == getTeam() ||
				abTeamsAffected[iI] <= 0)
			continue;
		for(iJ = 0; iJ < MAX_PLAYERS; iJ++) {
			CvPlayerAI& affectedCiv = GET_PLAYER((PlayerTypes)iJ);
			if(!affectedCiv.isAlive() || affectedCiv.getTeam() != affectedTeam.getID())
				continue;
			// <advc.130v>
			CvTeam const& nukingTeam = GET_TEAM(getTeam());
			if(nukingTeam.isCapitulated()) {
				abTeamsAffected[iI] = (int)ceil(abTeamsAffected[iI] / 2.0);
				affectedCiv.AI_changeMemoryCount(
						GET_TEAM(nukingTeam.getMasterTeam()).getLeaderID(),
						MEMORY_NUKED_US, abTeamsAffected[iI]);
			} // </advc.130v>
			// advc.130j:
			affectedCiv.AI_changeMemoryCount(getOwnerINLINE(), MEMORY_NUKED_US,
					abTeamsAffected[iI]);
		}
	} // </advc.130q>

	// XXX some AI should declare war here...

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		//if (GET_PLAYER((PlayerTypes)iI).isAlive())
		// K-Mod
		const CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.isAlive() && (GET_TEAM(kLoopPlayer.getTeam()).isHasMet(getTeam())
				|| kLoopPlayer.isSpectator())) // advc.127
		// K-Mod end
		{
			szBuffer = gDLL->getText("TXT_KEY_MISC_NUKE_LAUNCHED", GET_PLAYER(getOwnerINLINE()).getNameKey(), getNameKey());
			gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), (((PlayerTypes)iI) == getOwnerINLINE()), GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_NUKE_EXPLODES", MESSAGE_TYPE_MAJOR_EVENT, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true, true);
		}
	}

	if (isSuicide())
	{
		kill(true);
	}

	return true;
}


bool CvUnit::canRecon(const CvPlot* pPlot) const
{
	if (getDomainType() != DOMAIN_AIR)
	{
		return false;
	}

	if (airRange() == 0)
	{
		return false;
	}

	if (m_pUnitInfo->isSuicide())
	{
		return false;
	}

	return true;
}



bool CvUnit::canReconAt(const CvPlot* pPlot, int iX, int iY) const
{
	if (!canRecon(pPlot))
	{
		return false;
	}

	int iDistance = plotDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iX, iY);
	if (iDistance > airRange() || 0 == iDistance)
	{
		return false;
	}

	return true;
}


bool CvUnit::recon(int iX, int iY)
{
	CvPlot* pPlot;

	if (!canReconAt(plot(), iX, iY))
	{
		return false;
	}

	pPlot = GC.getMapINLINE().plotINLINE(iX, iY);

	setReconPlot(pPlot);

	finishMoves();

	if (pPlot->isActiveVisible(false))
	{
		CvAirMissionDefinition kAirMission;
		kAirMission.setMissionType(MISSION_RECON);
		kAirMission.setUnit(BATTLE_UNIT_ATTACKER, this);
		kAirMission.setUnit(BATTLE_UNIT_DEFENDER, NULL);
		kAirMission.setDamage(BATTLE_UNIT_DEFENDER, 0);
		kAirMission.setDamage(BATTLE_UNIT_ATTACKER, 0);
		kAirMission.setPlot(pPlot);
		kAirMission.setMissionTime(GC.getMissionInfo((MissionTypes)MISSION_RECON).getTime() * gDLL->getSecsPerTurn());
		gDLL->getEntityIFace()->AddMission(&kAirMission);
	}

	return true;
}


bool CvUnit::canParadrop(const CvPlot* pPlot) const
{
	if (getDropRange() <= 0)
	{
		return false;
	}

	if (hasMoved())
	{
		return false;
	}

	if (!pPlot->isFriendlyCity(*this, true))
	{
		return false;
	}

	return true;
}



bool CvUnit::canParadropAt(const CvPlot* pPlot, int iX, int iY) const
{
	if (!canParadrop(pPlot))
	{
		return false;
	}

	CvPlot* pTargetPlot = GC.getMapINLINE().plotINLINE(iX, iY);
	if (NULL == pTargetPlot || pTargetPlot == pPlot)
	{
		return false;
	}

	if (!pTargetPlot->isVisible(getTeam(), false))
	{
		return false;
	}

	if (!canMoveInto(pTargetPlot, false, false, true))
	{
		return false;
	}

	if (plotDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iX, iY) > getDropRange())
	{
		return false;
	}

	if (!canCoexistWithEnemyUnit(NO_TEAM))
	{
		if (pTargetPlot->isEnemyCity(*this))
		{
			return false;
		}

		if (pTargetPlot->isVisibleEnemyUnit(this))
		{
			return false;
		}
	}

	return true;
}


bool CvUnit::paradrop(int iX, int iY)
{
	if (!canParadropAt(plot(), iX, iY))
	{
		return false;
	}

	CvPlot* pPlot = GC.getMapINLINE().plotINLINE(iX, iY);

	changeMoves(GC.getMOVE_DENOMINATOR() / 2);
	setMadeAttack(true);

	//setXY(pPlot->getX_INLINE(), pPlot->getY_INLINE());
	setXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), true); // K-Mod

	//check if intercepted
	if(interceptTest(pPlot))
	{
		return true;
	}
	
	//play paradrop animation by itself
	if (pPlot->isActiveVisible(false))
	{
		CvAirMissionDefinition kAirMission;
		kAirMission.setMissionType(MISSION_PARADROP);
		kAirMission.setUnit(BATTLE_UNIT_ATTACKER, this);
		kAirMission.setUnit(BATTLE_UNIT_DEFENDER, NULL);
		kAirMission.setDamage(BATTLE_UNIT_DEFENDER, 0);
		kAirMission.setDamage(BATTLE_UNIT_ATTACKER, 0);
		kAirMission.setPlot(pPlot);
		kAirMission.setMissionTime(GC.getMissionInfo((MissionTypes)MISSION_PARADROP).getTime() * gDLL->getSecsPerTurn());
		gDLL->getEntityIFace()->AddMission(&kAirMission);
	}

	return true;
}


bool CvUnit::canAirBomb(const CvPlot* pPlot) const
{
	if (getDomainType() != DOMAIN_AIR)
	{
		return false;
	}

	if (airBombBaseRate() == 0)
	{
		return false;
	}

	if (isMadeAttack())
	{
		return false;
	}

	return true;
}


bool CvUnit::canAirBombAt(const CvPlot* pPlot, int iX, int iY) const
{
	CvCity* pCity;
	CvPlot* pTargetPlot;

	if (!canAirBomb(pPlot))
	{
		return false;
	}

	pTargetPlot = GC.getMapINLINE().plotINLINE(iX, iY);

	if (plotDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pTargetPlot->getX_INLINE(), pTargetPlot->getY_INLINE()) > airRange())
	{
		return false;
	}

	if (pTargetPlot->isOwned())
	{
		if (!potentialWarAction(pTargetPlot))
		{
			return false;
		}
	}

	pCity = pTargetPlot->getPlotCity();

	if (pCity != NULL)
	{
		//if (!(pCity->isBombardable(this)))
		if (!pCity->isBombardable(this) || !pCity->isRevealed(getTeam(), false)) // K-Mod
		{
			return false;
		}
	}
	else
	{
		/* original bts code
		if (pTargetPlot->getImprovementType() == NO_IMPROVEMENT)
		{
			return false;
		}

		if (GC.getImprovementInfo(pTargetPlot->getImprovementType()).isPermanent())
		{
			return false;
		}

		if (GC.getImprovementInfo(pTargetPlot->getImprovementType()).getAirBombDefense() == -1)
		{
			return false;
		} */
		// K-Mod. Don't allow the player to bomb improvements that they don't know exist.
		ImprovementTypes eActualImprovement = pTargetPlot->getImprovementType();
		ImprovementTypes eRevealedImprovement = pTargetPlot->getRevealedImprovementType(getTeam(), false);

		if (eActualImprovement == NO_IMPROVEMENT || eRevealedImprovement == NO_IMPROVEMENT)
			return false;

		if (GC.getImprovementInfo(eActualImprovement).isPermanent() || GC.getImprovementInfo(eRevealedImprovement).isPermanent())
			return false;

		if (GC.getImprovementInfo(eActualImprovement).getAirBombDefense() == -1 || GC.getImprovementInfo(eRevealedImprovement).getAirBombDefense() == -1)
			return false;
		// K-Mod end
	}

	return true;
}


bool CvUnit::airBomb(int iX, int iY)
{
	CvCity* pCity;
	CvPlot* pPlot;
	CvWString szBuffer;

	if (!canAirBombAt(plot(), iX, iY))
	{
		return false;
	}

	pPlot = GC.getMapINLINE().plotINLINE(iX, iY);

	/* if (!isEnemy(pPlot->getTeam()))
	{
		getGroup()->groupDeclareWar(pPlot, true);
	} */ // Disabled by K-Mod

	if (!isEnemy(pPlot->getTeam()))
	{
		return false;
	}

	if (interceptTest(pPlot))
	{
		return true;
	}

	pCity = pPlot->getPlotCity();

	if (pCity != NULL)
	{
		/*  <advc.004c> Same as in CvUnit::bombard except that IgnoreBuildingDefense
			doesn't have to be checked here b/c all air units have that */
		int modWithBuildings = pCity->getDefenseModifier(false);
		int modSansBuildings = pCity->getDefenseModifier(true);
		FAssertMsg(modSansBuildings > 0 || isHuman(),
				"The AI shoudn't bombard cities whose def is already 0");
		double chg = -airBombCurrRate() * (modWithBuildings / (double)modSansBuildings);
		pCity->changeDefenseModifier(std::min(0, (int)::floor(chg)));
		// Replacing this line: // </advc.004c>
		//pCity->changeDefenseModifier(-airBombCurrRate());

		szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_DEFENSES_REDUCED_TO", pCity->getNameKey(), pCity->getDefenseModifier(false), getNameKey());
		gDLL->getInterfaceIFace()->addHumanMessage(pCity->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_BOMBARDED", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pCity->getX_INLINE(), pCity->getY_INLINE(), true, true);

		szBuffer = gDLL->getText("TXT_KEY_MISC_ENEMY_DEFENSES_REDUCED_TO", getNameKey(), pCity->getNameKey(), pCity->getDefenseModifier(false));
		gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_BOMBARD", MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pCity->getX_INLINE(), pCity->getY_INLINE());
	}
	else
	{
		if (pPlot->getImprovementType() != NO_IMPROVEMENT)
		{
			if (GC.getGameINLINE().getSorenRandNum(airBombCurrRate(), "Air Bomb - Offense") >=
					GC.getGameINLINE().getSorenRandNum(GC.getImprovementInfo(pPlot->getImprovementType()).getAirBombDefense(), "Air Bomb - Defense"))
			{
				szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_UNIT_DESTROYED_IMP", getNameKey(), GC.getImprovementInfo(pPlot->getImprovementType()).getTextKeyWide());
				gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_PILLAGE", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pPlot->getX_INLINE(), pPlot->getY_INLINE());

				if (pPlot->isOwned())
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_IMP_WAS_DESTROYED", GC.getImprovementInfo(pPlot->getImprovementType()).getTextKeyWide(), getNameKey(), getVisualCivAdjective(pPlot->getTeam()));
					gDLL->getInterfaceIFace()->addHumanMessage(pPlot->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_PILLAGED", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true, true);
				}

				pPlot->setImprovementType((ImprovementTypes)(GC.getImprovementInfo(pPlot->getImprovementType()).getImprovementPillage()));
			}
			else
			{
				szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_UNIT_FAIL_DESTROY_IMP", getNameKey(), GC.getImprovementInfo(pPlot->getImprovementType()).getTextKeyWide());
				gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_BOMB_FAILS", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE());
			}
		}
	}

	setReconPlot(pPlot);

	setMadeAttack(true);
	changeMoves(GC.getMOVE_DENOMINATOR());

	if (pPlot->isActiveVisible(false))
	{
		CvAirMissionDefinition kAirMission;
		kAirMission.setMissionType(MISSION_AIRBOMB);
		kAirMission.setUnit(BATTLE_UNIT_ATTACKER, this);
		kAirMission.setUnit(BATTLE_UNIT_DEFENDER, NULL);
		kAirMission.setDamage(BATTLE_UNIT_DEFENDER, 0);
		kAirMission.setDamage(BATTLE_UNIT_ATTACKER, 0);
		kAirMission.setPlot(pPlot);
		kAirMission.setMissionTime(GC.getMissionInfo((MissionTypes)MISSION_AIRBOMB).getTime() * gDLL->getSecsPerTurn());

		gDLL->getEntityIFace()->AddMission(&kAirMission);
	}

	if (isSuicide())
	{
		kill(true);
	}

	return true;
}


CvCity* CvUnit::bombardTarget(const CvPlot* pPlot) const
{
	int iBestValue = MAX_INT;
	CvCity* pBestCity = NULL;

	for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		CvPlot* pLoopPlot = plotDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), ((DirectionTypes)iI));

		if (pLoopPlot != NULL)
		{
			CvCity* pLoopCity = pLoopPlot->getPlotCity();

			if (pLoopCity != NULL)
			{
				if (pLoopCity->isBombardable(this))
				{
					int iValue = pLoopCity->getDefenseDamage();

					// always prefer cities we are at war with
					if (isEnemy(pLoopCity->getTeam(), pPlot))
					{
						iValue *= 128;
					}

					if (iValue < iBestValue)
					{
						iBestValue = iValue;
						pBestCity = pLoopCity;
					}
				}
			}
		}
	}

	return pBestCity;
}


bool CvUnit::canBombard(const CvPlot* pPlot) const
{
	if (bombardRate() <= 0)
	{
		return false;
	}

	if (isMadeAttack())
	{
		return false;
	}

	if (isCargo())
	{
		return false;
	}

	if (bombardTarget(pPlot) == NULL)
	{
		return false;
	}

	return true;
}


bool CvUnit::bombard()
{
	CvPlot* pPlot = plot();
	if (!canBombard(pPlot))
	{
		return false;
	}

	CvCity* pBombardCity = bombardTarget(pPlot);
	FAssertMsg(pBombardCity != NULL, "BombardCity is not assigned a valid value");

	CvPlot* pTargetPlot = pBombardCity->plot();
	/* if (!isEnemy(pTargetPlot->getTeam()))
	{
		getGroup()->groupDeclareWar(pTargetPlot, true);
	} */ // Disabled by K-Mod

	if (!isEnemy(pTargetPlot->getTeam()))
	{
		return false;
	}
	// <advc.004c>
	bool bIgnore = ignoreBuildingDefense();
	int modWithBuildings = pBombardCity->getDefenseModifier(false);
	FAssertMsg(modWithBuildings > 0 || isHuman(),
			"The AI shoudn't bombard cities whose def is already 0");
	int modSansBuildings = pBombardCity->getDefenseModifier(true);
	// </advc.004c>
	int iBombardModifier = 0;
	
	if (!bIgnore) // advc.004c
	{
		iBombardModifier -= pBombardCity->getBuildingBombardDefense();
	}
	// <advc.004c> Same formula as in BtS (except for rounding)
	double chg = -(bombardRate() * std::max(0, 100 + iBombardModifier)) / 100.0;
	if(bIgnore && modSansBuildings > 0) /*  bIgnore doesn't just ignore
			BombardDefense, also need to decrease DefenseModifier proportional
			to the effect of buildings in order to properly ignore BuildingDefense. */
		chg *= modWithBuildings / (double)modSansBuildings;
	pBombardCity->changeDefenseModifier(std::min(0, (int)::floor(chg)));
	// </advc.004c>
	setMadeAttack(true);
	changeMoves(GC.getMOVE_DENOMINATOR());

	CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_DEFENSES_IN_CITY_REDUCED_TO", pBombardCity->getNameKey(), pBombardCity->getDefenseModifier(false), GET_PLAYER(getOwnerINLINE()).getNameKey());
	gDLL->getInterfaceIFace()->addHumanMessage(pBombardCity->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_BOMBARDED", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pBombardCity->getX_INLINE(), pBombardCity->getY_INLINE(), true, true);

	szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_REDUCE_CITY_DEFENSES", getNameKey(), pBombardCity->getNameKey(), pBombardCity->getDefenseModifier(false));
	gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_BOMBARD", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pBombardCity->getX_INLINE(), pBombardCity->getY_INLINE());

	if (pPlot->isActiveVisible(false))
	{
		CvUnit *pDefender = pBombardCity->plot()->getBestDefender(NO_PLAYER, getOwnerINLINE(), this, true);

		// Bombard entity mission
		CvMissionDefinition kDefiniton;
		kDefiniton.setMissionTime(GC.getMissionInfo(MISSION_BOMBARD).getTime() * gDLL->getSecsPerTurn());
		kDefiniton.setMissionType(MISSION_BOMBARD);
		kDefiniton.setPlot(pBombardCity->plot());
		kDefiniton.setUnit(BATTLE_UNIT_ATTACKER, this);
		kDefiniton.setUnit(BATTLE_UNIT_DEFENDER, pDefender);
		gDLL->getEntityIFace()->AddMission(&kDefiniton);
	}

	return true;
}


bool CvUnit::canPillage(const CvPlot* pPlot) const
{
	if (!(m_pUnitInfo->isPillage()))
	{
		return false;
	}

	if (pPlot->isCity())
	{
		return false;
	}

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       06/23/10                     Mongoose & jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
	// From Mongoose SDK
	if (isCargo())
	{
		return false;
	}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
	// <advc.111>
	if(pPlot->getOwnerINLINE() == NO_PLAYER &&
				pPlot->isVisibleOtherUnit(getOwnerINLINE()))
		return false; // </advc.111>

	if (pPlot->getImprovementType() == NO_IMPROVEMENT)
	{
		if (!(pPlot->isRoute()))
		{
			return false;
		}
	}
	else
	{
		if (GC.getImprovementInfo(pPlot->getImprovementType()).isPermanent())
		{
			return false;
		}
		/*  <advc.005c> Don't want to use GC.getDefineINT("RUINS_IMPROVEMENT") here
			for performance reasons. Can't set Ruins as bPermanent in XML b/c
			that would also prevent building over them. */
		CvImprovementInfo& ii = GC.getImprovementInfo(pPlot->getImprovementType());
		if(!ii.isGraphicalOnly() && ii.getPillageGold() <= 0 &&
				// i.e. not Forest Preserve and not Fort
				ii.getHappiness() <= 0 && !ii.isActsAsCity())
			return false; // </advc.005c>
	}

	if (pPlot->isOwned())
	{
		if (!potentialWarAction(pPlot))
		{
/*
** K-Mod, 16/dec/10, karadoc
** enabled the pillaging of own roads
*/
			/* original bts code
			if ((pPlot->getImprovementType() == NO_IMPROVEMENT) || (pPlot->getOwnerINLINE() != getOwnerINLINE()))
			*/
			if (pPlot->getOwnerINLINE() != getOwnerINLINE() || (pPlot->getImprovementType() == NO_IMPROVEMENT && !pPlot->isRoute()))
// K-Mod end
			{
				return false;
			}
		}
	}

	if (!(pPlot->isValidDomainForAction(*this)))
	{
		return false;
	}

	return true;
}


bool CvUnit::pillage()
{
	CvWString szBuffer;
	int iPillageGold;
	long lPillageGold;
	ImprovementTypes eTempImprovement = NO_IMPROVEMENT;
	RouteTypes eTempRoute = NO_ROUTE;

	CvPlot* pPlot = plot();

	if (!canPillage(pPlot))
	{
		return false;
	}

	if (pPlot->isOwned())
	{
		// we should not be calling this without declaring war first, so do not declare war here
		if (!isEnemy(pPlot->getTeam(), pPlot))
		{
/*
** K-Mod, 16/dec/10, karadoc
** enabled the pillaging of own roads
*/
			/* original bts code
			if ((pPlot->getImprovementType() == NO_IMPROVEMENT) || (pPlot->getOwnerINLINE() != getOwnerINLINE()))
			*/
			if (pPlot->getOwnerINLINE() != getOwnerINLINE() || (pPlot->getImprovementType() == NO_IMPROVEMENT && !pPlot->isRoute()))
// K-Mod end
			{
				return false;
			}
		}
	}

	if (pPlot->isWater())
	{
		// UncutDragon
		// original
		//CvUnit* pInterceptor = bestSeaPillageInterceptor(this, GC.getDefineINT("COMBAT_DIE_SIDES") / 2);
		// modified
		CvUnit* pInterceptor = bestSeaPillageInterceptor(this, GC.getCOMBAT_DIE_SIDES() / 2);
		// /UncutDragon
		if (NULL != pInterceptor)
		{
			setMadeAttack(false);

			int iWithdrawal = withdrawalProbability();
			changeExtraWithdrawal(-iWithdrawal); // no withdrawal since we are really the defender
			attack(pInterceptor->plot(), false);
			changeExtraWithdrawal(iWithdrawal);

			return false;
		}
	}

	if (pPlot->getImprovementType() != NO_IMPROVEMENT)
	{
		eTempImprovement = pPlot->getImprovementType();

		if (pPlot->getTeam() != getTeam())
		{
			// Use python to determine pillage amounts...
			//lPillageGold = 0;
			lPillageGold = -1; // K-Mod

			if (GC.getUSE_DO_PILLAGE_GOLD_CALLBACK()) // K-Mod. I've writen C to replace the python callback.
			{
				CyPlot* pyPlot = new CyPlot(pPlot);
				CyUnit* pyUnit = new CyUnit(this);

				CyArgsList argsList;
				argsList.add(gDLL->getPythonIFace()->makePythonObject(pyPlot));	// pass in plot class
				argsList.add(gDLL->getPythonIFace()->makePythonObject(pyUnit));	// pass in unit class

				gDLL->getPythonIFace()->callFunction(PYGameModule, "doPillageGold", argsList.makeFunctionArgs(),&lPillageGold);

				delete pyPlot;	// python fxn must not hold on to this pointer 
				delete pyUnit;	// python fxn must not hold on to this pointer 

				iPillageGold = (int)lPillageGold;
			}
			// K-Mod. C version of the original python code
			if (lPillageGold < 0)
			{
				int iPillageBase = GC.getImprovementInfo((ImprovementTypes)pPlot->getImprovementType()).getPillageGold();
				iPillageGold = 0;
				iPillageGold += GC.getGameINLINE().getSorenRandNum(iPillageBase, "Pillage Gold 1");
				iPillageGold += GC.getGameINLINE().getSorenRandNum(iPillageBase, "Pillage Gold 2");
				iPillageGold += getPillageChange() * iPillageGold / 100;
			}
			// K-Mod end

			if (iPillageGold > 0)
			{
				GET_PLAYER(getOwnerINLINE()).changeGold(iPillageGold);

				szBuffer = gDLL->getText("TXT_KEY_MISC_PLUNDERED_GOLD_FROM_IMP", iPillageGold, GC.getImprovementInfo(pPlot->getImprovementType()).getTextKeyWide());
				gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_PILLAGE", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pPlot->getX_INLINE(), pPlot->getY_INLINE());

				if (pPlot->isOwned())
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_IMP_DESTROYED", GC.getImprovementInfo(pPlot->getImprovementType()).getTextKeyWide(), getNameKey(), getVisualCivAdjective(pPlot->getTeam()));
					gDLL->getInterfaceIFace()->addHumanMessage(pPlot->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_PILLAGED", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true, true);
				}
			}
		}

		pPlot->setImprovementType((ImprovementTypes)(GC.getImprovementInfo(pPlot->getImprovementType()).getImprovementPillage()));
	}
	else if (pPlot->isRoute())
	{
		eTempRoute = pPlot->getRouteType();
		pPlot->setRouteType(NO_ROUTE, true); // XXX downgrade rail???
	}

	changeMoves(GC.getMOVE_DENOMINATOR());

	if (pPlot->isActiveVisible(false))
	{
		// Pillage entity mission
		CvMissionDefinition kDefiniton;
		kDefiniton.setMissionTime(GC.getMissionInfo(MISSION_PILLAGE).getTime() * gDLL->getSecsPerTurn());
		kDefiniton.setMissionType(MISSION_PILLAGE);
		kDefiniton.setPlot(pPlot);
		kDefiniton.setUnit(BATTLE_UNIT_ATTACKER, this);
		kDefiniton.setUnit(BATTLE_UNIT_DEFENDER, NULL);
		gDLL->getEntityIFace()->AddMission(&kDefiniton);
	}

	if (eTempImprovement != NO_IMPROVEMENT || eTempRoute != NO_ROUTE)
	{
		CvEventReporter::getInstance().unitPillage(this, eTempImprovement, eTempRoute, getOwnerINLINE());
	}

	return true;
}


bool CvUnit::canPlunder(const CvPlot* pPlot, bool bTestVisible) const
{
	if (getDomainType() != DOMAIN_SEA)
	{
		return false;
	}

	if (!(m_pUnitInfo->isPillage()))
	{
		return false;
	}

	if (!pPlot->isWater())
	{
		return false;
	}

	if (pPlot->isFreshWater())
	{
		return false;
	}

	if (!pPlot->isValidDomainForAction(*this))
	{
		return false;
	}

	if (!bTestVisible)
	{
		if (pPlot->getTeam() == getTeam())
		{
			return false;
		}
	}

	return true;
}


bool CvUnit::plunder()
{
	CvPlot* pPlot = plot();

	if (!canPlunder(pPlot))
	{
		return false;
	}

	setBlockading(true);

	finishMoves();

	return true;
}


void CvUnit::updatePlunder(int iChange, bool bUpdatePlotGroups)
{
	int iBlockadeRange = GC.getDefineINT("SHIP_BLOCKADE_RANGE");

	bool bOldTradeNet;
	bool bChanged = false;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/01/09                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
	//gDLL->getFAStarIFace()->ForceReset(&GC.getStepFinder());
	bool bValid = false;
	for (int i = -iBlockadeRange; i <= iBlockadeRange; ++i)
	{
		for (int j = -iBlockadeRange; j <= iBlockadeRange; ++j)
		{
			CvPlot* pLoopPlot = ::plotXY(getX_INLINE(), getY_INLINE(), i, j);

			if (NULL != pLoopPlot && pLoopPlot->isWater() && pLoopPlot->area() == area())
			{

				int iPathDist = GC.getMapINLINE().calculatePathDistance(plot(),pLoopPlot);
				
				// BBAI NOTES:  There are rare issues where the path finder will return incorrect results
				// for unknown reasons.  Seems to find a suboptimal path sometimes in partially repeatable 
				// circumstances.  The fix below is a hack to address the permanent one or two tile blockades which 
				// would appear randomly, it should cause extra blockade clearing only very rarely.
				/*
				if( iPathDist > iBlockadeRange )
				{
					// No blockading on other side of an isthmus
					continue;
				}
				*/
				
				if( (iPathDist >= 0) && (iPathDist <= iBlockadeRange + 2) )
				{
					for (int iTeam = 0; iTeam < MAX_TEAMS; ++iTeam)
					{
						if (isEnemy((TeamTypes)iTeam))
						{
							bValid = (iPathDist <= iBlockadeRange);
							if( !bValid && (iChange == -1 && pLoopPlot->getBlockadedCount((TeamTypes)iTeam) > 0) )
							{
								bValid = true;
							}

							if( bValid )
							{
								if (!bChanged)
								{
									bOldTradeNet = pLoopPlot->isTradeNetwork((TeamTypes)iTeam);
								}

								pLoopPlot->changeBlockadedCount((TeamTypes)iTeam, iChange);

								if (!bChanged)
								{
									bChanged = (bOldTradeNet != pLoopPlot->isTradeNetwork((TeamTypes)iTeam));
								}
							}
						}
					}
				}
			}
		}
	}

	if (bChanged)
	{
		gDLL->getInterfaceIFace()->setDirty(BlockadedPlots_DIRTY_BIT, true);

		if (bUpdatePlotGroups)
		{
			GC.getGameINLINE().updatePlotGroups();
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
}


int CvUnit::sabotageCost(const CvPlot* pPlot) const
{
	return GC.getDefineINT("BASE_SPY_SABOTAGE_COST");
}


// XXX compare with destroy prob...
int CvUnit::sabotageProb(const CvPlot* pPlot, ProbabilityTypes eProbStyle) const
{
	CvPlot* pLoopPlot;
	int iDefenseCount;
	int iCounterSpyCount;
	int iProb;
	int iI;

	iProb = 0; // XXX

	if (pPlot->isOwned())
	{
		iDefenseCount = pPlot->plotCount(PUF_canDefend, -1, -1, NO_PLAYER, pPlot->getTeam());
		iCounterSpyCount = pPlot->plotCount(PUF_isCounterSpy, -1, -1, NO_PLAYER, pPlot->getTeam());

		for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
		{
			pLoopPlot = plotDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), ((DirectionTypes)iI));

			if (pLoopPlot != NULL)
			{
				iCounterSpyCount += pLoopPlot->plotCount(PUF_isCounterSpy, -1, -1, NO_PLAYER, pPlot->getTeam());
			}
		}
	}
	else
	{
		iDefenseCount = 0;
		iCounterSpyCount = 0;
	}

	if (eProbStyle == PROBABILITY_HIGH)
	{
		iCounterSpyCount = 0;
	}

	iProb += (40 / (iDefenseCount + 1)); // XXX

	if (eProbStyle != PROBABILITY_LOW)
	{
		iProb += (50 / (iCounterSpyCount + 1)); // XXX
	}

	return iProb;
}


bool CvUnit::canSabotage(const CvPlot* pPlot, bool bTestVisible) const
{
	if (!(m_pUnitInfo->isSabotage()))
	{
		return false;
	}

	if (pPlot->getTeam() == getTeam())
	{
		return false;
	}

	if (pPlot->isCity())
	{
		return false;
	}

	if (pPlot->getImprovementType() == NO_IMPROVEMENT)
	{
		return false;
	}

	if (!bTestVisible)
	{
		if (GET_PLAYER(getOwnerINLINE()).getGold() < sabotageCost(pPlot))
		{
			return false;
		}
	}

	return true;
}


bool CvUnit::sabotage()
{
	CvCity* pNearestCity;
	CvPlot* pPlot;
	CvWString szBuffer;
	bool bCaught;

	if (!canSabotage(plot()))
	{
		return false;
	}

	pPlot = plot();

	bCaught = (GC.getGameINLINE().getSorenRandNum(100, "Spy: Sabotage") > sabotageProb(pPlot));

	GET_PLAYER(getOwnerINLINE()).changeGold(-(sabotageCost(pPlot)));

	if (!bCaught)
	{
		pPlot->setImprovementType((ImprovementTypes)(GC.getImprovementInfo(pPlot->getImprovementType()).getImprovementPillage()));

		finishMoves();

		pNearestCity = GC.getMapINLINE().findCity(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pPlot->getOwnerINLINE(), NO_TEAM, false);

		if (pNearestCity != NULL)
		{
			szBuffer = gDLL->getText("TXT_KEY_MISC_SPY_SABOTAGED", getNameKey(), pNearestCity->getNameKey());
			gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_SABOTAGE", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pPlot->getX_INLINE(), pPlot->getY_INLINE());

			if (pPlot->isOwned())
			{
				szBuffer = gDLL->getText("TXT_KEY_MISC_SABOTAGE_NEAR", pNearestCity->getNameKey());
				gDLL->getInterfaceIFace()->addHumanMessage(pPlot->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_SABOTAGE", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true, true);
			}
		}

		if (pPlot->isActiveVisible(false))
		{
			NotifyEntity(MISSION_SABOTAGE);
		}
	}
	else
	{
		if (pPlot->isOwned())
		{
			szBuffer = gDLL->getText("TXT_KEY_MISC_SPY_CAUGHT_AND_KILLED", GET_PLAYER(getOwnerINLINE()).getCivilizationAdjective(), getNameKey());
			gDLL->getInterfaceIFace()->addHumanMessage(pPlot->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_EXPOSE", MESSAGE_TYPE_INFO);
		}

		szBuffer = gDLL->getText("TXT_KEY_MISC_YOUR_SPY_CAUGHT", getNameKey());
		gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_EXPOSED", MESSAGE_TYPE_INFO);

		if (plot()->isActiveVisible(false))
		{
			NotifyEntity(MISSION_SURRENDER);
		}

		if (pPlot->isOwned())
		{
			if (!isEnemy(pPlot->getTeam(), pPlot))
			{   // advc.130j:
				GET_PLAYER(pPlot->getOwnerINLINE()).AI_rememberEvent(getOwnerINLINE(), MEMORY_SPY_CAUGHT);
			}
		}

		kill(true, pPlot->getOwnerINLINE());
	}

	return true;
}


int CvUnit::destroyCost(const CvPlot* pPlot) const
{
	CvCity* pCity;
	bool bLimited;

	pCity = pPlot->getPlotCity();

	if (pCity == NULL)
	{
		return 0;
	}

	bLimited = false;

	if (pCity->isProductionUnit())
	{
		bLimited = isLimitedUnitClass((UnitClassTypes)(GC.getUnitInfo(pCity->getProductionUnit()).getUnitClassType()));
	}
	else if (pCity->isProductionBuilding())
	{
		bLimited = isLimitedWonderClass((BuildingClassTypes)(GC.getBuildingInfo(pCity->getProductionBuilding()).getBuildingClassType()));
	}
	else if (pCity->isProductionProject())
	{
		bLimited = isLimitedProject(pCity->getProductionProject());
	}

	return (GC.getDefineINT("BASE_SPY_DESTROY_COST") + (pCity->getProduction() * ((bLimited) ? GC.getDefineINT("SPY_DESTROY_COST_MULTIPLIER_LIMITED") : GC.getDefineINT("SPY_DESTROY_COST_MULTIPLIER"))));
}


int CvUnit::destroyProb(const CvPlot* pPlot, ProbabilityTypes eProbStyle) const
{
	CvCity* pCity;
	CvPlot* pLoopPlot;
	int iDefenseCount;
	int iCounterSpyCount;
	int iProb;
	int iI;

	pCity = pPlot->getPlotCity();

	if (pCity == NULL)
	{
		return 0;
	}

	iProb = 0; // XXX

	iDefenseCount = pPlot->plotCount(PUF_canDefend, -1, -1, NO_PLAYER, pPlot->getTeam());

	iCounterSpyCount = pPlot->plotCount(PUF_isCounterSpy, -1, -1, NO_PLAYER, pPlot->getTeam());

	for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		pLoopPlot = plotDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), ((DirectionTypes)iI));

		if (pLoopPlot != NULL)
		{
			iCounterSpyCount += pLoopPlot->plotCount(PUF_isCounterSpy, -1, -1, NO_PLAYER, pPlot->getTeam());
		}
	}

	if (eProbStyle == PROBABILITY_HIGH)
	{
		iCounterSpyCount = 0;
	}

	iProb += (25 / (iDefenseCount + 1)); // XXX

	if (eProbStyle != PROBABILITY_LOW)
	{
		iProb += (50 / (iCounterSpyCount + 1)); // XXX
	}

	iProb += std::min(25, pCity->getProductionTurnsLeft()); // XXX

	return iProb;
}


bool CvUnit::canDestroy(const CvPlot* pPlot, bool bTestVisible) const
{
	CvCity* pCity;

	if (!(m_pUnitInfo->isDestroy()))
	{
		return false;
	}

	if (pPlot->getTeam() == getTeam())
	{
		return false;
	}

	pCity = pPlot->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (pCity->getProduction() == 0)
	{
		return false;
	}

	if (!bTestVisible)
	{
		if (GET_PLAYER(getOwnerINLINE()).getGold() < destroyCost(pPlot))
		{
			return false;
		}
	}

	return true;
}


bool CvUnit::destroy()
{
	CvCity* pCity;
	CvWString szBuffer;
	bool bCaught;

	if (!canDestroy(plot()))
	{
		return false;
	}

	bCaught = (GC.getGameINLINE().getSorenRandNum(100, "Spy: Destroy") > destroyProb(plot()));

	pCity = plot()->getPlotCity();
	FAssertMsg(pCity != NULL, "City is not assigned a valid value");

	GET_PLAYER(getOwnerINLINE()).changeGold(-(destroyCost(plot())));

	if (!bCaught)
	{
		pCity->setProduction(pCity->getProduction() / 2);

		finishMoves();

		szBuffer = gDLL->getText("TXT_KEY_MISC_SPY_DESTROYED_PRODUCTION", getNameKey(), pCity->getProductionNameKey(), pCity->getNameKey());
		gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_DESTROY", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pCity->getX_INLINE(), pCity->getY_INLINE());

		szBuffer = gDLL->getText("TXT_KEY_MISC_CITY_PRODUCTION_DESTROYED", pCity->getProductionNameKey(), pCity->getNameKey());
		gDLL->getInterfaceIFace()->addHumanMessage(pCity->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_DESTROY", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pCity->getX_INLINE(), pCity->getY_INLINE(), true, true);

		if (plot()->isActiveVisible(false))
		{
			NotifyEntity(MISSION_DESTROY);
		}
	}
	else
	{
		szBuffer = gDLL->getText("TXT_KEY_MISC_SPY_CAUGHT_AND_KILLED", GET_PLAYER(getOwnerINLINE()).getCivilizationAdjective(), getNameKey());
		gDLL->getInterfaceIFace()->addHumanMessage(pCity->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_EXPOSE", MESSAGE_TYPE_INFO);

		szBuffer = gDLL->getText("TXT_KEY_MISC_YOUR_SPY_CAUGHT", getNameKey());
		gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_EXPOSED", MESSAGE_TYPE_INFO);

		if (plot()->isActiveVisible(false))
		{
			NotifyEntity(MISSION_SURRENDER);
		}

		if (!isEnemy(pCity->getTeam()))
		{   // advc.130j:
			GET_PLAYER(pCity->getOwnerINLINE()).AI_rememberEvent(getOwnerINLINE(), MEMORY_SPY_CAUGHT);
		}

		kill(true, pCity->getOwnerINLINE());
	}

	return true;
}


int CvUnit::stealPlansCost(const CvPlot* pPlot) const
{
	CvCity* pCity;

	pCity = pPlot->getPlotCity();

	if (pCity == NULL)
	{
		return 0;
	}

	return (GC.getDefineINT("BASE_SPY_STEAL_PLANS_COST") + ((GET_TEAM(pCity->getTeam()).getTotalLand() + GET_TEAM(pCity->getTeam()).getTotalPopulation()) * GC.getDefineINT("SPY_STEAL_PLANS_COST_MULTIPLIER")));
}


// XXX compare with destroy prob...
int CvUnit::stealPlansProb(const CvPlot* pPlot, ProbabilityTypes eProbStyle) const
{
	CvCity* pCity;
	CvPlot* pLoopPlot;
	int iDefenseCount;
	int iCounterSpyCount;
	int iProb;
	int iI;

	pCity = pPlot->getPlotCity();

	if (pCity == NULL)
	{
		return 0;
	}

	iProb = ((pCity->isGovernmentCenter()) ? 20 : 0); // XXX

	iDefenseCount = pPlot->plotCount(PUF_canDefend, -1, -1, NO_PLAYER, pPlot->getTeam());

	iCounterSpyCount = pPlot->plotCount(PUF_isCounterSpy, -1, -1, NO_PLAYER, pPlot->getTeam());

	for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		pLoopPlot = plotDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), ((DirectionTypes)iI));

		if (pLoopPlot != NULL)
		{
			iCounterSpyCount += pLoopPlot->plotCount(PUF_isCounterSpy, -1, -1, NO_PLAYER, pPlot->getTeam());
		}
	}

	if (eProbStyle == PROBABILITY_HIGH)
	{
		iCounterSpyCount = 0;
	}

	iProb += (20 / (iDefenseCount + 1)); // XXX

	if (eProbStyle != PROBABILITY_LOW)
	{
		iProb += (50 / (iCounterSpyCount + 1)); // XXX
	}

	return iProb;
}


bool CvUnit::canStealPlans(const CvPlot* pPlot, bool bTestVisible) const
{
	CvCity* pCity;

	if (!(m_pUnitInfo->isStealPlans()))
	{
		return false;
	}

	if (pPlot->getTeam() == getTeam())
	{
		return false;
	}

	pCity = pPlot->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (!bTestVisible)
	{
		if (GET_PLAYER(getOwnerINLINE()).getGold() < stealPlansCost(pPlot))
		{
			return false;
		}
	}

	return true;
}


bool CvUnit::stealPlans()
{
	CvCity* pCity;
	CvWString szBuffer;
	bool bCaught;

	if (!canStealPlans(plot()))
	{
		return false;
	}

	bCaught = (GC.getGameINLINE().getSorenRandNum(100, "Spy: Steal Plans") > stealPlansProb(plot()));

	pCity = plot()->getPlotCity();
	FAssertMsg(pCity != NULL, "City is not assigned a valid value");

	GET_PLAYER(getOwnerINLINE()).changeGold(-(stealPlansCost(plot())));

	if (!bCaught)
	{
		GET_TEAM(getTeam()).changeStolenVisibilityTimer(pCity->getTeam(), 2);

		finishMoves();

		szBuffer = gDLL->getText("TXT_KEY_MISC_SPY_STOLE_PLANS", getNameKey(), pCity->getNameKey());
		gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_STEALPLANS", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pCity->getX_INLINE(), pCity->getY_INLINE());

		szBuffer = gDLL->getText("TXT_KEY_MISC_PLANS_STOLEN", pCity->getNameKey());
		gDLL->getInterfaceIFace()->addHumanMessage(pCity->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_STEALPLANS", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pCity->getX_INLINE(), pCity->getY_INLINE(), true, true);

		if (plot()->isActiveVisible(false))
		{
			NotifyEntity(MISSION_STEAL_PLANS);
		}
	}
	else
	{
		szBuffer = gDLL->getText("TXT_KEY_MISC_SPY_CAUGHT_AND_KILLED", GET_PLAYER(getOwnerINLINE()).getCivilizationAdjective(), getNameKey());
		gDLL->getInterfaceIFace()->addHumanMessage(pCity->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_EXPOSE", MESSAGE_TYPE_INFO);

		szBuffer = gDLL->getText("TXT_KEY_MISC_YOUR_SPY_CAUGHT", getNameKey());
		gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_EXPOSED", MESSAGE_TYPE_INFO);

		if (plot()->isActiveVisible(false))
		{
			NotifyEntity(MISSION_SURRENDER);
		}

		if (!isEnemy(pCity->getTeam()))
		{   // advc.130j:
			GET_PLAYER(pCity->getOwnerINLINE()).AI_rememberEvent(getOwnerINLINE(), MEMORY_SPY_CAUGHT);
		}

		kill(true, pCity->getOwnerINLINE());
	}

	return true;
}


bool CvUnit::canFound(const CvPlot* pPlot, bool bTestVisible) const
{
	if (!isFound())
	{
		return false;
	}

	if (!(GET_PLAYER(getOwnerINLINE()).canFound(pPlot->getX_INLINE(), pPlot->getY_INLINE(), bTestVisible)))
	{
		return false;
	}

	return true;
}


bool CvUnit::found()
{
	if (!canFound(plot()))
	{
		return false;
	}

	if (GC.getGameINLINE().getActivePlayer() == getOwnerINLINE()
			&& CvPlot::activeVisibility) // advc.706
	{
		gDLL->getInterfaceIFace()->lookAt(plot()->getPoint(), CAMERALOOKAT_NORMAL);
	}

	GET_PLAYER(getOwnerINLINE()).found(getX_INLINE(), getY_INLINE());

	if (plot()->isActiveVisible(false))
	{
		NotifyEntity(MISSION_FOUND);
	}

	kill(true);

	return true;
}


bool CvUnit::canSpread(const CvPlot* pPlot, ReligionTypes eReligion, bool bTestVisible) const
{
	CvCity* pCity;

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       08/19/09                                jdog5000      */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
/* orginal bts code
	if (GC.getUSE_USE_CANNOT_SPREAD_RELIGION_CALLBACK())
	{
		CyArgsList argsList;
		argsList.add(getOwnerINLINE());
		argsList.add(getID());
		argsList.add((int) eReligion);
		argsList.add(pPlot->getX());
		argsList.add(pPlot->getY());
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "cannotSpreadReligion", argsList.makeFunctionArgs(), &lResult);
		if (lResult > 0)
		{
			return false;
		}
	}
*/
				// UP efficiency: Moved below faster calls
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/

	if (eReligion == NO_RELIGION)
	{
		return false;
	}

	if (m_pUnitInfo->getReligionSpreads(eReligion) <= 0)
	{
		return false;
	}

	pCity = pPlot->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (pCity->isHasReligion(eReligion))
	{
		return false;
	} // <advc.099d>
	if(pCity->isDisorder())
		return false; // </advc.099d>
	if (!canEnterArea(pPlot->getTeam(), pPlot->area()))
	{
		return false;
	}

	if (!bTestVisible)
	{
		/* advc.123a (no change made): Could remove this line to disallow
		   spreading non-state religion while in Theocracy. I've disallowed
		   gifting of Missionaries (of a non-state religion) instead.
		   Both are ways to stop players from bypassing the Theocracy restriction
		   through gifting. */
		if (pCity->getTeam() != getTeam())
		{
			if (GET_PLAYER(pCity->getOwnerINLINE()).isNoNonStateReligionSpread())
			{
				if (eReligion != GET_PLAYER(pCity->getOwnerINLINE()).getStateReligion())
				{
					return false;
				}
			}
		}
	}

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       08/19/09                                jdog5000      */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
	if (GC.getUSE_USE_CANNOT_SPREAD_RELIGION_CALLBACK())
	{
		CyArgsList argsList;
		argsList.add(getOwnerINLINE());
		argsList.add(getID());
		argsList.add((int) eReligion);
		argsList.add(pPlot->getX());
		argsList.add(pPlot->getY());
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "cannotSpreadReligion", argsList.makeFunctionArgs(), &lResult);
		if (lResult > 0)
		{
			return false;
		}
	}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/

	return true;
}


bool CvUnit::spread(ReligionTypes eReligion)
{
	CvCity* pCity;
	CvWString szBuffer;
	//int iSpreadProb;

	if (!canSpread(plot(), eReligion))
	{
		return false;
	}

	pCity = plot()->getPlotCity();

	if (pCity != NULL)
	{
		/* original bts code
		iSpreadProb = m_pUnitInfo->getReligionSpreads(eReligion);

		if (pCity->getTeam() != getTeam())
		{
			iSpreadProb /= 2;
		}

		bool bSuccess;

		iSpreadProb += (((GC.getNumReligionInfos() - pCity->getReligionCount()) * (100 - iSpreadProb)) / GC.getNumReligionInfos()); */
		// K-Mod. A more dynamic formula
		int iPresentReligions = pCity->getReligionCount();
		int iMissingReligions = GC.getNumReligionInfos() - iPresentReligions;
		int iSpreadProb = iPresentReligions * (m_pUnitInfo->getReligionSpreads(eReligion) + pCity->getPopulation())
			+ iMissingReligions * std::max(100, 100 - 10 * iPresentReligions);
		iSpreadProb /= GC.getNumReligionInfos();

		bool bSuccess;
		// K-Mod end

		if (GC.getGameINLINE().getSorenRandNum(100, "Unit Spread Religion") < iSpreadProb)
		{
			pCity->setHasReligion(eReligion, true, true, false,
					getOwnerINLINE()); // advc.106e
			bSuccess = true;
		}
		else
		{
			/* original bts code
			szBuffer = gDLL->getText("TXT_KEY_MISC_RELIGION_FAILED_TO_SPREAD", getNameKey(), GC.getReligionInfo(eReligion).getChar(), pCity->getNameKey());
			gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_NOSPREAD", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pCity->getX_INLINE(), pCity->getY_INLINE());
			bSuccess = false;
			*/
			// K-Mod. Instead of simply failing, give some chance of removing one of the existing religions.
			std::vector<std::pair<int, ReligionTypes> > rankedReligions;
			int iRandomWeight = GC.getDefineINT("RELIGION_INFLUENCE_RANDOM_WEIGHT");
			for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
			{
				if (pCity->isHasReligion((ReligionTypes)iI) || iI == eReligion)
				{
					if (pCity != GC.getGame().getHolyCity((ReligionTypes)iI)) // holy city can't lose its religion!
					{
						int iInfluence = pCity->getReligionGrip((ReligionTypes)iI);
						iInfluence += GC.getGameINLINE().getSorenRandNum(iRandomWeight, "Religion influence");
						iInfluence += (iI == eReligion) ? m_pUnitInfo->getReligionSpreads(eReligion)/2 : 0;

						rankedReligions.push_back(std::make_pair(iInfluence, (ReligionTypes)iI));
					}
				}
			}
			std::partial_sort(rankedReligions.begin(), rankedReligions.begin()+1, rankedReligions.end());
			ReligionTypes eFailedReligion = rankedReligions[0].second;
			if (eFailedReligion == eReligion)
			{
				szBuffer = gDLL->getText("TXT_KEY_MISC_RELIGION_FAILED_TO_SPREAD", getNameKey(), GC.getReligionInfo(eReligion).getChar(), pCity->getNameKey());
				gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_NOSPREAD", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pCity->getX_INLINE(), pCity->getY_INLINE());
				bSuccess = false;
			}
			else
			{
				pCity->setHasReligion(eReligion, true, true, false,
						getOwnerINLINE()); // advc.106e
				pCity->setHasReligion(eFailedReligion, false, true, false,
						getOwnerINLINE()); // advc.106e
				bSuccess = true;
			}
			// K-Mod
		}

		// Python Event
		CvEventReporter::getInstance().unitSpreadReligionAttempt(this, eReligion, bSuccess);
	}

	if (plot()->isActiveVisible(false))
	{
		NotifyEntity(MISSION_SPREAD);
	}

	kill(true);

	return true;
}


bool CvUnit::canSpreadCorporation(const CvPlot* pPlot, CorporationTypes eCorporation, bool bTestVisible) const
{
	if (NO_CORPORATION == eCorporation)
	{
		return false;
	}

	if (!GET_PLAYER(getOwnerINLINE()).isActiveCorporation(eCorporation))
	{
		return false;
	}

	if (m_pUnitInfo->getCorporationSpreads(eCorporation) <= 0)
	{
		return false;
	}

	CvCity* pCity = pPlot->getPlotCity();

	if (NULL == pCity)
	{
		return false;
	}

	if (pCity->isHasCorporation(eCorporation))
	{
		return false;
	}
	 // <advc.099d>
	if(pCity->isDisorder())
		return false; // </advc.099d>
	if (!canEnterArea(pPlot->getTeam(), pPlot->area()))
	{
		return false;
	}

	if (!bTestVisible)
	{
		if (!GET_PLAYER(pCity->getOwnerINLINE()).isActiveCorporation(eCorporation))
		{
			return false;
		}

		for (int iCorporation = 0; iCorporation < GC.getNumCorporationInfos(); ++iCorporation)
		{
			if (pCity->isHeadquarters((CorporationTypes)iCorporation))
			{
				if (GC.getGameINLINE().isCompetingCorporation((CorporationTypes)iCorporation, eCorporation))
				{
					return false;
				}
			}
		}

		bool bValid = false;
		for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
		{
			BonusTypes eBonus = (BonusTypes)GC.getCorporationInfo(eCorporation).getPrereqBonus(i);
			if (NO_BONUS != eBonus)
			{
				if (pCity->hasBonus(eBonus))
				{
					bValid = true;
					break;
				}
			}
		}

		if (!bValid)
		{
			return false;
		}

		if (GET_PLAYER(getOwnerINLINE()).getGold() < spreadCorporationCost(eCorporation, pCity))
		{
			return false;
		}
	}

	return true;
}

int CvUnit::spreadCorporationCost(CorporationTypes eCorporation, CvCity* pCity) const
{
	int iCost = std::max(0, GC.getCorporationInfo(eCorporation).getSpreadCost() * (100 + GET_PLAYER(getOwnerINLINE()).calculateInflationRate()));
	iCost /= 100;

	if (NULL != pCity)
	{
		if (getTeam() != pCity->getTeam() && !GET_TEAM(pCity->getTeam()).isVassal(getTeam()))
		{
			iCost *= GC.getDefineINT("CORPORATION_FOREIGN_SPREAD_COST_PERCENT");
			iCost /= 100;
		}

		for (int iCorp = 0; iCorp < GC.getNumCorporationInfos(); ++iCorp)
		{
			if (iCorp != eCorporation)
			{
				if (pCity->isActiveCorporation((CorporationTypes)iCorp))
				{
					if (GC.getGameINLINE().isCompetingCorporation(eCorporation, (CorporationTypes)iCorp))
					{
						iCost *= 100 + GC.getCorporationInfo((CorporationTypes)iCorp).getSpreadFactor();
						iCost /= 100;
					}
				}
			}
		}
	}

	return iCost;
}

bool CvUnit::spreadCorporation(CorporationTypes eCorporation)
{
	int iSpreadProb;

	if (!canSpreadCorporation(plot(), eCorporation))
	{
		return false;
	}

	CvCity* pCity = plot()->getPlotCity();

	if (NULL != pCity)
	{
		GET_PLAYER(getOwnerINLINE()).changeGold(-spreadCorporationCost(eCorporation, pCity));

		iSpreadProb = m_pUnitInfo->getCorporationSpreads(eCorporation);

		if (pCity->getTeam() != getTeam())
		{
			iSpreadProb /= 2;
		}

		iSpreadProb += (((GC.getNumCorporationInfos() - pCity->getCorporationCount()) * (100 - iSpreadProb)) / GC.getNumCorporationInfos());

		if (GC.getGameINLINE().getSorenRandNum(100, "Unit Spread Corporation") < iSpreadProb)
		{
			pCity->setHasCorporation(eCorporation, true, true, false);
		}
		else
		{
			CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_CORPORATION_FAILED_TO_SPREAD", getNameKey(), GC.getCorporationInfo(eCorporation).getChar(), pCity->getNameKey());
			gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_NOSPREAD", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pCity->getX_INLINE(), pCity->getY_INLINE());
		}
	}

	if (plot()->isActiveVisible(false))
	{
		NotifyEntity(MISSION_SPREAD_CORPORATION);
	}

	kill(true);

	return true;
}


bool CvUnit::canJoin(const CvPlot* pPlot, SpecialistTypes eSpecialist) const
{
	CvCity* pCity;

	if (eSpecialist == NO_SPECIALIST)
	{
		return false;
	}

	if (!(m_pUnitInfo->getGreatPeoples(eSpecialist)))
	{
		return false;
	}

	pCity = pPlot->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (!(pCity->canJoin()))
	{
		return false;
	}

	if (pCity->getTeam() != getTeam())
	{
		return false;
	}

	if (isDelayedDeath())
	{
		return false;
	}

	return true;
}


bool CvUnit::join(SpecialistTypes eSpecialist)
{
	CvCity* pCity;

	if (!canJoin(plot(), eSpecialist))
	{
		return false;
	}

	pCity = plot()->getPlotCity();

	if (pCity != NULL)
	{
		pCity->changeFreeSpecialistCount(eSpecialist, 1);
	}

	if (plot()->isActiveVisible(false))
	{
		NotifyEntity(MISSION_JOIN);
	}

	kill(true);

	return true;
}


bool CvUnit::canConstruct(const CvPlot* pPlot, BuildingTypes eBuilding, bool bTestVisible) const
{
	CvCity* pCity;

	if (eBuilding == NO_BUILDING)
	{
		return false;
	}

	pCity = pPlot->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (getTeam() != pCity->getTeam())
	{
		return false;
	}

	if (pCity->getNumRealBuilding(eBuilding) > 0)
	{
		return false;
	}

	if (!(m_pUnitInfo->getForceBuildings(eBuilding)))
	{
		if (!(m_pUnitInfo->getBuildings(eBuilding)))
		{
			return false;
		}

		if (!(pCity->canConstruct(eBuilding, false, bTestVisible, true)))
		{
			return false;
		}
	}

	if (isDelayedDeath())
	{
		return false;
	}

	return true;
}


bool CvUnit::construct(BuildingTypes eBuilding)
{
	CvCity* pCity;

	if (!canConstruct(plot(), eBuilding))
	{
		return false;
	}

	pCity = plot()->getPlotCity();

	if (pCity != NULL)
	{
		pCity->setNumRealBuilding(eBuilding, pCity->getNumRealBuilding(eBuilding) + 1);

		CvEventReporter::getInstance().buildingBuilt(pCity, eBuilding);
	}

	if (plot()->isActiveVisible(false))
	{
		NotifyEntity(MISSION_CONSTRUCT);
	}

	kill(true);

	return true;
}


TechTypes CvUnit::getDiscoveryTech() const
{
	return ::getDiscoveryTech(getUnitType(), getOwnerINLINE());
}


int CvUnit::getDiscoverResearch(TechTypes eTech) const
{
	int iResearch;

	iResearch = (m_pUnitInfo->getBaseDiscover() + (m_pUnitInfo->getDiscoverMultiplier() * GET_TEAM(getTeam()).getTotalPopulation()));

	iResearch *= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getUnitDiscoverPercent();
	iResearch /= 100;

    if (eTech != NO_TECH)
    {
        iResearch = std::min(GET_TEAM(getTeam()).getResearchLeft(eTech), iResearch);
    }

	return std::max(0, iResearch);
}


bool CvUnit::canDiscover(const CvPlot* pPlot) const
{
	TechTypes eTech;

	eTech = getDiscoveryTech();

	if (eTech == NO_TECH)
	{
		return false;
	}

	if (getDiscoverResearch(eTech) == 0)
	{
		return false;
	}

	if (isDelayedDeath())
	{
		return false;
	}

	return true;
}


bool CvUnit::discover()
{
	TechTypes eDiscoveryTech;

	if (!canDiscover(plot()))
	{
		return false;
	}

	eDiscoveryTech = getDiscoveryTech();
	FAssertMsg(eDiscoveryTech != NO_TECH, "DiscoveryTech is not assigned a valid value");

	GET_TEAM(getTeam()).changeResearchProgress(eDiscoveryTech, getDiscoverResearch(eDiscoveryTech), getOwnerINLINE());

	// K-Mod. If the AI bulbs something, let them reconsider their current research.
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	if (!kOwner.isHuman() && kOwner.getCurrentResearch() != eDiscoveryTech)
	{
		kOwner.clearResearchQueue();
	}
	// K-Mod end

	if (plot()->isActiveVisible(false))
	{
		NotifyEntity(MISSION_DISCOVER);
	}

	kill(true);

	return true;
}


int CvUnit::getMaxHurryProduction(CvCity* pCity) const
{
	int iProduction;

	iProduction = (m_pUnitInfo->getBaseHurry() + (m_pUnitInfo->getHurryMultiplier() * pCity->getPopulation()));

	iProduction *= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getUnitHurryPercent();
	iProduction /= 100;

	return std::max(0, iProduction);
}


int CvUnit::getHurryProduction(const CvPlot* pPlot) const
{
	CvCity* pCity;
	int iProduction;

	pCity = pPlot->getPlotCity();

	if (pCity == NULL)
	{
		return 0;
	}

	iProduction = getMaxHurryProduction(pCity);

	iProduction = std::min(pCity->productionLeft(), iProduction);

	return std::max(0, iProduction);
}


bool CvUnit::canHurry(const CvPlot* pPlot, bool bTestVisible) const
{
	if (isDelayedDeath())
	{
		return false;
	}

	CvCity* pCity;

	if (getHurryProduction(pPlot) == 0)
	{
		return false;
	}

	pCity = pPlot->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (pCity->getProductionTurnsLeft() == 1)
	{
		return false;
	}

	if (!bTestVisible)
	{
		if (!(pCity->isProductionBuilding()))
		{
			return false;
		}
	}

	return true;
}


bool CvUnit::hurry()
{
	CvCity* pCity;

	if (!canHurry(plot()))
	{
		return false;
	}

	pCity = plot()->getPlotCity();

	if (pCity != NULL)
	{
		pCity->changeProduction(getHurryProduction(plot()));
	}

	if (plot()->isActiveVisible(false))
	{
		NotifyEntity(MISSION_HURRY);
	}

	kill(true);

	return true;
}


int CvUnit::getTradeGold(const CvPlot* pPlot) const
{
	CvCity* pCapitalCity;
	CvCity* pCity;
	int iGold;

	pCity = pPlot->getPlotCity();
	pCapitalCity = GET_PLAYER(getOwnerINLINE()).getCapitalCity();

	if (pCity == NULL)
	{
		return 0;
	}

	iGold = (m_pUnitInfo->getBaseTrade() + (m_pUnitInfo->getTradeMultiplier() * ((pCapitalCity != NULL) ? pCity->calculateTradeProfit(pCapitalCity) : 0)));

	iGold *= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getUnitTradePercent();
	iGold /= 100;

	return std::max(0, iGold);
}


bool CvUnit::canTrade(const CvPlot* pPlot, bool bTestVisible) const
{
	if (isDelayedDeath())
	{
		return false;
	}

	CvCity* pCity = pPlot->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	// K-Mod. if (getTradeGold(pPlot) == 0) use to be here. I've moved it to the bottom, for efficiency.

	if (!canEnterArea(pPlot->getTeam(), pPlot->area()))
	{
		return false;
	}

	if (!bTestVisible)
	{
		if (pCity->getTeam() == getTeam())
		{
			return false;
		}
	}

	if (getTradeGold(pPlot) == 0)
	{
		return false;
	}

	return true;
}


bool CvUnit::trade()
{
	if (!canTrade(plot()))
	{
		return false;
	}

	GET_PLAYER(getOwnerINLINE()).changeGold(getTradeGold(plot()));

	if (plot()->isActiveVisible(false))
	{
		NotifyEntity(MISSION_TRADE);
	}

	kill(true);

	return true;
}


int CvUnit::getGreatWorkCulture(const CvPlot* pPlot) const
{
	int iCulture;

/**
*** K-Mod, 7/dec/10, Karadoc
*** culture from great works now scales linearly with the era of the player.
*** (the base number has been reduced in the xml accordingly)
**/
	/* original bts code
	iCulture = m_pUnitInfo->getGreatWorkCulture();
	*/
	iCulture = m_pUnitInfo->getGreatWorkCulture() * (GET_PLAYER(getOwnerINLINE()).getCurrentEra());
/**
*** K-Mod end
**/

	iCulture *= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getUnitGreatWorkPercent();
	iCulture /= 100;

	return std::max(0, iCulture);
}


bool CvUnit::canGreatWork(const CvPlot* pPlot) const
{
	if (isDelayedDeath())
	{
		return false;
	}

	CvCity* pCity = pPlot->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (pCity->getOwnerINLINE() != getOwnerINLINE())
	{
		return false;
	}

	if (getGreatWorkCulture(pPlot) == 0)
	{
		return false;
	}

	return true;
}


bool CvUnit::greatWork()
{
	if (!canGreatWork(plot()))
	{
		return false;
	}

	CvCity* pCity = plot()->getPlotCity();

	if (pCity != NULL)
	{
		pCity->setCultureUpdateTimer(0);
		pCity->setOccupationTimer(0);

		int iCultureToAdd = 100 * getGreatWorkCulture(plot());
/**
*** K-Mod, 6/dec/10, Karadoc
*** apply culture in one hit. We don't need fake 'free city culture' anymore.
**/
		/* original bts code
		int iNumTurnsApplied = (GC.getDefineINT("GREAT_WORKS_CULTURE_TURNS") * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getUnitGreatWorkPercent()) / 100;

		for (int i = 0; i < iNumTurnsApplied; ++i)
		{
			pCity->changeCultureTimes100(getOwnerINLINE(), iCultureToAdd / iNumTurnsApplied, true, true);
		}

		if (iNumTurnsApplied > 0)
		{
			pCity->changeCultureTimes100(getOwnerINLINE(), iCultureToAdd % iNumTurnsApplied, false, true);
		}
		*/
		pCity->changeCultureTimes100(getOwnerINLINE(), iCultureToAdd, true, true);
		GET_PLAYER(getOwnerINLINE()).AI_updateCommerceWeights(); // significant culture change may cause signficant weight changes.
/**
*** K-Mod end
**/
	}

	if (plot()->isActiveVisible(false))
	{
		NotifyEntity(MISSION_GREAT_WORK);
	}

	kill(true);

	return true;
}


int CvUnit::getEspionagePoints(const CvPlot* pPlot) const
{
	int iEspionagePoints;

	iEspionagePoints = m_pUnitInfo->getEspionagePoints();

	iEspionagePoints *= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getUnitGreatWorkPercent();
	iEspionagePoints /= 100;

	return std::max(0, iEspionagePoints);
}

bool CvUnit::canInfiltrate(const CvPlot* pPlot, bool bTestVisible) const
{
	if (isDelayedDeath())
	{
		return false;
	}

	if (GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		return false;
	}

	if (getEspionagePoints(NULL) == 0)
	{
		return false;
	}

	CvCity* pCity = pPlot->getPlotCity();
	if (pCity == NULL || pCity->isBarbarian())
	{
		return false;
	}

	if (!bTestVisible)
	{
		if (NULL != pCity && pCity->getTeam() == getTeam())
		{
			return false;
		}
	}

	return true;
}


bool CvUnit::infiltrate()
{
	if (!canInfiltrate(plot()))
	{
		return false;
	}

	int iPoints = getEspionagePoints(NULL);
	GET_TEAM(getTeam()).changeEspionagePointsAgainstTeam(GET_PLAYER(plot()->getOwnerINLINE()).getTeam(), iPoints);
	GET_TEAM(getTeam()).changeEspionagePointsEver(iPoints);

	if (plot()->isActiveVisible(false))
	{
		NotifyEntity(MISSION_INFILTRATE);
	}

	kill(true);

	return true;
}


bool CvUnit::canEspionage(const CvPlot* pPlot, bool bTestVisible) const
{
	if (isDelayedDeath())
	{
		return false;
	}

	if (!isSpy())
	{
		return false;
	}

	if (GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		return false;
	}

	PlayerTypes ePlotOwner = pPlot->getOwnerINLINE();
	if (NO_PLAYER == ePlotOwner)
	{
		return false;
	}

	CvPlayer& kTarget = GET_PLAYER(ePlotOwner);

	if (kTarget.isBarbarian())
	{
		return false;
	}

	if (kTarget.getTeam() == getTeam())
	{
		return false;
	}

	if (GET_TEAM(getTeam()).isVassal(kTarget.getTeam()))
	{
		return false;
	}

	if (!bTestVisible)
	{
		if (isMadeAttack())
		{
			return false;
		}

		if (hasMoved())
		{
			return false;
		}

		if (kTarget.getTeam() != getTeam() && !isInvisible(kTarget.getTeam(), false))
		{
			return false;
		}
	}

	return true;
}

bool CvUnit::espionage(EspionageMissionTypes eMission, int iData)
{
	if (!canEspionage(plot()))
	{
		return false;
	}

	PlayerTypes eTargetPlayer = plot()->getOwnerINLINE();

	if (NO_ESPIONAGEMISSION == eMission)
	{
		FAssert(GET_PLAYER(getOwnerINLINE()).isHuman());
		CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_DOESPIONAGE);
		if (NULL != pInfo)
		{
			gDLL->getInterfaceIFace()->addPopup(pInfo, getOwnerINLINE(), true);
		}
	}
	else if (GC.getEspionageMissionInfo(eMission).isTwoPhases() && -1 == iData)
	{
		FAssert(GET_PLAYER(getOwnerINLINE()).isHuman());
		CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_DOESPIONAGE_TARGET);
		if (NULL != pInfo)
		{
			pInfo->setData1(eMission);
			gDLL->getInterfaceIFace()->addPopup(pInfo, getOwnerINLINE(), true);
		}
	}
	else
	{
		if (testSpyIntercepted(eTargetPlayer, true, GC.getEspionageMissionInfo(eMission).getDifficultyMod()))
		{
			return false;
		}

		if (GET_PLAYER(getOwnerINLINE()).doEspionageMission(eMission, eTargetPlayer, plot(), iData, this))
		{
			if (plot()->isActiveVisible(false))
			{
				NotifyEntity(MISSION_ESPIONAGE);
			}

			if (!testSpyIntercepted(eTargetPlayer, true, GC.getDefineINT("ESPIONAGE_SPY_MISSION_ESCAPE_MOD")))
			{
				setFortifyTurns(0);
				setMadeAttack(true);
				finishMoves();

				CvCity* pCapital = GET_PLAYER(getOwnerINLINE()).getCapitalCity();
				if (NULL != pCapital)
				{
					setXY(pCapital->getX_INLINE(), pCapital->getY_INLINE(), false, false, false);

					CvWString szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_SPY_SUCCESS", getNameKey(), pCapital->getNameKey());
					gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_POSITIVE_DINK", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"), pCapital->getX_INLINE(), pCapital->getY_INLINE(), true, true);
				}
			}
			// K-Mod
			if (getTeam() == GC.getGameINLINE().getActiveTeam())
				gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
			// K-Mod end
			return true;
		}
	}

	return false;
}

bool CvUnit::testSpyIntercepted(PlayerTypes eTargetPlayer, bool bMission, int iModifier)
{
	CvPlayer& kTargetPlayer = GET_PLAYER(eTargetPlayer);

	if (kTargetPlayer.isBarbarian())
	{
		return false;
	}

	if (GC.getGameINLINE().getSorenRandNum(10000, "Spy Interception") >= getSpyInterceptPercent(kTargetPlayer.getTeam(), bMission) * (100 + iModifier))
	{
		return false;
	}

	CvString szFormatNoReveal;
	CvString szFormatReveal;

	if (GET_TEAM(kTargetPlayer.getTeam()).getCounterespionageModAgainstTeam(getTeam()) > 0)
	{
		szFormatNoReveal = "TXT_KEY_SPY_INTERCEPTED_MISSION";
		szFormatReveal = "TXT_KEY_SPY_INTERCEPTED_MISSION_REVEAL";
	}
	else if (plot()->isEspionageCounterSpy(kTargetPlayer.getTeam()))
	{
		szFormatNoReveal = "TXT_KEY_SPY_INTERCEPTED_SPY";
		szFormatReveal = "TXT_KEY_SPY_INTERCEPTED_SPY_REVEAL";
	}
	else
	{
		szFormatNoReveal = "TXT_KEY_SPY_INTERCEPTED";
		szFormatReveal = "TXT_KEY_SPY_INTERCEPTED_REVEAL";
	}

	CvWString szCityName = kTargetPlayer.getCivilizationShortDescription();
	CvCity* pClosestCity = GC.getMapINLINE().findCity(getX_INLINE(), getY_INLINE(), eTargetPlayer, kTargetPlayer.getTeam(), true, false);
	if (pClosestCity != NULL)
	{
		szCityName = pClosestCity->getName();
	}

	CvWString szBuffer = gDLL->getText(szFormatReveal.GetCString(), GET_PLAYER(getOwnerINLINE()).getCivilizationAdjectiveKey(), getNameKey(), kTargetPlayer.getCivilizationAdjectiveKey(), szCityName.GetCString());
	gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_EXPOSED", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), getX_INLINE(), getY_INLINE(), true, true);

	if (GC.getGameINLINE().getSorenRandNum(100, "Spy Reveal identity") < GC.getDefineINT("ESPIONAGE_SPY_REVEAL_IDENTITY_PERCENT"))
	{
		if (!isEnemy(kTargetPlayer.getTeam()))
		{   // advc.130j:
			GET_PLAYER(eTargetPlayer).AI_rememberEvent(getOwnerINLINE(), MEMORY_SPY_CAUGHT);
		}

		gDLL->getInterfaceIFace()->addHumanMessage(eTargetPlayer, true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_EXPOSE", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), getX_INLINE(), getY_INLINE(), true, true);
	}
	else
	{
		szBuffer = gDLL->getText(szFormatNoReveal.GetCString(), getNameKey(), kTargetPlayer.getCivilizationAdjectiveKey(), szCityName.GetCString());
		gDLL->getInterfaceIFace()->addHumanMessage(eTargetPlayer, true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_EXPOSE", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), getX_INLINE(), getY_INLINE(), true, true);
	}

	if (plot()->isActiveVisible(false))
	{
		NotifyEntity(MISSION_SURRENDER);
	}

	kill(true);

	return true;
}

int CvUnit::getSpyInterceptPercent(TeamTypes eTargetTeam, bool bMission) const
{
	FAssert(isSpy());
	FAssert(getTeam() != eTargetTeam);

	int iSuccess = 0;

	/* original bts code
	int iTargetPoints = GET_TEAM(eTargetTeam).getEspionagePointsEver();
	int iOurPoints = GET_TEAM(getTeam()).getEspionagePointsEver();
	iSuccess += (GC.getDefineINT("ESPIONAGE_INTERCEPT_SPENDING_MAX") * iTargetPoints) / std::max(1, iTargetPoints + iOurPoints); */
	// K-Mod. Scale based on the teams' population.
	{
		const CvTeam& kTeam = GET_TEAM(getTeam());
		const CvTeam& kTargetTeam = GET_TEAM(eTargetTeam);

		int iPopScale = 5 * GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities();
		int iTargetPoints = 10 * kTargetTeam.getEspionagePointsEver() / std::max(1, iPopScale + kTargetTeam.getTotalPopulation(false));
		int iOurPoints = 10 * kTeam.getEspionagePointsEver() / std::max(1, iPopScale + kTeam.getTotalPopulation(false));
		iSuccess += GC.getDefineINT("ESPIONAGE_INTERCEPT_SPENDING_MAX") * iTargetPoints / std::max(1, iTargetPoints + iOurPoints);
	}
	// K-Mod end

	if (plot()->isEspionageCounterSpy(eTargetTeam))
	{
		iSuccess += GC.getDefineINT("ESPIONAGE_INTERCEPT_COUNTERSPY");
	}

	if (GET_TEAM(eTargetTeam).getCounterespionageModAgainstTeam(getTeam()) > 0)
	{
		iSuccess += GC.getDefineINT("ESPIONAGE_INTERCEPT_COUNTERESPIONAGE_MISSION");
	}

	// K-Mod. I've added the following condition for the recent mission bonus, to make spies less likely to be caught while exploring during peace time.
	if (bMission || atWar(getTeam(), eTargetTeam) || GET_TEAM(eTargetTeam).getCounterespionageModAgainstTeam(getTeam()) > 0 || plot()->isEspionageCounterSpy(eTargetTeam)) // K-Mod
	{
		if (0 == getFortifyTurns() || plot()->plotCount(PUF_isSpy, -1, -1, NO_PLAYER, getTeam()) > 1)
			iSuccess += GC.getDefineINT("ESPIONAGE_INTERCEPT_RECENT_MISSION");
	}

	return std::min(100, std::max(0, iSuccess));
}

bool CvUnit::isIntruding() const
{
	TeamTypes eLocalTeam = plot()->getTeam();
	
	if (NO_TEAM == eLocalTeam || eLocalTeam == getTeam())
	{
		return false;
	}

	// UNOFFICIAL_PATCH Start
	// * Vassal's spies no longer caught in master's territory
	//if (GET_TEAM(eLocalTeam).isVassal(getTeam()))
	if (GET_TEAM(eLocalTeam).isVassal(getTeam()) || GET_TEAM(getTeam()).isVassal(eLocalTeam))
	// UNOFFICIAL_PATCH End
	{
		return false;
	} // <advc.034>
	if(GET_TEAM(eLocalTeam).isDisengage(getTeam()))
		return false; // </advc.034>
	return true;
}

bool CvUnit::canGoldenAge(const CvPlot* pPlot, bool bTestVisible) const
{
	if (!isGoldenAge())
	{
		return false;
	}

	if (!bTestVisible)
	{
		if (GET_PLAYER(getOwnerINLINE()).unitsRequiredForGoldenAge() > GET_PLAYER(getOwnerINLINE()).unitsGoldenAgeReady())
		{
			return false;
		}
	}

	return true;
}


bool CvUnit::goldenAge()
{
	if (!canGoldenAge(plot()))
	{
		return false;
	}

	GET_PLAYER(getOwnerINLINE()).killGoldenAgeUnits(this);

	GET_PLAYER(getOwnerINLINE()).changeGoldenAgeTurns(GET_PLAYER(getOwnerINLINE()).getGoldenAgeLength());
	GET_PLAYER(getOwnerINLINE()).changeNumUnitGoldenAges(1);

	if (plot()->isActiveVisible(false))
	{
		NotifyEntity(MISSION_GOLDEN_AGE);
	}

	kill(true);

	return true;
}


bool CvUnit::canBuild(const CvPlot* pPlot, BuildTypes eBuild, bool bTestVisible) const
{
    FAssertMsg(eBuild < GC.getNumBuildInfos(), "Index out of bounds");
	if (!(m_pUnitInfo->getBuilds(eBuild)))
	{
		return false;
	}

	if (!(GET_PLAYER(getOwnerINLINE()).canBuild(pPlot, eBuild, false, bTestVisible)))
	{
		return false;
	}

	if (!pPlot->isValidDomainForAction(*this))
	{
		return false;
	}

	return true;
}

// Returns true if build finished...
bool CvUnit::build(BuildTypes eBuild)
{
	bool bFinished;

	FAssertMsg(eBuild < GC.getNumBuildInfos(), "Invalid Build");

	if (!canBuild(plot(), eBuild))
	{
		return false;
	}

	// Note: notify entity must come before changeBuildProgress - because once the unit is done building,
	// that function will notify the entity to stop building.
	NotifyEntity((MissionTypes)GC.getBuildInfo(eBuild).getMissionType());

	GET_PLAYER(getOwnerINLINE()).changeGold(-(GET_PLAYER(getOwnerINLINE()).getBuildCost(plot(), eBuild)));

	bFinished = plot()->changeBuildProgress(eBuild, workRate(false), getTeam());

	finishMoves(); // needs to be at bottom because movesLeft() can affect workRate()...

	if (bFinished)
	{
		if (GC.getBuildInfo(eBuild).isKill())
		{
			kill(true);
		}
	}

	// Python Event
	CvEventReporter::getInstance().unitBuildImprovement(this, eBuild, bFinished);

	return bFinished;
}


bool CvUnit::canPromote(PromotionTypes ePromotion, int iLeaderUnitId) const
{
	if (iLeaderUnitId >= 0)
	{
		if (iLeaderUnitId == getID())
		{
			return false;
		}

		// The command is always possible if it's coming from a Warlord unit that gives just experience points
		CvUnit* pWarlord = GET_PLAYER(getOwnerINLINE()).getUnit(iLeaderUnitId);
		if (pWarlord && 
			NO_UNIT != pWarlord->getUnitType() && 
			pWarlord->getUnitInfo().getLeaderExperience() > 0 && 
			NO_PROMOTION == pWarlord->getUnitInfo().getLeaderPromotion() &&
			canAcquirePromotionAny())
		{
			return true;
		}
	}

	if (ePromotion == NO_PROMOTION)
	{
		return false;
	}

	if (!canAcquirePromotion(ePromotion))
	{
		return false;
	}

	if (GC.getPromotionInfo(ePromotion).isLeader())
	{
		if (iLeaderUnitId >= 0)
		{
			CvUnit* pWarlord = GET_PLAYER(getOwnerINLINE()).getUnit(iLeaderUnitId);
			if (pWarlord && NO_UNIT != pWarlord->getUnitType())
			{
				return (pWarlord->getUnitInfo().getLeaderPromotion() == ePromotion);
			}
		}
		return false;
	}
	else
	{
		if (!isReadyForPromotion()) // advc.002e
		{
			return false;
		}
	}

	return true;
}

void CvUnit::promote(PromotionTypes ePromotion, int iLeaderUnitId)
{
	if (!canPromote(ePromotion, iLeaderUnitId))
	{
		return;
	}

	if (iLeaderUnitId >= 0)
	{
		CvUnit* pWarlord = GET_PLAYER(getOwnerINLINE()).getUnit(iLeaderUnitId);
		if (pWarlord)
		{
			pWarlord->giveExperience();
			if (!pWarlord->getNameNoDesc().empty())
			{
				setName(pWarlord->getNameKey());
			}

			//update graphics models
			m_eLeaderUnitType = pWarlord->getUnitType();
			reloadEntity();
		}
	}

	if (!GC.getPromotionInfo(ePromotion).isLeader())
	{
		changeLevel(1);
		changeDamage(-(getDamage() / 2));
	}

	setHasPromotion(ePromotion, true);

	testPromotionReady();

	CvSelectionGroup::path_finder.Reset(); // K-Mod. (This currently isn't important, because the AI doesn't use promotions mid-turn anyway.)

	if (IsSelected())
	{
		gDLL->getInterfaceIFace()->playGeneralSound(GC.getPromotionInfo(ePromotion).getSound());

		gDLL->getInterfaceIFace()->setDirty(UnitInfo_DIRTY_BIT, true);
		gDLL->getFAStarIFace()->ForceReset(&GC.getInterfacePathFinder()); // K-Mod.
	}
	else
	{
		setInfoBarDirty(true);
	}

	CvEventReporter::getInstance().unitPromoted(this, ePromotion);
}

bool CvUnit::lead(int iUnitId)
{
	if (!canLead(plot(), iUnitId))
	{
		return false;
	}

	PromotionTypes eLeaderPromotion = (PromotionTypes)m_pUnitInfo->getLeaderPromotion();

	if (-1 == iUnitId)
	{
		CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_LEADUNIT, eLeaderPromotion, getID());
		if (pInfo)
		{
			gDLL->getInterfaceIFace()->addPopup(pInfo, getOwnerINLINE(), true);
		}
		return false;
	}
	else
	{
		CvUnit* pUnit = GET_PLAYER(getOwnerINLINE()).getUnit(iUnitId);

		if (!pUnit || !pUnit->canPromote(eLeaderPromotion, getID()))			
		{
			return false;
		}

		pUnit->joinGroup(NULL, true, true);

		pUnit->promote(eLeaderPromotion, getID());

		if (plot()->isActiveVisible(false))
		{
			NotifyEntity(MISSION_LEAD);
		}

		kill(true);

		return true;
	}
}


int CvUnit::canLead(const CvPlot* pPlot, int iUnitId) const
{
	PROFILE_FUNC();

	if (isDelayedDeath())
	{
		return 0;
	}

	if (NO_UNIT == getUnitType())
	{
		return 0;
	}

	int iNumUnits = 0;
	CvUnitInfo& kUnitInfo = getUnitInfo();

	if (-1 == iUnitId)
	{
		CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();
		while(pUnitNode != NULL)
		{
			CvUnit* pUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = pPlot->nextUnitNode(pUnitNode);

			if (pUnit && pUnit != this && pUnit->getOwnerINLINE() == getOwnerINLINE() && pUnit->canPromote((PromotionTypes)kUnitInfo.getLeaderPromotion(), getID()))
			{
				++iNumUnits;
			}
		}
	}
	else
	{
		CvUnit* pUnit = GET_PLAYER(getOwnerINLINE()).getUnit(iUnitId);
		if (pUnit && pUnit != this && pUnit->canPromote((PromotionTypes)kUnitInfo.getLeaderPromotion(), getID()))
		{
			iNumUnits = 1;
		}
	}
	return iNumUnits;
}


int CvUnit::canGiveExperience(const CvPlot* pPlot) const
{
	int iNumUnits = 0;

	if (NO_UNIT != getUnitType() && m_pUnitInfo->getLeaderExperience() > 0)
	{
		CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();
		while(pUnitNode != NULL)
		{
			CvUnit* pUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = pPlot->nextUnitNode(pUnitNode);

			if (pUnit && pUnit != this && pUnit->getOwnerINLINE() == getOwnerINLINE() && pUnit->canAcquirePromotionAny())
			{
				++iNumUnits;
			}
		}
	}

	return iNumUnits;
}

bool CvUnit::giveExperience()
{
	CvPlot* pPlot = plot();

	if (pPlot)
	{
		int iNumUnits = canGiveExperience(pPlot);
		if (iNumUnits > 0)
		{
			int iTotalExperience = getStackExperienceToGive(iNumUnits);

			int iMinExperiencePerUnit = iTotalExperience / iNumUnits;
			int iRemainder = iTotalExperience % iNumUnits;

			CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();
			int i = 0;
			while(pUnitNode != NULL)
			{
				CvUnit* pUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = pPlot->nextUnitNode(pUnitNode);

				if (pUnit && pUnit != this && pUnit->getOwnerINLINE() == getOwnerINLINE() && pUnit->canAcquirePromotionAny())
				{
					pUnit->changeExperience(i < iRemainder ? iMinExperiencePerUnit+1 : iMinExperiencePerUnit);
					pUnit->testPromotionReady();
				}

				i++;
			}

			return true;
		}
	}

	return false;
}

int CvUnit::getStackExperienceToGive(int iNumUnits) const
{
	//return (m_pUnitInfo->getLeaderExperience() * (100 + std::min(50, (iNumUnits - 1) * GC.getDefineINT("WARLORD_EXTRA_EXPERIENCE_PER_UNIT_PERCENT")))) / 100;
	// K-Mod, +50% is too low as a maximum.
	return (m_pUnitInfo->getLeaderExperience() * (100 + std::min(GC.getDefineINT("WARLORD_MAXIMUM_EXTRA_EXPERIENCE_PERCENT"), (iNumUnits - 1) * GC.getDefineINT("WARLORD_EXTRA_EXPERIENCE_PER_UNIT_PERCENT")))) / 100;
}

int CvUnit::upgradePrice(UnitTypes eUnit) const
{
	int iPrice;

	CyArgsList argsList;
	argsList.add(getOwnerINLINE());
	argsList.add(getID());
	argsList.add((int) eUnit);
	long lResult=0;

	if (GC.getUSE_UNIT_UPGRADE_PRICE_CALLBACK()) // K-Mod. block unused python callbacks
	{
		gDLL->getPythonIFace()->callFunction(PYGameModule, "getUpgradePriceOverride", argsList.makeFunctionArgs(), &lResult);
		if (lResult >= 0)
		{
			return lResult;
		}
	}

	if (isBarbarian())
	{
		return 0;
	}

	iPrice = GC.getDefineINT("BASE_UNIT_UPGRADE_COST");

	iPrice += (std::max(0, (GET_PLAYER(getOwnerINLINE()).getProductionNeeded(eUnit) - GET_PLAYER(getOwnerINLINE()).getProductionNeeded(getUnitType()))) * GC.getDefineINT("UNIT_UPGRADE_COST_PER_PRODUCTION"));

	if (!isHuman() && !isBarbarian())
	{
		iPrice *= GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAIUnitUpgradePercent();
		iPrice /= 100;
		// advc.250d: Commented out
		/*iPrice *= std::max(0, ((GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAIPerEraModifier() * GET_PLAYER(getOwnerINLINE()).getCurrentEra()) + 100));
		iPrice /= 100;*/
	}

	iPrice -= (iPrice * getUpgradeDiscount()) / 100;

	return iPrice;
}


bool CvUnit::upgradeAvailable(UnitTypes eFromUnit, UnitClassTypes eToUnitClass, int iCount) const
{
	UnitTypes eLoopUnit;
	int iI;
	int numUnitClassInfos = GC.getNumUnitClassInfos();

	if (iCount > numUnitClassInfos)
	{
		return false;
	}

	CvUnitInfo &fromUnitInfo = GC.getUnitInfo(eFromUnit);

	if (fromUnitInfo.getUpgradeUnitClass(eToUnitClass))
	{
		return true;
	}

	for (iI = 0; iI < numUnitClassInfos; iI++)
	{
		if (fromUnitInfo.getUpgradeUnitClass(iI))
		{
			eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

			if (eLoopUnit != NO_UNIT)
			{
				if (upgradeAvailable(eLoopUnit, eToUnitClass, (iCount + 1)))
				{
					return true;
				}
			}
		}
	}

	return false;
}


bool CvUnit::canUpgrade(UnitTypes eUnit, bool bTestVisible) const
{
	if (eUnit == NO_UNIT)
	{
		return false;
	}

	if(!isReadyForUpgrade())
	{
		return false;
	}
	
	if (!bTestVisible)
	{
		if (GET_PLAYER(getOwnerINLINE()).getGold() < upgradePrice(eUnit))
		{
			return false;
		}
	}
	
	if (hasUpgrade(eUnit))
	{
		return true;
	}

	return false;
}

bool CvUnit::isReadyForUpgrade() const
{
	if (!canMove())
	{
		return false;
	}

	if (plot()->getTeam() != getTeam())
	{
		return false;
	}

	return true;
}

// has upgrade is used to determine if an upgrade is possible,
// it specifically does not check whether the unit can move, whether the current plot is owned, enough gold
// those are checked in canUpgrade()
// does not search all cities, only checks the closest one
bool CvUnit::hasUpgrade(bool bSearch) const
{
	return (getUpgradeCity(bSearch) != NULL);
}

// has upgrade is used to determine if an upgrade is possible,
// it specifically does not check whether the unit can move, whether the current plot is owned, enough gold
// those are checked in canUpgrade()
// does not search all cities, only checks the closest one
bool CvUnit::hasUpgrade(UnitTypes eUnit, bool bSearch) const
{
	return (getUpgradeCity(eUnit, bSearch) != NULL);
}

// finds the 'best' city which has a valid upgrade for the unit,
// it specifically does not check whether the unit can move, or if the player has enough gold to upgrade
// those are checked in canUpgrade()
// if bSearch is true, it will check every city, if not, it will only check the closest valid city
// NULL result means the upgrade is not possible
CvCity* CvUnit::getUpgradeCity(bool bSearch) const
{
	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());
	UnitAITypes eUnitAI = AI_getUnitAIType();
	CvArea* pArea = area();

	int iCurrentValue = kPlayer.AI_unitValue(getUnitType(), eUnitAI, pArea);

	int iBestSearchValue = MAX_INT;
	CvCity* pBestUpgradeCity = NULL;
	
	for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
	{
		int iNewValue = kPlayer.AI_unitValue(((UnitTypes)iI), eUnitAI, pArea);
		if (iNewValue > iCurrentValue)
		{
			int iSearchValue;
			CvCity* pUpgradeCity = getUpgradeCity((UnitTypes)iI, bSearch, &iSearchValue);
			if (pUpgradeCity != NULL)
			{
				// if not searching or close enough, then this match will do
				if (!bSearch || iSearchValue < 16)
				{
					return pUpgradeCity;
				}
				
				if (iSearchValue < iBestSearchValue)
				{
					iBestSearchValue = iSearchValue;
					pBestUpgradeCity = pUpgradeCity;
				}
			}
		}
	}

	return pBestUpgradeCity;
}

// finds the 'best' city which has a valid upgrade for the unit, to eUnit type
// it specifically does not check whether the unit can move, or if the player has enough gold to upgrade
// those are checked in canUpgrade()
// if bSearch is true, it will check every city, if not, it will only check the closest valid city
// if iSearchValue non NULL, then on return it will be the city's proximity value, lower is better
// NULL result means the upgrade is not possible
CvCity* CvUnit::getUpgradeCity(UnitTypes eUnit, bool bSearch, int* iSearchValue) const
{
	if (eUnit == NO_UNIT)
	{
		return NULL; // kmodx: was "false"; five more occurrences of this in this function
	}
	
	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());
	CvUnitInfo& kUnitInfo = GC.getUnitInfo(eUnit);

	if (GC.getCivilizationInfo(kPlayer.getCivilizationType()).getCivilizationUnits(kUnitInfo.getUnitClassType()) != eUnit)
	{
		return NULL;
	}

	if (!upgradeAvailable(getUnitType(), ((UnitClassTypes)(kUnitInfo.getUnitClassType()))))
	{
		return NULL;
	}

	if (kUnitInfo.getCargoSpace() < getCargo())
	{
		return NULL;
	}

	if (getCargo() > 0) // K-Mod. (no point looping through everything if there is no cargo anyway.)
	{
		CLLNode<IDInfo>* pUnitNode = plot()->headUnitNode();
		while (pUnitNode != NULL)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = plot()->nextUnitNode(pUnitNode);

			if (pLoopUnit->getTransportUnit() == this)
			{
				if (kUnitInfo.getSpecialCargo() != NO_SPECIALUNIT)
				{
					if (kUnitInfo.getSpecialCargo() != pLoopUnit->getSpecialUnitType())
					{
						return NULL;
					}
				}

				if (kUnitInfo.getDomainCargo() != NO_DOMAIN)
				{
					if (kUnitInfo.getDomainCargo() != pLoopUnit->getDomainType())
					{
						return NULL;
					}
				}
			}
		}
	}
	
	// sea units must be built on the coast
	bool bCoastalOnly = (getDomainType() == DOMAIN_SEA);

	// results
	int iBestValue = MAX_INT;
	CvCity* pBestCity = NULL;
		
	// if search is true, check every city for our team
	if (bSearch)
	{
		// air units can travel any distance
		bool bIgnoreDistance = (getDomainType() == DOMAIN_AIR);

		TeamTypes eTeam = getTeam();
		int iArea = getArea();
		int iX = getX_INLINE(), iY = getY_INLINE();

		// check every player on our team's cities
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			// is this player on our team?
			CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
			if (kLoopPlayer.isAlive() && kLoopPlayer.getTeam() == eTeam)
			{
				int iLoop;
				for (CvCity* pLoopCity = kLoopPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kLoopPlayer.nextCity(&iLoop))
				{
					// if coastal only, then make sure we are coast
					CvArea* pWaterArea = NULL;
					if (!bCoastalOnly || ((pWaterArea = pLoopCity->waterArea()) != NULL && !pWaterArea->isLake()))
					{
						// can this city train this unit?
						//if (pLoopCity->canTrain(eUnit, false, false, true))
						// advc.001b: Replacing the above
						if(pLoopCity->canUpgradeTo(eUnit))
						{
							// if we do not care about distance, then the first match will do
							if (bIgnoreDistance)
							{
								// if we do not care about distance, then return 1 for value
								if (iSearchValue != NULL)
								{
									*iSearchValue = 1;
								}

								return pLoopCity;
							}

							int iValue = plotDistance(iX, iY, pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE());

							// if not same area, not as good (lower numbers are better)
							if (iArea != pLoopCity->getArea() && (!bCoastalOnly || iArea != pWaterArea->getID()))
							{
								iValue *= 16;
							}

							// if we cannot path there, not as good (lower numbers are better)
							if (!generatePath(pLoopCity->plot(), 0, true))
							{
								iValue *= 16;
							}

							if (iValue < iBestValue)
							{
								iBestValue = iValue;
								pBestCity = pLoopCity;
							}
						}
					}
				}
			}
		}
	}
	else
	{
		// find the closest city
		CvCity* pClosestCity = GC.getMapINLINE().findCity(getX_INLINE(), getY_INLINE(), NO_PLAYER, getTeam(), true, bCoastalOnly);
		if (pClosestCity != NULL)
		{
			// if we can train, then return this city (otherwise it will return NULL)
			//if (pClosestCity->canTrain(eUnit, false, false, true))
			// advc.001b: Replacing the above
			if(pClosestCity->canUpgradeTo(eUnit))
			{
				// did not search, always return 1 for search value
				iBestValue = 1;

				pBestCity = pClosestCity;
			}
		}
	}

	// return the best value, if non-NULL
	if (iSearchValue != NULL)
	{
		*iSearchValue = iBestValue;
	}

	return pBestCity;
}

CvUnit* CvUnit::upgrade(UnitTypes eUnit) // K-Mod: this now returns the new unit.
{
	CvUnit* pUpgradeUnit;

	if (!canUpgrade(eUnit))
	{
		return this;
	}

	GET_PLAYER(getOwnerINLINE()).changeGold(-(upgradePrice(eUnit)));

	pUpgradeUnit = GET_PLAYER(getOwnerINLINE()).initUnit(eUnit, getX_INLINE(), getY_INLINE(), AI_getUnitAIType());

	FAssertMsg(pUpgradeUnit != NULL, "UpgradeUnit is not assigned a valid value");

	pUpgradeUnit->convert(this);
	pUpgradeUnit->joinGroup(getGroup()); // K-Mod, swapped order with convert. (otherwise units on boats would be ungrouped.)

	pUpgradeUnit->finishMoves();

	if (pUpgradeUnit->getLeaderUnitType() == NO_UNIT)
	{
		if (pUpgradeUnit->getExperience() > GC.getDefineINT("MAX_EXPERIENCE_AFTER_UPGRADE"))
		{
			pUpgradeUnit->setExperience(GC.getDefineINT("MAX_EXPERIENCE_AFTER_UPGRADE"));
		}
	}

	if (gUnitLogLevel > 2)
	{
		CvWString szString;
		getUnitAIString(szString, AI_getUnitAIType());
		logBBAI("    %S spends %d to upgrade %S to %S, unit AI %S", GET_PLAYER(getOwnerINLINE()).getCivilizationDescription(0), upgradePrice(eUnit), getName(0).GetCString(), pUpgradeUnit->getName(0).GetCString(), szString.GetCString());
	}

	return pUpgradeUnit; // K-Mod
}


HandicapTypes CvUnit::getHandicapType() const
{
	return GET_PLAYER(getOwnerINLINE()).getHandicapType();
}


CivilizationTypes CvUnit::getCivilizationType() const
{
	return GET_PLAYER(getOwnerINLINE()).getCivilizationType();
}

const wchar* CvUnit::getVisualCivAdjective(TeamTypes eForTeam) const
{
	if (getVisualOwner(eForTeam) == getOwnerINLINE())
	{
		return GC.getCivilizationInfo(getCivilizationType()).getAdjectiveKey();
	}

	return L"";
}

SpecialUnitTypes CvUnit::getSpecialUnitType() const
{
	return ((SpecialUnitTypes)(m_pUnitInfo->getSpecialUnitType()));
}


UnitTypes CvUnit::getCaptureUnitType(CivilizationTypes eCivilization) const
{
	FAssert(eCivilization != NO_CIVILIZATION);
	return ((m_pUnitInfo->getUnitCaptureClassType() == NO_UNITCLASS) ? NO_UNIT : (UnitTypes)GC.getCivilizationInfo(eCivilization).getCivilizationUnits(m_pUnitInfo->getUnitCaptureClassType()));
}


UnitCombatTypes CvUnit::getUnitCombatType() const
{
	return ((UnitCombatTypes)(m_pUnitInfo->getUnitCombatType()));
}


DomainTypes CvUnit::getDomainType() const
{
	return ((DomainTypes)(m_pUnitInfo->getDomainType()));
}


InvisibleTypes CvUnit::getInvisibleType() const
{
	return ((InvisibleTypes)(m_pUnitInfo->getInvisibleType()));
}

int CvUnit::getNumSeeInvisibleTypes() const
{
	return m_pUnitInfo->getNumSeeInvisibleTypes();
}

InvisibleTypes CvUnit::getSeeInvisibleType(int i) const
{
	return (InvisibleTypes)(m_pUnitInfo->getSeeInvisibleType(i));
}


int CvUnit::flavorValue(FlavorTypes eFlavor) const
{
	return m_pUnitInfo->getFlavorValue(eFlavor);
}


bool CvUnit::isBarbarian() const
{
	return GET_PLAYER(getOwnerINLINE()).isBarbarian();
}


bool CvUnit::isHuman() const
{
	return GET_PLAYER(getOwnerINLINE()).isHuman();
}


int CvUnit::visibilityRange() const
{
	return (GC.getDefineINT("UNIT_VISIBILITY_RANGE") + getExtraVisibilityRange());
}


int CvUnit::baseMoves() const
{
	return (m_pUnitInfo->getMoves() + getExtraMoves() + GET_TEAM(getTeam()).getExtraMoves(getDomainType()));
}


int CvUnit::maxMoves() const
{
	return (baseMoves() * GC.getMOVE_DENOMINATOR());
}


int CvUnit::movesLeft() const
{
	return std::max(0, (maxMoves() - getMoves()));
}


bool CvUnit::canMove() const
{
	if (isDead())
	{
		return false;
	}

	if (getMoves() >= maxMoves())
	{
		return false;
	}

	if (getImmobileTimer() > 0)
	{
		return false;
	}

	return true;
}


bool CvUnit::hasMoved()	const
{
	return (getMoves() > 0);
}


int CvUnit::airRange() const
{
	return (m_pUnitInfo->getAirRange() + getExtraAirRange());
}


int CvUnit::nukeRange() const
{
	return m_pUnitInfo->getNukeRange();
}


// XXX should this test for coal?
bool CvUnit::canBuildRoute() const
{
	int iI;

	for (iI = 0; iI < GC.getNumBuildInfos(); iI++)
	{
		if (GC.getBuildInfo((BuildTypes)iI).getRoute() != NO_ROUTE)
		{
			if (m_pUnitInfo->getBuilds(iI))
			{
				if (GET_TEAM(getTeam()).isHasTech((TechTypes)(GC.getBuildInfo((BuildTypes)iI).getTechPrereq())))
				{
					return true;
				}
			}
		}
	}

	return false;
}

BuildTypes CvUnit::getBuildType() const
{
	BuildTypes eBuild;

	if (getGroup()->headMissionQueueNode() != NULL)
	{
		switch (getGroup()->headMissionQueueNode()->m_data.eMissionType)
		{
		case MISSION_MOVE_TO:
			break;

		case MISSION_ROUTE_TO:
			if (getGroup()->getBestBuildRoute(plot(), &eBuild) != NO_ROUTE)
			{
				return eBuild;
			}
			break;

		case MISSION_MOVE_TO_UNIT:
		case MISSION_SKIP:
		case MISSION_SLEEP:
		case MISSION_FORTIFY:
		case MISSION_PLUNDER:
		case MISSION_AIRPATROL:
		case MISSION_SEAPATROL:
		case MISSION_HEAL:
		case MISSION_SENTRY:
		case MISSION_AIRLIFT:
		case MISSION_NUKE:
		case MISSION_RECON:
		case MISSION_PARADROP:
		case MISSION_AIRBOMB:
		case MISSION_BOMBARD:
		case MISSION_RANGE_ATTACK:
		case MISSION_PILLAGE:
		case MISSION_SABOTAGE:
		case MISSION_DESTROY:
		case MISSION_STEAL_PLANS:
		case MISSION_FOUND:
		case MISSION_SPREAD:
		case MISSION_SPREAD_CORPORATION:
		case MISSION_JOIN:
		case MISSION_CONSTRUCT:
		case MISSION_DISCOVER:
		case MISSION_HURRY:
		case MISSION_TRADE:
		case MISSION_GREAT_WORK:
		case MISSION_INFILTRATE:
		case MISSION_GOLDEN_AGE:
		case MISSION_LEAD:
		case MISSION_ESPIONAGE:
		case MISSION_DIE_ANIMATION:
			break;

		case MISSION_BUILD:
			return (BuildTypes)getGroup()->headMissionQueueNode()->m_data.iData1;
			break;

		default:
			FAssert(false);
			break;
		}
	}

	return NO_BUILD;
}


int CvUnit::workRate(bool bMax) const
{
	int iRate;

	if (!bMax)
	{
		if (!canMove())
		{
			return 0;
		}
	}

	iRate = m_pUnitInfo->getWorkRate();

	iRate *= std::max(0, (GET_PLAYER(getOwnerINLINE()).getWorkerSpeedModifier() + 100));
	iRate /= 100;

	if (!isHuman() && !isBarbarian())
	{
		iRate *= std::max(0, (GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAIWorkRateModifier() + 100));
		iRate /= 100;
	}

	return iRate;
}


bool CvUnit::isAnimal() const
{
	return m_pUnitInfo->isAnimal();
}


bool CvUnit::isNoBadGoodies() const
{
	return m_pUnitInfo->isNoBadGoodies();
}


bool CvUnit::isOnlyDefensive() const
{
	return m_pUnitInfo->isOnlyDefensive();
}


bool CvUnit::isNoCapture() const
{
	return m_pUnitInfo->isNoCapture();
}


bool CvUnit::isRivalTerritory() const
{
	return m_pUnitInfo->isRivalTerritory();
}


bool CvUnit::isMilitaryHappiness() const
{
	return m_pUnitInfo->isMilitaryHappiness()
			// <advc.001o>
			&& (plot()->getTeam() == getTeam() || plot()->getTeam() ==
			GET_TEAM(getTeam()).getMasterTeam()); // </advc.001o>
}


bool CvUnit::isInvestigate() const
{
	return m_pUnitInfo->isInvestigate();
}


bool CvUnit::isCounterSpy() const
{
	return m_pUnitInfo->isCounterSpy();
}


bool CvUnit::isSpy() const
{
	return m_pUnitInfo->isSpy();
}


bool CvUnit::isFound() const
{
	return m_pUnitInfo->isFound();
}


bool CvUnit::isGoldenAge() const
{
	if (isDelayedDeath())
	{
		return false;
	}

	return m_pUnitInfo->isGoldenAge();
}

bool CvUnit::canCoexistWithEnemyUnit(TeamTypes eTeam) const
{
	/* advc.104b: Without this, the AI can consider cities unreachable
	   when enemy units block a choke point. */
	if(measuringDistance != NO_TEAM) return true;

	if (NO_TEAM == eTeam)
	{
		if(alwaysInvisible())
		{
			return true;
		}
		
		return false;
	}

	if(isInvisible(eTeam, false))
	{
		return true;
	}

	return false;
}

bool CvUnit::isFighting() const
{
	return (getCombatUnit() != NULL);
}


bool CvUnit::isAttacking() const
{
	return (getAttackPlot() != NULL && !isDelayedDeath());
}


bool CvUnit::isDefending() const
{
	return (isFighting() && !isAttacking());
}


bool CvUnit::isCombat() const
{
	return (isFighting() || isAttacking());
}


int CvUnit::maxHitPoints() const
{
	return GC.getMAX_HIT_POINTS();
}


int CvUnit::currHitPoints()	const
{
	return (maxHitPoints() - getDamage());
}


bool CvUnit::isHurt() const
{
	return (getDamage() > 0);
}


bool CvUnit::isDead() const
{
	return (getDamage() >= maxHitPoints());
}


void CvUnit::setBaseCombatStr(int iCombat)
{
	m_iBaseCombat = iCombat;
}

int CvUnit::baseCombatStr() const
{
	return m_iBaseCombat;
}


// maxCombatStr can be called in four different configurations
//		pPlot == NULL, pAttacker == NULL for combat when this is the attacker
//		pPlot valid, pAttacker valid for combat when this is the defender
//		pPlot valid, pAttacker == NULL (new case), when this is the defender, attacker unknown
//		pPlot valid, pAttacker == this (new case), when the defender is unknown, but we want to calc approx str
//			note, in this last case, it is expected pCombatDetails == NULL, it does not have to be, but some 
//			values may be unexpectedly reversed in this case (iModifierTotal will be the negative sum)
int CvUnit::maxCombatStr(const CvPlot* pPlot, const CvUnit* pAttacker, CombatDetails* pCombatDetails) const
{
	int iCombat;

	FAssertMsg((pPlot == NULL) || (pPlot->getTerrainType() != NO_TERRAIN), "(pPlot == NULL) || (pPlot->getTerrainType() is not expected to be equal with NO_TERRAIN)");
	
	// handle our new special case
	const	CvPlot*	pAttackedPlot = NULL;
	bool	bAttackingUnknownDefender = false;
	if (pAttacker == this)
	{
		bAttackingUnknownDefender = true;
		pAttackedPlot = pPlot;
		
		// reset these values, we will fiddle with them below
		pPlot = NULL;
		pAttacker = NULL;
	}
	// otherwise, attack plot is the plot of us (the defender)
	else if (pAttacker != NULL)
	{
		pAttackedPlot = plot();
	}

	if (pCombatDetails != NULL)
	{
		pCombatDetails->iExtraCombatPercent = 0;
		pCombatDetails->iAnimalCombatModifierTA = 0;
		pCombatDetails->iAIAnimalCombatModifierTA = 0;
		pCombatDetails->iAnimalCombatModifierAA = 0;
		pCombatDetails->iAIAnimalCombatModifierAA = 0;
		pCombatDetails->iBarbarianCombatModifierTB = 0;
		pCombatDetails->iAIBarbarianCombatModifierTB = 0;
		pCombatDetails->iBarbarianCombatModifierAB = 0;
		pCombatDetails->iAIBarbarianCombatModifierAB = 0;
		pCombatDetails->iPlotDefenseModifier = 0;
		pCombatDetails->iFortifyModifier = 0;
		pCombatDetails->iCityDefenseModifier = 0;
		pCombatDetails->iHillsAttackModifier = 0;
		pCombatDetails->iHillsDefenseModifier = 0;
		pCombatDetails->iFeatureAttackModifier = 0;
		pCombatDetails->iFeatureDefenseModifier = 0;
		pCombatDetails->iTerrainAttackModifier = 0;
		pCombatDetails->iTerrainDefenseModifier = 0;
		pCombatDetails->iCityAttackModifier = 0;
		pCombatDetails->iDomainDefenseModifier = 0;
		pCombatDetails->iCityBarbarianDefenseModifier = 0;
		pCombatDetails->iClassDefenseModifier = 0;
		pCombatDetails->iClassAttackModifier = 0;
		pCombatDetails->iCombatModifierA = 0;
		pCombatDetails->iCombatModifierT = 0;
		pCombatDetails->iDomainModifierA = 0;
		pCombatDetails->iDomainModifierT = 0;
		pCombatDetails->iAnimalCombatModifierA = 0;
		pCombatDetails->iAnimalCombatModifierT = 0;
		pCombatDetails->iRiverAttackModifier = 0;
		pCombatDetails->iAmphibAttackModifier = 0;
		pCombatDetails->iKamikazeModifier = 0;
		pCombatDetails->iModifierTotal = 0;
		pCombatDetails->iBaseCombatStr = 0;
		pCombatDetails->iCombat = 0;
		pCombatDetails->iMaxCombatStr = 0;
		pCombatDetails->iCurrHitPoints = 0;
		pCombatDetails->iMaxHitPoints = 0;
		pCombatDetails->iCurrCombatStr = 0;
		pCombatDetails->eOwner = getOwnerINLINE();
		pCombatDetails->eVisualOwner = getVisualOwner();
		pCombatDetails->sUnitName = getName().GetCString();
	}

	if (baseCombatStr() == 0)
	{
		return 0;
	}

	int iModifier = 0;
	int iExtraModifier;

	iExtraModifier = getExtraCombatPercent();
	iModifier += iExtraModifier;
	if (pCombatDetails != NULL)
	{
		pCombatDetails->iExtraCombatPercent = iExtraModifier;
	}
	
	// do modifiers for animals and barbarians (leaving these out for bAttackingUnknownDefender case)
	if (pAttacker != NULL)
	{
		if (isAnimal())
		{
			if (pAttacker->isHuman())
			{
				// K-Mod. Give bonus based on player's difficulty, not game difficulty.
				//iExtraModifier = GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAnimalCombatModifier();
				iExtraModifier = GC.getHandicapInfo(GET_PLAYER(pAttacker->getOwnerINLINE()).getHandicapType()).getAnimalCombatModifier(); // K-Mod
				iModifier += iExtraModifier;
				if (pCombatDetails != NULL)
				{
					pCombatDetails->iAnimalCombatModifierTA = iExtraModifier;
				}
			}
			else
			{
				iExtraModifier = GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAIAnimalCombatModifier();
				iModifier += iExtraModifier;
				if (pCombatDetails != NULL)
				{
					pCombatDetails->iAIAnimalCombatModifierTA = iExtraModifier;
				}
			}
		}

		if (pAttacker->isAnimal())
		{
			if (isHuman())
			{
				//iExtraModifier = -GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAnimalCombatModifier();
				iExtraModifier = -GC.getHandicapInfo(GET_PLAYER(getOwnerINLINE()).getHandicapType()).getAnimalCombatModifier(); // K-Mod
				iModifier += iExtraModifier;
				if (pCombatDetails != NULL)
				{
					pCombatDetails->iAnimalCombatModifierAA = iExtraModifier;
				}
			}
			else
			{
				iExtraModifier = -GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAIAnimalCombatModifier();
				iModifier += iExtraModifier;
				if (pCombatDetails != NULL)
				{
					pCombatDetails->iAIAnimalCombatModifierAA = iExtraModifier;
				}
			}
		}

		if (isBarbarian())
		{
			if (pAttacker->isHuman())
			{
				//iExtraModifier = GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getBarbarianCombatModifier();
				iExtraModifier = GC.getHandicapInfo(GET_PLAYER(pAttacker->getOwnerINLINE()).getHandicapType()).getBarbarianCombatModifier(); // K-Mod
				iModifier += iExtraModifier;
				if (pCombatDetails != NULL)
				{
					pCombatDetails->iBarbarianCombatModifierTB = iExtraModifier;
				}
			}
			else
			{
				iExtraModifier = GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAIBarbarianCombatModifier();
				iModifier += iExtraModifier;
				if (pCombatDetails != NULL)
				{
					pCombatDetails->iAIBarbarianCombatModifierTB = iExtraModifier;
				}
			}
		}

		if (pAttacker->isBarbarian())
		{
			if (isHuman())
			{
				//iExtraModifier = -GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getBarbarianCombatModifier();
				iExtraModifier = -GC.getHandicapInfo(GET_PLAYER(getOwnerINLINE()).getHandicapType()).getBarbarianCombatModifier(); // K-Mod
				iModifier += iExtraModifier;
				if (pCombatDetails != NULL)
				{
					pCombatDetails->iBarbarianCombatModifierAB = iExtraModifier;
				}
			}
			else
			{
				iExtraModifier = -GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAIBarbarianCombatModifier();
				iModifier += iExtraModifier;
				if (pCombatDetails != NULL)
				{
					pCombatDetails->iAIBarbarianCombatModifierTB = iExtraModifier;
				}
			}
		}
	}
	
	// add defensive bonuses (leaving these out for bAttackingUnknownDefender case)
	if (pPlot != NULL)
	{
		if (!noDefensiveBonus())
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/30/10                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
			// When pAttacker is NULL but pPlot is not, this is a computation for this units defensive value
			// against an unknown attacker.  Always ignoring building defense in this case is a conservative estimate,
			// but causes AI to suicide against castle walls of low culture cities in early game.  Using this units
			// ignoreBuildingDefense does a little better ... in early game it corrects undervalue of castles.  One
			// downside is when medieval unit is defending a walled city against gunpowder.  Here, the over value
			// makes attacker a little more cautious, but with their tech lead it shouldn't matter too much.  Also
			// makes vulnerable units (ships, etc) feel safer in this case and potentially not leave, but ships
			// leave when ratio is pretty low anyway.

			//iExtraModifier = pPlot->defenseModifier(getTeam(), (pAttacker != NULL) ? pAttacker->ignoreBuildingDefense() : true);
			/*  <advc.012> Only functional change: feature defense is counted based
				on AI_plotDefense if the attacker is unknown */
			bool ibd = (pAttacker != NULL ?
					pAttacker->ignoreBuildingDefense() :
					ignoreBuildingDefense());
			if(pAttacker == NULL)
				iExtraModifier = GET_TEAM(getTeam()).AI_plotDefense(*pPlot, ibd);
			else iExtraModifier = pPlot->defenseModifier(getTeam(),
					ibd, pAttacker->getTeam());
			// </advc.012>
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			iModifier += iExtraModifier;
			if (pCombatDetails != NULL)
			{
				pCombatDetails->iPlotDefenseModifier = iExtraModifier;
			}
		}

		iExtraModifier = fortifyModifier();
		iModifier += iExtraModifier;
		if (pCombatDetails != NULL)
		{
			pCombatDetails->iFortifyModifier = iExtraModifier;
		}

		if (pPlot->isCity(true, getTeam()))
		{
			iExtraModifier = cityDefenseModifier();
			iModifier += iExtraModifier;
			if (pCombatDetails != NULL)
			{
				pCombatDetails->iCityDefenseModifier = iExtraModifier;
			}
		}

		if (pPlot->isHills())
		{
			iExtraModifier = hillsDefenseModifier();
			iModifier += iExtraModifier;
			if (pCombatDetails != NULL)
			{
				pCombatDetails->iHillsDefenseModifier = iExtraModifier;
			}
		}

		if (pPlot->getFeatureType() != NO_FEATURE)
		{
			iExtraModifier = featureDefenseModifier(pPlot->getFeatureType());
			iModifier += iExtraModifier;
			if (pCombatDetails != NULL)
			{
				pCombatDetails->iFeatureDefenseModifier = iExtraModifier;
			}
		}
		else
		{
			iExtraModifier = terrainDefenseModifier(pPlot->getTerrainType());
			iModifier += iExtraModifier;
			if (pCombatDetails != NULL)
			{
				pCombatDetails->iTerrainDefenseModifier = iExtraModifier;
			}
		}
	}

	// if we are attacking to an plot with an unknown defender, the calc the modifier in reverse
	if (bAttackingUnknownDefender)
	{
		pAttacker = this;
	}

	// calc attacker bonueses
	if (pAttacker != NULL && pAttackedPlot != NULL)
	{
		int iTempModifier = 0;

		if (pAttackedPlot->isCity(true, getTeam()))
		{
			iExtraModifier = -pAttacker->cityAttackModifier();
			iTempModifier += iExtraModifier;
			if (pCombatDetails != NULL)
			{
				pCombatDetails->iCityAttackModifier = iExtraModifier;
			}

			if (pAttacker->isBarbarian())
			{
				iExtraModifier = GC.getDefineINT("CITY_BARBARIAN_DEFENSE_MODIFIER");
				iTempModifier += iExtraModifier;
				if (pCombatDetails != NULL)
				{
					pCombatDetails->iCityBarbarianDefenseModifier = iExtraModifier;
				}
			}
		}

		if (pAttackedPlot->isHills())
		{
			iExtraModifier = -pAttacker->hillsAttackModifier();
			iTempModifier += iExtraModifier;
			if (pCombatDetails != NULL)
			{
				pCombatDetails->iHillsAttackModifier = iExtraModifier;
			}
		}
		
		if (pAttackedPlot->getFeatureType() != NO_FEATURE)
		{
			iExtraModifier = -pAttacker->featureAttackModifier(pAttackedPlot->getFeatureType());
			iTempModifier += iExtraModifier;
			if (pCombatDetails != NULL)
			{
				pCombatDetails->iFeatureAttackModifier = iExtraModifier;
			}
		}
		else
		{
			iExtraModifier = -pAttacker->terrainAttackModifier(pAttackedPlot->getTerrainType());
			iModifier += iExtraModifier;
			if (pCombatDetails != NULL)
			{
				pCombatDetails->iTerrainAttackModifier = iExtraModifier;
			}
		}

		// only compute comparisions if we are the defender with a known attacker
		if (!bAttackingUnknownDefender)
		{
			FAssertMsg(pAttacker != this, "pAttacker is not expected to be equal with this");

			iExtraModifier = unitClassDefenseModifier(pAttacker->getUnitClassType());
			iTempModifier += iExtraModifier;
			if (pCombatDetails != NULL)
			{
				pCombatDetails->iClassDefenseModifier = iExtraModifier;
			}

			iExtraModifier = -pAttacker->unitClassAttackModifier(getUnitClassType());
			iTempModifier += iExtraModifier;
			if (pCombatDetails != NULL)
			{
				pCombatDetails->iClassAttackModifier = iExtraModifier;
			}

			if (pAttacker->getUnitCombatType() != NO_UNITCOMBAT)
			{
				iExtraModifier = unitCombatModifier(pAttacker->getUnitCombatType());
				iTempModifier += iExtraModifier;
				if (pCombatDetails != NULL)
				{
					pCombatDetails->iCombatModifierA = iExtraModifier;
				}
			}
			if (getUnitCombatType() != NO_UNITCOMBAT)
			{
				iExtraModifier = -pAttacker->unitCombatModifier(getUnitCombatType());
				iTempModifier += iExtraModifier;
				if (pCombatDetails != NULL)
				{
					pCombatDetails->iCombatModifierT = iExtraModifier;
				}
			}

			iExtraModifier = domainModifier(pAttacker->getDomainType());
			iTempModifier += iExtraModifier;
			if (pCombatDetails != NULL)
			{
				pCombatDetails->iDomainModifierA = iExtraModifier;
			}

			iExtraModifier = -pAttacker->domainModifier(getDomainType());
			iTempModifier += iExtraModifier;
			if (pCombatDetails != NULL)
			{
				pCombatDetails->iDomainModifierT = iExtraModifier;
			}

			if (pAttacker->isAnimal())
			{
				iExtraModifier = animalCombatModifier();
				iTempModifier += iExtraModifier;
				if (pCombatDetails != NULL)
				{
					pCombatDetails->iAnimalCombatModifierA = iExtraModifier;
				}
			}

			if (isAnimal())
			{
				iExtraModifier = -pAttacker->animalCombatModifier();
				iTempModifier += iExtraModifier;
				if (pCombatDetails != NULL)
				{
					pCombatDetails->iAnimalCombatModifierT = iExtraModifier;
				}
			}
		}

		if (!(pAttacker->isRiver()))
		{
			if (pAttacker->plot()->isRiverCrossing(directionXY(pAttacker->plot(), pAttackedPlot)))
			{
				iExtraModifier = -GC.getRIVER_ATTACK_MODIFIER();
				iTempModifier += iExtraModifier;
				if (pCombatDetails != NULL)
				{
					pCombatDetails->iRiverAttackModifier = iExtraModifier;
				}
			}
		}

		if (!(pAttacker->isAmphib()))
		{
			if (!(pAttackedPlot->isWater()) && pAttacker->plot()->isWater())
			{
				iExtraModifier = -GC.getAMPHIB_ATTACK_MODIFIER();
				iTempModifier += iExtraModifier;
				if (pCombatDetails != NULL)
				{
					pCombatDetails->iAmphibAttackModifier = iExtraModifier;
				}
			}
		}

		if (pAttacker->getKamikazePercent() != 0)
		{
			iExtraModifier = pAttacker->getKamikazePercent();
			iTempModifier += iExtraModifier;
			if (pCombatDetails != NULL)
			{
				pCombatDetails->iKamikazeModifier = iExtraModifier;
			}
		}
		
		// if we are attacking an unknown defender, then use the reverse of the modifier
		if (bAttackingUnknownDefender)
		{
			iModifier -= iTempModifier;
		}
		else
		{
			iModifier += iTempModifier;
		}
	}

	if (pCombatDetails != NULL)
	{
		pCombatDetails->iModifierTotal = iModifier;
		pCombatDetails->iBaseCombatStr = baseCombatStr();
	}

	if (iModifier > 0)
	{
		iCombat = (baseCombatStr() * (iModifier + 100));
	}
	else
	{
		iCombat = ((baseCombatStr() * 10000) / (100 - iModifier));
	}

	if (pCombatDetails != NULL)
	{
		pCombatDetails->iCombat = iCombat;
		pCombatDetails->iMaxCombatStr = std::max(1, iCombat);
		pCombatDetails->iCurrHitPoints = currHitPoints();
		pCombatDetails->iMaxHitPoints = maxHitPoints();
		pCombatDetails->iCurrCombatStr = ((pCombatDetails->iMaxCombatStr * pCombatDetails->iCurrHitPoints) / pCombatDetails->iMaxHitPoints);
	}

	return std::max(1, iCombat);
}


int CvUnit::currCombatStr(const CvPlot* pPlot, const CvUnit* pAttacker, CombatDetails* pCombatDetails) const
{
	return ((maxCombatStr(pPlot, pAttacker, pCombatDetails) * currHitPoints()) / maxHitPoints());
}


int CvUnit::currFirepower(const CvPlot* pPlot, const CvUnit* pAttacker) const
{
	return ((maxCombatStr(pPlot, pAttacker) + currCombatStr(pPlot, pAttacker) + 1) / 2);
}

// this nomalizes str by firepower, useful for quick odds calcs
// the effect is that a damaged unit will have an effective str lowered by firepower/maxFirepower
// doing the algebra, this means we mulitply by 1/2(1 + currHP)/maxHP = (maxHP + currHP) / (2 * maxHP)
int CvUnit::currEffectiveStr(const CvPlot* pPlot, const CvUnit* pAttacker, CombatDetails* pCombatDetails) const
{
	int currStr = currCombatStr(pPlot, pAttacker, pCombatDetails);

	currStr *= (maxHitPoints() + currHitPoints());
	currStr /= (2 * maxHitPoints());
	
	return currStr;
}

float CvUnit::maxCombatStrFloat(const CvPlot* pPlot, const CvUnit* pAttacker) const
{
	return (((float)(maxCombatStr(pPlot, pAttacker))) / 100.0f);
}


float CvUnit::currCombatStrFloat(const CvPlot* pPlot, const CvUnit* pAttacker) const
{
	return (((float)(currCombatStr(pPlot, pAttacker))) / 100.0f);
}


bool CvUnit::canFight() const
{
	return (baseCombatStr() > 0);
}


bool CvUnit::canAttack() const
{
	if (!canFight())
	{
		return false;
	}

	if (isOnlyDefensive())
	{
		return false;
	}

	return true;
}
bool CvUnit::canAttack(const CvUnit& defender) const
{
	if (!canAttack())
	{
		return false;
	}

	if (defender.getDamage() >= combatLimit())
	{
		return false;
	}

	// Artillery can't amphibious attack
	if (plot()->isWater() && !defender.plot()->isWater())
	{
		if (combatLimit() < 100)
		{
			return false;
		}
	}

	return true;
}

bool CvUnit::canDefend(const CvPlot* pPlot) const
{
	if (pPlot == NULL)
	{
		pPlot = plot();
	}

	if (!canFight())
	{
		return false;
	}

	if (!pPlot->isValidDomainForAction(*this))
	{
		if (GC.getDefineINT("LAND_UNITS_CAN_ATTACK_WATER_CITIES") == 0)
		{
			return false;
		}
	}

	return true;
}


bool CvUnit::canSiege(TeamTypes eTeam) const
{
	if (!canDefend())
	{
		return false;
	}

	if (!isEnemy(eTeam))
	{
		return false;
	}

	if (!isNeverInvisible())
	{
		return false;
	}

	return true;
}

// <dlph.8> "Added function for checking whether a unit is a combat unit."
bool CvUnit::canCombat() const {

	return (baseCombatStr() > 0 || airBaseCombatStr() > 0 || nukeRange() >= 0);
} // </dlph.8>

int CvUnit::airBaseCombatStr() const
{
	return m_pUnitInfo->getAirCombat();
}


int CvUnit::airMaxCombatStr(const CvUnit* pOther) const
{
	int iModifier;
	int iCombat;

	if (airBaseCombatStr() == 0)
	{
		return 0;
	}

	iModifier = getExtraCombatPercent();

	if (getKamikazePercent() != 0)
	{
		iModifier += getKamikazePercent();
	}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						8/16/08		DanF5771 & jdog5000	*/
/* 																			*/
/* 	Bugfix																	*/
/********************************************************************************/
/* original BTS code
	if (getExtraCombatPercent() != 0)
	{
		iModifier += getExtraCombatPercent();
	}
*/
	// ExtraCombatPercent already counted above
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	if (NULL != pOther)
	{
		if (pOther->getUnitCombatType() != NO_UNITCOMBAT)
		{
			iModifier += unitCombatModifier(pOther->getUnitCombatType());
		}

		iModifier += domainModifier(pOther->getDomainType());

		if (pOther->isAnimal())
		{
			iModifier += animalCombatModifier();
		}
	}

	if (iModifier > 0)
	{
		iCombat = (airBaseCombatStr() * (iModifier + 100));
	}
	else
	{
		iCombat = ((airBaseCombatStr() * 10000) / (100 - iModifier));
	}

	return std::max(1, iCombat);
}


int CvUnit::airCurrCombatStr(const CvUnit* pOther) const
{
	return ((airMaxCombatStr(pOther) * currHitPoints()) / maxHitPoints());
}


float CvUnit::airMaxCombatStrFloat(const CvUnit* pOther) const
{
	return (((float)(airMaxCombatStr(pOther))) / 100.0f);
}


float CvUnit::airCurrCombatStrFloat(const CvUnit* pOther) const
{
	return (((float)(airCurrCombatStr(pOther))) / 100.0f);
}


int CvUnit::combatLimit() const
{
	return m_pUnitInfo->getCombatLimit();
}


int CvUnit::airCombatLimit() const
{
	return m_pUnitInfo->getAirCombatLimit();
}


bool CvUnit::canAirAttack() const
{
	return (airBaseCombatStr() > 0);
}


bool CvUnit::canAirDefend(const CvPlot* pPlot) const
{
	if (pPlot == NULL)
	{
		pPlot = plot();
	}

	if (maxInterceptionProbability() == 0)
	{
		return false;
	}

	if (getDomainType() != DOMAIN_AIR)
	{
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       10/30/09                     Mongoose & jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original bts code
		if (!pPlot->isValidDomainForLocation(*this))
*/
		// From Mongoose SDK
		// Land units which are cargo cannot intercept
		if (!pPlot->isValidDomainForLocation(*this) || isCargo())
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
		{
			return false;
		}
	}

	return true;
}


int CvUnit::airCombatDamage(const CvUnit* pDefender) const
{
	CvCity* pCity;
	CvPlot* pPlot;
	int iOurStrength;
	int iTheirStrength;
	int iStrengthFactor;
	int iDamage;

	pPlot = pDefender->plot();

	iOurStrength = airCurrCombatStr(pDefender);
	FAssertMsg(iOurStrength > 0, "Air combat strength is expected to be greater than zero");
	iTheirStrength = pDefender->maxCombatStr(pPlot, this);

	iStrengthFactor = ((iOurStrength + iTheirStrength + 1) / 2);

	iDamage = std::max(1, ((GC.getDefineINT("AIR_COMBAT_DAMAGE") * (iOurStrength + iStrengthFactor)) / (iTheirStrength + iStrengthFactor)));

	pCity = pPlot->getPlotCity();

	if (pCity != NULL)
	{
		iDamage *= std::max(0, (pCity->getAirModifier() + 100));
		iDamage /= 100;
	}

	return iDamage;
}


int CvUnit::rangeCombatDamage(const CvUnit* pDefender) const
{
	CvPlot* pPlot;
	int iOurStrength;
	int iTheirStrength;
	int iStrengthFactor;
	int iDamage;

	pPlot = pDefender->plot();

	iOurStrength = airCurrCombatStr(pDefender);
	FAssertMsg(iOurStrength > 0, "Combat strength is expected to be greater than zero");
	iTheirStrength = pDefender->maxCombatStr(pPlot, this);

	iStrengthFactor = ((iOurStrength + iTheirStrength + 1) / 2);

	iDamage = std::max(1, ((GC.getDefineINT("RANGE_COMBAT_DAMAGE") * (iOurStrength + iStrengthFactor)) / (iTheirStrength + iStrengthFactor)));

	return iDamage;
}


CvUnit* CvUnit::bestInterceptor(const CvPlot* pPlot) const
{
	CvUnit* pLoopUnit;
	CvUnit* pBestUnit;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;

	iBestValue = 0;
	pBestUnit = NULL;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (isEnemy(GET_PLAYER((PlayerTypes)iI).getTeam()) && !isInvisible(GET_PLAYER((PlayerTypes)iI).getTeam(), false, false))
			{
				for(pLoopUnit = GET_PLAYER((PlayerTypes)iI).firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = GET_PLAYER((PlayerTypes)iI).nextUnit(&iLoop))
				{
					if (pLoopUnit->canAirDefend())
					{
						if (!pLoopUnit->isMadeInterception())
						{
							if ((pLoopUnit->getDomainType() != DOMAIN_AIR) || !(pLoopUnit->hasMoved()))
							{
								if ((pLoopUnit->getDomainType() != DOMAIN_AIR) || (pLoopUnit->getGroup()->getActivityType() == ACTIVITY_INTERCEPT))
								{
									if (plotDistance(pLoopUnit->getX_INLINE(), pLoopUnit->getY_INLINE(), pPlot->getX_INLINE(), pPlot->getY_INLINE()) <= pLoopUnit->airRange())
									{
										iValue = pLoopUnit->currInterceptionProbability();

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

	return pBestUnit;
}


CvUnit* CvUnit::bestSeaPillageInterceptor(CvUnit* pPillager, int iMinOdds) const
{
	CvUnit* pBestUnit = NULL;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/21/10                                jdog5000      */
/*                                                                                              */
/* Lead From Behind                                                                             */
/************************************************************************************************/
	// From Lead From Behind by UncutDragon
	int pBestUnitRank = -1;

	for (int iDX = -1; iDX <= 1; ++iDX)
	{
		for (int iDY = -1; iDY <= 1; ++iDY)
		{
			CvPlot* pLoopPlot = plotXY(pPillager->getX_INLINE(), pPillager->getY_INLINE(), iDX, iDY);

			if (NULL != pLoopPlot)
			{
				CLLNode<IDInfo>* pUnitNode = pLoopPlot->headUnitNode();

				while (NULL != pUnitNode)
				{
					CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
					pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

					if (NULL != pLoopUnit)
					{	// advc.030: Replacing the line below
						if(pLoopUnit->canEnterArea(*pPillager->plot()->area()))
						//if (pLoopUnit->area() == pPillager->plot()->area())
						{
							if (!pLoopUnit->isInvisible(getTeam(), false))
							{
								if (isEnemy(pLoopUnit->getTeam()))
								{
									if (DOMAIN_SEA == pLoopUnit->getDomainType())
									{
										if (ACTIVITY_PATROL == pLoopUnit->getGroup()->getActivityType())
										{
											// UncutDragon
											// orignal
											//if (NULL == pBestUnit || pLoopUnit->isBetterDefenderThan(pBestUnit, this))
											// modified (added extra parameter)
											if (NULL == pBestUnit || pLoopUnit->isBetterDefenderThan(pBestUnit, this, &pBestUnitRank))
											// /UncutDragon
											{
												if (getCombatOdds(pPillager, pLoopUnit) < iMinOdds)
												{
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
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	return pBestUnit;
}


bool CvUnit::isAutomated() const
{
	return getGroup()->isAutomated();
}


bool CvUnit::isWaiting() const
{
	return getGroup()->isWaiting();
}


bool CvUnit::isFortifyable() const
{
	if (!canFight() || noDefensiveBonus() || ((getDomainType() != DOMAIN_LAND) && (getDomainType() != DOMAIN_IMMOBILE)))
	{
		return false;
	}

	return true;
}


int CvUnit::fortifyModifier() const
{
	if (!isFortifyable())
	{
		return 0;
	}

	return (getFortifyTurns() * GC.getFORTIFY_MODIFIER_PER_TURN());
}


int CvUnit::experienceNeeded() const
{
	if (GC.getUSE_GET_EXPERIENCE_NEEDED_CALLBACK()) // K-Mod. I've writen C to replace the python callback.
	{
		// Use python to determine pillage amounts...
		long lExperienceNeeded;

		lExperienceNeeded = 0;

		CyArgsList argsList;
		argsList.add(getLevel());	// pass in the units level
		argsList.add(getOwnerINLINE());	// pass in the units 

		gDLL->getPythonIFace()->callFunction(PYGameModule, "getExperienceNeeded", argsList.makeFunctionArgs(),&lExperienceNeeded);

		//iExperienceNeeded = (int)lExperienceNeeded;
		lExperienceNeeded = std::min((long)MAX_INT, lExperienceNeeded); // K-Mod

		if (lExperienceNeeded >= 0) // K-Mod
			return (int)lExperienceNeeded;
	}
	// K-Mod. C version of the original python code.
	// Note: python rounds towards negative infinity, but C++ rounds towards 0.
	// So the code needs to be slightly different to achieve the same effect.
	int iExperienceNeeded = getLevel() * getLevel() + 1;

	int iModifier = GET_PLAYER(getOwnerINLINE()).getLevelExperienceModifier();
	if (iModifier != 0)
		iExperienceNeeded = (iExperienceNeeded * (100+iModifier) + 99) / 100;

	return iExperienceNeeded;
	// K-Mod end
}


int CvUnit::attackXPValue() const
{
	return m_pUnitInfo->getXPValueAttack()
			- (isBarbarian() ? 1 : 0); // advc.312;;
}


int CvUnit::defenseXPValue() const
{
	return m_pUnitInfo->getXPValueDefense();
}


int CvUnit::maxXPValue() const
{
	int iMaxValue;

	iMaxValue = MAX_INT;

	if (isAnimal())
	{
		iMaxValue = std::min(iMaxValue, GC.getDefineINT("ANIMAL_MAX_XP_VALUE"));
	}

	if (isBarbarian())
	{
		iMaxValue = std::min(iMaxValue, GC.getDefineINT("BARBARIAN_MAX_XP_VALUE"));
	}

	return iMaxValue;
}


int CvUnit::firstStrikes() const
{
	return std::max(0, (m_pUnitInfo->getFirstStrikes() + getExtraFirstStrikes()));
}


int CvUnit::chanceFirstStrikes() const
{
	return std::max(0, (m_pUnitInfo->getChanceFirstStrikes() + getExtraChanceFirstStrikes()));
}


int CvUnit::maxFirstStrikes() const
{
	return (firstStrikes() + chanceFirstStrikes());
}


bool CvUnit::isRanged() const
{
	int i;
	CvUnitInfo * pkUnitInfo = &getUnitInfo();
	for ( i = 0; i < pkUnitInfo->getGroupDefinitions(); i++ )
	{
		if ( !getArtInfo(i, GET_PLAYER(getOwnerINLINE()).getCurrentEra())->getActAsRanged() )
		{
			return false;
		}
	}	
	return true;
}


bool CvUnit::alwaysInvisible() const
{
	return m_pUnitInfo->isInvisible();
}


bool CvUnit::immuneToFirstStrikes() const
{
	return (m_pUnitInfo->isFirstStrikeImmune() || (getImmuneToFirstStrikesCount() > 0));
}


bool CvUnit::noDefensiveBonus() const
{
	return m_pUnitInfo->isNoDefensiveBonus();
}


bool CvUnit::ignoreBuildingDefense() const
{
	return m_pUnitInfo->isIgnoreBuildingDefense();
}


bool CvUnit::canMoveImpassable() const
{
	return m_pUnitInfo->isCanMoveImpassable();
}

bool CvUnit::canMoveAllTerrain() const
{
	return m_pUnitInfo->isCanMoveAllTerrain();
}

bool CvUnit::flatMovementCost() const
{
	return m_pUnitInfo->isFlatMovementCost();
}


bool CvUnit::ignoreTerrainCost() const
{
	return m_pUnitInfo->isIgnoreTerrainCost();
}


bool CvUnit::isNeverInvisible() const
{
	return (!alwaysInvisible() && (getInvisibleType() == NO_INVISIBLE));
}


bool CvUnit::isInvisible(TeamTypes eTeam, bool bDebug, bool bCheckCargo) const
{
	if (bDebug && GC.getGameINLINE().isDebugMode())
	{
		return false;
	}

	if (getTeam() == eTeam)
	{
		return false;
	}

	if (alwaysInvisible())
	{
		return true;
	}

	if (bCheckCargo && isCargo())
	{
		return true;
	}

	if (getInvisibleType() == NO_INVISIBLE)
	{
		return false;
	}

	return !(plot()->isInvisibleVisible(eTeam, getInvisibleType()));
}


bool CvUnit::isNukeImmune() const
{
	return m_pUnitInfo->isNukeImmune();
}


int CvUnit::maxInterceptionProbability() const
{
	return std::max(0, m_pUnitInfo->getInterceptionProbability() + getExtraIntercept());
}


int CvUnit::currInterceptionProbability() const
{
	if (getDomainType() != DOMAIN_AIR)
	{
		return maxInterceptionProbability();
	}
	else
	{
		return ((maxInterceptionProbability() * currHitPoints()) / maxHitPoints());
	}
}


int CvUnit::evasionProbability() const
{
	return std::max(0, m_pUnitInfo->getEvasionProbability() + getExtraEvasion());
}


int CvUnit::withdrawalProbability() const
{
	if (getDomainType() == DOMAIN_LAND && plot()->isWater())
	{
		return 0;
	}

	return std::max(0, (m_pUnitInfo->getWithdrawalProbability() + getExtraWithdrawal()));
}


int CvUnit::collateralDamage() const
{
	return std::max(0, (m_pUnitInfo->getCollateralDamage()));
}


int CvUnit::collateralDamageLimit() const
{
	return std::max(0, m_pUnitInfo->getCollateralDamageLimit() * GC.getMAX_HIT_POINTS() / 100);
}


int CvUnit::collateralDamageMaxUnits() const
{
	return std::max(0, m_pUnitInfo->getCollateralDamageMaxUnits());
}


int CvUnit::cityAttackModifier() const
{
	return (m_pUnitInfo->getCityAttackModifier() + getExtraCityAttackPercent());
}


int CvUnit::cityDefenseModifier() const
{
	return (m_pUnitInfo->getCityDefenseModifier() + getExtraCityDefensePercent());
}


int CvUnit::animalCombatModifier() const
{
	return m_pUnitInfo->getAnimalCombatModifier();
}


int CvUnit::hillsAttackModifier() const
{
	return (m_pUnitInfo->getHillsAttackModifier() + getExtraHillsAttackPercent());
}


int CvUnit::hillsDefenseModifier() const
{
	return (m_pUnitInfo->getHillsDefenseModifier() + getExtraHillsDefensePercent());
}


int CvUnit::terrainAttackModifier(TerrainTypes eTerrain) const
{
	FAssertMsg(eTerrain >= 0, "eTerrain is expected to be non-negative (invalid Index)");
	FAssertMsg(eTerrain < GC.getNumTerrainInfos(), "eTerrain is expected to be within maximum bounds (invalid Index)");
	return (m_pUnitInfo->getTerrainAttackModifier(eTerrain) + getExtraTerrainAttackPercent(eTerrain));
}


int CvUnit::terrainDefenseModifier(TerrainTypes eTerrain) const
{
	FAssertMsg(eTerrain >= 0, "eTerrain is expected to be non-negative (invalid Index)");
	FAssertMsg(eTerrain < GC.getNumTerrainInfos(), "eTerrain is expected to be within maximum bounds (invalid Index)");
	return (m_pUnitInfo->getTerrainDefenseModifier(eTerrain) + getExtraTerrainDefensePercent(eTerrain));
}


int CvUnit::featureAttackModifier(FeatureTypes eFeature) const
{
	FAssertMsg(eFeature >= 0, "eFeature is expected to be non-negative (invalid Index)");
	FAssertMsg(eFeature < GC.getNumFeatureInfos(), "eFeature is expected to be within maximum bounds (invalid Index)");
	return (m_pUnitInfo->getFeatureAttackModifier(eFeature) + getExtraFeatureAttackPercent(eFeature));
}

int CvUnit::featureDefenseModifier(FeatureTypes eFeature) const
{
	FAssertMsg(eFeature >= 0, "eFeature is expected to be non-negative (invalid Index)");
	FAssertMsg(eFeature < GC.getNumFeatureInfos(), "eFeature is expected to be within maximum bounds (invalid Index)");
	return (m_pUnitInfo->getFeatureDefenseModifier(eFeature) + getExtraFeatureDefensePercent(eFeature));
}

int CvUnit::unitClassAttackModifier(UnitClassTypes eUnitClass) const
{
	FAssertMsg(eUnitClass >= 0, "eUnitClass is expected to be non-negative (invalid Index)");
	FAssertMsg(eUnitClass < GC.getNumUnitClassInfos(), "eUnitClass is expected to be within maximum bounds (invalid Index)");
	return m_pUnitInfo->getUnitClassAttackModifier(eUnitClass);
}


int CvUnit::unitClassDefenseModifier(UnitClassTypes eUnitClass) const
{
	FAssertMsg(eUnitClass >= 0, "eUnitClass is expected to be non-negative (invalid Index)");
	FAssertMsg(eUnitClass < GC.getNumUnitClassInfos(), "eUnitClass is expected to be within maximum bounds (invalid Index)");
	return m_pUnitInfo->getUnitClassDefenseModifier(eUnitClass);
}


int CvUnit::unitCombatModifier(UnitCombatTypes eUnitCombat) const
{
	FAssertMsg(eUnitCombat >= 0, "eUnitCombat is expected to be non-negative (invalid Index)");
	FAssertMsg(eUnitCombat < GC.getNumUnitCombatInfos(), "eUnitCombat is expected to be within maximum bounds (invalid Index)");
	return (m_pUnitInfo->getUnitCombatModifier(eUnitCombat) + getExtraUnitCombatModifier(eUnitCombat));
}


int CvUnit::domainModifier(DomainTypes eDomain) const
{
	FAssertMsg(eDomain >= 0, "eDomain is expected to be non-negative (invalid Index)");
	FAssertMsg(eDomain < NUM_DOMAIN_TYPES, "eDomain is expected to be within maximum bounds (invalid Index)");
	return (m_pUnitInfo->getDomainModifier(eDomain) + getExtraDomainModifier(eDomain));
}


int CvUnit::bombardRate() const
{
	return (m_pUnitInfo->getBombardRate() + getExtraBombardRate());
}


int CvUnit::airBombBaseRate() const
{
	return m_pUnitInfo->getBombRate();
}


int CvUnit::airBombCurrRate() const
{
	return ((airBombBaseRate() * currHitPoints()) / maxHitPoints());
}


SpecialUnitTypes CvUnit::specialCargo() const
{
	return ((SpecialUnitTypes)(m_pUnitInfo->getSpecialCargo()));
}


DomainTypes CvUnit::domainCargo() const
{
	return ((DomainTypes)(m_pUnitInfo->getDomainCargo()));
}


int CvUnit::cargoSpace() const
{
	return m_iCargoCapacity;
}

void CvUnit::changeCargoSpace(int iChange)
{
	if (iChange != 0)
	{
		m_iCargoCapacity += iChange;
		FAssert(m_iCargoCapacity >= 0);
		setInfoBarDirty(true);
	}
}

bool CvUnit::isFull() const
{
	return (getCargo() >= cargoSpace());
}


int CvUnit::cargoSpaceAvailable(SpecialUnitTypes eSpecialCargo, DomainTypes eDomainCargo) const
{
	if (specialCargo() != NO_SPECIALUNIT)
	{
		if (specialCargo() != eSpecialCargo)
		{
			return 0;
		}
	}

	if (domainCargo() != NO_DOMAIN)
	{
		if (domainCargo() != eDomainCargo)
		{
			return 0;
		}
	}

	return std::max(0, (cargoSpace() - getCargo()));
}


bool CvUnit::hasCargo() const
{
	return (getCargo() > 0);
}


/* bool CvUnit::canCargoAllMove() const
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pPlot;

	pPlot = plot();

	pUnitNode = pPlot->headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = pPlot->nextUnitNode(pUnitNode);

		if (pLoopUnit->getTransportUnit() == this)
		{
			if (pLoopUnit->getDomainType() == DOMAIN_LAND)
			{
				if (!(pLoopUnit->canMove()))
				{
					return false;
				}
			}
		}
	}

	return true;
} */

bool CvUnit::canCargoEnterArea(TeamTypes eTeam, const CvArea* pArea, bool bIgnoreRightOfPassage) const
{
	CvPlot* pPlot = plot();

	CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();

	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = pPlot->nextUnitNode(pUnitNode);

		if (pLoopUnit->getTransportUnit() == this)
		{
			if (!pLoopUnit->canEnterArea(eTeam, pArea, bIgnoreRightOfPassage))
			{
				return false;
			}
		}
	}

	return true;
}

int CvUnit::getUnitAICargo(UnitAITypes eUnitAI) const
{
	int iCount = 0;

	std::vector<CvUnit*> aCargoUnits;
	getCargoUnits(aCargoUnits);
	for (uint i = 0; i < aCargoUnits.size(); ++i)
	{
		if (aCargoUnits[i]->AI_getUnitAIType() == eUnitAI)
		{
			++iCount;
		}
	}

	return iCount;
}


int CvUnit::getID() const
{
	return m_iID;
}


int CvUnit::getIndex() const
{
	return (getID() & FLTA_INDEX_MASK);
}


IDInfo CvUnit::getIDInfo() const
{
	IDInfo unit(getOwnerINLINE(), getID());
	return unit;
}


void CvUnit::setID(int iID)
{
	m_iID = iID;
}


int CvUnit::getGroupID() const
{
	return m_iGroupID;
}


bool CvUnit::isInGroup() const
{
	return(getGroupID() != FFreeList::INVALID_INDEX);
}


bool CvUnit::isGroupHead() const // XXX is this used???
{
	return (getGroup()->getHeadUnit() == this);
}


CvSelectionGroup* CvUnit::getGroup() const
{
	return GET_PLAYER(getOwnerINLINE()).getSelectionGroup(getGroupID());
}


bool CvUnit::canJoinGroup(const CvPlot* pPlot, CvSelectionGroup* pSelectionGroup) const
{
	CvUnit* pHeadUnit;
	
	// do not allow someone to join a group that is about to be split apart
	// this prevents a case of a never-ending turn
	if (pSelectionGroup->AI_isForceSeparate())
	{
		return false;
	}
	
	if (pSelectionGroup->getOwnerINLINE() == NO_PLAYER)
	{
		pHeadUnit = pSelectionGroup->getHeadUnit();

		if (pHeadUnit != NULL)
		{
			if (pHeadUnit->getOwnerINLINE() != getOwnerINLINE())
			{
				return false;
			}
		}
	}
	else
	{
		if (pSelectionGroup->getOwnerINLINE() != getOwnerINLINE())
		{
			return false;
		}
	}

	if (pSelectionGroup->getNumUnits() > 0)
	{
		if (!(pSelectionGroup->atPlot(pPlot)))
		{
			return false;
		}

		if (pSelectionGroup->getDomainType() != getDomainType())
		{
			return false;
		}
	}

	return true;
}

// K-Mod has edited this function to increase readability and robustness
void CvUnit::joinGroup(CvSelectionGroup* pSelectionGroup, bool bRemoveSelected, bool bRejoin)
{
	CvSelectionGroup* pOldSelectionGroup = GET_PLAYER(getOwnerINLINE()).getSelectionGroup(getGroupID());

	if (pOldSelectionGroup && pSelectionGroup == pOldSelectionGroup)
		return; // attempting to join the group we are already in

	CvPlot* pPlot = plot();
	CvSelectionGroup* pNewSelectionGroup = pSelectionGroup;

	if (pNewSelectionGroup == NULL && bRejoin)
	{
		pNewSelectionGroup = GET_PLAYER(getOwnerINLINE()).addSelectionGroup();
		pNewSelectionGroup->init(pNewSelectionGroup->getID(), getOwnerINLINE());
	}

	if (pNewSelectionGroup == NULL || canJoinGroup(pPlot, pNewSelectionGroup))
	{
		if (pOldSelectionGroup != NULL)
		{
			bool bWasHead = false;
			if (!isHuman())
			{
				if (pOldSelectionGroup->getNumUnits() > 1)
				{
					if (pOldSelectionGroup->getHeadUnit() == this)
					{
						bWasHead = true;
					}
				}
			}

			pOldSelectionGroup->removeUnit(this);

			// if we were the head, if the head unitAI changed, then force the group to separate (non-humans)
			if (bWasHead)
			{
				FAssert(pOldSelectionGroup->getHeadUnit() != NULL);
				if (pOldSelectionGroup->getHeadUnit()->AI_getUnitAIType() != AI_getUnitAIType())
				{
					pOldSelectionGroup->AI_setForceSeparate();
				}
			}
		}

		if ((pNewSelectionGroup != NULL) && pNewSelectionGroup->addUnit(this, false))
		{
			m_iGroupID = pNewSelectionGroup->getID();
		}
		else
		{
			m_iGroupID = FFreeList::INVALID_INDEX;
		}

		if (getGroup() != NULL)
		{
			// K-Mod
			if (isGroupHead())
				GET_PLAYER(getOwnerINLINE()).updateGroupCycle(getGroup());
			// K-Mod end
			if (getGroup()->getNumUnits() > 1)
			{
				/* original bts code
				getGroup()->setActivityType(ACTIVITY_AWAKE); */
				// K-Mod
				// For the AI, only wake the group in particular circumstances. This is to avoid AI deadlocks where they just keep grouping and ungroup indefinitely.
				// If the activity type is not changed at all, then that would enable exploits such as adding new units to air patrol groups to bypass the movement conditions.
				if (isHuman())
				{
					getGroup()->setAutomateType(NO_AUTOMATE);
					getGroup()->setActivityType(ACTIVITY_AWAKE);
					getGroup()->clearMissionQueue();
					// K-Mod note. the mission queue has to be cleared, because when the shift key is released, the exe automatically sends the autoMission net message.
					// (if the mission queue isn't cleared, the units will immediately begin their message whenever units are added using shift.)
				}
				else if (getGroup()->AI_getMissionAIType() == MISSIONAI_GROUP || getLastMoveTurn() == GC.getGameINLINE().getTurnSlice())
					getGroup()->setActivityType(ACTIVITY_AWAKE);
				else if (getGroup()->getActivityType() != ACTIVITY_AWAKE)
					getGroup()->setActivityType(ACTIVITY_HOLD); // don't let them cheat.
				// K-Mod end
			}
			/* original bts code
			else
			{
				GET_PLAYER(getOwnerINLINE()).updateGroupCycle(this);
			} */
		}

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			if (pPlot != NULL)
			{
				pPlot->setFlagDirty(true);
			}
		}

		if (pPlot == gDLL->getInterfaceIFace()->getSelectionPlot())
		{
			gDLL->getInterfaceIFace()->setDirty(PlotListButtons_DIRTY_BIT, true);
		}
	}

	if (bRemoveSelected && IsSelected())
	{
		gDLL->getInterfaceIFace()->removeFromSelectionList(this);
	}
}


int CvUnit::getHotKeyNumber()
{
	return m_iHotKeyNumber;
}


void CvUnit::setHotKeyNumber(int iNewValue)
{
	CvUnit* pLoopUnit;
	int iLoop;

	FAssert(getOwnerINLINE() != NO_PLAYER);

	if (getHotKeyNumber() != iNewValue)
	{
		if (iNewValue != -1)
		{
			for(pLoopUnit = GET_PLAYER(getOwnerINLINE()).firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = GET_PLAYER(getOwnerINLINE()).nextUnit(&iLoop))
			{
				if (pLoopUnit->getHotKeyNumber() == iNewValue)
				{
					pLoopUnit->setHotKeyNumber(-1);
				}
			}
		}

		m_iHotKeyNumber = iNewValue;

		if (IsSelected())
		{
			gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true);
		}
	}
}


int CvUnit::getX() const
{
	return m_iX;
}


int CvUnit::getY() const
{
	return m_iY;
}


void CvUnit::setXY(int iX, int iY, bool bGroup, bool bUpdate, bool bShow, bool bCheckPlotVisible)
{
	CLLNode<IDInfo>* pUnitNode;
	CvCity* pOldCity;
	CvCity* pNewCity;
	CvCity* pWorkingCity;
	CvUnit* pTransportUnit;
	CvUnit* pLoopUnit;
	CvPlot* pOldPlot;
	CvPlot* pNewPlot;
	CvPlot* pLoopPlot;
	CLinkList<IDInfo> oldUnits;
	//ActivityTypes eOldActivityType;
	int iI;

	// OOS!! Temporary for Out-of-Sync madness debugging...
	/*if (GC.getLogging())
	{
		if (gDLL->getChtLvl() > 0)
		{
			char szOut[1024];
			sprintf(szOut, "Player %d Unit %d (%S's %S) moving from %d:%d to %d:%d\n", getOwnerINLINE(), getID(), GET_PLAYER(getOwnerINLINE()).getNameKey(), getName().GetCString(), getX_INLINE(), getY_INLINE(), iX, iY);
			gDLL->messageControlLog(szOut);
		}
	}*/

	FAssert(!at(iX, iY));
	FAssert(!isFighting());
	FAssert((iX == INVALID_PLOT_COORD) || (GC.getMapINLINE().plotINLINE(iX, iY)->getX_INLINE() == iX));
	FAssert((iY == INVALID_PLOT_COORD) || (GC.getMapINLINE().plotINLINE(iX, iY)->getY_INLINE() == iY));

	/* if (getGroup() != NULL)
	{
		eOldActivityType = getGroup()->getActivityType();
	}
	else
	{
		eOldActivityType = NO_ACTIVITY;
	} */

	setBlockading(false);

	/* original bts code
	if (!bGroup || isCargo())
	{
		joinGroup(NULL, true);
		bShow = false;
	} */
	// K-Mod. I've adjusted the code to allow cargo units to stay in their groups when possible.
	bShow = bShow && bGroup && !isCargo();
	if (!bGroup)
		joinGroup(0, true);
	// K-Mod end

	pNewPlot = GC.getMapINLINE().plotINLINE(iX, iY);

	if (pNewPlot != NULL)
	{
		pTransportUnit = getTransportUnit();

		if (pTransportUnit != NULL)
		{
			if (!(pTransportUnit->atPlot(pNewPlot)))
			{
				setTransportUnit(NULL);
			}
		}

		if (canFight())
		{
			oldUnits.clear();

			pUnitNode = pNewPlot->headUnitNode();

			while (pUnitNode != NULL)
			{
				oldUnits.insertAtEnd(pUnitNode->m_data);
				pUnitNode = pNewPlot->nextUnitNode(pUnitNode);
			}

			pUnitNode = oldUnits.head();

			while (pUnitNode != NULL)
			{
				pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = oldUnits.next(pUnitNode);

				if (pLoopUnit != NULL)
				{
					/*  advc.001, advc.300: Otherwise, a barb city can land
						on top of an animal and trap it. */
					if(pLoopUnit->isAnimal()) {
						pLoopUnit->kill(false);
						continue;
					}
					if (isEnemy(pLoopUnit->getTeam(), pNewPlot) || pLoopUnit->isEnemy(getTeam()))
					{
						if (!pLoopUnit->canCoexistWithEnemyUnit(getTeam()))
						{
							if (NO_UNITCLASS == pLoopUnit->getUnitInfo().getUnitCaptureClassType() && pLoopUnit->canDefend(pNewPlot))
							{
								pLoopUnit->jumpToNearestValidPlot(); // can kill unit
							}
							else
							{
								if (!m_pUnitInfo->isHiddenNationality() && !pLoopUnit->getUnitInfo().isHiddenNationality())
								{
									GET_TEAM(pLoopUnit->getTeam()).changeWarWeariness(getTeam(), *pNewPlot, GC.getDefineINT("WW_UNIT_CAPTURED"));
									GET_TEAM(getTeam()).changeWarWeariness(pLoopUnit->getTeam(), *pNewPlot, GC.getDefineINT("WW_CAPTURED_UNIT"));
									GET_TEAM(getTeam()).AI_changeWarSuccess(pLoopUnit->getTeam(), GC.getDefineINT("WAR_SUCCESS_UNIT_CAPTURING"));
								}

								if (!isNoCapture())
								{
									pLoopUnit->setCapturingPlayer(getOwnerINLINE());
								}

								pLoopUnit->kill(false, getOwnerINLINE());
							}
						}
					}
				}
			}
		}

		if (pNewPlot->isGoody(getTeam()))
		{
			GET_PLAYER(getOwnerINLINE()).doGoody(pNewPlot, this);
		}
	}

	pOldPlot = plot();

	if (pOldPlot != NULL)
	{
		pOldPlot->removeUnit(this, bUpdate && !hasCargo());

		pOldPlot->changeAdjacentSight(getTeam(), visibilityRange(), false, this, true);

		pOldPlot->area()->changeUnitsPerPlayer(getOwnerINLINE(), -1);
		pOldPlot->area()->changePower(getOwnerINLINE(), -(m_pUnitInfo->getPowerValue()));

		if (AI_getUnitAIType() != NO_UNITAI)
		{
			pOldPlot->area()->changeNumAIUnits(getOwnerINLINE(), AI_getUnitAIType(), -1);
		}

		if (isAnimal())
		{
			pOldPlot->area()->changeAnimalsPerPlayer(getOwnerINLINE(), -1);
		}

		if (pOldPlot->getTeam() != getTeam() && (pOldPlot->getTeam() == NO_TEAM || !GET_TEAM(pOldPlot->getTeam()).isVassal(getTeam())))
		{
			GET_PLAYER(getOwnerINLINE()).changeNumOutsideUnits(-1);
		}

		setLastMoveTurn(GC.getGameINLINE().getTurnSlice());

		pOldCity = pOldPlot->getPlotCity();

		if (pOldCity != NULL)
		{
			if (isMilitaryHappiness())
			{
				pOldCity->changeMilitaryHappinessUnits(-1);
			}
		}

		pWorkingCity = pOldPlot->getWorkingCity();

		if (pWorkingCity != NULL)
		{
			if (canSiege(pWorkingCity->getTeam()))
			{
				pWorkingCity->AI_setAssignWorkDirty(true);
			}
		}

		if (pOldPlot->isWater())
		{
			for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
			{
				pLoopPlot = plotDirection(pOldPlot->getX_INLINE(), pOldPlot->getY_INLINE(), ((DirectionTypes)iI));

				if (pLoopPlot != NULL)
				{
					if (pLoopPlot->isWater())
					{
						pWorkingCity = pLoopPlot->getWorkingCity();

						if (pWorkingCity != NULL)
						{
							if (canSiege(pWorkingCity->getTeam()))
							{
								pWorkingCity->AI_setAssignWorkDirty(true);
							}
						}
					}
				}
			}
		}

		if (pOldPlot->isActiveVisible(true))
		{
			pOldPlot->updateMinimapColor();
		}

		if (pOldPlot == gDLL->getInterfaceIFace()->getSelectionPlot())
		{
			gDLL->getInterfaceIFace()->verifyPlotListColumn();

			gDLL->getInterfaceIFace()->setDirty(PlotListButtons_DIRTY_BIT, true);
		}
	}

	if (pNewPlot != NULL)
	{
		m_iX = pNewPlot->getX_INLINE();
		m_iY = pNewPlot->getY_INLINE();
	}
	else
	{
		m_iX = INVALID_PLOT_COORD;
		m_iY = INVALID_PLOT_COORD;
	}

	FAssertMsg(plot() == pNewPlot, "plot is expected to equal pNewPlot");

	if (pNewPlot != NULL)
	{
		pNewCity = pNewPlot->getPlotCity();

		if (pNewCity != NULL)
		{
			if (isEnemy(pNewCity->getTeam()) && !canCoexistWithEnemyUnit(pNewCity->getTeam()) && canFight())
			{
				GET_TEAM(getTeam()).changeWarWeariness(pNewCity->getTeam(), *pNewPlot, GC.getDefineINT("WW_CAPTURED_CITY"));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/14/09                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
/* original bts code
				GET_TEAM(getTeam()).AI_changeWarSuccess(pNewCity->getTeam(), GC.getDefineINT("WAR_SUCCESS_CITY_CAPTURING"));
*/
				// Double war success if capturing capital city, always a significant blow to enemy
				// pNewCity still points to old city here, hasn't been acquired yet
				/*  advc.123d: Make it only +150% for capital. Not such a big blow
					if it's not the original capital (but can't easily check
					whether it is). */
				GET_TEAM(getTeam()).AI_changeWarSuccess(pNewCity->getTeam(), (pNewCity->isCapital() ? 3 : 2)*GC.getWAR_SUCCESS_CITY_CAPTURING()/2);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

				PlayerTypes eNewOwner = GET_PLAYER(getOwnerINLINE()).pickConqueredCityOwner(*pNewCity);

				if (NO_PLAYER != eNewOwner)
				{
					GET_PLAYER(eNewOwner).acquireCity(pNewCity, true, false, true); // will delete the pointer
					pNewCity = NULL;
				}
			}
		}

		//update facing direction
		if(pOldPlot != NULL)
		{
			DirectionTypes newDirection = estimateDirection(pOldPlot, pNewPlot);
			if(newDirection != NO_DIRECTION)
				m_eFacingDirection = newDirection;
		}

		//update cargo mission animations
		/* original bts code
		if (isCargo())
		{
			if (eOldActivityType != ACTIVITY_MISSION)
			{
				getGroup()->setActivityType(eOldActivityType);
			}
		} */ // disabled by K-Mod (obsolete)

		setFortifyTurns(0);

		pNewPlot->changeAdjacentSight(getTeam(), visibilityRange(), true, this, true); // needs to be here so that the square is considered visible when we move into it...

		pNewPlot->addUnit(this, bUpdate && !hasCargo());

		pNewPlot->area()->changeUnitsPerPlayer(getOwnerINLINE(), 1);
		pNewPlot->area()->changePower(getOwnerINLINE(), m_pUnitInfo->getPowerValue());

		if (AI_getUnitAIType() != NO_UNITAI)
		{
			pNewPlot->area()->changeNumAIUnits(getOwnerINLINE(), AI_getUnitAIType(), 1);
		}

		if (isAnimal())
		{
			pNewPlot->area()->changeAnimalsPerPlayer(getOwnerINLINE(), 1);
		}

		if (pNewPlot->getTeam() != getTeam() && (pNewPlot->getTeam() == NO_TEAM || !GET_TEAM(pNewPlot->getTeam()).isVassal(getTeam())))
		{
			GET_PLAYER(getOwnerINLINE()).changeNumOutsideUnits(1);
		}

		if (shouldLoadOnMove(pNewPlot))
		{
			load();
		}

		if (!alwaysInvisible() && !m_pUnitInfo->isHiddenNationality()) // K-Mod (just this condition)
		{
			for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
			{
				if (GET_TEAM((TeamTypes)iI).isAlive())
				{
					if (!isInvisible(((TeamTypes)iI), false))
					{
						if (pNewPlot->isVisible((TeamTypes)iI, false))
						{
							GET_TEAM((TeamTypes)iI).meet(getTeam(), true);
						}
					}
				}
			}
		}

		pNewCity = pNewPlot->getPlotCity();

		if (pNewCity != NULL)
		{
			if (isMilitaryHappiness())
			{
				pNewCity->changeMilitaryHappinessUnits(1);
			}
		}

		pWorkingCity = pNewPlot->getWorkingCity();

		if (pWorkingCity != NULL)
		{
			if (canSiege(pWorkingCity->getTeam()))
			{
				pWorkingCity->verifyWorkingPlot(pWorkingCity->getCityPlotIndex(pNewPlot));
			}
		}

		/* original bts code
		if (pNewPlot->isWater())
		{
			for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
			{
				pLoopPlot = plotDirection(pNewPlot->getX_INLINE(), pNewPlot->getY_INLINE(), ((DirectionTypes)iI));

				if (pLoopPlot != NULL)
				{
					if (pLoopPlot->isWater())
					{
						pWorkingCity = pLoopPlot->getWorkingCity();

						if (pWorkingCity != NULL)
						{
							if (canSiege(pWorkingCity->getTeam()))
							{
								pWorkingCity->verifyWorkingPlot(pWorkingCity->getCityPlotIndex(pLoopPlot));
							}
						}
					}
				}
			}
		} */ // disabled by K-Mod. The game mechanics that this was meant to handle are no longer used. (Nothing to do with K-Mod.)

		if (pNewPlot->isActiveVisible(true))
		{
			pNewPlot->updateMinimapColor();
		}

		if (GC.IsGraphicsInitialized())
		{
			//override bShow if check plot visible
			if(bCheckPlotVisible && pNewPlot->isVisibleToWatchingHuman())
				bShow = true;

			if (bShow)
			{
				QueueMove(pNewPlot);
			}
			else
			{
				SetPosition(pNewPlot);
			}
		}

		if (pNewPlot == gDLL->getInterfaceIFace()->getSelectionPlot())
		{
			gDLL->getInterfaceIFace()->verifyPlotListColumn();

			gDLL->getInterfaceIFace()->setDirty(PlotListButtons_DIRTY_BIT, true);
		}
	}

	if (pOldPlot != NULL)
	{
		if (hasCargo())
		{
			std::vector<std::pair<PlayerTypes, int> > cargo_groups; // K-Mod. (player, group) pair.
			pUnitNode = pOldPlot->headUnitNode();

			while (pUnitNode != NULL)
			{
				pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = pOldPlot->nextUnitNode(pUnitNode);

				if (pLoopUnit->getTransportUnit() == this)
				{
					pLoopUnit->setXY(iX, iY, bGroup, false);
					cargo_groups.push_back(std::make_pair(pLoopUnit->getOwnerINLINE(), pLoopUnit->getGroupID())); // K-Mod
				}
			}
			// K-Mod
			// If the group of the cargo units we just moved includes units that are not transported
			// by this transport group, then we need to separate them.

			// first remove duplicate group numbers
			std::sort(cargo_groups.begin(), cargo_groups.end());
			cargo_groups.erase(std::unique(cargo_groups.begin(), cargo_groups.end()), cargo_groups.end());

			// now check the units in each group
			for (size_t i = 0; i < cargo_groups.size(); i++)
			{
				CvSelectionGroup* pCargoGroup = GET_PLAYER(cargo_groups[i].first).getSelectionGroup(cargo_groups[i].second);
				FAssert(pCargoGroup);
				pUnitNode = pCargoGroup->headUnitNode();
				ActivityTypes eOldActivityType = pCargoGroup->getActivityType();

				while (pUnitNode)
				{
					pLoopUnit = ::getUnit(pUnitNode->m_data);
					pUnitNode = pCargoGroup->nextUnitNode(pUnitNode);

					if (pLoopUnit->getTransportUnit() == NULL ||
						pLoopUnit->getTransportUnit()->getGroup() != getGroup())
					{
						pLoopUnit->joinGroup(NULL, true);
						if (eOldActivityType != ACTIVITY_MISSION)
						{
							pLoopUnit->getGroup()->setActivityType(eOldActivityType);
						}
					}
					// while we're here, update the air-circling animation for fighter-planes
					else if (eOldActivityType == ACTIVITY_INTERCEPT)
						pLoopUnit->airCircle(true);
				}
			}
			// K-Mod end
		}
	}

	if (bUpdate && hasCargo())
	{
		if (pOldPlot != NULL)
		{
			pOldPlot->updateCenterUnit();
			pOldPlot->setFlagDirty(true);
		}

		if (pNewPlot != NULL)
		{
			pNewPlot->updateCenterUnit();
			pNewPlot->setFlagDirty(true);
		}
	}

	FAssert(pOldPlot != pNewPlot);
	//GET_PLAYER(getOwnerINLINE()).updateGroupCycle(this);
	// K-Mod. Only update the group cycle here if we are placing this unit on the map for the first time.
	if (!pOldPlot)
		GET_PLAYER(getOwnerINLINE()).updateGroupCycle(getGroup());
	// K-Mod end

	setInfoBarDirty(true);

	if (IsSelected())
	{
		if (isFound())
		{
			gDLL->getInterfaceIFace()->setDirty(GlobeLayer_DIRTY_BIT, true);
			gDLL->getEngineIFace()->updateFoundingBorder();
		}

		gDLL->getInterfaceIFace()->setDirty(ColoredPlots_DIRTY_BIT, true);
	}

	//update glow
	if (pNewPlot != NULL)
	{
		gDLL->getEntityIFace()->updateEnemyGlow(getUnitEntity());
	}

	// report event to Python, along with some other key state
	CvEventReporter::getInstance().unitSetXY(pNewPlot, this);
}


bool CvUnit::at(int iX, int iY) const
{
	return((getX_INLINE() == iX) && (getY_INLINE() == iY));
}


bool CvUnit::atPlot(const CvPlot* pPlot) const
{
	return (plot() == pPlot);
}


CvPlot* CvUnit::plot() const
{
	return GC.getMapINLINE().plotSorenINLINE(getX_INLINE(), getY_INLINE());
}


int CvUnit::getArea() const
{
	return plot()->getArea();
}


CvArea* CvUnit::area() const
{
	return plot()->area();
}


bool CvUnit::onMap() const
{
	return (plot() != NULL);
}


int CvUnit::getLastMoveTurn() const
{
	return m_iLastMoveTurn;
}


void CvUnit::setLastMoveTurn(int iNewValue)
{
	m_iLastMoveTurn = iNewValue;
	FAssert(getLastMoveTurn() >= 0);
}


CvPlot* CvUnit::getReconPlot() const
{
	return GC.getMapINLINE().plotSorenINLINE(m_iReconX, m_iReconY);
}


void CvUnit::setReconPlot(CvPlot* pNewValue)
{
	CvPlot* pOldPlot;

	pOldPlot = getReconPlot();

	if (pOldPlot != pNewValue)
	{
		if (pOldPlot != NULL)
		{
			pOldPlot->changeAdjacentSight(getTeam(), GC.getDefineINT("RECON_VISIBILITY_RANGE"), false, this, true);
			pOldPlot->changeReconCount(-1); // changeAdjacentSight() tests for getReconCount()
		}

		if (pNewValue == NULL)
		{
			m_iReconX = INVALID_PLOT_COORD;
			m_iReconY = INVALID_PLOT_COORD;
		}
		else
		{
			m_iReconX = pNewValue->getX_INLINE();
			m_iReconY = pNewValue->getY_INLINE();

			pNewValue->changeReconCount(1); // changeAdjacentSight() tests for getReconCount()
			pNewValue->changeAdjacentSight(getTeam(), GC.getDefineINT("RECON_VISIBILITY_RANGE"), true, this, true);
			m_iLastReconTurn = GC.getGameINLINE().getGameTurn(); // advc.029
		}
	}
}


int CvUnit::getGameTurnCreated() const
{
	return m_iGameTurnCreated;
}


void CvUnit::setGameTurnCreated(int iNewValue)
{
	m_iGameTurnCreated = iNewValue;
	FAssert(getGameTurnCreated() >= 0);
}


int CvUnit::getDamage() const
{
	return m_iDamage;
}


void CvUnit::setDamage(int iNewValue, PlayerTypes ePlayer, bool bNotifyEntity)
{
	int iOldValue;

	iOldValue = getDamage();

	m_iDamage = range(iNewValue, 0, maxHitPoints());

	FAssertMsg(currHitPoints() >= 0, "currHitPoints() is expected to be non-negative (invalid Index)");

	if (iOldValue != getDamage())
	{
		if (GC.getGameINLINE().isFinalInitialized() && bNotifyEntity)
		{
			NotifyEntity(MISSION_DAMAGE);
		}

		setInfoBarDirty(true);

		if (IsSelected())
		{
			gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true);
		}

		if (plot() == gDLL->getInterfaceIFace()->getSelectionPlot())
		{
			gDLL->getInterfaceIFace()->setDirty(PlotListButtons_DIRTY_BIT, true);
		}
	}

	if (isDead())
	{
		kill(true, ePlayer);
	}
}


void CvUnit::changeDamage(int iChange, PlayerTypes ePlayer) 
{
	setDamage((getDamage() + iChange), ePlayer);
}


int CvUnit::getMoves() const
{
	return m_iMoves;
}


void CvUnit::setMoves(int iNewValue)
{
	CvPlot* pPlot;

	if (getMoves() != iNewValue)
	{
		pPlot = plot();

		m_iMoves = iNewValue;

		FAssert(getMoves() >= 0);

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			if (pPlot != NULL)
			{
				pPlot->setFlagDirty(true);
			}
		}

		if (IsSelected())
		{
			gDLL->getFAStarIFace()->ForceReset(&GC.getInterfacePathFinder());

			gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true);
		}

		if (pPlot == gDLL->getInterfaceIFace()->getSelectionPlot())
		{
			gDLL->getInterfaceIFace()->setDirty(PlotListButtons_DIRTY_BIT, true);
		}
	}
}


void CvUnit::changeMoves(int iChange)														
{
	setMoves(getMoves() + iChange);
}


void CvUnit::finishMoves()																			
{
	setMoves(maxMoves());
}


int CvUnit::getExperience() const
{
	return m_iExperience;
}

void CvUnit::setExperience(int iNewValue, int iMax)
{
	if ((getExperience() != iNewValue) && (getExperience() < ((iMax == -1) ? MAX_INT : iMax)))
	{
		m_iExperience = std::min(((iMax == -1) ? MAX_INT : iMax), iNewValue);
		FAssert(getExperience() >= 0);

		if (IsSelected())
		{
			gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true);
		}
	}
}

void CvUnit::changeExperience(int iChange, int iMax, bool bFromCombat, bool bInBorders, bool bUpdateGlobal)
{
	int iUnitExperience = iChange;

	if (bFromCombat)
	{
		CvPlayer& kPlayer = GET_PLAYER(getOwnerINLINE());

		int iCombatExperienceMod = 100 + kPlayer.getGreatGeneralRateModifier();

		if (bInBorders)
		{
			iCombatExperienceMod += kPlayer.getDomesticGreatGeneralRateModifier() + kPlayer.getExpInBorderModifier();
			/*  advc.003 (comment): ExpInBorderModifier is a currently unused
				Civic effect. */
			iUnitExperience += (iChange * kPlayer.getExpInBorderModifier()) / 100;
		}

		if (bUpdateGlobal)
		{
			kPlayer.changeCombatExperience((iChange * iCombatExperienceMod) / 100);
		}
		/*  advc.312: 50% from barbs. NB: XP from e.g. Barracks also sets
			bUpdateGlobal to false, but that isn't combat XP; this change
			really only applies to XP from barb combat. */
		else kPlayer.changeCombatExperience((iChange * iCombatExperienceMod) / 200);

		if (getExperiencePercent() != 0)
		{
			iUnitExperience *= std::max(0, 100 + getExperiencePercent());
			iUnitExperience /= 100;
		}
	}

	setExperience((getExperience() + iUnitExperience), iMax);
}


int CvUnit::getLevel() const
{
	return m_iLevel;
}

void CvUnit::setLevel(int iNewValue)
{
	if (getLevel() != iNewValue)
	{
		m_iLevel = iNewValue;
		FAssert(getLevel() >= 0);

		if (getLevel() > GET_PLAYER(getOwnerINLINE()).getHighestUnitLevel())
		{
			GET_PLAYER(getOwnerINLINE()).setHighestUnitLevel(getLevel());
		}

		if (IsSelected())
		{
			gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true);
		}
	}
}

void CvUnit::changeLevel(int iChange)
{
	setLevel(getLevel() + iChange);
}

int CvUnit::getCargo() const
{
	return m_iCargo;
}

void CvUnit::changeCargo(int iChange)																
{
	m_iCargo += iChange;
	FAssert(getCargo() >= 0);
}

void CvUnit::getCargoUnits(std::vector<CvUnit*>& aUnits) const
{
	aUnits.clear();

	if (hasCargo())
	{
		CvPlot* pPlot = plot();
		CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();
		while (pUnitNode != NULL)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = pPlot->nextUnitNode(pUnitNode);
			if (pLoopUnit->getTransportUnit() == this)
			{
				aUnits.push_back(pLoopUnit);
			}
		}
	}

	FAssert(getCargo() == aUnits.size());
}

CvPlot* CvUnit::getAttackPlot() const
{
	return GC.getMapINLINE().plotSorenINLINE(m_iAttackPlotX, m_iAttackPlotY);
}


void CvUnit::setAttackPlot(const CvPlot* pNewValue, bool bAirCombat)
{
	if (getAttackPlot() != pNewValue)
	{
		if (pNewValue != NULL)
		{
			m_iAttackPlotX = pNewValue->getX_INLINE();
			m_iAttackPlotY = pNewValue->getY_INLINE();
		}
		else
		{
			m_iAttackPlotX = INVALID_PLOT_COORD;
			m_iAttackPlotY = INVALID_PLOT_COORD;
		}
	}

	m_bAirCombat = bAirCombat;
}

bool CvUnit::isAirCombat() const
{
	return m_bAirCombat;
}

int CvUnit::getCombatTimer() const
{
	return m_iCombatTimer;
}

void CvUnit::setCombatTimer(int iNewValue)			
{
	m_iCombatTimer = iNewValue;
	FAssert(getCombatTimer() >= 0);
}

void CvUnit::changeCombatTimer(int iChange)			
{
	setCombatTimer(getCombatTimer() + iChange);
}

int CvUnit::getCombatFirstStrikes() const
{
	return m_iCombatFirstStrikes;
}

void CvUnit::setCombatFirstStrikes(int iNewValue)			
{
	m_iCombatFirstStrikes = iNewValue;
	FAssert(getCombatFirstStrikes() >= 0);
}

void CvUnit::changeCombatFirstStrikes(int iChange)			
{
	setCombatFirstStrikes(getCombatFirstStrikes() + iChange);
}

int CvUnit::getFortifyTurns() const
{
	return m_iFortifyTurns;
}

void CvUnit::setFortifyTurns(int iNewValue)
{
	iNewValue = range(iNewValue, 0, GC.getDefineINT("MAX_FORTIFY_TURNS"));

	if (iNewValue != getFortifyTurns())
	{
		m_iFortifyTurns = iNewValue;
		setInfoBarDirty(true);
	}
}

void CvUnit::changeFortifyTurns(int iChange)
{
	setFortifyTurns(getFortifyTurns() + iChange);
}

int CvUnit::getBlitzCount() const
{
	return m_iBlitzCount;
}

bool CvUnit::isBlitz() const
{
	return (getBlitzCount() > 0);
}

void CvUnit::changeBlitzCount(int iChange)			
{
	m_iBlitzCount += iChange;
	FAssert(getBlitzCount() >= 0);
}

int CvUnit::getAmphibCount() const
{
	return m_iAmphibCount;
}

bool CvUnit::isAmphib() const
{
	return (getAmphibCount() > 0);
}

void CvUnit::changeAmphibCount(int iChange)
{
	m_iAmphibCount += iChange;
	FAssert(getAmphibCount() >= 0);
}

int CvUnit::getRiverCount() const
{
	return m_iRiverCount;
}

bool CvUnit::isRiver() const
{
	return (getRiverCount() > 0);
}

void CvUnit::changeRiverCount(int iChange)
{
	m_iRiverCount += iChange;
	FAssert(getRiverCount() >= 0);
}

int CvUnit::getEnemyRouteCount() const
{
	return m_iEnemyRouteCount;
}

bool CvUnit::isEnemyRoute() const
{
	return (getEnemyRouteCount() > 0);
}

void CvUnit::changeEnemyRouteCount(int iChange)
{
	m_iEnemyRouteCount += iChange;
	FAssert(getEnemyRouteCount() >= 0);
}

int CvUnit::getAlwaysHealCount() const
{
	return m_iAlwaysHealCount;
}

bool CvUnit::isAlwaysHeal() const
{
	return (getAlwaysHealCount() > 0);
}

void CvUnit::changeAlwaysHealCount(int iChange)
{
	m_iAlwaysHealCount += iChange;
	FAssert(getAlwaysHealCount() >= 0);
}

int CvUnit::getHillsDoubleMoveCount() const
{
	return m_iHillsDoubleMoveCount;
}

bool CvUnit::isHillsDoubleMove() const
{
	return (getHillsDoubleMoveCount() > 0);
}

void CvUnit::changeHillsDoubleMoveCount(int iChange)
{
	m_iHillsDoubleMoveCount += iChange;
	FAssert(getHillsDoubleMoveCount() >= 0);
}

int CvUnit::getImmuneToFirstStrikesCount() const
{
	return m_iImmuneToFirstStrikesCount;
}

void CvUnit::changeImmuneToFirstStrikesCount(int iChange)
{
	m_iImmuneToFirstStrikesCount += iChange;
	FAssert(getImmuneToFirstStrikesCount() >= 0);
}


int CvUnit::getExtraVisibilityRange() const																	
{
	return m_iExtraVisibilityRange;
}

void CvUnit::changeExtraVisibilityRange(int iChange)
{
	if (iChange != 0)
	{
		plot()->changeAdjacentSight(getTeam(), visibilityRange(), false, this, true);

		m_iExtraVisibilityRange += iChange;
		FAssert(getExtraVisibilityRange() >= 0);

		plot()->changeAdjacentSight(getTeam(), visibilityRange(), true, this, true);
	}
}

int CvUnit::getExtraMoves() const												
{
	return m_iExtraMoves;
}


void CvUnit::changeExtraMoves(int iChange)			
{
	m_iExtraMoves += iChange;
	FAssert(getExtraMoves() >= 0);
}


int CvUnit::getExtraMoveDiscount() const
{
	return m_iExtraMoveDiscount;
}


void CvUnit::changeExtraMoveDiscount(int iChange)			
{
	m_iExtraMoveDiscount += iChange;
	FAssert(getExtraMoveDiscount() >= 0);
}

int CvUnit::getExtraAirRange() const												
{
	return m_iExtraAirRange;
}

void CvUnit::changeExtraAirRange(int iChange)			
{
	m_iExtraAirRange += iChange;
}

int CvUnit::getExtraIntercept() const												
{
	return m_iExtraIntercept;
}

void CvUnit::changeExtraIntercept(int iChange)			
{
	m_iExtraIntercept += iChange;
}

int CvUnit::getExtraEvasion() const												
{
	return m_iExtraEvasion;
}

void CvUnit::changeExtraEvasion(int iChange)			
{
	m_iExtraEvasion += iChange;
}

int CvUnit::getExtraFirstStrikes() const
{
	return m_iExtraFirstStrikes;
}

void CvUnit::changeExtraFirstStrikes(int iChange)			
{
	m_iExtraFirstStrikes += iChange;
	FAssert(getExtraFirstStrikes() >= 0);
}

int CvUnit::getExtraChanceFirstStrikes() const
{
	return m_iExtraChanceFirstStrikes;
}

void CvUnit::changeExtraChanceFirstStrikes(int iChange)			
{
	m_iExtraChanceFirstStrikes += iChange;
	FAssert(getExtraChanceFirstStrikes() >= 0);
}


int CvUnit::getExtraWithdrawal() const
{
	return m_iExtraWithdrawal;
}


void CvUnit::changeExtraWithdrawal(int iChange)															
{
	m_iExtraWithdrawal += iChange;
	//FAssert(getExtraWithdrawal() >= 0);
	FAssert(withdrawalProbability() >= 0); // K-Mod. (the 'extra' can be negative during sea-patrol battles.)
}

int CvUnit::getExtraCollateralDamage() const
{
	return m_iExtraCollateralDamage;
}

void CvUnit::changeExtraCollateralDamage(int iChange)													
{
	m_iExtraCollateralDamage += iChange;
	FAssert(getExtraCollateralDamage() >= 0);
}

int CvUnit::getExtraBombardRate() const
{
	return m_iExtraBombardRate;
}

void CvUnit::changeExtraBombardRate(int iChange)													
{
	m_iExtraBombardRate += iChange;
	FAssert(getExtraBombardRate() >= 0);
}

int CvUnit::getExtraEnemyHeal() const
{
	return m_iExtraEnemyHeal;
}

void CvUnit::changeExtraEnemyHeal(int iChange)						
{
	m_iExtraEnemyHeal += iChange;
	FAssert(getExtraEnemyHeal() >= 0);
}

int CvUnit::getExtraNeutralHeal() const
{
	return m_iExtraNeutralHeal;
}

void CvUnit::changeExtraNeutralHeal(int iChange)			
{
	m_iExtraNeutralHeal += iChange;
	FAssert(getExtraNeutralHeal() >= 0);
}

int CvUnit::getExtraFriendlyHeal() const
{
	return m_iExtraFriendlyHeal;
}


void CvUnit::changeExtraFriendlyHeal(int iChange)			
{
	m_iExtraFriendlyHeal += iChange;
	FAssert(getExtraFriendlyHeal() >= 0);
}

int CvUnit::getSameTileHeal() const
{
	return m_iSameTileHeal;
}

void CvUnit::changeSameTileHeal(int iChange)
{
	m_iSameTileHeal += iChange;
	FAssert(getSameTileHeal() >= 0);
}

int CvUnit::getAdjacentTileHeal() const
{
	return m_iAdjacentTileHeal;
}

void CvUnit::changeAdjacentTileHeal(int iChange)
{
	m_iAdjacentTileHeal += iChange;
	FAssert(getAdjacentTileHeal() >= 0);
}

int CvUnit::getExtraCombatPercent() const
{
	return m_iExtraCombatPercent;
}

void CvUnit::changeExtraCombatPercent(int iChange)
{
	if (iChange != 0)
	{
		m_iExtraCombatPercent += iChange;

		setInfoBarDirty(true);
	}
}

int CvUnit::getExtraCityAttackPercent() const
{
	return m_iExtraCityAttackPercent;
}

void CvUnit::changeExtraCityAttackPercent(int iChange)
{
	if (iChange != 0)
	{
		m_iExtraCityAttackPercent += iChange;

		setInfoBarDirty(true);
	}
}

int CvUnit::getExtraCityDefensePercent() const
{
	return m_iExtraCityDefensePercent;
}

void CvUnit::changeExtraCityDefensePercent(int iChange)
{
	if (iChange != 0)
	{
		m_iExtraCityDefensePercent += iChange;

		setInfoBarDirty(true);
	}
}

int CvUnit::getExtraHillsAttackPercent() const
{
	return m_iExtraHillsAttackPercent;
}

void CvUnit::changeExtraHillsAttackPercent(int iChange)
{
	if (iChange != 0)
	{
		m_iExtraHillsAttackPercent += iChange;

		setInfoBarDirty(true);
	}
}

int CvUnit::getExtraHillsDefensePercent() const
{
	return m_iExtraHillsDefensePercent;
}

void CvUnit::changeExtraHillsDefensePercent(int iChange)
{
	if (iChange != 0)
	{
		m_iExtraHillsDefensePercent += iChange;

		setInfoBarDirty(true);
	}
}

int CvUnit::getRevoltProtection() const
{
	return m_iRevoltProtection;
}

void CvUnit::changeRevoltProtection(int iChange)
{
	if (iChange != 0)
	{
		m_iRevoltProtection += iChange;

		setInfoBarDirty(true);
	}
}

int CvUnit::getCollateralDamageProtection() const
{
	return m_iCollateralDamageProtection;
}

void CvUnit::changeCollateralDamageProtection(int iChange)
{
	if (iChange != 0)
	{
		m_iCollateralDamageProtection += iChange;

		setInfoBarDirty(true);
	}
}

int CvUnit::getPillageChange() const
{
	return m_iPillageChange;
}

void CvUnit::changePillageChange(int iChange)
{
	if (iChange != 0)
	{
		m_iPillageChange += iChange;

		setInfoBarDirty(true);
	}
}

int CvUnit::getUpgradeDiscount() const
{
	return m_iUpgradeDiscount;
}

void CvUnit::changeUpgradeDiscount(int iChange)
{
	if (iChange != 0)
	{
		m_iUpgradeDiscount += iChange;

		setInfoBarDirty(true);
	}
}

int CvUnit::getExperiencePercent() const
{
	return m_iExperiencePercent;
}

void CvUnit::changeExperiencePercent(int iChange)
{
	if (iChange != 0)
	{
		m_iExperiencePercent += iChange;

		setInfoBarDirty(true);
	}
}

int CvUnit::getKamikazePercent() const
{
	return m_iKamikazePercent;
}

void CvUnit::changeKamikazePercent(int iChange)
{
	if (iChange != 0)
	{
		m_iKamikazePercent += iChange;

		setInfoBarDirty(true);
	}
}

DirectionTypes CvUnit::getFacingDirection(bool checkLineOfSightProperty) const
{
	if (checkLineOfSightProperty)
	{
		if (m_pUnitInfo->isLineOfSight())
		{
			return m_eFacingDirection; //only look in facing direction
		}
		else
		{
			return NO_DIRECTION; //look in all directions
		}
	}
	else
	{
		return m_eFacingDirection;
	}
}

void CvUnit::setFacingDirection(DirectionTypes eFacingDirection)
{
	if (eFacingDirection != m_eFacingDirection)
	{
		if (m_pUnitInfo->isLineOfSight())
		{
			//remove old fog
			plot()->changeAdjacentSight(getTeam(), visibilityRange(), false, this, true);

			//change direction
			m_eFacingDirection = eFacingDirection;

			//clear new fog
			plot()->changeAdjacentSight(getTeam(), visibilityRange(), true, this, true);

			gDLL->getInterfaceIFace()->setDirty(ColoredPlots_DIRTY_BIT, true);
		}
		else
		{
			m_eFacingDirection = eFacingDirection;
		}

		//update formation
		NotifyEntity(NO_MISSION);
	}
}

void CvUnit::rotateFacingDirectionClockwise()
{
	//change direction
	DirectionTypes eNewDirection = (DirectionTypes) ((m_eFacingDirection + 1) % NUM_DIRECTION_TYPES);
	setFacingDirection(eNewDirection);
}

void CvUnit::rotateFacingDirectionCounterClockwise()
{
	//change direction
	DirectionTypes eNewDirection = (DirectionTypes) ((m_eFacingDirection + NUM_DIRECTION_TYPES - 1) % NUM_DIRECTION_TYPES);
	setFacingDirection(eNewDirection);
}

int CvUnit::getImmobileTimer() const
{
	return m_iImmobileTimer;
}

void CvUnit::setImmobileTimer(int iNewValue)
{
	if (iNewValue != getImmobileTimer())
	{
		m_iImmobileTimer = iNewValue;

		setInfoBarDirty(true);
	}
}

void CvUnit::changeImmobileTimer(int iChange)
{
	if (iChange != 0)
	{
		setImmobileTimer(std::max(0, getImmobileTimer() + iChange));
	}
}

bool CvUnit::isMadeAttack() const
{
	return m_bMadeAttack;
}


void CvUnit::setMadeAttack(bool bNewValue)
{
	m_bMadeAttack = bNewValue;
}


bool CvUnit::isMadeInterception() const
{
	return m_bMadeInterception;
}


void CvUnit::setMadeInterception(bool bNewValue)
{
	m_bMadeInterception = bNewValue;
}

/*  <advc.002e> Lying to the EXE seems to be the only way to stop it from
	showing promotion glows after loading a savegame. isReadyForPromotion takes
	over the calls from the DLL and Python.
	The EXE also calls isPromotionReady when showing enemy moves. Should be OK
	to show the glow on those only once the human turn starts (in
	CvPlayer::doWarnings). */
bool CvUnit::isPromotionReady() const {
	return m_bPromotionReady && GC.getDefineINT("SHOW_PROMOTION_GLOW") > 0;
}

bool CvUnit::isReadyForPromotion() const {

	return m_bPromotionReady;
} // </advc.002e>

void CvUnit::setPromotionReady(bool bNewValue)
{
	if (isReadyForPromotion() != bNewValue) // advc.002e
	{
		m_bPromotionReady = bNewValue;

		if (m_bPromotionReady)
		{
			getGroup()->setAutomateType(NO_AUTOMATE);
			getGroup()->clearMissionQueue();
			getGroup()->setActivityType(ACTIVITY_AWAKE);
		}
		if(GC.getDefineINT("SHOW_PROMOTION_GLOW") > 0) // advc.002e:
			gDLL->getEntityIFace()->showPromotionGlow(getUnitEntity(), bNewValue);

		if (IsSelected())
		{
			gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
		}
	}
}


void CvUnit::testPromotionReady()
{
	setPromotionReady((getExperience() >= experienceNeeded()) && canAcquirePromotionAny());
}


bool CvUnit::isDelayedDeath() const
{
	return m_bDeathDelay;
}


void CvUnit::startDelayedDeath()			
{
	m_bDeathDelay = true;
}


// Returns true if killed...
bool CvUnit::doDelayedDeath()
{
	if (m_bDeathDelay && !isFighting())
	{
		kill(false);
		return true;
	}

	return false;
}


bool CvUnit::isCombatFocus() const
{
	return m_bCombatFocus;
}


bool CvUnit::isInfoBarDirty() const
{
	return m_bInfoBarDirty;
}


void CvUnit::setInfoBarDirty(bool bNewValue)
{
	m_bInfoBarDirty = bNewValue;
}

bool CvUnit::isBlockading() const
{
	return m_bBlockading;
}

void CvUnit::setBlockading(bool bNewValue)
{
	if (bNewValue != isBlockading())
	{
		m_bBlockading = bNewValue;

		updatePlunder(isBlockading() ? 1 : -1, true);
	}
}

void CvUnit::collectBlockadeGold()
{
	if (plot()->getTeam() == getTeam())
	{
		return;
	}

	int iBlockadeRange = GC.getDefineINT("SHIP_BLOCKADE_RANGE");

	for (int i = -iBlockadeRange; i <= iBlockadeRange; ++i)
	{
		for (int j = -iBlockadeRange; j <= iBlockadeRange; ++j)
		{
			CvPlot* pLoopPlot = ::plotXY(getX_INLINE(), getY_INLINE(), i, j);

			if (NULL != pLoopPlot && pLoopPlot->isRevealed(getTeam(), false))
			{
				CvCity* pCity = pLoopPlot->getPlotCity();

				if (NULL != pCity && !pCity->isPlundered() && isEnemy(pCity->getTeam()) && !atWar(pCity->getTeam(), getTeam()))
				{	// <advc.033>
					bool blockadedPlotFound = false;
					for(int k = 0; k < NUM_DIRECTION_TYPES; k++) {
						CvPlot* adj = plotDirection(pCity->getX_INLINE(),
								pCity->getY_INLINE(), ((DirectionTypes)k));
						if(adj != NULL && adj->getBlockadedCount(pCity->getTeam()) &&
								/*  Could still be a third party blockading; need to also
									check path: */
								GC.getMapINLINE().calculatePathDistance(plot(), adj) < iBlockadeRange)
							blockadedPlotFound = true;
					}
					if(!blockadedPlotFound)
						continue;
					//if (pCity->area() == area() || pCity->plot()->isAdjacentToArea(area()))
					{	// </advc.033>
						int iGold = pCity->calculateTradeProfit(pCity) * pCity->getTradeRoutes();
						if (iGold > 0)
						{
							pCity->setPlundered(true);
							GET_PLAYER(getOwnerINLINE()).changeGold(iGold);
							GET_PLAYER(pCity->getOwnerINLINE()).changeGold(-iGold);

							CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_TRADE_ROUTE_PLUNDERED", getNameKey(), pCity->getNameKey(), iGold);
							gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_BUILD_BANK", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), getX_INLINE(), getY_INLINE());

							szBuffer = gDLL->getText("TXT_KEY_MISC_TRADE_ROUTE_PLUNDER", getNameKey(), pCity->getNameKey(), iGold);
							gDLL->getInterfaceIFace()->addHumanMessage(pCity->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_BUILD_BANK", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pCity->getX_INLINE(), pCity->getY_INLINE());
						}
					}
				}
			}
		}
	}
}


PlayerTypes CvUnit::getOwner() const
{
	return getOwnerINLINE();
}

PlayerTypes CvUnit::getVisualOwner(TeamTypes eForTeam) const
{
	if (NO_TEAM == eForTeam)
	{
		eForTeam = GC.getGameINLINE().getActiveTeam();
	}

	if (getTeam() != eForTeam && eForTeam != BARBARIAN_TEAM)
	{
		if (m_pUnitInfo->isHiddenNationality())
		{
			if (!plot()->isCity(true, getTeam()))
			{
				return BARBARIAN_PLAYER;
			}
		}
	}

	return getOwnerINLINE();
}


PlayerTypes CvUnit::getCombatOwner(TeamTypes eForTeam, const CvPlot* pPlot) const
{
	if (eForTeam != NO_TEAM && getTeam() != eForTeam && eForTeam != BARBARIAN_TEAM)
	{
		if (isAlwaysHostile(pPlot))
		{
			return BARBARIAN_PLAYER;
		}
	}

	return getOwnerINLINE();
}

TeamTypes CvUnit::getTeam() const
{
	return GET_PLAYER(getOwnerINLINE()).getTeam();
}


PlayerTypes CvUnit::getCapturingPlayer() const
{
	return m_eCapturingPlayer;
}


void CvUnit::setCapturingPlayer(PlayerTypes eNewValue)
{
	m_eCapturingPlayer = eNewValue;
}


const UnitTypes CvUnit::getUnitType() const
{
	return m_eUnitType;
}

CvUnitInfo &CvUnit::getUnitInfo() const
{
	return *m_pUnitInfo;
}


UnitClassTypes CvUnit::getUnitClassType() const
{
	return (UnitClassTypes)m_pUnitInfo->getUnitClassType();
}

const UnitTypes CvUnit::getLeaderUnitType() const
{
	return m_eLeaderUnitType;
}

void CvUnit::setLeaderUnitType(UnitTypes leaderUnitType)
{
	if(m_eLeaderUnitType != leaderUnitType)
	{
		m_eLeaderUnitType = leaderUnitType;
		reloadEntity();
	}
}

CvUnit* CvUnit::getCombatUnit() const
{
	return getUnit(m_combatUnit);
}


void CvUnit::setCombatUnit(CvUnit* pCombatUnit, bool bAttacking)
{
	if (isCombatFocus())
	{
		gDLL->getInterfaceIFace()->setCombatFocus(false);
	}

	if (pCombatUnit != NULL)
	{
		if (bAttacking)
		{
			if (GC.getLogging())
			{
				if (gDLL->getChtLvl() > 0)
				{
					// Log info about this combat...
					char szOut[1024];
					sprintf( szOut, "*** KOMBAT!\n     ATTACKER: Player %d Unit %d (%S's %S), CombatStrength=%d\n     DEFENDER: Player %d Unit %d (%S's %S), CombatStrength=%d\n",
						getOwnerINLINE(), getID(), GET_PLAYER(getOwnerINLINE()).getName(), getName().GetCString(), currCombatStr(NULL, NULL),
						pCombatUnit->getOwnerINLINE(), pCombatUnit->getID(), GET_PLAYER(pCombatUnit->getOwnerINLINE()).getName(), pCombatUnit->getName().GetCString(), pCombatUnit->currCombatStr(pCombatUnit->plot(), this));
					gDLL->messageControlLog(szOut);
				}
			}

			/* original bts code
			if (getDomainType() == DOMAIN_LAND
				&& !m_pUnitInfo->isIgnoreBuildingDefense()
				&& pCombatUnit->plot()->getPlotCity() 
				&& pCombatUnit->plot()->getPlotCity()->getBuildingDefense() > 0 
				&& cityAttackModifier() >= GC.getDefineINT("MIN_CITY_ATTACK_MODIFIER_FOR_SIEGE_TOWER")) */
			if (showSeigeTower(pCombatUnit)) // K-Mod
			{
				CvDLLEntity::SetSiegeTower(true);
			}
		}

		FAssertMsg(getCombatUnit() == NULL, "Combat Unit is not expected to be assigned");
		FAssertMsg(!(plot()->isFighting()), "(plot()->isFighting()) did not return false as expected");
		m_bCombatFocus = (bAttacking && !(gDLL->getInterfaceIFace()->isFocusedWidget()) && ((getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) || ((pCombatUnit->getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) && !(GC.getGameINLINE().isMPOption(MPOPTION_SIMULTANEOUS_TURNS)))));
		m_combatUnit = pCombatUnit->getIDInfo();
		setCombatFirstStrikes((pCombatUnit->immuneToFirstStrikes()) ? 0 : (firstStrikes() + GC.getGameINLINE().getSorenRandNum(chanceFirstStrikes() + 1, "First Strike")));
	}
	else
	{
		if(getCombatUnit() != NULL)
		{
			FAssertMsg(getCombatUnit() != NULL, "getCombatUnit() is not expected to be equal with NULL");
			FAssertMsg(plot()->isFighting(), "plot()->isFighting is expected to be true");
			m_bCombatFocus = false;
			m_combatUnit.reset();
			setCombatFirstStrikes(0);

			if (IsSelected())
			{
				gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true);
			}

			if (plot() == gDLL->getInterfaceIFace()->getSelectionPlot())
			{
				gDLL->getInterfaceIFace()->setDirty(PlotListButtons_DIRTY_BIT, true);
			}

			CvDLLEntity::SetSiegeTower(false);
		}
	}

	setCombatTimer(0);
	setInfoBarDirty(true);

	if (isCombatFocus())
	{
		gDLL->getInterfaceIFace()->setCombatFocus(true);
	}
}

// K-Mod. Return true if the combat animation should include a seige tower
// (code copied from setCombatUnit, above)
bool CvUnit::showSeigeTower(CvUnit* pDefender) const
{
	return getDomainType() == DOMAIN_LAND
		&& !m_pUnitInfo->isIgnoreBuildingDefense()
		&& pDefender->plot()->getPlotCity()
		&& pDefender->plot()->getPlotCity()->getBuildingDefense() > 0
		&& cityAttackModifier() >= GC.getDefineINT("MIN_CITY_ATTACK_MODIFIER_FOR_SIEGE_TOWER");
}
// K-Mod end

CvUnit* CvUnit::getTransportUnit() const
{
	return getUnit(m_transportUnit);
}


bool CvUnit::isCargo() const
{
	return (getTransportUnit() != NULL);
}


void CvUnit::setTransportUnit(CvUnit* pTransportUnit)
{
	CvUnit* pOldTransportUnit;

	pOldTransportUnit = getTransportUnit();

	if (pOldTransportUnit != pTransportUnit)
	{
		if (pOldTransportUnit != NULL)
		{
			pOldTransportUnit->changeCargo(-1);
		}

		if (pTransportUnit != NULL)
		{
			FAssertMsg(pTransportUnit->cargoSpaceAvailable(getSpecialUnitType(), getDomainType()) > 0, "Cargo space is expected to be available");

			//joinGroup(NULL, true); // Because what if a group of 3 tries to get in a transport which can hold 2...

			// K-Mod
			if (getGroup()->getNumUnits() > 1) // we could use > cargoSpace, I suppose. But maybe some quirks of game mechanics rely on this group split.
				joinGroup(NULL, true);
			else
			{
				getGroup()->clearMissionQueue();
				if (IsSelected())
					gDLL->getInterfaceIFace()->removeFromSelectionList(this);
			}
			FAssert(getGroup()->headMissionQueueNode() == 0); // we don't want them jumping off the boat to complete some unfinished mission!
			// K-Mod end

			m_transportUnit = pTransportUnit->getIDInfo();

			if (getDomainType() != DOMAIN_AIR)
			{
				getGroup()->setActivityType(ACTIVITY_SLEEP);
			}

			if (GC.getGameINLINE().isFinalInitialized())
			{
				finishMoves();
			}

			pTransportUnit->changeCargo(1);
			pTransportUnit->getGroup()->setActivityType(ACTIVITY_AWAKE);
		}
		else
		{
			m_transportUnit.reset();

			if (getGroup()->getActivityType() != ACTIVITY_MISSION) // K-Mod. (the unit might be trying to walk somewhere.)
				getGroup()->setActivityType(ACTIVITY_AWAKE);
		}
	}
}


int CvUnit::getExtraDomainModifier(DomainTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiExtraDomainModifier[eIndex];
}


void CvUnit::changeExtraDomainModifier(DomainTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiExtraDomainModifier[eIndex] = (m_aiExtraDomainModifier[eIndex] + iChange);
}


const CvWString CvUnit::getName(uint uiForm) const
{
	CvWString szBuffer;

	if (m_szName.empty())
	{
		return m_pUnitInfo->getDescription(uiForm);
	}

	szBuffer.Format(L"%s (%s)", m_szName.GetCString(), m_pUnitInfo->getDescription(uiForm));

	return szBuffer;
}


const wchar* CvUnit::getNameKey() const
{
	if (m_szName.empty())
	{
		return m_pUnitInfo->getTextKeyWide();
	}
	else
	{
		return m_szName.GetCString();
	}
}


const CvWString& CvUnit::getNameNoDesc() const
{
	return m_szName;
}


void CvUnit::setName(CvWString szNewValue)
{
	gDLL->stripSpecialCharacters(szNewValue);

	m_szName = szNewValue;

	if (IsSelected())
	{
		gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true);
	}
}


std::string CvUnit::getScriptData() const
{
	return m_szScriptData;
}


void CvUnit::setScriptData(std::string szNewValue)
{
	m_szScriptData = szNewValue;
}


int CvUnit::getTerrainDoubleMoveCount(TerrainTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumTerrainInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiTerrainDoubleMoveCount[eIndex];
}


bool CvUnit::isTerrainDoubleMove(TerrainTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumTerrainInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return (getTerrainDoubleMoveCount(eIndex) > 0);
}


void CvUnit::changeTerrainDoubleMoveCount(TerrainTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumTerrainInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiTerrainDoubleMoveCount[eIndex] = (m_paiTerrainDoubleMoveCount[eIndex] + iChange);
	FAssert(getTerrainDoubleMoveCount(eIndex) >= 0);
}


int CvUnit::getFeatureDoubleMoveCount(FeatureTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumFeatureInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiFeatureDoubleMoveCount[eIndex];
}


bool CvUnit::isFeatureDoubleMove(FeatureTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumFeatureInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return (getFeatureDoubleMoveCount(eIndex) > 0);
}


void CvUnit::changeFeatureDoubleMoveCount(FeatureTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumFeatureInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiFeatureDoubleMoveCount[eIndex] = (m_paiFeatureDoubleMoveCount[eIndex] + iChange);
	FAssert(getFeatureDoubleMoveCount(eIndex) >= 0);
}


int CvUnit::getExtraTerrainAttackPercent(TerrainTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumTerrainInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiExtraTerrainAttackPercent[eIndex];
}


void CvUnit::changeExtraTerrainAttackPercent(TerrainTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumTerrainInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_paiExtraTerrainAttackPercent[eIndex] += iChange;

		setInfoBarDirty(true);
	}
}

int CvUnit::getExtraTerrainDefensePercent(TerrainTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumTerrainInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiExtraTerrainDefensePercent[eIndex];
}


void CvUnit::changeExtraTerrainDefensePercent(TerrainTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumTerrainInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_paiExtraTerrainDefensePercent[eIndex] += iChange;

		setInfoBarDirty(true);
	}
}

int CvUnit::getExtraFeatureAttackPercent(FeatureTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumFeatureInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiExtraFeatureAttackPercent[eIndex];
}


void CvUnit::changeExtraFeatureAttackPercent(FeatureTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumFeatureInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_paiExtraFeatureAttackPercent[eIndex] += iChange;

		setInfoBarDirty(true);
	}
}

int CvUnit::getExtraFeatureDefensePercent(FeatureTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumFeatureInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiExtraFeatureDefensePercent[eIndex];
}


void CvUnit::changeExtraFeatureDefensePercent(FeatureTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumFeatureInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_paiExtraFeatureDefensePercent[eIndex] += iChange;

		setInfoBarDirty(true);
	}
}

int CvUnit::getExtraUnitCombatModifier(UnitCombatTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUnitCombatInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiExtraUnitCombatModifier[eIndex];
}


void CvUnit::changeExtraUnitCombatModifier(UnitCombatTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUnitCombatInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiExtraUnitCombatModifier[eIndex] = (m_paiExtraUnitCombatModifier[eIndex] + iChange);
}


bool CvUnit::canAcquirePromotion(PromotionTypes ePromotion) const
{
	FAssertMsg(ePromotion >= 0, "ePromotion is expected to be non-negative (invalid Index)");
	FAssertMsg(ePromotion < GC.getNumPromotionInfos(), "ePromotion is expected to be within maximum bounds (invalid Index)");

	if (isHasPromotion(ePromotion))
	{
		return false;
	}

	if (GC.getPromotionInfo(ePromotion).getPrereqPromotion() != NO_PROMOTION)
	{
		if (!isHasPromotion((PromotionTypes)(GC.getPromotionInfo(ePromotion).getPrereqPromotion())))
		{
			return false;
		}
	}

/*
** K-Mod, 14/jan/11, karadoc
** third optional prereq
*/
	/* original bts code
	if (GC.getPromotionInfo(ePromotion).getPrereqOrPromotion1() != NO_PROMOTION)
	{
		if (!isHasPromotion((PromotionTypes)(GC.getPromotionInfo(ePromotion).getPrereqOrPromotion1())))
		{
			if ((GC.getPromotionInfo(ePromotion).getPrereqOrPromotion2() == NO_PROMOTION) || !isHasPromotion((PromotionTypes)(GC.getPromotionInfo(ePromotion).getPrereqOrPromotion2())))
			{
				return false;
			}
		}
	}*/

	PromotionTypes ePrereq1 = (PromotionTypes)GC.getPromotionInfo(ePromotion).getPrereqOrPromotion1();
	PromotionTypes ePrereq2 = (PromotionTypes)GC.getPromotionInfo(ePromotion).getPrereqOrPromotion2();
	PromotionTypes ePrereq3 = (PromotionTypes)GC.getPromotionInfo(ePromotion).getPrereqOrPromotion3();
	if (ePrereq1 != NO_PROMOTION || ePrereq2 != NO_PROMOTION || ePrereq3 != NO_PROMOTION)
	{
		bool bValid = false;

		if (ePrereq1 != NO_PROMOTION && isHasPromotion(ePrereq1))
			bValid = true;
		if (ePrereq2 != NO_PROMOTION && isHasPromotion(ePrereq2))
			bValid = true;
		if (ePrereq3 != NO_PROMOTION && isHasPromotion(ePrereq3))
			bValid = true;

		if (!bValid)
		{
			return false;
		}
	}
/*
** K-Mod end
*/

	if (GC.getPromotionInfo(ePromotion).getTechPrereq() != NO_TECH)
	{
		if (!(GET_TEAM(getTeam()).isHasTech((TechTypes)(GC.getPromotionInfo(ePromotion).getTechPrereq()))))
		{
			return false;
		}
	}

	if (GC.getPromotionInfo(ePromotion).getStateReligionPrereq() != NO_RELIGION)
	{
		if (GET_PLAYER(getOwnerINLINE()).getStateReligion() != GC.getPromotionInfo(ePromotion).getStateReligionPrereq())
		{
			return false;
		}
	}

	if (!isPromotionValid(ePromotion))
	{
		return false;
	}

	return true;
}

bool CvUnit::isPromotionValid(PromotionTypes ePromotion) const
{
	if (!::isPromotionValid(ePromotion, getUnitType(), true))
	{
		return false;
	}

	CvPromotionInfo& promotionInfo = GC.getPromotionInfo(ePromotion);

	if (promotionInfo.getWithdrawalChange() + m_pUnitInfo->getWithdrawalProbability() + getExtraWithdrawal() > GC.getDefineINT("MAX_WITHDRAWAL_PROBABILITY"))
	{
		return false;
	}

	if (promotionInfo.getInterceptChange() + maxInterceptionProbability() > GC.getDefineINT("MAX_INTERCEPTION_PROBABILITY"))
	{
		return false;
	}

	if (promotionInfo.getEvasionChange() + evasionProbability() > GC.getDefineINT("MAX_EVASION_PROBABILITY"))
	{
		return false;
	}

	// <advc.124>
	PromotionTypes prereq = (PromotionTypes)promotionInfo.getPrereqPromotion();
	// Unit extra moves can currently only stem from promotions.
	if(promotionInfo.getMovesChange() + m_pUnitInfo->getMoves() + getExtraMoves() > 4 &&
			GET_PLAYER(getOwner()).AI_unitImpassableCount(getUnitType()) > 0 &&
			// Allow Morale
			(prereq == NO_PROMOTION || !GC.getPromotionInfo(prereq).isLeader()))
		return false; // </advc.124>

	return true;
}


bool CvUnit::canAcquirePromotionAny() const
{
	int iI;

	for (iI = 0; iI < GC.getNumPromotionInfos(); iI++)
	{
		if (canAcquirePromotion((PromotionTypes)iI))
		{
			return true;
		}
	}

	return false;
}


bool CvUnit::isHasPromotion(PromotionTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumPromotionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_pabHasPromotion[eIndex];
}


void CvUnit::setHasPromotion(PromotionTypes eIndex, bool bNewValue)
{
	int iChange;
	int iI;

	if (isHasPromotion(eIndex) != bNewValue)
	{
		m_pabHasPromotion[eIndex] = bNewValue;

		iChange = ((isHasPromotion(eIndex)) ? 1 : -1);

		changeBlitzCount((GC.getPromotionInfo(eIndex).isBlitz()) ? iChange : 0);
		changeAmphibCount((GC.getPromotionInfo(eIndex).isAmphib()) ? iChange : 0);
		changeRiverCount((GC.getPromotionInfo(eIndex).isRiver()) ? iChange : 0);
		changeEnemyRouteCount((GC.getPromotionInfo(eIndex).isEnemyRoute()) ? iChange : 0);
		changeAlwaysHealCount((GC.getPromotionInfo(eIndex).isAlwaysHeal()) ? iChange : 0);
		changeHillsDoubleMoveCount((GC.getPromotionInfo(eIndex).isHillsDoubleMove()) ? iChange : 0);
		changeImmuneToFirstStrikesCount((GC.getPromotionInfo(eIndex).isImmuneToFirstStrikes()) ? iChange : 0);

		changeExtraVisibilityRange(GC.getPromotionInfo(eIndex).getVisibilityChange() * iChange);
		changeExtraMoves(GC.getPromotionInfo(eIndex).getMovesChange() * iChange);
		changeExtraMoveDiscount(GC.getPromotionInfo(eIndex).getMoveDiscountChange() * iChange);
		changeExtraAirRange(GC.getPromotionInfo(eIndex).getAirRangeChange() * iChange);
		changeExtraIntercept(GC.getPromotionInfo(eIndex).getInterceptChange() * iChange);
		changeExtraEvasion(GC.getPromotionInfo(eIndex).getEvasionChange() * iChange);
		changeExtraFirstStrikes(GC.getPromotionInfo(eIndex).getFirstStrikesChange() * iChange);
		changeExtraChanceFirstStrikes(GC.getPromotionInfo(eIndex).getChanceFirstStrikesChange() * iChange);
		changeExtraWithdrawal(GC.getPromotionInfo(eIndex).getWithdrawalChange() * iChange);
		changeExtraCollateralDamage(GC.getPromotionInfo(eIndex).getCollateralDamageChange() * iChange);
		changeExtraBombardRate(GC.getPromotionInfo(eIndex).getBombardRateChange() * iChange);
		changeExtraEnemyHeal(GC.getPromotionInfo(eIndex).getEnemyHealChange() * iChange);
		changeExtraNeutralHeal(GC.getPromotionInfo(eIndex).getNeutralHealChange() * iChange);
		changeExtraFriendlyHeal(GC.getPromotionInfo(eIndex).getFriendlyHealChange() * iChange);
		changeSameTileHeal(GC.getPromotionInfo(eIndex).getSameTileHealChange() * iChange);
		changeAdjacentTileHeal(GC.getPromotionInfo(eIndex).getAdjacentTileHealChange() * iChange);
		changeExtraCombatPercent(GC.getPromotionInfo(eIndex).getCombatPercent() * iChange);
		changeExtraCityAttackPercent(GC.getPromotionInfo(eIndex).getCityAttackPercent() * iChange);
		changeExtraCityDefensePercent(GC.getPromotionInfo(eIndex).getCityDefensePercent() * iChange);
		changeExtraHillsAttackPercent(GC.getPromotionInfo(eIndex).getHillsAttackPercent() * iChange);
		changeExtraHillsDefensePercent(GC.getPromotionInfo(eIndex).getHillsDefensePercent() * iChange);
		changeRevoltProtection(GC.getPromotionInfo(eIndex).getRevoltProtection() * iChange);
		changeCollateralDamageProtection(GC.getPromotionInfo(eIndex).getCollateralDamageProtection() * iChange);
		changePillageChange(GC.getPromotionInfo(eIndex).getPillageChange() * iChange);
		changeUpgradeDiscount(GC.getPromotionInfo(eIndex).getUpgradeDiscount() * iChange);
		changeExperiencePercent(GC.getPromotionInfo(eIndex).getExperiencePercent() * iChange);
		changeKamikazePercent((GC.getPromotionInfo(eIndex).getKamikazePercent()) * iChange);
		changeCargoSpace(GC.getPromotionInfo(eIndex).getCargoChange() * iChange);

		for (iI = 0; iI < GC.getNumTerrainInfos(); iI++)
		{
			changeExtraTerrainAttackPercent(((TerrainTypes)iI), (GC.getPromotionInfo(eIndex).getTerrainAttackPercent(iI) * iChange));
			changeExtraTerrainDefensePercent(((TerrainTypes)iI), (GC.getPromotionInfo(eIndex).getTerrainDefensePercent(iI) * iChange));
			changeTerrainDoubleMoveCount(((TerrainTypes)iI), ((GC.getPromotionInfo(eIndex).getTerrainDoubleMove(iI)) ? iChange : 0));
		}

		for (iI = 0; iI < GC.getNumFeatureInfos(); iI++)
		{
			changeExtraFeatureAttackPercent(((FeatureTypes)iI), (GC.getPromotionInfo(eIndex).getFeatureAttackPercent(iI) * iChange));
			changeExtraFeatureDefensePercent(((FeatureTypes)iI), (GC.getPromotionInfo(eIndex).getFeatureDefensePercent(iI) * iChange));
			changeFeatureDoubleMoveCount(((FeatureTypes)iI), ((GC.getPromotionInfo(eIndex).getFeatureDoubleMove(iI)) ? iChange : 0));
		}

		for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
		{
			changeExtraUnitCombatModifier(((UnitCombatTypes)iI), (GC.getPromotionInfo(eIndex).getUnitCombatModifierPercent(iI) * iChange));
		}

		for (iI = 0; iI < NUM_DOMAIN_TYPES; iI++)
		{
			changeExtraDomainModifier(((DomainTypes)iI), (GC.getPromotionInfo(eIndex).getDomainModifierPercent(iI) * iChange));
		}

		if (IsSelected())
		{
			gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true);
		}

		//update graphics
		gDLL->getEntityIFace()->updatePromotionLayers(getUnitEntity());
	}
}


int CvUnit::getSubUnitCount() const
{
	return m_pUnitInfo->getGroupSize();
}


int CvUnit::getSubUnitsAlive() const
{
	return getSubUnitsAlive( getDamage());
}


int CvUnit::getSubUnitsAlive(int iDamage) const
{
	if (iDamage >= maxHitPoints())
	{
		return 0;
	}
	else
	{
		return std::max(1, (((m_pUnitInfo->getGroupSize() * (maxHitPoints() - iDamage)) + (maxHitPoints() / ((m_pUnitInfo->getGroupSize() * 2) + 1))) / maxHitPoints()));
	}
}
// returns true if unit can initiate a war action with plot (possibly by declaring war)
bool CvUnit::potentialWarAction(const CvPlot* pPlot) const
{
	TeamTypes ePlotTeam = pPlot->getTeam();
	TeamTypes eUnitTeam = getTeam();
	
	if (ePlotTeam == NO_TEAM)
	{
		return false;
	}

	if (isEnemy(ePlotTeam, pPlot))
	{
		return true;
	}
	
	if (getGroup()->AI_isDeclareWar(pPlot) && GET_TEAM(eUnitTeam).AI_getWarPlan(ePlotTeam) != NO_WARPLAN)
	{
		return true;
	}

	return false;
}

void CvUnit::read(FDataStreamBase* pStream)
{
	// Init data before load
	reset();

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iID);
	pStream->Read(&m_iGroupID);
	pStream->Read(&m_iHotKeyNumber);
	pStream->Read(&m_iX);
	pStream->Read(&m_iY);
	pStream->Read(&m_iLastMoveTurn);
	pStream->Read(&m_iReconX);
	pStream->Read(&m_iReconY);
	// <advc.029>
	if(uiFlag < 4) {
		if(m_iReconX != INVALID_PLOT_COORD && m_iReconY != INVALID_PLOT_COORD)
			m_iLastReconTurn = GC.getGameINLINE().getGameTurn();
	}
	else pStream->Read(&m_iLastReconTurn); // </advc.029>
	pStream->Read(&m_iGameTurnCreated);
	pStream->Read(&m_iDamage);
	pStream->Read(&m_iMoves);
	pStream->Read(&m_iExperience);
	pStream->Read(&m_iLevel);
	pStream->Read(&m_iCargo);
	pStream->Read(&m_iCargoCapacity);
	pStream->Read(&m_iAttackPlotX);
	pStream->Read(&m_iAttackPlotY);
	pStream->Read(&m_iCombatTimer);
	pStream->Read(&m_iCombatFirstStrikes);
	if (uiFlag < 2)
	{
		int iCombatDamage;
		pStream->Read(&iCombatDamage);
	}
	pStream->Read(&m_iFortifyTurns);
	pStream->Read(&m_iBlitzCount);
	pStream->Read(&m_iAmphibCount);
	pStream->Read(&m_iRiverCount);
	pStream->Read(&m_iEnemyRouteCount);
	pStream->Read(&m_iAlwaysHealCount);
	pStream->Read(&m_iHillsDoubleMoveCount);
	pStream->Read(&m_iImmuneToFirstStrikesCount);
	pStream->Read(&m_iExtraVisibilityRange);
	pStream->Read(&m_iExtraMoves);
	pStream->Read(&m_iExtraMoveDiscount);
	pStream->Read(&m_iExtraAirRange);
	pStream->Read(&m_iExtraIntercept);
	pStream->Read(&m_iExtraEvasion);
	pStream->Read(&m_iExtraFirstStrikes);
	pStream->Read(&m_iExtraChanceFirstStrikes);
	pStream->Read(&m_iExtraWithdrawal);
	pStream->Read(&m_iExtraCollateralDamage);
	pStream->Read(&m_iExtraBombardRate);
	pStream->Read(&m_iExtraEnemyHeal);
	pStream->Read(&m_iExtraNeutralHeal);
	pStream->Read(&m_iExtraFriendlyHeal);
	pStream->Read(&m_iSameTileHeal);
	pStream->Read(&m_iAdjacentTileHeal);
	pStream->Read(&m_iExtraCombatPercent);
	pStream->Read(&m_iExtraCityAttackPercent);
	pStream->Read(&m_iExtraCityDefensePercent);
	pStream->Read(&m_iExtraHillsAttackPercent);
	pStream->Read(&m_iExtraHillsDefensePercent);
	pStream->Read(&m_iRevoltProtection);
	pStream->Read(&m_iCollateralDamageProtection);
	pStream->Read(&m_iPillageChange);
	pStream->Read(&m_iUpgradeDiscount);
	pStream->Read(&m_iExperiencePercent);
	pStream->Read(&m_iKamikazePercent);
	pStream->Read(&m_iBaseCombat);
	pStream->Read((int*)&m_eFacingDirection);
	pStream->Read(&m_iImmobileTimer);

	pStream->Read(&m_bMadeAttack);
	pStream->Read(&m_bMadeInterception);
	pStream->Read(&m_bPromotionReady);
	pStream->Read(&m_bDeathDelay);
	pStream->Read(&m_bCombatFocus);
	// m_bInfoBarDirty not saved...
	pStream->Read(&m_bBlockading);
	if (uiFlag > 0)
	{
		pStream->Read(&m_bAirCombat);
	}

	pStream->Read((int*)&m_eOwner);
	pStream->Read((int*)&m_eCapturingPlayer);
	pStream->Read((int*)&m_eUnitType);
	FAssert(NO_UNIT != m_eUnitType);
	m_pUnitInfo = (NO_UNIT != m_eUnitType) ? &GC.getUnitInfo(m_eUnitType) : NULL;
	pStream->Read((int*)&m_eLeaderUnitType);

	pStream->Read((int*)&m_combatUnit.eOwner);
	pStream->Read(&m_combatUnit.iID);
	pStream->Read((int*)&m_transportUnit.eOwner);
	pStream->Read(&m_transportUnit.iID);

	pStream->Read(NUM_DOMAIN_TYPES, m_aiExtraDomainModifier);

	pStream->ReadString(m_szName);
	pStream->ReadString(m_szScriptData);

	pStream->Read(GC.getNumPromotionInfos(), m_pabHasPromotion);

	pStream->Read(GC.getNumTerrainInfos(), m_paiTerrainDoubleMoveCount);
	pStream->Read(GC.getNumFeatureInfos(), m_paiFeatureDoubleMoveCount);
	pStream->Read(GC.getNumTerrainInfos(), m_paiExtraTerrainAttackPercent);
	pStream->Read(GC.getNumTerrainInfos(), m_paiExtraTerrainDefensePercent);
	pStream->Read(GC.getNumFeatureInfos(), m_paiExtraFeatureAttackPercent);
	pStream->Read(GC.getNumFeatureInfos(), m_paiExtraFeatureDefensePercent);
	pStream->Read(GC.getNumUnitCombatInfos(), m_paiExtraUnitCombatModifier);
}


void CvUnit::write(FDataStreamBase* pStream)
{
	uint uiFlag=4; // advc.029
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iID);
	pStream->Write(m_iGroupID);
	pStream->Write(m_iHotKeyNumber);
	pStream->Write(m_iX);
	pStream->Write(m_iY);
	pStream->Write(m_iLastMoveTurn);
	pStream->Write(m_iReconX);
	pStream->Write(m_iReconY);
	pStream->Write(m_iLastReconTurn); // advc.029
	pStream->Write(m_iGameTurnCreated);
	pStream->Write(m_iDamage);
	pStream->Write(m_iMoves);
	pStream->Write(m_iExperience);
	pStream->Write(m_iLevel);
	pStream->Write(m_iCargo);
	pStream->Write(m_iCargoCapacity);
	pStream->Write(m_iAttackPlotX);
	pStream->Write(m_iAttackPlotY);
	pStream->Write(m_iCombatTimer);
	pStream->Write(m_iCombatFirstStrikes);
	pStream->Write(m_iFortifyTurns);
	pStream->Write(m_iBlitzCount);
	pStream->Write(m_iAmphibCount);
	pStream->Write(m_iRiverCount);
	pStream->Write(m_iEnemyRouteCount);
	pStream->Write(m_iAlwaysHealCount);
	pStream->Write(m_iHillsDoubleMoveCount);
	pStream->Write(m_iImmuneToFirstStrikesCount);
	pStream->Write(m_iExtraVisibilityRange);
	pStream->Write(m_iExtraMoves);
	pStream->Write(m_iExtraMoveDiscount);
	pStream->Write(m_iExtraAirRange);
	pStream->Write(m_iExtraIntercept);
	pStream->Write(m_iExtraEvasion);
	pStream->Write(m_iExtraFirstStrikes);
	pStream->Write(m_iExtraChanceFirstStrikes);
	pStream->Write(m_iExtraWithdrawal);
	pStream->Write(m_iExtraCollateralDamage);
	pStream->Write(m_iExtraBombardRate);
	pStream->Write(m_iExtraEnemyHeal);
	pStream->Write(m_iExtraNeutralHeal);
	pStream->Write(m_iExtraFriendlyHeal);
	pStream->Write(m_iSameTileHeal);
	pStream->Write(m_iAdjacentTileHeal);
	pStream->Write(m_iExtraCombatPercent);
	pStream->Write(m_iExtraCityAttackPercent);
	pStream->Write(m_iExtraCityDefensePercent);
	pStream->Write(m_iExtraHillsAttackPercent);
	pStream->Write(m_iExtraHillsDefensePercent);
	pStream->Write(m_iRevoltProtection);
	pStream->Write(m_iCollateralDamageProtection);
	pStream->Write(m_iPillageChange);
	pStream->Write(m_iUpgradeDiscount);
	pStream->Write(m_iExperiencePercent);
	pStream->Write(m_iKamikazePercent);
	pStream->Write(m_iBaseCombat);
	pStream->Write(m_eFacingDirection);
	pStream->Write(m_iImmobileTimer);

	pStream->Write(m_bMadeAttack);
	pStream->Write(m_bMadeInterception);
	pStream->Write(m_bPromotionReady);
	pStream->Write(m_bDeathDelay);
	pStream->Write(m_bCombatFocus);
	// m_bInfoBarDirty not saved...
	pStream->Write(m_bBlockading);
	pStream->Write(m_bAirCombat);

	pStream->Write(m_eOwner);
	pStream->Write(m_eCapturingPlayer);
	pStream->Write(m_eUnitType);
	pStream->Write(m_eLeaderUnitType);

	pStream->Write(m_combatUnit.eOwner);
	pStream->Write(m_combatUnit.iID);
	pStream->Write(m_transportUnit.eOwner);
	pStream->Write(m_transportUnit.iID);

	pStream->Write(NUM_DOMAIN_TYPES, m_aiExtraDomainModifier);

	pStream->WriteString(m_szName);
	pStream->WriteString(m_szScriptData);

	pStream->Write(GC.getNumPromotionInfos(), m_pabHasPromotion);

	pStream->Write(GC.getNumTerrainInfos(), m_paiTerrainDoubleMoveCount);
	pStream->Write(GC.getNumFeatureInfos(), m_paiFeatureDoubleMoveCount);
	pStream->Write(GC.getNumTerrainInfos(), m_paiExtraTerrainAttackPercent);
	pStream->Write(GC.getNumTerrainInfos(), m_paiExtraTerrainDefensePercent);
	pStream->Write(GC.getNumFeatureInfos(), m_paiExtraFeatureAttackPercent);
	pStream->Write(GC.getNumFeatureInfos(), m_paiExtraFeatureDefensePercent);
	pStream->Write(GC.getNumUnitCombatInfos(), m_paiExtraUnitCombatModifier);
}

// Protected Functions...

bool CvUnit::canAdvance(const CvPlot* pPlot, int iThreshold) const
{
	FAssert(canFight());
	FAssert(!(isAnimal() && pPlot->isCity()));
	FAssert(getDomainType() != DOMAIN_AIR);
	FAssert(getDomainType() != DOMAIN_IMMOBILE);

	if (pPlot->getNumVisibleEnemyDefenders(this) > iThreshold)
	{
		return false;
	}

	if (isNoCapture())
	{
		if (pPlot->isEnemyCity(*this))
		{
			return false;
		}
	}

	return true;
}

// K-Mod, I've rewritten this function just to make it a bit easier to understand, a bit more efficient, and a bit more robust.
// For example, the original code used a std::map<CvUnit*, int>; if the random number in the map turned out to be the same, it could potentially have led to OOS.
// The actual game mechanics are only very slightly changed. (I've removed the targets cap of "visible units - 1". That seemed like a silly limitation.)
void CvUnit::collateralCombat(const CvPlot* pPlot, CvUnit* pSkipUnit)
{
	std::vector<std::pair<int, IDInfo> > targetUnits;

	int iCollateralStrength = (getDomainType() == DOMAIN_AIR ? airBaseCombatStr() : baseCombatStr()) * collateralDamage() / 100;

	if (iCollateralStrength == 0 && getExtraCollateralDamage() == 0)
		return;

	CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();

	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = pPlot->nextUnitNode(pUnitNode);

		if (pLoopUnit != pSkipUnit && isEnemy(pLoopUnit->getTeam(), pPlot) && pLoopUnit->canDefend() && !pLoopUnit->isInvisible(getTeam(), false))
		{
			// This value thing is a bit bork. It's directly from the original code...
			int iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "Collateral Damage"));
			iValue *= pLoopUnit->currHitPoints();

			targetUnits.push_back(std::make_pair(iValue, pLoopUnit->getIDInfo()));
		}
	}

	CvCity* pCity = NULL;
	if (getDomainType() == DOMAIN_AIR)
	{
		pCity = pPlot->getPlotCity();
	}

	int iPossibleTargets = std::min((int)targetUnits.size(), collateralDamageMaxUnits());
	std::partial_sort(targetUnits.begin(), targetUnits.begin() + iPossibleTargets, targetUnits.end(), std::greater<std::pair<int, IDInfo> >());

	int iDamageCount = 0;

	for (int i = 0; i < iPossibleTargets; i++)
	{
		CvUnit* pTargetUnit = ::getUnit(targetUnits[i].second);
		FAssert(pTargetUnit);

		if (NO_UNITCOMBAT == getUnitCombatType() || !pTargetUnit->getUnitInfo().getUnitCombatCollateralImmune(getUnitCombatType()))
		{
			int iTheirStrength = pTargetUnit->baseCombatStr();

			int iStrengthFactor = ((iCollateralStrength + iTheirStrength + 1) / 2);

			int iCollateralDamage = (GC.getDefineINT("COLLATERAL_COMBAT_DAMAGE") * (iCollateralStrength + iStrengthFactor)) / (iTheirStrength + iStrengthFactor);

			iCollateralDamage *= 100 + getExtraCollateralDamage();

			iCollateralDamage *= std::max(0, 100 - pTargetUnit->getCollateralDamageProtection());
			iCollateralDamage /= 100;

			if (pCity != NULL)
			{
				iCollateralDamage *= 100 + pCity->getAirModifier();
				iCollateralDamage /= 100;
			}

			iCollateralDamage = std::max(0, iCollateralDamage/100);

			int iMaxDamage = std::min(collateralDamageLimit(), (collateralDamageLimit() * (iCollateralStrength + iStrengthFactor)) / (iTheirStrength + iStrengthFactor));
			int iUnitDamage = std::max(pTargetUnit->getDamage(), std::min(pTargetUnit->getDamage() + iCollateralDamage, iMaxDamage));

			if (pTargetUnit->getDamage() != iUnitDamage)
			{
				pTargetUnit->setDamage(iUnitDamage, getOwnerINLINE());
				iDamageCount++;
			}
		}
	}

	if (iDamageCount > 0)
	{
		CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_SUFFER_COL_DMG", iDamageCount);
		gDLL->getInterfaceIFace()->addHumanMessage(pSkipUnit->getOwnerINLINE(), (pSkipUnit->getDomainType() != DOMAIN_AIR), GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_COLLATERAL", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pSkipUnit->getX_INLINE(), pSkipUnit->getY_INLINE(), true, true);

		szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_INFLICT_COL_DMG", getNameKey(), iDamageCount);
		gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_COLLATERAL", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pSkipUnit->getX_INLINE(), pSkipUnit->getY_INLINE());
	}
}


void CvUnit::flankingStrikeCombat(const CvPlot* pPlot, int iAttackerStrength, int iAttackerFirepower, int iDefenderOdds, int iDefenderDamage, CvUnit* pSkipUnit)
{
	if (pPlot->isCity(true, pSkipUnit->getTeam()))
	{
		return;
	}

	CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();

	std::vector< std::pair<CvUnit*, int> > listFlankedUnits;
	while (NULL != pUnitNode)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = pPlot->nextUnitNode(pUnitNode);

		if (pLoopUnit != pSkipUnit)
		{
			if (!pLoopUnit->isDead() && isEnemy(pLoopUnit->getTeam(), pPlot))
			{
				if (!(pLoopUnit->isInvisible(getTeam(), false)))
				{
					if (pLoopUnit->canDefend())
					{
						int iFlankingStrength = m_pUnitInfo->getFlankingStrikeUnitClass(pLoopUnit->getUnitClassType());

						if (iFlankingStrength > 0)
						{
							int iFlankedDefenderStrength;
							int iFlankedDefenderOdds;
							int iAttackerDamage;
							int iFlankedDefenderDamage;

							getDefenderCombatValues(*pLoopUnit, pPlot, iAttackerStrength, iAttackerFirepower, iFlankedDefenderOdds, iFlankedDefenderStrength, iAttackerDamage, iFlankedDefenderDamage);

							if (GC.getGameINLINE().getSorenRandNum(GC.getCOMBAT_DIE_SIDES(), "Flanking Combat") >= iDefenderOdds)
							{
								int iCollateralDamage = (iFlankingStrength * iDefenderDamage) / 100;
								int iUnitDamage = std::max(pLoopUnit->getDamage(), std::min(pLoopUnit->getDamage() + iCollateralDamage, collateralDamageLimit()));

								if (pLoopUnit->getDamage() != iUnitDamage)
								{
									listFlankedUnits.push_back(std::make_pair(pLoopUnit, iUnitDamage));
								}
							}
						}
					}
				}
			}
		}
	}

	int iNumUnitsHit = std::min((int)listFlankedUnits.size(), collateralDamageMaxUnits());

	for (int i = 0; i < iNumUnitsHit; ++i)
	{
		int iIndexHit = GC.getGameINLINE().getSorenRandNum(listFlankedUnits.size(), "Pick Flanked Unit");
		CvUnit* pUnit = listFlankedUnits[iIndexHit].first;
		int iDamage = listFlankedUnits[iIndexHit].second;
		pUnit->setDamage(iDamage, getOwnerINLINE());
		if (pUnit->isDead())
		{
			CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_KILLED_UNIT_BY_FLANKING", getNameKey(), pUnit->getNameKey(), pUnit->getVisualCivAdjective(getTeam()));
			gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, GC.getEraInfo(GC.getGameINLINE().getCurrentEra()).getAudioUnitVictoryScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pPlot->getX_INLINE(), pPlot->getY_INLINE());
			szBuffer = gDLL->getText("TXT_KEY_MISC_YOUR_UNIT_DIED_BY_FLANKING", pUnit->getNameKey(), getNameKey(), getVisualCivAdjective(pUnit->getTeam()));
			gDLL->getInterfaceIFace()->addHumanMessage(pUnit->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, GC.getEraInfo(GC.getGameINLINE().getCurrentEra()).getAudioUnitDefeatScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE());

			pUnit->kill(false); 
		}
		
		listFlankedUnits.erase(std::remove(listFlankedUnits.begin(), listFlankedUnits.end(), listFlankedUnits[iIndexHit]));
	}

	if (iNumUnitsHit > 0)
	{
		CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_DAMAGED_UNITS_BY_FLANKING", getNameKey(), iNumUnitsHit);
		gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, GC.getEraInfo(GC.getGameINLINE().getCurrentEra()).getAudioUnitVictoryScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pPlot->getX_INLINE(), pPlot->getY_INLINE());

		if (NULL != pSkipUnit)
		{
			szBuffer = gDLL->getText("TXT_KEY_MISC_YOUR_UNITS_DAMAGED_BY_FLANKING", getNameKey(), iNumUnitsHit);
			gDLL->getInterfaceIFace()->addHumanMessage(pSkipUnit->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, GC.getEraInfo(GC.getGameINLINE().getCurrentEra()).getAudioUnitDefeatScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE());
		}
	}
}


// Returns true if we were intercepted...
bool CvUnit::interceptTest(const CvPlot* pPlot)
{
	if (GC.getGameINLINE().getSorenRandNum(100, "Evasion Rand") >= evasionProbability())
	{
		CvUnit* pInterceptor = bestInterceptor(pPlot);

		if (pInterceptor != NULL)
		{
			if (GC.getGameINLINE().getSorenRandNum(100, "Intercept Rand (Air)") < pInterceptor->currInterceptionProbability())
			{
				fightInterceptor(pPlot, false);

				return true;
			}
		}
	}

	return false;
}


CvUnit* CvUnit::airStrikeTarget(const CvPlot* pPlot) const
{
	CvUnit* pDefender;

	pDefender = pPlot->getBestDefender(NO_PLAYER, getOwnerINLINE(), this, true);

	if (pDefender != NULL)
	{
		if (!pDefender->isDead())
		{
			if (pDefender->canDefend())
			{
				return pDefender;
			}
		}
	}

	return NULL;
}


bool CvUnit::canAirStrike(const CvPlot* pPlot) const
{
	if (getDomainType() != DOMAIN_AIR)
	{
		return false;
	}

	if (!canAirAttack())
	{
		return false;
	}

	if (pPlot == plot())
	{
		return false;
	}

	if (!pPlot->isVisible(getTeam(), false))
	{
		return false;
	}

	if (plotDistance(getX_INLINE(), getY_INLINE(), pPlot->getX_INLINE(), pPlot->getY_INLINE()) > airRange())
	{
		return false;
	}

	if (airStrikeTarget(pPlot) == NULL)
	{
		return false;
	}

	return true;
}


bool CvUnit::airStrike(CvPlot* pPlot)
{
	if (!canAirStrike(pPlot))
	{
		return false;
	}

	if (interceptTest(pPlot))
	{
		return false;
	}

	CvUnit* pDefender = airStrikeTarget(pPlot);

	FAssert(pDefender != NULL);
	FAssert(pDefender->canDefend());

	setReconPlot(pPlot);

	setMadeAttack(true);
	changeMoves(GC.getMOVE_DENOMINATOR());

	int iDamage = airCombatDamage(pDefender);

	int iUnitDamage = std::max(pDefender->getDamage(), std::min((pDefender->getDamage() + iDamage), airCombatLimit()));

	CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_ARE_ATTACKED_BY_AIR", pDefender->getNameKey(), getNameKey(),
		    // advc.004g:
			((iUnitDamage - pDefender->getDamage()) * 100) / pDefender->maxHitPoints());
	gDLL->getInterfaceIFace()->addHumanMessage(pDefender->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_AIR_ATTACK", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true, true);

	szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_ATTACK_BY_AIR", getNameKey(), pDefender->getNameKey(),
		    // advc.004g:
		    ((iUnitDamage - pDefender->getDamage()) * 100) / pDefender->maxHitPoints());
	gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_AIR_ATTACKED", MESSAGE_TYPE_INFO, pDefender->getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pPlot->getX_INLINE(), pPlot->getY_INLINE());

	collateralCombat(pPlot, pDefender);

	pDefender->setDamage(iUnitDamage, getOwnerINLINE());

	return true;
}

bool CvUnit::canRangeStrike() const
{
	if (getDomainType() == DOMAIN_AIR)
	{
		return false;
	}

	if (airRange() <= 0)
	{
		return false;
	}

	if (airBaseCombatStr() <= 0)
	{
		return false;
	}

	if (!canFight())
	{
		return false;
	}

	if (isMadeAttack() && !isBlitz())
	{
		return false;
	}

	if (!canMove() && getMoves() > 0)
	{
		return false;
	}

	return true;
}

bool CvUnit::canRangeStrikeAt(const CvPlot* pPlot, int iX, int iY) const
{
	if (!canRangeStrike())
	{
		return false;
	}

	CvPlot* pTargetPlot = GC.getMapINLINE().plotINLINE(iX, iY);

	if (NULL == pTargetPlot)
	{
		return false;
	}

	if (!pPlot->isVisible(getTeam(), false))
	{
		return false;
	}

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       05/10/10                             jdog5000         */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
	// Need to check target plot too
	if (!pTargetPlot->isVisible(getTeam(), false))
	{
		return false;
	}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/

	if (plotDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pTargetPlot->getX_INLINE(), pTargetPlot->getY_INLINE()) > airRange())
	{
		return false;
	}

	CvUnit* pDefender = airStrikeTarget(pTargetPlot);
	if (NULL == pDefender)
	{
		return false;
	}

	if (!pPlot->canSeePlot(pTargetPlot, getTeam(), airRange(), getFacingDirection(true)))
	{
		return false;
	}
	
	return true;
}


bool CvUnit::rangeStrike(int iX, int iY)
{
	CvUnit* pDefender;
	CvWString szBuffer;
	int iUnitDamage;
	int iDamage;

	CvPlot* pPlot = GC.getMapINLINE().plot(iX, iY);
	if (NULL == pPlot)
	{
		return false;
	}

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       05/10/10                             jdog5000         */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original bts code
	if (!canRangeStrikeAt(pPlot, iX, iY))
	{
		return false;
	}
*/
	if (!canRangeStrikeAt(plot(), iX, iY))
	{
		return false;
	}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/

	pDefender = airStrikeTarget(pPlot);

	FAssert(pDefender != NULL);
	FAssert(pDefender->canDefend());

	if (GC.getDefineINT("RANGED_ATTACKS_USE_MOVES") == 0)
	{
		setMadeAttack(true);
	}
	changeMoves(GC.getMOVE_DENOMINATOR());

	iDamage = rangeCombatDamage(pDefender);

	iUnitDamage = std::max(pDefender->getDamage(), std::min((pDefender->getDamage() + iDamage), airCombatLimit()));

	szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_ARE_ATTACKED_BY_AIR", pDefender->getNameKey(), getNameKey(),
		    // advc.004g:
		    ((iUnitDamage - pDefender->getDamage()) * 100) / pDefender->maxHitPoints());
	//red icon over attacking unit
	gDLL->getInterfaceIFace()->addHumanMessage(pDefender->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_COMBAT", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), this->getX_INLINE(), this->getY_INLINE(), true, true);
	//white icon over defending unit
	gDLL->getInterfaceIFace()->addHumanMessage(pDefender->getOwnerINLINE(), false, 0, L"", "AS2D_COMBAT", MESSAGE_TYPE_DISPLAY_ONLY, pDefender->getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"), pDefender->getX_INLINE(), pDefender->getY_INLINE(), true, true);

	szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_ATTACK_BY_AIR", getNameKey(), pDefender->getNameKey(),
			// advc.004g:
		    ((iUnitDamage - pDefender->getDamage()) * 100) / pDefender->maxHitPoints());
	gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pDefender->getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pPlot->getX_INLINE(), pPlot->getY_INLINE());

	collateralCombat(pPlot, pDefender);

	//set damage but don't update entity damage visibility
	pDefender->setDamage(iUnitDamage, getOwnerINLINE(), false);

	if (pPlot->isActiveVisible(false))
	{
		// Range strike entity mission
		CvMissionDefinition kDefiniton;
		kDefiniton.setMissionTime(GC.getMissionInfo(MISSION_RANGE_ATTACK).getTime() * gDLL->getSecsPerTurn());
		kDefiniton.setMissionType(MISSION_RANGE_ATTACK);
		kDefiniton.setPlot(pDefender->plot());
		kDefiniton.setUnit(BATTLE_UNIT_ATTACKER, this);
		kDefiniton.setUnit(BATTLE_UNIT_DEFENDER, pDefender);
		gDLL->getEntityIFace()->AddMission(&kDefiniton);

		//delay death
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       05/10/10                             jdog5000         */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original bts code
		pDefender->getGroup()->setMissionTimer(GC.getMissionInfo(MISSION_RANGE_ATTACK).getTime());
*/
		// mission timer is not used like this in any other part of code, so it might cause OOS
		// issues ... at worst I think unit dies before animation is complete, so no real
		// harm in commenting it out.
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
	}

	return true;
}

//------------------------------------------------------------------------------------------------
// FUNCTION:    CvUnit::planBattle
//! \brief      Determines in general how a battle will progress.
//!
//!				Note that the outcome of the battle is not determined here. This function plans
//!				how many sub-units die and in which 'rounds' of battle.
//! \param      kBattle The battle definition, which receives the battle plan.
//! \param		combat_log The order and amplitude of damage taken by the units (K-Mod)
//! \retval     The number of game turns that the battle should be given.
//------------------------------------------------------------------------------------------------

// Rewriten for K-Mod!
int CvUnit::planBattle(CvBattleDefinition& kBattle, const std::vector<int>& combat_log_argument) const
{
	const int BATTLE_TURNS_SETUP = 4;
	const int BATTLE_TURNS_ENDING = 4;
	const int BATTLE_TURNS_MELEE = 6;
	const int BATTLE_TURNS_RANGED = 6;
	const int BATTLE_TURN_RECHECK = 4;

	CvUnit* pAttackUnit = kBattle.getUnit(BATTLE_UNIT_ATTACKER);
	CvUnit* pDefenceUnit = kBattle.getUnit(BATTLE_UNIT_DEFENDER);

	int iAttackerDamage = kBattle.getDamage(BATTLE_UNIT_ATTACKER, BATTLE_TIME_BEGIN);
	int iDefenderDamage = kBattle.getDamage(BATTLE_UNIT_DEFENDER, BATTLE_TIME_BEGIN);

	int iAttackerUnits = pAttackUnit->getSubUnitsAlive(iAttackerDamage);
	int iDefenderUnits = pDefenceUnit->getSubUnitsAlive(iDefenderDamage);
	int iAttackerUnitsKilled = 0;
	int iDefenderUnitsKilled = 0;

	// some hackery to ensure that we don't have to deal with an empty combat log...
	const std::vector<int> dummy_log(1, 0);
	const std::vector<int>& combat_log = combat_log_argument.size() == 0 ? dummy_log : combat_log_argument;
	FAssert(combat_log.size() > 0);
	// now we can just use 'combat_log' without having to worry about it being empty.

	//
	kBattle.setNumRangedRounds(0);
	kBattle.setNumMeleeRounds(0);
	kBattle.setDamage(BATTLE_UNIT_ATTACKER, BATTLE_TIME_RANGED, iAttackerDamage);
	kBattle.setDamage(BATTLE_UNIT_DEFENDER, BATTLE_TIME_RANGED, iDefenderDamage);
	kBattle.clearBattleRounds();
	// kBattle.setNumRangedRounds(iFirstStrikeRounds); // originally max(iFirstStrikesDelta, iKills / iFirstStrikesDelta)
	// int iFirstStrikeRounds = kBattle.getFirstStrikes(BATTLE_UNIT_ATTACKER) + kBattle.getFirstStrikes(BATTLE_UNIT_DEFENDER);
	bool bRanged = true;

	const static int iStandardNumRounds = GC.getDefineINT("STANDARD_BATTLE_ANIMATION_ROUNDS", 6);

	int iTotalBattleRounds = (iStandardNumRounds * (int)combat_log.size() * GC.getCOMBAT_DAMAGE() + GC.getMAX_HIT_POINTS()) / (2*GC.getMAX_HIT_POINTS());

	// Reduce number of rounds if both units have groupSize == 1, because nothing much happens in those battles.
	if (pAttackUnit->getGroupSize() == 1 && pDefenceUnit->getGroupSize() == 1)
		iTotalBattleRounds = (2*iTotalBattleRounds+1)/3;

	// apparently, there is a hardcoded minimum of 2 rounds. (game will crash if there less than 2 rounds.)
	iTotalBattleRounds = range(iTotalBattleRounds, 2, 10);

	int iBattleRound = 0;
	for (int i = 0; i < (int)combat_log.size(); i++)
	{
		// The combat animator can't handle rounds where there are more deaths on one side than survivers on the other.
		// therefore, we must sometimes end the round early to avoid violating that rule.
		if (combat_log[i] > 0)
		{
			iDefenderDamage = std::min(GC.getMAX_HIT_POINTS(), iDefenderDamage + combat_log[i]);
			iDefenderUnitsKilled = std::min(iAttackerUnits - iAttackerUnitsKilled, iDefenderUnits - pDefenceUnit->getSubUnitsAlive(iDefenderDamage));
			bRanged = bRanged && pAttackUnit->isRanged();
			if (bRanged)
				kBattle.addDamage(BATTLE_UNIT_DEFENDER, BATTLE_TIME_RANGED, combat_log[i]); // I'm not sure if this is actually used for the animation...
		}
		else if (combat_log[i] < 0)
		{
			iAttackerDamage = std::min(GC.getMAX_HIT_POINTS(), iAttackerDamage - combat_log[i]);
			iAttackerUnitsKilled = std::min(iDefenderUnits - iDefenderUnitsKilled, iAttackerUnits - pAttackUnit->getSubUnitsAlive(iAttackerDamage));
			bRanged = bRanged && pDefenceUnit->isRanged();
			if (bRanged)
				kBattle.addDamage(BATTLE_UNIT_DEFENDER, BATTLE_TIME_RANGED, combat_log[i]);
		}

		// Sometimes we may need to end more than one round at a time...
		bool bNextRound = false;
		do
		{
			bNextRound = iBattleRound < (i+1) * iTotalBattleRounds / (int)combat_log.size();

			// force the round to end if we are already at the deaths limit for whomever will take the next damage.
			if (!bNextRound && (iDefenderUnitsKilled > 0 || iAttackerUnitsKilled > 0))
			{
				if (i+1 == combat_log.size() ||
					// kmodx: Added parentheses
					(iDefenderUnitsKilled >= iAttackerUnits - iAttackerUnitsKilled && combat_log[i] > 0) ||
					(iAttackerUnitsKilled >= iDefenderUnits - iDefenderUnitsKilled && combat_log[i] < 0))
				{
					bNextRound = true;
				}
			}

			if (bNextRound)
			{
				//bRanged = bRanged && (i < iFirstStrikeRounds || (pAttackUnit->isRanged() && pDefenceUnit->isRanged()));

				if (bRanged)
					kBattle.addNumRangedRounds(1);
				else
					kBattle.addNumMeleeRounds(1);

				//
				CvBattleRound kRound;

				kRound.setRangedRound(bRanged);
				kRound.setWaveSize(computeWaveSize(bRanged, iAttackerUnits, iDefenderUnits));

				iAttackerUnits -= iAttackerUnitsKilled;
				iDefenderUnits -= iDefenderUnitsKilled;

				kRound.setNumAlive(BATTLE_UNIT_ATTACKER, iAttackerUnits);
				kRound.setNumAlive(BATTLE_UNIT_DEFENDER, iDefenderUnits);
				kRound.setNumKilled(BATTLE_UNIT_ATTACKER, iAttackerUnitsKilled);
				kRound.setNumKilled(BATTLE_UNIT_DEFENDER, iDefenderUnitsKilled);
				//

				kBattle.addBattleRound(kRound);

				iBattleRound++;
				// there may be some spillover kills if the round was forced...
				iDefenderUnitsKilled = iDefenderUnits - pDefenceUnit->getSubUnitsAlive(iDefenderDamage);
				iAttackerUnitsKilled = iAttackerUnits - pAttackUnit->getSubUnitsAlive(iAttackerDamage);
				// and if there are, we should increase the total number of rounds so that we can fit them in.
				if (iDefenderUnitsKilled > 0 || iAttackerUnitsKilled > 0)
					iTotalBattleRounds++;
			}
		} while (bNextRound);
	}
	//FAssert(iBattleRound == iTotalBattleRounds);
	FAssert(iAttackerDamage == kBattle.getDamage(BATTLE_UNIT_ATTACKER, BATTLE_TIME_END));
	FAssert(iDefenderDamage == kBattle.getDamage(BATTLE_UNIT_DEFENDER, BATTLE_TIME_END));

	FAssert(kBattle.getNumBattleRounds() >= 2);
	FAssert(verifyRoundsValid(kBattle));

	int extraTime = 0;

	// extra time for seige towers and surrendering leaders.
	if ((pAttackUnit->getLeaderUnitType() != NO_UNIT && pAttackUnit->isDead()) ||
		(pDefenceUnit->getLeaderUnitType() != NO_UNIT && pDefenceUnit->isDead()) ||
		pAttackUnit->showSeigeTower(pDefenceUnit))
	{
		extraTime = BATTLE_TURNS_MELEE;
	}

	// K-Mod note: the original code used:
	//   gDLL->getEntityIFace()->GetSiegeTower(pAttackUnit->getUnitEntity()) || gDLL->getEntityIFace()->GetSiegeTower(pDefenceUnit->getUnitEntity())
	// I've changed that to use showSeigeTower, because GetSiegeTower does not work for the Pitboss host, and therefore can cause OOS errors.

	return BATTLE_TURNS_SETUP + BATTLE_TURNS_ENDING + kBattle.getNumMeleeRounds() * BATTLE_TURNS_MELEE + kBattle.getNumRangedRounds() * BATTLE_TURNS_MELEE + extraTime;
}

//------------------------------------------------------------------------------------------------
// FUNCTION:	CvBattleManager::computeDeadUnits
//! \brief		Computes the number of units dead, for either the ranged or melee portion of combat.
//! \param		kDefinition The battle definition.
//! \param		bRanged true if computing the number of units that die during the ranged portion of combat,
//!					false if computing the number of units that die during the melee portion of combat.
//! \param		iUnit The index of the unit to compute (BATTLE_UNIT_ATTACKER or BATTLE_UNIT_DEFENDER).
//! \retval		The number of units that should die for the given unit in the given portion of combat
//------------------------------------------------------------------------------------------------
int CvUnit::computeUnitsToDie( const CvBattleDefinition & kDefinition, bool bRanged, BattleUnitTypes iUnit ) const
{
	FAssertMsg( iUnit == BATTLE_UNIT_ATTACKER || iUnit == BATTLE_UNIT_DEFENDER, "Invalid unit index");

	BattleTimeTypes iBeginIndex = bRanged ? BATTLE_TIME_BEGIN : BATTLE_TIME_RANGED;
	BattleTimeTypes iEndIndex = bRanged ? BATTLE_TIME_RANGED : BATTLE_TIME_END;
	return kDefinition.getUnit(iUnit)->getSubUnitsAlive(kDefinition.getDamage(iUnit, iBeginIndex)) - 
		kDefinition.getUnit(iUnit)->getSubUnitsAlive( kDefinition.getDamage(iUnit, iEndIndex));
}

//------------------------------------------------------------------------------------------------
// FUNCTION:    CvUnit::verifyRoundsValid
//! \brief      Verifies that all rounds in the battle plan are valid
//! \param      vctBattlePlan The battle plan
//! \retval     true if the battle plan (seems) valid, false otherwise
//------------------------------------------------------------------------------------------------
bool CvUnit::verifyRoundsValid( const CvBattleDefinition & battleDefinition ) const
{
	for(int i=0;i<battleDefinition.getNumBattleRounds();i++)
	{
		if(!battleDefinition.getBattleRound(i).isValid())
			return false;
	}
	return true;
}

//------------------------------------------------------------------------------------------------
// FUNCTION:    CvUnit::increaseBattleRounds
//! \brief      Increases the number of rounds in the battle.
//! \param      kBattleDefinition The definition of the battle
//------------------------------------------------------------------------------------------------
void CvUnit::increaseBattleRounds( CvBattleDefinition & kBattleDefinition ) const
{
	if ( kBattleDefinition.getUnit(BATTLE_UNIT_ATTACKER)->isRanged() && kBattleDefinition.getUnit(BATTLE_UNIT_DEFENDER)->isRanged())
	{
		kBattleDefinition.addNumRangedRounds(1);
	}
	else
	{
		kBattleDefinition.addNumMeleeRounds(1);
	}
}

//------------------------------------------------------------------------------------------------
// FUNCTION:    CvUnit::computeWaveSize
//! \brief      Computes the wave size for the round.
//! \param      bRangedRound true if the round is a ranged round
//! \param		iAttackerMax The maximum number of attackers that can participate in a wave (alive)
//! \param		iDefenderMax The maximum number of Defenders that can participate in a wave (alive)
//! \retval     The desired wave size for the given parameters
//------------------------------------------------------------------------------------------------
int CvUnit::computeWaveSize( bool bRangedRound, int iAttackerMax, int iDefenderMax ) const
{
	FAssertMsg( getCombatUnit() != NULL, "You must be fighting somebody!" );
	int aiDesiredSize[BATTLE_UNIT_COUNT];
	if ( bRangedRound )
	{
		aiDesiredSize[BATTLE_UNIT_ATTACKER] = getUnitInfo().getRangedWaveSize();
		aiDesiredSize[BATTLE_UNIT_DEFENDER] = getCombatUnit()->getUnitInfo().getRangedWaveSize();
	}
	else
	{
		aiDesiredSize[BATTLE_UNIT_ATTACKER] = getUnitInfo().getMeleeWaveSize();
		aiDesiredSize[BATTLE_UNIT_DEFENDER] = getCombatUnit()->getUnitInfo().getMeleeWaveSize();
	}

	aiDesiredSize[BATTLE_UNIT_DEFENDER] = aiDesiredSize[BATTLE_UNIT_DEFENDER] <= 0 ? iDefenderMax : aiDesiredSize[BATTLE_UNIT_DEFENDER];
	aiDesiredSize[BATTLE_UNIT_ATTACKER] = aiDesiredSize[BATTLE_UNIT_ATTACKER] <= 0 ? iDefenderMax : aiDesiredSize[BATTLE_UNIT_ATTACKER];
	return std::min( std::min( aiDesiredSize[BATTLE_UNIT_ATTACKER], iAttackerMax ), std::min( aiDesiredSize[BATTLE_UNIT_DEFENDER],
		iDefenderMax) );
}

bool CvUnit::isTargetOf(const CvUnit& attacker) const
{
	CvUnitInfo& attackerInfo = attacker.getUnitInfo();
	CvUnitInfo& ourInfo = getUnitInfo();

	if (!plot()->isCity(true, getTeam()))
	{
		if (NO_UNITCLASS != getUnitClassType() && attackerInfo.getTargetUnitClass(getUnitClassType()))
		{
			return true;
		}

		if (NO_UNITCOMBAT != getUnitCombatType() && attackerInfo.getTargetUnitCombat(getUnitCombatType()))
		{
			return true;
		}
	}

	if (NO_UNITCLASS != attackerInfo.getUnitClassType() && ourInfo.getDefenderUnitClass(attackerInfo.getUnitClassType()))
	{
		return true;
	}

	if (NO_UNITCOMBAT != attackerInfo.getUnitCombatType() && ourInfo.getDefenderUnitCombat(attackerInfo.getUnitCombatType()))
	{
		return true;
	}

	return false;
}

bool CvUnit::isEnemy(TeamTypes eTeam, const CvPlot* pPlot) const
{
	if (NULL == pPlot)
	{
		pPlot = plot();
	}

	return (atWar(GET_PLAYER(getCombatOwner(eTeam, pPlot)).getTeam(), eTeam));
}

bool CvUnit::isPotentialEnemy(TeamTypes eTeam, const CvPlot* pPlot) const
{
	if (NULL == pPlot)
	{
		pPlot = plot();
	}

	return (::isPotentialEnemy(GET_PLAYER(getCombatOwner(eTeam, pPlot)).getTeam(), eTeam));
}

bool CvUnit::isSuicide() const
{
	return (m_pUnitInfo->isSuicide() || getKamikazePercent() != 0);
}

int CvUnit::getDropRange() const
{
	return (m_pUnitInfo->getDropRange());
}

void CvUnit::getDefenderCombatValues(CvUnit& kDefender, const CvPlot* pPlot, int iOurStrength, int iOurFirepower, int& iTheirOdds, int& iTheirStrength, int& iOurDamage, int& iTheirDamage, CombatDetails* pTheirDetails) const
{
	iTheirStrength = kDefender.currCombatStr(pPlot, this, pTheirDetails);
	int iTheirFirepower = kDefender.currFirepower(pPlot, this);

	FAssert((iOurStrength + iTheirStrength) > 0);
	FAssert((iOurFirepower + iTheirFirepower) > 0);

	iTheirOdds = ((GC.getCOMBAT_DIE_SIDES() * iTheirStrength) / (iOurStrength + iTheirStrength));

	if(!GC.getGameINLINE().isOption(GAMEOPTION_SPAH)) { // advc.250b
	 if (kDefender.isBarbarian())
	 {
		if (GET_PLAYER(getOwnerINLINE()).getWinsVsBarbs() < GC.getHandicapInfo(GET_PLAYER(getOwnerINLINE()).getHandicapType()).getFreeWinsVsBarbs())
		{
			iTheirOdds = std::min((10 * GC.getCOMBAT_DIE_SIDES()) / 100, iTheirOdds);
		}
	 }
	 if (isBarbarian())
	 {
		if (GET_PLAYER(kDefender.getOwnerINLINE()).getWinsVsBarbs() < GC.getHandicapInfo(GET_PLAYER(kDefender.getOwnerINLINE()).getHandicapType()).getFreeWinsVsBarbs())
		{
			iTheirOdds =  std::max((90 * GC.getCOMBAT_DIE_SIDES()) / 100, iTheirOdds);
		}
	 }
	} // advc.250b

	int iStrengthFactor = ((iOurFirepower + iTheirFirepower + 1) / 2);

	iOurDamage = std::max(1, ((GC.getCOMBAT_DAMAGE() * (iTheirFirepower + iStrengthFactor)) / (iOurFirepower + iStrengthFactor)));
	iTheirDamage = std::max(1, ((GC.getCOMBAT_DAMAGE() * (iOurFirepower + iStrengthFactor)) / (iTheirFirepower + iStrengthFactor)));
}

int CvUnit::getTriggerValue(EventTriggerTypes eTrigger, const CvPlot* pPlot, bool bCheckPlot) const
{
	CvEventTriggerInfo& kTrigger = GC.getEventTriggerInfo(eTrigger);
	if (kTrigger.getNumUnits() <= 0)
	{
		return MIN_INT;
	}

	if (isDead())
	{
		return MIN_INT;
	}

	if (!CvString(kTrigger.getPythonCanDoUnit()).empty())
	{
		long lResult;

		CyArgsList argsList;
		argsList.add(eTrigger);
		argsList.add(getOwnerINLINE());
		argsList.add(getID());

		gDLL->getPythonIFace()->callFunction(PYRandomEventModule, kTrigger.getPythonCanDoUnit(), argsList.makeFunctionArgs(), &lResult);

		if (0 == lResult)
		{
			return MIN_INT;
		}
	}

	if (kTrigger.getNumUnitsRequired() > 0)
	{
		bool bFoundValid = false;
		for (int i = 0; i < kTrigger.getNumUnitsRequired(); ++i)
		{
			if (getUnitClassType() == kTrigger.getUnitRequired(i))
			{
				bFoundValid = true;
				break;
			}
		}

		if (!bFoundValid)
		{
			return MIN_INT;
		}
	}

	if (bCheckPlot)
	{
		if (kTrigger.isUnitsOnPlot())
		{
			if (!plot()->canTrigger(eTrigger, getOwnerINLINE()))
			{
				return MIN_INT;
			}
		}
	}

	int iValue = 0;

	if (0 == getDamage() && kTrigger.getUnitDamagedWeight() > 0)
	{
		return MIN_INT;
	}

	iValue += getDamage() * kTrigger.getUnitDamagedWeight();

	iValue += getExperience() * kTrigger.getUnitExperienceWeight();

	if (NULL != pPlot)
	{
		iValue += plotDistance(getX_INLINE(), getY_INLINE(), pPlot->getX_INLINE(), pPlot->getY_INLINE()) * kTrigger.getUnitDistanceWeight();
	}

	return iValue;
}

bool CvUnit::canApplyEvent(EventTypes eEvent) const
{
	CvEventInfo& kEvent = GC.getEventInfo(eEvent);

	if (0 != kEvent.getUnitExperience())
	{
		if (!canAcquirePromotionAny())
		{
			return false;
		}
	}

	if (NO_PROMOTION != kEvent.getUnitPromotion())
	{
		if (!canAcquirePromotion((PromotionTypes)kEvent.getUnitPromotion()))
		{
			return false;
		}
	}

	if (kEvent.getUnitImmobileTurns() > 0)
	{
		if (!canAttack())
		{
			return false;
		}
	}

	return true;
}

void CvUnit::applyEvent(EventTypes eEvent)
{
	if (!canApplyEvent(eEvent))
	{
		return;
	}
	
	CvEventInfo& kEvent = GC.getEventInfo(eEvent);

	if (0 != kEvent.getUnitExperience())
	{
		setDamage(0);
		changeExperience(kEvent.getUnitExperience());
	}

	if (NO_PROMOTION != kEvent.getUnitPromotion())
	{
		setHasPromotion((PromotionTypes)kEvent.getUnitPromotion(), true);
	}

	if (kEvent.getUnitImmobileTurns() > 0)
	{
		changeImmobileTimer(kEvent.getUnitImmobileTurns());
		CvWString szText = gDLL->getText("TXT_KEY_EVENT_UNIT_IMMOBILE", getNameKey(), kEvent.getUnitImmobileTurns());
		gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szText, "AS2D_UNITGIFTED", MESSAGE_TYPE_INFO, getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_UNIT_TEXT"), getX_INLINE(), getY_INLINE(), true, true);
	}

	CvWString szNameKey(kEvent.getUnitNameKey());

	if (!szNameKey.empty())
	{
		setName(gDLL->getText(kEvent.getUnitNameKey()));
	}

	if (kEvent.isDisbandUnit())
	{
		kill(false);
	}
}

const CvArtInfoUnit* CvUnit::getArtInfo(int i, EraTypes eEra) const
{
	return m_pUnitInfo->getArtInfo(i, eEra, (UnitArtStyleTypes) GC.getCivilizationInfo(getCivilizationType()).getUnitArtStyleType());
}

const TCHAR* CvUnit::getButton() const
{
	const CvArtInfoUnit* pArtInfo = getArtInfo(0, GET_PLAYER(getOwnerINLINE()).getCurrentEra());

	if (NULL != pArtInfo)
	{
		return pArtInfo->getButton();
	}

	return m_pUnitInfo->getButton();
}

int CvUnit::getGroupSize() const
{
	return m_pUnitInfo->getGroupSize();
}

int CvUnit::getGroupDefinitions() const
{
	return m_pUnitInfo->getGroupDefinitions();
}

int CvUnit::getUnitGroupRequired(int i) const
{
	return m_pUnitInfo->getUnitGroupRequired(i);
}

bool CvUnit::isRenderAlways() const
{
	return m_pUnitInfo->isRenderAlways();
}

float CvUnit::getAnimationMaxSpeed() const
{
	return m_pUnitInfo->getUnitMaxSpeed();
}

float CvUnit::getAnimationPadTime() const
{
	return m_pUnitInfo->getUnitPadTime();
}

const char* CvUnit::getFormationType() const
{
	return m_pUnitInfo->getFormationType();
}

bool CvUnit::isMechUnit() const
{
	return m_pUnitInfo->isMechUnit();
}

bool CvUnit::isRenderBelowWater() const
{
	return m_pUnitInfo->isRenderBelowWater();
}

int CvUnit::getRenderPriority(UnitSubEntityTypes eUnitSubEntity, int iMeshGroupType, int UNIT_MAX_SUB_TYPES) const
{
	if (eUnitSubEntity == UNIT_SUB_ENTITY_SIEGE_TOWER)
	{
		return (getOwner() * (GC.getNumUnitInfos() + 2) * UNIT_MAX_SUB_TYPES) + iMeshGroupType;
	}
	else
	{
		return (getOwner() * (GC.getNumUnitInfos() + 2) * UNIT_MAX_SUB_TYPES) + m_eUnitType * UNIT_MAX_SUB_TYPES + iMeshGroupType;
	}
}

bool CvUnit::isAlwaysHostile(const CvPlot* pPlot) const
{
	if (!m_pUnitInfo->isAlwaysHostile())
	{
		return false;
	}

	if (NULL != pPlot && pPlot->isCity(true, getTeam()))
	{
		return false;
	}

	return true;
}

bool CvUnit::verifyStackValid()
{
	if (!alwaysInvisible())
	{
		if (plot()->isVisibleEnemyUnit(this))
		{
			return jumpToNearestValidPlot();
		}
	}

	return true;
}

// <advc.102> Really belongs in CvSelectionGroup, but can't add members there.
void CvUnit::setInitiallyVisible(bool b) {

	initiallyVisible = b;
}

bool CvUnit::isInitiallyVisible() const {

	return initiallyVisible;
}// </advc.102>

// Private Functions...

//check if quick combat
bool CvUnit::isCombatVisible(const CvUnit* pDefender) const
{
	bool bVisible = false;

	if (!m_pUnitInfo->isQuickCombat())
	{
		if (NULL == pDefender || !pDefender->getUnitInfo().isQuickCombat())
		{
			if (isHuman())
			{
				if (!GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_QUICK_ATTACK))
				{
					bVisible = true;
				}
			}
			else if (NULL != pDefender && pDefender->isHuman())
			{
				if (!GET_PLAYER(pDefender->getOwnerINLINE()).isOption(PLAYEROPTION_QUICK_DEFENSE))
				{
					bVisible = true;
				}
			}
		}
	}

	return bVisible;
}

// used by the executable for the red glow and plot indicators
bool CvUnit::shouldShowEnemyGlow(TeamTypes eForTeam) const
{
	if (isDelayedDeath())
	{
		return false;
	}

	if (getDomainType() == DOMAIN_AIR)
	{
		return false;
	}

	if (!canFight())
	{
		return false;
	}

	CvPlot* pPlot = plot();
	if (pPlot == NULL)
	{
		return false;
	}

	TeamTypes ePlotTeam = pPlot->getTeam();
	if (ePlotTeam != eForTeam)
	{
		return false;
	}

	if (!isEnemy(ePlotTeam))
	{
		return false;
	}

	return true;
}

bool CvUnit::shouldShowFoundBorders() const
{
	return isFound();
}


void CvUnit::cheat(bool bCtrl, bool bAlt, bool bShift)
{
	if (gDLL->getChtLvl() > 0)
	{
		if (bCtrl)
		{
			setPromotionReady(true);
		}
	}
}

float CvUnit::getHealthBarModifier() const
{
	return (GC.getDefineFLOAT("HEALTH_BAR_WIDTH") / (GC.getGameINLINE().getBestLandUnitCombat() * 2));
}

void CvUnit::getLayerAnimationPaths(std::vector<AnimationPathTypes>& aAnimationPaths) const
{
	for (int i=0; i < GC.getNumPromotionInfos(); ++i)
	{
		PromotionTypes ePromotion = (PromotionTypes) i;
		if (isHasPromotion(ePromotion))
		{
			AnimationPathTypes eAnimationPath = (AnimationPathTypes) GC.getPromotionInfo(ePromotion).getLayerAnimationPath();
			if(eAnimationPath != ANIMATIONPATH_NONE)
			{
				aAnimationPaths.push_back(eAnimationPath);
			}
		}
	}
}

int CvUnit::getSelectionSoundScript() const
{
	int iScriptId = getArtInfo(0, GET_PLAYER(getOwnerINLINE()).getCurrentEra())->getSelectionSoundScriptId();
	if (iScriptId == -1)
	{
		iScriptId = GC.getCivilizationInfo(getCivilizationType()).getSelectionSoundScriptId();
	}
	return iScriptId;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/21/10                                jdog5000      */
/*                                                                                              */
/* Lead From Behind                                                                             */
/************************************************************************************************/
// From Lead From Behind by UncutDragon

// Original isBetterDefenderThan call (without the extra parameter) - now just a pass-through
bool CvUnit::isBetterDefenderThan(const CvUnit* pDefender, const CvUnit* pAttacker) const
{
	return isBetterDefenderThan(pDefender, pAttacker, NULL);
}

// Modified version of best defender code (minus the initial boolean tests,
// which we still check in the original method)
bool CvUnit::LFBisBetterDefenderThan(const CvUnit* pDefender, const CvUnit* pAttacker, int* pBestDefenderRank) const
{
	// We adjust ranking based on ratio of our adjusted strength compared to twice that of attacker
	// Effect is if we're over twice as strong as attacker, we increase our ranking
	// (more likely to be picked as defender) - otherwise, we reduce our ranking (less likely)

	// Get our adjusted rankings based on combat odds
	int iOurRanking = LFBgetDefenderRank(pAttacker);
	int iTheirRanking = -1;
	if (pBestDefenderRank)
		iTheirRanking = (*pBestDefenderRank);
	if (iTheirRanking == -1)
		iTheirRanking = pDefender->LFBgetDefenderRank(pAttacker);

	// In case of equal value, fall back on unit cycle order
	// (K-Mod. _reversed_ unit cycle order, so that inexperienced units defend first.)
	if (iOurRanking == iTheirRanking)
	{
		if (isBeforeUnitCycle(this, pDefender))
			iTheirRanking++;
		else
			iTheirRanking--;
	}

	// Retain the basic rank (before value adjustment) for the best defender
	if (pBestDefenderRank)
		if (iOurRanking > iTheirRanking)
			(*pBestDefenderRank) = iOurRanking;

	return (iOurRanking > iTheirRanking);
}

// Get the (adjusted) odds of attacker winning to use in deciding best attacker
int CvUnit::LFBgetAttackerRank(const CvUnit* pDefender, int& iUnadjustedRank) const
{
	if (pDefender)
	{
		int iDefOdds = pDefender->LFBgetDefenderOdds(this);
		iUnadjustedRank = 1000 - iDefOdds;
		// If attacker has a chance to withdraw, factor that in as well
		if (withdrawalProbability() > 0)
			iUnadjustedRank += ((iDefOdds * withdrawalProbability()) / 100);
	}
	else
	{
		// No defender ... just use strength, but try to make it a number out of 1000
		iUnadjustedRank = currCombatStr(NULL, NULL) / 5;
	}

	return LFBgetValueAdjustedOdds(iUnadjustedRank, false);
}

// Get the (adjusted) odds of defender winning to use in deciding best defender
int CvUnit::LFBgetDefenderRank(const CvUnit* pAttacker) const
{
	int iRank = LFBgetDefenderOdds(pAttacker);
	// Don't adjust odds for value if attacker is limited in their damage (i.e: no risk of death)
	if ((pAttacker != NULL) && (maxHitPoints() <= pAttacker->combatLimit()))
		iRank = LFBgetValueAdjustedOdds(iRank, true);

	return iRank;
}

// Get the unadjusted odds of defender winning (used for both best defender and best attacker)
int CvUnit::LFBgetDefenderOdds(const CvUnit* pAttacker) const
{
	// Check if we have a valid attacker
	bool bUseAttacker = false;
	int iAttStrength = 0;
	if (pAttacker)
		iAttStrength = pAttacker->currCombatStr(NULL, NULL);
	if (iAttStrength > 0)
		bUseAttacker = true;

	int iDefense = 0;

	if (bUseAttacker && GC.getLFBUseCombatOdds())
	{
		// We start with straight combat odds
		iDefense = LFBgetDefenderCombatOdds(pAttacker);
	} else {
		// Lacking a real opponent (or if combat odds turned off) fall back on just using strength
		iDefense = currCombatStr(plot(), pAttacker);
		if (bUseAttacker)
		{
			// Similiar to the standard method, except I reduced the affect (cut it in half) handle attacker
			// and defender together (instead of applying one on top of the other) and substract the
			// attacker first strikes (instead of adding attacker first strikes when defender is immune)
			int iFirstStrikes = 0;

			if (!pAttacker->immuneToFirstStrikes())
				iFirstStrikes += (firstStrikes() * 2) + chanceFirstStrikes();
			if (!immuneToFirstStrikes())
				iFirstStrikes -= ((pAttacker->firstStrikes() * 2) + pAttacker->chanceFirstStrikes());

			if (iFirstStrikes != 0)
			{
				// With COMBAT_DAMAGE=20, this makes each first strike worth 8% (and each chance worth 4%)
				iDefense *= ((iFirstStrikes * GC.getCOMBAT_DAMAGE() / 5) + 100);
				iDefense /= 100;
			}

			// Make it a number out of 1000, taking attacker into consideration
			iDefense = (iDefense * 1000) / (iDefense + iAttStrength);			
		}
	}

	if (hasCargo())
	{
		// This part is taken directly from the standard method
		// Reduces value if a unit is carrying other units
		/* (disabled by K-Mod)
		int iAssetValue = std::max(1, getUnitInfo().getAssetValue());
		int iCargoAssetValue = 0;
		std::vector<CvUnit*> aCargoUnits;
		getCargoUnits(aCargoUnits);
		for (uint i = 0; i < aCargoUnits.size(); ++i)
		{
			iCargoAssetValue += aCargoUnits[i]->getUnitInfo().getAssetValue();
		}
		iDefense = iDefense * iAssetValue / std::max(1, iAssetValue + iCargoAssetValue); */

		// K-Mod. The above code does not achieve the goal, which is to protect cargo-carrying ships from being killed first.
		// The problem with the code is that when the odds of winning are very small, the artificial reduction is also very small;
		// on the other hand, when the odds of winning are very great, the artificial reduction is huge. This is the opposite of what we want!
		// We want to let the boats fight if they are going to win anyway, but give them protection if they would lose.

		// I've added my own version of the value adjustment to LFBgetValueAdjustedOdds.
	}

	return iDefense;
}

// Take the unadjusted odds and adjust them based on unit value
int CvUnit::LFBgetValueAdjustedOdds(int iOdds, bool bDefender) const
{
	// Adjust odds based on value
	int iValue = LFBgetRelativeValueRating();
	// K-Mod: if we are defending, then let those with defensive promotions fight!
	if (bDefender)
	{
		int iDef = LFGgetDefensiveValueAdjustment();
		// I'm a little bit concerned that if a unit gets a bunch of promotions with an xp discount,
		// that unit may end up being valued less than a completely inexperienced unit.
		// Thus the experienced unit may end up being sacrificed to protect the rookie.

		iValue -= iDef;
		iValue = std::max(0, iValue);
	}
	// K-Mod end
	long iAdjustment = -250;
	if (GC.getLFBUseSlidingScale())
		iAdjustment = (iOdds - 990);
	// Value Adjustment = (odds-990)*(value*num/denom)^2
	long iValueAdj = (long)(iValue * GC.getLFBAdjustNumerator());
	//iValueAdj *= iValueAdj;
	iValueAdj *= iAdjustment;
	//iValueAdj /= (long)(GC.getLFBAdjustDenominator() * GC.getLFBAdjustDenominator());
	iValueAdj /= (long)GC.getLFBAdjustDenominator();
	int iRank = iOdds + iValueAdj + 10000;
	// Note that the +10000 is just to try keeping it > 0 - doesn't really matter, other than that -1
	// would be interpreted later as not computed yet, which would cause us to compute it again each time

	// K-Mod. If this unit is a transport, reduce the value based on the risk of losing the cargo.
	// (This replaces the adjustment from LFBgetDefenderOdds. For more info, see the comments in that function.)
	if (hasCargo())
	{
		int iAssetValue = std::max(1, getUnitInfo().getAssetValue());
		int iCargoAssetValue = 0;
		std::vector<CvUnit*> aCargoUnits;
		getCargoUnits(aCargoUnits);
		for (uint i = 0; i < aCargoUnits.size(); ++i)
		{
			iCargoAssetValue += aCargoUnits[i]->getUnitInfo().getAssetValue();
		}
		iRank -= 2 * (1000 - iOdds) * iCargoAssetValue / std::max(1, iAssetValue + iCargoAssetValue);
	}
	// K-Mod end

	return iRank;
}

// Method to evaluate the value of a unit relative to another
int CvUnit::LFBgetRelativeValueRating() const
{
	int iValueRating = 0;

	// Check if led by a Great General
	if (GC.getLFBBasedOnGeneral() > 0)
		if (NO_UNIT != getLeaderUnitType())
			iValueRating += GC.getLFBBasedOnGeneral();

	// Assign experience value in tiers
	// (formula changed for K-Mod)
	if (GC.getLFBBasedOnExperience() > 0)
	{
		//int iTier = 10;
		int iTier = getLevel();
		//while (getExperience() >= iTier)
		while (getExperience() >= iTier*iTier+1)
		{
			//iValueRating += GC.getLFBBasedOnExperience();
			//iTier *= 2;
			iTier++;
		}
		//iValueRating += std::max(getLevel(), iTier) * GC.getLFBBasedOnExperience();
		iValueRating += iTier * GC.getLFBBasedOnExperience();
	}

	// Check if unit is limited in how many can exist
	if (GC.getLFBBasedOnLimited() > 0)
		if (isLimitedUnitClass(getUnitClassType()))
			iValueRating += GC.getLFBBasedOnLimited();

	// Check if unit has ability to heal
	if (GC.getLFBBasedOnHealer() > 0)
		if (getSameTileHeal() > 0)
			iValueRating += GC.getLFBBasedOnHealer();

	return iValueRating;
}

// K-Mod. unit value adjustment based on how many defensive promotions are active on this plot.
// (The purpose of this is to encourage experienced units to fight when their promotions are especially suited to the plot they are defending.)
int CvUnit::LFGgetDefensiveValueAdjustment() const
{
	int iValue = 0;

	for (int iI = 0; iI < GC.getNumPromotionInfos(); ++iI)
	{
		if (!isHasPromotion((PromotionTypes)iI))
			continue;

		CvPromotionInfo &kPromotion = GC.getPromotionInfo((PromotionTypes)iI);
		bool bDefensive = false;

		// Cities and hills
		if ((kPromotion.getCityDefensePercent() > 0 && plot()->isCity()) ||
			(kPromotion.getHillsDefensePercent() > 0 && plot()->isHills()))
		{
			bDefensive = true;
		}
		// Features
		if (!bDefensive)
		{
			for (int iJ = 0; iJ < GC.getNumFeatureInfos(); ++iJ)
			{
				if (kPromotion.getFeatureDefensePercent(iJ) > 0 && plot()->getFeatureType() == iJ)
				{
					bDefensive = true;
					break;
				}
			}
		}
		// Terrain
		if (!bDefensive)
		{
			for (int iJ = 0; iJ < GC.getNumTerrainInfos(); ++iJ)
			{
				if (kPromotion.getTerrainDefensePercent(iJ) > 0 && plot()->getTerrainType() == iJ)
				{
					bDefensive = true;
					break;
				}
			}
		}

		if (bDefensive)
		{
			iValue += GC.getLFBDefensiveAdjustment();
		}
	}

	return iValue;
}
// K-Mod end

int CvUnit::LFBgetDefenderCombatOdds(const CvUnit* pAttacker) const
{
	int iAttackerStrength;
	int iAttackerFirepower;
	int iDefenderStrength;
	int iDefenderFirepower;
	int iDefenderOdds;
	int iStrengthFactor;
	int iDamageToAttacker;
	int iDamageToDefender;
	int iNeededRoundsAttacker;
	int iNeededRoundsDefender;
	int iAttackerLowFS;
	int iAttackerHighFS;
	int iDefenderLowFS;
	int iDefenderHighFS;
	int iDefenderHitLimit;

	iAttackerStrength = pAttacker->currCombatStr(NULL, NULL);
	iAttackerFirepower = pAttacker->currFirepower(NULL, NULL);

	iDefenderStrength = currCombatStr(plot(), pAttacker);
	iDefenderFirepower = currFirepower(plot(), pAttacker);

	FAssert((iAttackerStrength + iDefenderStrength) > 0);
	FAssert((iAttackerFirepower + iDefenderFirepower) > 0);

	iDefenderOdds = ((GC.getCOMBAT_DIE_SIDES() * iDefenderStrength) / (iAttackerStrength + iDefenderStrength));
	iStrengthFactor = ((iAttackerFirepower + iDefenderFirepower + 1) / 2);

	// calculate damage done in one round
	//////

	iDamageToAttacker = std::max(1,((GC.getCOMBAT_DAMAGE() * (iDefenderFirepower + iStrengthFactor)) / (iAttackerFirepower + iStrengthFactor)));
	iDamageToDefender = std::max(1,((GC.getCOMBAT_DAMAGE() * (iAttackerFirepower + iStrengthFactor)) / (iDefenderFirepower + iStrengthFactor)));

	// calculate needed rounds.
	// Needed rounds = round_up(health/damage)
	//////

	iDefenderHitLimit = maxHitPoints() - pAttacker->combatLimit();

	iNeededRoundsAttacker = (std::max(0, currHitPoints() - iDefenderHitLimit) + iDamageToDefender - 1 ) / iDamageToDefender;
	iNeededRoundsDefender = (pAttacker->currHitPoints() + iDamageToAttacker - 1 ) / iDamageToAttacker;

	// calculate possible first strikes distribution.
	// We can't use the getCombatFirstStrikes() function (only one result,
	// no distribution), so we need to mimic it.
	//////

	iAttackerLowFS = (immuneToFirstStrikes()) ? 0 : pAttacker->firstStrikes();
	iAttackerHighFS = (immuneToFirstStrikes()) ? 0 : (pAttacker->firstStrikes() + pAttacker->chanceFirstStrikes());

	iDefenderLowFS = (pAttacker->immuneToFirstStrikes()) ? 0 : firstStrikes();
	iDefenderHighFS = (pAttacker->immuneToFirstStrikes()) ? 0 : (firstStrikes() + chanceFirstStrikes());

	return LFBgetCombatOdds(iDefenderLowFS, iDefenderHighFS, iAttackerLowFS, iAttackerHighFS, iNeededRoundsDefender, iNeededRoundsAttacker, iDefenderOdds);
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

