#pragma once

#ifndef CV_INFO_BUILDING_H
#define CV_INFO_BUILDING_H

/*  advc.003x: Cut from CvInfos.h. Building-related classes:
	CvBuildingInfo
	CvBuildingClassInfo (for the mapping to CvBuildingInfo, CvCivilizationInfo gets precompiled)
	CvSpecialBuildingInfo
	CvVoteSourceInfo (tied to a building)
	CvVoteInfo (should stay with CvVoteSourceInfo)
	CvProjectInfo (very similar to a building) */
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvBuildingInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvBuildingInfo : public CvHotkeyInfo
{
public: // All the const functions are exposed to Python // advc.130f (tbd.): inline most of these?
	CvBuildingInfo();
	~CvBuildingInfo();
	// <advc.tag>
	enum IntElementTypes
	{
		RaiseDefense = CvHotkeyInfo::NUM_INT_ELEMENT_TYPES, // advc.004c
		NUM_INT_ELEMENT_TYPES
	};
	enum BoolElementTypes // unused so far
	{
		NUM_BOOL_ELEMENT_TYPES = CvHotkeyInfo::NUM_BOOL_ELEMENT_TYPES
	};
	using CvXMLInfo::get; // unhide
	__forceinline int get(IntElementTypes e) const
	{
		return get(static_cast<CvXMLInfo::IntElementTypes>(e));
	}
	__forceinline int get(BoolElementTypes e) const
	{
		return get(static_cast<CvXMLInfo::BoolElementTypes>(e));
	} // </advc.tag>

	inline int getBuildingClassType() const { return m_iBuildingClassType; } // advc.130f: inline
	int getVictoryPrereq() const;
	int getFreeStartEra() const;
	int getMaxStartEra() const;
	int getObsoleteTech() const;
	int getPrereqAndTech() const;
	int getNoBonus() const;
	int getPowerBonus() const;
	int getFreeBonus() const;
	int getNumFreeBonuses() const;
	int getFreeBuildingClass() const;
	int getFreePromotion() const;
	int getCivicOption() const;
	int getAIWeight() const;
	int getProductionCost() const;
	int getHurryCostModifier() const;
	int getHurryAngerModifier() const;
	int getAdvancedStartCost() const;
	int getAdvancedStartCostIncrease() const;
	int getMinAreaSize() const;
	int getNumCitiesPrereq() const;
	int getNumTeamsPrereq() const;
	int getUnitLevelPrereq() const;
	int getMinLatitude() const;
	int getMaxLatitude() const;
	int getGreatPeopleRateModifier() const;
	int getGreatGeneralRateModifier() const;
	int getDomesticGreatGeneralRateModifier() const;
	int getGlobalGreatPeopleRateModifier() const;
	int getAnarchyModifier() const;
	int getGoldenAgeModifier() const;
	int getGlobalHurryModifier() const;
	int getFreeExperience() const;
	int getGlobalFreeExperience() const;
	int getFoodKept() const;
	int getAirlift() const;
	int getAirModifier() const;
	int getAirUnitCapacity() const;
	int getNukeModifier() const;
	int getNukeExplosionRand() const;
	int getFreeSpecialist() const;
	int getAreaFreeSpecialist() const;
	int getGlobalFreeSpecialist() const;
	int getHappiness() const;
	int getAreaHappiness() const;
	int getGlobalHappiness() const;
	int getStateReligionHappiness() const;
	int getWorkerSpeedModifier() const;
	int getMilitaryProductionModifier() const;
	int getSpaceProductionModifier() const;
	int getGlobalSpaceProductionModifier() const;
	int getTradeRoutes() const;
	int getCoastalTradeRoutes() const;
	int getAreaTradeRoutes() const; // advc.310: Renamed; was getGlobalTradeRoutes.
	int getTradeRouteModifier() const;
	int getForeignTradeRouteModifier() const;
	int getAssetValue() const;
	int getPowerValue() const;
	int getSpecialBuildingType() const;
	int getAdvisorType() const;
	int getHolyCity() const;
	int getReligionType() const;
	int getStateReligion() const;
	int getPrereqReligion() const;
	int getPrereqCorporation() const;
	int getFoundsCorporation() const;
	int getGlobalReligionCommerce() const;
	int getGlobalCorporationCommerce() const;
	int getPrereqAndBonus() const;
	int getGreatPeopleUnitClass() const;
	int getGreatPeopleRateChange() const;
	int getConquestProbability() const;
	int getMaintenanceModifier() const;
	int getWarWearinessModifier() const;
	int getGlobalWarWearinessModifier() const;
	int getEnemyWarWearinessModifier() const;
	int getHealRateChange() const;
	int getHealth() const;
	int getAreaHealth() const;
	int getGlobalHealth() const;
	int getGlobalPopulationChange() const;
	int getFreeTechs() const;
	inline int getDefenseModifier() const { return m_iDefenseModifier; } // advc.130f: inline
	int getBombardDefenseModifier() const;
	int getAllCityDefenseModifier() const;
	int getEspionageDefenseModifier() const;
	int getMissionType() const;
	void setMissionType(int iNewType);
	int getVoteSourceType() const;

	float getVisibilityPriority() const;

	bool isTeamShare() const;
	bool isWater() const;
	bool isRiver() const;
	bool isPower() const;
	bool isDirtyPower() const;
	bool isAreaCleanPower() const;
	bool isAreaBorderObstacle() const;
	bool isForceTeamVoteEligible() const;
	bool isCapital() const;
	bool isGovernmentCenter() const;
	bool isGoldenAge() const;
	bool isMapCentering() const;
	bool isNoUnhappiness() const;
	//bool isNoUnhealthyPopulation() const;
	int getUnhealthyPopulationModifier() const;	// K-Mod, Exposed to Python
	bool isBuildingOnlyHealthy() const;
	bool isNeverCapture() const;
	bool isNukeImmune() const;
	bool isPrereqReligion() const;
	bool isCenterInCity() const;
	bool isStateReligion() const;
	bool isAllowsNukes() const;

	const TCHAR* getConstructSound() const;
	void setConstructSound(const TCHAR* szVal);
	const TCHAR* getArtDefineTag() const;
	void setArtDefineTag(const TCHAR* szVal);
	const TCHAR* getMovieDefineTag() const;
	void setMovieDefineTag(const TCHAR* szVal);

	// Array access:

	int getYieldChange(int i) const;
	int* getYieldChangeArray() const;
	int getYieldModifier(int i) const;
	int* getYieldModifierArray() const;
	int getPowerYieldModifier(int i) const;
	int* getPowerYieldModifierArray() const;
	int getAreaYieldModifier(int i) const;
	int* getAreaYieldModifierArray() const;
	int getGlobalYieldModifier(int i) const;
	int* getGlobalYieldModifierArray() const;
	int getSeaPlotYieldChange(int i) const;
	int* getSeaPlotYieldChangeArray() const;
	int getRiverPlotYieldChange(int i) const;
	int* getRiverPlotYieldChangeArray() const;
	int getGlobalSeaPlotYieldChange(int i) const;
	int* getGlobalSeaPlotYieldChangeArray() const;

	int getCommerceChange(int i) const;
	int* getCommerceChangeArray() const;
	int getObsoleteSafeCommerceChange(int i) const;
	int* getObsoleteSafeCommerceChangeArray() const;
	int getCommerceChangeDoubleTime(int i) const;
	int getCommerceModifier(int i) const;
	int* getCommerceModifierArray() const;
	int getGlobalCommerceModifier(int i) const;
	int* getGlobalCommerceModifierArray() const;
	int getSpecialistExtraCommerce(int i) const;
	int* getSpecialistExtraCommerceArray() const;
	int getStateReligionCommerce(int i) const;
	int* getStateReligionCommerceArray() const;
	int getCommerceHappiness(int i) const;
	int getReligionChange(int i) const;
	inline bool isAnyReligionChange() const { return (m_piReligionChange != NULL); } // advc.003t
	int getSpecialistCount(int i) const;
	int getFreeSpecialistCount(int i) const;
	int getBonusHealthChanges(int i) const;
	inline bool isAnyBonusHealthChanges() const { return (m_piBonusHealthChanges != NULL); } // advc.003t
	int getBonusHappinessChanges(int i) const;
	inline bool isAnyBonusHappinessChanges() const { return (m_piBonusHappinessChanges != NULL); } // advc.003t
	int getBonusProductionModifier(int i) const;
	inline bool isAnyBonusProductionModifier() const { return (m_piBonusProductionModifier != NULL); } // advc.003t
	int getUnitCombatFreeExperience(int i) const;
	int getDomainFreeExperience(int i) const;
	int getDomainProductionModifier(int i) const;
	int getPrereqAndTechs(int i) const;
	inline bool isAnyPrereqAndTech() const { return (m_piPrereqAndTechs != NULL); } // advc.003t
	int getPrereqOrBonuses(int i) const;
	inline bool isAnyPrereqOrBonus() const { return (m_piPrereqOrBonuses != NULL); } // advc.003t
	int getProductionTraits(int i) const;
	int getHappinessTraits(int i) const;
	int getBuildingHappinessChanges(int i) const;
	inline bool isAnyBuildingHappinessChanges() const { return (m_piBuildingHappinessChanges != NULL); } // advc.003t
	int getPrereqNumOfBuildingClass(int i) const;
	inline bool isAnyPrereqNumOfBuildingClass() const { return (m_piPrereqNumOfBuildingClass != NULL); } // advc.003t
	int getFlavorValue(int i) const;
	int getImprovementFreeSpecialist(int i) const;
	inline bool isAnyImprovementFreeSpecialist() const { return (m_piImprovementFreeSpecialist != NULL); } // advc.003t

	bool isCommerceFlexible(int i) const;
	bool isCommerceChangeOriginalOwner(int i) const;
	bool isBuildingClassNeededInCity(int i) const;
	inline bool isAnyBuildingClassNeededInCity() const { return (m_pbBuildingClassNeededInCity != NULL); } // advc.003t

	int getSpecialistYieldChange(int i, int j) const;
	int* getSpecialistYieldChangeArray(int i) const;
	int getBonusYieldModifier(int i, int j) const;
	int* getBonusYieldModifierArray(int i) const;
	// UNOFFICIAL_PATCH, Efficiency, 06/27/10, Afforess & jdog5000: START  // advc.003t: inlined
	inline bool isAnySpecialistYieldChange() const { return m_bAnySpecialistYieldChange; }
	inline bool isAnyBonusYieldModifier() const { return m_bAnyBonusYieldModifier; }
	// UNOFFICIAL_PATCH: END
	// Other

	const CvArtInfoBuilding* getArtInfo() const;
	const CvArtInfoMovie* getMovieInfo() const;
	const TCHAR* getButton() const;
	const TCHAR* getMovie() const;

	#if SERIALIZE_CVINFOS
	void read(FDataStreamBase*);
	void write(FDataStreamBase*);
	#endif
	bool read(CvXMLLoadUtility* pXML);
	// <advc.310>
	static void setDomesticGreatGeneralRateModifierEnabled(bool b);
	static void setAreaTradeRoutesEnabled(bool b);
	static void setAreaBorderObstacleEnabled(bool b);
	// </advc.310>

protected:
	int m_iBuildingClassType;
	int m_iVictoryPrereq;
	int m_iFreeStartEra;
	int m_iMaxStartEra;
	int m_iObsoleteTech;
	int m_iPrereqAndTech;
	int m_iNoBonus;
	int m_iPowerBonus;
	int m_iFreeBonus;
	int m_iNumFreeBonuses;
	int m_iFreeBuildingClass;
	int m_iFreePromotion;
	int m_iCivicOption;
	int m_iAIWeight;
	int m_iProductionCost;
	int m_iHurryCostModifier;
	int m_iHurryAngerModifier;
	int m_iAdvancedStartCost;
	int m_iAdvancedStartCostIncrease;
	int m_iMinAreaSize;
	int m_iNumCitiesPrereq;
	int m_iNumTeamsPrereq;
	int m_iUnitLevelPrereq;
	int m_iMinLatitude;
	int m_iMaxLatitude;
	int m_iGreatPeopleRateModifier;
	int m_iGreatGeneralRateModifier;
	int m_iDomesticGreatGeneralRateModifier;
	int m_iGlobalGreatPeopleRateModifier;
	int m_iAnarchyModifier;
	int m_iGoldenAgeModifier;
	int m_iGlobalHurryModifier;
	int m_iFreeExperience;
	int m_iGlobalFreeExperience;
	int m_iFoodKept;
	int m_iAirlift;
	int m_iAirModifier;
	int m_iAirUnitCapacity;
	int m_iNukeModifier;
	int m_iNukeExplosionRand;
	int m_iFreeSpecialist;
	int m_iAreaFreeSpecialist;
	int m_iGlobalFreeSpecialist;
	int m_iHappiness;
	int m_iAreaHappiness;
	int m_iGlobalHappiness;
	int m_iStateReligionHappiness;
	int m_iWorkerSpeedModifier;
	int m_iMilitaryProductionModifier;
	int m_iSpaceProductionModifier;
	int m_iGlobalSpaceProductionModifier;
	int m_iTradeRoutes;
	int m_iCoastalTradeRoutes;
	int m_iAreaTradeRoutes; // advc.310: was m_iGlobalTradeRoutes
	int m_iTradeRouteModifier;
	int m_iForeignTradeRouteModifier;
	int m_iAssetValue;
	int m_iPowerValue;
	int m_iSpecialBuildingType;
	int m_iAdvisorType;
	int m_iHolyCity;
	int m_iReligionType;
	int m_iStateReligion;
	int m_iPrereqReligion;
	int m_iPrereqCorporation;
	int m_iFoundsCorporation;
	int m_iGlobalReligionCommerce;
	int m_iGlobalCorporationCommerce;
	int m_iPrereqAndBonus;
	int m_iGreatPeopleUnitClass;
	int m_iGreatPeopleRateChange;
	int m_iConquestProbability;
	int m_iMaintenanceModifier;
	int m_iWarWearinessModifier;
	int m_iGlobalWarWearinessModifier;
	int m_iEnemyWarWearinessModifier;
	int m_iHealRateChange;
	int m_iHealth;
	int m_iAreaHealth;
	int m_iGlobalHealth;
	int m_iGlobalPopulationChange;
	int m_iFreeTechs;
	int m_iDefenseModifier;
	int m_iBombardDefenseModifier;
	int m_iAllCityDefenseModifier;
	int m_iEspionageDefenseModifier;
	int m_iMissionType;
	int m_iVoteSourceType;

	float m_fVisibilityPriority;

	bool m_bTeamShare;
	bool m_bWater;
	bool m_bRiver;
	bool m_bPower;
	bool m_bDirtyPower;
	bool m_bAreaCleanPower;
	bool m_bAreaBorderObstacle;
	bool m_bForceTeamVoteEligible;
	bool m_bCapital;
	bool m_bGovernmentCenter;
	bool m_bGoldenAge;
	bool m_bMapCentering;
	bool m_bNoUnhappiness;
	int m_iUnhealthyPopulationModifier; // K-Mod: was m_bNoUnhealthyPopulation
	bool m_bBuildingOnlyHealthy;
	bool m_bNeverCapture;
	bool m_bNukeImmune;
	bool m_bPrereqReligion;
	bool m_bCenterInCity;
	bool m_bStateReligion;
	bool m_bAllowsNukes;

	CvString m_szConstructSound;
	CvString m_szArtDefineTag;
	CvString m_szMovieDefineTag;

	int* m_piPrereqAndTechs;
	int* m_piPrereqOrBonuses;
	int* m_piProductionTraits;
	int* m_piHappinessTraits;
	int* m_piSeaPlotYieldChange;
	int* m_piRiverPlotYieldChange;
	int* m_piGlobalSeaPlotYieldChange;
	int* m_piYieldChange;
	int* m_piYieldModifier;
	int* m_piPowerYieldModifier;
	int* m_piAreaYieldModifier;
	int* m_piGlobalYieldModifier;
	int* m_piCommerceChange;
	int* m_piObsoleteSafeCommerceChange;
	int* m_piCommerceChangeDoubleTime;
	int* m_piCommerceModifier;
	int* m_piGlobalCommerceModifier;
	int* m_piSpecialistExtraCommerce;
	int* m_piStateReligionCommerce;
	int* m_piCommerceHappiness;
	int* m_piReligionChange;
	int* m_piSpecialistCount;
	int* m_piFreeSpecialistCount;
	int* m_piBonusHealthChanges;
	int* m_piBonusHappinessChanges;
	int* m_piBonusProductionModifier;
	int* m_piUnitCombatFreeExperience;
	int* m_piDomainFreeExperience;
	int* m_piDomainProductionModifier;
	int* m_piBuildingHappinessChanges;
	int* m_piPrereqNumOfBuildingClass;
	int* m_piFlavorValue;
	int* m_piImprovementFreeSpecialist;

	bool* m_pbCommerceFlexible;
	bool* m_pbCommerceChangeOriginalOwner;
	bool* m_pbBuildingClassNeededInCity;

	int** m_ppaiSpecialistYieldChange;
	int** m_ppaiBonusYieldModifier;
	// UNOFFICIAL_PATCH, Efficiency, 06/27/10, Afforess & jdog5000: START
	bool m_bAnySpecialistYieldChange;
	bool m_bAnyBonusYieldModifier;
	// UNOFFICIAL_PATCH: END

	void addElements(std::vector<XMLElement*>& r) const; // advc.tag
	// <advc.310>
	static bool m_bEnabledAreaBorderObstacle;
	static bool m_bEnabledAreaTradeRoutes;
	static bool m_bEnabledDomesticGreatGeneralRateModifier;
	// </advc.310>
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvBuildingClassInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvBuildingClassInfo : public CvInfoBase
{
public: // All the const functions are exposed to Python
	CvBuildingClassInfo();
	~CvBuildingClassInfo();

	int getMaxGlobalInstances() const;
	int getMaxTeamInstances() const;
	int getMaxPlayerInstances() const;
	int getExtraPlayerInstances() const;
	int getDefaultBuildingIndex() const;

	bool isNoLimit() const;
	bool isMonument() const;

	int getVictoryThreshold(int i) const;

	bool read(CvXMLLoadUtility* pXML);
	bool readPass3();

protected:
	int m_iMaxGlobalInstances;
	int m_iMaxTeamInstances;
	int m_iMaxPlayerInstances;
	int m_iExtraPlayerInstances;
	int m_iDefaultBuildingIndex;

	bool m_bNoLimit;
	bool m_bMonument;

	int* m_piVictoryThreshold;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvSpecialBuildingInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvSpecialBuildingInfo : public CvInfoBase
{
public: // All the const functions are exposed to Python
	CvSpecialBuildingInfo();
	virtual ~CvSpecialBuildingInfo();

	int getObsoleteTech() const;
	int getTechPrereq() const;
	int getTechPrereqAnyone() const;

	bool isValid() const;

	int getProductionTraits(int i) const;

	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iObsoleteTech;
	int m_iTechPrereq;
	int m_iTechPrereqAnyone;

	bool m_bValid;

	int* m_piProductionTraits;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvVoteSourceInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvVoteSourceInfo : public CvInfoBase
{
public: // The const functions are exposed to Python
	CvVoteSourceInfo();
	virtual ~CvVoteSourceInfo();

	int getVoteInterval() const;
	int getFreeSpecialist() const;
	int getCivic() const;
	const CvWString getPopupText() const; // (not exposed to python)
	const CvWString getSecretaryGeneralText() const;

	std::wstring pyGetSecretaryGeneralText() { return getSecretaryGeneralText(); }

	int getReligionYield(int i) const;
	int getReligionCommerce(int i) const;

	bool read(CvXMLLoadUtility* pXML);
	bool readPass3();

protected:
	int m_iVoteInterval;
	int m_iFreeSpecialist;
	int m_iCivic;

	int* m_aiReligionYields;
	int* m_aiReligionCommerces;

	CvString m_szPopupText;
	CvString m_szSecretaryGeneralText;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvVoteInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvVoteInfo :	public CvInfoBase
{
public: // All the const functions are exposed to Python
	CvVoteInfo();
	virtual ~CvVoteInfo();

	int getPopulationThreshold() const;
	int getStateReligionVotePercent() const;
	int getTradeRoutes() const;
	int getMinVoters() const;

	bool isSecretaryGeneral() const;
	bool isVictory() const;
	bool isFreeTrade() const;
	bool isNoNukes() const;
	bool isCityVoting() const;
	bool isCivVoting() const;
	bool isDefensivePact() const;
	bool isOpenBorders() const;
	bool isForcePeace() const;
	bool isForceNoTrade() const;
	bool isForceWar() const;
	bool isAssignCity() const;

	bool isForceCivic(int i) const;
	bool isVoteSourceType(int i) const;

	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iPopulationThreshold;
	int m_iStateReligionVotePercent;
	int m_iTradeRoutes;
	int m_iMinVoters;

	bool m_bSecretaryGeneral;
	bool m_bVictory;
	bool m_bFreeTrade;
	bool m_bNoNukes;
	bool m_bCityVoting;
	bool m_bCivVoting;
	bool m_bDefensivePact;
	bool m_bOpenBorders;
	bool m_bForcePeace;
	bool m_bForceNoTrade;
	bool m_bForceWar;
	bool m_bAssignCity;

	bool* m_pbForceCivic;
	bool* m_abVoteSourceTypes;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvProjectInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvProjectInfo : public CvInfoBase
{
public: // All const functions are exposed to Python
	CvProjectInfo();
	~CvProjectInfo();

	int getVictoryPrereq() const;
	int getTechPrereq() const;
	int getAnyoneProjectPrereq() const;
	int getMaxGlobalInstances() const;
	int getMaxTeamInstances() const;
	int getProductionCost() const;
	int getNukeInterception() const;
	int getTechShare() const;
	int getEveryoneSpecialUnit() const;
	int getEveryoneSpecialBuilding() const;
	int getVictoryDelayPercent() const;
	int getSuccessRate() const;

	bool isSpaceship() const;
	bool isAllowsNukes() const;
	const char* getMovieArtDef() const;

	const TCHAR* getCreateSound() const;
	void setCreateSound(const TCHAR* szVal);

	// Arrays access:

	int getBonusProductionModifier(int i) const;
	int getVictoryThreshold(int i) const;
	int getVictoryMinThreshold(int i) const;
	int getProjectsNeeded(int i) const;

	bool read(CvXMLLoadUtility* pXML);
	bool readPass2(CvXMLLoadUtility* pXML);

protected:
	int m_iVictoryPrereq;
	int m_iTechPrereq;
	int m_iAnyoneProjectPrereq;
	int m_iMaxGlobalInstances;
	int m_iMaxTeamInstances;
	int m_iProductionCost;
	int m_iNukeInterception;
	int m_iTechShare;
	int m_iEveryoneSpecialUnit;
	int m_iEveryoneSpecialBuilding;
	int m_iVictoryDelayPercent;
	int m_iSuccessRate;

	bool m_bSpaceship;
	bool m_bAllowsNukes;

	CvString m_szCreateSound;
	CvString m_szMovieArtDef;

	int* m_piBonusProductionModifier;
	int* m_piVictoryThreshold;
	int* m_piVictoryMinThreshold;
	int* m_piProjectsNeeded;
};

#endif
