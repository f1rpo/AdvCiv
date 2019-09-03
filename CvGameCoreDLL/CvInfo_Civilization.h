#pragma once

#ifndef CV_INFO_CIVILIZATION_H
#define CV_INFO_CIVILIZATION_H

/*  advc.003x: Cut from CvInfos.h.
	CvCivilizationInfo, CvLeaderHeadInfo, CvTraitInfo,
	CvDiplomacyInfo, CvDiplomacyResponse
	Want to precompile these. CvLeaderHeadInfo is frequently needed (by most of
	the AI code - but not only), is large and tags are added rarely. CvCivilization
	still (despite change advc.003w) plays a role in mapping building/ unit classes
	to types. */

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvCivilizationInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvCivilizationInfo : public CvInfoBase
{
public: // advc: All the const functions are exposed to Python
	CvCivilizationInfo();
	~CvCivilizationInfo();
	void reset(); // override

	int getDerivativeCiv() const;
	DllExport int getDefaultPlayerColor() const;
	int getArtStyleType() const;
	int getUnitArtStyleType() const;
	int getNumCityNames() const;
	int getNumLeaders() const;
	int getSelectionSoundScriptId() const;
	int getActionSoundScriptId() const;

	DllExport bool isAIPlayable() const;
	DllExport bool isPlayable() const;

	std::wstring pyGetShortDescription(uint uiForm) { return getShortDescription(uiForm); }
	DllExport const wchar* getShortDescription(uint uiForm = 0);
	const wchar* getShortDescriptionKey() const;
	std::wstring pyGetShortDescriptionKey() { return getShortDescriptionKey(); }

	std::wstring pyGetAdjective(uint uiForm) { return getAdjective(uiForm); }
	DllExport const wchar* getAdjective(uint uiForm = 0);
	const wchar* getAdjectiveKey() const;
	std::wstring pyGetAdjectiveKey() { return getAdjectiveKey(); }

	DllExport const TCHAR* getFlagTexture() const;
	const TCHAR* getArtDefineTag() const;
	void setArtDefineTag(const TCHAR* szVal);

	// Array access:

	int getCivilizationBuildings(int i) const;
	int getCivilizationUnits(int i) const;
	int getCivilizationFreeUnitsClass(int i) const;
	int getCivilizationInitialCivics(int i) const;

	DllExport bool isLeaders(int i) const;
	bool isCivilizationFreeBuildingClass(int i) const;
	bool isCivilizationFreeTechs(int i) const;
	bool isCivilizationDisableTechs(int i) const;

	std::string getCityNames(int i) const;

	const CvArtInfoCivilization* getArtInfo() const; // (not exposed to Python)
	const TCHAR* getButton() const;

	bool read(CvXMLLoadUtility* pXML);
	bool readPass2(CvXMLLoadUtility* pXML);
	#if SERIALIZE_CVINFOS
	void read(FDataStreamBase* stream);
	void write(FDataStreamBase* stream);
	#endif

protected:
	int m_iDefaultPlayerColor;
	int m_iArtStyleType;
	int m_iUnitArtStyleType; // FlavorUnits by Impaler[WrG]
	int m_iNumCityNames;
	int m_iNumLeaders; // the number of leaders the Civ has, this is needed so that random leaders can be generated easily
	int m_iSelectionSoundScriptId;
	int m_iActionSoundScriptId;
	int m_iDerivativeCiv;

	bool m_bAIPlayable;
	bool m_bPlayable;

	CvString m_szArtDefineTag;
	CvWString m_szShortDescriptionKey;
	CvWString m_szAdjectiveKey;

	int* m_piCivilizationBuildings;
	int* m_piCivilizationUnits;
	int* m_piCivilizationFreeUnitsClass;
	int* m_piCivilizationInitialCivics;

	bool* m_pbLeaders;
	bool* m_pbCivilizationFreeBuildingClass;
	bool* m_pbCivilizationFreeTechs;
	bool* m_pbCivilizationDisableTechs;

	CvString* m_paszCityNames;

	mutable std::vector<CvWString> m_aszShortDescription;
	mutable std::vector<CvWString> m_aszAdjective;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvLeaderHeadInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvLeaderHeadInfo : public CvInfoBase
{
friend class WarAndPeaceAI; // advc.104x (for applyPersonalityWeight)
public: // advc: All the const functions are exposed to Python except those added by mods
	CvLeaderHeadInfo();
	~CvLeaderHeadInfo();

	int getWonderConstructRand() const;
	int getBaseAttitude() const;
	int getBasePeaceWeight() const;
	int getPeaceWeightRand() const;
	int getWarmongerRespect() const;
	int getEspionageWeight() const;
	int getRefuseToTalkWarThreshold() const;
	int getNoTechTradeThreshold() const;
	int getTechTradeKnownPercent() const;
	int getMaxGoldTradePercent() const;
	int getMaxGoldPerTurnTradePercent() const;
	// BETTER_BTS_AI_MOD, Victory Strategy AI, 03/21/10, jdog5000: START
	int getCultureVictoryWeight() const;
	int getSpaceVictoryWeight() const;
	int getConquestVictoryWeight() const;
	int getDominationVictoryWeight() const;
	int getDiplomacyVictoryWeight() const;
	// BETTER_BTS_AI_MOD: END
	int getMaxWarRand() const;
	int getMaxWarNearbyPowerRatio() const;
	int getMaxWarDistantPowerRatio() const;
	int getMaxWarMinAdjacentLandPercent() const;
	int getLimitedWarRand() const;
	int getLimitedWarPowerRatio() const;
	int getDogpileWarRand() const;
	int getMakePeaceRand() const;
	int getDeclareWarTradeRand() const;
	int getDemandRebukedSneakProb() const;
	int getDemandRebukedWarProb() const;
	int getRazeCityProb() const;
	inline int getBuildUnitProb() const { return m_iBuildUnitProb; } // advc.003f
	int getBaseAttackOddsChange() const;
	int getAttackOddsChangeRand() const;
	int getWorseRankDifferenceAttitudeChange() const;
	int getBetterRankDifferenceAttitudeChange() const;
	int getCloseBordersAttitudeChange() const;
	int getLostWarAttitudeChange() const;
	int getAtWarAttitudeDivisor() const;
	int getAtWarAttitudeChangeLimit() const;
	int getAtPeaceAttitudeDivisor() const;
	int getAtPeaceAttitudeChangeLimit() const;
	int getSameReligionAttitudeChange() const;
	int getSameReligionAttitudeDivisor() const;
	int getSameReligionAttitudeChangeLimit() const;
	int getDifferentReligionAttitudeChange() const;
	int getDifferentReligionAttitudeDivisor() const;
	int getDifferentReligionAttitudeChangeLimit() const;
	int getBonusTradeAttitudeDivisor() const;
	int getBonusTradeAttitudeChangeLimit() const;
	int getOpenBordersAttitudeDivisor() const;
	int getOpenBordersAttitudeChangeLimit() const;
	int getDefensivePactAttitudeDivisor() const;
	int getDefensivePactAttitudeChangeLimit() const;
	int getShareWarAttitudeChange() const;
	int getShareWarAttitudeDivisor() const;
	int getShareWarAttitudeChangeLimit() const;
	int getFavoriteCivicAttitudeChange() const;
	int getFavoriteCivicAttitudeDivisor() const;
	int getFavoriteCivicAttitudeChangeLimit() const;
	int getDemandTributeAttitudeThreshold() const;
	int getNoGiveHelpAttitudeThreshold() const;
	int getTechRefuseAttitudeThreshold() const;
	int getStrategicBonusRefuseAttitudeThreshold() const;
	int getHappinessBonusRefuseAttitudeThreshold() const;
	int getHealthBonusRefuseAttitudeThreshold() const;
	int getMapRefuseAttitudeThreshold() const;
	int getDeclareWarRefuseAttitudeThreshold() const;
	int getDeclareWarThemRefuseAttitudeThreshold() const;
	int getStopTradingRefuseAttitudeThreshold() const;
	int getStopTradingThemRefuseAttitudeThreshold() const;
	int getAdoptCivicRefuseAttitudeThreshold() const;
	int getConvertReligionRefuseAttitudeThreshold() const;
	int getOpenBordersRefuseAttitudeThreshold() const;
	int getDefensivePactRefuseAttitudeThreshold() const;
	int getPermanentAllianceRefuseAttitudeThreshold() const;
	int getVassalRefuseAttitudeThreshold() const;
	int getVassalPowerModifier() const;
	int getFavoriteCivic() const;
	int getFavoriteReligion() const;
	int getFreedomAppreciation() const;

	const TCHAR* getArtDefineTag() const;
	void setArtDefineTag(const TCHAR* szVal);

	// Array access:

	bool hasTrait(int i) const;

	int getFlavorValue(int i) const;
	int getContactRand(int i) const;
	int getContactDelay(int i) const;
	int getMemoryDecayRand(int i) const;
	int getMemoryAttitudePercent(int i) const;
	int getNoWarAttitudeProb(int i) const;
	int getUnitAIWeightModifier(int i) const;
	int getImprovementWeightModifier(int i) const;
	int getDiploPeaceIntroMusicScriptIds(int i) const;
	int getDiploPeaceMusicScriptIds(int i) const;
	int getDiploWarIntroMusicScriptIds(int i) const;
	int getDiploWarMusicScriptIds(int i) const;

	// (not exposed to Python)
	DllExport const CvArtInfoLeaderhead* getArtInfo() const;
	const TCHAR* getLeaderHead() const;
	const TCHAR* getButton() const;
	#if SERIALIZE_CVINFOS
	void read(FDataStreamBase* stream);
	void write(FDataStreamBase* stream);
	#endif
	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iWonderConstructRand;
	int m_iBaseAttitude;
	int m_iBasePeaceWeight;
	int m_iPeaceWeightRand;
	int m_iWarmongerRespect;
	int m_iEspionageWeight;
	int m_iRefuseToTalkWarThreshold;
	int m_iNoTechTradeThreshold;
	int m_iTechTradeKnownPercent;
	int m_iMaxGoldTradePercent;
	int m_iMaxGoldPerTurnTradePercent;
	// BETTER_BTS_AI_MOD; Victory Strategy AI, 03/21/10, jdog5000: START
	int m_iCultureVictoryWeight;
	int m_iSpaceVictoryWeight;
	int m_iConquestVictoryWeight;
	int m_iDominationVictoryWeight;
	int m_iDiplomacyVictoryWeight;
	// BETTER_BTS_AI_MOD: END
	int m_iMaxWarRand;
	int m_iMaxWarNearbyPowerRatio;
	int m_iMaxWarDistantPowerRatio;
	int m_iMaxWarMinAdjacentLandPercent;
	int m_iLimitedWarRand;
	int m_iLimitedWarPowerRatio;
	int m_iDogpileWarRand;
	int m_iMakePeaceRand;
	int m_iDeclareWarTradeRand;
	int m_iDemandRebukedSneakProb;
	int m_iDemandRebukedWarProb;
	int m_iRazeCityProb;
	int m_iBuildUnitProb;
	int m_iBaseAttackOddsChange;
	int m_iAttackOddsChangeRand;
	int m_iWorseRankDifferenceAttitudeChange;
	int m_iBetterRankDifferenceAttitudeChange;
	int m_iCloseBordersAttitudeChange;
	int m_iLostWarAttitudeChange;
	int m_iAtWarAttitudeDivisor;
	int m_iAtWarAttitudeChangeLimit;
	int m_iAtPeaceAttitudeDivisor;
	int m_iAtPeaceAttitudeChangeLimit;
	int m_iSameReligionAttitudeChange;
	int m_iSameReligionAttitudeDivisor;
	int m_iSameReligionAttitudeChangeLimit;
	int m_iDifferentReligionAttitudeChange;
	int m_iDifferentReligionAttitudeDivisor;
	int m_iDifferentReligionAttitudeChangeLimit;
	int m_iBonusTradeAttitudeDivisor;
	int m_iBonusTradeAttitudeChangeLimit;
	int m_iOpenBordersAttitudeDivisor;
	int m_iOpenBordersAttitudeChangeLimit;
	int m_iDefensivePactAttitudeDivisor;
	int m_iDefensivePactAttitudeChangeLimit;
	int m_iShareWarAttitudeChange;
	int m_iShareWarAttitudeDivisor;
	int m_iShareWarAttitudeChangeLimit;
	int m_iFavoriteCivicAttitudeChange;
	int m_iFavoriteCivicAttitudeDivisor;
	int m_iFavoriteCivicAttitudeChangeLimit;
	int m_iDemandTributeAttitudeThreshold;
	int m_iNoGiveHelpAttitudeThreshold;
	int m_iTechRefuseAttitudeThreshold;
	int m_iStrategicBonusRefuseAttitudeThreshold;
	int m_iHappinessBonusRefuseAttitudeThreshold;
	int m_iHealthBonusRefuseAttitudeThreshold;
	int m_iMapRefuseAttitudeThreshold;
	int m_iDeclareWarRefuseAttitudeThreshold;
	int m_iDeclareWarThemRefuseAttitudeThreshold;
	int m_iStopTradingRefuseAttitudeThreshold;
	int m_iStopTradingThemRefuseAttitudeThreshold;
	int m_iAdoptCivicRefuseAttitudeThreshold;
	int m_iConvertReligionRefuseAttitudeThreshold;
	int m_iOpenBordersRefuseAttitudeThreshold;
	int m_iDefensivePactRefuseAttitudeThreshold;
	int m_iPermanentAllianceRefuseAttitudeThreshold;
	int m_iVassalRefuseAttitudeThreshold;
	int m_iVassalPowerModifier;
	int m_iFreedomAppreciation;
	int m_iFavoriteCivic;
	int m_iFavoriteReligion;

	CvString m_szArtDefineTag;

	bool* m_pbTraits;

	int* m_piFlavorValue;
	int* m_piContactRand;
	int* m_piContactDelay;
	int* m_piMemoryDecayRand;
	int* m_piMemoryAttitudePercent;
	int* m_piNoWarAttitudeProb;
	int* m_piUnitAIWeightModifier;
	int* m_piImprovementWeightModifier;
	int* m_piDiploPeaceIntroMusicScriptIds;
	int* m_piDiploPeaceMusicScriptIds;
	int* m_piDiploWarIntroMusicScriptIds;
	int* m_piDiploWarMusicScriptIds;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvTraitInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvTraitInfo : public CvInfoBase
{
public: // advc: All the const functions are exposed to Python except those added by mods
	CvTraitInfo();
	~CvTraitInfo();

	int getHealth() const;
	int getHappiness() const;
	int getMaxAnarchy() const;
	int getUpkeepModifier() const;
	int getLevelExperienceModifier() const;
	int getGreatPeopleRateModifier() const;
	int getGreatGeneralRateModifier() const;
	int getDomesticGreatGeneralRateModifier() const;
	int getMaxGlobalBuildingProductionModifier() const;
	int getMaxTeamBuildingProductionModifier() const;
	int getMaxPlayerBuildingProductionModifier() const;

	const TCHAR* getShortDescription() const;
	void setShortDescription(const TCHAR* szVal);

	// Array access:

	int getExtraYieldThreshold(int i) const;
	int getTradeYieldModifier(int i) const;
	int getCommerceChange(int i) const;
	int getCommerceModifier(int i) const;

	bool isFreePromotion(int i) const; // advc.003t: Return type was int
	inline bool isAnyFreePromotion() const { return (m_pabFreePromotion != NULL); } // advc.003t
	bool isFreePromotionUnitCombat(int i) const; // advc.003t: Return type was int

	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iHealth;
	int m_iHappiness;
	int m_iMaxAnarchy;
	int m_iUpkeepModifier;
	int m_iLevelExperienceModifier;
	int m_iGreatPeopleRateModifier;
	int m_iGreatGeneralRateModifier;
	int m_iDomesticGreatGeneralRateModifier;
	int m_iMaxGlobalBuildingProductionModifier;
	int m_iMaxTeamBuildingProductionModifier;
	int m_iMaxPlayerBuildingProductionModifier;

	CvString m_szShortDescription;

	int* m_paiExtraYieldThreshold;
	int* m_paiTradeYieldModifier;
	int* m_paiCommerceChange;
	int* m_paiCommerceModifier;

	bool* m_pabFreePromotion;
	bool* m_pabFreePromotionUnitCombat;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvDiplomacyResponse (helper for DiplomacyInfo)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvDiplomacyResponse
{
public: // advc: All the const functions are exposed to Python (some const qualifiers added by me)
	CvDiplomacyResponse();
	~CvDiplomacyResponse(); // advc: was virtual

	int getNumDiplomacyText() const;
	// advc.003j: Disabled the unused setters and bool* getters
	//void setNumDiplomacyText(int i);

	bool getCivilizationTypes(int i) const;
	//bool* getCivilizationTypes() const;
	//void setCivilizationTypes(int i, bool bVal);

	bool getLeaderHeadTypes(int i) const;
	//bool* getLeaderHeadTypes() const;
	//void setLeaderHeadTypes(int i, bool bVal);

	bool getAttitudeTypes(int i) const;
	//bool* getAttitudeTypes() const;
	//void setAttitudeTypes(int i, bool bVal);

	bool getDiplomacyPowerTypes(int i) const;
	//bool* getDiplomacyPowerTypes() const;
	//void setDiplomacyPowerTypes(int i, bool bVal);

	const TCHAR* getDiplomacyText(int i) const;
	const CvString* getDiplomacyText() const;
	void setDiplomacyText(int i, CvString szText);
	// advc.003i: I don't think these were actually included in the XML cache
	/*#if SERIALIZE_CVINFOS
	void read(FDataStreamBase* stream);
	void write(FDataStreamBase* stream);*/
	//#endif
	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iNumDiplomacyText;
	bool* m_pbCivilizationTypes;
	bool* m_pbLeaderHeadTypes;
	bool* m_pbAttitudeTypes;
	bool* m_pbDiplomacyPowerTypes;
	CvString* m_paszDiplomacyText;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvDiplomacyInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvDiplomacyInfo : public CvInfoBase
{
friend class CvXMLLoadUtility;
public: // advc: All the const functions are exposed to Python
	~CvDiplomacyInfo();
	//void uninit(); // advc: Merged into destructor

	const CvDiplomacyResponse& getResponse(int iNum) const;
	int getNumResponses() const;

	bool getCivilizationTypes(int i, int j) const;
	bool getLeaderHeadTypes(int i, int j) const;
	bool getAttitudeTypes(int i, int j) const;
	bool getDiplomacyPowerTypes(int i, int j) const;

	int getNumDiplomacyText(int i) const;

	const TCHAR* getDiplomacyText(int i, int j) const;
	// advc.003i: I don't think these were actually included in the XML cache
	/*#if SERIALIZE_CVINFOS
	void read(FDataStreamBase* stream);
	void write(FDataStreamBase* stream);*/
	//#endif
	bool read(CvXMLLoadUtility* pXML);

protected:
	std::vector<CvDiplomacyResponse*> m_pResponses;
};

#endif
