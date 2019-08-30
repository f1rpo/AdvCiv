#pragma once

#ifndef CV_INFO_UNIT_H
#define CV_INFO_UNIT_H

/*  advc.003x: Cut from CvInfos.h. Unit-related classes:
	CvUnitInfo
	CvUnitArtStyleTypeInfo (not derived from CvAssetInfoBase; doesn't belong in CvInfo_Asset.h)
	CvUnitClassInfo (for the mapping to CvUnitInfo, CvCivilizationInfo gets precompiled)
	CvSpecialUnitInfo
	CvPromotionInfo
	CvEspionageMissionInfo
	CvBuildInfo (via CvInfo_Build.h) */
#include "CvInfo_Build.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvUnitInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvUnitInfo : public CvHotkeyInfo
{
public: /*  All const functions are exposed to Python except some related to art and those added by mods.
			advc.130f: inlined a select few; should perhaps inline most of the non-array getters. */
	CvUnitInfo();
	~CvUnitInfo();

	int getAIWeight() const;
	inline int getProductionCost() const { return m_iProductionCost; }
	int getHurryCostModifier() const;
	int getAdvancedStartCost() const;
	int getAdvancedStartCostIncrease() const;
	int getMinAreaSize() const;
	inline int getMoves() const { return m_iMoves; }
	inline int getAirRange() const { return m_iAirRange; }
	int getAirUnitCap() const;
	int getDropRange() const;
	int getNukeRange() const;
	inline int getWorkRate() const { return m_iWorkRate; }
	int getBaseDiscover() const;
	int getDiscoverMultiplier() const;
	int getBaseHurry() const;
	int getHurryMultiplier() const;
	int getBaseTrade() const;
	int getTradeMultiplier() const;
	int getGreatWorkCulture() const;
	int getEspionagePoints() const;
	inline int getCombat() const { return m_iCombat; }
	void setCombat(int iNum);
	inline int getCombatLimit() const { return m_iCombatLimit; }
	inline int getAirCombat() const { return m_iAirCombat; }
	inline int getAirCombatLimit() const { return m_iAirCombatLimit; }
	int getXPValueAttack() const;
	int getXPValueDefense() const;
	inline int getFirstStrikes() const { return m_iFirstStrikes; }
	inline int getChanceFirstStrikes() const { return m_iChanceFirstStrikes; }
	int getInterceptionProbability() const;
	int getEvasionProbability() const;
	inline int getWithdrawalProbability() const { return m_iWithdrawalProbability; }
	inline int getCollateralDamage() const { return m_iCollateralDamage; }
	int getCollateralDamageLimit() const;
	int getCollateralDamageMaxUnits() const;
	int getCityAttackModifier() const;
	int getCityDefenseModifier() const;
	int getAnimalCombatModifier() const;
	int getBarbarianCombatModifier() const; // advc.315c
	int getHillsAttackModifier() const;
	int getHillsDefenseModifier() const;
	int getBombRate() const;
	int getBombardRate() const;
	int getSpecialCargo() const;
	int getDomainCargo() const;

	int getCargoSpace() const;
	int getConscriptionValue() const;
	int getCultureGarrisonValue() const;
	int getExtraCost() const;
	int getAssetValue() const;
	int getPowerValue() const;
	inline int getUnitClassType() const { return m_iUnitClassType; }
	int getSpecialUnitType() const;
	int getUnitCaptureClassType() const;
	inline int getUnitCombatType() const { return m_iUnitCombatType; }
	// advc.130f: force-inlined for CvArea::canBeEntered
	__forceinline int getDomainType() const { return m_iDomainType; }
	int getDefaultUnitAIType() const;
	inline int getInvisibleType() const { return m_iInvisibleType; }
	int getSeeInvisibleType(int i) const;
	inline int getNumSeeInvisibleTypes() const { return (int)m_aiSeeInvisibleTypes.size(); }
	int getAdvisorType() const;
	int getHolyCity() const;
	int getReligionType() const;
	int getStateReligion() const;
	int getPrereqReligion() const;
	int getPrereqCorporation() const;
	int getPrereqBuilding() const;
	int getPrereqAndTech() const;
	int getPrereqAndBonus() const;
	int getGroupSize() const; // the initial number of individuals in the unit group
	int getGroupDefinitions() const; // the number of UnitMeshGroups for this unit
	int getMeleeWaveSize() const;
	int getRangedWaveSize() const;
	int getNumUnitNames() const;
	int getCommandType() const;
	void setCommandType(int iNewType);

	inline bool isAnimal() const { return m_bAnimal; }
	bool isFoodProduction() const;
	bool isNoBadGoodies() const;
	bool isOnlyDefensive() const;
	bool isOnlyAttackAnimals() const; // advc.315a
	bool isOnlyAttackBarbarians() const; // advc.315b
	bool isNoCapture() const;
	bool isQuickCombat() const;
	bool isRivalTerritory() const;
	bool isMilitaryHappiness() const;
	bool isMilitarySupport() const;
	bool isMilitaryProduction() const;
	bool isPillage() const;
	inline bool isSpy() const { return m_bSpy; }
	bool isSabotage() const;
	bool isDestroy() const;
	bool isStealPlans() const;
	bool isInvestigate() const;
	bool isCounterSpy() const;
	bool isFound() const;
	bool isGoldenAge() const;
	inline bool isInvisible() const { return m_bInvisible; }
	void setInvisible(bool bEnable) ;
	bool isFirstStrikeImmune() const;
	bool isNoDefensiveBonus() const;
	bool isIgnoreBuildingDefense() const;
	// advc.003f: force-inlined for CvArea::canBeEntered
	__forceinline bool isCanMoveImpassable() const { return m_bCanMoveImpassable; }
	inline bool isCanMoveAllTerrain() const { return m_bCanMoveAllTerrain; }
	bool isFlatMovementCost() const;
	bool isIgnoreTerrainCost() const;
	bool isNukeImmune() const;
	bool isPrereqBonuses() const;
	bool isPrereqReligion() const;
	bool isMechUnit() const;
	bool isRenderBelowWater() const;
	bool isRenderAlways() const;
	bool isSuicide() const;
	bool isLineOfSight() const;
	bool isHiddenNationality() const;
	bool isAlwaysHostile() const;
	inline bool isNoRevealMap() const { return m_bNoRevealMap; }

	float getUnitMaxSpeed() const;
	float getUnitPadTime() const;

	// Array access:

	int getPrereqAndTechs(int i) const;
	inline bool isAnyPrereqAndTech() const { return (m_piPrereqAndTechs != NULL); } // advc.003t
	int getPrereqOrBonuses(int i) const;
	inline bool isAnyPrereqOrBonus() const { return (m_piPrereqOrBonuses != NULL); } // advc.003t
	// <advc.905b>
	int getSpeedBonuses(int i) const;
	int getExtraMoves(int i) const;
	// </advc.905b>
	int getProductionTraits(int i) const;
	int getFlavorValue(int i) const;
	int getTerrainAttackModifier(int i) const;
	int getTerrainDefenseModifier(int i) const;
	int getFeatureAttackModifier(int i) const;
	int getFeatureDefenseModifier(int i) const;
	int getUnitClassAttackModifier(int i) const;
	inline bool isAnyUnitClassAttackModifier() const { return (m_piUnitClassAttackModifier != NULL); } // advc.003t
	int getUnitClassDefenseModifier(int i) const;
	inline bool isAnyUnitClassDefenseModifier() const { return (m_piUnitClassDefenseModifier != NULL); } // advc.003t
	int getUnitCombatModifier(int i) const;
	int getUnitCombatCollateralImmune(int i) const;
	int getDomainModifier(int i) const;
	int getBonusProductionModifier(int i) const;
	inline bool isAnyBonusProductionModifier() const { return (m_piBonusProductionModifier != NULL); } // advc.003t
	int getUnitGroupRequired(int i) const;
	int getReligionSpreads(int i) const;
	int getCorporationSpreads(int i) const;
	int getTerrainPassableTech(int i) const;
	inline bool isAnyTerrainPassableTech() const { return (m_piTerrainPassableTech != NULL); } // advc.003t
	int getFeaturePassableTech(int i) const;
	int getFlankingStrikeUnitClass(int i) const;
	inline bool isAnyFlankingStrikeUnitClass() const { return (m_piFlankingStrikeUnitClass != NULL); } // advc.003t

	bool getUpgradeUnitClass(int i) const;
	inline bool isAnyUpgradeUnitClass() const { return (m_pbUpgradeUnitClass != NULL); } // advc.003t
	bool getTargetUnitClass(int i) const;
	inline bool isAnyTargetUnitClass() const { return (m_pbTargetUnitClass != NULL); } // advc.003t
	bool getTargetUnitCombat(int i) const;
	bool getDefenderUnitClass(int i) const;
	inline bool isAnyDefenderUnitClass() const { return (m_pbDefenderUnitClass != NULL); } // advc.003t
	bool getDefenderUnitCombat(int i) const;
	bool getUnitAIType(int i) const;
	bool getNotUnitAIType(int i) const;
	inline bool isAnyNotUnitAIType() const { return (m_pbNotUnitAIType != NULL); } // advc.003t
	bool getBuilds(int i) const;
	inline bool isAnyBuilds() const { return (m_pbBuilds != NULL); } // advc.003t
	bool getGreatPeoples(int i) const;
	bool getBuildings(int i) const;
	inline bool isAnyBuildings() const { return (m_pbBuildings != NULL); } // advc.003t
	bool getForceBuildings(int i) const; // advc.003t: Dummy function
	bool getTerrainImpassable(int i) const;
	inline bool isAnyTerrainImpassable() const { return (m_pbTerrainImpassable != NULL); } // advc.003t
	bool getFeatureImpassable(int i) const;
	inline bool isAnyFeatureImpassable() const { return (m_pbFeatureImpassable != NULL); } // advc.003t
	bool getTerrainNative(int i) const;
	bool getFeatureNative(int i) const;
	bool getFreePromotions(int i) const;
	inline bool isAnyFreePromotions() const { return (m_pbFreePromotions != NULL); } // advc.003t
	int getLeaderPromotion() const;
	int getLeaderExperience() const;

	const TCHAR* getEarlyArtDefineTag(int i, UnitArtStyleTypes eStyle) const;
	void setEarlyArtDefineTag(int i, const TCHAR* szVal);
	const TCHAR* getLateArtDefineTag(int i, UnitArtStyleTypes eStyle) const;
	void setLateArtDefineTag(int i, const TCHAR* szVal);
	const TCHAR* getMiddleArtDefineTag(int i, UnitArtStyleTypes eStyle) const;
	void setMiddleArtDefineTag(int i, const TCHAR* szVal);
	const TCHAR* getUnitNames(int i) const;
	const TCHAR* getFormationType() const;

	const TCHAR* getButton() const;
	void updateArtDefineButton();

	const CvArtInfoUnit* getArtInfo(int i, EraTypes eEra, UnitArtStyleTypes eStyle) const;
	#if SERIALIZE_CVINFOS
	void read(FDataStreamBase*);
	void write(FDataStreamBase*);
	#endif
	bool read(CvXMLLoadUtility* pXML);
	// <advc.315>
	inline bool isMostlyDefensive() const {
		return isOnlyDefensive() || isOnlyAttackAnimals() || isOnlyAttackBarbarians();
	} // </advc.315>
	// advc.003b:
	static inline bool canAnyMoveAllTerrain() { return m_bCanAnyMoveAllTerrain; }

protected:
	static bool m_bCanAnyMoveAllTerrain; // advc.003b

	int m_iAIWeight;
	int m_iProductionCost;
	int m_iHurryCostModifier;
	int m_iAdvancedStartCost;
	int m_iAdvancedStartCostIncrease;
	int m_iMinAreaSize;
	int m_iMoves;
	int m_iAirRange;
	int m_iAirUnitCap;
	int m_iDropRange;
	int m_iNukeRange;
	int m_iWorkRate;
	int m_iBaseDiscover;
	int m_iDiscoverMultiplier;
	int m_iBaseHurry;
	int m_iHurryMultiplier;
	int m_iBaseTrade;
	int m_iTradeMultiplier;
	int m_iGreatWorkCulture;
	int m_iEspionagePoints;
	int m_iCombat;
	int m_iCombatLimit;
	int m_iAirCombat;
	int m_iAirCombatLimit;
	int m_iXPValueAttack;
	int m_iXPValueDefense;
	int m_iFirstStrikes;
	int m_iChanceFirstStrikes;
	int m_iInterceptionProbability;
	int m_iEvasionProbability;
	int m_iWithdrawalProbability;
	int m_iCollateralDamage;
	int m_iCollateralDamageLimit;
	int m_iCollateralDamageMaxUnits;
	int m_iCityAttackModifier;
	int m_iCityDefenseModifier;
	int m_iAnimalCombatModifier;
	int m_iBarbarianCombatModifier; // advc.315
	int m_iHillsAttackModifier;
	int m_iHillsDefenseModifier;
	int m_iBombRate;
	int m_iBombardRate;
	int m_iSpecialCargo;

	int m_iDomainCargo;
	int m_iCargoSpace;
	int m_iConscriptionValue;
	int m_iCultureGarrisonValue;
	int m_iExtraCost;
	int m_iAssetValue;
	int m_iPowerValue;
	int m_iUnitClassType;
	int m_iSpecialUnitType;
	int m_iUnitCaptureClassType;
	int m_iUnitCombatType;
	int m_iDomainType;
	int m_iDefaultUnitAIType;
	int m_iInvisibleType;
	int m_iAdvisorType;
	int m_iHolyCity;
	int m_iReligionType;
	int m_iStateReligion;
	int m_iPrereqReligion;
	int m_iPrereqCorporation;
	int m_iPrereqBuilding;
	int m_iPrereqAndTech;
	int m_iPrereqAndBonus;
	int m_iGroupSize;
	int m_iGroupDefinitions;
	int m_iUnitMeleeWaveSize;
	int m_iUnitRangedWaveSize;
	int m_iNumUnitNames;
	int m_iCommandType;
	int m_iLeaderExperience;

	bool m_bAnimal;
	bool m_bFoodProduction;
	bool m_bNoBadGoodies;
	bool m_bOnlyDefensive;
	bool m_bOnlyAttackAnimals; // advc.315a
	bool m_bOnlyAttackBarbarians; // advc.315b
	bool m_bNoCapture;
	bool m_bQuickCombat;
	bool m_bRivalTerritory;
	bool m_bMilitaryHappiness;
	bool m_bMilitarySupport;
	bool m_bMilitaryProduction;
	bool m_bPillage;
	bool m_bSpy;
	bool m_bSabotage;
	bool m_bDestroy;
	bool m_bStealPlans;
	bool m_bInvestigate;
	bool m_bCounterSpy;
	bool m_bFound;
	bool m_bGoldenAge;
	bool m_bInvisible;
	bool m_bFirstStrikeImmune;
	bool m_bNoDefensiveBonus;
	bool m_bIgnoreBuildingDefense;
	bool m_bCanMoveImpassable;
	bool m_bCanMoveAllTerrain;
	bool m_bFlatMovementCost;
	bool m_bIgnoreTerrainCost;
	bool m_bNukeImmune;
	bool m_bPrereqBonuses;
	bool m_bPrereqReligion;
	bool m_bMechanized;
	bool m_bRenderBelowWater;
	bool m_bRenderAlways;
	bool m_bSuicide;
	bool m_bLineOfSight;
	bool m_bHiddenNationality;
	bool m_bAlwaysHostile;
	bool m_bNoRevealMap;
	int m_iLeaderPromotion;

	float m_fUnitMaxSpeed;
	float m_fUnitPadTime;

	int* m_piPrereqAndTechs;
	int* m_piPrereqOrBonuses;
	int* m_piSpeedBonuses[2]; // advc.905b
	int* m_piProductionTraits;
	int* m_piFlavorValue;
	int* m_piTerrainAttackModifier;
	int* m_piTerrainDefenseModifier;
	int* m_piFeatureAttackModifier;
	int* m_piFeatureDefenseModifier;
	int* m_piUnitClassAttackModifier;
	int* m_piUnitClassDefenseModifier;
	int* m_piUnitCombatModifier;
	int* m_piUnitCombatCollateralImmune;
	int* m_piDomainModifier;
	int* m_piBonusProductionModifier;
	int* m_piUnitGroupRequired;
	int* m_piReligionSpreads;
	int* m_piCorporationSpreads;
	int* m_piTerrainPassableTech;
	int* m_piFeaturePassableTech;
	int* m_piFlankingStrikeUnitClass;

	bool* m_pbUpgradeUnitClass;
	bool* m_pbTargetUnitClass;
	bool* m_pbTargetUnitCombat;
	bool* m_pbDefenderUnitClass;
	bool* m_pbDefenderUnitCombat;
	bool* m_pbUnitAIType;
	bool* m_pbNotUnitAIType;
	bool* m_pbBuilds;
	bool* m_pbGreatPeoples;
	bool* m_pbBuildings;
	//bool* m_pbForceBuildings; // advc.003t
	bool* m_pbTerrainNative;
	bool* m_pbFeatureNative;
	bool* m_pbTerrainImpassable;
	bool* m_pbFeatureImpassable;
	bool* m_pbFreePromotions;

	CvString* m_paszEarlyArtDefineTags;
	CvString* m_paszLateArtDefineTags;
	CvString* m_paszMiddleArtDefineTags;
	CvString* m_paszUnitNames;
	CvString m_szFormationType;
	CvString m_szArtDefineButton;

	std::vector<int> m_aiSeeInvisibleTypes;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvUnitArtStyleTypeInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvUnitArtStyleTypeInfo : public CvInfoBase
{
public:
	const TCHAR* getEarlyArtDefineTag(int /*Mesh Index*/ i, int /*UnitType*/ j) const;
	void setEarlyArtDefineTag(int /*Mesh Index*/ i, int /*UnitType*/ j, const TCHAR* szVal);
	const TCHAR* getLateArtDefineTag(int i, int j) const;
	void setLateArtDefineTag(int i, int j, const TCHAR* szVal);
	const TCHAR* getMiddleArtDefineTag(int i, int j) const;
	void setMiddleArtDefineTag(int i, int j, const TCHAR* szVal);

	bool read(CvXMLLoadUtility* pXML);

protected:
	struct ArtDefneTag
	{
		int iMeshIndex;
		int iUnitType;
		CvString szTag;
	};
	typedef std::vector<ArtDefneTag> ArtDefineArray;
	ArtDefineArray m_azEarlyArtDefineTags;
	ArtDefineArray m_azLateArtDefineTags;
	ArtDefineArray m_azMiddleArtDefineTags;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvUnitClassInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvUnitClassInfo : public CvInfoBase
{
public: // All the const functions are exposed to Python
	CvUnitClassInfo();

	int getMaxGlobalInstances() const;
	int getMaxTeamInstances() const;
	int getMaxPlayerInstances() const;
	int getInstanceCostModifier() const;
	int getDefaultUnitIndex() const;

	bool read(CvXMLLoadUtility* pXML);
	bool readPass3();

protected:
	int m_iMaxGlobalInstances;
	int m_iMaxTeamInstances;
	int m_iMaxPlayerInstances;
	int m_iInstanceCostModifier;
	int m_iDefaultUnitIndex;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvSpecialUnitInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvSpecialUnitInfo : public CvInfoBase
{
public:

	CvSpecialUnitInfo();
	~CvSpecialUnitInfo();

	bool isValid() const;
	bool isCityLoad() const;

	bool isCarrierUnitAIType(int i) const; // Exposed to Python
	int getProductionTraits(int i) const; // Exposed to Python

	bool read(CvXMLLoadUtility* pXML);

protected:
	bool m_bValid;
	bool m_bCityLoad;

	bool* m_pbCarrierUnitAITypes;
	int* m_piProductionTraits;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvPromotionInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvPromotionInfo :	public CvHotkeyInfo
{
public: // All the const functions are exposed to Python
	CvPromotionInfo();
	~CvPromotionInfo();

	int getLayerAnimationPath() const;
	int getPrereqPromotion() const;
	void setPrereqPromotion(int i);
	int getPrereqOrPromotion1() const;
	void setPrereqOrPromotion1(int i);
	int getPrereqOrPromotion2() const;
	void setPrereqOrPromotion2(int i);
	// K-Mod, 7/jan/11, karadoc
	int getPrereqOrPromotion3() const;
	void setPrereqOrPromotion3(int i); // K-Mod end
	int getTechPrereq() const;
	int getStateReligionPrereq() const;
	int getVisibilityChange() const;
	int getMovesChange() const;
	int getMoveDiscountChange() const;
	int getAirRangeChange() const;
	int getInterceptChange() const;
	int getEvasionChange() const;
	int getWithdrawalChange() const;
	int getCargoChange() const;
	int getCollateralDamageChange() const;
	int getBombardRateChange() const;
	int getFirstStrikesChange() const;
	int getChanceFirstStrikesChange() const;
	int getEnemyHealChange() const;
	int getNeutralHealChange() const;
	int getFriendlyHealChange() const;
	int getSameTileHealChange() const;
	int getAdjacentTileHealChange() const;
	int getCombatPercent() const;
	int getCityAttackPercent() const;
	int getCityDefensePercent() const;
	int getHillsAttackPercent() const;
	int getHillsDefensePercent() const;
	int getCommandType() const;
	void setCommandType(int iNewType);

	int getRevoltProtection() const;
	int getCollateralDamageProtection() const;
	int getPillageChange() const;
	int getUpgradeDiscount() const;
	int getExperiencePercent() const;
	int getKamikazePercent() const;

	bool isLeader() const;
	// advc.164: was isBlitz
	int getBlitz() const;
	bool isAmphib() const;
	bool isRiver() const;
	bool isEnemyRoute() const;
	bool isAlwaysHeal() const;
	bool isHillsDoubleMove() const;
	bool isImmuneToFirstStrikes() const;

	const TCHAR* getSound() const;
	void setSound(const TCHAR* szVal);

	// Array access:

	int getTerrainAttackPercent(int i) const;
	int getTerrainDefensePercent(int i) const;
	int getFeatureAttackPercent(int i) const;
	int getFeatureDefensePercent(int i) const;
	int getUnitCombatModifierPercent(int i) const;
	int getDomainModifierPercent(int i) const;

	bool getTerrainDoubleMove(int i) const;
	bool getFeatureDoubleMove(int i) const;
	bool getUnitCombat(int i) const;
	#if SERIALIZE_CVINFOS
	void read(FDataStreamBase* stream);
	void write(FDataStreamBase* stream);
	#endif
	bool read(CvXMLLoadUtility* pXML);
	bool readPass2(CvXMLLoadUtility* pXML);

protected:
	int m_iLayerAnimationPath;
	int m_iPrereqPromotion;
	int m_iPrereqOrPromotion1;
	int m_iPrereqOrPromotion2;
	int m_iPrereqOrPromotion3; // K-Mod

	int m_iTechPrereq;
	int m_iStateReligionPrereq;
	int m_iVisibilityChange;
	int m_iMovesChange;
	int m_iMoveDiscountChange;
	int m_iAirRangeChange;
	int m_iInterceptChange;
	int m_iEvasionChange;
	int m_iWithdrawalChange;
	int m_iCargoChange;
	int m_iCollateralDamageChange;
	int m_iBombardRateChange;
	int m_iFirstStrikesChange;
	int m_iChanceFirstStrikesChange;
	int m_iEnemyHealChange;
	int m_iNeutralHealChange;
	int m_iFriendlyHealChange;
	int m_iSameTileHealChange;
	int m_iAdjacentTileHealChange;
	int m_iCombatPercent;
	int m_iCityAttackPercent;
	int m_iCityDefensePercent;
	int m_iHillsAttackPercent;
	int m_iHillsDefensePercent;
	int m_iCommandType;
	int m_iRevoltProtection;
	int m_iCollateralDamageProtection;
	int m_iPillageChange;
	int m_iUpgradeDiscount;
	int m_iExperiencePercent;
	int m_iKamikazePercent;

	bool m_bLeader;
	//bool m_bBlitz;
	int m_iBlitz; // advc.164
	bool m_bAmphib;
	bool m_bRiver;
	bool m_bEnemyRoute;
	bool m_bAlwaysHeal;
	bool m_bHillsDoubleMove;
	bool m_bImmuneToFirstStrikes;

	CvString m_szSound;

	int* m_piTerrainAttackPercent;
	int* m_piTerrainDefensePercent;
	int* m_piFeatureAttackPercent;
	int* m_piFeatureDefensePercent;
	int* m_piUnitCombatModifierPercent;
	int* m_piDomainModifierPercent;

	bool* m_pbTerrainDoubleMove;
	bool* m_pbFeatureDoubleMove;
	bool* m_pbUnitCombat;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvEspionageMissionInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvEspionageMissionInfo : public CvInfoBase
{
public:
	CvEspionageMissionInfo();

	inline int getCost() const { return m_iCost; } // advc.003f: inline
	inline bool isPassive() const { return m_bIsPassive; } // advc.003f: inline
	bool isTwoPhases() const;
	bool isTargetsCity() const;
	bool isSelectPlot() const;

	int getTechPrereq() const;
	int getVisibilityLevel() const;
	bool isInvestigateCity() const;
	bool isSeeDemographics() const;
	bool isNoActiveMissions() const;
	bool isSeeResearch() const;

	bool isDestroyImprovement() const;
	int getDestroyBuildingCostFactor() const;
	int getDestroyUnitCostFactor() const;
	int getDestroyProjectCostFactor() const;
	int getDestroyProductionCostFactor() const;
	int getBuyUnitCostFactor() const;
	int getBuyCityCostFactor() const;
	int getStealTreasuryTypes() const;
	int getCityInsertCultureAmountFactor() const;
	int getCityInsertCultureCostFactor() const;
	int getCityPoisonWaterCounter() const;
	int getCityUnhappinessCounter() const;
	int getCityRevoltCounter() const;
	int getBuyTechCostFactor() const;
	int getSwitchCivicCostFactor() const;
	int getSwitchReligionCostFactor() const;
	int getPlayerAnarchyCounter() const;
	int getCounterespionageNumTurns() const;
	int getCounterespionageMod() const;
	int getDifficultyMod() const;
	inline bool isReturnToCapital() const { return m_bReturnToCapital; } // advc.103

	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iCost;
	bool m_bIsPassive;
	bool m_bIsTwoPhases;
	bool m_bTargetsCity;
	bool m_bSelectPlot;

	int m_iTechPrereq;
	int m_iVisibilityLevel;
	bool m_bInvestigateCity;
	bool m_bSeeDemographics;
	bool m_bNoActiveMissions;
	bool m_bSeeResearch;

	bool m_bDestroyImprovement;
	int m_iDestroyBuildingCostFactor;
	int m_iDestroyUnitCostFactor;
	int m_iDestroyProjectCostFactor;
	int m_iDestroyProductionCostFactor;
	int m_iBuyUnitCostFactor;
	int m_iBuyCityCostFactor;
	int m_iStealTreasuryTypes;
	int m_iCityInsertCultureAmountFactor;
	int m_iCityInsertCultureCostFactor;
	int m_iCityPoisonWaterCounter;
	int m_iCityUnhappinessCounter;
	int m_iCityRevoltCounter;
	int m_iBuyTechCostFactor;
	int m_iSwitchCivicCostFactor;
	int m_iSwitchReligionCostFactor;
	int m_iPlayerAnarchyCounter;
	int m_iCounterespionageNumTurns;
	int m_iCounterespionageMod;
	int m_iDifficultyMod;
	bool m_bReturnToCapital; // advc.103
};

#endif
