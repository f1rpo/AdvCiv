// plot.cpp

#include "CvGameCoreDLL.h"
#include "CvPlot.h"
#include "CvAI.h"
#include "CvCity.h"
#include "CvMap.h"
#include "CvArea.h"
#include "CvInfo_City.h"
#include "CvInfo_Terrain.h"
#include "CvInfo_GameOption.h"
#include "CvInfo_Civics.h" // for calculateImprovementYieldChange with bOptimal=true
#include "FAStarNode.h" // BETTER_BTS_AI_MOD, General AI, 11/30/08, jdog5000
#include "CvDLLSymbolIFaceBase.h"
#include "CvDLLPlotBuilderIFaceBase.h"
#include "CvDLLFlagEntityIFaceBase.h"

#define STANDARD_MINIMAP_ALPHA		(0.75f) // advc.002a: was 0.6
bool CvPlot::m_bAllFog = false; // advc.706
int CvPlot::iMaxVisibilityRangeCache; // advc.003h
#define NO_BUILD_IN_PROGRESS (-2) // advc.011


CvPlot::CvPlot() // advc: Merged with the deleted reset function
{
	// BETTER_BTS_AI_MOD, Efficiency (plot danger cache), 08/21/09, jdog5000: START
	m_iActivePlayerSafeRangeCache = -1;
	// BETTER_BTS_AI_MOD: END

	m_pFeatureSymbol = NULL;
	m_pPlotBuilder = NULL;
	m_pRouteSymbol = NULL;
	m_pRiverSymbol = NULL;
	m_pFlagSymbol = NULL;
	m_pFlagSymbolOffset = NULL;
	m_pCenterUnit = NULL;
	m_szScriptData = NULL;

	m_iX = 0;
	m_iY = 0;
	m_iArea = FFreeList::INVALID_INDEX;
	m_pPlotArea = NULL;
	m_iFeatureVariety = 0;
	m_iOwnershipDuration = 0;
	m_iImprovementDuration = 0;
	m_iUpgradeProgress = 0;
	m_iForceUnownedTimer = 0;
	m_iTurnsBuildsInterrupted = NO_BUILD_IN_PROGRESS; // advc.011
	m_iCityRadiusCount = 0;
	m_iRiverID = -1;
	m_iMinOriginalStartDist = -1;
	m_iReconCount = 0;
	m_iRiverCrossingCount = 0;
	m_iLatitude = -1; // advc.tsl
	m_iTotalCulture = 0; // advc.opt

	m_bStartingPlot = false;
	m_bNOfRiver = false;
	m_bWOfRiver = false;
	m_bIrrigated = false;
	m_bPotentialCityWork = false;
	m_bShowCitySymbols = false;
	m_bFlagDirty = false;
	m_bPlotLayoutDirty = false;
	m_bLayoutStateWorked = false;
	m_eSecondOwner = NO_PLAYER; // advc.035
	m_eOwner = NO_PLAYER;
	m_eTeam = NO_TEAM; // advc.opt
	m_ePlotType = PLOT_OCEAN;
	m_eTerrainType = NO_TERRAIN;
	m_eFeatureType = NO_FEATURE;
	m_eBonusType = NO_BONUS;
	m_eImprovementType = NO_IMPROVEMENT;
	m_eRouteType = NO_ROUTE;
	m_eRiverNSDirection = NO_CARDINALDIRECTION;
	m_eRiverWEDirection = NO_CARDINALDIRECTION;

	m_plotCity.reset();
	m_workingCity.reset();
	m_workingCityOverride.reset();
}


CvPlot::~CvPlot() // advc: Merged with the deleted uninit function
{
	SAFE_DELETE_ARRAY(m_szScriptData);

	gDLL->getFeatureIFace()->destroy(m_pFeatureSymbol);
	if(m_pPlotBuilder)
		gDLL->getPlotBuilderIFace()->destroy(m_pPlotBuilder);
	gDLL->getRouteIFace()->destroy(m_pRouteSymbol);
	gDLL->getRiverIFace()->destroy(m_pRiverSymbol);
	gDLL->getFlagEntityIFace()->destroy(m_pFlagSymbol);
	gDLL->getFlagEntityIFace()->destroy(m_pFlagSymbolOffset);
	m_pCenterUnit = NULL;

	deleteAllSymbols();
	m_units.clear();
}


void CvPlot::init(int iX, int iY)
{
	m_iX = iX;
	m_iY = iY;
	m_iLatitude = calculateLatitude(); // advc.tsl
}


void CvPlot::setupGraphical()
{
	PROFILE_FUNC();

	if (!GC.IsGraphicsInitialized())
		return;

	updateSymbols();
	updateFeatureSymbol();
	updateRiverSymbol();
	updateMinimapColor();

	updateVisibility();
	updateCenterUnit(); // K-Mod (This is required now that CvMap::updateCenterUnit doesn't always update the whole map.)
}

void CvPlot::updateGraphicEra()
{
	if(m_pRouteSymbol != NULL)
		gDLL->getRouteIFace()->updateGraphicEra(m_pRouteSymbol);

	if(m_pFlagSymbol != NULL)
		gDLL->getFlagEntityIFace()->updateGraphicEra(m_pFlagSymbol);
}

void CvPlot::erase()  // advc: some style changes
{
	CLinkList<IDInfo> oldUnits;
	{
		for (CLLNode<IDInfo> const* pUnitNode = headUnitNode(); pUnitNode != NULL;
				pUnitNode = nextUnitNode(pUnitNode))
			oldUnits.insertAtEnd(pUnitNode->m_data);
	}
	// kill units
	CLLNode<IDInfo>* pUnitNode = oldUnits.head();
	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = oldUnits.next(pUnitNode);
		if (pLoopUnit == NULL)
		{
			FAssertMsg(pLoopUnit != NULL, "Can this happen?"); // advc
			continue;
		}
		pLoopUnit->kill(false);
	}

	CvCity* pCity = getPlotCity();
	if (pCity != NULL)
		pCity->kill(false);

	setBonusType(NO_BONUS);
	setImprovementType(NO_IMPROVEMENT);
	setRouteType(NO_ROUTE, false);
	setFeatureType(NO_FEATURE);

	// disable rivers
	setNOfRiver(false, NO_CARDINALDIRECTION);
	setWOfRiver(false, NO_CARDINALDIRECTION);
	setRiverID(-1);
}


float CvPlot::getPointX() const
{
	return GC.getMap().plotXToPointX(getX());
}


float CvPlot::getPointY() const
{
	return GC.getMap().plotYToPointY(getY());
}


NiPoint3 CvPlot::getPoint() const
{
	NiPoint3 pt3Point;

	pt3Point.x = getPointX();
	pt3Point.y = getPointY();
	pt3Point.z = gDLL->getEngineIFace()->GetHeightmapZ(pt3Point);

	return pt3Point;
}


float CvPlot::getSymbolSize() const
{
	if (isVisibleWorked())
	{
		if (isShowCitySymbols())
			return 1.6f;
		else return 1.2f;
	}
	else
	{
		if (isShowCitySymbols())
			return 1.2f;
		else return 0.8f;
	}
}


float CvPlot::getSymbolOffsetX(int iOffset) const
{
	return 40.0f + iOffset * 28.0f * getSymbolSize() - GC.getPLOT_SIZE() / 2.0f;
}


float CvPlot::getSymbolOffsetY(int iOffset) const
{
	return -(GC.getPLOT_SIZE() / 2.0f) + 50.0f;
}


void CvPlot::doTurn()
{
	PROFILE_FUNC();

	if (getForceUnownedTimer() > 0)
		changeForceUnownedTimer(-1);

	if (isOwned())
		changeOwnershipDuration(1);

	if (getImprovementType() != NO_IMPROVEMENT)
		changeImprovementDuration(1);

	doFeature();
	doCulture();
	verifyUnitValidPlot();

	/*if (!isOwned())
		doImprovementUpgrade();*/ // advc (comment): Was already commented out in BtS

	// advc: This sounds pretty slow and I'm not that I've ever needed it
	/*#ifdef _DEBUG // XXX
	for (CLLNode<IDInfo> const* pUnitNode = headUnitNode(); pUnitNode != NULL;
		pUnitNode = nextUnitNode(pUnitNode))
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		FAssertMsg(pLoopUnit->atPlot(this), "pLoopUnit is expected to be at the current plot instance");
	}*/
	//#endif // XXX
}


void CvPlot::doImprovement()  // advc: some style changes
{
	PROFILE_FUNC();

	FAssert(isBeingWorked() && isOwned());

	if (getImprovementType() != NO_IMPROVEMENT && getBonusType() == NO_BONUS)
	{
		for (int i = 0; i < GC.getNumBonusInfos(); i++)
		{
			BonusTypes eBonus = (BonusTypes)i;
			CvBonusInfo const& kBonus = GC.getInfo(eBonus);
			if (!GET_TEAM(getTeam()).isHasTech((TechTypes)kBonus.getTechReveal()))
				continue;
			/*if (GC.getInfo(getImprovementType()).getImprovementBonusDiscoverRand(eBonus) > 0) { // BtS
				if (GC.getGame().getSorenRandNum(GC.getInfo(getImprovementType()).getImprovementBonusDiscoverRand(eBonus), "Bonus Discovery") == 0) {*/
			// UNOFFICIAL_PATCH, Gamespeed scaling, 03/04/10, jdog5000: START
			int iOdds = GC.getInfo(getImprovementType()).getImprovementBonusDiscoverRand(eBonus);
			if(iOdds <= 0)
				continue;
			// <advc.rom3>
			//Afforess: check for valid terrains for this bonus before discovering it
			if(!canHaveBonus(eBonus), false, /* advc.129: */ true)
				continue; // </advc.rom3>
			iOdds *= GC.getInfo(GC.getGame().getGameSpeedType()).getVictoryDelayPercent();
			iOdds /= 100;

			if (GC.getGame().getSorenRandNum(iOdds, "Bonus Discovery") == 0)
			{	// UNOFFICIAL_PATCH: END
				setBonusType(eBonus);
				CvCity* pCity = GC.getMap().findCity(getX(), getY(), getOwner(), NO_TEAM, false);
				if (pCity != NULL)
				{
					CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_DISCOVERED_NEW_RESOURCE",
							kBonus.getTextKeyWide(), pCity->getNameKey());
					gDLL->getInterfaceIFace()->addMessage(getOwner(), false,
							GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_DISCOVERBONUS",
							MESSAGE_TYPE_MINOR_EVENT, kBonus.getButton(),
							(ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"),
							getX(), getY(), true, true);
				}
				break;
			}
		}
	}

	doImprovementUpgrade();
}

void CvPlot::doImprovementUpgrade()
{
	if (getImprovementType() != NO_IMPROVEMENT)
	{
		ImprovementTypes eImprovementUpdrade = (ImprovementTypes)GC.getInfo(getImprovementType()).getImprovementUpgrade();
		if (eImprovementUpdrade != NO_IMPROVEMENT)
		{
			if ((isBeingWorked()
					&& !getWorkingCity()->isDisorder()) // advc.001
					|| GC.getInfo(eImprovementUpdrade).isOutsideBorders())
			{
				changeUpgradeProgress(GET_PLAYER(getOwner()).getImprovementUpgradeRate());

				if (getUpgradeProgress() >= GC.getGame().getImprovementUpgradeTime(getImprovementType()))
				{
					setImprovementType(eImprovementUpdrade);
				}
			}
		}
	}
}

void CvPlot::updateCulture(bool bBumpUnits, bool bUpdatePlotGroups)
{
	PROFILE_FUNC(); // advc

	if(isCity())
		return; // advc

	// <advc.035>
	PlayerTypes eCulturalOwner = calculateCulturalOwner();
	setSecondOwner(eCulturalOwner);
	if(GC.getDefineBOOL(CvGlobals::OWN_EXCLUSIVE_RADIUS) && eCulturalOwner != NO_PLAYER)
	{
		PlayerTypes eSecondOwner = calculateCulturalOwner(false, true);
		if(eSecondOwner != NO_PLAYER)
		{
			if(!GET_TEAM(eSecondOwner).isAtWar(TEAMID(eCulturalOwner)))
				eCulturalOwner = eSecondOwner;
			else setSecondOwner(eSecondOwner);
		}
		else
		{
			FAssertMsg(eSecondOwner != NO_PLAYER, "eCulturalOwner!=NO_PLAYER"
				" should imply eSecondOwner!=NO_PLAYER");
		}
	}
	setOwner(eCulturalOwner, // </advc.035>
			bBumpUnits, bUpdatePlotGroups);
}


void CvPlot::updateFog()
{
	//PROFILE_FUNC();

	if (!GC.IsGraphicsInitialized())
		return;

	FAssert(GC.getGame().getActiveTeam() != NO_TEAM);

	if (isRevealed(GC.getGame().getActiveTeam()))
	{
		if (gDLL->getInterfaceIFace()->isBareMapMode())
			gDLL->getEngineIFace()->LightenVisibility(getFOWIndex());
		else
		{
			static bool const bCityScreenFogEnabled = GC.getDefineBOOL("CITY_SCREEN_FOG_ENABLED"); // advc.opt: static
			if (bCityScreenFogEnabled && gDLL->getInterfaceIFace()->isCityScreenUp() &&
					gDLL->getInterfaceIFace()->getHeadSelectedCity() != getWorkingCity())
				gDLL->getEngineIFace()->DarkenVisibility(getFOWIndex());
			else if (isActiveVisible(false))
				gDLL->getEngineIFace()->LightenVisibility(getFOWIndex());
			else gDLL->getEngineIFace()->DarkenVisibility(getFOWIndex());
		}
	}
	else gDLL->getEngineIFace()->BlackenVisibility(getFOWIndex());
}


void CvPlot::updateVisibility()
{
	PROFILE("CvPlot::updateVisibility");

	if (!GC.IsGraphicsInitialized())
		return;

	setLayoutDirty(true);

	updateSymbolVisibility();
	updateFeatureSymbolVisibility();
	updateRouteSymbol();

	CvCity* pCity = getPlotCity();
	if (pCity != NULL)
		pCity->updateVisibility();
	updateCenterUnit(); // advc.061, advc.001w
}


void CvPlot::updateSymbolDisplay()
{
	//PROFILE_FUNC();

	if (!GC.IsGraphicsInitialized())
		return;

	for (int iLoop = 0; iLoop < getNumSymbols(); iLoop++)
	{
		CvSymbol* pLoopSymbol = getSymbol(iLoop);

		if (pLoopSymbol != NULL)
		{
			if (isShowCitySymbols())
				gDLL->getSymbolIFace()->setAlpha(pLoopSymbol, (isVisibleWorked()) ? 1.0f : 0.8f);
			else gDLL->getSymbolIFace()->setAlpha(pLoopSymbol, (isVisibleWorked()) ? 0.8f : 0.6f);
			gDLL->getSymbolIFace()->setScale(pLoopSymbol, getSymbolSize());
			gDLL->getSymbolIFace()->updatePosition(pLoopSymbol);
		}
	}
}


void CvPlot::updateSymbolVisibility()
{
	//PROFILE_FUNC();
	if (!GC.IsGraphicsInitialized())
		return;

	for (int iLoop = 0; iLoop < getNumSymbols(); iLoop++)
	{
		CvSymbol* pLoopSymbol = getSymbol(iLoop);

		if (pLoopSymbol != NULL)
		{
			if (isRevealed(GC.getGame().getActiveTeam(), true) &&
					(isShowCitySymbols() ||
					(gDLL->getInterfaceIFace()->isShowYields() &&
					!gDLL->getInterfaceIFace()->isCityScreenUp())))
				gDLL->getSymbolIFace()->Hide(pLoopSymbol, false);
			else gDLL->getSymbolIFace()->Hide(pLoopSymbol, true);
		}
	}
}


void CvPlot::updateSymbols()
{
	//PROFILE_FUNC();
	if (!GC.IsGraphicsInitialized())
		return;

	deleteAllSymbols();

	int yieldAmounts[NUM_YIELD_TYPES];
	int maxYield = 0;
	for (int iYieldType = 0; iYieldType < NUM_YIELD_TYPES; iYieldType++)
	{
		int iYield = calculateYield(((YieldTypes)iYieldType), true);
		yieldAmounts[iYieldType] = iYield;
		if(iYield>maxYield)
		{
			maxYield = iYield;
		}
	}

	if(maxYield>0)
	{
		static int maxYieldStack = GC.getDefineINT("MAX_YIELD_STACK"); // advc.opt: static
		int layers = maxYield /maxYieldStack + 1;

		CvSymbol *pSymbol= NULL;
		for(int i=0;i<layers;i++)
		{
			pSymbol = addSymbol();
			for (int iYieldType = 0; iYieldType < NUM_YIELD_TYPES; iYieldType++)
			{
				int iYield = yieldAmounts[iYieldType] - (maxYieldStack * i);
				LIMIT_RANGE(0,iYield, maxYieldStack);
				if(yieldAmounts[iYieldType])
					gDLL->getSymbolIFace()->setTypeYield(pSymbol,iYieldType,iYield);
			}
		}
		for(int i=0;i<getNumSymbols();i++)
		{
			SymbolTypes eSymbol  = (SymbolTypes)0;
			pSymbol = getSymbol(i);
			gDLL->getSymbolIFace()->init(pSymbol, gDLL->getSymbolIFace()->getID(pSymbol), i, eSymbol, this);
		}
	}

	updateSymbolDisplay();
	updateSymbolVisibility();
}


void CvPlot::updateMinimapColor()
{
	//PROFILE_FUNC();
	if (!GC.IsGraphicsInitialized())
		return;
	// <advc.002a>
	int const iMode = GC.getDefineINT(CvGlobals::MINIMAP_WATER_MODE);
	if(iMode == 3 && isWater())
		return; // </advc.002a>
	gDLL->getInterfaceIFace()->setMinimapColor(MINIMAPMODE_TERRITORY, getX(), getY(), plotMinimapColor(), STANDARD_MINIMAP_ALPHA
			/ ((isWater() && iMode != 4) ? 2.3f : 1.f)); // advc.002a
}


void CvPlot::updateCenterUnit()
{
	if (!GC.IsGraphicsInitialized())
		return;

	if (!isActiveVisible(true))
	{
		setCenterUnit(NULL);
		return;
	}

	setCenterUnit(getSelectedUnit());

	/*if (getCenterUnit() == NULL) {
		setCenterUnit(getBestDefender(GC.getGame().getActivePlayer(), NO_PLAYER,
				NULL, false, false, true));
	}*/
	PlayerTypes eActivePlayer = GC.getGame().getActivePlayer(); // advc
	if (getCenterUnit() == NULL)
		setCenterUnit(getBestDefender(eActivePlayer));

	if (getCenterUnit() == NULL)
	{	// <advc.028>
		CvUnit* pBestDef = getBestDefender(NO_PLAYER, eActivePlayer,
				gDLL->getInterfaceIFace()->getHeadSelectedUnit(), true,
				false, true); // advc.061
		if(pBestDef != NULL)
			setCenterUnit(pBestDef); // </advc.028>
	} // disabled by K-Mod. I don't think it's relevant whether or not the best defender can move.
	/*  advc.001: Enabled again, instead disabled the one with bTestCanMove=true --
		I think that's the one karadoc had meant to disable. */

	if (getCenterUnit() == NULL)
	{
		//setCenterUnit(getBestDefender(NO_PLAYER, GC.getGame().getActivePlayer(), gDLL->getInterfaceIFace()->getHeadSelectedUnit()));
		// <advc.028> Replacing the above
		CvUnit* pBestDef = getBestDefender(NO_PLAYER, eActivePlayer,
				gDLL->getInterfaceIFace()->getHeadSelectedUnit(), false,
				false, true); // advc.061
		if(pBestDef != NULL)
			setCenterUnit(pBestDef); // </advc.028>
	}

	if (getCenterUnit() == NULL)
	{
		//setCenterUnit(getBestDefender(NO_PLAYER, GC.getGame().getActivePlayer()));
		// <advc.028> Replacing the above
		CvUnit* pBestDef = getBestDefender(NO_PLAYER, eActivePlayer, NULL, false,
				false, true); // advc.061
		if(pBestDef != NULL)
			setCenterUnit(pBestDef); // </advc.028>
	}
}


void CvPlot::verifyUnitValidPlot()  // advc: some style changes
{
	//PROFILE_FUNC(); // advc.003o
	std::vector<std::pair<PlayerTypes, int> > bumped_groups; // K-Mod

	std::vector<CvUnit*> aUnits;
	for (CLLNode<IDInfo> const* pUnitNode = headUnitNode(); pUnitNode != NULL;
		pUnitNode = nextUnitNode(pUnitNode))
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		if (pLoopUnit != NULL)
			aUnits.push_back(pLoopUnit);
		FAssertMsg(pLoopUnit != NULL, "Can this happen?"); // advc
	}

	std::vector<CvUnit*>::iterator it = aUnits.begin();
	while (it != aUnits.end())
	{
		CvUnit* pLoopUnit = *it;
		bool bErased = false;
		if (pLoopUnit == NULL)
		{
			FAssertMsg(pLoopUnit != NULL, "Can this happen?"); // advc
			continue;
		}
		if (pLoopUnit->atPlot(this) && !pLoopUnit->isCargo() && !pLoopUnit->isCombat())
		{
			if (!isValidDomainForLocation(*pLoopUnit) ||
				!pLoopUnit->canEnterArea(getTeam(), area()))
			{
				if (!pLoopUnit->jumpToNearestValidPlot(true))
					bErased = true;
				// K-Mod:
				else bumped_groups.push_back(std::make_pair(pLoopUnit->getOwner(), pLoopUnit->getGroupID()));
			}
		}
		if (bErased)
			it = aUnits.erase(it);
		else ++it;
	}

	if (isOwned())
	{
		it = aUnits.begin();
		while (it != aUnits.end())
		{
			CvUnit* pLoopUnit = *it;
			bool bErased = false;
			if (pLoopUnit == NULL)
			{
				FAssertMsg(pLoopUnit != NULL, "Can this happen?"); // advc
				continue;
			}
			else
			{
				if (pLoopUnit->atPlot(this) && !pLoopUnit->isCombat())
				{
					if (pLoopUnit->getTeam() != getTeam() && (getTeam() == NO_TEAM ||
						!GET_TEAM(getTeam()).isVassal(pLoopUnit->getTeam())))
					{
						if (isVisibleEnemyUnit(pLoopUnit))
						{
							if (!pLoopUnit->isInvisible(getTeam(), false))
							{
								if (!pLoopUnit->jumpToNearestValidPlot(true))
									bErased = true;
								// K-Mod:
								else bumped_groups.push_back(std::make_pair(pLoopUnit->getOwner(), pLoopUnit->getGroupID()));
							}
						}
					}
				}
			}
			if (bErased)
				it = aUnits.erase(it);
			else ++it;
		}
	}

	// K-Mod
	// first remove duplicate group numbers
	std::sort(bumped_groups.begin(), bumped_groups.end());
	bumped_groups.erase(std::unique(bumped_groups.begin(), bumped_groups.end()), bumped_groups.end());
	// now divide the broken groups
	for (size_t i = 0; i < bumped_groups.size(); i++)
	{
		CvSelectionGroup* pLoopGroup = GET_PLAYER(bumped_groups[i].first).getSelectionGroup(bumped_groups[i].second);
		if (pLoopGroup)
			pLoopGroup->regroupSeparatedUnits();
	}
	// K-Mod end
}

/*  K-Mod, 2/jan/11, karadoc
	forceBumpUnits() forces all units off the plot, onto a nearby plot */
void CvPlot::forceBumpUnits()
{
	// Note: this function is almost certainly not optimal.
	// I just took the code from another function and I don't want to mess it up.
	// (advc: Presumably, he based it on verifyUnitValidPlot above.)
	std::vector<CvUnit*> aUnits;
	for (CLLNode<IDInfo> const* pUnitNode = headUnitNode(); pUnitNode != NULL;
		pUnitNode = nextUnitNode(pUnitNode))
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		if (pLoopUnit != NULL)
			aUnits.push_back(pLoopUnit);
		FAssertMsg(pLoopUnit != NULL, "Can this happen?"); // advc
	}

	std::vector<CvUnit*>::iterator it = aUnits.begin();
	while (it != aUnits.end())
	{
		CvUnit* pLoopUnit = *it;
		bool bErased = false;

		if (pLoopUnit == NULL)
		{
			FAssertMsg(pLoopUnit != NULL, "Can this happen?"); // advc
			continue;
		}
		else
		{
			if (pLoopUnit->atPlot(this) && !pLoopUnit->isCargo() &&
				!pLoopUnit->isCombat())
			{
				if (!pLoopUnit->jumpToNearestValidPlot(true, true))
					bErased = true;
			}
		}
		if (bErased)
			it = aUnits.erase(it);
		else ++it;
	}
} // K-Mod end

// K-Mod. Added bBomb argument.
// bBomb signals that the explosion should damage units, buildings, and city population.
// (I've also tidied up the code a little bit)
void CvPlot::nukeExplosion(int iRange, CvUnit* pNukeUnit, bool bBomb)
{
	// <advc.opt>
	static int const iNUKE_FALLOUT_PROB = GC.getDefineINT("NUKE_FALLOUT_PROB");
	static int const iNUKE_UNIT_DAMAGE_BASE = GC.getDefineINT("NUKE_UNIT_DAMAGE_BASE");
	static int const iNUKE_UNIT_DAMAGE_RAND_1 = GC.getDefineINT("NUKE_UNIT_DAMAGE_RAND_1");
	static int const iNUKE_UNIT_DAMAGE_RAND_2 = GC.getDefineINT("NUKE_UNIT_DAMAGE_RAND_2");
	static int const iNUKE_BUILDING_DESTRUCTION_PROB = GC.getDefineINT("NUKE_BUILDING_DESTRUCTION_PROB");
	static int const iNUKE_POPULATION_DEATH_BASE = GC.getDefineINT("NUKE_POPULATION_DEATH_BASE");
	static int const iNUKE_POPULATION_DEATH_RAND_1 = GC.getDefineINT("NUKE_POPULATION_DEATH_RAND_1");
	static int const iNUKE_POPULATION_DEATH_RAND_2 = GC.getDefineINT("NUKE_POPULATION_DEATH_RAND_2");
	// </advc.opt>
	for (int iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (int iDY = -(iRange); iDY <= iRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX(), getY(), iDX, iDY);
			if (pLoopPlot == NULL)
				continue;

			// (if we remove roads, don't remove them on the city... XXX)

			CvCity* pLoopCity = pLoopPlot->getPlotCity();
			if (pLoopCity == NULL)
			{
				if (!pLoopPlot->isWater() && !pLoopPlot->isImpassable())
				{
					if (NO_FEATURE == pLoopPlot->getFeatureType() || !GC.getInfo(pLoopPlot->getFeatureType()).isNukeImmune())
					{
						if (GC.getGame().getSorenRandNum(100, "Nuke Fallout") < iNUKE_FALLOUT_PROB)
						{
							pLoopPlot->setImprovementType(NO_IMPROVEMENT);
							pLoopPlot->setFeatureType((FeatureTypes)GC.getDefineINT("NUKE_FEATURE"));
						}
					}
				}
			}
			// K-Mod. If this is not a bomb, then we're finished with this plot.
			if (!bBomb)
				continue;

			CLinkList<IDInfo> oldUnits;
			{
				for (CLLNode<IDInfo> const* pUnitNode = pLoopPlot->headUnitNode();
						pUnitNode != NULL; pUnitNode = pLoopPlot->nextUnitNode(pUnitNode))
					oldUnits.insertAtEnd(pUnitNode->m_data);
			}
			CLLNode<IDInfo>* pUnitNode = oldUnits.head();
			while (pUnitNode != NULL)
			{
				CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = oldUnits.next(pUnitNode);

				if (pLoopUnit == NULL || pLoopUnit == pNukeUnit)
					continue;
				// <dlph.7>
				TeamTypes eAttackingTeam = NO_TEAM;
				if(pNukeUnit != NULL)
					eAttackingTeam = TEAMID(pNukeUnit->getOwner());
				// </dlph.7>
				if (!pLoopUnit->isNukeImmune() && !pLoopUnit->isDelayedDeath() &&
					// <dlph.7>
					// Nukes target only enemy and own units.
					// Needed because blocking by neutral players disabled.
					(eAttackingTeam == NO_TEAM ||
					pLoopUnit->isEnemy(eAttackingTeam) ||
					eAttackingTeam == TEAMID(pLoopUnit->getOwner())))
					// </dlph.7>
				{
					int iNukeDamage = (iNUKE_UNIT_DAMAGE_BASE +
							GC.getGame().getSorenRandNum(iNUKE_UNIT_DAMAGE_RAND_1, "Nuke Damage 1") +
							GC.getGame().getSorenRandNum(iNUKE_UNIT_DAMAGE_RAND_2, "Nuke Damage 2"));

					if (pLoopCity != NULL)
					{
						iNukeDamage *= std::max(0, (pLoopCity->getNukeModifier() + 100));
						iNukeDamage /= 100;
					}

					if (pLoopUnit->canFight() || pLoopUnit->airBaseCombatStr() > 0)
					{
						pLoopUnit->changeDamage(iNukeDamage, pNukeUnit != NULL ? pNukeUnit->getOwner() : NO_PLAYER);
					}
					//else if (iNukeDamage >= GC.getDefineINT("NUKE_NON_COMBAT_DEATH_THRESHOLD"))
					// <dlph.20>
					else if(GC.getGame().getSorenRandNum(100,
							"Non-Combat Nuke Rand") * 100 <
							std::max(0, ((pLoopCity == NULL ? 0 :
							pLoopCity->getNukeModifier()) + 100)) *
							(iNUKE_UNIT_DAMAGE_BASE - 1 +
							(iNUKE_UNIT_DAMAGE_RAND_1 +
							iNUKE_UNIT_DAMAGE_RAND_2 - 1) / 2))
							// </dlph.20>
						pLoopUnit->kill(false, pNukeUnit != NULL ? pNukeUnit->getOwner() : NO_PLAYER);
				}
			}
			if (pLoopCity == NULL)
				continue;

			for (int i = 0; i < GC.getNumBuildingInfos(); ++i)
			{
				BuildingTypes eBuilding = (BuildingTypes)i;
				if (pLoopCity->getNumRealBuilding(eBuilding) > 0)
				{
					if (!GC.getInfo(eBuilding).isNukeImmune())
					{
						if (GC.getGame().getSorenRandNum(100, "Building Nuked") <
							iNUKE_BUILDING_DESTRUCTION_PROB)
						{
							pLoopCity->setNumRealBuilding(eBuilding,
									pLoopCity->getNumRealBuilding(eBuilding) - 1);
						}
					}
				}
			}
			int iNukedPopulation = ((pLoopCity->getPopulation() *
					(iNUKE_POPULATION_DEATH_BASE +
					GC.getGame().getSorenRandNum(iNUKE_POPULATION_DEATH_RAND_1, "Population Nuked 1") +
					GC.getGame().getSorenRandNum(iNUKE_POPULATION_DEATH_RAND_2, "Population Nuked 2")))
					/ 100);
			iNukedPopulation *= std::max(0, (pLoopCity->getNukeModifier() + 100));
			iNukedPopulation /= 100;
			pLoopCity->changePopulation(-std::min(pLoopCity->getPopulation() - 1, iNukedPopulation));
		}
	}
	if (bBomb) // K-Mod
	{
		GC.getGame().changeNukesExploded(1);
		CvEventReporter::getInstance().nukeExplosion(this, pNukeUnit);
	}
}


bool CvPlot::isConnectedTo(const CvCity* pCity) const
{
	FAssert(isOwned());
	return (isSamePlotGroup(*pCity->plot(), getOwner()) ||
			isSamePlotGroup(*pCity->plot(), pCity->getOwner()));
}


bool CvPlot::isConnectedToCapital(PlayerTypes ePlayer) const
{
	if (ePlayer == NO_PLAYER)
		ePlayer = getOwner();

	if (ePlayer != NO_PLAYER)
	{
		CvCity* pCapitalCity = GET_PLAYER(ePlayer).getCapitalCity();
		if (pCapitalCity != NULL)
			return isConnectedTo(pCapitalCity);
	}

	return false;
}


int CvPlot::getPlotGroupConnectedBonus(PlayerTypes ePlayer, BonusTypes eBonus) const
{
	FAssert(ePlayer != NO_PLAYER && eBonus != NO_BONUS);

	CvPlotGroup* pPlotGroup = getPlotGroup(ePlayer);
	if (pPlotGroup != NULL)
		return pPlotGroup->getNumBonuses(eBonus);
	else return 0;
}


bool CvPlot::isPlotGroupConnectedBonus(PlayerTypes ePlayer, BonusTypes eBonus) const
{
	return (getPlotGroupConnectedBonus(ePlayer, eBonus) > 0);
}


bool CvPlot::isAdjacentPlotGroupConnectedBonus(PlayerTypes ePlayer, BonusTypes eBonus) const
{
	// K-Mod. Allow this plot to have whatever resources are available in the city working the plot.
	// (The purpose of this is to allow railroads to be built the 'oil' from Standard Ethonol.)
	CvCity* pCity = getWorkingCity();
	if (pCity && pCity->getOwner() == ePlayer && pCity->hasBonus(eBonus))
		return true;
	// K-Mod end

	for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), ((DirectionTypes)iI));
		if (pAdjacentPlot != NULL)
		{
			if (pAdjacentPlot->isPlotGroupConnectedBonus(ePlayer, eBonus))
				return true;
		}
	}

	return false;
}


void CvPlot::updatePlotGroupBonus(bool bAdd, /* advc.064d: */ bool bVerifyProduction)  // advc: style changes
{
	PROFILE_FUNC();

	if (!isOwned())
		return;

	CvPlotGroup* pPlotGroup = getPlotGroup(getOwner());
	if(pPlotGroup == NULL)
		return;

	CvCity* pPlotCity = getPlotCity();
	if (pPlotCity != NULL)
	{
		for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
		{
			BonusTypes eLoopBonus = (BonusTypes)iI;
			if (!GET_TEAM(getTeam()).isBonusObsolete(eLoopBonus))
			{
				pPlotGroup->changeNumBonuses(eLoopBonus,
						pPlotCity->getFreeBonus(eLoopBonus) * (bAdd ? 1 : -1));
			}
		}
		if (pPlotCity->isCapital())
		{
			for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
			{
				BonusTypes eLoopBonus = (BonusTypes)iI;
				pPlotGroup->changeNumBonuses(eLoopBonus,
						GET_PLAYER(getOwner()).getBonusExport(eLoopBonus) *
						(bAdd ? -1 : 1));
				pPlotGroup->changeNumBonuses(eLoopBonus,
						GET_PLAYER(getOwner()).getBonusImport(eLoopBonus) *
						(bAdd ? 1 : -1));
			}
		}
	}

	/* original code
	eNonObsoleteBonus = getNonObsoleteBonusType(getTeam());
	if (eNonObsoleteBonus != NO_BONUS) {
		if (GET_TEAM(getTeam()).isHasTech((TechTypes)(GC.getInfo(eNonObsoleteBonus).getTechCityTrade()))) {
			if (isCity(true, getTeam()) || ((getImprovementType() != NO_IMPROVEMENT) && GC.getInfo(getImprovementType()).isImprovementBonusTrade(eNonObsoleteBonus))) {
				if ((pPlotGroup != NULL) && isBonusNetwork(getTeam()))
					pPlotGroup->changeNumBonuses(eNonObsoleteBonus, ((bAdd) ? 1 : -1));
	} } } */
	// K-Mod. I'm just trying to standardize the code to reduce the potential for mistakes. There are no functionality changes here.
	BonusTypes eBonus = getNonObsoleteBonusType(getTeam(), true);
	if (eBonus != NO_BONUS && pPlotGroup && isBonusNetwork(getTeam()))
		pPlotGroup->changeNumBonuses(eBonus, bAdd ? 1 : -1);
	// K-Mod end
	// <advc.064d>
	if (bVerifyProduction)
		pPlotGroup->verifyCityProduction(); // </advc.064d>
}


bool CvPlot::isAdjacentToArea(int iAreaID) const
{
	PROFILE_FUNC();

	for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), ((DirectionTypes)iI));
		if (pAdjacentPlot != NULL)
		{
			if (pAdjacentPlot->getArea() == iAreaID)
			{
				return true;
			}
		}
	}

	return false;
}

bool CvPlot::isAdjacentToArea(const CvArea* pArea) const
{
	return isAdjacentToArea(pArea->getID());
}


bool CvPlot::shareAdjacentArea(const CvPlot* pPlot) const
{
	PROFILE_FUNC();

	int iLastArea = FFreeList::INVALID_INDEX;
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
		if (pAdjacentPlot != NULL)
		{
			int iCurrArea = pAdjacentPlot->getArea();
			if (iCurrArea != iLastArea)
			{
				if (pPlot->isAdjacentToArea(iCurrArea))
					return true;

				iLastArea = iCurrArea;
			}
		}
	}

	return false;
}


bool CvPlot::isAdjacentToLand() const
{
	PROFILE_FUNC();

	for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
		if (pAdjacentPlot != NULL)
		{
			if (!pAdjacentPlot->isWater())
				return true;
		}
	}

	return false;
}


bool CvPlot::isCoastalLand(int iMinWaterSize) const
{
	//PROFILE_FUNC(); // advc.003o: Called very frequently, probably mainly from CvUnitAI::AI_plotValid and teamStepValid_advc (CvGameCoreUtils).

	if (isWater())
		return false;
	// <advc.003t>
	if (iMinWaterSize < 0)
		iMinWaterSize = GC.getDefineINT(CvGlobals::MIN_WATER_SIZE_FOR_OCEAN);
	// </advc.003t>
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
		if (pAdjacentPlot == NULL)
			continue;

		if (pAdjacentPlot->isWater() /* advc.030: */ && !pAdjacentPlot->isImpassable() &&
				pAdjacentPlot->area()->getNumTiles() >= iMinWaterSize)
			return true;
	}
	return false;
}


bool CvPlot::isVisibleWorked() const
{
	if (isBeingWorked())
	{
		if (getTeam() == GC.getGame().getActiveTeam() || GC.getGame().isDebugMode())
			return true;
	}

	return false;
}


bool CvPlot::isWithinTeamCityRadius(TeamTypes eTeam, PlayerTypes eIgnorePlayer) const
{
	PROFILE_FUNC();

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayer const& kMember = GET_PLAYER((PlayerTypes)i);
		if (!kMember.isAlive() || kMember.getTeam() != eTeam)
			continue; // advc

		if (eIgnorePlayer == NO_PLAYER || kMember.getID() != eIgnorePlayer)
		{
			if (isPlayerCityRadius(kMember.getID()))
				return true;
		}
	}

	return false;
}


bool CvPlot::isLake() const
{
	CvArea* pArea = area();
	if (pArea != NULL)
		return pArea->isLake();

	return false;
}


// XXX if this changes need to call updateIrrigated() and pCity->updateFreshWaterHealth()
// XXX precalculate this???
bool CvPlot::isFreshWater() const
{
	if (isWater())
		return false;

	if (isImpassable())
		return false;

	if (isRiver())
		return true;

	for (int iDX = -1; iDX <= 1; iDX++)
	{
		for (int iDY = -1; iDY <= 1; iDY++)
		{
			CvPlot* pLoopPlot	= plotXY(getX(), getY(), iDX, iDY);
			if (pLoopPlot == NULL)
				continue;

			if (pLoopPlot->isLake())
				return true;

			if (pLoopPlot->isFeature())
			{
				if (GC.getInfo(pLoopPlot->getFeatureType()).isAddsFreshWater())
					return true;
			}
		}
	}

	return false;
}


bool CvPlot::isPotentialIrrigation() const
{
	if ((isCity() && !isHills()) || (getImprovementType() != NO_IMPROVEMENT &&
		GC.getInfo(getImprovementType()).isCarriesIrrigation()))
	{
		if (getTeam() != NO_TEAM && GET_TEAM(getTeam()).isIrrigation())
			return true;
	}

	return false;
}


bool CvPlot::canHavePotentialIrrigation() const
{
	if (isCity() && !isHills())
		return true;
	// <advc.opt>
	if(isWater())
		return false;
	// </advc.opt>
	for (int iI = 0; iI < GC.getNumImprovementInfos(); ++iI)
	{
		if (GC.getInfo((ImprovementTypes)iI).isCarriesIrrigation())
		{
			if (canHaveImprovement((ImprovementTypes)iI, NO_TEAM, true))
				return true;
		}
	}

	return false;
}


bool CvPlot::isIrrigationAvailable(bool bIgnoreSelf) const
{
	if (!bIgnoreSelf && isIrrigated())
		return true;

	if (isFreshWater())
		return true;

	for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
		if (pAdjacentPlot != NULL && pAdjacentPlot->isIrrigated())
			return true;
	}

	return false;
}


bool CvPlot::isRiverMask() const
{
	if (isNOfRiver() || isWOfRiver())
		return true;

	CvPlot* pPlot = plotDirection(getX(), getY(), DIRECTION_EAST);
	if (pPlot != NULL && pPlot->isNOfRiver())
		return true;

	pPlot = plotDirection(getX(), getY(), DIRECTION_SOUTH);
	if (pPlot != NULL && pPlot->isWOfRiver())
		return true;

	return false;
}


bool CvPlot::isRiverCrossingFlowClockwise(DirectionTypes eDirection) const
{
	switch(eDirection)
	{
	case DIRECTION_NORTH:
	{
		CvPlot* pPlot = plotDirection(getX(), getY(), DIRECTION_NORTH);
		if (pPlot != NULL)
			return (pPlot->getRiverWEDirection() == CARDINALDIRECTION_EAST);
		break;
	}
	case DIRECTION_EAST:
		return (getRiverNSDirection() == CARDINALDIRECTION_SOUTH);
	case DIRECTION_SOUTH:
		return (getRiverWEDirection() == CARDINALDIRECTION_WEST);
	case DIRECTION_WEST:
	{
		CvPlot* pPlot = plotDirection(getX(), getY(), DIRECTION_WEST);
		if(pPlot != NULL)
			return (pPlot->getRiverNSDirection() == CARDINALDIRECTION_NORTH);
		break;
	}
	default: FAssert(false);
	}

	return false;
}


bool CvPlot::isRiverSide() const
{
	for (int iI = 0; iI < NUM_CARDINALDIRECTION_TYPES; ++iI)
	{
		CvPlot* pLoopPlot = plotCardinalDirection(getX(), getY(), (CardinalDirectionTypes)iI);
		if (pLoopPlot != NULL)
		{
			if (isRiverCrossing(directionXY(this, pLoopPlot)))
				return true;
		}
	}

	return false;
}


bool CvPlot::isRiver() const
{
	return (getRiverCrossingCount() > 0);
}


bool CvPlot::isRiverConnection(DirectionTypes eDirection) const
{
	switch (eDirection)
	{
	case NO_DIRECTION: return false; // advc.opt (faster than checking it upfront?)
	case DIRECTION_NORTH:
		return (isRiverCrossing(DIRECTION_EAST) || isRiverCrossing(DIRECTION_WEST));
	case DIRECTION_NORTHEAST:
		return (isRiverCrossing(DIRECTION_NORTH) || isRiverCrossing(DIRECTION_EAST));
	case DIRECTION_EAST:
		return (isRiverCrossing(DIRECTION_NORTH) || isRiverCrossing(DIRECTION_SOUTH));
	case DIRECTION_SOUTHEAST:
		return (isRiverCrossing(DIRECTION_SOUTH) || isRiverCrossing(DIRECTION_EAST));
	case DIRECTION_SOUTH:
		return (isRiverCrossing(DIRECTION_EAST) || isRiverCrossing(DIRECTION_WEST));
	case DIRECTION_SOUTHWEST:
		return (isRiverCrossing(DIRECTION_SOUTH) || isRiverCrossing(DIRECTION_WEST));
	case DIRECTION_WEST:
		return (isRiverCrossing(DIRECTION_NORTH) || isRiverCrossing(DIRECTION_SOUTH));
	case DIRECTION_NORTHWEST:
		return (isRiverCrossing(DIRECTION_NORTH) || isRiverCrossing(DIRECTION_WEST));
	default:
		FAssert(false);
		return false;
	}
}


CvPlot* CvPlot::getNearestLandPlotInternal(int iDistance) const
{
	if (iDistance > GC.getMap().getGridHeight() && iDistance > GC.getMap().getGridWidth())
		return NULL;

	for (int iDX = -iDistance; iDX <= iDistance; iDX++)
	{
		for (int iDY = -iDistance; iDY <= iDistance; iDY++)
		{
			if (abs(iDX) + abs(iDY) == iDistance)
			{
				CvPlot* pPlot = plotXY(getX(), getY(), iDX, iDY);
				if (pPlot != NULL)
				{
					if (!pPlot->isWater())
						return pPlot;
				}
			}
		}
	}
	return getNearestLandPlotInternal(iDistance + 1);
}


int CvPlot::getNearestLandArea() const
{
	CvPlot* pPlot = getNearestLandPlot();
	return (pPlot != NULL ? pPlot->getArea() : -1);
}


CvPlot* CvPlot::getNearestLandPlot() const
{
	return getNearestLandPlotInternal(0);
}


int CvPlot::seeFromLevel(TeamTypes eTeam) const
{
	int iLevel = GC.getInfo(getTerrainType()).getSeeFromLevel();
	if (isPeak())
		iLevel += GC.getDefineINT(CvGlobals::PEAK_SEE_FROM_CHANGE);
	if (isHills())
		iLevel += GC.getDefineINT(CvGlobals::HILLS_SEE_FROM_CHANGE);
	if (isWater())
	{
		iLevel += GC.getDefineINT(CvGlobals::SEAWATER_SEE_FROM_CHANGE);
		if (GET_TEAM(eTeam).isExtraWaterSeeFrom())
			iLevel++;
	}
	return iLevel;
}


int CvPlot::seeThroughLevel() const
{
	int iLevel = GC.getInfo(getTerrainType()).getSeeThroughLevel();
	if (isFeature())
		iLevel += GC.getInfo(getFeatureType()).getSeeThroughChange();
	if (isPeak())
		iLevel += GC.getDefineINT(CvGlobals::PEAK_SEE_THROUGH_CHANGE);
	if (isHills())
		iLevel += GC.getDefineINT(CvGlobals::HILLS_SEE_THROUGH_CHANGE);
	if (isWater())
		iLevel += GC.getDefineINT(CvGlobals::SEAWATER_SEE_FROM_CHANGE);
	return iLevel;
}



void CvPlot::changeAdjacentSight(TeamTypes eTeam, int iRange, bool bIncrement,
	CvUnit const* pUnit, bool bUpdatePlotGroups) // advc: const CvUnit*
{
	bool bAerial = (pUnit != NULL && pUnit->getDomainType() == DOMAIN_AIR);

	DirectionTypes eFacingDirection = NO_DIRECTION;
	if (!bAerial && NULL != pUnit)
		eFacingDirection = pUnit->getFacingDirection(true);

	//fill invisible types
	std::vector<InvisibleTypes> aSeeInvisibleTypes;
	if (NULL != pUnit)
	{
		for(int i=0;i<pUnit->getNumSeeInvisibleTypes();i++)
			aSeeInvisibleTypes.push_back(pUnit->getSeeInvisibleType(i));
	}

	if(aSeeInvisibleTypes.size() == 0)
		aSeeInvisibleTypes.push_back(NO_INVISIBLE);

	//check one extra outer ring
	if (!bAerial)
		iRange++;

	for(int i=0;i<(int)aSeeInvisibleTypes.size();i++)
	{
		for (int dx = -iRange; dx <= iRange; dx++)
		{
			for (int dy = -iRange; dy <= iRange; dy++)
			{
				//check if in facing direction
				if (bAerial || shouldProcessDisplacementPlot(dx, dy, iRange - 1, eFacingDirection))
				{
					bool outerRing = false;
					if (abs(dx) == iRange || abs(dy) == iRange)
						outerRing = true;

					//check if anything blocking the plot
					if (bAerial || canSeeDisplacementPlot(eTeam, dx, dy, dx, dy, true, outerRing))
					{
						CvPlot* pPlot = plotXY(getX(), getY(), dx, dy);
						if (NULL != pPlot)
						{
							pPlot->changeVisibilityCount(eTeam, ((bIncrement) ? 1 : -1), aSeeInvisibleTypes[i], bUpdatePlotGroups,
									pUnit); // advc.071
						}
					}
				}

				if (eFacingDirection != NO_DIRECTION)
				{
					if(abs(dx) <= 1 && abs(dy) <= 1) //always reveal adjacent plots when using line of sight
					{
						CvPlot* pPlot = plotXY(getX(), getY(), dx, dy);
						if (NULL != pPlot)
						{
							pPlot->changeVisibilityCount(eTeam, 1, aSeeInvisibleTypes[i], bUpdatePlotGroups,
									pUnit); // advc.071
							pPlot->changeVisibilityCount(eTeam, -1, aSeeInvisibleTypes[i], bUpdatePlotGroups,
									pUnit); // advc.071
						}
					}
				}
			}
		}
	}
}

bool CvPlot::canSeePlot(CvPlot *pPlot, TeamTypes eTeam, int iRange, DirectionTypes eFacingDirection) const
{
	if (pPlot == NULL)
		return false;

	iRange++;

	//find displacement
	int dx = pPlot->getX() - getX();
	int dy = pPlot->getY() - getY();
	CvMap const& m = GC.getMap();
	dx = m.dxWrap(dx); //world wrap
	dy = m.dyWrap(dy);

	//check if in facing direction
	if (shouldProcessDisplacementPlot(dx, dy, iRange - 1, eFacingDirection))
	{
		bool outerRing = false;
		if (abs(dx) == iRange || abs(dy) == iRange)
			outerRing = true;

		//check if anything blocking the plot
		if (canSeeDisplacementPlot(eTeam, dx, dy, dx, dy, true, outerRing))
			return true;
	}

	return false;
}

bool CvPlot::canSeeDisplacementPlot(TeamTypes eTeam, int dx, int dy, int originalDX, int originalDY, bool firstPlot, bool outerRing) const  // advc: some style changes
{
	CvPlot* pPlot = ::plotXY(getX(), getY(), dx, dy);
	if (pPlot == NULL)
		return false;

	//base case is current plot
	if(dx == 0 && dy == 0)
		return true;

	//find closest of three points (1, 2, 3) to original line from Start (S) to End (E)
	//The diagonal is computed first as that guarantees a change in position
	// -------------
	// |   | 2 | S |
	// -------------
	// | E | 1 | 3 |
	// -------------

	int displacements[3][2] = {
		{dx - getSign(dx), dy - getSign(dy)},
		{dx - getSign(dx), dy},
		{dx, dy - getSign(dy)}
	};
	int allClosest[3];
	int closest = -1;
	for (int i = 0; i < 3; i++)
	{
		//int tempClosest = abs(displacements[i][0] * originalDX - displacements[i][1] * originalDY); //more accurate, but less structured on a grid
		allClosest[i] = abs(displacements[i][0] * dy - displacements[i][1] * dx); //cross product
		if(closest == -1 || allClosest[i] < closest)
			closest = allClosest[i];
	}

	//iterate through all minimum plots to see if any of them are passable
	for(int i = 0; i < 3; i++)
	{
		int nextDX = displacements[i][0];
		int nextDY = displacements[i][1];
		if(nextDX != dx || nextDY != dy) //make sure we change plots
		{
			if(allClosest[i] == closest && canSeeDisplacementPlot(
				eTeam, nextDX, nextDY, originalDX, originalDY, false, false))
			{
				int fromLevel = seeFromLevel(eTeam);
				int throughLevel = pPlot->seeThroughLevel();
				if(outerRing) //check strictly higher level
				{
					CvPlot* passThroughPlot = ::plotXY(getX(), getY(), nextDX, nextDY);
					int passThroughLevel = passThroughPlot->seeThroughLevel();
					if (fromLevel >= passThroughLevel)
					{
						//either we can see through to it or it is high enough to see from far
						if(fromLevel > passThroughLevel || pPlot->seeFromLevel(eTeam) > fromLevel)
							return true;
					}
				}
				else
				{
					if(fromLevel >= throughLevel) //we can clearly see this level
						return true;
					else if(firstPlot) //we can also see it if it is the first plot that is too tall
						return true;
				}
			}
		}
	}

	return false;
}

bool CvPlot::shouldProcessDisplacementPlot(int dx, int dy, int range, DirectionTypes eFacingDirection) const  // advc: some style changes
{
	if(eFacingDirection == NO_DIRECTION)
		return true;

	if((dx == 0) && (dy == 0)) //always process this plot
		return true;

	//							N		NE		E		SE			S		SW		W			NW
	int displacements[8][2] = {{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}};
	int directionX = displacements[eFacingDirection][0];
	int directionY = displacements[eFacingDirection][1];

	//compute angle off of direction
	int crossProduct = directionX * dy - directionY * dx; //cross product
	int dotProduct = directionX * dx + directionY * dy; //dot product

	float theta = atan2((float)crossProduct, (float)dotProduct);
	float spread = 60 * (float)M_PI / 180;
	if(abs(dx) <= 1 && abs(dy) <= 1) //close plots use wider spread
		spread = 90 * (float) M_PI / 180;

	if(theta >= -spread / 2 && theta <= spread / 2)
		return true;
	return false;

	/*DirectionTypes leftDirection = GC.getTurnLeftDirection(eFacingDirection);
	DirectionTypes rightDirection = GC.getTurnRightDirection(eFacingDirection);
	//test which sides of the line equation (cross product)
	int leftSide = displacements[leftDirection][0] * dy - displacements[leftDirection][1] * dx;
	int rightSide = displacements[rightDirection][0] * dy - displacements[rightDirection][1] * dx;
	if((leftSide <= 0) && (rightSide >= 0))
		return true;
	return false;*/
}

void CvPlot::updateSight(bool bIncrement, bool bUpdatePlotGroups)
{
	CvCity* pCity = getPlotCity();
	if (pCity != NULL)
	{	// Religion - Disabled with new Espionage System
		/*for (iI = 0; iI < GC.getNumReligionInfos(); ++iI) {
			if (pCity->isHasReligion((ReligionTypes)iI)) {
				CvCity* pHolyCity = GC.getGame().getHolyCity((ReligionTypes)iI);
				if (pHolyCity != NULL) {
					if (GET_PLAYER(pHolyCity->getOwner()).getStateReligion() == iI)
						changeAdjacentSight(pHolyCity->getTeam(), GC.getDefineINT(CvGlobals::PLOT_VISIBILITY_RANGE), bIncrement, NULL, bUpdatePlotGroups);
				}
			}
		}*/

		// Vassal
		for (int iI = 0; iI < MAX_TEAMS; ++iI)
		{
			if (GET_TEAM(getTeam()).isVassal((TeamTypes)iI))
			{
				changeAdjacentSight((TeamTypes)iI, GC.getDefineINT(CvGlobals::PLOT_VISIBILITY_RANGE), bIncrement, NULL, bUpdatePlotGroups);
			}
		}

		// EspionageEffect
		for (int iI = 0; iI < MAX_CIV_TEAMS; ++iI)
		{
			if (pCity->getEspionageVisibility((TeamTypes)iI))
			{
				// Passive Effect: enough EPs gives you visibility into someone's cities
				changeAdjacentSight((TeamTypes)iI, GC.getDefineINT(CvGlobals::PLOT_VISIBILITY_RANGE), bIncrement, NULL, bUpdatePlotGroups);
			}
		}
	}

	// Owned
	if (isOwned())
	{
		changeAdjacentSight(getTeam(), GC.getDefineINT(CvGlobals::PLOT_VISIBILITY_RANGE), bIncrement, NULL, bUpdatePlotGroups);
	}

	// Unit
	for (CLLNode<IDInfo> const* pUnitNode = headUnitNode(); pUnitNode != NULL;
		pUnitNode = nextUnitNode(pUnitNode))
	{
		CvUnit const* pLoopUnit = ::getUnit(pUnitNode->m_data);
		changeAdjacentSight(pLoopUnit->getTeam(), pLoopUnit->visibilityRange(),
				bIncrement, pLoopUnit, bUpdatePlotGroups);
	}

	if (getReconCount() > 0)
	{
		int iRange = GC.getDefineINT(CvGlobals::RECON_VISIBILITY_RANGE);
		for (int iI = 0; iI < MAX_PLAYERS; ++iI)
		{
			FOR_EACH_UNIT(pLoopUnit, GET_PLAYER((PlayerTypes)iI))
			{
				if (pLoopUnit->getReconPlot() == this)
				{
					changeAdjacentSight(pLoopUnit->getTeam(), iRange, bIncrement,
							pLoopUnit, bUpdatePlotGroups);
				}
			}
		}
	}
}

// <advc.003h> Cut and pasted from CvPlot::updateSeeFromSight
void CvPlot::setMaxVisibilityRangeCache()
{
	int iRange = GC.getDefineINT(CvGlobals::UNIT_VISIBILITY_RANGE) + 1;
	for(int iPromotion = 0; iPromotion < GC.getNumPromotionInfos(); iPromotion++)
		iRange += GC.getInfo((PromotionTypes)iPromotion).getVisibilityChange();
	iRange = std::max(GC.getDefineINT(CvGlobals::RECON_VISIBILITY_RANGE) + 1, iRange);
	iMaxVisibilityRangeCache = iRange;
} // </advc.003h>


void CvPlot::updateSeeFromSight(bool bIncrement, bool bUpdatePlotGroups)
{
	int const iRange = iMaxVisibilityRangeCache; // advc.003h
	for (int iDX = -iRange; iDX <= iRange; iDX++)
	{
		for (int iDY = -iRange; iDY <= iRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX(), getY(), iDX, iDY);
			if (pLoopPlot != NULL)
				pLoopPlot->updateSight(bIncrement, bUpdatePlotGroups);
		}
	}
}


bool CvPlot::canHaveBonus(BonusTypes eBonus, bool bIgnoreLatitude,
		bool bIgnoreFeature) const // advc.129
{
	if (eBonus == NO_BONUS)
		return true;

	if (getBonusType() != NO_BONUS)
		return false;

	if (isPeak())
		return false;

	CvBonusInfo const& kBonus = GC.getInfo(eBonus);
	// <advc.129>
	if(bIgnoreFeature)
	{
		if(!kBonus.isFeatureTerrain(getTerrainType()) &&
				!kBonus.isTerrain(getTerrainType()))
			return false;
	}
	else /* </advc.129> */ if (isFeature())
	{
		if (!kBonus.isFeature(getFeatureType()))
			return false;
		if (!kBonus.isFeatureTerrain(getTerrainType()))
			return false;
	}
	else if (!kBonus.isTerrain(getTerrainType()))
		return false;

	if (isHills())
	{
		if (!kBonus.isHills())
			return false;
	}
	else if (isFlatlands())
	{
		if (!kBonus.isFlatlands())
			return false;
	}

	if (kBonus.isNoRiverSide() && isRiverSide())
		return false;

	if (kBonus.getMinAreaSize() != -1)
	{
		if (area()->getNumTiles() < kBonus.getMinAreaSize())
			return false;
	}

	if (!bIgnoreLatitude)
	{
		if (getLatitude() > kBonus.getMaxLatitude())
			return false;

		if (getLatitude() < kBonus.getMinLatitude())
			return false;
	}

	if (!isPotentialCityWork())
		return false;

	return true;
}


bool CvPlot::canHaveImprovement(ImprovementTypes eImprovement, TeamTypes eTeam, bool bPotential,
	BuildTypes eBuild, bool bAnyBuild) const // dlph.9  advc: some style changes
{
	/*  K-Mod, 21/dec/10, karadoc
		changed to check for NO_IMPROVEMENT rather than just assume the input is an actual improvement */
	// FAssertMsg(eImprovement != NO_IMPROVEMENT, "Improvement is not assigned a valid value");
	if (eImprovement == NO_IMPROVEMENT)
		return true;
	// K-Mod end

	// <dlph.9>
	FAssertMsg(!bAnyBuild || eBuild == NO_BUILD,
			"expected: if bAnyBuild is true then eBuild is NO_BUILD");
	FAssertMsg(eBuild == NO_BUILD ||
			GC.getInfo(eBuild).getImprovement() == eImprovement,
			"expected that eBuild matches eImprovement");
	// </dlph.9>

	if (isCity())
		return false;

	if (isImpassable())
		return false;

	if (GC.getInfo(eImprovement).isWater() != isWater())
		return false;

	if (isFeature())
	{
		if (GC.getInfo(getFeatureType()).isNoImprovement())
			return false;
	}

	if (getBonusType(eTeam) != NO_BONUS && GC.getInfo(eImprovement).isImprovementBonusMakesValid(getBonusType(eTeam)))
		return true;

	if (GC.getInfo(eImprovement).isNoFreshWater() && isFreshWater())
		return false;

	if (GC.getInfo(eImprovement).isRequiresFlatlands() && !isFlatlands())
		return false;

	if (GC.getInfo(eImprovement).isRequiresFeature() && !isFeature())
		return false;
	{
		bool bValid = false;
		if (GC.getInfo(eImprovement).isHillsMakesValid() && isHills())
			bValid = true;
		if (GC.getInfo(eImprovement).isFreshWaterMakesValid() && isFreshWater())
			bValid = true;
		if (GC.getInfo(eImprovement).isRiverSideMakesValid() && isRiverSide())
			bValid = true;
		if (GC.getInfo(eImprovement).getTerrainMakesValid(getTerrainType()))
			bValid = true;
		if (isFeature() && GC.getInfo(eImprovement).getFeatureMakesValid(getFeatureType()))
			bValid = true;
		if (!bValid)
			return false;
	}
	if (GC.getInfo(eImprovement).isRequiresRiverSide())
	{
		bool bValid = false;
		for (int iI = 0; iI < NUM_CARDINALDIRECTION_TYPES; ++iI)
		{
			CvPlot* pLoopPlot = ::plotCardinalDirection(getX(), getY(), (CardinalDirectionTypes)iI);
			if (pLoopPlot == NULL)
				continue;

			if (isRiverCrossing(::directionXY(this, pLoopPlot)))
			{
				if (pLoopPlot->getImprovementType() != eImprovement)
				{
					bValid = true;
					break;
				}
			}
		}
		if (!bValid)
			return false;
	}

	/*for (iI = 0; iI < NUM_YIELD_TYPES; ++iI) {
		if (calculateNatureYield(((YieldTypes)iI), eTeam) < GC.getInfo(eImprovement).getPrereqNatureYield(iI))
			return false;
	}*/
	// <dlph.9> Replacing the above
	bool bFound = false;
	bool bBuildable = false;
	if (eBuild == NO_BUILD && !bAnyBuild)
	{
		for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
		{
			if (calculateNatureYield(((YieldTypes)iI), eTeam) <
					GC.getInfo(eImprovement).getPrereqNatureYield(iI))
				return false;
		}
	}
	else if (eBuild != NO_BUILD)
	{
		for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
		{
			if (calculateNatureYield(((YieldTypes)iI), eTeam, !isFeature() ||
				GC.getInfo(eBuild).isFeatureRemove(getFeatureType())) <
				GC.getInfo(eImprovement).getPrereqNatureYield(iI))
			{
				return false;
			}
		}
	}
	else
	{
		for (int i = 0; i < GC.getNumBuildInfos(); ++i)
		{
			CvBuildInfo& kBuild = GC.getInfo((BuildTypes)i);
			if (kBuild.getImprovement() == eImprovement)
			{
				bBuildable = true;
				bool bValid = true;
				for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
				{
					if (calculateNatureYield(((YieldTypes)iI), eTeam, !isFeature() ||
						kBuild.isFeatureRemove(getFeatureType())) <
						GC.getInfo(eImprovement).getPrereqNatureYield(iI))
					{
						bValid = false;
						break;
					}
				}
				if (bValid)
				{
					bFound = true;
					break;
				}
			}
		}

		if (bBuildable && !bFound)
			return false;
 	} // </dlph.9>

	if (getTeam() == NO_TEAM || !GET_TEAM(getTeam()).isIgnoreIrrigation())
	{
		if (!bPotential && GC.getInfo(eImprovement).isRequiresIrrigation() && !isIrrigationAvailable())
			return false;
	}

	return true;
}


bool CvPlot::canBuild(BuildTypes eBuild, PlayerTypes ePlayer, bool bTestVisible) const
{
	if (eBuild == NO_BUILD)
		return false;
	{
		bool bPyOverride=false;
		bool r = GC.getPythonCaller()->canBuild(*this, eBuild, ePlayer, bPyOverride);
		if (bPyOverride)
			return r;
	}
	bool bValid = false;
	ImprovementTypes eImprovement = (ImprovementTypes)GC.getInfo(eBuild).getImprovement();
	if (eImprovement != NO_IMPROVEMENT)
	{
		if (!canHaveImprovement(eImprovement, GET_PLAYER(ePlayer).getTeam(), bTestVisible,
				eBuild, false)) // dlph.9
			return false;

		if (getImprovementType() != NO_IMPROVEMENT)
		{
			if (GC.getInfo(getImprovementType()).isPermanent())
			{
				return false;
			}

			if (getImprovementType() == eImprovement)
			{
				return false;
			}

			ImprovementTypes eFinalImprovementType = finalImprovementUpgrade(getImprovementType());

			if (eFinalImprovementType != NO_IMPROVEMENT)
			{
				if (eFinalImprovementType == finalImprovementUpgrade(eImprovement))
				{
					return false;
				}
			}
		}

		if (!bTestVisible)
		{
			if (GET_PLAYER(ePlayer).getTeam() != getTeam())
			{
				//outside borders can't be built in other's culture
				if (GC.getInfo(eImprovement).isOutsideBorders())
				{
					if (getTeam() != NO_TEAM)
					{
						return false;
					}
				}
				else //only buildable in own culture
				{
					return false;
				}
			}
		}

		bValid = true;
	}

	RouteTypes eRoute = ((RouteTypes)(GC.getInfo(eBuild).getRoute()));

	if (eRoute != NO_ROUTE)
	{
		if (getRouteType() != NO_ROUTE)
		{
			if (GC.getInfo(getRouteType()).getValue() >= GC.getInfo(eRoute).getValue())
			{
				return false;
			}
		}

		if (!bTestVisible)
		{
			if (GC.getInfo(eRoute).getPrereqBonus() != NO_BONUS)
			{
				if (!isAdjacentPlotGroupConnectedBonus(ePlayer, ((BonusTypes)(GC.getInfo(eRoute).getPrereqBonus()))))
				{
					return false;
				}
			}

			bool bFoundValid = true;
			for (int i = 0; i < GC.getNUM_ROUTE_PREREQ_OR_BONUSES(); ++i)
			{
				if (NO_BONUS != GC.getInfo(eRoute).getPrereqOrBonus(i))
				{
					bFoundValid = false;

					if (isAdjacentPlotGroupConnectedBonus(ePlayer, ((BonusTypes)(GC.getInfo(eRoute).getPrereqOrBonus(i)))))
					{
						bFoundValid = true;
						break;
					}
				}
			}

			if (!bFoundValid)
			{
				return false;
			}
		}

		bValid = true;
	}

	if (isFeature())
	{
		if (GC.getInfo(eBuild).isFeatureRemove(getFeatureType()))
		{	/*if (isOwned() && (GET_PLAYER(ePlayer).getTeam() != getTeam()) && !atWar(GET_PLAYER(ePlayer).getTeam(), getTeam()))
				return false;
			bValid = true;*/
			// <advc.119> Replacing the above
			if(getTeam() == TEAMID(ePlayer))
				bValid = true;
			else return false; // </advc.119>
		}
	}

	return bValid;
}


int CvPlot::getBuildTime(BuildTypes eBuild, /* advc.251: */ PlayerTypes ePlayer) const
{
	int iTime = GC.getInfo(eBuild).getTime();
	if (isFeature())
		iTime += GC.getInfo(eBuild).getFeatureTime(getFeatureType());

	iTime *= std::max(0, (GC.getInfo(getTerrainType()).getBuildModifier() + 100));
	iTime /= 100;
	// <advc.251>
	iTime = (int)(GC.getInfo(GET_PLAYER(ePlayer).getHandicapType()).
			getBuildTimePercent() * 0.01 * iTime);
	iTime -= (iTime % 50); // Round down to a multiple of 50
	// </advc.251>
	iTime *= GC.getInfo(GC.getGame().getGameSpeedType()).getBuildPercent();
	iTime /= 100;

	iTime *= GC.getInfo(GC.getGame().getStartEra()).getBuildPercent();
	iTime /= 100;

	return iTime;
}


int CvPlot::getBuildTurnsLeft(BuildTypes eBuild, /* advc.251: */ PlayerTypes ePlayer,
	int iNowExtra, int iThenExtra, /* advc.011c: */ bool bIncludeUnits) const
{
	int iNowBuildRate = iNowExtra;
	int iThenBuildRate = iThenExtra;

	if (bIncludeUnits) // advc.011c
	{
		for (CLLNode<IDInfo> const* pUnitNode = headUnitNode(); pUnitNode != NULL;
			pUnitNode = nextUnitNode(pUnitNode))
		{
			CvUnit const* pLoopUnit = ::getUnit(pUnitNode->m_data);
			if (pLoopUnit->getBuildType() == eBuild)
			{
				if (pLoopUnit->canMove())
					iNowBuildRate += pLoopUnit->workRate(false);
				iThenBuildRate += pLoopUnit->workRate(true);
			}
		}
	}
	if (iThenBuildRate == 0)
	{
		//this means it will take forever under current circumstances
		return MAX_INT;
	}

	int iBuildLeft = getBuildTime(eBuild, /* advc.251: */ ePlayer);
	iBuildLeft -= getBuildProgress(eBuild);
	iBuildLeft -= iNowBuildRate;

	iBuildLeft = std::max(0, iBuildLeft);

	int iTurnsLeft = (iBuildLeft / iThenBuildRate);
	if (iTurnsLeft * iThenBuildRate < iBuildLeft)
		iTurnsLeft++;
	iTurnsLeft++;

	return std::max(1, iTurnsLeft);
}

// <advc.011c>
int CvPlot::getBuildTurnsLeft(BuildTypes eBuild, PlayerTypes ePlayer) const
{
	int iWorkRate = GET_PLAYER(ePlayer).getWorkRate(eBuild);
	if(iWorkRate > 0)
	{
		return getBuildTurnsLeft(eBuild, /* advc.251: */ ePlayer,
				iWorkRate, iWorkRate, false);
	}
	else return MAX_INT;
} // </advc.011c>


int CvPlot::getFeatureProduction(BuildTypes eBuild, TeamTypes eTeam, CvCity** ppCity) const
{
	if (!isFeature())
		return 0;

	*ppCity = getWorkingCity();

	if (*ppCity == NULL)
		*ppCity = GC.getMap().findCity(getX(), getY(), NO_PLAYER, eTeam, false);

	if (*ppCity == NULL)
		return 0;

	int iProduction = (GC.getInfo(eBuild).getFeatureProduction(getFeatureType()) - (std::max(0, (plotDistance(getX(), getY(), (*ppCity)->getX(), (*ppCity)->getY()) - 2)) * 5));

	iProduction *= std::max(0, (GET_PLAYER((*ppCity)->getOwner()).getFeatureProductionModifier() + 100));
	iProduction /= 100;

	iProduction *= GC.getInfo(GC.getGame().getGameSpeedType()).getFeatureProductionPercent();
	iProduction /= 100;
	// <advc.opt>
	static int const iBASE_FEATURE_PRODUCTION_PERCENT = GC.getDefineINT("BASE_FEATURE_PRODUCTION_PERCENT");
	static int const iFEATURE_PRODUCTION_PERCENT_MULTIPLIER = GC.getDefineINT("FEATURE_PRODUCTION_PERCENT_MULTIPLIER");
	static int const iDIFFERENT_TEAM_FEATURE_PRODUCTION_PERCENT = GC.getDefineINT("DIFFERENT_TEAM_FEATURE_PRODUCTION_PERCENT");
	// </advc.opt>
	iProduction *= std::min(100, iBASE_FEATURE_PRODUCTION_PERCENT +
			iFEATURE_PRODUCTION_PERCENT_MULTIPLIER * (*ppCity)->getPopulation());
	iProduction /= 100;

	if (getTeam() != eTeam)
	{
		iProduction *= iDIFFERENT_TEAM_FEATURE_PRODUCTION_PERCENT;
		iProduction /= 100;
	}

	return std::max(0, iProduction);
}


CvUnit* CvPlot::getBestDefender(PlayerTypes eOwner, PlayerTypes eAttackingPlayer,
	CvUnit const* pAttacker, bool bTestEnemy, bool bTestPotentialEnemy,
	/* advc.028: */ bool bTestVisible,
	bool bTestCanAttack, bool bAny) const // advc: new params (for CvPlot::hasDefender)
{
	// <advc> Ensure consistency of parameters
	if (pAttacker != NULL)
	{
		FAssert(pAttacker->getOwner() == eAttackingPlayer);
		eAttackingPlayer = pAttacker->getOwner();
	}
	// isEnemy implies isPotentialEnemy
	FAssert(!bTestEnemy || !bTestPotentialEnemy); // </advc>
	// BETTER_BTS_AI_MOD, Lead From Behind (UncutDragon), 02/21/10, jdog5000
	int iBestUnitRank = -1;
	CvUnit* pBestUnit = NULL;
	for (CLLNode<IDInfo> const* pUnitNode = headUnitNode(); pUnitNode != NULL; pUnitNode = nextUnitNode(pUnitNode)) // advc: while loop replaced
	{
		CvUnit& kLoopUnit = *::getUnit(pUnitNode->m_data);
		if (eOwner != NO_PLAYER && kLoopUnit.getOwner() != eOwner)
			continue;
		if (kLoopUnit.isCargo()) // advc: Was previously only checked with bTestCanMove - i.e. never.
			continue;
		// <advc> Moved the other conditions into an auxiliary function
		if(eAttackingPlayer == NO_PLAYER || kLoopUnit.canBeAttackedBy(eAttackingPlayer,
			pAttacker, bTestEnemy, bTestPotentialEnemy, /* advc.028: */ bTestVisible,
			bTestCanAttack))
		{
			if (bAny)
				return &kLoopUnit; // </advc>
			if (kLoopUnit.isBetterDefenderThan(pBestUnit, pAttacker,
					&iBestUnitRank, // UncutDragon
					bTestVisible)) // advc.061
				pBestUnit = &kLoopUnit;
		}
	}
	// BETTER_BTS_AI_MOD: END
	return pBestUnit;
}


CvUnit* CvPlot::getSelectedUnit() const
{
	for (CLLNode<IDInfo> const* pUnitNode = headUnitNode(); pUnitNode != NULL;
		pUnitNode = nextUnitNode(pUnitNode))
	{
		CvUnit* pLoopUnit = ::AI_getUnit(pUnitNode->m_data);
		if (pLoopUnit->IsSelected())
			return pLoopUnit;
	}
	return NULL;
}


int CvPlot::getUnitPower(PlayerTypes eOwner) const
{
	int iCount = 0;
	for (CLLNode<IDInfo> const* pUnitNode = headUnitNode(); pUnitNode != NULL;
		pUnitNode = nextUnitNode(pUnitNode))
	{
		CvUnit const* pLoopUnit = ::getUnit(pUnitNode->m_data);
		if (eOwner == NO_PLAYER || pLoopUnit->getOwner() == eOwner)
			iCount += pLoopUnit->getUnitInfo().getPowerValue();
	}
	return iCount;
}


int CvPlot::defenseModifier(TeamTypes eDefender, bool bIgnoreBuilding,
	TeamTypes eAttacker, // advc.012
	bool bHelp, /* advc.500b: */ bool bGarrisonStrength) const
{
	int iModifier = GC.getInfo(getTerrainType()).getDefenseModifier();
	FeatureTypes eFeature = getFeatureType();
	if(eFeature != NO_FEATURE)
	{
		iModifier += GC.getInfo(eFeature).getDefenseModifier();
		// <advc.012>
		if(eAttacker == NO_TEAM  || getTeam() != eAttacker)
			iModifier += GC.getInfo(eFeature).getRivalDefenseModifier();
		// </advc.012>
	}
	if (isHills())
		iModifier += GC.getDefineINT(CvGlobals::HILLS_EXTRA_DEFENSE);
	ImprovementTypes eImprovement;
	if (bHelp)
		eImprovement = getRevealedImprovementType(GC.getGame().getActiveTeam());
	else eImprovement = getImprovementType();

	if (eImprovement != NO_IMPROVEMENT)
	{
		if (eDefender != NO_TEAM && (getTeam() == NO_TEAM || GET_TEAM(eDefender).isFriendlyTerritory(getTeam())))
			iModifier += GC.getInfo(eImprovement).getDefenseModifier();
	}

	if (!bHelp)
	{
		CvCity* pCity = getPlotCity();
		if (pCity != NULL)
		{	// <advc.500b>
			if (bGarrisonStrength)
				iModifier += pCity->getBuildingDefense();
			else // </advc.500b>
				iModifier += pCity->getDefenseModifier(bIgnoreBuilding);
		}
	}

	return iModifier;
}


int CvPlot::movementCost(const CvUnit* pUnit, const CvPlot* pFromPlot,
	bool bAssumeRevealed) const // advc.001i
{
	// <advc.162>
	if(pUnit->isInvasionMove(*pFromPlot, *this))
		return pUnit->movesLeft(); // </advc.162>

	if (pUnit->flatMovementCost() || (pUnit->getDomainType() == DOMAIN_AIR))
		return GC.getMOVE_DENOMINATOR();

	/*if (pUnit->isHuman()) {
		if (!isRevealed(pUnit->getTeam(), false))
			return pUnit->maxMoves();
	}*/
	// K-Mod. Why let the AI cheat this?
	if ( /* advc.001i: The K-Mod condition is OK, but now that the pathfinder passes
			bAssumeRevealed=false, it's cleaner to check that too. */
		!bAssumeRevealed &&
		!isRevealed(pUnit->getTeam()))
	{
		/*if (!pFromPlot->isRevealed(pUnit->getTeam(), false))
			return pUnit->maxMoves();
		else return GC.getMOVE_DENOMINATOR() + 1;
		*/ // (further weight adjustments are now done in the pathfinder's moveCost function.)
		return GC.getMOVE_DENOMINATOR();
	} // K-Mod end

	if (!pFromPlot->isValidDomainForLocation(*pUnit))
		return pUnit->maxMoves();

	if (!isValidDomainForAction(*pUnit))
		return GC.getMOVE_DENOMINATOR();

	FAssert(pUnit->getDomainType() != DOMAIN_IMMOBILE);

	int iRegularCost;
	if (pUnit->ignoreTerrainCost())
		iRegularCost = 1;
	else
	{
		iRegularCost = (!isFeature() ?
				GC.getInfo(getTerrainType()).getMovementCost() :
				GC.getInfo(getFeatureType()).getMovementCost());
		if (isHills())
			iRegularCost += GC.getDefineINT(CvGlobals::HILLS_EXTRA_MOVEMENT);

		if (iRegularCost > 0)
			iRegularCost = std::max(1, iRegularCost - pUnit->getExtraMoveDiscount());
	}

	bool bHasTerrainCost = (iRegularCost > 1);
	iRegularCost = std::min(iRegularCost, pUnit->baseMoves());
	iRegularCost *= GC.getMOVE_DENOMINATOR();
	if (bHasTerrainCost)
	{
		if ((!isFeature() ? pUnit->isTerrainDoubleMove(getTerrainType()) :
			pUnit->isFeatureDoubleMove(getFeatureType())) ||
			(isHills() && pUnit->isHillsDoubleMove()))
		{
			iRegularCost /= 2;
		}
	}
	int iRouteCost, iRouteFlatCost;
	// <advc.001i> Pass along bAssumeRevealed
	if (pFromPlot->isValidRoute(pUnit, bAssumeRevealed) &&
		isValidRoute(pUnit, bAssumeRevealed) && // </advc.001i>
		(GET_TEAM(pUnit->getTeam()).isBridgeBuilding() ||
		!pFromPlot->isRiverCrossing(directionXY(pFromPlot, this))))
	{	// <advc.001i>
		RouteTypes eFromRoute = (bAssumeRevealed ? pFromPlot->getRouteType() :
				pFromPlot->getRevealedRouteType(pUnit->getTeam()));
		CvRouteInfo const& kFromRoute = GC.getInfo(eFromRoute);
		RouteTypes eToRoute = (bAssumeRevealed ? getRouteType() :
				getRevealedRouteType(pUnit->getTeam()));
		CvRouteInfo const& kToRoute = GC.getInfo(eToRoute);
		iRouteCost = std::max(
				kFromRoute.getMovementCost() +
				GET_TEAM(pUnit->getTeam()).getRouteChange(eFromRoute),
				kToRoute.getMovementCost() +
				GET_TEAM(pUnit->getTeam()).getRouteChange(eToRoute));
		// </advc.001i>
		iRouteFlatCost = std::max(
				kFromRoute.getFlatMovementCost() * pUnit->baseMoves(),
				kToRoute.getFlatMovementCost() * pUnit->baseMoves());
	}
	else
	{
		iRouteCost = MAX_INT;
		iRouteFlatCost = MAX_INT;
	}

	return std::max(1, std::min(iRegularCost, std::min(iRouteCost, iRouteFlatCost)));
}


int CvPlot::getExtraMovePathCost() const
{
	return GC.getGame().getPlotExtraCost(getX(), getY());
}


void CvPlot::changeExtraMovePathCost(int iChange)
{
	GC.getGame().changePlotExtraCost(getX(), getY(), iChange);
}


bool CvPlot::isAdjacentOwned() const
{
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), ((DirectionTypes)iI));
		if (pAdjacentPlot != NULL)
		{
			if (pAdjacentPlot->isOwned())
				return true;
		}
	}
	return false;
}


bool CvPlot::isAdjacentPlayer(PlayerTypes ePlayer, bool bLandOnly) const
{
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
		if (pAdjacentPlot != NULL)
		{
			if (pAdjacentPlot->getOwner() == ePlayer)
			{
				if (!bLandOnly || !(pAdjacentPlot->isWater()))
					return true;
			}
		}
	}

	return false;
}


bool CvPlot::isAdjacentTeam(TeamTypes eTeam, bool bLandOnly) const
{
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), ((DirectionTypes)iI));
		if (pAdjacentPlot != NULL)
		{
			if (pAdjacentPlot->getTeam() == eTeam)
			{
				if (!bLandOnly || !(pAdjacentPlot->isWater()))
					return true;
			}
		}
	}

	return false;
}


bool CvPlot::isWithinCultureRange(PlayerTypes ePlayer) const
{
	FOR_EACH_ENUM(CultureLevel)
	{
		if (isCultureRangeCity(ePlayer, eLoopCultureLevel))
			return true;
	}

	return false;
}


int CvPlot::getNumCultureRangeCities(PlayerTypes ePlayer) const
{
	int iCount = 0;
	FOR_EACH_ENUM(CultureLevel)
		iCount += getCultureRangeCities(ePlayer, eLoopCultureLevel);
	return iCount;
}

// BETTER_BTS_AI_MOD, General AI, 01/10/10, jdog5000: START
bool CvPlot::isHasPathToEnemyCity(TeamTypes eAttackerTeam, bool bIgnoreBarb) /* advc: */ const  // (and some minor style changes)
{
	PROFILE_FUNC();

	FAssert(eAttackerTeam != NO_TEAM);

	if (area()->getNumCities() - GET_TEAM(eAttackerTeam).countNumCitiesByArea(area()) == 0)
		return false;

	// Imitate instatiation of irrigated finder, pIrrigatedFinder
	// Can't mimic step finder initialization because it requires creation from the exe
	std::vector<TeamTypes> aeTeams;
	aeTeams.push_back(eAttackerTeam);
	aeTeams.push_back(NO_TEAM);
	FAStar* pTeamStepFinder = gDLL->getFAStarIFace()->create();
	CvMap const& m = GC.getMap();
	gDLL->getFAStarIFace()->Initialize(pTeamStepFinder, m.getGridWidth(), m.getGridHeight(),
			m.isWrapX(), m.isWrapY(), stepDestValid, stepHeuristic,
			stepCost, teamStepValid, stepAdd, NULL, NULL);
	gDLL->getFAStarIFace()->SetData(pTeamStepFinder, &aeTeams);

	bool bFound = false;
	// First check capitals
	for (int iI = 0; !bFound && iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive() && GET_TEAM(eAttackerTeam).AI_getWarPlan(GET_PLAYER((PlayerTypes)iI).getTeam()) != NO_WARPLAN)
		{
			if (!bIgnoreBarb || !(GET_PLAYER((PlayerTypes)iI).isBarbarian() || GET_PLAYER((PlayerTypes)iI).isMinorCiv()))
			{
				CvCity* pLoopCity = GET_PLAYER((PlayerTypes)iI).getCapitalCity();
				if (pLoopCity == NULL)
					continue;
				if (pLoopCity->area() == area())
				{
					bFound = gDLL->getFAStarIFace()->GeneratePath(pTeamStepFinder,
							getX(), getY(),
							pLoopCity->getX(), pLoopCity->getY(),
							false, 0, true);
					if (bFound)
						break;
				}
			}
		}
	}

	// Check all other cities
	for (int iI = 0; !bFound && iI < MAX_PLAYERS; iI++)
	{
		CvPlayer const& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.isAlive() && GET_TEAM(eAttackerTeam).AI_getWarPlan(GET_PLAYER((PlayerTypes)iI).getTeam()) != NO_WARPLAN)
		{
			if (!bIgnoreBarb || !(kLoopPlayer.isBarbarian() || kLoopPlayer.isMinorCiv()))
			{
				FOR_EACH_CITY(pLoopCity, kLoopPlayer)
				{
					if (pLoopCity->area() == area() && !pLoopCity->isCapital())
					{
						bFound = gDLL->getFAStarIFace()->GeneratePath(pTeamStepFinder,
								getX(), getY(), pLoopCity->getX(), pLoopCity->getY(),
								false, 0, true);
						if (bFound)
							break;
					}
				}
			}
		}
	}

	gDLL->getFAStarIFace()->destroy(pTeamStepFinder);

	return bFound;
}

bool CvPlot::isHasPathToPlayerCity(TeamTypes eMoveTeam, PlayerTypes eOtherPlayer) /* advc: */ const  // (and some minor style changes)
{
	PROFILE_FUNC();

	FAssert(eMoveTeam != NO_TEAM);

	if (area()->getCitiesPerPlayer(eOtherPlayer) == 0)
		return false;

	// Imitate instatiation of irrigated finder, pIrrigatedFinder
	// Can't mimic step finder initialization because it requires creation from the exe
	std::vector<TeamTypes> aeTeams;
	aeTeams.push_back(eMoveTeam);
	aeTeams.push_back(GET_PLAYER(eOtherPlayer).getTeam());
	FAStar* pTeamStepFinder = gDLL->getFAStarIFace()->create();
	CvMap const& m = GC.getMap();
	gDLL->getFAStarIFace()->Initialize(pTeamStepFinder, m.getGridWidth(), m.getGridHeight(),
			m.isWrapX(), m.isWrapY(), stepDestValid, stepHeuristic,
			stepCost, teamStepValid, stepAdd, NULL, NULL);
	gDLL->getFAStarIFace()->SetData(pTeamStepFinder, &aeTeams);

	bool bFound = false;
	FOR_EACH_CITY(pLoopCity, GET_PLAYER(eOtherPlayer))
	{
		if (pLoopCity->area() == area())
		{
			bFound = gDLL->getFAStarIFace()->GeneratePath(pTeamStepFinder, getX(), getY(),
					pLoopCity->getX(), pLoopCity->getY(), false, 0, true);
			if (bFound)
				break;
		}
	}

	gDLL->getFAStarIFace()->destroy(pTeamStepFinder);

	return bFound;
}

/*  advc.104b (comment): This BBAI function was previously unused, so I'm free
	to twist it to my purpose. I don't think it had ever been tested either
	b/c it didn't seem to work at all until I changed the GetLastNode call
	at the end. */
int CvPlot::calculatePathDistanceToPlot(TeamTypes eTeam, CvPlot const& kTargetPlot,
	TeamTypes eTargetTeam, DomainTypes eDomain, int iMaxPath) const // advc.104b
{
	PROFILE_FUNC(); // advc: The time is mostly spent in teamStepValid_advc
	FAssert(eTeam != NO_TEAM);
	FAssert(eTargetTeam != NO_TEAM);
	/*  advc.104b: Commented out. Want to be able to measure paths between
		coastal cities of different continents. (And shouldn't return "false"
		at any rate.) */
	/*if (pTargetPlot->area() != area())
		return false;*/

	// Imitate instatiation of irrigated finder, pIrrigatedFinder
	// Can't mimic step finder initialization because it requires creation from the exe
	/*  <advc.104b> vector type changed to int[]; dom, eTargetTeam (instead of
		NO_TEAM), iMaxPath and target coordinates added. */
	int aStepData[6] = {0};
	aStepData[0] = eTeam;
	aStepData[1] = eTargetTeam;
	FAssert(eDomain != NO_DOMAIN); // (or could treat it as DOMAIN_LAND)
	aStepData[2] = eDomain;
	aStepData[3] = kTargetPlot.getX();
	aStepData[4] = kTargetPlot.getY();
	aStepData[5] = iMaxPath; // </advc.104b>
	FAStar* pStepFinder = gDLL->getFAStarIFace()->create();
	gDLL->getFAStarIFace()->Initialize(pStepFinder,
			GC.getMap().getGridWidth(),
			GC.getMap().getGridHeight(),
			GC.getMap().isWrapX(),
			GC.getMap().isWrapY(),
			// advc.104b: Plugging in _advc functions
			stepDestValid_advc, stepHeuristic, stepCost, teamStepValid_advc, stepAdd,
			NULL, NULL);
	gDLL->getFAStarIFace()->SetData(pStepFinder, aStepData);

	int iPathDistance = -1;
	gDLL->getFAStarIFace()->GeneratePath(pStepFinder, getX(), getY(),
			kTargetPlot.getX(), kTargetPlot.getY(), false, 0, true);
	// advc.104b, advc.001: was &GC.getStepFinder() instead of pStepFinder
	FAStarNode* pNode = gDLL->getFAStarIFace()->GetLastNode(pStepFinder);
	if (pNode != NULL)
		iPathDistance = pNode->m_iData1;

	gDLL->getFAStarIFace()->destroy(pStepFinder);

	return iPathDistance;
}
// BETTER_BTS_AI_MOD: END

// K-Mod. (rewrite of a bbai function)
// I've changed the purpose of this function - because this is the way it is always used.
void CvPlot::invalidateBorderDangerCache()
{
	/* for (int iI = 0; iI < MAX_TEAMS; iI++)
		m_abBorderDangerCache[iI] = false;*/
	for (int iDX = -BORDER_DANGER_RANGE; iDX <= BORDER_DANGER_RANGE; iDX++)
	{
		for (int iDY = -BORDER_DANGER_RANGE; iDY <= BORDER_DANGER_RANGE; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX(), getY(), iDX, iDY);
			if (pLoopPlot)
			{
				for (TeamTypes i = (TeamTypes)0; i < MAX_TEAMS; i = (TeamTypes)(i+1))
				{
					pLoopPlot->setBorderDangerCache(i, false);
				}
			}
		}
	}
}
// K-Mod end

PlayerTypes CvPlot::calculateCulturalOwner(/* advc.099c: */ bool bIgnoreCultureRange,
	bool bOwnExclusiveRadius) const // advc.035
{
	PROFILE_FUNC();
	int iI;
	/*  advc.001: When a city is captured, the tiles in its culture range (but I
		think not the city plot itself) are set to unowned for 2 turns. This leads
		to 0% revolt chance for a turn or two when a city is razed and a new city
		is immediately founded near the ruins. Adding an isCity check to avoid confusion. */
	if(!isCity() && isForceUnowned())
		return NO_PLAYER;

	// <advc.035>
	bool abCityRadius[MAX_PLAYERS] = { false };
	bool bAnyCityRadius = false;
	if(bOwnExclusiveRadius)
	{
		std::vector<CvPlot*> aCross;
		::cityCross(*this, aCross);
		for(size_t i = 1; i < aCross.size(); i++)
		{
			if(aCross[i] == NULL) continue;
			CvPlot const& p = *aCross[i];
			if(!p.isCity() || p.getPlotCity()->isOccupation())
				continue;
			PlayerTypes eCityOwner = p.getPlotCity()->getOwner();
			if(isWithinCultureRange(eCityOwner))
			{
				abCityRadius[eCityOwner] = true;
				bAnyCityRadius = true;
			}
		}
	} // </advc.035>

	int iBestCulture = 0;
	PlayerTypes eBestPlayer = NO_PLAYER;
	for (iI = 0; iI < MAX_PLAYERS; ++iI)
	{	// <advc.035>
		if(bOwnExclusiveRadius && bAnyCityRadius && !abCityRadius[iI])
			continue; // </advc.035>
		PlayerTypes eLoopPlayer = (PlayerTypes)iI;
		if(GET_PLAYER(eLoopPlayer).isAlive() /* advc.099c: */ || bIgnoreCultureRange)
		{
			int iCulture = getCulture(eLoopPlayer);
			/*  <advc.035> When the range of a city expands, tile ownership is updated
				before tile culture is spread. If the expansion is sudden (WorldBuilder,
				perhaps also culture bomb), 0 tile culture in the new range is possible. */
			if (bOwnExclusiveRadius && iBestCulture == 0 && abCityRadius[iI])
				iBestCulture = -1;
			if (iCulture <= iBestCulture) // </advc.035>
				continue; // advc
			if (/* advc.099c: */ bIgnoreCultureRange ||
				isWithinCultureRange(eLoopPlayer))
			{
				if (iCulture > iBestCulture || (iCulture == iBestCulture && getOwner() == eLoopPlayer))
				{
					iBestCulture = iCulture;
					eBestPlayer = eLoopPlayer;
				}
			}
		}
	}

	if (!isCity() && eBestPlayer != NO_PLAYER)
	{
		int iBestPriority = MAX_INT;
		CvCity* pBestCity = NULL;
		for (iI = 0; iI < NUM_CITY_PLOTS; iI++)  // advc: reduced indentation
		{
			CvPlot* pLoopPlot = plotCity(getX(), getY(), iI);
			if(pLoopPlot == NULL)
				continue;
			CvCity* pLoopCity = pLoopPlot->getPlotCity();
			if(pLoopCity == NULL)
				continue;
			if(pLoopCity->getTeam() != TEAMID(eBestPlayer) &&
					!GET_TEAM(eBestPlayer).isVassal(pLoopCity->getTeam()))
				continue;
			if(getCulture(pLoopCity->getOwner()) <= 0)
				continue;
			if (!bIgnoreCultureRange && /* advc.099c: 099c cares only about
					city tile culture, but for consistency, I'm also implementing
					the IgnoreCultureRange switch for non-city tiles. */
					!isWithinCultureRange(pLoopCity->getOwner()))
				continue;

			int iPriority = GC.getCityPlotPriority()[iI];
			if (pLoopCity->getTeam() == TEAMID(eBestPlayer))
				iPriority += 5; // priority ranges from 0 to 4 -> give priority to Masters of a Vassal
			if (iPriority < iBestPriority || (iPriority == iBestPriority &&
					pLoopCity->getOwner() == eBestPlayer))
			{
				iBestPriority = iPriority;
				pBestCity = pLoopCity;
			}
		}

		if(pBestCity != NULL)
			eBestPlayer = pBestCity->getOwner();
	}

	if(eBestPlayer != NO_PLAYER)
		return eBestPlayer; // advc

	bool bValid = true;
	for (iI = 0; iI < NUM_CARDINALDIRECTION_TYPES; ++iI)
	{
		CvPlot* pLoopPlot = plotCardinalDirection(getX(), getY(), ((CardinalDirectionTypes)iI));
		if (pLoopPlot == NULL)
			continue;
		if (pLoopPlot->isOwned())
		{
			if (eBestPlayer == NO_PLAYER)
				eBestPlayer = pLoopPlot->getOwner();
			else if (eBestPlayer != pLoopPlot->getOwner())
			{
				bValid = false;
				break;
			}
		}
		else
		{
			bValid = false;
			break;
		}
	}
	if(!bValid)
		eBestPlayer = NO_PLAYER;

	return eBestPlayer;
}


void CvPlot::plotAction(PlotUnitFunc func, int iData1, int iData2, PlayerTypes eOwner, TeamTypes eTeam)
{
	CLLNode<IDInfo>* pUnitNode = headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);
		if (eOwner == NO_PLAYER || pLoopUnit->getOwner() == eOwner)
		{
			if (eTeam == NO_TEAM || pLoopUnit->getTeam() == eTeam)
				func(pLoopUnit, iData1, iData2);
		}
	}
}


int CvPlot::plotCount(ConstPlotUnitFunc funcA, int iData1A, int iData2A, PlayerTypes eOwner, TeamTypes eTeam, ConstPlotUnitFunc funcB, int iData1B, int iData2B) const
{
	int iCount = 0;
	for (CLLNode<IDInfo> const* pUnitNode = headUnitNode(); pUnitNode != NULL; pUnitNode = nextUnitNode(pUnitNode)) // advc: was a while loop
	{
		CvUnit const* pLoopUnit = ::getUnit(pUnitNode->m_data);
		if (eOwner == NO_PLAYER || pLoopUnit->getOwner() == eOwner)
		{
			if (eTeam == NO_TEAM || pLoopUnit->getTeam() == eTeam)
			{
				if (funcA == NULL || funcA(pLoopUnit, iData1A, iData2A))
				{
					if (funcB == NULL || funcB(pLoopUnit, iData1B, iData2B))
						iCount++;
				}
			}
		}
	}
	return iCount;
}


CvUnit* CvPlot::plotCheck(ConstPlotUnitFunc funcA, int iData1A, int iData2A, PlayerTypes eOwner, TeamTypes eTeam, ConstPlotUnitFunc funcB, int iData1B, int iData2B) const
{
	for (CLLNode<IDInfo> const* pUnitNode = headUnitNode(); pUnitNode != NULL; pUnitNode = nextUnitNode(pUnitNode)) // advc: was a while loop
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		if (eOwner == NO_PLAYER || pLoopUnit->getOwner() == eOwner)
		{
			if (eTeam == NO_TEAM || pLoopUnit->getTeam() == eTeam)
			{	// advc.045: Missing NULL check added. Bug? Tagging advc.001.
				if (funcA == NULL || funcA(pLoopUnit, iData1A, iData2A))
				{
					if (funcB == NULL || funcB(pLoopUnit, iData1B, iData2B))
						return pLoopUnit;
				}
			}
		}
	}
	return NULL;
}


bool CvPlot::isBarbarian() const
{
	return (getOwner() == BARBARIAN_PLAYER);
}


bool CvPlot::isRevealedBarbarian() const
{
	return (getRevealedOwner(GC.getGame().getActiveTeam(), true) == BARBARIAN_PLAYER);
}


bool CvPlot::isVisible(TeamTypes eTeam, bool bDebug) const
{
	if (bDebug && GC.getGame().isDebugMode())
		return true;

	if (eTeam == NO_TEAM)
		return false;

	return (getVisibilityCount(eTeam) > 0 || getStolenVisibilityCount(eTeam) > 0);
}

// <advc.300>
void CvPlot::getAdjacentLandAreaIds(std::set<int>& r) const
{
	for(int i = 0; i < NUM_DIRECTION_TYPES; i++)
	{
		CvPlot* pAdj = plotDirection(getX(), getY(), (DirectionTypes)i);
		if(pAdj != NULL && !pAdj->isWater())
			r.insert(pAdj->getArea());
	}
}

// Mostly cut from CvMap::syncRandPlot. Now ignores Barbarian units.
bool CvPlot::isCivUnitNearby(int iRadius) const
{
	if(iRadius < 0)
		return false;
	for(int dx = -iRadius; dx <= iRadius; dx++)
	{
		for(int dy = -iRadius; dy <= iRadius; dy++)
		{
			CvPlot* pPlot = plotXY(getX(), getY(), dx, dy);
			if(pPlot != NULL && pPlot->isUnit())
			{
				CvUnit* pAnyUnit = pPlot->plotCheck(PUF_isVisible, BARBARIAN_PLAYER);
				if(pAnyUnit == NULL)
					continue;
				if(pAnyUnit->getOwner() != BARBARIAN_PLAYER)
					return true;
			}
		}
	}
	return false;
}


CvPlot const* CvPlot::nearestInvisiblePlot(bool bOnlyLand, int iMaxPlotDist, TeamTypes eObserver) const
{
	if(!isVisible(eObserver, false))
		return this;
	CvPlot* r = NULL;
	CvMap const& m = GC.getMap();
	// Process plots in a spiral pattern (for performance reasons)
	for(int d = 1; d <= iMaxPlotDist; d++)
	{
		int iShortestDist = iMaxPlotDist + 1;
		for(int dx = -d; dx <= d; dx++)
		{
			for(int dy = -d; dy <= d; dy++)
			{
				// Don't process plots repeatedly:
				if(::abs(dx) < d && ::abs(dy) < d)
					continue;
				CvPlot* pPlot = m.plot(getX() + dx, getY() + dy);
				if(pPlot == NULL) continue; CvPlot const& p = *pPlot;
				if(p.isVisible(eObserver, false) || (bOnlyLand && p.isWater()) ||
						(p.isOwned() && p.getOwner() != BARBARIAN_PLAYER))
					continue;
				int iPlotDist = ::plotDistance(pPlot, this);
				if(iPlotDist < iShortestDist)
				{
					iShortestDist = iPlotDist;
					r = pPlot;
				}
			}
		}
		if(r != NULL)
			return r;
	}
	return NULL;
} // </advc.300>


bool CvPlot::isActiveVisible(bool bDebug) const
{	// <advc.706>
	if(m_bAllFog)
		return false; // </advc.706>
	return isVisible(GC.getGame().getActiveTeam(), bDebug);
}


bool CvPlot::isVisibleToCivTeam() const
{
	for (int iI = 0; iI < MAX_CIV_TEAMS; ++iI)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (isVisible(((TeamTypes)iI), false))
				return true;
		}
	}
	return false;
}

// <advc.706>
bool CvPlot::isAllFog()
{
	return m_bAllFog;
}


void CvPlot::setAllFog(bool b)
{
	m_bAllFog = b;
} // </advc.706>


bool CvPlot::isVisibleToWatchingHuman() const  // advc: style changes
{
	for (int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayer const& kHuman = GET_PLAYER((PlayerTypes)i);
		if (!kHuman.isAlive() || !kHuman.isHuman())
			continue;
		if (isVisible(kHuman.getTeam(), false))
			return true;
	}
	return false;
}


bool CvPlot::isAdjacentVisible(TeamTypes eTeam, bool bDebug) const
{
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
		if (pAdjacentPlot != NULL)
		{
			if (pAdjacentPlot->isVisible(eTeam, bDebug))
				return true;
		}
	}

	return false;
}

bool CvPlot::isAdjacentNonvisible(TeamTypes eTeam) const
{
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), ((DirectionTypes)iI));
		if (pAdjacentPlot != NULL)
		{
			if (!pAdjacentPlot->isVisible(eTeam, false))
				return true;
		}
	}

	return false;
}


bool CvPlot::isGoody(TeamTypes eTeam) const
{
	if (eTeam != NO_TEAM && GET_TEAM(eTeam).isBarbarian())
		return false;

	return ((getImprovementType() == NO_IMPROVEMENT) ? false : GC.getInfo(getImprovementType()).isGoody());
}


bool CvPlot::isRevealedGoody(TeamTypes eTeam) const
{
	if (eTeam == NO_TEAM)
		return isGoody();

	if (GET_TEAM(eTeam).isBarbarian())
		return false;

	return ((getRevealedImprovementType(eTeam) == NO_IMPROVEMENT) ? false :
		GC.getInfo(getRevealedImprovementType(eTeam)).isGoody());
}


void CvPlot::removeGoody()
{
	// <advc>
	if (!isGoody())
		return; // </advc>
	setImprovementType(NO_IMPROVEMENT);
	// <advc.004z>
	if(GC.getGame().getCurrentLayer() == GLOBE_LAYER_RESOURCE && isVisibleToWatchingHuman())
		gDLL->getInterfaceIFace()->setDirty(GlobeLayer_DIRTY_BIT, true);
	// </advc.004z>
}


bool CvPlot::isCity(bool bCheckImprovement, TeamTypes eForTeam) const
{
	//PROFILE_FUNC(); // advc.003o: Called from teamStepValid_advc (CvGameCoreUtils), CvPlot::isTradeNetworkConnected and many UnitAI routines.
	if(bCheckImprovement && getImprovementType() != NO_IMPROVEMENT &&
		GC.getInfo(getImprovementType()).isActsAsCity())
	{
		if(eForTeam == NO_TEAM || (getTeam() == NO_TEAM &&
				GC.getInfo(getImprovementType()).isOutsideBorders()))
			return true;
		if (GET_TEAM(eForTeam).isFriendlyTerritory(getTeam()))
		{
			// <advc.124>
			ImprovementTypes const eRevealedImprov = getRevealedImprovementType(eForTeam);
			if (eRevealedImprov != NO_IMPROVEMENT && GC.getInfo(eRevealedImprov).isActsAsCity())
				return true; // </advc.124>
		}
	}
	CvCity const* const pPlotCity = getPlotCity();
	return (pPlotCity != NULL   // advc.124:
			&& (eForTeam == NO_TEAM || pPlotCity->isRevealed(eForTeam, false)));
}


bool CvPlot::isFriendlyCity(const CvUnit& kUnit, bool bCheckImprovement) const
{
	if (!isCity(bCheckImprovement, kUnit.getTeam()))
		return false;

	if (isVisibleEnemyUnit(&kUnit))
		return false;

	TeamTypes ePlotTeam = getTeam();
	if (ePlotTeam == NO_TEAM)
		return false; // advc

	if (kUnit.isEnemy(ePlotTeam))
		return false;

	TeamTypes eTeam = GET_PLAYER(kUnit.getCombatOwner(ePlotTeam, this)).getTeam();
	if (eTeam == ePlotTeam)
		return true;

	if (GET_TEAM(eTeam).isOpenBorders(ePlotTeam))
		return true;

	if (GET_TEAM(ePlotTeam).isVassal(eTeam))
		return true;

	return false;
}


bool CvPlot::isEnemyCity(const CvUnit& kUnit) const
{
	CvCity* pCity = getPlotCity();
	if (pCity != NULL)
		return kUnit.isEnemy(pCity->getTeam(), this);
	return false;
}


bool CvPlot::isOccupation() const
{
	CvCity* pCity = getPlotCity();
	if (pCity != NULL)
		return pCity->isOccupation();
	return false;
}


bool CvPlot::isBeingWorked() const
{
	CvCity* pWorkingCity = getWorkingCity();
	if (pWorkingCity != NULL)
		return pWorkingCity->isWorkingPlot(this);
	return false;
}


bool CvPlot::isUnit() const
{
	return (getNumUnits() > 0);
}


bool CvPlot::isInvestigate(TeamTypes eTeam) const
{
	return (plotCheck(PUF_isInvestigate, -1, -1, NO_PLAYER, eTeam) != NULL);
}


bool CvPlot::isVisibleEnemyDefender(const CvUnit* pUnit) const
{
	return (plotCheck(PUF_canDefendEnemy, pUnit->getOwner(), pUnit->isAlwaysHostile(this),
			NO_PLAYER, NO_TEAM, PUF_isVisible, pUnit->getOwner()) != NULL);
}


CvUnit *CvPlot::getVisibleEnemyDefender(PlayerTypes ePlayer) const
{
	return plotCheck(PUF_canDefendEnemy, ePlayer, false, NO_PLAYER, NO_TEAM, PUF_isVisible, ePlayer);
}


int CvPlot::getNumDefenders(PlayerTypes ePlayer) const
{
	return plotCount(PUF_canDefend, -1, -1, ePlayer);
}


int CvPlot::getNumVisibleEnemyDefenders(const CvUnit* pUnit) const
{
	return plotCount(PUF_canDefendEnemy, pUnit->getOwner(), pUnit->isAlwaysHostile(this),
			NO_PLAYER, NO_TEAM, PUF_isVisible, pUnit->getOwner());
}


int CvPlot::getNumVisiblePotentialEnemyDefenders(const CvUnit* pUnit) const
{
	return plotCount(PUF_canDefendPotentialEnemy, pUnit->getOwner(), pUnit->isAlwaysHostile(this),
			NO_PLAYER, NO_TEAM, PUF_isVisible, pUnit->getOwner());
}


bool CvPlot::isVisibleEnemyUnit(PlayerTypes ePlayer) const
{
	return (plotCheck(PUF_isEnemy, ePlayer, false, NO_PLAYER, NO_TEAM, PUF_isVisible, ePlayer) != NULL);
}
// <advc.ctr>
bool CvPlot::isVisibleEnemyCityAttacker(PlayerTypes eDefender, TeamTypes eAssumePeace) const
{
	return (plotCheck(PUF_isEnemyCityAttacker, eDefender, eAssumePeace,
			NO_PLAYER, NO_TEAM, PUF_isVisible, eDefender) != NULL);
} // </advc.ctr>
// K-Mod
bool CvPlot::isVisiblePotentialEnemyUnit(PlayerTypes ePlayer) const
{
	return plotCheck(PUF_isPotentialEnemy, ePlayer, false, NO_PLAYER, NO_TEAM, PUF_isVisible, ePlayer) != NULL;
}
// K-Mod end
int CvPlot::getNumVisibleUnits(PlayerTypes ePlayer) const
{
	return plotCount(PUF_isVisibleDebug, ePlayer);
}


bool CvPlot::isVisibleEnemyUnit(const CvUnit* pUnit) const
{
	return (plotCheck(PUF_isEnemy, pUnit->getOwner(), pUnit->isAlwaysHostile(this),
			NO_PLAYER, NO_TEAM, PUF_isVisible, pUnit->getOwner()) != NULL);
}

// <advc.004l> Same checks as above, just doesn't loop through all units.
bool CvPlot::isVisibleEnemyUnit(CvUnit const* pUnit, CvUnit const* pPotentialEnemy) const
{
	return (::PUF_isEnemy(pPotentialEnemy, pUnit->getOwner(), pUnit->isAlwaysHostile(this)) &&
			!pPotentialEnemy->isInvisible(pUnit->getTeam(), false));
} // </advc.004l>


bool CvPlot::isVisibleOtherUnit(PlayerTypes ePlayer) const
{
	return (plotCheck(PUF_isOtherTeam, ePlayer, -1, NO_PLAYER, NO_TEAM, PUF_isVisible, ePlayer) != NULL);
}


bool CvPlot::isFighting() const
{
	return (plotCheck(PUF_isFighting) != NULL);
}


bool CvPlot::canHaveFeature(FeatureTypes eFeature) const
{
	if (eFeature == NO_FEATURE)
		return true;

	if (isFeature())
		return false;

	if (isPeak())
		return false;

	if (isCity())
		return false;

	CvFeatureInfo const& kFeature = GC.getInfo(eFeature);
	if (!kFeature.isTerrain(getTerrainType()))
		return false;

	if (kFeature.isNoCoast() && isCoastalLand())
		return false;

	if (kFeature.isNoRiver() && isRiver())
		return false;

	if (kFeature.isRequiresFlatlands() && isHills())
		return false;

	if (kFeature.isNoAdjacent())
	{
		for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
		{
			CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
			if (pAdjacentPlot != NULL)
			{
				if (pAdjacentPlot->getFeatureType() == eFeature)

					return false;
			}
		}
	}

	if (kFeature.isRequiresRiver() && !isRiver())
		return false;
	// <advc.129b>
	if(kFeature.isRequiresRiverSide() && !isRiverSide())
		return false; // </advc.129b>

	return true;
}


bool CvPlot::isValidRoute(const CvUnit* pUnit, /* advc.001i: */ bool bAssumeRevealed) const
{
	//if (isRoute())
	// <advc.001i> Replacing the above
	RouteTypes const eRoute = (bAssumeRevealed ? getRouteType() :
			getRevealedRouteType(pUnit->getTeam()));
	if(eRoute != NO_ROUTE) // </advc.001i>
	{
		if ((!pUnit->isEnemy(getTeam(), this) &&
			// advc.034:
			(getTeam() == NO_TEAM || !GET_TEAM(pUnit->getTeam()).isDisengage(getTeam())))
			|| pUnit->isEnemyRoute())
		{
			return true;
		}
	}

	return false;
}


bool CvPlot::isRiverNetwork(TeamTypes eTeam) const
{
	if (!isRiver())
		return false;

	if (GET_TEAM(eTeam).isRiverTrade())
		return true;

	/*  advc.124 (comment): This is what allows trade along owned river tiles
		without any prereq tech. */
	if (getTeam() == eTeam)
		return true;

	return false;
}

bool CvPlot::isNetworkTerrain(TeamTypes eTeam) const
{
	if (GET_TEAM(eTeam).isTerrainTrade(getTerrainType()))
		return true;

	if (isWater() && getTeam() == eTeam)
		return true;

	return false;
}


bool CvPlot::isBonusNetwork(TeamTypes eTeam) const
{
	if (isRoute() /* advc.124: */ && getRevealedRouteType(eTeam) != NO_ROUTE)
		return true;

	if (isRiverNetwork(eTeam))
		return true;

	if (isNetworkTerrain(eTeam))
		return true;

	return false;
}


bool CvPlot::isTradeNetwork(TeamTypes eTeam) const
{
	//PROFILE_FUNC(); // advc.003o

	if (getTeam() != NO_TEAM && GET_TEAM(eTeam).isAtWar(getTeam()) // advc.opt: faster than ::atWar
			/*  advc.124: War blocks trade, but blockades against the plot owner
				override this. If these blockades also affect eTeam, trade is again
				blocked (by the next conditional). */
			&& getBlockadedCount(getTeam()) <= 0)
		return false;

	if (getBlockadedCount(eTeam) > 0)
		return false;

	if (isTradeNetworkImpassable(eTeam))
		return false;

	//if (!isOwned()) { // advc.124 (commented out)
	if (!isRevealed(eTeam))
		return false;

	return isBonusNetwork(eTeam);
}


bool CvPlot::isTradeNetworkConnected(CvPlot const& kOther, TeamTypes eTeam) const  // advc: some style changes
{
	//PROFILE_FUNC(); // advc.003o

	if ((getTeam() != NO_TEAM && GET_TEAM(eTeam).isAtWar(getTeam()) // advc.opt: faster than ::atWar
			// advc.124:
			&& getBlockadedCount(getTeam()) <= getBlockadedCount(eTeam))
			||
			(kOther.getTeam() != NO_TEAM && GET_TEAM(eTeam).isAtWar(kOther.getTeam())
			// advc.124:
			&& kOther.getBlockadedCount(kOther.getTeam()) <= kOther.getBlockadedCount(eTeam)))
		return false;

	if (isTradeNetworkImpassable(eTeam) || kOther.isTradeNetworkImpassable(eTeam))
		return false;

	//if (!isOwned()) { // advc.124 (commented out)
	if (!isRevealed(eTeam) || !kOther.isRevealed(eTeam))
		return false;

	if (isRoute() /* advc.124: */ && getRevealedRouteType(eTeam) != NO_ROUTE)
	{
		if (kOther.isRoute()
				&& kOther.getRevealedRouteType(eTeam) != NO_ROUTE) // advc.124
			return true;
	}

	if (isCity(true, eTeam))
	{
		if (kOther.isNetworkTerrain(eTeam))
			return true;
	}
	// <advc.124> The isCityRadius check is just for performance
	if(isRoute() && isCityRadius() && kOther.isNetworkTerrain(eTeam))
	{
		CvCity const* const pWorkingCity = getWorkingCity();
		if(pWorkingCity != NULL && pWorkingCity->getTeam() == eTeam)
			return true;
	} // </advc.124>

	if (isNetworkTerrain(eTeam))
	{
		if (kOther.isCity(true, eTeam))
			return true;

		if (kOther.isNetworkTerrain(eTeam))
			return true;

		if (kOther.isRiverNetwork(eTeam))
		{
			if (kOther.isRiverConnection(directionXY(&kOther, this)))
				return true;
		}
		// <advc.124>
		if(kOther.isRoute() && kOther.isCityRadius())
		{
			CvCity const* const pWorkingCity = kOther.getWorkingCity();
			if(pWorkingCity != NULL && pWorkingCity->getTeam() == eTeam &&
					area()->getCitiesPerPlayer(getOwner()) <= 0)
				return true;
		} // </advc.124>
	}

	if (isRiverNetwork(eTeam))
	{
		if (kOther.isNetworkTerrain(eTeam))
		{
			if (isRiverConnection(directionXY(this, &kOther)))
				return true;
		}

		if (isRiverConnection(directionXY(this, &kOther)) || kOther.isRiverConnection(directionXY(&kOther, this)))
		{
			if (kOther.isRiverNetwork(eTeam))
				return true;
		}
	}

	return false;
}


bool CvPlot::isValidDomainForLocation(const CvUnit& unit) const
{
	if (isValidDomainForAction(unit))
		return true;

	return isCity(true, unit.getTeam());
}


bool CvPlot::isValidDomainForAction(const CvUnit& unit) const
{
	switch (unit.getDomainType())
	{
	case DOMAIN_SEA:
		return (isWater() || unit.canMoveAllTerrain());
	case DOMAIN_AIR:
		return false;
	case DOMAIN_LAND:
	case DOMAIN_IMMOBILE:
		return (!isWater() || unit.canMoveAllTerrain());
	default:
		FAssert(false);
		return false;
	}
}


bool CvPlot::isImpassable() const
{
	//PROFILE_FUNC(); // advc.003o: Most of the calls come from pathfinding
	if (isPeak())
		return true;
	return (!isFeature() ?
			// advc.opt: Check NO_TERRAIN only if NO_FEATURE
			(getTerrainType() != NO_TERRAIN && GC.getInfo(getTerrainType()).isImpassable()) :
			GC.getInfo(getFeatureType()).isImpassable());
}


int CvPlot::getXExternal() const
{
	return m_iX;
}


int CvPlot::getYExternal() const
{
	return m_iY;
}


bool CvPlot::at(int iX, int iY) const
{
	return (getX() == iX && getY() == iY);
}

// <advc.tsl>
void CvPlot::setLatitude(int iLatitude)
{
	if (iLatitude < 0 || iLatitude > 90)
	{
		FAssertMsg(false, "Latitude should be in the interval [0,90]");
		iLatitude = range(iLatitude, 0, 90);
	}
	m_iLatitude = iLatitude;
} // </advc.tsl>

// <advc>
int CvPlot::getMapIndex() const
{
	return GC.getMap().plotNum(getX(), getY());
} // </advc>


int CvPlot::getLatitude() const
{
	return m_iLatitude; // advc.tsl
}

// advc.tsl: was getLatitude()
char CvPlot::calculateLatitude() const
{
	/* orginal bts code
	int iLatitude;
	if (GC.getMap().isWrapX() || !(GC.getMap().isWrapY()))
		iLatitude = ((getY() * 100) / GC.getMap().getGridHeight());
	else iLatitude = ((getX() * 100) / GC.getMap().getGridWidth());
	iLatitude = ((iLatitude * (GC.getMap().getTopLatitude() - GC.getMap().getBottomLatitude())) / 100);
	return abs(iLatitude + GC.getMap().getBottomLatitude()); */
	// UNOFFICIAL_PATCH, Bugfix, 07/12/09, Temudjin & jdog5000: START
	int iLatitude;
	double fLatitude;
	if (GC.getMap().isWrapX() || !(GC.getMap().isWrapY()))
		fLatitude = ((getY() * 1.0) / (GC.getMap().getGridHeight()-1));
	else fLatitude = ((getX() * 1.0) / (GC.getMap().getGridWidth()-1));
	fLatitude = fLatitude * (GC.getMap().getTopLatitude() - GC.getMap().getBottomLatitude());
	iLatitude = (int)(fLatitude + 0.5);
	return intToChar(std::min(abs((iLatitude + GC.getMap().getBottomLatitude())), 90));
	// UNOFFICIAL_PATCH: END
}


int CvPlot::getFOWIndex() const
{
	CvMap const& m = GC.getMap(); // advc
	return (((m.getGridHeight() - 1) - getY()) *
			m.getGridWidth() * LANDSCAPE_FOW_RESOLUTION * LANDSCAPE_FOW_RESOLUTION) +
			(getX() * LANDSCAPE_FOW_RESOLUTION);
}


CvArea* CvPlot::area() const
{
	if(m_pPlotArea == NULL)
		m_pPlotArea = GC.getMap().getArea(getArea());
	return m_pPlotArea;
}


CvArea* CvPlot::waterArea(
	bool bNoImpassable) const // BETTER_BTS_AI_MOD, General AI, 01/02/09, jdog5000
{
	if (isWater())
		return area();

	int iBestValue = 0;
	CvArea* pBestArea = NULL;
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
		if (pAdjacentPlot == NULL
				// advc.030b: Can be NULL while recalculating areas at game start
				|| pAdjacentPlot->area() == NULL)
			continue; // advc

		if (pAdjacentPlot->isWater() &&
			// BETTER_BTS_AI_MOD, General AI, 01/02/09, jdog5000
			(!bNoImpassable || !pAdjacentPlot->isImpassable()))
		{
			int iValue = pAdjacentPlot->area()->getNumTiles();
			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				pBestArea = pAdjacentPlot->area();
			}
		}
	}
	return pBestArea;
}

CvArea* CvPlot::secondWaterArea() const
{
	FAssert(!isWater());

	CvArea* pWaterArea = waterArea();
	int iBestValue = 0;
	CvArea* pBestArea = NULL;
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
		if (pAdjacentPlot == NULL)
			continue; // advc

		if (pAdjacentPlot->isWater() &&
				/*  advc.031: Same as in waterArea, except that I see no
					need for a bNoImpassable parameter here - water areas
					blocked by ice should always be excluded. */
				!pAdjacentPlot->isImpassable() &&
				pAdjacentPlot->getArea() != pWaterArea->getID())
		{
			int iValue = pAdjacentPlot->area()->getNumTiles();
			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				pBestArea = pAdjacentPlot->area();
			}
		}
	}
	return pBestArea;
}


void CvPlot::setArea(int iNewValue, /* <advc.310> */ bool bProcess)  // advc: style changes
{
	if(!bProcess)
	{
		m_iArea = iNewValue;
		return;
	} // </advc.310>
	if(getArea() == iNewValue)
		return;
	if(area() != NULL)
		processArea(area(), -1);
	m_iArea = iNewValue;
	m_pPlotArea = NULL;
	if(area() != NULL)
	{
		processArea(area(), 1);
		updateIrrigated();
		updateYield();
	}
}


int CvPlot::getFeatureVariety() const
{
	FAssert(!isFeature() ||
			m_iFeatureVariety < GC.getInfo(getFeatureType()).getArtInfo()->getNumVarieties());
	FAssert(m_iFeatureVariety >= 0);
	return m_iFeatureVariety;
}


int CvPlot::getOwnershipDuration() const
{
	return m_iOwnershipDuration;
}


bool CvPlot::isOwnershipScore() const
{
	static int const iOWNERSHIP_SCORE_DURATION_THRESHOLD = GC.getDefineINT("OWNERSHIP_SCORE_DURATION_THRESHOLD"); // advc.opt
	return (getOwnershipDuration() >= iOWNERSHIP_SCORE_DURATION_THRESHOLD);
}


void CvPlot::setOwnershipDuration(int iNewValue)
{
	if(getOwnershipDuration() == iNewValue)
		return;

	bool bOldOwnershipScore = isOwnershipScore();

	m_iOwnershipDuration = iNewValue;
	FAssert(getOwnershipDuration() >= 0);

	if (bOldOwnershipScore != isOwnershipScore())
	{
		if (isOwned())
		{
			if (!isWater())
			{
				GET_PLAYER(getOwner()).changeTotalLandScored((isOwnershipScore()) ? 1 : -1);
			}
		}
	}
}


void CvPlot::changeOwnershipDuration(int iChange)
{
	setOwnershipDuration(getOwnershipDuration() + iChange);
}


int CvPlot::getImprovementDuration() const
{
	return m_iImprovementDuration;
}


void CvPlot::setImprovementDuration(int iNewValue)
{
	m_iImprovementDuration = iNewValue;
	FAssert(getImprovementDuration() >= 0);
}


void CvPlot::changeImprovementDuration(int iChange)
{
	setImprovementDuration(getImprovementDuration() + iChange);
}


int CvPlot::getUpgradeProgress() const
{
	return m_iUpgradeProgress;
}


int CvPlot::getUpgradeTimeLeft(ImprovementTypes eImprovement, PlayerTypes ePlayer) const
{
	int iUpgradeLeft = GC.getGame().getImprovementUpgradeTime(eImprovement) -
			(getImprovementType() == eImprovement ? getUpgradeProgress() : 0);
	if (ePlayer == NO_PLAYER)
		return iUpgradeLeft;

	int iUpgradeRate = GET_PLAYER(ePlayer).getImprovementUpgradeRate();
	if (iUpgradeRate == 0)
		return iUpgradeLeft;

	int iTurnsLeft = (iUpgradeLeft / iUpgradeRate);
	if (iTurnsLeft * iUpgradeRate < iUpgradeLeft)
		iTurnsLeft++;
	return std::max(1, iTurnsLeft);
}


void CvPlot::setUpgradeProgress(int iNewValue)
{
	m_iUpgradeProgress = iNewValue;
	FAssert(getUpgradeProgress() >= 0);
}


void CvPlot::changeUpgradeProgress(int iChange)
{
	setUpgradeProgress(getUpgradeProgress() + iChange);
}


int CvPlot::getForceUnownedTimer() const
{
	return m_iForceUnownedTimer;
}


bool CvPlot::isForceUnowned() const
{
	return (getForceUnownedTimer() > 0);
}


void CvPlot::setForceUnownedTimer(int iNewValue)
{
	m_iForceUnownedTimer = iNewValue;
	FAssert(getForceUnownedTimer() >= 0);
}


void CvPlot::changeForceUnownedTimer(int iChange)
{
	setForceUnownedTimer(getForceUnownedTimer() + iChange);
}


void CvPlot::changeCityRadiusCount(int iChange)
{
	m_iCityRadiusCount = (m_iCityRadiusCount + iChange);
	FAssert(getCityRadiusCount() >= 0);
}


bool CvPlot::isStartingPlot() const
{
	return m_bStartingPlot;
}


void CvPlot::setStartingPlot(bool bNewValue)
{
	m_bStartingPlot = bNewValue;
}


bool CvPlot::isNOfRiver() const
{
	return m_bNOfRiver;
}


void CvPlot::setNOfRiver(bool bNewValue, CardinalDirectionTypes eRiverDir)
{
	if (isNOfRiver() == bNewValue && eRiverDir == m_eRiverWEDirection)
		return; // advc

	if (isNOfRiver() != bNewValue)
	{
		updatePlotGroupBonus(false, /* advc.064d: */ false);
		m_bNOfRiver = bNewValue;
		updatePlotGroupBonus(true);

		updateRiverCrossing();
		updateYield();

		for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
		{
			CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), ((DirectionTypes)iI));
			if (pAdjacentPlot != NULL)
			{
				pAdjacentPlot->updateRiverCrossing();
				pAdjacentPlot->updateYield();
			}
		}

		if (area() != NULL)
			area()->changeNumRiverEdges((isNOfRiver()) ? 1 : -1);
	}
	FAssertMsg(eRiverDir == CARDINALDIRECTION_WEST || eRiverDir == CARDINALDIRECTION_EAST || eRiverDir == NO_CARDINALDIRECTION ||
			/*  <advc.006> The Earth scenarios have one erratic river segment at
				the (western) Canopic branch of the Nile Delta. Not a problem,
				it just looks odd. I'm excluding that segment from the assertion. */
			(getX() == 68 && getY() == 39 &&
			eRiverDir == CARDINALDIRECTION_NORTH), // </advc.006>
			"invalid parameter");
	m_eRiverWEDirection = eRiverDir;

	updateRiverSymbol(true, true);
}


bool CvPlot::isWOfRiver() const
{
	return m_bWOfRiver;
}


void CvPlot::setWOfRiver(bool bNewValue, CardinalDirectionTypes eRiverDir)
{
	if (isWOfRiver() == bNewValue && eRiverDir == m_eRiverNSDirection)
		return; // advc

	if (isWOfRiver() != bNewValue)
	{
		updatePlotGroupBonus(false, /* advc.064d: */ false);
		m_bWOfRiver = bNewValue;
		updatePlotGroupBonus(true);

		updateRiverCrossing();
		updateYield();

		for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
		{
			CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), ((DirectionTypes)iI));
			if (pAdjacentPlot != NULL)
			{
				pAdjacentPlot->updateRiverCrossing();
				pAdjacentPlot->updateYield();
			}
		}

		if (area())
			area()->changeNumRiverEdges((isWOfRiver()) ? 1 : -1);
	}

	FAssertMsg(eRiverDir == CARDINALDIRECTION_NORTH || eRiverDir == CARDINALDIRECTION_SOUTH || eRiverDir == NO_CARDINALDIRECTION, "invalid parameter");
	m_eRiverNSDirection = eRiverDir;

	updateRiverSymbol(true, true);
}


CardinalDirectionTypes CvPlot::getRiverNSDirection() const
{
	return (CardinalDirectionTypes)m_eRiverNSDirection;
}


CardinalDirectionTypes CvPlot::getRiverWEDirection() const
{
	return (CardinalDirectionTypes)m_eRiverWEDirection;
}


// This function finds an *inland* corner of this plot at which to place a river.
// It then returns the plot with that corner at its SE.

CvPlot* CvPlot::getInlandCorner() const
{
	CvPlot* pRiverPlot = NULL; // will be a plot through whose SE corner we want the river to run
	int aiShuffle[4];
	shuffleArray(aiShuffle, 4, GC.getGame().getMapRand());
	CvMap const& m = GC.getMap();
	for (int iI = 0; iI < 4; ++iI)
	{
		switch (aiShuffle[iI])
		{
		case 0:
			pRiverPlot = m.plotSoren(getX(), getY()); break;
		case 1:
			pRiverPlot = m.plotDirection(getX(), getY(), DIRECTION_NORTH); break;
		case 2:
			pRiverPlot = m.plotDirection(getX(), getY(), DIRECTION_NORTHWEST); break;
		case 3:
			pRiverPlot = m.plotDirection(getX(), getY(), DIRECTION_WEST); break;
		}
		if (pRiverPlot != NULL && !pRiverPlot->hasCoastAtSECorner())
			break;
		pRiverPlot = NULL;
	}
	return pRiverPlot;
}


bool CvPlot::hasCoastAtSECorner() const
{
	if (isWater())
		return true;

	CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), DIRECTION_EAST);
	if (pAdjacentPlot != NULL && pAdjacentPlot->isWater())
		return true;

	pAdjacentPlot = plotDirection(getX(), getY(), DIRECTION_SOUTHEAST);
	if (pAdjacentPlot != NULL && pAdjacentPlot->isWater())
		return true;

	pAdjacentPlot = plotDirection(getX(), getY(), DIRECTION_SOUTH);
	if (pAdjacentPlot != NULL && pAdjacentPlot->isWater())
		return true;

	return false;
}


bool CvPlot::isIrrigated() const
{
	return m_bIrrigated;
}


void CvPlot::setIrrigated(bool bNewValue)
{
	if(isIrrigated() == bNewValue)
		return;

	m_bIrrigated = bNewValue;

	for (int iDX = -1; iDX <= 1; iDX++)
	{
		for (int iDY = -1; iDY <= 1; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX(), getY(), iDX, iDY);
			if (pLoopPlot != NULL)
			{
				pLoopPlot->updateYield();
				pLoopPlot->setLayoutDirty(true);
			}
		}
	}
}


void CvPlot::updateIrrigated()
{
	PROFILE_FUNC();

	if (area() == NULL)
		return;

	if (!GC.getGame().isFinalInitialized())
		return;

	CvMap const& kMap = GC.getMap();
	FAStar* pIrrigatedFinder = gDLL->getFAStarIFace()->create();
	if (isIrrigated())
	{
		if (!isPotentialIrrigation())
		{
			setIrrigated(false);
			for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
			{
				CvPlot* pLoopPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
				if (pLoopPlot == NULL)
					continue; // advc

				bool bFoundFreshWater = false;
				gDLL->getFAStarIFace()->Initialize(pIrrigatedFinder,
						kMap.getGridWidth(), kMap.getGridHeight(),
						kMap.isWrapX(), kMap.isWrapY(), NULL, NULL, NULL,
						potentialIrrigation, NULL, checkFreshWater,
						&bFoundFreshWater);
				gDLL->getFAStarIFace()->GeneratePath(pIrrigatedFinder,
						pLoopPlot->getX(), pLoopPlot->getY(), -1, -1);
				if (!bFoundFreshWater)
				{
					bool bIrrigated = false;
					gDLL->getFAStarIFace()->Initialize(pIrrigatedFinder,
							kMap.getGridWidth(), kMap.getGridHeight(),
							kMap.isWrapX(), kMap.isWrapY(), NULL, NULL, NULL,
							potentialIrrigation, NULL, changeIrrigated, &bIrrigated);
					gDLL->getFAStarIFace()->GeneratePath(pIrrigatedFinder,
							pLoopPlot->getX(), pLoopPlot->getY(), -1, -1);
				}
			}
		}
	}
	else
	{
		if (isPotentialIrrigation() && isIrrigationAvailable(true))
		{
			bool bIrrigated = true;
			gDLL->getFAStarIFace()->Initialize(pIrrigatedFinder,
					kMap.getGridWidth(), kMap.getGridHeight(),
					kMap.isWrapX(), kMap.isWrapY(), NULL, NULL, NULL,
					potentialIrrigation, NULL, changeIrrigated, &bIrrigated);
			gDLL->getFAStarIFace()->GeneratePath(pIrrigatedFinder, getX(), getY(), -1, -1);
		}
	}

	gDLL->getFAStarIFace()->destroy(pIrrigatedFinder);
}


bool CvPlot::isPotentialCityWork() const
{
	return m_bPotentialCityWork;
}


bool CvPlot::isPotentialCityWorkForArea(CvArea* pArea) const
{
	PROFILE_FUNC();

	static bool const bWATER_POTENTIAL_CITY_WORK_FOR_AREA = GC.getDefineBOOL("WATER_POTENTIAL_CITY_WORK_FOR_AREA"); // advc.opt

	for (int iI = 0; iI < NUM_CITY_PLOTS; ++iI)
	{
		CvPlot* pLoopPlot = plotCity(getX(), getY(), iI);
		if (pLoopPlot != NULL)
		{
			if (bWATER_POTENTIAL_CITY_WORK_FOR_AREA || !pLoopPlot->isWater())
			{
				if (pLoopPlot->area() == pArea)
					return true;
			}
		}
	}

	return false;
}


void CvPlot::updatePotentialCityWork()
{
	PROFILE_FUNC();

	bool bValid = false;
	for (int iI = 0; iI < NUM_CITY_PLOTS; ++iI)
	{
		CvPlot* pLoopPlot = plotCity(getX(), getY(), iI);
		if (pLoopPlot != NULL)
		{
			if (!pLoopPlot->isWater())
			{
				bValid = true;
				break;
			}
		}
	}

	if (isPotentialCityWork() != bValid)
	{
		m_bPotentialCityWork = bValid;
		updateYield();
	}
}


bool CvPlot::isShowCitySymbols() const
{
	return m_bShowCitySymbols;
}


void CvPlot::updateShowCitySymbols()  // advc: style changes
{
	bool bNewShowCitySymbols = false;
	for (int iI = 0; iI < NUM_CITY_PLOTS; ++iI)
	{
		CvPlot* pLoopPlot = plotCity(getX(), getY(), iI);
		if (pLoopPlot == NULL)
			continue;
		CvCity* pLoopCity = pLoopPlot->getPlotCity();
		if (pLoopCity == NULL)
			continue;
		if (pLoopCity->isCitySelected() && gDLL->getInterfaceIFace()->isCityScreenUp())
		{
			if (pLoopCity->canWork(this))
			{
				bNewShowCitySymbols = true;
				break;
			}
		}
	}
	if (isShowCitySymbols() != bNewShowCitySymbols)
	{
		m_bShowCitySymbols = bNewShowCitySymbols;
		updateSymbolDisplay();
		updateSymbolVisibility();
	}
}


bool CvPlot::isFlagDirty() const
{
	return m_bFlagDirty;
}


void CvPlot::setFlagDirty(bool bNewValue)
{
	m_bFlagDirty = bNewValue;
}


PlayerTypes CvPlot::getOwnerExternal() const // advc.inl
{
	return getOwner();
}


void CvPlot::setOwner(PlayerTypes eNewValue, bool bCheckUnits, bool bUpdatePlotGroup)
{
	PROFILE_FUNC();

	if(getOwner() == eNewValue)
		return; // advc
	GC.getGame().addReplayMessage(REPLAY_MESSAGE_PLOT_OWNER_CHANGE, eNewValue, (char*)NULL, getX(), getY());

	CvCity* pOldCity = getPlotCity();

	if (pOldCity != NULL)
	{	// <advc>
		if(eNewValue == NO_PLAYER)
		{
			FAssert(eNewValue != NO_PLAYER);
			return;
		} // </advc>
		/*  advc.101: Include pre-revolt owner in messages (sometimes not easy
			to tell once the city has flipped, and in replays). */
		const wchar* szOldOwnerDescr = GET_PLAYER(pOldCity->getOwner()).getCivilizationDescriptionKey();
		CvWString szBuffer(gDLL->getText("TXT_KEY_MISC_CITY_REVOLTED_JOINED", pOldCity->getNameKey(), GET_PLAYER(eNewValue).getCivilizationDescriptionKey(),
				szOldOwnerDescr)); // advc.101
		gDLL->getInterfaceIFace()->addMessage(getOwner(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_CULTUREFLIP", MESSAGE_TYPE_MAJOR_EVENT,  ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_CITY_EDIT")->getPath(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), getX(), getY(), true, true);
		gDLL->getInterfaceIFace()->addMessage(eNewValue, false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_CULTUREFLIP",
				MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY, // advc.106b
				ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_CITY_EDIT")->getPath(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), getX(), getY(), true, true);
		// <advc.101> Tell other civs about it (akin to code in CvCity::doRevolt)
		for(int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			CvPlayer const& kObs = GET_PLAYER((PlayerTypes)i);
			if(!kObs.isAlive() || kObs.isMinorCiv() || kObs.getID() == getOwner() ||
					kObs.getID() == eNewValue || (!isRevealed(kObs.getTeam()) &&
					!kObs.isSpectator())) // advc.127
				continue;
			ColorTypes eColor = (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE");
			InterfaceMessageTypes eMsgType = MESSAGE_TYPE_MAJOR_EVENT;
			if(GET_TEAM(eNewValue).isVassal(kObs.getTeam()))
				eColor = (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN");
			else if(GET_TEAM(pOldCity->getTeam()).isVassal(kObs.getTeam()))
				eColor = (ColorTypes)GC.getInfoTypeForString("COLOR_RED");
			else eMsgType = MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY; // advc.106b
			gDLL->getInterfaceIFace()->addMessage(kObs.getID(), false,
					GC.getEVENT_MESSAGE_TIME(), szBuffer, 0, eMsgType,
					ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_CITY_EDIT")->getPath(),
					eColor, getX(), getY(), true, true);
		} // </advc.101>
		szBuffer = gDLL->getText("TXT_KEY_MISC_CITY_REVOLTS_JOINS", pOldCity->getNameKey(),
				GET_PLAYER(eNewValue).getCivilizationDescriptionKey(),
				szOldOwnerDescr); // advc.101
		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getOwner(),
				szBuffer, getX(), getY());
				// advc.106: Use ALT_HIGHLIGHT for research-related stuff now
				//,(ColorTypes)GC.getInfoTypeForString("COLOR_ALT_HIGHLIGHT_TEXT")
		FAssertMsg(pOldCity->getOwner() != eNewValue, "pOldCity->getOwner() is not expected to be equal with eNewValue");
		GET_PLAYER(eNewValue).acquireCity(pOldCity, false, false, bUpdatePlotGroup); // will delete the pointer
		pOldCity = NULL;
		CvCity* pNewCity = getPlotCity();
		FAssertMsg(pNewCity != NULL, "NewCity is not assigned a valid value");

		if (pNewCity != NULL)
		{
			CLinkList<IDInfo> oldUnits;
			{
				for (CLLNode<IDInfo> const* pUnitNode = headUnitNode(); pUnitNode != NULL;
						pUnitNode = nextUnitNode(pUnitNode))
					oldUnits.insertAtEnd(pUnitNode->m_data);
			}
			CLLNode<IDInfo>* pUnitNode = oldUnits.head();
			while (pUnitNode != NULL)
			{
				CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = oldUnits.next(pUnitNode);
				if (pLoopUnit)
				{
					if (pLoopUnit->isEnemy(GET_PLAYER(eNewValue).getTeam(), this))
					{
						FAssert(pLoopUnit->getTeam() != GET_PLAYER(eNewValue).getTeam());
						//pLoopUnit->kill(false, eNewValue);
						pLoopUnit->jumpToNearestValidPlot(); // advc.101: don't kill
					}
				}
			}

			UnitTypes eBestUnit = pNewCity->AI().AI_bestUnitAI(UNITAI_CITY_DEFENSE);
			if (eBestUnit == NO_UNIT)
				eBestUnit = pNewCity->AI().AI_bestUnitAI(UNITAI_ATTACK);

			if (eBestUnit != NO_UNIT)
			{
				int iFreeUnits = (GC.getDefineINT("BASE_REVOLT_FREE_UNITS") +
						(pNewCity->getHighestPopulation() *
						GC.getDefineINT("REVOLT_FREE_UNITS_PERCENT")) / 100);

				for (int iI = 0; iI < iFreeUnits; ++iI)
				{
					GET_PLAYER(eNewValue).initUnit(eBestUnit, getX(), getY(), UNITAI_CITY_DEFENSE);
				}
			}
		}
	}
	else
	{
		setOwnershipDuration(0);
		if (isOwned())
		{
			changeAdjacentSight(getTeam(), GC.getDefineINT(CvGlobals::PLOT_VISIBILITY_RANGE),
					false, NULL, bUpdatePlotGroup);
			if (area())
				area()->changeNumOwnedTiles(-1);
			GC.getMap().changeOwnedPlots(-1);

			if (!isWater())
			{
				GET_PLAYER(getOwner()).changeTotalLand(-1);
				GET_TEAM(getTeam()).changeTotalLand(-1);
				if (isOwnershipScore())
					GET_PLAYER(getOwner()).changeTotalLandScored(-1);
			}

			if (getImprovementType() != NO_IMPROVEMENT)
			{
				GET_PLAYER(getOwner()).changeImprovementCount(getImprovementType(), -1);
			}

			updatePlotGroupBonus(false, /* advc.064d: */ false);
		}

		CLLNode<IDInfo>* pUnitNode = headUnitNode();
		while (pUnitNode != NULL)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);

			if (pLoopUnit->getTeam() != getTeam() && (getTeam() == NO_TEAM ||
				!GET_TEAM(getTeam()).isVassal(pLoopUnit->getTeam())))
			{
				GET_PLAYER(pLoopUnit->getOwner()).changeNumOutsideUnits(-1);
			}

			if (pLoopUnit->isBlockading()
				// advc.033: Owner change shouldn't always disrupt blockade
				&& !pLoopUnit->canPlunder(pLoopUnit->plot()))
			{
				pLoopUnit->setBlockading(false);
				pLoopUnit->getGroup()->clearMissionQueue();
				pLoopUnit->getGroup()->setActivityType(ACTIVITY_AWAKE);
			}
		}

		m_eOwner = eNewValue;
		updateTeam(); // advc.opt

		setWorkingCityOverride(NULL);
		updateWorkingCity();

		if (isOwned())
		{
			changeAdjacentSight(getTeam(), GC.getDefineINT(CvGlobals::PLOT_VISIBILITY_RANGE), true, NULL, bUpdatePlotGroup);

			if (area())
				area()->changeNumOwnedTiles(1);
			GC.getMap().changeOwnedPlots(1);

			if (!isWater())
			{
				GET_PLAYER(getOwner()).changeTotalLand(1);
				GET_TEAM(getTeam()).changeTotalLand(1);
				if (isOwnershipScore())
					GET_PLAYER(getOwner()).changeTotalLandScored(1);
			}

			if (getImprovementType() != NO_IMPROVEMENT)
				GET_PLAYER(getOwner()).changeImprovementCount(getImprovementType(), 1);

			updatePlotGroupBonus(true);
		}

		pUnitNode = headUnitNode();
		while (pUnitNode != NULL)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);
			if (pLoopUnit->getTeam() != getTeam() && (getTeam() == NO_TEAM ||
				!GET_TEAM(getTeam()).isVassal(pLoopUnit->getTeam())))
			{
				GET_PLAYER(pLoopUnit->getOwner()).changeNumOutsideUnits(1);
			}
		}

		for (int iI = 0; iI < MAX_TEAMS; ++iI)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
				updateRevealedOwner((TeamTypes)iI);
		}

		updateIrrigated();
		updateYield();

		if (bUpdatePlotGroup)
			updatePlotGroup();

		if (bCheckUnits)
			verifyUnitValidPlot();

		if (isOwned())
		{
			if (isGoody())
				GET_PLAYER(getOwner()).doGoody(this, NULL);

			for (int iI = 0; iI < MAX_CIV_TEAMS; ++iI)
			{
				CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
				if (kLoopTeam.isAlive())
				{
					if (isVisible(kLoopTeam.getID(), false))
					{
						FirstContactData fcData(this); // advc.071
						kLoopTeam.meet(getTeam(), true, /* advc.071: */ &fcData);
					}
				}
			}
		}
		if (GC.getGame().isDebugMode())
		{
			updateMinimapColor();
			gDLL->getInterfaceIFace()->setDirty(GlobeLayer_DIRTY_BIT, true);
			gDLL->getEngineIFace()->SetDirty(CultureBorders_DIRTY_BIT, true);
		}
	}

	invalidateBorderDangerCache(); // K-Mod. (based on BBAI)
	updateSymbols();
}

// <advc.035>
PlayerTypes CvPlot::getSecondOwner() const
{
	if(isCity())
		return getPlotCity()->getOwner();
	return (PlayerTypes)m_eSecondOwner;
}


void CvPlot::setSecondOwner(PlayerTypes eNewValue)
{
	m_eSecondOwner = (char)eNewValue;
}


bool CvPlot::isContestedByRival(PlayerTypes eRival) const
{
	PlayerTypes eFirstOwner = getOwner();
	if(eFirstOwner == NO_PLAYER)
		return false;
	if(GC.getDefineBOOL(CvGlobals::OWN_EXCLUSIVE_RADIUS))
	{
		PlayerTypes eSecondOwner = getSecondOwner();
		return eSecondOwner != NO_PLAYER && eFirstOwner != NO_PLAYER &&
				eFirstOwner != eSecondOwner && (eRival == NO_PLAYER ||
				eSecondOwner == eRival || eFirstOwner == eRival) &&
				GET_TEAM(eFirstOwner).getMasterTeam() !=
				GET_TEAM(eSecondOwner).getMasterTeam();
	} // <advc.099b>
	else if(GC.getDefineINT(CvGlobals::CITY_RADIUS_DECAY) > 0)
	{
		if(eFirstOwner == eRival) // No longer contested; they own it.
			return false;
		int iTotalCulture = getTotalCulture();
		double exclWeight = GET_PLAYER(eFirstOwner).AI_exclusiveRadiusWeight();
		int iOurCulture = getCulture(eFirstOwner);
		// Just for efficiency
		if(iOurCulture * exclWeight >= 0.5 * iTotalCulture)
			return false;
		if(eRival != NO_PLAYER)
		{
			if(getCulture(eRival) >= iOurCulture * exclWeight &&
					exclusiveRadius(eRival) >= 0)
				return true;
			return false;
		}
		for(int i = 0; i < MAX_PLAYERS; i++)
		{
			CvPlayerAI const& pl = GET_PLAYER((PlayerTypes)i);
			if(!pl.isAlive() || i == getOwner())
				continue;
			int iDist = exclusiveRadius(pl.getID());
			if(iDist >= 0 && getCulture(pl.getID()) >=
					iOurCulture * pl.AI_exclusiveRadiusWeight(iDist))
				return true;
		}
		return false;
	} // </advc.099b>
	return false;
} // </advc.035>


void CvPlot::setPlotType(PlotTypes eNewValue, bool bRecalculate, bool bRebuildGraphics)  // advc: some style changes
{
	bool bRecalculateAreas = false; // advc.030
	static TerrainTypes const eLAND_TERRAIN = (TerrainTypes)GC.getDefineINT("LAND_TERRAIN"); // advc.opt
	if (getPlotType() != eNewValue)
	{
		if (getPlotType() == PLOT_OCEAN || eNewValue == PLOT_OCEAN)
			erase();

		bool bWasWater = isWater();
		bool bWasPeak = isPeak(); // advc.030

		updateSeeFromSight(false, true);

		m_ePlotType = eNewValue;

		updateYield();
		updatePlotGroup();

		updateSeeFromSight(true, true);

		if (getTerrainType() == NO_TERRAIN || GC.getInfo(getTerrainType()).isWater() != isWater())
		{
			setTerrainType(isWater() ? GC.getWATER_TERRAIN(isAdjacentToLand()) :
					eLAND_TERRAIN, bRecalculate, bRebuildGraphics);
		}

		GC.getMap().resetPathDistance();

		if (bWasWater != isWater())
		{
			if (bRecalculate)
			{
				for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
				{
					CvPlot* pLoopPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
					if (pLoopPlot == NULL)
						continue;
					if (pLoopPlot->isWater())
					{
						pLoopPlot->setTerrainType(GC.getWATER_TERRAIN(pLoopPlot->isAdjacentToLand()),
								bRecalculate, bRebuildGraphics);
					}
				}
			}

			for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
			{
				CvPlot* pLoopPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
				if (pLoopPlot != NULL)
				{
					pLoopPlot->updateYield();
					pLoopPlot->updatePlotGroup();
				}
			}

			for (int iI = 0; iI < NUM_CITY_PLOTS; ++iI)
			{
				CvPlot* pLoopPlot = plotCity(getX(), getY(), iI);
				if (pLoopPlot != NULL)
					pLoopPlot->updatePotentialCityWork();
			}

			GC.getMap().changeLandPlots((isWater()) ? -1 : 1);

			if (getBonusType() != NO_BONUS)
				GC.getMap().changeNumBonusesOnLand(getBonusType(), isWater() ? -1 : 1);

			if (isOwned())
			{
				GET_PLAYER(getOwner()).changeTotalLand(isWater() ? -1 : 1);
				GET_TEAM(getTeam()).changeTotalLand(isWater() ? -1 : 1);
			}

			if (bRecalculate)
			{
				CvArea* pNewArea = NULL;
				bRecalculateAreas = false;

				// XXX might want to change this if we allow diagonal water movement...
				if (isWater())
				{
					for (int iI = 0; iI < NUM_CARDINALDIRECTION_TYPES; ++iI)
					{
						CvPlot* pLoopPlot = plotCardinalDirection(getX(), getY(), (CardinalDirectionTypes)iI);
						if (pLoopPlot == NULL)
							continue;
						if (pLoopPlot->area()->isWater())
						{
							if (pNewArea == NULL)
								pNewArea = pLoopPlot->area();
							else if (pNewArea != pLoopPlot->area())
							{
								bRecalculateAreas = true;
								break;
							}
						}

					}
				}
				else
				{
					for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
					{
						CvPlot* pLoopPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
						if (pLoopPlot == NULL)
							continue; // advc
						if (!pLoopPlot->area()->isWater())
						{
							if (pNewArea == NULL)
								pNewArea = pLoopPlot->area();
							else if (pNewArea != pLoopPlot->area())
							{
								bRecalculateAreas = true;
								break;
							}
						}
					}
				}

				if (!bRecalculateAreas)
				{
					CvPlot* pLoopPlot = plotDirection(getX(), getY(), (DirectionTypes)(NUM_DIRECTION_TYPES - 1));
					CvArea* pLastArea = NULL;
					if (pLoopPlot != NULL)
						pLastArea = pLoopPlot->area();

					int iAreaCount = 0;
					for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
					{
						pLoopPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
						CvArea* pCurrArea = NULL;
						if (pLoopPlot != NULL)
							pCurrArea = pLoopPlot->area();

						if (pCurrArea != pLastArea)
							iAreaCount++;

						pLastArea = pCurrArea;
					}

					if (iAreaCount > 2)
						bRecalculateAreas = true;
				}
				if (bRecalculateAreas)
					GC.getMap().recalculateAreas();
				else
				{
					setArea(FFreeList::INVALID_INDEX);

					if (area() != NULL && area()->getNumTiles() == 1)
						GC.getMap().deleteArea(getArea());

					if (pNewArea == NULL)
					{
						pNewArea = GC.getMap().addArea();
						pNewArea->init(pNewArea->getID(), isWater());
					}

					setArea(pNewArea->getID());
				}
			}
		}
		// <advc.030>
		if(!isWater() && bWasPeak != isPeak() && !bRecalculateAreas && bRecalculate)
		{
			/*  When removing a peak, it's easy enough to tell whether we need to
				recalc, but too much work to come up with conditions for recalc
				when placing a peak; will have to always recalc. */
			if(isPeak())
				GC.getMap().recalculateAreas();
			else
			{
				int iArea = getArea();
				for(int i = 0; i < NUM_DIRECTION_TYPES; i++)
				{
					CvPlot* p = ::plotDirection(getX(), getY(),
							(DirectionTypes)i);
					if(p == NULL || p->isWater())
						continue;
					if(iArea != p->getArea())
					{
						GC.getMap().recalculateAreas();
						break;
					}
				}
			}
		} // </advc.030>
		if (bRebuildGraphics && GC.IsGraphicsInitialized())
		{
			//Update terrain graphical
			gDLL->getEngineIFace()->RebuildPlot(getX(), getY(), true, true);
			//gDLL->getEngineIFace()->SetDirty(MinimapTexture_DIRTY_BIT, true); //minimap does a partial update
			//gDLL->getEngineIFace()->SetDirty(GlobeTexture_DIRTY_BIT, true);

			updateFeatureSymbol();
			setLayoutDirty(true);
			updateRouteSymbol(false, true);
			updateRiverSymbol(false, true);
		}
	}
}


void CvPlot::setTerrainType(TerrainTypes eNewValue, bool bRecalculate, bool bRebuildGraphics)
{
	if(getTerrainType() == eNewValue)
		return;

	bool bUpdateSight = (getTerrainType() != NO_TERRAIN && // advc
			eNewValue != NO_TERRAIN &&
			(GC.getInfo(getTerrainType()).getSeeFromLevel() !=
			GC.getInfo(eNewValue).getSeeFromLevel() ||
			GC.getInfo(getTerrainType()).getSeeThroughLevel() !=
			GC.getInfo(eNewValue).getSeeThroughLevel()));

	if (bUpdateSight)
		updateSeeFromSight(false, true);

	m_eTerrainType = eNewValue;

	updateYield();
	updatePlotGroup();

	if (bUpdateSight)
		updateSeeFromSight(true, true);

	if (bRebuildGraphics && GC.IsGraphicsInitialized())
	{
		//Update terrain graphics
		gDLL->getEngineIFace()->RebuildPlot(getX(), getY(), false, true);
		//gDLL->getEngineIFace()->SetDirty(MinimapTexture_DIRTY_BIT, true); //minimap does a partial update
		//gDLL->getEngineIFace()->SetDirty(GlobeTexture_DIRTY_BIT, true);
	}

	if (GC.getInfo(getTerrainType()).isWater() != isWater())
	{
		setPlotType(GC.getInfo(getTerrainType()).isWater() ? PLOT_OCEAN : PLOT_LAND,
				bRecalculate, bRebuildGraphics);
	}
}


void CvPlot::setFeatureType(FeatureTypes eNewValue, int iVariety)
{
	if (eNewValue != NO_FEATURE)
	{
		if (iVariety == -1)
		{
			iVariety = ((GC.getInfo(eNewValue).getArtInfo()->getNumVarieties() *
					((getLatitude() * 9) / 8)) / 90);
		}

		iVariety = range(iVariety, 0, (GC.getInfo(eNewValue).getArtInfo()->getNumVarieties() - 1));
	}
	else iVariety = 0;

	FeatureTypes eOldFeature = getFeatureType();
	if(eOldFeature == eNewValue && m_iFeatureVariety == iVariety)
		return; // advc

	bool bUpdateSight = false;

	if (eOldFeature == NO_FEATURE || eNewValue == NO_FEATURE ||
		GC.getInfo(eOldFeature).getSeeThroughChange() != GC.getInfo(eNewValue).getSeeThroughChange())
	{
		bUpdateSight = true;
	}

	if (bUpdateSight)
		updateSeeFromSight(false, true);

	m_eFeatureType = eNewValue;
	m_iFeatureVariety = iVariety;

	updateYield();

	if (bUpdateSight)
		updateSeeFromSight(true, true);

	updateFeatureSymbol();

	if ((eOldFeature != NO_FEATURE && GC.getInfo(eOldFeature).getArtInfo()->isRiverArt()) ||
		(isFeature() && GC.getInfo(getFeatureType()).getArtInfo()->isRiverArt()))
	{
		updateRiverSymbolArt(true);
	}

	for (int iI = 0; iI < NUM_CITY_PLOTS; ++iI)
	{
		CvPlot* pLoopPlot = plotCity(getX(), getY(), iI);
		if (pLoopPlot != NULL)
		{
			CvCity* pLoopCity = pLoopPlot->getPlotCity();
			if (pLoopCity != NULL)
				pLoopCity->updateSurroundingHealthHappiness();
		}
	}

	if (!isFeature())
	{
		if (getImprovementType() != NO_IMPROVEMENT)
		{
			if (GC.getInfo(getImprovementType()).isRequiresFeature())
				setImprovementType(NO_IMPROVEMENT);
		}
	}
}


void CvPlot::setFeatureDummyVisibility(const char *dummyTag, bool show)
{
	FAssertMsg(m_pFeatureSymbol != NULL, "[Jason] No feature symbol.");
	if(m_pFeatureSymbol != NULL)
		gDLL->getFeatureIFace()->setDummyVisibility(m_pFeatureSymbol, dummyTag, show);
}


void CvPlot::addFeatureDummyModel(const char *dummyTag, const char *modelTag)
{
	FAssertMsg(m_pFeatureSymbol != NULL, "[Jason] No feature symbol.");
	if(m_pFeatureSymbol != NULL)
		gDLL->getFeatureIFace()->addDummyModel(m_pFeatureSymbol, dummyTag, modelTag);
}


void CvPlot::setFeatureDummyTexture(const char *dummyTag, const char *textureTag)
{
	FAssertMsg(m_pFeatureSymbol != NULL, "[Jason] No feature symbol.");
	if(m_pFeatureSymbol != NULL)
		gDLL->getFeatureIFace()->setDummyTexture(m_pFeatureSymbol, dummyTag, textureTag);
}


CvString CvPlot::pickFeatureDummyTag(int mouseX, int mouseY)
{
	FAssertMsg(m_pFeatureSymbol != NULL, "[Jason] No feature symbol.");
	if(m_pFeatureSymbol != NULL)
		return gDLL->getFeatureIFace()->pickDummyTag(m_pFeatureSymbol, mouseX, mouseY);
	return NULL;
}


void CvPlot::resetFeatureModel()
{
	FAssertMsg(m_pFeatureSymbol != NULL, "[Jason] No feature symbol.");
	if(m_pFeatureSymbol != NULL)
		gDLL->getFeatureIFace()->resetModel(m_pFeatureSymbol);
}


BonusTypes CvPlot::getBonusType(TeamTypes eTeam) const
{
	if (eTeam != NO_TEAM && m_eBonusType != NO_BONUS)
	{
		//if (!GET_TEAM(eTeam).isHasTech((TechTypes)(GC.getInfo((BonusTypes)m_eBonusType).getTechReveal())) && !GET_TEAM(eTeam).isForceRevealedBonus((BonusTypes)m_eBonusType))
		if (!GET_TEAM(eTeam).isBonusRevealed((BonusTypes)m_eBonusType)) // K-Mod
			return NO_BONUS;
	}

	return (BonusTypes)m_eBonusType;
}


BonusTypes CvPlot::getNonObsoleteBonusType(TeamTypes eTeam, bool bCheckConnected) const // K-Mod added bCheckConnected
{
	FAssert(eTeam != NO_TEAM);
	FAssert(GET_TEAM(eTeam).isAlive()); // K-Mod

	BonusTypes eBonus = getBonusType(eTeam);
	if (eBonus == NO_BONUS)
		return NO_BONUS; // advc
	if (GET_TEAM(eTeam).isBonusObsolete(eBonus))
		return NO_BONUS;

	// K-Mod
	if (bCheckConnected)
	{
		// note: this checks whether the bonus is connected for the owner of the plot, from the point of view of eTeam.
		TeamTypes ePlotTeam = getTeam();
		if (ePlotTeam == NO_TEAM || !GET_TEAM(ePlotTeam).isHasTech((TechTypes)GC.getInfo(eBonus).getTechCityTrade()))
			return NO_BONUS;

		// note: this function is used inside CvPlot::updatePlotGroupBonuses, which is called during CvPlot::setImprovementType
		// between when the improvement is changed and the revealed improvement type is updated...
		// therefore when eTeam == ePlotTeam, we use the real improvement, not the revealed one.
		ImprovementTypes eImprovement = (eTeam == NO_TEAM || eTeam == ePlotTeam ?
				getImprovementType() : getRevealedImprovementType(eTeam));

		FAssert(ePlotTeam != eTeam || eImprovement == getImprovementType());

		if (!isCity() && !GET_TEAM(ePlotTeam).doesImprovementConnectBonus(eImprovement, eBonus))
			return NO_BONUS;
	} // K-Mod end

	return eBonus;
}


void CvPlot::setBonusType(BonusTypes eNewValue)
{
	if(getBonusType() == eNewValue)
		return; // advc

	if (getBonusType() != NO_BONUS)
	{
		if (area())
			area()->changeNumBonuses(getBonusType(), -1);

		GC.getMap().changeNumBonuses(getBonusType(), -1);

		if (!isWater())
			GC.getMap().changeNumBonusesOnLand(getBonusType(), -1);
	}

	updatePlotGroupBonus(false, /* advc.064d: */ false);
	m_eBonusType = eNewValue;
	updatePlotGroupBonus(true);

	if (getBonusType() != NO_BONUS)
	{
		if (area())
			area()->changeNumBonuses(getBonusType(), 1);

		GC.getMap().changeNumBonuses(getBonusType(), 1);

		if (!isWater())
			GC.getMap().changeNumBonusesOnLand(getBonusType(), 1);
	}

	updateYield();
	setLayoutDirty(true);
	gDLL->getInterfaceIFace()->setDirty(GlobeLayer_DIRTY_BIT, true);
}


void CvPlot::setImprovementType(ImprovementTypes eNewValue)
{
	ImprovementTypes eOldImprovement = getImprovementType();
	if(getImprovementType() == eNewValue)
		return; // advc

	if (getImprovementType() != NO_IMPROVEMENT)
	{	// advc.opt:
		/*if (area())
			area()->changeNumImprovements(getImprovementType(), -1);*/
		if (isOwned())
			GET_PLAYER(getOwner()).changeImprovementCount(getImprovementType(), -1);
	}

	updatePlotGroupBonus(false, /* advc.064d: */ false);
	m_eImprovementType = eNewValue;
	updatePlotGroupBonus(true);

	if (getImprovementType() == NO_IMPROVEMENT)
		setImprovementDuration(0);

	setUpgradeProgress(0);

	for (int iI = 0; iI < MAX_TEAMS; ++iI)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (isVisible((TeamTypes)iI, false))
				setRevealedImprovementType((TeamTypes)iI, getImprovementType());
		}
	}

	if (getImprovementType() != NO_IMPROVEMENT)
	{	// advc.opt:
		/*if (area())
			area()->changeNumImprovements(getImprovementType(), 1);*/
		if (isOwned())
			GET_PLAYER(getOwner()).changeImprovementCount(getImprovementType(), 1);
	}

	updateIrrigated();
	updateYield();

	for (int iI = 0; iI < NUM_CITY_PLOTS; ++iI)
	{
		CvPlot* pLoopPlot = plotCity(getX(), getY(), iI);
		if (pLoopPlot != NULL)
		{
			CvCity* pLoopCity = pLoopPlot->getPlotCity();
			if (pLoopCity != NULL)
				pLoopCity->updateSurroundingHealthHappiness();
		}
	}

	// Building or removing a fort will now force a plotgroup update to verify resource connections.
	if ((NO_IMPROVEMENT != getImprovementType() && GC.getInfo(getImprovementType()).isActsAsCity()) !=
		(NO_IMPROVEMENT != eOldImprovement && GC.getInfo(eOldImprovement).isActsAsCity()))
	{
		updatePlotGroup(/* advc.064d: */ true);
	}

	if (eOldImprovement != NO_IMPROVEMENT && GC.getInfo(eOldImprovement).isActsAsCity())
		verifyUnitValidPlot();

	if (GC.getGame().isDebugMode())
		setLayoutDirty(true);

	if (getImprovementType() != NO_IMPROVEMENT)
		CvEventReporter::getInstance().improvementBuilt(getImprovementType(), getX(), getY());

	if (getImprovementType() == NO_IMPROVEMENT)
		CvEventReporter::getInstance().improvementDestroyed(eOldImprovement, getOwner(), getX(), getY());

	CvCity* pWorkingCity = getWorkingCity();
	if (NULL != pWorkingCity)
	{
		if ((NO_IMPROVEMENT != eNewValue && pWorkingCity->getImprovementFreeSpecialists(eNewValue) > 0)	||
			(NO_IMPROVEMENT != eOldImprovement && pWorkingCity->getImprovementFreeSpecialists(eOldImprovement) > 0))
		{
			pWorkingCity->AI_setAssignWorkDirty(true);
		}
	}

	gDLL->getInterfaceIFace()->setDirty(CitizenButtons_DIRTY_BIT, true);
}


void CvPlot::setRouteType(RouteTypes eNewValue, bool bUpdatePlotGroups)
{
	if(getRouteType() == eNewValue)
		return;

	bool bOldRoute = isRoute(); // XXX is this right???

	updatePlotGroupBonus(false, /* advc.064d: */ false);
	m_eRouteType = eNewValue;
	updatePlotGroupBonus(true);

	for (int iI = 0; iI < MAX_TEAMS; ++iI)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (isVisible((TeamTypes)iI, false))
				setRevealedRouteType((TeamTypes)iI, getRouteType());
		}
	}

	updateYield();

	if (bUpdatePlotGroups)
	{
		if (bOldRoute != isRoute())
			updatePlotGroup(/* advc.064d: */ bOldRoute);
	}

	if (GC.getGame().isDebugMode())
		updateRouteSymbol(true, true);

	if (getRouteType() != NO_ROUTE)
		CvEventReporter::getInstance().routeBuilt(getRouteType(), getX(), getY());

	// K-Mod. Fixing a bug in the border danger cache from BBAI.
	if (bOldRoute && !isRoute())
		invalidateBorderDangerCache();
	// K-Mod end
}


void CvPlot::updateCityRoute(bool bUpdatePlotGroup)  // advc: some style changes
{
	if (!isCity())
		return;

	FAssert(isOwned());

	RouteTypes eCityRoute = GET_PLAYER(getOwner()).getBestRoute();
	if (eCityRoute == NO_ROUTE)
		eCityRoute = ((RouteTypes)GC.getDefineINT("INITIAL_CITY_ROUTE_TYPE"));

	setRouteType(eCityRoute, bUpdatePlotGroup);
}


void CvPlot::setPlotCity(CvCity* pNewValue)  // advc: style changes
{
	if(getPlotCity() == pNewValue)
		return;

	if (isCity())
	{
		for (int iI = 0; iI < NUM_CITY_PLOTS; ++iI)
		{
			CvPlot* pLoopPlot = plotCity(getX(), getY(), iI);

			if (pLoopPlot != NULL)
			{
				pLoopPlot->changeCityRadiusCount(-1);
				pLoopPlot->changePlayerCityRadiusCount(getPlotCity()->getOwner(), -1);
			}
		}
	}

	updatePlotGroupBonus(false, /* advc.064d: */ false);
	if (isCity())
	{
		CvPlotGroup* pPlotGroup = getPlotGroup(getOwner());
		if (pPlotGroup != NULL)
		{
			FAssertMsg((0 < GC.getNumBonusInfos()), "GC.getNumBonusInfos() is not greater than zero but an array is being allocated in CvPlot::setPlotCity");
			for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
			{
				BonusTypes eLoopBonus = (BonusTypes)iI;
				getPlotCity()->changeNumBonuses((eLoopBonus),
						-pPlotGroup->getNumBonuses(eLoopBonus));
			}
		}
	}
	if (pNewValue != NULL)
		m_plotCity = pNewValue->getIDInfo();
	else m_plotCity.reset();
	if (isCity())
	{
		CvPlotGroup* pPlotGroup = getPlotGroup(getOwner());
		if (pPlotGroup != NULL)
		{
			for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
			{
				BonusTypes eLoopBonus = (BonusTypes)iI;
				getPlotCity()->changeNumBonuses(eLoopBonus,
						pPlotGroup->getNumBonuses(eLoopBonus));
			}
		}
	}
	updatePlotGroupBonus(true);

	if (isCity())
	{
		for (int iI = 0; iI < NUM_CITY_PLOTS; ++iI)
		{
			CvPlot* pLoopPlot = plotCity(getX(), getY(), iI);
			if (pLoopPlot != NULL)
			{
				pLoopPlot->changeCityRadiusCount(1);
				pLoopPlot->changePlayerCityRadiusCount(getPlotCity()->getOwner(), 1);
			}
		}
	}
	updateIrrigated();
	updateYield();
	updateMinimapColor();
}

// <advc.005c>
void CvPlot::setRuinsName(const CvWString& szName)
{
	m_szMostRecentCityName = szName;
}


const wchar* CvPlot::getRuinsName() const
{
	return m_szMostRecentCityName;
} // </advc.005c>


void CvPlot::updateWorkingCity()
{
	CvCity* pBestCity = getPlotCity();
	if (pBestCity == NULL)
	{
		pBestCity = getWorkingCityOverride();
		FAssertMsg(pBestCity == NULL || pBestCity->getOwner() == getOwner(), "pBest city is expected to either be NULL or the current plot instance's");
	}

	if (pBestCity == NULL && isOwned())
	{
		int iBestPlot = 0;
		for (int iI = 0; iI < NUM_CITY_PLOTS; ++iI)
		{
			CvPlot* pLoopPlot = plotCity(getX(), getY(), iI);
			if (pLoopPlot == NULL)
				continue;
			CvCity* pLoopCity = pLoopPlot->getPlotCity();
			if (pLoopCity == NULL)
				continue; // advc

			if (pLoopCity->getOwner() == getOwner())
			{	// XXX use getGameTurnAcquired() instead???
				int const* pCityPriority = GC.getCityPlotPriority(); // advc
				if (pBestCity == NULL ||
					pCityPriority[iI] < pCityPriority[iBestPlot] ||
					(pCityPriority[iI] == pCityPriority[iBestPlot] &&
					(pLoopCity->getGameTurnFounded() < pBestCity->getGameTurnFounded() ||
					(pLoopCity->getGameTurnFounded() == pBestCity->getGameTurnFounded() &&
					pLoopCity->getID() < pBestCity->getID()))))
				{
					iBestPlot = iI;
					pBestCity = pLoopCity;
				}
			}
		}
	}

	CvCity* pOldWorkingCity = getWorkingCity();
	if (pOldWorkingCity == pBestCity)
		return;

	if (pOldWorkingCity != NULL)
		pOldWorkingCity->setWorkingPlot(this, false);

	if (pBestCity != NULL)
	{
		FAssertMsg(isOwned(), "isOwned is expected to be true");
		FAssertMsg(!isBeingWorked(), "isBeingWorked did not return false as expected");
		m_workingCity = pBestCity->getIDInfo();
	}
	else m_workingCity.reset();

	if (pOldWorkingCity != NULL)
		pOldWorkingCity->AI_setAssignWorkDirty(true);
	if (getWorkingCity() != NULL)
		getWorkingCity()->AI_setAssignWorkDirty(true);

	updateYield();
	updateFog();
	updateShowCitySymbols();

	if (getOwner() == GC.getGame().getActivePlayer())
	{
		if (gDLL->getGraphicOption(GRAPHICOPTION_CITY_RADIUS))
		{
			//if (gDLL->getInterfaceIFace()->canSelectionListFound())
			// <advc.004h>
			CvUnit* pHeadSelectedUnit = gDLL->getInterfaceIFace()->getHeadSelectedUnit();
			if(pHeadSelectedUnit != NULL && pHeadSelectedUnit->canFound()) // </advc.004h>
				gDLL->getInterfaceIFace()->setDirty(ColoredPlots_DIRTY_BIT, true);
		}
	}
}


CvCity* CvPlot::getWorkingCityOverride() const
{
	return ::getCity(m_workingCityOverride);
}


void CvPlot::setWorkingCityOverride( const CvCity* pNewValue)
{
	if (getWorkingCityOverride() == pNewValue)
		return; // advc

	if (pNewValue != NULL)
	{
		FAssertMsg(pNewValue->getOwner() == getOwner(), "Argument city pNewValue's owner is expected to be the same as the current instance");
		m_workingCityOverride = pNewValue->getIDInfo();
	}
	else m_workingCityOverride.reset();

	updateWorkingCity();
}


int CvPlot::getRiverID() const
{
	return m_iRiverID;
}


void CvPlot::setRiverID(int iNewValue)
{
	m_iRiverID = iNewValue;
}


int CvPlot::getMinOriginalStartDist() const
{
	return m_iMinOriginalStartDist;
}


void CvPlot::setMinOriginalStartDist(int iNewValue)
{
	m_iMinOriginalStartDist = iNewValue;
}


int CvPlot::getReconCount() const
{
	return m_iReconCount;
}


void CvPlot::changeReconCount(int iChange)
{
	m_iReconCount = (m_iReconCount + iChange);
	FAssert(getReconCount() >= 0);
}


int CvPlot::getRiverCrossingCount() const
{
	return m_iRiverCrossingCount;
}


void CvPlot::changeRiverCrossingCount(int iChange)
{
	m_iRiverCrossingCount = (m_iRiverCrossingCount + iChange);
	FAssert(getRiverCrossingCount() >= 0);
}


bool CvPlot::isHabitable(bool bIgnoreSea) const {

	if(getYield(YIELD_FOOD) == 0)
		return false;
	if(!isWater() || isLake())
		return true;
	if(bIgnoreSea)
		return false;
	// Count shelf as habitable, but not arctic shelf or adj. only to one land corner.
	int iAdjHabitableLand = 0;
	for(int i = 0; i < GC.getNumDirections(); i++) {
		DirectionTypes dir = (DirectionTypes)i;
		CvPlot* pAdj = plotDirection(getX(), getY(), dir);
		if(pAdj == NULL)
			continue;
		if(pAdj->isHabitable(true))
			iAdjHabitableLand++;
		if(iAdjHabitableLand >= 2)
			return true;
	}
	return false;
}


int CvPlot::calculateNatureYield(YieldTypes eYield, TeamTypes eTeam, bool bIgnoreFeature) const
{
	// advc.016: Cut from calculateYield
	int iYield = GC.getGame().getPlotExtraYield(m_iX, m_iY, eYield);
	if (isImpassable())
	{
		//return 0;
		/*  advc.016: Impassable tiles with extra yields can be worked -
			as in BtS. This allows Python modders to make peaks workable. */
		return iYield;
	}
	iYield += GC.getInfo(getTerrainType()).getYield(eYield);
	if (isHills())
		iYield += GC.getInfo(eYield).getHillsChange();
	else if (isPeak())
		iYield += GC.getInfo(eYield).getPeakChange();
	else if (isLake())
		iYield += GC.getInfo(eYield).getLakeChange();
	if (eTeam != NO_TEAM)
	{
		BonusTypes eBonus = getBonusType(eTeam);
		if (eBonus != NO_BONUS)
			iYield += GC.getInfo(eBonus).getYieldChange(eYield);
	}
	if (isRiver())
	{
		/* <advc.500a> No change to the assigned value, but add it twice if more than
		   one river. */
		int iYieldPerRiver = ((bIgnoreFeature || !isFeature()) ?
				GC.getInfo(getTerrainType()).getRiverYieldChange(eYield) :
				GC.getInfo(getFeatureType()).getRiverYieldChange(eYield));
		int iRivers = 1;
		if(isConnectRiverSegments())
			iRivers++;
		iYield += iRivers * iYieldPerRiver; // </advc.500a>
	}

	if (isHills())
	{
		iYield += ((bIgnoreFeature || !isFeature()) ?
				GC.getInfo(getTerrainType()).getHillsYieldChange(eYield) :
				GC.getInfo(getFeatureType()).getHillsYieldChange(eYield));
	}

	if (!bIgnoreFeature)
	{
		if (isFeature())
			iYield += GC.getInfo(getFeatureType()).getYieldChange(eYield);
	}

	return std::max(0, iYield);
}


int CvPlot::calculateBestNatureYield(YieldTypes eIndex, TeamTypes eTeam) const
{
	return std::max(calculateNatureYield(eIndex, eTeam, false), calculateNatureYield(eIndex, eTeam, true));
}


int CvPlot::calculateTotalBestNatureYield(TeamTypes eTeam) const
{
	return (calculateBestNatureYield(YIELD_FOOD, eTeam) + calculateBestNatureYield(YIELD_PRODUCTION, eTeam) + calculateBestNatureYield(YIELD_COMMERCE, eTeam));
}

// BETTER_BTS_AI_MOD, City AI, 10/06/09, jdog5000: START
int CvPlot::calculateImprovementYieldChange(ImprovementTypes eImprovement, YieldTypes eYield, PlayerTypes ePlayer, bool bOptimal, bool bBestRoute) const
{
	PROFILE_FUNC();

	CvImprovementInfo const& kImpr = GC.getInfo(eImprovement);

	int iYield = kImpr.getYieldChange(eYield);
	if (isRiverSide())
		iYield += kImpr.getRiverSideYieldChange(eYield);
	if (isHills())
		iYield += kImpr.getHillsYieldChange(eYield);
	if (bOptimal ? true : isIrrigationAvailable())
		iYield += kImpr.getIrrigatedYieldChange(eYield);

	if (bOptimal)
	{
		int iBestYield = 0;
		for (int iI = 0; iI < GC.getNumRouteInfos(); ++iI)
		{
			iBestYield = std::max(iBestYield, kImpr.getRouteYieldChanges(iI, eYield));
		}
		iYield += iBestYield;
	}
	else
	{
		RouteTypes eRoute = getRouteType();
		if (bBestRoute && ePlayer != NO_PLAYER)
		{
			//eRoute = GET_PLAYER(ePlayer).getBestRoute(GC.getMap().plotSoren(getX(), getY()));
			eRoute = GET_PLAYER(ePlayer).getBestRoute(this); // K-Mod. (obvious?)
		}

		if (eRoute != NO_ROUTE)
			iYield += kImpr.getRouteYieldChanges(eRoute, eYield);
	}

	if (bOptimal || ePlayer == NO_PLAYER)
	{
		for (int iI = 0; iI < GC.getNumTechInfos(); ++iI)
		{
			iYield += kImpr.getTechYieldChanges(iI, eYield);
		}
		// K-Mod note: this doesn't calculate the 'optimal' yield, because it will count negative effects and it will count effects from competing civics.
		// Maybe I'll fix it later.
		for (int iI = 0; iI < GC.getNumCivicInfos(); ++iI)
		{
			iYield += GC.getInfo((CivicTypes)iI).getImprovementYieldChanges(eImprovement, eYield);
		}
	}
	else
	{
		iYield += GET_PLAYER(ePlayer).getImprovementYieldChange(eImprovement, eYield);
		iYield += GET_TEAM(ePlayer).getImprovementYieldChange(eImprovement, eYield);
	}

	if (ePlayer != NO_PLAYER)
	{
		BonusTypes eBonus = getBonusType(GET_PLAYER(ePlayer).getTeam());
		if (eBonus != NO_BONUS)
			iYield += kImpr.getImprovementBonusYield(eBonus, eYield);
	}

	return iYield;
} // BETTER_BTS_AI_MOD: END


char CvPlot::calculateYield(YieldTypes eYield, bool bDisplay) const
{
	if (bDisplay && GC.getGame().isDebugMode())
		return getYield(eYield);

	if (getTerrainType() == NO_TERRAIN)  // (advc: Can happen during map initialization)
		return 0;

	if (!isPotentialCityWork())
		return 0;

	bool bCity = false;
	PlayerTypes ePlayer;
	ImprovementTypes eImprovement;
	RouteTypes eRoute;
	if (bDisplay)
	{
		ePlayer = getRevealedOwner(GC.getGame().getActiveTeam());
		eImprovement = getRevealedImprovementType(GC.getGame().getActiveTeam());
		eRoute = getRevealedRouteType(GC.getGame().getActiveTeam());
		if (ePlayer == NO_PLAYER)
			ePlayer = GC.getGame().getActivePlayer();
	}
	else
	{
		ePlayer = getOwner();
		eImprovement = getImprovementType();
		eRoute = getRouteType();
	}
	int iNatureYield = // advc.908a: Preserve this for later
			calculateNatureYield(eYield, (ePlayer != NO_PLAYER ?
			GET_PLAYER(ePlayer).getTeam() : NO_TEAM));

	int iYield = iNatureYield;

	if (eImprovement != NO_IMPROVEMENT)
		iYield += calculateImprovementYieldChange(eImprovement, eYield, ePlayer);

	if (eRoute != NO_ROUTE)
		iYield += GC.getInfo(eRoute).getYieldChange(eYield);

	if (ePlayer != NO_PLAYER)
	{
		CvCity* pCity = getPlotCity();
		if (pCity != NULL)
		{
			if (!bDisplay || pCity->isRevealed(GC.getGame().getActiveTeam(), false))
			{
				CvYieldInfo const& kYield = GC.getInfo(eYield);
				iYield += kYield.getCityChange();
				if (kYield.getPopulationChangeDivisor() != 0)
				{
					iYield += ((pCity->getPopulation() +
						kYield.getPopulationChangeOffset()) /
						kYield.getPopulationChangeDivisor());
				}
				bCity = true;
			}
		}

		if (isWater() && !isImpassable())
		{
			iYield += GET_PLAYER(ePlayer).getSeaPlotYield(eYield);
			CvCity* pWorkingCity = getWorkingCity();
			if (pWorkingCity != NULL)
			{
				if (!bDisplay || pWorkingCity->isRevealed(GC.getGame().getActiveTeam(), false))
					iYield += pWorkingCity->getSeaPlotYield(eYield);
			}
		}

		if (isRiver() && !isImpassable())
		{
			CvCity* pWorkingCity = getWorkingCity();
			if (NULL != pWorkingCity)
			{
				if (!bDisplay || pWorkingCity->isRevealed(GC.getGame().getActiveTeam(), false))
					iYield += pWorkingCity->getRiverPlotYield(eYield);
			}
		}
	}

	if (bCity)
		iYield = std::max(iYield, GC.getInfo(eYield).getMinCity());

	// advc.016: Now factored into NatureYield
	//iYield += GC.getGame().getPlotExtraYield(m_iX, m_iY, eYield);

	if (ePlayer != NO_PLAYER)
	{	// <advc.908a>
		int iExtraYieldThresh = GET_PLAYER(ePlayer).getExtraYieldThreshold(eYield);
		if(iExtraYieldThresh > 0)
		{
			if(iYield > iExtraYieldThresh || iNatureYield >= iExtraYieldThresh)
				iYield += GC.getDefineINT(CvGlobals::EXTRA_YIELD); // </advc.908a>
		}

		if (GET_PLAYER(ePlayer).isGoldenAge())
		{
			if (iYield >= GC.getInfo(eYield).getGoldenAgeYieldThreshold())
				iYield += GC.getInfo(eYield).getGoldenAgeYield();
		}
	}

	iYield = std::max(0, iYield);
	return intToChar(iYield); // advc.enum
}


bool CvPlot::hasYield() const
{
	/*for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
	{
		if (getYield((YieldTypes)iI) > 0)
			return true;
	}
	return false;*/
	return m_aiYield.hasContent(); // advc.enum
}


void CvPlot::updateYield()
{
	if (area() == NULL)
		return;

	bool bChange = false;
	for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
	{
		YieldTypes eYield = (YieldTypes)iI;
		char iNewYield = calculateYield(eYield);
		if (getYield(eYield) == iNewYield)
			continue; // advc

		char iOldYield = getYield(eYield);
		m_aiYield.set(eYield, iNewYield);
		FAssert(getYield(eYield) >= 0);

		CvCity* pWorkingCity = getWorkingCity();
		if (pWorkingCity != NULL)
		{
			if (isBeingWorked())
				pWorkingCity->changeBaseYieldRate(eYield, getYield(eYield) - iOldYield);
			pWorkingCity->AI_setAssignWorkDirty(true);
		}
		bChange = true;
	}

	if (bChange)
		updateSymbols();
}


int CvPlot::countTotalCulture() const
{
	int iTotal = 0;
	for (int iI = 0; iI < MAX_PLAYERS; ++iI)
	{
		if (GET_PLAYER((PlayerTypes)iI).isEverAlive()) // advc.099: was isAlive
			iTotal += getCulture((PlayerTypes)iI);
	}
	return iTotal;
}


void CvPlot::updateTeam() // advc.opt: What getTeam used to do
{
	m_eTeam = (isOwned() ? TEAMID(getOwner()) : NO_TEAM);
}


TeamTypes CvPlot::findHighestCultureTeam() const
{
	PlayerTypes eBestPlayer = findHighestCulturePlayer();
	if (NO_PLAYER == eBestPlayer)
		return NO_TEAM;
	return GET_PLAYER(eBestPlayer).getTeam();
}


PlayerTypes CvPlot::findHighestCulturePlayer(/* advc.035: */ bool bAlive) const
{
	PlayerTypes eBestPlayer = NO_PLAYER;
	int iBestValue = 0;
	for (int iI = 0; iI < MAX_PLAYERS; ++iI)
	{
		PlayerTypes eLoopPlayer = (PlayerTypes)iI;
		if ((GET_PLAYER(eLoopPlayer).isEverAlive() // advc.099: was isAlive
			&& !bAlive) || GET_PLAYER(eLoopPlayer).isAlive()) // advc.035
		{
			int iValue = getCulture(eLoopPlayer);
			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				eBestPlayer = eLoopPlayer;
			}
		}
	}
	return eBestPlayer;
}


int CvPlot::calculateCulturePercent(PlayerTypes eIndex) const
{
	int iTotalCulture = getTotalCulture(); // advc.opt: was countTotalCulture
	if(iTotalCulture <= 0)
		return 0;
	return ((getCulture(eIndex) * 100) / iTotalCulture);
}


int CvPlot::calculateTeamCulturePercent(TeamTypes eIndex) const
{
	int iTeamCulturePercent = 0;
	for (int iI = 0; iI < MAX_PLAYERS; ++iI)
	{
		CvPlayer const& kPlayer = GET_PLAYER((PlayerTypes)iI);
		/*  advc.099: Was isAlive. (BtS doesn't call this function, but I'm
			using it for 130w.) */
		if (kPlayer.isEverAlive())
		{
			if (kPlayer.getTeam() == eIndex)
				iTeamCulturePercent += calculateCulturePercent(kPlayer.getID());
		}
	}
	return iTeamCulturePercent;
}


void CvPlot::setCulture(PlayerTypes eIndex, int iNewValue, bool bUpdate, bool bUpdatePlotGroups)  // advc: style changes
{
	PROFILE_FUNC();

	FAssert(eIndex >= 0);
	FAssert(eIndex < MAX_PLAYERS);

	if(getCulture(eIndex) == iNewValue)
		return;
	 // <advc.opt>
	if(GET_PLAYER(eIndex).isEverAlive())
		m_iTotalCulture += iNewValue - m_aiCulture.get(eIndex); // </advc.opt>
	m_aiCulture.set(eIndex, iNewValue);
	FAssert(getCulture(eIndex) >= 0);

	if(bUpdate)
		updateCulture(true, bUpdatePlotGroups);

	CvCity* pCity = getPlotCity();
	if(pCity != NULL)
		pCity->AI_setAssignWorkDirty(true);
}


void CvPlot::changeCulture(PlayerTypes eIndex, int iChange, bool bUpdate)
{
	if(iChange != 0)
		setCulture(eIndex, (getCulture(eIndex) + iChange), bUpdate, true);
}


int CvPlot::getFoundValue(PlayerTypes eIndex, /* advc.052: */ bool bRandomize) const
{
	FAssertBounds(0, MAX_PLAYERS, eIndex);

	if (m_aiFoundValue.get(eIndex) == -1)
	{
		short iValue = GC.getPythonCaller()->AI_foundValue(eIndex, *this);
		if (iValue == -1)
			m_aiFoundValue.set(eIndex, GET_PLAYER(eIndex).AI_foundValue(getX(), getY(), -1, true));

		if (m_aiFoundValue.get(eIndex) > area()->getBestFoundValue(eIndex))
			area()->setBestFoundValue(eIndex, m_aiFoundValue.get(eIndex));
	}
	//return m_aiFoundValue[eIndex];
	// <advc.052>
	int r = m_aiFoundValue.get(eIndex);
	if(bRandomize && !GET_PLAYER(eIndex).isHuman() && GC.getGame().isScenario())
	{	// Randomly change the value by +/- 1.5%
		double const plusMinus = 0.015;
		std::vector<long> hashInput;
		/*  Base the random multiplier on a number that is unique
			per game, but doesn't change throughout a game. */
		hashInput.push_back(GC.getGame().getSorenRand().
				getSeed());
		hashInput.push_back(getX());
		hashInput.push_back(getY());
		hashInput.push_back(eIndex);
		double randMult = 1 - plusMinus + 2 * plusMinus * ::hash(hashInput);
		r = ::round(r * randMult);
	}
	return r;
	// </advc.052>
}


bool CvPlot::isBestAdjacentFound(PlayerTypes eIndex)
{
	CvPlayerAI::CvFoundSettings kFoundSet(GET_PLAYER(eIndex), false); // K-Mod
	int iPlotValue = GET_PLAYER(eIndex).AI_foundValue_bulk(getX(), getY(), kFoundSet);
	if (iPlotValue == 0)
		return false;

	for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
		if (pAdjacentPlot != NULL && pAdjacentPlot->isRevealed(TEAMID(eIndex)))
		{
			//if (pAdjacentPlot->getFoundValue(eIndex) >= getFoundValue(eIndex))
			if (GET_PLAYER(eIndex).AI_foundValue_bulk(pAdjacentPlot->getX(),
					pAdjacentPlot->getY(), kFoundSet) > iPlotValue)
				return false;
		}
	}
	return true;
}


void CvPlot::setFoundValue(PlayerTypes eIndex, short iNewValue)
{
	m_aiFoundValue.set(eIndex, iNewValue);
}


void CvPlot::changePlayerCityRadiusCount(PlayerTypes eIndex, int iChange)
{
	m_aiPlayerCityRadiusCount.add(eIndex, iChange);
	FAssert(getPlayerCityRadiusCount(eIndex) >= 0);
}


CvPlotGroup* CvPlot::getPlotGroup(PlayerTypes ePlayer) const
{
	//PROFILE_FUNC();
	/*  advc.003o: Inlining would require CvPlayer.h to be included in CvPlot.h.
		Instead, I've redirected some calls to a new inline function isSamePlotGroup. */
	return GET_PLAYER(ePlayer).getPlotGroup(m_aiPlotGroup.get(ePlayer));
}


CvPlotGroup* CvPlot::getOwnerPlotGroup() const
{
	if (getOwner() == NO_PLAYER)
		return NULL;
	return getPlotGroup(getOwner());
}


void CvPlot::setPlotGroup(PlayerTypes ePlayer, CvPlotGroup* pNewValue,
	bool bVerifyProduction) // advc.064d
{
	CvPlotGroup* pOldPlotGroup = getPlotGroup(ePlayer);
	if(pOldPlotGroup == pNewValue)
		return;

	CvCity* pCity = getPlotCity();
	if (ePlayer == getOwner())
		updatePlotGroupBonus(false, /* advc.064d: */ false);

	if (pOldPlotGroup != NULL && pCity != NULL && pCity->getOwner() == ePlayer)
	{
		FOR_EACH_ENUM(Bonus)
			pCity->changeNumBonuses(eLoopBonus, -pOldPlotGroup->getNumBonuses(eLoopBonus));
	}

	if (pNewValue == NULL)
		m_aiPlotGroup.set(ePlayer, FFreeList::INVALID_INDEX);
	else m_aiPlotGroup.set(ePlayer, pNewValue->getID());

	if (getPlotGroup(ePlayer) != NULL && pCity != NULL && pCity->getOwner() == ePlayer)
	{
		FOR_EACH_ENUM(Bonus)
			pCity->changeNumBonuses(eLoopBonus, getPlotGroup(ePlayer)->getNumBonuses(eLoopBonus));
	}

	if (ePlayer == getOwner())
		updatePlotGroupBonus(true, /* advc.064d: */ bVerifyProduction);
}


void CvPlot::updatePlotGroup(/* advc.064d: */ bool bVerifyProduction)
{
	PROFILE_FUNC();
	for (PlayerIter<ALIVE> it; it.hasNext(); ++it)
	{
		CvPlayer& kPlayer = *it;
		updatePlotGroup(kPlayer.getID(), /* <advc.064d> */ false);
		/*  When recalculation of plot groups starts with updatePlotGroup, then
			bVerifyProduction sometimes interrupts city production prematurely;
			not sure why exactly. Will have to verify all cities instead. */
		if (bVerifyProduction && kPlayer.isHuman() && getOwner() == kPlayer.getID())
			kPlayer.verifyCityProduction(); // </advc.064d>
	}
}


void CvPlot::updatePlotGroup(PlayerTypes ePlayer, bool bRecalculate,
	bool bVerifyProduction) // advc.064d
{
	//PROFILE("CvPlot::updatePlotGroup(Player)");

	if (!GC.getGame().isFinalInitialized())
		return;

	TeamTypes eTeam = TEAMID(ePlayer);

	CvPlotGroup* pPlotGroup = getPlotGroup(ePlayer);
	if (pPlotGroup != NULL)
	{
		if (bRecalculate)
		{
			bool bConnected = false;
			if (isTradeNetwork(eTeam))
			{
				bConnected = true;
				FOR_EACH_ENUM(Direction)
				{
					CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), eLoopDirection);
					if (pAdjacentPlot == NULL)
						continue; // advc
					if (pAdjacentPlot->getPlotGroup(ePlayer) == pPlotGroup)
					{
						if (!isTradeNetworkConnected(*pAdjacentPlot, eTeam))
						{
							bConnected = false;
							break;
						}
					}
				}
			}
			if (!bConnected)
			{
				bool bEmpty = (pPlotGroup->getLengthPlots() == 1);
				FAssert(pPlotGroup->getLengthPlots() > 0);

				pPlotGroup->removePlot(this, /* advc.064d: */ bVerifyProduction);
				if (!bEmpty)
					pPlotGroup->recalculatePlots();
			}
		}
		pPlotGroup = getPlotGroup(ePlayer);
	}

	if (!isTradeNetwork(eTeam))
		return;

	CvMap& kMap = GC.getMap();
	FOR_EACH_ENUM(Direction)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), eLoopDirection);
		if (pAdjacentPlot == NULL)
			continue;

		CvPlotGroup* pAdjacentPlotGroup = pAdjacentPlot->getPlotGroup(ePlayer);
		if (pAdjacentPlotGroup != NULL && pAdjacentPlotGroup != pPlotGroup)
		{
			if (isTradeNetworkConnected(*pAdjacentPlot, eTeam))
			{
				if (pPlotGroup == NULL)
				{
					pAdjacentPlotGroup->addPlot(this);
					pPlotGroup = pAdjacentPlotGroup;
					FAssert(getPlotGroup(ePlayer) == pPlotGroup);
				}
				else
				{
					FAssert(getPlotGroup(ePlayer) == pPlotGroup);
					kMap.combinePlotGroups(ePlayer, pPlotGroup, pAdjacentPlotGroup,
							bVerifyProduction); // advc.064d
					pPlotGroup = getPlotGroup(ePlayer);
					FAssert(pPlotGroup != NULL);
				}
			}
		}
	}

	if (pPlotGroup == NULL)
		GET_PLAYER(ePlayer).initPlotGroup(this);
}


void CvPlot::changeVisibilityCount(TeamTypes eTeam, int iChange, InvisibleTypes eSeeInvisible, bool bUpdatePlotGroups,
	CvUnit const* pUnit) // advc.071
{
	if(iChange == 0)
		return;

	bool bOldVisible = isVisible(eTeam, false);

	m_aiVisibilityCount.add(eTeam, iChange);
	//FAssert(getVisibilityCount(eTeam) >= 0);
	/*  <advc.006> Had some problems here with the Earth1000AD scenario (as the
		initial cities were being placed). The problems remain unresolved. */
	if(getVisibilityCount(eTeam) < 0)
	{
		FAssert(m_aiVisibilityCount.get(eTeam) >= 0);
		m_aiVisibilityCount.set(eTeam, 0);
	} // </advc.006>

	if (eSeeInvisible != NO_INVISIBLE)
		changeInvisibleVisibilityCount(eTeam, eSeeInvisible, iChange);

	if (bOldVisible == isVisible(eTeam, false))
		return;

	if (isVisible(eTeam, false))
	{
		setRevealed(eTeam, true, false, NO_TEAM, bUpdatePlotGroups);
		for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
		{
			CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
			if (pAdjacentPlot != NULL)
			{	//pAdjacentPlot->updateRevealedOwner(eTeam);
				/*  K-Mod: updateRevealedOwner simply checks to see if there is a visible adjacent plot.
					But we've already checked that, so lets go right to the punch. */
				pAdjacentPlot->setRevealedOwner(eTeam, pAdjacentPlot->getOwner());
			}
		}

		if (getTeam() != NO_TEAM)
		{	// advc.071:
			FirstContactData fcData(this, pUnit == NULL ? NULL : pUnit->plot(), pUnit);
			GET_TEAM(getTeam()).meet(eTeam, true, /* advc.071: */ &fcData);
		}
		// K-Mod. Meet the owner of any units you can see.
		{
			PROFILE("CvPlot::changeVisibility -- meet units"); // (this is new, so I want to time it.)
			CvTeam& kTeam = GET_TEAM(eTeam);

			CLLNode<IDInfo> const* pUnitNode = headUnitNode();
			while (pUnitNode)
			{
				CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
				// advc.071:
				FirstContactData fcData(this, pUnit == NULL ? NULL : pUnit->plot(), pUnit, pLoopUnit);
				kTeam.meet(TEAMID(pLoopUnit->getVisualOwner(eTeam)), true,
						&fcData); // advc.071
				pUnitNode = nextUnitNode(pUnitNode);
			}
		}
		// K-Mod end
	}

	CvCity* pCity = getPlotCity();
	if (pCity != NULL)
		pCity->setInfoDirty(true);

	for (int iI = 0; iI < MAX_TEAMS; ++iI)
	{
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
		if (kLoopTeam.isAlive())
		{
			if (kLoopTeam.isStolenVisibility(eTeam))
			{
				changeStolenVisibilityCount(kLoopTeam.getID(),
						isVisible(eTeam, false) ? 1 : -1);
			}
		}
	}

	if (eTeam == GC.getGame().getActiveTeam())
	{
		updateFog();
		updateMinimapColor();
		updateCenterUnit();
	}
}


void CvPlot::changeStolenVisibilityCount(TeamTypes eTeam, int iChange)
{
	if(iChange == 0)
		return;

	bool bOldVisible = isVisible(eTeam, false);

	m_aiStolenVisibilityCount.add(eTeam, iChange);
	FAssert(getStolenVisibilityCount(eTeam) >= 0);

	if (bOldVisible != isVisible(eTeam, false))
	{
		if (isVisible(eTeam, false))
			setRevealed(eTeam, true, false, NO_TEAM, true);

		CvCity* pCity = getPlotCity();
		if (pCity != NULL)
			pCity->setInfoDirty(true);

		if (eTeam == GC.getGame().getActiveTeam())
		{
			updateFog();
			updateMinimapColor();
			updateCenterUnit();
		}
	}
}


void CvPlot::changeBlockadedCount(TeamTypes eTeam, int iChange)
{
	if (iChange == 0)
		return;

	m_aiBlockadedCount.add(eTeam, iChange);
	// BETTER_BTS_AI_MOD, Bugfix, 06/01/09, jdog5000: START
	// Hack so that never get negative blockade counts as a result of fixing issue causing
	// rare permanent blockades.
	if (getBlockadedCount(eTeam) < 0)
	{
		FAssert(isWater());
		m_aiBlockadedCount.set(eTeam, 0);
	}
	// BETTER_BTS_AI_MOD: END
	CvCity* pWorkingCity = getWorkingCity();
	if (pWorkingCity != NULL)
		pWorkingCity->AI_setAssignWorkDirty(true);
}


PlayerTypes CvPlot::getRevealedOwner(TeamTypes eTeam, bool bDebug) const
{
	if (bDebug && GC.getGame().isDebugMode())
		return getOwner();

	return getRevealedOwner(eTeam); // advc.inl: Call inline version
}


TeamTypes CvPlot::getRevealedTeam(TeamTypes eTeam, bool bDebug) const
{
	PlayerTypes eRevealedOwner = getRevealedOwner(eTeam, bDebug);
	if (eRevealedOwner != NO_PLAYER)
		return TEAMID(eRevealedOwner);
	return NO_TEAM;
}


void CvPlot::setRevealedOwner(TeamTypes eTeam, PlayerTypes eNewValue)
{
	if (getRevealedOwner(eTeam) == eNewValue)
		return;

	m_aiRevealedOwner.set(eTeam, eNewValue);
	// K-Mod
	if (eNewValue != NO_PLAYER)
		GET_TEAM(eTeam).makeHasSeen(TEAMID(eNewValue)); // K-Mod end
	if (eTeam == GC.getGame().getActiveTeam())
	{
		updateMinimapColor();
		if (GC.IsGraphicsInitialized())
		{
			gDLL->getInterfaceIFace()->setDirty(GlobeLayer_DIRTY_BIT, true);
			gDLL->getEngineIFace()->SetDirty(CultureBorders_DIRTY_BIT, true);
		}
	}
}


void CvPlot::updateRevealedOwner(TeamTypes eTeam)
{
	bool bRevealed = false;
	if (isVisible(eTeam, false))
		bRevealed = true;
	if (!bRevealed)
	{
		for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
		{
			CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
			if (pAdjacentPlot != NULL)
			{
				if (pAdjacentPlot->isVisible(eTeam, false))
				{
					bRevealed = true;
					break;
				}
			}
		}
	}
	if (bRevealed)
		setRevealedOwner(eTeam, getOwner());
}


bool CvPlot::isRiverCrossing(DirectionTypes eIndex) const
{
	if (eIndex == NO_DIRECTION)
		return false;
	return m_abRiverCrossing.get(eIndex);
}


void CvPlot::updateRiverCrossing(DirectionTypes eIndex)
{
	FAssertBounds(0, NUM_DIRECTION_TYPES, eIndex);

	CvPlot* pCornerPlot = NULL;
	bool bValid = false;
	CvPlot* pPlot = ::plotDirection(getX(), getY(), eIndex);
	if ((pPlot == NULL || !pPlot->isWater()) && !isWater())
	{
		switch (eIndex)
		{
		case DIRECTION_NORTH:
			if (pPlot != NULL)
				bValid = pPlot->isNOfRiver();
			break;

		case DIRECTION_NORTHEAST:
			pCornerPlot = plotDirection(getX(), getY(), DIRECTION_NORTH);
			break;

		case DIRECTION_EAST:
			bValid = isWOfRiver();
			break;

		case DIRECTION_SOUTHEAST:
			pCornerPlot = this;
			break;

		case DIRECTION_SOUTH:
			bValid = isNOfRiver();
			break;

		case DIRECTION_SOUTHWEST:
			pCornerPlot = plotDirection(getX(), getY(), DIRECTION_WEST);
			break;

		case DIRECTION_WEST:
			if (pPlot != NULL)
				bValid = pPlot->isWOfRiver();
			break;

		case DIRECTION_NORTHWEST:
			pCornerPlot = plotDirection(getX(), getY(), DIRECTION_NORTHWEST);
			break;

		default:
			FAssert(false);
			break;
		}

		if (pCornerPlot != NULL)
		{
			CvPlot* pNorthEastPlot = plotDirection(pCornerPlot->getX(), pCornerPlot->getY(),
					DIRECTION_EAST);
			CvPlot* pSouthEastPlot = plotDirection(pCornerPlot->getX(), pCornerPlot->getY(),
					DIRECTION_SOUTHEAST);
			CvPlot* pSouthWestPlot = plotDirection(pCornerPlot->getX(), pCornerPlot->getY(),
					DIRECTION_SOUTH);
			CvPlot* pNorthWestPlot = pCornerPlot;

			if (pSouthWestPlot && pNorthWestPlot && pSouthEastPlot && pNorthEastPlot)
			{
				if (pSouthWestPlot->isWOfRiver() && pNorthWestPlot->isWOfRiver())
					bValid = true;
				else if (pNorthEastPlot->isNOfRiver() && pNorthWestPlot->isNOfRiver())
					bValid = true;
				else if (eIndex == DIRECTION_NORTHEAST || eIndex == DIRECTION_SOUTHWEST)
				{
					if (pNorthEastPlot->isNOfRiver() && (pNorthWestPlot->isWOfRiver() || pNorthWestPlot->isWater()))
						bValid = true;
					else if ((pNorthEastPlot->isNOfRiver() || pSouthEastPlot->isWater()) && pNorthWestPlot->isWOfRiver())
						bValid = true;
					else if (pSouthWestPlot->isWOfRiver() && (pNorthWestPlot->isNOfRiver() || pNorthWestPlot->isWater()))
						bValid = true;
					else if ((pSouthWestPlot->isWOfRiver() || pSouthEastPlot->isWater()) && pNorthWestPlot->isNOfRiver())
						bValid = true;
				}
				else
				{
					FAssert((eIndex == DIRECTION_SOUTHEAST) || (eIndex == DIRECTION_NORTHWEST));

					if (pNorthWestPlot->isNOfRiver() && (pNorthWestPlot->isWOfRiver() || pNorthEastPlot->isWater()))
						bValid = true;
					else if ((pNorthWestPlot->isNOfRiver() || pSouthWestPlot->isWater()) && pNorthWestPlot->isWOfRiver())
						bValid = true;
					else if (pNorthEastPlot->isNOfRiver() && (pSouthWestPlot->isWOfRiver() || pSouthWestPlot->isWater()))
						bValid = true;
					else if ((pNorthEastPlot->isNOfRiver() || pNorthEastPlot->isWater()) && pSouthWestPlot->isWOfRiver())
						bValid = true;
				}
			}

		}
	}

	if (isRiverCrossing(eIndex) != bValid)
	{
		m_abRiverCrossing.set(eIndex, bValid);
		changeRiverCrossingCount(isRiverCrossing(eIndex) ? 1 : -1);
	}
}


void CvPlot::updateRiverCrossing()
{
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
		updateRiverCrossing((DirectionTypes)iI);
}


bool CvPlot::isRevealed(TeamTypes eTeam, bool bDebug) const
{
	if (bDebug && GC.getGame().isDebugMode())
		return true;

	return isRevealed(eTeam); // advc.inl: Call inline version
}


void CvPlot::setRevealed(TeamTypes eTeam, bool bNewValue, bool bTerrainOnly, TeamTypes eFromTeam, bool bUpdatePlotGroup)
{
	FAssertBounds(0, MAX_TEAMS, eTeam);

	CvCity* pCity = getPlotCity();
	bool bOldValue = isRevealed(eTeam); // advc.124
	if (bOldValue != bNewValue)
	{
		m_abRevealed.set(eTeam, bNewValue);

		if (area())
			area()->changeNumRevealedTiles(eTeam, isRevealed(eTeam) ? 1 : -1);
	} // <advc.124> Need to update plot group if any revealed info changes
	if (bUpdatePlotGroup &&
		(bOldValue != bNewValue ||
		getRevealedOwner(eTeam) != getOwner() ||
		getRevealedImprovementType(eTeam) != getImprovementType() ||
		getRevealedRouteType(eTeam) != getRouteType() ||
		(pCity != NULL && !pCity->isRevealed(eTeam, false)))) // </advc.124>
	{
		for (int iI = 0; iI < MAX_PLAYERS; ++iI)
		{
			PlayerTypes eLoopPlayer = (PlayerTypes)iI;
			if (GET_PLAYER(eLoopPlayer).isAlive())
			{
				if (TEAMID(eLoopPlayer) == eTeam)
					updatePlotGroup(eLoopPlayer);
			}
		}
	}
	if(bOldValue != bNewValue) // advc.124
	{
		if (eTeam == GC.getGame().getActiveTeam())
		{
			updateSymbols();
			updateFog();
			updateVisibility();

			gDLL->getInterfaceIFace()->setDirty(MinimapSection_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(GlobeLayer_DIRTY_BIT, true);
		}

		if (isRevealed(eTeam, false))
		{	// ONEVENT - PlotRevealed
			CvEventReporter::getInstance().plotRevealed(this, eTeam);
		}
	}

	if (bTerrainOnly)
		return;


	if (!isRevealed(eTeam))
	{
		setRevealedOwner(eTeam, NO_PLAYER);
		setRevealedImprovementType(eTeam, NO_IMPROVEMENT);
		setRevealedRouteType(eTeam, NO_ROUTE);

		if (pCity != NULL)
			pCity->setRevealed(eTeam, false);
		return; // advc
	}

	if (eFromTeam == NO_TEAM)
	{
		setRevealedOwner(eTeam, getOwner());
		setRevealedImprovementType(eTeam, getImprovementType());
		setRevealedRouteType(eTeam, getRouteType());

		if (pCity != NULL)
			pCity->setRevealed(eTeam, true);
	}
	else
	{
		if (getRevealedOwner(eFromTeam) == getOwner())
			setRevealedOwner(eTeam, getRevealedOwner(eFromTeam));

		if (getRevealedImprovementType(eFromTeam) == getImprovementType())
			setRevealedImprovementType(eTeam, getRevealedImprovementType(eFromTeam));

		if (getRevealedRouteType(eFromTeam) == getRouteType())
			setRevealedRouteType(eTeam, getRevealedRouteType(eFromTeam));

		if (pCity != NULL && pCity->isRevealed(eFromTeam, false))
			pCity->setRevealed(eTeam, true);
	}
}

bool CvPlot::isAdjacentRevealed(TeamTypes eTeam, /* advc.205c: */ bool bSkipOcean) const
{
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
		if (pAdjacentPlot == NULL)
			continue;
		if (pAdjacentPlot->isRevealed(eTeam) /* <advc.250c> */ && (!bSkipOcean ||
				pAdjacentPlot->getTerrainType() != GC.getWATER_TERRAIN(false))) // </advc.250c>
			return true;
	}
	return false;
}

bool CvPlot::isAdjacentNonrevealed(TeamTypes eTeam) const
{
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
		if (pAdjacentPlot != NULL)
		{
			if (!pAdjacentPlot->isRevealed(eTeam))
				return true;
		}
	}
	return false;
}


ImprovementTypes CvPlot::getRevealedImprovementType(TeamTypes eTeam, bool bDebug) const
{
	if (bDebug && GC.getGame().isDebugMode())
		return getImprovementType();

	return getRevealedImprovementType(eTeam); // advc.inl: Call inline version
}


void CvPlot::setRevealedImprovementType(TeamTypes eTeam, ImprovementTypes eNewValue)
{
	if (getRevealedImprovementType(eTeam) == eNewValue)
		return;

	m_aeRevealedImprovementType.set(eTeam, eNewValue);

	if (eTeam == GC.getGame().getActiveTeam())
	{
		updateSymbols();
		setLayoutDirty(true);
		//gDLL->getEngineIFace()->SetDirty(GlobeTexture_DIRTY_BIT, true);
	}
}


RouteTypes CvPlot::getRevealedRouteType(TeamTypes eTeam, bool bDebug) const
{
	if (bDebug && GC.getGame().isDebugMode())
		return getRouteType();

	return getRevealedRouteType(eTeam); // advc.inl: Call inline version
}


void CvPlot::setRevealedRouteType(TeamTypes eTeam, RouteTypes eNewValue)
{
	FAssertBounds(0, MAX_TEAMS, eTeam);

	if (getRevealedRouteType(eTeam) == eNewValue)
		return;

	m_aeRevealedRouteType.set(eTeam, eNewValue);

	if (eTeam == GC.getGame().getActiveTeam())
	{
		updateSymbols();
		updateRouteSymbol(true, true);
	}
}

// Returns true if build finished...
bool CvPlot::changeBuildProgress(BuildTypes eBuild, int iChange,
	/*TeamTypes eTeam*/ /* advc.251: */ PlayerTypes ePlayer)
{
	CvWString szBuffer;

	if(iChange == 0)
		return false;

	TeamTypes eTeam = TEAMID(ePlayer); // advc.251

	m_aiBuildProgress.add(eBuild, iChange);
	FAssert(getBuildProgress(eBuild) >= 0);

	/*  advc.011: Meaning that the interruption timer is reset,
		and starts counting again next turn. */
	m_iTurnsBuildsInterrupted = -1;

	if(getBuildProgress(eBuild) < getBuildTime(eBuild, /* advc.251: */ ePlayer))
		return false;

	m_aiBuildProgress.set(eBuild, 0);
	CvBuildInfo const& kBuild = GC.getInfo(eBuild);

	if (kBuild.getImprovement() != NO_IMPROVEMENT)
		setImprovementType((ImprovementTypes)kBuild.getImprovement());

	if (kBuild.getRoute() != NO_ROUTE)
		setRouteType((RouteTypes)kBuild.getRoute(), true);

	if (isFeature() && kBuild.isFeatureRemove(getFeatureType()))
	{
		FAssert(eTeam != NO_TEAM);
		CvCity* pCity;
		int iProduction = getFeatureProduction(eBuild, eTeam, &pCity);
		if (iProduction > 0)
		{
			pCity->changeFeatureProduction(iProduction);

			szBuffer = gDLL->getText("TXT_KEY_MISC_CLEARING_FEATURE_BONUS",
					GC.getInfo(getFeatureType()).getTextKeyWide(),
					iProduction, pCity->getNameKey());
			gDLL->getInterfaceIFace()->addMessage(pCity->getOwner(),
					false, GC.getEVENT_MESSAGE_TIME(), szBuffer,
					ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_CITY_EDIT")->getPath(),
					MESSAGE_TYPE_INFO, GC.getInfo(getFeatureType()).getButton(),
					(ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"),
					getX(), getY(), true, true);
		}
		// Python Event
		CvEventReporter::getInstance().plotFeatureRemoved(this, getFeatureType(), pCity);

		setFeatureType(NO_FEATURE);
	}

	return true;
}

// <advc.011> bTest checks if the next non-bTest call will cause decay
bool CvPlot::decayBuildProgress(bool bTest)
{
	PROFILE_FUNC();
	int const iDelay = GC.getDefineINT(CvGlobals::DELAY_UNTIL_BUILD_DECAY);
	if(bTest && m_iTurnsBuildsInterrupted > NO_BUILD_IN_PROGRESS &&
			m_iTurnsBuildsInterrupted + 1 < iDelay)
		return false;
	else
	{
		if(m_iTurnsBuildsInterrupted > NO_BUILD_IN_PROGRESS && m_iTurnsBuildsInterrupted < iDelay)
			m_iTurnsBuildsInterrupted++;
		if(m_iTurnsBuildsInterrupted < iDelay)
			return false;
	}
	bool r = false;
	bool bAnyInProgress = false;
	FOR_EACH_ENUM(Build)
	{
		if(m_aiBuildProgress.get(eLoopBuild) <= 0)
			continue;
		if(!bTest)
			m_aiBuildProgress.add(eLoopBuild, -1);
		r = true;
		if(m_aiBuildProgress.get(eLoopBuild) > 0)
		{
			bAnyInProgress = true;
			break;
		}
	}
	// Suspend decay (just for better performance)
	if(!bAnyInProgress && !bTest)
		m_iTurnsBuildsInterrupted = NO_BUILD_IN_PROGRESS;
	return r;
} // </advc.011>

void CvPlot::updateFeatureSymbolVisibility()
{
	//PROFILE_FUNC();

	if (!GC.IsGraphicsInitialized())
		return;

	if (m_pFeatureSymbol == NULL)
		return;

	bool bVisible = isRevealed(GC.getGame().getActiveTeam(), true);
	if (isFeature())
	{
		if(GC.getInfo(getFeatureType()).isVisibleAlways())
			bVisible = true;
	}

	bool wasVisible = !gDLL->getFeatureIFace()->IsHidden(m_pFeatureSymbol);
	if(wasVisible != bVisible)
	{
		gDLL->getFeatureIFace()->Hide(m_pFeatureSymbol, !bVisible);
		gDLL->getEngineIFace()->MarkPlotTextureAsDirty(m_iX, m_iY);
	}
}


void CvPlot::updateFeatureSymbol(bool bForce)
{
	//PROFILE_FUNC();

	if (!GC.IsGraphicsInitialized())
		return;

	gDLL->getEngineIFace()->RebuildTileArt(m_iX,m_iY);

	FeatureTypes eFeature = getFeatureType();
	if (eFeature == NO_FEATURE ||
		GC.getInfo(eFeature).getArtInfo()->isRiverArt() ||
		GC.getInfo(eFeature).getArtInfo()->getTileArtType() != TILE_ART_TYPE_NONE)
	{
		gDLL->getFeatureIFace()->destroy(m_pFeatureSymbol);
		return;
	}

	if (bForce || m_pFeatureSymbol == NULL ||
		gDLL->getFeatureIFace()->getFeature(m_pFeatureSymbol) != eFeature)
	{
		gDLL->getFeatureIFace()->destroy(m_pFeatureSymbol);
		m_pFeatureSymbol = gDLL->getFeatureIFace()->createFeature();
		FAssert(m_pFeatureSymbol != NULL);
		gDLL->getFeatureIFace()->init(m_pFeatureSymbol, 0, 0, eFeature, this);
		updateFeatureSymbolVisibility();
	} //update position and contours:
	else gDLL->getEntityIFace()->updatePosition((CvEntity*)m_pFeatureSymbol);
}


CvRoute* CvPlot::getRouteSymbol() const
{
	return m_pRouteSymbol;
}

// XXX route symbols don't really exist anymore...
void CvPlot::updateRouteSymbol(bool bForce, bool bAdjacent)
{
	PROFILE_FUNC();

	if (!GC.IsGraphicsInitialized())
		return;

	if (bAdjacent)
	{
		for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
		{
			CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)iI);
			if (pAdjacentPlot != NULL)
			{
				pAdjacentPlot->updateRouteSymbol(bForce, false);
				//pAdjacentPlot->setLayoutDirty(true);
			}
		}
	}

	RouteTypes eRoute = getRevealedRouteType(GC.getGame().getActiveTeam(), true);
	if (eRoute == NO_ROUTE)
	{
		gDLL->getRouteIFace()->destroy(m_pRouteSymbol);
		return;
	}

	if (bForce || m_pRouteSymbol == NULL ||
		gDLL->getRouteIFace()->getRoute(m_pRouteSymbol) != eRoute)
	{
		gDLL->getRouteIFace()->destroy(m_pRouteSymbol);
		m_pRouteSymbol = gDLL->getRouteIFace()->createRoute();
		FAssert(m_pRouteSymbol != NULL);
		gDLL->getRouteIFace()->init(m_pRouteSymbol, 0, 0, eRoute, this);
		setLayoutDirty(true);
	} //update position and contours:
	else gDLL->getEntityIFace()->updatePosition((CvEntity *)m_pRouteSymbol);
}


CvRiver* CvPlot::getRiverSymbol() const
{
	return m_pRiverSymbol;
}


CvFeature* CvPlot::getFeatureSymbol() const
{
	return m_pFeatureSymbol;
}


void CvPlot::updateRiverSymbol(bool bForce, bool bAdjacent)
{
	//PROFILE_FUNC();

	if (!GC.IsGraphicsInitialized())
		return;

	CvPlot* pAdjacentPlot = NULL;
	if (bAdjacent)
	{
		for(int i = 0; i < NUM_DIRECTION_TYPES; i++)
		{
			pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)i);
			if (pAdjacentPlot != NULL)
			{
				pAdjacentPlot->updateRiverSymbol(bForce, false);
				//pAdjacentPlot->setLayoutDirty(true);
			}
		}
	}

	if (!isRiverMask())
	{
		gDLL->getRiverIFace()->destroy(m_pRiverSymbol);
		return;
	}

	if (bForce || m_pRiverSymbol == NULL)
	{
		//create river
		gDLL->getRiverIFace()->destroy(m_pRiverSymbol);
		m_pRiverSymbol = gDLL->getRiverIFace()->createRiver();
		FAssert(m_pRiverSymbol != NULL);
		gDLL->getRiverIFace()->init(m_pRiverSymbol, 0, 0, 0, this);

		//force tree cuts for adjacent plots
		DirectionTypes affectedDirections[] = { NO_DIRECTION, DIRECTION_EAST, DIRECTION_SOUTHEAST, DIRECTION_SOUTH };
		for(int i = 0; i < 4; i++)
		{
			pAdjacentPlot = plotDirection(getX(), getY(), affectedDirections[i]);
			if (pAdjacentPlot != NULL)
			{
				gDLL->getEngineIFace()->ForceTreeOffsets(pAdjacentPlot->getX(), pAdjacentPlot->getY());
			}
		}

		//cut out canyons
		gDLL->getEngineIFace()->RebuildRiverPlotTile(getX(), getY(), true, false);

		//recontour adjacent rivers
		for(int i = 0; i < NUM_DIRECTION_TYPES; i++)
		{
			pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)i);
			if(pAdjacentPlot != NULL && pAdjacentPlot->m_pRiverSymbol != NULL)
			{	//update position and contours:
				gDLL->getEntityIFace()->updatePosition((CvEntity*)pAdjacentPlot->m_pRiverSymbol);
			}
		}
		// update the symbol
		setLayoutDirty(true);
	}
	//recontour rivers - update position and contours
	gDLL->getEntityIFace()->updatePosition((CvEntity*)m_pRiverSymbol);
}


void CvPlot::updateRiverSymbolArt(bool bAdjacent)
{
	//this is used to update floodplain features
	gDLL->getEntityIFace()->setupFloodPlains(m_pRiverSymbol);
	if(bAdjacent)
	{
		for(int i=0;i<NUM_DIRECTION_TYPES;i++)
		{
			CvPlot *pAdjacentPlot = plotDirection(getX(), getY(), (DirectionTypes)i);
			if(pAdjacentPlot != NULL && pAdjacentPlot->m_pRiverSymbol != NULL)
				gDLL->getEntityIFace()->setupFloodPlains(pAdjacentPlot->m_pRiverSymbol);
		}
	}
}


CvFlagEntity* CvPlot::getFlagSymbol() const
{
	return m_pFlagSymbol;
}

CvFlagEntity* CvPlot::getFlagSymbolOffset() const
{
	return m_pFlagSymbolOffset;
}

void CvPlot::updateFlagSymbol()
{
	//PROFILE_FUNC();

	if (!GC.IsGraphicsInitialized())
		return;

	PlayerTypes ePlayer = NO_PLAYER;
	PlayerTypes ePlayerOffset = NO_PLAYER;

	//CvUnit* pCenterUnit = getCenterUnit();
	CvUnit* pCenterUnit = getDebugCenterUnit(); // K-Mod

	//get the plot's unit's flag
	if (pCenterUnit != NULL)
		ePlayer = pCenterUnit->getVisualOwner();

	//get moving unit's flag
	if (gDLL->getInterfaceIFace()->getSingleMoveGotoPlot() == this)
	{
		if(ePlayer == NO_PLAYER)
			ePlayer = GC.getGame().getActivePlayer();
		else ePlayerOffset = GC.getGame().getActivePlayer();
	}

	//don't put two of the same flags
	if(ePlayerOffset == ePlayer)
		ePlayerOffset = NO_PLAYER;

	//destroy old flags
	if (ePlayer == NO_PLAYER)
		gDLL->getFlagEntityIFace()->destroy(m_pFlagSymbol);
	if (ePlayerOffset == NO_PLAYER)
		gDLL->getFlagEntityIFace()->destroy(m_pFlagSymbolOffset);

	//create and/or update unit's flag
	if (ePlayer != NO_PLAYER)
	{
		if (m_pFlagSymbol == NULL || gDLL->getFlagEntityIFace()->getPlayer(m_pFlagSymbol) != ePlayer)
		{
			if (m_pFlagSymbol != NULL)
				gDLL->getFlagEntityIFace()->destroy(m_pFlagSymbol);
			m_pFlagSymbol = gDLL->getFlagEntityIFace()->create(ePlayer);
			if (m_pFlagSymbol != NULL)
				gDLL->getFlagEntityIFace()->setPlot(m_pFlagSymbol, this, false);
		}

		if (m_pFlagSymbol != NULL)
			gDLL->getFlagEntityIFace()->updateUnitInfo(m_pFlagSymbol, this, false);
	}


	if (ePlayerOffset == NO_PLAYER)
		return; // advc

	//create and/or update offset flag
	if (m_pFlagSymbolOffset == NULL || gDLL->getFlagEntityIFace()->getPlayer(m_pFlagSymbolOffset) != ePlayerOffset)
	{
		if (m_pFlagSymbolOffset != NULL)
			gDLL->getFlagEntityIFace()->destroy(m_pFlagSymbolOffset);
		m_pFlagSymbolOffset = gDLL->getFlagEntityIFace()->create(ePlayerOffset);
		if (m_pFlagSymbolOffset != NULL)
			gDLL->getFlagEntityIFace()->setPlot(m_pFlagSymbolOffset, this, true);
	}

	if (m_pFlagSymbolOffset != NULL)
		gDLL->getFlagEntityIFace()->updateUnitInfo(m_pFlagSymbolOffset, this, true);
}


CvUnit* CvPlot::getCenterUnit() const
{
	return m_pCenterUnit;
}


CvUnit* CvPlot::getDebugCenterUnit() const
{
	CvUnit* pCenterUnit = getCenterUnit();
	if (pCenterUnit == NULL)
	{
		if (GC.getGame().isDebugMode())
		{
			CLLNode<IDInfo>* pUnitNode = headUnitNode();
			if(pUnitNode == NULL)
				pCenterUnit = NULL;
			else pCenterUnit = ::getUnit(pUnitNode->m_data);
		}
	}
	return pCenterUnit;
}


void CvPlot::setCenterUnit(CvUnit* pNewValue)
{
	CvUnit* pOldValue = getCenterUnit();
	if (pOldValue == pNewValue)
		return; // advc

	m_pCenterUnit = pNewValue;
	updateMinimapColor();
	setFlagDirty(true);
	if (getCenterUnit() != NULL)
		getCenterUnit()->setInfoBarDirty(true);
}


int CvPlot::getCultureRangeCities(PlayerTypes eOwnerIndex, CultureLevelTypes eRangeIndex) const
{
	return m_aaiCultureRangeCities.get(eOwnerIndex, eRangeIndex);
}


bool CvPlot::isCultureRangeCity(PlayerTypes eOwnerIndex, CultureLevelTypes eRangeIndex) const
{
	return (getCultureRangeCities(eOwnerIndex, eRangeIndex) > 0);
}


void CvPlot::changeCultureRangeCities(PlayerTypes eOwnerIndex, CultureLevelTypes eRangeIndex,
	int iChange, bool bUpdatePlotGroups)
{
	if(iChange == 0)
		return;

	bool bOldCultureRangeCities = isCultureRangeCity(eOwnerIndex, eRangeIndex);
	m_aaiCultureRangeCities.add(eOwnerIndex, eRangeIndex, iChange);
	FAssert(m_aaiCultureRangeCities.get(eOwnerIndex, eRangeIndex) >= 0); // advc
	if (bOldCultureRangeCities != isCultureRangeCity(eOwnerIndex, eRangeIndex))
		updateCulture(true, bUpdatePlotGroups);
}


void CvPlot::changeInvisibleVisibilityCount(TeamTypes eTeam, InvisibleTypes eInvisible, int iChange)
{
	if(iChange == 0)
		return;

	bool bOldInvisibleVisible = isInvisibleVisible(eTeam, eInvisible);
	m_aaiInvisibleVisibilityCount.add(eTeam, eInvisible, iChange);
	FAssert(m_aaiInvisibleVisibilityCount.get(eTeam, eInvisible) >= 0); // advc
	if (bOldInvisibleVisible != isInvisibleVisible(eTeam, eInvisible))
	{
		if (eTeam == GC.getGame().getActiveTeam())
			updateCenterUnit();
	}
}


int CvPlot::getNumUnits() const
{
	return m_units.getLength();
}


CvUnit* CvPlot::getUnitByIndex(int iIndex) const
{
	CLLNode<IDInfo>* pUnitNode = m_units.nodeNum(iIndex);
	if (pUnitNode != NULL)
		return ::getUnit(pUnitNode->m_data);
	return NULL;
}


void CvPlot::addUnit(CvUnit* pUnit, bool bUpdate)
{
	FAssert(pUnit->at(getX(), getY()));

	CLLNode<IDInfo>* pUnitNode = headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		if (!isBeforeUnitCycle(pLoopUnit, pUnit))
			break;
		pUnitNode = nextUnitNode(pUnitNode);
	}

	if (pUnitNode != NULL)
		m_units.insertBefore(pUnit->getIDInfo(), pUnitNode);
	else m_units.insertAtEnd(pUnit->getIDInfo());

	if (bUpdate)
	{
		updateCenterUnit();
		setFlagDirty(true);
	}
}


void CvPlot::removeUnit(CvUnit* pUnit, bool bUpdate)
{
	CLLNode<IDInfo>* pUnitNode = headUnitNode();
	while (pUnitNode != NULL)
	{
		if (::getUnit(pUnitNode->m_data) == pUnit)
		{
			FAssert(::getUnit(pUnitNode->m_data)->at(getX(), getY()));
			m_units.deleteNode(pUnitNode);
			break;
		}
		pUnitNode = nextUnitNode(pUnitNode);
	}

	if (bUpdate)
	{
		updateCenterUnit();
		setFlagDirty(true);
	}
}


CLLNode<IDInfo>* CvPlot::headUnitNode() const
{
	return m_units.head();
}


CLLNode<IDInfo>* CvPlot::tailUnitNode() const
{
	return m_units.tail();
}


int CvPlot::getNumSymbols() const
{
	return m_symbols.size();
}


CvSymbol* CvPlot::getSymbol(int iID) const
{
	return m_symbols[iID];
}


CvSymbol* CvPlot::addSymbol()
{
	CvSymbol* pSym=gDLL->getSymbolIFace()->createSymbol();
	m_symbols.push_back(pSym);
	return pSym;
}


void CvPlot::deleteSymbol(int iID)
{
	m_symbols.erase(m_symbols.begin()+iID);
}


void CvPlot::deleteAllSymbols()
{
	for(int i = 0; i < getNumSymbols(); i++)
	{
		gDLL->getSymbolIFace()->destroy(m_symbols[i]);
	}
	m_symbols.clear();
}

CvString CvPlot::getScriptData() const
{
	return m_szScriptData;
}

void CvPlot::setScriptData(const char* szNewValue)
{
	SAFE_DELETE_ARRAY(m_szScriptData);
	m_szScriptData = _strdup(szNewValue);
}

void CvPlot::doFeature()  // advc: some style changes
{
	PROFILE_FUNC();

	if (isFeature())
	{
		int iProbability = GC.getInfo(getFeatureType()).getDisappearanceProbability();
		if (iProbability > 0)
		{	/* original bts code
			if (GC.getGame().getSorenRandNum(10000, "Feature Disappearance") < iProbability)*/
			// UNOFFICIAL_PATCH, Gamespeed scaling, 03/04/10, jdog5000
			int iOdds = (10000*GC.getInfo(GC.getGame().getGameSpeedType()).getVictoryDelayPercent())/100;
			if (GC.getGame().getSorenRandNum(iOdds, "Feature Disappearance") < iProbability)
			// UNOFFICIAL_PATCH: END
			{
				setFeatureType(NO_FEATURE);
			}
		}
	}
	else if (!isUnit() && getImprovementType() == NO_IMPROVEMENT)
	{
		for (int i = 0; i < GC.getNumFeatureInfos(); i++)
		{
			FeatureTypes eFeature = (FeatureTypes)i;
			if (!canHaveFeature(eFeature))
				continue;
			if (getBonusType() != NO_BONUS && !GC.getInfo(getBonusType()).isFeature(eFeature))
				continue;
			int iProbability = 0;
			for (int iJ = 0; iJ < NUM_CARDINALDIRECTION_TYPES; iJ++)
			{
				CvPlot* pLoopPlot = plotCardinalDirection(getX(), getY(), (CardinalDirectionTypes)iJ);
				if (pLoopPlot == NULL)
					continue;
				if (pLoopPlot->getFeatureType() == eFeature)
				{
					if (pLoopPlot->getImprovementType() == NO_IMPROVEMENT)
						iProbability += GC.getInfo(eFeature).getGrowthProbability();
					else
					{
						iProbability += GC.getInfo(pLoopPlot->getImprovementType()).
								getFeatureGrowthProbability();
					}
				}
			}
			static int const iFEATURE_GROWTH_MODIFIER = GC.getDefineINT("FEATURE_GROWTH_MODIFIER");
			iProbability *= std::max(0, iFEATURE_GROWTH_MODIFIER + 100);
			iProbability /= 100;

			if (isRoute())
			{
				static int const iROUTE_FEATURE_GROWTH_MODIFIER = GC.getDefineINT("ROUTE_FEATURE_GROWTH_MODIFIER");
				iProbability *= std::max(0, iROUTE_FEATURE_GROWTH_MODIFIER + 100);
				iProbability /= 100;
			}
			if (iProbability > 0)
			{
				//if (GC.getGame().getSorenRandNum(10000, "Feature Growth") < iProbability)
				// UNOFFICIAL_PATCH, Gamespeed scaling, 03/04/10, jdog5000: START
				int iOdds = (10000*GC.getInfo(GC.getGame().getGameSpeedType()).
						getVictoryDelayPercent()) / 100;
				if (GC.getGame().getSorenRandNum(iOdds, "Feature Growth") < iProbability)
				// UNOFFICIAL_PATCH: END
				{
					setFeatureType(eFeature);
					CvCity* pCity = GC.getMap().findCity(getX(), getY(), getOwner(), NO_TEAM, false);
					if (pCity != NULL &&
						isVisible(GET_PLAYER(pCity->getOwner()).getTeam(), false)) // K-Mod
					{
						// Tell the owner of this city.
						CvWString szBuffer(gDLL->getText("TXT_KEY_MISC_FEATURE_GROWN_NEAR_CITY",
								GC.getInfo(eFeature).getTextKeyWide(), pCity->getNameKey()));
						
						gDLL->getInterfaceIFace()->addMessage(
								/*getOwner()*/ pCity->getOwner(), // K-Mod (bugfix)
								false, GC.getEVENT_MESSAGE_TIME(), szBuffer,
								"AS2D_FEATUREGROWTH", MESSAGE_TYPE_INFO,
								GC.getInfo(eFeature).getButton(),
								(ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"),
								getX(), getY(), true, true);
					}
					break;
				}
			}
		}
	}
}

void CvPlot::doCulture()
{
	// <advc> Moved the bulk of the code into a new CvCity member function
	CvCity* c = getPlotCity();
	if(c != NULL)
		c->doRevolt(); // </advc>
	doCultureDecay(); // advc.099b
	updateCulture(true, true);
}

// <advc.099b>
void CvPlot::doCultureDecay()
{
	PROFILE_FUNC();
	if(getTotalCulture() <= 0)
		return;
	int const iExclDecay = GC.getDefineINT(CvGlobals::CITY_RADIUS_DECAY);
	// advc.099:
	static int const iBaseDecayPerMill = GC.getDefineINT("TILE_CULTURE_DECAY_PER_MILL");
	bool abInRadius[MAX_CIV_PLAYERS] = {false};
	bool bInAnyRadius = false;
	int iMaxRadiusCulture = 0;
	int iMinDist = 10;
	/*  To avoid ownership oscillation and to avoid making players worried that a
		tile might flip */
	int const iCulturePercentThresh = 55;
	if(iExclDecay != 0 && isOwned() && !isCity())
	{
		CvCity* pWorkingCity = getWorkingCity();
		if(pWorkingCity == NULL || calculateCulturePercent(pWorkingCity->getOwner())
			< iCulturePercentThresh) // Check this only for performance
		{
			for(int i = 0; i < NUM_CITY_PLOTS; i++)
			{
				CvPlot* pp = ::plotCity(getX(), getY(), i);
				if(pp == NULL) continue; CvPlot& p = *pp;
				if(!p.isCity())
					continue;
				PlayerTypes const eCityOwnerId = p.getOwner();
				if(eCityOwnerId != NO_PLAYER && eCityOwnerId != BARBARIAN_PLAYER)
				{
					iMinDist = std::min(iMinDist, ::plotDistance(&p, this));
					iMaxRadiusCulture = std::max(iMaxRadiusCulture,
							getCulture(eCityOwnerId));
					abInRadius[eCityOwnerId] = true;
					bInAnyRadius = true;
				}
			}
		}
	}
	for(int i = 0; i < MAX_PLAYERS; i++)
	{
		PlayerTypes civId = (PlayerTypes)i;
		int iCulture = getCulture(civId);
		if(iCulture <= 0)
			continue;
		int iDecayPerMill = iBaseDecayPerMill;
		if(bInAnyRadius && !abInRadius[i] && iCulture >
			((100.0 - iCulturePercentThresh) /
			iCulturePercentThresh) * iMaxRadiusCulture)
		{
			double exclDecay = 0;
			if(iMinDist <= 2)
				exclDecay += iExclDecay;
			if(iMinDist <= 1)
				exclDecay += iExclDecay;
			if(iCulture < iMaxRadiusCulture)
			{
				// Gradually throttle decay when approaching the threshold
				exclDecay *= iCulture / (double)iMaxRadiusCulture;
			}
			iDecayPerMill += ::round(exclDecay);
		}
		iCulture -= (iCulture * iDecayPerMill) / 1000;
		setCulture(civId, iCulture, false, false);
	}
} // </advc.099b>

/*  <advc.099b>
	 0: city tile belonging to ePlayer
	-1: Not in the exclusive radius of ePlayer. I.e. not in the radius of any
		ePlayer city or also in the radius of another player's city.
	 1 or 2: In the exclusive radius of ePlayer.
	 1: In an inner ring
	 2: Not in any inner ring */
int CvPlot::exclusiveRadius(PlayerTypes ePlayer) const
{
	if(isCity())
	{
		if(getOwner() == ePlayer)
			return 0;
		return -1;
	}
	int r = -1;
	for(int i = 0; i < NUM_CITY_PLOTS; i++)
	{
		CvPlot* pPlot = ::plotCity(getX(), getY(), i);
		if(pPlot == NULL) continue; CvPlot const& p = *pPlot;
		if(!p.isCity())
			continue;
		if(p.getOwner() == ePlayer)
			r = ::plotDistance(&p, this);
		else return -1;
	}
	return r;
} // </advc.099b>


void CvPlot::processArea(CvArea* pArea, int iChange)  // advc: style changes
{
	// XXX not updating getBestFoundValue() or getAreaAIType()...

	if (iChange == 0)
	{
		FAssert(iChange != 0);
		return; // advc
	}

	pArea->changeNumTiles(iChange);
	if (isOwned())
		pArea->changeNumOwnedTiles(iChange);

	if (isNOfRiver())
		pArea->changeNumRiverEdges(iChange);
	if (isWOfRiver())
		pArea->changeNumRiverEdges(iChange);

	if (getBonusType() != NO_BONUS)
		pArea->changeNumBonuses(getBonusType(), iChange);
	// advc.opt:
	/*if (getImprovementType() != NO_IMPROVEMENT)
		pArea->changeNumImprovements(getImprovementType(), iChange);*/

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		PlayerTypes ePlayer = (PlayerTypes)i;
		if (GET_PLAYER(ePlayer).getStartingPlot() == this)
			pArea->changeNumStartingPlots(iChange);
		pArea->changePower(ePlayer, getUnitPower(ePlayer) * iChange);
		pArea->changeUnitsPerPlayer(ePlayer, plotCount(PUF_isPlayer, ePlayer) * iChange);
		// advc: No longer kept track of
		//pArea->changeAnimalsPerPlayer(ePlayer, plotCount(PUF_isAnimal, -1, -1, ePlayer) * iChange);
		for (int j = 0; j < NUM_UNITAI_TYPES; j++)
		{
			UnitAITypes eUnitAI = (UnitAITypes)j;
			pArea->changeNumAIUnits(ePlayer, eUnitAI,
					plotCount(PUF_isUnitAIType, eUnitAI, -1, ePlayer) * iChange);
		}
	}
	for (int iI = 0; iI < MAX_TEAMS; ++iI)
	{
		if (isRevealed((TeamTypes)iI))
			pArea->changeNumRevealedTiles((TeamTypes)iI, iChange);
	}

	CvCity* pCity = getPlotCity();
	if (pCity == NULL)
		return;

	// XXX make sure all of this (esp. the changePower()) syncs up...
	pArea->changePower(pCity->getOwner(), (getPopulationPower(pCity->getPopulation()) * iChange));

	pArea->changeCitiesPerPlayer(pCity->getOwner(), iChange);
	// <advc.030b>
	CvArea* pWaterArea = waterArea(true);
	/*  Fixme: During CvMap::recalculateAreas, for iChange=-1, the call above
	could fail to locate an adjacent water area because the area of all
	adjacent water tiles may already have been set to NULL. A subsequent
	processArea call with iChange=1 would then lead to an incorrect
	city count. I think this can only happen in a scenario with preplaced
	cities though, and I'm not sure what to do about it. */
	if(pWaterArea != NULL) {
		if(iChange > 0 || (iChange < 0 &&
			// See comment in CvCity::kill
			pWaterArea->getCitiesPerPlayer(getOwner()) > 0, true))
			pWaterArea->changeCitiesPerPlayer(getOwner(), iChange);
	} // </advc.030b>
	pArea->changePopulationPerPlayer(pCity->getOwner(), (pCity->getPopulation() * iChange));

	for (int i = 0; i < GC.getNumBuildingInfos(); i++)
	{
		BuildingTypes eBuilding = (BuildingTypes)i;
		int const iTotalChange = iChange * pCity->getNumActiveBuilding(eBuilding);
		if (iTotalChange <= 0)
			continue;
		CvBuildingInfo const& kBuilding = GC.getInfo(eBuilding);
		pArea->changePower(pCity->getOwner(), kBuilding.getPowerValue() * iTotalChange);
		if (kBuilding.getAreaHealth() > 0)
			pArea->changeBuildingGoodHealth(pCity->getOwner(), kBuilding.getAreaHealth() * iTotalChange);
		else pArea->changeBuildingBadHealth(pCity->getOwner(), kBuilding.getAreaHealth() * iTotalChange);
		pArea->changeBuildingHappiness(pCity->getOwner(), kBuilding.getAreaHappiness() * iTotalChange);
		pArea->changeFreeSpecialist(pCity->getOwner(), kBuilding.getAreaFreeSpecialist() * iTotalChange);
		pArea->changeCleanPowerCount(pCity->getTeam(), kBuilding.isAreaCleanPower() ? iTotalChange : 0);
		pArea->changeBorderObstacleCount(pCity->getTeam(), kBuilding.isAreaBorderObstacle() ? iTotalChange : 0);
		for (int j = 0; j < NUM_YIELD_TYPES; j++)
		{
			pArea->changeYieldRateModifier(pCity->getOwner(), (YieldTypes)j,
					kBuilding.getAreaYieldModifier(j) * iTotalChange);
		}
	}
	for (int i = 0; i < NUM_UNITAI_TYPES; i++)
	{
		pArea->changeNumTrainAIUnits(pCity->getOwner(), (UnitAITypes)
				i, pCity->getNumTrainUnitAI((UnitAITypes)i) * iChange);
	}
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (pArea->AI_getTargetCity((PlayerTypes)i) == pCity)
			pArea->AI_setTargetCity((PlayerTypes)i, NULL);
	}
}


ColorTypes CvPlot::plotMinimapColor()
{
	if (GC.getGame().getActivePlayer() != NO_PLAYER)
	{
		CvCity* pCity = getPlotCity();
		TeamTypes eActiveTeam = GC.getGame().getActiveTeam(); // advc
		if (pCity != NULL && pCity->isRevealed(eActiveTeam, true))
			return (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE");

		if (isActiveVisible(true)
			&& GC.getDefineINT(CvGlobals::MINIMAP_WATER_MODE) != 6) // advc.002a
		{
			CvUnit* pCenterUnit = getDebugCenterUnit();
			if (pCenterUnit != NULL)
			{
				return (ColorTypes)GC.getInfo(GET_PLAYER(pCenterUnit->
						getVisualOwner()).getPlayerColor()).getColorTypePrimary();
			}
		}
		// dlph.21: Removed !isRevealedBarbarian() clause
		if (getRevealedOwner(eActiveTeam, true) != NO_PLAYER)
		{
			return ((ColorTypes)(GC.getInfo(GET_PLAYER(getRevealedOwner(
					eActiveTeam, true)).getPlayerColor()).getColorTypePrimary()));
		}
	}

	return (ColorTypes)GC.getInfoTypeForString("COLOR_CLEAR");
}

// read object from a stream. used during load.
void CvPlot::read(FDataStreamBase* pStream)
{
	bool bVal;
	char cCount;
	int iCount;
	//reset(); // advc: Constructor handles that

	uint uiFlag=0;
	pStream->Read(&uiFlag);

	pStream->Read(&m_iX);
	pStream->Read(&m_iY);
	pStream->Read(&m_iArea);
	// (m_pPlotArea not saved)
	pStream->Read(&m_iFeatureVariety);
	pStream->Read(&m_iOwnershipDuration);
	pStream->Read(&m_iImprovementDuration);
	pStream->Read(&m_iUpgradeProgress);
	pStream->Read(&m_iForceUnownedTimer);
	// <advc.opt>
	if (uiFlag < 5)
	{
		short sTmp; pStream->Read(&sTmp);
		m_iCityRadiusCount = intToChar(sTmp);
	}
	else pStream->Read(&m_iCityRadiusCount); // </advc.opt>
	pStream->Read(&m_iRiverID);
	pStream->Read(&m_iMinOriginalStartDist);
	pStream->Read(&m_iReconCount);
	// <advc.opt>
	if (uiFlag < 5)
	{
		short sTmp; pStream->Read(&sTmp);
		m_iRiverCrossingCount = intToChar(sTmp);
	}
	else pStream->Read(&m_iRiverCrossingCount); // </advc.opt>
	// <advc.tsl>
	if(uiFlag >= 3)
	{
		if (uiFlag < 5)
		{
			short sTmp;
			pStream->Read(&sTmp);
			m_iLatitude = intToChar(sTmp);
		}
		else pStream->Read(&m_iLatitude);
	}
	else m_iLatitude = calculateLatitude(); // </advc.tsl>

	pStream->Read(&bVal);
	m_bStartingPlot = bVal;
	if(uiFlag < 4) // advc.opt: m_bHills removed
		pStream->Read(&bVal);
	pStream->Read(&bVal);
	m_bNOfRiver = bVal;
	pStream->Read(&bVal);
	m_bWOfRiver = bVal;
	pStream->Read(&bVal);
	m_bIrrigated = bVal;
	pStream->Read(&bVal);
	m_bPotentialCityWork = bVal;
	// m_bShowCitySymbols not saved
	// m_bFlagDirty not saved
	// m_bPlotLayoutDirty not saved
	// m_bLayoutStateWorked not saved

	pStream->Read(&m_eOwner);
	// <advc.opt>
	if (uiFlag >= 6)
		pStream->Read(&m_eTeam);
	else updateTeam(); // </advc.opt>
	if (uiFlag < 5)
	{
		short sTmp;
		pStream->Read(&sTmp);
		m_ePlotType = intToChar(sTmp);
		pStream->Read(&sTmp);
		m_eTerrainType = intToChar(sTmp);
		pStream->Read(&sTmp);
		m_eFeatureType = intToChar(sTmp);
	}
	else
	{ // </advc.opt>
		pStream->Read(&m_ePlotType);
		pStream->Read(&m_eTerrainType);
		pStream->Read(&m_eFeatureType);
	}
	pStream->Read(&m_eBonusType);
	// <advc.opt>
	if (uiFlag < 5)
	{
		short sTmp;
		pStream->Read(&sTmp);
		m_eImprovementType = intToChar(sTmp);
	}
	else pStream->Read(&m_eImprovementType);
	if (uiFlag < 5)
	{
		short sTmp;
		pStream->Read(&sTmp);
		m_eRouteType = intToChar(sTmp);
	}
	else pStream->Read(&m_eRouteType); // </advc.opt>
	pStream->Read(&m_eRiverNSDirection);
	pStream->Read(&m_eRiverWEDirection);
	// <advc.035>
	if(uiFlag >= 1)
		pStream->Read(&m_eSecondOwner);
	else m_eSecondOwner = m_eOwner;
	if(!GC.getDefineBOOL(CvGlobals::OWN_EXCLUSIVE_RADIUS))
		m_eSecondOwner = m_eOwner; // </advc.035>
	pStream->Read((int*)&m_plotCity.eOwner);
	pStream->Read(&m_plotCity.iID);
	pStream->Read((int*)&m_workingCity.eOwner);
	pStream->Read(&m_workingCity.iID);
	pStream->Read((int*)&m_workingCityOverride.eOwner);
	pStream->Read(&m_workingCityOverride.iID);

	m_aiYield.Read(pStream, false, uiFlag < 5);

	// BETTER_BTS_AI_MOD, Efficiency (plot danger cache), 08/21/09, jdog5000: START
	// K-Mod. I've changed the purpose of invalidateBorderDangerCache. It is no longer appropriate for this.
	//m_iActivePlayerNoBorderDangerCache = false;
	//invalidateBorderDangerCache();
	// BETTER_BTS_AI_MOD: END
	pStream->Read(&cCount);
	if (cCount > 0)
		m_aiCulture.Read(pStream);
	pStream->Read(&cCount);
	if (cCount > 0)
		m_aiFoundValue.Read(pStream, false);
	pStream->Read(&cCount);
	if (cCount > 0)
		m_aiPlayerCityRadiusCount.Read(pStream, false);
	pStream->Read(&cCount);
	if (cCount > 0)
		m_aiPlotGroup.Read(pStream);
	pStream->Read(&cCount);
	if (cCount > 0)
		m_aiVisibilityCount.Read(pStream, false);
	pStream->Read(&cCount);
	if (cCount > 0)
		m_aiStolenVisibilityCount.Read(pStream, false);
	pStream->Read(&cCount);
	if (cCount > 0)
		m_aiBlockadedCount.Read(pStream, false);
	pStream->Read(&cCount);
	if (cCount > 0)
		m_aiRevealedOwner.Read(pStream, false);
	pStream->Read(&cCount);
	if (cCount > 0)
		m_abRiverCrossing.Read(pStream);
	pStream->Read(&cCount);
	if (cCount > 0)
		m_abRevealed.Read(pStream);
	pStream->Read(&cCount);
	if (cCount > 0)
		m_aeRevealedImprovementType.Read(pStream, false, uiFlag < 5);
	pStream->Read(&cCount);
	if (cCount > 0)
		m_aeRevealedRouteType.Read(pStream, false, uiFlag < 5);

	m_szScriptData = pStream->ReadString();

	pStream->Read(&iCount);
	if (iCount > 0)
		m_aiBuildProgress.Read(pStream, false);
	// <advc.011>
	if (uiFlag < 5)
	{
		int iTmp; pStream->Read(&iTmp);
		m_iTurnsBuildsInterrupted = ::intToShort(iTmp);
	}
	else pStream->Read(&m_iTurnsBuildsInterrupted); // </advc.011>
	pStream->ReadString(m_szMostRecentCityName); // advc.005c
	// <advc.opt>
	if(uiFlag >= 2)
		pStream->Read(&m_iTotalCulture);
	else if(m_aiCulture.isAllocated()) // just to save time
	{	//m_iTotalCulture = countTotalCulture();
		/*  countTotalCulture checks CvPlayer::isEverAlive, but CvPlayer objects
			aren't loaded yet. I'm pretty sure though that the isEverAlive check
			can't make a difference in this case. */
		for(int i = 0; i < MAX_PLAYERS; i++)
			m_iTotalCulture += m_aiCulture.get((PlayerTypes)i);
	} // </advc.opt>
	pStream->Read(&cCount);
	if (cCount > 0)
		m_aaiCultureRangeCities.Read(pStream, false, true);
	pStream->Read(&cCount);
	if (cCount > 0)
		m_aaiInvisibleVisibilityCount.Read(pStream, false, true);

	m_units.Read(pStream);
}

// write object to a stream
// used during save
void CvPlot::write(FDataStreamBase* pStream)
{
	PROFILE_FUNC(); // advc
	uint uiFlag=0;
	uiFlag = 1; // advc.035
	uiFlag = 2; // advc.opt
	uiFlag = 3; // advc.tsl
	uiFlag = 4; // advc.opt: m_bHills removed
	uiFlag = 5; // advc.opt, advc.011, advc.enum: some int or short members turned into short or char
	uiFlag = 6; // advc.opt: m_eTeam
	pStream->Write(uiFlag);

	pStream->Write(m_iX);
	pStream->Write(m_iY);
	pStream->Write(m_iArea);
	// m_pPlotArea not saved
	pStream->Write(m_iFeatureVariety);
	pStream->Write(m_iOwnershipDuration);
	pStream->Write(m_iImprovementDuration);
	pStream->Write(m_iUpgradeProgress);
	pStream->Write(m_iForceUnownedTimer);
	pStream->Write(m_iCityRadiusCount);
	pStream->Write(m_iRiverID);
	pStream->Write(m_iMinOriginalStartDist);
	pStream->Write(m_iReconCount);
	pStream->Write(m_iRiverCrossingCount);
	pStream->Write(m_iLatitude); // advc.tsl

	pStream->Write(m_bStartingPlot);
	pStream->Write(m_bNOfRiver);
	pStream->Write(m_bWOfRiver);
	pStream->Write(m_bIrrigated);
	pStream->Write(m_bPotentialCityWork);
	// m_bShowCitySymbols not saved
	// m_bFlagDirty not saved
	// m_bPlotLayoutDirty not saved
	// m_bLayoutStateWorked not saved

	pStream->Write(m_eOwner);
	pStream->Write(m_eTeam); // advc.opt
	pStream->Write(m_ePlotType);
	pStream->Write(m_eTerrainType);
	pStream->Write(m_eFeatureType);
	pStream->Write(m_eBonusType);
	pStream->Write(m_eImprovementType);
	pStream->Write(m_eRouteType);
	pStream->Write(m_eRiverNSDirection);
	pStream->Write(m_eRiverWEDirection);
	pStream->Write(m_eSecondOwner); // advc.035
	pStream->Write(m_plotCity.eOwner);
	pStream->Write(m_plotCity.iID);
	pStream->Write(m_workingCity.eOwner);
	pStream->Write(m_workingCity.iID);
	pStream->Write(m_workingCityOverride.eOwner);
	pStream->Write(m_workingCityOverride.iID);

	m_aiYield.Write(pStream, false);
	if (!m_aiCulture.hasContent())
		pStream->Write((char)0);
	else
	{
		pStream->Write((char)m_aiCulture.getLength());
		m_aiCulture.Write(pStream);
	}
	if (!m_aiFoundValue.hasContent())
		pStream->Write((char)0);
	else
	{
		pStream->Write((char)m_aiFoundValue.getLength());
		m_aiFoundValue.Write(pStream, false);
	}
	if (!m_aiPlayerCityRadiusCount.hasContent())
		pStream->Write((char)0);
	else
	{
		pStream->Write((char)m_aiPlayerCityRadiusCount.getLength());
		m_aiPlayerCityRadiusCount.Write(pStream, false);
	}
	if (!m_aiPlotGroup.hasContent())
		pStream->Write((char)0);
	else
	{
		pStream->Write((char)m_aiPlotGroup.getLength());
		m_aiPlotGroup.Write(pStream);
	}
	if (!m_aiVisibilityCount.hasContent())
		pStream->Write((char)0);
	else
	{
		pStream->Write((char)m_aiVisibilityCount.getLength());
		m_aiVisibilityCount.Write(pStream, false);
	}
	if (!m_aiStolenVisibilityCount.hasContent())
		pStream->Write((char)0);
	else
	{
		pStream->Write((char)m_aiStolenVisibilityCount.getLength());
		m_aiStolenVisibilityCount.Write(pStream, false);
	}
	if (!m_aiBlockadedCount.hasContent())
		pStream->Write((char)0);
	else
	{
		pStream->Write((char)m_aiBlockadedCount.getLength());
		m_aiBlockadedCount.Write(pStream, false);
	}
	if (!m_aiRevealedOwner.hasContent())
		pStream->Write((char)0);
	else
	{
		pStream->Write((char)m_aiRevealedOwner.getLength());
		m_aiRevealedOwner.Write(pStream, false);
	}
	if (!m_abRiverCrossing.hasContent())
		pStream->Write((char)0);
	else
	{
		pStream->Write((char)m_abRiverCrossing.getLength());
		m_abRiverCrossing.Write(pStream);
	}
	if (!m_abRevealed.hasContent())
		pStream->Write((char)0);
	else
	{
		pStream->Write((char)m_abRevealed.getLength());
		m_abRevealed.Write(pStream);
	}
	if (!m_aeRevealedImprovementType.hasContent())
		pStream->Write((char)0);
	else
	{
		pStream->Write((char)m_aeRevealedImprovementType.getLength());
		m_aeRevealedImprovementType.Write(pStream, false);
	}
	if (!m_aeRevealedRouteType.hasContent())
		pStream->Write((char)0);
	else
	{
		pStream->Write((char)m_aeRevealedRouteType.getLength());
		m_aeRevealedRouteType.Write(pStream, false);
	}

	pStream->WriteString(m_szScriptData);

	if (!m_aiBuildProgress.hasContent())
		pStream->Write((int)0);
	else
	{
		pStream->Write((int)m_aiBuildProgress.getLength());
		m_aiBuildProgress.Write(pStream, false);
	}
	pStream->Write(m_iTurnsBuildsInterrupted); // advc.011
	pStream->WriteString(m_szMostRecentCityName); // advc.005c
	pStream->Write(m_iTotalCulture); // advc.opt

	if (!m_aaiCultureRangeCities.hasContent())
		pStream->Write((char)0);
	else
	{
		pStream->Write((char)m_aaiCultureRangeCities.Length());
		m_aaiCultureRangeCities.Write(pStream, false, true);
	}
	if (!m_aaiInvisibleVisibilityCount.hasContent())
		pStream->Write((char)0);
	else
	{
		pStream->Write((char)m_aaiInvisibleVisibilityCount.Length());
		m_aaiInvisibleVisibilityCount.Write(pStream, false, true);
	}

	m_units.Write(pStream);
}


void CvPlot::setLayoutDirty(bool bDirty)
{
	if (!GC.IsGraphicsInitialized())
		return;

	if (isLayoutDirty() != bDirty)
	{
		m_bPlotLayoutDirty = bDirty;
		if (isLayoutDirty() && (m_pPlotBuilder == NULL))
		{
			if (!updatePlotBuilder())
				m_bPlotLayoutDirty = false;
		}
	}
}


bool CvPlot::updatePlotBuilder()
{
	if (GC.IsGraphicsInitialized() && shouldUsePlotBuilder())
	{
		if (m_pPlotBuilder == NULL) // we need a plot builder... but it doesn't exist
		{
			m_pPlotBuilder = gDLL->getPlotBuilderIFace()->create();
			gDLL->getPlotBuilderIFace()->init(m_pPlotBuilder, this);
		}
		return true;
	}
	return false;
}


bool CvPlot::isLayoutDirty() const
{
	return m_bPlotLayoutDirty;
}


bool CvPlot::isLayoutStateDifferent() const
{
	bool bSame = true;
	bSame &= (m_bLayoutStateWorked == isBeingWorked()); // is worked
	return !bSame; // done
}


void CvPlot::setLayoutStateToCurrent()
{
	m_bLayoutStateWorked = isBeingWorked();
}


void CvPlot::getVisibleImprovementState(ImprovementTypes& eType, bool& bWorked)
{
	eType = NO_IMPROVEMENT;
	bWorked = false;

	if (GC.getGame().getActiveTeam() == NO_TEAM)
		return;

	eType = getRevealedImprovementType(GC.getGame().getActiveTeam(), true);
	if (eType == NO_IMPROVEMENT)
	{
		if (isActiveVisible(true))
		{
			if (isBeingWorked() && !isCity())
			{
				if (isWater())
					eType = (ImprovementTypes)GC.getDefineINT("WATER_IMPROVEMENT");
				else eType = (ImprovementTypes)GC.getDefineINT("LAND_IMPROVEMENT");
			}
		}
	}

	// worked state
	if (isActiveVisible(false) && isBeingWorked())
		bWorked = true;
}


void CvPlot::getVisibleBonusState(BonusTypes& eType, bool& bImproved, bool& bWorked)
{
	eType = NO_BONUS;
	bImproved = false;
	bWorked = false;

	if (GC.getGame().getActiveTeam() == NO_TEAM)
		return;

	if (GC.getGame().isDebugMode())
		eType = getBonusType();
	else if (isRevealed(GC.getGame().getActiveTeam()))
		eType = getBonusType(GC.getGame().getActiveTeam());

	if (eType != NO_BONUS) // improved and worked states ...
	{
		ImprovementTypes eRevealedImprovement = getRevealedImprovementType(GC.getGame().getActiveTeam(), true);
		if (eRevealedImprovement != NO_IMPROVEMENT &&
			GC.getInfo(eRevealedImprovement).isImprovementBonusTrade(eType))
		{
			bImproved = true;
			bWorked = isBeingWorked();
		}
	}
}


bool CvPlot::shouldUsePlotBuilder()
{
	bool bBonusImproved; bool bBonusWorked; bool bImprovementWorked;
	BonusTypes eBonusType;
	ImprovementTypes eImprovementType;
	getVisibleBonusState(eBonusType, bBonusImproved, bBonusWorked);
	getVisibleImprovementState(eImprovementType, bImprovementWorked);
	return (eBonusType != NO_BONUS || eImprovementType != NO_IMPROVEMENT);
}

/* This function has been disabled by K-Mod, because it doesn't work correctly and so using it is just a magnet for bugs.
int CvPlot::calculateMaxYield(YieldTypes eYield) const
{ // advc: body deleted  }*/

int CvPlot::getYieldWithBuild(BuildTypes eBuild, YieldTypes eYield, bool bWithUpgrade) const
{
	int iYield = 0;

	bool bIgnoreFeature = false;
	if (isFeature())
	{
		if (GC.getInfo(eBuild).isFeatureRemove(getFeatureType()))
			bIgnoreFeature = true;
	}
	int iNatureYield = // advc.908a: Preserve this for later
			calculateNatureYield(eYield, getTeam(), bIgnoreFeature);
	iYield += iNatureYield;

	ImprovementTypes eImprovement = (ImprovementTypes)GC.getInfo(eBuild).getImprovement();
	// K-Mod. if the build doesn't have its own improvement - use the existing one!
	if (eImprovement == NO_IMPROVEMENT)
	{
		eImprovement = getImprovementType();
		// note: unfortunately this won't account for the fact that some builds will destroy the existing improvement without creating a new one.
		// eg. chopping a forest which has a lumbermill. I'm sorry about that. I may correct it later.
	}
	// K-Mod end

	if (eImprovement != NO_IMPROVEMENT)
	{
		if (bWithUpgrade)
		{
			//in the case that improvements upgrade, use 2 upgrade levels higher for the
			//yield calculations.
			/*ImprovementTypes eUpgradeImprovement = (ImprovementTypes)GC.getInfo(eImprovement).getImprovementUpgrade();
			if (eUpgradeImprovement != NO_IMPROVEMENT) {
				//unless it's commerce on a low food tile, in which case only use 1 level higher
				if ((eYield != YIELD_COMMERCE) || (getYield(YIELD_FOOD) >= GC.getFOOD_CONSUMPTION_PER_POPULATION())) {
					ImprovementTypes eUpgradeImprovement2 = (ImprovementTypes)GC.getInfo(eUpgradeImprovement).getImprovementUpgrade();
					if (eUpgradeImprovement2 != NO_IMPROVEMENT)
						eUpgradeImprovement = eUpgradeImprovement2;
				}
			}
			if ((eUpgradeImprovement != NO_IMPROVEMENT) && (eUpgradeImprovement != eImprovement))
				eImprovement = eUpgradeImprovement;*/
			// <k146> Replacing the above
			// stuff that. Just use 2 levels.
			ImprovementTypes eFinalImprovement = finalImprovementUpgrade(eImprovement);
			if (eFinalImprovement != NO_IMPROVEMENT)
				eImprovement = eFinalImprovement;
			// </k146>
		}

		iYield += calculateImprovementYieldChange(eImprovement, eYield, getOwner(), false);
	}

	RouteTypes eRoute = (RouteTypes)GC.getInfo(eBuild).getRoute();
	if (eRoute != NO_ROUTE)
	{
		//eImprovement = getImprovementType(); // disabled by K-Mod
		if (eImprovement != NO_IMPROVEMENT)
		{
			for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
			{
				iYield += GC.getInfo(eImprovement).getRouteYieldChanges(eRoute, iI);
				if (getRouteType() != NO_ROUTE)
				{
					iYield -= GC.getInfo(eImprovement).getRouteYieldChanges(getRouteType(), iI);
				}
			}
		}
	}

	// K-Mod. Count the 'extra yield' for financial civs. (Don't bother with golden-age bonuses.)
	int iThreshold = GET_PLAYER(getOwner()).getExtraYieldThreshold(eYield);
	if (iThreshold > 0 &&
				(iYield > iThreshold || iNatureYield >= iThreshold)) // advc.908a
		iYield += GC.getDefineINT(CvGlobals::EXTRA_YIELD);
	// K-Mod end

	//return iYield;
	return std::max(0, iYield); // K-Mod - so that it matches calculateYield()
}


bool CvPlot::canTrigger(EventTriggerTypes eTrigger, PlayerTypes ePlayer) const
{
	FAssert(::isPlotEventTrigger(eTrigger));

	CvEventTriggerInfo& kTrigger = GC.getInfo(eTrigger);
	if (kTrigger.isOwnPlot() && getOwner() != ePlayer)
		return false;

	if (kTrigger.getPlotType() != NO_PLOT)
	{
		if (getPlotType() != kTrigger.getPlotType())
			return false;
	}

	if (kTrigger.getNumFeaturesRequired() > 0)
	{
		bool bFoundValid = false;
		for (int i = 0; i < kTrigger.getNumFeaturesRequired(); ++i)
		{
			if (kTrigger.getFeatureRequired(i) == getFeatureType())
			{
				bFoundValid = true;
				break;
			}
		}
		if (!bFoundValid)
			return false;
	}
	if (kTrigger.getNumTerrainsRequired() > 0)
	{
		bool bFoundValid = false;
		for (int i = 0; i < kTrigger.getNumTerrainsRequired(); ++i)
		{
			if (kTrigger.getTerrainRequired(i) == getTerrainType())
			{
				bFoundValid = true;
				break;
			}
		}
		if (!bFoundValid)
			return false;
	}
	if (kTrigger.getNumImprovementsRequired() > 0)
	{
		bool bFoundValid = false;
		for (int i = 0; i < kTrigger.getNumImprovementsRequired(); ++i)
		{
			if (kTrigger.getImprovementRequired(i) == getImprovementType())
			{
				bFoundValid = true;
				break;
			}
		}
		if (!bFoundValid)
			return false;
	}
	if (kTrigger.getNumBonusesRequired() > 0)
	{
		bool bFoundValid = false;
		for (int i = 0; i < kTrigger.getNumBonusesRequired(); ++i)
		{
			if (kTrigger.getBonusRequired(i) == getBonusType(kTrigger.isOwnPlot() ?
				GET_PLAYER(ePlayer).getTeam() : NO_TEAM))
			{
				bFoundValid = true;
				break;
			}
		}
		if (!bFoundValid)
			return false;
	}
	if (kTrigger.getNumRoutesRequired() > 0)
	{
		bool bFoundValid = false;
		if (getPlotCity() == NULL)
		{
			for (int i = 0; i < kTrigger.getNumRoutesRequired(); ++i)
			{
				if (kTrigger.getRouteRequired(i) == getRouteType())
				{
					bFoundValid = true;
					break;
				}
			}
		}
		if (!bFoundValid)
			return false;
	}
	if (kTrigger.isUnitsOnPlot())
	{
		bool bFoundValid = false;
		CLLNode<IDInfo>* pUnitNode = headUnitNode();
		while (NULL != pUnitNode)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);
			if (pLoopUnit->getOwner() == ePlayer)
			{
				if (pLoopUnit->getTriggerValue(eTrigger, this, false) != -1)
				{
					bFoundValid = true;
					break;
				}
			}
		}
		if (!bFoundValid)
			return false;
	}
	if (kTrigger.isPrereqEventCity() && kTrigger.getNumPrereqEvents() > 0)
	{
		bool bFoundValid = true;
		for (int iI = 0; iI < kTrigger.getNumPrereqEvents(); ++iI)
		{
			const EventTriggeredData* pTriggeredData = GET_PLAYER(ePlayer).
					getEventOccured((EventTypes)kTrigger.getPrereqEvent(iI));
			if (NULL == pTriggeredData || pTriggeredData->m_iPlotX != getX() ||
				pTriggeredData->m_iPlotY != getY())
			{
				bFoundValid = false;
				break;
			}
		}
		if (!bFoundValid)
			return false;
	}

	return true;
}


bool CvPlot::canApplyEvent(EventTypes eEvent) const
{
	CvEventInfo& kEvent = GC.getInfo(eEvent);
	if (kEvent.getFeatureChange() > 0)
	{
		if (NO_FEATURE != kEvent.getFeature())
		{
			if (NO_IMPROVEMENT != getImprovementType() || !canHaveFeature((FeatureTypes)kEvent.getFeature()))
				return false;
		}
	}
	else if (kEvent.getFeatureChange() < 0)
	{
		if (NO_FEATURE == getFeatureType())
			return false;
	}
	if (kEvent.getImprovementChange() > 0)
	{
		if (NO_IMPROVEMENT != kEvent.getImprovement())
		{
			if (!canHaveImprovement((ImprovementTypes)kEvent.getImprovement(), getTeam()))
				return false;
		}
	}
	else if (kEvent.getImprovementChange() < 0)
	{
		if (NO_IMPROVEMENT == getImprovementType())
			return false;
	}
	if (kEvent.getBonusChange() > 0)
	{
		if (NO_BONUS != kEvent.getBonus())
		{
			if (!canHaveBonus((BonusTypes)kEvent.getBonus(), false))
				return false;
		}
	}
	else if (kEvent.getBonusChange() < 0)
	{
		if (NO_BONUS == getBonusType())
			return false;
	}
	if (kEvent.getRouteChange() < 0)
	{
		if (NO_ROUTE == getRouteType())
			return false;

		if (isCity())
			return false;
	}

	return true;
}


void CvPlot::applyEvent(EventTypes eEvent)
{
	CvEventInfo& kEvent = GC.getInfo(eEvent);
	if (kEvent.getFeatureChange() > 0)
	{
		if (NO_FEATURE != kEvent.getFeature())
			setFeatureType((FeatureTypes)kEvent.getFeature());
	}
	else if (kEvent.getFeatureChange() < 0)
		setFeatureType(NO_FEATURE);

	if (kEvent.getImprovementChange() > 0)
	{
		if (NO_IMPROVEMENT != kEvent.getImprovement())
			setImprovementType((ImprovementTypes)kEvent.getImprovement());
	}
	else if (kEvent.getImprovementChange() < 0)
		setImprovementType(NO_IMPROVEMENT);

	if (kEvent.getBonusChange() > 0)
	{
		if (NO_BONUS != kEvent.getBonus())
			setBonusType((BonusTypes)kEvent.getBonus());
	}
	else if (kEvent.getBonusChange() < 0)
		setBonusType(NO_BONUS);

	if (kEvent.getRouteChange() > 0)
	{
		if (NO_ROUTE != kEvent.getRoute())
			setRouteType((RouteTypes)kEvent.getRoute(), true);
	}
	else if (kEvent.getRouteChange() < 0)
		setRouteType(NO_ROUTE, true);

	for (int i = 0; i < NUM_YIELD_TYPES; ++i)
	{
		int iChange = kEvent.getPlotExtraYield(i);
		if (iChange != 0)
			GC.getGame().setPlotExtraYield(m_iX, m_iY, (YieldTypes)i, iChange);
	}
}


bool CvPlot::canTrain(UnitTypes eUnit, bool bContinue, bool bTestVisible,
	bool bCheckAirUnitCap, // advc.001b
	BonusTypes eAssumeAvailable) const // advc.001u
{
	CvCity* pCity = getPlotCity();

	if (GC.getInfo(eUnit).isPrereqReligion())
	{
		if (pCity == NULL || //pCity->getReligionCount() > 0)
				pCity->getReligionCount() == 0) // K-Mod
			return false;
	}
	if (GC.getInfo(eUnit).getPrereqReligion() != NO_RELIGION)
	{
		if (NULL == pCity || !pCity->isHasReligion((ReligionTypes)
				GC.getInfo(eUnit).getPrereqReligion()))
			return false;
	}
	if (GC.getInfo(eUnit).getPrereqCorporation() != NO_CORPORATION)
	{
		if (NULL == pCity || !pCity->isActiveCorporation((CorporationTypes)
				GC.getInfo(eUnit).getPrereqCorporation()))
			return false;
	}

	/*if (GC.getInfo(eUnit).isPrereqBonuses()) {
		// ...
	}
	if (isCity()) {
		// ...
	}
	else if (area()->getNumTiles() < GC.getInfo(eUnit).getMinAreaSize())
		return false;
	/*  <advc.041> Replacing the above (moved into CvCityAI::AI_bestUnitAI). I.e.
		treat MinAreaSize and PrereqBonuses as mere recommendations (for the AI)
		rather than game rules.
		NB: MinAreaSize is still enforced as a rule for buildings.
		The last clause above should perhaps be checked by the AI before
		upgrading units; but AI sea units can end up in small water areas only via
		WorldBuilder [or possibly teleport -> advc.046], so I'm not bothering with this. */
	if(isCity())
	{
		if(GC.getInfo(eUnit).getDomainType() == DOMAIN_SEA && !isWater() &&
				!isCoastalLand())
			return false;
	} else // </advc.041>
	{
		if (GC.getInfo(eUnit).getDomainType() == DOMAIN_SEA)
		{
			if (!isWater())
				return false;
		}
		else if (GC.getInfo(eUnit).getDomainType() == DOMAIN_LAND)
		{
			if (isWater())
				return false;
		}
		else // advc (comment): Upgrade air units only in cities
			return false;
	}
	if (!bTestVisible)
	{
		if (GC.getInfo(eUnit).getHolyCity() != NO_RELIGION)
		{
			if (NULL == pCity || !pCity->isHolyCity((ReligionTypes)
					GC.getInfo(eUnit).getHolyCity()))
				return false;
		}

		if (GC.getInfo(eUnit).getPrereqBuilding() != NO_BUILDING)
		{
			if (NULL == pCity)
				return false;
			BuildingTypes ePrereqBuilding = (BuildingTypes)GC.getInfo(eUnit).getPrereqBuilding(); // advc
			if (pCity->getNumBuilding(ePrereqBuilding) == 0)
			{
				SpecialBuildingTypes eSpecialBuilding = (SpecialBuildingTypes)
						GC.getInfo(ePrereqBuilding).getSpecialBuildingType();
				if (eSpecialBuilding == NO_SPECIALBUILDING ||
						!GET_PLAYER(getOwner()).isSpecialBuildingNotRequired(eSpecialBuilding))
					return false;
			}
		}
		BonusTypes ePrereqAndBonus = (BonusTypes)GC.getInfo(eUnit).getPrereqAndBonus(); // advc
		if(ePrereqAndBonus != NO_BONUS
			&& ePrereqAndBonus != eAssumeAvailable) // advc.001u
		{
			if (NULL == pCity)
			{
				if (!isPlotGroupConnectedBonus(getOwner(), ePrereqAndBonus))
					return false;
			}
			else if (!pCity->hasBonus(ePrereqAndBonus))
				return false;
		}

		bool bRequiresBonus = false;
		bool bNeedsBonus = true;
		for (int iI = 0; iI < GC.getNUM_UNIT_PREREQ_OR_BONUSES(eUnit); ++iI)
		{
			BonusTypes ePrereqOrBonus = (BonusTypes)GC.getInfo(eUnit).getPrereqOrBonuses(iI); // advc
			if(ePrereqOrBonus != NO_BONUS &&
					ePrereqOrBonus != eAssumeAvailable) // advc.001u
			{
				bRequiresBonus = true;
				if (NULL == pCity)
				{
					if (isPlotGroupConnectedBonus(getOwner(), ePrereqOrBonus))
					{
						bNeedsBonus = false;
						break;
					}
				}
				else
				{
					if (pCity->hasBonus(ePrereqOrBonus))
					{
						bNeedsBonus = false;
						break;
					}
				}
			}
		}
		if (bRequiresBonus && bNeedsBonus)
			return false;
	}

	// <advc.001b> Enforce air unit cap
	if(bCheckAirUnitCap && !bTestVisible && GC.getInfo(eUnit).getAirUnitCap() > 0 &&
			airUnitSpaceAvailable(getTeam()) < 1)
		return false; // </advc.001b>

	return true;
}


int CvPlot::countFriendlyCulture(TeamTypes eTeam) const
{
	int iTotal = 0;
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kLoopPlayer.isEverAlive()) // advc.099: was isAlive
		{
			CvTeam& kLoopTeam = GET_TEAM(kLoopPlayer.getTeam());
			if (kLoopPlayer.getTeam() == eTeam || kLoopTeam.isVassal(eTeam) ||
				kLoopTeam.isOpenBorders(eTeam))
			{
				iTotal += getCulture((PlayerTypes)iPlayer);
			}
		}
	}
	return iTotal;
}


int CvPlot::countNumAirUnits(TeamTypes eTeam) const
{
	int iCount = 0;
	for (CLLNode<IDInfo> const* pUnitNode = headUnitNode(); pUnitNode != NULL;
		pUnitNode = nextUnitNode(pUnitNode))
	{
		CvUnit const* pLoopUnit = ::getUnit(pUnitNode->m_data);
		if (DOMAIN_AIR == pLoopUnit->getDomainType() && !pLoopUnit->isCargo() &&
			pLoopUnit->getTeam() == eTeam)
		{
			iCount += GC.getInfo(pLoopUnit->getUnitType()).getAirUnitCap();
		}
	}
	return iCount;
}


int CvPlot::airUnitSpaceAvailable(TeamTypes eTeam) const
{
	int iMaxUnits = 0;
	CvCity* pCity = getPlotCity();
	if (NULL != pCity)
		iMaxUnits = pCity->getAirUnitCapacity(getTeam());
	else iMaxUnits = GC.getDefineINT(CvGlobals::CITY_AIR_UNIT_CAPACITY);
	return (iMaxUnits - countNumAirUnits(eTeam));
}

// <advc.081> Cut from CvPlayerAI::AI_countNumAreaHostileUnits
int CvPlot::countAreaHostileUnits(PlayerTypes ePlayer, CvArea* pArea, bool bPlayer,
	bool bTeam, bool bNeutral, bool bHostile) const
{
	if(area() != pArea)
		return 0;
	TeamTypes eTeam = TEAMID(ePlayer);
	if(!isVisible(eTeam, false))
		return 0;
	if((bPlayer && getOwner() == ePlayer) ||
		(bTeam && getTeam() == eTeam) || (bNeutral && !isOwned()) ||
		(bHostile && isOwned() && GET_TEAM(eTeam).isAtWar(getTeam())))
	{
		return plotCount(PUF_isEnemy, ePlayer, false, NO_PLAYER, NO_TEAM,
				PUF_isVisible, ePlayer);
	}
	return 0;
} // </advc.081>


bool CvPlot::isEspionageCounterSpy(TeamTypes eTeam) const
{
	CvCity* pCity = getPlotCity();
	if (NULL != pCity && pCity->getTeam() == eTeam)
	{
		if (pCity->getEspionageDefenseModifier() > 0)
			return true;
	}
	return plotCheck(PUF_isCounterSpy, -1, -1, NO_PLAYER, eTeam); // advc.opt: was plotCount...>0
}


int CvPlot::getAreaIdForGreatWall() const
{
	return getArea();
}


int CvPlot::getSoundScriptId() const
{
	int iScriptId = -1;
	if (isActiveVisible(true))
	{
		if (getImprovementType() != NO_IMPROVEMENT)
			iScriptId = GC.getInfo(getImprovementType()).getWorldSoundscapeScriptId();
		else if (isFeature())
			iScriptId = GC.getInfo(getFeatureType()).getWorldSoundscapeScriptId();
		else if (getTerrainType() != NO_TERRAIN)
			iScriptId = GC.getInfo(getTerrainType()).getWorldSoundscapeScriptId();
	}
	return iScriptId;
}


int CvPlot::get3DAudioScriptFootstepIndex(int iFootstepTag) const
{
	if (isFeature())
		return GC.getInfo(getFeatureType()).get3DAudioScriptFootstepIndex(iFootstepTag);

	if (getTerrainType() != NO_TERRAIN)
		return GC.getInfo(getTerrainType()).get3DAudioScriptFootstepIndex(iFootstepTag);

	return -1;
}


float CvPlot::getAqueductSourceWeight() const
{
	float fWeight = 0.0f;
	if (isLake() || isPeak() ||
		(isFeature() && GC.getInfo(getFeatureType()).isAddsFreshWater()))
	{
		fWeight = 1.0f;
	}
	else if (isHills())
		fWeight = 0.67f;

	return fWeight;
}


bool CvPlot::shouldDisplayBridge(CvPlot* pToPlot, PlayerTypes ePlayer) const
{
	TeamTypes eObservingTeam = GET_PLAYER(ePlayer).getTeam();
	TeamTypes eOurTeam = getRevealedTeam(eObservingTeam, true);
	TeamTypes eOtherTeam = NO_TEAM;
	if (pToPlot != NULL)
		eOtherTeam = pToPlot->getRevealedTeam(eObservingTeam, true);

	if (eOurTeam == eObservingTeam || eOtherTeam == eObservingTeam ||
		(eOurTeam == NO_TEAM && eOtherTeam == NO_TEAM))
	{
		return GET_TEAM(eObservingTeam).isBridgeBuilding();
	}

	if (eOurTeam == NO_TEAM)
		return GET_TEAM(eOtherTeam).isBridgeBuilding();

	if (eOtherTeam == NO_TEAM)
		return GET_TEAM(eOurTeam).isBridgeBuilding();

	return (GET_TEAM(eOurTeam).isBridgeBuilding() && GET_TEAM(eOtherTeam).isBridgeBuilding());
}


bool CvPlot::checkLateEra() const
{
	PlayerTypes eBestPlayer = getOwner();
	if (eBestPlayer == NO_PLAYER)
	{
		eBestPlayer = GC.getGame().getActivePlayer();
		int iBestCulture = getCulture(eBestPlayer);
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			int iLoopCulture = getCulture((PlayerTypes) i);
			if (iLoopCulture > iBestCulture)
			{
				iBestCulture = iLoopCulture;
				eBestPlayer = (PlayerTypes)i;
			}
		}
	}
	return (GET_PLAYER(eBestPlayer).getCurrentEra() > GC.getNumEraInfos() / 2);
}

// <advc.300>
void CvPlot::killRandomUnit(PlayerTypes eOwner, DomainTypes eDomain)
{
	CvUnit* pVictim = NULL;
	CLLNode<IDInfo>* pUnitNode = headUnitNode();
	int iBestValue = -1;
	while(pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);
		if(pLoopUnit->getOwner() == eOwner && pLoopUnit->getDomainType() == eDomain)
		{
			int iValue = GC.getGame().getSorenRandNum(1000, "advc.300:killRandomUnit");
			if(iValue > iBestValue)
			{
				pVictim = pLoopUnit;
				iBestValue = iValue;
			}
		}
	}
	if(pVictim != NULL)
		pVictim->kill(false);
} // </advc.300>


// BETTER_BTS_AI_MOD, Lead From Behind (UncutDragon), 02/21/10, jdog5000: START
bool CvPlot::hasDefender(bool bTestCanAttack, PlayerTypes eOwner, PlayerTypes eAttackingPlayer,
	CvUnit const* pAttacker, bool bTestEnemy, bool bTestPotentialEnemy) const
{
	/*  advc: BBAI had repeated parts of getBestDefender here. To avoid that, I've
		moved bTestAttack into getBestDefender and gave that function a "bAny" param. */
	return (getBestDefender(eOwner, eAttackingPlayer, pAttacker, bTestEnemy,
			bTestPotentialEnemy, false, bTestCanAttack, /*bAny=*/true) != NULL);
} // BETTER_BTS_AI_MOD: END

// <advc.500a>
bool CvPlot::isConnectRiverSegments() const
{
#if 1
	return false; // disabled for now
#else
	bool cr[NUM_DIRECTION_TYPES];
	for(int i = 0; i < NUM_DIRECTION_TYPES; i++) {
		DirectionTypes dir = (DirectionTypes)i;
		cr[i] = isRiverCrossing(dir);
	}
	/*  River crossings in two directions and no connection between the
		two segments along this tile */
	return ((cr[DIRECTION_WEST] && cr[DIRECTION_EAST] &&
			    !cr[DIRECTION_NORTH] && !cr[DIRECTION_SOUTH]) ||
		   (cr[DIRECTION_NORTH] && cr[DIRECTION_SOUTH] &&
				!cr[DIRECTION_WEST] && !cr[DIRECTION_EAST]) ||
		   (cr[DIRECTION_SOUTHWEST] && cr[DIRECTION_NORTHEAST] &&
			    !(cr[DIRECTION_WEST] && cr[DIRECTION_NORTH]) &&
				!(cr[DIRECTION_EAST] && cr[DIRECTION_SOUTH])) ||
		   (cr[DIRECTION_SOUTHEAST] && cr[DIRECTION_NORTHWEST] &&
			    !(cr[DIRECTION_WEST] && cr[DIRECTION_SOUTH]) &&
				!(cr[DIRECTION_EAST] && cr[DIRECTION_NORTH])) ||
		   (cr[DIRECTION_NORTHWEST] && cr[DIRECTION_NORTHEAST] &&
				!cr[DIRECTION_NORTH] && !cr[DIRECTION_SOUTH]) ||
		   (cr[DIRECTION_SOUTHWEST] && cr[DIRECTION_NORTHWEST] &&
				!cr[DIRECTION_WEST] && !cr[DIRECTION_EAST]) ||
		   (cr[DIRECTION_SOUTHWEST] && cr[DIRECTION_SOUTHEAST] &&
				!cr[DIRECTION_SOUTH] && !cr[DIRECTION_NORTH]) ||
		   (cr[DIRECTION_SOUTHEAST] && cr[DIRECTION_NORTHEAST] &&
				!cr[DIRECTION_EAST] && !cr[DIRECTION_WEST]));
#endif
} // </advc.500a>

// <advc.121>
bool CvPlot::isConnectSea() const
{
	if(isWater())
		return false;
	/* Circle through the adjacent plots clockwise, looking for changes
	   from land to sea or vice versa. */
	static DirectionTypes eClockwise[NUM_DIRECTION_TYPES] = {
		DIRECTION_NORTH, DIRECTION_NORTHEAST, DIRECTION_EAST,
		DIRECTION_SOUTHEAST, DIRECTION_SOUTH, DIRECTION_SOUTHWEST,
		DIRECTION_WEST, DIRECTION_NORTHWEST
	};
	bool bPreviousSea = false;
	int iChanges = 0;
	for(int i = 0; i < NUM_DIRECTION_TYPES; i++)
	{
		CvPlot* pPlot = plotDirection(getX(), getY(), eClockwise[i]);
		if(pPlot == NULL)
			continue;
		bool bSea = pPlot->isWater() && !pPlot->isLake();
		if(bSea != bPreviousSea && i > 0)
			iChanges++;
		if(iChanges > 2)
			return true;
		bPreviousSea = bSea;
	}
	return false;
	/* Checking for a connection between different water areas would be easy enough
	   to do, but shortening paths within a water area can also be valuable. */
} // </advc.121>
