// advc.027: New file; see comment in header.

#include "CvGameCoreDLL.h"
#include "StartingPositionIteration.h"
#include "CvPlayerAI.h"
#include "CvGamePlay.h"
#include "CitySiteEvaluator.h"
#include "CvMap.h"
#include "PlotRadiusIterator.h"

using std::map;
using std::vector;
using std::make_pair;


StartingPositionIteration::StartingPositionIteration() : m_bRestrictedAreas(false)
{
	CvMap const& kMap = GC.getMap();

	/*	Generate a starting site for each civ, starting with humans,
		otherwise in a random order - just as CvGame::assignStartingPlots does.
		Map scripts might depend on this order. If a map script sets the
		starting sites, then we have to bail anyway, but we don't know that yet. */
	vector<CvPlayer*> civPlayers;
	for (PlayerIter<HUMAN,ANY_AGENT_RELATION,true> it; it.hasNext(); ++it)
		civPlayers.push_back(&(*it));
	for (PlayerIter<CIV_ALIVE,ANY_AGENT_RELATION,true> it; it.hasNext(); ++it)
	{
		if (!it->isHuman())
			civPlayers.push_back(&*it);
	}
	FAssert(!GC.getGame().isTeamGame());
	// In non-team games, Pangaea shouldn't need custom code for starting sites.
	bool const bIgnoreScript = (GC.getInitCore().getMapScriptName().compare(L"Pangaea") == 0);
	size_t i = 0;
	for (; i < civPlayers.size(); i++)
	{
		CvPlayer& kPlayer = *civPlayers[i];
		if (kPlayer.getStartingPlot() != NULL)
			break;
		gDLL->callUpdater();
		bool bSiteFoundByScript=false;
		bool bRestrictedAreas=false;
		CvPlot* pSite = kPlayer.findStartingPlot(false,
				&bSiteFoundByScript, &bRestrictedAreas);
		if (pSite == NULL || (bSiteFoundByScript && !bIgnoreScript))
			break;
		if (bRestrictedAreas)
			m_bRestrictedAreas = true;
		/*	Would rather not store sites at players until the end of our computations,
			but then findStartingPlot will return the same site over and over. */
		kPlayer.setStartingPlot(pSite, false);
	}
	if (i != civPlayers.size()) // Revert any changes and bail
	{
		for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
			it->setStartingPlot(NULL, false);
		return;
	} // Past this point, we'll have set sensible starting plots; won't revert anymore.

	// Precomputations (too costly to repeat in each iteration) ...

	initSiteEvaluator();
	PotentialSiteGen potentialSiteGen(*m_pEval, m_bRestrictedAreas);
	if (potentialSiteGen.numSites() < civPlayers.size())
		return;
	// Remove the current solution. Just want the alternatives.
	potentialSiteGen.removeOriginalSites();
	vector<CvPlot const*> potentialSites;
	potentialSiteGen.getPlots(potentialSites);
	// Mark alt. sites with a ruin
	/*for(size_t i = 0; i < potentialSites.size(); i++)
		GC.getMap().getPlotByIndex(GC.getMap().plotNum(*potentialSites[i])).setImprovementType(GC.getRUINS_IMPROVEMENT());*/

	EnumMap<PlotNumTypes,scaled> yieldValues;
	CitySiteEvaluator yieldEval(m_pEval->getPlayer(), -1, /*bStartingLoc=*/false, true);
	FOR_EACH_ENUM(PlotNum)
	{
		// CitySiteEvaluator has many useful subroutines for this; handle it there.
		yieldValues.set(eLoopPlotNum, yieldEval.
				evaluateWorkablePlot(kMap.getPlotByIndex(eLoopPlotNum)));
		// Mark nonnegative-value plots with a ruin
		//if(yieldValues.get(eLoopPlotNum)>0)GC.getMap().getPlotByIndex(i).setImprovementType(GC.getRUINS_IMPROVEMENT());
	}

	DistanceTable const* pathDists = NULL;
	{	// Temp data that I want to go out of scope
		// Need a vector of all sites. Add the original sites temporarily.
		for (size_t i = 0; i < civPlayers.size(); i++)
		{
			potentialSites.push_back(civPlayers[i]->getStartingPlot());
		}
		// Also want inter-site distances
		std::set<PlotNumTypes> sitePlotNums;
		for (size_t i = 0; i < potentialSites.size(); i++)
		{
			sitePlotNums.insert(kMap.plotNum(*potentialSites[i]));
		}
		std::vector<CvPlot const*> relevantPlots;
		FOR_EACH_ENUM(PlotNum)
		{
			if (yieldValues.get(eLoopPlotNum) > 0 ||
				sitePlotNums.count(eLoopPlotNum) > 0)
			{
				relevantPlots.push_back(&kMap.getPlotByIndex(eLoopPlotNum));
			}
		}
		gDLL->callUpdater(); // Dunno if these are needed or have any effect really
		pathDists = new DistanceTable(potentialSites, relevantPlots);
		gDLL->callUpdater();
		// Remove the original sites again
		for (size_t i = 0; i < civPlayers.size(); i++)
			potentialSites.pop_back();
	}

	/*	Start value computation; let's first try it for the initial solution,
		i.e. the starting plots currently set for the civ players alive. */
	EnumMap<PlayerTypes,short> foundValues;
	potentialSiteGen.getCurrentFoundValues(foundValues);
	EnumMap<PlayerTypes,scaled> startValues;
	computeStartValues(foundValues, yieldValues, *pathDists, startValues);

	// tbd: iterate, stop criterion

	delete pathDists; // (Move to destructor if we do nothing more here)
}


StartingPositionIteration::~StartingPositionIteration()
{
	SAFE_DELETE(m_pEval);
}


void StartingPositionIteration::initSiteEvaluator()
{
	/*	Teams have already received free tech at this point, so the player used
		for city site evaluation isn't irrelevant. (Its personality should be though.)
		Pick the player with the least tech, prefer human when breaking ties. */
	CvPlayerAI const* pPlayer = NULL;
	int iBestPlayerVal = MAX_INT;
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		int iVal = 10 * it->getTechScore();
		if (it->isHuman())
			iVal--;
		if (iVal < iBestPlayerVal)
		{
			iBestPlayerVal = iVal;
			pPlayer = &*it;
		}
	}
	m_pEval = new CitySiteEvaluator(*pPlayer, -1, true);
	// Evaluating the surroundings will be our job
	m_pEval->setIgnoreStartingSurroundings(true);
}


StartingPositionIteration::PotentialSiteGen::PotentialSiteGen(
	CitySiteEvaluator const& kEval, bool bRestrictedAreas) :
	m_kEval(kEval), m_pVicinityPenaltiesPerPlot(NULL)
{
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		m_sitesClosestToOriginalSite.insert(make_pair(
				it->getID(), new VoronoiCell()));
	}

	scaled const rMinFoundVal = computeMinFoundValue();
	std::set<CvArea const*> validAreas;
	if (bRestrictedAreas)
	{
		for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
		{
			validAreas.insert(it->getStartingPlot()->area());
		}
	}
	CvMap const& kMap = GC.getMap();
	FOR_EACH_ENUM(PlotNum)
	{
		CvPlot const& kPlot = kMap.getPlotByIndex(eLoopPlotNum);
		if (bRestrictedAreas && validAreas.count(kPlot.area()) <= 0)
			continue;
		short iFoundValue = std::max<short>(0, m_kEval.evaluate(kPlot));
		if (iFoundValue >= rMinFoundVal)
			m_foundValuesPerSite.insert(make_pair(eLoopPlotNum, iFoundValue));
	}
	uint const iPlayers = (uint)PlayerIter<CIV_ALIVE>::count();
	if (numSites() <= iPlayers)
		return;

	// "Clearing the neighborhood"

	m_pVicinityPenaltiesPerPlot = new EnumMap<PlotNumTypes,scaled>();
	for (map<PlotNumTypes,short>::const_iterator itSite = m_foundValuesPerSite.begin();
		itSite != m_foundValuesPerSite.end(); ++itSite)
	{
		CvPlot const& p = kMap.getPlotByIndex(itSite->first);
		recordSite(p, itSite->second, true);
	}
	while (numSites() > iPlayers * 8u)
	{
		/*	Stop earlier when running out of alternatives in some map region.
			(Tbd. - Perhaps better: take Voronoi cells with few sites left off limits.) */
		if (fewestPotentialSites() < 2 && numSites() < iPlayers * 15u)
			break;
		map<PlotNumTypes,short>::iterator minPos;
		scaled rMinVal = scaled::MAX();
		for (map<PlotNumTypes,short>::iterator it = m_foundValuesPerSite.begin();
			it != m_foundValuesPerSite.end(); ++it)
		{
			scaled rVal = it->second * std::max(fixp(0.3),
					1 - m_pVicinityPenaltiesPerPlot->get(it->first));
			if (rVal < rMinVal)
			{
				rMinVal = rVal;
				minPos = it;
			}
		}
		CvPlot const& kMinPlot = kMap.getPlotByIndex(minPos->first);
		int iMinFoundVal = minPos->second;
		m_foundValuesPerSite.erase(minPos);
		recordSite(kMinPlot, iMinFoundVal, false);
	}
	/*	Pointer member b/c I want to free the memory here; only the ctor and
		its subroutines use it. */
	delete m_pVicinityPenaltiesPerPlot;
}


StartingPositionIteration::PotentialSiteGen::~PotentialSiteGen()
{
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		delete m_sitesClosestToOriginalSite[it->getID()];
	}
}


scaled StartingPositionIteration::PotentialSiteGen::computeMinFoundValue() const
{
	vector<short> aiFoundValuesPerOriginalSite;
	short iWorstCurrFoundVal= MAX_SHORT;
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		short iFoundVal = m_kEval.evaluate(*it->getStartingPlot());
		aiFoundValuesPerOriginalSite.push_back(iFoundVal);
		iWorstCurrFoundVal = std::min(iWorstCurrFoundVal, iFoundVal);
	}
	scaled r = iWorstCurrFoundVal * fixp(0.87);
	r.increaseTo(stats::median(aiFoundValuesPerOriginalSite) * fixp(2/3.));
	return r;
}

/*	(Could get the found value from m_foundValuesPerSite, but the callers
	happen to have it at hand.) */
void StartingPositionIteration::PotentialSiteGen::recordSite(
	CvPlot const& kPlot, short iFoundValue, bool bAdd)
{
	CvMap const& kMap = GC.getMap();

	// Update vicinity penalties
	int const iSign = (bAdd ? 1 : -1);
	for (PlotCircleIter it(kPlot, 8, false); it.hasNext(); ++it)
	{
		PlotNumTypes const eLoopPlot = kMap.plotNum(*it);
		map<PlotNumTypes,short>::const_iterator pos = m_foundValuesPerSite.find(eLoopPlot);
		if (pos != m_foundValuesPerSite.end() && pos->second < iFoundValue)
		{
			m_pVicinityPenaltiesPerPlot->add(eLoopPlot, iSign *
					scaled(1, 4 << it.currPlotDist()));
			FAssert(!m_pVicinityPenaltiesPerPlot->get(eLoopPlot).isNegative());
		}
	}

	// Update Voronoi cells
	PlayerTypes eClosestPlayer = NO_PLAYER;
	int iShortestDist = MAX_INT;
	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		int iDist = kMap.stepDistance(&kPlot, itPlayer->getStartingPlot());
		if (iDist == 0) // Don't include the original sites themselves
			return;
		if (!kPlot.isArea(itPlayer->getStartingPlot()->getArea()))
			iDist += 100;
		if (iDist < iShortestDist)
		{
			iShortestDist = iDist;
			eClosestPlayer = itPlayer->getID();
		}
	}
	map<PlayerTypes,VoronoiCell*>::iterator pos = m_sitesClosestToOriginalSite.
			find(eClosestPlayer);
	VoronoiCell& kCell = *pos->second;
	PlotNumTypes ePlotNum = kMap.plotNum(kPlot);
	if (bAdd)
		kCell.insert(ePlotNum);
	else
	{
		VoronoiCell::iterator pos = kCell.find(ePlotNum);
		if (pos != kCell.end())
			kCell.erase(pos);
		else FAssert(pos != kCell.end())
	}
}


int StartingPositionIteration::PotentialSiteGen::fewestPotentialSites() const
{
	int r = MAX_INT;
	for (map<PlayerTypes,VoronoiCell*>::const_iterator it =
		m_sitesClosestToOriginalSite.begin();
		it != m_sitesClosestToOriginalSite.end(); ++it)
	{
		r = std::min(r, (int)it->second->size());
	}
	return r;
}


void StartingPositionIteration::PotentialSiteGen::removeOriginalSites()
{
	CvMap const& kMap = GC.getMap();
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		map<PlotNumTypes,short>::iterator pos = m_foundValuesPerSite.find(
				kMap.plotNum(*it->getStartingPlot()));
		if (pos != m_foundValuesPerSite.end())
			m_foundValuesPerSite.erase(pos);
	}
}


void StartingPositionIteration::PotentialSiteGen::getPlots(
	vector<CvPlot const*>& r) const
{
	FAssert(r.empty());
	CvMap const& kMap = GC.getMap();
	for (map<PlotNumTypes,short>::const_iterator it = m_foundValuesPerSite.begin();
		it != m_foundValuesPerSite.end(); ++it)
	{
		r.push_back(&kMap.getPlotByIndex(it->first));
	}
}


StartingPositionIteration::VoronoiCell* StartingPositionIteration::
	PotentialSiteGen::getCell(PlayerTypes eOriginalSite) const
{
	map<PlayerTypes,VoronoiCell*>::const_iterator pos = m_sitesClosestToOriginalSite.
			find(eOriginalSite);
	if (pos == m_sitesClosestToOriginalSite.end())
	{
		FAssertMsg(false, "No original site found for given player");
		return NULL;
	}
	return pos->second;
}


void StartingPositionIteration::PotentialSiteGen::getCurrentFoundValues(
	EnumMap<PlayerTypes,short>& kFoundValuesPerPlayer) const
{
	CvMap const& kMap = GC.getMap();
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		map<PlotNumTypes,short>::const_iterator pos = m_foundValuesPerSite.
				find(kMap.plotNum(*it->getStartingPlot()));
		if (pos != m_foundValuesPerSite.end())
			kFoundValuesPerPlayer.set(it->getID(), pos->second);
	}
}


StartingPositionIteration::DistanceTable::DistanceTable(
	vector<CvPlot const*>& kapSources, vector<CvPlot const*>& kapDestinations)
{
	CvMap const& kMap = GC.getMap();
	m_sourceIDs.resize(kMap.numPlots(), NOT_A_SOURCE);
	for (size_t i = 0; i < kapSources.size(); i++)
	{
		m_sourceIDs[kMap.plotNum(*kapSources[i])] = (SourceID)i;
	}
	m_destinationIDs.resize(kMap.numPlots(), NOT_A_DESTINATION);
	for (size_t i = 0; i < kapDestinations.size(); i++)
	{
		m_destinationIDs[kMap.plotNum(*kapDestinations[i])] = (DestinationID)i;
	}
	m_distances.resize(kapSources.size(),
			vector<short>(kapDestinations.size(), MAX_SHORT));
	for (size_t i = 0; i < kapSources.size(); i++)
	{
		computeDistances(*kapSources[i]);
	}
}


short StartingPositionIteration::DistanceTable::d(
	CvPlot const& kSource, CvPlot const& kDestination) const
{
	CvMap const& kMap = GC.getMap();
	SourceID eSource = m_sourceIDs[kMap.plotNum(kSource)];
	DestinationID eDestination = m_destinationIDs[kMap.plotNum(kSource)];
	FAssertBounds(0, m_distances.size(), eSource);
	FAssertBounds(0, m_distances[eSource].size(), eDestination);
	return m_distances[eSource][eDestination];
}


void StartingPositionIteration::DistanceTable::setDistance(
	CvPlot const& kSource, CvPlot const& kDestination, short iDistance)
{
	CvMap const& kMap = GC.getMap();
	SourceID eSource = m_sourceIDs[kMap.plotNum(kSource)];
	DestinationID eDestination = m_destinationIDs[kMap.plotNum(kDestination)];
	if (eDestination != NOT_A_DESTINATION)
	{
		FAssertBounds(0, m_distances.size(), eSource);
		FAssert(eDestination < (int)m_distances[eSource].size());
		m_distances[eSource][eDestination] = iDistance;
	}
}


namespace
{
	class Node
	{
		// No reference b/c that would block the implicit assignment operator
		CvPlot const* m_pPlot;
		short m_iDistance;
	public:
		Node(CvPlot const& kPlot, int iDistance) : m_pPlot(&kPlot), m_iDistance(iDistance) {}
		CvPlot const& get() const { return *m_pPlot; }
		short getDistance() const { return m_iDistance; }
		// For inverse ordering by distance
		bool operator<(Node const& kOther) const { return m_iDistance > kOther.m_iDistance; }
	};
}

void StartingPositionIteration::DistanceTable::computeDistances(CvPlot const& kSource)
{
	CvMap const& kMap = GC.getMap();
	// Plain old Dijkstra's algorithm
	std::priority_queue<Node> q;
	q.push(Node(kSource, 0));
	/*	Keep track of visited nodes in order to save time. Can't use m_distances
		for this b/c it only contains destinations; may have to visit all plots. */
	vector<bool> abReached(kMap.numPlots(), false);
	while (!q.empty())
	{
		Node v = q.top();
		q.pop();
		CvPlot const& kAt = v.get();
		setDistance(kSource, kAt, v.getDistance());
		{
			int iAt = kMap.plotNum(kAt);
			if (abReached[iAt])
				continue;
			abReached[iAt] = true;
		}
		FOR_EACH_ENUM(Direction)
		{
			CvPlot const* pAdj = kMap.plotDirection(
					kAt.getX(), kAt.getY(), eLoopDirection);
			if (pAdj == NULL ||
				/*	Note: even if not yet reached, it may already be in q;
					that's what the abReached[iAt] check above is for. */
				abReached[kMap.plotNum(*pAdj)])
			{
				continue;
			}
			short iDistance = stepDist(kAt, *pAdj);
			if (iDistance < MAX_SHORT)
				q.push(Node(*pAdj, v.getDistance() + iDistance));
		}
		/*	Might be more efficient to call std::make_heap once after pushing
			to the back of a vector (i.e. no priority_queue). Hard to say. */
	}
}


short StartingPositionIteration::DistanceTable::stepDist(
	CvPlot const& kFrom, CvPlot const& kTo)
{
	static const TerrainTypes eShallowTerrain = GC.getWATER_TERRAIN(true);
	if (kTo.isImpassable() || GC.getMap().waterStepInvalid(kFrom, kTo))
		return MAX_SHORT;

	bool const bDiagonal = (kFrom.getX() != kTo.getX() && kFrom.getY() != kTo.getY());
	if (!kTo.isWater()) // Land to land, water to land
		return (bDiagonal ? 12 : 9);
	if (!kFrom.isWater()) // Land to water
	{
		if (kTo.getTerrainType() == eShallowTerrain)
			return 125;
		return 250; // Land to ocean - can't normally be adjacent though
	}
	else if (kFrom.getTerrainType() == eShallowTerrain)
	{
		if (kTo.getTerrainType() == eShallowTerrain) // Shallow to shallow
			return 5;
		/*	Shallow to ocean: Here we could be more sophisticated by
			shifting some of the cost to a case
			'ocean adjacent to shallow water -> ocean not adjacent to shallow water'
			-- to account for the possibility of border spread. Might be too slow. */
		return 125;
	}
	return 4; // Ocean to water (shallow or deep)
}


void StartingPositionIteration::computeStartValues(
	EnumMap<PlayerTypes,short> const& kFoundValues,
	EnumMap<PlotNumTypes,scaled> const& kYieldValues,
	DistanceTable const& kPathDists,
	EnumMap<PlayerTypes,scaled>& kStartValues)
{
	
}
