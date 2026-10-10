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
import CvUtil
import ScreenInput
import SevoScreenEnums
import SASTextScale

from _sevopedia_helpers import *

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class SevoPediaPromotion:

	def __init__(self, main):
		self.iPromotion = -1
		self.top = main

		self.X_PROMOTION_PANE = self.top.X_PEDIA_PAGE
		self.Y_PROMOTION_PANE = self.top.Y_PEDIA_PAGE
		self.W_PROMOTION_PANE = 250
		self.H_PROMOTION_PANE = 116

		self.W_ICON = 100
		self.H_ICON = 100
		self.X_ICON = self.X_PROMOTION_PANE + (self.H_PROMOTION_PANE - self.H_ICON) / 2
		self.Y_ICON = self.Y_PROMOTION_PANE + (self.H_PROMOTION_PANE - self.H_ICON) / 2

		self.X_REQUIRES = self.X_PROMOTION_PANE + self.W_PROMOTION_PANE + MEDIUM_MARGIN
		self.Y_REQUIRES = self.Y_PROMOTION_PANE
		self.W_REQUIRES = self.top.R_PEDIA_PAGE - self.X_REQUIRES
		self.H_REQUIRES = 116

		self.X_LEADS_TO = self.X_PROMOTION_PANE
		self.Y_LEADS_TO = self.Y_PROMOTION_PANE + self.H_PROMOTION_PANE + SMALL_MARGIN
		self.W_LEADS_TO = self.top.R_PEDIA_PAGE - self.X_LEADS_TO
		self.H_LEADS_TO = NON_MULTILIST_PANEL_STANDARD_HEIGHT

		self.X_FREE_PROMOTIONS_UNITS = self.X_PROMOTION_PANE
		self.Y_FREE_PROMOTIONS_UNITS = self.Y_LEADS_TO + self.H_LEADS_TO + SMALL_MARGIN
		self.W_FREE_PROMOTIONS_UNITS = self.top.R_PEDIA_PAGE - self.X_FREE_PROMOTIONS_UNITS
		self.H_FREE_PROMOTIONS_UNITS = self.H_LEADS_TO

		self.X_FREE_PROMOTION_BUILDINGS = self.X_PROMOTION_PANE
		self.Y_FREE_PROMOTION_BUILDINGS = self.Y_FREE_PROMOTIONS_UNITS + self.H_FREE_PROMOTIONS_UNITS + SMALL_MARGIN
		self.W_FREE_PROMOTION_BUILDINGS = self.top.R_PEDIA_PAGE - self.X_FREE_PROMOTION_BUILDINGS
		self.H_FREE_PROMOTION_BUILDINGS = self.H_LEADS_TO

		self.X_SPECIAL = self.X_PROMOTION_PANE
		self.Y_SPECIAL = self.Y_FREE_PROMOTION_BUILDINGS + self.H_FREE_PROMOTION_BUILDINGS + SMALL_MARGIN
		self.W_SPECIAL = self.top.W_PEDIA_PAGE / 2 - 5
		self.H_SPECIAL = self.top.B_PEDIA_PAGE - self.Y_SPECIAL

		self.X_UNIT_COMBATS = self.X_SPECIAL + self.W_SPECIAL + MEDIUM_MARGIN
		self.Y_UNIT_COMBATS = self.Y_SPECIAL
		self.W_UNIT_COMBATS = self.W_SPECIAL
		self.H_UNIT_COMBATS = self.H_SPECIAL

	def interfaceScreen(self, iPromotion):
		self.iPromotion = iPromotion

		self.placePromotionPane()
		self.placeRequires()
		self.placeLeadsTo()
		self.placeFreePromotionsUnits()
		self.placeFreePromotionBuilding()
		self.placeSpecial()
		self.placeUnitCombats()

	def placePromotionPane(self):
		screen = self.top.getScreen()

		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_PROMOTION_PANE, self.Y_PROMOTION_PANE, self.W_PROMOTION_PANE, self.H_PROMOTION_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		# <!-- custom: no need for the blue frame on blue background, use transparent instead -->
		#screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_MAIN)
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_EMPTY)
		screen.addDDSGFC(self.top.getNextWidgetName(), gc.getPromotionInfo(self.iPromotion).getButton(), self.X_ICON + self.W_ICON/2 - PANE_ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - PANE_ICON_SIZE/2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def placeRequires(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_REQUIRES", ()), "", False, True, self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")
		ePromo = gc.getPromotionInfo(self.iPromotion).getPrereqPromotion()
		if (ePromo > -1):
			screen.attachImageButton(panelName, "", gc.getPromotionInfo(ePromo).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, ePromo, 1, False)
		ePromoOr1 = gc.getPromotionInfo(self.iPromotion).getPrereqOrPromotion1()
		ePromoOr2 = gc.getPromotionInfo(self.iPromotion).getPrereqOrPromotion2()
		ePromoOr3 = gc.getPromotionInfo(self.iPromotion).getPrereqOrPromotion3() # K-Mod
		if (ePromoOr1 > -1):
			if (ePromo > -1):
				screen.attachLabel(panelName, "", localText.getText("TXT_KEY_AND", ()))
				if (ePromoOr2 > -1):
					screen.attachLabel(panelName, "", "(")
			screen.attachImageButton(panelName, "", gc.getPromotionInfo(ePromoOr1).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, ePromoOr1, 1, False)
			if (ePromoOr2 > -1):
				screen.attachLabel(panelName, "", localText.getText("TXT_KEY_OR", ()))
				screen.attachImageButton(panelName, "", gc.getPromotionInfo(ePromoOr2).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, ePromoOr2, 1, False)
				if (ePromo > -1):
					screen.attachLabel(panelName, "", ")")
			# K-Mod, extra prereq
			if (ePromoOr3 > -1):
				screen.attachLabel(panelName, "", localText.getText("TXT_KEY_OR", ()))
				screen.attachImageButton(panelName, "", gc.getPromotionInfo(ePromoOr3).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, ePromoOr3, 1, False)
				if (ePromo > -1):
					screen.attachLabel(panelName, "", ")")			
		eTech = gc.getPromotionInfo(self.iPromotion).getTechPrereq()
		if (eTech > -1):
			screen.attachImageButton(panelName, "", gc.getTechInfo(eTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, eTech, 1, False)
		eReligion = gc.getPromotionInfo(self.iPromotion).getStateReligionPrereq()
		if (eReligion > -1):
			screen.attachImageButton(panelName, "", gc.getReligionInfo(eReligion).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION, eReligion, 1, False)

	def placeLeadsTo(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_LEADS_TO", ()), "", False, True, self.X_LEADS_TO, self.Y_LEADS_TO, self.W_LEADS_TO, self.H_LEADS_TO, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")
		isButtonFound = False
		for j in range(gc.getNumPromotionInfos()):
			# <!-- custom: include PrereqOrPromotion3 (K-Mod extra OR prereq) so Leads To lists all promotions that depend on this one; otherwise some valid upgrades are missing from this panel. (GPT-5.3-Codex) -->
			if (gc.getPromotionInfo(j).getPrereqPromotion() == self.iPromotion or gc.getPromotionInfo(j).getPrereqOrPromotion1() == self.iPromotion or gc.getPromotionInfo(j).getPrereqOrPromotion2() == self.iPromotion or gc.getPromotionInfo(j).getPrereqOrPromotion3() == self.iPromotion):
				screen.attachImageButton(panelName, "", gc.getPromotionInfo(j).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, j, 1, False)
				isButtonFound = True
		if not isButtonFound:
			draw_none_text(screen, self.top, self.X_LEADS_TO, self.Y_LEADS_TO, self.W_LEADS_TO, self.H_LEADS_TO)

	def placeFreePromotionsUnits(self):
		xPanel = self.X_FREE_PROMOTIONS_UNITS
		yPanel = self.Y_FREE_PROMOTIONS_UNITS
		wPanel = self.W_FREE_PROMOTIONS_UNITS
		hPanel = self.H_FREE_PROMOTIONS_UNITS

		txtKeyPanel = "TXT_KEY_PEDIA_FREE_PROMOTIONS_UNITS"

		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

		# Create panel with proper styling
		screen.addPanel(panelName, localText.getText(txtKeyPanel, ()), "", False, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50)

		rowListName = self.top.getNextWidgetName()

		# Create the MultiList control
		# Constants for button display
		multiListX = xPanel + MULTI_LIST_PANEL_OFFSET_X
		multiListY = yPanel + MULTI_LIST_PANEL_OFFSET_Y
		multiListW = wPanel + MULTI_LIST_PANEL_ADDITIONAL_W
		multiListH = hPanel + MULTI_LIST_PANEL_ADDITIONAL_H
		screen.addMultiListControlGFC(rowListName, "", multiListX, multiListY, multiListW, multiListH, SEVOPEDIA_MULTILIST_NUM_LISTS_AUTO_CALCULATE, MULTILIST_BUTTON_SIZE, MULTILIST_BUTTON_SIZE, TableStyles.TABLE_STYLE_STANDARD)

		isButtonFound = False
		#iButtonIndex = 0
		#maxButtonsPerRow = get_multilist_max_buttons_per_row(multiListW, MULTILIST_BUTTON_SIZE)

		# <!-- custom: code provided with the help of claude ai thanks -->
		# Loop through all units to find those with this promotion
		for iUnit in xrange(gc.getNumUnitInfos()):
			unitInfo = gc.getUnitInfo(iUnit)

			# Check if this unit has the current promotion as a free promotion
			if unitInfo.getFreePromotions(self.iPromotion):
				screen.appendMultiListButton(rowListName, unitInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)

				isButtonFound = True
				#iButtonIndex += 1

		if not isButtonFound:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel)

	def placeFreePromotionBuilding(self):
		xPanel = self.X_FREE_PROMOTION_BUILDINGS
		yPanel = self.Y_FREE_PROMOTION_BUILDINGS
		wPanel = self.W_FREE_PROMOTION_BUILDINGS
		hPanel = self.H_FREE_PROMOTION_BUILDINGS

		txtKeyPanel = "TXT_KEY_PEDIA_FREE_PROMOTION_BUILDINGS"

		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

		# Create panel with proper styling
		screen.addPanel(panelName, localText.getText(txtKeyPanel, ()), "", False, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50)

		rowListName = self.top.getNextWidgetName()

		# Create the MultiList control
		# Constants for button display
		multiListX = xPanel + MULTI_LIST_PANEL_OFFSET_X
		multiListY = yPanel + MULTI_LIST_PANEL_OFFSET_Y
		multiListW = wPanel + MULTI_LIST_PANEL_ADDITIONAL_W
		multiListH = hPanel + MULTI_LIST_PANEL_ADDITIONAL_H
		screen.addMultiListControlGFC(rowListName, "", multiListX, multiListY, multiListW, multiListH, SEVOPEDIA_MULTILIST_NUM_LISTS_AUTO_CALCULATE, MULTILIST_BUTTON_SIZE, MULTILIST_BUTTON_SIZE, TableStyles.TABLE_STYLE_STANDARD)

		isButtonFound = False
		#iButtonIndex = 0
		#maxButtonsPerRow = get_multilist_max_buttons_per_row(multiListW, BUTTON_SIZE)

		# <!-- custom: code provided with the help of claude ai thanks -->
		# Loop through all buildings to find those that grant this promotion
		for iBuilding in range(gc.getNumBuildingInfos()):
			buildingInfo = gc.getBuildingInfo(iBuilding)

			# Check if this building grants the current promotion
			if buildingInfo.getFreePromotion() == self.iPromotion:
				screen.appendMultiListButton(rowListName, buildingInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, 1, False)

				isButtonFound = True
				#iButtonIndex += 1

		if not isButtonFound:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel)

	def placeSpecial(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SPECIAL_ABILITIES", ()), "", True, False, self.X_SPECIAL, self.Y_SPECIAL, self.W_SPECIAL, self.H_SPECIAL, PanelStyles.PANEL_STYLE_BLUE50)
		listName = self.top.getNextWidgetName()
		szSpecialText = CyGameTextMgr().getPromotionHelp(self.iPromotion, True)[1:]
		szSpecialText = SASTextScale.normalizeLabelText(szSpecialText)
		screen.addMultilineText(listName, szSpecialText, self.X_SPECIAL+5, self.Y_SPECIAL+30, self.W_SPECIAL-10, self.H_SPECIAL-35, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeUnitCombats(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_PROMOTION_UNITS", ()), "", True, True, self.X_UNIT_COMBATS, self.Y_UNIT_COMBATS, self.W_UNIT_COMBATS, self.H_UNIT_COMBATS, PanelStyles.PANEL_STYLE_BLUE50)
		szTable = self.top.getNextWidgetName()
		screen.addTableControlGFC(szTable, 1, self.X_UNIT_COMBATS + 10, self.Y_UNIT_COMBATS + 40, self.W_UNIT_COMBATS - 20, self.H_UNIT_COMBATS - 50, False, False, 24, 24, TableStyles.TABLE_STYLE_EMPTY)
		i = 0
		for iI in range(gc.getNumUnitCombatInfos()):
			if (0 != gc.getPromotionInfo(self.iPromotion).getUnitCombat(iI)):
				# <!-- custom: note: i had removed the `iRow = screen.appendTableRow(szTable)` line to fix ruff warning of variable being unused so cleaning this up, after having asked chatgpt this seemed fine and safe to do and we had no errors so maybe was solved as well (as in sevopedia unit ruff warning fix/cleanup as well), other sevopedia classes didn't seem to have a similar issue from quick glance at each file, although i had not investigated it in depth, was hopefully fine i thought, and if in doubt i could have checked ingame display to see if it matches xml info, in sevopedia.
				# However: update: although the variable is not used, the line was needed to show all entries, else we only showed the first one, so remove the identifier rather while keeping the instruction, as chatgpt noticed as wel and poitned to me hehe thanks -->
				# iRow = screen.appendTableRow(szTable)
				screen.appendTableRow(szTable)
				screen.setTableText(szTable, 0, i, SASTextScale.labelText(gc.getUnitCombatInfo(iI).getDescription()), gc.getUnitCombatInfo(iI).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT_COMBAT, iI, -1, CvUtil.FONT_LEFT_JUSTIFY)
				i += 1

	def handleInput (self, inputClass):
		return 0
