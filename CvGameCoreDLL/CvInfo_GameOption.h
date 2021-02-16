#pragma once

#ifndef CV_INFO_GAME_OPTION_H
#define CV_INFO_GAME_OPTION_H

/*  advc.003x: Cut from CvInfos.h. Info classes holding the game settings from
	the Custom Game and (multiplayer) Staging Room screens:
	CvGameOptionInfo, CvMPOptionInfo, CvEraInfo, CvGameSpeedInfo, CvTurnTimerInfo,
	CvVictoryInfo, CvHandicapInfo, CvWorldInfo, CvClimateInfo, CvSeaLevelInfo */

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvGameOptionInfo - Game options and their default values
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvGameOptionInfo : public CvInfoBase
{
public:
	CvGameOptionInfo();

	DllExport bool getDefault() const;
	DllExport bool getVisible() const;
	// <advc.054>
	void setVisible(bool b);
	bool getVisibleXML() const; // </advc.054>
	bool read(CvXMLLoadUtility* pXML);

private:
	bool m_bDefault;
	bool m_bVisible;
	bool m_bVisibleXML; // advc.054
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvMPOptionInfo - Multiplayer options and their default values
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvMPOptionInfo : public CvInfoBase
{
public:
	CvMPOptionInfo();

	bool getDefault() const;
	bool read(CvXMLLoadUtility* pXML);

private:
	bool m_bDefault;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvEraInfo - Used to manage different types of Art Styles
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvEraInfo :  /* advc.tag: */ public CvXMLInfo
{
public: // All the const functions are exposed to Python except those added by mods
	CvEraInfo();
	~CvEraInfo();
	// <advc.tag>
	enum IntElementTypes
	{	// <advc.groundbr>
		AIMaxGroundbreakingPenalty = CvXMLInfo::NUM_INT_ELEMENT_TYPES,
		HumanMaxGroundbreakingPenalty, // </advc.groundbr>
		AIEraFactor, // advc.erai
		NUM_INT_ELEMENT_TYPES
	};
	enum BoolElementTypes
	{	// <advc.erai>
		AIAgeOfExploration = CvXMLInfo::NUM_BOOL_ELEMENT_TYPES,
		AIAgeOfPestilence,
		AIAgeOfPollution,
		AIAgeOfFertility,
		AIAgeOfGuns,
		AIAtomicAge, // </advc.erai>
		AllGoodyTechs, // advc.314
		NUM_BOOL_ELEMENT_TYPES
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

	int getStartingUnitMultiplier() const;
	int getStartingDefenseUnits() const;
	int getStartingWorkerUnits() const;
	int getStartingExploreUnits() const;
	int getAdvancedStartPoints() const;
	int getStartingGold() const;
	int getFreePopulation() const;
	int getStartPercent() const;
	// advc.inl: inlined some getters
	inline int getGrowthPercent() const { return m_iGrowthPercent; }
	inline int getTrainPercent() const { return m_iTrainPercent; }
	inline int getConstructPercent() const { return m_iConstructPercent; }
	inline int getCreatePercent() const { return m_iCreatePercent; }
	inline int getResearchPercent() const { return m_iResearchPercent; }
	// BETTER_BTS_AI_MOD, Tech Diffusion, 08/21/09, jdog5000:
	inline int getTechCostModifier() const { return m_iTechCostModifier; }
	inline int getBuildPercent() const { return m_iBuildPercent; }
	inline int getImprovementPercent() const { return m_iImprovementPercent; }
	inline int getGreatPeoplePercent() const { return m_iGreatPeoplePercent; }
	inline int getCulturePercent() const  { return m_iCulturePercent; } // advc.126
	inline int getAnarchyPercent() const { return m_iAnarchyPercent; }
	int getEventChancePerTurn() const;
	int getSoundtrackSpace() const;
	int getNumSoundtracks() const;
	const TCHAR* getAudioUnitVictoryScript() const;
	const TCHAR* getAudioUnitDefeatScript() const;

	bool isNoGoodies() const;
	bool isNoAnimals() const;
	bool isNoBarbUnits() const;
	bool isNoBarbCities() const;
	bool isFirstSoundtrackFirst() const;

	int getSoundtracks(int i) const;
	int getCitySoundscapeScriptId(int i) const;

	bool read(CvXMLLoadUtility* pXML);
	// <advc.erai>
	static void allInfosRead();
	/*	NB: The "age" functions return ERA_NEVER instead of NO_ERA.
		Not MAX_INT b/c we don't want callers to get in trouble with overflow. */
	#define ERA_NEVER ((EraTypes)100)
	static inline EraTypes AI_getAgeOfExploration()
	{
		return m_eAIAgeOfExploration;
	}
	static inline EraTypes AI_getAgeOfPestilence()
	{
		return m_eAIAgeOfPestilence;
	}
	static inline EraTypes AI_getAgeOfPollution()
	{
		return m_eAIAgeOfPollution;
	}
	static inline EraTypes AI_getAgeOfFertility()
	{
		return m_eAIAgeOfFertility;
	}
	static inline EraTypes AI_getAgeOfGuns()
	{
		return m_eAIAgeOfGuns;
	}
	static inline EraTypes AI_getAtomicAge()
	{
		return m_eAIAtomicAge;
	}
	// Akin to normalizeEraFactor in Kek-Mod (CvGameCoreUtils)
	static inline scaled normalizeEraNum(int iEra)
	{
		/*	Important that iEra is on the scale of the available eras.
			E.g. when AI code written for the 7 BtS eras checks
			kGame.getCurrentEra() > 1
			then it's the left side of the inequation that needs to be
			normalized, not the right side. Safer to use kGame.AI_getCurrEraFactor
			in such a case. normalizeEraNum is really only for era differences. */
		FAssertEnumBounds((EraTypes)iEra);
		/*	[Kek-Mod doesn't count the BtS Future era here:
			intdiv::uround(eEra * 6, GC.getNumEraInfos() - 1)] */
		return scaled(iEra * 7, GC.getNumEraInfos());
	}
	// </advc.erai>

protected:
	// <advc.erai>
	static EraTypes m_eAIAgeOfExploration;
	static EraTypes m_eAIAgeOfPestilence;
	static EraTypes m_eAIAgeOfPollution;
	static EraTypes m_eAIAgeOfFertility;
	static EraTypes m_eAIAgeOfGuns;
	static EraTypes m_eAIAtomicAge;
	// </advc.erai>

	int m_iStartingUnitMultiplier;
	int m_iStartingDefenseUnits;
	int m_iStartingWorkerUnits;
	int m_iStartingExploreUnits;
	int m_iAdvancedStartPoints;
	int m_iStartingGold;
	int m_iFreePopulation;
	int m_iStartPercent;
	int m_iGrowthPercent;
	int m_iTrainPercent;
	int m_iConstructPercent;
	int m_iCreatePercent;
	int m_iResearchPercent;
	int m_iTechCostModifier; // BETTER_BTS_AI_MOD, Tech Diffusion, 08/21/09, jdog5000
	int m_iBuildPercent;
	int m_iImprovementPercent;
	int m_iGreatPeoplePercent;
	int m_iCulturePercent; // advc.126
	int m_iAnarchyPercent;
	int m_iEventChancePerTurn;
	int m_iSoundtrackSpace;
	int m_iNumSoundtracks;
	CvString m_szAudioUnitVictoryScript;
	CvString m_szAudioUnitDefeatScript;

	bool m_bNoGoodies;
	bool m_bNoAnimals;
	bool m_bNoBarbUnits;
	bool m_bNoBarbCities;
	bool m_bFirstSoundtrackFirst;

	int* m_paiSoundtracks;
	int* m_paiCitySoundscapeScriptIds;

	void addElements(std::vector<XMLElement*>& r) const; // advc.tag
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvGameSpeedInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvGameSpeedInfo : public CvInfoBase
{
public: // The const functions are exposed to Python // advc.inl: inlined some getters
	CvGameSpeedInfo();
	~CvGameSpeedInfo();

	inline int getGrowthPercent() const { return m_iGrowthPercent; }
	inline int getTrainPercent() const { return m_iTrainPercent; }
	inline int getConstructPercent() const { return m_iConstructPercent; }
	inline int getCreatePercent() const { return m_iCreatePercent; }
	inline int getResearchPercent() const { return m_iResearchPercent; }
	inline int getBuildPercent() const { return m_iBuildPercent; }
	inline int getImprovementPercent() const { return m_iImprovementPercent; }
	inline int getGreatPeoplePercent() const { return m_iGreatPeoplePercent; }
	inline int getAnarchyPercent() const { return m_iAnarchyPercent; }
	int getBarbPercent() const;
	int getFeatureProductionPercent() const;
	int getUnitDiscoverPercent() const;
	int getUnitHurryPercent() const;
	int getUnitTradePercent() const;
	int getUnitGreatWorkPercent() const;
	inline int getGoldenAgePercent() const { return m_iGoldenAgePercent; }
	int getHurryPercent() const;
	int getHurryConscriptAngerPercent() const;
	int getInflationOffset() const;
	int getInflationPercent() const;
	inline int getVictoryDelayPercent() const { return m_iVictoryDelayPercent; }
	int getNumTurnIncrements() const;

	GameTurnInfo& getGameTurnInfo(int iIndex) const;

	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iGrowthPercent;
	int m_iTrainPercent;
	int m_iConstructPercent;
	int m_iCreatePercent;
	int m_iResearchPercent;
	int m_iBuildPercent;
	int m_iImprovementPercent;
	int m_iGreatPeoplePercent;
	int m_iAnarchyPercent;
	int m_iBarbPercent;
	int m_iFeatureProductionPercent;
	int m_iUnitDiscoverPercent;
	int m_iUnitHurryPercent;
	int m_iUnitTradePercent;
	int m_iUnitGreatWorkPercent;
	int m_iGoldenAgePercent;
	int m_iHurryPercent;
	int m_iHurryConscriptAngerPercent;
	int m_iInflationOffset;
	int m_iInflationPercent;
	int m_iVictoryDelayPercent;
	int m_iNumTurnIncrements;

	CvString m_szGameSpeedName;
	GameTurnInfo* m_pGameTurnInfo;

private:
	void allocateGameTurnInfos(const int iSize); // advc.003: was public
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvTurnTimerInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvTurnTimerInfo : public CvInfoBase
{
public: // The const functions are exposed to Python

	CvTurnTimerInfo();

	int getBaseTime() const;
	int getCityBonus() const;
	int getUnitBonus() const;
	int getFirstTurnMultiplier() const;

	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iBaseTime;
	int m_iCityBonus;
	int m_iUnitBonus;
	int m_iFirstTurnMultiplier;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvVictoryInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvVictoryInfo : public CvInfoBase
{
public: // The const functions are exposed to Python
	CvVictoryInfo();

	int getPopulationPercentLead() const;
	int getLandPercent() const;
	int getMinLandPercent() const;
	int getReligionPercent() const;
	int getCityCulture() const;
	int getNumCultureCities() const;
	int getTotalCultureRatio() const;
	int getVictoryDelayTurns() const;

	bool isTargetScore() const;
	bool isEndScore() const;
	bool isConquest() const;
	bool isDiploVote() const;
	DllExport bool isPermanent() const;

	const char* getMovie() const;

	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iPopulationPercentLead;
	int m_iLandPercent;
	int m_iMinLandPercent;
	int m_iReligionPercent;
	int m_iCityCulture;
	int m_iNumCultureCities;
	int m_iTotalCultureRatio;
	int m_iVictoryDelayTurns;

	bool m_bTargetScore;
	bool m_bEndScore;
	bool m_bConquest;
	bool m_bDiploVote;
	bool m_bPermanent;

	CvString m_szMovie;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvHandicapInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvHandicapInfo : public CvInfoBase
{
public: // The const functions are exposed to Python except functions added by mods
	CvHandicapInfo();
	~CvHandicapInfo();

	int getFreeWinsVsBarbs() const;
	int getAnimalAttackProb() const;
	int getStartingLocationPercent() const;
	int getAdvancedStartPointsMod() const;
	int getStartingGold() const;
	int getFreeUnits() const;
	int getUnitCostPercent() const;
	// <advc.251>
	inline int getBuildTimePercent() const { return m_iBuildTimePercent; }
	inline int getBaseGrowthThresholdPercent() const { return m_iBaseGrowthThresholdPercent; }
	inline int getGPThresholdPercent() const { return m_iGPThresholdPercent; }
	inline int getCultureLevelPercent() const { return m_iCultureLevelPercent; }
	// </advc.251>
	inline int getResearchPercent() const { return m_iResearchPercent; } // advc.inl: Let's inline this one too
	// <advc.251>
	inline int getTrainPercent() const { return m_iTrainPercent; }
	inline int getConstructPercent() const { return m_iConstructPercent; }
	inline int getCreatePercent() const { return m_iCreatePercent; }
	// </advc.251>
	int getDistanceMaintenancePercent() const;
	int getNumCitiesMaintenancePercent() const;
	int getMaxNumCitiesMaintenance() const;
	int getColonyMaintenancePercent() const;
	int getMaxColonyMaintenance() const;
	int getCorporationMaintenancePercent() const;
	int getCivicUpkeepPercent() const;
	int getInflationPercent() const;
	int getHealthBonus() const;
	int getHappyBonus() const;
	int getAttitudeChange() const;
	int getNoTechTradeModifier() const;
	int getTechTradeKnownModifier() const;
	int getUnownedTilesPerGameAnimal() const;
	int getUnownedTilesPerBarbarianUnit() const;
	int getUnownedWaterTilesPerBarbarianUnit() const;
	int getUnownedTilesPerBarbarianCity() const;
	int getBarbarianCreationTurnsElapsed() const;
	int getBarbarianCityCreationTurnsElapsed() const;
	int getBarbarianCityCreationProb() const;
	int getAnimalCombatModifier() const;
	int getBarbarianCombatModifier() const;
	int getAIAnimalCombatModifier() const;
	int getAIBarbarianCombatModifier() const;

	int getStartingDefenseUnits() const;
	int getStartingWorkerUnits() const;
	int getStartingExploreUnits() const;
	int getAIStartingUnitMultiplier() const;
	int getAIStartingDefenseUnits() const;
	int getAIStartingWorkerUnits() const;
	int getAIStartingExploreUnits() const;
	int getBarbarianInitialDefenders() const;
	int getAIDeclareWarProb() const;
	int getAIWorkRateModifier() const;
	int getAIGrowthPercent() const;
	// <advc.251>
	inline int getAIGPThresholdPercent() const { return m_iAIGPThresholdPercent; }
	inline int getAIResearchPercent() const { return m_iAIResearchPercent; }
	// </advc.251>
	int getAITrainPercent() const;
	int getAIWorldTrainPercent() const;
	int getAIConstructPercent() const;
	int getAIWorldConstructPercent() const;
	int getAICreatePercent() const;
	int getAIWorldCreatePercent() const;
	int getAICivicUpkeepPercent() const;
	int getAIUnitCostPercent() const;
	int getAIUnitSupplyPercent() const;
	int getAIUnitUpgradePercent() const;
	int getAIInflationPercent() const;
	int getAIWarWearinessPercent() const;
	//int getAIPerEraModifier() const;
	inline int getAIHandicapIncrementTurns() const { return m_iAIHandicapIncrementTurns; }
	int getAIAttitudeChangePercent() const; // advc.148
	int getAIAdvancedStartPercent() const;
	int getNumGoodies() const;
	int getDifficulty() const; // advc.250a; exposed to Python

	int getGoodies(int i) const;
	// advc.003t: Return type was int for these two
	bool isFreeTechs(int i) const;
	bool isAIFreeTechs(int i) const;
	#if ENABLE_XML_FILE_CACHE
	void read(FDataStreamBase* stream);
	void write(FDataStreamBase* stream);
	#endif
	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iFreeWinsVsBarbs;
	int m_iAnimalAttackProb;
	int m_iStartingLocationPercent;
	int m_iAdvancedStartPointsMod;
	int m_iStartingGold;
	int m_iFreeUnits;
	int m_iUnitCostPercent;
	// <advc.251>
	int m_iBuildTimePercent;
	int m_iBaseGrowthThresholdPercent;
	int m_iGPThresholdPercent;
	int m_iCultureLevelPercent;
	// </advc.251>
	int m_iResearchPercent;
	// <advc.251>
	int m_iTrainPercent;
	int m_iConstructPercent;
	int m_iCreatePercent;
	// </avdc.251>
	int m_iDistanceMaintenancePercent;
	int m_iNumCitiesMaintenancePercent;
	int m_iMaxNumCitiesMaintenance;
	int m_iColonyMaintenancePercent;
	int m_iMaxColonyMaintenance;
	int m_iCorporationMaintenancePercent;
	int m_iCivicUpkeepPercent;
	int m_iInflationPercent;
	int m_iHealthBonus;
	int m_iHappyBonus;
	int m_iAttitudeChange;
	int m_iNoTechTradeModifier;
	int m_iTechTradeKnownModifier;
	int m_iUnownedTilesPerGameAnimal;
	int m_iUnownedTilesPerBarbarianUnit;
	int m_iUnownedWaterTilesPerBarbarianUnit;
	int m_iUnownedTilesPerBarbarianCity;
	int m_iBarbarianCreationTurnsElapsed;
	int m_iBarbarianCityCreationTurnsElapsed;
	int m_iBarbarianCityCreationProb;
	int m_iAnimalCombatModifier;
	int m_iBarbarianCombatModifier;
	int m_iAIAnimalCombatModifier;
	int m_iAIBarbarianCombatModifier;

	int m_iStartingDefenseUnits;
	int m_iStartingWorkerUnits;
	int m_iStartingExploreUnits;
	int m_iAIStartingUnitMultiplier;
	int m_iAIStartingDefenseUnits;
	int m_iAIStartingWorkerUnits;
	int m_iAIStartingExploreUnits;
	int m_iBarbarianInitialDefenders;
	int m_iAIDeclareWarProb;
	int m_iAIWorkRateModifier;
	int m_iAIGrowthPercent;
	// <advc.251>
	int m_iAIGPThresholdPercent;
	int m_iAIResearchPercent;
	// </advc.251>
	int m_iAITrainPercent;
	int m_iAIWorldTrainPercent;
	int m_iAIConstructPercent;
	int m_iAIWorldConstructPercent;
	int m_iAICreatePercent;
	int m_iAIWorldCreatePercent;
	int m_iAICivicUpkeepPercent;
	int m_iAIUnitCostPercent;
	int m_iAIUnitSupplyPercent;
	int m_iAIUnitUpgradePercent;
	int m_iAIInflationPercent;
	int m_iAIWarWearinessPercent;
	//int m_iAIPerEraModifier;
	int m_iAIHandicapIncrementTurns; // advc.251
	int m_iAIAdvancedStartPercent;
	int m_iAIAttitudeChangePercent; // advc.148
	int m_iNumGoodies;
	int m_iDifficulty; // advc.250a

	CvString m_szHandicapName;

	int* m_piGoodies;

	bool* m_pbFreeTechs;
	bool* m_pbAIFreeTechs;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvWorldInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvWorldInfo : public CvInfoBase
{
public: // All the const functions are exposed to Python
	CvWorldInfo();

	DllExport int getDefaultPlayers() const { return m_iDefaultPlayers; } // advc.inl
	int getUnitNameModifier() const;
	int getTargetNumCities() const { return m_iTargetNumCities; } // advc.inl
	int getNumFreeBuildingBonuses() const;
	int getBuildingClassPrereqModifier() const { return m_iBuildingClassPrereqModifier; } // advc.inl
	int getMaxConscriptModifier() const;
	int getWarWearinessModifier() const;
	int getGridWidth() const;
	int getGridHeight() const;
	int getTerrainGrainChange() const;
	int getFeatureGrainChange() const;
	int getResearchPercent() const { return m_iResearchPercent; } // advc.inl
	int getTradeProfitPercent() const;
	int getDistanceMaintenancePercent() const;
	int getNumCitiesMaintenancePercent() const;
	int getColonyMaintenancePercent() const;
	int getCorporationMaintenancePercent() const;
	int getNumCitiesAnarchyPercent() const;
	int getAdvancedStartPointsMod() const;

	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iDefaultPlayers;
	int m_iUnitNameModifier;
	int m_iTargetNumCities;
	int m_iNumFreeBuildingBonuses;
	int m_iBuildingClassPrereqModifier;
	int m_iMaxConscriptModifier;
	int m_iWarWearinessModifier;
	int m_iGridWidth;
	int m_iGridHeight;
	int m_iTerrainGrainChange;
	int m_iFeatureGrainChange;
	int m_iResearchPercent;
	int m_iTradeProfitPercent;
	int m_iDistanceMaintenancePercent;
	int m_iNumCitiesMaintenancePercent;
	int m_iColonyMaintenancePercent;
	int m_iCorporationMaintenancePercent;
	int m_iNumCitiesAnarchyPercent;
	int m_iAdvancedStartPointsMod;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvClimateInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvClimateInfo : public CvInfoBase
{
public: // All the const functions are exposed to Python
	CvClimateInfo();

	int getDesertPercentChange() const;
	int getJungleLatitude() const;
	int getHillRange() const;
	int getPeakPercent() const;

	float getSnowLatitudeChange() const;
	float getTundraLatitudeChange() const;
	float getGrassLatitudeChange() const;
	float getDesertBottomLatitudeChange() const;
	float getDesertTopLatitudeChange() const;
	float getIceLatitude() const;
	float getRandIceLatitude() const;

	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iDesertPercentChange;
	int m_iJungleLatitude;
	int m_iHillRange;
	int m_iPeakPercent;

	float m_fSnowLatitudeChange;
	float m_fTundraLatitudeChange;
	float m_fGrassLatitudeChange;
	float m_fDesertBottomLatitudeChange;
	float m_fDesertTopLatitudeChange;
	float m_fIceLatitude;
	float m_fRandIceLatitude;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvSeaLevelInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvSeaLevelInfo : public CvInfoBase
{
public:
	CvSeaLevelInfo();

	int getSeaLevelChange() const; // Exposed to Python
	inline int getResearchPercent() const { return m_iResearchPercent; } // advc.910
	bool read(CvXMLLoadUtility* pXML);
	virtual CvWString getDescriptionInternal() const; // advc.137

protected:
	int m_iSeaLevelChange;
	int m_iResearchPercent; // advc.910
};

#endif
