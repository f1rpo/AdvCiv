//	FILE:	 CvMap.cpp
//	AUTHOR:  Soren Johnson
//	PURPOSE: Game map class
//-----------------------------------------------------------------------------
//	Copyright (c) 2004 Firaxis Games, Inc. All rights reserved.
//-----------------------------------------------------------------------------


#include "CvGameCoreDLL.h"
#include "CvMap.h"
#include "PlotRange.h"
#include "CvArea.h"
#include "CvGame.h"
#include "CvPlayer.h"
#include "CvCity.h"
#include "CvUnit.h"
#include "CvSelectionGroup.h"
#include "CvPlotGroup.h"
#include "CvFractal.h"
#include "CvMapGenerator.h"
#include "GroupPathFinder.h"
#include "FAStarFunc.h"
#include "FAStarNode.h"
#include "CvInfo_Terrain.h" // advc.pf (for pathfinder initialization)
#include "CvInfo_GameOption.h"
#include "CvReplayInfo.h" // advc.106n
#include "CvDLLIniParserIFaceBase.h"


CvMap::CvMap()
{
	CvMapInitData defaultMapData;
	m_pMapPlots = NULL;
	reset(&defaultMapData);
}


CvMap::~CvMap()
{
	uninit();
}

/*	Initializes the map
	pInitInfo  - Optional init structure (used for WB load) */
void CvMap::init(CvMapInitData* pInitInfo)
{
	PROFILE("CvMap::init");
	gDLL->logMemState(CvString::format("CvMap::init begin - world size=%s, climate=%s, sealevel=%s, num custom options=%6",
			GC.getInfo(GC.getInitCore().getWorldSize()).getDescription(),
			GC.getInfo(GC.getInitCore().getClimate()).getDescription(),
			GC.getInfo(GC.getInitCore().getSeaLevel()).getDescription(),
			GC.getInitCore().getNumCustomMapOptions()).c_str());
	GC.getPythonCaller()->callMapFunction("beforeInit");

	reset(pInitInfo); // Init serialized data
	m_areas.init(); // Init containers
	setup();
	gDLL->logMemState("CvMap before init plots");
	FAssert(numPlots() <= (EnumMap<PlotNumTypes,scaled>::MAX_LENGTH)); // advc.enum
	m_pMapPlots = new CvPlot[numPlots()];
	for (int iX = 0; iX < getGridWidth(); iX++)
	{
		gDLL->callUpdater();
		for (int iY = 0; iY < getGridHeight(); iY++)
		{
			getPlot(iX, iY).init(iX, iY);
		}
	}
	// <advc.opt>
	FOR_EACH_ENUM(PlotNum)
		getPlotByIndex(eLoopPlotNum).initAdjList(); // </advc.opt>
	calculateAreas();
	gDLL->logMemState("CvMap after init plots");
}


void CvMap::uninit()
{
	SAFE_DELETE_ARRAY(m_pMapPlots);
	m_replayTexture.clear(); // advc.106n
	m_areas.uninit();
	CvSelectionGroup::uninitPathFinder(); // advc.pf
}

// Initializes data members that are serialized.
void CvMap::reset(CvMapInitData* pInitInfo)
{
	uninit();

	// set grid size
	// initially set in terrain cell units
	m_iGridWidth = (GC.getInitCore().getWorldSize() != NO_WORLDSIZE) ?
			GC.getInfo(GC.getInitCore().getWorldSize()).getGridWidth() : 0; //todotw:tcells wide
	m_iGridHeight = (GC.getInitCore().getWorldSize() != NO_WORLDSIZE) ?
			GC.getInfo(GC.getInitCore().getWorldSize()).getGridHeight() : 0;

	// allow grid size override
	if (pInitInfo != NULL)
	{
		m_iGridWidth	= pInitInfo->m_iGridW;
		m_iGridHeight	= pInitInfo->m_iGridH;
	}
	else
	{
		WorldSizeTypes eWorldSize = GC.getInitCore().getWorldSize();
		if (eWorldSize != NO_WORLDSIZE)
		{
			// check map script for grid size override
			GC.getPythonCaller()->mapGridDimensions(eWorldSize, m_iGridWidth, m_iGridHeight);
		}

		// convert to plot dimensions
		if (GC.getNumLandscapeInfos() > 0)
		{	/*  advc.003x: A bit of code moved into new CvGlobals functions
				in order to remove the dependency of CvMap on CvLandscapeInfos */
			m_iGridWidth *= GC.getLandscapePlotsPerCellX();
			m_iGridHeight *= GC.getLandscapePlotsPerCellY();
		}
	}
	updatePlotNum(); // advc.opt

	m_iLandPlots = 0;
	m_iOwnedPlots = 0;

	if (pInitInfo)
	{
		m_iTopLatitude = pInitInfo->m_iTopLatitude;
		m_iBottomLatitude = pInitInfo->m_iBottomLatitude;
	}
	else
	{
		// Check map script for latitude override (map script beats ini file)
		GC.getPythonCaller()->mapLatitudeExtremes(m_iTopLatitude, m_iBottomLatitude);
	}

	m_iTopLatitude = std::min(m_iTopLatitude, 90);
	m_iTopLatitude = std::max(m_iTopLatitude, -90);
	m_iBottomLatitude = std::min(m_iBottomLatitude, 90);
	m_iBottomLatitude = std::max(m_iBottomLatitude, -90);

	m_iNextRiverID = 0;

	//
	// set wrapping
	//
	m_bWrapX = true;
	m_bWrapY = false;
	if (pInitInfo)
	{
		m_bWrapX = pInitInfo->m_bWrapX;
		m_bWrapY = pInitInfo->m_bWrapY;
	}
	else
	{
		// Check map script for wrap override (map script beats ini file)
		GC.getPythonCaller()->mapWraps(m_bWrapX, m_bWrapY);
	}

	m_aiNumBonus.reset();
	m_aiNumBonusOnLand.reset();

	m_areas.removeAll();
}

// Initializes all data that is not serialized but needs to be initialized after loading
void CvMap::setup()
{
	PROFILE_FUNC();

	CvDLLFAStarIFaceBase& kAStar = *gDLL->getFAStarIFace(); // advc
	kAStar.Initialize(&GC.getPathFinder(),
			getGridWidth(),	getGridHeight(),isWrapX(),	isWrapY(),
			pathDestValid,	pathHeuristic,	pathCost,	pathValid,
			pathAdd,		NULL,			NULL);
	kAStar.Initialize(&GC.getInterfacePathFinder(),
			getGridWidth(),	getGridHeight(),isWrapX(),	isWrapY(),
			pathDestValid,	pathHeuristic,	pathCost,	pathValid,
			pathAdd,		NULL,			NULL);
	kAStar.Initialize(&GC.getStepFinder(),
			getGridWidth(),	getGridHeight(),isWrapX(),	isWrapY(),
			stepDestValid,	stepHeuristic,	stepCost,	stepValid,
			stepAdd,		NULL,			NULL);
	kAStar.Initialize(&GC.getRouteFinder(),
			getGridWidth(), getGridHeight(), isWrapX(), isWrapY(),
			NULL,			NULL,			NULL,		routeValid,
			NULL,			NULL,			NULL);
	kAStar.Initialize(&GC.getBorderFinder(),
			getGridWidth(), getGridHeight(), isWrapX(), isWrapY(),
			NULL,			NULL,			NULL,		borderValid,
			NULL,			NULL,			NULL);
	kAStar.Initialize(&GC.getAreaFinder(),
			getGridWidth(), getGridHeight(), isWrapX(), isWrapY(),
			NULL,			NULL,			NULL,		areaValid,
			NULL,			joinArea,		NULL);
	kAStar.Initialize(&GC.getPlotGroupFinder(),
			getGridWidth(), getGridHeight(), isWrapX(), isWrapY(),
			NULL,			NULL,			NULL,		plotGroupValid,
			NULL,			countPlotGroup,	NULL);
	// advc (note): IrrigatedFinder gets instantiated in updateIrrigated
	// <advc.pf>
	CvSelectionGroup::initPathFinder();
	// Moved this computation out of KmodPathFinder.h to avoid a header inclusion
	int iMinMovementCost = MAX_INT;
	int iMinFlatMovementCost = MAX_INT;
	FOR_EACH_ENUM(Route)
	{
		CvRouteInfo const& kLoopRoute = GC.getInfo(eLoopRoute);
		int iCost = kLoopRoute.getMovementCost();
		FOR_EACH_ENUM(Tech)
		{
			if (kLoopRoute.getTechMovementChange(eLoopTech) < 0)
				iCost += kLoopRoute.getTechMovementChange(eLoopTech);
		}
		iMinMovementCost = std::min(iMinMovementCost, iCost);
		iMinFlatMovementCost = std::min(iMinFlatMovementCost,
				kLoopRoute.getFlatMovementCost());
	}
	GroupPathFinder::initHeuristicWeights(
			iMinMovementCost, iMinFlatMovementCost); // </advc.pf>
}


void CvMap::setupGraphical() // graphical only setup
{
	if (!GC.IsGraphicsInitialized())
		return;

	CvPlot::setMaxVisibilityRangeCache(); // advc.003h
	if (m_pMapPlots != NULL)
	{
		for (int iI = 0; iI < numPlots(); iI++)
		{
			gDLL->callUpdater(); // allow windows msgs to update
			getPlotByIndex(iI).setupGraphical();
		}
	}
	// <advc.106n> For games starting in a later era
	if (getReplayTexture() == NULL &&
		GC.getGame().getHighestEra() >= GC.getDefineINT("REPLAY_TEXTURE_ERA"))
	{
		// The EXE isn't quite ready here to provide the texture
		GC.getGame().setUpdateTimer(CvGame::UPDATE_STORE_REPLAY_TEXTURE, 5);
	} // </advc.106n>
}


void CvMap::erasePlots()
{
	for (int iI = 0; iI < numPlots(); iI++)
		plotByIndex(iI)->erase();
	m_replayTexture.clear(); // advc.106n
}


void CvMap::setRevealedPlots(TeamTypes eTeam, bool bNewValue, bool bTerrainOnly)
{
	PROFILE_FUNC();

	for (int iI = 0; iI < numPlots(); iI++)
	{
		getPlotByIndex(iI).setRevealed(eTeam, bNewValue, bTerrainOnly, NO_TEAM, false);
	}

	GC.getGame().updatePlotGroups();
}


void CvMap::setAllPlotTypes(PlotTypes ePlotType)
{
	//float startTime = (float) timeGetTime();

	for(int i = 0; i < numPlots(); i++)
	{
		getPlotByIndex(i).setPlotType(ePlotType, false, false);
	}

	recalculateAreas();

	//rebuild landscape
	gDLL->getEngineIFace()->RebuildAllPlots();

	//mark minimap as dirty
	gDLL->getEngineIFace()->SetDirty(MinimapTexture_DIRTY_BIT, true);
	gDLL->getEngineIFace()->SetDirty(GlobeTexture_DIRTY_BIT, true);

	//float endTime = (float) timeGetTime();
	//printToConsole(CvString::format("[Jason] setAllPlotTypes: %f\n", endTime - startTime).c_str());
}

// XXX generalize these funcs? (macro?)
void CvMap::doTurn()
{
	//PROFILE("CvMap::doTurn()"); // advc.003o
	for(int iI = 0; iI < numPlots(); iI++)
		getPlotByIndex(iI).doTurn();
}


void CvMap::updateFlagSymbols()
{
	PROFILE_FUNC();

	for (int iI = 0; iI < numPlots(); iI++)
	{
		CvPlot& kPlot = getPlotByIndex(iI);
		if (kPlot.isFlagDirty())
		{
			kPlot.updateFlagSymbol();
			kPlot.setFlagDirty(false);
		}
	}
}

// K-Mod:
void CvMap::setFlagsDirty()
{
	for (int i = 0; i < numPlots(); i++)
		plotByIndex(i)->setFlagDirty(true);
}


void CvMap::updateFog()
{
	for(int iI = 0; iI < numPlots(); iI++)
		getPlotByIndex(iI).updateFog();
}


void CvMap::updateVisibility()
{
	for (int iI = 0; iI < numPlots(); iI++)
		getPlotByIndex(iI).updateVisibility();
}


void CvMap::updateSymbolVisibility()
{
	for(int iI = 0; iI < numPlots(); iI++)
		getPlotByIndex(iI).updateSymbolVisibility();
}


void CvMap::updateSymbols()
{
	for(int iI = 0; iI < numPlots(); iI++)
		getPlotByIndex(iI).updateSymbols();
}


void CvMap::updateMinimapColor()
{
	for(int iI = 0; iI < numPlots(); iI++)
		getPlotByIndex(iI).updateMinimapColor();
}


void CvMap::updateSight(bool bIncrement)
{
	for (int iI = 0; iI < numPlots(); iI++)
		getPlotByIndex(iI).updateSight(bIncrement, false);

	GC.getGame().updatePlotGroups();
}


void CvMap::updateIrrigated()
{
	for(int i = 0; i < numPlots(); i++)
		updateIrrigated(getPlotByIndex(i));
}

/*	K-Mod. This function is called when the unit selection is changed
	or when a selected unit is promoted. (Or when UnitInfo_DIRTY_BIT is set.)
	The purpose is to update which unit is displayed in the center of each plot.
	The original implementation simply updated every plot on the map. This is
	a bad idea because it scales badly for big maps, and the update function
	on each plot can be expensive. The new functionality attempts to only
	update plots that are in movement range of the selected group;
	with a very generous approximation for what might be in range. */
void CvMap::updateCenterUnit()
{
	/*for (int iI = 0; iI < numPlots(); iI++)
		getPlotByIndex(iI).updateCenterUnit();*/ // BtS
	PROFILE_FUNC();
	int iRange = -1;

	for (CLLNode<IDInfo> const* pNode = gDLL->UI().headSelectionListNode();
		pNode != NULL; pNode = gDLL->UI().nextSelectionListNode(pNode))
	{
		CvUnit const& kLoopUnit = *::getUnit(pNode->m_data);
		//if (kLoopUnit.getDomainType() == DOMAIN_AIR)
		if (kLoopUnit.airRange() > 0) // advc.rstr
			iRange = std::max(iRange, kLoopUnit.airRange());
		DomainTypes eLoopDomain = kLoopUnit.getDomainType();
		if (eLoopDomain == DOMAIN_LAND || eLoopDomain == DOMAIN_SEA) // advc.rstr
		{
			int iStepCost = (eLoopDomain == DOMAIN_LAND ?
					GroupPathFinder::minimumStepCost(kLoopUnit.baseMoves()) :
					GC.getMOVE_DENOMINATOR());
			int iMoveRange = kLoopUnit.maxMoves() / iStepCost +
					(kLoopUnit.canParadrop(kLoopUnit.plot()) ?
					kLoopUnit.getDropRange() : 0);
			iRange = std::max(iRange, iMoveRange);
		}
		/*  Note: technically we only really need the minimum range; but I'm using the maximum range
			because I think it will produce more intuitive and useful information for the player. */
	}

	if (iRange < 0 || iRange*iRange > numPlots() / 2)
	{
		// update the whole map
		for (int i = 0; i < numPlots(); i++)
			getPlotByIndex(i).updateCenterUnit();
	}
	else
	{
		// only update within the range
		CvPlot* pCenterPlot = gDLL->UI().getHeadSelectedUnit()->plot();
		for (SquareIter it(*pCenterPlot, iRange); it.hasNext(); ++it)
		{
			it->updateCenterUnit();
		}
	}
} // K-Mod end


void CvMap::updateWorkingCity()
{
	for (int iI = 0; iI < numPlots(); iI++)
		getPlotByIndex(iI).updateWorkingCity();
}


void CvMap::updateMinOriginalStartDist(CvArea const& kArea)
{
	PROFILE_FUNC();

	for (int iI = 0; iI < numPlots(); iI++)
	{
		CvPlot& kPlot = getPlotByIndex(iI);
		if (kPlot.isArea(kArea))
			kPlot.setMinOriginalStartDist(-1);
	}

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlot* pStartingPlot = GET_PLAYER((PlayerTypes)iI).getStartingPlot();
		if (pStartingPlot == NULL || !pStartingPlot->isArea(kArea))
			continue;
		for (int iJ = 0; iJ < numPlots(); iJ++)
		{
			CvPlot& kPlot = getPlotByIndex(iJ);
			if (kPlot.isArea(kArea) && /* K-Mod: */ &kPlot != pStartingPlot)
			{
				//iDist = calculatePathDistance(pStartingPlot, pLoopPlot);
				int iDist = stepDistance(pStartingPlot, &kPlot);
				if (iDist != -1)
				{
					//int iCrowDistance = plotDistance(pStartingPlot, pLoopPlot);
					//iDist = std::min(iDist,  iCrowDistance * 2);
					if (kPlot.getMinOriginalStartDist() == -1 ||
						iDist < kPlot.getMinOriginalStartDist())
					{
						kPlot.setMinOriginalStartDist(iDist);
					}
				}
			}
		}
	}
}


void CvMap::updateYield()
{
	for (int iI = 0; iI < numPlots(); iI++)
		getPlotByIndex(iI).updateYield();
}


void CvMap::verifyUnitValidPlot()
{
	for (int iI = 0; iI < numPlots(); iI++)
		getPlotByIndex(iI).verifyUnitValidPlot();
}


void CvMap::combinePlotGroups(PlayerTypes ePlayer, CvPlotGroup* pPlotGroup1, CvPlotGroup* pPlotGroup2,
	bool bVerifyProduction) // advc.064d
{
	FAssert(pPlotGroup1 != NULL);
	FAssert(pPlotGroup2 != NULL);

	if(pPlotGroup1 == pPlotGroup2)
		return;

	CvPlotGroup* pNewPlotGroup;
	CvPlotGroup* pOldPlotGroup;

	if (pPlotGroup1->getLengthPlots() > pPlotGroup2->getLengthPlots())
	{
		pNewPlotGroup = pPlotGroup1;
		pOldPlotGroup = pPlotGroup2;
	}
	else
	{
		pNewPlotGroup = pPlotGroup2;
		pOldPlotGroup = pPlotGroup1;
	}

	CLLNode<XYCoords>* pPlotNode = pOldPlotGroup->headPlotsNode();
	while (pPlotNode != NULL)
	{
		CvPlot& kPlot = getPlot(pPlotNode->m_data.iX, pPlotNode->m_data.iY);
		pNewPlotGroup->addPlot(&kPlot, /* advc.064d: */ bVerifyProduction);
		pPlotNode = pOldPlotGroup->deletePlotsNode(pPlotNode);
	}
}


CvPlot* CvMap::syncRandPlot(RandPlotFlags eFlags, CvArea const* pArea,
	int iMinCivUnitDistance, // advc.300: Renamed from iMinUnitDistance
	int iTimeout,
	int* piValidCount) // advc.304: Number of valid tiles
{
	/*  <advc.304> Look exhaustively for a valid plot by default. Rationale:
		The biggest maps have about 10000 plots. If there is only one valid plot,
		then the BtS default of considering 100 plots drawn at random has only a
		(ca.) 1% chance of success. 10000 trials - slower than exhaustive search! -
		still have only a 63% chance of success. Also bear in mind that the
		1 plot in 10000 could be 1 of just 3 plots in the given pArea (or the only
		plot there), so not exactly a needle in a haystack from the caller's pov. */
	if (iTimeout < 0)
	{
		std::vector<CvPlot*> apValidPlots;
		for(int i = 0; i < numPlots(); i++)
		{
			CvPlot& kPlot = getPlotByIndex(i);
			if (isValidRandPlot(kPlot, eFlags, pArea, iMinCivUnitDistance))
				apValidPlots.push_back(&kPlot);
		}
		int iValid = (int)apValidPlots.size();
		if(piValidCount != NULL)
			*piValidCount = iValid;
		if(iValid == 0)
			return NULL;
		return apValidPlots[GC.getGame().getSorenRandNum(iValid, "advc.304")];
	}
	FAssert(iTimeout != 0);
	/*  BtS code (refactored): Limited number of trials
		(can be faster or slower than the above; that's not really the point) */
	// </advc.304>
	for (int i = 0; i < iTimeout; i++)
	{
		CvPlot& kTestPlot = getPlot(
				GC.getGame().getSorenRandNum(getGridWidth(), "Rand Plot Width"),
				GC.getGame().getSorenRandNum(getGridHeight(), "Rand Plot Height"));
		if (isValidRandPlot(kTestPlot, eFlags, pArea, iMinCivUnitDistance))
		{	/*  <advc.304> Not useful, but want to make sure it doesn't stay
				uninitialized. 1 since we found only 1 valid plot. */
			if(piValidCount != NULL)
				*piValidCount = 1; // </advc.304>
			return &kTestPlot;
		}
	} // <advc.304>
	if(piValidCount != NULL)
		*piValidCount = 0; // </advc.304>
	return NULL;
}

// advc: Body cut from syncRandPlot
bool CvMap::isValidRandPlot(CvPlot const& kPlot, RandPlotFlags eFlags,
	CvArea const* pArea, int iMinCivUnitDistance) const
{
	if (pArea != NULL && !kPlot.isArea(*pArea))
		return false;
	/*  advc.300: Code moved into new function isCivUnitNearby;
		Barbarians in surrounding plots are now ignored. */
	if(iMinCivUnitDistance >= 0 && (kPlot.isUnit() ||
		kPlot.isCivUnitNearby(iMinCivUnitDistance)))
	{
		return false;
	}
	if ((eFlags & RANDPLOT_LAND) && kPlot.isWater())
		return false;
	if ((eFlags & RANDPLOT_UNOWNED) && kPlot.isOwned())
		return false;
	if ((eFlags & RANDPLOT_ADJACENT_UNOWNED) && kPlot.isAdjacentOwned())
		return false;
	if ((eFlags & RANDPLOT_ADJACENT_LAND) && !kPlot.isAdjacentToLand())
		return false;
	if ((eFlags & RANDPLOT_PASSABLE) && kPlot.isImpassable())
		return false;
	if ((eFlags & RANDPLOT_NOT_VISIBLE_TO_CIV) && kPlot.isVisibleToCivTeam())
		return false;
	if ((eFlags & RANDPLOT_NOT_CITY) && kPlot.isCity())
		return false;
	// <advc.300>
	if((eFlags & RANDPLOT_HABITABLE) && kPlot.getYield(YIELD_FOOD) <= 0)
		return false;
	if((eFlags & RANDPLOT_WATERSOURCE) && !kPlot.isFreshWater() &&
		kPlot.getYield(YIELD_FOOD) <= 0)
	{
		return false;
	} // </advc.300>
	return true;
}


CvCity* CvMap::findCity(int iX, int iY, PlayerTypes eOwner, TeamTypes eTeam,
		bool bSameArea, bool bCoastalOnly, TeamTypes eTeamAtWarWith,
		DirectionTypes eDirection, CvCity const* pSkipCity, // advc: const city
		TeamTypes eObserver) const // advc.004r
{
	PROFILE_FUNC();

	int iBestValue = MAX_INT;
	CvCity* pBestCity = NULL;
	for (int i = // <advc.opt> Don't go through all players if eOwner is given
		(eOwner != NO_PLAYER ? eOwner : 0);
		i < (eOwner != NO_PLAYER ? eOwner + 1 : // </advc.opt>
		MAX_PLAYERS); i++) // XXX look for barbarian cities???
	{
		/*if (eOwner != NO_PLAYER && i != eOwner)
			continue;*/
		CvPlayer const& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
		if (!kLoopPlayer.isAlive())
			continue;
		if (eTeam != NO_TEAM && kLoopPlayer.getTeam() != eTeam)
			continue;

		FOR_EACH_CITY_VAR(pLoopCity, kLoopPlayer)
		{	// <advc.004r>
			if(eObserver != NO_TEAM && !pLoopCity->isRevealed(eObserver))
				continue; // </advc.004r>
			if (!bSameArea || pLoopCity->isArea(getPlot(iX, iY).getArea())  ||
				(bCoastalOnly && pLoopCity->waterArea() == getPlot(iX, iY).area()))
			{
				if ((!bCoastalOnly || pLoopCity->isCoastal()) &&
					(eTeamAtWarWith == NO_TEAM || ::atWar(kLoopPlayer.getTeam(), eTeamAtWarWith)) &&
					(eDirection == NO_DIRECTION || estimateDirection(
					dxWrap(pLoopCity->getX() - iX), dyWrap(pLoopCity->getY() - iY)) == eDirection) &&
					(pSkipCity == NULL || pLoopCity != pSkipCity))
				{
					int iValue = plotDistance(iX, iY, pLoopCity->getX(), pLoopCity->getY());
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


CvSelectionGroup* CvMap::findSelectionGroup(int iX, int iY, PlayerTypes eOwner,
	bool bReadyToSelect, bool bWorkers) const
{
	int iBestValue = MAX_INT;
	CvSelectionGroup* pBestSelectionGroup = NULL;
	for (int i =  // <advc.opt> Don't go through all players if eOwner is given
		(eOwner != NO_PLAYER ? eOwner : 0);
		i < (eOwner != NO_PLAYER ? eOwner + 1 : // </advc.opt>
		MAX_PLAYERS); i++) // XXX look for barbarian groups???
	{
		/*if (eOwner != NO_PLAYER && i != eOwner)
			continue;*/
		CvPlayer const& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
		if (!kLoopPlayer.isAlive())
			continue;

		FOR_EACH_GROUP_VAR(pLoopSelectionGroup, kLoopPlayer)
		{
			bool const bAIControl = pLoopSelectionGroup->AI_isControlled(); // advc.153
			if (bReadyToSelect && !pLoopSelectionGroup->readyToSelect(
				!bAIControl)) // advc.153: was false
			{
				continue;
			}
			if (bWorkers &&
				// advc.153: with moves
				!pLoopSelectionGroup->hasWorkerWithMoves())
			{
				continue;
			}
			int iValue = plotDistance(iX, iY,
					pLoopSelectionGroup->getX(), pLoopSelectionGroup->getY());
			/*	<advc.153> Select groups where only some units are ready last
				(unless plotDistance is 0) */
			if (!bAIControl && pLoopSelectionGroup->readyToSelect())
				iValue *= 100; // </advc.153>
			if (iValue < iBestValue)
			{
				iBestValue = iValue;
				pBestSelectionGroup = pLoopSelectionGroup;
			}
		}
	}
	return pBestSelectionGroup;
}


CvArea* CvMap::findBiggestArea(bool bWater)
{
	int iBestValue = 0;
	CvArea* pBestArea = NULL;
	FOR_EACH_AREA_VAR(pLoopArea)
	{
		if (pLoopArea->isWater() == bWater)
		{
			int iValue = pLoopArea->getNumTiles();
			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				pBestArea = pLoopArea;
			}
		}
	}
	return pBestArea;
}


int CvMap::getMapFractalFlags() const
{
	CvFractal::Flags eWrapX = CvFractal::NO_FLAGS;
	if (isWrapX())
		eWrapX = CvFractal::FRAC_WRAP_X;

	CvFractal::Flags eWrapY = CvFractal::NO_FLAGS;
	if (isWrapY())
		eWrapY = CvFractal::FRAC_WRAP_Y;
	/*	(advc.enum: Convert to int. It's only used in Python anyway, and
		don't want to include CvFractal.h in CvMap.h.) */
	return (eWrapX | eWrapY);
}

// Check plots for wetlands or seaWater. Returns true if found
bool CvMap::findWater(CvPlot const* pPlot, int iRange, bool bFreshWater) // advc: const CvPlot*
{
	PROFILE_FUNC();

	for (SquareIter it(*pPlot, iRange); it.hasNext(); ++it)
	{
		CvPlot const& kLoopPlot = *it;
		if (bFreshWater)
		{
			if (kLoopPlot.isFreshWater())
				return true;
		}
		else if (kLoopPlot.isWater())
			return true;
	}
	return false;
}


bool CvMap::isPlotExternal(int iX, int iY) const // advc.inl
{
	return isPlot(iX, iY);
}


int CvMap::numPlotsExternal() const // advc.inl
{
	return numPlots();
}


int CvMap::plotX(int iIndex) const
{
	return iIndex % getGridWidth();
}


int CvMap::plotY(int iIndex) const
{
	return iIndex / getGridWidth();
}


int CvMap::pointXToPlotX(float fX) /* advc: */ const
{
	float fWidth, fHeight;
	gDLL->getEngineIFace()->GetLandscapeGameDimensions(fWidth, fHeight);
	return (int)(((fX + fWidth/2) / fWidth) * getGridWidth());
}


float CvMap::plotXToPointX(int iX) /* advc: */ const
{
	float fWidth, fHeight;
	gDLL->getEngineIFace()->GetLandscapeGameDimensions(fWidth, fHeight);
	return ((iX * fWidth) / getGridWidth()) - fWidth/2 + GC.getPLOT_SIZE()/2;
}


int CvMap::pointYToPlotY(float fY) /* advc: */ const
{
	float fWidth, fHeight;
	gDLL->getEngineIFace()->GetLandscapeGameDimensions(fWidth, fHeight);
	return (int)(((fY + fHeight/2) / fHeight) * getGridHeight());
}


float CvMap::plotYToPointY(int iY) /* advc: */ const
{
	float fWidth, fHeight;
	gDLL->getEngineIFace()->GetLandscapeGameDimensions(fWidth, fHeight);
	return ((iY * fHeight) / getGridHeight()) - fHeight/2 + GC.getPLOT_SIZE()/2;
}


float CvMap::getWidthCoords() const
{
	return GC.getPLOT_SIZE() * getGridWidth();
}


float CvMap::getHeightCoords() const
{
	return GC.getPLOT_SIZE() * getGridHeight();
}


int CvMap::maxPlotDistance() const
{
	return std::max(1, plotDistance(0, 0,
			isWrapX() ? getGridWidth() / 2 : getGridWidth() - 1,
			isWrapY() ? getGridHeight() / 2 : getGridHeight() - 1));
}


int CvMap::maxStepDistance() const
{
	return std::max(1, stepDistance(0, 0,
			isWrapX() ? getGridWidth() / 2 : getGridWidth() - 1,
			isWrapY() ? getGridHeight() / 2 : getGridHeight() - 1));
}

// advc.140:
int CvMap::maxMaintenanceDistance() const
{
	return ::round(1 + maxTypicalDistance() * (10.0 /
			GC.getDefineINT(CvGlobals::MAX_DISTANCE_CITY_MAINTENANCE)));
}

/*	advc.140: Not sure what distance this measures exactly; I'm using it as a
	replacement (everyhwere) for maxPlotDistance with reduced impact of world wraps. */
int CvMap::maxTypicalDistance() const
{
	CvGame const& kGame = GC.getGame();
	double civRatio = kGame.getRecommendedPlayers() / (double)kGame.getCivPlayersEverAlive();
	// Already factored into getRecommendedPlayers, but I want to give it a little extra weight.
	double seaLvlModifier = (100 - 2.5 * kGame.getSeaLevelChange()) / 100.0;
	int iWraps = -1; // 0 if cylindrical (1 wrap), -1 flat, +1 toroidical
	if(isWrapX())
		iWraps++;
	if(isWrapY())
		iWraps++;
	CvWorldInfo const& kWorld = GC.getInfo(getWorldSize());
	double r = std::sqrt(kWorld.getGridWidth() * kWorld.getGridHeight() * civRatio *
			seaLvlModifier) * 3.5 - 5 * iWraps;
	return std::max(1, ::round(r));
}


int CvMap::getGridWidthExternal() const // advc.inl
{
	return getGridWidth();
}


int CvMap::getGridHeightExternal() const // advc.inl
{
	return getGridHeight();
}


void CvMap::changeLandPlots(int iChange)
{
	m_iLandPlots = (m_iLandPlots + iChange);
	FAssert(getLandPlots() >= 0);
}


void CvMap::changeOwnedPlots(int iChange)
{
	m_iOwnedPlots = (m_iOwnedPlots + iChange);
	FAssert(getOwnedPlots() >= 0);
}


int CvMap::getTopLatitude() const
{
	return m_iTopLatitude;
}


int CvMap::getBottomLatitude() const
{
	return m_iBottomLatitude;
}


short CvMap::getNextRiverID() const
{
	return m_iNextRiverID;
}


void CvMap::incrementNextRiverID()
{
	m_iNextRiverID++;
}


bool CvMap::isWrapXExternal() // advc.inl
{
	return isWrapX();
}


bool CvMap::isWrapYExternal() // advc.inl
{
	return isWrapY();
}

bool CvMap::isWrapExternal() // advc.inl
{
	return isWrap();
}


int CvMap::getNumCustomMapOptions() const
{
	return GC.getInitCore().getNumCustomMapOptions();
}


CustomMapOptionTypes CvMap::getCustomMapOption(int iOption) /* advc: */ const
{
	return GC.getInitCore().getCustomMapOption(iOption);
}

// advc.190b: Returns an empty string if the option is set to its default value
CvWString CvMap::getNonDefaultCustomMapOptionDesc(int iOption) const
{
	CvPythonCaller const& py = *GC.getPythonCaller();
	CvString szMapScriptNameNarrow(GC.getInitCore().getMapScriptName());
	CustomMapOptionTypes eOptionValue = getCustomMapOption(iOption);
	int iDefaultValue = py.customMapOptionDefault(szMapScriptNameNarrow.c_str(), iOption);
	// Negative value means that default couldn't be loaded (script may have been removed)
	if (iDefaultValue < 0 || eOptionValue == iDefaultValue)
		return L"";
	return py.customMapOptionDescription(szMapScriptNameNarrow.c_str(), iOption, eOptionValue);
}


int CvMap::getNumBonuses(BonusTypes eIndex) const
{
	return m_aiNumBonus.get(eIndex);
}


void CvMap::changeNumBonuses(BonusTypes eIndex, int iChange)
{
	m_aiNumBonus.add(eIndex, iChange);
	FAssert(getNumBonuses(eIndex) >= 0);
}


int CvMap::getNumBonusesOnLand(BonusTypes eIndex) const
{
	return m_aiNumBonusOnLand.get(eIndex);
}


void CvMap::changeNumBonusesOnLand(BonusTypes eIndex, int iChange)
{
	m_aiNumBonusOnLand.add(eIndex, iChange);
	FAssert(getNumBonusesOnLand(eIndex) >= 0);
}


CvPlot* CvMap::plotByIndexExternal(int iIndex) const // advc.inl
{
	return plotByIndex(iIndex);
}


CvPlot* CvMap::plotExternal(int iX, int iY) const // advc.inl
{
	return plot(iX, iY);
}


CvPlot* CvMap::pointToPlot(float fX, float fY)
{
	return plot(pointXToPlotX(fX), pointYToPlotY(fY));
}


int CvMap::getIndexAfterLastArea() const
{
	return m_areas.getIndexAfterLast();
}


int CvMap::getNumLandAreas() const
{
	int iNumLandAreas = 0;
	FOR_EACH_AREA(pLoopArea)
	{
		if (!pLoopArea->isWater())
			iNumLandAreas++;
	}
	return iNumLandAreas;
}


CvArea* CvMap::addArea()
{
	return m_areas.add();
}


void CvMap::deleteArea(int iID)
{
	m_areas.removeAt(iID);
}


void CvMap::recalculateAreas(/* advc.opt: */bool bUpdateIsthmuses)
{
	PROFILE_FUNC();
	// <advc.opt>
	if (bUpdateIsthmuses)
	{
		for (int i = 0; i < numPlots(); i++)
			getPlotByIndex(i).updateAnyIsthmus();
	} // </advc.opt>
	for (int i = 0; i < numPlots(); i++)
		getPlotByIndex(i).setArea(NULL);
	m_areas.removeAll();
	calculateAreas();
}


void CvMap::resetPathDistance()
{
	gDLL->getFAStarIFace()->ForceReset(&GC.getStepFinder());
}


int CvMap::calculatePathDistance(CvPlot const* pSource, CvPlot const* pDest) const
{
	if(pSource == NULL || pDest == NULL)
		return -1;
	if (gDLL->getFAStarIFace()->GeneratePath(&GC.getStepFinder(),
		pSource->getX(), pSource->getY(), pDest->getX(), pDest->getY(), false, 0, true))
	{
		FAStarNode* pNode = gDLL->getFAStarIFace()->GetLastNode(&GC.getStepFinder());
		if (pNode != NULL)
			return pNode->m_iData1;
	}
	return -1; // no passable path exists
}

/*	advc.pf: Cut from CvPlot::updateIrrigated
	so that all the non-unit FAStar stuff is in one place */
void CvMap::updateIrrigated(CvPlot& kPlot)
{
	PROFILE_FUNC();

	if (!GC.getGame().isFinalInitialized())
		return;

	FAStar* pIrrigatedFinder = gDLL->getFAStarIFace()->create();
	if (kPlot.isIrrigated())
	{
		if (!kPlot.isPotentialIrrigation())
		{
			kPlot.setIrrigated(false);
			FOR_EACH_ADJ_PLOT(kPlot)
			{
				bool bFoundFreshWater = false;
				gDLL->getFAStarIFace()->Initialize(pIrrigatedFinder,
						getGridWidth(), getGridHeight(),
						isWrapX(), isWrapY(), NULL, NULL, NULL,
						potentialIrrigation, NULL, checkFreshWater,
						&bFoundFreshWater);
				gDLL->getFAStarIFace()->GeneratePath(pIrrigatedFinder,
						pAdj->getX(), pAdj->getY(), -1, -1);
				if (!bFoundFreshWater)
				{
					bool bIrrigated = false;
					gDLL->getFAStarIFace()->Initialize(pIrrigatedFinder,
							getGridWidth(), getGridHeight(),
							isWrapX(), isWrapY(), NULL, NULL, NULL,
							/*	advc (note): GeneratePath will cause the
								changeIrrigated function to perform the
								updates by calling CvPlot::setIrrigated */
							potentialIrrigation, NULL, changeIrrigated, &bIrrigated);
					gDLL->getFAStarIFace()->GeneratePath(pIrrigatedFinder,
							pAdj->getX(), pAdj->getY(), -1, -1);
				}
			}
		}
	}
	else if (kPlot.isPotentialIrrigation() && kPlot.isIrrigationAvailable(true))
	{
		bool bIrrigated = true;
		gDLL->getFAStarIFace()->Initialize(pIrrigatedFinder,
				getGridWidth(), getGridHeight(),
				isWrapX(), isWrapY(), NULL, NULL, NULL,
				potentialIrrigation, NULL, changeIrrigated, &bIrrigated);
		gDLL->getFAStarIFace()->GeneratePath(pIrrigatedFinder,
				kPlot.getX(), kPlot.getY(), -1, -1);
	}

	gDLL->getFAStarIFace()->destroy(pIrrigatedFinder);
}


// BETTER_BTS_AI_MOD, Efficiency (plot danger cache), 08/21/09, jdog5000: START
void CvMap::invalidateActivePlayerSafeRangeCache()
{
	PROFILE_FUNC();

	for (int iI = 0; iI < numPlots(); iI++)
		getPlotByIndex(iI).setActivePlayerSafeRangeCache(-1);
}


void CvMap::invalidateBorderDangerCache(TeamTypes eTeam)
{
	PROFILE_FUNC();

	for(int iI = 0; iI < numPlots(); iI++)
		getPlotByIndex(iI).setBorderDangerCache(eTeam, false);
} // BETTER_BTS_AI_MOD: END

// read object from a stream. used during load
void CvMap::read(FDataStreamBase* pStream)
{
	CvMapInitData defaultMapData;

	reset(&defaultMapData);

	uint uiFlag=0;
	pStream->Read(&uiFlag);

	pStream->Read(&m_iGridWidth);
	pStream->Read(&m_iGridHeight);
	// <advc.opt>
	if (uiFlag >= 3)
		pStream->Read((int*)&m_ePlots);
	else updatePlotNum(); // </advc.opt>
	pStream->Read(&m_iLandPlots);
	pStream->Read(&m_iOwnedPlots);
	pStream->Read(&m_iTopLatitude);
	pStream->Read(&m_iBottomLatitude);
	// <advc.opt>
	int iRiver;
	pStream->Read(&iRiver);
	if (iRiver < MIN_SHORT || iRiver > MAX_SHORT)
		m_iNextRiverID = -1;
	else m_iNextRiverID = static_cast<short>(iRiver);
	// </advc.opt>
	pStream->Read(&m_bWrapX);
	pStream->Read(&m_bWrapY);

	m_aiNumBonus.Read(pStream);
	m_aiNumBonusOnLand.Read(pStream);

	if (numPlots() > 0)
	{
		m_pMapPlots = new CvPlot[numPlots()];
		for (int i = 0; i < numPlots(); i++)
			m_pMapPlots[i].read(pStream);
		// <advc.opt>
		for (int i = 0; i < numPlots(); i++)
			m_pMapPlots[i].initAdjList();
		if (uiFlag < 2)
		{
			for (int i = 0; i < numPlots(); i++)
				m_pMapPlots[i].updateAnyIsthmus();
		} // </advc.opt>
	}

	ReadStreamableFFreeListTrashArray(m_areas, pStream);
	// <advc> Let the plots know that the areas have been loaded
	for (int i = 0; i < numPlots(); i++)
	{
		plotByIndex(i)->initArea();
	} // </advc>
	setup();
	computeShelves(); // advc.300
	/*  advc.004z: Not sure if this is the ideal place for this, but it works.
		(The problem was that goody huts weren't always highlighted by the
		Resource layer after loading a game.) */
	gDLL->UI().setDirty(GlobeLayer_DIRTY_BIT, true);
	// <advc.106n>
	if (uiFlag > 0)
	{
		size_t iPixels;
		pStream->Read(&iPixels);
		m_replayTexture.reserve(iPixels);
		for (size_t i = 0; i < iPixels; i++)
		{
			byte ucPixel;
			pStream->Read(&ucPixel);
			m_replayTexture.push_back(ucPixel);
		}
	} // </advc.106n>
}


void CvMap::write(FDataStreamBase* pStream)
{
	REPRO_TEST_BEGIN_WRITE("Map");
	uint uiFlag;
	//uiFlag = 1; // advc.106n
	//uiFlag = 2; // advc.opt: CvPlot::m_bAnyIsthmus
	uiFlag = 3; // advc.opt: m_ePlots
	pStream->Write(uiFlag);

	pStream->Write(m_iGridWidth);
	pStream->Write(m_iGridHeight);
	pStream->Write(m_ePlots);
	pStream->Write(m_iLandPlots);
	pStream->Write(m_iOwnedPlots);
	pStream->Write(m_iTopLatitude);
	pStream->Write(m_iBottomLatitude);
	pStream->Write((int)m_iNextRiverID); // advc.opt (cast)

	pStream->Write(m_bWrapX);
	pStream->Write(m_bWrapY);

	FAssertMsg((0 < GC.getNumBonusInfos()), "GC.getNumBonusInfos() is not greater than zero but an array is being allocated");
	m_aiNumBonus.Write(pStream);
	m_aiNumBonusOnLand.Write(pStream);
	REPRO_TEST_END_WRITE();
	for (int iI = 0; iI < numPlots(); iI++)
		m_pMapPlots[iI].write(pStream);

	WriteStreamableFFreeListTrashArray(m_areas, pStream);
	// <advc.106n>
	pStream->Write(m_replayTexture.size());
	pStream->Write(m_replayTexture.size(), &m_replayTexture[0]);
	// </advc.106n>
}

// used for loading WB maps
void CvMap::rebuild(int iGridW, int iGridH, int iTopLatitude, int iBottomLatitude, bool bWrapX, bool bWrapY, WorldSizeTypes eWorldSize, ClimateTypes eClimate, SeaLevelTypes eSeaLevel, int iNumCustomMapOptions, CustomMapOptionTypes * aeCustomMapOptions)
{
	CvMapInitData initData(iGridW, iGridH, iTopLatitude, iBottomLatitude, bWrapX, bWrapY);

	// Set init core data
	CvInitCore& kInitCore = GC.getInitCore();
	kInitCore.setWorldSize(eWorldSize);
	kInitCore.setClimate(eClimate);
	kInitCore.setSeaLevel(eSeaLevel);
	kInitCore.setCustomMapOptions(iNumCustomMapOptions, aeCustomMapOptions);

	init(&initData); // Init map
}

// advc.opt:
void CvMap::updatePlotNum()
{
	m_ePlots = (PlotNumTypes)(getGridWidth() * getGridHeight());
}

// <advc.106n>
void CvMap::updateReplayTexture()
{
	byte* pTexture = gDLL->UI().getMinimapBaseTexture();
	FAssert(pTexture != NULL);
	if (pTexture == NULL)
		return;
	int const iPixels = CvReplayInfo::minimapPixels(GC.getDefineINT(CvGlobals::MINIMAP_RENDER_SIZE));
	m_replayTexture.clear();
	m_replayTexture.reserve(iPixels);
	for (int i = 0; i < iPixels; i++)
		m_replayTexture.push_back(pTexture[i]);
}


byte const* CvMap::getReplayTexture() const
{
	// When in HoF or updateReplayTexture was never called or MINIMAP_RENDER_SIZE has changed
	if (m_replayTexture.size() != CvReplayInfo::minimapPixels(
		GC.getDefineINT(CvGlobals::MINIMAP_RENDER_SIZE)))
	{
		return NULL;
	}
	return &m_replayTexture[0];
} // </advc.106n>


void CvMap::calculateAreas()
{
	PROFILE("CvMap::calculateAreas"); // <advc.030>
	if(GC.getDefineINT("PASSABLE_AREAS") > 0)
	{
		/*  Will recalculate from CvGame::setinitialItems once normalization is
			through. But need preliminary areas because normalization is done
			based on areas. Also, some scenarios don't call CvGame::
			setInitialItems; these only get the initial calculation based on
			land, sea and peaks (not ice). */
		calculateAreas_030();
		calculateReprAreas();
		return;
	} // </advc.030>
	for (int i = 0; i < numPlots(); i++)
	{
		CvPlot& kLoopPlot = getPlotByIndex(i);
		gDLL->callUpdater();
		if (kLoopPlot.area() == NULL)
		{
			CvArea* pArea = addArea();
			pArea->init(kLoopPlot.isWater());
			kLoopPlot.setArea(pArea);
			gDLL->getFAStarIFace()->GeneratePath(&GC.getAreaFinder(),
					kLoopPlot.getX(), kLoopPlot.getY(), -1, -1,
					kLoopPlot.isWater(), pArea->getID());
		}
	}
	updateLakes(); // advc.030
}

// <advc.030>
void CvMap::calculateAreas_030()
{
	for(int iPass = 0; iPass <= 1; iPass++)
	{
		for(int i = 0; i < numPlots(); i++)
		{
			CvPlot& p = getPlotByIndex(i);
			if(iPass == 0)
			{
				/*  Second pass for impassables; can't handle
					all-peak/ice areas otherwise. */
				if(p.isImpassable())
					continue;
			}
			if(p.area() != NULL)
				continue;
			FAssert(iPass == 0 || p.isImpassable());
			CvArea& a = *addArea();
			a.init(p.isWater());
			p.setArea(&a);
			calculateAreas_DFS(p);
			gDLL->callUpdater(); // Allow UI to update
		}
	}
}


void CvMap::updateLakes()
{
	// CvArea::getNumTiles no longer sufficient for identifying lakes
	FOR_EACH_AREA_VAR(a)
		a->updateLake();
	for(int i = 0; i < numPlots(); i++)
	{
		CvPlot& kPlot = getPlotByIndex(i);
		if(kPlot.isLake())
			kPlot.updateYield();
	}
	computeShelves(); // advc.300
}


void CvMap::calculateReprAreas()
{
	/*  Still need areas as in BtS for submarine movement. Store at each CvArea
		an area id representing all areas that would be encompassed by the same
		BtS area. To decide if a submarine move is possible, only need to
		check if the representative id of the submarine's current area equals
		that of its target area. That's done in CvArea::canBeEntered. */
	int iLoop = 0;
	int iReprChanged = 0; // For debugging; otherwise a bool would suffice.
	do
	{
		iReprChanged = 0;
		for(int i = 0; i < numPlots(); i++)
		{
			CvPlot& p = getPlotByIndex(i);
			int const x = p.getX();
			int const y = p.getY();
			FOR_EACH_ADJ_PLOT_VAR2(pAdjacent, p)
			{
				CvPlot& q = *pAdjacent;
				// Only orthogonal adjacency for water tiles
				if(p.isWater() && x != q.getX() && y != q.getY())
					continue;
				int const pReprArea = p.getArea().getRepresentativeArea();
				int const qReprArea = q.getArea().getRepresentativeArea();
				if(pReprArea != qReprArea && p.isWater() == q.isWater())
				{
					if(qReprArea < pReprArea)
						p.getArea().setRepresentativeArea(qReprArea);
					else q.getArea().setRepresentativeArea(pReprArea);
					iReprChanged++;
				}
			}
		}
		if(++iLoop > 10)
		{
			FAssert(iLoop <= 10);
			/*  Will have to write a faster algorithm then, based on the BtS code at
				the beginning of this function. That would also make it easier to
				set the lakes. */
			break;
		}
	} while(iReprChanged > 0);
	updateLakes();
}


void CvMap::calculateAreas_DFS(CvPlot const& kStart)
{
	/*  Explicit stack b/c memory can be an issue if a map has dimensions
		considerably larger than Huge and very large areas.
		I've run out of memory with a recursive implementation (with an attached
		debugger) after about 20000 calls on a 148x148 map generated by LPlate2's
		Eyeball Planet script. With the stack, at least the Release build should
		be pretty safe. */
	std::stack<CvPlot const*> stack;
	stack.push(&kStart);
	while(!stack.empty())
	{
		CvPlot const& p = *stack.top();
		stack.pop();
		FOR_EACH_ADJ_PLOT_VAR2(pAdjacent, p)
		{
			CvPlot& q = *pAdjacent;
			if(q.area() == NULL && p.isWater() == q.isWater() &&
				!isSeparatedByIsthmus(p, q) &&
				/*  Depth-first search that doesn't continue at impassables
					except to other impassables so that mountain ranges and
					ice packs end up in one CvArea. */
				(!p.isImpassable() || q.isImpassable()))
			{
				q.setArea(p.area());
				stack.push(&q);
			}
		}
	}
} // </advc.030>

// <advc.300>
// All shelves adjacent to a continent
void CvMap::getShelves(CvArea const& kArea, std::vector<Shelf*>& r) const
{
	int iArea = kArea.getID();
	for(std::map<Shelf::Id,Shelf*>::const_iterator it = m_shelves.begin();
		it != m_shelves.end(); ++it)
	{
		if(it->first.first == iArea)
			r.push_back(it->second);
	}
}


void CvMap::computeShelves()
{
	for(std::map<Shelf::Id,Shelf*>::iterator it = m_shelves.begin();
		it != m_shelves.end(); ++it)
	{
		SAFE_DELETE(it->second);
	}
	m_shelves.clear();
	for(int i = 0; i < numPlots(); i++)
	{
		CvPlot& p = getPlotByIndex(i);
		if(!p.isWater() || p.isLake() || p.isImpassable() || !p.isHabitable())
			continue;
		// Add plot to shelves of all adjacent land areas
		std::set<int> adjLands;
		FOR_EACH_ADJ_PLOT(p)
		{
			if(!pAdj->isWater())
				adjLands.insert(pAdj->getArea().getID());
		}
		for(std::set<int>::iterator it = adjLands.begin(); it != adjLands.end(); ++it)
		{
			Shelf::Id shelfID(*it, p.getArea().getID());
			std::map<Shelf::Id,Shelf*>::iterator shelfPos = m_shelves.find(shelfID);
			Shelf* pShelf;
			if(shelfPos == m_shelves.end())
			{
				pShelf = new Shelf();
				m_shelves.insert(std::make_pair(shelfID, pShelf));
			}
			else pShelf = shelfPos->second;
			pShelf->add(&p);
		}
	}
} // </advc.300>
