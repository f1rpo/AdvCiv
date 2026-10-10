## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
from CvPythonExtensions import *
import CvUtil
import ScreenInput
import PyHelpers
import time

# BUG - start
import AttitudeUtil
import BugCore
import BugPath
import CvModName # <!-- custom: Show centralized runtime/source identity and persisted save-version history in the Settings tab. (ChatGPT-5.6-Sol) -->
import BugUtil
import ColorUtil
import GameUtil
import PlayerUtil
import TechUtil
from SASFontUtils import *
import SASTextScale
from SASUtils import *
# <!-- custom: AdvCiv-SAS readability pass: use LABEL as the base victory-screen text tag (instead of BODY) for clearer upscaled UI text. (GPT-5.3-Codex) -->
# <!-- custom: add trait icons in the Settings tab (claude opus 4.5). -->
import TraitUtil

AdvisorOpt = BugCore.game.Advisors

AdvisorOpt = BugCore.game.Advisors
# BUG - end

# BUG - Mac Support - start
BugUtil.fixSets(globals())
# BUG - Mac Support - end

PyPlayer = PyHelpers.PyPlayer

# globals
gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

VICTORY_CONDITION_SCREEN = 0
GAME_SETTINGS_SCREEN = 1
UN_RESOLUTION_SCREEN = 2
UN_MEMBERS_SCREEN = 3
# advc.703
RF_SCORE_SCREEN = 4

class CvVictoryScreen:
	"Keeps track of victory conditions"

	def __init__(self, screenId):
		self.screenId = screenId
		self.SCREEN_NAME = "VictoryScreen"
		self.DEBUG_DROPDOWN_ID =  "VictoryScreenDropdownWidget"
		self.INTERFACE_ART_INFO = "TECH_BG"
		self.EXIT_AREA = "EXIT"
		self.EXIT_ID = "VictoryScreenExit"
		self.BACKGROUND_ID = "VictoryScreenBackground"
		self.HEADER_ID = "VictoryScreenHeader"
		self.WIDGET_ID = "VictoryScreenWidget"
		self.VC_TAB_ID = "VictoryTabWidget"
		self.SETTINGS_TAB_ID = "SettingsTabWidget"
		self.UN_RESOLUTION_TAB_ID = "VotingTabWidget"
		self.UN_MEMBERS_TAB_ID = "MembersTabWidget"
		self.PAGE_SCREEN_IDS = [VICTORY_CONDITION_SCREEN, GAME_SETTINGS_SCREEN, UN_RESOLUTION_SCREEN, UN_MEMBERS_SCREEN]
		self.PAGE_TAB_IDS = [self.VC_TAB_ID, self.SETTINGS_TAB_ID, self.UN_RESOLUTION_TAB_ID, self.UN_MEMBERS_TAB_ID]
		self.PAGE_NAME_LIST = []
		self.PAGE_LINK_WIDTH = []
		self.SPACESHIP_SCREEN_BUTTON = 1234

		# <advc.703>
		self.RF_SCORE_TAB_ID = "RiseFallTabWidget"
		# Adopted from CvReligionScreen:
		self.AREA1_ID =  "RiseFallAreaWidget1"
		self.AREA2_ID =  "RiseFallAreaWidget2"
		self.X_RF1_AREA = 45
		self.X_RF2_AREA = 522
		self.W_RF_AREA = 457
		# (These two can get adjusted based on the chapter count)
		self.Y_RF_AREA = 350
		self.H_RF_AREA = 340
		# </advc.703>

		self.Z_BACKGROUND = -6.1
		self.Z_CONTROLS = self.Z_BACKGROUND - 0.2
		self.DZ = -0.2

		self.Y_TITLE = SAS_ADVISOR_TITLE_Y

		self.X_AREA = 10
		self.Y_AREA = 60

		# <!-- custom: adjusted column widths for percentage display (claude opus 4.5) -->
		someWRoomForCulturePercentages = 27
		self.TABLE_WIDTH_BASE = [350 - (2 * someWRoomForCulturePercentages), 80, 180, 100 + someWRoomForCulturePercentages, 180, 100 + someWRoomForCulturePercentages]
		self.TABLE2_WIDTH_BASE = [740, 265]
		# <advc.703>
		self.RF_TABLEW_BASE = [125, 150, 250, 260, 205]
		# </advc.703>
# BUG Additions Start
		self.TABLE3_WIDTH_BASE = [450, 90, 90, 90, 90, 200]

		self.Vote_Pope_ID = "BUGVotePope_Widget"
		self.Vote_DipVic_ID = "BUGVoteDiplomaticVictory_Widget"
		self.Vote_X = 20
		self.Vote_Y = 688
		self.VoteType = 1  # 1 = Pope or GenSec, 2 = Diplomatic Victory
		self.VoteBody = 1  # 1 = AP, 2 = UN

		self.Vote_AP_ID = "BUGVoteAP_Widget"
		self.Vote_UN_ID = "BUGVoteUN_Widget"
# BUG Additions End

		self.X_LINK = 100
		self.DX_LINK = 220
		self.MARGIN = 20
		# <advc.004> Panel width increased; especially for change advc.708 (display of player handicap on one line).
		self.SETTINGS_PANEL_X1 = 30 # was 50
		self.SETTINGS_PANEL_X2 = 355
		self.SETTINGS_PANEL_X3 = 680 # was 660
		self.SETTINGS_PANEL_WIDTH = 320 # was 300
		# </advc.004>
		self.SETTINGS_PANEL_Y = 150
		self.SETTINGS_PANEL_HEIGHT = 500
		self.SETTINGS_PANEL_OUTER_MARGIN = 30

		self.nWidgetCount = 0
		self.iActivePlayer = -1
		self.bVoteTab = False

		self.iScreen = VICTORY_CONDITION_SCREEN

		self.ApolloTeamsChecked = set()
		self.ApolloTeamCheckResult = {}

		# <!-- custom: leader icon size for inline display, based on AdvCiv-SAS's approach in the Info Screen (claude opus 4.5). -->
		self.iLeaderIconSize = 24
		self.iEmojiAsIconIconSize = 16

		self.iLeaderButtonSize = 24

		# <!-- custom: language tracking for initText, based on Info Screen pattern (claude opus 4.5) -->
		self.iLanguageLoaded = -1
		self.SAS_CV_VICTORY_SCREEN_SETTINGS_AI_MAP_HEAVINESS_SHOW = None

		# <!-- custom: keep screen-independent advisor edge constants in init; compute runtime resolution-dependent bounds in interfaceScreen through shared SASUtils helpers. (GPT-5.3-Codex) -->
		self.W_LEFT_SPACE_FOR_COMMERCE_SLIDERS = SAS_ADVISOR_LEFT_SPACE_FOR_COMMERCE_SLIDERS
		self.W_RIGHT_SPACE_FOR_SCOREBOARD = SAS_ADVISOR_RIGHT_SPACE_FOR_SCOREBOARD
		self.H_TOP_SPACE_FOR_TECH_BAR = SAS_ADVISOR_TOP_SPACE_FOR_TECH_BAR
		self.H_BOTTOM_SPACE = SAS_ADVISOR_BOTTOM_SPACE

	def updateRuntimeLayout(self, screen):
		# <!-- custom: runtime-only geometry (X/Y/W/H anchors and content areas) is computed here from actual resolution; we intentionally avoid init-time placeholders for these screen-dependent values. (GPT-5.3-Codex) -->
		self.L_SCREEN, self.T_SCREEN, self.W_SCREEN, self.H_SCREEN = getAdvisorRuntimeBounds(screen, self.W_LEFT_SPACE_FOR_COMMERCE_SLIDERS, self.W_RIGHT_SPACE_FOR_SCOREBOARD, self.H_TOP_SPACE_FOR_TECH_BAR, self.H_BOTTOM_SPACE)
		self.X_SCREEN, self.X_EXIT, self.Y_EXIT, self.Y_LINK, self.Y_BOTTOM_PANEL = getAdvisorRuntimeAnchors(self.W_SCREEN, self.H_SCREEN)
		self.W_AREA = self.W_SCREEN - 14
		self.H_AREA = self.H_SCREEN - 118

		iTargetTableWidth = self.W_AREA - 5
		self.TABLE_WIDTH_0, self.TABLE_WIDTH_1, self.TABLE_WIDTH_2, self.TABLE_WIDTH_3, self.TABLE_WIDTH_4, self.TABLE_WIDTH_5 = self.scaleColumnsToWidth(self.TABLE_WIDTH_BASE, iTargetTableWidth)
		self.TABLE2_WIDTH_0, self.TABLE2_WIDTH_1 = self.scaleColumnsToWidth(self.TABLE2_WIDTH_BASE, iTargetTableWidth)
		self.TABLE3_WIDTH_0, self.TABLE3_WIDTH_1, self.TABLE3_WIDTH_2, self.TABLE3_WIDTH_3, self.TABLE3_WIDTH_4, self.TABLE3_WIDTH_5 = self.scaleColumnsToWidth(self.TABLE3_WIDTH_BASE, iTargetTableWidth)
		self.RF_TABLEW_0, self.RF_TABLEW_1, self.RF_TABLEW_2, self.RF_TABLEW_3, self.RF_TABLEW_4 = self.scaleColumnsToWidth(self.RF_TABLEW_BASE, iTargetTableWidth)
		# <!-- custom: for Resolutions/Members readability, reduce the first column only (no reallocation); remaining width stays unused on the right. (GPT-5.3-Codex) -->
		self.TABLE2_WIDTH_0 = self.TABLE2_WIDTH_0 / 2
		self.TABLE3_WIDTH_0 = self.TABLE3_WIDTH_0 / 2

		iGap = self.SETTINGS_PANEL_OUTER_MARGIN
		self.SETTINGS_PANEL_WIDTH = (self.W_SCREEN - 4 * iGap) / 3
		self.SETTINGS_PANEL_X1 = iGap
		self.SETTINGS_PANEL_X2 = self.SETTINGS_PANEL_X1 + self.SETTINGS_PANEL_WIDTH + iGap
		self.SETTINGS_PANEL_X3 = self.SETTINGS_PANEL_X2 + self.SETTINGS_PANEL_WIDTH + iGap
		self.SETTINGS_PANEL_Y = self.Y_AREA + iGap + 10
		self.SETTINGS_PANEL_HEIGHT = self.Y_BOTTOM_PANEL - self.Y_AREA - 2 * iGap
		self.PAGE_LINK_WIDTH[:] = []
		for i in range(len(self.PAGE_TAB_IDS)):
			self.PAGE_LINK_WIDTH.append(self.DX_LINK)

	def scaleColumnsToWidth(self, aiBaseWidths, iTargetWidth):
		iBaseTotal = sum(aiBaseWidths)
		iColumnCount = len(aiBaseWidths)
		if iColumnCount <= 0:
			return []
		if iTargetWidth < iColumnCount:
			iTargetWidth = iColumnCount
		aiScaled = []
		iUsed = 0
		for i in range(iColumnCount - 1):
			iWidth = (aiBaseWidths[i] * iTargetWidth + iBaseTotal / 2) / iBaseTotal
			if iWidth < 1:
				iWidth = 1
			aiScaled.append(iWidth)
			iUsed += iWidth
		iLast = iTargetWidth - iUsed
		if iLast < 1:
			iLast = 1
		aiScaled.append(iLast)
		return aiScaled

	# <!-- custom: initialize text and image tags once for performance, based on Info Screen pattern (claude opus 4.5) -->
	def initText(self):
		# only execute this function once per language...
		if self.iLanguageLoaded == CyGame().getCurrentLanguage() or not CyGame().isFinalInitialized():
			return
		self.iLanguageLoaded = CyGame().getCurrentLanguage()

		# <!-- custom: precompute trophy icon for Highest Score row (claude opus 4.5) -->
		szTrophyIconPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_TROPHY").getPath()
		self.szTrophyImgTag = u"<img=%s size=%d></img>" % (szTrophyIconPath, self.iEmojiAsIconIconSize)
		self.ART_MAINMENU_SLIDESHOW_LOAD = ArtFileMgr.getInterfaceArtInfo("MAINMENU_SLIDESHOW_LOAD").getPath()

		# <!-- custom: precompute commonly used icon chars (claude opus 4.5) -->
		self.iStrengthIcon = CyGame().getSymbolID(FontSymbols.STRENGTH_CHAR)
		self.iCitizenIcon = CyGame().getSymbolID(FontSymbols.CITIZEN_CHAR)
		self.iMapIcon = CyGame().getSymbolID(FontSymbols.MAP_CHAR)
		self.iBulletIcon = CyGame().getSymbolID(FontSymbols.BULLET_CHAR)
		self.iStarIcon = CyGame().getSymbolID(FontSymbols.STAR_CHAR)
		self.iSilverStarIcon = CyGame().getSymbolID(FontSymbols.SILVER_STAR_CHAR)
		self.iCultureIcon = gc.getCommerceInfo(CommerceTypes.COMMERCE_CULTURE).getChar()

		# <!-- custom: precompute commonly used text strings (claude opus 4.5) -->
		self.TEXT_RIVALS_LEFT = localText.getText("TXT_KEY_VICTORY_SCREEN_RIVALS_LEFT", ())
		self.TEXT_FULL_MEMBER = localText.getText("TXT_KEY_VOTESOURCE_FULL_MEMBER", ())
		self.TEXT_VOTING_MEMBER = localText.getText("TXT_KEY_VOTESOURCE_VOTING_MEMBER", ())
		self.TEXT_NON_VOTING_MEMBER = localText.getText("TXT_KEY_VOTESOURCE_NON_VOTING_MEMBER", ())
		self.TEXT_LEGENDARY_CITIES = localText.getText("TXT_KEY_VICTORY_SCREEN_LEGENDARY_CITIES", ())
		self.TEXT_POPUP_PASSED = localText.getText("TXT_KEY_POPUP_PASSED", ())
		self.TEXT_NOT_YET_BUILT = localText.getText("TXT_KEY_VICTORY_SCREEN_NOT_BUILT", ())
		self.TEXT_VOTE_SOURCE_NOT_ACTIVE = localText.getText("TXT_KEY_SAS_VICTORY_SCREEN_VOTE_SOURCE_NOT_ACTIVE", ())
		self.TEXT_MEMBERS_NO_VOTING_YET = localText.getText("TXT_KEY_SAS_VICTORY_SCREEN_MEMBERS_NO_VOTING_YET", ())
		self.TEXT_NOT_ACTIVE_SHORT = localText.getText("TXT_KEY_SAS_NOT_ACTIVE_SHORT", ())
		self.TEXT_THOUSANDS_SEPARATOR_COMMA = localText.getText("TXT_KEY_THOUSANDS_SEPARATOR_COMMA", ())
		self.COLOR_YELLOW = getInfoTypeOrFail("COLOR_YELLOW")
		self.EXIT_TEXT = sasFontTagTitle + localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT", ()).upper() + SAS_FONT_TAG_CLOSE
		self.HEADER_TEXT = sasFontTagTitle.bold + localText.getText("TXT_KEY_VICTORY_SCREEN_TITLE", ()).upper() + SAS_FONT_TAG_CLOSE
		self.TAB_VICTORIES = localText.getText("TXT_KEY_MAIN_MENU_VICTORIES", ()).upper()
		self.TAB_SETTINGS = localText.getText("TXT_KEY_MAIN_MENU_SETTINGS", ()).upper()
		self.TAB_VOTING = localText.getText("TXT_KEY_VOTING_TITLE", ()).upper()
		self.TAB_MEMBERS = localText.getText("TXT_KEY_MEMBERS_TITLE", ()).upper()
		self.TAB_SCORE = localText.getText("TXT_KEY_GAME_SCORE", ()).upper()
		self.PAGE_NAME_LIST[:] = [self.TAB_VICTORIES, self.TAB_SETTINGS, self.TAB_VOTING, self.TAB_MEMBERS]
		self.TEXT_DIPLOMATIC = localText.getText("TXT_KEY_BUG_VICTORY_DIPLOMATIC", ())
		self.LABEL_SETTINGS_HEADER = sasFontTagLabel.bold + self.TAB_SETTINGS + SAS_FONT_TAG_CLOSE
		self.LABEL_OPTIONS_HEADER = sasFontTagLabel.bold + localText.getText("TXT_KEY_MAIN_MENU_CUSTOM_SETUP_OPTIONS", ()).upper() + SAS_FONT_TAG_CLOSE

		# <!-- custom: precompute fully formatted strings that never change (claude opus 4.5) -->
		self.szConquestText = u"%c %s" % (self.iStrengthIcon, localText.getText("TXT_KEY_VICTORY_SCREEN_ELIMINATE_ALL", ()))
		self.szFullMemberText = u"%c %s" % (self.iSilverStarIcon, self.TEXT_FULL_MEMBER)
		self.szVotingMemberText = u"%c %s" % (self.iBulletIcon, self.TEXT_VOTING_MEMBER)

	# <!-- custom: helper function to get leader icon image tag for a player (claude opus 4.5) -->
	def getLeaderIconTag(self, iPlayer):
		if iPlayer < 0 or iPlayer >= gc.getMAX_PLAYERS():
			return u""
		pPlayer = gc.getPlayer(iPlayer)
		# <!-- custom: show icon even for dead players (for Settings tab) (claude opus 4.5) -->
		if not pPlayer.isEverAlive():
			return u""
		szLeaderButton = gc.getLeaderHeadInfo(pPlayer.getLeaderType()).getButton()
		return u"<img=%s size=%d></img>" % (szLeaderButton, self.iLeaderButtonSize)

	def getLeaderButton(self, iPlayer):
		if iPlayer < 0 or iPlayer >= gc.getMAX_PLAYERS():
			return u""
		pPlayer = gc.getPlayer(iPlayer)
		if not pPlayer.isEverAlive():
			return u""
		return gc.getLeaderHeadInfo(pPlayer.getLeaderType()).getButton()

	# <!-- custom: helper function to get player name with leader icon (claude opus 4.5) -->
	def getPlayerNameWithIcon(self, iPlayer):
		if iPlayer < 0 or iPlayer >= gc.getMAX_PLAYERS():
			return u""
		pPlayer = gc.getPlayer(iPlayer)
		szLeaderIcon = self.getLeaderIconTag(iPlayer)
		return u"%s %s" % (szLeaderIcon, pPlayer.getName())

	def getScreen(self):
		return CyGInterfaceScreen(self.SCREEN_NAME, self.screenId)

	def hideScreen(self):
		screen = self.getScreen()
		screen.hideScreen()

	def interfaceScreen(self):

		# <!-- custom: initialize text once for performance (claude opus 4.5) -->
		self.initText()

		# Create a new screen
		screen = self.getScreen()
		if screen.isActive():
			return
		screen.setRenderInterfaceOnly(True)
		screen.showScreen(PopupStates.POPUPSTATE_IMMEDIATE, False)
		self.updateRuntimeLayout(screen)

		# <!-- custom: preserve the selected debug/vassal perspective when reopening Victory screen; base code reset to the active player every open. (GPT-5.5) -->
		self.iActivePlayer = getAdvisorValidPerspectivePlayer(self.iActivePlayer, bAllowVassalPerspective=True)
		if self.iScreen == -1:
			self.iScreen = VICTORY_CONDITION_SCREEN

		# Set the background widget and exit button
		screen.addDDSGFC(self.BACKGROUND_ID, self.ART_MAINMENU_SLIDESHOW_LOAD, 0, 0, self.W_SCREEN, self.H_SCREEN, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		screen.addPanel( "TechTopPanel", u"", u"", True, False, 0, 0, self.W_SCREEN, 55, PanelStyles.PANEL_STYLE_TOPBAR )
		screen.addPanel( "TechBottomPanel", u"", u"", True, False, 0, self.Y_BOTTOM_PANEL, self.W_SCREEN, 55, PanelStyles.PANEL_STYLE_BOTTOMBAR )
		screen.showWindowBackground( False )
		screen.setDimensions(self.L_SCREEN, self.T_SCREEN, self.W_SCREEN, self.H_SCREEN)
		screen.setText(self.EXIT_ID, "Background", self.EXIT_TEXT, CvUtil.FONT_RIGHT_JUSTIFY, self.X_EXIT, self.Y_EXIT, self.Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_CLOSE_SCREEN, -1, -1 )

		# Header...
		screen.setLabel(self.HEADER_ID, "Background", self.HEADER_TEXT, CvUtil.FONT_CENTER_JUSTIFY, self.X_SCREEN, self.Y_TITLE, self.Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		if self.iScreen == VICTORY_CONDITION_SCREEN:
			self.showVictoryConditionScreen()
		elif self.iScreen == GAME_SETTINGS_SCREEN:
			self.showGameSettingsScreen()
		elif self.iScreen == UN_RESOLUTION_SCREEN:
			self.showVotingScreen()
		elif self.iScreen == UN_MEMBERS_SCREEN:
			self.showMembersScreen()
		# <advc.703>
		elif self.iScreen == RF_SCORE_SCREEN:
			self.showRiseFall()
		# </advc.703>

	def redrawActiveTab(self):
		if self.iScreen == VICTORY_CONDITION_SCREEN:
			self.showVictoryConditionScreen()
		elif self.iScreen == GAME_SETTINGS_SCREEN:
			self.showGameSettingsScreen()
		elif self.iScreen == UN_RESOLUTION_SCREEN:
			self.showVotingScreen()
		elif self.iScreen == UN_MEMBERS_SCREEN:
			self.showMembersScreen()
		elif self.iScreen == RF_SCORE_SCREEN:
			self.showRiseFall()

	def drawTabs(self):
		screen = self.getScreen()

		# <!-- custom: always show Voting/Members tabs for exhaustive navigation, even when no vote source is currently active; empty screens are preferable to hidden tabs so users can discover all sections. (GPT-5.3-Codex) -->
		aszPageTabIDs = list(self.PAGE_TAB_IDS)
		aszPageNames = list(self.PAGE_NAME_LIST)
		aiPageLinkWidths = list(self.PAGE_LINK_WIDTH)
		aiPageScreenIDs = list(self.PAGE_SCREEN_IDS)
		# <advc.703>
		if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_RISE_FALL):
			aszPageTabIDs.append(self.RF_SCORE_TAB_ID)
			aszPageNames.append(self.TAB_SCORE)
			aiPageLinkWidths.append(self.DX_LINK)
			aiPageScreenIDs.append(RF_SCORE_SCREEN)
		# </advc.703>
		drawAdvisorFooterTabs(screen, aszPageTabIDs, aszPageNames, aiPageLinkWidths, self.iScreen, self.Y_LINK, 0, self.COLOR_YELLOW, aiInactiveData1=aiPageScreenIDs, iXStart=self.X_LINK - self.DX_LINK / 2)

	def drawVoteSourceInactiveFallback(self, szIntroText, aiVoteBuildingClass):
		screen = self.getScreen()
		screen.addPanel(self.getNextWidgetName(), "", "", False, False, self.X_AREA-10, self.Y_AREA-15, self.W_AREA+20, self.H_AREA+30, PanelStyles.PANEL_STYLE_BLUE50)
		szTable = self.getNextWidgetName()
		screen.addTableControlGFC(szTable, 2, self.X_AREA, self.Y_AREA, self.W_AREA, self.H_AREA, False, False, self.iLeaderButtonSize, self.iLeaderButtonSize, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSelect(szTable, False)
		screen.setTableColumnHeader(szTable, 0, "", self.TABLE2_WIDTH_0)
		screen.setTableColumnHeader(szTable, 1, "", self.TABLE2_WIDTH_1)

		# <!-- custom: shared fallback table for empty vote-source tabs (Voting/Members).
		# Keep exhaustive tab navigation, show caller-specific intro text, and list required vote wonders with built/active state. (GPT-5.3-Codex) -->
		iRow = screen.appendTableRow(szTable)
		SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, szIntroText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		dRequirements = {}
		for (iVoteSource, iVoteBuildingClass, iUNTeam, bUnknown) in aiVoteBuildingClass:
			if not dRequirements.has_key(iVoteSource):
				dRequirements[iVoteSource] = (iVoteBuildingClass, iUNTeam, bUnknown)

		if len(dRequirements) == 0:
			for iBuilding in range(gc.getNumBuildingInfos()):
				iVoteSource = gc.getBuildingInfo(iBuilding).getVoteSourceType()
				if iVoteSource >= 0 and not dRequirements.has_key(iVoteSource):
					iBuildingClass = gc.getBuildingInfo(iBuilding).getBuildingClassType()
					iUNTeam = -1
					bUnknown = true
					for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
						if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
							if (gc.getTeam(iLoopTeam).getBuildingClassCount(iBuildingClass) > 0):
								iUNTeam = iLoopTeam
								if (iLoopTeam == gc.getPlayer(self.iActivePlayer).getTeam() or gc.getGame().isDebugMode() or gc.getTeam(gc.getPlayer(self.iActivePlayer).getTeam()).isHasMet(iLoopTeam)):
									bUnknown = false
								break
					dRequirements[iVoteSource] = (iBuildingClass, iUNTeam, bUnknown)

		aiVoteSources = dRequirements.keys()
		aiVoteSources.sort()
		for iVoteSource in aiVoteSources:
			iVoteBuildingClass, iUNTeam, bUnknown = dRequirements[iVoteSource]
			iBuildingType = gc.getBuildingClassInfo(iVoteBuildingClass).getDefaultBuildingIndex()
			szBuildingButton = gc.getBuildingInfo(iBuildingType).getButton()
			szSource = gc.getVoteSourceInfo(iVoteSource).getDescription()
			iRow = screen.appendTableRow(szTable)
			SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, szSource, szBuildingButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			if gc.getGame().isDiploVote(iVoteSource):
				if iUNTeam != -1:
					if bUnknown:
						szTeamName = localText.getText("TXT_KEY_TOPCIVS_UNKNOWN", ())
					else:
						szTeamName = gc.getTeam(iUNTeam).getName()
					szStatus = localText.getText("TXT_KEY_VICTORY_SCREEN_BUILT", (szTeamName, ))
				else:
					szStatus = self.TEXT_NOT_YET_BUILT
			else:
				szStatus = self.TEXT_NOT_ACTIVE_SHORT
			SASTextScale.setTableTextLabel(screen, szTable, 1, iRow, szStatus, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		self.drawTabs()

	def showVotingScreen(self):
		self.deleteAllWidgets()

		activePlayer = gc.getPlayer(self.iActivePlayer)
		iActiveTeam = activePlayer.getTeam()

		aiVoteBuildingClass = []
		for i in range(gc.getNumBuildingInfos()):
			for j in range(gc.getNumVoteSourceInfos()):
				if (gc.getBuildingInfo(i).getVoteSourceType() == j):
					iUNTeam = -1
					bUnknown = true
					for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
						if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
							if (gc.getTeam(iLoopTeam).getBuildingClassCount(gc.getBuildingInfo(i).getBuildingClassType()) > 0):
								iUNTeam = iLoopTeam
								if (iLoopTeam == iActiveTeam or gc.getGame().isDebugMode() or gc.getTeam(activePlayer.getTeam()).isHasMet(iLoopTeam)):
									bUnknown = false
								break

					#aiVoteBuildingClass.append((gc.getBuildingInfo(i).getBuildingClassType(), iUNTeam, bUnknown))
					aiVoteBuildingClass.append((j, gc.getBuildingInfo(i).getBuildingClassType(), iUNTeam, bUnknown)) # K-Mod

		if (len(aiVoteBuildingClass) == 0):
			self.drawVoteSourceInactiveFallback(self.TEXT_VOTE_SOURCE_NOT_ACTIVE, aiVoteBuildingClass)
			return

		screen = self.getScreen()

		screen.addPanel(self.getNextWidgetName(), "", "", False, False, self.X_AREA-10, self.Y_AREA-15, self.W_AREA+20, self.H_AREA+30, PanelStyles.PANEL_STYLE_BLUE50)
		szTable = self.getNextWidgetName()
		screen.addTableControlGFC(szTable, 2, self.X_AREA, self.Y_AREA, self.W_AREA, self.H_AREA, False, False, self.iLeaderButtonSize, self.iLeaderButtonSize, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSelect(szTable, False)		
		screen.setTableColumnHeader(szTable, 0, "", self.TABLE2_WIDTH_0)
		screen.setTableColumnHeader(szTable, 1, "", self.TABLE2_WIDTH_1)

		# for (iVoteBuildingClass, iUNTeam, bUnknown) in aiVoteBuildingClass:
			# iRow = screen.appendTableRow(szTable)
			# SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_ELECTION", (gc.getBuildingClassInfo(iVoteBuildingClass).getTextKey(), )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# if (iUNTeam != -1):
				# if bUnknown:
					# szName = localText.getText("TXT_KEY_TOPCIVS_UNKNOWN", ())
				# else:
					# szName = gc.getTeam(iUNTeam).getName()
				# SASTextScale.setTableTextLabel(screen, szTable, 1, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_BUILT", (szName, )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# else:
				# SASTextScale.setTableTextLabel(screen, szTable, 1, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_NOT_BUILT", ()), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		#for i in range(gc.getNumVoteSourceInfos()):
		bHasVotingRows = false
		for (i, iVoteBuildingClass, iUNTeam, bUnknown) in aiVoteBuildingClass: # K-Mod
			if gc.getGame().isDiploVote(i): # K-Mod (previously no condition)
				bHasVotingRows = true
				# K-Mod
				kVoteSource = gc.getVoteSourceInfo(i)
				iRow = screen.appendTableRow(szTable)

				# <!-- custom: add building icon to AP/UN header (claude opus 4.5) -->
				iBuildingType = gc.getBuildingClassInfo(iVoteBuildingClass).getDefaultBuildingIndex()
				szBuildingButton = gc.getBuildingInfo(iBuildingType).getButton()
				szHeaderText = sasFontTagTitle.bold + kVoteSource.getDescription().upper() + SAS_FONT_TAG_CLOSE
				SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, szHeaderText, szBuildingButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				if (gc.getGame().getVoteSourceReligion(i) != -1):
					# <!-- custom: add religion icon (claude opus 4.5) -->
					iReligion = gc.getGame().getVoteSourceReligion(i)
					szReligionButton = gc.getReligionInfo(iReligion).getButton()
					szReligionName = gc.getReligionInfo(iReligion).getDescription()
					SASTextScale.setTableTextLabel(screen, szTable, 1, iRow, szReligionName, szReligionButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				# K-Mod end

				#if (gc.getGame().canHaveSecretaryGeneral(i) and -1 != gc.getGame().getSecretaryGeneral(i)):
				if (gc.getGame().canHaveSecretaryGeneral(i) and -1 != gc.getGame().getSecretaryGeneral(i) and (gc.getGame().isDebugMode() or gc.getTeam(activePlayer.getTeam()).isHasMet(gc.getGame().getSecretaryGeneral(i)))):# K-Mod
					iRow = screen.appendTableRow(szTable)

					# <!-- custom: add star icon to Secretary General / AP Resident row (claude opus 4.5) -->
					szSecGenText = u"%c %s" % (self.iStarIcon, gc.getVoteSourceInfo(i).getSecretaryGeneralText())
					SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, szSecGenText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					# <!-- custom: add leader icon to secretary general name (claude opus 4.5) -->
					iSecGenPlayer = self.getPlayerOnTeam(gc.getGame().getSecretaryGeneral(i))
					szSecGenName = gc.getTeam(gc.getGame().getSecretaryGeneral(i)).getName()
					szSecGenButton = u""
					if iSecGenPlayer >= 0:
						szSecGenName = gc.getPlayer(iSecGenPlayer).getName()
						szSecGenButton = self.getLeaderButton(iSecGenPlayer)
					SASTextScale.setTableTextLabel(screen, szTable, 1, iRow, szSecGenName, szSecGenButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

				# K-Mod
				# vote timing
				if activePlayer.isVotingMember(i):
					iRow = screen.appendTableRow(szTable)
					iVoteTimer = gc.getGame().getVoteTimer(i)+1 # the +1 is so that when there will be a vote next turn, this reads "1" rather than "0".
					sString = localText.getText("TXT_KEY_BUG_VICTORY_TURNS_NEXT_VOTE", (iVoteTimer,) )
					#sString = u"<font=2b>" + sString + "</font>"
					sString = BugUtil.colorText(sString, "COLOR_YELLOW")
					SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					iSecGenTimer = gc.getGame().getSecretaryGeneralTimer(i)
					sString = localText.getText("TXT_KEY_BUG_VICTORY_VOTES_NEXT_ELECTION", (iSecGenTimer,) )
					#sString = u"<font=2b>" + sString + "</font>"
					sString = BugUtil.colorText(sString, "COLOR_YELLOW")
					SASTextScale.setTableTextLabel(screen, szTable, 1, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				# K-Mod end

				for iLoop in range(gc.getNumVoteInfos()):
					if gc.getGame().countPossibleVote(iLoop, i) > 0:
						info = gc.getVoteInfo(iLoop)
						# advc.178: Clauses for diplo victory added
						if gc.getGame().isChooseElection(iLoop) and (gc.getGame().isDiploVictoryValid() or not info.isVictory()):
							iRow = screen.appendTableRow(szTable)

							# <!-- custom: add bullet icon to resolution rows (claude opus 4.5) -->
							szResText = u"%c %s" % (self.iBulletIcon, info.getDescription())
							SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, szResText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

							if gc.getGame().isVotePassed(iLoop):
								SASTextScale.setTableTextLabel(screen, szTable, 1, iRow, self.TEXT_POPUP_PASSED, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
							else:
								# <!-- custom: use a dedicated key without the optional leading placeholder because this row only shows "(Requires X of Y total votes)" and should not depend on trimming localized text at runtime. (GPT-5.3-Codex) -->
								szVoteReqText = localText.getText("TXT_KEY_SAS_POPUP_ELECTION_OPTION_REQUIRES_ONLY", (gc.getGame().getVoteRequired(iLoop, i), gc.getGame().countPossibleVote(iLoop, i)))
								SASTextScale.setTableTextLabel(screen, szTable, 1, iRow, szVoteReqText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				iRow = screen.appendTableRow(szTable) # empty row between vote sources. (K-Mod)
		# Remove the final empty row (K-Mod)
		if screen.getTableNumRows(szTable) > 0:
			screen.setTableNumRows(szTable, screen.getTableNumRows(szTable)-1)
		if not bHasVotingRows:
			self.deleteAllWidgets()
			self.drawVoteSourceInactiveFallback(self.TEXT_VOTE_SOURCE_NOT_ACTIVE, aiVoteBuildingClass)
			return
		#
		# <advc.703> Add info from Members tab
		if not gc.getGame().isOption(GameOptionTypes.GAMEOPTION_RISE_FALL):
			self.drawTabs()
			return
		screen.appendTableRow(szTable) # Empty row as separator
		self.appendMemberRows(screen, szTable)
		# </advc.703>
		self.drawTabs()

# BUG Additions Start
	def showMembersScreen(self):
		iRelVote, iRelVoteIdx, iUNVote, iUNVoteIdx  = self.getVoteAvailable()
		if (iRelVote == -1 and iUNVote == -1):
			self.deleteAllWidgets()
			self.drawVoteSourceInactiveFallback(self.TEXT_MEMBERS_NO_VOTING_YET, [])
			return

		# <!-- custom: K-Mod imported this advanced BUG poll disabled and without a visible options checkbox.
		# An exact DLL prediction was runtime-tested but rejected because it would expose hidden AI vote state while retaining statistical-poll labels; keep the supported plain Members view as the default. See KI#327. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if AdvisorOpt.isMembers():
			if  iRelVote == -1:
				self.VoteBody = 2 # AP Not active
			elif iUNVote == -1:
				self.VoteBody = 1 # UN Not active

			if self.VoteBody == 1:
				iVoteBody = iRelVote
				iVoteIdx = iRelVoteIdx
			else:
				iVoteBody = iUNVote
				iVoteIdx = iUNVoteIdx

			self.showMembersScreen_BUG(iRelVote, iUNVote, iVoteBody, iVoteIdx)
		else:
			self.showMembersScreen_NonBUG()

		self.drawTabs()

	def getVoteAvailable(self):

		iRelVote = -1
		iRelVoteIdx = -1
		iUNVote = -1
		iUNVoteIdx = -1

		for i in range(gc.getNumVoteSourceInfos()):
			if gc.getGame().isDiploVote(i):
				if (gc.getGame().getVoteSourceReligion(i) != -1):
					iRelVote = i
				else:
					iUNVote = i

			if (gc.getGame().canHaveSecretaryGeneral(i) and gc.getGame().getSecretaryGeneral(i) != -1):
				for j in range(gc.getNumVoteInfos()):
					if gc.getVoteInfo(j).isVoteSourceType(i):
						if gc.getVoteInfo(j).isSecretaryGeneral():
							if (gc.getGame().getVoteSourceReligion(i) != -1):
								iRelVoteIdx = j
							else:
								iUNVoteIdx = j

							break

		BugUtil.debug("CvVictoryScreen: Rel Vote %i, UN Vote %i, Rel Vote Idx %i, UN Vote Idx %i", iRelVote, iRelVoteIdx, iUNVote, iUNVoteIdx)

		return iRelVote, iRelVoteIdx, iUNVote, iUNVoteIdx

	def showMembersScreen_BUG(self, iRelVote, iUNVote, iActiveVote, iVoteIdx):
		self.deleteAllWidgets()

		if (iRelVote == -1 and iUNVote == -1):
			return  # neither AP or UN are active

		activePlayer = gc.getPlayer(self.iActivePlayer)
		iActiveTeam = activePlayer.getTeam()

		screen = self.getScreen()

		screen.addPanel(self.getNextWidgetName(), "", "", False, False, self.X_AREA-10, self.Y_AREA-15, self.W_AREA+20, self.H_AREA+30, PanelStyles.PANEL_STYLE_BLUE50)

		# set up the header row
		szHeading = self.getNextWidgetName()
		screen.addTableControlGFC(szHeading, 3, self.X_AREA, self.Y_AREA, self.W_AREA, 30, False, False, self.iLeaderButtonSize, self.iLeaderButtonSize, TableStyles.TABLE_STYLE_STANDARD)
		screen.setTableColumnHeader(szHeading, 0, "", self.TABLE3_WIDTH_0)
		screen.setTableColumnHeader(szHeading, 1, "", self.TABLE3_WIDTH_1 + self.TABLE3_WIDTH_2)
		screen.setTableColumnHeader(szHeading, 2, "", self.TABLE3_WIDTH_3 + self.TABLE3_WIDTH_4)
		iRow = screen.appendTableRow(szHeading)

		# heading
		kVoteSource = gc.getVoteSourceInfo(iActiveVote)
		sTableHeader = sasFontTagTitle.bold + kVoteSource.getDescription().upper() + SAS_FONT_TAG_CLOSE
		if (gc.getGame().getVoteSourceReligion(iActiveVote) != -1):
			sTableHeader += " (" + gc.getReligionInfo(gc.getGame().getVoteSourceReligion(iActiveVote)).getDescription() + ")"

		SASTextScale.setTableTextLabel(screen, szHeading, 0, iRow, sTableHeader, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		# determine the two candidates, add to header
		iCandTeam1 = -1
		iCandTeam2 = -1
		for j in range(gc.getMAX_TEAMS()):
			BugUtil.debug("CvVictoryScreen: Team %i", j)

			if (gc.getTeam(j).isAlive() and gc.getGame().isTeamVoteEligible(j, iActiveVote)):
				BugUtil.debug("CvVictoryScreen: Team %i, %s <- vote eligible ", j, gc.getTeam(j).getName())
				if iCandTeam1 == -1:
					iCandTeam1 = j
				else:
					iCandTeam2 = j

		# get the first player for each team
		# going to use that to calculation attitude - too hard to calc attitude for team
		iCandPlayer1 = self.getPlayerOnTeam(iCandTeam1)
		iCandPlayer2 = self.getPlayerOnTeam(iCandTeam2)

		# candidate known returns -1 if there is no candidate, 0 if not known or 1 if known
		iCand1Known, sCand1Name = self.getCandStatusName(iCandTeam1)
		iCand2Known, sCand2Name = self.getCandStatusName(iCandTeam2)

		SASTextScale.setTableTextLabel(screen, szHeading, 1, iRow, sCand1Name, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
		SASTextScale.setTableTextLabel(screen, szHeading, 2, iRow, sCand2Name, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

		# set up the member table
		szTable = self.getNextWidgetName()
		screen.addTableControlGFC(szTable, 6, self.X_AREA, self.Y_AREA + 30, self.W_AREA, self.H_AREA-20-30, False, False, self.iLeaderButtonSize, self.iLeaderButtonSize, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSelect(szTable, False)
		screen.setTableColumnHeader(szTable, 0, "", self.TABLE3_WIDTH_0)
		screen.setTableColumnHeader(szTable, 1, "", self.TABLE3_WIDTH_1)
		screen.setTableColumnHeader(szTable, 2, "", self.TABLE3_WIDTH_2)
		screen.setTableColumnHeader(szTable, 3, "", self.TABLE3_WIDTH_3)
		screen.setTableColumnHeader(szTable, 4, "", self.TABLE3_WIDTH_4)
		screen.setTableColumnHeader(szTable, 5, "", self.TABLE3_WIDTH_5)
		iRow = screen.appendTableRow(szTable)

		# set up the vote selection texts
		iX = self.X_EXIT
		sText = gc.getVoteSourceInfo(iActiveVote).getSecretaryGeneralText()
		if self.VoteType == 1:
			sText = BugUtil.colorText(sText, "COLOR_YELLOW")
		screen.setText(self.Vote_Pope_ID, "", sText, CvUtil.FONT_RIGHT_JUSTIFY, iX, self.Vote_Y, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		iX -= 10 + CyInterface().determineWidth(sText)
		sText = self.TEXT_DIPLOMATIC
		if self.VoteType == 2:
			sText = BugUtil.colorText(sText, "COLOR_YELLOW")
		screen.setText(self.Vote_DipVic_ID, "", sText, CvUtil.FONT_RIGHT_JUSTIFY, iX, self.Vote_Y, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		if (iRelVote != -1 and iUNVote != -1):  # both AP and UN are active
			iX = self.Vote_X
			sText = gc.getVoteSourceInfo(iRelVote).getDescription()
			if iActiveVote == iRelVote:
				sText = BugUtil.colorText(sText, "COLOR_YELLOW")
			screen.setText(self.Vote_AP_ID, "", sText, CvUtil.FONT_LEFT_JUSTIFY, iX, self.Vote_Y, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

			iX += 10 + CyInterface().determineWidth(sText)
			sText = gc.getVoteSourceInfo(iUNVote).getDescription()
			if iActiveVote == iUNVote:
				sText = BugUtil.colorText(sText, "COLOR_YELLOW")
			screen.setText(self.Vote_UN_ID, "", sText, CvUtil.FONT_LEFT_JUSTIFY, iX, self.Vote_Y, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		else:
			screen.hide(self.Vote_AP_ID)
			screen.hide(self.Vote_UN_ID)

		# initialize the candidate votes and total vote counter
		iVoteTotal = [0] * 2
		iVoteCand = [0] * 2

		lMembers = []
		iAPUNTeam = self.getAP_UN_OwnerTeam()

		for j in range(gc.getMAX_PLAYERS()):
			pPlayer = gc.getPlayer(j)
			if (pPlayer.isAlive() and not pPlayer.isBarbarian()):
				iPlayer = j
				lPlayerName = pPlayer.getName()
				lPlayerVotes = 10000 - pPlayer.getVotes(iVoteIdx, iActiveVote)   # so that it sorts from most votes to least

				# <!-- custom: add icons to membership labels (claude opus 4.5) -->
				if (gc.getGame().canHaveSecretaryGeneral(iActiveVote) and iAPUNTeam == pPlayer.getTeam() and gc.getGame().getSecretaryGeneral(iActiveVote) == -1):
					lPlayerStatus = 0
					lPlayerLabel = u"%c %s" % (self.iStarIcon, gc.getVoteSourceInfo(iActiveVote).getSecretaryGeneralText())
				elif (gc.getGame().canHaveSecretaryGeneral(iActiveVote) and gc.getGame().getSecretaryGeneral(iActiveVote) == pPlayer.getTeam()):
					lPlayerStatus = 1
					lPlayerLabel = u"%c %s" % (self.iStarIcon, gc.getVoteSourceInfo(iActiveVote).getSecretaryGeneralText())
				elif (pPlayer.isFullMember(iActiveVote)):
					lPlayerStatus = 2
					lPlayerLabel = self.szFullMemberText
				elif (pPlayer.isVotingMember(iActiveVote)):
					lPlayerStatus = 3
					lPlayerLabel = self.szVotingMemberText
				else:
					lPlayerStatus = 4
					lPlayerLabel = self.TEXT_NON_VOTING_MEMBER

				lMembers.append([lPlayerStatus, lPlayerVotes, iPlayer, lPlayerLabel])

		lMembers.sort()

		for lMember in lMembers:
			lMemberStatus = lMember[0]
			lMemberVotes = 10000 - lMember[1]
			iMember = lMember[2]
			lMemberLabel = lMember[3]

			# player name
			bKnown, szPlayerText = self.getPlayerStatusName(iMember)

			# <!-- custom: add leader icon to player name (claude opus 4.5) -->
			szPlayerButton = u""
			if bKnown:
				szPlayerText = gc.getPlayer(iMember).getName()
				szPlayerButton = self.getLeaderButton(iMember)

			if (lMemberVotes > 0 and bKnown):
				szPlayerText += localText.getText("TXT_KEY_VICTORY_SCREEN_PLAYER_VOTES", (lMemberVotes, iActiveVote), )

			iRow = screen.appendTableRow(szTable)
			SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, szPlayerText, szPlayerButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			if iMember != self.iActivePlayer:
				# player attitude to candidate #1
				szText = AttitudeUtil.getAttitudeText (iMember, iCandPlayer1, True, True, False, False, False) # advc.152: To match the new signature of getAttitudeText
				if (szText is not None and iCand1Known == 1 and bKnown):
					SASTextScale.setTableTextLabel(screen, szTable, 1, iRow, szText, "", WidgetTypes.WIDGET_LEADERHEAD, iMember, iCandPlayer1, CvUtil.FONT_CENTER_JUSTIFY)

				# player attitude to candidate #2
				szText = AttitudeUtil.getAttitudeText (iMember, iCandPlayer2, True, True, False, False, False) # advc.152: To match the new signature of getAttitudeText
				if (szText is not None and iCand2Known == 1 and bKnown):
					SASTextScale.setTableTextLabel(screen, szTable, 3, iRow, szText, "", WidgetTypes.WIDGET_LEADERHEAD, iMember, iCandPlayer2, CvUtil.FONT_CENTER_JUSTIFY)

			iVote = self.getVotesForWhichCandidate(iMember, iCandTeam1, iCandTeam2, self.VoteType)
			iVote_Column = -1

			if iVote != -1:
				sVote = str(lMemberVotes)
				iVoteTotal[iVote - 1] += lMemberVotes

				# number of votes for Candidate #1
				if (iVote == 1 and lMemberVotes > 0 and iCand1Known == 1):
					SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, sVote, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

				# number of votes for Candidate #2
				if (iVote == 2 and lMemberVotes > 0 and iCand2Known == 1):
					SASTextScale.setTableTextLabel(screen, szTable, 4, iRow, sVote, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

			# player status
			if bKnown:
				SASTextScale.setTableTextLabel(screen, szTable, 5, iRow, lMemberLabel, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)

			# store the candidates own votes
			if (iCandTeam1 == gc.getPlayer(iMember).getTeam() and iCand1Known == 1):
				iVoteCand[0] = lMemberVotes
			if (iCandTeam2 == gc.getPlayer(iMember).getTeam() and iCand2Known == 1):
				iVoteCand[1] = lMemberVotes

		# calculate the maximum number of votes
		iMaxVotes = 0
		for iLoop in range(gc.getNumVoteInfos()):
			if gc.getGame().countPossibleVote(iLoop, iActiveVote) > 0:
				iMaxVotes = gc.getGame().countPossibleVote(iLoop, iActiveVote)
				break

		iRow = screen.appendTableRow(szTable)
		iVoteReq = self.getVoteReq(iActiveVote, self.VoteType)
		sVoteReq = "%i" % (iVoteReq)
		sString = sasFontTagLabel.bold + localText.getText("TXT_KEY_BUG_VICTORY_VOTES_TOTAL", ()) + SAS_FONT_TAG_CLOSE + u" "
		if (iCand1Known != 0 and iCand2Known != 0):
			sString +=  localText.getText("TXT_KEY_BUG_VICTORY_VOTES_REQUIRED", (sVoteReq,))
		SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		if iCand1Known == 1:
			# color code the totals
			sVoteTotal = str(iVoteTotal[0])
			iColor = self.getVoteTotalColor(iVoteReq, iVoteTotal[0], iVoteCand[0], iVoteTotal[0] > iVoteTotal[1], self.VoteType == 2)
			if iColor != -1:
				sVoteTotal = localText.changeTextColor(sVoteTotal, iColor)
			SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, sVoteTotal, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

		if iCand2Known == 1:
			sVoteTotal = str(iVoteTotal[1])
			iColor = self.getVoteTotalColor(iVoteReq, iVoteTotal[1], iVoteCand[1], iVoteTotal[1] > iVoteTotal[0], self.VoteType == 2)
			if iColor != -1:
				sVoteTotal = localText.changeTextColor(sVoteTotal, iColor)
			SASTextScale.setTableTextLabel(screen, szTable, 4, iRow, sVoteTotal, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

		# add a blank row
		iRow = screen.appendTableRow(szTable)

		# SecGen vote prediction
		if iVoteTotal[0] > iVoteTotal[1]:
			iWinner = 0
			sWin = sCand1Name
		else:
			iWinner = 1
			sWin = sCand2Name
		iLoser = 1 - iWinner

		fVotePercent = 100.0 * iVoteTotal[iWinner] / iMaxVotes
		fMargin = 100.0 * (iVoteTotal[iWinner] - iVoteTotal[iLoser]) / iMaxVotes

		if self.VoteType == 1:
			sSecGen = gc.getVoteSourceInfo(iActiveVote).getSecretaryGeneralText()
		else:
			sSecGen = self.TEXT_DIPLOMATIC

		# display SecGen vote prediction
		if (iCand1Known != 0 and iCand2Known != 0):
			sString = sSecGen + ":"
			iRow = screen.appendTableRow(szTable)
			SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			sString = "     " + localText.getText("TXT_KEY_BUG_VICTORY_BUG_POLL_RESULT", (sWin, self.formatPercent(fVotePercent), self.formatPercent(fMargin)))
			iRow = screen.appendTableRow(szTable)
			SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			# BUG Poll statistical error
			iRandError = 3.5 + gc.getASyncRand().get(10, "Election Results Statistical Error") / 10.0
			sString = localText.getText("TXT_KEY_BUG_VICTORY_BUG_POLL_ERROR", (self.formatPercent(iRandError), ))
			iRow = screen.appendTableRow(szTable)
			SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		iRow = screen.appendTableRow(szTable)

		# add info about vote timing
		iRow = screen.appendTableRow(szTable)
		iVoteTimer = gc.getGame().getVoteTimer(iActiveVote)
		sString = localText.getText("TXT_KEY_BUG_VICTORY_TURNS_NEXT_VOTE", (iVoteTimer,) )
		sString = sasFontTagLabel + sString + SAS_FONT_TAG_CLOSE
		SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		iRow = screen.appendTableRow(szTable)
		iSecGenTimer = gc.getGame().getSecretaryGeneralTimer(iActiveVote)
		sString = localText.getText("TXT_KEY_BUG_VICTORY_VOTES_NEXT_ELECTION", (iSecGenTimer,) )
		sString = sasFontTagLabel + sString + SAS_FONT_TAG_CLOSE
		SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	# <advc.703> Now shared by showVotes and showMembersScreen_NonBUG
	# (I don't think the BUG version can be enabled in K-Mod)
	def appendMemberRows(self, screen, szTable):
		activePlayer = gc.getPlayer(self.iActivePlayer)
		iActiveTeam = activePlayer.getTeam()
		for i in range(gc.getNumVoteSourceInfos()):
			if gc.getGame().isDiploVote(i):
				kVoteSource = gc.getVoteSourceInfo(i)
				iRow = screen.appendTableRow(szTable)

				# <!-- custom: add building icon to AP/UN header (claude opus 4.5) -->
				szHeaderText = sasFontTagTitle.bold + kVoteSource.getDescription().upper() + SAS_FONT_TAG_CLOSE
				szBuildingButton = u""
				for iBuilding in range(gc.getNumBuildingInfos()):
					if gc.getBuildingInfo(iBuilding).getVoteSourceType() == i:
						szBuildingButton = gc.getBuildingInfo(iBuilding).getButton()
						szHeaderText = sasFontTagTitle.bold + kVoteSource.getDescription().upper() + SAS_FONT_TAG_CLOSE
						break
				SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, szHeaderText, szBuildingButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				if (gc.getGame().getVoteSourceReligion(i) != -1):
					# <!-- custom: add religion icon (claude opus 4.5) -->
					iReligion = gc.getGame().getVoteSourceReligion(i)
					szReligionButton = gc.getReligionInfo(iReligion).getButton()
					szReligionName = gc.getReligionInfo(iReligion).getDescription()
					SASTextScale.setTableTextLabel(screen, szTable, 1, iRow, szReligionName, szReligionButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

				iSecretaryGeneralVote = -1
				if (gc.getGame().canHaveSecretaryGeneral(i) and -1 != gc.getGame().getSecretaryGeneral(i)):
					for j in range(gc.getNumVoteInfos()):
						print(j)
						if gc.getVoteInfo(j).isVoteSourceType(i):
							print("votesource")
							if gc.getVoteInfo(j).isSecretaryGeneral():
								print("secgen")
								iSecretaryGeneralVote = j
								break

				print(iSecretaryGeneralVote)
				for j in range(gc.getMAX_PLAYERS()):
					if gc.getPlayer(j).isAlive() and not gc.getPlayer(j).isBarbarian() and gc.getTeam(iActiveTeam).isHasMet(gc.getPlayer(j).getTeam()):
						# <!-- custom: add leader icon to player name (claude opus 4.5) -->
						szPlayerText = gc.getPlayer(j).getName()
						szPlayerButton = self.getLeaderButton(j)
						if (-1 != iSecretaryGeneralVote):
							szPlayerText += localText.getText("TXT_KEY_VICTORY_SCREEN_PLAYER_VOTES", (gc.getPlayer(j).getVotes(iSecretaryGeneralVote, i), ))

						# <!-- custom: add icons to membership status (claude opus 4.5) -->
						if (gc.getGame().canHaveSecretaryGeneral(i) and gc.getGame().getSecretaryGeneral(i) == gc.getPlayer(j).getTeam()):
							iRow = screen.appendTableRow(szTable)
							SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, szPlayerText, szPlayerButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
							szStatusText = u"%c %s" % (self.iStarIcon, gc.getVoteSourceInfo(i).getSecretaryGeneralText())
							SASTextScale.setTableTextLabel(screen, szTable, 1, iRow, szStatusText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						elif (gc.getPlayer(j).isFullMember(i)):
							iRow = screen.appendTableRow(szTable)
							SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, szPlayerText, szPlayerButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
							SASTextScale.setTableTextLabel(screen, szTable, 1, iRow, self.szFullMemberText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						elif (gc.getPlayer(j).isVotingMember(i)):
							iRow = screen.appendTableRow(szTable)
							SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, szPlayerText, szPlayerButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
							SASTextScale.setTableTextLabel(screen, szTable, 1, iRow, self.szVotingMemberText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

				iRow = screen.appendTableRow(szTable)
		# Remove the final empty row (K-Mod)
		if screen.getTableNumRows(szTable) > 0:
			screen.setTableNumRows(szTable, screen.getTableNumRows(szTable)-1)
	# </advc.703>

	def showMembersScreen_NonBUG(self):
		self.deleteAllWidgets()

		activePlayer = gc.getPlayer(self.iActivePlayer)
		iActiveTeam = activePlayer.getTeam()

		screen = self.getScreen()

		screen.addPanel(self.getNextWidgetName(), "", "", False, False, self.X_AREA-10, self.Y_AREA-15, self.W_AREA+20, self.H_AREA+30, PanelStyles.PANEL_STYLE_BLUE50)
		szTable = self.getNextWidgetName()
		screen.addTableControlGFC(szTable, 2, self.X_AREA, self.Y_AREA, self.W_AREA, self.H_AREA, False, False, self.iLeaderButtonSize, self.iLeaderButtonSize, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSelect(szTable, False)
		screen.setTableColumnHeader(szTable, 0, "", self.TABLE2_WIDTH_0)
		screen.setTableColumnHeader(szTable, 1, "", self.TABLE2_WIDTH_1)
		# advc.703: Code moved into subroutine
		self.appendMemberRows(screen, szTable)

	# <advc.703>
	def showRiseFall(self):
		self.deleteAllWidgets()	
		screen = self.getScreen()
		screen.addPanel(self.getNextWidgetName(), "", "", False, False, self.X_AREA-10, self.Y_AREA-15, self.W_AREA+20, self.H_AREA+30, PanelStyles.PANEL_STYLE_BLUE50)
		szTable = self.getNextWidgetName()
		screen.addTableControlGFC(szTable, 5, self.X_AREA, self.Y_AREA, self.W_AREA, self.H_AREA, False, False, 32,32, TableStyles.TABLE_STYLE_STANDARD)
		screen.setTableColumnHeader(szTable, 0, "", self.RF_TABLEW_0)
		screen.setTableColumnHeader(szTable, 1, "", self.RF_TABLEW_1)
		screen.setTableColumnHeader(szTable, 2, "", self.RF_TABLEW_2)
		screen.setTableColumnHeader(szTable, 3, "", self.RF_TABLEW_3)
		screen.setTableColumnHeader(szTable, 4, "", self.RF_TABLEW_4)
		screen.appendTableRow(szTable)
		iNumRows = screen.getTableNumRows(szTable)
		iTitleRow = iNumRows - 1
		szHeading = sasFontTagTitle.bold + localText.getText("TXT_KEY_RF_CHAPTER_PLURAL", ()) + SAS_FONT_TAG_CLOSE
		SASTextScale.setTableTextLabel(screen, szTable, 0, iTitleRow, szHeading, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		partialScore = 0
		allScored = true
		extGame = gc.getGame().getGameState() == GameStateTypes.GAMESTATE_EXTENDED
		currentTurn = gc.getGame().getGameTurn()
		maxChapters = gc.getGame().getMaxChapters()
		for i in range(maxChapters):
			startTurn = gc.getGame().getChapterStart(i)
			endTurn = gc.getGame().getChapterEnd(i)
			# Endless chapter for extended play
			extChapter = (endTurn < 0 and gc.getGame().getMaxTurns() > 0)
			ongoing = (currentTurn >= startTurn and (currentTurn <= endTurn or extChapter))
			if ongoing and not extChapter:
				allScored = false
			iRow = screen.appendTableRow(szTable)
			# [0] Chapter index
			s = str(i + 1)
			if ongoing:
				s = self.highlight(s)
			s = "  " + s # Don't want this to align exactly with the heading
			SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, s, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# [1] Civ description
			chapterCiv = gc.getGame().getChapterCiv(i)
			activeCiv = gc.getGame().getActivePlayer()
			s = "?"
			if currentTurn >= startTurn and chapterCiv >= 0:
				s = gc.getPlayer(chapterCiv).getCivilizationShortDescription(0)
				if ongoing and not extChapter:
					s = self.highlight(s)
			SASTextScale.setTableTextLabel(screen, szTable, 1, iRow, s, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# [2] Start/end turn
			s = localText.getText("TXT_KEY_RF_CHAPTER_INTERVAL", (startTurn, endTurn))
			if endTurn < 0:
				s = localText.getText("TXT_KEY_RF_TURN", (startTurn,))
			if ongoing and not extChapter:
				s = self.highlight(s)
			if endTurn >= 0: # Don't highlight this part
				s += " (" + BugUtil.getDisplayYear(gc.getGame().getTurnYear(endTurn)) + ")"
			SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, s, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# [3] Points
			chapterScore = gc.getGame().getChapterScore(i)
			scoreTurn = gc.getGame().getChapterScoreTurn(i)
			s = localText.getText("TXT_KEY_RF_CHAPTER_PTS", (chapterScore,))
			# Highlight the part in parentheses when ongoing, otherwise the pts.
			if currentTurn <= endTurn:
				s += " ("
				remaining = endTurn - currentTurn + 1
				if remaining > 1:
					s += self.highlight(localText.getText("TXT_KEY_RF_CHAPTER_REMAIN", (remaining,)))
				else:
					s += self.highlight(localText.getText("TXT_KEY_RF_CHAPTER_FINAL", ()))
				s += ")"
			elif currentTurn >= scoreTurn:
				# Doesn't actually imply that the chapter's already scored b/c
				# the chapter civ could come later in the turn order than the
				# current civ; but I'm too lazy to export isScored to Python.
				s = self.highlight(s)
			elif chapterCiv < 0 or not gc.getTeam(gc.getPlayer(chapterCiv).getTeam()).isHasMet(gc.getPlayer(activeCiv).getTeam()):
				s += " (" + localText.getText("TXT_KEY_RF_CHAPTER_LAST_KNOWN", (endTurn,)) + ")"
			if currentTurn < startTurn:
				s = "?"
			elif extChapter:
				s = "-"
			else:
				partialScore += chapterScore
			SASTextScale.setTableTextLabel(screen, szTable, 3, iRow, s, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# [4] Scoring turn
			s = localText.getText("TXT_KEY_RF_CHAPTER_SCORE_TURN", (scoreTurn,))
			if i == maxChapters - 1 and scoreTurn < 0:
				s = localText.getText("TXT_KEY_RF_CHAPTER_SCORE_END", ())
			elif currentTurn >= scoreTurn:
				s = self.highlight(s)
			if not extGame or currentTurn >= scoreTurn:
				SASTextScale.setTableTextLabel(screen, szTable, 4, iRow, s, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			if extChapter:
				break # The remaining chapters will never happen
		iRow = screen.appendTableRow(szTable)
		s1 = localText.getText("TXT_KEY_RF_PARTIAL_SUM", ())
		s2 = str(partialScore)
		if allScored: # Only relevant when the game is over
			s1 = self.highlight(localText.getText("TXT_KEY_RF_TOTAL", ()))
			s2 = self.highlight(s2)
		SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, s1, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		SASTextScale.setTableTextLabel(screen, szTable, 3, iRow, s2, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# The panels in the lower half are based on CvReligionScreen
		# By default, let them take up the bottom half of the screen. But if there isn't enough room for the chapters in the upper part ...
		iExcessChapters = min(maxChapters - 10, 3)
		if iExcessChapters > 0:
			iHeightMinus = iExcessChapters * 30 # not sure if 30 per chapter row is exactly right
			self.Y_RF_AREA += iHeightMinus
			self.H_RF_AREA -= iHeightMinus
		screen.addPanel(self.AREA1_ID, "", "", True, True, self.X_RF1_AREA, self.Y_RF_AREA, self.W_RF_AREA, self.H_RF_AREA, PanelStyles.PANEL_STYLE_BLUE50)
		screen.addPanel(self.AREA2_ID, "", "", True, True, self.X_RF2_AREA, self.Y_RF_AREA, self.W_RF_AREA, self.H_RF_AREA, PanelStyles.PANEL_STYLE_BLUE50)
		chapterScoreText = sasFontTagLabel.bold
		chapterScoreText += localText.getText("TXT_KEY_RF_CHAPTER_BREAKDOWN_H", ())
		chapterScoreText += u"</font>"
		chapterScoreText += sasFontTagLabel + "\n\n"
		if not extGame: # No breakdown after game end
			chapterScoreText += gc.getGame().chapterScoreBreakdown()
		chapterScoreText += u"</font>"
		riseScoreText = sasFontTagLabel.bold
		if extGame >= 0:
			riseScoreText += localText.getText("TXT_KEY_RF_RISE_FINAL_H", ())
		else:
			riseScoreText += localText.getText("TXT_KEY_RF_RISE_BREAKDOWN_H", ())
		riseScoreText += u"</font>"
		riseScoreText += sasFontTagLabel + "\n\n"
		riseScoreText += gc.getGame().riseScoreBreakdown()
		riseScoreText += u"</font>"
		screen.addMultilineText("Child" + self.AREA1_ID, chapterScoreText, self.X_RF1_AREA+20, self.Y_RF_AREA+20, self.W_RF_AREA-20, self.H_RF_AREA-20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.addMultilineText("Child" + self.AREA2_ID, riseScoreText, self.X_RF2_AREA+10, self.Y_RF_AREA+20, self.W_RF_AREA-10, self.H_RF_AREA-20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		self.drawTabs()

	def highlight(self, s):
		return sasFontTagLabel.bold + s + SAS_FONT_TAG_CLOSE
	# </advc.703>

	def formatPercent(self, f):
		return "%.1f%%" % f

	def getVoteReq(self, i, iVote):
		iMaxVotes = 0
		iMinVotes = 999999
		for iLoop in range(gc.getNumVoteInfos()):
			iVoteReq = gc.getGame().getVoteRequired(iLoop, i)
			if iVoteReq > 0:
				if iVoteReq > iMaxVotes:
					iMaxVotes = iVoteReq
				if iVoteReq < iMinVotes:
					iMinVotes = iVoteReq

		if iVote == 1:
			return iMinVotes
		else:
			return iMaxVotes

	def getCandStatusName(self, iCand):
		# iCand is a team
		if iCand == -1: # there is no candidate
			return -1, "-"

		activePlayer = gc.getPlayer(self.iActivePlayer)
		iActiveTeam = activePlayer.getTeam()

		if iActiveTeam == iCand:
			return 1, gc.getTeam(iCand).getName()

		if gc.getTeam(iActiveTeam).isHasMet(iCand):
			return 1, gc.getTeam(iCand).getName()
		else:
			return 0, localText.getText("TXT_KEY_TOPCIVS_UNKNOWN", ())

	def getPlayerStatusName(self, iPlayer):
		if iPlayer == -1: # there is no player
			return False, "-"

		pPlayer = gc.getPlayer(iPlayer)
		iPlayerTeam = pPlayer.getTeam()
		activePlayer = gc.getPlayer(self.iActivePlayer)
		iActiveTeam = activePlayer.getTeam()

		if iActiveTeam == iPlayerTeam:
			return True, pPlayer.getName()

		if gc.getTeam(iActiveTeam).isHasMet(iPlayerTeam):
			return True, pPlayer.getName()
		else:
			return False, localText.getText("TXT_KEY_TOPCIVS_UNKNOWN", ())

	def getVoteTotalColor(self, iVoteReq, iVoteTotal, iVoteCand, bWinner, bVictoryVote):
		print("%i %i %i" % (iVoteReq, iVoteTotal, iVoteCand))
		if not bWinner:
			return -1
		if (iVoteCand > iVoteReq and bVictoryVote):
			return ColorUtil.keyToType("COLOR_RED")
		if iVoteTotal > iVoteReq:
			return ColorUtil.keyToType("COLOR_GREEN")
		return -1

	def showGameSettingsScreen(self):
		self.deleteAllWidgets()	
		screen = self.getScreen()

		# <!-- custom: Victory-condition tabs support debug/vassal perspective, but Game Settings describe the actual player's game setup; keep this tab anchored to the real active player. (GPT-5.5) -->
		iSettingsPlayer = gc.getGame().getActivePlayer()
		activePlayer = gc.getPlayer(iSettingsPlayer)

		szSettingsPanel = self.getNextWidgetName()
		screen.addPanel(szSettingsPanel, "", "", True, True, self.SETTINGS_PANEL_X1, self.SETTINGS_PANEL_Y - 10, self.SETTINGS_PANEL_WIDTH, self.SETTINGS_PANEL_HEIGHT, PanelStyles.PANEL_STYLE_MAIN)
		szSettingsHeader = self.getNextWidgetName()
		screen.setLabel(szSettingsHeader, "Background", self.LABEL_SETTINGS_HEADER, CvUtil.FONT_CENTER_JUSTIFY, self.SETTINGS_PANEL_X1 + self.SETTINGS_PANEL_WIDTH / 2, self.SETTINGS_PANEL_Y + 2, self.Z_CONTROLS, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		placeAdvisorLegendLink(self, "CONCEPT_SAS_VICTORY_SCREEN_SETTINGS_LEGEND", self.W_SCREEN - 12, self.Y_TITLE)
		szSettingsTable = self.getNextWidgetName()
		screen.addListBoxGFC(szSettingsTable, "", self.SETTINGS_PANEL_X1 + self.MARGIN, self.SETTINGS_PANEL_Y + self.MARGIN, self.SETTINGS_PANEL_WIDTH - 2*self.MARGIN, self.SETTINGS_PANEL_HEIGHT - 2*self.MARGIN, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(szSettingsTable, False)

# K-Mod, disabled the following		
#		if showHOFSettingChecks and BugPath.isMac():
#			failedHOFChecks = True
#			showHOFSettingChecks = False

#			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, self.BuffyWarningMac, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		# <!-- custom: add leader icon to active player name -->
		szActivePlayerName = u"%s %s" % (self.getLeaderIconTag(iSettingsPlayer), localText.getText("TXT_KEY_LEADER_CIV_DESCRIPTION", (activePlayer.getNameKey(), activePlayer.getCivilizationShortDescriptionKey())))
		SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, szActivePlayerName, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, u"     " + TraitUtil.getLeaderTraitsWithIcons(activePlayer.getLeaderType(), activePlayer.getCivilizationType()), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		g = gc.getGame() # advc
		m = gc.getMap() # advc
		# <advc.190c>
		bCivLeaderSetupKnown = g.isCivLeaderSetupKnown()
		bScenario = g.isScenario()
		bAllRandom = False
		bNoneRandom = False
		bActiveCivRandom = False
		bActiveLeaderRandom = False
		if bCivLeaderSetupKnown:
			bAllRandom = True
			bNoneRandom = True
			for iLoopPlayer in range(gc.getMAX_CIV_PLAYERS()):
				p = gc.getPlayer(iLoopPlayer)
				if not p.isEverAlive() or p.isMinorCiv():
					continue
				if p.wasCivRandomlyChosen() or p.wasLeaderRandomlyChosen():
					bNoneRandom = False
				if not p.wasCivRandomlyChosen() or not p.wasLeaderRandomlyChosen():
					bAllRandom = False
		if bCivLeaderSetupKnown and not bAllRandom and not bNoneRandom:
			szActivePlayerChoice = u"     "
			bActiveCivRandom = activePlayer.wasCivRandomlyChosen()
			bActiveLeaderRandom = activePlayer.wasLeaderRandomlyChosen()
			szCivDesc = activePlayer.getCivilizationShortDescriptionKey()
			if bActiveCivRandom and bActiveLeaderRandom:
				szActivePlayerChoice += localText.getText("TXT_KEY_RANDOMLY_CHOSEN", ())
			elif bActiveCivRandom: # Can only happen with unrestricted leaders
				szActivePlayerChoice += localText.getText("TXT_KEY_RANDOM_CIV", (szCivDesc,))
			elif bActiveLeaderRandom:
				szActivePlayerChoice += localText.getText("TXT_KEY_RANDOM_LEADER", (szCivDesc,))
			else:
				szActivePlayerChoice += localText.getText("TXT_KEY_MANUALLY_CHOSEN", ())
			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, szActivePlayerChoice, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# </advc.190c>
		SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, " ", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		#SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, localText.getText("TXT_KEY_SETTINGS_DIFFICULTY", (gc.getHandicapInfo(activePlayer.getHandicapType()).getTextKey(), )), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		# K-Mod. In multiplayer games, show both the game difficulty and the player difficulty
		if activePlayer.getHandicapType() == g.getHandicapType():
			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, localText.getText("TXT_KEY_SETTINGS_DIFFICULTY", (gc.getHandicapInfo(activePlayer.getHandicapType()).getTextKey(), )), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		# <advc.708>
		elif g.isOption(GameOptionTypes.GAMEOPTION_RISE_FALL):
			szBuffer = localText.getText("TXT_KEY_RF_HANDICAP_SETTINGS", (gc.getHandicapInfo(g.getHandicapType()).getTextKey(), gc.getHandicapInfo(activePlayer.getHandicapType()).getTextKey(), gc.getHandicapInfo(g.getAIHandicap()).getTextKey(), ))
			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, szBuffer, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# </advc.708>
		else:
			# advc.076: last arg uses TXT_KEY_SHORTCUTS_GAME (not TXT_KEY_OPTIONS_GAME, which no longer just says "Game" - this BUG text key does)
			szBuffer = "%s :\n  %s (%s) / %s (%s)" % (localText.getText("TXT_KEY_PITBOSS_DIFFICULTY", ()), gc.getHandicapInfo(activePlayer.getHandicapType()).getText(), localText.getText("TXT_KEY_MAIN_MENU_PLAYER", ()), gc.getHandicapInfo(g.getHandicapType()).getText(), localText.getText("TXT_KEY_SHORTCUTS_GAME", ()))
			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, szBuffer, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		# K-Mod end
		SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, " ", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, m.getMapScriptName(), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		# <!-- custom: append the actual generated grid (CyMap getGridWidth/Height) so the real map size is verifiable, exposing scripts whose getGridSize ignores the WorldInfo size.
		# This helped catch the old SAS_WorldSizes import-star compact-size leak. (Claude code Opus 4.7) -->
		SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, localText.getText("TXT_KEY_SETTINGS_MAP_SIZE", (gc.getWorldInfo(m.getWorldSize()).getTextKey(), )) + (u" (%d x %d)" % (m.getGridWidth(), m.getGridHeight())), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, localText.getText("TXT_KEY_SETTINGS_CLIMATE", (gc.getClimateInfo(m.getClimate()).getTextKey(), )), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, localText.getText("TXT_KEY_SETTINGS_SEA_LEVEL", (gc.getSeaLevelInfo(m.getSeaLevel()).getTextKey(), )), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		# <!-- custom: Show the same cached landHeavy/navalHeavy AI map-heaviness flags used by DLL behavior and BBAI_GAME_SETTINGS, so they can be checked in-game without enabling logs. (GPT-5.5) -->
		if self.SAS_CV_VICTORY_SCREEN_SETTINGS_AI_MAP_HEAVINESS_SHOW is None:
			self.SAS_CV_VICTORY_SCREEN_SETTINGS_AI_MAP_HEAVINESS_SHOW = (gc.getDefineINT("SAS_CV_VICTORY_SCREEN_SETTINGS_AI_MAP_HEAVINESS_SHOW") > 0)
		if self.SAS_CV_VICTORY_SCREEN_SETTINGS_AI_MAP_HEAVINESS_SHOW:
			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, localText.getText("TXT_KEY_SAS_VICTORY_SETTINGS_AI_MAP_HEAVINESS", (int(g.isLandHeavyMapnameCached()), int(g.isNavalHeavyMapnameCached()))), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# <advc.190b>
		for i in range(m.getNumCustomMapOptions()):
			# Don't know how to call map script functions in Python, so I've implemented that in the DLL.
			szDesc = m.getNonDefaultCustomMapOptionDesc(i)
			if len(szDesc) <= 0: # Meaning that the option is set to its default value
				continue
			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, szDesc, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY) # </advc.190b>
		# <advc.190c>
		szPlayerCountText = str(g.countCivPlayersEverAlive()) + " " + localText.getText("TXT_KEY_MAIN_MENU_PLAYERS", ())
		if bAllRandom:
			szPlayerCountText += " ("
			szPlayerCountText += localText.getText("TXT_KEY_ALL_AT_RANDOM", ())
			szPlayerCountText += ")"
		elif bNoneRandom and not bScenario:
			szPlayerCountText += " ("
			szPlayerCountText += localText.getText("TXT_KEY_NONE_AT_RANDOM", ())
			szPlayerCountText += ")"
		SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, szPlayerCountText, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# </advc.190c>
		SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, " ", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, localText.getText("TXT_KEY_SETTINGS_STARTING_ERA", (gc.getEraInfo(g.getStartEra()).getTextKey(), )), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		# <advc.251>
		iStartTurn = g.getStartTurn()
		if iStartTurn != 0:
			szTurnDate = CyGameTextMgr().getDateStr(iStartTurn, False, g.getCalendar(), g.getStartYear(), g.getGameSpeedType())
			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, localText.getText("TXT_KEY_SETTINGS_START_TURN", (iStartTurn, szTurnDate)), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		# </advc.251>
		SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, localText.getText("TXT_KEY_SETTINGS_GAME_SPEED", (gc.getGameSpeedInfo(g.getGameSpeedType()).getTextKey(), )), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		# <!-- custom: Replace the old plain mod-name row with current runtime source identity plus the revision lineage persisted by this save.
		# The runtime row links to the detailed Sevopedia Mods Info page; save-history rows stay compact enough for this already-scrollable Settings list. (ChatGPT-5.6-Sol) -->
		iSourceConcept = getNewConceptID("CONCEPT_SAS_VERSION_SOURCE_INFO")
		eSourceWidget = WidgetTypes.WIDGET_GENERAL
		iSourceData1 = -1
		iSourceData2 = -1
		if iSourceConcept >= 0:
			eSourceWidget = WidgetTypes.WIDGET_PEDIA_DESCRIPTION
			iSourceData1 = CivilopediaPageTypes.CIVILOPEDIA_PAGE_CONCEPT_NEW
			iSourceData2 = iSourceConcept
		SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, localText.getText("TXT_KEY_SAS_VERSION_UI_RUNTIME_SOURCE", (CvModName.getDisplayNameAndVersion(),)), eSourceWidget, iSourceData1, iSourceData2, CvUtil.FONT_LEFT_JUSTIFY)

		iHistoryEntries = g.getNumSASVersionHistoryEntries()
		if iHistoryEntries > 0:
			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, localText.getText("TXT_KEY_SAS_VERSION_UI_SAVE_HISTORY", ()), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			for iHistory in range(iHistoryEntries):
				szHistoryDetails = CvModName.formatVersionDetails(g.getSASVersionHistoryVersion(iHistory), g.getSASVersionHistoryCommitHash(iHistory), g.getSASVersionHistoryDirtyState(iHistory))
				if not szHistoryDetails:
					szHistoryDetails = localText.getText("TXT_KEY_SAS_VERSION_UNKNOWN", ())
				szHistoryKey = "TXT_KEY_SAS_VERSION_UI_CREATED"
				if iHistory > 0:
					szHistoryKey = "TXT_KEY_SAS_VERSION_UI_TRANSITION"
				SASTextScale.appendListBoxStringNoUpdateLabel(screen, szSettingsTable, localText.getText(szHistoryKey, (szHistoryDetails, g.getSASVersionHistoryTurn(iHistory))), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		screen.updateListBox(szSettingsTable)

		szOptionsPanel = self.getNextWidgetName()
		screen.addPanel(szOptionsPanel, "", "", True, True, self.SETTINGS_PANEL_X2, self.SETTINGS_PANEL_Y - 10, self.SETTINGS_PANEL_WIDTH, self.SETTINGS_PANEL_HEIGHT, PanelStyles.PANEL_STYLE_MAIN)
		szOptionsHeader = self.getNextWidgetName()
		screen.setLabel(szOptionsHeader, "Background", self.LABEL_OPTIONS_HEADER, CvUtil.FONT_CENTER_JUSTIFY, self.SETTINGS_PANEL_X2 + self.SETTINGS_PANEL_WIDTH / 2, self.SETTINGS_PANEL_Y + 2, self.Z_CONTROLS, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		szOptionsTable = self.getNextWidgetName()
		screen.addListBoxGFC(szOptionsTable, "", self.SETTINGS_PANEL_X2 + self.MARGIN, self.SETTINGS_PANEL_Y + self.MARGIN, self.SETTINGS_PANEL_WIDTH - 2*self.MARGIN, self.SETTINGS_PANEL_HEIGHT - 2*self.MARGIN, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(szOptionsTable, False)
		bSPaH = g.isOption(GameOptionTypes.GAMEOPTION_SPAH) # advc.250b
		for i in range(GameOptionTypes.NUM_GAMEOPTION_TYPES):
			if g.isOption(i):
				# <advc.104> Handle Aggressive AI below; skip it here.
				if i == GameOptionTypes.GAMEOPTION_AGGRESSIVE_AI:
					continue # </advc.104>
				# <advc.250b> Handle Advanced Start options below if SPaH.
				if bSPaH:
					if i == GameOptionTypes.GAMEOPTION_SPAH:
						continue
					if i == GameOptionTypes.GAMEOPTION_ADVANCED_START:
						continue
				# </advc.250b>
				# <advc.300> Show earliest turn that barbarians can spawn on
				szDescr = gc.getGameOptionInfo(i).getDescription()
				if i == GameOptionTypes.GAMEOPTION_RAGING_BARBARIANS:
					iBarbarianStartTurn = g.getBarbarianStartTurn()
					# Stop displaying after some time [Since v0.94: Don't show the start turn at all.]
					if False and iBarbarianStartTurn + 25 > g.getGameTurn():
						szDescr += "\n\t("
						szDescr += localText.getText("TXT_KEY_BARB_START", ())
						szDescr += " " + str(iBarbarianStartTurn) + ")"
				# </advc.300>
				SASTextScale.appendListBoxStringNoUpdateLabel(screen, szOptionsTable, szDescr, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# <advc.190a> Disabled victory conditions. Some overlap with CvReplayInfo::addSettingsMsg
		for i in range(gc.getNumVictoryInfos()):
			if not g.isVictoryValid(i):
				SASTextScale.appendListBoxStringNoUpdateLabel(screen, szOptionsTable, gc.getVictoryInfo(i).getDescription() + " " + localText.getText("TXT_KEY_VICTORY_DISABLED",()), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# </advc.190a>

		# <advc.104> AI settings
		bLegacyAI = g.useKModAI()
		szAIOption = None
		if g.isOption(GameOptionTypes.GAMEOPTION_AGGRESSIVE_AI) and bLegacyAI:
			szAIOption = localText.getText("TXT_KEY_GAME_OPTION_AGGRESSIVE_AI",())
		elif bLegacyAI: # Only possible if Aggressive AI disabled through XML
			szAIOption = "Non-Aggressive AI"
		if not szAIOption is None:
			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szOptionsTable, szAIOption, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# </advc.104>

		if (g.isOption(GameOptionTypes.GAMEOPTION_ADVANCED_START)):
			if not bSPaH: # advc.250b
				szNumPoints = u"%s %d" % (localText.getText("TXT_KEY_ADVANCED_START_POINTS", ()), g.getNumAdvancedStartPoints())
			# <advc.250b>
			else:
				# Could be done directly in Python, but I also need it for advc.106i
				szNumPoints = g.SPaHPointsForSettingsScreen()
			# </advc.250b>
			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szOptionsTable, szNumPoints, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		if g.isGameMultiPlayer():
			for i in range(gc.getNumMPOptionInfos()):
				if (g.isMPOption(i)):
					SASTextScale.appendListBoxStringNoUpdateLabel(screen, szOptionsTable, gc.getMPOptionInfo(i).getDescription(), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			if g.getMaxTurns() > 0:
				# <advc.135d> Based on code in CvGame::setStartTurnYear
				iDefaultEndTurn = 0
				for i in range(gc.getGameSpeedInfo(g.getGameSpeedType()).getNumTurnIncrements()):
					iDefaultEndTurn += gc.getGameSpeedInfo(g.getGameSpeedType()).getGameTurnInfo(i).iNumGameTurnsPerIncrement
				if g.getMaxTurns() != iDefaultEndTurn:
				# </advc.135d>
					szMaxTurns = u"%s %d" % (localText.getText("TXT_KEY_TURN_LIMIT_TAG", ()), g.getMaxTurns())
					SASTextScale.appendListBoxStringNoUpdateLabel(screen, szOptionsTable, szMaxTurns, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			if (g.getMaxCityElimination() > 0):
				szMaxCityElimination = u"%s %d" % (localText.getText("TXT_KEY_CITY_ELIM_TAG", ()), g.getMaxCityElimination())
				SASTextScale.appendListBoxStringNoUpdateLabel(screen, szOptionsTable, szMaxCityElimination, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		if (g.hasSkippedSaveChecksum()):
			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szOptionsTable, localText.getText("TXT_KEY_BUFFYWARNING_CHECKSUM_SKIPPED", ()), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		screen.updateListBox(szOptionsTable)

		szCivsPanel = self.getNextWidgetName()
		# <advc.190c>
		# "Rivals" is misleading: teammates and vassals aren't excluded. (The active player is excluded, but also hasn't "met" itself except in a technical sense.)
		szRivalsHeadingTag = "TXT_KEY_PLAYERS_MET" #"TXT_KEY_RIVALS_MET"
		if bCivLeaderSetupKnown:
			szRivalsHeadingTag = "TXT_KEY_OTHER_PLAYERS" # </advc.190c>
		screen.addPanel(szCivsPanel, "", "", True, True, self.SETTINGS_PANEL_X3, self.SETTINGS_PANEL_Y - 10, self.SETTINGS_PANEL_WIDTH, self.SETTINGS_PANEL_HEIGHT, PanelStyles.PANEL_STYLE_MAIN)
		szCivsHeader = self.getNextWidgetName()
		screen.setLabel(szCivsHeader, "Background", sasFontTagLabel.bold + localText.getText(szRivalsHeadingTag, ()).upper() + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, self.SETTINGS_PANEL_X3 + self.SETTINGS_PANEL_WIDTH / 2, self.SETTINGS_PANEL_Y + 2, self.Z_CONTROLS, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		szCivsTable = self.getNextWidgetName()
		screen.addListBoxGFC(szCivsTable, "", self.SETTINGS_PANEL_X3 + self.MARGIN, self.SETTINGS_PANEL_Y + self.MARGIN, self.SETTINGS_PANEL_WIDTH - 2*self.MARGIN, self.SETTINGS_PANEL_HEIGHT - 2*self.MARGIN, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(szCivsTable, False)
		# <advc.190c>
		rivalsMet = []
		knownRivalsNotMet = []
		iUnknownRivals = 0
		for i in range(gc.getMAX_CIV_PLAYERS()):
			p = gc.getPlayer(i)
			# No functional change here
			if not p.isEverAlive() or p.getID() == activePlayer.getID() or p.isMinorCiv():
				continue
			if gc.getTeam(p.getTeam()).isHasMet(activePlayer.getTeam()):
				rivalsMet.append(p)
			elif bCivLeaderSetupKnown:
				if p.wasCivRandomlyChosen() and p.wasLeaderRandomlyChosen() and not g.isDebugMode():
					iUnknownRivals += 1
				else:
					knownRivalsNotMet.append(p)

		for p in rivalsMet: # </advc.190c>
			# <!-- custom: add leader icon to player names in Other Players panel (claude opus 4.5) -->
			szPlayerName = u"%s %s" % (self.getLeaderIconTag(p.getID()), localText.getText("TXT_KEY_LEADER_CIV_DESCRIPTION", (p.getNameKey(), p.getCivilizationShortDescriptionKey())))
			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szCivsTable, szPlayerName, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szCivsTable, u"     " + TraitUtil.getLeaderTraitsWithIcons(p.getLeaderType(), p.getCivilizationType()), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szCivsTable, " ", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# <advc.190c>
		for p in knownRivalsNotMet:
			# Don't call CvPlayer functions here. Use XML data b/c that's what appears during game setup.
			szLeader = gc.getLeaderHeadInfo(p.getLeaderType()).getDescription()
			szCiv = gc.getCivilizationInfo(p.getCivilizationType()).getShortDescription(0)
			szRivalInfo = localText.getText("TXT_KEY_RIVAL_NOT_MET", ())
			szRivalInfo += ": "

			if (not p.wasCivRandomlyChosen() and not p.wasLeaderRandomlyChosen()) or g.isDebugMode():
				# <!-- custom: add leader icon for rivals not met (claude opus 4.5) -->
				szRivalInfo += u"%s %s" % (self.getLeaderIconTag(p.getID()), localText.getText("TXT_KEY_LEADER_CIV_DESCRIPTION", (szLeader, szCiv)))
			elif not p.wasCivRandomlyChosen():
				szRivalInfo += szCiv
			else:
				# <!-- custom: add leader icon when only leader is known (claude opus 4.5) -->
				szRivalInfo += u"%s %s" % (self.getLeaderIconTag(p.getID()), szLeader)

			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szCivsTable, szRivalInfo, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			if not p.wasLeaderRandomlyChosen():
				# (Not relevant for the trait string, but let's pass a proper argument anyway.)
				eCiv = CivilizationTypes.NO_CIVILIZATION
				if not p.wasCivRandomlyChosen():
					eCiv = p.getCivilizationType()
				SASTextScale.appendListBoxStringNoUpdateLabel(screen, szCivsTable, u"     " + TraitUtil.getLeaderTraitsWithIcons(p.getLeaderType(), eCiv), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szCivsTable, " ", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY) # newline
		if iUnknownRivals > 0:
			szUnknown = localText.getText("TXT_KEY_UNKNOWN_RIVALS", (iUnknownRivals,))
			if iUnknownRivals == 1:
				szUnknown = localText.getText("TXT_KEY_ONE_UNKNOWN_RIVAL", ())
			SASTextScale.appendListBoxStringNoUpdateLabel(screen, szCivsTable, szUnknown, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		# </advc.190c>

		screen.updateListBox(szCivsTable)

		self.drawTabs()

	def showVictoryConditionScreen(self):
		activePlayer = PyHelpers.PyPlayer(self.iActivePlayer)
		iActiveTeam = gc.getPlayer(self.iActivePlayer).getTeam()

		# checking if apollo has been built - clear arrays / lists / whatever they are called
		self.ApolloTeamsChecked = set()
		self.ApolloTeamCheckResult = {}

		# Conquest
		nRivals = -1
# BUG Additions Start
		nknown = 0
		nVassaled = 0
# BUG Additions End
		for i in range(gc.getMAX_CIV_TEAMS()):
			if (gc.getTeam(i).isAlive() and not gc.getTeam(i).isMinorCiv() and not gc.getTeam(i).isBarbarian()):
				nRivals += 1
# BUG Additions Start
				if i != iActiveTeam:
					if gc.getTeam(i).isHasMet(iActiveTeam):
						nknown += 1
					if gc.getTeam(i).isVassal(iActiveTeam):
						nVassaled += 1
# BUG Additions End

		# Population
		totalPop = gc.getGame().getTotalPopulation()
		ourPop = activePlayer.getTeam().getTotalPopulation()
		if (totalPop > 0):
			popPercent = (ourPop * 100.0) / totalPop
		else:
			popPercent = 0.0

		iBestPopTeam = -1
		bestPop = 0
		for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
			if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
				if (iLoopTeam != iActiveTeam and (activePlayer.getTeam().isHasMet(iLoopTeam) or gc.getGame().isDebugMode())):
					teamPop = gc.getTeam(iLoopTeam).getTotalPopulation()
					if (teamPop > bestPop):
						bestPop = teamPop
						iBestPopTeam = iLoopTeam

		# Score
		ourScore = gc.getGame().getTeamScore(iActiveTeam)

		iBestScoreTeam = -1
		bestScore = 0
		for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
			if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
				if (iLoopTeam != iActiveTeam and (activePlayer.getTeam().isHasMet(iLoopTeam) or gc.getGame().isDebugMode())):
					teamScore = gc.getGame().getTeamScore(iLoopTeam)
					if (teamScore > bestScore):
						bestScore = teamScore
						iBestScoreTeam = iLoopTeam

		# Land Area
		totalLand = gc.getMap().getLandPlots()
		ourLand = activePlayer.getTeam().getTotalLand()
		if (totalLand > 0):
			landPercent = (ourLand * 100.0) / totalLand
		else:
			landPercent = 0.0

		iBestLandTeam = -1
		bestLand = 0
		for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
			if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
				if (iLoopTeam != iActiveTeam and (activePlayer.getTeam().isHasMet(iLoopTeam) or gc.getGame().isDebugMode())):
					teamLand = gc.getTeam(iLoopTeam).getTotalLand()
					if (teamLand > bestLand):
						bestLand = teamLand
						iBestLandTeam = iLoopTeam

		# Religion
		iOurReligion = -1
		ourReligionPercent = 0
		for iLoopReligion in range(gc.getNumReligionInfos()):
			if (activePlayer.getTeam().hasHolyCity(iLoopReligion)):
				religionPercent = gc.getGame().calculateReligionPercent(iLoopReligion)
				if (religionPercent > ourReligionPercent):
					ourReligionPercent = religionPercent
					iOurReligion = iLoopReligion

		iBestReligion = -1
		bestReligionPercent = 0
		for iLoopReligion in range(gc.getNumReligionInfos()):
			if (iLoopReligion != iOurReligion):
				religionPercent = gc.getGame().calculateReligionPercent(iLoopReligion)
				if (religionPercent > bestReligionPercent):
					bestReligionPercent = religionPercent
					iBestReligion = iLoopReligion

		# Vote
		aiVoteBuildingClass = []
		for i in range(gc.getNumBuildingInfos()):
			for j in range(gc.getNumVoteSourceInfos()):
				if (gc.getBuildingInfo(i).getVoteSourceType() == j):
					iUNTeam = -1
					bUnknown = true 
					for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
						if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
							if (gc.getTeam(iLoopTeam).getBuildingClassCount(gc.getBuildingInfo(i).getBuildingClassType()) > 0):
								iUNTeam = iLoopTeam
								if (iLoopTeam == iActiveTeam or gc.getGame().isDebugMode() or activePlayer.getTeam().isHasMet(iLoopTeam)):
									bUnknown = False
								break

					aiVoteBuildingClass.append((gc.getBuildingInfo(i).getBuildingClassType(), iUNTeam, bUnknown))

		#self.bVoteTab = (len(aiVoteBuildingClass) > 0)
		# K-Mod
		self.bVoteTab = False
		for i in range(gc.getNumVoteSourceInfos()):
			if gc.getGame().isDiploVote(i):
				self.bVoteTab = True
				break
		# K-Mod end

		self.deleteAllWidgets()	
		screen = self.getScreen()

		# Start filling in the table below
		screen.addPanel(self.getNextWidgetName(), "", "", False, False, self.X_AREA-10, self.Y_AREA-15, self.W_AREA+20, self.H_AREA+30, PanelStyles.PANEL_STYLE_BLUE50)
		szTable = self.getNextWidgetName()
		screen.addTableControlGFC(szTable, 6, self.X_AREA, self.Y_AREA, self.W_AREA, self.H_AREA, False, False, self.iLeaderButtonSize, self.iLeaderButtonSize, TableStyles.TABLE_STYLE_STANDARD)
		screen.setTableColumnHeader(szTable, 0, "", self.TABLE_WIDTH_0)
		screen.setTableColumnHeader(szTable, 1, "", self.TABLE_WIDTH_1)
		screen.setTableColumnHeader(szTable, 2, "", self.TABLE_WIDTH_2)
		screen.setTableColumnHeader(szTable, 3, "", self.TABLE_WIDTH_3)
		screen.setTableColumnHeader(szTable, 4, "", self.TABLE_WIDTH_4)
		screen.setTableColumnHeader(szTable, 5, "", self.TABLE_WIDTH_5)
		screen.appendTableRow(szTable)

		# <!-- custom: cache active player name and leader button before loop (claude opus 4.5). (GPT-5.2-Codex (summarized)) -->
		szActivePlayerNameWithColon = activePlayer.getName() + ":"
		szActivePlayerButton = self.getLeaderButton(self.iActivePlayer)

		for iLoopVC in range(gc.getNumVictoryInfos()):
			victory = gc.getVictoryInfo(iLoopVC)
			if gc.getGame().isVictoryValid(iLoopVC):

				iNumRows = screen.getTableNumRows(szTable)
				szVictoryType = sasFontTagTitle.bold + victory.getDescription().upper() + SAS_FONT_TAG_CLOSE
				if (victory.isEndScore() and (gc.getGame().getMaxTurns() > gc.getGame().getElapsedGameTurns())):
					szTurnsLeft = localText.getText("TXT_KEY_MISC_TURNS_LEFT", (gc.getGame().getMaxTurns() - gc.getGame().getElapsedGameTurns(), ))
					szVictoryType += u"    " + sasFontTagLabel + u"(" + szTurnsLeft + u")" + SAS_FONT_TAG_CLOSE

				iVictoryTitleRow = iNumRows - 1
				SASTextScale.setTableTextLabel(screen, szTable, 0, iVictoryTitleRow, szVictoryType, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				bSpaceshipFound = False

				bEntriesFound = False

				# <!-- custom: cache target score to avoid duplicate call (claude opus 4.5) -->
				iTargetScore = gc.getGame().getTargetScore()
				if (victory.isTargetScore() and iTargetScore != 0):

					iRow = screen.appendTableRow(szTable)
					SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_TARGET_SCORE", (iTargetScore, )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					# <!-- custom: add leader icon to active player (claude opus 4.5) -->
					SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, szActivePlayerNameWithColon, szActivePlayerButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					# <!-- custom: add thousand separator to score (claude opus 4.5) -->
					SASTextScale.setTableTextLabel(screen, szTable, 3, iRow, separateThousands(ourScore, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					if (iBestScoreTeam != -1):
						# <!-- custom: add leader icon (claude opus 4.5) -->

						iBestPlayer = self.getPlayerOnTeam(iBestScoreTeam)
						szBestName = gc.getTeam(iBestScoreTeam).getName() + ":"
						szBestButton = u""
						if iBestPlayer >= 0:
							szBestName = gc.getPlayer(iBestPlayer).getName() + ":"
							szBestButton = self.getLeaderButton(iBestPlayer)
						SASTextScale.setTableTextLabel(screen, szTable, 4, iRow, szBestName, szBestButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						# <!-- custom: add thousand separator to score (claude opus 4.5) -->
						SASTextScale.setTableTextLabel(screen, szTable, 5, iRow, separateThousands(bestScore, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					bEntriesFound = True

				if (victory.isEndScore()):

					szText1 = localText.getText("TXT_KEY_VICTORY_SCREEN_HIGHEST_SCORE", (CyGameTextMgr().getTimeStr(gc.getGame().getStartTurn() + gc.getGame().getMaxTurns(), False), ))

					iRow = screen.appendTableRow(szTable)
					# <!-- custom: add trophy icon to Highest Score row (claude opus 4.5) -->
					szScoreText = u"%s %s" % (self.szTrophyImgTag, szText1)
					SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, szScoreText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					# <!-- custom: add leader icon to active player (claude opus 4.5) -->
					SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, szActivePlayerNameWithColon, szActivePlayerButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					# <!-- custom: add thousand separator to score (claude opus 4.5) -->
					SASTextScale.setTableTextLabel(screen, szTable, 3, iRow, separateThousands(ourScore, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					if (iBestScoreTeam != -1):
						# <!-- custom: add leader icon (claude opus 4.5) -->
						iBestPlayer = self.getPlayerOnTeam(iBestScoreTeam)

						szBestName = gc.getTeam(iBestScoreTeam).getName() + ":"
						szBestButton = u""
						if iBestPlayer >= 0:
							szBestName = gc.getPlayer(iBestPlayer).getName() + ":"
							szBestButton = self.getLeaderButton(iBestPlayer)
						SASTextScale.setTableTextLabel(screen, szTable, 4, iRow, szBestName, szBestButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

						# <!-- custom: add thousand separator to score (claude opus 4.5) -->
						SASTextScale.setTableTextLabel(screen, szTable, 5, iRow, separateThousands(bestScore, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					bEntriesFound = True

				if (victory.isConquest()):
					iRow = screen.appendTableRow(szTable)

					# <!-- custom: add strength icon to Conquest row (claude opus 4.5) -->
					SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, self.szConquestText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, self.TEXT_RIVALS_LEFT, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					SASTextScale.setTableTextLabel(screen, szTable, 3, iRow, unicode(nRivals), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					bEntriesFound = True
# BUG Additions Start
					if AdvisorOpt.isVictories():
						if nVassaled != 0:
							sString = localText.getText("TXT_KEY_BUG_VICTORY_VASSALED", (nVassaled, ))
							SASTextScale.setTableTextLabel(screen, szTable, 4, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						if nRivals - nknown != 0:
							sString = localText.getText("TXT_KEY_BUG_VICTORY_UNKNOWN", (nRivals - nknown, ))
							SASTextScale.setTableTextLabel(screen, szTable, 5, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
# BUG Additions End

				# <!-- custom: cache adjusted population percent to avoid duplicate call (claude opus 4.5) -->
				iAdjustedPopPercent = gc.getGame().getAdjustedPopulationPercent(iLoopVC)
				if (iAdjustedPopPercent > 0):
					iRow = screen.appendTableRow(szTable)

					# <!-- custom: add population icon (claude opus 4.5) -->
					szPopText = u"%c %s" % (self.iCitizenIcon, localText.getText("TXT_KEY_VICTORY_SCREEN_PERCENT_POP", (iAdjustedPopPercent, )))
					SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, szPopText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					# <!-- custom: add leader icon to active player (claude opus 4.5) -->
					SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, szActivePlayerNameWithColon, szActivePlayerButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					SASTextScale.setTableTextLabel(screen, szTable, 3, iRow, (u"%.2f%%" % popPercent), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					if (iBestPopTeam != -1):
						# <!-- custom: add leader icon (claude opus 4.5) -->
						iBestPlayer = self.getPlayerOnTeam(iBestPopTeam)
						szBestName = gc.getTeam(iBestPopTeam).getName() + ":"
						szBestButton = u""
						if iBestPlayer >= 0:
							szBestName = gc.getPlayer(iBestPlayer).getName() + ":"
							szBestButton = self.getLeaderButton(iBestPlayer)
						SASTextScale.setTableTextLabel(screen, szTable, 4, iRow, szBestName, szBestButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						SASTextScale.setTableTextLabel(screen, szTable, 5, iRow, (u"%.2f%%" % (bestPop * 100 / totalPop)), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					bEntriesFound = True

				# <!-- custom: cache adjusted land percent to avoid duplicate call (claude opus 4.5) -->
				iAdjustedLandPercent = gc.getGame().getAdjustedLandPercent(iLoopVC)
				if (iAdjustedLandPercent > 0):
					iRow = screen.appendTableRow(szTable)
					# <!-- custom: add map icon (claude opus 4.5) -->
					szLandText = u"%c %s" % (self.iMapIcon, localText.getText("TXT_KEY_VICTORY_SCREEN_PERCENT_LAND", (iAdjustedLandPercent, )))
					SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, szLandText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					# <!-- custom: add leader icon to active player (claude opus 4.5) -->
					SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, szActivePlayerNameWithColon, szActivePlayerButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					SASTextScale.setTableTextLabel(screen, szTable, 3, iRow, (u"%.2f%%" % landPercent), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					if (iBestLandTeam != -1):
						# <!-- custom: add leader icon (claude opus 4.5) -->
						iBestPlayer = self.getPlayerOnTeam(iBestLandTeam)
						szBestName = gc.getTeam(iBestLandTeam).getName() + ":"
						szBestButton = u""
						if iBestPlayer >= 0:
							szBestName = gc.getPlayer(iBestPlayer).getName() + ":"
							szBestButton = self.getLeaderButton(iBestPlayer)
						SASTextScale.setTableTextLabel(screen, szTable, 4, iRow, szBestName, szBestButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						SASTextScale.setTableTextLabel(screen, szTable, 5, iRow, (u"%.2f%%" % (bestLand * 100 / totalLand)), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					bEntriesFound = True

				if (victory.getReligionPercent() > 0):
					iRow = screen.appendTableRow(szTable)
					SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_PERCENT_RELIGION", (victory.getReligionPercent(), )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					if (iOurReligion != -1):
						SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, gc.getReligionInfo(iOurReligion).getDescription() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						SASTextScale.setTableTextLabel(screen, szTable, 3, iRow, (u"%d%%" % ourReligionPercent), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					else:
						SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, activePlayer.getTeam().getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						SASTextScale.setTableTextLabel(screen, szTable, 3, iRow, u"No Holy City", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					if (iBestReligion != -1):
						SASTextScale.setTableTextLabel(screen, szTable, 4, iRow, gc.getReligionInfo(iBestReligion).getDescription() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						SASTextScale.setTableTextLabel(screen, szTable, 5, iRow, (u"%d%%" % religionPercent), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					bEntriesFound = True

				if (victory.getTotalCultureRatio() > 0):
					iRow = screen.appendTableRow(szTable)
					# <advc> Moved down to have this stuff in one place
					ourCulture = activePlayer.getTeam().countTotalCulture()
					iBestCultureTeam = -1
					bestCulture = 0
					for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
						if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
							if (iLoopTeam != iActiveTeam and (activePlayer.getTeam().isHasMet(iLoopTeam) or gc.getGame().isDebugMode())):
								teamCulture = gc.getTeam(iLoopTeam).countTotalCulture()
								if (teamCulture > bestCulture):
									bestCulture = teamCulture
									iBestCultureTeam = iLoopTeam
					# </advc>
					#iCulturePercent = int((100.0 * bestCulture) / victory.getTotalCultureRatio())
					# advc.004: The above is neither helpful nor really a percentage. Let's simply show the target ratio for now. What's really needed is, I think, the ratio of bestCulture to secondBestCulture, where secondBestCulture is computed w/o checking isHasMet. Or possibly that ratio divided by victory.getTotalCultureRatio (meaning that 100% is needed for victory). 
					iCulturePercent = victory.getTotalCultureRatio()
					SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_PERCENT_CULTURE", (iCulturePercent, )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, activePlayer.getTeam().getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					SASTextScale.setTableTextLabel(screen, szTable, 3, iRow, unicode(ourCulture), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					if (iBestLandTeam != -1):
						# <!-- custom: add leader icon (claude opus 4.5) -->
						iBestPlayer = self.getPlayerOnTeam(iBestCultureTeam)
						szBestName = gc.getTeam(iBestCultureTeam).getName() + ":"
						szBestButton = u""
						if iBestPlayer >= 0:
							szBestName = gc.getPlayer(iBestPlayer).getName() + ":"
							szBestButton = self.getLeaderButton(iBestPlayer)
						SASTextScale.setTableTextLabel(screen, szTable, 4, iRow, szBestName, szBestButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						SASTextScale.setTableTextLabel(screen, szTable, 5, iRow, unicode(bestCulture), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					bEntriesFound = True

				iBestBuildingTeam = -1
				bestBuilding = 0
				for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
					if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
						if (iLoopTeam != iActiveTeam and (activePlayer.getTeam().isHasMet(iLoopTeam) or gc.getGame().isDebugMode())):
							teamBuilding = 0
							for i in range(gc.getNumBuildingClassInfos()):
								if (gc.getBuildingClassInfo(i).getVictoryThreshold(iLoopVC) > 0):
									teamBuilding += gc.getTeam(iLoopTeam).getBuildingClassCount(i)
							if (teamBuilding > bestBuilding):
								bestBuilding = teamBuilding
								iBestBuildingTeam = iLoopTeam

				for i in range(gc.getNumBuildingClassInfos()):
					if (gc.getBuildingClassInfo(i).getVictoryThreshold(iLoopVC) > 0):
						iRow = screen.appendTableRow(szTable)
						szNumber = unicode(gc.getBuildingClassInfo(i).getVictoryThreshold(iLoopVC))
						SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_BUILDING", (szNumber, gc.getBuildingClassInfo(i).getTextKey())), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, activePlayer.getTeam().getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						SASTextScale.setTableTextLabel(screen, szTable, 3, iRow, activePlayer.getTeam().getBuildingClassCount(i), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

						if (iBestBuildingTeam != -1):
							# <!-- custom: add leader icon (claude opus 4.5) -->
							iBestPlayer = self.getPlayerOnTeam(iBestBuildingTeam)
							szBestName = gc.getTeam(iBestBuildingTeam).getName() + ":"
							szBestButton = u""
							if iBestPlayer >= 0:
								szBestName = gc.getPlayer(iBestPlayer).getName() + ":"
								szBestButton = self.getLeaderButton(iBestPlayer)
							SASTextScale.setTableTextLabel(screen, szTable, 4, iRow, szBestName, szBestButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
							SASTextScale.setTableTextLabel(screen, szTable, 5, iRow, gc.getTeam(iBestBuildingTeam).getBuildingClassCount(i), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

						bEntriesFound = True

				iBestProjectTeam = -1
				bestProject = -1
				for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
					if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
						if (iLoopTeam != iActiveTeam and (activePlayer.getTeam().isHasMet(iLoopTeam) or gc.getGame().isDebugMode()) and self.isApolloBuiltbyTeam(gc.getTeam(iLoopTeam))):
							teamProject = 0
							for i in range(gc.getNumProjectInfos()):
								if (gc.getProjectInfo(i).getVictoryThreshold(iLoopVC) > 0):
									teamProject += gc.getTeam(iLoopTeam).getProjectCount(i)
							if (teamProject > bestProject):
								bestProject = teamProject
								iBestProjectTeam = iLoopTeam

# BUG Additions Start
				if AdvisorOpt.isVictories():
					bApolloShown = False
					for i in range(gc.getNumProjectInfos()):
						if (gc.getProjectInfo(i).getVictoryThreshold(iLoopVC) > 0):
							if not self.isApolloBuilt():
								iRow = screen.appendTableRow(szTable)
								SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, localText.getText("TXT_KEY_PROJECT_APOLLO_PROGRAM", ()), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
								SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, activePlayer.getTeam().getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
								SASTextScale.setTableTextLabel(screen, szTable, 3, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_NOT_BUILT", ()), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
								bEntriesFound = True
								break
							else:
								if not bApolloShown:
									bApolloShown = True
									iRow = screen.appendTableRow(szTable)
									SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, localText.getText("TXT_KEY_PROJECT_APOLLO_PROGRAM", ()), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

									if self.isApolloBuiltbyTeam(activePlayer.getTeam()):
										SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_BUILT", (activePlayer.getTeam().getName(), )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
									else:
										SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, activePlayer.getTeam().getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
										SASTextScale.setTableTextLabel(screen, szTable, 3, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_NOT_BUILT", ()), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

									if (iBestProjectTeam != -1):
										SASTextScale.setTableTextLabel(screen, szTable, 4, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_BUILT", (gc.getTeam(iBestProjectTeam).getName(), )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

								iRow = screen.appendTableRow(szTable)
								iReqTech = gc.getProjectInfo(i).getTechPrereq()

								if (gc.getProjectInfo(i).getVictoryMinThreshold(iLoopVC) == gc.getProjectInfo(i).getVictoryThreshold(iLoopVC)):
									szNumber = unicode(gc.getProjectInfo(i).getVictoryThreshold(iLoopVC))
								else:
									szNumber = unicode(gc.getProjectInfo(i).getVictoryMinThreshold(iLoopVC)) + u"-" + unicode(gc.getProjectInfo(i).getVictoryThreshold(iLoopVC))

								# <!-- custom: add project icon to space ship part (claude opus 4.5) -->
								szProjectButton = gc.getProjectInfo(i).getButton()
								sSSPart = localText.getText("TXT_KEY_VICTORY_SCREEN_BUILDING", (szNumber, gc.getProjectInfo(i).getTextKey()))
								SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, sSSPart, szProjectButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

								if self.isApolloBuiltbyTeam(activePlayer.getTeam()):

									bHasTech = gc.getTeam(iActiveTeam).isHasTech(iReqTech)
									sSSPlayer = activePlayer.getTeam().getName() + ":"
									sSSCount = "%i [+%i]" % (activePlayer.getTeam().getProjectCount(i), activePlayer.getTeam().getProjectMaking(i))

									iHasTechColor = -1
									iSSColor = 0
									if activePlayer.getTeam().getProjectCount(i) == gc.getProjectInfo(i).getVictoryThreshold(iLoopVC):
										sSSCount = "%i" % (activePlayer.getTeam().getProjectCount(i))
										iSSColor = ColorUtil.keyToType("COLOR_GREEN")
									elif activePlayer.getTeam().getProjectCount(i) >= gc.getProjectInfo(i).getVictoryMinThreshold(iLoopVC):
										iSSColor = ColorUtil.keyToType("COLOR_YELLOW")

									if iSSColor > 0:
										sSSPlayer = localText.changeTextColor(sSSPlayer, iSSColor)
										sSSCount = localText.changeTextColor(sSSCount, iSSColor)

									SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, sSSPlayer, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
									if bHasTech:
										SASTextScale.setTableTextLabel(screen, szTable, 3, iRow, sSSCount, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

									#check if spaceship
									if (gc.getProjectInfo(i).isSpaceship()):
										bSpaceshipFound = True

								# add AI space ship info
								if (iBestProjectTeam != -1):
									pTeam = gc.getTeam(iBestProjectTeam)
									sSSPlayer = gc.getTeam(iBestProjectTeam).getName() + ":"
									sSSCount = "%i" % (pTeam.getProjectCount(i))

									Techs = TechUtil.getVisibleKnownTechs(pTeam.getLeaderID(), self.iActivePlayer)
									bHasTech = iReqTech in Techs

									iHasTechColor = -1
									iSSColor = 0
									if pTeam.getProjectCount(i) == gc.getProjectInfo(i).getVictoryThreshold(iLoopVC):
										iSSColor = ColorUtil.keyToType("COLOR_GREEN")
									elif pTeam.getProjectCount(i) >= gc.getProjectInfo(i).getVictoryMinThreshold(iLoopVC):
										iSSColor = ColorUtil.keyToType("COLOR_YELLOW")
									elif bHasTech:
										iSSColor = ColorUtil.keyToType("COLOR_PLAYER_ORANGE")

									if iSSColor > 0:
										sSSPlayer = localText.changeTextColor(sSSPlayer, iSSColor)
										sSSCount = localText.changeTextColor(sSSCount, iSSColor)

									SASTextScale.setTableTextLabel(screen, szTable, 4, iRow, sSSPlayer, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
									SASTextScale.setTableTextLabel(screen, szTable, 5, iRow, sSSCount, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

								bEntriesFound = True

				else: # vanilla BtS SShip display
					for i in range(gc.getNumProjectInfos()):
						if (gc.getProjectInfo(i).getVictoryThreshold(iLoopVC) > 0):
							iRow = screen.appendTableRow(szTable)
							if (gc.getProjectInfo(i).getVictoryMinThreshold(iLoopVC) == gc.getProjectInfo(i).getVictoryThreshold(iLoopVC)):
								szNumber = unicode(gc.getProjectInfo(i).getVictoryThreshold(iLoopVC))
							else:
								szNumber = unicode(gc.getProjectInfo(i).getVictoryMinThreshold(iLoopVC)) + u"-" + unicode(gc.getProjectInfo(i).getVictoryThreshold(iLoopVC))

							# <!-- custom: add project icon to space ship part (claude opus 4.5) -->
							szProjectButton = gc.getProjectInfo(i).getButton()
							szProjectText = localText.getText("TXT_KEY_VICTORY_SCREEN_BUILDING", (szNumber, gc.getProjectInfo(i).getTextKey()))
							SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, szProjectText, szProjectButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
							# <!-- custom: add leader icon to active player (claude opus 4.5) -->
							szActivePlayerName = activePlayer.getName() + ":"

							SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, szActivePlayerName, szActivePlayerButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
							SASTextScale.setTableTextLabel(screen, szTable, 3, iRow, str(activePlayer.getTeam().getProjectCount(i)), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

							#check if spaceship
							if (gc.getProjectInfo(i).isSpaceship()):
								bSpaceshipFound = True

							if (iBestProjectTeam != -1):
								# <!-- custom: add leader icon (claude opus 4.5) -->
								iBestPlayer = self.getPlayerOnTeam(iBestProjectTeam)
								szBestName = gc.getTeam(iBestProjectTeam).getName() + ":"
								szBestButton = u""
								if iBestPlayer >= 0:
									szBestName = gc.getPlayer(iBestPlayer).getName() + ":"
									szBestButton = self.getLeaderButton(iBestPlayer)
								SASTextScale.setTableTextLabel(screen, szTable, 4, iRow, szBestName, szBestButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
								SASTextScale.setTableTextLabel(screen, szTable, 5, iRow, unicode(gc.getTeam(iBestProjectTeam).getProjectCount(i)), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

							bEntriesFound = True
# BUG Additions End

				#add spaceship button
				if (bSpaceshipFound):
					screen.setButtonGFC("SpaceShipButton" + str(iLoopVC), SASTextScale.labelText(localText.getText("TXT_KEY_GLOBELAYER_STRATEGY_VIEW", ())), "", 0, 0, 15, 10, WidgetTypes.WIDGET_GENERAL, self.SPACESHIP_SCREEN_BUTTON, -1, ButtonStyles.BUTTON_STYLE_STANDARD )
					screen.attachControlToTableCell("SpaceShipButton" + str(iLoopVC), szTable, iVictoryTitleRow, 1)

					victoryDelay = gc.getTeam(iActiveTeam).getVictoryCountdown(iLoopVC)
					if((victoryDelay > 0) and (gc.getGame().getGameState() != GameStateTypes.GAMESTATE_EXTENDED)):
						victoryDate = CyGameTextMgr().getTimeStr(gc.getGame().getGameTurn() + victoryDelay, False)
						SASTextScale.setTableTextLabel(screen, szTable, 2, iVictoryTitleRow, localText.getText("TXT_KEY_SPACE_SHIP_SCREEN_ARRIVAL", ()) + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						SASTextScale.setTableTextLabel(screen, szTable, 3, iVictoryTitleRow, victoryDate, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						SASTextScale.setTableTextLabel(screen, szTable, 4, iVictoryTitleRow, localText.getText("TXT_KEY_REPLAY_SCREEN_TURNS", ()) + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						SASTextScale.setTableTextLabel(screen, szTable, 5, iVictoryTitleRow, str(victoryDelay), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

				if (victory.isDiploVote()):
					for (iVoteBuildingClass, iUNTeam, bUnknown) in aiVoteBuildingClass:
						iRow = screen.appendTableRow(szTable)
						# <!-- custom: add building icon for UN/AP (claude opus 4.5) -->
						iBuildingType = gc.getBuildingClassInfo(iVoteBuildingClass).getDefaultBuildingIndex()
						szBuildingButton = gc.getBuildingInfo(iBuildingType).getButton()
						szElectionText = localText.getText("TXT_KEY_VICTORY_SCREEN_ELECTION", (gc.getBuildingClassInfo(iVoteBuildingClass).getTextKey(), ))
						SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, szElectionText, szBuildingButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

						if (iUNTeam != -1):
							if bUnknown:
								szName = localText.getText("TXT_KEY_TOPCIVS_UNKNOWN", ())
								szNameButton = u""
							else:
								# <!-- custom: add leader icon (claude opus 4.5) -->
								iUNPlayer = self.getPlayerOnTeam(iUNTeam)
								szName = gc.getTeam(iUNTeam).getName()
								szNameButton = u""
								if iUNPlayer >= 0:
									szName = gc.getPlayer(iUNPlayer).getName()
									szNameButton = self.getLeaderButton(iUNPlayer)
							SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_BUILT", (szName, )), szNameButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

						else:
							SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_NOT_BUILT", ()), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						bEntriesFound = True

				if (victory.getCityCulture() != CultureLevelTypes.NO_CULTURELEVEL and victory.getNumCultureCities() > 0):
					ourBestCities = self.getListCultureCities(iActiveTeam, victory)

					# K-Mod - changed to loop through teams rather than players, to match actual victory conditions.
					iBestCultureTeam = -1
					bestCityCulture = 0
					maxCityCulture = GameUtil.getCultureThreshold(victory.getCityCulture()) # BUG

					for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
						if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
							if (iLoopTeam != iActiveTeam and (activePlayer.getTeam().isHasMet(iLoopTeam) or gc.getGame().isDebugMode())):
								theirBestCities = self.getListCultureCities(iLoopTeam, victory)

								iTotalCulture = 0
								for loopCity in theirBestCities:
									if loopCity[0] >= maxCityCulture:
										iTotalCulture += maxCityCulture
									else:
										iTotalCulture += loopCity[0]

								# Note: we could give more weight to the lower cities if we wanted a more accurate gauge of how close to victory the team is.
								if (iTotalCulture >= bestCityCulture):
									bestCityCulture = iTotalCulture
									iBestCultureTeam = iLoopTeam

					if (iBestCultureTeam != -1):
						theirBestCities = self.getListCultureCities(iBestCultureTeam, victory)
					else:
						theirBestCities = []

					iRow = screen.appendTableRow(szTable)
					# <advc.126>
					eVictoryLevel = victory.getCityCulture()
					iCultureThresh = gc.getGame().getCultureThreshold(eVictoryLevel)
					# <!-- custom: add culture icon before text and use thousand separator (claude opus 4.5) -->
					# <!-- custom: The TXT_KEY_VICTORY_SCREEN_CITY_CULTURE's output is hard to handle with numbers sometimes coming or a culture char or whatnot not at the position we want, in the victory screen's Legendary cities header as part of prettifying it.
					# It seems easier to create one if i didn't do a mistake thinking so (check if accurate as i don't know too much about these).
					# Simplified Cultural victory text for Victory Screen (claude opus 4.5). -->
					# <!-- note: uses TXT_KEY_VICTORY_SCREEN_LEGENDARY_CITIES instead of TXT_KEY_VICTORY_SCREEN_CITY_CULTURE for cleaner display -->
					szCultureVictoryText = u"%c %d %s (%s)" % (self.iCultureIcon, victory.getNumCultureCities(), self.TEXT_LEGENDARY_CITIES, separateThousands(iCultureThresh, self.TEXT_THOUSANDS_SEPARATOR_COMMA))
					SASTextScale.setTableTextLabel(screen, szTable, 0, iRow, szCultureVictoryText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					for i in range(victory.getNumCultureCities()):
						if (len(ourBestCities) > i):
							SASTextScale.setTableTextLabel(screen, szTable, 2, iRow, ourBestCities[i][1].getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
# BUG Additions Start
							# <!-- custom: add thousand separator and percentage to culture values (claude opus 4.5) -->
							iCultureValue = ourBestCities[i][0]
							fCulturePercent = 0.0
							if iCultureThresh > 0:
								fCulturePercent = (float(iCultureValue) / float(iCultureThresh)) * 100.0
							sString = u"%s (%.2f%%)" % (separateThousands(iCultureValue, self.TEXT_THOUSANDS_SEPARATOR_COMMA), fCulturePercent)

							SASTextScale.setTableTextLabel(screen, szTable, 3, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
# BUG Additions End

						if (len(theirBestCities) > i):
							SASTextScale.setTableTextLabel(screen, szTable, 4, iRow, theirBestCities[i][1].getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

# BUG Additions Start
							# <!-- custom: add thousand separator and percentage to rival culture values (claude opus 4.5) -->
							# Original BUG Code Explanation
							# The original code used theirBestCities[i][2] which was a turns to legendary estimate. So the original showed: 26018 (15) meaning "26,018 culture, 15 turns to legendary".
							# Our New Code
							# We replaced it with a percentage of threshold instead. So now it shows: 26,018 (57.82%) meaning "26,018 culture, 57.82% of the 45,000 needed"
							iCultureValue = theirBestCities[i][0]
							fCulturePercent = 0.0
							if iCultureThresh > 0:
								fCulturePercent = (float(iCultureValue) / float(iCultureThresh)) * 100.0
							sString = u"%s (%.2f%%)" % (separateThousands(iCultureValue, self.TEXT_THOUSANDS_SEPARATOR_COMMA), fCulturePercent)

							SASTextScale.setTableTextLabel(screen, szTable, 5, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
# BUG Additions End

						if (i < victory.getNumCultureCities()-1):
							iRow = screen.appendTableRow(szTable)
					bEntriesFound = True

				if (bEntriesFound):
					screen.appendTableRow(szTable)
					screen.appendTableRow(szTable)

		# Remove the two final empty rows (K-Mod)
		if screen.getTableNumRows(szTable) > 2:
			screen.setTableNumRows(szTable, screen.getTableNumRows(szTable)-2)
		#

		# civ picker dropdown
		if (CyGame().isDebugMode()):
			self.szDropdownName = self.DEBUG_DROPDOWN_ID
			addAdvisorDebugDropdown(screen, self.szDropdownName, self.iActivePlayer, bAllowVassalPerspective=True) # advc.007: barbs excluded

		self.drawTabs()

#	def getListCultureCities(self, iPlayer):
# rewritten for K-Mod
	def getListCultureCities(self, iTeam, victory):
		maxCityCulture = GameUtil.getCultureThreshold(victory.getCityCulture())

		if iTeam < 0:
			return []
		# else continue

		cultureCityList = [] # item format is (culture, city, turns to threshold)

		for i in range(gc.getMAX_CIV_PLAYERS()):
			loopPlayer = gc.getPlayer(i)
			if (not loopPlayer.isAlive() or loopPlayer.getTeam() != iTeam):
				continue
			# otherwise, loop through their cities
			(loopCity, iter) = loopPlayer.firstCity(false)
			while(loopCity): # advc.001d: isRevealed clause added
				if (not loopCity.isNone() and loopCity.getTeam() == iTeam and loopCity.isRevealed(gc.getGame().getActiveTeam(), True)):
					iRate = loopCity.getCommerceRateTimes100(CommerceTypes.COMMERCE_CULTURE)
					if iRate == 0:
						iTurns = -1
					else:
						iCultureLeftTimes100 = 100 * maxCityCulture - loopCity.getCultureTimes100(loopCity.getOwner())
						iTurns = int((iCultureLeftTimes100 + iRate - 1) / iRate)
					cultureCityList.append((loopCity.getCulture(loopCity.getOwner()), PyHelpers.PyCity(loopCity.getOwner(), loopCity.getID()), iTurns))
					# I don't see the point of PyCity. But that's what's used elsewhere.
				(loopCity, iter) = loopPlayer.nextCity(iter, false)

		cultureCityList.sort()
		cultureCityList.reverse()
		return cultureCityList[0:victory.getNumCultureCities()]
# K-Mod end

# BUG Additions Start
	def getVotesForWhichCandidate(self, iPlayer, iCand1, iCand2, iVote):
		# returns are 1 = vote for candidate 1
		#             2 = vote for candidate 2
		#            -1 = abstain

		# iVote = 1 means vote for SecGen or Pope
		# iVote = 2 means vote for diplomatic victory

		# candidates are teams!!!

		# * AI votes for itself if it can
		# * AI votes for a team member if it can
		# * AI votes for its master, if it is a vassal
		# * if the AI attitude to one of the candidates is 'friendly' and the other is 'pleased' or less, AI votes for 'friend'
		# * if both candidates are at 'friendly' status, votes for one with highest attitude
		# * if neither candidate is at 'friendly', abstains

		iPTeam = gc.getPlayer(iPlayer).getTeam()
		iPCand1 = self.getPlayerOnTeam(iCand1)
		iPCand2 = self.getPlayerOnTeam(iCand2)

		# * player votes for its own team if it can
		if iPTeam == iCand1:
			return 1
		if iPTeam == iCand2:
			return 2

		# if player is human, votes for self or abstains
		if iPlayer == self.iActivePlayer:
			return -1

		# * AI votes for its master, if it is a vassal
		if gc.getTeam(iPTeam).isVassal(iCand1):
			return 1
		if gc.getTeam(iPTeam).isVassal(iCand2):
			return 2

		# get player category (friendly) to candidates
		iC1Cat = AttitudeUtil.getAttitudeCategory(iPlayer, iPCand1)
		iC2Cat = AttitudeUtil.getAttitudeCategory(iPlayer, iPCand2)

		# the cut-off for SecGen votes is pleased (3)
		# the cut-off for Diplo victory votes is friendly (4)
		if iVote == 1:  # vote for SecGen or Pope
			iCutOff = 3
		else:
			iCutOff = 4

		# * if neither candidate is at 'friendly', abstains
		# assumes friendly = 4, pleased = 3, etc
		if (iC1Cat < iCutOff and iC2Cat < iCutOff):
			return -1

		# * if the AI attitude to one of the candidates is 'friendly' and the other is 'pleased' or less, AI votes for 'friend'
		if (iC1Cat >= iCutOff and iC1Cat > iC2Cat):
			return 1

		if (iC2Cat >= iCutOff and iC2Cat > iC1Cat):
			return 2

		# if the code gets to here, then both candidates are at or above the cutoff
		# and they are both at the same category (ie both friendly)
		# need to decide on straight attitude (visible only)

		# get player attitude to candidates
		iC1Att = AttitudeUtil.getAttitudeCount(iPlayer, iPCand1)
		iC2Att = AttitudeUtil.getAttitudeCount(iPlayer, iPCand2)

		# * if both candidates are at 'friendly' status, votes for one with highest attitude
		if iC2Att < iC1Att: # ties go to Candidate #1
			return 1
		else:
			return 2

		return -1

	def getPlayerOnTeam(self, iTeam):
		for i in range(gc.getMAX_PLAYERS()):
			if iTeam == gc.getPlayer(i).getTeam():
				return i

		return -1

	def getAP_UN_OwnerTeam(self):
		for i in range(gc.getNumBuildingInfos()):
			for j in range(gc.getNumVoteSourceInfos()):
				if (gc.getBuildingInfo(i).getVoteSourceType() == j):
					for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
						if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
							if (gc.getTeam(iLoopTeam).getBuildingClassCount(gc.getBuildingInfo(i).getBuildingClassType()) > 0):
								return iLoopTeam
								break
		return -1

	def canBuildSSComponent(self, vTeam, vComponent):
		if(not vTeam.isHasTech(vComponent.getTechPrereq())):
			return False
		else:
			for j in range(gc.getNumProjectInfos()):
				if(vTeam.getProjectCount(j) < vComponent.getProjectsNeeded(j)):
					return False
		return True

	def isApolloBuilt(self):
		activePlayer = gc.getPlayer(self.iActivePlayer)
		iActiveTeam = activePlayer.getTeam()

		# check if anyone has built the apollo project (PROJECT_APOLLO_PROGRAM)
		for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
			pLoopTeam = gc.getTeam(iLoopTeam)
			if (pLoopTeam.isAlive() and not pLoopTeam.isMinorCiv() and not pLoopTeam.isBarbarian()):
				if iLoopTeam == iActiveTeam:
					bContact = True
				elif (gc.getTeam(iActiveTeam).isHasMet(iLoopTeam) or gc.getGame().isDebugMode()):
					bContact = True
				else:
					bContact = False

				if bContact:
					if self.isApolloBuiltbyTeam(pLoopTeam):
						return True
		return False

	def isApolloBuiltbyTeam(self, vTeam):
		iTeam = vTeam.getID()
#		print vTeam.getName()

		if iTeam in self.ApolloTeamsChecked:
			sString = "1: %s" % (self.ApolloTeamCheckResult[iTeam])
#			print sString
#			return self.ApolloTeamCheckResult[iTeam]

		for i in range(gc.getNumProjectInfos()):
			component = gc.getProjectInfo(i)
			if (component.isSpaceship()):
				bApollo = True
				for j in range(gc.getNumProjectInfos()):
					if(vTeam.getProjectCount(j) < component.getProjectsNeeded(j)):
						bApollo = False
#					sString = "2: %s %s %i %i %s" % (component.getDescription(), gc.getProjectInfo(j).getDescription(), vTeam.getProjectCount(j), component.getProjectsNeeded(j), bApollo)
#					print sString

#				sString = "2: %s %s" % (component.getDescription(), bApollo)
#				print sString
				if bApollo:
					self.ApolloTeamCheckResult[iTeam] = True
					self.ApolloTeamsChecked.add(iTeam)
					return True
				break

		self.ApolloTeamCheckResult[iTeam] = False
		self.ApolloTeamsChecked.add(iTeam)
		return False
# BUG Additions End

	# returns a unique ID for a widget in this screen
	def getNextWidgetName(self):
		szName = self.WIDGET_ID + str(self.nWidgetCount)
		self.nWidgetCount += 1
		return szName

	def deleteAllWidgets(self):
		screen = self.getScreen()
		i = self.nWidgetCount - 1
		while (i >= 0):
			self.nWidgetCount = i
			screen.deleteWidget(self.getNextWidgetName())
			i -= 1

		self.nWidgetCount = 0

		screen.deleteWidget(self.Vote_Pope_ID)
		screen.deleteWidget(self.Vote_DipVic_ID)
		screen.deleteWidget(self.Vote_AP_ID)
		screen.deleteWidget(self.Vote_UN_ID)
		screen.deleteWidget(self.AREA1_ID)
		screen.deleteWidget(self.AREA2_ID)
		screen.deleteWidget("Child" + self.AREA1_ID)
		screen.deleteWidget("Child" + self.AREA2_ID)

	# handle the input for this screen...
	def handleInput (self, inputClass):
		sWidget = inputClass.getFunctionName()
		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED):
			if (sWidget == self.DEBUG_DROPDOWN_ID):
				szName = self.DEBUG_DROPDOWN_ID
				self.iActivePlayer = getAdvisorDebugDropdownSelectedPlayer(self.getScreen(), szName)
				# <!-- custom: preserve the current Victory sub-tab when changing debug/vassal perspective; old code forced the first Victories tab. (GPT-5.5) -->
				self.redrawActiveTab()
		elif (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED):
			if (sWidget == self.VC_TAB_ID and self.iScreen != VICTORY_CONDITION_SCREEN):
				self.iScreen = VICTORY_CONDITION_SCREEN
				self.showVictoryConditionScreen()
			elif (sWidget == self.SETTINGS_TAB_ID and self.iScreen != GAME_SETTINGS_SCREEN):
				self.iScreen = GAME_SETTINGS_SCREEN
				self.showGameSettingsScreen()
			elif (sWidget == self.UN_RESOLUTION_TAB_ID and self.iScreen != UN_RESOLUTION_SCREEN):
				self.iScreen = UN_RESOLUTION_SCREEN
				self.showVotingScreen()
			elif (sWidget == self.UN_MEMBERS_TAB_ID and self.iScreen != UN_MEMBERS_SCREEN):
				self.iScreen = UN_MEMBERS_SCREEN
				self.showMembersScreen()
			# <advc.703>
			elif (sWidget == self.RF_SCORE_TAB_ID and self.iScreen != RF_SCORE_SCREEN):
				self.iScreen = RF_SCORE_SCREEN
				self.showRiseFall()
			# </advc.703>

			elif (sWidget == self.Vote_Pope_ID and self.VoteType == 2):
				self.VoteType = 1
				self.iScreen = UN_MEMBERS_SCREEN
				self.showMembersScreen()

			elif (sWidget == self.Vote_DipVic_ID and self.VoteType == 1):
				self.VoteType = 2
				self.iScreen = UN_MEMBERS_SCREEN
				self.showMembersScreen()

			elif (sWidget == self.Vote_AP_ID and self.VoteBody == 2):
				self.VoteBody = 1
				self.iScreen = UN_MEMBERS_SCREEN
				self.showMembersScreen()

			elif (sWidget == self.Vote_UN_ID and self.VoteBody == 1):
				self.VoteBody = 2
				self.iScreen = UN_MEMBERS_SCREEN
				self.showMembersScreen()

			elif (inputClass.getData1() == self.SPACESHIP_SCREEN_BUTTON):
				#close screen
				screen = self.getScreen()
				screen.setDying(True)
				CyInterface().clearSelectedCities()

				# <!-- custom: note: useful temporary test hook: this popup target can be rerouted to inspect/debug legacy screens (e.g., Wonder Movie) during UI upscaling/layout checks, then restored to showSpaceShip. (GPT-5.3-Codex) -->
				popupInfo = CyPopupInfo()
				popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON_SCREEN)
				popupInfo.setData1(-1)
				popupInfo.setText(u"showSpaceShip")
				popupInfo.addPopup(self.iActivePlayer)

	def update(self, fDelta):
		return
