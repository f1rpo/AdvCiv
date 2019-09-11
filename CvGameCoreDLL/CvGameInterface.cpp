#include "CvGameCoreDLL.h"
#include "CvGame.h"
#include "CvAI.h"
#include "CvMap.h"
#include "CvInfo_City.h"
#include "CvInfo_Command.h"
#include "CvInfo_Terrain.h"
#include "CvInfo_GameOption.h"
#include "RiseFall.h" // advc.700
#include "CvPopupInfo.h"
#include "FAStarNode.h"
#include "CvGameTextMgr.h"
#include "CvMessageControl.h"
#include "CvBugOptions.h"
#include <fstream> // advc.003d

/*  advc: This file was, reportedly, added by patch 3.17.
	https://forums.civfanatics.com/threads/sdk-using-microsoft-visual-c-2005-express-edition.196283/page-7#post-6942578
	Functions previously implemented in CvGame.cpp were moved here. Almost all of
	those were DLLExports (exceptions: canDoControl, doControl) -- however, many
	other DLLExports remain in CvGame.cpp (for reasons that elude me). */

void CvGame::updateColoredPlots()
{
	PROFILE_FUNC();

	gDLL->getEngineIFace()->clearColoredPlots(PLOT_LANDSCAPE_LAYER_BASE);
	gDLL->getEngineIFace()->clearAreaBorderPlots(AREA_BORDER_LAYER_CITY_RADIUS);
	gDLL->getEngineIFace()->clearAreaBorderPlots(AREA_BORDER_LAYER_RANGED);
	gDLL->getEngineIFace()->clearAreaBorderPlots(AREA_BORDER_LAYER_BLOCKADING);

	if (!gDLL->GetWorldBuilderMode() || gDLL->getInterfaceIFace()->isInAdvancedStart())
		gDLL->getEngineIFace()->clearColoredPlots(PLOT_LANDSCAPE_LAYER_RECOMMENDED_PLOTS);

	if (GC.getPythonCaller()->updateColoredPlots())
		return;
	// <advc.004h>
	// Moved up
	CvUnit* pHeadSelectedUnit = gDLL->getInterfaceIFace()->getHeadSelectedUnit();
	if(pHeadSelectedUnit != NULL && pHeadSelectedUnit->isHuman())
		pHeadSelectedUnit->updateFoundingBorder();
	// </advc.004h>

	// advc: (also removed unnecessary NULL checks after m.plotByIndex calls)
	CvMap const& m = GC.getMap();
	int iPlots = m.numPlots();
	// BETTER_BTS_AI_MOD, Debug, 06/25/09, jdog5000: START
	if(gDLL->getInterfaceIFace()->isShowYields()) { // advc.007
		// City circles for debugging
		if (isDebugMode())
		{
			for (int iPlotLoop = 0; iPlotLoop < iPlots; iPlotLoop++)
			{
				CvPlot* pLoopPlot = m.plotByIndex(iPlotLoop);
				for(int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
				{
					if (GET_PLAYER((PlayerTypes)iI).isAlive())
					{
						if (GET_PLAYER((PlayerTypes)iI).AI_isPlotCitySite(*pLoopPlot))
						{
							gDLL->getEngineIFace()->addColoredPlot(pLoopPlot->getX(), pLoopPlot->getY(), GC.getColorInfo((ColorTypes)GC.getPlayerColorInfo(GET_PLAYER((PlayerTypes)iI).getPlayerColor()).getColorTypePrimary()).getColor(), PLOT_STYLE_CIRCLE, PLOT_LANDSCAPE_LAYER_BASE);
						}
					}
				}
			}
		}

		// Plot improvement replacement circles for debugging
		if (isDebugMode())
		{
			for (int iPlotLoop = 0; iPlotLoop < iPlots; iPlotLoop++)
			{
				CvPlot* pLoopPlot = m.plotByIndex(iPlotLoop);
				CvCityAI const* pWorkingCity = pLoopPlot->AI_getWorkingCity();
				ImprovementTypes eImprovement = pLoopPlot->getImprovementType();
				if (pWorkingCity != NULL && eImprovement != NO_IMPROVEMENT)
				{
					int iPlotIndex = pWorkingCity->getCityPlotIndex(pLoopPlot);
					int iBuildValue = pWorkingCity->AI_getBestBuildValue(iPlotIndex);
					BuildTypes eBestBuild = pWorkingCity->AI_getBestBuild(iPlotIndex);

					if (NO_BUILD != eBestBuild)
					{
						if (GC.getBuildInfo(eBestBuild).getImprovement() != NO_IMPROVEMENT && eImprovement != GC.getBuildInfo(eBestBuild).getImprovement())
						{
							gDLL->getEngineIFace()->addColoredPlot(pLoopPlot->getX(), pLoopPlot->getY(),
									GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_RED")).getColor(),
									PLOT_STYLE_CIRCLE, PLOT_LANDSCAPE_LAYER_BASE);
						}
					}
				}
			}
		}
	} // advc.007
	// BETTER_BTS_AI_MOD: END

	// City circles when in Advanced Start
	if (gDLL->getInterfaceIFace()->isInAdvancedStart())
	{
		for (int iPlotLoop = 0; iPlotLoop < iPlots; iPlotLoop++)
		{
			CvPlot* pLoopPlot = m.plotByIndex(iPlotLoop);
			if (GET_PLAYER(getActivePlayer()).getAdvancedStartCityCost(true, pLoopPlot) > 0)
			{
				bool bStartingPlot = false;
				for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
				{
					CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iPlayer);
					if (kPlayer.isAlive() && getActiveTeam() == kPlayer.getTeam())
					{
						if (pLoopPlot == kPlayer.getStartingPlot())
						{
							bStartingPlot = true;
							break;
						}
					}
				}
				if (bStartingPlot)
				{
					gDLL->getEngineIFace()->addColoredPlot(pLoopPlot->getX(), pLoopPlot->getY(), GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_WARNING_TEXT")).getColor(), PLOT_STYLE_CIRCLE, PLOT_LANDSCAPE_LAYER_RECOMMENDED_PLOTS);
				}
				else if (GET_PLAYER(getActivePlayer()).AI_isPlotCitySite(*pLoopPlot))
				{
					gDLL->getEngineIFace()->addColoredPlot(pLoopPlot->getX(), pLoopPlot->getY(), GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT")).getColor(), PLOT_STYLE_CIRCLE, PLOT_LANDSCAPE_LAYER_RECOMMENDED_PLOTS);
				}

				if (pLoopPlot->isRevealed(getActiveTeam(), false))
				{
					NiColorA color(GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_WHITE")).getColor());
					color.a = 0.4f;
					gDLL->getEngineIFace()->fillAreaBorderPlot(pLoopPlot->getX(), pLoopPlot->getY(), color, AREA_BORDER_LAYER_CITY_RADIUS);
				}
			}
		}
	}

	CvCity* pHeadSelectedCity = gDLL->getInterfaceIFace()->getHeadSelectedCity();
	if (pHeadSelectedCity != NULL)
	{
		if (gDLL->getInterfaceIFace()->isCityScreenUp())
		{
			for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
			{
				if (pHeadSelectedCity->isWorkingPlot(iI))
				{
					CvPlot* pLoopPlot = plotCity(pHeadSelectedCity->getX(), pHeadSelectedCity->getY(), iI);

					if (pLoopPlot != NULL)
					{
						NiColorA color(GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_WHITE")).getColor());
						color.a = 0.7f;
						gDLL->getEngineIFace()->addColoredPlot(pLoopPlot->getX(), pLoopPlot->getY(), color, PLOT_STYLE_CIRCLE, PLOT_LANDSCAPE_LAYER_BASE);
					}
				}
			}
		}
		else
		{
			CLLNode<IDInfo>* pSelectedCityNode = gDLL->getInterfaceIFace()->headSelectedCitiesNode();
			while (pSelectedCityNode != NULL)
			{
				CvCity* pSelectedCity = ::getCity(pSelectedCityNode->m_data);
				pSelectedCityNode = gDLL->getInterfaceIFace()->nextSelectedCitiesNode(pSelectedCityNode);

				if (pSelectedCity != NULL)
				{
					CvPlot* pRallyPlot = pSelectedCity->getRallyPlot();

					if (pRallyPlot != NULL)
					{
						gDLL->getEngineIFace()->addColoredPlot(pRallyPlot->getX(), pRallyPlot->getY(), GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_YELLOW")).getColor(), PLOT_STYLE_CIRCLE, PLOT_LANDSCAPE_LAYER_BASE);
					}
				}
			}
		} // <advc>
		return;
	}
	if (pHeadSelectedUnit == NULL)
		return; // </advc>

	if (gDLL->getGraphicOption(GRAPHICOPTION_CITY_RADIUS))
	{
		//if (gDLL->getInterfaceIFace()->canSelectionListFound())
		if(pHeadSelectedUnit->canFound()) // advc.004h
		{
			for (int iI = 0; iI < iPlots; iI++)
			{
				CvPlot* pLoopPlot = m.plotByIndex(iI);
				if (pLoopPlot->getOwner() == pHeadSelectedUnit->getOwner())
				{
					if (pLoopPlot->getWorkingCity() != NULL)
					{
						NiColorA color(GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT")/*(GC.getPlayerColorInfo(GET_PLAYER(pHeadSelectedUnit->getOwner()).getPlayerColor()).getColorTypePrimary())*/).getColor());
						color.a = 1.0f;
						gDLL->getEngineIFace()->fillAreaBorderPlot(pLoopPlot->getX(), pLoopPlot->getY(), color, AREA_BORDER_LAYER_CITY_RADIUS);
					}
				}
			}
		}
	}

	if (pHeadSelectedUnit->getDomainType() == DOMAIN_AIR)
	{
		int iMaxAirRange = 0;

		CLLNode<IDInfo>* pSelectedUnitNode = gDLL->getInterfaceIFace()->headSelectionListNode();
		while (pSelectedUnitNode != NULL)
		{
			CvUnit* pSelectedUnit = ::getUnit(pSelectedUnitNode->m_data);
			pSelectedUnitNode = gDLL->getInterfaceIFace()->nextSelectionListNode(pSelectedUnitNode);

			if (pSelectedUnit != NULL)
			{
				iMaxAirRange = std::max(iMaxAirRange, pSelectedUnit->airRange());
			}
		}

		if (iMaxAirRange > 0)
		{
			for (int iDX = -(iMaxAirRange); iDX <= iMaxAirRange; iDX++)
			{
				for (int iDY = -(iMaxAirRange); iDY <= iMaxAirRange; iDY++)
				{
					CvPlot* pLoopPlot = m.plotXY(pHeadSelectedUnit->getX(), pHeadSelectedUnit->getY(), iDX, iDY);

					if (pLoopPlot != NULL)
					{
						if (m.plotDistance(pHeadSelectedUnit->getX(), pHeadSelectedUnit->getY(), pLoopPlot->getX(), pLoopPlot->getY()) <= iMaxAirRange)
						{
							NiColorA color(GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_YELLOW")).getColor());
							color.a = 0.5f;
							gDLL->getEngineIFace()->fillAreaBorderPlot(pLoopPlot->getX(), pLoopPlot->getY(), color, AREA_BORDER_LAYER_RANGED);
						}
					}
				}
			}
		}
	}
	else if(pHeadSelectedUnit->airRange() > 0) //other ranged units
	{
		int iRange = pHeadSelectedUnit->airRange();
		for (int iDX = -(iRange); iDX <= iRange; iDX++)
		{
			for (int iDY = -(iRange); iDY <= iRange; iDY++)
			{
				CvPlot* pTargetPlot = m.plotXY(pHeadSelectedUnit->getX(), pHeadSelectedUnit->getY(), iDX, iDY);
				if (pTargetPlot != NULL && pTargetPlot->isVisible(pHeadSelectedUnit->getTeam(), false))
				{
					if (m.plotDistance(pHeadSelectedUnit->getX(), pHeadSelectedUnit->getY(), pTargetPlot->getX(), pTargetPlot->getY()) <= iRange)
					{
						if (pHeadSelectedUnit->plot()->canSeePlot(pTargetPlot, pHeadSelectedUnit->getTeam(), iRange, pHeadSelectedUnit->getFacingDirection(true)))
						{
							NiColorA color(GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_YELLOW")).getColor());
							color.a = 0.5f;
							gDLL->getEngineIFace()->fillAreaBorderPlot(pTargetPlot->getX(), pTargetPlot->getY(), color, AREA_BORDER_LAYER_RANGED);
						}
					}
				}
			}
		}
	}

	FAssert(getActivePlayer() != NO_PLAYER);

	if (!GET_PLAYER(getActivePlayer()).isOption(PLAYEROPTION_NO_UNIT_RECOMMENDATIONS))
	{
		CvUnitAI const& kRecommendUnit = pHeadSelectedUnit->AI(); // advc.003u
		if (kRecommendUnit.AI_getUnitAIType() == UNITAI_WORKER ||
			kRecommendUnit.AI_getUnitAIType() == UNITAI_WORKER_SEA)
		{
			if (kRecommendUnit.plot()->getOwner() == kRecommendUnit.getOwner())
			{
				CvCityAI* pCity = kRecommendUnit.plot()->AI_getWorkingCity();
				if (pCity != NULL)
				{
					CvPlot* pBestPlot = NULL;
					if (kRecommendUnit.AI_bestCityBuild(*pCity, &pBestPlot) &&
						pCity->AI_getBestBuildValue(plotCityXY(pCity, pBestPlot)) > 1)
					{
						FAssert(pBestPlot != NULL);
						gDLL->getEngineIFace()->addColoredPlot(pBestPlot->getX(), pBestPlot->getY(), GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT")).getColor(), PLOT_STYLE_CIRCLE, PLOT_LANDSCAPE_LAYER_RECOMMENDED_PLOTS);
						CvPlot* pNextBestPlot = NULL;
						if (kRecommendUnit.AI_bestCityBuild(*pCity, &pNextBestPlot, NULL, pBestPlot) &&
							pCity->AI_getBestBuildValue(plotCityXY(pCity, pNextBestPlot)) > 1)
						{
							FAssert(pNextBestPlot != NULL);
							gDLL->getEngineIFace()->addColoredPlot(pNextBestPlot->getX(), pNextBestPlot->getY(), GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT")).getColor(), PLOT_STYLE_CIRCLE, PLOT_LANDSCAPE_LAYER_RECOMMENDED_PLOTS);
						}
					}
				}
			}
		}

		// K-Mod. I've rearranged the following code a bit, so that it is more efficient, and so that it shows city sites within 7 turns, rather than just the ones in 4 plot range.
		// the original code has been deleted, because it was quite bulky.

		// city sites
		const CvPlayerAI& kActivePlayer = GET_PLAYER(getActivePlayer());
		KmodPathFinder site_path;
		site_path.SetSettings(pHeadSelectedUnit->getGroup(), 0, 7, GC.getMOVE_DENOMINATOR());
		if (pHeadSelectedUnit->canFound()) // advc.004h: was isFound
		{

			for (int i = 0; i < kActivePlayer.AI_getNumCitySites(); i++)
			{
				CvPlot* pSite = kActivePlayer.AI_getCitySite(i);
				if (pSite && site_path.GeneratePath(pSite))
				{
					gDLL->getEngineIFace()->addColoredPlot(pSite->getX(), pSite->getY(), GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT")).getColor(), PLOT_STYLE_CIRCLE, PLOT_LANDSCAPE_LAYER_RECOMMENDED_PLOTS);
				}
			}
		}

		// goody huts
		//if (kHeadUnit.isNoBadGoodies())
		if (pHeadSelectedUnit->canFight()) // advc.004z: Replacing the above
		{
			int iRange = 4;
			// <advc.004z>
			if (pHeadSelectedUnit->isNoBadGoodies())
				iRange++;
			else iRange--; // </advc.004z>
			site_path.SetSettings(pHeadSelectedUnit->getGroup(), 0, iRange, GC.getMOVE_DENOMINATOR()); // just a smaller range.

			for (int iDX = -(iRange); iDX <= iRange; iDX++)
			{
				for (int iDY = -(iRange); iDY <= iRange; iDY++)
				{
					CvPlot* pLoopPlot = m.plotXY(pHeadSelectedUnit->getX(), pHeadSelectedUnit->getY(), iDX, iDY);
					if (pLoopPlot != NULL && pLoopPlot->isVisible(pHeadSelectedUnit->getTeam(), false) &&
						pLoopPlot->isRevealedGoody(pHeadSelectedUnit->getTeam()))
					{
						if (site_path.GeneratePath(pLoopPlot))
						{
							gDLL->getEngineIFace()->addColoredPlot(pLoopPlot->getX(), pLoopPlot->getY(), GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT")).getColor(), PLOT_STYLE_CIRCLE, PLOT_LANDSCAPE_LAYER_RECOMMENDED_PLOTS);
						}
					}
				}
			}
		}
		// K-Mod end
	}

	if (pHeadSelectedUnit->isBlockading())
	{
		for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
		{
			CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
			if(kPlayer.getTeam() != getActiveTeam())
				continue; // advc
			FOR_EACH_UNIT(pLoopUnit, kPlayer)
			{
				if (pLoopUnit->isBlockading())
				{	/*  <advc.033> Replacing code that was (mostly) equivalent
					to CvUnit::updatePlunder */
					std::vector<CvPlot*> apRange;
					pLoopUnit->blockadeRange(apRange);
					for(size_t j = 0; j < apRange.size(); j++) { // </advc.033>
						NiColorA color(GC.getColorInfo((ColorTypes)GC.getPlayerColorInfo(
							GET_PLAYER(getActivePlayer()).getPlayerColor()).
							getColorTypePrimary()).getColor());
						color.a = 0.5f;
						gDLL->getEngineIFace()->fillAreaBorderPlot(
							apRange[j]->getX(),
							apRange[j]->getY(), color,
							AREA_BORDER_LAYER_BLOCKADING);
					}

				}
			}
		}
	}
}

void CvGame::updateBlockadedPlots()
{
	PROFILE_FUNC();

	gDLL->getEngineIFace()->clearAreaBorderPlots(AREA_BORDER_LAYER_BLOCKADED);

	for (int i = 0; i < GC.getMap().numPlots(); ++i)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(i);
		if (pLoopPlot->getBlockadedCount(getActiveTeam()) > 0 && pLoopPlot->isRevealed(getActiveTeam(), false))
		{
			NiColorA color(GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_BLACK")).getColor());
			color.a = 0.35f;
			gDLL->getEngineIFace()->fillAreaBorderPlot(pLoopPlot->getX(), pLoopPlot->getY(), color, AREA_BORDER_LAYER_BLOCKADED);
		}
	}
	/*  <advc.700> An odd place for initialization. Need graphics to be fully
		initialized so that I can send an error msg to the player if necessary.
		Not the case in e.g. CvGame::init or setInitialItems. */
	if(isOption(GAMEOPTION_RISE_FALL) && getElapsedGameTurns() <= 0)
		m_pRiseFall->init(); // </advc.700>
}


void CvGame::updateSelectionList()
{
	CvUnit* pHeadSelectedUnit;

	//if (GET_PLAYER(getActivePlayer()).isOption(PLAYEROPTION_NO_UNIT_CYCLING))
	if (GC.suppressCycling() || GET_PLAYER(getActivePlayer()).isOption(PLAYEROPTION_NO_UNIT_CYCLING)) // K-Mod
	{
		return;
	}

	pHeadSelectedUnit = gDLL->getInterfaceIFace()->getHeadSelectedUnit();

	if ((pHeadSelectedUnit == NULL) || !(pHeadSelectedUnit->getGroup()->readyToSelect(true)))
	{
		if ((gDLL->getInterfaceIFace()->getOriginalPlot() == NULL) || !(cyclePlotUnits(gDLL->getInterfaceIFace()->getOriginalPlot(), true, true, gDLL->getInterfaceIFace()->getOriginalPlotCount())))
		{
			if ((gDLL->getInterfaceIFace()->getSelectionPlot() == NULL) || !(cyclePlotUnits(gDLL->getInterfaceIFace()->getSelectionPlot(), true, true)))
			{
				cycleSelectionGroups(true);
			}
		}

		pHeadSelectedUnit = gDLL->getInterfaceIFace()->getHeadSelectedUnit();

		if (pHeadSelectedUnit != NULL)
		{
			if (!(pHeadSelectedUnit->getGroup()->readyToSelect()))
			{
				gDLL->getInterfaceIFace()->clearSelectionList();
			}
		}
	}
}


void CvGame::updateTestEndTurn()  // advc: nested else branches replaced w/ return statements
{
	if (!GET_PLAYER(getActivePlayer()).isTurnActive())
		return;

	// <advc.003g>
	if(!b_mFPTestDone)
		CvMessageControl::getInstance().sendFPTest(FPChecksum()); // </advc.003g>

	bool bAny = (gDLL->getInterfaceIFace()->getHeadSelectedUnit() != NULL &&
			!GET_PLAYER(getActivePlayer()).isOption(PLAYEROPTION_NO_UNIT_CYCLING));

	if (gDLL->getInterfaceIFace()->isEndTurnMessage())
	{
		if (GET_PLAYER(getActivePlayer()).hasReadyUnit(bAny))
			gDLL->getInterfaceIFace()->setEndTurnMessage(false);
		return;
	}

	if (GET_PLAYER(getActivePlayer()).hasBusyUnit() ||
			GET_PLAYER(getActivePlayer()).hasReadyUnit(bAny))
		return;

	bool bShift = GC.shiftKey();

	if (!(gDLL->getInterfaceIFace()->isForcePopup()))
	{
		if (!bShift && !GC.suppressCycling()) // K-Mod
			gDLL->getInterfaceIFace()->setForcePopup(true);
		return;
	}

	if (GET_PLAYER(getActivePlayer()).hasAutoUnit())
	{
		//if (!(GC.shiftKey()))
		// K-Mod. Don't start automoves if we currently have a group selected which would move.
		CvUnit* pSelectedUnit = gDLL->getInterfaceIFace()->getHeadSelectedUnit();
		if (!bShift && !GC.suppressCycling() && (pSelectedUnit == NULL || !pSelectedUnit->getGroup()->readyToAuto()))
		// K-Mod end
			CvMessageControl::getInstance().sendAutoMoves();
		return;
	}

	if (GET_PLAYER(getActivePlayer()).isOption(PLAYEROPTION_WAIT_END_TURN) || !gDLL->getInterfaceIFace()->isHasMovedUnit() || isHotSeat() || isPbem())
	{
		gDLL->getInterfaceIFace()->setEndTurnMessage(true);
		return;
	}

	if (gDLL->getInterfaceIFace()->getEndTurnCounter() > 0)
	{
		gDLL->getInterfaceIFace()->changeEndTurnCounter(-1);
		return;
	}

	CvMessageControl::getInstance().sendTurnComplete();
	gDLL->getInterfaceIFace()->setEndTurnCounter(3); // XXX
}

// advc: Merge of two BtS functions that had largely the same body
CvUnit* CvGame::getPlotUnits(CvPlot const* pPlot, std::vector<CvUnit*>* pPlotUnits, int iIndex) const
{
	PROFILE_FUNC();

	FAssert((iIndex == -1) != (pPlotUnits == NULL));

	if (pPlotUnits != NULL)
		pPlotUnits->clear();

	if(pPlot == NULL)
		return NULL;

	int iCount = 0;
	PlayerTypes eActivePlayer = getActivePlayer();
	TeamTypes eActiveTeam = getActiveTeam();
	for (int iPass = 0; iPass < 2; iPass++)
	{
		for (CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode(); pUnitNode != NULL;
			pUnitNode = pPlot->nextUnitNode(pUnitNode))
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			if ((pLoopUnit->getOwner() == eActivePlayer) != (iPass == 0))
				continue;
			if (pLoopUnit->isInvisible(eActiveTeam, true) || pLoopUnit->isCargo())
				continue;

			if (iCount == iIndex)
				return pLoopUnit;

			iCount++;
			if (pPlotUnits != NULL)
				pPlotUnits->push_back(pLoopUnit);
			//if (pLoopUnit1->getTeam() == activeTeam || isDebugMode()) {
			if (!pLoopUnit->hasCargo())
				continue;

			for (CLLNode<IDInfo>* pCargoUnitNode = pPlot->headUnitNode(); pCargoUnitNode != NULL;
				pCargoUnitNode = pPlot->nextUnitNode(pCargoUnitNode))
			{
				CvUnit* pCargoUnit = ::getUnit(pCargoUnitNode->m_data);
				if (pCargoUnit->isInvisible(eActiveTeam, true))
					continue;

				if (pCargoUnit->getTransportUnit() == pLoopUnit)
				{
					if (iCount == iIndex)
						return pCargoUnit;

					iCount++;
					if (pPlotUnits != NULL)
						pPlotUnits->push_back(pCargoUnit);
				}
			}
		}
	}
	return NULL;
}

void CvGame::cycleCities(bool bForward, bool bAdd) const
{
	CvCity* pHeadSelectedCity = gDLL->getInterfaceIFace()->getHeadSelectedCity();
	CvCity* pSelectCity = NULL;
	if (pHeadSelectedCity != NULL && (pHeadSelectedCity->getTeam() == getActiveTeam() || isDebugMode()))
	{
		int iLoop = pHeadSelectedCity->getIndex();
		iLoop += (bForward ? 1 : -1);

		CvCity* pLoopCity = GET_PLAYER(pHeadSelectedCity->getOwner()).nextCity(&iLoop, !bForward);

		if (pLoopCity == NULL)
			pLoopCity = GET_PLAYER(pHeadSelectedCity->getOwner()).firstCity(&iLoop, !bForward);

		if (pLoopCity != NULL && pLoopCity != pHeadSelectedCity)
			pSelectCity = pLoopCity;
	}
	else
	{
		int iLoop;
		pSelectCity = GET_PLAYER(getActivePlayer()).firstCity(&iLoop, !bForward);
	}

	if (pSelectCity != NULL)
	{
		if (bAdd)
		{
			gDLL->getInterfaceIFace()->clearSelectedCities();
			gDLL->getInterfaceIFace()->addSelectedCity(pSelectCity);
		}
		else gDLL->getInterfaceIFace()->selectCity(pSelectCity);
	}
}

// advc.003i: const removed so that updateTestEndTurn can be called
void CvGame::cycleSelectionGroups(bool bClear, bool bForward, bool bWorkers)
{
	CvSelectionGroup* pNextSelectionGroup;

	CvUnit* pCycleUnit = gDLL->getInterfaceIFace()->getHeadSelectedUnit();

	if (pCycleUnit != NULL)
	{
		if (pCycleUnit->getOwner() != getActivePlayer())
		{
			pCycleUnit = NULL;
		}
		bool bWrap=false;
		pNextSelectionGroup = GET_PLAYER(getActivePlayer()).cycleSelectionGroups(
				pCycleUnit, bForward, bWorkers, &bWrap);
		if (bWrap)
		{
			//if (GET_PLAYER(getActivePlayer()).hasAutoUnit())
			// K-Mod. I've weakend this condition so that the group cycle order can be refreshed by automoves.
			// (Maybe I should create & use "sendCycleRefresh" instead.)
			if (pNextSelectionGroup || GET_PLAYER(getActivePlayer()).hasAutoUnit())
			// K-Mod end
			{
				CvMessageControl::getInstance().sendAutoMoves();
			}
		}
	}
	else
	{
		CvPlot* pPlot = gDLL->getInterfaceIFace()->getLookAtPlot();
		pNextSelectionGroup = GC.getMap().findSelectionGroup(
				pPlot != NULL ? pPlot->getX() : 0,
				pPlot != NULL ? pPlot->getY() : 0,
				getActivePlayer(), true, bWorkers);
	}

	if (pNextSelectionGroup != NULL)
	{
		FAssert(pNextSelectionGroup->getOwner() == getActivePlayer());
		FAssert(pNextSelectionGroup->getHeadUnit() != NULL); // K-Mod
		gDLL->getInterfaceIFace()->selectUnit(pNextSelectionGroup->getHeadUnit(), bClear);
	}
	// K-Mod
	else if (pCycleUnit)
	{
		gDLL->getInterfaceIFace()->clearSelectionList();
		updateTestEndTurn();
		// <advc.002e> Hide glow when all units moved
		if(!getBugOptionBOOL("PLE__ShowPromotionGlow", false)) {
			CvPlayer const& kOwner = GET_PLAYER(pCycleUnit->getOwner());
			FOR_EACH_UNIT_VAR(u, kOwner)
				gDLL->getEntityIFace()->showPromotionGlow(u->getUnitEntity(), false);
		} // </advc.002e>
	} // K-Mod end

	if ((pCycleUnit != gDLL->getInterfaceIFace()->getHeadSelectedUnit()) || ((pCycleUnit != NULL) && pCycleUnit->getGroup()->readyToSelect()))
	{
		gDLL->getInterfaceIFace()->lookAtSelectionPlot();
	}
}

// K-Mod
void CvGame::cycleSelectionGroups_delayed(int iDelay, bool bIncremental, bool bDelayOnly)
{
	PROFILE_FUNC(); // I'm just hoping that the python call doesn't hurt the respose times

	if (GC.suppressCycling()) // cf. GvGame::updateSelectionList
		return;

	// Only rapid-cycle when not doing auto-play.
	// Also note, cycleSelectionGroups currently causes a crash if the game is not initialised.
	// (and this function is indirectly called during the set of up a new game - so we currently need that init check.)
	PlayerTypes eActive = getActivePlayer();
	if (isFinalInitialized() &&
		eActive != NO_PLAYER && GET_PLAYER(eActive).isHuman() &&
		getBugOptionBOOL("MainInterface__RapidUnitCycling", false))
	{
		if (!bDelayOnly)
		{
			if (GET_PLAYER(eActive).isOption(PLAYEROPTION_NO_UNIT_CYCLING)) // (for the non-rapid case, this option is handled elsewhere.)
				return;

			if (gDLL->getEngineIFace()->isCameraLocked())
				gDLL->getInterfaceIFace()->setCycleSelectionCounter(1); // immediate cycling might violate the camera lock. :(
			else
				cycleSelectionGroups(true);
		}
	}
	else
	{
		if (bIncremental)
			gDLL->getInterfaceIFace()->changeCycleSelectionCounter(iDelay);
		else gDLL->getInterfaceIFace()->setCycleSelectionCounter(iDelay);
	}
} // K-Mod end

// Returns true if unit was cycled...
bool CvGame::cyclePlotUnits(CvPlot* pPlot, bool bForward, bool bAuto, int iCount) const
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit = NULL;

	FAssertMsg(iCount >= -1, "iCount expected to be >= -1");

	if (iCount == -1)
	{
		pUnitNode = pPlot->headUnitNode();

		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);

			if (pLoopUnit->IsSelected())
			{
				break;
			}

			pUnitNode = pPlot->nextUnitNode(pUnitNode);
		}
	}
	else
	{
		pUnitNode = pPlot->headUnitNode();

		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);

			if ((iCount - 1) == 0)
			{
				break;
			}

			if (iCount > 0)
			{
				iCount--;
			}

			pUnitNode = pPlot->nextUnitNode(pUnitNode);
		}

		if (pUnitNode == NULL)
		{
			pUnitNode = pPlot->tailUnitNode();

			if (pUnitNode != NULL)
			{
				pLoopUnit = ::getUnit(pUnitNode->m_data);
			}
		}
	}

	if (pUnitNode != NULL)
	{
		CvUnit* pSelectedUnit = pLoopUnit;

		while (true)
		{
			if (bForward)
			{
				pUnitNode = pPlot->nextUnitNode(pUnitNode);
				if (pUnitNode == NULL)
				{
					pUnitNode = pPlot->headUnitNode();
				}
			}
			else
			{
				pUnitNode = pPlot->prevUnitNode(pUnitNode);
				if (pUnitNode == NULL)
				{
					pUnitNode = pPlot->tailUnitNode();
				}
			}

			pLoopUnit = ::getUnit(pUnitNode->m_data);

			if (iCount == -1)
			{
				if (pLoopUnit == pSelectedUnit)
				{
					break;
				}
			}

			if (pLoopUnit->getOwner() == getActivePlayer())
			{
				if (bAuto)
				{
					if (pLoopUnit->getGroup()->readyToSelect())
					{
						gDLL->getInterfaceIFace()->selectUnit(pLoopUnit, true);
						return true;
					}
				}
				else
				{
					gDLL->getInterfaceIFace()->insertIntoSelectionList(pLoopUnit, true, false);
					return true;
				}
			}

			if (pLoopUnit == pSelectedUnit)
			{
				break;
			}
		}
	}

	return false;
}

bool CvGame::selectCity(CvCity* pSelectCity, bool bCtrl, bool bAlt, bool bShift) const
{
	if (pSelectCity == NULL || !pSelectCity->canBeSelected())
	{
		return false;
	}

	if (!bShift)
	{
		gDLL->getInterfaceIFace()->clearSelectedCities();
	}

	if (bAlt)
	{
		FOR_EACH_CITY_VAR(pLoopCity, GET_PLAYER(pSelectCity->getOwner()))
			gDLL->getInterfaceIFace()->addSelectedCity(pLoopCity);
	}
	else if (bCtrl)
	{
		FOR_EACH_CITY_VAR(pLoopCity, GET_PLAYER(pSelectCity->getOwner()))
		{
			if (pLoopCity->getArea() == pSelectCity->getArea())
				gDLL->getInterfaceIFace()->addSelectedCity(pLoopCity);
		}
	}
	else gDLL->getInterfaceIFace()->addSelectedCity(pSelectCity, bShift);

	return true;
}

void CvGame::selectionListMove(CvPlot* pPlot, bool bAlt, bool bShift, bool bCtrl) const
{
	if (pPlot == NULL)
		return;

	if (GC.getPythonCaller()->cannotSelectionListMoveOverride(*pPlot, bAlt, bShift, bCtrl))
		return;

	CvUnit* pHeadSelectedUnit = gDLL->getInterfaceIFace()->getHeadSelectedUnit();
	if (pHeadSelectedUnit == NULL || pHeadSelectedUnit->getOwner() != getActivePlayer())
		return;

	if (bAlt)
	{
		//gDLL->getInterfaceIFace()->selectGroup(pHeadSelectedUnit, false, false, true);
		gDLL->getInterfaceIFace()->selectGroup(pHeadSelectedUnit, false, true, true); // K-Mod
	}
	else if (bCtrl)
		gDLL->getInterfaceIFace()->selectGroup(pHeadSelectedUnit, false, true, false);

	/* bts code
	pSelectedUnitNode = gDLL->getInterfaceIFace()->headSelectionListNode();
	while (pSelectedUnitNode != NULL)
	{
		// advc: Rest deleted
		pSelectedUnitNode = gDLL->getInterfaceIFace()->nextSelectionListNode(pSelectedUnitNode);
	} */ // K-Mod has moved this to selectionListGameNetMessage.

	selectionListGameNetMessage(GAMEMESSAGE_PUSH_MISSION, MISSION_MOVE_TO,
			pPlot->getX(), pPlot->getY(), 0, false, bShift);
}


void CvGame::selectionListGameNetMessage(int eMessage, int iData2, int iData3, int iData4,
		int iFlags, bool bAlt, bool bShift) const
{
	int aiPyData[] = { iData2, iData3, iData4 };
	if (GC.getPythonCaller()->cannotSelectionListNetOverride((GameMessageTypes)
			eMessage, aiPyData, iFlags, bAlt, bShift))
		return;

	CvUnit* pHeadSelectedUnit = gDLL->getInterfaceIFace()->getHeadSelectedUnit();
	if (pHeadSelectedUnit == NULL || pHeadSelectedUnit->getOwner() != getActivePlayer())
		return; // advc

	CLLNode<IDInfo>* pSelectedUnitNode;
	CvUnit* pSelectedUnit;
	if (eMessage == GAMEMESSAGE_JOIN_GROUP)
	{
		pSelectedUnitNode = gDLL->getInterfaceIFace()->headSelectionListNode();
		while (pSelectedUnitNode != NULL)
		{
			pSelectedUnit = ::getUnit(pSelectedUnitNode->m_data);
			pSelectedUnitNode = gDLL->getInterfaceIFace()->nextSelectionListNode(pSelectedUnitNode);

			if (bShift)
			{
				CvMessageControl::getInstance().sendJoinGroup(pSelectedUnit->getID(),
						FFreeList::INVALID_INDEX);
			}
			else
			{
				if (pSelectedUnit == pHeadSelectedUnit)
				{
					CvMessageControl::getInstance().sendJoinGroup(pSelectedUnit->getID(),
							FFreeList::INVALID_INDEX);
				}
				else // K-Mod
				{
					CvMessageControl::getInstance().sendJoinGroup(pSelectedUnit->getID(),
							pHeadSelectedUnit->getID());
				}
			}
		}

		if (bShift)
			gDLL->getInterfaceIFace()->selectUnit(pHeadSelectedUnit, true);
	}
	else if (eMessage == GAMEMESSAGE_DO_COMMAND)
	{	// K-Mod. When setting a unit to automate, we must be careful not to keep it grouped
		if (iData2 == COMMAND_AUTOMATE && !gDLL->getInterfaceIFace()->mirrorsSelectionGroup())
			selectionListGameNetMessage(GAMEMESSAGE_JOIN_GROUP);
		// K-Mod end
		pSelectedUnitNode = gDLL->getInterfaceIFace()->headSelectionListNode();
		while (pSelectedUnitNode != NULL)
		{
			pSelectedUnit = ::getUnit(pSelectedUnitNode->m_data);
			pSelectedUnitNode = gDLL->getInterfaceIFace()->nextSelectionListNode(pSelectedUnitNode);
			CvMessageControl::getInstance().sendDoCommand(pSelectedUnit->getID(),
					(CommandTypes)iData2, iData3, iData4, bAlt);
		}
	}
	else if (eMessage == GAMEMESSAGE_PUSH_MISSION || eMessage == GAMEMESSAGE_AUTO_MISSION)
	{
		if (!gDLL->getInterfaceIFace()->mirrorsSelectionGroup())
			selectionListGameNetMessage(GAMEMESSAGE_JOIN_GROUP);

		if (eMessage == GAMEMESSAGE_PUSH_MISSION)
		{	// K-Mod. I've moved the BUTTONPOPUP_DECLAREWARMOVE stuff to here from selectionListMove
			// so that it can catch left-click moves as well as right-click moves.
			//
			// Note: If MOVE_DECLARE_WAR is set, then we assume it was set by a BUTTONPOPUP_DECLAREWARMOVE
			// which was triggered already by this move. In which case we shouldn't check for declare war
			// this time. This is a kludge to prevent the popup from appearing twice.
			// Also, when this happens we should clear the MOVE_DECLARE_WAR flag. Otherwise it may cause
			// the pathfinder to fail in some cases.
			//
			// (I'd rather not have UI stuff like this in this function,
			//  but this is the only place where I can catch left-click moves.)
			if (iData2 == MISSION_MOVE_TO && !(iFlags & MOVE_DECLARE_WAR))
			{
				CvPlot* pPlot = GC.getMap().plot(iData3, iData4);
				FAssert(pPlot);
				pSelectedUnitNode = gDLL->getInterfaceIFace()->headSelectionListNode();
				while (pSelectedUnitNode != NULL)
				{
					pSelectedUnit = ::getUnit(pSelectedUnitNode->m_data);
					TeamTypes eRivalTeam = pSelectedUnit->getDeclareWarMove(pPlot);
					if (eRivalTeam != NO_TEAM)
					{	/* <advc.001> If an enemy unit is stacked with a neutral one,
						then the player apparently wants to attack the enemy unit
						(rather than declare war on the neutral party). However,
						if the enemy unit is on a tile owned by a third party that
						the player doesn't have OB or a vassal treaty with, then
						only a DoW on the third party makes sense. */
						if((pPlot->getTeam() != NO_TEAM &&
								!GET_TEAM(pSelectedUnit->getTeam()).
								isFriendlyTerritory(pPlot->getTeam())) ||
								!pPlot->isVisibleEnemyUnit(pSelectedUnit))
						{ // </advc.001>
							CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_DECLAREWARMOVE);
							if (NULL != pInfo)
							{
								pInfo->setData1(eRivalTeam);
								pInfo->setData2(pPlot->getX());
								pInfo->setData3(pPlot->getY());
								pInfo->setOption1(bShift);
								pInfo->setOption2(pPlot->getTeam() != eRivalTeam);
								gDLL->getInterfaceIFace()->addPopup(pInfo);
							}
							return;
						}
					}
					pSelectedUnitNode = gDLL->getInterfaceIFace()->nextSelectionListNode(pSelectedUnitNode);
				}
			} // <advc.011b>
			bool bModified = false;
			if(iData2 == MISSION_BUILD)
				bModified = GC.ctrlKey(); // </advc.001b> <advc.048>
			if(iData2 == MISSION_MOVE_TO)
				bModified = GC.altKey(); // </advc.048>
			CvMessageControl::getInstance().sendPushMission(pHeadSelectedUnit->getID(), ((MissionTypes)iData2), iData3, iData4,
					iFlags & ~ MOVE_DECLARE_WAR, bShift, // K-Mod end
					bModified); // advc.011b
		}
		else CvMessageControl::getInstance().sendAutoMission(pHeadSelectedUnit->getID());
	}
	else FAssert(false);
}


void CvGame::selectedCitiesGameNetMessage(int eMessage, int iData2, int iData3, int iData4, bool bOption, bool bAlt, bool bShift, bool bCtrl) const
{
	CLLNode<IDInfo>* pSelectedCityNode = gDLL->getInterfaceIFace()->headSelectedCitiesNode();

	while (pSelectedCityNode != NULL)
	{
		CvCity* pSelectedCity = ::getCity(pSelectedCityNode->m_data);
		pSelectedCityNode = gDLL->getInterfaceIFace()->nextSelectedCitiesNode(pSelectedCityNode);

		if (pSelectedCity != NULL)
		{
			if (pSelectedCity->getOwner() == getActivePlayer())
			{
				switch (eMessage)
				{
				case GAMEMESSAGE_PUSH_ORDER:
					cityPushOrder(pSelectedCity, ((OrderTypes)iData2), iData3, bAlt, bShift, bCtrl);
					break;

				case GAMEMESSAGE_POP_ORDER:
					/* original bts code
					if (pSelectedCity->getOrderQueueLength() > 1)
					{
						CvMessageControl::getInstance().sendPopOrder(pSelectedCity->getID(), iData2);
					} */
					// K-Mod. Some additional controls
					if (bAlt || (bShift != bCtrl))
					{
						// bCtrl moves the order up, bShift moves the order down.
						// bAlt toggles bSave (ie. repeat)
						int iNewPos = iData2 + (bShift ? 1 : 0) + (bCtrl ? -1 : 0);
						if (pSelectedCity->getOrderQueueLength() > iNewPos && iNewPos >= 0)
						{
							OrderData order = pSelectedCity->getOrderData(iData2);
							if (order.eOrderType != NO_ORDER && (bShift || bCtrl || order.eOrderType == ORDER_TRAIN))
							{
								order.bSave = order.bSave != (bAlt && order.eOrderType == ORDER_TRAIN);
								CvMessageControl::getInstance().sendPopOrder(pSelectedCity->getID(), iData2);
								CvMessageControl::getInstance().sendPushOrder(pSelectedCity->getID(), order.eOrderType, order.iData1, order.bSave, false, iNewPos);
							}
						}
					}
					// Allow us to cancel the final order for automated cities. (The governor can choose production at the end of the turn.)
					else if (pSelectedCity->getOrderQueueLength() > 1 || pSelectedCity->isProductionAutomated())
						CvMessageControl::getInstance().sendPopOrder(pSelectedCity->getID(), iData2);
					// K-Mod end
					break;

				case GAMEMESSAGE_DO_TASK:
					CvMessageControl::getInstance().sendDoTask(pSelectedCity->getID(), ((TaskTypes)iData2), iData3, iData4, bOption, bAlt, bShift, bCtrl);
					break;

				default:
					FAssert(false);
					break;
				}
			}
		}
	}
}


bool CvGame::canHandleAction(int iAction, CvPlot* pPlot, bool bTestVisible, bool bUseCache) const  // advc: style changes
{
	PROFILE_FUNC();

	bool bShift = GC.shiftKey();

	if (GC.getPythonCaller()->cannotHandleActionOverride(*pPlot, iAction, bTestVisible))
		return false;

	if (GC.getActionInfo(iAction).getControlType() != NO_CONTROL &&
			canDoControl((ControlTypes)GC.getActionInfo(iAction).getControlType()))
		return true;

	if (gDLL->getInterfaceIFace()->isCitySelection())
		return false; // XXX hack!

	CvUnit* pHeadSelectedUnit = gDLL->getInterfaceIFace()->getHeadSelectedUnit();
	if (pHeadSelectedUnit == NULL || pHeadSelectedUnit->getOwner() != getActivePlayer())
		return false;

	if (!isMPOption(MPOPTION_SIMULTANEOUS_TURNS) && !GET_PLAYER(pHeadSelectedUnit->getOwner()).isTurnActive())
		return false;

	CvSelectionGroup* pSelectedInterfaceList = gDLL->getInterfaceIFace()->getSelectionList();
	if (GC.getActionInfo(iAction).getMissionType() != NO_MISSION)
	{
		CvPlot* pMissionPlot = NULL;
		if (gDLL->getInterfaceIFace()->mirrorsSelectionGroup())
		{
			CvSelectionGroup* pSelectedGroup = pHeadSelectedUnit->getGroup();
			if (pPlot != NULL)
				pMissionPlot = pPlot;
			else if (bShift)
				pMissionPlot = pSelectedGroup->lastMissionPlot();
			else pMissionPlot = NULL;
			if (pMissionPlot == NULL ||
					!pMissionPlot->isVisible(pHeadSelectedUnit->getTeam(), false))
				pMissionPlot = pSelectedGroup->plot();
		}
		else pMissionPlot = pSelectedInterfaceList->plot();
		if (pSelectedInterfaceList->canStartMission(GC.getActionInfo(iAction).
				getMissionType(), GC.getActionInfo(iAction).getMissionData(), -1, pMissionPlot, bTestVisible, bUseCache))
			return true;
	}
	if (GC.getActionInfo(iAction).getCommandType() != NO_COMMAND)
	{
		if (pSelectedInterfaceList->canDoCommand((CommandTypes)
				GC.getActionInfo(iAction).getCommandType(),
				GC.getActionInfo(iAction).getCommandData(), -1, bTestVisible, bUseCache))
			return true;
	}
	if (gDLL->getInterfaceIFace()->canDoInterfaceMode((InterfaceModeTypes)
			GC.getActionInfo(iAction).getInterfaceModeType(), pSelectedInterfaceList))
		return true;

	return false;
}

void CvGame::setupActionCache() const
{
	gDLL->getInterfaceIFace()->getSelectionList()->setupActionCache();
}

void CvGame::handleAction(int iAction)
{
	bool bAlt = GC.altKey();
	bool bShift = GC.shiftKey();

	if (!gDLL->getInterfaceIFace()->canHandleAction(iAction))
	{
		return;
	}

	if (GC.getActionInfo(iAction).getControlType() != NO_CONTROL)
	{
		doControl((ControlTypes)GC.getActionInfo(iAction).getControlType());
	}

	if (gDLL->getInterfaceIFace()->canDoInterfaceMode((InterfaceModeTypes)GC.getActionInfo(iAction).getInterfaceModeType(), gDLL->getInterfaceIFace()->getSelectionList()))
	{
		CvUnit* pHeadSelectedUnit = gDLL->getInterfaceIFace()->getHeadSelectedUnit();
		if (pHeadSelectedUnit != NULL)
		{
			if (GC.getInterfaceModeInfo((InterfaceModeTypes)GC.getActionInfo(iAction).getInterfaceModeType()).getSelectAll())
			{
				//gDLL->getInterfaceIFace()->selectGroup(pHeadSelectedUnit, false, false, true);
				gDLL->getInterfaceIFace()->selectGroup(pHeadSelectedUnit, false, true, true); // K-Mod
			}
			else if (GC.getInterfaceModeInfo((InterfaceModeTypes)GC.getActionInfo(iAction).getInterfaceModeType()).getSelectType())
			{
				gDLL->getInterfaceIFace()->selectGroup(pHeadSelectedUnit, false, true, false);
			}
		}

		gDLL->getInterfaceIFace()->setInterfaceMode((InterfaceModeTypes)GC.getActionInfo(iAction).getInterfaceModeType());
	}

	if (GC.getActionInfo(iAction).getMissionType() != NO_MISSION)
	{
		selectionListGameNetMessage(GAMEMESSAGE_PUSH_MISSION, GC.getActionInfo(iAction).getMissionType(), GC.getActionInfo(iAction).getMissionData(), -1, 0, false, bShift);
	}

	if (GC.getActionInfo(iAction).getCommandType() != NO_COMMAND)
	{
		bool bSkip = false;

		if (GC.getActionInfo(iAction).getCommandType() == COMMAND_LOAD)
		{
			CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_LOADUNIT);
			if (NULL != pInfo)
			{
				gDLL->getInterfaceIFace()->addPopup(pInfo);
				bSkip = true;
			}
		}

		if (!bSkip)
		{
			if (GC.getActionInfo(iAction).isConfirmCommand())
			{
				CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_CONFIRMCOMMAND);
				if (NULL != pInfo)
				{
					pInfo->setData1(iAction);
					pInfo->setOption1(bAlt);
					gDLL->getInterfaceIFace()->addPopup(pInfo);
				}
			}
			else
			{
				selectionListGameNetMessage(GAMEMESSAGE_DO_COMMAND, GC.getActionInfo(iAction).getCommandType(), GC.getActionInfo(iAction).getCommandData(), -1, 0, bAlt);
			}
		}
	}
}


bool CvGame::canDoControl(ControlTypes eControl) const
{
	if (GC.getPythonCaller()->cannotDoControlOverride(eControl))
		return false;

	/*  <advc.706> I don't think loading is possible in between turns, but there
		would be no harm in it. */
	if(CvPlot::isAllFog() && eControl != CONTROL_LOAD_GAME &&
			eControl != CONTROL_QUICK_LOAD && eControl != CONTROL_OPTIONS_SCREEN)
		return false;
	// </advc.706>
	switch (eControl)
	{
	case CONTROL_SELECTYUNITTYPE:
	case CONTROL_SELECTYUNITALL:
	case CONTROL_SELECT_HEALTHY:
	case CONTROL_SELECTCITY:
	case CONTROL_SELECTCAPITAL:
	case CONTROL_NEXTUNIT:
	case CONTROL_PREVUNIT:
	case CONTROL_CYCLEUNIT:
	case CONTROL_CYCLEUNIT_ALT:
	case CONTROL_CYCLEWORKER:
	case CONTROL_LASTUNIT:
	case CONTROL_AUTOMOVES:
	case CONTROL_SAVE_GROUP:
	case CONTROL_QUICK_SAVE:
	case CONTROL_QUICK_LOAD:
	case CONTROL_ORTHO_CAMERA:
	case CONTROL_CYCLE_CAMERA_FLYING_MODES:
	case CONTROL_ISOMETRIC_CAMERA_LEFT:
	case CONTROL_ISOMETRIC_CAMERA_RIGHT:
	case CONTROL_FLYING_CAMERA:
	case CONTROL_MOUSE_FLYING_CAMERA:
	case CONTROL_TOP_DOWN_CAMERA:
	case CONTROL_TURN_LOG:
	case CONTROL_CHAT_ALL:
	case CONTROL_CHAT_TEAM:
	case CONTROL_GLOBE_VIEW:
		if (!gDLL->getInterfaceIFace()->isFocused())
		{
			return true;
		}
		break;

	case CONTROL_FORCEENDTURN:
		if (!gDLL->getInterfaceIFace()->isFocused() && !gDLL->getInterfaceIFace()->isInAdvancedStart())
		{
			return true;
		}
		break;


	case CONTROL_PING:
	case CONTROL_SIGN:
	case CONTROL_GRID:
	case CONTROL_BARE_MAP:
	case CONTROL_YIELDS:
	case CONTROL_RESOURCE_ALL:
	case CONTROL_UNIT_ICONS:
	case CONTROL_GLOBELAYER:
	case CONTROL_SCORES:
	case CONTROL_FREE_COLONY:
		if (!gDLL->getInterfaceIFace()->isFocusedWidget())
		{
			return true;
		}
		break;

	case CONTROL_OPTIONS_SCREEN:
	case CONTROL_DOMESTIC_SCREEN:
	case CONTROL_VICTORY_SCREEN:
	case CONTROL_CIVILOPEDIA:
	case CONTROL_RELIGION_SCREEN:
	case CONTROL_CORPORATION_SCREEN:
	case CONTROL_CIVICS_SCREEN:
	case CONTROL_FOREIGN_SCREEN:
	case CONTROL_FINANCIAL_SCREEN:
	case CONTROL_MILITARY_SCREEN:
	case CONTROL_TECH_CHOOSER:
	case CONTROL_DIPLOMACY:
	case CONTROL_HALL_OF_FAME:
	case CONTROL_INFO:
	case CONTROL_DETAILS:
	case CONTROL_SAVE_NORMAL:
		return true;
		break;
	case CONTROL_ESPIONAGE_SCREEN:
		if (!isOption(GAMEOPTION_NO_ESPIONAGE))
		{
			return true;
		}
		break;

	case CONTROL_NEXTCITY:
	case CONTROL_PREVCITY:
		if (!gDLL->getInterfaceIFace()->isSpaceshipScreenUp())
		{
			return true;
		}
		break;

	case CONTROL_ADMIN_DETAILS:
		return true;
		break;

	case CONTROL_CENTERONSELECTION:
		if (gDLL->getInterfaceIFace()->getLookAtPlot() != gDLL->getInterfaceIFace()->getSelectionPlot())
		{
			return true;
		}
		break;

	case CONTROL_LOAD_GAME:
		if (!(isNetworkMultiPlayer()))
		{
			return true;
		}
		break;

	case CONTROL_RETIRE:
		if ((getGameState() == GAMESTATE_ON) || isGameMultiPlayer())
		{
			if (GET_PLAYER(getActivePlayer()).isAlive())
			{
				if (isPbem() || isHotSeat())
				{
					if (!GET_PLAYER(getActivePlayer()).isEndTurn())
					{
						return true;
					}
				}
				else
				{
					return true;
				}
			}
		}
		break;

	case CONTROL_WORLD_BUILDER:
		return isDebugToolsAllowed(true); // advc.135c
		/*if (!(isGameMultiPlayer()) && GC.getInitCore().getAdminPassword().empty() && !gDLL->getInterfaceIFace()->isInAdvancedStart())
			return true;*/
		break;

	case CONTROL_ENDTURN:
	case CONTROL_ENDTURN_ALT:
		if (gDLL->getInterfaceIFace()->isEndTurnMessage() && !gDLL->getInterfaceIFace()->isFocused() && !gDLL->getInterfaceIFace()->isInAdvancedStart())
		{
			return true;
		}
		break;

	default:
		FAssertMsg(false, "eControl did not match any valid options");
		break;
	}

	return false;
}


void CvGame::doControl(ControlTypes eControl)
{
	CvPopupInfo* pInfo;
	CvUnit* pHeadSelectedUnit;
	CvUnit* pUnit;
	CvPlot* pPlot;

	if (!canDoControl(eControl))
		return;
	// <advc>
	CvDLLInterfaceIFaceBase* pInterface = gDLL->getInterfaceIFace();
	CvDLLEngineIFaceBase* pEngine = gDLL->getEngineIFace(); // </advc>
	switch (eControl)
	{
	case CONTROL_CENTERONSELECTION:
		pInterface->lookAtSelectionPlot();
		break;

	case CONTROL_SELECTYUNITTYPE:
		pHeadSelectedUnit = pInterface->getHeadSelectedUnit();
		if (pHeadSelectedUnit != NULL)
		{
			pInterface->selectGroup(pHeadSelectedUnit, false, true, false);
		}
		break;

	case CONTROL_SELECTYUNITALL:
		pHeadSelectedUnit = pInterface->getHeadSelectedUnit();
		if (pHeadSelectedUnit != NULL)
		{
			//pInterface->selectGroup(pHeadSelectedUnit, false, false, true);
			pInterface->selectGroup(pHeadSelectedUnit, false, true, true); // K-Mod
		}
		break;

	case CONTROL_SELECT_HEALTHY:
		{
			CvUnit* pGroupHead = NULL;
			pHeadSelectedUnit = pInterface->getHeadSelectedUnit();
			pInterface->clearSelectionList();
			if (pHeadSelectedUnit != NULL)
			{
				pPlot = pHeadSelectedUnit->plot();
				std::vector<CvUnit *> plotUnits;
				getPlotUnits(pPlot, plotUnits);
				pInterface->selectionListPreChange();
				for (int iI = 0; iI < (int) plotUnits.size(); iI++)
				{
					pUnit = plotUnits[iI];

					if (pUnit->getOwner() == getActivePlayer())
					{
						//if (!isMPOption(MPOPTION_SIMULTANEOUS_TURNS) || getTurnSlice() - pUnit->getLastMoveTurn() > GC.getDefineINT("MIN_TIMER_UNIT_DOUBLE_MOVES")) // disabled by K-Mod
						{
							if (pUnit->isHurt())
							{
								if (pGroupHead != NULL)
								{
									CvMessageControl::getInstance().sendJoinGroup(pUnit->getID(), pGroupHead->getID());
								}
								else
								{
									pGroupHead = pUnit;
								}

								pInterface->insertIntoSelectionList(pUnit, false, false, true, true, true);
							}
						}
					}
				}

				pInterface->selectionListPostChange();
			}
		}
		break;

	case CONTROL_SELECTCITY:
		if (pInterface->isCityScreenUp())
		{
			cycleCities();
		}
		else
		{
			pInterface->selectLookAtCity();
		}
		break;

	case CONTROL_SELECTCAPITAL: {
		CvCity* pCapitalCity = GET_PLAYER(getActivePlayer()).getCapitalCity();
		if (pCapitalCity != NULL)
		{
			pInterface->selectCity(pCapitalCity);
		}
		break;
	}
	case CONTROL_NEXTCITY:
		if (pInterface->isCitySelection())
		{
			cycleCities(true, !(pInterface->isCityScreenUp()));
		}
		else
		{
			pInterface->selectLookAtCity(true);
		}
		pInterface->lookAtSelectionPlot();
		break;

	case CONTROL_PREVCITY:
		if (pInterface->isCitySelection())
		{
			cycleCities(false, !(pInterface->isCityScreenUp()));
		}
		else
		{
			pInterface->selectLookAtCity(true);
		}
		pInterface->lookAtSelectionPlot();
		break;

	case CONTROL_NEXTUNIT:
		pPlot = pInterface->getSelectionPlot();
		if (pPlot != NULL)
		{
			cyclePlotUnits(pPlot);
		}
		break;

	case CONTROL_PREVUNIT:
		pPlot = pInterface->getSelectionPlot();
		if (pPlot != NULL)
		{
			cyclePlotUnits(pPlot, false);
		}
		break;

	case CONTROL_CYCLEUNIT:
	case CONTROL_CYCLEUNIT_ALT:
		cycleSelectionGroups(true);
		break;

	case CONTROL_CYCLEWORKER:
		cycleSelectionGroups(true, true, true);
		break;

	case CONTROL_LASTUNIT:
		pUnit = pInterface->getLastSelectedUnit();

		if (pUnit != NULL)
		{
			pInterface->selectUnit(pUnit, true);
			pInterface->lookAtSelectionPlot();
		}
		else
		{
			cycleSelectionGroups(true, false);
		}

		pInterface->setLastSelectedUnit(NULL);
		break;

	case CONTROL_ENDTURN:
	case CONTROL_ENDTURN_ALT:
		if (pInterface->isEndTurnMessage())
		{
			CvMessageControl::getInstance().sendTurnComplete();
		}
		break;

	case CONTROL_FORCEENDTURN:
		CvMessageControl::getInstance().sendTurnComplete();
		break;

	case CONTROL_AUTOMOVES:
		CvMessageControl::getInstance().sendAutoMoves();
		break;

	case CONTROL_PING:
		pInterface->setInterfaceMode(INTERFACEMODE_PING);
		break;

	case CONTROL_SIGN:
		pInterface->setInterfaceMode(INTERFACEMODE_SIGN);
		break;

	case CONTROL_GRID:
		pEngine->SetGridMode(!pEngine->GetGridMode());
		break;

	case CONTROL_BARE_MAP:
		pInterface->toggleBareMapMode();
		break;

	case CONTROL_YIELDS:
		pInterface->toggleYieldVisibleMode();
		break;

	case CONTROL_RESOURCE_ALL:
		pEngine->toggleResourceLayer();
		break;

	case CONTROL_UNIT_ICONS:
		pEngine->toggleUnitLayer();
		break;

	case CONTROL_GLOBELAYER:
		pEngine->toggleGlobeview();
		break;

	case CONTROL_SCORES:
		pInterface->toggleScoresVisible();
		break;

	case CONTROL_LOAD_GAME:
		gDLL->LoadGame();
		break;

	case CONTROL_OPTIONS_SCREEN:
		GC.getPythonCaller()->showPythonScreen("OptionsScreen");
		break;

	case CONTROL_RETIRE: // <advc.706> Need three buttons, so no BUTTONPOPUP_CONFIRM_MENU.
		if(isOption(GAMEOPTION_RISE_FALL)) {
			pInfo = new CvPopupInfo(BUTTONPOPUP_RF_RETIRE);
			if(pInfo != NULL)
				pInterface->addPopup(pInfo, getActivePlayer(), true);
		}
		else // </advc.706>
		// K-Mod. (original code moved into CvGame::retire)
		{
			pInfo = new CvPopupInfo(BUTTONPOPUP_CONFIRM_MENU);
			if (NULL != pInfo)
			{
				pInfo->setData1(2);
				pInterface->addPopup(pInfo, getActivePlayer(), true);
			}
		}
		// K-Mod end
		break;

	case CONTROL_SAVE_GROUP:
		gDLL->SaveGame(SAVEGAME_GROUP);
		break;

	case CONTROL_SAVE_NORMAL:
		gDLL->SaveGame(SAVEGAME_NORMAL);
		break;

	case CONTROL_QUICK_SAVE:
		if (!isNetworkMultiPlayer())	// SP only!
		{
			CvEventReporter::getInstance().preQuickSave(); // advc.106l
			gDLL->QuickSave();
		}
		break;

	case CONTROL_QUICK_LOAD:
		if (!isNetworkMultiPlayer())	// SP only!
		{	// <advc.003d>
			/*  Loading works fine in windowed mode, and when a debugger is
				attached, exitingToMainMenu can actually be quite slow.
				(Fullscreen pretty much rules out that a debugger is attached.) */
			if(gDLL->getGraphicOption(GRAPHICOPTION_FULLSCREEN)) {
				/*  On my system, it's "C:\\Users\\Administrator\\Documents\\My Games\\Beyond the Sword\\Saves\\single\\quick\\QuickSave.CivBeyondSwordSave";
					the user directory can vary. */
				CvString szQuickSavePath(::getUserDirPath());
				if(!szQuickSavePath.empty()) {
					szQuickSavePath += "\\Beyond the Sword\\Saves\\single\\quick\\QuickSave.CivBeyondSwordSave";
					// CTD if loading fails, so let's make sure that the file is good.
					std::ifstream quickSaveFile(szQuickSavePath);
					if(quickSaveFile.good()) {
						pInterface->exitingToMainMenu(szQuickSavePath.c_str());
						break;
					}
				}
				FAssertMsg(false, "Failed to find quicksave");
			} // </advc.003d>
			gDLL->QuickLoad();
		}
		break;

	case CONTROL_ORTHO_CAMERA:
		pEngine->SetOrthoCamera(!pEngine->GetOrthoCamera());
		break;

	case CONTROL_CYCLE_CAMERA_FLYING_MODES:
		pEngine->CycleFlyingMode(1);
		break;

	case CONTROL_ISOMETRIC_CAMERA_LEFT:
		pEngine->MoveBaseTurnLeft();
		break;

	case CONTROL_ISOMETRIC_CAMERA_RIGHT:
		pEngine->MoveBaseTurnRight();
		break;

	case CONTROL_FLYING_CAMERA:
		pEngine->SetFlying(!pEngine->GetFlying());
		break;

	case CONTROL_MOUSE_FLYING_CAMERA:
		pEngine->SetMouseFlying(!pEngine->GetMouseFlying());
		break;

	case CONTROL_TOP_DOWN_CAMERA:
		pEngine->SetSatelliteMode(!pEngine->GetSatelliteMode());
		break;

	case CONTROL_CIVILOPEDIA:
		GC.getPythonCaller()->callScreenFunction("pediaShow");
		break;

	case CONTROL_RELIGION_SCREEN:
		GC.getPythonCaller()->showPythonScreen("ReligionScreen");
		break;

	case CONTROL_CORPORATION_SCREEN:
		GC.getPythonCaller()->showPythonScreen("CorporationScreen");
		break;

	case CONTROL_CIVICS_SCREEN:
		GC.getPythonCaller()->showPythonScreen("CivicsScreen");
		break;

	case CONTROL_FOREIGN_SCREEN:
		GC.getPythonCaller()->showForeignAdvisorScreen();
		break;

	case CONTROL_FINANCIAL_SCREEN:
		GC.getPythonCaller()->showPythonScreen("FinanceAdvisor");
		break;

	case CONTROL_MILITARY_SCREEN:
		GC.getPythonCaller()->showPythonScreen("MilitaryAdvisor");
		break;

	case CONTROL_TECH_CHOOSER:
		GC.getPythonCaller()->showPythonScreen("TechChooser");
		break;

	case CONTROL_TURN_LOG:
		if (!gDLL->GetWorldBuilderMode() || pInterface->isInAdvancedStart())
			pInterface->toggleTurnLog();
		break;

	case CONTROL_CHAT_ALL:
		if (!gDLL->GetWorldBuilderMode() || pInterface->isInAdvancedStart())
			pInterface->showTurnLog(CHATTARGET_ALL);
		break;

	case CONTROL_CHAT_TEAM:
		if (!gDLL->GetWorldBuilderMode() || pInterface->isInAdvancedStart())
			pInterface->showTurnLog(CHATTARGET_TEAM);
		break;

	case CONTROL_DOMESTIC_SCREEN:
		//argsList.add(-1); // advc.003y: Unused param removed from CyScreensInterface.py
		GC.getPythonCaller()->showPythonScreen("DomesticAdvisor");
		break;

	case CONTROL_VICTORY_SCREEN:
		GC.getPythonCaller()->showPythonScreen("VictoryScreen");
		break;

	case CONTROL_INFO:
		GC.getPythonCaller()->showInfoScreen(0, getGameState() != GAMESTATE_ON);
		break;

	case CONTROL_GLOBE_VIEW:
		pEngine->toggleGlobeview();
		break;

	case CONTROL_DETAILS:
		pInterface->showDetails();
		break;

	case CONTROL_ADMIN_DETAILS:
		if (GC.getInitCore().getAdminPassword().empty())
			pInterface->showAdminDetails();
		else
		{
			pInfo = new CvPopupInfo(BUTTONPOPUP_ADMIN_PASSWORD);
			if (NULL != pInfo)
			{
				pInfo->setData1((int)CONTROL_ADMIN_DETAILS);
				pInterface->addPopup(pInfo, NO_PLAYER, true);
			}
		}
		break;

	case CONTROL_HALL_OF_FAME:
		GC.getPythonCaller()->showHallOfFameScreen(true);
		break;

	case CONTROL_WORLD_BUILDER:
		// K-Mod. (original code moved into CvGame::retire)
		// <advc.007>
		if(isDebugMode())
			enterWorldBuilder();
		else { // </advc.007>
			pInfo = new CvPopupInfo(BUTTONPOPUP_CONFIRM_MENU);
			if (pInfo != NULL)
			{
				pInfo->setData1(4);
				pInterface->addPopup(pInfo, getActivePlayer(), true);
			}
		} // K-Mod end
		break;

	case CONTROL_ESPIONAGE_SCREEN:
		GC.getPythonCaller()->showPythonScreen("EspionageAdvisor");
		break;

	case CONTROL_FREE_COLONY:
	{
		pInfo = new CvPopupInfo(BUTTONPOPUP_FREE_COLONY);
		if (pInfo != NULL)
			pInterface->addPopup(pInfo);
		break;
	}
	case CONTROL_DIPLOMACY:
		pInfo = new CvPopupInfo(BUTTONPOPUP_DIPLOMACY);
		if (pInfo != NULL)
			pInterface->addPopup(pInfo);
		break;

	default:
		FAssertMsg(false, "eControl did not match any valid options");
		break;
	}
}

// K-Mod. This code use to be inside CvGame::doControl. I've moved it here and told doControl to simply create a confirmation popup.
void CvGame::retire()
{
	FAssert(canDoControl(CONTROL_RETIRE));

	if (!isGameMultiPlayer() || countHumanPlayersAlive() == 1)
	{
		if (gDLL->GetAutorun())
		{
			GC.getInitCore().setSlotStatus(getActivePlayer(), SS_COMPUTER);
		}
		else
		{
			setGameState(GAMESTATE_OVER);
			gDLL->getInterfaceIFace()->setDirty(Soundtrack_DIRTY_BIT, true);
		}
	}
	else
	{
		if (isNetworkMultiPlayer())
		{
			gDLL->sendMPRetire();
			gDLL->getInterfaceIFace()->exitingToMainMenu();
		}
		else
		{
			gDLL->handleRetirement(getActivePlayer());
		}
	}
}

void CvGame::enterWorldBuilder()
{
	FAssert(canDoControl(CONTROL_WORLD_BUILDER));
	if (GC.getInitCore().getAdminPassword().empty())
	{	// <advc.315c>
		/*  In multiplayer, setWorldBuilder apparently checks ChtLvl>0 and setChtLvl
			doesn't work. Need to make the EXE believe that we're in singleplayer. */
		m_bFeignSP = true;
		gDLL->setChtLvl(1); // </advc.315c>
		gDLL->getInterfaceIFace()->setWorldBuilder(!(gDLL->GetWorldBuilderMode()));
		m_bFeignSP = false; // advc.315c
	}
	else
	{
		CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_ADMIN_PASSWORD);
		if (NULL != pInfo)
		{
			pInfo->setData1((int)CONTROL_WORLD_BUILDER);
			gDLL->getInterfaceIFace()->addPopup(pInfo, NO_PLAYER, true);
		}
	}
}
// K-Mod end

void CvGame::getGlobeLayers(std::vector<CvGlobeLayerData>& aLayers) const
{
	CvGlobeLayerData kUnit(GLOBE_LAYER_UNIT);
	kUnit.m_strName = "UNITS";
	kUnit.m_strButtonHelpTag = "TXT_KEY_GLOBELAYER_UNITS";
	kUnit.m_strButtonStyle = "Button_HUDGlobeUnit_Style";
	kUnit.m_iNumOptions =
			(GC.getDefineINT("SHOW_UNIT_LAYER_OPTIONS") <= 0 ? 0 : // advc.004z
			NUM_UNIT_OPTION_TYPES);
	kUnit.m_bGlobeViewRequired = false;
	aLayers.push_back(kUnit);

	CvGlobeLayerData kResource(GLOBE_LAYER_RESOURCE);
	kResource.m_strName = "RESOURCES";
	kResource.m_strButtonHelpTag = "TXT_KEY_GLOBELAYER_RESOURCES";
	kResource.m_strButtonStyle = "Button_HUDBtnResources_Style";
	/*  advc.004z (comment): Could check and enforce
		getBugOptionBOOL("MainInterface__ResourceIconOptions", false)
		here, but then the BUG option would require a restart. Therefore,
		this is done in CvMainInterface.py (updateGlobeviewButtons) instead. */
	kResource.m_iNumOptions = NUM_RESOURCE_OPTION_TYPES;
	kResource.m_bGlobeViewRequired = false;
	aLayers.push_back(kResource);

	CvGlobeLayerData kCulture(GLOBE_LAYER_CULTURE);
	kCulture.m_strName = "CULTURE";
	kCulture.m_strButtonHelpTag = "TXT_KEY_GLOBELAYER_CULTURE";
	kCulture.m_strButtonStyle = "Button_HUDGlobeCulture_Style";
	kCulture.m_bShouldCitiesZoom = true;
	aLayers.push_back(kCulture);

	CvGlobeLayerData kTrade(GLOBE_LAYER_TRADE);
	kTrade.m_strName = "TRADE";
	kTrade.m_strButtonHelpTag = "TXT_KEY_GLOBELAYER_TRADE";
	kTrade.m_strButtonStyle = "Button_HUDGlobeTrade_Style";
	kTrade.m_bShouldCitiesZoom = true;
	aLayers.push_back(kTrade);

	CvGlobeLayerData kReligion(GLOBE_LAYER_RELIGION);
	kReligion.m_strName = "RELIGION";
	kReligion.m_strButtonHelpTag = "TXT_KEY_GLOBELAYER_RELIGION";
	kReligion.m_strButtonStyle = "Button_HUDGlobeReligion_Style";
	kReligion.m_iNumOptions = GC.getNumReligionInfos();
	kReligion.m_bShouldCitiesZoom = true;
	aLayers.push_back(kReligion);
}

void CvGame::startFlyoutMenu(const CvPlot* pPlot, std::vector<CvFlyoutMenuData>& aFlyoutItems) const
{
	aFlyoutItems.clear();

	bool bUnits = false;
	bool bFortifyUnit = false;
	bool bSleepUnit = false;
	bool bWakeUnit = false;
	CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = pPlot->nextUnitNode(pUnitNode);

		if (pLoopUnit->getOwner() == getActivePlayer())
		{
			bUnits = true;

			if (pLoopUnit->canFortify(pPlot))
			{
				bFortifyUnit = true;
			}
			else if (pLoopUnit->canSleep(pPlot))
			{
				bSleepUnit = true;
			}
			else if (pLoopUnit->isWaiting())
			{
				bWakeUnit = true;
			}
		}
	}

	CvWString szBuffer;
	CvCity* pCity = pPlot->getPlotCity();
	if (pCity != NULL)
	{
		if (pCity->getOwner() == getActivePlayer())
		{
			szBuffer = gDLL->getText("TXT_KEY_CHANGE_PRODUCTION");
			aFlyoutItems.push_back(CvFlyoutMenuData(NO_FLYOUT, -1, -1, -1, szBuffer));
			CvCivilization const& kCiv = pCity->getCivilization(); // advc.003w
			for (int i = 0; i < kCiv.getNumUnits(); i++)
			{
				UnitTypes eLoopUnit = kCiv.unitAt(i);
				if (!pCity->canTrain(eLoopUnit))
					continue; // advc
				szBuffer = GC.getUnitInfo(eLoopUnit).getDescription();
				int iTurns = pCity->getProductionTurnsLeft(eLoopUnit, 0);
				if(iTurns < MAX_INT) // advc.004x
					szBuffer.append(CvWString::format(L" (%d)", iTurns));
				aFlyoutItems.push_back(CvFlyoutMenuData(FLYOUT_TRAIN, eLoopUnit,
						pPlot->getX(), pPlot->getY(), szBuffer));
			}
			for (int i = 0; i < kCiv.getNumBuildings(); i++)
			{
				BuildingTypes eLoopBuilding = kCiv.buildingAt(i);
				if (!pCity->canConstruct(eLoopBuilding))
					continue; // advc
				szBuffer = GC.getBuildingInfo(eLoopBuilding).getDescription();
				int iTurns = pCity->getProductionTurnsLeft(eLoopBuilding, 0);
				if(iTurns < MAX_INT) // advc.004x
					szBuffer.append(CvWString::format(L" (%d)", iTurns));
				aFlyoutItems.push_back(CvFlyoutMenuData(FLYOUT_CONSTRUCT, eLoopBuilding,
						pPlot->getX(), pPlot->getY(), szBuffer));
			}
			for (int iI = 0; iI < GC.getNumProjectInfos(); iI++)
			{
				ProjectTypes eLoopProject = (ProjectTypes) iI;
				if (!pCity->canCreate(eLoopProject))
					continue; // advc
				szBuffer = GC.getProjectInfo(eLoopProject).getDescription();
				int iTurns = pCity->getProductionTurnsLeft(eLoopProject, 0);
				if(iTurns < MAX_INT) // advc.004x
					szBuffer.append(CvWString::format(L" (%d)", iTurns));
				aFlyoutItems.push_back(CvFlyoutMenuData(FLYOUT_CREATE, eLoopProject,
						pPlot->getX(), pPlot->getY(), szBuffer));
			}
			for (int iI = 0; iI < GC.getNumProcessInfos(); iI++)
			{
				ProcessTypes eLoopProcess = (ProcessTypes)iI;
				if (!pCity->canMaintain(eLoopProcess))
					continue; // advc
				szBuffer = GC.getProcessInfo(eLoopProcess).getDescription();
				aFlyoutItems.push_back(CvFlyoutMenuData(FLYOUT_MAINTAIN, eLoopProcess,
						pPlot->getX(), pPlot->getY(), szBuffer));
			}


			aFlyoutItems.push_back(CvFlyoutMenuData(NO_FLYOUT, -1, -1, -1, L""));

			for (int iI = 0; iI < GC.getNumHurryInfos(); iI++)
			{
				if (pCity->canHurry((HurryTypes)iI))
				{
					szBuffer = gDLL->getText("TXT_KEY_HURRY_PRODUCTION");

					int iHurryGold = pCity->hurryGold((HurryTypes)iI);
					if (iHurryGold > 0)
					{
						szBuffer += gDLL->getText("TXT_KEY_HURRY_PRODUCTION_GOLD", iHurryGold);
					}

					int iHurryPopulation = pCity->hurryPopulation((HurryTypes)iI);
					if (iHurryPopulation > 0)
					{
						szBuffer += gDLL->getText("TXT_KEY_HURRY_PRODUCTION_POP", iHurryPopulation);
					}

					aFlyoutItems.push_back(CvFlyoutMenuData(FLYOUT_HURRY, iI, pPlot->getX(), pPlot->getY(), szBuffer));
				}
			}

			if (pCity->canConscript())
			{
				UnitTypes eConscriptUnit = pCity->getConscriptUnit();
				if (eConscriptUnit != NO_UNIT)
				{
					szBuffer = gDLL->getText("TXT_KEY_DRAFT_UNIT", GC.getUnitInfo(eConscriptUnit).getDescription(), pCity->getConscriptPopulation());
					aFlyoutItems.push_back(CvFlyoutMenuData(FLYOUT_CONSCRIPT, GC.getNumHurryInfos(), pPlot->getX(), pPlot->getY(), szBuffer));
				}
			}
		}
	}

	CvUnit* pHeadSelectedUnit = gDLL->getInterfaceIFace()->getHeadSelectedUnit();
	if (pHeadSelectedUnit != NULL && !pHeadSelectedUnit->atPlot(pPlot))
	{
		gDLL->getFAStarIFace()->SetData(&GC.getInterfacePathFinder(), gDLL->getInterfaceIFace()->getSelectionList());
		if (pHeadSelectedUnit->getDomainType() == DOMAIN_AIR ||
				gDLL->getFAStarIFace()->GeneratePath(&GC.getInterfacePathFinder(),
				pHeadSelectedUnit->getX(), pHeadSelectedUnit->getY(),
				pPlot->getX(), pPlot->getY(), false, MOVE_DECLARE_WAR, true))
		{
			if (pHeadSelectedUnit->getDomainType() == DOMAIN_AIR)
			{
				szBuffer = gDLL->getText("TXT_KEY_FLYOUT_MENU_FLY_TO");
			}
			else
			{
				szBuffer = gDLL->getText("TXT_KEY_FLYOUT_MENU_MOVE_TO", gDLL->getFAStarIFace()->GetLastNode(&GC.getInterfacePathFinder())->m_iData2);
			}
			aFlyoutItems.push_back(CvFlyoutMenuData(FLYOUT_MOVE_TO, 0, pPlot->getX(), pPlot->getY(), szBuffer));
		}
	}

	if (bUnits)
	{
		szBuffer = gDLL->getText("TXT_KEY_FLYOUT_MENU_SELECT_ALL");
		aFlyoutItems.push_back(CvFlyoutMenuData(FLYOUT_SELECT_ALL, 0, pPlot->getX(), pPlot->getY(), szBuffer));

		if (bWakeUnit)
		{
			szBuffer = gDLL->getText("TXT_KEY_FLYOUT_MENU_WAKE_ALL");
			aFlyoutItems.push_back(CvFlyoutMenuData(FLYOUT_WAKE_ALL, 0, pPlot->getX(), pPlot->getY(), szBuffer));
		}

		if (bFortifyUnit)
		{
			szBuffer = gDLL->getText("TXT_KEY_FLYOUT_MENU_FORTIFY_ALL");
			aFlyoutItems.push_back(CvFlyoutMenuData(FLYOUR_FORTIFY_ALL, 0, pPlot->getX(), pPlot->getY(), szBuffer));
		}
		else if (bSleepUnit)
		{
			szBuffer = gDLL->getText("TXT_KEY_FLYOUT_MENU_SLEEP_ALL");
			aFlyoutItems.push_back(CvFlyoutMenuData(FLYOUR_SLEEP_ALL, 0, pPlot->getX(), pPlot->getY(), szBuffer));
		}

		static std::vector<CvUnit*> plotUnits;
		getPlotUnits(pPlot, plotUnits);
		for (int iI = 0; iI < (int) plotUnits.size(); iI++)
		{
			CvUnit* pLoopUnit = plotUnits[iI];
			if (pLoopUnit->getOwner() == getActivePlayer())
			{
				CvWStringBuffer szTempBuffer;
				GAMETEXT.setUnitHelp(szTempBuffer, pLoopUnit, true);
				aFlyoutItems.push_back(CvFlyoutMenuData(FLYOUT_SELECT_UNIT, pLoopUnit->getID(), pPlot->getX(), pPlot->getY(), szTempBuffer.getCString()));
			}
		}
	}
}


void CvGame::applyFlyoutMenu(const CvFlyoutMenuData& kItem)
{
	CvPlot* pPlot = GC.getMap().plot(kItem.m_iX, kItem.m_iY);
	if (pPlot != NULL)
	{
		switch (kItem.m_eFlyout)
		{
		case NO_FLYOUT:
		default:
			FAssert(false);
			break;

		case FLYOUT_HURRY:
			{
				CvCity* pCity = pPlot->getPlotCity();
				if (pCity != NULL)
				{
					if (pCity->getOwner() == getActivePlayer())
					{
						CvMessageControl::getInstance().sendDoTask(pCity->getID(), TASK_HURRY, kItem.m_iID, -1, false, false, false, false);
					}
				}
			}
			break;

		case FLYOUT_CONSCRIPT:
			{
				CvCity* pCity = pPlot->getPlotCity();
				if (pCity != NULL)
				{
					if (pCity->getOwner() == getActivePlayer())
					{
						CvMessageControl::getInstance().sendDoTask(pCity->getID(), TASK_CONSCRIPT, -1, -1, false, false, false, false);
					}
				}
			}
			break;

		case FLYOUT_TRAIN:
			{
				CvCity* pCity = pPlot->getPlotCity();
				if (pCity != NULL)
				{
					if (pCity->getOwner() == getActivePlayer())
					{
						cityPushOrder(pCity, ORDER_TRAIN, kItem.m_iID);
					}
				}
			}
			break;

		case FLYOUT_CONSTRUCT:
			{
				CvCity* pCity = pPlot->getPlotCity();
				if (pCity != NULL)
				{
					if (pCity->getOwner() == getActivePlayer())
					{
						cityPushOrder(pCity, ORDER_CONSTRUCT, kItem.m_iID);
					}
				}
			}
			break;

		case FLYOUT_CREATE:
			{
				CvCity* pCity = pPlot->getPlotCity();
				if (pCity != NULL)
				{
					if (pCity->getOwner() == getActivePlayer())
					{
						cityPushOrder(pCity, ORDER_CREATE, kItem.m_iID);
					}
				}
			}
			break;

		case FLYOUT_MAINTAIN:
			{
				CvCity* pCity = pPlot->getPlotCity();
				if (pCity != NULL)
				{
					if (pCity->getOwner() == getActivePlayer())
					{
						cityPushOrder(pCity, ORDER_MAINTAIN, kItem.m_iID);
					}
				}
			}
			break;

		case FLYOUT_MOVE_TO:
			selectionListMove(pPlot, false, false, false);
			break;

		case FLYOUT_SELECT_UNIT:
			{
				CvUnit* pUnit = GET_PLAYER(getActivePlayer()).getUnit(kItem.m_iID);
				if (pUnit != NULL)
				{
					gDLL->getInterfaceIFace()->selectUnit(pUnit, true);
				}
			}
			break;

		case FLYOUT_SELECT_ALL:
			gDLL->getInterfaceIFace()->selectAll(pPlot);
			break;

		case FLYOUT_WAKE_ALL:
			{
				CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();
				while (pUnitNode != NULL)
				{
					CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
					pUnitNode = pPlot->nextUnitNode(pUnitNode);
					//if (pLoopUnit->isGroupHead())
					if (pLoopUnit->isGroupHead() && pLoopUnit->getOwner() == getActivePlayer()) // K-Mod
					{
						CvMessageControl::getInstance().sendDoCommand(pLoopUnit->getID(), COMMAND_WAKE, -1, -1, false);
					}
				}
			}
			break;

		case FLYOUR_FORTIFY_ALL:
		case FLYOUR_SLEEP_ALL:
			{
				CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();
				while (pUnitNode != NULL)
				{
					CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
					pUnitNode = pPlot->nextUnitNode(pUnitNode);
					if (pLoopUnit->isGroupHead() && pLoopUnit->getOwner() == getActivePlayer()) // K-Mod
					{
						CvMessageControl::getInstance().sendPushMission(pLoopUnit->getID(), ((pLoopUnit->isFortifyable()) ? MISSION_FORTIFY : MISSION_SLEEP), -1, -1, 0, false,
								GC.ctrlKey()); // advc.011b
					}
				}
			}
			break;
		}
	}
}

CvPlot* CvGame::getNewHighlightPlot() const  // advc: refactored
{
	if (gDLL->GetWorldBuilderMode())
		return GC.getPythonCaller()->WBGetHighlightPlot();
	if (GC.getInterfaceModeInfo(gDLL->getInterfaceIFace()->getInterfaceMode()).getHighlightPlot())
		return gDLL->getInterfaceIFace()->getMouseOverPlot();
	return NULL;
}


ColorTypes CvGame::getPlotHighlightColor(CvPlot* pPlot) const  // advc: refactored
{
	if (pPlot == NULL)
		return NO_COLOR;

	ColorTypes eDefaultColor = (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN");
	if (gDLL->GetWorldBuilderMode())
		return eDefaultColor;

	switch (gDLL->getInterfaceIFace()->getInterfaceMode())
	{
	case INTERFACEMODE_PING:
	case INTERFACEMODE_SIGN:
		if (!pPlot->isRevealed(getActiveTeam(), true))
			return NO_COLOR;
	case INTERFACEMODE_PYTHON_PICK_PLOT:
		if (!pPlot->isRevealed(getActiveTeam(), true) ||
				!GC.getPythonCaller()->canPickRevealedPlot(*pPlot))
			return NO_COLOR;
	case INTERFACEMODE_SAVE_PLOT_NIFS:
		return (ColorTypes)GC.getInfoTypeForString("COLOR_DARK_GREY");
	}
	if (!gDLL->getInterfaceIFace()->getSelectionList()->canDoInterfaceModeAt(
			gDLL->getInterfaceIFace()->getInterfaceMode(), pPlot))
		return (ColorTypes)GC.getInfoTypeForString("COLOR_DARK_GREY");
	return eDefaultColor;
}

void CvGame::loadBuildQueue(const CvString& strItem) const
{
	// advc.003w: The first two loops were needlessly complicated
	CvCivilization const& kCiv = *getActiveCivilization();
	for (int i = 0; i < kCiv.getNumUnits(); i++)
	{
		UnitTypes eUnit = kCiv.unitAt(i);
		if (GC.getUnitInfo(eUnit).getType() == strItem)
		{
			selectedCitiesGameNetMessage(GAMEMESSAGE_PUSH_ORDER, ORDER_TRAIN,
					eUnit, -1, false, false, true);
			return;
		}
	}
	for (int i = 0; i < kCiv.getNumBuildings(); i++)
	{
		BuildingTypes eBuilding = kCiv.buildingAt(i);
		if (GC.getBuildingInfo(eBuilding).getType() == strItem)
		{
			selectedCitiesGameNetMessage(GAMEMESSAGE_PUSH_ORDER, ORDER_CONSTRUCT,
					eBuilding, -1, false, false, true);
			return;
		}
	}
	for (int iI = 0; iI < GC.getNumProjectInfos(); iI++)
	{
		if (strItem == GC.getProjectInfo((ProjectTypes)iI).getType())
		{
			selectedCitiesGameNetMessage(GAMEMESSAGE_PUSH_ORDER, ORDER_CREATE,
					(ProjectTypes)iI, -1, false, false, true);
			return;
		}
	}

	for (int iI = 0; iI < GC.getNumProcessInfos(); iI++)
	{
		if (strItem == GC.getProcessInfo((ProcessTypes)iI).getType())
		{
			selectedCitiesGameNetMessage(GAMEMESSAGE_PUSH_ORDER, ORDER_MAINTAIN,
					(ProcessTypes)iI, -1, false, false, true);
			return;
		}
	}
}

void CvGame::cheatSpaceship() const
{	// <advc.007b> I don't know how this is triggered; it's safer to block it.
	if(!isDebugMode())
		return; // </advc.007b>
	//add one space project that is still available
	CvTeam& kTeam = GET_TEAM(getActiveTeam());
	for (int i = 0; i < GC.getNumProjectInfos(); i++)
	{
		ProjectTypes eProject = (ProjectTypes) i;
		CvProjectInfo& kProject = GC.getProjectInfo(eProject);
		if (kProject.isSpaceship())
		{
			//cheat required projects
			for (int j = 0; j < GC.getNumProjectInfos(); j++)
			{
				ProjectTypes eRequiredProject = (ProjectTypes) j;
				int iNumReqProjects = kProject.getProjectsNeeded(eRequiredProject);
				while (kTeam.getProjectCount(eRequiredProject) < iNumReqProjects)
				{
					kTeam.changeProjectCount(eRequiredProject, 1);
				}
			}

			//cheat required techs
			TechTypes eRequiredTech = (TechTypes) kProject.getTechPrereq();
			if (!kTeam.isHasTech(eRequiredTech))
			{
				kTeam.setHasTech(eRequiredTech, true, getActivePlayer(), true, true);
			}

			//cheat one space component
			if (kTeam.getProjectCount(eProject) < kProject.getMaxTeamInstances())
			{
				kTeam.changeProjectCount(eProject, 1);

				CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN, eProject);
				pInfo->setText(L"showSpaceShip");
				gDLL->getInterfaceIFace()->addPopup(pInfo, getActivePlayer());
			}
		}
	}
}

DomainTypes CvGame::getUnitDomain(UnitTypes eUnit) const
{
	return (DomainTypes) GC.getUnitInfo(eUnit).getDomainType();
}

const CvArtInfoBuilding* CvGame::getBuildingArtInfo(BuildingTypes eBuilding) const
{
	return GC.getBuildingInfo(eBuilding).getArtInfo();
}

bool CvGame::isWaterBuilding(BuildingTypes eBuilding) const
{
	return GC.getBuildingInfo(eBuilding).isWater();
}

CivilopediaWidgetShowTypes CvGame::getWidgetShow(BonusTypes eBonus) const
{
	// Don't show the widget if it's an abstract bonus that doesn't have any terrain (or art) associated with it.
	// Hit singles, movies, musicals, in our case
	bool bShowWidget = false;

	for (int i = 0; i < GC.getNumTerrainInfos(); i++)
	{
		if (GC.getBonusInfo(eBonus).isTerrain(i))
		{
			bShowWidget = true;
			break;
		}
	}

	if (!bShowWidget)
	{
		for (int i = 0; i < GC.getNumFeatureInfos(); i++)
		{
			if (GC.getBonusInfo(eBonus).isFeature(i))
			{
				bShowWidget = true;
				break;
			}
		}
	}

	if (!bShowWidget)
	{
		return CIVILOPEDIA_WIDGET_SHOW_NONE;
	}

	CivilopediaWidgetShowTypes eType = CIVILOPEDIA_WIDGET_SHOW_LAND;
	for (int i = 0; i < GC.getNumTerrainInfos(); i++)
	{
		if (GC.getTerrainInfo((TerrainTypes) i).isWater())
		{
			if (GC.getBonusInfo(eBonus).isTerrain(i))
			{
				eType = CIVILOPEDIA_WIDGET_SHOW_WATER;
			}
		}
	}

	return eType;
}

CivilopediaWidgetShowTypes CvGame::getWidgetShow(ImprovementTypes eImprovement) const
{
	CivilopediaWidgetShowTypes eType = CIVILOPEDIA_WIDGET_SHOW_LAND;
	if (GC.getImprovementInfo(eImprovement).isWater())
	{
		eType = CIVILOPEDIA_WIDGET_SHOW_WATER;
	}
	return eType;
}

VictoryTypes CvGame::getSpaceVictory() const
{
	VictoryTypes eVictory = NO_VICTORY;
	for (int i=0; i < GC.getNumProjectInfos(); i++)
	{
		ProjectTypes eProject = (ProjectTypes) i;
		if (GC.getProjectInfo(eProject).isSpaceship())
		{
			eVictory = (VictoryTypes) GC.getProjectInfo(eProject).getVictoryPrereq();
			break;
		}
	}

	FAssertMsg(eVictory != NO_VICTORY, "Invalid space victory type.");
	return eVictory;
}

void CvGame::nextActivePlayer(bool bForward)
{
	int iNewPlayer = getActivePlayer();
	for (int i = 1; i < MAX_PLAYERS; ++i)
	{
		if (bForward)
		{
			iNewPlayer += 1;
		}
		else
		{
			iNewPlayer += MAX_PLAYERS - 1;
		}
		iNewPlayer %= MAX_PLAYERS;

		PlayerTypes eNewPlayer = (PlayerTypes) iNewPlayer;
		if (GET_PLAYER(eNewPlayer).isAlive() && !(GET_PLAYER(eNewPlayer).isBarbarian()))
		{
			GC.getInitCore().setSlotStatus(getActivePlayer(), SS_COMPUTER);
			GC.getInitCore().setSlotStatus(eNewPlayer, SS_TAKEN);
			GET_PLAYER(getActivePlayer()).setTurnActive(false, false);
			GET_PLAYER(eNewPlayer).setTurnActive(true, false);
			setActivePlayer(eNewPlayer, true);
			break;
		}
	}
}


int CvGame::getNextSoundtrack(EraTypes eLastEra, int iLastSoundtrack) const
{
	EraTypes eCurEra = GET_PLAYER(getActivePlayer()).getCurrentEra();
	CvEraInfo& kCurrentEra = GC.getEraInfo(eCurEra);
	if (kCurrentEra.getNumSoundtracks() == 0)
	{
		return -1;
	}
	else if (kCurrentEra.getNumSoundtracks() == 1 || (eLastEra != eCurEra && kCurrentEra.isFirstSoundtrackFirst()))
	{
		return kCurrentEra.getSoundtracks(0);
	}
	else
	{
		return kCurrentEra.getSoundtracks(getASyncRand().get(kCurrentEra.getNumSoundtracks(), "Pick Song ASYNC"));
	}
}

int CvGame::getSoundtrackSpace() const
{
	return std::max(1, GC.getEraInfo(GET_PLAYER(getActivePlayer()).getCurrentEra()).getSoundtrackSpace());
}

bool CvGame::isSoundtrackOverride(CvString& strSoundtrack) const
{
	if (GC.getDefineINT("VICTORY_SOUNDTRACK_AVAILABLE") != 0)
	{
		if (getGameState() == GAMESTATE_EXTENDED || getGameState() == GAMESTATE_OVER)
		{
			if (getWinner() == getActiveTeam())
			{
				strSoundtrack = "AS2D_VICTORY";
			}
			else
			{
				strSoundtrack = "AS2D_DEFEAT";
			}
			return true;
		}
	}
	return false;
}

void CvGame::initSelection() const
{
	bool bSelected = false;
	CvPlayer const& kActivePlayer = GET_PLAYER(getActivePlayer());
	FOR_EACH_UNIT_VAR(pLoopUnit, kActivePlayer)
	{
		if (pLoopUnit->getGroup()->readyToSelect())
		{
			if (pLoopUnit->canFight())
			{
				selectUnit(pLoopUnit, true);
				bSelected = true;
				break;
			}
		}
	}

	if (!bSelected)
	{
		FOR_EACH_UNIT_VAR(pLoopUnit, kActivePlayer)
		{
			if (pLoopUnit->getGroup()->readyToSelect())
			{
				selectUnit(pLoopUnit, true);
				bSelected = true;
				break;
			}
		}
	}

	if (!bSelected)
	{
		FOR_EACH_UNIT_VAR(pLoopUnit, kActivePlayer)
		{
			gDLL->getInterfaceIFace()->centerCamera(pLoopUnit);
			break;
		}
	}
}

bool CvGame::canDoPing(CvPlot* pPlot, PlayerTypes ePlayer) const
{
	if (pPlot == NULL || !pPlot->isRevealed(getActiveTeam(), false))
	{
		return false;
	}

	if (GET_PLAYER(ePlayer).getTeam() != getActiveTeam())
	{
		return false;
	}

	return true;
}

bool CvGame::shouldDisplayReturn() const
{
	return gDLL->getInterfaceIFace()->isCitySelection();
}

bool CvGame::shouldDisplayEndTurn() const
{
	return (!gDLL->getInterfaceIFace()->isCitySelection() && GET_PLAYER(getActivePlayer()).isTurnActive());
}

bool CvGame::shouldDisplayWaitingOthers() const
{	// <advc.706>
	if(CvPlot::isAllFog())
		return false; // </advc.706>
	if (!gDLL->getInterfaceIFace()->isCitySelection())
	{
		if (!GET_PLAYER(getActivePlayer()).isTurnActive())
		{
			return true;
		}

		if (gDLL->getInterfaceIFace()->isInAdvancedStart() && GET_PLAYER(getActivePlayer()).getAdvancedStartPoints() < 0)
		{
			return true;
		}
	}
	return false;
}

bool CvGame::shouldDisplayWaitingYou() const
{
	if (!gDLL->getInterfaceIFace()->isCitySelection())
	{
		if (GET_PLAYER(getActivePlayer()).isTurnActive())
		{
			if (isNetworkMultiPlayer())
			{
				if (isMPOption(MPOPTION_SIMULTANEOUS_TURNS) && 1 == countNumHumanGameTurnActive())
				{
					return true;
				}

				if (isSimultaneousTeamTurns() && 1 == GET_TEAM(getActiveTeam()).countNumHumanGameTurnActive() && GET_TEAM(getActiveTeam()).getAliveCount() > 1)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool CvGame::shouldDisplayEndTurnButton() const
{
	if (!gDLL->getInterfaceIFace()->isCitySelection())
	{
		if (!gDLL->GetWorldBuilderMode())
		{
			if (GET_PLAYER(getActivePlayer()).isTurnActive())
			{
				return true;
			}
		}
	}

	return false;
}

bool CvGame::shouldDisplayFlag() const
{
	if (gDLL->getInterfaceIFace()->isCitySelection() ||
		gDLL->getInterfaceIFace()->getHeadSelectedCity() != NULL ||
		gDLL->isDiplomacy() ||
		gDLL->isMPDiplomacyScreenUp() ||
		gDLL->GetWorldBuilderMode())
	{
		return false;
	}

	return true;
}

bool CvGame::shouldDisplayUnitModel() const
{
	if (gDLL->isDiplomacy() ||
		gDLL->isMPDiplomacyScreenUp() ||
		gDLL->GetWorldBuilderMode())
	{
		return false;
	}

	if (gDLL->getInterfaceIFace()->getHeadSelectedUnit() != NULL || gDLL->getInterfaceIFace()->isCityScreenUp())
	{
		return true;
	}

	return false;
}

bool CvGame::shouldShowResearchButtons() const
{
	if (!gDLL->GetWorldBuilderMode())
	{
		CvPlayer const& kActivePlayer = GET_PLAYER(getActivePlayer()); // advc
		if(kActivePlayer.isAlive() && !gDLL->getInterfaceIFace()->isCityScreenUp()) {
			if(kActivePlayer.isResearch() && // advc.004x
					kActivePlayer.getCurrentResearch() == NO_TECH)
				return true;
		}
	}

	return false;
}

bool CvGame::shouldCenterMinimap() const
{
	return (isDebugMode() || GET_TEAM(getActiveTeam()).isMapCentering());
}

EndTurnButtonStates CvGame::getEndTurnState() const
{
	EndTurnButtonStates eNewState = END_TURN_GO;

	/* original bts code
	if ((isNetworkMultiPlayer() &&
		(isMPOption(MPOPTION_SIMULTANEOUS_TURNS) && 1 == countNumHumanGameTurnActive() ||
		(!isSimultaneousTeamTurns() && 1 == GET_TEAM(getActiveTeam()).countNumHumanGameTurnActive() && GET_TEAM(getActiveTeam()).getAliveCount() > 1)))) */
	// K-Mod. Don't use GET_TEAM in pitboss mode. (and note, I've fixed a typo in the parentheses.)
	if (isNetworkMultiPlayer() && getActiveTeam() != NO_TEAM &&
		((isMPOption(MPOPTION_SIMULTANEOUS_TURNS) && 1 == countNumHumanGameTurnActive()) ||
		(!isSimultaneousTeamTurns() && 1 == GET_TEAM(getActiveTeam()).countNumHumanGameTurnActive() && GET_TEAM(getActiveTeam()).getAliveCount() > 1)))
	// K-Mod end
	{
		eNewState = END_TURN_OVER_HIGHLIGHT;
	}
	else
	{
		eNewState = END_TURN_GO;
	}

	return eNewState;
}

void CvGame::handleCityScreenPlotPicked(CvCity* pCity, CvPlot* pPlot,
		bool bAlt, bool bShift, bool bCtrl) const
{
	FAssert(pPlot != NULL);
	if (pCity != NULL && pPlot != NULL)
	{
		int iIndex = pCity->getCityPlotIndex(pPlot);
		if (pPlot->getOwner() == getActivePlayer() &&
				pCity->getOwner() == getActivePlayer() && iIndex != -1)
		{
			CvMessageControl::getInstance().sendDoTask(pCity->getID(),
					TASK_CHANGE_WORKING_PLOT, iIndex, -1, false, bAlt, bShift, bCtrl);
		}
		else if (GC.getDefineINT("CITY_SCREEN_CLICK_WILL_EXIT"))
		{
			gDLL->getInterfaceIFace()->clearSelectedCities();
		}
	}
}

void CvGame::handleCityScreenPlotDoublePicked(CvCity* pCity, CvPlot* pPlot, bool bAlt, bool bShift, bool bCtrl) const
{	// advc.004t: Commented out
	/*if (pCity != NULL) {
		if (pCity->plot() == pPlot)
			gDLL->getInterfaceIFace()->clearSelectedCities();
	}*/
}

void CvGame::handleCityScreenPlotRightPicked(CvCity* pCity, CvPlot* pPlot, bool bAlt, bool bShift, bool bCtrl) const
{
	if (pCity != NULL && pPlot != NULL)
	{	/*  <advc.004t> Can't assign a working city to the city plot, so use this
			for exiting the screen. */
		if(pCity->plot() == pPlot) {
			CvPlot const* pCityPlot = (gDLL->getInterfaceIFace()->isCityScreenUp() ?
					gDLL->getInterfaceIFace()->getHeadSelectedCity()->plot() : NULL);
			gDLL->getInterfaceIFace()->clearSelectedCities();
			if(pCityPlot != NULL)
				gDLL->getInterfaceIFace()->lookAt(pCityPlot->getPoint(), CAMERALOOKAT_NORMAL);
			return;
		} // </advc.004t>
		if (pCity->getOwner() == getActivePlayer() &&
				pPlot->getOwner() == getActivePlayer() &&
				pCity->getCityPlotIndex(pPlot) != -1)
		{
			CvMessageControl::getInstance().sendDoTask(pCity->getID(),
					TASK_CLEAR_WORKING_OVERRIDE, pCity->getCityPlotIndex(pPlot),
					-1, false, bAlt, bShift, bCtrl);
		}
	}
}

void CvGame::handleCityPlotRightPicked(CvCity* pCity, CvPlot* pPlot, bool bAlt, bool bShift, bool bCtrl) const
{
	if (pPlot != NULL)
	{
		if (pCity != NULL && gDLL->getInterfaceIFace()->isCitySelected(pCity))
		{
			selectedCitiesGameNetMessage(GAMEMESSAGE_DO_TASK, TASK_CLEAR_RALLY_PLOT);
		}
		else
		{
			if (bShift)
			{
				selectedCitiesGameNetMessage(GAMEMESSAGE_DO_TASK, TASK_RALLY_PLOT, pPlot->getX(), pPlot->getY());
			}
			else
			{
				gDLL->getInterfaceIFace()->clearSelectedCities();
			}
		}
	}
}

void CvGame::handleMiddleMouse(bool bCtrl, bool bAlt, bool bShift)
{
	if (gDLL->getInterfaceIFace()->isCitySelection())
	{
		gDLL->getInterfaceIFace()->clearSelectedCities();
	}
	else
	{
		if (bAlt)
		{
			doControl(CONTROL_SELECTYUNITALL);
		}
		else if (bCtrl)
		{
			doControl(CONTROL_SELECTYUNITTYPE);
		}
		else
		{
			doControl(CONTROL_CENTERONSELECTION);
		}
	}
}

void CvGame::handleDiplomacySetAIComment(DiploCommentTypes eComment) const
{
	PlayerTypes eOtherPlayer = (PlayerTypes) gDLL->getDiplomacyPlayer();
	FAssert(eOtherPlayer != NO_PLAYER);
	if (GC.getInfoTypeForString("AI_DIPLOCOMMENT_ACCEPT_ASK") == eComment ||
		GC.getInfoTypeForString("AI_DIPLOCOMMENT_ACCEPT_DEMAND") == eComment)
	{
		if (!GET_TEAM(getActiveTeam()).isAVassal() && !GET_TEAM(eOtherPlayer).isAVassal())
		{
			CLinkList<TradeData> playerList;
			CLinkList<TradeData> loopPlayerList;
			TradeData kTradeData;
			setTradeItem(&kTradeData, TRADE_PEACE_TREATY);
			playerList.insertAtEnd(kTradeData);
			loopPlayerList.insertAtEnd(kTradeData);

			gDLL->sendImplementDealMessage(eOtherPlayer, &playerList, &loopPlayerList);
		}
	}
	// <advc.072>
	const_cast<CvGame* const>(this)->m_bShowingCurrentDeals =
			(eComment == GC.getInfoTypeForString("AI_DIPLOCOMMENT_CURRENT_DEALS"));
	// </advc.072>
}
