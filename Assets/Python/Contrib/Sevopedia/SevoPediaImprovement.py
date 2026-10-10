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
from SASUtils import getInfoTypeOrFail

from _sevopedia_helpers import *

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

IMPROVEMENT_LEADER_CACHE = None

def precomputeImprovementLeaderCache():
	global IMPROVEMENT_LEADER_CACHE

	if IMPROVEMENT_LEADER_CACHE is not None:
		return IMPROVEMENT_LEADER_CACHE

	leaderIds, leaderToCiv, unused_total = get_real_leader_maps_and_count(EXCLUDED_LEADER_TYPES_FROM_SEVOPEDIA)
	improvementData = {}

	for iImprovement in range(gc.getNumImprovementInfos()):
		weightToLeaders = {}
		for iLeader in leaderIds:
			leaderInfo = gc.getLeaderHeadInfo(iLeader)
			iWeight = leaderInfo.getImprovementWeightModifier(iImprovement)
			if iWeight != 0:
				if iWeight not in weightToLeaders:
					weightToLeaders[iWeight] = []
				weightToLeaders[iWeight].append(iLeader)

		if not weightToLeaders:
			improvementData[iImprovement] = (None, (), 0)
			continue

		weightsSorted = sorted(weightToLeaders.keys(), reverse=True)
		maxLeaders = 1
		for weight in weightsSorted:
			if len(weightToLeaders[weight]) > maxLeaders:
				maxLeaders = len(weightToLeaders[weight])
		improvementData[iImprovement] = (weightToLeaders, tuple(weightsSorted), maxLeaders)

	IMPROVEMENT_LEADER_CACHE = {
		"leaderIds": leaderIds,
		"leaderToCiv": leaderToCiv,
		"improvements": improvementData,
	}

	print("Sevopedia Improvement leader cache prebuilt. This should appear only once per gaming session.")
	return IMPROVEMENT_LEADER_CACHE

class SevoPediaImprovement:

	def __init__(self, main):
		self.iImprovement = -1
		self.bHistoryExpanded = False
		self.bContentExpanded = False
		self.top = main

		self.SAS_iBuildRoad = getInfoTypeOrFail("BUILD_ROAD")
		self.SAS_iBuildRailroad = getInfoTypeOrFail("BUILD_RAILROAD")
		self.I_CONCEPT_IRRIGATION = getInfoTypeOrFail("CONCEPT_IRRIGATION")
		self.I_TERRAIN_HILL = getInfoTypeOrFail("TERRAIN_HILL")

		self.X_IMPROVEMENT_PANE = self.top.X_PEDIA_PAGE
		self.Y_IMPROVEMENT_PANE = self.top.Y_PEDIA_PAGE
		self.H_REQUIRES = NON_MULTILIST_PANEL_STANDARD_HEIGHT
		self.H_IMPROVEMENT_PANE = (2 * self.H_REQUIRES) + SMALL_MARGIN
		self.W_IMPROVEMENT_PANE = self.H_IMPROVEMENT_PANE

		self.W_ICON = 150
		self.H_ICON = 150
		self.X_ICON = self.X_IMPROVEMENT_PANE + (self.H_IMPROVEMENT_PANE - self.H_ICON) / 2
		self.Y_ICON = self.Y_IMPROVEMENT_PANE + (self.H_IMPROVEMENT_PANE - self.H_ICON) / 2

		self.X_INFO_TEXT = self.X_IMPROVEMENT_PANE + 8
		self.Y_INFO_TEXT = self.Y_IMPROVEMENT_PANE + 8
		self.W_INFO_TEXT = self.W_IMPROVEMENT_PANE - 16
		self.H_INFO_TEXT = 60

		self.W_BONUS_YIELDS = 260
		self.W_MOST_YIELDS = self.W_BONUS_YIELDS

		self.X_BONUS_YIELDS = self.top.R_PEDIA_PAGE - self.W_BONUS_YIELDS
		self.Y_BONUS_YIELDS = self.Y_IMPROVEMENT_PANE
		self.H_BONUS_YIELDS = self.top.B_PEDIA_PAGE - self.Y_BONUS_YIELDS

		self.X_MOST_YIELDS = self.X_BONUS_YIELDS - self.W_MOST_YIELDS - MEDIUM_MARGIN
		self.Y_MOST_YIELDS = self.Y_BONUS_YIELDS
		self.H_MOST_YIELDS = self.H_BONUS_YIELDS

		leftAreaRight = self.X_MOST_YIELDS - MEDIUM_MARGIN
		self.W_LEFT_AREA = leftAreaRight - self.X_IMPROVEMENT_PANE

		self.W_BUILD_PANEL = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.H_BUILD_PANEL = self.H_REQUIRES
		self.W_REQUIRES = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)

		self.X_BUILD_PANEL = self.X_IMPROVEMENT_PANE + self.W_IMPROVEMENT_PANE + MEDIUM_MARGIN
		self.Y_BUILD_PANEL = self.Y_IMPROVEMENT_PANE

		self.X_REQUIRES = self.X_BUILD_PANEL
		self.Y_REQUIRES = self.Y_BUILD_PANEL + self.H_BUILD_PANEL + SMALL_MARGIN

		self.X_TERRAIN_MAKES_VALIDS = self.X_BUILD_PANEL + self.W_BUILD_PANEL + MEDIUM_MARGIN
		self.Y_TERRAIN_MAKES_VALIDS = self.Y_IMPROVEMENT_PANE
		self.H_TERRAIN_MAKES_VALIDS = self.H_REQUIRES

		self.X_FEATURE_MAKES_VALIDS = self.X_TERRAIN_MAKES_VALIDS
		self.Y_FEATURE_MAKES_VALIDS = self.Y_TERRAIN_MAKES_VALIDS + self.H_TERRAIN_MAKES_VALIDS + SMALL_MARGIN
		self.H_FEATURE_MAKES_VALIDS = self.H_REQUIRES

		self.H_TOP_RIGHT_STACK = max(self.H_TERRAIN_MAKES_VALIDS + SMALL_MARGIN + self.H_FEATURE_MAKES_VALIDS, self.H_BUILD_PANEL + SMALL_MARGIN + self.H_REQUIRES)
		availableAfterTerrain = leftAreaRight - self.X_TERRAIN_MAKES_VALIDS
		if availableAfterTerrain < 0:
			availableAfterTerrain = 0

		self.W_TERRAIN_MAKES_VALIDS = get_panel_width_for_buttons(5, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.W_FEATURE_MAKES_VALIDS = self.W_TERRAIN_MAKES_VALIDS
		self.X_IMPROVEMENT_ANIMATION = self.X_TERRAIN_MAKES_VALIDS + self.W_TERRAIN_MAKES_VALIDS + MEDIUM_MARGIN
		self.Y_IMPROVEMENT_ANIMATION = self.Y_IMPROVEMENT_PANE
		self.W_IMPROVEMENT_ANIMATION = leftAreaRight - self.X_IMPROVEMENT_ANIMATION
		if self.W_IMPROVEMENT_ANIMATION < 120:
			self.W_IMPROVEMENT_ANIMATION = 120
			maxTerrainValidsW = leftAreaRight - MEDIUM_MARGIN - self.X_TERRAIN_MAKES_VALIDS - self.W_IMPROVEMENT_ANIMATION
			if maxTerrainValidsW < 120:
				maxTerrainValidsW = 120
			self.W_TERRAIN_MAKES_VALIDS = min(self.W_TERRAIN_MAKES_VALIDS, maxTerrainValidsW)
			self.W_FEATURE_MAKES_VALIDS = self.W_TERRAIN_MAKES_VALIDS
			self.X_IMPROVEMENT_ANIMATION = self.X_TERRAIN_MAKES_VALIDS + self.W_TERRAIN_MAKES_VALIDS + MEDIUM_MARGIN
			self.W_IMPROVEMENT_ANIMATION = leftAreaRight - self.X_IMPROVEMENT_ANIMATION
		self.H_IMPROVEMENT_ANIMATION = self.W_IMPROVEMENT_ANIMATION

		self.H_TOP_ROW = max(self.H_IMPROVEMENT_PANE, self.H_TOP_RIGHT_STACK, self.H_IMPROVEMENT_ANIMATION)

		self.X_IMPROVEMENT_LEADERS = self.X_IMPROVEMENT_PANE
		self.Y_IMPROVEMENT_LEADERS = self.Y_IMPROVEMENT_PANE + self.H_TOP_ROW + SMALL_MARGIN
		self.W_IMPROVEMENT_LEADERS = leftAreaRight - self.X_IMPROVEMENT_PANE

		self.H_SPECIAL = 289
		self.H_HISTORY = 289
		self.Y_SPECIAL = self.top.B_PEDIA_PAGE - self.H_SPECIAL
		self.X_SPECIAL = self.X_IMPROVEMENT_PANE
		self.W_SPECIAL = min(360, (self.W_IMPROVEMENT_LEADERS / 3) + 100)
		self.X_HISTORY = self.X_SPECIAL + self.W_SPECIAL + MEDIUM_MARGIN
		self.Y_HISTORY = self.Y_SPECIAL
		self.W_HISTORY = leftAreaRight - self.X_HISTORY
		self.H_HISTORY = self.H_SPECIAL

		self.H_IMPROVEMENT_LEADERS = self.Y_SPECIAL - SMALL_MARGIN - self.Y_IMPROVEMENT_LEADERS
		if self.H_IMPROVEMENT_LEADERS < 0:
			self.H_IMPROVEMENT_LEADERS = 0

		self.SCALE_ANIMATION = 0.7

		# <!-- custom: Leader icon sizes now use centralized INCHART_* constants from _sevopedia_helpers.
		# IMPROVEMENT_LEADER_ICON_SIZE, IMPROVEMENT_LEADER_BUTTON_SPACING, IMPROVEMENT_LEADER_ROW_H replaced by INCHART_ICON_SIZE, INCHART_ICON_SPACING, INCHART_ROW_HEIGHT -->

	def interfaceScreen(self, iImprovement):
		if self.iImprovement != iImprovement:
			self.bHistoryExpanded = False
			self.bContentExpanded = False
		self.iImprovement = iImprovement

		if self.bContentExpanded:
			self.placeImprovementAnimation()
			return

		self.placeImprovementPane()
		self.placeSpecial()
		self.placeBonusYields()
		if not self.bHistoryExpanded:
			self.placeImprovementAnimation()
		self.placeBuilds()
		self.placeRequires()
		self.placeMostYields()
		self.placeImprovementLeaderTable()
		self.placeTerrainMakesValids()
		self.placeFeatureMakesValids()
		self.placeHistory()

	def placeImprovementPane(self):
		screen = self.top.getScreen()
		info = gc.getImprovementInfo(self.iImprovement)
		screen.addPanel( self.top.getNextWidgetName(), "", "", False, False, self.X_IMPROVEMENT_PANE, self.Y_IMPROVEMENT_PANE, self.W_IMPROVEMENT_PANE, self.H_IMPROVEMENT_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		# <!-- custom: was PanelStyles.PANEL_STYLE_MAIN -->
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_EMPTY)
		screen.addDDSGFC(self.top.getNextWidgetName(), info.getButton(), self.X_ICON + self.W_ICON/2 - PANE_ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - PANE_ICON_SIZE/2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		panel = self.top.getNextWidgetName()
		screen.addListBoxGFC(panel, "", self.X_INFO_TEXT, self.Y_INFO_TEXT, self.W_INFO_TEXT, self.H_INFO_TEXT, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(panel, False)
		screen.appendListBoxString(panel, SASTextScale.titleText(info.getDescription()), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_CENTER_JUSTIFY)
		screen.appendListBoxString(panel, SASTextScale.labelText(localText.getText("TXT_KEY_PEDIA_IMPROVEMENT", ())), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_CENTER_JUSTIFY)

		# <!-- custom: move base yield info from yields panel to the improvement_pane panel, prettier or/adn clearer/more accurate as well maybe (a bit like in sevopedia terrain and sevopedia feature) -->
		# <!-- custom: line removed that seemed safe to do see diff with earlier code for comparison if needed -->
		# <!-- custom: this part is for yields that do not require any additional tech than the one required to gain access to the ressources (for example + 2 hammer with mine, + 4 commerce with town)
		s = u""
		nCount = 0

		for k in range(YieldTypes.NUM_YIELD_TYPES):
			iYieldChange = info.getYieldChange(k)

			if iYieldChange != 0:
				if iYieldChange > 0:
					sign = "+"
				else:
					sign = ""
				s += (u"%s%i%c " % (sign, iYieldChange, gc.getYieldInfo(k).getChar()))
				nCount += 1

		if nCount > 0:
			szYield = u"<font=4>%s</font>" % s

			xCenteringAdjust = 0
			for i in range(nCount):
				xCenteringAdjust -= 23

			xCenteringPositioning = ((self.W_IMPROVEMENT_PANE-10) / 2) - 4
			yBottomPositioning = self.H_IMPROVEMENT_PANE - 44

			screen.addMultilineText(self.top.getNextWidgetName(), szYield, self.X_IMPROVEMENT_PANE + xCenteringPositioning + xCenteringAdjust +5, self.Y_IMPROVEMENT_PANE - 13 + yBottomPositioning, self.W_IMPROVEMENT_PANE-10, self.H_IMPROVEMENT_PANE-10, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeSpecial(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		# <!-- custom: prettier and clearer without the/a header, also gives a bit extra room in case we have many effects to place, and matches sevopedia terrain and feature display as well or more closely -->
		#screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_EFFECTS", ()), "", True, False, self.X_SPECIAL, self.Y_SPECIAL, self.W_SPECIAL, self.H_SPECIAL, PanelStyles.PANEL_STYLE_BLUE50 )
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_IMPROVEMENT_INFO", ()), "", True, False, self.X_SPECIAL, self.Y_SPECIAL, self.W_SPECIAL, self.H_SPECIAL, PanelStyles.PANEL_STYLE_BLUE50 )

		listName = self.top.getNextWidgetName()
		szSpecialText = CyGameTextMgr().getImprovementHelp(self.iImprovement, True)

		szSpecialText = szSpecialText.replace("\n\n", "\n").strip()

		screen.addMultilineText(listName, SASTextScale.labelText(szSpecialText), self.X_SPECIAL + 10, self.Y_SPECIAL + 30, self.W_SPECIAL - 20, self.H_SPECIAL - 40, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeBonusYields(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		# <!-- custom: prettier and clearer without the/a header, also gives a bit extra room in case we have many effects to place, and matches sevopedia terrain and feature display as well or more closely -->
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_SAS_BONUSS_YIELDS", ()), "", True, True, self.X_BONUS_YIELDS, self.Y_BONUS_YIELDS, self.W_BONUS_YIELDS, self.H_BONUS_YIELDS, PanelStyles.PANEL_STYLE_BLUE50 )
		info = gc.getImprovementInfo(self.iImprovement)
		for j in range(gc.getNumBonusInfos()):
			bFirst = True
			szYield = u""
			bEffect = False
			for k in range(YieldTypes.NUM_YIELD_TYPES):
				iYieldChange = info.getImprovementBonusYield(j, k)
				if iYieldChange != 0:
					bEffect = True
					# Uncomment for 3.13 behavior. Note that Uranium shows incorrect hammer yield (should be +2)
					#iYieldChange += info.getYieldChange(k)
					if bFirst:
						bFirst = False
					else:
						szYield += u", "
					if iYieldChange > 0:
						sign = u"+"
					else:
						sign = u""
					szYield += (u"%s%i%c" % (sign, iYieldChange, gc.getYieldInfo(k).getChar()))
			if bEffect:
				childPanelName = self.top.getNextWidgetName()
				screen.attachPanel(panelName, childPanelName, "", "", False, False, PanelStyles.PANEL_STYLE_EMPTY)
				screen.attachLabel(childPanelName, "", "  ")
				screen.attachImageButton( childPanelName, "", gc.getBonusInfo(j).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, j, 1, False )
				screen.attachLabel(childPanelName, "", SASTextScale.titleText(szYield))

	def placeImprovementAnimation(self):
		screen = self.top.getScreen()
		iAnimX, iAnimY, iAnimW, iAnimH = draw_expandable_content_panel_container(screen, self.top, u"", self.X_IMPROVEMENT_ANIMATION, self.Y_IMPROVEMENT_ANIMATION, self.W_IMPROVEMENT_ANIMATION, self.H_IMPROVEMENT_ANIMATION, self.bContentExpanded, SAS_MAGIC_PEDIA_PYTHON_CONTENT_EXPAND, SAS_MAGIC_PEDIA_PYTHON_CONTENT_RELOAD)
		screen.addImprovementGraphicGFC(self.top.getNextWidgetName(), self.iImprovement, iAnimX, iAnimY, iAnimW, iAnimH, WidgetTypes.WIDGET_GENERAL, -1, -1, X_ROTATION_ANIMATION, Z_ROTATION_ANIMATION, self.SCALE_ANIMATION, True)

	def placeBuilds(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_BUILD", ()), "", False, True, self.X_BUILD_PANEL, self.Y_BUILD_PANEL, self.W_BUILD_PANEL, self.H_BUILD_PANEL, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		bButtonFound = False
		for iBuild in range(gc.getNumBuildInfos()):
			if gc.getBuildInfo(iBuild).getImprovement() == self.iImprovement:
				buildInfo = gc.getBuildInfo(iBuild)
				# <!-- custom: use WIDGET_HELP_IMPROVEMENT for build buttons so hover text is available; click redirect is handled in SevoPediaMain. See KI#113. (GPT-5.3-Codex) -->
				screen.attachImageButton(panelName, "", buildInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_HELP_IMPROVEMENT, buildInfo.getTechPrereq(), iBuild, False)
				bButtonFound = True

		if not bButtonFound:
			draw_none_text(screen, self.top, self.X_BUILD_PANEL, self.Y_BUILD_PANEL, self.W_BUILD_PANEL, self.H_BUILD_PANEL)

	def placeRequires(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_REQUIRES", ()), "", False, True, self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES, PanelStyles.PANEL_STYLE_BLUE50 )
		screen.attachLabel(panelName, "", "  ")
		bButtonFound = False
		for iBuild in range(gc.getNumBuildInfos()):
			if gc.getBuildInfo(iBuild).getImprovement() == self.iImprovement:
				iTech = gc.getBuildInfo(iBuild).getTechPrereq()
				if iTech > -1:
					screen.attachImageButton( panelName, "", gc.getTechInfo(iTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, 1, False )
					bButtonFound = True

		if not bButtonFound:
			draw_none_text(screen, self.top, self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES)

	# <!-- custom: use the bonus-yields row template for consistency with placeBonusYields above. (Claude code Opus 4.7) -->
	def placeMostYields(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SEVOPEDIA_IMPROVEMENT_MOST_TILE_YIELD_CHANGES", ()), "", True, True, self.X_MOST_YIELDS, self.Y_MOST_YIELDS, self.W_MOST_YIELDS, self.H_MOST_YIELDS, PanelStyles.PANEL_STYLE_BLUE50)

		Info = gc.getImprovementInfo(self.iImprovement)

		# Irrigated yield changes
		sText = ""
		for k in xrange(YieldTypes.NUM_YIELD_TYPES):
			iYieldChange = Info.getIrrigatedYieldChange(k)
			if iYieldChange != 0:
				sText += u"%+d%c" % (iYieldChange, gc.getYieldInfo(k).getChar())
		if len(sText) > 0:
			attach_button_label_row(screen, self.top, panelName, ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_IRRIGATION").getPath(), WidgetTypes.WIDGET_PEDIA_DESCRIPTION, CivilopediaPageTypes.CIVILOPEDIA_PAGE_CONCEPT, self.I_CONCEPT_IRRIGATION, sText)

		# Hills yield changes
		sText = ""
		for k in xrange(YieldTypes.NUM_YIELD_TYPES):
			iYieldChange = Info.getHillsYieldChange(k)
			if iYieldChange != 0:
				sText += u"%+d%c" % (iYieldChange, gc.getYieldInfo(k).getChar())
		if len(sText) > 0:
			iHill = self.I_TERRAIN_HILL
			attach_button_label_row(screen, self.top, panelName, gc.getTerrainInfo(iHill).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_TERRAIN, iHill, 1, sText)

		# River yield changes
		sText = ""
		for k in xrange(YieldTypes.NUM_YIELD_TYPES):
			iYieldChange = Info.getRiverSideYieldChange(k)
			if iYieldChange != 0:
				sText += u"%+d%c" % (iYieldChange, gc.getYieldInfo(k).getChar())
		if len(sText) > 0:
			# <!-- custom: unlike in m-e mod or so it seems, we don't use WidgetTypes.WIDGET_PYTHON (in this placeMostYields method) and id 6783 neither, so go for a simpler implementation, that matches how we redirect using concepts in sevopedia unit as of now to the concept_cities for example, we now have a new concept_rivers to redirect to now in advciv-sas, use that rather; done with the help of gemini ai thanks -->
			# This button now links to the Civilopedia concept for Rivers.
			# Its tooltip and click behavior are handled by the built-in Civ4 widget system.
			# Ensure 'CONCEPT_RIVERS' is defined in your Civilopedia XML.
			riversConceptID = get_concept_id("CONCEPT_RIVERS")
			widgetType, widgetID1, widgetID2 = get_concept_widgetType_widgetID1_widgetID2(riversConceptID, WidgetTypes, CivilopediaPageTypes)
			attach_button_label_row(screen, self.top, panelName, ArtFileMgr.getInterfaceArtInfo("WORLDBUILDER_RIVER_PLACEMENT").getPath(), widgetType, widgetID1, widgetID2, sText)

		# Tech yield changes
		for item in xrange(gc.getNumTechInfos()):
			sText = ""
			for k in xrange(YieldTypes.NUM_YIELD_TYPES):
				iYieldChange = Info.getTechYieldChanges(item, k)
				if iYieldChange != 0:
					sText += u"%+d%c" % (iYieldChange, gc.getYieldInfo(k).getChar())
			if len(sText):
				attach_button_label_row(screen, self.top, panelName, gc.getTechInfo(item).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, item, 1, sText)

		# Civic yield changes
		for item in xrange(gc.getNumCivicInfos()):
			sText = ""
			for k in xrange(YieldTypes.NUM_YIELD_TYPES):
				iYieldChange = gc.getCivicInfo(item).getImprovementYieldChanges(self.iImprovement, k)
				if iYieldChange != 0:
					sText += u"%+d%c" % (iYieldChange, gc.getYieldInfo(k).getChar())
			if len(sText):
				attach_button_label_row(screen, self.top, panelName, gc.getCivicInfo(item).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, item, 1, sText)

		# Route yield changes
		for item in xrange(gc.getNumRouteInfos()):
			sText = ""
			for k in xrange(YieldTypes.NUM_YIELD_TYPES):
				iYieldChange = Info.getRouteYieldChanges(item, k)
				if iYieldChange != 0:
					sText += u"%+d%c" % (iYieldChange, gc.getYieldInfo(k).getChar())
			if len(sText):
				# <!-- custom: link route yield changes to the real Build entries (road/railroad) now that Builds are a category.
				# This removes the old concept route hack and keeps behavior consistent with other Build links. Credit: Claude Opus 4.5 + GPT-5.2-Codex. (GPT-5.2-Codex (summarized)) -->
				routeInfo = gc.getRouteInfo(item)
				iBuild = -1
				if routeInfo.getType() == "ROUTE_ROAD":
					iBuild = self.SAS_iBuildRoad
				elif routeInfo.getType() == "ROUTE_RAILROAD":
					iBuild = self.SAS_iBuildRailroad
				if iBuild < 0:
					raise Exception("SevoPediaImprovement: missing Build for route %s" % routeInfo.getType())

				attach_button_label_row(screen, self.top, panelName, routeInfo.getButton(), WidgetTypes.WIDGET_HELP_IMPROVEMENT, gc.getBuildInfo(iBuild).getTechPrereq(), iBuild, sText)

	def placeImprovementLeaderTable(self):
		screen = self.top.getScreen()
		xPanel = self.X_IMPROVEMENT_LEADERS
		yPanel = self.Y_IMPROVEMENT_LEADERS
		wPanel = self.W_IMPROVEMENT_LEADERS
		hPanel = self.H_IMPROVEMENT_LEADERS

		screen.addPanel(self.top.getNextWidgetName(), localText.getText("TXT_KEY_PEDIA_SAS_IMPROVEMENT_FAVORED_BY_LEADERS", ()), "", True, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50)
		innerPanelX = xPanel + INCHART_TABLE_MARGIN
		innerPanelY = yPanel + 28
		innerPanelW = wPanel - (2 * INCHART_TABLE_MARGIN)
		innerPanelH = hPanel - 34
		screen.addPanel(self.top.getNextWidgetName(), "", "", True, True, innerPanelX, innerPanelY, innerPanelW, innerPanelH, PanelStyles.PANEL_STYLE_BLUE50)

		cache = IMPROVEMENT_LEADER_CACHE
		if cache is None:
			cache = precomputeImprovementLeaderCache()

		leaderToCiv = cache["leaderToCiv"]
		weightToLeaders, weightsSorted, maxLeaders = cache["improvements"].get(self.iImprovement, (None, (), 0))

		if not weightToLeaders:
			draw_none_text(screen, self.top, innerPanelX, innerPanelY, innerPanelW, innerPanelH)
			return

		tableMargin = INCHART_TABLE_MARGIN
		tableX = innerPanelX + tableMargin
		tableY = innerPanelY + tableMargin
		tableW = innerPanelW - (2 * tableMargin)
		tableH = innerPanelH - (2 * tableMargin)

		weightColW = 60
		countColW = 60
		fixedColsW = weightColW + countColW
		leaderColW, maxLeaders = inchart_calc_icon_col_width(tableW, fixedColsW, maxLeaders)

		tableName = self.top.getNextWidgetName()
		screen.addTableControlGFC(tableName, 2 + maxLeaders, tableX, tableY, tableW, tableH, True, False, INCHART_ROW_HEIGHT, INCHART_ROW_HEIGHT, CHART_TABLE_STYLE)
		screen.enableSort(tableName)

		screen.setTableColumnHeader(tableName, 0, localText.getText("TXT_KEY_PEDIA_SAS_WEIGHT", ()), weightColW)
		screen.setTableColumnHeader(tableName, 1, localText.getText("TXT_KEY_PEDIA_SAS_COUNT", ()), countColW)
		inchart_set_icon_column_headers(screen, tableName, 2, maxLeaders, leaderColW)

		for weight in weightsSorted:
			iRow = screen.appendTableRow(tableName)
			screen.setTableText(tableName, 0, iRow, SASTextScale.labelText(u"%+d" % weight), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
			leaderCount = len(weightToLeaders[weight])
			screen.setTableText(tableName, 1, iRow, SASTextScale.labelText(u"%d" % leaderCount), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
			inchart_set_icon_cells(screen, tableName, iRow, weightToLeaders[weight], 2, maxLeaders, INCHART_ICON_TYPE_LEADER, {"leaderToCiv": leaderToCiv})

	# <!-- custom: _setLeaderIconCells removed - now uses centralized inchart_set_icon_cells from _sevopedia_helpers -->

	# <!-- custom: new addition thanks to chatgpt; as for logic this is how it works-functions based on chatgpt's explanation as well as my own research/findings in (translate to english using web browser or such) https://gforestshade.github.io/kujira/post/civ4improvementinfos/#terrainmakesvalids: if some terrains are specified then the improvement is only allowed on these terrains, else improvement is allowed on all terrains; not sure i got it all right (in particular in the case of irrigation or such conditions seemingly allowing the improvement on a terrain even if not listed here), so i am not sure it is all accurate but maybe is, check to be sure, and adjust this if needed; i implemented it as such and also added an explicative text that maybe the restriction could be elsewhere if not in improvementinfos. -->
	def placeTerrainMakesValids(self):
		xPanel = self.X_TERRAIN_MAKES_VALIDS
		yPanel = self.Y_TERRAIN_MAKES_VALIDS
		wPanel = self.W_TERRAIN_MAKES_VALIDS
		hPanel = self.H_TERRAIN_MAKES_VALIDS

		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_TERRAIN_MAKES_VALIDS", ()), "", False, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50 )
		screen.attachLabel(panelName, "", "  ")

		isButtonFound = False

		for iTerrain in xrange(gc.getNumTerrainInfos()):
			if gc.getImprovementInfo(self.iImprovement).getTerrainMakesValid(iTerrain):
				isButtonFound = True
				screen.attachImageButton(panelName, "", gc.getTerrainInfo(iTerrain).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TERRAIN, iTerrain, 1, False)

		if not isButtonFound:
			txtKeyNoButtonFound = "TXT_KEY_PEDIA_TERRAIN_MAKES_VALIDS_NO_RESTRICTION"
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel)

	def placeFeatureMakesValids(self):
		xPanel = self.X_FEATURE_MAKES_VALIDS
		yPanel = self.Y_FEATURE_MAKES_VALIDS
		wPanel = self.W_FEATURE_MAKES_VALIDS
		hPanel = self.H_FEATURE_MAKES_VALIDS

		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_FEATURE_MAKES_VALIDS", ()), "", False, True, xPanel, yPanel, wPanel, hPanel, PanelStyles.PANEL_STYLE_BLUE50 )
		screen.attachLabel(panelName, "", "  ")

		isButtonFound = False

		for iFeature in xrange(gc.getNumFeatureInfos()):
			if gc.getImprovementInfo(self.iImprovement).getFeatureMakesValid(iFeature):
				isButtonFound = True
				screen.attachImageButton(panelName, "", gc.getFeatureInfo(iFeature).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_FEATURE, iFeature, 1, False)

		if not isButtonFound:
			txtKeyNoButtonFound = "TXT_KEY_PEDIA_FEATURE_MAKES_VALIDS_NO_RESTRICTION"
			draw_none_text(screen, self.top, xPanel, yPanel, wPanel, hPanel)

	def setHistoryExpanded(self, bExpanded):
		self.bHistoryExpanded = bExpanded

	def setContentExpanded(self, bExpanded):
		self.bContentExpanded = bExpanded

	# <!-- custom: addition based on our existing mod's code in some other class, as for pedia entries, imported from m-e mod (see main readme for mod abbreviation details in as of now credits section) (but i also found them later in c2c mod and they are seemingly the same but the file is sadly/unfortunately way too bloated so going for the m-e mod one(s if talking about the assets themselves in thinking/saying so)) -->
	def placeHistory(self):
		screen = self.top.getScreen()
		szText = gc.getImprovementInfo(self.iImprovement).getCivilopedia()
		szTitle = localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ())
		draw_expandable_text_panel(screen, self.top, szTitle, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, szText, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)
	def handleInput (self, inputClass):
		return 0

