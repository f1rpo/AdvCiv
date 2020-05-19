#pragma once

#ifndef CV_INFO_CIVICS_H
#define CV_INFO_CIVICS_H

/*  advc.003x: Cut from CvInfos.h. Just the civics classes:
	CvCivicInfo, CvCivicOptionInfo, CvUpkeepInfo.
	Would fit well together with CvTechInfo, but civics aren't included as often
	and are fairly likely to change; don't want to precompile them.
	Including civics in CvInfo_GameOption.h would be practical but counterintuitive. */

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvCivicInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvCivicInfo : public CvInfoBase
{
public: // The const functions are exposed to Python except those (to be) added by AdvCiv

	CvCivicInfo();
	~CvCivicInfo();

	int getCivicOptionType() const;
	int getAnarchyLength() const;
	int getUpkeep() const;
	int getAIWeight() const;
	int getGreatPeopleRateModifier() const;
	int getGreatGeneralRateModifier() const;
	int getDomesticGreatGeneralRateModifier() const;
	int getStateReligionGreatPeopleRateModifier() const;
	int getDistanceMaintenanceModifier() const;
	int getNumCitiesMaintenanceModifier() const;
	int getCorporationMaintenanceModifier() const;
	int getExtraHealth() const;
	int getExtraHappiness() const { return m_iExtraHappiness; } // K-Mod, Exposed to Python
	int getFreeExperience() const;
	int getWorkerSpeedModifier() const;
	int getImprovementUpgradeRateModifier() const;
	int getMilitaryProductionModifier() const;
	int getBaseFreeUnits() const;
	int getBaseFreeMilitaryUnits() const;
	int getFreeUnitsPopulationPercent() const;
	int getFreeMilitaryUnitsPopulationPercent() const;
	int getGoldPerUnit() const;
	int getGoldPerMilitaryUnit() const;
	int getHappyPerMilitaryUnit() const;
	int getLuxuryModifier() const; // advc.912c
	int getLargestCityHappiness() const;
	int getWarWearinessModifier() const;
	int getFreeSpecialist() const;
	int getTradeRoutes() const;
	TechTypes getTechPrereq() const { return m_eTechPrereq; } // advc.inl (return type was int)
	int getCivicPercentAnger() const;
	int getMaxConscript() const;
	int getStateReligionHappiness() const;
	int getNonStateReligionHappiness() const;
	int getStateReligionUnitProductionModifier() const;
	int getStateReligionBuildingProductionModifier() const;
	int getStateReligionFreeExperience() const;
	int getExpInBorderModifier() const;

	bool isMilitaryFoodProduction() const;
	//bool isNoUnhealthyPopulation() const;
	int getUnhealthyPopulationModifier() const;	// K-Mod, Exposed to Python
	bool isBuildingOnlyHealthy() const;
	bool isNoForeignTrade() const;
	bool isNoCorporations() const;
	bool isNoForeignCorporations() const;
	bool isStateReligion() const;
	bool isNoNonStateReligionSpread() const;

	std::wstring pyGetWeLoveTheKing() { return getWeLoveTheKing(); }
	const wchar* getWeLoveTheKing();
	void setWeLoveTheKingKey(const TCHAR* szVal);

	// Array access:

	int getYieldModifier(int i) const;
	int* getYieldModifierArray() const;
	int getCapitalYieldModifier(int i) const;
	int* getCapitalYieldModifierArray() const;
	int getTradeYieldModifier(int i) const;
	int* getTradeYieldModifierArray() const;
	int getCommerceModifier(int i) const;
	int* getCommerceModifierArray() const;
	int getCapitalCommerceModifier(int i) const;
	int* getCapitalCommerceModifierArray() const;
	int getSpecialistExtraCommerce(int i) const;
	int* getSpecialistExtraCommerceArray() const;
	int getBuildingHappinessChanges(int i) const;
	inline bool isAnyBuildingHappinessChanges() const { return (m_paiBuildingHappinessChanges != NULL); } // advc.003t
	int getBuildingHealthChanges(int i) const;
	inline bool isAnyBuildingHealthChanges() const { return (m_paiBuildingHealthChanges != NULL); } // advc.003t
	int getFeatureHappinessChanges(int i) const;

	bool isHurry(int i) const;
	bool isSpecialBuildingNotRequired(int i) const;
	bool isSpecialistValid(int i) const;

	int getImprovementYieldChanges(int i, int j) const;
	#if SERIALIZE_CVINFOS
	void read(FDataStreamBase* stream);
	void write(FDataStreamBase* stream);
	#endif
	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iCivicOptionType;
	int m_iAnarchyLength;
	int m_iUpkeep;
	int m_iAIWeight;
	int m_iGreatPeopleRateModifier;
	int m_iGreatGeneralRateModifier;
	int m_iDomesticGreatGeneralRateModifier;
	int m_iStateReligionGreatPeopleRateModifier;
	int m_iDistanceMaintenanceModifier;
	int m_iNumCitiesMaintenanceModifier;
	int m_iCorporationMaintenanceModifier;
	int m_iExtraHealth;
	int m_iExtraHappiness; // K-Mod
	int m_iFreeExperience;
	int m_iWorkerSpeedModifier;
	int m_iImprovementUpgradeRateModifier;
	int m_iMilitaryProductionModifier;
	int m_iBaseFreeUnits;
	int m_iBaseFreeMilitaryUnits;
	int m_iFreeUnitsPopulationPercent;
	int m_iFreeMilitaryUnitsPopulationPercent;
	int m_iGoldPerUnit;
	int m_iGoldPerMilitaryUnit;
	int m_iHappyPerMilitaryUnit;
	int m_iLuxuryModifier; // advc.912c
	int m_iLargestCityHappiness;
	int m_iWarWearinessModifier;
	int m_iFreeSpecialist;
	int m_iTradeRoutes;
	TechTypes m_eTechPrereq;
	int m_iCivicPercentAnger;
	int m_iMaxConscript;
	int m_iStateReligionHappiness;
	int m_iNonStateReligionHappiness;
	int m_iStateReligionUnitProductionModifier;
	int m_iStateReligionBuildingProductionModifier;
	int m_iStateReligionFreeExperience;
	int m_iExpInBorderModifier;

	bool m_bMilitaryFoodProduction;
	//bool m_bNoUnhealthyPopulation;
	int m_iUnhealthyPopulationModifier; // K-Mod
	bool m_bBuildingOnlyHealthy;
	bool m_bNoForeignTrade;
	bool m_bNoCorporations;
	bool m_bNoForeignCorporations;
	bool m_bStateReligion;
	bool m_bNoNonStateReligionSpread;

	CvWString m_szWeLoveTheKingKey;

	int* m_piYieldModifier;
	int* m_piCapitalYieldModifier;
	int* m_piTradeYieldModifier;
	int* m_piCommerceModifier;
	int* m_piCapitalCommerceModifier;
	int* m_piSpecialistExtraCommerce;
	int* m_paiBuildingHappinessChanges;
	int* m_paiBuildingHealthChanges;
	int* m_paiFeatureHappinessChanges;

	bool* m_pabHurry;
	bool* m_pabSpecialBuildingNotRequired;
	bool* m_pabSpecialistValid;

	int** m_ppiImprovementYieldChanges;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvCivicOptionInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvCivicOptionInfo : public CvInfoBase
{
public:
	CvCivicOptionInfo();
	~CvCivicOptionInfo();

	bool getTraitNoUpkeep(int i) const; // Exposed to Python
	bool read(CvXMLLoadUtility* pXML);

protected:
	bool* m_pabTraitNoUpkeep;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvUpkeepInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvUpkeepInfo : public CvInfoBase
{
public:
	CvUpkeepInfo();

	int getPopulationPercent() const; //	Exposed to Python
	int getCityPercent() const;	//	Exposed to Python

	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iPopulationPercent;
	int m_iCityPercent;

};

#endif
