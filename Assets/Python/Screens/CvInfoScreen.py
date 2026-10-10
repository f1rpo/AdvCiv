## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
#
# Thanks to "Ulf 'ulfn' Norell" from Apolyton for his additions relating to the graph section of this screen
#
# This file has been edited for K-Mod in various places. Some changes marked, some not. (deletions generally not marked)
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from CvPythonExtensions import *
import CvUtil
import ScreenInput
import CvScreenEnums

#import time
import math
import copy # advc.091
import re

from PyHelpers import PyPlayer
from SASUtils import *
from SASFontUtils import *
import SASTextScale
# <!-- custom: AdvCiv-SAS readability pass: use LABEL as the base info-screen text tag (instead of BODY) for clearer upscaled UI text. (GPT-5.3-Codex) -->

# <!-- custom: Begin - Score Tab dependencies (scoreboard visibility filters + diplomacy/attitude/player helpers). (GPT-5.3-Codex) -->
import Scoreboard
import DiplomacyUtil
import AttitudeUtil
import PlayerUtil
import FontUtil
import TraitUtil
# <!-- custom: End - Score Tab dependencies (scoreboard visibility filters + diplomacy/attitude/player helpers). (GPT-5.3-Codex) -->

# BUG - 3.17 No Espionage - start
import GameUtil
# BUG - 3.17 No Espionage - end

#BUG: Change Graphs - start
import BugCore
import BugUtil
AdvisorOpt = BugCore.game.Advisors
ScoreOpt = BugCore.game.Scores
#BUG: Change Graphs - end

# globals
gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class CvInfoScreen:
	"Info Screen! Contains the Demographics, Wonders / Top Cities and Statistics Screens"

	def __init__(self, screenId):

		# <advc.077> Settings for Demographics tab
		# Shows the active player's value and rank in a single column
		self.bRankInValueColumn = True
		# Exports column: no room anymore
		self.bShowExports = False
		# Instead of showing a "?" when the best or worst rival's demographics are unknown, show the demographics of the best or worst rival whose demographics are visible.
		self.bShowBestKnown = True
		# If the above is False, setting this to True will show the name of the best and worst leader even if demographics are not visible (so long as the leader has been met).
		self.bAlwaysShowBestWorstNameIfMet = False
		if self.bShowBestKnown:
			self.bAlwaysShowBestWorstNameIfMet = False
		self.bRevealAll = False
		# Rank numbers shown for the active player and its rivals are based on
		# only on demographics of civs that the active player has met.
		# Set to False to ignore has-met status when calculating tank numbers.
		self.bRanksAmongKnown = True
		# </advc.077>

		self.screenId = screenId
		self.DEMO_SCREEN_NAME = "DemographicsScreen"
		self.TOP_CITIES_SCREEN_NAME = "TopCitiesScreen"

		self.INTERFACE_ART_INFO = "TECH_BG"

		self.WIDGET_ID = "DemoScreenWidget"
		self.LINE_ID = "DemoLine"

		self.Z_BACKGROUND = -6.1
		self.Z_CONTROLS = self.Z_BACKGROUND - 0.2
		self.DZ = -0.2
		self.Z_HELP_AREA = self.Z_CONTROLS - 1

		self.W_LEFT_SPACE_FOR_COMMERCE_SLIDERS = SAS_ADVISOR_LEFT_SPACE_FOR_COMMERCE_SLIDERS
		self.W_RIGHT_SPACE_FOR_SCOREBOARD = SAS_ADVISOR_RIGHT_SPACE_FOR_SCOREBOARD
		self.H_TOP_SPACE_FOR_TECH_BAR = SAS_ADVISOR_TOP_SPACE_FOR_TECH_BAR
		self.H_BOTTOM_SPACE = SAS_ADVISOR_BOTTOM_SPACE

		self.BORDER_WIDTH = 4
		self.W_HELP_AREA = 200

		# K-Mod
		self.iLanguageLoaded = -1

		# <!-- custom: Score Tab registration in the Info screen tab bar/order. (GPT-5.3-Codex) -->
		self.PAGE_NAME_LIST = ["TXT_KEY_INFO_GRAPH", "TXT_KEY_GAME_SCORE", "TXT_KEY_SAS_INFO_TIMELINE", "TXT_KEY_DEMO_SCREEN_TITLE", "TXT_KEY_WONDERS_SCREEN_TOP_CITIES_TEXT", "TXT_KEY_INFO_SCREEN_STATISTICS_TITLE"]
		self.PAGE_DRAW_LIST = [self.drawGraphTab, self.drawScoreTab, self.drawTimelineTab, self.drawDemographicsTab, self.drawTopCitiesTab, self.drawStatsTab]

		self.PAGE_LINK_WIDTH = [] # game text is not available at the time this function is called, so we can't calculate the widths yet.
		# K-Mod end

		self.graphEnd		= CyGame().getGameTurn() - 1
		self.graphZoom		= self.graphEnd - CyGame().getStartTurn()
		self.nWidgetCount	= 0
		self.nLineCount		= 0

		# This is used to allow the wonders screen to refresh without redrawing everything
		self.iNumWondersPermanentWidgets = 0

		# <!-- custom: Score Tab ID insertion; all later tab IDs shift by +1. (GPT-5.3-Codex) -->
		self.iGraphID			= 0
		self.iScoreID			= 1
		self.iTimelineID		= 2
		self.iDemographicsID	= 3
		self.iTopCitiesID		= 4
		self.iStatsID			= 5

		# <!-- custom: move screen-independent Info Screen layout constants to init; updateRuntimeLayout now focuses on runtime-dependent geometry. (GPT-5.3-Codex) -->
		# <!-- custom: X/Y margins are screen-independent constants set in init. (GPT-5.3-Codex) -->
		self.X_MARGIN = 45
		self.Y_MARGIN = 80
		self.SMALL_MARGIN = 20
		self.H_GRAPH_DROPDOWN = 35
		self.W_DEMO_DROPDOWN = 260
		self.PANEL_HEIGHT = 55
		self.TIMELINE_TABLE_VERTICAL_GAP = 28
		self.W_LEFT_BUTTON = 20
		self.H_LEFT_BUTTON = 20
		self.X_LEGEND_MARGIN = 10
		self.Y_LEGEND_MARGIN = 5
		self.W_LEGEND_LINE = 30

		self.iGraphTurnLabelYOffset = 16
		self.Graph_Status_1in1 = 0
		self.Graph_Status_7in1 = 1
		self.Graph_Status_3in1 = 2
		self.X_7_IN_1_CHART_ADJ = [0, 1, 2, 1, 2, 1, 2]
		self.Y_7_IN_1_CHART_ADJ = [0, 0, 0, 1, 1, 2, 2]
		self.X_3_IN_1_CHART_ADJ = [0, 1, 1]
		self.Y_3_IN_1_CHART_ADJ = [0, 0, 1]
		self.W_DEMOGRAPHICS_COL_DEM = 320
		self.DEMOGRAPHICS_W_TEXT = 140
		self.DEMOGRAPHICS_H_TEXT = 15
		self.DEMOGRAPHICS_X_TEXT_BUFFER = 0
		self.DEMOGRAPHICS_Y_TEXT_BUFFER = 43
		self.DEMOGRAPHICS_NUMERICAL_COL_W = 150
		self.DEMOGRAPHICS_RIVAL_COL_W = 220
		self.W_DEMOGRAPHICS_COL_VALUE = self.DEMOGRAPHICS_NUMERICAL_COL_W
		self.W_DEMOGRAPHICS_COL_RANK = self.DEMOGRAPHICS_NUMERICAL_COL_W
		self.W_DEMOGRAPHICS_COL_RIVAL_BEST = self.DEMOGRAPHICS_RIVAL_COL_W
		self.W_DEMOGRAPHICS_COL_RIVAL_WORST = self.DEMOGRAPHICS_RIVAL_COL_W
		self.W_DEMOGRAPHICS_COL_RIVAL_AVG = self.DEMOGRAPHICS_NUMERICAL_COL_W
		self.X_COL_1 = 535
		self.X_COL_2 = self.X_COL_1 + self.DEMOGRAPHICS_W_TEXT + self.DEMOGRAPHICS_X_TEXT_BUFFER
		self.X_COL_3 = self.X_COL_2 + self.DEMOGRAPHICS_W_TEXT + self.DEMOGRAPHICS_X_TEXT_BUFFER
		self.X_COL_4 = self.X_COL_3 + self.DEMOGRAPHICS_W_TEXT + self.DEMOGRAPHICS_X_TEXT_BUFFER
		self.Y_ROW_1 = 100
		self.Y_ROW_2 = self.Y_ROW_1 + self.DEMOGRAPHICS_H_TEXT + self.DEMOGRAPHICS_Y_TEXT_BUFFER
		self.Y_ROW_3 = self.Y_ROW_2 + self.DEMOGRAPHICS_H_TEXT + self.DEMOGRAPHICS_Y_TEXT_BUFFER
		self.Y_ROW_4 = self.Y_ROW_3 + self.DEMOGRAPHICS_H_TEXT + self.DEMOGRAPHICS_Y_TEXT_BUFFER
		self.Y_ROW_5 = self.Y_ROW_4 + self.DEMOGRAPHICS_H_TEXT + self.DEMOGRAPHICS_Y_TEXT_BUFFER
		self.Y_ROW_6 = self.Y_ROW_5 + self.DEMOGRAPHICS_H_TEXT + self.DEMOGRAPHICS_Y_TEXT_BUFFER
		self.Y_ROW_7 = self.Y_ROW_6 + self.DEMOGRAPHICS_H_TEXT + self.DEMOGRAPHICS_Y_TEXT_BUFFER
		self.Y_ROW_8 = self.Y_ROW_7 + self.DEMOGRAPHICS_H_TEXT + self.DEMOGRAPHICS_Y_TEXT_BUFFER
		self.Y_ROW_9 = self.Y_ROW_8 + self.DEMOGRAPHICS_H_TEXT + self.DEMOGRAPHICS_Y_TEXT_BUFFER
		self.Y_ROW_10 = self.Y_ROW_9 + self.DEMOGRAPHICS_H_TEXT + self.DEMOGRAPHICS_Y_TEXT_BUFFER
		self.iRankIconSize = 16
		self.W_WONDERS_RIGHT_PANE = 765
		self.iTopCitiesHeaderHeight = 35
		self.H_CITIES_DESC = 58
		self.Y_CITIES_DESC_BUFFER = -9
		self.MULTI_LIST_PANEL_OFFSET_X_NO_HEADER = 5
		self.MULTI_LIST_PANEL_OFFSET_Y_NO_HEADER = 5
		self.iTopCitiesWonderButtonSize = 46
		self.iTopCitiesWonderNumListsAutoCalculate = 1
		self.iTopCitiesWonderColumnIndexAuto = 0
		self.iTopCitiesWonderNumRows = 2
		self.iTopCitiesWonderPanelMargin = 8
		self.iTopCitiesWonderPanelHeightAdjust = -3
		self.iTopCitiesDescWonderGap = -2
		self.iTopCitiesInterCityMargin = 6
		self.iTopCitiesInterLeftRightPartsGapAdjust = 2
		self.iTopCitiesRightPartWAdjust = -16
		self.W_CITY_ANIMATION = 150
		self.W_WONDERS_CHART_COL_BUTTON = 30
		self.WONDERS_COL_MOVE_TO_CITY_ID = 4
		self.W_WONDERS_CHART_COL_DATE = 110
		self.W_WONDERS_CHART_COL_OWNER = 155
		self.W_WONDERS_CHART_COL_CITY = 155
		self.W_WONDERS_STATS_PANE = 210
		self.H_WONDERS_STATS_PANE = 220
		self.WONDERS_FUDGE_FACTOR = 3
		self.W_WONDERS_DROPDOWN = 420
		self.W_WONDER_LIST_BASE = 200
		self.W_WONDER_LIST = self.W_WONDER_LIST_BASE
		self.H_WONDER_LIST = 180
		self.X_WONDER_GRAPHIC = 540
		self.W_WONDER_GRAPHIC = 420
		self.H_WONDER_GRAPHIC = 190
		self.X_ROTATION_WONDER_ANIMATION = -20
		self.Z_ROTATION_WONDER_ANIMATION = 30
		self.WONDERS_SCALE_ANIMATION = 0.5
		self.W_PROJECT_ICON = 128
		self.X_WONDER_SPECIAL_TITLE = 540
		self.Y_WONDER_SPECIAL_TITLE = 517
		self.X_WONDER_SPECIAL_PANE = 540
		self.Y_WONDER_SPECIAL_PANE = 545
		self.W_WONDER_SPECIAL_PANE = 420
		self.H_WONDER_SPECIAL_PANE = 125
		self.Y_STATS_BOTTOM_CHART = 280
		self.Y_LEADER_ICON = 95
		self.W_LEADER_ICON = 128
		self.H_LEADER_ICON = 156
		self.iExtraWTopChartValue = 12
		self.H_STATS_TOP_PANEL = 0
		self.STATS_TOP_CHART_W_COL_1 = 100
		self.iNumTopChartCols = 2
		self.W_DEMOGRAPHICS_BUTTON_SIZE = 24
		self.H_DEMOGRAPHICS_BUTTON_SIZE = 24
		self.W_STATS_BUTTON_SIZE = 24
		self.H_STATS_BUTTON_SIZE = 24
		self.Y_TIMELINE_TABLE_LOG_BUTTON = 14

		self.iActiveTab = -1
#		self.iActiveTab = self.iGraphID

		self.iGraphTabID = -1
		self.iGraph_Smoothing_1in1 = -1
		self.iGraph_Smoothing_7in1 = -1
		self.iGraph_Smoothing_3in1 = -1

		self.TOTAL_SCORE	= 0
		self.ECONOMY_SCORE	= 1
		self.INDUSTRY_SCORE	= 2
		self.AGRICULTURE_SCORE	= 3
		self.POWER_SCORE	= 4
		self.CULTURE_SCORE	= 5
		self.ESPIONAGE_SCORE	= 6
		self.NUM_SCORES		= 7
		self.RANGE_SCORES	= range(self.NUM_SCORES)

		self.scoreCache	= []
		for t in self.RANGE_SCORES:
			self.scoreCache.append(None)

		# <!-- custom: Timeline tab cache - precompiled regex patterns for performance (Claude Opus 4.5) -->
		self.RE_COLOR_OPEN = re.compile(r"<color=.*?>")
		self.RE_COLOR_CLOSE = re.compile(r"</color>")

		# <!-- custom: Timeline tab aggressive cache - instance vars instead of dict for speed (Claude Opus 4.5) -->
		self.timelineCacheEntries = None  # List of formatted display strings
		self.timelineCacheActivePlayer = -1
		self.timelineCacheBRevealAll = False
		self.timelineCacheNumMessages = 0
		self.timelineCacheGameTurn = -1
		self.TIMELINE_TABLE_ID = "InfoScreenTimelineTable"
		self.TIMELINE_TABLE_CACHE_KEY = None

		self.GRAPH_H_LINE = "GraphHLine"
		self.GRAPH_V_LINE = "GraphVLine"

		self.xSelPt = 0
		self.ySelPt = 0

		self.graphLeftButtonID = ""
		self.graphRightButtonID = ""
		self.szTimelineDbgLogPrettySummaryButton = ""

		# <!-- custom: all screen-dependent geometry (bounds + tab layouts) is computed in updateRuntimeLayout() from the active resolution, following the same runtime pattern as CvForeignAdvisor. (GPT-5.3-Codex) -->

		self.reset()

	def initText(self):
		# K-Mod
		if not CyGame().isFinalInitialized():
			return

		# Calculate width for the links every time because it depends on runtime X_EXIT/screen width.
		aszPageLabels = []
		for i in self.PAGE_NAME_LIST:
			aszPageLabels.append(localText.getText(i, ()).upper())
		szExitLabel = localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT", ()).upper()
		self.PAGE_LINK_WIDTH[:] = getAdvisorRuntimeLinkWidths(CyInterface(), aszPageLabels, szExitLabel, self.X_EXIT)

		# only execute language-dependent text initialization once per active language.
		if self.iLanguageLoaded == CyGame().getCurrentLanguage():
			return
		self.iLanguageLoaded = CyGame().getCurrentLanguage()
		# K-Mod end

		###### TEXT ######
		self.SCREEN_TITLE = sasFontTagTitle.bold + localText.getText("TXT_KEY_INFO_SCREEN", ()).upper() + SAS_FONT_TAG_CLOSE

		self.EXIT_TEXT = sasFontTagTitle + localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT", ()).upper() + SAS_FONT_TAG_CLOSE

		self.TEXT_THOUSANDS_SEPARATOR_COMMA = localText.getText("TXT_KEY_THOUSANDS_SEPARATOR_COMMA", ())
		self.TEXT_SHOW_ALL_PLAYERS =  localText.getText("TXT_KEY_SHOW_ALL_PLAYERS", ())
		self.TEXT_SHOW_ALL_PLAYERS_GRAY = localText.getColorText("TXT_KEY_SHOW_ALL_PLAYERS", (), getInfoTypeOrFail("COLOR_PLAYER_GRAY")).upper()

		self.TEXT_ENTIRE_TIMELINE = localText.getText("TXT_KEY_SAS_INFO_ENTIRE_TIMELINE", ())
		self.TEXT_TIMELINE_EMPTY = localText.getText("TXT_KEY_SAS_INFO_TIMELINE_EMPTY", ())
		self.TEXT_TIMELINE_UNKNOWN_CITY = localText.getText("TXT_KEY_SAS_INFO_TIMELINE_UNKNOWN_CITY", ())
		self.TEXT_TIMELINE_DBG_LOG_PRETTY_SUMMARY_BUTTON = localText.getText("TXT_KEY_SAS_INFO_TIMELINE_LOG_BUTTON", ())

		self.TEXT_SCORE = localText.getText("TXT_KEY_GAME_SCORE", ())
		self.TEXT_POWER = localText.getText("TXT_KEY_POWER", ())
		self.TEXT_CULTURE = localText.getObjectText("TXT_KEY_COMMERCE_CULTURE", 0)
		self.TEXT_ESPIONAGE = localText.getObjectText("TXT_KEY_ESPIONAGE_CULTURE", 0)

		# <!-- custom: Score Tab shared UI constants cached once per language load. (GPT-5.3-Codex) -->
		self.SCORETAB_COLOR_ALT_HIGHLIGHT_TEXT = getInfoTypeOrFail("COLOR_ALT_HIGHLIGHT_TEXT")
		self.SCORETAB_COLOR_RED = getInfoTypeOrFail("COLOR_RED")
		self.COLOR_YELLOW = getInfoTypeOrFail("COLOR_YELLOW")
		self.COLOR_GREY = getInfoTypeOrFail("COLOR_GREY")
		self.SCORETAB_WAR_CHAR = FontUtil.getChar(FontSymbols.WAR_CHAR)
		self.SCORETAB_PEACE_CHAR = FontUtil.getChar(FontSymbols.PEACE_CHAR)
		self.SCORETAB_TRADE_CHAR = FontUtil.getChar(FontSymbols.TRADE_CHAR)
		self.SCORETAB_BORDERS_CHAR = FontUtil.getChar(FontSymbols.OPEN_BORDERS_CHAR)
		self.SCORETAB_PACT_CHAR = FontUtil.getChar(FontSymbols.DEFENSIVE_PACT_CHAR)
		self.SCORETAB_WORST_ENEMY_CHAR = FontUtil.getChar(FontSymbols.ANGRY_POP_CHAR)
		self.SCORETAB_GOLDEN_AGE_CHAR = FontUtil.getChar(FontSymbols.GOLDEN_AGE_CHAR)
		self.SCORETAB_ANARCHY_CHAR = FontUtil.getChar(FontSymbols.BAD_GOLD_CHAR)
		self.SCORETAB_ESPIONAGE_CHAR = u"%c" % gc.getCommerceInfo(CommerceTypes.COMMERCE_ESPIONAGE).getChar()
		self.SCORETAB_COLOR_MARKER = u"|||||"

		self.TEXT_VALUE = localText.getText("TXT_KEY_DEMO_SCREEN_VALUE_TEXT", ())
		self.TEXT_RANK = localText.getText("TXT_KEY_DEMO_SCREEN_RANK_TEXT", ())
		self.TEXT_AVERAGE = localText.getText("TXT_KEY_DEMOGRAPHICS_SCREEN_RIVAL_AVERAGE", ())
		self.TEXT_BEST = localText.getText("TXT_KEY_INFO_RIVAL_BEST", ())
		self.TEXT_WORST = localText.getText("TXT_KEY_INFO_RIVAL_WORST", ())

		self.TEXT_ECONOMY = localText.getText("TXT_KEY_DEMO_SCREEN_ECONOMY_TEXT", ())
		self.TEXT_INDUSTRY = localText.getText("TXT_KEY_DEMO_SCREEN_INDUSTRY_TEXT", ())
		self.TEXT_AGRICULTURE = localText.getText("TXT_KEY_DEMO_SCREEN_AGRICULTURE_TEXT", ())
		self.TEXT_MILITARY = localText.getText("TXT_KEY_DEMO_SCREEN_MILITARY_TEXT", ())
		self.TEXT_LAND_AREA = localText.getText("TXT_KEY_DEMO_SCREEN_LAND_AREA_TEXT", ())
		self.TEXT_POPULATION = localText.getText("TXT_KEY_DEMO_SCREEN_POPULATION_TEXT", ())
		self.TEXT_HAPPINESS = localText.getText("TXT_KEY_DEMO_SCREEN_HAPPINESS_TEXT", ())
		self.TEXT_HEALTH = localText.getText("TXT_KEY_DEMO_SCREEN_HEALTH_TEXT", ())
		#self.TEXT_IMP_EXP = localText.getText("TXT_KEY_DEMO_SCREEN_EXPORTS_TEXT", ()) + " - " + localText.getText("TXT_KEY_DEMO_SCREEN_IMPORTS_TEXT", ())
		# <advc.077> Replacing the above (or perhaps "Export Revenues" would be nicer?)
		self.TEXT_EXP = localText.getText("TXT_KEY_CONCEPT_FOREIGN_TRADE", ())
		# Put bullet in a variable, then disable it.
		#szBullet = (u"  %c" % CyGame().getSymbolID(FontSymbols.BULLET_CHAR))
		szBullet = ""

		# </advc.077>
		self.TEXT_ECONOMY_MEASURE = szBullet + localText.getText("TXT_KEY_DEMO_SCREEN_ECONOMY_MEASURE", ())
		self.TEXT_INDUSTRY_MEASURE = szBullet + localText.getText("TXT_KEY_DEMO_SCREEN_INDUSTRY_MEASURE", ())
		self.TEXT_AGRICULTURE_MEASURE = szBullet + localText.getText("TXT_KEY_DEMO_SCREEN_AGRICULTURE_MEASURE", ())
		self.TEXT_MILITARY_MEASURE = ""
		self.TEXT_LAND_AREA_MEASURE = szBullet + localText.getText("TXT_KEY_DEMO_SCREEN_LAND_AREA_MEASURE", ())
		self.TEXT_POPULATION_MEASURE = ""
		self.TEXT_HAPPINESS_MEASURE = "%"
		self.TEXT_HEALTH_MEASURE = szBullet + localText.getText("TXT_KEY_DEMO_SCREEN_POPULATION_MEASURE", ())
		# advc.077: was IMP_EXP
		self.TEXT_EXP_MEASURE = szBullet + localText.getText("TXT_KEY_DEMO_SCREEN_ECONOMY_MEASURE", ())

		# <!-- custom: move these up from drawTextChart as we don't need to compute them every time if i'm not mistaken, and simplified into one variable (old code deleted). -->
		# New: Append icons, and use different text keys for economy, industry, agriculture.
		# Add measure after a colon (and commented out below)
		self.szEconomyTitle =  localText.getText("TXT_KEY_DEMOGRAPHICS_ECONOMY_TEXT", ()) + (u" (%c+%c)" % (gc.getCommerceInfo(CommerceTypes.COMMERCE_GOLD).getChar(), gc.getCommerceInfo(CommerceTypes.COMMERCE_RESEARCH).getChar())) + ": " + self.TEXT_ECONOMY_MEASURE
		self.szIndustryTitle =  localText.getText("TXT_KEY_DEMOGRAPHICS_INDUSTRY_TEXT", ()) + (u" (%c)" % gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar()) + ": " + self.TEXT_INDUSTRY_MEASURE
		self.szAgricultureTitle = localText.getText("TXT_KEY_DEMOGRAPHICS_AGRICULTURE_TEXT", ()) + (u" (%c)" % gc.getYieldInfo(YieldTypes.YIELD_FOOD).getChar()) + ": " + self.TEXT_AGRICULTURE_MEASURE
		self.szMilitaryTitle =  self.TEXT_MILITARY + (u" (%c)" % CyGame().getSymbolID(FontSymbols.STRENGTH_CHAR))
		self.szHappinessTitle = self.TEXT_HAPPINESS + (u" (%c)" % CyGame().getSymbolID(FontSymbols.HAPPY_CHAR))
		self.szHealthTitle = self.TEXT_HEALTH + (u" (%c)" % CyGame().getSymbolID(FontSymbols.HEALTHY_CHAR)) + ": " + self.TEXT_HEALTH_MEASURE
		self.szExpTitle = self.TEXT_EXP + (u" (%c)" % CyGame().getSymbolID(FontSymbols.TRADE_CHAR)) + ": " + self.TEXT_EXP_MEASURE
		# <!-- custom add char icons there with claude opus 4.5's help thanks. -->
		self.szLandAreaTitle = u"%s (%c): %s" % (self.TEXT_LAND_AREA, CyGame().getSymbolID(FontSymbols.MAP_CHAR), self.TEXT_LAND_AREA_MEASURE)
		self.szPopulationTitle = u"%s (%c)" % (self.TEXT_POPULATION, CyGame().getSymbolID(FontSymbols.CITIZEN_CHAR))

		# <!-- custom: rank buttons for demographics tab, added with claude opus 4.5's help thanks. -->
		szRank1IconPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_TROPHY").getPath()  # 🏆
		szRank2IconPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_2ND_PLACE_MEDAL").getPath()  # 🥈
		szRank3IconPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_3RD_PLACE_MEDAL").getPath()  # 🥉

		# <!-- custom: precompute full image tag strings for efficiency, added with claude opus 4.5's help thanks. -->
		self.szRank1ImgTag = u"<img=%s size=%d></img>" % (szRank1IconPath, self.iRankIconSize)
		self.szRank2ImgTag = u"<img=%s size=%d></img>" % (szRank2IconPath, self.iRankIconSize)
		self.szRank3ImgTag = u"<img=%s size=%d></img>" % (szRank3IconPath, self.iRankIconSize)

		self.TEXT_TIME_PLAYED = localText.getText("TXT_KEY_INFO_SCREEN_TIME_PLAYED", ())
		self.TEXT_CITIES_CURRENT = localText.getText("TXT_KEY_CONCEPT_CITIES", ()) # K-Mod
		self.TEXT_CITIES_BUILT = localText.getText("TXT_KEY_INFO_SCREEN_CITIES_BUILT", ())
		self.TEXT_CITIES_RAZED = localText.getText("TXT_KEY_INFO_SCREEN_CITIES_RAZED", ())
		self.TEXT_NUM_GOLDEN_AGES = localText.getText("TXT_KEY_INFO_SCREEN_NUM_GOLDEN_AGES", ())
		self.TEXT_NUM_RELIGIONS_FOUNDED = localText.getText("TXT_KEY_INFO_SCREEN_RELIGIONS_FOUNDED", ())
		# advc.002b: Text key was TXT_KEY_CURRENT; abbreviate.
		self.TEXT_CURRENT = localText.getText("TXT_KEY_CURRENT_ABBR", ())
		self.TEXT_UNITS = localText.getText("TXT_KEY_CONCEPT_UNITS", ())
		self.TEXT_BUILDINGS = localText.getText("TXT_KEY_CONCEPT_BUILDINGS", ())
		self.TEXT_KILLED = localText.getText("TXT_KEY_INFO_SCREEN_KILLED", ())
		self.TEXT_LOST = localText.getText("TXT_KEY_INFO_SCREEN_LOST", ())
		self.TEXT_BUILT = localText.getText("TXT_KEY_INFO_SCREEN_BUILT", ())
		self.TEXT_IMPROVEMENTS = localText.getText("TXT_KEY_CONCEPT_IMPROVEMENTS", ())

#BUG: Change Graphs - start
		self.SHOW_ALL = sasFontTagLabel + localText.getText("TXT_KEY_SHOW_ALL", ()) + SAS_FONT_TAG_CLOSE
		self.SHOW_NONE = sasFontTagLabel + localText.getText("TXT_KEY_SHOW_NONE", ()) + SAS_FONT_TAG_CLOSE
		self.LOG_SCALE = sasFontTagLabel + localText.getText("TXT_KEY_LOGSCALE", ()) + SAS_FONT_TAG_CLOSE

		sTemp1 = [""] * self.NUM_SCORES
		sTemp2 = [""] * self.NUM_SCORES

		sTemp1[0] = localText.getText("TXT_KEY_GAME_SCORE", ())
		sTemp1[1] = localText.getText("TXT_KEY_DEMO_SCREEN_ECONOMY_TEXT", ())
		sTemp1[2] = localText.getText("TXT_KEY_DEMO_SCREEN_INDUSTRY_TEXT", ())
		sTemp1[3] = localText.getText("TXT_KEY_DEMO_SCREEN_AGRICULTURE_TEXT", ())
		sTemp1[4] = localText.getText("TXT_KEY_POWER", ())
		sTemp1[5] = localText.getObjectText("TXT_KEY_COMMERCE_CULTURE", 0)
		sTemp1[6] = localText.getObjectText("TXT_KEY_ESPIONAGE_CULTURE", 0)

		for i in range(self.NUM_SCORES):
			sTemp2[i] = BugUtil.colorText(sTemp1[i], "COLOR_YELLOW")

		self.sGraphText = []
		self.sGraphText.append(sTemp1)
		self.sGraphText.append(sTemp2)

		# determine the big graph text spacing
		self.X_GRAPH_TEXT = [0] * self.NUM_SCORES

		iW_GRAPH = self.W_SCREEN - 2 * self.X_MARGIN
		iTEXT_W = 0
		for i in range(self.NUM_SCORES):
			iTEXT_W += CyInterface().determineWidth(sTemp1[i])

		iText_Space = max(0, iW_GRAPH - iTEXT_W) / (self.NUM_SCORES - 1)

		for i in range(self.NUM_SCORES):
			if i == 0:
				self.X_GRAPH_TEXT[i] = self.X_MARGIN
				continue

			self.X_GRAPH_TEXT[i] = self.X_GRAPH_TEXT[i - 1] + CyInterface().determineWidth(sTemp1[i - 1]) + iText_Space

		self.BUG_GRAPH_HELP = localText.getText("TXT_KEY_BUG_CHART_HELP", ())
		self.BUG_LEGEND_DEAD = localText.getText("TXT_KEY_BUG_DEAD_CIV", ())
		# <!-- custom: performance optimimization: avoid redundant reuse if i'm not mistaken. -->
		self.BUG_LEGEND_DEAD_SQ_BRACKETED = " [" + self.BUG_LEGEND_DEAD + "]"
#BUG: Change Graphs - end

		# <!-- custom: move these up from drawWondersDropdownBox or such as we don't need to compute them every time if i'm not mistaken, and simplified into one variable (old code replaced). -->
		self.TEXT_WORLD_WONDERS = localText.getText("TXT_KEY_TOP_CITIES_SCREEN_WORLD_WONDERS", ())
		self.TEXT_NATIONAL_WONDERS = localText.getText("TXT_KEY_TOP_CITIES_SCREEN_NATIONAL_WONDERS", ())
		self.TEXT_PROJECTS = localText.getText("TXT_KEY_PEDIA_CATEGORY_PROJECT", ())
		self.szWondersSpecialTitle = sasFontTagLabel.bold + localText.getText("TXT_KEY_PEDIA_SPECIAL_ABILITIES", ()) + SAS_FONT_TAG_CLOSE

		# <!-- custom: moved here from drawWondersList_BUG: performance optimimization: avoid redundant recomputation if i'm not mistaken. -->
		self.sNameWonders = BugUtil.getPlainText("TXT_KEY_WONDER_NAME")
		self.sDateWonders = BugUtil.getPlainText("TXT_KEY_WONDER_DATE")
		self.sOwnerWonders = BugUtil.getPlainText("TXT_KEY_WONDER_OWNER")
		self.sCityWonders = BugUtil.getPlainText("TXT_KEY_WONDER_CITY")

		# world wonder / national wonder / projects icons / buttons
		self.BUGWorldWonder_On = ArtFileMgr.getInterfaceArtInfo("BUG_WORLDWONDER_ON").getPath()
		self.BUGWorldWonder_Off = ArtFileMgr.getInterfaceArtInfo("BUG_WORLDWONDER_OFF").getPath()
		self.BUGNatWonder_On = ArtFileMgr.getInterfaceArtInfo("BUG_NATWONDER_ON").getPath()
		self.BUGNatWonder_Off = ArtFileMgr.getInterfaceArtInfo("BUG_NATWONDER_OFF").getPath()
		self.BUGProject_On = ArtFileMgr.getInterfaceArtInfo("BUG_PROJECT_ON").getPath()
		self.BUGProject_Off = ArtFileMgr.getInterfaceArtInfo("BUG_PROJECT_OFF").getPath()

		# <!-- custom: added with the help of claude opus 4.5 thanks, moved up to not recompute every time if i'm not mistaken. -->
		self.szTimeIconStats = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_HOURGLASS_NOT_DONE").getPath()  # ⏳
		self.szCityIconStats = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTONS_CITYSELECTION").getPath()
		self.szFoundCityIconStats = "Art/Interface/Buttons/Actions/FoundCity.dds"
		self.szRazeIconStats = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_FIRE").getPath()  # 🔥
		self.szReligionIconStats = "Art/Interface/Buttons/General/ConvertReligion.dds"
		self.szGoldenAgeIconStats = "Art/Interface/Buttons/Actions/GoldenAge.dds"

		# <!-- custom: cache deterministic art paths/text used every render in Graph-related UI to avoid repeated lookups. (GPT-5.3-Codex) -->
		self.ART_MAINMENU_SLIDESHOW_LOAD = ArtFileMgr.getInterfaceArtInfo("MAINMENU_SLIDESHOW_LOAD").getPath()
		self.ART_POPUPS_BACKGROUND_TRANSPARENT = ArtFileMgr.getInterfaceArtInfo("POPUPS_BACKGROUND_TRANSPARENT").getPath()
		self.ART_SCREEN_BG = ArtFileMgr.getInterfaceArtInfo("SCREEN_BG").getPath()
		self.ART_INTERFACE_BUTTON_NULL = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTON_NULL").getPath()

		# <!-- custom: not strictly for text but use this to compute cheaply our sas defines once -->
		self.IS_SAS_CV_INFO_SCREEN_TIMELINE_DBG_LOG_PRETTY_SUMMARY_BUTTON_ENABLE = (gc.getDefineINT("SAS_CV_INFO_SCREEN_TIMELINE_LOG_BUTTON_ENABLE") > 0)
		# <!-- custom: cache toggle; when disabled, timeline entries are rebuilt and not stored on the instance. (GPT-5.2-Codex) -->
		self.IS_SAS_CV_INFO_SCREEN_TIMELINE_CACHE_ENABLE = (gc.getDefineINT("SAS_CV_INFO_SCREEN_TIMELINE_CACHE_ENABLE") > 0)
		self.SAS_CV_INFO_SCREEN_SCORE_TAB_MAX_RENDER_THRESHOLD = gc.getDefineINT("SAS_CV_INFO_SCREEN_SCORE_TAB_MAX_RENDER_THRESHOLD")

	def reset(self):

		# City Members

		self.szCityNames = ["", "", "", "", ""]
		self.iCitySizes = [-1, -1, -1, -1, -1]
		self.szCityDescs = ["", "", "", "", ""]
		self.aaCitiesXY = [[-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1]]
		self.iCityValues = [0, 0, 0, 0, 0]
		self.pCityPointers = [0, 0, 0, 0, 0]

#		self.bShowAllPlayers = false
		self.graphEnd = CyGame().getGameTurn() - 1
		self.graphZoom = self.graphEnd - CyGame().getStartTurn()
		self.iShowingPlayer = -1

		for t in self.RANGE_SCORES:
			self.scoreCache[t]	= None

	def resetWonders(self):

		self.szWonderDisplayMode = self.szWDM_WorldWonder

		self.iWonderID = -1			# BuildingType ID of the active wonder, e.g. Palace is 0, Globe Theater is 66
		self.iActiveWonderCounter = 0		# Screen ID for this wonder (0, 1, 2, etc.) - different from the above variable

		self.aiWonderListBoxIDs = []
		self.aiTurnYearBuilt = []
		self.aiWonderBuiltBy = []
		self.aszWonderCity = []

	def getScreen(self):
		return CyGInterfaceScreen(self.DEMO_SCREEN_NAME, self.screenId)

	def hideScreen(self):
		screen = self.getScreen()
		screen.hideScreen()

	def updateRuntimeLayout(self, screen):
		# <!-- custom: compute Info Screen geometry at runtime from the current resolution; this matches Foreign Advisor's pattern and avoids stale init-time layout. (GPT-5.3-Codex) -->
		self.X_SCREEN, self.Y_SCREEN, self.W_SCREEN, self.H_SCREEN = getAdvisorRuntimeBounds(screen, self.W_LEFT_SPACE_FOR_COMMERCE_SLIDERS, self.W_RIGHT_SPACE_FOR_SCOREBOARD, self.H_TOP_SPACE_FOR_TECH_BAR, self.H_BOTTOM_SPACE)
		self.X_TITLE, self.X_EXIT, self.Y_EXIT, self.Y_LINK, _ = getAdvisorRuntimeAnchors(self.W_SCREEN, self.H_SCREEN)
		self.Y_TITLE = SAS_ADVISOR_TITLE_Y

		# <!-- custom: margins/constants are initialized once in __init__; runtime method computes only values that depend on current screen bounds. (GPT-5.3-Codex) -->

# GRAPH

		# <!-- custom: add "_GRAPH_" to the name to not confuse it with the "_WONDERS_" dropdown and avoid future mistakes if i'm not mistaken. -->
		# <!-- custom: upscale this tab and make coordinates more dynamic with the help of chatgpt 5.2 -->
		self.X_DEMO_DROPDOWN = self.X_MARGIN
		self.Y_DEMO_DROPDOWN = self.Y_MARGIN

		self.X_ZOOM_DROPDOWN = self.X_DEMO_DROPDOWN
		self.Y_ZOOM_DROPDOWN = self.Y_DEMO_DROPDOWN + self.H_GRAPH_DROPDOWN

		self.W_ZOOM_DROPDOWN = self.W_DEMO_DROPDOWN

		self.X_LEGEND = self.X_DEMO_DROPDOWN
		self.Y_LEGEND = self.Y_ZOOM_DROPDOWN + self.H_GRAPH_DROPDOWN + 3
		# <!-- custom: we widened the left control column for legend readability at larger label fonts (3+); keep dropdown width matched to this same column so both stay aligned.
		# In AdvCiv-SAS, most advisors were expanded to use more of the screen, so plenty width still remains for the graph even after this legend increase. (GPT-5.3-Codex) -->
		self.W_LEGEND = self.W_DEMO_DROPDOWN
		#self.H_LEGEND = 200	this is computed from the number of players

		self.X_GRAPH = self.X_LEGEND + self.W_LEGEND + 10
		self.Y_GRAPH = self.Y_MARGIN
		self.W_GRAPH = self.W_SCREEN - self.X_GRAPH - self.X_MARGIN
		self.H_GRAPH = self.H_SCREEN - self.Y_GRAPH - 98

		# Timeline tab layout - vertically centered between header and footer panels
		# Content area boundaries (between the two panels)
		self.CONTENT_Y_TOP = self.PANEL_HEIGHT  # Header panel bottom
		self.CONTENT_Y_BOTTOM = self.H_SCREEN - self.PANEL_HEIGHT  # Footer panel top

		# Table positioning - truly centered
		self.X_TIMELINE_TABLE = self.X_MARGIN
		self.Y_TIMELINE_TABLE = self.CONTENT_Y_TOP + self.TIMELINE_TABLE_VERTICAL_GAP
		self.W_TIMELINE_TABLE = self.W_SCREEN - (2 * self.X_MARGIN)
		self.H_TIMELINE_TABLE = (self.CONTENT_Y_BOTTOM - self.CONTENT_Y_TOP) - (2 * self.TIMELINE_TABLE_VERTICAL_GAP)

		# <!-- custom: size the Timeline LOG button after game defines are initialized; doing this in __init__ queried SAS_UI_FONT_LABEL too early during CvScreensInterface startup and logged a false invalid-define warning. (GPT-5.5) -->
		# <!-- custom: previous error shown for reference in PythonDbg.log (now fixed by moving this from init to updateRuntimeLayout) -->
		#
		# load_module CvStrategyOverlay
		# load_module SASDefineGuard
		# SASFontUtils: invalid or unavailable SAS_UI_FONT_LABEL=0; using default 3
		# init-ing world builder screen
		# load_module CvWBPopups
		#
		# load_module CvCameraControls
		if getSASUIFontLabel() > 3:
			self.W_TIMELINE_TABLE_LOG_BUTTON = 66
			self.H_TIMELINE_TABLE_LOG_BUTTON = 32
		else:
			self.W_TIMELINE_TABLE_LOG_BUTTON = 50
			self.H_TIMELINE_TABLE_LOG_BUTTON = 28

		# LOG button - positioned in top header bar (0-55px height)
		self.X_TIMELINE_TABLE_LOG_BUTTON = self.W_SCREEN - self.W_TIMELINE_TABLE_LOG_BUTTON - 50

		self.X_LEFT_BUTTON = self.X_GRAPH
		self.Y_LEFT_BUTTON = self.Y_GRAPH + self.H_GRAPH

		self.W_RIGHT_BUTTON = self.W_LEFT_BUTTON
		self.H_RIGHT_BUTTON = self.H_LEFT_BUTTON
		self.X_RIGHT_BUTTON = self.X_GRAPH + self.W_GRAPH - self.W_RIGHT_BUTTON
		self.Y_RIGHT_BUTTON = self.Y_LEFT_BUTTON

		self.X_LEFT_LABEL = self.X_LEFT_BUTTON + self.W_LEFT_BUTTON + 10
		self.X_RIGHT_LABEL = self.X_RIGHT_BUTTON - 10
		self.Y_LABEL = self.Y_GRAPH + self.H_GRAPH + 3

#BUG: Change Graphs - start
		self.Graph_Status_Current = self.Graph_Status_1in1
		self.Graph_Status_Prior = self.Graph_Status_7in1
#		self.BIG_GRAPH = False

# the 7-in-1 graphs are layout out as follows:
#    0 1 2
#    L 3 4
#    L 5 6
#
# where L is the legend and a number represents a graph

# the 3-in-1 graphs are layout out as follows:
#    0 1
#    L 2
#
# where L is the legend and a number represents a graph
#BUG: Change Graphs - end

		# DEMOGRAPHICS

		# <!-- custom: rename self.X_CHART and such to self.X_DEMOGRAPHICS_CHART and such for clarity and most importantly to avoid future errors. -->
		# <!-- custom: note: deleted seemingly unused self.BUTTON_SIZE if i'm not mistaken. -->

		# <!-- custom: upscale this tab and make coordinates more dynamic with the help of chatgpt 5.2 -->
		self.X_DEMOGRAPHICS_CHART = self.X_MARGIN
		self.Y_DEMOGRAPHICS_CHART = self.Y_MARGIN
		self.W_DEMOGRAPHICS_CHART = self.W_SCREEN - 2 * self.X_DEMOGRAPHICS_CHART
		self.H_DEMOGRAPHICS_CHART = self.H_SCREEN - 2 * self.Y_DEMOGRAPHICS_CHART

		self.bAbleToShowAllPlayers = false
		self.iShowingPlayer = -1
		self.aiDropdownPlayerIDs = []

		# TOP CITIES

		# <!-- custom: self.W_WONDERS_RIGHT_PANE moved to top cities since it uses it to compute its dimensions. -->
		# <!-- custom: gap between the panels as noted by gemini 3 pro and chatgpt 5.2 before it thanks. -->
		self.W_WONDERS_INTER_PANE_GAP = self.X_MARGIN

		# <!-- custom: calculate wonder panel height based on rows, added with claude opus 4.5's help thanks. -->
		self.H_CITIES_WONDER_PANEL = (self.iTopCitiesWonderNumRows * self.iTopCitiesWonderButtonSize) + (2 * self.iTopCitiesWonderPanelMargin) + self.iTopCitiesWonderPanelHeightAdjust

		# <!-- custom: total height of right side (city desc + gap + wonder panel), added with claude opus 4.5's help thanks. -->
		self.iTopCitiesRightSideHeight = self.H_CITIES_DESC + self.iTopCitiesDescWonderGap + self.H_CITIES_WONDER_PANEL

		# <!-- custom: in many of these variable names, add or rename to "_TOP_CITIES_" in the name for clarity and to avoid future mistakes if i'm not mistaken. -->
		self.X_TOP_CITIES_LEFT_PANE = self.W_WONDERS_INTER_PANE_GAP
		self.Y_TOP_CITIES_LEFT_PANE = self.Y_MARGIN
		self.W_TOP_CITIES_LEFT_PANE = self.W_SCREEN - (2 * self.X_TOP_CITIES_LEFT_PANE) - self.W_WONDERS_INTER_PANE_GAP - self.W_WONDERS_RIGHT_PANE
		# <!-- custom: mirror wonders panel height (H_SCREEN - 2 * Y_MARGIN), added with claude opus 4.5's help thanks. -->
		self.H_TOP_CITIES_LEFT_PANE = self.H_SCREEN - 2 * self.Y_MARGIN

		# Animated City thingies
		self.X_CITY_ANIMATION = self.X_TOP_CITIES_LEFT_PANE + 20
		self.Z_CITY_ANIMATION = self.Z_BACKGROUND - 0.5
		# <!-- custom: match city animation height with right side panels, adjusted by trial, added with claude opus 4.5's help thanks. -->
		self.H_CITY_ANIMATION = self.iTopCitiesRightSideHeight - 10
		# <!-- custom: Y buffer adjusted to align top of animation with top of city desc panel, added with claude opus 4.5's help thanks. -->
		self.Y_CITY_ANIMATION_BUFFER = self.H_CITY_ANIMATION / 2 - 3

		# <!-- custom: dynamically compute Y_CITIES_BUFFER based on component heights, added with claude opus 4.5's help thanks. -->
		# Total height per city = right side height + inter-city margin
		self.Y_CITIES_BUFFER = self.iTopCitiesRightSideHeight + self.iTopCitiesInterCityMargin

		# Placement of Cities
		self.X_COL_1_CITIES = self.X_TOP_CITIES_LEFT_PANE + 20

		self.Y_ROWS_CITIES = []
		# <!-- custom: add header height offset so cities start below the header, added with claude opus 4.5's help thanks. -->
		self.Y_ROWS_CITIES.append(self.Y_TOP_CITIES_LEFT_PANE + 20 + self.iTopCitiesHeaderHeight)
		for i in range(1, 5):
			self.Y_ROWS_CITIES.append(self.Y_ROWS_CITIES[i-1] + self.Y_CITIES_BUFFER)

		# <!-- custom: right part of the top cities panel if i'm not mistaken. -->
		self.X_COL_1_CITIES_DESC = self.X_TOP_CITIES_LEFT_PANE + self.W_CITY_ANIMATION + 30 + self.iTopCitiesInterLeftRightPartsGapAdjust
		self.W_CITIES_DESC = self.W_TOP_CITIES_LEFT_PANE - self.W_CITY_ANIMATION - 30 + self.iTopCitiesRightPartWAdjust - self.iTopCitiesInterLeftRightPartsGapAdjust

		# <!-- custom: wonder panel Y offset from city desc panel, added with claude opus 4.5's help thanks. -->
		self.Y_CITIES_WONDER_BUFFER = self.H_CITIES_DESC + self.iTopCitiesDescWonderGap

		# WONDERS
		self.X_WONDERS_RIGHT_PANE = self.X_TOP_CITIES_LEFT_PANE + self.W_TOP_CITIES_LEFT_PANE + self.W_WONDERS_INTER_PANE_GAP
		self.Y_WONDERS_RIGHT_PANE = self.Y_MARGIN
		self.H_WONDERS_RIGHT_PANE = self.H_SCREEN - 2 * self.Y_WONDERS_RIGHT_PANE

		self.X_WONDERS_CHART = self.X_WONDERS_RIGHT_PANE + self.SMALL_MARGIN
		self.Y_WONDERS_CHART = self.Y_WONDERS_RIGHT_PANE + 60
		self.W_WONDERS_CHART = self.W_WONDERS_RIGHT_PANE - (2 * self.SMALL_MARGIN)
		self.H_WONDERS_CHART = self.H_WONDERS_RIGHT_PANE - self.Y_WONDERS_RIGHT_PANE

		totalWondersCharMostColsW = self.W_WONDERS_CHART_COL_BUTTON + self.W_WONDERS_CHART_COL_DATE + self.W_WONDERS_CHART_COL_OWNER + self.W_WONDERS_CHART_COL_CITY
		self.W_WONDERS_CHART_COL_NAME = self.W_WONDERS_CHART - totalWondersCharMostColsW

		self.X_WONDERS_STATS_PANE = self.X_WONDERS_RIGHT_PANE + 20
		self.Y_WONDERS_STATS_PANE = self.Y_WONDERS_RIGHT_PANE + 20

		self.X_WONDERS_DROPDOWN = self.X_WONDERS_RIGHT_PANE + 20
		self.Y_WONDERS_DROPDOWN = self.Y_WONDERS_RIGHT_PANE + 20

		self.X_WONDER_LIST = self.X_WONDERS_RIGHT_PANE + 240 + (2 * self.WONDERS_FUDGE_FACTOR)
		self.Y_WONDER_LIST = self.Y_WONDERS_RIGHT_PANE + 60
		self.W_WONDER_LIST = self.W_WONDER_LIST_BASE - (2 * self.WONDERS_FUDGE_FACTOR)

		self.Y_WONDER_GRAPHIC = self.Y_WONDERS_RIGHT_PANE + 20 + 200 + 35

		self.X_PROJECT_ICON = self.X_WONDER_GRAPHIC + self.W_WONDER_GRAPHIC / 2
		self.Y_PROJECT_ICON = self.Y_WONDER_GRAPHIC + self.H_WONDER_GRAPHIC / 2

		self.szWDM_WorldWonder = "World Wonders"
		self.szWDM_NatnlWonder = "National Wonders"
		self.szWDM_Project = "Projects"

		self.BUGWorldWonderWidget = self.szWDM_WorldWonder + "Widget"
		self.BUGNatWonderWidget = self.szWDM_NatnlWonder + "Widget"
		self.BUGProjectWidget = self.szWDM_Project + "Widget"

		self.szWonderDisplayMode = self.szWDM_WorldWonder

		self.iWonderID = -1 			# BuildingType ID of the active wonder, e.g. Palace is 0, Globe Theater is 66
		self.iActiveWonderCounter = 0	# Screen ID for this wonder (0, 1, 2, etc.) - different from the above variable

	# STATISTICS
		self.X_STATS_BOTTOM_CHART = self.X_MARGIN
		iTotalStatsTabBottomW = self.W_SCREEN - 2 * self.X_STATS_BOTTOM_CHART
		# <!-- custom: Improvements column removed; units/buildings split the full bottom width 455:260, preserving their previous ratio. (Claude code Opus 4.7) -->
		self.W_STATS_BOTTOM_CHART_UNITS = int(iTotalStatsTabBottomW * 455 / 715)
		self.W_STATS_BOTTOM_CHART_BUILDINGS = iTotalStatsTabBottomW - self.W_STATS_BOTTOM_CHART_UNITS
		self.H_STATS_BOTTOM_CHART = self.H_SCREEN - self.Y_STATS_BOTTOM_CHART - 78

		self.X_STATS_TOP_PANEL = self.X_MARGIN
		self.Y_STATS_TOP_PANEL = self.Y_MARGIN
		self.W_STATS_TOP_PANEL = self.W_SCREEN - 2 * self.X_STATS_TOP_PANEL

		self.X_LEADER_ICON = self.X_STATS_TOP_PANEL

		self.X_STATS_TOP_CHART = self.X_LEADER_ICON + self.X_MARGIN + self.W_LEADER_ICON
		self.Y_STATS_TOP_CHART = self.Y_LEADER_ICON
		self.W_STATS_TOP_CHART = self.W_STATS_BOTTOM_CHART_UNITS - self.X_STATS_TOP_CHART + self.iExtraWTopChartValue
		self.H_STATS_TOP_CHART = self.H_LEADER_ICON

		self.STATS_TOP_CHART_W_COL_0 = self.W_STATS_TOP_CHART - self.STATS_TOP_CHART_W_COL_1

		self.X_LEADER_NAME = self.X_LEADER_ICON
		self.Y_LEADER_NAME = self.Y_STATS_TOP_CHART - 40

	def getLastTurn(self):
		return (gc.getGame().getReplayMessageTurn(gc.getGame().getNumReplayMessages()-1))

	# Screen construction function
	def showScreen(self, iTurn, iTabID, iEndGame):

#BUG Timer
		self.timer = BugUtil.Timer("InfoScreen")

		self.iStartTurn = 0
		for iI in range(gc.getGameSpeedInfo(gc.getGame().getGameSpeedType()).getNumTurnIncrements()):
			self.iStartTurn += gc.getGameSpeedInfo(gc.getGame().getGameSpeedType()).getGameTurnInfo(iI).iNumGameTurnsPerIncrement
		self.iStartTurn *= gc.getEraInfo(gc.getGame().getStartEra()).getStartPercent()
		self.iStartTurn /= 100

		self.iTurn = 0

		if (iTurn > self.getLastTurn()):
			return

		screen = self.getScreen()
		if screen.isActive():
			return
		self.updateRuntimeLayout(screen)
		self.initText()

		# Create a new screen
		screen.setRenderInterfaceOnly(True)
		screen.showScreen(PopupStates.POPUPSTATE_IMMEDIATE, False)
		# <!-- custom: Match Tech Chooser/Military Advisor persistent-screen pattern so heavy widgets such as the Timeline table can survive Info Screen close/reopen. (GPT-5.5) -->
		screen.setPersistent(True)

		self.reset()

		self.deleteAllWidgets()

		# Set the background widget and exit button
		screen.addDDSGFC("DemographicsScreenBackground", self.ART_MAINMENU_SLIDESHOW_LOAD, 0, 0, self.W_SCREEN, self.H_SCREEN, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		screen.addPanel( "TechTopPanel", u"", u"", True, False, 0, 0, self.W_SCREEN, self.PANEL_HEIGHT, PanelStyles.PANEL_STYLE_TOPBAR )
		# <!-- custom: in the foreign advisor and similar screens, too many players do not fit in one view, and the window does not use the full game window space.
		# Make it larger like Sevopedia to reduce scrolling. Credit: Gemini 3 Pro; fixes reviewed by Claude Sonnet 4.5. (GPT-5.2-Codex (summarized)) -->
		# Top panels cutting off content: The TopPanel and BottomPanel are positioned at y=0 and y=713 respectively. These need updating:
		# screen.addPanel( "TechBottomPanel", u"", u"", True, False, 0, 713, self.W_SCREEN, 55, PanelStyles.PANEL_STYLE_BOTTOMBAR )
		# screen.showWindowBackground( False )
		screen.addPanel( "TechBottomPanel", u"", u"", True, False, 0, self.H_SCREEN - self.PANEL_HEIGHT, self.W_SCREEN, self.PANEL_HEIGHT, PanelStyles.PANEL_STYLE_BOTTOMBAR )
		screen.showWindowBackground( False )

		# <!-- custom: unlike the foreign advisor reuse, we must change the center settings here or the screen stays centered.
		# With this change we can move the military advisor screen around the window. Credit: Gemini 3 Pro. (GPT-5.2-Codex (summarized)) -->
		# screen.setDimensions(screen.centerX(self.X_SCREEN), screen.centerY(self.Y_SCREEN), self.W_SCREEN, self.H_SCREEN)
		screen.setDimensions(self.X_SCREEN, self.Y_SCREEN, self.W_SCREEN, self.H_SCREEN)

		self.szExitButtonName = self.getNextWidgetName()
		screen.setText(self.szExitButtonName, "Background", self.EXIT_TEXT, CvUtil.FONT_RIGHT_JUSTIFY, self.X_EXIT, self.Y_EXIT, self.Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		# Header...
		self.szHeaderWidget = self.getNextWidgetName()
		screen.setLabel(self.szHeaderWidget, "Background", self.SCREEN_TITLE, CvUtil.FONT_CENTER_JUSTIFY, self.X_TITLE, self.Y_TITLE, self.Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# Help area for tooltips
		screen.setHelpTextArea(self.W_HELP_AREA, FontTypes.SMALL_FONT, self.X_SCREEN, self.Y_SCREEN, self.Z_HELP_AREA, 1, self.ART_POPUPS_BACKGROUND_TRANSPARENT, True, True, CvUtil.FONT_LEFT_JUSTIFY, 0 )

		self.DEBUG_DROPDOWN_ID = "InfoScreenDropdownWidget"
		self.szDropdownName = self.DEBUG_DROPDOWN_ID
		if not hasattr(self, "iActivePlayer"):
			self.iActivePlayer = CyGame().getActivePlayer()
		self.iActivePlayer = getAdvisorValidPerspectivePlayer(self.iActivePlayer, bAllowVassalPerspective=True)
		# <!-- custom: preserve the selected debug/vassal perspective when reopening the advisor; bSelectActive=False rebuilt the dropdown with no selected row in debug mode. (GPT-5.5) -->
		if not addAdvisorDebugDropdown(screen, self.szDropdownName, self.iActivePlayer, bAllowVassalPerspective=True): # advc.007: was MAX_PLAYERS
			self.DEBUG_DROPDOWN_ID = ""

		self.iActivePlayer = getAdvisorValidPerspectivePlayer(self.iActivePlayer, bAllowVassalPerspective=True)
		self.pActivePlayer = gc.getPlayer(self.iActivePlayer)
		self.iActiveTeam = self.pActivePlayer.getTeam()
		self.pActiveTeam = gc.getTeam(self.iActiveTeam)
		# advc.007: Don't reveal all when perspective switched (I guess this is possible when the game ends while in Debug mode)	
		self.bRevealAll = (self.iActiveTeam == gc.getGame().getActiveTeam() and (CyGame().isDebugMode() or CyGame().getGameState() == GameStateTypes.GAMESTATE_OVER)) # advc.077

		self.iInvestigateCityMission = -1
		# See if Espionage allows graph to be shown for each player
		if GameUtil.isEspionage():
			for iMissionLoop in range(gc.getNumEspionageMissionInfos()):
				if (gc.getEspionageMissionInfo(iMissionLoop).isInvestigateCity()):
					self.iInvestigateCityMission = iMissionLoop
					break

		self.determineKnownPlayers(iEndGame)

		# "Save" current widgets so they won't be deleted later when changing tabs
		self.iNumPermanentWidgets = self.nWidgetCount

		# Reset variables
		self.graphEnd	= CyGame().getGameTurn() - 1
		self.graphZoom	= self.graphEnd - CyGame().getStartTurn()

#		self.iActiveTab = iTabID
		if self.iActiveTab == -1 or self.iActiveTab >= len(self.PAGE_NAME_LIST):
			self.iActiveTab = self.iGraphID

		if (self.iNumPlayersMet > 1):
			self.iShowingPlayer = 666#self.iActivePlayer
		else:
			self.iShowingPlayer = self.iActivePlayer

		self.redrawContents()

		return

	def redrawContents(self):

		screen = self.getScreen()
		self.deleteAllWidgets(self.iNumPermanentWidgets)
		self.iNumWondersPermanentWidgets = 0

		# Draw Tab buttons and tabs. (rewriten for K-Mod)
		aszTabLabels = []
		aszTabWidgetIDs = []
		for i in range(len(self.PAGE_NAME_LIST)):
			aszTabLabels.append(localText.getText(self.PAGE_NAME_LIST[i], ()).upper())
			aszTabWidgetIDs.append("InfoTabButton" + str(i))
		drawAdvisorFooterTabs(screen, aszTabWidgetIDs, aszTabLabels, self.PAGE_LINK_WIDTH, self.iActiveTab, self.Y_LINK, 0, self.COLOR_YELLOW)

		if (self.iActiveTab >= 0 and self.iActiveTab < len(self.PAGE_NAME_LIST)):
			self.PAGE_DRAW_LIST[self.iActiveTab]()
		#

	# GRAPH

	def drawGraphTab(self):

#BUG: Change Graphs - start
		if self.iGraphTabID == -1:
			self.iGraphTabID = self.TOTAL_SCORE
			self.iGraph_Smoothing_1in1 = 0
			self.iGraph_Smoothing_7in1 = 0
			self.bPlayerInclude = [True] * gc.getMAX_CIV_PLAYERS()
#BUG: Change Graphs - end

		self.drawPermanentGraphWidgets()
		self.drawGraphs()

	# TIMELINE

	def getReligionFoundingTimelineTexts(self):
		# <!-- custom: Precompute localized religion-founding prefixes/suffixes once per Timeline rebuild; classification must remain independent of current plot visibility because a razed hidden holy-city plot can be revealed later. See KI#300. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		szCitySentinel = u"__SAS_TIMELINE_HOLY_CITY__"
		aFoundingTexts = []
		for iReligion in range(gc.getNumReligionInfos()):
			szReligion = gc.getReligionInfo(iReligion).getDescription()
			szKnown = localText.getText("TXT_KEY_MISC_REL_FOUNDED", (szReligion, szCitySentinel))
			iSentinel = szKnown.find(szCitySentinel)
			if iSentinel == -1:
				continue
			aFoundingTexts.append((szKnown[:iSentinel], szKnown[iSentinel + len(szCitySentinel):], localText.getText("TXT_KEY_MISC_REL_FOUNDED_UNKNOWN", (szReligion,))))
		return aFoundingTexts

	def getHiddenReligionFoundedText(self, szReplayText, aFoundingTexts):
		# <!-- custom: Match the localized founding template through its sentinel-derived parts rather than an English substring or the current city object's current name. See KI#300. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		for szPrefix, szSuffix, szUnknown in aFoundingTexts:
			if szReplayText.startswith(szPrefix) and szReplayText.endswith(szSuffix):
				return szUnknown
		return None

	# <!-- custom: Timeline cache - builds cached entries for faster tab loading (Claude Opus 4.5) -->
	def buildTimelineCache(self, bForceRebuild = False):
		# <!-- custom: A non-null global replay snapshot does not expose which observer's historical audience mask built it.
		# Always construct Timeline data for the selected advisor perspective, or reveal all only in that explicit mode.
		# This preserves KI#337 across perspective switches. See KI#1060. (GPT-5.6-Sol) -->
		replayInfo = CyReplayInfo()
		iReplayPlayer = self.iActivePlayer
		if self.bRevealAll:
			iReplayPlayer = -1
		replayInfo.createInfo(iReplayPlayer)

		iNumMessages = replayInfo.getNumReplayMessages()
		iGameTurn = CyGame().getGameTurn()

		if self.IS_SAS_CV_INFO_SCREEN_TIMELINE_CACHE_ENABLE:
			# Check if cache is still valid
			# <!-- custom: Include game turn so spoiler-filtered Timeline entries can refresh later even when replay-message count is unchanged; tested from turn 0, where "founded in an unknown city" later updated to a known city name, while same-turn reuse remains intact. (GPT-5.5) -->
			if not bForceRebuild and self.timelineCacheEntries is not None and self.timelineCacheActivePlayer == self.iActivePlayer and self.timelineCacheBRevealAll == self.bRevealAll and self.timelineCacheNumMessages == iNumMessages and self.timelineCacheGameTurn == iGameTurn:
				# Cache is valid, no rebuild needed
				return self.timelineCacheEntries

		# Rebuild cache
		entries = []

		if iNumMessages <= 0:
			if self.IS_SAS_CV_INFO_SCREEN_TIMELINE_CACHE_ENABLE:
				self.timelineCacheEntries = entries
				self.timelineCacheActivePlayer = self.iActivePlayer
				self.timelineCacheBRevealAll = self.bRevealAll
				self.timelineCacheNumMessages = iNumMessages
				self.timelineCacheGameTurn = iGameTurn
			return entries

		# Build unknown colors dict once
		dUnknownColors = {}
		if not self.bRevealAll:
			for iPlayer in range(gc.getMAX_CIV_PLAYERS()):
				pLoopPlayer = gc.getPlayer(iPlayer)
				if pLoopPlayer.isEverAlive():
					if not self.pActiveTeam.isHasMet(pLoopPlayer.getTeam()):
						eColor = replayInfo.getColor(iPlayer)
						if eColor != -1:
							dUnknownColors[eColor] = True

		# Cache game text manager and map references
		gameTextMgr = CyGameTextMgr()
		cyMap = CyMap()
		iCalendar = replayInfo.getCalendar()
		iStartYear = replayInfo.getStartYear()
		iGameSpeed = replayInfo.getGameSpeed()
		iBarbarianPlayer = gc.getBARBARIAN_PLAYER()
		iCityFoundedMessage = getattr(ReplayMessageTypes, "REPLAY_MESSAGE_CITY_FOUNDED", -1)
		if self.bRevealAll:
			aReligionFoundingTexts = []
		else:
			aReligionFoundingTexts = self.getReligionFoundingTimelineTexts()

		# Iterate backwards (newest first) so no reverse needed later
		for iMessage in range(iNumMessages - 1, -1, -1):
			iPlayer = replayInfo.getReplayMessagePlayer(iMessage)
			iX = replayInfo.getReplayMessagePlotX(iMessage)
			iY = replayInfo.getReplayMessagePlotY(iMessage)
			eColor = replayInfo.getReplayMessageColor(iMessage)
			eMessageType = replayInfo.getReplayMessageType(iMessage)
			bAllowHiddenPlotMessage = False
			szHiddenReligionText = None
			szText = None
			szTextNoColor = None

			bPlotHidden = False
			pPlot = None
			if iX > -1 and iY > -1:
				pPlot = cyMap.plot(iX, iY)
				if pPlot is not None:
					if not pPlot.isRevealed(self.iActiveTeam, False):
						bPlotHidden = True

			bHideCityFounded = False
			if eMessageType == iCityFoundedMessage:
				if iX == -1 or iY == -1 or pPlot is None:
					bHideCityFounded = True
				else:
					iRevealedOwner = pPlot.getRevealedOwner(self.iActiveTeam, False)
					if iRevealedOwner != iPlayer:
						bHideCityFounded = True
			# <!-- custom: hide "city founded" unless revealed owner matches the replay message player; this avoids barbarian spoilers when the plot is revealed but ownership is not.
			# Finding a reliable barbarian hide required checking revealed owner rather than plot visibility alone; optional debug line left for future verification, not tested with this exact code path. (GPT-5.2-Codex); Long_Comments_py.txt #13 -->
			# <!-- custom: Classify founding messages even when the plot is now visible; a razed holy city has no current object from which to infer whether its historical replay name was known. See KI#300. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			if not self.bRevealAll:
				szText = replayInfo.getReplayMessageText(iMessage)
				if szText:
					szTextNoColor = self.RE_COLOR_CLOSE.sub("", self.RE_COLOR_OPEN.sub("", szText))
					szHiddenReligionText = self.getHiddenReligionFoundedText(szTextNoColor, aReligionFoundingTexts)
					if szHiddenReligionText is not None:
						bAllowHiddenPlotMessage = True

			bHide = False
			if not self.bRevealAll:
				if bHideCityFounded:
					bHide = True
				if (not bAllowHiddenPlotMessage and iPlayer == iBarbarianPlayer and (bPlotHidden or iX == -1 or iY == -1)):
					bHide = True
				if not bAllowHiddenPlotMessage and bPlotHidden:
					bHide = True
				if iPlayer != -1:
					pLoopPlayer = gc.getPlayer(iPlayer)
					if pLoopPlayer.isEverAlive():
						if not self.pActiveTeam.isHasMet(pLoopPlayer.getTeam()):
							bHide = True
				if not bHide and iX > -1 and iY > -1:
					iOwner = pPlot.getOwner()
					if iOwner != -1:
						pOwner = gc.getPlayer(iOwner)
						if pOwner.isEverAlive():
							if not self.pActiveTeam.isHasMet(pOwner.getTeam()):
								bHide = True
				if not bHide and eColor in dUnknownColors and not bAllowHiddenPlotMessage:
					bHide = True

			if bHide:
				continue

			if szText is None:
				szText = replayInfo.getReplayMessageText(iMessage)
			if szText == "":
				continue

			iTurnMsg = replayInfo.getReplayMessageTurn(iMessage)
			szEventDate = gameTextMgr.getDateStr(iTurnMsg, False, iCalendar, iStartYear, iGameSpeed)

			# Use precompiled regex patterns
			if szTextNoColor is None:
				szTextNoColor = self.RE_COLOR_CLOSE.sub("", self.RE_COLOR_OPEN.sub("", szText))
			szText = szTextNoColor

			# <!-- custom: CyPlot.getPlotCity returns a managed CyCity wrapper even when its native city pointer is null, so Python identity alone did not recognize a razed holy city.
			# Resolve the wrapper once and use isNone before any city access; this completes the earlier hidden-founding repair after a razed holy-city plot becomes visible. See KI#300. (GPT-5.6-Sol) -->
			pCity = None
			if not self.bRevealAll and pPlot is not None:
				pCity = pPlot.getPlotCity()
			bCityMissing = pCity is None or pCity.isNone()
			if not self.bRevealAll and szHiddenReligionText is not None and (bPlotHidden or bCityMissing):
				# <!-- custom: A hidden religion founding remains globally visible, but its replay text must not reveal a historical holy-city name merely because the city was razed before the observer learned it; use the localized unknown-location line identified above. See KI#300. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				szText = szHiddenReligionText
			elif not self.bRevealAll and not bCityMissing:
				bCityOwnerMet = False
				iCityOwner = pCity.getOwner()
				if iCityOwner != -1:
					pCityOwner = gc.getPlayer(iCityOwner)
					if pCityOwner.isEverAlive():
						if self.pActiveTeam.isHasMet(pCityOwner.getTeam()):
							bCityOwnerMet = True
				if not bCityOwnerMet or bPlotHidden:
					szCityName = pCity.getName()
					if szCityName:
						szText = szText.replace(szCityName, self.TEXT_TIMELINE_UNKNOWN_CITY)

			szFormattedText = localText.changeTextColor(sasFontTagLabel + szEventDate + u": " + szText + SAS_FONT_TAG_CLOSE, eColor)
			entries.append(szFormattedText)

		if self.IS_SAS_CV_INFO_SCREEN_TIMELINE_CACHE_ENABLE:
			# Store in instance vars (faster than dict)
			self.timelineCacheEntries = entries
			self.timelineCacheActivePlayer = self.iActivePlayer
			self.timelineCacheBRevealAll = self.bRevealAll
			self.timelineCacheNumMessages = iNumMessages
			self.timelineCacheGameTurn = iGameTurn
		return entries

	def drawTimelineTab(self):
		screen = self.getScreen()
		aEntries = self.buildTimelineCache(not self.IS_SAS_CV_INFO_SCREEN_TIMELINE_CACHE_ENABLE)
		timelineTableCacheKey = self.getTimelineTableCacheKey(aEntries)

		if self.IS_SAS_CV_INFO_SCREEN_TIMELINE_DBG_LOG_PRETTY_SUMMARY_BUTTON_ENABLE:
			self.szTimelineDbgLogPrettySummaryButton = self.getNextWidgetName()
			szLabel = sasFontTagLabel + self.TEXT_TIMELINE_DBG_LOG_PRETTY_SUMMARY_BUTTON.upper() + SAS_FONT_TAG_CLOSE
			screen.setButtonGFC(self.szTimelineDbgLogPrettySummaryButton, szLabel, "", self.X_TIMELINE_TABLE_LOG_BUTTON, self.Y_TIMELINE_TABLE_LOG_BUTTON, self.W_TIMELINE_TABLE_LOG_BUTTON, self.H_TIMELINE_TABLE_LOG_BUTTON, WidgetTypes.WIDGET_GENERAL, 1, -1, ButtonStyles.BUTTON_STYLE_STANDARD)

		# <!-- custom: Timeline entry caching avoids replay parsing, but rebuilding hundreds of rows was still slow.
		# Preserving the old ListBox helped tab switches, but not Info Screen close/reopen; convert it to a one-column table so we can verify/reuse it with getTableNumRows like Military Advisor's Battles table.
		# Rebuild when player/reveal/turn/language/layout/history signature changes so spoiler filtering can refresh at turn boundaries. (GPT-5.5) -->
		if self.IS_SAS_CV_INFO_SCREEN_TIMELINE_CACHE_ENABLE and self.TIMELINE_TABLE_CACHE_KEY == timelineTableCacheKey and self.canReuseTimelineTableWidget(screen, aEntries):
			screen.show(self.TIMELINE_TABLE_ID)
			screen.moveToFront(self.TIMELINE_TABLE_ID)
			return
		screen.deleteWidget(self.TIMELINE_TABLE_ID)
		if self.IS_SAS_CV_INFO_SCREEN_TIMELINE_CACHE_ENABLE:
			self.TIMELINE_TABLE_CACHE_KEY = timelineTableCacheKey
		else:
			self.TIMELINE_TABLE_CACHE_KEY = None
		self.szTimelineList = self.TIMELINE_TABLE_ID
		screen.addTableControlGFC(self.szTimelineList, 1, self.X_TIMELINE_TABLE, self.Y_TIMELINE_TABLE, self.W_TIMELINE_TABLE, self.H_TIMELINE_TABLE, False, False, 24, 24, TableStyles.TABLE_STYLE_STANDARD)
		screen.setTableColumnHeader(self.szTimelineList, 0, u"", self.W_TIMELINE_TABLE)

		if not aEntries:
			szText = sasFontTagLabel + self.TEXT_TIMELINE_EMPTY + SAS_FONT_TAG_CLOSE
			iRow = screen.appendTableRow(self.szTimelineList)
			SASTextScale.setTableTextLabel(screen, self.szTimelineList, 0, iRow, szText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			return

		# Cache is already in display order (newest first).
		fnAppendRow = screen.appendTableRow
		fnSetText = SASTextScale.setTableTextLabel
		szWidget = self.szTimelineList
		for szFormattedText in aEntries:
			iRow = fnAppendRow(szWidget)
			fnSetText(screen, szWidget, 0, iRow, szFormattedText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def canReuseTimelineTableWidget(self, screen, aEntries):
		iExpectedRows = len(aEntries)
		if iExpectedRows < 1:
			iExpectedRows = 1
		try:
			return screen.getTableNumRows(self.TIMELINE_TABLE_ID) >= iExpectedRows
		except:
			return False

	def getTimelineTableCacheKey(self, aEntries):
		szFirstEntry = None
		szLastEntry = None
		if aEntries:
			szFirstEntry = aEntries[0]
			szLastEntry = aEntries[-1]
		return (self.iActivePlayer, self.bRevealAll, CyGame().getGameTurn(), CyGame().getCurrentLanguage(), self.X_TIMELINE_TABLE, self.Y_TIMELINE_TABLE, self.W_TIMELINE_TABLE, self.H_TIMELINE_TABLE, len(aEntries), szFirstEntry, szLastEntry)

	# <!-- custom: Score Tab renderer (scoreboard-style sortable matrix for all visible players). (GPT-5.3-Codex) -->
	def drawScoreTab(self):
		screen = self.getScreen()
		game = gc.getGame()
		# <!-- custom: in Debug mode, this tab must follow the selected perspective player (dropdown), not always the human active player. (GPT-5.3-Codex) -->
		eActivePlayer = self.iActivePlayer
		pActivePlayer = self.pActivePlayer
		eActiveTeam = self.iActiveTeam
		pActiveTeam = self.pActiveTeam

		visiblePlayers = []
		for j in range(gc.getMAX_CIV_PLAYERS()):
			ePlayer = game.getRankPlayer(j)
			if ePlayer < 0:
				continue
			pPlayer = gc.getPlayer(ePlayer)
			if not pPlayer.isEverAlive():
				continue
			if not Scoreboard.Scoreboard.isShowTeamScore(pPlayer.getTeam()):
				continue
			if not Scoreboard.Scoreboard.isShowPlayerScore(ePlayer):
				continue
			visiblePlayers.append(ePlayer)

		# <!-- custom: wrap the dense Score Table in a high-contrast panel so the slideshow background no longer bleeds through empty rows; this keeps the score matrix readable while still using the shared maximized advisor layout helper.
		# Panel widget name MUST come from getNextWidgetName so it joins the per-tab deletion pool (deleteAllWidgets at the iNumPermanentWidgets snapshot in redrawContents); a hardcoded name persists across tab switches and bleeds the panel into other tabs. (Claude code Opus 4.7 + GPT-5.5) -->
		(iPanelX, iPanelY, iPanelW, iPanelH), (iTableX, iTableY, iTableW, iTableH) = getAdvisorMaximizedPanelLayout(self.W_SCREEN, self.H_SCREEN - self.PANEL_HEIGHT)
		screen.addPanel(self.getNextWidgetName(), "", "", True, True, iPanelX, iPanelY, iPanelW, iPanelH, PanelStyles.PANEL_STYLE_SOLID)

		szTable = self.getNextWidgetName()
		screen.addTableControlGFC(szTable, 29, iTableX, iTableY, iTableW, iTableH, True, True, self.W_STATS_BUTTON_SIZE, self.H_STATS_BUTTON_SIZE, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSort(szTable)

		# <!-- custom: Score Tab mirrors scoreboard semantics in a sortable table: one civ per row, columns for high-value scoreboard signals. (GPT-5.3-Codex) -->
		iColLeader = 0
		iColCiv = 1
		iColColor = 2
		iColPid = 3
		iColName = 4
		iColTrait1 = 5
		iColTrait2 = 6
		iColAttitude = 7
		iColAttitudeNum = 8
		iColScore = 9
		iColDelta = 10
		iColPower = 11
		iColPowerAbs = 12
		iColCities = 13
		iColPowerPerCity = 14
		iColLandPct = 15
		iColVM = 16
		iColTrade = 17
		iColBorders = 18
		iColPact = 19
		iColReligion = 20
		iColWarPeace = 21
		iColWontTalk = 22
		iColWorstEnemy = 23
		iColGoldenAge = 24
		iColEspionage = 25
		iColTechs = 26
		iColResearch = 27
		iColResearchPct = 28

		# <!-- custom: reserve right-side width for table scrollbar gutter in this always-expanded layout, so text does not clip/overflow under it; this budget is absorbed by V/M width balancing. (GPT-5.3-Codex) -->
		iMaxRenderWidthReservePx = 14
		iW = iTableW - iMaxRenderWidthReservePx
		iIconW = 28
		iMinColW = 42
		iFlagW = iMinColW
		iDipW = iMinColW
		iTraitW = iMinColW + 2
		iAttW = iMinColW + 2
		iAttNumW = iMinColW + 2
		iColorW = iMinColW
		# <!-- custom: avoid shriking ID below the normal compact column minimum: double-digit player IDs clip in the Civ4 table widget, e.g. "10" or "11" drawing as "1." or "1" (when font upscaled) with iPidW = iMinColW (42) - 10. (ChatGPT-5.5) -->
		iPidW = iMinColW
		iResearchPctW = iPidW
		# <!-- custom: keep Tech width equal to '%' width because both are compact progress/count metrics and are near each other, so matched width reads cleaner; Tech is fine as 2-digit like '%' in AdvCiv-SAS (no 3-digit tech counts expected). (GPT-5.3-Codex) -->
		iTechsW = iPidW
		iNameW = 115
		# <!-- custom: second pass tuning after 1665 screenshot: reduce Score/dSc slightly, widen Att/Att#, significantly widen Land% and final research %, and fund it mostly from V/M baseline. (GPT-5.3-Codex) -->
		iScoreW = 69
		iDeltaW = 52
		iPowerW = 78
		iPowerAbsW = 71
		iPowerPerCityW = 66
		iCitiesW = iMinColW
		iLandPctW = iMinColW + 28
		iVMW = iMinColW
		iResearchW = iMinColW
		# <!-- custom: width equalization buffer goes into V/M so it grows when horizontal space allows.
		# If needed, let V/M absorb the small width cost instead, since even accounting for double digit vassals/master and font upscaling, we rarely have that many, and if so they are still visible via Scoreboard. (GPT-5.3-Codex + ChatGPT-5.5) -->
		iUsedW = (2 * iIconW + iScoreW + iDeltaW + iDipW + iPowerW + iPowerAbsW + iCitiesW + iPowerPerCityW + iLandPctW + iVMW + iTraitW + iTraitW + iAttNumW + iColorW + iResearchPctW + iTechsW + iAttW + 8 * iFlagW + iPidW + iNameW + iResearchW)
		iExtraW = iW - iUsedW
		iVMW += iExtraW
		if iVMW < iMinColW:
			iDeficit = iMinColW - iVMW
			iVMW = iMinColW
			iResearchShrink = min(iDeficit, iResearchW - iMinColW)
			iResearchW -= iResearchShrink
			iDeficit -= iResearchShrink
			iNameShrink = min(iDeficit, iNameW - iMinColW)
			iNameW -= iNameShrink
			iDeficit -= iNameShrink
			iVMW -= iDeficit

		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColLeader, u"", iIconW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColCiv, u"", iIconW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColColor, u"Col", iColorW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColPid, u"ID", iPidW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColName, u"Leader", iNameW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColTrait1, u"T1", iTraitW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColTrait2, u"T2", iTraitW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColAttitude, u"Att", iAttW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColAttitudeNum, u"Att#", iAttNumW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColScore, self.TEXT_SCORE, iScoreW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColDelta, u"dSc", iDeltaW)
		# <!-- custom: rank column removed because this table is already score-ordered by construction, so Rank duplicated the same ordering signal and added noise. (GPT-5.3-Codex) -->
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColPower, u"PowR", iPowerW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColPowerAbs, u"PowT", iPowerAbsW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColCities, u"Cit", iCitiesW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColPowerPerCity, u"PoT/C", iPowerPerCityW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColLandPct, u"Land%", iLandPctW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColVM, u"V/M", iVMW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColTrade, u"Trd", iFlagW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColBorders, u"OB", iFlagW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColPact, u"DP", iFlagW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColReligion, u"Rel", iFlagW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColWarPeace, u"Dip", iDipW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColWontTalk, u"WT", iFlagW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColWorstEnemy, u"WE", iFlagW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColGoldenAge, u"GA", iFlagW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColEspionage, u"Esp", iFlagW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColTechs, u"Tech", iTechsW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColResearch, u"Res", iResearchW)
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iColResearchPct, u"%", iResearchPctW)

		# <!-- custom: perf pass - hoist stable lookups/options used in the per-player loop. (GPT-5.3-Codex) -->
		bDebugMode = game.isDebugMode()
		bUseEspionage = GameUtil.isEspionage()
		bIncludeCurrentTurnDelta = ScoreOpt.isScoreDeltaIncludeCurrentTurn()
		bPowerThemVersusYou = ScoreOpt.isPowerThemVersusYou()
		iPowerDecimals = max(3, ScoreOpt.getPowerDecimals())
		iPowerColor = ScoreOpt.getPowerColor()
		iHighPowerColor = ScoreOpt.getHighPowerColor()
		iLowPowerColor = ScoreOpt.getLowPowerColor()
		fHighPowerRatio = ScoreOpt.getHighPowerRatio()
		fLowPowerRatio = ScoreOpt.getLowPowerRatio()
		iActivePower = pActivePlayer.getPower()
		iActiveGameTurn = game.getGameTurn()
		bActiveTeamHasMultipleMembers = (pActiveTeam.getNumMembers() > 1)
		iLandPlots = CyMap().getLandPlots()
		iMaxTeams = gc.getMAX_TEAMS()
		teamCache = []
		for iTeam in range(iMaxTeams):
			teamCache.append(gc.getTeam(iTeam))
		teamHasVassals = [False] * iMaxTeams
		teamMasterTeams = []
		teamVassalTeams = []
		teamTechCounts = [0] * iMaxTeams
		iNumTechInfos = gc.getNumTechInfos()
		for iTeam in range(iMaxTeams):
			teamMasterTeams.append([])
			teamVassalTeams.append([])
		for iTeam in range(iMaxTeams):
			kTeam = teamCache[iTeam]
			if not kTeam.isAlive():
				continue
			iTechCount = 0
			for iTech in range(iNumTechInfos):
				if kTeam.isHasTech(iTech):
					iTechCount += 1
			teamTechCounts[iTeam] = iTechCount
			if kTeam.isAVassal():
				for iOwnerTeam in range(iMaxTeams):
					if kTeam.isVassal(iOwnerTeam):
						teamHasVassals[iOwnerTeam] = True
						teamMasterTeams[iTeam].append(iOwnerTeam)
						teamVassalTeams[iOwnerTeam].append(iTeam)
						break

		visiblePlayersByTeam = {}
		for eLoopPlayer in visiblePlayers:
			eLoopTeam = gc.getPlayer(eLoopPlayer).getTeam()
			if eLoopTeam not in visiblePlayersByTeam:
				visiblePlayersByTeam[eLoopTeam] = []
			visiblePlayersByTeam[eLoopTeam].append(eLoopPlayer)

		for ePlayer in visiblePlayers:
			pPlayer = gc.getPlayer(ePlayer)
			eTeam = pPlayer.getTeam()
			pTeam = teamCache[eTeam]
			bMet = (pActiveTeam.isHasMet(eTeam) or bDebugMode or ePlayer == eActivePlayer)

			screen.appendTableRow(szTable)
			iRow = screen.getTableNumRows(szTable) - 1

			# <!-- custom: Score Tab is a reference matrix, so leader/civ identity cells open Sevopedia instead of diplomacy; contact actions are already covered by the scoreboard and Foreign Advisor. (GPT-5.5) -->
			# <!-- custom: Pass the displayed player's civilization to leader-hover trait help; literal data2=1 meant Arabia and could name the wrong civ-specific production replacement. See KI#332. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			SASTextScale.setTableTextLabel(screen, szTable, iColLeader, iRow, getAdvisorIconSortKey(pPlayer.getLeaderType() + 1, iRow), gc.getLeaderHeadInfo(pPlayer.getLeaderType()).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_LEADER, pPlayer.getLeaderType(), pPlayer.getCivilizationType(), CvUtil.FONT_LEFT_JUSTIFY)
			SASTextScale.setTableTextLabel(screen, szTable, iColCiv, iRow, getAdvisorIconSortKey(pPlayer.getCivilizationType() + 1, iRow), gc.getCivilizationInfo(pPlayer.getCivilizationType()).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIV, pPlayer.getCivilizationType(), -1, CvUtil.FONT_LEFT_JUSTIFY)
			szColor = self.SCORETAB_COLOR_MARKER
			ePlayerColor = pPlayer.getPlayerColor()
			if ePlayerColor > -1:
				kPlayerColor = gc.getPlayerColorInfo(ePlayerColor)
				if kPlayerColor is not None:
					szColor = localText.changeTextColor(self.SCORETAB_COLOR_MARKER, kPlayerColor.getTextColorType())
			SASTextScale.setTableTextLabel(screen, szTable, iColColor, iRow, szColor, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
			SASTextScale.setTableIntLabel(screen, szTable, iColPid, iRow, str(ePlayer), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)

			szName = pPlayer.getName()
			if not bMet:
				szName = localText.getText("TXT_KEY_TOPCIVS_UNKNOWN", ())
			SASTextScale.setTableTextLabel(screen, szTable, iColName, iRow, szName, "", WidgetTypes.WIDGET_PEDIA_JUMP_TO_LEADER, pPlayer.getLeaderType(), pPlayer.getCivilizationType(), CvUtil.FONT_LEFT_JUSTIFY)
			# <!-- custom: Score Tab trait columns (T1/T2): trait icon chars for compact readability next to leader identity data. (GPT-5.3-Codex) -->
			iTrait1 = -1
			iTrait2 = -1
			kLeaderInfo = gc.getLeaderHeadInfo(pPlayer.getLeaderType())
			for iTraitLoop in range(gc.getNumTraitInfos()):
				if kLeaderInfo.hasTrait(iTraitLoop):
					if iTrait1 < 0:
						iTrait1 = iTraitLoop
					elif iTrait2 < 0:
						iTrait2 = iTraitLoop
						break
			szTrait1 = u""
			szTrait2 = u""
			if iTrait1 > -1:
				szTrait1 = TraitUtil.getIcon(iTrait1)
			if iTrait2 > -1:
				szTrait2 = TraitUtil.getIcon(iTrait2)
			SASTextScale.setTableTextLabel(screen, szTable, iColTrait1, iRow, szTrait1, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
			SASTextScale.setTableTextLabel(screen, szTable, iColTrait2, iRow, szTrait2, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

			iScore = game.getPlayerScore(ePlayer)
			eScoreWidget = WidgetTypes.WIDGET_GENERAL
			if ePlayer == eActivePlayer:
				eScoreWidget = WidgetTypes.WIDGET_SCORE_BREAKDOWN
			# <!-- custom: data2=-1 preserves the active-player score breakdown without triggering the scoreboard-only data2=0 expansion control. See KI#388. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			# <!-- custom: Score-tab sort fix: Civ4 table sorting is type-aware, and values inserted with setTableText sort lexicographically (e.g. "96" before "413"), which broke PoT/C and other numeric columns.
			# Use setTableInt for numeric fields so ordering is truly numeric, while still keeping setTableText fallbacks where values are unknown/hidden so blanks remain blank instead of fake defaults. (GPT-5.3-Codex) -->
			SASTextScale.setTableIntLabel(screen, szTable, iColScore, iRow, str(iScore), "", eScoreWidget, ePlayer, -1, CvUtil.FONT_RIGHT_JUSTIFY)

			iGameTurn = iActiveGameTurn
			if ePlayer >= eActivePlayer:
				iGameTurn -= 1
			if bIncludeCurrentTurnDelta:
				iScoreDelta = iScore
			elif iGameTurn >= 0:
				iScoreDelta = pPlayer.getScoreHistory(iGameTurn)
			else:
				iScoreDelta = 0
			iPrevGameTurn = iGameTurn - 1
			if iPrevGameTurn >= 0:
				iScoreDelta -= pPlayer.getScoreHistory(iPrevGameTurn)
			# <!-- custom: keep dSc color cues (green/red) while preserving numeric sort.
			# Civ4 sorting uses cell type, so this must stay setTableInt; we pass a colorized string value to keep visual feedback without falling back to lexicographic setTableText sorting. (GPT-5.3-Codex) -->
			szScoreDelta = str(iScoreDelta)
			if iScoreDelta > 0:
				szScoreDelta = u"+%d" % iScoreDelta
				szScoreDelta = localText.changeTextColor(szScoreDelta, self.SCORETAB_COLOR_ALT_HIGHLIGHT_TEXT)
			elif iScoreDelta < 0:
				szScoreDelta = localText.changeTextColor(szScoreDelta, self.SCORETAB_COLOR_RED)
			SASTextScale.setTableIntLabel(screen, szTable, iColDelta, iRow, szScoreDelta, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)

			szWarPeace = u""
			if pTeam.isAtWar(eActiveTeam):
				szWarPeace = self.SCORETAB_WAR_CHAR
			elif pActiveTeam.isForcePeace(eTeam):
				szWarPeace = self.SCORETAB_PEACE_CHAR
			SASTextScale.setTableTextLabel(screen, szTable, iColWarPeace, iRow, szWarPeace, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

			szPower = u""
			szPowerAbs = u""
			szPowerPerCity = u""
			fShownPowerRatio = None
			iPowerColorForWidget = 0
			iTheirPower = -1
			if ePlayer == eActivePlayer:
				# <!-- custom: always show self-known absolute power metrics (PowT/PowT/C) for the active row.
				# We intentionally keep PowR blank for self because a self-vs-self ratio is not meaningful in this table. (GPT-5.3-Codex) -->
				iTheirPower = iActivePower
			elif bDebugMode or pActivePlayer.canSeeDemographics(ePlayer):
				iTheirPower = pPlayer.getPower()
			if ePlayer != eActivePlayer and iTheirPower > 0:
					fRatio = float(iActivePower) / float(iTheirPower)
					if bPowerThemVersusYou:
						if fRatio > 0:
							fRatio = 1.0 / fRatio
						else:
							fRatio = 999.0
					fShownPowerRatio = fRatio
					szPower = BugUtil.formatFloat(fRatio, iPowerDecimals)
					# <!-- custom: apply the same threshold-based power coloring as scoreboard (high/low ratio bands + optional default color). (GPT-5.3-Codex) -->
					bAlly = (eTeam == eActiveTeam)
					if (iHighPowerColor >= 0 and not bAlly and fRatio >= fHighPowerRatio):
						iPowerColorForWidget = iHighPowerColor
					elif (iLowPowerColor >= 0 and not bAlly and fRatio <= fLowPowerRatio):
						iPowerColorForWidget = iLowPowerColor
					elif iPowerColor >= 0:
						iPowerColorForWidget = iPowerColor
					if iPowerColorForWidget > 0:
						szPower = localText.changeTextColor(szPower, iPowerColorForWidget)
			# <!-- custom: Use an Info Screen-specific widget with no scoreboard expansion side effect, and carry the selected advisor perspective explicitly for accurate ratio and espionage help. See KI#388 and KI#1059. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			SASTextScale.setTableTextLabel(screen, szTable, iColPower, iRow, szPower, "", WidgetTypes.WIDGET_POWER_RATIO_INFO_SCREEN, ePlayer, eActivePlayer, CvUtil.FONT_RIGHT_JUSTIFY)
			if iTheirPower > -1:
				SASTextScale.setTableIntLabel(screen, szTable, iColPowerAbs, iRow, str(iTheirPower), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
			else:
				SASTextScale.setTableTextLabel(screen, szTable, iColPowerAbs, iRow, szPowerAbs, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)

			if bDebugMode:
				iCities = pPlayer.getNumCities()
			else:
				iCities = PlayerUtil.getNumRevealedCities(ePlayer)
			SASTextScale.setTableIntLabel(screen, szTable, iColCities, iRow, str(iCities), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
			iPowerPerCity = -1
			if iTheirPower > -1 and iCities > 0:
				# <!-- custom: PowT/C must be comparable across rows, so compute it from absolute power (PowT / Cit) for everyone. (GPT-5.3-Codex) -->
				iPowerPerCity = iTheirPower / iCities
			if iPowerPerCity > -1:
				SASTextScale.setTableIntLabel(screen, szTable, iColPowerPerCity, iRow, str(iPowerPerCity), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
			else:
				SASTextScale.setTableTextLabel(screen, szTable, iColPowerPerCity, iRow, szPowerPerCity, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)

			szLandPct = u""
			if bMet:
				if iLandPlots > 0:
					iLandPctTimes100 = (10000 * pPlayer.getTotalLand()) / iLandPlots
					szLandPct = u"%d.%02d" % (iLandPctTimes100 / 100, iLandPctTimes100 % 100)
			SASTextScale.setTableTextLabel(screen, szTable, iColLandPct, iRow, szLandPct, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)

			szVM = u""
			if bMet:
				asVM = []
				if pTeam.isAVassal():
					for eMasterTeam in teamMasterTeams[eTeam]:
						for eMasterPlayer in visiblePlayersByTeam.get(eMasterTeam, []):
							asVM.append(u"|" + str(eMasterPlayer))
				if teamHasVassals[eTeam]:
					# <!-- custom: V/M uses role prefixes, not separators: '|ID' marks a master and ',ID' marks a vassal.
					# The comma is a per-ID vassal prefix (not a CSV/comma-separated list delimiter).
					# We still prepend it to every vassal ID so multi-digit IDs remain readable and unambiguous. (GPT-5.3-Codex) -->
					for eVassalTeam in teamVassalTeams[eTeam]:
						for eVassalPlayer in visiblePlayersByTeam.get(eVassalTeam, []):
							asVM.append(u"," + str(eVassalPlayer))
				szVM = u"".join(asVM)
			SASTextScale.setTableTextLabel(screen, szTable, iColVM, iRow, szVM, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

			eCurrentResearch = pPlayer.getCurrentResearch()
			szResearchButton = ""
			szResearchPct = u""
			iProgressPct = -1
			iResearchSortGroup = 0
			if bMet and (bDebugMode or (pActivePlayer.canSeeResearch(ePlayer) and (eTeam != eActiveTeam or bActiveTeamHasMultipleMembers))):
				if eCurrentResearch != -1:
					kTech = gc.getTechInfo(eCurrentResearch)
					szResearchButton = kTech.getButton()
					iResearchSortGroup = eCurrentResearch + 1
					iResearchCost = pTeam.getResearchCost(eCurrentResearch)
					if iResearchCost > 0:
						iProgressPct = pTeam.getResearchProgress(eCurrentResearch) * 100 / iResearchCost
						iProgressPct = max(0, min(99, iProgressPct))
						szResearchPct = str(iProgressPct)
			SASTextScale.setTableIntLabel(screen, szTable, iColTechs, iRow, str(teamTechCounts[eTeam]), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
			SASTextScale.setTableTextLabel(screen, szTable, iColResearch, iRow, getAdvisorIconSortKey(iResearchSortGroup, iRow), szResearchButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
			if iProgressPct > -1:
				SASTextScale.setTableIntLabel(screen, szTable, iColResearchPct, iRow, str(iProgressPct), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
			else:
				SASTextScale.setTableTextLabel(screen, szTable, iColResearchPct, iRow, szResearchPct, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)

			szEsp = u""
			if bMet and bUseEspionage and pActivePlayer.getEspionageSpendingWeightAgainstTeam(eTeam) > 0:
				szEsp = self.SCORETAB_ESPIONAGE_CHAR
			SASTextScale.setTableTextLabel(screen, szTable, iColEspionage, iRow, szEsp, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
			szTrade = u""
			if bMet and ePlayer != eActivePlayer and pPlayer.canTradeNetworkWith(eActivePlayer):
				szTrade = self.SCORETAB_TRADE_CHAR
			# <!-- custom: The ordinary trade-routes widget provides the same hover without the scoreboard-only expansion side effect. See KI#388. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			SASTextScale.setTableTextLabel(screen, szTable, iColTrade, iRow, szTrade, "", WidgetTypes.WIDGET_TRADE_ROUTES, eActivePlayer, ePlayer, CvUtil.FONT_CENTER_JUSTIFY)

			szBorders = u""
			if bMet and pTeam.isOpenBorders(eActiveTeam):
				szBorders = self.SCORETAB_BORDERS_CHAR
			SASTextScale.setTableTextLabel(screen, szTable, iColBorders, iRow, szBorders, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

			szPact = u""
			if bMet and pTeam.isDefensivePact(eActiveTeam):
				szPact = self.SCORETAB_PACT_CHAR
			SASTextScale.setTableTextLabel(screen, szTable, iColPact, iRow, szPact, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

			szReligion = u""
			eStateReligion = pPlayer.getStateReligion()
			if bMet and eStateReligion != -1:
				kReligion = gc.getReligionInfo(eStateReligion)
				if pPlayer.hasHolyCity(eStateReligion):
					szReligion = u"%c" % kReligion.getHolyCityChar()
				else:
					szReligion = u"%c" % kReligion.getChar()
			SASTextScale.setTableTextLabel(screen, szTable, iColReligion, iRow, szReligion, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

			# <!-- custom: note: textual attitude icon chars used to disappear when text was upscaled (label 3/4) while displaying fine at label 2 or lower; fixed by adding attitude glyph coverage in GameFont.tga (they had only existed in GameFont_75.tga). See KI#117. (Claude code Opus 4.6 + GPT-5.3-Codex) -->
			szAttitude = u""
			szAttitudeNum = u""
			iAttCount = 0
			bHasAttitudeNum = False
			if bMet and not pPlayer.isHuman() and ePlayer != eActivePlayer:
				szAttitude = AttitudeUtil.getAttitudeIcon(ePlayer, eActivePlayer)
				iAttCount = AttitudeUtil.getAttitudeCount(ePlayer, eActivePlayer)
				# <!-- custom: same pattern as dSc: keep Att# as setTableInt for numeric ordering, then apply color to the displayed value string. (GPT-5.3-Codex) -->
				szAttitudeNum = u"%+d" % iAttCount
				iAttColor = AttitudeUtil.getAttitudeColor(ePlayer, eActivePlayer)
				if iAttColor >= 0:
					szAttitudeNum = localText.changeTextColor(szAttitudeNum, iAttColor)
				bHasAttitudeNum = True
			SASTextScale.setTableTextLabel(screen, szTable, iColAttitude, iRow, szAttitude, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
			if bHasAttitudeNum:
				SASTextScale.setTableIntLabel(screen, szTable, iColAttitudeNum, iRow, szAttitudeNum, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
			else:
				SASTextScale.setTableTextLabel(screen, szTable, iColAttitudeNum, iRow, szAttitudeNum, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)

			szWontTalk = u""
			if bMet and not DiplomacyUtil.isWillingToTalk(ePlayer, eActivePlayer):
				szWontTalk = u"!"
			SASTextScale.setTableTextLabel(screen, szTable, iColWontTalk, iRow, szWontTalk, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

			szWorstEnemy = u""
			if bMet and AttitudeUtil.isWorstEnemy(ePlayer, eActivePlayer):
				szWorstEnemy = self.SCORETAB_WORST_ENEMY_CHAR
			SASTextScale.setTableTextLabel(screen, szTable, iColWorstEnemy, iRow, szWorstEnemy, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

			szGoldenAge = u""
			bGoldenAge = pPlayer.isGoldenAge()
			bAnarchy = pPlayer.isAnarchy()
			if ePlayer != eActivePlayer and (bGoldenAge or bAnarchy):
				if bAnarchy:
					szGoldenAge = self.SCORETAB_ANARCHY_CHAR
				else:
					szGoldenAge = self.SCORETAB_GOLDEN_AGE_CHAR
			SASTextScale.setTableTextLabel(screen, szTable, iColGoldenAge, iRow, szGoldenAge, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

		# <!-- custom: refactored to the shared placeAdvisorLegendLink helper in SASUtils so AdvCiv-SAS advisors share one canonical legend-link path.
		# The helper applies the SAS_SHOW_LEGEND_LINK gate and missing-concept guard internally. (Claude code Opus 4.7) -->
		placeAdvisorLegendLink(self, "CONCEPT_SAS_SCORE_TAB_COLUMNS", iTableX + iTableW - 6, self.Y_TITLE)

	def dbgLogPrettySummary(self):
		if self.IS_SAS_CV_INFO_SCREEN_TIMELINE_CACHE_ENABLE:
			aEntries = self.timelineCacheEntries
		else:
			aEntries = self.buildTimelineCache(True)
		if not aEntries:
			print("SAS_CV_INFO_SCREEN_TIMELINE_PANEL_EMPTY")
			return
		print("SAS_CV_INFO_SCREEN_TIMELINE_PANEL_BEGIN")
		# Strip font/color tags from cached display strings for clean log output
		# Entries are newest-first, reverse for chronological log output
		reFont = re.compile(r"</?font[^>]*>")
		reColor = re.compile(r"</?color[^>]*>")
		aLines = []
		for szEntry in reversed(aEntries):
			szClean = reFont.sub("", szEntry)
			szClean = reColor.sub("", szClean)
			aLines.append(u"- " + szClean)
		print("\n".join(aLines))
		print("SAS_CV_INFO_SCREEN_TIMELINE_PANEL_END")

	def drawPermanentGraphWidgets(self):

		screen = self.getScreen()

#BUG: Change Graphs - start
		self.sGraphTextHeadingWidget = [0] * self.NUM_SCORES
		self.sGraphTextBannerWidget = [0] * self.NUM_SCORES
		self.sGraphPanelWidget = [0] * self.NUM_SCORES
		self.sGraphBGWidget = [0] * self.NUM_SCORES
		for i in range(self.NUM_SCORES):
			self.sGraphTextHeadingWidget[i] = self.getNextWidgetName()
			self.sGraphTextBannerWidget[i] = self.getNextWidgetName()
			self.sGraphPanelWidget[i] = self.getNextWidgetName()
			self.sGraphBGWidget[i] = self.getNextWidgetName()

		self.sGraph7in1 = self.getNextWidgetName()
		self.sGraph3in1 = self.getNextWidgetName()
		self.sGraph1in1 = self.getNextWidgetName()

		self.sPlayerTextWidget = [0] * gc.getMAX_CIV_PLAYERS()
		for i in range(gc.getMAX_CIV_PLAYERS()):
			self.sPlayerTextWidget[i] = self.getNextWidgetName()

		self.sShowAllWidget = self.getNextWidgetName()
		self.sShowNoneWidget = self.getNextWidgetName()

		if not AdvisorOpt.isGraphs():
			self.drawLegend()

		if AdvisorOpt.isGraphs():
			iX_LEFT_BUTTON = self.X_MARGIN
		else:
			iX_LEFT_BUTTON = self.X_LEFT_BUTTON
#BUG: Change Graphs - end

		self.graphLeftButtonID = self.getNextWidgetName()
		screen.setButtonGFC( self.graphLeftButtonID, u"", "", iX_LEFT_BUTTON, self.Y_LEFT_BUTTON, self.W_LEFT_BUTTON, self.H_LEFT_BUTTON, WidgetTypes.WIDGET_GENERAL, -1, -1, ButtonStyles.BUTTON_STYLE_ARROW_LEFT )
		self.graphRightButtonID = self.getNextWidgetName()
		screen.setButtonGFC( self.graphRightButtonID, u"", "", self.X_RIGHT_BUTTON, self.Y_RIGHT_BUTTON, self.W_RIGHT_BUTTON, self.H_RIGHT_BUTTON, WidgetTypes.WIDGET_GENERAL, -1, -1, ButtonStyles.BUTTON_STYLE_ARROW_RIGHT )
		screen.enable(self.graphLeftButtonID, False)
		screen.enable(self.graphRightButtonID, False)

		# Dropdown Box
		self.szGraphDropdownWidget = self.getNextWidgetName()
		screen.addDropDownBoxGFC(self.szGraphDropdownWidget, self.X_DEMO_DROPDOWN, self.Y_DEMO_DROPDOWN, self.W_DEMO_DROPDOWN, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		screen.addPullDownString(self.szGraphDropdownWidget, self.TEXT_SCORE, 0, 0, self.iGraphTabID == self.TOTAL_SCORE )
		screen.addPullDownString(self.szGraphDropdownWidget, self.TEXT_ECONOMY, 1, 1, self.iGraphTabID == self.ECONOMY_SCORE )
		screen.addPullDownString(self.szGraphDropdownWidget, self.TEXT_INDUSTRY, 2, 2, self.iGraphTabID == self.INDUSTRY_SCORE )
		screen.addPullDownString(self.szGraphDropdownWidget, self.TEXT_AGRICULTURE, 3, 3, self.iGraphTabID == self.AGRICULTURE_SCORE )
		screen.addPullDownString(self.szGraphDropdownWidget, self.TEXT_POWER, 4, 4, self.iGraphTabID == self.POWER_SCORE )
		screen.addPullDownString(self.szGraphDropdownWidget, self.TEXT_CULTURE, 5, 5, self.iGraphTabID == self.CULTURE_SCORE )
#BUG - 3.17 No Espionage - start
		if (GameUtil.isEspionage()):
			screen.addPullDownString(self.szGraphDropdownWidget, self.TEXT_ESPIONAGE, 6, 6, self.iGraphTabID == self.ESPIONAGE_SCORE )
#BUG - 3.17 No Espionage - end

#BUG: Change Graphs - start
		if AdvisorOpt.isGraphs():
			screen.hide(self.szGraphDropdownWidget)

			iX_ZOOM_DROPDOWN = 870
			iY_ZOOM_DROPDOWN = 10
			if CyGame().isDebugMode():
				iY_SMOOTH_DROPDOWN = iY_ZOOM_DROPDOWN + 50
			else:
				iY_SMOOTH_DROPDOWN = iY_ZOOM_DROPDOWN
		else:
			iX_ZOOM_DROPDOWN = self.X_ZOOM_DROPDOWN
			iY_ZOOM_DROPDOWN = self.Y_ZOOM_DROPDOWN

		# graph smoothing dropdown
		if AdvisorOpt.isGraphs():
			self.szGraphSmoothingDropdownWidget_1in1 = self.getNextWidgetName()
			self.szGraphSmoothingDropdownWidget_7in1 = self.getNextWidgetName()

	#		screen.addDropDownBoxGFC(self.szGraphSmoothingDropdownWidget, iX_ZOOM_DROPDOWN - self.W_DEMO_DROPDOWN - 60, iY_ZOOM_DROPDOWN, self.W_DEMO_DROPDOWN + 50, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
			screen.addDropDownBoxGFC(self.szGraphSmoothingDropdownWidget_1in1, 10, iY_SMOOTH_DROPDOWN, self.W_DEMO_DROPDOWN + 50, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
			screen.addDropDownBoxGFC(self.szGraphSmoothingDropdownWidget_7in1, 10, iY_SMOOTH_DROPDOWN, self.W_DEMO_DROPDOWN + 50, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
			for i in range(11):
				screen.addPullDownString(self.szGraphSmoothingDropdownWidget_1in1, localText.getText("TXT_KEY_GRAPH_SMOOTHING", (i,)), i, i, False )
				screen.addPullDownString(self.szGraphSmoothingDropdownWidget_7in1, localText.getText("TXT_KEY_GRAPH_SMOOTHING", (i,)), i, i, False )

			screen.hide(self.szGraphSmoothingDropdownWidget_1in1)
			screen.hide(self.szGraphSmoothingDropdownWidget_7in1)

			# 3 in 1 graph selectionS
			self.szGraphDropdownWidget_3in1 = [""] * 3
			self.iGraph_3in1 = [0, 1, 2]
			iW_GRAPH = 463
			iH_GRAPH = 290
			for i in range(3):
				self.szGraphDropdownWidget_3in1[i] = self.getNextWidgetName()
				x = self.X_MARGIN + self.X_3_IN_1_CHART_ADJ[i] * (iW_GRAPH + 11) + 10
				y = self.Y_MARGIN + self.Y_3_IN_1_CHART_ADJ[i] * (iH_GRAPH + 10) + 5
				screen.addDropDownBoxGFC(self.szGraphDropdownWidget_3in1[i], x, y, self.W_DEMO_DROPDOWN + 50, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
				for j in range(self.NUM_SCORES):
					screen.addPullDownString(self.szGraphDropdownWidget_3in1[i], self.sGraphText[0][j], j, j, self.iGraph_3in1[i] == j )
				screen.hide(self.szGraphDropdownWidget_3in1[i])

		if not AdvisorOpt.isGraphs():
			self.iGraph_Smoothing = 0

#BUG: Change Graphs - end

		self.dropDownTurns = []
		self.szTurnsDropdownWidget = self.getNextWidgetName()
		screen.addDropDownBoxGFC(self.szTurnsDropdownWidget, iX_ZOOM_DROPDOWN, iY_ZOOM_DROPDOWN, self.W_ZOOM_DROPDOWN, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		start = CyGame().getStartTurn()
		now = CyGame().getGameTurn()
		nTurns = now - start - 1
		screen.addPullDownString(self.szTurnsDropdownWidget, self.TEXT_ENTIRE_TIMELINE, 0, 0, False)
		self.dropDownTurns.append(nTurns)
		iCounter = 1
		last = 50
		while (last < nTurns):
			screen.addPullDownString(self.szTurnsDropdownWidget, localText.getText("TXT_KEY_INFO_NUM_TURNS", (last,)), iCounter, iCounter, False)
			self.dropDownTurns.append(last)
			iCounter += 1
			last += 50

		if AdvisorOpt.isGraphs():
			screen.setText(self.sGraph1in1, "", "1/1", CvUtil.FONT_CENTER_JUSTIFY, 22,  90, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
			screen.setText(self.sGraph3in1, "", "3/1", CvUtil.FONT_CENTER_JUSTIFY, 22, 110, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
			screen.setText(self.sGraph7in1, "", "7/1", CvUtil.FONT_CENTER_JUSTIFY, 22, 130, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
			screen.setLabel(self.getNextWidgetName(), "", self.BUG_GRAPH_HELP, CvUtil.FONT_CENTER_JUSTIFY, self.X_TITLE, self.Y_EXIT - 40, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		self.iNumPreDemoChartWidgets = self.nWidgetCount

	def updateGraphButtons(self):
		screen = self.getScreen()
		screen.enable(self.graphLeftButtonID, self.graphEnd - self.graphZoom > CyGame().getStartTurn())
		screen.enable(self.graphRightButtonID, self.graphEnd < CyGame().getGameTurn() - 1)

	def checkGraphBounds(self):
		start = CyGame().getStartTurn()
		end = CyGame().getGameTurn() - 1
		if (self.graphEnd - self.graphZoom < start):
			self.graphEnd = start + self.graphZoom
		if (self.graphEnd > end):
			self.graphEnd = end

	def zoomGraph(self, zoom):
		self.graphZoom = zoom
		self.checkGraphBounds()
		self.updateGraphButtons()

	def slideGraph(self, right):
		self.graphEnd += right
		self.checkGraphBounds()
		self.updateGraphButtons()

	def buildScoreCache(self, scoreType):

		# Check if the scores have already been computed
		if (self.scoreCache[scoreType]):
			return

#		print("Rebuilding score cache")

		# <advc.091>
		aiCachePlayers = copy.copy(self.aiPlayersMet)
		if scoreType == self.TOTAL_SCORE:
			for p in self.aiPlayersMetNAEspionage:
				if self.showTotalScoreGraph(p):
					aiCachePlayers.append(p)
		# </advc.091>

		# Get the player with the highest ID
		maxPlayer = 0
		for p in aiCachePlayers: # advc.091: was self.aiPlayersMet
			if (maxPlayer < p):
				maxPlayer = p

		# Compute the scores
		self.scoreCache[scoreType] = []
		for p in range(maxPlayer + 1):

			if (p not in aiCachePlayers): # advc.091: was self.aiPlayersMet
				# Don't compute score for people we haven't met
				self.scoreCache[scoreType].append(None)
			else:
				self.scoreCache[scoreType].append([])
				firstTurn	= CyGame().getStartTurn()
				thisTurn	= CyGame().getGameTurn()
				turn	= firstTurn
				# advc.091:
				bHideBeginning = (scoreType == self.TOTAL_SCORE and not self.pActivePlayer.hasEverSeenDemographics(p) and p in self.aiPlayersMetNAEspionage)
				while (turn <= thisTurn):
					# <advc.091>
					if bHideBeginning and turn < self.pActiveTeam.getHasMetTurn(gc.getPlayer(p).getTeam()):
						score = -1
					else: # </advc.091>
						score = self.computeHistory(scoreType, p, turn)
					self.scoreCache[scoreType][p].append(score)
					turn += 1

		return

	def computeHistory(self, scoreType, iPlayer, iTurn):

		iScore = gc.getPlayer(iPlayer).getScoreHistory(iTurn)

		if (iScore == 0):	# for some reason only the score is 0 when you're dead..?
			return 0

		if (scoreType == self.TOTAL_SCORE):
			return iScore
		elif (scoreType == self.ECONOMY_SCORE):
			return gc.getPlayer(iPlayer).getEconomyHistory(iTurn)
		elif (scoreType == self.INDUSTRY_SCORE):
			return gc.getPlayer(iPlayer).getIndustryHistory(iTurn)
		elif (scoreType == self.AGRICULTURE_SCORE):
			return gc.getPlayer(iPlayer).getAgricultureHistory(iTurn)
		elif (scoreType == self.POWER_SCORE):
			return gc.getPlayer(iPlayer).getPowerHistory(iTurn)
		elif (scoreType == self.CULTURE_SCORE):
			return gc.getPlayer(iPlayer).getCultureHistory(iTurn)
		elif (scoreType == self.ESPIONAGE_SCORE):
			return gc.getPlayer(iPlayer).getEspionageHistory(iTurn)

	# Requires the cache to be built
	def getHistory(self, scoreType, iPlayer, iRelTurn):
		return self.scoreCache[scoreType][iPlayer][iRelTurn]

	def drawGraphLines(self, sGRAPH_CANVAS_ID):
		screen = self.getScreen()

		if (self.xSelPt != 0 or self.ySelPt != 0):
			screen.addLineGFC(sGRAPH_CANVAS_ID, self.GRAPH_H_LINE, 0, self.ySelPt, self.W_GRAPH, self.ySelPt, self.COLOR_GREY)
			screen.addLineGFC(sGRAPH_CANVAS_ID, self.GRAPH_V_LINE, self.xSelPt, 0, self.xSelPt, self.H_GRAPH, self.COLOR_GREY)
		else:
			screen.addLineGFC(sGRAPH_CANVAS_ID, self.GRAPH_H_LINE, -1, -1, -1, -1, self.COLOR_GREY)
			screen.addLineGFC(sGRAPH_CANVAS_ID, self.GRAPH_V_LINE, -1, -1, -1, -1, self.COLOR_GREY)

	def drawXLabel(self, screen, turn, x, just = CvUtil.FONT_CENTER_JUSTIFY):
#BUG: Change Graphs - start
		if AdvisorOpt.isGraphs():
			if self.Graph_Status_Current == self.Graph_Status_1in1:
				screen.show(self.graphLeftButtonID)
				screen.show(self.graphRightButtonID)
			else:
				screen.hide(self.graphLeftButtonID)
				screen.hide(self.graphRightButtonID)
				return
#BUG: Change Graphs - end

		# <!-- custom: show year on first line, added with claude opus 4.5's help thanks. -->
		screen.setLabel(self.getNextWidgetName(), "", sasFontTagLabel + self.getTurnDate(turn) + SAS_FONT_TAG_CLOSE, just, x, self.Y_LABEL, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		# <!-- custom: show turn number on second line below year, added with claude opus 4.5's help thanks. -->
		screen.setLabel(self.getNextWidgetName(), "", sasFontTagLabel + (u"T%d" % turn) + SAS_FONT_TAG_CLOSE, just, x, self.Y_LABEL + self.iGraphTurnLabelYOffset, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def drawGraphs(self):

		self.timer.reset()
		self.timer.start()

		self.deleteAllLines()
		self.deleteAllWidgets(self.iNumPreDemoChartWidgets)

#BUG: Change Graphs - start
		if not AdvisorOpt.isGraphs():
			self.drawGraph(self.iGraphTabID)
		else:
			screen = self.getScreen()

			# show the right smoothing dropdown
			if self.Graph_Status_Current == self.Graph_Status_1in1:
				screen.show(self.szGraphSmoothingDropdownWidget_1in1)
				screen.hide(self.szGraphSmoothingDropdownWidget_7in1)
			else:
				screen.hide(self.szGraphSmoothingDropdownWidget_1in1)
				screen.show(self.szGraphSmoothingDropdownWidget_7in1)

			for i in range(7):
				screen.hide(self.sGraphTextHeadingWidget[i])
				screen.hide(self.sGraphPanelWidget[i])

			for i in range(3):
				screen.hide(self.szGraphDropdownWidget_3in1[i])

			if self.Graph_Status_Current == self.Graph_Status_1in1:
				self.drawGraph(self.iGraphTabID)
				self.drawLegend()

				iY = self.Y_MARGIN - 30
				for i in range(7):
					if i == 7 and not GameUtil.isEspionage():
						continue

					iX = self.X_GRAPH_TEXT[i]
					screen.hide(self.sGraphTextBannerWidget[i])
					if i == self.iGraphTabID:
						screen.setText(self.sGraphTextBannerWidget[i], "", self.sGraphText[1][i], CvUtil.FONT_LEFT_JUSTIFY, iX, iY, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
					else:
						screen.setText(self.sGraphTextBannerWidget[i], "", self.sGraphText[0][i], CvUtil.FONT_LEFT_JUSTIFY, iX, iY, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

			elif self.Graph_Status_Current == self.Graph_Status_7in1:
				for i in range(7):
					if i == 7 and not GameUtil.isEspionage():
						continue

					screen.hide(self.sGraphTextBannerWidget[i])
					self.drawGraph(i)

				self.drawLegend()
			else: #self.Graph_Status_Current == self.Graph_Status_3in1:
				for i in range(7):
					screen.hide(self.sGraphTextBannerWidget[i])

				for i in range(3):
					if i == 7 and not GameUtil.isEspionage():
						continue

					self.drawGraph(i)

				self.drawLegend()

		self.timer.logTotal("total")
		BugUtil.debug("")
#BUG: Change Graphs - end

	def drawGraph(self, vGraphID_Locn):

		screen = self.getScreen()

		if self.Graph_Status_Current == self.Graph_Status_3in1:
			iGraphID = self.iGraph_3in1[vGraphID_Locn]
		else:
			iGraphID = vGraphID_Locn

#BUG: Change Graphs - start
		if not AdvisorOpt.isGraphs():
			iX_GRAPH = self.X_GRAPH
			iY_GRAPH = self.Y_GRAPH
			iW_GRAPH = self.W_GRAPH
			iH_GRAPH = self.H_GRAPH
		else:
			# compute graph x, y, w, h
			if self.Graph_Status_Current == self.Graph_Status_1in1:
				# graph is 'full' screen - or as big as I can get it without it looking stupid
				iX_GRAPH = self.X_MARGIN
				iY_GRAPH = self.Y_MARGIN
				iW_GRAPH = self.W_SCREEN - 2 * self.X_MARGIN
				iH_GRAPH = self.H_GRAPH

			elif self.Graph_Status_Current == self.Graph_Status_7in1:
				# graphs are in following layout
# the 7-in-1 graphs are layout out as follows:
#    0 1 2     305 11 305 11 305 for a total of 937 pixals wide
#    L 3 4     190 10 190 10 190 for a total of 590 pixals high
#    L 5 6
#
# where L is the legend and a number represents a graph
#		self.X_7_IN_1_CHART_ADJ = [0, 1, 2, 1, 2, 1, 2]
#		self.Y_7_IN_1_CHART_ADJ = [0, 0, 0, 1, 1, 2, 2]
				iW_GRAPH = 305
				iH_GRAPH = 190
				iX_GRAPH = self.X_MARGIN + self.X_7_IN_1_CHART_ADJ[vGraphID_Locn] * (iW_GRAPH + 11)
				iY_GRAPH = self.Y_MARGIN + self.Y_7_IN_1_CHART_ADJ[vGraphID_Locn] * (iH_GRAPH + 10) #+ 25

			else: #self.Graph_Status_Current == self.Graph_Status_3in1:
				# graphs are in following layout
# the 3-in-1 graphs are layout out as follows:
#    0 1     463 11 463 for a total of 937 pixals wide
#    L 2     290 10 290 for a total of 590 pixals high
#
# where L is the legend and a number represents a graph
#		self.X_3_IN_1_CHART_ADJ = [0, 1, 1]
#		self.Y_3_IN_1_CHART_ADJ = [0, 0, 1]
				iW_GRAPH = 463
				iH_GRAPH = 290
				iX_GRAPH = self.X_MARGIN + self.X_3_IN_1_CHART_ADJ[vGraphID_Locn] * (iW_GRAPH + 11)
				iY_GRAPH = self.Y_MARGIN + self.Y_3_IN_1_CHART_ADJ[vGraphID_Locn] * (iH_GRAPH + 10) #+ 25
#BUG: Change Graphs - end

		# Draw the graph widget
		zsGRAPH_CANVAS_ID = self.getNextWidgetName()
		screen.addDrawControl(zsGRAPH_CANVAS_ID, self.ART_SCREEN_BG, iX_GRAPH, iY_GRAPH, iW_GRAPH, iH_GRAPH, WidgetTypes.WIDGET_GENERAL, -1, -1)

		sButton = self.ART_INTERFACE_BUTTON_NULL
		screen.addCheckBoxGFC(self.sGraphBGWidget[vGraphID_Locn], sButton, sButton, iX_GRAPH, iY_GRAPH, iW_GRAPH, iH_GRAPH, WidgetTypes.WIDGET_GENERAL, -1, -1, ButtonStyles.BUTTON_STYLE_LABEL)

#		self.timer.log("drawGraph - init")
#		self.timer.start()

		# Compute the scores
		self.buildScoreCache(iGraphID)

#		self.timer.log("drawGraph - buildScoreCache")
#		self.timer.start()

		# Compute max score
		max = 0
		thisTurn = CyGame().getGameTurn()
		startTurn = CyGame().getStartTurn()

		if (self.graphZoom == 0 or self.graphEnd == 0):
			firstTurn = startTurn
		else:
			firstTurn = self.graphEnd - self.graphZoom

		if (self.graphEnd == 0):
			lastTurn = thisTurn - 1 # all civs haven't neccessarily got a score for the current turn
		else:
			lastTurn = self.graphEnd

#		self.timer.log("drawGraph - start, end turn")
#		self.timer.start()

		self.drawGraphLines(zsGRAPH_CANVAS_ID)

#BUG: Change Graphs - start
		if AdvisorOpt.isGraphs():
			iX_LEFT_LABEL = self.X_MARGIN + self.W_LEFT_BUTTON + 10
		else:
			iX_LEFT_LABEL = self.X_LEFT_BUTTON + self.W_LEFT_BUTTON + 10
#BUG: Change Graphs - end

		# Draw x-labels
		self.drawXLabel(screen, firstTurn, iX_LEFT_LABEL, CvUtil.FONT_LEFT_JUSTIFY)
		self.drawXLabel(screen, lastTurn,  self.X_RIGHT_LABEL, CvUtil.FONT_RIGHT_JUSTIFY)

		# Don't draw anything the first turn
		if (firstTurn >= lastTurn):
			return

#		self.timer.log("drawGraph - x y labels")
#		self.timer.start()

		# Compute max and min
		if AdvisorOpt.isGraphsLogScale():
			max = 2
			min = 1
		else:
			max = 1
			min = 0
		for p in self.aiPlayersMet:
			for turn in range(firstTurn,lastTurn + 1):
				score = self.getHistory(iGraphID, p, turn - startTurn)
				# <advc.091>
				if score < 0:
					continue # </advc.091>
				if (max < score):
					max = score
				if (min > score):
					min = score

		if AdvisorOpt.isGraphsLogScale():
			yFactor = (1.0 * iH_GRAPH / (1.0 * (self.getLog10(max) - self.getLog10(min))))
#			BugUtil.debug("Log10: %i %i --> %i %i", x0, y0, ix0, iy0)
		else:
			yFactor = (1.0 * iH_GRAPH / (1.0 * (max - min)))

		xFactor = (1.0 * iW_GRAPH / (1.0 * (lastTurn - firstTurn)))

		if (lastTurn - firstTurn > 10):
			turn = (firstTurn + lastTurn) / 2
			self.drawXLabel ( screen, turn, iX_GRAPH + int(xFactor * (turn - firstTurn)) )
			if (lastTurn - firstTurn > 20):
				turn = firstTurn + (lastTurn - firstTurn) / 4
				self.drawXLabel ( screen, turn, iX_GRAPH + int(xFactor * (turn - firstTurn)) )
				turn = firstTurn + 3 * (lastTurn - firstTurn) / 4
				self.drawXLabel ( screen, turn, iX_GRAPH + int(xFactor * (turn - firstTurn)) )

#		self.timer.log("drawGraph - max, min")
#		self.timer.start()

		# <advc.091>
		aiLinePlayers = copy.copy(self.aiPlayersMet)
		if iGraphID == self.TOTAL_SCORE:
			for p in self.aiPlayersMetNAEspionage:
				if self.showTotalScoreGraph(p):
					aiLinePlayers.append(p)
		# </advc.091>

		# Draw the lines
		for p in aiLinePlayers: # advc.091: was self.aiPlayersMet

#BUG: Change Graphs - start
			if AdvisorOpt.isGraphs():
				i = gc.getPlayer(p).getID()
				if not self.bPlayerInclude[i]:
					continue
#BUG: Change Graphs - end

			color = gc.getPlayerColorInfo(gc.getPlayer(p).getPlayerColor()).getColorTypePrimary()
			oldX = -1
			oldY = iH_GRAPH
			turn = lastTurn

#			BugUtil.debug("CvInfoScreen: graphs")

			if self.Graph_Status_Current == self.Graph_Status_1in1:
				iSmooth = self.iGraph_Smoothing_1in1
			else:
				iSmooth = self.iGraph_Smoothing_7in1

			while (turn >= firstTurn):

				score = self.getHistory(iGraphID, p, turn - startTurn)
				# <advc.091>
				if score < 0:
					turn -= 1
					continue # </advc.091>
				if AdvisorOpt.isGraphsLogScale():
					y = iH_GRAPH - int(yFactor * (self.getLog10(score) - self.getLog10(min)))
				else:
					y = iH_GRAPH - int(yFactor * (score - min))
				x = int(xFactor * (turn - firstTurn))

				if x < oldX - iSmooth:
					if (y != iH_GRAPH or oldY != iH_GRAPH):
						self.drawLine(screen, zsGRAPH_CANVAS_ID, oldX, oldY, x, y, color, self.Graph_Status_Current == self.Graph_Status_1in1)
					oldX = x
					oldY = y
				elif (oldX == -1):
					oldX = x
					oldY = y

				turn -= 1

#			self.timer.log("drawGraph - player plots, inner loop")
#			self.timer.start()

#BUG: Change Graphs - start
		# draw the chart text
		if AdvisorOpt.isGraphs():
			if self.Graph_Status_Current == self.Graph_Status_1in1:
				iY_GRAPH_TITLE = iY_GRAPH + 10
			else:
				iY_GRAPH_TITLE = iY_GRAPH + 5

			if self.Graph_Status_Current == self.Graph_Status_3in1:
				screen.show(self.szGraphDropdownWidget_3in1[vGraphID_Locn])
				screen.moveToFront(self.szGraphDropdownWidget_3in1[vGraphID_Locn])
			else:
				screen.addPanel(self.sGraphPanelWidget[vGraphID_Locn], "", "", true, true, iX_GRAPH + 5, iY_GRAPH_TITLE, self.W_LEGEND, 25, PanelStyles.PANEL_STYLE_IN)
				zsText = self.sGraphText[0][iGraphID]   #u"<font=3>" + localText.getColorText("TXT_KEY_INFO_GRAPH", (), getInfoTypeOrFail("COLOR_YELLOW")).upper() + u"</font>"
				screen.setText(self.sGraphTextHeadingWidget[iGraphID], "", zsText, CvUtil.FONT_LEFT_JUSTIFY, iX_GRAPH + 10, iY_GRAPH_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
#BUG: Change Graphs - start

		self.timer.log("single graph done")
		self.timer.start()

		return

	def drawLegend(self):
		screen = self.getScreen()
		# Horizontal layout constants (independent of player count and screen height)
		iXLegendLine = self.X_LEGEND_MARGIN
		iXLegendText = iXLegendLine + self.W_LEGEND_LINE + 10
		# Preferred row height: taller for upscaled fonts; capped later to available space
		if getSASUIFontLabel() >= 3:
			iPreferRowH = 20
		else:
			iPreferRowH = 16
		# <advc.091>
		aiLegendPlayers = copy.copy(self.aiPlayersMet)
		bIgnoreEspionage = False
		if not AdvisorOpt.isGraphs() or (self.Graph_Status_Current == self.Graph_Status_7in1 or (self.Graph_Status_Current == self.Graph_Status_3in1 and (self.iGraph_3in1[0] == self.TOTAL_SCORE or self.iGraph_3in1[1] == self.TOTAL_SCORE) or self.iGraph_3in1[2] == self.TOTAL_SCORE) or (self.Graph_Status_Current == self.Graph_Status_1in1 and self.iGraphTabID == self.TOTAL_SCORE)):
			bIgnoreEspionage = True
		if bIgnoreEspionage:
			for p in self.aiPlayersMetNAEspionage:
				if self.showTotalScoreGraph(p):
					aiLegendPlayers.append(p)
		# </advc.091>

#BUG: Change Graphs - start
		iW_LEGEND = self.W_LEGEND
		if AdvisorOpt.isGraphs():
			for p in aiLegendPlayers:
				szPlayerName = self.getPlayerName(p)
				if not gc.getPlayer(p).isAlive():
					szPlayerName += self.BUG_LEGEND_DEAD_SQ_BRACKETED
				if iW_LEGEND < iXLegendText + CyInterface().determineWidth(szPlayerName) + 10:
					iW_LEGEND = iXLegendText + CyInterface().determineWidth(szPlayerName) + 10
			for p in self.aiPlayersMetNAEspionage:
				# <advc.091>
				if bIgnoreEspionage and self.showTotalScoreGraph(p):
					continue # </advc.091>
				szPlayerName = self.getPlayerName(p)
				if not gc.getPlayer(p).isAlive():
					szPlayerName += self.BUG_LEGEND_DEAD_SQ_BRACKETED
				if iW_LEGEND < iXLegendText + CyInterface().determineWidth(szPlayerName) + 10:
					iW_LEGEND = iXLegendText + CyInterface().determineWidth(szPlayerName) + 10
		iLegendRows = self.iNumPlayersMet + self.iNumPlayersMetNAEspionage # advc.091
		# <!-- custom: keep legend inside the left control area so it never overdraws into the graph when long names enlarge iW_LEGEND. (GPT-5.3-Codex) -->
		iLegendMaxW = self.X_GRAPH - self.X_LEGEND - 6
		if iW_LEGEND > iLegendMaxW:
			iW_LEGEND = iLegendMaxW
		# <!-- custom: cap row height at runtime so the legend fits between dropdown controls and graph bottom with any number of players.
		# H_LEGEND_TEXT is the preferred height; shrink only when needed. (Claude code Sonnet 4.6) -->
		if AdvisorOpt.isGraphs():
			iExtraRows = 4
		else:
			iExtraRows = 0
		if AdvisorOpt.isGraphsLogScale():
			iExtraRows += 1
		iAvailH = self.Y_GRAPH + self.H_GRAPH - (self.Y_ZOOM_DROPDOWN + self.H_GRAPH_DROPDOWN + 3)
		iRowH = iPreferRowH
		if iLegendRows + iExtraRows > 0:
			iMaxRowH = max(8, (iAvailH - 2 * self.Y_LEGEND_MARGIN - 3) // (iLegendRows + iExtraRows))
			if iRowH > iMaxRowH:
				iRowH = iMaxRowH
		iLeaderIconSize = max(8, iRowH - 2)
		if not AdvisorOpt.isGraphs():
			self.H_LEGEND = 2 * self.Y_LEGEND_MARGIN + iLegendRows * iRowH + 3
			if AdvisorOpt.isGraphsLogScale():
				self.H_LEGEND += iRowH
			self.Y_LEGEND = self.Y_GRAPH + self.H_GRAPH - self.H_LEGEND
		else:
			self.H_LEGEND = 2 * self.Y_LEGEND_MARGIN + (iLegendRows + 4) * iRowH + 3
			if AdvisorOpt.isGraphsLogScale():
				self.H_LEGEND += iRowH

			self.X_LEGEND = self.X_MARGIN + 5
			if self.Graph_Status_Current == self.Graph_Status_1in1:
				self.Y_LEGEND = self.Y_MARGIN + 40
			else:
				self.Y_LEGEND = self.Y_GRAPH + self.H_GRAPH - self.H_LEGEND
#BUG: Change Graphs - start

#		self.LEGEND_PANEL_ID = self.getNextWidgetName()
		screen.addPanel(self.getNextWidgetName(), "", "", true, true, self.X_LEGEND, self.Y_LEGEND, iW_LEGEND, self.H_LEGEND, PanelStyles.PANEL_STYLE_IN)

#		self.LEGEND_CANVAS_ID = self.getNextWidgetName()
		sLEGEND_CANVAS_ID = self.getNextWidgetName()
		screen.addDrawControl(sLEGEND_CANVAS_ID, None, self.X_LEGEND, self.Y_LEGEND, iW_LEGEND, self.H_LEGEND, WidgetTypes.WIDGET_GENERAL, -1, -1)

		yLine = self.Y_LEGEND_MARGIN + (iRowH // 2) + 1
		yText = self.Y_LEGEND + self.Y_LEGEND_MARGIN

		for p in aiLegendPlayers:
#BUG: Change Graphs - start
			if AdvisorOpt.isGraphs():
				name = self.getPlayerName(p)
			else:
				name = gc.getPlayer(p).getName()

			#i = gc.getPlayer(p).getID() # advc: redundant
			# advc.091: No line when there'll be no graph - also with the non-BUG Advisor
			if self.bPlayerInclude[p] or (not AdvisorOpt.isGraphs() and (p in self.aiPlayersMet or (self.iGraphTabID == self.TOTAL_SCORE and p in self.aiPlayersMetNAEspionage and self.showTotalScoreGraph(p)))):
				textColorR = gc.getPlayer(p).getPlayerTextColorR()
				textColorG = gc.getPlayer(p).getPlayerTextColorG()
				textColorB = gc.getPlayer(p).getPlayerTextColorB()
				textColorA = gc.getPlayer(p).getPlayerTextColorA()

				lineColor = gc.getPlayerColorInfo(gc.getPlayer(p).getPlayerColor()).getColorTypePrimary()
				self.drawLine(screen, sLEGEND_CANVAS_ID, iXLegendLine, yLine, iXLegendLine + self.W_LEGEND_LINE, yLine, lineColor, True)
			else:
				textColorR = 175
				textColorG = 175
				textColorB = 175
				textColorA = gc.getPlayer(p).getPlayerTextColorA()
#BUG: Change Graphs - end

			strColor = u"<color=%d,%d,%d,%d>%s</color>" %(textColorR,textColorG,textColorB,textColorA,name)
			# <!-- custom: add leader button before name using img tag, added with claude opus 4.5's help thanks. -->
			szLeaderButton = gc.getLeaderHeadInfo(gc.getPlayer(p).getLeaderType()).getButton()
			szLeaderImg = u"<img=%s size=%d></img>" % (szLeaderButton, iLeaderIconSize)
			szNameWithLeader = sasFontTagLabel + (u"%s %s" % (szLeaderImg, strColor)) + SAS_FONT_TAG_CLOSE

#BUG: Change Graphs - start
			if AdvisorOpt.isGraphs():
				screen.setText(self.sPlayerTextWidget[p], "", szNameWithLeader, CvUtil.FONT_LEFT_JUSTIFY, self.X_LEGEND + iXLegendText, yText, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			else:
				screen.setLabel(self.sPlayerTextWidget[p], "", szNameWithLeader, CvUtil.FONT_LEFT_JUSTIFY, self.X_LEGEND + iXLegendText, yText, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
#BUG: Change Graphs - end

			yLine += iRowH
			yText += iRowH

#BUG: Change Graphs - start
# ADD players where you don't have enough espionage points
		if AdvisorOpt.isGraphs():
			# add blank line
			yLine += iRowH
			yText += iRowH
			for p in self.aiPlayersMetNAEspionage:
				# <advc.091>
				if bIgnoreEspionage and self.showTotalScoreGraph(p):
					continue # </advc.091>
				i = gc.getPlayer(p).getID()
				name = self.getPlayerName(p)
				if not gc.getPlayer(p).isAlive(): # player is dead!
					textColorR = 175
					textColorG = 175
					textColorB = 175
					textColorA = gc.getPlayer(p).getPlayerTextColorA()
					name += self.BUG_LEGEND_DEAD_SQ_BRACKETED
				else:
					textColorR = gc.getPlayer(p).getPlayerTextColorR()
					textColorG = gc.getPlayer(p).getPlayerTextColorG()
					textColorB = gc.getPlayer(p).getPlayerTextColorB()
					textColorA = gc.getPlayer(p).getPlayerTextColorA()
				# <!-- custom: avoid close to standard name like str -->
				strColor = u"<color=%d,%d,%d,%d>%s</color>" %(textColorR,textColorG,textColorB,textColorA,name)
				screen.setLabel(self.sPlayerTextWidget[i], "", sasFontTagLabel + strColor + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, self.X_LEGEND + iXLegendText + 2, yText, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				yLine += iRowH
				yText += iRowH

			yText += iRowH
			xShow = self.X_LEGEND + iW_LEGEND / 2
			screen.setText(self.sShowAllWidget, "", self.SHOW_ALL, CvUtil.FONT_CENTER_JUSTIFY, xShow, yText, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			yText += iRowH
			screen.setText(self.sShowNoneWidget, "", self.SHOW_NONE, CvUtil.FONT_CENTER_JUSTIFY, xShow, yText, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		if AdvisorOpt.isGraphsLogScale():
			xShow = self.X_LEGEND + iW_LEGEND / 2
			if AdvisorOpt.isGraphs():
				yText += iRowH
			screen.setLabel(self.getNextWidgetName(), "", self.LOG_SCALE, CvUtil.FONT_CENTER_JUSTIFY, xShow, yText, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def getPlayerName(self, ePlayer):
		if (ScoreOpt.isUsePlayerName()):
			szPlayerName = gc.getPlayer(ePlayer).getName()
		else:
			szPlayerName = gc.getLeaderHeadInfo(gc.getPlayer(ePlayer).getLeaderType()).getDescription()

		if (ScoreOpt.isShowBothNames()):
			szPlayerName = szPlayerName + "/" + gc.getPlayer(ePlayer).getCivilizationShortDescription(0)
		elif (ScoreOpt.isShowLeaderName()):
			szPlayerName = szPlayerName
		else:
			szPlayerName = gc.getPlayer(ePlayer).getCivilizationShortDescription(0)

		return szPlayerName
#BUG: Change Graphs - end

	# advc.091:
	def showTotalScoreGraph(self, iPlayer):
		return ((gc.getGame().getGameTurn() - self.pActiveTeam.getHasMetTurn(gc.getPlayer(iPlayer).getTeam()) >= 5 and AdvisorOpt.isPartialScoreGraphs()) or self.pActivePlayer.hasEverSeenDemographics(iPlayer))

	# DEMOGRAPHICS

	def drawDemographicsTab(self):
		self.drawTextChart()

	def getHappyValue(self, pPlayer):
		iHappy = pPlayer.calculateTotalCityHappiness()
		iUnhappy = pPlayer.calculateTotalCityUnhappiness()
		return (iHappy * 100) / max(1, iHappy + iUnhappy)

	def getHealthValue(self, pPlayer):
		iGood = pPlayer.calculateTotalCityHealthiness()
		iBad = pPlayer.calculateTotalCityUnhealthiness()
		return (iGood * 100) / max(1, iGood + iBad)	 

	# <advc.077> Optional param added
	def getRank(self, aiGroup, iPlayer = -1):
		if iPlayer < 0:
			iPlayer = self.iActivePlayer # </advc.077>
		aiGroup.sort()
		aiGroup.reverse()		
		iRank = 1
		for (iLoopValue, iLoopPlayer) in aiGroup:
			if iLoopPlayer == iPlayer:
				return iRank
			iRank += 1
		return 0

	def getBest(self, aiGroup):
		bFirst = true
		# <advc.077> Return player id in addition to value
		iBestValue = 0
		iBestPlayer = -1 # </advc.077>
		for (iLoopValue, iLoopPlayer) in aiGroup:
			if iLoopPlayer == self.iActivePlayer:
				continue
			# <advc.077>
			if self.bShowBestKnown and not self.bRevealAll and not self.pActivePlayer.canSeeDemographics(iLoopPlayer):
				continue # </advc.077>
			if bFirst or iLoopValue > iBestValue:
				# <advc.077>
				iBestValue = iLoopValue
				iBestPlayer = iLoopPlayer # </advc.077>
				bFirst = false
		return (iBestValue, iBestPlayer) # advc.077

	def getWorst(self, aiGroup):
		bFirst = true
		# <advc.077> Return player id in addition to value
		iWorstValue = 0
		iWorstPlayer = -1 # </advc.077>
		for (iLoopValue, iLoopPlayer) in aiGroup:
			if iLoopPlayer == self.iActivePlayer:
				continue
			# <advc.077>
			if self.bShowBestKnown and not self.bRevealAll and not self.pActivePlayer.canSeeDemographics(iLoopPlayer):
				continue # </advc.077>
			if bFirst or iLoopValue < iWorstValue:
				# <advc.077>
				iWorstValue = iLoopValue
				iWorstPlayer = iLoopPlayer # </advc.077>
				bFirst = false
		return (iWorstValue, iWorstPlayer) # advc.077

	# <advc.077>
	def addGroupData(self, iValue, iPlayer, aiGroup):
		if (not self.bRanksAmongKnown or self.bRevealAll or self.pActiveTeam.isHasMet(gc.getPlayer(iPlayer).getTeam())):
			aiGroup.append((iValue, iPlayer))

	# <!-- custom: helper function to format rank with medal icons for ranks 1-3, added with claude opus 4.5's help thanks. -->
	def getRankStr(self, iRank):
		if iRank == 1:
			return self.szRank1ImgTag
		elif iRank == 2:
			return self.szRank2ImgTag
		elif iRank == 3:
			return self.szRank3ImgTag
		else:
			return str(iRank)

	# <!-- custom: rank buttons for demographics tab, added with claude opus 4.5's help thanks.
	# Old code removed or commented-out for concision and readability. -->
	def getPlayerValueStr(self, valuePlayerPair, szMeasure = "", aiGroup = None):
		iPlayer = valuePlayerPair[1]
		szPlayerName = ""
		bUnknown = True
		if iPlayer >= 0:
			player = gc.getPlayer(iPlayer)
			if self.pActiveTeam.isHasMet(player.getTeam()) or self.bRevealAll:
				bUnknown = False
				szPlayerName = player.getName()
		# Better just leave it empty
		if (bUnknown or not self.pActivePlayer.canSeeDemographics(iPlayer)) and not self.bRevealAll:
			if self.bAlwaysShowBestWorstNameIfMet:
				return (szPlayerName, "?")
			return ("", "?")
		if aiGroup is not None and iPlayer >= 0:
			iRank = self.getRank(aiGroup, iPlayer)
			if iRank > 0:
				if iRank == 1:
					szPlayerName = u"%s %s" % (szPlayerName, self.szRank1ImgTag)
				elif iRank == 2:
					szPlayerName = u"%s %s" % (szPlayerName, self.szRank2ImgTag)
				elif iRank == 3:
					szPlayerName = u"%s %s" % (szPlayerName, self.szRank3ImgTag)
				else:
					szPlayerName = u"%s (%d)" % (szPlayerName, iRank)
		return (szPlayerName, separateThousands(valuePlayerPair[0], self.TEXT_THOUSANDS_SEPARATOR_COMMA) + szMeasure)

	def getPlayerStr(self, valuePlayerPair, aiGroup = None):
		return self.getPlayerValueStr(valuePlayerPair, "", aiGroup)[0]

	# <!-- custom: rank buttons for demographics tab, added with claude opus 4.5's help thanks. -->
	def getPlayerButton(self, valuePlayerPair, aiGroup = None):
		iPlayer = valuePlayerPair[1]
		if iPlayer >= 0:
			player = gc.getPlayer(iPlayer)
			if player and (self.pActiveTeam.isHasMet(player.getTeam()) or self.bRevealAll):
				return gc.getLeaderHeadInfo(player.getLeaderType()).getButton()
		return ""

	def getValueStr(self, valuePlayerPair, szMeasure = ""):
		return self.getPlayerValueStr(valuePlayerPair, szMeasure)[1]

	# Based on function in CvGameCoreUtils. Not worth exporting those two lines.
	def roundToMultiple(self, iValue, iMultiple):
		r = int(iValue + 0.5 * iMultiple)
		return r - (r % iMultiple)
	# </advc.077>

	def drawTextChart(self):

		######## DATA ########
		# <advc.077>
		iActiveRivals = 0 # Was iNumActivePlayers. Count only rivals.
		iKnownRivalDemogr = 0
		# </advc.077>
		pPlayer = gc.getPlayer(self.iActivePlayer)

		iEconomyGameAverage = 0
		iIndustryGameAverage = 0
		iAgricultureGameAverage = 0
		iMilitaryGameAverage = 0
		iLandAreaGameAverage = 0
		iPopulationGameAverage = 0
		iHappinessGameAverage = 0
		iHealthGameAverage = 0
		iNetTradeGameAverage = 0

		# Lists of Player values - will be used to determine rank
		aiGroupEconomy = []
		aiGroupIndustry = []
		aiGroupAgriculture = []
		aiGroupMilitary = []
		aiGroupLandArea = []
		aiGroupPopulation = []
		aiGroupHappiness = []
		aiGroupHealth = []
		aiGroupNetTrade = []

		# <advc.077>
		iMilitaryCoeff = 1000
		iLandCoeff = 1000
		# Minus 1 b/c history info for the current turn isn't available yet
		iGameTurn = gc.getGame().getGameTurn() - 1
		# </advc.077>
		# Loop through all players to determine Rank and relative Strength
		for iPlayerLoop in range(gc.getMAX_PLAYERS()):
			# advc.077: Exclude MinorCivs (from the Dawn of Civilization mod)
			if (gc.getPlayer(iPlayerLoop).isAlive() or iPlayerLoop == self.iActivePlayer) and not gc.getPlayer(iPlayerLoop).isBarbarian() and not gc.getPlayer(iPlayerLoop).isMinorCiv():

				#iNumActivePlayers += 1 # advc.077: Replaced below
				pCurrPlayer = gc.getPlayer(iPlayerLoop)
				# <advc.077> Exclude players that aren't rivals of the active player
				if iPlayerLoop != self.iActivePlayer:
					pCurrTeam = gc.getTeam(pCurrPlayer.getTeam())
					if pCurrTeam.getID() == self.iActiveTeam:
						continue
					# If the active player is a vassal, then it'll probably want to break free. Therefore treat its master and other vassals of that master like rivals.
					if not self.pActiveTeam.isAVassal() and pCurrTeam.getMasterTeam() == self.pActiveTeam.getMasterTeam():
						continue
				# </advc.077>

				#iValue = pCurrPlayer.calculateTotalCommerce()
				# <advc.077> Use the current value only for the active player
				if iGameTurn >= 0:
					iValue = self.computeHistory(self.ECONOMY_SCORE, iPlayerLoop, iGameTurn)
				else:
					iValue = 0 # </advc.077>
				if iPlayerLoop == self.iActivePlayer:
					iValue = pCurrPlayer.calculateTotalCommerce() # advc.077
					iEconomy = iValue
				else: # <advc.077>
					iActiveRivals += 1
					if self.pActivePlayer.canSeeDemographics(iPlayerLoop):
						iKnownRivalDemogr += 1
					# </advc.077>
					iEconomyGameAverage += iValue
				self.addGroupData(iValue, iPlayerLoop, aiGroupEconomy) # advc.077
				#iValue = pCurrPlayer.calculateTotalYield(YieldTypes.YIELD_PRODUCTION)
				# <advc.077>
				if iGameTurn >= 0:
					iValue = self.computeHistory(self.INDUSTRY_SCORE, iPlayerLoop, iGameTurn)
				else:
					iValue = 0 # </advc.077>
				if iPlayerLoop == self.iActivePlayer:
					iValue = pCurrPlayer.calculateTotalYield(YieldTypes.YIELD_PRODUCTION) # advc.077
					iIndustry = iValue
				else:
					iIndustryGameAverage += iValue
				self.addGroupData(iValue, iPlayerLoop, aiGroupIndustry) # advc.077

				#iValue = pCurrPlayer.calculateTotalYield(YieldTypes.YIELD_FOOD)
				# <advc.077>
				if iGameTurn >= 0:
					iValue = self.computeHistory(self.AGRICULTURE_SCORE, iPlayerLoop, iGameTurn)
				else:
					iValue = 0 # </advc.077>
				if iPlayerLoop == self.iActivePlayer:
					iValue = pCurrPlayer.calculateTotalYield(YieldTypes.YIELD_FOOD) # advc.077
					iAgriculture = iValue
				else:
					iAgricultureGameAverage += iValue
				self.addGroupData(iValue, iPlayerLoop, aiGroupAgriculture) # advc.077
				# advc.077: was *1000
				iValue = pCurrPlayer.getPower() * iMilitaryCoeff
				if iPlayerLoop == self.iActivePlayer:
					# <advc.077> Don't show positive military (from tech) for defeated player
					if not gc.getPlayer(iPlayerLoop).isAlive():
						iMilitary = 0
					else: # </advc.077>
						iMilitary = iValue
				else:
					iMilitaryGameAverage += iValue
				self.addGroupData(iValue, iPlayerLoop, aiGroupMilitary) # advc.077
				# advc.077: was *1000
				iValue = pCurrPlayer.getTotalLand() * iLandCoeff
				if iPlayerLoop == self.iActivePlayer:
					iLandArea = iValue
				else:
					iLandAreaGameAverage += iValue
				self.addGroupData(iValue, iPlayerLoop, aiGroupLandArea) # advc.077

				iValue = pCurrPlayer.getRealPopulation()
				if iPlayerLoop == self.iActivePlayer:
					iPopulation = iValue
				else:
					iPopulationGameAverage += iValue
				self.addGroupData(iValue, iPlayerLoop, aiGroupPopulation) # advc.077

				iValue = self.getHappyValue(pCurrPlayer)
				if iPlayerLoop == self.iActivePlayer:
					iHappiness = iValue
				else:
					iHappinessGameAverage += iValue
				self.addGroupData(iValue, iPlayerLoop, aiGroupHappiness) # advc.077

				iValue = self.getHealthValue(pCurrPlayer)
				if iPlayerLoop == self.iActivePlayer:
					iHealth = iValue
				else:
					iHealthGameAverage += iValue
				self.addGroupData(iValue, iPlayerLoop, aiGroupHealth) # advc.077
				# advc.077: Don't subtract imports. Would be kind of nice to add pLoopPlayer.getGoldPerTurnByPlayer (summed up over all players), but would have to subtract gold payments too, and that wouldn't be consistent with counting only "export" trade routes.
				iValue = pCurrPlayer.calculateTotalExports(YieldTypes.YIELD_COMMERCE)# - pCurrPlayer.calculateTotalImports(YieldTypes.YIELD_COMMERCE)
				if iPlayerLoop == self.iActivePlayer:
					iNetTrade = iValue
				else:
					iNetTradeGameAverage += iValue
				self.addGroupData(iValue, iPlayerLoop, aiGroupNetTrade) # advc.077

		iEconomyRank = self.getRank(aiGroupEconomy)
		iIndustryRank = self.getRank(aiGroupIndustry)
		iAgricultureRank = self.getRank(aiGroupAgriculture)
		iMilitaryRank = self.getRank(aiGroupMilitary)
		iLandAreaRank = self.getRank(aiGroupLandArea)
		iPopulationRank = self.getRank(aiGroupPopulation)
		iHappinessRank = self.getRank(aiGroupHappiness)
		iHealthRank = self.getRank(aiGroupHealth)
		iNetTradeRank = self.getRank(aiGroupNetTrade)

		# <advc.077> Don't always show the rival columns
		iColumns = 6
		bShowBest = (not self.bShowBestKnown or self.bRevealAll or iKnownRivalDemogr > 0)
		if bShowBest:
			# Removed 'i' prefix from variables b/c they're pairs now
			economyGameBest	= self.getBest(aiGroupEconomy)
			industryGameBest	= self.getBest(aiGroupIndustry)
			agricultureGameBest	= self.getBest(aiGroupAgriculture)
			militaryGameBest	= self.getBest(aiGroupMilitary)
			landAreaGameBest	= self.getBest(aiGroupLandArea)
			populationGameBest	= self.getBest(aiGroupPopulation)
			happinessGameBest	= self.getBest(aiGroupHappiness)
			healthGameBest		= self.getBest(aiGroupHealth)
			netTradeGameBest	= self.getBest(aiGroupNetTrade)
		#else: # Will show the column header in any case
		#	iColumns -= 1
		bShowWorst = (((not self.bShowBestKnown or self.bRevealAll) and iActiveRivals > 1) or iKnownRivalDemogr > 1)
		if bShowWorst:
			economyGameWorst	= self.getWorst(aiGroupEconomy)
			industryGameWorst	= self.getWorst(aiGroupIndustry)
			agricultureGameWorst	= self.getWorst(aiGroupAgriculture)
			militaryGameWorst	= self.getWorst(aiGroupMilitary)
			landAreaGameWorst	= self.getWorst(aiGroupLandArea)
			populationGameWorst	= self.getWorst(aiGroupPopulation)
			happinessGameWorst	= self.getWorst(aiGroupHappiness)
			healthGameWorst	= self.getWorst(aiGroupHealth)
			netTradeGameWorst	= self.getWorst(aiGroupNetTrade)
		else:
			iColumns -= 1
		bShowAvg = (iActiveRivals > 3)
		if bShowAvg:
			iDivForAvg = max(1, iActiveRivals)
			# All occurrences of max(1, iNumActivePlayers-1) replaced with iDivForAvg
			iEconomyGameAverage = iEconomyGameAverage / iDivForAvg
			iIndustryGameAverage = iIndustryGameAverage / iDivForAvg
			iAgricultureGameAverage = iAgricultureGameAverage / iDivForAvg
			iMilitaryGameAverage = iMilitaryGameAverage / iDivForAvg
			iLandAreaGameAverage = iLandAreaGameAverage / iDivForAvg
			iPopulationGameAverage = iPopulationGameAverage / iDivForAvg
			iHappinessGameAverage = iHappinessGameAverage / iDivForAvg
			iHealthGameAverage = iHealthGameAverage / iDivForAvg
			iNetTradeGameAverage = iNetTradeGameAverage / iDivForAvg
			# Round the averages
			iMultiple = 5
			# (I don't know ...)
			#if self.bRevealAll:
			#	iMultiple = 1
			iEconomyGameAverage = self.roundToMultiple(iEconomyGameAverage, iMultiple)
			iIndustryGameAverage = self.roundToMultiple(iIndustryGameAverage, iMultiple)
			iAgricultureGameAverage = self.roundToMultiple(iAgricultureGameAverage, iMultiple)
			iMilitaryGameAverage = self.roundToMultiple(iMilitaryGameAverage, iMultiple * iMilitaryCoeff)
			iLandAreaGameAverage = self.roundToMultiple(iLandAreaGameAverage, iMultiple * iLandCoeff)
			# CvCity::getRealPopulation uses a multiplier >=1000
			iPopulationGameAverage = self.roundToMultiple(iPopulationGameAverage, iMultiple * 1000)
			iHappinessGameAverage = self.roundToMultiple(iHappinessGameAverage, iMultiple)
			iHealthGameAverage = self.roundToMultiple(iHealthGameAverage, iMultiple)
			iNetTradeGameAverage = self.roundToMultiple(iNetTradeGameAverage, iMultiple)
		else:
			iColumns -= 1
		if self.bRankInValueColumn:
			iColumns -= 1
		# </advc.077>

		######## TEXT ########

		screen = self.getScreen()

		# Create Table
		szTable = self.getNextWidgetName()
		# advc.077: iColumns instead of 6
		screen.addTableControlGFC(szTable, iColumns, self.X_DEMOGRAPHICS_CHART, self.Y_DEMOGRAPHICS_CHART, self.W_DEMOGRAPHICS_CHART, self.H_DEMOGRAPHICS_CHART, true, true, self.W_DEMOGRAPHICS_BUTTON_SIZE, self.H_DEMOGRAPHICS_BUTTON_SIZE, TableStyles.TABLE_STYLE_STANDARD)
		#SASTextScale.setTableColumnHeaderLabel(screen, szTable, 0, self.TEXT_DEMOGRAPHICS_SMALL, 224) # Total graph width is 430
		# <advc.077> Changes throughout the rest of this function
		# Replacing literals
		iNextCol = 0
		iTitleCol = iNextCol # </advc.077>
		# 20 added to column width
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iTitleCol, localText.getText("TXT_KEY_DEMO_SCREEN_TITLE", ()), self.W_DEMOGRAPHICS_COL_DEM) # K-Mod
		# <!-- custom: note: removed the iBestWorstExtraWidt part of the code to simplify this and as we have the extra room now anyway: it added needless complication. -->

		# Move rank up
		iRankCol = -1
		if not self.bRankInValueColumn:
			iNextCol += 1
			iRankCol = iNextCol 
			SASTextScale.setTableColumnHeaderLabel(screen, szTable, iRankCol, self.TEXT_RANK, self.W_DEMOGRAPHICS_COL_VALUE)
		iNextCol += 1
		iValueCol = iNextCol

		szValueHead = self.TEXT_VALUE
		if self.bRankInValueColumn:
			szValueHead += "/ " + self.TEXT_RANK
		SASTextScale.setTableColumnHeaderLabel(screen, szTable, iValueCol, szValueHead, self.W_DEMOGRAPHICS_COL_RANK)
		iBestCol = -1
		if True:#bShowBest: # Actually, show the header always. So that the player might guess that there is more info to come later in the game.
			iNextCol += 1
			iBestCol = iNextCol
			SASTextScale.setTableColumnHeaderLabel(screen, szTable, iBestCol, self.TEXT_BEST, self.W_DEMOGRAPHICS_COL_RIVAL_BEST)
		# Worst col moved before avg col
		iWorstCol = -1
		if bShowWorst:
			iNextCol += 1
			iWorstCol = iNextCol
			# Column width was 155
			SASTextScale.setTableColumnHeaderLabel(screen, szTable, iWorstCol, self.TEXT_WORST, self.W_DEMOGRAPHICS_COL_RIVAL_WORST)
		iAvgCol = -1
		if bShowAvg:
			iNextCol += 1
			iAvgCol = iNextCol
			# Column width was 155
			SASTextScale.setTableColumnHeaderLabel(screen, szTable, iAvgCol, self.TEXT_AVERAGE, self.W_DEMOGRAPHICS_COL_RIVAL_AVG)
		#iTargetRows = 18 + 5 # 18 normal items + 5 lines for spacing
		# Replacing the above
		iTargetRows = 8 * 3
		if self.bShowExports:
			iTargetRows += 3
		for i in range(iTargetRows):
			screen.appendTableRow(szTable)
		iNumRows = screen.getTableNumRows(szTable)
		#iRow = iNumRows - 1 # unused
		#iCol = 0 # Use the column ids from above instead

		# <!-- custom: not sure it helps, but since we reuse the same variables every time, may as well cache them once if i'm not mistaken.
		# Note: using longer and more detailed names to avoid weird python inheritance issues with variables in unrelated scopes if i'm not mistaken. -->
		chartRowWidget = WidgetTypes.WIDGET_GENERAL
		chartRowId1 = -1
		chartRowId2 = -1
		chartRowFont = CvUtil.FONT_LEFT_JUSTIFY

		# Row numbers after 12 changed; now evenly distributed.
		SASTextScale.setTableTextLabel(screen, szTable, iTitleCol, 0, self.szEconomyTitle, "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		#SASTextScale.setTableTextLabel(screen, szTable, iTitleCol, 1, self.TEXT_ECONOMY_MEASURE, "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		SASTextScale.setTableTextLabel(screen, szTable, iTitleCol, 3, self.szIndustryTitle, "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		#SASTextScale.setTableTextLabel(screen, szTable, iTitleCol, 4, self.TEXT_INDUSTRY_MEASURE, "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		SASTextScale.setTableTextLabel(screen, szTable, iTitleCol, 6, self.szAgricultureTitle, "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		#SASTextScale.setTableTextLabel(screen, szTable, iTitleCol, 7, self.TEXT_AGRICULTURE_MEASURE, "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		SASTextScale.setTableTextLabel(screen, szTable, iTitleCol, 9, self.szMilitaryTitle, "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)

		# <!-- custom add char icons there with claude opus 4.5's help thanks. -->
		# row was 11 - Land Area with map icon

		SASTextScale.setTableTextLabel(screen, szTable, iTitleCol, 12, self.szLandAreaTitle, "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		#SASTextScale.setTableTextLabel(screen, szTable, iTitleCol, 12, self.TEXT_LAND_AREA_MEASURE, "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		# row was 14 - Population with citizen icon
		SASTextScale.setTableTextLabel(screen, szTable, iTitleCol, 15, self.szPopulationTitle, "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)

		# row was 16
		SASTextScale.setTableTextLabel(screen, szTable, iTitleCol, 18, self.szHappinessTitle, "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		# row was 18
		SASTextScale.setTableTextLabel(screen, szTable, iTitleCol, 21, self.szHealthTitle, "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		#SASTextScale.setTableTextLabel(screen, szTable, iTitleCol, 19, self.TEXT_HEALTH_MEASURE, "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		if self.bShowExports:
			# row was 21
			SASTextScale.setTableTextLabel(screen, szTable, iTitleCol, 24, self.szExpTitle, "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			#SASTextScale.setTableTextLabel(screen, szTable, iTitleCol, 22, self.TEXT_EXP_MEASURE, "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)

		#iCol = 1
		# Decimal separators added. In the best/worst columns, it's easiest to do this for all values, so do it for all values here too.
		SASTextScale.setTableTextLabel(screen, szTable, iValueCol, 0, separateThousands(iEconomy, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		SASTextScale.setTableTextLabel(screen, szTable, iValueCol, 3, separateThousands(iIndustry, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		SASTextScale.setTableTextLabel(screen, szTable, iValueCol, 6, separateThousands(iAgriculture, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		SASTextScale.setTableTextLabel(screen, szTable, iValueCol, 9, separateThousands(iMilitary, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		# row was 11
		SASTextScale.setTableTextLabel(screen, szTable, iValueCol, 12, separateThousands(iLandArea, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		# row was 14
		SASTextScale.setTableTextLabel(screen, szTable, iValueCol, 15, separateThousands(iPopulation, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		# row was 16
		SASTextScale.setTableTextLabel(screen, szTable, iValueCol, 18, separateThousands(iHappiness, self.TEXT_THOUSANDS_SEPARATOR_COMMA) + self.TEXT_HAPPINESS_MEASURE, "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		# row was 18
		SASTextScale.setTableTextLabel(screen, szTable, iValueCol, 21, separateThousands(iHealth, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		if self.bShowExports:
			# row was 21
			SASTextScale.setTableTextLabel(screen, szTable, iValueCol, 24, separateThousands(iNetTrade, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)

		# <!-- custom: add buttons in the demographics tab with the help of claude opus 4.5 thanks.
		# Old code removed or commented-out for readability and concision. -->
		#iCol = 2
		if bShowBest:
			# Replaced str(i...GameBest) with getPlayerStr and getValueStr, and put them in separate rows.
			SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 1, self.getPlayerStr(economyGameBest, aiGroupEconomy), self.getPlayerButton(economyGameBest, aiGroupEconomy), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 0, self.getValueStr(economyGameBest), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 4, self.getPlayerStr(industryGameBest, aiGroupIndustry), self.getPlayerButton(industryGameBest, aiGroupIndustry), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 3, self.getValueStr(industryGameBest), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 7, self.getPlayerStr(agricultureGameBest, aiGroupAgriculture), self.getPlayerButton(agricultureGameBest, aiGroupAgriculture), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 6, self.getValueStr(agricultureGameBest), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 10, self.getPlayerStr(militaryGameBest, aiGroupMilitary), self.getPlayerButton(militaryGameBest, aiGroupMilitary), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 9, self.getValueStr(militaryGameBest), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 12
			SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 13, self.getPlayerStr(landAreaGameBest, aiGroupLandArea), self.getPlayerButton(landAreaGameBest, aiGroupLandArea), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 11
			SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 12, self.getValueStr(landAreaGameBest), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 15
			SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 16, self.getPlayerStr(populationGameBest, aiGroupPopulation), self.getPlayerButton(populationGameBest, aiGroupPopulation), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 14
			SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 15, self.getValueStr(populationGameBest), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 17
			SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 19, self.getPlayerStr(happinessGameBest, aiGroupHappiness), self.getPlayerButton(happinessGameBest, aiGroupHappiness), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 16
			SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 18, self.getValueStr(happinessGameBest, self.TEXT_HAPPINESS_MEASURE), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 19
			SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 22, self.getPlayerStr(healthGameBest, aiGroupHealth), self.getPlayerButton(healthGameBest, aiGroupHealth), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 18
			SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 21, self.getValueStr(healthGameBest), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			if self.bShowExports:
				# row was 22
				SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 25, self.getPlayerStr(netTradeGameBest, aiGroupNetTrade), self.getPlayerButton(netTradeGameBest, aiGroupNetTrade), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
				SASTextScale.setTableTextLabel(screen, szTable, iBestCol, 24, self.getValueStr(netTradeGameBest), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)

		#iCol = 3
		if bShowAvg:
			# Decimal separators added
			SASTextScale.setTableTextLabel(screen, szTable, iAvgCol, 0, separateThousands(iEconomyGameAverage, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			SASTextScale.setTableTextLabel(screen, szTable, iAvgCol, 3, separateThousands(iIndustryGameAverage, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			SASTextScale.setTableTextLabel(screen, szTable, iAvgCol, 6, separateThousands(iAgricultureGameAverage, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			SASTextScale.setTableTextLabel(screen, szTable, iAvgCol, 9, separateThousands(iMilitaryGameAverage, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 11
			SASTextScale.setTableTextLabel(screen, szTable, iAvgCol, 12, separateThousands(iLandAreaGameAverage, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 14
			SASTextScale.setTableTextLabel(screen, szTable, iAvgCol, 15, separateThousands(iPopulationGameAverage, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 16
			SASTextScale.setTableTextLabel(screen, szTable, iAvgCol, 18, separateThousands(iHappinessGameAverage, self.TEXT_THOUSANDS_SEPARATOR_COMMA) + self.TEXT_HAPPINESS_MEASURE, "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 18
			SASTextScale.setTableTextLabel(screen, szTable, iAvgCol, 21, separateThousands(iHealthGameAverage, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			if self.bShowExports:
				# row was 21
				SASTextScale.setTableTextLabel(screen, szTable, iAvgCol, 24, separateThousands(iNetTradeGameAverage, self.TEXT_THOUSANDS_SEPARATOR_COMMA), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)

		#iCol = 4
		if bShowWorst:
			# Replaced str(i...GameWorst) with getPlayerStr and getValueStr, and put them in a separate rows.
			SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 1, self.getPlayerStr(economyGameWorst, aiGroupEconomy), self.getPlayerButton(economyGameWorst, aiGroupEconomy), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 0, self.getValueStr(economyGameWorst), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 4, self.getPlayerStr(industryGameWorst, aiGroupIndustry), self.getPlayerButton(industryGameWorst, aiGroupIndustry), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 3, self.getValueStr(industryGameWorst), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 7, self.getPlayerStr(agricultureGameWorst, aiGroupAgriculture), self.getPlayerButton(agricultureGameWorst, aiGroupAgriculture), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 6, self.getValueStr(agricultureGameWorst), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 10, self.getPlayerStr(militaryGameWorst, aiGroupMilitary), self.getPlayerButton(militaryGameWorst, aiGroupMilitary), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 9, self.getValueStr(militaryGameWorst), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 12
			SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 13, self.getPlayerStr(landAreaGameWorst, aiGroupLandArea), self.getPlayerButton(landAreaGameWorst, aiGroupLandArea), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 11
			SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 12, self.getValueStr(landAreaGameWorst), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 15
			SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 16, self.getPlayerStr(populationGameWorst, aiGroupPopulation), self.getPlayerButton(populationGameWorst, aiGroupPopulation), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 14
			SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 15, self.getValueStr(populationGameWorst), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 17
			SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 19, self.getPlayerStr(happinessGameWorst, aiGroupHappiness), self.getPlayerButton(happinessGameWorst, aiGroupHappiness), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 16
			SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 18, self.getValueStr(happinessGameWorst, self.TEXT_HAPPINESS_MEASURE), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 19
			SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 22, self.getPlayerStr(healthGameWorst, aiGroupHealth), self.getPlayerButton(healthGameWorst, aiGroupHealth), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			# row was 18
			SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 21, self.getValueStr(healthGameWorst), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
			if self.bShowExports:
				# row was 22
				SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 25, self.getPlayerStr(netTradeGameWorst, aiGroupNetTrade), self.getPlayerButton(netTradeGameWorst, aiGroupNetTrade), chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
				# row was 21
				SASTextScale.setTableTextLabel(screen, szTable, iWorstCol, 24, self.getValueStr(netTradeGameWorst), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)

		#iCol = 5
		# Put it in iValueCol if that option is set
		iCol = iRankCol
		iOffset = 0
		if self.bRankInValueColumn:
			iCol = iValueCol
			iOffset = 1
		# <!-- custom: use getRankStr to show medal icons for ranks 1-3, added with claude opus 4.5's help thanks. -->
		SASTextScale.setTableTextLabel(screen, szTable, iCol, 0 + iOffset, self.getRankStr(iEconomyRank), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		SASTextScale.setTableTextLabel(screen, szTable, iCol, 3 + iOffset, self.getRankStr(iIndustryRank), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		SASTextScale.setTableTextLabel(screen, szTable, iCol, 6 + iOffset, self.getRankStr(iAgricultureRank), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		SASTextScale.setTableTextLabel(screen, szTable, iCol, 9 + iOffset, self.getRankStr(iMilitaryRank), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		# row was 11
		SASTextScale.setTableTextLabel(screen, szTable, iCol, 12 + iOffset, self.getRankStr(iLandAreaRank), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		# row was 14
		SASTextScale.setTableTextLabel(screen, szTable, iCol, 15 + iOffset, self.getRankStr(iPopulationRank), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		# row was 16
		SASTextScale.setTableTextLabel(screen, szTable, iCol, 18 + iOffset, self.getRankStr(iHappinessRank), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		# row was 18
		SASTextScale.setTableTextLabel(screen, szTable, iCol, 21 + iOffset, self.getRankStr(iHealthRank), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		if self.bShowExports:
			# row was 21
			SASTextScale.setTableTextLabel(screen, szTable, iCol, 24 + iOffset, self.getRankStr(iNetTradeRank), "", chartRowWidget, chartRowId1, chartRowId2, chartRowFont)
		# </advc.077>
		return

	# TOP CITIES

	def drawTopCitiesTab(self):

		screen = self.getScreen()

		# Background Panes
		self.szLeftPaneWidget = self.getNextWidgetName()
		screen.addPanel( self.szLeftPaneWidget, "", "", true, true, self.X_TOP_CITIES_LEFT_PANE, self.Y_TOP_CITIES_LEFT_PANE, self.W_TOP_CITIES_LEFT_PANE, self.H_TOP_CITIES_LEFT_PANE, PanelStyles.PANEL_STYLE_MAIN )#PanelStyles.PANEL_STYLE_DAWNTOP )

		# <!-- custom: add "Top 5 Cities in the World" header (Civ3 vibe!), removed bold, added with claude opus 4.5's help thanks. -->
		szHeaderText = sasFontTagTitle + localText.getText("TXT_KEY_TOP_5_CITIES_IN_THE_WORLD", ()) + SAS_FONT_TAG_CLOSE
		screen.setLabel(self.getNextWidgetName(), "", szHeaderText, CvUtil.FONT_CENTER_JUSTIFY, self.X_TOP_CITIES_LEFT_PANE + self.W_TOP_CITIES_LEFT_PANE / 2, self.Y_TOP_CITIES_LEFT_PANE + 12, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		self.drawTopCities()
		self.drawWondersTab()

	def drawTopCities(self):

		self.calculateTopCities()
		self.determineCityData()

		screen = self.getScreen()

		self.szCityNameWidgets = []
		self.szCityDescWidgets = []
		self.szCityAnimWidgets = []

		for iWidgetLoop in range(self.iNumCities):

			szTextPanel = self.getNextWidgetName()
			screen.addPanel( szTextPanel, "", "", false, true, self.X_COL_1_CITIES_DESC, self.Y_ROWS_CITIES[iWidgetLoop] + self.Y_CITIES_DESC_BUFFER, self.W_CITIES_DESC, self.H_CITIES_DESC, PanelStyles.PANEL_STYLE_DAWNTOP )
			self.szCityNameWidgets.append(self.getNextWidgetName())
#			szProjectDesc = u"<font=3b>" + pProjectInfo.getDescription().upper() + u"</font>"
			szCityDesc = sasFontTagTitle.bold + str(self.iCitySizes[iWidgetLoop]) + SAS_FONT_TAG_CLOSE + " - " + sasFontTagLabel.bold + self.szCityNames[iWidgetLoop] + SAS_FONT_TAG_CLOSE + "\n"
			szCityDesc += sasFontTagLabel + self.szCityDescs[iWidgetLoop] + SAS_FONT_TAG_CLOSE
			screen.addMultilineText(self.szCityNameWidgets[iWidgetLoop], szCityDesc, self.X_COL_1_CITIES_DESC + 6, self.Y_ROWS_CITIES[iWidgetLoop] + self.Y_CITIES_DESC_BUFFER + 3, self.W_CITIES_DESC - 6, self.H_CITIES_DESC - 6, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
#			screen.attachMultilineText( szTextPanel, self.szCityNameWidgets[iWidgetLoop], str(self.iCitySizes[iWidgetLoop]) + " - " + self.szCityNames[iWidgetLoop] + "\n" + self.szCityDescs[iWidgetLoop], WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			iCityX = self.aaCitiesXY[iWidgetLoop][0]
			iCityY = self.aaCitiesXY[iWidgetLoop][1]
			pPlot = CyMap().plot(iCityX, iCityY)
			pCity = pPlot.getPlotCity()

			iDistance = 200 + (pCity.getPopulation() * 5)
			if (iDistance > 350):
				iDistance = 350

			self.szCityAnimWidgets.append(self.getNextWidgetName())
			# advc.001d: Replaced gc.getGame().getActiveTeam() with self.iActiveTeam. But don't check bRevealAll - addPlotGraphicGFC requires the city to be revealed, will show an empty tile otherwise.
			# advc.007: bDebug=True unless perspective switched. 
			if pCity.isRevealed(self.iActiveTeam, self.iActiveTeam == gc.getGame().getActiveTeam()):
				screen.addPlotGraphicGFC(self.szCityAnimWidgets[iWidgetLoop], self.X_CITY_ANIMATION, self.Y_ROWS_CITIES[iWidgetLoop] + self.Y_CITY_ANIMATION_BUFFER - self.H_CITY_ANIMATION / 2, self.W_CITY_ANIMATION, self.H_CITY_ANIMATION, pPlot, iDistance, false, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# Draw Wonder icons
		self.drawCityWonderIcons()

		return

	def drawCityWonderIcons(self):

		screen = self.getScreen()

		aaiTopCitiesWonders = []
		aiTopCitiesNumWonders = []
		for i in range(self.iNumCities):
			aaiTopCitiesWonders.append(0)
			aiTopCitiesNumWonders.append(0)

		# Loop through top cities and determine if they have any wonders to display
		for iCityLoop in range(self.iNumCities):

			if (self.pCityPointers[iCityLoop]):

				pCity = self.pCityPointers[iCityLoop]

				aiTempWondersList = []

				# Loop through buildings

				for iBuildingLoop in range(gc.getNumBuildingInfos()):

					pBuilding = gc.getBuildingInfo(iBuildingLoop)

					# If this building is a wonder...
					# advc:
					if isWorldWonderClass(pBuilding.getBuildingClassType()):

						if (pCity.getNumBuilding(iBuildingLoop) > 0):

							aiTempWondersList.append(iBuildingLoop)
							aiTopCitiesNumWonders[iCityLoop] += 1

				aaiTopCitiesWonders[iCityLoop] = aiTempWondersList

		# <!-- custom: use multilist for wonder icons to allow multiple rows, added with claude opus 4.5's help thanks. -->
		# Create Scrollable areas under each city
		self.szCityWonderScrollArea = []
		for iCityLoop in range(self.iNumCities):

			self.szCityWonderScrollArea.append(self.getNextWidgetName())

			szIconPanel = self.szCityWonderScrollArea[iCityLoop]

			iMultiListX = self.X_COL_1_CITIES_DESC
			iMultiListY = self.Y_ROWS_CITIES[iCityLoop] + self.Y_CITIES_WONDER_BUFFER + self.Y_CITIES_DESC_BUFFER
			iMultiListW = self.W_CITIES_DESC
			iMultiListH = self.H_CITIES_WONDER_PANEL

			# Create panel first
			screen.addPanel(szIconPanel, "", "", False, True, iMultiListX, iMultiListY, iMultiListW, iMultiListH, PanelStyles.PANEL_STYLE_DAWNTOP)

			# <!-- custom: use no-header offsets since wonder panel has no title text, added with claude opus 4.5's help thanks. -->
			# Create multilist control for multiple rows of wonder buttons
			szMultiListName = self.getNextWidgetName()
			screen.addMultiListControlGFC(szMultiListName, "", iMultiListX + self.MULTI_LIST_PANEL_OFFSET_X_NO_HEADER, iMultiListY + self.MULTI_LIST_PANEL_OFFSET_Y_NO_HEADER, iMultiListW - (2 * self.MULTI_LIST_PANEL_OFFSET_X_NO_HEADER), iMultiListH - (2 * self.MULTI_LIST_PANEL_OFFSET_Y_NO_HEADER), self.iTopCitiesWonderNumListsAutoCalculate, self.iTopCitiesWonderButtonSize, self.iTopCitiesWonderButtonSize, TableStyles.TABLE_STYLE_STANDARD)

			# Now place the wonder buttons
			for iWonderLoop in range(aiTopCitiesNumWonders[iCityLoop]):
				iBuildingID = aaiTopCitiesWonders[iCityLoop][iWonderLoop]
				screen.appendMultiListButton(szMultiListName, gc.getBuildingInfo(iBuildingID).getButton(), self.iTopCitiesWonderColumnIndexAuto, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuildingID, -1, False)

	def calculateTopCities(self):

		# Calculate the top 5 cities

		for iPlayerLoop in range(gc.getMAX_PLAYERS()):

			apCityList = PyPlayer(iPlayerLoop).getCityList()

			for pCity in apCityList:

				iTotalCityValue = ((pCity.getCulture() / 5) + (pCity.getFoodRate() + pCity.getProductionRate() \
					+ pCity.calculateGoldRate())) * pCity.getPopulation()

				for iRankLoop in range(5):

					if (iTotalCityValue > self.iCityValues[iRankLoop] and not pCity.isBarbarian()):

						self.addCityToList(iRankLoop, pCity, iTotalCityValue)

						break

	# Recursive
	def addCityToList(self, iRank, pCity, iTotalCityValue):

		if (iRank > 4):

			return

		else:
			pTempCity = self.pCityPointers[iRank]

			# Verify a city actually exists at this rank
			if (pTempCity):

				iTempCityValue = self.iCityValues[iRank]

				self.addCityToList(iRank+1, pTempCity, iTempCityValue)

				self.pCityPointers[iRank] = pCity
				self.iCityValues[iRank] = iTotalCityValue

			else:
				self.pCityPointers[iRank] = pCity
				self.iCityValues[iRank] = iTotalCityValue

				return

	def determineCityData(self):

		self.iNumCities = 0

		for iRankLoop in range(5):

			pCity = self.pCityPointers[iRankLoop]

			# If this city exists and has data we can use
			if (pCity):

				pPlayer = gc.getPlayer(pCity.getOwner())

				iTurnYear = CyGame().getTurnYear(pCity.getGameTurnFounded())

				if (iTurnYear < 0):
					szTurnFounded = localText.getText("TXT_KEY_TIME_BC", (-iTurnYear,))#"%d %s" %(-iTurnYear, self.TEXT_BC)
				else:
					szTurnFounded = localText.getText("TXT_KEY_TIME_AD", (iTurnYear,))#"%d %s" %(iTurnYear, self.TEXT_AD)
				# advc.007: bDebug=True unless perspective switched
				bRevealed = pCity.GetCy().isRevealed(self.iActiveTeam, self.iActiveTeam == gc.getGame().getActiveTeam())
				# advc.001d: hasMet clause commented out, replaced gc.getGame().getActiveTeam() with self.iActiveTeam. Check bRevealAll.
				if bRevealed or self.bRevealAll:# or gc.getTeam(pPlayer.getTeam()).isHasMet(gc.getGame().getActiveTeam()):
					self.szCityNames[iRankLoop] = pCity.getName().upper()
					self.szCityDescs[iRankLoop] = ("%s, %s" %(pPlayer.getCivilizationAdjective(0), localText.getText("TXT_KEY_MISC_FOUNDED_IN", (szTurnFounded,))))
				else:
					self.szCityNames[iRankLoop] = localText.getText("TXT_KEY_UNKNOWN", ()).upper()
					self.szCityDescs[iRankLoop] = ("%s" %(localText.getText("TXT_KEY_MISC_FOUNDED_IN", (szTurnFounded,)), ))
				self.iCitySizes[iRankLoop] = pCity.getPopulation()
				self.aaCitiesXY[iRankLoop] = [pCity.getX(), pCity.getY()]

				self.iNumCities += 1
			else:

				self.szCityNames[iRankLoop] = ""
				self.iCitySizes[iRankLoop] = -1
				self.szCityDescs[iRankLoop] = ""
				self.aaCitiesXY[iRankLoop] = [-1, -1]

		return

	# WONDERS

	def drawWondersTab(self):
		screen = self.getScreen()

		self.szRightPaneWidget = self.getNextWidgetName()
		screen.addPanel( self.szRightPaneWidget, "", "", true, true, self.X_WONDERS_RIGHT_PANE, self.Y_WONDERS_RIGHT_PANE, self.W_WONDERS_RIGHT_PANE, self.H_WONDERS_RIGHT_PANE, PanelStyles.PANEL_STYLE_MAIN )#PanelStyles.PANEL_STYLE_DAWNTOP )

		self.drawWondersDropdownBox()

		if AdvisorOpt.isShowInfoWonders():
			self.calculateWondersList_BUG()
			self.drawWondersList_BUG()
		else:
			self.calculateWondersList()
			self.drawWondersList()

	def drawWondersDropdownBox(self):
		"Draws the Wonders Dropdown Box"

		screen = self.getScreen()

		self.szWondersDropdownWidget = self.getNextWidgetName()

		if AdvisorOpt.isShowInfoWonders():
			if self.szWonderDisplayMode == self.szWDM_WorldWonder:
				sWW = self.BUGWorldWonder_On
				sNW = self.BUGNatWonder_Off
				sPj = self.BUGProject_Off
				sDesc = self.TEXT_WORLD_WONDERS
			elif self.szWonderDisplayMode == self.szWDM_NatnlWonder:
				sWW = self.BUGWorldWonder_Off
				sNW = self.BUGNatWonder_On
				sPj = self.BUGProject_Off
				sDesc = self.TEXT_NATIONAL_WONDERS
			else:
				sWW = self.BUGWorldWonder_Off
				sNW = self.BUGNatWonder_Off
				sPj = self.BUGProject_On
				sDesc = self.TEXT_PROJECTS

			sDesc = sasFontTagTitle + sDesc + SAS_FONT_TAG_CLOSE

			screen.setImageButton(self.BUGWorldWonderWidget, sWW,  self.X_WONDERS_DROPDOWN +  0, self.Y_WONDERS_DROPDOWN, 24, 24, WidgetTypes.WIDGET_INFO_WORLD_WONDERS, -1, -1)
			screen.setImageButton(self.BUGNatWonderWidget, sNW,  self.X_WONDERS_DROPDOWN + 30, self.Y_WONDERS_DROPDOWN, 24, 24, WidgetTypes.WIDGET_INFO_NATIONAL_WONDERS, -1, -1)
			screen.setImageButton(self.BUGProjectWidget, sPj,  self.X_WONDERS_DROPDOWN + 60, self.Y_WONDERS_DROPDOWN, 24, 24, WidgetTypes.WIDGET_INFO_PROJECTS, -1, -1)

			screen.setLabel(self.getNextWidgetName(), "Background", sDesc, CvUtil.FONT_LEFT_JUSTIFY, self.X_WONDERS_DROPDOWN + 100, self.Y_WONDERS_DROPDOWN + 3, self.Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		else:
			screen.addDropDownBoxGFC(self.szWondersDropdownWidget, self.X_WONDERS_DROPDOWN, self.Y_WONDERS_DROPDOWN, self.W_WONDERS_DROPDOWN, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)

			if (self.szWonderDisplayMode == self.szWDM_WorldWonder):
				bDefault = true
			else:
				bDefault = false
			screen.addPullDownString(self.szWondersDropdownWidget, self.TEXT_WORLD_WONDERS, 0, 0, bDefault )

			if (self.szWonderDisplayMode == self.szWDM_NatnlWonder):
				bDefault = true
			else:
				bDefault = false
			screen.addPullDownString(self.szWondersDropdownWidget, self.TEXT_NATIONAL_WONDERS, 1, 1, bDefault )

			if (self.szWonderDisplayMode == self.szWDM_Project):
				bDefault = true
			else:
				bDefault = false
			screen.addPullDownString(self.szWondersDropdownWidget, self.TEXT_PROJECTS, 2, 2, bDefault )

		return

	def determineListBoxContents(self):

		screen = self.getScreen()

		# Fill wonders listbox

		iNumWondersBeingBuilt = len(self.aaWondersBeingBuilt)

		szWonderName = ""
		self.aiWonderListBoxIDs = []
		self.aiTurnYearBuilt = []
		self.aiWonderBuiltBy = []
		self.aszWonderCity = []

		if (self.szWonderDisplayMode == self.szWDM_Project):

	############### Create ListBox for Projects ###############

			for iWonderLoop in range(iNumWondersBeingBuilt):

				iProjectType = self.aaWondersBeingBuilt[iWonderLoop][0]
				pProjectInfo = gc.getProjectInfo(iProjectType)
				szProjectName = pProjectInfo.getDescription()

				self.aiWonderListBoxIDs.append(iProjectType)
				self.aiTurnYearBuilt.append(-6666)
				szWonderBuiltBy = self.aaWondersBeingBuilt[iWonderLoop][1]
				self.aiWonderBuiltBy.append(szWonderBuiltBy)
				szWonderCity = ""
				self.aszWonderCity.append(szWonderCity)

				SASTextScale.appendListBoxStringLabel(screen,  self.szWondersListBox, szProjectName + " (" + szWonderBuiltBy + ")", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

			for iWonderLoop in range(self.iNumWonders):

				iProjectType = self.aaWondersBuilt[iWonderLoop][1]
				pProjectInfo = gc.getProjectInfo(iProjectType)
				szProjectName = pProjectInfo.getDescription()

				self.aiWonderListBoxIDs.append(iProjectType)
				self.aiTurnYearBuilt.append(-9999)
				szWonderBuiltBy = self.aaWondersBuilt[iWonderLoop][2]
				self.aiWonderBuiltBy.append(szWonderBuiltBy)
				szWonderCity = self.aaWondersBuilt[iWonderLoop][3]
				self.aszWonderCity.append(szWonderCity)

				SASTextScale.appendListBoxStringLabel(screen,  self.szWondersListBox, szProjectName + " (" + szWonderBuiltBy + ")", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		else:

	############### Create ListBox for Wonders ###############

			for iWonderLoop in range(iNumWondersBeingBuilt):

				iWonderType = self.aaWondersBeingBuilt[iWonderLoop][0]
				pWonderInfo = gc.getBuildingInfo(iWonderType)
				szWonderName = pWonderInfo.getDescription()

				self.aiWonderListBoxIDs.append(iWonderType)
				self.aiTurnYearBuilt.append(-9999)
				szWonderBuiltBy = self.aaWondersBeingBuilt[iWonderLoop][1]
				self.aiWonderBuiltBy.append(szWonderBuiltBy)
				szWonderCity = ""
				self.aszWonderCity.append(szWonderCity)

				SASTextScale.appendListBoxStringLabel(screen,  self.szWondersListBox, szWonderName + " (" + szWonderBuiltBy + ")", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

			for iWonderLoop in range(self.iNumWonders):

				iWonderType = self.aaWondersBuilt[iWonderLoop][1]
				pWonderInfo = gc.getBuildingInfo(iWonderType)
				szWonderName = pWonderInfo.getDescription()

				self.aiWonderListBoxIDs.append(iWonderType)
				self.aiTurnYearBuilt.append(self.aaWondersBuilt[iWonderLoop][0])
				szWonderBuiltBy = self.aaWondersBuilt[iWonderLoop][2]
				self.aiWonderBuiltBy.append(szWonderBuiltBy)
				szWonderCity = self.aaWondersBuilt[iWonderLoop][3]
				self.aszWonderCity.append(szWonderCity)

				SASTextScale.appendListBoxStringLabel(screen,  self.szWondersListBox, szWonderName + " (" + szWonderBuiltBy + ")", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

	def drawWondersList(self):

		screen = self.getScreen()

		if (self.iNumWondersPermanentWidgets == 0):

			# Wonders List ListBox
			self.szWondersListBox = self.getNextWidgetName()
			screen.addListBoxGFC(self.szWondersListBox, "", self.X_WONDER_LIST, self.Y_WONDER_LIST, self.W_WONDER_LIST, self.H_WONDER_LIST, TableStyles.TABLE_STYLE_STANDARD )
			screen.setStyle(self.szWondersListBox, "Table_StandardCiv_Style")

			self.determineListBoxContents()

			self.iNumWondersPermanentWidgets = self.nWidgetCount

		self.szWondersTable = self.getNextWidgetName()

		# Stats Panel
		panelName = self.getNextWidgetName()
		screen.addPanel( panelName, "", "", true, true, self.X_WONDERS_STATS_PANE, self.Y_WONDERS_STATS_PANE, self.W_WONDERS_STATS_PANE, self.H_WONDERS_STATS_PANE, PanelStyles.PANEL_STYLE_IN )

		# DISPLAY SINGLE WONDER

		# Set default wonder if any exist in this list
		if (len(self.aiWonderListBoxIDs) > 0 and self.iWonderID == -1):
			self.iWonderID = self.aiWonderListBoxIDs[0]

		# Only display/do the following if a wonder is actively being displayed
		if (self.iWonderID > -1):

		# DISPLAY PROJECT MODE

			if (self.szWonderDisplayMode == self.szWDM_Project):

				pProjectInfo = gc.getProjectInfo(self.iWonderID)

				# Stats panel (cont'd) - Name
				szProjectDesc = sasFontTagLabel.bold + pProjectInfo.getDescription().upper() + SAS_FONT_TAG_CLOSE
				szStatsText = szProjectDesc + "\n\n"

				# Say whether this project is built yet or not

				iTurnYear = self.aiTurnYearBuilt[self.iActiveWonderCounter]
				if (iTurnYear == -6666):	# -6666 used for wonders in progress
					szTempText = localText.getText("TXT_KEY_BEING_BUILT", ())

				else:
					szTempText = localText.getText("TXT_KEY_INFO_SCREEN_BUILT", ())

				szWonderDesc = "%s, %s" %(self.aiWonderBuiltBy[self.iActiveWonderCounter], szTempText)
				szStatsText += szWonderDesc + "\n"

				if (self.aszWonderCity[self.iActiveWonderCounter] != ""):
					szStatsText += self.aszWonderCity[self.iActiveWonderCounter] + "\n\n"
				else:
					szStatsText += "\n"

				if (pProjectInfo.getProductionCost() > 0):
					# advc.001d: Replaced gc.getActivePlayer with self.pActivePlayer
					szCost = localText.getText("TXT_KEY_PEDIA_COST", (self.pActivePlayer.getProjectProductionNeeded(self.iWonderID),))
					szStatsText += szCost.upper() + (u"%c" % gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar()) + "\n"

				if (isWorldProject(self.iWonderID)):
					iMaxInstances = gc.getProjectInfo(self.iWonderID).getMaxGlobalInstances()
					szProjectType = localText.getText("TXT_KEY_PEDIA_WORLD_PROJECT", ())
					if (iMaxInstances > 1):
						szProjectType += " " + localText.getText("TXT_KEY_PEDIA_WONDER_INSTANCES", (iMaxInstances,))
					szStatsText += szProjectType.upper() + "\n"

				if (isTeamProject(self.iWonderID)):
					iMaxInstances = gc.getProjectInfo(self.iWonderID).getMaxTeamInstances()
					szProjectType = localText.getText("TXT_KEY_PEDIA_TEAM_PROJECT", ())
					if (iMaxInstances > 1):
						szProjectType += " " + localText.getText("TXT_KEY_PEDIA_WONDER_INSTANCES", (iMaxInstances,))
					szStatsText += szProjectType.upper()

				screen.addMultilineText(self.getNextWidgetName(), szStatsText, self.X_WONDERS_STATS_PANE + 5, self.Y_WONDERS_STATS_PANE + 15, self.W_WONDERS_STATS_PANE - 10, self.H_WONDERS_STATS_PANE - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

				# Add Graphic
				iIconX = self.X_PROJECT_ICON - self.W_PROJECT_ICON / 2
				iIconY = self.Y_PROJECT_ICON - self.W_PROJECT_ICON / 2

				screen.addDDSGFC(self.getNextWidgetName(), gc.getProjectInfo(self.iWonderID).getButton(), iIconX, iIconY, self.W_PROJECT_ICON, self.W_PROJECT_ICON, WidgetTypes.WIDGET_GENERAL, -1, -1 )

				# Special Abilities ListBox
				self.szSpecialTitleWidget = self.getNextWidgetName()
				screen.setText(self.szSpecialTitleWidget, "", self.szWondersSpecialTitle, CvUtil.FONT_LEFT_JUSTIFY, self.X_WONDER_SPECIAL_TITLE, self.Y_WONDER_SPECIAL_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

				panelName = self.getNextWidgetName()
				screen.addPanel( panelName, "", "", true, true, self.X_WONDER_SPECIAL_PANE, self.Y_WONDER_SPECIAL_PANE, self.W_WONDER_SPECIAL_PANE, self.H_WONDER_SPECIAL_PANE, PanelStyles.PANEL_STYLE_IN)

				listName = self.getNextWidgetName()
				screen.attachListBoxGFC( panelName, listName, "", TableStyles.TABLE_STYLE_EMPTY )
				screen.enableSelect(listName, False)

				szSpecialText = CyGameTextMgr().getProjectHelp(self.iWonderID, True, None)
				# <!-- custom: use native code instead similarly, no need to import string module to split a string similarly -->
				splitText = szSpecialText.split("\n")
				for special in splitText:
					if len( special ) != 0:
						SASTextScale.appendListBoxStringLabel(screen,  listName, special, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

			else:

	# DISPLAY WONDER MODE

				pWonderInfo = gc.getBuildingInfo(self.iWonderID)

				# Stats panel (cont'd) - Name
				szWonderDesc = sasFontTagLabel.bold + gc.getBuildingInfo(self.iWonderID).getDescription().upper() + SAS_FONT_TAG_CLOSE
				szStatsText = szWonderDesc + "\n\n"

				# Wonder built-in year
				iTurnYear = self.aiTurnYearBuilt[self.iActiveWonderCounter]#self.aaWondersBuilt[self.iActiveWonderCounter][0]#.append([0,iProjectLoop,""]

				szDateBuilt = ""

				if (iTurnYear != -9999):	# -9999 used for wonders in progress
					if (iTurnYear < 0):
						szTurnFounded = localText.getText("TXT_KEY_TIME_BC", (-iTurnYear,))
					else:
						szTurnFounded = localText.getText("TXT_KEY_TIME_AD", (iTurnYear,))

					szDateBuilt = (", %s" %(szTurnFounded))

				else:
					szDateBuilt = (", %s" %(localText.getText("TXT_KEY_BEING_BUILT", ())))

				szWonderDesc = "%s%s" %(self.aiWonderBuiltBy[self.iActiveWonderCounter], szDateBuilt)
				szStatsText += szWonderDesc + "\n"

				if (self.aszWonderCity[self.iActiveWonderCounter] != ""):
					szStatsText += self.aszWonderCity[self.iActiveWonderCounter] + "\n\n"
				else:
					szStatsText += "\n"

				# Building attributes

				if (pWonderInfo.getProductionCost() > 0):
					# advc.001d: Replaced gc.getActivePlayer with self.pActivePlayer
					szCost = localText.getText("TXT_KEY_PEDIA_COST", (self.pActivePlayer.getBuildingProductionNeeded(self.iWonderID),))
					szStatsText += szCost.upper() + (u"%c" % gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar()) + "\n"

				for k in range(CommerceTypes.NUM_COMMERCE_TYPES):
					if (pWonderInfo.getObsoleteSafeCommerceChange(k) != 0):
						if (pWonderInfo.getObsoleteSafeCommerceChange(k) > 0):
							szSign = "+"
						else:
							szSign = ""

						szCommerce = gc.getCommerceInfo(k).getDescription() + ": "

						szText1 = szCommerce.upper() + szSign + str(pWonderInfo.getObsoleteSafeCommerceChange(k))
						szText2 = szText1 + (u"%c" % (gc.getCommerceInfo(k).getChar()))
						szStatsText += szText2 + "\n"

				if (pWonderInfo.getHappiness() > 0):
					szText = localText.getText("TXT_KEY_PEDIA_HAPPY", (pWonderInfo.getHappiness(),))
					szStatsText += szText + (u"%c" % CyGame().getSymbolID(FontSymbols.HAPPY_CHAR)) + "\n"

				elif (pWonderInfo.getHappiness() < 0):
					szText = localText.getText("TXT_KEY_PEDIA_UNHAPPY", (-pWonderInfo.getHappiness(),))
					szStatsText += szText + (u"%c" % CyGame().getSymbolID(FontSymbols.UNHAPPY_CHAR)) + "\n"

				if (pWonderInfo.getHealth() > 0):
					szText = localText.getText("TXT_KEY_PEDIA_HEALTHY", (pWonderInfo.getHealth(),))
					szStatsText += szText + (u"%c" % CyGame().getSymbolID(FontSymbols.HEALTHY_CHAR)) + "\n"

				elif (pWonderInfo.getHealth() < 0):
					szText = localText.getText("TXT_KEY_PEDIA_UNHEALTHY", (-pWonderInfo.getHealth(),))
					szStatsText += szText + (u"%c" % CyGame().getSymbolID(FontSymbols.UNHEALTHY_CHAR)) + "\n"

				if (pWonderInfo.getGreatPeopleRateChange() != 0):
					szText = localText.getText("TXT_KEY_PEDIA_GREAT_PEOPLE", (pWonderInfo.getGreatPeopleRateChange(),))
					szStatsText += szText + (u"%c" % CyGame().getSymbolID(FontSymbols.GREAT_PEOPLE_CHAR)) + "\n"

				screen.addMultilineText(self.getNextWidgetName(), szStatsText, self.X_WONDERS_STATS_PANE + 5, self.Y_WONDERS_STATS_PANE + 15, self.W_WONDERS_STATS_PANE - 10, self.H_WONDERS_STATS_PANE - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

				# Add Graphic
				screen.addBuildingGraphicGFC(self.getNextWidgetName(), self.iWonderID, self.X_WONDER_GRAPHIC, self.Y_WONDER_GRAPHIC, self.W_WONDER_GRAPHIC, self.H_WONDER_GRAPHIC, WidgetTypes.WIDGET_GENERAL, -1, -1, self.X_ROTATION_WONDER_ANIMATION, self.Z_ROTATION_WONDER_ANIMATION, self.WONDERS_SCALE_ANIMATION, True)

				# Special Abilities ListBox

				# <!-- custom: note: there was a localText.getText("TXT_KEY_PEDIA_SPECIAL_ABILITIES", ()) commented-out in the base advciv code at the addPanel part, removed for concision/clarity. -->
				self.szSpecialTitleWidget = self.getNextWidgetName()
				screen.setText(self.szSpecialTitleWidget, "", self.szWondersSpecialTitle, CvUtil.FONT_LEFT_JUSTIFY, self.X_WONDER_SPECIAL_TITLE, self.Y_WONDER_SPECIAL_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

				panelName = self.getNextWidgetName()
				screen.addPanel( panelName, "", "", true, true, self.X_WONDER_SPECIAL_PANE, self.Y_WONDER_SPECIAL_PANE, self.W_WONDER_SPECIAL_PANE, self.H_WONDER_SPECIAL_PANE, PanelStyles.PANEL_STYLE_IN)

				listName = self.getNextWidgetName()
				screen.attachListBoxGFC( panelName, listName, "", TableStyles.TABLE_STYLE_EMPTY )
				screen.enableSelect(listName, False)

				szSpecialText = CyGameTextMgr().getBuildingHelp(self.iWonderID, True, False, False, None)
				splitText = szSpecialText.split("\n")
				for special in splitText:
					if len( special ) != 0:
						SASTextScale.appendListBoxStringLabel(screen,  listName, special, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

	def calculateWondersList(self):

		self.aaWondersBeingBuilt = []
		self.aaWondersBuilt = []
		self.iNumWonders = 0
		# advc.001d: Commented out. Already set properly by caller.
		#self.pActivePlayer = gc.getPlayer(CyGame().getActivePlayer())

		# Loop through players to determine Wonders
		for iPlayerLoop in range(gc.getMAX_PLAYERS()):

			pPlayer = gc.getPlayer(iPlayerLoop)
			iPlayerTeam = pPlayer.getTeam()

			# No Barbarians and only display national wonders for the active player's team
			if (pPlayer and not pPlayer.isBarbarian() and ((self.szWonderDisplayMode != self.szWDM_NatnlWonder) or (iPlayerTeam == gc.getTeam(gc.getPlayer(self.iActivePlayer).getTeam()).getID()))):

				# Loop through this player's cities and determine if they have any wonders to display
				apCityList = PyPlayer(iPlayerLoop).getCityList()
				for pCity in apCityList:

					pCityPlot = CyMap().plot(pCity.getX(), pCity.getY())

					# Check to see if active player can see this city
					szCityName = ""
					if (pCityPlot.isActiveVisible(false)):
						szCityName = pCity.getName()

					# Loop through projects to find any under construction
					if (self.szWonderDisplayMode == self.szWDM_Project):
						for iProjectLoop in range(gc.getNumProjectInfos()):

							iProjectProd = pCity.getProductionProject()
							pProject = gc.getProjectInfo(iProjectLoop)

							# Project is being constructed
							if (iProjectProd == iProjectLoop):

								# Project Mode
								if (iPlayerTeam == gc.getTeam(gc.getPlayer(self.iActivePlayer).getTeam()).getID()):

									self.aaWondersBeingBuilt.append([iProjectProd, pPlayer.getCivilizationShortDescription(0)])

					# Loop through buildings
					else:

						for iBuildingLoop in range(gc.getNumBuildingInfos()):

							iBuildingProd = pCity.getProductionBuilding()

							pBuilding = gc.getBuildingInfo(iBuildingLoop)

							# World Wonder Mode
							# advc:
							if self.szWonderDisplayMode == self.szWDM_WorldWonder and isWorldWonderClass(pBuilding.getBuildingClassType()):

								# Is this city building a wonder?
								if (iBuildingProd == iBuildingLoop):

									# Only show our wonders under construction
									if (iPlayerTeam == gc.getPlayer(self.iActivePlayer).getTeam()):

										self.aaWondersBeingBuilt.append([iBuildingProd, pPlayer.getCivilizationShortDescription(0)])

								if (pCity.getNumBuilding(iBuildingLoop) > 0):
									if (iPlayerTeam == gc.getPlayer(self.iActivePlayer).getTeam() or gc.getTeam(gc.getPlayer(self.iActivePlayer).getTeam()).isHasMet(iPlayerTeam)):								
										self.aaWondersBuilt.append([pCity.getBuildingOriginalTime(iBuildingLoop),iBuildingLoop,pPlayer.getCivilizationShortDescription(0),szCityName])
									else:
										self.aaWondersBuilt.append([pCity.getBuildingOriginalTime(iBuildingLoop),iBuildingLoop, localText.getText("TXT_KEY_UNKNOWN", ()), localText.getText("TXT_KEY_UNKNOWN", ())])
	#								print("Adding World wonder to list: %s, %d, %s" %(pCity.getBuildingOriginalTime(iBuildingLoop),iBuildingLoop,pPlayer.getCivilizationAdjective(0)))
									self.iNumWonders += 1

							# National/Team Wonder Mode
							# advc:
							elif self.szWonderDisplayMode == self.szWDM_NatnlWonder and (isNationalWonderClass(pBuilding.getBuildingClassType()) or isTeamWonderClass(pBuilding.getBuildingClassType())):

								# Is this city building a wonder?
								if (iBuildingProd == iBuildingLoop):

									# Only show our wonders under construction
									if (iPlayerTeam == gc.getPlayer(self.iActivePlayer).getTeam()):

										self.aaWondersBeingBuilt.append([iBuildingProd, pPlayer.getCivilizationShortDescription(0)])

								if (pCity.getNumBuilding(iBuildingLoop) > 0):

	#								print("Adding National wonder to list: %s, %d, %s" %(pCity.getBuildingOriginalTime(iBuildingLoop),iBuildingLoop,pPlayer.getCivilizationAdjective(0)))
									if (iPlayerTeam == gc.getPlayer(self.iActivePlayer).getTeam() or gc.getTeam(gc.getPlayer(self.iActivePlayer).getTeam()).isHasMet(iPlayerTeam)):								
										self.aaWondersBuilt.append([pCity.getBuildingOriginalTime(iBuildingLoop),iBuildingLoop,pPlayer.getCivilizationShortDescription(0), szCityName])
									else:
										self.aaWondersBuilt.append([pCity.getBuildingOriginalTime(iBuildingLoop),iBuildingLoop, localText.getText("TXT_KEY_UNKNOWN", ()), localText.getText("TXT_KEY_UNKNOWN", ())])
									self.iNumWonders += 1

		# This array used to store which players have already used up a team's slot so team projects don't get added to list more than once
		aiTeamsUsed = []

		# Project Mode
		if (self.szWonderDisplayMode == self.szWDM_Project):

			# Loop through players to determine Projects
			for iPlayerLoop in range(gc.getMAX_PLAYERS()):

				pPlayer = gc.getPlayer(iPlayerLoop)
				iTeamLoop = pPlayer.getTeam()

				# Block duplicates
				if (iTeamLoop not in aiTeamsUsed):

					aiTeamsUsed.append(iTeamLoop)
					pTeam = gc.getTeam(iTeamLoop)

					if (pTeam.isAlive() and not pTeam.isBarbarian()):

						# Loop through projects
						for iProjectLoop in range(gc.getNumProjectInfos()):

							for iI in range(pTeam.getProjectCount(iProjectLoop)):

								if (iTeamLoop == gc.getPlayer(self.iActivePlayer).getTeam() or gc.getTeam(gc.getPlayer(self.iActivePlayer).getTeam()).isHasMet(iTeamLoop)):								
									self.aaWondersBuilt.append([-9999,iProjectLoop,gc.getPlayer(iPlayerLoop).getCivilizationShortDescription(0),szCityName])
								else:
									self.aaWondersBuilt.append([-9999,iProjectLoop, localText.getText("TXT_KEY_UNKNOWN", ()), localText.getText("TXT_KEY_UNKNOWN", ())])
								self.iNumWonders += 1

		# Sort wonders in order of date built
		self.aaWondersBuilt.sort()
		self.aaWondersBuilt.reverse()

#		print("List of wonders/projects Built:")
#		print(self.aaWondersBuilt)

	def calculateWondersList_BUG(self):

		self.aaWondersBeingBuilt_BUG = []
		self.aaWondersBuilt_BUG = []
		self.iNumWonders = 0
		# advc.007: Use this to reveal all wonders in Debug mode unless perspective switched
		# advc.001d: Check bRevealALl
		bDebug = (CyGame().isDebugMode() or self.bRevealAll) and self.iActiveTeam == gc.getGame().getActiveTeam()

		# Loop through players to determine Wonders
		for iPlayerLoop in range(gc.getMAX_PLAYERS()):

			pPlayer = gc.getPlayer(iPlayerLoop)
			iPlayerTeam = pPlayer.getTeam()

			# No barbs and only display national wonders for the active player's team
			# advc.001d: The above comment was apparently copied from the
			# BtS calculateWondersList function, but national wonders aren't
			# actually skipped. Actually, no reason to exclude barbs.
			# Skip national wonders later so that they're still shown if a
			# city was investigated (a K-Mod change). I.e. remove this check
			# entirely:
			#if (pPlayer and not pPlayer.isBarbarian()):
			if 1:

				# Loop through this player's cities and determine if they have any wonders to display
				apCityList = PyPlayer(iPlayerLoop).getCityList()
				for pCity in apCityList:
					pCityPlot = CyMap().plot(pCity.getX(), pCity.getY())

					# advc.007: Note that pCity is a PyCity (PyHelpers.py), whose isRevealed function takes only one argument. Need a CyCity instead.
					# advc.001d: Check bRevealAll
					bRevealed = pCity.GetCy().isRevealed(self.iActiveTeam, bDebug) or self.bRevealAll

					# Loop through projects to find any under construction
					if self.szWonderDisplayMode == self.szWDM_Project:
						for iProjectLoop in range(gc.getNumProjectInfos()):

							iProjectProd = pCity.getProductionProject()

							# Project is being constructed
							if iProjectProd == iProjectLoop:
								if iPlayerTeam == self.iActiveTeam or bDebug:
									self.aaWondersBeingBuilt_BUG.append([iProjectLoop,pPlayer.getCivilizationShortDescription(0), pCity, iPlayerLoop])
								# advc.001d: elif - Don't list it twice in Debug mode
								elif (self.pActiveTeam.isHasMet(iPlayerTeam)
								and self.iInvestigateCityMission != -1 # K-Mod, bugfix
								and self.pActivePlayer.canDoEspionageMission(self.iInvestigateCityMission, pCity.getOwner(), pCity.plot(), -1) # advc.001d: Replaced gc.getGame().getActiveTeam with self.iActiveTeam
								and bRevealed) or bDebug:
									self.aaWondersBeingBuilt_BUG.append([iProjectLoop,pPlayer.getCivilizationShortDescription(0), pCity, iPlayerLoop])

					# Loop through buildings
					else:
						for iBuildingLoop in range(gc.getNumBuildingInfos()):
							iBuildingProd = pCity.getProductionBuilding()
							# <advc.001d> While I'm a it - no need to list
							# Palaces
							pBuilding = gc.getBuildingInfo(iBuildingLoop)
							if pBuilding.isCapital():
								continue
							# World Wonder Mode
							# advc:
							if self.szWonderDisplayMode == self.szWDM_WorldWonder and isWorldWonderClass(pBuilding.getBuildingClassType()):
								# Is this city building a wonder?
								if (iBuildingProd == iBuildingLoop):
									if (iPlayerTeam == self.iActiveTeam):
										self.aaWondersBeingBuilt_BUG.append([iBuildingLoop,pPlayer.getCivilizationShortDescription(0), pCity, iPlayerLoop])
									# advc.001d: elif - Don't list it twice in Debug mode
									elif (self.pActiveTeam.isHasMet(iPlayerTeam)
									and self.iInvestigateCityMission != -1 # K-Mod, bugfix
									and self.pActivePlayer.canDoEspionageMission(self.iInvestigateCityMission, pCity.getOwner(), pCity.plot(), -1) # advc.001d: Replaced gc.getGame().getActiveTeam with self.iActiveTeam
									and bRevealed) or bDebug:
										self.aaWondersBeingBuilt_BUG.append([iBuildingLoop,pPlayer.getCivilizationShortDescription(0), pCity, iPlayerLoop])

								if (pCity.getNumBuilding(iBuildingLoop) > 0):
									# advc.001d: bRevealed added. If the city is revealed, then the active player knows the city owner from the color of the borders - and wonders are shown on the main map.
									if iPlayerTeam == self.iActiveTeam or bRevealed or self.pActiveTeam.isHasMet(iPlayerTeam) or bDebug:
										self.aaWondersBuilt_BUG.append([pCity.getBuildingOriginalTime(iBuildingLoop),iBuildingLoop,True,pPlayer.getCivilizationShortDescription(0),pCity, iPlayerLoop])
									else:
										self.aaWondersBuilt_BUG.append([pCity.getBuildingOriginalTime(iBuildingLoop),iBuildingLoop,False, localText.getText("TXT_KEY_UNKNOWN", ()),pCity,gc.getBARBARIAN_PLAYER()])
										# kekm: Replaced hardcoded 18 with barbarian player
	#								print("Adding World wonder to list: %s, %d, %s" %(pCity.getBuildingOriginalTime(iBuildingLoop),iBuildingLoop,pPlayer.getCivilizationAdjective(0)))
									self.iNumWonders += 1

							# National/Team Wonder Mode
							elif self.szWonderDisplayMode == self.szWDM_NatnlWonder and (isNationalWonderClass(pBuilding.getBuildingClassType()) or isTeamWonderClass(pBuilding.getBuildingClassType())):

								# Is this city building a wonder?
								if (iBuildingProd == iBuildingLoop):
									# Only show our wonders under construction
									if iPlayerTeam == self.iActiveTeam or bDebug:
										self.aaWondersBeingBuilt_BUG.append([iBuildingLoop,pPlayer.getCivilizationShortDescription(0), pCity, iPlayerLoop])
									# advc.001d: elif - Don't list it twice in Debug mode
									elif (self.pActiveTeam.isHasMet(iPlayerTeam)
									and self.iInvestigateCityMission != -1 # K-Mod, bugfix
									and self.pActivePlayer.canDoEspionageMission(self.iInvestigateCityMission, pCity.getOwner(), pCity.plot(), -1) # advc.001d: Replaced gc.getGame().getActiveTeam with self.iActiveTeam
									and bRevealed) or bDebug:
										self.aaWondersBeingBuilt_BUG.append([iBuildingLoop,pPlayer.getCivilizationShortDescription(0), pCity, iPlayerLoop])

								# Has this city built a wonder?
								if (pCity.getNumBuilding(iBuildingLoop) > 0):
									if iPlayerTeam == self.iActiveTeam or bDebug:
										self.aaWondersBuilt_BUG.append([pCity.getBuildingOriginalTime(iBuildingLoop),iBuildingLoop,True,pPlayer.getCivilizationShortDescription(0), pCity, iPlayerLoop])
										self.iNumWonders += 1
									# advc.001d (comment): To hide (finished) national wonders of other teams, remove this code block:
									elif self.pActiveTeam.isHasMet(iPlayerTeam) and bRevealed:
										self.aaWondersBuilt_BUG.append([pCity.getBuildingOriginalTime(iBuildingLoop),iBuildingLoop,True,pPlayer.getCivilizationShortDescription(0), pCity, iPlayerLoop])
										self.iNumWonders += 1

		# This array used to store which players have already used up a team's slot so team projects don't get added to list more than once
		aiTeamsUsed = []

		# Project Mode
		if (self.szWonderDisplayMode == self.szWDM_Project):

			# Loop through players to determine Projects
			for iPlayerLoop in range(gc.getMAX_PLAYERS()):

				pPlayer = gc.getPlayer(iPlayerLoop)
				iTeamLoop = pPlayer.getTeam()

				# Block duplicates
				if (iTeamLoop not in aiTeamsUsed):

					aiTeamsUsed.append(iTeamLoop)
					pTeam = gc.getTeam(iTeamLoop)

					if (pTeam.isAlive() and not pTeam.isBarbarian()):

						# Loop through projects
						for iProjectLoop in range(gc.getNumProjectInfos()):

							for iI in range(pTeam.getProjectCount(iProjectLoop)):

								if iTeamLoop == self.iActiveTeam or self.pActiveTeam.isHasMet(iTeamLoop) or bDebug:
									self.aaWondersBuilt_BUG.append([-9999,iProjectLoop,True,gc.getPlayer(iPlayerLoop).getCivilizationShortDescription(0),None, iPlayerLoop])
								else:
									# advc.001: Last param was 9999
									self.aaWondersBuilt_BUG.append([-9999,iProjectLoop,False, localText.getText("TXT_KEY_UNKNOWN", ()), None, gc.getBARBARIAN_PLAYER()])
								self.iNumWonders += 1

		# Sort wonders in order of date built
		self.aaWondersBuilt_BUG.sort()
		self.aaWondersBuilt_BUG.reverse()

#		print("List of wonders/projects Built:")
#		print(self.aaWondersBuilt)

	def drawWondersList_BUG(self):

		self.szWondersListBox = self.getNextWidgetName()
		self.szWondersTable = self.getNextWidgetName()

		screen = self.getScreen()
		screen.addTableControlGFC(self.szWondersTable, 5, self.X_WONDERS_CHART, self.Y_WONDERS_CHART, self.W_WONDERS_CHART, self.H_WONDERS_CHART, True, True, 24,24, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSort(self.szWondersTable)

		# <!-- custom: old zoomArt = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTONS_CITYSELECTION").getPath() declaration no longer needed after our change to the columns as per claude opus 4.5's review (see comments at self.WONDERS_COL_MOVE_TO_CITY_ID). -->

		SASTextScale.setTableColumnHeaderLabel(screen, self.szWondersTable, 0, "", self.W_WONDERS_CHART_COL_BUTTON)
		SASTextScale.setTableColumnHeaderLabel(screen, self.szWondersTable, 1, self.sNameWonders, self.W_WONDERS_CHART_COL_NAME)
		SASTextScale.setTableColumnHeaderLabel(screen, self.szWondersTable, 2, self.sDateWonders, self.W_WONDERS_CHART_COL_DATE)
		SASTextScale.setTableColumnHeaderLabel(screen, self.szWondersTable, 3, self.sOwnerWonders, self.W_WONDERS_CHART_COL_OWNER)
		SASTextScale.setTableColumnHeaderLabel(screen, self.szWondersTable, self.WONDERS_COL_MOVE_TO_CITY_ID, self.sCityWonders, self.W_WONDERS_CHART_COL_CITY)

		iWBB = len(self.aaWondersBeingBuilt_BUG)

		for iWonderLoop in range(iWBB):
#			self.aaWondersBeingBuilt_BUG contains the following:
			iWonderType = self.aaWondersBeingBuilt_BUG[iWonderLoop][0]
			szWonderBuiltBy = self.aaWondersBeingBuilt_BUG[iWonderLoop][1]
			pCity = self.aaWondersBeingBuilt_BUG[iWonderLoop][2]
			iPlayer = self.aaWondersBeingBuilt_BUG[iWonderLoop][3]

			color = -1
			ePlayerColor = gc.getPlayer(iPlayer).getPlayerColor()
			if ePlayerColor != -1:
				playerColor = gc.getPlayerColorInfo(ePlayerColor)
				if playerColor:
					#color = playerColor.getColorTypePrimary()
					color = playerColor.getTextColorType() # kekm.36

			if (self.szWonderDisplayMode == self.szWDM_Project):
				pWonderInfo = gc.getProjectInfo(iWonderType)
				iWidget = WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROJECT
			else:
				pWonderInfo = gc.getBuildingInfo(iWonderType)
				iWidget = WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING

			szWonderName = pWonderInfo.getDescription()
			szTurnYearBuilt = sasFontTagLabel + (u"%c" % gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar()) + SAS_FONT_TAG_CLOSE

			# Check to see if active player can see this city
			# advc.001d: replaced gc.getGame().getActiveTeam with self.iActiveTeam. Check bRevealAll.
			# advc.007: bDebug=True unless perspective switched. 
			bRevealed = pCity.GetCy().isRevealed(self.iActiveTeam, self.iActiveTeam == gc.getGame().getActiveTeam()) or self.bRevealAll
			if pCity and bRevealed:
				szCityName = pCity.getName()
			else:
				szCityName = u""

			if AdvisorOpt.isWonderListUsePlayerColor():
				szWonderBuiltBy = localText.changeTextColor(szWonderBuiltBy, color)
				szCityName = localText.changeTextColor(szCityName, color)

			screen.appendTableRow(self.szWondersTable)
			# SASTextScale.setTableTextLabel(screen, self.szWondersTable, 0, iWonderLoop, "", zoomArt, WidgetTypes.WIDGET_ZOOM_CITY, pCity.getOwner(), pCity.getID(), CvUtil.FONT_LEFT_JUSTIFY)
			SASTextScale.setTableTextLabel(screen, self.szWondersTable, 0, iWonderLoop, "", pWonderInfo.getButton(), iWidget, iWonderType, -1, CvUtil.FONT_LEFT_JUSTIFY)
			SASTextScale.setTableTextLabel(screen, self.szWondersTable, 1, iWonderLoop, szWonderName, "", iWidget, iWonderType, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# <!-- custom: Wonders keeps formatted date/status text (e.g. BC/AD or production icon) rather than raw signed years.
			# World Advisor already has compact signed numeric years for sortable year display. (GPT-5.5) -->
			SASTextScale.setTableTextLabel(screen, self.szWondersTable, 2, iWonderLoop, szTurnYearBuilt, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
			SASTextScale.setTableTextLabel(screen, self.szWondersTable, 3, iWonderLoop, szWonderBuiltBy, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# SASTextScale.setTableTextLabel(screen, self.szWondersTable, 4, iWonderLoop, szCityName, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			SASTextScale.setTableTextLabel(screen, self.szWondersTable, 4, iWonderLoop, szCityName, "", WidgetTypes.WIDGET_ZOOM_CITY, pCity.getOwner(), pCity.getID(), CvUtil.FONT_LEFT_JUSTIFY)

		for iWonderLoop in range(self.iNumWonders):
#			self.aaWondersBuilt_BUG contains the following:
			iTurnYearBuilt = self.aaWondersBuilt_BUG[iWonderLoop][0]
			iWonderType = self.aaWondersBuilt_BUG[iWonderLoop][1]
			bKnown = self.aaWondersBuilt_BUG[iWonderLoop][2]
			szWonderBuiltBy = self.aaWondersBuilt_BUG[iWonderLoop][3]
			pCity = self.aaWondersBuilt_BUG[iWonderLoop][4]
			iPlayer = self.aaWondersBuilt_BUG[iWonderLoop][5]

			color = -1
			ePlayerColor = gc.getPlayer(iPlayer).getPlayerColor()
			if ePlayerColor != -1:
				playerColor = gc.getPlayerColorInfo(ePlayerColor)
				if playerColor:
					#color = playerColor.getColorTypePrimary()
					color = playerColor.getTextColorType() # kekm.36

			if (self.szWonderDisplayMode == self.szWDM_Project):
				pWonderInfo = gc.getProjectInfo(iWonderType)
				iWidget = WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROJECT
			else:
				pWonderInfo = gc.getBuildingInfo(iWonderType)
				iWidget = WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING
			szWonderName = pWonderInfo.getDescription()

			if iTurnYearBuilt == -9999:
				szTurnYearBuilt = u""
			else:
				szTurnYearBuilt = BugUtil.getDisplayYear(iTurnYearBuilt)

			# Check to see if active player can see this city
			# advc.001d: Replaced gc.getGame().getActiveTeam with self.iActiveTeam. Check bRevealAll.
			# advc.007: bDebug=True unless perspective switched. 
			bRevealed = pCity and (pCity.GetCy().isRevealed(self.iActiveTeam, self.iActiveTeam == gc.getGame().getActiveTeam()) or self.bRevealAll)
			if bRevealed:
				szCityName = pCity.getName()
			else:
				szCityName = u""

			if AdvisorOpt.isWonderListUsePlayerColor():
				szWonderBuiltBy = localText.changeTextColor(szWonderBuiltBy, color)
				szCityName = localText.changeTextColor(szCityName, color)

			screen.appendTableRow(self.szWondersTable)
			# if bKnown and bRevealed:
			# 	SASTextScale.setTableTextLabel(screen, self.szWondersTable, 0, iWonderLoop+iWBB, "", zoomArt, WidgetTypes.WIDGET_ZOOM_CITY, pCity.getOwner(), pCity.getID(), CvUtil.FONT_LEFT_JUSTIFY)
			SASTextScale.setTableTextLabel(screen, self.szWondersTable, 0, iWonderLoop+iWBB, ""            , pWonderInfo.getButton(), iWidget, iWonderType, -1, CvUtil.FONT_LEFT_JUSTIFY)
			SASTextScale.setTableTextLabel(screen, self.szWondersTable, 1, iWonderLoop+iWBB, szWonderName, "", iWidget, iWonderType, -1, CvUtil.FONT_LEFT_JUSTIFY)
			SASTextScale.setTableTextLabel(screen, self.szWondersTable, 2, iWonderLoop+iWBB, szTurnYearBuilt, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
			SASTextScale.setTableTextLabel(screen, self.szWondersTable, 3, iWonderLoop+iWBB, szWonderBuiltBy, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# SASTextScale.setTableTextLabel(screen, self.szWondersTable, 4, iWonderLoop+iWBB, szCityName, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			if bKnown and bRevealed:
				SASTextScale.setTableTextLabel(screen, self.szWondersTable, 4, iWonderLoop+iWBB, szCityName, "", WidgetTypes.WIDGET_ZOOM_CITY, pCity.getOwner(), pCity.getID(), CvUtil.FONT_LEFT_JUSTIFY)
			else:
				SASTextScale.setTableTextLabel(screen, self.szWondersTable, 4, iWonderLoop+iWBB, szCityName, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	# STATISTICS

	def drawStatsTab(self):
		screen = self.getScreen()

		iNumUnits = gc.getNumUnitInfos()
		iNumBuildings = gc.getNumBuildingInfos()

		self.iNumUnitStatsChartCols = 5
		self.iNumBuildingStatsChartCols = 2

		self.iNumUnitStatsChartRows = iNumUnits
		self.iNumBuildingStatsChartRows = iNumBuildings

# CALCULATE STATS

		iMinutesPlayed = CyGame().getMinutesPlayed()
		iHoursPlayed = iMinutesPlayed / 60
		iMinutesPlayed = iMinutesPlayed - (iHoursPlayed * 60)

		szMinutesString = str(iMinutesPlayed)
		if (iMinutesPlayed < 10):
			szMinutesString = "0" + szMinutesString
		szHoursString = str(iHoursPlayed)
		if (iHoursPlayed < 10):
			szHoursString = "0" + szHoursString

		szTimeString = szHoursString + ":" + szMinutesString

		iNumCitiesBuilt = CyStatistics().getPlayerNumCitiesBuilt(self.iActivePlayer)

		iNumCitiesRazed = CyStatistics().getPlayerNumCitiesRazed(self.iActivePlayer)

		iNumReligionsFounded = 0
		for iReligionLoop in range(gc.getNumReligionInfos()):
			if (CyStatistics().getPlayerReligionFounded(self.iActivePlayer, iReligionLoop)):
				iNumReligionsFounded += 1

		aiUnitsBuilt = []
		for iUnitLoop in range(iNumUnits):
			aiUnitsBuilt.append(CyStatistics().getPlayerNumUnitsBuilt(self.iActivePlayer, iUnitLoop))

		aiUnitsKilled = []
		for iUnitLoop in range(iNumUnits):
			aiUnitsKilled.append(CyStatistics().getPlayerNumUnitsKilled(self.iActivePlayer, iUnitLoop))

		aiUnitsLost = []
		for iUnitLoop in range(iNumUnits):
			aiUnitsLost.append(CyStatistics().getPlayerNumUnitsLost(self.iActivePlayer, iUnitLoop))

		aiBuildingsBuilt = []
		for iBuildingLoop in range(iNumBuildings):
			aiBuildingsBuilt.append(CyStatistics().getPlayerNumBuildingsBuilt(self.iActivePlayer, iBuildingLoop))

		aiUnitsCurrent = []
		for iUnitLoop in range(iNumUnits):
			aiUnitsCurrent.append(0)

		apUnitList = PyPlayer(self.iActivePlayer).getUnitList()
		for pUnit in apUnitList:
			iType = pUnit.getUnitType()
			aiUnitsCurrent[iType] += 1

		# <!-- custom: Improvements column removed from the Stats tab. Rationale: this tab tracks save-persistent history counters (Units Built/Killed/Lost, Buildings Built — all from CyStatistics). The Improvements column was a snapshot ("Current", live plot iteration) — inconsistent with the rest of the table and not from CyStatistics (BTS has no getPlayerNumImprovementsBuilt). Concrete example of the inconsistency: build a Farm on a plot, replace it with a Cottage, then replace it back with a Farm — Units/Buildings tables would show Built=2 for an analogous build/destroy/rebuild, but the Improvements column showed Current=1 (one Farm on the map right now), giving "total Farms 1" instead of "total Farms built 2". Current-state info is now covered by World Advisor's Territory tab — a better home: more exhaustive and thematically grouped with other current-geography views. (Claude code Opus 4.7) -->

		# TOP PANEL

		# Add Panel
		szTopPanelWidget = self.getNextWidgetName()
		screen.addPanel( szTopPanelWidget, u"", u"", True, False, self.X_STATS_TOP_PANEL, self.Y_STATS_TOP_PANEL, self.W_STATS_TOP_PANEL, self.H_STATS_TOP_PANEL, PanelStyles.PANEL_STYLE_DAWNTOP )

		# Leaderhead graphic
		#player = gc.getPlayer(gc.getGame().getActivePlayer())
		player = gc.getPlayer(self.iActivePlayer) # K-Mod
		szLeaderWidget = self.getNextWidgetName()
		screen.addLeaderheadGFC(szLeaderWidget, player.getLeaderType(), AttitudeTypes.ATTITUDE_PLEASED, self.X_LEADER_ICON, self.Y_LEADER_ICON, self.W_LEADER_ICON, self.H_LEADER_ICON, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# Leader Name
		self.szLeaderNameWidget = self.getNextWidgetName()
		szText = sasFontTagTitle.bold + gc.getPlayer(self.iActivePlayer).getName() + SAS_FONT_TAG_CLOSE
		screen.setText(self.szLeaderNameWidget, "", szText, CvUtil.FONT_LEFT_JUSTIFY, self.X_LEADER_NAME, self.Y_LEADER_NAME, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# Create Table
		szTopChart = self.getNextWidgetName()
		screen.addTableControlGFC(szTopChart, self.iNumTopChartCols, self.X_STATS_TOP_CHART, self.Y_STATS_TOP_CHART, self.W_STATS_TOP_CHART, self.H_STATS_TOP_CHART, False, True, self.W_STATS_BUTTON_SIZE, self.H_STATS_BUTTON_SIZE, TableStyles.TABLE_STYLE_STANDARD)

		# Add Columns
		SASTextScale.setTableColumnHeaderLabel(screen, szTopChart, 0, "", self.STATS_TOP_CHART_W_COL_0)
		SASTextScale.setTableColumnHeaderLabel(screen, szTopChart, 1, "", self.STATS_TOP_CHART_W_COL_1)

		# Add Rows
		# for i in range(self.iNumTopChartRows - 1):
			# screen.appendTableRow(szTopChart)
		# iNumRows = screen.getTableNumRows(szTopChart)

		# Graph itself
		iRow = 0

		# <!-- custom: not sure it helps, but since we reuse the same variables every time, may as well cache them once if i'm not mistaken.
		# Note: using longer and more detailed names to avoid weird python inheritance issues with variables in unrelated scopes if i'm not mistaken. -->
		statsRowWidget = WidgetTypes.WIDGET_GENERAL
		statsRowId1 = -1
		statsRowId2 = -1
		statsRowFont = CvUtil.FONT_LEFT_JUSTIFY

		# <!-- custom: added buttons in the top chart with the help of claude opus 4.5 thanks, old code replaced. -->
		# Time Played - using gear as a "game mechanics" metaphor, or add a clock emoji later
		screen.appendTableRow(szTopChart)
		iCol = 0
		SASTextScale.setTableTextLabel(screen, szTopChart, iCol, iRow, self.TEXT_TIME_PLAYED, self.szTimeIconStats, statsRowWidget, statsRowId1, statsRowId2, statsRowFont)
		iCol = 1
		SASTextScale.setTableTextLabel(screen, szTopChart, iCol, iRow, szTimeString, "", statsRowWidget, statsRowId1, statsRowId2, statsRowFont)

		# K-Mod.
		iNumCitiesCurrent = gc.getPlayer(self.iActivePlayer).getNumCities()
		# advc.004: No longer optional
		# <!-- custom: always show, we have the room; unindent previously wrapped by an if blocks as well; added with claude opus 4.5's help thanks. -->
		# if iNumCitiesCurrent != 0:# or not AdvisorOpt.isNonZeroStatsOnly():
		iRow += 1
		screen.appendTableRow(szTopChart)
		iCol = 0
		SASTextScale.setTableTextLabel(screen, szTopChart, iCol, iRow, self.TEXT_CITIES_CURRENT, self.szCityIconStats, statsRowWidget, statsRowId1, statsRowId2, statsRowFont)
		iCol = 1
		SASTextScale.setTableTextLabel(screen, szTopChart, iCol, iRow, str(iNumCitiesCurrent), "", statsRowWidget, statsRowId1, statsRowId2, statsRowFont)
		# K-Mod end
		# advc.004:
		# if iNumCitiesBuilt != 0:# or not AdvisorOpt.isNonZeroStatsOnly():
		iRow += 1
		screen.appendTableRow(szTopChart)
		iCol = 0
		SASTextScale.setTableTextLabel(screen, szTopChart, iCol, iRow, self.TEXT_CITIES_BUILT, self.szFoundCityIconStats, statsRowWidget, statsRowId1, statsRowId2, statsRowFont)
		iCol = 1
		SASTextScale.setTableTextLabel(screen, szTopChart, iCol, iRow, str(iNumCitiesBuilt), "", statsRowWidget, statsRowId1, statsRowId2, statsRowFont)
		# advc.004:
		# if iNumCitiesRazed != 0:# or not AdvisorOpt.isNonZeroStatsOnly():
		iRow += 1
		screen.appendTableRow(szTopChart)
		iCol = 0
		SASTextScale.setTableTextLabel(screen, szTopChart, iCol, iRow, self.TEXT_CITIES_RAZED, self.szRazeIconStats, statsRowWidget, statsRowId1, statsRowId2, statsRowFont)
		iCol = 1
		SASTextScale.setTableTextLabel(screen, szTopChart, iCol, iRow, str(iNumCitiesRazed), "", statsRowWidget, statsRowId1, statsRowId2, statsRowFont)
		# advc.004:
		# if iNumReligionsFounded != 0:# or not AdvisorOpt.isNonZeroStatsOnly():
		iRow += 1
		screen.appendTableRow(szTopChart)
		iCol = 0
		SASTextScale.setTableTextLabel(screen, szTopChart, iCol, iRow, self.TEXT_NUM_RELIGIONS_FOUNDED, self.szReligionIconStats, statsRowWidget, statsRowId1, statsRowId2, statsRowFont)
		iCol = 1
		SASTextScale.setTableTextLabel(screen, szTopChart, iCol, iRow, str(iNumReligionsFounded), "", statsRowWidget, statsRowId1, statsRowId2, statsRowFont)

		# K-Mod.
		iNumGoldenAges = CyStatistics().getPlayerNumGoldenAges(self.iActivePlayer)
		# advc.004:
		# if iNumGoldenAges != 0:# or not AdvisorOpt.isNonZeroStatsOnly():
		iRow += 1
		screen.appendTableRow(szTopChart)
		iCol = 0
		SASTextScale.setTableTextLabel(screen, szTopChart, iCol, iRow, self.TEXT_NUM_GOLDEN_AGES, self.szGoldenAgeIconStats, statsRowWidget, statsRowId1, statsRowId2, statsRowFont)
		iCol = 1
		SASTextScale.setTableTextLabel(screen, szTopChart, iCol, iRow, str(iNumGoldenAges), "", statsRowWidget, statsRowId1, statsRowId2, statsRowFont)
		# K-Mod end

		# BOTTOM PANEL

		# Create Tables
		szUnitsTable = self.getNextWidgetName()
		screen.addTableControlGFC(szUnitsTable, self.iNumUnitStatsChartCols, self.X_STATS_BOTTOM_CHART, self.Y_STATS_BOTTOM_CHART, self.W_STATS_BOTTOM_CHART_UNITS, self.H_STATS_BOTTOM_CHART, True, True, self.W_STATS_BUTTON_SIZE, self.H_STATS_BUTTON_SIZE, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSort(szUnitsTable)

		szBuildingsTable = self.getNextWidgetName()
		screen.addTableControlGFC(szBuildingsTable, self.iNumBuildingStatsChartCols, self.X_STATS_BOTTOM_CHART + self.W_STATS_BOTTOM_CHART_UNITS, self.Y_STATS_BOTTOM_CHART, self.W_STATS_BOTTOM_CHART_BUILDINGS, self.H_STATS_BOTTOM_CHART, True, True, self.W_STATS_BUTTON_SIZE, self.H_STATS_BUTTON_SIZE, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSort(szBuildingsTable)

		# Reducing the width a bit to leave room for the vertical scrollbar, preventing a horizontal scrollbar from also being created
		iChartWidth = self.W_STATS_BOTTOM_CHART_UNITS + self.W_STATS_BOTTOM_CHART_BUILDINGS - 24

		# Add Columns
		iColWidth = int((iChartWidth / 12 * 3))
		SASTextScale.setTableColumnHeaderLabel(screen, szUnitsTable, 0, self.TEXT_UNITS, iColWidth)
		iColWidth = int((iChartWidth / 12 * 1))
		SASTextScale.setTableColumnHeaderLabel(screen, szUnitsTable, 1, self.TEXT_CURRENT, iColWidth)
		iColWidth = int((iChartWidth / 12 * 1))
		SASTextScale.setTableColumnHeaderLabel(screen, szUnitsTable, 2, self.TEXT_BUILT, iColWidth)
		iColWidth = int((iChartWidth / 12 * 1))
		SASTextScale.setTableColumnHeaderLabel(screen, szUnitsTable, 3, self.TEXT_KILLED, iColWidth)
		iColWidth = int((iChartWidth / 12 * 1))
		SASTextScale.setTableColumnHeaderLabel(screen, szUnitsTable, 4, self.TEXT_LOST, iColWidth)
		# <!-- custom: Buildings Name shrunk from 4 to 3 grid units (matching Units Name) so the Built column gets its full intended width and its header isn't clipped by the table's scrollbar margin. (Claude code Opus 4.7) -->
		iColWidth = int((iChartWidth / 12 * 3))
		SASTextScale.setTableColumnHeaderLabel(screen, szBuildingsTable, 0, self.TEXT_BUILDINGS, iColWidth)
		iColWidth = int((iChartWidth / 12 * 1))
		SASTextScale.setTableColumnHeaderLabel(screen, szBuildingsTable, 1, self.TEXT_BUILT, iColWidth)

# K-Mod: I've disabled this pre-appending of rows code - because it messes up my other stuff.
		# # Add Rows
		# for i in range(self.iNumUnitStatsChartRows):
			# screen.appendTableRow(szUnitsTable)
		# iNumUnitRows = screen.getTableNumRows(szUnitsTable)

		# for i in range(self.iNumBuildingStatsChartRows):
			# screen.appendTableRow(szBuildingsTable)
		# iNumBuildingRows = screen.getTableNumRows(szBuildingsTable)

# #BUG: improvements - start
		# if AdvisorOpt.isShowImprovements():
		# if True: # advc.004
			# for i in range(self.iNumImprovementStatsChartRows):
				# if (aiImprovementsCurrent[i] > 0):
					# screen.appendTableRow(szImprovementsTable)
			# iNumImprovementRows = screen.getTableNumRows(szImprovementsTable)
# #BUG: improvements - end

		# Add Units to table
		iRow = 0 # K-Mod
		# <!-- custom: default stats-tab unit order to "most built first" so the first view is actionable; name remains the tie-breaker and users can still re-sort by clicking headers. (GPT-5.3-Codex) -->
		unitIDsByBuiltDesc = []
		for iUnitLoop in range(iNumUnits):
			# K-Mod. Hide-rows option.
			if ((True or AdvisorOpt.isNonZeroStatsOnly()) # advc: no longer optional
					and aiUnitsCurrent[iUnitLoop] == 0 and aiUnitsBuilt[iUnitLoop] == 0
					and aiUnitsKilled[iUnitLoop] == 0 and aiUnitsLost[iUnitLoop] == 0):
				continue # K-Mod end
			unitIDsByBuiltDesc.append((-aiUnitsBuilt[iUnitLoop], gc.getUnitInfo(iUnitLoop).getDescription(), iUnitLoop))
		unitIDsByBuiltDesc.sort()
		for i in range(len(unitIDsByBuiltDesc)):
			(iNegBuilt, szUnitName, iUnitLoop) = unitIDsByBuiltDesc[i]
			screen.appendTableRow(szUnitsTable)
			#iRow = iUnitLoop
			iCol = 0

			# <!-- custom: add buttons in the stats tab's rows with claude opus 4.5's help thanks. -->
			#SASTextScale.setTableTextLabel(screen, szUnitsTable, iCol, iRow, szUnitName, "", statsRowWidget, statsRowId1, statsRowId2, statsRowFont)
			szUnitButton = gc.getUnitInfo(iUnitLoop).getButton()
			SASTextScale.setTableTextLabel(screen, szUnitsTable, iCol, iRow, szUnitName, szUnitButton, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnitLoop, -1, statsRowFont)

			iCol = 1
			iNumUnitsCurrent = aiUnitsCurrent[iUnitLoop]
			SASTextScale.setTableIntLabel(screen, szUnitsTable, iCol, iRow, str(iNumUnitsCurrent), "", statsRowWidget, statsRowId1, statsRowId2, statsRowFont)

			iCol = 2
			iNumUnitsBuilt = aiUnitsBuilt[iUnitLoop]
			SASTextScale.setTableIntLabel(screen, szUnitsTable, iCol, iRow, str(iNumUnitsBuilt), "", statsRowWidget, statsRowId1, statsRowId2, statsRowFont)

			iCol = 3
			iNumUnitsKilled = aiUnitsKilled[iUnitLoop]
			SASTextScale.setTableIntLabel(screen, szUnitsTable, iCol, iRow, str(iNumUnitsKilled), "", statsRowWidget, statsRowId1, statsRowId2, statsRowFont)

			iCol = 4
			iNumUnitsLost = aiUnitsLost[iUnitLoop]
			SASTextScale.setTableIntLabel(screen, szUnitsTable, iCol, iRow, str(iNumUnitsLost), "", statsRowWidget, statsRowId1, statsRowId2, statsRowFont)
			iRow += 1 # K-Mod

		# Add Buildings to table
		iRow = 0 # K-Mod
		# <!-- custom: default stats-tab building order to "most built first" for faster scanning; name is only a stable tie-breaker. (GPT-5.3-Codex) -->
		buildingIDsByBuiltDesc = []
		for iBuildingLoop in range(iNumBuildings):
			# K-Mod. Hide-rows option.
			if aiBuildingsBuilt[iBuildingLoop] == 0: #and AdvisorOpt.isNonZeroStatsOnly(): # advc: No longer optional
				continue # K-Mod end
			buildingIDsByBuiltDesc.append((-aiBuildingsBuilt[iBuildingLoop], gc.getBuildingInfo(iBuildingLoop).getDescription(), iBuildingLoop))
		buildingIDsByBuiltDesc.sort()
		for i in range(len(buildingIDsByBuiltDesc)):
			(iNegBuilt, szBuildingName, iBuildingLoop) = buildingIDsByBuiltDesc[i]
			screen.appendTableRow(szBuildingsTable)
			#iRow = iBuildingLoop
			iCol = 0
			szBuildingName = gc.getBuildingInfo(iBuildingLoop).getDescription()

			# <!-- custom: add buttons in the stats tab's rows with claude opus 4.5's help thanks. -->
			# SASTextScale.setTableTextLabel(screen, szBuildingsTable, iCol, iRow, szBuildingName, "", statsRowWidget, statsRowId1, statsRowId2, statsRowFont)
			szBuildingButton = gc.getBuildingInfo(iBuildingLoop).getButton()
			SASTextScale.setTableTextLabel(screen, szBuildingsTable, iCol, iRow, szBuildingName, szBuildingButton, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuildingLoop, -1, statsRowFont)

			iCol = 1
			iNumBuildingsBuilt = aiBuildingsBuilt[iBuildingLoop]
			SASTextScale.setTableIntLabel(screen, szBuildingsTable, iCol, iRow, str(iNumBuildingsBuilt), "", statsRowWidget, statsRowId1, statsRowId2, statsRowFont)
			iRow += 1 # K-Mod

	# OTHER

	def drawLine (self, screen, canvas, x0, y0, x1, y1, color, bThreeLines):
		if bThreeLines:
			screen.addLineGFC(canvas, self.getNextLineName(), x0, y0 + 1, x1, y1 + 1, color)
			screen.addLineGFC(canvas, self.getNextLineName(), x0 + 1, y0, x1 + 1, y1, color)
		screen.addLineGFC(canvas, self.getNextLineName(), x0, y0, x1, y1, color)

	def getLog10(self, x):
		return math.log10(max(1,x))

	def getTurnDate(self,turn):

		year = CyGame().getTurnYear(turn)

		if (year < 0):
			return localText.getText("TXT_KEY_TIME_BC", (-year,))
		else:
			return localText.getText("TXT_KEY_TIME_AD", (year,))

	def lineName(self,i):
		return self.LINE_ID + str(i)

	def getNextLineName(self):
		name = self.lineName(self.nLineCount)
		self.nLineCount += 1
		return name

	# returns a unique ID for a widget in this screen
	def getNextWidgetName(self):
		szName = self.WIDGET_ID + str(self.nWidgetCount)
		self.nWidgetCount += 1
		return szName

	def deleteAllLines(self):
		screen = self.getScreen()
		i = 0
		while (i < self.nLineCount):
			screen.deleteWidget(self.lineName(i))
			i += 1
		self.nLineCount = 0

	def deleteAllWidgets(self, iNumPermanentWidgets = 0):
		self.deleteAllLines()
		screen = self.getScreen()
		if self.TIMELINE_TABLE_CACHE_KEY is None:
			screen.deleteWidget(self.TIMELINE_TABLE_ID)
		else:
			screen.hide(self.TIMELINE_TABLE_ID)
		i = self.nWidgetCount - 1
		while (i >= iNumPermanentWidgets):
			self.nWidgetCount = i
			screen.deleteWidget(self.getNextWidgetName())
			i -= 1

		self.nWidgetCount = iNumPermanentWidgets
		self.yMessage = 5

		screen.deleteWidget(self.BUGWorldWonderWidget)
		screen.deleteWidget(self.BUGNatWonderWidget)
		screen.deleteWidget(self.BUGProjectWidget)

	# handle the input for this screen...
	def handleInput (self, inputClass):
#		BugUtil.debugInput(inputClass)

		screen = self.getScreen()

		szShortWidgetName = inputClass.getFunctionName()
		szWidgetName = inputClass.getFunctionName() + str(inputClass.getID())
		code = inputClass.getNotifyCode()

		# Exit
		if ( szWidgetName == self.szExitButtonName and code == NotifyCode.NOTIFY_CLICKED \
				or inputClass.getData() == int(InputTypes.KB_RETURN) ):
			# Reset Wonders so nothing lingers next time the screen is opened
			self.resetWonders()
			screen.hideScreen()

		# Slide graph
		if (szWidgetName == self.graphLeftButtonID and code == NotifyCode.NOTIFY_CLICKED):
			self.slideGraph(- 2 * self.graphZoom / 5)
			self.drawGraphs()

		elif (szWidgetName == self.graphRightButtonID and code == NotifyCode.NOTIFY_CLICKED):
			self.slideGraph(2 * self.graphZoom / 5)
			self.drawGraphs()

		BugUtil.debug("A:" + szShortWidgetName)

		# Dropdown Box/ ListBox
		if (code == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED):

			# Debug dropdown
			if (inputClass.getFunctionName() == self.DEBUG_DROPDOWN_ID):
				self.iActivePlayer = getAdvisorDebugDropdownSelectedPlayer(screen, self.DEBUG_DROPDOWN_ID)
				self.reset() # advc.001d
				self.pActivePlayer = gc.getPlayer(self.iActivePlayer)
				self.iActiveTeam = self.pActivePlayer.getTeam()
				self.pActiveTeam = gc.getTeam(self.iActiveTeam)

				self.determineKnownPlayers()
				# Force recache of all scores
				self.scoreCache = []
				for t in self.RANGE_SCORES:
					self.scoreCache.append(None)
				self.redrawContents()

			iSelected = inputClass.getData()
#			print("iSelected : %d" %(iSelected))

			# WONDERS / TOP CITIES TAB

			if (self.iActiveTab == self.iTopCitiesID):

				# Wonder type dropdown box
				if (szWidgetName == self.szWondersDropdownWidget or szShortWidgetName == self.BUGWorldWonderWidget or szShortWidgetName == self.BUGNatWonderWidget or szShortWidgetName == self.BUGProjectWidget):

					self.handleInput_Wonders(inputClass)

				# Wonders ListBox
				elif (szWidgetName == self.szWondersListBox):
					if not AdvisorOpt.isShowInfoWonders():
						self.reset()
						self.iWonderID = self.aiWonderListBoxIDs[iSelected]
						self.iActiveWonderCounter = iSelected
						self.deleteAllWidgets(self.iNumWondersPermanentWidgets)
						self.drawWondersList()

				# BUG Wonders table
				elif (szWidgetName == self.szWondersTable):
					if (inputClass.getMouseX() == self.WONDERS_COL_MOVE_TO_CITY_ID):
						screen.hideScreen()
						pPlayer = gc.getPlayer(inputClass.getData1())
						pCity = pPlayer.getCity(inputClass.getData2())
						CyCamera().JustLookAtPlot(pCity.plot())

			# GRAPH TAB

			elif (self.iActiveTab == self.iGraphID):

				# Graph dropdown to select what values are being graphed
				if (szWidgetName == self.szGraphDropdownWidget):

					if (iSelected == 0):
						self.iGraphTabID = self.TOTAL_SCORE

					elif (iSelected == 1):
						self.iGraphTabID = self.ECONOMY_SCORE

					elif (iSelected == 2):
						self.iGraphTabID = self.INDUSTRY_SCORE

					elif (iSelected == 3):
						self.iGraphTabID = self.AGRICULTURE_SCORE

					elif (iSelected == 4):
						self.iGraphTabID = self.POWER_SCORE

					elif (iSelected == 5):
						self.iGraphTabID = self.CULTURE_SCORE

					elif (iSelected == 6):
						self.iGraphTabID = self.ESPIONAGE_SCORE

					self.drawGraphs()

				elif (szWidgetName == self.szTurnsDropdownWidget):

					self.zoomGraph(self.dropDownTurns[iSelected])
					self.drawGraphs()

				elif AdvisorOpt.isGraphs(): # K-Mod. (the xin1 widgets are only defined when isGraphs is true)
					if (szWidgetName == self.szGraphSmoothingDropdownWidget_1in1):
						self.iGraph_Smoothing_1in1 = iSelected
						self.drawGraphs()

					elif (szWidgetName == self.szGraphSmoothingDropdownWidget_7in1):
						self.iGraph_Smoothing_7in1 = iSelected
						self.drawGraphs()

					else:
						for i in range(3):
							if (szWidgetName == self.szGraphDropdownWidget_3in1[i]):
								self.iGraph_3in1[i] = iSelected
								self.drawGraphs()
								break

		# Something Clicked
		elif (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED):

			######## Screen 'Tabs' for Navigation ########

			if self.iActiveTab == self.iTimelineID and szWidgetName == self.szTimelineDbgLogPrettySummaryButton and code == NotifyCode.NOTIFY_CLICKED:
				self.dbgLogPrettySummary()
				return 0

			# (K-Mod version)
			if (inputClass.getButtonType() == WidgetTypes.WIDGET_GENERAL):
				iData1 = inputClass.getData1()
				if (iData1 != self.iActiveTab and iData1 >= 0 and iData1 < len(self.PAGE_NAME_LIST) and szWidgetName == "InfoTabButton"+str(iData1)):
					self.iActiveTab = iData1
					self.reset()
					self.redrawContents()

			# Wonder type dropdown box
			if self.iActiveTab == self.iTopCitiesID: # K-Mod
				if (szShortWidgetName == self.BUGWorldWonderWidget or szShortWidgetName == self.BUGNatWonderWidget or szShortWidgetName == self.BUGProjectWidget):
					self.handleInput_Wonders(inputClass)

#BUG: Change Graphs - start
			if AdvisorOpt.isGraphs() and self.iActiveTab == self.iGraphID:
				for i in range(7):
					if (szWidgetName == self.sGraphTextHeadingWidget[i] or (szWidgetName == self.sGraphBGWidget[i] and code == NotifyCode.NOTIFY_CLICKED)):
						if self.Graph_Status_Current == self.Graph_Status_1in1:
							self.Graph_Status_Current = self.Graph_Status_Prior
							self.Graph_Status_Prior = self.Graph_Status_1in1
						else:
							self.Graph_Status_Prior = self.Graph_Status_Current
							self.Graph_Status_Current = self.Graph_Status_1in1
						self.iGraphTabID = i
						self.drawGraphs()
						break

					elif szWidgetName == self.sGraphTextBannerWidget[i]:
						if self.iGraphTabID == i:
							self.Graph_Status_Current = self.Graph_Status_7in1
						self.iGraphTabID = i
						self.drawGraphs()
						break

				if szWidgetName == self.sGraph1in1:
					self.Graph_Status_Current = self.Graph_Status_1in1
					self.Graph_Status_Prior = self.Graph_Status_Current
					self.drawGraphs()
				elif szWidgetName == self.sGraph3in1:
					self.Graph_Status_Current = self.Graph_Status_3in1
					self.Graph_Status_Prior = self.Graph_Status_Current
					self.drawGraphs()
				elif szWidgetName == self.sGraph7in1:
					self.Graph_Status_Current = self.Graph_Status_7in1
					self.Graph_Status_Prior = self.Graph_Status_Current
					self.drawGraphs()

				for i in range(gc.getMAX_CIV_PLAYERS()):
					if szWidgetName == self.sPlayerTextWidget[i]:
						self.bPlayerInclude[i] = not self.bPlayerInclude[i]
						self.drawGraphs()
						break

				if szWidgetName == self.sShowAllWidget:
					for i in range(gc.getMAX_CIV_PLAYERS()):
						self.bPlayerInclude[i] = True
					self.drawGraphs()

				if szWidgetName == self.sShowNoneWidget:
					for i in range(gc.getMAX_CIV_PLAYERS()):
						self.bPlayerInclude[i] = False
					self.drawGraphs()
#BUG: Change Graphs - start

		return 0

	def handleInput_Wonders (self, inputClass):
		szShortWidgetName = inputClass.getFunctionName()
		szWidgetName = inputClass.getFunctionName() + str(inputClass.getID())
		code = inputClass.getNotifyCode()
		iSelected = inputClass.getData()

		# Reset wonders stuff so that when the type shown changes the old contents don't mess with things

		self.iNumWonders = 0
		self.iActiveWonderCounter = 0
		self.iWonderID = -1
		self.aaWondersBuilt = []
		self.aaWondersBuilt_BUG = []

		self.aaWondersBeingBuilt = []
		self.aaWondersBeingBuilt_BUG = []

		if szWidgetName == self.szWondersDropdownWidget:
			if (iSelected == 0):
				self.szWonderDisplayMode = self.szWDM_WorldWonder

			elif (iSelected == 1):
				self.szWonderDisplayMode = self.szWDM_NatnlWonder

			elif (iSelected == 2):
				self.szWonderDisplayMode = self.szWDM_Project
		else:
			if szShortWidgetName == self.BUGWorldWonderWidget:
				self.szWonderDisplayMode = self.szWDM_WorldWonder
			elif szShortWidgetName == self.BUGNatWonderWidget:
				self.szWonderDisplayMode = self.szWDM_NatnlWonder
			elif szShortWidgetName == self.BUGProjectWidget:
				self.szWonderDisplayMode = self.szWDM_Project

		self.reset()

		self.calculateWondersList()
		if not AdvisorOpt.isShowInfoWonders():
			self.determineListBoxContents()

		# Change selected wonder to the one at the top of the new list
		if (self.iNumWonders > 0 and not AdvisorOpt.isShowInfoWonders()):
			self.iWonderID = self.aiWonderListBoxIDs[0]

		self.redrawContents()

		return

	def update(self, fDelta):
		return

	def determineKnownPlayers(self, iEndGame=0):
		# Determine who this active player knows
		self.aiPlayersMet = []
		self.aiPlayersMetNAEspionage = []
		self.iNumPlayersMet = 0
		self.iNumPlayersMetNAEspionage = 0
		for iLoopPlayer in range(gc.getMAX_CIV_PLAYERS()):
			pLoopPlayer = gc.getPlayer(iLoopPlayer)
			iLoopPlayerTeam = pLoopPlayer.getTeam()
			if (gc.getTeam(iLoopPlayerTeam).isEverAlive()):
				# advc.077: Replaced two iEndGame checks (the second had been added by K-Mod) with self.bRevealAll
				if (self.pActiveTeam.isHasMet(iLoopPlayerTeam) or self.bRevealAll):
					if self.pActivePlayer.canSeeDemographics(iLoopPlayer) or self.bRevealAll:
						self.aiPlayersMet.append(iLoopPlayer)
						self.iNumPlayersMet += 1
					else:
						self.aiPlayersMetNAEspionage.append(iLoopPlayer)
						self.iNumPlayersMetNAEspionage += 1
