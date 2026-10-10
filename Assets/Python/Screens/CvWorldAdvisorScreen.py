## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from CvPythonExtensions import *
import CvUtil
from SASUtils import *
from SASFontUtils import *
import SASTextScale

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class CvWorldAdvisorScreen:
	"World Advisor"

	def __init__(self, screenId):
		self.screenId = screenId
		self.SCREEN_NAME = "WorldAdvisorScreen"
		self.WIDGET_ID = "WorldAdvisorWidget"

		self.Z_BACKGROUND = -6.1
		self.Z_CONTROLS = self.Z_BACKGROUND - 0.2
		self.DZ = -0.2
		self.Z_HELP_AREA = self.Z_CONTROLS - 1

		self.W_LEFT_SPACE_FOR_COMMERCE_SLIDERS = SAS_ADVISOR_LEFT_SPACE_FOR_COMMERCE_SLIDERS
		self.W_RIGHT_SPACE_FOR_SCOREBOARD = SAS_ADVISOR_RIGHT_SPACE_FOR_SCOREBOARD
		self.H_TOP_SPACE_FOR_TECH_BAR = SAS_ADVISOR_TOP_SPACE_FOR_TECH_BAR
		self.H_BOTTOM_SPACE = SAS_ADVISOR_BOTTOM_SPACE

		self.W_HELP_AREA = 200
		self.PANEL_HEIGHT = 55
		self.X_MARGIN = 45

		# Layout constants ordered to match the tab order (BFC 1 / BFC 2 / Territory / Environment); BFC_* are shared by BFC 1 / BFC 2 / Territory.
		self.BFC_TABLE_GAP = 18
		self.BFC_CITY_COL_WIDTH = 170
		self.BFC_ZOOM_COL_WIDTH = 28
		self.BFC_ICON_SIZE = 24

		self.ENV_INNER_MARGIN = 24
		self.ENV_COLUMN_GAP = 20
		self.ENV_TOP_PANEL_HEIGHT = 100
		self.ENV_Y_SPACING = 30
		self.ENV_TEXT_MARGIN = 15

		# <!-- custom: Environment tab is last because its global-warming readouts are less important. (Claude code Opus 4.7) -->
		self.PAGE_NAME_LIST = ["TXT_KEY_WORLD_ADVISOR_BFC1_TAB", "TXT_KEY_WORLD_ADVISOR_BFC2_TAB", "TXT_KEY_WORLD_ADVISOR_TERRITORY_TAB", "TXT_KEY_ECONOMICS_ADVISOR_ENVIRONMENT_TAB"]
		self.PAGE_LINK_WIDTH = []
		self.iBFC1ID = 0
		self.iBFC2ID = 1
		self.iTerritoryID = 2
		self.iEnvironmentID = 3
		self.iActiveTab = self.iBFC1ID
		self.DEBUG_DROPDOWN_ID = "WorldAdvisorDropdownWidget"

		self.iLanguageLoaded = -1
		self.nWidgetCount = 0
		self.iActivePlayer = -1
		self.pActivePlayer = None
		self.iActiveTeam = -1
		self.pActiveTeam = None
		# <!-- custom: cached global warming defines. (Claude code Sonnet 4.6) -->
		self.iGLOBAL_WARMING_RESTORATION_RATE = None
		self.iGLOBAL_WARMING_PROB = None

	def reset(self):
		self.iActivePlayer = getAdvisorValidPerspectivePlayer(self.iActivePlayer, bAllowVassalPerspective=True)
		if self.iActivePlayer == -1:
			return
		self.pActivePlayer = gc.getPlayer(self.iActivePlayer)
		self.iActiveTeam = self.pActivePlayer.getTeam()
		self.pActiveTeam = gc.getTeam(self.iActiveTeam)

	def getMapObserverTeam(self):
		# <!-- custom: Vassal perspective changes the World Advisor's subject, but the active human remains the map observer.
		# Using the vassal team for fog and resource knowledge exposed plots and bonuses the human had not discovered; debug player selection retains its reveal bypass and selected-team resource behavior. See KI#224. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if CyGame().isDebugMode():
			return self.iActiveTeam
		return gc.getPlayer(CyGame().getActivePlayer()).getTeam()

	def getScreen(self):
		return CyGInterfaceScreen(self.SCREEN_NAME, self.screenId)

	def updateRuntimeLayout(self, screen):
		self.X_SCREEN, self.Y_SCREEN, self.W_SCREEN, self.H_SCREEN = getAdvisorRuntimeBounds(screen, self.W_LEFT_SPACE_FOR_COMMERCE_SLIDERS, self.W_RIGHT_SPACE_FOR_SCOREBOARD, self.H_TOP_SPACE_FOR_TECH_BAR, self.H_BOTTOM_SPACE)
		self.X_TITLE, self.X_EXIT, self.Y_EXIT, self.Y_LINK, _ = getAdvisorRuntimeAnchors(self.W_SCREEN, self.H_SCREEN)
		self.Y_TITLE = SAS_ADVISOR_TITLE_Y

		self.CONTENT_Y_TOP = self.PANEL_HEIGHT
		self.CONTENT_Y_BOTTOM = self.H_SCREEN - self.PANEL_HEIGHT

		self.BFC_X_TABLE = self.X_MARGIN
		self.BFC_W_TABLE = self.W_SCREEN - 2 * self.X_MARGIN
		self.BFC_Y_PLOT_TABLE = self.CONTENT_Y_TOP + 18
		self.BFC_H_PLOT_TABLE = (self.CONTENT_Y_BOTTOM - self.BFC_Y_PLOT_TABLE - self.BFC_TABLE_GAP - 18) / 2
		self.BFC_Y_TERRAIN_TABLE = self.BFC_Y_PLOT_TABLE + self.BFC_H_PLOT_TABLE + self.BFC_TABLE_GAP
		self.BFC_H_TERRAIN_TABLE = self.CONTENT_Y_BOTTOM - self.BFC_Y_TERRAIN_TABLE - 18

		self.ENV_X_LEFT_PANEL = self.X_MARGIN
		self.ENV_PANE_WIDTH = (self.W_SCREEN - (2 * self.X_MARGIN) - (2 * self.ENV_COLUMN_GAP)) / 3
		self.ENV_X_MIDDLE_PANEL = self.ENV_X_LEFT_PANEL + self.ENV_PANE_WIDTH + self.ENV_COLUMN_GAP
		self.ENV_X_RIGHT_PANEL = self.ENV_X_MIDDLE_PANEL + self.ENV_PANE_WIDTH + self.ENV_COLUMN_GAP
		self.ENV_Y_TOP_PANEL = self.CONTENT_Y_TOP + self.ENV_INNER_MARGIN
		self.ENV_H_TOP_PANEL = self.ENV_TOP_PANEL_HEIGHT
		self.ENV_Y_LOCATION = self.ENV_Y_TOP_PANEL + self.ENV_H_TOP_PANEL + self.ENV_INNER_MARGIN
		self.ENV_PANE_HEIGHT = self.CONTENT_Y_BOTTOM - self.ENV_Y_LOCATION - self.ENV_INNER_MARGIN

	def initText(self):
		if not CyGame().isFinalInitialized():
			return

		aszPageLabels = []
		for szPageName in self.PAGE_NAME_LIST:
			aszPageLabels.append(localText.getText(szPageName, ()).upper())
		szExitLabel = localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT", ()).upper()
		if len(aszPageLabels) > 1:
			self.PAGE_LINK_WIDTH[:] = getAdvisorRuntimeLinkWidths(CyInterface(), aszPageLabels, szExitLabel, self.X_EXIT)
		else:
			self.PAGE_LINK_WIDTH[:] = []
			for szPageLabel in aszPageLabels:
				self.PAGE_LINK_WIDTH.append(CyInterface().determineWidth(szPageLabel) + 30)

		if self.iLanguageLoaded == CyGame().getCurrentLanguage():
			return
		self.iLanguageLoaded = CyGame().getCurrentLanguage()

		self.SCREEN_TITLE = sasFontTagTitle.bold + localText.getText("TXT_KEY_WORLD_ADVISOR_TITLE", ()).upper() + SAS_FONT_TAG_CLOSE
		self.EXIT_TEXT = sasFontTagTitle + localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT", ()).upper() + SAS_FONT_TAG_CLOSE
		self.ART_MAINMENU_SLIDESHOW_LOAD = ArtFileMgr.getInterfaceArtInfo("MAINMENU_SLIDESHOW_LOAD").getPath()
		self.ART_POPUPS_BACKGROUND_TRANSPARENT = ArtFileMgr.getInterfaceArtInfo("POPUPS_BACKGROUND_TRANSPARENT").getPath()
		self.ART_CITY_SELECTION_BUTTON = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTONS_CITYSELECTION").getPath()
		self.iTerrainPeak = getInfoTypeOrFail("TERRAIN_PEAK")
		self.iTerrainHill = getInfoTypeOrFail("TERRAIN_HILL")
		self.BUTTON_TERRAIN_PEAK = gc.getTerrainInfo(self.iTerrainPeak).getButton()
		self.BUTTON_TERRAIN_HILL = gc.getTerrainInfo(self.iTerrainHill).getButton()
		# <!-- custom: River has no terrain type (it's a plot-edge property), so reuse the WorldBuilder river-placement art.
		# Lake intentionally stays text-only because BTS represents lakes as coast-type water plots and TERRAIN_COAST's icon is already used for Coast, making the Lake column misleading.
		# Coastal Land has no canonical art so it stays text-only like Land/Water/Flat. (Claude code Opus 4.7; summarized GPT-5.5) -->
		self.BUTTON_RIVER = ArtFileMgr.getInterfaceArtInfo("WORLDBUILDER_RIVER_PLACEMENT").getPath()

		# Shared (used across multiple tabs)
		self.TEXT_TOTAL = localText.getText("TXT_KEY_TOTAL", ())
		self.TEXT_CITY = localText.getText("TXT_KEY_WORLD_ADVISOR_CITY", ())
		self.TEXT_BFC = localText.getText("TXT_KEY_WORLD_ADVISOR_BFC_SHORT", ())
		self.TEXT_WATER = localText.getText("TXT_KEY_WORLD_ADVISOR_BFC_WATER", ())
		self.TEXT_PEAK = localText.getText("TXT_KEY_WORLD_ADVISOR_BFC_PEAK", ())
		self.TEXT_HILL = localText.getText("TXT_KEY_WORLD_ADVISOR_BFC_HILL", ())
		self.TEXT_FLAT = localText.getText("TXT_KEY_WORLD_ADVISOR_BFC_FLAT", ())
		self.TEXT_RIVER = localText.getText("TXT_KEY_WORLD_ADVISOR_BFC_RIVER", ())
		self.TEXT_LAKE = localText.getText("TXT_KEY_WORLD_ADVISOR_BFC_LAKE", ())
		self.TEXT_COASTAL = localText.getText("TXT_KEY_WORLD_ADVISOR_BFC_COASTAL", ())
		self.COLOR_GREEN = getInfoTypeOrFail("COLOR_GREEN")
		self.COLOR_YELLOW = getInfoTypeOrFail("COLOR_YELLOW")
		self.COLOR_RED = getInfoTypeOrFail("COLOR_RED")

		# BFC 1
		self.TEXT_POP = localText.getText("TXT_KEY_WORLD_ADVISOR_BFC1_POP", ())
		self.TEXT_YEAR = localText.getText("TXT_KEY_WORLD_ADVISOR_BFC1_YEAR", ())

		# Territory
		self.TEXT_TERRITORY_PLOTS = localText.getText("TXT_KEY_WORLD_ADVISOR_TERRITORY_PLOTS", ())
		self.TEXT_TERRITORY_TERRAIN = localText.getText("TXT_KEY_WORLD_ADVISOR_TERRITORY_TERRAIN", ())
		self.TEXT_TERRITORY_FEATURES = localText.getText("TXT_KEY_WORLD_ADVISOR_TERRITORY_FEATURES", ())
		self.TEXT_TERRITORY_BONUSES = localText.getText("TXT_KEY_WORLD_ADVISOR_TERRITORY_BONUSES", ())
		self.TEXT_TERRITORY_IMPROVEMENTS = localText.getText("TXT_KEY_CONCEPT_IMPROVEMENTS", ())
		self.TEXT_TERRITORY_SUBURBS = localText.getText("TXT_KEY_WORLD_ADVISOR_TERRITORY_SUBURBS_ABBR", ())
		self.TEXT_TERRITORY_TOTAL_ABBR = localText.getText("TXT_KEY_WORLD_ADVISOR_TERRITORY_TOTAL_ABBR", ())

		# Environment
		self.TEXT_GW_SEVERITY_RATING = localText.getText("TXT_KEY_GW_SEVERITY_RATING", ()).upper()
		self.TEXT_ENV_POLLUTION = localText.getText("TXT_KEY_POLLUTION", ())
		self.TEXT_ENV_TOTAL = localText.getText("TXT_KEY_TOTAL", ())
		self.TEXT_ENV_CHANCES = localText.getText("TXT_KEY_CHANCES", ())
		self.TEXT_ENV_POPULATION = localText.getText("TXT_KEY_DEMO_SCREEN_POPULATION_TEXT", ())
		self.TEXT_ENV_BUILDINGS = localText.getText("TXT_KEY_CONCEPT_BUILDINGS", ())
		self.TEXT_ENV_RESOURCES = localText.getText("TXT_KEY_CONCEPT_RESOURCES", ())
		self.TEXT_ENV_ELECTRICITY = localText.getText("TXT_KEY_TECH_ELECTRICITY", ())
		self.TEXT_ENV_TOTAL_OFFSETS = localText.getText("TXT_KEY_ENVIRONMENT_TOTAL_OFFSETS", ())
		self.TEXT_ENV_TOTAL_IMPACT = localText.getText("TXT_KEY_ENVIRONMENT_TOTAL_IMPACT", ())
		self.TEXT_ENV_LOCAL_THRESHOLD = localText.getText("TXT_KEY_ENVIRONMENT_LOCAL_THRESHOLD", ())
		self.TEXT_ENV_TOTAL_POLLUTION = localText.getText("TXT_KEY_ENVIRONMENT_TOTAL_POLLUTION", ())
		self.TEXT_ENV_THRESHOLD = localText.getText("TXT_KEY_ENVIRONMENT_THRESHOLD", ())
		self.TEXT_ENV_GW_CHANGE_RATE = localText.getText("TXT_KEY_ENVIRONMENT_GW_CHANGE_RATE", ())
		self.TEXT_ENV_RELATIVE_CONTRIBUTION = localText.getText("TXT_KEY_ENVIRONMENT_RELATIVE_CONTRIBUTION", ())
		self.TEXT_ENV_GW_INDEX = localText.getText("TXT_KEY_ENVIRONMENT_GW_INDEX", ())
		self.TEXT_ENV_EXPECTED_EVENTS = localText.getText("TXT_KEY_ENVIRONMENT_EXPECTED_EVENTS", ())
		self.TEXT_ENV_EVENT_TALLY = localText.getText("TXT_KEY_ENVIRONMENT_EVENT_TALLY", ())
		self.TEXT_ENV_LOCAL_ANGER_LEVEL = localText.getText("TXT_KEY_ENVIRONMENT_LOCAL_ANGER_LEVEL", ())
		self.LABEL_ENV_DOMESTIC = sasFontTagLabel + localText.getText("TXT_KEY_ENVIRONMENT_DOMESTIC", ()).upper() + SAS_FONT_TAG_CLOSE
		self.LABEL_ENV_GLOBAL = sasFontTagLabel + localText.getText("TXT_KEY_ENVIRONMENT_GLOBAL", ()).upper() + SAS_FONT_TAG_CLOSE
		self.LABEL_ENV_EFFECTS = sasFontTagLabel + localText.getText("TXT_KEY_ENVIRONMENT_EFFECTS", ()).upper() + SAS_FONT_TAG_CLOSE

	def showScreen(self):
		screen = self.getScreen()
		if screen.isActive():
			return
		self.updateRuntimeLayout(screen)
		self.initText()

		screen.setRenderInterfaceOnly(True)
		screen.showScreen(PopupStates.POPUPSTATE_IMMEDIATE, False)

		self.reset()
		self.deleteAllWidgets()

		screen.addDDSGFC("WorldAdvisorBackground", self.ART_MAINMENU_SLIDESHOW_LOAD, 0, 0, self.W_SCREEN, self.H_SCREEN, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.addPanel("WorldAdvisorTopPanel", u"", u"", True, False, 0, 0, self.W_SCREEN, self.PANEL_HEIGHT, PanelStyles.PANEL_STYLE_TOPBAR)
		screen.addPanel("WorldAdvisorBottomPanel", u"", u"", True, False, 0, self.H_SCREEN - self.PANEL_HEIGHT, self.W_SCREEN, self.PANEL_HEIGHT, PanelStyles.PANEL_STYLE_BOTTOMBAR)
		screen.showWindowBackground(False)
		screen.setDimensions(self.X_SCREEN, self.Y_SCREEN, self.W_SCREEN, self.H_SCREEN)

		self.szExitButtonName = self.getNextWidgetName()
		screen.setText(self.szExitButtonName, "Background", self.EXIT_TEXT, CvUtil.FONT_RIGHT_JUSTIFY, self.X_EXIT, self.Y_EXIT, self.Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		self.szHeaderWidget = self.getNextWidgetName()
		screen.setLabel(self.szHeaderWidget, "Background", self.SCREEN_TITLE, CvUtil.FONT_CENTER_JUSTIFY, self.X_TITLE, self.Y_TITLE, self.Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		screen.setHelpTextArea(self.W_HELP_AREA, FontTypes.SMALL_FONT, self.X_SCREEN, self.Y_SCREEN, self.Z_HELP_AREA, 1, self.ART_POPUPS_BACKGROUND_TRANSPARENT, True, True, CvUtil.FONT_LEFT_JUSTIFY, 0)

		addAdvisorDebugDropdown(screen, self.DEBUG_DROPDOWN_ID, self.iActivePlayer, bAllowVassalPerspective=True)

		self.iNumPermanentWidgets = self.nWidgetCount
		if self.iActiveTab < 0 or self.iActiveTab >= len(self.PAGE_NAME_LIST):
			self.iActiveTab = self.iBFC1ID
		self.redrawContents()

	def redrawContents(self):
		screen = self.getScreen()
		self.deleteAllWidgets(self.iNumPermanentWidgets)

		aszTabLabels = []
		aszTabWidgetIDs = []
		for iPage in range(len(self.PAGE_NAME_LIST)):
			aszTabLabels.append(localText.getText(self.PAGE_NAME_LIST[iPage], ()).upper())
			aszTabWidgetIDs.append("WorldAdvisorTabButton" + str(iPage))
		drawAdvisorFooterTabs(screen, aszTabWidgetIDs, aszTabLabels, self.PAGE_LINK_WIDTH, self.iActiveTab, self.Y_LINK, 0, self.COLOR_YELLOW, iXStart=self.X_MARGIN)

		if self.iActiveTab == self.iBFC1ID:
			self.drawBFC1Tab()
		elif self.iActiveTab == self.iBFC2ID:
			self.drawBFC2Tab()
		elif self.iActiveTab == self.iTerritoryID:
			self.drawTerritoryTab()
		elif self.iActiveTab == self.iEnvironmentID:
			self.drawEnvironmentTab()

	# BFC 1
	def drawBFC1Tab(self):
		aszTableRows, aiPlotTotals, aaiTerrainTotals, aaiFeatureTotals = self.collectBFC1Data()
		aiTerrainColumns = self.getTerrainColumnsWithoutPlotShapes()
		aiFeatureColumns = range(gc.getNumFeatureInfos())

		self.drawBFC1PlotTable(aszTableRows, aiPlotTotals)
		self.drawBFC1TerrainFeatureTable(aszTableRows, aaiTerrainTotals, aaiFeatureTotals, aiTerrainColumns, aiFeatureColumns)

	def collectBFC1Data(self):
		player = gc.getPlayer(self.iActivePlayer)
		bDebug = CyGame().isDebugMode()
		iObserverTeam = self.getMapObserverTeam()
		aszRows = []
		# Plot count slots: 0=Total 1=Water 2=Peak 3=Hill 4=Flat 5=River 6=Lake 7=Coastal
		aiPlotTotals = [0, 0, 0, 0, 0, 0, 0, 0]
		aaiTerrainTotals = []
		aaiFeatureTotals = []

		(pCity, iter) = player.firstCity(False)
		while pCity and not pCity.isNone():
			aiPlotCounts = [0, 0, 0, 0, 0, 0, 0, 0]
			aiTerrainCounts = [0] * gc.getNumTerrainInfos()
			aiFeatureCounts = [0] * gc.getNumFeatureInfos()

			for iPlot in range(gc.getNUM_CITY_PLOTS()):
				pPlot = pCity.getCityIndexPlot(iPlot)
				if pPlot and not pPlot.isNone() and (bDebug or pPlot.isRevealed(iObserverTeam, False)):
					aiPlotCounts[0] += 1
					if pPlot.isWater():
						aiPlotCounts[1] += 1
						if pPlot.isLake():
							aiPlotCounts[6] += 1
					else:
						if pPlot.isPeak():
							aiPlotCounts[2] += 1
						elif pPlot.isHills():
							aiPlotCounts[3] += 1
						else:
							aiPlotCounts[4] += 1
						if pPlot.isCoastalLand():
							aiPlotCounts[7] += 1
					if pPlot.isRiverSide():
						aiPlotCounts[5] += 1

					iTerrain = pPlot.getTerrainType()
					if iTerrain >= 0:
						aiTerrainCounts[iTerrain] += 1
					iFeature = pPlot.getFeatureType()
					if iFeature >= 0:
						aiFeatureCounts[iFeature] += 1

			iFoundYear = CyGame().getTurnYear(pCity.getGameTurnFounded())
			aszRows.append([pCity.getName(), pCity.getOwner(), pCity.getID(), pCity.getPopulation(), iFoundYear, aiPlotCounts, aiTerrainCounts, aiFeatureCounts])
			for i in range(len(aiPlotTotals)):
				aiPlotTotals[i] += aiPlotCounts[i]
			aaiTerrainTotals.append(aiTerrainCounts)
			aaiFeatureTotals.append(aiFeatureCounts)
			(pCity, iter) = player.nextCity(iter, False)

		return aszRows, aiPlotTotals, aaiTerrainTotals, aaiFeatureTotals

	def drawBFC1PlotTable(self, aszRows, aiPlotTotals):
		screen = self.getScreen()
		szTable = self.getNextWidgetName()
		iNumCols = 12
		screen.addTableControlGFC(szTable, iNumCols, self.BFC_X_TABLE, self.BFC_Y_PLOT_TABLE, self.BFC_W_TABLE, self.BFC_H_PLOT_TABLE, True, True, self.BFC_ICON_SIZE, self.BFC_ICON_SIZE, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSort(szTable)
		iCityW = self.BFC_CITY_COL_WIDTH
		iDataW = (self.BFC_W_TABLE - self.BFC_ZOOM_COL_WIDTH - iCityW) / (iNumCols - 2)
		screen.setTableColumnHeader(szTable, 0, "", self.BFC_ZOOM_COL_WIDTH)
		screen.setTableColumnHeader(szTable, 1, sasFontTagLabel + self.TEXT_CITY + SAS_FONT_TAG_CLOSE, iCityW)
		screen.setTableColumnHeader(szTable, 2, sasFontTagLabel + self.TEXT_POP + SAS_FONT_TAG_CLOSE, iDataW)
		screen.setTableColumnHeader(szTable, 3, sasFontTagLabel + self.TEXT_YEAR + SAS_FONT_TAG_CLOSE, iDataW)
		screen.setTableColumnHeader(szTable, 4, sasFontTagLabel + self.TEXT_BFC + SAS_FONT_TAG_CLOSE, iDataW)
		screen.setTableColumnHeader(szTable, 5, sasFontTagLabel + self.TEXT_WATER + SAS_FONT_TAG_CLOSE, iDataW)
		# <!-- custom: Plot-type columns have room for icon + text; Peak and Hill buttons are visually similar, so labels avoid ambiguity. (GPT-5.5) -->
		screen.setTableColumnHeader(szTable, 6, SASTextScale.labelImageText(self.BUTTON_TERRAIN_PEAK, self.BFC_ICON_SIZE, self.TEXT_PEAK), iDataW)
		screen.setTableColumnHeader(szTable, 7, SASTextScale.labelImageText(self.BUTTON_TERRAIN_HILL, self.BFC_ICON_SIZE, self.TEXT_HILL), iDataW)
		screen.setTableColumnHeader(szTable, 8, sasFontTagLabel + self.TEXT_FLAT + SAS_FONT_TAG_CLOSE, iDataW)
		screen.setTableColumnHeader(szTable, 9, SASTextScale.labelImageText(self.BUTTON_RIVER, self.BFC_ICON_SIZE, self.TEXT_RIVER), iDataW)
		screen.setTableColumnHeader(szTable, 10, sasFontTagLabel + self.TEXT_LAKE + SAS_FONT_TAG_CLOSE, iDataW)
		screen.setTableColumnHeader(szTable, 11, sasFontTagLabel + self.TEXT_COASTAL + SAS_FONT_TAG_CLOSE, iDataW)

		for iRow in range(len(aszRows)):
			self.appendBFC1PlotRow(screen, szTable, iRow, aszRows[iRow][0], aszRows[iRow][1], aszRows[iRow][2], aszRows[iRow][3], aszRows[iRow][4], aszRows[iRow][5])
		if len(aszRows) > 0:
			iTotalPopulation = 0
			for rowData in aszRows:
				iTotalPopulation += rowData[3]
			self.appendBFC1PlotRow(screen, szTable, len(aszRows), self.TEXT_TOTAL, -1, -1, iTotalPopulation, 0, aiPlotTotals)

	def appendBFC1PlotRow(self, screen, szTable, iRow, szCityName, iOwner, iCityID, iPopulation, iFoundYear, aiPlotCounts):
		screen.appendTableRow(szTable)
		if iOwner >= 0:
			screen.setTableText(szTable, 0, iRow, "", self.ART_CITY_SELECTION_BUTTON, WidgetTypes.WIDGET_ZOOM_CITY, iOwner, iCityID, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, 1, iRow, sasFontTagLabel + szCityName + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		else:
			screen.setTableText(szTable, 0, iRow, u"", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, 1, iRow, sasFontTagLabel + szCityName + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.setTableInt(szTable, 2, iRow, sasFontTagLabel + str(iPopulation) + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
		if iOwner >= 0:
			SASTextScale.setTableIntLabel(screen, szTable, 3, iRow, iFoundYear, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
		else:
			screen.setTableText(szTable, 3, iRow, u"", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
		for iCol in range(len(aiPlotCounts)):
			screen.setTableInt(szTable, iCol + 4, iRow, sasFontTagLabel + str(aiPlotCounts[iCol]) + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def drawBFC1TerrainFeatureTable(self, aszRows, aaiTerrainTotals, aaiFeatureTotals, aiTerrainColumns, aiFeatureColumns):
		screen = self.getScreen()
		szTable = self.getNextWidgetName()
		iNumDataCols = len(aiTerrainColumns) + len(aiFeatureColumns)
		iNumCols = 2 + iNumDataCols
		screen.addTableControlGFC(szTable, iNumCols, self.BFC_X_TABLE, self.BFC_Y_TERRAIN_TABLE, self.BFC_W_TABLE, self.BFC_H_TERRAIN_TABLE, True, True, self.BFC_ICON_SIZE, self.BFC_ICON_SIZE, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSort(szTable)
		iDataW = 0
		if iNumDataCols > 0:
			iDataW = (self.BFC_W_TABLE - self.BFC_ZOOM_COL_WIDTH - self.BFC_CITY_COL_WIDTH) / iNumDataCols
		screen.setTableColumnHeader(szTable, 0, "", self.BFC_ZOOM_COL_WIDTH)
		screen.setTableColumnHeader(szTable, 1, sasFontTagLabel + self.TEXT_CITY + SAS_FONT_TAG_CLOSE, self.BFC_CITY_COL_WIDTH)
		iCol = 2
		for iTerrain in aiTerrainColumns:
			screen.setTableColumnHeader(szTable, iCol, SASTextScale.labelImageText(gc.getTerrainInfo(iTerrain).getButton(), self.BFC_ICON_SIZE), iDataW)
			iCol += 1
		for iFeature in aiFeatureColumns:
			screen.setTableColumnHeader(szTable, iCol, SASTextScale.labelImageText(gc.getFeatureInfo(iFeature).getButton(), self.BFC_ICON_SIZE), iDataW)
			iCol += 1

		for iRow in range(len(aszRows)):
			self.appendBFC1TerrainFeatureRow(screen, szTable, iRow, aszRows[iRow], aiTerrainColumns, aiFeatureColumns)
		if len(aszRows) > 0:
			aiTerrainTotals = [0] * gc.getNumTerrainInfos()
			aiFeatureTotals = [0] * gc.getNumFeatureInfos()
			for iRow in range(len(aszRows)):
				for iTerrain in range(gc.getNumTerrainInfos()):
					aiTerrainTotals[iTerrain] += aaiTerrainTotals[iRow][iTerrain]
				for iFeature in range(gc.getNumFeatureInfos()):
					aiFeatureTotals[iFeature] += aaiFeatureTotals[iRow][iFeature]
			self.appendBFC1TerrainFeatureRow(screen, szTable, len(aszRows), [self.TEXT_TOTAL, -1, -1, 0, 0, [], aiTerrainTotals, aiFeatureTotals], aiTerrainColumns, aiFeatureColumns)

	def appendBFC1TerrainFeatureRow(self, screen, szTable, iRow, rowData, aiTerrainColumns, aiFeatureColumns):
		screen.appendTableRow(szTable)
		szCityName = rowData[0]
		iOwner = rowData[1]
		iCityID = rowData[2]
		if iOwner >= 0:
			screen.setTableText(szTable, 0, iRow, "", self.ART_CITY_SELECTION_BUTTON, WidgetTypes.WIDGET_ZOOM_CITY, iOwner, iCityID, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, 1, iRow, sasFontTagLabel + szCityName + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		else:
			screen.setTableText(szTable, 0, iRow, u"", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, 1, iRow, sasFontTagLabel + szCityName + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		iCol = 2
		for iTerrain in aiTerrainColumns:
			self.setBFCCountCell(screen, szTable, iCol, iRow, rowData[6][iTerrain])
			iCol += 1
		for iFeature in aiFeatureColumns:
			self.setBFCCountCell(screen, szTable, iCol, iRow, rowData[7][iFeature])
			iCol += 1

	def drawBFC2Tab(self):
		aiBonusColumns = []
		for iBonus in range(gc.getNumBonusInfos()):
			if gc.getBonusInfo(iBonus).getPlacementOrder() >= 0:
				aiBonusColumns.append(iBonus)
		aszRows, aaiBonusCounts, aaiImprovementCounts, aaiRouteCounts = self.collectBFC2Data()
		self.drawBFC2IconTable(aszRows, [(aiBonusColumns, gc.getBonusInfo, aaiBonusCounts)], self.BFC_Y_PLOT_TABLE, self.BFC_H_PLOT_TABLE)
		self.drawBFC2IconTable(aszRows, [(range(gc.getNumImprovementInfos()), gc.getImprovementInfo, aaiImprovementCounts), (range(gc.getNumRouteInfos()), gc.getRouteInfo, aaiRouteCounts)], self.BFC_Y_TERRAIN_TABLE, self.BFC_H_TERRAIN_TABLE)

	def collectBFC2Data(self):
		player = gc.getPlayer(self.iActivePlayer)
		bDebug = CyGame().isDebugMode()
		iObserverTeam = self.getMapObserverTeam()
		aszRows = []
		aaiBonusCounts = []
		aaiImprovementCounts = []
		aaiRouteCounts = []
		(pCity, iter) = player.firstCity(False)
		while pCity and not pCity.isNone():
			aiBonusCounts = [0] * gc.getNumBonusInfos()
			aiImprovementCounts = [0] * gc.getNumImprovementInfos()
			aiRouteCounts = [0] * gc.getNumRouteInfos()
			for iPlot in range(gc.getNUM_CITY_PLOTS()):
				pPlot = pCity.getCityIndexPlot(iPlot)
				if pPlot and not pPlot.isNone() and (bDebug or pPlot.isRevealed(iObserverTeam, False)):
					iBonus = pPlot.getBonusType(iObserverTeam)
					if iBonus >= 0:
						aiBonusCounts[iBonus] += 1
					iImprovement = pPlot.getRevealedImprovementType(iObserverTeam, bDebug)
					if iImprovement >= 0:
						aiImprovementCounts[iImprovement] += 1
					iRoute = pPlot.getRevealedRouteType(iObserverTeam, bDebug)
					if iRoute >= 0:
						aiRouteCounts[iRoute] += 1
			aszRows.append([pCity.getName(), pCity.getOwner(), pCity.getID()])
			aaiBonusCounts.append(aiBonusCounts)
			aaiImprovementCounts.append(aiImprovementCounts)
			aaiRouteCounts.append(aiRouteCounts)
			(pCity, iter) = player.nextCity(iter, False)
		return aszRows, aaiBonusCounts, aaiImprovementCounts, aaiRouteCounts

	# <!-- custom: aoGroups is a list of (aiColumns, fnGetInfo, aaiCounts) tuples; aaiCounts[iCity][iType] holds the per-city count for that group.
	# Multi-group support lets one BFC 2 table render two icon families side by side (improvements + routes). (Claude code Opus 4.7) -->
	def drawBFC2IconTable(self, aszRows, aoGroups, iY, iH):
		screen = self.getScreen()
		szTable = self.getNextWidgetName()
		iNumDataCols = 0
		for aiColumns, fnGetInfo, aaiCounts in aoGroups:
			iNumDataCols += len(aiColumns)
		iNumCols = 2 + iNumDataCols
		screen.addTableControlGFC(szTable, iNumCols, self.BFC_X_TABLE, iY, self.BFC_W_TABLE, iH, True, True, self.BFC_ICON_SIZE, self.BFC_ICON_SIZE, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSort(szTable)
		iDataW = 0
		if iNumDataCols > 0:
			iDataW = (self.BFC_W_TABLE - self.BFC_ZOOM_COL_WIDTH - self.BFC_CITY_COL_WIDTH) / iNumDataCols
		screen.setTableColumnHeader(szTable, 0, "", self.BFC_ZOOM_COL_WIDTH)
		screen.setTableColumnHeader(szTable, 1, sasFontTagLabel + self.TEXT_CITY + SAS_FONT_TAG_CLOSE, self.BFC_CITY_COL_WIDTH)
		iColIdx = 2
		for aiColumns, fnGetInfo, aaiCounts in aoGroups:
			for iType in aiColumns:
				screen.setTableColumnHeader(szTable, iColIdx, SASTextScale.labelImageText(fnGetInfo(iType).getButton(), self.BFC_ICON_SIZE), iDataW)
				iColIdx += 1

		for iRow in range(len(aszRows)):
			self.appendBFC2Row(screen, szTable, iRow, aszRows[iRow], aoGroups, False)
		if len(aszRows) > 0:
			self.appendBFC2Row(screen, szTable, len(aszRows), [self.TEXT_TOTAL, -1, -1], aoGroups, True)

	def appendBFC2Row(self, screen, szTable, iRow, rowData, aoGroups, bIsTotal):
		screen.appendTableRow(szTable)
		szCityName = rowData[0]
		iOwner = rowData[1]
		iCityID = rowData[2]
		if iOwner >= 0:
			screen.setTableText(szTable, 0, iRow, "", self.ART_CITY_SELECTION_BUTTON, WidgetTypes.WIDGET_ZOOM_CITY, iOwner, iCityID, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, 1, iRow, sasFontTagLabel + szCityName + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		else:
			screen.setTableText(szTable, 0, iRow, u"", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, 1, iRow, sasFontTagLabel + szCityName + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		iColIdx = 2
		for aiColumns, fnGetInfo, aaiCounts in aoGroups:
			for iType in aiColumns:
				if bIsTotal:
					iCount = 0
					for iCityRow in range(len(aaiCounts)):
						iCount += aaiCounts[iCityRow][iType]
				else:
					iCount = aaiCounts[iRow][iType]
				self.setBFCCountCell(screen, szTable, iColIdx, iRow, iCount)
				iColIdx += 1

	# TERRITORY
	def drawTerritoryTab(self):
		aPlotRows, aTerrainRows, aFeatureRows, aBonusRows, aImprovementRows = self.collectTerritoryData()
		iGap = 14
		iTableW = (self.BFC_W_TABLE - 3 * iGap) / 4
		iY = self.BFC_Y_PLOT_TABLE
		iH = self.CONTENT_Y_BOTTOM - iY - 18
		iHalfH = (iH - iGap) / 2
		iX2 = self.BFC_X_TABLE + iTableW + iGap
		self.drawTerritoryCountTable(aPlotRows, self.TEXT_TERRITORY_PLOTS, self.BFC_X_TABLE, iY, iTableW, iH)
		self.drawTerritoryCountTable(aTerrainRows, self.TEXT_TERRITORY_TERRAIN, iX2, iY, iTableW, iHalfH)
		self.drawTerritoryCountTable(aFeatureRows, self.TEXT_TERRITORY_FEATURES, iX2, iY + iHalfH + iGap, iTableW, iHalfH)
		self.drawTerritoryCountTable(aBonusRows, self.TEXT_TERRITORY_BONUSES, self.BFC_X_TABLE + 2 * (iTableW + iGap), iY, iTableW, iH)
		self.drawTerritoryCountTable(aImprovementRows, self.TEXT_TERRITORY_IMPROVEMENTS, self.BFC_X_TABLE + 3 * (iTableW + iGap), iY, self.BFC_W_TABLE - 3 * (iTableW + iGap), iH)
		self.drawTerritoryLegendLink()

	def drawTerritoryLegendLink(self):
		iX = self.BFC_X_TABLE + self.BFC_W_TABLE - 6
		iY = self.Y_TITLE
		placeAdvisorLegendLink(self, "CONCEPT_SAS_WORLD_ADVISOR_TERRITORY_LEGEND", iX, iY)

	def collectTerritoryData(self):
		aiBFCPlots = self.getTerritoryBFCPlotSet()
		# Plot count slots: 0=Total 1=Water 2=Peak 3=Hill 4=Flat 5=River 6=Lake 7=Coastal
		aiPlotCounts = [[0, 0], [0, 0], [0, 0], [0, 0], [0, 0], [0, 0], [0, 0], [0, 0]]
		aaiTerrainCounts = []
		aaiFeatureCounts = []
		aaiRouteCounts = []
		aaiBonusCounts = []
		aaiImprovementCounts = []
		for i in range(gc.getNumTerrainInfos()):
			aaiTerrainCounts.append([0, 0])
		for i in range(gc.getNumFeatureInfos()):
			aaiFeatureCounts.append([0, 0])
		for i in range(gc.getNumRouteInfos()):
			aaiRouteCounts.append([0, 0])
		for i in range(gc.getNumBonusInfos()):
			aaiBonusCounts.append([0, 0])
		for i in range(gc.getNumImprovementInfos()):
			aaiImprovementCounts.append([0, 0])

		cyMap = CyMap()
		bDebug = CyGame().isDebugMode()
		iObserverTeam = self.getMapObserverTeam()
		for iPlot in range(cyMap.numPlots()):
			pPlot = cyMap.plotByIndex(iPlot)
			# <!-- custom: Territory membership used live ownership even under fog, revealing unseen border changes despite KI#224's observer-relative detail reads.
			# Use the observer's remembered owner; debug mode still receives live ownership through the API's debug flag. See KI#1069. (GPT-5.6-Sol) -->
			if pPlot and not pPlot.isNone() and (bDebug or pPlot.isRevealed(iObserverTeam, False)) and pPlot.getRevealedOwner(iObserverTeam, bDebug) == self.iActivePlayer:
				iBucket = 1
				if iPlot in aiBFCPlots:
					iBucket = 0
				aiPlotCounts[0][iBucket] += 1
				if pPlot.isWater():
					aiPlotCounts[1][iBucket] += 1
					if pPlot.isLake():
						aiPlotCounts[6][iBucket] += 1
				else:
					if pPlot.isPeak():
						aiPlotCounts[2][iBucket] += 1
					elif pPlot.isHills():
						aiPlotCounts[3][iBucket] += 1
					else:
						aiPlotCounts[4][iBucket] += 1
					if pPlot.isCoastalLand():
						aiPlotCounts[7][iBucket] += 1
				if pPlot.isRiverSide():
					aiPlotCounts[5][iBucket] += 1

				iTerrain = pPlot.getTerrainType()
				if iTerrain >= 0:
					aaiTerrainCounts[iTerrain][iBucket] += 1
				iFeature = pPlot.getFeatureType()
				if iFeature >= 0:
					aaiFeatureCounts[iFeature][iBucket] += 1
				iRoute = pPlot.getRevealedRouteType(iObserverTeam, bDebug)
				if iRoute >= 0:
					aaiRouteCounts[iRoute][iBucket] += 1
				iBonus = pPlot.getBonusType(iObserverTeam)
				if iBonus >= 0:
					aaiBonusCounts[iBonus][iBucket] += 1
				iImprovement = pPlot.getRevealedImprovementType(iObserverTeam, bDebug)
				if iImprovement >= 0:
					aaiImprovementCounts[iImprovement][iBucket] += 1

		aPlotRows = [
			[self.TEXT_WATER, "", WidgetTypes.WIDGET_GENERAL, -1, aiPlotCounts[1][0], aiPlotCounts[1][1]],
			[self.TEXT_PEAK, self.BUTTON_TERRAIN_PEAK, WidgetTypes.WIDGET_GENERAL, -1, aiPlotCounts[2][0], aiPlotCounts[2][1]],
			[self.TEXT_HILL, self.BUTTON_TERRAIN_HILL, WidgetTypes.WIDGET_GENERAL, -1, aiPlotCounts[3][0], aiPlotCounts[3][1]],
			[self.TEXT_FLAT, "", WidgetTypes.WIDGET_GENERAL, -1, aiPlotCounts[4][0], aiPlotCounts[4][1]],
			[self.TEXT_RIVER, self.BUTTON_RIVER, WidgetTypes.WIDGET_GENERAL, -1, aiPlotCounts[5][0], aiPlotCounts[5][1]],
			[self.TEXT_LAKE, "", WidgetTypes.WIDGET_GENERAL, -1, aiPlotCounts[6][0], aiPlotCounts[6][1]],
			[self.TEXT_COASTAL, "", WidgetTypes.WIDGET_GENERAL, -1, aiPlotCounts[7][0], aiPlotCounts[7][1]],
			[self.TEXT_TOTAL, "", WidgetTypes.WIDGET_GENERAL, -1, aiPlotCounts[0][0], aiPlotCounts[0][1]],
			]
		# <!-- custom: Peak and Hill are plot-shape categories, not real terrain here: e.g. a Grassland Hill still has Grassland as getTerrainType().
		# Keep them in Plots, not Terrain. (GPT-5.5) -->
		aTerrainRows = []
		for iTerrain in self.getTerrainColumnsWithoutPlotShapes():
			self.appendTerritoryInfoRow(aTerrainRows, gc.getTerrainInfo(iTerrain), WidgetTypes.WIDGET_PEDIA_JUMP_TO_TERRAIN, iTerrain, aaiTerrainCounts[iTerrain])
		aFeatureRows = []
		for iFeature in range(gc.getNumFeatureInfos()):
			self.appendTerritoryInfoRow(aFeatureRows, gc.getFeatureInfo(iFeature), WidgetTypes.WIDGET_PEDIA_JUMP_TO_FEATURE, iFeature, aaiFeatureCounts[iFeature])
		aBonusRows = []
		for iBonus in range(gc.getNumBonusInfos()):
			if gc.getBonusInfo(iBonus).getPlacementOrder() >= 0:
				self.appendTerritoryInfoRow(aBonusRows, gc.getBonusInfo(iBonus), WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus, aaiBonusCounts[iBonus])
		aImprovementRows = []
		for iImprovement in range(gc.getNumImprovementInfos()):
			self.appendTerritoryInfoRow(aImprovementRows, gc.getImprovementInfo(iImprovement), WidgetTypes.WIDGET_PEDIA_JUMP_TO_IMPROVEMENT, iImprovement, aaiImprovementCounts[iImprovement])
		for iRoute in range(gc.getNumRouteInfos()):
			self.appendTerritoryInfoRow(aImprovementRows, gc.getRouteInfo(iRoute), WidgetTypes.WIDGET_GENERAL, -1, aaiRouteCounts[iRoute])
		return aPlotRows, aTerrainRows, aFeatureRows, aBonusRows, aImprovementRows

	def getTerrainColumnsWithoutPlotShapes(self):
		aiTerrainColumns = []
		for iTerrain in range(gc.getNumTerrainInfos()):
			if iTerrain != self.iTerrainPeak and iTerrain != self.iTerrainHill:
				aiTerrainColumns.append(iTerrain)
		return aiTerrainColumns

	def getTerritoryBFCPlotSet(self):
		aiBFCPlots = {}
		cyMap = CyMap()
		player = gc.getPlayer(self.iActivePlayer)
		(pCity, iter) = player.firstCity(False)
		while pCity and not pCity.isNone():
			for iPlot in range(gc.getNUM_CITY_PLOTS()):
				pPlot = pCity.getCityIndexPlot(iPlot)
				if pPlot and not pPlot.isNone():
					aiBFCPlots[cyMap.plotNum(pPlot.getX(), pPlot.getY())] = True
			(pCity, iter) = player.nextCity(iter, False)
		return aiBFCPlots

	def appendTerritoryInfoRow(self, aRows, info, eWidget, iData, aiCounts):
		# <!-- custom: Keep zero-count rows in Territory so each table doubles as a stable checklist of map types known to the mod; players can still sort by BFC/Sub/Tot to put present assets first. (GPT-5.5) -->
		aRows.append([info.getDescription(), info.getButton(), eWidget, iData, aiCounts[0], aiCounts[1]])

	def drawTerritoryCountTable(self, aRows, szTitle, iX, iY, iW, iH):
		screen = self.getScreen()
		szTable = self.getNextWidgetName()
		screen.addTableControlGFC(szTable, 4, iX, iY, iW, iH, True, True, self.BFC_ICON_SIZE, self.BFC_ICON_SIZE, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSort(szTable)
		iCountW = 60
		iNameW = iW - 3 * iCountW
		screen.setTableColumnHeader(szTable, 0, sasFontTagLabel + szTitle + SAS_FONT_TAG_CLOSE, iNameW)
		screen.setTableColumnHeader(szTable, 1, sasFontTagLabel + self.TEXT_BFC + SAS_FONT_TAG_CLOSE, iCountW)
		screen.setTableColumnHeader(szTable, 2, sasFontTagLabel + self.TEXT_TERRITORY_SUBURBS + SAS_FONT_TAG_CLOSE, iCountW)
		screen.setTableColumnHeader(szTable, 3, sasFontTagLabel + self.TEXT_TERRITORY_TOTAL_ABBR + SAS_FONT_TAG_CLOSE, iCountW)
		for iRow in range(len(aRows)):
			screen.appendTableRow(szTable)
			row = aRows[iRow]
			screen.setTableText(szTable, 0, iRow, sasFontTagLabel + row[0] + SAS_FONT_TAG_CLOSE, row[1], row[2], row[3], -1, CvUtil.FONT_LEFT_JUSTIFY)
			self.setBFCCountCell(screen, szTable, 1, iRow, row[4])
			self.setBFCCountCell(screen, szTable, 2, iRow, row[5])
			self.setBFCCountCell(screen, szTable, 3, iRow, row[4] + row[5])
		bHasTotal = False
		for row in aRows:
			if row[0] == self.TEXT_TOTAL:
				bHasTotal = True
				break
		if len(aRows) > 0 and not bHasTotal:
			iTotalBFC = 0
			iTotalSub = 0
			for row in aRows:
				iTotalBFC += row[4]
				iTotalSub += row[5]
			iTotalRow = len(aRows)
			screen.appendTableRow(szTable)
			screen.setTableText(szTable, 0, iTotalRow, sasFontTagLabel + self.TEXT_TOTAL + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			self.setBFCCountCell(screen, szTable, 1, iTotalRow, iTotalBFC)
			self.setBFCCountCell(screen, szTable, 2, iTotalRow, iTotalSub)
			self.setBFCCountCell(screen, szTable, 3, iTotalRow, iTotalBFC + iTotalSub)

	def setBFCCountCell(self, screen, szTable, iCol, iRow, iCount):
		if iCount == 0:
			screen.setTableText(szTable, iCol, iRow, u"", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
		else:
			screen.setTableInt(szTable, iCol, iRow, sasFontTagLabel + str(iCount) + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	# ENVIRONMENT
	def drawEnvironmentTab(self):
		screen = self.getScreen()
		game = CyGame()
		player = gc.getPlayer(self.iActivePlayer)

		X_LEFT_PANEL = self.ENV_X_LEFT_PANEL
		X_MIDDLE_PANEL = self.ENV_X_MIDDLE_PANEL
		X_RIGHT_PANEL = self.ENV_X_RIGHT_PANEL
		Y_TOP_PANEL = self.ENV_Y_TOP_PANEL
		H_TOP_PANEL = self.ENV_H_TOP_PANEL
		Y_LOCATION = self.ENV_Y_LOCATION
		PANE_WIDTH = self.ENV_PANE_WIDTH
		PANE_HEIGHT = self.ENV_PANE_HEIGHT
		TEXT_MARGIN = self.ENV_TEXT_MARGIN
		Y_SPACING = self.ENV_Y_SPACING

		szTopPanel = self.getNextWidgetName()
		screen.addPanel(szTopPanel, u"", "", True, True, X_LEFT_PANEL, Y_TOP_PANEL, X_RIGHT_PANEL + PANE_WIDTH - X_LEFT_PANEL, H_TOP_PANEL, PanelStyles.PANEL_STYLE_MAIN)

		iGlobalWarmingIndex = game.getGlobalWarmingIndex()
		iGlobalWarmingChances = game.getGlobalWarmingChances()
		iGwEventTally = game.getGwEventTally()
		iSeverityRating = game.calculateGwSeverityRating()

		szText = self.TEXT_GW_SEVERITY_RATING + ": "
		# advc.137: Better use separate keys
		if iSeverityRating < 30:
			szText += localText.getColorText("TXT_KEY_LOW", (), self.COLOR_GREEN).upper()
		elif iSeverityRating < 75:
			szText += localText.getColorText("TXT_KEY_MEDIUM", (), self.COLOR_YELLOW).upper()
		else:
			szText += localText.getColorText("TXT_KEY_HIGH", (), self.COLOR_RED).upper()
		if CyGame().isDebugMode():
			szText += u" (%d)" % iSeverityRating
		screen.setLabel(self.getNextWidgetName(), szTopPanel, sasFontTagTitle + szText + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, (X_LEFT_PANEL + PANE_WIDTH + X_RIGHT_PANEL) / 2, Y_TOP_PANEL + H_TOP_PANEL / 2 - Y_SPACING / 2, self.Z_CONTROLS + self.DZ, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		szLeftPanel = self.getNextWidgetName()
		screen.addPanel(szLeftPanel, u"", "", True, True, X_LEFT_PANEL, Y_LOCATION, PANE_WIDTH, PANE_HEIGHT, PanelStyles.PANEL_STYLE_MAIN)
		screen.setLabel(self.getNextWidgetName(), "Background", self.LABEL_ENV_DOMESTIC, CvUtil.FONT_CENTER_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH / 2, Y_LOCATION + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		szMiddlePanel = self.getNextWidgetName()
		screen.addPanel(szMiddlePanel, u"", "", True, True, X_MIDDLE_PANEL, Y_LOCATION, PANE_WIDTH, PANE_HEIGHT, PanelStyles.PANEL_STYLE_MAIN)
		screen.setLabel(self.getNextWidgetName(), "Background", self.LABEL_ENV_GLOBAL, CvUtil.FONT_CENTER_JUSTIFY, X_MIDDLE_PANEL + PANE_WIDTH / 2, Y_LOCATION + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		szRightPanel = self.getNextWidgetName()
		screen.addPanel(szRightPanel, u"", "", True, True, X_RIGHT_PANEL, Y_LOCATION, PANE_WIDTH, PANE_HEIGHT, PanelStyles.PANEL_STYLE_MAIN)
		screen.setLabel(self.getNextWidgetName(), "Background", self.LABEL_ENV_EFFECTS, CvUtil.FONT_CENTER_JUSTIFY, X_RIGHT_PANEL + PANE_WIDTH / 2, Y_LOCATION + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# Left panel: domestic
		yLocation = Y_LOCATION
		iPopulation = player.calculatePollution(PollutionTypes.POLLUTION_POPULATION)
		iBuildings = player.calculatePollution(PollutionTypes.POLLUTION_BUILDINGS)
		iResources = player.calculatePollution(PollutionTypes.POLLUTION_BONUSES)
		iPower = player.calculatePollution(PollutionTypes.POLLUTION_POWER)
		iLocalPollution = player.calculatePollution(PollutionTypes.POLLUTION_ALL)
		iLocalThreshold = game.calculateGwSustainabilityThreshold(self.iActivePlayer)
		iPollutionTypes = (iPopulation != 0) * PollutionTypes.POLLUTION_POPULATION | (iBuildings != 0) * PollutionTypes.POLLUTION_BUILDINGS | (iResources != 0) * PollutionTypes.POLLUTION_BONUSES | (iPower != 0) * PollutionTypes.POLLUTION_POWER
		iLocalDefence = game.calculateGwLandDefence(self.iActivePlayer)

		yLocation += 1.5 * Y_SPACING
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_POLLUTION + ":" + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_POLLUTION_SOURCE, iPollutionTypes, -1)

		if iPopulation != 0:
			yLocation += Y_SPACING
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_POPULATION + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + 2 * TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_POLLUTION_SOURCE, PollutionTypes.POLLUTION_POPULATION, -1)
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + unicode(iPopulation) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_POLLUTION_SOURCE, PollutionTypes.POLLUTION_POPULATION, -1)
		if iBuildings != 0:
			yLocation += Y_SPACING
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_BUILDINGS + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + 2 * TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_POLLUTION_SOURCE, PollutionTypes.POLLUTION_BUILDINGS, -1)
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + unicode(iBuildings) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_POLLUTION_SOURCE, PollutionTypes.POLLUTION_BUILDINGS, -1)
		if iResources != 0:
			yLocation += Y_SPACING
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_RESOURCES + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + 2 * TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_POLLUTION_SOURCE, PollutionTypes.POLLUTION_BONUSES, -1)
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + unicode(iResources) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_POLLUTION_SOURCE, PollutionTypes.POLLUTION_BONUSES, -1)
		if iPower != 0:
			yLocation += Y_SPACING
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_ELECTRICITY + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + 2 * TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_POLLUTION_SOURCE, PollutionTypes.POLLUTION_POWER, -1)
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + unicode(iPower) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_POLLUTION_SOURCE, PollutionTypes.POLLUTION_POWER, -1)

		yLocation += Y_SPACING
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_TOTAL + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_POLLUTION_SOURCE, iPollutionTypes, -1)
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + unicode(iLocalPollution) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_POLLUTION_SOURCE, iPollutionTypes, -1)
		yLocation += 1.5 * Y_SPACING
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_TOTAL_OFFSETS + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_POLLUTION_OFFSETS, -1, -1)
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + unicode(-iLocalDefence) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_POLLUTION_OFFSETS, -1, -1)
		yLocation += 1.5 * Y_SPACING
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_TOTAL_IMPACT + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + unicode(iLocalPollution - iLocalDefence) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		yLocation += 1.5 * Y_SPACING
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_LOCAL_THRESHOLD + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_SUSTAINABILITY_THRESHOLD, -1, -1)
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + unicode(iLocalThreshold) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_SUSTAINABILITY_THRESHOLD, -1, -1)

		# Middle panel: Global
		yLocation = Y_LOCATION
		iGlobalPollution = game.calculateGlobalPollution()
		iGlobalDefence = game.calculateGwLandDefence(PlayerTypes.NO_PLAYER)
		iThreshold = game.calculateGwSustainabilityThreshold(PlayerTypes.NO_PLAYER)
		if self.iGLOBAL_WARMING_RESTORATION_RATE is None:
			self.iGLOBAL_WARMING_RESTORATION_RATE = gc.getDefineINT("GLOBAL_WARMING_RESTORATION_RATE")
			self.iGLOBAL_WARMING_PROB = gc.getDefineINT("GLOBAL_WARMING_PROB")
		iChangeRate = iGlobalPollution - iGlobalDefence - iThreshold - iGlobalWarmingIndex * self.iGLOBAL_WARMING_RESTORATION_RATE / 100

		yLocation += 1.5 * Y_SPACING
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_TOTAL_POLLUTION + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_MIDDLE_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_POLLUTION_SOURCE, PollutionTypes.POLLUTION_ALL, -1)
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + unicode(iGlobalPollution) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_MIDDLE_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_POLLUTION_SOURCE, PollutionTypes.POLLUTION_ALL, -1)
		yLocation += 1.5 * Y_SPACING
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_TOTAL_OFFSETS + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_MIDDLE_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_POLLUTION_OFFSETS, -1, -1)
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + unicode(-iGlobalDefence) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_MIDDLE_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_POLLUTION_OFFSETS, -1, -1)
		yLocation += 1.5 * Y_SPACING
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_THRESHOLD + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_MIDDLE_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_SUSTAINABILITY_THRESHOLD, -1, -1)
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + unicode(iThreshold) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_MIDDLE_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_SUSTAINABILITY_THRESHOLD, -1, -1)
		yLocation += 1.5 * Y_SPACING
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_GW_CHANGE_RATE + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_MIDDLE_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + unicode(max(0, iChangeRate)) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_MIDDLE_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		iResponsibility = 100 * (iLocalPollution - iLocalDefence)
		iResponsibility /= max(1, iLocalThreshold)
		iResponsibility *= iThreshold
		iResponsibility /= max(1, iGlobalPollution - iGlobalDefence)
		if iGwEventTally >= 0 or CyGame().isDebugMode():
			yLocation += 1.5 * Y_SPACING
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_RELATIVE_CONTRIBUTION + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_MIDDLE_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_GW_RELATIVE_CONTRIBUTION, -1, -1)
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + unicode(iResponsibility) + u"%" + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_MIDDLE_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_GW_RELATIVE_CONTRIBUTION, -1, -1)

		# Right panel: Effects
		yLocation = Y_LOCATION
		yLocation += 1.5 * Y_SPACING
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_GW_INDEX + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_RIGHT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_GW_INDEX, -1, -1)
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + unicode(iGlobalWarmingIndex) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_RIGHT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_GW_INDEX, -1, -1)
		yLocation += 1.5 * Y_SPACING
		fExpectedEvents = 1.0 * iGlobalWarmingChances * self.iGLOBAL_WARMING_PROB / (10.0 * gc.getGameSpeedInfo(game.getGameSpeedType()).getVictoryDelayPercent())
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_EXPECTED_EVENTS + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_RIGHT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + u"%0.1f" % fExpectedEvents + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_RIGHT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		if iGwEventTally >= 0:
			yLocation += 1.5 * Y_SPACING
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_EVENT_TALLY + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_RIGHT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + unicode(iGwEventTally) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_RIGHT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		iGwPercentAnger = player.getGwPercentAnger()
		if iGwPercentAnger >= 2:
			yLocation += 1.5 * Y_SPACING
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_LOCAL_ANGER_LEVEL + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_RIGHT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_GW_UNHAPPY, -1, -1)
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + unicode(iGwPercentAnger) + u"%" + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_RIGHT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_GW_UNHAPPY, -1, -1)
		if CyGame().isDebugMode():
			yLocation += 1.5 * Y_SPACING
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + self.TEXT_ENV_CHANCES + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_RIGHT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + unicode(game.getGlobalWarmingChances()) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_RIGHT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, self.Z_CONTROLS + self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def getNextWidgetName(self):
		szName = self.WIDGET_ID + str(self.nWidgetCount)
		self.nWidgetCount += 1
		return szName

	def deleteAllWidgets(self, iNumPermanentWidgets = 0):
		screen = self.getScreen()
		i = self.nWidgetCount - 1
		while i >= iNumPermanentWidgets:
			self.nWidgetCount = i
			screen.deleteWidget(self.getNextWidgetName())
			i -= 1
		self.nWidgetCount = iNumPermanentWidgets

	def handleInput(self, inputClass):
		szWidgetName = inputClass.getFunctionName() + str(inputClass.getID())
		if (szWidgetName == self.szExitButtonName and inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED or inputClass.getData() == int(InputTypes.KB_RETURN)):
			self.getScreen().hideScreen()
			return 1

		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED:
			if inputClass.getFunctionName() == self.DEBUG_DROPDOWN_ID:
				screen = self.getScreen()
				self.iActivePlayer = getAdvisorDebugDropdownSelectedPlayer(screen, self.DEBUG_DROPDOWN_ID)
				self.pActivePlayer = gc.getPlayer(self.iActivePlayer)
				self.iActiveTeam = self.pActivePlayer.getTeam()
				self.pActiveTeam = gc.getTeam(self.iActiveTeam)
				self.redrawContents()
				return 1
			if (self.iActiveTab == self.iBFC1ID or self.iActiveTab == self.iBFC2ID) and inputClass.getMouseX() == 0:
				if isAdvisorReadOnlyPerspective(self.iActivePlayer):
					return 1
				self.getScreen().hideScreen()
				CyInterface().selectCity(gc.getPlayer(inputClass.getData1()).getCity(inputClass.getData2()), True)
				popupInfo = CyPopupInfo()
				popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON_SCREEN)
				popupInfo.setText(u"showWorldAdvisorScreen")
				popupInfo.addPopup(inputClass.getData1())
				return 1

		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED:
			iSelected = inputClass.getData1()
			if iSelected >= 0 and iSelected < len(self.PAGE_NAME_LIST) and iSelected != self.iActiveTab and szWidgetName == "WorldAdvisorTabButton" + str(iSelected):
				self.iActiveTab = iSelected
				self.redrawContents()
				return 1
		return 0

	def update(self, fDelta):
		return
