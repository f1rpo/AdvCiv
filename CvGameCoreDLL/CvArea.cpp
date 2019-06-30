// area.cpp

#include "CvGameCoreDLL.h"
#include "CvMap.h"
#include "CvGamePlay.h"
#include "CvInfos.h"
#include "CvDLLInterfaceIFaceBase.h"

// Public Functions...

CvArea::CvArea()
{
	m_aiUnitsPerPlayer = new int[MAX_PLAYERS];
	m_aiAnimalsPerPlayer = new int[MAX_PLAYERS];
	m_aiCitiesPerPlayer = new int[MAX_PLAYERS];
	m_aiPopulationPerPlayer = new int[MAX_PLAYERS];
	m_aiBuildingGoodHealth = new int[MAX_PLAYERS];
	m_aiBuildingBadHealth = new int[MAX_PLAYERS];
	m_aiBuildingHappiness = new int[MAX_PLAYERS];
	m_aiTradeRoutes = new int[MAX_PLAYERS]; // advc.310
	m_aiFreeSpecialist = new int[MAX_PLAYERS];
	m_aiPower = new int[MAX_PLAYERS];
	m_aiBestFoundValue = new int[MAX_PLAYERS];
	m_aiNumRevealedTiles = new int[MAX_TEAMS];
	m_aiCleanPowerCount = new int[MAX_TEAMS];
	m_aiBorderObstacleCount = new int[MAX_TEAMS];

	m_aeAreaAIType = new AreaAITypes[MAX_TEAMS];

	m_aTargetCities = new IDInfo[MAX_PLAYERS];

	m_aaiYieldRateModifier = new int*[MAX_PLAYERS];
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		m_aaiYieldRateModifier[i] = new int[NUM_YIELD_TYPES];
	}
	m_aaiNumTrainAIUnits = new int*[MAX_PLAYERS];
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		m_aaiNumTrainAIUnits[i] = new int[NUM_UNITAI_TYPES];
	}
	m_aaiNumAIUnits = new int*[MAX_PLAYERS];
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		m_aaiNumAIUnits[i] = new int[NUM_UNITAI_TYPES];
	}

	m_paiNumBonuses = NULL;
	m_paiNumImprovements = NULL;


	reset(0, false, true);
}


CvArea::~CvArea()
{
	uninit();

	SAFE_DELETE_ARRAY(m_aiUnitsPerPlayer);
	SAFE_DELETE_ARRAY(m_aiAnimalsPerPlayer);
	SAFE_DELETE_ARRAY(m_aiCitiesPerPlayer);
	SAFE_DELETE_ARRAY(m_aiPopulationPerPlayer);
	SAFE_DELETE_ARRAY(m_aiBuildingGoodHealth);
	SAFE_DELETE_ARRAY(m_aiBuildingBadHealth);
	SAFE_DELETE_ARRAY(m_aiBuildingHappiness);
	SAFE_DELETE_ARRAY(m_aiTradeRoutes); // advc.310
	SAFE_DELETE_ARRAY(m_aiFreeSpecialist);
	SAFE_DELETE_ARRAY(m_aiPower);
	SAFE_DELETE_ARRAY(m_aiBestFoundValue);
	SAFE_DELETE_ARRAY(m_aiNumRevealedTiles);
	SAFE_DELETE_ARRAY(m_aiCleanPowerCount);
	SAFE_DELETE_ARRAY(m_aiBorderObstacleCount);
	SAFE_DELETE_ARRAY(m_aeAreaAIType);
	SAFE_DELETE_ARRAY(m_aTargetCities);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		SAFE_DELETE_ARRAY(m_aaiYieldRateModifier[i]);
	}
	SAFE_DELETE_ARRAY(m_aaiYieldRateModifier);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		SAFE_DELETE_ARRAY(m_aaiNumTrainAIUnits[i]);
	}
	SAFE_DELETE_ARRAY(m_aaiNumTrainAIUnits);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		SAFE_DELETE_ARRAY(m_aaiNumAIUnits[i]);
	}
	SAFE_DELETE_ARRAY(m_aaiNumAIUnits);
}


void CvArea::init(int iID, bool bWater)
{
	//--------------------------------
	// Init saved data
	reset(iID, bWater);

	//--------------------------------
	// Init non-saved data

	//--------------------------------
	// Init other game data
}


void CvArea::uninit()
{
	SAFE_DELETE_ARRAY(m_paiNumBonuses);
	SAFE_DELETE_ARRAY(m_paiNumImprovements);
}


// FUNCTION: reset()
// Initializes data members that are serialized.
void CvArea::reset(int iID, bool bWater, bool bConstructorCall)
{
	int iI, iJ;

	//--------------------------------
	// Uninit class
	uninit();

	m_iID = iID;
	m_iNumTiles = 0;
	m_iNumOwnedTiles = 0;
	m_iNumRiverEdges = 0;
	m_iNumUnits = 0;
	m_iNumCities = 0;
	m_iTotalPopulation = 0;
	m_iNumStartingPlots = 0;
	m_iBarbarianCitiesEver = 0; // advc.300
	// <advc.030>
	m_bLake = false;
	m_iRepresentativeAreaId = iID;
	// </advc.030>
	m_bWater = bWater;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_aiUnitsPerPlayer[iI] = 0;
		m_aiAnimalsPerPlayer[iI] = 0;
		m_aiCitiesPerPlayer[iI] = 0;
		m_aiPopulationPerPlayer[iI] = 0;
		m_aiBuildingGoodHealth[iI] = 0;
		m_aiBuildingBadHealth[iI] = 0;
		m_aiBuildingHappiness[iI] = 0;
		m_aiTradeRoutes[iI] = 0; // advc.310
		m_aiFreeSpecialist[iI] = 0;
		m_aiPower[iI] = 0;
		m_aiBestFoundValue[iI] = 0;
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		m_aiNumRevealedTiles[iI] = 0;
		m_aiCleanPowerCount[iI] = 0;
		m_aiBorderObstacleCount[iI] = 0;
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		m_aeAreaAIType[iI] = NO_AREAAI;
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_aTargetCities[iI].reset();
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		for (iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
		{
			m_aaiYieldRateModifier[iI][iJ] = 0;
		}

		for (iJ = 0; iJ < NUM_UNITAI_TYPES; iJ++)
		{
			m_aaiNumTrainAIUnits[iI][iJ] = 0;
			m_aaiNumAIUnits[iI][iJ] = 0;
		}
	}

	if (!bConstructorCall)
	{
		FAssertMsg(0 < GC.getNumBonusInfos(), "GC.getNumBonusInfos() is negative but an array is being allocated in CvArea::reset");
		m_paiNumBonuses = new int[GC.getNumBonusInfos()];
		for (iI = 0; iI < GC.getNumBonusInfos(); iI++)
		{
			m_paiNumBonuses[iI] = 0;
		}

		FAssertMsg(0 < GC.getNumImprovementInfos(), "GC.getNumImprovementInfos() is negative but an array is being allocated in CvArea::reset");
		m_paiNumImprovements = new int[GC.getNumImprovementInfos()];
		for (iI = 0; iI < GC.getNumImprovementInfos(); iI++)
		{
			m_paiNumImprovements[iI] = 0;
		}
	}
}


void CvArea::setID(int iID)														
{
	m_iID = iID;
	m_iRepresentativeAreaId = iID; // advc.030
}


int CvArea::calculateTotalBestNatureYield() const
{
	int iCount = 0;

	for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(iI);

		if (pLoopPlot->getArea() == getID())
		{
			iCount += pLoopPlot->calculateTotalBestNatureYield(NO_TEAM);
		}
	}

	return iCount;
}


int CvArea::countCoastalLand() const
{
	if (isWater())
	{
		return 0;
	}

	int iCount = 0;

	for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(iI);

		if (pLoopPlot->getArea() == getID())
		{
			if (pLoopPlot->isCoastalLand())
			{
				iCount++;
			}
		}
	}

	return iCount;
}


int CvArea::countNumUniqueBonusTypes() const
{
	int iCount;
	int iI;

	iCount = 0;

	for (iI = 0; iI < GC.getNumBonusInfos(); iI++)
	{
		if (getNumBonuses((BonusTypes)iI) > 0)
		{
			if (GC.getBonusInfo((BonusTypes)iI).isOneArea())
			{
				iCount++;
			}
		}
	}

	return iCount;
}


int CvArea::countHasReligion(ReligionTypes eReligion, PlayerTypes eOwner) const
{
	CvCity* pLoopCity;
	int iCount;
	int iLoop;
	int iI;

	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if ((eOwner == NO_PLAYER) || (iI == eOwner))
			{
				for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
				{
					if (pLoopCity->area()->getID() == getID())
					{
						if (pLoopCity->isHasReligion(eReligion))
						{
							iCount++;
						}
					}
				}
			}
		}
	}

	return iCount;
}

int CvArea::countHasCorporation(CorporationTypes eCorporation, PlayerTypes eOwner) const
{
	int iCount = 0;

	for (int iI = 0; iI < MAX_PLAYERS; ++iI)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if ((eOwner == NO_PLAYER) || (iI == eOwner))
			{
				int iLoop;
				for (CvCity* pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); NULL != pLoopCity; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
				{
					if (pLoopCity->area()->getID() == getID())
					{
						if (pLoopCity->isHasCorporation(eCorporation))
						{
							++iCount;
						}
					}
				}
			}
		}
	}

	return iCount;
}


void CvArea::updateLake(bool bCheckRepr) {

	PROFILE("CvArea::updateLake");
	m_bLake = false;
	if(!isWater())
		return;
	int iTotalTiles = getNumTiles();
	if(iTotalTiles > GC.getLAKE_MAX_AREA_SIZE())
		return;
	if(!bCheckRepr) {
		m_bLake = true;
		return;
	}
	CvMap& m = GC.getMap(); int foo=-1;
	for(CvArea* other = m.firstArea(&foo); other != NULL; other = m.nextArea(&foo)) {
		if(other->m_iRepresentativeAreaId == m_iRepresentativeAreaId && other->m_iID != m_iID) {
			iTotalTiles += other->getNumTiles();
			if(iTotalTiles > GC.getLAKE_MAX_AREA_SIZE())
				return;
		}
	}
	m_bLake = true;
}

void CvArea::setRepresentativeArea(int eArea) {

	m_iRepresentativeAreaId = eArea;
}

int CvArea::getRepresentativeArea() const {

	return m_iRepresentativeAreaId;
}

/*  Replacement for the BtS area()==area() checks. Mostly used for
	performance reasons before costlier more specific checks. */
bool CvArea::canBeEntered(CvArea const& kFrom, CvUnit const* u) const {

	if(getID() == kFrom.getID())
		return true;
	/*  If I wanted to support canMoveAllTerrain here, then I couldn't do
		anything more when u==NULL. So that's not supported. */
	if(isWater() == kFrom.isWater() && (m_iRepresentativeAreaId !=
			kFrom.m_iRepresentativeAreaId || (u != NULL && !u->canMoveImpassable())))
		return false;
	/*  Can't rule out movement between water and land without knowing if the
		unit is a ship inside a city or a land unit aboard a transport */
	if(u == NULL)
		return true;
	if(isWater() && (u->getDomainType() != DOMAIN_SEA || !u->plot()->isCity()))
		return false;
	if(!isWater() && (u->getDomainType() != DOMAIN_LAND || !u->isCargo()))
		return false;
	/*  In the cases above, movement may actually still be possible if it's
		a sea unit entering a city or a land unit boarding a transport.
		So this function really assumes that u enters a hostile plot. */
	return true;
} // </advc.030>

void CvArea::changeNumTiles(int iChange)
{
	if(iChange == 0)
		return;

	bool bOldLake = isLake();

	m_iNumTiles = (m_iNumTiles + iChange);
	FAssert(getNumTiles() >= 0);

	if (bOldLake != isLake())
	{
		GC.getMap().updateIrrigated();
		GC.getMap().updateYield();
	}
}


void CvArea::changeNumOwnedTiles(int iChange)									
{
	m_iNumOwnedTiles = (m_iNumOwnedTiles + iChange);
	FAssert(getNumOwnedTiles() >= 0);
	FAssert(getNumUnownedTiles() >= 0);
}


// <advc.300>
std::pair<int,int> CvArea::countOwnedUnownedHabitableTiles(bool bIgnoreBarb) const {

	std::pair<int,int> r(0, 0);
	CvMap const& kMap = GC.getMap();
	for(int i = 0; i < kMap.numPlots(); i++) {
		CvPlot* pPlot = kMap.plotByIndex(i);
		if(pPlot == NULL || pPlot->area() == NULL || pPlot->area()->getID() != getID()
				|| !pPlot->isHabitable())
			continue;
		if(pPlot->isOwned() && (!bIgnoreBarb ||
				pPlot->getOwner() != BARBARIAN_PLAYER))
			r.first++;
		else r.second++;
	}
	return r;
}

int CvArea::countCivCities() const {

	int r = 0;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
		r += getCitiesPerPlayer((PlayerTypes)i);
	return r;
}

int CvArea::countCivs(bool bSubtractOCC) const {

	/* Perhaps an owned tile (across the sea) should suffice, but tiles-per-civ
	   aren't cached/ serialized (yet). */
	int r = 0;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		PlayerTypes ePlayer = (PlayerTypes)i;
		if(getCitiesPerPlayer(ePlayer) > 0 &&
				(!bSubtractOCC || !GET_PLAYER(ePlayer).isHuman() ||
				!GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE)))
			r++;
	}
	return r;
}


bool CvArea::hasAnyAreaPlayerBonus(BonusTypes eBonus) const {

	for(int i = 0; i < MAX_PLAYERS; i++) {
		PlayerTypes ePlayer = (PlayerTypes)i;
		// Barbarian, minor civ, anything goes so long as there's a city.
		if(getCitiesPerPlayer(ePlayer) > 0 && GET_PLAYER(ePlayer).hasBonus(eBonus))
			return true;
	}
	return false;
}

int CvArea::getBarbarianCitiesEverCreated() const {

	return m_iBarbarianCitiesEver;
}

void CvArea::reportBarbarianCityCreated() {

	m_iBarbarianCitiesEver++;
} // </advc.300>


void CvArea::changeNumRiverEdges(int iChange)									
{
	m_iNumRiverEdges = (m_iNumRiverEdges + iChange);
	FAssert(getNumRiverEdges() >= 0);
}


int CvArea::getTotalPopulation() const					
{
	return m_iTotalPopulation;
}


void CvArea::changeNumStartingPlots(int iChange)
{
	m_iNumStartingPlots = m_iNumStartingPlots + iChange;
	FAssert(getNumStartingPlots() >= 0);
}


int CvArea::getUnitsPerPlayer(PlayerTypes eIndex) const												
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	return m_aiUnitsPerPlayer[eIndex];
}


void CvArea::changeUnitsPerPlayer(PlayerTypes eIndex, int iChange)							
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	m_iNumUnits = (m_iNumUnits + iChange);
	FAssert(getNumUnits() >= 0);
	m_aiUnitsPerPlayer[eIndex] = (m_aiUnitsPerPlayer[eIndex] + iChange);
	FAssert(getUnitsPerPlayer(eIndex) >= 0);
}


int CvArea::getAnimalsPerPlayer(PlayerTypes eIndex) const			
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	return m_aiAnimalsPerPlayer[eIndex];
}


void CvArea::changeAnimalsPerPlayer(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	m_aiAnimalsPerPlayer[eIndex] = (m_aiAnimalsPerPlayer[eIndex] + iChange);
	FAssert(getAnimalsPerPlayer(eIndex) >= 0);
}


int CvArea::getCitiesPerPlayer(PlayerTypes eIndex,
		// <advc.030b>
		bool bCheckAdjacentCoast) const {
	/*  Perhaps this parameter isn't really needed, but this function gets called
		from so many places that I can't check if one of them might have a problem
		with water areas having a positive city count. */
	if(!bCheckAdjacentCoast && isWater())
		return 0; // </advc.030b>
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	return m_aiCitiesPerPlayer[eIndex];
}


void CvArea::changeCitiesPerPlayer(PlayerTypes eIndex, int iChange)							
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	m_iNumCities = (m_iNumCities + iChange);
	reportBarbarianCityCreated(); // advc.300
	FAssert(getNumCities() >= 0);
	m_aiCitiesPerPlayer[eIndex] = (m_aiCitiesPerPlayer[eIndex] + iChange);
	FAssert(getCitiesPerPlayer(eIndex, true) >= 0); // advc.030b
}


int CvArea::getPopulationPerPlayer(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	return m_aiPopulationPerPlayer[eIndex];
}


void CvArea::changePopulationPerPlayer(PlayerTypes eIndex, int iChange)							
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	m_iTotalPopulation = (m_iTotalPopulation + iChange);
	FAssert(getTotalPopulation() >= 0);
	m_aiPopulationPerPlayer[eIndex] = (m_aiPopulationPerPlayer[eIndex] + iChange);
	FAssert(getPopulationPerPlayer(eIndex) >= 0);
}


int CvArea::getBuildingGoodHealth(PlayerTypes eIndex) const 
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	return m_aiBuildingGoodHealth[eIndex];
}


void CvArea::changeBuildingGoodHealth(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");

	if (iChange != 0)
	{
		m_aiBuildingGoodHealth[eIndex] = (m_aiBuildingGoodHealth[eIndex] + iChange);
		FAssert(getBuildingGoodHealth(eIndex) >= 0);

		GET_PLAYER(eIndex).AI_makeAssignWorkDirty();
	}
}


int CvArea::getBuildingBadHealth(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	return m_aiBuildingBadHealth[eIndex];
}


void CvArea::changeBuildingBadHealth(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");

	if (iChange != 0)
	{
		m_aiBuildingBadHealth[eIndex] = (m_aiBuildingBadHealth[eIndex] + iChange);
		FAssert(getBuildingBadHealth(eIndex) >= 0);

		GET_PLAYER(eIndex).AI_makeAssignWorkDirty();
	}
}


int CvArea::getBuildingHappiness(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	return m_aiBuildingHappiness[eIndex];
}


void CvArea::changeBuildingHappiness(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");

	if (iChange != 0)
	{
		m_aiBuildingHappiness[eIndex] = (m_aiBuildingHappiness[eIndex] + iChange);

		GET_PLAYER(eIndex).AI_makeAssignWorkDirty();
	}
}

// <advc.310>
int CvArea::getTradeRoutes(PlayerTypes eIndex) const {

	FAssert(eIndex >= 0); FAssert(eIndex < MAX_PLAYERS);
	return m_aiTradeRoutes[eIndex];
}

void CvArea::changeTradeRoutes(PlayerTypes eIndex, int iChange) {

	FAssert(eIndex >= 0); FAssert(eIndex < MAX_PLAYERS);
	if(iChange == 0)
		return;
	m_aiTradeRoutes[eIndex] += iChange;
	FAssert(getTradeRoutes(eIndex) >= 0);
	GET_PLAYER(eIndex).updateTradeRoutes();
} // </advc.310>

int CvArea::getFreeSpecialist(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	return m_aiFreeSpecialist[eIndex];
}


void CvArea::changeFreeSpecialist(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");

	if (iChange != 0)
	{
		m_aiFreeSpecialist[eIndex] = (m_aiFreeSpecialist[eIndex] + iChange);
		FAssert(getFreeSpecialist(eIndex) >= 0);

		GET_PLAYER(eIndex).AI_makeAssignWorkDirty();
	}
}


int CvArea::getPower(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	return m_aiPower[eIndex];
}


void CvArea::changePower(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	m_aiPower[eIndex] = (m_aiPower[eIndex] + iChange);
	/*  <advc.006> Can happen when continuing a game after changing a
		unit power value in XML */
	if(m_aiPower[eIndex] < 0) {
		m_aiPower[eIndex] = 0; // </advc.006>
		FAssertMsg(getPower(eIndex) >= 0, "OK when playing from an old savegame "
				"with an updated version of the mod");
	}
}


int CvArea::getBestFoundValue(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	return m_aiBestFoundValue[eIndex];
}


void CvArea::setBestFoundValue(PlayerTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	m_aiBestFoundValue[eIndex] = iNewValue;
	FAssert(getBestFoundValue(eIndex) >= 0);
}


int CvArea::getNumRevealedTiles(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	return m_aiNumRevealedTiles[eIndex];
}


void CvArea::changeNumRevealedTiles(TeamTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	m_aiNumRevealedTiles[eIndex] = (m_aiNumRevealedTiles[eIndex] + iChange);
	FAssert(getNumRevealedTiles(eIndex) >= 0);
}


int CvArea::getCleanPowerCount(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be < MAX_TEAMS");
	return m_aiCleanPowerCount[eIndex];
}


bool CvArea::isCleanPower(TeamTypes eIndex) const
{
	return (getCleanPowerCount(eIndex) > 0);
}


void CvArea::changeCleanPowerCount(TeamTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be < MAX_TEAMS");

	if(iChange == 0)
		return;

	bool bOldCleanPower = isCleanPower(eIndex);

	m_aiCleanPowerCount[eIndex] = (m_aiCleanPowerCount[eIndex] + iChange);

	if (bOldCleanPower != isCleanPower(eIndex))
	{
		GET_TEAM(eIndex).updateCommerce();
		GET_TEAM(eIndex).updatePowerHealth();

		if (eIndex == GC.getGame().getActiveTeam())
		{
			gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
		}
	}
}


int CvArea::getBorderObstacleCount(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be < MAX_TEAMS");
	return m_aiBorderObstacleCount[eIndex];
}

bool CvArea::isBorderObstacle(TeamTypes eIndex) const
{
	return (getBorderObstacleCount(eIndex) > 0);
}


void CvArea::changeBorderObstacleCount(TeamTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be < MAX_TEAMS");

	m_aiBorderObstacleCount[eIndex] += iChange;

	if (iChange > 0 && m_aiBorderObstacleCount[eIndex] == iChange)
	{
		GC.getMap().verifyUnitValidPlot();
	}
}



AreaAITypes CvArea::getAreaAIType(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be < MAX_TEAMS");
	return m_aeAreaAIType[eIndex];
}


void CvArea::setAreaAIType(TeamTypes eIndex, AreaAITypes eNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be < MAX_TEAMS");
	m_aeAreaAIType[eIndex] = eNewValue;
}


CvCity* CvArea::getTargetCity(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");
	return getCity(m_aTargetCities[eIndex]);
}


void CvArea::setTargetCity(PlayerTypes eIndex, CvCity* pNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be < MAX_PLAYERS");

	if (pNewValue != NULL)
	{
		m_aTargetCities[eIndex] = pNewValue->getIDInfo();
	}
	else
	{
		m_aTargetCities[eIndex].reset();
	}
}


int CvArea::getYieldRateModifier(PlayerTypes eIndex1, YieldTypes eIndex2) const
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be >= 0");
	FAssertMsg(eIndex1 < MAX_PLAYERS, "eIndex1 is expected to be < MAX_PLAYERS");
	FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be >= 0");
	FAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be < NUM_YIELD_TYPES");
	return m_aaiYieldRateModifier[eIndex1][eIndex2];
}


void CvArea::changeYieldRateModifier(PlayerTypes eIndex1, YieldTypes eIndex2, int iChange)
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be >= 0");
	FAssertMsg(eIndex1 < MAX_PLAYERS, "eIndex1 is expected to be < MAX_PLAYERS");
	FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be >= 0");
	FAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aaiYieldRateModifier[eIndex1][eIndex2] = (m_aaiYieldRateModifier[eIndex1][eIndex2] + iChange);
		
		GET_PLAYER(eIndex1).invalidateYieldRankCache(eIndex2);

		if (eIndex2 == YIELD_COMMERCE)
		{
			GET_PLAYER(eIndex1).updateCommerce();
		}

		GET_PLAYER(eIndex1).AI_makeAssignWorkDirty();

		if (GET_PLAYER(eIndex1).getTeam() == GC.getGame().getActiveTeam())
		{
			gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
		}
	}
}


int CvArea::getNumTrainAIUnits(PlayerTypes eIndex1, UnitAITypes eIndex2) const
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be >= 0");
	FAssertMsg(eIndex1 < MAX_PLAYERS, "eIndex1 is expected to be < MAX_PLAYERS");
	FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be >= 0");
	FAssertMsg(eIndex2 < NUM_UNITAI_TYPES, "eIndex2 is expected to be < NUM_UNITAI_TYPES");
	return m_aaiNumTrainAIUnits[eIndex1][eIndex2];
}


void CvArea::changeNumTrainAIUnits(PlayerTypes eIndex1, UnitAITypes eIndex2, int iChange)
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be >= 0");
	FAssertMsg(eIndex1 < MAX_PLAYERS, "eIndex1 is expected to be < MAX_PLAYERS");
	FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be >= 0");
	FAssertMsg(eIndex2 < NUM_UNITAI_TYPES, "eIndex2 is expected to be < NUM_UNITAI_TYPES");
	m_aaiNumTrainAIUnits[eIndex1][eIndex2] = (m_aaiNumTrainAIUnits[eIndex1][eIndex2] + iChange);
	FAssert(getNumTrainAIUnits(eIndex1, eIndex2) >= 0);
}


int CvArea::getNumAIUnits(PlayerTypes eIndex1, UnitAITypes eIndex2) const
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be >= 0");
	FAssertMsg(eIndex1 < MAX_PLAYERS, "eIndex1 is expected to be < MAX_PLAYERS");
	FAssertMsg(eIndex2 < NUM_UNITAI_TYPES, "eIndex2 is expected to be < NUM_UNITAI_TYPES");
	// <advc.124> NO_UNITAI counts all units of eIndex1
	//FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be >= 0");
	if(eIndex2 < 0) {
		int r = 0;
		for(int i = 0; i < NUM_UNITAI_TYPES; i++)
			r += m_aaiNumAIUnits[eIndex1][i];
		return r;
	} // </advc.124>
	return m_aaiNumAIUnits[eIndex1][eIndex2];
}


void CvArea::changeNumAIUnits(PlayerTypes eIndex1, UnitAITypes eIndex2, int iChange)
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be >= 0");
	FAssertMsg(eIndex1 < MAX_PLAYERS, "eIndex1 is expected to be < MAX_PLAYERS");
	FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be >= 0");
	FAssertMsg(eIndex2 < NUM_UNITAI_TYPES, "eIndex2 is expected to be < NUM_UNITAI_TYPES");
	m_aaiNumAIUnits[eIndex1][eIndex2] = (m_aaiNumAIUnits[eIndex1][eIndex2] + iChange);
	FAssert(getNumAIUnits(eIndex1, eIndex2) >= 0);
}


int CvArea::getNumBonuses(BonusTypes eBonus) const
{
	FAssertMsg(eBonus >= 0, "eBonus expected to be >= 0");
	FAssertMsg(eBonus < GC.getNumBonusInfos(), "eBonus expected to be < GC.getNumBonusInfos");
	return m_paiNumBonuses[eBonus];
}


int CvArea::getNumTotalBonuses() const
{
	int iTotal = 0;

	for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
	{
		iTotal += m_paiNumBonuses[iI];
	}

	return iTotal;
}


void CvArea::changeNumBonuses(BonusTypes eBonus, int iChange)					
{
	FAssertMsg(eBonus >= 0, "eBonus expected to be >= 0");
	FAssertMsg(eBonus < GC.getNumBonusInfos(), "eBonus expected to be < GC.getNumBonusInfos");
	m_paiNumBonuses[eBonus] = (m_paiNumBonuses[eBonus] + iChange);
	FAssert(getNumBonuses(eBonus) >= 0);
}


int CvArea::getNumImprovements(ImprovementTypes eImprovement) const
{
	FAssertMsg(eImprovement >= 0, "eImprovement expected to be >= 0");
	FAssertMsg(eImprovement < GC.getNumImprovementInfos(), "eImprovement expected to be < GC.getNumImprovementInfos");
	return m_paiNumImprovements[eImprovement];
}


void CvArea::changeNumImprovements(ImprovementTypes eImprovement, int iChange)	
{
	FAssertMsg(eImprovement >= 0, "eImprovement expected to be >= 0");
	FAssertMsg(eImprovement < GC.getNumImprovementInfos(), "eImprovement expected to be < GC.getNumImprovementInfos");
	m_paiNumImprovements[eImprovement] = (m_paiNumImprovements[eImprovement] + iChange);
	FAssert(getNumImprovements(eImprovement) >= 0);
}


void CvArea::read(FDataStreamBase* pStream)
{
	int iI;

	// Init saved data
	reset();

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iID);
	pStream->Read(&m_iNumTiles);
	pStream->Read(&m_iNumOwnedTiles);
	pStream->Read(&m_iNumRiverEdges);
	pStream->Read(&m_iNumUnits);
	pStream->Read(&m_iNumCities);
	pStream->Read(&m_iTotalPopulation);
	pStream->Read(&m_iNumStartingPlots);
	pStream->Read(&m_iBarbarianCitiesEver); // advc.300

	pStream->Read(&m_bWater);
	// <advc.030>
	if(uiFlag >= 1) {
		pStream->Read(&m_bLake);
		pStream->Read(&m_iRepresentativeAreaId);
	}
	else {
		updateLake(false);
		m_iRepresentativeAreaId = m_iID;
	} // </advc.030>
	pStream->Read(MAX_PLAYERS, m_aiUnitsPerPlayer);
	pStream->Read(MAX_PLAYERS, m_aiAnimalsPerPlayer);
	pStream->Read(MAX_PLAYERS, m_aiCitiesPerPlayer);
	pStream->Read(MAX_PLAYERS, m_aiPopulationPerPlayer);
	pStream->Read(MAX_PLAYERS, m_aiBuildingGoodHealth);
	pStream->Read(MAX_PLAYERS, m_aiBuildingBadHealth);
	pStream->Read(MAX_PLAYERS, m_aiBuildingHappiness);
	pStream->Read(MAX_PLAYERS, m_aiTradeRoutes); // advc.310
	pStream->Read(MAX_PLAYERS, m_aiFreeSpecialist);
	pStream->Read(MAX_PLAYERS, m_aiPower);
	pStream->Read(MAX_PLAYERS, m_aiBestFoundValue);
	pStream->Read(MAX_TEAMS, m_aiNumRevealedTiles);
	pStream->Read(MAX_TEAMS, m_aiCleanPowerCount);
	pStream->Read(MAX_TEAMS, m_aiBorderObstacleCount);

	pStream->Read(MAX_TEAMS, (int*)m_aeAreaAIType);

	for (iI=0;iI<MAX_PLAYERS;iI++)
	{
		pStream->Read((int*)&m_aTargetCities[iI].eOwner);
		pStream->Read(&m_aTargetCities[iI].iID);
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		pStream->Read(NUM_YIELD_TYPES, m_aaiYieldRateModifier[iI]);
	}
	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		pStream->Read(NUM_UNITAI_TYPES, m_aaiNumTrainAIUnits[iI]);
	}
	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		pStream->Read(NUM_UNITAI_TYPES, m_aaiNumAIUnits[iI]);
	}

	pStream->Read(GC.getNumBonusInfos(), m_paiNumBonuses);
	pStream->Read(GC.getNumImprovementInfos(), m_paiNumImprovements);
}


void CvArea::write(FDataStreamBase* pStream)
{
	int iI;

	uint uiFlag=0;
	uiFlag = 1; // advc.030
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iID);
	pStream->Write(m_iNumTiles);
	pStream->Write(m_iNumOwnedTiles);
	pStream->Write(m_iNumRiverEdges);
	pStream->Write(m_iNumUnits);
	pStream->Write(m_iNumCities);
	pStream->Write(m_iTotalPopulation);
	pStream->Write(m_iNumStartingPlots);
	pStream->Write(m_iBarbarianCitiesEver); // advc.300

	pStream->Write(m_bWater);
	// <advc.030>
	pStream->Write(m_bLake);
	pStream->Write(m_iRepresentativeAreaId);
	// </advc.030>
	pStream->Write(MAX_PLAYERS, m_aiUnitsPerPlayer);
	pStream->Write(MAX_PLAYERS, m_aiAnimalsPerPlayer);
	pStream->Write(MAX_PLAYERS, m_aiCitiesPerPlayer);
	pStream->Write(MAX_PLAYERS, m_aiPopulationPerPlayer);
	pStream->Write(MAX_PLAYERS, m_aiBuildingGoodHealth);
	pStream->Write(MAX_PLAYERS, m_aiBuildingBadHealth);
	pStream->Write(MAX_PLAYERS, m_aiBuildingHappiness);
	pStream->Write(MAX_PLAYERS, m_aiTradeRoutes); // advc.310
	pStream->Write(MAX_PLAYERS, m_aiFreeSpecialist);
	pStream->Write(MAX_PLAYERS, m_aiPower);
	pStream->Write(MAX_PLAYERS, m_aiBestFoundValue);
	pStream->Write(MAX_TEAMS, m_aiNumRevealedTiles);
	pStream->Write(MAX_TEAMS, m_aiCleanPowerCount);
	pStream->Write(MAX_TEAMS, m_aiBorderObstacleCount);

	pStream->Write(MAX_TEAMS, (int*)m_aeAreaAIType);

	for (iI=0;iI<MAX_PLAYERS;iI++)
	{
		pStream->Write(m_aTargetCities[iI].eOwner);
		pStream->Write(m_aTargetCities[iI].iID);
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		pStream->Write(NUM_YIELD_TYPES, m_aaiYieldRateModifier[iI]);
	}
	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		pStream->Write(NUM_UNITAI_TYPES, m_aaiNumTrainAIUnits[iI]);
	}
	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		pStream->Write(NUM_UNITAI_TYPES, m_aaiNumAIUnits[iI]);
	}
	pStream->Write(GC.getNumBonusInfos(), m_paiNumBonuses);
	pStream->Write(GC.getNumImprovementInfos(), m_paiNumImprovements);
}

// Protected Functions...
