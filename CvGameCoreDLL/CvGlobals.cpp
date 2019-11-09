// globals.cpp  // advc: Rearranged a lot of things in this file
#include "CvGameCoreDLL.h"
#include "CvGlobals.h"
#include "FVariableSystem.h"
#include "CvGamePlay.h"
#include "CvGameAI.h"
#include "CvMap.h"
#include "CvInfo_All.h"
#include "CvGameTextMgr.h"
#include "CvXMLLoadUtility.h" // advc.003v
#include "CvDLLUtilityIFaceBase.h"
#include "CvDLLXMLIFaceBase.h"
// <advc.003o>
#ifdef USE_TSC_PROFILER
#include "TSCProfiler.h"
#endif // </advc.003o>

CvGlobals gGlobals; // singleton instance

CvGlobals::CvGlobals() :
m_bGraphicsInitialized(false),
m_bLogging(false),
m_bRandLogging(false),
m_bOverwriteLogs(false),
m_bSynchLogging(false),
m_bDLLProfiler(false),
m_pkMainMenu(NULL),
m_iNewPlayers(0),
m_bZoomOut(false),
m_bZoomIn(false),
m_bLoadGameFromFile(false),
m_pFMPMgr(NULL),
m_asyncRand(NULL),
m_pPythonCaller(NULL), // advc.003y
m_pLogger(NULL), // advc.003t
m_interface(NULL),
m_game(NULL),
m_messageQueue(NULL),
m_hotJoinMsgQueue(NULL),
m_messageControl(NULL),
m_messageCodes(NULL),
m_dropMgr(NULL),
m_portal(NULL),
m_setupData(NULL),
// <kmodx> Missing initialization
m_iniInitCore(NULL),
m_loadedInitCore(NULL),
// </kmodx>
m_initCore(NULL),
m_statsReporter(NULL),
m_map(NULL),
m_diplomacyScreen(NULL),
m_mpDiplomacyScreen(NULL),
m_pathFinder(NULL),
m_interfacePathFinder(NULL),
m_stepFinder(NULL),
m_routeFinder(NULL),
m_borderFinder(NULL),
m_areaFinder(NULL),
m_plotGroupFinder(NULL),
m_pXMLLoadUtility(NULL), // advc.003v
m_pDLL(NULL),
m_Profiler(NULL),
m_VarSystem(NULL),
m_aiGlobalDefinesCache(NULL), // advc.003t, advc.003c
m_bHoFScreenUp(false), // advc.106i
m_fCAMERA_MIN_YAW(0), m_fCAMERA_MAX_YAW(0), m_fCAMERA_FAR_CLIP_Z_HEIGHT(0),
m_fCAMERA_MAX_TRAVEL_DISTANCE(0), m_fCAMERA_START_DISTANCE(0),
m_fAIR_BOMB_HEIGHT(0), m_fPLOT_SIZE(0), m_fCAMERA_SPECIAL_PITCH(0),
m_fCAMERA_MAX_TURN_OFFSET(0), m_fCAMERA_MIN_DISTANCE(0),
m_fCAMERA_UPPER_PITCH(0), m_fCAMERA_LOWER_PITCH(0),
m_fFIELD_OF_VIEW(0), m_fSHADOW_SCALE(0),
m_fUNIT_MULTISELECT_DISTANCE(0),
// <advc> Safer to initialize these
m_paszEntityEventTypes2(NULL),
m_paszEntityEventTypes(NULL),
m_paszAnimationOperatorTypes(NULL),
m_paszFunctionTypes(NULL),
m_paszFlavorTypes(NULL),
m_paszArtStyleTypes(NULL),
m_paszCitySizeTypes(NULL),
m_paszContactTypes(NULL),
m_paszDiplomacyPowerTypes(NULL),
m_paszAutomateTypes(NULL),
m_paszDirectionTypes(NULL),
m_paszFootstepAudioTypes(NULL),
m_paszFootstepAudioTags(NULL),
m_iNumEntityEventTypes(0),
m_iNumAnimationOperatorTypes(0),
m_iNumFlavorTypes(0),
m_iNumArtStyleTypes(0),
m_iNumFootstepAudioTypes(0),
// </advc> <advc.opt>
m_iRUINS_IMPROVEMENT(NO_IMPROVEMENT),
m_iDEFAULT_SPECIALIST(NO_SPECIALIST)
{
	m_aiWATER_TERRAIN[0] = m_aiWATER_TERRAIN[1] = -1; // </advc.opt>
	setCurrentXMLFile(NULL); // advc.006e
}

void CvGlobals::init() // allocate
{
	int aiPlotDirectionX[NUM_DIRECTION_TYPES] =
	{
		0,	// DIRECTION_NORTH
		1,	// DIRECTION_NORTHEAST
		1,	// DIRECTION_EAST
		1,	// DIRECTION_SOUTHEAST
		0,	// DIRECTION_SOUTH
		-1,	// DIRECTION_SOUTHWEST
		-1,	// DIRECTION_WEST
		-1,	// DIRECTION_NORTHWEST
	};

	int aiPlotDirectionY[NUM_DIRECTION_TYPES] =
	{
		1,	// DIRECTION_NORTH
		1,	// DIRECTION_NORTHEAST
		0,	// DIRECTION_EAST
		-1,	// DIRECTION_SOUTHEAST
		-1,	// DIRECTION_SOUTH
		-1,	// DIRECTION_SOUTHWEST
		0,	// DIRECTION_WEST
		1,	// DIRECTION_NORTHWEST
	};

	int aiPlotCardinalDirectionX[NUM_CARDINALDIRECTION_TYPES] =
	{
		0,	// CARDINALDIRECTION_NORTH
		1,	// CARDINALDIRECTION_EAST
		0,	// CARDINALDIRECTION_SOUTH
		-1,	// CARDINALDIRECTION_WEST
	};

	int aiPlotCardinalDirectionY[NUM_CARDINALDIRECTION_TYPES] =
	{
		1,	// CARDINALDIRECTION_NORTH
		0,	// CARDINALDIRECTION_EAST
		-1,	// CARDINALDIRECTION_SOUTH
		0,	// CARDINALDIRECTION_WEST
	};

	int aiCityPlotX[NUM_CITY_PLOTS] =
	{
		0,
		0, 1, 1, 1, 0,-1,-1,-1,
		0, 1, 2, 2, 2, 1, 0,-1,-2,-2,-2,-1,
	};

	int aiCityPlotY[NUM_CITY_PLOTS] =
	{
		0,
		1, 1, 0,-1,-1,-1, 0, 1,
		2, 2, 1, 0,-1,-2,-2,-2,-1, 0, 1, 2,
	};

	int aiCityPlotPriority[NUM_CITY_PLOTS] =
	{
		0,
		1, 2, 1, 2, 1, 2, 1, 2,
		3, 4, 4, 3, 4, 4, 3, 4, 4, 3, 4, 4,
	};

	int aaiXYCityPlot[CITY_PLOTS_DIAMETER][CITY_PLOTS_DIAMETER] =
	{
		{-1, 17, 18, 19, -1,},

		{16, 6, 7, 8, 20,},

		{15, 5, 0, 1, 9,},

		{14, 4, 3, 2, 10,},

		{-1, 13, 12, 11, -1,}
	};

	DirectionTypes aeTurnRightDirection[NUM_DIRECTION_TYPES] =
	{
		DIRECTION_NORTHEAST,	// DIRECTION_NORTH
		DIRECTION_EAST,			// DIRECTION_NORTHEAST
		DIRECTION_SOUTHEAST,	// DIRECTION_EAST
		DIRECTION_SOUTH,		// DIRECTION_SOUTHEAST
		DIRECTION_SOUTHWEST,	// DIRECTION_SOUTH
		DIRECTION_WEST,			// DIRECTION_SOUTHWEST
		DIRECTION_NORTHWEST,	// DIRECTION_WEST
		DIRECTION_NORTH,		// DIRECTION_NORTHWEST
	};

	DirectionTypes aeTurnLeftDirection[NUM_DIRECTION_TYPES] =
	{
		DIRECTION_NORTHWEST,	// DIRECTION_NORTH
		DIRECTION_NORTH,		// DIRECTION_NORTHEAST
		DIRECTION_NORTHEAST,	// DIRECTION_EAST
		DIRECTION_EAST,			// DIRECTION_SOUTHEAST
		DIRECTION_SOUTHEAST,	// DIRECTION_SOUTH
		DIRECTION_SOUTH,		// DIRECTION_SOUTHWEST
		DIRECTION_SOUTHWEST,	// DIRECTION_WEST
		DIRECTION_WEST,			// DIRECTION_NORTHWEST
	};

	DirectionTypes aaeXYDirection[DIRECTION_DIAMETER][DIRECTION_DIAMETER] =
	{
		DIRECTION_SOUTHWEST, DIRECTION_WEST,	DIRECTION_NORTHWEST,
		DIRECTION_SOUTH,     NO_DIRECTION,    DIRECTION_NORTH,
		DIRECTION_SOUTHEAST, DIRECTION_EAST,	DIRECTION_NORTHEAST,
	};
	/*  <advc.006> getInfoTypeForString gets called for each of the PlotTypes values
		at startup and when reloading Python scripts. PlotTypes isn't an info type,
		so this is probably an error, but I can't locate that error. In conjunction
		with changes in getInfoTypeForString, adding the PlotTypes values to the
		enum types map prevents a failed assertion. */
	setTypesEnum("PLOT_PEAK", PLOT_PEAK);
	setTypesEnum("PLOT_HILLS", PLOT_HILLS);
	setTypesEnum("PLOT_LAND", PLOT_LAND);
	setTypesEnum("PLOT_OCEAN", PLOT_OCEAN); // </advc.006>

	FAssertMsg(gDLL != NULL, "Civ app needs to set gDLL");

	m_VarSystem = new FVariableSystem();
	m_asyncRand = new CvRandom();
	m_initCore = new CvInitCore();
	m_loadedInitCore = new CvInitCore();
	m_iniInitCore = new CvInitCore();
	gDLL->initGlobals(); // some globals need to be allocated outside the dll
	m_pLogger = new CvDLLLogger(isLogging(), isRandLogging()); // advc.003t
	m_game = new CvGameAI();
	m_map = new CvMap();

	CvPlayer::initStatics();
	CvTeam::initStatics();

	//m_pt3Origin = NiPoint3(0.0f, 0.0f, 0.0f); // advc.003j: unused

	memcpy(m_aiPlotDirectionX, aiPlotDirectionX, sizeof(m_aiPlotDirectionX));
	memcpy(m_aiPlotDirectionY, aiPlotDirectionY, sizeof(m_aiPlotDirectionY));
	memcpy(m_aiPlotCardinalDirectionX, aiPlotCardinalDirectionX, sizeof(m_aiPlotCardinalDirectionX));
	memcpy(m_aiPlotCardinalDirectionY, aiPlotCardinalDirectionY, sizeof(m_aiPlotCardinalDirectionY));
	memcpy(m_aiCityPlotX, aiCityPlotX, sizeof(m_aiCityPlotX));
	memcpy(m_aiCityPlotY, aiCityPlotY, sizeof(m_aiCityPlotY));
	memcpy(m_aiCityPlotPriority, aiCityPlotPriority, sizeof(m_aiCityPlotPriority));
	memcpy(m_aeTurnLeftDirection, aeTurnLeftDirection, sizeof(m_aeTurnLeftDirection));
	memcpy(m_aeTurnRightDirection, aeTurnRightDirection, sizeof(m_aeTurnRightDirection));
	memcpy(m_aaiXYCityPlot, aaiXYCityPlot, sizeof(m_aaiXYCityPlot));
	memcpy(m_aaeXYDirection, aaeXYDirection,sizeof(m_aaeXYDirection));
}
// advc: Not needed anymore
/*#define COPY(dst, src, typeName) \
	{ \
		int iNum = sizeof(src) / sizeof(typeName); \
		dst = new typeName[iNum]; \
		for (int i = 0; i < iNum; i++) \
			dst[i] = src[i]; \
	}*/

void CvGlobals::uninit() // free
{
	// See also CvXMLLoadUtilityInit::CleanUpGlobalVariables()
	// <advc.003o>
	#ifdef USE_TSC_PROFILER
	TSCProfiler::getInstance().writeFile();
	#endif // </advc.003o>
	SAFE_DELETE_ARRAY(m_aiGlobalDefinesCache); // advc.003t

	SAFE_DELETE(m_game);
	SAFE_DELETE(m_map);

	CvPlayer::freeStatics();
	CvTeam::freeStatics();

	SAFE_DELETE(m_asyncRand);
	SAFE_DELETE(m_pPythonCaller); // advc.003y
	SAFE_DELETE(m_pLogger); // advc.003t
	SAFE_DELETE(m_initCore);
	SAFE_DELETE(m_loadedInitCore);
	SAFE_DELETE(m_iniInitCore);
	gDLL->uninitGlobals();	// free globals allocated outside the dll
	SAFE_DELETE(m_VarSystem);

	// already deleted outside of the dll, set to null for safety
	m_messageQueue=NULL; m_hotJoinMsgQueue=NULL; m_messageControl=NULL;
	m_setupData=NULL; m_messageCodes=NULL; m_dropMgr=NULL;
	m_portal=NULL; m_statsReporter=NULL; m_interface=NULL;
	m_diplomacyScreen=NULL; m_mpDiplomacyScreen=NULL; m_pathFinder=NULL;
	m_interfacePathFinder=NULL; m_stepFinder=NULL; m_routeFinder=NULL;
	m_borderFinder=NULL; m_areaFinder=NULL; m_plotGroupFinder=NULL;

	m_typesMap.clear();
	m_aInfoVectors.clear();
}

void CvGlobals::clearTypesMap()
{
	m_typesMap.clear();
	if (m_VarSystem)
		m_VarSystem->UnInit();
}


CvDiplomacyScreen* CvGlobals::getDiplomacyScreen()
{
	return m_diplomacyScreen;
}

CMPDiplomacyScreen* CvGlobals::getMPDiplomacyScreen()
{
	return m_mpDiplomacyScreen;
}

CvMessageCodeTranslator& CvGlobals::getMessageCodes()
{
	return *m_messageCodes;
}

FMPIManager*& CvGlobals::getFMPMgrPtr()
{
	return m_pFMPMgr;
}

CvPortal& CvGlobals::getPortal()
{
	return *m_portal;
}

CvSetupData& CvGlobals::getSetupData()
{
	return *m_setupData;
}

CvInitCore& CvGlobals::getLoadedInitCore()
{
	return *m_loadedInitCore;
}

CvInitCore& CvGlobals::getIniInitCore()
{
	return *m_iniInitCore;
}

CvStatsReporter& CvGlobals::getStatsReporter()
{
	return *m_statsReporter;
}

CvStatsReporter* CvGlobals::getStatsReporterPtr()
{
	return m_statsReporter;
}

CvInterface& CvGlobals::getInterface()
{
	return *m_interface;
}

CvInterface* CvGlobals::getInterfacePtr()
{
	return m_interface;
}

CMessageQueue& CvGlobals::getMessageQueue()
{
	return *m_messageQueue;
}

CMessageQueue& CvGlobals::getHotMessageQueue()
{
	return *m_hotJoinMsgQueue;
}

CMessageControl& CvGlobals::getMessageControl()
{
	return *m_messageControl;
}

CvDropMgr& CvGlobals::getDropMgr()
{
	return *m_dropMgr;
}

// advc.003j: unused
//NiPoint3& CvGlobals::getPt3Origin() { return m_pt3Origin; }
//NiPoint3& CvGlobals::getPt3CameraDir() { return m_pt3CameraDir; }

std::vector<CvInterfaceModeInfo*>& CvGlobals::getInterfaceModeInfo()
{
	return m_paInterfaceModeInfo;
}

CvColorInfo& CvGlobals::getColorInfo(ColorTypes eColor)
{
	FAssert(eColor > -1);
	/*  <advc.106i> So that AdvCiv is able to show replays from mods with
		extra colors. */
	if(eColor >= getNumColorInfos())
	{
		FAssert(m_bHoFScreenUp || eColor < getNumColorInfos());
		// +7: Skip colors from COLOR_CLEAR to COLOR_LIGHT_GREY
		eColor = (ColorTypes)((eColor + 7) % getNumColorInfos());
	} // </advc.106i>
	return getInfo(eColor); // advc.enum
}

int CvGlobals::getNumThroneRoomInfos()
{
	loadThroneRoomInfo(); // advc.003v: Load it as late as possible
	// <advc.enum>
	CvGlobals const& kThis = *this;
	return kThis.getNumThroneRoomInfos(); // </advc.enum>
}

void CvGlobals::setActiveLandscapeID(int iLandscapeID)
{
	m_iActiveLandscapeID = iLandscapeID;
}
// <advc.003x>
int CvGlobals::getLandscapePlotsPerCellX() const
{
	return getLandscapeInfo(getActiveLandscapeID()).getPlotsPerCellX();
}

int CvGlobals::getLandscapePlotsPerCellY() const
{
	return getLandscapeInfo(getActiveLandscapeID()).getPlotsPerCellY();
} // </advc.003x>

int& CvGlobals::getNumPlayableCivilizationInfos()
{
	return m_iNumPlayableCivilizationInfos;
}

int& CvGlobals::getNumAIPlayableCivilizationInfos()
{
	return m_iNumAIPlayableCivilizationInfos;
}

int& CvGlobals::getNumEntityEventTypes()
{
	return m_iNumEntityEventTypes;
}

CvString*& CvGlobals::getEntityEventTypes()
{
	return m_paszEntityEventTypes;
}

CvString& CvGlobals::getEntityEventTypes(EntityEventTypes e)
{
	FASSERT_BOUNDS(0, getNumEntityEventTypes(), e, "CvGlobals::getEntityEventTypes");
	return m_paszEntityEventTypes[e];
}

int& CvGlobals::getNumAnimationOperatorTypes()
{
	return m_iNumAnimationOperatorTypes;
}

CvString*& CvGlobals::getAnimationOperatorTypes()
{
	return m_paszAnimationOperatorTypes;
}

CvString& CvGlobals::getAnimationOperatorTypes(AnimationOperatorTypes e)
{
	FASSERT_BOUNDS(0, getNumAnimationOperatorTypes(), e, "CvGlobals::getAnimationOperatorTypes");
	return m_paszAnimationOperatorTypes[e];
}

CvString*& CvGlobals::getFunctionTypes()
{
	return m_paszFunctionTypes;
}

CvString& CvGlobals::getFunctionTypes(FunctionTypes e)
{
	FASSERT_BOUNDS(0, NUM_FUNC_TYPES, e, "CvGlobals::getFunctionTypes");
	return m_paszFunctionTypes[e];
}

CvString*& CvGlobals::getFlavorTypes()
{
	return m_paszFlavorTypes;
}

CvString& CvGlobals::getFlavorTypes(FlavorTypes e)
{
	FASSERT_BOUNDS(0, getNumFlavorTypes(), e, "CvGlobals::getFlavorTypes");
	return m_paszFlavorTypes[e];
}

int& CvGlobals::getNumArtStyleTypes()
{
	return m_iNumArtStyleTypes;
}

CvString*& CvGlobals::getArtStyleTypes()
{
	return m_paszArtStyleTypes;
}

CvString& CvGlobals::getArtStyleTypes(ArtStyleTypes e)
{
	FASSERT_BOUNDS(0, getNumArtStyleTypes(), e, "CvGlobals::getArtStyleTypes");
	return m_paszArtStyleTypes[e];
}

CvString*& CvGlobals::getCitySizeTypes()
{
	return m_paszCitySizeTypes;
}

CvString& CvGlobals::getCitySizeTypes(CitySizeTypes e)
{
	FASSERT_BOUNDS(0, NUM_CITYSIZE_TYPES, e, "CvGlobals::getCitySizeTypes");
	return m_paszCitySizeTypes[e];
}

CvString*& CvGlobals::getContactTypes()
{
	return m_paszContactTypes;
}

CvString& CvGlobals::getContactTypes(ContactTypes e)
{
	FASSERT_BOUNDS(0, NUM_CONTACT_TYPES, e, "CvGlobals::getContactTypes");
	return m_paszContactTypes[e];
}

CvString*& CvGlobals::getDiplomacyPowerTypes()
{
	return m_paszDiplomacyPowerTypes;
}

CvString& CvGlobals::getDiplomacyPowerTypes(DiplomacyPowerTypes e)
{
	FASSERT_BOUNDS(0, NUM_DIPLOMACYPOWER_TYPES, e, "CvGlobals::getDiplomacyPowerTypes");
	return m_paszDiplomacyPowerTypes[e];
}

CvString*& CvGlobals::getAutomateTypes()
{
	return m_paszAutomateTypes;
}

CvString& CvGlobals::getAutomateTypes(AutomateTypes e)
{
	FASSERT_BOUNDS(0, NUM_AUTOMATE_TYPES, e, "CvGlobals::getAutomateTypes");
	return m_paszAutomateTypes[e];
}

CvString*& CvGlobals::getDirectionTypes()
{
	return m_paszDirectionTypes;
}

CvString& CvGlobals::getDirectionTypes(AutomateTypes e)
{
	FASSERT_BOUNDS(0, NUM_DIRECTION_TYPES, e, "CvGlobals::getDirectionTypes");
	return m_paszDirectionTypes[e];
}

int& CvGlobals::getNumFootstepAudioTypes()
{
	return m_iNumFootstepAudioTypes;
}

CvString*& CvGlobals::getFootstepAudioTypes()
{
	return m_paszFootstepAudioTypes;
}

CvString& CvGlobals::getFootstepAudioTypes(int i)
{
	FASSERT_BOUNDS(0, getNumFootstepAudioTypes(), i, "CvGlobals::getFootstepAudioTypes");
	return m_paszFootstepAudioTypes[i];
}

int CvGlobals::getFootstepAudioTypeByTag(CvString strTag)
{
	int iIndex = -1;
	if (strTag.GetLength() <= 0)
		return iIndex;
	for (int i = 0; i < m_iNumFootstepAudioTypes; i++)
	{
		if (strTag.CompareNoCase(m_paszFootstepAudioTypes[i]) == 0)
		{
			iIndex = i;
			break;
		}
	}
	return iIndex;
}

CvString*& CvGlobals::getFootstepAudioTags()
{
	return m_paszFootstepAudioTags;
}

CvString& CvGlobals::getFootstepAudioTags(int i)
{
	/*  advc: Upper-bound check added; apparently, there are as many types a tags.
		A variable m_iNumFootstepAudioTags was unused, so I removed it. */
	FASSERT_BOUNDS(0, getNumFootstepAudioTypes(), i, "CvGlobals::getFootstepAudioTags");
	return m_paszFootstepAudioTags[i];
}

void CvGlobals::setCurrentXMLFile(const TCHAR* szFileName)
{
	// <advc.006e>
	if (szFileName == NULL)
		m_szCurrentXMLFile = "(None)"; // </advc.006e>
	else m_szCurrentXMLFile = szFileName;
}

CvString const& CvGlobals::getCurrentXMLFile() const
{
	return m_szCurrentXMLFile;
}
// <advc.003v>
// It seems that the DLL wasn't keeping a handle to the XMLLoadUtility so far
void CvGlobals::setXMLLoadUtility(CvXMLLoadUtility* pXML)
{
	m_pXMLLoadUtility = pXML;
}

void CvGlobals::loadOptionalXMLInfo()
{
	bool bSuccess = false;
	if (m_pXMLLoadUtility != NULL)
		bSuccess = m_pXMLLoadUtility->LoadOptionalGlobals();
	FAssertMsg(bSuccess, "Failed to load optional XML data");
}

void CvGlobals::loadThroneRoomInfo()
{
	bool bSuccess = false;
	if (m_pXMLLoadUtility != NULL)
		bSuccess = m_pXMLLoadUtility->LoadThroneRoomInfo();
	FAssertMsg(bSuccess, "Failed to load XML data for Throne Room");
} // </advc.003v>
// advc.003t:
#define MAKE_STRING(VAR) #VAR,
// <advc.003t>
void CvGlobals::cacheGlobalInts(char const* szChangedDefine, int iNewValue)
{
	const char* const aszGlobalDefinesTagNames[] = {
		DO_FOR_EACH_GLOBAL_DEFINE(MAKE_STRING)
	};
	FAssert(sizeof(aszGlobalDefinesTagNames) / sizeof(char*) == NUM_GLOBAL_DEFINES);

	if (szChangedDefine != NULL) // Cache update
	{
		for (int i = 0; i < NUM_GLOBAL_DEFINES; i++)
		{
			if (std::strcmp(aszGlobalDefinesTagNames[i], szChangedDefine) == 0)
			{
				m_aiGlobalDefinesCache[i] = iNewValue;
				break;
			}
		}
		return;
	}

	// Initialize cache (or full update)
	SAFE_DELETE_ARRAY(m_aiGlobalDefinesCache);
	m_aiGlobalDefinesCache = new int[NUM_GLOBAL_DEFINES];
	for (int i = 0; i < NUM_GLOBAL_DEFINES; i++)
	{
		/*  Let's not throw away the default values from BBAI
			(though they should of course not be needed) */
		int iDefault = 0;
		switch((GlobalDefines)i)
		{
		// BETTER_BTS_AI_MOD, Efficiency, Options, 02/21/10, jdog5000: START
		// BBAI AI Variables
		case WAR_SUCCESS_CITY_CAPTURING: iDefault = 25; break;
		case BBAI_ATTACK_CITY_STACK_RATIO: iDefault = 110; break;
		case BBAI_SKIP_BOMBARD_BASE_STACK_RATIO: iDefault = 300; break;
		case BBAI_SKIP_BOMBARD_MIN_STACK_RATIO: iDefault = 140; break;
		//case TECH_COST_FIRST_KNOWN_PREREQ_MODIFIER: iDefault = 20; break; // advc.910: Should be 0 also by default
		case TECH_COST_FIRST_KNOWN_PREREQ_MODIFIER: iDefault = 20; break;
		case TECH_COST_KNOWN_PREREQ_MODIFIER: iDefault = 20; break;
		// From Lead From Behind by UncutDragon
		case LFB_ENABLE: iDefault = 1; break;
		case LFB_BASEDONGENERAL: iDefault = 1; break;
		case LFB_BASEDONEXPERIENCE: iDefault = 1; break;
		case LFB_BASEDONLIMITED: iDefault = 1; break;
		case LFB_BASEDONHEALER: iDefault = 1; break;
		case LFB_DEFENSIVEADJUSTMENT: iDefault = 1; break;
		case LFB_USESLIDINGSCALE: iDefault = 1; break;
		case LFB_ADJUSTNUMERATOR: iDefault = 1; break;
		case LFB_ADJUSTDENOMINATOR: iDefault = 3; break;
		case LFB_USECOMBATODDS: iDefault = 1; break;
		case COMBAT_DIE_SIDES: iDefault = -1; break;
		case COMBAT_DAMAGE: iDefault = -1; break;
		// BETTER_BTS_AI_MOD: END
		}
		m_aiGlobalDefinesCache[i] = getDefineINT(aszGlobalDefinesTagNames[i], iDefault);
	} // </advc.003t>
}

void CvGlobals::cacheGlobalFloats()
{
	m_fPOWER_CORRECTION = getDefineFLOAT("POWER_CORRECTION"); // advc.104

	m_fCAMERA_MIN_YAW = getDefineFLOAT("CAMERA_MIN_YAW");
	m_fCAMERA_MAX_YAW = getDefineFLOAT("CAMERA_MAX_YAW");
	m_fCAMERA_FAR_CLIP_Z_HEIGHT = getDefineFLOAT("CAMERA_FAR_CLIP_Z_HEIGHT");
	m_fCAMERA_MAX_TRAVEL_DISTANCE = getDefineFLOAT("CAMERA_MAX_TRAVEL_DISTANCE");
	m_fCAMERA_START_DISTANCE = getDefineFLOAT("CAMERA_START_DISTANCE");
	m_fAIR_BOMB_HEIGHT = getDefineFLOAT("AIR_BOMB_HEIGHT");
	m_fPLOT_SIZE = getDefineFLOAT("PLOT_SIZE");
	m_fCAMERA_SPECIAL_PITCH = getDefineFLOAT("CAMERA_SPECIAL_PITCH");
	m_fCAMERA_MAX_TURN_OFFSET = getDefineFLOAT("CAMERA_MAX_TURN_OFFSET");
	m_fCAMERA_MIN_DISTANCE = getDefineFLOAT("CAMERA_MIN_DISTANCE");
	m_fCAMERA_UPPER_PITCH = getDefineFLOAT("CAMERA_UPPER_PITCH");
	m_fCAMERA_LOWER_PITCH = getDefineFLOAT("CAMERA_LOWER_PITCH");
	m_fFIELD_OF_VIEW = getDefineFLOAT("FIELD_OF_VIEW");
	m_fSHADOW_SCALE = getDefineFLOAT("SHADOW_SCALE");
	m_fUNIT_MULTISELECT_DISTANCE = getDefineFLOAT("UNIT_MULTISELECT_DISTANCE");
}

void CvGlobals::cacheGlobals()
{
	// <advc.003t> Moved into subroutines to allow partial updates
	cacheGlobalInts();
	cacheGlobalFloats();
	// Strings: Mostly can't cache these here (too early) // </advc.003t>
	// <advc.003y>
	// New class to handle Python callback defines
	m_pPythonCaller = new CvPythonCaller();
	// Some of the callback defines are handled by CvDllPythonEvents
	CvEventReporter::getInstance().initPythonCallbackGuards();
}

// <advc.opt>
void CvGlobals::setRUINS_IMPROVEMENT(int iValue)
{
	m_iRUINS_IMPROVEMENT = iValue;
}

void CvGlobals::setWATER_TERRAIN(bool bShallow, int iValue)
{
	m_aiWATER_TERRAIN[bShallow] = iValue;
} 

void CvGlobals::setDEFAULT_SPECIALIST(int iValue)
{
	m_iDEFAULT_SPECIALIST = iValue;
} // </advc.opt>

int CvGlobals::getDefineINT(const char * szName,
		// BETTER_BTS_AI_MOD, 02/21/10, jdog5000: START
		const int iDefault) const
{
	int iReturn = iDefault;
	// BETTER_BTS_AI_MOD: END
	bool bSuccess = // advc.003c
			getDefinesVarSystem()->GetValue(szName, iReturn);
	FAssert(bSuccess); // advc.003c
	return iReturn;
}


float CvGlobals::getDefineFLOAT(const char * szName) const
{
	float fReturn = 0;
	bool bSuccess = // advc.003c
			getDefinesVarSystem()->GetValue(szName, fReturn);
	/*  advc.003c: The EXE queries CAMERA_MIN_DISTANCE during startup, which
		fails but doesn't cause any problems. */
	FAssert(bSuccess || std::strcmp("CAMERA_MIN_DISTANCE", szName) == 0);
	return fReturn;
}

const char * CvGlobals::getDefineSTRING(const char * szName) const
{
	const char * szReturn = NULL;
	bool bSuccess = // advc.003c
			getDefinesVarSystem()->GetValue(szName, szReturn);
	FAssert(bSuccess); // advc.003c
	return szReturn;
}

void CvGlobals::setDefineINT(const char * szName, int iValue, /* advc.003t: */ bool bUpdateCache)
{
	getDefinesVarSystem()->SetValue(szName, iValue);
	// <advc.003t>
	if(bUpdateCache)
		cacheGlobalInts(szName, iValue); // Pinpoint update </advc.003t>
}

void CvGlobals::setDefineFLOAT(const char * szName, float fValue, /* advc.003t: */ bool bUpdateCache)
{
	getDefinesVarSystem()->SetValue(szName, fValue);
	// <advc.003t>
	if(bUpdateCache)
		cacheGlobalFloats(); // </advc.003t>
}

void CvGlobals::setDefineSTRING(const char * szName, const char * szValue, /* advc.opt: */ bool bUpdateCache)
{
	getDefinesVarSystem()->SetValue(szName, szValue);
	//cacheGlobals();
	FAssertMsg(!bUpdateCache, "No strings to update"); // advc.003t
}

int CvGlobals::getMAX_CIV_PLAYERS()
{
	return MAX_CIV_PLAYERS;
}
/*  <advc.003t> Optional parameters added. The return value is only an upper bound,
	even if an argument is given. */
int CvGlobals::getNUM_UNIT_PREREQ_OR_BONUSES(UnitTypes eUnit) const
{
	return (eUnit == NO_UNIT || getInfo(eUnit).isAnyPrereqOrBonus() ?
			getDefineINT(NUM_UNIT_PREREQ_OR_BONUSES) : 0);
}

int CvGlobals::getNUM_UNIT_AND_TECH_PREREQS(UnitTypes eUnit) const
{
	return (eUnit == NO_UNIT || getInfo(eUnit).isAnyPrereqAndTech() ?
			getDefineINT(NUM_UNIT_AND_TECH_PREREQS) : 0);
}

int CvGlobals::getNUM_BUILDING_PREREQ_OR_BONUSES(BuildingTypes eBuilding) const
{
	return (eBuilding == NO_BUILDING || getInfo(eBuilding).isAnyPrereqOrBonus() ?
			getDefineINT(NUM_BUILDING_PREREQ_OR_BONUSES) : 0);
}

int CvGlobals::getNUM_BUILDING_AND_TECH_PREREQS(BuildingTypes eBuilding) const
{
	return (eBuilding == NO_BUILDING || getInfo(eBuilding).isAnyPrereqAndTech() ?
			getDefineINT(NUM_BUILDING_AND_TECH_PREREQS) : 0);
}

int CvGlobals::getNUM_AND_TECH_PREREQS(TechTypes eTech) const
{
	return (eTech == NO_TECH || getInfo(eTech).isAnyPrereqAndTech() ?
			getDefineINT(NUM_AND_TECH_PREREQS) : 0);
}

int CvGlobals::getNUM_OR_TECH_PREREQS(TechTypes eTech) const
{
	return (eTech == NO_TECH || getInfo(eTech).isAnyPrereqOrTech() ?
			getDefineINT(NUM_OR_TECH_PREREQS) : 0);
}

int CvGlobals::getNUM_ROUTE_PREREQ_OR_BONUSES(RouteTypes eRoute) const
{
	return (eRoute == NO_ROUTE || getInfo(eRoute).isAnyPrereqOrBonus() ?
			getDefineINT(NUM_ROUTE_PREREQ_OR_BONUSES) : 0);
}
// </advc.003t>
int CvGlobals::getNUM_CORPORATION_PREREQ_BONUSES() const
{
	return getDefineINT(NUM_CORPORATION_PREREQ_BONUSES);
}

int CvGlobals::getUSE_FINISH_TEXT_CALLBACK()
{
	return static_cast<int>(getPythonCaller()->isUseFinishTextCallback()); // advc.003y
}

void CvGlobals::setDLLIFace(CvDLLUtilityIFaceBase* pDll)
{
	m_pDLL = pDll;
}

void CvGlobals::setDLLProfiler(FProfiler* prof)
{
	m_Profiler=prof;
}

FProfiler* CvGlobals::getDLLProfiler()
{
	return m_Profiler;
}

void CvGlobals::enableDLLProfiler(bool bEnable)
{
	m_bDLLProfiler = bEnable;
}

bool CvGlobals::isDLLProfilerEnabled() const
{
	//return m_bDLLProfiler;
	// K-Mod. (I don't know how to enable this in-game...)
#ifdef FP_PROFILE_ENABLE
	return true;
#else
	return false;
#endif
	// K-Mod end
}


//
// Global Types Hash Map
//

int CvGlobals::getTypesEnum(const char* szType, /* advc.006: */ bool bHideAssert) const
{
	FAssertMsg(szType, "null type string");
	TypesMap::const_iterator it = m_typesMap.find(szType);
	if (it != m_typesMap.end())
		return it->second;
	/*FAssertMsg(strcmp(szType, "NONE")==0 || strcmp(szType, "")==0, CvString::format("type %s not found", szType).c_str());
	return -1;*/
	/*  advc.006: Replacing the above with code from getInfoTypeForString, which
		now calls this function. */
	//if(!bHideAssert)
	if (!bHideAssert && /* K-Mod: */ !(strcmp(szType, "")==0 || strcmp(szType, "NONE")==0))
	{
		char const* szCurrentXMLFile = getCurrentXMLFile().GetCString();
		CvString szError;
		szError.Format("type %s not found, Current XML file is: %s", szType, szCurrentXMLFile);
		FAssertMsg(false, szError.c_str());
		gDLL->logMsg("xml.log", szError);
	}
	return -1;
}

void CvGlobals::setTypesEnum(const char* szType, int iEnum)
{
	FAssertMsg(szType, "null type string");
	FAssertMsg(m_typesMap.find(szType)==m_typesMap.end(), "types entry already exists");
	m_typesMap[szType] = iEnum;
}

// <advc.003c>
bool CvGlobals::isCachingDone() const
{
	return m_aiGlobalDefinesCache != NULL;
} // </advc.003c>

// <advc.106i>
void CvGlobals::setHoFScreenUp(bool b)
{
	m_bHoFScreenUp = b;
} // </advc.106i>

//
// Global Infos Hash Map
//

int CvGlobals::getInfoTypeForString(const char* szType, bool bHideAssert) const
{
	FAssertMsg(szType, "null info type string");
	InfosMap::const_iterator it = m_infosMap.find(szType);
	if (it != m_infosMap.end())
		return it->second;
	/*  advc.006: Fall back on getTypesEnum. (Needed for advc.003x - though I don't
		quite understand why it worked before I split up CvInfos.h.)
		Assertion code moved there. */
	return getTypesEnum(szType, bHideAssert);
}

void CvGlobals::setInfoTypeFromString(const char* szType, int idx)
{
	FAssertMsg(szType, "null info type string");
#ifdef _DEBUG
	InfosMap::const_iterator it = m_infosMap.find(szType);
	int iExisting = (it!=m_infosMap.end()) ? it->second : -1;
	FAssertMsg(iExisting==-1 || iExisting==idx || strcmp(szType, "ERROR")==0, CvString::format("xml info type entry %s already exists", szType).c_str());
#endif
	m_infosMap[szType] = idx;
}

void CvGlobals::infoTypeFromStringReset()
{
	m_infosMap.clear();
}

//
// non-inline versions
// <advc.inl>
CvMap& CvGlobals::getMapExternal() { return getMap(); }
CvGameAI& CvGlobals::getGameExternal() { return AI_getGame(); } // </advc.inl>
CvGameAI *CvGlobals::getGamePointer(){ return m_game; }

int CvGlobals::getMaxCivPlayers() const
{
	return MAX_CIV_PLAYERS;
}

bool CvGlobals::IsGraphicsInitialized() const { return m_bGraphicsInitialized;}

// advc: onGraphicsInitialized call added
void CvGlobals::SetGraphicsInitialized(bool bVal)
{
	if(bVal == m_bGraphicsInitialized)
		return;
	m_bGraphicsInitialized = bVal;
	if(m_bGraphicsInitialized)
		getGame().onGraphicsInitialized();
}

namespace // advc: Hide in unnamed namespace
{
	template <class T>
	void deleteInfoArray(std::vector<T*>& array)
	{
		for (std::vector<T*>::iterator it = array.begin(); it != array.end(); ++it)
		{
			SAFE_DELETE(*it);
		}

		array.clear();
	}

	template <class T>
	bool readInfoArray(FDataStreamBase* pStream, std::vector<T*>& array, const char* szClassName)
	{
	#if SERIALIZE_CVINFOS
		addToInfosVectors(&array);

		int iSize;
		pStream->Read(&iSize);
		FAssertMsg(iSize==sizeof(T), CvString::format("class size doesn't match cache size - check info read/write functions:%s", szClassName).c_str());
		if (iSize!=sizeof(T))
			return false;
		pStream->Read(&iSize);

		deleteInfoArray(array);

		for (int i = 0; i < iSize; ++i)
		{
			array.push_back(new T);
		}

		int iIndex = 0;
		for (std::vector<T*>::iterator it = array.begin(); it != array.end(); ++it)
		{
			(*it)->read(pStream);
			setInfoTypeFromString((*it)->getType(), iIndex);
			++iIndex;
		}

		return true;
	#else
		FAssert(false);
		return false;
	#endif
	}

	template <class T>
	bool writeInfoArray(FDataStreamBase* pStream,  std::vector<T*>& array)
	{
	#if SERIALIZE_CVINFOS
		int iSize = sizeof(T);
		pStream->Write(iSize);
		pStream->Write(array.size());
		for (std::vector<T*>::iterator it = array.begin(); it != array.end(); ++it)
		{
			(*it)->write(pStream);
		}
		return true;
	#else
		FAssert(false);
		return false;
	#endif
	}
} // advc: end of unnamed namespace

void CvGlobals::deleteInfoArrays()
{
	deleteInfoArray(m_paWorldInfo);
	// <advc.enum>
	#define DELETE_INFO_ARRAY(Name, Dummy) deleteInfoArray(m_pa##Name##Info);
	DO_FOR_EACH_INFO_TYPE(DELETE_INFO_ARRAY); // </advc.enum>

	SAFE_DELETE_ARRAY(getEntityEventTypes());
	SAFE_DELETE_ARRAY(getAnimationOperatorTypes());
	SAFE_DELETE_ARRAY(getFunctionTypes());
	SAFE_DELETE_ARRAY(getFlavorTypes());
	SAFE_DELETE_ARRAY(getArtStyleTypes());
	SAFE_DELETE_ARRAY(getCitySizeTypes());
	SAFE_DELETE_ARRAY(getContactTypes());
	SAFE_DELETE_ARRAY(getDiplomacyPowerTypes());
	SAFE_DELETE_ARRAY(getAutomateTypes());
	SAFE_DELETE_ARRAY(getDirectionTypes());
	SAFE_DELETE_ARRAY(getFootstepAudioTypes());
	SAFE_DELETE_ARRAY(getFootstepAudioTags());

	clearTypesMap();
	m_aInfoVectors.clear();
}

// (advc.003i: Only used by readInfoArray, i.e. unused if XML cache disabled.)
void CvGlobals::addToInfosVectors(void* infoVector)
{
	std::vector<CvInfoBase*>* infoBaseVector = reinterpret_cast<std::vector<CvInfoBase*>*>(infoVector);
	m_aInfoVectors.push_back(infoBaseVector);
}

bool CvGlobals::readBuildingInfoArray(FDataStreamBase* pStream)
{
	return readInfoArray(pStream, m_paBuildingInfo, "CvBuildingInfo");
}

void CvGlobals::writeBuildingInfoArray(FDataStreamBase* pStream)
{
	writeInfoArray(pStream, m_paBuildingInfo);
}

bool CvGlobals::readTechInfoArray(FDataStreamBase* pStream)
{
	return readInfoArray(pStream, m_paTechInfo, "CvTechInfo");
}

void CvGlobals::writeTechInfoArray(FDataStreamBase* pStream)
{
	writeInfoArray(pStream, m_paTechInfo);
}

bool CvGlobals::readUnitInfoArray(FDataStreamBase* pStream)
{
	return readInfoArray(pStream, m_paUnitInfo, "CvUnitInfo");
}

void CvGlobals::writeUnitInfoArray(FDataStreamBase* pStream)
{
	writeInfoArray(pStream, m_paUnitInfo);
}

bool CvGlobals::readLeaderHeadInfoArray(FDataStreamBase* pStream)
{
	return readInfoArray(pStream, m_paLeaderHeadInfo, "CvLeaderHeadInfo");
}

void CvGlobals::writeLeaderHeadInfoArray(FDataStreamBase* pStream)
{
	writeInfoArray(pStream, m_paLeaderHeadInfo);
}

bool CvGlobals::readCivilizationInfoArray(FDataStreamBase* pStream)
{
	return readInfoArray(pStream, m_paCivilizationInfo, "CvCivilizationInfo");
}

void CvGlobals::writeCivilizationInfoArray(FDataStreamBase* pStream)
{
	writeInfoArray(pStream, m_paCivilizationInfo);
}

bool CvGlobals::readPromotionInfoArray(FDataStreamBase* pStream)
{
	return readInfoArray(pStream, m_paPromotionInfo, "CvPromotionInfo");
}

void CvGlobals::writePromotionInfoArray(FDataStreamBase* pStream)
{
	writeInfoArray(pStream, m_paPromotionInfo);
}

bool CvGlobals::readDiplomacyInfoArray(FDataStreamBase* pStream)
{
	return readInfoArray(pStream, m_paDiplomacyInfo, "CvDiplomacyInfo");
}

void CvGlobals::writeDiplomacyInfoArray(FDataStreamBase* pStream)
{
	writeInfoArray(pStream, m_paDiplomacyInfo);
}

bool CvGlobals::readCivicInfoArray(FDataStreamBase* pStream)
{
	return readInfoArray(pStream, m_paCivicInfo, "CvCivicInfo");
}

void CvGlobals::writeCivicInfoArray(FDataStreamBase* pStream)
{
	writeInfoArray(pStream, m_paCivicInfo);
}

bool CvGlobals::readHandicapInfoArray(FDataStreamBase* pStream)
{
	return readInfoArray(pStream, m_paHandicapInfo, "CvHandicapInfo");
}

void CvGlobals::writeHandicapInfoArray(FDataStreamBase* pStream)
{
	writeInfoArray(pStream, m_paHandicapInfo);
}

bool CvGlobals::readBonusInfoArray(FDataStreamBase* pStream)
{
	return readInfoArray(pStream, m_paBonusInfo, "CvBonusInfo");
}

void CvGlobals::writeBonusInfoArray(FDataStreamBase* pStream)
{
	writeInfoArray(pStream, m_paBonusInfo);
}

bool CvGlobals::readImprovementInfoArray(FDataStreamBase* pStream)
{
	return readInfoArray(pStream, m_paImprovementInfo, "CvImprovementInfo");
}

void CvGlobals::writeImprovementInfoArray(FDataStreamBase* pStream)
{
	writeInfoArray(pStream, m_paImprovementInfo);
}

bool CvGlobals::readEventInfoArray(FDataStreamBase* pStream)
{
	return readInfoArray(pStream, m_paEventInfo, "CvEventInfo");
}

void CvGlobals::writeEventInfoArray(FDataStreamBase* pStream)
{
	writeInfoArray(pStream, m_paEventInfo);
}

bool CvGlobals::readEventTriggerInfoArray(FDataStreamBase* pStream)
{
	return readInfoArray(pStream, m_paEventTriggerInfo, "CvEventTriggerInfo");
}

void CvGlobals::writeEventTriggerInfoArray(FDataStreamBase* pStream)
{
	writeInfoArray(pStream, m_paEventTriggerInfo);
}

int CvGlobals::getNUM_ENGINE_DIRTY_BITS() const
{
	return NUM_ENGINE_DIRTY_BITS;
}

int CvGlobals::getNUM_INTERFACE_DIRTY_BITS() const
{
	return NUM_INTERFACE_DIRTY_BITS;
}

int CvGlobals::getNUM_YIELD_TYPES() const
{
	return NUM_YIELD_TYPES;
}

int CvGlobals::getNUM_FORCECONTROL_TYPES() const
{
	return NUM_FORCECONTROL_TYPES;
}

int CvGlobals::getNUM_INFOBAR_TYPES() const
{
	return NUM_INFOBAR_TYPES;
}

int CvGlobals::getNUM_HEALTHBAR_TYPES() const
{
	return NUM_HEALTHBAR_TYPES;
}

int CvGlobals::getNUM_LEADERANIM_TYPES() const
{
	return NUM_LEADERANIM_TYPES;
}

void CvGlobals::infosReset()
{
	for (int i = 0; i < (int)m_aInfoVectors.size(); i++)
	{
		std::vector<CvInfoBase *> *infoBaseVector = m_aInfoVectors[i];
		for (int j = 0; j < (int)infoBaseVector->size(); j++)
			infoBaseVector->at(j)->reset();
	}
}

int CvGlobals::getNumDirections() const { return NUM_DIRECTION_TYPES; }
int CvGlobals::getNumGameOptions() const { return NUM_GAMEOPTION_TYPES; }
int CvGlobals::getNumMPOptions() const { return NUM_MPOPTION_TYPES; }
int CvGlobals::getNumSpecialOptions() const { return NUM_SPECIALOPTION_TYPES; }
int CvGlobals::getNumGraphicOptions() const { return NUM_GRAPHICOPTION_TYPES; }
int CvGlobals::getNumTradeableItems() const { return NUM_TRADEABLE_ITEMS; }
int CvGlobals::getNumBasicItems() const { return NUM_BASIC_ITEMS; }
int CvGlobals::getNumTradeableHeadings() const { return NUM_TRADEABLE_HEADINGS; }
int CvGlobals::getNumPlayerOptionInfos() const { return NUM_PLAYEROPTION_TYPES; }
int CvGlobals::getMaxNumSymbols() const { return MAX_NUM_SYMBOLS; }
int CvGlobals::getNumGraphicLevels() const { return NUM_GRAPHICLEVELS; }

void CvGlobals::setInterface(CvInterface* pVal) { m_interface = pVal; }
void CvGlobals::setDiplomacyScreen(CvDiplomacyScreen* pVal) { m_diplomacyScreen = pVal; }
void CvGlobals::setMPDiplomacyScreen(CMPDiplomacyScreen* pVal) { m_mpDiplomacyScreen = pVal; }
void CvGlobals::setMessageQueue(CMessageQueue* pVal) { m_messageQueue = pVal; }
void CvGlobals::setHotJoinMessageQueue(CMessageQueue* pVal) { m_hotJoinMsgQueue = pVal; }
void CvGlobals::setMessageControl(CMessageControl* pVal) { m_messageControl = pVal; }
void CvGlobals::setSetupData(CvSetupData* pVal) { m_setupData = pVal; }
void CvGlobals::setMessageCodeTranslator(CvMessageCodeTranslator* pVal) { m_messageCodes = pVal; }
void CvGlobals::setDropMgr(CvDropMgr* pVal) { m_dropMgr = pVal; }
void CvGlobals::setPortal(CvPortal* pVal) { m_portal = pVal; }
void CvGlobals::setStatsReport(CvStatsReporter* pVal) { m_statsReporter = pVal; }
void CvGlobals::setPathFinder(FAStar* pVal) { m_pathFinder = pVal; }
void CvGlobals::setInterfacePathFinder(FAStar* pVal) { m_interfacePathFinder = pVal; }
void CvGlobals::setStepFinder(FAStar* pVal) { m_stepFinder = pVal; }
void CvGlobals::setRouteFinder(FAStar* pVal) { m_routeFinder = pVal; }
void CvGlobals::setBorderFinder(FAStar* pVal) { m_borderFinder = pVal; }
void CvGlobals::setAreaFinder(FAStar* pVal) { m_areaFinder = pVal; }
void CvGlobals::setPlotGroupFinder(FAStar* pVal) { m_plotGroupFinder = pVal; }
CvDLLUtilityIFaceBase* CvGlobals::getDLLIFaceNonInl() { return m_pDLL; }
