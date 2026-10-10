## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
##
## Win-Loss code based on mod by Shelly Crawford (Fallblau on CFC/Apolyton)
## Rewritten by EmperorFool to use player's language and not break if no message is found.
## Modified by Alerum of the BUG Team to bring up to latest revision of Civilization 4: Patch 1.74.
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md).

from CvPythonExtensions import *
import BugUtil
import CvUtil
import ScreenInput
from CvScreenEnums import *
import CvReplayScreen
import CvScreensInterface
from SASFontUtils import *
import SASTextScale
import TraitUtil
import re

# globals
gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

SORT_BY_NORMALIZED_SCORE = 0
SORT_BY_FINISH_DATE = 1
SORT_BY_GAME_SCORE = 2

class CvHallOfFameScreen:
	# Top scores and more
	#
	def __init__(self, screenId):
		self.screenId = screenId
		self.SCREEN_NAME = "HallOfFameScreen"

		self.WIDGET_ID = "HallOfFameWidget"
		self.HEADER_ID = "HallOfFameHeaderWidget"
		self.EXIT_ID = "HallOfFameScreenExitWidget"
		self.REPLAY_ID = "HallOfFameScreenReplayWidget"
		self.BACKGROUND_ID = "HallOfFameScreenBackground"
		self.TABLE_ID = "HallOfFameScreenTable"
		self.LEADER_DROPDOWN_ID = "HallOfFameScreenLeaderDropdown"
		self.DIFFICULTY_DROPDOWN_ID = "HallOfFameScreenDifficultyDropdown"
		self.MAPSIZE_DROPDOWN_ID = "HallOfFameScreenMapsizeDropdown"
		self.CLIMATE_DROPDOWN_ID = "HallOfFameScreenClimateDropdown"
		self.SEALEVEL_DROPDOWN_ID = "HallOfFameScreenSealevelDropdown"
		self.ERA_DROPDOWN_ID = "HallOfFameScreenEraDropdown"
		self.SPEED_DROPDOWN_ID = "HallOfFameScreenSpeedDropdown"
		self.VICTORY_DROPDOWN_ID = "HallOfFameScreenVictoryDropdown"
		self.MULTIPLAYER_DROPDOWN_ID = "HallOfFameScreenMultiplayerDropdown"
		self.SORT_DROPDOWN_ID = "HallOfFameScreenSortDropdown"
		self.REPLAY_BUTTON_ID = "HallOfFameReplayButton"

		self.X_SCREEN = 500
		#self.Y_SCREEN = 396 advc: unused
		self.W_SCREEN = 1024
		self.H_SCREEN = 768
		self.Y_TITLE = 12

		self.X_EXIT = 994
		self.Y_EXIT = 726

		self.DROPDOWN_WIDTH = 200
		self.DROPDOWN_Y = 70
		self.DROPDOWN_SPACING_X = 45
		self.DROPDOWN_SPACING_Y = 50
		self.DROPDOWN_START_X = self.DROPDOWN_SPACING_X
		self.TABLE_X = 2
		self.TABLE_Y = 2 * self.DROPDOWN_SPACING_Y + self.DROPDOWN_Y
		self.TABLE_W = 1018
		self.TABLE_H = 545
		self.TOP_BOTTOM_PANEL_H = 55
		self.HOF_ICON_SIZE = 24
		self.HOF_COLOR_MARKER = u"|||||"

		self.nWidgetCount = 0
		self.infoList = [] # K-Mod

		self.bAllowReplay = False

	def getScreen(self):
		return CyGInterfaceScreen(self.SCREEN_NAME, self.screenId)

	def hideScreen(self):
		screen = self.getScreen()
		screen.hideScreen()									

	def updateLayoutForResolution(self, screen):
		# <!-- custom: Hall of Fame now uses replay-style fullscreen layout so table/dropdowns scale with resolution and reduce wasted margins at widescreen. (GPT-5.3-Codex) -->
		self.W_SCREEN = screen.getXResolution()
		self.H_SCREEN = screen.getYResolution()
		self.X_SCREEN = self.W_SCREEN / 2
		self.X_EXIT = self.W_SCREEN - 30
		self.Y_EXIT = self.H_SCREEN - 42
		self.DROPDOWN_WIDTH = max(200, min(240, self.W_SCREEN / 6))
		self.DROPDOWN_SPACING_X = max(20, self.W_SCREEN / 60)
		self.DROPDOWN_SPACING_Y = 50
		iDropDownColumns = 4
		iDropDownTotalW = iDropDownColumns * self.DROPDOWN_WIDTH + (iDropDownColumns - 1) * self.DROPDOWN_SPACING_X
		self.DROPDOWN_START_X = (self.W_SCREEN - iDropDownTotalW) / 2
		self.TABLE_X = 2
		self.TABLE_Y = 2 * self.DROPDOWN_SPACING_Y + self.DROPDOWN_Y
		self.TABLE_W = self.W_SCREEN - 2 * self.TABLE_X
		self.TABLE_H = self.H_SCREEN - self.TABLE_Y - self.TOP_BOTTOM_PANEL_H - 2
		if self.TABLE_H < 250:
			self.TABLE_H = 250

	def getDropdownPos(self, iNumDropDowns):
		iRow = iNumDropDowns % 2
		iColumn = iNumDropDowns / 2
		xDropDown = self.DROPDOWN_START_X + iColumn * (self.DROPDOWN_WIDTH + self.DROPDOWN_SPACING_X)
		yDropDown = self.DROPDOWN_Y + iRow * self.DROPDOWN_SPACING_Y
		return xDropDown, yDropDown

	# Screen construction function
	def interfaceScreen(self, bAllowReplay):

		# Create a new screen
		screen = self.getScreen()
		if screen.isActive():
			return
		screen.setRenderInterfaceOnly(True)
		screen.showScreen(PopupStates.POPUPSTATE_IMMEDIATE, False)
		screen.setAlwaysShown(True)
		self.updateLayoutForResolution(screen)

		self.bAllowReplay = bAllowReplay
		self.iLeaderFilter = -1
		self.iHandicapFilter = -1
		self.iWorldFilter = -1
		self.iClimateFilter = -1
		self.iSeaLevelFilter = -1
		self.iEraFilter = -1
		self.iSpeedFilter = -1
		self.iVictoryFilter = -1
		if gc.getGame().isGameMultiPlayer():
			self.iMultiplayerFilter = 1
		else:
			self.iMultiplayerFilter = 0
		self.iSortBy = SORT_BY_NORMALIZED_SCORE

		self.EXIT_TEXT = sasFontTagTitle + localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT", ()).upper() + SAS_FONT_TAG_CLOSE
		# <!-- custom: initialize TraitUtil here because doing it in __init__ broke Civ startup;
		# by the time Hall of Fame opens, the game/font symbols are ready and trait icons render correctly. (GPT-5.5) -->
		TraitUtil.init()

		self.hallOfFame = CyHallOfFameInfo()
		self.hallOfFame.loadReplays()

		# Set the background widget and exit button
		screen.setDimensions(0, 0, self.W_SCREEN, self.H_SCREEN)
		screen.addDDSGFC(self.BACKGROUND_ID, ArtFileMgr.getInterfaceArtInfo("SCREEN_BG_OPAQUE").getPath(), 0, 0, self.W_SCREEN, self.H_SCREEN, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		screen.addPanel( "TechTopPanel", u"", u"", True, False, 0, 0, self.W_SCREEN, self.TOP_BOTTOM_PANEL_H, PanelStyles.PANEL_STYLE_TOPBAR )
		screen.addPanel( "TechBottomPanel", u"", u"", True, False, 0, self.H_SCREEN - self.TOP_BOTTOM_PANEL_H, self.W_SCREEN, self.TOP_BOTTOM_PANEL_H, PanelStyles.PANEL_STYLE_BOTTOMBAR )
		screen.showWindowBackground(False)
		screen.setText(self.EXIT_ID, "", self.EXIT_TEXT, CvUtil.FONT_RIGHT_JUSTIFY, self.X_EXIT, self.Y_EXIT, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		# Header...
		screen.setLabel(self.HEADER_ID, "Background", sasFontTagTitle.bold + localText.getText("TXT_KEY_HALL_OF_FAME_SCREEN_TITLE", ()).upper() + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, self.X_SCREEN, self.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		xDropDown, yDropDown = self.getDropdownPos(0)
		iNumDropDowns = 0

		# Leader dropdown initialization
		screen.addDropDownBoxGFC(self.LEADER_DROPDOWN_ID, xDropDown, yDropDown, self.DROPDOWN_WIDTH, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		screen.addPullDownString(self.LEADER_DROPDOWN_ID, localText.getText("TXT_KEY_ALL_LEADERS", ()), -1, -1, True)
		for iCiv in range(gc.getNumCivilizationInfos()):
			civ = gc.getCivilizationInfo(iCiv)
			if civ.isPlayable():
				for iLeader in range(gc.getNumLeaderHeadInfos()):
					if civ.isLeaders(iLeader):
						screen.addPullDownString(self.LEADER_DROPDOWN_ID, gc.getLeaderHeadInfo(iLeader).getDescription(), iCiv, iLeader, False)
		iNumDropDowns += 1

		xDropDown, yDropDown = self.getDropdownPos(iNumDropDowns)

		# Victory dropdown initialization
		screen.addDropDownBoxGFC(self.VICTORY_DROPDOWN_ID, xDropDown, yDropDown, self.DROPDOWN_WIDTH, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		screen.addPullDownString(self.VICTORY_DROPDOWN_ID, localText.getText("TXT_KEY_ALL_VICTORIES", ()), -1, -1, True)
		for i in range(gc.getNumVictoryInfos()):
			screen.addPullDownString(self.VICTORY_DROPDOWN_ID, gc.getVictoryInfo(i).getDescription(), i, i, False)
		iNumDropDowns += 1

		xDropDown, yDropDown = self.getDropdownPos(iNumDropDowns)

		# Difficulty level dropdown initialization
		screen.addDropDownBoxGFC(self.DIFFICULTY_DROPDOWN_ID, xDropDown, yDropDown, self.DROPDOWN_WIDTH, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		screen.addPullDownString(self.DIFFICULTY_DROPDOWN_ID, localText.getText("TXT_KEY_ALL_DIFFICULTIES", ()), -1, -1, True)
		for iHandicap in range(gc.getNumHandicapInfos()):
			screen.addPullDownString(self.DIFFICULTY_DROPDOWN_ID, gc.getHandicapInfo(iHandicap).getDescription(), iHandicap, iHandicap, False)
		iNumDropDowns += 1

		xDropDown, yDropDown = self.getDropdownPos(iNumDropDowns)

		# World Size dropdown initialization
		screen.addDropDownBoxGFC(self.MAPSIZE_DROPDOWN_ID, xDropDown, yDropDown, self.DROPDOWN_WIDTH, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		screen.addPullDownString(self.MAPSIZE_DROPDOWN_ID, localText.getText("TXT_KEY_ALL_WORLD_SIZES", ()), -1, -1, True)
		for i in range(gc.getNumWorldInfos()):
			screen.addPullDownString(self.MAPSIZE_DROPDOWN_ID, gc.getWorldInfo(i).getDescription(), i, i, False)
		iNumDropDowns += 1

		xDropDown, yDropDown = self.getDropdownPos(iNumDropDowns)

		# Era dropdown initialization
		screen.addDropDownBoxGFC(self.ERA_DROPDOWN_ID, xDropDown, yDropDown, self.DROPDOWN_WIDTH, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		screen.addPullDownString(self.ERA_DROPDOWN_ID, localText.getText("TXT_KEY_ALL_ERAS", ()), -1, -1, True)
		for i in range(gc.getNumEraInfos()):
			screen.addPullDownString(self.ERA_DROPDOWN_ID, gc.getEraInfo(i).getDescription(), i, i, False)
		iNumDropDowns += 1

		xDropDown, yDropDown = self.getDropdownPos(iNumDropDowns)

		# Game Speed dropdown initialization
		screen.addDropDownBoxGFC(self.SPEED_DROPDOWN_ID, xDropDown, yDropDown, self.DROPDOWN_WIDTH, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		screen.addPullDownString(self.SPEED_DROPDOWN_ID, localText.getText("TXT_KEY_ALL_GAME_SPEEDS", ()), -1, -1, True)
		for i in range(gc.getNumGameSpeedInfos()):
			screen.addPullDownString(self.SPEED_DROPDOWN_ID, gc.getGameSpeedInfo(i).getDescription(), i, i, False)
		iNumDropDowns += 1

		xDropDown, yDropDown = self.getDropdownPos(iNumDropDowns)

		# Climate dropdown initialization
		#screen.addDropDownBoxGFC(self.CLIMATE_DROPDOWN_ID, xDropDown, yDropDown, self.DROPDOWN_WIDTH, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		#screen.addPullDownString(self.CLIMATE_DROPDOWN_ID, localText.getText("TXT_KEY_ALL_CLIMATES", ()), -1, -1, True)
		#for i in range(gc.getNumClimateInfos()):
		#	screen.addPullDownString(self.CLIMATE_DROPDOWN_ID, gc.getClimateInfo(i).getDescription(), i, i, False)
		#iNumDropDowns += 1

		#yDropDown = self.DROPDOWN_SPACING_Y * (iNumDropDowns % 2) + self.DROPDOWN_Y
		#xDropDown = (self.DROPDOWN_WIDTH  + self.DROPDOWN_SPACING_X) * (iNumDropDowns / 2) + self.DROPDOWN_SPACING_X

		# Sea Level dropdown initialization
		#screen.addDropDownBoxGFC(self.SEALEVEL_DROPDOWN_ID, xDropDown, yDropDown, self.DROPDOWN_WIDTH, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		#screen.addPullDownString(self.SEALEVEL_DROPDOWN_ID, localText.getText("TXT_KEY_ALL_SEA_LEVELS", ()), -1, -1, True)
		#for i in range(gc.getNumSeaLevelInfos()):
		#	screen.addPullDownString(self.SEALEVEL_DROPDOWN_ID, gc.getSeaLevelInfo(i).getDescription(), i, i, False)
		#iNumDropDowns += 1

		xDropDown, yDropDown = self.getDropdownPos(iNumDropDowns)

		# Multiplayer dropdown initialization
		screen.addDropDownBoxGFC(self.MULTIPLAYER_DROPDOWN_ID, xDropDown, yDropDown, self.DROPDOWN_WIDTH, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		if self.iMultiplayerFilter == 1:
			screen.addPullDownString(self.MULTIPLAYER_DROPDOWN_ID, localText.getText("TXT_KEY_MAIN_MENU_MULTIPLAYER", ()), 1, 1, True)
			screen.addPullDownString(self.MULTIPLAYER_DROPDOWN_ID, localText.getText("TXT_KEY_MAIN_MENU_SINGLE_PLAYER", ()), 0, 0, False)
		else:
			screen.addPullDownString(self.MULTIPLAYER_DROPDOWN_ID, localText.getText("TXT_KEY_MAIN_MENU_SINGLE_PLAYER", ()), 0, 0, True)
			screen.addPullDownString(self.MULTIPLAYER_DROPDOWN_ID, localText.getText("TXT_KEY_MAIN_MENU_MULTIPLAYER", ()), 1, 1, False)
		iNumDropDowns += 1

		xDropDown, yDropDown = self.getDropdownPos(iNumDropDowns)

		# Score dropdown initialization
		screen.addDropDownBoxGFC(self.SORT_DROPDOWN_ID, xDropDown, yDropDown, self.DROPDOWN_WIDTH, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		screen.addPullDownString(self.SORT_DROPDOWN_ID, localText.getText("TXT_KEY_HALL_OF_FAME_SORT_BY_NORMALIZED_SCORE", ()), SORT_BY_NORMALIZED_SCORE, SORT_BY_NORMALIZED_SCORE, True)
		screen.addPullDownString(self.SORT_DROPDOWN_ID, localText.getText("TXT_KEY_HALL_OF_FAME_SORT_BY_DATE", ()), SORT_BY_FINISH_DATE, SORT_BY_FINISH_DATE, False)
		screen.addPullDownString(self.SORT_DROPDOWN_ID, localText.getText("TXT_KEY_HALL_OF_FAME_SORT_BY_GAME_SCORE", ()), SORT_BY_GAME_SCORE, SORT_BY_GAME_SCORE, False)
		iNumDropDowns += 1

		self.drawContents()

	def isDisplayed(self, replayInfo):
		return ((self.iLeaderFilter == -1 or self.iLeaderFilter == replayInfo.getLeader(replayInfo.getActivePlayer())) and (self.iHandicapFilter == -1 or self.iHandicapFilter == replayInfo.getDifficulty()) and (self.iWorldFilter == -1 or self.iWorldFilter == replayInfo.getWorldSize()) and (self.iClimateFilter == -1 or self.iClimateFilter == replayInfo.getClimate()) and (self.iSeaLevelFilter == -1 or self.iSeaLevelFilter == replayInfo.getSeaLevel()) and (self.iEraFilter == -1 or self.iEraFilter == replayInfo.getEra()) and (self.iSpeedFilter == -1 or self.iSpeedFilter == replayInfo.getGameSpeed()) and (self.iVictoryFilter == -1 or self.iVictoryFilter == replayInfo.getVictoryType()) and ((self.iMultiplayerFilter == 1) == replayInfo.isMultiplayer()))

	def getReplayCivType(self, replayInfo):
		# <!-- custom: Hall of Fame identity columns mirror the Info Screen Score Tab: leader icon, civ icon, player color marker, leader/civ name, then trait icons.
		# CyReplayInfo stores leader id and player color directly, so those survive custom leader names.
		# It does not store civilization id, only civ name strings; match them back to XML for the civ icon, and let custom civ names return -1 for no unreliable icon. (GPT-5.5) -->
		szShort = replayInfo.getShortCivDescription()
		szFull = replayInfo.getCivDescription()
		szAdjective = replayInfo.getCivAdjective()
		for iCiv in range(gc.getNumCivilizationInfos()):
			civInfo = gc.getCivilizationInfo(iCiv)
			if (szShort == civInfo.getShortDescription(0) or szFull == civInfo.getDescription() or szAdjective == civInfo.getAdjective(0)):
				return iCiv
		return -1

	def getReplayColorMarker(self, replayInfo):
		iColor = replayInfo.getColor(replayInfo.getActivePlayer())
		if iColor >= 0:
			return localText.changeTextColor(self.HOF_COLOR_MARKER, iColor)
		return self.HOF_COLOR_MARKER

	def getLeaderTraitIcons(self, iLeader):
		szTrait1 = u""
		szTrait2 = u""
		if iLeader >= 0 and iLeader < gc.getNumLeaderHeadInfos():
			leaderInfo = gc.getLeaderHeadInfo(iLeader)
			for iTrait in range(gc.getNumTraitInfos()):
				if leaderInfo.hasTrait(iTrait):
					if szTrait1 == u"":
						szTrait1 = TraitUtil.getIcon(iTrait)
					elif szTrait2 == u"":
						szTrait2 = TraitUtil.getIcon(iTrait)
						break
		return szTrait1, szTrait2

	def drawContents(self):

		screen = self.getScreen()

		# K-Mod. Delete old widgets.
		for i in range(len(self.infoList)):
			szButtonName = self.REPLAY_BUTTON_ID + str(i)
			screen.deleteWidget(szButtonName)
		screen.deleteWidget(self.TABLE_ID)
		# K-Mod end

		screen.addTableControlGFC(self.TABLE_ID, 15, self.TABLE_X, self.TABLE_Y, self.TABLE_W, self.TABLE_H, True, True, self.HOF_ICON_SIZE, self.HOF_ICON_SIZE, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSelect(self.TABLE_ID, False)
		screen.enableSort(self.TABLE_ID)
		# <advc.106i> Don't show replay button column when game has just ended;
		# replays don't work at that point. (Why is that? Issue introduced by AdvCiv? Oh well ...)
		bAllowReplay = (gc.getGame().getGameState() != GameStateTypes.GAMESTATE_OVER)
		iColumn = 0 # Replacing hardcoded column numbers
		iReplayButtonColumnW = 20
		iIconColumnW = 34
		iColorColumnW = 45
		iTraitColumnW = 44
		# <!-- custom: with fullscreen Hall of Fame we have more lateral room, so expand narrow columns when needed (e.g., Finished/Speed) and let Leader absorb the remaining width. (GPT-5.3-Codex) -->
		iNormalizedScoreW = 140
		iDateW = 115
		iGameScoreW = 100
		iVictoryW = 140
		iDifficultyW = 150
		iWorldSizeW = 125
		iEraW = 125
		iSpeedW = 250
		iNonLeaderTotalW = (iNormalizedScoreW + iDateW + iGameScoreW + iVictoryW + iDifficultyW + iWorldSizeW + iEraW + iSpeedW)
		iLeaderColumnW = self.TABLE_W - iNonLeaderTotalW - (2 * iIconColumnW) - iColorColumnW - (2 * iTraitColumnW) - 12
		if bAllowReplay:
			iLeaderColumnW -= iReplayButtonColumnW
		if iLeaderColumnW < 202:
			iLeaderColumnW = 202
		if bAllowReplay:
			SASTextScale.setTableColumnHeaderLabel(screen, self.TABLE_ID, iColumn, "", iReplayButtonColumnW)
			iColumn += 1 # </advc.106i>
		SASTextScale.setTableColumnHeaderLabel(screen, self.TABLE_ID, iColumn, "", iIconColumnW)
		iColumn += 1
		SASTextScale.setTableColumnHeaderLabel(screen, self.TABLE_ID, iColumn, "", iIconColumnW)
		iColumn += 1
		SASTextScale.setTableColumnHeaderLabel(screen, self.TABLE_ID, iColumn, u"Col", iColorColumnW)
		iColumn += 1
		SASTextScale.setTableColumnHeaderLabel(screen, self.TABLE_ID, iColumn, localText.getText("TXT_KEY_PITBOSS_LEADER", ()), iLeaderColumnW)
		iColumn += 1
		SASTextScale.setTableColumnHeaderLabel(screen, self.TABLE_ID, iColumn, u"T1", iTraitColumnW)
		iColumn += 1
		SASTextScale.setTableColumnHeaderLabel(screen, self.TABLE_ID, iColumn, u"T2", iTraitColumnW)
		iColumn += 1
		SASTextScale.setTableColumnHeaderLabel(screen, self.TABLE_ID, iColumn, localText.getText("TXT_KEY_NORMALIZED_SCORE", ()), iNormalizedScoreW)
		iColumn += 1
		SASTextScale.setTableColumnHeaderLabel(screen, self.TABLE_ID, iColumn, localText.getText("TXT_KEY_HALL_OF_FAME_SORT_BY_DATE", ()), iDateW)
		iColumn += 1
		SASTextScale.setTableColumnHeaderLabel(screen, self.TABLE_ID, iColumn, localText.getText("TXT_KEY_GAME_SCORE", ()), iGameScoreW)
		iColumn += 1
		SASTextScale.setTableColumnHeaderLabel(screen, self.TABLE_ID, iColumn, localText.getText("TXT_KEY_CONCEPT_VICTORY", ()), iVictoryW)
		iColumn += 1
		SASTextScale.setTableColumnHeaderLabel(screen, self.TABLE_ID, iColumn, localText.getText("TXT_KEY_PITBOSS_DIFFICULTY", ()), iDifficultyW)
		iColumn += 1
		SASTextScale.setTableColumnHeaderLabel(screen, self.TABLE_ID, iColumn, localText.getText("TXT_KEY_HOF_SCREEN_SIZE", ()), iWorldSizeW)
		iColumn += 1
		SASTextScale.setTableColumnHeaderLabel(screen, self.TABLE_ID, iColumn, localText.getText("TXT_KEY_HOF_SCREEN_STARTING_ERA", ()), iEraW)
		iColumn += 1
		SASTextScale.setTableColumnHeaderLabel(screen, self.TABLE_ID, iColumn, localText.getText("TXT_KEY_HOF_SCREEN_GAME_SPEED", ()), iSpeedW)
#		screen.setTableColumnHeader(self.TABLE_ID,, "", 73)

		# count the filtered replays
		iNumGames = 0
		for i in range(self.hallOfFame.getNumGames()):
			replayInfo = self.hallOfFame.getReplayInfo(i)
			if self.isDisplayed(replayInfo):
				iNumGames += 1

		self.infoList = [(-1,"",-1,"",-1,"","","","","",0,-1,-1,"","","")] * iNumGames
		iItem = 0
		for i in range(self.hallOfFame.getNumGames()):
			replayInfo = self.hallOfFame.getReplayInfo(i)
			if self.isDisplayed(replayInfo):
				szUnknown = localText.getText("TXT_KEY_UNKNOWN", ()) # advc.106i
				szVictory = u""
				if replayInfo.getVictoryType() <= 0:
					szVictory = localText.getText("TXT_KEY_NONE", ())
				# <advc.106i>
				elif replayInfo.getVictoryType() >= gc.getNumVictoryInfos():
					szVictory = szUnknown # </advc.106i>
				else:
					szVictory = gc.getVictoryInfo(replayInfo.getVictoryType()).getDescription()
# BUG - Win/Loss Info - start
				results = {
					True: localText.getText("TXT_KEY_HOF_WINLOSS_WIN", ()),
					False: localText.getText("TXT_KEY_HOF_WINLOSS_LOSS", ())
				}
				win, szType = self.isReplayWinner(replayInfo)
				szVictory = results[win] + szType
# BUG - Win/Loss Info - end

				if self.iSortBy == SORT_BY_NORMALIZED_SCORE:
					iValue = -replayInfo.getNormalizedScore()
				elif self.iSortBy == SORT_BY_FINISH_DATE:
					iValue = replayInfo.getFinalTurn()
				elif self.iSortBy == SORT_BY_GAME_SCORE:
					iValue = -replayInfo.getFinalScore()
				# <advc.106i>  (also legacy support for advc.250a)
				iHandicap = replayInfo.getDifficulty()
				if iHandicap >= 0 and iHandicap < gc.getNumHandicapInfos():
					szHandicap = gc.getHandicapInfo(iHandicap).getDescription()
				else:
					szHandicap = szUnknown
				iWorldSize = replayInfo.getWorldSize()
				if iWorldSize >= 0 and iWorldSize < gc.getNumWorldInfos():
					szWorldSize = gc.getWorldInfo(iWorldSize).getDescription()
				else:
					szWorldSize = szUnknown
				iStartEra = replayInfo.getEra()
				if iStartEra >= 0 and iStartEra < gc.getNumEraInfos():
					szStartEra = gc.getEraInfo(iStartEra).getDescription()
				else:
					szStartEra = szUnknown
				iSpeed = replayInfo.getGameSpeed()
				if iSpeed >= 0 and iSpeed < gc.getNumGameSpeedInfos():
					szSpeed = gc.getGameSpeedInfo(iSpeed).getDescription()
				else:
					szSpeed = szUnknown
				# </advc.106i>
				szFinalDate = CyGameTextMgr().getDateStr(replayInfo.getFinalTurn(), false, replayInfo.getCalendar(), replayInfo.getStartYear(), replayInfo.getGameSpeed())
				iReplayLeader = replayInfo.getLeader(replayInfo.getActivePlayer())
				iReplayCiv = self.getReplayCivType(replayInfo)
				szReplayColor = self.getReplayColorMarker(replayInfo)
				szTrait1, szTrait2 = self.getLeaderTraitIcons(iReplayLeader)
				self.infoList[iItem] = (iValue,
						localText.getText("TXT_KEY_LEADER_CIV_DESCRIPTION", (replayInfo.getLeaderName(), replayInfo.getShortCivDescription())),
						replayInfo.getNormalizedScore(),
						szFinalDate,
						replayInfo.getFinalScore(), 
						szVictory,
						# <advc.106i>
						szHandicap,
						szWorldSize,
# Note: These two have been disabled since Vanilla Civ 4. AdvCiv no longer stores the sea level.
# gc.getClimateInfo(replayInfo.getClimate()).getDescription(),
# gc.getSeaLevelInfo(replayInfo.getSeaLevel()).getDescription(),
						szStartEra,
						szSpeed, # </advc.106i>
						i,
						iReplayLeader,
						iReplayCiv,
						szReplayColor,
						szTrait1,
						szTrait2)
				iItem += 1
		self.infoList.sort()

		for i in range(len(self.infoList)):
			if bAllowReplay: # advc.106i
				szButtonName = self.REPLAY_BUTTON_ID + str(i)
				# <advc.106i>
				data1 = 0
				if i > 22:
					data1 = 1
				# </advc.106i>
				screen.setButtonGFC(szButtonName, self.infoList[i][1], "", 0, 0, 10, 10, WidgetTypes.WIDGET_SHOW_REPLAY, data1, -1, ButtonStyles.BUTTON_STYLE_STANDARD)

			screen.appendTableRow(self.TABLE_ID)
			# <advc.106i> Replacing hardcoded column numbers
			iColumn = 0
			if bAllowReplay: 
				# Moved up:
				screen.attachControlToTableCell(szButtonName, self.TABLE_ID, i, iColumn)
				iColumn += 1
			# </advc.106i>
			iReplayLeader = self.infoList[i][11]
			iReplayCiv = self.infoList[i][12]
			if iReplayLeader >= 0 and iReplayLeader < gc.getNumLeaderHeadInfos():
				# <!-- custom: Hall-of-Fame replay rows retain the matching civilization; use it for leader-hover trait replacements instead of literal civilization ID 1 (Arabia). See KI#332. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				SASTextScale.setTableTextLabel(screen, self.TABLE_ID, iColumn, i, "", gc.getLeaderHeadInfo(iReplayLeader).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_LEADER, iReplayLeader, iReplayCiv, CvUtil.FONT_CENTER_JUSTIFY)
			iColumn += 1
			if iReplayCiv >= 0 and iReplayCiv < gc.getNumCivilizationInfos():
				SASTextScale.setTableTextLabel(screen, self.TABLE_ID, iColumn, i, "", gc.getCivilizationInfo(iReplayCiv).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIV, iReplayCiv, -1, CvUtil.FONT_CENTER_JUSTIFY)
			iColumn += 1
			SASTextScale.setTableTextLabel(screen, self.TABLE_ID, iColumn, i, self.infoList[i][13], "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
			iColumn += 1
			SASTextScale.setTableTextLabel(screen, self.TABLE_ID, iColumn, i, self.infoList[i][1], "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			iColumn += 1
			SASTextScale.setTableTextLabel(screen, self.TABLE_ID, iColumn, i, self.infoList[i][14], "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
			iColumn += 1
			SASTextScale.setTableTextLabel(screen, self.TABLE_ID, iColumn, i, self.infoList[i][15], "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
			iColumn += 1
			if self.infoList[i][2] >= 0:
				SASTextScale.setTableIntLabel(screen, self.TABLE_ID, iColumn, i, u"%d" % self.infoList[i][2], "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
			iColumn += 1
			# <!-- custom: The Hall-of-Fame font-upscaling pass changed this date-typed cell to generic text, so clicking the Date header sorted formatted dates lexically.
			# Keep the shared font through setTableDateLabel without losing Civ4's dedicated date sorting. See KI#229. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			SASTextScale.setTableDateLabel(screen, self.TABLE_ID, iColumn, i, self.infoList[i][3], "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			iColumn += 1
			if self.infoList[i][4] >= 0:
				SASTextScale.setTableIntLabel(screen, self.TABLE_ID, iColumn, i, u"%d" % self.infoList[i][4], "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
			iColumn += 1
			SASTextScale.setTableTextLabel(screen, self.TABLE_ID, iColumn, i, self.infoList[i][5], "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			iColumn += 1
			SASTextScale.setTableTextLabel(screen, self.TABLE_ID, iColumn, i, self.infoList[i][6], "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			iColumn += 1
			SASTextScale.setTableTextLabel(screen, self.TABLE_ID, iColumn, i, self.infoList[i][7], "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			iColumn += 1
			SASTextScale.setTableTextLabel(screen, self.TABLE_ID, iColumn, i, self.infoList[i][8], "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			iColumn += 1
			SASTextScale.setTableTextLabel(screen, self.TABLE_ID, iColumn, i, self.infoList[i][9], "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		return

# BUG - Win/Loss Info - start
	def isReplayWinner(self, replay):
		szWinText = localText.getText("TXT_KEY_GAME_WON", ("(.*)", "(.*)"))
		reWinText = re.compile(szWinText)
		leaderGroup = 1
		typeGroup = 2
		leader = replay.getLeaderName()
		msgNum = replay.getNumReplayMessages() - 1
		BugUtil.debug("scanning replay for %s with %d msgs", leader, msgNum + 1)
		count = 0
		while msgNum >= 0:
			msg = replay.getReplayMessageText(msgNum)
			if count > 25: # advc.opt: was 100
				BugUtil.debug("no victory message found; skipping")
				break
			matches = reWinText.match(msg)
			if matches:
				type = matches.group(typeGroup)
				if leader in matches.group(leaderGroup).split('/'):
					BugUtil.info("replay: %s win for %s", type, leader)
					return True, type
				BugUtil.info("replay: %s loss for %s", type, leader)
				return False, type
			msgNum -= 1
		return False, localText.getText("TXT_KEY_NONE", ())
# BUG - Win/Loss Info - end

	# handle the input for this screen...
	def handleInput (self, inputClass):
		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED):
			if (inputClass.getFunctionName() == self.LEADER_DROPDOWN_ID):			
				screen = self.getScreen()
				iIndex = screen.getSelectedPullDownID(self.LEADER_DROPDOWN_ID)
				self.iLeaderFilter = screen.getPullDownData(self.LEADER_DROPDOWN_ID, iIndex)
				self.drawContents()
			elif (inputClass.getFunctionName() == self.DIFFICULTY_DROPDOWN_ID):			
				screen = self.getScreen()
				iIndex = screen.getSelectedPullDownID(self.DIFFICULTY_DROPDOWN_ID)
				self.iHandicapFilter = screen.getPullDownData(self.DIFFICULTY_DROPDOWN_ID, iIndex)
				self.drawContents()
			elif (inputClass.getFunctionName() == self.MAPSIZE_DROPDOWN_ID):			
				screen = self.getScreen()
				iIndex = screen.getSelectedPullDownID(self.MAPSIZE_DROPDOWN_ID)
				self.iWorldFilter = screen.getPullDownData(self.MAPSIZE_DROPDOWN_ID, iIndex)
				self.drawContents()
			elif (inputClass.getFunctionName() == self.CLIMATE_DROPDOWN_ID):			
				screen = self.getScreen()
				iIndex = screen.getSelectedPullDownID(self.CLIMATE_DROPDOWN_ID)
				self.iClimateFilter = screen.getPullDownData(self.CLIMATE_DROPDOWN_ID, iIndex)
				self.drawContents()
			elif (inputClass.getFunctionName() == self.SEALEVEL_DROPDOWN_ID):			
				screen = self.getScreen()
				iIndex = screen.getSelectedPullDownID(self.SEALEVEL_DROPDOWN_ID)
				self.iSeaLevelFilter = screen.getPullDownData(self.SEALEVEL_DROPDOWN_ID, iIndex)
				self.drawContents()
			elif (inputClass.getFunctionName() == self.ERA_DROPDOWN_ID):			
				screen = self.getScreen()
				iIndex = screen.getSelectedPullDownID(self.ERA_DROPDOWN_ID)
				self.iEraFilter = screen.getPullDownData(self.ERA_DROPDOWN_ID, iIndex)
				self.drawContents()
			elif (inputClass.getFunctionName() == self.SPEED_DROPDOWN_ID):			
				screen = self.getScreen()
				iIndex = screen.getSelectedPullDownID(self.SPEED_DROPDOWN_ID)
				self.iSpeedFilter = screen.getPullDownData(self.SPEED_DROPDOWN_ID, iIndex)
				self.drawContents()
			elif (inputClass.getFunctionName() == self.VICTORY_DROPDOWN_ID):			
				screen = self.getScreen()
				iIndex = screen.getSelectedPullDownID(self.VICTORY_DROPDOWN_ID)
				self.iVictoryFilter = screen.getPullDownData(self.VICTORY_DROPDOWN_ID, iIndex)
				self.drawContents()
			elif (inputClass.getFunctionName() == self.MULTIPLAYER_DROPDOWN_ID):			
				screen = self.getScreen()
				iIndex = screen.getSelectedPullDownID(self.MULTIPLAYER_DROPDOWN_ID)
				self.iMultiplayerFilter = screen.getPullDownData(self.MULTIPLAYER_DROPDOWN_ID, iIndex)
				self.drawContents()
			elif (inputClass.getFunctionName() == self.SORT_DROPDOWN_ID):			
				screen = self.getScreen()
				iIndex = screen.getSelectedPullDownID(self.SORT_DROPDOWN_ID)
				self.iSortBy = screen.getPullDownData(self.SORT_DROPDOWN_ID, iIndex)
				self.drawContents()
		elif (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED):
			if (inputClass.getFunctionName() == self.EXIT_ID):
				screen = self.getScreen()
				screen.hideScreen()
			elif (inputClass.getFunctionName() == self.REPLAY_BUTTON_ID and self.bAllowReplay):
				iRow = inputClass.getID()
				if iRow < len(self.infoList):
					CvScreensInterface.replayScreen.replayInfo = self.hallOfFame.getReplayInfo(self.infoList[iRow][10])
					CvScreensInterface.replayScreen.showScreen(True)

		return 0

	def update(self, fDelta):
		return					

