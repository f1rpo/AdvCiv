## ColorUtil
##
## Utility module for accessing Civ4 colors and managing a master dropdown dataset.
##
## Notes
##   - Must be initialized externally by calling init()
##
## Copyright (c) 2008 The BUG Mod.
##
## Author: EmperorFool
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: AdvCiv-SAS does not actively maintain this third-party BUG library file.
# Edits here are limited to repo-wide consistency passes (e.g. getInfoTypeOrFail for fail-loud XML lookups). (Claude code Opus 4.7) -->

from CvPythonExtensions import *
from SASUtils import getInfoTypeOrFail
import BugUtil

gc = CyGlobalContext()

# tuple of interesting color names to be selectable from dropdowns
COLOR_KEYS = ("COLOR_RED", "COLOR_YELLOW", "COLOR_CYAN", "COLOR_GREEN", "COLOR_BLUE", "COLOR_MAGENTA", "COLOR_WHITE", "COLOR_LIGHT_GREY", "COLOR_GREY", "COLOR_DARK_GREY", "COLOR_BLACK")

COLOR_INDEX_IDX = 0
COLOR_TYPE_IDX = 1
COLOR_KEY_IDX = 2
COLOR_NAME_IDX = 3

# list of color tuples (index, type, key, display name)
COLORS = []

# list of color types (enums)
COLOR_TYPES = []

# Maps type # to color tuple
COLORS_BY_TYPE = {}

# Maps key to color tuple
COLORS_BY_KEY = {}

# list of color translated display names
COLOR_DISPLAY_NAMES = []

# Maps key to color type
TYPES_BY_KEY = {}

def getColorKeys():
	return COLOR_KEYS

def getColorTypes():
	return COLOR_TYPES

def getColorDisplayNames():
	# Returns a tuple of the color display names from the color names above.
	return COLOR_DISPLAY_NAMES

def typeToIndex(type):
	# Returns the index of the color from its info type, None if not found.
	try:
		return COLORS_BY_TYPE[type][COLOR_INDEX_IDX]
	except KeyError:
		return None

def indexToType(index):
	# Returns the info type of the color from its index, None if not found.
	return COLORS[index][COLOR_TYPE_IDX]

def keyToIndex(key):
	# Returns the index of the color from its key, None if not found.
	#
	try:
		return COLORS_BY_KEY[key][COLOR_INDEX_IDX]
	except KeyError:
		return None

def indexToKey(index):
	# Returns the key of the color from its index, None if not found.
	#
	return COLORS[index][COLOR_KEY_IDX]

def keyToType(key):
	# Returns the info type of the color from its key, -1 if not found.
	# This works for any valid color -- not just those in the list.
	#
	if key in TYPES_BY_KEY:
		return TYPES_BY_KEY[key]
	if key in COLORS_BY_KEY:
		type = COLORS_BY_KEY[key][COLOR_TYPE_IDX]
	else:
		type = getInfoTypeOrFail(key)
	TYPES_BY_KEY[key] = type
	return type

def createColors(argsList=None):
	for key in COLOR_KEYS:
		type = getInfoTypeOrFail(key)
		if (type >= 0):
			info = gc.getColorInfo(type)
			if (info):
				name = BugUtil.getPlainText("TXT_KEY_" + key, "")
				if (not name):
					name = key.replace("COLOR_", "").replace("_", " ").title()
				COLOR_TYPES.append(type)
				COLOR_DISPLAY_NAMES.append(name)
				color = (len(COLORS), type, key, name)
				COLORS.append(color)
				COLORS_BY_TYPE[type] = color
				COLORS_BY_KEY[key] = color
				TYPES_BY_KEY[key] = type

def init(colors=None):
	if colors is not None and (isinstance(colors, list) or isinstance(colors, tuple)):
		global COLOR_KEYS
		COLOR_KEYS = tuple(colors)
	createColors()
