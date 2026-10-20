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

class SevoPediaCorporation:

	def __init__(self, main):
		self.iCorporation = -1
		self.bHistoryExpanded = False
		self.top = main

		self.X_CORPORATION_PANE = self.top.X_PEDIA_PAGE
		self.Y_CORPORATION_PANE = self.top.Y_PEDIA_PAGE
		self.W_CORPORATION_PANE = 200
		self.H_CORPORATION_PANE = 230

		self.W_ICON = PANE_ICON_SIZE
		self.H_ICON = PANE_ICON_SIZE
		self.X_ICON = self.X_CORPORATION_PANE + (PANE_ICON_FRAME_SIZE - PANE_ICON_SIZE) / 2 + 19
		self.Y_ICON = self.Y_CORPORATION_PANE + (self.H_CORPORATION_PANE - self.H_ICON) / 2 + 3

		self.X_RIGHT = self.X_CORPORATION_PANE + self.W_CORPORATION_PANE + MEDIUM_MARGIN
		self.H_TOP_ROW = NON_MULTILIST_PANEL_STANDARD_HEIGHT

		self.W_REQUIRES = get_panel_width_for_buttons(2, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.X_REQUIRES = self.X_RIGHT
		self.Y_REQUIRES = self.Y_CORPORATION_PANE
		self.H_REQUIRES = self.H_TOP_ROW

		self.W_COMPETES = self.W_REQUIRES
		self.X_COMPETES = self.X_REQUIRES
		self.Y_COMPETES = self.Y_REQUIRES + self.H_TOP_ROW + SMALL_MARGIN
		self.H_COMPETES = self.H_TOP_ROW

		self.X_BONUSES_CONSUMED = self.X_REQUIRES + self.W_REQUIRES + SMALL_MARGIN
		self.Y_BONUSES_CONSUMED = self.Y_REQUIRES
		self.W_BONUSES_CONSUMED = self.top.R_PEDIA_PAGE - self.X_BONUSES_CONSUMED
		self.H_BONUSES_CONSUMED = self.H_TOP_ROW

		self.W_MOVIE = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.X_MOVIE = self.top.R_PEDIA_PAGE - self.W_MOVIE
		self.Y_MOVIE = self.Y_COMPETES
		self.H_MOVIE = self.H_TOP_ROW
		self.playButtonPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_PLAY_BUTTON").getPath()

		self.X_BONUSES_GENERATED = self.X_BONUSES_CONSUMED
		self.Y_BONUSES_GENERATED = self.Y_BONUSES_CONSUMED + self.H_TOP_ROW + SMALL_MARGIN
		self.W_BONUSES_GENERATED = self.X_MOVIE - SMALL_MARGIN - self.X_BONUSES_GENERATED
		self.H_BONUSES_GENERATED = self.H_TOP_ROW

		self.X_SPECIAL = self.X_CORPORATION_PANE
		self.Y_SPECIAL = self.Y_BONUSES_GENERATED + self.H_BONUSES_GENERATED + SMALL_MARGIN
		self.W_SPECIAL = self.top.R_PEDIA_PAGE - self.X_SPECIAL
		self.H_SPECIAL = 300

		self.X_HISTORY = self.X_CORPORATION_PANE
		self.Y_HISTORY = self.Y_SPECIAL + self.H_SPECIAL + SMALL_MARGIN
		self.W_HISTORY = self.top.R_PEDIA_PAGE - self.X_HISTORY
		self.H_HISTORY = self.top.B_PEDIA_PAGE - self.Y_HISTORY

	def interfaceScreen(self, iCorporation):
		if self.iCorporation != iCorporation:
			self.bHistoryExpanded = False
		self.iCorporation = iCorporation

		self.placeCorporationPane()
		self.placeRequires()
		self.placeCompetesWith()
		self.placeBonusesConsumed()
		self.placeBonusesGenerated()
		self.placeMovie()
		self.placeSpecial()
		self.placeHistory()

	def placeCorporationPane(self):
		screen = self.top.getScreen()
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_CORPORATION_PANE, self.Y_CORPORATION_PANE, self.W_CORPORATION_PANE, self.H_CORPORATION_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_EMPTY)
		screen.addDDSGFC(self.top.getNextWidgetName(), gc.getCorporationInfo(self.iCorporation).getButton(), self.X_ICON + self.W_ICON/2 - PANE_ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - PANE_ICON_SIZE/2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def placeRequires(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_REQUIRES", ()), "", False, True, self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")
		iTech = gc.getCorporationInfo(self.iCorporation).getTechPrereq()
		if (iTech > -1):
			screen.attachImageButton(panelName, "", gc.getTechInfo(iTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, 1, False)
		for iBuilding in range(gc.getNumBuildingInfos()):
			if (gc.getBuildingInfo(iBuilding).getFoundsCorporation() == self.iCorporation):
				screen.attachImageButton(panelName, "", gc.getBuildingInfo(iBuilding).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, 1, False)
		for iUnit in range(gc.getNumUnitInfos()):
			bRequired = False
			for iBuilding in range(gc.getNumBuildingInfos()):
				if (gc.getBuildingInfo(iBuilding).getFoundsCorporation() == self.iCorporation):
					# advc.003t:
					if gc.getUnitInfo(iUnit).getBuildings(iBuilding):# or gc.getUnitInfo(iUnit).getForceBuildings(iBuilding):
						bRequired = True
						break
			if bRequired:
				screen.attachImageButton(panelName, "", gc.getUnitInfo(iUnit).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)

	def _getPrereqBonuses(self):
		prereqs = []
		info = gc.getCorporationInfo(self.iCorporation)
		for i in range(gc.getNUM_CORPORATION_PREREQ_BONUSES()):
			iBonus = info.getPrereqBonus(i)
			if iBonus > -1 and iBonus not in prereqs:
				prereqs.append(iBonus)
		return prereqs

	def placeCompetesWith(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_COMPETES_WITH", ()), "", False, True, self.X_COMPETES, self.Y_COMPETES, self.W_COMPETES, self.H_COMPETES, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		myPrereqs = self._getPrereqBonuses()
		bFound = False
		for iCorp in range(gc.getNumCorporationInfos()):
			if iCorp == self.iCorporation:
				continue
			kOther = gc.getCorporationInfo(iCorp)
			bShares = False
			for i in range(gc.getNUM_CORPORATION_PREREQ_BONUSES()):
				iBonus = kOther.getPrereqBonus(i)
				if iBonus in myPrereqs:
					bShares = True
					break
			if bShares:
				screen.attachImageButton(panelName, "", kOther.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CORPORATION, iCorp, 1, False)
				bFound = True

		if not bFound:
			draw_none_text(screen, self.top, self.X_COMPETES, self.Y_COMPETES, self.W_COMPETES, self.H_COMPETES)

	def placeBonusesConsumed(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_BONUSES_CONSUMED", ()), "", False, True, self.X_BONUSES_CONSUMED, self.Y_BONUSES_CONSUMED, self.W_BONUSES_CONSUMED, self.H_BONUSES_CONSUMED, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		prereqs = self._getPrereqBonuses()
		for iBonus in prereqs:
			screen.attachImageButton(panelName, "", gc.getBonusInfo(iBonus).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus, 1, False)

		if len(prereqs) == 0:
			draw_none_text(screen, self.top, self.X_BONUSES_CONSUMED, self.Y_BONUSES_CONSUMED, self.W_BONUSES_CONSUMED, self.H_BONUSES_CONSUMED)

	def placeBonusesGenerated(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_BONUSES_GENERATED", ()), "", False, True, self.X_BONUSES_GENERATED, self.Y_BONUSES_GENERATED, self.W_BONUSES_GENERATED, self.H_BONUSES_GENERATED, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		info = gc.getCorporationInfo(self.iCorporation)
		iBonusProduced = info.getBonusProduced()
		if iBonusProduced > -1:
			screen.attachImageButton(panelName, "", gc.getBonusInfo(iBonusProduced).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonusProduced, 1, False)
		else:
			draw_none_text(screen, self.top, self.X_BONUSES_GENERATED, self.Y_BONUSES_GENERATED, self.W_BONUSES_GENERATED, self.H_BONUSES_GENERATED)

	def placeMovie(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_MOVIE_PANEL", ()), "", False, True, self.X_MOVIE, self.Y_MOVIE, self.W_MOVIE, self.H_MOVIE, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")
		iMovieType = SAS_MAGIC_PEDIA_MOVIE_TYPE_CORPORATION
		if self.top.pediaMovies.hasMovie(iMovieType, self.iCorporation):
			iPackedMovie = self.top.SAS_packMovieKey(iMovieType, self.iCorporation)
			buttonSize = 64
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
		szSpecialText = CyGameTextMgr().parseCorporationInfo(self.iCorporation, True)[1:]
		screen.addMultilineText(listName, SASTextScale.labelText(szSpecialText), self.X_SPECIAL+10, self.Y_SPECIAL+30, self.W_SPECIAL-20, self.H_SPECIAL-40, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def setHistoryExpanded(self, bExpanded):
		self.bHistoryExpanded = bExpanded

	def placeHistory(self):
		screen = self.top.getScreen()
		szText = gc.getCorporationInfo(self.iCorporation).getCivilopedia()
		szTitle = u""
		draw_expandable_text_panel(screen, self.top, szTitle, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, szText, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)
	def handleInput (self, inputClass):
		return 0
