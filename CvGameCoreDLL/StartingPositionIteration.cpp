// advc.027: New file; see comment in header.

#include "CvGameCoreDLL.h"
#include "StartingPositionIteration.h"
#include "CvPlayerAI.h"
#include "CvGamePlay.h"
#include "CitySiteEvaluator.h"
#include "CvMap.h"
#include "CvArea.h"
#include "PlotRadiusIterator.h"
#include "CityPlotIterator.h"
#include "CvInfo_GameOption.h" // for StartingLocPercent handicap

using std::map;
using std::vector;
using std::make_pair;

//#define SPI_LOG // Enables log file for starting position iteration

bool StartingPositionIteration::isDebug()
{
	#ifdef SPI_LOG
		return true;
	#else
		return false;
	#endif
}


StartingPositionIteration::StartingPositionIteration() :
	m_bRestrictedAreas(false), m_bNormalizationTargetReady(false)
{
	if (!GC.getDefineBOOL("ENABLE_STARTING_POSITION_ITERATION"))
		return;
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
	bool const bIgnoreScript = GC.getInitCore().isPangaea();
	size_t i = 0;
	for (; i < civPlayers.size(); i++)
	{
		CvPlayer& kPlayer = *civPlayers[i];
		if (kPlayer.getStartingPlot() != NULL)
		{
			/*	Meaning that, if a map script sets all starting sites in
				assignStartingPlots but still allows the default implementation,
				then starting position iteration is allowed but can't change
				the start areas. (for PerfectMongoose) */
			m_bRestrictedAreas = true;
			continue;
		}
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

	m_pEval = createSiteEvaluator();
	PotentialSites potentialSiteGen(*m_pEval, m_bRestrictedAreas);
	if (potentialSiteGen.numSites() < civPlayers.size())
		return;
	/*for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it) // (debug) Mark original sites with a ruin
		it->getStartingPlot()->setImprovementType(GC.getRUINS_IMPROVEMENT());*/
	// Remove the current solution. Just want the alternatives.
	potentialSiteGen.updateCurrSites();
	vector<CvPlot const*> aPotentialSites;
	potentialSiteGen.getPlots(aPotentialSites);
	/*for (size_t i = 0; i < aPotentialSites.size(); i++) // (debug) Mark alt. sites with a ruin
		GC.getMap().getPlotByIndex(GC.getMap().plotNum(*aPotentialSites[i])).setImprovementType(GC.getRUINS_IMPROVEMENT());*/

	EnumMap<PlotNumTypes,scaled> yieldValues;
	std::map<CvArea const*,scaled> yieldsPerArea;
	{
		vector<scaled> arLandYields;
		CitySiteEvaluator yieldEval(m_pEval->getPlayer(), -1, /*bStartingLoc=*/false, true);
		FOR_EACH_ENUM(PlotNum)
		{
			CvPlot const& kPlot = kMap.getPlotByIndex(eLoopPlotNum);
			scaled rYieldVal = yieldEval.evaluateWorkablePlot(kPlot);
			// CitySiteEvaluator has many useful subroutines for this; handle it there.
			yieldValues.set(eLoopPlotNum, rYieldVal);
			/*if(yieldValues.get(eLoopPlotNum) > 0) // (debug) Mark nonnegative-value plots with a ruin
				GC.getMap().getPlotByIndex(i).setImprovementType(GC.getRUINS_IMPROVEMENT());*/
			// Might possibly overflow w/o the divisor. Don't care about the scale anyway.
			rYieldVal /= 32;
			yieldsPerArea[kPlot.area()] += rYieldVal;
			if (!kPlot.isWater())
				arLandYields.push_back(rYieldVal);
		}
		m_rMedianLandYieldVal = stats::median(arLandYields);
	}

	DistanceTable const* pathDists = NULL;
	{	// Temp data that I want to go out of scope
		// Need a vector of all sites. Add the original sites temporarily.
		for (size_t i = 0; i < civPlayers.size(); i++)
		{
			aPotentialSites.push_back(civPlayers[i]->getStartingPlot());
		}
		// Also want inter-site distances
		std::set<PlotNumTypes> sitePlotNums;
		for (size_t i = 0; i < aPotentialSites.size(); i++)
		{
			sitePlotNums.insert(kMap.plotNum(*aPotentialSites[i]));
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
		pathDists = new DistanceTable(aPotentialSites, relevantPlots);
		gDLL->callUpdater();
		// Remove the original sites again
		for (size_t i = 0; i < civPlayers.size(); i++)
			aPotentialSites.pop_back();
	}
	// So that subroutines can read - but not write - these
	m_pYieldValues = &yieldValues;
	m_pYieldsPerArea = &yieldsPerArea;
	m_pPathDists = pathDists;
	m_pPotentialSites = &potentialSiteGen;

	doIterations(potentialSiteGen); // May modify the potential sites
	delete pathDists;
	m_pPathDists = NULL;
}


StartingPositionIteration::~StartingPositionIteration()
{
	SAFE_DELETE(m_pEval);
}


NormalizationTarget* StartingPositionIteration::createNormalizationTarget() const
{
	if (!m_bNormalizationTargetReady)
		return NULL;
	NormalizationTarget& kResult = *new NormalizationTarget(
			*createSiteEvaluator(true), m_currSolutionAttribs);
	return &kResult;
}

// Allocates memory
CitySiteEvaluator* StartingPositionIteration::createSiteEvaluator(bool bNormalize) const
{
	/*	Teams have already received free tech at this point, so the player used
		for city site evaluation isn't irrelevant. (Its personality should be though.)
		Pick the player with the least tech, prefer human when breaking ties.
		But no 1-city challenge players! */
	bool const bOCC = GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE);
	CvPlayerAI const* pPlayer = NULL;
	int iBestPlayerVal = MAX_INT;
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		int iVal = 10 * it->getTechScore();
		if (it->isHuman())
		{
			if (bOCC)
				iVal = MAX_INT;
			iVal--;
		}
		if (iVal < iBestPlayerVal)
		{
			iBestPlayerVal = iVal;
			pPlayer = &*it;
		}
	}
	CitySiteEvaluator* r = new CitySiteEvaluator(*pPlayer, -1, !bNormalize, bNormalize);
	// Evaluating the surroundings will be our job
	r->setIgnoreStartingSurroundings(true);
	return r;
}


StartingPositionIteration::PotentialSites::PotentialSites(
	CitySiteEvaluator const& kEval, bool bRestrictedAreas) :
	m_kEval(kEval), m_pVicinityPenaltiesPerPlot(NULL)
{
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		m_sitesClosestToCurrSite.insert(make_pair(
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
		{
			// Randomize a little - mainly so that settling in place isn't a no-brainer
			iFoundValue = (iFoundValue + iFoundValue *
					((iFoundValue / rMinFoundVal - 1) * fixp(0.05) *
					(scaled::rand(GC.getGame().getMapRand(), NULL) - fixp(0.5)))).round();
			m_foundValuesPerSite.insert(make_pair(eLoopPlotNum, iFoundValue));
		}
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


StartingPositionIteration::PotentialSites::~PotentialSites()
{
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		delete m_sitesClosestToCurrSite[it->getID()];
	}
}


scaled StartingPositionIteration::PotentialSites::computeMinFoundValue()
{
	vector<short> aiFoundValuesPerCurrSite; // vector for stats::median
	short iWorstCurrFoundVal = MAX_SHORT;
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		short iFoundVal = m_kEval.evaluate(*it->getStartingPlot());
		aiFoundValuesPerCurrSite.push_back(iFoundVal);
		m_foundValuesPerCurrSite.set(it->getID(), iFoundVal);
		iWorstCurrFoundVal = std::min(iWorstCurrFoundVal, iFoundVal);
	}
	scaled r = iWorstCurrFoundVal * fixp(0.88);
	r.increaseTo(stats::median(aiFoundValuesPerCurrSite) * fixp(0.7));
	return scaled::max(r, 1);
}

/*	(Could get the found value from m_foundValuesPerSite, but the callers
	happen to have it at hand.) */
void StartingPositionIteration::PotentialSites::recordSite(
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
	PlayerTypes eClosestPlayer = closestPlayer(kPlot);
	if (eClosestPlayer == NO_PLAYER) // Remote sites will be set by updateCurrSites
		return;
	map<PlayerTypes,VoronoiCell*>::iterator pos = m_sitesClosestToCurrSite.
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
		else FAssert(pos != kCell.end());
	}
}


PlayerTypes StartingPositionIteration::PotentialSites::closestPlayer(
	CvPlot const& kPlot) const
{
	PlayerTypes eClosestPlayer = NO_PLAYER;
	CvMap const& kMap = GC.getMap();
	int iShortestDist = MAX_INT;
	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		int iDist = kMap.stepDistance(&kPlot, itPlayer->getStartingPlot());
		if (iDist <= 0) // Don't include the current sites themselves in Voronoi cells
			return NO_PLAYER;
		if (!kPlot.isArea(itPlayer->getStartingPlot()->getArea()))
			iDist += 25;
		if (iDist < iShortestDist)
		{
			iShortestDist = iDist;
			eClosestPlayer = itPlayer->getID();
		}
	}
	if (iShortestDist > 30) // remote site
		return NO_PLAYER;
	return eClosestPlayer;
}


int StartingPositionIteration::PotentialSites::fewestPotentialSites() const
{
	int r = MAX_INT;
	for (map<PlayerTypes,VoronoiCell*>::const_iterator it =
		m_sitesClosestToCurrSite.begin();
		it != m_sitesClosestToCurrSite.end(); ++it)
	{
		r = std::min(r, (int)it->second->size());
	}
	return r;
}


void StartingPositionIteration::PotentialSites::updateCurrSites(bool bUpdateCells)
{
	CvMap const& kMap = GC.getMap();
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		map<PlotNumTypes,short>::iterator pos = m_foundValuesPerSite.find(
				kMap.plotNum(*it->getStartingPlot()));
		if (pos != m_foundValuesPerSite.end())
		{
			m_foundValuesPerCurrSite.set(it->getID(), pos->second);
			m_foundValuesPerSite.erase(pos);
		} /* If a player's current start site wasn't in foundValuesPerSite,
			 then that player's start site hasn't moved; no update needed. */
	}
	if (bUpdateCells)
	{
		for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
		{
			m_sitesClosestToCurrSite[it->getID()]->clear();
		}
	}
	m_remoteSitesByAreaSize.clear();
	for (std::map<PlotNumTypes,short>::const_iterator it = m_foundValuesPerSite.begin();
		it != m_foundValuesPerSite.end(); ++it)
	{
		PlotNumTypes ePlotNum = it->first;
		CvPlot const& kPlot = kMap.getPlotByIndex(ePlotNum);
		PlayerTypes eClosestPlayer = closestPlayer(kPlot);
		if (eClosestPlayer == NO_PLAYER)
		{
			m_remoteSitesByAreaSize.push_back(make_pair(
					kPlot.getArea().getNumTiles() * 100 + it->second, ePlotNum));
		}
		else if (bUpdateCells)
			m_sitesClosestToCurrSite[eClosestPlayer]->insert(ePlotNum);
	}
	std::sort(m_remoteSitesByAreaSize.rbegin(), m_remoteSitesByAreaSize.rend());
}


void StartingPositionIteration::PotentialSites::getPlots(
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
	PotentialSites::getCell(PlayerTypes eCurrSite) const
{
	map<PlayerTypes,VoronoiCell*>::const_iterator pos = m_sitesClosestToCurrSite.
			find(eCurrSite);
	if (pos == m_sitesClosestToCurrSite.end())
	{
		FAssertMsg(false, "No current site found for given player");
		return NULL;
	}
	return pos->second;
}


PlotNumTypes StartingPositionIteration::PotentialSites::getRemoteSite(
	int iIndex) const
{
	if (iIndex >= (int)m_remoteSitesByAreaSize.size())
		return NO_PLOT_NUM;
	return m_remoteSitesByAreaSize[iIndex].second;
}


void StartingPositionIteration::PotentialSites::getCurrFoundValues(
	EnumMap<PlayerTypes,short>& kFoundValuesPerPlayer) const
{
	CvMap const& kMap = GC.getMap();
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		map<PlotNumTypes,short>::const_iterator pos = m_foundValuesPerSite.
				find(kMap.plotNum(*it->getStartingPlot()));
		if (pos != m_foundValuesPerSite.end())
			kFoundValuesPerPlayer.set(it->getID(), pos->second);
		else
		{
			short iCurrFoundVal = m_foundValuesPerCurrSite.get(it->getID());
			if (iCurrFoundVal > 0)
				kFoundValuesPerPlayer.set(it->getID(), iCurrFoundVal);
		}
	}
}


StartingPositionIteration::DistanceTable::DistanceTable(
	vector<CvPlot const*>& kapSources, vector<CvPlot const*>& kapDestinations)
{
	scaled rStartEraFactor = 1;
	if (GC.getNumEraInfos() > 1)
	{
		rStartEraFactor = scaled(GC.getGame().getStartEra(), GC.getNumEraInfos() - 1);
		rStartEraFactor = 1 - rStartEraFactor.sqrt() / 2;
	}
	m_iFirstFrontierCost = (short)(150 * rStartEraFactor).round();
	m_iSecondFrontierCost = (short)(155 * rStartEraFactor.pow(fixp(1.5))).round();

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
		CvPlot const& kSource = *kapSources[i];
		computeDistances(kSource);
		/*	Destinations can be (potential) city sites or workable tiles.
			Water destinations are never city sites. To work a water tile,
			no ships are needed, so the frontier penalties applied by
			computeDistance are far too high. (computeDistances can't sort
			this out b/c it doesn't distinguish between moving a settler
			into/ through a plot and working a plot.) */
		for (size_t i = 0; i < kapDestinations.size(); i++)
		{
			CvPlot const& kWaterDest = *kapDestinations[i];
			if (!kWaterDest.isWater())
				continue;
			/*	Use the distance of the land destination closest to kSource
				that can work kDest. Bad land tiles aren't stored in the
				DistanceTable, so this will fail for e.g. an Ocean Fish workable
				only from a Snow tile. Fall back on CvMap::plotDist and
				CvPlot::sameArea in that case. */
			CvPlot const* pNearestLand = NULL;
			short iShortestDist = MAX_SHORT;
			bool bDest = false;
			bool bInnerRing = false;
			for (CityPlotIter it(kWaterDest, false); it.hasNext(); ++it)
			{
				if (!it->canFound())
					continue;
				bool bInnerRingLoop = (it.currID() < NUM_INNER_PLOTS);
				if (m_destinationIDs[kMap.plotNum(*it)] != NOT_A_DESTINATION)
				{
					short iDist = d(kSource, *it);
					if (!bDest || iDist < iShortestDist)
					{
						bDest = true;
						iShortestDist = iDist;
						pNearestLand = &*it;
						bInnerRing = bInnerRingLoop;
					}
				}
				else if (!bDest && it->sameArea(kSource))
				{
					short iDist = static_cast<short>(kMap.plotDistance(&kSource, &*it));
					if (iDist < iShortestDist)
					{
						iShortestDist = iDist;
						pNearestLand = &*it;
						bInnerRing = bInnerRingLoop;
					}
				}
			}
			if (pNearestLand != NULL)
			{	// To account for the distance between pNearest and kWaterDest
				iShortestDist += getAvgCityDist() / 5;
				if (!bInnerRing)
					iShortestDist += getAvgCityDist() / 4;
				// Sea improvements are costlier than land improvements
				if (kWaterDest.getBonusType(NO_TEAM) != NO_BONUS)
					iShortestDist += (5 * getAvgCityDist()) / 8;
				setDistance(kSource, kWaterDest, iShortestDist);
			}
		}
	}
}


short StartingPositionIteration::DistanceTable::d(
	CvPlot const& kSource, CvPlot const& kDestination) const
{
	FAssertMsg(!kSource.isWater(), "kSource should be a (potential) city site");
	CvMap const& kMap = GC.getMap();
	SourceID eSource = m_sourceIDs[kMap.plotNum(kSource)];
	DestinationID eDestination = m_destinationIDs[kMap.plotNum(kDestination)];
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
	bool const bSourceCoastal = kSource.isCoastalLand(-1);
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
			short iDistance = stepDist(kAt, *pAdj, bSourceCoastal);
			if (iDistance < MAX_SHORT)
				q.push(Node(*pAdj, v.getDistance() + iDistance));
		}
		/*	Might be more efficient to call std::make_heap once after pushing
			to the back of a vector (i.e. no priority_queue). Hard to say. */
	}
}

/*	If the scale of the distances is changed, the value returned by
	getAvgCityDist needs to be adjusted. */
short StartingPositionIteration::DistanceTable::stepDist(
	CvPlot const& kFrom, CvPlot const& kTo, bool bSourceCoastal) const
{
	static const TerrainTypes eShallowTerrain = GC.getWATER_TERRAIN(true);
	if (kTo.isImpassable() || GC.getMap().isSeparatedByIsthmus(kFrom, kTo))
		return MAX_SHORT;

	// Land to shallow water and shallow to deep water are frontiers
	int const iFrontierAdjustment = (bSourceCoastal ? -1 : 1) * 10;
	bool const bDiagonal = (kFrom.getX() != kTo.getX() && kFrom.getY() != kTo.getY());
	if (!kTo.isWater()) // Land to land, water to land
		return (bDiagonal ? 12 : 9);
	if (!kFrom.isWater()) // Land to water
	{
		if (kTo.getTerrainType() == eShallowTerrain)
			return m_iFirstFrontierCost + iFrontierAdjustment;
		// (Land to ocean. Can't normally be adjacent.)
		return m_iFirstFrontierCost + m_iSecondFrontierCost + 2 * iFrontierAdjustment;
	}
	else if (kFrom.getTerrainType() == eShallowTerrain)
	{
		if (kTo.getTerrainType() == eShallowTerrain) // Shallow to shallow
			return 5;
		/*	Shallow to ocean: Here we could be more sophisticated by
			shifting some of the cost to a case
			'ocean adjacent to shallow water -> ocean not adjacent to shallow water'
			-- to account for the possibility of border spread. Might be too slow. */
		return m_iSecondFrontierCost + iFrontierAdjustment;
	}
	return 4; // Ocean to water (shallow or deep)
}


StartingPositionIteration::Step::Step()
{
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		m_originalPosition.push_back(make_pair(
				it->getID(), it->getStartingPlot()));
	}
}


void StartingPositionIteration::Step::move(PlayerTypes ePlayer, CvPlot& kTo)
{
	m_moves.push_back(std::make_pair(ePlayer, &kTo));
}


void StartingPositionIteration::Step::take()
{
	for (size_t i = 0; i < m_moves.size(); i++)
	{
		GET_PLAYER(m_moves[i].first).setStartingPlot(
				m_moves[i].second, false);
	}
}


void StartingPositionIteration::Step::takeBack()
{
	for (size_t i = 0; i < m_originalPosition.size(); i++)
	{
		GET_PLAYER(m_originalPosition[i].first).setStartingPlot(
				m_originalPosition[i].second, false);
	}
}

PlayerTypes StartingPositionIteration::Step::getFirstMovePlayer() const
{
	if (m_moves.empty())
		return NO_PLAYER;
	return m_moves[0].first;
}

// (Overloading operator<< for a nested private class would be messy)
std::string StartingPositionIteration::Step::debugStr() const
{
	std::ostringstream r;
	for (size_t i = 0; i < m_moves.size(); i++)
	{
		CvPlot const* pTo = m_moves[i].second;
		CvPlot const* pFrom = NULL;
		for (size_t j = 0; j < m_originalPosition.size(); j++)
		{
			if (m_originalPosition[j].first == m_moves[i].first)
			{
				pFrom = m_originalPosition[j].second;
				break;
			}
		}
		if (pFrom == NULL)
		{
			FAssert(pFrom != NULL);
			continue;
		}
		r << "Move player " << (int)m_moves[i].first << " from (" <<
				pFrom->getX() << "," << pFrom->getY() << ") to (" <<
				pTo->getX() << "," << pTo->getY() << ")";
		if (!pFrom->sameArea(*pTo))
			r << " [different landmass]";
		r << "\n";
	}
	return r.str();
}


void StartingPositionIteration::evaluateCurrPosition(
	SolutionAttributes& kResult, bool bLog) const
{
	EnumMap<PlayerTypes,short> foundValues;
	m_pPotentialSites->getCurrFoundValues(foundValues);
	computeStartValues(foundValues, kResult, bLog);
	kResult.m_rStartPosVal = startingPositionValue(kResult);
}

//#define DEBUG_SPACE_BREAKDOWN
#ifdef DEBUG_SPACE_BREAKDOWN
struct BreakDownData { PlotNumTypes ePlot; scaled rYieldVal; scaled rAccessFactor;
	BreakDownData(PlotNumTypes ePlot, scaled rYieldVal, scaled rAccessFactor) :
	ePlot(ePlot), rYieldVal(rYieldVal), rAccessFactor(rAccessFactor) {}
	bool operator<(BreakDownData const& kOther) const{ return ePlot < kOther.ePlot; } };
#endif

// The results are on the scale of AIFoundValue
void StartingPositionIteration::computeStartValues(
	EnumMap<PlayerTypes,short> const& kFoundValues, SolutionAttributes& kResult,
	bool bLog) const
{
	#ifdef SPI_LOG
		std::ostringstream out;
		if (bLog)
			out << "Evaluating starting position ...\n\n";
	#endif
	CvMap const& kMap = GC.getMap();
	typedef ScaledNum<1024*32> claim_t; // Inverted distances - pretty small
	EnumMap<PlotNumTypes,claim_t> sumOfClaims;
	short const iAvgCityDist = m_pPathDists->getAvgCityDist(); // for scale adjustment
	/*	Essentially measures distances from the edge of the starting city radius
		instead of the starting city tile */
	short const iDistSubtr = iAvgCityDist / 2;
	FOR_EACH_ENUM(PlotNum)
	{
		if (m_pYieldValues->get(eLoopPlotNum) <= 0)
			continue;
		for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
		{
			CvPlot const& kStartPlot = *it->getStartingPlot();
			CvPlot const& kLoopPlot = kMap.getPlotByIndex(eLoopPlotNum);
			if (kMap.plotDistance(&kStartPlot, &kLoopPlot) <= 2)
			{
				// Accounted for by found values
				sumOfClaims.set(eLoopPlotNum, 0);
				break;
			}
			short iDist = m_pPathDists->d(kStartPlot, kLoopPlot);
			if (iDist == MAX_SHORT)
				continue;
			iDist = std::max(1, iDist - iDistSubtr);
			sumOfClaims.add(eLoopPlotNum, claim_t(1, iDist));
		}
	}

	/*	Compute these upfront b/c it depends on the typical distance between
		starting sites whether a given distance is "too close" */
	EnumMap<PlayerTypes,scaled> rivalDistFactors;
	scaled const rTypicalDistFactor = computeRivalDistFactors(rivalDistFactors, false);
	// Need this once for all rivals and once just for those in the same area
	EnumMap<PlayerTypes,scaled> sameAreaRivalDistFactors;
	scaled rSameAreaTargetDistFactor = computeRivalDistFactors(
			sameAreaRivalDistFactors, true) * fixp(4/3.);

	// Again, need to know the distribution before computing the actual per-player values.
	vector<scaled> arAreaYieldsPerPlayer;
	EnumMap<PlayerTypes,scaled> areaYieldsPerPlayer;
	EnumMap<PlayerTypes,scaled> spaceValues;
	int const iCivEverAlive = PlayerIter<CIV_ALIVE>::count();
	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		scaled rSpaceVal;
		CvPlot const& kStartPlot = *itPlayer->getStartingPlot();
		#ifdef DEBUG_SPACE_BREAKDOWN
			vector<std::pair<scaled,BreakDownData> > aSpaceBreakDown;
		#endif
		FOR_EACH_ENUM(PlotNum)
		{
			claim_t rTotal = sumOfClaims.get(eLoopPlotNum);
			if (rTotal <= 0)
				continue;
			short iStartDist = m_pPathDists->d(
					kStartPlot, kMap.getPlotByIndex(eLoopPlotNum));
			if (iStartDist == MAX_SHORT)
				continue;
			short iDist = std::max(1, iStartDist - iDistSubtr);
			claim_t rAccessFactor = 1 / (rTotal * iDist);
			/*	So far, it's proportional to distance. While this encourages
				starting positions that at least give everyone a chance, it still
				assigns too much value to contested plots, especially to plots
				that are close to multiple starting sites (high rTotal).
				On the higher difficulty levels, humans usually can't settle
				their 2nd and 3rd cities faster than the AI. */
			rAccessFactor.exponentiate(fixp(1.85));
			/*	Apart from competition, nearby plots are better than remote ones.
				Divisor d^(d/750) -- weird formula, but seems to result in the kind
				of hyperbola I had in mind. */
			rAccessFactor /= scaled(iDist).pow(scaled(iDist, 750));
			/*	For uncontested plots very close to kStartPlot, the exact distance
				shouldn't matter. */
			rAccessFactor.decreaseTo(1);
			scaled rYieldVal = m_pYieldValues->get(eLoopPlotNum);
			rSpaceVal += rYieldVal * rAccessFactor;
			#ifdef DEBUG_SPACE_BREAKDOWN
				aSpaceBreakDown.push_back(make_pair(scaled(rYieldVal*rAccessFactor),BreakDownData(eLoopPlotNum,rYieldVal,rAccessFactor)));
			#endif
		}
		#ifdef DEBUG_SPACE_BREAKDOWN
		out << "\nSpace breakdown for player" << itPlayer->getID() << "\n";
		std::sort(aSpaceBreakDown.rbegin(),aSpaceBreakDown.rend());
		scaled rSumZero, rSumOne, rSumTwo, rSum3to9;
		for(size_t i = 0; i < aSpaceBreakDown.size(); i++) {
			if(aSpaceBreakDown[i].first.round() > 1)
				out << aSpaceBreakDown[i].first.round() << "="<< aSpaceBreakDown[i].second.rYieldVal.round() << "*" << aSpaceBreakDown[i].second.rAccessFactor.str(100) <<" (" << kMap.getPlotByIndex(aSpaceBreakDown[i].second.ePlot).getX() << "," << kMap.getPlotByIndex(aSpaceBreakDown[i].second.ePlot).getY() << ")\n";
			if(aSpaceBreakDown[i].first.round() == 0)
				rSumZero += aSpaceBreakDown[i].first;
			else if(aSpaceBreakDown[i].first.round() == 1)
				rSumOne += aSpaceBreakDown[i].first;
			else if(aSpaceBreakDown[i].first.round() == 2)
				rSumTwo += aSpaceBreakDown[i].first;
			else if(aSpaceBreakDown[i].first.round() < 10)
				rSum3to9 += aSpaceBreakDown[i].first;
		}
		out << "Near-0 space values sum up to " << rSumZero.round() << "\n";
		out << "Near-1 space values sum up to " << rSumOne.round() << "\n";
		out << "Near-2 space values sum up to " << rSumTwo.round() << "\n";
		out << "(3,9)-space values sum up to " << rSum3to9.round() << "\n";
		out << "All the rest sums up to " << (rSpaceVal - rSumZero - rSumOne - rSumTwo - rSum3to9).round() << "\n";
		#endif
		/*	Area size needs to be taken into account explicitly b/c empires spread
			across multiple areas are difficult to defend and pay colony maintenance */
		scaled rAreaYields;
		std::map<CvArea const*,scaled>::const_iterator pos = 
				m_pYieldsPerArea->find(kStartPlot.area());
		if (pos != m_pYieldsPerArea->end())
			rAreaYields = pos->second;
		int iSameAreaRivals = 0;
		for (PlayerIter<CIV_ALIVE,NOT_SAME_TEAM_AS> itRival(itPlayer->getTeam());
			itRival.hasNext(); ++itRival)
		{
			if (kStartPlot.sameArea(*itRival->getStartingPlot()))
				iSameAreaRivals++;
		}
		rAreaYields /= 1 + iSameAreaRivals;
		areaYieldsPerPlayer.set(itPlayer->getID(), rAreaYields);
		arAreaYieldsPerPlayer.push_back(rAreaYields);
		spaceValues.set(itPlayer->getID(), rSpaceVal);
	}

	vector<scaled> arFoundValues;
	vector<scaled> arSpaceValues;
	scaled const rMedianAreaYield = scaled::max(stats::median(arAreaYieldsPerPlayer), 1);
	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		scaled rAreaYields = areaYieldsPerPlayer.get(itPlayer->getID());
		scaled rRatio = rAreaYields / rMedianAreaYield;
		scaled rTolerance = fixp(0.23);
		if (rRatio + rTolerance < 1)
			spaceValues.multiply(itPlayer->getID(), rRatio + rTolerance);
		/*	If possible, we want there to be room for the core cities in
			the player's starting area. */
		scaled rTargetMinYields = GC.getInfo(kMap.getWorldSize()).
				getTargetNumCities() * fixp(0.78) * m_rMedianLandYieldVal * NUM_CITY_PLOTS;
		// Adjust to crowdedness
		rTargetMinYields *= scaled(
				GC.getGame().getRecommendedPlayers(), iCivEverAlive).sqrt();
		// No colony maintenance if vassals are disabled
		if (GC.getGame().isOption(GAMEOPTION_NO_VASSAL_STATES))
			rTargetMinYields *= fixp(0.82);
		rRatio = rAreaYields / rTargetMinYields;
		if (rRatio < 1)
			spaceValues.multiply(itPlayer->getID(), rRatio.pow(fixp(0.36)));
		arFoundValues.push_back(kFoundValues.get(itPlayer->getID()));
		arSpaceValues.push_back(spaceValues.get(itPlayer->getID()));
	}
	
	scaled const rMedianFoundValue = stats::median(arFoundValues);
	scaled const rMedianSpaceValue = stats::median(arSpaceValues);

	kResult.m_volatilityValues.reset();
	CvGame const& kGame = GC.getGame();
	EraTypes const eStartEra = kGame.getStartEra();
	/*	Adjust this to set the overall balance between city radius
		and space for expansion */
	scaled rBaseSpaceWeight = fixp(1.6);
	if (eStartEra <= 1)
	{
		if (kGame.isOption(GAMEOPTION_NO_BARBARIANS))
			rBaseSpaceWeight *= fixp(1.025);
		else if (kGame.isOption(GAMEOPTION_RAGING_BARBARIANS))
			rBaseSpaceWeight *= fixp(0.95);
	}
	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		scaled rFoundValue = kFoundValues.get(itPlayer->getID());
		scaled rSpaceWeight = rBaseSpaceWeight;
		if (rMedianSpaceValue > 0)
			rSpaceWeight *= rMedianFoundValue / rMedianSpaceValue;
		scaled rFoundWeight = 1;
		scaled rFromSpaceValue = spaceValues.get(itPlayer->getID());
		if (rMedianFoundValue > 0 && rMedianSpaceValue > 0 &&
			rFoundValue > 0 && rFromSpaceValue > 0)
		{	// Not healthy if these are far apart
			scaled rFoundRatio = rFoundValue / rMedianFoundValue;
			scaled rSpaceRatio = rFromSpaceValue / rMedianSpaceValue;
			scaled rSmaller = std::min(rFoundRatio, rSpaceRatio);
			scaled rGreater = std::max(rFoundRatio, rSpaceRatio);
			if (rGreater > 0)
			{
				kResult.m_volatilityValues.add(itPlayer->getID(),
						scaled::max(0, 1 - rSmaller / rGreater - fixp(0.1)));
			}
			/*	Very little space is always a problem and very very little
				really mustn't happen */
			scaled const rThresh = fixp(0.85);
			if (rSpaceRatio < rThresh)
			{
				kResult.m_volatilityValues.add(itPlayer->getID(),
						(rThresh - rSpaceRatio).pow(2) / rThresh);
				scaled rFoundMinus = rSpaceRatio.pow(2) * rFoundWeight;
				rFoundWeight -= rFoundMinus;
				/*	If space values are generally higher than found values, then
					shifting weight to space might increase the start value although
					the player actually has too little space. Hence the divisor. */
				rSpaceWeight += rFoundMinus / rBaseSpaceWeight;
			}
		}
		else FAssert(false);
		CvPlot const& kStartPlot = *itPlayer->getStartingPlot();
		int iWarTargets = 0;
		int iSameAreaTradeTargets = 0;
		int iDifferentAreaTradeTargets = 0;
		for (PlayerIter<CIV_ALIVE,NOT_SAME_TEAM_AS> itRival(itPlayer->getTeam());
			itRival.hasNext(); ++itRival)
		{
			CvPlot const& kRivalStartPlot = *itRival->getStartingPlot();
			short const iDist = m_pPathDists->d(kStartPlot, kRivalStartPlot);
			if (kStartPlot.sameArea(kRivalStartPlot))
			{
				iSameAreaTradeTargets++;
				iWarTargets++;
			}
			else if (iDist < 8 * iAvgCityDist)
				iDifferentAreaTradeTargets++;
		}
		scaled rWarFactor = 1;
		if (!kGame.isOption(GAMEOPTION_ALWAYS_PEACE))
		{
			if (iWarTargets == 0)
			{
				rWarFactor = fixp(1.1);
				/*	Strong starting site is less useful
					if there is no competition for space */
				scaled rFoundMinus = fixp(1/4.) * rFoundWeight;
				rFoundWeight -= rFoundMinus;
				rSpaceWeight += rFoundMinus / rBaseSpaceWeight;
			}
			else
			{
				scaled const rBaseWarFactor = fixp(0.45);
				rWarFactor = rBaseWarFactor;
				rWarFactor /= scaled(1 + std::min(3, iWarTargets) * rWarFactor).sqrt();
				rWarFactor /= rBaseWarFactor; // normalize
			}
		}
		scaled const rBaseTradeFactor = fixp(0.82);
		scaled rTradeFactor = rBaseTradeFactor;
		int const iTotalTradeTargets = iSameAreaTradeTargets + iDifferentAreaTradeTargets;
		if (iTotalTradeTargets > 0)
		{
			if (kGame.isOption(GAMEOPTION_NO_TECH_TRADING))
				rTradeFactor = fixp(0.4);
			else if (kGame.isOption(GAMEOPTION_NO_TECH_BROKERING))
				rTradeFactor -= fixp(0.05) * (std::min(4, iTotalTradeTargets) - 1);
			scaled const rTradeVal = iSameAreaTradeTargets + iDifferentAreaTradeTargets *
					(1 + fixp(0.2) * per100(GC.getDefineINT(CvGlobals::OVERSEAS_TRADE_MODIFIER)));
			rTradeFactor *= (1 + rTradeVal * rTradeFactor).pow(fixp(1/3.));
		}
		rTradeFactor /= fixp(1.5) * rBaseTradeFactor; // Normalize; looks nicer in the log file.
		scaled rRivalDistFactor = rivalDistFactors.get(itPlayer->getID());
		scaled rSameAreaRivalDistFactor = sameAreaRivalDistFactors.get(itPlayer->getID());
		if (rRivalDistFactor < rTypicalDistFactor)
		{
			rRivalDistFactor /= rTypicalDistFactor;
			rRivalDistFactor.exponentiate(fixp(0.82));
			/*	Doesn't hurt players that much beyond the effect already covered
				by rFromExpansionSpace. But close starts should be avoided also
				to make the early game less tense. */
			kResult.m_volatilityValues.add(itPlayer->getID(), scaled::max(0,
					1 - rRivalDistFactor - fixp(1/16.)));
		}
		else
		{
			rRivalDistFactor = 1;
			/*	Too far apart isn't good either. The algorithm may try to balance a
				(sub-)continent with too much space overall by giving everyone there a
				coastal start and thus an awkward empire shape. Don't really want that. */
			if (rSameAreaTargetDistFactor > 0 &&
				rSameAreaRivalDistFactor > rSameAreaTargetDistFactor)
			{
				rSameAreaRivalDistFactor /= rSameAreaTargetDistFactor;
				rSameAreaRivalDistFactor.exponentiate(fixp(0.5));
				kResult.m_volatilityValues.add(itPlayer->getID(), scaled::max(0,
						rSameAreaRivalDistFactor - 1));
			}
			else if (rSameAreaRivalDistFactor == 0) // isolated
			{
				/*	All things equal, let's avoid isolation. And let's not try too
					hard to make up for isolation w/o sufficient space through a
					high found value. (That'll further increase volatility.) */
				kResult.m_volatilityValues.add(itPlayer->getID(), fixp(0.1));
			}
			// Otherwise, distances are sufficiently covered by rFromExpansionSpace.
		}
		rFromSpaceValue *= rSpaceWeight;
		scaled rRivalMultiplier = rWarFactor * rTradeFactor * rRivalDistFactor;

		/*	Note: NormalizationTarget relies on the start value formula having
			this basic form ((FoundValue * a + b) * c). */
		scaled rStartVal = rFoundValue * rFoundWeight + rFromSpaceValue;
		rStartVal *= rRivalMultiplier;

		kResult.m_startValues.set(itPlayer->getID(), rStartVal);
		kResult.m_foundValues.set(itPlayer->getID(), rFoundValue);
		kResult.m_foundWeights.set(itPlayer->getID(), rFoundWeight);
		kResult.m_rivalMultipliers.set(itPlayer->getID(), rRivalMultiplier);
		/*	Thought this might be a fun option, but actually doesn't seem
			all that interesting to disable volatility. */
		/*static bool const bIgnoreVolatility = GC.getDefineBOOL("ALLOW_UNUSUAL_STARTS", false);
		if (bIgnoreVolatility)
			kResult.m_volatilityValues.set(itPlayer->getID(), 0);*/
		#ifdef SPI_LOG
		scaled rFromFoundVal = rFoundValue * rFoundWeight;
		if (bLog)
		{
			out << "Site #" << (int)itPlayer->getID() << "(" << CvString(itPlayer->getName()).c_str() << ")" << "\n";
			out << "From found value: " << rFromFoundVal.str(1);
			if (rFoundWeight.getPercent() != 100)
				out << " (weight " << rFoundWeight.str(100) << ")";
			out << "\n";
			out << "From exp. space: " << rFromSpaceValue.str(1) << "\n";
			if (rRivalDistFactor.getPercent() < 100)
				out << "Rival distance factor: " << rRivalDistFactor.str(100) << "\n";
			else if (rSameAreaRivalDistFactor > rSameAreaTargetDistFactor &&
				rSameAreaRivalDistFactor.getPercent() > 100)
			{
				out << "Rival distance factor: " << rSameAreaRivalDistFactor.str(100) << "\n";
			}
			if (kResult.m_volatilityValues.get(itPlayer->getID()) > 0)
				out << "Volatility: " << kResult.m_volatilityValues.get(itPlayer->getID()).str(100) << "\n";
			out << "War factor (" << iWarTargets << " pot. enemies): " << rWarFactor.str(100) << "\n";
			out << "Trade factor (" << iTotalTradeTargets << " pot. partners): " << rTradeFactor.str(100) << "\n";
			out << "Start value: " << rStartVal.str(1) << "\n\n";
		}
		#endif
	}
	#ifdef SPI_LOG
	if (bLog)
	{
		out << std::endl;
		gDLL->logMsg("StartingPos.log", out.str().c_str(), false, false);
	}
	#endif
}

/*	Returns a "typical" distance factor and stores per-player (i.e. per starting site)
	distances in the out parameter. Distance factors account for the path distances
	between the current starting sites. They're on the scale of DistanceTable::d. */
scaled StartingPositionIteration::computeRivalDistFactors(
	EnumMap<PlayerTypes,scaled>& kResult, bool bSameArea) const
{
	vector<scaled> arRivalDistFactors;
	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		vector<short> aiRivalDists;
		CvPlot const& kStartPlot = *itPlayer->getStartingPlot();
		for (PlayerIter<CIV_ALIVE,NOT_SAME_TEAM_AS> itRival(itPlayer->getTeam());
			itRival.hasNext(); ++itRival)
		{
			if (!bSameArea || kStartPlot.sameArea(*itRival->getStartingPlot()))
			{
				aiRivalDists.push_back(m_pPathDists->d(
						kStartPlot, *itRival->getStartingPlot()));
			}
		}
		scaled rDistFactor = weightedDistance(aiRivalDists);
		kResult.set(itPlayer->getID(), rDistFactor);
		arRivalDistFactors.push_back(rDistFactor);
	}
	scaled rTypicalDistFactor;
	if (!arRivalDistFactors.empty())
	{
		rTypicalDistFactor = stats::median(arRivalDistFactors);
		rTypicalDistFactor = std::min(fixp(1.25) * rTypicalDistFactor,
				(rTypicalDistFactor + 3 * m_pPathDists->getAvgCityDist()) / 2);
	}
	return rTypicalDistFactor;
}

// Need this little formula in two places. Param isn't const b/c I need to sort it.
scaled StartingPositionIteration::weightedDistance(vector<short>& kDistances)
{
	std::sort(kDistances.begin(), kDistances.end());
	scaled r;
	scaled rSumOfWeights;
	for (size_t i = 0; i < kDistances.size(); i++)
	{
		scaled rWeight(1, (int)i + 1);
		scaled rPlus = kDistances[i] * rWeight;
		// Further rivals beyond the closest can only reduce rDistFactor
		rPlus.decreaseTo(kDistances[0]);
		rPlus *= rWeight;
		rSumOfWeights += rWeight;
		r += rPlus;
	}
	if (rSumOfWeights.isPositive())
		r /= rSumOfWeights;
	return r;
}


scaled StartingPositionIteration::outlierValue(
	EnumMap<PlayerTypes,scaled> const& kStartValues, PlayerTypes eIndex,
	scaled& rPercentage, scaled rNegativeOutlierExtraWeight,
	scaled* pMedian, scaled* pMax) const
{
	scaled rMedian;
	if (pMedian != NULL)
		rMedian = *pMedian;
	scaled rMax;
	if (pMax != NULL)
		rMax = *pMax;
	if (pMedian == NULL || pMax == NULL)
	{
		vector<scaled> arSamples;
		for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
		{
			arSamples.push_back(kStartValues.get(it->getID()));
		}
		rMedian = stats::median(arSamples);
		rMax = stats::max(arSamples);
	}
	scaled const rStartVal = kStartValues.get(eIndex);
	scaled r = (rStartVal - rMedian).abs();
	// Don't mind small differences (at all); uniformity isn't the goal.
	scaled const rPlusMinus = fixp(0.1);
	r -= rMedian * rPlusMinus;
	r.increaseTo(0);
	if (rStartVal < rMedian) // Small outliers are worse
	{
		r *= 2 * (1 + rNegativeOutlierExtraWeight);
		rPercentage = r / (rMedian * (1 + rPlusMinus));
	}
	else
	{	// The max (or very close to it) is extra problematic
		if (rStartVal > scaled::max(fixp(1.25) * rMedian, fixp(0.92) * rMax))
			r *= fixp(4/3.);
		rPercentage = r / (rMedian * (1 - rPlusMinus));
	}
	return r;
}


scaled StartingPositionIteration::startingPositionValue(
	SolutionAttributes& kResult) const
{
	scaled rWorstOutlierVal;
	kResult.m_eWorstOutlier = NO_PLAYER;
	scaled rMedian;
	scaled rMax;
	scaled rSum;
	scaled rWorstVolatility;
	PlayerTypes eWorstVolatilityPlayer = NO_PLAYER;
	{
		vector<scaled> arSamples;
		for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
		{
			scaled rStartVal = kResult.m_startValues.get(it->getID());
			arSamples.push_back(rStartVal);
			/*	Generally between 0 (unproblematic) and
				1 (highly volatile or otherwise degenerate) */
			scaled rVolatitlity = kResult.m_volatilityValues.get(it->getID());
			rSum += rStartVal * (1 - rVolatitlity);
			if (rVolatitlity > rWorstVolatility)
			{
				rWorstVolatility = rVolatitlity;
				eWorstVolatilityPlayer = it->getID();
			}
		}
		rMedian = stats::median(arSamples);
	}
	// Encourage strong starting positions
	scaled r = rSum / fixp(8/3.);
	// Encourage balanced starting positions
	PlayerIter<CIV_ALIVE> it;
	for (; it.hasNext(); ++it)
	{
		scaled rError;
		scaled rOutlierVal = outlierValue(
				kResult.m_startValues, it->getID(), rError, 0, &rMedian);
		if (rOutlierVal > rWorstOutlierVal)
		{
			rWorstOutlierVal = rOutlierVal;
			kResult.m_eWorstOutlier = it->getID();
		}  /* Not a high exponent b/c there's already tolerance built into
			  outlierValue, which (relatively speaking) magnifies outliers. */
		rOutlierVal.exponentiate(fixp(1.1));
		r -= rOutlierVal / fixp(1.5); // Divisor for normalization
		rError += kResult.m_volatilityValues.get(it->getID()) * fixp(2/3.);
		kResult.m_rAvgError += rError;
	}
	kResult.m_rAvgError /= it.nextIndex();
	// Volatility outliers need some strong discouragement
	scaled rTolerance = fixp(0.11);
	if (rWorstVolatility > rTolerance)
	{
		r *= (1 - (rWorstVolatility - rTolerance) /
				(scaled(it.nextIndex()).sqrt() * fixp(4/3.)));
		if (kResult.m_eWorstOutlier == NO_PLAYER)
			kResult.m_eWorstOutlier = eWorstVolatilityPlayer;
	}
	return r;
}


void StartingPositionIteration::currAltSites(PlayerTypes eCurrSitePlayer,
	// (Low first val means high priority)
	vector<std::pair<short,PlotNumTypes> >& kAltSitesByPriority,
	bool bIncludeRemote, PlotNumTypes eTakenSite) const
{
	VoronoiCell const* pCell = m_pPotentialSites->getCell(eCurrSitePlayer);
	if (pCell == NULL)
		return;
	CvPlot const& kCurrSite = *GET_PLAYER(eCurrSitePlayer).getStartingPlot();
	short iCurrClosestPathDist = MAX_SHORT;
	for (PlayerIter<CIV_ALIVE,NOT_SAME_TEAM_AS> it(TEAMID(eCurrSitePlayer));
		it.hasNext(); ++it)
	{
		iCurrClosestPathDist = std::min(iCurrClosestPathDist,
				m_pPathDists->d(kCurrSite, *it->getStartingPlot()));
	}
	vector<std::pair<short,PlotNumTypes> > altSitesByDist;
	CvMap const& kMap = GC.getMap();
	std::vector<CvPlot const*> apRivalSites;
	if (eTakenSite != NO_PLOT_NUM)
		apRivalSites.push_back(&kMap.getPlotByIndex(eTakenSite));
	for (PlayerIter<CIV_ALIVE,NOT_SAME_TEAM_AS> itRival(TEAMID(eCurrSitePlayer));
		itRival.hasNext(); ++itRival)
	{
		apRivalSites.push_back(itRival->getStartingPlot());
	}
	for (VoronoiCell::const_iterator itSite = pCell->begin(); itSite != pCell->end();
		++itSite)
	{
		if (*itSite == eTakenSite)
			continue;
		CvPlot const& kAltSite = kMap.getPlotByIndex(*itSite);
		/*	startingPositionValue discourages starting sites that are
			too close together, but want to _guarantee_ a lower bound. */
		short iAltClosestPathDist = MAX_SHORT;
		int iAltClosestPlotDist = MAX_INT;
		for (size_t i = 0; i < apRivalSites.size(); i++)
		{
			iAltClosestPathDist = std::min(iAltClosestPathDist,
					m_pPathDists->d(kAltSite, *apRivalSites[i]));
			iAltClosestPlotDist = std::min(iAltClosestPlotDist,
					plotDistance(&kAltSite, apRivalSites[i]));
		}
		if (iAltClosestPathDist >= iCurrClosestPathDist ||
			(2 * iAltClosestPathDist >= 3 * m_pPathDists->getAvgCityDist() &&
			iAltClosestPlotDist > 5)) // Avoid overlapping city radii
		{
			altSitesByDist.push_back(make_pair(
					m_pPathDists->d(kCurrSite, kAltSite), *itSite));
		}
	}
	std::sort(altSitesByDist.begin(), altSitesByDist.end());
	/*	Depending on how evenly the potential sites are distributed, a cell could
		contain a few dozen of them. Don't need that many alternatives. */
	while (altSitesByDist.size() > 6)
		altSitesByDist.pop_back();

	/*	Could directly add remote sites to kAltSitesByPriority w/ first value 0
		for maximal priority. Or with minimal priority if this block is moved up.
		For now, add remote sites and their distance values to altSitesByDist
		and then sort that container again. */
	if (bIncludeRemote)
	{
		int iIsolatedSitesAdded = 0;
		for (int i = 0; i < 3; i++)
		{
			PlotNumTypes eRemoteSite = m_pPotentialSites->getRemoteSite(i);
			if (eRemoteSite == NO_PLOT_NUM || eRemoteSite == eTakenSite)
				break;
			altSitesByDist.push_back(make_pair(
					m_pPathDists->d(kCurrSite, kMap.getPlotByIndex(eRemoteSite)),
					eRemoteSite));
			iIsolatedSitesAdded++;
		}
		/*	The above are "remote" sites in a strict sense - remote from everyone.
			Also include sites that are merely remote from eCurrSitePlayer. */
		// (Would be better to encapsulate this slice of STL hell somehow ...)
		typedef std::map<CvArea const*,std::set<PlotNumTypes>*> AreaSiteMap;
		AreaSiteMap otherAreaSites;
		/*	Going through rival Voronoi cells ensures that the sites aren't
			remote sites in the strict sense nor sites in our pCell. */
		for (PlayerIter<CIV_ALIVE,NOT_SAME_TEAM_AS> itRival(TEAMID(eCurrSitePlayer));
			itRival.hasNext(); ++itRival)
		{
			VoronoiCell const* pRivalCell = m_pPotentialSites->getCell(itRival->getID());
			if (pRivalCell == NULL)
				continue;
			for (VoronoiCell::const_iterator itRemoteSite = pRivalCell->begin();
				itRemoteSite != pRivalCell->end(); ++itRemoteSite)
			{
				if (*itRemoteSite == eTakenSite)
					continue;
				CvPlot const& kRemoteSite = kMap.getPlotByIndex(*itRemoteSite);
				if (kRemoteSite.sameArea(kCurrSite) || pCell->count(*itRemoteSite) > 0)
					continue;
				std::set<PlotNumTypes>* pAreaSites = NULL;
				AreaSiteMap::iterator pos = otherAreaSites.find(kRemoteSite.area());
				if (pos == otherAreaSites.end())
				{
					pAreaSites = new std::set<PlotNumTypes>();
					otherAreaSites.insert(make_pair(kRemoteSite.area(), pAreaSites));
				}
				else pAreaSites = pos->second;
				pAreaSites->insert(*itRemoteSite);
			}
		}
		vector<std::pair<PlotNumTypes,int> > otherAreasBySize;
		for (AreaSiteMap::const_iterator it = otherAreaSites.begin();
			it != otherAreaSites.end(); ++it)
		{
			otherAreasBySize.push_back(make_pair(
					it->first->getNumTiles(), it->first->getID()));
		}
		std::sort(otherAreasBySize.rbegin(), otherAreasBySize.rend());
		for (size_t i = 0; i < otherAreasBySize.size() &&
			// Consider at most 5 remote sites in total
			i < std::min(3u, (size_t)std::max(0, 5 - iIsolatedSitesAdded)); i++)
		{
			AreaSiteMap::const_iterator pos = otherAreaSites.find(
					kMap.getArea(otherAreasBySize[i].second));
			// Find the most isolated site in the area
			PlotNumTypes eBestSite = NO_PLOT_NUM;
			scaled rBestDist = -1;
			for (std::set<PlotNumTypes>::const_iterator itSite = pos->second->begin();
				itSite != pos->second->end(); ++itSite)
			{
				if (*itSite == eTakenSite)
					continue;
				CvPlot const& kSite = kMap.getPlotByIndex(*itSite);
				vector<short> aDists;
				for (PlayerIter<CIV_ALIVE,NOT_SAME_TEAM_AS> it(TEAMID(eCurrSitePlayer));
					it.hasNext(); ++it)
				{
					aDists.push_back(m_pPathDists->d(*it->getStartingPlot(), kSite));
				}
				scaled rDistFactor = weightedDistance(aDists);
				if (rDistFactor > rBestDist)
				{
					eBestSite = *itSite;
					rBestDist = rDistFactor;
				}
			}
			if (eBestSite != NO_PLOT_NUM)
			{
				altSitesByDist.push_back(make_pair(
						m_pPathDists->d(kCurrSite, kMap.getPlotByIndex(eBestSite)),
						eBestSite));
			}
		}
		for (AreaSiteMap::iterator it = otherAreaSites.begin();
			it != otherAreaSites.end(); ++it)
		{
			delete it->second;
		}
		// Sort again now that remote sites are in
		std::sort(altSitesByDist.begin(), altSitesByDist.end());
	}
	for (size_t i = 0; i < altSitesByDist.size(); i++)
		kAltSitesByPriority.push_back(altSitesByDist[i]);
}


bool StartingPositionIteration::considerStep(Step& kStep,
	SolutionAttributes& kCurrSolutionAttribs) const
{
	FAssertMsg(kCurrSolutionAttribs.m_eWorstOutlier != NO_PLAYER, "Should've terminated already");
	kStep.take();
	SolutionAttributes newSolutionAttribs;
	evaluateCurrPosition(newSolutionAttribs);
	if (newSolutionAttribs.m_rStartPosVal > kCurrSolutionAttribs.m_rStartPosVal)
	{
		if (newSolutionAttribs.m_eWorstOutlier == NO_PLAYER ||
			// Minor improvement in overall fairness and new worst outlier or getting close
			((newSolutionAttribs.m_eWorstOutlier != kCurrSolutionAttribs.m_eWorstOutlier ||
			kCurrSolutionAttribs.m_rAvgError < fixp(0.02)) &&
			newSolutionAttribs.m_rAvgError + fixp(0.01) < kCurrSolutionAttribs.m_rAvgError) ||
			// Same worst outlier and significant improvement in overall fairness
			newSolutionAttribs.m_rAvgError + fixp(0.02) < kCurrSolutionAttribs.m_rAvgError)
		{
			if (kStep.getNumMoves() > 1)
			{
				/*	One last check: The outlier value of the site moved in kStep's first move
					needs to improve. That's the site we're primarily interested in. */
				scaled rDummy;
				scaled rNewOutlierVal = outlierValue(newSolutionAttribs.m_startValues,
						kStep.getFirstMovePlayer(), rDummy);
				scaled rCurrOutlierVal = outlierValue(kCurrSolutionAttribs.m_startValues,
						kStep.getFirstMovePlayer(), rDummy);
				if (rNewOutlierVal * fixp(1.01) > rCurrOutlierVal)
				{
					//logStep(kStep, kCurrSolutionAttribs, newSolutionAttribs, false);
					kStep.takeBack();
					return false;
				}
			}
			logStep(kStep, kCurrSolutionAttribs, newSolutionAttribs, true);
			/*	Commit to this step (w/o considering further alternatives).
				Tbd.: Could probably get better results by considering all alternatives. */
			/*	Copy newSolutionAttribs into kCurrSolutionAttribs.
				(EnumMap has no copy-ctor, and I don't want to write one right now.) */
			kCurrSolutionAttribs.m_eWorstOutlier = newSolutionAttribs.m_eWorstOutlier;
			kCurrSolutionAttribs.m_rAvgError = newSolutionAttribs.m_rAvgError;
			kCurrSolutionAttribs.m_rStartPosVal = newSolutionAttribs.m_rStartPosVal;
			for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
			{
				kCurrSolutionAttribs.m_startValues.set(it->getID(),
						newSolutionAttribs.m_startValues.get(it->getID()));
				kCurrSolutionAttribs.m_foundValues.set(it->getID(),
						newSolutionAttribs.m_foundValues.get(it->getID()));
				kCurrSolutionAttribs.m_foundWeights.set(it->getID(),
						newSolutionAttribs.m_foundWeights.get(it->getID()));
				kCurrSolutionAttribs.m_rivalMultipliers.set(it->getID(),
						newSolutionAttribs.m_rivalMultipliers.get(it->getID()));
			}
			return true;
		}
	}
	// This step makes matters worse or isn't a significant improvement
	//logStep(kStep, kCurrSolutionAttribs, newSolutionAttribs, false);
	kStep.takeBack();
	return false;
}


void StartingPositionIteration::logStep(Step const& kStep,
	SolutionAttributes const& kOldSolution,
	SolutionAttributes& kNewSolution, bool bStepTaken) const
{
#ifdef SPI_LOG
	std::ostringstream out;
	if (!bStepTaken)
		out << "Step not taken:\n";
	out << kStep.debugStr();
	// ScaledNum doesn't really support streams; need to cache strings locally.
	CvString szNewErr = kNewSolution.m_rAvgError.str(1000);
	out << (kNewSolution.m_rAvgError < kOldSolution.m_rAvgError ?
			"Reduces" : "Increases") <<
			" avg. error to " << szNewErr <<
			" (was " << kOldSolution.m_rAvgError.str(1000) << ")\n";
	CvString szNewVal = kNewSolution.m_rStartPosVal.str(1);
	out << (kNewSolution.m_rStartPosVal > kOldSolution.m_rStartPosVal ?
			"Increases" : "Decreases") << " start position value to " <<
			szNewVal << " (was " << kOldSolution.m_rStartPosVal.str(1) << ")\n";
	if (kNewSolution.m_eWorstOutlier != kOldSolution.m_eWorstOutlier)
	{
		out << "New worst outlier: " << kNewSolution.m_eWorstOutlier <<
				" (was " << kOldSolution.m_eWorstOutlier << ")\n";
	}
	out << std::endl;
	gDLL->logMsg("StartingPos.log", out.str().c_str(), false, false);
	evaluateCurrPosition(kNewSolution, /*bLog=*/true);
#endif
}


void StartingPositionIteration::doIterations(PotentialSites& kPotentialSites)
{
	evaluateCurrPosition(m_currSolutionAttribs, true);
	m_bNormalizationTargetReady = true;

	if (m_currSolutionAttribs.m_eWorstOutlier == NO_PLAYER)
		return;

	CvMap const& kMap = GC.getMap();
	bool bPrevStepRemote = false;
	/*	Iterate until we return due to getting stuck in a (local) optimum
		or until we run out of time */
	int iStepsConsidered = 0;
	while (iStepsConsidered < 700)
	{
		vector<std::pair<scaled,PlayerTypes> > currSitesByOutlierVal;
		for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
		{
			scaled rDummy;
			scaled rOutlierVal = outlierValue(m_currSolutionAttribs.m_startValues,
					it->getID(), rDummy,
					// Focus on negative outliers while the avg. error is high
					fixp(4/3.) * (m_currSolutionAttribs.m_rAvgError - fixp(0.15)));
			if (rOutlierVal > 0)
			{	/*	Also take into account volatility. Crowded areas tend to have higher
					volatility. Need to avoid moving a small number of high-value
					outliers that have small areas to themselves onto a large landmass
					that is already overcrowded. Or better to do this more explicitly?
					Can't easily check for crowdedness here though ... */
				/*int iAreaPlayers = 1;
				for (PlayerIter<CIV_ALIVE,NOT_SAME_TEAM_AS> itRival(it->getTeam());
					itRival.hasNext(); ++itRival)
				{
					if (itRival->getStartingPlot()->sameArea(*it->getStartingPlot()))
						iAreaPlayers++;
				}
				rOutlierVal += rOutlierVal * (m_currSolutionAttribs.m_rAvgError - fixp(0.1)) *
						scaled(iAreaPlayers).sqrt();*/
				rOutlierVal += rOutlierVal * (m_currSolutionAttribs.m_rAvgError - fixp(0.1)) *
						2 * m_currSolutionAttribs.m_volatilityValues.get(it->getID());
				currSitesByOutlierVal.push_back(make_pair(rOutlierVal, it->getID()));
			}
		}
		if (currSitesByOutlierVal.empty())
			return;
		std::sort(currSitesByOutlierVal.rbegin(), currSitesByOutlierVal.rend());
		for (size_t i = 0; i < currSitesByOutlierVal.size(); i++)
		{
			PlayerTypes const eCurrSitePlayer = currSitesByOutlierVal[i].second;
			CvPlot const& kCurrSite = *GET_PLAYER(eCurrSitePlayer).getStartingPlot();
			/*	(Low first val meaning high priority. For now, the priority values
				are distance values.) */
			vector<std::pair<short,PlotNumTypes> > altSitesByPriority;
			currAltSites(eCurrSitePlayer, altSitesByPriority,
					/*	To save time, stop considering remote sites when approaching
						a decent solution. Unless the prev. step was still a remote one. */
					bPrevStepRemote || m_currSolutionAttribs .m_rAvgError > per1000(285));
			for (size_t j = 0; j < altSitesByPriority.size(); j++)
			{
				PlotNumTypes const eAltSite = altSitesByPriority[j].second;
				CvPlot& kAltSite = kMap.getPlotByIndex(eAltSite);
				short const iMoveDist = m_pPathDists->d(kCurrSite, kAltSite);
				bool const bStepRemote = (
						m_pPathDists->d(kCurrSite, kAltSite) >= m_pPathDists->getLongDist());
				Step singleMoveStep;
				singleMoveStep.move(eCurrSitePlayer, kAltSite);
				vector<std::pair<short,PlayerTypes> > otherCurrSitesByDist;
				for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
				{
					if (itPlayer->getID() != eCurrSitePlayer)
					{
						short d = std::min<short>(
								m_pPathDists->d(*itPlayer->getStartingPlot(), kAltSite),
								/*	Prefer moving the player closest to the new site
									of eCurrPlayer, but, if none are really close,
									also consider moving the player closest to the
									old site of eCurrPlayer. */
								(m_pPathDists->d(*itPlayer->getStartingPlot(),
								kCurrSite) * 3) / 2);
						if (d < m_pPathDists->getLongDist())
							otherCurrSitesByDist.push_back(make_pair(d, itPlayer->getID()));
					}
				}
				/*	If there are no other starting sites near the alt. site nor
					the current site, then consider moving only the current site. */
				if (otherCurrSitesByDist.empty())
				{
					iStepsConsidered++;
					if (considerStep(singleMoveStep, m_currSolutionAttribs))
					{
						bPrevStepRemote = bStepRemote;
						goto next_iteration;
					}
				}
				std::sort(otherCurrSitesByDist.begin(), otherCurrSitesByDist.end());
				/*	Don't try a great number of combinations in this deeply nested loop.
					Rather run more iterations of the main loop instead. */
				while (otherCurrSitesByDist.size() > 2)
					otherCurrSitesByDist.pop_back();
				for (size_t k = 0; k < otherCurrSitesByDist.size(); k++)
				{
					if (2 * otherCurrSitesByDist[k].first >
						3 * otherCurrSitesByDist[0].first)
					{
						continue;
					}
					vector<std::pair<short,PlotNumTypes> > otherAltSitesByDist;
					currAltSites(otherCurrSitesByDist[k].second, otherAltSitesByDist,
							// Allow remote site for 2nd move only if 1st move is remote
							iMoveDist > m_pPathDists->getLongDist(),
							// Don't move the second player to the same site as the first
							eAltSite);
					for (size_t a = 0; a < otherAltSitesByDist.size(); a++)
					{
						Step doubleMoveStep(singleMoveStep);
						doubleMoveStep.move(otherCurrSitesByDist[k].second,
								kMap.getPlotByIndex(otherAltSitesByDist[a].second));
						iStepsConsidered++;
						if (considerStep(doubleMoveStep, m_currSolutionAttribs))
						{	/*	Never mind the 2nd move; can't be remote if the
								1st isn't remote. */
							bPrevStepRemote = bStepRemote;
							goto next_iteration;
						}
					}
				}
				// No double-move step is worthwhile. Consider the single-move step.
				iStepsConsidered++;
				if (considerStep(singleMoveStep, m_currSolutionAttribs))
				{
					bPrevStepRemote = bStepRemote;
					goto next_iteration;
				}
			}
		}
		return; // No step taken; apparently we're done.
		next_iteration: // To allow breaking out of the inner loops
		kPotentialSites.updateCurrSites(true); // Also ensures that we never return to a site
	}
	#ifdef SPI_LOG
		gDLL->logMsg("StartingPos.log", "Terminated b/c of time limit", false, false);
	#endif
}


NormalizationTarget::NormalizationTarget(CitySiteEvaluator& kEval,
	StartingPositionIteration::SolutionAttributes const& kSolution)
{
	m_pEval = &kEval;
	CvMap const& kMap = GC.getMap();
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		PlayerTypes ePlayer = it->getID();
		m_startValData.insert(make_pair(
				kMap.plotNum(*it->getStartingPlot()),
				StartValBreakdown(
				kSolution.m_startValues.get(ePlayer),
				kSolution.m_volatilityValues.get(ePlayer),
				kSolution.m_foundValues.get(ePlayer),
				kSolution.m_foundWeights.get(ePlayer),
				kSolution.m_rivalMultipliers.get(ePlayer))));
	}
}


NormalizationTarget::~NormalizationTarget()
{
	delete m_pEval;
}


bool NormalizationTarget::isReached(CvPlot const& kStartSite,
	bool bNearlyReached, bool bClearlyExceeded) const
{
	CvMap const& kMap = GC.getMap();
	PlotNumTypes const ePlot = kMap.plotNum(kStartSite);

	scaled rCurrStartVal = -1;
	short iCurrFoundVal = -1;
	vector<scaled> arCurrFoundValues;
	vector<scaled> arCurrStartValues;
	for (std::map<PlotNumTypes,StartValBreakdown>::const_iterator it =
		m_startValData.begin(); it != m_startValData.end(); ++it)
	{
		PlotNumTypes const eLoopPlot = it->first;
		CvPlot const& kLoopPlot = kMap.getPlotByIndex(eLoopPlot);
		short iFoundVal = m_pEval->evaluate(kLoopPlot);
		arCurrFoundValues.push_back(iFoundVal);
		StartValBreakdown svb = it->second;
		scaled rStartVal = svb.rTotal;
		/*	Eliminate the multipliers, subtract the pre-normalization found value,
			add the current found value, re-apply the multipliers. */
		if (svb.rRivalMult.isPositive())
			rStartVal /= svb.rRivalMult;
		// svb.rFoundVal is the unweighted pre-normalization found value
		rStartVal = rStartVal + (iFoundVal - svb.rFoundVal) * svb.rFoundWeight;
		if (svb.rRivalMult.isPositive())
			rStartVal *= svb.rRivalMult;
		arCurrStartValues.push_back(rStartVal);
		if (&kLoopPlot == &kStartSite)
		{
			iCurrFoundVal = iFoundVal;
			rCurrStartVal = rStartVal;
		}
	}
	FAssert(iCurrFoundVal != -1 && rCurrStartVal != -1);
	scaled const rMedianCurrFoundVal = stats::median(arCurrFoundValues);
	scaled const rMedianCurrStartVal = stats::median(arCurrStartValues);
	scaled const rMaxCurrStartVal = stats::max(arCurrStartValues);
	
	scaled rIncreaseNeeded =
			// Don't increase kStartSite's found value far beyond the median found value
			std::min(fixp(1.1) * rMedianCurrFoundVal - iCurrFoundVal,
			// Don't keep kStartSite's start value far below the highest start value
			std::max(fixp(0.8) * rMaxCurrStartVal - rCurrStartVal, std::max(
			/*	If kStartSite's start value is below the median start value,
				then increase kStartSite's foundValue at least to the median found value. */
			std::min(rMedianCurrStartVal - rCurrStartVal,
			rMedianCurrFoundVal - iCurrFoundVal),
			/*	Try to ensure that the found value isn't far below the median found value -
				regardless of start values. */
			fixp(0.85) * (rMedianCurrFoundVal - iCurrFoundVal))));
	if (bClearlyExceeded)
		return (-rIncreaseNeeded > per100(5) * iCurrFoundVal);
	scaled rTolerance = fixp(2.5);
	if (bNearlyReached)
	{
		/*	(Not sure if rTolerance really has a big impact in general.
			I did in some of my tests.) */
		switch(GC.getGame().getStartingPlotNormalizationLevel())
		{
		case CvGame::NORMALIZE_HIGH: rTolerance += 2;
		case CvGame::NORMALIZE_MEDIUM: rTolerance += 4;
		default: rTolerance += 6;
		}
	}
	rTolerance /= 100;
	/*	Be more tolerant of a subpar starting site if the player at that site
		is supposed to be handicapped.
		(Should get the player through a parameter I guess. Eh ...) */
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		if (it->getStartingPlot() == &kStartSite)
		{
			rTolerance += rTolerance * per100(GC.getInfo(it->getHandicapType()).
					getStartingLocationPercent() - 50) * per100(75);
			rTolerance.increaseTo(0);
			break;
		}
	}
	return (rIncreaseNeeded <= rTolerance * iCurrFoundVal);
}


scaled NormalizationTarget::getStartValue(CvPlot const& kStartSite) const
{
	StartValBreakdown const* pBrk = getBreakdown(kStartSite);
	if (pBrk == NULL)
		return -1;
	return pBrk->rTotal;
}


scaled NormalizationTarget::getVolatilityValue(CvPlot const& kStartSite) const
{
	StartValBreakdown const* pBrk = getBreakdown(kStartSite);
	if (pBrk == NULL)
		return 0;
	return pBrk->rVolatility;
}


NormalizationTarget::StartValBreakdown const* NormalizationTarget::getBreakdown(
	CvPlot const& kSite) const
{
	PlotNumTypes const ePlot = GC.getMap().plotNum(kSite);
	map<PlotNumTypes,StartValBreakdown>::const_iterator pos = m_startValData.find(ePlot);
	if (pos == m_startValData.end())
	{
		FAssertMsg(false, "Starting plot not found in normalization inputs");
		return NULL;
	}
	return &pos->second;
}
