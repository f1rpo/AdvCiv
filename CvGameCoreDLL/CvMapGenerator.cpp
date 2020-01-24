#include "CvGameCoreDLL.h"
#include "CvMapGenerator.h"
#include "CvGame.h"
#include "PlotRange.h"
#include "CvArea.h" // advc.003s
#include "CvFractal.h"
#include "CvInfo_Terrain.h"
#include "CvInfo_GameOption.h"


CvMapGenerator* CvMapGenerator::m_pInst = NULL; // static


CvMapGenerator& CvMapGenerator::GetInstance() // singleton accessor
{
	if (m_pInst==NULL)
		m_pInst = new CvMapGenerator;
	return *m_pInst;
}


bool CvMapGenerator::canPlaceBonusAt(BonusTypes eBonus, int iX, int iY, bool bIgnoreLatitude)  // advc: style changes
{
	PROFILE_FUNC();

	CvMap& m = GC.getMap();
	CvPlot* pPlot = m.plot(iX, iY);
	if(pPlot == NULL)
		return false;
	CvArea const& kArea = pPlot->getArea();

	if(!pPlot->canHaveBonus(eBonus, bIgnoreLatitude))
		return false;

	{
		bool bOverride=false;
		bool r = GC.getPythonCaller()->canPlaceBonusAt(*pPlot, bOverride);
		if (bOverride)
			return r;
	}
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		CvPlot* pLoopPlot = plotDirection(iX, iY, ((DirectionTypes)iI));
		if(pLoopPlot == NULL)
			continue;
		BonusTypes eLoopBonus = pLoopPlot->getBonusType();
		if(eLoopBonus != NO_BONUS && eLoopBonus != eBonus)
			return false;
	}

	CvBonusInfo& pInfo = GC.getInfo(eBonus);
	CvBonusClassInfo& pClassInfo = GC.getInfo((BonusClassTypes)
			pInfo.getBonusClassType());

	if(pPlot->isWater())
	{
		if(((m.getNumBonusesOnLand(eBonus) * 100) / (m.getNumBonuses(eBonus) + 1)) <
				pInfo.getMinLandPercent())
			return false;
	}

	int const iRange = pClassInfo.getUniqueRange();
	for (int iDX = -iRange; iDX <= iRange; iDX++)
	for (int iDY = -iRange; iDY <= iRange; iDY++)
	{
		CvPlot* pLoopPlot = plotXY(iX, iY, iDX, iDY);
		if(pLoopPlot == NULL || !pLoopPlot->isArea(kArea))
			continue;
		if (plotDistance(iX, iY, pLoopPlot->getX(), pLoopPlot->getY()) <= iRange)
		{
			BonusTypes eOtherBonus = pLoopPlot->getBonusType();
			if(eOtherBonus == NO_BONUS)
				continue;
			// Make sure there are none of the same bonus nearby:
			if(eBonus == eOtherBonus)
				return false;
			// Make sure there are no bonuses of the same class nearby:
			if(GC.getInfo(eOtherBonus).getBonusClassType() ==
					pInfo.getBonusClassType())
				return false;
		}
	}
	// <advc.129> Prevent more than one adjacent copy regardless of range.
	int iFound = 0;
	for(int i = 0; i < NUM_DIRECTION_TYPES; i++)
	{
		CvPlot* pp = plotDirection(iX, iY, (DirectionTypes)i);
		if(pp == NULL) continue; CvPlot const& p = *pp;
		if(!p.isArea(kArea))
			continue;
		if(p.getBonusType() == eBonus)
		{
			iFound++;
			if(iFound >= 2)
				return false;
			/*  A single adjacent copy could already have another adjacent copy.
				However, if that's prohibited, clusters of more than 2 resources
				won't be placed at all. (They're only placed around one central
				tile, which also gets the resource.) Better to change the placement
				pattern then (addUniqueBonusType). */
			/*for(int j = 0; j < NUM_DIRECTION_TYPES; j++) {
				CvPlot* pp2 = plotDirection(p.getX(), p.getY(),
						(DirectionTypes)j);
				if(pp2 == NULL) continue; CvPlot const& p2 = *pp2;
				if(!p2.isArea(kArea)) continue;
				if(p2.getBonusType() == eBonus)
					return false;
			}*/
		}
	} // </advc.129>

	return true;
}


bool CvMapGenerator::canPlaceGoodyAt(ImprovementTypes eImprovement, int iX, int iY)
{
	PROFILE_FUNC();

	FAssertMsg(eImprovement != NO_IMPROVEMENT, "Improvement is not assigned a valid value");
	FAssertMsg(GC.getInfo(eImprovement).isGoody(), "ImprovementType eImprovement is expected to be a goody");

	if (GC.getGame().isOption(GAMEOPTION_NO_GOODY_HUTS))
		return false;

	CvPlot* pPlot = GC.getMap().plot(iX, iY);
	if (!pPlot->canHaveImprovement(eImprovement, NO_TEAM))
		return false;
	{
		bool bOverride=false;
		bool r = GC.getPythonCaller()->canPlaceGoodyAt(*pPlot, bOverride);
		if (bOverride)
			return r;
	}
	if (pPlot->isImproved() || pPlot->getBonusType() != NO_BONUS || pPlot->isImpassable())
		return false;

	int iUniqueRange = GC.getInfo(eImprovement).getGoodyUniqueRange();
	for (int iDX = -iUniqueRange; iDX <= iUniqueRange; iDX++)
	{
		for (int iDY = -iUniqueRange; iDY <= iUniqueRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(iX, iY, iDX, iDY);
			if (pLoopPlot != NULL && pLoopPlot->getImprovementType() == eImprovement)
				return false;
		}
	}

	return true;
}


void CvMapGenerator::addGameElements()
{
	addRivers();
	gDLL->logMemState("CvMapGen after add rivers");

	addLakes();
	gDLL->logMemState("CvMapGen after add lakes");

	addFeatures();
	gDLL->logMemState("CvMapGen after add features");

	addBonuses();
	gDLL->logMemState("CvMapGen after add bonuses");

	addGoodies();
	gDLL->logMemState("CvMapGen after add goodies");

	// Call for Python to make map modifications after it's been generated
	afterGeneration();
}


void CvMapGenerator::addLakes()
{
	PROFILE_FUNC();

	if (GC.getPythonCaller()->addLakes())
		return;

	gDLL->NiTextOut("Adding Lakes...");
	int const iLAKE_PLOT_RAND = GC.getDefineINT("LAKE_PLOT_RAND"); // advc.opt
	for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
	{
		//gDLL->callUpdater(); // advc.opt: Not needed I reckon
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(iI);
		FAssertMsg(pLoopPlot != NULL, "LoopPlot is not assigned a valid value");

		if (!pLoopPlot->isWater())
		{
			if (!pLoopPlot->isCoastalLand())
			{
				if (!pLoopPlot->isRiver())
				{
					if (GC.getGame().getMapRandNum(iLAKE_PLOT_RAND, "addLakes") == 0)
					{
						pLoopPlot->setPlotType(PLOT_OCEAN);
					}
				}
			}
		}
	}
}

void CvMapGenerator::addRivers()  // advc: Refactored
{
	PROFILE_FUNC();

	if (GC.getPythonCaller()->addRivers())
		return;

	gDLL->NiTextOut("Adding Rivers...");

	int const iRiverSourceRange = GC.getDefineINT("RIVER_SOURCE_MIN_RIVER_RANGE");
	int const iSeaWaterRange = GC.getDefineINT("RIVER_SOURCE_MIN_SEAWATER_RANGE");
	int const iPlotsPerRiverEdge =  GC.getDefineINT("PLOTS_PER_RIVER_EDGE");
	// advc.129: Randomize the traversal order
	int* aiShuffledIndices = ::shuffle(GC.getMap().numPlots(), GC.getGame().getMapRand());
	for (int iPass = 0; iPass < 4; iPass++)
	{
		int iRiverSourceRangeLoop = (iPass <= 1 ? iRiverSourceRange : iRiverSourceRange / 2);
		int iSeaWaterRangeLoop =  (iPass <= 1 ? iSeaWaterRange : iSeaWaterRange / 2);

		for (int i = 0; i < GC.getMap().numPlots(); i++)
		{
			CvPlot const* pLoopPlot = GC.getMap().plotByIndex(
					aiShuffledIndices[i]); // advc.129
			if (pLoopPlot->isWater())
				continue;

			bool bValid;
			switch(iPass)
			{
			case 0:
				bValid = (pLoopPlot->isHills() || pLoopPlot->isPeak());
				break;
			case 1:
				bValid = (!pLoopPlot->isCoastalLand() &&
						GC.getGame().getMapRandNum(8, "addRivers") == 0);
				break;
			case 2:
				bValid =  ((pLoopPlot->isHills() || pLoopPlot->isPeak()) &&
					  pLoopPlot->getArea().getNumRiverEdges() < 1 +
					  pLoopPlot->getArea().getNumTiles() / iPlotsPerRiverEdge);
				break;
			case 3:
				bValid = (pLoopPlot->getArea().getNumRiverEdges() < 1 +
						pLoopPlot->getArea().getNumTiles() / iPlotsPerRiverEdge);
				break;
			default: FAssertMsg(false, "Invalid iPass");
			}
			if (!bValid)
				continue;

			gDLL->callUpdater(); // advc.opt: Moved down; shouldn't need to update the UI in every iteration.
			if (!GC.getMap().findWater(pLoopPlot, iRiverSourceRange, true) &&
				!GC.getMap().findWater(pLoopPlot, iSeaWaterRange, false))
			{
				CvPlot* pStartPlot = pLoopPlot->getInlandCorner();
				if (pStartPlot != NULL)
					doRiver(pStartPlot);
			}
		}
	}
	SAFE_DELETE_ARRAY(aiShuffledIndices); // advc.129
}

// pStartPlot = the plot at whose SE corner the river is starting
void CvMapGenerator::doRiver(CvPlot *pStartPlot, CardinalDirectionTypes eLastCardinalDirection, CardinalDirectionTypes eOriginalCardinalDirection, int iThisRiverID)
{
	if (iThisRiverID == -1)
	{
		iThisRiverID = GC.getMap().getNextRiverID();
		GC.getMap().incrementNextRiverID();
	}

	int iOtherRiverID = pStartPlot->getRiverID();
	if (iOtherRiverID != -1 && iOtherRiverID != iThisRiverID)
	{
		return; // Another river already exists here; can't branch off of an existing river!
	}

	CvPlot *pRiverPlot = NULL;
	CvPlot *pAdjacentPlot = NULL;

	CardinalDirectionTypes eBestCardinalDirection = NO_CARDINALDIRECTION;

	if (eLastCardinalDirection==CARDINALDIRECTION_NORTH)
	{
		pRiverPlot = pStartPlot;
		if (pRiverPlot == NULL)
		{
			return;
		}
		pAdjacentPlot = plotCardinalDirection(pRiverPlot->getX(), pRiverPlot->getY(), CARDINALDIRECTION_EAST);
		if ((pAdjacentPlot == NULL) || pRiverPlot->isWOfRiver() || pRiverPlot->isWater() || pAdjacentPlot->isWater())
		{
			return;
		}

		pStartPlot->setRiverID(iThisRiverID);
		pRiverPlot->setWOfRiver(true, eLastCardinalDirection);
		pRiverPlot = plotCardinalDirection(pRiverPlot->getX(), pRiverPlot->getY(), CARDINALDIRECTION_NORTH);
	}
	else if (eLastCardinalDirection==CARDINALDIRECTION_EAST)
	{
		pRiverPlot = plotCardinalDirection(pStartPlot->getX(), pStartPlot->getY(), CARDINALDIRECTION_EAST);
		if (pRiverPlot == NULL)
		{
			return;
		}
		pAdjacentPlot = plotCardinalDirection(pRiverPlot->getX(), pRiverPlot->getY(), CARDINALDIRECTION_SOUTH);
		if ((pAdjacentPlot == NULL) || pRiverPlot->isNOfRiver() || pRiverPlot->isWater() || pAdjacentPlot->isWater())
		{
			return;
		}

		pStartPlot->setRiverID(iThisRiverID);
		pRiverPlot->setNOfRiver(true, eLastCardinalDirection);
	}
	else if (eLastCardinalDirection==CARDINALDIRECTION_SOUTH)
	{
		pRiverPlot = plotCardinalDirection(pStartPlot->getX(), pStartPlot->getY(), CARDINALDIRECTION_SOUTH);
		if (pRiverPlot == NULL)
		{
			return;
		}
		pAdjacentPlot = plotCardinalDirection(pRiverPlot->getX(), pRiverPlot->getY(), CARDINALDIRECTION_EAST);
		if ((pAdjacentPlot == NULL) || pRiverPlot->isWOfRiver() || pRiverPlot->isWater() || pAdjacentPlot->isWater())
		{
			return;
		}

		pStartPlot->setRiverID(iThisRiverID);
		pRiverPlot->setWOfRiver(true, eLastCardinalDirection);
	}

	else if (eLastCardinalDirection==CARDINALDIRECTION_WEST)
	{
		pRiverPlot = pStartPlot;
		if (pRiverPlot == NULL)
		{
			return;
		}
		pAdjacentPlot = plotCardinalDirection(pRiverPlot->getX(), pRiverPlot->getY(), CARDINALDIRECTION_SOUTH);
		if ((pAdjacentPlot == NULL) || pRiverPlot->isNOfRiver() || pRiverPlot->isWater() || pAdjacentPlot->isWater())
		{
			return;
		}

		pStartPlot->setRiverID(iThisRiverID);
		pRiverPlot->setNOfRiver(true, eLastCardinalDirection);
		pRiverPlot = plotCardinalDirection(pRiverPlot->getX(), pRiverPlot->getY(), CARDINALDIRECTION_WEST);
	}
	else
	{
		//FAssertMsg(false, "Illegal direction type");
		// River is starting here, set the direction in the next step
		pRiverPlot = pStartPlot;

		GC.getPythonCaller()->riverStartCardinalDirection(*pRiverPlot, eBestCardinalDirection);
	}

	if (pRiverPlot == NULL)
	{
		return; // The river has flowed off the edge of the map. All is well.
	}
	else if (pRiverPlot->hasCoastAtSECorner())
	{
		return; // The river has flowed into the ocean. All is well.
	}

	if (eBestCardinalDirection == NO_CARDINALDIRECTION)
	{
		int iBestValue = MAX_INT;

		for (int iI = 0; iI < NUM_CARDINALDIRECTION_TYPES; iI++)
		{
			if (getOppositeCardinalDirection((CardinalDirectionTypes)iI) != eOriginalCardinalDirection)
			{
				if (getOppositeCardinalDirection((CardinalDirectionTypes)iI) != eLastCardinalDirection)
				{
					pAdjacentPlot = plotCardinalDirection(pRiverPlot->getX(), pRiverPlot->getY(), ((CardinalDirectionTypes)iI));
					if (pAdjacentPlot != NULL)
					{
						int iValue = getRiverValueAtPlot(pAdjacentPlot);
						if (iValue < iBestValue)
						{
							iBestValue = iValue;
							eBestCardinalDirection = (CardinalDirectionTypes)iI;
						}
					}
				}
			}
		}
	}

	if (eBestCardinalDirection != NO_CARDINALDIRECTION)
	{
		if  (eOriginalCardinalDirection	== NO_CARDINALDIRECTION)
		{
			eOriginalCardinalDirection = eBestCardinalDirection;
		}
		doRiver(pRiverPlot, eBestCardinalDirection, eOriginalCardinalDirection, iThisRiverID);
	}
}
//Note from Blake:
//Iustus wrote this function, it ensures that a new river actually
//creates fresh water on the passed plot. Quite useful really
//Although I veto'd its use since I like that you don't always
//get fresh water starts.
// pFreshWaterPlot = the plot we want to give a fresh water river
//
bool CvMapGenerator::addRiver(CvPlot* pFreshWaterPlot)
{
	FAssert(pFreshWaterPlot != NULL);

	// cannot have a river flow next to water
	if (pFreshWaterPlot->isWater())
		return false;

	// if it already has a fresh water river, then success! we done
	if (pFreshWaterPlot->isRiver())
		return true;

	// make two passes, once for each flow direction of the river
	int iNWFlowPass = GC.getGame().getMapRandNum(2, "addRiver");
	for (int iPass = 0; iPass <= 1; iPass++)
	{
		// try placing a river edge in each direction, in random order
		FOR_EACH_ENUM_RAND(CardinalDirection, GC.getGame().getMapRand())
		{
			CardinalDirectionTypes eRiverDirection = NO_CARDINALDIRECTION;
			CvPlot *pRiverPlot = NULL;

			switch (eLoopCardinalDirection)
			{
			case CARDINALDIRECTION_NORTH:
				if (iPass == iNWFlowPass)
				{
					pRiverPlot = plotDirection(pFreshWaterPlot->getX(), pFreshWaterPlot->getY(), DIRECTION_NORTH);
					eRiverDirection = CARDINALDIRECTION_WEST;
				}
				else
				{
					pRiverPlot = plotDirection(pFreshWaterPlot->getX(), pFreshWaterPlot->getY(), DIRECTION_NORTHWEST);
					eRiverDirection = CARDINALDIRECTION_EAST;
				}
				break;

			case CARDINALDIRECTION_EAST:
				if (iPass == iNWFlowPass)
				{
					pRiverPlot = pFreshWaterPlot;
					eRiverDirection = CARDINALDIRECTION_NORTH;
				}
				else
				{
					pRiverPlot = plotDirection(pFreshWaterPlot->getX(), pFreshWaterPlot->getY(), DIRECTION_NORTH);
					eRiverDirection = CARDINALDIRECTION_SOUTH;
				}
				break;

			case CARDINALDIRECTION_SOUTH:
				if (iPass == iNWFlowPass)
				{
					pRiverPlot = pFreshWaterPlot;
					eRiverDirection = CARDINALDIRECTION_WEST;
				}
				else
				{
					pRiverPlot = plotDirection(pFreshWaterPlot->getX(), pFreshWaterPlot->getY(), DIRECTION_WEST);
					eRiverDirection = CARDINALDIRECTION_EAST;
				}
				break;

			case CARDINALDIRECTION_WEST:
				if (iPass == iNWFlowPass)
				{
					pRiverPlot = plotDirection(pFreshWaterPlot->getX(), pFreshWaterPlot->getY(), DIRECTION_WEST);
					eRiverDirection = CARDINALDIRECTION_NORTH;
				}
				else
				{
					pRiverPlot = plotDirection(pFreshWaterPlot->getX(), pFreshWaterPlot->getY(), DIRECTION_NORTHWEST);
					eRiverDirection = CARDINALDIRECTION_SOUTH;
				}
				break;

			default:
				FAssertMsg(false, "invalid cardinal direction");
			}

			if (pRiverPlot != NULL && !pRiverPlot->hasCoastAtSECorner())
			{
				// try to make the river
				doRiver(pRiverPlot, eRiverDirection, eRiverDirection, -1);

				// if it succeeded, then we will be a river now!
				if (pFreshWaterPlot->isRiver())
					return true;
			}
		}
	}
	return false;
}


void CvMapGenerator::addFeatures()
{
	PROFILE_FUNC();

	if (GC.getPythonCaller()->addFeatures())
		return;

	for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
	{
		CvPlot& kPlot = GC.getMap().getPlotByIndex(iI);
		for (int iJ = 0; iJ < GC.getNumFeatureInfos(); iJ++)
		{
			if (kPlot.canHaveFeature((FeatureTypes)iJ))
			{
				if (GC.getGame().getMapRandNum(10000, "addFeaturesAtPlot") <
						GC.getInfo((FeatureTypes)iJ).getAppearanceProbability())
					kPlot.setFeatureType((FeatureTypes)iJ);
			}
		}
	}
}

void CvMapGenerator::addBonuses()
{
	PROFILE_FUNC();
	gDLL->NiTextOut("Adding Bonuses...");

	CvPythonCaller const& py = *GC.getPythonCaller();
	if (py.addBonuses())
		return;
	/*  <advc.129> Only do an iteration for those PlacementOrder numbers that are
		actually used in the BonusInfos. */
	std::vector<int> aiOrdinals;
	for (int i = 0; i < GC.getNumBonusInfos(); i++)
	{
		int iOrder = GC.getInfo((BonusTypes)i).getPlacementOrder();
		if (iOrder >= 0) // The negative ones aren't supposed to be placed at all
			aiOrdinals.push_back(iOrder);
	}
	::removeDuplicates(aiOrdinals);
	FAssertMsg(aiOrdinals.size() <= (uint)12, "Shuffling the bonus indices this often might be slow(?)");
	std::sort(aiOrdinals.begin(), aiOrdinals.end());
	//for (int iOrder = 0; iOrder < GC.getNumBonusInfos(); iOrder++)
	for (size_t i = 0; i < aiOrdinals.size(); i++)
	{
		int iOrder = aiOrdinals[i];	
		/*  advc.129: Break ties in the order randomly (perhaps better
			not to do this though if the assertion above fails) */
		FOR_EACH_ENUM_RAND(Bonus, GC.getGame().getMapRand())
		{
			//gDLL->callUpdater();
			if (GC.getInfo(eLoopBonus).getPlacementOrder() != iOrder)
				continue;

			gDLL->callUpdater(); // advc.opt: Moved down; don't need to update the UI quite so frequently.
			if (!py.addBonusType(eLoopBonus))
			{
				if (GC.getInfo(eLoopBonus).isOneArea())
					addUniqueBonusType(eLoopBonus);
				else addNonUniqueBonusType(eLoopBonus);
			}
		}
	}
}

void CvMapGenerator::addUniqueBonusType(BonusTypes eBonusType)
{
	// K-Mod note: the areas tried stuff was originally done using an array.
	// I've rewritten it to use std::set, for no good reason. The functionality is unchanged.
	// (But it is now slightly more efficient and easier to read.)
	std::set<int> areas_tried;

	CvBonusInfo const& pBonusInfo = GC.getInfo(eBonusType);
	int const iTarget = calculateNumBonusesToAdd(eBonusType);
	bool const bIgnoreLatitude = GC.getPythonCaller()->isBonusIgnoreLatitude();
	// advc.opt: Don't waste time trying to place land resources in the ocean
	bool const bWater = (pBonusInfo.isTerrain(GC.getWATER_TERRAIN(true)) ||
			pBonusInfo.isTerrain(GC.getWATER_TERRAIN(false)));
	FAssertMsg(pBonusInfo.isOneArea(), "addUniqueBonusType called with non-unique bonus type");
	CvMap& kMap = GC.getMap(); // advc
	// <advc.129>
	for (int iPass = 0; iPass < 2; iPass++)
	{	/*  Two passes - just to make sure that the new per-area limit doesn't
			lead to fewer resources overall. */
		bool bIgnoreAreaLimit = (iPass == 1); // </advc.129>
		while (/* advc.129: */kMap.getNumBonuses(eBonusType) < iTarget)
		{
			int iBestValue = 0;
			CvArea *pBestArea = NULL;
			FOR_EACH_AREA_VAR(pLoopArea)
			{	// <advc.opt>
				if (pLoopArea->isWater() && !bWater)
					continue; // </advc.opt>
				if (areas_tried.count(pLoopArea->getID()) == 0)
				{
					int iNumTiles = pLoopArea->getNumTiles();
					int iAddedTotal = kMap.getNumBonuses(eBonusType);
					if (iAddedTotal * 3 < 2 * iTarget &&
							iNumTiles < 4 * NUM_CITY_PLOTS) // K-Mod
						continue;

					// number of unique bonuses starting on the area, plus this one
					int iNumUniqueBonusesOnArea = 1 + pLoopArea->countNumUniqueBonusTypes();
					//int iValue = iNumTiles / iNumUniqueBonusesOnArea;
					int iValue = ((iNumTiles *
							// advc.129: Decrease the impact of iNumTiles when approaching the target resource count
							(iTarget - iAddedTotal)) / iTarget +
							GC.getGame().getMapRandNum(3 * NUM_CITY_PLOTS,
							"addUniqueBonusType area value (K-Mod)")) / iNumUniqueBonusesOnArea; 
					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestArea = pLoopArea;
					}
				}
			}

			if (pBestArea == NULL)
				break; // can't place bonus on any area

			areas_tried.insert(pBestArea->getID());
			// <advc.129>
			int iAdded = 0;
			int const iAreaLimit = std::min(2, 3 * pBestArea->getNumTiles()) +
					pBestArea->getNumTiles() / 25; // </advc.129>

			// Place the bonuses: (advc: some style changes from here on)

			int* aiShuffledIndices = shuffle(kMap.numPlots(), GC.getGame().getMapRand());
			for (int iI = 0; iI < kMap.numPlots() &&
				(bIgnoreAreaLimit || iAdded < iAreaLimit) && // advc.129
				kMap.getNumBonuses(eBonusType) < iTarget; iI++)
			{
				CvPlot& kRandPlot = kMap.getPlotByIndex(aiShuffledIndices[iI]);

				if (pBestArea != kRandPlot.area())
					continue;

				int const x = kRandPlot.getX();
				int const y = kRandPlot.getY();
				if (!canPlaceBonusAt(eBonusType, x, y, bIgnoreLatitude))
					continue;

				/*	<advc.129> About to place a cluster of eBonusType. Don't place that
					cluster near an earlier cluster (or any instance) of the same bonus class
					- that's what can lead to e.g. starts with 5 Gold. canPlaceBonusAt
					can't enforce this well b/c it doesn't know whether a cluster starts
					(distance needs to be heeded) or continues (distance mustn't be heeded). */
				int iClassToAvoid = pBonusInfo.getBonusClassType();
				/*  Only resources that occur in clusters are problematic. Not sure about
					the iClassToAvoid>0 clause. 0 is the "general" bonus class containing
					all the clustered resources except for Gold, Silver, Gems which I've
					moved to a separate class "precious". I.e. currently only double clusters
					of precious bonuses are avoided. Eliminating all double clusters might
					get in the way of (early) resource trades too much and make the map
					less exciting than it could be. */
				if(pBonusInfo.getGroupRand() > 0 && iClassToAvoid > 0)
				{
					bool bSkip = false;
					/*  Can't use pClassInfo.getUniqueRange() b/c this has to be
						0 for bonuses that appear in clusters. 5 hardcoded. */
					int const iDist = 5;
					for (PlotCircleIter it(x, y, iDist); it.hasNext(); ++it)
					{
						CvPlot const& p = *it;
						if (!p.sameArea(kRandPlot))
							continue;
						BonusTypes eOtherBonus = p.getBonusType();
						if(eOtherBonus != NO_BONUS && GC.getInfo(eOtherBonus).
							getBonusClassType() == iClassToAvoid &&
							GC.getInfo(eOtherBonus).getGroupRand() > 0)
						{
							bSkip = true;
							break;
						}
					}
					if(bSkip)
						continue;
				} // </advc.129>
				kRandPlot.setBonusType(eBonusType);
				// <dvc.129>
				iAdded++;
				iAdded += placeGroup(eBonusType, kRandPlot, bIgnoreLatitude); // Replacing the code below
				/*for (int iDX = -(pBonusInfo.getGroupRange()); iDX <= pBonusInfo.getGroupRange(); iDX++) {
					for (int iDY = -(pBonusInfo.getGroupRange()); iDY <= pBonusInfo.getGroupRange(); iDY++) {
						if (GC.getMap().getNumBonuses(eBonusType) < iBonusCount) {
							CvPlot* pLoopPlot	= plotXY(pPlot->getX(), pPlot->getY(), iDX, iDY);
							if (pLoopPlot != NULL && (pLoopPlot->area() == pBestArea)) {
								if (canPlaceBonusAt(eBonusType, pLoopPlot->getX(), pLoopPlot->getY(), bIgnoreLatitude)) {
									if (GC.getGame().getMapRandNum(100, "addUniqueBonusType") < pBonusInfo.getGroupRand())
										pLoopPlot->setBonusType(eBonusType);
				} } } } }*/

			}
			SAFE_DELETE_ARRAY(aiShuffledIndices);
		}
	}
}

void CvMapGenerator::addNonUniqueBonusType(BonusTypes eBonusType)
{
	int iBonusCount = calculateNumBonusesToAdd(eBonusType);
	if (iBonusCount == 0)
		return;

	int *aiShuffledIndices = shuffle(GC.getMap().numPlots(), GC.getGame().getMapRand());
	// advc.129: Moved into placeGroup
	//CvBonusInfo& pBonusInfo = GC.getInfo(eBonusType);
	bool const bIgnoreLatitude = GC.getPythonCaller()->isBonusIgnoreLatitude();
	for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
	{
		CvPlot& kPlot = GC.getMap().getPlotByIndex(aiShuffledIndices[iI]);
		if (!canPlaceBonusAt(eBonusType, kPlot.getX(), kPlot.getY(), bIgnoreLatitude))
			continue; // advc

		kPlot.setBonusType(eBonusType);
		iBonusCount--;
		// advc.129: Replacing the loop below
		iBonusCount -= placeGroup(eBonusType, kPlot, bIgnoreLatitude, iBonusCount);
		/*for (int iDX = -(pBonusInfo.getGroupRange()); iDX <= pBonusInfo.getGroupRange(); iDX++) {
			for (int iDY = -(pBonusInfo.getGroupRange()); iDY <= pBonusInfo.getGroupRange(); iDY++) {
				if (iBonusCount > 0) {
					CvPlot* pLoopPlot	= plotXY(kPlot.getX(), kPlot.getY(), iDX, iDY);
					if (pLoopPlot != NULL) {
						if (canPlaceBonusAt(eBonusType, pLoopPlot->getX(), pLoopPlot->getY(), bIgnoreLatitude)) {
							if (GC.getGame().getMapRandNum(100, "addNonUniqueBonusType") < pBonusInfo.getGroupRand()) {
								pLoopPlot->setBonusType(eBonusType);
								iBonusCount--;
		} } } } } }
		FAssertMsg(iBonusCount >= 0, "iBonusCount must be >= 0");*/
		if (iBonusCount == 0)
			break;
	}
	SAFE_DELETE_ARRAY(aiShuffledIndices);
}


// <advc.129>
int CvMapGenerator::placeGroup(BonusTypes eBonusType, CvPlot const& kCenter,
		bool bIgnoreLatitude, int iLimit)
{
	CvBonusInfo const& kBonus = GC.getInfo(eBonusType);
	// The one in the center is already placed, but that doesn't count here.
	int iPlaced = 0;
	std::vector<CvPlot*> apGroupRange;
	for (SquareIter it(kCenter, kBonus.getGroupRange()); it.hasNext(); ++it)
	{
		{
			CvPlot& p = *it;
			if(canPlaceBonusAt(eBonusType, p.getX(), p.getY(), bIgnoreLatitude))
				apGroupRange.push_back(&p);
		}
	}
	int sz = (int)apGroupRange.size();
	if(sz <= 0)
		return 0;
	std::vector<int> aiShuffled(sz);
	::shuffleVector(aiShuffled, GC.getGame().getMapRand());
	for(int j = 0; j < sz && iLimit > 0; j++)
	{
		int iProb = kBonus.getGroupRand();
		iProb = ::round(iProb * std::pow(2/3.0, iPlaced));
		if (GC.getGame().getMapRandNum(100, "addNonUniqueBonusType") < iProb)
		{
			apGroupRange[aiShuffled[j]]->setBonusType(eBonusType);
			iLimit--;
			iPlaced++;
		}
	}
	FAssert(iLimit >= 0);
	return iPlaced;
} // </advc.129>


void CvMapGenerator::addGoodies()  // advc: some style changes
{
	PROFILE_FUNC();

	if (GC.getPythonCaller()->addGoodies())
		return;

	gDLL->NiTextOut("Adding Goodies...");

	if (GC.getInfo(GC.getGame().getStartEra()).isNoGoodies())
		return;

	CvMap const& kMap = GC.getMap();
	int* aiShuffledIndices = shuffle(kMap.numPlots(), GC.getGame().getMapRand());
	FOR_EACH_ENUM(Improvement)
	{
		ImprovementTypes const eGoody = eLoopImprovement;
		if (!GC.getInfo(eGoody).isGoody())
			continue;
		int const iTilesPerGoody = GC.getInfo(eGoody).getTilesPerGoody();
		if (iTilesPerGoody <= 0)
			continue;
		// advc.opt: Count the goodies here instead of storing the counts at CvArea
		std::map<int,short> goodiesPerArea;
		for (int i = 0; i < kMap.numPlots(); i++)
		{
			CvPlot& kPlot = kMap.getPlotByIndex(aiShuffledIndices[i]);
			if (kPlot.isWater())
				continue;

			CvArea const& kArea = kPlot.getArea();
			if (goodiesPerArea[kArea.getID()] < // advc.opt: was kArea.getNumImprovements(eImprov)
				(kArea.getNumTiles() + iTilesPerGoody / 2) / iTilesPerGoody)
			{
				if (canPlaceGoodyAt(eGoody, kPlot.getX(), kPlot.getY()))
				{
					kPlot.setImprovementType(eGoody);
					goodiesPerArea[kArea.getID()]++; // advc.opt
				}
			}
		}
		gDLL->callUpdater(); // advc.opt: Moved out of the loop
	}
	SAFE_DELETE_ARRAY(aiShuffledIndices);
}


void CvMapGenerator::eraseRivers()
{
	CvMap const& kMap = GC.getMap();
	for (int i = 0; i < kMap.numPlots(); i++)
	{
		CvPlot& kPlot = kMap.getPlotByIndex(i);
		if (kPlot.isNOfRiver())
			kPlot.setNOfRiver(false, NO_CARDINALDIRECTION);
		if (kPlot.isWOfRiver())
			kPlot.setWOfRiver(false, NO_CARDINALDIRECTION);
	}
}

void CvMapGenerator::eraseFeatures()
{
	CvMap const& kMap = GC.getMap();
	for (int i = 0; i < kMap.numPlots(); i++)
		kMap.getPlotByIndex(i).setFeatureType(NO_FEATURE);
}

void CvMapGenerator::eraseBonuses()
{
	CvMap const& kMap = GC.getMap();
	for (int i = 0; i < kMap.numPlots(); i++)
		kMap.getPlotByIndex(i).setBonusType(NO_BONUS);
}

void CvMapGenerator::eraseGoodies()
{
	CvMap const& kMap = GC.getMap();
	for (int i = 0; i < kMap.numPlots(); i++)
		kMap.getPlotByIndex(i).removeGoody();
	// (advc: isGoody check moved into removeGoody)
}


void CvMapGenerator::generateRandomMap()
{
	PROFILE_FUNC();

	CvPythonCaller const& py = *GC.getPythonCaller();
	py.callMapFunction("beforeGeneration");
	if (py.generateRandomMap()) // will call applyMapData when done
		return;

	char buf[256];

	sprintf(buf, "Generating Random Map %S, %S...", gDLL->getMapScriptName().GetCString(), GC.getInfo(GC.getMap().getWorldSize()).getDescription());
	gDLL->NiTextOut(buf);

	generatePlotTypes();
	generateTerrain();
	/* advc.300: Already done in CvMap::calculateAreas, but when calculateAreas
	   is called during map generation, tile yields aren't yet set. */
	GC.getMap().computeShelves();
	// <advc.108>
	if (py.isAnyCustomMapOptionSetTo(gDLL->getText("TXT_KEY_MAP_BALANCED")))
		GC.getGame().setStartingPlotNormalizationLevel(CvGame::NORMALIZE_HIGH);
	else GC.getGame().setStartingPlotNormalizationLevel(CvGame::NORMALIZE_DEFAULT);
	// </advc.108>
}

void CvMapGenerator::generatePlotTypes()
{
	int const iNumPlots = GC.getMap().numPlots();
	int* paiPlotTypes = new int[iNumPlots];
	if (!GC.getPythonCaller()->generatePlotTypes(paiPlotTypes, iNumPlots))
	{
		for (int iI = 0; iI < iNumPlots; iI++)
			paiPlotTypes[iI] = PLOT_LAND;
	}
	setPlotTypes(paiPlotTypes);
	SAFE_DELETE_ARRAY(paiPlotTypes);
}

void CvMapGenerator::generateTerrain()
{
	PROFILE_FUNC();

	std::vector<int> pyTerrain;
	CvMap const& kMap = GC.getMap();
	if (GC.getPythonCaller()->generateTerrainTypes(pyTerrain, kMap.numPlots()))
	{
		for (int i = 0; i < kMap.numPlots(); i++)
		{
			//gDLL->callUpdater(); // addvc.003b
			kMap.plotByIndex(i)->setTerrainType((TerrainTypes)pyTerrain[i], false, false);
		}
	}
}


void CvMapGenerator::afterGeneration()
{
	PROFILE_FUNC();
	// Allows for user-defined Python Actions for map generation after it's already been created
	GC.getPythonCaller()->callMapFunction("afterGeneration");
	GC.getLogger().logMapStats(); // advc.mapstat
}

void CvMapGenerator::setPlotTypes(const int* paiPlotTypes)
{
	CvMap& kMap = GC.getMap();
	for (int iI = 0; iI < kMap.numPlots(); iI++)
	{
		//gDLL->callUpdater(); // advc.opt: Not needed I reckon
		kMap.getPlotByIndex(iI).setPlotType((PlotTypes)paiPlotTypes[iI], false, false);
	}

	kMap.recalculateAreas();

	for (int iI = 0; iI < kMap.numPlots(); iI++)
	{
		//gDLL->callUpdater(); // advc.opt
		CvPlot& kPlot = kMap.getPlotByIndex(iI);
		if (kPlot.isWater())
		{
			if (kPlot.isAdjacentToLand())
				kPlot.setTerrainType(GC.getWATER_TERRAIN(true), false, false);
			else kPlot.setTerrainType(GC.getWATER_TERRAIN(false), false, false);
		}
	}
}


int CvMapGenerator::getRiverValueAtPlot(CvPlot* pPlot)
{
	bool bOverride=false;
	int pyValue = GC.getPythonCaller()->riverValue(*pPlot, bOverride);
	if (bOverride)
		return pyValue;
	int iSum = pyValue; // Add to value from Python

	iSum += (NUM_PLOT_TYPES - pPlot->getPlotType()) * 20;

	for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		CvPlot* pAdjacentPlot = plotDirection(pPlot->getX(), pPlot->getY(), (DirectionTypes)iI);
		if (pAdjacentPlot != NULL)
			iSum += (NUM_PLOT_TYPES - pAdjacentPlot->getPlotType());
		else iSum += (NUM_PLOT_TYPES * 10);
	}
	CvRandom riverRand;
	riverRand.init((pPlot->getX() * 43251267) + (pPlot->getY() * 8273903));
	iSum += riverRand.get(10, "River Rand");

	return iSum;
}

int CvMapGenerator::calculateNumBonusesToAdd(BonusTypes eBonusType)
{
	CvBonusInfo& pBonusInfo = GC.getInfo(eBonusType);

	// Calculate iBonusCount, the amount of this bonus to be placed:

	int iRand1 = GC.getGame().getMapRandNum(pBonusInfo.getRandAppearance1(), "calculateNumBonusesToAdd-1");
	int iRand2 = GC.getGame().getMapRandNum(pBonusInfo.getRandAppearance2(), "calculateNumBonusesToAdd-2");
	int iRand3 = GC.getGame().getMapRandNum(pBonusInfo.getRandAppearance3(), "calculateNumBonusesToAdd-3");
	int iRand4 = GC.getGame().getMapRandNum(pBonusInfo.getRandAppearance4(), "calculateNumBonusesToAdd-4");
	int iBaseCount = pBonusInfo.getConstAppearance() + iRand1 + iRand2 + iRand3 + iRand4;

	bool const bIgnoreLatitude = GC.getPythonCaller()->isBonusIgnoreLatitude();

	// Calculate iNumPossible, the number of plots that are eligible to have this bonus:

	int iLandTiles = 0;
	if (pBonusInfo.getTilesPer() > 0)
	{
		int iNumPossible = 0;
		for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
		{
			CvPlot const& kPlot = GC.getMap().getPlotByIndex(iI);
			if (kPlot.canHaveBonus(eBonusType, bIgnoreLatitude))
				iNumPossible++;
		}
		// <advc.129>
		if(GC.getDefineBOOL("SUBLINEAR_BONUS_QUANTITIES"))
		{
			int iSubtrahend = pBonusInfo.getTilesPer(); // Typically 16 or 32
			int iRemainder = iNumPossible;
			int iResult = 0;
			/* Place one for the first, say, 16 tiles, the next after 17, then 18 ...
			   i.e. number of resources placed grows sublinearly with the number of
			   eligible plots. */
			while(true)
			{
				iRemainder -= iSubtrahend;
				if(iRemainder < 0)
					break;
				iResult++;
				iSubtrahend++;
			}
			iLandTiles += iResult;
		}
		else // </advc.129>
			iLandTiles += (iNumPossible / pBonusInfo.getTilesPer());
	}

	int iPlayers = (GC.getGame().countCivPlayersAlive() * pBonusInfo.getPercentPerPlayer()) / 100;
	int iBonusCount = (iBaseCount * (iLandTiles + iPlayers)) / 100;
	iBonusCount = std::max(1, iBonusCount);
	return iBonusCount;
}
