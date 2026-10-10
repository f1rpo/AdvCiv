## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#

# Thanks to Requies and Elhoim from CivFanatics for this interface mod

# This file has been edited for K-Mod in various places. Some changes marked, others not.

# <!-- custom: canonical Foreign Trade advisor implementation (Treaties/Bonuses/Techs/Cities).
# Diplomacy/Intel tabs are split to CvForeignDiplomacyAdvisor shell.
# This file replaces the old interdependent two-file setup ("CvForeignAdvisor.py" + "CvExoticForeignAdvisor.py"), which was inconsistent with other screen structure.
# For contrast, true alternatives are independent variants (e.g. "CvBUGMilitaryAdvisor.py" / "CvMilitaryAdvisor.py"), while major screens like "CvMainInterface.py" and "CvInfoScreen.py" are lengthy but unified files. (GPT-5.3-Codex) -->

from CvPythonExtensions import *
import CvUtil
import ScreenInput
import CvScreenEnums
import math
from SASUtils import *
from SASFontUtils import *
# <!-- custom: AdvCiv-SAS readability pass: use LABEL as the base advisor text tag (instead of BODY) for clearer upscaled UI text. (GPT-5.3-Codex) -->

############################################
### BEGIN CHANGES ENHANCED INTERFACE MOD ###
############################################
import IconGrid_BUG
#from IconGrid_BUG import IconGrid_BUG
##########################################
### END CHANGES ENHANCED INTERFACE MOD ###
##########################################

import DomPyHelpers
from PyHelpers import PyPlayer as EspionagePyPlayer
import TechTree
import AttitudeUtil
import BugCore
import BugDll
import BugUtil
import DealUtil
import DiplomacyUtil
import FavoriteCivicDetector
import FontUtil
import TradeUtil
from SASTextScale import labelText, appendListBoxStringLabel, setTableTextLabel

# globals
gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

PyPlayer = DomPyHelpers.DomPyPlayer
PyCity = DomPyHelpers.DomPyCity

AdvisorOpt = BugCore.game.Advisors

NUM_FOREIGN_SCREENS = 8

# tech trade columns
(iTechColLeader, iTechColStatus, iTechColWants, iTechColCantYou, iTechColResearch, iTechColGold, iTechColWill, iTechColWont, iTechColCantThem,) = range(9)
# <advc.ctr>
(iCityColLeader, iCityColStatus, iCityColWants, iCityColRejects, iCityColWillCede, iCityColWontCede,) = range(6) # </advc.ctr>

# Debugging help
def ForeignAdvisorPrint (stuff):
	stuff = "ForeignAdv: " + stuff
	BugUtil.debug(stuff)

class CvForeignAdvisor:
	"Foreign Advisor Screen"

	def __init__(self):
		self.iScreen = -1
		self.nWidgetCount = 0
		self.nLineCount = 0
		self.WIDGET_ID = "ForeignAdvisorWidget"
		self.LINE_ID = "ForeignAdvisorLine"
		self.SCREEN_NAME = "ForeignTradeAdvisor"
		self.DEBUG_DROPDOWN_ID =  "ForeignAdvisorDropdownWidget"
		self.EXIT_ID = "ForeignAdvisorExitWidget"
		self.BACKGROUND_ID = "ForeignAdvisorBackground"

		# <!-- custom: these are screen-independent edge constants (safe in __init__); runtime geometry that depends on actual resolution is computed in interfaceScreen.
		# Constants are shared through SASUtils so advisor screens use one source of truth. (GPT-5.3-Codex) -->
		self.W_LEFT_SPACE_FOR_COMMERCE_SLIDERS = SAS_ADVISOR_LEFT_SPACE_FOR_COMMERCE_SLIDERS
		self.W_RIGHT_SPACE_FOR_SCOREBOARD = SAS_ADVISOR_RIGHT_SPACE_FOR_SCOREBOARD
		self.H_TOP_SPACE_FOR_TECH_BAR = SAS_ADVISOR_TOP_SPACE_FOR_TECH_BAR
		self.H_BOTTOM_SPACE = SAS_ADVISOR_BOTTOM_SPACE
		self.Y_TITLE = SAS_ADVISOR_TITLE_Y
		self.X_LEADER = 80
		self.Y_LEADER = 115
		self.H_LEADER = 64
		self.W_LEADER = 64
		self.X_LINK = 50
		self.DX_LINK = 220
		self.H_LEGEND = 180
		self.W_LEGEND = 160
		self.MARGIN_LEGEND = 10
		self.X_LEGEND = 20
		self.Y_LEADER_CIRCLE_TOP = 87
		self.RADIUS_LEADER_ARC = 480
		self.LINE_WIDTH = 6
		self.BUTTON_SIZE = 64
		self.iSelectedLeader = -1
		self.iActiveLeader = -1
		self.listSelectedLeaders = []
		self.iShiftKeyDown = 0

		self.GLANCE_HEADER = "ForeignAdvisorGlanceHeader"
		self.GLANCE_BUTTON = "ForeignAdvisorPlusMinus"
		self.X_LINK = 0

		# <!-- custom: remove these iExtraWidth and iExtraHeight-like as we don't want yellow margins: they are distracting and not not useful; channge with the help of gemini pro 3 thanks. -->
		# self.X_GLANCE_OFFSET = 6 # advc.004: was 10
		# self.Y_GLANCE_OFFSET = 3
		self.X_GLANCE_OFFSET = 0
		self.Y_GLANCE_OFFSET = 0
		self.GLANCE_BUTTON_SIZE = 46
		self.PLUS_MINUS_SIZE = 25
		self.bGlancePlus = True
		# <!-- custom: add glance tab column pagination because with 24+ players the column icons overflow the screen width while vertical scrolling still works fine; we use arrow navigation rather than switching to a table widget, which would lose the per-cell widget type interactions (attitude popups, war-trade clicks). See KI#134. (Claude Code Sonnet 4.6+ Claude code Opus 4.7) -->
		self.iGlancePage = 0
		self.iGlanceNumPages = 1
		self.ltGlanceColumns = []
		self.glanceLeftArrowID = ""
		self.glanceRightArrowID = ""
		self.GLANCE_MAX_PLAYERS_PER_PAGE = None

		self.INFO_BORDER = 10

		############################################
		### BEGIN CHANGES ENHANCED INTERFACE MOD ###
		############################################

		###################
		# General options #
		###################

		# Show the names of the leaders if 'True'
		self.SHOW_LEADER_NAMES = False

		# Show a border around the rows
		self.SHOW_ROW_BORDERS = True

		# Minimum space at the top and bottom of the screen.
		# <!-- custom: after our changes, blue panel is overfilling on top and bottom, adjusted with the help of gemini 3 pro thanks -->
		# The blue panel is overflowing because MIN_TOP_BOTTOM_SPACE is set to 30, but the Civ4 top/bottom UI bars are 55 pixels tall. We need to increase the margin so the panel clears those bars.
		# <!-- custom: update: with 55 we have some yellow uneeded margins a slightly lower value seems to work much better at removing these, but a too low one creates an unwanted display somehow, adjusted as such based on a similar suggestion of gemini 3 pro thanks -->
		# self.MIN_TOP_BOTTOM_SPACE = 30 # advc.073: was 60
		self.MIN_TOP_BOTTOM_SPACE = 45

		# Minimum space at the left and right end of the screen.
		# <!-- custom: reduce this as we don't need so much space on the sides, and we need the space to show more info as per gemini 3 pro's solution thanks -->
		# self.MIN_LEFT_RIGHT_SPACE = 25
		self.MIN_LEFT_RIGHT_SPACE = 0

		# Extra border at the left and right ends of the column groups (import/export)
		self.GROUP_BORDER = 8

		# Extra space before the label of the column groups (import/export)
		self.GROUP_LABEL_OFFSET = "   "

		# Minimum space between the columns
		self.MIN_COLUMN_SPACE = 5

		# Minimum space between the rows
		self.MIN_ROW_SPACE = 1

		##########################
		# Resources view options #
		##########################

		# If 'True', the amount for each surplus resource is subtracted by one. So it shows how many you
		# can give away without losing the resource yourself. This value isn't affected by any default 
		# layout.
		self.RES_SHOW_EXTRA_AMOUNT = False # advc.073: was True

		# If 'True', the amount's are shown as an overlay on top of the lower left corner of the resources.
		# If 'False', the amount's are shown below the resources so you'll need to use a higher value for 
		# self.RES_SURPLUS_HEIGHT (see below).
		# advc.073 (comment): I doubt that False will work correctly with the changes I've made
		self.RES_SHOW_SURPLUS_AMOUNT_ON_TOP = True

		# If 'True', the resource columns are grouped as import and export.
		self.RES_SHOW_IMPORT_EXPORT_HEADER = True

		# If 'True', two extra columns are used to display resources that are traded in active deals.
		self.RES_SHOW_ACTIVE_TRADE = True

		# Height of the panel showing the surplus resources. If self.RES_SHOW_SURPLUS_AMOUNT_ON_TOP is 'False'
		# you'll need to set a higher value for this variable (110 is recommended).
		# <!-- custom: it seems to me just one row of bonuses is enough, to begin with we don't have that much in our mod and not so much are tradeable at the same time anyway (plus we removed the header as well so we don't need as much space), so reduce this -->
		# self.RES_SURPLUS_HEIGHT = 110 # advc.073: was 80
		self.RES_SURPLUS_HEIGHT = 60

		self.RES_GOLD_COL_WIDTH = 25

		# Space between the two panels.
		self.RES_PANEL_SPACE = 0

		#############################
		# Technologies view options #
		#############################

		# If 'True', use icon size 32x32
		# If 'False', use icon size 64x64
		self.TECH_USE_SMALL_ICONS = True

		self.TECH_STATUS_COL_WIDTH = 40
		self.TECH_GOLD_COL_WIDTH = 60

		###############
		# End options #
		###############

		self.TITLE_HEIGHT = 24
		self.TABLE_CONTROL_HEIGHT = 24
		self.RESOURCE_ICON_SIZE = 34
		self.SCROLL_TABLE_UP = 1
		self.SCROLL_TABLE_DOWN = 2

		##########################################
		### END CHANGES ENHANCED INTERFACE MOD ###
		##########################################

		self.SCREEN_DICT = {
			"BONUS": 0,
			"TECH": 1,
			"RELATIONS": 2,
			"ACTIVE_TRADE": 3,
			"INFO": 4,
			"GLANCE": 5,
			"CITIES": 6, # advc.ctr
			"ESPIONAGE": 7,
			}

		self.REV_SCREEN_DICT = {}

		for key, value in self.SCREEN_DICT.items():
			self.REV_SCREEN_DICT[value] = key

		self.DRAW_DICT = {
			"BONUS": self.drawResourceDeals,
			"TECH": self.drawTechDeals,
			"RELATIONS": self.drawRelations,
			"ACTIVE_TRADE": self.drawActive,
			"INFO": self.drawInfo,
			"GLANCE": self.drawGlance,
			"CITIES": self.drawCityDeals, # advc.ctr
			"ESPIONAGE": self.drawEspionage,
			}

		self.TXT_KEY_DICT = {
			"BONUS": "TXT_KEY_FOREIGN_ADVISOR_RESOURCES",
			"TECH": "TXT_KEY_FOREIGN_ADVISOR_TECHS",
			"RELATIONS": "TXT_KEY_FOREIGN_ADVISOR_RELATIONS",
			# <!-- custom: renamed Active tab label to Treaties to match the reduced scope (no bonus clutter). (GPT-5.3-Codex) -->
			"ACTIVE_TRADE": "TXT_KEY_FOREIGN_ADVISOR_TREATIES",
			"INFO": "TXT_KEY_FOREIGN_ADVISOR_INFO",
			"ESPIONAGE": "TXT_KEY_SAS_ESPIONAGE_SHORT",
			"GLANCE": "TXT_KEY_FOREIGN_ADVISOR_GLANCE",
			"CITIES": "TXT_KEY_CONCEPT_CITIES", # advc.ctr
			}

		# <!-- custom: keep the two Foreign-advisor tab clusters explicit: F4 uses trade tabs, while CvForeignDiplomacyAdvisor selects the diplomacy/intel tabs. (GPT-5.5) -->
		self.TRADE_TAB_KEYS = ["ACTIVE_TRADE", "BONUS", "TECH", "CITIES"]
		self.DIPLOMACY_TAB_KEYS = ["RELATIONS", "GLANCE", "INFO", "ESPIONAGE"]
		self.FOREIGN_TAB_KEYS = self.TRADE_TAB_KEYS

		# K-Mod
		self.LABEL_WIDTH_LIST = []
		self.iLanguageLoaded = -1
		# K-Mod end

		# <!-- custom: shared size for treaty icons rendered inline in active-deals text. (GPT-5.3-Codex) -->
		self.SAS_TREATY_ICON_SIZE = 24
		# <!-- custom: Espionage tab persistent state/constants; runtime geometry is computed in interfaceScreen from actual resolution. (GPT-5.3-Codex) -->
		self.ESP_DEBUG_DROPDOWN_ID =  "EspionageAdvisorDropdownWidget"
		self.ESP_WIDGET_ID = "EspionageAdvisorWidget"
		self.ESP_BORDER_WIDTH = 4
		self.ESP_PANE_HEIGHT = 450
		self.ESP_PANE_WIDTH = 283
		self.ESP_X_SLIDERS = 50
		self.ESP_X_INCOME = 373
		self.ESP_X_EXPENSES = 696
		self.ESP_Y_TREASURY = 90
		self.ESP_H_TREASURY = 100
		self.ESP_Y_LOCATION = 230
		self.ESP_Y_SPACING = 30
		self.ESP_TEXT_MARGIN = 15
		self.ESP_Z_BACKGROUND = -2.1
		self.ESP_Z_CONTROLS = self.ESP_Z_BACKGROUND - 0.2
		# <!-- custom: placeAdvisorLegendLink expects the shared advisor Z_CONTROLS name; Foreign Advisor's Espionage tab uses ESP_Z_CONTROLS internally, so mirror it here instead of changing the shared helper. (GPT-5.5-Thinking + GPT-5.5?) -->
		self.Z_CONTROLS = self.ESP_Z_CONTROLS
		self.ESP_DZ = -0.2
		# <!-- custom: Espionage tab fixed layout constants belong in init; only resolution-derived geometry is computed at screen open. (GPT-5.3-Codex) -->
		self.ESP_OUTER_MARGIN = 0
		self.ESP_PANEL_GAP = 0
		self.ESP_INNER_MARGIN = 12
		self.ESP_MAIN_BLEED_X = 4
		self.ESP_MAIN_BLEED_Y = 12
		self.ESP_Y_MAIN_PANE_BASE = 54
		self.ESP_LEFT_PANE_WIDTH_PERCENT = 38
		self.ESP_LEFT_PANE_WIDTH_MIN = 380
		self.ESP_COMPACT_LEFT_PANE_EXTRA = 200
		self.ESP_COMPACT_RIGHT_PANE_WIDTH_MIN = 620
		self.ESP_LEFT_PANE_HEIGHT_MIN = 420
		self.ESP_SCROLL_WIDTH_MIN = 220
		self.ESP_SCROLL_HEIGHT_MIN = 220
		self.ESP_TOTAL_PANE_WIDTH_MIN = 320
		self.ESP_CITY_LIST_TOP_OFFSET = 45
		self.ESP_CITY_LIST_WIDTH_PERCENT = 21
		self.ESP_CITY_LIST_WIDTH_MIN = 110
		self.ESP_CITY_LIST_HEIGHT_MIN = 150
		self.ESP_EFFECTS_COSTS_WIDTH_PERCENT = 9
		self.ESP_EFFECTS_COSTS_WIDTH_MIN = 52
		self.ESP_EFFECTS_WIDTH_MIN = 140
		self.ESP_EFFECTS_COSTS_GAP = 10
		self.ESP_EFFECTS_HEIGHT_MIN = 70
		self.ESP_EFFECTS_HEIGHT_DIVISOR = 3
		self.ESP_EFFECTS_HEIGHT_OFFSET = 50
		self.ESP_MISSIONS_TOP_GAP = 50
		self.ESP_MISSIONS_HEIGHT_MIN = 90
		self.ESP_MISSIONS_HEIGHT_NUMERATOR = 2
		self.ESP_MISSIONS_HEIGHT_DENOMINATOR = 3
		self.ESP_MISSION_BUTTON_TOP_GAP = 10
		self.ESP_MISSION_BUTTON_HEIGHT = 30
		self.ESP_ROW_LEADER_ICON_SIZE = 44
		self.ESP_ROW_X_LEADER_ICON = 8
		self.ESP_ROW_X_NAME = 58
		self.ESP_ROW_X_WEIGHT = 98
		self.ESP_ROW_X_RIGHT_MIN = 205
		self.ESP_ROW_X_RIGHT_OFFSET = 120
		self.ESP_ROW_Y_TOP = -15
		self.ESP_ROW_Y_BOTTOM = 2
		self.ESP_ROW_Y_WEIGHT = 9
		self.ESP_ROW_Y_BUTTON = 16
		self.ESP_ROW_Y_LEADER_ICON = 0
		self.ESP_ROW_X_BUTTON_PLUS = 58
		self.ESP_ROW_X_BUTTON_MINUS = 76
		# <!-- custom: With many met players, the old two-line Espionage target rows made +/- buttons, weight, EPs, and +per-turn text hard to see/click.
		# Use compact single-line rows instead of pagination so all targets remain in one scroll list.
		# Both layouts omit the repeated espionage commerce glyph; standard rows use that space for a larger leader portrait and move +/- beside it, while compact rows enlarge the portrait within the existing row.
		# Compact rows also omit the per-row "Weight:" and cost labels because their meaning is clear from context; the wider column anchors keep weight, EPs, and per-turn text separated. (GPT-5.5-Thinking + GPT-5.5? + GPT-5.5) -->
		self.SAS_CV_FOREIGN_ADVISOR_ESPIONAGE_COMPACT_ROWS_THRESHOLD = None
		self.ESP_COMPACT_LEADER_ICON_SIZE = 28
		self.ESP_COMPACT_BUTTON_SIZE = 20
		self.ESP_COMPACT_ROW_X_ICON = 4
		self.ESP_COMPACT_ROW_X_BUTTON_PLUS = 34
		self.ESP_COMPACT_ROW_X_BUTTON_MINUS = 56
		self.ESP_COMPACT_ROW_X_NAME = 82
		self.ESP_COMPACT_ROW_X_WEIGHT = 340
		self.ESP_COMPACT_ROW_X_POINTS = 490
		self.ESP_COMPACT_ROW_X_AMOUNT = 690
		self.ESP_COMPACT_ROW_RIGHT_MARGIN = 6
		self.ESP_COMPACT_ROW_Y_TEXT = 3
		self.ESP_COMPACT_ROW_Y_BUTTON = 5
		self.nEspionageWidgetCount = 0
		self.ESP_iTargetPlayer = -1
		self.ESP_iActiveCityID = -1
		self.ESP_iSelectedMission = -1
		# <!-- custom: cached vanilla engine defines. (Claude code Sonnet 4.6) -->
		self.iCOMMERCE_PERCENT_CHANGE_INCREMENTS = None
		self.szLEADERHEAD_RANDOM = None

	# <!-- custom: initialize text and icon caches once for performance, based on Info Screen and Victory Screen pattern (claude code sonnet 4.5) -->
	def initText(self):
		# only execute this function once per language...
		if self.iLanguageLoaded == CyGame().getCurrentLanguage() or not CyGame().isFinalInitialized():
			return
		self.iLanguageLoaded = CyGame().getCurrentLanguage()

		# <!-- custom: precompute commonly used text strings to avoid repeated lookups (claude code sonnet 4.5) -->
		self.EXIT_TEXT = sasFontTagTitle + localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT", ()).upper() + SAS_FONT_TAG_CLOSE
		self.SCREEN_TITLE = sasFontTagTitle.bold + localText.getText("TXT_KEY_FOREIGN_TRADE_ADVISOR_TITLE", ()).upper() + SAS_FONT_TAG_CLOSE

		# <!-- custom: precompute tab/column header texts (claude code sonnet 4.5) -->
		self.TEXT_LEADER = localText.getText("TXT_KEY_FOREIGN_ADVISOR_LEADER", ())
		self.TEXT_WILL_IMPORT = localText.getText("TXT_KEY_FOREIGN_ADVISOR_WILL_IMPORT", ())
		self.TEXT_WILL_EXPORT = localText.getText("TXT_KEY_FOREIGN_ADVISOR_WILL_EXPORT", ())
		self.TEXT_WILL_NOT_EXPORT = localText.getText("TXT_KEY_FOREIGN_ADVISOR_WILL_NOT_EXPORT", ())
		self.TEXT_NO_NEED = localText.getText("TXT_KEY_FOREIGN_ADVISOR_NO_NEED", ())
		self.TEXT_IMPORTING = localText.getText("TXT_KEY_FOREIGN_ADVISOR_IMPORTING", ())
		self.TEXT_EXPORTING = localText.getText("TXT_KEY_FOREIGN_ADVISOR_EXPORTING", ())

		# <!-- custom: tech table headers (claude code sonnet 4.5) -->
		self.TEXT_WANTS = localText.getText("TXT_KEY_FOREIGN_ADVISOR_WANTS", ())
		self.TEXT_CANT_RECEIVE = localText.getText("TXT_KEY_FOREIGN_ADVISOR_CANT_RECEIVE", ())
		self.TEXT_CAN_RESEARCH = localText.getText("TXT_KEY_FOREIGN_ADVISOR_CAN_RESEARCH", ())
		self.TEXT_FOR_TRADE = localText.getText("TXT_KEY_FOREIGN_ADVISOR_FOR_TRADE_2", ())
		self.TEXT_NOT_FOR_TRADE = localText.getText("TXT_KEY_FOREIGN_ADVISOR_NOT_FOR_TRADE_2", ())

		# <!-- custom: city table headers (claude code sonnet 4.5) -->
		self.TEXT_WILL_CEDE = localText.getText("TXT_KEY_FOREIGN_ADVISOR_WILL_CEDE", ())
		# <!-- custom: cache Espionage tab static texts in initText like other tabs. (GPT-5.3-Codex) -->
		self.TEXT_ESPIONAGE_PASSIVE_EFFECTS = localText.getText("TXT_KEY_ESPIONAGE_SCREEN_PASSIVE_EFFECTS", ())
		self.TEXT_ESPIONAGE_MISSIONS = localText.getText("TXT_KEY_ESPIONAGE_SCREEN_MISSIONS", ())
		self.TEXT_ESPIONAGE_COST = localText.getText("TXT_KEY_ESPIONAGE_SCREEN_COST", ())
		self.TEXT_ESPIONAGE_WEIGHT = localText.getText("TXT_KEY_ESPIONAGE_SCREEN_SPENDING_WEIGHT", ())
		self.TEXT_ESPIONAGE_PASSIVE_AUTOMATIC = localText.getText("TXT_KEY_ESPIONAGE_PASSIVE_AUTOMATIC", ())
		self.TEXT_ESPIONAGE_MISSIONS_SPY = localText.getText("TXT_KEY_ESPIONAGE_MISSIONS_SPY", ())
		self.TEXT_ESPIONAGE_NO_SPY = localText.getText("TXT_KEY_ESPIONAGE_NO_SPY", ())
		self.TEXT_MISC_THRESHOLD = localText.getText("TXT_KEY_MISC_THRESHOLD", ())
		self.TEXT_CONCEPT_CITIES = localText.getText("TXT_KEY_CONCEPT_CITIES", ())
		self.COLOR_WHITE = getInfoTypeOrFail("COLOR_WHITE")
		self.COLOR_RED = getInfoTypeOrFail("COLOR_RED")
		self.COLOR_BLUE = getInfoTypeOrFail("COLOR_BLUE")
		self.COLOR_CITY_GREEN = getInfoTypeOrFail("COLOR_CITY_GREEN")
		self.COLOR_YELLOW = getInfoTypeOrFail("COLOR_YELLOW")
		self.COLOR_CYAN = getInfoTypeOrFail("COLOR_CYAN")
		self.COLOR_LIGHT_GREY = getInfoTypeOrFail("COLOR_LIGHT_GREY")
		self.COLOR_GREEN = getInfoTypeOrFail("COLOR_GREEN")

		eDenialColor = self.COLOR_WHITE
		self.TEXT_REJECTS = localText.getColorText("TXT_KEY_FOREIGN_ADVISOR_REJECTS", (), eDenialColor)
		self.TEXT_WONT_CEDE = localText.getColorText("TXT_KEY_FOREIGN_ADVISOR_WONT_CEDE", (), eDenialColor)
		# <!-- custom: messages (claude code sonnet 4.5) -->
		self.TEXT_NOT_CONNECTED = localText.getText("TXT_KEY_FOREIGN_ADVISOR_NOT_CONNECTED", ())
		self.TEXT_NO_TECH_TRADING = localText.getText("TXT_KEY_FOREIGN_ADVISOR_NO_TECH_TRADING", ())
		self.TEXT_MORE_CITIES = localText.getText("TXT_KEY_FOREIGN_ADVISOR_MORE_CITIES", (0,))  # format later with actual count
		self.TEXT_FAVORITES = localText.getText("TXT_KEY_PEDIA_FAVORITES", ())
		self.TEXT_REL_CONTACT = localText.getText("TXT_KEY_FOREIGN_ADVISOR_CONTACT", ())
		self.TEXT_CONCEPT_WAR = localText.getText("TXT_KEY_CONCEPT_WAR", ())
		self.TEXT_TRADE_DEFENSIVE_PACT = localText.getText("TXT_KEY_TRADE_DEFENSIVE_PACT_STRING", ())
		self.TEXT_TRADE_OPEN_BORDERS = localText.getText("TXT_KEY_TRADE_OPEN_BORDERS_STRING", ())
		self.TEXT_PITBOSS_TEAM = localText.getText("TXT_KEY_PITBOSS_TEAM", ())
		self.TEXT_MISC_VASSAL_SHORT = localText.getText("TXT_KEY_MISC_VASSAL_SHORT", ())
		self.TEXT_MISC_MASTER = localText.getText("TXT_KEY_MISC_MASTER", ())
		self.LABEL_REL_CONTACT = sasFontTagLabel + self.TEXT_REL_CONTACT + SAS_FONT_TAG_CLOSE
		self.LABEL_CONCEPT_WAR = sasFontTagLabel + self.TEXT_CONCEPT_WAR + SAS_FONT_TAG_CLOSE
		self.LABEL_TRADE_DEFENSIVE_PACT = sasFontTagLabel + self.TEXT_TRADE_DEFENSIVE_PACT + SAS_FONT_TAG_CLOSE
		self.LABEL_TRADE_OPEN_BORDERS = sasFontTagLabel + self.TEXT_TRADE_OPEN_BORDERS + SAS_FONT_TAG_CLOSE
		self.LABEL_PITBOSS_TEAM = sasFontTagLabel + self.TEXT_PITBOSS_TEAM + SAS_FONT_TAG_CLOSE
		self.LABEL_MISC_VASSAL_SHORT = sasFontTagLabel + self.TEXT_MISC_VASSAL_SHORT + SAS_FONT_TAG_CLOSE

		# <!-- custom: precompute commonly used icon symbols (claude code sonnet 4.5) -->
		self.iReligionIcon = CyGame().getSymbolID(FontSymbols.RELIGION_CHAR)
		self.iTradeIcon = CyGame().getSymbolID(FontSymbols.TRADE_CHAR)
		self.iCitizenIcon = CyGame().getSymbolID(FontSymbols.CITIZEN_CHAR)
		self.iOccupationIcon = CyGame().getSymbolID(FontSymbols.OCCUPATION_CHAR)
		self.iMapIcon = CyGame().getSymbolID(FontSymbols.MAP_CHAR)
		self.iGoldIcon = gc.getCommerceInfo(CommerceTypes.COMMERCE_GOLD).getChar()

		# <!-- custom: precompute formatted icon strings (claude code sonnet 4.5) -->
		self.szReligionIconStr = u"%c" % self.iReligionIcon
		self.szTradeIconStr = u"%c" % self.iTradeIcon
		self.szTradeCommerceIconStr = u"%c%c" % (self.iTradeIcon, gc.getYieldInfo(YieldTypes.YIELD_COMMERCE).getChar())

		# <!-- custom: i don't know what this is for, but since we renamed it to as of now just "Favorites:" to include favorite religions as well, may as well remove this alternative one -->
		# if FavoriteCivicDetector.isDetectionNecessary():
		# 	fcHeaderText = BugUtil.getPlainText("TXT_KEY_FOREIGN_ADVISOR_POSSIBLE_FAV_CIVICS")
		# else:
		# 	fcHeaderText = BugUtil.getPlainText("TXT_KEY_PEDIA_FAV_CIVIC")
		#fcHeaderText = BugUtil.getPlainText("TXT_KEY_PEDIA_FAVORITES")

		# <!-- custom: perf opt: looks like this can be moved to init entirely if i'm not mistaken. -->
		# <!-- custom: szReligionIconStr/szTradeIconStr/szTradeCommerceIconStr are cached icon values for performance (claude code sonnet 4.5) -->
		self.headerTexts = (BugUtil.getPlainText("TXT_KEY_FOREIGN_ADVISOR_ABBR_LEADER"), BugUtil.getPlainText("TXT_KEY_FOREIGN_ADVISOR_ABBR_ATTITUDE"), self.szReligionIconStr, self.szTradeIconStr, self.szTradeCommerceIconStr, BugUtil.getPlainText("TXT_KEY_CIVICOPTION_ABBR_GOVERNMENT"), BugUtil.getPlainText("TXT_KEY_CIVICOPTION_ABBR_LEGAL"), BugUtil.getPlainText("TXT_KEY_CIVICOPTION_ABBR_LABOR"), BugUtil.getPlainText("TXT_KEY_CIVICOPTION_ABBR_ECONOMY"), BugUtil.getPlainText("TXT_KEY_CIVICOPTION_ABBR_RELIGION"), "", BugUtil.getPlainText("TXT_KEY_PEDIA_FAVORITES"))

		# <!-- custom: cache treaty icon paths once. (GPT-5.3-Codex) -->
		self.SAS_TRADE_ICON_OPEN_BORDERS = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_OPENBORDERS").getPath()
		self.SAS_TRADE_ICON_DEFENSIVE_PACT = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_DEFENSIVEPACT").getPath()
		self.SAS_TRADE_ICON_PERMANENT_ALLIANCE = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_PERMALLIANCE").getPath()
		self.SAS_TRADE_ICON_VASSAL = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_VASSAL").getPath()
		self.SAS_ART_SCREEN_BG_OPAQUE = ArtFileMgr.getInterfaceArtInfo("SCREEN_BG_OPAQUE").getPath()
		self.SAS_ART_BUTTON_HILITE_SQUARE = ArtFileMgr.getInterfaceArtInfo("BUTTON_HILITE_SQUARE").getPath()
		self.SAS_ART_WHITE_CIRCLE_40 = ArtFileMgr.getInterfaceArtInfo("WHITE_CIRCLE_40").getPath()
		self.SAS_ART_BUTTON_PLUS = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTONS_PLUS").getPath()
		self.SAS_ART_BUTTON_MINUS = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTONS_MINUS").getPath()
		self.TEXT_CANT_TRADE = localText.getText("TXT_KEY_FOREIGN_ADVISOR_CANT_TRADE", ())

	def updateRuntimeLayout(self, screen):
		# <!-- custom: runtime layout wrapper (same structure as CvInfoScreen) so resolution-dependent bounds/anchors are centralized. (GPT-5.3-Codex) -->
		self.X_SCREEN, self.Y_SCREEN, self.W_SCREEN, self.H_SCREEN = getAdvisorRuntimeBounds(screen, self.W_LEFT_SPACE_FOR_COMMERCE_SLIDERS, self.W_RIGHT_SPACE_FOR_SCOREBOARD, self.H_TOP_SPACE_FOR_TECH_BAR, self.H_BOTTOM_SPACE)
		self.X_TITLE, self.X_EXIT, self.Y_EXIT, self.Y_LINK, self.Y_BOTTOM_PANEL = getAdvisorRuntimeAnchors(self.W_SCREEN, self.H_SCREEN)
		self.Y_LEGEND = self.H_SCREEN - self.H_LEGEND - 75
		self.X_LEADER_CIRCLE_TOP = self.X_SCREEN + 10
		self.ESP_X_SCREEN = self.X_SCREEN
		self.ESP_Y_SCREEN = self.Y_SCREEN
		self.ESP_W_SCREEN = self.W_SCREEN
		self.ESP_H_SCREEN = self.H_SCREEN
		# <!-- custom: Espionage geometry is runtime-dependent (resolution + advisor bounds), so keep it in the shared runtime layout method. (GPT-5.3-Codex) -->
		self.ESP_X_MAIN_PANE = -self.ESP_MAIN_BLEED_X
		self.ESP_Y_MAIN_PANE = self.ESP_Y_MAIN_PANE_BASE - self.ESP_MAIN_BLEED_Y
		self.ESP_W_MAIN_PANE = self.ESP_W_SCREEN + (2 * self.ESP_MAIN_BLEED_X)
		self.ESP_H_MAIN_PANE = (self.Y_BOTTOM_PANEL - self.ESP_Y_MAIN_PANE) + self.ESP_MAIN_BLEED_Y
		self.ESP_X_LEFT_PANE = self.ESP_OUTER_MARGIN
		self.ESP_Y_LEFT_PANE = self.ESP_Y_MAIN_PANE + self.ESP_OUTER_MARGIN
		self.ESP_W_LEFT_PANE = max(self.ESP_LEFT_PANE_WIDTH_MIN, self.ESP_W_SCREEN * self.ESP_LEFT_PANE_WIDTH_PERCENT / 100)
		self.ESP_H_LEFT_PANE = max(self.ESP_LEFT_PANE_HEIGHT_MIN, self.ESP_H_MAIN_PANE - self.ESP_OUTER_MARGIN)
		self.ESP_X_SCROLL = self.ESP_X_LEFT_PANE + self.ESP_INNER_MARGIN
		self.ESP_Y_SCROLL = self.ESP_Y_LEFT_PANE + self.ESP_INNER_MARGIN
		self.ESP_W_SCROLL = max(self.ESP_SCROLL_WIDTH_MIN, self.ESP_W_LEFT_PANE - (2 * self.ESP_INNER_MARGIN))
		self.ESP_H_SCROLL = max(self.ESP_SCROLL_HEIGHT_MIN, self.ESP_H_LEFT_PANE - (2 * self.ESP_INNER_MARGIN))
		self.ESP_X_TOTAL_PANE = self.ESP_X_LEFT_PANE + self.ESP_W_LEFT_PANE + self.ESP_PANEL_GAP
		self.ESP_Y_TOTAL_PANE = self.ESP_Y_LEFT_PANE
		self.ESP_W_TOTAL_PANE = max(self.ESP_TOTAL_PANE_WIDTH_MIN, self.ESP_W_SCREEN - self.ESP_X_TOTAL_PANE - self.ESP_OUTER_MARGIN)
		self.ESP_X_RIGHT_PANE = self.ESP_X_TOTAL_PANE
		# <!-- custom: no separate top-summary pane in active layout; right pane starts at top and uses full height. (GPT-5.3-Codex) -->
		self.ESP_Y_RIGHT_PANE = self.ESP_Y_LEFT_PANE
		self.ESP_W_RIGHT_PANE = self.ESP_W_TOTAL_PANE
		self.ESP_H_RIGHT_PANE = self.ESP_H_LEFT_PANE
		self.ESP_X_CITY_LIST = self.ESP_X_RIGHT_PANE + self.ESP_INNER_MARGIN
		self.ESP_Y_CITY_LIST = self.ESP_Y_RIGHT_PANE + self.ESP_CITY_LIST_TOP_OFFSET
		self.ESP_W_CITY_LIST = max(self.ESP_CITY_LIST_WIDTH_MIN, self.ESP_W_RIGHT_PANE * self.ESP_CITY_LIST_WIDTH_PERCENT / 100)
		self.ESP_H_CITY_LIST = max(self.ESP_CITY_LIST_HEIGHT_MIN, self.ESP_H_RIGHT_PANE - (self.ESP_CITY_LIST_TOP_OFFSET + self.ESP_INNER_MARGIN))
		self.ESP_X_EFFECTS_LIST = self.ESP_X_CITY_LIST + self.ESP_W_CITY_LIST + self.ESP_INNER_MARGIN
		self.ESP_Y_EFFECTS_LIST = self.ESP_Y_CITY_LIST
		self.ESP_W_EFFECTS_COSTS_LIST = max(self.ESP_EFFECTS_COSTS_WIDTH_MIN, self.ESP_W_RIGHT_PANE * self.ESP_EFFECTS_COSTS_WIDTH_PERCENT / 100)
		self.ESP_W_EFFECTS_LIST = max(self.ESP_EFFECTS_WIDTH_MIN, self.ESP_W_RIGHT_PANE - (self.ESP_X_EFFECTS_LIST - self.ESP_X_RIGHT_PANE) - self.ESP_EFFECTS_COSTS_GAP - self.ESP_W_EFFECTS_COSTS_LIST - self.ESP_INNER_MARGIN)
		self.ESP_H_EFFECTS_LIST = max(self.ESP_EFFECTS_HEIGHT_MIN, (self.ESP_H_CITY_LIST / self.ESP_EFFECTS_HEIGHT_DIVISOR) - self.ESP_EFFECTS_HEIGHT_OFFSET)
		self.ESP_X_EFFECTS_COSTS_LIST = self.ESP_X_EFFECTS_LIST + self.ESP_W_EFFECTS_LIST + self.ESP_EFFECTS_COSTS_GAP
		self.ESP_Y_EFFECTS_COSTS_LIST = self.ESP_Y_EFFECTS_LIST
		self.ESP_H_EFFECTS_COSTS_LIST = self.ESP_H_EFFECTS_LIST
		self.ESP_X_MISSIONS_LIST = self.ESP_X_EFFECTS_LIST
		self.ESP_Y_MISSIONS_LIST = self.ESP_Y_EFFECTS_LIST + self.ESP_H_EFFECTS_LIST + self.ESP_MISSIONS_TOP_GAP
		self.ESP_W_MISSIONS_LIST = self.ESP_W_EFFECTS_LIST
		self.ESP_H_MISSIONS_LIST = max(self.ESP_MISSIONS_HEIGHT_MIN, self.ESP_H_CITY_LIST * self.ESP_MISSIONS_HEIGHT_NUMERATOR / self.ESP_MISSIONS_HEIGHT_DENOMINATOR)
		self.ESP_X_MISSIONS_COSTS_LIST = self.ESP_X_MISSIONS_LIST + self.ESP_W_MISSIONS_LIST + self.ESP_EFFECTS_COSTS_GAP
		self.ESP_Y_MISSIONS_COSTS_LIST = self.ESP_Y_MISSIONS_LIST
		self.ESP_W_MISSIONS_COSTS_LIST = self.ESP_W_EFFECTS_COSTS_LIST
		self.ESP_H_MISSIONS_COSTS_LIST = self.ESP_H_MISSIONS_LIST
		self.ESP_X_MISSION_BUTTON = self.ESP_X_MISSIONS_LIST
		self.ESP_Y_MISSION_BUTTON = self.ESP_Y_MISSIONS_LIST + self.ESP_H_MISSIONS_LIST + self.ESP_MISSION_BUTTON_TOP_GAP
		self.ESP_W_MISSION_BUTTON = self.ESP_W_MISSIONS_LIST + self.ESP_W_MISSIONS_COSTS_LIST + self.ESP_EFFECTS_COSTS_GAP
		self.ESP_H_MISSION_BUTTON = self.ESP_MISSION_BUTTON_HEIGHT
		self.ESP_ROW_X_RIGHT = max(self.ESP_ROW_X_RIGHT_MIN, self.ESP_W_SCROLL - self.ESP_ROW_X_RIGHT_OFFSET)

	def widenEspionageLeftPaneForCompactRows(self):
		iLeftPaneWidth = min(self.ESP_W_LEFT_PANE + self.ESP_COMPACT_LEFT_PANE_EXTRA, max(self.ESP_W_LEFT_PANE, self.ESP_W_SCREEN - self.ESP_COMPACT_RIGHT_PANE_WIDTH_MIN - self.ESP_OUTER_MARGIN))
		if iLeftPaneWidth == self.ESP_W_LEFT_PANE:
			return
		self.ESP_W_LEFT_PANE = iLeftPaneWidth
		self.ESP_W_SCROLL = max(self.ESP_SCROLL_WIDTH_MIN, self.ESP_W_LEFT_PANE - (2 * self.ESP_INNER_MARGIN))
		self.ESP_X_TOTAL_PANE = self.ESP_X_LEFT_PANE + self.ESP_W_LEFT_PANE + self.ESP_PANEL_GAP
		self.ESP_W_TOTAL_PANE = max(self.ESP_TOTAL_PANE_WIDTH_MIN, self.ESP_W_SCREEN - self.ESP_X_TOTAL_PANE - self.ESP_OUTER_MARGIN)
		self.ESP_X_RIGHT_PANE = self.ESP_X_TOTAL_PANE
		self.ESP_W_RIGHT_PANE = self.ESP_W_TOTAL_PANE
		self.ESP_X_CITY_LIST = self.ESP_X_RIGHT_PANE + self.ESP_INNER_MARGIN
		self.ESP_W_CITY_LIST = max(self.ESP_CITY_LIST_WIDTH_MIN, self.ESP_W_RIGHT_PANE * self.ESP_CITY_LIST_WIDTH_PERCENT / 100)
		self.ESP_X_EFFECTS_LIST = self.ESP_X_CITY_LIST + self.ESP_W_CITY_LIST + self.ESP_INNER_MARGIN
		self.ESP_W_EFFECTS_COSTS_LIST = max(self.ESP_EFFECTS_COSTS_WIDTH_MIN, self.ESP_W_RIGHT_PANE * self.ESP_EFFECTS_COSTS_WIDTH_PERCENT / 100)
		self.ESP_W_EFFECTS_LIST = max(self.ESP_EFFECTS_WIDTH_MIN, self.ESP_W_RIGHT_PANE - (self.ESP_X_EFFECTS_LIST - self.ESP_X_RIGHT_PANE) - self.ESP_EFFECTS_COSTS_GAP - self.ESP_W_EFFECTS_COSTS_LIST - self.ESP_INNER_MARGIN)
		self.ESP_X_EFFECTS_COSTS_LIST = self.ESP_X_EFFECTS_LIST + self.ESP_W_EFFECTS_LIST + self.ESP_EFFECTS_COSTS_GAP
		self.ESP_X_MISSIONS_LIST = self.ESP_X_EFFECTS_LIST
		self.ESP_W_MISSIONS_LIST = self.ESP_W_EFFECTS_LIST
		self.ESP_X_MISSIONS_COSTS_LIST = self.ESP_X_MISSIONS_LIST + self.ESP_W_MISSIONS_LIST + self.ESP_EFFECTS_COSTS_GAP
		self.ESP_W_MISSIONS_COSTS_LIST = self.ESP_W_EFFECTS_COSTS_LIST
		self.ESP_X_MISSION_BUTTON = self.ESP_X_MISSIONS_LIST
		self.ESP_W_MISSION_BUTTON = self.ESP_W_MISSIONS_LIST + self.ESP_W_MISSIONS_COSTS_LIST + self.ESP_EFFECTS_COSTS_GAP
		self.ESP_ROW_X_RIGHT = max(self.ESP_ROW_X_RIGHT_MIN, self.ESP_W_SCROLL - self.ESP_ROW_X_RIGHT_OFFSET)

	def updateRuntimeTabLinkWidths(self):
		# <!-- custom: tab-link widths depend on runtime X_EXIT; recompute every open using shared helper. (GPT-5.3-Codex) -->
		aszTabLabels = []
		for i in self.FOREIGN_TAB_KEYS:
			aszTabLabels.append(localText.getText(self.TXT_KEY_DICT[i], ()).upper())
		self.LABEL_WIDTH_LIST[:] = getAdvisorRuntimeLinkWidths(CyInterface(), aszTabLabels, self.EXIT_TEXT, self.X_EXIT)

	def interfaceScreen (self, iScreen):
		screen = self.getScreen()
		self.updateRuntimeLayout(screen)

		# <!-- custom: initialize language-dependent text once, then recompute tab widths from runtime geometry. (GPT-5.3-Codex) -->
		self.initText()
		self.updateRuntimeTabLinkWidths()

		# self.ATTITUDE_DICT = {
		# 	"COLOR_YELLOW": re.sub (":", "|", localText.getText ("TXT_KEY_ATTITUDE_FRIENDLY", ())),
		# 	"COLOR_GREEN" : re.sub (":", "|", localText.getText ("TXT_KEY_ATTITUDE_PLEASED", ())),
		# 	"COLOR_CYAN" : re.sub (":", "|", localText.getText ("TXT_KEY_ATTITUDE_ANNOYED", ())),
		# 	"COLOR_RED" : re.sub (":", "|", localText.getText ("TXT_KEY_ATTITUDE_FURIOUS", ())),
		# }

		self.WAR_ICON = smallSymbol(FontSymbols.WAR_CHAR)
		self.PEACE_ICON = smallSymbol(FontSymbols.PEACE_CHAR)
		# <!-- custom: add "Willing to become a vassal" type of button as it is useful for the human player to see it in UI in the glances tab.
		# Added with the help of claude sonnet 4.5 and gemini 3 pro thanks; check if accurate -->
		self.WILLING_VASSAL_ICON = smallSymbol(FontSymbols.SILVER_STAR_CHAR) # using a crown/strength symbol for vassalization
		self.VASSAL_ICON = smallSymbol(FontSymbols.STRENGTH_CHAR)  # or use a different symbol

		self.objTechTree = TechTree.TechTree()

		if (iScreen < 0):
			if (self.iScreen < 0):
				iScreen = self.SCREEN_DICT[self.FOREIGN_TAB_KEYS[0]]
			else:
				iScreen = self.iScreen

		if (self.iScreen != iScreen):	
			self.killScreen()
			self.iScreen = iScreen

		if screen.isActive():
			return
		screen.setRenderInterfaceOnly(True)
		screen.showScreen( PopupStates.POPUPSTATE_IMMEDIATE, False)

		# <!-- custom: preserve the selected debug perspective when reopening Foreign Advisor; base code reset to the active player every open. (GPT-5.5) -->
		self.iActiveLeader = getAdvisorValidPerspectivePlayer(self.iActiveLeader)
		self.iSelectedLeader = self.iActiveLeader
		self.listSelectedLeaders = []
		#self.listSelectedLeaders.append(self.iSelectedLeader)

		############################################
		### BEGIN CHANGES ENHANCED INTERFACE MOD ###
		############################################
		#self.W_SCREEN = screen.getXResolution()
		#self.H_SCREEN = screen.getYResolution()

		# RJG Start - following line added as per RJG (http://forums.civfanatics.com/showpost.php?p=6996936&postcount=15)
		# FROM BUG MA Widescreen START
		# over-ride screen width, height

		##
		# K-Mod, 7/dec/12, karadoc
		#returned the window to the standard size
		##
		#self.W_SCREEN = screen.getXResolution() - 40
		#self.X_SCREEN = (screen.getXResolution() - 24) / 2
		#self.L_SCREEN = 20

		#if self.W_SCREEN < 1024:
			#self.W_SCREEN = 1024
			#self.L_SCREEN = 0

		# FROM BUG MA Widescreen END
		#self.X_EXIT = self.W_SCREEN - 10
		# RJG End
		#self.DX_LINK = (self.X_EXIT - self.X_LINK) / (len (self.SCREEN_DICT) + 1) # disabled by K-Mod

		# Set the background and exit button, and show the screen
		screen.setDimensions(self.X_SCREEN, self.Y_SCREEN, self.W_SCREEN, self.H_SCREEN)
		screen.addDrawControl(self.BACKGROUND_ID, self.SAS_ART_SCREEN_BG_OPAQUE, 0, 0, self.W_SCREEN, self.H_SCREEN, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		screen.addPanel( "TopPanel", u"", u"", True, False, 0, 0, self.W_SCREEN, 55, PanelStyles.PANEL_STYLE_TOPBAR )
		screen.addPanel( "BottomPanel", u"", u"", True, False, 0, self.Y_BOTTOM_PANEL, self.W_SCREEN, 55, PanelStyles.PANEL_STYLE_BOTTOMBAR )
		##########################################
		### END CHANGES ENHANCED INTERFACE MOD ###
		##########################################

		# Set the background and exit button, and show the screen
		# <!-- custom: in the foreign advisor and similar screens, we can't see all info in one screen when there are too many players, yet the window does not use all the game window space.
		# Make it larger, similarly to what we did for sevopedia, so that we don't have to scroll or less so.
		# Code added with the help of gemini 3 pro and then fixed with claude sonnet 4.5's review thanks ;check if accurate -->
		# # RJG Start - following line added as per RJG (http://forums.civfanatics.com/showpost.php?p=6996936&postcount=15)
		# # K-Mod, undone
		# screen.setDimensions(screen.centerX(0), screen.centerY(0), self.W_SCREEN, self.H_SCREEN)
		# #screen.setDimensions(self.L_SCREEN, screen.centerY(0), self.W_SCREEN, self.H_SCREEN)
		# # RJG end
		# <!-- custom: keep advisor anchored like non-BUG military advisor (left/top offsets + explicit width/height), not centered. (GPT-5.3-Codex) -->

		screen.showWindowBackground(False)
		screen.setText(self.EXIT_ID, "", self.EXIT_TEXT, CvUtil.FONT_RIGHT_JUSTIFY, self.X_EXIT, self.Y_EXIT, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_CLOSE_SCREEN, -1, -1 )

		self.nWidgetCount = 0
		self.nLineCount = 0

		if (CyGame().isDebugMode()):
			self.szDropdownName = self.getWidgetName(self.DEBUG_DROPDOWN_ID)
			addAdvisorDebugDropdown(screen, self.szDropdownName, self.iActiveLeader) # advc.007: Exclude Barbarians

		CyInterface().setDirty(InterfaceDirtyBits.Foreign_Screen_DIRTY_BIT, False)

		# Draw leader heads
		self.drawContents(True)

	# Drawing Leaderheads
	def drawContents(self, bInitial):

		if (self.iScreen < 0):
			return

		self.objActiveLeader = gc.getPlayer(self.iActiveLeader)
		self.iActiveTeam = self.objActiveLeader.getTeam()
		self.objActiveTeam = gc.getTeam(self.iActiveTeam)
		self.deleteAllWidgets()

		screen = self.getScreen()

		# Header...
		screen.setLabel(self.getNextWidgetName(), "", self.SCREEN_TITLE, CvUtil.FONT_CENTER_JUSTIFY, self.X_TITLE, self.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		if (self.REV_SCREEN_DICT.has_key(self.iScreen)):
			self.DRAW_DICT[self.REV_SCREEN_DICT[self.iScreen]] (bInitial)
		else:
			return

		# Link to other Foreign advisor screens
		#xLink = self.DX_LINK / 2;
		# <!-- custom: LABEL_WIDTH_LIST is recomputed at screen entry because runtime width depends on X_EXIT and the active Foreign tab cluster. (Claude code Sonnet 4.5 + GPT-5.5) -->

		aszTabWidgetIDs = []
		aszTabLabels = []
		aiTabScreenIDs = []
		aszTabKeys = self.FOREIGN_TAB_KEYS
		for i in range (len (aszTabKeys)):
			szScreen = aszTabKeys[i]
			aszTabWidgetIDs.append(self.getNextWidgetName())
			aszTabLabels.append(localText.getText(self.TXT_KEY_DICT[szScreen], ()).upper())
			aiTabScreenIDs.append(self.SCREEN_DICT[szScreen])
		# <!-- custom: When removing the old interdependent Foreign/Exotic Foreign setup, our first unified Foreign + Diplomacy-shell code had repeated clicks on the 2nd tab of Foreign Diplomacy Advisor (Glance) jump to the 2nd tab of Foreign Trade Advisor (Bonuses).
		# That is not consistent with how other links behave and is unnecessary for players.
		# Passing self.iScreen as the active page id keeps the selected label inert while inactive tabs still route through their SCREEN_DICT ids. (GPT-5.3-Codex + GPT-5.5) -->
		drawAdvisorFooterTabs(screen, aszTabWidgetIDs, aszTabLabels, self.LABEL_WIDTH_LIST, self.iScreen, self.Y_LINK, 0, self.COLOR_YELLOW, WidgetTypes.WIDGET_FOREIGN_ADVISOR, aiInactiveData1=aiTabScreenIDs)

	def drawActive (self, bInitial):
		screen = self.getScreen()

		# <!-- custom: this tab targets active treaty-style items; some one-shot trade types can still surface in mixed/custom deals, so icon fallbacks below intentionally handle both ongoing and occasional non-ongoing items. (GPT-5.3-Codex) -->
		def formatTreatyTrade(tradeData, iPlayerFrom, iPlayerTo):
			szTrade = CyGameTextMgr().getTradeString(tradeData, iPlayerFrom, iPlayerTo)
			if tradeData.ItemType == TradeableItems.TRADE_OPEN_BORDERS:
				return u"<img=%s size=%d></img> %s" % (self.SAS_TRADE_ICON_OPEN_BORDERS, self.SAS_TREATY_ICON_SIZE, szTrade)
			elif tradeData.ItemType == TradeableItems.TRADE_DEFENSIVE_PACT:
				return u"<img=%s size=%d></img> %s" % (self.SAS_TRADE_ICON_DEFENSIVE_PACT, self.SAS_TREATY_ICON_SIZE, szTrade)
			elif tradeData.ItemType == TradeableItems.TRADE_PERMANENT_ALLIANCE:
				return u"<img=%s size=%d></img> %s" % (self.SAS_TRADE_ICON_PERMANENT_ALLIANCE, self.SAS_TREATY_ICON_SIZE, szTrade)
			elif tradeData.ItemType == TradeableItems.TRADE_VASSAL or tradeData.ItemType == TradeableItems.TRADE_SURRENDER:
				return u"<img=%s size=%d></img> %s" % (self.SAS_TRADE_ICON_VASSAL, self.SAS_TREATY_ICON_SIZE, szTrade)
			elif tradeData.ItemType == TradeableItems.TRADE_TECHNOLOGIES:
				return u"<img=%s size=%d></img> %s" % (gc.getTechInfo(tradeData.iData).getButton(), self.SAS_TREATY_ICON_SIZE, szTrade)
			elif tradeData.ItemType == TradeableItems.TRADE_CIVIC:
				return u"<img=%s size=%d></img> %s" % (gc.getCivicInfo(tradeData.iData).getButton(), self.SAS_TREATY_ICON_SIZE, szTrade)
			elif tradeData.ItemType == TradeableItems.TRADE_RELIGION:
				return u"<img=%s size=%d></img> %s" % (gc.getReligionInfo(tradeData.iData).getButton(), self.SAS_TREATY_ICON_SIZE, szTrade)
			# <!-- custom: Resource entries are formatted only when a visible mixed deal must disclose its complete cancellation scope; resource-only deals remain filtered below. See KI#241. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			elif tradeData.ItemType == TradeableItems.TRADE_RESOURCES:
				return u"<img=%s size=%d></img> %s" % (gc.getBonusInfo(tradeData.iData).getButton(), self.SAS_TREATY_ICON_SIZE, szTrade)
			elif tradeData.ItemType == TradeableItems.TRADE_GOLD or tradeData.ItemType == TradeableItems.TRADE_GOLD_PER_TURN:
				return u"%c %s" % (self.iGoldIcon, szTrade)
			elif tradeData.ItemType == TradeableItems.TRADE_MAPS:
				# <!-- custom: map trading is typically one-time (not an ongoing treaty), but keep icon fallback in case mixed/custom deal text surfaces here. (GPT-5.3-Codex) -->
				return u"%c %s" % (self.iMapIcon, szTrade)
			else:
				return u"%c %s" % (self.iTradeIcon, szTrade)

		# Get the Players
		playerActive = gc.getPlayer(self.iActiveLeader)

		# Put everything inside a main panel, so we get vertical scrolling
		mainPanelName = self.getNextWidgetName()

		#screen.addPanel(mainPanelName, "", "", True, True, 50, 100, self.W_SCREEN - 100, self.H_SCREEN - 200, PanelStyles.PANEL_STYLE_EMPTY)
		# <advc.066> Replacing the above (same as in drawInfoOriginal)
		# <!-- custom: remove the margins same as in the other foreign advisor tabs after our changes, similarly to what gemini 3 pro advised in its solution thanks.
		# Note: a negative leftRightMargin value such as -3 allows to remove the last yellow edges that remain at 0 it seems, not applied here for beautification -->
		# leftRightMargin = 25
		# topBottomMargin = 50
		leftRightMargin = 0
		# <!-- custom: 43 is a bit nicer as it overflows less on the top and bottom, but we lose one leader row with it with the current inter row spacing as of now, so keep 42 rather despite the slight overflowing outside the top and bottom edges as of now -->
		# <!-- custom: note: does not necessarily lead to one more leader row as it seems (but not sure, check if accurate as this is just a guess of mine even though it does seem to be as such but check to be sure) rows can be higher if they have more lines as of now in the foreign advisor's active tab but doing as such for consistency as well as in base advciv code as well coincidentally i mean. -->
		topBottomMargin = 42
		mainPanelWidth = self.W_SCREEN - 2 * leftRightMargin
		mainPanelHeight = self.H_SCREEN - 2 * topBottomMargin
		if not gc.getGame().isDebugMode():
			hasMetCount = gc.getTeam(playerActive.getTeam()).getHasMetCivCount(True)
			if hasMetCount > 0: # The 300 is a pretty arbitrary value
				mainPanelHeight = min(mainPanelHeight, 300 * hasMetCount)
		screen.addPanel(mainPanelName, "", "", True, True, leftRightMargin, topBottomMargin, mainPanelWidth, mainPanelHeight, PanelStyles.PANEL_STYLE_EMPTY)
		# </advc.066>

		# loop through all players and sort them by number of active deals
		listPlayers = [(0,0)] * gc.getMAX_PLAYERS()
		nNumPLayers = 0
		for iLoopPlayer in range(gc.getMAX_PLAYERS()):
			# <!-- custom: hoist for performance optimization quite similarly to how gemini 3 pro proposed in a related solution -->
			objLoopPlayer = gc.getPlayer(iLoopPlayer)

			if (objLoopPlayer.isAlive() and iLoopPlayer != self.iActiveLeader and not objLoopPlayer.isBarbarian() and  not objLoopPlayer.isMinorCiv()):
				if (gc.getTeam(objLoopPlayer.getTeam()).isHasMet(gc.getPlayer(self.iActiveLeader).getTeam()) or gc.getGame().isDebugMode()):
					nDeals = 0				
					for i in range(gc.getGame().getIndexAfterLastDeal()):
						deal = gc.getGame().getDeal(i)
						if ((deal.getFirstPlayer() == iLoopPlayer and deal.getSecondPlayer() == self.iActiveLeader) or (deal.getSecondPlayer() == iLoopPlayer and deal.getFirstPlayer() == self.iActiveLeader)):
							nDeals += 1
					listPlayers[nNumPLayers] = (nDeals, iLoopPlayer)
					nNumPLayers += 1
		listPlayers.sort()
		listPlayers.reverse()

		# loop through all players and display leaderheads
		for j in range (nNumPLayers):
			iLoopPlayer = listPlayers[j][1]

			# <!-- custom: hoist for performance optimization quite similarly to how gemini 3 pro proposed in a related solution -->
			# <!-- custom: note: increment variable naming to avoid weird python non-related scope inheritance/default/fallback or whatever it is called -->
			objLoopPlayer2 = gc.getPlayer(iLoopPlayer)

			# Player panel
			playerPanelName = self.getNextWidgetName()
			# advc.066: Third argument was objLoopPlayer2.getName()
			screen.attachPanel(mainPanelName, playerPanelName, "", "", False, True, PanelStyles.PANEL_STYLE_MAIN)

			screen.attachLabel(playerPanelName, "", "   ")

			screen.attachImageButton(playerPanelName, "", gc.getLeaderHeadInfo(objLoopPlayer2.getLeaderType()).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_LEADERHEAD, iLoopPlayer, -1, False)

			innerPanelName = self.getNextWidgetName()
			screen.attachPanel(playerPanelName, innerPanelName, "", "", False, False, PanelStyles.PANEL_STYLE_EMPTY)

			dealPanelName = self.getNextWidgetName()
			screen.attachListBoxGFC(innerPanelName, dealPanelName, "", TableStyles.TABLE_STYLE_EMPTY)	
			screen.enableSelect(dealPanelName, False)

			# <!-- custom: keep one row per deal so each row remains clickable for cancel (WIDGET_DEAL_KILL); only merge items within that single deal row. (GPT-5.3-Codex) -->
			for i in range(gc.getGame().getIndexAfterLastDeal()):
				deal = gc.getGame().getDeal(i)

				if (deal.getFirstPlayer() == iLoopPlayer and deal.getSecondPlayer() == self.iActiveLeader and not deal.isNone()) or (deal.getSecondPlayer() == iLoopPlayer and deal.getFirstPlayer() == self.iActiveLeader):
					listDealEntries = []
					listFirstDealEntries = []
					listSecondDealEntries = []
					bHasResource = False
					for iTrade in range(deal.getLengthFirstTrades()):
						tradeData = deal.getFirstTrade(iTrade)
						if tradeData:
							szTrade = formatTreatyTrade(tradeData, deal.getFirstPlayer(), deal.getSecondPlayer())
							if szTrade:
								listFirstDealEntries.append(szTrade)
							if tradeData.ItemType == TradeableItems.TRADE_RESOURCES:
								bHasResource = True
							elif szTrade and szTrade not in listDealEntries:
								listDealEntries.append(szTrade)
					for iTrade in range(deal.getLengthSecondTrades()):
						tradeData = deal.getSecondTrade(iTrade)
						if tradeData:
							szTrade = formatTreatyTrade(tradeData, deal.getSecondPlayer(), deal.getFirstPlayer())
							if szTrade:
								listSecondDealEntries.append(szTrade)
							if tradeData.ItemType == TradeableItems.TRADE_RESOURCES:
								bHasResource = True
							elif szTrade and szTrade not in listDealEntries:
								listDealEntries.append(szTrade)
					if len(listDealEntries) == 0:
						continue
					# <!-- custom: Treaties intentionally omits resource-only deals, but a visible mixed-deal row previously hid its resource terms while WIDGET_DEAL_KILL still canceled the complete deal.
					# Format every term with its resource/gold/treaty icon and the engine's localized perspective grammar, so displayed and canceled terms match. See KI#241. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
					if bHasResource:
						if deal.getFirstPlayer() == iLoopPlayer:
							listOurDealEntries = listFirstDealEntries
							listTheirDealEntries = listSecondDealEntries
							iOtherPlayer = deal.getSecondPlayer()
						else:
							listOurDealEntries = listSecondDealEntries
							listTheirDealEntries = listFirstDealEntries
							iOtherPlayer = deal.getFirstPlayer()
						objOtherPlayer = gc.getPlayer(iOtherPlayer)
						szOtherPlayer = u"<img=%s size=%d></img> %s" % (gc.getLeaderHeadInfo(objOtherPlayer.getLeaderType()).getButton(), self.SAS_TREATY_ICON_SIZE, objOtherPlayer.getName())
						szOurDeal = u", ".join(listOurDealEntries)
						szTheirDeal = u", ".join(listTheirDealEntries)
						if len(listOurDealEntries) > 0 and len(listTheirDealEntries) > 0:
							szDealText = localText.getText("TXT_KEY_MISC_OUR_DEAL", (szOurDeal, szOtherPlayer, szTheirDeal))
						elif len(listOurDealEntries) > 0:
							szDealText = localText.getText("TXT_KEY_MISC_DEAL_ONESIDED_OURS", (szOurDeal, szOtherPlayer))
						else:
							szDealText = localText.getText("TXT_KEY_MISC_DEAL_ONESIDED_THEIRS", (szTheirDeal, szOtherPlayer))
					else:
						szDealText = u", ".join(listDealEntries)
					# <advc.072>
					iShowTurnsMode = AdvisorOpt.getShowDealTurns()
					bShowTurns = False
					if iShowTurnsMode == 0 or iShowTurnsMode == 2:
						bShowTurns = True
					# </advc.072>
					# advc.007: Now that CvDeal::isCancelable checks if eByPlayer is a party to the deal, turns-to-cancel isn't shown in the widget anymore. To fix this, treat the ShowDealTurnsLeft option as enabled when viewing the screen from the perspective of another civ in Debug mode.
					if bShowTurns or self.iActiveLeader != gc.getGame().getActivePlayer():
						if BugDll.isPresent():
							if not deal.isCancelable(self.iActiveLeader, False):
								if deal.isCancelable(self.iActiveLeader, True):
									szDealText += u" %s" % BugUtil.getText("INTERFACE_CITY_TURNS", (deal.turnsToCancel(self.iActiveLeader),))
								else:
									# don't bother adding "This deal cannot be canceled" message
									#szDealText += u" (%s)" % deal.getCannotCancelReason(self.iActiveLeader)
									pass
						else:
							iTurns = DealUtil.Deal(deal).turnsToCancel(self.iActiveLeader)
							if iTurns > 0:
								szDealText += u" %s" % BugUtil.getText("INTERFACE_CITY_TURNS", (iTurns,))
					screen.appendListBoxString(dealPanelName, labelText(szDealText), WidgetTypes.WIDGET_DEAL_KILL, deal.getID(), -1, CvUtil.FONT_LEFT_JUSTIFY)

	def drawEspionage(self, bInitial):
		# <!-- custom: render Espionage directly as a native Foreign Advisor tab (no helper wrapper object). (GPT-5.3-Codex) -->
		self.ESP_iActivePlayer = self.iActiveLeader
		if self.ESP_iTargetPlayer == -1:
			self.ESP_iActiveCityID = -1
			self.ESP_iSelectedMission = -1
		self.drawEspionageContents()
		# <!-- custom: compact Espionage rows remove repeated labels/icons to keep SAS48 controls readable, so keep a quiet Sevopedia legend link for the field meanings. (GPT-5.5-Thinking + GPT-5.5?) -->
		placeAdvisorLegendLink(self, "CONCEPT_SAS_FOREIGN_ADVISOR_ESPIONAGE_LEGEND", self.W_SCREEN - 12, self.Y_TITLE)
		self.refreshEspionageScreen()

	#	RJG Start
	def drawRelations (self, bInitial):
		screen = self.getScreen()
		#self.W_SCREEN = screen.getXResolution() - 40
		#self.X_SCREEN = (screen.getXResolution() - 24) / 2

		# <!-- custom: in the foreign advisor and similar screens, we can't see all info in one screen when there are too many players, yet the window does not use all the game window space.
		# Make it larger, similarly to what we did for sevopedia, so that we don't have to scroll or less so.
		# Code added with the help of gemini 3 pro and then fixed with claude sonnet 4.5's review thanks ;check if accurate -->
		# self.X_LEADER_CIRCLE_TOP = self.X_SCREEN
		# --- FIX: Center the web in the middle of the screen ---
		# We take the full screen width and divide by 2 to find the center pixel
		self.X_LEADER_CIRCLE_TOP = self.W_SCREEN // 2

		self.drawRelationsBase (bInitial)
	#	RJG End

	def killScreen(self):
		if (self.iScreen >= 0):
			screen = self.getScreen()
			screen.hideScreen()
			self.iScreen = -1
		return

	def getScreen(self):
		return CyGInterfaceScreen(self.SCREEN_NAME + str(self.iScreen), CvScreenEnums.FOREIGN_ADVISOR)

	def drawRelationsBase(self, bInitial):

		if self.iShiftKeyDown == 1:
			if (self.iSelectedLeader in self.listSelectedLeaders):
				self.listSelectedLeaders.remove(self.iSelectedLeader)
			else:
				self.listSelectedLeaders.append(self.iSelectedLeader)
		else:
			self.listSelectedLeaders = []
			if (not bInitial):
				self.listSelectedLeaders.append(self.iSelectedLeader)	

		bNoLeadersSelected = (len(self.listSelectedLeaders) == 0)
		bSingleLeaderSelected = (len(self.listSelectedLeaders) == 1)
		if bSingleLeaderSelected:
			self.iSelectedLeader = self.listSelectedLeaders[0]

		# Get the Players
		playerActive = gc.getPlayer(self.iActiveLeader)

		# count the leaders
		iCount = 0
		leaderMap = { }
		# Count all other leaders
		for iPlayer in range(gc.getMAX_PLAYERS()):
			player = gc.getPlayer(iPlayer)
			if (player.isAlive() and iPlayer != self.iActiveLeader and (gc.getTeam(player.getTeam()).isHasMet(playerActive.getTeam()) or gc.getGame().isDebugMode()) and not player.isBarbarian() and not player.isMinorCiv()):
				leaderMap[iPlayer] = iCount
				iCount = iCount + 1
		fLeaderTop = self.Y_LEADER_CIRCLE_TOP

		# <!-- custom: make radius wider so we use more of our expanded screen now to draw the leader relation's web.
		# Now nicely also dynamically adjusts to screen resolution.
		# Added with the help of gemini 3 pro, check if accurate -->
		# fRadius = self.RADIUS_LEADER_ARC - self.H_LEADER
		# Dynamic Resolution Scaling
		# Define Dynamic Radius
		# We want the web to be as tall as possible without going off the bottom.
		# This sets radius to roughly 75% of screen height. Adjust 0.75 as needed.
		# fRadius = (self.H_SCREEN * 0.75) - self.H_LEADER
		fRadius = (self.H_SCREEN * 64) / 100

		fLeaderArcTop = fLeaderTop + self.H_LEADER + 10
		if iCount < 8:
			iLeaderHeight = int((3 * self.H_LEADER) / 2)
			iLeaderWidth = int((3 * self.W_LEADER) / 2)
		else:
			iLeaderHeight = self.H_LEADER
			iLeaderWidth = self.W_LEADER

		screen = self.getScreen()

		#screen.addPanel(self.getNextWidgetName(), "", "", False, False, 0, 50, self.W_SCREEN, 667, PanelStyles.PANEL_STYLE_MAIN_WHITE)
		#screen.addPanel(self.getNextWidgetName(), "", "", False, False, 0, 50, self.W_SCREEN, 667, PanelStyles.PANEL_STYLE_MAIN_WHITE)
		#screen.addPanel(self.getNextWidgetName(), "", "", False, False, 0, 50, self.W_SCREEN, 667, PanelStyles.PANEL_STYLE_MAIN_WHITE)

		# legend
		screen.addPanel(self.getNextWidgetName(), u"", u"", True, False, self.X_LEGEND, self.Y_LEGEND, self.W_LEGEND, self.H_LEGEND, PanelStyles.PANEL_STYLE_IN)
		x = self.X_LEGEND + self.MARGIN_LEGEND
		y = self.Y_LEGEND + self.MARGIN_LEGEND
		screen.setLabel(self.getNextWidgetName(), "", self.LABEL_REL_CONTACT, CvUtil.FONT_LEFT_JUSTIFY, x, y-10, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		y += self.MARGIN_LEGEND
		screen.addLineGFC(self.BACKGROUND_ID, self.getNextLineName(), x, y, x + self.W_LEGEND - 2*self.MARGIN_LEGEND, y, self.COLOR_WHITE)
		y += 2 * self.MARGIN_LEGEND
		screen.setLabel(self.getNextWidgetName(), "", self.LABEL_CONCEPT_WAR, CvUtil.FONT_LEFT_JUSTIFY, x, y-10, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		y += self.MARGIN_LEGEND
		screen.addLineGFC(self.BACKGROUND_ID, self.getNextLineName(), x, y, x + self.W_LEGEND - 2*self.MARGIN_LEGEND, y, self.COLOR_RED)
		y += 2 * self.MARGIN_LEGEND
		screen.setLabel(self.getNextWidgetName(), "", self.LABEL_TRADE_DEFENSIVE_PACT, CvUtil.FONT_LEFT_JUSTIFY, x, y-10, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		y += self.MARGIN_LEGEND
		screen.addLineGFC(self.BACKGROUND_ID, self.getNextLineName(), x, y, x + self.W_LEGEND - 2*self.MARGIN_LEGEND, y, self.COLOR_BLUE)
		y += 2 * self.MARGIN_LEGEND
		screen.setLabel(self.getNextWidgetName(), "", self.LABEL_TRADE_OPEN_BORDERS, CvUtil.FONT_LEFT_JUSTIFY, x, y-10, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		y += self.MARGIN_LEGEND
		screen.addLineGFC(self.BACKGROUND_ID, self.getNextLineName(), x, y, x + self.W_LEGEND - 2*self.MARGIN_LEGEND, y, self.COLOR_CITY_GREEN)
		y += 2 * self.MARGIN_LEGEND
		screen.setLabel(self.getNextWidgetName(), "", self.LABEL_PITBOSS_TEAM, CvUtil.FONT_LEFT_JUSTIFY, x, y-10, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		y += self.MARGIN_LEGEND
		screen.addLineGFC(self.BACKGROUND_ID, self.getNextLineName(), x, y, x + self.W_LEGEND - 2*self.MARGIN_LEGEND, y, self.COLOR_YELLOW)
		y += 2 * self.MARGIN_LEGEND
		screen.setLabel(self.getNextWidgetName(), "", self.LABEL_MISC_VASSAL_SHORT, CvUtil.FONT_LEFT_JUSTIFY, x, y-10, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		y += self.MARGIN_LEGEND
		screen.addLineGFC(self.BACKGROUND_ID, self.getNextLineName(), x, y, x + self.W_LEGEND - 2*self.MARGIN_LEGEND, y, self.COLOR_CYAN)

		# Our leader head

		# K-mod (moved from below)
		if bSingleLeaderSelected:
			iBaseLeader = self.iSelectedLeader
		else:
			iBaseLeader = self.iActiveLeader

		playerBase = gc.getPlayer(iBaseLeader)
		# K-Mod end
		szLeaderHead = self.getNextWidgetName()
		#screen.addCheckBoxGFC(szLeaderHead, gc.getLeaderHeadInfo(playerActive.getLeaderType()).getButton(), self.SAS_ART_BUTTON_HILITE_SQUARE, self.X_LEADER_CIRCLE_TOP - iLeaderWidth/2, int(fLeaderTop), iLeaderWidth, iLeaderHeight, WidgetTypes.WIDGET_LEADERHEAD, self.iActiveLeader, -1, ButtonStyles.BUTTON_STYLE_LABEL)
		screen.addCheckBoxGFC(szLeaderHead, gc.getLeaderHeadInfo(playerActive.getLeaderType()).getButton(), self.SAS_ART_BUTTON_HILITE_SQUARE, self.X_LEADER_CIRCLE_TOP - iLeaderWidth/2, int(fLeaderTop), iLeaderWidth, iLeaderHeight, WidgetTypes.WIDGET_LEADERHEAD, self.iActiveLeader, iBaseLeader, ButtonStyles.BUTTON_STYLE_LABEL) # K-Mod
		if (self.iActiveLeader in self.listSelectedLeaders):
			screen.setState(szLeaderHead, True)
		else:
			screen.setState(szLeaderHead, False)
		szName = self.getNextWidgetName()
		szLeaderName = sasFontTagLabel + playerActive.getName() + SAS_FONT_TAG_CLOSE
		screen.setLabel(szName, "", szLeaderName, CvUtil.FONT_CENTER_JUSTIFY, self.X_LEADER_CIRCLE_TOP, fLeaderTop + iLeaderHeight + 5, 0, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# K-Mod. vassal / master label
		szName = self.getNextWidgetName()
		szText = u""
		player = playerActive
		if iBaseLeader != self.iActiveLeader and gc.getTeam(player.getTeam()).isHasMet(playerBase.getTeam()):
			if (gc.getTeam(player.getTeam()).isVassal(playerBase.getTeam())):
				szText += self.TEXT_MISC_VASSAL_SHORT
			elif (gc.getTeam(playerBase.getTeam()).isVassal(player.getTeam())):
				szText += self.TEXT_MISC_MASTER
			if not playerActive.isHuman():
				if szText != "":
					szText += ", "
				szText += gc.getAttitudeInfo(playerActive.AI_getAttitude(iBaseLeader)).getDescription()
			if szText != "":
				szText = " (" + szText + ")"
		screen.setLabel(szName, "", labelText(szText), CvUtil.FONT_CENTER_JUSTIFY, self.X_LEADER_CIRCLE_TOP, fLeaderTop + iLeaderHeight + 25, 0, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		# K-Mod end

		# angle increment in radians (180 degree range)
		if (iCount < 2):
			deltaTheta = 0
		else:
			deltaTheta = 3.1415927 / (iCount - 1)

		# draw other leaderheads
		for iPlayer in leaderMap.keys():
			player = gc.getPlayer(iPlayer)

			# (iBaseLeader code was here)

			fX = int(self.X_LEADER_CIRCLE_TOP - fRadius * math.cos(deltaTheta * leaderMap[iPlayer]) - iLeaderWidth/2) 
			fY = int(fLeaderArcTop + fRadius * math.sin(deltaTheta * leaderMap[iPlayer]) - iLeaderHeight/2)

			szLeaderHead = self.getNextWidgetName()
			screen.addCheckBoxGFC(szLeaderHead, gc.getLeaderHeadInfo(player.getLeaderType()).getButton(), self.SAS_ART_BUTTON_HILITE_SQUARE, int(fX), int(fY), iLeaderWidth, iLeaderHeight, WidgetTypes.WIDGET_LEADERHEAD, iPlayer, iBaseLeader, ButtonStyles.BUTTON_STYLE_LABEL)
			if (iPlayer in self.listSelectedLeaders):
				screen.setState(szLeaderHead, True)
			else:
				screen.setState(szLeaderHead, False)

			szName = self.getNextWidgetName()
			szText = sasFontTagLabel + player.getName() + SAS_FONT_TAG_CLOSE
			screen.setLabel(szName, "", szText, CvUtil.FONT_CENTER_JUSTIFY, fX + iLeaderWidth/2, fY + iLeaderHeight + 5, 0, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

			# Leader attitude towards active player. (rewritten by K-Mod)
			szName = self.getNextWidgetName()
			szText = u""
			if iBaseLeader != iPlayer and gc.getTeam(player.getTeam()).isHasMet(playerBase.getTeam()):
				if (gc.getTeam(player.getTeam()).isVassal(playerBase.getTeam())):
					szText += self.TEXT_MISC_VASSAL_SHORT
				elif (gc.getTeam(playerBase.getTeam()).isVassal(player.getTeam())):
					szText += self.TEXT_MISC_MASTER
				if not gc.getPlayer(iPlayer).isHuman():
					if szText != "":
						szText += ", "
					szText += gc.getAttitudeInfo(gc.getPlayer(iPlayer).AI_getAttitude(iBaseLeader)).getDescription()
				if szText != "":
					szText = " (" + szText + ")"
			screen.setLabel(szName, "", labelText(szText), CvUtil.FONT_CENTER_JUSTIFY, fX + iLeaderWidth/2, fY + iLeaderHeight + 25, 0, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		# draw lines
		for iSelectedLeader in range(gc.getMAX_PLAYERS()):
			bDisplayed = (not gc.getPlayer(iSelectedLeader).isBarbarian() and not gc.getPlayer(iSelectedLeader).isMinorCiv() and gc.getPlayer(iSelectedLeader).isAlive() and (gc.getGame().isDebugMode() or gc.getTeam(playerActive.getTeam()).isHasMet(gc.getPlayer(iSelectedLeader).getTeam())))
			if iSelectedLeader in self.listSelectedLeaders or (bNoLeadersSelected and bDisplayed):
				# get selected player and location
				if (iSelectedLeader in leaderMap):
					thetaSelected = deltaTheta * leaderMap[iSelectedLeader]
					fXSelected = self.X_LEADER_CIRCLE_TOP - fRadius * math.cos(thetaSelected)
					fYSelected = fLeaderArcTop + fRadius * math.sin(thetaSelected)
				else:
					fXSelected = self.X_LEADER_CIRCLE_TOP
					fYSelected = fLeaderTop + iLeaderHeight/2

				for iPlayer in leaderMap.keys():
					player = gc.getPlayer(iPlayer)

					fX = self.X_LEADER_CIRCLE_TOP - fRadius * math.cos(deltaTheta * leaderMap[iPlayer])
					fY = fLeaderArcTop + fRadius * math.sin(deltaTheta * leaderMap[iPlayer])

					# draw lines
					if (iSelectedLeader != iPlayer):
						if (player.getTeam() == gc.getPlayer(iSelectedLeader).getTeam()):
							szName = self.getNextLineName()
							screen.addLineGFC(self.BACKGROUND_ID, szName, int(fXSelected), int(fYSelected), int(fX), int(fY), self.COLOR_YELLOW )						
						elif (gc.getTeam(player.getTeam()).isVassal(gc.getPlayer(iSelectedLeader).getTeam()) or gc.getTeam(gc.getPlayer(iSelectedLeader).getTeam()).isVassal(player.getTeam())):
							szName = self.getNextLineName()
							screen.addLineGFC(self.BACKGROUND_ID, szName, int(fXSelected), int(fYSelected), int(fX), int(fY), self.COLOR_CYAN )						
						elif (gc.getTeam(player.getTeam()).isHasMet(gc.getPlayer(iSelectedLeader).getTeam())):
							if (gc.getTeam(player.getTeam()).isAtWar(gc.getPlayer(iSelectedLeader).getTeam())):
								szName = self.getNextLineName()
								screen.addLineGFC(self.BACKGROUND_ID, szName, int(fXSelected), int(fYSelected), int(fX), int(fY), self.COLOR_RED )
							else:
								bJustPeace = True
								if (gc.getTeam(player.getTeam()).isOpenBorders(gc.getPlayer(iSelectedLeader).getTeam())):
									fDy = fYSelected - fY
									fDx = fXSelected - fX
									fTheta = math.atan2(fDy, fDx)
									if (fTheta > 0.5 * math.pi):
										fTheta -= math.pi
									elif (fTheta < -0.5 * math.pi):
										fTheta += math.pi
									fSecondLineOffsetY = self.LINE_WIDTH * math.cos(fTheta)
									fSecondLineOffsetX = -self.LINE_WIDTH * math.sin(fTheta)
									szName = self.getNextLineName()
									screen.addLineGFC(self.BACKGROUND_ID, szName, int(fXSelected + fSecondLineOffsetX), int(fYSelected + fSecondLineOffsetY), int(fX + fSecondLineOffsetX), int(fY + fSecondLineOffsetY), self.COLOR_CITY_GREEN )
									bJustPeace = False
								if (gc.getTeam(player.getTeam()).isDefensivePact(gc.getPlayer(iSelectedLeader).getTeam())):
									szName = self.getNextLineName()
									screen.addLineGFC(self.BACKGROUND_ID, szName, int(fXSelected), int(fYSelected), int(fX), int(fY), self.COLOR_BLUE )
									bJustPeace = False
								if (bJustPeace):
									szName = self.getNextLineName()
									screen.addLineGFC(self.BACKGROUND_ID, szName, int(fXSelected), int(fYSelected), int(fX), int(fY), self.COLOR_WHITE )

				player = gc.getPlayer(self.iActiveLeader)
				if (player.getTeam() == gc.getPlayer(iSelectedLeader).getTeam()):
					szName = self.getNextLineName()
					screen.addLineGFC(self.BACKGROUND_ID, szName, int(fXSelected), int(fYSelected), self.X_LEADER_CIRCLE_TOP, fLeaderTop + iLeaderHeight/2, self.COLOR_YELLOW )
				elif (gc.getTeam(player.getTeam()).isVassal(gc.getPlayer(iSelectedLeader).getTeam()) or gc.getTeam(gc.getPlayer(iSelectedLeader).getTeam()).isVassal(player.getTeam())):
					szName = self.getNextLineName()
					screen.addLineGFC(self.BACKGROUND_ID, szName, int(fXSelected), int(fYSelected), self.X_LEADER_CIRCLE_TOP, fLeaderTop + iLeaderHeight/2, self.COLOR_CYAN )
				elif (gc.getTeam(player.getTeam()).isHasMet(gc.getPlayer(iSelectedLeader).getTeam())):
					if (gc.getTeam(player.getTeam()).isAtWar(gc.getPlayer(iSelectedLeader).getTeam())):
						szName = self.getNextLineName()
						screen.addLineGFC(self.BACKGROUND_ID, szName, int(fXSelected), int(fYSelected), self.X_LEADER_CIRCLE_TOP, fLeaderTop + iLeaderHeight/2, self.COLOR_RED )
					else:
						bJustPeace = True
						if (gc.getTeam(player.getTeam()).isOpenBorders(gc.getPlayer(iSelectedLeader).getTeam())):
							fDy = fLeaderTop + iLeaderHeight/2 - fYSelected
							fDx = self.X_LEADER_CIRCLE_TOP - fXSelected
							fTheta = math.atan2(fDy, fDx)
							if (fTheta > 0.5 * math.pi):
								fTheta -= math.pi
							elif (fTheta < -0.5 * math.pi):
								fTheta += math.pi
							fSecondLineOffsetY = self.LINE_WIDTH * math.cos(fTheta)
							fSecondLineOffsetX = -self.LINE_WIDTH * math.sin(fTheta)
							szName = self.getNextLineName()
							screen.addLineGFC(self.BACKGROUND_ID, szName, int(fXSelected + fSecondLineOffsetX), int(fYSelected + fSecondLineOffsetY), int(self.X_LEADER_CIRCLE_TOP + fSecondLineOffsetX), int(fLeaderTop + iLeaderHeight/2 + fSecondLineOffsetY), self.COLOR_CITY_GREEN )
							bJustPeace = False
						if (gc.getTeam(player.getTeam()).isDefensivePact(gc.getPlayer(iSelectedLeader).getTeam())):
							szName = self.getNextLineName()
							screen.addLineGFC(self.BACKGROUND_ID, szName, int(fXSelected), int(fYSelected), int(self.X_LEADER_CIRCLE_TOP), int(fLeaderTop + iLeaderHeight/2), self.COLOR_BLUE )
							bJustPeace = False
						if (bJustPeace):
							szName = self.getNextLineName()
							screen.addLineGFC(self.BACKGROUND_ID, szName, int(fXSelected), int(fYSelected), int(self.X_LEADER_CIRCLE_TOP), int(fLeaderTop + iLeaderHeight/2), self.COLOR_WHITE )

	def getNextWidgetName(self):
		szName = self.WIDGET_ID + str(self.nWidgetCount * NUM_FOREIGN_SCREENS + self.iScreen)
		self.nWidgetCount += 1
		return szName

	def getNextLineName(self):
		szName = self.LINE_ID + str(self.nLineCount * NUM_FOREIGN_SCREENS + self.iScreen)
		self.nLineCount += 1
		return szName

	def getWidgetName(self, szBaseName):
		szName = szBaseName + str(self.iScreen)
		return szName

	def clearAllLines(self):
		screen = self.getScreen()
		nLines = self.nLineCount
		self.nLineCount = 0
		for i in range(nLines):
			screen.removeLineGFC(self.BACKGROUND_ID, self.getNextLineName())
		self.nLineCount = 0	

	def deleteAllWidgets(self):
		screen = self.getScreen()
		i = self.nWidgetCount - 1
		while (i >= 0):
			self.nWidgetCount = i
			screen.deleteWidget(self.getNextWidgetName())
			i -= 1

		self.nWidgetCount = 0
		self.clearAllLines()			

	def update(self, fDelta):
		if (CyInterface().isDirty(InterfaceDirtyBits.Foreign_Screen_DIRTY_BIT) == True):
			CyInterface().setDirty(InterfaceDirtyBits.Foreign_Screen_DIRTY_BIT, False)
			self.drawContents(False)
		elif (self.iScreen == self.SCREEN_DICT["ESPIONAGE"] and CyInterface().isDirty(InterfaceDirtyBits.Espionage_Advisor_DIRTY_BIT)):
			CyInterface().setDirty(InterfaceDirtyBits.Espionage_Advisor_DIRTY_BIT, False)
			self.refreshEspionageScreen()
		return

	def drawInfo (self, bInitial):
		# <!-- custom: always use the unified table-style info layout (drawInfoImproved) for stable column behavior with UI scaling and scrolling. (GPT-5.3-Codex) -->
		self.drawInfoImproved(bInitial)

	def drawInfoOriginal (self, bInitial):
		# ForeignAdvisorPrint ("Entered drawInfo")

		screen = self.getScreen()

		# Get the Players
		playerActive = gc.getPlayer(self.iActiveLeader)

		# Put everything inside a main panel, so we get vertical scrolling
		mainPanelName = self.getNextWidgetName()

		#screen.addPanel(mainPanelName, "", "", True, True, 50, 100, self.W_SCREEN - 100, self.H_SCREEN - 200, PanelStyles.PANEL_STYLE_EMPTY)
		# <advc.066> Replacing the above (same as in drawActive)
		# <!-- custom: remove the margins same as in the other foreign advisor tabs after our changes, similarly to what gemini 3 pro advised in its solution thanks.
		# Note: a negative leftRightMargin value such as -3 allows to remove the last yellow edges that remain at 0 it seems, not applied here for beautification -->
		# leftRightMargin = 25
		# topBottomMargin = 50
		leftRightMargin = 0
		# <!-- custom: 43 is a bit nicer as it overflows less on the top and bottom, but we lose one leader row with it with the current inter row spacing as of now, so keep 42 rather despite the slight overflowing outside the top and bottom edges as of now -->
		topBottomMargin = 42
		mainPanelWidth = self.W_SCREEN - 2 * leftRightMargin
		mainPanelHeight = self.H_SCREEN - 2 * topBottomMargin

		# <!-- custom: hoist for performance optimization, done with the help of gemini 3 pro thanks -->
		bDebugMode = gc.getGame().isDebugMode()
		playerActiveTeam = playerActive.getTeam()

		if not bDebugMode:
			hasMetCount = gc.getTeam(playerActiveTeam).getHasMetCivCount(True)
			if hasMetCount > 0: # 100 was just a guess, but it seems like this is the exact height of a row.
				mainPanelHeight = min(mainPanelHeight, 100 * hasMetCount)
		screen.addPanel(mainPanelName, "", "", True, True, leftRightMargin, topBottomMargin, mainPanelWidth, mainPanelHeight, PanelStyles.PANEL_STYLE_EMPTY)
		# </advc.066>

		ltCivicOptions = range (gc.getNumCivicOptionInfos())

		# <!-- custom: add spacing so the parts of the row is not stuck to the other ones too tight quite similarly to how per gemini 3 pro's suggested as part of its solution thanks -->
		# This adds spaces to separate 'Current Civics' from 'Favorites'
		# szSeparator = ""
		szSeparator = "                                                                         "
		# <!-- custom: hoist and compute once the strings is computationally more efficient -->
		# <!-- custom: remove unneeded civics text as it's distracting and the civics listing is already self-explanatory or fairly easily guessable or searchable -->
		# szCivicsPreText = szSeparator + localText.getText("TXT_KEY_CIVICS_SCREEN_TITLE", ()) + ":"
		szCivicsPreText = szSeparator
		# <!-- custom: add spacing so the Favorites part of the row is not stuck to the other ones too tight quite similarly to how per gemini 3 pro's suggested as part of its solution thanks -->
		# szFavoritesPreText = self.TEXT_FAVORITES + ":"
		szFavoritesPreText = szSeparator + self.TEXT_FAVORITES + ":"

		# loop through all players and display leaderheads
		# Their leaderheads
		for iLoopPlayer in range(gc.getMAX_PLAYERS()):
			# <!-- custom: hoist for performance optimization, done with the help of gemini 3 pro thanks -->
			objLoopPlayer = gc.getPlayer(iLoopPlayer)

			if (objLoopPlayer.isAlive() and iLoopPlayer != self.iActiveLeader and (gc.getTeam(objLoopPlayer.getTeam()).isHasMet(playerActiveTeam) or bDebugMode) and not objLoopPlayer.isBarbarian() and not objLoopPlayer.isMinorCiv()):

				nPlayerReligion = objLoopPlayer.getStateReligion()
				objReligion = gc.getReligionInfo (nPlayerReligion)

				objLeaderHead = gc.getLeaderHeadInfo (objLoopPlayer.getLeaderType())

				# Player panel
				playerPanelName = self.getNextWidgetName()
				# advc.066: Third argument was objLoopPlayer.getName()
				screen.attachPanel(mainPanelName, playerPanelName, "", "", False, True, PanelStyles.PANEL_STYLE_MAIN)

				screen.attachImageButton(playerPanelName, "", objLeaderHead.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_LEADERHEAD, iLoopPlayer, self.iActiveLeader, False)

				infoPanelName = self.getNextWidgetName()
				screen.attachPanel(playerPanelName, infoPanelName, "", "", False, False, PanelStyles.PANEL_STYLE_EMPTY)

				religionName = self.getNextWidgetName()
				szPlayerReligion = ""

				if (nPlayerReligion != -1):

					if (objLoopPlayer.hasHolyCity (nPlayerReligion)):
						szPlayerReligion = u"%c" %(objReligion.getHolyCityChar())
					elif objReligion:
						szPlayerReligion = u"%c" %(objReligion.getChar())

				screen.attachTextGFC(infoPanelName, "", labelText(szPlayerReligion), FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				# advc.004: BULL widget help enabled
				screen.attachTextGFC(infoPanelName, "", labelText(localText.getText("TXT_KEY_FOREIGN_ADVISOR_TRADE", (self.calculateTrade (self.iActiveLeader, iLoopPlayer)[0], ))), FontTypes.GAME_FONT,  WidgetTypes.WIDGET_TRADE_ROUTES, self.iActiveLeader, iLoopPlayer)

				screen.attachTextGFC(infoPanelName, "", labelText(szCivicsPreText), FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

				for nCivicOption in ltCivicOptions:
					nCivic = objLoopPlayer.getCivics (nCivicOption)
					screen.attachImageButton (infoPanelName, "", gc.getCivicInfo (nCivic).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, nCivic, 1, False)

				# <!-- custom: also show favorite religions in the foreign advisor's info tab.
				# Code added with the help of gemini 3 pro thanks, also refactor this part of the code as well for clarity -->
				nFavoriteCivic = objLeaderHead.getFavoriteCivic()
				hasFavoriteCivic = (objLoopPlayer.isFavoriteCivicKnown() and nFavoriteCivic != -1) # advc.130n

				nFavoriteReligion = objLeaderHead.getFavoriteReligion()
				hasFavoriteReligion = (nFavoriteReligion != -1)

				if (hasFavoriteCivic or hasFavoriteReligion):
					screen.attachTextGFC(infoPanelName, "", labelText(szFavoritesPreText), FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

					if hasFavoriteCivic:
						objCivicInfo = gc.getCivicInfo(nFavoriteCivic)
						screen.attachImageButton (infoPanelName, "", objCivicInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, nFavoriteCivic, 1, False)
						# <!-- custom: remove the "(Government)" or "(Economy)" or such types of text as they are distracting and no longer relevant since we have favorite religions as well -->
						#screen.attachTextGFC(infoPanelName, "", "(" + gc.getCivicOptionInfo (objCivicInfo.getCivicOptionType()).getDescription() + ")", FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

					if hasFavoriteReligion:
						objReligionInfo = gc.getReligionInfo(nFavoriteReligion)
						if objReligionInfo:
							screen.attachImageButton(infoPanelName, "", objReligionInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION, nFavoriteReligion, 1, False)

	def drawInfoImproved (self, bInitial):
		screen = self.getScreen()

		# Some spacing variables to help with the layout
		iOutsideGap = 6
		iInsideGap = 10
		iBetweenGap = iOutsideGap - 2
		iHeaderHeight = 32

		# Header
		headerBackgroundPanelName = self.getNextWidgetName()
		iLeft = iOutsideGap
		iTop = 50 + iOutsideGap
		iWidth = self.W_SCREEN - (2 * iOutsideGap)
		iHeight = iHeaderHeight + (2 * iInsideGap)
		screen.addPanel(headerBackgroundPanelName, "", "", True, False, iLeft, iTop, iWidth, iHeight, PanelStyles.PANEL_STYLE_MAIN)

		headerPanelName = self.getNextWidgetName()
		iLeft = iLeft + iInsideGap
		iTop = iTop + iInsideGap
		iWidth = iWidth - (2 * iInsideGap)
		iHeight = iHeaderHeight
		screen.addPanel(headerPanelName, "", "", False, True, iLeft, iTop, iWidth, iHeight, PanelStyles.PANEL_STYLE_EMPTY)

		iOffset = 0

		for headerText in self.headerTexts:
			itemName = self.getNextWidgetName()
			screen.attachTextGFC(headerPanelName, itemName, labelText(headerText), FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			screen.setHitTest(itemName, HitTestTypes.HITTEST_NOHIT)
			iOffset = iOffset + 65

		# Main
		mainBackgroundPanelName = self.getNextWidgetName()
		iLeft = iOutsideGap
		iTop = iTop + iHeaderHeight + iInsideGap + iBetweenGap
		iWidth = self.W_SCREEN - (2 * iOutsideGap)
		iHeight = self.H_SCREEN - 100 - (2 * iOutsideGap) - iBetweenGap - iHeaderHeight - (2 * iInsideGap)
		screen.addPanel(mainBackgroundPanelName, "", "", True, False, iLeft, iTop, iWidth, iHeight, PanelStyles.PANEL_STYLE_MAIN)

		mainPanelName = self.getNextWidgetName()
		iLeft = iLeft + iInsideGap
		iTop = iTop + iInsideGap
		iWidth = iWidth - (2 * iInsideGap)
		iHeight = iHeight - (2 * iInsideGap)
		screen.addPanel(mainPanelName, "", "", True, True, iLeft, iTop, iWidth, iHeight, PanelStyles.PANEL_STYLE_EMPTY)

		FavoriteCivicDetector.doUpdate()

		# display the active player's row at the top
		self.drawInfoRow(screen, mainPanelName, self.iActiveLeader, PanelStyles.PANEL_STYLE_MAIN_BLACK25)

		# loop through all other players and add their rows; show known first
		lKnownPlayers = []
		lUnknownPlayers = []
		# <!-- custom: hoist for performance optimization. -->
		bDebugMode = gc.getGame().isDebugMode()
		for iLoopPlayer in range(gc.getMAX_PLAYERS()):
			if (iLoopPlayer != self.iActiveLeader):
				objLoopPlayer = gc.getPlayer(iLoopPlayer)
				if (self.objActiveTeam.isHasMet(objLoopPlayer.getTeam()) or bDebugMode):
					lKnownPlayers.append(iLoopPlayer)
				else:
					lUnknownPlayers.append(iLoopPlayer)
		for iLoopPlayer in lKnownPlayers:
			self.drawInfoRow(screen, mainPanelName, iLoopPlayer, PanelStyles.PANEL_STYLE_OUT)
		for iLoopPlayer in lUnknownPlayers:
			self.drawInfoRow(screen, mainPanelName, iLoopPlayer, PanelStyles.PANEL_STYLE_OUT)

	def drawInfoRow (self, screen, mainPanelName, iLoopPlayer, ePanelStyle):
		objLoopPlayer = gc.getPlayer(iLoopPlayer)
		iLoopTeam = objLoopPlayer.getTeam()
		objLoopTeam = gc.getTeam(iLoopTeam)
		bIsActivePlayer = (iLoopPlayer == self.iActiveLeader)
		if (objLoopPlayer.isAlive()
			#and (self.objActiveTeam.isHasMet(iLoopTeam) or gc.getGame().isDebugMode())
			and not objLoopPlayer.isBarbarian()
			and not objLoopPlayer.isMinorCiv()):

			objLeaderHead = gc.getLeaderHeadInfo (objLoopPlayer.getLeaderType())
			objAttitude = AttitudeUtil.Attitude(iLoopPlayer, self.iActiveLeader)

			# Player panel
			playerPanelName = self.getNextWidgetName()
			szPlayerLabel = "" # objLoopPlayer.getName()
			screen.attachPanel(mainPanelName, playerPanelName, szPlayerLabel, "", False, True, ePanelStyle)

			# Panels always created but essentially blank if unmet
			itemName = self.getNextWidgetName()
			if (not self.objActiveTeam.isHasMet(iLoopTeam) and not gc.getGame().isDebugMode()):
				if self.szLEADERHEAD_RANDOM is None:
					self.szLEADERHEAD_RANDOM = gc.getDefineSTRING("LEADERHEAD_RANDOM")
				screen.attachImageButton(playerPanelName, itemName, self.szLEADERHEAD_RANDOM, GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_GENERAL, -1, -1, False)
				return
			else:
				screen.attachImageButton(playerPanelName, itemName, objLeaderHead.getButton(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_LEADERHEAD, iLoopPlayer, self.iActiveLeader, False)
			#screen.setHitTest(itemName, HitTestTypes.HITTEST_NOHIT)

			infoPanelName = self.getNextWidgetName()
			screen.attachPanel(playerPanelName, infoPanelName, "", "", False, False, PanelStyles.PANEL_STYLE_EMPTY)

			# Attitude
			itemName = self.getNextWidgetName()
			if (not bIsActivePlayer):
				szAttStr = sasFontTagLabel + objAttitude.getText(True, True, False, False) + SAS_FONT_TAG_CLOSE
			else:
				szAttStr = ""
			# advc.004: BULL widget help enabled
			screen.attachTextGFC(infoPanelName, itemName, szAttStr, FontTypes.GAME_FONT, WidgetTypes.WIDGET_LEADERHEAD_RELATIONS, iLoopPlayer, self.iActiveLeader)
			# Disable the widget if this is active player since it's a blank string.
			if bIsActivePlayer:
				screen.setHitTest(itemName, HitTestTypes.HITTEST_NOHIT)

			# Religion
			itemName = self.getNextWidgetName()
			nReligion = objLoopPlayer.getStateReligion()
			if (nReligion != -1):
				objReligion = gc.getReligionInfo (nReligion)

				if (objLoopPlayer.hasHolyCity (nReligion)):
					szPlayerReligion = u"%c" %(objReligion.getHolyCityChar())
				elif objReligion:
					szPlayerReligion = u"%c" %(objReligion.getChar())

				if (not bIsActivePlayer):
					iDiploModifier = 0
					if (nReligion == self.objActiveLeader.getStateReligion()):
						iDiploModifier = objAttitude.getModifier("TXT_KEY_MISC_ATTITUDE_SAME_RELIGION")
					else:
						iDiploModifier = objAttitude.getModifier("TXT_KEY_MISC_ATTITUDE_DIFFERENT_RELIGION")
					if (iDiploModifier):
						if (iDiploModifier > 0):
							szColor = "COLOR_GREEN"
						else:
							szColor = "COLOR_RED"
						szPlayerReligion = localText.changeTextColor(szPlayerReligion + " [%+d]" % (iDiploModifier), getInfoTypeOrFail(szColor))
				szPlayerReligion = sasFontTagLabel + szPlayerReligion + SAS_FONT_TAG_CLOSE
			else:
				szPlayerReligion = ""
			# advc.004: BULL widget help enabled
			screen.attachTextGFC(infoPanelName, itemName, szPlayerReligion, FontTypes.GAME_FONT, WidgetTypes.WIDGET_LEADERHEAD_RELATIONS, iLoopPlayer, self.iActiveLeader)
			# Disable the widget if this is active player since we don't have diplo info.
			if bIsActivePlayer:
				screen.setHitTest(itemName, HitTestTypes.HITTEST_NOHIT)

			# Trade
			if (bIsActivePlayer or objLoopPlayer.canHaveTradeRoutesWith(self.iActiveLeader)):
				(iTradeCommerce, iTradeRoutes) = self.calculateTrade (self.iActiveLeader, iLoopPlayer)
				# advc.001: Commented out (cf. TradeUtil.py)
				#if TradeUtil.isFractionalTrade():
				#	iTradeCommerce //= 100
				szTradeYield = u"%d %c" % (iTradeCommerce, gc.getYieldInfo(YieldTypes.YIELD_COMMERCE).getChar())
				szTradeRoutes = u"%d" % (iTradeRoutes)
			else:
				szTradeYield = u"-"
				szTradeRoutes = u"-"
			itemName = self.getNextWidgetName()
			# advc.004: BULL widget help enabled (2x)
			screen.attachTextGFC(infoPanelName, itemName, labelText(szTradeRoutes), FontTypes.GAME_FONT, WidgetTypes.WIDGET_TRADE_ROUTES, self.iActiveLeader, iLoopPlayer)
			itemName = self.getNextWidgetName()
			screen.attachTextGFC(infoPanelName, itemName, labelText(szTradeYield), FontTypes.GAME_FONT, WidgetTypes.WIDGET_TRADE_ROUTES, self.iActiveLeader, iLoopPlayer)

			# Civics
			for nCivicOption in range (gc.getNumCivicOptionInfos()):
				nCivic = objLoopPlayer.getCivics (nCivicOption)
				buttonName = self.getNextWidgetName()
				screen.attachImageButton (infoPanelName, buttonName, gc.getCivicInfo (nCivic).getButton(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, nCivic, 1, False)

			# Spacer so Favorite Civics aren't right next to current civics
			screen.attachTextGFC(infoPanelName, "", " ", FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

			# Favorite Civic
			if (not bIsActivePlayer):
				nFavoriteReligion = objLeaderHead.getFavoriteReligion()
				bHasFavoriteReligion = (nFavoriteReligion != -1)
				nFavoriteCivic = objLeaderHead.getFavoriteCivic()
				if FavoriteCivicDetector.isDetectionNecessary():
					objFavorite = FavoriteCivicDetector.getFavoriteCivicInfo(iLoopPlayer)
					if objFavorite.isKnown():
						# We know it. Fall through to standard procedure after setting it.
						nFavoriteCivic = objFavorite.getFavorite()
					else:
						iNumPossibles = objFavorite.getNumPossibles()
						BugUtil.debug("CvForeignAdvisor::drawInfoRows() Number of Guesses: %d" %(iNumPossibles))
						if iNumPossibles > 5:
							# Too many possibilities; display question mark
							screen.attachImageButton (infoPanelName, "", "Art/BUG/QuestionMark.dds", GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_GENERAL, -1, -1, False)
							if bHasFavoriteReligion:
								screen.attachImageButton(infoPanelName, "", gc.getReligionInfo(nFavoriteReligion).getButton(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION, nFavoriteReligion, 1, False)
							return
						else:
							# Loop over possibles and display all
							for nFavoriteCivic in objFavorite.getPossibles():
								objCivicInfo = gc.getCivicInfo (nFavoriteCivic)
								screen.attachImageButton (infoPanelName, "", objCivicInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, nFavoriteCivic, 1, False)
							if bHasFavoriteReligion:
								screen.attachImageButton(infoPanelName, "", gc.getReligionInfo(nFavoriteReligion).getButton(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION, nFavoriteReligion, 1, False)
							return

				if nFavoriteCivic != -1:
					objCivicInfo = gc.getCivicInfo (nFavoriteCivic)
					screen.attachImageButton (infoPanelName, "", objCivicInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, nFavoriteCivic, 1, False)
					if (self.objActiveLeader.isCivic (nFavoriteCivic)):
						iDiploModifier = objAttitude.getModifier("TXT_KEY_MISC_ATTITUDE_FAVORITE_CIVIC")
						if (iDiploModifier):
							if (iDiploModifier > 0):
								szColor = "COLOR_GREEN"
							else:
								szColor = "COLOR_RED"
							szDiplo = sasFontTagLabel + localText.changeTextColor(" [%+d]" % (iDiploModifier), getInfoTypeOrFail(szColor)) + SAS_FONT_TAG_CLOSE
						else:
							szDiplo = ""
						itemName = self.getNextWidgetName()
						# advc.004: BULL widget help enabled
						screen.attachTextGFC(infoPanelName, itemName, szDiplo, FontTypes.GAME_FONT, WidgetTypes.WIDGET_LEADERHEAD_RELATIONS, iLoopPlayer, self.iActiveLeader)
						#screen.setHitTest(itemName, HitTestTypes.HITTEST_NOHIT)
				if bHasFavoriteReligion:
					screen.attachImageButton(infoPanelName, "", gc.getReligionInfo(nFavoriteReligion).getButton(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION, nFavoriteReligion, 1, False)

	def calculateTrade (self, nPlayer, nTradePartner):
		# Trade status...
		iDomesticYield, iDomesticCount, iForeignYield, iForeignCount = TradeUtil.calculateTradeRoutes(nPlayer, nTradePartner)
		return iDomesticYield + iForeignYield, iDomesticCount + iForeignCount

	def drawGlance (self, bInitial):
		# ForeignAdvisorPrint ("Entered drawGlance")

		screen = self.getScreen()

		# <advc.066> Reduce panel height when there are few civs
		self.mainPanelHeight = self.H_SCREEN - 155
		if not gc.getGame().isDebugMode():
			playerActive = gc.getPlayer(self.iActiveLeader)
			hasMetCount = gc.getTeam(playerActive.getTeam()).getHasMetCivCount(True)
			self.mainPanelHeight = min(self.mainPanelHeight, 100 * (hasMetCount + 1))
		# </advc.066>

		# Put everything inside a main panel, so we get vertical scrolling
		headerPanelName = self.getNextWidgetName()
		screen.addPanel(headerPanelName, "", "", True, True, 0, 50, self.W_SCREEN, 60, PanelStyles.PANEL_STYLE_TOPBAR)

		if (bInitial):
			self.initializeGlance()
			self.iSelectedLeader = self.iActiveLeader

		self.drawGlanceHeader(screen, headerPanelName)

		mainPanelName = self.getNextWidgetName()
		# advc.066: The height was self.H_SCREEN - 155, now computed above.

		# <!-- custom: add extra width tweak to remove the yellow side edges, and extra height tweak to remove distracting top bright band plus have more room on bottom side's row as well, with gemini 3 pro's help thanks -->
		# iExtraWidth = 0
		iExtraWidth = 4
		# iExtraHeight = 0
		iExtraHeight = 7

		# <!-- custom: adjust base Y (was 104) as it was a bit too low (as in at the bottom) which led to an asymmetrical display in bottom being too high as compared to top's edge over limit after our changes, vs now it being much more centered which allows to more cleanly/easily adjust iExtraHeight -->
		screen.addPanel(mainPanelName, "", "", True, True, 0 - iExtraWidth, 106 - iExtraHeight, self.W_SCREEN + (2 * iExtraWidth), self.mainPanelHeight + (2 * iExtraHeight), PanelStyles.PANEL_STYLE_MAIN)
		self.drawGlanceRows (screen, mainPanelName, self.iSelectedLeader != self.iActiveLeader, self.iSelectedLeader)

	def initializeGlance (self):
		self.nCount = 0
		self.ltPlayerRelations = [[0] * gc.getMAX_PLAYERS() for i in range (gc.getMAX_PLAYERS())]
		self.ltPlayerMet = [False] * gc.getMAX_PLAYERS()
		for iLoopPlayer in range(gc.getMAX_PLAYERS()):
			# <!-- custom: hoist for performance optimization quite similarly to how gemini 3 pro proposed in a related solution -->
			objLoopPlayer = gc.getPlayer(iLoopPlayer)

			if (objLoopPlayer.isAlive()
			and (gc.getTeam(objLoopPlayer.getTeam()).isHasMet(gc.getPlayer(self.iActiveLeader).getTeam())
			or gc.getGame().isDebugMode())
			and not objLoopPlayer.isBarbarian()
			and not objLoopPlayer.isMinorCiv()
			# <advc.130v> Exclude capitulated vassals. The last check is for AI Auto Play.
			and (not gc.getTeam(objLoopPlayer.getTeam()).isCapitulated()
			or objLoopPlayer.isHuman() or iLoopPlayer == self.iActiveLeader)):
			# </advc.130v>
				# ForeignAdvisorPrint ("Player = %d" % iLoopPlayer)
				self.ltPlayerMet [iLoopPlayer] = True

				for nHost in range(gc.getMAX_PLAYERS()):
					if (gc.getPlayer(nHost).isAlive() and nHost != self.iActiveLeader and (gc.getTeam(gc.getPlayer(nHost).getTeam()).isHasMet(gc.getPlayer(self.iActiveLeader).getTeam()) or gc.getGame().isDebugMode()) and not gc.getPlayer(nHost).isBarbarian() and not gc.getPlayer(nHost).isMinorCiv()):
						nRelation = AttitudeUtil.getAttitudeCount(nHost, iLoopPlayer)
						self.ltPlayerRelations [iLoopPlayer][nHost] = nRelation

				# Player panel
				self.nCount += 1
		# <!-- custom: build ordered column list for pagination (Claude Code Sonnet 4.6) -->
		self.ltGlanceColumns = [i for i in range(gc.getMAX_PLAYERS()) if self.ltPlayerMet[i] and i != self.iActiveLeader]
		if self.GLANCE_MAX_PLAYERS_PER_PAGE is None:
			self.GLANCE_MAX_PLAYERS_PER_PAGE = gc.getDefineINT("SAS_CV_FOREIGN_ADVISOR_GLANCE_MAX_PLAYERS_PER_PAGE")
		nPageCols = min(len(self.ltGlanceColumns), self.GLANCE_MAX_PLAYERS_PER_PAGE)
		self.iGlanceNumPages = max(1, (len(self.ltGlanceColumns) + self.GLANCE_MAX_PLAYERS_PER_PAGE - 1) / self.GLANCE_MAX_PLAYERS_PER_PAGE)
		# <!-- custom: preserve last page across reopens; clamp in case player count changed. (Claude Code Sonnet 4.6) -->
		self.iGlancePage = min(self.iGlancePage, self.iGlanceNumPages - 1)
		# advc.066: Was (self.W_SCREEN - 20) /... I don't think there are margins to account for here.
		# <!-- custom: use nPageCols+1 as denominator; the +1 reserves a left slot matching the original nCount which included the active player who has no column icon. (Claude Code Sonnet 4.6) -->
		self.X_Spread = self.W_SCREEN / max(nPageCols + 1, 1)
		if self.X_Spread < 57 and 57 * (nPageCols + 1) <= self.W_SCREEN:
			self.X_Spread = 57 # advc.066: Lower bound was 58
		# advc.066: Was (self.H_SCREEN - 50) /...
		self.Y_Spread = (self.mainPanelHeight + 105) / (self.nCount + 2)
		self.Y_Text_Offset = (self.Y_Spread - 36) / 2
		if self.Y_Text_Offset < 0:
			self.Y_Text_Offset = 0

	def drawGlanceHeader (self, screen, panelName):
		# <!-- custom: page-aware column drawing with arrow navigation (Claude Code Sonnet 4.6) -->
		iPageStart = self.iGlancePage * self.GLANCE_MAX_PLAYERS_PER_PAGE
		pageColumnSet = set(self.ltGlanceColumns[iPageStart : iPageStart + self.GLANCE_MAX_PLAYERS_PER_PAGE])

		nCount = 1
		for iLoopPlayer in range (gc.getMAX_PLAYERS()):
			if iLoopPlayer not in pageColumnSet:
				continue
			# <!-- custom: hoist for performance optimization quite similarly to how gemini 3 pro proposed in a related solution -->
			currentPlayer = gc.getPlayer(iLoopPlayer)
			szName = self.getNextWidgetName()
			screen.addCheckBoxGFCAt(panelName, szName, gc.getLeaderHeadInfo(currentPlayer.getLeaderType()).getButton(), self.SAS_ART_BUTTON_HILITE_SQUARE, self.X_GLANCE_OFFSET + (self.X_Spread * nCount), self.Y_GLANCE_OFFSET, self.GLANCE_BUTTON_SIZE, self.GLANCE_BUTTON_SIZE, WidgetTypes.WIDGET_LEADERHEAD, iLoopPlayer, self.iActiveLeader, ButtonStyles.BUTTON_STYLE_LABEL, False)
			if (self.iSelectedLeader == iLoopPlayer):
				screen.setState(szName, True)
			else:
				screen.setState(szName, False)
			nCount += 1

		self.glanceLeftArrowID = ""
		self.glanceRightArrowID = ""
		if self.iGlanceNumPages > 1:
			iArrowSize = 24
			iArrowY = 50 + (60 - iArrowSize) / 2
			self.glanceLeftArrowID = self.getNextWidgetName()
			screen.setButtonGFC(self.glanceLeftArrowID, u"", "", 5, iArrowY, iArrowSize, iArrowSize, WidgetTypes.WIDGET_GENERAL, -1, -1, ButtonStyles.BUTTON_STYLE_ARROW_LEFT)
			screen.enable(self.glanceLeftArrowID, self.iGlancePage > 0)
			self.glanceRightArrowID = self.getNextWidgetName()
			screen.setButtonGFC(self.glanceRightArrowID, u"", "", self.W_SCREEN - iArrowSize - 5, iArrowY, iArrowSize, iArrowSize, WidgetTypes.WIDGET_GENERAL, -1, -1, ButtonStyles.BUTTON_STYLE_ARROW_RIGHT)
			screen.enable(self.glanceRightArrowID, self.iGlancePage < self.iGlanceNumPages - 1)

	def drawGlanceRows (self, screen, mainPanelName, bSorted = False, nPlayer = 1):
		# ForeignAdvisorPrint ("MAX Players = %d" % gc.getMAX_PLAYERS())
		# <!-- custom: restrict columns to current page (Claude Code Sonnet 4.6) -->
		iPageStart = self.iGlancePage * self.GLANCE_MAX_PLAYERS_PER_PAGE
		pageColumnSet = set(self.ltGlanceColumns[iPageStart : iPageStart + self.GLANCE_MAX_PLAYERS_PER_PAGE])
		ltSortedRelations = [(None,-1)] * gc.getMAX_PLAYERS()
		self.loadColIntoList (self.ltPlayerRelations, ltSortedRelations, nPlayer)
		if bSorted:
			ltSortedRelations.sort()
			if (self.bGlancePlus):
				ltSortedRelations.reverse()
			self.bGlancePlus = not self.bGlancePlus
		else:
			# If not sorted, we take the original ID list and move active player to the front.
			#ltSortedRelations = map(lambda x: (0, x), range(gc.getMAX_PLAYERS()))
			nFirstElement = self.ltPlayerRelations[self.iActiveLeader][nPlayer]
			ltSortedRelations.remove((nFirstElement, self.iActiveLeader))
			ltSortedRelations.insert(0, (nFirstElement, self.iActiveLeader))

		# loop through all players and display leaderheads
		for nOffset in range (gc.getMAX_PLAYERS()):
			if ltSortedRelations[nOffset][1] != -1:
				break

		for i in range (self.nCount):
			iLoopPlayer = ltSortedRelations[nOffset + i][1]
			# ForeignAdvisorPrint ("iLoopPlayer = %d" % iLoopPlayer)

			# <!-- custom: hoist for performance optimization quite similarly to how gemini 3 pro proposed in a related solution -->
			currentPlayer = gc.getPlayer(iLoopPlayer)

			playerPanelName = self.getNextWidgetName()
			if iLoopPlayer == self.iActiveLeader:
				screen.attachPanel(mainPanelName, playerPanelName, "", "", False, True, PanelStyles.PANEL_STYLE_MAIN_BLACK50)
			else:
				screen.attachPanel(mainPanelName, playerPanelName, "", "", False, True, PanelStyles.PANEL_STYLE_MAIN_BLACK25)

			nCount = 1

			# <!-- custom: This is extra info by gemini 3 pro; check if accurate -->
			# Inner Loop: Draw the columns
			# iLoopPlayer = The Row Leader (You, or an AI)
			# j           = The Column Leader (The Rival)

			for j in range (gc.getMAX_PLAYERS()):
				if self.ltPlayerMet[j] and j in pageColumnSet:
					# <!-- custom: This is extra info by gemini 3 pro; check if accurate -->
					# This line HIDES the column for the Human Player (You), which matches your screenshot.
					# <!-- custom: pageColumnSet already excludes the active player so j != self.iActiveLeader is always true here; kept for clarity. (Claude Code Sonnet 4.6) -->
					if j != self.iActiveLeader:
						szName = self.getNextWidgetName()
						nAttitude = self.ltPlayerRelations[iLoopPlayer][j]

						# <!-- custom: This is extra info by gemini 3 pro; check if accurate -->
						# 1. Base Attitude Text (Smilies/Numbers)
						# <!-- custom: leave diagonal Glance cells empty: a player-vs-itself cell is not a real attitude modifier against another player.
						# Rendering the matrix default as +0 was misleading, and in Both mode it was inconsistent with real +0 relation cells that also show an attitude icon. See KI#140. (GPT-5.5) -->
						bSelfRelationCell = (j == iLoopPlayer)
						if bSelfRelationCell:
							szText = ""
						elif nAttitude is not None:
							szText = AttitudeUtil.getAttitudeText(j, iLoopPlayer, AdvisorOpt.isShowGlanceNumbers(), AdvisorOpt.isShowGlanceSmilies(), True, True, AdvisorOpt.isShowGlanceWarTrades()) # advc.152: WarTrades added
						else:
							szText = ""

						# <!-- custom: add "Willing to become a vassal" type of button as it is useful for the human player to see it in UI in the glances tab.
						# Also for information or exhaustiveness the human player occupies the first row and has no column in the glances tab it seems if i'm not mistaken.
						# Added with the combined help of claude sonnet 4.5 and gemini pro 3, since neither could get it right on their own or fast enough; claude helped most but gemini too and me too i mean thanks to me too.
						# I fed claude the C++ code of int CvTeamAI::AI_vassalTradeVal, DenialTypes CvTeamAI::AI_vassalTrade, int CvTeamAI::AI_surrenderTradeVal, and DenialTypes CvTeamAI::AI_surrenderTrade, since its previous python implementation didn't work.
						# Check if accurate -->
						# Goal: If this is MY Row (iLoopPlayer is me), check if the RIVAL (j) wants to be my vassal.
						# Vassal Check (Human Row Edition)
						if iLoopPlayer == self.iActiveLeader:
							# Define roles for this specific check
							pPotentialVassal = gc.getPlayer(j)           # The Rival (Column)
							# pPotentialMaster = currentPlayer           # Me (Row)

							# A. Are they ALREADY our vassal?
							if gc.getTeam(pPotentialVassal.getTeam()).isVassal(currentPlayer.getTeam()):
								szText += self.VASSAL_ICON

							# B. Are they WILLING to become our vassal?
							else:
								tradeData = TradeData()

								# Check 1: Peaceful Vassal
								tradeData.ItemType = TradeableItems.TRADE_VASSAL

								# Can we trade it? AND Is there NO denial?
								# Note: We ask if *pPotentialVassal* can trade *Vassal State* to *Us*
								if pPotentialVassal.canTradeItem(iLoopPlayer, tradeData, False):
									if pPotentialVassal.getTradeDenial(iLoopPlayer, tradeData) == DenialTypes.NO_DENIAL:
										szText += self.WILLING_VASSAL_ICON

								# Check 2: Capitulation (Surrender during war)
								# Only check if they are at war with us
								elif gc.getTeam(pPotentialVassal.getTeam()).isAtWar(currentPlayer.getTeam()):
									tradeData.ItemType = TradeableItems.TRADE_SURRENDER
									if pPotentialVassal.canTradeItem(iLoopPlayer, tradeData, False):
										if pPotentialVassal.getTradeDenial(iLoopPlayer, tradeData) == DenialTypes.NO_DENIAL:
											szText += self.WILLING_VASSAL_ICON
						# End - Vassal Check (Human Row Edition)

						# <advc.152>
						widgType = WidgetTypes.WIDGET_LEADERHEAD_RELATIONS
						if bSelfRelationCell:
							widgType = WidgetTypes.WIDGET_GENERAL
						elif AdvisorOpt.isShowGlanceWarTrades():
							widgType = WidgetTypes.WIDGET_LH_GLANCE
						# </advc.152>
						# <!-- custom: when trailing status icons >= 2 (e.g., +11 or -8 with WE + war/peace/war-trade icons), keep that trailing icon run at fixed font=2 to avoid last-glyph clipping at larger label-font scales (this was observed to also happen for single-digit attitudes, so we do not gate on the number of attitude digits). Keep the rest of the cell text at scaled label font. See KI#116. (GPT-5.3-Codex) -->
						szDisplayText = labelText(szText)
						if szText:
							iLastSpace = szText.rfind(u" ")
							if iLastSpace != -1:
								szTail = szText[iLastSpace + 1:]
								# <!-- custom: we use the final tail text shape here instead of a separate icon counter because icons are assembled across multiple code paths (AttitudeUtil + local appends), and this keeps the workaround local/simple. (GPT-5.4? + GPT-5.3-Codex) -->
								bHasPlainTrailingIcons = (szTail and szTail.find(u"<") == -1 and (not szTail[0].isdigit()) and szTail[0] != u"+" and szTail[0] != u"-")
								iTrailingIconCount = len(szTail)
								if bHasPlainTrailingIcons and iTrailingIconCount >= 2:
									szHead = szText[:iLastSpace]
									iLabelFont = getSASUIFontLabel()
									if szHead:
										szDisplayText = u"<font=%d>%s</font> <font=2>%s</font>" % (iLabelFont, szHead, szTail)
									else:
										szDisplayText = u"<font=2>%s</font>" % szTail
						screen.setTextAt (szName, playerPanelName, szDisplayText, CvUtil.FONT_CENTER_JUSTIFY, self.X_GLANCE_OFFSET - 2 + (self.X_Spread * nCount), self.Y_GLANCE_OFFSET + self.Y_Text_Offset, -0.1, FontTypes.GAME_FONT, widgType, j, iLoopPlayer)
						nCount += 1

			if nCount > 8:
				screen.attachImageButton(playerPanelName, "", gc.getLeaderHeadInfo(currentPlayer.getLeaderType()).getButton(), GenericButtonSizes.BUTTON_SIZE_32, WidgetTypes.WIDGET_LEADERHEAD, iLoopPlayer, self.iActiveLeader, False)
			else:
				screen.attachImageButton(playerPanelName, "", gc.getLeaderHeadInfo(currentPlayer.getLeaderType()).getButton(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_LEADERHEAD, iLoopPlayer, self.iActiveLeader, False)

	def loadColIntoList (self, ltPlayers, ltTarget, nCol):
		nCount = 0
		for i in range (len (ltTarget)):
			if (self.ltPlayerMet[i]):
				# ForeignAdvisorPrint ("player met = %d; nCount = %d" % (i, nCount))
				ltTarget[nCount] = (ltPlayers[i][nCol], i)
				nCount += 1

	def handlePlusMinusToggle (self):
		# ForeignAdvisorPrint ("Entered handlePlusMinusToggle")

		self.bGlancePlus = not self.bGlancePlus
		self.drawContents (False)

	############################################
	### BEGIN CHANGES ENHANCED INTERFACE MOD ###
	############################################

	def initTradeTable(self):
		screen = self.getScreen()
		# advc.073: Moved the first of the two IconGrid_BUG.GRID_TEXT_COLUMN (GPT that the AI will pay) three positions back in both branches
		if (self.RES_SHOW_ACTIVE_TRADE):
			columns = ( IconGrid_BUG.GRID_ICON_COLUMN, IconGrid_BUG.GRID_MULTI_LIST_COLUMN, IconGrid_BUG.GRID_TEXT_COLUMN, IconGrid_BUG.GRID_MULTI_LIST_COLUMN, IconGrid_BUG.GRID_MULTI_LIST_COLUMN, IconGrid_BUG.GRID_MULTI_LIST_COLUMN, IconGrid_BUG.GRID_MULTI_LIST_COLUMN, IconGrid_BUG.GRID_MULTI_LIST_COLUMN, IconGrid_BUG.GRID_TEXT_COLUMN )
		else:
			columns = ( IconGrid_BUG.GRID_ICON_COLUMN, IconGrid_BUG.GRID_MULTI_LIST_COLUMN, IconGrid_BUG.GRID_TEXT_COLUMN, IconGrid_BUG.GRID_MULTI_LIST_COLUMN, IconGrid_BUG.GRID_MULTI_LIST_COLUMN, IconGrid_BUG.GRID_MULTI_LIST_COLUMN )
		self.NUM_RESOURCE_COLUMNS = len(columns) - 1

		# <advc> Intermediate values leftSpace and topSpace added
		# <!-- custom: we don't need all this yellow empty space, better use it for our blue panel and grids or such to show more information -->
		# leftSpace = self.MIN_LEFT_RIGHT_SPACE + 10
		# topSpace = self.MIN_TOP_BOTTOM_SPACE + 10
		leftSpace = 0
		topSpace = self.MIN_TOP_BOTTOM_SPACE

		gridX = leftSpace
		# <!-- custom: not sure how this all works, some changes don't seem effective at all, manually adjusting as such empirically.
		# and randomly as such somehow seemed to land it right -->
		# gridY = topSpace + self.RES_SURPLUS_HEIGHT + self.RES_PANEL_SPACE + self.TITLE_HEIGHT
		gridY = topSpace + self.RES_SURPLUS_HEIGHT + self.RES_PANEL_SPACE + self.TITLE_HEIGHT - 60
		gridWidth = self.W_SCREEN - gridX - leftSpace
		# <!-- custom: adjust empirically height as it now overfills on bottom -->
		gridHeight = self.H_SCREEN - gridY - topSpace
		# </advc>

		self.resIconGridName = self.getNextWidgetName()
		self.resIconGrid = IconGrid_BUG.IconGrid_BUG( self.resIconGridName, screen, gridX, gridY, gridWidth, gridHeight, columns, True, self.SHOW_LEADER_NAMES, self.SHOW_ROW_BORDERS )

		self.resIconGrid.setGroupBorder(self.GROUP_BORDER)
		self.resIconGrid.setGroupLabelOffset(self.GROUP_LABEL_OFFSET)
		self.resIconGrid.setMinColumnSpace(self.MIN_COLUMN_SPACE)
		self.resIconGrid.setMinRowSpace(self.MIN_ROW_SPACE)

		self.leaderCol = 0
		#self.surplusCol = 1
		#self.usedCol = 2
		#self.willTradeCol = 3
		#self.wontTradeCol = 4
		#self.canPayCol = 5
		# <advc.073>
		self.willImportCol = 1
		self.canPayCol = 2
		self.willExportCol = 3
		self.wontExportCol = 4
		self.noNeedCol = 5
		# </advc.073>
		self.activeExportCol = 6
		self.activeImportCol = 7
		self.payingCol = 8

		# <!-- custom: use cached text values for performance (claude code sonnet 4.5) -->
		self.resIconGrid.setHeader( self.leaderCol, labelText(self.TEXT_LEADER) )
		# <advc.073>
		# txt keys were TXT_KEY_FOREIGN_ADVISOR_FOR_TRADE_2, TXT_KEY_FOREIGN_ADVISOR_NOT_FOR_TRADE_2, TXT_KEY_FOREIGN_ADVISOR_FOR_TRADE_2, TXT_KEY_FOREIGN_ADVISOR_NOT_FOR_TRADE_2
		# was surplusCol
		self.resIconGrid.setHeader( self.willImportCol, labelText(self.TEXT_WILL_IMPORT) )
		# Moved up
		self.resIconGrid.setHeader( self.canPayCol, labelText(u"%c" % gc.getCommerceInfo(CommerceTypes.COMMERCE_GOLD).getChar()) )
		self.resIconGrid.setTextColWidth(self.canPayCol, self.RES_GOLD_COL_WIDTH)
		# Replaced with noNeedCol below
		#self.resIconGrid.setHeader( self.usedCol, localText.getText("TXT_KEY_FOREIGN_ADVISOR_WILL_NOT_IMPORT", ()) )
		# was willTradeCol
		self.resIconGrid.setHeader( self.willExportCol, labelText(self.TEXT_WILL_EXPORT) )
		# was wontTradeCol
		self.resIconGrid.setHeader( self.wontExportCol, labelText(self.TEXT_WILL_NOT_EXPORT) )
		# New column that takes over most of the wontTradeCol resources
		self.resIconGrid.setHeader( self.noNeedCol, labelText(self.TEXT_NO_NEED) )
		# </advc.073>

		if (self.RES_SHOW_ACTIVE_TRADE):
			# advc.073: was TXT_KEY_FOREIGN_ADVISOR_EXPORT. Now all the headings take the perspective of the foreign leader (except noNeed)
			self.resIconGrid.setHeader( self.activeExportCol, labelText(self.TEXT_IMPORTING) )
			# advc.073: was TXT_KEY_FOREIGN_ADVISOR_IMPORT
			self.resIconGrid.setHeader( self.activeImportCol, labelText(self.TEXT_EXPORTING) )
			self.resIconGrid.setHeader( self.payingCol, labelText(u"%c" % gc.getCommerceInfo(CommerceTypes.COMMERCE_GOLD).getChar()) )
			self.resIconGrid.setTextColWidth(self.payingCol, self.RES_GOLD_COL_WIDTH)

		if (self.RES_SHOW_IMPORT_EXPORT_HEADER):
			self.resIconGrid.createColumnGroup(" ", 1)
			#self.resIconGrid.createColumnGroup(localText.getText("TXT_KEY_FOREIGN_ADVISOR_EXPORT", ()), 2)
			# advc.073: Replacing the above. (I want the groups, but not the headings.)
			self.resIconGrid.createColumnGroup(" ", 2)
			#self.resIconGrid.createColumnGroup(localText.getText("TXT_KEY_FOREIGN_ADVISOR_IMPORT", ()), 3)
			# advc.073: Replacing the above
			self.resIconGrid.createColumnGroup(" ", 3)
			if (self.RES_SHOW_ACTIVE_TRADE):
				#self.resIconGrid.createColumnGroup(localText.getText("TXT_KEY_FOREIGN_ADVISOR_ACTIVE", ()), 3)
				# advc.073: Replacing the above
				self.resIconGrid.createColumnGroup(" ", 3)

		# <!-- custom: keep the bonus tab aligned to runtime advisor bounds (not centered on preferred grid size) so high resolutions don't create narrow/offset panels.
		# Use slightly tighter inner padding/margins so more width/height is used. (GPT-5.3-Codex) -->
		# <!-- custom: let the bonus-tab blue panels extend a few pixels further to remove leftover side gutters at higher resolutions. (GPT-5.3-Codex) -->
		self.RES_LEFT_RIGHT_SPACE = self.MIN_LEFT_RIGHT_SPACE - 5
		self.RES_TOP_BOTTOM_SPACE = max(22, self.MIN_TOP_BOTTOM_SPACE - 18)
		gridX = self.RES_LEFT_RIGHT_SPACE + 6
		gridY = self.RES_TOP_BOTTOM_SPACE + self.RES_SURPLUS_HEIGHT + self.RES_PANEL_SPACE + self.TITLE_HEIGHT + 6
		gridWidth = self.W_SCREEN - 2 * gridX
		gridHeight = self.H_SCREEN - gridY - self.RES_TOP_BOTTOM_SPACE - 6
		self.resIconGrid.setPosition(gridX, gridY)
		self.resIconGrid.setSize(gridWidth, gridHeight)

	def calculateSurplusPanelLayout(self):
		self.SURPLUS_X = self.RES_LEFT_RIGHT_SPACE
		self.SURPLUS_Y = self.RES_TOP_BOTTOM_SPACE
		self.SURPLUS_WIDTH = self.W_SCREEN - 2 * self.RES_LEFT_RIGHT_SPACE

		self.SURPLUS_ICONS_X = self.SURPLUS_X + 10
		if (self.RES_SHOW_SURPLUS_AMOUNT_ON_TOP):
			self.SURPLUS_TABLE_X = self.SURPLUS_ICONS_X + 15
			# advc.073: Take RESOURCE_ICON_SIZE times 2 (two rows now)
			SURPLUS_VERTICAL_SPACING = (self.RES_SURPLUS_HEIGHT - 2 * self.RESOURCE_ICON_SIZE - self.TITLE_HEIGHT) / 2
			self.SURPLUS_ICONS_Y = self.SURPLUS_Y + SURPLUS_VERTICAL_SPACING + self.TITLE_HEIGHT
			self.SURPLUS_TABLE_Y = self.SURPLUS_ICONS_Y + (self.RESOURCE_ICON_SIZE - self.TABLE_CONTROL_HEIGHT) / 2 + 8
		else:
			self.SURPLUS_TABLE_X = self.SURPLUS_ICONS_X + 5
			SURPLUS_VERTICAL_SPACING = ( self.RES_SURPLUS_HEIGHT - self.RESOURCE_ICON_SIZE - self.TABLE_CONTROL_HEIGHT - self.TITLE_HEIGHT ) / 2 + 3
			self.SURPLUS_ICONS_Y = self.SURPLUS_Y + SURPLUS_VERTICAL_SPACING + self.TITLE_HEIGHT
			self.SURPLUS_TABLE_Y = self.SURPLUS_ICONS_Y + self.RESOURCE_ICON_SIZE

		self.SURPLUS_CIRCLE_X_START = self.SURPLUS_TABLE_X + 4
		# advc.073: Renamed from SURPLUS_CIRCLE_Y
		self.SURPLUS_CIRCLE_Y_START = self.SURPLUS_TABLE_Y + 5

	def drawResourceDeals(self, bInitial):
		screen = self.getScreen()

		activePlayer = gc.getPlayer(self.iActiveLeader)
		self.initTradeTable()

		# Find all the surplus resources
		tradeData = TradeData()
		tradeData.ItemType = TradeableItems.TRADE_RESOURCES
		listSurplus = []
		listNonSurplus = [] # advc.073

		for iLoopBonus in range(gc.getNumBonusInfos()):
			bSurplus = False # advc.073
			tradeData.iData = iLoopBonus
			for iLoopPlayer in range(gc.getMAX_PLAYERS()):
				currentPlayer = gc.getPlayer(iLoopPlayer)
				if not currentPlayer.isAlive() or currentPlayer.isBarbarian() or currentPlayer.isMinorCiv() or not gc.getTeam(currentPlayer.getTeam()).isHasMet(activePlayer.getTeam()) or iLoopPlayer == self.iActiveLeader or not activePlayer.canTradeItem(iLoopPlayer, tradeData, False):
					continue # advc
				if activePlayer.getNumTradeableBonuses(iLoopBonus) > 1:
					listSurplus.append(iLoopBonus)
					bSurplus = True # advc.073
					break
			# <advc.073>
			if not bSurplus and activePlayer.getNumTradeableBonuses(iLoopBonus) > 0:
				listNonSurplus.append(iLoopBonus)
			# </advc.073>
		self.calculateSurplusPanelLayout()

		# Assemble the surplus panel
		self.mainAvailablePanel = self.getNextWidgetName()
		# <!-- custom: save some space, don't use a header. -->
		# screen.addPanel( self.mainAvailablePanel, localText.getText("TXT_KEY_FOREIGN_ADVISOR_SURPLUS_RESOURCES", ()), "", False, False, self.SURPLUS_X, self.SURPLUS_Y, self.SURPLUS_WIDTH, self.RES_SURPLUS_HEIGHT, PanelStyles.PANEL_STYLE_MAIN )
		screen.addPanel( self.mainAvailablePanel, "", "", False, False, self.SURPLUS_X, self.SURPLUS_Y, self.SURPLUS_WIDTH, self.RES_SURPLUS_HEIGHT, PanelStyles.PANEL_STYLE_MAIN )

		self.availableMultiList = self.getNextWidgetName()
		# advc.073: I don't know how to make the surplus amounts wrap into another row, so the surplus resources will all have to be placed in the top row, even if there isn't enough space.
		maxIconsPerRow = max(len(listSurplus), self.SURPLUS_WIDTH // self.RESOURCE_ICON_SIZE)
		# advc.073: Width based on max of listSurplus and listNonSurplus; height times 2.
		screen.addMultiListControlGFC( self.availableMultiList, "", self.SURPLUS_ICONS_X, self.SURPLUS_ICONS_Y, self.RESOURCE_ICON_SIZE * min(maxIconsPerRow,len(listSurplus)+len(listNonSurplus)), 2 * self.RESOURCE_ICON_SIZE, 1, 32, 32, TableStyles.TABLE_STYLE_EMPTY )

		self.availableTable = self.getNextWidgetName()
		# add the circles behind the amounts
		if (self.RES_SHOW_SURPLUS_AMOUNT_ON_TOP):
			for iIndex in range(len(listSurplus)):
				# advc.073: SURPLUS_CIRCLE_Y replaced with SURPLUS_CIRCLE_Y_START plus an offset.
				circleY = self.SURPLUS_CIRCLE_Y_START
				# Can't do the same for the amounts, so don't do this after all:
				#if iIndex > maxIconsPerRow:
				#	circleY += self.RESOURCE_ICON_SIZE
				screen.addDDSGFC( self.availableTable + "Circle" + str(iIndex), self.SAS_ART_WHITE_CIRCLE_40, self.SURPLUS_CIRCLE_X_START + iIndex * self.RESOURCE_ICON_SIZE, circleY, 16, 16, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		# add the table showing the amounts
		screen.addTableControlGFC( self.availableTable, len(listSurplus), self.SURPLUS_TABLE_X, self.SURPLUS_TABLE_Y, len(listSurplus) * self.RESOURCE_ICON_SIZE, self.TABLE_CONTROL_HEIGHT, False, False, 16, 16, TableStyles.TABLE_STYLE_EMPTY )

		# Add the bonuses to the surplus panel with their amount
		for iIndex in range(len(listSurplus)):
			# screen.addCheckBoxGFCAt (self.mainAvailablePanel, "Foo" + str(iIndex), gc.getBonusInfo (listSurplus[iIndex]).getButton(), ArtFileMgr.getInterfaceArtInfo ("BUTTON_HILITE_SQUARE").getPath(), self.X_GLANCE_OFFSET + (self.RESOURCE_ICON_SIZE * iIndex), 10, 32, 32, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, listSurplus[iIndex], -1, ButtonStyles.BUTTON_STYLE_LABEL, False)
			# advc.073: Pass the active player to the BULL widget in order to signal that all takers are supposed to be listed
			screen.appendMultiListButton( self.availableMultiList, gc.getBonusInfo(listSurplus[iIndex]).getButton(), 0, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS_TRADE, listSurplus[iIndex], self.iActiveLeader, False )
			screen.setTableColumnHeader( self.availableTable, iIndex, u"", self.RESOURCE_ICON_SIZE )

			amount = activePlayer.getNumTradeableBonuses(listSurplus[iIndex])
			if (self.RES_SHOW_EXTRA_AMOUNT):
				amount = amount - 1

			if (self.RES_SHOW_SURPLUS_AMOUNT_ON_TOP):
				amountStr = sasFontTagLabel + localText.changeTextColor(str(amount), self.COLOR_YELLOW) + SAS_FONT_TAG_CLOSE
			else:
				amountStr = sasFontTagLabel + str(amount) + SAS_FONT_TAG_CLOSE
			screen.setTableText( self.availableTable, iIndex, 0, amountStr, "", WidgetTypes.WIDGET_GENERAL, -1, -1, 0 )
		# <advc.073>
		for iIndex in range(len(listNonSurplus)):
			screen.appendMultiListButton(self.availableMultiList, gc.getBonusInfo(listNonSurplus[iIndex]).getButton(), 0, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS_TRADE, listNonSurplus[iIndex], self.iActiveLeader, False)
			screen.setTableColumnHeader(self.availableTable, iIndex, u"", self.RESOURCE_ICON_SIZE)
		# </advc.073>

		# # Assemble the panel that shows the trade table
		# <!-- custom: beautify, trim or adjust the edges to remove empty space or overfilling blue panel or such -->
		# iExtraY = 0
		# iExtraHeight = 0
		iExtraY = - 20
		iExtraHeight = -12
		self.TABLE_PANEL_X = self.RES_LEFT_RIGHT_SPACE
		self.TABLE_PANEL_Y = self.SURPLUS_Y + self.RES_SURPLUS_HEIGHT + self.RES_PANEL_SPACE + iExtraY
		self.TABLE_PANEL_WIDTH = self.W_SCREEN - 2 * self.RES_LEFT_RIGHT_SPACE
		self.TABLE_PANEL_HEIGHT = self.H_SCREEN - self.TABLE_PANEL_Y - self.RES_TOP_BOTTOM_SPACE + iExtraHeight

		self.tradePanel = self.getNextWidgetName()
		# <!-- custom: save some space, don't use a header. -->
		# screen.addPanel( self.tradePanel, localText.getText("TXT_KEY_FOREIGN_ADVISOR_TRADE_TABLE", ()), "", True, True, self.TABLE_PANEL_X, self.TABLE_PANEL_Y, self.TABLE_PANEL_WIDTH, self.TABLE_PANEL_HEIGHT, PanelStyles.PANEL_STYLE_MAIN )
		screen.addPanel( self.tradePanel, "", "", True, True, self.TABLE_PANEL_X, self.TABLE_PANEL_Y, self.TABLE_PANEL_WIDTH, self.TABLE_PANEL_HEIGHT, PanelStyles.PANEL_STYLE_MAIN )

		self.resIconGrid.createGrid()

		# find all players that need to be listed 
		self.resIconGrid.clearData()
		tradeData = TradeData()
		tradeData.ItemType = TradeableItems.TRADE_RESOURCES
		currentRow = 0

		for iLoopPlayer in range(gc.getMAX_PLAYERS()):
			currentPlayer = gc.getPlayer(iLoopPlayer)
			if ( currentPlayer.isAlive() and not currentPlayer.isBarbarian() and not currentPlayer.isMinorCiv() and gc.getTeam(currentPlayer.getTeam()).isHasMet(activePlayer.getTeam()) and iLoopPlayer != self.iActiveLeader ):
				message = ""
				if ( not activePlayer.canTradeNetworkWith(iLoopPlayer) ):
					# <!-- custom: unlike base AdvCiv, keep showing GPT here even when not connected to trade network (still gated on isGoldTrading / Currency tech), to avoid opening Diplomacy each turn just to see it.
					# Side effect: the pre-existing "Not Connected" overlay then collides with the GPT number, so prepend tabs to push it past the GPT column. See KI#132. (claude code opus 4.7) -->
					message = "\t\t" + self.TEXT_NOT_CONNECTED

				self.resIconGrid.appendRow(labelText(currentPlayer.getName()), labelText(message))
				self.resIconGrid.addIcon( currentRow, self.leaderCol, gc.getLeaderHeadInfo(currentPlayer.getLeaderType()).getButton(), 64, WidgetTypes.WIDGET_LEADERHEAD, iLoopPlayer, self.iActiveLeader )

				# gold
				# advc.036:
				bWillTalk = currentPlayer.AI_isWillingToTalk(self.iActiveLeader)
				if (gc.getTeam(activePlayer.getTeam()).isGoldTrading() or gc.getTeam(currentPlayer.getTeam()).isGoldTrading()) and bWillTalk:
					# <!-- custom: looks like gc.getPlayer(iLoopPlayer) could be optimized with the cached currentPlayer variable so did as such -->
					sAmount = str(currentPlayer.AI_maxGoldPerTurnTrade(self.iActiveLeader))
					self.resIconGrid.setText(currentRow, self.canPayCol, labelText(sAmount))

				# bonuses
				importFromPlayer = [] # advc.036
				for iLoopBonus in range(gc.getNumBonusInfos()):
					tradeData.iData = iLoopBonus
					if (activePlayer.canTradeItem(iLoopPlayer, tradeData, False)):
						if (activePlayer.canTradeItem(iLoopPlayer, tradeData, (not currentPlayer.isHuman()))): # surplus
							importFromPlayer.append(iLoopBonus) # advc.036
						else: # used
							# advc.073: Use the BONUS_TRADE widget from BULL so that the DLL can tell that tile yields shouldn't be shown
							#self.resIconGrid.addIcon( currentRow, self.usedCol, gc.getBonusInfo(iLoopBonus).getButton(), 64, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS_TRADE, iLoopBonus, iLoopPlayer )
							pass # Or rather: Disable this column
					if (currentPlayer.canTradeItem(self.iActiveLeader, tradeData, False)):
						#if (currentPlayer.canTradeItem(self.iActiveLeader, tradeData, (not currentPlayer.isHuman()))): # will trade
						# <advc.073> Replacing the above: separate column now for must-be-joking resources
						iDenial = currentPlayer.getTradeDenial(self.iActiveLeader, tradeData)
						# Human isn't going to deny, but I still want to populate the noNeedCol and wontExportCol.
						if iDenial == DenialTypes.NO_DENIAL and currentPlayer.isHuman():
							if activePlayer.getNumAvailableBonuses(iLoopBonus) > 0 and activePlayer.AI_corporationBonusVal(iLoopBonus) <= 0:
								iDenial = DenialTypes.DENIAL_JOKING
							elif currentPlayer.getNumAvailableBonuses(iLoopBonus) < 2 or currentPlayer.AI_corporationBonusVal(iLoopBonus) > 0:
								iDenial = DenialTypes.DENIAL_NO_GAIN
						if iDenial != DenialTypes.DENIAL_JOKING:
							if iDenial == DenialTypes.NO_DENIAL and bWillTalk:
								# Use the BONUS_TRADE widget from BULL everywhere (also needed for advc.036). Was willTradeCol.
								self.resIconGrid.addIcon( currentRow, self.willExportCol, gc.getBonusInfo(iLoopBonus).getButton(), 64, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS_TRADE, iLoopBonus, iLoopPlayer )
							else: # won't trade
								# advc.073: Changed so that WIDGET_PEDIA_JUMP_TO_BONUS_TRADE works w/o BugDll. And was wontTradeCol.
								self.resIconGrid.addIcon( currentRow, self.wontExportCol, gc.getBonusInfo(iLoopBonus).getButton(), 64, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS_TRADE, iLoopBonus, iLoopPlayer )
						else: # New column for resources that the active player doesn't need
							self.resIconGrid.addIcon(currentRow, self.noNeedCol, gc.getBonusInfo(iLoopBonus).getButton(), 64, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS_TRADE, iLoopBonus, iLoopPlayer)
						# </advc.073>
				# <advc.036> Sorting both listSurplus and importSurplusFromPlayer by the number of copies owned by the active player would be even better, but would take me too long to write in Python.
				importSurplusFromPlayer = []
				rest = []
				for iBonus in importFromPlayer:
					if iBonus in listSurplus:
						importSurplusFromPlayer.append(iBonus)
					else:
						rest.append(iBonus)
				importSorted = importSurplusFromPlayer + rest
				for iLoopBonus in importSorted:
					# Cut and pasted from the "bonuses" loop above, but using the BONUS_TRADE widget from BULL.
					# advc.073: Was surplusCol; and add 1000 to iLoopBonus in order to signal that we're in the import column (hack).
					self.resIconGrid.addIcon( currentRow, self.willImportCol, gc.getBonusInfo(iLoopBonus).getButton(), 64, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS_TRADE, iLoopBonus+1000, iLoopPlayer )
				# </advc.036>
				if (self.RES_SHOW_ACTIVE_TRADE):
					amount = 0
					for iLoopDeal in range(gc.getGame().getIndexAfterLastDeal()):
						deal = gc.getGame().getDeal(iLoopDeal)
						# BUG - Kill Deal - start
						if not deal.isNone():
							if ( deal.getFirstPlayer() == iLoopPlayer and deal.getSecondPlayer() == self.iActiveLeader):
								for iLoopTradeItem in range(deal.getLengthFirstTrades()):
									tradeData2 = deal.getFirstTrade(iLoopTradeItem)
									if (tradeData2.ItemType == TradeableItems.TRADE_GOLD_PER_TURN):
										amount += tradeData2.iData
									if (tradeData2.ItemType == TradeableItems.TRADE_RESOURCES):
										# advc.073: DEAL_KILL widget enabled; advc.085: iData2 set to -1
										self.resIconGrid.addIcon( currentRow, self.activeImportCol, gc.getBonusInfo(tradeData2.iData).getButton(), 64, WidgetTypes.WIDGET_DEAL_KILL, iLoopDeal, -1)
								for iLoopTradeItem in range(deal.getLengthSecondTrades()):
									tradeData2 = deal.getSecondTrade(iLoopTradeItem)
									if (tradeData2.ItemType == TradeableItems.TRADE_GOLD_PER_TURN):
										amount -= tradeData2.iData
									if (tradeData2.ItemType == TradeableItems.TRADE_RESOURCES):
										# advc.073: DEAL_KILL widget enabled; advc.085: iData2 set to -1
										self.resIconGrid.addIcon( currentRow, self.activeExportCol, gc.getBonusInfo(tradeData2.iData).getButton(), 64, WidgetTypes.WIDGET_DEAL_KILL, iLoopDeal, -1)

							if ( deal.getSecondPlayer() == iLoopPlayer and deal.getFirstPlayer() == self.iActiveLeader ):
								for iLoopTradeItem in range(deal.getLengthFirstTrades()):
									tradeData2 = deal.getFirstTrade(iLoopTradeItem)
									if (tradeData2.ItemType == TradeableItems.TRADE_GOLD_PER_TURN):
										amount -= tradeData2.iData
									if (tradeData2.ItemType == TradeableItems.TRADE_RESOURCES):
										# advc.073: DEAL_KILL widget enabled; advc.085: iData2 set to -1
										self.resIconGrid.addIcon( currentRow, self.activeExportCol, gc.getBonusInfo(tradeData2.iData).getButton(), 64, WidgetTypes.WIDGET_DEAL_KILL, iLoopDeal, -1)
								for iLoopTradeItem in range(deal.getLengthSecondTrades()):
									tradeData2 = deal.getSecondTrade(iLoopTradeItem)
									if (tradeData2.ItemType == TradeableItems.TRADE_GOLD_PER_TURN):
										amount += tradeData2.iData
									if (tradeData2.ItemType == TradeableItems.TRADE_RESOURCES):
										# advc.073: DEAL_KILL widget enabled; advc.085: iData2 set to -1
										self.resIconGrid.addIcon( currentRow, self.activeImportCol, gc.getBonusInfo(tradeData2.iData).getButton(), 64, WidgetTypes.WIDGET_DEAL_KILL, iLoopDeal, -1)
						# BUG - Kill Deal - end
					if (amount != 0):
						self.resIconGrid.setText(currentRow, self.payingCol, labelText(str(amount)))
				currentRow += 1
		self.resIconGrid.refresh()

	def scrollTradeTableUp(self):
		if (self.iScreen == self.SCREEN_DICT["BONUS"]):
			self.resIconGrid.scrollUp()
		elif (self.iScreen == self.SCREEN_DICT["TECH"]):
			self.techIconGrid.scrollUp()
		# <advc.ctr>
		elif (self.iScreen == self.SCREEN_DICT["CITIES"]):
			self.cityIconGrid.scrollUp()
		# </advc.ctr>

	def scrollTradeTableDown(self):
		if (self.iScreen == self.SCREEN_DICT["BONUS"]):
			self.resIconGrid.scrollDown()
		elif (self.iScreen == self.SCREEN_DICT["TECH"]):
			self.techIconGrid.scrollDown()
		# <advc.ctr>
		elif (self.iScreen == self.SCREEN_DICT["CITIES"]):
			self.cityIconGrid.scrollDown()
		# </advc.ctr>

	def drawTechDeals(self, bInitial):
		screen = self.getScreen()
		activePlayer = gc.getPlayer(self.iActiveLeader)
		iActiveTeam = activePlayer.getTeam()
		activeTeam = gc.getTeam(iActiveTeam)

		self.initTechTable()

		# Assemble the panel
		# <!-- custom: add extra width tweak to remove the yellow side edges, and extra height tweak to remove distracting top bright band plus have more room on bottom side's row as well, with gemini 3 pro's help thanks -->
		# iExtraWidth = 0
		iExtraWidth = 4
		TECH_PANEL_X = self.TECH_LEFT_RIGHT_SPACE - iExtraWidth
		TECH_PANEL_WIDTH = self.W_SCREEN - 2 * self.TECH_LEFT_RIGHT_SPACE + (2 * iExtraWidth)
		# iExtraHeight = 0
		iExtraHeight = 4
		TECH_PANEL_Y = self.TECH_TOP_BOTTOM_SPACE - iExtraHeight
		TECH_PANEL_HEIGHT = self.H_SCREEN - 2 * self.TECH_TOP_BOTTOM_SPACE + (2 * iExtraHeight)

		self.tradePanel = self.getNextWidgetName()
		screen.addPanel( self.tradePanel, "", "", True, True, TECH_PANEL_X, TECH_PANEL_Y, TECH_PANEL_WIDTH, TECH_PANEL_HEIGHT, PanelStyles.PANEL_STYLE_MAIN )

		self.techIconGrid.createGrid()

		self.techIconGrid.clearData()
		tradeData = TradeData()
		tradeData.ItemType = TradeableItems.TRADE_TECHNOLOGIES
		currentRow = 0

		for iLoopPlayer in range(gc.getMAX_PLAYERS()):
			currentPlayer = gc.getPlayer(iLoopPlayer)
			iLoopTeam = currentPlayer.getTeam()
			currentTeam = gc.getTeam(iLoopTeam)

			if ( currentPlayer.isAlive() and not currentPlayer.isBarbarian() and not currentPlayer.isMinorCiv() and gc.getTeam(currentPlayer.getTeam()).isHasMet(activePlayer.getTeam()) and iLoopPlayer != self.iActiveLeader ):
				message = ""
				#if ( not gc.getTeam(activePlayer.getTeam()).isTechTrading() and not gc.getTeam(currentPlayer.getTeam()).isTechTrading() ):
				# advc.120d: Make sure that Tech tab is consistent with Espionage screen
				if not activePlayer.canSeeTech(iLoopPlayer):
					message = self.TEXT_NO_TECH_TRADING
				self.techIconGrid.appendRow(labelText(currentPlayer.getName()), labelText(message))
				self.techIconGrid.addIcon( currentRow, iTechColLeader, gc.getLeaderHeadInfo(currentPlayer.getLeaderType()).getButton(), 64, WidgetTypes.WIDGET_LEADERHEAD, iLoopPlayer, self.iActiveLeader )

				# BUG - AI status - start
				zsStatus = ""
				if not DiplomacyUtil.isWillingToTalk(currentPlayer, activePlayer):
					zsStatus += u"!"

				if (currentTeam.isAtWar(iActiveTeam)):
					zsStatus += self.WAR_ICON
				elif (currentTeam.isForcePeace(iActiveTeam)):
					zsStatus += self.PEACE_ICON

				self.techIconGrid.setText(currentRow, iTechColStatus, labelText(zsStatus))
				# BUG - AI status - end
				# advc.036:
				bWillTalk = currentPlayer.AI_isWillingToTalk(self.iActiveLeader)
				if (gc.getTeam(activePlayer.getTeam()).isGoldTrading() or gc.getTeam(currentPlayer.getTeam()).isGoldTrading()) and bWillTalk:
					# <!-- custom: looks like gc.getPlayer(iLoopPlayer) could be optimized with the cached currentPlayer variable so did as such -->
					sAmount = str(currentPlayer.AI_maxGoldTrade(self.iActiveLeader))
					self.techIconGrid.setText(currentRow, iTechColGold, labelText(sAmount))

				#if (gc.getTeam(activePlayer.getTeam()).isTechTrading() or gc.getTeam(currentPlayer.getTeam()).isTechTrading() ):
				if activePlayer.canSeeTech(iLoopPlayer): # advc.120d
					for iLoopTech in range(gc.getNumTechInfos()):
						tradeData.iData = iLoopTech
						if (activePlayer.canTradeItem(iLoopPlayer, tradeData, False) and activePlayer.getTradeDenial(iLoopPlayer, tradeData) == DenialTypes.NO_DENIAL): # wants
							self.techIconGrid.addIcon( currentRow, iTechColWants, gc.getTechInfo(iLoopTech).getButton(), 64, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iLoopTech )
						elif (gc.getTeam(activePlayer.getTeam()).isHasTech(iLoopTech) and currentPlayer.canResearch(iLoopTech, False)):
							self.techIconGrid.addIcon( currentRow, iTechColCantYou, gc.getTechInfo(iLoopTech).getButton(), 64, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iLoopTech )
						elif currentPlayer.canResearch(iLoopTech, False):
							self.techIconGrid.addIcon( currentRow, iTechColResearch, gc.getTechInfo(iLoopTech).getButton(), 64, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iLoopTech )
						if (currentPlayer.canTradeItem(self.iActiveLeader, tradeData, False)):
							if currentPlayer.getTradeDenial(self.iActiveLeader, tradeData) == DenialTypes.NO_DENIAL and bWillTalk: # will trade
								self.techIconGrid.addIcon( currentRow, iTechColWill, gc.getTechInfo(iLoopTech).getButton(), 64, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iLoopTech )
							else: # won't trade
								# advc.073: Changed so that WIDGET_PEDIA_JUMP_TO_TECH_TRADE works w/o BugDll
								self.techIconGrid.addIcon( currentRow, iTechColWont, gc.getTechInfo(iLoopTech).getButton(), 64, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH_TRADE, iLoopTech, iLoopPlayer )
						else:
							# <advc.550i> If tech trading disabled, show a hint about that only once (in row 0).
							if currentRow == 0 and gc.getGame().isOption(GameOptionTypes.GAMEOPTION_NO_TECH_TRADING):
								message = gc.getGameOptionInfo(GameOptionTypes.GAMEOPTION_NO_TECH_TRADING).getDescription()
								self.techIconGrid.setText(currentRow, iTechColWill, labelText(message))
							# </advc.550i>
							if (gc.getTeam(currentPlayer.getTeam()).isHasTech(iLoopTech) and activePlayer.canResearch(iLoopTech, False)):
								self.techIconGrid.addIcon( currentRow, iTechColCantThem, gc.getTechInfo(iLoopTech).getButton(), 64, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iLoopTech )
				currentRow += 1
		self.techIconGrid.refresh()

	def initTechTable(self):
		screen = self.getScreen()

		# 1. Define the full screen
		# <!-- custom: note: this is the starting position of the grid/table, not of the blue panel -->
		# <!-- custom: note: for example gridX moves the grid's starting position more to the left or right (for example so we use what used to be the yellow empty space on the left (and also on the right) if needed) -->
		gridX = self.MIN_LEFT_RIGHT_SPACE + 10
		gridY = self.MIN_TOP_BOTTOM_SPACE + 10

		# <!-- custom: no need for the right side edge/margin in the blue panel, use this space or shrink the screen by this size if we don't need it in any other foreign advisor screen -->
		# gridWidth = self.W_SCREEN - self.MIN_LEFT_RIGHT_SPACE * 2 - 20
		# <!-- custom: we don't need margins in these areas, maximize space -->
		# <!-- custom: also widen X since we removed the yellow edges that were uneeded, fill this space with our grid as we want to use it not leave it blue with our grid ending before it. -->
		gridWidth = self.W_SCREEN + 10
		# <!-- custom: trim bottom part of the grid else it overfills below the bottom end of the blue panel, which is not what we need nor want -->
		# gridHeight = self.H_SCREEN - self.MIN_TOP_BOTTOM_SPACE * 2 - 20
		gridHeight = self.H_SCREEN - self.MIN_TOP_BOTTOM_SPACE * 2

		# 2. Setup the columns
		willTradeColumnType = IconGrid_BUG.GRID_MULTI_LIST_COLUMN
		# <advc.550i> Make the will-trade column a text column so that the no-tech-trading hint can be placed there
		if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_NO_TECH_TRADING):
			willTradeColumnType = IconGrid_BUG.GRID_TEXT_COLUMN
		# </advc.550i>

		columns = (IconGrid_BUG.GRID_ICON_COLUMN, IconGrid_BUG.GRID_TEXT_COLUMN, IconGrid_BUG.GRID_MULTI_LIST_COLUMN, IconGrid_BUG.GRID_MULTI_LIST_COLUMN, IconGrid_BUG.GRID_MULTI_LIST_COLUMN, IconGrid_BUG.GRID_TEXT_COLUMN, willTradeColumnType, IconGrid_BUG.GRID_MULTI_LIST_COLUMN, IconGrid_BUG.GRID_MULTI_LIST_COLUMN)

		self.techIconGridName = self.getNextWidgetName()
		self.techIconGrid = IconGrid_BUG.IconGrid_BUG( self.techIconGridName, screen, gridX, gridY, gridWidth, gridHeight, columns, self.TECH_USE_SMALL_ICONS, self.SHOW_LEADER_NAMES, self.SHOW_ROW_BORDERS )

		self.techIconGrid.setGroupBorder(self.GROUP_BORDER)
		self.techIconGrid.setGroupLabelOffset(self.GROUP_LABEL_OFFSET)
		self.techIconGrid.setMinColumnSpace(self.MIN_COLUMN_SPACE)
		self.techIconGrid.setMinRowSpace(self.MIN_ROW_SPACE)

		# 3. Set widths for the specific text columns (Status, Gold)
		# Note: IconGrid_BUG does NOT allow setting widths for Icon columns (Wants, Can Research, etc). 
		# It calculates them automatically by dividing the remaining space evenly.
		# self.techIconGrid.setHeader( iTechColLeader, self.TEXT_LEADER )
		# self.techIconGrid.setHeader( iTechColStatus, "" )
		self.techIconGrid.setTextColWidth( iTechColStatus, self.TECH_STATUS_COL_WIDTH )
		# <!-- custom: use cached text values for performance (claude code sonnet 4.5) -->
		self.techIconGrid.setHeader( iTechColWants, labelText(self.TEXT_WANTS) )
		# advc.004g: was TXT_KEY_FOREIGN_ADVISOR_CANT_TRADE
		self.techIconGrid.setHeader( iTechColCantYou, labelText(self.TEXT_CANT_RECEIVE) )
		self.techIconGrid.setHeader( iTechColResearch, labelText(self.TEXT_CAN_RESEARCH) )
		self.techIconGrid.setHeader( iTechColGold, labelText(u"%c" % gc.getCommerceInfo(CommerceTypes.COMMERCE_GOLD).getChar()) )
		self.techIconGrid.setTextColWidth( iTechColGold, self.TECH_GOLD_COL_WIDTH )

		self.techIconGrid.setHeader( iTechColWill, labelText(self.TEXT_FOR_TRADE) )
		# <advc.550i>
		if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_NO_TECH_TRADING):
			self.techIconGrid.setTextColWidth(iTechColWill, 2 * self.TECH_GOLD_COL_WIDTH) # </advc.550i>

		self.techIconGrid.setHeader( iTechColWont, labelText(self.TEXT_NOT_FOR_TRADE) )
		self.techIconGrid.setHeader( iTechColCantThem, labelText(self.TEXT_CANT_TRADE) )

		# <!-- custom: fit more information in each row so we don't have to scroll to see extra techs.
		# Change with the help of gemini 3 pro -->
		# Based on your screenshots (specifically Civ4ScreenShot0084.jpg) and the code provided, the issue is not the column width variables (TECH_STATUS_COL_WIDTH).
		# The problem is at the very end of the initTechTable function. The code calculates the "Preferred Width" (the minimum width required to hold the icons) and then shrinks the table to fit that minimum size, centering it in your large window. This ignores the full screen width you successfully set up earlier.
		# You need to replace the end of that function to stop it from shrinking and to manually force the columns to share the available space.
		# We removed the lines that called getPrefferedWidth() and resized the grid variables.
		# This ensures the grid stays at the full size we calculated at the top.
		# IconGrid_BUG will automatically spread the columns to fill this space.
		# gridWidth = self.techIconGrid.getPrefferedWidth()
		# gridHeight = self.techIconGrid.getPrefferedHeight()
		# self.TECH_LEFT_RIGHT_SPACE = (self.W_SCREEN - gridWidth - 20) / 2
		# self.TECH_TOP_BOTTOM_SPACE = (self.H_SCREEN - gridHeight - 20) / 2
		# gridX = self.TECH_LEFT_RIGHT_SPACE + 10
		# gridY = self.TECH_TOP_BOTTOM_SPACE + 10
		#
		# self.techIconGrid.setPosition(gridX, gridY)
		# self.techIconGrid.setSize(gridWidth, gridHeight)

		# --- FIX: Define these variables so drawTechDeals doesn't crash ---
		# We set them to the minimum margins so the panel uses the full available space
		self.TECH_LEFT_RIGHT_SPACE = self.MIN_LEFT_RIGHT_SPACE
		self.TECH_TOP_BOTTOM_SPACE = self.MIN_TOP_BOTTOM_SPACE

	# <advc.ctr>
	def initCityTable(self): # Based on initTechTable
		screen = self.getScreen()

		# GRID_ICON for the leader head, then text for the war/peace icon and 4x multi-text for the four city columns.
		columns = (IconGrid_BUG.GRID_ICON_COLUMN, IconGrid_BUG.GRID_TEXT_COLUMN, IconGrid_BUG.GRID_MULTI_TEXT_COLUMN, IconGrid_BUG.GRID_MULTI_TEXT_COLUMN,IconGrid_BUG.GRID_MULTI_TEXT_COLUMN, IconGrid_BUG.GRID_MULTI_TEXT_COLUMN)

		gridWidth = self.W_SCREEN - self.MIN_LEFT_RIGHT_SPACE * 2 - 20
		gridHeight = self.H_SCREEN - self.MIN_TOP_BOTTOM_SPACE * 2 - 20
		iconGrid = IconGrid_BUG.IconGrid_BUG(self.getNextWidgetName(), screen, self.MIN_LEFT_RIGHT_SPACE + 10, self.MIN_TOP_BOTTOM_SPACE + 10, gridWidth, gridHeight, columns, True, self.SHOW_LEADER_NAMES, self.SHOW_ROW_BORDERS)
		iconGrid.setGroupBorder(self.GROUP_BORDER)
		iconGrid.setGroupLabelOffset(self.GROUP_LABEL_OFFSET)
		iconGrid.setMinColumnSpace(self.MIN_COLUMN_SPACE)
		iconGrid.setMinRowSpace(self.MIN_ROW_SPACE)

		iStatusColWidth = self.TECH_STATUS_COL_WIDTH
		iconGrid.setTextColWidth(iCityColStatus, iStatusColWidth)
		iRemainingWidth = gridWidth - iStatusColWidth - 64
		iCityColWidth = iRemainingWidth // 4
		iconGrid.setTextColWidth(iCityColWants, iCityColWidth)
		iconGrid.setTextColWidth(iCityColRejects, iCityColWidth)
		iconGrid.setTextColWidth(iCityColWillCede, iCityColWidth)
		iconGrid.setTextColWidth(iCityColWontCede, iCityColWidth)

		iconGrid.setHeader(iCityColWants, labelText(self.TEXT_WANTS))
		eDenialColor = self.COLOR_WHITE
		iconGrid.setHeader(iCityColRejects, labelText(localText.getColorText("TXT_KEY_FOREIGN_ADVISOR_REJECTS", (), eDenialColor)))
		iconGrid.setHeader(iCityColWillCede, labelText(self.TEXT_WILL_CEDE))
		iconGrid.setHeader(iCityColWontCede, labelText(localText.getColorText("TXT_KEY_FOREIGN_ADVISOR_WONT_CEDE", (), eDenialColor)))

		gridWidth = iconGrid.getPrefferedWidth()
		gridHeight = iconGrid.getPrefferedHeight()
		LEFT_RIGHT_SPACE = (self.W_SCREEN - gridWidth - 20) / 2
		TOP_BOTTOM_SPACE = (self.H_SCREEN - gridHeight - 20) / 2

		# <!-- custom: manually adjust the grid's position and size so it better fits the screen -->
		# iExtraX = 0
		# iExtraWidth = 0
		iExtraX = 24
		# <!-- custom: note: empirically, it seems that reducing width to have symmetrical margins than on the left side causes buttons (or is it icons?)
		# like the war declare or peace one to be too close to the leader's button.
		# On the other hand, the right space is useless in itself, and we have no other column on the right, so it's fine if it overfills beyond the screen on the right side, as long as it helps have enough space from the leader button.
		# Could maybe ideally fix this more properly but may be possibly very tedious and is fine enough as such i mean if i may say so left as such as long as it displays nicely on the left side.
		# (comparing it to the foreign advisor's tech tab and trying to have a more or less identical left side display seems more important). -->
		iExtraWidth = 10

		iconGrid.setPosition(LEFT_RIGHT_SPACE + 10 + iExtraX, TOP_BOTTOM_SPACE + 10)
		iconGrid.setSize(gridWidth + iExtraWidth, gridHeight)

		return iconGrid, LEFT_RIGHT_SPACE, TOP_BOTTOM_SPACE

	def drawCityDeals(self, bInitial): # Based on drawTechDeals
		screen = self.getScreen()
		self.eDenialColor = self.COLOR_LIGHT_GREY

		iconGrid, PANEL_X, PANEL_Y = self.initCityTable()
		PANEL_WIDTH = self.W_SCREEN - 2 * PANEL_X
		PANEL_HEIGHT = self.H_SCREEN - 2 * PANEL_Y

		# <!-- custom: manually adjust the blue panel's position and size so it better fits the screen -->
		# iExtraX = 0
		# iExtraY = 0
		# iExtraWidth = 0
		# iExtraHeight = 0
		iExtraX = 20
		iExtraY = -12
		iExtraWidth = -40
		iExtraHeight = 24

		screen.addPanel(self.getNextWidgetName(), "", "", True, True, PANEL_X +iExtraX, PANEL_Y +iExtraY, PANEL_WIDTH + iExtraWidth, PANEL_HEIGHT + iExtraHeight, PanelStyles.PANEL_STYLE_MAIN )

		iconGrid.createGrid()
		iconGrid.clearData()
		tradeData = TradeData()
		tradeData.ItemType = TradeableItems.TRADE_CITIES

		iActivePlayer = self.iActiveLeader
		activePlayer = gc.getPlayer(iActivePlayer)
		iActiveTeam = activePlayer.getTeam()
		currentRow = 0
		for iCurrentPlayer in range(gc.getMAX_CIV_PLAYERS()):
			currentPlayer = gc.getPlayer(iCurrentPlayer)
			currentTeam = gc.getTeam(currentPlayer.getTeam())

			if not currentPlayer.isAlive() or currentPlayer.isMinorCiv() or not currentTeam.isHasMet(iActiveTeam) or iCurrentPlayer == iActivePlayer:
				continue

			iconGrid.appendRow(labelText(currentPlayer.getName()), "")

			iconGrid.addIcon(currentRow, iCityColLeader, gc.getLeaderHeadInfo(currentPlayer.getLeaderType()).getButton(), 64, WidgetTypes.WIDGET_LEADERHEAD, iCurrentPlayer, iActivePlayer)
			# BUG - AI status - start
			szStatus = ""
			if not DiplomacyUtil.isWillingToTalk(currentPlayer, activePlayer):
				szStatus += u"!"
			if currentTeam.isAtWar(iActiveTeam):
				szStatus += self.WAR_ICON
			elif currentTeam.isForcePeace(iActiveTeam):
				szStatus += self.PEACE_ICON
			iconGrid.setText(currentRow, iCityColStatus, labelText(szStatus))
			# BUG - AI status - end
			# Precompute the contents of each cell in order to shorten the city strings as necessary
			iLiberate = iCityColWontCede + 1 # Pseudo-column for sorting
			citiesPerCol = { iCityColWants : [], iCityColRejects : [], iCityColWillCede : [], iCityColWontCede : [], iLiberate : [] }
			(city, iter) = activePlayer.firstCity(False)
			while(city):
				tradeData.iData = city.getID()
				if activePlayer.canTradeItem(iCurrentPlayer, tradeData, False):
					iCol = iCityColWants
					if activePlayer.getTradeDenial(iCurrentPlayer, tradeData) != DenialTypes.NO_DENIAL:
						iCol = iCityColRejects
					elif city.getLiberationPlayer(False) == iCurrentPlayer:
						iCol = iLiberate
					citiesPerCol[iCol].append(city)
				(city, iter) = activePlayer.nextCity(iter, False)
			(city, iter) = currentPlayer.firstCity(False)
			while(city):
				tradeData.iData = city.getID()
				if currentPlayer.canTradeItem(iActivePlayer, tradeData, False):
					iCol = iCityColWillCede
					if currentPlayer.getTradeDenial(iActivePlayer, tradeData) != DenialTypes.NO_DENIAL:
						iCol = iCityColWontCede
					citiesPerCol[iCol].append(city)
				(city, iter) = currentPlayer.nextCity(iter, False)
			for iCol in citiesPerCol:
				# Smallest population first; more likely to be traded.
				citiesPerCol[iCol].sort(None, lambda city: city.getPopulation())
			for iCol in citiesPerCol:
				bGrayOut = (iCol == iCityColRejects or iCol == iCityColWontCede)
				iCities = len(citiesPerCol[iCol])
				iAdded = 0
				for city in citiesPerCol[iCol]:
					iDisplayCol = iCol
					# Cities that the non-active player wants liberated go in the "wants" column
					if iCol == iLiberate:
						iDisplayCol = iCityColWants
					szCity = ""
					widgetType = WidgetTypes.WIDGET_CITY_TRADE
					iWhoTo = iActivePlayer
					if city.getOwner() == iActivePlayer:
						iWhoTo = iCurrentPlayer
					# Need to fold both players into a single variable
					widgetData1 = city.getOwner() + 100 * (iWhoTo + 1)
					widgetData2 = city.getID()
					if iAdded == 5 and iCities > 6:
						szCity = localText.getText("TXT_KEY_FOREIGN_ADVISOR_MORE_CITIES", (iCities - iAdded,))
						# There's still room for a boolean:
						widgetData1 = (widgetData1 + 1) * 100
					else:
						szCity = self.getCityText(city, iCol == iLiberate, bGrayOut, iCities > 3)
					iconGrid.addText(currentRow, iDisplayCol, labelText(szCity), widgetType, widgetData1, widgetData2)
					iAdded += 1
					if iAdded >= 6:
						break

			currentRow += 1

		self.cityIconGrid = iconGrid # for input handlers
		iconGrid.refresh()

	def getCityText(self, city, bLiberate, bGrayOut, bShort):
		r = city.getName()
		iCharLimit = 15
		if bShort:
			iCharLimit = 12
			if bLiberate:
				iCharLimit = 10
		if len(r) > iCharLimit:
			r = r[:iCharLimit-3] + "..."
		# Looks nicer w/o population counts
		#if not bShort:
			#r += " [" + str(city.getPopulation()) + "]"
		if bGrayOut:
			r = localText.changeTextColor(r, self.eDenialColor)
		# With a citizen icon, the population counts look even worse.
		#if not bShort:
		#	r += " " + (u"%c" % CyGame().getSymbolID(FontSymbols.CITIZEN_CHAR))
			# <!-- custom: use cached icon value for performance (claude code sonnet 4.5) -->
		#	r += str(city.getPopulation())
		if bLiberate:
			r += " " + (u"%c" % self.iOccupationIcon)
		return r
	# </advc.ctr>

	##########################################
	### END CHANGES ENHANCED INTERFACE MOD ###
	##########################################

	# Handles the input for this screen...
	def handleInput (self, inputClass):
		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED):
			# advc.004: Removed BugDLL call from second condition
			if inputClass.getButtonType() == WidgetTypes.WIDGET_LEADERHEAD or inputClass.getButtonType() == WidgetTypes.WIDGET_LEADERHEAD_RELATIONS:
				if (inputClass.getFlags() & MouseFlags.MOUSE_LBUTTONUP):
					self.iSelectedLeader = inputClass.getData1()
					self.drawContents(False)
					return 1
				elif (inputClass.getFlags() & MouseFlags.MOUSE_RBUTTONUP):
					if inputClass.getData1() != self.iActiveLeader:
						self.getScreen().hideScreen()
						return 1
			elif (inputClass.getFunctionName() == self.GLANCE_BUTTON):
				self.handlePlusMinusToggle()
				return 1
			# <!-- custom: glance tab column pagination arrows (Claude Code Sonnet 4.6) -->
			elif self.glanceLeftArrowID and inputClass.getFunctionName() + str(inputClass.getID()) == self.glanceLeftArrowID:
				self.iGlancePage = max(0, self.iGlancePage - 1)
				self.drawContents(False)
				return 1
			elif self.glanceRightArrowID and inputClass.getFunctionName() + str(inputClass.getID()) == self.glanceRightArrowID:
				self.iGlancePage = min(self.iGlanceNumPages - 1, self.iGlancePage + 1)
				self.drawContents(False)
				return 1
	############################################
	### BEGIN CHANGES ENHANCED INTERFACE MOD ###
	############################################
			elif (inputClass.getButtonType() == WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS):
#				ForeignAdvisorPrint ("FOOOOOO!!!!")
				pass
	##########################################
	### END CHANGES ENHANCED INTERFACE MOD ###
	##########################################

		elif (inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED):
			if (inputClass.getFunctionName() + str(inputClass.getID()) == self.getWidgetName(self.DEBUG_DROPDOWN_ID)):
				print('debug dropdown event')
				szName = self.getWidgetName(self.DEBUG_DROPDOWN_ID)
				self.iActiveLeader = getAdvisorDebugDropdownSelectedPlayer(self.getScreen(), szName)
				self.drawContents(True)
				return 1
		elif (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CHARACTER):
			if (inputClass.getData() == int(InputTypes.KB_LSHIFT) or inputClass.getData() == int(InputTypes.KB_RSHIFT)):
				self.iShiftKeyDown = inputClass.getID()
				return 1

		if (self.iScreen == self.SCREEN_DICT["BONUS"]):
			return self.resIconGrid.handleInput(inputClass)
		elif (self.iScreen == self.SCREEN_DICT["TECH"]):
			return self.techIconGrid.handleInput(inputClass)
		elif (self.iScreen == self.SCREEN_DICT["ESPIONAGE"]):
			self.ESP_iActivePlayer = self.iActiveLeader
			return self.handleEspionageInput(inputClass)
		# <advc.ctr>
		elif (self.iScreen == self.SCREEN_DICT["CITIES"]):
			return self.cityIconGrid.handleInput(inputClass)
		# </advc.ctr>
		return 0

	# <!-- custom: Espionage tab logic integrated directly into CvForeignAdvisor (no helper class/wrapper object).
	# Kept existing Espionage comments unless obsolete. (GPT-5.3-Codex) -->

	def drawEspionageContents(self):

		self.deleteAllEspionageWidgets()

		# Create a new screen, called EspionageAdvisor, using the file EspionageAdvisor.py for input
		screen = self.getScreen()

		pActivePlayer = gc.getPlayer(self.ESP_iActivePlayer)
		pActiveTeam = gc.getTeam(pActivePlayer.getTeam())

		self.ESP_aiKnownPlayers = []
		self.ESP_aiUnknownPlayers = []
		self.ESP_iNumEntries= 0

		for iLoop in range(gc.getMAX_PLAYERS()):
			pPlayer = gc.getPlayer(iLoop)
			if (pPlayer.getTeam() != pActivePlayer.getTeam() and not pPlayer.isBarbarian()):
				if (pPlayer.isAlive()):
					if (pActiveTeam.isHasMet(pPlayer.getTeam())):

						self.ESP_aiKnownPlayers.append(iLoop)
						self.ESP_iNumEntries = self.ESP_iNumEntries + 1

		# <!-- custom: The old standalone Espionage Advisor reset target/city/mission state on every open, but integrating it into the persistent Foreign Advisor object made those selections survive indefinitely.
		# Preserve a still-valid target, but when the retained rival is no longer alive/met/eligible, clear all dependent state before selecting the first current target; this prevents stale city and mission data from following an invalid rival. See KI#234. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if self.ESP_iTargetPlayer not in self.ESP_aiKnownPlayers:
			self.ESP_iTargetPlayer = -1
			self.ESP_iActiveCityID = -1
			self.ESP_iSelectedMission = -1
		if self.ESP_iTargetPlayer == -1 and len(self.ESP_aiKnownPlayers) > 0:
			self.ESP_iTargetPlayer = self.ESP_aiKnownPlayers[0]

		while(self.ESP_iNumEntries < 17):
			self.ESP_iNumEntries = self.ESP_iNumEntries + 1
			self.ESP_aiUnknownPlayers.append(self.ESP_iNumEntries)
		bCompactLeaderRows = self.useCompactEspionageRows()
		if bCompactLeaderRows:
			self.widenEspionageLeftPaneForCompactRows()

		self.ESP_szMainPaneWidget = "EspionageMainPane"
		screen.addPanel( self.ESP_szMainPaneWidget, "", "", true, true, self.ESP_X_MAIN_PANE, self.ESP_Y_MAIN_PANE, self.ESP_W_MAIN_PANE, self.ESP_H_MAIN_PANE, PanelStyles.PANEL_STYLE_MAIN )

		self.ESP_szLeftPaneWidget = "LeftPane"
		screen.addPanel( self.ESP_szLeftPaneWidget, "", "", true, true, self.ESP_X_LEFT_PANE, self.ESP_Y_LEFT_PANE, self.ESP_W_LEFT_PANE, self.ESP_H_LEFT_PANE, PanelStyles.PANEL_STYLE_EMPTY )

		self.ESP_szScrollPanel = "ScrollPanel"
		screen.addPanel( self.ESP_szScrollPanel, "", "", true, true, self.ESP_X_SCROLL, self.ESP_Y_SCROLL, self.ESP_W_SCROLL, self.ESP_H_SCROLL, PanelStyles.PANEL_STYLE_EMPTY)

		############################
		#### Total EPs Per Turn Text
		############################
		# <!-- custom: top summary panel intentionally disabled in integrated Foreign->Espionage tab.
		# Redundant now that global commerce sliders are visible on the right; preserving this block commented
		# keeps rollback trivial if we want a dedicated Espionage header again later. (GPT-5.3-Codex) -->
		#self.ESP_szTotalPaneWidget = "TotalPane"
		#screen.addPanel( self.ESP_szTotalPaneWidget, "", "", true, true,
		#	self.ESP_X_TOTAL_PANE, self.ESP_Y_TOTAL_PANE, self.ESP_W_TOTAL_PANE, self.ESP_H_TOTAL_PANE, PanelStyles.PANEL_STYLE_MAIN )
		#
		## <advc.120c> Replace TOTAL_NUM_EPS text with espionage slider
		## (The slider is in the refresh function though)
		##self.szMakingText = "MakingText"
		##self.X_MAKING_TEXT = 490
		##self.Y_MAKING_TEXT = 85
		##szText = sasFontTagTitle + localText.getText("TXT_KEY_ESPIONAGE_SCREEN_TOTAL_NUM_EPS", (pActivePlayer.getCommerceRate(CommerceTypes.COMMERCE_ESPIONAGE), )) + SAS_FONT_TAG_CLOSE
		##screen.setLabel(self.szMakingText, "Background", szText, CvUtil.FONT_LEFT_JUSTIFY, self.X_MAKING_TEXT, self.Y_MAKING_TEXT, self.ESP_Z_CONTROLS, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		## </advc.120c>

		############################
		#### Right Panel
		############################

		self.ESP_szRightPaneWidget = "RightPane"
		screen.addPanel( self.ESP_szRightPaneWidget, "", "", true, true, self.ESP_X_RIGHT_PANE, self.ESP_Y_RIGHT_PANE, self.ESP_W_RIGHT_PANE, self.ESP_H_RIGHT_PANE, PanelStyles.PANEL_STYLE_EMPTY )

		if (self.ESP_iTargetPlayer != -1):

			self.ESP_szCitiesTitleText = "CitiesTitle"
			szText = sasFontTagTitle + self.TEXT_CONCEPT_CITIES + SAS_FONT_TAG_CLOSE
			screen.setLabel(self.ESP_szCitiesTitleText, "Background", szText, CvUtil.FONT_LEFT_JUSTIFY, self.ESP_X_CITY_LIST, self.ESP_Y_CITY_LIST - 40, self.ESP_Z_CONTROLS, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

			self.ESP_szEffectsTitleText = "EffectsTitle"
			szText = sasFontTagTitle + self.TEXT_ESPIONAGE_PASSIVE_EFFECTS + SAS_FONT_TAG_CLOSE
			screen.setLabel(self.ESP_szEffectsTitleText, "Background", szText, CvUtil.FONT_LEFT_JUSTIFY, self.ESP_X_EFFECTS_LIST, self.ESP_Y_EFFECTS_LIST - 40, self.ESP_Z_CONTROLS, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

			self.ESP_szMissionsTitleText = "MissionsTitle"
			szText = sasFontTagTitle + self.TEXT_ESPIONAGE_MISSIONS + SAS_FONT_TAG_CLOSE
			screen.setLabel(self.ESP_szMissionsTitleText, "Background", szText, CvUtil.FONT_LEFT_JUSTIFY, self.ESP_X_MISSIONS_LIST, self.ESP_Y_MISSIONS_LIST - 40, self.ESP_Z_CONTROLS, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
			# advc.120d: was TXT_KEY_ESPIONAGE_SCREEN_COST
			self.ESP_szEffectsCostTitleText = "EffectsCostTitle"
			szText = sasFontTagTitle + self.TEXT_MISC_THRESHOLD + SAS_FONT_TAG_CLOSE
			# advc.120d: Subtract 40 from horizontal position
			screen.setLabel(self.ESP_szEffectsCostTitleText, "Background", szText, CvUtil.FONT_LEFT_JUSTIFY, self.ESP_X_EFFECTS_COSTS_LIST-40, self.ESP_Y_EFFECTS_COSTS_LIST - 40, self.ESP_Z_CONTROLS, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

			self.ESP_szMissionsCostTitleText = "MissionsCostTitle"
			szText = sasFontTagTitle + self.TEXT_ESPIONAGE_COST + SAS_FONT_TAG_CLOSE
			screen.setLabel(self.ESP_szMissionsCostTitleText, "Background", szText, CvUtil.FONT_LEFT_JUSTIFY, self.ESP_X_MISSIONS_COSTS_LIST, self.ESP_Y_MISSIONS_COSTS_LIST - 40, self.ESP_Z_CONTROLS, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

			############################
			#### Left Leaders Panel
			############################

			wNamePanel = 220
			hNamePanel = 30
			iCompactXWeight, iCompactXPoints, iCompactXAmount = self.getCompactEspionageRowColumns()

			for iPlayerID in self.ESP_aiKnownPlayers:

				pTargetPlayer = gc.getPlayer(iPlayerID)

				iTargetTeam = pTargetPlayer.getTeam()

				iX = 0
				iY = 14

				attach = "LeaderContainer%d" % (iPlayerID)

				screen.attachPanel(self.ESP_szScrollPanel, attach, "", "", True, False, PanelStyles.PANEL_STYLE_STANDARD)

				szName = "LeaderImageA%d" %(iPlayerID)
				screen.attachSeparator(attach, szName, true, 30)

				self.ESP_iLeaderImagesID = 456
				szName = "LeaderImage%d" %(iPlayerID)
				if bCompactLeaderRows:
					screen.addCheckBoxGFCAt(attach, szName, gc.getLeaderHeadInfo(gc.getPlayer(iPlayerID).getLeaderType()).getButton(), self.SAS_ART_BUTTON_HILITE_SQUARE, self.ESP_COMPACT_ROW_X_ICON, 2, self.ESP_COMPACT_LEADER_ICON_SIZE, self.ESP_COMPACT_LEADER_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, self.ESP_iLeaderImagesID, iPlayerID, ButtonStyles.BUTTON_STYLE_LABEL, False)
				else:
					screen.addCheckBoxGFCAt(attach, szName, gc.getLeaderHeadInfo(gc.getPlayer(iPlayerID).getLeaderType()).getButton(), self.SAS_ART_BUTTON_HILITE_SQUARE, self.ESP_ROW_X_LEADER_ICON, self.ESP_ROW_Y_LEADER_ICON, self.ESP_ROW_LEADER_ICON_SIZE, self.ESP_ROW_LEADER_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, self.ESP_iLeaderImagesID, iPlayerID, ButtonStyles.BUTTON_STYLE_LABEL, False)
				if (self.ESP_iTargetPlayer == iPlayerID):
					screen.setState(szName, true)

				if not bCompactLeaderRows:
					szName = "LeaderNamePanel%d" %(iPlayerID)
					screen.attachPanelAt( attach, szName, "", "", true, false, PanelStyles.PANEL_STYLE_MAIN, iX + 5, iY-15, wNamePanel, hNamePanel, WidgetTypes.WIDGET_GENERAL, -1, -1 )

				szName = "NameText%d" %(iPlayerID)
				szTempBuffer = u"<color=%d,%d,%d,%d>%s (%s)</color>" %(pTargetPlayer.getPlayerTextColorR(), pTargetPlayer.getPlayerTextColorG(), pTargetPlayer.getPlayerTextColorB(), pTargetPlayer.getPlayerTextColorA(), pTargetPlayer.getName(), self.getEspionageMultiplierAgainstTarget(iPlayerID, bCompactLeaderRows))
				szText = sasFontTagLabel + szTempBuffer + SAS_FONT_TAG_CLOSE
				if bCompactLeaderRows:
					screen.setLabelAt( szName, attach, szText, CvUtil.FONT_LEFT_JUSTIFY, self.ESP_COMPACT_ROW_X_NAME, self.ESP_COMPACT_ROW_Y_TEXT, self.ESP_Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
				else:
					screen.setLabelAt( szName, attach, szText, 0, iX + self.ESP_ROW_X_NAME, iY + self.ESP_ROW_Y_TOP, self.ESP_Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

				szName = "PointsText%d" %(iPlayerID)
				szText = sasFontTagLabel + localText.getText("TXT_KEY_ESPIONAGE_NUM_EPS", (pActiveTeam.getEspionagePointsAgainstTeam(iTargetTeam), )) + SAS_FONT_TAG_CLOSE
				if bCompactLeaderRows:
					screen.setLabelAt( szName, attach, szText, CvUtil.FONT_RIGHT_JUSTIFY, iCompactXPoints, self.ESP_COMPACT_ROW_Y_TEXT, self.ESP_Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
				else:
					screen.setLabelAt( szName, attach, szText, 0, self.ESP_ROW_X_RIGHT, iY + self.ESP_ROW_Y_TOP + 1, self.ESP_Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

				szName = "SpendingText%d" %(iPlayerID)
				if bCompactLeaderRows:
					szText = sasFontTagLabel + (u"%d" %(pActivePlayer.getEspionageSpendingWeightAgainstTeam(iTargetTeam))) + SAS_FONT_TAG_CLOSE
					screen.setLabelAt( szName, attach, szText, CvUtil.FONT_RIGHT_JUSTIFY, iCompactXWeight, self.ESP_COMPACT_ROW_Y_TEXT, self.ESP_Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
				else:
					szText = sasFontTagLabel + (u"%s: %d" %(self.TEXT_ESPIONAGE_WEIGHT, pActivePlayer.getEspionageSpendingWeightAgainstTeam(iTargetTeam))) + SAS_FONT_TAG_CLOSE
					screen.setLabelAt( szName, attach, szText, 0, self.ESP_ROW_X_WEIGHT, iY + self.ESP_ROW_Y_WEIGHT, self.ESP_Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

				szName = "AmountText%d" %(iPlayerID)
				szText = self.getEspionagePerTurnText(pActivePlayer.getEspionageSpending(iTargetTeam))

				if bCompactLeaderRows:
					screen.setLabelAt( szName, attach, szText, CvUtil.FONT_RIGHT_JUSTIFY, iCompactXAmount, self.ESP_COMPACT_ROW_Y_TEXT, self.ESP_Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
				else:
					screen.setLabelAt( szName, attach, szText, 0, self.ESP_ROW_X_RIGHT, iY + self.ESP_ROW_Y_WEIGHT, self.ESP_Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

				iSize = 16
				self.ESP_iIncreaseButtonID = 555
				szName = "IncreaseButton%d" %(iPlayerID)
				if bCompactLeaderRows:
					iSize = self.ESP_COMPACT_BUTTON_SIZE
					screen.setImageButtonAt( szName, attach, self.SAS_ART_BUTTON_PLUS, self.ESP_COMPACT_ROW_X_BUTTON_PLUS, self.ESP_COMPACT_ROW_Y_BUTTON, iSize, iSize, WidgetTypes.WIDGET_GENERAL, self.ESP_iIncreaseButtonID, iPlayerID )
				else:
					screen.setImageButtonAt( szName, attach, self.SAS_ART_BUTTON_PLUS, self.ESP_ROW_X_BUTTON_PLUS, iY + self.ESP_ROW_Y_BUTTON, iSize, iSize, WidgetTypes.WIDGET_GENERAL, self.ESP_iIncreaseButtonID, iPlayerID )
				self.ESP_iDecreaseButtonID = 556
				szName = "DecreaseButton%d" %(iPlayerID)
				if bCompactLeaderRows:
					screen.setImageButtonAt( szName, attach, self.SAS_ART_BUTTON_MINUS, self.ESP_COMPACT_ROW_X_BUTTON_MINUS, self.ESP_COMPACT_ROW_Y_BUTTON, iSize, iSize, WidgetTypes.WIDGET_GENERAL, self.ESP_iDecreaseButtonID, iPlayerID )
				else:
					screen.setImageButtonAt( szName, attach, self.SAS_ART_BUTTON_MINUS, self.ESP_ROW_X_BUTTON_MINUS, iY + self.ESP_ROW_Y_BUTTON, iSize, iSize, WidgetTypes.WIDGET_GENERAL, self.ESP_iDecreaseButtonID, iPlayerID )

			for iPlayerID in self.ESP_aiUnknownPlayers:
				attach = "EmptyLeaderContainer%d" % (iPlayerID)
				screen.attachPanel(self.ESP_szScrollPanel, attach, "", "", True, False, PanelStyles.PANEL_STYLE_STANDARD)
				screen.attachSeparator(attach, "EmptyLeaderImageA%d" %(iPlayerID), true, 30)

	def getEspionageMultiplierAgainstTarget(self, iTargetPlayer=-1, bCompact=False):

		szMultiplier = ""

		if (iTargetPlayer == -1):
			iTargetPlayer = self.ESP_iTargetPlayer

		pActivePlayer = gc.getPlayer(self.ESP_iActivePlayer)
		pActiveTeam = gc.getTeam(pActivePlayer.getTeam())
		pTargetPlayer = gc.getPlayer(iTargetPlayer)
		pTargetTeam = gc.getTeam(pTargetPlayer.getTeam())

		iModifier = getEspionageModifier(pActivePlayer.getTeam(), pTargetPlayer.getTeam())
		if bCompact:
			szMultiplier = u"%d%%" %(iModifier)
		else:
			szMultiplier = localText.getText("TXT_KEY_ESPIONAGE_COST", (iModifier, ))

		if (pActiveTeam.getCounterespionageTurnsLeftAgainstTeam(pTargetPlayer.getTeam()) > 0):
			szMultiplier += u"*"

		if (pTargetTeam.getCounterespionageTurnsLeftAgainstTeam(pActivePlayer.getTeam()) > 0):
			szMultiplier += u"+"

		return szMultiplier

	def getEspionagePerTurnText(self, iEspionagePerTurn):
		# <!-- custom: centralize the repeated EPs-per-turn formatting; compact rows still use the normal translated text because the widened left pane has enough room and the full label is clearer. (GPT-5.5-Thinking + GPT-5.5?) -->
		szPerTurn = localText.getText("TXT_KEY_ESPIONAGE_NUM_EPS_PER_TURN", (iEspionagePerTurn, ))
		if iEspionagePerTurn > 0:
			return sasFontTagLabel + (u"<color=0,255,0,0>%s</color>" %(szPerTurn)) + SAS_FONT_TAG_CLOSE
		return sasFontTagLabel + (u"<color=192,0,0,0>%s</color>" %(szPerTurn)) + SAS_FONT_TAG_CLOSE

	def useCompactEspionageRows(self):
		if self.SAS_CV_FOREIGN_ADVISOR_ESPIONAGE_COMPACT_ROWS_THRESHOLD is None:
			self.SAS_CV_FOREIGN_ADVISOR_ESPIONAGE_COMPACT_ROWS_THRESHOLD = gc.getDefineINT("SAS_CV_FOREIGN_ADVISOR_ESPIONAGE_COMPACT_ROWS_THRESHOLD")
		return (self.SAS_CV_FOREIGN_ADVISOR_ESPIONAGE_COMPACT_ROWS_THRESHOLD <= 0 or len(self.ESP_aiKnownPlayers) > self.SAS_CV_FOREIGN_ADVISOR_ESPIONAGE_COMPACT_ROWS_THRESHOLD)

	def getCompactEspionageRowColumns(self):
		iCompactXAmount = min(self.ESP_COMPACT_ROW_X_AMOUNT, self.ESP_W_SCROLL - self.ESP_COMPACT_ROW_RIGHT_MARGIN)
		iCompactXPoints = min(self.ESP_COMPACT_ROW_X_POINTS, iCompactXAmount - 80)
		iCompactXWeight = min(self.ESP_COMPACT_ROW_X_WEIGHT, iCompactXPoints - 80)
		iCompactXWeight = max(self.ESP_COMPACT_ROW_X_NAME + 150, iCompactXWeight)
		iCompactXPoints = max(iCompactXWeight + 55, iCompactXPoints)
		iCompactXAmount = max(iCompactXPoints + 70, iCompactXAmount)
		return iCompactXWeight, iCompactXPoints, iCompactXAmount

	def refreshEspionageScreen(self):

		self.deleteAllEspionageWidgets()

		if (self.ESP_iTargetPlayer != -1):

			# Create a new screen, called EspionageAdvisor, using the file EspionageAdvisor.py for input
			screen = self.getScreen()

			pActivePlayer = gc.getPlayer(self.ESP_iActivePlayer)
			pActiveTeam = gc.getTeam(pActivePlayer.getTeam())

			# <!-- custom: disabled the Espionage slider draw here: global commerce sliders already exist on the right, so this duplicate control is redundant/noisy in the integrated Foreign tab.
			# Keep drawEspionageSlider() code for possible future reuse. (GPT-5.3-Codex) -->
			# self.drawEspionageSlider() # advc.120c

			bCompactLeaderRows = self.useCompactEspionageRows()
			aCompactColumns = self.getCompactEspionageRowColumns()
			iCompactXWeight = aCompactColumns[0]
			iCompactXAmount = aCompactColumns[2]

			for iPlayerID in self.ESP_aiKnownPlayers:

				iX = 0
				iY = 15

				pTargetPlayer = gc.getPlayer(iPlayerID)
				iTargetTeam = pTargetPlayer.getTeam()

				attach = "LeaderContainer%d" % (iPlayerID)

				szName = "SpendingText%d" %(iPlayerID)
				screen.deleteWidget(szName)
				if bCompactLeaderRows:
					szText = sasFontTagLabel + u"%d" %(pActivePlayer.getEspionageSpendingWeightAgainstTeam(iTargetTeam)) + SAS_FONT_TAG_CLOSE
					screen.setLabelAt( szName, attach, szText, CvUtil.FONT_RIGHT_JUSTIFY, iCompactXWeight, self.ESP_COMPACT_ROW_Y_TEXT, self.ESP_Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
				else:
					szText = sasFontTagLabel + self.TEXT_ESPIONAGE_WEIGHT + ": %d" %(pActivePlayer.getEspionageSpendingWeightAgainstTeam(iTargetTeam)) + SAS_FONT_TAG_CLOSE
					screen.setLabelAt( szName, attach, szText, 0, self.ESP_ROW_X_WEIGHT, iY + self.ESP_ROW_Y_WEIGHT, self.ESP_Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

				szName = "AmountText%d" %(iPlayerID)
				szText = self.getEspionagePerTurnText(pActivePlayer.getEspionageSpending(iTargetTeam))

				screen.deleteWidget(szName)
				if bCompactLeaderRows:
					screen.setLabelAt( szName, attach, szText, CvUtil.FONT_RIGHT_JUSTIFY, iCompactXAmount, self.ESP_COMPACT_ROW_Y_TEXT, self.ESP_Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
				else:
					screen.setLabelAt( szName, attach, szText, 0, self.ESP_ROW_X_RIGHT, iY + self.ESP_ROW_Y_WEIGHT, self.ESP_Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

			# Is there any other players which have been met?
			if (self.ESP_iTargetPlayer != -1):

				pTargetPlayer = gc.getPlayer(self.ESP_iTargetPlayer)
				pyTargetPlayer = EspionagePyPlayer(self.ESP_iTargetPlayer)
				iTargetTeam = pTargetPlayer.getTeam() # advc.120d
				# List of Cities
				self.ESP_szCityListBox = self.getNextEspionageWidgetName()
				screen.addListBoxGFC(self.ESP_szCityListBox, "", self.ESP_X_CITY_LIST, self.ESP_Y_CITY_LIST, self.ESP_W_CITY_LIST, self.ESP_H_CITY_LIST, TableStyles.TABLE_STYLE_STANDARD)
				screen.enableSelect(self.ESP_szCityListBox, True)
				screen.setStyle(self.ESP_szCityListBox, "Table_StandardCiv_Style")

				# Loop through target's cities, see which are visible and add them to the list
				apCityList = pyTargetPlayer.getCityList()

				iLoop = 0

				for pyCity in apCityList:

					pCity = pyCity.GetCy()

					if (pCity.isRevealed(pActivePlayer.getTeam(), false)):
						appendListBoxStringLabel(screen, self.ESP_szCityListBox, pCity.getName(), WidgetTypes.WIDGET_GENERAL, pCity.getID(), 0, CvUtil.FONT_LEFT_JUSTIFY)

						if (self.ESP_iActiveCityID == -1 or pTargetPlayer.getCity(self.ESP_iActiveCityID).isNone()):
							self.ESP_iActiveCityID = pCity.getID()

						if (self.ESP_iActiveCityID == pCity.getID()):
							screen.setSelectedListBoxStringGFC(self.ESP_szCityListBox, iLoop)

						iLoop += 1

				#screen.updateListBox(self.ESP_szCityListBox)

				wTable0 = self.ESP_W_EFFECTS_LIST
				wTable1 = 0
				wTable2 = self.ESP_W_EFFECTS_COSTS_LIST
				wTable3 = 20

				szEffectsTable = self.getNextEspionageWidgetName()
				szHelpText = self.TEXT_ESPIONAGE_PASSIVE_AUTOMATIC
				screen.addTableControlGFCWithHelp(szEffectsTable, 4, self.ESP_X_EFFECTS_LIST, self.ESP_Y_EFFECTS_LIST, self.ESP_W_EFFECTS_LIST + self.ESP_W_EFFECTS_COSTS_LIST + wTable1 + wTable3, self.ESP_H_EFFECTS_LIST, False, False, 32,32, TableStyles.TABLE_STYLE_STANDARD, szHelpText)
				screen.setTableColumnHeader(szEffectsTable, 0, "", wTable0)
				screen.setTableColumnHeader(szEffectsTable, 1, "", wTable1)
				screen.setTableColumnHeader(szEffectsTable, 2, "", wTable2)
				screen.setTableColumnHeader(szEffectsTable, 3, "", wTable3)

				szMissionsTable = self.getNextEspionageWidgetName()
				szHelpText = self.TEXT_ESPIONAGE_MISSIONS_SPY
				# <advc.120d>
				if not pActivePlayer.canSpy():
					szHelpText = self.TEXT_ESPIONAGE_NO_SPY # </advc.120d>
				screen.addTableControlGFCWithHelp(szMissionsTable, 4, self.ESP_X_MISSIONS_LIST, self.ESP_Y_MISSIONS_LIST, self.ESP_W_MISSIONS_LIST + self.ESP_W_MISSIONS_COSTS_LIST + wTable1 + wTable3, self.ESP_H_MISSIONS_LIST, False, False, 32,32, TableStyles.TABLE_STYLE_STANDARD, szHelpText)
				screen.setTableColumnHeader(szMissionsTable, 0, "", wTable0)
				screen.setTableColumnHeader(szMissionsTable, 1, "", wTable1)
				screen.setTableColumnHeader(szMissionsTable, 2, "", wTable2)
				screen.setTableColumnHeader(szMissionsTable, 3, "", wTable3)

				# Loop through all Missions
				for iPass in range(3): # advc.120d
					for iMissionLoop in range(gc.getNumEspionageMissionInfos()):

						pMission = gc.getEspionageMissionInfo(iMissionLoop)
						if (pMission.getCost() == -1):
							continue # advc
						# <advc.120d>
						isSabotage = False
						if pMission.getDestroyProjectCostFactor() + pMission.getDestroyBuildingCostFactor() + pMission.getDestroyProductionCostFactor() > 0 or pMission.isDestroyImprovement():
							isSabotage = True
						isSteal = False
						if pMission.getBuyTechCostFactor() + pMission.getStealTreasuryTypes() > 0:
							isSteal = True
						if (iPass < 2) == isSabotage or (iPass > 0) == isSteal:
							continue
						# Moved up:
						pActiveCity = gc.getPlayer(self.ESP_iTargetPlayer).getCity(self.ESP_iActiveCityID)
						# </advc.120d>
						# Only passive effects
						if (pMission.isPassive()):

							pPlot = None

							if (self.ESP_iActiveCityID != -1 and pMission.isTargetsCity()):
								pPlot = pActiveCity.plot()

							if (self.ESP_iActiveCityID != -1 or not pMission.isTargetsCity()):

								iCost = pActivePlayer.getEspionageMissionCost(iMissionLoop, self.ESP_iTargetPlayer, pPlot, -1)

								szTechText = ""
								if (pMission.getTechPrereq() != -1):
									szTechText = " (%s)" %(gc.getTechInfo(pMission.getTechPrereq()).getDescription())

								szText = pMission.getDescription() + szTechText

								if (pMission.getTechPrereq() != -1):
									pTeam = gc.getTeam(pActivePlayer.getTeam())
									if (not pTeam.isHasTech(pMission.getTechPrereq())):
										szText = u"<color=255,0,0,0>%s</color>" %(szText)
								# <advc.120d>
								costStr = str(iCost)
								if pActivePlayer.canDoEspionageMission(iMissionLoop, self.ESP_iTargetPlayer, pPlot, 0):
									costStr = localText.changeTextColor(costStr, self.COLOR_GREEN)
								# </advc.120d>
								iRow = screen.appendTableRow(szEffectsTable)
								setTableTextLabel(screen, szEffectsTable, 0, iRow, szText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
								setTableTextLabel(screen, szEffectsTable, 2, iRow, costStr, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)

						# Active Mission
						else:
							# <advc.120d>
							if not pActivePlayer.canSpy():
								continue # </advc.120d>
							if (self.ESP_iSelectedMission == -1):
								self.ESP_iSelectedMission = iMissionLoop

							pPlot = -1
							if (self.ESP_iActiveCityID != -1):
								pPlot = pActiveCity.plot()
							else:
								pPlot = None

							iCost = pActivePlayer.getEspionageMissionCost(iMissionLoop, self.ESP_iTargetPlayer, pPlot, -1)

							szTechText = ""
							if (pMission.getTechPrereq() != -1):
								szTechText = " (%s)" %(gc.getTechInfo(pMission.getTechPrereq()).getDescription())
							# <advc.120d>
							elif pMission.getBuyTechCostFactor() > 0 and iCost > 0 and pActivePlayer.canSeeTech(self.ESP_iTargetPlayer):
								stealTech = pActivePlayer.getStealCostTech(self.ESP_iTargetPlayer)
								if stealTech >= 0:
									szTechText = " (%s)" %(gc.getTechInfo(stealTech).getDescription())
							missingPrereq = False
							# </advc.120d>
							szText = pMission.getDescription() + szTechText

							if (pMission.getTechPrereq() != -1):
								pTeam = gc.getTeam(pActivePlayer.getTeam())
								if (not pTeam.isHasTech(pMission.getTechPrereq())):
									szText = u"<color=255,0,0,0>%s</color>" %(szText)
									missingPrereq = True
							# <advc.120d>
							costStr = "(n/a)"
							if iCost > 0:
								costStr = str(iCost)
								if pActiveTeam.getEspionagePointsAgainstTeam(iTargetTeam) >= iCost:
									costStr = localText.changeTextColor(costStr, self.COLOR_GREEN)
							# Secret costs
							if pMission.getBuyTechCostFactor() > 0 and not pActivePlayer.canSeeTech(self.ESP_iTargetPlayer):
								# This can't happen so long as Alphabet enables both Spies and tech trading
								costStr = "?"
							# Display the Destroy missions as a single item
							elif isSabotage:
								if pMission.getDestroyProductionCostFactor() > 0:
									costStr = "?"
									szText = localText.getText("TXT_KEY_ESPIONAGE_SABOTAGE", ())
								elif pMission.getDestroyProjectCostFactor() <= 0 or iCost <= 0:
									continue
							if pMission.getStealTreasuryTypes() > 0 and iCost > 0:
								if not missingPrereq:
									iAmount = pActivePlayer.getEspionageGoldQuantity(iMissionLoop, self.ESP_iTargetPlayer, pActiveCity)
									if iAmount > 0:
										szText += " (%s)" %(localText.getText("TXT_KEY_ESPIONAGE_STEAL_AMOUNT", (iAmount,)))
								else: # Don't give away AI gold if the player can't actually steal gold
									costStr = "?"
							iRow = screen.appendTableRow(szMissionsTable)
							setTableTextLabel(screen, szMissionsTable, 2, iRow, costStr, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
							# The line below: Moved down </advc.120d>
							setTableTextLabel(screen, szMissionsTable, 0, iRow, szText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		return 0

	# returns a unique ID for a widget in this screen
	def getNextEspionageWidgetName(self):
		szName = self.ESP_WIDGET_ID + str(self.nEspionageWidgetCount)
		self.nEspionageWidgetCount += 1
		return szName

	def deleteAllEspionageWidgets(self):
		screen = self.getScreen()
		i = self.nEspionageWidgetCount - 1
		while (i >= 0):
			self.nEspionageWidgetCount = i
			screen.deleteWidget(self.getNextEspionageWidgetName())
			i -= 1

		self.nEspionageWidgetCount = 0

	# Will handle the input for this screen...
	def handleEspionageInput(self, inputClass):
		'Calls function mapped in EspionageAdvisorInputMap'

		screen = self.getScreen()

		##### Debug Dropdown #####
		if (CyGame().isDebugMode()):
			if (inputClass.getFunctionName() == self.ESP_DEBUG_DROPDOWN_ID):
				iIndex = screen.getSelectedPullDownID(self.ESP_DEBUG_DROPDOWN_ID)
				self.ESP_iActivePlayer = screen.getPullDownData(self.ESP_DEBUG_DROPDOWN_ID, iIndex)
				self.ESP_iTargetPlayer = -1 # advc.001d
				self.drawEspionageContents()
				CyInterface().setDirty(InterfaceDirtyBits.Espionage_Advisor_DIRTY_BIT, True)
		# advc.001d: Moved from above the Debug menu
		pActivePlayer = gc.getPlayer(self.ESP_iActivePlayer)
		bCompactLeaderRows = self.useCompactEspionageRows()

		if (self.ESP_iTargetPlayer != -1):

			##### Player Images #####
			if (inputClass.getData1() == self.ESP_iLeaderImagesID):

				self.ESP_iTargetPlayer = inputClass.getData2()

				# Loop through all images
				for iPlayerID in self.ESP_aiKnownPlayers:
					szName = "LeaderImage%d" %(iPlayerID)
					if (self.ESP_iTargetPlayer == iPlayerID):
						screen.setState(szName, true)
					else:
						screen.setState(szName, false)

					self.ESP_iActiveCityID = -1

				CyInterface().setDirty(InterfaceDirtyBits.Espionage_Advisor_DIRTY_BIT, True)

			##### City Listbox #####
			if ("%s%d" %(inputClass.getFunctionName(), inputClass.getID()) == self.ESP_szCityListBox):
				iCityID = inputClass.getData1()
				self.ESP_iActiveCityID = iCityID#gc.getPlayer(self.ESP_iTargetPlayer).getCity(iCityID)
				CyInterface().setDirty(InterfaceDirtyBits.Espionage_Advisor_DIRTY_BIT, True)

			# Is there any other players which have been met?
			if (self.ESP_iTargetPlayer != -1):

				##### Increase Button #####
				if (inputClass.getData1() == self.ESP_iIncreaseButtonID):

					iPlayerID = inputClass.getData2()
					iTargetTeam = gc.getPlayer(iPlayerID).getTeam()

					#CyMessageControl().sendEspionageSpendingWeightChange(iTargetTeam, 1)
					# K-Mod. Right click -> min/max
					if (inputClass.getFlags() & MouseFlags.MOUSE_RBUTTONUP):
						CyMessageControl().sendEspionageSpendingWeightChange(iTargetTeam, 99)
					else:
						CyMessageControl().sendEspionageSpendingWeightChange(iTargetTeam, 1)
					# K-Mod end

					szText = self.getEspionagePerTurnText(pActivePlayer.getEspionageSpending(iTargetTeam))

					if bCompactLeaderRows:
						iCompactXAmount = self.getCompactEspionageRowColumns()[2]
						screen.setLabelAt( "AmountText%d" %(iPlayerID), "LeaderContainer%d" % (iPlayerID), szText, CvUtil.FONT_RIGHT_JUSTIFY, iCompactXAmount, self.ESP_COMPACT_ROW_Y_TEXT, self.ESP_Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
					else:
						screen.setLabelAt( "AmountText%d" %(iPlayerID), "LeaderContainer%d" % (iPlayerID), szText, 0, self.ESP_ROW_X_RIGHT, 15 + self.ESP_ROW_Y_WEIGHT, self.ESP_Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

					CyInterface().setDirty(InterfaceDirtyBits.Espionage_Advisor_DIRTY_BIT, True)

				##### Decrease Button #####
				elif (inputClass.getData1() == self.ESP_iDecreaseButtonID):

					iPlayerID = inputClass.getData2()
					iTargetTeam = gc.getPlayer(iPlayerID).getTeam()

					if (pActivePlayer.getEspionageSpendingWeightAgainstTeam(iTargetTeam) > 0):	# Can't reduce weight below 0

						#CyMessageControl().sendEspionageSpendingWeightChange(iTargetTeam, -1)
						# K-Mod. Right click -> min/max
						if (inputClass.getFlags() & MouseFlags.MOUSE_RBUTTONUP):
							CyMessageControl().sendEspionageSpendingWeightChange(iTargetTeam, -99)
						else:
							CyMessageControl().sendEspionageSpendingWeightChange(iTargetTeam, -1)
						# K-Mod end

						szText = self.getEspionagePerTurnText(pActivePlayer.getEspionageSpending(iTargetTeam))

						if bCompactLeaderRows:
							iCompactXAmount = self.getCompactEspionageRowColumns()[2]
							screen.setLabelAt( "AmountText%d" %(iPlayerID), "LeaderContainer%d" % (iPlayerID), szText, CvUtil.FONT_RIGHT_JUSTIFY, iCompactXAmount, self.ESP_COMPACT_ROW_Y_TEXT, self.ESP_Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
						else:
							screen.setLabelAt( "AmountText%d" %(iPlayerID), "LeaderContainer%d" % (iPlayerID), szText, 0, self.ESP_ROW_X_RIGHT, 15 + self.ESP_ROW_Y_WEIGHT, self.ESP_Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

						CyInterface().setDirty(InterfaceDirtyBits.Espionage_Advisor_DIRTY_BIT, True)

		return 0

	# <advc.120c>
	def drawEspionageSlider(self):
		screen = self.getScreen()
		pActivePlayer = gc.getPlayer(self.ESP_iActivePlayer)

		eCommerce = CommerceTypes.COMMERCE_ESPIONAGE
		if not pActivePlayer.isCommerceFlexible(eCommerce):
			return

		# Copied from EconomicsAdvisor.py
		x = self.ESP_X_TOTAL_PANE
		y = self.ESP_Y_TOTAL_PANE
		if self.iCOMMERCE_PERCENT_CHANGE_INCREMENTS is None:
			self.iCOMMERCE_PERCENT_CHANGE_INCREMENTS = gc.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS")
		screen.setButtonGFC("plusButton", u"", "", x + self.ESP_TEXT_MARGIN, y + self.ESP_TEXT_MARGIN, 20, 20, WidgetTypes.WIDGET_CHANGE_PERCENT, eCommerce, self.iCOMMERCE_PERCENT_CHANGE_INCREMENTS, ButtonStyles.BUTTON_STYLE_CITY_PLUS )
		screen.setButtonGFC("minusButton", u"", "", x + self.ESP_TEXT_MARGIN + 24, y + self.ESP_TEXT_MARGIN, 20, 20, WidgetTypes.WIDGET_CHANGE_PERCENT, eCommerce, -self.iCOMMERCE_PERCENT_CHANGE_INCREMENTS, ButtonStyles.BUTTON_STYLE_CITY_MINUS )
		szText = sasFontTagLabel + gc.getCommerceInfo(eCommerce).getDescription() + u" (" + unicode(pActivePlayer.getCommercePercent(eCommerce)) + u"%)" + SAS_FONT_TAG_CLOSE
		screen.setLabel("espRateLabel", "Background",  szText, CvUtil.FONT_LEFT_JUSTIFY, x + self.ESP_TEXT_MARGIN + 50, y + self.ESP_TEXT_MARGIN, self.ESP_Z_CONTROLS + self.ESP_DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		szRate = sasFontTagLabel + unicode(pActivePlayer.getCommerceRate(CommerceTypes(eCommerce))) + SAS_FONT_TAG_CLOSE
		screen.setLabel("espRate", "Background", szRate, CvUtil.FONT_RIGHT_JUSTIFY, x + self.ESP_PANE_WIDTH - self.ESP_TEXT_MARGIN, y + self.ESP_TEXT_MARGIN, self.ESP_Z_CONTROLS + self.ESP_DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		# Update labels
		screen.deleteWidget("espRateLabel")
		screen.deleteWidget("espRate")
		# Same code as above, except for the buttons
		eCommerce = CommerceTypes.COMMERCE_ESPIONAGE
		szText = sasFontTagLabel + gc.getCommerceInfo(eCommerce).getDescription() + u" (" + unicode(pActivePlayer.getCommercePercent(eCommerce)) + u"%)" + SAS_FONT_TAG_CLOSE
		screen.setLabel("espRateLabel", "Background",  szText, CvUtil.FONT_LEFT_JUSTIFY, x + self.ESP_TEXT_MARGIN + 50, y + self.ESP_TEXT_MARGIN, self.ESP_Z_CONTROLS + self.ESP_DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		szRate = sasFontTagLabel + (u"<color=0,255,0,0>%s</color>" %(localText.getText("TXT_KEY_ESPIONAGE_NUM_EPS_PER_TURN", (pActivePlayer.getCommerceRate(CommerceTypes(eCommerce)), )))) + SAS_FONT_TAG_CLOSE
		screen.setLabel("espRate", "Background", szRate, CvUtil.FONT_RIGHT_JUSTIFY, x + self.ESP_PANE_WIDTH + self.ESP_TEXT_MARGIN, y + self.ESP_TEXT_MARGIN, self.ESP_Z_CONTROLS + self.ESP_DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
	# </advc.120c>

def smallText(text):
	return sasFontTagLabel + text + SAS_FONT_TAG_CLOSE

def smallSymbol(symbol):
	return smallText(FontUtil.getChar(symbol))
