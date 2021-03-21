#pragma once

#ifndef PLOT_RADIUS_ITERATOR_H
#define PLOT_RADIUS_ITERATOR_H

// advc.plotr: New header

#include "CvMap.h"

/*  Iterators over the CvPlot objects within a radius around a central plot or unit.
	If bINCIRCLE is false, then the radius is measured according to the
	stepDistance metric, which results in a square of plots. The center plot
	doesn't count toward the radius, so e.g. iRadius=3 yields a 7x7 square.

	If bINCIRCLE is true, then the plotDistance metric is used, which corresponds
	to a range of plots that approximates the incircle of a square of the same radius.
	The implementation for bINCIRCLE=true isn't terribly efficient: A square is
	generated and plots outside of the plostDistance radius are skipped.
	(That's also what the BtS code did.)
	There are derived classes at the end of this file that hide the template parameter.
	For the special case of iterating over a city radius, see CityPlotIterator.h.
	If iRadius is known to be 1 at compile time, then one of the macros in
	PlotAdjListTraversal.h should be preferred; they're faster.

	The order of traversal corresponds to a North-East-South-West (clockwise) spiral.
	NULL plots are skipped, but CvUnitAI::AI_plotValid isn't checked - should only be
	checked when considering to move into a tile, and even then there can be faster
	alternatives (see comments at the definition of AI_plotValid). */

template<bool bINCIRCLE = false>
class SquareIterator
{
public:
	SquareIterator(CvPlot const& kCenter, int iRadius, bool bIncludeCenter = true)
	{
		/*	I had written a const and non-const version (as derived classes that
			set a "PlotType" template parameter), but CityPlotIterator and
			AgentIterator don't have const versions either, and const-correctness
			would make it harder to add more iterator classes in the future.
			So I've removed those derived classes.
			Const-casting CvPlot should generally be safe. */
		m_pNext = const_cast<CvPlot*>(&kCenter);
		init(iRadius, bIncludeCenter);
	}

	SquareIterator(CvUnit const& kUnit, int iRadius, bool bIncludeCenter = true)
	{
		// Don't want to include CvUnit.h
		m_pNext = getUnitPlot(kUnit);
		init(iRadius, bIncludeCenter);
	}

	SquareIterator(int x, int y, int iRadius, bool bIncludeCenter = true)
	{
		m_pNext = GC.getMap().plot(x, y);
		init(iRadius, bIncludeCenter);
	}

	__forceinline bool hasNext() const
	{
		return (m_pNext != NULL);
	}

	__forceinline SquareIterator& operator++()
	{
		computeNext();
		return *this;
	}

	__forceinline CvPlot& operator*() const
	{
		return *m_pNext;
	}

	__forceinline CvPlot* operator->() const
	{
		return m_pNext;
	}

	inline int currStepDist() const
	{
		return ::stepDistance(m_pCenter, m_pNext);
	}

	inline int currPlotDist() const
	{
		return ::plotDistance(m_pCenter, m_pNext);
	}

	inline int radius() const
	{
		return m_iRadius;
	}

protected:
	void computeNext()
	{
		//PROFILE_FUNC(); //  Immediate sample stack overflow (b/c recursive?).
						  //  TSC_PROFILE suggests 10 mio. calls in a late-game turn.
		m_iPos++;
		if (m_iPos >= m_iMaxPos)
		{
			m_pNext = NULL;
			return;
		}
		// Change direction?
		if (m_iConsecMoves * (m_iConsecMoves + 1) >= m_iPos)
		{
			// Modulo by a constant power of 2: should be fast
			m_eDir = static_cast<CardinalDirectionTypes>(
					(m_eDir + 1) % NUM_CARDINALDIRECTION_TYPES);
			m_iConsecMoves = 1;
		}
		else m_iConsecMoves++;
		/*	Can't use CvMap::plotCardinalDirection b/c we need the current coordinates
			even if they're off the map */
		m_iCurrX += GC.getPlotCardinalDirectionX()[m_eDir];
		m_iCurrY += GC.getPlotCardinalDirectionY()[m_eDir];
		m_pNext = GC.getMap().plotValidXY(m_iCurrX, m_iCurrY);
		if (m_pNext == NULL)
		{
			computeNext();
			return; // tail recursion
		}
		if (bINCIRCLE)
		{
			if (currPlotDist() > m_iRadius)
				computeNext();
		}
	}

private:
	CvPlot const* m_pCenter; // Only for curr...Dist functions
	int m_iRadius;
	int m_iPos;
	int m_iMaxPos;
	CvPlot* m_pNext;
	int m_iCurrX, m_iCurrY;
	CardinalDirectionTypes m_eDir;
	int m_iConsecMoves;

	void init(int iRadius, bool bIncludeCenter)
	{
		m_pCenter = m_pNext;
		m_iCurrX = m_pNext->getX();
		m_iCurrY = m_pNext->getY();
		m_iMaxPos = SQR(2 * iRadius + 1) - 1;
		m_iRadius = iRadius;
		m_iPos = -1;
		// (We'll immediately turn NORTH after kCenter)
		m_eDir = CARDINALDIRECTION_WEST;
		m_iConsecMoves = 0;
		if (!bIncludeCenter)
		{
			m_iMaxPos--;
			computeNext();
		}
	}

	CvPlot* getUnitPlot(CvUnit const& kUnit) const;
};

// These two derived classes really only hide the template parameter ...

class SquareIter : public SquareIterator<false>
{
public:
	SquareIter(CvPlot const& kCenter, int iRadius, bool bIncludeCenter = true) :
			SquareIterator<false>(kCenter, iRadius, bIncludeCenter) {}

	SquareIter(CvUnit const& kUnit, int iRadius, bool bIncludeCenter = true) :
			SquareIterator<false>(kUnit, iRadius, bIncludeCenter) {}

	SquareIter(int x, int y, int iRadius, bool bIncludeCenter = true) :
			SquareIterator<false>(x, y, iRadius, bIncludeCenter) {}

	__forceinline SquareIter& operator++()
	{
		computeNext();
		return *this;
	}
};

class PlotCircleIter : public SquareIterator<true>
{
public:
	PlotCircleIter(CvPlot const& kCenter, int iRadius, bool bIncludeCenter = true) :
			SquareIterator<true>(kCenter, iRadius, bIncludeCenter) {}

	PlotCircleIter(CvUnit const& kUnit, int iRadius, bool bIncludeCenter = true) :
			SquareIterator<true>(kUnit, iRadius, bIncludeCenter) {}

	PlotCircleIter(int x, int y, int iRadius, bool bIncludeCenter = true) :
			SquareIterator<true>(x, y, iRadius, bIncludeCenter) {}

	__forceinline PlotCircleIter& operator++()
	{
		computeNext();
		return *this;
	}
};

#endif
