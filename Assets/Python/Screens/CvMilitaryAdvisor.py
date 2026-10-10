## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
from CvPythonExtensions import *
import CvUtil
import ScreenInput
import time
import PyHelpers
import re
import SASBattleHistory
import SASTextScale
from SASFontUtils import *
from SASUtils import *

PyPlayer = PyHelpers.PyPlayer
PyInfo = PyHelpers.PyInfo

# globals
gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class CvMilitaryAdvisor:
	"Military Advisor"

	def __init__(self, screenId):

		self.screenId = screenId
		self.MILITARY_SCREEN_NAME = "MilitaryAdvisor"
		self.BACKGROUND_ID = "MilitaryAdvisorBackground"
		self.EXIT_ID = "MilitaryAdvisorExitWidget"

		self.WIDGET_ID = "MilitaryAdvisorWidget"
		self.REFRESH_WIDGET_ID = "MilitaryAdvisorRefreshWidget"
		self.ATTACH_WIDGET_ID = "MilitaryAdvisorAttachWidget"
		self.SELECTION_WIDGET_ID = "MilitaryAdvisorSelectionWidget"
		self.ATTACHED_WIDGET_ID = "MilitaryAdvisorAttachedWidget" # no need to explicitly delete these
		self.LEADER_BUTTON_ID = "MilitaryAdvisorLeaderButton"
		self.UNIT_PANEL_ID = "MilitaryAdvisorUnitPanel"
		self.MAP_MINIMAP_PANEL_ID = "MilitaryAdvisorMapMinimapPanel"
		self.UNIT_BUTTON_ID = "MilitaryAdvisorUnitButton"
		self.UNIT_BUTTON_LABEL_ID = "MilitaryAdvisorUnitButtonLabel"
		self.LEADER_PANEL_ID = "MilitaryAdvisorLeaderPanel"
		self.UNIT_LIST_ID = "MilitaryAdvisorUnitList"
		self.UNIT_ICON_ID = "MilitaryAdvisorUnitIcon"
		self.GREAT_GENERAL_BAR_ID = "MilitaryAdvisorGreatGeneralBar"
		self.GREAT_GENERAL_LABEL_ID = "MilitaryAdvisorGreatGeneralLabel"
		self.DEBUG_DROPDOWN_ID = "MilitaryAdvisorBattleDropdownWidget"
		self.BATTLE_TABLE_ID = "MilitaryAdvisorBattleTable"
		self.BATTLE_LOG_BUTTON_ID = "MilitaryAdvisorBattleLogButton"
		# <!-- custom: Civ3-style Summary tab.
		# The same support/composition/deployment numbers live elsewhere in Civ4 (Finance hover, Composition tab) but are scattered and discoverable only by hover; this tab consolidates them for diagnosis at a glance.
		# Each numeric label still binds the matching WIDGET_HELP_FINANCE_* widget where applicable so the engine's per-line breakdown hover stays available.
		# Named "Summary" rather than "Overview" to avoid confusion with the Domestic Advisor's OVERVIEW 1-4 tabs. (Claude code Opus 4.7) -->
		self.PAGE_SUMMARY = 0
		self.PAGE_MAP = 1
		self.PAGE_BATTLES = 2
		self.PAGE_COMPOSITION = 3
		# <!-- custom: Apply the XML default on this advisor object's first opening, then retain the selected tab across close/reopen within the Civ4 session. See KI#129 (Update). (GPT-5.6-Sol) -->
		self.iActivePage = None
		self.PAGE_TAB_IDS = ["MilitaryAdvisorTabButton0", "MilitaryAdvisorTabButton1", "MilitaryAdvisorTabButton2", "MilitaryAdvisorTabButton3"]
		self.PAGE_IDS = [self.PAGE_SUMMARY, self.PAGE_MAP, self.PAGE_BATTLES, self.PAGE_COMPOSITION]
		self.PAGE_LINK_WIDTH = [0, 0, 0, 0]
		self.SUMMARY_PANEL_ID = "MilitaryAdvisorSummaryPanel"
		self.COMPOSITION_UNITS_TABLE_ID = "MilitaryAdvisorCompositionUnitsTable"
		self.COMPOSITION_PROMOTIONS_TABLE_ID = "MilitaryAdvisorCompositionPromotionsTable"
		self.COMPOSITION_COMBATS_TABLE_ID = "MilitaryAdvisorCompositionCombatsTable"
		self.BATTLE_ROW_PLOTS = {}
		self.BATTLE_TABLE_CACHE_KEY = None

		self.Z_BACKGROUND = -2.1
		self.Z_CONTROLS = self.Z_BACKGROUND - 0.2
		self.DZ = -0.2

		# <!-- custom: keep screen-independent advisor shell constants in init; compute runtime resolution-dependent geometry in updateRuntimeLayout via SASUtils helpers. (GPT-5.3-Codex) -->
		self.W_LEFT_SPACE_FOR_COMMERCE_SLIDERS = SAS_ADVISOR_LEFT_SPACE_FOR_COMMERCE_SLIDERS
		self.W_RIGHT_SPACE_FOR_SCOREBOARD = SAS_ADVISOR_RIGHT_SPACE_FOR_SCOREBOARD
		self.H_TOP_SPACE_FOR_TECH_BAR = SAS_ADVISOR_TOP_SPACE_FOR_TECH_BAR
		self.H_BOTTOM_SPACE = SAS_ADVISOR_BOTTOM_SPACE
		self.Y_TITLE = SAS_ADVISOR_TITLE_Y

		self.BORDER_WIDTH = 4
		self.W_HELP_AREA = 200

		self.nWidgetCount = 0
		self.nRefreshWidgetCount = 0
		self.nAttachedWidgetCount = 0
		self.iActivePlayer = -1
		self.iAdvisorPerspectivePlayer = -1
		self.selectedPlayerList = []
		self.selectedGroupList = []
		self.selectedUnitList = []
		self.bMapMinimapInitDone = False
		self.iLanguageLoaded = -1

		# <!-- custom: expand/layout elements (e.g., great general bar) to use the new screen size. Credit: Gemini 3 Pro. (GPT-5.2-Codex (summarized)) -->
		self.SIDE_MARGIN = 40

		# --- A. Leaders Panel (Top - Full Width) ---
		self.X_LEADERS = 20
		self.Y_LEADERS = 70
		# <!-- custom: keep the Map tab as the single spatial unit browser.
		# A separate Map 2 tab with different minimap geometry had several tab-switch issues: stale dimensions, blank minimaps, or minimap bleed into other tabs depending on the attempted fix.
		# The stable compromise is to widen the existing unit list and preserve the individual-units toggle. (GPT-5.5) -->
		self.H_LEADERS = 380
		self.LEADER_BUTTON_SIZE = 64
		self.LEADER_BUTTON_SIZE_STEP = 8
		self.LEADER_MARGIN = 12

		self.bUnitDetails = False
		self.iShiftKeyDown = 0

		# --- B. Unit List (Right Side - Fixed Width) ---
		# <!-- custom: at 1080p, this allows 9 leader icons per row on the left; tune the unit panel width manually because it controls the remaining left-column width.
		# A long unit panel width gives budget for long unit names, many promotions, larger promotion icons, etc. (GPT-5.5) -->
		self.W_TEXT = 745

		# --- C. Minimap Panel (Left Side - Fills Remaining Space) ---
		self.X_MAP_MINIMAP = 20
		# <!-- custom: PANEL_MARGIN refers to the panel border wrap, not an empty margin. (GPT-5.2-Codex (summarized)) -->
		self.PANEL_MARGIN = 20

		# --- D. Great General Bar (Moved to Bottom Left)
		self.H_GREAT_GENERAL_BAR = 30
		self.X_GREAT_GENERAL_BAR = 20

		self.iSAS_CV_MILITARY_ADVISOR_DEFAULT_TAB = None
		self.IS_SAS_CV_MILITARY_ADVISOR_UNIT_COMBATS_UNITS_ICONS = None
		self.iSAS_CV_MILITARY_ADVISOR_INLINE_ICON_SIZE_BASE = None
		self.iSAS_CV_MILITARY_ADVISOR_INLINE_ICON_HIGH_RES_MIN_HEIGHT = None
		self.iSAS_CV_MILITARY_ADVISOR_INLINE_ICON_SIZE_HIGH_RES = None
		self.IS_SAS_CV_MILITARY_ADVISOR_BATTLE_PLOT_CONTEXT_ENABLE = None
		self.IS_SAS_CV_MILITARY_ADVISOR_BATTLES_LOG_BUTTON_ENABLE = None
		self.iINITIAL_OUTSIDE_UNIT_GOLD_PERCENT = None
		self.iNUM_BUILDING_AND_TECH_PREREQS = None
		# <!-- custom: cache Summary-tab military-building groups per civilization.
		# Building identities are discovered from civ-specific XML effects rather than named BuildingClasses; the helper applies only fixed UI row caps, so UBs and modmods remain data-driven without inventing a balance-sensitive ranking score. (ChatGPT-5.6-Sol) -->
		self.dSummaryMilitaryBuildingGroups = {}
		self.iUnitCombatMountedMelee = None
		self.iUnitCombatMountedRanged = None
		self.iBattleTerrainPeak = None
		self.iBattleTerrainHill = None
		self.COLOR_YELLOW = None
		self.COLOR_RED = None
		self.COLOR_GREEN = None
		self.COLOR_WHITE = None
		self.COLOR_GREAT_PEOPLE_STORED = None
		self.COLOR_GREAT_PEOPLE_RATE = None
		self.COLOR_EMPTY = None
		self.iPromoLeader = None
		self.BATTLE_HILL_PEAK_COL_ID = None
		self.BATTLE_TERRAIN_COL_ID = None
		self.BATTLE_FEATURE_COL_ID = None
		self.BATTLE_PLOT_COL_ID = None
		self.BATTLE_NUM_COLS = None

	def initDefines(self):
		# <!-- custom: Military Advisor was the remaining advisor here that read SAS XML UI defines directly in the constructor.
		# Changing defines while Civ4 was running could then produce crashy behaviour similar to hot-changing Python, unlike the Tech Chooser/Main Interface lazy/sentinel pattern used here.
		# Cache once after screen setup starts; empirically, this helper pattern fixed that crashy behaviour so runtime XML changes simply have no effect until the required Civ4 restart.
		# Check each cached define's sentinel instead of only the first one for cheap exhaustive safety. See KI#128. (GPT-5.5) -->
		if self.iSAS_CV_MILITARY_ADVISOR_DEFAULT_TAB is None:
			self.iSAS_CV_MILITARY_ADVISOR_DEFAULT_TAB = gc.getDefineINT("SAS_CV_MILITARY_ADVISOR_DEFAULT_TAB")
			if self.iSAS_CV_MILITARY_ADVISOR_DEFAULT_TAB not in self.PAGE_IDS:
				raise ValueError("SAS_CV_MILITARY_ADVISOR_DEFAULT_TAB must be a zero-based tab position from 0 to %d" % (len(self.PAGE_IDS) - 1))
		if self.IS_SAS_CV_MILITARY_ADVISOR_UNIT_COMBATS_UNITS_ICONS is None:
			self.IS_SAS_CV_MILITARY_ADVISOR_UNIT_COMBATS_UNITS_ICONS = (gc.getDefineINT("SAS_CV_MILITARY_ADVISOR_UNIT_COMBATS_UNITS_ICONS") > 0)
		if self.iSAS_CV_MILITARY_ADVISOR_INLINE_ICON_SIZE_BASE is None:
			self.iSAS_CV_MILITARY_ADVISOR_INLINE_ICON_SIZE_BASE = gc.getDefineINT("SAS_CV_MILITARY_ADVISOR_INLINE_ICON_SIZE_BASE")
		if self.iSAS_CV_MILITARY_ADVISOR_INLINE_ICON_HIGH_RES_MIN_HEIGHT is None:
			self.iSAS_CV_MILITARY_ADVISOR_INLINE_ICON_HIGH_RES_MIN_HEIGHT = gc.getDefineINT("SAS_CV_MILITARY_ADVISOR_INLINE_ICON_HIGH_RES_MIN_HEIGHT")
		if self.iSAS_CV_MILITARY_ADVISOR_INLINE_ICON_SIZE_HIGH_RES is None:
			self.iSAS_CV_MILITARY_ADVISOR_INLINE_ICON_SIZE_HIGH_RES = gc.getDefineINT("SAS_CV_MILITARY_ADVISOR_INLINE_ICON_SIZE_HIGH_RES")
		if self.IS_SAS_CV_MILITARY_ADVISOR_BATTLE_PLOT_CONTEXT_ENABLE is None:
			self.IS_SAS_CV_MILITARY_ADVISOR_BATTLE_PLOT_CONTEXT_ENABLE = (gc.getDefineINT("SAS_CV_MILITARY_ADVISOR_BATTLE_PLOT_CONTEXT_ENABLE") > 0)
		if self.IS_SAS_CV_MILITARY_ADVISOR_BATTLES_LOG_BUTTON_ENABLE is None:
			self.IS_SAS_CV_MILITARY_ADVISOR_BATTLES_LOG_BUTTON_ENABLE = (gc.getDefineINT("SAS_CV_MILITARY_ADVISOR_BATTLES_LOG_BUTTON_ENABLE") > 0)
		if self.iINITIAL_OUTSIDE_UNIT_GOLD_PERCENT is None:
			self.iINITIAL_OUTSIDE_UNIT_GOLD_PERCENT = gc.getDefineINT("INITIAL_OUTSIDE_UNIT_GOLD_PERCENT")
		if self.iNUM_BUILDING_AND_TECH_PREREQS is None:
			self.iNUM_BUILDING_AND_TECH_PREREQS = gc.getDefineINT("NUM_BUILDING_AND_TECH_PREREQS")
		# <!-- custom: resolve remaining required XML IDs strictly and check each sentinel separately for exhaustive lazy-init safety; military-building identities are intentionally discovered from XML effects instead of named classes. (Claude code Opus 4.7 + GPT-5.5 + ChatGPT-5.6-Sol) -->
		if self.iUnitCombatMountedMelee is None:
			self.iUnitCombatMountedMelee = getInfoTypeOrFail("UNITCOMBAT_MOUNTED_MELEE")
		if self.iUnitCombatMountedRanged is None:
			self.iUnitCombatMountedRanged = getInfoTypeOrFail("UNITCOMBAT_MOUNTED_RANGED")
		if self.iBattleTerrainPeak is None:
			self.iBattleTerrainPeak = getInfoTypeOrFail("TERRAIN_PEAK")
		if self.iBattleTerrainHill is None:
			self.iBattleTerrainHill = getInfoTypeOrFail("TERRAIN_HILL")
		if self.COLOR_YELLOW is None:
			self.COLOR_YELLOW = getInfoTypeOrFail("COLOR_YELLOW")
		if self.COLOR_RED is None:
			self.COLOR_RED = getInfoTypeOrFail("COLOR_RED")
		if self.COLOR_GREEN is None:
			self.COLOR_GREEN = getInfoTypeOrFail("COLOR_GREEN")
		if self.COLOR_WHITE is None:
			self.COLOR_WHITE = getInfoTypeOrFail("COLOR_WHITE")
		if self.COLOR_GREAT_PEOPLE_STORED is None:
			self.COLOR_GREAT_PEOPLE_STORED = getInfoTypeOrFail("COLOR_GREAT_PEOPLE_STORED")
		if self.COLOR_GREAT_PEOPLE_RATE is None:
			self.COLOR_GREAT_PEOPLE_RATE = getInfoTypeOrFail("COLOR_GREAT_PEOPLE_RATE")
		if self.COLOR_EMPTY is None:
			self.COLOR_EMPTY = getInfoTypeOrFail("COLOR_EMPTY")
		# <!-- custom: PROMOTION_LEADER is required for the Summary tab's GG-led units row; fail loudly like other AdvCiv-SAS XML contracts instead of silently showing misleading 0 data. (Claude code Opus 4.7, GPT-5.5) -->
		if self.iPromoLeader is None:
			self.iPromoLeader = getInfoTypeOrFail("PROMOTION_LEADER")
		# <!-- custom: Battle column IDs depend on the plot-context define, so derive them beside the define cache; this keeps callers from needing to know whether initDefines has already expanded the table layout. (GPT-5.5) -->
		if self.BATTLE_NUM_COLS is None:
			self.BATTLE_HILL_PEAK_COL_ID = -1
			self.BATTLE_TERRAIN_COL_ID = -1
			self.BATTLE_FEATURE_COL_ID = -1
			if self.IS_SAS_CV_MILITARY_ADVISOR_BATTLE_PLOT_CONTEXT_ENABLE:
				self.BATTLE_HILL_PEAK_COL_ID = 22
				self.BATTLE_TERRAIN_COL_ID = self.BATTLE_HILL_PEAK_COL_ID + 1
				self.BATTLE_FEATURE_COL_ID = self.BATTLE_HILL_PEAK_COL_ID + 2
				self.BATTLE_PLOT_COL_ID = self.BATTLE_HILL_PEAK_COL_ID + 3
			else:
				self.BATTLE_PLOT_COL_ID = 22
			self.BATTLE_NUM_COLS = self.BATTLE_PLOT_COL_ID + 1

	def initText(self):
		# <!-- custom: cache Military Advisor language-dependent text and deterministic art/symbol lookups once to avoid repeated calls on refresh paths.
		# Required info/color IDs are lazy strict caches in initDefines. (GPT-5.3-Codex, GPT-5.5) -->
		if not CyGame().isFinalInitialized():
			return
		if self.iLanguageLoaded == CyGame().getCurrentLanguage():
			return
		self.iLanguageLoaded = CyGame().getCurrentLanguage()
		self.EXIT_TEXT = sasFontTagTitle + localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT", ()).upper() + SAS_FONT_TAG_CLOSE
		self.TITLE = sasFontTagTitle.bold + localText.getText("TXT_KEY_MILITARY_ADVISOR_TITLE", ()).upper() + SAS_FONT_TAG_CLOSE
		self.TEXT_COMBAT_EXPERIENCE = localText.getText("TXT_KEY_MISC_COMBAT_EXPERIENCE", ())
		self.TEXT_ALL_UNITS = localText.getText("TXT_KEY_PEDIA_ALL_UNITS", ()).upper()
		self.TEXT_UNIT_TOGGLE_ON = localText.getText("TXT_KEY_MILITARY_ADVISOR_UNIT_TOGGLE_ON", ())
		self.TEXT_UNIT_TOGGLE_OFF = localText.getText("TXT_KEY_MILITARY_ADVISOR_UNIT_TOGGLE_OFF", ())
		self.TEXT_TAB_SUMMARY = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_TAB", ()).upper()
		self.TEXT_TAB_MAP = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_MAP_TAB", ()).upper()
		self.TEXT_TAB_BATTLES = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_BATTLES_TAB", ()).upper()
		self.TEXT_TAB_COMPOSITION = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_COMPOSITION_TAB", ()).upper()
		self.PAGE_NAME_LIST = [self.TEXT_TAB_SUMMARY, self.TEXT_TAB_MAP, self.TEXT_TAB_BATTLES, self.TEXT_TAB_COMPOSITION]
		self.TEXT_SUMMARY_SUPPORT = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_SUPPORT", ())
		self.TEXT_SUMMARY_ARMY = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_ARMY", ())
		self.TEXT_SUMMARY_DEPLOYMENT = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_DEPLOYMENT", ())
		self.TEXT_SUMMARY_TOTAL_UNITS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_TOTAL_UNITS", ())
		self.TEXT_SUMMARY_UNIT_COST = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_UNIT_COST", ())
		self.TEXT_SUMMARY_UNIT_SUPPLY = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_UNIT_SUPPLY", ())
		self.TEXT_SUMMARY_TOTAL_GOLD = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_TOTAL_GOLD", ())
		self.TEXT_SUMMARY_ANARCHY = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_ANARCHY", ())
		self.TEXT_SUMMARY_MIL_BUILDINGS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_MIL_BUILDINGS", ())
		self.TEXT_SUMMARY_UNIT_PRODUCTION = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_UNIT_PRODUCTION", ())
		self.TEXT_SUMMARY_AVG_MODIFIER = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_AVG_MODIFIER", ())
		self.TEXT_SUMMARY_BEST_CITY = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_BEST_CITY", ())
		self.TEXT_SUMMARY_AVG_NEW_XP = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_AVG_NEW_XP", ())
		self.TEXT_SUMMARY_BEST_NEW_XP = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_BEST_NEW_XP", ())
		self.TEXT_SUMMARY_DEFENSES = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_DEFENSES", ())
		self.TEXT_SUMMARY_BEST_DEFENDED = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_BEST_DEFENDED", ())
		self.TEXT_SUMMARY_AVG_DEFENSE = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_AVG_DEFENSE", ())
		self.TEXT_SUMMARY_ALLIED_HAMMERS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_ALLIED_HAMMERS", ())
		self.TEXT_SUMMARY_OUTSIDE_UNITS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_OUTSIDE_UNITS", ())
		self.TEXT_SUMMARY_FREE_USED_SHORT = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_FREE_USED_SHORT", ())
		self.TEXT_SUMMARY_FREE_USED_CAP_SHORT = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_FREE_USED_CAP_SHORT", ())
		self.TEXT_SUMMARY_PAID_SHORT = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_PAID_SHORT", ())
		self.TEXT_SUMMARY_MILITARY_SHORT = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_MILITARY_SHORT", ())
		self.TEXT_SUMMARY_COSTLESS_SHORT = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_COSTLESS_SHORT", ())
		self.TEXT_SUMMARY_GOLD_PER_UNIT = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_GOLD_PER_UNIT", ())
		self.TEXT_SUMMARY_GOLD_PER_MIL_UNIT = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_GOLD_PER_MIL_UNIT", ())
		self.TEXT_SUMMARY_EXTRA_COST = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_EXTRA_COST", ())
		self.TEXT_SUMMARY_GOLD_PER_OUTSIDE = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_GOLD_PER_OUTSIDE", ())
		self.TEXT_SUMMARY_INFLATION_SHORT = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_INFLATION_SHORT", ())
		self.TEXT_SUMMARY_MILITARY_UNITS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_MILITARY_UNITS", ())
		self.TEXT_SUMMARY_CIVILIAN_UNITS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_CIVILIAN_UNITS", ())
		self.TEXT_SUMMARY_LAND_UNITS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_LAND_UNITS", ())
		self.TEXT_SUMMARY_SEA_UNITS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_SEA_UNITS", ())
		self.TEXT_SUMMARY_AIR_UNITS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_AIR_UNITS", ())
		self.TEXT_SUMMARY_STRONGEST = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_STRONGEST", ())
		self.TEXT_SUMMARY_HIGHEST_COST = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_HIGHEST_COST", ())
		self.TEXT_SUMMARY_TOTAL_HAMMERS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_TOTAL_HAMMERS", ())
		self.TEXT_SUMMARY_AVG_XP = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_AVG_XP", ())
		self.TEXT_SUMMARY_AVG_LEVEL = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_AVG_LEVEL", ())
		self.TEXT_SUMMARY_AVG_HAMMERS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_AVG_HAMMERS", ())
		self.TEXT_SUMMARY_POWER_RANK_KNOWN = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_POWER_RANK_KNOWN", ())
		self.TEXT_SUMMARY_GENERALS_FREE = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_GENERALS_FREE", ())
		self.TEXT_SUMMARY_GENERALED = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_GENERALED", ())
		self.TEXT_SUMMARY_MAX_HEALTH = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_MAX_HEALTH", ())
		self.TEXT_SUMMARY_AVG_HEALTH = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_AVG_HEALTH", ())
		self.TEXT_SUMMARY_MIN_HEALTH = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_MIN_HEALTH", ())
		self.TEXT_SUMMARY_FAVORITE_PROMOS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_FAVORITE_PROMOS", ())
		self.TEXT_SUMMARY_COVERT_UNITS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_COVERT_UNITS", ())
		self.TEXT_SUMMARY_INVISIBLE = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_INVISIBLE", ())
		self.TEXT_SUMMARY_SPIES = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_SPIES", ())
		self.TEXT_SUMMARY_HIDDEN_NAT = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_HIDDEN_NAT", ())
		self.TEXT_SUMMARY_HEALTH_FULL = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_HEALTH_FULL", ())
		self.TEXT_SUMMARY_HEALTH_HIGH = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_HEALTH_HIGH", ())
		self.TEXT_SUMMARY_HEALTH_MEDIUM = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_HEALTH_MEDIUM", ())
		self.TEXT_SUMMARY_HEALTH_LOW = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_HEALTH_LOW", ())
		self.TEXT_SUMMARY_BATTLES_TOTAL = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_BATTLES_TOTAL", ())
		self.TEXT_SUMMARY_BATTLES_WON = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_BATTLES_WON", ())
		self.TEXT_SUMMARY_BATTLES_RETREATED = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_BATTLES_RETREATED", ())
		self.TEXT_SUMMARY_BATTLES_LOST = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_BATTLES_LOST", ())
		self.TEXT_SUMMARY_GOOD_LUCK = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_GOOD_LUCK", ())
		self.TEXT_SUMMARY_BAD_LUCK = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_BAD_LUCK", ())
		self.TEXT_SUMMARY_LUCKIEST_WIN = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_LUCKIEST_WIN", ())
		self.TEXT_SUMMARY_UNLUCKIEST_LOSS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_UNLUCKIEST_LOSS", ())
		self.TEXT_SUMMARY_UPGRADEABLE = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_UPGRADEABLE", ())
		self.TEXT_SUMMARY_UPGRADE_MIN_COST = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_UPGRADE_MIN_COST", ())
		self.TEXT_SUMMARY_UPGRADE_AVG_COST = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_UPGRADE_AVG_COST", ())
		self.TEXT_SUMMARY_UPGRADE_MAX_COST = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_UPGRADE_MAX_COST", ())
		self.TEXT_SUMMARY_UPGRADE_TOTAL_COST = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_UPGRADE_TOTAL_COST", ())
		self.TEXT_SUMMARY_WOUNDED = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_WOUNDED", ())
		self.TEXT_SUMMARY_IN_CITIES = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_IN_CITIES", ())
		self.TEXT_SUMMARY_IN_OWN_TERRITORY = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_IN_OWN_TERRITORY", ())
		self.TEXT_SUMMARY_IN_ALLIED = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_IN_ALLIED", ())
		self.TEXT_SUMMARY_IN_NEUTRAL = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_IN_NEUTRAL", ())
		self.TEXT_SUMMARY_IN_ENEMY = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_IN_ENEMY", ())
		self.TEXT_SUMMARY_IN_WILD = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_IN_WILD", ())
		self.TEXT_SUMMARY_AT_SEA = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_SUMMARY_AT_SEA", ())
		self.TEXT_COMPOSITION_UNITS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_COMPOSITION_UNITS", ())
		self.TEXT_COMPOSITION_PROMOTIONS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_COMPOSITION_PROMOTIONS", ())
		self.TEXT_COMPOSITION_COMBATS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_COMPOSITION_COMBATS", ())
		self.TEXT_COMPOSITION_COUNT = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_COMPOSITION_COUNT", ())
		self.TEXT_BATTLES_EMPTY = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_BATTLES_EMPTY", ())
		self.TEXT_BATTLES_LOG_BUTTON = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_BATTLES_LOG_BUTTON", ())
		self.TEXT_BATTLE_TURN = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_BATTLE_TURN", ())
		self.TEXT_BATTLE_YEAR = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_BATTLE_YEAR", ())
		self.TEXT_BATTLE_EST_ODDS = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_BATTLE_EST_ODDS", ())
		self.TEXT_BATTLE_ROLE = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_BATTLE_ROLE", ())
		self.TEXT_BATTLE_XP = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_BATTLE_XP", ())
		self.TEXT_BATTLE_OUR_UNIT = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_BATTLE_OUR_UNIT", ())
		self.TEXT_BATTLE_THEIR_UNIT = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_BATTLE_THEIR_UNIT", ())
		self.TEXT_BATTLE_CAPTURE_COUNT = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_BATTLE_CAPTURE_COUNT", ())
		self.TEXT_BATTLE_PID = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_BATTLE_PID", ())
		self.TEXT_BATTLE_HILL_PEAK = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_BATTLE_HILL_PEAK", ())
		self.TEXT_BATTLE_TERRAIN = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_BATTLE_TERRAIN", ())
		self.TEXT_BATTLE_FEATURE = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_BATTLE_FEATURE", ())
		self.TEXT_BATTLE_PLOT = localText.getText("TXT_KEY_SAS_MILITARY_ADVISOR_BATTLE_PLOT", ())
		self.ART_MAINMENU_SLIDESHOW_LOAD = ArtFileMgr.getInterfaceArtInfo("MAINMENU_SLIDESHOW_LOAD").getPath()
		self.ART_BUTTON_HILITE_SQUARE = ArtFileMgr.getInterfaceArtInfo("BUTTON_HILITE_SQUARE").getPath()
		self.ART_BATTLE_PLOT_BUTTON = ArtFileMgr.getInterfaceArtInfo("INTERFACE_MINIMAP_PING").getPath()
		self.ART_BATTLE_CITY_BUTTON = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTONS_CITYSELECTION").getPath()
		self.ART_BATTLE_CITY_CAPTURED_BUTTON = ArtFileMgr.getInterfaceArtInfo("INTERFACE_RESISTANCE").getPath()
		self.ART_BATTLE_ROLE_ATTACKER = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_CROSSED_SWORDS").getPath()
		# <!-- custom: tried a military medal emoji but it reads as 2 half emoji with highly clashing colors; hard to read in a data-rich small column width table.
		# Prefer the star emoji with darker variant that renders very well.
		# Trophy seems more suited to races or competitions so not used in military context here -->
		self.ART_BATTLE_RESULT_WON = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_WHITE_MEDIUM_STAR_2").getPath()
		# <!-- custom: a red plain fairly uniform emoji like this one conveys very well visually the idea of lost/death and scans very fast -->
		self.ART_BATTLE_RESULT_LOST = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_BROKEN_HEART").getPath()
		# <!-- custom: this left arrow variant is fairly clearly distinguishable from the other 2 result emojis; it is plain and dense so fills the cell nicely for retreat.
		# Person Running was not chosen because it is too thin and visually non-homogenous, making it harder to scan quickly. (GPT-5.5) -->
		self.ART_BATTLE_RESULT_RETREAT = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_LEFT_ARROW_2").getPath()
		self.RESULT_WON = 0
		self.RESULT_LOST = 1
		self.RESULT_RETREAT = 2
		self.STRENGTH_CHAR = u"%c" % CyGame().getSymbolID(FontSymbols.STRENGTH_CHAR)
		# <!-- custom: regular gold coin via CommerceInfo.getChar() (matches Domestic Advisor gold headers).
		# Earlier we used FontSymbols.BAD_GOLD_CHAR, the red maintenance-coin variant, which was visually wrong for normal cost subtotals.
		# Hammer comes from YIELD_PRODUCTION. (Claude code Opus 4.7, GPT-5.5) -->
		self.GOLD_CHAR = u"%c" % gc.getCommerceInfo(CommerceTypes.COMMERCE_GOLD).getChar()
		self.HAMMER_CHAR = u"%c" % gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar()
		self.GREAT_GENERAL_CHAR = u"%c" % CyGame().getSymbolID(FontSymbols.GREAT_GENERAL_CHAR)
		self.STAR_CHAR = u"%c" % CyGame().getSymbolID(FontSymbols.STAR_CHAR)

	def updateRuntimeLayout(self, screen):
		# <!-- custom: compute runtime shell bounds/anchors through shared helpers so Military Advisor follows the same runtime layout flow as Foreign/Info/Domestic. (GPT-5.3-Codex) -->
		self.X_SCREEN, self.Y_SCREEN, self.W_SCREEN, self.H_SCREEN = getAdvisorRuntimeBounds(screen, self.W_LEFT_SPACE_FOR_COMMERCE_SLIDERS, self.W_RIGHT_SPACE_FOR_SCOREBOARD, self.H_TOP_SPACE_FOR_TECH_BAR, self.H_BOTTOM_SPACE)
		self.X_TITLE, self.X_EXIT, self.Y_EXIT, _, self.Y_BOTTOM_PANEL = getAdvisorRuntimeAnchors(self.W_SCREEN, self.H_SCREEN)
		self.Y_LINK = self.Y_EXIT

		# <!-- custom: unit list panel scrolls often once a player has many units, so it is lifted to the top and the leader bar trimmed to accommodate it; the upscaled advisor screen has the lateral room, and the taller multi-row leader bar keeps full icon size longer despite the narrower width.
		# Leader-to-map-minimap vertical gap and bottom reserve are kept small because every pixel cut from H_MAP_MINIMAP_MAX shrinks W_MAP_MINIMAP via the world aspect ratio, which can reduce leader icons per row.
		# Excess horizontal space (typical at 1080p+) is split evenly across left margin / center gap / right margin so all three look balanced instead of tight-tight-huge.
		# The combat-experience bar tracks the Map tab minimap X / W to stay flush with it. (Claude code Opus 4.7 + GPT-5.5) -->
		iAdvisorMargin = 20
		iLeaderToMapMinimapGap = 5
		iBottomReserve = 15
		self.X_TEXT = self.W_SCREEN - iAdvisorMargin - self.W_TEXT
		self.Y_TEXT = self.Y_LEADERS
		self.H_TEXT = self.H_SCREEN - self.Y_TEXT - 55 - iAdvisorMargin

		self.Y_MAP_MINIMAP = self.Y_LEADERS + self.H_LEADERS + iLeaderToMapMinimapGap
		self.Y_GREAT_GENERAL_BAR = self.Y_BOTTOM_PANEL - self.H_GREAT_GENERAL_BAR - iBottomReserve
		self.H_MAP_MINIMAP_MAX = self.Y_GREAT_GENERAL_BAR - iLeaderToMapMinimapGap - self.Y_MAP_MINIMAP

		iGridW = CyMap().getGridWidth()
		iGridH = CyMap().getGridHeight()
		iMaxColW = self.W_SCREEN - 3 * iAdvisorMargin - self.W_TEXT
		self.W_MAP_MINIMAP = (self.H_MAP_MINIMAP_MAX * iGridW) / iGridH
		self.H_MAP_MINIMAP = self.H_MAP_MINIMAP_MAX
		if self.W_MAP_MINIMAP > iMaxColW:
			self.W_MAP_MINIMAP = iMaxColW
			self.H_MAP_MINIMAP = (self.W_MAP_MINIMAP * iGridH) / iGridW
		iGap = (self.W_SCREEN - self.W_MAP_MINIMAP - self.W_TEXT) / 3
		self.X_LEADERS = iGap
		self.X_MAP_MINIMAP = iGap
		self.X_GREAT_GENERAL_BAR = iGap
		self.X_TEXT = self.X_LEADERS + self.W_MAP_MINIMAP + iGap
		self.W_LEADERS = self.W_MAP_MINIMAP
		self.W_GREAT_GENERAL_BAR = self.W_MAP_MINIMAP

		# <!-- custom: inline icon size is resolution-dependent (vertical room), so compute once in runtime layout and reuse during row rendering. (GPT-5.3-Codex) -->
		self.iInlineIconSize = self.iSAS_CV_MILITARY_ADVISOR_INLINE_ICON_SIZE_BASE
		if self.iSAS_CV_MILITARY_ADVISOR_INLINE_ICON_HIGH_RES_MIN_HEIGHT > 0 and screen.getYResolution() >= self.iSAS_CV_MILITARY_ADVISOR_INLINE_ICON_HIGH_RES_MIN_HEIGHT:
			self.iInlineIconSize = self.iSAS_CV_MILITARY_ADVISOR_INLINE_ICON_SIZE_HIGH_RES
		# <!-- custom: Increase expanded-row promotion icons from the old DLL-hardcoded size 16 to our new Military Advisor row unit-icon size.
		# The row already has this height, so promotion icons become more readable without increasing row height. (GPT-5.5) -->
		self.iPromotionInlineIconSize = self.iInlineIconSize
		self.PAGE_LINK_WIDTH[:] = getAdvisorRuntimeLinkWidths(CyInterface(), self.PAGE_NAME_LIST, self.EXIT_TEXT, self.X_EXIT)

	def getScreen(self):
		return CyGInterfaceScreen(self.MILITARY_SCREEN_NAME, self.screenId)

	def hideScreen(self):
		screen = self.getScreen()
		screen.hideScreen()

	def setMapMinimapVisibility(self, screen, bVisible):
		# <!-- custom: refactor the existing minimap visibility toggle into a helper after Map 2 experiments; the separate Map 2 tab was dropped due to tab-switch minimap issues, but this helper keeps the remaining Map visibility call explicit and localized. (GPT-5.5) -->
		iOldMode = CyInterface().getShowInterface()
		if bVisible:
			CyInterface().setShowInterface(InterfaceVisibility.INTERFACE_MINIMAP_ONLY)
		else:
			CyInterface().setShowInterface(InterfaceVisibility.INTERFACE_HIDE)
		screen.updateMinimapVisibility()
		CyInterface().setShowInterface(iOldMode)

	def deleteAllWidgets(self):
		screen = self.getScreen()
		iCount = self.nWidgetCount
		self.nWidgetCount = 0
		while self.nWidgetCount < iCount:
			screen.deleteWidget(self.getNextWidgetName())
		self.nWidgetCount = 0
		# <!-- custom: For in-place Military Advisor tab redraws, delete page-owned widgets only; keep the screen shell and minimap frame alive because the Map tab minimap stopped rendering correctly after hide/show tab switches.
		# Pattern follows CvBUGMilitaryAdvisor's in-place tab rebuild. See KI#129. (GPT-5.5) -->
		for szWidget in (self.UNIT_PANEL_ID, self.UNIT_BUTTON_ID, self.UNIT_BUTTON_LABEL_ID, self.LEADER_PANEL_ID, self.UNIT_LIST_ID, self.GREAT_GENERAL_BAR_ID, self.GREAT_GENERAL_LABEL_ID, self.DEBUG_DROPDOWN_ID, self.BATTLE_LOG_BUTTON_ID, self.COMPOSITION_UNITS_TABLE_ID, self.COMPOSITION_PROMOTIONS_TABLE_ID, self.COMPOSITION_COMBATS_TABLE_ID, self.SUMMARY_PANEL_ID):
			screen.deleteWidget(szWidget)
		if self.BATTLE_TABLE_CACHE_KEY is None:
			screen.deleteWidget(self.BATTLE_TABLE_ID)
		else:
			screen.hide(self.BATTLE_TABLE_ID)
		for iPlayer in range(gc.getMAX_PLAYERS()):
			screen.deleteWidget(self.getLeaderButton(iPlayer))

	# Screen construction function
	def interfaceScreen(self):

		# Create a new screen
		screen = self.getScreen()
		if screen.isActive():
			return
		screen.setRenderInterfaceOnly(True)
		screen.showScreen(PopupStates.POPUPSTATE_IMMEDIATE, False)
		# <!-- custom: Keep Military Advisor non-persistent.
		# Empirically, screen.setPersistent(True) preserved expensive widgets such as the built Battles table across full advisor close/reopen, making very large late-game battle histories faster to reopen, but Civ4's native minimap did not survive that lifecycle reliably.
		# Within one open Military Advisor instance, tab switching remained lightweight and kept the Map tab working; the failure happened after exiting and reopening the advisor, which left the minimap area as an empty blue panel.
		# Debug mode triggered the same broken state, and turning debug mode back off did not restore it; only save reload did. See KI#129 (Update). (GPT-5.5-Thinking) -->
		# <!-- custom: note: Prefer always-correct Map tab rendering over the rarer benefit of preserving the Battles table across full advisor close/reopen. (GPT-5.5-Thinking) -->
		screen.setPersistent(False)

		self.initDefines()
		# <!-- custom: Reapplying the XML default on every real opening discarded the user's last selected tab.
		# Use it only on the first opening; this advisor object then retains later tab selections across close/reopen. See KI#129 (Update). (GPT-5.6-Sol) -->
		if self.iActivePage is None:
			self.iActivePage = self.iSAS_CV_MILITARY_ADVISOR_DEFAULT_TAB
		self.initText()
		self.updateRuntimeLayout(screen)
		self.bMapMinimapInitDone = False

		self.nWidgetCount = 0

		# Set the background and exit button, and show the screen
		# <!-- custom: unlike Foreign Advisor, we must set X/Y directly or the screen stays centered. Credit: Gemini 3 Pro. (GPT-5.2-Codex (summarized)) -->
		screen.setDimensions(self.X_SCREEN, self.Y_SCREEN, self.W_SCREEN, self.H_SCREEN)

		screen.addDDSGFC(self.BACKGROUND_ID, self.ART_MAINMENU_SLIDESHOW_LOAD, 0, 0, self.W_SCREEN, self.H_SCREEN, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		screen.addPanel( "TopPanel", u"", u"", True, False, 0, 0, self.W_SCREEN, 55, PanelStyles.PANEL_STYLE_TOPBAR )
		screen.addPanel( "BottomPanel", u"", u"", True, False, 0, self.Y_BOTTOM_PANEL, self.W_SCREEN, 55, PanelStyles.PANEL_STYLE_BOTTOMBAR )

		screen.showWindowBackground(False)
		screen.setText(self.EXIT_ID, "Background", self.EXIT_TEXT, CvUtil.FONT_RIGHT_JUSTIFY, self.X_EXIT, self.Y_EXIT, self.Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_CLOSE_SCREEN, -1, -1 )

		# Header...
		self.szHeader = self.getNextWidgetName()
		screen.setText(self.szHeader, "Background", self.TITLE, CvUtil.FONT_CENTER_JUSTIFY, self.X_TITLE, self.Y_TITLE, self.Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		self.drawActivePage()

	def redrawContents(self):
		self.deleteAllWidgets()
		self.szHeader = self.getNextWidgetName()
		self.getScreen().setText(self.szHeader, "Background", self.TITLE, CvUtil.FONT_CENTER_JUSTIFY, self.X_TITLE, self.Y_TITLE, self.Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		self.drawActivePage()

	def restoreAdvisorPerspectivePlayer(self):
		# <!-- custom: Map tab uses iActivePlayer for its unit-map/minimap perspective and resets it to the real active player.
		# Keep the dropdown perspective for Summary/Battles/Composition separately so selecting an AI or vassal is not lost after visiting Map or reopening this advisor. (GPT-5.5) -->
		if self.iAdvisorPerspectivePlayer >= 0:
			self.iActivePlayer = self.iAdvisorPerspectivePlayer
		self.iActivePlayer = getAdvisorValidPerspectivePlayer(self.iActivePlayer, bIncludeBarbarians=True, bAllowVassalPerspective=True)
		self.iAdvisorPerspectivePlayer = self.iActivePlayer

	# <!-- custom: Map-tab tab switches and close/reopen used to reset the leader bar to the active player, which made debug comparisons and quick unit-count checks tedious.
	# Keep selected leaders while they are still selectable; fall back to the active player only when selection is empty or invalid. (ChatGPT-5.5) -->
	def isMapSelectedPlayerValid(self, iPlayer):
		if iPlayer < 0 or iPlayer >= gc.getMAX_PLAYERS():
			return False
		pPlayer = gc.getPlayer(iPlayer)
		if not pPlayer.isAlive():
			return False
		if gc.getGame().isDebugMode():
			return True
		iActiveTeam = gc.getPlayer(gc.getGame().getActivePlayer()).getTeam()
		return gc.getTeam(pPlayer.getTeam()).isHasMet(iActiveTeam)

	def restoreMapSelectedPlayers(self):
		aOldSelectedPlayers = self.selectedPlayerList[:]
		self.selectedPlayerList = [iPlayer for iPlayer in self.selectedPlayerList if self.isMapSelectedPlayerValid(iPlayer)]
		if len(self.selectedPlayerList) == 0:
			self.selectedPlayerList.append(self.iActivePlayer)
		if self.selectedPlayerList != aOldSelectedPlayers:
			self.selectedGroupList = []

	def drawActivePage(self):
		screen = self.getScreen()
		self.drawTabs()

		if self.iActivePage == self.PAGE_SUMMARY:
			self.restoreAdvisorPerspectivePlayer()
			self.drawSummary()
			return

		if self.iActivePage == self.PAGE_BATTLES:
			self.restoreAdvisorPerspectivePlayer()
			self.drawBattleHistory()
			return

		if self.iActivePage == self.PAGE_COMPOSITION:
			self.restoreAdvisorPerspectivePlayer()
			self.drawComposition()
			return

		if self.iAdvisorPerspectivePlayer < 0:
			self.restoreAdvisorPerspectivePlayer()
		self.iActivePlayer = gc.getGame().getActivePlayer()

		# Minimap initialization
		# <!-- custom: Redraw Military Advisor tab contents in-place instead of hideScreen()+interfaceScreen() on tab switches; otherwise the Map tab minimap would stop rendering after returning from Battles/Composition.
		# Initialize the minimap frame once per real advisor opening, then refresh/re-front it when returning to Map. See KI#129. (GPT-5.5) -->
		if not self.bMapMinimapInitDone:
			screen.addPanel(self.MAP_MINIMAP_PANEL_ID, u"", "", False, False, self.X_MAP_MINIMAP, self.Y_MAP_MINIMAP, self.W_MAP_MINIMAP, self.H_MAP_MINIMAP, PanelStyles.PANEL_STYLE_MAIN)
			screen.initMinimap(self.X_MAP_MINIMAP + self.PANEL_MARGIN, self.X_MAP_MINIMAP + self.W_MAP_MINIMAP - self.PANEL_MARGIN, self.Y_MAP_MINIMAP + self.PANEL_MARGIN, self.Y_MAP_MINIMAP + self.H_MAP_MINIMAP - self.PANEL_MARGIN, self.Z_CONTROLS)
			self.bMapMinimapInitDone = True
		# <!-- custom: In debug mode the Map tab leader bar can select alive players outside the active player's revealed map.
		# Use the full minimap section then; otherwise a selected AI on another continent can have visible unit rows/overlays outside the drawn minimap area.
		# Normal play keeps the active player's known minimap section. See KI#142. (GPT-5.5?) -->
		screen.updateMinimapSection(gc.getGame().isDebugMode(), False)
		screen.updateMinimapColorFromMap(MinimapModeTypes.MINIMAPMODE_TERRITORY, 0.6)
		screen.setMinimapMode(MinimapModeTypes.MINIMAPMODE_MILITARY)

		self.setMapMinimapVisibility(screen, True)
		screen.bringMinimapToFront()

		self.unitsList = [(0, 0, [], 0)] * gc.getNumUnitInfos()
		self.selectedUnitList = []
		self.restoreMapSelectedPlayers()

		self.drawCombatExperience()

		self.refresh(true)

	def drawTabs(self):
		screen = self.getScreen()
		drawAdvisorFooterTabs(screen, self.PAGE_TAB_IDS, self.PAGE_NAME_LIST, self.PAGE_LINK_WIDTH, self.iActivePage, self.Y_LINK, self.Z_CONTROLS, self.COLOR_YELLOW)

	def getTurnDate(self, iTurn):
		iYear = CyGame().getTurnYear(iTurn)
		if iYear < 0:
			return localText.getText("TXT_KEY_TIME_BC", (-iYear,))
		return localText.getText("TXT_KEY_TIME_AD", (iYear,))

	def getBattleUnitStrength(self, iUnit):
		# <!-- custom: Battles table uses strength, not hammer cost; cost is less reliable for animals, spawned units, and edge cases with 0 or unusual production cost. (GPT-5.5) -->
		iCombat = gc.getUnitInfo(iUnit).getCombat()
		if iCombat <= 0:
			iCombat = gc.getUnitInfo(iUnit).getAirCombat()
		return iCombat

	def getBattleStoredStrengthText(self, iStrength):
		if iStrength > 0:
			return u"%.2f" % (iStrength / 100.0)
		return u""

	def getBattleEntryColumns(self, entry):
		if len(entry) < 7:
			return None
		iTurn, iWinner, iLoser, iWinnerUnit, iLoserUnit, iX, iY = entry[:7]
		if iWinnerUnit < 0 or iWinnerUnit >= gc.getNumUnitInfos() or iLoserUnit < 0 or iLoserUnit >= gc.getNumUnitInfos():
			return None
		if SASBattleHistory.isRetreatEntry(entry):
			iResult = self.RESULT_RETREAT
			iColor = self.COLOR_YELLOW
		elif iWinner == self.iActivePlayer:
			iResult = self.RESULT_WON
			iColor = self.COLOR_GREEN
		else:
			iResult = self.RESULT_LOST
			iColor = self.COLOR_RED
		iAttacker = -1
		iDefender = -1
		if len(entry) >= 9:
			iAttacker = entry[7]
			iDefender = entry[8]
		iAttackerCurrStr = 0
		iAttackerEndStr = 0
		iAttackerMaxStr = 0
		iDefenderCurrStr = 0
		iDefenderEndStr = 0
		iDefenderMaxStr = 0
		if len(entry) >= 13:
			iAttackerCurrStr = entry[9]
			iAttackerMaxStr = entry[10]
			iDefenderCurrStr = entry[11]
			iDefenderMaxStr = entry[12]
		if len(entry) >= 18:
			iAttackerEndStr = entry[16]
			iDefenderEndStr = entry[17]
		if iAttacker == iWinner and iDefender == iLoser:
			iAttackerUnit = iWinnerUnit
			iDefenderUnit = iLoserUnit
		elif iAttacker == iLoser and iDefender == iWinner:
			iAttackerUnit = iLoserUnit
			iDefenderUnit = iWinnerUnit
		else:
			# <!-- custom: battle rows saved before attacker/defender/strength capture only know winner/loser; keep them visible with winner first.
			# Combat-log current/max strength stays blank when unknown; XML base strength is shown in a separate column so we don't present invented effective combat values. (GPT-5.5) -->
			iAttacker = iWinner
			iDefender = iLoser
			iAttackerUnit = iWinnerUnit
			iDefenderUnit = iLoserUnit
		return (iTurn, iResult, iColor, iAttacker, iAttackerUnit, iAttackerCurrStr, iAttackerEndStr, iAttackerMaxStr, iDefender, iDefenderUnit, iDefenderCurrStr, iDefenderEndStr, iDefenderMaxStr, iX, iY)

	def getBattlePerspectiveColumns(self, entry):
		tColumns = self.getBattleEntryColumns(entry)
		if tColumns is None:
			return None
		iTurn, iResult, iColor, iAttacker, iAttackerUnit, iAttackerCurrStr, iAttackerEndStr, iAttackerMaxStr, iDefender, iDefenderUnit, iDefenderCurrStr, iDefenderEndStr, iDefenderMaxStr, iX, iY = tColumns
		# <!-- custom: The selected vassal remains the Battles subject, but outside debug mode the real active human is the map observer.
		# Sanitize location and stored plot context when that observer has not revealed the battle plot; this protects the table, LOG output and camera path together. See KI#335. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		iX, iY, iTerrain, iFeature, iHillPeakTerrain, iCityContext = self.getBattleObserverPlotContext(entry, iX, iY)
		iWinner = entry[1]
		iLoser = entry[2]
		bRoleKnown = False
		if len(entry) >= 9:
			iStoredAttacker = entry[7]
			iStoredDefender = entry[8]
			bRoleKnown = ((iStoredAttacker == iWinner and iStoredDefender == iLoser) or (iStoredAttacker == iLoser and iStoredDefender == iWinner))
		if iAttacker == self.iActivePlayer:
			iOurRole = -1
			if bRoleKnown:
				iOurRole = 1
			iAttackerXP, bAttackerGG, iDefenderXP, bDefenderGG = SASBattleHistory.getUnitContext(entry)
			return (iTurn, iResult, iColor, iOurRole, iAttackerUnit, iAttackerCurrStr, iAttackerEndStr, iAttackerMaxStr, iAttackerXP, bAttackerGG, iDefender, iDefenderUnit, iDefenderCurrStr, iDefenderEndStr, iDefenderMaxStr, iDefenderXP, bDefenderGG, iX, iY) + self.getBattleCapturePerspective(entry) + (iTerrain, iFeature, iHillPeakTerrain, iCityContext)
		if iDefender == self.iActivePlayer:
			iOurRole = -1
			if bRoleKnown:
				iOurRole = 0
			iAttackerXP, bAttackerGG, iDefenderXP, bDefenderGG = SASBattleHistory.getUnitContext(entry)
			return (iTurn, iResult, iColor, iOurRole, iDefenderUnit, iDefenderCurrStr, iDefenderEndStr, iDefenderMaxStr, iDefenderXP, bDefenderGG, iAttacker, iAttackerUnit, iAttackerCurrStr, iAttackerEndStr, iAttackerMaxStr, iAttackerXP, bAttackerGG, iX, iY) + self.getBattleCapturePerspective(entry) + (iTerrain, iFeature, iHillPeakTerrain, iCityContext)
		return None

	# <!-- custom: Centralize subject-versus-observer plot privacy so every Battles consumer receives the same permitted coordinates and context. See KI#335. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	def getBattleObserverPlotContext(self, entry, iX, iY):
		iTerrain, iFeature, iHillPeakTerrain, iCityContext = SASBattleHistory.getPlotContext(entry)
		if CyGame().isDebugMode():
			return (iX, iY, iTerrain, iFeature, iHillPeakTerrain, iCityContext)
		kMap = CyMap()
		if 0 <= iX < kMap.getGridWidth() and 0 <= iY < kMap.getGridHeight():
			pPlot = kMap.plot(iX, iY)
			iObserverTeam = gc.getPlayer(CyGame().getActivePlayer()).getTeam()
			if pPlot is not None and not pPlot.isNone() and pPlot.isRevealed(iObserverTeam, False):
				return (iX, iY, iTerrain, iFeature, iHillPeakTerrain, iCityContext)
		return (-1, -1, -1, -1, -1, 0)

	def getBattleCapturePerspective(self, entry):
		if len(entry) < 16:
			return (0, -1)
		iCapturingPlayer = entry[13]
		iCapturedUnit = entry[14]
		iCapturedCount = entry[15]
		if iCapturedCount <= 0 or iCapturedUnit < 0 or iCapturedUnit >= gc.getNumUnitInfos():
			return (0, -1)
		if iCapturingPlayer == self.iActivePlayer:
			return (iCapturedCount, iCapturedUnit)
		return (-iCapturedCount, iCapturedUnit)

	def getBattleCaptureCountText(self, iCapturedCount):
		if iCapturedCount == 0:
			return u""
		szText = unicode(abs(iCapturedCount))
		if iCapturedCount > 0:
			return localText.changeTextColor(szText, self.COLOR_GREEN)
		return localText.changeTextColor(szText, self.COLOR_RED)

	def getBattleCaptureUnitText(self, iCapturedCount, iCapturedUnit):
		if iCapturedCount == 0 or iCapturedUnit < 0:
			return u""
		return gc.getUnitInfo(iCapturedUnit).getDescription()

	def getBattleCaptureUnitButton(self, iCapturedCount, iCapturedUnit):
		if iCapturedCount == 0 or iCapturedUnit < 0:
			return ""
		return gc.getUnitInfo(iCapturedUnit).getButton()

	def getBattleXPText(self, iXP, bGreatGeneral):
		if iXP < 0:
			return u""
		szText = unicode(iXP)
		if bGreatGeneral:
			return localText.changeTextColor(szText, self.COLOR_YELLOW)
		return szText

	def setBattleTerrainFeatureCell(self, screen, iRow, iCol, iInfo, bTerrain):
		szSortKey = getAdvisorIconSortKey(iInfo + 1, iRow)
		if bTerrain:
			if iInfo >= 0 and iInfo < gc.getNumTerrainInfos():
				SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, iCol, iRow, szSortKey, gc.getTerrainInfo(iInfo).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_TERRAIN, iInfo, -1, CvUtil.FONT_CENTER_JUSTIFY)
				return
		else:
			if iInfo >= 0 and iInfo < gc.getNumFeatureInfos():
				SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, iCol, iRow, szSortKey, gc.getFeatureInfo(iInfo).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_FEATURE, iInfo, -1, CvUtil.FONT_CENTER_JUSTIFY)
				return
		SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, iCol, iRow, getAdvisorIconSortKey(0, iRow), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def setBattleHillPeakCell(self, screen, iRow, iHillPeakTerrain):
		# <!-- custom: show only peak/hill here: flat/water have no separate pedia redirects and are already visible through terrain, while peak/hill are separate combat-relevant map properties with Sevopedia entries/icons. (GPT-5.5) -->
		if iHillPeakTerrain == self.iBattleTerrainPeak or iHillPeakTerrain == self.iBattleTerrainHill:
			SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, self.BATTLE_HILL_PEAK_COL_ID, iRow, getAdvisorIconSortKey(iHillPeakTerrain + 1, iRow), gc.getTerrainInfo(iHillPeakTerrain).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_TERRAIN, iHillPeakTerrain, -1, CvUtil.FONT_CENTER_JUSTIFY)
			return
		SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, self.BATTLE_HILL_PEAK_COL_ID, iRow, getAdvisorIconSortKey(0, iRow), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def getBattleTerrainTypeText(self, iTerrain):
		if iTerrain >= 0 and iTerrain < gc.getNumTerrainInfos():
			return gc.getTerrainInfo(iTerrain).getType()
		return ""

	def getBattleFeatureTypeText(self, iFeature):
		if iFeature >= 0 and iFeature < gc.getNumFeatureInfos():
			return gc.getFeatureInfo(iFeature).getType()
		return ""

	# <!-- custom: Coordinates now accompany city context so an observer-hidden row has no clickable Plot art or apparent camera action. See KI#335. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	def getBattlePlotButton(self, iCityContext, iX, iY):
		if iX < 0 or iY < 0:
			return ""
		if SASBattleHistory.isCityContextCaptured(iCityContext):
			return self.ART_BATTLE_CITY_CAPTURED_BUTTON
		if SASBattleHistory.isCityContextDefended(iCityContext):
			return self.ART_BATTLE_CITY_BUTTON
		return self.ART_BATTLE_PLOT_BUTTON

	def getBattleCurrentStrengthText(self, iCurrentStrength, iMaxStrength):
		szText = self.getBattleStoredStrengthText(iCurrentStrength)
		if not szText or iMaxStrength <= 0:
			return szText
		iPct = (100 * iCurrentStrength) / iMaxStrength
		if iCurrentStrength >= iMaxStrength:
			return szText
		if iPct > 66:
			return localText.changeTextColor(szText, self.COLOR_GREEN)
		if iPct > 33:
			return localText.changeTextColor(szText, self.COLOR_YELLOW)
		return localText.changeTextColor(szText, self.COLOR_RED)

	def getBattleBaseStrengthText(self, iUnit):
		iStrength = self.getBattleUnitStrength(iUnit)
		if iStrength > 0:
			return unicode(iStrength)
		return u""

	def getBattleEstimatedOddsText(self, iOurStrength, iTheirStrength):
		# <!-- custom: estimate row-level combat favorability from the two stored start-effective strengths only; this is not exact Civ4 odds because first strikes, withdrawal, damage rounds, etc. are not modeled here. (GPT-5.5) -->
		iTotalStrength = iOurStrength + iTheirStrength
		if iOurStrength <= 0 or iTheirStrength <= 0 or iTotalStrength <= 0:
			return u""
		return unicode((100 * iOurStrength + (iTotalStrength / 2)) / iTotalStrength)

	def getBattleRoleArt(self, iOurRole):
		# <!-- custom: empty string (not unicode) because the szIcon arg of CyGInterfaceScreen.setTableText is bound as char const * (str); passing unicode triggers an ArgumentError. Credit: Claude code Opus 4.7. (GPT-5.5) -->
		if iOurRole == 1:
			return self.ART_BATTLE_ROLE_ATTACKER
		return ""

	def getBattleResultArt(self, iResult):
		if iResult == self.RESULT_WON:
			return self.ART_BATTLE_RESULT_WON
		if iResult == self.RESULT_LOST:
			return self.ART_BATTLE_RESULT_LOST
		if iResult == self.RESULT_RETREAT:
			return self.ART_BATTLE_RESULT_RETREAT
		return ""

	def getBattleResultLogText(self, iResult):
		# <!-- custom: dev-only PythonDbg log dump labels: hardcoded English (no localization round-trip via TXT_KEY) since logs are dev/diagnostic and never user-facing. Credit: Claude code Opus 4.7. (GPT-5.5) -->
		if iResult == self.RESULT_WON:
			return u"Won"
		if iResult == self.RESULT_LOST:
			return u"Lost"
		if iResult == self.RESULT_RETREAT:
			return u"Ret."
		return u""

	def getBattleRoleText(self, iOurRole):
		# <!-- custom: text variant kept for the PythonDbg.log dump because stdout cannot render table art icons. (GPT-5.5) -->
		if iOurRole == 1:
			return u"A"
		return u""

	def getBattleTableCacheKey(self, aEntries, iTableX, iTableY, iTableW, iTableH):
		entryFirst = None
		entryLast = None
		if aEntries:
			entryFirst = aEntries[0]
			entryLast = aEntries[-1]
		# <!-- custom: Observer identity and debug mode control battle-location privacy, so include both in the preserved table's cache key rather than reusing rows built under another visibility context. See KI#335. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		return (self.iActivePlayer, CyGame().getActivePlayer(), CyGame().isDebugMode(), CyGame().getCurrentLanguage(), self.IS_SAS_CV_MILITARY_ADVISOR_BATTLE_PLOT_CONTEXT_ENABLE, self.BATTLE_NUM_COLS, iTableX, iTableY, iTableW, iTableH, len(aEntries), entryFirst, entryLast)

	def canReuseBattleTableWidget(self, screen, aEntries):
		# <!-- custom: Cache keys can survive Military Advisor close/reopen even when the table widget is gone; verify the preserved table still has enough rows before using the fast show/moveToFront path. (GPT-5.5) -->
		iExpectedRows = len(aEntries)
		if iExpectedRows < 1:
			iExpectedRows = 1
		try:
			return screen.getTableNumRows(self.BATTLE_TABLE_ID) >= iExpectedRows
		except:
			return False

	def drawBattleHistory(self):
		screen = self.getScreen()
		# <!-- custom: Map tab has its own leader buttons, but Battles has no player selector without this shared debug/vassal dropdown; include Barbarians because battle history can involve barbarian/animal units. (GPT-5.5) -->
		addAdvisorDebugDropdown(screen, self.DEBUG_DROPDOWN_ID, self.iActivePlayer, bIncludeBarbarians=True, bAllowVassalPerspective=True)
		(iX, iY, iW, iH), (iTableX, iTableY, iTableW, iTableH) = getAdvisorMaximizedPanelLayout(self.W_SCREEN, self.Y_BOTTOM_PANEL)
		screen.addPanel(self.UNIT_PANEL_ID, "", "", True, True, iX, iY, iW, iH, PanelStyles.PANEL_STYLE_MAIN)
		aEntries = SASBattleHistory.getEntriesForPlayer(self.iActivePlayer)
		battleTableCacheKey = self.getBattleTableCacheKey(aEntries, iTableX, iTableY, iTableW, iTableH)
		if self.IS_SAS_CV_MILITARY_ADVISOR_BATTLES_LOG_BUTTON_ENABLE:
			screen.setButtonGFC(self.BATTLE_LOG_BUTTON_ID, sasFontTagLabel + self.TEXT_BATTLES_LOG_BUTTON.upper() + SAS_FONT_TAG_CLOSE, "", self.X_EXIT - 110, self.Y_TITLE + 2, 64, 28, WidgetTypes.WIDGET_GENERAL, -1, -1, ButtonStyles.BUTTON_STYLE_STANDARD)
		placeAdvisorLegendLink(self, "CONCEPT_SAS_MILITARY_ADVISOR_BATTLES_LEGEND", self.W_SCREEN - 12, self.Y_TITLE)
		# <!-- custom: Late-game Battles tab opened slowly in a sample with 710+ rows.
		# Data-layer caching and fully prepared row-payload caching were tested first and did not significantly help; preserving the built table widget makes same-open tab switches almost instant as long as the Military Advisor is not exited.
		# Do not make the whole Military Advisor screen persistent just to preserve this table across full advisor close/reopen: testing showed that persistence breaks Civ4's native Map tab minimap after advisor exit/reopen and debug-mode transitions.
		# Rebuild Battles after full close/reopen; keep the lightweight cache for tab switches within one advisor opening. See KI#129. (GPT-5.5-Thinking) -->
		if self.BATTLE_TABLE_CACHE_KEY == battleTableCacheKey and self.canReuseBattleTableWidget(screen, aEntries):
			screen.show(self.BATTLE_TABLE_ID)
			screen.moveToFront(self.BATTLE_TABLE_ID)
			return
		screen.deleteWidget(self.BATTLE_TABLE_ID)
		self.BATTLE_TABLE_CACHE_KEY = battleTableCacheKey
		self.BATTLE_ROW_PLOTS = {}
		screen.addTableControlGFC(self.BATTLE_TABLE_ID, self.BATTLE_NUM_COLS, iTableX, iTableY, iTableW, iTableH, True, True, 24, 24, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSort(self.BATTLE_TABLE_ID)
		screen.setStyle(self.BATTLE_TABLE_ID, "Table_StandardCiv_Style")
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 0, self.TEXT_BATTLE_TURN, 55)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 1, self.TEXT_BATTLE_YEAR, 104)
		iResultColW = 35
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 2, u"R", iResultColW)
		iPlotColW = 35
		# <!-- custom: use one shared 2-digit-friendly width for compact numeric columns (E%, base strength, XP, PID, Cap#).
		# Values like 100%, 3-digit strength, or 3-digit XP may clip, but this preserves the plot-camera column when the vertical scrollbar appears. (GPT-5.5) -->
		iTwoDigitColW = 38
		iEstOddsColW = iTwoDigitColW
		iRoleColW = 35
		iBaseColW = iTwoDigitColW
		iXPColW = iTwoDigitColW
		iPIDColW = iTwoDigitColW
		iCaptureCountColW = iPIDColW
		iCaptureUnitColW = 35
		iTerrainColW = 0
		iFeatureColW = 0
		iHillPeakColW = 0
		if self.IS_SAS_CV_MILITARY_ADVISOR_BATTLE_PLOT_CONTEXT_ENABLE:
			iTerrainColW = 35
			iFeatureColW = 35
			iHillPeakColW = 35
		iStrengthColW = 78
		# <!-- custom: reserve right-side width for the table scrollbar gutter so unit-name text does not clip under it; Battles fills fast so the scrollbar is the common case.
		# Matches CvInfoScreen's 14px convention. (Claude code Opus 4.7) -->
		iScrollbarGutterW = 14
		iUnitColBudget = iTableW - iScrollbarGutterW
		iUnitColW = max(105, (iUnitColBudget - 55 - 105 - iResultColW - iEstOddsColW - iRoleColW - iBaseColW - iStrengthColW - iStrengthColW - iStrengthColW - iXPColW - 35 - 35 - iPIDColW - iBaseColW - iStrengthColW - iStrengthColW - iStrengthColW - iXPColW - iCaptureCountColW - iCaptureUnitColW - iHillPeakColW - iTerrainColW - iFeatureColW - iPlotColW) / 2)
		# <!-- custom: show battles from the inspected player's perspective: our unit is always on the left and their unit follows immediately, so each row reads as a direct matchup.
		# Our repeated civ/leader/PID are omitted because the dropdown/log PerspectivePlayer already identifies us. (GPT-5.5) -->
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 3, self.TEXT_BATTLE_EST_ODDS, iEstOddsColW)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 4, self.TEXT_BATTLE_ROLE, iRoleColW)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 5, self.STRENGTH_CHAR + u"b", iBaseColW)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 6, self.STRENGTH_CHAR + u"s", iStrengthColW)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 7, self.STRENGTH_CHAR + u"e", iStrengthColW)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 8, self.STRENGTH_CHAR + u"m", iStrengthColW)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 9, self.TEXT_BATTLE_XP, iXPColW)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 10, self.TEXT_BATTLE_OUR_UNIT, iUnitColW)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 11, self.TEXT_BATTLE_THEIR_UNIT, iUnitColW)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 12, self.TEXT_BATTLE_XP, iXPColW)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 13, self.STRENGTH_CHAR + u"b", iBaseColW)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 14, self.STRENGTH_CHAR + u"s", iStrengthColW)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 15, self.STRENGTH_CHAR + u"e", iStrengthColW)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 16, self.STRENGTH_CHAR + u"m", iStrengthColW)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 17, "", 35)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 18, "", 35)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 19, self.TEXT_BATTLE_PID, iPIDColW)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 20, self.TEXT_BATTLE_CAPTURE_COUNT, iCaptureCountColW)
		# <!-- custom: show captured unit type as a compact pedia icon rather than text.
		# The stored row supports one captured unit type; if a future mod allows one combat to create multiple captured types, expand this column/model then. (GPT-5.5) -->
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, 21, "", iCaptureUnitColW)
		if self.IS_SAS_CV_MILITARY_ADVISOR_BATTLE_PLOT_CONTEXT_ENABLE:
			SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, self.BATTLE_HILL_PEAK_COL_ID, self.TEXT_BATTLE_HILL_PEAK, iHillPeakColW)
			SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, self.BATTLE_TERRAIN_COL_ID, self.TEXT_BATTLE_TERRAIN, iTerrainColW)
			SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, self.BATTLE_FEATURE_COL_ID, self.TEXT_BATTLE_FEATURE, iFeatureColW)
		SASTextScale.setTableColumnHeaderLabel(screen, self.BATTLE_TABLE_ID, self.BATTLE_PLOT_COL_ID, self.TEXT_BATTLE_PLOT, iPlotColW)
		if not aEntries:
			iRow = screen.appendTableRow(self.BATTLE_TABLE_ID)
			SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, 1, iRow, self.TEXT_BATTLES_EMPTY, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			return
		for entry in reversed(aEntries):
			self.addBattleHistoryTableRow(screen, entry)

	def addBattleHistoryTableRow(self, screen, entry):
		tColumns = self.getBattlePerspectiveColumns(entry)
		if tColumns is None:
			return
		iTurn, iResult, iColor, iOurRole, iOurUnit, iOurCurrStr, iOurEndStr, iOurMaxStr, iOurXP, bOurGG, iTheirPlayer, iTheirUnit, iTheirCurrStr, iTheirEndStr, iTheirMaxStr, iTheirXP, bTheirGG, iX, iY, iCapturedCount, iCapturedUnit, iTerrain, iFeature, iHillPeakTerrain, iCityContext = tColumns
		iRow = screen.appendTableRow(self.BATTLE_TABLE_ID)
		self.BATTLE_ROW_PLOTS[iRow] = (iX, iY)
		kTheirPlayer = gc.getPlayer(iTheirPlayer)
		iTheirCiv = kTheirPlayer.getCivilizationType()
		iTheirLeader = kTheirPlayer.getLeaderType()
		eCapturedUnitWidget = WidgetTypes.WIDGET_GENERAL
		if iCapturedUnit >= 0:
			eCapturedUnitWidget = WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT
		SASTextScale.setTableIntLabel(screen, self.BATTLE_TABLE_ID, 0, iRow, str(iTurn), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, 1, iRow, self.getTurnDate(iTurn), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		# <!-- custom: show result as compact icon art in the table to save width and reduce colored-text overload; keep Won/Lost/Ret.
		# text in the PythonDbg.log dump below. (GPT-5.5) -->
		SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, 2, iRow, getAdvisorIconSortKey((iResult + 1) * 10, iRow), self.getBattleResultArt(iResult), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
		SASTextScale.setTableIntLabel(screen, self.BATTLE_TABLE_ID, 3, iRow, self.getBattleEstimatedOddsText(iOurCurrStr, iTheirCurrStr), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, 4, iRow, getAdvisorIconSortKey(iOurRole + 1, iRow), self.getBattleRoleArt(iOurRole), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
		SASTextScale.setTableIntLabel(screen, self.BATTLE_TABLE_ID, 5, iRow, self.getBattleBaseStrengthText(iOurUnit), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, 6, iRow, self.getBattleCurrentStrengthText(iOurCurrStr, iOurMaxStr), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, 7, iRow, self.getBattleCurrentStrengthText(iOurEndStr, iOurMaxStr), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, 8, iRow, self.getBattleStoredStrengthText(iOurMaxStr), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		SASTextScale.setTableIntLabel(screen, self.BATTLE_TABLE_ID, 9, iRow, self.getBattleXPText(iOurXP, bOurGG), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		szOurUnitName = gc.getUnitInfo(iOurUnit).getDescription()
		szTheirUnitName = gc.getUnitInfo(iTheirUnit).getDescription()
		if bOurGG:
			szOurUnitName = localText.changeTextColor(szOurUnitName, self.COLOR_YELLOW)
		if bTheirGG:
			szTheirUnitName = localText.changeTextColor(szTheirUnitName, self.COLOR_YELLOW)
		SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, 10, iRow, szOurUnitName, gc.getUnitInfo(iOurUnit).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iOurUnit, -1, CvUtil.FONT_LEFT_JUSTIFY)
		SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, 11, iRow, szTheirUnitName, gc.getUnitInfo(iTheirUnit).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iTheirUnit, -1, CvUtil.FONT_LEFT_JUSTIFY)
		SASTextScale.setTableIntLabel(screen, self.BATTLE_TABLE_ID, 12, iRow, self.getBattleXPText(iTheirXP, bTheirGG), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		SASTextScale.setTableIntLabel(screen, self.BATTLE_TABLE_ID, 13, iRow, self.getBattleBaseStrengthText(iTheirUnit), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, 14, iRow, self.getBattleCurrentStrengthText(iTheirCurrStr, iTheirMaxStr), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, 15, iRow, self.getBattleCurrentStrengthText(iTheirEndStr, iTheirMaxStr), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, 16, iRow, self.getBattleStoredStrengthText(iTheirMaxStr), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, 17, iRow, getAdvisorIconSortKey(iTheirCiv + 1, iRow), gc.getCivilizationInfo(iTheirCiv).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIV, iTheirCiv, -1, CvUtil.FONT_CENTER_JUSTIFY)
		# <!-- custom: Battle rows already store the opponent's exact civilization; pass it to leader-hover trait help instead of literal civilization ID 1 (Arabia). See KI#332. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, 18, iRow, getAdvisorIconSortKey(iTheirLeader + 1, iRow), gc.getLeaderHeadInfo(iTheirLeader).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_LEADER, iTheirLeader, iTheirCiv, CvUtil.FONT_CENTER_JUSTIFY)
		SASTextScale.setTableIntLabel(screen, self.BATTLE_TABLE_ID, 19, iRow, str(iTheirPlayer), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		SASTextScale.setTableIntLabel(screen, self.BATTLE_TABLE_ID, 20, iRow, self.getBattleCaptureCountText(iCapturedCount), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, 21, iRow, getAdvisorIconSortKey(iCapturedUnit + 1, iRow), self.getBattleCaptureUnitButton(iCapturedCount, iCapturedUnit), eCapturedUnitWidget, iCapturedUnit, -1, CvUtil.FONT_CENTER_JUSTIFY)
		if self.IS_SAS_CV_MILITARY_ADVISOR_BATTLE_PLOT_CONTEXT_ENABLE:
			self.setBattleHillPeakCell(screen, iRow, iHillPeakTerrain)
			self.setBattleTerrainFeatureCell(screen, iRow, self.BATTLE_TERRAIN_COL_ID, iTerrain, True)
			self.setBattleTerrainFeatureCell(screen, iRow, self.BATTLE_FEATURE_COL_ID, iFeature, False)
		SASTextScale.setTableTextLabel(screen, self.BATTLE_TABLE_ID, self.BATTLE_PLOT_COL_ID, iRow, getAdvisorIconSortKey(iCityContext + 1, iRow), self.getBattlePlotButton(iCityContext, iX, iY), WidgetTypes.WIDGET_GENERAL, iX, iY, CvUtil.FONT_CENTER_JUSTIFY)

	def dbgLogBattleHistory(self):
		aEntries = SASBattleHistory.getEntriesForPlayer(self.iActivePlayer)
		if not aEntries:
			print("SAS_MILITARY_ADVISOR_BATTLE_HISTORY_EMPTY")
			return
		print("SAS_MILITARY_ADVISOR_BATTLE_HISTORY_BEGIN")
		self.dbgLogBattleHistoryPerspectivePlayer()
		self.dbgLogBattleHistoryPlayers(aEntries)
		# <!-- custom: Keep observer-permitted stored hill/peak, terrain, and feature context in copied logs even when the optional UI columns are disabled.
		# getBattlePerspectiveColumns has already sanitized location/context that the real active player has not revealed under a vassal perspective. See KI#335. (GPT-5.5 + ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		print("Turn | Year | Result | Est% | Role | OurStrB | OurStrS | OurStrE | OurStrM | OurXP | OurGG | OurUnit | TheirUnit | TheirXP | TheirGG | TheirStrB | TheirStrS | TheirStrE | TheirStrM | TheirPID | Cap# | CapUnit | HillPeak | Terrain | Feature | X | Y")
		for entry in aEntries:
			tColumns = self.getBattlePerspectiveColumns(entry)
			if tColumns is None:
				continue
			iTurn, iResult, _, iOurRole, iOurUnit, iOurCurrStr, iOurEndStr, iOurMaxStr, iOurXP, bOurGG, iTheirPlayer, iTheirUnit, iTheirCurrStr, iTheirEndStr, iTheirMaxStr, iTheirXP, bTheirGG, iX, iY, iCapturedCount, iCapturedUnit, iTerrain, iFeature, iHillPeakTerrain, iCityContext = tColumns
			print("%d | %s | %s | %s | %s | %s | %s | %s | %s | %s | %d | %s | %s | %s | %d | %s | %s | %s | %s | %d | %d | %s | %s | %s | %s | %d | %d" % (iTurn, self.getTurnDate(iTurn), self.getBattleResultLogText(iResult), self.getBattleEstimatedOddsText(iOurCurrStr, iTheirCurrStr), self.getBattleRoleText(iOurRole), self.getBattleBaseStrengthText(iOurUnit), self.getBattleStoredStrengthText(iOurCurrStr), self.getBattleStoredStrengthText(iOurEndStr), self.getBattleStoredStrengthText(iOurMaxStr), self.getBattleXPText(iOurXP, False), bOurGG, gc.getUnitInfo(iOurUnit).getDescription(), gc.getUnitInfo(iTheirUnit).getDescription(), self.getBattleXPText(iTheirXP, False), bTheirGG, self.getBattleBaseStrengthText(iTheirUnit), self.getBattleStoredStrengthText(iTheirCurrStr), self.getBattleStoredStrengthText(iTheirEndStr), self.getBattleStoredStrengthText(iTheirMaxStr), iTheirPlayer, iCapturedCount, self.getBattleCaptureUnitText(iCapturedCount, iCapturedUnit), self.getBattleTerrainTypeText(iHillPeakTerrain), self.getBattleTerrainTypeText(iTerrain), self.getBattleFeatureTypeText(iFeature), iX, iY))
		print("SAS_MILITARY_ADVISOR_BATTLE_HISTORY_END")

	def dbgLogBattleHistoryPerspectivePlayer(self):
		kPlayer = gc.getPlayer(self.iActivePlayer)
		szLeaderType = gc.getLeaderHeadInfo(kPlayer.getLeaderType()).getType()
		szCivType = gc.getCivilizationInfo(kPlayer.getCivilizationType()).getType()
		# <!-- custom: Battles alone has subject-versus-observer map sanitization, so record the two inputs controlling it locally instead of imposing niche context fields on every advisor LOG button. See KI#335. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		print("BattleHistoryContext: DebugMode=%d | ObserverPID=%d" % (CyGame().isDebugMode(), CyGame().getActivePlayer()))
		print("PerspectivePlayer: %d | %s | %s | %s" % (self.iActivePlayer, kPlayer.getName(), szLeaderType, szCivType))

	def dbgLogBattleHistoryPlayers(self, aEntries):
		# <!-- custom: Battles rows are relative to the inspected player, so UI omits our repeated civ/leader/PID; the log prints perspective once and keeps a compact opponent PID legend for copied logs with duplicate leaders/civs. (GPT-5.5) -->
		aiPlayers = []
		for entry in aEntries:
			tColumns = self.getBattlePerspectiveColumns(entry)
			if tColumns is None:
				continue
			# <!-- custom: Opponent PID is column 10 in getBattlePerspectiveColumns; using column 8 (our XP) made the LOG button call gc.getPlayer(XP), causing a None player and Python error on high-XP rows. (GPT-5.5) -->
			iTheirPlayer = tColumns[10]
			if iTheirPlayer not in aiPlayers:
				aiPlayers.append(iTheirPlayer)
		aiPlayers.sort()
		print("OpponentPlayers: PID | PlayerName | LeaderType | CivType")
		for iPlayer in aiPlayers:
			kPlayer = gc.getPlayer(iPlayer)
			szLeaderType = gc.getLeaderHeadInfo(kPlayer.getLeaderType()).getType()
			szCivType = gc.getCivilizationInfo(kPlayer.getCivilizationType()).getType()
			print("OpponentPlayer: %d | %s | %s | %s" % (iPlayer, kPlayer.getName(), szLeaderType, szCivType))

	# <!-- custom: Composition tab is a CURRENT snapshot, distinct from the Score-tab Stats panel (lifetime CyStatistics totals).
	# Unlike the Map tab, it does not classify units by combat class for grouping; it is purely numerical counts, so the two tabs are complementary.
	# Name picked over "Forces" or "Current" because "composition" by definition refers to the present make-up (it cannot mean a past composition without becoming a different one), so the tab is self-evidently current.
	# canFight() filters by baseCombatStr > 0: civilians are excluded since they cannot fight and the Map tab already lists them without a combat group; animals also have no UnitCombat class but they can fight, so they belong here, appearing in the Units col and naturally dropping from the Combats col.
	# CyUnit.isCombat() looks tempting but actually wraps isInCombat(); empirically the table came out empty with isCombat() and populated correctly after switching to canFight(). (Claude code Opus 4.7) -->
	def collectCompositionData(self):
		dUnits = {}
		dPromotions = {}
		dCombats = {}
		pPlayer = gc.getPlayer(self.iActivePlayer)
		(pUnit, iter) = pPlayer.firstUnit(False)
		iNumPromotionInfos = gc.getNumPromotionInfos()
		while pUnit and not pUnit.isNone():
			if pUnit.canFight():
				iUnitType = pUnit.getUnitType()
				dUnits[iUnitType] = dUnits.get(iUnitType, 0) + 1
				iCombatType = pUnit.getUnitCombatType()
				if iCombatType >= 0:
					dCombats[iCombatType] = dCombats.get(iCombatType, 0) + 1
				for iPromo in range(iNumPromotionInfos):
					if pUnit.isHasPromotion(iPromo):
						dPromotions[iPromo] = dPromotions.get(iPromo, 0) + 1
			(pUnit, iter) = pPlayer.nextUnit(iter, False)
		return dUnits, dPromotions, dCombats

	def drawCompositionCountTable(self, szTable, szTitle, dCounts, getInfoFn, ePediaWidget, iX, iY, iW, iH):
		screen = self.getScreen()
		# <!-- custom: build (description, button, infoIdx, count) rows then sort by count desc so the heaviest entries land at the top before the user touches a column header. (Claude code Opus 4.7) -->
		aRows = []
		for iIdx, iCount in dCounts.items():
			info = getInfoFn(iIdx)
			aRows.append((info.getDescription(), info.getButton(), iIdx, iCount))
		aRows.sort(key=lambda r: (-r[3], r[0]))
		screen.addTableControlGFC(szTable, 2, iX, iY, iW, iH, True, True, 24, 24, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSort(szTable)
		screen.setStyle(szTable, "Table_StandardCiv_Style")
		iCountColW = 80
		iNameColW = iW - iCountColW - 14
		screen.setTableColumnHeader(szTable, 0, sasFontTagLabel + szTitle + SAS_FONT_TAG_CLOSE, iNameColW)
		screen.setTableColumnHeader(szTable, 1, sasFontTagLabel + self.TEXT_COMPOSITION_COUNT + SAS_FONT_TAG_CLOSE, iCountColW)
		for iRow, (szName, szButton, iIdx, iCount) in enumerate(aRows):
			screen.appendTableRow(szTable)
			screen.setTableText(szTable, 0, iRow, sasFontTagLabel + szName + SAS_FONT_TAG_CLOSE, szButton, ePediaWidget, iIdx, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableInt(szTable, 1, iRow, sasFontTagLabel + str(iCount) + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def collectSummaryData(self):
		# <!-- custom: single unit-iteration pass collects everything the Summary tab needs (domain split, military/civilian split, deployment, wounded count, upgrade gold).
		# canFight() is the established military filter for this advisor (see collectCompositionData).
		# Upgrade options are walked per unit class because per-unit upgrade availability depends on the player's tech, not the unit info alone. (Claude code Opus 4.7) -->
		pPlayer = gc.getPlayer(self.iActivePlayer)
		iCiv = pPlayer.getCivilizationType()
		pCivInfo = gc.getCivilizationInfo(iCiv)
		iNumUnitClasses = gc.getNumUnitClassInfos()
		dUpgradeTargets = {}

		def getUpgradeTargets(iUnitType):
			if iUnitType in dUpgradeTargets:
				return dUpgradeTargets[iUnitType]
			pUnitInfo = gc.getUnitInfo(iUnitType)
			aTargets = []
			for iClass in range(iNumUnitClasses):
				if pUnitInfo.getUpgradeUnitClass(iClass):
					iToUnit = pCivInfo.getCivilizationUnits(iClass)
					if iToUnit >= 0 and pPlayer.canTrain(iToUnit, False, False):
						aTargets.append(iToUnit)
			dUpgradeTargets[iUnitType] = aTargets
			return aTargets

		iTotal = 0
		iMilitary = 0
		iCivilian = 0
		iLand = 0
		iSea = 0
		iAir = 0
		iWounded = 0
		iInCities = 0
		iInOwnTerritory = 0
		iInAllied = 0
		iInNeutral = 0
		iInEnemy = 0
		iInWild = 0
		iAtSea = 0
		iUpgradeable = 0
		iNoMilSupportUnits = 0
		# <!-- custom: army-quality counters use base XML strength/cost plus current unit XP/level; combat-only averages exclude workers/settlers so civilian units do not dilute army stats. (Claude code Opus 4.7, GPT-5.5) -->
		iStrongestStr = -1
		iStrongestUnitType = -1
		iHighestCost = -1
		iHighestCostUnitType = -1
		iTotalHammers = 0
		iXpSum = 0
		iLevelSum = 0
		# <!-- custom: separate unspent Great General units from combat units already carrying PROMOTION_LEADER; they answer "do I have a GG to attach?"
		# vs "how many units are already GG-led?". (Claude code Opus 4.7, GPT-5.5) -->
		iStandaloneGenerals = 0
		iLedByGeneral = 0
		# <!-- custom: army health stats. iMaxHealth tracks the highest hp (typically 100 for any uninjured unit), iMinHealth the most-wounded. iHealthSum / count gives the average. Health = 100 - (damage / max-hp * 100).
		# CvUnit.currHitPoints() and maxHitPoints() return the engine's numbers, but for percentages we use (1 - getDamage()/100) since Civ4 stores damage as a 0..100 integer. (Claude code Opus 4.7) -->
		iMaxHealth = -1
		iMinHealth = -1
		iHealthSum = 0
		# <!-- custom: health-band buckets mirror the Battles tab's color tiers (=100 full, >66 high, >33 medium, else low) so distribution rows under Wounded read with the same mental model as combat-row tinting.
		# Edge case: 0-hp units can't really exist in normal play (they'd die first) but if one shows up it counts in Low. (Claude code Opus 4.7) -->
		iHealthFull = 0
		iHealthHigh = 0
		iHealthMedium = 0
		iHealthLow = 0
		# <!-- custom: covert-unit categories: invisible (subs and similar - UnitInfo.getInvisibleType() != -1 declares an invisibility class), spies (isSpy flag), hidden-nationality (privateers and similar - pirate flag).
		# A single unit can be more than one of these (e.g. a stealth spy in some mods); we count independently rather than partitioning.
		# Named "covert" rather than "special" because Civ4 already uses "special" as a distinct concept (special unit classes, special buildings), so reusing that word would suggest a different categorization. (Claude code Opus 4.7) -->
		iInvisibleUnits = 0
		iSpyUnits = 0
		iHiddenNatUnits = 0
		# <!-- custom: dPromotionCounts -> {promotionID: occurrences across military units}, used to surface the army's top promotions.
		# Counted only on canFight() units; civilians can technically carry promotions in some mods but they distort the "what's my army built around?"
		# signal. (Claude code Opus 4.7) -->
		dPromotionCounts = {}
		iNumPromotionInfos = gc.getNumPromotionInfos()
		# <!-- custom: build the upgrade-cost list rather than just the running total so we can compute min/avg/max afterwards. (Claude code Opus 4.7) -->
		aUpgradeCosts = []
		# <!-- custom: do NOT track an "average strength gained per upgrade" stat.
		# CvUnitInfo::getCombat() returns base XML strength only and excludes combat modifiers (collateral, city attack, withdraw, first strikes).
		# E.g. Catapult vs Trebuchet are concurrent siege units (not an upgrade chain in AdvCiv-SAS), with Trebuchet having lower base strength but stronger city-attack/collateral modifiers; either picked over the other for a real strategic reason that a base-strength delta would read as flat or negative.
		# A delta number without those modifiers would mislead more than inform. (Claude code Opus 4.7) -->

		# <!-- custom: cache team relations once instead of re-fetching per unit; team lookups in Civ4 Python are cheap but iterate fast enough to be worth hoisting on large empires.
		# The "ally" predicate is intentionally broad: same team OR mutual vassalage (us->them or them->us).
		# Defensive pacts and open borders are deliberately NOT counted as ally here because they don't grant the same trust level for stationing armies. (Claude code Opus 4.7) -->
		eMyTeam = pPlayer.getTeam()
		kMyTeam = gc.getTeam(eMyTeam)
		(pUnit, iter) = pPlayer.firstUnit(False)
		while pUnit and not pUnit.isNone():
			iTotal += 1
			iUnitType = pUnit.getUnitType()
			pUnitInfo = gc.getUnitInfo(iUnitType)
			# <!-- custom: count units flagged bMilitarySupport=0 in CIV4UnitInfos.xml (e.g. Robotic Infantry, "No military support cost" in Sevopedia). CvUnit::changeMilitarySupportUnits skips changeNumMilitaryUnits() for these, so they pay regular unit cost but bypass the military-cost portion of CvPlayer::calculateUnitCost. (Claude code Opus 4.7) -->
			if not pUnitInfo.isMilitarySupport():
				iNoMilSupportUnits += 1
			# <!-- custom: covert-unit predicates checked on every owned unit (not just canFight) so spies count.
			# Independent flags can co-occur. (Claude code Opus 4.7) -->
			if pUnitInfo.getInvisibleType() != -1:
				iInvisibleUnits += 1
			if pUnitInfo.isSpy():
				iSpyUnits += 1
			if pUnitInfo.isHiddenNationality():
				iHiddenNatUnits += 1
			bMilitary = pUnit.canFight()
			if bMilitary:
				iMilitary += 1
				iDomain = pUnit.getDomainType()
				if iDomain == DomainTypes.DOMAIN_LAND:
					iLand += 1
				elif iDomain == DomainTypes.DOMAIN_SEA:
					iSea += 1
				elif iDomain == DomainTypes.DOMAIN_AIR:
					iAir += 1
				if pUnit.getDamage() > 0:
					iWounded += 1
				# <!-- custom: getCombat() is land/sea base strength, getAirCombat() is air; take the larger of the two so an Air-domain unit isn't compared on its 0 land combat. (Claude code Opus 4.7) -->
				iBaseStr = pUnitInfo.getCombat()
				iAirStr = pUnitInfo.getAirCombat()
				if iAirStr > iBaseStr:
					iBaseStr = iAirStr
				if iBaseStr > iStrongestStr:
					iStrongestStr = iBaseStr
					iStrongestUnitType = iUnitType
				iCost = pUnitInfo.getProductionCost()
				if iCost > 0:
					iTotalHammers += iCost
					if iCost > iHighestCost:
						iHighestCost = iCost
						iHighestCostUnitType = iUnitType
				iXpSum += pUnit.getExperience()
				iLevelSum += pUnit.getLevel()
				# <!-- custom: getLeaderPromotion() >= 0 means this UnitInfo is a Great General template (it carries a leader promotion to grant on attach), so the unit ITSELF is a standalone GG.
				# Otherwise check whether the unit has previously received PROMOTION_LEADER (i.e. a regular combat unit that absorbed a GG).
				# The two cases are exclusive. (Claude code Opus 4.7) -->
				if pUnitInfo.getLeaderPromotion() >= 0:
					iStandaloneGenerals += 1
				elif pUnit.isHasPromotion(self.iPromoLeader):
					iLedByGeneral += 1
				# <!-- custom: hit-points-based health % (uses currHitPoints/maxHitPoints rather than 100-getDamage so promotion/building max-HP overrides are reflected). iMinHealth=-1 sentinel because 0 is a valid (dying) value. (Claude code Opus 4.7) -->
				iMaxHp = pUnit.maxHitPoints()
				if iMaxHp > 0:
					iHealthPct = (100 * pUnit.currHitPoints()) / iMaxHp
					iHealthSum += iHealthPct
					if iHealthPct > iMaxHealth:
						iMaxHealth = iHealthPct
					if iMinHealth < 0 or iHealthPct < iMinHealth:
						iMinHealth = iHealthPct
					# <!-- custom: band thresholds match getSummaryHealthColor (= 100 / >66 / >33 / else); keep them aligned so a Medium band always means "yellow-tinted health" elsewhere. (Claude code Opus 4.7) -->
					if iHealthPct >= 100:
						iHealthFull += 1
					elif iHealthPct > 66:
						iHealthHigh += 1
					elif iHealthPct > 33:
						iHealthMedium += 1
					else:
						iHealthLow += 1
				# <!-- custom: count each carried promotion.
				# Loop bound is iNumPromotionInfos rather than a fixed list; mod-mods can add promotions and we want them all to compete for the top-promotion slots. (Claude code Opus 4.7, GPT-5.5) -->
				for iPromo in range(iNumPromotionInfos):
					if pUnit.isHasPromotion(iPromo):
						dPromotionCounts[iPromo] = dPromotionCounts.get(iPromo, 0) + 1
			else:
				iCivilian += 1

			# <!-- custom: refined deployment buckets. Own/Allied/Enemy/Neutral civ territory + unowned Wild (land) + Open Water (sea). Allied = our team or mutual vassalage; Enemy = our team at war with the plot owner's team.
			# Neutral = any other met civ. Wild = no owner AND not water; At Sea = no owner AND water. Wounded is a cross-cutting flag (already counted above). (Claude code Opus 4.7) -->
			pPlot = pUnit.plot()
			if pPlot and not pPlot.isNone():
				iPlotOwner = pPlot.getOwner()
				if iPlotOwner == self.iActivePlayer:
					if pPlot.isCity():
						iInCities += 1
					else:
						iInOwnTerritory += 1
				elif iPlotOwner == -1:
					if pPlot.isWater():
						iAtSea += 1
					else:
						iInWild += 1
				else:
					eOwnerTeam = gc.getPlayer(iPlotOwner).getTeam()
					if eOwnerTeam == eMyTeam or kMyTeam.isVassal(eOwnerTeam) or gc.getTeam(eOwnerTeam).isVassal(eMyTeam):
						iInAllied += 1
					elif kMyTeam.isAtWar(eOwnerTeam):
						iInEnemy += 1
					else:
						iInNeutral += 1

			aTargets = getUpgradeTargets(iUnitType)
			if aTargets:
				iUpgradeable += 1
				iCheapest = -1
				for iToUnit in aTargets:
					iPrice = pUnit.upgradePrice(iToUnit)
					# <!-- custom: Zero is a valid upgrade price for a Great-General-led unit with a 100% discount.
					# Keep -1 only as the no-candidate sentinel so free upgrades contribute to min/average statistics. See KI#220. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
					if iPrice >= 0 and (iCheapest < 0 or iPrice < iCheapest):
						iCheapest = iPrice
				if iCheapest >= 0:
					aUpgradeCosts.append(iCheapest)

			(pUnit, iter) = pPlayer.nextUnit(iter, False)

		# <!-- custom: derive upgrade-cost aggregates after the loop so we can show min/avg/max alongside the existing total.
		# Avg is integer-rounded since the engine only deals in whole gold. (Claude code Opus 4.7) -->
		iUpgradeMin = 0
		iUpgradeAvg = 0
		iUpgradeMax = 0
		iUpgradeTotal = 0
		if aUpgradeCosts:
			iUpgradeMin = min(aUpgradeCosts)
			iUpgradeMax = max(aUpgradeCosts)
			iUpgradeTotal = sum(aUpgradeCosts)
			iUpgradeAvg = iUpgradeTotal / len(aUpgradeCosts)

		# <!-- custom: army averages need the military unit count (canFight()) as denominator, not getNumMilitaryUnits() - the latter is the surcharge-paying subset and would skew the average.
		# Default to 0.0 to avoid divide-by-zero on civ-only perspectives or pre-game-start views.
		# Floats (1 decimal place) because integer truncation hid useful gradations: an army averaging "2.7" XP reads very differently from "2.0", and the display row format string uses %.1f to surface that. (Claude code Opus 4.7) -->
		fAvgXp = 0.0
		fAvgLevel = 0.0
		fAvgHammers = 0.0
		fAvgHealth = 0.0
		if iMilitary > 0:
			fAvgXp = float(iXpSum) / iMilitary
			fAvgLevel = float(iLevelSum) / iMilitary
			fAvgHammers = float(iTotalHammers) / iMilitary
			fAvgHealth = float(iHealthSum) / iMilitary
		# <!-- custom: max/min health default to 100/100 when there are no military units; means "no problem to report" reads cleaner than the -1 sentinels. (Claude code Opus 4.7) -->
		if iMaxHealth < 0:
			iMaxHealth = 100
		if iMinHealth < 0:
			iMinHealth = 100

		szStrongestName = u""
		szStrongestButton = u""
		if iStrongestUnitType >= 0:
			pStrongestInfo = gc.getUnitInfo(iStrongestUnitType)
			szStrongestName = pStrongestInfo.getDescription()
			szStrongestButton = pStrongestInfo.getButton()
		szHighestCostName = u""
		szHighestCostButton = u""
		if iHighestCostUnitType >= 0:
			pHighestCostInfo = gc.getUnitInfo(iHighestCostUnitType)
			szHighestCostName = pHighestCostInfo.getDescription()
			szHighestCostButton = pHighestCostInfo.getButton()

		# <!-- custom: one city pass collects building, military-production, generic new-XP, and building-defense stats for the Summary tab.
		# Military infrastructure/fortification identities come from getSummaryMilitaryBuildingGroups instead of named XML classes. (Claude code Opus 4.7 + GPT-5.5 + ChatGPT-5.6-Sol) -->
		aTrainingBuildings, aNationalMilitaryBuildings, aDefenseBuildings = self.getSummaryMilitaryBuildingGroups(iCiv)
		dBuildingCounts = {}
		for iBuilding in aTrainingBuildings + aDefenseBuildings:
			dBuildingCounts[iBuilding] = 0
		dNationalBuildingCities = {}
		for iBuilding in aNationalMilitaryBuildings:
			dNationalBuildingCities[iBuilding] = u""
		iNumCities = 0
		iNumCoastalCities = 0
		iProdModSum = 0
		iBestProdMod = -1000000
		szBestProdCity = u""
		iXpSumCity = 0
		iBestXp = -1
		szBestXpCity = u""
		iBestDefense = -1
		szBestDefenseCity = u""
		iDefenseSum = 0
		(pCity, iCityIter) = pPlayer.firstCity(False)
		while pCity and not pCity.isNone():
			szCityName = pCity.getName()
			iNumCities += 1
			# <!-- custom: coastal check uses minWaterSize=10 (the standard "ocean tile" threshold the engine uses for naval-build prerequisites); lakes don't count. Drydock denominator. (GPT-5.5) -->
			if pCity.isCoastal(10):
				iNumCoastalCities += 1
			for iBuilding in dBuildingCounts:
				if pCity.getNumBuilding(iBuilding) > 0:
					dBuildingCounts[iBuilding] += 1
			# <!-- custom: selected national military buildings are player-limited; retain the owning city for the Summary row, with an empty string meaning not built yet. (ChatGPT-5.6-Sol) -->
			for iBuilding in aNationalMilitaryBuildings:
				if pCity.getNumBuilding(iBuilding) > 0:
					dNationalBuildingCities[iBuilding] = szCityName
			# <!-- custom: getMilitaryProductionModifier is the aggregate %-bonus this city gives to military-unit production (Barracks, Heroic Epic, West Point, Pentagon, Theocracy, etc. all roll into it).
			# Sum + best-city tracking gives the "where do I train?"
			# answer at a glance.
			# Individual iMilitaryProductionModifier sources above 0 (including civics) are not enumerated because the Summary tab is already full; the aggregate keeps this concise, though less ideal than a full source breakdown. (GPT-5.5) -->
			iProdMod = pCity.getMilitaryProductionModifier()
			iProdModSum += iProdMod
			if iProdMod > iBestProdMod:
				iBestProdMod = iProdMod
				szBestProdCity = szCityName
			# <!-- custom: use generic city free XP here, not getProductionExperience(unit), so "New XP (All)" stays unit-agnostic.
			# Building-specific domain/unit-combat XP is shown on the infrastructure rows instead; folding it into this headline made the average hard to interpret. (GPT-5.5 + ChatGPT-5.6-Sol) -->
			iXp = pCity.getFreeExperience()
			iXpSumCity += iXp
			if iXp > iBestXp:
				iBestXp = iXp
				szBestXpCity = szCityName
			# <!-- custom: getBuildingDefense is the city's defense-modifier contribution from buildings alone (excludes plot/cultural defense).
			# Direct comparator for "best fortified" since plot defense varies by terrain. (GPT-5.5) -->
			iDef = pCity.getBuildingDefense()
			iDefenseSum += iDef
			if iDef > iBestDefense:
				iBestDefense = iDef
				szBestDefenseCity = szCityName
			(pCity, iCityIter) = pPlayer.nextCity(iCityIter, False)

		# <!-- custom: allied hammers sum combat-unit production cost for teammates and mutual vassals only; defensive pacts/open borders do not make the army effectively ours.
		# Keep this separate from the active player's own army loop. (Claude code Opus 4.7, GPT-5.5) -->
		iAlliedHammers = 0
		for iLoopPlayer in range(gc.getMAX_PLAYERS()):
			if iLoopPlayer == self.iActivePlayer:
				continue
			pLoopPlayer = gc.getPlayer(iLoopPlayer)
			if not pLoopPlayer.isAlive() or pLoopPlayer.isBarbarian() or pLoopPlayer.isMinorCiv():
				continue
			eLoopTeam = pLoopPlayer.getTeam()
			if eLoopTeam == eMyTeam or kMyTeam.isVassal(eLoopTeam) or gc.getTeam(eLoopTeam).isVassal(eMyTeam):
				(pAllyUnit, iAllyIter) = pLoopPlayer.firstUnit(False)
				while pAllyUnit and not pAllyUnit.isNone():
					if pAllyUnit.canFight():
						iAllyCost = gc.getUnitInfo(pAllyUnit.getUnitType()).getProductionCost()
						if iAllyCost > 0:
							iAlliedHammers += iAllyCost
					(pAllyUnit, iAllyIter) = pLoopPlayer.nextUnit(iAllyIter, False)
		iKnownPowerRank = 1
		iKnownPowerPlayers = 0
		iOurPower = pPlayer.getPower()
		bDebugMode = gc.getGame().isDebugMode()
		for iLoopPlayer in range(gc.getMAX_PLAYERS()):
			pLoopPlayer = gc.getPlayer(iLoopPlayer)
			if not pLoopPlayer.isAlive() or pLoopPlayer.isBarbarian() or pLoopPlayer.isMinorCiv():
				continue
			if iLoopPlayer != self.iActivePlayer and not bDebugMode and not pPlayer.canSeeDemographics(iLoopPlayer):
				continue
			iKnownPowerPlayers += 1
			if pLoopPlayer.getPower() > iOurPower:
				iKnownPowerRank += 1
		# <!-- custom: average modifier rounds to int; preserving sign via "+%d%%" / "%d%%" so positive bonuses are obvious.
		# avg XP keeps a decimal. (GPT-5.5) -->
		iAvgProdMod = 0
		fAvgFreeXp = 0.0
		iAvgDefense = 0
		if iNumCities > 0:
			iAvgProdMod = iProdModSum / iNumCities
			fAvgFreeXp = float(iXpSumCity) / iNumCities
			# <!-- custom: integer-rounded average of getBuildingDefense across own cities; pairs with Best Defended in Deployment so the user sees "where's my strongest fortress + how protected is the empire on average" together. (Claude code Opus 4.7) -->
			iAvgDefense = iDefenseSum / iNumCities

		# <!-- custom: top 5 promotions across the army, descending by occurrence and tiebroken by description; full distribution remains in the Composition tab. (Claude code Opus 4.7, GPT-5.5) -->
		aFavoritePromoRows = []
		aPromoList = []
		for iPromo, iCount in dPromotionCounts.items():
			pPromoInfo = gc.getPromotionInfo(iPromo)
			aPromoList.append((iCount, pPromoInfo.getDescription(), pPromoInfo.getButton()))
		aPromoList.sort(key=lambda r: (-r[0], r[1]))
		for iCount, szName, szButton in aPromoList[:5]:
			aFavoritePromoRows.append((szName, iCount, szButton))

		iInflationFactor = 100 + pPlayer.calculateInflationRate()
		# <!-- custom: Use exact DLL support-cost breakdown tuples exposed for this Summary tab.
		# Reconstructing the math in Python hid K-Mod/AdvCiv multipliers and produced misleading rows like "1 x 100% = 0"; these tuples let the UI show the same intermediate values as CvPlayer::calculateUnitCost/calculateUnitSupply. (GPT-5.5) -->
		iFreeUnits, iFreeMilitaryUnits, iPaidUnits, iPaidMilitaryUnits, iUnitCostMultiplier, iCalculatedRegularUnitCost, iCalculatedMilitaryUnitCost, iCalculatedExtraUnitCost, iCalculatedRawUnitCost = pPlayer.calculateUnitCostBreakdown()
		iPaidOutside, iCalculatedBaseSupplyCost, iCalculatedRawUnitSupply = pPlayer.calculateUnitSupplyBreakdown()
		bAnarchy = pPlayer.isAnarchy()
		# <!-- custom: The breakdown bindings expose internal formula subtotals, which intentionally ignore the public CvPlayer wrappers' anarchy exemption.
		# Preserve their useful free/paid counters, but show every charged support component as zero while anarchy waives Unit Cost and Unit Supply. See KI#219. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if bAnarchy:
			iRegularUnitCost = 0
			iMilitaryUnitCost = 0
			iExtraUnitCost = 0
			iRawUnitCost = 0
			iBaseSupplyCost = 0
			iRawUnitSupply = 0
		else:
			iRegularUnitCost = iCalculatedRegularUnitCost
			iMilitaryUnitCost = iCalculatedMilitaryUnitCost
			iExtraUnitCost = iCalculatedExtraUnitCost
			iRawUnitCost = iCalculatedRawUnitCost
			iBaseSupplyCost = iCalculatedBaseSupplyCost
			iRawUnitSupply = iCalculatedRawUnitSupply
		iUnitCost = (iRawUnitCost * iInflationFactor + 50) / 100
		iUnitSupply = (iRawUnitSupply * iInflationFactor + 50) / 100
		iTotalGold = iUnitCost + iUnitSupply
		iGoldPerUnit = pPlayer.getGoldPerUnit()
		iGoldPerMilitaryUnit = pPlayer.getGoldPerMilitaryUnit()
		iGoldPerOutsideUnit = self.iINITIAL_OUTSIDE_UNIT_GOLD_PERCENT
		iNumMilitaryUnits = pPlayer.getNumMilitaryUnits()
		iOutsideUnits = pPlayer.getNumOutsideUnits()
		iFreeOutside = iOutsideUnits - iPaidOutside

		# <!-- custom: build compact generic row descriptors after the shared city pass.
		# Ordinary water buildings use the same coastal-city denominator the old Drydock row used; national rows carry their owning city.
		# Building descriptions/icons come from the active civilization's actual replacement, not the base class identity. (ChatGPT-5.6-Sol) -->
		aMilitaryBuildingRows = []
		for iBuilding in aTrainingBuildings:
			kInfo = gc.getBuildingInfo(iBuilding)
			iDenominator = iNumCities
			if kInfo.isWater():
				iDenominator = iNumCoastalCities
			aMilitaryBuildingRows.append((self.getSummaryBuildingXpLabel(kInfo.getDescription(), iBuilding), dBuildingCounts[iBuilding], iDenominator, self.summaryBuildingHasTech(kMyTeam, iBuilding), kInfo.getButton(), False, u""))
		for iBuilding in aNationalMilitaryBuildings:
			kInfo = gc.getBuildingInfo(iBuilding)
			aMilitaryBuildingRows.append((kInfo.getDescription(), 0, 1, self.summaryBuildingHasTech(kMyTeam, iBuilding), kInfo.getButton(), True, dNationalBuildingCities[iBuilding]))
		aDefenseBuildingRows = []
		for iBuilding in aDefenseBuildings:
			kInfo = gc.getBuildingInfo(iBuilding)
			aDefenseBuildingRows.append((kInfo.getDescription(), dBuildingCounts[iBuilding], iNumCities, kInfo.getButton()))

		return {
			"total": iTotal,
			"military": iMilitary,
			"civilian": iCivilian,
			"land": iLand,
			"sea": iSea,
			"air": iAir,
			"wounded": iWounded,
			"in_cities": iInCities,
			"in_own_territory": iInOwnTerritory,
			"in_allied": iInAllied,
			"in_neutral": iInNeutral,
			"in_enemy": iInEnemy,
			"in_wild": iInWild,
			"at_sea": iAtSea,
			"strongest_str": iStrongestStr,
			"strongest_name": szStrongestName,
			"highest_cost": iHighestCost,
			"highest_cost_name": szHighestCostName,
			"total_hammers": iTotalHammers,
			"avg_xp": fAvgXp,
			"avg_level": fAvgLevel,
			"avg_hammers": fAvgHammers,
			"known_power_rank": iKnownPowerRank,
			"known_power_players": iKnownPowerPlayers,
			"standalone_generals": iStandaloneGenerals,
			"led_by_general": iLedByGeneral,
			"strongest_button": szStrongestButton,
			"highest_cost_button": szHighestCostButton,
			"max_health": iMaxHealth,
			"min_health": iMinHealth,
			"avg_health": fAvgHealth,
			"military_building_rows": aMilitaryBuildingRows,
			"defense_building_rows": aDefenseBuildingRows,
			"avg_prod_mod": iAvgProdMod,
			"best_prod_mod": iBestProdMod,
			"best_prod_city": szBestProdCity,
			"avg_free_xp": fAvgFreeXp,
			"best_free_xp": max(0, iBestXp),
			"best_xp_city": szBestXpCity,
			"best_defense": max(0, iBestDefense),
			"best_defense_city": szBestDefenseCity,
			"avg_defense": iAvgDefense,
			"allied_hammers": iAlliedHammers,
			"favorite_promotions": aFavoritePromoRows,
			"health_full": iHealthFull,
			"health_high": iHealthHigh,
			"health_medium": iHealthMedium,
			"health_low": iHealthLow,
			"invisible_units": iInvisibleUnits,
			"spy_units": iSpyUnits,
			"hidden_nat_units": iHiddenNatUnits,
			"upgradeable": iUpgradeable,
			"upgrade_min": iUpgradeMin,
			"upgrade_avg": iUpgradeAvg,
			"upgrade_max": iUpgradeMax,
			"upgrade_total": iUpgradeTotal,
			"raw_unit_cost": iRawUnitCost,
			"raw_unit_supply": iRawUnitSupply,
			"regular_unit_cost": iRegularUnitCost,
			"military_unit_cost": iMilitaryUnitCost,
			"unit_cost": iUnitCost,
			"unit_supply": iUnitSupply,
			"total_gold": iTotalGold,
			"base_supply_cost": iBaseSupplyCost,
			"free_units": iFreeUnits,
			"free_used_units": iTotal - iPaidUnits,
			"paid_units": iPaidUnits,
			"num_military": iNumMilitaryUnits,
			"free_military_units": iFreeMilitaryUnits,
			"free_used_military_units": iNumMilitaryUnits - iPaidMilitaryUnits,
			"paid_military_units": iPaidMilitaryUnits,
			"outside_units": iOutsideUnits,
			"free_outside": iFreeOutside,
			"paid_outside": iPaidOutside,
			"no_mil_support": iNoMilSupportUnits,
			"gold_per_unit": iGoldPerUnit,
			"unit_cost_multiplier": iUnitCostMultiplier,
			"gold_per_military_unit": iGoldPerMilitaryUnit,
			"extra_unit_cost": iExtraUnitCost,
			"gold_per_outside_unit": iGoldPerOutsideUnit,
			"inflation_percent": iInflationFactor - 100,
			"anarchy": bAnarchy,
		}

	def drawSummaryColumn(self, screen, iColX, iColY, iColW, iColH, szTitle, aRows):
		# <!-- custom: rows are (szLabel, szValue, eHelpWidget, iData1, iData2, iIndent, iValueColor). iIndent=0 is a main row, iIndent=1 is a sub-row (e.g. "Free Used/Cap" under "Total Units") that nudges the label right to reveal the formula breakdown. iValueColor=-1 leaves the value text default.
		# Passing a real COLOR_* tints just the right-aligned value so a "Total Cost + Supply" of 0 reads green at a glance. eHelpWidget=WIDGET_GENERAL means no hover, otherwise the engine's own per-line Finance breakdown surfaces on mouseover. (Claude code Opus 4.7, GPT-5.5) -->
		iTitleMargin = 12
		iRowMargin = 14
		iIndentStep = 22
		iRowSpacing = 26
		iZ = self.Z_CONTROLS + self.DZ
		screen.addPanel(self.getNextWidgetName(), u"", u"", True, True, iColX, iColY, iColW, iColH, PanelStyles.PANEL_STYLE_MAIN)
		screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagTitle + szTitle.upper() + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, iColX + iColW / 2, iColY + iTitleMargin, iZ, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		iRowY = iColY + iTitleMargin + 30
		for tRow in aRows:
			if tRow[0] is None:
				iRowY += iRowSpacing / 2
				continue
			szLabel, szValue, eHelpWidget, iData1, iData2 = tRow[0], tRow[1], tRow[2], tRow[3], tRow[4]
			# <!-- custom: Civ4 ships Python 2.4 which lacks ternary expressions; use explicit if/else assigns rather than `x if y else z`. (Claude code Opus 4.7) -->
			iIndent = 0
			if len(tRow) > 5:
				iIndent = tRow[5]
			iValueColor = -1
			if len(tRow) > 6:
				iValueColor = tRow[6]
			# <!-- custom: optional 8th tuple element is an icon path (typically gc.getUnitInfo(...).getButton()); when present we draw a small DDS sized to the row so it doesn't grow row height, then shift the value text left of the icon.
			# Empty/None path means no icon (default).
			# Optional 9th tuple element is a SECOND icon path used by lucky/unlucky combat-callout rows that pair "our unit" with "their unit"; the two icons render side-by-side immediately left of the value text.
			# Civ4 setLabel widgets can't host inline images, so we render image and text as separate widgets positioned manually. (Claude code Opus 4.7) -->
			szIconPath = u""
			if len(tRow) > 7:
				szIconPath = tRow[7]
			szIconPath2 = u""
			if len(tRow) > 8:
				szIconPath2 = tRow[8]
			iLabelX = iColX + iRowMargin + iIndent * iIndentStep
			# <!-- custom: step the right-aligned value inward at deeper indent so child numbers don't right-stack flush with their parent's total (e.g. "Outside 9 / Free 4 / Paid 5" read as three peers when all aligned at the column edge).
			# Mirroring the label stairstep on the value side restores the "9 = 4 + 5" reading at a glance. (Claude code Opus 4.7) -->
			iValueX = iColX + iColW - iRowMargin - iIndent * iIndentStep
			# <!-- custom: icon dims stay smaller than row spacing so Summary row icons do not push text-only rows taller. (Claude code Opus 4.7, GPT-5.5) -->
			iIconSize = 22
			iIconRightGap = 4
			iValueTextX = iValueX
			# <!-- custom: two-icon layout sits both icons at the row's right edge with the value text right-aligned to their left.
			# Order on screen, left-to-right: label ... value-text [icon1][icon2].
			# Previous attempt put icons immediately after the label, where the right-aligned value text would overrun them when long (the "Luckiest Win" rows).
			# Anchoring both icons to the right edge means value text always wraps cleanly to their left, regardless of text length.
			# Single-icon path unchanged (icon at right edge, text just left of it). (Claude code Opus 4.7) -->
			if szIconPath and szIconPath2:
				iIconYOffset = self.getSummaryRowIconY(iRowY)
				iIcon2Left = iValueX - iIconSize
				iIcon1Left = iIcon2Left - iIconSize
				iValueTextX = iIcon1Left - iIconRightGap
				screen.addDDSGFC(self.getNextWidgetName(), szIconPath, iIcon1Left, iIconYOffset, iIconSize, iIconSize, WidgetTypes.WIDGET_GENERAL, -1, -1)
				screen.addDDSGFC(self.getNextWidgetName(), szIconPath2, iIcon2Left, iIconYOffset, iIconSize, iIconSize, WidgetTypes.WIDGET_GENERAL, -1, -1)
			elif szIconPath:
				iValueTextX = iValueX - iIconSize - iIconRightGap
				screen.addDDSGFC(self.getNextWidgetName(), szIconPath, iValueX - iIconSize, self.getSummaryRowIconY(iRowY), iIconSize, iIconSize, WidgetTypes.WIDGET_GENERAL, -1, -1)
			szValueText = szValue
			if iValueColor >= 0:
				szValueText = localText.changeTextColor(szValueText, iValueColor)
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + szLabel + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, iLabelX, iRowY, iZ, FontTypes.GAME_FONT, eHelpWidget, iData1, iData2)
			screen.setLabel(self.getNextWidgetName(), "Background", sasFontTagLabel + szValueText + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, iValueTextX, iRowY, iZ, FontTypes.GAME_FONT, eHelpWidget, iData1, iData2)
			iRowY += iRowSpacing

	def getSummaryRowIconY(self, iRowY):
		# <!-- custom: local Summary-row icon Y nudge for DDS icons placed beside text values; keep this Military Advisor-specific because the exact offset is empirical and tied to this row height/font. (Claude code Opus 4.7, GPT-5.5) -->
		return iRowY + 1

	def getSummaryCostResultText(self, szFormula, iResult, iColor):
		# <!-- custom: every caller of this helper produces a Support-column gold amount, so suffix the result with the normal gold coin; formula operands remain unitless counts, rates, and multipliers. (Claude code Opus 4.7, GPT-5.5) -->
		return szFormula + u" = " + localText.changeTextColor(unicode(iResult) + self.GOLD_CHAR, iColor)

	def summaryBuildingHasMilitarySupportEffect(self, kInfo):
		# <!-- custom: semantic Military Summary predicate: local/global XP, military/domain production, Great-General generation, unit-combat/domain XP, or a free promotion.
		# Deliberately no hand-tuned "importance" weights; selection order comes from BuildingClass XML and only the visible row count is capped. (ChatGPT-5.6-Sol) -->
		if kInfo.getFreeExperience() > 0 or kInfo.getGlobalFreeExperience() > 0 or kInfo.getMilitaryProductionModifier() > 0 or kInfo.getGreatGeneralRateModifier() > 0 or kInfo.getDomesticGreatGeneralRateModifier() > 0 or kInfo.getFreePromotion() >= 0:
			return True
		for eDomain in (DomainTypes.DOMAIN_LAND, DomainTypes.DOMAIN_SEA, DomainTypes.DOMAIN_AIR):
			if kInfo.getDomainFreeExperience(eDomain) > 0 or kInfo.getDomainProductionModifier(eDomain) > 0:
				return True
		for iUnitCombat in range(gc.getNumUnitCombatInfos()):
			if kInfo.getUnitCombatFreeExperience(iUnitCombat) > 0:
				return True
		return False

	def summaryBuildingHasFortificationEffect(self, kInfo):
		# <!-- custom: keep the Deployment "Defenses" block about conventional city fortification, matching getBuildingDefense/anti-bombard context; anti-air/nuclear buildings are separate mechanics and are not pulled in merely because they are military-themed. (ChatGPT-5.6-Sol) -->
		return (kInfo.getDefenseModifier() > 0 or kInfo.getBombardDefenseModifier() > 0)

	def getSummaryMilitaryBuildingGroups(self, iCivilization):
		if iCivilization in self.dSummaryMilitaryBuildingGroups:
			return self.dSummaryMilitaryBuildingGroups[iCivilization]
		pCivInfo = gc.getCivilizationInfo(iCivilization)
		aLocal = []
		aNational = []
		aDefense = []
		for iClass in range(gc.getNumBuildingClassInfos()):
			iBuilding = pCivInfo.getCivilizationBuildings(iClass)
			if iBuilding < 0:
				continue
			kClass = gc.getBuildingClassInfo(iClass)
			# <!-- custom: World/team wonders are excluded from this compact own-infrastructure block; their availability depends on global state.
			# Ordinary buildings and player-limited national buildings have stable per-player coverage semantics. (ChatGPT-5.6-Sol) -->
			if kClass.getMaxGlobalInstances() >= 0 or kClass.getMaxTeamInstances() >= 0:
				continue
			kInfo = gc.getBuildingInfo(iBuilding)
			iMaxPlayerInstances = kClass.getMaxPlayerInstances()
			if iMaxPlayerInstances < 0:
				if self.summaryBuildingHasMilitarySupportEffect(kInfo):
					aLocal.append(iBuilding)
				if self.summaryBuildingHasFortificationEffect(kInfo):
					aDefense.append(iBuilding)
			elif iMaxPlayerInstances == 1 and self.summaryBuildingHasMilitarySupportEffect(kInfo):
				aNational.append(iBuilding)
		# <!-- custom: Summary column height is fixed.
		# Preserve five military-infrastructure rows total while reserving at most two for national buildings; remaining slots go to ordinary infrastructure.
		# Defenses get two rows.
		# Caps are UI layout policy, not gameplay valuation.
		# BuildingClass XML order is the deterministic tie/order rule, avoiding another balance-sensitive scoring system. (ChatGPT-5.6-Sol) -->
		iMaxMilitaryRows = 5
		iMaxNationalRows = 2
		iNationalRows = min(iMaxNationalRows, len(aNational))
		aNational = aNational[:iNationalRows]
		aLocal = aLocal[:max(0, iMaxMilitaryRows - iNationalRows)]
		aDefense = aDefense[:2]
		tGroups = (aLocal, aNational, aDefense)
		self.dSummaryMilitaryBuildingGroups[iCivilization] = tGroups
		return tGroups

	def getSummaryBuildingXpLabel(self, szBaseLabel, iBuilding):
		if iBuilding < 0:
			return szBaseLabel
		pBuildingInfo = gc.getBuildingInfo(iBuilding)
		aParts = []
		iFreeExperience = pBuildingInfo.getFreeExperience()
		if iFreeExperience > 0:
			aParts.append(u"+%d XP All" % iFreeExperience)
		for eDomain, szDomain in ((DomainTypes.DOMAIN_LAND, u"Land"), (DomainTypes.DOMAIN_SEA, u"Sea"), (DomainTypes.DOMAIN_AIR, u"Air")):
			iExperience = pBuildingInfo.getDomainFreeExperience(eDomain)
			if iExperience > 0:
				aParts.append(u"+%d XP %s" % (iExperience, szDomain))
		iMountedMeleeExperience = pBuildingInfo.getUnitCombatFreeExperience(self.iUnitCombatMountedMelee)
		iMountedRangedExperience = pBuildingInfo.getUnitCombatFreeExperience(self.iUnitCombatMountedRanged)
		if iMountedMeleeExperience > 0 and iMountedMeleeExperience == iMountedRangedExperience:
			aParts.append(u"+%d XP Mounts" % iMountedMeleeExperience)
		for iUnitCombat in range(gc.getNumUnitCombatInfos()):
			if iUnitCombat == self.iUnitCombatMountedMelee or iUnitCombat == self.iUnitCombatMountedRanged:
				continue
			iExperience = pBuildingInfo.getUnitCombatFreeExperience(iUnitCombat)
			if iExperience > 0:
				aParts.append(u"+%d XP %s" % (iExperience, gc.getUnitCombatInfo(iUnitCombat).getDescription()))
		if not aParts:
			return szBaseLabel
		return u"%s (%s)" % (szBaseLabel, u", ".join(aParts))

	def summaryBuildingHasTech(self, kTeam, iBuilding):
		# <!-- custom: True only if the active team has every AND-tech prereq, so Military Buildings rows show "-" for not-yet-researchable buildings instead of a misleading "0 / N"; same prereq scan as CvAdvisorUtils, and isHasTech(NO_TECH) is True so tech-less buildings pass. (Claude code Opus 4.7) -->
		if iBuilding < 0:
			return False
		kInfo = gc.getBuildingInfo(iBuilding)
		if not kTeam.isHasTech(kInfo.getPrereqAndTech()):
			return False
		for iPrereq in range(self.iNUM_BUILDING_AND_TECH_PREREQS):
			if not kTeam.isHasTech(kInfo.getPrereqAndTechs(iPrereq)):
				return False
		return True

	def collectBattleStats(self):
		# <!-- custom: reuse the Battles tab's recorded combat log (SASBattleHistory) to surface aggregate prowess on the Deployment column: totals, win/loss/retreat split, "good luck" wins (we beat odds < 50%) and "bad luck" losses (we lost despite odds > 50%), and the single luckiest win / unluckiest loss as one-liner callouts.
		# Retreats are not classed as luck cases because the engine retreats are dictated by promotions (Combat I/II/III withdrawal chance) rather than dice odds; counting them would muddle the signal. Luckiest = win with the lowest pre-battle odds; Unluckiest = loss with the highest pre-battle odds.
		# The thresholds at 50% are intentionally hard-edged so a 50%-odds win/loss is "expected" rather than lucky. (Claude code Opus 4.7) -->
		iTotal = 0
		iWins = 0
		iLosses = 0
		iRetreats = 0
		iGoodLuckWins = 0
		iBadLuckLosses = 0
		iLuckiestOdds = 101
		szLuckiestWinText = u""
		szLuckiestWinOurIcon = u""
		szLuckiestWinTheirIcon = u""
		iUnluckiestOdds = -1
		szUnluckiestLossText = u""
		szUnluckiestLossOurIcon = u""
		szUnluckiestLossTheirIcon = u""
		aEntries = SASBattleHistory.getEntriesForPlayer(self.iActivePlayer)
		for entry in aEntries:
			tCols = self.getBattlePerspectiveColumns(entry)
			if tCols is None:
				continue
			iTurn = tCols[0]
			iResult = tCols[1]
			iOurUnit = tCols[4]
			iOurCurrStr = tCols[5]
			iOtherUnit = tCols[11]
			iOtherCurrStr = tCols[12]
			iTotal += 1
			# <!-- custom: odds = our share of summed starting strengths; same formula as the Battles tab's getBattleEstimatedOddsText. Skip entries with zero/missing strengths (older saves before strength logging) - they don't contribute to luck stats but still count in totals. (Claude code Opus 4.7) -->
			iOdds = -1
			if iOurCurrStr > 0 and iOtherCurrStr > 0:
				iOdds = (100 * iOurCurrStr + (iOurCurrStr + iOtherCurrStr) / 2) / (iOurCurrStr + iOtherCurrStr)
			# <!-- custom: assemble the lucky-row text once per candidate so we keep the icons paired with the same battle's strengths/odds/year.
			# Strengths divided by 100 to match the Battles tab's stored-strength display; .1f gives enough precision without bloating the row.
			# Unit names are intentionally omitted from the text - the two icons already identify the units, and dropping the names is what unblocks fitting the year + strengths in the same row width. (Claude code Opus 4.7) -->
			def _luckText():
				return u"%.1f vs %.1f %s (%d%%)" % (iOurCurrStr / 100.0, iOtherCurrStr / 100.0, self.getTurnDate(iTurn), iOdds)
			if iResult == self.RESULT_WON:
				iWins += 1
				if iOdds >= 0 and iOdds < 50:
					iGoodLuckWins += 1
				if iOdds >= 0 and iOdds < iLuckiestOdds:
					iLuckiestOdds = iOdds
					szLuckiestWinText = _luckText()
					szLuckiestWinOurIcon = gc.getUnitInfo(iOurUnit).getButton()
					szLuckiestWinTheirIcon = gc.getUnitInfo(iOtherUnit).getButton()
			elif iResult == self.RESULT_LOST:
				iLosses += 1
				if iOdds >= 0 and iOdds > 50:
					iBadLuckLosses += 1
				if iOdds >= 0 and iOdds > iUnluckiestOdds:
					iUnluckiestOdds = iOdds
					szUnluckiestLossText = _luckText()
					szUnluckiestLossOurIcon = gc.getUnitInfo(iOurUnit).getButton()
					szUnluckiestLossTheirIcon = gc.getUnitInfo(iOtherUnit).getButton()
			elif iResult == self.RESULT_RETREAT:
				iRetreats += 1
		return {
			"total": iTotal,
			"wins": iWins,
			"losses": iLosses,
			"retreats": iRetreats,
			"good_luck_wins": iGoodLuckWins,
			"bad_luck_losses": iBadLuckLosses,
			"luckiest_win_text": szLuckiestWinText,
			"luckiest_win_our_icon": szLuckiestWinOurIcon,
			"luckiest_win_their_icon": szLuckiestWinTheirIcon,
			"unluckiest_loss_text": szUnluckiestLossText,
			"unluckiest_loss_our_icon": szUnluckiestLossOurIcon,
			"unluckiest_loss_their_icon": szUnluckiestLossTheirIcon,
		}

	def getSummaryHealthColor(self, fHealthPct):
		# <!-- custom: tier colors for health% match the Battles tab's current-strength coloring (getBattleCurrentStrengthText) for consistency: 100% = default (no tint, reads "full"), >66% = green, >33% = yellow, else red. Float threshold so Avg Health = 99.4% still tints green rather than reading as "perfect". (Claude code Opus 4.7) -->
		if fHealthPct >= 100:
			return -1
		if fHealthPct > 66:
			return self.COLOR_GREEN
		if fHealthPct > 33:
			return self.COLOR_YELLOW
		return self.COLOR_RED

	def drawSummary(self):
		screen = self.getScreen()
		addAdvisorDebugDropdown(screen, self.DEBUG_DROPDOWN_ID, self.iActivePlayer, bIncludeBarbarians=True, bAllowVassalPerspective=True)
		(iX, iY, iW, iH), (_, _, _, _) = getAdvisorMaximizedPanelLayout(self.W_SCREEN, self.Y_BOTTOM_PANEL)
		screen.addPanel(self.SUMMARY_PANEL_ID, "", "", True, True, iX, iY, iW, iH, PanelStyles.PANEL_STYLE_MAIN)
		placeAdvisorLegendLink(self, "CONCEPT_SAS_MILITARY_ADVISOR_SUMMARY_LEGEND", self.W_SCREEN - 12, self.Y_TITLE)
		dStats = self.collectSummaryData()

		iColGap = 20
		iSideMargin = 40
		iColW = (iW - 2 * iSideMargin - 2 * iColGap) / 3
		iColH = iH - 60
		iColY = iY + 30
		iColXSupport = iX + iSideMargin
		iColXComposition = iColXSupport + iColW + iColGap
		iColXDeployment = iColXComposition + iColW + iColGap

		eNone = WidgetTypes.WIDGET_GENERAL
		eHelpNumUnits = WidgetTypes.WIDGET_HELP_FINANCE_NUM_UNITS
		eHelpUnitCost = WidgetTypes.WIDGET_HELP_FINANCE_UNIT_COST
		eHelpAwaySupply = WidgetTypes.WIDGET_HELP_FINANCE_AWAY_SUPPLY
		# <!-- custom: Total Cost + Supply needs a combined hover for those two expenses only, not the whole-expense inflation breakdown. See KI#390. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		eHelpUnitCostAndSupply = WidgetTypes.WIDGET_HELP_FINANCE_UNIT_COST_AND_SUPPLY

		# <!-- custom: row tuple is (label, value, helpWidget, data1, data2, indent, valueColor). Support column treats Total Units as the parent of all unit-support detail: Free Used/Cap and Paid for regular unit support, then No Mil. Cost Units/Mil. Cost Units for the military surcharge, then Outside Units for supply. Blank lines separate breakdowns that use different splitting logic, while one-level indentation shows they still depend on the Total Units set.
		# Total Cost + Supply stays top-level because it combines unit cost and unit supply. Mil. Cost Units has its own Free Used/Cap and Paid threshold because paid military units are computed against getNumMilitaryUnits(), not against paid total units. No Mil. Cost includes any unit with bMilitarySupport=0, including civilians and certain combat units (e.g. Robotic Infantry); those units still count toward regular unit support. Keep count rows neutral; color only actual gold-cost rows. (Claude code Opus 4.7, GPT-5.5) -->
		# <!-- custom: Paid count rows are left default (white) rather than red, since "paid" just means "beyond the free threshold", not "unaffordable"; red is reserved for actual positive gold costs in the formulas and subtotals. (Claude code Opus 4.7, GPT-5.5) -->
		# <!-- custom: Use simple burden coloring for the support math: green = 0 cost, red = positive cost. The earlier affordability coloring (yellow if the player still had nonnegative net gold/turn) made rows like `(84 x 100 x 82) / 10000 = 68` look merely cautionary even though this panel is explaining where the unit-support burden comes from. (GPT-5.5) -->
		def _supportColor(iCost):
			if iCost <= 0:
				return self.COLOR_GREEN
			return self.COLOR_RED
		iUnitCostColor = _supportColor(dStats["unit_cost"])
		iUnitSupplyColor = _supportColor(dStats["unit_supply"])
		iTotalGold = dStats["total_gold"]
		iTotalGoldColor = _supportColor(iTotalGold)
		szUnitCostLabel = self.TEXT_SUMMARY_UNIT_COST
		szUnitSupplyLabel = self.TEXT_SUMMARY_UNIT_SUPPLY
		szTotalGoldLabel = self.TEXT_SUMMARY_TOTAL_GOLD
		if dStats["anarchy"]:
			# <!-- custom: Label each waived total so the zero result remains clear beside the preserved paid-unit/rate operands above it. See KI#219. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			szUnitCostLabel = u"%s (%s)" % (szUnitCostLabel, self.TEXT_SUMMARY_ANARCHY)
			szUnitSupplyLabel = u"%s (%s)" % (szUnitSupplyLabel, self.TEXT_SUMMARY_ANARCHY)
			szTotalGoldLabel = u"%s (%s)" % (szTotalGoldLabel, self.TEXT_SUMMARY_ANARCHY)
		szUnitCostText = self.getSummaryCostResultText(u"%d + %d + %d" % (dStats["regular_unit_cost"], dStats["military_unit_cost"], dStats["extra_unit_cost"]), dStats["raw_unit_cost"], iUnitCostColor)
		szUnitSupplyText = unicode(dStats["raw_unit_supply"]) + self.GOLD_CHAR
		szFreeUsedCapText = u"%d/%d" % (dStats["free_used_units"], dStats["free_units"])
		szFreeUsedMilitaryCapText = u"%d/%d" % (dStats["free_used_military_units"], dStats["free_military_units"])
		if dStats["inflation_percent"] > 0:
			szUnitCostLabel = u"%s x %s" % (self.TEXT_SUMMARY_UNIT_COST, self.TEXT_SUMMARY_INFLATION_SHORT)
			szUnitSupplyLabel = u"%s x %s" % (self.TEXT_SUMMARY_UNIT_SUPPLY, self.TEXT_SUMMARY_INFLATION_SHORT)
			szInflationFactor = u"%d%%" % (100 + dStats["inflation_percent"])
			szUnitCostText = self.getSummaryCostResultText(u"(%d + %d + %d = %d) x %s" % (dStats["regular_unit_cost"], dStats["military_unit_cost"], dStats["extra_unit_cost"], dStats["raw_unit_cost"], szInflationFactor), dStats["unit_cost"], iUnitCostColor)
			szUnitSupplyText = self.getSummaryCostResultText(u"%d x %s" % (dStats["raw_unit_supply"], szInflationFactor), dStats["unit_supply"], iUnitSupplyColor)
		szTotalGoldText = self.getSummaryCostResultText(u"%d + %d" % (dStats["unit_cost"], dStats["unit_supply"]), iTotalGold, iTotalGoldColor)
		szOutsideRateText = self.getSummaryCostResultText(u"(%d x %d) / 100" % (dStats["paid_outside"], dStats["gold_per_outside_unit"]), dStats["base_supply_cost"], iUnitSupplyColor)
		if dStats["base_supply_cost"] != dStats["raw_unit_supply"]:
			# <!-- custom: AI handicap multiplies the base supply cost (CvPlayer.cpp:6301-6308).
			# When the post-handicap value differs we show both: pre-handicap subtotal -> handicap-adjusted final, both as gold. (Claude code Opus 4.7) -->
			szOutsideRateText = u"(%d x %d) / 100 = %d%s -> %s" % (dStats["paid_outside"], dStats["gold_per_outside_unit"], dStats["base_supply_cost"], self.GOLD_CHAR, localText.changeTextColor(unicode(dStats["raw_unit_supply"]) + self.GOLD_CHAR, iUnitSupplyColor))
		# <!-- custom: rendered support tree:
		# Total Units
		#   Free Used/Cap / Paid / P x R x M
		#   [blank line: military-surcharge breakdown of the same Total Units set]
		#   No Mil. Cost Units
		#   Mil. Cost Units
		#     Free Used/Cap / Paid / P x R / Extra Cost
		#   [blank line: subtotal after regular support and military surcharge]
		#   U Cost x I
		#   [blank line: outside-supply breakdown of the same Total Units set]
		#   Outside Units
		#     Free / Paid / P x R
		#   U Supply x I
		# [blank line]
		# Total Cost + Supply = U Cost + U Supply. Keep this comment close to aSupport because the row tuples only show numeric indent levels, not the intended rendered hierarchy. (GPT-5.5) -->
		aSupport = [
			(self.TEXT_SUMMARY_TOTAL_UNITS, unicode(dStats["total"]), eHelpNumUnits, self.iActivePlayer, 1, 0, -1),
			(self.TEXT_SUMMARY_FREE_USED_CAP_SHORT, szFreeUsedCapText, eNone, -1, -1, 1, -1),
			(self.TEXT_SUMMARY_PAID_SHORT, unicode(dStats["paid_units"]), eNone, -1, -1, 1, -1),
			# <!-- custom: support formulas are width-constrained; XML abbreviates only formula-row labels: P = Paid, U = Unit, R = Rate, M = Modifier, I = Inflation. The adjacent full-word rows and formulas show the actual operands explicitly. (GPT-5.5) -->
			# <!-- custom: Keep the CvPlayer.cpp integer formula explicit here.
			# K-Mod stores getGoldPerUnit/getUnitCostMultiplier as percent-like ints but computes `paid * rate * multiplier / 10000`, so e.g. 1 x 100 x 82 floors to 0; showing "1 x 100% x 82%" would read like normal decimal math and look wrong. (GPT-5.5) -->
			(self.TEXT_SUMMARY_GOLD_PER_UNIT, self.getSummaryCostResultText(u"(%d x %d x %d) / 10000" % (dStats["paid_units"], dStats["gold_per_unit"], dStats["unit_cost_multiplier"]), dStats["regular_unit_cost"], iUnitCostColor), eHelpUnitCost, self.iActivePlayer, 1, 1, -1),
			(None, None, eNone, -1, -1, 0, -1),
			# <!-- custom: No Mil. Cost Units and Mil. Cost Units are sibling subsets of Total Units for the military-surcharge breakdown; neither is nested inside the other because bMilitarySupport=0 includes civilians and certain combat units (e.g. Robotic Infantry). (GPT-5.5) -->
			(self.TEXT_SUMMARY_COSTLESS_SHORT, unicode(dStats["no_mil_support"]), eNone, -1, -1, 1, -1),
			(self.TEXT_SUMMARY_MILITARY_SHORT, unicode(dStats["num_military"]), eNone, -1, -1, 1, -1),
			(self.TEXT_SUMMARY_FREE_USED_CAP_SHORT, szFreeUsedMilitaryCapText, eNone, -1, -1, 2, -1),
			(self.TEXT_SUMMARY_PAID_SHORT, unicode(dStats["paid_military_units"]), eNone, -1, -1, 2, -1),
			(self.TEXT_SUMMARY_GOLD_PER_MIL_UNIT, self.getSummaryCostResultText(u"(%d x %d) / 100" % (dStats["paid_military_units"], dStats["gold_per_military_unit"]), dStats["military_unit_cost"], iUnitCostColor), eHelpUnitCost, self.iActivePlayer, 1, 2, -1),
			(self.TEXT_SUMMARY_EXTRA_COST, unicode(dStats["extra_unit_cost"]) + self.GOLD_CHAR, eHelpUnitCost, self.iActivePlayer, 1, 2, iUnitCostColor),
			(None, None, eNone, -1, -1, 0, -1),
			(szUnitCostLabel, szUnitCostText, eHelpUnitCost, self.iActivePlayer, 1, 1, -1),
			(None, None, eNone, -1, -1, 0, -1),
			(self.TEXT_SUMMARY_OUTSIDE_UNITS, unicode(dStats["outside_units"]), eNone, -1, -1, 1, -1),
			(self.TEXT_SUMMARY_FREE_USED_SHORT, unicode(dStats["free_outside"]), eNone, -1, -1, 2, -1),
			(self.TEXT_SUMMARY_PAID_SHORT, unicode(dStats["paid_outside"]), eNone, -1, -1, 2, -1),
			(self.TEXT_SUMMARY_GOLD_PER_OUTSIDE, szOutsideRateText, eHelpAwaySupply, self.iActivePlayer, 1, 2, -1),
			(szUnitSupplyLabel, szUnitSupplyText, eHelpAwaySupply, self.iActivePlayer, 1, 1, -1),
			(None, None, eNone, -1, -1, 0, -1),
			# <!-- custom: Match this combined subtotal with its exact unit-cost and away-supply breakdowns instead of unrelated whole-player expenses. See KI#390. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			(szTotalGoldLabel, szTotalGoldText, eHelpUnitCostAndSupply, self.iActivePlayer, 1, 0, -1),
		]
		# <!-- custom: Support appends data-driven city-level military infrastructure, then military-production/new-XP aggregates and allied combat-unit hammers.
		# Ordinary water infrastructure uses coastal-city coverage; selected national buildings show their owning city. (Claude code Opus 4.7 + GPT-5.5 + ChatGPT-5.6-Sol) -->
		def _nationalWonderValue(bHasTech, szCity):
			if not bHasTech:
				return u"-"
			if szCity:
				return szCity
			return u"0 / 1"
		def _signedPct(iValue):
			if iValue > 0:
				return u"+%d%%" % iValue
			return u"%d%%" % iValue
		# <!-- custom: gate Best City rows on a strictly positive value, not just on a city existing.
		# Otherwise the first-found city always "wins" a 0-vs-0 comparison and we'd render misleading "Haithabu +0%" / "Haithabu (0)" rows when no city actually has a military-production or XP bonus. (Claude code Opus 4.7, GPT-5.5-thinking) -->
		szBestProdValue = u"-"
		if dStats["best_prod_city"] and dStats["best_prod_mod"] > 0:
			szBestProdValue = u"%s %s" % (dStats["best_prod_city"], _signedPct(dStats["best_prod_mod"]))
		szBestXpValue = u"-"
		if dStats["best_xp_city"] and dStats["best_free_xp"] > 0:
			szBestXpValue = u"%s (%d)" % (dStats["best_xp_city"], dStats["best_free_xp"])
		aSupport.extend([(None, None, eNone, -1, -1), (self.TEXT_SUMMARY_MIL_BUILDINGS, u"", eNone, -1, -1, 0, -1)])
		for szLabel, iBuilt, iDenominator, bHasTech, szIcon, bNational, szCity in dStats["military_building_rows"]:
			szValue = u"-"
			if bNational:
				szValue = _nationalWonderValue(bHasTech, szCity)
			elif bHasTech and iDenominator > 0:
				szValue = u"%d / %d" % (iBuilt, iDenominator)
			aSupport.append((szLabel, szValue, eNone, -1, -1, 1, -1, szIcon))
		aSupport.extend([(None, None, eNone, -1, -1), (self.TEXT_SUMMARY_UNIT_PRODUCTION, u"", eNone, -1, -1, 0, -1), (self.TEXT_SUMMARY_AVG_MODIFIER, _signedPct(dStats["avg_prod_mod"]), eNone, -1, -1, 1, -1), (self.TEXT_SUMMARY_BEST_CITY, szBestProdValue, eNone, -1, -1, 1, -1), (self.TEXT_SUMMARY_AVG_NEW_XP, u"%.1f" % dStats["avg_free_xp"], eNone, -1, -1, 1, -1), (self.TEXT_SUMMARY_BEST_NEW_XP, szBestXpValue, eNone, -1, -1, 1, -1), (None, None, eNone, -1, -1), (self.TEXT_SUMMARY_ALLIED_HAMMERS, unicode(dStats["allied_hammers"]) + self.HAMMER_CHAR, eNone, -1, -1, 0, -1),])
		# <!-- custom: Defenses moved from Support to Deployment because city protection reads closer to "where my forces/cities are" than to support cost. (Claude code Opus 4.7, GPT-5.5) -->

		# <!-- custom: Army column summarizes the active player's own forces: composition, strongest/costliest/power-rank callouts, quality means, Great General state, promotions, covert-unit flags, and upgrade costs. (Claude code Opus 4.7, GPT-5.5) -->
		szStrongest = u""
		szStrongestIcon = u""
		if dStats["strongest_str"] >= 0:
			szStrongest = u"%s %d%s" % (dStats["strongest_name"], dStats["strongest_str"], self.STRENGTH_CHAR)
			szStrongestIcon = dStats["strongest_button"]
		szHighestCost = u""
		szHighestCostIcon = u""
		if dStats["highest_cost"] >= 0:
			szHighestCost = u"%s %d%s" % (dStats["highest_cost_name"], dStats["highest_cost"], self.HAMMER_CHAR)
			szHighestCostIcon = dStats["highest_cost_button"]
		# <!-- custom: integer pct; a nonzero count never shows 0% (bumped to 1) so present-but-small categories are not misread as absent. (Claude code Opus 4.7) -->
		def _pctOf(iCount, iWhole):
			iP = (100 * iCount) / iWhole
			if iP == 0 and iCount > 0:
				iP = 1
			return iP
		# <!-- custom: count-and-percent helper.
		# Denominator-zero returns just the count so we do not show "0 (NaN%)". (Claude code Opus 4.7) -->
		def _withPct(iCount, iWhole):
			if iWhole <= 0:
				return unicode(iCount)
			return u"%d (%d%%)" % (iCount, _pctOf(iCount, iWhole))
		# <!-- custom: exhaustive-partition formatter so the group totals 100%.
		# Per-item floor alone undershoots, and the obvious "last = 100 - others" then wrongly tags an empty bucket: domain 20/1/0 of 21 was 95/4/0 (=99, Air shown "0 (1%)") -> 96/4/0; mil/civ 21/5 of 26 was 80/19 (=99) -> 81/19.
		# Fix: floor each, give the remainder to the largest NONZERO bucket; zero-count buckets stay 0%.
		# Partition buckets only, not overlapping/independent. (Claude code Opus 4.7) -->
		def _partPct(aCounts, iWhole):
			if iWhole <= 0:
				return [unicode(c) for c in aCounts]
			aP = [_pctOf(aCounts[i], iWhole) for i in range(len(aCounts))]
			iDiff = 100 - sum(aP)
			if iDiff != 0:
				iMaxIdx = 0
				for i in range(len(aCounts)):
					if aCounts[i] > aCounts[iMaxIdx]:
						iMaxIdx = i
				if aCounts[iMaxIdx] > 0:
					aP[iMaxIdx] += iDiff
			return [u"%d (%d%%)" % (aCounts[i], aP[i]) for i in range(len(aCounts))]
		iMilCivTotal = dStats["military"] + dStats["civilian"]
		iMilCount = dStats["military"]
		# <!-- custom: harmonized partitions (each sums to 100%). (Claude code Opus 4.7) -->
		aMilCiv = _partPct([iMilCount, dStats["civilian"]], iMilCivTotal)
		aDomain = _partPct([dStats["land"], dStats["sea"], dStats["air"]], iMilCount)
		aHealthBands = _partPct([dStats["health_full"], dStats["health_high"], dStats["health_medium"], dStats["health_low"]], iMilCount)
		szTotalHammers = unicode(dStats["total_hammers"]) + self.HAMMER_CHAR
		szAvgHammers = (u"%.1f" % dStats["avg_hammers"]) + self.HAMMER_CHAR
		szAvgXp = u"%.1f" % dStats["avg_xp"]
		szAvgLevel = u"%.1f" % dStats["avg_level"]
		szGeneralsFree = u"%d%s" % (dStats["standalone_generals"], self.GREAT_GENERAL_CHAR)
		szLedByGeneral = u"%d%s" % (dStats["led_by_general"], self.STAR_CHAR)
		szPowerRankKnown = u"%d / %d" % (dStats["known_power_rank"], dStats["known_power_players"])
		# <!-- custom: keep total/average hammer investment near strongest/costliest and power rank; these rows frame army size/value before quality rows. (Claude code Opus 4.7, GPT-5.5) -->
		aArmy = [
			(self.TEXT_SUMMARY_MILITARY_UNITS, aMilCiv[0], eNone, -1, -1),
			(self.TEXT_SUMMARY_CIVILIAN_UNITS, aMilCiv[1], eNone, -1, -1),
			(None, None, eNone, -1, -1),
			(self.TEXT_SUMMARY_LAND_UNITS, aDomain[0], eNone, -1, -1),
			(self.TEXT_SUMMARY_SEA_UNITS, aDomain[1], eNone, -1, -1),
			(self.TEXT_SUMMARY_AIR_UNITS, aDomain[2], eNone, -1, -1),
			(None, None, eNone, -1, -1),
			(self.TEXT_SUMMARY_STRONGEST, szStrongest, eNone, -1, -1, 0, -1, szStrongestIcon),
			(self.TEXT_SUMMARY_HIGHEST_COST, szHighestCost, eNone, -1, -1, 0, -1, szHighestCostIcon),
			(self.TEXT_SUMMARY_TOTAL_HAMMERS, szTotalHammers, eNone, -1, -1),
			(self.TEXT_SUMMARY_AVG_HAMMERS, szAvgHammers, eNone, -1, -1),
			(self.TEXT_SUMMARY_POWER_RANK_KNOWN, szPowerRankKnown, eNone, -1, -1),
			(None, None, eNone, -1, -1),
			(self.TEXT_SUMMARY_AVG_XP, szAvgXp, eNone, -1, -1),
			(self.TEXT_SUMMARY_AVG_LEVEL, szAvgLevel, eNone, -1, -1),
			(self.TEXT_SUMMARY_GENERALS_FREE, szGeneralsFree, eNone, -1, -1),
			(self.TEXT_SUMMARY_GENERALED, szLedByGeneral, eNone, -1, -1),
			(None, None, eNone, -1, -1),
			# <!-- custom: top promotion rows carry promotion buttons so army lean is visible at a glance; cap is applied in collectSummaryData to keep this column compact. (Claude code Opus 4.7, GPT-5.5) -->
			(self.TEXT_SUMMARY_FAVORITE_PROMOS, u"", eNone, -1, -1, 0, -1),
		]
		for szPromoName, iPromoCount, szPromoButton in dStats["favorite_promotions"]:
			# <!-- custom: promotion rows show count + %-of-military so a "Combat I 8 (62%)" reads as "most of my army has this" without further math. (Claude code Opus 4.7) -->
			aArmy.append((szPromoName, _withPct(iPromoCount, iMilCount), eNone, -1, -1, 1, -1, szPromoButton))
		# <!-- custom: covert-units block always emitted (even when all three categories are zero) so the section's existence and exhaustive coverage are discoverable.
		# Previously suppressed-when-empty, which led to "I have no covert section, is the feature broken?"
		# confusion.
		# The block is short (parent + 3 rows) and its zero state is informative - it confirms there's no covert coverage in the current army. (Claude code Opus 4.7) -->
		aArmy.extend([(None, None, eNone, -1, -1), (self.TEXT_SUMMARY_COVERT_UNITS, u"", eNone, -1, -1, 0, -1), (self.TEXT_SUMMARY_INVISIBLE, _withPct(dStats["invisible_units"], dStats["total"]), eNone, -1, -1, 1, -1), (self.TEXT_SUMMARY_SPIES, _withPct(dStats["spy_units"], dStats["total"]), eNone, -1, -1, 1, -1), (self.TEXT_SUMMARY_HIDDEN_NAT, _withPct(dStats["hidden_nat_units"], dStats["total"]), eNone, -1, -1, 1, -1),])
		aArmy.extend([(None, None, eNone, -1, -1), (self.TEXT_SUMMARY_UPGRADEABLE, unicode(dStats["upgradeable"]), eNone, -1, -1, 0, -1), (self.TEXT_SUMMARY_UPGRADE_MIN_COST, unicode(dStats["upgrade_min"]) + self.GOLD_CHAR, eNone, -1, -1, 1, -1), (self.TEXT_SUMMARY_UPGRADE_AVG_COST, unicode(dStats["upgrade_avg"]) + self.GOLD_CHAR, eNone, -1, -1, 1, -1), (self.TEXT_SUMMARY_UPGRADE_MAX_COST, unicode(dStats["upgrade_max"]) + self.GOLD_CHAR, eNone, -1, -1, 1, -1), (self.TEXT_SUMMARY_UPGRADE_TOTAL_COST, unicode(dStats["upgrade_total"]) + self.GOLD_CHAR, eNone, -1, -1, 1, -1),])

		# <!-- custom: Deployment groups unit locations by strategic relation, then city defenses, health distribution, and recent battle-history summary.
		# Wounded + health % rows reuse Battles-tab health colors for consistency. (Claude code Opus 4.7, GPT-5.5) -->
		iWoundedColor = self.getSummaryHealthColor(100 - (100 * dStats["wounded"]) / max(1, iMilCount))
		iMaxHealthColor = self.getSummaryHealthColor(dStats["max_health"])
		iAvgHealthColor = self.getSummaryHealthColor(dStats["avg_health"])
		iMinHealthColor = self.getSummaryHealthColor(dStats["min_health"])
		szWoundedValue = _withPct(dStats["wounded"], iMilCount)
		# <!-- custom: show one best-defended city instead of enumerating city garrisons; detailed per-city coverage belongs in Domestic Advisor, while this Summary tab stays at-a-glance. (Claude code Opus 4.7, GPT-5.5) -->
		szBestDefendedRow = u"-"
		if dStats["best_defense_city"] and dStats["best_defense"] > 0:
			szBestDefendedRow = u"%s (+%d%%)" % (dStats["best_defense_city"], dStats["best_defense"])
		aDeployment = [
			(self.TEXT_SUMMARY_IN_CITIES, unicode(dStats["in_cities"]), eNone, -1, -1),
			(self.TEXT_SUMMARY_BEST_DEFENDED, szBestDefendedRow, eNone, -1, -1, 1, -1),
			(self.TEXT_SUMMARY_AVG_DEFENSE, u"+%d%%" % dStats["avg_defense"], eNone, -1, -1, 1, -1),
		]
		# <!-- custom: Allied / Neutral / Enemy collapsed into one tight block (no blank lines between them) because they're three states of the same concept - "this plot belongs to a foreign civ, and our diplomatic relation with them is X".
		# Treating them as separate groups was visually misleading.
		# In Own Territory keeps its own blank separator above as it's a different category (our own land, no diplomatic relation involved).
		# Wild + At Sea also stay paired but get a separator from the diplomatic block since they're unowned plots, not "foreign". (Claude code Opus 4.7) -->
		aDeployment.extend([
			(self.TEXT_SUMMARY_IN_OWN_TERRITORY, unicode(dStats["in_own_territory"]), eNone, -1, -1),
			(None, None, eNone, -1, -1),
			(self.TEXT_SUMMARY_IN_ALLIED, unicode(dStats["in_allied"]), eNone, -1, -1),
			(self.TEXT_SUMMARY_IN_NEUTRAL, unicode(dStats["in_neutral"]), eNone, -1, -1),
			(self.TEXT_SUMMARY_IN_ENEMY, unicode(dStats["in_enemy"]), eNone, -1, -1),
			(None, None, eNone, -1, -1),
			(self.TEXT_SUMMARY_IN_WILD, unicode(dStats["in_wild"]), eNone, -1, -1),
			(self.TEXT_SUMMARY_AT_SEA, unicode(dStats["at_sea"]), eNone, -1, -1),
			(None, None, eNone, -1, -1),
			# <!-- custom: Defenses live in Deployment beside best-defended city rather than Support cost math; concrete fortification rows are appended below from XML effects rather than named building classes. (Claude code Opus 4.7 + GPT-5.5 + ChatGPT-5.6-Sol) -->
			(self.TEXT_SUMMARY_DEFENSES, u"", eNone, -1, -1, 0, -1),
		])
		for szLabel, iBuilt, iDenominator, szIcon in dStats["defense_building_rows"]:
			aDeployment.append((szLabel, u"%d / %d" % (iBuilt, iDenominator), eNone, -1, -1, 1, -1, szIcon))
		aDeployment.extend([
			(None, None, eNone, -1, -1),
			(self.TEXT_SUMMARY_WOUNDED, szWoundedValue, eNone, -1, -1, 0, iWoundedColor),
			(self.TEXT_SUMMARY_MAX_HEALTH, u"%d%%" % dStats["max_health"], eNone, -1, -1, 1, iMaxHealthColor),
			(self.TEXT_SUMMARY_AVG_HEALTH, u"%.1f%%" % dStats["avg_health"], eNone, -1, -1, 1, iAvgHealthColor),
			(self.TEXT_SUMMARY_MIN_HEALTH, u"%d%%" % dStats["min_health"], eNone, -1, -1, 1, iMinHealthColor),
			(None, None, eNone, -1, -1),
			# <!-- custom: distribution by health band complements the Max/Avg/Min range rows above: range shows the spread, bands show the shape ("most of my army at full, 1 critical" vs "half medium-health").
			# Band rows are intentionally LEFT NEUTRAL (no color) - the value here is a count of units, not a health percentage, so the green/yellow/red tier rules from getSummaryHealthColor don't apply.
			# Coloring "Low Health: 0 (0%)" red would falsely imply something is wrong.
			# The Max/Avg/Min rows above already carry the colored health verdict; the band rows just show the distribution shape. (Claude code Opus 4.7) -->
			(self.TEXT_SUMMARY_HEALTH_FULL, aHealthBands[0], eNone, -1, -1, 1, -1),
			(self.TEXT_SUMMARY_HEALTH_HIGH, aHealthBands[1], eNone, -1, -1, 1, -1),
			(self.TEXT_SUMMARY_HEALTH_MEDIUM, aHealthBands[2], eNone, -1, -1, 1, -1),
			(self.TEXT_SUMMARY_HEALTH_LOW, aHealthBands[3], eNone, -1, -1, 1, -1),
		])

		# <!-- custom: battle-record block at the bottom of Deployment, sourced from the persisted SASBattleHistory log (same data feeding the Battles tab).
		# Three sub-groups separated by blank rows: (1) outcome counts Won/Retreated/Lost (retreat in the middle because it's an intermediate outcome between win and loss); (2) luck against estimated odds - Good-Luck Wins / Bad-Luck Losses, "dice-overruled" outcomes; (3) the standout examples - Luckiest Win and Unluckiest Loss, rendered with the two-icon row layout (our unit + their unit + strengths + year + final odds).
		# Unit names are dropped from those text rows because the icons identify the units and the freed width is what lets the year + strengths fit.
		# Whole block suppressed when no battles have been recorded yet. (Claude code Opus 4.7) -->
		dBattle = self.collectBattleStats()
		if dBattle["total"] > 0:
			aDeployment.append((None, None, eNone, -1, -1))
			aDeployment.append((self.TEXT_SUMMARY_BATTLES_TOTAL, unicode(dBattle["total"]), eNone, -1, -1))
			# <!-- custom: reuse the Battles tab's outcome glyphs; harmonized so Won+Retreated+Lost = 100%. (Claude code Opus 4.7) -->
			aBattleOutcome = _partPct([dBattle["wins"], dBattle["retreats"], dBattle["losses"]], dBattle["total"])
			aDeployment.append((self.TEXT_SUMMARY_BATTLES_WON, aBattleOutcome[0], eNone, -1, -1, 1, self.COLOR_GREEN, self.ART_BATTLE_RESULT_WON))
			aDeployment.append((self.TEXT_SUMMARY_BATTLES_RETREATED, aBattleOutcome[1], eNone, -1, -1, 1, self.COLOR_YELLOW, self.ART_BATTLE_RESULT_RETREAT))
			aDeployment.append((self.TEXT_SUMMARY_BATTLES_LOST, aBattleOutcome[2], eNone, -1, -1, 1, self.COLOR_RED, self.ART_BATTLE_RESULT_LOST))
			aDeployment.append((None, None, eNone, -1, -1))
			# <!-- custom: always show both luck rows (counts are naturally 0) so Deployment keeps a stable row set, instead of dropping Bad-Luck Losses with no losses yet. (Claude code Opus 4.7) -->
			aDeployment.append((self.TEXT_SUMMARY_GOOD_LUCK, _withPct(dBattle["good_luck_wins"], dBattle["wins"]), eNone, -1, -1, 1, self.COLOR_GREEN))
			aDeployment.append((self.TEXT_SUMMARY_BAD_LUCK, _withPct(dBattle["bad_luck_losses"], dBattle["losses"]), eNone, -1, -1, 1, self.COLOR_RED))
			aDeployment.append((None, None, eNone, -1, -1))
			# <!-- custom: always show Luckiest/Unluckiest too; "-" placeholder (no icons) when no qualifying battle yet, like Best Defended. (Claude code Opus 4.7) -->
			if dBattle["luckiest_win_text"]:
				aDeployment.append((self.TEXT_SUMMARY_LUCKIEST_WIN, dBattle["luckiest_win_text"], eNone, -1, -1, 1, self.COLOR_GREEN, dBattle["luckiest_win_our_icon"], dBattle["luckiest_win_their_icon"]))
			else:
				aDeployment.append((self.TEXT_SUMMARY_LUCKIEST_WIN, u"-", eNone, -1, -1, 1, -1))
			if dBattle["unluckiest_loss_text"]:
				aDeployment.append((self.TEXT_SUMMARY_UNLUCKIEST_LOSS, dBattle["unluckiest_loss_text"], eNone, -1, -1, 1, self.COLOR_RED, dBattle["unluckiest_loss_our_icon"], dBattle["unluckiest_loss_their_icon"]))
			else:
				aDeployment.append((self.TEXT_SUMMARY_UNLUCKIEST_LOSS, u"-", eNone, -1, -1, 1, -1))

		self.drawSummaryColumn(screen, iColXSupport, iColY, iColW, iColH, self.TEXT_SUMMARY_SUPPORT, aSupport)
		self.drawSummaryColumn(screen, iColXComposition, iColY, iColW, iColH, self.TEXT_SUMMARY_ARMY, aArmy)
		self.drawSummaryColumn(screen, iColXDeployment, iColY, iColW, iColH, self.TEXT_SUMMARY_DEPLOYMENT, aDeployment)

	def drawComposition(self):
		screen = self.getScreen()
		addAdvisorDebugDropdown(screen, self.DEBUG_DROPDOWN_ID, self.iActivePlayer, bIncludeBarbarians=True, bAllowVassalPerspective=True)
		# <!-- custom: counts + tight row height compress vertical space well, so an outer-margin layout would already fit all rows.
		# But we use the maxed advisor layout because Promotion or Unit type lists can grow unexpectedly; e.g., in mod-mods (although we don't support them). (Claude code Opus 4.7) -->
		(iX, iY, iW, iH), (iTableX, iTableY, iTableW, iTableH) = getAdvisorMaximizedPanelLayout(self.W_SCREEN, self.Y_BOTTOM_PANEL)
		screen.addPanel(self.UNIT_PANEL_ID, "", "", True, True, iX, iY, iW, iH, PanelStyles.PANEL_STYLE_MAIN)
		iTableGap = 14
		iColW = (iTableW - 2 * iTableGap) / 3
		dUnits, dPromotions, dCombats = self.collectCompositionData()
		self.drawCompositionCountTable(self.COMPOSITION_UNITS_TABLE_ID, self.TEXT_COMPOSITION_UNITS, dUnits, gc.getUnitInfo, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iTableX, iTableY, iColW, iTableH)
		self.drawCompositionCountTable(self.COMPOSITION_PROMOTIONS_TABLE_ID, self.TEXT_COMPOSITION_PROMOTIONS, dPromotions, gc.getPromotionInfo, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, iTableX + iColW + iTableGap, iTableY, iColW, iTableH)
		self.drawCompositionCountTable(self.COMPOSITION_COMBATS_TABLE_ID, self.TEXT_COMPOSITION_COMBATS, dCombats, gc.getUnitCombatInfo, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT_COMBAT, iTableX + 2 * (iColW + iTableGap), iTableY, iColW, iTableH)

	def drawCombatExperience(self):
		if self.iActivePage != self.PAGE_MAP:
			return
		# <!-- custom: hoist repeated active-player/threshold lookups to locals for this draw pass (same behavior, fewer repeated engine calls). (GPT-5.3-Codex) -->
		kActivePlayer = gc.getPlayer(self.iActivePlayer)
		iGreatPeopleThreshold = kActivePlayer.greatPeopleThreshold(true)
		if iGreatPeopleThreshold > 0:
			iExperience = kActivePlayer.getCombatExperience()

			screen = self.getScreen()
			screen.addStackedBarGFC(self.GREAT_GENERAL_BAR_ID, self.X_GREAT_GENERAL_BAR, self.Y_GREAT_GENERAL_BAR, self.W_GREAT_GENERAL_BAR, self.H_GREAT_GENERAL_BAR, InfoBarTypes.NUM_INFOBAR_TYPES, WidgetTypes.WIDGET_HELP_GREAT_GENERAL, -1, -1)
			screen.setStackedBarColors(self.GREAT_GENERAL_BAR_ID, InfoBarTypes.INFOBAR_STORED, self.COLOR_GREAT_PEOPLE_STORED)
			screen.setStackedBarColors(self.GREAT_GENERAL_BAR_ID, InfoBarTypes.INFOBAR_RATE, self.COLOR_GREAT_PEOPLE_RATE)
			screen.setStackedBarColors(self.GREAT_GENERAL_BAR_ID, InfoBarTypes.INFOBAR_RATE_EXTRA, self.COLOR_EMPTY)
			screen.setStackedBarColors(self.GREAT_GENERAL_BAR_ID, InfoBarTypes.INFOBAR_EMPTY, self.COLOR_EMPTY)
			screen.setBarPercentage(self.GREAT_GENERAL_BAR_ID, InfoBarTypes.INFOBAR_STORED, float(iExperience) / float(iGreatPeopleThreshold))
			screen.setLabel(self.GREAT_GENERAL_LABEL_ID, "", sasFontTagLabel + self.TEXT_COMBAT_EXPERIENCE + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, self.X_GREAT_GENERAL_BAR + self.W_GREAT_GENERAL_BAR/2, self.Y_GREAT_GENERAL_BAR + 6, 0, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_GREAT_GENERAL, -1, -1)

	# returns a unique ID for a widget in this screen
	def getNextWidgetName(self):
		szName = self.WIDGET_ID + str(self.nWidgetCount)
		self.nWidgetCount += 1
		return szName

	def resetMinimapColor(self):
		screen = self.getScreen()
		kMap = CyMap()
		for iX in range(kMap.getGridWidth()):
			for iY in range(kMap.getGridHeight()):
				screen.setMinimapColor(MinimapModeTypes.MINIMAPMODE_MILITARY, iX, iY, -1, 0.6)

	def getBattlePlotFromInput(self, inputClass):
		# <!-- custom: prefer the row cache because table cell data1/data2 can empirically arrive as valid-looking default map-corner coordinates (e.g. 0,0), making the camera jump to a corner instead of the battle plot. (GPT-5.5) -->
		if inputClass.getData() in self.BATTLE_ROW_PLOTS:
			return self.BATTLE_ROW_PLOTS[inputClass.getData()]
		iX = inputClass.getData1()
		iY = inputClass.getData2()
		if 0 <= iX < CyMap().getGridWidth() and 0 <= iY < CyMap().getGridHeight():
			return (iX, iY)
		return self.BATTLE_ROW_PLOTS.get(inputClass.getData(), (-1, -1))

	# handle the input for this screen...
	def handleInput (self, inputClass):
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED and inputClass.getFunctionName() == self.DEBUG_DROPDOWN_ID:
			self.iActivePlayer = getAdvisorDebugDropdownSelectedPlayer(self.getScreen(), self.DEBUG_DROPDOWN_ID)
			self.iAdvisorPerspectivePlayer = self.iActivePlayer
			self.redrawContents()
			return 1
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED and inputClass.getFunctionName() == self.BATTLE_LOG_BUTTON_ID:
			self.dbgLogBattleHistory()
			return 1
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED and inputClass.getFunctionName() == self.BATTLE_TABLE_ID and inputClass.getMouseX() == self.BATTLE_PLOT_COL_ID:
			iX, iY = self.getBattlePlotFromInput(inputClass)
			if iX >= 0 and iY >= 0:
				self.hideScreen()
				CyCamera().JustLookAtPlot(CyMap().plot(iX, iY))
			return 1
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED and inputClass.getButtonType() == WidgetTypes.WIDGET_GENERAL:
			iPage = inputClass.getData1()
			if iPage in self.PAGE_IDS and iPage != self.iActivePage:
				self.iActivePage = iPage
				self.redrawContents()
				return 1
		if ( inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED and inputClass.getFunctionName() in (self.UNIT_BUTTON_ID, self.UNIT_BUTTON_LABEL_ID)) :
			self.bUnitDetails = not self.bUnitDetails
			self.refreshUnitSelection(True)
		elif (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CHARACTER):
			if (inputClass.getData() == int(InputTypes.KB_LSHIFT) or inputClass.getData() == int(InputTypes.KB_RSHIFT)):
				self.iShiftKeyDown = inputClass.getID() 

		return 0

	def update(self, fDelta):
		if self.iActivePage != self.PAGE_MAP:
			return
		screen = self.getScreen()
		screen.updateMinimap(fDelta)

	def minimapClicked(self):
		self.hideScreen()

	def isSelectedGroup(self, iGroup, bIndirect):
		if (bIndirect):
			if -1 in self.selectedGroupList:
				return True
			if iGroup == -1:
				return False
		return ((iGroup + gc.getNumUnitInfos()) in self.selectedGroupList)

	def isSelectedUnitType(self, iUnit, bIndirect):
		if (bIndirect):
			if -1 in self.selectedGroupList:
				return True
			if self.isSelectedGroup(gc.getUnitInfo(iUnit).getUnitCombatType(), True):
				return True
		return (iUnit in self.selectedGroupList)

	def isSelectedUnit(self, iPlayer, iUnitId, bIndirect):
		if (bIndirect):
			if -1 in self.selectedGroupList:
				return True
			unit = gc.getPlayer(iPlayer).getUnit(iUnitId)
			if self.isSelectedGroup(gc.getUnitInfo(unit.getUnitType()).getUnitCombatType(), True):
				return True
			if self.isSelectedUnitType(unit.getUnitType(), True):
				return True
		return ((iPlayer, iUnitId) in self.selectedUnitList)

	def refreshSelectedLeader(self, iPlayer):
		if self.iActivePage != self.PAGE_MAP:
			return
		if self.iShiftKeyDown == 1:
			if (iPlayer in self.selectedPlayerList):
				self.selectedPlayerList.remove(iPlayer)
			else:
				self.selectedPlayerList.append(iPlayer)
		else:
			self.selectedPlayerList = []
			self.selectedPlayerList.append(iPlayer)	

		self.refresh(True)

	def getLeaderButton(self, iPlayer):
		szName = self.LEADER_BUTTON_ID + str(iPlayer)
		return szName

	def refreshSelectedGroup(self, iSelected):
		if self.iActivePage != self.PAGE_MAP:
			return
		if (iSelected in self.selectedGroupList):
			self.selectedGroupList.remove(iSelected)
		else:
			self.selectedGroupList.append(iSelected)
		self.refreshUnitSelection(false)

	def refreshSelectedUnit(self, iPlayer, iUnitId):
		if self.iActivePage != self.PAGE_MAP:
			return
		selectedUnit = (iPlayer, iUnitId)
		if (selectedUnit in self.selectedUnitList):
			self.selectedUnitList.remove(selectedUnit)
		else:
			self.selectedUnitList.append(selectedUnit)
		self.refreshUnitSelection(false)		

	# <!-- custom: refactor refreshUnitSelection to reduce redundancy while preserving behavior. Credit: ChatGPT 5.2; Gemini 3 Pro review. (GPT-5.2-Codex (summarized)) -->
	# <!-- custom: add icon overlays on listbox rows with indent. Credit: Claude Opus 4.5. (GPT-5.2-Codex (summarized)) -->
	def refreshUnitSelection(self, bReload):
		screen = self.getScreen()

		screen.minimapClearAllFlashingTiles()

		iNumUnitInfos = gc.getNumUnitInfos()
		# <!-- custom: hoist active-player/team lookups used across loops in this refresh pass to avoid repeated accessor calls. (GPT-5.3-Codex) -->
		kActivePlayer = gc.getPlayer(self.iActivePlayer)
		iActiveTeam = kActivePlayer.getTeam()

		if (bReload):
			if (self.bUnitDetails):
				iButtonStyle = ButtonStyles.BUTTON_STYLE_CITY_MINUS
				szButtonText = self.TEXT_UNIT_TOGGLE_OFF
			else:
				iButtonStyle = ButtonStyles.BUTTON_STYLE_CITY_PLUS
				szButtonText = self.TEXT_UNIT_TOGGLE_ON
			screen.setButtonGFC(self.UNIT_BUTTON_ID, u"", "", self.X_TEXT + self.PANEL_MARGIN, self.Y_TEXT + self.PANEL_MARGIN/2, 20, 20, WidgetTypes.WIDGET_GENERAL, -1, -1, iButtonStyle )
			# <!-- custom: keep the quiet original +/- toggle placement, but make the descriptive text clickable too; a top-right action button was easier to click but visually distracting for this map/list view. (GPT-5.5) -->
			screen.setText(self.UNIT_BUTTON_LABEL_ID, "", sasFontTagLabel + szButtonText + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, self.X_TEXT + self.PANEL_MARGIN + 22, self.Y_TEXT + self.PANEL_MARGIN/2 + 2, 0, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# self.unitsList[iUnit][0] is the UnitCombatGroup (e.g. Melee)
		# self.unitsList[iUnit][1] is the unit type (e.g. Warrior)
		# self.unitsList[iUnit][2] is a list of the active player's actual units
		# self.unitsList[iUnit][3] is the total number of those units seen by the active player (not only his own)

		iColorYellow = self.COLOR_YELLOW
		iColorRed = self.COLOR_RED
		iColorWhite = self.COLOR_WHITE
		iWidget = WidgetTypes.WIDGET_MINIMAP_HIGHLIGHT
		iFont = CvUtil.FONT_LEFT_JUSTIFY

		# <!-- custom: render icons inline in list rows so icon/text stay aligned while scrolling; this replaces the old overlay-button workaround that did not scroll with rows. (GPT-5.3-Codex) -->
		iListX = self.X_TEXT + self.PANEL_MARGIN
		iListY = self.Y_TEXT + self.PANEL_MARGIN + 15
		iListW = self.W_TEXT - 2*self.PANEL_MARGIN
		iListH = self.H_TEXT - 2*self.PANEL_MARGIN - 15
		# Extra text-indent is only needed when icons are disabled.
		szUnitIndentSpace = u"  "  # <!-- custom: level 1: unit-type rows (e.g. Warrior (3)) (GPT-5.3-Codex) -->
		szDetailIndentSpace = u"      "  # <!-- custom: level 2: individual unit detail rows (expanded view) (GPT-5.3-Codex) -->
		if self.IS_SAS_CV_MILITARY_ADVISOR_UNIT_COMBATS_UNITS_ICONS:
			# <!-- custom: with inline icons enabled, remove legacy text-side indents so icon indent/gap can be tuned independently; otherwise icon-indent edits would drag icon and text together.
			# This notably lets us keep unit icons close to unit text for readability instead of forcing a wide icon->text gap. (GPT-5.3-Codex) -->
			szUnitIndentSpace = u" "
			szDetailIndentSpace = u""
		# <!-- custom: per-level icon spacing.
		# Level meaning: 0 = unit-combat group header rows (e.g. MELEE), 1 = unit-type rows (e.g. Warrior (3)), 2 = individual unit detail rows (when details are expanded).
		# Combat headers stay left with a bit more icon->text gap; unit/detail rows are pushed right and kept closer to their text for faster scanning. (GPT-5.3-Codex) -->
		szIconIndentByLevel = {0: u"", 1: u"    ", 2: u"        "}
		szIconGapByLevel = {0: u"  ", 1: u"", 2: u" "}

		def formatListRowText(szText, szButton, iIndentLevel):
			szLabelText = sasFontTagLabel + szText + SAS_FONT_TAG_CLOSE
			if self.IS_SAS_CV_MILITARY_ADVISOR_UNIT_COMBATS_UNITS_ICONS and szButton:
				# <!-- custom: note: inline <img> icon paths are stricter than icon-slot rendering; avoid risky button filenames (spaces/parentheses) to prevent magenta icons. See KI#118. (GPT-5.3-Codex) -->
				return u"%s<img=%s size=%d></img>%s%s" % (szIconIndentByLevel.get(iIndentLevel, u""), szButton, self.iInlineIconSize, szIconGapByLevel.get(iIndentLevel, u" "), szLabelText)
			return szLabelText

		if (bReload):
			def addUnitListRow(iIndex, szText, iData1, iData2, szButton, iIndentLevel):
				screen.appendListBoxString(self.UNIT_LIST_ID, formatListRowText(szText, szButton, iIndentLevel), iWidget, iData1, iData2, iFont)
				return iIndex + 1
		else:
			def addUnitListRow(iIndex, szText, iData1, iData2, szButton, iIndentLevel):
				screen.setListBoxStringGFC(self.UNIT_LIST_ID, iIndex, formatListRowText(szText, szButton, iIndentLevel), iWidget, iData1, iData2, iFont)
				return iIndex + 1

		def formatSelection(szIndent, szText, bUnderline, bYellow):
			if (bUnderline):
				szText = szIndent + u"<u>" + szText + u"</u>"
			else:
				szText = szIndent + szText
			if (bYellow):
				szText = localText.changeTextColor(szText, iColorYellow)
			return szText

		if bReload:
			for iUnit in range(iNumUnitInfos):
				kUnitInfo = gc.getUnitInfo(iUnit)
				self.unitsList[iUnit] = (kUnitInfo.getUnitCombatType(), iUnit, [], 0)

			for iPlayer in range(gc.getMAX_PLAYERS()):			
				player = PyPlayer(iPlayer)
				if (player.isAlive()):
					unitList = player.getUnitList()
					for loopUnit in unitList:
						unitType = loopUnit.getUnitType()

						bVisible = False
						plot = loopUnit.plot()
						if (not plot.isNone()):
							# advc.007: bDebug=True in isVisible and isInvisible call
							bVisible = plot.isVisible(iActiveTeam, True) and not loopUnit.isInvisible(iActiveTeam, True)

						if unitType >= 0 and unitType < iNumUnitInfos and bVisible:
							iNumUnits = self.unitsList[unitType][3]
							if (iPlayer == self.iActivePlayer):
								iNumUnits += 1
							if loopUnit.getVisualOwner() in self.selectedPlayerList:
								self.unitsList[unitType][2].append(loopUnit)							

							self.unitsList[unitType] = (self.unitsList[unitType][0], self.unitsList[unitType][1], self.unitsList[unitType][2], iNumUnits)

			# sort by unit combat type
			self.unitsList.sort()

		szText = self.TEXT_ALL_UNITS
		bAllSelected = (-1 in self.selectedGroupList)
		szText = formatSelection(u"", szText, bAllSelected, bAllSelected)
		if (bReload):
			screen.addListBoxGFC(self.UNIT_LIST_ID, "", iListX, iListY, iListW, iListH, TableStyles.TABLE_STYLE_STANDARD)
			screen.enableSelect(self.UNIT_LIST_ID, False)
			screen.setStyle(self.UNIT_LIST_ID, "Table_StandardCiv_Style")

		iPrevUnitCombat = -2
		iItem = addUnitListRow(0, szText, 1, -1, "", 0)

		dPrimaryColor = {}
		dPyPlayer = {}
		dTeamId = {}

		for iUnit in range(iNumUnitInfos):
			if (len(self.unitsList[iUnit][2]) > 0):
				if (iPrevUnitCombat != self.unitsList[iUnit][0] and self.unitsList[iUnit][0] != -1):
					iPrevUnitCombat = self.unitsList[iUnit][0]
					szDescription = gc.getUnitCombatInfo(self.unitsList[iUnit][0]).getDescription().upper()
					szCombatButton = gc.getUnitCombatInfo(self.unitsList[iUnit][0]).getButton()
					szDescription = formatSelection(u"", szDescription, self.isSelectedGroup(self.unitsList[iUnit][0], False), self.isSelectedGroup(self.unitsList[iUnit][0], True))
					iItem = addUnitListRow(iItem, szDescription, 1, self.unitsList[iUnit][0] + iNumUnitInfos, szCombatButton, 0)

				kUnitInfo = gc.getUnitInfo(self.unitsList[iUnit][1])
				szDescription = kUnitInfo.getDescription() + u" (" + unicode(len(self.unitsList[iUnit][2])) + u")"
				szUnitButton = kUnitInfo.getButton()
				szDescription = formatSelection(szUnitIndentSpace, szDescription, self.isSelectedUnitType(self.unitsList[iUnit][1], False), self.isSelectedUnitType(self.unitsList[iUnit][1], True))
				iItem = addUnitListRow(iItem, szDescription, 1, self.unitsList[iUnit][1], szUnitButton, 1)

				for loopUnit in self.unitsList[iUnit][2]:

					if (self.bUnitDetails):
						# <!-- custom: Expanded Map-tab unit rows already belong to the selected player; use the DLL omit-owner wrapper to avoid repeated owner suffixes like "PC", which also corrupted worker build rows such as "Build a Cottage" into "Build a PC". See KI#130. (GPT-5.5); note: this also fixes stray leader name in unit rows, which was unneeded since selected leader is already visible. -->
						szDescription = CyGameTextMgr().getSpecificUnitHelpOmitOwner(loopUnit, true, false, self.iPromotionInlineIconSize)

						listMatches = re.findall("<.*?color.*?>", szDescription)	
						for szMatch in listMatches:
							szDescription = szDescription.replace(szMatch, u"")

						if (loopUnit.isWaiting()):
							szDescription = '*' + szDescription

						szDescription = formatSelection(szDetailIndentSpace, szDescription, self.isSelectedUnit(loopUnit.getOwner(), loopUnit.getID(), False), self.isSelectedUnit(loopUnit.getOwner(), loopUnit.getID(), True))
						szUnitButton = gc.getUnitInfo(loopUnit.getUnitType()).getButton()
						iItem = addUnitListRow(iItem, szDescription, -loopUnit.getOwner(), loopUnit.getID(), szUnitButton, 2)

					iPlayer = loopUnit.getVisualOwner()
					if (iPlayer not in dPyPlayer):
						dPyPlayer[iPlayer] = PyPlayer(iPlayer)
					player = dPyPlayer[iPlayer]
					if (iPlayer not in dPrimaryColor):
						dPrimaryColor[iPlayer] = gc.getPlayerColorInfo(gc.getPlayer(iPlayer).getPlayerColor()).getColorTypePrimary()
					iColor = dPrimaryColor[iPlayer]
					screen.setMinimapColor(MinimapModeTypes.MINIMAPMODE_MILITARY, loopUnit.getX(), loopUnit.getY(), iColor, 0.6)
					if (self.isSelectedUnit(loopUnit.getOwner(), loopUnit.getID(), True) and (iPlayer in self.selectedPlayerList)):
						if (player.getTeam().isAtWar(iActiveTeam)):
							iFlashColor = iColorRed
						else:
							if (iPlayer not in dTeamId):
								dTeamId[iPlayer] = gc.getPlayer(iPlayer).getTeam()
							if (dTeamId[iPlayer] != iActiveTeam):
								iFlashColor = iColorYellow
							else:
								iFlashColor = iColorWhite
						screen.minimapFlashPlot(loopUnit.getX(), loopUnit.getY(), iFlashColor, -1)

	def refresh(self, bReload):

		if (self.iActivePlayer < 0):
			return

		screen = self.getScreen()
		# <!-- custom: hoist values reused by the leader loop (team/debug) so we don't repeat gc lookups per iteration. (GPT-5.3-Codex) -->
		kActivePlayer = gc.getPlayer(self.iActivePlayer)
		iActiveTeam = kActivePlayer.getTeam()
		bDebugMode = gc.getGame().isDebugMode()

		if (bReload):
			# Set scrollable area for unit buttons
			screen.addPanel(self.UNIT_PANEL_ID, "", "", True, True, self.X_TEXT, self.Y_TEXT, self.W_TEXT, self.H_TEXT, PanelStyles.PANEL_STYLE_MAIN)

			# Set scrollable area for leaders
			screen.addPanel(self.LEADER_PANEL_ID, "", "", False, True, self.X_LEADERS, self.Y_LEADERS, self.W_LEADERS, self.H_LEADERS, PanelStyles.PANEL_STYLE_MAIN)

		listLeaders = []
		for iLoopPlayer in range(gc.getMAX_PLAYERS()):
			player = gc.getPlayer(iLoopPlayer)
			if player.isAlive() and (gc.getTeam(player.getTeam()).isHasMet(iActiveTeam) or bDebugMode):
				listLeaders.append(iLoopPlayer)

		iNumLeaders = len(listLeaders)
		# <!-- custom: choose the largest 8-pixel leader-button size that makes the entire current roster fit inside the leader panel.
		# Square/narrow map aspect ratios can make W_LEADERS much smaller than on ordinary wide maps, so a single fixed overflow size could still exceed H_LEADERS with SAS48; normal layouts keep the preferred 64-pixel size. (ChatGPT-5.6-Sol) -->
		iButtonSize = self.LEADER_BUTTON_SIZE
		while iButtonSize > self.LEADER_BUTTON_SIZE_STEP:
			iColumns = max(1, int(self.W_LEADERS / (iButtonSize + self.LEADER_MARGIN)))
			iRows = (iNumLeaders + iColumns - 1) / iColumns
			if iRows * (iButtonSize + self.LEADER_MARGIN) <= self.H_LEADERS:
				break
			iButtonSize -= self.LEADER_BUTTON_SIZE_STEP
		iColumns = max(1, int(self.W_LEADERS / (iButtonSize + self.LEADER_MARGIN)))

		# loop through all players and display leaderheads
		for iIndex in range(iNumLeaders):
			iLoopPlayer = listLeaders[iIndex]
			player = gc.getPlayer(iLoopPlayer)

			x = self.X_LEADERS + self.LEADER_MARGIN + (iIndex % iColumns) * (iButtonSize + self.LEADER_MARGIN)
			y = self.Y_LEADERS + self.LEADER_MARGIN + (iIndex // iColumns) * (iButtonSize + self.LEADER_MARGIN)

			if (bReload):
				if player.isBarbarian():
					# <!-- custom: read the Barbarian button path from CIV4ArtDefines_Civilization.xml instead of hardcoding "Art/Interface/Buttons/Civilizations/Barbarian.dds". (Claude code Opus 4.7) -->
					szButton = gc.getCivilizationInfo(player.getCivilizationType()).getButton()
				else:
					szButton = gc.getLeaderHeadInfo(gc.getPlayer(iLoopPlayer).getLeaderType()).getButton()
				screen.addCheckBoxGFC(self.getLeaderButton(iLoopPlayer), szButton, self.ART_BUTTON_HILITE_SQUARE, x, y, iButtonSize, iButtonSize, WidgetTypes.WIDGET_MINIMAP_HIGHLIGHT, 2, iLoopPlayer, ButtonStyles.BUTTON_STYLE_LABEL)
				screen.setState(self.getLeaderButton(iLoopPlayer), (iLoopPlayer in self.selectedPlayerList))				

		self.refreshUnitSelection(bReload)
