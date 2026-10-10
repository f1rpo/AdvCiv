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
import SevoScreenEnums
from SASFontUtils import *
import SASTextScale

from _sevopedia_helpers import *

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

# <!-- custom: change its value if you don't want to see AI information in the special abilities panel -->
IS_SHOW_AI_INFO = (gc.getDefineINT("SAS_SEVOPEDIA_BONUS_SHOW_AI_INFORMATION") > 0)

class SevoPediaBonus:

	def __init__(self, main):
		self.iBonus = -1
		self.bHistoryExpanded = False
		self.bContentExpanded = False
		self.top = main

		self.X_BONUS_PANE = self.top.X_PEDIA_PAGE
		self.Y_BONUS_PANE = self.top.Y_PEDIA_PAGE
		self.W_BONUS_PANE = (self.top.R_PEDIA_PAGE - self.X_BONUS_PANE - MEDIUM_MARGIN) / 2
		self.H_BONUS_PANE = 250

		self.W_ICON = PANE_ICON_SIZE
		self.H_ICON = PANE_ICON_SIZE
		# <!-- custom: if PANE_ICON_SIZE is small (e.g. 64), start at the center of self.X_BONUS_PANE, but if PANE_ICON_SIZE is big (e.g. 164) start at the left most part of self.X_BONUS_PANE; same reasoning for Y position -->
		self.X_ICON = self.X_BONUS_PANE + (PANE_ICON_FRAME_SIZE - PANE_ICON_SIZE) / 2
		self.Y_ICON = self.Y_BONUS_PANE + (self.H_BONUS_PANE - self.H_ICON) / 2

		# <!-- custom: add an extra margin to accommodate the potentially larger PANE_ICON_SIZE (than for example 64), if diff is 0 this is harmless to keep too so is dynamical code that can handle optionally larger PANE_ICON_SIZE (vs old PANE_ICON_SIZE of 64) that you may keep or remove as you prefer -->
		self.SMALLER_ICON_SIZE_THAN_ICON_FRAME_MARGIN = (PANE_ICON_FRAME_SIZE - PANE_ICON_SIZE) / 2

		self.STATS_PANE_LEFT_SIDE_MARGIN = 0
		self.STATS_PANE_UPPER_PADDING = 38

		self.X_STATS_PANE = self.X_BONUS_PANE + self.STATS_PANE_LEFT_SIDE_MARGIN + self.W_ICON + (2 * self.SMALLER_ICON_SIZE_THAN_ICON_FRAME_MARGIN)
		self.Y_STATS_PANE = self.Y_BONUS_PANE + self.STATS_PANE_UPPER_PADDING
		self.W_STATS_PANE = self.W_BONUS_PANE - self.W_ICON - (2 * self.SMALLER_ICON_SIZE_THAN_ICON_FRAME_MARGIN) - self.STATS_PANE_LEFT_SIDE_MARGIN
		self.H_STATS_PANE = self.H_BONUS_PANE - self.STATS_PANE_UPPER_PADDING

		# <!-- custom: see sevopediaunit's self.W_TOTAL_EFFECTIVE_UNIT_PANE for differences in implementation -->
		self.W_TOTAL_EFFECTIVE_BONUS_PANE = self.W_BONUS_PANE

		self.X_IMPROVEMENTS = self.X_BONUS_PANE
		self.Y_IMPROVEMENTS = self.Y_BONUS_PANE + self.H_BONUS_PANE + SMALL_MARGIN
		self.W_IMPROVEMENTS = self.W_TOTAL_EFFECTIVE_BONUS_PANE
		self.H_IMPROVEMENTS = NON_MULTILIST_PANEL_STANDARD_HEIGHT

		self.X_UNITS = self.X_BONUS_PANE
		self.W_UNITS = self.top.R_PEDIA_PAGE - self.X_UNITS
		self.Y_UNITS = self.Y_IMPROVEMENTS + self.H_IMPROVEMENTS + SMALL_MARGIN
		self.H_UNITS = self.H_IMPROVEMENTS

		# <!-- custom: put the buildings panel under the units panel so easier to refer to it directly, it will also be identical in dimensions (or very close) so we will not need to change much (coordinates or other things) this way -->
		self.X_BUILDINGS_AND_PROJECTS = self.X_BONUS_PANE
		self.Y_BUILDINGS_AND_PROJECTS = self.Y_UNITS + self.H_UNITS + SMALL_MARGIN
		self.W_BUILDINGS_AND_PROJECTS = self.W_UNITS
		self.H_BUILDINGS_AND_PROJECTS = self.H_IMPROVEMENTS

		self.X_TERRAINS = self.X_BONUS_PANE
		self.Y_TERRAINS = self.Y_BUILDINGS_AND_PROJECTS + self.H_BUILDINGS_AND_PROJECTS + SMALL_MARGIN
		self.W_TERRAINS = self.W_IMPROVEMENTS
		self.H_TERRAINS = self.H_IMPROVEMENTS

		self.X_SPECIAL = self.X_BONUS_PANE
		self.Y_SPECIAL = self.Y_TERRAINS + self.H_TERRAINS + SMALL_MARGIN
		self.W_SPECIAL = self.W_TOTAL_EFFECTIVE_BONUS_PANE
		self.H_SPECIAL = self.top.B_PEDIA_PAGE - self.Y_SPECIAL

		self.H_ADJUST_HEIGHT_ANIMATION_TO_MATCH_ADJACENT_PANE = 7

		self.X_BONUS_ANIMATION = self.X_BONUS_PANE + self.W_TOTAL_EFFECTIVE_BONUS_PANE + MEDIUM_MARGIN
		self.Y_BONUS_ANIMATION = self.Y_BONUS_PANE + self.H_ADJUST_HEIGHT_ANIMATION_TO_MATCH_ADJACENT_PANE
		self.W_BONUS_ANIMATION = self.W_TOTAL_EFFECTIVE_BONUS_PANE
		self.H_BONUS_ANIMATION = self.H_BONUS_PANE - self.H_ADJUST_HEIGHT_ANIMATION_TO_MATCH_ADJACENT_PANE

		self.SCALE_ANIMATION = 0.7

		# <!-- custom: since we split this right side width in 3 panels, (each) and it is separated by 2 MEDIUM_MARGINS, substract these before diving -->
		self.X_REVEALED_BY = self.X_BONUS_ANIMATION
		self.Y_REVEALED_BY = self.Y_BONUS_ANIMATION + self.H_BONUS_ANIMATION + SMALL_MARGIN
		self.W_REVEALED_BY = (self.W_BONUS_PANE - (2 * MEDIUM_MARGIN)) / 3
		self.H_REVEALED_BY = self.H_IMPROVEMENTS

		self.X_TRADEABLE_SINCE = self.X_REVEALED_BY + self.W_REVEALED_BY + MEDIUM_MARGIN
		self.Y_TRADEABLE_SINCE = self.Y_REVEALED_BY
		self.W_TRADEABLE_SINCE = self.W_REVEALED_BY
		self.H_TRADEABLE_SINCE = self.H_IMPROVEMENTS

		self.X_OBSOLETE_WITH = self.X_TRADEABLE_SINCE + self.W_TRADEABLE_SINCE + MEDIUM_MARGIN
		self.Y_OBSOLETE_WITH = self.Y_REVEALED_BY
		self.W_OBSOLETE_WITH = self.W_REVEALED_BY
		self.H_OBSOLETE_WITH = self.H_IMPROVEMENTS

		self.X_FEATURES = self.X_BONUS_ANIMATION
		self.Y_FEATURES = self.Y_TERRAINS
		self.W_FEATURES = (self.W_BONUS_ANIMATION / 2) - MEDIUM_MARGIN
		self.H_FEATURES = self.H_IMPROVEMENTS

		self.X_FEATURE_TERRAIN_BOOLEANS = self.X_FEATURES + self.W_FEATURES + MEDIUM_MARGIN
		self.Y_FEATURE_TERRAIN_BOOLEANS = self.Y_FEATURES
		self.W_FEATURE_TERRAIN_BOOLEANS = self.W_FEATURES
		self.H_FEATURE_TERRAIN_BOOLEANS = self.H_IMPROVEMENTS

		self.X_HISTORY = self.X_BONUS_ANIMATION
		self.Y_HISTORY = self.Y_FEATURES + self.H_FEATURES + SMALL_MARGIN
		self.W_HISTORY = self.W_BONUS_ANIMATION
		self.H_HISTORY = self.top.B_PEDIA_PAGE - self.Y_HISTORY

	def interfaceScreen(self, iBonus):
		if self.iBonus != iBonus:
			self.bHistoryExpanded = False
			self.bContentExpanded = False
		self.iBonus = iBonus

		if self.bContentExpanded:
			self.placeBonusAnimation()
			return

		# <!-- custom: Move the panel position, so that it starts from a lower point in the screen.
		# We will use this newly freed place to put the larger Buildings, while we create a RevealedBy panel instead where the Buildings panel was.
		# Hopefully much clearer and more room to fit many buildings this way now, and to separate tech revealed by and tech tradeable since, both currently in the "Requires" panel which is very weird i think, now renamed to "RevealedBy" and "TradeableSince") -->
		self.placeBonusPane()
		self.placeStats()
		if not self.bHistoryExpanded:
			self.placeBonusAnimation()
		self.placeImprovements()
		self.placeUnits()
		self.placeBuildingsAndProjects()
		self.placeTerrains()
		self.placeFeatures()
		self.placeFeatureTerrainBooleans()
		self.placeSpecial()
		# <!-- custom: split the former/old placeRequires, into 2 functions/methods now instead, placeRevealedBy and placeTradeableSince (which is hopefully more accurate this way too) -->
		self.placeRevealedBy()
		self.placeTradeableSince()
		self.placeObsoleteWith()
		self.placeHistory()

	def placeBonusPane(self):
		screen = self.top.getScreen()
		screen.addPanel( self.top.getNextWidgetName(), "", "", False, False, self.X_BONUS_PANE, self.Y_BONUS_PANE, self.W_BONUS_PANE, self.H_BONUS_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		# <!-- custom: was PanelStyles.PANEL_STYLE_MAIN -->
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_EMPTY)
		screen.addDDSGFC(self.top.getNextWidgetName(), gc.getBonusInfo(self.iBonus).getButton(), self.X_ICON + self.W_ICON/2 - PANE_ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - PANE_ICON_SIZE/2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1 )

	def placeStats(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addListBoxGFC(panelName, "", self.X_STATS_PANE, self.Y_STATS_PANE, self.W_STATS_PANE, self.H_STATS_PANE, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(panelName, False)

		# <!-- custom: handle multiple potential yield changes by separating the header from yield stats display -->
		szTextHeader = SASTextScale.titleText(localText.getText("TXT_KEY_PEDIA_SEVOPEDIA_BONUS_NATURAL_TILE_YIELD_CHANGES", ()) + u"\n")
		screen.appendListBoxString(panelName, szTextHeader, WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

		for k in range(YieldTypes.NUM_YIELD_TYPES):
			iYieldChange = gc.getBonusInfo(self.iBonus).getYieldChange(k)
			if iYieldChange != 0:
				if iYieldChange > 0:
					sign = "+"
				else:
					sign = ""
				# <!-- custom: beautify, no need to mention "FOOD: +1", just "+1" is enough especially with the food icon -->
				# advc.003y: Remove an awkward space
				#szYield = (u"%s: %s%i" % (gc.getYieldInfo(k).getDescription(), sign, iYieldChange))
				szYield = (u"%s%i" % (sign, iYieldChange))
				# <!-- custom: add information about the precise type of yield it is, which can be otherwise very confusing -->
				szText1 = (u"%c  " % gc.getYieldInfo(k).getChar()) + szYield
				szText2 = SASTextScale.titleText(szText1 + u"\n")
				screen.appendListBoxString(panelName, szText2, WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

	# <!-- custom: switch to an horizontal panel version provided by claude ai at my request thanks. -->
	# <!-- custom: note: this needs to be debugged (simplify code, add left side padding before first button) -->
	def placeImprovements(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SEVOPEDIA_BONUS_TOTAL_BASE_AND_EXTRA_IMPROVEMENTS_YIELD_CHANGES", ()), "", True, True, self.X_IMPROVEMENTS, self.Y_IMPROVEMENTS, self.W_IMPROVEMENTS, self.H_IMPROVEMENTS, PanelStyles.PANEL_STYLE_BLUE50)

		# Create a row container for horizontal layout
		rowPanelName = self.top.getNextWidgetName()
		screen.attachPanel(panelName, rowPanelName, "", "", False, False, PanelStyles.PANEL_STYLE_EMPTY)

		# Add vertical spacing at top to help center content
		# We estimate about 40% of panel height as top margin to achieve vertical centering
		# <!-- custom: note: currently vertical button above label doesn't work so code should be cleaned up and solved, but this is good enough so keeping it as is as it working and functional and an improvement over previous vertical scrolling code -->
		#verticalSpacerName = self.top.getNextWidgetName()
		#verticalMargin = int(self.H_IMPROVEMENTS * 0.2)  # 20% of panel height as top margin
		#screen.attachLabel(rowPanelName, verticalSpacerName, "\n" * int(verticalMargin/10))

		# <!-- custom: hack to add left padding, ideally find how to do it properly while keeping our nice spaced horizontal display -->
		screen.attachLabel(panelName, "", "")

		bAnyFound = False
		xOffset = 0
		buttonSpacing = 20  # Spacing between button columns

		for j in range(gc.getNumImprovementInfos()):
			bFirst = True
			szYield = u""
			bEffect = False
			for k in range(YieldTypes.NUM_YIELD_TYPES):
				iYieldChange = gc.getImprovementInfo(j).getImprovementBonusYield(self.iBonus, k)
				if iYieldChange != 0:
					bEffect = True
					bAnyFound = True
					iYieldChange += gc.getImprovementInfo(j).getYieldChange(k)
					if bFirst:
						bFirst = False
					else:
						szYield += ", "
					if iYieldChange > 0:
						sign = "+"
					else:
						sign = ""
					szYield += SASTextScale.titleText(u"%c%s%i" % (gc.getYieldInfo(k).getChar(), sign, iYieldChange))

			if bEffect:
				# Add horizontal spacing if not the first button
				if xOffset > 0:
					spacerName = self.top.getNextWidgetName()
					spacerText = " " * buttonSpacing
					screen.attachLabel(rowPanelName, spacerName, spacerText)

				# <!-- custom: note: currently vertical button above label doesn't work so code should be cleaned up and solved, but this is good enough so keeping it as is as it working and functional and an improvement over previous vertical scrolling code -->
				# Create a column panel for vertical arrangement (button above label)
				colPanelName = self.top.getNextWidgetName()
				screen.attachPanel(rowPanelName, colPanelName, "", "", False, False, PanelStyles.PANEL_STYLE_EMPTY)

				# Attach the button to the column
				buttonName = self.top.getNextWidgetName()
				screen.attachImageButton(colPanelName, buttonName, gc.getImprovementInfo(j).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_IMPROVEMENT, j, 1, False)

				# Attach the yield label below the button
				yieldLabelName = self.top.getNextWidgetName()
				screen.attachLabel(colPanelName, yieldLabelName, szYield)

				# Update horizontal position
				xOffset += 1

		if not bAnyFound:
			draw_none_text(screen, self.top, self.X_IMPROVEMENTS, self.Y_IMPROVEMENTS, self.W_IMPROVEMENTS, self.H_IMPROVEMENTS)

	def placeUnits(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		# advc.905b: txt key was TXT_KEY_PEDIA_ALLOWS
		screen.addPanel( panelName, localText.getText("TXT_KEY_WB_UNITS", ()), "", False, True, self.X_UNITS, self.Y_UNITS, self.W_UNITS, self.H_UNITS, PanelStyles.PANEL_STYLE_BLUE50 )
		screen.attachLabel(panelName, "", "  ")
		iActivePlayer = gc.getGame().getActivePlayer() # advc.003l
		bAnyFound = False
		for eLoopUnit in range(gc.getNumUnitInfos()):
			bFound = False
			if gc.getUnitInfo(eLoopUnit).getPrereqAndBonus() == self.iBonus:
				bFound = True
				bAnyFound = True
			else:
				j = 0
				while (not bFound and j < gc.getNUM_UNIT_PREREQ_OR_BONUSES()):
					if gc.getUnitInfo(eLoopUnit).getPrereqOrBonuses(j) == self.iBonus:
						bFound = True
						bAnyFound = True
					j += 1
			# <advc.905b>
			if not bFound:
				for j in range(gc.getNUM_UNIT_PREREQ_OR_BONUSES()):
					if gc.getUnitInfo(eLoopUnit).getSpeedBonuses(j) == self.iBonus:
						bFound = True
						bAnyFound = True
						break
			# </advc.905b>
			if bFound:
				szButton = gc.getUnitInfo(eLoopUnit).getButton()
				# <advc.003l>
				if iActivePlayer >= 0:
					szButton = gc.getPlayer(iActivePlayer).getUnitButton(eLoopUnit)
				# </advc.003l>
				screen.attachImageButton( panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, eLoopUnit, 1, False )
		# <!-- custom: placeBuildings is now renamed to placeBuildingsAndProjects -->
		# advc.905b: Allowed buildings moved to placeBuildings

		if not bAnyFound:
			draw_none_text(screen, self.top, self.X_UNITS, self.Y_UNITS, self.W_UNITS, self.H_UNITS)

	def placeBuildingsAndProjects(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		# advc.004y: txt key was TXT_KEY_PEDIA_CATEGORY_BUILDING
		# <!-- custom: note: Now the buildings panel is larger as the units panel (is), useful for ressources like marble that could not fit all -->
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_CATEGORY_BUILDING_PROJECT", ()), "", False, True, self.X_BUILDINGS_AND_PROJECTS, self.Y_BUILDINGS_AND_PROJECTS, self.W_BUILDINGS_AND_PROJECTS, self.H_BUILDINGS_AND_PROJECTS, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")
		# <!-- custom: placeAllows is now renamed to placeUnits -->
		# advc.905b: Moved this loop from placeAllows, which only deals with units now.
		# <!-- custom: i don't know why the bAnyFound approach i tried in the other methods did not work for buildings and projects (was displaying "None" text even if we did have buttons unlike what we wanted), so using a bAnyDisplayed slightly different approach than in the other methods that use instead bAnyFound, functionnal result is the same that now indeed we can display "None" if no building is displayed, else display buttons instead and not the "None" text as intended. -->
		bAnyDisplayed = False
		for eLoopBuilding in range(gc.getNumBuildingInfos()):
			bFound = False
			if gc.getBuildingInfo(eLoopBuilding).getPrereqAndBonus() == self.iBonus:
				bFound = True
			else:
				j = 0
				while (not bFound and j < gc.getNUM_BUILDING_PREREQ_OR_BONUSES()):
					if gc.getBuildingInfo(eLoopBuilding).getPrereqOrBonuses(j) == self.iBonus:
						bFound = True
					j += 1
			if bFound:
				bAnyDisplayed = True
				screen.attachImageButton( panelName, "", gc.getBuildingInfo(eLoopBuilding).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, eLoopBuilding, 1, False )

		for iBuilding in range(gc.getNumBuildingInfos()):
			info = gc.getBuildingInfo(iBuilding)
			bShow = (info.getFreeBonus() == self.iBonus or info.getBonusHealthChanges(self.iBonus) > 0 or info.getBonusHappinessChanges(self.iBonus) > 0 or info.getBonusProductionModifier(self.iBonus) > 0)
			if not bShow:
				for eYield in range(YieldTypes.NUM_YIELD_TYPES):
					if info.getBonusYieldModifier(self.iBonus, eYield) > 0:
						bShow = True
						break
			if bShow:
				bAnyDisplayed = True
				screen.attachImageButton( panelName, "", info.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, 1, False )

		# <!-- custom: in the aluminium bonus, all projects show the apostolic palace in the tooltip, and redirect to the apostolic palace instead of the correct project.
		# Fixed with the help of chatgpt 5.2 thanks -->
		# <advc.004y>
		for iProject in range(gc.getNumProjectInfos()):
			info = gc.getProjectInfo(iProject)
			if info.getBonusProductionModifier(self.iBonus) != 0:
				bAnyDisplayed = True
				screen.attachImageButton(panelName, "", info.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROJECT, iProject, 1, False)
		# </advc.004y>

		if not bAnyDisplayed:
			draw_none_text(screen, self.top, self.X_BUILDINGS_AND_PROJECTS, self.Y_BUILDINGS_AND_PROJECTS, self.W_BUILDINGS_AND_PROJECTS, self.H_BUILDINGS_AND_PROJECTS)

	# <!-- custom: add some terrains (as of now minus some plot types), and features information, code based on multilist code in sevopedia religion that we also use in several places. -->
	def placeTerrains(self):
		xPanel = self.X_TERRAINS
		yPanel = self.Y_TERRAINS
		wPanel = self.W_TERRAINS
		hPanel = self.H_TERRAINS

		# <!-- custom: see the corresponding asset's XML code comment for details -->
		txtKeyPanel = "TXT_KEY_PEDIA_TERRAIN_BOOLEANS"

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

		# <!-- custom: core logic provided with the help of chatgpt thanks. -->
		for iTerrain in xrange(gc.getNumTerrainInfos()):
			# Check if bonus can appear on this terrain
			if gc.getBonusInfo(self.iBonus).isTerrain(iTerrain):
				screen.appendMultiListButton(rowListName, gc.getTerrainInfo(iTerrain).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TERRAIN, iTerrain, 1, False)

				isButtonFound = True
				#iButtonIndex += 1

		if not isButtonFound:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel)

	def placeFeatures(self):
		xPanel = self.X_FEATURES
		yPanel = self.Y_FEATURES
		wPanel = self.W_FEATURES
		hPanel = self.H_FEATURES

		# <!-- custom: see the corresponding asset's XML code comment for details -->
		txtKeyPanel = "TXT_KEY_PEDIA_FEATURE_BOOLEANS"

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

		# <!-- custom: core logic provided with the help of chatgpt thanks. -->
		for iFeature in xrange(gc.getNumFeatureInfos()):
			# Check if bonus can appear on this feature
			if gc.getBonusInfo(self.iBonus).isFeature(iFeature):
				screen.appendMultiListButton(rowListName, gc.getFeatureInfo(iFeature).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_FEATURE, iFeature, 1, False)

				isButtonFound = True
				#iButtonIndex += 1

		if not isButtonFound:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel)

	# <!-- custom: also show Bonuses available through FeatureTerrainBooleans, such as as of now bonus_gemstones being available in grassland forest, but not in TerrainBooleans (no grassland entry there), so show this info here as well; code provided with the help of chatgpt thanks and such etc -->
	def placeFeatureTerrainBooleans(self):
		xPanel = self.X_FEATURE_TERRAIN_BOOLEANS
		yPanel = self.Y_FEATURE_TERRAIN_BOOLEANS
		wPanel = self.W_FEATURE_TERRAIN_BOOLEANS
		hPanel = self.H_FEATURE_TERRAIN_BOOLEANS

		# <!-- custom: see the corresponding asset's XML code comment for details -->
		txtKeyPanel = "TXT_KEY_PEDIA_FEATURE_TERRAIN_BOOLEANS"

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

		# <!-- custom: core logic provided with the help of chatgpt thanks. -->
		for iTerrain in xrange(gc.getNumTerrainInfos()):
			if gc.getBonusInfo(self.iBonus).isFeatureTerrain(iTerrain):
				screen.appendMultiListButton(rowListName, gc.getTerrainInfo(iTerrain).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TERRAIN, iTerrain, 1, False)

				isButtonFound = True
				#iButtonIndex += 1

		if not isButtonFound:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel)

	def placeSpecial(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

		# <!-- custom: code added with the help of claude ai thanks based on our existing code on advciv-sas and my prompt too or such -->
		if IS_SHOW_AI_INFO:
			txtKeyPanel = "TXT_KEY_PEDIA_EFFECTS_WITH_SOME_AI_INFORMATION"
		else:
			txtKeyPanel = "TXT_KEY_PEDIA_EFFECTS"

		screen.addPanel(panelName, localText.getText(txtKeyPanel, ()), "", True, False, self.X_SPECIAL, self.Y_SPECIAL, self.W_SPECIAL, self.H_SPECIAL, PanelStyles.PANEL_STYLE_BLUE50)

		listName = self.top.getNextWidgetName()
		szSpecialText = CyGameTextMgr().getBonusHelp(self.iBonus, True)[1:]

		if IS_SHOW_AI_INFO:
			bullet = localText.getText("[ICON_BULLET]", ())

			# Get bonus info
			bonusInfo = gc.getBonusInfo(self.iBonus)

			# Add iAIObjective
			aiObjective = bonusInfo.getAIObjective()

			# <!-- custom: note: separate info from other non AI entries more cleanly / clearly, so on more new line for first AI entry -->
			if szSpecialText.strip():
				szSpecialText += "\n\n"
			szSpecialText += "%siAIObjective: %d" % (bullet, aiObjective)

			# Add iAITradeModifier
			aiTradeModifier = bonusInfo.getAITradeModifier()
			szSpecialText += "\n%siAITradeModifier: %d" % (bullet, aiTradeModifier)

		screen.addMultilineText(listName, SASTextScale.labelText(szSpecialText), self.X_SPECIAL+5, self.Y_SPECIAL+30, self.W_SPECIAL-10, self.H_SPECIAL-35, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeRevealedBy(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		# <!-- custom: note that TXT_KEY_PEDIA_BONUS_APPEARANCE is "Reveals", using a custom TXT KEY "Revealed By" TXT_KEY_PEDIA_BONUS_APPEARANCE_REVEALED_BY_CUSTOM specifically for the Sevopedia, because now that we formatted it like that, "Revealed By" would fit better than "Reveals".
		# Stil keeping the old entry intact for the Civilopedia note: "Appearance"/"Reveals" is the logic it seems of an appearance but i still find it very confusing, why not use the same name, but as long as it works is maybe fine -->
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_BONUS_APPEARANCE_REVEALED_BY_CUSTOM", ()), "", False, True, self.X_REVEALED_BY, self.Y_REVEALED_BY, self.W_REVEALED_BY, self.H_REVEALED_BY, PanelStyles.PANEL_STYLE_BLUE50 )
		screen.attachLabel(panelName, "", "  ")

		iTech = gc.getBonusInfo(self.iBonus).getTechReveal()
		if iTech > -1:
			screen.attachImageButton( panelName, "", gc.getTechInfo(iTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, 1, False )
			# <!-- custom: we don't need the extra "(Reveals)" or "(Enables)" texts now that they are both not a "requires" anymore (which i think was very inaccurate, now fixed -->
			#screen.attachLabel(panelName, "", u"(" + localText.getText("TXT_KEY_PEDIA_BONUS_APPEARANCE", ()) + u")")
		else:
			draw_none_text(screen, self.top, self.X_REVEALED_BY, self.Y_REVEALED_BY, self.W_REVEALED_BY, self.H_REVEALED_BY, "TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_ALWAYS")

	def placeTradeableSince(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_BONUS_TRADE", ()), "", False, True, self.X_TRADEABLE_SINCE, self.Y_TRADEABLE_SINCE, self.W_TRADEABLE_SINCE, self.H_TRADEABLE_SINCE, PanelStyles.PANEL_STYLE_BLUE50 )
		screen.attachLabel(panelName, "", "  ")

		iTech = gc.getBonusInfo(self.iBonus).getTechCityTrade()
		if iTech > -1:
			screen.attachImageButton( panelName, "", gc.getTechInfo(iTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, 1, False )
			# <!-- custom: same as in placeRevealedBy -->
			#screen.attachLabel(panelName, "", u"(" + localText.getText("TXT_KEY_PEDIA_BONUS_TRADE", ()) + u")")
		else:
			draw_none_text(screen, self.top, self.X_TRADEABLE_SINCE, self.Y_TRADEABLE_SINCE, self.W_TRADEABLE_SINCE, self.H_TRADEABLE_SINCE, "TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_ALWAYS")

	# Places the tech that obsoletes this bonus resource in the Sevopedia.
	def placeObsoleteWith(self):
		screen = self.top.getScreen()

		# Get the current bonus
		bonusInfo = gc.getBonusInfo(self.iBonus)
		# Get the tech that obsoletes this bonus
		iTechObsolete = bonusInfo.getTechObsolete()
		techInfo = gc.getTechInfo(iTechObsolete)

		# Set up the panel
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_BONUS_OBSOLETE_WITH", ()), "", False, True, self.X_OBSOLETE_WITH, self.Y_OBSOLETE_WITH, self.W_OBSOLETE_WITH, self.H_OBSOLETE_WITH, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		if techInfo > -1:
			# Add the tech button directly to the panel
			szButton = techInfo.getButton()
			screen.attachImageButton(panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTechObsolete, 1, False)
		else:
			draw_none_text(screen, self.top, self.X_OBSOLETE_WITH, self.Y_OBSOLETE_WITH, self.W_OBSOLETE_WITH, self.H_OBSOLETE_WITH, "TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_NEVER")

	def setHistoryExpanded(self, bExpanded):
		self.bHistoryExpanded = bExpanded

	def setContentExpanded(self, bExpanded):
		self.bContentExpanded = bExpanded

	def placeBonusAnimation(self):
		screen = self.top.getScreen()
		iAnimX, iAnimY, iAnimW, iAnimH = draw_expandable_content_panel_container(screen, self.top, u"", self.X_BONUS_ANIMATION, self.Y_BONUS_ANIMATION, self.W_BONUS_ANIMATION, self.H_BONUS_ANIMATION, self.bContentExpanded, SAS_MAGIC_PEDIA_PYTHON_CONTENT_EXPAND, SAS_MAGIC_PEDIA_PYTHON_CONTENT_RELOAD)
		screen.addBonusGraphicGFC(self.top.getNextWidgetName(), self.iBonus, iAnimX, iAnimY, iAnimW, iAnimH, WidgetTypes.WIDGET_GENERAL, -1, -1, X_ROTATION_ANIMATION, Z_ROTATION_ANIMATION, self.SCALE_ANIMATION, True)

	def placeHistory(self):
		screen = self.top.getScreen()
		szText = gc.getBonusInfo(self.iBonus).getCivilopedia()
		szTitle = localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ())
		draw_expandable_text_panel(screen, self.top, szTitle, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, szText, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)
	def handleInput (self, inputClass):
		return 0
