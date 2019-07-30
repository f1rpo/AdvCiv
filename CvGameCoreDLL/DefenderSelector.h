#pragma once

#ifndef DEFENDER_FILTER_H
#define DEFENDER_FILTER_H

class CvUnit;
class CvPlot;

/*  advco.defr: Main class hierarchy for the defender randomization module.
	Part of the Advanced Combat (working title) mod. */
// Abstract base class
class DefenderSelector
{
public:
	DefenderSelector(CvPlot const& kPlot);
	virtual ~DefenderSelector();
	virtual void uninit();
	virtual void update(); // Update cached data (if any)
	struct Settings // Constraints on the defender selection
	{	// Same defaults as in the BtS code (not very helpful ...)
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
	virtual CvUnit* getDefender(Settings const& kSettings) const=0;
	// Returns by parameter r. Empty r means all units!
	virtual void getDefenders(std::vector<CvUnit*>& r, Settings const& kSettings) const;

protected:
	CvPlot const& m_kPlot;
};

// Encapsulates the BtS defender selection algorithm: select the best defender
class BestDefenderSelector : public DefenderSelector
{
public:
	BestDefenderSelector(CvPlot const& kPlot);
	~BestDefenderSelector();
	CvUnit* getDefender(Settings const& kSettings) const;
};

// Selects the best defender from a random set
class RandomizedSelector : public BestDefenderSelector
{
public:
	RandomizedSelector(CvPlot const& kPlot);
	~RandomizedSelector();
	void uninit();
	void update();
	CvUnit* getDefender(Settings const& kSettings) const;
	void getDefenders(std::vector<CvUnit*>& r, Settings const& kSettings) const;
	static int maxAvailableDefenders();

protected:
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
