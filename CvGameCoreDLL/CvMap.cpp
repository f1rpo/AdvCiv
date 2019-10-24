//
//	FILE:	 CvMap.cpp
//	AUTHOR:  Soren Johnson
//	PURPOSE: Game map class
//-----------------------------------------------------------------------------
//	Copyright (c) 2004 Firaxis Games, Inc. All rights reserved.
//-----------------------------------------------------------------------------
//


#include "CvGameCoreDLL.h"
#include "CvMap.h"
#include "CvGameAI.h"
#include "CvPlayerAI.h"
#include "CvPlotGroup.h"
#include "CvFractal.h"
#include "CvMapGenerator.h"
#include "FAStarNode.h"
#include "CvInitCore.h"
#include "CvInfos.h"
#include "CyArgsList.h"
#include "CvDLLEngineIFaceBase.h"
#include "CvDLLIniParserIFaceBase.h"
#include "CvDLLFAStarIFaceBase.h"
#include "CvDLLFAStarIFaceBase.h"
#include "CvDLLPythonIFaceBase.h"
#include "CvDLLInterfaceIFaceBase.h" // K-Mod
#include <stack> // advc.030


CvMap::CvMap()
{
	CvMapInitData defaultMapData;

	m_paiNumBonus = NULL;
	m_paiNumBonusOnLand = NULL;

	m_pMapPlots = NULL;

	reset(&defaultMapData);
}


CvMap::~CvMap()
{
	uninit();
}

// Initializes the map
// Parameters:
//	pInitInfo  - Optional init structure (used for WB load)
void CvMap::init(CvMapInitData* pInitInfo)
{
	PROFILE("CvMap::init");
	gDLL->logMemState(CvString::format("CvMap::init begin - world size=%s, climate=%s, sealevel=%s, num custom options=%6",
		GC.getWorldInfo(GC.getInitCore().getWorldSize()).getDescription(),
		GC.getClimateInfo(GC.getInitCore().getClimate()).getDescription(),
		GC.getSeaLevelInfo(GC.getInitCore().getSeaLevel()).getDescription(),
		GC.getInitCore().getNumCustomMapOptions()).c_str());

	gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "beforeInit");

	//--------------------------------
	// Init saved data
	reset(pInitInfo);

	//--------------------------------
	// Init containers
	m_areas.init();

	//--------------------------------
	// Init non-saved data
	setup();

	//--------------------------------
	// Init other game data
	gDLL->logMemState("CvMap before init plots");
	m_pMapPlots = new CvPlot[numPlots()];
	for (int iX = 0; iX < getGridWidth(); iX++)
	{
		gDLL->callUpdater();
		for (int iY = 0; iY < getGridHeight(); iY++)
		{
			plotSoren(iX, iY)->init(iX, iY);
		}
	}
	calculateAreas();
	gDLL->logMemState("CvMap after init plots");
}


void CvMap::uninit()
{
	SAFE_DELETE_ARRAY(m_paiNumBonus);
	SAFE_DELETE_ARRAY(m_paiNumBonusOnLand);

	SAFE_DELETE_ARRAY(m_pMapPlots);

	m_areas.uninit();
}

// Initializes data members that are serialized.
void CvMap::reset(CvMapInitData* pInitInfo)
{
	//--------------------------------
	// Uninit class
	uninit();

	//
	// set grid size
	// initially set in terrain cell units
	//
	m_iGridWidth = (GC.getInitCore().getWorldSize() != NO_WORLDSIZE) ? GC.getWorldInfo(GC.getInitCore().getWorldSize()).getGridWidth (): 0;	//todotw:tcells wide
	m_iGridHeight = (GC.getInitCore().getWorldSize() != NO_WORLDSIZE) ? GC.getWorldInfo(GC.getInitCore().getWorldSize()).getGridHeight (): 0;

	// allow grid size override
	if (pInitInfo)
	{
		m_iGridWidth	= pInitInfo->m_iGridW;
		m_iGridHeight	= pInitInfo->m_iGridH;
	}
	else
	{
		// check map script for grid size override
		if (GC.getInitCore().getWorldSize() != NO_WORLDSIZE)
		{
			std::vector<int> out;
			CyArgsList argsList;
			argsList.add(GC.getInitCore().getWorldSize());
			bool ok = gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "getGridSize", argsList.makeFunctionArgs(), &out);

			if (ok && !gDLL->getPythonIFace()->pythonUsingDefaultImpl() && out.size() == 2)
			{
				m_iGridWidth = out[0];
				m_iGridHeight = out[1];
				FAssertMsg(m_iGridWidth > 0 && m_iGridHeight > 0, "the width and height returned by python getGridSize() must be positive");
			}
		}

		// convert to plot dimensions
		if (GC.getNumLandscapeInfos() > 0)
		{
			m_iGridWidth *= GC.getLandscapeInfo(GC.getActiveLandscapeID()).getPlotsPerCellX();
			m_iGridHeight *= GC.getLandscapeInfo(GC.getActiveLandscapeID()).getPlotsPerCellY();
		}
	}

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

		long resultTop = -1, resultBottom = -1;
		bool okX = gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "getTopLatitude", NULL, &resultTop);
		bool overrideX = !gDLL->getPythonIFace()->pythonUsingDefaultImpl();
		bool okY = gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "getBottomLatitude", NULL, &resultBottom);
		bool overrideY = !gDLL->getPythonIFace()->pythonUsingDefaultImpl();

		if (okX && okY && overrideX && overrideY && resultTop != -1 && resultBottom != -1)
		{
			m_iTopLatitude = resultTop;
			m_iBottomLatitude = resultBottom;
		}
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

		long resultX = -1, resultY = -1;
		bool okX = gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "getWrapX", NULL, &resultX);
		bool overrideX = !gDLL->getPythonIFace()->pythonUsingDefaultImpl();
		bool okY = gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "getWrapY", NULL, &resultY);
		bool overrideY = !gDLL->getPythonIFace()->pythonUsingDefaultImpl();

		if (okX && okY && overrideX && overrideY && resultX != -1 && resultY != -1)
		{
			m_bWrapX = (resultX != 0);
			m_bWrapY = (resultY != 0);
		}
	}

	if (GC.getNumBonusInfos())
	{
		FAssertMsg((0 < GC.getNumBonusInfos()), "GC.getNumBonusInfos() is not greater than zero but an array is being allocated in CvMap::reset");
		FAssertMsg(m_paiNumBonus==NULL, "mem leak m_paiNumBonus");
		m_paiNumBonus = new int[GC.getNumBonusInfos()];
		FAssertMsg(m_paiNumBonusOnLand==NULL, "mem leak m_paiNumBonusOnLand");
		m_paiNumBonusOnLand = new int[GC.getNumBonusInfos()];
		for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
		{
			m_paiNumBonus[iI] = 0;
			m_paiNumBonusOnLand[iI] = 0;
		}
	}

	m_areas.removeAll();
}

// Initializes all data that is not serialized but needs to be initialized after loading.
void CvMap::setup()
{
	PROFILE("CvMap::setup");

	KmodPathFinder::InitHeuristicWeights(); // K-Mod
	gDLL->getFAStarIFace()->Initialize(&GC.getPathFinder(), getGridWidth(), getGridHeight(), isWrapX(), isWrapY(), pathDestValid, pathHeuristic, pathCost, pathValid, pathAdd, NULL, NULL);
	gDLL->getFAStarIFace()->Initialize(&GC.getInterfacePathFinder(), getGridWidth(), getGridHeight(), isWrapX(), isWrapY(), pathDestValid, pathHeuristic, pathCost, pathValid, pathAdd, NULL, NULL);
	gDLL->getFAStarIFace()->Initialize(&GC.getStepFinder(), getGridWidth(), getGridHeight(), isWrapX(), isWrapY(), stepDestValid, stepHeuristic, stepCost, stepValid, stepAdd, NULL, NULL);
	gDLL->getFAStarIFace()->Initialize(&GC.getRouteFinder(), getGridWidth(), getGridHeight(), isWrapX(), isWrapY(), NULL, NULL, NULL, routeValid, NULL, NULL, NULL);
	gDLL->getFAStarIFace()->Initialize(&GC.getBorderFinder(), getGridWidth(), getGridHeight(), isWrapX(), isWrapY(), NULL, NULL, NULL, borderValid, NULL, NULL, NULL);
	gDLL->getFAStarIFace()->Initialize(&GC.getAreaFinder(), getGridWidth(), getGridHeight(), isWrapX(), isWrapY(), NULL, NULL, NULL, areaValid, NULL, joinArea, NULL);
	gDLL->getFAStarIFace()->Initialize(&GC.getPlotGroupFinder(), getGridWidth(), getGridHeight(), isWrapX(), isWrapY(), NULL, NULL, NULL, plotGroupValid, NULL, countPlotGroup, NULL);
}


//////////////////////////////////////
// graphical only setup
//////////////////////////////////////
void CvMap::setupGraphical()
{
	if (!GC.IsGraphicsInitialized())
		return;
	CvPlot::setMaxVisibilityRangeCache(); // advc.003h
	if (m_pMapPlots != NULL)
	{
		int iI;
		for (iI = 0; iI < numPlots(); iI++)
		{
			gDLL->callUpdater();	// allow windows msgs to update
			plotByIndex(iI)->setupGraphical();
		}
	}
}


void CvMap::erasePlots()
{
	int iI;

	for (iI = 0; iI < numPlots(); iI++)
	{
		plotByIndex(iI)->erase();
	}
}


void CvMap::setRevealedPlots(TeamTypes eTeam, bool bNewValue, bool bTerrainOnly)
{
	PROFILE_FUNC();

	int iI;

	for (iI = 0; iI < numPlots(); iI++)
	{
		plotByIndex(iI)->setRevealed(eTeam, bNewValue, bTerrainOnly, NO_TEAM, false);
	}

	GC.getGame().updatePlotGroups();
}


void CvMap::setAllPlotTypes(PlotTypes ePlotType)
{
	//float startTime = (float) timeGetTime();

	for(int i=0;i<numPlots();i++)
	{
		plotByIndex(i)->setPlotType(ePlotType, false, false);
	}

	recalculateAreas();

	//rebuild landscape
	gDLL->getEngineIFace()->RebuildAllPlots();

	//mark minimap as dirty
	gDLL->getEngineIFace()->SetDirty(MinimapTexture_DIRTY_BIT, true);
	gDLL->getEngineIFace()->SetDirty(GlobeTexture_DIRTY_BIT, true);

	//float endTime = (float) timeGetTime();
	//OutputDebugString(CvString::format("[Jason] setAllPlotTypes: %f\n", endTime - startTime).c_str());
}


// XXX generalize these funcs? (macro?)
void CvMap::doTurn()
{
	PROFILE("CvMap::doTurn()");

	for(int iI = 0; iI < numPlots(); iI++)
		plotByIndex(iI)->doTurn();
}

// K-Mod
void CvMap::setFlagsDirty()
{
	for(int i = 0; i < numPlots(); i++) // advc.003
		plotByIndex(i)->setFlagDirty(true);
}
// K-Mod end

void CvMap::updateFlagSymbols()
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	int iI;

	for (iI = 0; iI < numPlots(); iI++)
	{
		pLoopPlot = plotByIndex(iI);

		if (pLoopPlot->isFlagDirty())
		{
			pLoopPlot->updateFlagSymbol();
			pLoopPlot->setFlagDirty(false);
		}
	}
}


void CvMap::updateFog()
{
	for(int iI = 0; iI < numPlots(); iI++)
		plotByIndex(iI)->updateFog();
}


void CvMap::updateVisibility()
{
	for (int iI = 0; iI < numPlots(); iI++)
		plotByIndex(iI)->updateVisibility();
}


void CvMap::updateSymbolVisibility()
{
	for(int iI = 0; iI < numPlots(); iI++)
		plotByIndex(iI)->updateSymbolVisibility();
}


void CvMap::updateSymbols()
{
	for(int iI = 0; iI < numPlots(); iI++)
		plotByIndex(iI)->updateSymbols();
}


void CvMap::updateMinimapColor()
{
	for(int iI = 0; iI < numPlots(); iI++)
		plotByIndex(iI)->updateMinimapColor();
}


void CvMap::updateSight(bool bIncrement)
{
	for (int iI = 0; iI < numPlots(); iI++)
	{
		plotByIndex(iI)->updateSight(bIncrement, false);
	}

	GC.getGame().updatePlotGroups();
}


void CvMap::updateIrrigated()
{
	for(int iI = 0; iI < numPlots(); iI++)
		plotByIndex(iI)->updateIrrigated();
}


// K-Mod. This function is called when the unit selection is changed, or when a selected unit is promoted. (Or when UnitInfo_DIRTY_BIT is set.)
// The purpose is to update which unit is displayed in the center of each plot.

// The original implementation simply updated every plot on the map. This is a bad idea because it scales badly for big maps, and the update function on each plot can be expensive.
// The new functionality attempts to only update plots that are in movement range of the selected group; with a very generous approximation for what might be in range.
void CvMap::updateCenterUnit()
{
	/* original bts code
	int iI;
	for (iI = 0; iI < numPlots(); iI++)
		plotByIndex(iI)->updateCenterUnit();*/
	PROFILE_FUNC();
	int iRange = -1;

	CLLNode<IDInfo>* pSelectionNode = gDLL->getInterfaceIFace()->headSelectionListNode();
	while (pSelectionNode)
	{
		const CvUnit* pLoopUnit = ::getUnit(pSelectionNode->m_data);
		pSelectionNode = gDLL->getInterfaceIFace()->nextSelectionListNode(pSelectionNode);

		int iLoopRange;
		if (pLoopUnit->getDomainType() == DOMAIN_AIR)
		{
			iLoopRange = pLoopUnit->airRange();
		}
		else
		{
			int iStepCost = pLoopUnit->getDomainType() == DOMAIN_LAND ? KmodPathFinder::MinimumStepCost(pLoopUnit->baseMoves()) : GC.getMOVE_DENOMINATOR();
			iLoopRange = pLoopUnit->maxMoves() / iStepCost + (pLoopUnit->canParadrop(pLoopUnit->plot()) ? pLoopUnit->getDropRange() : 0);
		}
		iRange = std::max(iRange, iLoopRange);
		// Note: technically we only really need the minimum range; but I'm using the maximum range because I think it will produce more intuitive and useful information for the player.
	}

	if (iRange < 0 || iRange*iRange > numPlots() / 2)
	{
		// update the whole map
		for (int i = 0; i < numPlots(); i++)
		{
			plotByIndex(i)->updateCenterUnit();
		}
	}
	else
	{
		// only update within the range
		CvPlot* pCenterPlot = gDLL->getInterfaceIFace()->getHeadSelectedUnit()->plot();
		for (int x = -iRange; x <= iRange; x++)
		{
			for (int y = -iRange; y <= iRange; y++)
			{
				CvPlot* pLoopPlot = plotXY(pCenterPlot, x, y);
				if (pLoopPlot)
					pLoopPlot->updateCenterUnit();
			}
		}
	}
}
// K-Mod end

void CvMap::updateWorkingCity()
{
	int iI;

	for (iI = 0; iI < numPlots(); iI++)
	{
		plotByIndex(iI)->updateWorkingCity();
	}
}


void CvMap::updateMinOriginalStartDist(CvArea* pArea)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	int iI, iJ;

	for (iI = 0; iI < numPlots(); iI++)
	{
		pLoopPlot = plotByIndex(iI);

		if (pLoopPlot->area() == pArea)
		{
			pLoopPlot->setMinOriginalStartDist(-1);
		}
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlot* pStartingPlot = GET_PLAYER((PlayerTypes)iI).getStartingPlot();

		if (pStartingPlot != NULL)
		{
			if (pStartingPlot->area() == pArea)
			{
				for (iJ = 0; iJ < numPlots(); iJ++)
				{
					pLoopPlot = plotByIndex(iJ);

					//if (pLoopPlot->area() == pArea)
					if (pLoopPlot->area() == pArea && pLoopPlot != pStartingPlot) // K-Mod
					{
						//iDist = calculatePathDistance(pStartingPlot, pLoopPlot);
						int iDist = stepDistance(pStartingPlot->getX(), pStartingPlot->getY(), pLoopPlot->getX(), pLoopPlot->getY());

						if (iDist != -1)
						{
							//int iCrowDistance = plotDistance(pStartingPlot->getX(), pStartingPlot->getY(), pLoopPlot->getX(), pLoopPlot->getY());
							//iDist = std::min(iDist,  iCrowDistance * 2);
							if ((pLoopPlot->getMinOriginalStartDist() == -1) || (iDist < pLoopPlot->getMinOriginalStartDist()))
							{
								pLoopPlot->setMinOriginalStartDist(iDist);
							}
						}
					}
				}
			}
		}
	}
}


void CvMap::updateYield()
{
	int iI;

	for (iI = 0; iI < numPlots(); iI++)
	{
		plotByIndex(iI)->updateYield();
	}
}


void CvMap::verifyUnitValidPlot()
{
	for (int iI = 0; iI < numPlots(); iI++)
	{
		plotByIndex(iI)->verifyUnitValidPlot();
	}
}


void CvMap::combinePlotGroups(PlayerTypes ePlayer, CvPlotGroup* pPlotGroup1, CvPlotGroup* pPlotGroup2)
{
	FAssertMsg(pPlotGroup1 != NULL, "pPlotGroup is not assigned to a valid value");
	FAssertMsg(pPlotGroup2 != NULL, "pPlotGroup is not assigned to a valid value");

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
		CvPlot* pPlot = plotSoren(pPlotNode->m_data.iX, pPlotNode->m_data.iY);
		pNewPlotGroup->addPlot(pPlot);
		pPlotNode = pOldPlotGroup->deletePlotsNode(pPlotNode);
	}
}

CvPlot* CvMap::syncRandPlot(int iFlags, int iArea, int iMinUnitDistance, int iTimeout,
		int* piLegal) // advc.304
{
	/*  <advc.304> The standard 100 trials for Monte Carlo selection often fail to
		find a plot when only handful of tiles are legal on large maps.
		10000 trials would probably do, but that isn't much faster anymore than
		gathering all valid plots upfront - which is what I'm doing. */
	/*while (iCount < iTimeout) {
		pTestPlot = plotSoren(GC.getGame().getSorenRandNum(getGridWidth(), "Rand Plot Width"), GC.getGame().getSorenRandNum(getGridHeight(), "Rand Plot Height"));*/
	std::vector<CvPlot*> apLegalPlots;
	for(int i = 0; i < numPlots(); i++) {
		CvPlot* pTestPlot = plotByIndex(i);
		if(pTestPlot == NULL)
			continue; // </advc.304>
		if (iArea == -1 || pTestPlot->getArea() == iArea)
		{
			bool bValid = true;

			/* advc.300: Code moved into a new function.
			   Barbarians in surrounding plots are now ignored.. */
			if(pTestPlot->isCivUnitNearby(iMinUnitDistance) || pTestPlot->isUnit())
				bValid = false;

			if (bValid)
			{
				if (iFlags & RANDPLOT_LAND)
				{
					if (pTestPlot->isWater())
					{
						bValid = false;
					}
				}
			}

			if (bValid)
			{
				if (iFlags & RANDPLOT_UNOWNED)
				{
					if (pTestPlot->isOwned())
					{
						bValid = false;
					}
				}
			}

			if (bValid)
			{
				if (iFlags & RANDPLOT_ADJACENT_UNOWNED)
				{
					if (pTestPlot->isAdjacentOwned())
					{
						bValid = false;
					}
				}
			}

			if (bValid)
			{
				if (iFlags & RANDPLOT_ADJACENT_LAND)
				{
					if (!(pTestPlot->isAdjacentToLand()))
					{
						bValid = false;
					}
				}
			}

			if (bValid)
			{
				if (iFlags & RANDPLOT_PASSABLE)
				{
					if (pTestPlot->isImpassable())
					{
						bValid = false;
					}
				}
			}

			if (bValid)
			{
				if (iFlags & RANDPLOT_NOT_VISIBLE_TO_CIV)
				{
					if (pTestPlot->isVisibleToCivTeam())
					{
						bValid = false;
					}
				}
			}

			if (bValid)
			{
				if (iFlags & RANDPLOT_NOT_CITY)
				{
					if (pTestPlot->isCity())
					{
						bValid = false;
					}
				}
			}

			// <advc.300>
			if(bValid && (iFlags & RANDPLOT_HABITABLE) &&
					pTestPlot->getYield(YIELD_FOOD) <= 0)
				bValid = false;
			if(bValid && (iFlags & RANDPLOT_WATERSOURCE) &&
					!pTestPlot->isFreshWater() && pTestPlot->getYield(YIELD_FOOD) <= 0)
				bValid = false; // </advc.300>

			if (bValid)
			{
				apLegalPlots.push_back(pTestPlot); // advc.304
				/*pPlot = pTestPlot;
				break;*/
			}
		}

		// <advc.304>
		//iCount++;
	}
	//return pPlot;
	int iLegal = (int)apLegalPlots.size();
	if(piLegal != NULL)
		*piLegal = iLegal;
	if(iLegal == 0)
		return NULL;
	return apLegalPlots[GC.getGame().getSorenRandNum(iLegal, "advc.304")];
	// </advc.304>
}


CvCity* CvMap::findCity(int iX, int iY, PlayerTypes eOwner, TeamTypes eTeam, bool bSameArea, bool bCoastalOnly, TeamTypes eTeamAtWarWith, DirectionTypes eDirection, CvCity* pSkipCity,
		TeamTypes eObserver) const // advc.004r
{
	PROFILE_FUNC();

	int iBestValue = MAX_INT;
	CvCity* pBestCity = NULL;

	// XXX look for barbarian cities???
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if ((eOwner == NO_PLAYER) || (iI == eOwner))
			{
				if ((eTeam == NO_TEAM) || (GET_PLAYER((PlayerTypes)iI).getTeam() == eTeam))
				{
					int iLoop;
					for (CvCity* pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
					{	// <advc.004r>
						if(eObserver != NO_TEAM && !pLoopCity->isRevealed(eObserver, false))
							continue; // </advc.004r>
						if (!bSameArea || (pLoopCity->area() == plot(iX, iY)->area()) || (bCoastalOnly && (pLoopCity->waterArea() == plot(iX, iY)->area())))
						{
							if (!bCoastalOnly || pLoopCity->isCoastal())
							{
								if ((eTeamAtWarWith == NO_TEAM) || atWar(GET_PLAYER((PlayerTypes)iI).getTeam(), eTeamAtWarWith))
								{
									if ((eDirection == NO_DIRECTION) || (estimateDirection(dxWrap(pLoopCity->getX() - iX), dyWrap(pLoopCity->getY() - iY)) == eDirection))
									{
										if ((pSkipCity == NULL) || (pLoopCity != pSkipCity))
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
						}
					}
				}
			}
		}
	}

	return pBestCity;
}


CvSelectionGroup* CvMap::findSelectionGroup(int iX, int iY, PlayerTypes eOwner, bool bReadyToSelect, bool bWorkers) const
{
	int iBestValue = MAX_INT;
	CvSelectionGroup* pBestSelectionGroup = NULL;

	// XXX look for barbarian cities???

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if ((eOwner == NO_PLAYER) || (iI == eOwner))
			{
				int iLoop;
				for(CvSelectionGroup* pLoopSelectionGroup = GET_PLAYER((PlayerTypes)iI).firstSelectionGroup(&iLoop); pLoopSelectionGroup != NULL; pLoopSelectionGroup = GET_PLAYER((PlayerTypes)iI).nextSelectionGroup(&iLoop))
				{
					if (!bReadyToSelect || pLoopSelectionGroup->readyToSelect())
					{
						if (!bWorkers || pLoopSelectionGroup->hasWorker())
						{
							int iValue = plotDistance(iX, iY, pLoopSelectionGroup->getX(), pLoopSelectionGroup->getY());

							if (iValue < iBestValue)
							{
								iBestValue = iValue;
								pBestSelectionGroup = pLoopSelectionGroup;
							}
						}
					}
				}
			}
		}
	}

	return pBestSelectionGroup;
}


CvArea* CvMap::findBiggestArea(bool bWater)
{
	int iBestValue = 0;
	CvArea* pBestArea = NULL;

	int iLoop;
	for(CvArea* pLoopArea = firstArea(&iLoop); pLoopArea != NULL; pLoopArea = nextArea(&iLoop))
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
	int wrapX = 0;
	if (isWrapX())
	{
		wrapX = (int)CvFractal::FRAC_WRAP_X;
	}

	int wrapY = 0;
	if (isWrapY())
	{
		wrapY = (int)CvFractal::FRAC_WRAP_Y;
	}

	return (wrapX | wrapY);
}


//"Check plots for wetlands or seaWater.  Returns true if found"
bool CvMap::findWater(CvPlot* pPlot, int iRange, bool bFreshWater)
{
	PROFILE("CvMap::findWater()");

	CvPlot* pLoopPlot;
	int iDX, iDY;

	for (iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (iDY = -(iRange); iDY <= iRange; iDY++)
		{
			pLoopPlot	= plotXY(pPlot->getX(), pPlot->getY(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (bFreshWater)
				{
					if (pLoopPlot->isFreshWater())
					{
						return true;
					}
				}
				else
				{
					if (pLoopPlot->isWater())
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}


bool CvMap::isPlotExternal(int iX, int iY) const // advc.003f
{
	return isPlot(iX, iY);
}


int CvMap::numPlotsExternal() const // advc.003f
{
	return numPlots();
}


int CvMap::plotNumExternal(int iX, int iY) const // advc.003f
{
	return plotNum(iX, iY);
}


int CvMap::plotX(int iIndex) const
{
	return (iIndex % getGridWidth());
}


int CvMap::plotY(int iIndex) const
{
	return (iIndex / getGridWidth());
}


int CvMap::pointXToPlotX(float fX)
{
	float fWidth, fHeight;
	gDLL->getEngineIFace()->GetLandscapeGameDimensions(fWidth, fHeight);
	return (int)(((fX + (fWidth/2.0f)) / fWidth) * getGridWidth());
}


float CvMap::plotXToPointX(int iX)
{
	float fWidth, fHeight;
	gDLL->getEngineIFace()->GetLandscapeGameDimensions(fWidth, fHeight);
	return ((iX * fWidth) / ((float)getGridWidth())) - (fWidth / 2.0f) + (GC.getPLOT_SIZE() / 2.0f);
}


int CvMap::pointYToPlotY(float fY)
{
	float fWidth, fHeight;
	gDLL->getEngineIFace()->GetLandscapeGameDimensions(fWidth, fHeight);
	return (int)(((fY + (fHeight/2.0f)) / fHeight) * getGridHeight());
}


float CvMap::plotYToPointY(int iY)
{
	float fWidth, fHeight;
	gDLL->getEngineIFace()->GetLandscapeGameDimensions(fWidth, fHeight);
	return ((iY * fHeight) / ((float)getGridHeight())) - (fHeight / 2.0f) + (GC.getPLOT_SIZE() / 2.0f);
}


float CvMap::getWidthCoords() const
{
	return (GC.getPLOT_SIZE() * ((float)getGridWidth()));
}


float CvMap::getHeightCoords() const
{
	return (GC.getPLOT_SIZE() * ((float)getGridHeight()));
}


int CvMap::maxPlotDistance() const
{
	//return std::max(1, plotDistance(0, 0, ((isWrapX()) ? (getGridWidth() / 2) : (getGridWidth() - 1)), ((isWrapY()) ? (getGridHeight() / 2) : (getGridHeight() - 1))));
	// <advc.140> Replacing the above
	CvGame const& g = GC.getGame();
	double civRatio = g.getRecommendedPlayers() / (double)g.getCivPlayersEverAlive();
	// Already factored into getRecommendedPlayers, but I want to give it a little extra weight.
	double seaLvlModifier = (100 - 2.5 * g.getSeaLevelChange()) / 100.0;
	int iWraps = -1; // 0 if cylindrical (1 wrap), -1 flat, +1 toroidical
	if(isWrapX())
		iWraps++;
	if(isWrapY())
		iWraps++;
	CvWorldInfo const& kWorld = GC.getWorldInfo(getWorldSize());
	double r = std::sqrt(kWorld.getGridWidth() * kWorld.getGridHeight() * civRatio *
			seaLvlModifier) * 3.5 - 5 * iWraps;
	return std::max(1, ::round(r)); // </advc.140>
}


int CvMap::maxStepDistance() const
{
	return std::max(1, stepDistance(0, 0, ((isWrapX()) ? (getGridWidth() / 2) : (getGridWidth() - 1)), ((isWrapY()) ? (getGridHeight() / 2) : (getGridHeight() - 1))));
}

// <advc.140>
int CvMap::maxMaintenanceDistance() const {

	return ::round(1 + maxPlotDistance() * (10.0 / GC.getMAX_DISTANCE_CITY_MAINTENANCE()));
} // </advc.140>

int CvMap::getGridWidthExternal() const // advc.003f
{
	return getGridWidth();
}


int CvMap::getGridHeightExternal() const // advc.003f
{
	return getGridHeight();
}


int CvMap::getLandPlots() const
{
	return m_iLandPlots;
}


void CvMap::changeLandPlots(int iChange)
{
	m_iLandPlots = (m_iLandPlots + iChange);
	FAssert(getLandPlots() >= 0);
}


int CvMap::getOwnedPlots() const
{
	return m_iOwnedPlots;
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


int CvMap::getNextRiverID() const
{
	return m_iNextRiverID;
}


void CvMap::incrementNextRiverID()
{
	m_iNextRiverID++;
}


bool CvMap::isWrapXExternal() // advc.003f
{
	return isWrapX();
}


bool CvMap::isWrapYExternal() // advc.003f
{
	return isWrapY();
}

bool CvMap::isWrapExternal() // advc.003f
{
	return isWrap();
}

// advc.003: const replacement for DllExport getWorldSize
WorldSizeTypes CvMap::getWorldSize() const
{
	return GC.getInitCore().getWorldSize();
}


ClimateTypes CvMap::getClimate() const
{
	return GC.getInitCore().getClimate();
}


SeaLevelTypes CvMap::getSeaLevel() const
{
	return GC.getInitCore().getSeaLevel();
}



int CvMap::getNumCustomMapOptions() const
{
	return GC.getInitCore().getNumCustomMapOptions();
}


CustomMapOptionTypes CvMap::getCustomMapOption(int iOption)
{
	return GC.getInitCore().getCustomMapOption(iOption);
}


int CvMap::getNumBonuses(BonusTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiNumBonus[eIndex];
}


void CvMap::changeNumBonuses(BonusTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiNumBonus[eIndex] = (m_paiNumBonus[eIndex] + iChange);
	FAssert(getNumBonuses(eIndex) >= 0);
}


int CvMap::getNumBonusesOnLand(BonusTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiNumBonusOnLand[eIndex];
}


void CvMap::changeNumBonusesOnLand(BonusTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiNumBonusOnLand[eIndex] = (m_paiNumBonusOnLand[eIndex] + iChange);
	FAssert(getNumBonusesOnLand(eIndex) >= 0);
}


CvPlot* CvMap::plotByIndexExternal(int iIndex) const // advc.003f
{
	return plotByIndex(iIndex);
}


CvPlot* CvMap::plotExternal(int iX, int iY) const // advc.003f
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


int CvMap::getNumAreas() const
{
	return m_areas.getCount();
}


int CvMap::getNumLandAreas() const  // advc.003: style changes
{
	int iNumLandAreas = 0;
	int iLoop;
	for(CvArea* pLoopArea = firstArea(&iLoop); pLoopArea != NULL; pLoopArea = nextArea(&iLoop))
	{
		if (!pLoopArea->isWater())
			iNumLandAreas++;
	}
	return iNumLandAreas;
}


CvArea* CvMap::getArea(int iID) const
{
	return m_areas.getAt(iID);
}


CvArea* CvMap::addArea()
{
	return m_areas.add();
}


void CvMap::deleteArea(int iID)
{
	m_areas.removeAt(iID);
}


CvArea* CvMap::firstArea(int *pIterIdx, bool bRev) const
{
	return !bRev ? m_areas.beginIter(pIterIdx) : m_areas.endIter(pIterIdx);
}


CvArea* CvMap::nextArea(int *pIterIdx, bool bRev) const
{
	return !bRev ? m_areas.nextIter(pIterIdx) : m_areas.prevIter(pIterIdx);
}


void CvMap::recalculateAreas()
{
	PROFILE("CvMap::recalculateAreas");

	int iI;

	for (iI = 0; iI < numPlots(); iI++)
	{
		plotByIndex(iI)->setArea(FFreeList::INVALID_INDEX);
	}

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
			pSource->getX(), pSource->getY(),
			pDest->getX(), pDest->getY(), false, 0, true))
	{
		FAStarNode* pNode = gDLL->getFAStarIFace()->GetLastNode(&GC.getStepFinder());

		if (pNode != NULL)
		{
			return pNode->m_iData1;
		}
	}

	return -1; // no passable path exists
}



// BETTER_BTS_AI_MOD, Efficiency (plot danger cache), 08/21/09, jdog5000: START  // advc.003: unnecessary NULL checks removed
void CvMap::invalidateActivePlayerSafeRangeCache()
{
	PROFILE_FUNC();

	for (int iI = 0; iI < numPlots(); iI++)
	{
		plotByIndex(iI)->setActivePlayerSafeRangeCache(-1);
	}
}


void CvMap::invalidateBorderDangerCache(TeamTypes eTeam)
{
	PROFILE_FUNC();

	for(int iI = 0; iI < numPlots(); iI++)
	{
		plotByIndex(iI)->setBorderDangerCache(eTeam, false);
	}
}
// BETTER_BTS_AI_MOD: END

//
// read object from a stream
// used during load
//
void CvMap::read(FDataStreamBase* pStream)
{
	CvMapInitData defaultMapData;

	// Init data before load
	reset(&defaultMapData);

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iGridWidth);
	pStream->Read(&m_iGridHeight);
	pStream->Read(&m_iLandPlots);
	pStream->Read(&m_iOwnedPlots);
	pStream->Read(&m_iTopLatitude);
	pStream->Read(&m_iBottomLatitude);
	pStream->Read(&m_iNextRiverID);

	pStream->Read(&m_bWrapX);
	pStream->Read(&m_bWrapY);

	FAssertMsg((0 < GC.getNumBonusInfos()), "GC.getNumBonusInfos() is not greater than zero but an array is being allocated");
	pStream->Read(GC.getNumBonusInfos(), m_paiNumBonus);
	pStream->Read(GC.getNumBonusInfos(), m_paiNumBonusOnLand);

	if (numPlots() > 0)
	{
		m_pMapPlots = new CvPlot[numPlots()];
		int iI;
		for (iI = 0; iI < numPlots(); iI++)
		{
			m_pMapPlots[iI].read(pStream);
		}
	}

	// call the read of the free list CvArea class allocations
	ReadStreamableFFreeListTrashArray(m_areas, pStream);

	setup();
	computeShelves(); // advc.300
	/*  advc.004z: Not sure if this is the ideal place for this, but it works.
		(The problem was that goody huts weren't always highlighted by the
		Resource layer after loading a game.) */
	gDLL->getInterfaceIFace()->setDirty(GlobeLayer_DIRTY_BIT, true);
}

// save object to a stream
// used during save
//
void CvMap::write(FDataStreamBase* pStream)
{
	uint uiFlag=0;
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iGridWidth);
	pStream->Write(m_iGridHeight);
	pStream->Write(m_iLandPlots);
	pStream->Write(m_iOwnedPlots);
	pStream->Write(m_iTopLatitude);
	pStream->Write(m_iBottomLatitude);
	pStream->Write(m_iNextRiverID);

	pStream->Write(m_bWrapX);
	pStream->Write(m_bWrapY);

	FAssertMsg((0 < GC.getNumBonusInfos()), "GC.getNumBonusInfos() is not greater than zero but an array is being allocated");
	pStream->Write(GC.getNumBonusInfos(), m_paiNumBonus);
	pStream->Write(GC.getNumBonusInfos(), m_paiNumBonusOnLand);

	int iI;
	for (iI = 0; iI < numPlots(); iI++)
	{
		m_pMapPlots[iI].write(pStream);
	}

	// call the read of the free list CvArea class allocations
	WriteStreamableFFreeListTrashArray(m_areas, pStream);
}


//
// used for loading WB maps
//
void CvMap::rebuild(int iGridW, int iGridH, int iTopLatitude, int iBottomLatitude, bool bWrapX, bool bWrapY, WorldSizeTypes eWorldSize, ClimateTypes eClimate, SeaLevelTypes eSeaLevel, int iNumCustomMapOptions, CustomMapOptionTypes * aeCustomMapOptions)
{
	CvMapInitData initData(iGridW, iGridH, iTopLatitude, iBottomLatitude, bWrapX, bWrapY);

	// Set init core data
	GC.getInitCore().setWorldSize(eWorldSize);
	GC.getInitCore().setClimate(eClimate);
	GC.getInitCore().setSeaLevel(eSeaLevel);
	GC.getInitCore().setCustomMapOptions(iNumCustomMapOptions, aeCustomMapOptions);

	// Init map
	init(&initData);
}


//////////////////////////////////////////////////////////////////////////
// Protected Functions...
//////////////////////////////////////////////////////////////////////////

void CvMap::calculateAreas()
{
	PROFILE("CvMap::calculateAreas"); // <advc.030>
	if(GC.getDefineINT("PASSABLE_AREAS") > 0) {
		/*  Will recalculate from CvGame::setinitialItems once normalization is
			through. But need preliminary areas because normalization is done
			based on areas. Also, some scenarios don't call CvGame::
			setInitialItems; these only get the initial calculation based on
			land, sea and peaks (not ice). */
		calculateAreas_030();
		calculateReprAreas();
		return;
	} // </advc.030>

	for (int iI = 0; iI < numPlots(); iI++)
	{
		CvPlot* pLoopPlot = plotByIndex(iI);
		gDLL->callUpdater();
		FAssertMsg(pLoopPlot != NULL, "LoopPlot is not assigned a valid value");

		if (pLoopPlot->getArea() == FFreeList::INVALID_INDEX)
		{
			CvArea* pArea = addArea();
			pArea->init(pArea->getID(), pLoopPlot->isWater());

			int iArea = pArea->getID();

			pLoopPlot->setArea(iArea);

			gDLL->getFAStarIFace()->GeneratePath(&GC.getAreaFinder(), pLoopPlot->getX(), pLoopPlot->getY(), -1, -1, pLoopPlot->isWater(), iArea);
		}
	}
	updateLakes(); // advc.030
}

// <advc.030>
void CvMap::calculateAreas_030() {

	for(int pass = 0; pass <= 1; pass++) {
		for(int i = 0; i < numPlots(); i++) {
			CvPlot& p = *plotByIndex(i);
			if(pass == 0) {
				/*  Second pass for impassables; can't handle
					all-peak/ice areas otherwise. */
				if(p.isImpassable())
					continue;
			}
			if(p.getArea() != FFreeList::INVALID_INDEX)
				continue;
			FAssert(pass == 0 || p.isImpassable());
			CvArea& a = *addArea();
			int iArea = a.getID();
			a.init(iArea, p.isWater());
			p.setArea(iArea);
			calculateAreas_DFS(p);
			gDLL->callUpdater(); // Allow UI to update
		}
	}
}

void CvMap::updateLakes() {

	// CvArea::getNumTiles no longer sufficient for identifying lakes
	int foo;
	for(CvArea* a = firstArea(&foo); a != NULL; a = nextArea(&foo))
		a->updateLake();
	for(int i = 0; i < numPlots(); i++) {
		CvPlot* pPlot = plotByIndex(i);
		if(pPlot->isLake())
			pPlot->updateYield();
	}
	computeShelves(); // advc.300
}

void CvMap::calculateReprAreas() {

	/*  Still need areas as in BtS for submarine movement. Store at each CvArea
		an area id representing all areas that would be encompassed by the same
		BtS area. To decide if a submarine move is possible, only need to
		check if the representative id of the submarine's current area equals
		that of its target area. That's done in CvArea::canBeEntered. */
	int iLoop = 0;
	int iReprChanged = 0; // For debugging; otherwise a bool would suffice.
	do {
		iReprChanged = 0;
		for(int i = 0; i < numPlots(); i++) {
			CvPlot& p = *plotByIndex(i);
			int const x = p.getX();
			int const y = p.getY();
			for(int j = 0; j < NUM_DIRECTION_TYPES; j++) {
				CvPlot* pAdjacent = plotDirection(x, y, (DirectionTypes)j);
				if(pAdjacent == NULL)
					continue;
				CvPlot& q = *pAdjacent;
				// Only orthogonal adjacency for water tiles
				if(p.isWater() && x != q.getX() && y != q.getY())
					continue;
				int const pReprArea = p.area()->getRepresentativeArea();
				int const qReprArea = q.area()->getRepresentativeArea();
				if(pReprArea != qReprArea && p.isWater() == q.isWater()) {
					if(qReprArea < pReprArea)
						p.area()->setRepresentativeArea(qReprArea);
					else q.area()->setRepresentativeArea(pReprArea);
					iReprChanged++;
				}
			}
		}
		if(++iLoop > 10) {
			FAssert(iLoop <= 10);
			/*  Will have to write a faster algorithm then, based on the BtS code at
				the beginning of this function. That would also make it easier to
				set the lakes. */
			break;
		}
	} while(iReprChanged > 0);
	updateLakes();
}


void CvMap::calculateAreas_DFS(CvPlot const& kStart) {

	/*  Explicit stack b/c memory can be an issue if a map has dimensions
		considerably larger than Huge and very large areas.
		I've run out of memory with a recursive implementation (with an attached
		debugger) after about 20000 calls on a 148x148 map generated by LPlate2's
		Eyeball Planet script. With the stack, at least the Release build should
		be pretty safe. */
	std::stack<CvPlot const*> stack;
	stack.push(&kStart);
	while(!stack.empty()) {
		CvPlot const& p = *stack.top();
		stack.pop();
		int const x = p.getX();
		int const y = p.getY();
		for(int i = 0; i < NUM_DIRECTION_TYPES; i++) {
			CvPlot* pAdjacent = plotDirection(x, y, (DirectionTypes)i);
			if(pAdjacent == NULL)
				continue;
			CvPlot& q = *pAdjacent;
			/*  The two neighbors that p and q have in common if p and q are
				diagonally adjacent: */
			CvPlot* s = plot(p.getX(), q.getY());
			CvPlot* t = plot(q.getX(), p.getY());
			FAssertMsg(s != NULL && t != NULL, "Map appears to be non-convex");
			if(q.getArea() == FFreeList::INVALID_INDEX && p.isWater() == q.isWater() &&
					// For water tiles, orthogonal adjacency is unproblematic.
					(!p.isWater() || x == q.getX() || y == q.getY() ||
					// Diagonal adjacency only works if either s or t are water
					s == NULL || s->isWater() || t == NULL || t->isWater()) &&
					/*  Depth-first search that doesn't continue at impassables
						except to other impassables so that mountain ranges and
						ice packs end up in one CvArea. */
					(!p.isImpassable() || q.isImpassable())) {
				q.setArea(p.getArea());
				stack.push(&q);
			}
		}
	}
} // </advc.030>

// <advc.300>
// All shelves adjacent to a continent
void CvMap::getShelves(int iArea, std::vector<Shelf*>& r) const {

	for(std::map<Shelf::Id,Shelf*>::const_iterator it = shelves.begin();
			it != shelves.end(); it++) {
		if(it->first.first == iArea)
			r.push_back(it->second);
	}
}


void CvMap::computeShelves() {

	for(std::map<Shelf::Id,Shelf*>::iterator it = shelves.begin();
			it != shelves.end(); it++)
		SAFE_DELETE(it->second);
	shelves.clear();

	for(int i = 0; i < numPlots(); i++) {
		CvPlot& p = *plotByIndex(i);
		// For each passable marine water plot
		if(!p.isWater() || p.isLake() || p.isImpassable() || !p.isHabitable())
			continue;
		// Add plot to shelves of all adjacent land areas
		std::set<int> adjLands;
		p.getAdjacentLandAreaIds(adjLands);
		for(std::set<int>::iterator it = adjLands.begin(); it != adjLands.end(); it++) {
			Shelf::Id shelfID(*it, p.getArea());
			std::map<Shelf::Id,Shelf*>::iterator shelfPos = shelves.find(shelfID);
			Shelf* pShelf;
			if(shelfPos == shelves.end()) {
				pShelf = new Shelf();
				shelves.insert(std::make_pair(shelfID, pShelf));
			}
			else pShelf = shelfPos->second;
			pShelf->add(&p);
		}
	}
}
// </advc.300>
