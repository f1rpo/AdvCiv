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

class SevoPediaProject:

	def __init__(self, main):
		self.iProject = -1
		self.bHistoryExpanded = False
		self.top = main

		self.W_REQUIRES = get_panel_width_for_buttons(2, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.X_REQUIRES = self.top.R_PEDIA_PAGE - self.W_REQUIRES
		self.Y_REQUIRES = self.top.Y_PEDIA_PAGE
		self.H_REQUIRES = 116
		self.W_MOVIE = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.X_MOVIE = self.X_REQUIRES - self.W_MOVIE - 10
		self.Y_MOVIE = self.Y_REQUIRES
		self.H_MOVIE = self.H_REQUIRES
		self.playButtonPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_PLAY_BUTTON").getPath()

		self.X_PROJECT_PANE = self.top.X_PEDIA_PAGE
		self.Y_PROJECT_PANE = self.top.Y_PEDIA_PAGE
		self.W_PROJECT_PANE = self.top.W_PEDIA_PAGE - self.W_REQUIRES - self.W_MOVIE - 20
		self.H_PROJECT_PANE = 116

		self.W_ICON = 100
		self.H_ICON = 100
		self.X_ICON = self.X_PROJECT_PANE + (self.H_PROJECT_PANE - self.H_ICON) / 2
		self.Y_ICON = self.Y_PROJECT_PANE + (self.H_PROJECT_PANE - self.H_ICON) / 2

		self.X_STATS_PANE = self.X_ICON + self.W_ICON
		self.Y_STATS_PANE = 79
		self.W_STATS_PANE = 200
		self.H_STATS_PANE = 200

		self.X_SPECIAL = self.X_PROJECT_PANE
		self.Y_SPECIAL = self.Y_PROJECT_PANE + self.H_PROJECT_PANE + 10
		self.W_SPECIAL = self.top.R_PEDIA_PAGE - self.X_SPECIAL
		self.H_SPECIAL = 210

		self.X_HISTORY = self.X_PROJECT_PANE
		self.Y_HISTORY = self.Y_SPECIAL + self.H_SPECIAL + 10
		self.W_HISTORY = self.top.R_PEDIA_PAGE - self.X_PROJECT_PANE
		self.H_HISTORY = self.top.B_PEDIA_PAGE - self.Y_HISTORY

		# <!-- custom: cached vanilla define for cost display in stats panel. (Claude code Sonnet 4.6) -->
		self.iPROJECT_PRODUCTION_PERCENT = None

	def interfaceScreen(self, iProject):
		if self.iProject != iProject:
			self.bHistoryExpanded = False
		self.iProject = iProject

		self.placeProjectPane()
		self.placeStats()
		self.placeRequires()
		self.placeMovie()
		self.placeSpecial()
		self.placeHistory()

	def placeProjectPane(self):
		screen = self.top.getScreen()

		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_PROJECT_PANE, self.Y_PROJECT_PANE, self.W_PROJECT_PANE, self.H_PROJECT_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		# <!-- custom: no need for the blue frame on blue background, use transparent instead -->
		# screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_MAIN)
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_EMPTY)
		screen.addDDSGFC(self.top.getNextWidgetName(), gc.getProjectInfo(self.iProject).getButton(), self.X_ICON + self.W_ICON/2 - PANE_ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - PANE_ICON_SIZE/2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def placeStats(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addListBoxGFC(panelName, "", self.X_STATS_PANE, self.Y_STATS_PANE, self.W_STATS_PANE, self.H_STATS_PANE, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(panelName, False)
		projectInfo = gc.getProjectInfo(self.iProject)

		if (isWorldProject(self.iProject)):
			iMaxInstances = gc.getProjectInfo(self.iProject).getMaxGlobalInstances()
			szProjectType = localText.getText("TXT_KEY_PEDIA_WORLD_PROJECT", ())
			if (iMaxInstances > 1):
				szProjectType += " " + localText.getText("TXT_KEY_PEDIA_WONDER_INSTANCES", (iMaxInstances,))
			screen.appendListBoxString(panelName, SASTextScale.titleText(szProjectType.upper()), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

		if (isTeamProject(self.iProject)):
			iMaxInstances = gc.getProjectInfo(self.iProject).getMaxTeamInstances()
			szProjectType = localText.getText("TXT_KEY_PEDIA_TEAM_PROJECT", ())
			if (iMaxInstances > 1):
				szProjectType += " " + localText.getText("TXT_KEY_PEDIA_WONDER_INSTANCES", (iMaxInstances,))
			screen.appendListBoxString(panelName, SASTextScale.titleText(szProjectType.upper()), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

		if (projectInfo.getProductionCost() > 0):
			# <!-- custom: simplify textual info for hammer yield to minimum, remove the "COST:" ugly part -->
			if self.top.iActivePlayer == -1:
				if self.iPROJECT_PRODUCTION_PERCENT is None:
					self.iPROJECT_PRODUCTION_PERCENT = gc.getDefineINT("PROJECT_PRODUCTION_PERCENT")
				szCost = (projectInfo.getProductionCost() * self.iPROJECT_PRODUCTION_PERCENT) / 100
			else:
				szCost = gc.getActivePlayer().getProjectProductionNeeded(self.iProject)
			szTextHammerYield = u"%c %d" % (gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar(), szCost)
			screen.appendListBoxString(panelName, SASTextScale.titleText(szTextHammerYield), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

	def placeRequires(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_REQUIRES", ()), "", False, True, self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES, PanelStyles.PANEL_STYLE_BLUE50)
		screen.enableSelect(panelName, False)
		screen.attachLabel(panelName, "", "  ")
		iPrereq = gc.getProjectInfo(self.iProject).getTechPrereq()
		if (iPrereq >= 0):
			screen.attachImageButton(panelName, "", gc.getTechInfo(iPrereq).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iPrereq, 1, False)

	def placeMovie(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_MOVIE_PANEL", ()), "", False, True, self.X_MOVIE, self.Y_MOVIE, self.W_MOVIE, self.H_MOVIE, PanelStyles.PANEL_STYLE_BLUE50)

		# <!-- custom: use attachLabel for padding similar to Requires panel -->
		screen.attachLabel(panelName, "", "  ")

		iMovieType = SAS_MAGIC_PEDIA_MOVIE_TYPE_PROJECT
		if self.top.pediaMovies.hasMovie(iMovieType, self.iProject):
			iPackedMovie = self.top.SAS_packMovieKey(iMovieType, self.iProject)
			buttonSize = 64
			# <!-- custom: setImageButtonAt positions relative to panel content area (below header).
			# X: Standard centering works correctly.
			# Y: Must be set to 10 (not calculated from panelHeaderHeight) - empirically determined positioning fix. (Claude code Sonnet 4.5) -->
			buttonX = (self.W_MOVIE - buttonSize) / 2
			buttonY = 10
			screen.setImageButtonAt(self.top.getNextWidgetName(), panelName, self.playButtonPath, buttonX, buttonY, buttonSize, buttonSize, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_MOVIE_ENTRY, iPackedMovie)
		else:
			draw_none_text(screen, self.top, self.X_MOVIE, self.Y_MOVIE, self.W_MOVIE, self.H_MOVIE)

	def placeSpecial(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_EFFECTS", ()), "", True, False, self.X_SPECIAL, self.Y_SPECIAL, self.W_SPECIAL, self.H_SPECIAL, PanelStyles.PANEL_STYLE_BLUE50)
		listName = self.top.getNextWidgetName()
		szSpecialText = CyGameTextMgr().getProjectHelp(self.iProject, True, None)[1:]
		screen.addMultilineText(listName, SASTextScale.labelText(szSpecialText), self.X_SPECIAL+5, self.Y_SPECIAL+30, self.W_SPECIAL-10, self.H_SPECIAL-35, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def setHistoryExpanded(self, bExpanded):
		self.bHistoryExpanded = bExpanded

	def placeHistory(self):
		screen = self.top.getScreen()
		szText = gc.getProjectInfo(self.iProject).getCivilopedia()
		szTitle = u""
		draw_expandable_text_panel(screen, self.top, szTitle, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, szText, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)
	def getProjectType(self, iProject):
		if (isWorldProject(iProject)):
			return (3)
		if (isTeamProject(iProject)):
			return (2)
		return (1)

	def getProjectSortedList(self):
		listOfAllTypes = []
		for iBuildingType in range(4):
			listBuildings = []
			iCount = 0
			for iBuilding in range(gc.getNumProjectInfos()):
				if (self.getProjectType(iBuilding) == iBuildingType and not gc.getProjectInfo(iBuilding).isGraphicalOnly()):
					listBuildings.append(iBuilding)
					iCount += 1
			listSorted = [(0,0)] * iCount
			iI = 0
			for iBuilding in listBuildings:
				listSorted[iI] = (gc.getProjectInfo(iBuilding).getDescription(), iBuilding)
				iI += 1
			listSorted.sort()
			for i in range(len(listSorted)):
				listOfAllTypes.append(listSorted[i])
		return listOfAllTypes

	def handleInput (self, inputClass):
		return 0
