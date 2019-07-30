#include "CvGameCoreDLL.h"
#include "DefenderSelector.h"
#include "CvUnit.h"
#include "CvPlot.h"
#include "CvGamePlay.h"
#include "CvGlobals.h"

DefenderSelector::DefenderSelector(CvPlot const& kPlot) : m_kPlot(kPlot) {}

DefenderSelector::~DefenderSelector() {}

void DefenderSelector::uninit() {}

void DefenderSelector::update() {}

void DefenderSelector::getDefenders(std::vector<CvUnit*>& r, Settings const& kSettings) const {}

BestDefenderSelector::BestDefenderSelector(CvPlot const& kPlot) : DefenderSelector(kPlot) {}

BestDefenderSelector::~BestDefenderSelector() {}

// Cut from CvPlot::getBestDefender; funtionally equivalent.
CvUnit* BestDefenderSelector::getDefender(Settings const& kSettings) const
{
	CvUnit* r = NULL;
	// BETTER_BTS_AI_MOD, Lead From Behind (UncutDragon), 02/21/10, jdog5000
	for (CLLNode<IDInfo>* pUnitNode = m_kPlot.headUnitNode(); pUnitNode != NULL; // advc.003: while loop replaced
			pUnitNode = m_kPlot.nextUnitNode(pUnitNode))
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		if (kSettings.eDefOwner == NO_PLAYER || pLoopUnit->getOwner() == kSettings.eDefOwner)
		{	// advc.003: Moved the other conditions into an auxiliary function
			if(pLoopUnit->canDefendAtCurrentPlot(kSettings.eAttOwner, kSettings.pAttUnit,
					kSettings.bTestAtWar, kSettings.bTestPotentialEnemy,
					false, kSettings.bTestVisible)) // advc.028
			{
				if (pLoopUnit->isBetterDefenderThan(r, kSettings.pAttUnit,
						NULL, //&iBestUnitRank // UncutDragon (advc: NULL should work as well)
						kSettings.bTestVisible)) // advc.061
					r = pLoopUnit;
			}
		}
	}
	// BETTER_BTS_AI_MOD: END
	return r;
}

RandomizedSelector::RandomizedSelector(CvPlot const& kPlot) : BestDefenderSelector(kPlot) {}

RandomizedSelector::~RandomizedSelector() {}

void RandomizedSelector::uninit()
{
	m_cache.clear();
}

void RandomizedSelector::update()
{
	m_cache.setValid(false); // Recompute only on demand
}

CvUnit* RandomizedSelector::getDefender(Settings const& kSettings) const
{
	CvUnit* r = NULL;
	std::vector<CvUnit*> defenders;
	getDefenders(defenders, kSettings);
	if (defenders.empty())
	{	// If everyone is available, use the BtS algorithm.
		return BestDefenderSelector::getDefender(kSettings);
	}
	for (size_t i = 0; i < defenders.size(); i++)
	{
		if (defenders[i]->isBetterDefenderThan(r, kSettings.pAttUnit, NULL, kSettings.bTestVisible))
			r = defenders[i];
	}
	return r;
}

void RandomizedSelector::getDefenders(std::vector<CvUnit*>& r, Settings const& kSettings) const
{
	FAssert(r.empty());
	if (kSettings.eAttOwner == NO_PLAYER)
	{	// Only valid in async code for plots that the active player can't attack
		return;
	}
	//PROFILE_FUNC(); // Ca. 1.5 permille of the running time
	validateCache(kSettings.eAttOwner);
	for (int i = 0; i < m_cache.size() &&
		r.size() < (uint)maxAvailableDefenders(); i++)
	{
		CvUnit* pUnit = ::getUnit(m_cache.at(i));
		if (pUnit == NULL)
		{	// But it could perhaps also be that the unit is currently dying; that would be OK.
			FAssertMsg(pUnit != NULL, "Cache out of date?");
			continue;
		}
		if (kSettings.eDefOwner != NO_PLAYER && pUnit->getOwner() != kSettings.eDefOwner)
			continue;
		if (pUnit->canDefendAtCurrentPlot(kSettings.eAttOwner, kSettings.pAttUnit,
			kSettings.bTestAtWar, kSettings.bTestPotentialEnemy, false, kSettings.bTestVisible))
		{
			/*  Noncombatants are unavailable unless no proper defender is available:
				empty r means all are available. */
			if (canCombat(*pUnit, kSettings))
				r.push_back(pUnit);
		}
	}
}

int RandomizedSelector::maxAvailableDefenders()
{
	/*  Tbd.: If the upper bound remains constant, move it to XML/ Globals
		and get rid of this function (or at least make it non-public). */
	return 4; // GC.getMAX_AVAILABLE_DEFENDERS()
}

bool RandomizedSelector::canCombat(CvUnit const& kDefUnit, Settings const& kSettings) const
{
	if (!kDefUnit.canFight())
		return false;
	if (!kDefUnit.isEnemy(TEAMID(kSettings.eAttOwner)))
		return false;
	/*  Assume a land attacker unless a sea attacker is given. In particular,
		assume a land attacker when an air attacker is given. */
	bool bSeaAttacker = (kSettings.pAttUnit != NULL && kSettings.pAttUnit->getDomainType() == DOMAIN_SEA);
	if (!bSeaAttacker && !m_kPlot.isWater() && (kDefUnit.getDomainType() != DOMAIN_LAND ||
			kDefUnit.isCargo()))
		return false;
	return true;
}

void RandomizedSelector::validateCache(PlayerTypes eAttackerOwner) const
{
	if (!m_cache.isValid())
		cacheDefenders(eAttackerOwner);
}

void RandomizedSelector::cacheDefenders(PlayerTypes eAttackerOwner) const
{
	PROFILE_FUNC(); // Less than 1 permille of the running time
	m_cache.clear(m_kPlot.getNumUnits());
	for (CLLNode<IDInfo>* pNode = m_kPlot.headUnitNode(); pNode != NULL;
		pNode = m_kPlot.nextUnitNode(pNode))
	{
		CvUnit const* pUnit = ::getUnit(pNode->m_data);
		if (pUnit == NULL)
		{
			// Might be OK; if defenders get cached while units are dying ...
			FAssert(pUnit != NULL);
			continue;
		}
		int iAvailability = biasValue(*pUnit) * (50 + randomValue(*pUnit, eAttackerOwner));
		m_cache.add(pUnit->getIDInfo(), iAvailability);
	}
	m_cache.sort();
	/*  Tbd. Call DefenderSelector::update as described below.
		However, the time saved is going to be negligible, so this may not be
		worth polluting the code base, and isn't a priority in any case.
		- from CvUnit::kill (update the plot where the unit was killed) and
		  on non-lethal damage (but not while combat is still being resolved)
		- from CvUnit::setXY (on the source and destination plot)
		- at the start of each CvPlayer's turn (all plots)
		- from CvTeam::declareWar and makePeace (only plots with military units
		  owned by the war target)
		 Updates after combat should also write a message to the combat log.
		 And when the active player destroys a defender in combat, the on-screen
		 message should say which unit has become available. */
	// For now, keep the cache permanently invalid.
	//m_cache.setValid(true);
}

int RandomizedSelector::biasValue(CvUnit const& kUnit) const
{
	return kUnit.currHitPoints();
		// Bias for units with higher AI power value?
		//+ 2 * kUnit.getUnitInfo().getPower()
}

int RandomizedSelector::randomValue(CvUnit const& kUnit, PlayerTypes eAttackerOwner) const
{
	std::vector<long> hashInputs;
	hashInputs.push_back(GC.getGame().getGameTurn());
	hashInputs.push_back(eAttackerOwner);
	hashInputs.push_back(kUnit.getOwner());
	hashInputs.push_back(kUnit.getID());
	hashInputs.push_back(m_kPlot.getX());
	hashInputs.push_back(m_kPlot.getY());
	// Let hash use eAttackerOwner's starting coordinates as a sort of game id
	return ::round(100 * ::hash(hashInputs, eAttackerOwner));
}

RandomizedSelector::Cache::Cache() : m_bValid(false) {}

void RandomizedSelector::Cache::setValid(bool b)
{
	m_bValid = b;
}

void RandomizedSelector::Cache::clear(int iReserve)
{
	m_entries.clear();
	setValid(false);
	if (iReserve > 0) // Just for performance
		m_entries.reserve(iReserve);
}

bool RandomizedSelector::Cache::isValid() const
{
	return m_bValid;
}

IDInfo RandomizedSelector::Cache::at(int iPosition) const
{
	return m_entries.at(iPosition).second;
}

int RandomizedSelector::Cache::size() const
{
	return (int)m_entries.size();
}

void RandomizedSelector::Cache::add(IDInfo id, int iValue)
{
	m_entries.push_back(CacheEntry(iValue, id));
}

void RandomizedSelector::Cache::sort()
{
	// No point in stable_sort as the cache is fully recomputed before sorting
	std::sort(m_entries.rbegin(), m_entries.rend());
}
