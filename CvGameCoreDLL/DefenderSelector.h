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
	/*  The caller provides a vector of legal defenders. The selection is returned
		through the same parameter. */
	void selectAvailableDefenders(std::vector<CvUnit*>& kAvailable,
		PlayerTypes eAttackerOwner, CvUnit const* pAttacker = NULL) const;
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
	bool canFight(CvUnit const& kDefender, PlayerTypes eAttackerOwner, CvUnit const* pAttacker = NULL) const;
	int biasValue(CvUnit const& kUnit) const;
	int randomValue(CvUnit const& kUnit, PlayerTypes eAttackerOwner) const;
};

#endif
