#pragma once

#ifndef DEFENDER_FILTER_H
#define DEFENDER_FILTER_H

#include "CvGameCoreDLL.h"

class CvUnit;
class CvPlot;

/*  advco.defr: Main class for the defender randomization module. Part of the
	Advanced Combat (working title) mod. */
class DefenderSelector
{
public:
	DefenderSelector(CvPlot const& kPlot);
	void uninit();
	void update();
	struct Settings // Constraints on the defender selection
	{	// Same defaults as in BtS (not very helpful ...)
		Settings(PlayerTypes eDefOwner = NO_PLAYER, PlayerTypes eAttOwner = NO_PLAYER,
			CvUnit const* pAttUnit = NULL, bool bTestAtWar = false,
			bool bTestPotentialEnemy = false, bool bTestVisible = false) :
			eDefOwner(eDefOwner), eAttOwner(eAttOwner), pAttUnit(pAttUnit),
			bTestAtWar(bTestAtWar), bTestPotentialEnemy(bTestPotentialEnemy),
			bTestVisible(bTestVisible) {}
		PlayerTypes eDefOwner, eAttOwner;
		CvUnit const* pAttUnit;
		bool bTestAtWar, bTestPotentialEnemy, bTestVisible;
	};
	CvUnit* getBestDefender(Settings const& kSettings) const;
	// Returns by parameter r. Empties r if all units are available!
	void getAvailableDefenders(std::vector<CvUnit*>& r, Settings const& kSettings) const;
	static int maxAvailableDefenders();

protected:
	CvPlot const& m_kPlot;

	class Cache
	{
	public:
		Cache();
		void setValid(bool b);
		void clear(int iReserve = -1);
		bool isValid() const;
		IDInfo at(int iPosition) const;
		int size() const;
		void add(IDInfo id, int iValue);
		void sort();
	protected:
		// Can still replace this with a struct later
		typedef std::pair<int,IDInfo> CacheEntry;
		bool m_bValid;
		std::vector<CacheEntry> m_entries;
	};
	mutable Cache m_cache;
	void validateCache(PlayerTypes eAttackerOwner) const;
	// The main algorithm; want this outside of the cache class.
	void cacheDefenders(PlayerTypes eAttackerOwner) const;
	int biasValue(CvUnit const& kUnit) const;
	int randomValue(CvUnit const& kUnit, PlayerTypes eAttackerOwner) const;
	bool canCombat(CvUnit const& kDefUnit, Settings const& kSettings) const;
};

#endif
