// advc.300: New class; see Shelf.h for description

#include "CvGameCoreDLL.h"
#include "Shelf.h"
#include "CvGame.h"
#include "CvPlot.h"
#include "CvUnit.h"
#include "CvPlayer.h"

using std::vector;


void Shelf::add(CvPlot* plot)
{
	plots.push_back(plot);
}


CvPlot* Shelf::randomPlot(RandPlotFlags restrictions, int unitDistance, int* legalCount) const
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
		 !(RANDPLOT_LAND & restrictions) &&
		 (!(RANDPLOT_UNOWNED & restrictions) || !plot->isOwned()) &&
		 (!(RANDPLOT_ADJACENT_UNOWNED & restrictions) || !plot->isAdjacentOwned()) &&
		 (!(RANDPLOT_NOT_VISIBLE_TO_CIV & restrictions) || !plot->isVisibleToCivTeam()) &&
		 // In case a mod enables sea cities:
		 (!(RANDPLOT_NOT_CITY & restrictions) || !plot->isCity()) &&
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
		CvUnit* anyUnit = CvUnit::fromIDInfo(unitNode->m_data);
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
		CvUnit* anyUnit = CvUnit::fromIDInfo(unitNode->m_data);
		if(anyUnit != NULL && anyUnit->isBarbarian() &&
			anyUnit->getUnitCombatType() != NO_UNITCOMBAT)
		{
			anyUnit->kill(false);
			return true;
		}
	}
	return false;
}

// advc.306:
CvUnit* Shelf::randomBarbarianTransport() const
{
	vector<CvUnit*> apValid;
	for(size_t i = 0; i < plots.size(); i++)
	{
		if(plots[i] == NULL) continue; CvPlot const& plot = *plots[i];
		if(plot.isVisibleToCivTeam())
			continue;
		FOR_EACH_UNIT_VAR_IN(pTransport, plot)
		{
			if(pTransport->getOwner() != BARBARIAN_PLAYER)
				break;
			CvUnitInfo const& u = GC.getInfo(pTransport->getUnitType());
			int iCargo = std::min(2, u.getCargoSpace()); // Load at most 2
			iCargo -= std::max(0, pTransport->getCargo());
			if(iCargo > 0)
				apValid.push_back(pTransport);
		}
	}
	if(apValid.empty())
		return NULL;
	int iValid = apValid.size();
	scaled rNoneProb = fixp(0.2) + scaled(iValid, 10);
	if(!rNoneProb.bernoulliSuccess(GC.getGame().getSRand(), "no Barbarian transport"))
		return NULL;
	return apValid[GC.getGame().getSorenRandNum(iValid, "choose Barbarian transport")];
}

Shelf::Id::Id(int landId, int waterId) : std::pair<int,int>(landId, waterId) {}
