# Sid Meier's Civilization 4
# Copyright Firaxis Games 2005
#
#
# Sevopedia 2.3
#   sevotastic.blogspot.com
#   sevotastic@yahoo.com
#
# additional work by Gaurav, Progor, Ket, Vovan, Fitchn, LunarMongoose, EmperorFool
# see ReadMe [advc: BUG help file] for details
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from CvPythonExtensions import *

import CvUtil
import ScreenInput
import SevoScreenEnums

import CvPediaScreen
import SevoPediaTech
import SevoPediaUnit
import SevoPediaBuilding
import SevoPediaPromotion
import SevoPediaUnitChart
import SevoPediaHandicapChart
import SevoPediaGameSpeedChart
import SevoPediaWorldSizeChart
import SevoPediaEraChart
import SevoPediaBonus
import SevoPediaTerrain
import SevoPediaFeature
import SevoPediaImprovement
import SevoPediaBuild
import SevoPediaCivic
import SevoPediaCivilization
import SevoPediaLeader
import SevoPediaTrait
import SevoPediaSpecialist
import SevoPediaHistory
import SevoPediaProject
import SevoPediaReligion
import SevoPediaCorporation
import SevoPediaVote
import SevoPediaEventTrigger
import SevoPediaMovie
import SevoPediaMusic
import SevoPediaIndex

import UnitUpgradesGraph
import TraitUtil
import BugCore
import BugUtil
from SASFontUtils import *
import SASTextScale
from SASUtils import getInfoTypeOrFail
from SASMagicNumbers import *

from _sevopedia_helpers import *
import _sevopedia_main_groupings as SAS_MainGroupings
import SASDefineGuard

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

AdvisorOpt = BugCore.game.Advisors

g_TraitUtilInitDone = False

class SevoPediaMain(CvPediaScreen.CvPediaScreen):

	def __init__(self):
		self.PEDIA_MAIN_SCREEN	= "PediaMainScreen"
		self.INTERFACE_ART_INFO	= "SCREEN_BG_OPAQUE"

		self.TAB_TOC   = "Contents"
		self.TAB_INDEX = "Index"
		self.SAS_USE_BOTTOM_TABS = False

		self.WIDGET_ID		= "PediaMainWidget"
		self.BACKGROUND_ID	= "PediaMainBackground"
		self.TOP_PANEL_ID	= "PediaMainTopPanel"
		self.BOT_PANEL_ID	= "PediaMainBottomPanel"
		self.HEAD_ID		= "PediaMainHeader"
		self.TOC_ID			= "PediaMainContents"
		self.INDEX_ID		= "PediaMainIndex"
		self.BACK_ID		= "PediaMainBack"
		self.NEXT_ID		= "PediaMainForward"
		self.SAS_CLEAR_ID = "PediaMainClear"
		self.SAS_GAME_PLAYER_ID_PREV_ID = "PediaMainGamePlayerIdPrev"
		self.SAS_GAME_PLAYER_ID_NEXT_ID = "PediaMainGamePlayerIdNext"
		self.EXIT_ID		= "PediaMainExit"
		self.CATEGORY_LIST_ID	= "PediaMainCategoryList"
		self.ITEM_LIST_ID	= "PediaMainItemList"
		self.UPGRADES_GRAPH_ID	= "PediaMainUpgradesGraph"

		# <!-- custom: search bar for the left item list (initially based on how other mod(s) do) (chatgpt 5.2 + claude opus 4.5) -->
		self.SAS_SEARCH_PANEL_ID = "PediaMainSearchPanel"
		self.SAS_SEARCH_LABEL_ID = "PediaMainSearchLabel"
		self.SAS_CLEAR_SEARCH_ID = "PediaMainSearchClear"
		self.SAS_SEARCH_KEYS_TOGGLE_ID = "PediaMainSearchKeysToggle"
		self.SAS_SEARCH_KEY_PREFIX = "PediaMainSearchKey"
		self.SAS_SEARCH_DEFAULT_TEXT = u"Enter text"
		self.SAS_SEARCH_KEYS_TEXT = u"KEYS"
		self.SAS_SEARCH_H = 32
		self.SAS_SEARCH_KEYBOARD_CHARS = (u"(", u")", u"_", u"/", u"*", u"+", u"-", u"=", u"[", u"]", u"'", u"#", u"&", u"~", u";", u",", u":", u"!", u"?", u".", u"@", u"\\", u"|", u"<", u">")
		# <!-- custom: End - search bar for the left item list (initially based on how other mod(s) do) (chatgpt 5.2 + claude opus 4.5) -->
		self.H_SCREEN = 768
		self.W_SCREEN = 1024

		# <advc.004y>
		self.bWideScreen = True
		self.bFullScreen = True
		if self.bFullScreen:
			self.bWideScreen = True
		# <!-- custom: Not much value in being able to see the elements at the edge parts of the screen, however i think the extra room can be useful to fit more data or simply enlarge the view, hopefully making it all clearer and more pleasant to see.
		# Other mods use the entire screen for the sevopedia, tested removing margins and i like it very much so removed them now. -->
		self.HORIZONTAL_MARGIN = 0
		# VERTICAL_MARGIN: Want the Advisor buttons to remain visible. BOTTOM_MARGIN could be 0, but I don't think asymmetrical margins look good.
		self.TOP_MARGIN = 0
		self.BOTTOM_MARGIN = 0
		if self.bWideScreen:
			self.W_SCREEN = max(self.W_SCREEN, self.getScreen().getXResolution() - 2 * self.HORIZONTAL_MARGIN)
			if self.W_SCREEN <= 1024:
				self.bWideScreen = False
				self.bFullScreen = False
		if self.bFullScreen:
			self.H_SCREEN = max(self.H_SCREEN, self.getScreen().getYResolution() - self.BOTTOM_MARGIN - self.TOP_MARGIN)
			if self.H_SCREEN <= 768:
				self.bFullScreen = False
		#self.X_SCREEN = 500 # now unused
		#self.Y_SCREEN = 396 # unused to begin with
		# </advc.004y>

		self.H_PANEL = 55

		self.X_TOP_PANEL = 0
		self.Y_TOP_PANEL = 0
		self.W_TOP_PANEL = self.W_SCREEN
		self.H_TOP_PANEL = self.H_PANEL

		self.X_BOT_PANEL = 0
		self.Y_BOT_PANEL = self.H_SCREEN - self.H_PANEL
		self.W_BOT_PANEL = self.W_SCREEN
		self.H_BOT_PANEL = self.H_PANEL

		self.X_CATEGORIES = 0
		self.Y_CATEGORIES = (self.Y_TOP_PANEL + self.H_TOP_PANEL) - 4
		# <!-- custom: shorter category column width to have more room in sevopedia; Long_Comments_py.txt #15.
		# Update: increased for upscaled text. -->
		# <!-- custom: keep categories narrower at default font, but widen when UI label font is upscaled so text doesn't clip (GPT-5.3-Codex); Long_Comments_py.txt #16. -->
		iCategoryWidth = 124
		if getSASUIFontLabel() > 3:
			iCategoryWidth = 166
		self.W_CATEGORIES = iCategoryWidth
		# <advc.004y>
		if self.bWideScreen:
			self.W_CATEGORIES = iCategoryWidth
		# </advc.004y>
		self.H_CATEGORIES = (self.Y_BOT_PANEL + 3) - self.Y_CATEGORIES

		self.X_ITEMS = self.X_CATEGORIES + self.W_CATEGORIES + 2
		self.Y_ITEMS = self.Y_CATEGORIES
		# <!-- custom: same reasoning as for/in smaller categories, did not test but should be fine with the new space was 210 -->
		self.W_ITEMS = 230
		# <advc.004y>
		if self.bWideScreen:
			self.W_ITEMS = 230
		# </advc.004y>
		self.H_ITEMS = self.H_CATEGORIES

		self.X_PEDIA_PAGE = self.X_ITEMS + self.W_ITEMS + 18
		self.Y_PEDIA_PAGE = self.Y_ITEMS + 13
		self.R_PEDIA_PAGE = self.W_SCREEN - 20
		self.B_PEDIA_PAGE = self.Y_ITEMS + self.H_ITEMS - 16
		self.W_PEDIA_PAGE = self.R_PEDIA_PAGE - self.X_PEDIA_PAGE
		self.H_PEDIA_PAGE = self.B_PEDIA_PAGE - self.Y_PEDIA_PAGE
		self.SAS_W_ITEMS_BASE = self.W_ITEMS

		self.X_TITLE = (self.W_SCREEN - 24) // 2 # advc.004y: was 500
		self.Y_TITLE = 8
		# <advc.004y>
		self.X_TOC = 45 # was 75
		Y_FOOTER_CONTROLS = self.Y_BOT_PANEL + 16 # was 730
		# </advc.004y>
		self.Y_TOC = Y_FOOTER_CONTROLS
		self.X_INDEX = self.X_TOC + 135 # advc.004y: was 210
		self.Y_INDEX = Y_FOOTER_CONTROLS
		self.X_BACK = (self.W_SCREEN - 4) // 2 # advc.004y: was 510
		self.Y_BACK = Y_FOOTER_CONTROLS
		self.X_NEXT = 133 + (self.W_SCREEN // 2) # advc.004y: was 645
		self.Y_NEXT = Y_FOOTER_CONTROLS
		# <!-- custom: place Clear near the Legend footer link, opposite Exit and visually separate from Back/Next. (GPT-5.5) -->
		self.SAS_X_CLEAR = self.X_TOC + 120
		self.SAS_Y_CLEAR = Y_FOOTER_CONTROLS
		self.SAS_X_GAME_PLAYER_ID_PREV = self.SAS_X_CLEAR + 120
		self.SAS_X_GAME_PLAYER_ID_NEXT = self.SAS_X_GAME_PLAYER_ID_PREV + 65
		self.X_EXIT = self.W_SCREEN - 30 # advc.004y: was 994
		self.Y_EXIT = Y_FOOTER_CONTROLS

		self.tab = None
		self.iActivePlayer = -1
		self.nWidgetCount = 0

		self.categoryList = []
		self.iCategory = -1
		self.iItem = -1
		self.iItemIndex = -1
		self.pediaHistory = []
		self.pediaFuture = []
		self.SAS_lastPediaJump = None

		# <!-- custom: compute once to be computationally more efficient, and added with the help of chatgpt 5.2 thanks. -->
		self.SAS_cacheCivicsTuple = None
		self.SAS_cacheTechsTuple = None
		self.SAS_cacheRegularBuildingsTuple = None
		self.SAS_cacheNationalWondersTuple = None
		self.SAS_cacheWorldWondersTuple = None
		self.SAS_cacheUnitsTuple = None
		self.SAS_cacheCorporationsTuple = None
		self.SAS_cacheCorporationHQBuildingByCorp = None
		self.SAS_cacheReligionsTuple = None
		self.SAS_cacheProjectsTuple = None
		self.SAS_cacheSpecialistsTuple = None
		self.SAS_cacheBonusesTuple = None
		self.SAS_cacheImprovementsTuple = None
		self.SAS_cacheBuildsTuple = None
		self.SAS_cacheTerrainsTuple = None
		self.SAS_cacheFeaturesTuple = None
		self.SAS_cacheMoviesTuple = None
		self.SAS_cacheMusicTuple = None
		self.SAS_cacheVotesTuple = None
		self.SAS_cacheEventTriggersTuple = None
		self.SAS_musicEraTracks = None
		self.SAS_musicLeaderTracks = None
		self.SAS_musicCivTracks = None
		self.SAS_musicScriptTracks = None
		self.SAS_musicScript3DTracks = None
		self.SAS_firstCivScript3DMusicKey = None

		# <!-- custom: search bar state variables (chatgpt 5.2 + claude opus 4.5) -->
		self.SAS_szSearchString = u""
		self.SAS_lastItemsWidget = None
		self.SAS_lastItemsInfo = None
		# <!-- custom: only one Sevopedia page renders at a time, so a single callable is enough to remember what to re-render after a search keystroke.
		# Each list page sets this when drawn: placeItems for regular listbox categories, SevoPediaIndex.placeIndex for the Index 3-column table.
		# Search handlers just invoke whichever is registered, no per-category branching. (Claude code Opus 4.7) -->
		self.SAS_activeListRefresher = None
		# <!-- custom: each searchable page also registers its UP/DOWN arrow navigator here.
		# Normal one-column pages register SAS_navigateItemList; Index registers its own table-aware navigator.
		# Avoids special-casing isIndexShowing() in handleInput. (Claude code Opus 4.7) -->
		self.SAS_activeKeyNavigator = None

		# <!-- custom: debounce for search bar to prevent double keypress even when key-up events are interleaved.
		# Note: BtS/AdvCiv can fire NOTIFY_CHARACTER twice per press for letters/digits, and the 2nd event can arrive after another key when typing fast. (chatgpt 5.2 + claude opus 4.5) -->
		self.SAS_keyDebounceByKey = {}
		self.SAS_navigationKeyDebounceByKey = {}
		self.SAS_searchKeyboardIds = []
		self.SAS_bSearchKeyboardVisible = False

		# <!-- custom: map original list indices to displayed rows when filtering so selection/highlight stays correct (chatgpt 5.2 + claude opus 4.5) -->
		self.SAS_listIdxToRow = None
		# <!-- custom: End - search bar for the left item list (chatgpt 5.2 + claude opus 4.5) -->

		# <!-- custom: cache selectable (non-header) indices for arrow navigation (chatgpt 5.2 + claude opus 4.5) -->
		self.SAS_selectableListIdx = []
		self.SAS_itemToSelectablePos = {}

		# <!-- custom: do not build sevopedia leader cache until we click on the leaders category, so that if we never open at all the leaders category, no need to compute needlessly for their cache.
		# And if we do access the leaders page, then building once the cache is enough for the entire session, no need to rebuild it even if we exit sevopedia.
		# Therefore store the cache in sevopedia leader, but add a flag to not build cache at module load of sevopedia leader, but later on click in/at placeLeaders time and from what i understand of chatgpt's explanation. -->
		self.IS_SEVOPEDIALEADER_CACHE_PREBUILT = False
		# <!-- custom: tech statistics cache for starting tech pairs/combos and untradeable techs by era tables. (Claude Opus 4.5) -->
		self.IS_TECH_STATISTICS_PREBUILT = False
		self.IS_FEATURES_PRE_LOADING_XML_DATA_VALIDATION_DONE = False

		self.pediaBuilding	= SevoPediaBuilding.SevoPediaBuilding(self)
		self.pediaLeader	= SevoPediaLeader.SevoPediaLeader(self)
		self.pediaIndex     = SevoPediaIndex.SevoPediaIndex(self)
		self.pediaMovies    = SevoPediaMovie.SevoPediaMovie(self)
		self.pediaMusic     = SevoPediaMusic.SevoPediaMusic(self)
		# <!-- custom: keep a shared handicap chart instance so its internal table cache survives between openings. (GPT-5.2-Codex) -->
		self.pediaHandicapChart = SevoPediaHandicapChart.SevoPediaHandicapChart(self)
		# <!-- custom: keep a shared game speed chart instance so its internal table cache survives between openings. (GPT-5.2-Codex) -->
		self.pediaGameSpeedChart = SevoPediaGameSpeedChart.SevoPediaGameSpeedChart(self)
		self.pediaWorldSizeChart = SevoPediaWorldSizeChart.SevoPediaWorldSizeChart(self)
		self.pediaEraChart = SevoPediaEraChart.SevoPediaEraChart(self)

		# <!-- custom: category list refactor: previously category order, list generators, screen handlers, and link keys lived in separate maps and hardcoded blocks, so reordering or adding a category required edits in multiple places and could desync labels from links; now a single SAS_CATEGORY_DEFS tuple is the source of truth.
		# This makes category edits safer (one-row change), easier to review, and less error-prone, while keeping the same runtime behavior and allowing explicit per-category icons without fallback/group logic.
		# Icons are computed once here and embedded in the tuple, so they are not global state and don't depend on later setup.
		# To add or reorder, edit SAS_CATEGORY_DEFS only; maps, list generators, and PEDIA_MAIN links update automatically. (GPT-5.2-Codex (summarized)) -->
		iconCommerceResearch = u"%c  " % (gc.getCommerceInfo(CommerceTypes.COMMERCE_RESEARCH).getChar())
		iconStrength = u"%c  " % (CyGame().getSymbolID(FontSymbols.STRENGTH_CHAR))
		iconSilverStar = u"%c  " % (CyGame().getSymbolID(FontSymbols.SILVER_STAR_CHAR))
		iconYieldProduction = u"%c  " % (gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar())
		iconYieldFood = u"%c  " % (gc.getYieldInfo(YieldTypes.YIELD_FOOD).getChar())
		iconGreatPeople = u"%c  " % (CyGame().getSymbolID(FontSymbols.GREAT_PEOPLE_CHAR))
		iconCitizen = u"%c  " % (CyGame().getSymbolID(FontSymbols.CITIZEN_CHAR))
		iconMap = u"%c  " % (CyGame().getSymbolID(FontSymbols.MAP_CHAR))
		iconCommerceCulture = u"%c  " % (gc.getCommerceInfo(CommerceTypes.COMMERCE_CULTURE).getChar())
		iconYieldCommerce = u"%c  " % (gc.getYieldInfo(YieldTypes.YIELD_COMMERCE).getChar())
		iconDefense = u"%c  " % (CyGame().getSymbolID(FontSymbols.DEFENSE_CHAR))
		iconGreatGeneral = u"%c  " % (CyGame().getSymbolID(FontSymbols.GREAT_GENERAL_CHAR))
		iconOccupation = u"%c  " % (CyGame().getSymbolID(FontSymbols.OCCUPATION_CHAR))

		# <!-- custom: central category wiring for list generators, screen handlers, and PEDIA_MAIN links.
		# To add a category, insert one row in SAS_CATEGORY_DEFS: (PEDIA_ENUM, TXT_KEY, icon, listMethodName, screenClassOrInstance, PEDIA_MAIN_LINK_KEY or None).
		# Example: (SevoScreenEnums.PEDIA_TECHS, "TXT_KEY_PEDIA_CATEGORY_TECH", iconCommerceResearch, "placeTechs", SevoPediaTech.SevoPediaTech, "PEDIA_MAIN_TECH").
		# Reorder by moving rows here; no other maps or link tables need edits. (GPT-5.2-Codex) -->
		self.SAS_CATEGORY_DEFS = (
			(SevoScreenEnums.PEDIA_INDEX, "TXT_KEY_PEDIA_SCREEN_INDEX", iconYieldCommerce, "placeIndexCategory", None, None),
			(SevoScreenEnums.PEDIA_BTS_CONCEPTS, "TXT_KEY_PEDIA_CATEGORY_CONCEPT_NEW", iconYieldCommerce, "placeBTSConcepts", SevoPediaHistory.SevoPediaHistory, None),
			(SevoScreenEnums.PEDIA_CONCEPTS, "TXT_KEY_PEDIA_CATEGORY_CONCEPT", iconYieldCommerce, "placeConcepts", SevoPediaHistory.SevoPediaHistory, "PEDIA_MAIN_CONCEPT"),
			(SevoScreenEnums.PEDIA_HINTS, "TXT_KEY_PEDIA_CATEGORY_HINTS", iconYieldCommerce, "placeHints", SevoPediaHistory.SevoPediaHistory, "PEDIA_MAIN_HINTS"),
			(SevoScreenEnums.PEDIA_SHORTCUTS, "TXT_KEY_PEDIA_CATEGORY_SHORTCUTS", iconYieldCommerce, "placeShortcuts", SevoPediaHistory.SevoPediaHistory, "PEDIA_MAIN_SHORTCUTS"),
			(SevoScreenEnums.PEDIA_MOVIES, "TXT_KEY_PEDIA_SAS_CATEGORY_MOVIES", iconCommerceCulture, "placeMovies", self.pediaMovies, None),
			(SevoScreenEnums.PEDIA_MUSIC, "TXT_KEY_PEDIA_SAS_CATEGORY_MUSIC", iconCommerceCulture, "placeMusic", self.pediaMusic, None),
			(SevoScreenEnums.PEDIA_ERA_CHART, "TXT_KEY_PEDIA_SAS_CATEGORY_ERA_CHART", iconDefense, "placeEraChart", self.pediaEraChart, None),
			(SevoScreenEnums.PEDIA_HANDICAP_CHART, "TXT_KEY_PEDIA_SAS_CATEGORY_HANDICAP_CHART", iconDefense, "placeHandicapChart", self.pediaHandicapChart, None),
			(SevoScreenEnums.PEDIA_GAME_SPEED_CHART, "TXT_KEY_PEDIA_SAS_CATEGORY_GAME_SPEED_CHART", iconDefense, "placeGameSpeedChart", self.pediaGameSpeedChart, None),
			(SevoScreenEnums.PEDIA_WORLD_SIZE_CHART, "TXT_KEY_PEDIA_SAS_CATEGORY_WORLD_SIZE_CHART", iconDefense, "placeWorldSizeChart", self.pediaWorldSizeChart, None),
			(SevoScreenEnums.PEDIA_TERRAINS, "TXT_KEY_PEDIA_CATEGORY_TERRAIN", iconMap, "placeTerrains", SevoPediaTerrain.SevoPediaTerrain, "PEDIA_MAIN_TERRAIN"),
			(SevoScreenEnums.PEDIA_FEATURES, "TXT_KEY_PEDIA_CATEGORY_FEATURE", iconMap, "placeFeatures", SevoPediaFeature.SevoPediaFeature, "PEDIA_MAIN_FEATURE"),
			(SevoScreenEnums.PEDIA_BONUSES, "TXT_KEY_PEDIA_CATEGORY_BONUS", iconMap, "placeBonuses", SevoPediaBonus.SevoPediaBonus, "PEDIA_MAIN_BONUS"),
			(SevoScreenEnums.PEDIA_IMPROVEMENTS, "TXT_KEY_PEDIA_CATEGORY_IMPROVEMENT", iconYieldFood, "placeImprovements", SevoPediaImprovement.SevoPediaImprovement, "PEDIA_MAIN_IMPROVEMENT"),
			(SevoScreenEnums.PEDIA_BUILDS, "TXT_KEY_PEDIA_CATEGORY_BUILD", iconYieldFood, "placeBuilds", SevoPediaBuild.SevoPediaBuild, None),
			(SevoScreenEnums.PEDIA_UNITS, "TXT_KEY_PEDIA_CATEGORY_UNITS", iconStrength, "placeUnits", SevoPediaUnit.SevoPediaUnit, "PEDIA_MAIN_UNIT"),
			(SevoScreenEnums.PEDIA_UNIT_UPGRADES, "TXT_KEY_PEDIA_CATEGORY_UNIT_UPGRADES", iconStrength, "placeUnitUpgrades", None, None),
			(SevoScreenEnums.PEDIA_UNIT_CATEGORIES, "TXT_KEY_PEDIA_CATEGORY_UNIT_CHART", iconSilverStar, "placeUnitCategories", SevoPediaUnitChart.SevoPediaUnitChart, "PEDIA_MAIN_UNIT_GROUP"),
			(SevoScreenEnums.PEDIA_PROMOTIONS, "TXT_KEY_PEDIA_CATEGORY_PROMOTION", iconGreatGeneral, "placePromotions", SevoPediaPromotion.SevoPediaPromotion, "PEDIA_MAIN_PROMOTION"),
			(SevoScreenEnums.PEDIA_PROMOTION_TREE, "TXT_KEY_PEDIA_CATEGORY_PROMOTION_TREE", iconGreatGeneral, "placePromotionTree", None, None),
			(SevoScreenEnums.PEDIA_SPECIALISTS, "TXT_KEY_PEDIA_CATEGORY_SPECIALIST", iconGreatPeople, "placeSpecialists", SevoPediaSpecialist.SevoPediaSpecialist, "PEDIA_MAIN_SPECIALIST"),
			(SevoScreenEnums.PEDIA_CIVS, "TXT_KEY_PEDIA_CATEGORY_CIV", iconCitizen, "placeCivs", SevoPediaCivilization.SevoPediaCivilization, "PEDIA_MAIN_CIV"),
			(SevoScreenEnums.PEDIA_LEADERS, "TXT_KEY_PEDIA_CATEGORY_LEADER", iconCitizen, "placeLeaders", self.pediaLeader, "PEDIA_MAIN_LEADER"),
			(SevoScreenEnums.PEDIA_TRAITS, "TXT_KEY_PEDIA_TRAITS", iconCitizen, "placeTraits", SevoPediaTrait.SevoPediaTrait, "PEDIA_MAIN_TRAIT"),
			(SevoScreenEnums.PEDIA_BUILDINGS, "TXT_KEY_PEDIA_CATEGORY_BUILDING", iconYieldProduction, "placeBuildings", self.pediaBuilding, "PEDIA_MAIN_BUILDING"),
			(SevoScreenEnums.PEDIA_NATIONAL_WONDERS, "TXT_KEY_PEDIA_CATEGORY_NATIONAL_WONDERS", iconYieldProduction, "placeNationalWonders", SevoPediaBuilding.SevoPediaBuilding, None),
			(SevoScreenEnums.PEDIA_WORLD_WONDERS, "TXT_KEY_PEDIA_CATEGORY_WORLD_WONDERS", iconYieldProduction, "placeWorldWonders", SevoPediaBuilding.SevoPediaBuilding, None),
			(SevoScreenEnums.PEDIA_PROJECTS, "TXT_KEY_PEDIA_CATEGORY_PROJECT", iconYieldProduction, "placeProjects", SevoPediaProject.SevoPediaProject, "PEDIA_MAIN_PROJECT"),
			(SevoScreenEnums.PEDIA_CIVICS, "TXT_KEY_PEDIA_CATEGORY_CIVIC", iconOccupation, "placeCivics", SevoPediaCivic.SevoPediaCivic, "PEDIA_MAIN_CIVIC"),
			(SevoScreenEnums.PEDIA_RELIGIONS, "TXT_KEY_PEDIA_CATEGORY_RELIGION", iconOccupation, "placeReligions", SevoPediaReligion.SevoPediaReligion, "PEDIA_MAIN_RELIGION"),
			(SevoScreenEnums.PEDIA_CORPORATIONS, "TXT_KEY_CONCEPT_CORPORATIONS", iconOccupation, "placeCorporations", SevoPediaCorporation.SevoPediaCorporation, None),
			(SevoScreenEnums.PEDIA_VOTES, "TXT_KEY_PEDIA_CATEGORY_VOTE", iconOccupation, "placeVotes", SevoPediaVote.SevoPediaVote, None),
			(SevoScreenEnums.PEDIA_EVENT_TRIGGERS, "TXT_KEY_PEDIA_SAS_CATEGORY_EVENT_TRIGGERS", iconOccupation, "placeEventTriggers", SevoPediaEventTrigger.SevoPediaEventTrigger, None),
			(SevoScreenEnums.PEDIA_TECHS, "TXT_KEY_PEDIA_CATEGORY_TECH", iconCommerceResearch, "placeTechs", SevoPediaTech.SevoPediaTech, "PEDIA_MAIN_TECH"),
		)
		self.mapListGenerators = {}
		self.mapScreenFunctions = {}
		self.SAS_mainLinkToCategory = {}
		for iEnum, szLabelKey, szIconGroup, szListMethod, screenSpec, szLinkKey in self.SAS_CATEGORY_DEFS:
			if szListMethod:
				self.mapListGenerators[iEnum] = getattr(self, szListMethod)
			if screenSpec:
				if screenSpec in (self.pediaBuilding, self.pediaLeader, self.pediaHandicapChart, self.pediaGameSpeedChart, self.pediaWorldSizeChart, self.pediaEraChart, self.pediaMovies, self.pediaMusic):
					self.mapScreenFunctions[iEnum] = screenSpec
				else:
					self.mapScreenFunctions[iEnum] = screenSpec(self)
			if szLinkKey:
				self.SAS_mainLinkToCategory[szLinkKey] = iEnum

		# <!-- custom: add highlight text for sevopedia sorting needs as we added, here fetched once for performance optimization or such -->
		self.COLOR_HIGHLIGHT_TEXT = getInfoTypeOrFail("COLOR_HIGHLIGHT_TEXT")
		self.IS_SAS_SEVOPEDIA_MAIN_CIVICS_GROUP_BY_CIVIC_TYPES = (gc.getDefineINT("SAS_SEVOPEDIA_MAIN_CIVICS_GROUP_BY_CIVIC_TYPES") > 0)
		self.IS_SAS_SEVOPEDIA_MAIN_TECHS_GROUP_BY_ERA = (gc.getDefineINT("SAS_SEVOPEDIA_MAIN_TECHS_GROUP_BY_ERA") > 0)
		self.IS_SAS_SEVOPEDIA_MAIN_BUILDINGS_GROUP_BY_ERA = (gc.getDefineINT("SAS_SEVOPEDIA_MAIN_BUILDINGS_GROUP_BY_ERA") > 0)
		self.IS_SAS_SEVOPEDIA_MAIN_UNITS_GROUP_BY_ERA = (gc.getDefineINT("SAS_SEVOPEDIA_MAIN_UNITS_GROUP_BY_ERA") > 0)
		self.IS_SAS_SEVOPEDIA_MAIN_CORPORATIONS_GROUP_BY_ERA = (gc.getDefineINT("SAS_SEVOPEDIA_MAIN_CORPORATIONS_GROUP_BY_ERA") > 0)
		self.IS_SAS_SEVOPEDIA_MAIN_RELIGIONS_GROUP_BY_ERA = (gc.getDefineINT("SAS_SEVOPEDIA_MAIN_RELIGIONS_GROUP_BY_ERA") > 0)
		self.IS_SAS_SEVOPEDIA_MAIN_PROJECTS_GROUP_BY_ERA = (gc.getDefineINT("SAS_SEVOPEDIA_MAIN_PROJECTS_GROUP_BY_ERA") > 0)
		self.SAS_SEVOPEDIA_MUSIC_ITEMS_WIDTH = gc.getDefineINT("SAS_SEVOPEDIA_MUSIC_ITEMS_WIDTH")
		self.SAS_SEVOPEDIA_LEADER_ITEMS_NO_REDUCE_MIN_WIDTH = gc.getDefineINT("SAS_SEVOPEDIA_LEADER_ITEMS_NO_REDUCE_MIN_WIDTH")
		self.SAS_SEVOPEDIA_LEADER_ITEMS_WIDTH_FONT_1 = gc.getDefineINT("SAS_SEVOPEDIA_LEADER_ITEMS_WIDTH_FONT_1")
		self.SAS_SEVOPEDIA_LEADER_ITEMS_WIDTH_FONT_2 = gc.getDefineINT("SAS_SEVOPEDIA_LEADER_ITEMS_WIDTH_FONT_2")
		self.SAS_SEVOPEDIA_LEADER_ITEMS_WIDTH_FONT_3 = gc.getDefineINT("SAS_SEVOPEDIA_LEADER_ITEMS_WIDTH_FONT_3")
		self.SAS_SEVOPEDIA_LEADER_ITEMS_WIDTH_FONT_4 = gc.getDefineINT("SAS_SEVOPEDIA_LEADER_ITEMS_WIDTH_FONT_4")
		self.IS_SAS_SEVOPEDIA_MAIN_SPECIALISTS_GROUP_BY_TYPE = (gc.getDefineINT("SAS_SEVOPEDIA_MAIN_SPECIALISTS_GROUP_BY_TYPE") > 0)
		self.IS_SAS_SEVOPEDIA_MAIN_BONUSES_GROUP_BY_IMPROVEMENT = (gc.getDefineINT("SAS_SEVOPEDIA_MAIN_BONUSES_GROUP_BY_IMPROVEMENT") > 0)
		self.IS_SAS_SEVOPEDIA_MAIN_IMPROVEMENTS_GROUP_BY_TERRAIN = (gc.getDefineINT("SAS_SEVOPEDIA_MAIN_IMPROVEMENTS_GROUP_BY_TERRAIN") > 0)
		self.IS_SAS_SEVOPEDIA_MAIN_BUILDS_GROUP_BY_TYPE = (gc.getDefineINT("SAS_SEVOPEDIA_MAIN_BUILDS_GROUP_BY_TYPE") > 0)
		self.IS_SAS_SEVOPEDIA_MAIN_TERRAINS_GROUP_BY_LAND_WATER = (gc.getDefineINT("SAS_SEVOPEDIA_MAIN_TERRAINS_GROUP_BY_LAND_WATER") > 0)
		self.IS_SAS_SEVOPEDIA_MAIN_FEATURES_GROUP_BY_LAND_WATER = (gc.getDefineINT("SAS_SEVOPEDIA_MAIN_FEATURES_GROUP_BY_LAND_WATER") > 0)
		self.IS_SAS_SEVOPEDIA_MAIN_CIVS_GROUP_BY_ARTSTYLE = (gc.getDefineINT("SAS_SEVOPEDIA_MAIN_CIVS_GROUP_BY_ARTSTYLE") > 0)
		self.IS_SAS_SEVOPEDIA_MAIN_LEADERS_GROUP_BY_CIV = (gc.getDefineINT("SAS_SEVOPEDIA_MAIN_LEADERS_GROUP_BY_CIV") > 0)
		self.IS_SAS_SEVOPEDIA_SEARCH_CLICKABLE_SPECIAL_CHARS_ENABLE = (gc.getDefineINT("SAS_SEVOPEDIA_SEARCH_CLICKABLE_SPECIAL_CHARS_ENABLE") > 0)
		self.IS_SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_ENABLE = (gc.getDefineINT("SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_ENABLE") > 0)
		self.IS_SAS_SEVOPEDIA_MUSIC_LEADER_INTRO_PEACE_FIRST_ONLY = (gc.getDefineINT("SAS_SEVOPEDIA_MUSIC_LEADER_INTRO_PEACE_FIRST_ONLY") > 0)
		self.IS_SAS_SEVOPEDIA_MUSIC_LEADER_PEACE_FIRST_ONLY = (gc.getDefineINT("SAS_SEVOPEDIA_MUSIC_LEADER_PEACE_FIRST_ONLY") > 0)
		self.IS_SAS_SEVOPEDIA_MUSIC_LEADER_INTRO_WAR_FIRST_LEADER_ONLY = (gc.getDefineINT("SAS_SEVOPEDIA_MUSIC_LEADER_INTRO_WAR_FIRST_LEADER_ONLY") > 0)
		self.IS_SAS_SEVOPEDIA_MUSIC_LEADER_WAR_FIRST_LEADER_ONLY = (gc.getDefineINT("SAS_SEVOPEDIA_MUSIC_LEADER_WAR_FIRST_LEADER_ONLY") > 0)

	# <!-- custom: search bar helper methods (chatgpt 5.2 + claude opus 4.5) -->
	def SAS_safeDeleteWidget(self, screen, szWidget):
		try:
			screen.deleteWidget(szWidget)
		except:
			pass

	# <!-- custom: search bar helper methods (chatgpt 5.2 + claude opus 4.5) -->
	def SAS_deleteSearchWidgets(self, screen):
		self.SAS_safeDeleteWidget(screen, self.SAS_SEARCH_PANEL_ID)
		self.SAS_safeDeleteWidget(screen, self.SAS_SEARCH_LABEL_ID)
		self.SAS_safeDeleteWidget(screen, self.SAS_CLEAR_SEARCH_ID)
		self.SAS_safeDeleteWidget(screen, self.SAS_SEARCH_KEYS_TOGGLE_ID)
		for szWidget in self.SAS_searchKeyboardIds:
			self.SAS_safeDeleteWidget(screen, szWidget)
		self.SAS_searchKeyboardIds = []

	# <!-- custom: clear search state for special pages that delete the item list (chatgpt 5.2 + claude opus 4.5) -->
	def SAS_prepareSpecialPageDeletingItemList(self, screen):
		# <!-- custom: search bar for the left item list (chatgpt 5.2 + claude opus 4.5) -->
		# <!-- custom: clear search state for special pages that delete the item list (chatgpt 5.2 + claude opus 4.5) -->
		self.SAS_szSearchString = u""
		self.SAS_lastItemsWidget = None
		self.SAS_lastItemsInfo = None
		self.SAS_deleteSearchWidgets(screen)
		# <!-- custom: End - search bar for the left item list (chatgpt 5.2 + claude opus 4.5) -->

		screen.deleteWidget(self.ITEM_LIST_ID)

	# <!-- custom: search bar helper methods (chatgpt 5.2 + claude opus 4.5) -->
	def SAS_isSearchActive(self):
		return (self.SAS_szSearchString is not None and len(self.SAS_szSearchString.strip()) > 0)

	# <!-- custom: invoke the registered refresher; see SAS_activeListRefresher comment in __init__. (Claude code Opus 4.7) -->
	def SAS_refreshActiveListView(self):
		if self.SAS_activeListRefresher is not None:
			self.SAS_activeListRefresher()

	def _SAS_refreshLastItems(self):
		if self.SAS_lastItemsWidget is not None:
			self.placeItems(self.SAS_lastItemsWidget, self.SAS_lastItemsInfo)

	# <!-- custom: search bar sync panel (chatgpt 5.2 + claude opus 4.5) -->
	def SAS_syncSearchPanel(self):
		screen = self.getScreen()

		# Recreate each time: simple + safe (mirrors the approach used in other mod(s)).
		self.SAS_deleteSearchWidgets(screen)

		# <!-- custom: search bar sits in the top header, X aligned with the items column.
		# Right edge stops `iSafetyToTitle` pixels short of the central "Sevopedia" title to leave room for the CLEAR button and avoid visually colliding with the title.
		# CLEAR is rendered OUTSIDE the panel as a screen-level setText widget; a setLabel inside the search panel parent does not reach handleInput in this engine, so the click never fires. (Claude code Opus 4.7) -->
		iSafetyToTitle = 200
		iClearGap = 8
		iX = self.X_ITEMS
		iH = self.SAS_SEARCH_H
		iY = self.Y_TOP_PANEL + (self.H_TOP_PANEL - iH) / 2
		iW = self.X_TITLE - iSafetyToTitle - iX

		screen.addPanel(self.SAS_SEARCH_PANEL_ID, u"", u"", True, True, iX, iY, iW, iH, PanelStyles.PANEL_STYLE_BLUE50)

		if self.SAS_isSearchActive():
			szText = self.SAS_getSearchDisplayText(self.SAS_szSearchString)
		else:
			szText = self.SAS_SEARCH_DEFAULT_TEXT

		screen.setLabel(self.SAS_SEARCH_LABEL_ID, self.SAS_SEARCH_PANEL_ID, SASTextScale.labelText(szText), CvUtil.FONT_LEFT_JUSTIFY, iX + 6, iY + 6, 0, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# Show the CLEAR button (outside the panel, screen-level) only when search is active.
		if self.SAS_isSearchActive():
			iClearX = iX + iW + iClearGap
			iClearY = self.Y_TOP_PANEL + 16
			screen.setText(self.SAS_CLEAR_SEARCH_ID, "Background", self.SAS_CLEAR_TEXT, CvUtil.FONT_LEFT_JUSTIFY, iClearX, iClearY, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		if not self.IS_SAS_SEVOPEDIA_SEARCH_CLICKABLE_SPECIAL_CHARS_ENABLE:
			self.SAS_bSearchKeyboardVisible = False
			return

		iKeysToggleX = self.X_EXIT
		iKeysToggleY = self.Y_TITLE
		screen.setText(self.SAS_SEARCH_KEYS_TOGGLE_ID, "Background", SASTextScale.titleText(self.SAS_SEARCH_KEYS_TEXT), CvUtil.FONT_RIGHT_JUSTIFY, iKeysToggleX, iKeysToggleY, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		if self.SAS_bSearchKeyboardVisible:
			self.SAS_syncSearchKeyboard(screen)

	# <!-- custom: compact click keyboard for punctuation that Civ4 keyboard events often fail to expose on non-US layouts. (GPT-5.5) -->
	def SAS_syncSearchKeyboard(self, screen):
		iX = self.X_TITLE + 100
		iAvailableW = self.X_EXIT - iX - 36
		iKeyGap = 6
		iKeyW = 24
		if iAvailableW < len(self.SAS_SEARCH_KEYBOARD_CHARS) * (iKeyW + iKeyGap):
			iKeyGap = 2
			iKeyW = max(12, iAvailableW / len(self.SAS_SEARCH_KEYBOARD_CHARS) - iKeyGap)
		iY = self.Y_TOP_PANEL + 18
		self.SAS_searchKeyboardIds = []
		for iChar, szChar in enumerate(self.SAS_SEARCH_KEYBOARD_CHARS):
			szWidget = self.SAS_SEARCH_KEY_PREFIX + str(iChar)
			self.SAS_searchKeyboardIds.append(szWidget)
			screen.setText(szWidget, "Background", SASTextScale.labelText(self.SAS_getSearchKeyDisplayText(szChar)), CvUtil.FONT_CENTER_JUSTIFY, iX + (iKeyW / 2) + iChar * (iKeyW + iKeyGap), iY, 0, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_SEARCH_KEY, iChar)

	# <!-- custom: escape renderer-sensitive search text before passing it through font-tagged UI labels.
	# Empirically, raw < or > can corrupt the rendered label (e.g. showing /font-like tag fragments), while raw & can be invisible and prevent following characters from rendering clearly.
	# These raw chars are XML/markup-sensitive and rarely useful for ordinary XML text-key search, but keep them raw in SAS_szSearchString so clicked/typed input remains faithful; escape only the display string. (GPT-5.5) -->
	def SAS_getSearchDisplayText(self, szText):
		return szText.replace(u"&", u"&amp;").replace(u"<", u"&lt;").replace(u">", u"&gt;")

	def SAS_getSearchKeyDisplayText(self, szText):
		return self.SAS_getSearchDisplayText(szText)

	def SAS_appendSearchCharacter(self, szChar):
		if len(szChar) > 0:
			self.SAS_szSearchString = self.SAS_szSearchString + szChar
			self.SAS_refreshActiveListView()
			return 1
		return 0

	# <!-- custom: convert InputTypes keyboard code to visible character, based on how other mod(s) use ScreenInput.getVisibleCharacter (chatgpt 5.2 + claude opus 4.5) -->
	def SAS_getSearchCharacterKeys(self):
		return (int(InputTypes.KB_MINUS), int(InputTypes.KB_EQUALS), int(InputTypes.KB_LBRACKET), int(InputTypes.KB_RBRACKET), int(InputTypes.KB_SEMICOLON), int(InputTypes.KB_APOSTROPHE), int(InputTypes.KB_GRAVE), int(InputTypes.KB_BACKSLASH), int(InputTypes.KB_COMMA), int(InputTypes.KB_PERIOD), int(InputTypes.KB_SLASH), int(InputTypes.KB_NUMPADSTAR), int(InputTypes.KB_NUMPADMINUS), int(InputTypes.KB_NUMPADPLUS), int(InputTypes.KB_NUMPADPERIOD), int(InputTypes.KB_NUMPADEQUALS), int(InputTypes.KB_AT), int(InputTypes.KB_UNDERLINE), int(InputTypes.KB_COLON), int(InputTypes.KB_NUMPADCOMMA), int(InputTypes.KB_NUMPADSLASH))

	# <!-- custom: identify keys that need debounce in the search (chatgpt 5.2 + claude opus 4.5) -->
	# <!-- custom: debounce alnum and editing/navigation keys for search because BtS can fire duplicate NOTIFY_CHARACTER events on list widgets; this includes Space so queries like "la t" don't become "la t". (GPT-5.3-Codex) -->
	def SAS_shouldDebounceKey(self, iKey):
		# Letters A-Z
		if iKey >= int(InputTypes.KB_A) and iKey <= int(InputTypes.KB_Z):
			return True
		# Digits 0-9
		if iKey >= int(InputTypes.KB_0) and iKey <= int(InputTypes.KB_9):
			return True
		# Space
		if iKey == int(InputTypes.KB_SPACE):
			return True
		if iKey in self.SAS_getSearchCharacterKeys():
			return True
		# Backspace (and optionally Delete) can also double-fire. No need for KB_DELETE or KB_RETURN since: Delete has nothing to do (no cursor position), Enter exits Sevopedia (default game behavior, not our code)
		if iKey == int(InputTypes.KB_BACKSPACE):
			return True

		# <!-- custom: Based on C2C mod's implementation thanks: add navigation of the item list with the UP/DOWN arrow keys.
		# Code adjusted for AdvCiv-SAS with the help of chatgpt 5.2 and claude opus 4.5. -->
		if iKey == int(InputTypes.KB_UP) or iKey == int(InputTypes.KB_DOWN):
			return True
		# <!-- custom: End - Based on C2C mod's implementation thanks: add navigation of the item list with the UP/DOWN arrow keys.
		# Code adjusted for AdvCiv-SAS with the help of chatgpt 5.2 and claude opus 4.5. -->
		# <!-- custom: Left/Right are handled earlier in handleInput with SAS_navigationKeyDebounceByKey, so they stay separate from the search/list debounce state that category redraws reset. (Claude code Opus 4.7 + GPT-5.5) -->

		return False

	def SAS_shouldIgnoreDebouncedKey(self, iKey):
		if self.SAS_shouldDebounceKey(iKey):
			if self.SAS_keyDebounceByKey.get(iKey, 0):
				self.SAS_keyDebounceByKey[iKey] = 0
				return True
			self.SAS_keyDebounceByKey[iKey] = 1
		return False

	def SAS_shouldIgnoreDebouncedNavigationKey(self, iKey):
		if self.SAS_navigationKeyDebounceByKey.get(iKey, 0):
			self.SAS_navigationKeyDebounceByKey[iKey] = 0
			return True
		self.SAS_navigationKeyDebounceByKey[iKey] = 1
		return False

	def SAS_getVisibleCharacter(self, inputClass):
		iKey = inputClass.getData()
		bShift = inputClass.isShiftKeyDown()

		# Letters A-Z
		if iKey >= int(InputTypes.KB_A) and iKey <= int(InputTypes.KB_Z):
			iLetterOffset = iKey - int(InputTypes.KB_A)
			if bShift:
				return unichr(65 + iLetterOffset)  # Uppercase A-Z
			else:
				return unichr(97 + iLetterOffset)  # Lowercase a-z

		# Numbers 0-9
		if iKey >= int(InputTypes.KB_0) and iKey <= int(InputTypes.KB_9):
			iNumberOffset = iKey - int(InputTypes.KB_0)
			if bShift:
				return (u")", u"!", u"@", u"#", u"$", u"%", u"^", u"&", u"*", u"(")[iNumberOffset]
			return unichr(48 + iNumberOffset)

		# Space
		if iKey == int(InputTypes.KB_SPACE):
			return u" "

		# <!-- custom: Direct punctuation keys are Civ4/DirectInput physical-key names; on some keyboard layouts the engine does not expose the intended typed character, so the click keyboard above remains the reliable fallback.
		# Prefer this low-risk fallback over DLL/input-layer plumbing because punctuation searches are rare in Civ4 item names, and many markup-sensitive chars are unlikely to appear raw in XML text anyway. (GPT-5.5) -->
		if iKey == int(InputTypes.KB_MINUS):
			if bShift:
				return u"_"
			return u"-"
		if iKey == int(InputTypes.KB_EQUALS):
			if bShift:
				return u"+"
			return u"="
		if iKey == int(InputTypes.KB_LBRACKET):
			if bShift:
				return u"{"
			return u"["
		if iKey == int(InputTypes.KB_RBRACKET):
			if bShift:
				return u"}"
			return u"]"
		if iKey == int(InputTypes.KB_SEMICOLON):
			if bShift:
				return u":"
			return u";"
		if iKey == int(InputTypes.KB_APOSTROPHE):
			if bShift:
				return u"\""
			return u"'"
		if iKey == int(InputTypes.KB_GRAVE):
			if bShift:
				return u"~"
			return u"`"
		if iKey == int(InputTypes.KB_BACKSLASH):
			if bShift:
				return u"|"
			return u"\\"
		if iKey == int(InputTypes.KB_COMMA):
			if bShift:
				return u"<"
			return u","
		if iKey == int(InputTypes.KB_PERIOD):
			if bShift:
				return u">"
			return u"."
		if iKey == int(InputTypes.KB_SLASH):
			if bShift:
				return u"?"
			return u"/"
		if iKey == int(InputTypes.KB_AT):
			return u"@"
		if iKey == int(InputTypes.KB_UNDERLINE):
			return u"_"
		if iKey == int(InputTypes.KB_COLON):
			return u":"
		if iKey == int(InputTypes.KB_NUMPADSTAR):
			return u"*"
		if iKey == int(InputTypes.KB_NUMPADMINUS):
			return u"-"
		if iKey == int(InputTypes.KB_NUMPADPLUS):
			return u"+"
		if iKey == int(InputTypes.KB_NUMPADPERIOD):
			return u"."
		if iKey == int(InputTypes.KB_NUMPADEQUALS):
			return u"="
		if iKey == int(InputTypes.KB_NUMPADCOMMA):
			return u","
		if iKey == int(InputTypes.KB_NUMPADSLASH):
			return u"/"

		# No visible character for this key
		return u""
	# <!-- custom: End - search bar for the left item list (chatgpt 5.2 + claude opus 4.5) -->

	# <!-- custom: Based on C2C mod's implementation thanks: add navigation of the item list with the UP/DOWN arrow keys.
	# Code adjusted for AdvCiv-SAS with the help of chatgpt 5.2 and claude opus 4.5. -->
	# Navigate the item list by iDirection rows (-1 = up, +1 = down). Skips headers/spacers (item[1] == -1) and respects filtering.
	def SAS_navigateItemList(self, iDirection):
		if not self.isContentsShowing():
			return False
		# <!-- custom: Index uses the shared search refresher but not the normal one-column item list; UP/DOWN item-list navigation applies only to pages drawn by placeItems. (GPT-5.5 + Claude code Opus 4.7) -->
		if self.SAS_activeListRefresher != self._SAS_refreshLastItems:
			return False
		if not self.list or len(self.list) == 0:
			return False

		# Prefer cached selectable indices (built in placeItems)
		listSelectableRows = getattr(self, "SAS_selectableListIdx", None)
		if not listSelectableRows:
			# Fallback: old behavior (build by scanning)
			listSelectableRows = []
			for idx, item in enumerate(self.list):
				if item[1] == -1:
					continue
				if self.SAS_isSearchActive() and getattr(self, "SAS_listIdxToRow", None) is not None:
					if not self.SAS_listIdxToRow.has_key(idx):
						continue
				listSelectableRows.append(idx)

		if len(listSelectableRows) == 0:
			return False

		# O(1) lookup of current position if possible
		iCurrentPos = -1
		dPos = getattr(self, "SAS_itemToSelectablePos", None)
		if dPos is not None:
			iCurrentPos = dPos.get(self.iItem, -1)

		# Calculate new position
		if iCurrentPos == -1:
			# No item selected, select first or last based on direction
			if iDirection > 0:
				iNewPos = 0
			else:
				iNewPos = len(listSelectableRows) - 1
		else:
			iNewPos = iCurrentPos + iDirection
			# Clamp to valid range
			if iNewPos < 0:
				iNewPos = 0
			elif iNewPos >= len(listSelectableRows):
				iNewPos = len(listSelectableRows) - 1

		# Get the item at new position and jump to it
		iNewListIdx = listSelectableRows[iNewPos]
		iNewItem = self.list[iNewListIdx][1]

		if iNewItem != self.iItem:
			self.pediaJump(self.iCategory, iNewItem, False, False)
			return True

		return False
	# <!-- custom: End - Based on C2C mod's implementation thanks: add navigation of the item list with the UP/DOWN arrow keys.
	# Code adjusted for AdvCiv-SAS with the help of chatgpt 5.2 and claude opus 4.5. -->

	def getScreen(self):
		return CyGInterfaceScreen(self.PEDIA_MAIN_SCREEN, SevoScreenEnums.PEDIA_MAIN)

	def createScreen(self, screen):
		if screen.isActive():
			return
		BugUtil.debug("Creating screen")
		self.iCategory = -1
		self.tab = None
		self.setPediaCommonWidgets()

	def pediaShow(self):
		SASDefineGuard.verify_or_raise("SevoPediaMain.pediaShow")
		global g_TraitUtilInitDone
		if not g_TraitUtilInitDone:
			TraitUtil.init()
			g_TraitUtilInitDone = True
		self.iActivePlayer = gc.getGame().getActivePlayer()
		self.iCategory = -1
		if self.SAS_lastPediaJump is not None:
			current = self.SAS_lastPediaJump
		elif self.pediaHistory:
			current = self.pediaHistory[-1]
		else:
			# <!-- custom: default to the first row in SAS_CATEGORY_DEFS, so the opening category always matches the current category order instead of being hardcoded to Techs. (GPT-5.2-Codex) -->
			current = (SevoScreenEnums.PEDIA_MAIN, self.SAS_CATEGORY_DEFS[0][0])
		# <!-- custom: keep Back/Next history across closing and reopening Sevopedia during the same game session.
		# This lets players inspect several entries (e.g. Leader AIP pages), exit to the map, reopen Sevopedia, and still use Back/Next instead of rebuilding the same navigation chain. See KI#125. (GPT-5.5) -->
		self.pediaJump(current[0], current[1], False, True)

	def pediaJump(self, iCategory, iItem, bRemoveFwdList, bIsLink):
		# <!-- custom: note: fixed a (seemingly base advciv) bug in in CvDLLWidgetData.cpp where iItem was -1 for obsolete bonuses redirecting from tech advisor, unlike obsolete buildings which didn't have the issue weirdly/strangely, with chatgpt's help and thanks to my prompt too and observation of the issue and or such but also chatgpt's help in guiding me bit too; i had put a workaround here to use a placeholder for iItem but no needed anymore now that this is fixed so reverted everything as base advciv code was minus this extra code comment, see also code comment at WIDGET_HELP_BONUS_REVEAL in CvDLLWidgetData.cpp or known issue number 22 as of now in known issues of advciv-sas readme for details as well. -->
		bAddToHistory = False
		if (not self.pediaHistory):
			bAddToHistory = True
		else:
			prev = self.pediaHistory[-1]
			if (prev[0] != iCategory or prev[1] != iItem):
				bAddToHistory = True
		if (bAddToHistory):
			self.pediaHistory.append((iCategory, iItem))
		if (bRemoveFwdList):
			self.pediaFuture = []
		iPrevCategory = self.iCategory
		iPrevItem = self.iItem

		screen = self.getScreen()
		if not screen.isActive():
			self.createScreen(screen)

		# <!-- custom: fix Sevopedia category-open UX: many categories land on a blank header/spacer page first (item id -1), which slows navigation; auto-jump to first real item on fresh category clicks only.
		# Also avoid auto-jump for no-item categories. See KI#119. (GPT-5.3-Codex) -->
		if (iCategory == SevoScreenEnums.PEDIA_MAIN):
			BugUtil.debug("Main link %d" % iItem)
			self.SAS_lastPediaJump = (iCategory, iItem)
			self.showContents(bIsLink, iItem)
			iListIndex = self.SAS_categoryEnumToIndex.get(iItem, -1)
			if iListIndex != -1:
				screen.setSelectedListBoxStringGFC(self.CATEGORY_LIST_ID, iListIndex)
			if bRemoveFwdList and self.SAS_lastItemsWidget is not None:
				iFirstItem = self.SAS_getFirstSelectableItemIdFromCurrentList()
				if iFirstItem != -1:
					# <!-- custom: drop the transient category node from history when auto-redirecting to the first real item; this avoids needless BACK/NEXT steps that pollute pedia navigation.
					# Keep category history when no redirect occurs (e.g., no-item category). See KI#119. (GPT-5.3-Codex) -->
					if self.pediaHistory and self.pediaHistory[-1] == (SevoScreenEnums.PEDIA_MAIN, iItem):
						self.pediaHistory.pop()
					return self.pediaJump(iItem, iFirstItem, bRemoveFwdList, False)
			return

		if (iCategory == SevoScreenEnums.PEDIA_BUILDINGS):
			iCategory += self.getBuildingType(iItem)
		elif (iCategory == SevoScreenEnums.PEDIA_BTS_CONCEPTS):
			iCategory = self.determineNewConceptSubCategory(iItem)
			BugUtil.debug("Switching to category %d" % iCategory)
		self.SAS_lastPediaJump = (iCategory, iItem)
		self.showContents(bIsLink, iCategory)
		iListIndex = self.SAS_categoryEnumToIndex.get(iCategory, -1)
		if iListIndex != -1:
			screen.setSelectedListBoxStringGFC(self.CATEGORY_LIST_ID, iListIndex)
		if (iCategory not in (SevoScreenEnums.PEDIA_UNIT_UPGRADES, SevoScreenEnums.PEDIA_PROMOTION_TREE, SevoScreenEnums.PEDIA_HINTS)):
			screen.enableSelect(self.ITEM_LIST_ID, True)
			if (self.iItemIndex != -1):
				BugUtil.debug("Deselecting item %d" % self.iItemIndex)
				screen.selectRow(self.ITEM_LIST_ID, self.iItemIndex, False)
			BugUtil.debug("Selecting item %d" % iItem)
			self.iItem = iItem
			for i, item in enumerate(self.list):
				if (item[1] == iItem):
					# <!-- custom: when filtering, select the displayed row, not the original list index (chatgpt 5.2 + claude opus 4.5) -->
					iRowToSelect = i
					if self.SAS_isSearchActive() and (self.SAS_listIdxToRow is not None):
						iRowToSelect = self.SAS_listIdxToRow.get(i, i)
					BugUtil.debug("Selecting %dth item %d (row %d)" % (i, iItem, iRowToSelect))
					#screen.setSelectedListBoxStringGFC(self.ITEM_LIST_ID, iRowToSelect)
					screen.selectRow(self.ITEM_LIST_ID, iRowToSelect, True)
					self.iItemIndex = iRowToSelect
					#break
					# <!-- custom: End - when filtering, select the displayed row, not the original list index (chatgpt 5.2 + claude opus 4.5) -->

		# <!-- custom: when switching items inside the same category, preserve active expanded overlays (history/content) by reopening them after the new item is drawn.
		# This keeps the user in expanded mode while browsing via the item list. (GPT-5.3-Codex) -->
		bKeepHistoryExpanded = False
		bKeepContentExpanded = False
		if iPrevCategory == iCategory and iPrevItem != -1 and iItem != iPrevItem:
			pediaScreenCurrent = self.mapScreenFunctions.get(iCategory)
			if pediaScreenCurrent is not None:
				bKeepHistoryExpanded = bool(getattr(pediaScreenCurrent, "bHistoryExpanded", False))
				bKeepContentExpanded = bool(getattr(pediaScreenCurrent, "bContentExpanded", False))

		#self.iCategory = iCategory
		BugUtil.debug("Drawing screen %d item %d" % (iCategory, iItem))
		self.deleteAllWidgets()
		func = self.mapScreenFunctions.get(iCategory)
		func.interfaceScreen(iItem)
		if bKeepContentExpanded and hasattr(func, "setContentExpanded"):
			func.setContentExpanded(True)
			# <!-- custom: clear first-pass (collapsed) widgets before re-rendering expanded, otherwise artifacts from the normal layout can remain visible under/around the overlay. (GPT-5.3-Codex) -->
			self.deleteAllWidgets()
			func.interfaceScreen(iItem)
		elif bKeepHistoryExpanded and hasattr(func, "setHistoryExpanded"):
			func.setHistoryExpanded(True)
			# <!-- custom: same cleanup for history-expanded re-render; prevents leftover page controls (e.g. AI panel/expand button) when switching items while expanded. (GPT-5.3-Codex) -->
			self.deleteAllWidgets()
			func.interfaceScreen(iItem)
		self.SAS_setFooterNavigationTexts(screen, iCategory)
		# <!-- custom: fix first-visit Sevopedia category keyboard navigation within the same open Sevopedia session: the first click on Specialists left UP/DOWN inactive, but after clicking Bonuses and then Specialists again, UP/DOWN worked in Specialists.
		# Tested: first Specialists open now has working UP/DOWN.
		# Additional sanity checks, not confirmed original failures: first direct open to expanded panels works, and first direct open to the Leaders category animation panel works. (GPT-5.5) -->
		self.SAS_refocusItemListForKeyboardNavigation()

	def determineNewConceptSubCategory(self, iItem):
		info = gc.getNewConceptInfo(iItem)
		BugUtil.debug("NewConcept item %d is %s" % (iItem, info.getDescription()))
		# <!-- custom: Removed isTraitInfo check - CONCEPT_TRAIT_* entries deleted from XML since Traits now use WIDGET_PYTHON routing. (Claude Opus 4.5 / GPT-5.5) -->
		if (self.isShortcutInfo(info)):
			return SevoScreenEnums.PEDIA_SHORTCUTS
		return SevoScreenEnums.PEDIA_BTS_CONCEPTS

	def SAS_getFirstSelectableItemIdFromCurrentList(self):
		if not hasattr(self, "list"):
			return -1
		for item in self.list:
			if len(item) > 1 and item[1] != -1:
				return item[1]
		return -1

	def SAS_refocusItemListForKeyboardNavigation(self):
		if self.SAS_lastItemsWidget is None:
			return
		try:
			self.getScreen().setFocus(self.ITEM_LIST_ID)
		except:
			pass

	def isContentsShowing(self):
		return self.tab == self.TAB_TOC

	def showContents(self, bForce=False, iCategory=None):
		if iCategory is None:
			iCategory = self.SAS_CATEGORY_DEFS[0][0]
		self.deleteAllWidgets()
		# <!-- custom: keep Sevopedia layout as restart-based for resolution changes; main interface still needs restart after in-session resolution switches to avoid broken layout, so we don't add dynamic per-resolution reflow across Sevopedia screens. (GPT-5.3-Codex) -->
		screen = self.getScreen()
		if iCategory == SevoScreenEnums.PEDIA_MUSIC:
			iMusicItemsWidth = self.SAS_SEVOPEDIA_MUSIC_ITEMS_WIDTH
			if iMusicItemsWidth <= 0:
				iMusicItemsWidth = self.SAS_W_ITEMS_BASE
			self.SAS_setItemsWidth(iMusicItemsWidth)
		elif iCategory == SevoScreenEnums.PEDIA_LEADERS:
			self.SAS_setItemsWidth(self.SAS_getLeaderItemsWidthByCurrentUIFont())
		else:
			self.SAS_setItemsWidth(self.SAS_W_ITEMS_BASE)
		self.SAS_setFooterNavigationTexts(screen, iCategory)
		if not self.isContentsShowing():
			BugUtil.debug("Drawing category list")
			self.placeCategories(iCategory)
			if not self.SAS_USE_BOTTOM_TABS:
				self.SAS_safeDeleteWidget(screen, self.TOC_ID)
				self.SAS_safeDeleteWidget(screen, self.INDEX_ID)
			if self.SAS_USE_BOTTOM_TABS:
				screen.setText(self.TOC_ID, "Background", self.TOC_ACTIVE_TEXT,   CvUtil.FONT_LEFT_JUSTIFY,   self.X_TOC,   self.Y_TOC,   0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL,      -1, -1)
				screen.setText(self.INDEX_ID, "Background", self.INDEX_TEXT, CvUtil.FONT_LEFT_JUSTIFY, self.X_INDEX, self.Y_INDEX, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL,      -1, -1)
			screen.show(self.BACK_ID)
			screen.show(self.NEXT_ID)
			screen.show(self.SAS_CLEAR_ID)

		if not self.isContentsShowing() or self.iCategory != iCategory or bForce:
			BugUtil.debug("Drawing item list %d" % iCategory)
			# <!-- custom: search bar for the left item list (chatgpt 5.2 + claude opus 4.5) -->
			# <!-- custom: reset search state on every category change.
			# Players almost always want to look up something different on the next visit, so we drop the query rather than restoring it. (Claude code Opus 4.7) -->
			# <!-- custom: reset search state when changing category (chatgpt 5.2 + claude opus 4.5) -->
			self.SAS_lastItemsWidget = None
			self.SAS_lastItemsInfo = None
			self.SAS_activeListRefresher = None
			self.SAS_activeKeyNavigator = None
			if (not self.isContentsShowing()) or (self.iCategory != iCategory):
				self.SAS_szSearchString = u""
				# <!-- custom: reset debounce state when search is reset (chatgpt 5.2 + claude opus 4.5) -->
				self.SAS_keyDebounceByKey = {}
			# <!-- custom: End - search bar for the left item list (chatgpt 5.2 + claude opus 4.5) -->
			self.mapListGenerators.get(iCategory)()
			self.iCategory = iCategory
			self.iItem = -1
			self.iItemIndex = -1

			# <!-- custom: force keyboard focus so UP/DOWN works without requiring a click (chatgpt 5.2 + claude opus 4.5) -->
			try:
				# <!-- custom: after refactoring showContents, `screen` is initialized once in the outer method scope and reused here; keep this old line commented for traceability in case this focus block is revisited. (GPT-5.3-Codex) -->
				# screen = self.getScreen()
				# Focus the left list when present
				if self.SAS_lastItemsWidget is not None:
					try:
						screen.setFocus(self.ITEM_LIST_ID)
					except:
						pass
			except:
				pass
		self.tab = self.TAB_TOC

	def isIndexShowing(self):
		return self.tab == self.TAB_INDEX

	def showIndex(self):
		if not self.SAS_USE_BOTTOM_TABS:
			return
		if self.isIndexShowing():
			return
		self.deleteAllWidgets()
		self.deleteListWidgets()
		screen = self.getScreen()
		screen.setText(self.TOC_ID, "Background", self.TOC_TEXT,   CvUtil.FONT_LEFT_JUSTIFY,   self.X_TOC,   self.Y_TOC,   0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL,      -1, -1)
		screen.setText(self.INDEX_ID, "Background", self.INDEX_ACTIVE_TEXT, CvUtil.FONT_LEFT_JUSTIFY, self.X_INDEX, self.Y_INDEX, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL,      -1, -1)
		screen.hide(self.BACK_ID)
		screen.hide(self.NEXT_ID)
		screen.hide(self.SAS_CLEAR_ID)
		self.pediaIndex.interfaceScreen()
		self.tab = self.TAB_INDEX

	def placeIndexCategory(self):
		screen = self.getScreen()
		self.SAS_lastItemsWidget = None
		self.SAS_lastItemsInfo = None
		self.SAS_activeListRefresher = None
		self.SAS_activeKeyNavigator = None
		self.SAS_szSearchString = u""
		self.SAS_keyDebounceByKey = {}
		self.SAS_deleteSearchWidgets(screen)
		self.SAS_safeDeleteWidget(screen, self.ITEM_LIST_ID)
		self.pediaIndex.interfaceScreen(True)

	def setPediaCommonWidgets(self):
		# advc.004y: was TXT_KEY_SEVOPEDIA_TITLE
		self.HEAD_TEXT = sasFontTagTitle.bold + localText.getText("TXT_KEY_CIVILOPEDIA_TITLE",      ())         + SAS_FONT_TAG_CLOSE
		self.BACK_TEXT = SASTextScale.titleText(localText.getText("TXT_KEY_PEDIA_SCREEN_BACK",    ()).upper())
		self.NEXT_TEXT = SASTextScale.titleText(localText.getText("TXT_KEY_PEDIA_SCREEN_FORWARD", ()).upper())
		self.SAS_CLEAR_TEXT = SASTextScale.titleText(localText.getText("TXT_KEY_PEDIA_SAS_CLEAR", ()).upper())
		self.EXIT_TEXT = SASTextScale.titleText(localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT",    ()).upper())
		# <!-- custom: build grey labels for global footer controls that stay visible but may be inactive.
		# Page-specific Legend links still appear only when applicable: no Legend means no page legend, not an inactive global command. See KI#126. (GPT-5.5) -->
		eLightGrey = getInfoTypeOrFail("COLOR_LIGHT_GREY")
		self.SAS_eFooterDisabledColor = eLightGrey
		self.BACK_TEXT_DISABLED = SASTextScale.titleText(localText.changeTextColor(localText.getText("TXT_KEY_PEDIA_SCREEN_BACK", ()).upper(), eLightGrey))
		self.NEXT_TEXT_DISABLED = SASTextScale.titleText(localText.changeTextColor(localText.getText("TXT_KEY_PEDIA_SCREEN_FORWARD", ()).upper(), eLightGrey))
		self.SAS_CLEAR_TEXT_DISABLED = SASTextScale.titleText(localText.changeTextColor(localText.getText("TXT_KEY_PEDIA_SAS_CLEAR", ()).upper(), eLightGrey))
		# <!-- custom: use bracket-direction labels for player-id arrows instead of "<-" / "->"; empirically, literal angle brackets in this footer text path render as broken markup such as "/color" and can corrupt the control text. (GPT-5.5) -->
		self.SAS_GAME_PLAYER_ID_PREV_TEXT_DISABLED = SASTextScale.titleText(localText.changeTextColor(localText.getText("TXT_KEY_PEDIA_SAS_GAME_PLAYER_ID_PREV", (-1,)), eLightGrey))
		self.SAS_GAME_PLAYER_ID_NEXT_TEXT_DISABLED = SASTextScale.titleText(localText.changeTextColor(localText.getText("TXT_KEY_PEDIA_SAS_GAME_PLAYER_ID_NEXT", (-1,)), eLightGrey))

		self.TOC_TEXT = SASTextScale.titleText(localText.getText("TXT_KEY_PEDIA_SCREEN_CONTENTS", ()).upper())
		self.INDEX_TEXT = SASTextScale.titleText(localText.getText("TXT_KEY_PEDIA_SCREEN_INDEX",  ()).upper())
		eYellow = getInfoTypeOrFail("COLOR_YELLOW")
		self.TOC_ACTIVE_TEXT = SASTextScale.titleText(localText.getColorText("TXT_KEY_PEDIA_SCREEN_CONTENTS", (), eYellow).upper())
		self.INDEX_ACTIVE_TEXT = SASTextScale.titleText(localText.getColorText("TXT_KEY_PEDIA_SCREEN_INDEX",  (), eYellow).upper())

		# These are terrain TYPES that should be classified under the "GraphicalOnly (High)" header rather than "Land". This is purely a UI grouping choice.
		self.SAS_SEVOPEDIA_TERRAIN_GRAPHICAL_ONLY_HIGH_TYPES = ("TERRAIN_HILL", "TERRAIN_PEAK")
		self.SAS_SEVOPEDIA_TERRAIN_GRAPHICAL_ONLY_HIGH_IDS = []
		for szType in self.SAS_SEVOPEDIA_TERRAIN_GRAPHICAL_ONLY_HIGH_TYPES:
			iT = getInfoTypeOrFail(szType)
			if iT >= 0:
				self.SAS_SEVOPEDIA_TERRAIN_GRAPHICAL_ONLY_HIGH_IDS.append(iT)

		self.categoryList = []
		self.SAS_categoryEnumToIndex = {}
		for iEnum, szLabelKey, szIcon, szListMethod, screenSpec, szLinkKey in self.SAS_CATEGORY_DEFS:
			self.categoryList.append((szIcon, localText.getText(szLabelKey, ()), iEnum))
		for i, category in enumerate(self.categoryList):
			self.SAS_categoryEnumToIndex[category[2]] = i

		self.SAS_linkMatchDefs = (
			("getNumTechInfos", "getTechInfo", SevoScreenEnums.PEDIA_TECHS),
			("getNumUnitInfos", "getUnitInfo", SevoScreenEnums.PEDIA_UNITS),
			("getNumUnitCombatInfos", "getUnitCombatInfo", SevoScreenEnums.PEDIA_UNIT_CATEGORIES),
			("getNumPromotionInfos", "getPromotionInfo", SevoScreenEnums.PEDIA_PROMOTIONS),
			("getNumBuildingInfos", "getBuildingInfo", SevoScreenEnums.PEDIA_BUILDINGS),
			("getNumProjectInfos", "getProjectInfo", SevoScreenEnums.PEDIA_PROJECTS),
			("getNumSpecialistInfos", "getSpecialistInfo", SevoScreenEnums.PEDIA_SPECIALISTS),
			("getNumTerrainInfos", "getTerrainInfo", SevoScreenEnums.PEDIA_TERRAINS),
			("getNumFeatureInfos", "getFeatureInfo", SevoScreenEnums.PEDIA_FEATURES),
			("getNumBonusInfos", "getBonusInfo", SevoScreenEnums.PEDIA_BONUSES),
			("getNumImprovementInfos", "getImprovementInfo", SevoScreenEnums.PEDIA_IMPROVEMENTS),
			("getNumBuildInfos", "getBuildInfo", SevoScreenEnums.PEDIA_BUILDS),
			("getNumCivilizationInfos", "getCivilizationInfo", SevoScreenEnums.PEDIA_CIVS),
			("getNumLeaderHeadInfos", "getLeaderHeadInfo", SevoScreenEnums.PEDIA_LEADERS),
			# <!-- custom: Add traits to link matching so clicking <link=literal>TraitName</link> in Sevopedia Leader (or anywhere else) navigates to the Sevopedia Traits page.
			# The DLL wraps trait names in <link=literal> tags (see CvGameTextMgr.cpp parseTraits), and this entry tells the link() method to search TraitInfo for a match and jump to PEDIA_TRAITS.
			# Without this, clicking trait links had no effect. (Claude Opus 4.5) -->
			("getNumTraitInfos", "getTraitInfo", SevoScreenEnums.PEDIA_TRAITS),
			("getNumCivicInfos", "getCivicInfo", SevoScreenEnums.PEDIA_CIVICS),
			("getNumReligionInfos", "getReligionInfo", SevoScreenEnums.PEDIA_RELIGIONS),
			("getNumCorporationInfos", "getCorporationInfo", SevoScreenEnums.PEDIA_CORPORATIONS),
			("getNumConceptInfos", "getConceptInfo", SevoScreenEnums.PEDIA_CONCEPTS),
			("getNumNewConceptInfos", "getNewConceptInfo", SevoScreenEnums.PEDIA_BTS_CONCEPTS),
		)
		screen = self.getScreen()
		screen.setRenderInterfaceOnly(True)
		screen.setScreenGroup(1)
		screen.showScreen(PopupStates.POPUPSTATE_IMMEDIATE, False)
		# advc.001: Was getInterfaceArtInfo("SCREEN_BG_OPAQUE"), meaning that self.INTERFACE_ART_INFO was ignored. The original Civilopedia has the same bug fwiw.
		screen.addDDSGFC(self.BACKGROUND_ID, ArtFileMgr.getInterfaceArtInfo(self.INTERFACE_ART_INFO).getPath(), 0, 0, self.W_SCREEN, self.H_SCREEN, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.addPanel(self.TOP_PANEL_ID, u"", u"", True, False, self.X_TOP_PANEL, self.Y_TOP_PANEL, self.W_TOP_PANEL, self.H_TOP_PANEL, PanelStyles.PANEL_STYLE_TOPBAR)
		screen.addPanel(self.BOT_PANEL_ID, u"", u"", True, False, self.X_BOT_PANEL, self.Y_BOT_PANEL, self.W_BOT_PANEL, self.H_BOT_PANEL, PanelStyles.PANEL_STYLE_BOTTOMBAR)
		# <advc.004y>
		X_SCREEN = self.HORIZONTAL_MARGIN
		Y_SCREEN = self.BOTTOM_MARGIN
		if not self.bWideScreen:
			X_SCREEN = screen.centerX(0)
		if not self.bFullScreen:
			Y_SCREEN = screen.centerY(0)
		# </advc.004y>
		screen.setDimensions(X_SCREEN, Y_SCREEN, self.W_SCREEN, self.H_SCREEN)

		screen.setText(self.HEAD_ID, "Background", self.HEAD_TEXT, CvUtil.FONT_CENTER_JUSTIFY, self.X_TITLE, self.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL,      -1, -1)
		# <!-- custom: Note: removed generic footer setText from this common-widgets block because it became redundant/no longer relevant here; footer link placement is now handled in showContents where category-specific behavior (Leader AIP overflow exception) is applied. (GPT-5.3-Codex) -->

	def SAS_getFooterLegendX(self):
		return self.X_TOC - 10

	def SAS_setFooterNavigationTexts(self, screen, iCategory):
		bCanBack = (len(self.pediaHistory) > 1)
		bCanForward = (len(self.pediaFuture) > 0)
		bCanClear = (bCanBack or bCanForward)
		if bCanBack:
			szBackText = self.BACK_TEXT
		else:
			szBackText = self.BACK_TEXT_DISABLED
		if bCanForward:
			szNextText = self.NEXT_TEXT
		else:
			szNextText = self.NEXT_TEXT_DISABLED
		if bCanClear:
			szClearText = self.SAS_CLEAR_TEXT
		else:
			szClearText = self.SAS_CLEAR_TEXT_DISABLED
		if iCategory == SevoScreenEnums.PEDIA_LEADERS:
			iClearX, iPrevPlayerX, iNextPlayerX, iBackX, iNextX, iExitX = self.SAS_getLeaderFooterNavigationPositions()
			screen.setText(self.SAS_CLEAR_ID, "Background", szClearText, CvUtil.FONT_CENTER_JUSTIFY, iClearX, self.SAS_Y_CLEAR, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			self.SAS_setGamePlayerIdNavigationTexts(screen, iCategory, iPrevPlayerX, iNextPlayerX, CvUtil.FONT_CENTER_JUSTIFY)
			screen.setText(self.BACK_ID, "Background", szBackText, CvUtil.FONT_CENTER_JUSTIFY, iBackX, self.Y_BACK, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_BACK, 1, -1)
			screen.setText(self.NEXT_ID, "Background", szNextText, CvUtil.FONT_CENTER_JUSTIFY, iNextX, self.Y_NEXT, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_FORWARD, 1, -1)
			screen.setText(self.EXIT_ID, "Background", self.EXIT_TEXT, CvUtil.FONT_CENTER_JUSTIFY, iExitX, self.Y_EXIT, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_CLOSE_SCREEN, -1, -1)
			return
		# <!-- custom: keep footer controls stable but grey Back/Next/Clear when they have no effect. (GPT-5.5) -->
		screen.setText(self.BACK_ID, "Background", szBackText, CvUtil.FONT_LEFT_JUSTIFY, self.X_BACK, self.Y_BACK, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_BACK, 1, -1)
		screen.setText(self.NEXT_ID, "Background", szNextText, CvUtil.FONT_LEFT_JUSTIFY, self.X_NEXT, self.Y_NEXT, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_FORWARD, 1, -1)
		screen.setText(self.SAS_CLEAR_ID, "Background", szClearText, CvUtil.FONT_LEFT_JUSTIFY, self.SAS_X_CLEAR, self.SAS_Y_CLEAR, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.setText(self.EXIT_ID, "Background", self.EXIT_TEXT, CvUtil.FONT_RIGHT_JUSTIFY, self.X_EXIT, self.Y_EXIT, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_CLOSE_SCREEN, -1, -1)
		self.SAS_setGamePlayerIdNavigationTexts(screen, iCategory)

	def SAS_getLeaderFooterNavigationPositions(self):
		# <!-- custom: Leader AIP columns take the full page height and their width shifts with font/upscale settings, so fixed footer links could overlap the AIP text zone.
		# Keep Legend in its normal footer spot and treat it as the first slot, then center Clear/player arrows/Back/Next/Exit in the following slots up to the real AIP left edge. (GPT-5.5) -->
		iStartX = self.SAS_getFooterLegendX()
		iEndX = self.pediaLeader.getAIPersonalityLeftX()
		# Slot 0 is the fixed Legend area; slots 1-6 are Clear, player prev, player next, Back, Next, Exit.
		iSlotsCount = 7
		iSlotW = (iEndX - iStartX) / iSlotsCount
		aiSlotCenterXCoords = []
		for i in range(1, iSlotsCount):
			aiSlotCenterXCoords.append(iStartX + iSlotW * i + iSlotW / 2)
		return aiSlotCenterXCoords

	def SAS_setGamePlayerIdNavigationTexts(self, screen, iCategory, iPrevX=None, iNextX=None, eJustify=CvUtil.FONT_LEFT_JUSTIFY):
		targets = self.SAS_getGamePlayerIdTargets(iCategory)
		iPrevItem = -1
		iNextItem = -1
		szPrevText = self.SAS_GAME_PLAYER_ID_PREV_TEXT_DISABLED
		szNextText = self.SAS_GAME_PLAYER_ID_NEXT_TEXT_DISABLED
		if len(targets) > 0:
			prevTarget = targets[0]
			nextTarget = targets[len(targets) - 1]
			iPos = self.SAS_getGamePlayerIdTargetPos(targets, self.iItem)
			if iPos != -1:
				if iPos > 0:
					prevTarget = targets[iPos - 1]
					iPrevItem = prevTarget[1]
				if iPos < len(targets) - 1:
					nextTarget = targets[iPos + 1]
					iNextItem = nextTarget[1]
			else:
				prevTarget = (-1, -1)
				nextTarget = targets[0]
				iNextItem = nextTarget[1]
			if iPrevItem >= 0:
				szPrevText = SASTextScale.titleText(localText.getText("TXT_KEY_PEDIA_SAS_GAME_PLAYER_ID_PREV", (prevTarget[0],)))
			else:
				szPrevText = SASTextScale.titleText(localText.changeTextColor(localText.getText("TXT_KEY_PEDIA_SAS_GAME_PLAYER_ID_PREV", (prevTarget[0],)), self.SAS_eFooterDisabledColor))
			if iNextItem >= 0:
				szNextText = SASTextScale.titleText(localText.getText("TXT_KEY_PEDIA_SAS_GAME_PLAYER_ID_NEXT", (nextTarget[0],)))
			else:
				szNextText = SASTextScale.titleText(localText.changeTextColor(localText.getText("TXT_KEY_PEDIA_SAS_GAME_PLAYER_ID_NEXT", (nextTarget[0],)), self.SAS_eFooterDisabledColor))
		if iCategory == SevoScreenEnums.PEDIA_LEADERS or iCategory == SevoScreenEnums.PEDIA_CIVS:
			if iPrevX is None:
				iPrevX = self.SAS_X_GAME_PLAYER_ID_PREV
			if iNextX is None:
				iNextX = self.SAS_X_GAME_PLAYER_ID_NEXT
			if iPrevItem >= 0:
				screen.setText(self.SAS_GAME_PLAYER_ID_PREV_ID, "Background", szPrevText, eJustify, iPrevX, self.Y_BACK, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_GAME_PLAYER_ID_PREV, iPrevItem)
			else:
				screen.setText(self.SAS_GAME_PLAYER_ID_PREV_ID, "Background", szPrevText, eJustify, iPrevX, self.Y_BACK, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			if iNextItem >= 0:
				screen.setText(self.SAS_GAME_PLAYER_ID_NEXT_ID, "Background", szNextText, eJustify, iNextX, self.Y_NEXT, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_GAME_PLAYER_ID_NEXT, iNextItem)
			else:
				screen.setText(self.SAS_GAME_PLAYER_ID_NEXT_ID, "Background", szNextText, eJustify, iNextX, self.Y_NEXT, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		else:
			screen.setText(self.SAS_GAME_PLAYER_ID_PREV_ID, "Background", "", CvUtil.FONT_LEFT_JUSTIFY, self.SAS_X_GAME_PLAYER_ID_PREV, self.Y_BACK, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			screen.setText(self.SAS_GAME_PLAYER_ID_NEXT_ID, "Background", "", CvUtil.FONT_LEFT_JUSTIFY, self.SAS_X_GAME_PLAYER_ID_NEXT, self.Y_NEXT, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def SAS_getGamePlayerIdTargetPos(self, targets, iItem):
		for i, target in enumerate(targets):
			if target[1] == iItem:
				return i
		return -1

	def SAS_getGamePlayerIdTargets(self, iCategory):
		if iCategory != SevoScreenEnums.PEDIA_LEADERS and iCategory != SevoScreenEnums.PEDIA_CIVS:
			return []
		iActivePlayer = gc.getGame().getActivePlayer()
		if iActivePlayer < 0 or iActivePlayer >= gc.getMAX_CIV_PLAYERS():
			return []
		activePlayer = gc.getPlayer(iActivePlayer)
		iActiveTeam = activePlayer.getTeam()
		activeTeam = gc.getTeam(iActiveTeam)
		bDebugMode = gc.getGame().isDebugMode()
		targets = []
		seen = {}
		# <!-- custom: game-player-id arrows intentionally cover only Leaders/Civs, whose pages map cleanly to current opponents.
		# Broader pages such as Traits, Units, or Techs are relation graphs with duplicates/missing links and can confuse scope.
		# Browse unique known living major players' leader/civ infos; exclude barbarian/minor slots like AIP and require met players to avoid spoilers unless debug mode is active.
		# Rebuild on draw/click so stale targets cannot crash if state changed before reopening. (GPT-5.5) -->
		for iPlayer in range(gc.getMAX_CIV_PLAYERS()):
			player = gc.getPlayer(iPlayer)
			if not player.isAlive():
				continue
			if player.isBarbarian() or player.isMinorCiv():
				continue
			iTeam = player.getTeam()
			if iPlayer != iActivePlayer and not bDebugMode and not activeTeam.isHasMet(iTeam):
				continue
			if iCategory == SevoScreenEnums.PEDIA_LEADERS:
				iInfo = player.getLeaderType()
			else:
				iInfo = player.getCivilizationType()
			if iInfo < 0:
				continue
			if not seen.has_key(iInfo):
				targets.append((iPlayer, iInfo))
				seen[iInfo] = True
		return targets

	def SAS_jumpGamePlayerId(self, iData2):
		if iData2 < 0:
			self.SAS_setFooterNavigationTexts(self.getScreen(), self.iCategory)
			return 1
		if self.iCategory != SevoScreenEnums.PEDIA_LEADERS and self.iCategory != SevoScreenEnums.PEDIA_CIVS:
			return 1
		targets = self.SAS_getGamePlayerIdTargets(self.iCategory)
		if self.SAS_getGamePlayerIdTargetPos(targets, iData2) == -1:
			self.SAS_setFooterNavigationTexts(self.getScreen(), self.iCategory)
			return 1
		# <!-- custom: active game-player-id clicks are real Sevopedia navigation and should enter Back/Next history; disabled or stale controls return above after only refreshing footer state. (GPT-5.5) -->
		return self.pediaJump(self.iCategory, iData2, True, False)

	def placeCategories(self, iCategory=None):
		screen = self.getScreen()
		screen.addListBoxGFC(self.CATEGORY_LIST_ID, "", self.X_CATEGORIES, self.Y_CATEGORIES, self.W_CATEGORIES, self.H_CATEGORIES, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSelect(self.CATEGORY_LIST_ID, True)
		screen.setStyle(self.CATEGORY_LIST_ID, "Table_StandardCiv_Style")
		screen.clearListBoxGFC(self.CATEGORY_LIST_ID)
		for i, category in enumerate(self.categoryList):
			graphic = category[0]
			# <advc.002b> Prepend graphic only if there is room
			szHeading = category[1]
			# For English, 16 happens to be OK.
			# <!-- custom: allow room to fit more characters; our text "Concepts (Outdated)" doesn't fit otherwise.
			# Not increasing this further to accommodate the -1 for other languages, as they are unlikely to use such long texts anyway, and i really need or want the extra space (that is not so useful in categories headers i think), was 16 -->
			iThresh = 19
			if gc.getGame().getCurrentLanguage() != 0:
				# <!-- custom: was 15 -->
				iThresh = 19
			if len(szHeading) <= iThresh:
				szHeading = graphic + szHeading # </advc.002b>
			szHeading = SASTextScale.labelText(szHeading)
			screen.appendListBoxStringNoUpdate(self.CATEGORY_LIST_ID, szHeading, WidgetTypes.WIDGET_PEDIA_MAIN, category[2], 0, CvUtil.FONT_LEFT_JUSTIFY)
		screen.updateListBox(self.CATEGORY_LIST_ID)

	def placeTechs(self):
		self.list = self.getTechList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, gc.getTechInfo)

		# <!-- custom: precompute tech statistics cache for starting tech pairs/combos and untradeable techs by era tables. (Claude Opus 4.5) -->
		if not self.IS_TECH_STATISTICS_PREBUILT:
			SevoPediaTech.precomputeTechStatisticsCache()
			self.IS_TECH_STATISTICS_PREBUILT = True

	# Helper to group techs by era (single-pass bucketing).
	# Yep — techs are the easiest one to refactor "like buildings/units", because a tech already has its era (getEra()), so we just do a single pass, bucket into groups[iEra], then emit era headers in era order.
	def SAS_getTechsGroupedByEra(self):
		return SAS_MainGroupings.SAS_getTechsGroupedByEra(self.isSortLists())

	def getTechList(self):
		if self.SAS_cacheTechsTuple is None:
			if self.IS_SAS_SEVOPEDIA_MAIN_TECHS_GROUP_BY_ERA:
				self.SAS_cacheTechsTuple = tuple(self.SAS_getTechsGroupedByEra())
			else:
				# <!-- custom: base advciv's formula, only difference is we cache it now -->
				self.SAS_cacheTechsTuple = tuple(self.getSortedList(gc.getNumTechInfos(), gc.getTechInfo))
		return self.SAS_cacheTechsTuple

	def placeUnits(self):
		self.list = self.getUnitList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, gc.getUnitInfo)

	# Compute the "availability era" for a unit, factoring in unit tech prereqs, prereq building tech, and religion founding tech (if any).
	def SAS_getUnitAvailabilityEra(self, iUnit, iNumUnitAndTechs, iNumBuildingAndTechs):
		return SAS_MainGroupings.SAS_getUnitAvailabilityEra(iUnit, iNumUnitAndTechs, iNumBuildingAndTechs)
	def SAS_getUnitsGroupedByEra_fromBaseList(self, baseList):
		return SAS_MainGroupings.SAS_getUnitsGroupedByEra_fromBaseList(baseList, False, self.SAS_getUnitAvailabilityEra)

	# <!-- custom: similarly, in sevopedia units, group units by era (based on prereq tech) instead of one long list.
	# Code added with the help of chatgpt 5.2 thanks -->
	def getUnitList(self):
		if self.SAS_cacheUnitsTuple is None:
			baseList = self.getSortedList(gc.getNumUnitInfos(), gc.getUnitInfo)
			if self.IS_SAS_SEVOPEDIA_MAIN_UNITS_GROUP_BY_ERA:
				self.SAS_cacheUnitsTuple = tuple(self.SAS_getUnitsGroupedByEra_fromBaseList(baseList))
			else:
				self.SAS_cacheUnitsTuple = tuple(baseList)
		return self.SAS_cacheUnitsTuple

	def placeUnitUpgrades(self):
		screen = self.getScreen()
		self.SAS_prepareSpecialPageDeletingItemList(screen)
		self.UPGRADES_GRAPH_ID = self.getNextWidgetName()
		screen.addScrollPanel(self.UPGRADES_GRAPH_ID, u"", self.X_ITEMS, self.Y_PEDIA_PAGE - 13, self.W_SCREEN - self.X_ITEMS, self.H_PEDIA_PAGE + 2, PanelStyles.PANEL_STYLE_STANDARD)
		# <!-- custom: Unit Upgrade graph used ACTIVATE_NORMAL so Up/Down could scroll it, but that swallowed Left/Right before Sevopedia Back/Next handling.
		# ACTIVATE_MIMICPARENTFOCUS fixes Left/Right Back/Next; losing arrow-key graph scrolling is acceptable because smooth pedia Back/Next navigation is more important. (GPT-5.5) -->
		screen.setActivation(self.UPGRADES_GRAPH_ID, ActivationTypes.ACTIVATE_MIMICPARENTFOCUS)

		# <!-- custom: focus the graph so arrow scrolling works without clicking (chatgpt 5.2 + claude opus 4.5) -->
		try:
			screen.setFocus(self.UPGRADES_GRAPH_ID)
		except:
			pass

		upgradesGraph = UnitUpgradesGraph.UnitUpgradesGraph(self)
		upgradesGraph.getGraph()
		upgradesGraph.drawGraph()

	def placeHandicapChart(self):
		screen = self.getScreen()
		self.SAS_prepareSpecialPageDeletingItemList(screen)
		self.pediaHandicapChart.interfaceScreen()

	def placeGameSpeedChart(self):
		screen = self.getScreen()
		self.SAS_prepareSpecialPageDeletingItemList(screen)
		self.pediaGameSpeedChart.interfaceScreen()

	def placeWorldSizeChart(self):
		screen = self.getScreen()
		self.SAS_prepareSpecialPageDeletingItemList(screen)
		self.pediaWorldSizeChart.interfaceScreen()

	def placeEraChart(self):
		screen = self.getScreen()
		self.SAS_prepareSpecialPageDeletingItemList(screen)
		self.pediaEraChart.interfaceScreen()

	def placeUnitCategories(self):
		self.list = self.getUnitCategoryList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT_COMBAT, gc.getUnitCombatInfo)

	def getUnitCategoryList(self):
		return self.getSortedList(gc.getNumUnitCombatInfos(), gc.getUnitCombatInfo)

	def placePromotions(self):
		self.list = self.getPromotionList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, gc.getPromotionInfo)

	def getPromotionList(self):
		return self.getSortedList(gc.getNumPromotionInfos(), gc.getPromotionInfo)

	def placePromotionTree(self):
		screen = self.getScreen()
		self.SAS_prepareSpecialPageDeletingItemList(screen)
		self.UPGRADES_GRAPH_ID = self.getNextWidgetName()
		screen.addScrollPanel(self.UPGRADES_GRAPH_ID, u"", self.X_ITEMS, self.Y_PEDIA_PAGE - 13, self.W_SCREEN - self.X_ITEMS, self.H_PEDIA_PAGE + 2, PanelStyles.PANEL_STYLE_STANDARD)
		# <!-- custom: Promotion Tree graph used ACTIVATE_NORMAL so Up/Down could scroll it, but that swallowed Left/Right before Sevopedia Back/Next handling.
		# ACTIVATE_MIMICPARENTFOCUS fixes Left/Right Back/Next; losing arrow-key graph scrolling is acceptable because smooth pedia Back/Next navigation is more important. (GPT-5.5) -->
		screen.setActivation(self.UPGRADES_GRAPH_ID, ActivationTypes.ACTIVATE_MIMICPARENTFOCUS)

		# <!-- custom: focus the graph so arrow scrolling works without clicking (chatgpt 5.2 + claude opus 4.5) -->
		try:
			screen.setFocus(self.UPGRADES_GRAPH_ID)
		except:
			pass

		upgradesGraph = UnitUpgradesGraph.PromotionsGraph(self)
		upgradesGraph.getGraph()
		upgradesGraph.drawGraph()

	def placeBuildings(self):
		self.list = self.getBuildingList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, gc.getBuildingInfo)

	# <!-- custom: moved away from SevoPediaBuilding as they are unused there and cleaner to centralize logic here, with the help of ChatGPT-5.2 Thinking thanks -->
	def getBuildingType(self, iBuilding):
		if isWorldWonderClass(gc.getBuildingInfo(iBuilding).getBuildingClassType()):
			return 2
		elif isNationalWonderClass(gc.getBuildingInfo(iBuilding).getBuildingClassType()):
			return 1
		else:
			return 0

	def getBuildingSortedList(self, iBuildingType):
		list2 = []
		for iBuilding in range(gc.getNumBuildingInfos()):
			if self.getBuildingType(iBuilding) == iBuildingType:
				list2.append((gc.getBuildingInfo(iBuilding).getDescription(), iBuilding))
		if self.isSortLists():
			list2.sort()
		return list2

	# Compute the "availability era" for a building, factoring in tech prereqs, special building tech, and religion founding tech.
	def SAS_getBuildingAvailabilityEra(self, iBuilding, iNumAndTechs):
		return SAS_MainGroupings.SAS_getBuildingAvailabilityEra(iBuilding, iNumAndTechs)

	# <!-- custom: helper we can reuse for regular buildings, national wonders, world wonders, with the help of chatgpt 5.2 thanks -->
	def SAS_getBuildingsGroupedByEra_fromBaseList(self, baseList):
		return SAS_MainGroupings.SAS_getBuildingsGroupedByEra_fromBaseList(baseList, False, self.SAS_getBuildingAvailabilityEra)

	def getBuildingList(self):
		if self.SAS_cacheRegularBuildingsTuple is None:
			baseList = self.getBuildingSortedList(0)
			if self.IS_SAS_SEVOPEDIA_MAIN_BUILDINGS_GROUP_BY_ERA:
				self.SAS_cacheRegularBuildingsTuple = tuple(self.SAS_getBuildingsGroupedByEra_fromBaseList(baseList))
			else:
				self.SAS_cacheRegularBuildingsTuple = tuple(baseList)
		return self.SAS_cacheRegularBuildingsTuple

	def placeNationalWonders(self):
		self.list = self.getNationalWonderList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, gc.getBuildingInfo)

	# <!-- custom: also group them by era (tech prereq) with the help of chatgpt 5.2 thanks -->
	def getNationalWonderList(self):
		if self.IS_SAS_SEVOPEDIA_MAIN_BUILDINGS_GROUP_BY_ERA:
			if self.SAS_cacheNationalWondersTuple is None:
				baseList = self.getBuildingSortedList(1)
				self.SAS_cacheNationalWondersTuple = tuple(self.SAS_getBuildingsGroupedByEra_fromBaseList(baseList))
			return self.SAS_cacheNationalWondersTuple
		else:
			if self.SAS_cacheNationalWondersTuple is None:
				self.SAS_cacheNationalWondersTuple = tuple(self.getBuildingSortedList(1))
			return self.SAS_cacheNationalWondersTuple

	def placeWorldWonders(self):
		self.list = self.getWorldWonderList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, gc.getBuildingInfo)

	# <!-- custom: also group them by era (tech prereq) with the help of chatgpt 5.2 thanks -->
	def getWorldWonderList(self):
		if self.IS_SAS_SEVOPEDIA_MAIN_BUILDINGS_GROUP_BY_ERA:
			if self.SAS_cacheWorldWondersTuple is None:
				baseList = self.getBuildingSortedList(2)
				self.SAS_cacheWorldWondersTuple = tuple(self.SAS_getBuildingsGroupedByEra_fromBaseList(baseList))
			return self.SAS_cacheWorldWondersTuple
		else:
			if self.SAS_cacheWorldWondersTuple is None:
				self.SAS_cacheWorldWondersTuple = tuple(self.getBuildingSortedList(2))
			return self.SAS_cacheWorldWondersTuple

	def placeProjects(self):
		self.list = self.getProjectList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROJECT, gc.getProjectInfo)

	# Compute the "availability era" for a project, factoring in its tech prereq.
	def SAS_getProjectAvailabilityEra(self, iProject):
		info = gc.getProjectInfo(iProject)
		if not info or info.isGraphicalOnly():
			return None  # caller should skip

		iTech = info.getTechPrereq()
		if iTech >= 0:
			return gc.getTechInfo(iTech).getEra()

		return -1  # "No Tech Prerequisite" bucket

	# Helper we can reuse for project lists, with the help of chatgpt 5.2 thanks.
	def SAS_getProjectsGroupedByEra_fromBaseList(self, baseList):
		return SAS_MainGroupings.SAS_getProjectsGroupedByEra_fromBaseList(baseList, False, self.SAS_getProjectAvailabilityEra)

	# <!-- custom: similarly, in sevopedia projects, group projects by era instead of one long list.
	# Code added with the help of chatgpt 5.2 thanks -->
	def getProjectList(self):
		if self.SAS_cacheProjectsTuple is None:
			baseList = self.getSortedList(gc.getNumProjectInfos(), gc.getProjectInfo)
			if self.IS_SAS_SEVOPEDIA_MAIN_PROJECTS_GROUP_BY_ERA:
				self.SAS_cacheProjectsTuple = tuple(self.SAS_getProjectsGroupedByEra_fromBaseList(baseList))
			else:
				self.SAS_cacheProjectsTuple = tuple(baseList)
		return self.SAS_cacheProjectsTuple

	def placeSpecialists(self):
		self.list = self.getSpecialistList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_SPECIALIST, gc.getSpecialistInfo)

	# <!-- custom: Helper to group specialists by type (e.g. Engineer vs Great Engineer).
	# Great specialists are identified by "GREAT_" in SpecialistInfo.getType(). (ChatGPT-5.6-Sol) -->
	def SAS_getSpecialistsGroupedByType(self):
		return SAS_MainGroupings.SAS_getSpecialistsGroupedByType(self.isSortLists())

	def getSpecialistList(self):
		if self.SAS_cacheSpecialistsTuple is None:
			if self.IS_SAS_SEVOPEDIA_MAIN_SPECIALISTS_GROUP_BY_TYPE:
				self.SAS_cacheSpecialistsTuple = tuple(self.SAS_getSpecialistsGroupedByType())
			else:
				self.SAS_cacheSpecialistsTuple = tuple(self.getSortedList(gc.getNumSpecialistInfos(), gc.getSpecialistInfo))
		return self.SAS_cacheSpecialistsTuple

	def placeTerrains(self):
		self.list = self.getTerrainList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_TERRAIN, gc.getTerrainInfo)

	def SAS_getTerrainsGroupedByLandWater_fromBaseList(self, baseList):
		return SAS_MainGroupings.SAS_getTerrainsGroupedByLandWater_fromBaseList(baseList, False, self.SAS_SEVOPEDIA_TERRAIN_GRAPHICAL_ONLY_HIGH_IDS)

	def getTerrainList(self):
		if self.SAS_cacheTerrainsTuple is None:
			# <!-- custom: also show terrain_peak and terrain_hill in sevopedia terrains display even though they are plot types but with the changes in sevopedia terrain (new placeRelevantUnits and placeUnitsImpassable panels as of now), these still provide very valuable info so adding them helps a lot; code provided with the help of chatgpt thanks and previous reworks i did with it or with claude ai or and such -->
			# Note: TERRAIN_HILL and TERRAIN_PEAK already exist in CIV4TerrainInfos.xml, but have <bGraphicalOnly>1</bGraphicalOnly>, so they are hidden by the normal list. For Sevopedia, we want them visible, so we include GraphicalOnly here.
			baseList = self.getUnfilteredSortedList(gc.getNumTerrainInfos(), gc.getTerrainInfo)
			if self.IS_SAS_SEVOPEDIA_MAIN_TERRAINS_GROUP_BY_LAND_WATER:
				self.SAS_cacheTerrainsTuple = tuple(self.SAS_getTerrainsGroupedByLandWater_fromBaseList(baseList))
			else:
				self.SAS_cacheTerrainsTuple = tuple(baseList)
		return self.SAS_cacheTerrainsTuple

	def placeFeatures(self):
		self.list = self.getFeatureList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_FEATURE, gc.getFeatureInfo)

		# <!-- custom: quite similarly to other precompute codes in sevopedia main, also do sevopedia bonus's pre-loading logic at time where it is more/most efficient. -->
		if not self.IS_FEATURES_PRE_LOADING_XML_DATA_VALIDATION_DONE:
			SevoPediaFeature.do_pre_load_xml_features_info_required_data_validation()
			self.IS_FEATURES_PRE_LOADING_XML_DATA_VALIDATION_DONE = True
			print("Sevopedia Feature pre load XML data validation done from Sevopedia Main. This should appear only once even if we exit sevopedia entirely, as long as we are during the same gaming session (i.e. game was not exited) (for info, in SevopediaMain, self.IS_FEATURES_PRE_LOADING_XML_DATA_VALIDATION_DONE=%s)." % str(self.IS_FEATURES_PRE_LOADING_XML_DATA_VALIDATION_DONE))

	def SAS_getFeaturesGroupedByLandWater_fromBaseList(self, baseList):
		return SAS_MainGroupings.SAS_getFeaturesGroupedByLandWater_fromBaseList(baseList, False, self.SAS_SEVOPEDIA_TERRAIN_GRAPHICAL_ONLY_HIGH_IDS)

	def getFeatureList(self):
		if self.SAS_cacheFeaturesTuple is None:
			baseList = self.getSortedList(gc.getNumFeatureInfos(), gc.getFeatureInfo)
			if self.IS_SAS_SEVOPEDIA_MAIN_FEATURES_GROUP_BY_LAND_WATER:
				self.SAS_cacheFeaturesTuple = tuple(self.SAS_getFeaturesGroupedByLandWater_fromBaseList(baseList))
			else:
				self.SAS_cacheFeaturesTuple = tuple(baseList)
		return self.SAS_cacheFeaturesTuple

	def placeBonuses(self):
		self.list = self.getBonusList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, gc.getBonusInfo)

	def SAS_getTradingImprovementsForBonus(self, iBonus):
		return SAS_MainGroupings.SAS_getTradingImprovementsForBonus(iBonus)

	def SAS_getBonusesGroupedByImprovement_fromBaseList(self, baseList):
		return SAS_MainGroupings.SAS_getBonusesGroupedByImprovement_fromBaseList(baseList, False)

	def getBonusList(self):
		if self.SAS_cacheBonusesTuple is None:
			baseList = self.getSortedList(gc.getNumBonusInfos(), gc.getBonusInfo)
			if self.IS_SAS_SEVOPEDIA_MAIN_BONUSES_GROUP_BY_IMPROVEMENT:
				self.SAS_cacheBonusesTuple = tuple(self.SAS_getBonusesGroupedByImprovement_fromBaseList(baseList))
			else:
				self.SAS_cacheBonusesTuple = tuple(baseList)
		return self.SAS_cacheBonusesTuple

	def placeImprovements(self):
		self.list = self.getImprovementList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_IMPROVEMENT, gc.getImprovementInfo)
		SevoPediaImprovement.precomputeImprovementLeaderCache()

	def placeBuilds(self):
		self.list = self.getBuildList()
		self.placeItems(WidgetTypes.WIDGET_PYTHON, gc.getBuildInfo)

	def SAS_isBonusCapableImprovement(self, iImprovement):
		return SAS_MainGroupings.SAS_isBonusCapableImprovement(iImprovement)

	def SAS_isFoodYieldImprovement(self, iImprovement):
		return SAS_MainGroupings.SAS_isFoodYieldImprovement(iImprovement)

	def SAS_getImprovementsGroupedByTerrain_fromBaseList(self, baseList):
		return SAS_MainGroupings.SAS_getImprovementsGroupedByTerrain_fromBaseList(baseList, False)

	def getImprovementList(self):
		if self.SAS_cacheImprovementsTuple is None:
			# <!-- custom: Use the unfiltered list so graphical-only improvements can be considered below; Territory/BFC tables now expose those markers on real plots, and grouping code keeps them separate from Worker improvements. (GPT-5.5) -->
			imprList = self.getUnfilteredSortedList(gc.getNumImprovementInfos(), gc.getImprovementInfo)
			# <advc.004y>
			baseList = []
			for descr, i in imprList:
				info = gc.getImprovementInfo(i)
				# The alt. conditions are for Forest Preserve and Fort
				# <!-- custom: Include special map improvements alongside the newly unfiltered graphical-only entries. (GPT-5.5) -->
				if info.isGraphicalOnly() or SAS_MainGroupings.SAS_isSpecialMapImprovement(i) or info.getPillageGold() > 0 or info.isRequiresFeature() or info.isOutsideBorders():
					baseList.append((descr, i))
			# </advc.004y>

			if self.IS_SAS_SEVOPEDIA_MAIN_IMPROVEMENTS_GROUP_BY_TERRAIN:
				self.SAS_cacheImprovementsTuple = tuple(self.SAS_getImprovementsGroupedByTerrain_fromBaseList(baseList))
			else:
				self.SAS_cacheImprovementsTuple = tuple(baseList)

		return self.SAS_cacheImprovementsTuple

	# <!-- custom: in sevopedia build list, group builds mirroring the improvement/feature categories.
	# Implemented with claude opus 4.5's help thanks a lot -->
	def SAS_getBuildsGroupedByType_fromBaseList(self, baseList):
		return SAS_MainGroupings.SAS_getBuildsGroupedByType_fromBaseList(baseList, False)

	def getBuildList(self):
		if self.SAS_cacheBuildsTuple is None:
			baseList = self.getSortedList(gc.getNumBuildInfos(), gc.getBuildInfo)
			if self.IS_SAS_SEVOPEDIA_MAIN_BUILDS_GROUP_BY_TYPE:
				self.SAS_cacheBuildsTuple = tuple(self.SAS_getBuildsGroupedByType_fromBaseList(baseList))
			else:
				self.SAS_cacheBuildsTuple = tuple(baseList)
		return self.SAS_cacheBuildsTuple

	def placeCivs(self):
		self.list = self.getCivilizationList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIV, gc.getCivilizationInfo)

	def getCivilizationList(self):
		civList = self.getSortedList(gc.getNumCivilizationInfos(), gc.getCivilizationInfo)
		# <advc.004y> Filter out minor civ, but not Barbarians (which do have sensible strategy text). Distinguish b/w them by checking for free Palace. Not sure if that's really better than using getInfoTypeOrFail("CIVILIZATION_MINOR")
		r = []
		for descr,i in civList:
			# <!-- custom: reveal minor nation, i want all information available in the sevopedia.
			# This information may be useful. -->
			#info = gc.getCivilizationInfo(i)
			#if not info.isPlayable():
			#	iCapitalBuildingClass = gc.getDefineINT("CAPITAL_BUILDINGCLASS")
			#	if (iCapitalBuildingClass >= 0 and info.isCivilizationFreeBuildingClass(iCapitalBuildingClass)):
			#		continue
			r.append((descr,i))

		if self.IS_SAS_SEVOPEDIA_MAIN_CIVS_GROUP_BY_ARTSTYLE:
			return SAS_MainGroupings.SAS_getCivilizationsGroupedByArtStyle(self.isSortLists())

		return r # </advc.004y>

	def placeLeaders(self):
		self.list = self.getLeaderList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_LEADER, gc.getLeaderHeadInfo)

		# <!-- custom: prebuild the sevopedia leader cache only after we click on leaders button, so that if we open sevopedia and never access the leaders page, we don't compute needlessly a cached leader that is needlessly expensive.
		# After asking chatgpt, it advised me to do this here; note: place it after the list is computed so it doesn't appear to hang before user click on an item.
		# Update: no longer relevant now that we render the first item right away (no blank item page anymore), but kept as such because works fine as such as well -->
		if self.IS_SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_ENABLE and (not self.IS_SEVOPEDIALEADER_CACHE_PREBUILT):
			# <!-- custom: when AI personality is disabled by define, skip cache precompute entirely to avoid needless work. (GPT-5.3-Codex) -->
			SevoPediaLeader.LEADERS_INFO_CACHED = SevoPediaLeader.getPrecomputedCacheOnceOnlyFromSevopediaMainInSevopediaLeaderForEntireSession()
			# <!-- custom: do not rebuild if built once already, for the entire session keep the same cache, even if we exit sevopedia, store data in memory or wherever it is stored, but do not build it until we click on leaders category the first time, not at module load (so a bit later than module load and not automatic but conditional in this case), but still before any leader is selected -->
			self.IS_SEVOPEDIALEADER_CACHE_PREBUILT = True
			print("Sevopedia Leader cache prebuilt from Sevopedia Main. This should appear only once even if we exit sevopedia entirely, as long as we are during the same gaming session (i.e. game was not exited) (for info, in SevopediaMain, self.IS_SEVOPEDIALEADER_CACHE_PREBUILT=%s)." % str(self.IS_SEVOPEDIALEADER_CACHE_PREBUILT))

	def getLeaderList(self):
		if self.IS_SAS_SEVOPEDIA_MAIN_LEADERS_GROUP_BY_CIV:
			return SAS_MainGroupings.SAS_getLeadersGroupedByCivilization(self.isSortLists())

		# <advc.004y>
		r = self.getSortedList(gc.getNumLeaderHeadInfos(), gc.getLeaderHeadInfo)
		# Barbs should be in position 0, but use WonderConstructRand to confirm.
		if len(r) > 0 and gc.getLeaderHeadInfo(r[0][1]).getWonderConstructRand() <= 0:
			r.pop(0)
		return r # </advc.004y>

	# <!-- custom: Sevopedia Traits rework - Previously traits used a hacky CONCEPT_TRAIT_* wrapper approach where trait entries were stored as NewConcept entries and required extracting the actual TraitInfo via string parsing.
	# Now traits use WIDGET_PYTHON routing like Builds, reading directly from CIV4TraitInfos.xml.
	# This is cleaner, allows proper linking from Sevopedia Leader, and the CONCEPT_TRAIT_* XML entries are deleted.
	# See also: SevoPediaIndex.py (row mapping), SevoPediaTrait.py (receives trait ID directly), and CvGameTextMgr.cpp parseTraits (DLL adds <link=literal> for clickable trait links). (Claude Opus 4.5 / GPT-5.5) -->
	def placeTraits(self):
		self.list = self.getTraitList()
		self.placeItems(WidgetTypes.WIDGET_PYTHON, gc.getTraitInfo)
		# <!-- custom: Trigger cache precomputation on first Traits category click.
		# Similar to SevoPediaLeader's pattern - compute once, reuse for session. (Claude Opus 4.5) -->
		SevoPediaTrait.precomputeTraitStatisticsCache()

	# <!-- custom: Sort traits alphabetically by raw description (without icon), then display with icon.
	# The icon character would cause incorrect sorting if included. (Claude Opus 4.5) -->
	def getTraitList(self):
		traitList = []
		for iTrait in range(gc.getNumTraitInfos()):
			info = gc.getTraitInfo(iTrait)
			if info:
				# Store (raw_description_for_sorting, display_description_with_icon, id)
				rawDesc = info.getDescription()
				displayDesc = u"%c %s" % (TraitUtil.getIcon(iTrait), rawDesc)
				traitList.append((rawDesc, displayDesc, iTrait))
		if self.isSortLists():
			traitList.sort(key=lambda x: x[0])  # Sort by raw description
		# Return list in expected format: (display_description, id)
		return [(item[1], item[2]) for item in traitList]

	def getTraitInfo(self, id):
		info = gc.getTraitInfo(id)
		if info is None:
			return None
		# Wrap to add icon to description like original implementation
		class TraitInfoWrapper:
			def __init__(self, traitInfo, eTrait):
				self.traitInfo = traitInfo
				self.eTrait = eTrait
			def getDescription(self):
				return u"%c %s" % (TraitUtil.getIcon(self.eTrait), self.traitInfo.getDescription())
			def getButton(self):
				return self.traitInfo.getButton()
			def isGraphicalOnly(self):
				return False  # Traits are never graphical-only
		return TraitInfoWrapper(info, id)

	# <!-- custom: Removed isTraitInfo() - no longer needed since CONCEPT_TRAIT_* entries deleted from XML.
	# Traits now use WIDGET_PYTHON routing. (Claude Opus 4.5 / GPT-5.5) -->

	def placeCivics(self):
		self.list = self.getCivicList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, gc.getCivicInfo)

	# <!-- custom: helper we can reuse for civics grouping, with the help of chatgpt 5.2 thanks -->
	def SAS_getCivicsGroupedByCivicOption(self):
		return SAS_MainGroupings.SAS_getCivicsGroupedByCivicOption(self.isSortLists())

	def getCivicList(self):
		if self.SAS_cacheCivicsTuple is None:
			if self.IS_SAS_SEVOPEDIA_MAIN_CIVICS_GROUP_BY_CIVIC_TYPES:
				self.SAS_cacheCivicsTuple = tuple(self.SAS_getCivicsGroupedByCivicOption())
			else:
				self.SAS_cacheCivicsTuple = tuple(self.getSortedList(gc.getNumCivicInfos(), gc.getCivicInfo))
		return self.SAS_cacheCivicsTuple

	def placeReligions(self):
		self.list = self.getReligionList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION, gc.getReligionInfo)

	# Compute the "availability era" for a religion, based on its founding tech prereq.
	def SAS_getReligionAvailabilityEra(self, iReligion):
		info = gc.getReligionInfo(iReligion)
		if not info or info.isGraphicalOnly():
			return None  # caller should skip

		iTech = info.getTechPrereq()
		if iTech >= 0:
			return gc.getTechInfo(iTech).getEra()

		return -1  # no tech prereq bucket

	# Helper we can reuse for religions.
	def SAS_getReligionsGroupedByEra_fromBaseList(self, baseList):
		return SAS_MainGroupings.SAS_getReligionsGroupedByEra_fromBaseList(baseList, False, self.SAS_getReligionAvailabilityEra)

	# <!-- custom: similarly, in sevopedia religions, group religions by era (based on their founding tech) instead of one long list.
	# Code added with the help of chatgpt 5.2 thanks -->
	def getReligionList(self):
		if self.SAS_cacheReligionsTuple is None:
			if self.IS_SAS_SEVOPEDIA_MAIN_RELIGIONS_GROUP_BY_ERA:
				# <!-- custom: keep XML order within each era group (e.g. Paganism before Hinduism), so grouped religions follow CIV4ReligionInfo.xml instead of alphabetic sorting. (GPT-5.3-Codex) -->
				baseList = self.getSortedList(gc.getNumReligionInfos(), gc.getReligionInfo, noSort=True)
				self.SAS_cacheReligionsTuple = tuple(self.SAS_getReligionsGroupedByEra_fromBaseList(baseList))
			else:
				baseList = self.getSortedList(gc.getNumReligionInfos(), gc.getReligionInfo)
				self.SAS_cacheReligionsTuple = tuple(baseList)
		return self.SAS_cacheReligionsTuple

	def placeCorporations(self):
		self.list = self.getCorporationList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_JUMP_TO_CORPORATION, gc.getCorporationInfo)

	def SAS_getCorporationHQBuilding(self, iCorporation):
		return SAS_MainGroupings.SAS_getCorporationHQBuilding(iCorporation)
	def SAS_getCorporationAvailabilityEra(self, iCorporation, iNumBuildingAndTechs):
		return SAS_MainGroupings.SAS_getCorporationAvailabilityEra(iCorporation, iNumBuildingAndTechs)
	def SAS_getCorporationsGroupedByEra_fromBaseList(self, baseList):
		return SAS_MainGroupings.SAS_getCorporationsGroupedByEra_fromBaseList(baseList, False, self.SAS_getCorporationAvailabilityEra)

	# <!-- custom: similarly, in sevopedia corporations, group corporations by era (based on founding building prereq tech era) instead of one long list.
	# Code added with the help of chatgpt 5.2 thanks -->
	def getCorporationList(self):
		if self.SAS_cacheCorporationsTuple is None:
			baseList = self.getSortedList(gc.getNumCorporationInfos(), gc.getCorporationInfo)
			if self.IS_SAS_SEVOPEDIA_MAIN_CORPORATIONS_GROUP_BY_ERA:
				self.SAS_cacheCorporationsTuple = tuple(self.SAS_getCorporationsGroupedByEra_fromBaseList(baseList))
			else:
				self.SAS_cacheCorporationsTuple = tuple(baseList)
		return self.SAS_cacheCorporationsTuple

	# <!-- custom: Sevopedia Votes category (grouped by vote source).
	# No native engine jump widget for votes, so items use WIDGET_PYTHON routed via SAS_MAGIC_PEDIA_PYTHON_VOTE_ENTRY.
	# Vote pages have no dedicated icon art, so the left-list button reuses the hosting building's icon.
	# Design assumes each vote has exactly one source (modders adding multi-source votes would need to extend the grouping helper). (Claude code Opus 4.7) -->
	def placeVotes(self):
		self.list = self.getVoteList()
		self.placeItems(WidgetTypes.WIDGET_PYTHON, self.getVoteInfoForList)

	def getVoteList(self):
		if self.SAS_cacheVotesTuple is None:
			listEntries = SAS_MainGroupings.SAS_getVotesGroupedByVoteSource(self.isSortLists())
			self.SAS_cacheVotesTuple = tuple(listEntries)
		return self.SAS_cacheVotesTuple

	def getVoteInfoForList(self, id):
		# Return CvVoteInfo; used as the `info` arg to placeItems. The button branch in placeItems detects this function identity and routes to SAS_getVoteButton.
		return gc.getVoteInfo(id)

	def SAS_getVoteButton(self, iVote):
		voteInfo = gc.getVoteInfo(iVote)
		if not voteInfo:
			return ""
		for iSrc in range(gc.getNumVoteSourceInfos()):
			if voteInfo.isVoteSourceType(iSrc):
				for iBuilding in range(gc.getNumBuildingInfos()):
					bi = gc.getBuildingInfo(iBuilding)
					if bi and bi.getVoteSourceType() == iSrc:
						return bi.getButton()
				break
		return ""

	# <!-- custom: Sevopedia Event Triggers category (grouped by earliest era, with the "Any era (no tech requirement)" bucket placed FIRST since those events fire from turn 1).
	# No native engine jump widget for triggers, so items route via WIDGET_PYTHON + SAS_MAGIC_PEDIA_PYTHON_EVENT_TRIGGER_ENTRY like Votes.
	# Left-list button falls back to the first child event's icon when the trigger itself has no button art in XML. (Claude code Opus 4.7) -->
	def placeEventTriggers(self):
		self.list = self.getEventTriggerList()
		self.placeItems(WidgetTypes.WIDGET_PYTHON, self.getEventTriggerInfoForList)

	def getEventTriggerList(self):
		if self.SAS_cacheEventTriggersTuple is None:
			listEntries = SAS_MainGroupings.SAS_getEventTriggersGroupedByEra(self.isSortLists())
			self.SAS_cacheEventTriggersTuple = tuple(listEntries)
		return self.SAS_cacheEventTriggersTuple

	def getEventTriggerInfoForList(self, id):
		return gc.getEventTriggerInfo(id)

	def SAS_getEventTriggerButton(self, iTrigger):
		info = gc.getEventTriggerInfo(iTrigger)
		if not info:
			return ""
		szButton = info.getButton()
		if szButton:
			return szButton
		for i in range(info.getNumEvents()):
			iEvent = info.getEvent(i)
			if iEvent < 0:
				continue
			eventInfo = gc.getEventInfo(iEvent)
			if eventInfo:
				szEventButton = eventInfo.getButton()
				if szEventButton:
					return szEventButton
		return ""

	def placeConcepts(self):
		self.list = self.getConceptList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_DESCRIPTION, gc.getConceptInfo)

	def getConceptList(self):
		return self.getSortedList(gc.getNumConceptInfos(), gc.getConceptInfo)

	def placeBTSConcepts(self):
		self.list = self.getNewConceptList()
		self.placeItems(WidgetTypes.WIDGET_PEDIA_DESCRIPTION, self.getNewConceptInfo)

	def getNewConceptList(self):
		return self.getSortedList(gc.getNumNewConceptInfos(), self.getNewConceptInfo)

	def getNewConceptInfo(self, id):
		info = gc.getNewConceptInfo(id)
		# <!-- custom: Removed isTraitInfo check - CONCEPT_TRAIT_* entries deleted from XML since Traits now use WIDGET_PYTHON routing. (Claude Opus 4.5 / GPT-5.5) -->
		if not self.isShortcutInfo(info):
			return info
		return None

	def placeHints(self):
		screen = self.getScreen()
		self.SAS_prepareSpecialPageDeletingItemList(screen)
		# <advc.004y> Put a Blue50-styled panel behind the ListBox below, using the same dimensions.
		screen.addPanel(self.getNextWidgetName(), "", "", false, false, self.X_ITEMS, self.Y_PEDIA_PAGE - 10, self.W_SCREEN - self.X_ITEMS, self.H_PEDIA_PAGE + 23, PanelStyles.PANEL_STYLE_BLUE50)
		# </advc.004y>
		szHintBox = self.getNextWidgetName()
		screen.addListBoxGFC(szHintBox, "", self.X_ITEMS, self.Y_PEDIA_PAGE - 10, self.W_SCREEN - self.X_ITEMS, self.H_PEDIA_PAGE + 23, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSelect(szHintBox, False)
		szHintsText = CyGameTextMgr().buildHintsList()
		# <!-- custom: no need to import string module just to split a string, use native code instead and as per chatgpt's explanation and what i understood of it -->
		hintText = szHintsText.split("\n")
		for hint in hintText:
			if len(hint) != 0:
				screen.appendListBoxStringNoUpdate(szHintBox, SASTextScale.labelText(hint), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.updateListBox(szHintBox)

	def placeShortcuts(self):
		self.list = self.getSortedList(gc.getNumNewConceptInfos(), self.getShortcutInfo)
		self.placeItems(WidgetTypes.WIDGET_PEDIA_DESCRIPTION, self.getShortcutInfo)

	def getShortcutInfo(self, id):
		info = gc.getNewConceptInfo(id)
		if self.isShortcutInfo(info):
			return info
		return None

	def placeMovies(self):
		self.list = self.getMovieList()
		self.placeItems(WidgetTypes.WIDGET_PYTHON, self.getMovieInfo)

	def placeMusic(self):
		self.list = self.getMusicList()
		self.placeItems(WidgetTypes.WIDGET_PYTHON, self.getMusicInfo)

	def getMovieList(self):
		# <!-- custom: build base list in groupings module, then add headers there. (ChatGPT-5.2 Thinking) -->
		if self.SAS_cacheMoviesTuple is None:
			listEntries = SAS_MainGroupings.SAS_getMoviesListGroupedByType(self.isSortLists(), self.SAS_packMovieKey, self.SAS_unpackMovieKey, SAS_MAGIC_PEDIA_MOVIE_TYPE_VICTORY, SAS_MAGIC_PEDIA_MOVIE_TYPE_WONDER, SAS_MAGIC_PEDIA_MOVIE_TYPE_PROJECT, SAS_MAGIC_PEDIA_MOVIE_TYPE_RELIGION, SAS_MAGIC_PEDIA_MOVIE_TYPE_ERA, SAS_MAGIC_PEDIA_MOVIE_TYPE_CORPORATION)
			self.SAS_cacheMoviesTuple = tuple(listEntries)
		return self.SAS_cacheMoviesTuple

	def getMusicList(self):
		# <!-- custom: build music base lists + section headers in groupings module to keep this file clean. (ChatGPT-5.2 Thinking) -->
		if self.SAS_cacheMusicTuple is None:
			(listEntries, self.SAS_musicEraTracks, self.SAS_musicLeaderTracks, self.SAS_musicCivTracks, self.SAS_musicScriptTracks, self.SAS_musicScript3DTracks, self.SAS_firstCivScript3DMusicKey) = SAS_MainGroupings.SAS_getMusicListAndTables(self.isSortLists(), self.SAS_packMusicKey, self.SAS_unpackMusicKey, SAS_MAGIC_PEDIA_MUSIC_TYPE_TECH, SAS_MAGIC_PEDIA_MUSIC_TYPE_ERA, SAS_MAGIC_PEDIA_MUSIC_TYPE_LEADER, SAS_MAGIC_PEDIA_MUSIC_TYPE_CIV, SAS_MAGIC_PEDIA_MUSIC_TYPE_SCRIPT, SAS_MAGIC_PEDIA_MUSIC_TYPE_SCRIPT_3D, self.IS_SAS_SEVOPEDIA_MUSIC_LEADER_INTRO_PEACE_FIRST_ONLY, self.IS_SAS_SEVOPEDIA_MUSIC_LEADER_PEACE_FIRST_ONLY, self.IS_SAS_SEVOPEDIA_MUSIC_LEADER_INTRO_WAR_FIRST_LEADER_ONLY, self.IS_SAS_SEVOPEDIA_MUSIC_LEADER_WAR_FIRST_LEADER_ONLY)
			self.SAS_cacheMusicTuple = tuple(listEntries)
		return self.SAS_cacheMusicTuple

	def SAS_packMovieKey(self, iMovieType, iMovieId):
		return (iMovieType << 16) | (iMovieId & 0xFFFF)

	def SAS_unpackMovieKey(self, iPacked):
		iMovieType = (iPacked >> 16) & 0xFFFF
		iMovieId = iPacked & 0xFFFF
		return (iMovieType, iMovieId)

	def SAS_packMusicKey(self, iMusicType, iMusicId):
		return (iMusicType << 16) | (iMusicId & 0xFFFF)

	def SAS_unpackMusicKey(self, iPacked):
		iMusicType = (iPacked >> 16) & 0xFFFF
		iMusicId = iPacked & 0xFFFF
		return (iMusicType, iMusicId)

	def getMovieInfo(self, iPacked):
		iMovieType, iMovieId = self.SAS_unpackMovieKey(iPacked)
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_VICTORY:
			return gc.getVictoryInfo(iMovieId)
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_WONDER:
			return gc.getBuildingInfo(iMovieId)
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_PROJECT:
			return gc.getProjectInfo(iMovieId)
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_RELIGION:
			return gc.getReligionInfo(iMovieId)
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_ERA:
			return gc.getEraInfo(iMovieId)
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_CORPORATION:
			return gc.getCorporationInfo(iMovieId)
		return None

	def getMusicInfo(self, iPacked):
		iMusicType, iMusicId = self.SAS_unpackMusicKey(iPacked)
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_TECH:
			if iMusicId < 0:
				return None
			return gc.getTechInfo(iMusicId)
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_ERA:
			if (self.SAS_musicEraTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicEraTracks)):
				return None
			iEra, _, _ = self.SAS_musicEraTracks[iMusicId]
			return gc.getEraInfo(iEra)
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_LEADER:
			if (self.SAS_musicLeaderTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicLeaderTracks)):
				return None
			iLeader, _, _, _, _ = self.SAS_musicLeaderTracks[iMusicId]
			return gc.getLeaderHeadInfo(iLeader)
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_CIV:
			if (self.SAS_musicCivTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicCivTracks)):
				return None
			iCiv, _, _, _, _ = self.SAS_musicCivTracks[iMusicId]
			return gc.getCivilizationInfo(iCiv)
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_SCRIPT:
			return None
		return None

	def SAS_getMusicSoundScript(self, iPacked):
		iMusicType, iMusicId = self.SAS_unpackMusicKey(iPacked)
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_TECH:
			info = gc.getTechInfo(iMusicId)
			if not info:
				return ""
			try:
				return info.getSound()
			except:
				return ""
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_SCRIPT:
			if (self.SAS_musicScriptTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicScriptTracks)):
				return ""
			szScript, _, _ = self.SAS_musicScriptTracks[iMusicId]
			return szScript
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_CIV:
			if (self.SAS_musicCivTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicCivTracks)):
				return ""
			_, _, szScript, _, _ = self.SAS_musicCivTracks[iMusicId]
			return szScript
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_SCRIPT_3D:
			if (self.SAS_musicScript3DTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicScript3DTracks)):
				return ""
			szScript, _, _ = self.SAS_musicScript3DTracks[iMusicId]
			return szScript
		return ""

	def SAS_getMusicSoundId(self, iPacked):
		iMusicType, iMusicId = self.SAS_unpackMusicKey(iPacked)
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_ERA:
			if (self.SAS_musicEraTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicEraTracks)):
				return -1
			_, iTrackId, _ = self.SAS_musicEraTracks[iMusicId]
			return iTrackId
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_LEADER:
			if (self.SAS_musicLeaderTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicLeaderTracks)):
				return -1
			_, _, _, iSoundId, _ = self.SAS_musicLeaderTracks[iMusicId]
			return iSoundId
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_CIV:
			if (self.SAS_musicCivTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicCivTracks)):
				return -1
			_, iSoundId, _, _, _ = self.SAS_musicCivTracks[iMusicId]
			return iSoundId
		return -1

	def SAS_getMusicEra(self, iPacked):
		iMusicType, iMusicId = self.SAS_unpackMusicKey(iPacked)
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_TECH:
			info = gc.getTechInfo(iMusicId)
			if info:
				return info.getEra()
			return -1
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_ERA:
			if (self.SAS_musicEraTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicEraTracks)):
				return -1
			iEra, _, _ = self.SAS_musicEraTracks[iMusicId]
			return iEra
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_LEADER:
			if (self.SAS_musicLeaderTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicLeaderTracks)):
				return -1
			_, iEra, _, _, _ = self.SAS_musicLeaderTracks[iMusicId]
			return iEra
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_CIV:
			return -1
		return -1

	def SAS_getMusicTitle(self, iPacked):
		iMusicType, iMusicId = self.SAS_unpackMusicKey(iPacked)
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_TECH:
			info = gc.getTechInfo(iMusicId)
			if info:
				return info.getDescription()
			return u""
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_ERA:
			if (self.SAS_musicEraTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicEraTracks)):
				return u""
			iEra, _, iTrack = self.SAS_musicEraTracks[iMusicId]
			info = gc.getEraInfo(iEra)
			if info:
				szTrackName = u""
				try:
					szTrackName = info.getSoundtrackScriptName(iTrack)
				except:
					szTrackName = u""
				szLabel = u"Track " + (u"%02d" % (iTrack + 1))
				if szTrackName:
					szLabel = szLabel + u" - " + unicode(szTrackName)
				return szLabel
			return u"Track " + (u"%02d" % (iTrack + 1))
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_LEADER:
			if (self.SAS_musicLeaderTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicLeaderTracks)):
				return u""
			_, _, _, _, szLabel = self.SAS_musicLeaderTracks[iMusicId]
			return unicode(szLabel)
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_SCRIPT:
			if (self.SAS_musicScriptTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicScriptTracks)):
				return u""
			_, _, szLabel = self.SAS_musicScriptTracks[iMusicId]
			return unicode(szLabel)
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_CIV:
			if (self.SAS_musicCivTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicCivTracks)):
				return u""
			_, _, _, szLabel, _ = self.SAS_musicCivTracks[iMusicId]
			return unicode(szLabel)
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_SCRIPT_3D:
			if (self.SAS_musicScript3DTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicScript3DTracks)):
				return u""
			_, _, szLabel = self.SAS_musicScript3DTracks[iMusicId]
			return unicode(szLabel)
		return u""

	def SAS_getMusicButton(self, iPacked):
		iMusicType, iMusicId = self.SAS_unpackMusicKey(iPacked)
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_TECH:
			info = gc.getTechInfo(iMusicId)
			if info:
				return info.getButton()
			return ""
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_ERA:
			iEra = self.SAS_getMusicEra(iPacked)
			if iEra != -1:
				return gc.getEraInfo(iEra).getButton()
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_LEADER:
			if (self.SAS_musicLeaderTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicLeaderTracks)):
				return ""
			iLeader, _, _, _, _ = self.SAS_musicLeaderTracks[iMusicId]
			info = gc.getLeaderHeadInfo(iLeader)
			if info:
				return info.getButton()
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_CIV:
			if (self.SAS_musicCivTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicCivTracks)):
				return ""
			iCiv, _, _, _, _ = self.SAS_musicCivTracks[iMusicId]
			info = gc.getCivilizationInfo(iCiv)
			if info:
				return info.getButton()
		return ""

	def SAS_getMusicLeaderId(self, iPacked):
		# Returns the leader ID if this is a leader music type, otherwise returns -1
		iMusicType, iMusicId = self.SAS_unpackMusicKey(iPacked)
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_LEADER:
			if (self.SAS_musicLeaderTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicLeaderTracks)):
				return -1
			iLeader, _, _, _, _ = self.SAS_musicLeaderTracks[iMusicId]
			return iLeader
		return -1

	def SAS_getMusicCivId(self, iPacked):
		# Returns the civ ID if this is a civ music type, otherwise returns -1
		iMusicType, iMusicId = self.SAS_unpackMusicKey(iPacked)
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_CIV:
			if (self.SAS_musicCivTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicCivTracks)):
				return -1
			iCiv, _, _, _, _ = self.SAS_musicCivTracks[iMusicId]
			return iCiv
		return -1

	def SAS_isMusicSound3D(self, iPacked):
		# Returns True when a sound id should be played as 3D audio.
		iMusicType, iMusicId = self.SAS_unpackMusicKey(iPacked)
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_CIV:
			if (self.SAS_musicCivTracks is None) or (iMusicId < 0) or (iMusicId >= len(self.SAS_musicCivTracks)):
				return False
			_, _, _, _, bIs3D = self.SAS_musicCivTracks[iMusicId]
			return bIs3D
		return False

	def SAS_getLeaderPeaceMusicKey(self, iLeader):
		# Returns the packed music key for a leader's "Peace" music entry, or -1 if not found. Ensure music cache is populated first
		if self.SAS_musicLeaderTracks is None:
			self.getMusicList()
		if self.SAS_musicLeaderTracks is None or len(self.SAS_musicLeaderTracks) == 0:
			return -1
		for iMusicId, track in enumerate(self.SAS_musicLeaderTracks):
			trackLeader, _, trackType, _, _ = track
			if trackLeader == iLeader and trackType == "Peace":
				return self.SAS_packMusicKey(SAS_MAGIC_PEDIA_MUSIC_TYPE_LEADER, iMusicId)
		return -1

	def SAS_getFirstEraMusicKey(self):
		# Returns the packed music key for the first era track (Track 01 of Ancient era), or -1 if not found. Ensure music cache is populated first
		if self.SAS_musicEraTracks is None:
			self.getMusicList()
		if self.SAS_musicEraTracks is None or len(self.SAS_musicEraTracks) == 0:
			return -1
		return self.SAS_packMusicKey(SAS_MAGIC_PEDIA_MUSIC_TYPE_ERA, 0)

	def SAS_getFirstCivScript3DMusicKey(self):
		# Returns the packed music key for the first item in the Civilizations 3D scripts grouping.
		if self.SAS_firstCivScript3DMusicKey is None:
			self.getMusicList()
		if self.SAS_firstCivScript3DMusicKey is None:
			return -1
		return self.SAS_firstCivScript3DMusicKey

	def SAS_getFirstCivMusicKey(self, iCiv):
		# Returns the packed music key for the first civ-specific entry (Select/Order) in the Music list.
		if self.SAS_musicCivTracks is None:
			self.getMusicList()
		if self.SAS_musicCivTracks is None or len(self.SAS_musicCivTracks) == 0:
			return -1
		for iMusicId, track in enumerate(self.SAS_musicCivTracks):
			iTrackCiv, _, _, _, _ = track
			if iTrackCiv == iCiv:
				return self.SAS_packMusicKey(SAS_MAGIC_PEDIA_MUSIC_TYPE_CIV, iMusicId)
		return -1

	def isShortcutInfo(self, info):
		return info.getType().find("SHORTCUTS") != -1

	def placeItems(self, widget, info):
		screen = self.getScreen()

		# <!-- custom: search bar for the left item list (chatgpt 5.2 + claude opus 4.5) -->
		# <!-- custom: remember last list kind so typing can rebuild it (chatgpt 5.2 + claude opus 4.5) -->
		self.SAS_lastItemsWidget = widget
		self.SAS_lastItemsInfo = info
		# <!-- custom: register self as the active refresher; see SAS_activeListRefresher comment. (Claude code Opus 4.7) -->
		self.SAS_activeListRefresher = self._SAS_refreshLastItems
		self.SAS_activeKeyNavigator = self.SAS_navigateItemList

		# <!-- custom: search bar (top of item list) (chatgpt 5.2 + claude opus 4.5) -->
		self.SAS_syncSearchPanel()

		# <!-- custom: search bar lives in the top header now, so the items list reclaims the vertical space it used to give up. (Claude code Opus 4.7) -->
		iTableY = self.Y_ITEMS
		iTableH = self.H_ITEMS
		# <!-- custom: End - search bar for the left item list (chatgpt 5.2 + claude opus 4.5) -->

		screen.clearListBoxGFC(self.ITEM_LIST_ID)

		screen.addTableControlGFC(self.ITEM_LIST_ID, 1, self.X_ITEMS, iTableY, self.W_ITEMS, iTableH, False, False, 24, 24, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSelect(self.ITEM_LIST_ID, False)
		screen.setStyle(self.ITEM_LIST_ID, "Table_StandardCiv_Style")
		screen.setTableColumnHeader(self.ITEM_LIST_ID, 0, "", self.W_ITEMS)

		# <!-- custom: search bar for the left item list (chatgpt 5.2 + claude opus 4.5) -->
		# <!-- custom: get filter string for search (chatgpt 5.2 + claude opus 4.5) -->
		szFilter = self.SAS_szSearchString.strip().lower()
		bFiltering = (len(szFilter) > 0)

		# <!-- custom: when filtering, keep headers and separators only for groups that contain at least one match (chatgpt 5.2 + claude opus 4.5) -->
		# Implementation note: self.list may contain header/separator rows encoded as ("Title", -1) or ("", -1). When filtering, we precompute which original list indices should be shown. We also build SAS_listIdxToRow so pediaJump can select the correct displayed row.
		setShowListIdx = None
		self.SAS_listIdxToRow = None
		self.SAS_rowToListIdx = {}

		# <!-- custom: rebuild arrow-navigation selectable caches every time we rebuild the list (fixes stale indices / out-of-range) (ChatGPT 5.2 Thinking) -->
		self.SAS_selectableListIdx = []
		self.SAS_itemToSelectablePos = {}

		if bFiltering:
			setShowListIdx = set()

			# Identify header rows (non-empty title with data1 == -1)
			listHeaderIdx = []
			for iListIdx, it in enumerate(self.list):
				if it[1] == -1:
					szTitle = it[0]
					if (szTitle is not None) and (len(szTitle.strip()) > 0):
						listHeaderIdx.append(iListIdx)

			# If there are no headers in this list, fall back to the simple behavior.
			if len(listHeaderIdx) == 0:
				for iListIdx, it in enumerate(self.list):
					if it[1] != -1:
						szName = it[0]
						if (szName is not None) and (szFilter in szName.lower()):
							setShowListIdx.add(iListIdx)
			else:
				listShownHeaderIdx = []
				for iH, iHeaderIdx in enumerate(listHeaderIdx):
					if (iH + 1) < len(listHeaderIdx):
						iNextHeaderIdx = listHeaderIdx[iH + 1]
					else:
						iNextHeaderIdx = len(self.list)
					bSectionHasMatch = False
					# Scan items in this header section for matches.
					for j in range(iHeaderIdx + 1, iNextHeaderIdx):
						it = self.list[j]
						if it[1] != -1:
							szName = it[0]
							if (szName is not None) and (szFilter in szName.lower()):
								bSectionHasMatch = True
								setShowListIdx.add(j)
					if bSectionHasMatch:
						listShownHeaderIdx.append(iHeaderIdx)
						setShowListIdx.add(iHeaderIdx)

				# Include a blank separator ("", -1) before each shown header, except the first shown section.
				for iK in range(1, len(listShownHeaderIdx)):
					iHeaderIdx = listShownHeaderIdx[iK]
					iSpacerIdx = iHeaderIdx - 1
					if iSpacerIdx >= 0:
						it = self.list[iSpacerIdx]
						if it[1] == -1:
							szTitle = it[0]
							if (szTitle is None) or (len(szTitle.strip()) == 0):
								setShowListIdx.add(iSpacerIdx)

			# Build list-index -> displayed-row mapping for correct highlighting in pediaJump.
			self.SAS_listIdxToRow = {}

			# <!-- custom: rebuild arrow-navigation caches (chatgpt 5.2 + claude opus 4.5) -->
			self.SAS_selectableListIdx = []
			self.SAS_itemToSelectablePos = {}

		i = 0
		for idx, item in enumerate(self.list):
			data1 = item[1] # advc.001: Moved up
			szCustomHeaderButtonPlaceItems = ""
			if len(item) > 2:
				szCustomHeaderButtonPlaceItems = item[2]

			# <!-- custom: if filtering, skip rows not selected by our header-aware filter (chatgpt 5.2 + claude opus 4.5) -->
			if bFiltering:
				if idx not in setShowListIdx:
					continue

			# <!-- custom: record mapping from original list index to displayed row index (chatgpt 5.2 + claude opus 4.5) -->
			if self.SAS_listIdxToRow is not None:
				self.SAS_listIdxToRow[idx] = i
			# <!-- custom: End - search bar for the left item list (chatgpt 5.2 + claude opus 4.5) -->

			# <!-- custom: make a common initial variable so we can tweak it in specific elif or such blocks as we see fit and keep common logic at the end; using a long name to avoid weird python scope inheritance issues to unrelated scopes -->
			sTitlePlaceItems = item[0]
			widgetPlaceItems = widget
			bSAS_hasCustomData2 = False
			# Even though you later handle data1 == -1 inside the civics block, you still do szButtonPlaceItems = info(item[1]).getButton() before any header check. When getCivicList() inserts headers and spacers, you now have rows like ("Government", -1) and ("", -1). So you end up calling gc.getCivicInfo(-1).getButton() and Civ4 blows up with Access violation - no RTTI data!. Minimal fix:
			szButtonPlaceItems = ""
			if data1 != -1:
				if info == self.getMovieInfo:
					infoObj = info(data1)
					if infoObj:
						szButtonPlaceItems = infoObj.getButton()
				elif info == self.getMusicInfo:
					szButtonPlaceItems = self.SAS_getMusicButton(data1)
				elif info == self.getVoteInfoForList:
					szButtonPlaceItems = self.SAS_getVoteButton(data1)
				elif info == self.getEventTriggerInfoForList:
					szButtonPlaceItems = self.SAS_getEventTriggerButton(data1)
				else:
					szButtonPlaceItems = info(data1).getButton()

			if info == gc.getBuildInfo:
				widgetPlaceItems = WidgetTypes.WIDGET_HELP_IMPROVEMENT
				if data1 != -1:
					data2 = data1
					data1 = gc.getBuildInfo(data2).getTechPrereq()
					bSAS_hasCustomData2 = True

			# <!-- custom: Handle traits with WIDGET_PYTHON like Builds. (Claude Opus 4.5) -->
			if info == gc.getTraitInfo:
				widgetPlaceItems = WidgetTypes.WIDGET_PYTHON
				if data1 != -1:
					data2 = data1
					data1 = SAS_MAGIC_PEDIA_PYTHON_TRAIT
					bSAS_hasCustomData2 = True

			if info == self.getMovieInfo:
				widgetPlaceItems = WidgetTypes.WIDGET_PYTHON
				if data1 != -1:
					data2 = data1
					data1 = SAS_MAGIC_PEDIA_PYTHON_MOVIE_ENTRY
					bSAS_hasCustomData2 = True
			if info == self.getMusicInfo:
				widgetPlaceItems = WidgetTypes.WIDGET_PYTHON
				if data1 != -1:
					data2 = data1
					data1 = SAS_MAGIC_PEDIA_PYTHON_MUSIC_ENTRY
					bSAS_hasCustomData2 = True
			# <!-- custom: votes use WIDGET_PYTHON routing since no native jump-to-vote widget exists. (Claude code Opus 4.7) -->
			if info == self.getVoteInfoForList:
				widgetPlaceItems = WidgetTypes.WIDGET_PYTHON
				if data1 != -1:
					data2 = data1
					data1 = SAS_MAGIC_PEDIA_PYTHON_VOTE_ENTRY
					bSAS_hasCustomData2 = True
			# <!-- custom: event triggers also have no native jump widget, so route through WIDGET_PYTHON like Votes. (Claude code Opus 4.7) -->
			if info == self.getEventTriggerInfoForList:
				widgetPlaceItems = WidgetTypes.WIDGET_PYTHON
				if data1 != -1:
					data2 = data1
					data1 = SAS_MAGIC_PEDIA_PYTHON_EVENT_TRIGGER_ENTRY
					bSAS_hasCustomData2 = True

			if info == gc.getConceptInfo:
				data1 = CivilopediaPageTypes.CIVILOPEDIA_PAGE_CONCEPT
				# <!-- custom: we cannot change to data1 here as we changed it in this scope so need to properly use item[1] rather as chatgpt 5.2 recommends and that indeed lead to an issue ingame of displaying always same sevopedia concept's text.
				# To be safe, not changing it in other places as well unless we need to -->
				data2 = item[1]

			elif info == self.getNewConceptInfo or info == self.getShortcutInfo: # advc (removed self.getTraitInfo - now uses WIDGET_PYTHON)
				data1 = CivilopediaPageTypes.CIVILOPEDIA_PAGE_CONCEPT_NEW
				data2 = item[1]

			# <advc.001> Widget help for leaders needs the civ ID in data2 (from Taurus)
			elif (info == gc.getLeaderHeadInfo):
				# <!-- custom: detect headers/spacers from the original list marker (item[1] == -1), not from data1.
				# In Builds, data1 is repurposed to tech prereq for WIDGET_HELP_IMPROVEMENT; removable builds use <PrereqTech>NONE</PrereqTech>, so data1 becomes -1 even though they are real entries.
				# If we check data1 here, those entries are misclassified as headers, their button/icon is cleared, and they appear without icons in Sevopedia Builds; the index remains fine because it uses a different list path. See also KI#113. (GPT-5.3-Codex) -->
				if item[1] == -1:
					sTitlePlaceItems = CyTranslator().changeTextColor(item[0], self.COLOR_HIGHLIGHT_TEXT)
					widgetPlaceItems = WidgetTypes.WIDGET_GENERAL
					szButtonPlaceItems = szCustomHeaderButtonPlaceItems
					data2 = 1
				else:
					data2 = SevoPediaLeader.SevoPediaLeader.getCiv(item[1]) # </advc.001>

			else:
				# advc (note): 0 tends to mean no tooltip (or an empty one?). -1 should also work as the default; BULL likes to use 1.
				if not bSAS_hasCustomData2:
					data2 = 1

				# <!-- custom: order Sevopedia civics by civic type (e.g. Government, Economy, etc.).
				# The earlier implementation was derived from RFC Dawn of Civilization and adapted with ChatGPT 5.2. (ChatGPT-5.6-Sol) -->
				# <!-- custom: Header rows such as ("Government", -1) must be handled separately because placeItems() otherwise calls info(item[1]).getButton(); the adopted grouping logic represents these as non-clickable highlighted rows. (ChatGPT-5.6-Sol) -->
				# <!-- custom: similarly, in sevopedia techs, group techs by era (e.g. Ancient Era, Classical Era, etc.) instead of one long list.
				# Also did similarly for sevopedia buildings and similar pages.
				# Code added with the help of chatgpt 5.2 thanks -->
				# <!-- custom: (That is basically the DoC approach, adapted to your variable names.).
				# After this, your item lists can safely contain (..., -1) headers and blank separators. (ChatGPT-5.2) -->
				if item[1] == -1:
					sTitlePlaceItems = CyTranslator().changeTextColor(item[0], self.COLOR_HIGHLIGHT_TEXT)
					widgetPlaceItems = WidgetTypes.WIDGET_GENERAL
					szButtonPlaceItems = szCustomHeaderButtonPlaceItems

			# <!-- custom: cache selectable rows for arrow navigation (skip headers/spacers) (chatgpt 5.2 + claude opus 4.5) -->
			if item[1] != -1:
				self.SAS_itemToSelectablePos[item[1]] = len(self.SAS_selectableListIdx)
				self.SAS_selectableListIdx.append(idx)

			screen.appendTableRow(self.ITEM_LIST_ID)
			screen.setTableText(self.ITEM_LIST_ID, 0, i, SASTextScale.labelText(sTitlePlaceItems), szButtonPlaceItems, widgetPlaceItems, data1, data2, CvUtil.FONT_LEFT_JUSTIFY)
			self.SAS_rowToListIdx[i] = idx
			i += 1
		#screen.updateListBox(self.ITEM_LIST_ID)

	def back(self):
		if (len(self.pediaHistory) > 1):
			self.pediaFuture.append(self.pediaHistory.pop())
			current = self.pediaHistory.pop()
			self.pediaJump(current[0], current[1], False, True)
		return 1

	def forward(self):
		if (self.pediaFuture):
			current = self.pediaFuture.pop()
			self.pediaJump(current[0], current[1], False, True)
		return 1

	def SAS_clearNavigation(self):
		self.pediaFuture = []
		if self.SAS_lastPediaJump is not None:
			self.pediaHistory = [self.SAS_lastPediaJump]
		elif self.iCategory != -1:
			self.pediaHistory = [(self.iCategory, self.iItem)]
		else:
			self.pediaHistory = []
		self.SAS_setFooterNavigationTexts(self.getScreen(), self.iCategory)
		return 1

	def link(self, szLink):
		iCategory = self.SAS_mainLinkToCategory.get(szLink, None)
		if iCategory is not None:
			return self.pediaJump(SevoScreenEnums.PEDIA_MAIN, iCategory, True, True)

		for szNumMethod, szInfoMethod, iCategory in self.SAS_linkMatchDefs:
			iCount = getattr(gc, szNumMethod)()
			getInfo = getattr(gc, szInfoMethod)
			for i in range(iCount):
				info = getInfo(i)
				if info and info.isMatchForLink(szLink, False):
					return self.pediaJump(iCategory, i, True, True)

	def handleInput (self, inputClass):
		if self.pediaMusic.handleOverlayInput(inputClass):
			return 1

		if self.pediaMovies.handleOverlayInput(inputClass):
			return 1

		# Forward to leader page (existing behavior).
		if (inputClass.getPythonFile() == SevoScreenEnums.PEDIA_LEADERS):
			return self.pediaLeader.handleInput(inputClass)
		if self.iCategory == SevoScreenEnums.PEDIA_HANDICAP_CHART:
			iHandled = self.pediaHandicapChart.handleInput(inputClass)
			if iHandled:
				return iHandled
		if self.iCategory == SevoScreenEnums.PEDIA_GAME_SPEED_CHART:
			iHandled = self.pediaGameSpeedChart.handleInput(inputClass)
			if iHandled:
				return iHandled
		if self.iCategory == SevoScreenEnums.PEDIA_WORLD_SIZE_CHART:
			iHandled = self.pediaWorldSizeChart.handleInput(inputClass)
			if iHandled:
				return iHandled
		if self.iCategory == SevoScreenEnums.PEDIA_ERA_CHART:
			iHandled = self.pediaEraChart.handleInput(inputClass)
			if iHandled:
				return iHandled

		# <!-- custom: forward to Index for its letter-button and table clicks; fall through if it does not consume the event so the shared search handler below can still process typing. (Claude code Opus 4.7) -->
		if self.SAS_USE_BOTTOM_TABS and self.isIndexShowing():
			iHandled = self.pediaIndex.handleInput(inputClass)
			if iHandled:
				return iHandled
		if self.isContentsShowing() and self.iCategory == SevoScreenEnums.PEDIA_INDEX:
			iHandled = self.pediaIndex.handleInput(inputClass)
			if iHandled:
				return iHandled

		# <!-- custom: clear button click (chatgpt 5.2 + claude opus 4.5) -->
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED:
			if inputClass.getFunctionName() == self.SAS_CLEAR_ID:
				# <!-- custom: manual reset for session-persistent Sevopedia Back/Next history; keeps the current page only so players can recover a clean navigation chain without closing the game. (GPT-5.5) -->
				return self.SAS_clearNavigation()
			if inputClass.getFunctionName() == self.SAS_CLEAR_SEARCH_ID:
				if self.SAS_isSearchActive():
					self.SAS_szSearchString = u""
					# <!-- custom: reset debounce state when clearing search (chatgpt 5.2 + claude opus 4.5) -->
					self.SAS_keyDebounceByKey = {}
					self.SAS_refreshActiveListView()
				return 1
			if inputClass.getFunctionName() == self.SAS_SEARCH_KEYS_TOGGLE_ID:
				if self.IS_SAS_SEVOPEDIA_SEARCH_CLICKABLE_SPECIAL_CHARS_ENABLE:
					self.SAS_bSearchKeyboardVisible = not self.SAS_bSearchKeyboardVisible
					self.SAS_syncSearchPanel()
				return 1
		# <!-- custom: keyboard input using InputTypes constants like other mod(s) do (chatgpt 5.2 + claude opus 4.5) -->
		# <!-- custom: gate fires whenever a list-rendering page has registered itself as the active refresher, regardless of which tab/mode hosts it. (Claude code Opus 4.7) -->
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CHARACTER:
			screen = self.getScreen()
			if screen.isActive() and (not inputClass.isAltKeyDown()) and (not inputClass.isCtrlKeyDown()):
				iKey = inputClass.getData()
				if iKey == int(InputTypes.KB_LEFT) or iKey == int(InputTypes.KB_RIGHT):
					# <!-- custom: Left/Right Back/Next worked one item at a time within the same category, but crossing back to another category jumped twice from one keypress.
					# Category redraws reset search debounce state; keeping navigation debounce separate fixed this. (GPT-5.5) -->
					if self.SAS_shouldIgnoreDebouncedNavigationKey(iKey):
						return 1
					if iKey == int(InputTypes.KB_LEFT):
						return self.back()
					return self.forward()
				if (self.isContentsShowing() or self.isIndexShowing()) and self.SAS_activeListRefresher is not None:
					# <!-- custom: debounce per key so fast typing does not produce interleaved duplicates like 'gr' -> 'grgr' (chatgpt 5.2 + claude opus 4.5) -->
					if self.SAS_shouldIgnoreDebouncedKey(iKey):
						return 1

					szChar = self.SAS_getVisibleCharacter(inputClass)
					if len(szChar) > 0:
						return self.SAS_appendSearchCharacter(szChar)

					# Handle backspace to delete last character
					if iKey == int(InputTypes.KB_BACKSPACE):
						if len(self.SAS_szSearchString) > 0:
							self.SAS_szSearchString = self.SAS_szSearchString[:-1]
							self.SAS_refreshActiveListView()
							return 1
					# Handle escape to clear search
					elif iKey == int(InputTypes.KB_ESCAPE):
						if self.SAS_isSearchActive():
							self.SAS_szSearchString = u""
							# <!-- custom: reset debounce state when clearing search (chatgpt 5.2 + claude opus 4.5) -->
							self.SAS_keyDebounceByKey = {}
							self.SAS_refreshActiveListView()
							return 1
					# <!-- custom: Based on C2C mod's implementation thanks: add navigation of the item list with the UP/DOWN arrow keys.
					# Code adjusted for AdvCiv-SAS with the help of chatgpt 5.2 and claude opus 4.5. -->
					# <!-- custom: arrow key navigation for item list (chatgpt 5.2 + claude opus 4.5) -->
					# <!-- custom: later refactored from a direct SAS_navigateItemList call into a dispatcher that invokes whichever navigator the active page registered (SAS_navigateItemList for normal pages, SAS_navigateIndexTable for Index). (Claude code Opus 4.7) -->
					elif iKey == int(InputTypes.KB_UP):
						nav = self.SAS_activeKeyNavigator
						if nav is not None and nav(-1):
							return 1
					elif iKey == int(InputTypes.KB_DOWN):
						nav = self.SAS_activeKeyNavigator
						if nav is not None and nav(1):
							return 1
					# <!-- custom: End - Based on C2C mod's implementation thanks: add navigation of the item list with the UP/DOWN arrow keys.
					# Code adjusted for AdvCiv-SAS with the help of chatgpt 5.2 and claude opus 4.5. -->

		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED):
			if inputClass.getFunctionName() == self.ITEM_LIST_ID and self.iCategory == SevoScreenEnums.PEDIA_BUILDS:
				iRow = inputClass.getData()
				iListIdx = self.SAS_rowToListIdx.get(iRow, None)
				if iListIdx is not None:
					item = self.list[iListIdx]
					if item[1] != -1:
						return self.pediaJump(SevoScreenEnums.PEDIA_BUILDS, item[1], True, False)
			# <!-- custom: Handle trait item list clicks like Builds.
			# Without this, clicking trait entries in the Traits category had no effect (though Index linking worked fine). (Claude Opus 4.5) -->
			if inputClass.getFunctionName() == self.ITEM_LIST_ID and self.iCategory == SevoScreenEnums.PEDIA_TRAITS:
				iRow = inputClass.getData()
				iListIdx = self.SAS_rowToListIdx.get(iRow, None)
				if iListIdx is not None:
					item = self.list[iListIdx]
					if item[1] != -1:
						return self.pediaJump(SevoScreenEnums.PEDIA_TRAITS, item[1], True, False)
			if inputClass.getFunctionName() == self.ITEM_LIST_ID and self.iCategory == SevoScreenEnums.PEDIA_MOVIES:
				iRow = inputClass.getData()
				iListIdx = self.SAS_rowToListIdx.get(iRow, None)
				if iListIdx is not None:
					item = self.list[iListIdx]
					if item[1] != -1:
						self.pediaJump(SevoScreenEnums.PEDIA_MOVIES, item[1], True, False)
						self.pediaMovies.playMovie(item[1])
						return 1
			if inputClass.getFunctionName() == self.ITEM_LIST_ID and self.iCategory == SevoScreenEnums.PEDIA_MUSIC:
				iRow = inputClass.getData()
				iListIdx = self.SAS_rowToListIdx.get(iRow, None)
				if iListIdx is not None:
					item = self.list[iListIdx]
					if item[1] != -1:
						self.pediaJump(SevoScreenEnums.PEDIA_MUSIC, item[1], True, False)
						self.pediaMusic.playMusic(item[1])
						return 1
			# <!-- custom: Votes list click -> vote page. (Claude code Opus 4.7) -->
			if inputClass.getFunctionName() == self.ITEM_LIST_ID and self.iCategory == SevoScreenEnums.PEDIA_VOTES:
				iRow = inputClass.getData()
				iListIdx = self.SAS_rowToListIdx.get(iRow, None)
				if iListIdx is not None:
					item = self.list[iListIdx]
					if item[1] != -1:
						return self.pediaJump(SevoScreenEnums.PEDIA_VOTES, item[1], True, False)
			# <!-- custom: Event Triggers list click -> trigger page. (Claude code Opus 4.7) -->
			if inputClass.getFunctionName() == self.ITEM_LIST_ID and self.iCategory == SevoScreenEnums.PEDIA_EVENT_TRIGGERS:
				iRow = inputClass.getData()
				iListIdx = self.SAS_rowToListIdx.get(iRow, None)
				if iListIdx is not None:
					item = self.list[iListIdx]
					if item[1] != -1:
						return self.pediaJump(SevoScreenEnums.PEDIA_EVENT_TRIGGERS, item[1], True, False)

		# Existing TOC/INDEX buttons.
		if self.SAS_USE_BOTTOM_TABS:
			if (inputClass.getFunctionName() == self.TOC_ID):
				self.showContents()
				return 1
			elif (inputClass.getFunctionName() == self.INDEX_ID):
				self.showIndex()
				return 1

		if inputClass.getButtonType() == WidgetTypes.WIDGET_PYTHON:
			iData1 = inputClass.getData1()
			iData2 = inputClass.getData2()
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_BUILD:
				return self.pediaJump(SevoScreenEnums.PEDIA_BUILDS, iData2, True, False)
			# <!-- custom: Handle trait clicks with WIDGET_PYTHON. (Claude Opus 4.5) -->
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_TRAIT:
				return self.pediaJump(SevoScreenEnums.PEDIA_TRAITS, iData2, True, False)
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_MOVIE_ENTRY:
				return self.pediaJump(SevoScreenEnums.PEDIA_MOVIES, iData2, True, False)
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_MOVIE_PLAY:
				self.pediaMovies.playMovie(iData2)
				return 1
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_MUSIC_ENTRY:
				return self.pediaJump(SevoScreenEnums.PEDIA_MUSIC, iData2, True, False)
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_MUSIC_PLAY:
				self.pediaMusic.playMusic(iData2)
				return 1
			# <!-- custom: Vote WIDGET_PYTHON route (no native WIDGET_PEDIA_JUMP_TO_VOTE exists). (Claude code Opus 4.7) -->
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_VOTE_ENTRY:
				return self.pediaJump(SevoScreenEnums.PEDIA_VOTES, iData2, True, False)
			# <!-- custom: Event Trigger WIDGET_PYTHON route (same situation as Votes — no native jump-to-event-trigger widget in the DLL). (Claude code Opus 4.7) -->
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_EVENT_TRIGGER_ENTRY:
				return self.pediaJump(SevoScreenEnums.PEDIA_EVENT_TRIGGERS, iData2, True, False)
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_GAME_PLAYER_ID_PREV or iData1 == SAS_MAGIC_PEDIA_PYTHON_GAME_PLAYER_ID_NEXT:
				return self.SAS_jumpGamePlayerId(iData2)
			# <!-- custom: route search-key clicks through WIDGET_PYTHON/data2 instead of parsing widget-name suffixes.
			# Empirically, Civ can strip numeric suffixes from generated names and trigger: ValueError: invalid literal for int(): in SevoPediaMain.handleInput. (GPT-5.5) -->
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_SEARCH_KEY:
				if self.IS_SAS_SEVOPEDIA_SEARCH_CLICKABLE_SPECIAL_CHARS_ENABLE and iData2 >= 0 and iData2 < len(self.SAS_SEARCH_KEYBOARD_CHARS):
					return self.SAS_appendSearchCharacter(self.SAS_SEARCH_KEYBOARD_CHARS[iData2])
				return 1
			# <!-- custom: chart LOG button is routed through WIDGET_PYTHON/data1 instead of function-name matching because generated widget names can be unstable in Sevopedia; this keeps clicks reliable like Movie/Music actions. (GPT-5.3-Codex) -->
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_CHART_LOG:
				if self.iCategory == SevoScreenEnums.PEDIA_HANDICAP_CHART:
					self.pediaHandicapChart.dumpCsvLog()
					return 1
				if self.iCategory == SevoScreenEnums.PEDIA_GAME_SPEED_CHART:
					self.pediaGameSpeedChart.dumpCsvLog()
					return 1
				if self.iCategory == SevoScreenEnums.PEDIA_WORLD_SIZE_CHART:
					self.pediaWorldSizeChart.dumpCsvLog()
					return 1
				if self.iCategory == SevoScreenEnums.PEDIA_ERA_CHART:
					self.pediaEraChart.dumpCsvLog()
					return 1
			# <!-- custom: toggle expanded Background/History overlay for any pedia category via WIDGET_PYTHON routing.
			# All categories share one ID since only one category is active at a time. (Claude code Sonnet 4.6 + GPT-5.3-Codex) -->
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND:
				if self.iItem != -1:
					pediaScreen = self.mapScreenFunctions.get(self.iCategory, None)
					if pediaScreen and hasattr(pediaScreen, "setHistoryExpanded"):
						pediaScreen.setHistoryExpanded(iData2 == 1)
						self.pediaJump(self.iCategory, self.iItem, False, False)
						return 1
			# <!-- custom: toggle expanded content (non-text; e.g., animation) overlay for pages that implement setContentExpanded (starting with Units). (Claude code Sonnet 4.6 + GPT-5.3-Codex) -->
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_CONTENT_EXPAND:
				if self.iItem != -1:
					pediaScreen = self.mapScreenFunctions.get(self.iCategory, None)
					if pediaScreen and hasattr(pediaScreen, "setContentExpanded"):
						pediaScreen.setContentExpanded(iData2 == 1)
						self.pediaJump(self.iCategory, self.iItem, False, False)
						return 1
			# <!-- custom: reload expanded content panel (re-renders animation with potentially different color) without changing expanded state. (Claude code Sonnet 4.6) -->
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_CONTENT_RELOAD:
				if self.iItem != -1:
					self.pediaJump(self.iCategory, self.iItem, False, False)
					return 1
			# <!-- custom: dev playground: bump the cached panel style index by iData2 (signed delta) and re-jump to redraw the same item with the expanded overlay still open, so the new style is visible immediately without closing/reopening.
			# Reuses pediaJump's existing "preserve expanded state" behavior. See KI#128. (Claude code Opus 4.7 + GPT-5.5) -->
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_PANEL_STYLE_CYCLE:
				if self.iItem != -1:
					cycle_sas_pedia_panel_style(iData2)
					self.pediaJump(self.iCategory, self.iItem, False, False)
					return 1
			# <!-- custom: dev playground: same as the panel style cycle but for the underlying full-screen background DDS art key. See KI#128. (Claude code Opus 4.7 + GPT-5.5) -->
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_BACKGROUND_CYCLE:
				if self.iItem != -1:
					cycle_sas_pedia_background(iData2)
					self.pediaJump(self.iCategory, self.iItem, False, False)
					return 1
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_TEXT_COLOR_CYCLE:
				if self.iItem != -1:
					cycle_sas_pedia_text_color(iData2)
					self.pediaJump(self.iCategory, self.iItem, False, False)
					return 1
			# <!-- custom: route Sevopedia leader attitude preview buttons here as a fallback because some WIDGET_PYTHON clicks may not reach SevoPediaLeader.handleInput depending on pythonFile routing. (GPT-5.3-Codex) -->
			# <!-- custom: this is code from AdvCiv-SAS-NIF-Gallery mod as part of adding sevopedia leader attitude and action buttons: while trying to minimally adding this feature in AdvCiv-SAS mod as well, i tried to remove these iData1 attitude and action checks but then attitude and action buttons become ineffective (i.e. clicking produces no animation action or attitude), so kept here as well in AdvCiv-SAS here as well. -->
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_LEADER_ATTITUDE:
				if self.iCategory == SevoScreenEnums.PEDIA_LEADERS:
					return self.pediaLeader.applyLeaderAttitude(iData2)
			# <!-- custom: route Sevopedia leader action preview buttons (no/greeting/agree/disagree) here as the same fallback path used for attitude buttons. (GPT-5.3-Codex) -->
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_LEADER_ACTION:
				if self.iCategory == SevoScreenEnums.PEDIA_LEADERS:
					return self.pediaLeader.applyLeaderAction(iData2)
			# <!-- custom: route era art preview buttons. (Claude code Sonnet 4.6) -->
			if iData1 == SAS_MAGIC_PEDIA_PYTHON_LEADER_ERA:
				if self.iCategory == SevoScreenEnums.PEDIA_LEADERS:
					return self.pediaLeader.applyLeaderEra(iData2)
		elif inputClass.getButtonType() == WidgetTypes.WIDGET_HELP_IMPROVEMENT:
			iBuild = inputClass.getData2()
			# <!-- custom: use same hybrid trick as Tech Chooser for Sevopedia build entries: this widget restores build hover text, and we route clicks to Builds pedia so behavior stays useful for all build types. See KI#113. (GPT-5.3-Codex) -->
			if iBuild >= 0 and iBuild < gc.getNumBuildInfos():
				return self.pediaJump(SevoScreenEnums.PEDIA_BUILDS, iBuild, True, False)

		return 0
		# <!-- custom: End - search bar for the left item list (chatgpt 5.2 + claude opus 4.5) -->

	def update(self, fDelta):
		if self.pediaMovies.isMoviePlayerOpen():
			self.pediaMovies.updateTimer(fDelta)
		if self.pediaMusic.isMusicPlayerOpen():
			self.pediaMusic.updateTimer(fDelta)

	def deleteAllWidgets(self):
		screen = self.getScreen()
		# <!-- custom: SevoPediaLeader attitude preview widgets use fixed IDs, so they are not covered by the sequential getNextWidgetName deletion loop; remove them explicitly to prevent persistence across category changes. (GPT-5.3-Codex) -->
		self.pediaLeader.deleteAttitudeWidgets(screen)
		iNumWidgets = self.nWidgetCount
		self.nWidgetCount = 0
		for i in range(iNumWidgets):
			screen.deleteWidget(self.getNextWidgetName())
		self.nWidgetCount = 0

	def deleteListWidgets(self):
		screen = self.getScreen()
		screen.deleteWidget("PediaMainCategoryList")
		screen.deleteWidget("PediaMainItemList")
		# <!-- custom: search bar for the left item list (chatgpt 5.2 + claude opus 4.5) -->
		# <!-- custom: also delete search widgets when deleting list widgets (chatgpt 5.2 + claude opus 4.5) -->
		self.SAS_deleteSearchWidgets(screen)
		# <!-- custom: End - search bar for the left item list (chatgpt 5.2 + claude opus 4.5) -->

	def SAS_setItemsWidth(self, iW):
		self.W_ITEMS = iW
		self.X_PEDIA_PAGE = self.X_ITEMS + self.W_ITEMS + 18
		self.R_PEDIA_PAGE = self.W_SCREEN - 20
		self.W_PEDIA_PAGE = self.R_PEDIA_PAGE - self.X_PEDIA_PAGE

	def SAS_getLeaderItemsWidthByCurrentUIFont(self):
		# <!-- custom: nonpositive cutoff disables wide-screen reduction; without the positive guard every normal resolution met a 0/-1 cutoff and forced base width, reversing the documented setting. See KI#230. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if (self.SAS_SEVOPEDIA_LEADER_ITEMS_NO_REDUCE_MIN_WIDTH > 0) and (self.getScreen().getXResolution() >= self.SAS_SEVOPEDIA_LEADER_ITEMS_NO_REDUCE_MIN_WIDTH):
			return self.SAS_W_ITEMS_BASE

		iLabelFont = getSASUIFontLabel()
		if iLabelFont <= 1:
			iWidth = self.SAS_SEVOPEDIA_LEADER_ITEMS_WIDTH_FONT_1
		elif iLabelFont == 2:
			iWidth = self.SAS_SEVOPEDIA_LEADER_ITEMS_WIDTH_FONT_2
		elif iLabelFont == 3:
			iWidth = self.SAS_SEVOPEDIA_LEADER_ITEMS_WIDTH_FONT_3
		else:
			iWidth = self.SAS_SEVOPEDIA_LEADER_ITEMS_WIDTH_FONT_4
		if iWidth <= 0:
			return self.SAS_W_ITEMS_BASE
		return iWidth

	def getNextWidgetName(self):
		szName = self.WIDGET_ID + str(self.nWidgetCount)
		self.nWidgetCount += 1
		return szName

	def isSortLists(self):
		return AdvisorOpt.SevopediaSortItemList()

	# advc.004y: bCheckGraphicalOnly flag added (for CvInfo types that don't have that element)
	def getSortedList(self, numInfos, getInfo, noSort=False, bCheckGraphicalOnly = True):
		list = []
		for i in range(numInfos):
			item = getInfo(i)
			# advc.004y: GraphicalOnly check added
			if item and (not bCheckGraphicalOnly or not item.isGraphicalOnly()):
				list.append((item.getDescription(), i))
		if self.isSortLists() and not noSort:
			list.sort()
		return list

	# <!-- custom: according to chatgpt 5.2 and if i understood it correctly, TERRAIN_HILL and TERRAIN_PEAK already exist in our terrains list as per CIV4TerrainInfos.xml.
	# However they have an <bGraphicalOnly>1</bGraphicalOnly> so they are excluded from the display.
	# Reveal them from the display here.
	# We'll later handle their incorrect <bWater>1</bWater> property when handling the list.
	# At least we have all the terrains we need now -->
	# <!-- custom: generalize this logic by using an alternative helper that we can use if we need to (e.g. for peak, hill, or anything else we'd want it to use it for) without affecting or slowing down the other parts of the code that already use the (filtered/default) getSortedList. -->
	# Wrapper for clarity: same as getSortedList(), but includes GraphicalOnly entries ("Unfiltered" here specifically means "don’t filter GraphicalOnly".). Useful for categories where GraphicalOnly infos are still meaningful in Sevopedia (e.g. terrains like TERRAIN_HILL / TERRAIN_PEAK in our CIV4TerrainInfos.xml).
	def getUnfilteredSortedList(self, numInfos, getInfo, noSort=False, bCheckGraphicalOnly=False):
		return self.getSortedList(numInfos, getInfo, noSort, bCheckGraphicalOnly)
