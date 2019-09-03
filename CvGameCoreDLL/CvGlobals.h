#pragma once

// CvGlobals.h

#ifndef CIV4_GLOBALS_H
#define CIV4_GLOBALS_H
// advc.003t: Disable warnings about unknown pragma
#pragma warning(disable:4068)

// K-Mod. Created the following function for rounded integer division
static inline int ROUND_DIVIDE(int a, int b)
{
	return (a+((a/b>0)?1:-1)*(b/2)) / b;
}
// K-Mod end

//
// 'global' vars for Civ IV.  singleton class.
// All globals and global types should be contained in this class
//

class FProfiler;
class CvDLLUtilityIFaceBase;
class CvPythonCaller; // advc.003y
class CvDLLLogger; // advc.003t
class CvRandom;
class CvGameAI;
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
class CvCivicsScreen;
class CvWBUnitEditScreen;
class CvWBCityEditScreen;
class CMPDiplomacyScreen;
class FMPIManager;
class FAStar;
class CvInterface;
class CMainMenu;
class CvEngine;
class CvArtFileMgr;
class FVariableSystem;
class CvMap;
class CvPlayerAI;
class CvTeamAI;
class CvInterfaceModeInfo;
class CvWorldInfo;
class CvClimateInfo;
class CvSeaLevelInfo;
class CvColorInfo;
class CvPlayerColorInfo;
class CvAdvisorInfo;
class CvRouteModelInfo;
class CvRiverInfo;
class CvRiverModelInfo;
class CvWaterPlaneInfo;
class CvTerrainPlaneInfo;
class CvCameraOverlayInfo;
class CvAnimationPathInfo;
class CvAnimationCategoryInfo;
class CvEntityEventInfo;
class CvEffectInfo;
class CvAttachableInfo;
class CvCameraInfo;
class CvUnitFormationInfo;
class CvGameText;
class CvLandscapeInfo;
class CvTerrainInfo;
class CvBonusClassInfo;
class CvBonusInfo;
class CvFeatureInfo;
class CvCivilizationInfo;
class CvLeaderHeadInfo;
class CvTraitInfo;
class CvCursorInfo;
class CvThroneRoomCamera;
class CvThroneRoomInfo;
class CvThroneRoomStyleInfo;
class CvSlideShowInfo;
class CvSlideShowRandomInfo;
class CvWorldPickerInfo;
class CvSpaceShipInfo;
class CvUnitInfo;
class CvSpecialUnitInfo;
class CvInfoBase;
class CvYieldInfo;
class CvCommerceInfo;
class CvRouteInfo;
class CvImprovementInfo;
class CvGoodyInfo;
class CvBuildInfo;
class CvHandicapInfo;
class CvGameSpeedInfo;
class CvTurnTimerInfo;
class CvProcessInfo;
class CvVoteInfo;
class CvProjectInfo;
class CvBuildingClassInfo;
class CvBuildingInfo;
class CvSpecialBuildingInfo;
class CvUnitClassInfo;
class CvActionInfo;
class CvMissionInfo;
class CvControlInfo;
class CvCommandInfo;
class CvAutomateInfo;
class CvPromotionInfo;
class CvTechInfo;
class CvReligionInfo;
class CvCorporationInfo;
class CvSpecialistInfo;
class CvCivicOptionInfo;
class CvCivicInfo;
class CvDiplomacyInfo;
class CvEraInfo;
class CvHurryInfo;
class CvEmphasizeInfo;
class CvUpkeepInfo;
class CvCultureLevelInfo;
class CvVictoryInfo;
//class CvQuestInfo; // advc.003j
class CvGameOptionInfo;
class CvMPOptionInfo;
class CvForceControlInfo;
class CvPlayerOptionInfo;
class CvGraphicOptionInfo;
class CvTutorialInfo;
class CvEventTriggerInfo;
class CvEventInfo;
class CvEspionageMissionInfo;
class CvUnitArtStyleTypeInfo;
class CvVoteSourceInfo;
class CvMainMenuInfo;


class CvGlobals
{
//	friend class CvDLLUtilityIFace;
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
	__forceinline static CvGlobals const& getConstInstance(); // advc.003t

	CvGlobals();
	virtual ~CvGlobals(); // advc (comment) Probably has to stay virtual for the EXE

	DllExport void init();
	DllExport void uninit();
	void clearTypesMap();

	DllExport CvDiplomacyScreen* getDiplomacyScreen();
	DllExport CMPDiplomacyScreen* getMPDiplomacyScreen();

	DllExport FMPIManager*& getFMPMgrPtr();
	DllExport CvPortal& getPortal();
	DllExport CvSetupData& getSetupData();
	DllExport inline CvInitCore& getInitCore() { return *m_initCore; }
	inline CvInitCore& getInitCore() const { return *m_initCore; } // advc.003t: const replacement
	DllExport CvInitCore& getLoadedInitCore();
	DllExport CvInitCore& getIniInitCore();
	DllExport CvMessageCodeTranslator& getMessageCodes();
	DllExport CvStatsReporter& getStatsReporter();
	CvStatsReporter* getStatsReporterPtr();
	DllExport CvInterface& getInterface();
	DllExport CvInterface* getInterfacePtr();
	DllExport int getMaxCivPlayers() const;
	// inlined for perf reasons, do not use outside of dll  // advc.003f: Both renamed
	#ifdef _USRDLL
	CvMap& getMap() const { return *m_map; } // was getMapINLINE
	CvGameAI& getGame() const { return *m_game; } // was getGameINLINE
	#endif
	CvMap& getMapExternal(); // advc.003f: Exported through .def file
	CvGameAI& getGameExternal(); // advc.003f: Exported through .def file
	DllExport CvGameAI *getGamePointer();
	// <advc.003y>
	inline CvPythonCaller const* getPythonCaller() const
	{
		return m_pPythonCaller;
	} // </advc.003y>
	DllExport inline CvRandom& getASyncRand() { return *m_asyncRand; } // advc.003t: inline
	DllExport CMessageQueue& getMessageQueue();
	DllExport CMessageQueue& getHotMessageQueue();
	DllExport CMessageControl& getMessageControl();
	DllExport CvDropMgr& getDropMgr();
	/*  advc.003t: inlined and constified. The caller is certainly going to change
		the returned FAStar objects, but CvGlobals doesn't own those objects, so
		it shouldn't be our concern. */
	DllExport inline FAStar& getPathFinder() { CvGlobals const& kThis = *this; return kThis.getPathFinder(); }
	inline FAStar& getPathFinder() const { return *m_pathFinder; }
	DllExport inline FAStar& getInterfacePathFinder() { CvGlobals const& kThis = *this; return kThis.getInterfacePathFinder(); }
	inline FAStar& getInterfacePathFinder() const { return *m_interfacePathFinder; }
	DllExport inline FAStar& getStepFinder() { CvGlobals const& kThis = *this; return kThis.getStepFinder(); }
	inline FAStar& getStepFinder() const { return *m_stepFinder; }
	DllExport inline FAStar& getRouteFinder() { CvGlobals const& kThis = *this; return kThis.getRouteFinder(); }
	inline FAStar& getRouteFinder() const { return *m_routeFinder; }
	DllExport inline FAStar& getBorderFinder() { CvGlobals const& kThis = *this; return kThis.getBorderFinder(); }
	inline FAStar& getBorderFinder() const { return *m_borderFinder; }
	DllExport inline FAStar& getAreaFinder() { CvGlobals const& kThis = *this; return kThis.getAreaFinder(); }
	inline FAStar& getAreaFinder() const { return *m_areaFinder; }
	DllExport inline FAStar& getPlotGroupFinder() { CvGlobals const& kThis = *this; return kThis.getPlotGroupFinder(); }
	inline FAStar& getPlotGroupFinder() const { return *m_plotGroupFinder; }
	//NiPoint3& getPt3Origin(); // advc.003j: unused

	DllExport std::vector<CvInterfaceModeInfo*>& getInterfaceModeInfo();
	DllExport inline CvInterfaceModeInfo& getInterfaceModeInfo(InterfaceModeTypes e)
	// advc.003t: Need a const replacement
	{ CvGlobals const& kThis = *this; return kThis.getInterfaceModeInfo(e); }
	inline CvInterfaceModeInfo& getInterfaceModeInfo(InterfaceModeTypes e) const
	{
		FASSERT_BOUNDS(0, NUM_INTERFACEMODE_TYPES, e, "CvGlobals::getInterfaceModeInfo");
		return *(m_paInterfaceModeInfo[e]);
	}

	//NiPoint3& getPt3CameraDir(); // advc.003j: unused
	// advc.003t: inlined
	DllExport inline bool& getLogging() { return m_bLogging; }
	DllExport inline bool& getRandLogging() { return m_bRandLogging; }
	DllExport inline bool& getSynchLogging() { return m_bSynchLogging; }
	DllExport inline bool& overwriteLogs() { return m_bOverwriteLogs; }
	// <advc.003t> const versions of the above
	// The first two are exposed to Python for dlph.27
	inline bool isLogging() const { return m_bLogging; }
	inline bool isRandLogging() const { return m_bRandLogging; }
	inline bool isSynchLogging() const { return m_bSynchLogging; }
	inline bool isOverwriteLogs() const { return m_bOverwriteLogs; }
	// <advc.003t>
	inline CvDLLLogger& getLogger() const
	{
		return *m_pLogger;
	} // </advc.003t>

	// advc.003t: Inlined and constified
	DllExport inline int* getPlotDirectionX() { return m_aiPlotDirectionX; }
	inline int const* getPlotDirectionX() const { return m_aiPlotDirectionX; }
	DllExport inline int* getPlotDirectionY() { return m_aiPlotDirectionY; }
	inline int const* getPlotDirectionY() const { return m_aiPlotDirectionY; }
	DllExport inline int* getPlotCardinalDirectionX() { return m_aiPlotCardinalDirectionX; };
	inline int const* getPlotCardinalDirectionX() const { return m_aiPlotCardinalDirectionX; };
	DllExport inline int* getPlotCardinalDirectionY() { return m_aiPlotCardinalDirectionY; };
	inline int const* getPlotCardinalDirectionY() const { return m_aiPlotCardinalDirectionY; };
	DllExport inline DirectionTypes getXYDirection(int i, int j) { CvGlobals const& kThis = *this; return kThis.getXYDirection(i,j); }
	inline DirectionTypes getXYDirection(int i, int j) const
	{
		FASSERT_BOUNDS(0, DIRECTION_DIAMETER, i, "CvGlobals::getXYDirection");
		FASSERT_BOUNDS(0, DIRECTION_DIAMETER, j, "CvGlobals::getXYDirection");
		return m_aaeXYDirection[i][j];
	}
	inline int getXYCityPlot(int i, int j) const
	{
		FASSERT_BOUNDS(0, CITY_PLOTS_DIAMETER, i, "CvGlobals::getXYCityPlot");
		FASSERT_BOUNDS(0, CITY_PLOTS_DIAMETER, j, "CvGlobals::getXYCityPlot");
		return m_aaiXYCityPlot[i][j];
	}
	inline int const* getCityPlotX() const { return m_aiCityPlotX; }
	inline int const* getCityPlotY() const { return m_aiCityPlotY; }
	inline int const* CvGlobals::getCityPlotPriority() const { return m_aiCityPlotPriority; }
	inline DirectionTypes const* getTurnLeftDirection() const { return m_aeTurnLeftDirection; }
	inline DirectionTypes getTurnLeftDirection(int i) const
	{
		FASSERT_BOUNDS(0, NUM_DIRECTION_TYPES, i, "CvGlobals::getTurnLeftDirection");
		return m_aeTurnLeftDirection[i];
	}
	inline DirectionTypes const* getTurnRightDirection() const { return m_aeTurnRightDirection; }
	inline DirectionTypes getTurnRightDirection(int i) const
	{
		FASSERT_BOUNDS(0, NUM_DIRECTION_TYPES, i, "CvGlobals::getTurnRightDirection");
		return m_aeTurnRightDirection[i];
	}

	//
	// Global Infos
	// All info type strings are upper case and are kept in this hash map for fast lookup
	//
	DllExport int getInfoTypeForString(const char* szType, bool bHideAssert = false) const;			// returns the infos index, use this when searching for an info type string
	void setInfoTypeFromString(const char* szType, int idx);
	DllExport void infoTypeFromStringReset();
	void addToInfosVectors(void *infoVector);
	DllExport void infosReset();
// <advc.003t> All inlined, constified
#pragma region InfoAccessors
	DllExport int getNumWorldInfos() { CvGlobals const& kThis = *this; return kThis.getNumWorldInfos(); }
	inline int getNumWorldInfos() const
	{
		return (int)m_paWorldInfo.size();
	}
	DllExport CvWorldInfo& getWorldInfo(WorldSizeTypes eWorld) { CvGlobals const& kThis = *this; return kThis.getWorldInfo(eWorld); }
	inline CvWorldInfo& getWorldInfo(WorldSizeTypes eWorld) const
	{
		FASSERT_BOUNDS(0, getNumWorldInfos(), eWorld, "CvGlobals::getWorldInfo");
		return *m_paWorldInfo[eWorld];
	}
	DllExport int getNumClimateInfos() { CvGlobals const& kThis = *this; return kThis.getNumClimateInfos(); }
	inline int getNumClimateInfos() const
	{
		return (int)m_paClimateInfo.size();
	}
	DllExport CvClimateInfo& getClimateInfo(ClimateTypes eClimate) { CvGlobals const& kThis = *this; return kThis.getClimateInfo(eClimate); }
	inline CvClimateInfo& getClimateInfo(ClimateTypes eClimate) const
	{
		FASSERT_BOUNDS(0, getNumClimateInfos(), eClimate, "CvGlobals::getClimateInfo");
		return *m_paClimateInfo[eClimate];
	}
	DllExport int getNumSeaLevelInfos() { CvGlobals const& kThis = *this; return kThis.getNumSeaLevelInfos(); }
	inline int getNumSeaLevelInfos() const
	{
		return (int)m_paSeaLevelInfo.size();
	}
	DllExport CvSeaLevelInfo& getSeaLevelInfo(SeaLevelTypes eSeaLevel) { CvGlobals const& kThis = *this; return kThis.getSeaLevelInfo(eSeaLevel); }
	inline CvSeaLevelInfo& getSeaLevelInfo(SeaLevelTypes eSeaLevel) const
	{
		FASSERT_BOUNDS(0, getNumSeaLevelInfos(), eSeaLevel, "CvGlobals::getSeaLevelInfo");
		return *m_paSeaLevelInfo[eSeaLevel];
	}
	inline int getNumColorInfos() const
	{
		return (int)m_paColorInfo.size();
	}
	DllExport CvColorInfo& getColorInfo(ColorTypes eColor) { CvGlobals const& kThis = *this; return kThis.getColorInfo(eColor); }
	CvColorInfo& getColorInfo(ColorTypes eColor) const; // advc.106i: not inlined
	DllExport int getNumPlayerColorInfos() { CvGlobals const& kThis = *this; return kThis.getNumPlayerColorInfos(); }
	inline int getNumPlayerColorInfos() const
	{
		return (int)m_paPlayerColorInfo.size();
	}
	DllExport CvPlayerColorInfo& getPlayerColorInfo(PlayerColorTypes ePlayerColor) { CvGlobals const& kThis = *this; return kThis.getPlayerColorInfo(ePlayerColor); }
	inline CvPlayerColorInfo& getPlayerColorInfo(PlayerColorTypes ePlayerColor) const
	{
		FASSERT_BOUNDS(0, getNumPlayerColorInfos(), ePlayerColor, "CvGlobals::getPlayerColorInfo");
		return *m_paPlayerColorInfo[ePlayerColor];
	}
	inline int getNumAdvisorInfos() const
	{
		return (int)m_paAdvisorInfo.size();
	}
	inline CvAdvisorInfo& getAdvisorInfo(AdvisorTypes eAdvisor) const
	{
		return *m_paAdvisorInfo[eAdvisor];
	}
	DllExport int getNumHints() { CvGlobals const& kThis = *this; return kThis.getNumHints(); }
	inline int getNumHints() const
	{
		return (int)m_paHints.size();
	}
	DllExport CvInfoBase& getHints(int iHint) { CvGlobals const& kThis = *this; return kThis.getHints(iHint); }
	inline CvInfoBase& getHints(int iHint) const
	{
		FASSERT_BOUNDS(0, getNumHints(), iHint, "CvGlobals::getHints");
		return *m_paHints[iHint];
	}
	inline int getNumMainMenus() const
	{
		return (int)m_paMainMenus.size();
	}
	DllExport CvMainMenuInfo& getMainMenus(int iMainMenu) { CvGlobals const& kThis = *this; return kThis.getMainMenus(iMainMenu); }
	inline CvMainMenuInfo& getMainMenus(int iMainMenu) const
	{
		FASSERT_BOUNDS(0, getNumMainMenus(), iMainMenu, "CvGlobals::getMainMenus");
		return *m_paMainMenus[iMainMenu];
	}
	DllExport int getNumRouteModelInfos() { CvGlobals const& kThis = *this; return kThis.getNumRouteModelInfos(); }
	inline int getNumRouteModelInfos() const
	{
		return (int)m_paRouteModelInfo.size();
	}
	DllExport CvRouteModelInfo& getRouteModelInfo(int iRouteModel) { CvGlobals const& kThis = *this; return kThis.getRouteModelInfo(iRouteModel); }
	inline CvRouteModelInfo& getRouteModelInfo(int iRouteModel) const
	{
		FASSERT_BOUNDS(0, getNumRouteModelInfos(), iRouteModel, "CvGlobals::getRouteModelInfo");
		return *m_paRouteModelInfo[iRouteModel];
	}
	inline int getNumRiverInfos() const
	{
		return (int)m_paRiverInfo.size();
	}
	inline CvRiverInfo& getRiverInfo(RiverTypes eRiver) const
	{
		FASSERT_BOUNDS(0, getNumRiverInfos(), eRiver, "CvGlobals::getRiverInfo");
		return *m_paRiverInfo[eRiver];
	}
	DllExport int getNumRiverModelInfos() { CvGlobals const& kThis = *this; return kThis.getNumRiverModelInfos(); }
	inline int getNumRiverModelInfos() const
	{
		return (int)m_paRiverModelInfo.size();
	}
	DllExport CvRiverModelInfo& getRiverModelInfo(int iRiverModel) { CvGlobals const& kThis = *this; return kThis.getRiverModelInfo(iRiverModel); }
	inline CvRiverModelInfo& getRiverModelInfo(int iRiverModel) const
	{
		FASSERT_BOUNDS(0, getNumRiverModelInfos(), iRiverModel, "CvGlobals::getRiverModelInfo");
		return *m_paRiverModelInfo[iRiverModel];
	}
	inline int getNumWaterPlaneInfos() const
	{
		return (int)m_paWaterPlaneInfo.size();
	}
	DllExport CvWaterPlaneInfo& getWaterPlaneInfo(int iWaterPlane) { CvGlobals const& kThis = *this; return kThis.getWaterPlaneInfo(iWaterPlane); }
	inline CvWaterPlaneInfo& getWaterPlaneInfo(int iWaterPlane) const
	{
		FASSERT_BOUNDS(0, getNumWaterPlaneInfos(), iWaterPlane, "CvGlobals::getWaterPlaneInfo");
		return *m_paWaterPlaneInfo[iWaterPlane];
	}
	DllExport int getNumTerrainPlaneInfos() { CvGlobals const& kThis = *this; return kThis.getNumTerrainPlaneInfos(); }
	inline int getNumTerrainPlaneInfos() const
	{
		return (int)m_paTerrainPlaneInfo.size();
	}
	DllExport CvTerrainPlaneInfo& getTerrainPlaneInfo(int iTerrainPlane) { CvGlobals const& kThis = *this; return kThis.getTerrainPlaneInfo(iTerrainPlane); }
	inline CvTerrainPlaneInfo& getTerrainPlaneInfo(int iTerrainPlane) const
	{
		FASSERT_BOUNDS(0, getNumTerrainPlaneInfos(), iTerrainPlane, "CvGlobals::getTerrainPlaneInfo");
		return *m_paTerrainPlaneInfo[iTerrainPlane];
	}
	DllExport int getNumCameraOverlayInfos() { CvGlobals const& kThis = *this; return kThis.getNumCameraOverlayInfos(); }
	inline int getNumCameraOverlayInfos() const
	{
		return (int)m_paCameraOverlayInfo.size();
	}
	DllExport CvCameraOverlayInfo& getCameraOverlayInfo(int iCameraOverlay) { CvGlobals const& kThis = *this; return kThis.getCameraOverlayInfo((CameraOverlayTypes)iCameraOverlay); }
	inline CvCameraOverlayInfo& getCameraOverlayInfo(CameraOverlayTypes eCameraOverlay) const
	{
		FASSERT_BOUNDS(0, getNumCameraOverlayInfos(), eCameraOverlay, "CvGlobals::getCameraOverlayInfo");
		return *m_paCameraOverlayInfo[eCameraOverlay];
	}
	inline int getNumAnimationPathInfos() const
	{
		return (int)m_paAnimationPathInfo.size();
	}
	DllExport CvAnimationPathInfo& getAnimationPathInfo(AnimationPathTypes eAnimationPath) { CvGlobals const& kThis = *this; return kThis.getAnimationPathInfo(eAnimationPath); }
	inline CvAnimationPathInfo& getAnimationPathInfo(AnimationPathTypes eAnimationPath) const
	{
		FASSERT_BOUNDS(0, getNumAnimationPathInfos(), eAnimationPath, "CvGlobals::getAnimationPathInfo");
		return *m_paAnimationPathInfo[eAnimationPath];
	}
	inline int getNumAnimationCategoryInfos() const
	{
		return (int)m_paAnimationCategoryInfo.size();
	}
	DllExport CvAnimationCategoryInfo& getAnimationCategoryInfo(AnimationCategoryTypes eAnimationCategory) { CvGlobals const& kThis = *this; return kThis.getAnimationCategoryInfo(eAnimationCategory); }
	inline CvAnimationCategoryInfo& getAnimationCategoryInfo(AnimationCategoryTypes eAnimationCategory) const
	{
		FASSERT_BOUNDS(0, getNumAnimationCategoryInfos(), eAnimationCategory, "CvGlobals::getAnimationCategoryInfo");
		return *m_paAnimationCategoryInfo[eAnimationCategory];
	}
	inline int getNumEntityEventInfos() const
	{
		return (int)m_paEntityEventInfo.size();
	}
	DllExport CvEntityEventInfo& getEntityEventInfo(EntityEventTypes e) { CvGlobals const& kThis = *this; return kThis.getEntityEventInfo(e); }
	inline CvEntityEventInfo& getEntityEventInfo(EntityEventTypes eEntityEvent) const
	{
		FASSERT_BOUNDS(0, getNumEntityEventInfos(), eEntityEvent, "CvGlobals::getEntityEventInfo");
		return *m_paEntityEventInfo[eEntityEvent];
	}
	inline int getNumEffectInfos() const
	{
		return (int)m_paEffectInfo.size();
	}
	DllExport CvEffectInfo& getEffectInfo(int iEffect) { CvGlobals const& kThis = *this; return kThis.getEffectInfo((EffectTypes)iEffect); }
	inline CvEffectInfo& getEffectInfo(EffectTypes eEffect) const
	{
		FASSERT_BOUNDS(0, getNumEffectInfos(), eEffect, "CvGlobals::getEffectInfo");
		return *m_paEffectInfo[eEffect];
	}
	inline int getNumAttachableInfos() const
	{
		return (int)m_paAttachableInfo.size();
	}
	DllExport CvAttachableInfo& getAttachableInfo(int iAttachable) { CvGlobals const& kThis = *this; return kThis.getAttachableInfo((AttachableTypes)iAttachable); }
	inline CvAttachableInfo& getAttachableInfo(AttachableTypes eAttachable) const
	{
		FASSERT_BOUNDS(0, getNumAttachableInfos(), eAttachable, "CvGlobals::getAttachableInfo");
		return *m_paAttachableInfo[eAttachable];
	}
	inline int getNumCameraInfos() const
	{
		return (int)m_paCameraInfo.size();
	}
	inline CvCameraInfo& getCameraInfo(CameraAnimationTypes eCameraAnimation) const
	{
		FASSERT_BOUNDS(0, getNumCameraInfos(), eCameraAnimation, "CvGlobals::getCameraInfo");
		return *m_paCameraInfo[eCameraAnimation];
	}
	DllExport int getNumUnitFormationInfos() { CvGlobals const& kThis = *this; return kThis.getNumUnitFormationInfos(); }
	inline int getNumUnitFormationInfos() const
	{
		return (int)m_paUnitFormationInfo.size();
	}
	DllExport CvUnitFormationInfo& getUnitFormationInfo(int iUnitFormation) { CvGlobals const& kThis = *this; return kThis.getUnitFormationInfo(iUnitFormation); }
	inline CvUnitFormationInfo& getUnitFormationInfo(int iUnitFormation) const
	{
		FASSERT_BOUNDS(0, getNumUnitFormationInfos(), iUnitFormation, "CvGlobals::getUnitFormationInfo");
		return *m_paUnitFormationInfo[iUnitFormation];
	}
	inline int getNumGameTextXML() const
	{
		return (int)m_paGameTextXML.size();
	}
	inline int getNumLandscapeInfos() const
	{
		return (int)m_paLandscapeInfo.size();
	}
	DllExport CvLandscapeInfo& getLandscapeInfo(int iLandscape) { CvGlobals const& kThis = *this; return kThis.getLandscapeInfo(iLandscape); }
	inline CvLandscapeInfo& getLandscapeInfo(int iLandscape) const
	{
		FASSERT_BOUNDS(0, getNumLandscapeInfos(), iLandscape, "CvGlobals::getLandscapeInfo");
		return *m_paLandscapeInfo[iLandscape];
	}
	DllExport inline int getActiveLandscapeID() { CvGlobals const& kThis = *this; return kThis.getActiveLandscapeID(); }
	inline int getActiveLandscapeID() const { return m_iActiveLandscapeID; } // advc.003t: const version
	DllExport void setActiveLandscapeID(int iLandscapeID);

	// <advc.003x> So that CvMap doesn't have to use CvLandscapeInfo directly
	int getLandscapePlotsPerCellX() const;
	int getLandscapePlotsPerCellY() const; // </advc.003x>

	DllExport int getNumTerrainInfos() { CvGlobals const& kThis = *this; return kThis.getNumTerrainInfos(); }
	inline int getNumTerrainInfos() const
	{
		return (int)m_paTerrainInfo.size();
	}
	DllExport CvTerrainInfo& getTerrainInfo(TerrainTypes eTerrain) { CvGlobals const& kThis = *this; return kThis.getTerrainInfo(eTerrain); }
	inline CvTerrainInfo& getTerrainInfo(TerrainTypes eTerrain) const
	{
		FASSERT_BOUNDS(0, getNumTerrainInfos(), eTerrain, "CvGlobals::getTerrainInfo");
		return *m_paTerrainInfo[eTerrain];
	}
	inline int getNumBonusClassInfos() const
	{
		return (int)m_paBonusClassInfo.size();
	}
	inline CvBonusClassInfo& getBonusClassInfo(BonusClassTypes eBonusClass) const
	{
		FASSERT_BOUNDS(0, getNumBonusClassInfos(), eBonusClass, "CvGlobals::getBonusClassInfo");
		return *m_paBonusClassInfo[eBonusClass];
	}
	DllExport int getNumBonusInfos() { CvGlobals const& kThis = *this; return kThis.getNumBonusInfos(); }
	inline int getNumBonusInfos() const
	{
		return (int)m_paBonusInfo.size();
	}
	DllExport CvBonusInfo& getBonusInfo(BonusTypes eBonus) { CvGlobals const& kThis = *this; return kThis.getBonusInfo(eBonus); }
	inline CvBonusInfo& getBonusInfo(BonusTypes eBonus) const
	{
		FASSERT_BOUNDS(0, getNumBonusInfos(), eBonus, "CvGlobals::getBonusInfo");
		return *m_paBonusInfo[eBonus];
	}
	DllExport int getNumFeatureInfos() { CvGlobals const& kThis = *this; return kThis.getNumFeatureInfos(); }
	inline int getNumFeatureInfos() const
	{
		return (int)m_paFeatureInfo.size();
	}
	DllExport CvFeatureInfo& getFeatureInfo(FeatureTypes eFeature) { CvGlobals const& kThis = *this; return kThis.getFeatureInfo(eFeature); }
	inline CvFeatureInfo& getFeatureInfo(FeatureTypes eFeature) const
	{
		FASSERT_BOUNDS(0, getNumFeatureInfos(), eFeature, "CvGlobals::getFeatureInfo");
		return *m_paFeatureInfo[eFeature];
	}
	DllExport int& getNumPlayableCivilizationInfos();
	DllExport int& getNumAIPlayableCivilizationInfos();
	DllExport int getNumCivilizationInfos() { CvGlobals const& kThis = *this; return kThis.getNumCivilizationInfos(); }
	inline int getNumCivilizationInfos() const
	{
		return (int)m_paCivilizationInfo.size();
	}
	DllExport CvCivilizationInfo& getCivilizationInfo(CivilizationTypes eCivilization) { CvGlobals const& kThis = *this; return kThis.getCivilizationInfo(eCivilization); }
	inline CvCivilizationInfo& getCivilizationInfo(CivilizationTypes eCivilization) const
	{
		FASSERT_BOUNDS(0, getNumCivilizationInfos(), eCivilization, "CvGlobals::getCivilizationInfo");
		return *m_paCivilizationInfo[eCivilization];
	}
	DllExport int getNumLeaderHeadInfos() { CvGlobals const& kThis = *this; return kThis.getNumLeaderHeadInfos(); }
	inline int getNumLeaderHeadInfos() const
	{
		return (int)m_paLeaderHeadInfo.size();
	}
	DllExport CvLeaderHeadInfo& getLeaderHeadInfo(LeaderHeadTypes eLeaderHead) { CvGlobals const& kThis = *this; return kThis.getLeaderHeadInfo(eLeaderHead); }
	inline CvLeaderHeadInfo& getLeaderHeadInfo(LeaderHeadTypes eLeaderHead) const
	{
		FASSERT_BOUNDS(0, getNumLeaderHeadInfos(), eLeaderHead, "CvGlobals::getLeaderHeadInfo");
		return *m_paLeaderHeadInfo[eLeaderHead];
	}
	inline int getNumTraitInfos() const
	{
		return (int)m_paTraitInfo.size();
	}
	inline CvTraitInfo& getTraitInfo(TraitTypes eTrait) const
	{
		FASSERT_BOUNDS(0, getNumTraitInfos(), eTrait, "CvGlobals::getTraitInfo");
		return *m_paTraitInfo[eTrait];
	}
	DllExport int getNumCursorInfos() { CvGlobals const& kThis = *this; return kThis.getNumCursorInfos(); }
	inline int getNumCursorInfos() const
	{
		return (int)m_paCursorInfo.size();
	}
	DllExport CvCursorInfo& getCursorInfo(CursorTypes eCursor) { CvGlobals const& kThis = *this; return kThis.getCursorInfo(eCursor); }
	inline CvCursorInfo& getCursorInfo(CursorTypes eCursor) const
	{
		FASSERT_BOUNDS(0, getNumCursorInfos(), eCursor, "CvGlobals::getCursorInfo");
		return *m_paCursorInfo[eCursor];
	}
	inline int getNumThroneRoomCameras() const
	{
		return (int)m_paThroneRoomCamera.size();
	}
	DllExport CvThroneRoomCamera& getThroneRoomCamera(int iThroneRoomCamera) { CvGlobals const& kThis = *this; return kThis.getThroneRoomCamera(iThroneRoomCamera); }
	inline CvThroneRoomCamera& getThroneRoomCamera(int iThroneRoomCamera) const
	{
		FASSERT_BOUNDS(0, getNumThroneRoomCameras(), iThroneRoomCamera, "CvGlobals::getThroneRoomCamera");
		return *m_paThroneRoomCamera[iThroneRoomCamera];
	}
	DllExport int getNumThroneRoomInfos(); // advc.003v: No inline definition; needs to do some extra work now.
	inline int getNumThroneRoomInfos() const
	{
		return (int)m_paThroneRoomInfo.size();
	}
	DllExport CvThroneRoomInfo& getThroneRoomInfo(int iThroneRoom) { CvGlobals const& kThis = *this; return kThis.getThroneRoomInfo(iThroneRoom); }
	inline CvThroneRoomInfo& getThroneRoomInfo(int iThroneRoom) const
	{
		FASSERT_BOUNDS(0, getNumThroneRoomInfos(), iThroneRoom, "CvGlobals::getThroneRoomInfo");
		return *m_paThroneRoomInfo[iThroneRoom];
	}
	DllExport int getNumThroneRoomStyleInfos() { CvGlobals const& kThis = *this; return kThis.getNumThroneRoomStyleInfos(); }
	inline int getNumThroneRoomStyleInfos() const
	{
		return (int)m_paThroneRoomStyleInfo.size();
	}
	DllExport CvThroneRoomStyleInfo& getThroneRoomStyleInfo(int iThroneRoomStyle) { CvGlobals const& kThis = *this; return kThis.getThroneRoomStyleInfo(iThroneRoomStyle); }
	inline CvThroneRoomStyleInfo& getThroneRoomStyleInfo(int iThroneRoomStyle) const
	{
		FASSERT_BOUNDS(0, getNumThroneRoomStyleInfos(), iThroneRoomStyle, "CvGlobals::getThroneRoomStyleInfo");
		return *m_paThroneRoomStyleInfo[iThroneRoomStyle];
	}
	DllExport int getNumSlideShowInfos() { CvGlobals const& kThis = *this; return kThis.getNumSlideShowInfos(); }
	inline int getNumSlideShowInfos() const
	{
		return (int)m_paSlideShowInfo.size();
	}
	DllExport CvSlideShowInfo& getSlideShowInfo(int iSlideShow) { CvGlobals const& kThis = *this; return kThis.getSlideShowInfo(iSlideShow); }
	inline CvSlideShowInfo& getSlideShowInfo(int iSlideShow) const
	{
		FASSERT_BOUNDS(0, getNumSlideShowInfos(), iSlideShow, "CvGlobals::getSlideShowInfo");
		return *m_paSlideShowInfo[iSlideShow];
	}
	DllExport int getNumSlideShowRandomInfos() { CvGlobals const& kThis = *this; return kThis.getNumSlideShowRandomInfos(); }
	inline int getNumSlideShowRandomInfos() const
	{
		return (int)m_paSlideShowRandomInfo.size();
	}
	DllExport CvSlideShowRandomInfo& getSlideShowRandomInfo(int iSlideShowRandom) { CvGlobals const& kThis = *this; return kThis.getSlideShowRandomInfo(iSlideShowRandom); }
	inline CvSlideShowRandomInfo& getSlideShowRandomInfo(int iSlideShowRandom) const
	{
		FASSERT_BOUNDS(0, getNumSlideShowRandomInfos(), iSlideShowRandom, "CvGlobals::getSlideShowRandomInfo");
		return *m_paSlideShowRandomInfo[iSlideShowRandom];
	}
	DllExport int getNumWorldPickerInfos() { CvGlobals const& kThis = *this; return kThis.getNumWorldPickerInfos(); }
	inline int getNumWorldPickerInfos() const
	{
		return (int)m_paWorldPickerInfo.size();
	}
	DllExport CvWorldPickerInfo& getWorldPickerInfo(int iWorldPicker) { CvGlobals const& kThis = *this; return kThis.getWorldPickerInfo(iWorldPicker); }
	inline CvWorldPickerInfo& getWorldPickerInfo(int iWorldPicker) const
	{
		FASSERT_BOUNDS(0, getNumWorldPickerInfos(), iWorldPicker, "CvGlobals::getWorldPickerInfo");
		return *m_paWorldPickerInfo[iWorldPicker];
	}
	DllExport int getNumSpaceShipInfos() { CvGlobals const& kThis = *this; return kThis.getNumSpaceShipInfos(); }
	inline int getNumSpaceShipInfos() const
	{
		return (int)m_paSpaceShipInfo.size();
	}
	DllExport CvSpaceShipInfo& getSpaceShipInfo(int iSpaceShip) { CvGlobals const& kThis = *this; return kThis.getSpaceShipInfo(iSpaceShip); }
	inline CvSpaceShipInfo& getSpaceShipInfo(int iSpaceShip) const
	{
		FASSERT_BOUNDS(0, getNumSpaceShipInfos(), iSpaceShip, "CvGlobals::getSpaceShipInfo");
		return *m_paSpaceShipInfo[iSpaceShip];
	}
	inline int getNumUnitInfos() const
	{
		return (int)m_paUnitInfo.size();
	}
	inline CvUnitInfo& getUnitInfo(UnitTypes eUnit) const
	{
		FASSERT_BOUNDS(0, getNumUnitInfos(), eUnit, "CvGlobals::getUnitInfo");
		return *m_paUnitInfo[eUnit];
	}
	inline int getNumSpecialUnitInfos() const
	{
		return (int)m_paSpecialUnitInfo.size();
	}
	inline CvSpecialUnitInfo& getSpecialUnitInfo(SpecialUnitTypes eSpecialUnit) const
	{
		FASSERT_BOUNDS(0, getNumSpecialUnitInfos(), eSpecialUnit, "CvGlobals::getSpecialUnitInfo");
		return *m_paSpecialUnitInfo[eSpecialUnit];
	}
	inline int getNumConceptInfos() const
	{
		return (int)m_paConceptInfo.size();
	}
	inline CvInfoBase& getConceptInfo(ConceptTypes eConcept) const
	{
		FASSERT_BOUNDS(0, getNumConceptInfos(), eConcept, "CvGlobals::getConceptInfo");
		return *m_paConceptInfo[eConcept];
	}
	inline int getNumNewConceptInfos() const
	{
		return (int)m_paNewConceptInfo.size();
	}
	inline CvInfoBase& getNewConceptInfo(NewConceptTypes eNewConcept) const
	{
		FASSERT_BOUNDS(0, getNumNewConceptInfos(), eNewConcept, "CvGlobals::getNewConceptInfo");
		return *m_paNewConceptInfo[eNewConcept];
	}
	inline int getNumCityTabInfos() const
	{
		return (int)m_paCityTabInfo.size();
	}
	inline CvInfoBase& getCityTabInfo(CityTabTypes eCityTab) const
	{
		FASSERT_BOUNDS(0, getNumCityTabInfos(), eCityTab, "CvGlobals::getCityTabInfo");
		return *m_paCityTabInfo[eCityTab];
	}
	inline int getNumCalendarInfos() const
	{
		return (int)m_paCalendarInfo.size();
	}
	inline CvInfoBase& getCalendarInfo(CalendarTypes eCalendar) const
	{
		FASSERT_BOUNDS(0, getNumCalendarInfos(), eCalendar, "CvGlobals::getCalendarInfo");
		return *m_paCalendarInfo[eCalendar];
	}
	inline int getNumSeasonInfos() const
	{
		return (int)m_paSeasonInfo.size();
	}
	inline CvInfoBase& getSeasonInfo(SeasonTypes eSeason) const
	{
		FASSERT_BOUNDS(0, getNumSeasonInfos(), eSeason, "CvGlobals::getSeasonInfo");
		return *m_paSeasonInfo[eSeason];
	}
	inline int getNumMonthInfos() const
	{
		return (int)m_paMonthInfo.size();
	}
	inline CvInfoBase& getMonthInfo(MonthTypes eMonth) const
	{
		FASSERT_BOUNDS(0, getNumMonthInfos(), eMonth, "CvGlobals::getMonthInfo");
		return *m_paMonthInfo[eMonth];
	}
	inline int getNumDenialInfos() const
	{
		return (int)m_paDenialInfo.size();
	}
	inline CvInfoBase& getDenialInfo(DenialTypes eDenial) const
	{
		FASSERT_BOUNDS(0, getNumDenialInfos(), eDenial, "CvGlobals::getDenialInfo");
		return *m_paDenialInfo[eDenial];
	}
	inline int getNumInvisibleInfos() const
	{
		return (int)m_paInvisibleInfo.size();
	}
	inline CvInfoBase& getInvisibleInfo(InvisibleTypes eInvisible) const
	{
		FASSERT_BOUNDS(0, getNumInvisibleInfos(), eInvisible, "CvGlobals::getInvisibleInfo");
		return *m_paInvisibleInfo[eInvisible];
	}
	inline int getNumVoteSourceInfos() const
	{
		return (int)m_paVoteSourceInfo.size();
	}
	inline CvVoteSourceInfo& getVoteSourceInfo(VoteSourceTypes eVoteSource) const
	{
		FASSERT_BOUNDS(0, getNumVoteSourceInfos(), eVoteSource, "CvGlobals::getVoteSourceInfo");
		return *m_paVoteSourceInfo[eVoteSource];
	}
	inline int getNumUnitCombatInfos() const
	{
		return (int)m_paUnitCombatInfo.size();
	}
	inline CvInfoBase& getUnitCombatInfo(UnitCombatTypes eUnitCombat) const
	{
		FASSERT_BOUNDS(0, getNumUnitCombatInfos(), eUnitCombat, "CvGlobals::getUnitCombatInfo");
		return *m_paUnitCombatInfo[eUnitCombat];
	}
	inline CvInfoBase& getDomainInfo(DomainTypes eDomain) const
	{
		FASSERT_BOUNDS(0, NUM_DOMAIN_TYPES, eDomain, "CvGlobals::getDomainInfo");
		return *m_paDomainInfo[eDomain];
	}
	inline CvInfoBase& getUnitAIInfo(UnitAITypes eUnitAI) const
	{
		FASSERT_BOUNDS(0, NUM_UNITAI_TYPES, eUnitAI, "CvGlobals::getUnitAIInfo");
		return *m_paUnitAIInfo[eUnitAI];
	}
	inline CvInfoBase& getAttitudeInfo(AttitudeTypes eAttitude) const
	{
		FASSERT_BOUNDS(0, NUM_ATTITUDE_TYPES, eAttitude, "CvGlobals::getAttitudeInfo");
		return *m_paAttitudeInfo[eAttitude];
	}
	inline CvInfoBase& getMemoryInfo(MemoryTypes eMemory) const
	{
		FASSERT_BOUNDS(0, NUM_MEMORY_TYPES, eMemory, "CvGlobals::getMemoryInfo");
		return *m_paMemoryInfo[eMemory];
	}
	inline int getNumGameOptionInfos() const
	{
		return (int)m_paGameOptionInfo.size();
	}
	DllExport CvGameOptionInfo& getGameOptionInfo(GameOptionTypes eGameOption) { CvGlobals const& kThis = *this; return kThis.getGameOptionInfo(eGameOption); }
	inline CvGameOptionInfo& getGameOptionInfo(GameOptionTypes eGameOption) const
	{
		FASSERT_BOUNDS(0, NUM_GAMEOPTION_TYPES, eGameOption, "CvGlobals::getGameOptionInfo");
		return *m_paGameOptionInfo[eGameOption];
	}
	inline int getNumMPOptionInfos() const
	{
		return (int)m_paMPOptionInfo.size();
	}
	DllExport CvMPOptionInfo& getMPOptionInfo(MultiplayerOptionTypes eMPOption) { CvGlobals const& kThis = *this; return kThis.getMPOptionInfo(eMPOption); }
	inline CvMPOptionInfo& getMPOptionInfo(MultiplayerOptionTypes eMPOption) const
	{
		FASSERT_BOUNDS(0, NUM_MPOPTION_TYPES, eMPOption, "CvGlobals::getMPOptionInfo");
		return *m_paMPOptionInfo[eMPOption];
	}
	inline int getNumForceControlInfos() const
	{
		return (int)m_paForceControlInfo.size();
	}
	DllExport CvForceControlInfo& getForceControlInfo(ForceControlTypes eForceControl) { CvGlobals const& kThis = *this; return kThis.getForceControlInfo(eForceControl); }
	inline CvForceControlInfo& getForceControlInfo(ForceControlTypes eForceControl) const
	{
		FASSERT_BOUNDS(0, getNumForceControlInfos(), eForceControl, "CvGlobals::getForceControlInfo");
		return *m_paForceControlInfo[eForceControl];
	}
	DllExport CvPlayerOptionInfo& getPlayerOptionInfo(PlayerOptionTypes ePlayerOption) { CvGlobals const& kThis = *this; return kThis.getPlayerOptionInfo(ePlayerOption); }
	inline CvPlayerOptionInfo& getPlayerOptionInfo(PlayerOptionTypes ePlayerOption) const
	{
		FASSERT_BOUNDS(0, getNumPlayerOptionInfos(), ePlayerOption, "CvGlobals::getPlayerOptionInfo");
		return *m_paPlayerOptionInfo[ePlayerOption];
	}
	DllExport CvGraphicOptionInfo& getGraphicOptionInfo(GraphicOptionTypes eGraphicOption) { CvGlobals const& kThis = *this; return kThis.getGraphicOptionInfo(eGraphicOption); }
	inline CvGraphicOptionInfo& getGraphicOptionInfo(GraphicOptionTypes eGraphicOption) const
	{
		FASSERT_BOUNDS(0, NUM_GRAPHICOPTION_TYPES, eGraphicOption, "CvGlobals::getGraphicOptionInfo");
		return *m_paGraphicOptionInfo[eGraphicOption];
	}
	inline CvYieldInfo& getYieldInfo(YieldTypes eYield) const
	{
		FASSERT_BOUNDS(0, NUM_YIELD_TYPES, eYield, "CvGlobals::getYieldInfo");
		return *m_paYieldInfo[eYield];
	}
	inline CvCommerceInfo& getCommerceInfo(CommerceTypes eCommerce) const
	{
		FASSERT_BOUNDS(0, NUM_COMMERCE_TYPES, eCommerce, "CvGlobals::getCommerceInfo");
		return *m_paCommerceInfo[eCommerce];
	}
	DllExport int getNumRouteInfos() { CvGlobals const& kThis = *this; return kThis.getNumRouteInfos(); }
	inline int getNumRouteInfos() const
	{
		return (int)m_paRouteInfo.size();
	}
	inline CvRouteInfo& getRouteInfo(RouteTypes eRoute) const
	{
		FASSERT_BOUNDS(0, getNumRouteInfos(), eRoute, "CvGlobals::getRouteInfo");
		return *m_paRouteInfo[eRoute];
	}
	DllExport int getNumImprovementInfos() { CvGlobals const& kThis = *this; return kThis.getNumImprovementInfos(); }
	inline int getNumImprovementInfos() const
	{
		return (int)m_paImprovementInfo.size();
	}
	DllExport CvImprovementInfo& getImprovementInfo(ImprovementTypes eImprovement) { CvGlobals const& kThis = *this; return kThis.getImprovementInfo(eImprovement); }
	inline CvImprovementInfo& getImprovementInfo(ImprovementTypes eImprovement) const
	{
		FASSERT_BOUNDS(0, getNumImprovementInfos(), eImprovement, "CvGlobals::getImprovementInfo");
		return *m_paImprovementInfo[eImprovement];
	}
	inline int getNumGoodyInfos() const
	{
		return (int)m_paGoodyInfo.size();
	}
	inline CvGoodyInfo& getGoodyInfo(GoodyTypes eGoody) const
	{
		FASSERT_BOUNDS(0, getNumGoodyInfos(), eGoody, "CvGlobals::getGoodyInfo");
		return *m_paGoodyInfo[eGoody];
	}
	inline int getNumBuildInfos() const
	{
		return (int)m_paBuildInfo.size();
	}
	DllExport CvBuildInfo& getBuildInfo(BuildTypes eBuild) { CvGlobals const& kThis = *this; return kThis.getBuildInfo(eBuild); }
	inline CvBuildInfo& getBuildInfo(BuildTypes eBuild) const
	{
		FASSERT_BOUNDS(0, getNumBuildInfos(), eBuild, "CvGlobals::getBuildInfo");
		return *m_paBuildInfo[eBuild];
	}
	DllExport int getNumHandicapInfos() { CvGlobals const& kThis = *this; return kThis.getNumHandicapInfos(); }
	inline int getNumHandicapInfos() const
	{
		return (int)m_paHandicapInfo.size();
	}
	DllExport CvHandicapInfo& getHandicapInfo(HandicapTypes eHandicap) { CvGlobals const& kThis = *this; return kThis.getHandicapInfo(eHandicap); }
	inline CvHandicapInfo& getHandicapInfo(HandicapTypes eHandicap) const
	{
		FASSERT_BOUNDS(0, getNumHandicapInfos(), eHandicap, "CvGlobals::getHandicapInfo");
		return *m_paHandicapInfo[eHandicap];
	}
	DllExport int getNumGameSpeedInfos() { CvGlobals const& kThis = *this; return kThis.getNumGameSpeedInfos(); }
	inline int getNumGameSpeedInfos() const
	{
		return (int)m_paGameSpeedInfo.size();
	}
	DllExport CvGameSpeedInfo& getGameSpeedInfo(GameSpeedTypes eGameSpeed) { CvGlobals const& kThis = *this; return kThis.getGameSpeedInfo(eGameSpeed); }
	inline CvGameSpeedInfo& getGameSpeedInfo(GameSpeedTypes eGameSpeed) const
	{
		FASSERT_BOUNDS(0, getNumGameSpeedInfos(), eGameSpeed, "CvGlobals::getGameSpeedInfo");
		return *m_paGameSpeedInfo[eGameSpeed];
	}
	DllExport int getNumTurnTimerInfos() { CvGlobals const& kThis = *this; return kThis.getNumTurnTimerInfos(); }
	inline int getNumTurnTimerInfos() const
	{
		return (int)m_paTurnTimerInfo.size();
	}
	DllExport CvTurnTimerInfo& getTurnTimerInfo(TurnTimerTypes eTurnTimer) { CvGlobals const& kThis = *this; return kThis.getTurnTimerInfo(eTurnTimer); }
	inline CvTurnTimerInfo& getTurnTimerInfo(TurnTimerTypes eTurnTimer) const
	{
		FASSERT_BOUNDS(0, getNumTurnTimerInfos(), eTurnTimer, "CvGlobals::getTurnTimerInfo");
		return *m_paTurnTimerInfo[eTurnTimer];
	}
	inline int getNumProcessInfos() const
	{
		return (int)m_paProcessInfo.size();
	}
	inline CvProcessInfo& getProcessInfo(ProcessTypes eProcess) const
	{
		FASSERT_BOUNDS(0, getNumProcessInfos(), eProcess, "CvGlobals::getProcessInfo");
		return *m_paProcessInfo[eProcess];
	}
	inline int getNumVoteInfos() const
	{
		return (int)m_paVoteInfo.size();
	}
	inline CvVoteInfo& getVoteInfo(VoteTypes eVote) const
	{
		FASSERT_BOUNDS(0, getNumVoteInfos(), eVote, "CvGlobals::getVoteInfo");
		return *m_paVoteInfo[eVote];
	}
	inline int getNumProjectInfos() const
	{
		return (int)m_paProjectInfo.size();
	}
	inline CvProjectInfo& getProjectInfo(ProjectTypes eProject) const
	{
		FASSERT_BOUNDS(0, getNumProjectInfos(), eProject, "CvGlobals::getProjectInfo");
		return *m_paProjectInfo[eProject];
	}
	inline int getNumBuildingClassInfos() const
	{
		return (int)m_paBuildingClassInfo.size();
	}
	inline CvBuildingClassInfo& getBuildingClassInfo(BuildingClassTypes eBuildingClass) const
	{
		FASSERT_BOUNDS(0, getNumBuildingClassInfos(), eBuildingClass, "CvGlobals::getBuildingClassInfo");
		return *m_paBuildingClassInfo[eBuildingClass];
	}
	inline int getNumBuildingInfos() const
	{
		return (int)m_paBuildingInfo.size();
	}
	inline CvBuildingInfo& getBuildingInfo(BuildingTypes eBuilding) const
	{
		FASSERT_BOUNDS(0, getNumBuildingInfos(), eBuilding, "CvGlobals::getBuildingInfo");
		return *m_paBuildingInfo[eBuilding];
	}
	inline int getNumSpecialBuildingInfos() const
	{
		return (int)m_paSpecialBuildingInfo.size();
	}
	inline CvSpecialBuildingInfo& getSpecialBuildingInfo(SpecialBuildingTypes eSpecialBuilding) const
	{
		FASSERT_BOUNDS(0, getNumSpecialBuildingInfos(), eSpecialBuilding, "CvGlobals::getSpecialBuildingInfo");
		return *m_paSpecialBuildingInfo[eSpecialBuilding];
	}
	inline int getNumUnitClassInfos() const
	{
		return (int)m_paUnitClassInfo.size();
	}
	inline CvUnitClassInfo& getUnitClassInfo(UnitClassTypes eUnitClass) const
	{
		FASSERT_BOUNDS(0, getNumUnitClassInfos(), eUnitClass, "CvGlobals::getUnitClassInfo");
		return *m_paUnitClassInfo[eUnitClass];
	}
	DllExport int getNumActionInfos() { CvGlobals const& kThis = *this; return kThis.getNumActionInfos(); }
	inline int getNumActionInfos() const
	{
		return (int)m_paActionInfo.size();
	}
	DllExport CvActionInfo& getActionInfo(int iAction) { CvGlobals const& kThis = *this; return kThis.getActionInfo(iAction); }
	inline CvActionInfo& getActionInfo(int iAction) const
	{
		FASSERT_BOUNDS(0, getNumActionInfos(), iAction, "CvGlobals::getActionInfo");
		return *m_paActionInfo[iAction];
	}
	DllExport CvMissionInfo& getMissionInfo(MissionTypes eMission) { CvGlobals const& kThis = *this; return kThis.getMissionInfo(eMission); }
	inline CvMissionInfo& getMissionInfo(MissionTypes eMission) const
	{
		FASSERT_BOUNDS(0, getNumMissionInfos(), eMission, "CvGlobals::getMissionInfo");
		return *m_paMissionInfo[eMission];
	}
	inline CvControlInfo& getControlInfo(ControlTypes eControl) const
	{
		FASSERT_BOUNDS(0, getNumControlInfos(), eControl, "CvGlobals::getControlInfo");
		return *m_paControlInfo[eControl];
	}
	inline CvCommandInfo& getCommandInfo(CommandTypes eCommand) const
	{
		FASSERT_BOUNDS(0, getNumCommandInfos(), eCommand, "CvGlobals::getCommandInfo");
		return *m_paCommandInfo[eCommand];
	}
	inline int getNumAutomateInfos() const
	{
		return (int)m_paAutomateInfo.size();
	}
	inline CvAutomateInfo& getAutomateInfo(AutomateTypes eAutomate) const
	{
		FASSERT_BOUNDS(0, getNumAutomateInfos(), eAutomate, "CvGlobals::getAutomateInfo");
		return *m_paAutomateInfo[eAutomate];
	}
	inline int getNumPromotionInfos() const
	{
		return (int)m_paPromotionInfo.size();
	}
	inline CvPromotionInfo& getPromotionInfo(PromotionTypes ePromotion) const
	{
		FASSERT_BOUNDS(0, getNumPromotionInfos(), ePromotion, "CvGlobals::getPromotionInfo");
		return *m_paPromotionInfo[ePromotion];
	}
	inline int getNumTechInfos() const
	{
		return (int)m_paTechInfo.size();
	}
	inline CvTechInfo& getTechInfo(TechTypes eTech) const
	{
		FASSERT_BOUNDS(0, getNumTechInfos(), eTech, "CvGlobals::getTechInfo");
		return *m_paTechInfo[eTech];
	}
	inline int getNumReligionInfos() const
	{
		return (int)m_paReligionInfo.size();
	}
	inline CvReligionInfo& getReligionInfo(ReligionTypes eReligion) const
	{
		FASSERT_BOUNDS(0, getNumReligionInfos(), eReligion, "CvGlobals::getReligionInfo");
		return *m_paReligionInfo[eReligion];
	}
	inline int getNumCorporationInfos() const
	{
		return (int)m_paCorporationInfo.size();
	}
	inline CvCorporationInfo& getCorporationInfo(CorporationTypes eCorporation) const
	{
		FASSERT_BOUNDS(0, getNumCorporationInfos(), eCorporation, "CvGlobals::getCorporationInfo");
		return *m_paCorporationInfo[eCorporation];
	}
	inline int getNumSpecialistInfos() const
	{
		return (int)m_paSpecialistInfo.size();
	}
	inline CvSpecialistInfo& getSpecialistInfo(SpecialistTypes eSpecialist) const
	{
		FASSERT_BOUNDS(0, getNumSpecialistInfos(), eSpecialist, "CvGlobals::getSpecialistInfo");
		return *m_paSpecialistInfo[eSpecialist];
	}
	inline int getNumCivicOptionInfos() const
	{
		return (int)m_paCivicOptionInfo.size();
	}
	inline CvCivicOptionInfo& getCivicOptionInfo(CivicOptionTypes eCivicOption) const
	{
		FASSERT_BOUNDS(0, getNumCivicOptionInfos(), eCivicOption, "CvGlobals::getCivicOptionInfo");
		return *m_paCivicOptionInfo[eCivicOption];
	}
	inline int getNumCivicInfos() const
	{
		return (int)m_paCivicInfo.size();
	}
	inline CvCivicInfo& getCivicInfo(CivicTypes eCivic) const
	{
		FASSERT_BOUNDS(0, getNumCivicInfos(), eCivic, "CvGlobals::getCivicInfo");
		return *m_paCivicInfo[eCivic];
	}
	inline int getNumDiplomacyInfos() const
	{
		return (int)m_paDiplomacyInfo.size();
	}
	inline CvDiplomacyInfo& getDiplomacyInfo(int iDiplomacy) const
	{
		FASSERT_BOUNDS(0, getNumDiplomacyInfos(), iDiplomacy, "CvGlobals::getDiplomacyInfo");
		return *m_paDiplomacyInfo[iDiplomacy];
	}
	DllExport int getNumEraInfos() { CvGlobals const& kThis = *this; return kThis.getNumEraInfos(); }
	inline int getNumEraInfos() const
	{
		return (int)m_aEraInfo.size();
	}
	DllExport CvEraInfo& getEraInfo(EraTypes eEra) { CvGlobals const& kThis = *this; return kThis.getEraInfo(eEra); }
	inline CvEraInfo& getEraInfo(EraTypes eEra) const
	{
		FASSERT_BOUNDS(0, getNumEraInfos(), eEra, "CvGlobals::getEraInfo");
		return *m_aEraInfo[eEra];
	}
	inline int getNumHurryInfos() const
	{
		return (int)m_paHurryInfo.size();
	}
	inline CvHurryInfo& getHurryInfo(HurryTypes eHurry) const
	{
		FASSERT_BOUNDS(0, getNumHurryInfos(), eHurry, "CvGlobals::getHurryInfo");
		return *m_paHurryInfo[eHurry];
	}
	inline int getNumEmphasizeInfos() const
	{
		return (int)m_paEmphasizeInfo.size();
	}
	inline CvEmphasizeInfo& getEmphasizeInfo(EmphasizeTypes eEmphasize) const
	{
		FASSERT_BOUNDS(0, getNumEmphasizeInfos(), eEmphasize, "CvGlobals::getEmphasizeInfo");
		return *m_paEmphasizeInfo[eEmphasize];
	}
	inline int getNumUpkeepInfos() const
	{
		return (int)m_paUpkeepInfo.size();
	}
	inline CvUpkeepInfo& getUpkeepInfo(UpkeepTypes eUpkeep) const
	{
		FASSERT_BOUNDS(0, getNumUpkeepInfos(), eUpkeep, "CvGlobals::getUpkeepInfo");
		return *m_paUpkeepInfo[eUpkeep];
	}
	inline int getNumCultureLevelInfos() const
	{
		return (int)m_paCultureLevelInfo.size();
	}
	inline CvCultureLevelInfo& getCultureLevelInfo(CultureLevelTypes eCultureLevel) const
	{
		FASSERT_BOUNDS(0, getNumCultureLevelInfos(), eCultureLevel, "CvGlobals::getCultureLevelInfo");
		return *m_paCultureLevelInfo[eCultureLevel];
	}
	DllExport int getNumVictoryInfos() { CvGlobals const& kThis = *this; return kThis.getNumVictoryInfos(); }
	inline int getNumVictoryInfos() const
	{
		return (int)m_paVictoryInfo.size();
	}
	DllExport CvVictoryInfo& getVictoryInfo(VictoryTypes eVictory) { CvGlobals const& kThis = *this; return kThis.getVictoryInfo(eVictory); }
	inline CvVictoryInfo& getVictoryInfo(VictoryTypes eVictory) const
	{
		FASSERT_BOUNDS(0, getNumVictoryInfos(), eVictory, "CvGlobals::getVictoryInfo");
		return *m_paVictoryInfo[eVictory];
	}
	// advc.003j:
	/*int getNumQuestInfos();
	CvQuestInfo& getQuestInfo(int iIndex);*/
	inline int getNumTutorialInfos() const
	{
		return (int)m_paTutorialInfo.size();
	}
	inline CvTutorialInfo& getTutorialInfo(int iTutorial) const
	{
		FASSERT_BOUNDS(0, getNumTutorialInfos(), iTutorial, "CvGlobals::getTutorialInfo");
		return *m_paTutorialInfo[iTutorial];
	}
	inline int getNumEventTriggerInfos() const
	{
		return (int)m_paEventTriggerInfo.size();
	}
	inline CvEventTriggerInfo& getEventTriggerInfo(EventTriggerTypes eEventTrigger) const
	{
		FASSERT_BOUNDS(0, getNumEventTriggerInfos(), eEventTrigger, "CvGlobals::getEventTriggerInfo");
		return *m_paEventTriggerInfo[eEventTrigger];
	}
	inline int getNumEventInfos() const
	{
		return (int)m_paEventInfo.size();
	}
	inline CvEventInfo& getEventInfo(EventTypes eEvent) const
	{
		FASSERT_BOUNDS(0, getNumEventInfos(), eEvent, "CvGlobals::getEventInfo");
		return *m_paEventInfo[eEvent];
	}
	inline int getNumEspionageMissionInfos() const
	{
		return (int)m_paEspionageMissionInfo.size();
	}
	inline CvEspionageMissionInfo& getEspionageMissionInfo(EspionageMissionTypes eEspionageMission) const
	{
		FASSERT_BOUNDS(0, getNumEspionageMissionInfos(), eEspionageMission, "CvGlobals::getEspionageMissionInfo");
		return *m_paEspionageMissionInfo[eEspionageMission];
	}
	inline int getNumUnitArtStyleTypeInfos() const
	{
		return (int)m_paUnitArtStyleTypeInfo.size();
	}
	inline CvUnitArtStyleTypeInfo& getUnitArtStyleTypeInfo(UnitArtStyleTypes eUnitArtStyle) const
	{
		FASSERT_BOUNDS(0, getNumUnitArtStyleTypeInfos(), eUnitArtStyle, "CvGlobals::getUnitArtStyleTypeInfo");
		return *m_paUnitArtStyleTypeInfo[eUnitArtStyle];
	}
#pragma endregion InfoAccessors
// </advc.003t>
	//
	// Global Types
	// All type strings are upper case and are kept in this hash map for fast lookup
	// The other functions are kept for convenience when enumerating, but most are not used
	//
	DllExport int getTypesEnum(const char* szType) const // use this when searching for a type
	// <advc.006> Add bHideAssert param
	{ return getTypesEnum(szType, false); }
	int getTypesEnum(const char* szType, bool bHideAssert) const; // </advc.006>
	void setTypesEnum(const char* szType, int iEnum);

	DllExport int getNUM_ENGINE_DIRTY_BITS() const;
	DllExport int getNUM_INTERFACE_DIRTY_BITS() const;
	DllExport int getNUM_YIELD_TYPES() const;
	int getNUM_COMMERCE_TYPES() const;
	DllExport int getNUM_FORCECONTROL_TYPES() const;
	DllExport int getNUM_INFOBAR_TYPES() const;
	DllExport int getNUM_HEALTHBAR_TYPES() const;
	int getNUM_CONTROL_TYPES() const;
	DllExport int getNUM_LEADERANIM_TYPES() const;

	int& getNumEntityEventTypes();
	CvString*& getEntityEventTypes();
	CvString& getEntityEventTypes(EntityEventTypes e);

	int& getNumAnimationOperatorTypes();
	CvString*& getAnimationOperatorTypes();
	CvString& getAnimationOperatorTypes(AnimationOperatorTypes e);

	CvString*& getFunctionTypes();
	CvString& getFunctionTypes(FunctionTypes e);

	inline int& getNumFlavorTypes() { return m_iNumFlavorTypes; }
	inline int const& getNumFlavorTypes() const { return m_iNumFlavorTypes; } // advc.003t: const version
	CvString*& getFlavorTypes();
	CvString& getFlavorTypes(FlavorTypes e);

	DllExport int& getNumArtStyleTypes();
	CvString*& getArtStyleTypes();
	DllExport CvString& getArtStyleTypes(ArtStyleTypes e);

	inline int& getNumCitySizeTypes() { return m_iNumCitySizeTypes; }
	inline int getNumCitySizeTypes() const { return m_iNumCitySizeTypes; } // advc.00t: const version
	CvString*& getCitySizeTypes();
	CvString& getCitySizeTypes(int i);

	CvString*& getContactTypes();
	CvString& getContactTypes(ContactTypes e);

	CvString*& getDiplomacyPowerTypes();
	CvString& getDiplomacyPowerTypes(DiplomacyPowerTypes e);

	CvString*& getAutomateTypes();
	CvString& getAutomateTypes(AutomateTypes e);

	CvString*& getDirectionTypes();
	CvString& getDirectionTypes(AutomateTypes e);

	DllExport inline int& getNumFootstepAudioTypes() { return m_iNumFootstepAudioTypes; }
	inline int getNumFootstepAudioTypes() const { return m_iNumFootstepAudioTypes; } // advc.003t: const version
	CvString*& getFootstepAudioTypes();
	CvString& getFootstepAudioTypes(int i);
	int getFootstepAudioTypeByTag(CvString strTag);

	CvString*& getFootstepAudioTags();
	DllExport CvString& getFootstepAudioTags(int i);

	CvString const& getCurrentXMLFile() const; // advc.003t: 2x const
	void setCurrentXMLFile(const TCHAR* szFileName);
	// <advc.003v>
	void setXMLLoadUtility(CvXMLLoadUtility* pXML);
	void loadOptionalXMLInfo();
	void loadThroneRoomInfo();
	// </advc.003v>

	//
	///////////////// BEGIN global defines
	// THESE ARE READ-ONLY
	//

	DllExport FVariableSystem* getDefinesVarSystem()
	// <advc> Need a const version
	{	CvGlobals const& kThis = *this;
		return const_cast<FVariableSystem*>(kThis.getDefinesVarSystem());
	} FVariableSystem const* getDefinesVarSystem() const { return m_VarSystem; }
	// </advc>
	void cacheGlobals();

	// ***** EXPOSED TO PYTHON *****
	DllExport inline int getDefineINT(const char * szName) const
	{
		return getDefineINT(szName, 0); // advc: Call the BBAI version
	}
	// BETTER_BTS_AI_MOD, Efficiency, Options, 02/21/10, jdog5000:
	int getDefineINT(const char * szName, const int iDefault) const;
	// <advc.003t>
	inline bool getDefineBOOL(const char * szName, const bool bDefault = false) const
	{
		return (getDefineINT(szName, (int)bDefault) > 0);
	} // </advc.003t>
	DllExport float getDefineFLOAT(const char * szName) const;
	DllExport const char * getDefineSTRING(const char * szName) const;
	/*  advc.003t: Params for suppressing cache update added. False for string b/c
		there are none that we could update. */
	void setDefineINT(const char * szName, int iValue, bool bUpdateCache = true);
	void setDefineFLOAT(const char * szName, float fValue, bool bUpdateCache = true);
	void setDefineSTRING(const char * szName, const char * szValue, bool bUpdateCache = false);
	// <advc.003t>
#pragma region GlobalDefines
	/*  Access cached integer GlobalDefines through enum values
		(not exposed to Python - though that might be nice).
		For string<->enum value conversion, create the enum through a macro
		based (closely) on this code snippet by Jasper Bekkers:
		https://stackoverflow.com/questions/201593/is-there-a-simple-way-to-convert-c-enum-to-string/238157#238157 */
	#define ENUMERATE_GLOBAL_DEFINES(DO) \
		DO(EXTRA_YIELD) /* K-Mod */ \
		DO(CITY_RADIUS_DECAY) /* advc.130s */ \
		DO(REVOLTS_IGNORE_CULTURE_RANGE) /* advc.099c */ \
		DO(NUM_WARNING_REVOLTS) /* advc.101 */ \
		DO(OCCUPATION_COUNTDOWN_EXPONENT) /* advc.023 */ \
		DO(MAX_DISTANCE_CITY_MAINTENANCE) /* advc.140 */ \
		DO(OWN_EXCLUSIVE_RADIUS) /* advc.035 */ \
		DO(ANNOUNCE_REPARATIONS) /* advc.039 */ \
		DO(BARB_PEAK_PERCENT) /* advc.300 */ \
		DO(PER_PLAYER_MESSAGE_CONTROL_LOG) /* advc.007 */ \
		DO(MINIMAP_WATER_MODE) /* advc.002a */ \
		DO(DELAY_UNTIL_BUILD_DECAY) /* advc.011 */ \
		DO(DISENGAGE_LENGTH) /* advc.034 */ \
		DO(AT_WAR_ATTITUDE_CHANGE) /* advc.130g */ \
		DO(RESEARCH_MODIFIER_EXTRA_TEAM_MEMBER) /* advc.156 */ \
		DO(WORKER_RESERVE_PERCENT) /* advc.113 */ \
		DO(CITY_DEFENSE_DAMAGE_HEAL_RATE) /* cdtw.2 */ \
		/* <advc.003b> */ \
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
		DO(CITY_TRADE_CULTURE_THRESH) \
		DO(AI_OFFER_EXTRA_GOLD_PERCENT) \
		DO(RECON_VISIBILITY_RANGE) \
		DO(UNIT_VISIBILITY_RANGE) \
		DO(MAX_FORTIFY_TURNS) \
		DO(WAR_SUCCESS_ATTACKING) \
		DO(WAR_SUCCESS_DEFENDING) \
		DO(ESPIONAGE_SPY_INTERCEPT_MOD) \
		DO(ESPIONAGE_SPY_NO_INTRUDE_INTERCEPT_MOD) \
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
		/* </advc.003b> */ \
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
		DO(EVENT_MESSAGE_TIME) \
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
		DO(LFB_USECOMBATODDS) /* BETTER_BTS_AI_MOD: END */
	#define MAKE_ENUM(VAR) VAR,
	enum GlobalDefines
	{
		NO_GLOBAL_DEFINE = -1,
		ENUMERATE_GLOBAL_DEFINES(MAKE_ENUM)
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
	DllExport inline int getEVENT_MESSAGE_TIME() { CvGlobals const& kThis = *this; return kThis.getEVENT_MESSAGE_TIME(); }
	inline int getEVENT_MESSAGE_TIME() const { return getDefineINT(EVENT_MESSAGE_TIME); }
	inline int getEVENT_MESSAGE_TIME_LONG() const { return getDefineINT(EVENT_MESSAGE_TIME_LONG); } // advc: Treat these two the same
	// BETTER_BTS_AI_MOD, Efficiency, Options, 02/21/10, jdog5000: START
	inline int getWAR_SUCCESS_CITY_CAPTURING() const { return getDefineINT(WAR_SUCCESS_CITY_CAPTURING); }
	inline int getCOMBAT_DIE_SIDES() const { return getDefineINT(COMBAT_DIE_SIDES); }
	inline int getCOMBAT_DAMAGE() const { return getDefineINT(COMBAT_DAMAGE); }
	// BETTER_BTS_AI_MOD: END
	// </advc.003t>
	/*  <advc.003b> (TextVals can't be loaded by cacheGlobals. Hence also won't be
		updated when a setDefine... function is called.) */
	inline ImprovementTypes getRUINS_IMPROVEMENT() const
	{
		FAssertMsg(m_iRUINS_IMPROVEMENT != NO_IMPROVEMENT, "RUINS_IMPROVEMENT accessed before CvXMLLoadUtility::SetPostGlobalsGlobalDefines");
		return (ImprovementTypes)m_iRUINS_IMPROVEMENT;
	}
	void setRUINS_IMPROVEMENT(int iVal);
	inline SpecialistTypes getDEFAULT_SPECIALIST() const
	{
		FAssertMsg(m_iDEFAULT_SPECIALIST != NO_SPECIALIST, "DEFAULT_SPECIALIST accessed before CvXMLLoadUtility::SetPostGlobalsGlobalDefines");
		return (SpecialistTypes)m_iDEFAULT_SPECIALIST;
	}
	void setDEFAULT_SPECIALIST(int iVal);
	inline TerrainTypes getWATER_TERRAIN(bool bShallow) const
	{
		int r = m_aiWATER_TERRAIN[bShallow];
		FAssertMsg(r != NO_TERRAIN, "WATER_TERRAIN accessed before CvXMLLoadUtility::SetPostGlobalsGlobalDefines");
		return (TerrainTypes)r;
	}
	void setWATER_TERRAIN(bool bShallow, int iValue);
	// </advc.003b>
	// <advc.003t> Parameters added  // The getNUM...PREREQ... functions are all exposed to Python
	int getNUM_UNIT_PREREQ_OR_BONUSES(UnitTypes eUnit = NO_UNIT) const;
	// <advc.905b> No parameter here
	inline int getNUM_UNIT_SPEED_BONUSES() const
	{
		return getNUM_UNIT_PREREQ_OR_BONUSES();
	} // </advc.905b>
	int getNUM_UNIT_AND_TECH_PREREQS(UnitTypes eUnit = NO_UNIT) const;
	int getNUM_BUILDING_PREREQ_OR_BONUSES(BuildingTypes eBuilding = NO_BUILDING) const;
	int getNUM_BUILDING_AND_TECH_PREREQS(BuildingTypes eBuilding = NO_BUILDING) const;
	int getNUM_AND_TECH_PREREQS(TechTypes = NO_TECH) const;
	int getNUM_OR_TECH_PREREQS(TechTypes = NO_TECH) const;
	int getNUM_ROUTE_PREREQ_OR_BONUSES(RouteTypes eRoute = NO_ROUTE) const;
	// </advc.003t>
	int getNUM_CORPORATION_PREREQ_BONUSES() const; // advc: A param like above doesn't help b/c all corps require resources
	inline float getPOWER_CORRECTION() const { return m_fPOWER_CORRECTION; } // advc.104
	// <advc.003t> All inlined and constified
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

	DllExport int getUSE_FINISH_TEXT_CALLBACK();
	// advc.003y: Moved the other callback getters to CvPythonCaller
#pragma endregion GlobalDefines
	// more reliable versions of the 'gDLL->xxxKey' functions:
	// NOTE: I've replaced all calls to the gDLL key functions with calls to these functions.
	inline bool altKey() const { return (GetKeyState(VK_MENU) & 0x8000); }
	inline bool ctrlKey() const { return (GetKeyState(VK_CONTROL) & 0x8000); }
	inline bool shiftKey() const { return (GetKeyState(VK_SHIFT) & 0x8000); }
	// hold X to temporarily suppress automatic unit cycling.
	inline bool suppressCycling() const { return (GetKeyState('X') & 0x8000) ||
			((GetKeyState('U') & 0x8000) && shiftKey()); } // advc.088
	// K-Mod end
	// advc.003t: Inlined and constified
	DllExport inline int getMAX_CIV_PLAYERS() { CvGlobals const& kThis = *this; return kThis.getMAX_CIV_PLAYERS(); }
	inline int getMAX_CIV_PLAYERS() const { return MAX_CIV_PLAYERS; }
	inline int getMAX_PLAYERS() const { return MAX_PLAYERS; }
	inline int getMAX_CIV_TEAMS() const { return MAX_CIV_TEAMS; }
	inline int getMAX_TEAMS() const { return MAX_TEAMS; }
	inline int getBARBARIAN_PLAYER() const { return BARBARIAN_PLAYER; }
	inline int getBARBARIAN_TEAM() const { return BARBARIAN_TEAM; }
	inline int getINVALID_PLOT_COORD() const { return INVALID_PLOT_COORD; }
	inline int getNUM_CITY_PLOTS() const { return NUM_CITY_PLOTS; }
	inline int getCITY_HOME_PLOT() const { return CITY_HOME_PLOT; }

	// ***** END EXPOSED TO PYTHON *****
	////////////// END DEFINES //////////////////

	DllExport void setDLLIFace(CvDLLUtilityIFaceBase* pDll);
#ifdef _USRDLL
	// inlined for perf reasons, do not use outside of dll
	CvDLLUtilityIFaceBase* getDLLIFace() const { return m_pDLL; } // advc.003t: const
#endif
	DllExport CvDLLUtilityIFaceBase* getDLLIFaceNonInl();
	DllExport void setDLLProfiler(FProfiler* prof);
	FProfiler* getDLLProfiler();
	DllExport void enableDLLProfiler(bool bEnable);
	bool isDLLProfilerEnabled() const;

	DllExport bool IsGraphicsInitialized() const;
	DllExport void SetGraphicsInitialized(bool bVal);

	// for caching
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

	//
	// additional accessors for initting globals
	//

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
	DllExport int getNumDirections() const;
	DllExport int getNumGameOptions() const;
	DllExport int getNumMPOptions() const;
	DllExport int getNumSpecialOptions() const;
	DllExport int getNumGraphicOptions() const;
	DllExport int getNumTradeableItems() const;
	DllExport int getNumBasicItems() const;
	DllExport int getNumTradeableHeadings() const;
	int getNumCommandInfos() const;
	int getNumControlInfos() const;
	int getNumMissionInfos() const;
	DllExport int getNumPlayerOptionInfos() const;
	DllExport int getMaxNumSymbols() const;
	DllExport int getNumGraphicLevels() const;
	int getNumGlobeLayers() const;

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
	//NiPoint3  m_pt3CameraDir; // advc.003j: Unused; not even written.
	int m_iNewPlayers;

	CMainMenu* m_pkMainMenu;

	bool m_bZoomOut;
	bool m_bZoomIn;
	bool m_bLoadGameFromFile;

	FMPIManager * m_pFMPMgr;

	CvRandom* m_asyncRand;
	CvPythonCaller* m_pPythonCaller; // advc.003y
	CvDLLLogger* m_pLogger; // advc.003t
	CvGameAI* m_game;

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

	int* m_aiPlotDirectionX;	// [NUM_DIRECTION_TYPES];
	int* m_aiPlotDirectionY;	// [NUM_DIRECTION_TYPES];
	int* m_aiPlotCardinalDirectionX;	// [NUM_CARDINALDIRECTION_TYPES];
	int* m_aiPlotCardinalDirectionY;	// [NUM_CARDINALDIRECTION_TYPES];
	int* m_aiCityPlotX;	// [NUM_CITY_PLOTS];
	int* m_aiCityPlotY;	// [NUM_CITY_PLOTS];
	int* m_aiCityPlotPriority;	// [NUM_CITY_PLOTS];
	int m_aaiXYCityPlot[CITY_PLOTS_DIAMETER][CITY_PLOTS_DIAMETER];

	DirectionTypes* m_aeTurnLeftDirection;	// [NUM_DIRECTION_TYPES];
	DirectionTypes* m_aeTurnRightDirection;	// [NUM_DIRECTION_TYPES];
	DirectionTypes m_aaeXYDirection[DIRECTION_DIAMETER][DIRECTION_DIAMETER];

	//InterfaceModeInfo m_aInterfaceModeInfo[NUM_INTERFACEMODE_TYPES] =
	std::vector<CvInterfaceModeInfo*> m_paInterfaceModeInfo;

	/***********************************************************************************************************************
	Globals loaded from XML
	************************************************************************************************************************/

	// all type strings are upper case and are kept in this hash map for fast lookup, Moose
	typedef stdext::hash_map<std::string /* type string */, int /* info index */> InfosMap;
	InfosMap m_infosMap;
	std::vector<std::vector<CvInfoBase *> *> m_aInfoVectors;

	std::vector<CvColorInfo*> m_paColorInfo;
	std::vector<CvPlayerColorInfo*> m_paPlayerColorInfo;
	std::vector<CvAdvisorInfo*> m_paAdvisorInfo;
	std::vector<CvInfoBase*> m_paHints;
	std::vector<CvMainMenuInfo*> m_paMainMenus;
	std::vector<CvTerrainInfo*> m_paTerrainInfo;
	std::vector<CvLandscapeInfo*> m_paLandscapeInfo;
	int m_iActiveLandscapeID;
	std::vector<CvWorldInfo*> m_paWorldInfo;
	std::vector<CvClimateInfo*> m_paClimateInfo;
	std::vector<CvSeaLevelInfo*> m_paSeaLevelInfo;
	std::vector<CvYieldInfo*> m_paYieldInfo;
	std::vector<CvCommerceInfo*> m_paCommerceInfo;
	std::vector<CvRouteInfo*> m_paRouteInfo;
	std::vector<CvFeatureInfo*> m_paFeatureInfo;
	std::vector<CvBonusClassInfo*> m_paBonusClassInfo;
	std::vector<CvBonusInfo*> m_paBonusInfo;
	std::vector<CvImprovementInfo*> m_paImprovementInfo;
	std::vector<CvGoodyInfo*> m_paGoodyInfo;
	std::vector<CvBuildInfo*> m_paBuildInfo;
	std::vector<CvHandicapInfo*> m_paHandicapInfo;
	std::vector<CvGameSpeedInfo*> m_paGameSpeedInfo;
	std::vector<CvTurnTimerInfo*> m_paTurnTimerInfo;
	std::vector<CvCivilizationInfo*> m_paCivilizationInfo;
	int m_iNumPlayableCivilizationInfos;
	int m_iNumAIPlayableCivilizationInfos;
	std::vector<CvLeaderHeadInfo*> m_paLeaderHeadInfo;
	std::vector<CvTraitInfo*> m_paTraitInfo;
	std::vector<CvCursorInfo*> m_paCursorInfo;
	std::vector<CvThroneRoomCamera*> m_paThroneRoomCamera;
	std::vector<CvThroneRoomInfo*> m_paThroneRoomInfo;
	std::vector<CvThroneRoomStyleInfo*> m_paThroneRoomStyleInfo;
	std::vector<CvSlideShowInfo*> m_paSlideShowInfo;
	std::vector<CvSlideShowRandomInfo*> m_paSlideShowRandomInfo;
	std::vector<CvWorldPickerInfo*> m_paWorldPickerInfo;
	std::vector<CvSpaceShipInfo*> m_paSpaceShipInfo;
	std::vector<CvProcessInfo*> m_paProcessInfo;
	std::vector<CvVoteInfo*> m_paVoteInfo;
	std::vector<CvProjectInfo*> m_paProjectInfo;
	std::vector<CvBuildingClassInfo*> m_paBuildingClassInfo;
	std::vector<CvBuildingInfo*> m_paBuildingInfo;
	std::vector<CvSpecialBuildingInfo*> m_paSpecialBuildingInfo;
	std::vector<CvUnitClassInfo*> m_paUnitClassInfo;
	std::vector<CvUnitInfo*> m_paUnitInfo;
	std::vector<CvSpecialUnitInfo*> m_paSpecialUnitInfo;
	std::vector<CvInfoBase*> m_paConceptInfo;
	std::vector<CvInfoBase*> m_paNewConceptInfo;
	std::vector<CvInfoBase*> m_paCityTabInfo;
	std::vector<CvInfoBase*> m_paCalendarInfo;
	std::vector<CvInfoBase*> m_paSeasonInfo;
	std::vector<CvInfoBase*> m_paMonthInfo;
	std::vector<CvInfoBase*> m_paDenialInfo;
	std::vector<CvInfoBase*> m_paInvisibleInfo;
	std::vector<CvVoteSourceInfo*> m_paVoteSourceInfo;
	std::vector<CvInfoBase*> m_paUnitCombatInfo;
	std::vector<CvInfoBase*> m_paDomainInfo;
	std::vector<CvInfoBase*> m_paUnitAIInfo;
	std::vector<CvInfoBase*> m_paAttitudeInfo;
	std::vector<CvInfoBase*> m_paMemoryInfo;
	std::vector<CvInfoBase*> m_paFeatInfo;
	std::vector<CvGameOptionInfo*> m_paGameOptionInfo;
	std::vector<CvMPOptionInfo*> m_paMPOptionInfo;
	std::vector<CvForceControlInfo*> m_paForceControlInfo;
	std::vector<CvPlayerOptionInfo*> m_paPlayerOptionInfo;
	std::vector<CvGraphicOptionInfo*> m_paGraphicOptionInfo;
	std::vector<CvSpecialistInfo*> m_paSpecialistInfo;
	std::vector<CvEmphasizeInfo*> m_paEmphasizeInfo;
	std::vector<CvUpkeepInfo*> m_paUpkeepInfo;
	std::vector<CvCultureLevelInfo*> m_paCultureLevelInfo;
	std::vector<CvReligionInfo*> m_paReligionInfo;
	std::vector<CvCorporationInfo*> m_paCorporationInfo;
	std::vector<CvActionInfo*> m_paActionInfo;
	std::vector<CvMissionInfo*> m_paMissionInfo;
	std::vector<CvControlInfo*> m_paControlInfo;
	std::vector<CvCommandInfo*> m_paCommandInfo;
	std::vector<CvAutomateInfo*> m_paAutomateInfo;
	std::vector<CvPromotionInfo*> m_paPromotionInfo;
	std::vector<CvTechInfo*> m_paTechInfo;
	std::vector<CvCivicOptionInfo*> m_paCivicOptionInfo;
	std::vector<CvCivicInfo*> m_paCivicInfo;
	std::vector<CvDiplomacyInfo*> m_paDiplomacyInfo;
	std::vector<CvEraInfo*> m_aEraInfo;	// [NUM_ERA_TYPES];
	std::vector<CvHurryInfo*> m_paHurryInfo;
	std::vector<CvVictoryInfo*> m_paVictoryInfo;
	std::vector<CvRouteModelInfo*> m_paRouteModelInfo;
	std::vector<CvRiverInfo*> m_paRiverInfo;
	std::vector<CvRiverModelInfo*> m_paRiverModelInfo;
	std::vector<CvWaterPlaneInfo*> m_paWaterPlaneInfo;
	std::vector<CvTerrainPlaneInfo*> m_paTerrainPlaneInfo;
	std::vector<CvCameraOverlayInfo*> m_paCameraOverlayInfo;
	std::vector<CvAnimationPathInfo*> m_paAnimationPathInfo;
	std::vector<CvAnimationCategoryInfo*> m_paAnimationCategoryInfo;
	std::vector<CvEntityEventInfo*> m_paEntityEventInfo;
	std::vector<CvUnitFormationInfo*> m_paUnitFormationInfo;
	std::vector<CvEffectInfo*> m_paEffectInfo;
	std::vector<CvAttachableInfo*> m_paAttachableInfo;
	std::vector<CvCameraInfo*> m_paCameraInfo;
	//std::vector<CvQuestInfo*> m_paQuestInfo; // advc.003j
	std::vector<CvTutorialInfo*> m_paTutorialInfo;
	std::vector<CvEventTriggerInfo*> m_paEventTriggerInfo;
	std::vector<CvEventInfo*> m_paEventInfo;
	std::vector<CvEspionageMissionInfo*> m_paEspionageMissionInfo;
	std::vector<CvUnitArtStyleTypeInfo*> m_paUnitArtStyleTypeInfo;

	// Game Text
	std::vector<CvGameText*> m_paGameTextXML;

	//////////////////////////////////////////////////////////////////////////
	// GLOBAL TYPES
	//////////////////////////////////////////////////////////////////////////

	// all type strings are upper case and are kept in this hash map for fast lookup, Moose
	typedef stdext::hash_map<std::string /* type string */, int /*enum value */> TypesMap;
	TypesMap m_typesMap;

	// XXX These are duplicates and are kept for enumeration convenience - most could be removed, Moose
	CvString *m_paszEntityEventTypes2;
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
	int m_iNumCitySizeTypes;

	CvString *m_paszContactTypes;

	CvString *m_paszDiplomacyPowerTypes;

	CvString *m_paszAutomateTypes;

	CvString *m_paszDirectionTypes;

	CvString *m_paszFootstepAudioTypes;
	int m_iNumFootstepAudioTypes;

	CvString *m_paszFootstepAudioTags;
	int m_iNumFootstepAudioTags;

	CvString m_szCurrentXMLFile;
	bool m_bHoFScreenUp; // advc.106i
	//////////////////////////////////////////////////////////////////////////
	// Formerly Global Defines
	//////////////////////////////////////////////////////////////////////////

	FVariableSystem* m_VarSystem;

	int* m_aiGlobalDefinesCache; // advc.003t
	// <advc.003b>
	int m_iRUINS_IMPROVEMENT;
	int m_iDEFAULT_SPECIALIST;
	int m_aiWATER_TERRAIN[2]; // </advc.003b>
	float m_fPOWER_CORRECTION; // advc.104

	float m_fCAMERA_MIN_YAW;
	float m_fCAMERA_MAX_YAW;
	float m_fCAMERA_FAR_CLIP_Z_HEIGHT;
	float m_fCAMERA_MAX_TRAVEL_DISTANCE;
	float m_fCAMERA_START_DISTANCE;
	float m_fAIR_BOMB_HEIGHT;
	float m_fPLOT_SIZE;
	float m_fCAMERA_SPECIAL_PITCH;
	float m_fCAMERA_MAX_TURN_OFFSET;
	float m_fCAMERA_MIN_DISTANCE;
	float m_fCAMERA_UPPER_PITCH;
	float m_fCAMERA_LOWER_PITCH;
	float m_fFIELD_OF_VIEW;
	float m_fSHADOW_SCALE;
	float m_fUNIT_MULTISELECT_DISTANCE;

	CvXMLLoadUtility* m_pXMLLoadUtility; // advc.003v

	// DLL interface
	CvDLLUtilityIFaceBase* m_pDLL;

	FProfiler* m_Profiler;		// profiler
	CvString m_szDllProfileText;
	// <advc.003t>
private:
	void cacheGlobalInts(char const* szChangedDefine = NULL, int iNewValue = 0);
	void cacheGlobalFloats(); // </advc.003t>
};

extern CvGlobals gGlobals;	// for debugging

//
// inlines
//
__forceinline CvGlobals& CvGlobals::getInstance()
{
	return gGlobals;
}
// <advc.003t>
__forceinline CvGlobals const& CvGlobals::getConstInstance()
{
	return gGlobals;
} // </advc.003t>


//
// helpers
//
#define GC CvGlobals::getConstInstance() // advc.003t: was ...getInstance()
#ifndef _USRDLL
#define gDLL GC.getDLLIFaceNonInl()
#else
#define gDLL GC.getDLLIFace()
#endif

// advc.003t: Direct access to RNG (can't get it from the const GC)
inline CvRandom& getASyncRand() { return CvGlobals::getInstance().getASyncRand(); }

#ifndef _USRDLL
#define NUM_DIRECTION_TYPES (GC.getNumDirections())
#define NUM_GAMEOPTION_TYPES (GC.getNumGameOptions())
#define NUM_MPOPTION_TYPES (GC.getNumMPOptions())
#define NUM_SPECIALOPTION_TYPES (GC.getNumSpecialOptions())
#define NUM_GRAPHICOPTION_TYPES (GC.getNumGraphicOptions())
#define NUM_TRADEABLE_ITEMS (GC.getNumTradeableItems())
#define NUM_BASIC_ITEMS (GC.getNumBasicItems())
#define NUM_TRADEABLE_HEADINGS (GC.getNumTradeableHeadings())
#define NUM_COMMAND_TYPES (GC.getNumCommandInfos())
#define NUM_CONTROL_TYPES (GC.getNumControlInfos())
#define NUM_MISSION_TYPES (GC.getNumMissionInfos())
#define NUM_PLAYEROPTION_TYPES (GC.getNumPlayerOptionInfos())
#define MAX_NUM_SYMBOLS (GC.getMaxNumSymbols())
#define NUM_GRAPHICLEVELS (GC.getNumGraphicLevels())
#define NUM_GLOBE_LAYER_TYPES (GC.getNumGlobeLayers())
#endif
// advc.003t: Re-enable warnings about unknown pragma
#pragma warning(default:4068)

#endif
