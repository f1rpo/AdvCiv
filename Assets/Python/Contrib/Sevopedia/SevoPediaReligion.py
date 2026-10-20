# Sid Meier's Civilization 4
# Copyright Firaxis Games 2005
#
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

class SevoPediaReligion:

	def __init__(self, main):
		self.iReligion = -1
		self.bHistoryExpanded = False
		self.top = main

		# <!-- custom: multilist favorites panel width from helper so callers can switch button count per row without redoing panel math. (GPT-5.3-Codex) -->
		self.W_FAVORITES = get_multilist_panel_width_for_buttons(4, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_MULTI_LIST_LEFT_EDGE_PADDING, HYPOTHESIZED_MULTI_LIST_RIGHT_EDGE_PADDING, HYPOTHESIZED_MULTI_LIST_INTER_BUTTON_SPACING)
		self.X_FAVORITES = self.top.R_PEDIA_PAGE - self.W_FAVORITES
		self.Y_FAVORITES = self.top.Y_PEDIA_PAGE
		self.H_FAVORITES = self.top.B_PEDIA_PAGE - self.Y_FAVORITES

		self.X_RELIGION_PANE = self.top.X_PEDIA_PAGE
		self.Y_RELIGION_PANE = self.top.Y_PEDIA_PAGE
		self.W_RELIGION_PANE = 200
		self.H_RELIGION_PANE = 230

		self.W_ICON = PANE_ICON_SIZE
		self.H_ICON = PANE_ICON_SIZE
		# <!-- custom: icon at the center of the panel for its middle point.
		# Note: formula is approximative but seems to work quite well for a button size of 64px -->
		self.X_ICON = self.X_RELIGION_PANE + (PANE_ICON_FRAME_SIZE - PANE_ICON_SIZE) / 2 + 19
		self.Y_ICON = self.Y_RELIGION_PANE + (self.H_RELIGION_PANE - self.H_ICON) / 2 + 3

		self.W_REMAINING_CENTER_SPACE = self.top.R_PEDIA_PAGE - (self.W_FAVORITES + MEDIUM_MARGIN) - MEDIUM_MARGIN - (self.X_RELIGION_PANE + self.W_RELIGION_PANE + MEDIUM_MARGIN)
		self.W_LEFT_COLUMN = self.W_REMAINING_CENTER_SPACE / 2

		self.X_BUILDINGS = self.X_RELIGION_PANE + self.W_RELIGION_PANE + MEDIUM_MARGIN
		self.Y_BUILDINGS = self.Y_RELIGION_PANE
		self.W_BUILDINGS = self.W_LEFT_COLUMN - 84 - MEDIUM_MARGIN
		self.H_BUILDINGS = NON_MULTILIST_PANEL_STANDARD_HEIGHT

		self.W_REQUIRES = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.W_MOVIE = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.playButtonPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_PLAY_BUTTON").getPath()

		self.X_UNITS = self.X_BUILDINGS
		self.Y_UNITS = self.Y_BUILDINGS + self.H_BUILDINGS + SMALL_MARGIN
		self.W_UNITS = self.W_BUILDINGS
		self.H_UNITS = self.H_BUILDINGS

		self.H_REQUIRES = self.H_BUILDINGS

		self.X_MOVIE = self.X_BUILDINGS + self.W_BUILDINGS + MEDIUM_MARGIN
		self.Y_MOVIE = self.Y_BUILDINGS
		self.H_MOVIE = self.H_REQUIRES

		self.X_REQUIRES = self.X_MOVIE
		self.Y_REQUIRES = self.Y_UNITS

		self.X_SPECIAL = self.X_BUILDINGS + self.W_LEFT_COLUMN + MEDIUM_MARGIN
		self.Y_SPECIAL = self.Y_BUILDINGS
		self.W_SPECIAL = self.top.R_PEDIA_PAGE - self.X_SPECIAL - self.W_FAVORITES - MEDIUM_MARGIN
		self.H_SPECIAL = self.H_BUILDINGS + SMALL_MARGIN + self.H_UNITS

		self.X_HISTORY = self.X_RELIGION_PANE
		self.Y_HISTORY = self.Y_UNITS + self.H_UNITS + SMALL_MARGIN
		self.W_HISTORY = self.top.R_PEDIA_PAGE - self.X_HISTORY - self.W_FAVORITES - MEDIUM_MARGIN
		self.H_HISTORY = self.top.B_PEDIA_PAGE - self.Y_HISTORY

	def interfaceScreen(self, iReligion):
		if self.iReligion != iReligion:
			self.bHistoryExpanded = False
		self.iReligion = iReligion

		self.placeFavorites()
		self.placeReligionPane()
		self.placeBuilding()
		self.placeMovie()
		self.placeUnit()
		self.placeRequires()
		# <!-- custom: no V.Source panel on Religion page: vote sources with religion-yield/religion-commerce effects are global systems (UN/AP-level) and not specific to one religion entry, so showing them per-religion would be misleading. (GPT-5.3-Codex) -->
		self.placeSpecial()
		self.placeHistory()

	# <!-- custom: in https://civ4bug.sourceforge.net/PythonAPI/AllClasses.html i have found this:
	# VOID addMultiListControlGFC (STRING szName, STRING helpText, INT iX, INT iY, INT iWidth, INT iHeight, INT numLists, INT defaultWidth, INT defaultHeight, TableStyle eStyle)
	# and in https://github.com/f1rpo/AdvCiv/blob/master/Assets/Python/Screens/CvMainInterface.py#L2050C1-L2053C38 i have found this:
	# 		self.screen.addMultiListControlGFC("BottomButtonList", u"",
	#			lRect.x(), lRect.y(), lRect.width(), lRect.height(),
	#			4, iButtonSize, iButtonSize, # numLists, defaultWidth, defaultHeight
	#			TableStyles.TABLE_STYLE_STANDARD)
	# if it helps us adapt/use the addMultiListControlGFC method -->
	# <!-- custom: part of the code here (placeFavorites in particular but possibly not only, is imported from History Rewritten mod C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\History Rewritten\Assets\Python\Pedia\CvPediaReligion.py which may be modified or not for AdvCiv-SAS, with the help of claude AI thanks -->
	def placeFavorites(self):
		xPanel = self.X_FAVORITES
		yPanel = self.Y_FAVORITES
		wPanel = self.W_FAVORITES
		hPanel = self.H_FAVORITES

		# <!-- custom: this panel lists AI favorites for the selected religion, so "Favorites" is clearer than "Leaders". (GPT-5.3-Codex) -->
		txtKeyPanel = "TXT_KEY_PEDIA_FAVORITES"
		headerLabel = localText.getText(txtKeyPanel, ())
		numWithReligion, totalRealLeaders = get_favorite_leader_counts(FAVORITE_LEADER_TYPE_RELIGION, self.iReligion, EXCLUDED_LEADER_TYPES_FROM_SEVOPEDIA)
		headerText = format_leaders_header_text(numWithReligion, totalRealLeaders, headerLabel)

		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

		# Create panel with proper styling
		screen.addPanel(panelName, headerText, "", False, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50)

		# <!-- custom: note: this doesn't seem to do anything in multilist methods and in particular no padding so do not use this here -->
		# Additional left side padding for the button(s)
		#screen.attachLabel(panelName, "", "  ")

		rowListName = self.top.getNextWidgetName()

		# Create the MultiList control
		# Constants for button display
		multiListX = xPanel + MULTI_LIST_PANEL_OFFSET_X
		multiListY = yPanel + MULTI_LIST_PANEL_OFFSET_Y
		multiListW = wPanel + MULTI_LIST_PANEL_ADDITIONAL_W
		multiListH = hPanel + MULTI_LIST_PANEL_ADDITIONAL_H
		screen.addMultiListControlGFC(rowListName, "", multiListX, multiListY, multiListW, multiListH, SEVOPEDIA_MULTILIST_NUM_LISTS_AUTO_CALCULATE, MULTILIST_BUTTON_SIZE, MULTILIST_BUTTON_SIZE, TableStyles.TABLE_STYLE_STANDARD)

		# Find all leaders who have this religion as favorite, <!-- custom: and --> add <!-- custom: them --> all to the list <!-- custom: (not catch them all... or maybe.. or not or yes... xd) -->
		for iLeader in xrange(gc.getNumLeaderHeadInfos()):
			leaderInfo = gc.getLeaderHeadInfo(iLeader)
			if leaderInfo.getFavoriteReligion() == self.iReligion:
				leaderInfo = gc.getLeaderHeadInfo(iLeader)
				# <!-- custom: Generic Sevopedia leader links use the Leader page's canonical unique civilization for trait production help; literal data2=1 incorrectly meant Arabia. See KI#332. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				screen.appendMultiListButton(rowListName, leaderInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_LEADER, iLeader, self.top.pediaLeader.getCiv(iLeader), False)

	def placeReligionPane(self):
		screen = self.top.getScreen()

		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_RELIGION_PANE, self.Y_RELIGION_PANE, self.W_RELIGION_PANE, self.H_RELIGION_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		# <!-- custom: no need for the blue frame on blue background, use transparent instead -->
		#screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_MAIN)
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_EMPTY)
		screen.addDDSGFC(self.top.getNextWidgetName(), gc.getReligionInfo(self.iReligion).getButton(), self.X_ICON + self.W_ICON/2 - PANE_ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - PANE_ICON_SIZE/2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1)

	# <!-- custom: part of the code here (placeBuilding and placeUnit in particular is imported from Rise of Mankind (291) mod C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\Rise of Mankind\ which may be modified or not for AdvCiv-SAS -->
	# Rise of Mankind 2.9
	def placeBuilding(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_BUILDINGS_ROM_291", ()), "", False, True, self.X_BUILDINGS, self.Y_BUILDINGS, self.W_BUILDINGS, self.H_BUILDINGS, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")
		for iBuilding in range(gc.getNumBuildingInfos()):
			# buildings have several options for religions so need to check them all, only one of them needs to be true
			iPrereq = gc.getBuildingInfo(iBuilding).getPrereqReligion()
			iPrereq2 = gc.getBuildingInfo(iBuilding).getReligionType()
			iPrereq3 = gc.getBuildingInfo(iBuilding).getStateReligion()
			#iPrereq4 = gc.getBuildingInfo(iBuilding).getGlobalReligionCommerce()
			if (iPrereq == self.iReligion or iPrereq2 == self.iReligion or iPrereq3 == self.iReligion):
				screen.attachImageButton(panelName, "", gc.getBuildingInfo(iBuilding).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, 1, False)
			# <!-- custom: adjusted indentation of code comment so it is properly indented at same level of indentation than at this part of the code -->
			#elif (iPrereq == self.iReligion and iPrereq4 > 0):
			#	screen.attachImageButton(panelName, "", gc.getBuildingInfo(iBuilding).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, 1, False)

	def placeMovie(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_MOVIE_PANEL", ()), "", False, True, self.X_MOVIE, self.Y_MOVIE, self.W_MOVIE, self.H_MOVIE, PanelStyles.PANEL_STYLE_BLUE50)

		# <!-- custom: use attachLabel for padding similar to Requires panel -->
		screen.attachLabel(panelName, "", "  ")

		iMovieType = SAS_MAGIC_PEDIA_MOVIE_TYPE_RELIGION
		if self.top.pediaMovies.hasMovie(iMovieType, self.iReligion):
			iPackedMovie = self.top.SAS_packMovieKey(iMovieType, self.iReligion)

			buttonSize = 64
			# <!-- custom: setImageButtonAt positions relative to panel content area (below header).
			# X: Standard centering works correctly.
			# Y: Must be set to 10 (not calculated from panelHeaderHeight) - empirically determined positioning fix. (Claude code Sonnet 4.5) -->
			buttonX = (self.W_MOVIE - buttonSize) / 2
			buttonY = 10
			screen.setImageButtonAt(self.top.getNextWidgetName(), panelName, self.playButtonPath, buttonX, buttonY, buttonSize, buttonSize, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_MOVIE_ENTRY, iPackedMovie)
		else:
			draw_none_text(screen, self.top, self.X_MOVIE, self.Y_MOVIE, self.W_MOVIE, self.H_MOVIE)

	def placeUnit(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_UNITS_ROM_291", ()), "", False, True, self.X_UNITS, self.Y_UNITS, self.W_UNITS, self.H_UNITS, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")
		for iUnit in range(gc.getNumUnitInfos()):
			iPrereq = gc.getUnitInfo(iUnit).getPrereqReligion()
			iPrereq2 = gc.getUnitInfo(iUnit).getReligionType()
			iPrereq3 = gc.getUnitInfo(iUnit).getStateReligion()
			if (iPrereq == self.iReligion or iPrereq2 == self.iReligion or iPrereq3 == self.iReligion):
				screen.attachImageButton(panelName, "", gc.getUnitInfo(iUnit).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)

	def placeRequires(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_REQUIRES", ()), "", False, True, self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")
		iTech = gc.getReligionInfo(self.iReligion).getTechPrereq()
		if iTech > -1:
			screen.attachImageButton(panelName, "", gc.getTechInfo(iTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, 1, False)

	def placeSpecial(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_EFFECTS", ()), "", True, False, self.X_SPECIAL, self.Y_SPECIAL, self.W_SPECIAL, self.H_SPECIAL, PanelStyles.PANEL_STYLE_BLUE50)
		listName = self.top.getNextWidgetName()
		screen.attachListBoxGFC(panelName, listName, "", TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(listName, False)
		szSpecialText = CyGameTextMgr().parseReligionInfo(self.iReligion, True)
		# <!-- custom: use text formatting that allows for the top to have some room before we show text, based on sevopedia terrain's placeSpecial rework -->
		screen.addMultilineText(listName, SASTextScale.labelText(szSpecialText), self.X_SPECIAL+5, self.Y_SPECIAL+10, self.W_SPECIAL-10, self.H_SPECIAL-20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def setHistoryExpanded(self, bExpanded):
		self.bHistoryExpanded = bExpanded

	def placeHistory(self):
		screen = self.top.getScreen()
		szText = gc.getReligionInfo(self.iReligion).getCivilopedia()
		szTitle = u""
		draw_expandable_text_panel(screen, self.top, szTitle, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, szText, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)
	def handleInput (self, inputClass):
		return 0
