# Sid Meier's Civilization 4
# Copyright Firaxis Games 2005
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#

from CvPythonExtensions import *
from SASMagicNumbers import *
import CvUtil
# <!-- custom: Consume the surviving shared helper directly after retiring the overlapping SASUIResources dependency; avoid keeping a redundant visual-resource facade. (ChatGPT-5.6-Sol) -->
import SASTextScale
from SASUtils import getInfoTypeOrFail

from _sevopedia_helpers import *

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class SevoPediaBuild:

	def __init__(self, main):
		self.iBuild = -1
		self.bHistoryExpanded = False
		self.top = main
		self.SAS_iBuildRoad = getInfoTypeOrFail("BUILD_ROAD")
		self.SAS_iBuildRailroad = getInfoTypeOrFail("BUILD_RAILROAD")

		self.X_BUILD_PANE = self.top.X_PEDIA_PAGE
		self.Y_BUILD_PANE = self.top.Y_PEDIA_PAGE
		self.W_BUILD_PANE = 330
		self.H_BUILD_PANE = 127

		self.W_ICON = 100
		self.H_ICON = 100
		self.X_ICON = self.X_BUILD_PANE + 10
		self.Y_ICON = self.Y_BUILD_PANE + 10
		self.X_INFO_TEXT = self.X_BUILD_PANE + 110
		self.Y_INFO_TEXT = self.Y_ICON + 15
		self.W_INFO_TEXT = self.W_BUILD_PANE - 120
		self.H_INFO_TEXT = self.H_BUILD_PANE - 20

		self.X_REQUIRES = self.X_BUILD_PANE + self.W_BUILD_PANE + MEDIUM_MARGIN
		self.Y_REQUIRES = self.Y_BUILD_PANE
		self.W_REQUIRES = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.H_REQUIRES = self.H_BUILD_PANE

		self.X_FEATURE_PRODUCTION = self.X_REQUIRES + self.W_REQUIRES + MEDIUM_MARGIN
		self.Y_FEATURE_PRODUCTION = self.Y_BUILD_PANE
		self.W_FEATURE_PRODUCTION = get_panel_width_for_buttons(2, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.H_FEATURE_PRODUCTION = self.H_BUILD_PANE

		self.X_IMPROVEMENTS = self.X_FEATURE_PRODUCTION + self.W_FEATURE_PRODUCTION + MEDIUM_MARGIN
		self.Y_IMPROVEMENTS = self.Y_BUILD_PANE
		self.W_IMPROVEMENTS = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.H_IMPROVEMENTS = self.H_BUILD_PANE

		self.X_UNITS_BUILD = self.X_IMPROVEMENTS + self.W_IMPROVEMENTS + MEDIUM_MARGIN
		self.Y_UNITS_BUILD = self.Y_BUILD_PANE
		self.W_UNITS_BUILD = self.top.R_PEDIA_PAGE - self.X_UNITS_BUILD
		self.H_UNITS_BUILD = self.H_BUILD_PANE

		self.X_FEATURE_STRUCTS = self.X_BUILD_PANE
		self.Y_FEATURE_STRUCTS = self.Y_BUILD_PANE + self.H_BUILD_PANE + SMALL_MARGIN
		self.W_FEATURE_STRUCTS = self.top.R_PEDIA_PAGE - self.X_FEATURE_STRUCTS
		self.H_FEATURE_STRUCTS = 350

		self.X_SPECIAL = self.X_BUILD_PANE
		self.Y_SPECIAL = self.Y_FEATURE_STRUCTS + self.H_FEATURE_STRUCTS + SMALL_MARGIN
		self.W_SPECIAL = self.W_BUILD_PANE
		self.H_SPECIAL = self.top.B_PEDIA_PAGE - self.Y_SPECIAL

		self.X_HISTORY = self.X_SPECIAL + self.W_SPECIAL + MEDIUM_MARGIN
		self.Y_HISTORY = self.Y_SPECIAL
		self.W_HISTORY = self.top.R_PEDIA_PAGE - self.X_HISTORY
		self.H_HISTORY = self.H_SPECIAL
		self.FEATURE_STRUCT_BUTTON_BOTTOM_MARGIN = 20
		self.FEATURE_STRUCT_BUTTON_ROW_H = 64
		self.FEATURE_STRUCT_PANEL_HEADER_H = 28
		self.FEATURE_STRUCT_BUTTON_TOP_REL = self.H_FEATURE_STRUCTS - self.FEATURE_STRUCT_PANEL_HEADER_H - self.FEATURE_STRUCT_BUTTON_ROW_H - self.FEATURE_STRUCT_BUTTON_BOTTOM_MARGIN

	def _getBuildFeatureProductionModifierTechs(self):
		buildInfo = gc.getBuildInfo(self.iBuild)
		bHasRemovalProduction = False
		for iFeature in xrange(gc.getNumFeatureInfos()):
			if buildInfo.getFeatureProduction(iFeature) > 0:
				bHasRemovalProduction = True
				break
		if not bHasRemovalProduction:
			return []
		return get_feature_production_modifier_techs()

	def interfaceScreen(self, iBuild):
		if self.iBuild != iBuild:
			self.bHistoryExpanded = False
		self.iBuild = iBuild

		self.placeBuildPane()
		self.placeRequires()
		self.placeFeatureProduction()
		self.placeImprovements()
		self.placeUnitsBuild()
		self.placeFeatureStructs()
		self.placeSpecial()
		self.placeHistory()

	def placeBuildPane(self):
		screen = self.top.getScreen()
		buildInfo = gc.getBuildInfo(self.iBuild)
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_BUILD_PANE, self.Y_BUILD_PANE, self.W_BUILD_PANE, self.H_BUILD_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_EMPTY)
		screen.addDDSGFC(self.top.getNextWidgetName(), gc.getBuildInfo(self.iBuild).getButton(), self.X_ICON + self.W_ICON / 2 - PANE_ICON_SIZE / 2, self.Y_ICON + self.H_ICON / 2 - PANE_ICON_SIZE / 2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1)

		panel = self.top.getNextWidgetName()
		screen.addListBoxGFC(panel, "", self.X_INFO_TEXT, self.Y_INFO_TEXT, self.W_INFO_TEXT, self.H_INFO_TEXT, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(panel, False)
		screen.appendListBoxString(panel, SASTextScale.titleText(buildInfo.getDescription()), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)
		screen.appendListBoxString(panel, SASTextScale.labelText(localText.getText("TXT_KEY_PEDIA_BUILD", ())), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

	def placeRequires(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_REQUIRES", ()), "", False, True, self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		buildInfo = gc.getBuildInfo(self.iBuild)
		bFound = False

		iTech = buildInfo.getTechPrereq()
		if iTech > -1:
			screen.attachImageButton(panelName, "", gc.getTechInfo(iTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, 1, False)
			bFound = True

		if not bFound:
			draw_none_text(screen, self.top, self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES)

	def placeImprovements(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_RESULTS_SHORT", ()), "", False, True, self.X_IMPROVEMENTS, self.Y_IMPROVEMENTS, self.W_IMPROVEMENTS, self.H_IMPROVEMENTS, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		buildInfo = gc.getBuildInfo(self.iBuild)
		bFound = False

		iImprovement = buildInfo.getImprovement()
		if iImprovement > -1:
			screen.attachImageButton(panelName, "", gc.getImprovementInfo(iImprovement).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_IMPROVEMENT, iImprovement, 1, False)
			bFound = True

		iRoute = buildInfo.getRoute()
		if iRoute > -1:
			routeInfo = gc.getRouteInfo(iRoute)
			iBuild = -1
			if routeInfo.getType() == "ROUTE_ROAD":
				iBuild = self.SAS_iBuildRoad
			elif routeInfo.getType() == "ROUTE_RAILROAD":
				iBuild = self.SAS_iBuildRailroad
			if iBuild < 0:
				raise Exception("SevoPediaBuild: missing Build for route %s" % routeInfo.getType())
			# <!-- custom: use WIDGET_HELP_IMPROVEMENT for route-result build hover text; click redirect to Builds is handled in SevoPediaMain. See KI#113. (GPT-5.3-Codex) -->
			screen.attachImageButton(panelName, "", routeInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_HELP_IMPROVEMENT, gc.getBuildInfo(iBuild).getTechPrereq(), iBuild, False)
			bFound = True

		if not bFound:
			draw_none_text(screen, self.top, self.X_IMPROVEMENTS, self.Y_IMPROVEMENTS, self.W_IMPROVEMENTS, self.H_IMPROVEMENTS)

	def placeFeatureProduction(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_FEATURE_PRODUCTION_PANEL", ()), "", False, True, self.X_FEATURE_PRODUCTION, self.Y_FEATURE_PRODUCTION, self.W_FEATURE_PRODUCTION, self.H_FEATURE_PRODUCTION, PanelStyles.PANEL_STYLE_BLUE50)

		techModifiers = self._getBuildFeatureProductionModifierTechs()
		if len(techModifiers) <= 0:
			draw_none_text(screen, self.top, self.X_FEATURE_PRODUCTION, self.Y_FEATURE_PRODUCTION, self.W_FEATURE_PRODUCTION, self.H_FEATURE_PRODUCTION)
			return

		# <!-- custom: multilist instead of attachImageButton so each chop-production tech shows its +%% modifier as numTxt under the button, like Sevopedia Unit. (Claude code Opus 4.7) -->
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

	def placeUnitsBuild(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_UNITS_BUILD", ()), "", False, True, self.X_UNITS_BUILD, self.Y_UNITS_BUILD, self.W_UNITS_BUILD, self.H_UNITS_BUILD, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		iActivePlayer = gc.getGame().getActivePlayer()
		bUnitFound = False

		for iUnit in xrange(gc.getNumUnitInfos()):
			unitInfo = gc.getUnitInfo(iUnit)
			if unitInfo.isGraphicalOnly():
				continue
			if unitInfo.getBuilds(self.iBuild):
				szButton = unitInfo.getButton()
				if iActivePlayer >= 0:
					szButton = gc.getPlayer(iActivePlayer).getUnitButton(iUnit)
				screen.attachImageButton(panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)
				bUnitFound = True

		if not bUnitFound:
			draw_none_text(screen, self.top, self.X_UNITS_BUILD, self.Y_UNITS_BUILD, self.W_UNITS_BUILD, self.H_UNITS_BUILD)

	def placeFeatureStructs(self):
		screen = self.top.getScreen()
		buildInfo = gc.getBuildInfo(self.iBuild)
		featureStructs = []

		for iFeature in xrange(gc.getNumFeatureInfos()):
			featureInfo = gc.getFeatureInfo(iFeature)
			if featureInfo.isGraphicalOnly():
				continue

			featureTech = buildInfo.getFeatureTech(iFeature)
			featureTime = buildInfo.getFeatureTime(iFeature)
			featureProduction = buildInfo.getFeatureProduction(iFeature)
			bRemove = buildInfo.isFeatureRemove(iFeature)
			if bRemove or featureTech > -1 or featureTime > 0 or featureProduction > 0:
				featureStructs.append((iFeature, featureTech, featureTime, featureProduction, bRemove))

		if len(featureStructs) == 0:
			return

		panelCount = len(featureStructs)
		panelW = (self.W_FEATURE_STRUCTS - ((panelCount - 1) * MEDIUM_MARGIN)) / panelCount
		panelH = self.H_FEATURE_STRUCTS
		panelY = self.Y_FEATURE_STRUCTS
		textMarginX = 7
		textTop = panelY + 28
		buttonRowH = self.FEATURE_STRUCT_BUTTON_ROW_H
		buttonTopRel = self.FEATURE_STRUCT_BUTTON_TOP_REL

		bullet = localText.getText("[ICON_BULLET]", ())

		for i, (iFeature, featureTech, featureTime, featureProduction, bRemove) in enumerate(featureStructs):
			featureInfo = gc.getFeatureInfo(iFeature)
			panelX = self.X_FEATURE_STRUCTS + (i * (panelW + MEDIUM_MARGIN))
			panelTitle = u"%s: %s" % (localText.getText("TXT_KEY_PEDIA_BUILD_FEATURE_STRUCT_INFO", ()), featureInfo.getDescription())
			panelName = self.top.getNextWidgetName()
			screen.addPanel(panelName, panelTitle, "", False, True, panelX, panelY, panelW, panelH, PanelStyles.PANEL_STYLE_BLUE50)
			screen.attachLabel(panelName, "", "  ")

			lines = []
			lines.append(u"%s%s: %s" % (bullet, localText.getText("TXT_KEY_PEDIA_BUILD_FEATURE", ()), featureInfo.getDescription()))
			if featureTime > 0:
				lines.append(u"%s%s: %d" % (bullet, localText.getText("TXT_KEY_PEDIA_BUILD_REMOVE_TIME", ()), featureTime))
			if featureProduction > 0:
				lines.append(u"%s+%d%s %s" % (bullet, featureProduction, localText.getText("[ICON_PRODUCTION]", ()), localText.getText("TXT_KEY_PEDIA_BUILD_REMOVE_PRODUCTION", ())))
			if bRemove and featureTime == 0 and featureProduction == 0:
				lines.append(u"%s%s" % (bullet, localText.getText("TXT_KEY_PEDIA_BUILD_REMOVES_FEATURE_SIMPLE", ())))

			textName = self.top.getNextWidgetName()
			text = u"\n".join(lines)
			textH = (panelY + buttonTopRel) - textTop - 6
			if textH < 20:
				textH = 20
			screen.addMultilineText(textName, SASTextScale.labelText(text), panelX + textMarginX, textTop, panelW - (textMarginX * 2), textH, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			buttons = []
			if featureTech > -1:
				buttons.append(("tech", featureTech, gc.getTechInfo(featureTech).getButton()))
			buttons.append(("feature", iFeature, featureInfo.getButton()))

			if len(buttons) > 0:
				buttonSpacing = 4
				buttonRowW = (len(buttons) * buttonRowH) + ((len(buttons) - 1) * buttonSpacing)
				buttonXStart = (panelW - buttonRowW) / 2

				for iButtonIndex, (buttonType, buttonId, buttonArt) in enumerate(buttons):
					x = buttonXStart + (iButtonIndex * (buttonRowH + buttonSpacing))
					if buttonType == "tech":
						screen.setImageButtonAt(self.top.getNextWidgetName(), panelName, buttonArt, x, buttonTopRel, buttonRowH, buttonRowH, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, buttonId, 1)
					else:
						screen.setImageButtonAt(self.top.getNextWidgetName(), panelName, buttonArt, x, buttonTopRel, buttonRowH, buttonRowH, WidgetTypes.WIDGET_PEDIA_JUMP_TO_FEATURE, buttonId, 1)

	def placeSpecial(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_BUILD_INFO", ()), "", True, False, self.X_SPECIAL, self.Y_SPECIAL, self.W_SPECIAL, self.H_SPECIAL, PanelStyles.PANEL_STYLE_BLUE50)

		buildInfo = gc.getBuildInfo(self.iBuild)
		szSpecialText = u""

		bullet = localText.getText("[ICON_BULLET]", ())
		# <!-- custom: do not repeat the created Improvement here because the Result panel already shows it with a Pedia link.
		# Keep Route below because Sevopedia has no Route category/page to link to, so the text bullet still adds information. (GPT-5.5) -->

		iRoute = buildInfo.getRoute()
		if iRoute > -1:
			if szSpecialText.strip():
				szSpecialText += u"\n"
			szSpecialText += u"%s%s: %s" % (bullet, localText.getText("TXT_KEY_PEDIA_BUILD_ROUTE", ()), gc.getRouteInfo(iRoute).getDescription())

		iTime = buildInfo.getTime()
		if iTime > 0:
			if szSpecialText.strip():
				szSpecialText += u"\n"
			szSpecialText += u"%s%s: %d" % (bullet, localText.getText("TXT_KEY_PEDIA_BUILD_TIME", ()), iTime)

		iCost = buildInfo.getCost()
		if iCost > 0:
			if szSpecialText.strip():
				szSpecialText += u"\n"
			szSpecialText += u"%s%s: %d" % (bullet, localText.getText("TXT_KEY_PEDIA_BUILD_COST", ()), iCost)

		if buildInfo.isKill():
			if szSpecialText.strip():
				szSpecialText += u"\n"
			szSpecialText += u"%s%s" % (bullet, localText.getText("TXT_KEY_PEDIA_BUILD_KILL_WORKER", ()))

		listName = self.top.getNextWidgetName()
		screen.addMultilineText(listName, SASTextScale.labelText(szSpecialText), self.X_SPECIAL + 10, self.Y_SPECIAL + 30, self.W_SPECIAL - 20, self.H_SPECIAL - 40, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def setHistoryExpanded(self, bExpanded):
		self.bHistoryExpanded = bExpanded

	def placeHistory(self):
		screen = self.top.getScreen()
		szText = gc.getBuildInfo(self.iBuild).getCivilopedia()
		szTitle = localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ())
		draw_expandable_text_panel(screen, self.top, szTitle, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, szText, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)
	def handleInput(self, inputClass):
		return 0

