// area.cpp

#include "CvGameCoreDLL.h"
#include "CvArea.h"
#include "CvMap.h"
#include "CvGamePlay.h"
#include "CvCity.h"
#include "CvUnit.h"
#include "CvPlayerAI.h"
#include "CvInfo_Terrain.h"


CvArea::CvArea()
{
	m_aTargetCities = new IDInfo[MAX_PLAYERS];
	// advc: Default id was 0; invalid seems safer.
	reset(FFreeList::INVALID_INDEX, false, true);
}


CvArea::~CvArea()
{
	uninit();
	SAFE_DELETE_ARRAY(m_aTargetCities);
}


void CvArea::init(bool bWater) // advc: iID param removed; always gets set beforehand.
{
	reset(getID(), bWater);
}


void CvArea::uninit() { /* (advc.enum: No memory to free here currently) */ }

// Initializes data members that are serialized.
void CvArea::reset(int iID, bool bWater, bool bConstructorCall)
{
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

	m_aiUnitsPerPlayer.reset();
	m_aiCitiesPerPlayer.reset();
	m_aiPopulationPerPlayer.reset();
	m_aiBuildingGoodHealth.reset();
	m_aiBuildingBadHealth.reset();
	m_aiBuildingHappiness.reset();
	m_aiTradeRoutes.reset(); // advc.310
	m_aiFreeSpecialist.reset();
	m_aiPower.reset();
	m_aiBestFoundValue.reset();
	m_aiNumRevealedTiles.reset();
	m_aiCleanPowerCount.reset();
	m_aiBorderObstacleCount.reset();
	m_aiBonuses.reset();
	//m_aiImprovements.reset(); // advc.opt
	m_aeAreaAIType.reset();

	for (int i = 0; i < MAX_PLAYERS; i++)
		m_aTargetCities[i].reset();

	m_aaiYieldRateModifier.reset();
	m_aaiNumTrainAIUnits.reset();
	m_aaiNumAIUnits.reset();
}


void CvArea::setID(int iID)
{
	m_iID = iID;
	m_iRepresentativeAreaId = iID; // advc.030
}


int CvArea::calculateTotalBestNatureYield() const
{
	int iCount = 0;
	CvMap const& kMap = GC.getMap();
	for (int i = 0; i < kMap.numPlots(); i++)
	{
		CvPlot const& kPlot = kMap.getPlotByIndex(i);
		if (kPlot.isArea(*this))
			iCount += kPlot.calculateTotalBestNatureYield(NO_TEAM);
	}
	return iCount;
}


int CvArea::countCoastalLand() const
{
	if (isWater())
		return 0;

	int iCount = 0;
	CvMap const& kMap = GC.getMap();
	for (int iI = 0; iI < kMap.numPlots(); iI++)
	{
		CvPlot const& kPlot = kMap.getPlotByIndex(iI);
		if (kPlot.isArea(*this))
		{
			if (kPlot.isCoastalLand())
				iCount++;
		}
	}
	return iCount;
}


int CvArea::countNumUniqueBonusTypes() const
{
	int iCount = 0;
	for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
	{
		if (getNumBonuses((BonusTypes)iI) > 0)
		{
			if (GC.getInfo((BonusTypes)iI).isOneArea())
				iCount++;
		}
	}
	return iCount;
}


int CvArea::countHasReligion(ReligionTypes eReligion, PlayerTypes eOwner) const
{
	int iCount = 0;
	// <advc.opt> Don't go through all players if eOwner is given
	if (eOwner == NO_PLAYER)
	{
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive()) // Recursive call with eOwner!=NO_PLAYER
				iCount += countHasReligion(eReligion, (PlayerTypes)iI);
		}
		return iCount;
	} // </advc.opt>
	FOR_EACH_CITY(pLoopCity, GET_PLAYER(eOwner))
	{
		if (pLoopCity->isArea(*this))
		{
			if (pLoopCity->isHasReligion(eReligion))
				iCount++;
		}
	}
	return iCount;
}

int CvArea::countHasCorporation(CorporationTypes eCorporation, PlayerTypes eOwner) const
{
	int iCount = 0;
	// <advc.opt> Don't go through all players if eOwner is given
	if (eOwner == NO_PLAYER)
	{
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive()) // Recursive call with eOwner!=NO_PLAYER
				iCount += countHasCorporation(eCorporation, (PlayerTypes)iI);
		}
		return iCount;
	} // </advc.opt>
	
	FOR_EACH_CITY(pLoopCity, GET_PLAYER(eOwner))
	{
		if (pLoopCity->isArea(*this))
		{
			if (pLoopCity->isHasCorporation(eCorporation))
				iCount++;
		}
	}
	return iCount;
}


void CvArea::updateLake(bool bCheckRepr)
{
	PROFILE_FUNC();
	m_bLake = false;
	if(!isWater())
		return;
	int iTotalTiles = getNumTiles();
	if(iTotalTiles > GC.getDefineINT(CvGlobals::LAKE_MAX_AREA_SIZE))
		return;
	if(!bCheckRepr)
	{
		m_bLake = true;
		return;
	}
	FOR_EACH_AREA(pOther)
	{
		if(pOther->m_iRepresentativeAreaId == m_iRepresentativeAreaId &&
			pOther->getID() != getID())
		{
			iTotalTiles += pOther->getNumTiles();
			if(iTotalTiles > GC.getDefineINT(CvGlobals::LAKE_MAX_AREA_SIZE))
				return;
		}
	}
	m_bLake = true;
}

void CvArea::setRepresentativeArea(int eArea)
{
	m_iRepresentativeAreaId = eArea;
}

int CvArea::getRepresentativeArea() const
{
	return m_iRepresentativeAreaId;
}

/*  Replacement for the BtS area()==area() checks. Mostly used for
	performance reasons before costlier more specific checks. */
bool CvArea::canBeEntered(CvArea const& kFrom, CvUnit const* u) const
{
	//PROFILE_FUNC();
	/*  Called very often. Mostly from the various plot danger functions.
		advc.inl: I've force-inlined all functions called from here.
		Still consumes a significant portion of the total turn time. */
	if(getID() == kFrom.getID())
		return true;
	/*  If I wanted to support canMoveAllTerrain here, then I couldn't do
		anything more when u==NULL. So that's not supported. */
	if(isWater() == kFrom.isWater() &&
		(m_iRepresentativeAreaId != kFrom.m_iRepresentativeAreaId ||
		(u != NULL && !u->canMoveImpassable())))
	{
		return false;
	}
	/*  Can't rule out movement between water and land without knowing if the
		unit is a ship inside a city or a land unit aboard a transport */
	if(u == NULL)
		return true;
	if(isWater() && (u->getDomainType() != DOMAIN_SEA || !u->getPlot().isCity()))
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
std::pair<int,int> CvArea::countOwnedUnownedHabitableTiles(bool bIgnoreBarb) const
{
	std::pair<int,int> r(0, 0);
	CvMap const& kMap = GC.getMap();
	for(int i = 0; i < kMap.numPlots(); i++)
	{
		CvPlot const& kPlot = kMap.getPlotByIndex(i);
		if(!kPlot.isArea(*this) || !kPlot.isHabitable())
			continue;
		if(kPlot.isOwned() && (!bIgnoreBarb || kPlot.getOwner() != BARBARIAN_PLAYER))
			r.first++;
		else r.second++;
	}
	return r;
}


int CvArea::countCivs(bool bSubtractOCC) const
{
	/* Perhaps an owned tile (across the sea) should suffice, but tiles-per-civ
	   aren't cached/ serialized (yet). */
	int r = 0;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		PlayerTypes ePlayer = (PlayerTypes)i;
		if(getCitiesPerPlayer(ePlayer) > 0 &&
				(!bSubtractOCC || !GET_PLAYER(ePlayer).isHuman() ||
				!GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE)))
			r++;
	}
	return r;
}


bool CvArea::hasAnyAreaPlayerBonus(BonusTypes eBonus) const
{
	for(int i = 0; i < MAX_PLAYERS; i++)
	{
		PlayerTypes ePlayer = (PlayerTypes)i;
		// Barbarian, minor civ, anything goes so long as there's a city.
		if(getCitiesPerPlayer(ePlayer) > 0 && GET_PLAYER(ePlayer).hasBonus(eBonus))
			return true;
	}
	return false;
}

int CvArea::getBarbarianCitiesEverCreated() const
{
	return m_iBarbarianCitiesEver;
}

void CvArea::reportBarbarianCityCreated()
{
	m_iBarbarianCitiesEver++;
} // </advc.300>


void CvArea::changeNumRiverEdges(int iChange)
{
	m_iNumRiverEdges = (m_iNumRiverEdges + iChange);
	FAssert(getNumRiverEdges() >= 0);
}


void CvArea::changeNumStartingPlots(int iChange)
{
	m_iNumStartingPlots = m_iNumStartingPlots + iChange;
	FAssert(getNumStartingPlots() >= 0);
}


void CvArea::changeUnitsPerPlayer(PlayerTypes eIndex, int iChange)
{
	m_aiUnitsPerPlayer.add(eIndex, iChange);
	// advc (can be temporarily negative while recalculating areas)
	FAssert(getUnitsPerPlayer(eIndex) >= 0 || gDLL->GetWorldBuilderMode());
	m_iNumUnits += iChange;
	FAssert(getNumUnits() >= 0);
}


int CvArea::getCitiesPerPlayer(PlayerTypes eIndex, /* <advc.030b> */ bool bCheckAdjacentCoast) const
{
	/*  Perhaps this parameter isn't really needed, but this function gets called
		from so many places that I can't check if one of them might have a problem
		with water areas having a positive city count. */
	if(!bCheckAdjacentCoast && isWater())
		return 0; // </advc.030b>
	return m_aiCitiesPerPlayer.get(eIndex);
}


void CvArea::changeCitiesPerPlayer(PlayerTypes eIndex, int iChange)
{
	m_aiCitiesPerPlayer.add(eIndex, iChange);
	FAssert(getCitiesPerPlayer(eIndex, true) >= 0 || gDLL->GetWorldBuilderMode()); // advc
	m_iNumCities += iChange;
	FAssert(getNumCities() >= 0);
	// <advc.300>
	for (int i = 0; i < iChange; i++)
		reportBarbarianCityCreated(); // </advc.300>
}


void CvArea::changePopulationPerPlayer(PlayerTypes eIndex, int iChange)
{
	m_aiPopulationPerPlayer.add(eIndex, iChange);
	FAssert(getPopulationPerPlayer(eIndex) >= 0 || gDLL->GetWorldBuilderMode()); // advc
	m_iTotalPopulation += iChange;
	FAssert(getTotalPopulation() >= 0);
}


void CvArea::changeBuildingGoodHealth(PlayerTypes eIndex, int iChange)
{
	if (iChange != 0)
	{
		m_aiBuildingGoodHealth.add(eIndex, iChange);
		FAssert(getBuildingGoodHealth(eIndex) >= 0);
		GET_PLAYER(eIndex).AI_makeAssignWorkDirty();
	}
}


void CvArea::changeBuildingBadHealth(PlayerTypes eIndex, int iChange)
{
	if (iChange != 0)
	{
		m_aiBuildingBadHealth.add(eIndex, iChange);
		FAssert(getBuildingBadHealth(eIndex) >= 0);
		GET_PLAYER(eIndex).AI_makeAssignWorkDirty();
	}
}


void CvArea::changeBuildingHappiness(PlayerTypes eIndex, int iChange)
{
	if (iChange != 0)
	{
		m_aiBuildingHappiness.add(eIndex, iChange);
		GET_PLAYER(eIndex).AI_makeAssignWorkDirty();
	}
}

// advc.310:
void CvArea::changeTradeRoutes(PlayerTypes eIndex, int iChange)
{
	if(iChange != 0)
	{
		m_aiTradeRoutes.add(eIndex, iChange);
		FAssert(getTradeRoutes(eIndex) >= 0);
		GET_PLAYER(eIndex).updateTradeRoutes();
	}
}


void CvArea::changeFreeSpecialist(PlayerTypes eIndex, int iChange)
{
	if (iChange != 0)
	{
		m_aiFreeSpecialist.add(eIndex, iChange);
		FAssert(getFreeSpecialist(eIndex) >= 0);
		GET_PLAYER(eIndex).AI_makeAssignWorkDirty();
	}
}


void CvArea::changePower(PlayerTypes eIndex, int iChange)
{
	m_aiPower.add(eIndex, iChange);
	/*  <advc.006> Can happen when continuing a game after changing a
		unit power value in XML */
	if(getPower(eIndex) < 0)
	{
		m_aiPower.set(eIndex, 0); // </advc.006>
		FAssertMsg(getPower(eIndex) >= 0, "OK when playing from an old savegame "
				"with an updated version of the mod");
	}
}


void CvArea::setBestFoundValue(PlayerTypes eIndex, int iNewValue)
{
	m_aiBestFoundValue.set(eIndex, iNewValue);
	FAssert(getBestFoundValue(eIndex) >= 0);
}


void CvArea::changeNumRevealedTiles(TeamTypes eIndex, int iChange)
{
	m_aiNumRevealedTiles.add(eIndex, iChange);
	FAssert(getNumRevealedTiles(eIndex) >= 0);
}


void CvArea::changeCleanPowerCount(TeamTypes eIndex, int iChange)
{
	if(iChange == 0)
		return;
	bool bOldCleanPower = isCleanPower(eIndex);
	m_aiCleanPowerCount.add(eIndex, iChange);
	if (bOldCleanPower != isCleanPower(eIndex))
	{
		GET_TEAM(eIndex).updateCommerce();
		GET_TEAM(eIndex).updatePowerHealth();
		if (eIndex == GC.getGame().getActiveTeam())
			gDLL->UI().setDirty(CityInfo_DIRTY_BIT, true);
	}
}


void CvArea::changeBorderObstacleCount(TeamTypes eIndex, int iChange)
{
	m_aiBorderObstacleCount.add(eIndex, iChange);
	if (iChange > 0 && m_aiBorderObstacleCount.get(eIndex) == iChange)
		GC.getMap().verifyUnitValidPlot();
}


void CvArea::setAreaAIType(TeamTypes eIndex, AreaAITypes eNewValue)
{
	m_aeAreaAIType.set(eIndex, eNewValue);
}


CvCityAI* CvArea::AI_getTargetCity(PlayerTypes eIndex) const
{
	FAssertBounds(0, MAX_PLAYERS, eIndex);
	return ::AI_getCity(m_aTargetCities[eIndex]);
}


void CvArea::AI_setTargetCity(PlayerTypes eIndex, CvCity* pNewValue)
{
	FAssertBounds(0, MAX_PLAYERS, eIndex);
	if (pNewValue != NULL)
		m_aTargetCities[eIndex] = pNewValue->getIDInfo();
	else m_aTargetCities[eIndex].reset();
}


void CvArea::changeYieldRateModifier(PlayerTypes eIndex1, YieldTypes eIndex2, int iChange)
{
	if (iChange == 0)
		return;

	m_aaiYieldRateModifier.add(eIndex1, eIndex2, toShort(iChange));

	GET_PLAYER(eIndex1).invalidateYieldRankCache(eIndex2);
	if (eIndex2 == YIELD_COMMERCE)
		GET_PLAYER(eIndex1).updateCommerce();
	GET_PLAYER(eIndex1).AI_makeAssignWorkDirty();
	if (GET_PLAYER(eIndex1).getTeam() == GC.getGame().getActiveTeam())
		gDLL->UI().setDirty(CityInfo_DIRTY_BIT, true);
}


void CvArea::changeNumTrainAIUnits(PlayerTypes eIndex1, UnitAITypes eIndex2, int iChange)
{
	m_aaiNumTrainAIUnits.add(eIndex1, eIndex2, iChange);
	FAssert(getNumTrainAIUnits(eIndex1, eIndex2) >= 0);
}


int CvArea::getNumAIUnits(PlayerTypes eIndex1, UnitAITypes eIndex2) const
{
	// <advc.124> NO_UNITAI counts all units of eIndex1
	//FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be >= 0");
	if(eIndex2 < 0)
	{
		int r = 0;
		FOR_EACH_ENUM(UnitAI)
			r += m_aaiNumAIUnits.get(eIndex1, eLoopUnitAI);
		return r;
	} // </advc.124>
	return m_aaiNumAIUnits.get(eIndex1, eIndex2);
}


void CvArea::changeNumAIUnits(PlayerTypes eIndex1, UnitAITypes eIndex2, int iChange)
{
	m_aaiNumAIUnits.add(eIndex1, eIndex2, iChange);
	FAssert(getNumAIUnits(eIndex1, eIndex2) >= 0);
}


void CvArea::changeNumBonuses(BonusTypes eBonus, int iChange)
{
	m_aiBonuses.add(eBonus, iChange);
	FAssert(getNumBonuses(eBonus) >= 0);
}

// advc.opt: No longer used
/*int CvArea::getNumImprovements(ImprovementTypes eImprovement) const
{
	return m_aiImprovements.get(eImprovement);
}

void CvArea::changeNumImprovements(ImprovementTypes eImprovement, int iChange)
{
	m_aiImprovements.add(eImprovement, iChange);
	FAssert(getNumImprovements(eImprovement) >= 0);
}*/


void CvArea::read(FDataStreamBase* pStream)
{
	reset();

	uint uiFlag=0;
	pStream->Read(&uiFlag);

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
	if(uiFlag >= 1)
	{
		pStream->Read(&m_bLake);
		pStream->Read(&m_iRepresentativeAreaId);
	}
	else
	{
		updateLake(false);
		m_iRepresentativeAreaId = m_iID;
	} // </advc.030>
	m_aiUnitsPerPlayer.Read(pStream);
	// <advc>
	if (uiFlag < 2)
	{
		// Discard AnimalsPerPlayer
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			int iTmp;
			pStream->Read(&iTmp);
		}
	} // </advc>
	m_aiCitiesPerPlayer.Read(pStream);
	m_aiPopulationPerPlayer.Read(pStream);
	m_aiBuildingGoodHealth.Read(pStream);
	m_aiBuildingBadHealth.Read(pStream);
	m_aiBuildingHappiness.Read(pStream);
	m_aiTradeRoutes.Read(pStream); // advc.310
	m_aiFreeSpecialist.Read(pStream);
	m_aiPower.Read(pStream);
	m_aiBestFoundValue.Read(pStream);
	m_aiNumRevealedTiles.Read(pStream);
	m_aiCleanPowerCount.Read(pStream);
	m_aiBorderObstacleCount.Read(pStream);
	m_aeAreaAIType.Read(pStream);

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		pStream->Read((int*)&m_aTargetCities[i].eOwner);
		pStream->Read(&m_aTargetCities[i].iID);
		m_aTargetCities[i].validateOwner(); // advc.opt
	}

	m_aaiYieldRateModifier.Read(pStream, uiFlag < 2);
	m_aaiNumTrainAIUnits.Read(pStream);
	m_aaiNumAIUnits.Read(pStream);
	m_aiBonuses.Read(pStream);
	//m_aiImprovements.Read(pStream);
	// <advc.opt>
	if (uiFlag < 3)
	{
		EnumMap<ImprovementTypes,int> dummy;
		dummy.Read(pStream);
	} // </advc.opt>
}


void CvArea::write(FDataStreamBase* pStream)
{
	PROFILE_FUNC(); // advc
	REPRO_TEST_BEGIN_WRITE(CvString::format("Area(%d)", getID()));
	uint uiFlag;
	//uiFlag = 1; // advc.030
	//uiFlag = 2; // advc: Remove m_aiAnimalsPerPlayer, advc.enum: write m_aaiYieldRateModifier as short
	uiFlag = 3; // advc.opt: Remove m_aiImprovements
	pStream->Write(uiFlag);

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
	m_aiUnitsPerPlayer.Write(pStream);
	//m_aiAnimalsPerPlayer.Write(pStream); // advc: removed
	m_aiCitiesPerPlayer.Write(pStream);
	m_aiPopulationPerPlayer.Write(pStream);
	m_aiBuildingGoodHealth.Write(pStream);
	m_aiBuildingBadHealth.Write(pStream);
	m_aiBuildingHappiness.Write(pStream);
	m_aiTradeRoutes.Write(pStream); // advc.310
	m_aiFreeSpecialist.Write(pStream);
	m_aiPower.Write(pStream);
	m_aiBestFoundValue.Write(pStream);
	m_aiNumRevealedTiles.Write(pStream);
	m_aiCleanPowerCount.Write(pStream);
	m_aiBorderObstacleCount.Write(pStream);
	m_aeAreaAIType.Write(pStream);

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		pStream->Write(m_aTargetCities[i].eOwner);
		pStream->Write(m_aTargetCities[i].iID);
	}

	m_aaiYieldRateModifier.Write(pStream, false);
	m_aaiNumTrainAIUnits.Write(pStream);
	m_aaiNumAIUnits.Write(pStream);
	m_aiBonuses.Write(pStream);
	//m_aiImprovements.Write(pStream); // advc.opt
	REPRO_TEST_END_WRITE();
}
