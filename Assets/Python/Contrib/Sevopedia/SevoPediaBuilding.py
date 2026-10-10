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
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: Long_Comments_py.txt #4 -->

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

class SevoPediaBuilding:

	def __init__(self, main):
		self.iBuilding = -1
		self.bHistoryExpanded = False
		self.bContentExpanded = False
		self.top = main

		self.X_BUILDING_PANE = self.top.X_PEDIA_PAGE
		self.Y_BUILDING_PANE = self.top.Y_PEDIA_PAGE
		self.W_BUILDING_PANE = (self.top.R_PEDIA_PAGE - self.X_BUILDING_PANE - MEDIUM_MARGIN) / 2
		self.H_BUILDING_PANE = 190

		self.W_ICON = PANE_ICON_SIZE
		self.H_ICON = PANE_ICON_SIZE
		# <!-- custom: if PANE_ICON_SIZE is small (e.g. 64), start at the center of self.X_BUILDING_PANE, but if PANE_ICON_SIZE is big (e.g. 164) start at the left most part of self.X_BUILDING_PANE; same reasoning for Y position -->
		self.X_ICON = self.X_BUILDING_PANE + (PANE_ICON_FRAME_SIZE - PANE_ICON_SIZE) / 2
		self.Y_ICON = self.Y_BUILDING_PANE + (self.H_BUILDING_PANE - self.H_ICON) / 2

		# <!-- custom: add an extra margin to accommodate the potentially larger PANE_ICON_SIZE (than for example 64), if diff is 0 this is harmless to keep too so is dynamical code that can handle optionally larger PANE_ICON_SIZE (vs old PANE_ICON_SIZE of 64) that you may keep or remove as you prefer -->
		self.SMALLER_ICON_SIZE_THAN_ICON_FRAME_MARGIN = (PANE_ICON_FRAME_SIZE - PANE_ICON_SIZE) / 2

		self.STATS_PANE_LEFT_SIDE_MARGIN = 0
		self.STATS_PANE_UPPER_PADDING = 38

		self.X_STATS_PANE = self.X_BUILDING_PANE + self.STATS_PANE_LEFT_SIDE_MARGIN + self.W_ICON + (2 * self.SMALLER_ICON_SIZE_THAN_ICON_FRAME_MARGIN)
		self.Y_STATS_PANE = self.Y_BUILDING_PANE + self.STATS_PANE_UPPER_PADDING
		self.W_STATS_PANE = self.W_BUILDING_PANE - self.W_ICON - (2 * self.SMALLER_ICON_SIZE_THAN_ICON_FRAME_MARGIN) - self.STATS_PANE_LEFT_SIDE_MARGIN
		self.H_STATS_PANE = self.H_BUILDING_PANE - self.STATS_PANE_UPPER_PADDING

		self.H_STATS_PANE_LINE_HEIGHT = 38

		self.X_FLAT_GREAT_PERSON = None
		self.Y_FLAT_GREAT_PERSON = None

		# <!-- custom: see sevopediaunit's self.W_TOTAL_EFFECTIVE_UNIT_PANE for differences in implementation -->
		self.W_TOTAL_EFFECTIVE_BUILDING_PANE = self.W_BUILDING_PANE

		self.W_MOVIE = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)

		self.X_REQUIRES = self.X_BUILDING_PANE
		self.Y_REQUIRES = self.Y_BUILDING_PANE + self.H_BUILDING_PANE + SMALL_MARGIN
		self.W_REQUIRES = self.W_TOTAL_EFFECTIVE_BUILDING_PANE - self.W_MOVIE - MEDIUM_MARGIN
		self.H_REQUIRES = NON_MULTILIST_PANEL_STANDARD_HEIGHT

		self.W_OBSOLETE_WITH = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.H_MOVIE = self.H_REQUIRES

		self.X_MOVIE = self.X_REQUIRES + self.W_REQUIRES + MEDIUM_MARGIN
		self.Y_MOVIE = self.Y_REQUIRES
		self.playButtonPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_PLAY_BUTTON").getPath()
		self.powerButtonPath = ArtFileMgr.getInterfaceArtInfo("SAS_POWER").getPath()

		self.X_REQUIRED_FOR = self.X_BUILDING_PANE
		self.Y_REQUIRED_FOR = self.Y_REQUIRES + self.H_REQUIRES + SMALL_MARGIN
		self.W_REQUIRED_FOR = self.W_TOTAL_EFFECTIVE_BUILDING_PANE - self.W_OBSOLETE_WITH - MEDIUM_MARGIN
		self.H_REQUIRED_FOR = self.H_REQUIRES

		self.X_OBSOLETE_WITH = self.X_REQUIRED_FOR + self.W_REQUIRED_FOR + MEDIUM_MARGIN
		self.Y_OBSOLETE_WITH = self.Y_REQUIRED_FOR
		self.H_OBSOLETE_WITH = self.H_REQUIRED_FOR

		self.W_FREE_WITH = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)

		self.X_FREE_PBBS = self.X_BUILDING_PANE
		self.Y_FREE_PBBS = self.Y_REQUIRED_FOR + self.H_REQUIRED_FOR + SMALL_MARGIN
		self.W_FREE_PBBS = self.W_TOTAL_EFFECTIVE_BUILDING_PANE - self.W_FREE_WITH - MEDIUM_MARGIN
		self.H_FREE_PBBS = self.H_REQUIRES

		self.X_FREE_WITH = self.X_FREE_PBBS + self.W_FREE_PBBS + MEDIUM_MARGIN
		self.Y_FREE_WITH = self.Y_FREE_PBBS
		self.H_FREE_WITH = self.H_REQUIRES

		self.X_SPECIAL = self.X_BUILDING_PANE
		self.Y_SPECIAL = self.Y_FREE_PBBS + self.H_FREE_PBBS + SMALL_MARGIN
		self.W_SPECIAL = self.W_TOTAL_EFFECTIVE_BUILDING_PANE
		self.H_SPECIAL = self.top.B_PEDIA_PAGE - self.Y_SPECIAL

		self.H_ADJUST_HEIGHT_ANIMATION_TO_MATCH_ADJACENT_PANE = 7

		self.X_BUILDING_ANIMATION = self.X_BUILDING_PANE + self.W_TOTAL_EFFECTIVE_BUILDING_PANE + MEDIUM_MARGIN
		self.Y_BUILDING_ANIMATION = self.Y_BUILDING_PANE + self.H_ADJUST_HEIGHT_ANIMATION_TO_MATCH_ADJACENT_PANE
		self.W_BUILDING_ANIMATION = self.W_TOTAL_EFFECTIVE_BUILDING_PANE
		self.H_BUILDING_ANIMATION = self.H_BUILDING_PANE + SMALL_MARGIN + self.H_REQUIRES + SMALL_MARGIN + self.H_FREE_PBBS - self.H_ADJUST_HEIGHT_ANIMATION_TO_MATCH_ADJACENT_PANE

		self.SCALE_ANIMATION = 0.7

		self.W_CIVILIZATIONS = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)

		self.X_REPLACE = self.X_BUILDING_ANIMATION
		self.Y_REPLACE = self.Y_BUILDING_ANIMATION + self.H_BUILDING_ANIMATION + SMALL_MARGIN
		self.W_REPLACE = self.W_BUILDING_ANIMATION - MEDIUM_MARGIN - self.W_CIVILIZATIONS
		self.H_REPLACE = self.H_REQUIRES

		self.X_CIVILIZATIONS = self.X_BUILDING_ANIMATION + self.W_REPLACE + MEDIUM_MARGIN
		self.Y_CIVILIZATIONS = self.Y_REPLACE
		self.H_CIVILIZATIONS = self.H_REPLACE

		self.X_HISTORY = self.X_BUILDING_ANIMATION
		self.Y_HISTORY = self.Y_CIVILIZATIONS + self.H_CIVILIZATIONS + SMALL_MARGIN
		self.W_HISTORY = self.W_BUILDING_ANIMATION
		self.H_HISTORY = self.top.B_PEDIA_PAGE - self.Y_HISTORY

		# <!-- custom: cached vanilla define for cost display in stats panel. (Claude code Sonnet 4.6) -->
		self.iBUILDING_PRODUCTION_PERCENT = None

	def interfaceScreen(self, iBuilding):
		if self.iBuilding != iBuilding:
			self.bHistoryExpanded = False
			self.bContentExpanded = False
		self.iBuilding = iBuilding

		if self.bContentExpanded:
			self.placeBuildingAnimation()
			return

		self.placeBuildingPane()
		self.placeStats()
		self.placeRequires()
		self.placeMovie()
		self.placeRequiredFor()
		self.placeObsoleteWith()
		self.placeFreePBBS()
		self.placeFreeWith()
		self.placeSpecial()
		if not self.bHistoryExpanded:
			self.placeBuildingAnimation()
		self.placeReplace()
		self.placeCivilizations()
		self.placeHistory()
		place_new_concept_legend_link(self.top, "CONCEPT_SAS_SEVOPEDIA_NUMTXT_LEGEND")

	def placeBuildingPane(self):
		screen = self.top.getScreen()

		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_BUILDING_PANE, self.Y_BUILDING_PANE, self.W_TOTAL_EFFECTIVE_BUILDING_PANE, self.H_BUILDING_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_MAIN)
		screen.addDDSGFC(self.top.getNextWidgetName(), gc.getBuildingInfo(self.iBuilding).getButton(), self.X_ICON + self.W_ICON/2 - PANE_ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - PANE_ICON_SIZE/2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1 )

	def setupStatsPanel(self, screen, panelName, txtKey, panelStyle):
		screen.addPanel(panelName, localText.getText(txtKey, ()), "", True, True, self.X_STATS_PANE, self.Y_STATS_PANE, self.W_STATS_PANE, self.H_STATS_PANE, panelStyle,)

	def fillStatsCell(self, screen, label, xLabel, y):
		labelText = SASTextScale.titleText(label)
		screen.setText(self.top.getNextWidgetName(), "", labelText, CvUtil.FONT_LEFT_JUSTIFY, xLabel, y, 0, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def getStatsNextItemCoordinates(self, x, y, rowItemId, columnWidth):
		anticipatedNextRowId = rowItemId + 1
		anticipatedNextRowNum = anticipatedNextRowId + 1
		if (anticipatedNextRowNum * self.H_STATS_PANE_LINE_HEIGHT > (self.H_STATS_PANE - self.STATS_PANE_UPPER_PADDING)):
			# <!-- custom: move to next column and back to first row, reset columItemId to 0 -->
			x = x + columnWidth
			y = self.Y_STATS_PANE
			rowItemId = 0
			return x, y, rowItemId
		else:
			# <!-- custom: else x remains the same, and we go to next row (we stay in same column as long as we have room) -->
			y += self.H_STATS_PANE_LINE_HEIGHT
			rowItemId += 1
			return x, y, rowItemId

	# <!-- custom: table code based on placeAIPersonality panel method/function in sevopedialeader we (me and chatgpt) had written and enhanced together and all, modifying/adjusting it for this sevopediabuilding (much) simpler panel (stats pane) need but still important as we don't want to scroll after say 4th element, move to 2nd column rather and resume filling there. -->
	def placeStats(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		placeStatsTxtKeyPanel = ""

		buildingInfo = gc.getBuildingInfo(self.iBuilding)

		numColumns = 3
		columnWidth = self.W_STATS_PANE / numColumns

		# <!-- custom: Long_Comments_py.txt #6 -->

		# <!-- custom: blue panel style PanelStyles.PANEL_STYLE_BLUE50 is/can be useful for debugging, otherwise we don't need a blue on blue color, prefer transparent ("EMPTY") -->
		self.setupStatsPanel(screen, panelName, placeStatsTxtKeyPanel, PanelStyles.PANEL_STYLE_EMPTY)

		x = self.X_STATS_PANE
		y = self.Y_STATS_PANE
		rowItemId = 0

		# <!-- custom: it seems we never use the code below and i don't understand too well what it is for, but especially in our placeStats pane i don't think we use it at all, so commenting it out; Long_Comments_py.txt #5 -->

		# <!-- custom: 1: Cost -->
		if buildingInfo.getProductionCost() > 0:
			if self.iBUILDING_PRODUCTION_PERCENT is None:
				self.iBUILDING_PRODUCTION_PERCENT = gc.getDefineINT("BUILDING_PRODUCTION_PERCENT")
			buildingCost = (buildingInfo.getProductionCost() * self.iBUILDING_PRODUCTION_PERCENT)/100
			if self.top.iActivePlayer != -1:
				buildingCost = gc.getPlayer(self.top.iActivePlayer).getBuildingProductionNeeded(self.iBuilding)

			szCost = localText.getText("TXT_KEY_PEDIA_COST_CUSTOM", (buildingCost,))
			szText2 = u"%c  %s" % (gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar(), szCost)
			self.fillStatsCell(screen, szText2, x, y)
			x, y, rowItemId = self.getStatsNextItemCoordinates(x, y, rowItemId, columnWidth)

		# <!-- custom: 2 Direct Yield Changes (like Food, Production, Gold), and Yield Modifiers (Food +x%, Production +x%, Gold +x%) with power breakdown added thanks to Claude AI and my prompts or tweaks/adjustments or not or yes or and but or not but or and(2) -->
		for k in range(YieldTypes.NUM_YIELD_TYPES):
			iYieldChange = buildingInfo.getYieldChange(k)

			if iYieldChange != 0:
				if iYieldChange > 0:
					szSign = "+"
				else:
					szSign = ""
				szText1 = szSign + str(iYieldChange)
				szText2 = u"%c  %s" % (gc.getYieldInfo(k).getChar(), szText1)
				self.fillStatsCell(screen, szText2, x, y)
				x, y, rowItemId = self.getStatsNextItemCoordinates(x, y, rowItemId, columnWidth)

			# <!-- custom: also adding yield modifiers in the same loop as yield changes for efficiency but also main reason would also beto display for example hammer +25% just after hammer +1 for example, and not after after gold + 5 -->
			iYieldModifier = buildingInfo.getYieldModifier(k)
			iPowerYieldModifier = buildingInfo.getPowerYieldModifier(k)

			# Total modifier (regular + power)
			iTotalYieldModifier = iYieldModifier + iPowerYieldModifier

			if iTotalYieldModifier != 0:
				szText1 = ""

				# Base modifier part
				if iYieldModifier != 0:
					if iYieldModifier > 0:
						szSign = "+"
					else:
						szSign = ""
					szText1 = szSign + str(iYieldModifier) + "%"

				# Power modifier part (optional, only if exists)
				if iPowerYieldModifier != 0:
					if len(szText1) > 0:
						szText1 += ", "
					if iPowerYieldModifier > 0:
						szPowerSign = "+"
					else:
						szPowerSign = ""
					szText1 += szPowerSign + str(iPowerYieldModifier) + "% w/"

					# Add power button
					buttonSize = 24
					szButtonText = u"<img=%s size=%s></img>" % (self.powerButtonPath, str(buttonSize))
					szText1 += szButtonText

				szText2 = u"%c  %s" % (gc.getYieldInfo(k).getChar(), szText1)
				self.fillStatsCell(screen, szText2, x, y)
				x, y, rowItemId = self.getStatsNextItemCoordinates(x, y, rowItemId, columnWidth)

		# <!-- custom: 3: Commerce Changes (Science, Gold, Culture, Espionage), and Commerce Modifiers and Double Times (3) -->
		for k in range(CommerceTypes.NUM_COMMERCE_TYPES):
			iTotalCommerce = buildingInfo.getObsoleteSafeCommerceChange(k) + buildingInfo.getCommerceChange(k)
			if iTotalCommerce != 0:
				if iTotalCommerce > 0:
					szSign = "+"
				else:
					szSign = ""
				szText1 = szSign + str(iTotalCommerce)
				szText2 = u"%c  %s" % (gc.getCommerceInfo(k).getChar(), szText1)
				self.fillStatsCell(screen, szText2, x, y)
				x, y, rowItemId = self.getStatsNextItemCoordinates(x, y, rowItemId, columnWidth)

			# <!-- custom: also process the modifiers associated to the raw commerce changes just after the raw value (for example culture +1 then the next line is immediately culture +50% and only then/after gold + 1 for example (and now not culture + 1 then gold + 1 and only then culture + 50%), this is how it would work this code) -->
			iCommerceModifier = buildingInfo.getCommerceModifier(k)
			iCommerceDoubleTime = buildingInfo.getCommerceChangeDoubleTime(k)
			iGlobalCommerceModifier = buildingInfo.getGlobalCommerceModifier(k)

			# <!-- custom: placeSpecial (already) handles the full info display (currently not double times though), so we can simply be concise maybe and display the total of "local" (if any (too than in global that i wrote the if any of before)) + global commerce modifier (if any) rather -->
			# Total modifier (local + global)
			iTotalModifier = iCommerceModifier + iGlobalCommerceModifier

			# Display if either modifier or double time exists
			if iTotalModifier != 0 or iCommerceDoubleTime > 0:
				szText = ""

				# Add modifier percentage
				if iTotalModifier != 0:
					if iTotalModifier > 0:
						szSign = "+"
					else:
						szSign = ""
					szText += szSign + str(iTotalModifier) + "%"

				# Add double time if present
				if iCommerceDoubleTime > 0:
					if len(szText) > 0:
						szText += ", "
					szText += "x2(" + str(iCommerceDoubleTime) + "Y)"

				szText2 = u"%c  %s" % (gc.getCommerceInfo(k).getChar(), szText)
				self.fillStatsCell(screen, szText2, x, y)
				x, y, rowItemId = self.getStatsNextItemCoordinates(x, y, rowItemId, columnWidth)

		# <!-- custom: 4: Happiness or Unhappiness (4) -->
		iHappiness = buildingInfo.getHappiness()
		if self.top.iActivePlayer != -1:
			if self.iBuilding == gc.getCivilizationInfo(gc.getPlayer(self.top.iActivePlayer).getCivilizationType()).getCivilizationBuildings(buildingInfo.getBuildingClassType()):
				iHappiness += gc.getPlayer(self.top.iActivePlayer).getExtraBuildingHappiness(self.iBuilding)

		if iHappiness > 0:
			szText = localText.getText("TXT_KEY_PEDIA_HAPPY_CUSTOM", (iHappiness,))
			szText2 = u"%c  %s" % (CyGame().getSymbolID(FontSymbols.HAPPY_CHAR), szText)
			self.fillStatsCell(screen, szText2, x, y)
			x, y, rowItemId = self.getStatsNextItemCoordinates(x, y, rowItemId, columnWidth)
		elif iHappiness < 0:
			szText = localText.getText("TXT_KEY_PEDIA_UNHAPPY_CUSTOM", (-iHappiness,))
			szText2 = u"%c  %s" % (CyGame().getSymbolID(FontSymbols.UNHAPPY_CHAR), szText)
			self.fillStatsCell(screen, szText2, x, y)
			x, y, rowItemId = self.getStatsNextItemCoordinates(x, y, rowItemId, columnWidth)

		# <!-- custom: 5: -->
		iHealth = buildingInfo.getHealth()
		if self.top.iActivePlayer != -1:
			if self.iBuilding == gc.getCivilizationInfo(gc.getPlayer(self.top.iActivePlayer).getCivilizationType()).getCivilizationBuildings(buildingInfo.getBuildingClassType()):
				iHealth += gc.getPlayer(self.top.iActivePlayer).getExtraBuildingHealth(self.iBuilding)

		if iHealth > 0:
			szText = localText.getText("TXT_KEY_PEDIA_HEALTHY_CUSTOM", (iHealth,))
			szText2 = u"%c  %s" % (CyGame().getSymbolID(FontSymbols.HEALTHY_CHAR), szText)
			self.fillStatsCell(screen, szText2, x, y)
			x, y, rowItemId = self.getStatsNextItemCoordinates(x, y, rowItemId, columnWidth)
		elif iHealth < 0:
			szText = localText.getText("TXT_KEY_PEDIA_UNHEALTHY_CUSTOM", (-iHealth,))
			szText2 = u"%c  %s" % (CyGame().getSymbolID(FontSymbols.UNHEALTHY_CHAR), szText)
			self.fillStatsCell(screen, szText2, x, y)
			x, y, rowItemId = self.getStatsNextItemCoordinates(x, y, rowItemId, columnWidth)

		# <!-- custom: 6: Great people change with button display of the great people type too, and great people modifier -->
		if buildingInfo.getGreatPeopleRateChange() != 0:
			# Create the text with the great person rate change
			szText = localText.getText("TXT_KEY_PEDIA_GREAT_PEOPLE_CUSTOM", (buildingInfo.getGreatPeopleRateChange(),))

			# Format with the great people character
			szText2 = u"%c  %s" % (CyGame().getSymbolID(FontSymbols.GREAT_PEOPLE_CHAR), szText)

			# Display the text
			self.fillStatsCell(screen, szText2, x, y)
			# <!-- custom: since this is our last usage/placeStats info displayed, we don't get the next coordinates, but instead store current coordinates (of last item displayed) to know where to place our great people button later. -->
			self.X_FLAT_GREAT_PERSON = x
			self.Y_FLAT_GREAT_PERSON = y
			x, y, rowItemId = self.getStatsNextItemCoordinates(x, y, rowItemId, columnWidth)

		# <!-- custom: also add Great People Modifier (similar to how we handle yield modifiers) -->
		iGreatPeopleModifier = buildingInfo.getGreatPeopleRateModifier()
		iGlobalGreatPeopleModifier = buildingInfo.getGlobalGreatPeopleRateModifier()

		# Total modifier (local + global)
		iTotalGreatPeopleModifier = iGreatPeopleModifier + iGlobalGreatPeopleModifier

		if iTotalGreatPeopleModifier != 0:
			szText1 = ""

			# Base modifier part
			if iGreatPeopleModifier != 0:
				if iGreatPeopleModifier > 0:
					szSign = "+"
				else:
					szSign = ""
				szText1 = szSign + str(iGreatPeopleModifier) + "%"

			# Global modifier part (if exists and different from base)
			if iGlobalGreatPeopleModifier != 0:
				if len(szText1) > 0:
					szText1 += ", "
				if iGlobalGreatPeopleModifier > 0:
					szGlobalSign = "+"
				else:
					szGlobalSign = ""
				szText1 += szGlobalSign + str(iGlobalGreatPeopleModifier) + "% (Global)"

			szText2 = u"%c  %s" % (CyGame().getSymbolID(FontSymbols.GREAT_PEOPLE_CHAR), szText1)
			self.fillStatsCell(screen, szText2, x, y)
			x, y, rowItemId = self.getStatsNextItemCoordinates(x, y, rowItemId, columnWidth)

		# <!-- custom: now that textual and placeStats display is finished (minus Great People info), add the button now. -->
		# Only proceed if this building affects great people rate
		if buildingInfo.getGreatPeopleRateChange() != 0:
			buttonXOffset = 66
			# <!-- custom: slightly shift the button if button size (w and h) are higher than 32 to simulate a better centering effect towards the text of "+2" (great people count example) for example, the buttonYOffset = +2 if it were same value is purely coincidental value that suited us to center the button, no correlation or link at least voluntary -->
			buttonYOffset = -2
			buttonW = 39
			buttonH = buttonW

			#  First get the building's great person type (e.g., SPECIALIST_GREAT_MERCHANT)
			greatPersonType = buildingInfo.getGreatPeopleUnitClass()
			greatPersonButton = None

			if greatPersonType != -1:
				# Get the unit info for the great person type
				if self.top.iActivePlayer != -1:
					iGreatPersonUnit = gc.getCivilizationInfo(gc.getPlayer(self.top.iActivePlayer).getCivilizationType()).getCivilizationUnits(greatPersonType)
				else:
					iGreatPersonUnit = gc.getUnitClassInfo(greatPersonType).getDefaultUnitIndex()

				if iGreatPersonUnit != -1:
					greatPersonInfo = gc.getUnitInfo(iGreatPersonUnit)
					greatPersonButton = greatPersonInfo.getButton()

				if greatPersonButton:
					buttonWidget = self.top.getNextWidgetName()
					# <!-- custom: add tooltip when hovering on the great person button, with the help of chatgpt 5.2 thanks -->
					# convert to coords relative to the stats panel (which starts at X_STATS_PANE / Y_STATS_PANE)
					buttonX = (self.X_FLAT_GREAT_PERSON + buttonXOffset) - self.X_STATS_PANE
					buttonY = (self.Y_FLAT_GREAT_PERSON + buttonYOffset) - self.Y_STATS_PANE
					screen.setImageButtonAt(buttonWidget, panelName, greatPersonButton, buttonX, buttonY, buttonW, buttonH, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iGreatPersonUnit, 1)

	# <!-- custom: additional info by chatgpt thanks: "The self.iBuilding is a unique ID already.
	# But the prerequisites (like isBuildingClassNeededInCity) refer to a class, not a specific building.
	# That's where the helper comes in."
	# + also "The helper get_iDefaultBuilding_current_civ(iBuildingClass) is not for the current building (self.iBuilding).
	# It's used to resolve prerequisite buildings by class — and each building class can have different versions (UUs) for each civ."
	# i don't know if accurate but maybe is, so adding this info here as part of refactoring and wondering if we should use it in required for to which chatgpt also replied thanks but or not but or yes but"In placeRequiredFor: You’re checking: for each building: if building X requires our current building's class: show building X" and "You already have the concrete building (X).
	# No need to resolve anything — you are showing the building that depends on yours, not the class." -->
	def get_iDefaultBuilding_current_civ(self, i):
		# Get the default building of this class for the current civilization
		if self.top.iActivePlayer != -1:
			return gc.getCivilizationInfo(gc.getPlayer(self.top.iActivePlayer).getCivilizationType()).getCivilizationBuildings(i)
		else:
			return gc.getBuildingClassInfo(i).getDefaultBuildingIndex()

	def placeRequires(self):
		xPanel = self.X_REQUIRES
		yPanel = self.Y_REQUIRES
		wPanel = self.W_REQUIRES
		hPanel = self.H_REQUIRES

		txtKeyPanel = "TXT_KEY_PEDIA_REQUIRES"

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
		iButtonIndex = 0
		# <!-- custom: SEVOPEDIA_MULTILIST_NUM_LISTS_AUTO_CALCULATE --> =1 in your case (auto-fit); so we calculate column layout manually -->
		maxButtonsPerRow = get_multilist_max_buttons_per_row(multiListW, MULTILIST_BUTTON_SIZE)

		for iPrereqTech in xrange(gc.getNumTechInfos()):
			if isTechRequiredForBuilding(iPrereqTech, self.iBuilding):
				screen.appendMultiListButton(rowListName, gc.getTechInfo(iPrereqTech).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iPrereqTech, 1, False)

				isButtonFound = True
				# <!-- custom: note: no specific text, no need for numTxt and such, simply display the button then end this part of the code, but we still increment so that buttons that buttons that need/use a numTxt under have their numTxt correctly positioned -->
				iButtonIndex += 1

		iPrereqAndBonus = gc.getBuildingInfo(self.iBuilding).getPrereqAndBonus()
		if iPrereqAndBonus >= 0:
			screen.appendMultiListButton(rowListName, gc.getBonusInfo(iPrereqAndBonus).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iPrereqAndBonus, 1, False)

			isButtonFound = True
			iButtonIndex += 1

		for k in range(gc.getNUM_BUILDING_PREREQ_OR_BONUSES()):
			iPrereqOrBonus = gc.getBuildingInfo(self.iBuilding).getPrereqOrBonuses(k)
			if iPrereqOrBonus >= 0:
				screen.appendMultiListButton(rowListName, gc.getBonusInfo(iPrereqOrBonus).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iPrereqOrBonus, 1, False)

				isButtonFound = True
				iButtonIndex += 1

		iCorporation = gc.getBuildingInfo(self.iBuilding).getFoundsCorporation()
		bFirst = True
		if iCorporation >= 0:
			for k in range(gc.getNUM_CORPORATION_PREREQ_BONUSES()):
				iPrereqCorporationBonus = gc.getCorporationInfo(iCorporation).getPrereqBonus(k)
				if iPrereqCorporationBonus >= 0:
					# <!-- custom: in all cases (whether there is a numTxt or not to place as well), place the button -->
					screen.appendMultiListButton(rowListName, gc.getBonusInfo(iPrereqCorporationBonus).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iPrereqCorporationBonus, 1, False)

					# <!-- custom: then before incrementing the position for the next numTxt, handle the "or" numTxt placement of the current button we just placed -->
					if not bFirst:
						numTxt = localText.getText("TXT_KEY_OR", ())
						# <!-- custom: "or" numTxt aligned between this button and the previous one -->
						extraCorrectionX = get_extra_correction_x(numTxt) + get_extra_correction_x_inbetween_buttons(MULTILIST_BUTTON_SIZE)
						add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)
					else:
						bFirst = False

					# <!-- custom: note: increment only after the "or" numTxt is placed so that it is just inbetween this button and the next one, not in between next one and the one after next one -->
					isButtonFound = True
					iButtonIndex += 1

		iPrereqReligion = gc.getBuildingInfo(self.iBuilding).getPrereqReligion()
		if iPrereqReligion >= 0:
			screen.appendMultiListButton(rowListName, gc.getReligionInfo(iPrereqReligion).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION, iPrereqReligion, 1, False)

			isButtonFound = True
			iButtonIndex += 1

		# Check for project requirements - New code for Manhattan Project and other projects
		buildingInfo = gc.getBuildingInfo(self.iBuilding)
		iSpecialBuildingType = buildingInfo.getSpecialBuildingType()

		# Check all projects to see if any enables this special building
		if iSpecialBuildingType >= 0:
			bFirst = True

			for iProject in range(gc.getNumProjectInfos()):
				projectInfo = gc.getProjectInfo(iProject)
				if projectInfo.getEveryoneSpecialBuilding() == iSpecialBuildingType:
					screen.appendMultiListButton(rowListName, projectInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROJECT, iProject, 1, False)

					if not bFirst:
						# <!-- custom: workaround as our code doesn't handle inconsistent occurence size (a button is 64px currently if not always or not, but a label "or" would be smaller than 64px, messing the txtNums alignment of subsequent buttons (if any), so to not rewrite all our code or tweak it too deeply, maybe this alternative solution is/can be quite elegant too instead, of putting the "or" label rather as a txtNum just between the buttons, and belonging to the new 2nd button we are adding (no "or" if first project so maybe more sensical or intuitive this way even though is still a hack but maybe not so bad), not having thus to rewrite our otherwise working code. -->
						# Also since our next button is a project, if we were to use this "or", it is fine to be a bit more aggressive with the adjustment and place the txtNum right inbetween both buttons, as no txtNum will be left or right of this txtNum directly in contact with it and colliding or being merged in unintended way, so i quite like this elegant solution :) hehe. -->
						numTxt = localText.getText("TXT_KEY_OR", ())
						# <!-- custom: "or" numTxt aligned between this button and the previous one -->
						extraCorrectionX = get_extra_correction_x(numTxt) + get_extra_correction_x_inbetween_buttons(MULTILIST_BUTTON_SIZE)
						add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)
					else:
						bFirst = False

					isButtonFound = True
					iButtonIndex += 1

		# Check for required buildings
		# <!-- custom: see also for this part the note/code comment at top of this py file -->

		buildingInfo = gc.getBuildingInfo(self.iBuilding)
		for i in range(gc.getNumBuildingClassInfos()):
			# Check if this building class is needed in the city
			if buildingInfo.isBuildingClassNeededInCity(i):
				iDefaultBuilding = self.get_iDefaultBuilding_current_civ(i)

				# If a valid building exists, display its button
				if iDefaultBuilding != -1:
					screen.appendMultiListButton(rowListName, gc.getBuildingInfo(iDefaultBuilding).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iDefaultBuilding, 1, False)

					numTxt = "InC"
					extraCorrectionX = get_extra_correction_x(numTxt)
					add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

					isButtonFound = True
					iButtonIndex += 1

		# Handle PrereqBuildingClasses (buildings needed across civilization)
		for i in range(gc.getNumBuildingClassInfos()):
			iNumRequired = 0
			iNumRequired = buildingInfo.getPrereqNumOfBuildingClass(i)

			if iNumRequired > 0:
				iDefaultBuilding = self.get_iDefaultBuilding_current_civ(i)

				# If a valid building exists, display its button with number required
				if iDefaultBuilding != -1:
					screen.appendMultiListButton(rowListName, gc.getBuildingInfo(iDefaultBuilding).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iDefaultBuilding, 1, False)

					numTxt = "AC %s+R" % iNumRequired
					extraCorrectionX = get_extra_correction_x(numTxt)
					add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

					isButtonFound = True
					iButtonIndex += 1

		if not isButtonFound:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel)

	def placeMovie(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_MOVIE_PANEL", ()), "", False, True, self.X_MOVIE, self.Y_MOVIE, self.W_MOVIE, self.H_MOVIE, PanelStyles.PANEL_STYLE_BLUE50)

		# <!-- custom: use attachLabel for padding similar to Obsolete panel -->
		screen.attachLabel(panelName, "", "  ")

		iMovieType = SAS_MAGIC_PEDIA_MOVIE_TYPE_WONDER
		if self.top.pediaMovies.hasMovie(iMovieType, self.iBuilding):
			iPackedMovie = self.top.SAS_packMovieKey(iMovieType, self.iBuilding)
			buttonSize = 64
			# <!-- custom: setImageButtonAt positions relative to panel content area (below header).
			# X: Standard centering works correctly.
			# Y: Must be set to 10 (not calculated from panelHeaderHeight) - empirically determined positioning fix. (Claude code Sonnet 4.5) -->
			buttonX = (self.W_MOVIE - buttonSize) / 2
			buttonY = 10
			screen.setImageButtonAt(self.top.getNextWidgetName(), panelName, self.playButtonPath, buttonX, buttonY, buttonSize, buttonSize, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_MOVIE_ENTRY, iPackedMovie)
		else:
			draw_none_text(screen, self.top, self.X_MOVIE, self.Y_MOVIE, self.W_MOVIE, self.H_MOVIE)

	# <!-- custom: code with the help of gemini ai. -->
	def is_building_prereq_overridden_by_civic(self, iBuildingId):
		# Checks if the prerequisite for the given building ID can be overridden by any civic.
		# Returns True if an override exists, False otherwise.
		#
		buildingInfo = gc.getBuildingInfo(iBuildingId)
		iSpecialBuildingType = buildingInfo.getSpecialBuildingType()

		# Only special buildings can have their requirements overridden this way by civics
		if iSpecialBuildingType == -1:
			return False

		# Iterate through all civics
		for iCivic in range(gc.getNumCivicInfos()):
			loopCivicInfo = gc.getCivicInfo(iCivic)
			# Use the confirmed API method: isSpecialBuildingNotRequired(SpecialBuildingType eIndex)
			# Pass the integer ID directly, as Python bindings usually handle this.
			if loopCivicInfo.isSpecialBuildingNotRequired(iSpecialBuildingType):
				return True

		return False

	def placeRequiredFor(self):
		# Shows buildings that require this building as a prerequisite
		#
		xPanel = self.X_REQUIRED_FOR
		yPanel = self.Y_REQUIRED_FOR
		wPanel = self.W_REQUIRED_FOR
		hPanel = self.H_REQUIRED_FOR

		txtKeyPanel = "TXT_KEY_PEDIA_REQUIRED_FOR"

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
		iButtonIndex = 0
		maxButtonsPerRow = get_multilist_max_buttons_per_row(multiListW, MULTILIST_BUTTON_SIZE)

		# Get the building class of our current building
		iCurrentBuildingClass = gc.getBuildingInfo(self.iBuilding).getBuildingClassType()

		# Loop through all buildings to check which ones require this building class
		for iLoopBuilding in range(gc.getNumBuildingInfos()):
			loopBuildingInfo = gc.getBuildingInfo(iLoopBuilding)

			# Check if this building is needed via BuildingClassNeededs
			if loopBuildingInfo.isBuildingClassNeededInCity(iCurrentBuildingClass):
				screen.appendMultiListButton(rowListName, loopBuildingInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iLoopBuilding, 1, False)

				numTxt = "InC"
				extraCorrectionX = get_extra_correction_x(numTxt)
				add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

				isButtonFound = True
				iButtonIndex += 1

			# Check if this building is needed via PrereqBuildingClasses
			iNumRequired = loopBuildingInfo.getPrereqNumOfBuildingClass(iCurrentBuildingClass)
			if iNumRequired > 0:
				screen.appendMultiListButton(rowListName, loopBuildingInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iLoopBuilding, 1, False)

				numTxt = "AC %s+R" % iNumRequired
				extraCorrectionX = get_extra_correction_x(numTxt)
				add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

				isButtonFound = True
				iButtonIndex += 1

		# <!-- custom: code provided with the help of gemini ai. -->
		# Loop through all units to check their building prerequisites
		for iLoopUnit in range(gc.getNumUnitInfos()):
			loopUnitInfo = gc.getUnitInfo(iLoopUnit)
			iPrereqBuilding = loopUnitInfo.getPrereqBuilding()

			if iPrereqBuilding == self.iBuilding:
				screen.appendMultiListButton(rowListName, loopUnitInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iLoopUnit, 1, False)

				# <!-- custom: add numTxt info that this building prereq is or may beoverriden by some civics such as organized religion civic for missionaries in this case at least is the only one i can think of -->
				if self.is_building_prereq_overridden_by_civic(self.iBuilding):
					numTxt = "(!)Cvc"
					extraCorrectionX = get_extra_correction_x(numTxt)
					add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

				# <!-- custom: else do not display any numTxt, and just proceed in all cases -->
				isButtonFound = True
				iButtonIndex += 1

		# <!-- custom: show corporations that this building can found so "Required For" also includes corp entries (e.g. Cereal Mills HQ building). (GPT-5.3-Codex) -->
		iFoundedCorporation = gc.getBuildingInfo(self.iBuilding).getFoundsCorporation()
		if iFoundedCorporation >= 0:
			screen.appendMultiListButton(rowListName, gc.getCorporationInfo(iFoundedCorporation).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CORPORATION, iFoundedCorporation, 1, False)

			isButtonFound = True
			iButtonIndex += 1

		if not isButtonFound:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel, "TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_NOTHING")

	def placeObsoleteWith(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

		# Create panel with proper styling
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_OBSOLETE", ()), "", False, True, self.X_OBSOLETE_WITH, self.Y_OBSOLETE_WITH, self.W_OBSOLETE_WITH, self.H_OBSOLETE_WITH, PanelStyles.PANEL_STYLE_BLUE50)
		# <!-- custom: additionnal left side padding for the button(s) -->
		screen.attachLabel(panelName, "", "  ")

		# Get the building info
		buildingInfo = gc.getBuildingInfo(self.iBuilding)

		# Check if the building has an obsolete tech directly <!-- custom: (i assume is about the obsoletetech info in (adjust to your mod path) for example C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS\Assets\XML\Buildings\CIV4BuildingInfos.xml) -->
		iObsoleteTech = buildingInfo.getObsoleteTech()

		# If no direct obsolete tech, check if it's a special building type
		# <!-- custom: (e.g. the jewish monastery appears as never obsolete from the direct obsolete tech check due to <ObsoleteTech>NONE</ObsoleteTech>, but it does get obsolete at scientific method (now removed) though in <ObsoleteTech>TECH_SCIENTIFIC_METHOD</ObsoleteTech> at (adjust with your mod path if different) for example C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS\Assets\XML\Buildings\CIV4SpecialBuildingInfos.xml (now this file has been imported in AdvCiv-SAS as well in case we need to change it and to have all info we want and control it)) -->
		if iObsoleteTech == -1:
			iSpecialBuildingType = buildingInfo.getSpecialBuildingType()
			if iSpecialBuildingType != -1:
				# Get the special building info
				specialBuildingInfo = gc.getSpecialBuildingInfo(iSpecialBuildingType)
				# Check if the special building has an obsolete tech
				iObsoleteTech = specialBuildingInfo.getObsoleteTech()

		# <!-- custom: after having checked both directly for an obsolete tech, or indirectly through special buiding, now display button if any:
		if iObsoleteTech != -1:
			screen.attachImageButton(panelName, "", gc.getTechInfo(iObsoleteTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iObsoleteTech, -1, False)

		else:
			# <!-- custom: prettier display -->
			#screen.attachLabel(panelName, "", localText.getText("TXT_KEY_PEDIA_NEVER_OBSOLETE", ()))
			draw_none_text(screen, self.top, self.X_OBSOLETE_WITH, self.Y_OBSOLETE_WITH, self.W_OBSOLETE_WITH, self.H_OBSOLETE_WITH, "TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_NEVER")

	def placeFreePBBS(self):
		xPanel = self.X_FREE_PBBS
		yPanel = self.Y_FREE_PBBS
		wPanel = self.W_FREE_PBBS
		hPanel = self.H_FREE_PBBS

		txtKeyPanel = "TXT_KEY_PEDIA_FREE_PBBS"

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
		iButtonIndex = 0
		maxButtonsPerRow = get_multilist_max_buttons_per_row(multiListW, MULTILIST_BUTTON_SIZE)

		# Get the building info
		buildingInfo = gc.getBuildingInfo(self.iBuilding)

		# Check if the building grants a free promotion, and attach it -->
		iFreePromotion = buildingInfo.getFreePromotion()
		if iFreePromotion != -1:
			screen.appendMultiListButton(rowListName, gc.getPromotionInfo(iFreePromotion).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, iFreePromotion, 1, False)

			numTxt = "AUC"
			extraCorrectionX = get_extra_correction_x(numTxt)
			add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

			isButtonFound = True
			iButtonIndex += 1

		# Check if the building grants a free building
		iFreeBuildingClass = buildingInfo.getFreeBuildingClass()
		if iFreeBuildingClass != -1:
			iDefaultBuilding =  self.get_iDefaultBuilding_current_civ(iFreeBuildingClass)

			# If a valid building exists, display its button
			if iDefaultBuilding != -1:
				screen.appendMultiListButton(rowListName, gc.getBuildingInfo(iDefaultBuilding).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iDefaultBuilding, 1, False)

				numTxt = "ACs"
				extraCorrectionX = get_extra_correction_x(numTxt)
				add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

				isButtonFound = True
				iButtonIndex += 1

		# Check if the building grants a free bonus
		iFreeBonus = buildingInfo.getFreeBonus()

		if iFreeBonus != -1:
			iNumFreeBonuses = buildingInfo.getNumFreeBonuses()

			screen.appendMultiListButton(rowListName, gc.getBonusInfo(iFreeBonus).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iFreeBonus, 1, False)

			numTxt = get_numTxt_num_free_bonus_or_random_map(iNumFreeBonuses)
			extraCorrectionX = get_extra_correction_x(numTxt)
			add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

			isButtonFound = True
			iButtonIndex += 1

		# Check if the building grants free specialists - simpler approach
		for iSpecialist in range(gc.getNumSpecialistInfos()):
			if buildingInfo.getFreeSpecialistCount(iSpecialist) > 0:
				iSpecialistCount = buildingInfo.getFreeSpecialistCount(iSpecialist)

				screen.appendMultiListButton(rowListName, gc.getSpecialistInfo(iSpecialist).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_SPECIALIST, iSpecialist, 1, False)

				numTxt = "%d" % iSpecialistCount
				extraCorrectionX = get_extra_correction_x(numTxt)
				add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

				isButtonFound = True
				iButtonIndex += 1

		if not isButtonFound:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel)

	def isBuildingUnique(self, iBuildingIndex):
		# Helper function to determine if a building is civ-specific (unique)
		#
		buildingInfo = gc.getBuildingInfo(iBuildingIndex)
		buildingClassInfo = gc.getBuildingClassInfo(buildingInfo.getBuildingClassType())

		# A building is unique if it's not the default building for its class
		defaultBuildingForClass = buildingClassInfo.getDefaultBuildingIndex()
		return iBuildingIndex != defaultBuildingForClass

	def getBuildingCiv(self, iBuildingIndex):
		# Helper function to get which civ a unique building belongs to
		#
		buildingInfo = gc.getBuildingInfo(iBuildingIndex)
		buildingClassType = buildingInfo.getBuildingClassType()

		# Check all civs to see which one has this building as their unique version
		for iCiv in range(gc.getNumCivilizationInfos()):
			civInfo = gc.getCivilizationInfo(iCiv)
			civBuildingForClass = civInfo.getCivilizationBuildings(buildingClassType)
			if civBuildingForClass == iBuildingIndex:
				return iCiv
		return -1  # Should not happen for unique buildings

	def buildingClassHasUniqueVersions(self, buildingClassType):
		# Helper function to check if a building class has any unique versions
		#
		buildingClassInfo = gc.getBuildingClassInfo(buildingClassType)
		defaultBuilding = buildingClassInfo.getDefaultBuildingIndex()

		# Check if any civ has a different building for this class
		for iCiv in range(gc.getNumCivilizationInfos()):
			civInfo = gc.getCivilizationInfo(iCiv)
			civBuildingForClass = civInfo.getCivilizationBuildings(buildingClassType)
			if civBuildingForClass != defaultBuilding:
				return True
		return False

	def placeFreeWith(self):
		xPanel = self.X_FREE_WITH
		yPanel = self.Y_FREE_WITH
		wPanel = self.W_FREE_WITH
		hPanel = self.H_FREE_WITH

		txtKeyPanel = "TXT_KEY_PEDIA_FREE_WITH"

		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

		# Create panel with proper styling
		screen.addPanel(panelName, localText.getText(txtKeyPanel, ()), "", False, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50)
		# <!-- custom: additional left side padding for the button(s) -->
		screen.attachLabel(panelName, "", "  ")

		# Get the current building info
		currentBuildingInfo = gc.getBuildingInfo(self.iBuilding)
		currentBuildingClass = currentBuildingInfo.getBuildingClassType()

		# <!-- custom: handle no building found message -->
		isButtonFound = False

		# Check if current building is civ-specific (unique)
		currentIsUnique = self.isBuildingUnique(self.iBuilding)
		if currentIsUnique:
			currentCiv = self.getBuildingCiv(self.iBuilding)
		else:
			currentCiv = -1

		# Check if there are any unique versions of the current building
		hasUniqueVersions = self.buildingClassHasUniqueVersions(currentBuildingClass)

		# Check all buildings to see which ones provide this building for free
		for iBuildingLoop in range(gc.getNumBuildingInfos()):
			buildingInfo = gc.getBuildingInfo(iBuildingLoop)

			# Check if this building provides our current building for free
			freeBuildingClass = buildingInfo.getFreeBuildingClass()
			if freeBuildingClass == currentBuildingClass:
				providerIsUnique = self.isBuildingUnique(iBuildingLoop)
				if providerIsUnique:
					providerCiv = self.getBuildingCiv(iBuildingLoop)
				else:
					providerCiv = -1

				# Determine if we should show this provider based on the logic:
				shouldShow = False

				if currentIsUnique:
					# Current building is unique - only show providers that this civ can actually use
					if not providerIsUnique or providerCiv == currentCiv:
						shouldShow = True
				else:
					# Current building is generic
					if hasUniqueVersions:
						# Generic building has unique versions - only show generic providers
						if not providerIsUnique:
							shouldShow = True
					else:
						# No unique versions exist - show all providers
						shouldShow = True

				if shouldShow:
					isButtonFound = True
					screen.attachImageButton(panelName, "", gc.getBuildingInfo(iBuildingLoop).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuildingLoop, -1, False)

		if not isButtonFound:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel)

	def placeReplace(self):
		xPanel = self.X_REPLACE
		yPanel = self.Y_REPLACE
		wPanel = self.W_REPLACE
		hPanel = self.H_REPLACE

		iBuildingClass = gc.getBuildingInfo(self.iBuilding).getBuildingClassType()
		iBaseBuilding = gc.getBuildingClassInfo(iBuildingClass).getDefaultBuildingIndex()

		# Use different text key depending on whether this is a unique or base building
		if self.iBuilding != iBaseBuilding:
			# This is a unique building that replaces a base building
			txtKeyPanel = "TXT_KEY_PEDIA_REPLACE_REPLACES_CUSTOM"
		else:
			# This is a base building that can be replaced by unique buildings
			txtKeyPanel = "TXT_KEY_PEDIA_REPLACE_REPLACED_BY_CUSTOM"

		screen = self.top.getScreen()
		panel = self.top.getNextWidgetName()

		# Create panel with proper styling
		screen.addPanel(panel, localText.getText(txtKeyPanel, ()), "", False, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50)
		# <!-- custom: additionnal left side padding for the button(s) -->
		screen.attachLabel(panel, "", "  ")

		# <!-- custom: handle no building found message -->
		isButtonFound = False

		# If this is a unique (i.e.civ-specific) building, show the base building it replaces
		if self.iBuilding != iBaseBuilding:
			isButtonFound = True
			screen.attachImageButton(panel, "", gc.getBuildingInfo(iBaseBuilding).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBaseBuilding, 1, False)
			return

		else:
			# If this is the base building, show all unique (i.e.civ-specific) buildings that replace it
			for iBuilding in xrange(gc.getNumBuildingInfos()):
				if self.iBuilding != iBuilding and not gc.getBuildingInfo(iBuilding).isGraphicalOnly():
					if iBuildingClass == gc.getBuildingInfo(iBuilding).getBuildingClassType():
						isButtonFound = True
						screen.attachImageButton(panel, "", gc.getBuildingInfo(iBuilding).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, 1, False)

		if not isButtonFound:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel, "TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_NOTHING")

	def placeCivilizations(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

		# Create panel with proper styling
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_CIVILIZATIONS", ()), "", False, True, self.X_CIVILIZATIONS, self.Y_CIVILIZATIONS, self.W_CIVILIZATIONS, self.H_CIVILIZATIONS, PanelStyles.PANEL_STYLE_BLUE50)
		# <!-- custom: additionnal left side padding for the button(s) -->
		screen.attachLabel(panelName, "", "  ")

		# Get building class info
		iBuildingClass = gc.getBuildingInfo(self.iBuilding).getBuildingClassType()
		iDefaultBuilding = gc.getBuildingClassInfo(iBuildingClass).getDefaultBuildingIndex()

		# Check if this is a unique (i.e.civ-specific) building (not the default building for its class)
		bIsUnique = (self.iBuilding != iDefaultBuilding)

		# If this is a unique (i.e.civ-specific) building, show which civ can build it
		if bIsUnique:
			# Find which civ has this unique (i.e.civ-specific) building
			for iCiv in range(gc.getNumCivilizationInfos()):
				# <!-- custom: include barbarians and perhaps other non playable civs in the display for example for to display the barbarian palace, barbarian granary (so comment out the isPlayable check as Claude AI explained indeed and that i implemented in a simpler manner thanks to its explanation, but also unindent the below below too) -->
				#if gc.getCivilizationInfo(iCiv).isPlayable():
				iCivBuilding = gc.getCivilizationInfo(iCiv).getCivilizationBuildings(iBuildingClass)
				if iCivBuilding == self.iBuilding:
					screen.attachImageButton(panelName, "", gc.getCivilizationInfo(iCiv).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIV, iCiv, -1, False)
		# <!-- custom: A class-default building is not available to civilizations that map its class to a replacement or NONE.
		# Keep the one-token panel as `AllR`, with its exact All Remaining Civilizations scope explained in the Sevopedia Legend, instead of redundantly listing every ordinary civilization. See KI#321. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		else:
			# <!-- custom: prettier display -->
			#screen.attachLabel(panelName, "", localText.getText("TXT_KEY_PEDIA_AVAILABLE_ALL_CIVS", ()))
			yPanelCenter = self.Y_CIVILIZATIONS + (self.H_CIVILIZATIONS / 2)
			textName = self.top.getNextWidgetName()
			szText = localText.getText("TXT_KEY_PEDIA_SAS_ALL_REMAINING_CIVS", ())
			screen.addMultilineText(textName, SASTextScale.labelText(szText), self.X_CIVILIZATIONS + 7, yPanelCenter, self.W_CIVILIZATIONS - 14, self.H_CIVILIZATIONS - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	# <!-- custom: add iconquestprob with the help of claude ai, quite similarly than for the getChopProductionText addition in sevopedia feature) -->
	def getIConquestProbText(self):
		buildingInfo = gc.getBuildingInfo(self.iBuilding)
		# <!-- custom: return any value even 0 to display it as such -->
		conquestProb = buildingInfo.getConquestProbability()

		return (u"%siConquestProb: %d" % (localText.getText("[ICON_BULLET]", ()), conquestProb))

	def placeSpecial(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_SPECIAL_ABILITIES", ()), "", True, False, self.X_SPECIAL, self.Y_SPECIAL, self.W_SPECIAL, self.H_SPECIAL, PanelStyles.PANEL_STYLE_BLUE50 )
		listName = self.top.getNextWidgetName()
		szSpecialText = CyGameTextMgr().getBuildingHelp(self.iBuilding, True, False, False, None)[1:]

		# Add conquest probability information
		conquestProbText = self.getIConquestProbText()
		if conquestProbText:
			szSpecialText += "\n%s" % conquestProbText

		screen.addMultilineText(listName, SASTextScale.labelText(szSpecialText), self.X_SPECIAL+5, self.Y_SPECIAL+30, self.W_SPECIAL-10, self.H_SPECIAL-35, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeBuildingAnimation(self):
		screen = self.top.getScreen()
		iAnimX, iAnimY, iAnimW, iAnimH = draw_expandable_content_panel_container(screen, self.top, u"", self.X_BUILDING_ANIMATION, self.Y_BUILDING_ANIMATION, self.W_BUILDING_ANIMATION, self.H_BUILDING_ANIMATION, self.bContentExpanded, SAS_MAGIC_PEDIA_PYTHON_CONTENT_EXPAND, SAS_MAGIC_PEDIA_PYTHON_CONTENT_RELOAD)
		screen.addBuildingGraphicGFC(self.top.getNextWidgetName(), self.iBuilding, iAnimX, iAnimY, iAnimW, iAnimH, WidgetTypes.WIDGET_GENERAL, -1, -1, X_ROTATION_ANIMATION, Z_ROTATION_ANIMATION, self.SCALE_ANIMATION, True)

	def setHistoryExpanded(self, bExpanded):
		self.bHistoryExpanded = bExpanded

	def setContentExpanded(self, bExpanded):
		self.bContentExpanded = bExpanded

	def placeHistory(self):
		screen = self.top.getScreen()
		szText = gc.getBuildingInfo(self.iBuilding).getCivilopedia()
		szTitle = localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ())
		draw_expandable_text_panel(screen, self.top, szTitle, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, szText, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)
	def handleInput (self, inputClass):
		return 0

