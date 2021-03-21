#pragma once

#ifndef CIV4_GLOBALS_H
#define CIV4_GLOBALS_H
// <advc> Disable warnings about unknown pragma (MSVC03 doesn't know pragma region)
#pragma warning(push)
#pragma warning(disable: 68) // </advc>

/*	'global' vars for Civ IV.  singleton class.
	All globals and global types should be contained in this class. */

#pragma region ForwardDeclarations
class FProfiler;
class CvDLLUtilityIFaceBase;
class CvPythonCaller; // advc.003y
class CvDLLLogger;
class CvRandom;
class CvGame; // advc.003u
class CvGameAI;
class CvAgents; // advc.agent
class CMessageControl;
class CvDropMgr;
class CMessageQueue;
class CvSetupData;
class CvInitCore;
class CvMessageCodeTranslator;
class CvPortal;
class CvStatsReporter;
class CvDLLInterfaceIFaceBase;
class CvPlayerAI;
class CvDiplomacyScreen;
class CMPDiplomacyScreen;
class FMPIManager;
class FAStar;
class CvInterface;
class CvArtFileMgr;
class FVariableSystem;
class CvMap;
class CvPlayerAI;
class CvTeamAI;
class CvWorldInfo;
// <advc.enum>
#define FORWARD_DECLARE_INFO_CLASS(Name, Dummy) class Cv##Name##Info;
DO_FOR_EACH_INFO_TYPE(FORWARD_DECLARE_INFO_CLASS) // </advc.enum>
#pragma endregion ForwardDeclarations

class CvGlobals
{
	friend class CvXMLLoadUtility;
	/*  advc: Removed the std::vector<Cv...Info*>& getters for each info type.
		Comments in CvGlobals.cpp had said that those getters were
		"For Moose - XML Load Util and CvInfos."
		The only calls came from CvXMLLoadUtility (mostly the three
		CvXMLLoadUtility::Load...Infos functions). CvXMLLoadUtility is a friend
		class anyway; not a good reason to fully expose all the info vectors.
		Some more comments deleted from CvGlobals.cpp:
		getWaterPlaneInfo: "For Moose - CvDecal and CvWater"
		getUnitFormationInfo: "For Moose - CvUnitEntity"
		getBuildingInfo, getTechInfo "For Moose - XML Load Util, CvInfos, CvCacheObject"
		None of those were called outside of CvXMLLoadUtility.
		getHandicapInfo, getGameSpeed, getTurnTimer: "Do NOT export outside of the DLL"
		Strange, but no reason not to remove those getter. */
public:

	// singleton accessor
	DllExport __forceinline static CvGlobals& getInstance();
	__forceinline static CvGlobals const& getConstInstance();

	CvGlobals();

	DllExport void init();
	DllExport void uninit();
	void clearTypesMap();

	DllExport CvDiplomacyScreen* getDiplomacyScreen();
	DllExport CMPDiplomacyScreen* getMPDiplomacyScreen();

	DllExport FMPIManager*& getFMPMgrPtr();
	DllExport CvPortal& getPortal();
	DllExport CvSetupData& getSetupData();
	DllExport CvInitCore& getInitCore()  // <advc> const replacement
	{ CvGlobals const& kThis = *this; return kThis.getInitCore(); }
	inline CvInitCore& getInitCore() const { return *m_initCore; } // </advc>
	DllExport CvInitCore& getLoadedInitCore();
	DllExport CvInitCore& getIniInitCore();
	DllExport CvMessageCodeTranslator& getMessageCodes();
	DllExport CvStatsReporter& getStatsReporter();
	CvStatsReporter* getStatsReporterPtr();
	DllExport CvInterface& getInterface();
	DllExport CvInterface* getInterfacePtr();
	DllExport int getMaxCivPlayers() const;
	#ifdef _USRDLL // inlined for perf reasons, do not use outside of dll
	// advc.inl: forceinline just to be sure
	__forceinline CvMap& getMap() const { return *m_map; } // was getMapINLINE
	__forceinline CvGame& getGame() const // was getGameINLINE; advc.003u: return type was CvGameAI&
	{	// Can't be helped; this function has to be inlined, and I won't include CvGameAI.h here.
		return *reinterpret_cast<CvGame*>(m_game);
	} 
	__forceinline CvGameAI& AI_getGame() const { return *m_game; } // advc.003u
	CvAgents& getAgents() const { return *m_agents; } // advc.agents
	#endif
	CvMap& getMapExternal(); // advc.inl: Exported through .def file
	CvGameAI& getGameExternal(); // advc.inl: Exported through .def file
	DllExport CvGameAI* getGamePointer();
	// <advc.003y>
	inline CvPythonCaller const* getPythonCaller() const
	{
		return m_pPythonCaller;
	} // </advc.003y>
	DllExport inline CvRandom& getASyncRand() { return *m_asyncRand; } // advc.inl
	CvRandom& getASyncRand() const { return *m_asyncRand; } // advc
	DllExport CMessageQueue& getMessageQueue();
	DllExport CMessageQueue& getHotMessageQueue();
	DllExport CMessageControl& getMessageControl();
	DllExport CvDropMgr& getDropMgr();
	/*  advc: inlined and constified. The caller is certainly going to change
		the returned FAStar objects, but CvGlobals doesn't own those objects, so
		it shouldn't be our concern. */
	DllExport FAStar& getPathFinder() { CvGlobals const& kThis = *this; return kThis.getPathFinder(); }
	inline FAStar& getPathFinder() const { return *m_pathFinder; }
	DllExport FAStar& getInterfacePathFinder() { CvGlobals const& kThis = *this; return kThis.getInterfacePathFinder(); }
	inline FAStar& getInterfacePathFinder() const { return *m_interfacePathFinder; }
	DllExport FAStar& getStepFinder() { CvGlobals const& kThis = *this; return kThis.getStepFinder(); }
	inline FAStar& getStepFinder() const { return *m_stepFinder; }
	DllExport FAStar& getRouteFinder() { CvGlobals const& kThis = *this; return kThis.getRouteFinder(); }
	inline FAStar& getRouteFinder() const { return *m_routeFinder; }
	DllExport FAStar& getBorderFinder() { CvGlobals const& kThis = *this; return kThis.getBorderFinder(); }
	inline FAStar& getBorderFinder() const { return *m_borderFinder; }
	DllExport FAStar& getAreaFinder() { CvGlobals const& kThis = *this; return kThis.getAreaFinder(); }
	inline FAStar& getAreaFinder() const { return *m_areaFinder; }
	DllExport FAStar& getPlotGroupFinder() { CvGlobals const& kThis = *this; return kThis.getPlotGroupFinder(); }
	inline FAStar& getPlotGroupFinder() const { return *m_plotGroupFinder; }
	//NiPoint3& getPt3Origin(); // advc.003j: unused
	//NiPoint3& getPt3CameraDir(); // advc.003j: unused
	DllExport bool& getLogging() { return m_bLogging; }
	DllExport bool& getRandLogging() { return m_bRandLogging; }
	DllExport bool& getSynchLogging() { return m_bSynchLogging; }
	DllExport bool& overwriteLogs() { return m_bOverwriteLogs; }
	// <advc> const inline versions of the above
	// The first two are exposed to Python for kekm.27
	inline bool isLogging() const { return m_bLogging; }
	inline bool isRandLogging() const { return m_bRandLogging; }
	inline bool isSynchLogging() const { return m_bSynchLogging; }
	inline bool isOverwriteLogs() const { return m_bOverwriteLogs; }
	// <advc>
	inline CvDLLLogger& getLogger() const
	{
		return *m_pLogger;
	} // </advc>

	// advc: Inlined and constified
	DllExport int* getPlotDirectionX() { return m_aiPlotDirectionX; }
	inline int const* getPlotDirectionX() const { return m_aiPlotDirectionX; }
	DllExport int* getPlotDirectionY() { return m_aiPlotDirectionY; }
	inline int const* getPlotDirectionY() const { return m_aiPlotDirectionY; }
	DllExport int* getPlotCardinalDirectionX() { return m_aiPlotCardinalDirectionX; };
	inline int const* getPlotCardinalDirectionX() const { return m_aiPlotCardinalDirectionX; };
	DllExport int* getPlotCardinalDirectionY() { return m_aiPlotCardinalDirectionY; };
	inline int const* getPlotCardinalDirectionY() const { return m_aiPlotCardinalDirectionY; };
	DllExport DirectionTypes getXYDirection(int i, int j) { CvGlobals const& kThis = *this; return kThis.getXYDirection(i,j); }
	inline DirectionTypes getXYDirection(int i, int j) const
	{
		FAssertBounds(0, DIRECTION_DIAMETER, i);
		FAssertBounds(0, DIRECTION_DIAMETER, j);
		return m_aaeXYDirection[i][j];
	}
	inline CityPlotTypes getXYCityPlot(int i, int j) const // advc.enum: return type was int
	{
		FAssertBounds(0, CITY_PLOTS_DIAMETER, i);
		FAssertBounds(0, CITY_PLOTS_DIAMETER, j);
		return m_aaeXYCityPlot[i][j];
	}
	inline int const* getCityPlotX() const { return m_aiCityPlotX; }
	inline int const* getCityPlotY() const { return m_aiCityPlotY; }
	inline int const* CvGlobals::getCityPlotPriority() const { return m_aiCityPlotPriority; }
	inline DirectionTypes const* getTurnLeftDirection() const { return m_aeTurnLeftDirection; }
	inline DirectionTypes getTurnLeftDirection(int i) const
	{
		FAssertBounds(0, NUM_DIRECTION_TYPES, i);
		return m_aeTurnLeftDirection[i];
	}
	inline DirectionTypes const* getTurnRightDirection() const { return m_aeTurnRightDirection; }
	inline DirectionTypes getTurnRightDirection(int i) const
	{
		FAssertBounds(0, NUM_DIRECTION_TYPES, i);
		return m_aeTurnRightDirection[i];
	}

	//
	// Global Infos
	// All info type strings are upper case and are kept in this hash map for fast lookup
	// returns the infos index, use this when searching for an info type string
	DllExport int getInfoTypeForString(const char* szType, bool bHideAssert = false) const
	// <advc.006> Need another param
	{ return getInfoTypeForString(szType, bHideAssert, false); }
	int getInfoTypeForString(const char* szType, bool bHideAssert, bool bFromPython) const;
	// </advc.006>  // <advc>
	inline ColorTypes getColorType(const char* szType) const
	{
		static CvString szPrefix = "COLOR_";
		return (ColorTypes)getInfoTypeForString((szPrefix + szType).c_str());
	}
	inline DiploCommentTypes getAIDiploCommentType(const char* szType) const
	{
		static CvString szPrefix = "AI_DIPLOCOMMENT_";
		return (DiploCommentTypes)getInfoTypeForString((szPrefix + szType).c_str());
	} // </advc>
	void setInfoTypeFromString(const char* szType, int idx);
	DllExport void infoTypeFromStringReset();
	DllExport void infosReset();

	//inline int getNumGameTextXML() const { return (int)m_paGameTextXML.size(); } // advc.003j: unused
	DllExport inline int getActiveLandscapeID() { CvGlobals const& kThis = *this; return kThis.getActiveLandscapeID(); }
	inline int getActiveLandscapeID() const { return m_iActiveLandscapeID; } // advc
	DllExport void setActiveLandscapeID(int iLandscapeID);
	// <advc.003x> So that CvMap doesn't have to use CvLandscapeInfo directly
	int getLandscapePlotsPerCellX() const;
	int getLandscapePlotsPerCellY() const; // </advc.003x>
	DllExport int& getNumPlayableCivilizationInfos();
	DllExport int& getNumAIPlayableCivilizationInfos();
// <advc.enum>
#pragma region InfoAccessors
	// DllExports turned into wrappers/adapters
	// Start with those that have no associated enum type
	DllExport std::vector<CvInterfaceModeInfo*>& getInterfaceModeInfo(); // (advc: deprecated)
	DllExport int getNumRouteModelInfos() { CvGlobals const& kThis = *this; return kThis.getNumRouteModelInfos(); }
	DllExport int getNumRiverModelInfos() { CvGlobals const& kThis = *this; return kThis.getNumRiverModelInfos(); }
	DllExport int getNumTerrainPlaneInfos() { CvGlobals const& kThis = *this; return kThis.getNumTerrainPlaneInfos(); }
	DllExport int getNumUnitFormationInfos() { CvGlobals const& kThis = *this; return kThis.getNumUnitFormationInfos(); }
	DllExport int getNumThroneRoomInfos(); // 003v: No in-line definition; needs to do some extra work.
	DllExport int getNumThroneRoomStyleInfos() { CvGlobals const& kThis = *this; return kThis.getNumThroneRoomStyleInfos(); }
	DllExport int getNumSlideShowInfos() { CvGlobals const& kThis = *this; return kThis.getNumSlideShowInfos(); }
	DllExport int getNumSlideShowRandomInfos() { CvGlobals const& kThis = *this; return kThis.getNumSlideShowRandomInfos(); }
	DllExport int getNumWorldPickerInfos() { CvGlobals const& kThis = *this; return kThis.getNumWorldPickerInfos(); }
	DllExport int getNumSpaceShipInfos() { CvGlobals const& kThis = *this; return kThis.getNumSpaceShipInfos(); }
	DllExport int getNumHints() { CvGlobals const& kThis = *this; return kThis.getNumHintInfos(); }
	DllExport int getNumCameraOverlayInfos() { CvGlobals const& kThis = *this; return kThis.getNumCameraOverlayInfos(); }
	DllExport int getNumActionInfos() { CvGlobals const& kThis = *this; return kThis.getNumActionInfos(); }
	DllExport CvRouteModelInfo& getRouteModelInfo(int iRouteModel) { CvGlobals const& kThis = *this; return kThis.getRouteModelInfo(iRouteModel); }
	DllExport CvRiverModelInfo& getRiverModelInfo(int iRiverModel) { CvGlobals const& kThis = *this; return kThis.getRiverModelInfo(iRiverModel); }
	DllExport CvTerrainPlaneInfo& getTerrainPlaneInfo(int iTerrainPlane) { CvGlobals const& kThis = *this; return kThis.getTerrainPlaneInfo(iTerrainPlane); }
	DllExport CvUnitFormationInfo& getUnitFormationInfo(int iUnitFormation) { CvGlobals const& kThis = *this; return kThis.getUnitFormationInfo(iUnitFormation); }
	DllExport CvThroneRoomInfo& getThroneRoomInfo(int iThroneRoom) { CvGlobals const& kThis = *this; return kThis.getThroneRoomInfo(iThroneRoom); }
	DllExport CvThroneRoomStyleInfo& getThroneRoomStyleInfo(int iThroneRoomStyle) { CvGlobals const& kThis = *this; return kThis.getThroneRoomStyleInfo(iThroneRoomStyle); }
	DllExport CvThroneRoomCamera& getThroneRoomCamera(int iThroneRoomCamera) { CvGlobals const& kThis = *this; return kThis.getThroneRoomCameraInfo(iThroneRoomCamera); }
	DllExport CvSlideShowInfo& getSlideShowInfo(int iSlideShow) { CvGlobals const& kThis = *this; return kThis.getSlideShowInfo(iSlideShow); }
	DllExport CvSlideShowRandomInfo& getSlideShowRandomInfo(int iSlideShowRandom) { CvGlobals const& kThis = *this; return kThis.getSlideShowRandomInfo(iSlideShowRandom); }
	DllExport CvWorldPickerInfo& getWorldPickerInfo(int iWorldPicker) { CvGlobals const& kThis = *this; return kThis.getWorldPickerInfo(iWorldPicker); }
	DllExport CvSpaceShipInfo& getSpaceShipInfo(int iSpaceShip) { CvGlobals const& kThis = *this; return kThis.getSpaceShipInfo(iSpaceShip); }
	DllExport CvInfoBase& getHints(int iHint) { CvGlobals const& kThis = *this; return kThis.getHintInfo(iHint); }
	DllExport CvMainMenuInfo& getMainMenus(int iMainMenu)
	{	/*	advc (note): The caller should check this - but the EXE doesn't. Important
			for switching in between mods when a mod adds or removes menu backgrounds. */
		if (iMainMenu >= getNumMainMenuInfos())
			iMainMenu = 0;
		CvGlobals const& kThis = *this; return kThis.getMainMenuInfo(iMainMenu);
	}
	DllExport CvWaterPlaneInfo& getWaterPlaneInfo(int iWaterPlane) { CvGlobals const& kThis = *this; return kThis.getWaterPlaneInfo(iWaterPlane); }
	DllExport CvLandscapeInfo& getLandscapeInfo(int iLandscape) { CvGlobals const& kThis = *this; return kThis.getLandscapeInfo(iLandscape); }
	DllExport CvCameraOverlayInfo& getCameraOverlayInfo(int iCameraOverlay) { CvGlobals const& kThis = *this; return kThis.getCameraOverlayInfo((CameraOverlayTypes)iCameraOverlay); }
	DllExport CvActionInfo& getActionInfo(int iAction) { CvGlobals const& kThis = *this; return kThis.getActionInfo(iAction); }
	// DllExports with associated enum type
	DllExport int getNumPlayerColorInfos() { CvGlobals const& kThis = *this; return kThis.getNumPlayerColorInfos(); }
	DllExport int getNumWorldInfos() { CvGlobals const& kThis = *this; return kThis.getNumWorldInfos(); }
	DllExport int getNumSeaLevelInfos() { CvGlobals const& kThis = *this; return kThis.getNumSeaLevelInfos(); }
	DllExport int getNumClimateInfos() { CvGlobals const& kThis = *this; return kThis.getNumClimateInfos(); }
	DllExport int getNumTerrainInfos() { CvGlobals const& kThis = *this; return kThis.getNumTerrainInfos(); }
	DllExport int getNumBonusInfos() { CvGlobals const& kThis = *this; return kThis.getNumBonusInfos(); }
	DllExport int getNumFeatureInfos() { CvGlobals const& kThis = *this; return kThis.getNumFeatureInfos(); }
	DllExport int getNumCivilizationInfos() { CvGlobals const& kThis = *this; return kThis.getNumCivilizationInfos(); }
	DllExport int getNumLeaderHeadInfos() { CvGlobals const& kThis = *this; return kThis.getNumLeaderHeadInfos(); }
	DllExport int getNumCursorInfos() { CvGlobals const& kThis = *this; return kThis.getNumCursorInfos(); }
	DllExport int getNumRouteInfos() { CvGlobals const& kThis = *this; return kThis.getNumRouteInfos(); }
	DllExport int getNumImprovementInfos() { CvGlobals const& kThis = *this; return kThis.getNumImprovementInfos(); }
	DllExport int getNumHandicapInfos() { CvGlobals const& kThis = *this; return kThis.getNumHandicapInfos(); }
	DllExport int getNumGameSpeedInfos() { CvGlobals const& kThis = *this; return kThis.getNumGameSpeedInfos(); }
	DllExport int getNumTurnTimerInfos() { CvGlobals const& kThis = *this; return kThis.getNumTurnTimerInfos(); }
	DllExport int getNumEraInfos() { CvGlobals const& kThis = *this; return kThis.getNumEraInfos(); }
	DllExport int getNumVictoryInfos() { CvGlobals const& kThis = *this; return kThis.getNumVictoryInfos(); }

	DllExport CvEffectInfo& getEffectInfo(int iEffect) { return getInfo((EffectTypes)iEffect); }
	DllExport CvAttachableInfo& getAttachableInfo(int iAttachable) { return getInfo((AttachableTypes)iAttachable); }
	DllExport CvColorInfo& getColorInfo(ColorTypes eColor); // advc.106i: No inline definition; needs to do some extra work.
	DllExport CvPlayerColorInfo& getPlayerColorInfo(PlayerColorTypes ePlayerColor) { return getInfo(ePlayerColor); }
	DllExport CvWorldInfo& getWorldInfo(WorldSizeTypes eWorld) { return getInfo(eWorld); }
	DllExport CvInterfaceModeInfo& getInterfaceModeInfo(InterfaceModeTypes e) { return getInfo(e); }
	DllExport CvClimateInfo& getClimateInfo(ClimateTypes eClimate) { return getInfo(eClimate); }
	DllExport CvSeaLevelInfo& getSeaLevelInfo(SeaLevelTypes eSeaLevel) { return getInfo(eSeaLevel); }
	DllExport CvAnimationPathInfo& getAnimationPathInfo(AnimationPathTypes eAnimationPath) { return getInfo(eAnimationPath); }
	DllExport CvAnimationCategoryInfo& getAnimationCategoryInfo(AnimationCategoryTypes eAnimationCategory) { return getInfo(eAnimationCategory); }
	DllExport CvEntityEventInfo& getEntityEventInfo(EntityEventTypes e) { return getInfo(e); }
	DllExport CvTerrainInfo& getTerrainInfo(TerrainTypes eTerrain) { return getInfo(eTerrain); }
	DllExport CvBonusInfo& getBonusInfo(BonusTypes eBonus) { return getInfo(eBonus); }
	DllExport CvFeatureInfo& getFeatureInfo(FeatureTypes eFeature) { return getInfo(eFeature); }
	DllExport CvCivilizationInfo& getCivilizationInfo(CivilizationTypes eCivilization) { return getInfo(eCivilization); }
	DllExport CvLeaderHeadInfo& getLeaderHeadInfo(LeaderHeadTypes eLeaderHead) { return getInfo(eLeaderHead); }
	DllExport CvCursorInfo& getCursorInfo(CursorTypes eCursor) { return getInfo(eCursor); }
	DllExport CvGameOptionInfo& getGameOptionInfo(GameOptionTypes eGameOption) { return getInfo(eGameOption); }
	DllExport CvMPOptionInfo& getMPOptionInfo(MultiplayerOptionTypes eMPOption) { return getInfo(eMPOption); }
	DllExport CvForceControlInfo& getForceControlInfo(ForceControlTypes eForceControl) { return getInfo(eForceControl); }
	DllExport CvPlayerOptionInfo& getPlayerOptionInfo(PlayerOptionTypes ePlayerOption) { return getInfo(ePlayerOption); }
	DllExport CvGraphicOptionInfo& getGraphicOptionInfo(GraphicOptionTypes eGraphicOption) { return getInfo(eGraphicOption); }
	DllExport CvImprovementInfo& getImprovementInfo(ImprovementTypes eImprovement) { return getInfo(eImprovement); }
	DllExport CvBuildInfo& getBuildInfo(BuildTypes eBuild) { return getInfo(eBuild); }
	DllExport CvHandicapInfo& getHandicapInfo(HandicapTypes eHandicap) { return getInfo(eHandicap); }
	DllExport CvGameSpeedInfo& getGameSpeedInfo(GameSpeedTypes eGameSpeed) { return getInfo(eGameSpeed); }
	DllExport CvTurnTimerInfo& getTurnTimerInfo(TurnTimerTypes eTurnTimer) { return getInfo(eTurnTimer); }
	DllExport CvMissionInfo& getMissionInfo(MissionTypes eMission) { return getInfo(eMission); }
	DllExport CvEraInfo& getEraInfo(EraTypes eEra) { return getInfo(eEra); }
	DllExport CvVictoryInfo& getVictoryInfo(VictoryTypes eVictory) { return getInfo(eVictory); }

	// Generate accessors for use within the DLL
	DO_FOR_EACH_STATIC_INFO_TYPE(MAKE_INFO_ACCESSORS_STATIC)
	DO_FOR_EACH_DYN_INFO_TYPE(MAKE_INFO_ACCESSORS_DYN)
	DO_FOR_EACH_INT_INFO_TYPE(MAKE_INFO_ACCESSORS_INT)
	// World(Size)Info: awkward to generate through a macro
	inline CvWorldInfo& getInfo(WorldSizeTypes eWorld) const
	{
		FAssertBounds(0, getNumWorldInfos(), eWorld);
		return *m_paWorldInfo[eWorld];
	}
	inline int getNumWorldInfos() const
	{
		return (int)m_paWorldInfo.size();
	}
	inline CvWorldInfo& getWorldInfo(WorldSizeTypes eWorld) const // deprecated
	{
		return getInfo(eWorld);
	}
#pragma endregion InfoAccessors
	// </advc.enum>

	/*	Global Types
		All type strings are upper case and are kept in this hash map for fast lookup
		The other functions are kept for convenience when enumerating, but most are not used. */
	DllExport int getTypesEnum(const char* szType) const // use this when searching for a type
	// <advc.006> Add two params
	{ return getTypesEnum(szType, false); }
	int getTypesEnum(const char* szType, bool bHideAssert, bool bFromPython = false) const;
	// </advc.006>
	void setTypesEnum(const char* szType, int iEnum);

	int& getNumEntityEventTypes();
	CvString*& getEntityEventTypes();
	CvString& getEntityEventTypes(EntityEventTypes e);

	int& getNumAnimationOperatorTypes();
	CvString*& getAnimationOperatorTypes();
	CvString& getAnimationOperatorTypes(AnimationOperatorTypes e);

	CvString*& getFunctionTypes();
	CvString& getFunctionTypes(FunctionTypes e);

	inline int& getNumFlavorTypes() { return m_iNumFlavorTypes; }
	inline int const& getNumFlavorTypes() const { return m_iNumFlavorTypes; } // advc
	CvString*& getFlavorTypes();
	CvString& getFlavorTypes(FlavorTypes e);

	DllExport int& getNumArtStyleTypes();
	CvString*& getArtStyleTypes();
	DllExport CvString& getArtStyleTypes(ArtStyleTypes e);

	//int& getNumCitySizeTypes(); // advc: Use NUM_CITYSIZE_TYPES instead
	CvString*& getCitySizeTypes();
	CvString& getCitySizeTypes(CitySizeTypes e);

	CvString*& getContactTypes();
	CvString& getContactTypes(ContactTypes e);

	CvString*& getDiplomacyPowerTypes();
	CvString& getDiplomacyPowerTypes(DiplomacyPowerTypes e);

	CvString*& getAutomateTypes();
	CvString& getAutomateTypes(AutomateTypes e);

	CvString*& getDirectionTypes();
	CvString& getDirectionTypes(AutomateTypes e);

	DllExport int& getNumFootstepAudioTypes();
	int getNumFootstepAudioTypes() const { return m_iNumFootstepAudioTypes; } // advc
	CvString*& getFootstepAudioTypes();
	CvString& getFootstepAudioTypes(int i);
	int getFootstepAudioTypeByTag(CvString strTag);

	CvString*& getFootstepAudioTags();
	DllExport CvString& getFootstepAudioTags(int i);

	CvString const& getCurrentXMLFile() const; // advc: 2x const
	void setCurrentXMLFile(const TCHAR* szFileName);
	// <advc.003v>
	void setXMLLoadUtility(CvXMLLoadUtility* pXML);
	void loadOptionalXMLInfo();
	void loadThroneRoomInfo();
	// </advc.003v>

	DllExport FVariableSystem* getDefinesVarSystem() { return m_VarSystem; }
	// advc: Need a const version
	FVariableSystem const* getDefinesVarSystem() const { return m_VarSystem; }
	void cacheGlobals();

	// ***** EXPOSED TO PYTHON *****
	DllExport inline int getDefineINT(char const* szName) const
	{
		return getDefineINT(szName, 0); // advc.opt: Call the BBAI version
	}
	// BETTER_BTS_AI_MOD, Efficiency, Options, 02/21/10, jdog5000:
	int getDefineINT(char const* szName, int iDefault) const;
	// <advc>
	inline bool getDefineBOOL(char const* szName, bool bDefault = false) const
	{
		return (getDefineINT(szName, (int)bDefault) > 0);
	} // </advc>
	DllExport float getDefineFLOAT(char const* szName) const;
	/*	advc (note): Global TextVals loaded by CvXMLLoadUtility::
		SetPostGlobalsGlobalDefines need to be accessed through getDefineINT instead. */
	DllExport const char* getDefineSTRING(char const* szName) const;
	/*  advc.opt: Params for suppressing cache update added. False for string b/c
		there are none that we could update. */
	void setDefineINT(char const* szName, int iValue, bool bUpdateCache = true);
	void setDefineFLOAT(char const* szName, float fValue, bool bUpdateCache = true);
	void setDefineSTRING(char const* szName, char const* szValue, bool bUpdateCache = false);
	// advc.opt:
#pragma region GlobalDefines
	/*  Access cached integer GlobalDefines through enum values
		(not exposed to Python - though that might be nice). */
	#define DO_FOR_EACH_GLOBAL_DEFINE(DO) \
		DO(EXTRA_YIELD) /* K-Mod */ \
		DO(CITY_RADIUS_DECAY) /* advc.130s */ \
		DO(REVOLTS_IGNORE_CULTURE_RANGE) /* advc.099c */ \
		DO(NUM_WARNING_REVOLTS) /* advc.101 */ \
		DO(OCCUPATION_COUNTDOWN_EXPONENT) /* advc.023 */ \
		DO(MAX_DISTANCE_CITY_MAINTENANCE) /* advc.140 */ \
		DO(OWN_EXCLUSIVE_RADIUS) /* advc.035 */ \
		DO(ANNOUNCE_REPARATIONS) /* advc.039 */ \
		DO(PER_PLAYER_MESSAGE_CONTROL_LOG) /* advc.007 */ \
		DO(DELAY_UNTIL_BUILD_DECAY) /* advc.011 */ \
		DO(DISENGAGE_LENGTH) /* advc.034 */ \
		DO(AT_WAR_ATTITUDE_CHANGE) /* advc.130g */ \
		DO(RESEARCH_MODIFIER_EXTRA_TEAM_MEMBER) /* advc.156 */ \
		DO(WORKER_RESERVE_PERCENT) /* advc.113 */ \
		DO(CITY_DEFENSE_DAMAGE_HEAL_RATE) /* cdtw.2 */ \
		/* <advc.148> */ \
		DO(RELATIONS_THRESH_FRIENDLY) \
		DO(RELATIONS_THRESH_PLEASED) \
		DO(RELATIONS_THRESH_FURIOUS) \
		DO(RELATIONS_THRESH_ANNOYED) \
		/* </advc.148> */ \
		DO(MINIMAP_RENDER_SIZE) /* advc.106m */ \
		DO(CAN_TRAIN_CHECKS_AIR_UNIT_CAP) /* advc.001b */ \
		/* <advc.groundbr> */ \
		DO(AI_GROUNDBREAKING_PENALTY_ENABLE) \
		DO(HUMAN_GROUNDBREAKING_PENALTY_ENABLE) \
		/* </advc.groundbr> */ \
		DO(ENABLE_162) /* advc.162 */ \
		/* <advc.ctr> */ \
		DO(CITY_TRADE_CULTURE_THRESH) \
		DO(NATIVE_CITY_CULTURE_THRESH) /* </advc.ctr> */ \
		/* <advc.opt> */ \
		DO(DIPLOMACY_VALUE_REMAINDER) \
		DO(PEACE_TREATY_LENGTH) \
		DO(PLOT_VISIBILITY_RANGE) \
		DO(MAX_WORLD_WONDERS_PER_CITY) \
		DO(MAX_TEAM_WONDERS_PER_CITY) \
		DO(MAX_NATIONAL_WONDERS_PER_CITY_FOR_OCC) \
		DO(MAX_NATIONAL_WONDERS_PER_CITY) \
		DO(CONSCRIPT_POPULATION_PER_COST) \
		DO(NO_MILITARY_PERCENT_ANGER) \
		DO(HURRY_POP_ANGER) \
		DO(CONSCRIPT_POP_ANGER) \
		DO(VASSAL_HAPPINESS) \
		DO(HURRY_ANGER_DIVISOR) \
		DO(FRESH_WATER_HEALTH_CHANGE) \
		DO(POWER_HEALTH_CHANGE) \
		DO(DIRTY_POWER_HEALTH_CHANGE) \
		DO(CITY_AIR_UNIT_CAPACITY) \
		DO(COLLATERAL_COMBAT_DAMAGE) \
		DO(TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER) \
		DO(AI_OFFER_EXTRA_GOLD_PERCENT) \
		DO(RECON_VISIBILITY_RANGE) \
		DO(UNIT_VISIBILITY_RANGE) \
		DO(MAX_FORTIFY_TURNS) \
		DO(WAR_SUCCESS_ATTACKING) \
		DO(WAR_SUCCESS_DEFENDING) \
		DO(ESPIONAGE_SPY_INTERCEPT_MOD) \
		DO(ESPIONAGE_SPY_NO_INTRUDE_INTERCEPT_MOD) \
		DO(USE_SPIES_NO_ENTER_BORDERS) \
		DO(MIN_EXPERIENCE_PER_COMBAT) \
		DO(MAX_EXPERIENCE_PER_COMBAT) \
		DO(EXPERIENCE_FROM_WITHDRAWL) \
		DO(LAND_UNITS_CAN_ATTACK_WATER_CITIES) \
		DO(BASE_UNIT_UPGRADE_COST) \
		DO(UNIT_UPGRADE_COST_PER_PRODUCTION) \
		DO(AIR_COMBAT_DAMAGE) \
		DO(SHIP_BLOCKADE_RANGE) \
		DO(MAX_TRADE_ROUTES) \
		DO(ENABLE_DEBUG_TOOLS_MULTIPLAYER) \
		DO(FREE_VASSAL_LAND_PERCENT) \
		DO(FREE_VASSAL_POPULATION_PERCENT) \
		DO(OVERSEAS_TRADE_MODIFIER) \
		DO(MIN_REVOLUTION_TURNS) \
		/* </advc.opt> */ \
		DO(PATH_DAMAGE_WEIGHT) \
		DO(HILLS_EXTRA_DEFENSE) \
		DO(RIVER_ATTACK_MODIFIER) \
		DO(AMPHIB_ATTACK_MODIFIER) \
		DO(HILLS_EXTRA_MOVEMENT) \
		DO(PERCENT_ANGER_DIVISOR) \
		DO(MIN_CITY_RANGE) \
		DO(CITY_MAX_NUM_BUILDINGS) \
		DO(LAKE_MAX_AREA_SIZE) \
		DO(PEAK_SEE_THROUGH_CHANGE) \
		DO(HILLS_SEE_THROUGH_CHANGE) \
		DO(SEAWATER_SEE_FROM_CHANGE) \
		DO(PEAK_SEE_FROM_CHANGE) \
		DO(HILLS_SEE_FROM_CHANGE) \
		DO(MAX_CITY_DEFENSE_DAMAGE) \
		DO(MIN_WATER_SIZE_FOR_OCEAN) \
		DO(MOVE_DENOMINATOR) \
		DO(FOOD_CONSUMPTION_PER_POPULATION) \
		DO(MAX_HIT_POINTS) \
		DO(MAX_PLOT_LIST_ROWS) \
		DO(UNIT_MULTISELECT_MAX) \
		/*DO(EVENT_MESSAGE_TIME) \ (cached separately) */ \
		DO(EVENT_MESSAGE_TIME_LONG) \
		DO(NUM_UNIT_PREREQ_OR_BONUSES) \
		DO(NUM_UNIT_AND_TECH_PREREQS) \
		DO(NUM_BUILDING_PREREQ_OR_BONUSES) \
		DO(NUM_BUILDING_AND_TECH_PREREQS) \
		DO(NUM_AND_TECH_PREREQS) \
		DO(NUM_OR_TECH_PREREQS) \
		DO(NUM_ROUTE_PREREQ_OR_BONUSES) \
		DO(NUM_CORPORATION_PREREQ_BONUSES) \
		/* BETTER_BTS_AI_MOD, Efficiency, Options, 02/21/10, jdog5000: START */ \
		DO(COMBAT_DIE_SIDES) \
		DO(COMBAT_DAMAGE) \
		DO(WAR_SUCCESS_CITY_CAPTURING) \
		DO(TECH_COST_KNOWN_PREREQ_MODIFIER) \
		DO(TECH_COST_FIRST_KNOWN_PREREQ_MODIFIER) \
		DO(BBAI_DEFENSIVE_PACT_BEHAVIOR) \
		DO(BBAI_ATTACK_CITY_STACK_RATIO) \
		DO(BBAI_SKIP_BOMBARD_BASE_STACK_RATIO) \
		DO(BBAI_SKIP_BOMBARD_MIN_STACK_RATIO) \
		/* From Lead From Behind by UncutDragon. (edited for K-Mod) */ \
		DO(LFB_ENABLE) DO(LFB_BASEDONGENERAL) DO(LFB_BASEDONEXPERIENCE) \
		DO(LFB_BASEDONLIMITED) DO(LFB_BASEDONHEALER) DO(LFB_DEFENSIVEADJUSTMENT) \
		DO(LFB_USESLIDINGSCALE) DO(LFB_ADJUSTNUMERATOR) DO(LFB_ADJUSTDENOMINATOR) \
		DO(LFB_USECOMBATODDS) /* BETTER_BTS_AI_MOD: END */ \
		DO(POWER_CORRECTION) /* advc.104 */
	#define MAKE_ENUMERATOR(VAR) VAR,
	enum GlobalDefines
	{
		NO_GLOBAL_DEFINE = -1,
		DO_FOR_EACH_GLOBAL_DEFINE(MAKE_ENUMERATOR)
		NUM_GLOBAL_DEFINES
	};
	__forceinline int getDefineINT(GlobalDefines eVarName) const
	{
		return m_aiGlobalDefinesCache[eVarName];
	}
	__forceinline bool getDefineBOOL(GlobalDefines eVarName) const
	{
		return (getDefineINT(eVarName) > 0);
	}
	// Keep these as wrappers; too many call locations to change or DllExport.
	// These are all exposed to Python
	inline int getMOVE_DENOMINATOR() const { return getDefineINT(MOVE_DENOMINATOR); }
	inline int getFOOD_CONSUMPTION_PER_POPULATION() const { return getDefineINT(FOOD_CONSUMPTION_PER_POPULATION); }
	inline int getMAX_HIT_POINTS() const { return getDefineINT(MAX_HIT_POINTS); }
	inline int getPERCENT_ANGER_DIVISOR() const { return getDefineINT(PERCENT_ANGER_DIVISOR); }
	inline int getMAX_CITY_DEFENSE_DAMAGE() const { return getDefineINT(MAX_CITY_DEFENSE_DAMAGE); }
	DllExport inline int getMAX_PLOT_LIST_ROWS() { CvGlobals const& kThis = *this; return kThis.getMAX_PLOT_LIST_ROWS(); }
	inline int getMAX_PLOT_LIST_ROWS() const { return getDefineINT(MAX_PLOT_LIST_ROWS); }
	DllExport inline int getUNIT_MULTISELECT_MAX() { CvGlobals const& kThis = *this; return kThis.getUNIT_MULTISELECT_MAX(); }
	inline int getUNIT_MULTISELECT_MAX() const { return getDefineINT(UNIT_MULTISELECT_MAX); }
	// Note: The EXE calls this during audio init if all audio devices are disabled
	DllExport inline int getEVENT_MESSAGE_TIME() { CvGlobals const& kThis = *this; return kThis.getEVENT_MESSAGE_TIME(); }
	inline int getEVENT_MESSAGE_TIME() const { return m_iEventMessageTime; }
	// BETTER_BTS_AI_MOD, Efficiency, Options, 02/21/10, jdog5000: START
	inline int getWAR_SUCCESS_CITY_CAPTURING() const { return getDefineINT(WAR_SUCCESS_CITY_CAPTURING); }
	inline int getCOMBAT_DIE_SIDES() const { return getDefineINT(COMBAT_DIE_SIDES); }
	inline int getCOMBAT_DAMAGE() const { return getDefineINT(COMBAT_DAMAGE); }
	// BETTER_BTS_AI_MOD: END
	/*  <advc.opt> (TextVals can't be loaded by cacheGlobals. Hence also won't be
		updated when a setDefine... function is called.) */
	inline ImprovementTypes getRUINS_IMPROVEMENT() const
	{
		FAssertMsg(m_eRUINS_IMPROVEMENT != NO_IMPROVEMENT, "RUINS_IMPROVEMENT accessed before CvXMLLoadUtility::SetPostGlobalsGlobalDefines");
		return m_eRUINS_IMPROVEMENT;
	}
	void setRUINS_IMPROVEMENT(int iVal);
	inline SpecialistTypes getDEFAULT_SPECIALIST() const
	{
		FAssertMsg(m_eDEFAULT_SPECIALIST != NO_SPECIALIST, "DEFAULT_SPECIALIST accessed before CvXMLLoadUtility::SetPostGlobalsGlobalDefines");
		return m_eDEFAULT_SPECIALIST;
	}
	void setDEFAULT_SPECIALIST(int iVal);
	inline TerrainTypes getWATER_TERRAIN(bool bShallow) const
	{
		TerrainTypes r = m_aeWATER_TERRAIN[bShallow];
		FAssertMsg(r != NO_TERRAIN, "WATER_TERRAIN accessed before CvXMLLoadUtility::SetPostGlobalsGlobalDefines");
		return r;
	}
	void setWATER_TERRAIN(bool bShallow, int iValue);
	// </advc.opt>
	/*	advc.003t: Still exposed to Python, but, in the DLL, CvInfo functions,
		e.g. CvTechInfo::getNumOrTechPrereqs, should be used instead. */
	/*int getNUM_UNIT_PREREQ_OR_BONUSES() const;
	int getNUM_UNIT_AND_TECH_PREREQS() const;
	int getNUM_BUILDING_PREREQ_OR_BONUSES() const;
	int getNUM_BUILDING_AND_TECH_PREREQS() const;
	int getNUM_AND_TECH_PREREQS() const;
	int getNUM_OR_TECH_PREREQS() const;
	int getNUM_ROUTE_PREREQ_OR_BONUSES() const;
	int getNUM_CORPORATION_PREREQ_BONUSES() const;*/
	// advc: All inlined and constified
	DllExport inline float getCAMERA_MIN_YAW() { CvGlobals const& kThis = *this; return kThis.getCAMERA_MIN_YAW(); }
	inline float getCAMERA_MIN_YAW() const { return m_fCAMERA_MIN_YAW; }
	DllExport inline float getCAMERA_MAX_YAW() { CvGlobals const& kThis = *this; return kThis.getCAMERA_MAX_YAW(); }
	inline float getCAMERA_MAX_YAW() const { return m_fCAMERA_MAX_YAW; }
	DllExport inline float getCAMERA_FAR_CLIP_Z_HEIGHT() { CvGlobals const& kThis = *this; return kThis.getCAMERA_FAR_CLIP_Z_HEIGHT(); }
	inline float getCAMERA_FAR_CLIP_Z_HEIGHT() const { return m_fCAMERA_FAR_CLIP_Z_HEIGHT; }
	DllExport inline float getCAMERA_MAX_TRAVEL_DISTANCE() { CvGlobals const& kThis = *this; return kThis.getCAMERA_MAX_TRAVEL_DISTANCE(); }
	inline float getCAMERA_MAX_TRAVEL_DISTANCE() const { return m_fCAMERA_MAX_TRAVEL_DISTANCE; }
	DllExport inline float getCAMERA_START_DISTANCE() { CvGlobals const& kThis = *this; return kThis.getCAMERA_START_DISTANCE(); }
	inline float getCAMERA_START_DISTANCE() const { return m_fCAMERA_START_DISTANCE; }
	DllExport inline float getAIR_BOMB_HEIGHT() { CvGlobals const& kThis = *this; return kThis.getAIR_BOMB_HEIGHT(); }
	inline float getAIR_BOMB_HEIGHT() const { return m_fAIR_BOMB_HEIGHT; }
	DllExport inline float getPLOT_SIZE() { CvGlobals const& kThis = *this; return kThis.getPLOT_SIZE(); }
	inline float getPLOT_SIZE() const { return m_fPLOT_SIZE; }
	DllExport inline float getCAMERA_SPECIAL_PITCH() { CvGlobals const& kThis = *this; return kThis.getCAMERA_SPECIAL_PITCH(); }
	inline float getCAMERA_SPECIAL_PITCH() const { return m_fCAMERA_SPECIAL_PITCH; }
	DllExport inline float getCAMERA_MAX_TURN_OFFSET() { CvGlobals const& kThis = *this; return kThis.getCAMERA_MAX_TURN_OFFSET(); }
	inline float getCAMERA_MAX_TURN_OFFSET() const { return m_fCAMERA_MAX_TURN_OFFSET; }
	DllExport inline float getCAMERA_MIN_DISTANCE() { CvGlobals const& kThis = *this; return kThis.getCAMERA_MIN_DISTANCE(); }
	inline float getCAMERA_MIN_DISTANCE() const { return m_fCAMERA_MIN_DISTANCE; }
	DllExport inline float getCAMERA_UPPER_PITCH() { CvGlobals const& kThis = *this; return kThis.getCAMERA_UPPER_PITCH(); }
	inline float getCAMERA_UPPER_PITCH() const { return m_fCAMERA_UPPER_PITCH; }
	DllExport inline float getCAMERA_LOWER_PITCH() { CvGlobals const& kThis = *this; return kThis.getCAMERA_LOWER_PITCH(); }
	inline float getCAMERA_LOWER_PITCH() const { return m_fCAMERA_LOWER_PITCH; }
	DllExport inline float getFIELD_OF_VIEW() { CvGlobals const& kThis = *this; return kThis.getFIELD_OF_VIEW(); }
	inline float getFIELD_OF_VIEW() const { return m_fFIELD_OF_VIEW; }
	DllExport float getSHADOW_SCALE() { CvGlobals const& kThis = *this; return kThis.getSHADOW_SCALE(); }
	inline float getSHADOW_SCALE() const { return m_fSHADOW_SCALE; }
	DllExport inline float getUNIT_MULTISELECT_DISTANCE() { CvGlobals const& kThis = *this; return kThis.getUNIT_MULTISELECT_DISTANCE(); }
	inline float getUNIT_MULTISELECT_DISTANCE() const { return m_fUNIT_MULTISELECT_DISTANCE; }
	void updateCameraStartDistance(bool bReset); // advc.004m  (exposed to Python)

	DllExport int getUSE_FINISH_TEXT_CALLBACK();
	// advc.003y: Moved the other callback getters to CvPythonCaller
#pragma endregion GlobalDefines
	/*	K-Mod: more reliable versions of the 'gDLL->xxxKey' functions
		NOTE: I've replaced all calls to the gDLL key functions with calls to these functions. */
	inline bool altKey() const { return (GetKeyState(VK_MENU) & 0x8000); }
	inline bool ctrlKey() const { return (GetKeyState(VK_CONTROL) & 0x8000); }
	inline bool shiftKey() const { return (GetKeyState(VK_SHIFT) & 0x8000); }
	// hold X to temporarily suppress automatic unit cycling.
	bool suppressCycling() const
	{
		return (GetKeyState('X') & 0x8000) ||
			/*	advc.088: Needs to be consistent with GC.getInfo(CONTROL_UNSELECT_ALL).
				getHotKeyVal(), isAltDown() etc. Could check those here, but not in-line.
				Not quite sure if performance matters here. If it doesn't, then key X
				should perhaps also be a ControlInfo. */
			((GetKeyState('U') & 0x8000) && shiftKey() && altKey());
	}
	// K-Mod end

	DllExport int getMAX_CIV_PLAYERS(); // advc: Shouldn't be used in the DLL
	/*  The rest of these (getMAX_PLAYERS, getMAX_CIV_TEAMS, getMAX_TEAMS,
		getBARBARIAN_PLAYER, getBARBARIAN_TEAM, getINVALID_PLOT_COORD,
		getNUM_CITY_PLOTS, getCITY_HOME_PLOT) were only used by CyGlobalContext. */

	DllExport void setDLLIFace(CvDLLUtilityIFaceBase* pDll);
#ifdef _USRDLL
	// inlined for perf reasons, do not use outside of dll
	inline CvDLLUtilityIFaceBase* getDLLIFace() const { return m_pDLL; } // advc: const, inline keyword added
#endif
	DllExport CvDLLUtilityIFaceBase* getDLLIFaceNonInl();
	DllExport void setDLLProfiler(FProfiler* prof);
	FProfiler* getDLLProfiler();
	DllExport void enableDLLProfiler(bool bEnable);
	bool isDLLProfilerEnabled() const;

	DllExport bool IsGraphicsInitialized() const;
	DllExport void SetGraphicsInitialized(bool bVal);

	// for caching
#pragma region ReadInfoArrays
	DllExport bool readBuildingInfoArray(FDataStreamBase* pStream);
	DllExport void writeBuildingInfoArray(FDataStreamBase* pStream);

	DllExport bool readTechInfoArray(FDataStreamBase* pStream);
	DllExport void writeTechInfoArray(FDataStreamBase* pStream);

	DllExport bool readUnitInfoArray(FDataStreamBase* pStream);
	DllExport void writeUnitInfoArray(FDataStreamBase* pStream);

	DllExport bool readLeaderHeadInfoArray(FDataStreamBase* pStream);
	DllExport void writeLeaderHeadInfoArray(FDataStreamBase* pStream);

	DllExport bool readCivilizationInfoArray(FDataStreamBase* pStream);
	DllExport void writeCivilizationInfoArray(FDataStreamBase* pStream);

	DllExport bool readPromotionInfoArray(FDataStreamBase* pStream);
	DllExport void writePromotionInfoArray(FDataStreamBase* pStream);

	DllExport bool readDiplomacyInfoArray(FDataStreamBase* pStream);
	DllExport void writeDiplomacyInfoArray(FDataStreamBase* pStream);

	DllExport bool readCivicInfoArray(FDataStreamBase* pStream);
	DllExport void writeCivicInfoArray(FDataStreamBase* pStream);

	DllExport bool readHandicapInfoArray(FDataStreamBase* pStream);
	DllExport void writeHandicapInfoArray(FDataStreamBase* pStream);

	DllExport bool readBonusInfoArray(FDataStreamBase* pStream);
	DllExport void writeBonusInfoArray(FDataStreamBase* pStream);

	DllExport bool readImprovementInfoArray(FDataStreamBase* pStream);
	DllExport void writeImprovementInfoArray(FDataStreamBase* pStream);

	DllExport bool readEventInfoArray(FDataStreamBase* pStream);
	DllExport void writeEventInfoArray(FDataStreamBase* pStream);

	DllExport bool readEventTriggerInfoArray(FDataStreamBase* pStream);
	DllExport void writeEventTriggerInfoArray(FDataStreamBase* pStream);
#pragma endregion ReadInfoArrays

	// additional accessors for initializing globals ...

	DllExport void setInterface(CvInterface* pVal);
	DllExport void setDiplomacyScreen(CvDiplomacyScreen* pVal);
	DllExport void setMPDiplomacyScreen(CMPDiplomacyScreen* pVal);
	DllExport void setMessageQueue(CMessageQueue* pVal);
	DllExport void setHotJoinMessageQueue(CMessageQueue* pVal);
	DllExport void setMessageControl(CMessageControl* pVal);
	DllExport void setSetupData(CvSetupData* pVal);
	DllExport void setMessageCodeTranslator(CvMessageCodeTranslator* pVal);
	DllExport void setDropMgr(CvDropMgr* pVal);
	DllExport void setPortal(CvPortal* pVal);
	DllExport void setStatsReport(CvStatsReporter* pVal);
	DllExport void setPathFinder(FAStar* pVal);
	DllExport void setInterfacePathFinder(FAStar* pVal);
	DllExport void setStepFinder(FAStar* pVal);
	DllExport void setRouteFinder(FAStar* pVal);
	DllExport void setBorderFinder(FAStar* pVal);
	DllExport void setAreaFinder(FAStar* pVal);
	DllExport void setPlotGroupFinder(FAStar* pVal);

	// So that CvEnums are moddable in the DLL
	// (advc: Don't use these functions in the DLL)
	DllExport int getNumDirections() const;
	DllExport int getNumGameOptions() const;
	DllExport int getNumMPOptions() const;
	DllExport int getNumSpecialOptions() const;
	DllExport int getNumGraphicOptions() const;
	DllExport int getNumTradeableItems() const;
	DllExport int getNumBasicItems() const;
	DllExport int getNumTradeableHeadings() const;
	DllExport int getNumPlayerOptionInfos() const;
	DllExport int getMaxNumSymbols() const;
	DllExport int getNumGraphicLevels() const;
	/*int getNumCommandInfos() const { return NUM_COMMAND_TYPES; }
	int getNumControlInfos() const { return NUM_CONTROL_TYPES; }
	int getNumMissionInfos() const { return NUM_MISSION_TYPES; }
	int getNumGlobeLayers() const { return NUM_GLOBE_LAYER_TYPES; }*/ // advc
	DllExport int getNUM_ENGINE_DIRTY_BITS() const;
	DllExport int getNUM_INTERFACE_DIRTY_BITS() const;
	DllExport int getNUM_YIELD_TYPES() const;
	DllExport int getNUM_FORCECONTROL_TYPES() const;
	DllExport int getNUM_INFOBAR_TYPES() const;
	DllExport int getNUM_HEALTHBAR_TYPES() const;
	DllExport int getNUM_LEADERANIM_TYPES() const;
	/*int getNUM_CONTROL_TYPES() const;
	int getNUM_COMMERCE_TYPES() const;*/ // advc

	void deleteInfoArrays();
	bool isCachingDone() const; // advc.003c
	void setHoFScreenUp(bool b); // advc.106i

protected:

	bool m_bGraphicsInitialized;
	bool m_bDLLProfiler;
	bool m_bLogging;
	bool m_bRandLogging;
	bool m_bSynchLogging;
	bool m_bOverwriteLogs;
	/*NiPoint3  m_pt3CameraDir;
	int m_iNewPlayers;
	CMainMenu* m_pkMainMenu;
	bool m_bZoomOut;
	bool m_bZoomIn;
	bool m_bLoadGameFromFile;*/ // advc.003j: Unused; not even written.

	FMPIManager * m_pFMPMgr;

	CvRandomExtended* m_asyncRand; // advc.007b (was CvRandom)
	CvPythonCaller* m_pPythonCaller; // advc.003y
	CvDLLLogger* m_pLogger;
	CvGameAI* m_game;
	CvAgents* m_agents; // advc.agents

	CMessageQueue* m_messageQueue;
	CMessageQueue* m_hotJoinMsgQueue;
	CMessageControl* m_messageControl;
	CvSetupData* m_setupData;
	CvInitCore* m_iniInitCore;
	CvInitCore* m_loadedInitCore;
	CvInitCore* m_initCore;
	CvMessageCodeTranslator * m_messageCodes;
	CvDropMgr* m_dropMgr;
	CvPortal* m_portal;
	CvStatsReporter * m_statsReporter;
	CvInterface* m_interface;

	CvArtFileMgr* m_pArtFileMgr;

	CvMap* m_map;

	CvDiplomacyScreen* m_diplomacyScreen;
	CMPDiplomacyScreen* m_mpDiplomacyScreen;

	FAStar* m_pathFinder;
	FAStar* m_interfacePathFinder;
	FAStar* m_stepFinder;
	FAStar* m_routeFinder;
	FAStar* m_borderFinder;
	FAStar* m_areaFinder;
	FAStar* m_plotGroupFinder;

	//NiPoint3 m_pt3Origin; // advc.003j: unused

	// advc: Allocate these arrays statically
	int m_aiPlotDirectionX[NUM_DIRECTION_TYPES];
	int m_aiPlotDirectionY[NUM_DIRECTION_TYPES];
	int m_aiPlotCardinalDirectionX[NUM_CARDINALDIRECTION_TYPES];
	int m_aiPlotCardinalDirectionY[NUM_CARDINALDIRECTION_TYPES];
	int m_aiCityPlotX[NUM_CITY_PLOTS];
	int m_aiCityPlotY[NUM_CITY_PLOTS];
	int m_aiCityPlotPriority[NUM_CITY_PLOTS];
	CityPlotTypes m_aaeXYCityPlot[CITY_PLOTS_DIAMETER][CITY_PLOTS_DIAMETER];
	DirectionTypes m_aeTurnLeftDirection[NUM_DIRECTION_TYPES];
	DirectionTypes m_aeTurnRightDirection[NUM_DIRECTION_TYPES];
	DirectionTypes m_aaeXYDirection[DIRECTION_DIAMETER][DIRECTION_DIAMETER];


	// Globals loaded from XML ...

	// all type strings are upper case and are kept in this hash map for fast lookup, Moose
	typedef stdext::hash_map<std::string /* type string */, int /* info index */> InfosMap;
	InfosMap m_infosMap;
	int m_iActiveLandscapeID;
	int m_iNumPlayableCivilizationInfos;
	int m_iNumAIPlayableCivilizationInfos;
	// <advc.enum>
	// Don't need to keep track of these anymore
	//std::vector<std::vector<CvInfoBase*>*> m_aInfoVectors;
	#define DECLARE_INFO_VECTOR(Name, Dummy) \
		std::vector<Cv##Name##Info*> m_pa##Name##Info;
	DO_FOR_EACH_INFO_TYPE(DECLARE_INFO_VECTOR) // </advc.enum>
	std::vector<CvWorldInfo*> m_paWorldInfo;


	// GLOBAL TYPES ...

	// all type strings are upper case and are kept in this hash map for fast lookup, Moose
	typedef stdext::hash_map<std::string /* type string */, int /*enum value */> TypesMap;
	TypesMap m_typesMap;

	// XXX These are duplicates and are kept for enumeration convenience - most could be removed, Moose
	//CvString *m_paszEntityEventTypes2; // advc.003j: actually unused
	CvString *m_paszEntityEventTypes;
	int m_iNumEntityEventTypes;

	CvString *m_paszAnimationOperatorTypes;
	int m_iNumAnimationOperatorTypes;

	CvString* m_paszFunctionTypes;

	CvString* m_paszFlavorTypes;
	int m_iNumFlavorTypes;

	CvString *m_paszArtStyleTypes;
	int m_iNumArtStyleTypes;

	CvString *m_paszCitySizeTypes;

	CvString *m_paszContactTypes;
	CvString *m_paszDiplomacyPowerTypes;
	CvString *m_paszAutomateTypes;
	CvString *m_paszDirectionTypes;

	CvString *m_paszFootstepAudioTypes;
	int m_iNumFootstepAudioTypes;

	CvString *m_paszFootstepAudioTags;

	CvString m_szCurrentXMLFile;
	bool m_bHoFScreenUp; // advc.106i

	FVariableSystem* m_VarSystem;
	// <advc.opt>
	int* m_aiGlobalDefinesCache;
	int m_iEventMessageTime; // Cached separately b/c the EXE can access it before XML loading
	ImprovementTypes m_eRUINS_IMPROVEMENT;
	SpecialistTypes m_eDEFAULT_SPECIALIST;
	TerrainTypes m_aeWATER_TERRAIN[2]; // </advc.opt>

	float m_fCAMERA_MIN_YAW;
	float m_fCAMERA_MAX_YAW;
	float m_fCAMERA_FAR_CLIP_Z_HEIGHT;
	float m_fCAMERA_MAX_TRAVEL_DISTANCE;
	float m_fAIR_BOMB_HEIGHT;
	float m_fPLOT_SIZE;
	float m_fCAMERA_SPECIAL_PITCH;
	float m_fCAMERA_MAX_TURN_OFFSET;
	float m_fCAMERA_MIN_DISTANCE;
	float m_fCAMERA_UPPER_PITCH;
	float m_fCAMERA_LOWER_PITCH;
	float m_fSHADOW_SCALE;
	float m_fUNIT_MULTISELECT_DISTANCE;
	float m_fFIELD_OF_VIEW;
	float m_fCAMERA_START_DISTANCE;

	CvXMLLoadUtility* m_pXMLLoadUtility; // advc.003v

	CvDLLUtilityIFaceBase* m_pDLL;

	FProfiler* m_Profiler;
	CvString m_szDllProfileText;

private:
	// <advc.opt>
	void cacheGlobalInts(char const* szChangedDefine = NULL, int iNewValue = 0);
	void cacheGlobalFloats(/* advc.004m: */ bool bAllowRecursion = true); // </advc.opt>
	// advc.006:
	void handleUnknownTypeString(char const* szType, bool bHideAssert, bool bFromPython) const;
	//void addToInfosVectors(void* infoVector); // advc.enum (no longer used)
};

extern CvGlobals gGlobals;	// for debugging


// inlines ...

__forceinline CvGlobals& CvGlobals::getInstance()
{
	return gGlobals;
}
// advc:
__forceinline CvGlobals const& CvGlobals::getConstInstance()
{
	return gGlobals;
}

/*  <advc.enum> These aren't member functions because they need to overload the
	SET_ENUM_LENGTH_STATIC functions defined in CvEnums.h. I'd rather not make
	those functions members of CvGlobals. (Though that could make it easier for
	the compiler to remove unused functions. Hmm.) */
DO_FOR_EACH_DYN_INFO_TYPE(SET_ENUM_LENGTH)
// These two have a dynamic length (not known at compile time) but aren't in the DYN_INFO_TYPE list
inline WorldSizeTypes getEnumLength(WorldSizeTypes) { return (WorldSizeTypes)gGlobals.getNumWorldInfos(); }
inline FlavorTypes getEnumLength(FlavorTypes) { return (FlavorTypes)gGlobals.getNumFlavorTypes(); }
// </advc.enum>


// helpers ...

#define GC CvGlobals::getConstInstance() // advc: was ...getInstance()
#ifndef _USRDLL
#define gDLL GC.getDLLIFaceNonInl()
#else
#define gDLL GC.getDLLIFace()
#endif

#ifndef _USRDLL
#define NUM_DIRECTION_TYPES (GC.getNumDirections())
#define NUM_GAMEOPTION_TYPES (GC.getNumGameOptions())
#define NUM_MPOPTION_TYPES (GC.getNumMPOptions())
#define NUM_SPECIALOPTION_TYPES (GC.getNumSpecialOptions())
#define NUM_GRAPHICOPTION_TYPES (GC.getNumGraphicOptions())
#define NUM_TRADEABLE_ITEMS (GC.getNumTradeableItems())
#define NUM_BASIC_ITEMS (GC.getNumBasicItems())
#define NUM_TRADEABLE_HEADINGS (GC.getNumTradeableHeadings())
// advc: The EXE doesn't call these
/*#define NUM_COMMAND_TYPES (GC.getNumCommandInfos())
#define NUM_CONTROL_TYPES (GC.getNumControlInfos())
#define NUM_MISSION_TYPES (GC.getNumMissionInfos())*/
#define NUM_PLAYEROPTION_TYPES (GC.getNumPlayerOptionInfos())
#define MAX_NUM_SYMBOLS (GC.getMaxNumSymbols())
#define NUM_GRAPHICLEVELS (GC.getNumGraphicLevels())
#define NUM_GLOBE_LAYER_TYPES (GC.getNumGlobeLayers())
#endif
#pragma warning(pop) // advc: Re-enable "unknown pragma" warning

#endif
