#include "CvGameCoreDLL.h"
#include "CvReplayInfo.h"
#include "CvGameAI.h"
#include "CvPlayerAI.h"
#include "CvInfos.h"
#include "CvMap.h"
#include "CvReplayMessage.h"
#include "CvGameTextMgr.h"
#include "CvInitCore.h"
#include "StartPointsAsHandicap.h" // advc.250b
#include "CvDLLInterfaceIFaceBase.h"

int CvReplayInfo::REPLAY_VERSION = 6; // advc.707, advc.106i: 4 in BtS
bool CvReplayInfo::STORE_REPLAYS_AS_BTS = false; // advc.106i

CvReplayInfo::CvReplayInfo() :
	m_iActivePlayer(0),
	m_eDifficulty(NO_HANDICAP),
	m_eWorldSize(NO_WORLDSIZE),
	m_eClimate(NO_CLIMATE),
	m_eSeaLevel(NO_SEALEVEL),
	m_eEra(NO_ERA),
	m_eGameSpeed(NO_GAMESPEED),
	m_iInitialTurn(0),
	m_iFinalTurn(0),
	m_eVictoryType(NO_VICTORY),
	m_iMapHeight(0),
	m_iMapWidth(0),
	m_pcMinimapPixels(NULL),
	m_eCalendar(NO_CALENDAR), // advc.003: Safer to initialize it here
	m_iNormalizedScore(0),
	m_bMultiplayer(false),
	m_iStartYear(0)
{
	m_nMinimapSize = ((GC.getDefineINT("MINIMAP_RENDER_SIZE") * GC.getDefineINT("MINIMAP_RENDER_SIZE")) / 2);
	m = new Data(); // advc.003k
	m->iFinalScore = -1; // advc.707
	// <advc.106i>
	m->szPurportedModName = gDLL->getModName(); // Local copy; not sure if necessary.
	m->iVersionRead = -1;
	m->bDisplayOtherMods = false;
	// See comment in GlobalDefines_advc.xml
	STORE_REPLAYS_AS_BTS = (GC.getDefineINT("HOF_STORE_REPLAYS_AS_BTS") > 0 &&
			(GC.getNumPlayerColorInfos() <= 44 ||
			/*  If no colors are added beyond those in BtS, then the new
				player colors are apparently all old colors; no problem then. */
			GC.getNumColorInfos() <= 127) &&
			/*  This replay object may well not use any of the added world sizes etc.,
				but I want the same replay format for all replays written by the mod. */
			GC.getNumWorldInfos() <= 6 && GC.getNumVictoryInfos() >= 7 &&
			GC.getNumHandicapInfos() >= 9 && GC.getNumGameSpeedInfos() >= 4);
	// </advc.106i>
}

CvReplayInfo::~CvReplayInfo()
{
	for (uint i = 0; i < m_listReplayMessages.size(); i++)
	{
		SAFE_DELETE(m_listReplayMessages[i]);
	}
	SAFE_DELETE(m_pcMinimapPixels);
	SAFE_DELETE(m); // advc.003k
}

void CvReplayInfo::createInfo(PlayerTypes ePlayer)
{
	CvGame& game = GC.getGameINLINE();
	CvMap& map = GC.getMapINLINE();

	if (ePlayer == NO_PLAYER)
	{
		ePlayer = game.getActivePlayer();
	}
	if (NO_PLAYER != ePlayer)
	{
		CvPlayer& player = GET_PLAYER(ePlayer);

		m_eDifficulty = player.getHandicapType();
		m_szLeaderName = player.getName();
		m_szCivDescription = player.getCivilizationDescription();
		m_szShortCivDescription = player.getCivilizationShortDescription();
		m_szCivAdjective = player.getCivilizationAdjective();
		m_szMapScriptName = GC.getInitCore().getMapScriptName();
		m_eWorldSize = map.getWorldSize();
		m_eClimate = map.getClimate();
		m_eSeaLevel = map.getSeaLevel();
		m_eEra = game.getStartEra();
		m_eGameSpeed = game.getGameSpeedType();

		m_listGameOptions.clear();
		for (int i = 0; i < NUM_GAMEOPTION_TYPES; i++)
		{
			GameOptionTypes eOption = (GameOptionTypes)i;
			if (game.isOption(eOption))
			{
				m_listGameOptions.push_back(eOption);
			}
		}

		m_listVictoryTypes.clear();
		for (int i = 0; i < GC.getNumVictoryInfos(); i++)
		{
			VictoryTypes eVictory = (VictoryTypes)i;
			if (game.isVictoryValid(eVictory))
			{
				m_listVictoryTypes.push_back(eVictory);
			}
		}
		if (game.getWinner() == player.getTeam())
		{
			m_eVictoryType = game.getVictory();
		}
		else
		{
			m_eVictoryType = NO_VICTORY;
		}

		m_iNormalizedScore = player.calculateScore(true, player.getTeam() == GC.getGameINLINE().getWinner());
		// <advc.707> Treat R&F games as "Score" victory (previously unused)
		if(game.isOption(GAMEOPTION_RISE_FALL)) {
			for(int i = 0; i < GC.getNumVictoryInfos(); i++) {
				VictoryTypes eVictory = (VictoryTypes)i;
				if(GC.getVictoryInfo(eVictory).isTargetScore()) {
					m_eVictoryType = eVictory;
					break;
				}
			}
		} // </advc.707>
	}

	m_bMultiplayer = game.isGameMultiPlayer();


	m_iInitialTurn = GC.getGameINLINE().getStartTurn();
	m_iStartYear = GC.getGameINLINE().getStartYear();
	m_iFinalTurn = game.getGameTurn();
	GAMETEXT.setYearStr(m_szFinalDate, m_iFinalTurn, false, GC.getGameINLINE().getCalendar(), GC.getGameINLINE().getStartYear(), GC.getGameINLINE().getGameSpeedType());

	m_eCalendar = GC.getGameINLINE().getCalendar();


	std::map<PlayerTypes, int> mapPlayers;
	m_listPlayerScoreHistory.clear();
	int iPlayerIndex = 0;
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
	{
		CvPlayer& player = GET_PLAYER((PlayerTypes)iPlayer);
		if (player.isEverAlive())
		{
			mapPlayers[(PlayerTypes)iPlayer] = iPlayerIndex;
			if ((PlayerTypes)iPlayer == game.getActivePlayer())
			{
				m_iActivePlayer = iPlayerIndex;
			}
			++iPlayerIndex;

			PlayerInfo playerInfo;
			playerInfo.m_eLeader = player.getLeaderType();
			//playerInfo.m_eColor = (ColorTypes)GC.getPlayerColorInfo(player.getPlayerColor()).getColorTypePrimary();
			playerInfo.m_eColor = (ColorTypes)GC.getPlayerColorInfo(GC.getInitCore().getColor((PlayerTypes)iPlayer)).getColorTypePrimary(); // K-Mod. (bypass the conceal colour check.)
			for (int iTurn = m_iInitialTurn; iTurn <= m_iFinalTurn; iTurn++)
			{
				TurnData score;
				score.m_iScore = player.getScoreHistory(iTurn);
				score.m_iAgriculture = player.getAgricultureHistory(iTurn);
				score.m_iIndustry = player.getIndustryHistory(iTurn);
				score.m_iEconomy = player.getEconomyHistory(iTurn);

				playerInfo.m_listScore.push_back(score);
			}
			m_listPlayerScoreHistory.push_back(playerInfo);
		}
	}

	m_listReplayMessages.clear();
	// <advc.106h>
	if(game.getGameState() == GAMESTATE_OVER &&
			GC.getDefineINT("SETTINGS_IN_REPLAYS") > 0)
		addSettingsMsg(); // </advc.106h>
	for (uint i = 0; i < game.getNumReplayMessages(); i++)
	{
		std::map<PlayerTypes, int>::iterator it = mapPlayers.find(game.getReplayMessagePlayer(i));
		if (it != mapPlayers.end())
		{
			CvReplayMessage* pMsg = new CvReplayMessage(game.getReplayMessageTurn(i), game.getReplayMessageType(i), (PlayerTypes)it->second);
			if (NULL != pMsg)
			{
				pMsg->setColor(game.getReplayMessageColor(i));
				pMsg->setText(game.getReplayMessageText(i));
				pMsg->setPlot(game.getReplayMessagePlotX(i), game.getReplayMessagePlotY(i));
				m_listReplayMessages.push_back(pMsg);
			}	
		}
		else
		{
			CvReplayMessage* pMsg = new CvReplayMessage(game.getReplayMessageTurn(i), game.getReplayMessageType(i), NO_PLAYER);
			if (NULL != pMsg)
			{
				pMsg->setColor(game.getReplayMessageColor(i));
				pMsg->setText(game.getReplayMessageText(i));
				pMsg->setPlot(game.getReplayMessagePlotX(i), game.getReplayMessagePlotY(i));
				m_listReplayMessages.push_back(pMsg);
			}	
		}
	}

	m_iMapWidth = GC.getMapINLINE().getGridWidthINLINE();
	m_iMapHeight = GC.getMapINLINE().getGridHeightINLINE();
	
	SAFE_DELETE(m_pcMinimapPixels);	
	m_pcMinimapPixels = new unsigned char[m_nMinimapSize];
	
	void *ptexture = (void*)gDLL->getInterfaceIFace()->getMinimapBaseTexture();
	if (ptexture)
		memcpy((void*)m_pcMinimapPixels, ptexture, m_nMinimapSize);
	if(!STORE_REPLAYS_AS_BTS) // advc.106i
		m_szModName = gDLL->getModName();
	// <advc.707>
	if(m->iFinalScore < 0)
		m->iFinalScore = getFinalPlayerScore(); // </advc.707>
}

// <advc.106h>
void CvReplayInfo::addSettingsMsg() {

	CvGame& g = GC.getGameINLINE();
	PlayerTypes ePlayer = g.getInitialActivePlayer();
	if(ePlayer == NO_PLAYER)
		return;
	bool bScenario = false;
	// Strip away file ending of WB scenario
	CvWString const szWBEnding = L".CivBeyondSwordWBSave";
	CvWString szMapName = getMapScriptName();
	if(szMapName.length() > szWBEnding.length() && szMapName.substr(szMapName.length() -
			szWBEnding.length(), szWBEnding.length()).compare(szWBEnding) == 0) {
		szMapName = szMapName.substr(0, szMapName.length() - szWBEnding.length());
		bScenario = true;
	}
	/*  Can't use getTextKeyWide for sea level b/c of the recommendation text
		added by advc.137 (same issue in CvVictoryScreen.py) */
	int iSeaLevelChange = GC.getSeaLevelInfo(getSeaLevel()).getSeaLevelChange();
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	CvWString szSettings = gDLL->getText("TXT_KEY_MISC_RELOAD", 1) + L". " +
			gDLL->getText("TXT_KEY_MAIN_MENU_SETTINGS") + L":\n" +
			gDLL->getText("TXT_KEY_NAME_LEADER_CIV",
			GC.getLeaderHeadInfo(kPlayer.getLeaderType()).getTextKeyWide(),
			kPlayer.getCivilizationShortDescriptionKey(), kPlayer.getReplayName()) + L"\n" +
			gDLL->getText("TXT_KEY_SETTINGS_DIFFICULTY",
			GC.getHandicapInfo(getDifficulty()).getTextKeyWide()) + L"\n" +
			(bScenario ? szMapName : gDLL->getText("TXT_KEY_SIZE_MAP_WITH",
			GC.getWorldInfo(getWorldSize()).getTextKeyWide(),
			getMapScriptName().GetCString()) + L" " +
			gDLL->getText("TXT_KEY_SETTINGS_SEA_LEVEL",
			(iSeaLevelChange == 0 ? GC.getSeaLevelInfo(getSeaLevel()).getTextKeyWide() :
			gDLL->getText((iSeaLevelChange < 0 ? "TXT_KEY_LOW" : "TXT_KEY_HIGH"))))) +
			(getClimate() == 0 ? L"" : (L", " +
			gDLL->getText("TXT_KEY_SETTINGS_CLIMATE",
			GC.getClimateInfo(getClimate()).getTextKeyWide()))) + L"\n" +
			gDLL->getText("TXT_KEY_SETTINGS_GAME_SPEED",
			GC.getGameSpeedInfo(getGameSpeed()).getTextKeyWide()) +
			(getEra() == 0 ? L"" : (L", " +
			gDLL->getText("TXT_KEY_SETTINGS_STARTING_ERA",
			GC.getEraInfo(getEra()).getTextKeyWide()))) + L"\n";
	// <advc.250b>
	if(g.isOption(GAMEOPTION_ADVANCED_START) && !g.isOption(GAMEOPTION_SPAH)) {
		szSettings += gDLL->getText("TXT_KEY_ADVANCED_START_POINTS") + L" "
				+ CvWString::format(L"%d", g.getNumAdvancedStartPoints()) + L"\n";
	} // </advc.250b>
	int iDisabled = 0;
	for(int i = 0; i < GC.getNumVictoryInfos(); i++) {
		VictoryTypes eVictory = (VictoryTypes)i;
		if(g.isVictoryValid(eVictory))
			continue;
		iDisabled++;
		szSettings += GC.getVictoryInfo(eVictory).getDescription();
		szSettings += L", ";
	}
	if(iDisabled > 0) {
		szSettings = szSettings.substr(0, szSettings.length() - 2) + L" "; // Drop the final comma
		szSettings += gDLL->getText("TXT_KEY_VICTORY_DISABLED") + L"\n";
	} // <advc.250b>
	if(g.isOption(GAMEOPTION_SPAH)) {
		// bTab=false b/c that's a bit too much indentation
		std::wstring* pszPointDistrib = g.startPointsAsHandicap().forSettingsScreen(false);
		if(pszPointDistrib != NULL)
			szSettings += *pszPointDistrib;
	} // </advc.250b>
	int iOptions = 0;
	for(int i = 0; i < GC.getNumGameOptionInfos(); i++) {
		GameOptionTypes eOption = (GameOptionTypes)i;
		// advc.250b:
		if(eOption == GAMEOPTION_ADVANCED_START || eOption == GAMEOPTION_SPAH ||
				!g.isOption(eOption) ||
				// advc.104:
				(eOption == GAMEOPTION_AGGRESSIVE_AI && getWPAI.isEnabled()))
			continue;
		iOptions++;
		szSettings += GC.getGameOptionInfo(eOption).getDescription();
		szSettings += L", ";
	}
	if(iOptions > 0)
		szSettings = szSettings.substr(0, szSettings.length() - 2) + L"\n";
	CvWString const szKey = "TXT_KEY_REPLAY_PREFIX";
	// gDLL->getModName(false) doesn't yield a wstring
	CvWString szModName = gDLL->getText(szKey);
	// Don't list mod name if the tag isn't present
	if(szKey.compare(szModName) == 0)
		szSettings = szSettings.substr(0, szSettings.length() - 1); // drop \n
	else {
		// Remove brackets
		if(szModName.at(0) == '[' && szModName.at(szModName.length() - 1) == ']')
			szModName = szModName.substr(1, szModName.length() - 2);
		szSettings += szModName + L" Mod";
	}
	CvReplayMessage* pSettingsMsg = new CvReplayMessage(0,
			REPLAY_MESSAGE_MAJOR_EVENT, ePlayer);
	pSettingsMsg->setText(szSettings);
	pSettingsMsg->setColor((ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"));
	FAssert(m_listReplayMessages.empty());
	m_listReplayMessages.push_back(pSettingsMsg);
} // </advc.106h>


int CvReplayInfo::getNumPlayers() const
{
	return (int)m_listPlayerScoreHistory.size();
}


bool CvReplayInfo::isValidPlayer(int i) const
{
	return (i >= 0 && i < (int)m_listPlayerScoreHistory.size());
}

bool CvReplayInfo::isValidTurn(int i) const
{
	return (i >= m_iInitialTurn && i <= m_iFinalTurn);
}

int CvReplayInfo::getActivePlayer() const
{
	return m_iActivePlayer;
}

LeaderHeadTypes CvReplayInfo::getLeader(int iPlayer) const
{
	if (iPlayer < 0)
	{
		iPlayer = m_iActivePlayer;
	}
	if (isValidPlayer(iPlayer))
	{
		return m_listPlayerScoreHistory[iPlayer].m_eLeader;
	}
	return NO_LEADER;
}

ColorTypes CvReplayInfo::getColor(int iPlayer) const
{
	if (iPlayer < 0)
	{
		iPlayer = m_iActivePlayer;
	}
	if (isValidPlayer(iPlayer))
	{
		return m_listPlayerScoreHistory[iPlayer].m_eColor;
	}
	return NO_COLOR;
}

HandicapTypes CvReplayInfo::getDifficulty() const
{
	return m_eDifficulty;
}

const CvWString& CvReplayInfo::getLeaderName() const
{
	return m_szLeaderName;
}

const CvWString& CvReplayInfo::getCivDescription() const
{
	return m_szCivDescription;
}

const CvWString& CvReplayInfo::getShortCivDescription() const
{
	return m_szShortCivDescription;
}

const CvWString& CvReplayInfo::getCivAdjective() const
{
	return m_szCivAdjective;
}

const CvWString& CvReplayInfo::getMapScriptName() const
{
	return m_szMapScriptName;
}

WorldSizeTypes CvReplayInfo::getWorldSize() const
{
	return m_eWorldSize;
}

ClimateTypes CvReplayInfo::getClimate() const
{
	return m_eClimate;
}

SeaLevelTypes CvReplayInfo::getSeaLevel() const
{
	return m_eSeaLevel;
}

EraTypes CvReplayInfo::getEra() const
{
	return m_eEra;
}

GameSpeedTypes CvReplayInfo::getGameSpeed() const
{
	return m_eGameSpeed;
}

bool CvReplayInfo::isGameOption(GameOptionTypes eOption) const
{
	for (uint i = 0; i < m_listGameOptions.size(); i++)
	{
		if (m_listGameOptions[i] == eOption)
		{
			return true;
		}
	}
	return false;
}

bool CvReplayInfo::isVictoryCondition(VictoryTypes eVictory) const
{
	for (uint i = 0; i < m_listVictoryTypes.size(); i++)
	{
		if (m_listVictoryTypes[i] == eVictory)
		{
			return true;
		}
	}
	return false;
}

VictoryTypes CvReplayInfo::getVictoryType() const
{
	return m_eVictoryType;
}

bool CvReplayInfo::isMultiplayer() const
{
	return m_bMultiplayer;
}


void CvReplayInfo::addReplayMessage(CvReplayMessage* pMessage)
{
	m_listReplayMessages.push_back(pMessage);
}

void CvReplayInfo::clearReplayMessageMap()
{
	for (ReplayMessageList::const_iterator itList = m_listReplayMessages.begin(); itList != m_listReplayMessages.end(); itList++)
	{
		const CvReplayMessage* pMessage = *itList;
		if (NULL != pMessage)
		{
			delete pMessage;
		}
	}
	m_listReplayMessages.clear();
}

int CvReplayInfo::getReplayMessageTurn(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (-1);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (-1);
	}
	return pMessage->getTurn();
}

ReplayMessageTypes CvReplayInfo::getReplayMessageType(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (NO_REPLAY_MESSAGE);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (NO_REPLAY_MESSAGE);
	}
	return pMessage->getType();
}

int CvReplayInfo::getReplayMessagePlotX(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (-1);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (-1);
	}
	return pMessage->getPlotX();
}

int CvReplayInfo::getReplayMessagePlotY(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (-1);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (-1);
	}
	return pMessage->getPlotY();
}

PlayerTypes CvReplayInfo::getReplayMessagePlayer(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (NO_PLAYER);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (NO_PLAYER);
	}
	return pMessage->getPlayer();
}

LPCWSTR CvReplayInfo::getReplayMessageText(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (NULL);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (NULL);
	}
	return pMessage->getText().GetCString();
}

ColorTypes CvReplayInfo::getReplayMessageColor(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (NO_COLOR);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (NO_COLOR);
	}
	return pMessage->getColor();
}


uint CvReplayInfo::getNumReplayMessages() const
{
	return m_listReplayMessages.size();
}


int CvReplayInfo::getInitialTurn() const
{
	return m_iInitialTurn;
}

int CvReplayInfo::getStartYear() const
{
	return m_iStartYear;
}

int CvReplayInfo::getFinalTurn() const
{
	return m_iFinalTurn;
}

const wchar* CvReplayInfo::getFinalDate() const
{
	return m_szFinalDate;
}

CalendarTypes CvReplayInfo::getCalendar() const
{
	return m_eCalendar;
}


int CvReplayInfo::getPlayerScore(int iPlayer, int iTurn) const
{
	if (isValidPlayer(iPlayer) && isValidTurn(iTurn))
	{
		return m_listPlayerScoreHistory[iPlayer].m_listScore[iTurn-m_iInitialTurn].m_iScore;
	}
	return 0;
}

int CvReplayInfo::getPlayerEconomy(int iPlayer, int iTurn) const
{
	if (isValidPlayer(iPlayer) && isValidTurn(iTurn))
	{
		return m_listPlayerScoreHistory[iPlayer].m_listScore[iTurn-m_iInitialTurn].m_iEconomy;
	}
	return 0;
}

int CvReplayInfo::getPlayerIndustry(int iPlayer, int iTurn) const
{
	if (isValidPlayer(iPlayer) && isValidTurn(iTurn))
	{
		return m_listPlayerScoreHistory[iPlayer].m_listScore[iTurn-m_iInitialTurn].m_iIndustry;
	}
	return 0;
}

int CvReplayInfo::getPlayerAgriculture(int iPlayer, int iTurn) const
{
	if (isValidPlayer(iPlayer) && isValidTurn(iTurn))
	{
		return m_listPlayerScoreHistory[iPlayer].m_listScore[iTurn-m_iInitialTurn].m_iAgriculture;
	}
	return 0;
}

int CvReplayInfo::getFinalScore() const
{
	return m->iFinalScore; // advc.707
}
// <advc.707> This new function does what getFinalScore used to do
int CvReplayInfo::getFinalPlayerScore() const
{
	return getPlayerScore(m_iActivePlayer, m_iFinalTurn);
}
// Can now also set the final score to sth. other than the player score
void CvReplayInfo::setFinalScore(int iScore)
{
	m->iFinalScore = iScore;
} // </advc.707>

int CvReplayInfo::getFinalEconomy() const
{
	return getPlayerEconomy(m_iActivePlayer, m_iFinalTurn);
}

int CvReplayInfo::getFinalIndustry() const
{
	return getPlayerIndustry(m_iActivePlayer, m_iFinalTurn);
}

int CvReplayInfo::getFinalAgriculture() const
{
	return getPlayerAgriculture(m_iActivePlayer, m_iFinalTurn);
}

int CvReplayInfo::getNormalizedScore() const
{
	return m_iNormalizedScore;
}

int CvReplayInfo::getMapHeight() const
{
	return m_iMapHeight;
}

int CvReplayInfo::getMapWidth() const
{
	return m_iMapWidth;
}

const unsigned char* CvReplayInfo::getMinimapPixels() const
{
	return m_pcMinimapPixels;
}

const char* CvReplayInfo::getModName() const
{	/*  <advc.106i> Pretend to the EXE that every replay is an AdvCiv replay.
		(Let CvReplayInfo::read decide which ones to show in HoF.) */ 
	if(STORE_REPLAYS_AS_BTS || GC.getDefineINT("HOF_DISPLAY_BTS_REPLAYS") > 0 ||
			GC.getDefineINT("HOF_DISPLAY_OTHER_MOD_REPLAYS") > 0)
		return m->szPurportedModName; // </advc.106i>
	return m_szModName;
}


bool CvReplayInfo::read(FDataStreamBase& stream)
{
	bool bSuccess = true;
	try {
		int iType;
		int iNumTypes;
		int iVersion;
		stream.Read(&iVersion);
		// <advc.106i> Unpack mod id and version
		int iAdvCivID = GC.getDefineINT("SAVE_VERSION");
		int iModID = -1;
		m->bDisplayOtherMods = (GC.getDefineINT("HOF_DISPLAY_OTHER_MOD_REPLAYS") > 0);
		if(iVersion >= 100 * iAdvCivID) {
			iModID = iVersion / 100;
			iVersion = iVersion % 100;
			/*  This would have to be an AdvCiv modmod with a different id, or
				any mod that uses a very high replay version number. */
			if(iModID != iAdvCivID && !m->bDisplayOtherMods)
				return false;
		} /* Replay from another mod that has increased the replay version;
			 won't be able to parse that. (Actually, it might be OK - if the mod
			 only appends additional data at the end of the stream.) */
		if(iVersion > REPLAY_VERSION)
			return false;
		m->iVersionRead = iVersion; // For the checkBounds function
		// </advc.106i>
		if (iVersion < 2)
			return false;
		stream.Read(&m_iActivePlayer);
		if(!checkBounds(m_iActivePlayer, 0, 64)) return false; // advc.106i
		stream.Read(&iType);
		m_eDifficulty = (HandicapTypes)iType;
		if(!checkBounds(m_eDifficulty, 0, GC.getNumHandicapInfos())) return false; // advc.106i  (not -1 b/c of advc.250a)
		stream.ReadString(m_szLeaderName);
		stream.ReadString(m_szCivDescription);
		stream.ReadString(m_szShortCivDescription);
		stream.ReadString(m_szCivAdjective);
		if(iVersion > 3)
			stream.ReadString(m_szMapScriptName);
		else m_szMapScriptName = gDLL->getText("TXT_KEY_TRAIT_PLAYER_UNKNOWN");
		if(!checkBounds(m_szLeaderName.length() + m_szCivDescription.length() + m_szShortCivDescription.length() + m_szCivAdjective.length() + m_szMapScriptName.length(), 5, 250)) return false; // advc.106i
		stream.Read(&iType);
		m_eWorldSize = (WorldSizeTypes)iType;
		if(!checkBounds(m_eWorldSize, 0, GC.getNumWorldInfos() - 1)) return false; // advc.106i
		stream.Read(&iType);
		m_eClimate = (ClimateTypes)iType;
		if(!checkBounds(m_eClimate, 0, GC.getNumClimateInfos() - 1)) return false; // advc.106i
		stream.Read(&iType);
		 // <advc.106i> For compatibility with advc.707
		if(iVersion != 5 && iVersion >= 6) { // ==5 handled at the end of this function
			m->iFinalScore = iType;
			m_eSeaLevel = NO_SEALEVEL; // unused
		}
		else {
			m->iFinalScore = MIN_INT; // Compute it later
			m_eSeaLevel = (SeaLevelTypes)iType;
			if(!checkBounds(m_eSeaLevel, 0, GC.getNumSeaLevelInfos() - 1)) return false; // advc.106i
		} // </advc.106i>
		stream.Read(&iType);
		m_eEra = (EraTypes)iType;
		if(!checkBounds(m_eEra, 0, GC.getNumEraInfos() - 1)) return false; // advc.106i
		stream.Read(&iType);
		m_eGameSpeed = (GameSpeedTypes)iType;
		if(!checkBounds(m_eGameSpeed, 0, GC.getNumGameSpeedInfos() - 1)) return false; // advc.106i
		stream.Read(&iNumTypes);
		if(!checkBounds(iNumTypes, 0, GC.getNumGameOptionInfos() - 1)) return false; // advc.106i
		for (int i = 0; i < iNumTypes; i++)
		{
			stream.Read(&iType);
			if(!checkBounds(iType, 0, GC.getNumGameOptionInfos() - 1)) return false; // advc.106i
			m_listGameOptions.push_back((GameOptionTypes)iType);
		}
		stream.Read(&iNumTypes);
		for (int i = 0; i < iNumTypes; i++)
		{
			stream.Read(&iType);
			if(!checkBounds(iType, 0, GC.getNumVictoryInfos() - 1)) return false; // advc.106i
			m_listVictoryTypes.push_back((VictoryTypes)iType);
		}
		stream.Read(&iType);
		m_eVictoryType = (VictoryTypes)iType;
		if(!checkBounds(m_eVictoryType, -1, GC.getNumVictoryInfos() - 1)) return false; // advc.106i
		stream.Read(&iNumTypes);
		if(!checkBounds(iNumTypes, 0, 50000)) return false; // advc.106i
		for (int i = 0; i < iNumTypes; i++)
		{
			CvReplayMessage* pMessage = new CvReplayMessage(0);
			if (NULL != pMessage)
			{
				pMessage->read(stream);
			}
			m_listReplayMessages.push_back(pMessage);
		}
		stream.Read(&m_iInitialTurn);
		if(!checkBounds(m_iInitialTurn, 0, 1500)) return false; // advc.106i
		stream.Read(&m_iStartYear);
		if(!checkBounds(m_iStartYear, -10000, 100000)) return false; // advc.106i
		stream.Read(&m_iFinalTurn);
		checkBounds(m_iFinalTurn, 0, 10000); // advc.106i
		stream.ReadString(m_szFinalDate);
		if(!checkBounds(m_szFinalDate.length(), 4, 50)) return false; // advc.106i
		stream.Read(&iType);
		m_eCalendar = (CalendarTypes)iType;
		if(!checkBounds(m_eCalendar, 0, GC.getNumCalendarInfos() - 1)) return false; // advc.106i
		stream.Read(&m_iNormalizedScore);
		if(!checkBounds(m_iNormalizedScore, -10000, 1000000)) return false; // advc.106i
		stream.Read(&iNumTypes);
		if(!checkBounds(iNumTypes, 1, 64)) return false; // advc.106i
		for (int i = 0; i < iNumTypes; i++)
		{
			PlayerInfo info;
			stream.Read(&iType);
			info.m_eLeader = (LeaderHeadTypes)iType;
			if(!checkBounds(info.m_eLeader, 0, GC.getNumLeaderHeadInfos() - 1)) return false; // advc.106i
			stream.Read(&iType);
			info.m_eColor = (ColorTypes)iType;
			if(!checkBounds(info.m_eColor, 0, GC.getNumColorInfos() - 1)) return false; // advc.106i
			int jNumTypes;
			stream.Read(&jNumTypes);
			if(!checkBounds(jNumTypes, 0, 10000)) return false; // advc.106i
			for (int j = 0; j < jNumTypes; j++)
			{
				TurnData data;
				stream.Read(&(data.m_iScore));
				stream.Read(&(data.m_iEconomy));
				stream.Read(&(data.m_iIndustry));
				stream.Read(&(data.m_iAgriculture));
				info.m_listScore.push_back(data);
			}
			m_listPlayerScoreHistory.push_back(info);
		}
		stream.Read(&m_iMapWidth);
		if(!checkBounds(m_iMapWidth, 1, 1000)) return false; // advc.106i
		stream.Read(&m_iMapHeight);
		if(!checkBounds(m_iMapHeight, 1, 1000)) return false; // advc.106i
		SAFE_DELETE(m_pcMinimapPixels);
		m_pcMinimapPixels = new unsigned char[m_nMinimapSize];
		stream.Read(m_nMinimapSize, m_pcMinimapPixels);
		stream.Read(&m_bMultiplayer);
		if (iVersion > 2)
		{	// <advc.106i>
			try { /* I've had some issues with m_szModName sometimes remaining
					 uninitialized, so I've added a separate try block. */
				stream.ReadString(m_szModName);
				m_szModName.GetLength(); // To check if bad ptr
			} catch(...) {
				FAssertMsg(false, "Failed to read replay file");
				return false;
			}
			if(m_szModName.empty()) {
				if(iModID < 0 && GC.getDefineINT("HOF_DISPLAY_BTS_REPLAYS") <= 0)
					return false;
			}
			else if(!m->bDisplayOtherMods &&
					std::strcmp(m_szModName.GetCString(), gDLL->getModName()) != 0)
				return false; // Replay from a different mod
			// </advc.106i>
		} // <advc.707>
		if(iVersion == 5)
			stream.Read(&m->iFinalScore);
		// </advc.707>
	} catch(...) {
		FAssertMsg(false, "Failed to read replay file");
		return false;
	} // <advc.707>
	if(m->iFinalScore == MIN_INT)
		m->iFinalScore = getFinalPlayerScore();
	// </advc.707>
	return bSuccess;
}

void CvReplayInfo::write(FDataStreamBase& stream)
{
	//stream.Write(REPLAY_VERSION);
	// <advc.106i> Fold AdvCiv's (hopefully) globally unique id into the replay version
	stream.Write(GC.getDefineINT("SAVE_VERSION") * 100 + REPLAY_VERSION);
	stream.Write(m_iActivePlayer);
	stream.Write((int)m_eDifficulty);
	stream.WriteString(m_szLeaderName);
	stream.WriteString(m_szCivDescription);
	stream.WriteString(m_szShortCivDescription);
	stream.WriteString(m_szCivAdjective);
	stream.WriteString(m_szMapScriptName);
	stream.Write((int)m_eWorldSize);
	stream.Write((int)m_eClimate);
	//stream.Write((int)m_eSeaLevel);
	/*  advc.106i: m_eSeaLevel is unused in AdvCiv and BtS. Need to stick to the
		BtS replay format. Well, it's probably OK to append additional data at
		the end, but I'm not quite sure. */
	stream.Write(m->iFinalScore); // advc.707
	stream.Write((int)m_eEra);
	stream.Write((int)m_eGameSpeed);
	stream.Write((int)m_listGameOptions.size());
	for (uint i = 0; i < m_listGameOptions.size(); i++)
	{
		stream.Write((int)m_listGameOptions[i]);
	}
	stream.Write((int)m_listVictoryTypes.size());
	for (uint i = 0; i < m_listVictoryTypes.size(); i++)
	{
		stream.Write((int)m_listVictoryTypes[i]);
	}
	stream.Write((int)m_eVictoryType);
	stream.Write((int)m_listReplayMessages.size());
	for (uint i = 0; i < m_listReplayMessages.size(); i++)
	{
		if (NULL != m_listReplayMessages[i])
		{
			m_listReplayMessages[i]->write(stream);
		}
	}
	stream.Write(m_iInitialTurn);
	stream.Write(m_iStartYear);
	stream.Write(m_iFinalTurn);
	stream.WriteString(m_szFinalDate);
	stream.Write((int)m_eCalendar);
	stream.Write(m_iNormalizedScore);
	stream.Write((int)m_listPlayerScoreHistory.size());
	for (uint i = 0; i < m_listPlayerScoreHistory.size(); i++)
	{
		PlayerInfo& info = m_listPlayerScoreHistory[i];
		stream.Write((int)info.m_eLeader);
		stream.Write((int)info.m_eColor);
		stream.Write((int)info.m_listScore.size());
		for (uint j = 0; j < info.m_listScore.size(); j++)
		{
			stream.Write(info.m_listScore[j].m_iScore);
			stream.Write(info.m_listScore[j].m_iEconomy);
			stream.Write(info.m_listScore[j].m_iIndustry);
			stream.Write(info.m_listScore[j].m_iAgriculture);
		}
	}
	stream.Write(m_iMapWidth);
	stream.Write(m_iMapHeight);
	stream.Write(m_nMinimapSize, m_pcMinimapPixels);
	stream.Write(m_bMultiplayer);
	stream.WriteString(m_szModName);
}

// <advc.106i>
bool CvReplayInfo::checkBounds(int iValue, int iLower, int iUpper) const {

	/*  If CvReplayInfo::read encounters a replay from another mod, it won't be able
		to tell until it reaches the mod name at the end of the stream. I'm not aware
		of any mods that change the replay format, but, if they do, the read function
		is going to get out of step, which may throw an exception, which causes
		the replay to be disregarded (good). If there is no exception, then the
		mod name check should reject the replay. But ... I don't know, I don't
		want to rely on exceptions. As all mods place their replays in the same
		folder, it's not such an exceptional thing to encounter a strange replay. */
	if(m->bDisplayOtherMods) // Still want sanity checks then, but be more generous.
		iUpper *= 2;
	if(iValue < iLower || iValue > iUpper) {
		/*  The assertion is only there to warn me when an AdvCiv replay gets
			rejected. If another mod has changed the replay format, then probably
			just once, from version 4 to 5, but not to REPLAY_VERSION. */
		FAssert(m->bDisplayOtherMods || m->iVersionRead < REPLAY_VERSION);
		return false;
	}
	return true;
} // </advc.106i>
