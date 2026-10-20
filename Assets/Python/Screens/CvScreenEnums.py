## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005

# <!-- custom: The BUG Mod notice below is retained for the definitions moved from SevoScreenEnums.py into this file's SevoScreenEnums class. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->

## Copyright 2007 BUG Mod

# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# Enum for screens...

from CvPythonExtensions import CivilopediaPageTypes

DOMESTIC_ADVISOR = 0
REWARD_SCREEN = 1
DAWN_OF_MAN = 2
TECH_CHOOSER = 3
FOREIGN_ADVISOR = 4
# <!-- custom: legacy screen id 5 now maps to Foreign Diplomacy naming. (GPT-5.3-Codex) -->
FOREIGN_DIPLOMACY_SCREEN = 5
WONDER_MOVIE_SCREEN = 6
RELIGION_SCREEN = 7
INTRO_MOVIE_SCREEN = 8
# <!-- custom: canonical policy advisor screen id; removed civics/policy alias layer for strict naming consistency. (GPT-5.3-Codex) -->
POLICY_ADVISOR_SCREEN = 9
DIPLOMACY_SCREEN = 10
OPTIONS_SCREEN = 11
TECH_SPLASH = 12
REPLAY_SCREEN = 13
MILITARY_ADVISOR = 14
VICTORY_SCREEN = 15
TOP_CIVS = 16
TUTORIAL_SCREEN = 17
INFO_SCREEN = 18
ERA_MOVIE_SCREEN = 19
ADVISOR_SCREEN = 20
HALL_OF_FAME = 21
DAN_QUAYLE_SCREEN = 22
WORLDBUILDER_SCREEN = 23
WORLDBUILDER_DIPLOMACY_SCREEN = 24
VICTORY_MOVIE_SCREEN = 25
UN_SCREEN = 26
ESPIONAGE_ADVISOR = 27
SPACE_SHIP_SCREEN = 28
CORPORATION_SCREEN = 29
# <!-- custom: split Foreign advisor shell id for diplomacy/intel cluster (F3 slot). (GPT-5.3-Codex) -->
FOREIGN_DIPLOMACY_ADVISOR = 30
# <!-- custom: World Advisor owns the Environment tab (moved out of its former location). (GPT-5.5) -->
WORLD_ADVISOR = 31

GAMEFONT_DISPLAY_SCREEN = 75 # advc.gfd

MAIN_INTERFACE = 99
DEBUG_INFO_SCREEN = 100

# <!-- custom: Retain these original vanilla Civilopedia IDs for the legacy option selected by CvScreensInterface.createCivilopedia; they are not aliases for the overlapping SevoScreenEnums IDs below.
# SAS supports Sevopedia; retaining the inherited alternative does not establish coverage of SAS-specific pages/categories. (GPT-6.1-Sol) -->
# Civilopedia Pages start at 200
PEDIA_START = 200
PEDIA_MAIN = 199#CivilopediaPageTypes.CIVILOPEDIA_PAGE_MAIN + PEDIA_START
PEDIA_TECH = CivilopediaPageTypes.CIVILOPEDIA_PAGE_TECH + PEDIA_START
PEDIA_UNIT = CivilopediaPageTypes.CIVILOPEDIA_PAGE_UNIT + PEDIA_START
PEDIA_BUILDING = CivilopediaPageTypes.CIVILOPEDIA_PAGE_BUILDING + PEDIA_START
PEDIA_TERRAIN = CivilopediaPageTypes.CIVILOPEDIA_PAGE_TERRAIN + PEDIA_START
PEDIA_FEATURE = CivilopediaPageTypes.CIVILOPEDIA_PAGE_FEATURE + PEDIA_START
PEDIA_BONUS = CivilopediaPageTypes.CIVILOPEDIA_PAGE_BONUS + PEDIA_START
PEDIA_IMPROVEMENT = CivilopediaPageTypes.CIVILOPEDIA_PAGE_IMPROVEMENT + PEDIA_START
PEDIA_SPECIALIST = CivilopediaPageTypes.CIVILOPEDIA_PAGE_SPECIALIST + PEDIA_START
PEDIA_PROMOTION = CivilopediaPageTypes.CIVILOPEDIA_PAGE_PROMOTION + PEDIA_START
PEDIA_UNIT_CHART = CivilopediaPageTypes.CIVILOPEDIA_PAGE_UNIT_GROUP + PEDIA_START
PEDIA_CIVILIZATION = CivilopediaPageTypes.CIVILOPEDIA_PAGE_CIV + PEDIA_START
PEDIA_LEADER = CivilopediaPageTypes.CIVILOPEDIA_PAGE_LEADER + PEDIA_START
PEDIA_RELIGION = CivilopediaPageTypes.CIVILOPEDIA_PAGE_RELIGION + PEDIA_START
PEDIA_CORPORATION = CivilopediaPageTypes.CIVILOPEDIA_PAGE_CORPORATION + PEDIA_START
PEDIA_CIVIC = CivilopediaPageTypes.CIVILOPEDIA_PAGE_CIVIC + PEDIA_START
PEDIA_PROJECT = CivilopediaPageTypes.CIVILOPEDIA_PAGE_PROJECT + PEDIA_START
PEDIA_CONCEPT = CivilopediaPageTypes.CIVILOPEDIA_PAGE_CONCEPT + PEDIA_START
PEDIA_CONCEPT_NEW = CivilopediaPageTypes.CIVILOPEDIA_PAGE_CONCEPT_NEW + PEDIA_START
PEDIA_HINTS = CivilopediaPageTypes.CIVILOPEDIA_PAGE_HINTS + PEDIA_START
PEDIA_HISTORY = PEDIA_HINTS+1#CivilopediaPageTypes.CIVILOPEDIA_PAGE_HISTORY + PEDIA_START

# <!-- custom: Moved the constants from the removed SevoScreenEnums.py into class SevoScreenEnums in CvScreenEnums.py, so general/vanilla and Sevopedia IDs share one catalog without renumbering.
# The inherited Sevopedia definitions overlap vanilla page IDs and PEDIA_MAIN names; the class keeps those meanings distinct. (GPT-6.1-Sol) -->
class SevoScreenEnums:
	PEDIA_MAIN		= 200
	PEDIA_TECHS		= 201
	PEDIA_UNITS		= 202
	PEDIA_UNIT_UPGRADES	= 203
	PEDIA_UNIT_CATEGORIES	= 204
	PEDIA_PROMOTIONS	= 205
	PEDIA_PROMOTION_TREE	= 206
	PEDIA_BUILDINGS		= 207
	PEDIA_NATIONAL_WONDERS	= 208
	PEDIA_WORLD_WONDERS	= 209
	PEDIA_PROJECTS		= 210
	PEDIA_SPECIALISTS	= 211
	PEDIA_TERRAINS		= 212
	PEDIA_FEATURES		= 213
	PEDIA_BONUSES		= 214
	PEDIA_IMPROVEMENTS	= 215
	PEDIA_BUILDS		= 232
	PEDIA_CIVS			= 216
	PEDIA_LEADERS		= 217
	PEDIA_TRAITS		= 218 # <!-- custom: advc.004y: Restored; retain the trait category in the consolidated catalog. (GPT-6.1-Sol) -->
	PEDIA_CIVICS		= 219
	PEDIA_RELIGIONS		= 220
	PEDIA_CORPORATIONS	= 221
	PEDIA_CONCEPTS		= 222
	PEDIA_BTS_CONCEPTS	= 223
	PEDIA_HINTS		   	= 224
	PEDIA_SHORTCUTS 	= 225
	PEDIA_HANDICAP_CHART	= 226
	PEDIA_INDEX		= 227
	PEDIA_STRATEGY  	= 228
	PEDIA_GAME_SPEED_CHART	= 229
	PEDIA_WORLD_SIZE_CHART	= 230
	PEDIA_ERA_CHART		= 231
	PEDIA_MOVIES		= 233
	PEDIA_MUSIC		= 234
	PEDIA_VOTES		= 235
	PEDIA_EVENT_TRIGGERS	= 236


# <!-- custom: Enrich Python screen diagnostics by deriving names once from this catalog, instead of repeating ID-to-name tables at log callers.
# Like the derive-once category wiring in SevoPediaMain.py (SAS_CATEGORY_DEFS), reuse canonical definitions; here the inputs are integer constants, not page builders or navigation rules.
# Both contain general screens; each contains only its own encyclopedia pages, so one selection handles the overlapping IDs.
# PEDIA_START is an offset, not a screen. (GPT-6.1-Sol) -->
_SAS_SCREEN_NAMES = {}
_SAS_SEVO_SCREEN_NAMES = {}
for _sasName, _sasValue in globals().items():
	if _sasName.isupper() and _sasName != "PEDIA_START" and isinstance(_sasValue, int):
		_SAS_SCREEN_NAMES[_sasValue] = "CvScreenEnums." + _sasName
		if not _sasName.startswith("PEDIA_"):
			_SAS_SEVO_SCREEN_NAMES[_sasValue] = "CvScreenEnums." + _sasName
for _sasName, _sasValue in vars(SevoScreenEnums).items():
	if _sasName.startswith("PEDIA_") and isinstance(_sasValue, int):
		_SAS_SEVO_SCREEN_NAMES[_sasValue] = "CvScreenEnums.SevoScreenEnums." + _sasName


def getSASScreenName(iScreen, bUsingSevopedia=False):
	# <!-- custom: Select the active encyclopedia catalog here so diagnostic callers need no per-page conditions.
	# Retain the numeric ID at callers too: EXE/custom screens absent from the selected catalog are explicitly unregistered rather than guessed. (GPT-6.1-Sol) -->
	if iScreen == -1:
		return "NO_SCREEN"
	if bUsingSevopedia:
		names = _SAS_SEVO_SCREEN_NAMES
	else:
		names = _SAS_SCREEN_NAMES
	return names.get(iScreen, "UNREGISTERED_SCREEN")
