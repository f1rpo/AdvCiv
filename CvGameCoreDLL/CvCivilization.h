#pragma once

#ifndef CV_CIVILIZATION_H
#define CV_CIVILIZATION_H

class CvBuildingInfo;
class CvUnitInfo;

/*  advc.003w: New class. Mainly to encapsulate the mapping between building/ unit classes
	and (civilization-specific) building/ unit types.
	Could develop this (under a different class name) into a cache for items
	that cities of a player can produce at present (i.e. taking into account
	tech requirements) or ever (taking into account CvGame::canConstruct).
	The gains in performance aren't going to be great though. */
class CvCivilization : private boost::noncopyable
{
public:
	explicit CvCivilization(CvCivilizationInfo const& kInfo);
	inline int getNumBuildings() const
	{
		return m_buildings.size();
	}
	inline BuildingTypes buildingAt(int iBuildingIndex) const
	{
		return m_buildings[iBuildingIndex];
	}
	inline BuildingClassTypes buildingClassAt(int iBuildingIndex) const
	{	/*  (I've tested a vector<BuildingClassTypes> of the same size as m_buildings
			to make this more efficient -- didn't help at all.) */
		return buildingClass(buildingAt(iBuildingIndex));
	}
	inline int getNumUnits() const
	{
		return m_units.size();
	}
	inline UnitTypes unitAt(int iUnitIndex) const
	{
		return m_units[iUnitIndex];
	}
	inline UnitClassTypes unitClassAt(int iUnitIndex) const
	{
		return unitClass(unitAt(iUnitIndex));
	}
	BuildingTypes getBuilding(BuildingClassTypes eBuildingClass) const;
	UnitTypes getUnit(UnitClassTypes eUnitClass) const;

	inline int getNumUniqueBuildings() const
	{
		return m_uniqueBuildings.size();
	}
	inline BuildingTypes uniqueBuildingAt(int iUniqueBuildingIndex) const
	{
		return m_uniqueBuildings[iUniqueBuildingIndex];
	}
	inline int getNumUniqueUnits() const
	{
		return m_uniqueUnits.size();
	}
	inline UnitTypes uniqueUnitAt(int iUniqueUnitIndex) const
	{
		return m_uniqueUnits[iUniqueUnitIndex];
	}
	bool isUnique(BuildingTypes eBuilding) const;
	inline bool isUnique(BuildingClassTypes eBuildingClass) const
	{
		return isUnique(getBuilding(eBuildingClass));
	}
	bool isUnique(UnitTypes eUnit) const;
	inline bool isUnique(UnitClassTypes eUnitClass) const
	{
		return isUnique(getUnit(eUnitClass));
	}

	inline bool isFreeBuilding(BuildingTypes eBuilding) const
	{
		return isFreeBuilding(buildingClass(eBuilding));
	}
	bool isFreeBuilding(BuildingClassTypes eBuildingClass) const;
	inline int getNumFreeUnits(UnitTypes eUnit) const
	{
		return getNumFreeUnits(unitClass(eUnit));
	}
	int getNumFreeUnits(UnitClassTypes eUnitClass) const;

	CivicTypes getInitialCivic(CivicOptionTypes eCivicOption) const;

	static BuildingClassTypes buildingClass(BuildingTypes eBuilding);
	static UnitClassTypes unitClass(UnitTypes eUnit);

private:
	std::vector<BuildingTypes> m_buildings;
	std::vector<UnitTypes> m_units;
	std::vector<BuildingTypes> m_uniqueBuildings;
	std::vector<UnitTypes> m_uniqueUnits;
	CvCivilizationInfo const& m_kInfo;
};

#endif
