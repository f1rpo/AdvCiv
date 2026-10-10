# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: originally derived from RFC Dawn of Civilization's CvPediaTerrain.py; subsequently substantially adjusted and extended for AdvCiv-SAS. (ChatGPT-5.6-Sol) -->
#

from CvPythonExtensions import *
from SASMagicNumbers import *
import CvUtil
from SASFontUtils import *
import SASTextScale
from SASUtils import getInfoTypeOrFail

from _sevopedia_helpers import *

gc = CyGlobalContext()
localText = CyTranslator()

class SevoPediaTerrain:
	def __init__(self, main):
		self.iTerrain = -1
		self.top = main
		self.bHistoryExpanded = False

		self.W_LEFT_COLUMN = get_panel_width_for_buttons(5, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)

		self.X_INFO_PANE = self.top.X_PEDIA_PAGE
		self.Y_INFO_PANE = self.top.Y_PEDIA_PAGE
		self.W_INFO_PANE = self.W_LEFT_COLUMN
		self.H_INFO_PANE = 120

		self.W_ICON = 100
		self.H_ICON = 100
		self.X_ICON = self.X_INFO_PANE + 10
		self.Y_ICON = self.Y_INFO_PANE + 10

		self.X_INFO_TEXT = self.X_INFO_PANE + 110
		self.Y_INFO_TEXT = self.Y_ICON + 15
		self.W_INFO_TEXT = 260
		self.H_INFO_TEXT = self.H_INFO_PANE - 20

		self.X_BUILD_UNITS = self.X_INFO_PANE + self.W_INFO_PANE + MEDIUM_MARGIN
		self.Y_BUILD_UNITS = self.Y_INFO_PANE
		self.W_BUILD_UNITS = self.top.R_PEDIA_PAGE - self.X_BUILD_UNITS
		self.H_BUILD_UNITS = self.H_INFO_PANE

		self.X_FEATURES = self.X_INFO_PANE
		self.Y_FEATURES = self.Y_INFO_PANE + self.H_INFO_PANE + SMALL_MARGIN
		self.W_FEATURES = self.W_LEFT_COLUMN
		self.H_FEATURES = NON_MULTILIST_PANEL_STANDARD_HEIGHT

		self.X_IMPROVEMENTS = self.X_FEATURES + self.W_FEATURES + MEDIUM_MARGIN
		self.Y_IMPROVEMENTS = self.Y_FEATURES
		self.W_IMPROVEMENTS = self.top.R_PEDIA_PAGE - self.X_IMPROVEMENTS
		self.H_IMPROVEMENTS = self.H_FEATURES

		self.X_BONUSES_WITH_NO_FEATURE = self.X_FEATURES
		self.Y_BONUSES_WITH_NO_FEATURE = self.Y_FEATURES + self.H_FEATURES + SMALL_MARGIN
		self.W_BONUSES_WITH_NO_FEATURE = self.top.R_PEDIA_PAGE - self.X_BONUSES_WITH_NO_FEATURE
		self.H_BONUSES_WITH_NO_FEATURE = self.H_FEATURES

		self.X_BONUSES_ONLY_WITH_FEATURE = self.X_FEATURES
		self.Y_BONUSES_ONLY_WITH_FEATURE = self.Y_BONUSES_WITH_NO_FEATURE + self.H_BONUSES_WITH_NO_FEATURE + SMALL_MARGIN
		self.W_BONUSES_ONLY_WITH_FEATURE = self.top.R_PEDIA_PAGE - self.X_BONUSES_ONLY_WITH_FEATURE
		self.H_BONUSES_ONLY_WITH_FEATURE = self.H_FEATURES

		# <!-- custom: see code comment at self.H_MULTILIST_MULTIPLE_ROWS_BUTTON_SIZE in sevopedia unit for details -->
		self.H_RELEVANT_UNITS = self.H_FEATURES + MULTILIST_BUTTON_SIZE

		self.X_RELEVANT_UNITS = self.X_FEATURES
		self.Y_RELEVANT_UNITS = self.Y_BONUSES_ONLY_WITH_FEATURE + self.H_BONUSES_ONLY_WITH_FEATURE + SMALL_MARGIN
		self.W_RELEVANT_UNITS = self.top.R_PEDIA_PAGE - self.X_BONUSES_ONLY_WITH_FEATURE
		self.H_RELEVANT_UNITS = self.H_FEATURES + MULTILIST_BUTTON_SIZE

		self.X_UNITS_IMPASSABLE = self.X_FEATURES
		self.Y_UNITS_IMPASSABLE = self.Y_RELEVANT_UNITS + self.H_RELEVANT_UNITS + SMALL_MARGIN
		self.W_UNITS_IMPASSABLE = self.top.R_PEDIA_PAGE - self.X_RELEVANT_UNITS
		self.H_UNITS_IMPASSABLE = self.H_FEATURES + HIDE_SECOND_ROW_MULTI_LIST

		self.X_SPECIAL = self.X_INFO_PANE
		self.Y_SPECIAL = self.Y_UNITS_IMPASSABLE + self.H_UNITS_IMPASSABLE + SMALL_MARGIN
		self.W_SPECIAL = self.W_LEFT_COLUMN
		self.H_SPECIAL = self.top.B_PEDIA_PAGE - self.Y_SPECIAL

		self.X_HISTORY = self.X_SPECIAL + self.W_SPECIAL + MEDIUM_MARGIN
		self.Y_HISTORY = self.Y_SPECIAL
		self.W_HISTORY = self.top.R_PEDIA_PAGE - self.X_HISTORY
		self.H_HISTORY = self.H_SPECIAL

		self.I_TERRAIN_PEAK = getInfoTypeOrFail("TERRAIN_PEAK")
		self.I_TERRAIN_HILL = getInfoTypeOrFail("TERRAIN_HILL")
		self.I_TERRAIN_COAST = getInfoTypeOrFail("TERRAIN_COAST")
		self.I_TERRAIN_OCEAN = getInfoTypeOrFail("TERRAIN_OCEAN")

		self.I_PROMOTION_HILLS_MASTER1 = getInfoTypeOrFail("PROMOTION_HILLS_MASTER1")
		self.I_PROMOTION_HILLS_MASTER2 = getInfoTypeOrFail("PROMOTION_HILLS_MASTER2")
		self.I_PROMOTION_HILLS_MASTER3 = getInfoTypeOrFail("PROMOTION_HILLS_MASTER3")
		self.I_PROMOTION_NAVIGATOR = getInfoTypeOrFail("PROMOTION_NAVIGATOR")

	def interfaceScreen(self, iTerrain):
		if self.iTerrain != iTerrain:
			self.bHistoryExpanded = False
		self.iTerrain = iTerrain

		self.placeInfo()
		self.placeBuildUnits()
		self.placeFeatures()
		self.placeImprovements()
		self.placeBonusesWithNoFeature()
		self.placeBonusesOnlyWithFeature()
		self.placeRelevantUnits()
		self.placeUnitsImpassable()
		self.placeSpecial()
		self.placeHistory()
		place_new_concept_legend_link(self.top, "CONCEPT_SAS_SEVOPEDIA_NUMTXT_LEGEND")

	def placeInfo(self):
		screen = self.top.getScreen()
		panel = self.top.getNextWidgetName()
		info = gc.getTerrainInfo(self.iTerrain)

		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_INFO_PANE, self.Y_INFO_PANE, self.W_INFO_PANE, self.H_INFO_PANE, PanelStyles.PANEL_STYLE_BLUE50)

		# <!-- custom: was PanelStyles.PANEL_STYLE_MAIN -->
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_EMPTY)
		screen.addDDSGFC(self.top.getNextWidgetName(), info.getButton(), self.X_ICON + self.W_ICON / 2 - PANE_ICON_SIZE / 2, self.Y_ICON + self.H_ICON / 2 - PANE_ICON_SIZE / 2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1)

		screen.addListBoxGFC(panel, "", self.X_INFO_TEXT, self.Y_INFO_TEXT, self.W_INFO_TEXT, self.H_INFO_TEXT, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(panel, False)
		screen.appendListBoxString(panel, SASTextScale.titleText(info.getDescription()), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

		# <!-- custom: replace the earlier hardcoded "Terrain" label with a dedicated text key. (ChatGPT-5.6-Sol) -->
		# <!-- custom: also display some terrains as plot types / terrains, see xml txt key code comment for details. -->
		if self.iTerrain == self.I_TERRAIN_PEAK or self.iTerrain == self.I_TERRAIN_HILL or self.iTerrain == self.I_TERRAIN_OCEAN:
			txtKeyTerrainPlotTypeLabel = "TXT_KEY_PEDIA_PLOT_TYPE_TERRAIN_CUSTOM"
		else:
			txtKeyTerrainPlotTypeLabel = "TXT_KEY_PEDIA_TERRAIN_CUSTOM"

		screen.appendListBoxString(panel, SASTextScale.labelText(localText.getText(txtKeyTerrainPlotTypeLabel, ())), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

		if self.iTerrain == self.I_TERRAIN_HILL:
			szStats = (u"%d%c  " % (-1, gc.getYieldInfo(YieldTypes.YIELD_FOOD).getChar()))
			szStats += (u"+%d%c  " % (1, gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar()))
		else:
			szStats = u""
			for iYield in xrange(YieldTypes.NUM_YIELD_TYPES):
				iYieldChange = info.getYield(iYield)
				if iYieldChange != 0:
					szStats += (u"%d%c  " % (iYieldChange, gc.getYieldInfo(iYield).getChar()))

		screen.appendListBoxString(panel, SASTextScale.titleText(szStats), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

	# <!-- custom: The Improvements and Units (Any Build) panels described the same buildable-improvement relation through different predicates, so Hill and water improvements could appear without their capable units.
	# Share the complete terrain validity reconstruction across both panels. See KI#302. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	def isImprovementValidForTerrain(self, iImprovement):
		ImprovementInfo = gc.getImprovementInfo(iImprovement)
		if ImprovementInfo.isGraphicalOnly() or ImprovementInfo.isGoody() or self.iTerrain == self.I_TERRAIN_PEAK:
			return False
		if self.iTerrain == self.I_TERRAIN_HILL:
			return ImprovementInfo.isHillsMakesValid() or self.isImprovementValidThroughHillTerrain(iImprovement) or self.isImprovementValidThroughHillFeature(iImprovement) or self.isImprovementValidThroughHillBonus(iImprovement)
		return ImprovementInfo.getTerrainMakesValid(self.iTerrain) or (ImprovementInfo.isWater() and (self.iTerrain == self.I_TERRAIN_COAST or self.iTerrain == self.I_TERRAIN_OCEAN))

	def placeBuildUnits(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		iActivePlayer = gc.getGame().getActivePlayer()

		improvementBuilds = []

		for iBuild in xrange(gc.getNumBuildInfos()):
			buildInfo = gc.getBuildInfo(iBuild)
			iImprovement = buildInfo.getImprovement()
			if iImprovement != -1 and self.isImprovementValidForTerrain(iImprovement):
				improvementBuilds.append(iBuild)

		if len(improvementBuilds) == 0:
			return

		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_UNITS_ANY_BUILD", ()), "", False, True, self.X_BUILD_UNITS, self.Y_BUILD_UNITS, self.W_BUILD_UNITS, self.H_BUILD_UNITS, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		bUnitFound = False
		for iUnit in xrange(gc.getNumUnitInfos()):
			unitInfo = gc.getUnitInfo(iUnit)
			if unitInfo.isGraphicalOnly():
				continue

			bCanBuild = False
			for iBuild in improvementBuilds:
				if unitInfo.getBuilds(iBuild):
					bCanBuild = True
					break

			if bCanBuild:
				szButton = unitInfo.getButton()
				if iActivePlayer >= 0:
					szButton = gc.getPlayer(iActivePlayer).getUnitButton(iUnit)
				screen.attachImageButton(panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)
				bUnitFound = True

		if not bUnitFound:
			draw_none_text(screen, self.top, self.X_BUILD_UNITS, self.Y_BUILD_UNITS, self.W_BUILD_UNITS, self.H_BUILD_UNITS)

	def placeSpecial(self):
		xPanel = self.X_SPECIAL
		yPanel = self.Y_SPECIAL
		wPanel = self.W_SPECIAL
		hPanel = self.H_SPECIAL

		screen = self.top.getScreen()
		panel = self.top.getNextWidgetName()
		text = self.top.getNextWidgetName()
		info = gc.getTerrainInfo(self.iTerrain)

		screen.addPanel(panel, localText.getText("TXT_KEY_PEDIA_SPECIAL_ABILITIES", ()), "", True, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50)

		# <!-- custom: preserve the earlier adopted peak-entry behavior for now, with only the iHill refactor above; peak information remains incomplete (for example, impassability is not covered here). (ChatGPT-5.6-Sol) -->
		if self.iTerrain == self.I_TERRAIN_PEAK or self.iTerrain == self.I_TERRAIN_HILL:
			txtKeyNoDisplay = "TXT_KEY_PEDIA_TERRAIN_EXCLUDED_FROM_DISPLAY_PLOT_TYPE_WITH_EXPLANATION"
			textName = self.top.getNextWidgetName()
			szText = localText.getText(txtKeyNoDisplay, ())
			# <!-- custom: note: do not use yPanelCenter as this is a panel with quite high height, higher (no pun) than default or usual panel height, and it seems that maybe the panel's height is not a clean as chatgpt said this word clean to rephrase my more explanation and question of it maybe not being a multiplier of one line height, so starting from center it would overfill (as it would start a bit below the exact half if inner panel does not have an exactly aligned total height being a line multiplier maybe, check if accurate, so to solve this start a bit higher than the center instead. -->
			#yPanelCenter = yPanel + (hPanel / 2)
			yPanelCenter = yPanel + int(0.42 * hPanel)
			screen.addMultilineText(textName, SASTextScale.labelText(szText), xPanel + 7, yPanelCenter, wPanel - 14, hPanel - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		else:
			szText = info.getHelp()
			szText += CyGameTextMgr().getTerrainHelp(self.iTerrain, True)
			szText = szText.replace("\n\n", "\n").strip()
			screen.addMultilineText(text, SASTextScale.labelText(szText), self.X_SPECIAL + 10, self.Y_SPECIAL + 30, self.W_SPECIAL - 20, self.H_SPECIAL - 40, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeFeatures(self):
		xPanel = self.X_FEATURES
		yPanel = self.Y_FEATURES
		wPanel = self.W_FEATURES
		hPanel = self.H_FEATURES

		screen = self.top.getScreen()
		panel = self.top.getNextWidgetName()

		screen.addPanel(panel, localText.getText("TXT_KEY_MISC_FEATURES", ()), "", False, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panel, "", "  ")

		# <!-- custom: not sure we have a reason to as display is currently empty, but just in case or for consistency, also exclude from display here -->
		if self.iTerrain == self.I_TERRAIN_PEAK or self.iTerrain == self.I_TERRAIN_HILL:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel, "TXT_KEY_PEDIA_TERRAIN_EXCLUDED_FROM_DISPLAY_PLOT_TYPE")

		else:
			for iFeature in xrange(gc.getNumFeatureInfos()):
				FeatureInfo = gc.getFeatureInfo(iFeature)

				if FeatureInfo.isGraphicalOnly():
					continue
				elif FeatureInfo.isTerrain(self.iTerrain):
					screen.attachImageButton(panel, "", FeatureInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_FEATURE, iFeature, 1, False)

	# <!-- custom: TERRAIN_HILL is a pedia stand-in for a plot shape layered over a real terrain; the DLL rules are correct, but Sevopedia had to reconstruct them for display.
	# In-game plains hill Grapes needs a Plantation, but the Hill page did not show Plantation; see KI#153 for this and other Hill improvement edge cases. (GPT-5.5) -->
	def isImprovementValidThroughHillBonus(self, iImprovement):
		ImprovementInfo = gc.getImprovementInfo(iImprovement)
		for iBonus in xrange(gc.getNumBonusInfos()):
			BonusInfo = gc.getBonusInfo(iBonus)
			if BonusInfo.isGraphicalOnly() or not BonusInfo.isHills():
				continue
			if ImprovementInfo.isImprovementBonusTrade(iBonus):
				return True
		return False

	def isImprovementValidThroughHillTerrain(self, iImprovement):
		ImprovementInfo = gc.getImprovementInfo(iImprovement)
		if ImprovementInfo.isRequiresFlatlands():
			return False
		for iTerrain in xrange(gc.getNumTerrainInfos()):
			if iTerrain == self.I_TERRAIN_HILL or iTerrain == self.I_TERRAIN_PEAK:
				continue
			TerrainInfo = gc.getTerrainInfo(iTerrain)
			if TerrainInfo.isWater():
				continue
			if ImprovementInfo.getTerrainMakesValid(iTerrain):
				return True
		return False

	def isImprovementValidThroughHillFeature(self, iImprovement):
		ImprovementInfo = gc.getImprovementInfo(iImprovement)
		for iFeature in xrange(gc.getNumFeatureInfos()):
			FeatureInfo = gc.getFeatureInfo(iFeature)
			if FeatureInfo.isGraphicalOnly() or FeatureInfo.isRequiresFlatlands():
				continue
			if not ImprovementInfo.getFeatureMakesValid(iFeature):
				continue
			for iTerrain in xrange(gc.getNumTerrainInfos()):
				if iTerrain == self.I_TERRAIN_HILL or iTerrain == self.I_TERRAIN_PEAK:
					continue
				TerrainInfo = gc.getTerrainInfo(iTerrain)
				if TerrainInfo.isWater():
					continue
				if FeatureInfo.isTerrain(iTerrain):
					return True
		return False

	def placeImprovements(self):
		xPanel = self.X_IMPROVEMENTS
		yPanel = self.Y_IMPROVEMENTS
		wPanel = self.W_IMPROVEMENTS
		hPanel = self.H_IMPROVEMENTS

		screen = self.top.getScreen()
		panel = self.top.getNextWidgetName()

		screen.addPanel(panel, localText.getText("TXT_KEY_PEDIA_IMPROVEMENTS_CUSTOM", ()), "", False, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panel, "", "  ")

		# <!-- custom: exclude peak (no improvements), but show hills using improvement rules instead of skipping; hills were previously hidden because output was unreliable (forts missing, cottages conditional on food, forest preserve dependent on forest/jungle). (GPT-5.2-Codex + GPT-5.5) -->
		if self.iTerrain == self.I_TERRAIN_PEAK:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel, "TXT_KEY_PEDIA_TERRAIN_EXCLUDED_FROM_DISPLAY_PLOT_TYPE")

		else:
			for iImprovement in xrange(gc.getNumImprovementInfos()):
				ImprovementInfo = gc.getImprovementInfo(iImprovement)
				if self.isImprovementValidForTerrain(iImprovement):
					screen.attachImageButton(panel, "", ImprovementInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_IMPROVEMENT, iImprovement, 1, False)

	def placeBonusesWithNoFeature(self):
		xPanel = self.X_BONUSES_WITH_NO_FEATURE
		yPanel = self.Y_BONUSES_WITH_NO_FEATURE
		wPanel = self.W_BONUSES_WITH_NO_FEATURE
		hPanel = self.H_BONUSES_WITH_NO_FEATURE

		screen = self.top.getScreen()
		panel = self.top.getNextWidgetName()

		# <!-- custom: explicitly require the TERRAIN_HILL id instead of silently continuing when it is missing; this also refactors the earlier adopted peak handling. (ChatGPT-5.6-Sol) -->
		txtKeyPanel = "TXT_KEY_PEDIA_TERRAIN_BONUSES_WITH_NO_FEATURE"
		if self.iTerrain == self.I_TERRAIN_PEAK:
			txtKeyPanel = "TXT_KEY_PEDIA_TERRAIN_BONUSES_NOT_APPLICABLE_FOR_THIS_PLOT_TYPE"
		elif self.iTerrain == self.I_TERRAIN_HILL:
			txtKeyPanel = "TXT_KEY_PEDIA_TERRAIN_BONUSES_HILL"

		screen.addPanel(panel, localText.getText(txtKeyPanel, ()), "", False, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panel, "", "  ")

		# <!-- custom: not applicable for this plot type / terrain, so show an alternative text instead -->
		if self.iTerrain == self.I_TERRAIN_PEAK:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel, "TXT_KEY_PEDIA_TERRAIN_EXCLUDED_FROM_DISPLAY_PLOT_TYPE")

		else:
			for iBonus in xrange(gc.getNumBonusInfos()):
				bonusInfo = gc.getBonusInfo(iBonus)
				if bonusInfo.isGraphicalOnly():
					continue
				# <!-- custom: refactor the earlier adopted condition without changing output: check the general all-terrains case before the hill-specific case. (ChatGPT-5.6-Sol) -->
				elif (bonusInfo.isTerrain(self.iTerrain)) or (self.iTerrain == self.I_TERRAIN_HILL and bonusInfo.isHills()):
					screen.attachImageButton(panel, "", bonusInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus, 1, False)

	# <!-- custom: code provided by chatgpt thanks to my prompts too and adjustments too -->
	def placeBonusesOnlyWithFeature(self):
		xPanel = self.X_BONUSES_ONLY_WITH_FEATURE
		yPanel = self.Y_BONUSES_ONLY_WITH_FEATURE
		wPanel = self.W_BONUSES_ONLY_WITH_FEATURE
		hPanel = self.H_BONUSES_ONLY_WITH_FEATURE

		screen = self.top.getScreen()
		panel = self.top.getNextWidgetName()

		txtKeyPanel = "TXT_KEY_PEDIA_TERRAIN_BONUSES_FEATURE_TERRAIN_BOOLEANS"
		if self.iTerrain == self.I_TERRAIN_HILL or self.iTerrain == self.I_TERRAIN_PEAK:
			txtKeyPanel = "TXT_KEY_PEDIA_TERRAIN_BONUSES_NOT_APPLICABLE_FOR_THIS_PLOT_TYPE"

		screen.addPanel(panel, localText.getText(txtKeyPanel, ()), "", False, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panel, "", "  ")

		if self.iTerrain == self.I_TERRAIN_HILL or self.iTerrain == self.I_TERRAIN_PEAK:
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel, "TXT_KEY_PEDIA_TERRAIN_EXCLUDED_FROM_DISPLAY_PLOT_TYPE")

		else:
			for iBonus in xrange(gc.getNumBonusInfos()):
				BonusInfo = gc.getBonusInfo(iBonus)
				if BonusInfo.isGraphicalOnly():
					continue

				if BonusInfo.isFeatureTerrain(self.iTerrain):
					screen.attachImageButton(panel, "", BonusInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus, 1, False)

	# <!-- custom: code provided with the help of chatgpt thanks -->
	def placeRelevantUnits(self):
		xPanel = self.X_RELEVANT_UNITS
		yPanel = self.Y_RELEVANT_UNITS
		wPanel = self.W_RELEVANT_UNITS
		hPanel = self.H_RELEVANT_UNITS

		txtKeyPanel = "TXT_KEY_PEDIA_TERRAIN_FEATURE_RELEVANT_UNITS"

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

		#isButtonFound = False
		iButtonIndex = 0
		maxButtonsPerRow = get_multilist_max_buttons_per_row(multiListW, MULTILIST_BUTTON_SIZE)

		iPeak = self.I_TERRAIN_PEAK
		# <!-- custom: for peak we display units that can walk on the tile rather than those that have modifier, i find this or think this is more informative -->
		# <!-- custom: terrain_hill uses its own kind of modifier, plus also i'd want to show guerilla promotion as well, so handle its display separately -->
		if self.iTerrain == iPeak:
			for iUnit in xrange(gc.getNumUnitInfos()):
				unitInfo = gc.getUnitInfo(iUnit)

				if unitInfo.isGraphicalOnly():
					continue

				# <!-- custom: parts of the below condition(s)/code by chatgpt 5, check if accurate and check if all is accurate if want to be sure -->
				# inside placeRelevantUnits(), in the if self.iTerrain == iPeak: loop
				unitInfoDomain = unitInfo.getDomainType()
				passTech = (unitInfo.getTerrainPassableTech(iPeak) != -1)

				# <!-- custom: also handle water units that can move through all terrains but only on water; also for peak logic is different than for other terrains in placeRelevantUnits, do not place only units that have modifiers for this "terrain" (as is plot type too), but place more broadly any unit, even if it doesn't have a modifier, as long as it can walk on the tile, then display the numTxt or any information optionally if the unit has it, else default to something like "_/_" (no attack or def modifier) or whatever the numTxt generating function gives us. -->
				# <!-- custom: also show boat with legs, in case some crazy mod mod nicely impelments this xd or us but less likely or not or yes or etc -->
				# Peak — Relevant Units (includes "boat with legs"; All-Terrain short-circuits)
				can_walk_on_peak = (unitInfo.isCanMoveAllTerrain() or (unitInfoDomain == DomainTypes.DOMAIN_LAND and (unitInfo.isCanMoveImpassable() or passTech)))

				if can_walk_on_peak:
					screen.appendMultiListButton(rowListName, unitInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)

					iTerrainAttack = unitInfo.getTerrainAttackModifier(self.iTerrain)
					iTerrainDefense = unitInfo.getTerrainDefenseModifier(self.iTerrain)

					numTxt = get_numTxt_attack_defense_modifiers(iTerrainAttack, iTerrainDefense)
					extraCorrectionX = get_extra_correction_x(numTxt)
					add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

					#isButtonFound = True
					iButtonIndex += 1

		elif self.iTerrain == self.I_TERRAIN_HILL:
			# <!-- custom: raise an error if asset does not exist (in advciv-sas we have renamed PROMOTION_GUERILLA1 to PROMOTION_HILLS_MASTER1 and such) -->
			for iUnit in xrange(gc.getNumUnitInfos()):
				unitInfo = gc.getUnitInfo(iUnit)

				if unitInfo.isGraphicalOnly():
					continue

				iHillsAttack = unitInfo.getHillsAttackModifier()
				iHillsDefense = unitInfo.getHillsDefenseModifier()

				isHasHM1 = unitInfo.getFreePromotions(self.I_PROMOTION_HILLS_MASTER1)
				isHasHM2 = unitInfo.getFreePromotions(self.I_PROMOTION_HILLS_MASTER2)
				isHasHM3 = unitInfo.getFreePromotions(self.I_PROMOTION_HILLS_MASTER3)

				if ((iHillsAttack != 0) or (iHillsDefense != 0) or isHasHM1 or isHasHM2 or isHasHM3):
					screen.appendMultiListButton(rowListName, unitInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)

					if (iHillsAttack != 0 or iHillsDefense != 0):
						numTxt = get_numTxt_attack_defense_modifiers(iHillsAttack, iHillsDefense)
					else:
						numTxt = get_numTxt_promotion_tier_compact("HM", isHasHM1, isHasHM2, isHasHM3)

					extraCorrectionX = get_extra_correction_x(numTxt)
					add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

					#isButtonFound = True
					iButtonIndex += 1

		elif self.iTerrain == self.I_TERRAIN_COAST or self.iTerrain == self.I_TERRAIN_OCEAN:
			# <!-- custom: raise an error if asset does not exist (in advciv-sas we have renamed PROMOTION_GUERILLA1 to PROMOTION_HILLS_MASTER1 and such) -->
			for iUnit in xrange(gc.getNumUnitInfos()):
				unitInfo = gc.getUnitInfo(iUnit)

				if unitInfo.isGraphicalOnly():
					continue

				iTerrainAttack = unitInfo.getTerrainAttackModifier(self.iTerrain)
				iTerrainDefense = unitInfo.getTerrainDefenseModifier(self.iTerrain)

				isHasN = unitInfo.getFreePromotions(self.I_PROMOTION_NAVIGATOR)

				if (unitInfo.isCanMoveAllTerrain() or ((unitInfo.getDomainType() == DomainTypes.DOMAIN_SEA) and (not unitInfo.getTerrainImpassable(self.iTerrain)) and (unitInfo.getTerrainPassableTech(self.iTerrain) == -1)) or (iTerrainAttack != 0) or (iTerrainDefense != 0) or isHasN):
					screen.appendMultiListButton(rowListName, unitInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)

					if iTerrainAttack != 0 or iTerrainDefense != 0:
						numTxt = get_numTxt_attack_defense_modifiers(iTerrainAttack, iTerrainDefense)
					elif isHasN:
						numTxt = "N"
					else:
						numTxt = "_/_"  # or "" if you prefer no tag

					extraCorrectionX = get_extra_correction_x(numTxt)
					add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

					#isButtonFound = True
					iButtonIndex += 1

		else:
			for iUnit in xrange(gc.getNumUnitInfos()):
				unitInfo = gc.getUnitInfo(iUnit)

				if unitInfo.isGraphicalOnly():
					continue

				iTerrainAttack = unitInfo.getTerrainAttackModifier(self.iTerrain)
				iTerrainDefense = unitInfo.getTerrainDefenseModifier(self.iTerrain)

				if iTerrainAttack != 0 or iTerrainDefense != 0:
					screen.appendMultiListButton(rowListName, unitInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)

					numTxt = get_numTxt_attack_defense_modifiers(iTerrainAttack, iTerrainDefense)
					extraCorrectionX = get_extra_correction_x(numTxt)
					add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

					#isButtonFound = True
					iButtonIndex += 1

	# <!-- custom: code provided with the help of chatgpt thanks -->
	def placeUnitsImpassable(self):
		xPanel = self.X_UNITS_IMPASSABLE
		yPanel = self.Y_UNITS_IMPASSABLE
		wPanel = self.W_UNITS_IMPASSABLE
		hPanel = self.H_UNITS_IMPASSABLE

		txtKeyPanel = "TXT_KEY_PEDIA_TERRAIN_UNITS_IMPASSABLE"

		if self.iTerrain == self.I_TERRAIN_PEAK:
			txtKeyPanel = "TXT_KEY_PEDIA_TERRAIN_UNITS_IMPASSABLE_PEAK"

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

		iPeak = self.I_TERRAIN_PEAK
		# <!-- custom: for peak we display units that can walk on the tile rather than those that have modifier, i find this or think this is more informative -->
		if self.iTerrain == iPeak:
			for iUnit in xrange(gc.getNumUnitInfos()):
				unitInfo = gc.getUnitInfo(iUnit)

				if unitInfo.isGraphicalOnly():
					continue

				# <!-- custom: parts of the below condition(s)/code by chatgpt 5, check if accurate and check if all is accurate if want to be sure -->
				# inside placeUnitsImpassable(), in the if self.iTerrain == iPeak: loop
				unitInfoDomain = unitInfo.getDomainType()
				passTech = (unitInfo.getTerrainPassableTech(iPeak) != -1)

				# <!-- custom: "boat with legs" edge case handled in as of now relevant units as chatgpt 5 says as well, if no boat unit at all has legs, as is as of now in advciv-sas, then they would instead be displayed in this units impassable panel for exhaustiveness and notsimply omitted if i understood it correctly, check if accurate -->
				# Peak — Units Impassable (show land w/out bypass; all sea unless All-Terrain)
				# regular ships (DOMAIN_SEA) are blocked on peak
				blocked = ((not unitInfo.isCanMoveAllTerrain()) and ((unitInfoDomain == DomainTypes.DOMAIN_LAND and not (unitInfo.isCanMoveImpassable() or passTech)) or (unitInfoDomain == DomainTypes.DOMAIN_SEA)))

				if blocked:
					screen.appendMultiListButton(rowListName, unitInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)

		elif self.iTerrain == self.I_TERRAIN_COAST or self.iTerrain == self.I_TERRAIN_OCEAN:
			for iUnit in xrange(gc.getNumUnitInfos()):
				unitInfo = gc.getUnitInfo(iUnit)

				if unitInfo.isGraphicalOnly():
					continue

				# <!-- custom: unitInfo.isCanMoveAllTerrain() returns too many units but (not unitInfo.isCanMoveAllTerrain()) does not, so unlike in placeRelevantUnits, wrap this unitInfo.isCanMoveAllTerrain() in placeUnitsImpassable in the domain sea check, ideally i would have wanted to check our xml by having a large permissive check, but the results are unreadable since almost all units have this (not unitInfo.isCanMoveAllTerrain()) that is true -->
				if unitInfo.getDomainType() == DomainTypes.DOMAIN_SEA:
					if ((unitInfo.getTerrainImpassable(self.iTerrain) or (unitInfo.getTerrainPassableTech(self.iTerrain) != -1)) and (not unitInfo.isCanMoveAllTerrain())):
						screen.appendMultiListButton(rowListName, unitInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)

		else:
			# <!-- custom: parts of the below condition(s)/code by chatgpt 5, check if accurate and check if all is accurate if want to be sure -->
			info = gc.getTerrainInfo(self.iTerrain)

			for iUnit in xrange(gc.getNumUnitInfos()):
				unitInfo = gc.getUnitInfo(iUnit)

				if unitInfo.isGraphicalOnly():
					continue

				# Right now you include units if they have a passable tech (inverted logic). To show units that are truly blocked (no bypass), use the same pattern you used for features:
				blocked = ((info.isImpassable() or unitInfo.getTerrainImpassable(self.iTerrain)) and not unitInfo.isCanMoveImpassable() and unitInfo.getTerrainPassableTech(self.iTerrain) == -1 and not unitInfo.isCanMoveAllTerrain())
				if blocked:
					screen.appendMultiListButton(rowListName, unitInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)

	def setHistoryExpanded(self, bExpanded):
		self.bHistoryExpanded = bExpanded

	def placeHistory(self):
		screen = self.top.getScreen()
		szText = gc.getTerrainInfo(self.iTerrain).getCivilopedia()
		szTitle = localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ())
		draw_expandable_text_panel(screen, self.top, szTitle, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, szText, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)
	def handleInput (self, inputClass):
		return 0

