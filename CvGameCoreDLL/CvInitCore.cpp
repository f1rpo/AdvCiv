// game.cpp

#include "CvGameCoreDLL.h"
#include "CvInitCore.h"
#include "CvPlayer.h"
#include "CvInfo_GameOption.h"
#include "CvDLLUtilityIFaceBase.h"


CvInitCore::CvInitCore()
{
	m_abOptions = new bool[NUM_GAMEOPTION_TYPES];
	m_abMPOptions = new bool[NUM_MPOPTION_TYPES];
	m_abForceControls = new bool[NUM_FORCECONTROL_TYPES];

	m_aszLeaderName = new CvWString[MAX_PLAYERS];
	m_aszCivDescription = new CvWString[MAX_PLAYERS];
	m_aszCivShortDesc = new CvWString[MAX_PLAYERS];
	m_aszCivAdjective = new CvWString[MAX_PLAYERS];
	m_aszCivPassword = new CvWString[MAX_PLAYERS];
	m_aszEmail = new CvString[MAX_PLAYERS];
	m_aszSmtpHost = new CvString[MAX_PLAYERS];

	m_abWhiteFlag = new bool[MAX_PLAYERS];
	m_aszFlagDecal = new CvWString[MAX_PLAYERS];

	m_aeCiv = new CivilizationTypes[MAX_PLAYERS];
	m_aeLeader = new LeaderHeadTypes[MAX_PLAYERS];
	m_aeTeam = new TeamTypes[MAX_PLAYERS];
	m_aeHandicap = new HandicapTypes[MAX_PLAYERS];
	m_aeColor = new PlayerColorTypes[MAX_PLAYERS];
	m_aeArtStyle = new ArtStyleTypes[MAX_PLAYERS];

	// Slot data
	m_aeSlotStatus = new SlotStatus[MAX_PLAYERS];
	m_aeSlotClaim = new SlotClaim[MAX_PLAYERS];

	// Civ flags
	m_abPlayableCiv = new bool[MAX_PLAYERS];
	m_abMinorNationCiv = new bool[MAX_PLAYERS];

	// Unsaved player data
	m_aiNetID = new int[MAX_PLAYERS];
	m_abReady = new bool[MAX_PLAYERS];

	m_aszPythonCheck = new CvString[MAX_PLAYERS];
	m_aszXMLCheck = new CvString[MAX_PLAYERS];

	m_aeCustomMapOptions = NULL;
	m_abVictories = NULL;

	reset(NO_GAMEMODE);
}


CvInitCore::~CvInitCore()
{
	uninit();

	SAFE_DELETE_ARRAY(m_abOptions);
	SAFE_DELETE_ARRAY(m_abMPOptions);
	SAFE_DELETE_ARRAY(m_abForceControls);
	SAFE_DELETE_ARRAY(m_aszLeaderName);
	SAFE_DELETE_ARRAY(m_aszCivDescription);
	SAFE_DELETE_ARRAY(m_aszCivShortDesc);
	SAFE_DELETE_ARRAY(m_aszCivAdjective);
	SAFE_DELETE_ARRAY(m_aszCivPassword);
	SAFE_DELETE_ARRAY(m_aszEmail);
	SAFE_DELETE_ARRAY(m_aszSmtpHost);
	SAFE_DELETE_ARRAY(m_abWhiteFlag);
	SAFE_DELETE_ARRAY(m_aszFlagDecal);
	SAFE_DELETE_ARRAY(m_aeCiv);
	SAFE_DELETE_ARRAY(m_aeLeader);
	SAFE_DELETE_ARRAY(m_aeTeam);
	SAFE_DELETE_ARRAY(m_aeHandicap);
	SAFE_DELETE_ARRAY(m_aeColor);
	SAFE_DELETE_ARRAY(m_aeArtStyle);
	SAFE_DELETE_ARRAY(m_aeSlotStatus);
	SAFE_DELETE_ARRAY(m_aeSlotClaim);
	SAFE_DELETE_ARRAY(m_abPlayableCiv);
	SAFE_DELETE_ARRAY(m_abMinorNationCiv);
	SAFE_DELETE_ARRAY(m_aiNetID);
	SAFE_DELETE_ARRAY(m_abReady);
	SAFE_DELETE_ARRAY(m_aszPythonCheck);
	SAFE_DELETE_ARRAY(m_aszXMLCheck);
}

void CvInitCore::init(GameMode eMode)
{
	//--------------------------------
	// Init saved data
	reset(eMode);
}

void CvInitCore::uninit()
{
	// <advc.003w> So that memory doesn't remain allocated after exit to main menu
	if (GC.IsGraphicsInitialized())
	{
		for (int i = 0; i < MAX_CIV_PLAYERS; i++)
			setCiv((PlayerTypes)i, NO_CIVILIZATION);
	} // </advc.003w>
	clearCustomMapOptions();
	clearVictories();
}

// FUNCTION: reset()
// Initializes data members that are serialized.
void CvInitCore::reset(GameMode eMode)
{
	//--------------------------------
	// Uninit class
	uninit();

	resetGame();
	resetPlayers();

	setMode(eMode);

	if (getMode() != NO_GAMEMODE)
	{
		setDefaults();
	}
}

void CvInitCore::setDefaults()
{
	for (int i = 0; i < NUM_GAMEOPTION_TYPES; ++i)
	{
		m_abOptions[i] = GC.getInfo((GameOptionTypes)i).getDefault();
	}
	for (int i = 0; i < NUM_MPOPTION_TYPES; ++i)
	{
		m_abMPOptions[i] = GC.getInfo((MultiplayerOptionTypes)i).getDefault();
	}
}


bool CvInitCore::getHuman(PlayerTypes eID) const
{
	if (getSlotStatus(eID) == SS_TAKEN)
		return true;
	else if (getSlotStatus(eID) == SS_OPEN)
		return (gDLL->isGameActive() || getHotseat() || getPitboss() || getPbem());
	return false;
}

int CvInitCore::getNumHumans() const
{
	int iNumHumans = 0;
	for (int i = 0; i < MAX_CIV_PLAYERS; ++i)
	{
		if (getHuman((PlayerTypes)i))
		{
			++iNumHumans;
		}
	}
	return iNumHumans;
}

int CvInitCore::getNumDefinedPlayers() const
{
	int iCount = 0;
	for (int i = 0; i < MAX_CIV_PLAYERS; ++i)
	{
		if ((getCiv((PlayerTypes)i) != NO_CIVILIZATION) && (getLeader((PlayerTypes)i) != NO_LEADER))
		{
			iCount++;
		}
	}
	return iCount;
}

bool CvInitCore::getMultiplayer() const
{
	switch (getType())
	{
	case GAME_MP_NEW:
	case GAME_MP_LOAD:
	case GAME_MP_SCENARIO:
		return true;
	default:
		return false;
	}
}

bool CvInitCore::getNewGame() const
{
	switch (getType())
	{
	case GAME_SP_NEW:
	case GAME_SP_SCENARIO:
	case GAME_MP_NEW:
	case GAME_MP_SCENARIO:
	case GAME_HOTSEAT_NEW:
	case GAME_HOTSEAT_SCENARIO:
	case GAME_PBEM_NEW:
	case GAME_PBEM_SCENARIO:
		return true;
	default:
		return false;
	}
}

bool CvInitCore::getSavedGame() const
{
	switch (getType())
	{
	case GAME_SP_LOAD:
	case GAME_MP_LOAD:
	case GAME_HOTSEAT_LOAD:
	case GAME_PBEM_LOAD:
		return true;
	default:
		return false;
	}
}

bool CvInitCore::getPitboss() const
{
	return (getMode() == GAMEMODE_PITBOSS);
}

bool CvInitCore::getHotseat() const
{
	return (getType() == GAME_HOTSEAT_NEW || getType() == GAME_HOTSEAT_SCENARIO || getType() == GAME_HOTSEAT_LOAD);
}


bool CvInitCore::getPbem() const
{
	return (getType() == GAME_PBEM_NEW || getType() == GAME_PBEM_SCENARIO || getType() == GAME_PBEM_LOAD);
}


bool CvInitCore::getSlotVacant(PlayerTypes eID) const
{
	FAssertBounds(0, MAX_CIV_PLAYERS, eID);

	bool bRetVal = false;

	if ( checkBounds(eID, 0, MAX_CIV_PLAYERS) )
	{
		bool bTakeoverAI = getMPOption(MPOPTION_TAKEOVER_AI);
		SlotStatus eStatus = getSlotStatus(eID);

		// Check the status of this slot
		if ( (eStatus == SS_OPEN) || (bTakeoverAI && (eStatus == SS_COMPUTER)) )
		{
			bRetVal = ( getSlotClaim(eID) != SLOTCLAIM_ASSIGNED );
		}
	}

	return bRetVal;
}

PlayerTypes CvInitCore::getAvailableSlot()
{
	// Get the next ID available ID
	// First check for open slots only
	for (int i = 0; i < MAX_CIV_PLAYERS; ++i)
	{
		PlayerTypes eID = (PlayerTypes)i;
		if ( (getSlotClaim(eID) == SLOTCLAIM_UNASSIGNED) && (getSlotStatus(eID) == SS_OPEN) )
		{
			setSlotClaim(eID, SLOTCLAIM_ASSIGNED);
			return eID;
		}
	}

	// That didn't work, check to see if we can assign computer slots
	if (getMPOption(MPOPTION_TAKEOVER_AI))
	{
		for (int i = 0; i < MAX_CIV_PLAYERS; ++i)
		{
			PlayerTypes eID = (PlayerTypes)i;
			if ( (getSlotClaim(eID) == SLOTCLAIM_UNASSIGNED) && (getSlotStatus(eID) == SS_COMPUTER) )
			{
				setSlotClaim(eID, SLOTCLAIM_ASSIGNED);
				return eID;
			}
		}
	}

	// None available at all...
	return NO_PLAYER;
}

void CvInitCore::reassignPlayer(PlayerTypes eOldID, PlayerTypes eNewID)
{
	if (!checkBounds(eOldID, 0, MAX_CIV_PLAYERS) || !checkBounds(eNewID, 0, MAX_CIV_PLAYERS))
	{
		FAssertBounds(0, MAX_CIV_PLAYERS, eOldID);
		FAssertBounds(0, MAX_CIV_PLAYERS, eNewID);
		return; // advc
	}
	// *** SAVE TARGET SLOT DETAILS TEMPORARILY
	// Temp civ details
	CvWString szLeaderName = m_aszLeaderName[eNewID];
	CvWString szCivDescription = m_aszCivDescription[eNewID];
	CvWString szCivShortDesc = m_aszCivShortDesc[eNewID];
	CvWString szCivAdjective = m_aszCivAdjective[eNewID];
	CvWString szCivPassword = m_aszCivPassword[eNewID];
	CvString szEmail = m_aszEmail[eNewID];
	CvString szSmtpHost = m_aszSmtpHost[eNewID];
	bool bWhiteFlag = m_abWhiteFlag[eNewID];
	CvWString szFlagDecal = m_aszFlagDecal[eNewID];
	CivilizationTypes eCiv = m_aeCiv[eNewID];
	LeaderHeadTypes eLeader = m_aeLeader[eNewID];
	TeamTypes eTeam = m_aeTeam[eNewID];
	HandicapTypes eHandicap = m_aeHandicap[eNewID];
	PlayerColorTypes eColor = m_aeColor[eNewID];
	ArtStyleTypes eArtStyle = m_aeArtStyle[eNewID];
	// Temp slot data
	SlotStatus eSlotStatus = m_aeSlotStatus[eNewID];
	SlotClaim eSlotClaim = m_aeSlotClaim[eNewID];
	// Temp civ flags
	bool bPlayableCiv = m_abPlayableCiv[eNewID];
	bool bMinorNationCiv = m_abMinorNationCiv[eNewID];
	// Temp unsaved player data
	int iNetID = m_aiNetID[eNewID];
	bool bReady = m_abReady[eNewID];
	CvString szPythonCheck = m_aszPythonCheck[eNewID];
	CvString szXMLCheck = m_aszXMLCheck[eNewID];

	// *** SAVE OLD SLOT DETAILS IN NEW SLOT
	// New civ details
	m_aszLeaderName[eNewID] = m_aszLeaderName[eOldID];
	m_aszCivDescription[eNewID] = m_aszCivDescription[eOldID];
	m_aszCivShortDesc[eNewID] = m_aszCivShortDesc[eOldID];
	m_aszCivAdjective[eNewID] = m_aszCivAdjective[eOldID];
	m_aszCivPassword[eNewID] = m_aszCivPassword[eOldID];
	m_aszEmail[eNewID] = m_aszEmail[eOldID];
	m_aszSmtpHost[eNewID] = m_aszSmtpHost[eOldID];
	m_abWhiteFlag[eNewID] = m_abWhiteFlag[eOldID];
	m_aszFlagDecal[eNewID] = m_aszFlagDecal[eOldID];
	//m_aeCiv[eNewID] = m_aeCiv[eOldID];
	setCiv(eNewID, m_aeCiv[eOldID]); // advc.003w
	m_aeLeader[eNewID] = m_aeLeader[eOldID];
	m_aeTeam[eNewID] = m_aeTeam[eOldID];
	m_aeHandicap[eNewID] = m_aeHandicap[eOldID];
	m_aeColor[eNewID] = m_aeColor[eOldID];
	m_aeArtStyle[eNewID] = m_aeArtStyle[eOldID];
	// New slot data
	m_aeSlotStatus[eNewID] = m_aeSlotStatus[eOldID];
	m_aeSlotClaim[eNewID] = m_aeSlotClaim[eOldID];
	// New civ flags
	m_abPlayableCiv[eNewID] = m_abPlayableCiv[eOldID];
	m_abMinorNationCiv[eNewID] = m_abMinorNationCiv[eOldID];
	// New unsaved player data
	m_aiNetID[eNewID] = m_aiNetID[eOldID];
	m_abReady[eNewID] = m_abReady[eOldID];
	m_aszPythonCheck[eNewID] = m_aszPythonCheck[eOldID];
	m_aszXMLCheck[eNewID] = m_aszXMLCheck[eOldID];

	// *** SAVE TEMP DETAILS IN OLD SLOT
	// New civ details
	m_aszLeaderName[eOldID] = szLeaderName;
	m_aszCivDescription[eOldID] = szCivDescription;
	m_aszCivShortDesc[eOldID] = szCivShortDesc;
	m_aszCivAdjective[eOldID] = szCivAdjective;
	m_aszCivPassword[eOldID] = szCivPassword;
	m_aszEmail[eOldID] = szEmail;
	m_aszSmtpHost[eOldID] = szSmtpHost;
	m_abWhiteFlag[eOldID] = bWhiteFlag;
	m_aszFlagDecal[eOldID] = szFlagDecal;
	//m_aeCiv[eOldID] = eCiv;
	setCiv(eOldID, eCiv); // advc.003w
	m_aeLeader[eOldID] = eLeader;
	m_aeTeam[eOldID] = eTeam;
	m_aeHandicap[eOldID] = eHandicap;
	m_aeColor[eOldID] = eColor;
	m_aeArtStyle[eOldID] = eArtStyle;
	// New slot data
	m_aeSlotStatus[eOldID] = eSlotStatus;
	m_aeSlotClaim[eOldID] = eSlotClaim;
	// New civ flags
	m_abPlayableCiv[eOldID] = bPlayableCiv;
	m_abMinorNationCiv[eOldID] = bMinorNationCiv;
	// New unsaved player data
	m_aiNetID[eOldID] = iNetID;
	m_abReady[eOldID] = bReady;
	m_aszPythonCheck[eOldID] = szPythonCheck;
	m_aszXMLCheck[eOldID] = szXMLCheck;

	// We may have a new active player id...
	if (getActivePlayer() == eOldID)
		setActivePlayer(eNewID);
	else if (getActivePlayer() == eNewID)
		setActivePlayer(eOldID);

	if(CvPlayer::areStaticsInitialized())
	{
		GET_PLAYER(eOldID).updateTeamType();
		GET_PLAYER(eNewID).updateTeamType();
		GET_PLAYER(eOldID).updateHuman();
		GET_PLAYER(eNewID).updateHuman();
	}
}

void CvInitCore::closeInactiveSlots()
{
	// Open inactive slots mean different things to different game modes and types...
	// Let's figure out what they mean for us

	for (int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		PlayerTypes eID = (PlayerTypes)i;
		if (getSlotStatus(eID) == SS_OPEN)
		{
			if (getPitboss() || getHotseat() || getPbem())
			{
				// Pitboss & hotseat - all "open" slots are non-present human players
				setSlotStatus(eID, SS_TAKEN);
			}
			else if (getType() == GAME_MP_SCENARIO)
			{
				// Multiplayer scenario - all "open" slots should be filled with an AI player
				setSlotStatus(eID, SS_COMPUTER);
			}
			else
			{
				// If it's a normal game, all "open" slots should be closed.
				setSlotStatus(eID, SS_CLOSED);
			}
			setSlotClaim(eID, SLOTCLAIM_UNASSIGNED);

			gDLL->sendPlayerInfo(eID);
		}
	}
}

void CvInitCore::reopenInactiveSlots()
{
	// "Inactive" open slots will only be in Pitboss and Hotseat
	if (getPitboss() || getHotseat() || getPbem())
	{
		for (int i = 0; i < MAX_CIV_PLAYERS; ++i)
		{
			PlayerTypes eID = (PlayerTypes)i;
			// Reopen all slots that don't have active connections
			if (getSlotStatus(eID) == SS_TAKEN)
			{
				if ( getSlotClaim(eID) != SLOTCLAIM_ASSIGNED )
				{
					setSlotStatus(eID, SS_OPEN);
				}
			}
		}
	}
}

void CvInitCore::resetGame()
{
	// Descriptive strings about game and map
	m_eType = GAME_NONE;
	m_szGameName.clear();
	m_szGamePassword.clear();
	m_szAdminPassword.clear();
	m_szMapScriptName.clear();
	m_bPangaea = false; // advc
	m_bWBMapNoPlayers = false;

	// Standard game parameters
	m_eWorldSize = NO_WORLDSIZE;											// STANDARD_ option?
	/*  <advc.003c> This function is called multiple times before XML is loaded.
		GC.getDefineINT returns 0 then, which is fine, but now also triggers
		a failed assertion. Therefore check if GC is done with the loading
		(and caching) of XML values.
		The in-line comments "NO_ option?" below are from the Vanilla developers.
		If I'd just set everything to NO_..., I'd have to set proper values at
		some later point though. */
	bool cd = GC.isCachingDone();
	m_eClimate = cd ? (ClimateTypes)GC.getDefineINT("STANDARD_CLIMATE") : NO_CLIMATE;			// NO_ option?
	m_eSeaLevel = cd ? (SeaLevelTypes)GC.getDefineINT("STANDARD_SEALEVEL") : NO_SEALEVEL;		// NO_ option?
	m_eEra = cd ? (EraTypes)GC.getDefineINT("STANDARD_ERA") : NO_ERA;						// NO_ option?
	m_eGameSpeed = cd ? (GameSpeedTypes)GC.getDefineINT("STANDARD_GAMESPEED") : NO_GAMESPEED;	// NO_ option?
	m_eTurnTimer = cd ? (TurnTimerTypes)GC.getDefineINT("STANDARD_TURNTIMER") : NO_TURNTIMER;	// NO_ option?
	m_eCalendar = cd ? (CalendarTypes)GC.getDefineINT("STANDARD_CALENDAR") : NO_CALENDAR;		// NO_ option?
	// </advc.003c>
	// Map-specific custom parameters
	clearCustomMapOptions();
	/*  advc: Unused as far as I can tell, but still better to ensure that it
		gets initialized. */
	m_iNumHiddenCustomMapOptions = 0;
	// Data-defined victory conditions
	refreshVictories();

	// Standard game options
	int i;
	for (i = 0; i < NUM_GAMEOPTION_TYPES; ++i)
	{
		m_abOptions[i] = false;
	}
	for (i = 0; i < NUM_MPOPTION_TYPES; ++i)
	{
		m_abMPOptions[i] = false;
	}
	m_bStatReporting = false;

	for (i = 0; i < NUM_FORCECONTROL_TYPES; ++i)
	{
		m_abForceControls[i] = false;
	}

	// Game turn mgmt
	m_iGameTurn = 0;
	m_iMaxTurns = 0;
	m_iPitbossTurnTime = 0;
	m_iTargetScore = 0;

	// City Elimination
	m_iMaxCityElimination = 0;

	m_iNumAdvancedStartPoints = 0;

	// Unsaved game data
	m_uiSyncRandSeed = 0;
	m_uiMapRandSeed = 0;
	m_eActivePlayer = NO_PLAYER;

	// Temp vars
	m_szTemp.clear();
}

void CvInitCore::resetGame(CvInitCore* pSource, bool bClear, bool bSaveGameType)
{
	FAssertMsg(pSource, "Passed null pointer to CvInitCore::resetGame");
	FAssertMsg(!bClear || !bSaveGameType, "Should not be clearing data while trying to preserve gametype info in CvInitCore::resetGame");

	if (bClear || !pSource)
		resetGame();

	if(!pSource)
		return; // advc

	// Only copy over saved data

	// Descriptive strings about game and map
	if (!bSaveGameType || getGameMultiplayer() != pSource->getGameMultiplayer())
	{
		setType(pSource->getType());
	}
	setGameName(pSource->getGameName());
	setGamePassword(pSource->getGamePassword());
	setAdminPassword(pSource->getAdminPassword(), false);
	setMapScriptName(pSource->getMapScriptName());

	setWBMapNoPlayers(pSource->getWBMapNoPlayers());

	// Standard game parameters
	setWorldSize(pSource->getWorldSize());
	setClimate(pSource->getClimate());
	setSeaLevel(pSource->getSeaLevel());
	setEra(pSource->getEra());
	setGameSpeed(pSource->getGameSpeed());
	setTurnTimer(pSource->getTurnTimer());
	setCalendar(pSource->getCalendar());

	// Map-specific custom parameters
	setCustomMapOptions(pSource->getNumCustomMapOptions(), pSource->getCustomMapOptions());
	m_iNumHiddenCustomMapOptions = pSource->getNumHiddenCustomMapOptions();
	setVictories(pSource->getNumVictories(), pSource->getVictories());

	// Standard game options
	FOR_EACH_ENUM(GameOption)
	{
		bool b = pSource->getOption(eLoopGameOption);
		// <dlph.18>
		CvGameOptionInfo const& kLoopGameOption = GC.getInfo(eLoopGameOption);
		if(kLoopGameOption.getVisible() == 0)
			b = kLoopGameOption.getDefault(); // </dlph.18>
		setOption(eLoopGameOption, b);
	}

	FOR_EACH_ENUM(MPOption)
	{
		setMPOption(eLoopMPOption, pSource->getMPOption(eLoopMPOption));
	}
	setStatReporting(pSource->getStatReporting());

	// Game turn mgmt
	setGameTurn(pSource->getGameTurn());
	setMaxTurns(pSource->getMaxTurns());
	setPitbossTurnTime(pSource->getPitbossTurnTime());
	setTargetScore(pSource->getTargetScore());

	// City Elimination
	setMaxCityElimination(pSource->getMaxCityElimination());

	setNumAdvancedStartPoints(pSource->getNumAdvancedStartPoints());

	setSyncRandSeed(pSource->getSyncRandSeed());
	setMapRandSeed(pSource->getMapRandSeed());
}

void CvInitCore::resetPlayers()
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		resetPlayer((PlayerTypes)i);
	}
}

void CvInitCore::resetPlayers(CvInitCore * pSource, bool bClear, bool bSaveSlotInfo)
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		resetPlayer((PlayerTypes)i, pSource, bClear, bSaveSlotInfo);
	}
}

void CvInitCore::resetPlayer(PlayerTypes eID)
{
	if (!checkBounds(eID, 0, MAX_PLAYERS))
	{
		FAssertBounds(0, MAX_PLAYERS, eID);
		return;
	}

	// Only copy over saved data

	// Civ details
	m_aszLeaderName[eID].clear();
	m_aszCivDescription[eID].clear();
	m_aszCivShortDesc[eID].clear();
	m_aszCivAdjective[eID].clear();
	m_aszCivPassword[eID].clear();
	m_aszEmail[eID].clear();
	m_aszSmtpHost[eID].clear();

	m_abWhiteFlag[eID] = false;
	m_aszFlagDecal[eID].clear();

	m_aeCiv[eID] = NO_CIVILIZATION;
	m_aeLeader[eID] = NO_LEADER;
	m_aeTeam[eID] = (TeamTypes)eID;
	// <advc.003c> See comment in resetGame
	m_aeHandicap[eID] = GC.isCachingDone() ?
			(HandicapTypes)GC.getDefineINT("STANDARD_HANDICAP") : NO_HANDICAP; // </advc.003c>
	m_aeColor[eID] = NO_PLAYERCOLOR;
	m_aeArtStyle[eID] = NO_ARTSTYLE;


	// Slot data
	m_aeSlotStatus[eID] = SS_CLOSED;
	m_aeSlotClaim[eID] = SLOTCLAIM_UNASSIGNED;

	// Civ flags
	m_abPlayableCiv[eID] = false;
	m_abMinorNationCiv[eID] = false;

	// Unsaved player data
	m_aiNetID[eID] = -1;
	m_abReady[eID] = false;
	m_aszPythonCheck[eID].clear();
	m_aszXMLCheck[eID].clear();

	if(CvPlayer::areStaticsInitialized())
	{
		GET_PLAYER(eID).updateTeamType();
		GET_PLAYER(eID).updateHuman();
	}
}

void CvInitCore::resetPlayer(PlayerTypes eID, CvInitCore * pSource, bool bClear, bool bSaveSlotInfo)
{
	FAssertMsg(pSource, "Passed null pointer to CvInitCore::resetPlayer");
	FAssertBounds(0, MAX_PLAYERS, eID);

	FAssertMsg(!bClear || !bSaveSlotInfo, "Should not be clearing data while trying to preserve slot info in CvInitCore::resetPlayer");

	if (checkBounds(eID, 0, MAX_PLAYERS))
	{
		if (bClear || !pSource)
			resetPlayer(eID);

		if (pSource != NULL)
		{
			// Civ details
			setCivDescription(eID, pSource->getCivDescription(eID));
			setCivShortDesc(eID, pSource->getCivShortDesc(eID));
			setCivAdjective(eID, pSource->getCivAdjective(eID));

			setCivPassword(eID, pSource->getCivPassword(eID), false);
			setEmail(eID, pSource->getEmail(eID));
			setSmtpHost(eID, pSource->getSmtpHost(eID));
			setFlagDecal(eID, pSource->getFlagDecal(eID));
			setWhiteFlag(eID, pSource->getWhiteFlag(eID));

			setHandicap(eID, pSource->getHandicap(eID));
			setCiv(eID, pSource->getCiv(eID));
			setTeam(eID, pSource->getTeam(eID));
			setLeader(eID, pSource->getLeader(eID));
			setColor(eID, pSource->getColor(eID));
			setArtStyle(eID, pSource->getArtStyle(eID));

			setPlayableCiv(eID, pSource->getPlayableCiv(eID));
			setMinorNationCiv(eID, pSource->getMinorNationCiv(eID));

			// Slot data
			if (!bSaveSlotInfo)
			{
				// We don't wanna reset the slot data if we are loading a game
				// from init - we want to keep the changes we made during init
				setLeaderName(eID, pSource->getLeaderName(eID));
				setSlotStatus(eID, pSource->getSlotStatus(eID));
				setSlotClaim(eID, pSource->getSlotClaim(eID));
				// <advc.001p> Reset players while loading from within a game to avoid crash
				if (pSource->getSavedGame() && GET_PLAYER(eID).isEverAlive())
					GET_PLAYER(eID).reset(eID); // </advc.001p>
			}
		}
	}
}


CvWString CvInitCore::getMapScriptName() const
{
	if (!getWBMapScript() && // advc: Order switched; not sure what getTransferredMap does.
		gDLL->getTransferredMap())
	{
		// If it's a transferred Python file, we have to hack in the transferred extension
		return (m_szMapScriptName + CvWString(MAP_TRANSFER_EXT));
	}
	return m_szMapScriptName;
}

void CvInitCore::setMapScriptName(const CvWString & szMapScriptName)
{
	m_szMapScriptName = szMapScriptName;
	refreshCustomMapOptions();
	updatePangaea();	
}

bool CvInitCore::getWBMapScript() const
{
	return (gDLL->isDescFileName( CvString(m_szMapScriptName).GetCString() ));
}

/*  <advc.030> This only works at the start of a game b/c all savegames have
	type GAME_..._LOAD. Use CvGame::isScenario if it's not the start of a game. */
bool CvInitCore::isScenario() const
{
	return m_eType == GAME_SP_SCENARIO || m_eType == GAME_MP_SCENARIO ||
			m_eType == GAME_HOTSEAT_SCENARIO || m_eType == GAME_PBEM_SCENARIO;
} // </advc.030>


void CvInitCore::setWorldSize(const CvWString & szWorldSize)
{
	for (int i = 0; i < GC.getNumWorldInfos(); i++)
	{
		if ( wcsicmp(szWorldSize.GetCString(), CvWString(GC.getInfo((WorldSizeTypes)i).getType()).GetCString()) == 0 )
		{
			setWorldSize((WorldSizeTypes)i);
		}
	}
}

const CvWString & CvInitCore::getWorldSizeKey(CvWString & szBuffer) const
{
	if ( checkBounds(getWorldSize(), 0, GC.getNumWorldInfos()) )
	{
		szBuffer = GC.getInfo(getWorldSize()).getType();
		return szBuffer;
	}
	else
	{
		szBuffer = L"NO_WORLDSIZE";
		return szBuffer;
	}
}

void CvInitCore::setClimate(const CvWString & szClimate)
{
	for (int i = 0; i < GC.getNumClimateInfos(); i++)
	{
		if ( wcsicmp(szClimate.GetCString(), CvWString(GC.getInfo((ClimateTypes)i).getType()).GetCString()) == 0 )
		{
			setClimate((ClimateTypes)i);
		}
	}
}

const CvWString & CvInitCore::getClimateKey(CvWString & szBuffer) const
{
	if ( checkBounds(getClimate(), 0, GC.getNumClimateInfos()) )
	{
		szBuffer = GC.getInfo(getClimate()).getType();
		return szBuffer;
	}
	else
	{
		szBuffer = L"NO_CLIMATE";
		return szBuffer;
	}
}

void CvInitCore::setSeaLevel(const CvWString & szSeaLevel)
{
	for (int i = 0; i < GC.getNumSeaLevelInfos(); i++)
	{
		if ( wcsicmp(szSeaLevel.GetCString(), CvWString(GC.getInfo((SeaLevelTypes)i).getType()).GetCString()) == 0 )
		{
			setSeaLevel((SeaLevelTypes)i);
		}
	}
}

const CvWString & CvInitCore::getSeaLevelKey(CvWString & szBuffer) const
{
	if ( checkBounds(getSeaLevel(), 0, GC.getNumSeaLevelInfos()) )
	{
		szBuffer = GC.getInfo(getSeaLevel()).getType();
		return szBuffer;
	}
	else
	{
		szBuffer = L"NO_SEALEVEL";
		return szBuffer;
	}
}

void CvInitCore::setEra(const CvWString & szEra)
{
	for (int i = 0; i < GC.getNumEraInfos(); i++)
	{
		if ( wcsicmp(szEra.GetCString(), CvWString(GC.getInfo((EraTypes)i).getType()).GetCString()) == 0 )
		{
			setEra((EraTypes)i);
		}
	}
}

const CvWString & CvInitCore::getEraKey(CvWString & szBuffer) const
{
	if ( checkBounds(getEra(), 0, GC.getNumEraInfos()) )
	{
		szBuffer = GC.getInfo(getEra()).getType();
		return szBuffer;
	}
	else
	{
		szBuffer = L"NO_ERA";
		return szBuffer;
	}
}

void CvInitCore::setGameSpeed(const CvWString & szGameSpeed)
{
	for (int i = 0; i < GC.getNumGameSpeedInfos(); i++)
	{
		if ( wcsicmp(szGameSpeed.GetCString(), CvWString(GC.getInfo((GameSpeedTypes)i).getType()).GetCString()) == 0 )
		{
			setGameSpeed((GameSpeedTypes)i);
		}
	}
}

const CvWString & CvInitCore::getGameSpeedKey(CvWString & szBuffer) const
{
	if ( checkBounds(getGameSpeed(), 0, GC.getNumGameSpeedInfos()) )
	{
		szBuffer = GC.getInfo(getGameSpeed()).getType();
		return szBuffer;
	}
	else
	{
		szBuffer = L"NO_GAMESPEED";
		return szBuffer;
	}
}

void CvInitCore::setTurnTimer(const CvWString & szTurnTimer)
{
	for (int i = 0; i < GC.getNumTurnTimerInfos(); i++)
	{
		if ( wcsicmp(szTurnTimer.GetCString(), CvWString(GC.getInfo((TurnTimerTypes)i).getType()).GetCString()) == 0 )
		{
			setTurnTimer((TurnTimerTypes)i);
		}
	}
}

const CvWString & CvInitCore::getTurnTimerKey(CvWString & szBuffer) const
{
	if ( checkBounds(getTurnTimer(), 0, GC.getNumTurnTimerInfos()) )
	{
		szBuffer = GC.getInfo(getTurnTimer()).getType();
		return szBuffer;
	}
	else
	{
		szBuffer = L"NO_TURNTIMER";
		return szBuffer;
	}
}

void CvInitCore::setCalendar(const CvWString & szCalendar)
{
	for (int i = 0; i < NUM_CALENDAR_TYPES; i++)
	{
		if ( wcsicmp(szCalendar.GetCString(), CvWString(GC.getInfo((CalendarTypes)i).getType()).GetCString()) == 0 )
		{
			setCalendar((CalendarTypes)i);
		}
	}
}

const CvWString & CvInitCore::getCalendarKey(CvWString & szBuffer) const
{
	if ( checkBounds(getCalendar(), 0, NUM_CALENDAR_TYPES) )
	{
		szBuffer = GC.getInfo(getCalendar()).getType();
		return szBuffer;
	}
	else
	{
		szBuffer = L"NO_CALENDAR";
		return szBuffer;
	}
}


void CvInitCore::clearCustomMapOptions()
{
	SAFE_DELETE_ARRAY(m_aeCustomMapOptions);
	m_iNumCustomMapOptions = 0;
}

void CvInitCore::refreshCustomMapOptions()  // advc.003y: refactored
{
	clearCustomMapOptions();

	if (getWBMapScript())
		return;

	CvString szMapScriptNameNarrow(getMapScriptName());
	char const* szMapScriptName = szMapScriptNameNarrow.GetCString();
	if (!gDLL->pythonMapExists(szMapScriptName))
	{	// advc: Map script doesn't have to be present when loading a game
		FAssertMsg(getType() == GAME_SP_LOAD, "Map script not found");
		return;
	}
	CvPythonCaller const& py = *GC.getPythonCaller();
	m_iNumHiddenCustomMapOptions = py.numCustomMapOptions(szMapScriptName, true);
	int iOptions = py.numCustomMapOptions(szMapScriptName, false);
	if (iOptions <= 0)
		return;

	CustomMapOptionTypes* aeMapOptions = new CustomMapOptionTypes[iOptions];
	for (int i = 0; i < iOptions; i++)
		aeMapOptions[i] = py.customMapOptionDefault(szMapScriptName, i);
	setCustomMapOptions(iOptions, aeMapOptions);
	SAFE_DELETE_ARRAY(aeMapOptions);
}

// advc:
void CvInitCore::updatePangaea()
{
	// That's the name of the .py file; not language-dependent.
	m_bPangaea = (getMapScriptName().compare(L"Pangaea") == 0);
}

void CvInitCore::clearVictories()
{
	SAFE_DELETE_ARRAY(m_abVictories);
	m_iNumVictories = 0;
}

void CvInitCore::refreshVictories()
{
	clearVictories();
	m_iNumVictories = GC.getNumVictoryInfos();
	if (m_iNumVictories > 0)
	{
		m_abVictories = new bool[m_iNumVictories];
		for (int i = 0; i < m_iNumVictories; ++i)
		{
			m_abVictories[i] = true;
		}
	}
}

void CvInitCore::setCustomMapOptions(int iNumCustomMapOptions, const CustomMapOptionTypes * aeCustomMapOptions)
{
	clearCustomMapOptions();
	if (iNumCustomMapOptions)
	{
		FAssertMsg(aeCustomMapOptions, "CustomMap Num/Pointer mismatch in CvInitCore::setCustomMapOptions");
		m_iNumCustomMapOptions = iNumCustomMapOptions;
		m_aeCustomMapOptions = new CustomMapOptionTypes[m_iNumCustomMapOptions];
		for (int i = 0; i < m_iNumCustomMapOptions; ++i)
		{
			m_aeCustomMapOptions[i] = aeCustomMapOptions[i];
		}
	}
}

CustomMapOptionTypes CvInitCore::getCustomMapOption(int iOptionID) const
{
	FAssertBounds(0, m_iNumCustomMapOptions, iOptionID);
	if (checkBounds(iOptionID, 0, m_iNumCustomMapOptions))
		return m_aeCustomMapOptions[iOptionID];
	return NO_CUSTOM_MAPOPTION;
}

void CvInitCore::setCustomMapOption(int iOptionID, CustomMapOptionTypes eCustomMapOption)
{
	FAssertBounds(0, m_iNumCustomMapOptions, iOptionID);
	if (checkBounds(iOptionID, 0, m_iNumCustomMapOptions))
		m_aeCustomMapOptions[iOptionID] = eCustomMapOption;
}


void CvInitCore::setVictories(int iNumVictories, const bool * abVictories)
{
	clearVictories();
	if (iNumVictories)
	{
		FAssertMsg(abVictories, "Victory Num/Pointer mismatch in CvInitCore::setVictories");
		m_iNumVictories = iNumVictories;
		m_abVictories = new bool[m_iNumVictories];
		for (int i = 0; i < m_iNumVictories; ++i)
		{
			m_abVictories[i] = abVictories[i];
		}
	}
}

bool CvInitCore::getVictory(VictoryTypes eVictoryID) const
{
	FAssertBounds(0, m_iNumVictories, eVictoryID);
	if (checkBounds(eVictoryID, 0, m_iNumVictories))
		return m_abVictories[eVictoryID];
	return false;
}

void CvInitCore::setVictory(VictoryTypes eVictoryID, bool bVictory)
{
	FAssertBounds(0, m_iNumVictories, eVictoryID);
	if (checkBounds(eVictoryID, 0, m_iNumVictories))
		m_abVictories[eVictoryID] = bVictory;
}


bool CvInitCore::getOption(GameOptionTypes eIndex) const
{
	FAssertBounds(0, NUM_GAMEOPTION_TYPES, eIndex);
	if (checkBounds(eIndex, 0, NUM_GAMEOPTION_TYPES))
		return m_abOptions[eIndex];
	return false;
}

void CvInitCore::setOption(GameOptionTypes eIndex, bool bOption)
{
	FAssertBounds(0, NUM_GAMEOPTION_TYPES, eIndex);
	if (checkBounds(eIndex, 0, NUM_GAMEOPTION_TYPES))
		m_abOptions[eIndex] = bOption;
}

bool CvInitCore::getMPOption(MultiplayerOptionTypes eIndex) const
{
	FAssertBounds(0, NUM_MPOPTION_TYPES, eIndex);
	if (checkBounds(eIndex, 0, NUM_MPOPTION_TYPES))
		return m_abMPOptions[eIndex];
	return false;
}

void CvInitCore::setMPOption(MultiplayerOptionTypes eIndex, bool bOption)
{
	FAssertBounds(0, NUM_MPOPTION_TYPES, eIndex);
	if (checkBounds(eIndex, 0, NUM_MPOPTION_TYPES))
		m_abMPOptions[eIndex] = bOption;
}

bool CvInitCore::getForceControl(ForceControlTypes eIndex) const
{
	FAssertBounds(0, NUM_FORCECONTROL_TYPES, eIndex);
	if (checkBounds(eIndex, 0, NUM_FORCECONTROL_TYPES))
		return m_abForceControls[eIndex];
	return false;
}

void CvInitCore::setForceControl(ForceControlTypes eIndex, bool bOption)
{
	FAssertBounds(0, NUM_FORCECONTROL_TYPES, eIndex);
	if ( checkBounds(eIndex, 0, NUM_FORCECONTROL_TYPES) )
	{
		m_abForceControls[eIndex] = bOption;
	}
}


void CvInitCore::setActivePlayer(PlayerTypes eActivePlayer)
{
	m_eActivePlayer = eActivePlayer;

	if (m_eActivePlayer != NO_PLAYER)
	{
		// Automatically claim this slot
		setSlotClaim(m_eActivePlayer, SLOTCLAIM_ASSIGNED);
	}
}

void CvInitCore::setType(GameType eType)
{
	if (getType() != eType)
	{
		m_eType = eType;
		// <advc.054>
		// Permanent war/peace always visible in scenarios
		CvGameOptionInfo& kPermWarPeace = GC.getInfo(GAMEOPTION_NO_CHANGING_WAR_PEACE);
		if(eType == GAME_SP_SCENARIO || eType == GAME_MP_SCENARIO ||
			eType == GAME_HOTSEAT_SCENARIO || eType == GAME_PBEM_SCENARIO)
		{
			kPermWarPeace.setVisible(true);
		}
		// Otherwise as set in XML
		else kPermWarPeace.setVisible(kPermWarPeace.getVisibleXML());
		// Never visible in MP
		std::vector<GameOptionTypes> aeHideMP;
		aeHideMP.push_back(GAMEOPTION_LOCK_MODS);
		aeHideMP.push_back(GAMEOPTION_NEW_RANDOM_SEED);
		aeHideMP.push_back(GAMEOPTION_RISE_FALL); // advc.701
		for(size_t i = 0; i < aeHideMP.size(); i++)
		{
			CvGameOptionInfo& kOption = GC.getInfo(aeHideMP[i]);
			if(eType == GAME_MP_SCENARIO || eType == GAME_MP_NEW || eType == GAME_MP_LOAD ||
				eType == GAME_HOTSEAT_SCENARIO || eType == GAME_HOTSEAT_NEW ||
				eType == GAME_PBEM_LOAD || eType == GAME_PBEM_NEW ||
				eType == GAME_PBEM_SCENARIO)
			{
				kOption.setVisible(false);
			}
			// Otherwise as set in XML
			else kOption.setVisible(kOption.getVisibleXML());
		} // </advc.054>
		if(CvPlayer::areStaticsInitialized())
		{
			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				GET_PLAYER((PlayerTypes)i).updateHuman();
			}
		}
	}
}

void CvInitCore::setType(const CvWString & szType)
{
	if (wcsicmp(szType.GetCString(), L"singleplayer") == 0)
	{
		setType(GAME_SP_NEW);
	}
	else if (wcsicmp(szType.GetCString(), L"spload") == 0)
	{
		setType(GAME_SP_LOAD);
	}
	//FAssertMsg(false, "Invalid game type in ini file!");
	setType(GAME_NONE);
}

void CvInitCore::setMode(GameMode eMode)
{
	if (getMode() != eMode)
	{
		m_eMode = eMode;
		if(CvPlayer::areStaticsInitialized())
		{
			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				GET_PLAYER((PlayerTypes)i).updateHuman();
			}
		}
	}
}


const CvWString & CvInitCore::getLeaderName(PlayerTypes eID, uint uiForm) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		m_szTemp = gDLL->getObjectText(CvString(m_aszLeaderName[eID]).GetCString(), uiForm, true);
	else
	{
		FAssertBounds(0, MAX_PLAYERS, eID);
		m_szTemp.clear();
	}
	return m_szTemp;
}

void CvInitCore::setLeaderName(PlayerTypes eID, const CvWString & szLeaderName)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
	{
		CvWString szName = szLeaderName;
		gDLL->stripSpecialCharacters(szName);
		m_aszLeaderName[eID] = szName;
	}
	else FAssertBounds(0, MAX_PLAYERS, eID);
	gDLL->UI().setDirty(Score_DIRTY_BIT, true); // advc.001m
}

const CvWString & CvInitCore::getLeaderNameKey(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aszLeaderName[eID];

	FAssertBounds(0, MAX_PLAYERS, eID);
	m_szTemp.clear();
	return m_szTemp;
}

const CvWString & CvInitCore::getCivDescription(PlayerTypes eID, uint uiForm) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		m_szTemp = gDLL->getObjectText(CvString(m_aszCivDescription[eID]).GetCString(), uiForm, true);
	else
	{
		FAssertBounds(0, MAX_PLAYERS, eID);
		m_szTemp.clear();
	}
	return m_szTemp;
}

void CvInitCore::setCivDescription(PlayerTypes eID, const CvWString & szCivDescription)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
	{
		CvWString szName = szCivDescription;
		gDLL->stripSpecialCharacters(szName);
		m_aszCivDescription[eID] = szName;
	}
	else FAssertBounds(0, MAX_PLAYERS, eID);
}

const CvWString & CvInitCore::getCivDescriptionKey(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aszCivDescription[eID];

	FAssertBounds(0, MAX_PLAYERS, eID);
	m_szTemp.clear();
	return m_szTemp;
}

const CvWString & CvInitCore::getCivShortDesc(PlayerTypes eID, uint uiForm) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
	{
		// Assume we have stored the key
		m_szTemp = gDLL->getObjectText(CvString(m_aszCivShortDesc[eID]).GetCString(), uiForm, true);
	}
	else
	{
		FAssertBounds(0, MAX_PLAYERS, eID);
		m_szTemp.clear();
	}
	return m_szTemp;
}

void CvInitCore::setCivShortDesc(PlayerTypes eID, const CvWString & szCivShortDesc)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
	{
		CvWString szName = szCivShortDesc;
		gDLL->stripSpecialCharacters(szName);
		m_aszCivShortDesc[eID] = szName;
	}
	else FAssertBounds(0, MAX_PLAYERS, eID);
}

const CvWString & CvInitCore::getCivShortDescKey(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aszCivShortDesc[eID];

	FAssertBounds(0, MAX_PLAYERS, eID);
	m_szTemp.clear();
	return m_szTemp;
}

const CvWString & CvInitCore::getCivAdjective(PlayerTypes eID, uint uiForm) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
	{
		// Assume we have stored the key
		m_szTemp = gDLL->getObjectText(CvString(m_aszCivAdjective[eID]).GetCString(), uiForm, true);
	}
	else
	{
		FAssertBounds(0, MAX_PLAYERS, eID);
		m_szTemp.clear();
	}
	return m_szTemp;
}

void CvInitCore::setCivAdjective(PlayerTypes eID, const CvWString & szCivAdjective)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
	{
		CvWString szName = szCivAdjective;
		gDLL->stripSpecialCharacters(szName);
		m_aszCivAdjective[eID] = szName;
	}
	else FAssertBounds(0, MAX_PLAYERS, eID);
}

const CvWString & CvInitCore::getCivAdjectiveKey(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aszCivAdjective[eID];

	FAssertBounds(0, MAX_PLAYERS, eID);
	m_szTemp.clear();
	return m_szTemp;
}

const CvWString & CvInitCore::getCivPassword(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aszCivPassword[eID];

	FAssertBounds(0, MAX_PLAYERS, eID);
	m_szTemp.clear();
	return m_szTemp;
}

void CvInitCore::setCivPassword(PlayerTypes eID, const CvWString & szCivPassword, bool bEncrypt)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
	{
		if (szCivPassword.empty() || !bEncrypt)
			m_aszCivPassword[eID] = szCivPassword;
		else
		{
			m_aszCivPassword[eID] = CvWString(gDLL->md5String((char*)
					CvString(szCivPassword).GetCString()));
		}
	}
	else FAssertBounds(0, MAX_PLAYERS, eID);
}


const CvString & CvInitCore::getEmail(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aszEmail[eID];

	FAssertBounds(0, MAX_PLAYERS, eID);
	m_szTempA.clear();
	return m_szTempA;
}

void CvInitCore::setEmail(PlayerTypes eID, const CvString & szEmail)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		m_aszEmail[eID] = szEmail;
	else FAssertBounds(0, MAX_PLAYERS, eID);
}

const CvString & CvInitCore::getSmtpHost(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aszSmtpHost[eID];

	FAssertBounds(0, MAX_PLAYERS, eID);
	m_szTempA.clear();
	return m_szTempA;
}

void CvInitCore::setSmtpHost(PlayerTypes eID, const CvString & szHost)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		m_aszSmtpHost[eID] = szHost;
	else FAssertBounds(0, MAX_PLAYERS, eID);
}


bool CvInitCore::getWhiteFlag(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_abWhiteFlag[eID];

	FAssertBounds(0, MAX_PLAYERS, eID);
	return false;
}

void CvInitCore::setWhiteFlag(PlayerTypes eID, bool bWhiteFlag)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		m_abWhiteFlag[eID] = bWhiteFlag;
	else FAssertBounds(0, MAX_PLAYERS, eID);
}

const CvWString & CvInitCore::getFlagDecal(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aszFlagDecal[eID];

	FAssertBounds(0, MAX_PLAYERS, eID);
	m_szTemp.clear();
	return m_szTemp;
}

void CvInitCore::setFlagDecal(PlayerTypes eID, const CvWString & szFlagDecal)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		m_aszFlagDecal[eID] = szFlagDecal;
	else FAssertBounds(0, MAX_PLAYERS, eID);
}


CivilizationTypes CvInitCore::getCiv(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aeCiv[eID];
	FAssertBounds(0, MAX_PLAYERS, eID);
	return NO_CIVILIZATION;
}

void CvInitCore::setCiv(PlayerTypes eID, CivilizationTypes eCiv)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
	{	/*  advc.003w: Guard added
			(This way, the CvCivilization data is not even recomputed when
			reloading from within a game.) */
		if (m_aeCiv[eID] != eCiv)
		{
			m_aeCiv[eID] = eCiv;
			GET_PLAYER(eID).setCivilization(eCiv); // advc.003w
		}
	}
	else FAssertBounds(0, MAX_PLAYERS, eID);
}

LeaderHeadTypes CvInitCore::getLeader(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aeLeader[eID];

	FAssertBounds(0, MAX_PLAYERS, eID);
	return NO_LEADER;
}

void CvInitCore::setLeader(PlayerTypes eID, LeaderHeadTypes eLeader)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		m_aeLeader[eID] = eLeader;
	else FAssertBounds(0, MAX_PLAYERS, eID);
}

TeamTypes CvInitCore::getTeam(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aeTeam[eID];

	FAssertBounds(0, MAX_PLAYERS, eID);
	return NO_TEAM;
}

void CvInitCore::setTeam(PlayerTypes eID, TeamTypes eTeam)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
	{
		if (getTeam(eID) != eTeam)
		{
			m_aeTeam[eID] = eTeam;
			if(CvPlayer::areStaticsInitialized())
				GET_PLAYER(eID).updateTeamType();
		}
	}
	else FAssertBounds(0, MAX_PLAYERS, eID);
}

HandicapTypes CvInitCore::getHandicap(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aeHandicap[eID];

	FAssertBounds(0, MAX_PLAYERS, eID);
	return NO_HANDICAP;
}

void CvInitCore::setHandicap(PlayerTypes eID, HandicapTypes eHandicap)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		m_aeHandicap[eID] = eHandicap;
	else FAssertBounds(0, MAX_PLAYERS, eID);
}

PlayerColorTypes CvInitCore::getColor(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aeColor[eID];

	FAssertBounds(0, MAX_PLAYERS, eID);
	return NO_PLAYERCOLOR;
}

void CvInitCore::setColor(PlayerTypes eID, PlayerColorTypes eColor)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		m_aeColor[eID] = eColor;
	else FAssertBounds(0, MAX_PLAYERS, eID);
}

ArtStyleTypes CvInitCore::getArtStyle(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aeArtStyle[eID];

	FAssertBounds(0, MAX_PLAYERS, eID);
	return NO_ARTSTYLE;
}

void CvInitCore::setArtStyle(PlayerTypes eID, ArtStyleTypes eArtStyle)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		m_aeArtStyle[eID] = eArtStyle;
	else FAssertBounds(0, MAX_PLAYERS, eID);
}


SlotStatus CvInitCore::getSlotStatus(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aeSlotStatus[eID];

	FAssertBounds(0, MAX_PLAYERS, eID);
	return SS_CLOSED;
}

void CvInitCore::setSlotStatus(PlayerTypes eID, SlotStatus eSlotStatus)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
	{
		if (getSlotStatus(eID) != eSlotStatus)
		{
			m_aeSlotStatus[eID] = eSlotStatus;
			if(CvPlayer::areStaticsInitialized())
				GET_PLAYER(eID).updateHuman();
		}
	}
	else FAssertBounds(0, MAX_PLAYERS, eID);
}

SlotClaim CvInitCore::getSlotClaim(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aeSlotClaim[eID];

	FAssertBounds(0, MAX_PLAYERS, eID);
	return SLOTCLAIM_UNASSIGNED;
}

void CvInitCore::setSlotClaim(PlayerTypes eID, SlotClaim eSlotClaim)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		m_aeSlotClaim[eID] = eSlotClaim;
	else FAssertBounds(0, MAX_PLAYERS, eID);
}

bool CvInitCore::getReady(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_abReady[eID];

	FAssertBounds(0, MAX_PLAYERS, eID);
	return false;
}

void CvInitCore::setReady(PlayerTypes eID, bool bReady)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		m_abReady[eID] = bReady;
	else FAssertBounds(0, MAX_PLAYERS, eID);
}


bool CvInitCore::getPlayableCiv(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
	{
		if (getWBMapScript() && !getWBMapNoPlayers())
			return m_abPlayableCiv[eID];
		else
		{
			if (getCiv(eID) != NO_CIVILIZATION)
				return GC.getInfo(getCiv(eID)).isPlayable();
			else
			{
				// Don't allow people to play the barb civ
				return (eID < BARBARIAN_PLAYER);
			}
		}
	}
	else
	{
		FAssertBounds(0, MAX_PLAYERS, eID);
		return false;
	}
}

void CvInitCore::setPlayableCiv(PlayerTypes eID, bool bPlayableCiv)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		m_abPlayableCiv[eID] = bPlayableCiv;
	else FAssertBounds(0, MAX_PLAYERS, eID);
}

bool CvInitCore::getMinorNationCiv(PlayerTypes eID) const
{
	
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_abMinorNationCiv[eID];
	else
	{
		FAssertBounds(0, MAX_PLAYERS, eID);
		return false;
	}
}

void CvInitCore::setMinorNationCiv(PlayerTypes eID, bool bMinorNationCiv)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
	{
		/*  advc.003m: Not just a matter of calling CvTeam::updateMinorCiv -
			wars would have to be declared etc. */
		FAssertMsg(bMinorNationCiv == m_abMinorNationCiv[eID] || !GET_PLAYER(eID).isAlive(), "Minor civ status has changed after game start; this isn't supported.");
		m_abMinorNationCiv[eID] = bMinorNationCiv;
	}
	else FAssertBounds(0, MAX_PLAYERS, eID);
}

int CvInitCore::getNetID(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aiNetID[eID];
	else
	{
		FAssertBounds(0, MAX_PLAYERS, eID);
		return -1;
	}
}

void CvInitCore::setNetID(PlayerTypes eID, int iNetID)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		m_aiNetID[eID] = iNetID;
	else FAssertBounds(0, MAX_PLAYERS, eID);
}

const CvString & CvInitCore::getPythonCheck(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aszPythonCheck[eID];
	else
	{
		FAssertBounds(0, MAX_PLAYERS, eID);
		m_szTempCheck.clear();
		return m_szTempCheck;
	}
}

void CvInitCore::setPythonCheck(PlayerTypes eID, const CvString & szPythonCheck)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		m_aszPythonCheck[eID] = szPythonCheck;
	else FAssertBounds(0, MAX_PLAYERS, eID);
}

const CvString & CvInitCore::getXMLCheck(PlayerTypes eID) const
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		return m_aszXMLCheck[eID];
	else
	{
		FAssertBounds(0, MAX_PLAYERS, eID);
		m_szTempCheck.clear();
		return m_szTempCheck;
	}
}

void CvInitCore::setXMLCheck(PlayerTypes eID, const CvString & szXMLCheck)
{
	if (checkBounds(eID, 0, MAX_PLAYERS))
		m_aszXMLCheck[eID] = szXMLCheck;
	else FAssertBounds(0, MAX_PLAYERS, eID);
}

void CvInitCore::setAdminPassword(const CvWString & szAdminPassword, bool bEncrypt)
{
	if (szAdminPassword.empty() || !bEncrypt)
	{
		m_szAdminPassword = szAdminPassword;
	}
	else
	{
		m_szAdminPassword = CvWString(gDLL->md5String((char*)CvString(szAdminPassword).GetCString()));
	}
}

void CvInitCore::resetAdvancedStartPoints()
{
	int iPoints = 0;

	if (NO_ERA != getEra())
	{
		iPoints += GC.getInfo(getEra()).getAdvancedStartPoints();
	}

	if (NO_WORLDSIZE != getWorldSize())
	{
		iPoints *= GC.getInfo(getWorldSize()).getAdvancedStartPointsMod();
		iPoints /= 100;
	}
	// <advc.250c> Reduce start-point costs based on game speed instead.
	/*if (NO_GAMESPEED != getGameSpeed()) {
		iPoints *= GC.getInfo(getGameSpeed()).getGrowthPercent();
		iPoints /= 100;
	}*/
	// (Effect of world size removed through WorldInfo XML)
	// </advc.250c>
	setNumAdvancedStartPoints(iPoints);
}

// <advc.250c>
int CvInitCore::getAdvancedStartMinPoints() const
{
	FOR_EACH_ENUM(UnitClass)
	{
		CvUnitInfo const& u = GC.getInfo(GC.getInfo(eLoopUnitClass).getDefaultUnit());
		if(u.isFound())
			return u.getAdvancedStartCost();
	}
	FAssert(false);
	return -1;
} // </advc.250c>


void CvInitCore::read(FDataStreamBase* pStream)
{
	uint uiFlag=0;
	pStream->Read(&uiFlag);

	// GAME DATA
	pStream->Read((int*)&m_eType);
	pStream->ReadString(m_szGameName);
	pStream->ReadString(m_szGamePassword);
	pStream->ReadString(m_szAdminPassword);
	pStream->ReadString(m_szMapScriptName);
	// <advc>
	if (uiFlag >= 3)
		pStream->Read(&m_bPangaea);
	else updatePangaea(); // </advc>
	pStream->Read(&m_bWBMapNoPlayers);

	pStream->Read((int*)&m_eWorldSize);
	pStream->Read((int*)&m_eClimate);
	pStream->Read((int*)&m_eSeaLevel);
	pStream->Read((int*)&m_eEra);
	pStream->Read((int*)&m_eGameSpeed);
	pStream->Read((int*)&m_eTurnTimer);
	pStream->Read((int*)&m_eCalendar);

	SAFE_DELETE_ARRAY(m_aeCustomMapOptions);
	pStream->Read(&m_iNumCustomMapOptions);
	pStream->Read(&m_iNumHiddenCustomMapOptions);
	if (m_iNumCustomMapOptions > 0)
	{
		m_aeCustomMapOptions = new CustomMapOptionTypes[m_iNumCustomMapOptions];
		pStream->Read(m_iNumCustomMapOptions, (int*)m_aeCustomMapOptions);
	}

	SAFE_DELETE_ARRAY(m_abVictories);
	pStream->Read(&m_iNumVictories);
	if (m_iNumVictories > 0)
	{
		m_abVictories = new bool[m_iNumVictories];
		pStream->Read(m_iNumVictories, m_abVictories);
	}
	// <advc.912d>
	if(uiFlag <= 0)
	{
		pStream->Read(NUM_GAMEOPTION_TYPES - 2, m_abOptions);
		m_abOptions[NUM_GAMEOPTION_TYPES - 2] = false;
		m_abOptions[NUM_GAMEOPTION_TYPES - 1] = false;
	}
	else if(uiFlag == 1)
	{
		pStream->Read(NUM_GAMEOPTION_TYPES - 1, m_abOptions);
		m_abOptions[NUM_GAMEOPTION_TYPES - 1] = false;
	}
	else pStream->Read(NUM_GAMEOPTION_TYPES, m_abOptions);
	// </advc.912d>

	pStream->Read(NUM_MPOPTION_TYPES, m_abMPOptions);

	pStream->Read(&m_bStatReporting);

	pStream->Read(&m_iGameTurn);
	pStream->Read(&m_iMaxTurns);
	pStream->Read(&m_iPitbossTurnTime);
	pStream->Read(&m_iTargetScore);

	pStream->Read(&m_iMaxCityElimination);
	pStream->Read(&m_iNumAdvancedStartPoints);

	// PLAYER DATA
	pStream->ReadString(MAX_PLAYERS, m_aszLeaderName);
	pStream->ReadString(MAX_PLAYERS, m_aszCivDescription);
	pStream->ReadString(MAX_PLAYERS, m_aszCivShortDesc);
	pStream->ReadString(MAX_PLAYERS, m_aszCivAdjective);
	pStream->ReadString(MAX_PLAYERS, m_aszCivPassword);
	pStream->ReadString(MAX_PLAYERS, m_aszEmail);
	pStream->ReadString(MAX_PLAYERS, m_aszSmtpHost);

	pStream->Read(MAX_PLAYERS, m_abWhiteFlag);
	pStream->ReadString(MAX_PLAYERS, m_aszFlagDecal);

	pStream->Read(MAX_PLAYERS, (int*)m_aeCiv);
	pStream->Read(MAX_PLAYERS, (int*)m_aeLeader);
	pStream->Read(MAX_PLAYERS, (int*)m_aeTeam);


	pStream->Read(MAX_PLAYERS, (int*)m_aeHandicap);
	pStream->Read(MAX_PLAYERS, (int*)m_aeColor);
	pStream->Read(MAX_PLAYERS, (int*)m_aeArtStyle);

	pStream->Read(MAX_PLAYERS, (int*)m_aeSlotStatus);
	pStream->Read(MAX_PLAYERS, (int*)m_aeSlotClaim);

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (m_aeSlotClaim[i] == SLOTCLAIM_ASSIGNED)
			m_aeSlotClaim[i] = SLOTCLAIM_RESERVED;
	}

	pStream->Read(MAX_PLAYERS, m_abPlayableCiv);
	pStream->Read(MAX_PLAYERS, m_abMinorNationCiv);

	if (CvPlayer::areStaticsInitialized())
	{
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)i); // advc
			/*  <advc.706> Had a reproducible crash when loading a non-R&F game
				from within an R&F game right after inspecting a city.
				Resetting the human players before reading any other player data
				seems to have fixed it. */
			if(kLoopPlayer.isHuman())
			{
				kLoopPlayer.reset((PlayerTypes)i);
				kLoopPlayer.setIsHuman(true);
			} // </advc.706>
			kLoopPlayer.updateHuman();
			kLoopPlayer.updateTeamType();
		}
	}
}


void CvInitCore::write(FDataStreamBase* pStream)
{
	REPRO_TEST_BEGIN_WRITE("InitCore");
	uint uiFlag = 1;
	uiFlag = 2; // advc.912d
	uiFlag = 3; // advc: m_bPangaea
	pStream->Write(uiFlag);

	// GAME DATA ...

	//pStream->Write(m_eType);
	/*	<advc.001p> Make sure that resetPlayer will be able to tell
		that a game is being loaded when reloading this savegame.
		After loading, the EXE calls setType(..._LOAD). When ..._LOAD is
		already the game type, some code in setType won't be executed -
		but I don't think that code needs to run at that point. */
	GameType eWriteGameType = m_eType;
	switch (eWriteGameType)
	{
	case GAME_SP_NEW:
	case GAME_SP_SCENARIO:
		eWriteGameType = GAME_SP_LOAD;
		break;
	case GAME_MP_NEW:
	case GAME_MP_SCENARIO:
		eWriteGameType = GAME_MP_LOAD;
		break;
	case GAME_HOTSEAT_NEW:
	case GAME_HOTSEAT_SCENARIO:
		eWriteGameType = GAME_HOTSEAT_LOAD;
		break;
	case GAME_PBEM_NEW:
	case GAME_PBEM_SCENARIO:
		eWriteGameType = GAME_PBEM_LOAD;
	}
	pStream->Write(eWriteGameType); // </advc.001p>
	pStream->WriteString(m_szGameName);
	pStream->WriteString(m_szGamePassword);
	pStream->WriteString(m_szAdminPassword);
	pStream->WriteString(m_szMapScriptName);
	pStream->Write(m_bPangaea); // advc
	pStream->Write(m_bWBMapNoPlayers);

	pStream->Write(m_eWorldSize);
	pStream->Write(m_eClimate);
	pStream->Write(m_eSeaLevel);
	pStream->Write(m_eEra);
	pStream->Write(m_eGameSpeed);
	pStream->Write(m_eTurnTimer);
	pStream->Write(m_eCalendar);

	pStream->Write(m_iNumCustomMapOptions);
	pStream->Write(m_iNumHiddenCustomMapOptions);
	pStream->Write(m_iNumCustomMapOptions, (int*)m_aeCustomMapOptions);

	pStream->Write(m_iNumVictories);
	pStream->Write(m_iNumVictories, m_abVictories);

	pStream->Write(NUM_GAMEOPTION_TYPES, m_abOptions);
	pStream->Write(NUM_MPOPTION_TYPES, m_abMPOptions);

	pStream->Write(m_bStatReporting);

	pStream->Write(m_iGameTurn);
	pStream->Write(m_iMaxTurns);
	pStream->Write(m_iPitbossTurnTime);
	pStream->Write(m_iTargetScore);

	pStream->Write(m_iMaxCityElimination);
	pStream->Write(m_iNumAdvancedStartPoints);

	// PLAYER DATA ...

	pStream->WriteString(MAX_PLAYERS, m_aszLeaderName);
	pStream->WriteString(MAX_PLAYERS, m_aszCivDescription);
	pStream->WriteString(MAX_PLAYERS, m_aszCivShortDesc);
	pStream->WriteString(MAX_PLAYERS, m_aszCivAdjective);
	pStream->WriteString(MAX_PLAYERS, m_aszCivPassword);
	pStream->WriteString(MAX_PLAYERS, m_aszEmail);
	pStream->WriteString(MAX_PLAYERS, m_aszSmtpHost);

	pStream->Write(MAX_PLAYERS, m_abWhiteFlag);
	pStream->WriteString(MAX_PLAYERS, m_aszFlagDecal);

	pStream->Write(MAX_PLAYERS, (int*)m_aeCiv);
	pStream->Write(MAX_PLAYERS, (int*)m_aeLeader);
	pStream->Write(MAX_PLAYERS, (int*)m_aeTeam);
	pStream->Write(MAX_PLAYERS, (int*)m_aeHandicap);
	pStream->Write(MAX_PLAYERS, (int*)m_aeColor);
	pStream->Write(MAX_PLAYERS, (int*)m_aeArtStyle);
	REPRO_TEST_END_WRITE(); // (skip slot data)
	pStream->Write(MAX_PLAYERS, (int*)m_aeSlotStatus);
	pStream->Write(MAX_PLAYERS, (int*)m_aeSlotClaim);

	pStream->Write(MAX_PLAYERS, m_abPlayableCiv);
	pStream->Write(MAX_PLAYERS, m_abMinorNationCiv);
}
