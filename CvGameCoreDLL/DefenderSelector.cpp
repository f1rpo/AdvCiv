#include "CvGameCoreDLL.h"
#include "DefenderSelector.h"
#include "CvUnit.h"
#include "CvPlot.h"
#include "CvGamePlay.h"
#include "CvGlobals.h"

DefenderSelector::DefenderSelector(CvPlot const& kPlot) : m_kPlot(kPlot) {}

void DefenderSelector::uninit()
{
	m_cache.clear();
}

void DefenderSelector::update()
{
	m_cache.setValid(false); // Recompute only on demand
}

void DefenderSelector::selectAvailableDefenders(std::vector<CvUnit*>& kAvailable,
	PlayerTypes eAttackerOwner, CvUnit const* pAttacker) const
{
	/*  Need a (potential) attacking player. NO_PLAYER calls do happen, but
		only on plots with units owned by the active player. */
	if (eAttackerOwner == NO_PLAYER)
		return;
	PROFILE_FUNC(); // Tbd.: Profile it
	validateCache(eAttackerOwner);
	/*  For a "fast" legality check. (What would be faster: Call CvPlot::canDefendHere
		from this function instead of letting the caller do it beforehand.) */
	std::set<IDInfo> combatants;
	/*  Units that can't fight the attacker are always available and don't count
		toward maxAvailableDefenders (except spies) */
	std::vector<CvUnit*> nonCombatants;
	for (size_t i = 0; i < kAvailable.size(); i++)
	{
		if (!canFight(*kAvailable[i], eAttackerOwner, pAttacker) &&
			!kAvailable[i]->alwaysInvisible())
		{
			nonCombatants.push_back(kAvailable[i]);
		}
		else combatants.insert(kAvailable[i]->getIDInfo());
	}
	kAvailable.clear();
	for (int i = 0; i < m_cache.size() &&
		kAvailable.size() < (uint)maxAvailableDefenders(); i++)
	{
		IDInfo unitID(m_cache.at(i));
		if (combatants.count(unitID) > 0)
			kAvailable.push_back(::getUnit(unitID));
	}
	kAvailable.insert(kAvailable.end(), nonCombatants.begin(), nonCombatants.end());
}

int DefenderSelector::maxAvailableDefenders()
{
	// Tbd.: If the upper bound remains constant, move it to XML/ Globals.
	return 4; // GC.getMAX_AVAILABLE_DEFENDERS()
}

void DefenderSelector::validateCache(PlayerTypes eAttackerOwner) const
{
	if (!m_cache.isValid())
		cacheDefenders(eAttackerOwner);
}

void DefenderSelector::cacheDefenders(PlayerTypes eAttackerOwner) const
{
	PROFILE_FUNC(); /*  Tbd.: Profile it; time spent here should decrease a lot
						once the setValid call at the end is uncommented. */
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
	/*  The update calls aren't implemented yet, so, for now, keep the cache
		permanently invalid.
		Tbd. Call DefenderSelector::update:
		- from CvUnit::kill (update the plot where the unit was killed) and
		  on non-lethal damage (but not while combat is still being resolved)
		- from CvUnit::setXY (on the source and destination plot)
		- at the start of each CvPlayer's turn (all plots)
		- from CvTeam::declareWar and makePeace (only plots with military units
		  owned by the war target)
		 Updates after combat should also write a message to the combat log.
		 And when the active player destroys a defender in combat, the on-screen
		 message should say which unit has become available. */
	//m_cache.setValid(true);
}

bool DefenderSelector::canFight(CvUnit const& kDefender, PlayerTypes eAttackerOwner,
		CvUnit const* pAttacker) const
{
	if (!kDefender.canFight() || !kDefender.isEnemy(TEAMID(eAttackerOwner)))
		return false;
	/*  Assume a land attacker unless a sea attacker is given. In particular,
		assume a land attacker when an air attacker is given. */
	bool bSeaAttacker = (pAttacker != NULL && pAttacker->getDomainType() == DOMAIN_SEA);
	if (!bSeaAttacker && kDefender.getDomainType() != DOMAIN_LAND)
		return false;
	return true;
}

int DefenderSelector::biasValue(CvUnit const& kUnit) const
{
	return kUnit.currHitPoints();
		// Bias for units with higher AI power value?
		//+ 2 * kUnit.getUnitInfo().getPower()
}

int DefenderSelector::randomValue(CvUnit const& kUnit, PlayerTypes eAttackerOwner) const
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

DefenderSelector::Cache::Cache() : m_bValid(false) {}

void DefenderSelector::Cache::setValid(bool b)
{
	m_bValid = b;
}

void DefenderSelector::Cache::clear(int iReserve)
{
	m_entries.clear();
	setValid(false);
	if (iReserve > 0) // Just for performance
		m_entries.reserve(iReserve);
}

bool DefenderSelector::Cache::isValid() const
{
	return m_bValid;
}

IDInfo DefenderSelector::Cache::at(int iPosition) const
{
	return m_entries.at(iPosition).second;
}

int DefenderSelector::Cache::size() const
{
	return (int)m_entries.size();
}

void DefenderSelector::Cache::add(IDInfo id, int iValue)
{
	m_entries.push_back(CacheEntry(iValue, id));
}

void DefenderSelector::Cache::sort()
{
	// No point in stable_sort as the cache is fully recomputed before sorting
	std::sort(m_entries.rbegin(), m_entries.rend());
}
