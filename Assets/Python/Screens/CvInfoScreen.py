## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005

# Thanks to "Ulf 'ulfn' Norell" from Apolyton for his additions relating to the graph section of this screen

# This file has been edited for K-Mod in various places. Some changes marked, some not. (deletions generally not marked)

from CvPythonExtensions import *
import CvScreenEnums
import CvUtil
import ScreenInput

import string
#import time
import math

from PyHelpers import PyPlayer

# BUG - 3.17 No Espionage - start
import GameUtil
# BUG - 3.17 No Espionage - end

#BUG: Change Graphs - start
import BugCore
import BugUtil
AdvisorOpt = BugCore.game.Advisors
ScoreOpt = BugCore.game.Scores
#BUG: Change Graphs - end
import copy # advc.091

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
		# </advc.077>

		self.screenId = screenId
		self.DEMO_SCREEN_NAME = "DemographicsScreen"
		self.TOP_CITIES_SCREEN_NAME = "TopCitiesScreen"

		self.INTERFACE_ART_INFO = "TECH_BG"

		self.WIDGET_ID = "DemoScreenWidget"
		self.LINE_ID   = "DemoLine"

		self.Z_BACKGROUND = -6.1
		self.Z_CONTROLS = self.Z_BACKGROUND - 0.2
		self.DZ = -0.2
		self.Z_HELP_AREA = self.Z_CONTROLS - 1

		self.X_SCREEN = 0
		self.Y_SCREEN = 0
		self.W_SCREEN = 1024
		self.H_SCREEN = 768
		self.X_TITLE = 512
		self.Y_TITLE = 8
		self.BORDER_WIDTH = 4
		self.W_HELP_AREA = 200

		# K-Mod
		self.iLanguageLoaded = -1

		#self.X_EXIT = 994
		self.X_EXIT = self.W_SCREEN - 30
		#self.Y_EXIT = 730
		self.Y_EXIT = self.H_SCREEN - 42

		self.PAGE_NAME_LIST = [
			"TXT_KEY_INFO_GRAPH",
			"TXT_KEY_DEMO_SCREEN_TITLE",
			"TXT_KEY_WONDERS_SCREEN_TOP_CITIES_TEXT",
			"TXT_KEY_INFO_SCREEN_STATISTICS_TITLE",
			]
		self.PAGE_DRAW_LIST = [
			self.drawGraphTab,
			self.drawDemographicsTab,
			self.drawTopCitiesTab,
			self.drawStatsTab,
			]

		self.PAGE_LINK_WIDTH = [] # game text is not available at the time this function is called, so we can't calculate the widths yet.

		self.Y_LINK = self.H_SCREEN - 42
		# K-Mod end

		self.graphEnd		= CyGame().getGameTurn() - 1
		self.graphZoom		= self.graphEnd - CyGame().getStartTurn()
		self.nWidgetCount	= 0
		self.nLineCount		= 0

		# This is used to allow the wonders screen to refresh without redrawing everything
		self.iNumWondersPermanentWidgets = 0

		self.iGraphID			= 0
		self.iDemographicsID	= 1
		self.iTopCitiesID		= 2
		self.iStatsID			= 3

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

		self.GRAPH_H_LINE = "GraphHLine"
		self.GRAPH_V_LINE = "GraphVLine"

		self.xSelPt = 0
		self.ySelPt = 0

		self.graphLeftButtonID = ""
		self.graphRightButtonID = ""

################################################## GRAPH ###################################################

		self.X_MARGIN	= 45
		self.Y_MARGIN	= 80
		self.H_DROPDOWN	= 35

		self.X_DEMO_DROPDOWN	= self.X_MARGIN
		self.Y_DEMO_DROPDOWN	= self.Y_MARGIN
		self.W_DEMO_DROPDOWN	= 150 #247

		self.X_ZOOM_DROPDOWN	= self.X_DEMO_DROPDOWN
		self.Y_ZOOM_DROPDOWN	= self.Y_DEMO_DROPDOWN + self.H_DROPDOWN

		self.W_ZOOM_DROPDOWN	= self.W_DEMO_DROPDOWN

		self.X_LEGEND		= self.X_DEMO_DROPDOWN
		self.Y_LEGEND		= self.Y_ZOOM_DROPDOWN + self.H_DROPDOWN + 3
		self.W_LEGEND		= self.W_DEMO_DROPDOWN
		#self.H_LEGEND		= 200	this is computed from the number of players

		self.X_GRAPH = self.X_DEMO_DROPDOWN + self.W_DEMO_DROPDOWN + 10
		self.Y_GRAPH = self.Y_MARGIN
		self.W_GRAPH = self.W_SCREEN - self.X_GRAPH - self.X_MARGIN
		self.H_GRAPH = 590

		self.W_LEFT_BUTTON  = 20
		self.H_LEFT_BUTTON  = 20
		self.X_LEFT_BUTTON  = self.X_GRAPH
		self.Y_LEFT_BUTTON  = self.Y_GRAPH + self.H_GRAPH

		self.W_RIGHT_BUTTON  = self.W_LEFT_BUTTON
		self.H_RIGHT_BUTTON  = self.H_LEFT_BUTTON
		self.X_RIGHT_BUTTON  = self.X_GRAPH + self.W_GRAPH - self.W_RIGHT_BUTTON
		self.Y_RIGHT_BUTTON  = self.Y_LEFT_BUTTON

		self.X_LEFT_LABEL   = self.X_LEFT_BUTTON + self.W_LEFT_BUTTON + 10
		self.X_RIGHT_LABEL  = self.X_RIGHT_BUTTON - 10
		self.Y_LABEL		= self.Y_GRAPH + self.H_GRAPH + 3

		self.X_LEGEND_MARGIN	= 10
		self.Y_LEGEND_MARGIN	= 5
		self.X_LEGEND_LINE	= self.X_LEGEND_MARGIN
		self.Y_LEGEND_LINE	= self.Y_LEGEND_MARGIN + 9  # to center it relative to the text
		self.W_LEGEND_LINE	= 30
		self.X_LEGEND_TEXT	= self.X_LEGEND_LINE + self.W_LEGEND_LINE + 10
		self.Y_LEGEND_TEXT	= self.Y_LEGEND_MARGIN
		self.H_LEGEND_TEXT	= 16

#BUG: Change Graphs - start
		self.Graph_Status_1in1 = 0
		self.Graph_Status_7in1 = 1
		self.Graph_Status_3in1 = 2
		self.Graph_Status_Current = self.Graph_Status_1in1
		self.Graph_Status_Prior = self.Graph_Status_7in1
#		self.BIG_GRAPH = False

# the 7-in-1 graphs are layout out as follows:
#    0 1 2
#    L 3 4
#    L 5 6
#
# where L is the legend and a number represents a graph
		self.X_7_IN_1_CHART_ADJ = [0, 1, 2, 1, 2, 1, 2]
		self.Y_7_IN_1_CHART_ADJ = [0, 0, 0, 1, 1, 2, 2]

# the 3-in-1 graphs are layout out as follows:
#    0 1
#    L 2
#
# where L is the legend and a number represents a graph
		self.X_3_IN_1_CHART_ADJ = [0, 1, 1]
		self.Y_3_IN_1_CHART_ADJ = [0, 0, 1]
#BUG: Change Graphs - end

############################################### DEMOGRAPHICS ###############################################

		self.X_CHART = 45
		self.Y_CHART = 80
		self.W_CHART = 934
		self.H_CHART = 600

		self.BUTTON_SIZE = 20

		self.W_TEXT = 140
		self.H_TEXT = 15
		self.X_TEXT_BUFFER = 0
		self.Y_TEXT_BUFFER = 43

		self.X_COL_1 = 535
		self.X_COL_2 = self.X_COL_1 + self.W_TEXT + self.X_TEXT_BUFFER
		self.X_COL_3 = self.X_COL_2 + self.W_TEXT + self.X_TEXT_BUFFER
		self.X_COL_4 = self.X_COL_3 + self.W_TEXT + self.X_TEXT_BUFFER

		self.Y_ROW_1 = 100
		self.Y_ROW_2 = self.Y_ROW_1 + self.H_TEXT + self.Y_TEXT_BUFFER
		self.Y_ROW_3 = self.Y_ROW_2 + self.H_TEXT + self.Y_TEXT_BUFFER
		self.Y_ROW_4 = self.Y_ROW_3 + self.H_TEXT + self.Y_TEXT_BUFFER
		self.Y_ROW_5 = self.Y_ROW_4 + self.H_TEXT + self.Y_TEXT_BUFFER
		self.Y_ROW_6 = self.Y_ROW_5 + self.H_TEXT + self.Y_TEXT_BUFFER
		self.Y_ROW_7 = self.Y_ROW_6 + self.H_TEXT + self.Y_TEXT_BUFFER
		self.Y_ROW_8 = self.Y_ROW_7 + self.H_TEXT + self.Y_TEXT_BUFFER
		self.Y_ROW_9 = self.Y_ROW_8 + self.H_TEXT + self.Y_TEXT_BUFFER
		self.Y_ROW_10 = self.Y_ROW_9 + self.H_TEXT + self.Y_TEXT_BUFFER

		self.bAbleToShowAllPlayers = false
		self.iShowingPlayer = -1
		self.aiDropdownPlayerIDs = []

############################################### TOP CITIES ###############################################

		self.X_LEFT_PANE = 45
		self.Y_LEFT_PANE = 70
		self.W_LEFT_PANE = 470
		self.H_LEFT_PANE = 620

		# Text

		self.W_TC_TEXT = 280
		self.H_TC_TEXT = 15
		self.X_TC_TEXT_BUFFER = 0
		self.Y_TC_TEXT_BUFFER = 43

		# Animated City thingies

		self.X_CITY_ANIMATION = self.X_LEFT_PANE + 20
		self.Z_CITY_ANIMATION = self.Z_BACKGROUND - 0.5
		self.W_CITY_ANIMATION = 150
		self.H_CITY_ANIMATION = 110
		self.Y_CITY_ANIMATION_BUFFER = self.H_CITY_ANIMATION / 2

		self.X_ROTATION_CITY_ANIMATION = -20
		self.Z_ROTATION_CITY_ANIMATION = 30
		self.SCALE_ANIMATION = 0.5

		# Placement of Cities

		self.X_COL_1_CITIES = self.X_LEFT_PANE + 20
		self.Y_CITIES_BUFFER = 118

		self.Y_ROWS_CITIES = []
		self.Y_ROWS_CITIES.append(self.Y_LEFT_PANE + 20)
		for i in range(1,5):
			self.Y_ROWS_CITIES.append(self.Y_ROWS_CITIES[i-1] + self.Y_CITIES_BUFFER)

		self.X_COL_1_CITIES_DESC = self.X_LEFT_PANE + self.W_CITY_ANIMATION + 30
		self.Y_CITIES_DESC_BUFFER = -4
		self.W_CITIES_DESC = 275
		self.H_CITIES_DESC = 60

		self.Y_CITIES_WONDER_BUFFER = 57
		self.W_CITIES_WONDER = 275
		self.H_CITIES_WONDER = 51

############################################### WONDERS ###############################################

		self.X_RIGHT_PANE = 520
		self.Y_RIGHT_PANE = 70
		self.W_RIGHT_PANE = 460
		self.H_RIGHT_PANE = 620

		# Info about this wonder, e.g. name, cost so on

		self.X_STATS_PANE = self.X_RIGHT_PANE + 20
		self.Y_STATS_PANE = self.Y_RIGHT_PANE + 20
		self.W_STATS_PANE = 210
		self.H_STATS_PANE = 220

		# Wonder mode dropdown Box

		self.X_DROPDOWN = self.X_RIGHT_PANE + 240 + 3 # the 3 is the 'fudge factor' due to the widgets not lining up perfectly
		self.Y_DROPDOWN = self.Y_RIGHT_PANE + 20
		self.W_DROPDOWN = 200

		# List Box that displays all wonders built

		self.X_WONDER_LIST = self.X_RIGHT_PANE + 240 + 6 # the 6 is the 'fudge factor' due to the widgets not lining up perfectly
		self.Y_WONDER_LIST = self.Y_RIGHT_PANE + 60
		self.W_WONDER_LIST = 200 - 6 # the 6 is the 'fudge factor' due to the widgets not lining up perfectly
		self.H_WONDER_LIST = 180

		# Animated Wonder thingies

		self.X_WONDER_GRAPHIC = 540
		self.Y_WONDER_GRAPHIC = self.Y_RIGHT_PANE + 20 + 200 + 35
		self.W_WONDER_GRAPHIC = 420
		self.H_WONDER_GRAPHIC = 190

		self.X_ROTATION_WONDER_ANIMATION = -20
		self.Z_ROTATION_WONDER_ANIMATION = 30

		# Icons used for Projects instead because no on-map art exists
		self.X_PROJECT_ICON = self.X_WONDER_GRAPHIC + self.W_WONDER_GRAPHIC / 2
		self.Y_PROJECT_ICON = self.Y_WONDER_GRAPHIC + self.H_WONDER_GRAPHIC / 2
		self.W_PROJECT_ICON = 128

		# Special Stats about this wonder

		self.X_SPECIAL_TITLE = 540
		self.Y_SPECIAL_TITLE = 310 + 200 + 7

		self.X_SPECIAL_PANE = 540
		self.Y_SPECIAL_PANE = 310 + 200 + 20 + 15
		self.W_SPECIAL_PANE = 420
		self.H_SPECIAL_PANE = 140 - 15

		self.szWDM_WorldWonder = "World Wonders"
		self.szWDM_NatnlWonder = "National Wonders"
		self.szWDM_Project = "Projects"

		self.BUGWorldWonderWidget = self.szWDM_WorldWonder + "Widget"
		self.BUGNatWonderWidget = self.szWDM_NatnlWonder + "Widget"
		self.BUGProjectWidget = self.szWDM_Project + "Widget"

		self.szWonderDisplayMode = self.szWDM_WorldWonder

		self.iWonderID = -1			# BuildingType ID of the active wonder, e.g. Palace is 0, Globe Theater is 66
		self.iActiveWonderCounter = 0		# Screen ID for this wonder (0, 1, 2, etc.) - different from the above variable

		self.X_WONDERS_CHART = self.X_RIGHT_PANE + 20 #DanF
		self.Y_WONDERS_CHART = self.Y_RIGHT_PANE + 60
		self.W_WONDERS_CHART = 420
		self.H_WONDERS_CHART = 540

############################################### STATISTICS ###############################################

		# STATISTICS TAB

		# Top Panel

		self.X_STATS_TOP_PANEL = 45
		self.Y_STATS_TOP_PANEL = 75
		self.W_STATS_TOP_PANEL = 935
		self.H_STATS_TOP_PANEL = 180

		# Leader

		self.X_LEADER_ICON = 250
		self.Y_LEADER_ICON = 95
		self.H_LEADER_ICON = 140
		self.W_LEADER_ICON = 110

		# Top Chart

		self.X_STATS_TOP_CHART = 400
		self.Y_STATS_TOP_CHART = 130
		self.W_STATS_TOP_CHART = 380
		self.H_STATS_TOP_CHART = 102

		self.STATS_TOP_CHART_W_COL_0 = 304
		self.STATS_TOP_CHART_W_COL_1 = 76

		self.iNumTopChartCols = 2

		self.X_LEADER_NAME = self.X_STATS_TOP_CHART
		self.Y_LEADER_NAME = self.Y_STATS_TOP_CHART - 40

		self.reset()

	def initText(self):
		# K-Mod
		# only execute this function once...
		if self.iLanguageLoaded == CyGame().getCurrentLanguage() or not CyGame().isFinalInitialized():
			return
		self.iLanguageLoaded = CyGame().getCurrentLanguage()

		# Calculate width for the links.
		self.PAGE_LINK_WIDTH[:] = []
		width_list = []
		for i in self.PAGE_NAME_LIST:
			width_list.append(CyInterface().determineWidth(localText.getText(i, ()).upper()) + 20)
		total_width = sum(width_list) + CyInterface().determineWidth(localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT", ()).upper()) + 20;

		for i in width_list:
			self.PAGE_LINK_WIDTH.append((self.X_EXIT * i + total_width/2) / total_width)
		# K-Mod end

		###### TEXT ######
		self.SCREEN_TITLE = u"<font=4b>" + localText.getText("TXT_KEY_INFO_SCREEN", ()).upper() + u"</font>"

		self.EXIT_TEXT = u"<font=4>" + localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT", ()).upper() + u"</font>"

		self.TEXT_SHOW_ALL_PLAYERS =  localText.getText("TXT_KEY_SHOW_ALL_PLAYERS", ())
		self.TEXT_SHOW_ALL_PLAYERS_GRAY = localText.getColorText("TXT_KEY_SHOW_ALL_PLAYERS", (), gc.getInfoTypeForString("COLOR_PLAYER_GRAY")).upper()
		
		self.TEXT_ENTIRE_HISTORY = localText.getText("TXT_KEY_INFO_ENTIRE_HISTORY", ())
		
		self.TEXT_SCORE = localText.getText("TXT_KEY_GAME_SCORE", ())
		self.TEXT_POWER = localText.getText("TXT_KEY_POWER", ())
		self.TEXT_CULTURE = localText.getObjectText("TXT_KEY_COMMERCE_CULTURE", 0)
		self.TEXT_ESPIONAGE = localText.getObjectText("TXT_KEY_ESPIONAGE_CULTURE", 0)

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

		self.TEXT_TIME_PLAYED = localText.getText("TXT_KEY_INFO_SCREEN_TIME_PLAYED", ())
		self.TEXT_CITIES_CURRENT = localText.getText("TXT_KEY_CONCEPT_CITIES", ()) # K-Mod
		self.TEXT_CITIES_BUILT = localText.getText("TXT_KEY_INFO_SCREEN_CITIES_BUILT", ())
		self.TEXT_CITIES_RAZED = localText.getText("TXT_KEY_INFO_SCREEN_CITIES_RAZED", ())
		self.TEXT_NUM_GOLDEN_AGES = localText.getText("TXT_KEY_INFO_SCREEN_NUM_GOLDEN_AGES", ())
		self.TEXT_NUM_RELIGIONS_FOUNDED = localText.getText("TXT_KEY_INFO_SCREEN_RELIGIONS_FOUNDED", ())

		self.TEXT_CURRENT = localText.getText("TXT_KEY_CURRENT", ())
		self.TEXT_UNITS = localText.getText("TXT_KEY_CONCEPT_UNITS", ())
		self.TEXT_BUILDINGS = localText.getText("TXT_KEY_CONCEPT_BUILDINGS", ())
		self.TEXT_KILLED = localText.getText("TXT_KEY_INFO_SCREEN_KILLED", ())
		self.TEXT_LOST = localText.getText("TXT_KEY_INFO_SCREEN_LOST", ())
		self.TEXT_BUILT = localText.getText("TXT_KEY_INFO_SCREEN_BUILT", ())
		self.TEXT_IMPROVEMENTS = localText.getText("TXT_KEY_CONCEPT_IMPROVEMENTS", ())

#BUG: Change Graphs - start
		self.SHOW_ALL = u"<font=2>" + localText.getText("TXT_KEY_SHOW_ALL", ()) + u"</font>"
		self.SHOW_NONE = u"<font=2>" + localText.getText("TXT_KEY_SHOW_NONE", ()) + u"</font>"
		self.LOG_SCALE = u"<font=2>" + localText.getText("TXT_KEY_LOGSCALE", ()) + u"</font>"

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
#BUG: Change Graphs - end

		# world wonder / national wonder / projects icons / buttons
		self.BUGWorldWonder_On = ArtFileMgr.getInterfaceArtInfo("BUG_WORLDWONDER_ON").getPath()
		self.BUGWorldWonder_Off = ArtFileMgr.getInterfaceArtInfo("BUG_WORLDWONDER_OFF").getPath()
		self.BUGNatWonder_On = ArtFileMgr.getInterfaceArtInfo("BUG_NATWONDER_ON").getPath()
		self.BUGNatWonder_Off = ArtFileMgr.getInterfaceArtInfo("BUG_NATWONDER_OFF").getPath()
		self.BUGProject_On = ArtFileMgr.getInterfaceArtInfo("BUG_PROJECT_ON").getPath()
		self.BUGProject_Off = ArtFileMgr.getInterfaceArtInfo("BUG_PROJECT_OFF").getPath()

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

	def getLastTurn(self):
		return (gc.getGame().getReplayMessageTurn(gc.getGame().getNumReplayMessages()-1))

	# Screen construction function
	def showScreen(self, iTurn, iTabID, iEndGame):

#BUG Timer
		self.timer = BugUtil.Timer("InfoScreen")

		self.initText();

		self.iStartTurn = 0
		for iI in range(gc.getGameSpeedInfo(gc.getGame().getGameSpeedType()).getNumTurnIncrements()):
			self.iStartTurn += gc.getGameSpeedInfo(gc.getGame().getGameSpeedType()).getGameTurnInfo(iI).iNumGameTurnsPerIncrement
		self.iStartTurn *= gc.getEraInfo(gc.getGame().getStartEra()).getStartPercent()
		self.iStartTurn /= 100

		self.iTurn = 0

		if (iTurn > self.getLastTurn()):
			return

		# Create a new screen
		screen = self.getScreen()
		if screen.isActive():
			return
		screen.setRenderInterfaceOnly(True)
		screen.showScreen(PopupStates.POPUPSTATE_IMMEDIATE, False)

		self.reset()

		self.deleteAllWidgets()

		# Set the background widget and exit button
		screen.addDDSGFC("DemographicsScreenBackground", ArtFileMgr.getInterfaceArtInfo("MAINMENU_SLIDESHOW_LOAD").getPath(), 0, 0, self.W_SCREEN, self.H_SCREEN, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		screen.addPanel( "TechTopPanel", u"", u"", True, False, 0, 0, self.W_SCREEN, 55, PanelStyles.PANEL_STYLE_TOPBAR )
		screen.addPanel( "TechBottomPanel", u"", u"", True, False, 0, 713, self.W_SCREEN, 55, PanelStyles.PANEL_STYLE_BOTTOMBAR )
		screen.showWindowBackground( False )
		screen.setDimensions(screen.centerX(self.X_SCREEN), screen.centerY(self.Y_SCREEN), self.W_SCREEN, self.H_SCREEN)
		self.szExitButtonName = self.getNextWidgetName()
		screen.setText(self.szExitButtonName, "Background", self.EXIT_TEXT, CvUtil.FONT_RIGHT_JUSTIFY, self.X_EXIT, self.Y_EXIT, self.Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		# Header...
		self.szHeaderWidget = self.getNextWidgetName()
		screen.setLabel(self.szHeaderWidget, "Background", self.SCREEN_TITLE, CvUtil.FONT_CENTER_JUSTIFY, self.X_TITLE, self.Y_TITLE, self.Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# Help area for tooltips
		screen.setHelpTextArea(self.W_HELP_AREA, FontTypes.SMALL_FONT, self.X_SCREEN, self.Y_SCREEN, self.Z_HELP_AREA, 1, ArtFileMgr.getInterfaceArtInfo("POPUPS_BACKGROUND_TRANSPARENT").getPath(), True, True, CvUtil.FONT_LEFT_JUSTIFY, 0 )

		self.DEBUG_DROPDOWN_ID = ""

		if (CyGame().isDebugMode()):
			self.DEBUG_DROPDOWN_ID = "InfoScreenDropdownWidget"
			self.szDropdownName = self.DEBUG_DROPDOWN_ID
			screen.addDropDownBoxGFC(self.szDropdownName, 22, 12, 300, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
			for j in range(gc.getMAX_CIV_PLAYERS()): # advc.007: was MAX_PLAYERS
				if (gc.getPlayer(j).isAlive()):
					screen.addPullDownString(self.szDropdownName, gc.getPlayer(j).getName(), j, j, False)

		self.iActivePlayer = CyGame().getActivePlayer()
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
		if self.iActiveTab == -1:
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
		xLink = 0
		for i in range (len(self.PAGE_NAME_LIST)):
			szTextId = "InfoTabButton"+str(i)
			if (self.iActiveTab != i):
				screen.setText (szTextId, "", u"<font=4>" + localText.getText (self.PAGE_NAME_LIST[i], ()).upper() + u"</font>", CvUtil.FONT_CENTER_JUSTIFY, xLink+self.PAGE_LINK_WIDTH[i]/2, self.Y_LINK, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, i, -1)
			else:
				screen.setText (szTextId, "", u"<font=4>" + localText.getColorText (self.PAGE_NAME_LIST[i], (), gc.getInfoTypeForString ("COLOR_YELLOW")).upper() + u"</font>", CvUtil.FONT_CENTER_JUSTIFY, xLink+self.PAGE_LINK_WIDTH[i]/2, self.Y_LINK, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			xLink += self.PAGE_LINK_WIDTH[i]

		if (self.iActiveTab >= 0 and self.iActiveTab < len(self.PAGE_NAME_LIST)):
			self.PAGE_DRAW_LIST[self.iActiveTab]()
		#

#############################################################################################################
#################################################### GRAPH ##################################################
#############################################################################################################

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
		now   = CyGame().getGameTurn()
		nTurns = now - start - 1
		screen.addPullDownString(self.szTurnsDropdownWidget, self.TEXT_ENTIRE_HISTORY, 0, 0, False)
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
		end   = CyGame().getGameTurn() - 1
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
			screen.addLineGFC(sGRAPH_CANVAS_ID, self.GRAPH_H_LINE, 0, self.ySelPt, self.W_GRAPH, self.ySelPt, gc.getInfoTypeForString("COLOR_GREY"))
			screen.addLineGFC(sGRAPH_CANVAS_ID, self.GRAPH_V_LINE, self.xSelPt, 0, self.xSelPt, self.H_GRAPH, gc.getInfoTypeForString("COLOR_GREY"))
		else:
			screen.addLineGFC(sGRAPH_CANVAS_ID, self.GRAPH_H_LINE, -1, -1, -1, -1, gc.getInfoTypeForString("COLOR_GREY"))
			screen.addLineGFC(sGRAPH_CANVAS_ID, self.GRAPH_V_LINE, -1, -1, -1, -1, gc.getInfoTypeForString("COLOR_GREY"))


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

		screen.setLabel(self.getNextWidgetName(), "", u"<font=2>" + self.getTurnDate(turn) + u"</font>",
						just , x , self.Y_LABEL, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

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
		screen.addDrawControl(zsGRAPH_CANVAS_ID, ArtFileMgr.getInterfaceArtInfo("SCREEN_BG").getPath(), iX_GRAPH, iY_GRAPH, iW_GRAPH, iH_GRAPH, WidgetTypes.WIDGET_GENERAL, -1, -1)

		sButton = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTON_NULL").getPath()
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
				zsText = self.sGraphText[0][iGraphID]   #u"<font=3>" + localText.getColorText("TXT_KEY_INFO_GRAPH", (), gc.getInfoTypeForString("COLOR_YELLOW")).upper() + u"</font>"
				screen.setText(self.sGraphTextHeadingWidget[iGraphID], "", zsText, CvUtil.FONT_LEFT_JUSTIFY, iX_GRAPH + 10, iY_GRAPH_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
#BUG: Change Graphs - start

		self.timer.log("single graph done")
		self.timer.start()

		return

	def drawLegend(self):
		screen = self.getScreen()
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
				if not gc.getPlayer(p).isAlive(): szPlayerName += " [" + self.BUG_LEGEND_DEAD + "]"
				if iW_LEGEND < self.X_LEGEND_TEXT + CyInterface().determineWidth(szPlayerName) + 10:
					iW_LEGEND = self.X_LEGEND_TEXT + CyInterface().determineWidth(szPlayerName) + 10
			for p in self.aiPlayersMetNAEspionage:
				# <advc.091>
				if bIgnoreEspionage and self.showTotalScoreGraph(p):
					continue # </advc.091>
				szPlayerName = self.getPlayerName(p)
				if not gc.getPlayer(p).isAlive(): szPlayerName += " [" + self.BUG_LEGEND_DEAD + "]"
				if iW_LEGEND < self.X_LEGEND_TEXT + CyInterface().determineWidth(szPlayerName) + 10:
					iW_LEGEND = self.X_LEGEND_TEXT + CyInterface().determineWidth(szPlayerName) + 10
		iLegendRows = self.iNumPlayersMet + self.iNumPlayersMetNAEspionage # advc.091
		if not AdvisorOpt.isGraphs():
			self.H_LEGEND = 2 * self.Y_LEGEND_MARGIN + iLegendRows * self.H_LEGEND_TEXT + 3
			if AdvisorOpt.isGraphsLogScale():
				self.H_LEGEND += self.H_LEGEND_TEXT
			self.Y_LEGEND = self.Y_GRAPH + self.H_GRAPH - self.H_LEGEND
		else:
			self.H_LEGEND = 2 * self.Y_LEGEND_MARGIN + (iLegendRows + 4) * self.H_LEGEND_TEXT + 3
			if AdvisorOpt.isGraphsLogScale():
				self.H_LEGEND += self.H_LEGEND_TEXT

			self.X_LEGEND = self.X_MARGIN + 5
			if self.Graph_Status_Current == self.Graph_Status_1in1:
				self.Y_LEGEND = self.Y_MARGIN + 40
			else:
				self.Y_LEGEND = self.Y_GRAPH + self.H_GRAPH - self.H_LEGEND
#BUG: Change Graphs - start

#		self.LEGEND_PANEL_ID = self.getNextWidgetName()
		screen.addPanel(self.getNextWidgetName(), "", "", true, true, 
						self.X_LEGEND, self.Y_LEGEND, iW_LEGEND, self.H_LEGEND,
						PanelStyles.PANEL_STYLE_IN)

#		self.LEGEND_CANVAS_ID = self.getNextWidgetName()
		sLEGEND_CANVAS_ID = self.getNextWidgetName()
		screen.addDrawControl(sLEGEND_CANVAS_ID, None, self.X_LEGEND, self.Y_LEGEND, iW_LEGEND, self.H_LEGEND, WidgetTypes.WIDGET_GENERAL, -1, -1)

		yLine = self.Y_LEGEND_LINE
		yText = self.Y_LEGEND + self.Y_LEGEND_TEXT

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
				self.drawLine(screen, sLEGEND_CANVAS_ID, self.X_LEGEND_LINE, yLine, self.X_LEGEND_LINE + self.W_LEGEND_LINE, yLine, lineColor, True)
			else:
				textColorR = 175
				textColorG = 175
				textColorB = 175
				textColorA = gc.getPlayer(p).getPlayerTextColorA()
#BUG: Change Graphs - end

			str = u"<color=%d,%d,%d,%d>%s</color>" %(textColorR,textColorG,textColorB,textColorA,name)

#BUG: Change Graphs - start
			if AdvisorOpt.isGraphs():
				screen.setText(self.sPlayerTextWidget[p], "", u"<font=2>" + str + u"</font>", CvUtil.FONT_LEFT_JUSTIFY,
							   self.X_LEGEND + self.X_LEGEND_TEXT, yText, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			else:
				screen.setLabel(self.sPlayerTextWidget[p], "", u"<font=2>" + str + u"</font>", CvUtil.FONT_LEFT_JUSTIFY,
								self.X_LEGEND + self.X_LEGEND_TEXT, yText, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
#BUG: Change Graphs - end

			yLine += self.H_LEGEND_TEXT
			yText += self.H_LEGEND_TEXT

#BUG: Change Graphs - start
# ADD players where you don't have enough espionage points
		if AdvisorOpt.isGraphs():
			# add blank line
			yLine += self.H_LEGEND_TEXT
			yText += self.H_LEGEND_TEXT
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
					name += " [" + self.BUG_LEGEND_DEAD + "]"
				else:
					textColorR = gc.getPlayer(p).getPlayerTextColorR()
					textColorG = gc.getPlayer(p).getPlayerTextColorG()
					textColorB = gc.getPlayer(p).getPlayerTextColorB()
					textColorA = gc.getPlayer(p).getPlayerTextColorA()
				str = u"<color=%d,%d,%d,%d>%s</color>" %(textColorR,textColorG,textColorB,textColorA,name)
				screen.setLabel(self.sPlayerTextWidget[i], "", u"<font=2>" + str + u"</font>", CvUtil.FONT_LEFT_JUSTIFY, self.X_LEGEND + self.X_LEGEND_TEXT + 2, yText, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				yLine += self.H_LEGEND_TEXT
				yText += self.H_LEGEND_TEXT

			yText += self.H_LEGEND_TEXT
			xShow = self.X_LEGEND + iW_LEGEND / 2
			screen.setText(self.sShowAllWidget, "", self.SHOW_ALL, CvUtil.FONT_CENTER_JUSTIFY, xShow, yText, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			yText += self.H_LEGEND_TEXT
			screen.setText(self.sShowNoneWidget, "", self.SHOW_NONE, CvUtil.FONT_CENTER_JUSTIFY, xShow, yText, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		if AdvisorOpt.isGraphsLogScale():
			xShow = self.X_LEGEND + iW_LEGEND / 2
			if AdvisorOpt.isGraphs():
				yText += self.H_LEGEND_TEXT
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

#############################################################################################################
################################################# DEMOGRAPHICS ##############################################
#############################################################################################################

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
	def getPlayerValueStr(self, valuePlayerPair, szMeasure = "", aiGroup = None):
		iPlayer = valuePlayerPair[1]
		szPlayerName = ""
		bUnknown = True
		if iPlayer > 0:
			player = gc.getPlayer(iPlayer)
			if self.pActiveTeam.isHasMet(player.getTeam()) or self.bRevealAll:
				bUnknown = False
				szPlayerName = player.getName()
		# Better just leave it empty
		#if bUnknown:
		#	szPlayerName = localText.getText("TXT_KEY_UNKNOWN", ())
		if (bUnknown or not self.pActivePlayer.canSeeDemographics(iPlayer)) and not self.bRevealAll:
			if self.bAlwaysShowBestWorstNameIfMet:
				return (szPlayerName,"?")
			return ("","?")
		if self.bShowBestKnown and not self.bRevealAll and not aiGroup is None:
			# Make sure player name isn't too long (I use the same code in CvOptionsScreen)
			iCharLimit = 18
			if len(szPlayerName) > iCharLimit:
				szPlayerName = szPlayerName[:iCharLimit-3] + '...'
			szPlayerName += " (" + localText.getText("TXT_KEY_DEMO_SCREEN_RANK_TEXT", ()) + " " + str(self.getRank(aiGroup, iPlayer)) + ")"
		return (szPlayerName, self.separateThousands(valuePlayerPair[0]) + szMeasure)
	
	def getPlayerStr(self, valuePlayerPair, aiGroup = None):
		return self.getPlayerValueStr(valuePlayerPair, "", aiGroup)[0]
		
	def getValueStr(self, valuePlayerPair, szMeasure = ""):
		return self.getPlayerValueStr(valuePlayerPair, szMeasure)[1]
	# Based on function in CvGameCoreUtils. Not worth exporting those two lines.
	def roundToMultiple(self, iValue, iMultiple):
		r = int(iValue + 0.5 * iMultiple)
		return r - (r % iMultiple)
	
	def separateThousands(self, iValue):
		szSep = localText.getText("TXT_KEY_THOUSANDS_SEPARATOR", ())
		# The rest of the function is adopted from this StackOverflow answer by Nadia Alramli: https://stackoverflow.com/posts/1823189/revisions
		s = '%d' % iValue
		groups = []
		while s and s[-1].isdigit():
			groups.append(s[-3:])
			s = s[:-3]
		return s + szSep.join(reversed(groups))
	
	def getIcon(self, uFontSymbol):
		return CyGame().getSymbolID(uFontSymbol)
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

		# Lists of Player values - will be used to determine rank, strength and average per city
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
				# advc.077: Use the current value only for the active player
				iValue = self.computeHistory(self.ECONOMY_SCORE, iPlayerLoop, iGameTurn)
				if iPlayerLoop == self.iActivePlayer:
					iValue = pCurrPlayer.calculateTotalCommerce() # advc.077
					iEconomy = iValue
				else: # <advc.077>
					iActiveRivals += 1
					if self.pActivePlayer.canSeeDemographics(iPlayerLoop):
						iKnownRivalDemogr += 1
					# </advc.077>
					iEconomyGameAverage += iValue
				aiGroupEconomy.append((iValue, iPlayerLoop))
				
				#iValue = pCurrPlayer.calculateTotalYield(YieldTypes.YIELD_PRODUCTION)
				iValue = self.computeHistory(self.INDUSTRY_SCORE, iPlayerLoop, iGameTurn) # advc.077
				if iPlayerLoop == self.iActivePlayer:
					iValue = pCurrPlayer.calculateTotalYield(YieldTypes.YIELD_PRODUCTION) # advc.077
					iIndustry = iValue
				else:
					iIndustryGameAverage += iValue
				aiGroupIndustry.append((iValue, iPlayerLoop))

				#iValue = pCurrPlayer.calculateTotalYield(YieldTypes.YIELD_FOOD)
				iValue = self.computeHistory(self.AGRICULTURE_SCORE, iPlayerLoop, iGameTurn) # advc.077
				if iPlayerLoop == self.iActivePlayer:
					iValue = pCurrPlayer.calculateTotalYield(YieldTypes.YIELD_FOOD) # advc.077
					iAgriculture = iValue
				else:
					iAgricultureGameAverage += iValue
				aiGroupAgriculture.append((iValue, iPlayerLoop))
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
				aiGroupMilitary.append((iValue, iPlayerLoop))
				# advc.077: was *1000
				iValue = pCurrPlayer.getTotalLand() * iLandCoeff
				if iPlayerLoop == self.iActivePlayer:
					iLandArea = iValue
				else:
					iLandAreaGameAverage += iValue
				aiGroupLandArea.append((iValue, iPlayerLoop))

				iValue = pCurrPlayer.getRealPopulation()
				if iPlayerLoop == self.iActivePlayer:
					iPopulation = iValue
				else:
					iPopulationGameAverage += iValue
				aiGroupPopulation.append((iValue, iPlayerLoop))

				iValue = self.getHappyValue(pCurrPlayer)
				if iPlayerLoop == self.iActivePlayer:
					iHappiness = iValue
				else:
					iHappinessGameAverage += iValue
				aiGroupHappiness.append((iValue, iPlayerLoop))

				iValue = self.getHealthValue(pCurrPlayer)
				if iPlayerLoop == self.iActivePlayer:
					iHealth = iValue
				else:
					iHealthGameAverage += iValue
				aiGroupHealth.append((iValue, iPlayerLoop))
				# advc.077: Don't subtract imports. Would be kind of nice to add pLoopPlayer.getGoldPerTurnByPlayer (summed up over all players), but would have to subtract gold payments too, and that wouldn't be consistent with counting only "export" trade routes.
				iValue = pCurrPlayer.calculateTotalExports(YieldTypes.YIELD_COMMERCE)# - pCurrPlayer.calculateTotalImports(YieldTypes.YIELD_COMMERCE)
				if iPlayerLoop == self.iActivePlayer:
					iNetTrade = iValue
				else:
					iNetTradeGameAverage += iValue
				aiGroupNetTrade.append((iValue, iPlayerLoop))
					
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
			# All occurrences of 'iNumActivePlayers - 1' replaced with iActiveRivals
			iEconomyGameAverage = iEconomyGameAverage / max(1, iActiveRivals)
			iIndustryGameAverage = iIndustryGameAverage / max(1, iActiveRivals)
			iAgricultureGameAverage = iAgricultureGameAverage / max(1, iActiveRivals)
			iMilitaryGameAverage = iMilitaryGameAverage / max(1, iActiveRivals)
			iLandAreaGameAverage = iLandAreaGameAverage / max(1, iActiveRivals)
			iPopulationGameAverage = iPopulationGameAverage / max(1, iActiveRivals)
			iHappinessGameAverage = iHappinessGameAverage / max(1, iActiveRivals)
			iHealthGameAverage = iHealthGameAverage / max(1, iActiveRivals)
			iNetTradeGameAverage = iNetTradeGameAverage / max(1, iActiveRivals)
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
		screen.addTableControlGFC(szTable, iColumns, self.X_CHART, self.Y_CHART, self.W_CHART, self.H_CHART, true, true, 32, 32, TableStyles.TABLE_STYLE_STANDARD)
		#screen.setTableColumnHeader(szTable, 0, self.TEXT_DEMOGRAPHICS_SMALL, 224) # Total graph width is 430
		# <advc.077> Changes throughout the rest of this function
		# Replacing literals
		iNextCol = 0
		iTitleCol = iNextCol # </advc.077>
		# 20 added to column width
		screen.setTableColumnHeader(szTable, iTitleCol, localText.getText("TXT_KEY_DEMO_SCREEN_TITLE", ()), 244) # K-Mod
		iBestWorstExtraWidth = 0
		iRankColumnWidth = 76 # was 90
		# Move rank up
		iRankCol = -1
		if not self.bRankInValueColumn:
			iNextCol += 1
			iRankCol = iNextCol 
			screen.setTableColumnHeader(szTable, iRankCol, self.TEXT_RANK, iRankColumnWidth)
		else: # Use the space for the best/ worst columns then. These may occasionally need the extra space b/c of long leader names and two-digit rank numbers.
			iBestWorstExtraWidth += iRankColumnWidth / 2
		iNextCol += 1
		iValueCol = iNextCol
		
		szValueHead = self.TEXT_VALUE
		if self.bRankInValueColumn:
			szValueHead += "/ " + self.TEXT_RANK
		# Column width was 155
		screen.setTableColumnHeader(szTable, iValueCol, szValueHead, 120)
		iBestCol = -1
		if True:#bShowBest: # Actually, show the header always. So that the player might guess that there is more info to come later in the game.
			iNextCol += 1
			iBestCol = iNextCol
			# Column width was 155
			screen.setTableColumnHeader(szTable, iBestCol, self.TEXT_BEST, 170 + iBestWorstExtraWidth)
		iAvgCol = -1
		if bShowAvg:
			iNextCol += 1
			iAvgCol = iNextCol
			# Column width was 155
			screen.setTableColumnHeader(szTable, iAvgCol, self.TEXT_AVERAGE, 154)
		iWorstCol = -1
		if bShowWorst:
			iNextCol += 1
			iWorstCol = iNextCol
			# Column width was 155
			screen.setTableColumnHeader(szTable, iWorstCol, self.TEXT_WORST, 170 + iBestWorstExtraWidth)
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
		# New: Append icons, and use different text keys for economy, industry, agriculture.
		szEconomyTitle =  localText.getText("TXT_KEY_DEMOGRAPHICS_ECONOMY_TEXT", ()) + (u" (%c+%c)" % (gc.getCommerceInfo(CommerceTypes.COMMERCE_GOLD).getChar(), gc.getCommerceInfo(CommerceTypes.COMMERCE_RESEARCH).getChar()))
		szIndustryTitle =  localText.getText("TXT_KEY_DEMOGRAPHICS_INDUSTRY_TEXT", ()) + (u" (%c)" % gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar())
		szAgricultureTitle = localText.getText("TXT_KEY_DEMOGRAPHICS_AGRICULTURE_TEXT", ()) + (u" (%c)" % gc.getYieldInfo(YieldTypes.YIELD_FOOD).getChar())
		szMilitaryTitle =  self.TEXT_MILITARY + (u" (%c)" % self.getIcon(FontSymbols.STRENGTH_CHAR))
		szHappinessTitle = self.TEXT_HAPPINESS + (u" (%c)" % self.getIcon(FontSymbols.HAPPY_CHAR))
		szHealthTitle = self.TEXT_HEALTH + (u" (%c)" % self.getIcon(FontSymbols.HEALTHY_CHAR))
		szExpTitle = self.TEXT_EXP + (u" (%c)" % self.getIcon(FontSymbols.TRADE_CHAR))
		# Add measure after a colon (and commented out below)
		szEconomyTitle += ": " + self.TEXT_ECONOMY_MEASURE
		szIndustryTitle += ": " + self.TEXT_INDUSTRY_MEASURE
		szAgricultureTitle += ": " + self.TEXT_AGRICULTURE_MEASURE
		szLandAreaTitle = self.TEXT_LAND_AREA + ": " + self.TEXT_LAND_AREA_MEASURE
		szHealthTitle += ": " + self.TEXT_HEALTH_MEASURE
		szExpTitle += ": " + self.TEXT_EXP_MEASURE
		# Row numbers after 12 changed; now evenly distributed.
		screen.setTableText(szTable, iTitleCol, 0, szEconomyTitle, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		#screen.setTableText(szTable, iTitleCol, 1, self.TEXT_ECONOMY_MEASURE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.setTableText(szTable, iTitleCol, 3, szIndustryTitle, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		#screen.setTableText(szTable, iTitleCol, 4, self.TEXT_INDUSTRY_MEASURE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.setTableText(szTable, iTitleCol, 6, szAgricultureTitle, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		#screen.setTableText(szTable, iTitleCol, 7, self.TEXT_AGRICULTURE_MEASURE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.setTableText(szTable, iTitleCol, 9, szMilitaryTitle, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# row was 11
		screen.setTableText(szTable, iTitleCol, 12, szLandAreaTitle, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		#screen.setTableText(szTable, iTitleCol, 12, self.TEXT_LAND_AREA_MEASURE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# row was 14
		screen.setTableText(szTable, iTitleCol, 15, self.TEXT_POPULATION, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# row was 16
		screen.setTableText(szTable, iTitleCol, 18, szHappinessTitle, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# row was 18
		screen.setTableText(szTable, iTitleCol, 21, szHealthTitle, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		#screen.setTableText(szTable, iTitleCol, 19, self.TEXT_HEALTH_MEASURE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		if self.bShowExports:
			# row was 21
			screen.setTableText(szTable, iTitleCol, 24, szExpTitle, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			#screen.setTableText(szTable, iTitleCol, 22, self.TEXT_EXP_MEASURE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		#iCol = 1
		# Decimal separators added. In the best/worst columns, it's easiest to do this for all values, so do it for all values here too.
		screen.setTableText(szTable, iValueCol, 0, self.separateThousands(iEconomy), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.setTableText(szTable, iValueCol, 3, self.separateThousands(iIndustry), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.setTableText(szTable, iValueCol, 6, self.separateThousands(iAgriculture), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.setTableText(szTable, iValueCol, 9, self.separateThousands(iMilitary), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# row was 11
		screen.setTableText(szTable, iValueCol, 12, self.separateThousands(iLandArea), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# row was 14
		screen.setTableText(szTable, iValueCol, 15, self.separateThousands(iPopulation), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# row was 16
		screen.setTableText(szTable, iValueCol, 18, self.separateThousands(iHappiness) + self.TEXT_HAPPINESS_MEASURE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# row was 18
		screen.setTableText(szTable, iValueCol, 21, self.separateThousands(iHealth), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		if self.bShowExports:
			# row was 21
			screen.setTableText(szTable, iValueCol, 24, self.separateThousands(iNetTrade), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		#iCol = 2
		if bShowBest:
			# Replaced str(i...GameBest) with getPlayerStr and getValueStr, and put them in separate rows.
			screen.setTableText(szTable, iBestCol, 1, self.getPlayerStr(economyGameBest, aiGroupEconomy), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, iBestCol, 0, self.getValueStr(economyGameBest), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, iBestCol, 4, self.getPlayerStr(industryGameBest, aiGroupIndustry), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, iBestCol, 3, self.getValueStr(industryGameBest), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, iBestCol, 7,self. getPlayerStr(agricultureGameBest, aiGroupAgriculture), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, iBestCol, 6, self.getValueStr(agricultureGameBest), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, iBestCol, 10, self.getPlayerStr(militaryGameBest, aiGroupMilitary), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, iBestCol, 9, self.getValueStr(militaryGameBest), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 12
			screen.setTableText(szTable, iBestCol, 13, self.getPlayerStr(landAreaGameBest, aiGroupLandArea), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 11
			screen.setTableText(szTable, iBestCol, 12, self.getValueStr(landAreaGameBest), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 15
			screen.setTableText(szTable, iBestCol, 16, self.getPlayerStr(populationGameBest, aiGroupPopulation), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 14
			screen.setTableText(szTable, iBestCol, 15, self.getValueStr(populationGameBest), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 17
			screen.setTableText(szTable, iBestCol, 19, self.getPlayerStr(happinessGameBest, aiGroupHappiness), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 16
			screen.setTableText(szTable, iBestCol, 18, self.getValueStr(happinessGameBest, self.TEXT_HAPPINESS_MEASURE), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 19
			screen.setTableText(szTable, iBestCol, 22, self.getPlayerStr(healthGameBest, aiGroupHealth), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 18
			screen.setTableText(szTable, iBestCol, 21, self.getValueStr(healthGameBest), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			if self.bShowExports:
				# row was 22
				screen.setTableText(szTable, iBestCol, 25, self.getPlayerStr(netTradeGameBest, aiGroupNetTrade), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				screen.setTableText(szTable, iBestCol, 24, self.getValueStr(netTradeGameBest), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		#iCol = 3
		if bShowAvg:
			# Decimal separators added
			screen.setTableText(szTable, iAvgCol, 0, self.separateThousands(iEconomyGameAverage), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, iAvgCol, 3, self.separateThousands(iIndustryGameAverage), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, iAvgCol, 6, self.separateThousands(iAgricultureGameAverage), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, iAvgCol, 9, self.separateThousands(iMilitaryGameAverage), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 11
			screen.setTableText(szTable, iAvgCol, 12, self.separateThousands(iLandAreaGameAverage), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 14
			screen.setTableText(szTable, iAvgCol, 15, self.separateThousands(iPopulationGameAverage), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 16
			screen.setTableText(szTable, iAvgCol, 18, self.separateThousands(iHappinessGameAverage) + self.TEXT_HAPPINESS_MEASURE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 18
			screen.setTableText(szTable, iAvgCol, 21, self.separateThousands(iHealthGameAverage), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			if self.bShowExports:
				# row was 21
				screen.setTableText(szTable, iAvgCol, 24, self.separateThousands(iNetTradeGameAverage), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		#iCol = 4
		if bShowWorst:
			# Replaced str(i...GameWorst) with getPlayerStr and getValueStr, and put them in a separate rows.
			screen.setTableText(szTable, iWorstCol, 1, self.getPlayerStr(economyGameWorst, aiGroupEconomy), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, iWorstCol, 0, self.getValueStr(economyGameWorst), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, iWorstCol, 4, self.getPlayerStr(industryGameWorst, aiGroupIndustry), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, iWorstCol, 3, self.getValueStr(industryGameWorst), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, iWorstCol, 7, self.getPlayerStr(agricultureGameWorst, aiGroupAgriculture), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, iWorstCol, 6, self.getValueStr(agricultureGameWorst), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, iWorstCol, 10, self.getPlayerStr(militaryGameWorst, aiGroupMilitary), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(szTable, iWorstCol, 9, self.getValueStr(militaryGameWorst), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 12
			screen.setTableText(szTable, iWorstCol, 13, self.getPlayerStr(landAreaGameWorst, aiGroupLandArea), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 11
			screen.setTableText(szTable, iWorstCol, 12, self.getValueStr(landAreaGameWorst), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 15
			screen.setTableText(szTable, iWorstCol, 16, self.getPlayerStr(populationGameWorst, aiGroupPopulation), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 14
			screen.setTableText(szTable, iWorstCol, 15, self.getValueStr(populationGameWorst), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 17
			screen.setTableText(szTable, iWorstCol, 19, self.getPlayerStr(happinessGameWorst, aiGroupHappiness), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 16
			screen.setTableText(szTable, iWorstCol, 18, self.getValueStr(happinessGameWorst, self.TEXT_HAPPINESS_MEASURE), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 19
			screen.setTableText(szTable, iWorstCol, 22, self.getPlayerStr(healthGameWorst, aiGroupHealth), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# row was 18
			screen.setTableText(szTable, iWorstCol, 21, self.getValueStr(healthGameWorst), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			if self.bShowExports:
				# row was 22
				screen.setTableText(szTable, iWorstCol, 25, self.getPlayerStr(netTradeGameWorst, aiGroupNetTrade), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				# row was 21
				screen.setTableText(szTable, iWorstCol, 24, self.getValueStr(netTradeGameWorst), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		#iCol = 5
		# Put it in iValueCol if that option is set
		iCol = iRankCol
		iOffset = 0
		if self.bRankInValueColumn:
			iCol = iValueCol
			iOffset = 1
		screen.setTableText(szTable, iCol, 0 + iOffset, str(iEconomyRank), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.setTableText(szTable, iCol, 3 + iOffset, str(iIndustryRank), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.setTableText(szTable, iCol, 6 + iOffset, str(iAgricultureRank), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.setTableText(szTable, iCol, 9 + iOffset, str(iMilitaryRank), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# row was 11
		screen.setTableText(szTable, iCol, 12 + iOffset, str(iLandAreaRank), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# row was 14
		screen.setTableText(szTable, iCol, 15 + iOffset, str(iPopulationRank), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# row was 16
		screen.setTableText(szTable, iCol, 18 + iOffset, str(iHappinessRank), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# row was 18
		screen.setTableText(szTable, iCol, 21 + iOffset, str(iHealthRank), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		if self.bShowExports:
			# row was 21
			screen.setTableText(szTable, iCol, 24 + iOffset, str(iNetTradeRank), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# </advc.077>
		return

#############################################################################################################
################################################## TOP CITIES ###############################################
#############################################################################################################

	def drawTopCitiesTab(self):

		screen = self.getScreen()

		# Background Panes
		self.szLeftPaneWidget = self.getNextWidgetName()
		screen.addPanel( self.szLeftPaneWidget, "", "", true, true,
			self.X_LEFT_PANE, self.Y_LEFT_PANE, self.W_LEFT_PANE, self.H_LEFT_PANE, PanelStyles.PANEL_STYLE_MAIN )#PanelStyles.PANEL_STYLE_DAWNTOP )

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
			screen.addPanel( szTextPanel, "", "", false, true,
				self.X_COL_1_CITIES_DESC, self.Y_ROWS_CITIES[iWidgetLoop] + self.Y_CITIES_DESC_BUFFER, self.W_CITIES_DESC, self.H_CITIES_DESC, PanelStyles.PANEL_STYLE_DAWNTOP )
			self.szCityNameWidgets.append(self.getNextWidgetName())
#			szProjectDesc = u"<font=3b>" + pProjectInfo.getDescription().upper() + u"</font>"
			szCityDesc = u"<font=4b>" + str(self.iCitySizes[iWidgetLoop]) + u"</font>" + " - " + u"<font=3b>" + self.szCityNames[iWidgetLoop] + u"</font>" + "\n"
			szCityDesc += self.szCityDescs[iWidgetLoop]
			screen.addMultilineText(self.szCityNameWidgets[iWidgetLoop], szCityDesc,
				self.X_COL_1_CITIES_DESC + 6, self.Y_ROWS_CITIES[iWidgetLoop] + self.Y_CITIES_DESC_BUFFER + 3, self.W_CITIES_DESC - 6, self.H_CITIES_DESC - 6, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
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
		self.drawCityWonderIcons();

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

		# Create Scrollable areas under each city
		self.szCityWonderScrollArea = []
		for iCityLoop in range (self.iNumCities):

			self.szCityWonderScrollArea.append(self.getNextWidgetName())

			#iScollAreaY = (self.Y_CITIES_BUFFER * iCityLoop) + 90 + self.Y_CITIES_WONDER_BUFFER

			szIconPanel = self.szCityWonderScrollArea[iCityLoop]
			screen.addPanel( szIconPanel, "", "", false, true,
				self.X_COL_1_CITIES_DESC, self.Y_ROWS_CITIES[iCityLoop] + self.Y_CITIES_WONDER_BUFFER + self.Y_CITIES_DESC_BUFFER, self.W_CITIES_DESC, self.H_CITIES_DESC, PanelStyles.PANEL_STYLE_DAWNTOP )

			# Now place the wonder buttons
			for iWonderLoop in range(aiTopCitiesNumWonders[iCityLoop]):

				iBuildingID = aaiTopCitiesWonders[iCityLoop][iWonderLoop]
				screen.attachImageButton( szIconPanel, "", gc.getBuildingInfo(iBuildingID).getButton(),
					GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuildingID, -1, False )

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

#############################################################################################################
################################################### WONDERS #################################################
#############################################################################################################

	def drawWondersTab(self):

		if AdvisorOpt.isShowInfoWonders():
			self.X_DROPDOWN = self.X_RIGHT_PANE + 20 # the 3 is the 'fudge factor' due to the widgets not lining up perfectly    #DanF 240 + 3
			self.Y_DROPDOWN = self.Y_RIGHT_PANE + 20
			self.W_DROPDOWN = 420 #DanF 200
		else:
			self.X_DROPDOWN = self.X_RIGHT_PANE + 240 + 3 # the 3 is the 'fudge factor' due to the widgets not lining up perfectly
			self.Y_DROPDOWN = self.Y_RIGHT_PANE + 20
			self.W_DROPDOWN = 200

		screen = self.getScreen()

		self.szRightPaneWidget = self.getNextWidgetName()
		screen.addPanel( self.szRightPaneWidget, "", "", true, true,
			self.X_RIGHT_PANE, self.Y_RIGHT_PANE, self.W_RIGHT_PANE, self.H_RIGHT_PANE, PanelStyles.PANEL_STYLE_MAIN )#PanelStyles.PANEL_STYLE_DAWNTOP )

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
				sDesc = localText.getText("TXT_KEY_TOP_CITIES_SCREEN_WORLD_WONDERS", ())
			elif self.szWonderDisplayMode == self.szWDM_NatnlWonder:
				sWW = self.BUGWorldWonder_Off
				sNW = self.BUGNatWonder_On
				sPj = self.BUGProject_Off
				sDesc = localText.getText("TXT_KEY_TOP_CITIES_SCREEN_NATIONAL_WONDERS", ())
			else:
				sWW = self.BUGWorldWonder_Off
				sNW = self.BUGNatWonder_Off
				sPj = self.BUGProject_On
				sDesc = localText.getText("TXT_KEY_PEDIA_CATEGORY_PROJECT", ())

			sDesc = u"<font=4>" + sDesc + u"</font>"

			screen.setImageButton(self.BUGWorldWonderWidget, sWW,  self.X_DROPDOWN +  0, self.Y_DROPDOWN, 24, 24, WidgetTypes.WIDGET_INFO_WORLD_WONDERS, -1, -1)
			screen.setImageButton(self.BUGNatWonderWidget, sNW,  self.X_DROPDOWN + 30, self.Y_DROPDOWN, 24, 24, WidgetTypes.WIDGET_INFO_NATIONAL_WONDERS, -1, -1)
			screen.setImageButton(self.BUGProjectWidget, sPj,  self.X_DROPDOWN + 60, self.Y_DROPDOWN, 24, 24, WidgetTypes.WIDGET_INFO_PROJECTS, -1, -1)

			screen.setLabel(self.getNextWidgetName(), "Background", sDesc, CvUtil.FONT_LEFT_JUSTIFY, self.X_DROPDOWN + 100, self.Y_DROPDOWN + 3, self.Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		else:
			screen.addDropDownBoxGFC(self.szWondersDropdownWidget,
				self.X_DROPDOWN, self.Y_DROPDOWN, self.W_DROPDOWN, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)

			if (self.szWonderDisplayMode == self.szWDM_WorldWonder):
				bDefault = true
			else:
				bDefault = false
			screen.addPullDownString(self.szWondersDropdownWidget, localText.getText("TXT_KEY_TOP_CITIES_SCREEN_WORLD_WONDERS", ()), 0, 0, bDefault )

			if (self.szWonderDisplayMode == self.szWDM_NatnlWonder):
				bDefault = true
			else:
				bDefault = false
			screen.addPullDownString(self.szWondersDropdownWidget, localText.getText("TXT_KEY_TOP_CITIES_SCREEN_NATIONAL_WONDERS", ()), 1, 1, bDefault )

			if (self.szWonderDisplayMode == self.szWDM_Project):
				bDefault = true
			else:
				bDefault = false
			screen.addPullDownString(self.szWondersDropdownWidget, localText.getText("TXT_KEY_PEDIA_CATEGORY_PROJECT", ()), 2, 2, bDefault )

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

				screen.appendListBoxString( self.szWondersListBox, szProjectName + " (" + szWonderBuiltBy + ")", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

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

				screen.appendListBoxString( self.szWondersListBox, szProjectName + " (" + szWonderBuiltBy + ")", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

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

				screen.appendListBoxString( self.szWondersListBox, szWonderName + " (" + szWonderBuiltBy + ")", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

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

				screen.appendListBoxString( self.szWondersListBox, szWonderName + " (" + szWonderBuiltBy + ")", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

	def drawWondersList(self):

		screen = self.getScreen()

		if (self.iNumWondersPermanentWidgets == 0):

			# Wonders List ListBox
			self.szWondersListBox = self.getNextWidgetName()
			screen.addListBoxGFC(self.szWondersListBox, "",
				self.X_WONDER_LIST, self.Y_WONDER_LIST, self.W_WONDER_LIST, self.H_WONDER_LIST, TableStyles.TABLE_STYLE_STANDARD )
			screen.setStyle(self.szWondersListBox, "Table_StandardCiv_Style")

			self.determineListBoxContents()

			self.iNumWondersPermanentWidgets = self.nWidgetCount

		self.szWondersTable = self.getNextWidgetName()

		# Stats Panel
		panelName = self.getNextWidgetName()
		screen.addPanel( panelName, "", "", true, true,
				 self.X_STATS_PANE, self.Y_STATS_PANE, self.W_STATS_PANE, self.H_STATS_PANE, PanelStyles.PANEL_STYLE_IN )

############################################### DISPLAY SINGLE WONDER ###############################################

		# Set default wonder if any exist in this list
		if (len(self.aiWonderListBoxIDs) > 0 and self.iWonderID == -1):
			self.iWonderID = self.aiWonderListBoxIDs[0]

		# Only display/do the following if a wonder is actively being displayed
		if (self.iWonderID > -1):

############################################### DISPLAY PROJECT MODE ###############################################

			if (self.szWonderDisplayMode == self.szWDM_Project):

				pProjectInfo = gc.getProjectInfo(self.iWonderID)

				# Stats panel (cont'd) - Name
				szProjectDesc = u"<font=3b>" + pProjectInfo.getDescription().upper() + u"</font>"
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

				screen.addMultilineText(self.getNextWidgetName(), szStatsText, self.X_STATS_PANE + 5, self.Y_STATS_PANE + 15, self.W_STATS_PANE - 10, self.H_STATS_PANE - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

				# Add Graphic
				iIconX = self.X_PROJECT_ICON - self.W_PROJECT_ICON / 2
				iIconY = self.Y_PROJECT_ICON - self.W_PROJECT_ICON / 2

				screen.addDDSGFC(self.getNextWidgetName(), gc.getProjectInfo(self.iWonderID).getButton(),
						 iIconX, iIconY, self.W_PROJECT_ICON, self.W_PROJECT_ICON, WidgetTypes.WIDGET_GENERAL, -1, -1 )

				# Special Abilities ListBox

				szSpecialTitle = u"<font=3b>" + localText.getText("TXT_KEY_PEDIA_SPECIAL_ABILITIES", ()) + u"</font>"
				self.szSpecialTitleWidget = self.getNextWidgetName()
				screen.setText(self.szSpecialTitleWidget, "", szSpecialTitle, CvUtil.FONT_LEFT_JUSTIFY, self.X_SPECIAL_TITLE, self.Y_SPECIAL_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

				panelName = self.getNextWidgetName()
				screen.addPanel( panelName, "", "", true, true,
						 self.X_SPECIAL_PANE, self.Y_SPECIAL_PANE, self.W_SPECIAL_PANE, self.H_SPECIAL_PANE, PanelStyles.PANEL_STYLE_IN)

				listName = self.getNextWidgetName()
				screen.attachListBoxGFC( panelName, listName, "", TableStyles.TABLE_STYLE_EMPTY )
				screen.enableSelect(listName, False)

				szSpecialText = CyGameTextMgr().getProjectHelp(self.iWonderID, True, None)
				splitText = string.split( szSpecialText, "\n" )
				for special in splitText:
					if len( special ) != 0:
						screen.appendListBoxString( listName, special, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

			else:

	############################################### DISPLAY WONDER MODE ###############################################

				pWonderInfo = gc.getBuildingInfo(self.iWonderID)

				# Stats panel (cont'd) - Name
				szWonderDesc = u"<font=3b>" + gc.getBuildingInfo(self.iWonderID).getDescription().upper() + u"</font>"
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

				screen.addMultilineText(self.getNextWidgetName(), szStatsText, self.X_STATS_PANE + 5, self.Y_STATS_PANE + 15, self.W_STATS_PANE - 10, self.H_STATS_PANE - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

				# Add Graphic
				screen.addBuildingGraphicGFC(self.getNextWidgetName(), self.iWonderID,
					self.X_WONDER_GRAPHIC, self.Y_WONDER_GRAPHIC, self.W_WONDER_GRAPHIC, self.H_WONDER_GRAPHIC,
					WidgetTypes.WIDGET_GENERAL, -1, -1, self.X_ROTATION_WONDER_ANIMATION, self.Z_ROTATION_WONDER_ANIMATION, self.SCALE_ANIMATION, True)

				# Special Abilities ListBox

				szSpecialTitle = u"<font=3b>" + localText.getText("TXT_KEY_PEDIA_SPECIAL_ABILITIES", ()) + u"</font>"
				self.szSpecialTitleWidget = self.getNextWidgetName()
				screen.setText(self.szSpecialTitleWidget, "", szSpecialTitle, CvUtil.FONT_LEFT_JUSTIFY, self.X_SPECIAL_TITLE, self.Y_SPECIAL_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

				panelName = self.getNextWidgetName()
				screen.addPanel( panelName, "", "", true, true,#localText.getText("TXT_KEY_PEDIA_SPECIAL_ABILITIES", ())
						 self.X_SPECIAL_PANE, self.Y_SPECIAL_PANE, self.W_SPECIAL_PANE, self.H_SPECIAL_PANE, PanelStyles.PANEL_STYLE_IN)

				listName = self.getNextWidgetName()
				screen.attachListBoxGFC( panelName, listName, "", TableStyles.TABLE_STYLE_EMPTY )
				screen.enableSelect(listName, False)

				szSpecialText = CyGameTextMgr().getBuildingHelp(self.iWonderID, True, False, False, None)
				splitText = string.split( szSpecialText, "\n" )
				for special in splitText:
					if len( special ) != 0:
						screen.appendListBoxString( listName, special, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

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
										self.aaWondersBuilt.append([pCity.getBuildingOriginalTime(iBuildingLoop),iBuildingLoop,localText.getText("TXT_KEY_UNKNOWN", ()),localText.getText("TXT_KEY_UNKNOWN", ())])
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
										self.aaWondersBuilt.append([pCity.getBuildingOriginalTime(iBuildingLoop),iBuildingLoop,localText.getText("TXT_KEY_UNKNOWN", ()), localText.getText("TXT_KEY_UNKNOWN", ())])
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
									self.aaWondersBuilt.append([-9999,iProjectLoop,localText.getText("TXT_KEY_UNKNOWN", ()),localText.getText("TXT_KEY_UNKNOWN", ())])
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
										self.aaWondersBuilt_BUG.append([pCity.getBuildingOriginalTime(iBuildingLoop),iBuildingLoop,False,localText.getText("TXT_KEY_UNKNOWN", ()),pCity,gc.getBARBARIAN_PLAYER()])
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
									self.aaWondersBuilt_BUG.append([-9999,iProjectLoop,False,localText.getText("TXT_KEY_UNKNOWN", ()), None, gc.getBARBARIAN_PLAYER()])
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
		screen.addTableControlGFC(self.szWondersTable, 5, self.X_WONDERS_CHART, self.Y_WONDERS_CHART, self.W_WONDERS_CHART, self.H_WONDERS_CHART,
					  True, True, 24,24, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSort(self.szWondersTable)

		zoomArt = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTONS_CITYSELECTION").getPath()

		sName = BugUtil.getPlainText("TXT_KEY_WONDER_NAME")
		sDate = BugUtil.getPlainText("TXT_KEY_WONDER_DATE")
		sOwner = BugUtil.getPlainText("TXT_KEY_WONDER_OWNER")
		sCity = BugUtil.getPlainText("TXT_KEY_WONDER_CITY")

		screen.setTableColumnHeader(self.szWondersTable, 0, "", 30)
		screen.setTableColumnHeader(self.szWondersTable, 1, sName, 115)
		# advc.002b: Confer 5 width from the owner column to the date column
		screen.setTableColumnHeader(self.szWondersTable, 2, sDate, 75)
		screen.setTableColumnHeader(self.szWondersTable, 3, sOwner, 95)
		screen.setTableColumnHeader(self.szWondersTable, 4, sCity, 100)

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
			szTurnYearBuilt = u"<font=2>%c</font>" % gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar()

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
			screen.setTableText(self.szWondersTable, 0, iWonderLoop, ""             , zoomArt, WidgetTypes.WIDGET_ZOOM_CITY, pCity.getOwner(), pCity.getID(), CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(self.szWondersTable, 1, iWonderLoop, szWonderName   , "", iWidget, iWonderType, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableInt (self.szWondersTable, 2, iWonderLoop, szTurnYearBuilt, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
			screen.setTableText(self.szWondersTable, 3, iWonderLoop, szWonderBuiltBy, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(self.szWondersTable, 4, iWonderLoop, szCityName     , "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

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
			bRevealed = pCity.GetCy().isRevealed(self.iActiveTeam, self.iActiveTeam == gc.getGame().getActiveTeam()) or self.bRevealAll
			if pCity and bRevealed:
				szCityName = pCity.getName()
			else:
				szCityName = u""

			if AdvisorOpt.isWonderListUsePlayerColor():
				szWonderBuiltBy = localText.changeTextColor(szWonderBuiltBy, color)
				szCityName = localText.changeTextColor(szCityName, color)

			screen.appendTableRow(self.szWondersTable)
			if bKnown and pCity and bRevealed:
				screen.setTableText(self.szWondersTable, 0, iWonderLoop+iWBB, "", zoomArt, WidgetTypes.WIDGET_ZOOM_CITY, pCity.getOwner(), pCity.getID(), CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(self.szWondersTable, 1, iWonderLoop+iWBB, szWonderName   , "", iWidget, iWonderType, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableInt (self.szWondersTable, 2, iWonderLoop+iWBB, szTurnYearBuilt, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
			screen.setTableText(self.szWondersTable, 3, iWonderLoop+iWBB, szWonderBuiltBy, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(self.szWondersTable, 4, iWonderLoop+iWBB, szCityName     , "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)


#############################################################################################################
################################################## STATISTICS ###############################################
#############################################################################################################

	def drawStatsTab(self):

		# Bottom Chart
#BUG: improvements - start
		if AdvisorOpt.isShowImprovements():
			self.X_STATS_BOTTOM_CHART = 45
			self.Y_STATS_BOTTOM_CHART = 280
			self.W_STATS_BOTTOM_CHART_UNITS = 455
			self.W_STATS_BOTTOM_CHART_BUILDINGS = 260
			self.W_STATS_BOTTOM_CHART_IMPROVEMENTS = 220
			self.H_STATS_BOTTOM_CHART = 410
		else:
			self.X_STATS_BOTTOM_CHART = 45
			self.Y_STATS_BOTTOM_CHART = 280
			self.W_STATS_BOTTOM_CHART_UNITS = 545
			self.W_STATS_BOTTOM_CHART_BUILDINGS = 390
			self.H_STATS_BOTTOM_CHART = 410
#BUG: improvements - end

		screen = self.getScreen()

		iNumUnits = gc.getNumUnitInfos()
		iNumBuildings = gc.getNumBuildingInfos()
		iNumImprovements = gc.getNumImprovementInfos()

		self.iNumUnitStatsChartCols = 5
		self.iNumBuildingStatsChartCols = 2
		self.iNumImprovementStatsChartCols = 2

		self.iNumUnitStatsChartRows = iNumUnits
		self.iNumBuildingStatsChartRows = iNumBuildings
		self.iNumImprovementStatsChartRows = iNumImprovements

################################################### CALCULATE STATS ###################################################

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

		aiImprovementsCurrent = []
		for iImprovementLoop in range(iNumImprovements):
			aiImprovementsCurrent.append(0)

		iGridW = CyMap().getGridWidth()
		iGridH = CyMap().getGridHeight()
		for iX in range(iGridW):
			for iY in range(iGridH):
				plot = CyMap().plot(iX, iY)
				if (plot.getOwner() == self.iActivePlayer):
					iType = plot.getImprovementType()
					if (iType != ImprovementTypes.NO_IMPROVEMENT):
						aiImprovementsCurrent[iType] += 1

################################################### TOP PANEL ###################################################

		# Add Panel
		szTopPanelWidget = self.getNextWidgetName()
		screen.addPanel( szTopPanelWidget, u"", u"", True, False, self.X_STATS_TOP_PANEL, self.Y_STATS_TOP_PANEL, self.W_STATS_TOP_PANEL, self.H_STATS_TOP_PANEL,
				 PanelStyles.PANEL_STYLE_DAWNTOP )

		# Leaderhead graphic
		#player = gc.getPlayer(gc.getGame().getActivePlayer())
		player = gc.getPlayer(self.iActivePlayer) # K-Mod
		szLeaderWidget = self.getNextWidgetName()
		screen.addLeaderheadGFC(szLeaderWidget, player.getLeaderType(), AttitudeTypes.ATTITUDE_PLEASED,
			self.X_LEADER_ICON, self.Y_LEADER_ICON, self.W_LEADER_ICON, self.H_LEADER_ICON, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# Leader Name
		self.szLeaderNameWidget = self.getNextWidgetName()
		szText = u"<font=4b>" + gc.getPlayer(self.iActivePlayer).getName() + u"</font>"
		screen.setText(self.szLeaderNameWidget, "", szText, CvUtil.FONT_LEFT_JUSTIFY,
				   self.X_LEADER_NAME, self.Y_LEADER_NAME, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# Create Table
		szTopChart = self.getNextWidgetName()
		screen.addTableControlGFC(szTopChart, self.iNumTopChartCols, self.X_STATS_TOP_CHART, self.Y_STATS_TOP_CHART, self.W_STATS_TOP_CHART, self.H_STATS_TOP_CHART,
					  False, True, 32,32, TableStyles.TABLE_STYLE_STANDARD)

		# Add Columns
		screen.setTableColumnHeader(szTopChart, 0, "", self.STATS_TOP_CHART_W_COL_0)
		screen.setTableColumnHeader(szTopChart, 1, "", self.STATS_TOP_CHART_W_COL_1)

		# Add Rows
		# for i in range(self.iNumTopChartRows - 1):
			# screen.appendTableRow(szTopChart)
		# iNumRows = screen.getTableNumRows(szTopChart)

		# Graph itself
		iRow = 0

		screen.appendTableRow(szTopChart)
		iCol = 0
		screen.setTableText(szTopChart, iCol, iRow, self.TEXT_TIME_PLAYED, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		iCol = 1
		screen.setTableText(szTopChart, iCol, iRow, szTimeString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		# K-Mod.
		iNumCitiesCurrent = gc.getPlayer(self.iActivePlayer).getNumCities()
		# advc.004: No longer optional
		if iNumCitiesCurrent != 0:# or not AdvisorOpt.isNonZeroStatsOnly():
			iRow += 1
			screen.appendTableRow(szTopChart)
			iCol = 0
			screen.setTableText(szTopChart, iCol, iRow, self.TEXT_CITIES_CURRENT, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			iCol = 1
			screen.setTableText(szTopChart, iCol, iRow, str(iNumCitiesCurrent), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# K-Mod end (note. I've also changed some of the structure of the surrounding code...)
		# advc.004:
		if iNumCitiesBuilt != 0:# or not AdvisorOpt.isNonZeroStatsOnly():
			iRow += 1
			screen.appendTableRow(szTopChart)
			iCol = 0
			screen.setTableText(szTopChart, iCol, iRow, self.TEXT_CITIES_BUILT, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			iCol = 1
			screen.setTableText(szTopChart, iCol, iRow, str(iNumCitiesBuilt), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# advc.004:
		if iNumCitiesRazed != 0:# or not AdvisorOpt.isNonZeroStatsOnly():
			iRow += 1
			screen.appendTableRow(szTopChart)
			iCol = 0
			screen.setTableText(szTopChart, iCol, iRow, self.TEXT_CITIES_RAZED, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			iCol = 1
			screen.setTableText(szTopChart, iCol, iRow, str(iNumCitiesRazed), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# advc.004:
		if iNumReligionsFounded != 0:# or not AdvisorOpt.isNonZeroStatsOnly():
			iRow += 1
			screen.appendTableRow(szTopChart)
			iCol = 0
			screen.setTableText(szTopChart, iCol, iRow, self.TEXT_NUM_RELIGIONS_FOUNDED, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			iCol = 1
			screen.setTableText(szTopChart, iCol, iRow, str(iNumReligionsFounded), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		# K-Mod.
		iNumGoldenAges = CyStatistics().getPlayerNumGoldenAges(self.iActivePlayer)
		# advc.004:
		if iNumGoldenAges != 0:# or not AdvisorOpt.isNonZeroStatsOnly():
			iRow += 1
			screen.appendTableRow(szTopChart)
			iCol = 0
			screen.setTableText(szTopChart, iCol, iRow, self.TEXT_NUM_GOLDEN_AGES, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			iCol = 1
			screen.setTableText(szTopChart, iCol, iRow, str(iNumGoldenAges), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# K-Mod end

################################################### BOTTOM PANEL ###################################################

		# Create Tables
		szUnitsTable = self.getNextWidgetName()
		screen.addTableControlGFC(szUnitsTable, self.iNumUnitStatsChartCols, self.X_STATS_BOTTOM_CHART, self.Y_STATS_BOTTOM_CHART, self.W_STATS_BOTTOM_CHART_UNITS, self.H_STATS_BOTTOM_CHART,
					  True, True, 32,32, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSort(szUnitsTable)

		szBuildingsTable = self.getNextWidgetName()
		screen.addTableControlGFC(szBuildingsTable, self.iNumBuildingStatsChartCols, self.X_STATS_BOTTOM_CHART + self.W_STATS_BOTTOM_CHART_UNITS, self.Y_STATS_BOTTOM_CHART, self.W_STATS_BOTTOM_CHART_BUILDINGS, self.H_STATS_BOTTOM_CHART,
					  True, True, 32,32, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSort(szBuildingsTable)

#BUG: improvements - start
		if AdvisorOpt.isShowImprovements():
			szImprovementsTable = self.getNextWidgetName()
			screen.addTableControlGFC(szImprovementsTable, self.iNumImprovementStatsChartCols, self.X_STATS_BOTTOM_CHART + self.W_STATS_BOTTOM_CHART_UNITS + self.W_STATS_BOTTOM_CHART_BUILDINGS, self.Y_STATS_BOTTOM_CHART, self.W_STATS_BOTTOM_CHART_IMPROVEMENTS, self.H_STATS_BOTTOM_CHART,
						  True, True, 32,32, TableStyles.TABLE_STYLE_STANDARD)
			screen.enableSort(szImprovementsTable)
#BUG: improvements - end

		# Reducing the width a bit to leave room for the vertical scrollbar, preventing a horizontal scrollbar from also being created
#BUG: improvements - start
		if AdvisorOpt.isShowImprovements():
			iChartWidth = self.W_STATS_BOTTOM_CHART_UNITS + self.W_STATS_BOTTOM_CHART_BUILDINGS + self.W_STATS_BOTTOM_CHART_IMPROVEMENTS - 24

			# Add Columns
			iColWidth = int((iChartWidth / 16 * 3))
			screen.setTableColumnHeader(szUnitsTable, 0, self.TEXT_UNITS, iColWidth)
			iColWidth = int((iChartWidth / 14 * 1))
			screen.setTableColumnHeader(szUnitsTable, 1, self.TEXT_CURRENT, iColWidth)
			iColWidth = int((iChartWidth / 14 * 1))
			screen.setTableColumnHeader(szUnitsTable, 2, self.TEXT_BUILT, iColWidth)
			iColWidth = int((iChartWidth / 14 * 1))
			screen.setTableColumnHeader(szUnitsTable, 3, self.TEXT_KILLED, iColWidth)
			iColWidth = int((iChartWidth / 14 * 1))
			screen.setTableColumnHeader(szUnitsTable, 4, self.TEXT_LOST, iColWidth)
			iColWidth = int((iChartWidth / 16 * 3))
			screen.setTableColumnHeader(szBuildingsTable, 0, self.TEXT_BUILDINGS, iColWidth)
			iColWidth = int((iChartWidth / 14 * 1))
			screen.setTableColumnHeader(szBuildingsTable, 1, self.TEXT_BUILT, iColWidth)
			iColWidth = int((iChartWidth / 14 * 2))
			screen.setTableColumnHeader(szImprovementsTable, 0, self.TEXT_IMPROVEMENTS, iColWidth)
			iColWidth = int((iChartWidth / 14 * 1))
			screen.setTableColumnHeader(szImprovementsTable, 1, self.TEXT_CURRENT, iColWidth)
		else:
			iChartWidth = self.W_STATS_BOTTOM_CHART_UNITS + self.W_STATS_BOTTOM_CHART_BUILDINGS - 24

			# Add Columns
			iColWidth = int((iChartWidth / 12 * 3))
			screen.setTableColumnHeader(szUnitsTable, 0, self.TEXT_UNITS, iColWidth)
			iColWidth = int((iChartWidth / 12 * 1))
			screen.setTableColumnHeader(szUnitsTable, 1, self.TEXT_CURRENT, iColWidth)
			iColWidth = int((iChartWidth / 12 * 1))
			screen.setTableColumnHeader(szUnitsTable, 2, self.TEXT_BUILT, iColWidth)
			iColWidth = int((iChartWidth / 12 * 1))
			screen.setTableColumnHeader(szUnitsTable, 3, self.TEXT_KILLED, iColWidth)
			iColWidth = int((iChartWidth / 12 * 1))
			screen.setTableColumnHeader(szUnitsTable, 4, self.TEXT_LOST, iColWidth)
			iColWidth = int((iChartWidth / 12 * 4))
			screen.setTableColumnHeader(szBuildingsTable, 0, self.TEXT_BUILDINGS, iColWidth)
			iColWidth = int((iChartWidth / 12 * 1))
			screen.setTableColumnHeader(szBuildingsTable, 1, self.TEXT_BUILT, iColWidth)
#BUG: improvements - end

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
			# for i in range(self.iNumImprovementStatsChartRows):
				# if (aiImprovementsCurrent[i] > 0):
					# screen.appendTableRow(szImprovementsTable)
			# iNumImprovementRows = screen.getTableNumRows(szImprovementsTable)
# #BUG: improvements - end

		# Add Units to table
		iRow = 0 # K-Mod
		for iUnitLoop in range(iNumUnits):
			# K-Mod. Hide zero rows option.
			if ((True or AdvisorOpt.isNonZeroStatsOnly()) # advc.004
					and aiUnitsCurrent[iUnitLoop] == 0 and aiUnitsBuilt[iUnitLoop] == 0
					and aiUnitsKilled[iUnitLoop] == 0 and aiUnitsLost[iUnitLoop] == 0):
				continue
			screen.appendTableRow(szUnitsTable)
			# K-Mod end
			#iRow = iUnitLoop

			iCol = 0
			szUnitName = gc.getUnitInfo(iUnitLoop).getDescription()
			screen.setTableText(szUnitsTable, iCol, iRow, szUnitName, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			iCol = 1
			iNumUnitsCurrent = aiUnitsCurrent[iUnitLoop]
			screen.setTableInt(szUnitsTable, iCol, iRow, str(iNumUnitsCurrent), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			iCol = 2
			iNumUnitsBuilt = aiUnitsBuilt[iUnitLoop]
			screen.setTableInt(szUnitsTable, iCol, iRow, str(iNumUnitsBuilt), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			iCol = 3
			iNumUnitsKilled = aiUnitsKilled[iUnitLoop]
			screen.setTableInt(szUnitsTable, iCol, iRow, str(iNumUnitsKilled), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			iCol = 4
			iNumUnitsLost = aiUnitsLost[iUnitLoop]
			screen.setTableInt(szUnitsTable, iCol, iRow, str(iNumUnitsLost), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			iRow += 1 # K-Mod

		# Add Buildings to table
		iRow = 0 # K-Mod
		for iBuildingLoop in range(iNumBuildings):
			# K-Mod. Hide zero rows option.
			# advc.004:
			if aiBuildingsBuilt[iBuildingLoop] == 0: #and AdvisorOpt.isNonZeroStatsOnly():
				continue
			screen.appendTableRow(szBuildingsTable)
			# K-Mod end
			#iRow = iBuildingLoop

			iCol = 0
			szBuildingName = gc.getBuildingInfo(iBuildingLoop).getDescription()
			screen.setTableText(szBuildingsTable, iCol, iRow, szBuildingName, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			iCol = 1
			iNumBuildingsBuilt = aiBuildingsBuilt[iBuildingLoop]
			screen.setTableInt(szBuildingsTable, iCol, iRow, str(iNumBuildingsBuilt), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			iRow += 1 # K-Mod

#BUG: improvements - start
		if AdvisorOpt.isShowImprovements():
			# Add Improvements to table	
			iRow = 0

			for iImprovementLoop in range(iNumImprovements):
				iNumImprovementsCurrent = aiImprovementsCurrent[iImprovementLoop]
				if (iNumImprovementsCurrent > 0):
					screen.appendTableRow(szImprovementsTable) # K-Mod
					iCol = 0
					szImprovementName = gc.getImprovementInfo(iImprovementLoop).getDescription()
					screen.setTableText(szImprovementsTable, iCol, iRow, szImprovementName, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					iCol = 1
					screen.setTableInt(szImprovementsTable, iCol, iRow, str(iNumImprovementsCurrent), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					iRow += 1
#BUG: improvements - end

#############################################################################################################
##################################################### OTHER #################################################
#############################################################################################################

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
				iIndex = screen.getSelectedPullDownID(self.DEBUG_DROPDOWN_ID)
				self.iActivePlayer = screen.getPullDownData(self.DEBUG_DROPDOWN_ID, iIndex)
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

############################### WONDERS / TOP CITIES TAB ###############################

			if (self.iActiveTab == self.iTopCitiesID):

				# Wonder type dropdown box
				if (szWidgetName == self.szWondersDropdownWidget
				or szShortWidgetName == self.BUGWorldWonderWidget
				or szShortWidgetName == self.BUGNatWonderWidget
				or szShortWidgetName == self.BUGProjectWidget):

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
					if (inputClass.getMouseX() == 0):
						screen.hideScreen()
						pPlayer = gc.getPlayer(inputClass.getData1())
						pCity = pPlayer.getCity(inputClass.getData2())
						CyCamera().JustLookAtPlot(pCity.plot())



################################## GRAPH TAB ###################################

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

			# (K-Mod version)
			if (inputClass.getButtonType() == WidgetTypes.WIDGET_GENERAL):
				iData1 = inputClass.getData1()
				if (iData1 != self.iActiveTab and iData1 >= 0 and iData1 < len(self.PAGE_NAME_LIST) and szWidgetName == "InfoTabButton"+str(iData1)):
					self.iActiveTab = iData1
					self.reset()
					self.redrawContents()

			# Wonder type dropdown box
			if self.iActiveTab == self.iTopCitiesID: # K-Mod
				if (szShortWidgetName == self.BUGWorldWonderWidget
				or szShortWidgetName == self.BUGNatWonderWidget
				or szShortWidgetName == self.BUGProjectWidget):
					self.handleInput_Wonders(inputClass)

#BUG: Change Graphs - start
			if AdvisorOpt.isGraphs() and self.iActiveTab == self.iGraphID:
				for i in range(7):
					if (szWidgetName == self.sGraphTextHeadingWidget[i]
					or (szWidgetName == self.sGraphBGWidget[i] and code == NotifyCode.NOTIFY_CLICKED)):
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
		if (self.iNumWonders > 0
		and not AdvisorOpt.isShowInfoWonders()):
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
