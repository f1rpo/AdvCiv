# Sid Meier's Civilization 4
# Copyright Firaxis Games 2005

#
# Sevopedia 2.3
#   sevotastic.blogspot.com
#   sevotastic@yahoo.com
#
# additional work by Gaurav, Progor, Ket, Vovan, Fitchn, LunarMongoose
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#

from CvPythonExtensions import *
from SASMagicNumbers import *
import CvUtil
import ScreenInput
# <!-- custom: Removed the unused SevoScreenEnums import after its constants moved from SevoScreenEnums.py to the SevoScreenEnums class in CvScreenEnums.py.
# This page uses none of those IDs, and the removed constants-only module had no initialization side effects. (GPT-6.1-Sol) -->
import SASTextScale

from _sevopedia_helpers import *

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class SevoPediaCivic:

	def __init__(self, main):
		self.iCivic = -1
		self.bHistoryExpanded = False
		self.top = main

		# <!-- custom: multilist favorites panel width from helper so callers can switch button count per row without redoing panel math. (GPT-5.3-Codex) -->
		self.W_FAVORITES = get_multilist_panel_width_for_buttons(4, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_MULTI_LIST_LEFT_EDGE_PADDING, HYPOTHESIZED_MULTI_LIST_RIGHT_EDGE_PADDING, HYPOTHESIZED_MULTI_LIST_INTER_BUTTON_SPACING)
		self.X_FAVORITES = self.top.R_PEDIA_PAGE - self.W_FAVORITES
		self.Y_FAVORITES = self.top.Y_PEDIA_PAGE
		self.H_FAVORITES = self.top.B_PEDIA_PAGE - self.Y_FAVORITES

		self.X_CIVIC_PANE = self.top.X_PEDIA_PAGE
		self.Y_CIVIC_PANE = self.top.Y_PEDIA_PAGE
		self.W_CIVIC_PANE = 440
		self.H_CIVIC_PANE = 230

		self.W_ICON = 125
		self.H_ICON = 125
		self.X_ICON = self.X_CIVIC_PANE + (self.H_CIVIC_PANE - self.H_ICON) / 2
		self.Y_ICON = self.Y_CIVIC_PANE + (self.H_CIVIC_PANE - self.H_ICON) / 2

		self.X_STATS = self.X_ICON + self.W_ICON 
		self.Y_STATS = 141
		self.W_STATS = 250
		self.H_STATS = 200

		# <!-- custom: left-to-right civic page layout:
		# civic pane -> effects -> requires (top) / vote source + force votes (bottom) -> favorites.
		# Effects width is computed from the remaining horizontal space after placing right-side panels. (GPT-5.4?) -->
		self.X_SPECIAL = self.X_CIVIC_PANE + self.W_CIVIC_PANE + MEDIUM_MARGIN

		self.W_REQUIRES = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.W_VOTE_SOURCE = self.W_REQUIRES
		self.W_VOTES = self.W_REQUIRES
		xRightEdgeBeforeFavorites = self.X_FAVORITES - MEDIUM_MARGIN
		self.W_RIGHT_BLOCK = self.W_REQUIRES + MEDIUM_MARGIN + self.W_VOTE_SOURCE
		self.X_REQUIRES = xRightEdgeBeforeFavorites - self.W_RIGHT_BLOCK
		self.X_VOTE_SOURCE = self.X_REQUIRES + self.W_REQUIRES + MEDIUM_MARGIN
		self.X_VOTES = self.X_VOTE_SOURCE
		self.W_SPECIAL = self.X_REQUIRES - MEDIUM_MARGIN - self.X_SPECIAL
		self.H_REQUIRES = NON_MULTILIST_PANEL_STANDARD_HEIGHT
		self.Y_REQUIRES = self.Y_CIVIC_PANE
		self.Y_VOTE_SOURCE = self.Y_REQUIRES
		self.H_VOTE_SOURCE = NON_MULTILIST_PANEL_STANDARD_HEIGHT
		self.Y_VOTES = self.Y_VOTE_SOURCE + self.H_VOTE_SOURCE + SMALL_MARGIN
		self.H_VOTES = NON_MULTILIST_PANEL_STANDARD_HEIGHT

		self.Y_SPECIAL = self.Y_CIVIC_PANE

		self.H_SPECIAL = self.H_CIVIC_PANE

		self.X_HISTORY = self.X_CIVIC_PANE
		self.Y_HISTORY = self.Y_CIVIC_PANE + self.H_CIVIC_PANE + SMALL_MARGIN
		self.W_HISTORY = self.top.R_PEDIA_PAGE - self.X_HISTORY - (self.W_FAVORITES + MEDIUM_MARGIN)
		self.H_HISTORY = self.top.B_PEDIA_PAGE - self.Y_HISTORY

	def interfaceScreen(self, iCivic):
		if self.iCivic != iCivic:
			self.bHistoryExpanded = False
		self.iCivic = iCivic

		self.placeFavorites()
		self.placeCivicPane()
		self.placeStats()
		self.placeSpecial()
		self.placeRequires()
		self.placeVoteSource()
		self.placeVotes()
		self.placeHistory()

	def placeFavorites(self):
		xPanel = self.X_FAVORITES
		yPanel = self.Y_FAVORITES
		wPanel = self.W_FAVORITES
		hPanel = self.H_FAVORITES

		# <!-- custom: this panel lists AI favorites for the selected civic, so "Favorites" is clearer than "Leaders". (GPT-5.3-Codex) -->
		txtKeyPanel = "TXT_KEY_PEDIA_FAVORITES"
		headerLabel = localText.getText(txtKeyPanel, ())
		numWithCivic, totalRealLeaders = get_favorite_leader_counts(FAVORITE_LEADER_TYPE_CIVIC, self.iCivic, EXCLUDED_LEADER_TYPES_FROM_SEVOPEDIA)
		headerText = format_leaders_header_text(numWithCivic, totalRealLeaders, headerLabel)

		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

		# Create panel with proper styling
		screen.addPanel(panelName, headerText, "", False, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50)

		rowListName = self.top.getNextWidgetName()

		# Create the MultiList control
		# Constants for button display
		multiListX = xPanel + MULTI_LIST_PANEL_OFFSET_X
		multiListY = yPanel + MULTI_LIST_PANEL_OFFSET_Y
		multiListW = wPanel + MULTI_LIST_PANEL_ADDITIONAL_W
		multiListH = hPanel + MULTI_LIST_PANEL_ADDITIONAL_H
		screen.addMultiListControlGFC(rowListName, "", multiListX, multiListY, multiListW, multiListH, SEVOPEDIA_MULTILIST_NUM_LISTS_AUTO_CALCULATE, MULTILIST_BUTTON_SIZE, MULTILIST_BUTTON_SIZE, TableStyles.TABLE_STYLE_STANDARD)

		# Find all leaders who have this civic as favorite
		for iLeader in xrange(gc.getNumLeaderHeadInfos()):
			leaderInfo = gc.getLeaderHeadInfo(iLeader)
			if leaderInfo.getFavoriteCivic() == self.iCivic:
				leaderInfo = gc.getLeaderHeadInfo(iLeader)
				# <!-- custom: Generic Sevopedia leader links use the Leader page's canonical unique civilization for trait production help; literal data2=1 incorrectly meant Arabia. See KI#332. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				screen.appendMultiListButton(rowListName, leaderInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_LEADER, iLeader, self.top.pediaLeader.getCiv(iLeader), False)

	def placeCivicPane(self):
		screen = self.top.getScreen()

		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_CIVIC_PANE, self.Y_CIVIC_PANE, self.W_CIVIC_PANE, self.H_CIVIC_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		# <!-- custom: no need for the blue frame on blue background, use transparent instead -->
		#screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_MAIN)
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_EMPTY)
		screen.addDDSGFC(self.top.getNextWidgetName(), gc.getCivicInfo(self.iCivic).getButton(), self.X_ICON + self.W_ICON/2 - PANE_ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - PANE_ICON_SIZE/2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def placeStats(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addListBoxGFC(panelName, "", self.X_STATS, self.Y_STATS, self.W_STATS, self.H_STATS, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(panelName, False)
		iCivicOptionType = gc.getCivicInfo(self.iCivic).getCivicOptionType()
		if (iCivicOptionType != -1):
			screen.appendListBoxString(panelName, SASTextScale.titleText(gc.getCivicOptionInfo(iCivicOptionType).getDescription().upper()), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)
		pUpkeepInfo = gc.getUpkeepInfo(gc.getCivicInfo(self.iCivic).getUpkeep())
		if (pUpkeepInfo):
			screen.appendListBoxString(panelName, SASTextScale.titleText(pUpkeepInfo.getDescription().upper()), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

	def placeSpecial(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_EFFECTS", ()), "", True, False, self.X_SPECIAL, self.Y_SPECIAL, self.W_SPECIAL, self.H_SPECIAL, PanelStyles.PANEL_STYLE_BLUE50)
		listName = self.top.getNextWidgetName()
		screen.attachListBoxGFC(panelName, listName, "", TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(listName, False)
		szSpecialText = CyGameTextMgr().parseCivicInfo(self.iCivic, True, False, True)
		if not szSpecialText or len(szSpecialText.strip()) == 0:
			txtKeyNone = "TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_NONE"
			szText = localText.getText(txtKeyNone, ())
			yPanelCenter = self.Y_SPECIAL + (self.H_SPECIAL / 2)
			screen.addMultilineText(listName, SASTextScale.labelText(szText), self.X_SPECIAL + 7, yPanelCenter, self.W_SPECIAL - 14, self.H_SPECIAL - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			return
		# <!-- custom: leave some room on top, based on placeSpecial in sevopedia terrain -->
		#screen.addMultilineText(listName, szSpecialText, self.X_SPECIAL+5, self.Y_SPECIAL+5, self.W_SPECIAL-10, self.H_SPECIAL-10, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.addMultilineText(listName, SASTextScale.labelText(szSpecialText), self.X_SPECIAL+5, self.Y_SPECIAL+10, self.W_SPECIAL-10, self.H_SPECIAL-20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeRequires(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_REQUIRES", ()), "", False, True, self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES, PanelStyles.PANEL_STYLE_BLUE50)
		screen.enableSelect(panelName, False)
		screen.attachLabel(panelName, "", "  ")
		iTech = gc.getCivicInfo(self.iCivic).getTechPrereq()
		if (iTech > -1):
			screen.attachImageButton(panelName, "", gc.getTechInfo(iTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, 1, False)
		else:
			draw_none_text(screen, self.top, self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES)

	def placeVotes(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_CIVIC_FORCE_VOTES_PANEL", ()), "", False, True, self.X_VOTES, self.Y_VOTES, self.W_VOTES, self.H_VOTES, PanelStyles.PANEL_STYLE_BLUE50)
		screen.enableSelect(panelName, False)
		screen.attachLabel(panelName, "", "  ")

		bFound = False
		for iVote in range(gc.getNumVoteInfos()):
			voteInfo = gc.getVoteInfo(iVote)
			if voteInfo and voteInfo.isForceCivic(self.iCivic):
				szButton = ""
				for iVoteSource in range(gc.getNumVoteSourceInfos()):
					if voteInfo.isVoteSourceType(iVoteSource):
						for iBuilding in range(gc.getNumBuildingInfos()):
							bi = gc.getBuildingInfo(iBuilding)
							if bi and bi.getVoteSourceType() == iVoteSource:
								szButton = bi.getButton()
								break
						break
				if szButton:
					screen.attachImageButton(panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_VOTE_ENTRY, iVote, False)
					bFound = True
				break
		if not bFound:
			draw_none_text(screen, self.top, self.X_VOTES, self.Y_VOTES, self.W_VOTES, self.H_VOTES)

	def placeVoteSource(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_VOTE_SOURCE", ()), "", False, True, self.X_VOTE_SOURCE, self.Y_VOTE_SOURCE, self.W_VOTE_SOURCE, self.H_VOTE_SOURCE, PanelStyles.PANEL_STYLE_BLUE50)
		screen.enableSelect(panelName, False)
		screen.attachLabel(panelName, "", "  ")

		bFound = False
		for iVoteSource in range(gc.getNumVoteSourceInfos()):
			srcInfo = gc.getVoteSourceInfo(iVoteSource)
			if srcInfo and srcInfo.getCivic() == self.iCivic:
				for iBuilding in range(gc.getNumBuildingInfos()):
					bi = gc.getBuildingInfo(iBuilding)
					if bi and bi.getVoteSourceType() == iVoteSource:
						screen.attachImageButton(panelName, "", bi.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, 1, False)
						bFound = True
						break
				if bFound:
					break
		if not bFound:
			draw_none_text(screen, self.top, self.X_VOTE_SOURCE, self.Y_VOTE_SOURCE, self.W_VOTE_SOURCE, self.H_VOTE_SOURCE)

	def setHistoryExpanded(self, bExpanded):
		self.bHistoryExpanded = bExpanded

	def placeHistory(self):
		screen = self.top.getScreen()
		szText = gc.getCivicInfo(self.iCivic).getCivilopedia()
		szTitle = u""
		draw_expandable_text_panel(screen, self.top, szTitle, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, szText, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)
	def handleInput (self, inputClass):
		return 0
