#
#	FILE:	 Inland_Sea.py
#	AUTHOR:  Bob Thomas (Sirian)
#	CONTRIB: Soren Johnson, Andy Szybalski
#	PURPOSE: Regional map script - Loosely simulates a Mediterranean type
#	         temperate zone with civs ringing a central sea.
#-----------------------------------------------------------------------------
#	Copyright (c) 2005 Firaxis Games, Inc. All rights reserved.
#-----------------------------------------------------------------------------
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#

from CvPythonExtensions import *
import CvUtil
import CvMapGeneratorUtil
import sys
from SAS_WorldSizeUtils import *
from CvMapGeneratorUtil import HintedWorld
from CvMapGeneratorUtil import BonusBalancer

balancer = BonusBalancer()

hinted_world = None

def getDescription():
	return "TXT_KEY_MAP_SCRIPT_INLAND_SEA_DESCR"

def isAdvancedMap():
	"This map should show up in simple mode"
	return 0

def getNumCustomMapOptions():
	return 2

def getNumHiddenCustomMapOptions():
	return 2

def getCustomMapOptionName(argsList):
	[iOption] = argsList
	option_names = {
		0:	"TXT_KEY_MAP_WORLD_WRAP",
		1:  "TXT_KEY_CONCEPT_RESOURCES"
		}
	if not option_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	translated_text = unicode(CyTranslator().getText(option_names[iOption], ()))
	return translated_text

def getNumCustomMapOptionValues(argsList):
	[iOption] = argsList
	option_values = {
		0:	2,
		1:	2
		}
	return option_values[iOption]

def getCustomMapOptionDescAt(argsList):
	[iOption, iSelection] = argsList
	selection_names = {
		0:	{
			0: "TXT_KEY_MAP_WRAP_FLAT",
			1: "TXT_KEY_MAP_WRAP_CYLINDER"
		},
		1:	{
			0: "TXT_KEY_WORLD_STANDARD",
			1: "TXT_KEY_MAP_BALANCED"
			}
		}
	if not selection_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	translated_text = unicode(CyTranslator().getText(selection_names[iOption][iSelection], ()))
	return translated_text

def getCustomMapOptionDefault(argsList):
	[iOption] = argsList
	option_defaults = {
		0:	0,
		1:  0
		}
	return option_defaults[iOption]

def isRandomCustomMapOption(argsList):
	[iOption] = argsList
	option_random = {
		0:	false,
		1:  false
		}
	return option_random[iOption]

def getWrapX():
	map = CyMap()
	return (map.getCustomMapOption(0) == 1)

def getWrapY():
	return false

def normalizeAddExtras():
	if (CyMap().getCustomMapOption(1) == 1):
		balancer.normalizeAddExtras()
	CyPythonMgr().allowDefaultImpl()	# do the rest of the usual normalizeStartingPlots stuff, don't overrride

def addBonusType(argsList):
	[iBonusType] = argsList
	gc = CyGlobalContext()
	type_string = gc.getBonusInfo(iBonusType).getType()

	if (CyMap().getCustomMapOption(1) == 1):
		if (type_string in balancer.resourcesToBalance) or (type_string in balancer.resourcesToEliminate):
			return None # don't place any of this bonus randomly

	CyPythonMgr().allowDefaultImpl() # pretend we didn't implement this method, and let C handle this bonus in the default way

def beforeGeneration():
	"Set up global variables for start point templates"
	global templates
	global shuffledPlayers
	global iTemplateRoll
	templates = {}
	shuffledPlayers = []
	iTemplateRoll = 0
	gc = CyGlobalContext()
	dice = gc.getGame().getMapRand()
	iW = CyMap().getGridWidth()
	iH = CyMap().getGridHeight()

	# List of number of template instances, indexed by number of players.
	configs = [0, 1, 6, 4, 3, 2, 2, 2, 4, 2, 2, 2, 1, 2, 1, 2, 1, 2, 1]

	# Choose a Template to be used for this game.
	iPlayers = gc.getGame().countCivPlayersEverAlive()
	# <!-- custom: Inland Sea templates only cover up to 18 players; for larger counts, keep globals initialized and let findStartingPlot fall back to default start placement. (GPT-5.3-Codex) -->
	if iPlayers > 18 or iPlayers >= len(configs):
		return 0
	iNumTemplates = configs[iPlayers]
	iTemplateRoll = dice.get(iNumTemplates, "Template Selection - Inland Sea PYTHON")

	# Set variance for start plots according to map size vs number of players.
	map_size = CyMap().getWorldSize()
	# <!-- custom: Use SAS worldsizes so Inland Sea supports ARENA/SAS sizes and unknown future sizes without KeyError. (GPT-5.3-Codex; ChatGPT-5.5) -->
	sizevalues = {
		WorldSizeTypes.WORLDSIZE_ARENA: (2, 3),
		WorldSizeTypes.WORLDSIZE_DUEL: (2, 3),
		WorldSizeTypes.WORLDSIZE_TINY: (2, 3),
		WorldSizeTypes.WORLDSIZE_SMALL: (3, 4),
		WorldSizeTypes.WORLDSIZE_STANDARD: (4, 7),
		WorldSizeTypes.WORLDSIZE_LARGE: (5, 10),
		WorldSizeTypes.WORLDSIZE_HUGE: (6, 15),
		WorldSizeTypes.WORLDSIZE_SAS24: (7, 18),
		WorldSizeTypes.WORLDSIZE_SAS32: (8, 22),
		WorldSizeTypes.WORLDSIZE_SAS40: (9, 26),
		WorldSizeTypes.WORLDSIZE_SAS48: (10, 30),
		}
	(threeVar, twoVar) = sas_lookup_world_size(map_size, sizevalues)
	if iPlayers <= threeVar:
		fVar = 3
	elif iPlayers <= twoVar:
		fVar = 2
	else:
		fVar = 1

	# Templates are nested by keys: {(NumPlayers, TemplateID): {PlayerID: [X, Y, xVariance, yVariance]}}
	templates = {(1,0): {0: [0.5, 0.5, int(0.5 * iW), int(0.5 * iH)]},
                 (2,0): {0: [0.1, 0.5, fVar, int(0.5 * iH)],
                         1: [0.9, 0.5, fVar, int(0.5 * iH)]},
                 (2,1): {0: [0.5, 0.167, int(0.3 * iW), fVar],
                         1: [0.5, 0.833, int(0.3 * iW), fVar]},
                 (2,2): {0: [0.3, 0.167, int(0.3 * iW), fVar],
                         1: [0.7, 0.833, int(0.3 * iW), fVar]},
                 (2,3): {0: [0.7, 0.167, int(0.3 * iW), fVar],
                         1: [0.3, 0.833, int(0.3 * iW), fVar]},
                 (2,4): {0: [0.2, 0.333, int(0.2 * iW), int(0.333 * iH)],
                         1: [0.8, 0.667, int(0.2 * iW), int(0.333 * iH)]},
                 (2,5): {0: [0.8, 0.333, int(0.2 * iW), int(0.333 * iH)],
                         1: [0.2, 0.677, int(0.2 * iW), int(0.333 * iH)]},
                 (3,0): {0: [0.1, 0.5, fVar, fVar],
                         1: [0.7, 0.167, fVar, fVar],
                         2: [0.7, 0.833, fVar, fVar]},
                 (3,1): {0: [0.9, 0.5, fVar, fVar],
                         1: [0.3, 0.167, fVar, fVar],
                         2: [0.3, 0.833, fVar, fVar]},
                 (3,2): {0: [0.5, 0.167, fVar, fVar],
                         1: [0.1, 0.833, fVar, fVar],
                         2: [0.9, 0.833, fVar, fVar]},
                 (3,3): {0: [0.5, 0.833, fVar, fVar],
                         1: [0.1, 0.167, fVar, fVar],
                         2: [0.9, 0.167, fVar, fVar]},
                 (4,0): {0: [0.1, 0.5, fVar, fVar],
                         1: [0.5, 0.167, fVar, fVar],
                         2: [0.9, 0.5, fVar, fVar],
                         3: [0.5, 0.833, fVar, fVar]},
                 (4,1): {0: [0.1, 0.167, fVar, fVar],
                         1: [0.7, 0.167, fVar, fVar],
                         2: [0.9, 0.833, fVar, fVar],
                         3: [0.3, 0.833, fVar, fVar]},
                 (4,2): {0: [0.1, 0.833, fVar, fVar],
                         1: [0.7, 0.833, fVar, fVar],
                         2: [0.9, 0.167, fVar, fVar],
                         3: [0.3, 0.167, fVar, fVar]},
                 (5,0): {0: [0.5, 0.167, fVar, fVar],
                         1: [0.125, 0.333, fVar, fVar],
                         2: [0.25, 0.833, fVar, fVar],
                         3: [0.75, 0.833, fVar, fVar],
                         4: [0.875, 0.333, fVar, fVar]},
                 (5,1): {0: [0.5, 0.833, fVar, fVar],
                         1: [0.125, 0.667, fVar, fVar],
                         2: [0.25, 0.167, fVar, fVar],
                         3: [0.75, 0.167, fVar, fVar],
                         4: [0.875, 0.667, fVar, fVar]},
                 (6,0): {0: [0.1, 0.5, fVar, fVar],
                         1: [0.3, 0.167, fVar, fVar],
                         2: [0.7, 0.167, fVar, fVar],
                         3: [0.9, 0.5, fVar, fVar],
                         4: [0.7, 0.833, fVar, fVar],
                         5: [0.3, 0.833, fVar, fVar]},
                 (6,1): {0: [0.1, 0.167, fVar, fVar],
                         1: [0.5, 0.167, fVar, fVar],
                         2: [0.9, 0.167, fVar, fVar],
                         3: [0.9, 0.833, fVar, fVar],
                         4: [0.5, 0.833, fVar, fVar],
                         5: [0.1, 0.833, fVar, fVar]},
                 (7,0): {0: [0.1, 0.5, fVar, fVar],
                         1: [0.2, 0.125, fVar, fVar],
                         2: [0.6, 0.125, fVar, fVar],
                         3: [0.9, 0.25, fVar, fVar],
                         4: [0.9, 0.75, fVar, fVar],
                         5: [0.6, 0.875, fVar, fVar],
                         6: [0.2, 0.875, fVar, fVar]},
                 (7,1): {0: [0.9, 0.5, fVar, fVar],
                         1: [0.8, 0.125, fVar, fVar],
                         2: [0.4, 0.125, fVar, fVar],
                         3: [0.1, 0.25, fVar, fVar],
                         4: [0.1, 0.75, fVar, fVar],
                         5: [0.4, 0.875, fVar, fVar],
                         6: [0.8, 0.875, fVar, fVar]},
                 (8,0): {0: [0.583, 0.125, fVar, fVar],
                         1: [0.25, 0.125, fVar, fVar],
                         2: [0.083, 0.375, fVar, fVar],
                         3: [0.083, 0.875, fVar, fVar],
                         4: [0.417, 0.875, fVar, fVar],
                         5: [0.75, 0.875, fVar, fVar],
                         6: [0.917, 0.625, fVar, fVar],
                         7: [0.917, 0.125, fVar, fVar]},
                 (8,1): {0: [0.417, 0.125, fVar, fVar],
                         1: [0.083, 0.125, fVar, fVar],
                         2: [0.083, 0.625, fVar, fVar],
                         3: [0.25, 0.875, fVar, fVar],
                         4: [0.583, 0.875, fVar, fVar],
                         5: [0.917, 0.875, fVar, fVar],
                         6: [0.917, 0.375, fVar, fVar],
                         7: [0.75, 0.125, fVar, fVar]},
                 (8,2): {0: [0.1, 0.5, fVar, fVar],
                         1: [0.2, 0.125, fVar, fVar],
                         2: [0.5, 0.125, fVar, fVar],
                         3: [0.8, 0.125, fVar, fVar],
                         4: [0.9, 0.5, fVar, fVar],
                         5: [0.8, 0.875, fVar, fVar],
                         6: [0.5, 0.875, fVar, fVar],
                         7: [0.2, 0.875, fVar, fVar]},
                 (8,3): {0: [0.1, 0.75, fVar, fVar],
                         1: [0.1, 0.25, fVar, fVar],
                         2: [0.333, 0.125, fVar, fVar],
                         3: [0.667, 0.125, fVar, fVar],
                         4: [0.9, 0.25, fVar, fVar],
                         5: [0.9, 0.75, fVar, fVar],
                         6: [0.667, 0.875, fVar, fVar],
                         7: [0.333, 0.875, fVar, fVar]},
                 (9,0): {0: [0.833, 0.15, fVar, fVar],
                         1: [0.5, 0.15, fVar, fVar],
                         2: [0.167, 0.15, fVar, fVar],
                         3: [0.08, 0.412, fVar, fVar],
                         4: [0.08, 0.775, fVar, fVar],
                         5: [0.35, 0.85, fVar, fVar],
                         6: [0.65, 0.85, fVar, fVar],
                         7: [0.92, 0.775, fVar, fVar],
                         8: [0.92, 0.412, fVar, fVar]},
                 (9,1): {0: [0.833, 0.85, fVar, fVar],
                         1: [0.5, 0.85, fVar, fVar],
                         2: [0.167, 0.85, fVar, fVar],
                         3: [0.08, 0.588, fVar, fVar],
                         4: [0.08, 0.225, fVar, fVar],
                         5: [0.35, 0.15, fVar, fVar],
                         6: [0.65, 0.15, fVar, fVar],
                         7: [0.92, 0.225, fVar, fVar],
                         8: [0.92, 0.588, fVar, fVar]},
                 (10,0): {0: [0.875, 0.15, fVar, fVar],
                          1: [0.625, 0.15, fVar, fVar],
                          2: [0.375, 0.15, fVar, fVar],
                          3: [0.125, 0.15, fVar, fVar],
                          4: [0.08, 0.5, fVar, fVar],
                          5: [0.125, 0.85, fVar, fVar],
                          6: [0.375, 0.85, fVar, fVar],
                          7: [0.625, 0.85, fVar, fVar],
                          8: [0.875, 0.85, fVar, fVar],
                          9: [0.92, 0.5, fVar, fVar]},
                 (10,1): {0: [0.75, 0.15, fVar, fVar],
                          1: [0.5, 0.15, fVar, fVar],
                          2: [0.25, 0.15, fVar, fVar],
                          3: [0.08, 0.33, fVar, fVar],
                          4: [0.08, 0.67, fVar, fVar],
                          5: [0.25, 0.85, fVar, fVar],
                          6: [0.5, 0.85, fVar, fVar],
                          7: [0.75, 0.85, fVar, fVar],
                          8: [0.92, 0.67, fVar, fVar],
                          9: [0.92, 0.33, fVar, fVar]},
                 (11,0): {0: [0.875, 0.15, fVar, fVar],
                          1: [0.625, 0.15, fVar, fVar],
                          2: [0.375, 0.15, fVar, fVar],
                          3: [0.125, 0.15, fVar, fVar],
                          4: [0.08, 0.45, fVar, fVar],
                          5: [0.08, 0.75, fVar, fVar],
                          6: [0.28, 0.85, fVar, fVar],
                          7: [0.5, 0.85, fVar, fVar],
                          8: [0.72, 0.85, fVar, fVar],
                          9: [0.92, 0.75, fVar, fVar],
                          10: [0.92, 0.45, fVar, fVar]},
                 (11,1): {0: [0.875, 0.85, fVar, fVar],
                          1: [0.625, 0.85, fVar, fVar],
                          2: [0.375, 0.85, fVar, fVar],
                          3: [0.125, 0.85, fVar, fVar],
                          4: [0.08, 0.55, fVar, fVar],
                          5: [0.08, 0.25, fVar, fVar],
                          6: [0.28, 0.15, fVar, fVar],
                          7: [0.5, 0.15, fVar, fVar],
                          8: [0.72, 0.15, fVar, fVar],
                          9: [0.92, 0.25, fVar, fVar],
                          10: [0.92, 0.55, fVar, fVar]},
                 (12,0): {0: [0.7, 0.15, fVar, fVar],
                          1: [0.5, 0.15, fVar, fVar],
                          2: [0.3, 0.15, fVar, fVar],
                          3: [0.1, 0.15, fVar, fVar],
                          4: [0.08, 0.5, fVar, fVar],
                          5: [0.1, 0.85, fVar, fVar],
                          6: [0.3, 0.85, fVar, fVar],
                          7: [0.5, 0.85, fVar, fVar],
                          8: [0.7, 0.85, fVar, fVar],
                          9: [0.9, 0.85, fVar, fVar],
                          10: [0.92, 0.5, fVar, fVar],
                          11: [0.9, 0.15, fVar, fVar]},
                 (13,0): {0: [0.7, 0.125, fVar, fVar],
                          1: [0.5, 0.125, fVar, fVar],
                          2: [0.3, 0.125, fVar, fVar],
                          3: [0.1, 0.125, fVar, fVar],
                          4: [0.08, 0.425, fVar, fVar],
                          5: [0.08, 0.725, fVar, fVar],
                          6: [0.2, 0.875, fVar, fVar],
                          7: [0.4, 0.875, fVar, fVar],
                          8: [0.6, 0.875, fVar, fVar],
                          9: [0.8, 0.875, fVar, fVar],
                          10: [0.92, 0.725, fVar, fVar],
                          11: [0.92, 0.425, fVar, fVar],
                          12: [0.9, 0.125, fVar, fVar]},
                 (13,1): {0: [0.7, 0.875, fVar, fVar],
                          1: [0.5, 0.875, fVar, fVar],
                          2: [0.3, 0.875, fVar, fVar],
                          3: [0.1, 0.875, fVar, fVar],
                          4: [0.08, 0.575, fVar, fVar],
                          5: [0.08, 0.275, fVar, fVar],
                          6: [0.2, 0.125, fVar, fVar],
                          7: [0.4, 0.125, fVar, fVar],
                          8: [0.6, 0.125, fVar, fVar],
                          9: [0.8, 0.125, fVar, fVar],
                          10: [0.92, 0.275, fVar, fVar],
                          11: [0.92, 0.575, fVar, fVar],
                          12: [0.9, 0.875, fVar, fVar]},
                 (14,0): {0: [0.7, 0.125, fVar, fVar],
                          1: [0.5, 0.125, fVar, fVar],
                          2: [0.3, 0.125, fVar, fVar],
                          3: [0.1, 0.125, fVar, fVar],
                          4: [0.08, 0.375, fVar, fVar],
                          5: [0.08, 0.625, fVar, fVar],
                          6: [0.1, 0.875, fVar, fVar],
                          7: [0.3, 0.875, fVar, fVar],
                          8: [0.5, 0.875, fVar, fVar],
                          9: [0.7, 0.875, fVar, fVar],
                          10: [0.9, 0.875, fVar, fVar],
                          11: [0.92, 0.625, fVar, fVar],
                          12: [0.92, 0.375, fVar, fVar],
                          13: [0.9, 0.125, fVar, fVar]},
                 (15,0): {0: [0.583, 0.125, fVar, fVar],
                          1: [0.417, 0.125, fVar, fVar],
                          2: [0.25, 0.125, fVar, fVar],
                          3: [0.083, 0.125, fVar, fVar],
                          4: [0.083, 0.4, fVar, fVar],
                          5: [0.083, 0.65, fVar, fVar],
                          6: [0.1, 0.9, fVar, fVar],
                          7: [0.3, 0.875, fVar, fVar],
                          8: [0.5, 0.875, fVar, fVar],
                          9: [0.7, 0.875, fVar, fVar],
                          10: [0.9, 0.9, fVar, fVar],
                          11: [0.917, 0.65, fVar, fVar],
                          12: [0.917, 0.4, fVar, fVar],
                          13: [0.917, 0.125, fVar, fVar],
                          14: [0.75, 0.125, fVar, fVar]},
                 (15,1): {0: [0.583, 0.875, fVar, fVar],
                          1: [0.417, 0.875, fVar, fVar],
                          2: [0.25, 0.875, fVar, fVar],
                          3: [0.083, 0.875, fVar, fVar],
                          4: [0.083, 0.6, fVar, fVar],
                          5: [0.083, 0.35, fVar, fVar],
                          6: [0.1, 0.1, fVar, fVar],
                          7: [0.3, 0.125, fVar, fVar],
                          8: [0.5, 0.125, fVar, fVar],
                          9: [0.7, 0.125, fVar, fVar],
                          10: [0.9, 0.1, fVar, fVar],
                          11: [0.917, 0.35, fVar, fVar],
                          12: [0.917, 0.6, fVar, fVar],
                          13: [0.917, 0.875, fVar, fVar],
                          14: [0.75, 0.875, fVar, fVar]},
                 (16,0): {0: [0.583, 0.125, fVar, fVar],
                          1: [0.417, 0.125, fVar, fVar],
                          2: [0.25, 0.125, fVar, fVar],
                          3: [0.083, 0.125, fVar, fVar],
                          4: [0.083, 0.375, fVar, fVar],
                          5: [0.083, 0.625, fVar, fVar],
                          6: [0.083, 0.875, fVar, fVar],
                          7: [0.25, 0.875, fVar, fVar],
                          8: [0.417, 0.875, fVar, fVar],
                          9: [0.583, 0.875, fVar, fVar],
                          10: [0.75, 0.875, fVar, fVar],
                          11: [0.917, 0.875, fVar, fVar],
                          12: [0.917, 0.625, fVar, fVar],
                          13: [0.917, 0.375, fVar, fVar],
                          14: [0.917, 0.125, fVar, fVar],
                          15: [0.75, 0.125, fVar, fVar]},
                 (17,0): {0: [0.5, 0.125, fVar, fVar],
                          1: [0.35, 0.125, fVar, fVar],
                          2: [0.2, 0.125, fVar, fVar],
                          3: [0.05, 0.175, fVar, fVar],
                          4: [0.083, 0.45, fVar, fVar],
                          5: [0.083, 0.7, fVar, fVar],
                          6: [0.083, 0.95, fVar, fVar],
                          7: [0.25, 0.875, fVar, fVar],
                          8: [0.417, 0.875, fVar, fVar],
                          9: [0.583, 0.875, fVar, fVar],
                          10: [0.75, 0.875, fVar, fVar],
                          11: [0.917, 0.95, fVar, fVar],
                          12: [0.917, 0.7, fVar, fVar],
                          13: [0.917, 0.45, fVar, fVar],
                          14: [0.95, 0.175, fVar, fVar],
                          15: [0.8, 0.125, fVar, fVar],
                          16: [0.65, 0.125, fVar, fVar]},
                 (17,1): {0: [0.5, 0.875, fVar, fVar],
                          1: [0.35, 0.875, fVar, fVar],
                          2: [0.2, 0.875, fVar, fVar],
                          3: [0.05, 0.825, fVar, fVar],
                          4: [0.083, 0.65, fVar, fVar],
                          5: [0.083, 0.3, fVar, fVar],
                          6: [0.083, 0.05, fVar, fVar],
                          7: [0.25, 0.125, fVar, fVar],
                          8: [0.417, 0.125, fVar, fVar],
                          9: [0.583, 0.125, fVar, fVar],
                          10: [0.75, 0.125, fVar, fVar],
                          11: [0.917, 0.05, fVar, fVar],
                          12: [0.917, 0.3, fVar, fVar],
                          13: [0.917, 0.65, fVar, fVar],
                          14: [0.95, 0.825, fVar, fVar],
                          15: [0.8, 0.875, fVar, fVar],
                          16: [0.65, 0.875, fVar, fVar]},
                 (18,0): {0: [0.5, 0.125, fVar, fVar],
                          1: [0.35, 0.125, fVar, fVar],
                          2: [0.2, 0.125, fVar, fVar],
                          3: [0.05, 0.125, fVar, fVar],
                          4: [0.075, 0.375, fVar, fVar],
                          5: [0.075, 0.625, fVar, fVar],
                          6: [0.05, 0.875, fVar, fVar],
                          7: [0.2, 0.875, fVar, fVar],
                          8: [0.35, 0.875, fVar, fVar],
                          9: [0.5, 0.875, fVar, fVar],
                          10: [0.65, 0.875, fVar, fVar],
                          11: [0.8, 0.875, fVar, fVar],
                          12: [0.95, 0.875, fVar, fVar],
                          13: [0.925, 0.625, fVar, fVar],
                          14: [0.925, 0.375, fVar, fVar],
                          15: [0.95, 0.125, fVar, fVar],
                          16: [0.8, 0.125, fVar, fVar],
                          17: [0.65, 0.125, fVar, fVar]}
	}
	# End of Templates data.

	# <!-- custom: Inland Sea used compact list positions as real PlayerTypes, so sparse/high-ID players lost their handcrafted regions.
	# Assign shuffled template IDs directly to the actual ever-alive player IDs. See KI#267. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	player_list = range(iPlayers)
	shuffledPlayers = {}
	for playerLoop in range(gc.getMAX_CIV_PLAYERS()):
		if gc.getPlayer(playerLoop).isEverAlive():
			iChoosePlayer = dice.get(len(player_list), "Shuffling Template IDs - Inland Sea PYTHON")
			shuffledPlayers[playerLoop] = player_list[iChoosePlayer]
			del player_list[iChoosePlayer]
	return 0

def minStartingDistanceModifier():
	numPlrs = CyGlobalContext().getGame().countCivPlayersEverAlive()
	if numPlrs  <= 18:
		return -95
	else:
		return -50

def findStartingPlot(argsList):
	# Set up for maximum of 18 players! If more, use default implementation.
	iPlayers = CyGlobalContext().getGame().countCivPlayersEverAlive()
	if iPlayers > 18:
		CyPythonMgr().allowDefaultImpl()
		return
	# <!-- custom: Safety fallback: if template globals are missing/empty for any reason, use default placement instead of raising NameError. (GPT-5.3-Codex) -->
	if (not globals().has_key('shuffledPlayers') or not globals().has_key('templates') or not globals().has_key('iTemplateRoll') or len(shuffledPlayers) <= 0 or len(templates) <= 0):
		CyPythonMgr().allowDefaultImpl()
		return

	[playerID] = argsList
	global plotSuccess
	global plotValue

	def isValid(playerID, x, y):
		gc = CyGlobalContext()
		map = CyMap()
		pPlot = map.plot(x, y)
		iW = map.getGridWidth()
		iH = map.getGridHeight()
		iPlayers = gc.getGame().countCivPlayersEverAlive()

		# Use global data set up via beforeGeneration().
		global templates
		global shuffledPlayers
		global iTemplateRoll
		# <!-- custom: The repaired mapping is keyed by actual PlayerTypes rather than compact positions; reject only a genuinely unmapped player. See KI#267. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if not shuffledPlayers.has_key(playerID):
			return false
		playerTemplateAssignment = shuffledPlayers[playerID]
		[fLat, fLon, varX, varY] = templates[(iPlayers, iTemplateRoll)][playerTemplateAssignment]

		# Check to ensure the plot is on the main landmass.
		if (pPlot.getArea() != map.findBiggestArea(False).getID()):
			return false

		# Now check for eligibility according to the defintions found in the template.
		# <!-- custom: Inherited 17-player templates place some centers at 0.05/0.95; on Duel/Tiny and compact Arena grids, clamping the completed window could invert its bounds.
		# Clamp the center inside the two-plot safety envelope first so every configured window contains its own center. See KI#295. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		iX = max(2, min(iW - 3, int(iW * fLat)))
		iY = max(2, min(iH - 3, int(iH * fLon)))
		westX = max(2, iX - varX)
		eastX = min(iW - 3, iX + varX)
		southY = max(2, iY - varY)
		northY = min(iH - 3, iY + varY)
		if x < westX or x > eastX or y < southY or y > northY:
			return false
		else:
			return true

	getStartingPlot(playerID, isValid)
	if plotSuccess:
		return plotValue
	else:
		CyPythonMgr().allowDefaultImpl()
		return

def getStartingPlot(playerID, validFn = None):
	gc = CyGlobalContext()
	map = CyMap()
	player = gc.getPlayer(playerID)
	global plotSuccess
	global plotValue
	plotSuccess = false
	plotValue = -1

	player.AI_updateFoundValues(True)

	iRange = player.startingPlotRange()
	iPass = 0

	while (iPass < 50):
		iBestValue = 0
		pBestPlot = None

		for iX in range(map.getGridWidth()):
			for iY in range(map.getGridHeight()):
				if validFn is not None and not validFn(playerID, iX, iY):
					continue
				pLoopPlot = map.plot(iX, iY)

				val = pLoopPlot.getFoundValue(playerID)

				if val > iBestValue:

					valid = True

					for iI in range(gc.getMAX_CIV_PLAYERS()):
						if (gc.getPlayer(iI).isAlive()):
							if (iI != playerID):
								if gc.getPlayer(iI).startingPlotWithinRange(pLoopPlot, playerID, iRange, iPass):
									valid = False
									break

					if valid:
							iBestValue = val
							pBestPlot = pLoopPlot

		if pBestPlot is not None:
			plotSuccess = true
			plotValue = map.plotNum(pBestPlot.getX(), pBestPlot.getY())
			break

		print("player %s pass %s failed" % (playerID, iPass))

		iPass += 1

	return -1

def getTopLatitude():
	return 60
def getBottomLatitude():
	return -60

def getGridSize(argsList):
	"Because this is such a land-heavy map, override getGridSize() to make the map smaller"
	# <!-- custom: Use SAS worldsizes with dynamic calibrated sizing above Huge so ARENA/SAS sizes don't raise KeyError; Inland Sea is land-heavy (not almost-all-land), so derive SAS tiers from this script's Huge anchor instead of hardcoding them. (GPT-5.3-Codex; ChatGPT-5.5) -->
	grid_sizes = {
		WorldSizeTypes.WORLDSIZE_ARENA: (5, 3),
		WorldSizeTypes.WORLDSIZE_DUEL: (6, 4),
		WorldSizeTypes.WORLDSIZE_TINY: (8, 5),
		WorldSizeTypes.WORLDSIZE_SMALL: (10, 6),
		WorldSizeTypes.WORLDSIZE_STANDARD: (13, 8),
		WorldSizeTypes.WORLDSIZE_LARGE: (16, 10),
		WorldSizeTypes.WORLDSIZE_HUGE: (21, 13),
	}

	if (argsList[0] == -1): # (-1,) is passed to function on loads
		return []
	[eWorldSize] = argsList
	return sas_lookup_world_size_with_calibrated_sas(eWorldSize, grid_sizes)

# Subclasses to fix the FRAC_POLAR zero row bugs.
class ISFractalWorld(CvMapGeneratorUtil.FractalWorld):
	def generatePlotTypes(self, water_percent=78, shift_plot_types=True, grain_amount=3):
		# Check for changes to User Input variances.
		self.checkForOverrideDefaultUserInputVariances()

		self.hillsFrac.fracInit(self.iNumPlotsX, self.iNumPlotsY, grain_amount, self.mapRand, 0, self.fracXExp, self.fracYExp)
		self.peaksFrac.fracInit(self.iNumPlotsX, self.iNumPlotsY, grain_amount+1, self.mapRand, 0, self.fracXExp, self.fracYExp)

		water_percent += self.seaLevelChange
		water_percent = min(water_percent, self.seaLevelMax)
		water_percent = max(water_percent, self.seaLevelMin)

		iWaterThreshold = self.continentsFrac.getHeightFromPercent(water_percent)
		iHillsBottom1 = self.hillsFrac.getHeightFromPercent(max((self.hillGroupOneBase - self.hillGroupOneRange), 0))
		iHillsTop1 = self.hillsFrac.getHeightFromPercent(min((self.hillGroupOneBase + self.hillGroupOneRange), 100))
		iHillsBottom2 = self.hillsFrac.getHeightFromPercent(max((self.hillGroupTwoBase - self.hillGroupTwoRange), 0))
		iHillsTop2 = self.hillsFrac.getHeightFromPercent(min((self.hillGroupTwoBase + self.hillGroupTwoRange), 100))
		iPeakThreshold = self.peaksFrac.getHeightFromPercent(self.peakPercent)

		for x in range(self.iNumPlotsX):
			for y in range(self.iNumPlotsY):
				i = y*self.iNumPlotsX + x
				val = self.continentsFrac.getHeight(x,y)
				if val <= iWaterThreshold:
					self.plotTypes[i] = PlotTypes.PLOT_OCEAN
				else:
					hillVal = self.hillsFrac.getHeight(x,y)
					if ((hillVal >= iHillsBottom1 and hillVal <= iHillsTop1) or (hillVal >= iHillsBottom2 and hillVal <= iHillsTop2)):
						peakVal = self.peaksFrac.getHeight(x,y)
						if (peakVal <= iPeakThreshold):
							self.plotTypes[i] = PlotTypes.PLOT_PEAK
						else:
							self.plotTypes[i] = PlotTypes.PLOT_HILLS
					else:
						self.plotTypes[i] = PlotTypes.PLOT_LAND

		if shift_plot_types:
			self.shiftPlotTypes()

		return self.plotTypes

class ISHintedWorld(CvMapGeneratorUtil.HintedWorld, ISFractalWorld):
	def __doInitFractal(self):
		self.shiftHintsToMap()

		# don't call base method, this overrides it.
		size = len(self.data)
		minExp = min(self.fracXExp, self.fracYExp)
		iGrain = None
		for i in range(minExp):
			width = (1 << (self.fracXExp - minExp + i))
			height = (1 << (self.fracYExp - minExp + i))
			if not self.iFlags & CyFractal.FracVals.FRAC_WRAP_X:
				width += 1
			if not self.iFlags & CyFractal.FracVals.FRAC_WRAP_Y:
				height += 1
			if size == width*height:
				iGrain = i
		assert(iGrain is not None)
		iFlags = self.map.getMapFractalFlags()
		self.continentsFrac.fracInitHints(self.iNumPlotsX, self.iNumPlotsY, iGrain, self.mapRand, iFlags, self.data, self.fracXExp, self.fracYExp)

	def generatePlotTypes(self, water_percent=-1, shift_plot_types=False):
		for i in range(len(self.data)):
			if self.data[i] is None:
				self.data[i] = self.mapRand.get(48, "Generate Plot Types PYTHON")

		self.__doInitFractal()
		if (water_percent == -1):
			numPlots = len(self.data)
			numWaterPlots = 0
			for val in self.data:
				if val < 192:
					numWaterPlots += 1
			water_percent = int(100*numWaterPlots/numPlots)

		# Call superclass
		return ISFractalWorld.generatePlotTypes(self, water_percent, shift_plot_types)

def generatePlotTypes():
	global hinted_world
	gc = CyGlobalContext()
	map = CyMap()
	mapRand = gc.getGame().getMapRand()

	NiTextOut("Setting Plot Types (Python Inland Sea) ...")

	hinted_world = ISHintedWorld(4,2)
	area = hinted_world.w * hinted_world.h

	for y in range(hinted_world.h):
		for x in range(hinted_world.w):
			if x in (0, hinted_world.w-1) or y in (0, hinted_world.h-1):
				hinted_world.setValue(x, y, 200 + mapRand.get(55, "Plot Types - Inland Sea PYTHON"))
			else:
				hinted_world.setValue(x, y, 0)

	hinted_world.buildAllContinents()
	return hinted_world.generatePlotTypes()

# subclass TerrainGenerator to eliminate arctic, equatorial latitudes

class ISTerrainGenerator(CvMapGeneratorUtil.TerrainGenerator):
	def getLatitudeAtPlot(self, iX, iY):
		"returns 0.0 for tropical, up to 1.0 for polar"
		lat = CvMapGeneratorUtil.TerrainGenerator.getLatitudeAtPlot(self, iX, iY) 	# range [0,1]
		lat = 0.07 + 0.56*lat				# range [0.07, 0.56]
		return lat

def generateTerrainTypes():
	NiTextOut("Generating Terrain (Python Inland Sea) ...")
	terraingen = ISTerrainGenerator()
	terrainTypes = terraingen.generateTerrain()
	return terrainTypes

# subclass FeatureGenerator to eliminate arctic, equatorial latitudes

class ISFeatureGenerator(CvMapGeneratorUtil.FeatureGenerator):
	def getLatitudeAtPlot(self, iX, iY):
		"returns 0.0 for tropical, up to 1.0 for polar"
		lat = CvMapGeneratorUtil.FeatureGenerator.getLatitudeAtPlot(self, iX, iY) 	# range [0,1]
		lat = 0.07 + 0.56*lat				# range [0.07, 0.56]
		return lat

def addFeatures():
	NiTextOut("Adding Features (Python Inland Sea) ...")
	featuregen = ISFeatureGenerator()
	featuregen.addFeatures()
	return 0

def getRiverStartCardinalDirection(argsList):
	pPlot = argsList[0]
	map = CyMap()

	if (pPlot.getY() > ((map.getGridHeight() * 2) / 3)):
		return CardinalDirectionTypes.CARDINALDIRECTION_SOUTH

	if (pPlot.getY() < (map.getGridHeight() / 3)):
		return CardinalDirectionTypes.CARDINALDIRECTION_NORTH

	if (pPlot.getX() > (map.getGridWidth() / 2)):
		return CardinalDirectionTypes.CARDINALDIRECTION_WEST

	return CardinalDirectionTypes.CARDINALDIRECTION_EAST

def getRiverAltitude(argsList):
	pPlot = argsList[0]
	map = CyMap()

	CyPythonMgr().allowDefaultImpl()

	return ((abs(pPlot.getX() - (map.getGridWidth() / 2)) + abs(pPlot.getY() - (map.getGridHeight() / 2))) * 20)
