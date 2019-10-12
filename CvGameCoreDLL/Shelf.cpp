// <advc.300> New class; see Shelf.h for description

#include "CvGameCoreDLL.h"
#include "Shelf.h"
#include "CvInfo_Unit.h"
#include "CvGame.h"
#include "CvPlot.h"
#include "CvUnit.h"

using std::vector;


void Shelf::add(CvPlot* plot)
{
	plots.push_back(plot);
}


CvPlot* Shelf::randomPlot(int restrictionFlags, int unitDistance, int* legalCount) const
{
	/*  Based on CvMap::syncRandPlot, but shelves are (normally) so small
		that random sampling isn't efficient. Instead, compute the legal
		plots first, then return one of those at random (NULL if none). */
	vector<CvPlot*> legal;
	for(size_t i = 0; i < plots.size(); i++)
	{
		CvPlot* plot = plots[i];
		bool isLegal =
		 plot != NULL &&
		 !(RANDPLOT_LAND & restrictionFlags) &&
		 (!(RANDPLOT_UNOWNED & restrictionFlags) || !plot->isOwned()) &&
		 (!(RANDPLOT_ADJACENT_UNOWNED & restrictionFlags) || !plot->isAdjacentOwned()) &&
		 (!(RANDPLOT_NOT_VISIBLE_TO_CIV & restrictionFlags) || !plot->isVisibleToCivTeam()) &&
		 // In case a mod enables sea cities:
		 (!(RANDPLOT_NOT_CITY) || !plot->isCity()) &&
		 (!plot->isCivUnitNearby(unitDistance)) &&
		 !plot->isUnit();
		/*  RANDPLOT_PASSIBLE, RANDPLOT_ADJACENT_LAND, RANDPLOT_HABITABLE:
			Ensured by CvMap::computeShelves. */
		if(isLegal)
			legal.push_back(plot);
	}
	int nLegal = legal.size();
	if(legalCount != NULL)
		*legalCount = nLegal;
	if(nLegal == 0)
		return NULL;
	return legal[GC.getGame().getSorenRandNum(nLegal, "advc.300")];
}


int Shelf::size() const
{
	return (int)plots.size();
}


int Shelf::countUnownedPlots() const
{
	int r = 0;
	for(size_t i = 0; i < plots.size(); i++)
	{
		CvPlot* plot = plots[i];
		if(plot != NULL && !plot->isOwned())
			r++;
	}
	return r;
}


int Shelf::countBarbarians() const
{
	int r = 0;
	for(size_t i = 0; i < plots.size(); i++)
	{
		CvPlot* plot = plots[i];
		if(plot == NULL)
			continue;
		CLLNode<IDInfo>* unitNode = plot->headUnitNode();
		if(unitNode == NULL)
			continue;
		CvUnit* anyUnit = ::getUnit(unitNode->m_data);
		if(anyUnit != NULL && anyUnit->isBarbarian())
			r += plot->getNumUnits();
	}
	return r;
}


bool Shelf::killBarbarian()
{
	for(size_t i = 0; i < plots.size(); i++)
	{
		CvPlot* plot = plots[i]; if(plot == NULL) continue;
		CLLNode<IDInfo>* unitNode = plot->headUnitNode();
		if(unitNode == NULL)
			continue;
		CvUnit* anyUnit = ::getUnit(unitNode->m_data);
		if(anyUnit != NULL && anyUnit->isBarbarian() &&
			anyUnit->getUnitCombatType() != NO_UNITCOMBAT)
		{
			anyUnit->kill(false);
			return true;
		}
	}
	return false;
}

// <advc.306>
CvUnit* Shelf::randomBarbarianCargoUnit() const
{
	vector<CvUnit*> legal;
	for(size_t i = 0; i < plots.size(); i++)
	{
		if(plots[i] == NULL) continue; CvPlot const& plot = * plots[i];
		if(plot.isVisibleToCivTeam())
			continue;
		for(int j = 0; j < plot.getNumUnits(); j++)
		{
			CvUnit* u = plot.getUnitByIndex(j); if(u == NULL) continue;
			if(u->getOwner() != BARBARIAN_PLAYER)
				break;
			CvUnitInfo const& ui = GC.getUnitInfo(u->getUnitType());
			int cargoSpace = std::min(2, ui.getCargoSpace()); // Load at most 2
			cargoSpace -= std::max(0, u->getCargo());
			if(cargoSpace > 0)
				legal.push_back(u);
		}
	}
	int nLegal = legal.size();
	if(nLegal == 0)
		return NULL;
	double pr = 0.2 + nLegal / 10.0;
	if(!::bernoulliSuccess(pr, "advc.306 (shelf)"))
		return NULL;
	return legal[GC.getGame().getSorenRandNum(nLegal, "advc.306")];
} // </advc.306>


Shelf::Id::Id(int landId, int waterId) : std::pair<int,int>(landId, waterId) {}
// </advc.300>
