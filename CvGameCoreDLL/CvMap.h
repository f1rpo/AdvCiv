#pragma once

#ifndef CIV4_MAP_H
#define CIV4_MAP_H

//	FILE:	 CvMap.h
//	AUTHOR:  Soren Johnson
//	PURPOSE: Game map class
//	Copyright (c) 2004 Firaxis Games, Inc. All rights reserved.

#include "CvPlot.h"
#include "Shelf.h" // advc.300

class CvArea;
class FAStar;
class CvPlotGroup;
class CvSelectionGroup;

struct CvMapInitData // holds initialization info
{
	int m_iGridW; // in-game plots
	int m_iGridH; // in-game plots
	int m_iTopLatitude;
	int m_iBottomLatitude;

	bool m_bWrapX;
	bool m_bWrapY;

	CvMapInitData(int iGridW = 0, int iGridH = 0,
		int iTopLatitude = 90, int iBottomLatitude = -90,
		bool bWrapX = false, bool bWrapY = false)
	:	m_iGridH(iGridH), m_iGridW(iGridW),
		m_iTopLatitude(iTopLatitude), m_iBottomLatitude(iBottomLatitude),
		m_bWrapY(bWrapY), m_bWrapX(bWrapX)
	{}
};

class CvMap /* advc.003e: */ : private boost::noncopyable
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
	int plotDistance(int iX1, int iY1, int iX2, int iY2) const
	{	/*	advc.opt: inline keyword removed. Wasn't getting inlined either.
			xDistance and yDistance and its auxiliary functions are getting inlined.
			I'm guessing that's why the compiler resists inlining plotDistance;
			inlining the whole computation is probably bad for branch prediction. */
		int iDX = xDistance(iX1, iX2);
		int iDY = yDistance(iY1, iY2);
		//return std::max(iDX, iDY) + std::min(iDX, iDY) / 2;
		/*	advc.opt: Non-branching replacement
			(abs in stdlib uses just xor and sub). Could also use the max and min
			functions in CvGameCoreUtils.h, but this here seems a little faster. */
		return (3 * (iDX + iDY) + abs(iDX - iDY)) / 4;
	}

	// K-Mod, plot-to-plot alias for convenience:
	inline int plotDistance(const CvPlot* plot1, const CvPlot* plot2) const
	{
		return plotDistance(
				plot1->getX(), plot1->getY(),
				plot2->getX(), plot2->getY());
	}

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
	int stepDistance(int iX1, int iY1, int iX2, int iY2) const
	{	// std::opt: Was std::max. inline keyword removed; cf. plotDistance.
		return branchless::max(xDistance(iX1, iX2), yDistance(iY1, iY2));
	}

	// K-Mod, plot-to-plot alias for convenience:
	inline int stepDistance(const CvPlot* plot1, const CvPlot* plot2) const
	{
		return stepDistance(
				plot1->getX(), plot1->getY(),
				plot2->getX(), plot2->getY());
	}

	inline CvPlot* plotDirection(int iX, int iY, DirectionTypes eDirection) const
	{
		if(eDirection == NO_DIRECTION)
			return plotValidXY(iX, iY);
		// advc.opt: Don't check for INVALID_PLOT_COORD
		return plotValidXY(
				iX + GC.getPlotDirectionX()[eDirection],
				iY + GC.getPlotDirectionY()[eDirection]);
	}

	inline CvPlot* plotCardinalDirection(int iX, int iY,
		CardinalDirectionTypes eCardinalDirection) const
	{
		// advc.opt: Don't check for INVALID_PLOT_COORD
		return plotValidXY(
			iX + GC.getPlotCardinalDirectionX()[eCardinalDirection],
			iY + GC.getPlotCardinalDirectionY()[eCardinalDirection]);
	}

	inline CvPlot* plotXY(int iX, int iY, int iDX, int iDY) const
	{
		// advc.opt: Don't check for INVALID_PLOT_COORD
		return plotValidXY(iX + iDX, iY + iDY);
	}
	// K-Mod start
	inline CvPlot* plotXY(const CvPlot* pPlot, int iDX, int iDY) const
	{
		return plotXY(pPlot->getX(), pPlot->getY(), iDX, iDY);
	} // K-Mod end

	inline DirectionTypes directionXY(int iDX, int iDY) const
	{
		/*if (abs(iDX) > DIRECTION_RADIUS || abs(iDY) > DIRECTION_RADIUS)
			return NO_DIRECTION;*/ // advc.opt: Apparently can't happen, so:
		FAssert(!(abs(iDX) > DIRECTION_RADIUS || abs(iDY) > DIRECTION_RADIUS));
		return GC.getXYDirection(iDX + DIRECTION_RADIUS, iDY + DIRECTION_RADIUS);
	}

	inline DirectionTypes directionXY(CvPlot const& kFromPlot, CvPlot const& kToPlot) const // advc: take params as references
	{
		return directionXY(
				dxWrap(kToPlot.getX() - kFromPlot.getX()),
				dyWrap(kToPlot.getY() - kFromPlot.getY()));
	}

	inline int dxWrap(int iDX) const
	{
		return wrapCoordDifference(iDX, getGridWidth(), isWrapX());
	}

	inline int dyWrap(int iDY) const
	{
		return wrapCoordDifference(iDY, getGridHeight(), isWrapY());
	}

	inline int xDistance(int iFromX, int iToX) const
	{
		return coordDistance(iFromX, iToX, getGridWidth(), isWrapX());
	}

	inline int yDistance(int iFromY, int iToY) const
	{
		return coordDistance(iFromY, iToY, getGridHeight(), isWrapY());
	}

	inline CvPlot* plotCity(int iX, int iY, CityPlotTypes ePlot) const						// Exposed to Python (CyGameCoreUtils.py)
	{	// advc.enum: 3rd param was int
		// advc.opt: Don't check for INVALID_PLOT_COORD
		return plotValidXY(iX + GC.getCityPlotX()[ePlot], iY + GC.getCityPlotY()[ePlot]);
	}

	CityPlotTypes plotCityXY(int iDX, int iDY) const // advc.enum: return CityPlotTypes		// Exposed to Python (CyGameCoreUtils.py)
	{
		if (abs(iDX) > CITY_PLOTS_RADIUS || abs(iDY) > CITY_PLOTS_RADIUS)
			return NO_CITYPLOT; // advc.enum
		return GC.getXYCityPlot(iDX + CITY_PLOTS_RADIUS, iDY + CITY_PLOTS_RADIUS);
	}
	// advc: 1st param (CvCity*) replaced with two ints - to allow hypothetical city sites
	inline CityPlotTypes plotCityXY(int iCityX, int iCityY, CvPlot const& kPlot) const		// Exposed to Python (CyGameCoreUtils.py)
	{
		return plotCityXY(dxWrap(kPlot.getX() - iCityX), dyWrap(kPlot.getY() - iCityY));
	}
	// advc:
	inline bool adjacentOrSame(CvPlot const& kFirstPlot, CvPlot const& kSecondPlot) const
	{
		return (stepDistance(&kFirstPlot, &kSecondPlot) <= 1);
	}
	// advc.opt: Check cache at CvPlot before doing the computation
	inline bool isSeparatedByIsthmus(CvPlot const& kFrom, CvPlot const& kTo) const
	{
		if (!kFrom.isAnyIsthmus())
			return false;
		return isSeparatedByIsthmus_bulk(kFrom, kTo);
	}

private: // Auxiliary functions
	/*	These look too large and branchy for inlining, but the keywords
		(already present in BtS) do seem to improve performance a little bit.
		Maybe b/c plotDistance and stepDistance aren't being inlined. */

	inline int coordDistance(int iFrom, int iTo, int iRange, bool bWrap) const
	{
		int iDelta = abs(iFrom - iTo); // advc.opt: Make sure this is computed only once
		if (bWrap && iDelta > iRange / 2)
			return iRange - iDelta;
		return iDelta;
	}

	inline int wrapCoordDifference(int iDiff, int iRange, bool bWrap) const
	{
		if (!bWrap)
			return iDiff;
		if (iDiff > iRange / 2)
			return iDiff - iRange;
		if (iDiff < -(iRange / 2))
			return iDiff + iRange;
		return iDiff;
	}

	inline int coordRange(int iCoord, int iRange, bool bWrap) const
	{
		if (!bWrap || iRange == 0)
			return iCoord;
		if (iCoord < 0)
			return (iRange + (iCoord % iRange));
		if (iCoord >= iRange)
			return (iCoord % iRange);
		return iCoord;
	}
	// (end of functions moved from CvGameCoreUtils.h) </advc.make>
	// advc (for advc.030, advc.027): Cut from teamStepValid in CvGameCoreUtils
	bool isSeparatedByIsthmus_bulk(CvPlot const& kFrom, CvPlot const& kTo) const
	{
		return (kFrom.isWater() && kTo.isWater() &&
			// Safe wrt. map edges b/c we know (assume) that kFrom and kTo are adjacent
			!getPlot(kFrom.getX(), kTo.getY()).isWater() &&
			!getPlot(kTo.getX(), kFrom.getY()).isWater());
	}

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

public: // advc: made several functions const

	DllExport void erasePlots();																			// Exposed to Python
	void setRevealedPlots(TeamTypes eTeam, bool bNewValue, bool bTerrainOnly = false);		// Exposed to Python
	void setAllPlotTypes(PlotTypes ePlotType);												// Exposed to Python

	void doTurn();

	DllExport void updateFlagSymbols();
	void setFlagsDirty(); // K-Mod
	DllExport void updateFog();
	void updateVisibility();																// Exposed to Python
	DllExport void updateSymbolVisibility();
	void updateSymbols();
	DllExport void updateMinimapColor();															// Exposed to Python
	void updateSight(bool bIncrement);
	void updateIrrigated();
	DllExport void updateCenterUnit();
	void updateWorkingCity();
	void updateMinOriginalStartDist(CvArea const& kArea);										// Exposed to Python
	void updateYield();

	void verifyUnitValidPlot();
	void combinePlotGroups(PlayerTypes ePlayer, CvPlotGroup* pPlotGroup1, CvPlotGroup* pPlotGroup2,
			bool bVerifyProduction = true); // advc.064d

	CvPlot* syncRandPlot(RandPlotFlags eFlags = RANDPLOT_ANY,								// Exposed to Python
			CvArea const* pArea = NULL, // advc: was iArea
			int iMinCivUnitDistance = -1,
			int iTimeout = -1, int* piValidCount = NULL); // advc.304 (default timeout was 100)
	// <advc>
	bool isValidRandPlot(CvPlot const& kPlot, RandPlotFlags eFlags,
			CvArea const* pArea, int iMinCivUnitDistance) const; // </advc>

	DllExport CvCity* findCity(int iX, int iY, PlayerTypes eOwner = NO_PLAYER, TeamTypes eTeam = NO_TEAM, bool bSameArea = true, bool bCoastalOnly = false, TeamTypes eTeamAtWarWith = NO_TEAM, DirectionTypes eDirection = NO_DIRECTION, CvCity* pSkipCity = NULL)	// Exposed to Python
	{	// <advc.004r>
		return findCity(iX, iY, eOwner, eTeam, bSameArea, bCoastalOnly,
				eTeamAtWarWith, eDirection, pSkipCity, NO_TEAM);
	}
	CvCity* findCity(int iX, int iY, PlayerTypes eOwner = NO_PLAYER, TeamTypes eTeam = NO_TEAM,
			bool bSameArea = true, bool bCoastalOnly = false, TeamTypes eTeamAtWarWith = NO_TEAM,
			DirectionTypes eDirection = NO_DIRECTION, CvCity const* pSkipCity = NULL, TeamTypes eObserver = NO_TEAM) const;
	// </advc.004r>
	CvSelectionGroup* findSelectionGroup(int iX, int iY, PlayerTypes eOwner = NO_PLAYER,					// Exposed to Python
			bool bReadyToSelect = false, bool bWorkers = false) const;

	CvArea* findBiggestArea(bool bWater);																						// Exposed to Python

	int getMapFractalFlags() const;																				// Exposed to Python
	bool findWater(CvPlot const* pPlot, int iRange, bool bFreshWater);										// Exposed to Python

	bool isPlotExternal(int iX, int iY) const; // advc.inl: Exported through .def file							// Exposed to Python
	inline bool isPlot(int iX, int iY) const // advc.inl: Renamed from isPlotINLINE; return type was int.
	{
		return (iX >= 0 && iX < getGridWidth() && iY >= 0 && iY < getGridHeight());
	}
	int numPlotsExternal() const; // advc.inl: Exported through .def file							// Exposed to Python
	inline PlotNumTypes numPlots() const // advc.inl: Renamed from numPlotsINLINE
	{
		return m_ePlots;//getGridWidth() * getGridHeight(); // advc.opt
	}
	/*	advc.inl: Merged with plotNumINLINE (plotNum wasn't called externally).
		advc.enum: return type changed from int.
		Tbd(?).: Cache this at CvPlot (and possibly CvCity)? */
	inline PlotNumTypes plotNum(int iX, int iY) const 												// Exposed to Python
	{
		return (PlotNumTypes)(iY * getGridWidth() + iX);
	}  // advc: wrapper
	__forceinline PlotNumTypes plotNum(CvPlot const& kPlot) const
	{
		return plotNum(kPlot.getX(), kPlot.getY());
	}
	int plotX(int iIndex) const;																										// Exposed to Python
	int plotY(int iIndex) const;																										// Exposed to Python

	int pointXToPlotX(float fX) const;
	DllExport float plotXToPointX(int iX)  // <advc> const version
	{
		CvMap const& kThis(*this);
		return kThis.plotXToPointX(iX);
	}
	float plotXToPointX(int iX) const; // </advc>
	int pointYToPlotY(float fY) const;
	DllExport float plotYToPointY(int iY)  // <advc> const version
	{
		CvMap const& kThis(*this);
		return kThis.plotYToPointY(iY);
	}
	float plotYToPointY(int iY) const; // </advc>

	float getWidthCoords() const;
	float getHeightCoords() const;

	int maxPlotDistance() const;																								// Exposed to Python
	int maxStepDistance() const;																								// Exposed to Python
	int maxMaintenanceDistance() const; // advc.140
	int maxTypicalDistance() const; // advc.140

	int getGridWidthExternal() const; // advc.inl: Exported through .def file							// Exposed to Python
	inline int getGridWidth() const // advc.inl: Renamed from getGridWidthINLINE
	{
		return m_iGridWidth;
	}
	int getGridHeightExternal() const; // advc.inl: Exported through .def file							// Exposed to Python																	// Exposed to Python
	inline int getGridHeight() const // advc.inl: Renamed from getGridHeightINLINE
	{
		return m_iGridHeight;
	}

	int getLandPlots() const { return m_iLandPlots; } // advc.inl										// Exposed to Python
	void changeLandPlots(int iChange);
	int getWaterPlots() const { return numPlots() - getLandPlots(); } // advc

	int getOwnedPlots() const { return m_iOwnedPlots; }														// Exposed to Python
	void changeOwnedPlots(int iChange);

	int getTopLatitude() const;																									// Exposed to Python
	int getBottomLatitude() const;																							// Exposed to Python

	short getNextRiverID() const;																									// Exposed to Python
	void incrementNextRiverID();																					// Exposed to Python

	bool isWrapXExternal(); // advc.inl: Exported through .def file							// Exposed to Python
	inline bool isWrapX() const { return m_bWrapX; } // advc.inl: Renamed from isWrapXINLINE
	bool isWrapYExternal(); // advc.inl: Exported through .def file							// Exposed to Python
	inline bool isWrapY() const { return m_bWrapY; } // advc.inl: Renamed from isWrapYINLINE
	bool isWrapExternal(); // advc.inl: Exported through .def file
	inline bool isWrap() const // advc.inl: Renamed from isWrapINLINE
	{
		return m_bWrapX || m_bWrapY;
	}

	DllExport WorldSizeTypes getWorldSize() // <advc> const version									// Exposed to Python
	{
		CvMap const& kThis = *this;
		return kThis.getWorldSize();
	}
	WorldSizeTypes getWorldSize() const
	{
		return GC.getInitCore().getWorldSize();
	} // </advc>
	ClimateTypes getClimate() const { return GC.getInitCore().getClimate(); } // advc.inl						// Exposed to Python
	SeaLevelTypes getSeaLevel() const { return GC.getInitCore().getSeaLevel(); } // advc.inl					// Exposed to Python

	int getNumCustomMapOptions() const;
	CustomMapOptionTypes getCustomMapOption(int iOption) const;											// Exposed to Python
	CvWString getNonDefaultCustomMapOptionDesc(int iOption) const; // advc.190b (exposed to Python)

	int getNumBonuses(BonusTypes eIndex) const;																	// Exposed to Python
	void changeNumBonuses(BonusTypes eIndex, int iChange);

	int getNumBonusesOnLand(BonusTypes eIndex) const;														// Exposed to Python
	void changeNumBonusesOnLand(BonusTypes eIndex, int iChange);

	CvPlot* plotByIndexExternal(int iIndex) const; // advc.inl: Exported through .def file							// Exposed to Python
	// advc.enum (tbd.): Change param to PlotNumTypes
	inline CvPlot* plotByIndex(int iIndex) const // advc.inl: Renamed from plotByIndexINLINE
	{
		return ((iIndex >= 0 && iIndex < numPlots()) ? &(m_pMapPlots[iIndex]) : NULL);
	} // <advc.inl> Faster (w/o branching)
	__forceinline CvPlot& getPlotByIndex(int iIndex) const
	{
		FAssertBounds(0, numPlots(), iIndex);
		return m_pMapPlots[iIndex];
	} // </advc.inl>
	CvPlot* plotExternal(int iX, int iY) const; // advc.inl: Exported through .def file							// Exposed to Python
	/*  advc.inl: Renamed from plotINLINE. Was inlined, but I'm getting slightly
		better performance without that (having replaced some calls with getPlot) . */
	CvPlot* plot(int iX, int iY) const
	{
		if (iX == INVALID_PLOT_COORD || iY == INVALID_PLOT_COORD)
			return NULL;
		int iMapX = coordRange(iX, getGridWidth(), isWrapX());
		int iMapY = coordRange(iY, getGridHeight(), isWrapY());
		return (isPlot(iMapX, iMapY) ? &m_pMapPlots[plotNum(iMapX, iMapY)] : NULL);
	}
	// advc.inl: Renamed from plotSorenINLINE (was already force-inlined in BtS and that helps too)
	__forceinline CvPlot* plotSoren(int iX, int iY) const
	{
		if (iX == INVALID_PLOT_COORD || iY == INVALID_PLOT_COORD)
			return NULL;
		FAssert(isPlot(iX, iY)); // advc: Assertion added
		return &(m_pMapPlots[plotNum(iX, iY)]);
	} // <advc.inl> Even faster and less confusingly named; replacing the above in most places.
	__forceinline CvPlot& getPlot(int x, int y) const
	{
		FAssert(isPlot(x, y));
		return m_pMapPlots[plotNum(x, y)];
	} // </advc.inl>
	/*	advc.opt: Yet another plot getter. Checks coordRange but not INVALID_PLOT_COORD.
		For functions that compute x,y as an offset from a (valid) plot -
		not plausible that the new coordinates would equal INVALID_PLOT_COORD.
		'inline' tested - faster without it. */
	CvPlot* plotValidXY(int iX, int iY) const
	{
		int iMapX = coordRange(iX, getGridWidth(), isWrapX());
		int iMapY = coordRange(iY, getGridHeight(), isWrapY());
		return (isPlot(iMapX, iMapY) ? &m_pMapPlots[plotNum(iMapX, iMapY)] : NULL);
	}

	DllExport CvPlot* pointToPlot(float fX, float fY);												// Exposed to Python

	int getIndexAfterLastArea() const;																// Exposed to Python
	int getNumAreas() const																			// Exposed to Python
	{
		return m_areas.getCount();
	}
	int getNumLandAreas() const;
	CvArea* getArea(int iID) const																	// Exposed to Python
	{
		return m_areas.getAt(iID);
	}
	CvArea* addArea();
	void deleteArea(int iID);
	// iteration
	CvArea* firstArea(int *pIterIdx, bool bRev=false) const											// Exposed to Python
	{	//return (!bRev ? m_areas.beginIter(pIterIdx) : m_areas.endIter(pIterIdx));
		FAssert(!bRev);
		return m_areas.beginIter(pIterIdx); // advc.opt
	}
	CvArea* nextArea(int *pIterIdx, bool bRev=false) const											// Exposed to Python
	{	//return (!bRev ? m_areas.nextIter(pIterIdx) : m_areas.prevIter(pIterIdx));
		return m_areas.nextIter(pIterIdx); // advc.opt
	}

	void recalculateAreas(bool bUpdateIsthmuses = true);												// Exposed to Python
	// <advc.300>
	void computeShelves();
	void getShelves(CvArea const& kArea, std::vector<Shelf*>& r) const;
	// </advc.300>
	void resetPathDistance();																		// Exposed to Python
	int calculatePathDistance(CvPlot const* pSource, CvPlot const* pDest) const;					// Exposed to Python
	void updateIrrigated(CvPlot& kPlot); // advc.pf

	// BETTER_BTS_AI_MOD, Efficiency (plot danger cache), 08/21/09, jdog5000: START
	//void invalidateIsActivePlayerNoDangerCache();
	void invalidateActivePlayerSafeRangeCache(); // K-Mod version
	void invalidateBorderDangerCache(TeamTypes eTeam);
	// BETTER_BTS_AI_MOD: END

	// Serialization:
	virtual void read(FDataStreamBase* pStream);
	virtual void write(FDataStreamBase* pStream);

	void rebuild(int iGridW, int iGridH, int iTopLatitude, int iBottomLatitude,			 			// Exposed to Python
			bool bWrapX, bool bWrapY, WorldSizeTypes eWorldSize,
			ClimateTypes eClimate, SeaLevelTypes eSeaLevel,
			int iNumCustomMapOptions, CustomMapOptionTypes* eCustomMapOptions);
	void updateReplayTexture(); // advc.106n
	byte const* getReplayTexture() const; // advc.106n
	/*	<advc.002a> Set through BUG options, but I worry that accessing those
		while redrawing the minimap plot for plot would be too slow.
		Let BUG cache the settings here whenever they change. */
	class MinimapSettings : private boost::noncopyable
	{
	public:
		MinimapSettings()
		:	m_bShowUnits(false),
			/*	Replacing STANDARD_MINIMAP_ALPHA in CvPlot.cpp,
				which was 0.6f. */
			m_fLandAlpha(0.75f),
			m_fWaterAlpha(m_fLandAlpha)
		{}
		// All exposed to Python via CyMap
		void setShowUnits(bool b) { m_bShowUnits = b; }
		void setWaterAlpha(float f) { m_fWaterAlpha = f; }
		void setLandAlpha(float f) { m_fLandAlpha = f; }
		bool isShowUnits() const { return m_bShowUnits; }
		float getWaterAlpha() const { return m_fWaterAlpha; }
		float getLandAlpha() const { return m_fLandAlpha; }
	private:
		bool m_bShowUnits;
		float m_fWaterAlpha;
		float m_fLandAlpha;
	};
	MinimapSettings& getMinimapSettings() { return m_minimapSettings; }
	MinimapSettings const& getMinimapSettings() const { return m_minimapSettings; }
	// </advc.002a>

protected:

	int m_iGridWidth;
	int m_iGridHeight;
	PlotNumTypes m_ePlots; // advc.opt
	int m_iLandPlots;
	int m_iOwnedPlots;
	int m_iTopLatitude;
	int m_iBottomLatitude;
	short m_iNextRiverID; // advc.opt: was int (these get stored at CvPlot)

	bool m_bWrapX;
	bool m_bWrapY;
	// <advc.enum>
	EnumMap<BonusTypes,int> m_aiNumBonus;
	EnumMap<BonusTypes,int> m_aiNumBonusOnLand;
	// </advc.enum>
	CvPlot* m_pMapPlots;
	std::map<Shelf::Id,Shelf*> m_shelves; // advc.300
	FFreeListTrashArray<CvArea> m_areas;
	std::vector<byte> m_replayTexture; // advc.106n
	MinimapSettings m_minimapSettings; // advc.002a

	void calculateAreas();
	// <advc.030>
	void calculateAreas_030();
	void calculateReprAreas();
	void calculateAreas_DFS(CvPlot const& p);
	void updateLakes();
	// </advc.030>
	void updatePlotNum(); // advc.opt
};

// advc.enum: (for EnumMap)
__forceinline PlotNumTypes getEnumLength(PlotNumTypes)
{
	return GC.getMap().numPlots();
}

/* <advc.make> Global wrappers for distance functions. The int versions are
	exposed to Python */
inline int plotDistance(int iX1, int iY1, int iX2, int iY2) {
	return GC.getMap().plotDistance(iX1, iY1, iX2, iY2);
}
inline int plotDistance(const CvPlot* plot1, const CvPlot* plot2) {
	return GC.getMap().plotDistance(plot1, plot2);
}
inline int stepDistance(int iX1, int iY1, int iX2, int iY2) {
	return GC.getMap().stepDistance(iX1, iY1, iX2, iY2);
}
inline int stepDistance(const CvPlot* plot1, const CvPlot* plot2) {
	return GC.getMap().stepDistance(plot1, plot2);
}
inline CvPlot* plotDirection(int iX, int iY, DirectionTypes eDirection) {
	return GC.getMap().plotDirection(iX, iY, eDirection);
}
inline CvPlot* plotCardinalDirection(int iX, int iY, CardinalDirectionTypes eCardinalDirection)	{
	return GC.getMap().plotCardinalDirection(iX, iY, eCardinalDirection);
}
inline CvPlot* plotXY(int iX, int iY, int iDX, int iDY)	{
	return GC.getMap().plotXY(iX, iY, iDX, iDY);
}
inline CvPlot* plotXY(const CvPlot* pPlot, int iDX, int iDY) {
	return GC.getMap().plotXY(pPlot, iDX, iDY);
}
inline DirectionTypes directionXY(int iDX, int iDY)	{
	return GC.getMap().directionXY(iDX, iDY);
}
inline DirectionTypes directionXY(CvPlot const& kFromPlot, CvPlot const& kToPlot) {
	// advc: params changed to references
	return GC.getMap().directionXY(kFromPlot, kToPlot);
}
inline CvPlot* plotCity(int iX, int iY, CityPlotTypes ePlot) {
	return GC.getMap().plotCity(iX, iY, ePlot);
}
inline CityPlotTypes plotCityXY(int iCityX, int iCityY, CvPlot const& kPlot) {
	return GC.getMap().plotCityXY(iCityX, iCityY, kPlot);
}
inline bool adjacentOrSame(CvPlot const& kFirstPlot, CvPlot const& kSecondPlot) { // advc
	return GC.getMap().adjacentOrSame(kFirstPlot, kSecondPlot);
}
// (Moved from CvGameCoreUtils.h)
#ifndef _USRDLL // use non inline functions when not in the dll
	#define getMap	getMapExternal
	#define getGridHeight	getGridHeightExternal
	#define getGridWidth	getGridWidthExternal
	#define isWrapY	isWrapYExternal
	#define isWrapX	isWrapXExternal
	#define plot	plotExternal
	#define getX	getXExternal
	#define getY	getYExternal
#endif
// </advc.make>
#endif
