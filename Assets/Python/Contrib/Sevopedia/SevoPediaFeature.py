# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: originally derived from RFC Dawn of Civilization's CvPediaFeature.py; subsequently adjusted for AdvCiv-SAS, including renaming placeDetails to placeSpecial for consistency with other Sevopedia classes. (ChatGPT-5.6-Sol) -->

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

def do_pre_load_xml_features_info_required_data_validation():
	for iFeature in xrange(gc.getNumFeatureInfos()):
		# Start with no known max
		maxSoFarProduction = None
		maxSoFarTime = None

		# Validate consistency across all builds
		# Check all builds for ones that can remove this feature
		for iBuild in xrange(gc.getNumBuildInfos()):
			buildInfo = gc.getBuildInfo(iBuild)

			if buildInfo.isFeatureRemove(iFeature):
				removeProduction = buildInfo.getFeatureProduction(iFeature)

				# <!-- custom: there shouldn't be any negative value, but if relevant in a mod, they may want to also take them into account here -->
				if removeProduction > 0:
					if maxSoFarProduction is None:
						maxSoFarProduction = removeProduction
					else:
						if removeProduction > maxSoFarProduction:
							# <!-- custom: (note: this simplifies display for the where case cottages and farms would have different values (e.g. 20 vs 35 (imaginary values to illustrate)) which would be highly unusual. This edge case is ignored to simplify code, display, and logic, and raising an error instead. Ideally if a mod implements such a feature (the remove iProduction 20 vs 35 for different builds), which would be strange btw as building a farm also has a remove the jungle feature as part of it it is not a different operation at least it seems so ingame anyways, they'd or can remove this check and error, and then alter display to see the different possible values maybe, although again i don't see too much of a case where it may be useful, but just in case and if i am maybe mistaken, you'd need to change this code and then the display logic to display the different value -->
							if maxSoFarProduction > 0:
								raise ValueError(u"[VALUE ERROR] Unexpected different iProduction removeProduction=%d value where a not equal to 0 value maxSoFarProduction=%d already existed between builds (cottage, remove_jungle, farm, etc.) at feature=%d (feature type is %s). Please make sure your iProduction is consistent across all builds that can remove this feature, or if purposely designed as such, modify this code to display such information if you want or remove the error." % (removeProduction, maxSoFarProduction, iFeature, gc.getFeatureInfo(iFeature).getType()))

							maxSoFarProduction = removeProduction

				removeTime = buildInfo.getFeatureTime(iFeature)

				if removeTime > 0:
					if maxSoFarTime is None:
						maxSoFarTime = removeTime
					else:
						if removeTime > maxSoFarTime:
							if maxSoFarTime > 0:
								# We assume all builds (farm, cottage, etc.) that remove the same feature should take same iTime.
								# If not, raise an error here and advise manual display implementation if desired.
								raise ValueError(u"[VALUE ERROR] Unexpected iTime difference=%d when existing maxSoFarTime=%d for builds removing feature=%d (%s). Please ensure all builds that remove this feature share the same iTime, or update display logic to show per-build time if desired." % (removeTime, maxSoFarTime, iFeature, gc.getFeatureInfo(iFeature).getType()))

							maxSoFarTime = removeTime

class SevoPediaFeature:
	def __init__(self, main):
		self.iFeature = -1
		self.bHistoryExpanded = False
		self.top = main

		self.W_LEFT_COLUMN = get_panel_width_for_buttons(5, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)

		self.X_INFO_PANE = self.top.X_PEDIA_PAGE
		self.Y_INFO_PANE = self.top.Y_PEDIA_PAGE
		self.W_INFO_PANE = self.W_LEFT_COLUMN
		# <!-- custom: make some room for the new fields we added in placeSpecial and align display horizontally with placeSpecial, was 120 -->
		self.H_INFO_PANE = 127

		self.W_ICON = 100
		self.H_ICON = 100
		self.X_ICON = self.X_INFO_PANE + 10
		self.Y_ICON = self.Y_INFO_PANE + 10

		self.X_INFO_TEXT = self.X_INFO_PANE + 110
		self.Y_INFO_TEXT = self.Y_ICON + 15
		self.W_INFO_TEXT = 260
		self.H_INFO_TEXT = self.H_INFO_PANE - 20

		self.X_BUILD_REMOVES = self.X_INFO_PANE + self.W_INFO_PANE + MEDIUM_MARGIN
		self.Y_BUILD_REMOVES = self.Y_INFO_PANE
		self.W_BUILD_REMOVES = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.H_BUILD_REMOVES = self.H_INFO_PANE

		self.X_FEATURE_PRODUCTION = self.X_BUILD_REMOVES + self.W_BUILD_REMOVES + MEDIUM_MARGIN
		self.Y_FEATURE_PRODUCTION = self.Y_INFO_PANE
		self.W_FEATURE_PRODUCTION = get_panel_width_for_buttons(2, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.H_FEATURE_PRODUCTION = self.H_INFO_PANE

		self.X_UNITS_REMOVE = self.X_FEATURE_PRODUCTION + self.W_FEATURE_PRODUCTION + MEDIUM_MARGIN
		self.Y_UNITS_REMOVE = self.Y_INFO_PANE
		self.W_UNITS_REMOVE = self.top.R_PEDIA_PAGE - self.X_UNITS_REMOVE
		self.H_UNITS_REMOVE = self.H_INFO_PANE

		self.X_FEATURES = self.X_INFO_PANE
		self.Y_FEATURES = self.Y_INFO_PANE + self.H_INFO_PANE + SMALL_MARGIN
		self.W_FEATURES = self.W_LEFT_COLUMN
		self.H_FEATURES = NON_MULTILIST_PANEL_STANDARD_HEIGHT

		self.X_IMPROVEMENTS = self.X_FEATURES + self.W_FEATURES + MEDIUM_MARGIN
		self.Y_IMPROVEMENTS = self.Y_FEATURES
		self.W_IMPROVEMENTS = self.top.R_PEDIA_PAGE - self.X_IMPROVEMENTS
		self.H_IMPROVEMENTS = self.H_FEATURES

		self.X_BONUSES_ON_ANY_TERRAIN = self.X_FEATURES
		self.Y_BONUSES_ON_ANY_TERRAIN = self.Y_FEATURES + self.H_FEATURES + SMALL_MARGIN
		self.W_BONUSES_ON_ANY_TERRAIN = self.top.R_PEDIA_PAGE - self.X_BONUSES_ON_ANY_TERRAIN
		self.H_BONUSES_ON_ANY_TERRAIN = self.H_FEATURES

		# <!-- custom: new placeRelevantUnits and placeUnitsImpassable based on the new sevopedia terrain implementation -->
		self.X_RELEVANT_UNITS = self.X_BONUSES_ON_ANY_TERRAIN
		self.Y_RELEVANT_UNITS = self.Y_BONUSES_ON_ANY_TERRAIN + self.H_BONUSES_ON_ANY_TERRAIN + SMALL_MARGIN
		self.W_RELEVANT_UNITS = self.top.R_PEDIA_PAGE - self.X_BONUSES_ON_ANY_TERRAIN
		# <!-- custom: see code comment at self.H_MULTILIST_MULTIPLE_ROWS_BUTTON_SIZE in sevopedia unit for details -->
		self.H_RELEVANT_UNITS = self.H_FEATURES + MULTILIST_BUTTON_SIZE

		self.X_UNITS_IMPASSABLE = self.X_RELEVANT_UNITS
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

		self.I_FEATURE_ICE = getInfoTypeOrFail("FEATURE_ICE")
		self.I_FEATURE_FOREST = getInfoTypeOrFail("FEATURE_FOREST")
		self.I_FEATURE_JUNGLE = getInfoTypeOrFail("FEATURE_JUNGLE")

		self.I_PROMOTION_WOODSMAN1 = getInfoTypeOrFail("PROMOTION_WOODSMAN1")
		self.I_PROMOTION_WOODSMAN2 = getInfoTypeOrFail("PROMOTION_WOODSMAN2")
		self.I_PROMOTION_WOODSMAN3 = getInfoTypeOrFail("PROMOTION_WOODSMAN3")

	def interfaceScreen(self, iFeature):
		if self.iFeature != iFeature:
			self.bHistoryExpanded = False
		self.iFeature = iFeature

		self.placeInfo()
		self.placeBuildUnits()
		self.placeTerrain()
		self.placeImprovements()
		self.placeBonusesOnAnyTerrain()
		self.placeRelevantUnits()
		self.placeUnitsImpassable()
		self.placeSpecial()
		self.placeHistory()
		place_new_concept_legend_link(self.top, "CONCEPT_SAS_SEVOPEDIA_NUMTXT_LEGEND")

	def _getRemovalBuildsWithProduction(self):
		buildIds = []
		for iBuild in xrange(gc.getNumBuildInfos()):
			buildInfo = gc.getBuildInfo(iBuild)
			if buildInfo.getImprovement() != -1:
				continue
			if not buildInfo.isFeatureRemove(self.iFeature):
				continue
			if buildInfo.getFeatureProduction(self.iFeature) > 0:
				buildIds.append(iBuild)
		return buildIds

	def placeInfo(self):
		screen = self.top.getScreen()
		panel = self.top.getNextWidgetName()
		info = gc.getFeatureInfo(self.iFeature)

		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_INFO_PANE, self.Y_INFO_PANE, self.W_INFO_PANE, self.H_INFO_PANE, PanelStyles.PANEL_STYLE_BLUE50)

		# <!-- custom: was PanelStyles.PANEL_STYLE_MAIN -->
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_EMPTY)
		screen.addDDSGFC(self.top.getNextWidgetName(), info.getButton(), self.X_ICON + self.W_ICON / 2 - PANE_ICON_SIZE / 2, self.Y_ICON + self.H_ICON / 2 - PANE_ICON_SIZE / 2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1)

		screen.addListBoxGFC(panel, "", self.X_INFO_TEXT, self.Y_INFO_TEXT, self.W_INFO_TEXT, self.H_INFO_TEXT, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(panel, False)
		screen.appendListBoxString(panel, SASTextScale.titleText(info.getDescription()), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

		szText = ""
		screen.appendListBoxString(panel, SASTextScale.labelText(localText.getText("TXT_KEY_PEDIA_FEATURE", ())), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

		szText = u""
		for iYield in xrange(YieldTypes.NUM_YIELD_TYPES):
			iYieldChange = info.getYieldChange(iYield)
			if iYieldChange != 0:
				szSign = ""
				if iYieldChange > 0:
					szSign = "+"
				szText += (u"%s%d%c  " % (szSign, iYieldChange, gc.getYieldInfo(iYield).getChar()))

		screen.appendListBoxString(panel, SASTextScale.titleText(szText), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

	def placeBuildUnits(self):
		screen = self.top.getScreen()
		iActivePlayer = gc.getGame().getActivePlayer()

		removalBuilds = []
		improvementBuilds = []

		for iBuild in xrange(gc.getNumBuildInfos()):
			buildInfo = gc.getBuildInfo(iBuild)
			iImprovement = buildInfo.getImprovement()
			if iImprovement != -1:
				improvementInfo = gc.getImprovementInfo(iImprovement)
				if improvementInfo.isGoody():
					continue
				if improvementInfo.getFeatureMakesValid(self.iFeature):
					improvementBuilds.append(iBuild)
			elif buildInfo.isFeatureRemove(self.iFeature):
				removalBuilds.append(iBuild)

		if len(removalBuilds) == 0 and len(improvementBuilds) == 0:
			return

		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_FEATURE_BUILD_REMOVE", ()), "", False, True, self.X_BUILD_REMOVES, self.Y_BUILD_REMOVES, self.W_BUILD_REMOVES, self.H_BUILD_REMOVES, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		if len(removalBuilds) > 0:
			for iBuild in removalBuilds:
				buildInfo = gc.getBuildInfo(iBuild)
				# <!-- custom: use WIDGET_HELP_IMPROVEMENT for build hover text; click routing to Builds pedia is handled centrally in SevoPediaMain.handleInput. See KI#113. (GPT-5.3-Codex) -->
				screen.attachImageButton(panelName, "", buildInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_HELP_IMPROVEMENT, buildInfo.getTechPrereq(), iBuild, False)
		else:
			draw_none_text(screen, self.top, self.X_BUILD_REMOVES, self.Y_BUILD_REMOVES, self.W_BUILD_REMOVES, self.H_BUILD_REMOVES)

		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_FEATURE_PRODUCTION_PANEL", ()), "", False, True, self.X_FEATURE_PRODUCTION, self.Y_FEATURE_PRODUCTION, self.W_FEATURE_PRODUCTION, self.H_FEATURE_PRODUCTION, PanelStyles.PANEL_STYLE_BLUE50)
		techModifiers = get_feature_production_modifier_techs()
		removalBuildsWithProduction = self._getRemovalBuildsWithProduction()
		if len(techModifiers) > 0 and len(removalBuildsWithProduction) > 0:
			# <!-- custom: multilist instead of attachImageButton so each chop-production tech shows its +%% modifier as numTxt under the button, like the modifier buttons in Sevopedia Unit. (Claude code Opus 4.7) -->
			rowListName = self.top.getNextWidgetName()
			multiListX = self.X_FEATURE_PRODUCTION + MULTI_LIST_PANEL_OFFSET_X
			multiListY = self.Y_FEATURE_PRODUCTION + MULTI_LIST_PANEL_OFFSET_Y
			multiListW = self.W_FEATURE_PRODUCTION + MULTI_LIST_PANEL_ADDITIONAL_W
			multiListH = self.H_FEATURE_PRODUCTION + MULTI_LIST_PANEL_ADDITIONAL_H
			screen.addMultiListControlGFC(rowListName, "", multiListX, multiListY, multiListW, multiListH, SEVOPEDIA_MULTILIST_NUM_LISTS_AUTO_CALCULATE, MULTILIST_BUTTON_SIZE, MULTILIST_BUTTON_SIZE, TableStyles.TABLE_STYLE_STANDARD)
			iButtonIndex = 0
			maxButtonsPerRow = get_multilist_max_buttons_per_row(multiListW, MULTILIST_BUTTON_SIZE)
			for iTech, iModifier in techModifiers:
				screen.appendMultiListButton(rowListName, gc.getTechInfo(iTech).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, 1, False)
				numTxt = u"%+d%%" % iModifier
				extraCorrectionX = get_extra_correction_x(numTxt)
				add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)
				iButtonIndex += 1
		else:
			draw_none_text(screen, self.top, self.X_FEATURE_PRODUCTION, self.Y_FEATURE_PRODUCTION, self.W_FEATURE_PRODUCTION, self.H_FEATURE_PRODUCTION)

		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_UNITS_REMOVE", ()), "", False, True, self.X_UNITS_REMOVE, self.Y_UNITS_REMOVE, self.W_UNITS_REMOVE, self.H_UNITS_REMOVE, PanelStyles.PANEL_STYLE_BLUE50)
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
			if not bCanBuild:
				for iBuild in removalBuilds:
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
			draw_none_text(screen, self.top, self.X_UNITS_REMOVE, self.Y_UNITS_REMOVE, self.W_UNITS_REMOVE, self.H_UNITS_REMOVE)

	# <!-- custom: code provided with the help of claude ai and chatgpt too -->
	def placeSpecial(self):
		screen = self.top.getScreen()
		panel = self.top.getNextWidgetName()
		text = self.top.getNextWidgetName()
		info = gc.getFeatureInfo(self.iFeature)

		screen.addPanel(panel, localText.getText("TXT_KEY_PEDIA_SPECIAL_ABILITIES", ()), "", True, True, self.X_SPECIAL, self.Y_SPECIAL, self.W_SPECIAL, self.H_SPECIAL, PanelStyles.PANEL_STYLE_BLUE50)
		szSpecialText = info.getHelp() + CyGameTextMgr().getFeatureHelp(self.iFeature, True)
		baseLines = []
		if szSpecialText.strip():
			baseLines = szSpecialText.replace("\n\n", "\n").strip().split("\n")

		removalBuildIds = self._getRemovalBuildsWithProduction()
		removeProduction = 0
		if len(removalBuildIds) > 0:
			removeProduction = gc.getBuildInfo(removalBuildIds[0]).getFeatureProduction(self.iFeature)

		extraLines = []
		if removeProduction > 0:
			bullet = localText.getText("[ICON_BULLET]", ())
			extraLines.append(u"%s+%d%s %s" % (bullet, removeProduction, localText.getText("[ICON_PRODUCTION]", ()), localText.getText("TXT_KEY_PEDIA_BUILD_REMOVE_PRODUCTION", ())))

		szSpecialText = u"\n".join(baseLines + extraLines).strip()
		screen.addMultilineText(text, SASTextScale.labelText(szSpecialText), self.X_SPECIAL + 10, self.Y_SPECIAL + 30, self.W_SPECIAL - 20, self.H_SPECIAL - 40, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeTerrain(self):
		screen = self.top.getScreen()
		panel = self.top.getNextWidgetName()
		info = gc.getFeatureInfo(self.iFeature)

		screen.addPanel(panel, localText.getText("TXT_KEY_PEDIA_TERRAINS_MINUS_PLOT_TYPES", ()), "", False, True, self.X_FEATURES, self.Y_FEATURES, self.W_FEATURES, self.H_FEATURES, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panel, "", "  ")

		for iTerrain in xrange(gc.getNumTerrainInfos()):
			TerrainInfo = gc.getTerrainInfo(iTerrain)
			if info.isTerrain(iTerrain):
				screen.attachImageButton(panel, "", TerrainInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TERRAIN, iTerrain, 1, False)

	def placeImprovements(self):
		screen = self.top.getScreen()
		panel = self.top.getNextWidgetName()

		screen.addPanel(panel, localText.getText("TXT_KEY_PEDIA_IMPROVEMENTS_CUSTOM", ()), "", False, True, self.X_IMPROVEMENTS, self.Y_IMPROVEMENTS, self.W_IMPROVEMENTS, self.H_IMPROVEMENTS, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panel, "", "  ")

		for iImprovement in xrange(gc.getNumImprovementInfos()):
			ImprovementInfo = gc.getImprovementInfo(iImprovement)
			if ImprovementInfo.isGoody():
				continue
			elif ImprovementInfo.getFeatureMakesValid(self.iFeature):
				screen.attachImageButton(panel, "", ImprovementInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_IMPROVEMENT, iImprovement, 1, False)

	def placeBonusesOnAnyTerrain(self):
		screen = self.top.getScreen()
		panel = self.top.getNextWidgetName()

		screen.addPanel(panel, localText.getText("TXT_KEY_PEDIA_FEATURE_BONUSES_FEATURE_BOOLEANS", ()), "", False, True, self.X_BONUSES_ON_ANY_TERRAIN, self.Y_BONUSES_ON_ANY_TERRAIN, self.W_BONUSES_ON_ANY_TERRAIN, self.H_BONUSES_ON_ANY_TERRAIN, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panel, "", "  ")

		for iBonus in xrange(gc.getNumBonusInfos()):
			bonusInfo = gc.getBonusInfo(iBonus)
			if bonusInfo.isGraphicalOnly():
				continue
			if bonusInfo.isFeature(self.iFeature):
				screen.attachImageButton(panel, "", bonusInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus, 1, False)

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

		# <!-- custom: handle feature(s) that is (are) impassable such as of now only the ice cap feature quite similarly than the peak terrain was handled in sevopedia terrain's placeRelevantUnits, with some differences, see code comments at sevopedia feature's placeUnitsImpassable -->
		# <!-- custom: handle features with promotions or other specific effects separately, similar to how was done in sevopedia terrain previous new placeRelevantUnits and placeUnitsImpassable this new features code is based on -->
		iIce = self.I_FEATURE_ICE
		if self.iFeature == iIce:
			for iUnit in xrange(gc.getNumUnitInfos()):
				unitInfo = gc.getUnitInfo(iUnit)

				if unitInfo.isGraphicalOnly():
					continue

				# <!-- custom: note: ingame it seems that bCanMoveImpassable is not enough to move on ice cap (feature ice) that is only allowed on water terrains it seems, so not displaying all units here-->
				# inside: if self.iFeature == iIce:
				can_cross_ice = (unitInfo.isCanMoveAllTerrain() or (unitInfo.getDomainType() == DomainTypes.DOMAIN_SEA and (unitInfo.isCanMoveImpassable() or unitInfo.getFeaturePassableTech(iIce) != -1)))
				if can_cross_ice:
					screen.appendMultiListButton(rowListName, unitInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)

					iFeatureAttack = unitInfo.getFeatureAttackModifier(self.iFeature)
					iFeatureDefense = unitInfo.getFeatureDefenseModifier(self.iFeature)

					numTxt = get_numTxt_attack_defense_modifiers(iFeatureAttack, iFeatureDefense)
					extraCorrectionX = get_extra_correction_x(numTxt)
					add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

					#isButtonFound = True
					iButtonIndex += 1

		# <!-- custom: in advciv-sas as of now at least if not always or not, woodsman promotions apply both to jungle but also to forest tiles as well, seems more sensical to me, adjust below logic depending on which features have promotions applying to them or some other effects you'd want to display in the numTxt or not -->
		elif self.iFeature == self.I_FEATURE_FOREST or self.iFeature == self.I_FEATURE_JUNGLE:

			for iUnit in xrange(gc.getNumUnitInfos()):
				unitInfo = gc.getUnitInfo(iUnit)

				if unitInfo.isGraphicalOnly():
					continue

				iFeatureAttack = unitInfo.getFeatureAttackModifier(self.iFeature)
				iFeatureDefense = unitInfo.getFeatureDefenseModifier(self.iFeature)

				isHasW1 = unitInfo.getFreePromotions(self.I_PROMOTION_WOODSMAN1)
				isHasW2 = unitInfo.getFreePromotions(self.I_PROMOTION_WOODSMAN2)
				isHasW3 = unitInfo.getFreePromotions(self.I_PROMOTION_WOODSMAN3)

				if ((iFeatureAttack != 0) or (iFeatureDefense != 0) or isHasW1 or isHasW2 or isHasW3):
					screen.appendMultiListButton(rowListName, unitInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)

					if iFeatureAttack != 0 or iFeatureDefense != 0:
						numTxt = get_numTxt_attack_defense_modifiers(iFeatureAttack, iFeatureDefense)
					else:
						s = ""
						if isHasW1:
							s = "W1"
						if isHasW2:
							if s:
								s += "+2"
							else:
								s = "W2"
						if isHasW3:
							if s:
								s += "+3"
							else:
								s = "W3"
						if s:
							numTxt = s

					extraCorrectionX = get_extra_correction_x(numTxt)
					add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

					#isButtonFound = True
					iButtonIndex += 1

		else:
			for iUnit in xrange(gc.getNumUnitInfos()):
				unitInfo = gc.getUnitInfo(iUnit)

				if unitInfo.isGraphicalOnly():
					continue

				iFeatureAttack = unitInfo.getFeatureAttackModifier(self.iFeature)
				iFeatureDefense = unitInfo.getFeatureDefenseModifier(self.iFeature)

				if iFeatureAttack != 0 or iFeatureDefense != 0:
					screen.appendMultiListButton(rowListName, unitInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)

					numTxt = get_numTxt_attack_defense_modifiers(iFeatureAttack, iFeatureDefense)
					extraCorrectionX = get_extra_correction_x(numTxt)
					add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, numTxt, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)

					#isButtonFound = True
					iButtonIndex += 1

	def placeUnitsImpassable(self):
		xPanel = self.X_UNITS_IMPASSABLE
		yPanel = self.Y_UNITS_IMPASSABLE
		wPanel = self.W_UNITS_IMPASSABLE
		hPanel = self.H_UNITS_IMPASSABLE

		txtKeyPanel = "TXT_KEY_PEDIA_FEATURE_UNITS_IMPASSABLE"

		# <!-- custom: https://forums.civfanatics.com/threads/make-ice-workable.462809/?utm_source=chatgpt.com based on this provided to me by chatgpt thanks, it seems the ice feature (called as of now "ice cap" feature in advciv-sas is impassable but can be walked on if a unit has such a flag, so using a code similar to the terrain_peak handling in sevopedia terrain's as of now placeUnitsImpassable -->
		if self.iFeature == self.I_FEATURE_ICE:
			txtKeyPanel = "TXT_KEY_PEDIA_FEATURE_UNITS_IMPASSABLE_ICE"

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

		iIce = self.I_FEATURE_ICE
		if self.iFeature == iIce:
			for iUnit in xrange(gc.getNumUnitInfos()):
				unitInfo = gc.getUnitInfo(iUnit)

				if unitInfo.isGraphicalOnly():
					continue

				unitInfoDomain = unitInfo.getDomainType()

				# <!-- custom: allow any domain unlike for the peak code, so workers could walk on the water ice cap feature, unlike in peak where as of now naval units are not displayed as being able to walk on peak as it would make less sense too even though theoretically possible (boat with legs... xd..), so having permissive domain any allowed here unlike in peak code(but now that i think of it why not, such robotic things or mechanical walking things on a naval unit making it hybrid are probably not too far fetched maybe probably even exists or in sci-fi or dream if i may say or such other or not or other or etc); however still require unitInfo.isCanMoveAllTerrain(), as ice cap (ice feature) is only allowed on water terrains it seems, and bCanMoveImpassable is not enough for land units to allow them to walk there. So as of now we'd allow the gunship and airship to walk on ice cap, but not the scout or quechua warrior, even though they all have the bCanMoveImpassable, but only the airship and gunship among these also have canMoveAllTerrain as of now i'm not mistaken so only these can also walk on ice cap on top of walking on peak; note: code provided by chatgpt 5 thanks to my prompts and or such and which i adjusted or such, check if accurate-->
				# <!-- custom: below condition/code by chatgpt 5 which i formatted or refactored/adjusted a bit or not or yes or etc, check if accurate -->
				# Your Relevant Units panel (from earlier) already shows:
				# - land/air with All-Terrain (Gunship, Airship, Recon Drone), and
				# - sea units with a bypass (Move-Impassable or passable tech).
				# This Impassable panel now mirrors Peak-style behavior by listing every other unit that cannot enter Ice Cap, which should remove the "why can these 3 enter?" confusion.
				# Land/Air: blocked unless they have All-Terrain (your recon/gunship/airship will be allowed)
				land_or_air_blocked = (unitInfoDomain != DomainTypes.DOMAIN_SEA) and (not unitInfo.isCanMoveAllTerrain())

				# Sea: blocked if no bypass (or explicitly flagged impassable on ICE)
				sea_blocked = (unitInfoDomain == DomainTypes.DOMAIN_SEA) and (unitInfo.getFeatureImpassable(iIce) or not (unitInfo.isCanMoveImpassable() or unitInfo.getFeaturePassableTech(iIce) != -1 or unitInfo.isCanMoveAllTerrain()))

				if land_or_air_blocked or sea_blocked:
					screen.appendMultiListButton(rowListName, unitInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)

		else:
			info = gc.getFeatureInfo(self.iFeature)

			for iUnit in xrange(gc.getNumUnitInfos()):
				unitInfo = gc.getUnitInfo(iUnit)

				if unitInfo.isGraphicalOnly():
					continue

				# <!-- custom: below condition/code by chatgpt 5, check if accurate -->
				blocked = ((info.isImpassable() or unitInfo.getFeatureImpassable(self.iFeature)) and not unitInfo.isCanMoveImpassable() and unitInfo.getFeaturePassableTech(self.iFeature) == -1 and not unitInfo.isCanMoveAllTerrain())
				if blocked:
					screen.appendMultiListButton(rowListName, unitInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)

	def setHistoryExpanded(self, bExpanded):
		self.bHistoryExpanded = bExpanded

	def placeHistory(self):
		screen = self.top.getScreen()
		szText = gc.getFeatureInfo(self.iFeature).getCivilopedia()
		szTitle = localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ())
		draw_expandable_text_panel(screen, self.top, szTitle, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, szText, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)
	def handleInput (self, inputClass):
		return 0

