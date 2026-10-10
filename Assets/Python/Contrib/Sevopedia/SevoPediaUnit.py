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

from CvPythonExtensions import *
from SASMagicNumbers import *
import CvUtil
import ScreenInput
import SevoScreenEnums
from SASFontUtils import *
import SASTextScale
from SASUtils import getInfoTypeOrFail

from _sevopedia_helpers import *

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

# <!-- custom: change its value if you don't want to see AI information in the special abilities panel -->
IS_SHOW_AI_INFO = (gc.getDefineINT("SAS_SEVOPEDIA_UNIT_SHOW_AI_INFORMATION") > 0)

class SevoPediaUnit:

	def __init__(self, main):
		self.iUnit = -1
		self.top = main
		self.bHistoryExpanded = False
		self.bContentExpanded = False
		self.I_TERRAIN_HILL = getInfoTypeOrFail("TERRAIN_HILL")

		self.X_UNIT_PANE = self.top.X_PEDIA_PAGE
		self.Y_UNIT_PANE = self.top.Y_PEDIA_PAGE
		# <!-- custom: no margins to merge edges with promo pane for nicer display maybe -->
		self.W_UNIT_PANE = (self.top.R_PEDIA_PAGE - self.X_UNIT_PANE - MEDIUM_MARGIN) / 4
		self.H_UNIT_PANE = 190

		self.W_ICON = PANE_ICON_SIZE
		self.H_ICON = PANE_ICON_SIZE
		# <!-- custom: if PANE_ICON_SIZE is small (e.g. 64), start at the center of self.X_UNIT_PANE, but if PANE_ICON_SIZE is big (e.g. 164) start at the left most part of self.X_UNIT_PANE; same reasoning for Y position -->
		self.X_ICON = self.X_UNIT_PANE + (PANE_ICON_FRAME_SIZE - PANE_ICON_SIZE) / 2
		self.Y_ICON = self.Y_UNIT_PANE + (self.H_UNIT_PANE - self.H_ICON) / 2

		# <!-- custom: add an extra margin to accommodate the potentially larger PANE_ICON_SIZE (than for example 64), if diff is 0 this is harmless to keep too so is dynamical code that can handle optionally larger PANE_ICON_SIZE (vs old PANE_ICON_SIZE of 64) that you may keep or remove as you prefer -->
		self.SMALLER_ICON_SIZE_THAN_ICON_FRAME_MARGIN = (PANE_ICON_FRAME_SIZE - PANE_ICON_SIZE) / 2

		self.STATS_PANE_LEFT_SIDE_MARGIN = 0
		self.STATS_PANE_UPPER_PADDING = 42

		self.PROMOTION_ICON_SIZE = 32

		# <!-- custom: no margins to merge edges with unit pane for nicer display maybe -->
		# <!-- custom: merge effect by partially joining their borders, i accidentally found or maybe got the idea and looks very nice, just is slightly not centered maybe fixable or not but and in all cases -->
		self.W_MERGE_PANELS_EFFECT = 5

		self.X_PROMO_PANE = self.X_UNIT_PANE + self.W_UNIT_PANE - self.W_MERGE_PANELS_EFFECT
		self.Y_PROMO_PANE = self.Y_UNIT_PANE
		self.W_PROMO_PANE = self.W_UNIT_PANE + self.W_MERGE_PANELS_EFFECT
		self.H_PROMO_PANE = self.H_UNIT_PANE

		self.X_STATS_PANE = self.X_UNIT_PANE + self.STATS_PANE_LEFT_SIDE_MARGIN + self.W_ICON + (2 *self.SMALLER_ICON_SIZE_THAN_ICON_FRAME_MARGIN)
		self.Y_STATS_PANE = self.Y_UNIT_PANE + self.STATS_PANE_UPPER_PADDING
		self.W_STATS_PANE = self.W_UNIT_PANE - self.W_ICON - (2 * self.SMALLER_ICON_SIZE_THAN_ICON_FRAME_MARGIN) - self.STATS_PANE_LEFT_SIDE_MARGIN
		self.H_STATS_PANE = self.H_UNIT_PANE - self.STATS_PANE_UPPER_PADDING

		# <!-- custom: see sevopediabuilding's self.W_TOTAL_EFFECTIVE_BUILDING_PANE for differences in implementation -->
		self.W_TOTAL_EFFECTIVE_UNIT_PANE = self.W_UNIT_PANE + self.W_PROMO_PANE - self.W_MERGE_PANELS_EFFECT

		# <!-- custom: reorganized layout (Claude code Opus 4.5):
		# Row 1 (left half): Requires (bigger) | Obsolete With (84px) | Free Promotions (small, 2 buttons)
		# Row 2 (left half): Replaces (bigger) | Civs (84px) | Upgrades To (same W as Free Promotions)
		# Row 2 (right half, under animation): Against Classes | Peak/Hill/City
		# Row 3-4: Of Other Units panel (full width, 2 rows height)
		# Row 5: Special | History -->

		self.H_REQUIRES = NON_MULTILIST_PANEL_STANDARD_HEIGHT

		# Narrow panels width (same as SevoPediaBuilding's obsolete panel)
		self.W_OBSOLETE_WITH = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.W_CIVILIZATIONS = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)

		# Small panels width - fits 2 buttons with standard panel spacing.
		self.W_SMALL_PANEL = get_panel_width_for_buttons(2, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)

		# Row 1 left half: Requires | Obsolete With | Free Promotions
		self.W_FREE_PROMOTIONS = self.W_SMALL_PANEL

		# Requires takes remaining space
		self.X_REQUIRES = self.X_UNIT_PANE
		self.Y_REQUIRES = self.Y_UNIT_PANE + self.H_UNIT_PANE + SMALL_MARGIN
		self.W_REQUIRES = self.W_TOTAL_EFFECTIVE_UNIT_PANE - MEDIUM_MARGIN - self.W_OBSOLETE_WITH - MEDIUM_MARGIN - self.W_FREE_PROMOTIONS
		self.H_REQUIRES = NON_MULTILIST_PANEL_STANDARD_HEIGHT

		self.X_OBSOLETE_WITH = self.X_REQUIRES + self.W_REQUIRES + MEDIUM_MARGIN
		self.Y_OBSOLETE_WITH = self.Y_REQUIRES
		self.H_OBSOLETE_WITH = self.H_REQUIRES

		self.X_FREE_PROMOTIONS = self.X_OBSOLETE_WITH + self.W_OBSOLETE_WITH + MEDIUM_MARGIN
		self.Y_FREE_PROMOTIONS = self.Y_REQUIRES
		self.H_FREE_PROMOTIONS = self.H_REQUIRES

		# Row 2 left half: Replaces | Civs | Upgrades To (aligned under Free Promotions)
		self.W_UPGRADES_TO = self.W_SMALL_PANEL
		self.X_UPGRADES_TO = self.X_FREE_PROMOTIONS  # Aligned under Free Promotions
		self.Y_UPGRADES_TO = self.Y_REQUIRES + self.H_REQUIRES + SMALL_MARGIN
		self.H_UPGRADES_TO = self.H_REQUIRES

		# Civs aligned under Obsolete With
		self.X_CIVILIZATIONS = self.X_OBSOLETE_WITH  # Aligned under Obsolete With
		self.Y_CIVILIZATIONS = self.Y_UPGRADES_TO
		self.H_CIVILIZATIONS = self.H_REQUIRES

		self.X_REPLACE = self.X_UNIT_PANE
		self.Y_REPLACE = self.Y_UPGRADES_TO
		self.W_REPLACE = self.X_CIVILIZATIONS - self.X_REPLACE - MEDIUM_MARGIN  # Takes remaining space (same as Requires)
		self.H_REPLACE = self.H_REQUIRES

		# <!-- custom: adjust this based on your multilist button size; note that this is used only in the _OF_OTHER_UNITS_MODIFIERS related panel, so not applying this everywhere as all other multilist panels as of now only use one row or if they have multiple rows they don't use a numTxt; tested only for a button size of 64 as rest of the multilist code, but it should maybe handle quite well another button size minus perhaps small numTxt adjustments or corrections that may be influenced by button size; currently not implemented since we only use a button size of 64px so as of now using the same variable in all our file rather than declaring and passing a different button size locally in the multilist panels that have many rows, for simplicity. -->
		# self.H_MULTILIST_MULTIPLE_ROWS_BUTTON_SIZE = 64
		# Row 3-4: Of Other Units panel (full width, 2 rows height) - under Row 2
		self.X_OF_OTHER_UNITS_MODIFIERS = self.X_UNIT_PANE
		self.Y_OF_OTHER_UNITS_MODIFIERS = self.Y_REPLACE + self.H_REPLACE + SMALL_MARGIN
		self.W_OF_OTHER_UNITS_MODIFIERS = self.top.R_PEDIA_PAGE - self.X_UNIT_PANE
		# self.H_OF_OTHER_UNITS_MODIFIERS = self.H_REQUIRES + self.H_MULTILIST_MULTIPLE_ROWS_BUTTON_SIZE
		self.H_OF_OTHER_UNITS_MODIFIERS = self.H_REQUIRES + MULTILIST_BUTTON_SIZE

		# Row 5: Special panel (left half)
		self.X_SPECIAL = self.X_UNIT_PANE
		self.Y_SPECIAL = self.Y_OF_OTHER_UNITS_MODIFIERS + self.H_OF_OTHER_UNITS_MODIFIERS + SMALL_MARGIN
		self.W_SPECIAL = self.W_TOTAL_EFFECTIVE_UNIT_PANE
		self.H_SPECIAL = self.top.B_PEDIA_PAGE - self.Y_SPECIAL

		# Right column: Unit Animation
		self.H_ADJUST_HEIGHT_ANIMATION_TO_MATCH_ADJACENT_PANE = 7

		self.X_UNIT_ANIMATION = self.X_UNIT_PANE + self.W_TOTAL_EFFECTIVE_UNIT_PANE + MEDIUM_MARGIN
		self.Y_UNIT_ANIMATION = self.Y_UNIT_PANE + self.H_ADJUST_HEIGHT_ANIMATION_TO_MATCH_ADJACENT_PANE
		self.W_UNIT_ANIMATION = self.W_TOTAL_EFFECTIVE_UNIT_PANE
		# <!-- custom: make it one panel height smaller to accommodate the higher placeSpecial panel that we use to display AI information and or such if any other info displayed or to better display the info in cases where the placeSpecial was a bit too short in height -->
		# self.H_UNIT_ANIMATION = self.H_UNIT_PANE + SMALL_MARGIN + self.H_REQUIRES + SMALL_MARGIN + self.H_UPGRADES_TO - self.H_ADJUST_HEIGHT_ANIMATION_TO_MATCH_ADJACENT_PANE
		self.H_UNIT_ANIMATION = self.H_UNIT_PANE + SMALL_MARGIN + self.H_REQUIRES - self.H_ADJUST_HEIGHT_ANIMATION_TO_MATCH_ADJACENT_PANE

		self.SCALE_ANIMATION = 1.0

		# Row 2 right half (under animation): Against Classes (left) | Peak/Hill/City (right)
		# <!-- custom: swapped positions - Against Classes on left, Peak/Hill/City on right -->
		self.H_UNDER_ANIMATION_PANELS = self.H_REQUIRES

		self.X_OF_UNIT_MODIFIERS_AGAINST_OTHERS = self.X_UNIT_ANIMATION
		self.Y_OF_UNIT_MODIFIERS_AGAINST_OTHERS = self.Y_UNIT_ANIMATION + self.H_UNIT_ANIMATION + SMALL_MARGIN
		self.W_OF_UNIT_MODIFIERS_AGAINST_OTHERS = (self.W_UNIT_ANIMATION - MEDIUM_MARGIN) / 2
		self.H_OF_UNIT_MODIFIERS_AGAINST_OTHERS = self.H_UNDER_ANIMATION_PANELS

		self.X_PEAK_HILL_CITY_TERRAINS_FEATURES_MODIFIERS = self.X_OF_UNIT_MODIFIERS_AGAINST_OTHERS + self.W_OF_UNIT_MODIFIERS_AGAINST_OTHERS + MEDIUM_MARGIN
		self.Y_PEAK_HILL_CITY_TERRAINS_FEATURES_MODIFIERS = self.Y_OF_UNIT_MODIFIERS_AGAINST_OTHERS
		self.W_PEAK_HILL_CITY_TERRAINS_FEATURES_MODIFIERS = self.W_OF_UNIT_MODIFIERS_AGAINST_OTHERS
		self.H_PEAK_HILL_CITY_TERRAINS_FEATURES_MODIFIERS = self.H_UNDER_ANIMATION_PANELS
		self.citiesButtonPath = ArtFileMgr.getInterfaceArtInfo("SAS_CITIES_LANDSCAPE").getPath()

		# Row 5 right half: History panel - aligned with Special panel
		self.X_HISTORY = self.X_UNIT_ANIMATION
		self.Y_HISTORY = self.Y_SPECIAL  # Same Y as Special panel for alignment
		self.W_HISTORY = self.W_UNIT_ANIMATION
		self.H_HISTORY = self.top.B_PEDIA_PAGE - self.Y_HISTORY

		# <!-- custom: cached vanilla defines used in stats and prereqs drawing methods. (Claude code Sonnet 4.6) -->
		self.iUNIT_PRODUCTION_PERCENT = None
		self.iNUM_UNIT_AND_TECH_PREREQS = None

	def interfaceScreen(self, iUnit):
		if self.iUnit != iUnit:
			self.bHistoryExpanded = False
			self.bContentExpanded = False
		self.iUnit = iUnit

		if self.bContentExpanded:
			self.placeUnitAnimation()
			return

		# Row 0: Unit Pane + Promotions (left) | Animation (right)
		self.placeUnitPane()
		self.placeStats()
		self.placePromotions()
		if not self.bHistoryExpanded:
			self.placeUnitAnimation()

		# Row 1 left: Requires | Obsolete With | Free Promotions
		self.placeRequires()
		self.placeObsoleteWith()
		self.placeFreePromotions()

		# Row 2 left: Replaces | Civs | Upgrades To
		# Row 2 right (under animation): Against Classes | Peak/Hill/City
		self.placeReplace()
		self.placeCivilizations()
		self.placeUpgradesTo()
		self.placeModifiersOfThisUnitAgainstOtherUnitClassesCombatTypes()
		self.placePeakHillCityTerrainsFeaturesModifiers()

		# Row 3-4: Of Other Units panel (full width)
		self.placeModifiersOfOtherUnitClassesCombatTypesAgainstThisUnit()

		# Row 5: Special (left) | History (right)
		self.placeSpecial()
		self.placeHistory()
		place_new_concept_legend_link(self.top, "CONCEPT_SAS_SEVOPEDIA_NUMTXT_LEGEND")

	def _getHistoryText(self):
		szText = u""
		unitInfo = gc.getUnitInfo(self.iUnit)
		# <!-- custom: prepend unique names for units that define them, then keep original background text below. (GPT-5.3-Codex) -->
		aUniqueNames = []
		for iName in xrange(unitInfo.getNumUnitNames()):
			szName = localText.getText(unitInfo.getUnitNames(iName), ())
			if szName and szName not in aUniqueNames:
				aUniqueNames.append(szName)
		if len(aUniqueNames) > 0:
			szText += sasFontTagTitle + u"Unique Names" + SAS_FONT_TAG_CLOSE + u"\n\n"
			for i in xrange(len(aUniqueNames)):
				if i == 0:
					szLine = localText.getText("[ICON_STAR]", ()) + aUniqueNames[i]
				else:
					szLine = localText.getText("[ICON_BULLET]", ()) + aUniqueNames[i]
				if i > 0:
					szText += u"\n"
				szText += sasFontTagLabel + szLine + SAS_FONT_TAG_CLOSE
			szText += u"\n\n"
		# <!-- custom: same reasoning as for TXT_KEY_CIVILOPEDIA_STRATEGY in SevoPediaBuilding.py, removing (hiding) the entry entirely from sevopedia. -->
		# <!-- custom: same reasoning as for/in SevopediaBuilding.py, i also don't need the redundant "History:" -->
		# <!-- custom: normalize/upscale concept text in Sevopedia Unit history panel ("Background") to match concept page readability. (GPT-5.3-Codex) -->
		szText += SASTextScale.normalizeLabelText(unitInfo.getCivilopedia())
		return szText

	def setHistoryExpanded(self, bExpanded):
		self.bHistoryExpanded = bExpanded

	def setContentExpanded(self, bExpanded):
		self.bContentExpanded = bExpanded

	def placeUnitPane(self):
		screen = self.top.getScreen()

		# <!-- custom: note: using self.W_UNIT_PANE at panel creationunlike in sevopediabuilding part equivalent to this code where we use self.W_TOTAL_EFFECTIVE_BUILDING_PANE due to differences in their implementations, see sevopediabuilding for comparison or details -->
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_UNIT_PANE, self.Y_UNIT_PANE, self.W_UNIT_PANE, self.H_UNIT_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_MAIN)
		szButton = gc.getUnitInfo(self.iUnit).getButton()
		# <advc.003l>
		iActivePlayer = gc.getGame().getActivePlayer()
		if iActivePlayer >= 0:
			szButton = gc.getPlayer(iActivePlayer).getUnitButton(self.iUnit)
		# </advc.003l>
		screen.addDDSGFC(self.top.getNextWidgetName(), szButton, self.X_ICON + self.W_ICON/2 - PANE_ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - PANE_ICON_SIZE/2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def placeStats(self):
		screen = self.top.getScreen()
		# <!-- custom: self.top.getNextWidgetName() is regenerated many times and `szName = self.top.getNextWidgetName()` is never used, removing unused variables and (so i am) trying to avoid redundance as long as code seems to work fine or as intended and also to avoid or fix the ruff warning, i asked chatgpt to be sure and it seems this is fine perhaps even good to do so but anyways code seems to function fine and no warning so hopefully fixed in this case i mean at least; check if accurate and to be sure; -->
		panelName = self.top.getNextWidgetName()
		iCombatType = gc.getUnitInfo(self.iUnit).getUnitCombatType()

		if iCombatType != -1:
			screen.setImageButton(self.top.getNextWidgetName(), gc.getUnitCombatInfo(iCombatType).getButton(), self.X_STATS_PANE, self.Y_STATS_PANE - 35, 32, 32, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT_COMBAT, iCombatType, 0)
			screen.setText(self.top.getNextWidgetName(), "", SASTextScale.titleText(gc.getUnitCombatInfo(iCombatType).getDescription()), CvUtil.FONT_LEFT_JUSTIFY, self.X_STATS_PANE + 37, self.Y_STATS_PANE - 30, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT_COMBAT, iCombatType, 0)

		# <!-- custom: keep ListBox for stats here; trying multiline/setText rendering gave the same apparent size for these lines in-game, so this remains for simplicity and stable layout. (GPT-5.3-Codex) -->
		screen.addListBoxGFC(panelName, "", self.X_STATS_PANE, self.Y_STATS_PANE, self.W_STATS_PANE, self.H_STATS_PANE, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(panelName, False)

		if (gc.getUnitInfo(self.iUnit).getAirCombat() > 0 and gc.getUnitInfo(self.iUnit).getCombat() == 0):
			iStrength = gc.getUnitInfo(self.iUnit).getAirCombat()
		else:
			iStrength = gc.getUnitInfo(self.iUnit).getCombat()
		if iStrength > 0: # advc.004y: Don't show 0 strength for nukes
			szStrength = localText.getText("TXT_KEY_PEDIA_STRENGTH_CUSTOM", (iStrength,))
			szStrengthText = u"%c  " % CyGame().getSymbolID(FontSymbols.STRENGTH_CHAR) + szStrength
			screen.appendListBoxStringNoUpdate(panelName, SASTextScale.titleText(szStrengthText), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

		eDomain = gc.getUnitInfo(self.iUnit).getDomainType()
		# <!-- custom: don't show movement for domain immobile units (missiles and such for example, not air fighters and such) -->
		if eDomain != DomainTypes.DOMAIN_IMMOBILE:
			# <!-- custom: show all stats that we want/have as they are, including 1 move air or 4 move helicopter -->
			## Don't show 1 move for air units
			#elif eDomain != DomainTypes.DOMAIN_AIR: # </advc.004y>
			szMovement = localText.getText("TXT_KEY_PEDIA_MOVEMENT_CUSTOM", (gc.getUnitInfo(self.iUnit).getMoves(),))
			szMovementText = u"%c  " % CyGame().getSymbolID(FontSymbols.MOVES_CHAR) + szMovement
			screen.appendListBoxStringNoUpdate(panelName, SASTextScale.titleText(szMovementText), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

		# advc.004y: Moved range above production cost. Condition for ICBM added.
		if gc.getUnitInfo(self.iUnit).getAirRange() > 0 or gc.getUnitInfo(self.iUnit).getNukeRange() >= 0:
			iRange = gc.getUnitInfo(self.iUnit).getAirRange()
			if iRange > 0: # advc.004y
				szRange = localText.getText("TXT_KEY_PEDIA_RANGE_CUSTOM", (iRange,))
			# <advc.004y> Unlimited range
			else:
				szRange = localText.getText("TXT_KEY_PEDIA_RANGE_UNLIMITED_CUSTOM", ())
			# </advc.004y>
			szRangeText = u"R    " + szRange
			screen.appendListBoxStringNoUpdate(panelName, SASTextScale.titleText(szRangeText), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)
		if (gc.getUnitInfo(self.iUnit).getProductionCost() >= 0 and not gc.getUnitInfo(self.iUnit).isFound()):
			if self.iUNIT_PRODUCTION_PERCENT is None:
				self.iUNIT_PRODUCTION_PERCENT = gc.getDefineINT("UNIT_PRODUCTION_PERCENT")
			unitCost = (gc.getUnitInfo(self.iUnit).getProductionCost() * self.iUNIT_PRODUCTION_PERCENT)/100
			if self.top.iActivePlayer != -1:
				unitCost = gc.getActivePlayer().getUnitProductionNeeded(self.iUnit)

			szCost = localText.getText("TXT_KEY_PEDIA_COST_CUSTOM", (unitCost,))
			szCostText = u"%c  " % gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar() + szCost
			screen.appendListBoxStringNoUpdate(panelName, SASTextScale.titleText(szCostText), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)
		screen.updateListBox(panelName)

	def placeRequires(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_REQUIRES", ()), "", False, True, self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")
		isButtonFound = False

		iPrereq = gc.getUnitInfo(self.iUnit).getPrereqAndTech()
		if iPrereq >= 0:
			screen.attachImageButton(panelName, "", gc.getTechInfo(iPrereq).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iPrereq, 1, False)
			isButtonFound = True
		if self.iNUM_UNIT_AND_TECH_PREREQS is None:
			self.iNUM_UNIT_AND_TECH_PREREQS = gc.getDefineINT("NUM_UNIT_AND_TECH_PREREQS")
		for j in range(self.iNUM_UNIT_AND_TECH_PREREQS):
			iPrereq = gc.getUnitInfo(self.iUnit).getPrereqAndTechs(j)
			if iPrereq >= 0:
				screen.attachImageButton(panelName, "", gc.getTechInfo(iPrereq).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iPrereq, -1, False)
				isButtonFound = True
		bFirst = True

		iPrereq = gc.getUnitInfo(self.iUnit).getPrereqAndBonus()
		if iPrereq >= 0:
			bFirst = False
			screen.attachImageButton(panelName, "", gc.getBonusInfo(iPrereq).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iPrereq, -1, False)
			isButtonFound = True
		nOr = 0
		for j in range(gc.getNUM_UNIT_PREREQ_OR_BONUSES()):
			if gc.getUnitInfo(self.iUnit).getPrereqOrBonuses(j) > -1:
				nOr += 1
		szLeftDelimeter = ""
		szRightDelimeter = ""
		if not bFirst:
			if nOr > 1:
				szLeftDelimeter = localText.getText("TXT_KEY_AND", ()) + "("
				szRightDelimeter = ") "
			elif nOr > 0:
				szLeftDelimeter = localText.getText("TXT_KEY_AND", ())
		if len(szLeftDelimeter) > 0:
			screen.attachLabel(panelName, "", SASTextScale.labelText(szLeftDelimeter))
		bFirst = True

		for j in range(gc.getNUM_UNIT_PREREQ_OR_BONUSES()):
			eBonus = gc.getUnitInfo(self.iUnit).getPrereqOrBonuses(j)
			if eBonus > -1:
				if not bFirst:
					screen.attachLabel(panelName, "", SASTextScale.labelText(localText.getText("TXT_KEY_OR", ())))
				else:
					bFirst = False
				screen.attachImageButton(panelName, "", gc.getBonusInfo(eBonus).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, eBonus, -1, False)
				isButtonFound = True
		if len(szRightDelimeter) > 0:
			screen.attachLabel(panelName, "", SASTextScale.labelText(szRightDelimeter))

		iPrereq = gc.getUnitInfo(self.iUnit).getPrereqReligion()
		if iPrereq >= 0:
			# <!-- custom: fix base advciv bug, replace WidgetTypes.WIDGET_HELP_RELIGION with WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION as is done already by base advciv and successfully in sevopedia building -->
			screen.attachImageButton(panelName, "", gc.getReligionInfo(iPrereq).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION, iPrereq, -1, False)
			isButtonFound = True

		# <!-- custom: Base AdvCiv omits the unit corporation prerequisite from this panel even though CvPlot::canTrain enforces it and all seven Executive units use it.
		# Show the required corporation with its normal Sevopedia jump so the page matches actual city training eligibility. See KI#236. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		iPrereq = gc.getUnitInfo(self.iUnit).getPrereqCorporation()
		if iPrereq >= 0:
			screen.attachImageButton(panelName, "", gc.getCorporationInfo(iPrereq).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CORPORATION, iPrereq, -1, False)
			isButtonFound = True

		iPrereq = gc.getUnitInfo(self.iUnit).getPrereqBuilding()
		if iPrereq >= 0:
			screen.attachImageButton(panelName, "", gc.getBuildingInfo(iPrereq).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iPrereq, -1, False)
			isButtonFound = True

		# Project requirements - New code for Manhattan Project and other projects
		unitInfo = gc.getUnitInfo(self.iUnit)
		iSpecialUnitType = unitInfo.getSpecialUnitType()
		# Find projects that require this special unit type
		projectsRequired = []
		# Check all projects to see if any enables this special unit
		if iSpecialUnitType >= 0:
			for iProject in range(gc.getNumProjectInfos()):
				projectInfo = gc.getProjectInfo(iProject)
				if projectInfo.getEveryoneSpecialUnit() == iSpecialUnitType:
					projectsRequired.append(iProject)
		# Special hardcoded case for nukes and Manhattan Project since they might not be linked via XML
		if unitInfo.getNukeRange() > 0:  # If this is a nuclear unit (like ICBM)
			for iProject in range(gc.getNumProjectInfos()):
				projectInfo = gc.getProjectInfo(iProject)
				if "MANHATTAN_PROJECT" in projectInfo.getType():
					if iProject not in projectsRequired:
						projectsRequired.append(iProject)
					break
		# Display all project requirements with "OR" text between them
		bFirst = True
		for iProject in projectsRequired:
			if not bFirst:
				# Add "OR" text between projects
				screen.attachLabel(panelName, "", SASTextScale.labelText(localText.getText("TXT_KEY_OR", ())))

			screen.attachImageButton(panelName, "", gc.getProjectInfo(iProject).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROJECT, iProject, -1, False)
			bFirst = False
			isButtonFound = True

		if not isButtonFound:
			draw_none_text(screen, self.top, self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES)

	def placeUpgradesTo(self):
		xPanel = self.X_UPGRADES_TO
		yPanel = self.Y_UPGRADES_TO
		wPanel = self.W_UPGRADES_TO
		hPanel = self.H_UPGRADES_TO

		txtKeyPanel = "TXT_KEY_PEDIA_UPGRADES_TO"

		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

		# Create panel with proper styling
		screen.addPanel(panelName, localText.getText(txtKeyPanel, ()), "", False, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50)
		# <!-- custom: additionnal left side padding for the button(s) -->
		screen.attachLabel(panelName, "", "  ")

		# <!-- custom: handle no unit found message -->
		isButtonFound = False

		iActivePlayer = gc.getGame().getActivePlayer() # advc.003l
		for k in range(gc.getNumUnitClassInfos()):
			if self.top.iActivePlayer == -1:
				eLoopUnit = gc.getUnitClassInfo(k).getDefaultUnitIndex()
			else:
				eLoopUnit = gc.getCivilizationInfo(gc.getGame().getActiveCivilizationType()).getCivilizationUnits(k)
			if (eLoopUnit >= 0 and gc.getUnitInfo(self.iUnit).getUpgradeUnitClass(k)):
				szButton = gc.getUnitInfo(eLoopUnit).getButton()
				# <advc.003l>
				if iActivePlayer >= 0:
					szButton = gc.getPlayer(iActivePlayer).getUnitButton(eLoopUnit)
				# </advc.003l>
				isButtonFound = True
				screen.attachImageButton(panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, eLoopUnit, 1, False)

		if not isButtonFound:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel, "TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_NOTHING")

	def placeFreePromotions(self):
		xPanel = self.X_FREE_PROMOTIONS
		yPanel = self.Y_FREE_PROMOTIONS
		wPanel = self.W_FREE_PROMOTIONS
		hPanel = self.H_FREE_PROMOTIONS

		txtKeyPanel = "TXT_KEY_PEDIA_FREE_PROMOTIONS"

		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

		# Create panel with proper styling
		screen.addPanel(panelName, localText.getText(txtKeyPanel, ()), "", False, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50)
		# Additional left side padding for the button(s)
		screen.attachLabel(panelName, "", "  ")

		# Get the unit info
		unitInfo = gc.getUnitInfo(self.iUnit)
		# Track if we found any free items to display
		isButtonFound = False

		# Check if the unit has any free promotions
		for iPromotion in range(gc.getNumPromotionInfos()):
			if unitInfo.getFreePromotions(iPromotion):
				isButtonFound = True
				# Attach promotion button
				screen.attachImageButton(panelName, "", gc.getPromotionInfo(iPromotion).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, iPromotion, -1, False)

		if not isButtonFound:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel)

	def placeModifiersOfThisUnitAgainstOtherUnitClassesCombatTypes(self):
		xPanel = self.X_OF_UNIT_MODIFIERS_AGAINST_OTHERS
		yPanel = self.Y_OF_UNIT_MODIFIERS_AGAINST_OTHERS
		wPanel = self.W_OF_UNIT_MODIFIERS_AGAINST_OTHERS
		hPanel = self.H_OF_UNIT_MODIFIERS_AGAINST_OTHERS

		txtKeyPanel = "TXT_KEY_PEDIA_OF_THIS_UNIT_MODIFIERS_AGAINST_OTHERS"

		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

		# Create panel with proper styling
		screen.addPanel(panelName, localText.getText(txtKeyPanel, ()), "", False, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50)

		rowListName = self.top.getNextWidgetName()

		# <!-- custom: addMultiListControlGFC code from our existing implementation we successfully did for the sevopedia religion leaders panel -->
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

		# Get the unit info
		unitInfo = gc.getUnitInfo(self.iUnit)
		iUnitCombatType = unitInfo.getUnitCombatType()

		for i in xrange(gc.getNumUnitClassInfos()):
			# Check class-based modifiers (attack and defense)
			iModAttack = unitInfo.getUnitClassAttackModifier(i)
			iModDefense = unitInfo.getUnitClassDefenseModifier(i)

			if iModAttack != 0 or iModDefense != 0:
				# <!-- custom: as highlighted by chatgpt and provided now after asked by me that/who noticed too, the unit widget expects unit indexes, not unit class indexes, so fetching a corresponding index to this unit class index; then chatgpt also helped solve the unitClass button not displaying due to being generic now solved and with my help too and idea(s) -->
				# Find a representative unit from this class
				unitClassInfo = gc.getUnitClassInfo(i)
				iRepresentativeUnit = unitClassInfo.getDefaultUnitIndex()
				screen.appendMultiListButton(rowListName, gc.getUnitInfo(iRepresentativeUnit).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iRepresentativeUnit, 1, False)

				# Handle class modifiers with x/y format
				numTxt = get_numTxt_attack_defense_modifiers(iModAttack, iModDefense)
				extraCorrectionX = get_extra_correction_x(numTxt)
				add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

				isButtonFound = True
				iButtonIndex += 1

		# Check for UnitCombatMods
		# <!-- custom: we need to do this in a separate loop according to chatgpt as "i is a UnitClass index; gc.getUnitCombatInfo(i) expects a UnitCombatTypes index" and indeed i don't know if this is the cause but we got an error when trying to refactor too aggressively with claude ai i meanthe code (ignorantly perhaps of me but good to try or not or yes), so making sure i mean to have a separate loop for combat type modifiers in understanding this -->
		# Check for unit combat types that this unit belongs to
		if iUnitCombatType != -1:  # Make sure this unit has a combat type
			# Loop through all units to find those with UnitCombatMods against this combat type
			for i in xrange(gc.getNumUnitCombatInfos()):
				iModCombat = unitInfo.getUnitCombatModifier(i)

				if iModCombat != 0:
					# <!-- custom: switch to combat type categories instead using relevant widget instead as provided by claude ai after i prompted it and reflecting on it in this case, our previous code was seemingly mistaken -->
					screen.appendMultiListButton(rowListName, gc.getUnitCombatInfo(i).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT_COMBAT, i, 1, False)

					numTxt = get_numTxt_combat_type_modifiers(iModCombat)
					extraCorrectionX = get_extra_correction_x(numTxt)
					add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

					isButtonFound = True
					iButtonIndex += 1

		if not isButtonFound:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel)

	def placeModifiersOfOtherUnitClassesCombatTypesAgainstThisUnit(self):
		xPanel = self.X_OF_OTHER_UNITS_MODIFIERS
		yPanel = self.Y_OF_OTHER_UNITS_MODIFIERS
		wPanel = self.W_OF_OTHER_UNITS_MODIFIERS
		hPanel = self.H_OF_OTHER_UNITS_MODIFIERS

		txtKeyPanel = "TXT_KEY_PEDIA_OF_OTHER_UNITS_MODIFIERS_AGAINST_THIS_UNIT"

		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

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

		# Get the unit info for the current unit
		unitInfo = gc.getUnitInfo(self.iUnit)
		iUnitClass = unitInfo.getUnitClassType()
		iUnitCombatType = unitInfo.getUnitCombatType()

		# Loop through all unit types to find those with UnitClassAttackMods or UnitClassDefenseMods against our unit class
		for i in range(gc.getNumUnitInfos()):
			otherUnitInfo = gc.getUnitInfo(i)

			# Check class-based modifiers (attack and defense)
			iModAttack = otherUnitInfo.getUnitClassAttackModifier(iUnitClass)
			iModDefense = otherUnitInfo.getUnitClassDefenseModifier(iUnitClass)

			if iModAttack != 0 or iModDefense != 0:
				screen.appendMultiListButton(rowListName, otherUnitInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, i, 1, False)

				# Handle class modifiers with x/y format
				numTxt = get_numTxt_attack_defense_modifiers(iModAttack, iModDefense)
				extraCorrectionX = get_extra_correction_x(numTxt)
				add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

				isButtonFound = True
				iButtonIndex += 1

		# Check for unit combat types that this unit belongs to
		if iUnitCombatType != -1:  # Make sure this unit has a combat type
			# Loop through all units to find those with UnitCombatMods against this combat type
			for i in range(gc.getNumUnitInfos()):
				otherUnitInfo = gc.getUnitInfo(i)
				iModCombat = otherUnitInfo.getUnitCombatModifier(iUnitCombatType)

				if iModCombat != 0:
					screen.appendMultiListButton(rowListName, otherUnitInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, i, 1, False)

					numTxt = get_numTxt_combat_type_modifiers(iModCombat)
					extraCorrectionX = get_extra_correction_x(numTxt)
					add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

					isButtonFound = True
					iButtonIndex += 1

		if not isButtonFound:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel)

	def placePeakHillCityTerrainsFeaturesModifiers(self):
		xPanel = self.X_PEAK_HILL_CITY_TERRAINS_FEATURES_MODIFIERS
		yPanel = self.Y_PEAK_HILL_CITY_TERRAINS_FEATURES_MODIFIERS
		wPanel = self.W_PEAK_HILL_CITY_TERRAINS_FEATURES_MODIFIERS
		hPanel = self.H_PEAK_HILL_CITY_TERRAINS_FEATURES_MODIFIERS

		txtKeyPanel = "TXT_KEY_PEDIA_PEAK_HILL_CITY_TERRAINS_FEATURES_MODIFIERS"

		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

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

		# Get the unit info
		unitInfo = gc.getUnitInfo(self.iUnit)

		# Hills bonuses
		iHillsAttack = unitInfo.getHillsAttackModifier()
		iHillsDefense = unitInfo.getHillsDefenseModifier()

		# <!-- custom: include negative values as well and after having asked chatgpt to be sure or quite more sure -->
		if iHillsAttack != 0 or iHillsDefense != 0:
			widgetType = WidgetTypes.WIDGET_PEDIA_JUMP_TO_TERRAIN
			iHill = self.I_TERRAIN_HILL
			widgetID2 = -1

			screen.appendMultiListButton(rowListName, gc.getTerrainInfo(iHill).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, widgetType, iHill, widgetID2, False)

			numTxt = get_numTxt_attack_defense_modifiers(iHillsAttack, iHillsDefense)
			extraCorrectionX = get_extra_correction_x(numTxt)
			add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

			isButtonFound = True
			iButtonIndex += 1

		# Terrain Attack/Defense bonuses
		# <!-- custom: note peak is handled here as well (i.e. same as for the other terrains only here), except for hills that follow a different modifier formula with getHillsAttackModifier and getHillsDefenseModifier -->
		for i in range(gc.getNumTerrainInfos()):	
			iTerrainAttack = unitInfo.getTerrainAttackModifier(i)
			iTerrainDefense = unitInfo.getTerrainDefenseModifier(i)

			if iTerrainAttack != 0 or iTerrainDefense != 0:
				widgetType = WidgetTypes.WIDGET_PEDIA_JUMP_TO_TERRAIN
				widgetID2 = -1

				screen.appendMultiListButton(rowListName, gc.getTerrainInfo(i).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, widgetType, i, widgetID2, False)

				numTxt = get_numTxt_attack_defense_modifiers(iTerrainAttack, iTerrainDefense)
				extraCorrectionX = get_extra_correction_x(numTxt)
				add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

				isButtonFound = True
				iButtonIndex += 1

		# City bonuses
		iCityAttack = unitInfo.getCityAttackModifier()
		iCityDefense = unitInfo.getCityDefenseModifier()

		if iCityAttack != 0 or iCityDefense != 0:
			citiesConceptID = get_concept_id("CONCEPT_CITIES")
			widgetType, widgetID1, widgetID2 = get_concept_widgetType_widgetID1_widgetID2(citiesConceptID, WidgetTypes, CivilopediaPageTypes)

			screen.appendMultiListButton(rowListName, self.citiesButtonPath, SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, widgetType, widgetID1, widgetID2, False)

			numTxt = get_numTxt_attack_defense_modifiers(iCityAttack, iCityDefense)
			extraCorrectionX = get_extra_correction_x(numTxt)
			add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

			isButtonFound = True
			iButtonIndex += 1

		# Feature Attack/Defense bonuses
		for i in range(gc.getNumFeatureInfos()):	
			iFeatureAttack = unitInfo.getFeatureAttackModifier(i)
			iFeatureDefense = unitInfo.getFeatureDefenseModifier(i)

			if iFeatureAttack != 0 or iFeatureDefense != 0:
				widgetType = WidgetTypes.WIDGET_PEDIA_JUMP_TO_FEATURE
				widgetID2 = -1

				screen.appendMultiListButton(rowListName, gc.getFeatureInfo(i).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, widgetType, i, widgetID2, False)

				numTxt = get_numTxt_attack_defense_modifiers(iFeatureAttack, iFeatureDefense)
				extraCorrectionX = get_extra_correction_x(numTxt)
				add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

				isButtonFound = True
				iButtonIndex += 1

		if not isButtonFound:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel)

	def placeSpecial(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

		if IS_SHOW_AI_INFO:
			txtKeyPanel = "TXT_KEY_PEDIA_SPECIAL_ABILITIES_WITH_SOME_AI_INFORMATION"
		else:
			txtKeyPanel = "TXT_KEY_PEDIA_SPECIAL_ABILITIES"

		screen.addPanel(panelName, localText.getText(txtKeyPanel, ()), "", True, False, self.X_SPECIAL, self.Y_SPECIAL, self.W_SPECIAL, self.H_SPECIAL, PanelStyles.PANEL_STYLE_BLUE50)

		listName = self.top.getNextWidgetName()
		szSpecialText = CyGameTextMgr().getUnitHelp(self.iUnit, True, False, False, None)[1:]

		bullet = localText.getText("[ICON_BULLET]", ())

		# <!-- custom: show if unit has no military support cost info for example for the robotic_infantry we added in advciv-sas, code provided with the help of chatgpt thanks etc thanks and thanks to me too hehe if i may say in this case, and adjusted for advciv-sas or not by me too -->
		# Get unit info
		unitInfo = gc.getUnitInfo(self.iUnit)

		if not unitInfo.isMilitarySupport():
			noMilitarySupportCostText = localText.getText("TXT_KEY_UNIT_NO_MILITARY_SUPPORT_COST", ())
			szSpecialText += u"\n%s%s" % (bullet, noMilitarySupportCostText)

		# <!-- custom: if unit grants unit(s) on capture, code added thanks to claude ai as well as my prompt and adjustments or such or not or yes or etc -->
		unitCaptureClassType = unitInfo.getUnitCaptureClassType()
		if unitCaptureClassType != -1:
			unitCaptureClassTypeInfo = gc.getUnitClassInfo(unitCaptureClassType)
			if unitCaptureClassTypeInfo:
				captureText = localText.getText("TXT_KEY_UNIT_MAY_GRANT_UNITS_ON_CAPTURE", ())
				szSpecialText += u"\n%s%s: %s" % (bullet, captureText, unitCaptureClassTypeInfo.getType())

		# <!-- custom: add ai info for players and me too hehe: should be valuable for info or balancing, added with the help of claude ai and my prompts and adjustments and inspect or and such...
		# if players want to see it xd, as for me yes i want! -->
		if IS_SHOW_AI_INFO:
			# Add Default UnitAI
			defaultUnitAI = unitInfo.getDefaultUnitAIType()
			if defaultUnitAI != -1:
				# Remove the "UNITAI_" prefix for cleaner display
				defaultUnitAIText = str(defaultUnitAI).replace("UNITAI_", "")
			else:
				# <!-- custom: also show the absence of it (as of now shown as "NO_UNITAI" in sevopedia unit it seems, abbreviated as of now to "_" for visual clarity / quick scanning) -->
				defaultUnitAIText =	"_"
			# <!-- custom: note: separate info from other non AI entries more cleanly / clearly, so on more new line for first AI entry -->
			szSpecialText += "\n\n%sDefault UnitAI: %s" % (bullet, defaultUnitAIText)

			# Add all <!-- custom: UnitAIs -->
			unitAIsText = ""
			for i in xrange(UnitAITypes.NUM_UNITAI_TYPES):
				if unitInfo.getUnitAIType(i):
					if unitAIsText:
						unitAIsText += ", "
					unitAIsText += gc.getUnitAIInfo(i).getType().replace("UNITAI_", "")
			if not unitAIsText:
				unitAIsText = "_"
			szSpecialText += "\n%sUnitAIs: %s" % (bullet, unitAIsText)

			# Add all <!-- custom: NotUnitAIs -->
			notUnitAIsText = ""
			for i in xrange(UnitAITypes.NUM_UNITAI_TYPES):
				if unitInfo.getNotUnitAIType(i):
					if notUnitAIsText:
						notUnitAIsText += ", "
					notUnitAIsText += gc.getUnitAIInfo(i).getType().replace("UNITAI_", "")
			if not notUnitAIsText:
				notUnitAIsText = "_"
			szSpecialText += "\n%sNotUnitAIs: %s" % (bullet, notUnitAIsText)

			# Add AIWeight
			aiWeight = unitInfo.getAIWeight()
			szSpecialText += "\n%siAIWeight: %d" % (bullet, aiWeight)

		screen.addMultilineText(listName, SASTextScale.labelText(szSpecialText), self.X_SPECIAL+5, self.Y_SPECIAL+30, self.W_SPECIAL-10, self.H_SPECIAL-35, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeUnitAnimation(self):
		screen = self.top.getScreen()
		iAnimX, iAnimY, iAnimW, iAnimH = draw_expandable_content_panel_container(screen, self.top, u"", self.X_UNIT_ANIMATION, self.Y_UNIT_ANIMATION, self.W_UNIT_ANIMATION, self.H_UNIT_ANIMATION, self.bContentExpanded, SAS_MAGIC_PEDIA_PYTHON_CONTENT_EXPAND, SAS_MAGIC_PEDIA_PYTHON_CONTENT_RELOAD)
		screen.addUnitGraphicGFC(self.top.getNextWidgetName(), self.iUnit, iAnimX, iAnimY, iAnimW, iAnimH, WidgetTypes.WIDGET_GENERAL, -1, -1, X_ROTATION_ANIMATION, Z_ROTATION_ANIMATION, self.SCALE_ANIMATION, True)

	# <!-- custom: placeReplace (renamed from placeReplacements) was originally derived from RFC Dawn of Civilization's CvPediaUnit.py and subsequently adapted for AdvCiv-SAS. (ChatGPT-5.6-Sol) -->
	def placeReplace(self):
		xPanel = self.X_REPLACE
		yPanel = self.Y_REPLACE
		wPanel = self.W_REPLACE
		hPanel = self.H_REPLACE

		iUnitClass = gc.getUnitInfo(self.iUnit).getUnitClassType()
		iBaseUnit = gc.getUnitClassInfo(iUnitClass).getDefaultUnitIndex()

		# Use different text key depending on whether this is a unique or base unit
		if self.iUnit != iBaseUnit:
			# This is a unique unit that replaces a base unit
			txtKeyPanel = "TXT_KEY_PEDIA_REPLACE_REPLACES_CUSTOM"
		else:
			# This is a base unit that can be replaced by unique units
			txtKeyPanel = "TXT_KEY_PEDIA_REPLACE_REPLACED_BY_CUSTOM"

		screen = self.top.getScreen()
		panel = self.top.getNextWidgetName()

		# Create panel with proper styling
		screen.addPanel(panel, localText.getText(txtKeyPanel, ()), "", False, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50)
		# <!-- custom: additionnal left side padding for the button(s) -->
		screen.attachLabel(panel, "", "  ")

		# <!-- custom: handle no unit found message -->
		isButtonFound = False

		# If this is a unique (i.e.civ-specific) unit, show the base unit it replaces
		if self.iUnit != iBaseUnit:
			isButtonFound = True
			screen.attachImageButton(panel, "", gc.getUnitInfo(iBaseUnit).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iBaseUnit, 1, False)
			return

		else:
			# If this is the base building, show all unique (i.e.civ-specific) buildings that replace it
			for iUnit in xrange(gc.getNumUnitInfos()):
				if self.iUnit != iUnit and not gc.getUnitInfo(iUnit).isGraphicalOnly():
					if iUnitClass == gc.getUnitInfo(iUnit).getUnitClassType():
						isButtonFound = True
						screen.attachImageButton(panel, "", gc.getUnitInfo(iUnit).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)

		if not isButtonFound:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel, "TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_NOTHING")

	# <!-- custom: note: this sevopediaunit's below placeCivilizations function/method can handle several civs, see sevopedia building's placeCivilizations's code comment for details -->
	def placeCivilizations(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

		# Create panel with proper styling
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_CIVILIZATIONS", ()), "", False, True, self.X_CIVILIZATIONS, self.Y_CIVILIZATIONS, self.W_CIVILIZATIONS, self.H_CIVILIZATIONS, PanelStyles.PANEL_STYLE_BLUE50)
		# <!-- custom: additionnal left side padding for the button(s) -->
		screen.attachLabel(panelName, "", "  ")

		# Get unit class info
		iUnitClass = gc.getUnitInfo(self.iUnit).getUnitClassType()
		iDefaultUnit = gc.getUnitClassInfo(iUnitClass).getDefaultUnitIndex()

		# Check if this is a unique (i.e.civ-specific) unit (not the default unit for its class)
		bIsUnique = (self.iUnit != iDefaultUnit)

		# If this is a unique (i.e.civ-specific) unit, show which civ can build it
		if bIsUnique:
			# Find which civ has this unique (i.e.civ-specific) unit
			for iCiv in range(gc.getNumCivilizationInfos()):
				# <!-- custom: include barbarians and perhaps other non playable civs in the display -->
				#if gc.getCivilizationInfo(iCiv).isPlayable():
				iCivUnit = gc.getCivilizationInfo(iCiv).getCivilizationUnits(iUnitClass)
				if iCivUnit == self.iUnit:
					screen.attachImageButton(panelName, "", gc.getCivilizationInfo(iCiv).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIV, iCiv, -1, False)
		# <!-- custom: A class-default unit is not available to civilizations that map its class to a replacement or NONE.
		# Keep the one-token panel as `AllR`, with its exact All Remaining Civilizations scope explained in the Sevopedia Legend, instead of redundantly listing every ordinary civilization. See KI#321. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		else:
			# <!-- custom: prettier display -->
			textName = self.top.getNextWidgetName()
			szText = localText.getText("TXT_KEY_PEDIA_SAS_ALL_REMAINING_CIVS", ())
			yCenterPanel = self.Y_CIVILIZATIONS + (self.H_CIVILIZATIONS / 2)
			screen.addMultilineText(textName, SASTextScale.labelText(szText), self.X_CIVILIZATIONS + 7, yCenterPanel, self.W_CIVILIZATIONS - 14, self.H_CIVILIZATIONS - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	# <!-- custom: placeObsoleteWith - mirrors SevoPediaBuilding's obsolete with panel (Claude code Opus 4.5) -->
	def placeObsoleteWith(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

		# Create panel with proper styling - use same text key as SevoPediaBuilding
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_OBSOLETE", ()), "", False, True, self.X_OBSOLETE_WITH, self.Y_OBSOLETE_WITH, self.W_OBSOLETE_WITH, self.H_OBSOLETE_WITH, PanelStyles.PANEL_STYLE_BLUE50)
		# Additional left side padding for the button(s)
		screen.attachLabel(panelName, "", "  ")

		unitInfo = gc.getUnitInfo(self.iUnit)

		# Check if the unit has an obsolete tech
		iObsoleteTech = unitInfo.getObsoleteTech()

		if iObsoleteTech != -1:
			screen.attachImageButton(panelName, "", gc.getTechInfo(iObsoleteTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iObsoleteTech, -1, False)
		else:
			# No obsolete tech - show "Never" message (same as SevoPediaBuilding)
			textName = self.top.getNextWidgetName()
			szText = localText.getText("TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_NEVER", ())
			yCenterPanel = self.Y_OBSOLETE_WITH + (self.H_OBSOLETE_WITH / 2)
			screen.addMultilineText(textName, SASTextScale.labelText(szText), self.X_OBSOLETE_WITH + 7, yCenterPanel, self.W_OBSOLETE_WITH - 14, self.H_OBSOLETE_WITH - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeHistory(self):
		screen = self.top.getScreen()
		szText = self._getHistoryText()
		szTitle = localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ())
		draw_expandable_text_panel(screen, self.top, szTitle, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, szText, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)
	def placePromotions(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		# <!-- custom: no TXT_KEY_PEDIA_CATEGORY_PROMOTION "header" for smoother display with how the unit pane is done (and promo pane is next to it now) -->
		screen.addPanel(panelName, localText.getText("", ()), "", True, True, self.X_PROMO_PANE, self.Y_PROMO_PANE, self.W_PROMO_PANE, self.H_PROMO_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		rowListName = self.top.getNextWidgetName()
		screen.addMultiListControlGFC(rowListName, "", self.X_PROMO_PANE+15, self.Y_PROMO_PANE+40, self.W_PROMO_PANE-20, self.H_PROMO_PANE-40, SEVOPEDIA_MULTILIST_NUM_LISTS_AUTO_CALCULATE, self.PROMOTION_ICON_SIZE, self.PROMOTION_ICON_SIZE, TableStyles.TABLE_STYLE_STANDARD)

		# <!-- custom: disabling entirely if (isPromotionValid(k, self.iUnit, False) we get too many promotions, but enabling it some are missing (see below in code comments), so as advised by chatgpt checking which promotions belong to the unit through another condition, seems to solve/fix the issue as now we see the missing promotions as per the xml -->
		eUnitCombat = gc.getUnitInfo(self.iUnit).getUnitCombatType()
		# No promotions to show for units with no combat type
		if eUnitCombat == -1:
			return

		for k in range(gc.getNumPromotionInfos()):
			# <!-- custom: disable isPromotionValid(k, self.iUnit, False) check as some promotions are missing such as collateral damage 1 and 2 and also leadership promotion in the generic swordsman panel for example, as advised by chatgpt, (it said "In-game, a Swordsman may earn the promotion eventually (e.g. through experience), but if it doesn't yet satisfy all prereqs, isPromotionValid might return False" which i don't know if it is accurate; check if accurate.). -->
			#if (isPromotionValid(k, self.iUnit, False) and not gc.getPromotionInfo(k).isGraphicalOnly()):
			if gc.getPromotionInfo(k).getUnitCombat(eUnitCombat) > 0 and not gc.getPromotionInfo(k).isGraphicalOnly():
				screen.appendMultiListButton(rowListName, gc.getPromotionInfo(k).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, k, -1, False)

	def handleInput (self, inputClass):
		return 0

