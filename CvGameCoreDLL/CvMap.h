#pragma once

#ifndef CIV4_MAP_H
#define CIV4_MAP_H

//
//	FILE:	 CvMap.h
//	AUTHOR:  Soren Johnson
//	PURPOSE: Game map class
//-----------------------------------------------------------------------------
//	Copyright (c) 2004 Firaxis Games, Inc. All rights reserved.
//-----------------------------------------------------------------------------
//

#include "CvArea.h"
#include "CvPlot.h"
// <advc.300>
#include "Shelf.h"
#include <map>
// </advc.300>

class FAStar;
class CvPlotGroup;
class CvSelectionGroup;

//
// holds initialization info
//
struct CvMapInitData
{
	int m_iGridW;						// in game plots
	int m_iGridH;						// in game plots
	int m_iTopLatitude;
	int m_iBottomLatitude;

	bool m_bWrapX;
	bool m_bWrapY;

	CvMapInitData(int iGridW=0, int iGridH=0, int iTopLatitude=90, int iBottomLatitude=-90, bool bWrapX=false, bool bWrapY=false) :
		m_iGridH(iGridH),m_iGridW(iGridW),m_iTopLatitude(iTopLatitude),m_iBottomLatitude(iBottomLatitude),m_bWrapY(bWrapY),m_bWrapX(bWrapX)
	{ }
};

//
// CvMap
//
class CvMap
		: private boost::noncopyable // advc.003e
{
	/*  <advc.make> All the inline functions below used to be global functions
		in CvGameCoreUtils.h except for coordRange, which was already in CvMap.h,
		but as a global function. I've made some minor style changes.
		Since I don't want to change all the call locations, I'm adding
		global (inline) wrappers at the end of this file. */
public:
	// 4 | 4 | 3 | 3 | 3 | 4 | 4
	// -------------------------
	// 4 | 3 | 2 | 2 | 2 | 3 | 4
	// -------------------------
	// 3 | 2 | 1 | 1 | 1 | 2 | 3
	// -------------------------
	// 3 | 2 | 1 | 0 | 1 | 2 | 3
	// -------------------------
	// 3 | 2 | 1 | 1 | 1 | 2 | 3
	// -------------------------
	// 4 | 3 | 2 | 2 | 2 | 3 | 4
	// -------------------------
	// 4 | 4 | 3 | 3 | 3 | 4 | 4
	//
	// Returns the distance between plots according to the pattern above...
	inline int plotDistance(int iX1, int iY1, int iX2, int iY2) const
	{
		int iDX = xDistance(iX1, iX2);
		int iDY = yDistance(iY1, iY2);
		return std::max(iDX, iDY) + (std::min(iDX, iDY) / 2);
	}

	// K-Mod, plot-to-plot alias for convenience:
	inline int plotDistance(const CvPlot* plot1, const CvPlot* plot2) const
	{
		return plotDistance(
				plot1->getX_INLINE(), plot1->getY_INLINE(),
				plot2->getX_INLINE(), plot2->getY_INLINE());
	}
	// K-Mod end

	// 3 | 3 | 3 | 3 | 3 | 3 | 3
	// -------------------------
	// 3 | 2 | 2 | 2 | 2 | 2 | 3
	// -------------------------
	// 3 | 2 | 1 | 1 | 1 | 2 | 3
	// -------------------------
	// 3 | 2 | 1 | 0 | 1 | 2 | 3
	// -------------------------
	// 3 | 2 | 1 | 1 | 1 | 2 | 3
	// -------------------------
	// 3 | 2 | 2 | 2 | 2 | 2 | 3
	// -------------------------
	// 3 | 3 | 3 | 3 | 3 | 3 | 3
	//
	// Returns the distance between plots according to the pattern above...
	inline int stepDistance(int iX1, int iY1, int iX2, int iY2) const
	{
		return std::max(xDistance(iX1, iX2), yDistance(iY1, iY2));
	}

	// K-Mod, plot-to-plot alias for convenience:
	inline int stepDistance(const CvPlot* plot1, const CvPlot* plot2) const
	{
		return stepDistance(
				plot1->getX_INLINE(), plot1->getY_INLINE(),
				plot2->getX_INLINE(), plot2->getY_INLINE());
	} // K-Mod end

	inline CvPlot* plotDirection(int iX, int iY, DirectionTypes eDirection) const
	{
		if(eDirection == NO_DIRECTION)
			return plotINLINE(iX, iY);
		else return plotINLINE(
				iX + GC.getPlotDirectionX()[eDirection],
				iY + GC.getPlotDirectionY()[eDirection]);
	}

	inline CvPlot* plotCardinalDirection(int iX, int iY, CardinalDirectionTypes eCardinalDirection) const
	{
		return plotINLINE(
			iX + GC.getPlotCardinalDirectionX()[eCardinalDirection],
			iY + GC.getPlotCardinalDirectionY()[eCardinalDirection]);
	}

	inline CvPlot* plotXY(int iX, int iY, int iDX, int iDY) const
	{
		return plotINLINE(iX + iDX, iY + iDY);
	}
	// K-Mod start
	inline CvPlot* plotXY(const CvPlot* pPlot, int iDX, int iDY) const
	{
		return plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);
	} // K-Mod end 

	inline DirectionTypes directionXY(int iDX, int iDY) const
	{
		if (abs(iDX) > DIRECTION_RADIUS || abs(iDY) > DIRECTION_RADIUS)
			return NO_DIRECTION;
		else return GC.getXYDirection(iDX + DIRECTION_RADIUS, iDY + DIRECTION_RADIUS);
	}

	inline DirectionTypes directionXY(const CvPlot* pFromPlot, const CvPlot* pToPlot) const
	{
		return directionXY(
				dxWrap(pToPlot->getX_INLINE() - pFromPlot->getX_INLINE()),
				dyWrap(pToPlot->getY_INLINE() - pFromPlot->getY_INLINE()));
	}

	inline int dxWrap(int iDX) const
	{
		return wrapCoordDifference(iDX, getGridWidthINLINE(), isWrapXINLINE());
	}

	inline int dyWrap(int iDY) const
	{
		return wrapCoordDifference(iDY, getGridHeightINLINE(), isWrapYINLINE());
	}

	inline int xDistance(int iFromX, int iToX) const
	{
		return coordDistance(iFromX, iToX, getGridWidthINLINE(), isWrapXINLINE());
	}

	inline int yDistance(int iFromY, int iToY) const
	{
		return coordDistance(iFromY, iToY, getGridHeightINLINE(), isWrapYINLINE());
	}
private: // Auxiliary functions
	inline int coordDistance(int iFrom, int iTo, int iRange, bool bWrap) const
	{
		if (bWrap && abs(iFrom - iTo) > iRange / 2)
			return iRange - abs(iFrom - iTo);
		return abs(iFrom - iTo);
	}

	inline int wrapCoordDifference(int iDiff, int iRange, bool bWrap) const
	{
		if (!bWrap)
			return iDiff;
		if (iDiff > iRange / 2)
			return iDiff - iRange;
		else if (iDiff < -(iRange / 2))
			return iDiff + iRange;
		return iDiff;
	}

	inline int coordRange(int iCoord, int iRange, bool bWrap) const
	{
		if (!bWrap || iRange == 0)
			return iCoord;
		if (iCoord < 0 )
			return (iRange + (iCoord % iRange));
		else if (iCoord >= iRange)
			return (iCoord % iRange);
		return iCoord;
	}
	// </advc.make>

	friend class CyMap;
public:

	CvMap();
	virtual ~CvMap();

	DllExport void init(CvMapInitData* pInitData=NULL);
	DllExport void setupGraphical();
	DllExport void reset(CvMapInitData* pInitData);

protected:

	void uninit();
	void setup();

public: // advc.003: made several functions const

	DllExport void erasePlots();																			// Exposed to Python
	void setRevealedPlots(TeamTypes eTeam, bool bNewValue, bool bTerrainOnly = false);		// Exposed to Python
	void setAllPlotTypes(PlotTypes ePlotType);												// Exposed to Python

	void doTurn();

	void setFlagsDirty(); // K-Mod
	DllExport void updateFlagSymbols();

	DllExport void updateFog();
	void updateVisibility();																// Exposed to Python
	DllExport void updateSymbolVisibility();
	void updateSymbols();
	DllExport void updateMinimapColor();															// Exposed to Python
	void updateSight(bool bIncrement);
	void updateIrrigated();
	DllExport void updateCenterUnit();
	void updateWorkingCity();
	void updateMinOriginalStartDist(CvArea* pArea);										// Exposed to Python
	void updateYield();

	void verifyUnitValidPlot();

	void combinePlotGroups(PlayerTypes ePlayer, CvPlotGroup* pPlotGroup1, CvPlotGroup* pPlotGroup2);	

	CvPlot* syncRandPlot(int iFlags = 0, int iArea = -1, int iMinUnitDistance = -1, int iTimeout = 100, // Exposed to Python 
			int* piLegal = NULL); // advc.304

	DllExport CvCity* findCity(int iX, int iY, PlayerTypes eOwner = NO_PLAYER, TeamTypes eTeam = NO_TEAM, bool bSameArea = true, bool bCoastalOnly = false, TeamTypes eTeamAtWarWith = NO_TEAM, DirectionTypes eDirection = NO_DIRECTION, CvCity* pSkipCity = NULL) {	// Exposed to Python
		// <advc.004r>
		return findCity(iX, iY, eOwner, eTeam, bSameArea, bCoastalOnly,
				eTeamAtWarWith, eDirection, pSkipCity, NO_TEAM);
	}
	CvCity* findCity(int iX, int iY, PlayerTypes eOwner = NO_PLAYER, TeamTypes eTeam = NO_TEAM,
			bool bSameArea = true, bool bCoastalOnly = false, TeamTypes eTeamAtWarWith = NO_TEAM,
			DirectionTypes eDirection = NO_DIRECTION, CvCity* pSkipCity = NULL, TeamTypes eObserver = NO_TEAM) const;
	// </advc.004r>
	CvSelectionGroup* findSelectionGroup(int iX, int iY, PlayerTypes eOwner = NO_PLAYER, bool bReadyToSelect = false, bool bWorkers = false) const;				// Exposed to Python

	CvArea* findBiggestArea(bool bWater);																						// Exposed to Python

	int getMapFractalFlags() const;																												// Exposed to Python
	bool findWater(CvPlot* pPlot, int iRange, bool bFreshWater);										// Exposed to Python

	DllExport bool isPlot(int iX, int iY) const;																		// Exposed to Python
#ifdef _USRDLL
	inline int isPlotINLINE(int iX, int iY) const
	{
		return (iX >= 0 && iX < getGridWidthINLINE() && iY >= 0 && iY < getGridHeightINLINE());
	}
#endif
	DllExport int numPlots() const; 																								// Exposed to Python
#ifdef _USRDLL
	inline int numPlotsINLINE() const
	{
		return getGridWidthINLINE() * getGridHeightINLINE();
	}
#endif
	int plotNum(int iX, int iY) const;																		// Exposed to Python
#ifdef _USRDLL
	inline int plotNumINLINE(int iX, int iY) const
	{
		return ((iY * getGridWidthINLINE()) + iX);
	}
#endif
	int plotX(int iIndex) const;																										// Exposed to Python
	int plotY(int iIndex) const;																										// Exposed to Python

	int pointXToPlotX(float fX);
	DllExport float plotXToPointX(int iX);

	int pointYToPlotY(float fY);
	DllExport float plotYToPointY(int iY);
	
	float getWidthCoords() const;
	float getHeightCoords() const;

	int maxPlotDistance() const;																								// Exposed to Python
	int maxStepDistance() const;																								// Exposed to Python
	int maxMaintenanceDistance() const; // advc.140

	DllExport int getGridWidth() const;																		// Exposed to Python
#ifdef _USRDLL
	inline int getGridWidthINLINE() const
	{
		return m_iGridWidth;
	}
#endif
	DllExport int getGridHeight() const;																	// Exposed to Python
#ifdef _USRDLL
	inline int getGridHeightINLINE() const
	{
		return m_iGridHeight;
	}
#endif
	int getLandPlots() const;																					// Exposed to Python
	void changeLandPlots(int iChange);

	int getOwnedPlots() const;																				// Exposed to Python
	void changeOwnedPlots(int iChange);

	int getTopLatitude() const;																									// Exposed to Python
	int getBottomLatitude() const;																							// Exposed to Python

	int getNextRiverID() const;																									// Exposed to Python
	void incrementNextRiverID();																					// Exposed to Python

	DllExport bool isWrapX();																							// Exposed to Python
#ifdef _USRDLL
	inline bool isWrapXINLINE() const
	{
		return m_bWrapX;
	}
#endif
	DllExport bool isWrapY();																							// Exposed to Python
#ifdef _USRDLL
	inline bool isWrapYINLINE() const
	{
		return m_bWrapY;
	}
#endif
	DllExport bool isWrap();
#ifdef _USRDLL
	inline bool isWrapINLINE() const
	{
		return m_bWrapX || m_bWrapY;
	}
#endif
	DllExport WorldSizeTypes getWorldSize() {															// Exposed to Python
		// <advc.003> const replacement
			return worldSize();
	}
	WorldSizeTypes worldSize() const; // </advc.003>
	ClimateTypes getClimate() const;																	// Exposed to Python
	SeaLevelTypes getSeaLevel() const;																// Exposed to Python

	int getNumCustomMapOptions() const;
	CustomMapOptionTypes getCustomMapOption(int iOption);											// Exposed to Python

	int getNumBonuses(BonusTypes eIndex) const;																	// Exposed to Python
	void changeNumBonuses(BonusTypes eIndex, int iChange);

	int getNumBonusesOnLand(BonusTypes eIndex) const;														// Exposed to Python
	void changeNumBonusesOnLand(BonusTypes eIndex, int iChange);

	DllExport CvPlot* plotByIndex(int iIndex) const;											// Exposed to Python
#ifdef _USRDLL
	inline CvPlot* plotByIndexINLINE(int iIndex) const
	{
		return ((iIndex >= 0 && iIndex < getGridWidthINLINE() * getGridHeightINLINE()) ?
				&(m_pMapPlots[iIndex]) : NULL);
	}
#endif
	DllExport CvPlot* plot(int iX, int iY) const;													// Exposed to Python
#ifdef _USRDLL
	__forceinline CvPlot* plotINLINE(int iX, int iY) const
	{
		if (iX == INVALID_PLOT_COORD || iY == INVALID_PLOT_COORD)
		{
			return NULL;
		}
		int iMapX = coordRange(iX, getGridWidthINLINE(), isWrapXINLINE());
		int iMapY = coordRange(iY, getGridHeightINLINE(), isWrapYINLINE());
		return (isPlotINLINE(iMapX, iMapY) ? &(m_pMapPlots[plotNumINLINE(iMapX, iMapY)]) : NULL);
	}
	__forceinline CvPlot* plotSorenINLINE(int iX, int iY) const
	{
		if (iX == INVALID_PLOT_COORD || iY == INVALID_PLOT_COORD)
		{
			return NULL;
		}
		return &(m_pMapPlots[plotNumINLINE(iX, iY)]);
	}
#endif
	DllExport CvPlot* pointToPlot(float fX, float fY);

	int getIndexAfterLastArea() const;														// Exposed to Python
	int getNumAreas() const;																	// Exposed to Python
	int getNumLandAreas() const;
	CvArea* getArea(int iID) const;																// Exposed to Python
	CvArea* addArea();
	void deleteArea(int iID);
	// iteration
	CvArea* firstArea(int *pIterIdx, bool bRev=false) const;									// Exposed to Python
	CvArea* nextArea(int *pIterIdx, bool bRev=false) const;									// Exposed to Python

	void recalculateAreas();																		// Exposed to Python
	// <advc.300>
	void computeShelves();
	void getShelves(int iArea, std::vector<Shelf*>& r) const;
	// </advc.300>
	void resetPathDistance();																		// Exposed to Python
	int calculatePathDistance(CvPlot const* pSource, CvPlot const* pDest) const;					// Exposed to Python

	// BETTER_BTS_AI_MOD, Efficiency (plot danger cache), 08/21/09, jdog5000: START
	//void invalidateIsActivePlayerNoDangerCache();
	void invalidateActivePlayerSafeRangeCache(); // K-Mod version
	void invalidateBorderDangerCache(TeamTypes eTeam);
	// BETTER_BTS_AI_MOD: END

	// Serialization:
	virtual void read(FDataStreamBase* pStream);
	virtual void write(FDataStreamBase* pStream);

	void rebuild(int iGridW, int iGridH, int iTopLatitude, int iBottomLatitude, bool bWrapX, bool bWrapY, WorldSizeTypes eWorldSize, ClimateTypes eClimate, SeaLevelTypes eSeaLevel, int iNumCustomMapOptions, CustomMapOptionTypes * eCustomMapOptions);		// Exposed to Python

protected:

	int m_iGridWidth;
	int m_iGridHeight;
	int m_iLandPlots;
	int m_iOwnedPlots;
	int m_iTopLatitude;
	int m_iBottomLatitude;
	int m_iNextRiverID;

	bool m_bWrapX;
	bool m_bWrapY;

	int* m_paiNumBonus;
	int* m_paiNumBonusOnLand;

	CvPlot* m_pMapPlots;
	std::map<Shelf::Id,Shelf*> shelves; // advc.300

	FFreeListTrashArray<CvArea> m_areas;
	void calculateAreas();
	// <advc.030>
	void calculateAreas_030();
	void calculateReprAreas();
	void calculateAreas_DFS(CvPlot const& p);
	void updateLakes();
	// </advc.030>
};

/* <advc.make> Global wrappers for distance functions. The int versions are
	exposed to Python */
inline int plotDistance(int iX1, int iY1, int iX2, int iY2) {
	return GC.getMapINLINE().plotDistance(iX1, iY1, iX2, iY2);
}
inline int plotDistance(const CvPlot* plot1, const CvPlot* plot2) {
	return GC.getMapINLINE().plotDistance(plot1, plot2);
}
inline int stepDistance(int iX1, int iY1, int iX2, int iY2) {
	return GC.getMapINLINE().stepDistance(iX1, iY1, iX2, iY2);
}
inline int stepDistance(const CvPlot* plot1, const CvPlot* plot2) {
	return GC.getMapINLINE().stepDistance(plot1, plot2);
}
inline CvPlot* plotDirection(int iX, int iY, DirectionTypes eDirection) {
	return GC.getMapINLINE().plotDirection(iX, iY, eDirection);
}
inline CvPlot* plotCardinalDirection(int iX, int iY, CardinalDirectionTypes eCardinalDirection)	{
	return GC.getMapINLINE().plotCardinalDirection(iX, iY, eCardinalDirection);
}
inline CvPlot* plotXY(int iX, int iY, int iDX, int iDY)	{
	return GC.getMapINLINE().plotXY(iX, iY, iDX, iDY);
}
inline CvPlot* plotXY(const CvPlot* pPlot, int iDX, int iDY) {
	return GC.getMapINLINE().plotXY(pPlot, iDX, iDY);
}
inline DirectionTypes directionXY(int iDX, int iDY)	{
	return GC.getMapINLINE().directionXY(iDX, iDY);
}
inline DirectionTypes directionXY(const CvPlot* pFromPlot, const CvPlot* pToPlot) {
	return GC.getMapINLINE().directionXY(pFromPlot, pToPlot);
}
// </advc.make>
#endif
