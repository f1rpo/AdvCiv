#
#	FILE:	 Ring.py
#	AUTHOR:  Bob Thomas (Sirian)
#	CONTRIB: Soren Johnson, Andy Szybalski
#	PURPOSE: Regional map script - One area per player, ring-linked.
#-----------------------------------------------------------------------------
#	Copyright (c) 2005 Firaxis Games, Inc. All rights reserved.
#-----------------------------------------------------------------------------
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from CvPythonExtensions import *
import CvUtil
import CvMapGeneratorUtil
import math
from SAS_WorldSizeUtils import *
from CvMapGeneratorUtil import MultilayeredFractal
from CvMapGeneratorUtil import FractalWorld
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator
from CvMapGeneratorUtil import BonusBalancer
from SASUtils import getInfoTypeOrFail

balancer = BonusBalancer()
bSuccessFlag = True
bUseDefaultStartPlacement = False

def getDescription():
	return "TXT_KEY_MAP_SCRIPT_RING_DESCR"

def isAdvancedMap():
	# <!-- custom: show this map in Simple Game; keep available in Custom Game too. (GPT-5.3-Codex) -->
	return 0

def getNumCustomMapOptions():
	"""
	Number of different user-defined options for this map
	"""
	return 6

def getNumHiddenCustomMapOptions():
	return 2

def getCustomMapOptionName(argsList):
	"""
	Returns name of specified option
	argsList[0] is Option ID (int)
	"""
	[iOption] = argsList
	option_names = {
		0:	"TXT_KEY_MAP_SCRIPT_AREAS_PER_PLAYER",
		1:	"TXT_KEY_MAP_SCRIPT_LAND_SHAPE",
		2:	"TXT_KEY_MAP_SCRIPT_NEUTRAL_TERRITORY",
		3:	"TXT_KEY_MAP_SCRIPT_ISTHMUS_WIDTH",
		4:  "TXT_KEY_MAP_WORLD_WRAP",
		5:  "TXT_KEY_CONCEPT_RESOURCES"		
		}
	if not option_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	translated_text = unicode(CyTranslator().getText(option_names[iOption], ()))
	return translated_text

def getNumCustomMapOptionValues(argsList):
	[iOption] = argsList
	option_values = {
		0:	2,
		1:	3,
		2:	4,
		3:	3,
		4:  3,
		5:  2
		}
	return option_values[iOption]

def getCustomMapOptionDescAt(argsList):
	[iOption, iSelection] = argsList
	selection_names = {
		0:	{
			0: "TXT_KEY_MAP_SCRIPT_1_PER_PLAYER",
			1: "TXT_KEY_MAP_SCRIPT_2_10_PLAYERS_MAX"
			},
		1:	{
			0: "TXT_KEY_MAP_SCRIPT_NATURAL",
			1: "TXT_KEY_MAP_SCRIPT_PRESSED",
			2: "TXT_KEY_MAP_SCRIPT_SOLID"
			},
		2:	{
			0: "TXT_KEY_MAP_SCRIPT_VARIED",
			1: "TXT_KEY_MAP_SCRIPT_PRESSED",
			2: "TXT_KEY_MAP_SCRIPT_NATURAL",
			3: "TXT_KEY_MAP_SCRIPT_ISLANDS"
			},
		3:	{
			0: "TXT_KEY_MAP_SCRIPT_1_PLOT_WIDE",
			1: "TXT_KEY_MAP_SCRIPT_2_PLOTS_WIDE",
			2: "TXT_KEY_MAP_SCRIPT_3_PLOTS_WIDE"
			},
		4:	{
			0: "TXT_KEY_MAP_WRAP_FLAT",
			1: "TXT_KEY_MAP_WRAP_CYLINDER",
			2: "TXT_KEY_MAP_WRAP_TOROID"
			},
		5:	{
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
		1:	-1,
		2:	-1,
		3:	1,
		4:  0,
		5:  0
		}
	return option_defaults[iOption]

def isRandomCustomMapOption(argsList):
	[iOption] = argsList
	option_random = {
		0:	true,
		1:	true,
		2:	true,
		3:  true,
		4:  false,
		5:  false
		}
	return option_random[iOption]

def getWrapX():
	map = CyMap()
	return (map.getCustomMapOption(4) == 1 or map.getCustomMapOption(4) == 2)

def getWrapY():
	map = CyMap()
	return (map.getCustomMapOption(4) == 2)

def normalizeAddExtras():
	if (CyMap().getCustomMapOption(5) == 1):
		balancer.normalizeAddExtras()
	CyPythonMgr().allowDefaultImpl()	# do the rest of the usual normalizeStartingPlots stuff, don't overrride

def addBonusType(argsList):
	[iBonusType] = argsList
	gc = CyGlobalContext()
	type_string = gc.getBonusInfo(iBonusType).getType()

	if (CyMap().getCustomMapOption(5) == 1):
		if (type_string in balancer.resourcesToBalance) or (type_string in balancer.resourcesToEliminate):
			return None # don't place any of this bonus randomly

	CyPythonMgr().allowDefaultImpl() # pretend we didn't implement this method, and let C handle this bonus in the default way

def isBonusIgnoreLatitude():
	return True

def getGridSize(argsList):
	# <!-- custom: Keep Ring's original compact base profile and calibrate only sizes above Huge with shared SAS helpers so ARENA/SAS sizes avoid KeyError. (GPT-5.3-Codex; ChatGPT-5.5) -->
	grid_sizes = {
		WorldSizeTypes.WORLDSIZE_ARENA: (6,4),
		WorldSizeTypes.WORLDSIZE_DUEL: (8,5),
		WorldSizeTypes.WORLDSIZE_TINY: (10,6),
		WorldSizeTypes.WORLDSIZE_SMALL: (13,8),
		WorldSizeTypes.WORLDSIZE_STANDARD: (16,10),
		WorldSizeTypes.WORLDSIZE_LARGE: (21,13),
		WorldSizeTypes.WORLDSIZE_HUGE: (26,16)
	}

	if (argsList[0] == -1): # (-1,) is passed to function on loads
		return []
	[eWorldSize] = argsList
	return sas_lookup_world_size_with_calibrated_sas(eWorldSize, grid_sizes)

def beforeGeneration():
	# Set up land mass data for all players (Square Wheels) then access later.
	gc = CyGlobalContext()
	map = CyMap()
	dice = gc.getGame().getMapRand()
	iW = map.getGridWidth()
	iH = map.getGridHeight()
	iPlayers = gc.getGame().countCivPlayersEverAlive()

	# Success flag. Set to false if number of players falls outside legal range.
	global bSuccessFlag
	global bUseDefaultStartPlacement
	if iPlayers < 2:
		bSuccessFlag = False
		return 0
	else:
		bSuccessFlag = True
	# <!-- custom: Keep Ring map generation active for very high player counts by capping template generation to 18 regions; overflow players use default start placement. (GPT-5.3-Codex) -->
	bUseDefaultStartPlacement = (iPlayers > 18)
	iPlayers = min(iPlayers, 18)

	# Check Ring Type. Set templates.
	userInputLandmass = map.getCustomMapOption(0)
	if userInputLandmass == 1 and iPlayers >= 2 and iPlayers <= 10: # Buffered
		# Templates are nested by keys: {NumRegions: {RegionID: [WestLon, EastLon, SouthLat, NorthLat]}}
		templates = {2: {0: [0.0, 0.333, 0.0, 1.0],
                         1: [0.667, 1.0, 0.0, 1.0]},
                     3: {0: [0.0, 0.4, 0.667, 1.0],
                         1: [0.8, 1.0, 0.333, 1.0],
                         2: [0.2, 0.6, 0.0, 0.333]},
                     4: {0: [0.0, 0.333, 0.333, 0.667],
                         1: [0.333, 0.667, 0.667, 1.0],
                         2: [0.667, 1.0, 0.333, 0.667],
                         3: [0.333, 0.667, 0.0, 0.333]},
                     5: {0: [0.0, 0.167, 0.4, 0.8],
                         1: [0.333, 0.667, 0.8, 1.0],
                         2: [0.833, 1.0, 0.4, 0.8],
                         3: [0.583, 0.917, 0.0, 0.2],
                         4: [0.083, 0.417, 0.0, 0.2]},
                     6: {0: [0.0, 0.333, 0.8, 1.0],
                         1: [0.5, 0.833, 0.8, 1.0],
                         2: [0.833, 1.0, 0.4, 0.8],
                         3: [0.667, 1.0, 0.0, 0.2],
                         4: [0.167, 0.5, 0.0, 0.2],
                         5: [0.0, 0.167, 0.2, 0.6]},
                     7: {0: [0.0, 0.2, 0.75, 1.0],
                         1: [0.4, 0.6, 0.75, 1.0],
                         2: [0.8, 1.0, 0.75, 1.0],
                         3: [0.8, 1.0, 0.25, 0.5],
                         4: [0.6, 0.8, 0.0, 0.25],
                         5: [0.2, 0.4, 0.0, 0.25],
                         6: [0.0, 0.2, 0.25, 0.5]},
                     8: {0: [0.0, 0.2, 0.6, 0.8],
                         1: [0.2, 0.4, 0.8, 1.0],
                         2: [0.6, 0.8, 0.8, 1.0],
                         3: [0.8, 1.0, 0.6, 0.8],
                         4: [0.8, 1.0, 0.2, 0.4],
                         5: [0.6, 0.8, 0.0, 0.2],
                         6: [0.2, 0.4, 0.0, 0.2],
                         7: [0.0, 0.2, 0.2, 0.4]},
                     9: {0: [0.0, 0.167, 0.6, 0.8],
                         1: [0.167, 0.333, 0.8, 1.0],
                         # <!-- custom: The inherited 0.677 east boundary overlapped the adjacent buffer beginning at 0.667; match the symmetric lower region and shared buffer boundary. See KI#292. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
                         2: [0.5, 0.667, 0.8, 1.0],
                         3: [0.833, 1.0, 0.8, 1.0],
                         4: [0.833, 1.0, 0.4, 0.6],
                         5: [0.833, 1.0, 0.0, 0.2],
                         6: [0.5, 0.667, 0.0, 0.2],
                         7: [0.167, 0.333, 0.0, 0.2],
                         8: [0.0, 0.167, 0.2, 0.4]},
                     10: {0: [0.0, 0.167, 0.667, 0.833],
                          1: [0.167, 0.333, 0.833, 1.0],
                          2: [0.5, 0.667, 0.833, 1.0],
                          3: [0.833, 1.0, 0.833, 1.0],
                          4: [0.833, 1.0, 0.5, 0.667],
                          5: [0.833, 1.0, 0.167, 0.333],
                          6: [0.667, 0.833, 0.0, 0.167],
                          7: [0.333, 0.5, 0.0, 0.167],
                          8: [0.0, 0.167, 0.0, 0.167],
                          9: [0.0, 0.167, 0.333, 0.5]}
		}
		buffers = {2: {0: [0.4, 0.6, 0.1, 0.3],
                       1: [0.4, 0.6, 0.7, 0.9]},
                   3: {0: [0.4, 0.8, 0.667, 1.0],
                       1: [0.6, 1.0, 0.0, 0.333],
                       2: [0.0, 0.2, 0.0, 0.667]},
                   4: {0: [0.0, 0.333, 0.667, 1.0],
                       1: [0.667, 1.0, 0.667, 1.0],
                       2: [0.667, 1.0, 0.0, 0.333],
                       3: [0.0, 0.333, 0.0, 0.333]},
                   5: {0: [0.0, 0.167, 0.2, 0.4],
                       1: [0.167, 0.333, 0.8, 1.0],
                       2: [0.667, 0.833, 0.8, 1.0],
                       3: [0.833, 1.0, 0.2, 0.4],
                       4: [0.417, 0.583, 0.0, 0.2]},
                   6: {0: [0.0, 0.167, 0.6, 0.8],
                       1: [0.333, 0.5, 0.8, 1.0],
                       2: [0.833, 1.0, 0.8, 1.0],
                       3: [0.833, 1.0, 0.2, 0.4],
                       4: [0.5, 0.667, 0.0, 0.2],
                       5: [0.0, 0.167, 0.0, 0.2]},
                   7: {0: [0.2, 0.4, 0.75, 1.0],
                       1: [0.6, 0.8, 0.75, 1.0],
                       2: [0.8, 1.0, 0.5, 0.75],
                       3: [0.8, 1.0, 0.0, 0.25],
                       4: [0.4, 0.6, 0.0, 0.25],
                       5: [0.0, 0.2, 0.0, 0.25],
                       6: [0.0, 0.2, 0.5, 0.75]},
                   8: {0: [0.0, 0.2, 0.8, 1.0],
                       1: [0.4, 0.6, 0.8, 1.0],
                       2: [0.8, 1.0, 0.8, 1.0],
                       3: [0.8, 1.0, 0.4, 0.6],
                       4: [0.8, 1.0, 0.0, 0.2],
                       5: [0.4, 0.6, 0.0, 0.2],
                       6: [0.0, 0.2, 0.0, 0.2],
                       7: [0.0, 0.2, 0.4, 0.6]},
                   9: {0: [0.0, 0.167, 0.8, 1.0],
                       1: [0.333, 0.5, 0.8, 1.0],
                       2: [0.667, 0.833, 0.8, 1.0],
                       3: [0.833, 1.0, 0.6, 0.8],
                       4: [0.833, 1.0, 0.2, 0.4],
                       5: [0.667, 0.833, 0.0, 0.2],
                       6: [0.333, 0.5, 0.0, 0.2],
                       7: [0.0, 0.167, 0.0, 0.2],
                       8: [0.0, 0.167, 0.4, 0.6]},
                   10: {0: [0.0, 0.167, 0.5, 0.667],
                        1: [0.0, 0.167, 0.833, 1.0],
                        2: [0.333, 0.5, 0.833, 1.0],
                        3: [0.667, 0.833, 0.833, 1.0],
                        4: [0.833, 1.0, 0.667, 0.833],
                        5: [0.833, 1.0, 0.333, 0.5],
                        6: [0.833, 1.0, 0.0, 0.167],
                        7: [0.5, 0.667, 0.0, 0.167],
                        8: [0.167, 0.333, 0.0, 0.167],
                        9: [0.0, 0.167, 0.167, 0.333]}
		}
		center_isle = {2: [0.4, 0.6, 0.4, 0.6],
                       3: [0.28, 0.72, 0.4, 0.6],
                       4: [0.4, 0.6, 0.4, 0.6],
                       5: [0.27, 0.73, 0.3, 0.7],
                       6: [0.25, 0.75, 0.3, 0.7],
                       7: [0.3, 0.7, 0.35, 0.65],
                       8: [0.32, 0.68, 0.32, 0.68],
                       9: [0.25, 0.75, 0.3, 0.7],
                       10: [0.25, 0.75, 0.25, 0.75]
		}
		# The wheel is a series of coordinates, line segments drawn from one to the next. [Lon0, Lat0], [Lon1, Lat1], [Lon2, Lat2], etc.
		the_wheel = {2: [[0.167, 0.2], [0.167, 0.8], [0.833, 0.8], [0.833, 0.2]],
                     3: [[0.1, 0.167], [0.1, 0.833], [0.9, 0.833], [0.9, 0.167]],
                     4: [[0.167, 0.167], [0.167, 0.833], [0.833, 0.833], [0.833, 0.167]],
                     5: [[0.083, 0.7], [0.25, 0.9], [0.75, 0.9], [0.917, 0.7], [0.917, 0.3], [0.833, 0.1], [0.167, 0.1], [0.083, 0.3]],
                     6: [[0.083, 0.1], [0.083, 0.9], [0.917, 0.9], [0.917, 0.1]],
                     7: [[0.1, 0.125], [0.1, 0.875], [0.9, 0.875], [0.9, 0.125]],
                     8: [[0.1, 0.1], [0.1, 0.9], [0.9, 0.9], [0.9, 0.1]],
                     9: [[0.083, 0.1], [0.083, 0.9], [0.917, 0.9], [0.917, 0.1]],
                     10: [[0.083, 0.083], [0.083, 0.917], [0.917, 0.917], [0.917, 0.083]]
		}

	else: # Normal.
		# Templates are nested by keys: {NumRegions: {RegionID: [WestLon, EastLon, SouthLat, NorthLat]}}
		templates = {2: {0: [0.0, 0.333, 0.0, 1.0],
                         1: [0.667, 1.0, 0.0, 1.0]},
                     3: {0: [0.0, 0.25, 0.25, 0.75],
                         1: [0.5, 1.0, 0.75, 1.0],
                         2: [0.5, 1.0, 0.0, 0.25]},
                     4: {0: [0.0, 0.333, 0.333, 0.667],
                         1: [0.333, 0.667, 0.667, 1.0],
                         2: [0.667, 1.0, 0.333, 0.667],
                         3: [0.333, 0.667, 0.0, 0.333]},
                     5: {0: [0.0, 0.25, 0.333, 1.0],
                         1: [0.25, 0.75, 0.667, 1.0],
                         2: [0.75, 1.0, 0.333, 1.0],
                         3: [0.5, 1.0, 0.0, 0.333],
                         4: [0.0, 0.5, 0.0, 0.333]},
                     6: {0: [0.0, 0.4, 0.667, 1.0],
                         1: [0.4, 0.8, 0.667, 1.0],
                         2: [0.8, 1.0, 0.333, 1.0],
                         3: [0.6, 1.0, 0.0, 0.333],
                         4: [0.2, 0.6, 0.0, 0.333],
                         5: [0.0, 0.2, 0.0, 0.667]},
                     7: {0: [0.0, 0.4, 0.75, 1.0],
                         1: [0.4, 0.8, 0.75, 1.0],
                         2: [0.8, 1.0, 0.5, 1.0],
                         3: [0.8, 1.0, 0.0, 0.5],
                         4: [0.4, 0.8, 0.0, 0.25],
                         5: [0.0, 0.4, 0.0, 0.25],
                         6: [0.0, 0.2, 0.25, 0.75]},
                     8: {0: [0.0, 0.3, 0.35, 0.65],
                         1: [0.0, 0.3, 0.7, 1.0],
                         2: [0.35, 0.65, 0.7, 1.0],
                         3: [0.7, 1.0, 0.7, 1.0],
                         4: [0.7, 1.0, 0.35, 0.65],
                         5: [0.7, 1.0, 0.0, 0.3],
                         6: [0.35, 0.65, 0.0, 0.3],
                         7: [0.0, 0.3, 0.0, 0.3]},
                     9: {0: [0.0, 0.167, 0.6, 1.0],
                         1: [0.167, 0.5, 0.8, 1.0],
                         2: [0.5, 0.833, 0.8, 1.0],
                         3: [0.833, 1.0, 0.6, 1.0],
                         4: [0.833, 1.0, 0.2, 0.6],
                         5: [0.667, 1.0, 0.0, 0.2],
                         6: [0.333, 0.667, 0.0, 0.2],
                         7: [0.0, 0.333, 0.0, 0.2],
                         8: [0.0, 0.167, 0.2, 0.6]},
                     10: {0: [0.0, 0.167, 0.5, 0.833],
                          1: [0.0, 0.333, 0.833, 1.0],
                          2: [0.333, 0.667, 0.833, 1.0],
                          3: [0.667, 1.0, 0.833, 1.0],
                          4: [0.833, 1.0, 0.5, 0.833],
                          5: [0.833, 1.0, 0.167, 0.5],
                          6: [0.667, 1.0, 0.0, 0.167],
                          7: [0.333, 0.667, 0.0, 0.167],
                          8: [0.0, 0.333, 0.0, 0.167],
                          9: [0.0, 0.167, 0.167, 0.5]},
                     11: {0: [0.0, 0.2, 0.167, 0.5],
                          1: [0.0, 0.2, 0.5, 0.833],
                          2: [0.2, 0.4, 0.667, 1.0],
                          3: [0.4, 0.6, 0.667, 1.0],
                          4: [0.6, 0.8, 0.667, 1.0],
                          5: [0.8, 1.0, 0.667, 1.0],
                          6: [0.8, 1.0, 0.333, 0.667],
                          7: [0.8, 1.0, 0.0, 0.333],
                          8: [0.6, 0.8, 0.0, 0.333],
                          9: [0.4, 0.6, 0.0, 0.333],
                          10: [0.2, 0.4, 0.0, 0.333]},
                     12: {0: [0.0, 0.2, 0.333, 0.667],
                          1: [0.0, 0.2, 0.667, 1.0],
                          2: [0.2, 0.4, 0.667, 1.0],
                          3: [0.4, 0.6, 0.667, 1.0],
                          4: [0.6, 0.8, 0.667, 1.0],
                          5: [0.8, 1.0, 0.667, 1.0],
                          6: [0.8, 1.0, 0.333, 0.667],
                          7: [0.8, 1.0, 0.0, 0.333],
                          8: [0.6, 0.8, 0.0, 0.333],
                          9: [0.4, 0.6, 0.0, 0.333],
                          10: [0.2, 0.4, 0.0, 0.333],
                          11: [0.0, 0.2, 0.0, 0.333]},
                     13: {0: [0.0, 0.2, 0.375, 0.625],
                          1: [0.0, 0.2, 0.7, 0.95],
                          2: [0.2, 0.4, 0.75, 1.0],
                          3: [0.4, 0.6, 0.75, 1.0],
                          4: [0.6, 0.8, 0.75, 1.0],
                          5: [0.8, 1.0, 0.75, 1.0],
                          6: [0.8, 1.0, 0.5, 0.75],
                          7: [0.8, 1.0, 0.25, 0.5],
                          8: [0.8, 1.0, 0.0, 0.25],
                          9: [0.6, 0.8, 0.0, 0.25],
                          10: [0.4, 0.6, 0.0, 0.25],
                          11: [0.2, 0.4, 0.0, 0.25],
                          12: [0.0, 0.2, 0.05, 0.3]},
                     14: {0: [0.0, 0.2, 0.5, 0.75],
                          1: [0.0, 0.2, 0.75, 1.0],
                          2: [0.2, 0.4, 0.75, 1.0],
                          3: [0.4, 0.6, 0.75, 1.0],
                          4: [0.6, 0.8, 0.75, 1.0],
                          5: [0.8, 1.0, 0.75, 1.0],
                          6: [0.8, 1.0, 0.5, 0.75],
                          7: [0.8, 1.0, 0.25, 0.5],
                          8: [0.8, 1.0, 0.0, 0.25],
                          9: [0.6, 0.8, 0.0, 0.25],
                          10: [0.4, 0.6, 0.0, 0.25],
                          11: [0.2, 0.4, 0.0, 0.25],
                          12: [0.0, 0.2, 0.0, 0.25],
                          13: [0.0, 0.2, 0.25, 0.5]},
                     15: {0: [0.0, 0.167, 0.375, 0.625],
                          1: [0.0, 0.167, 0.7, 0.95],
                          2: [0.167, 0.333, 0.75, 1.0],
                          3: [0.333, 0.5, 0.75, 1.0],
                          4: [0.5, 0.667, 0.75, 1.0],
                          5: [0.667, 0.833, 0.75, 1.0],
                          6: [0.833, 1.0, 0.75, 1.0],
                          7: [0.833, 1.0, 0.5, 0.75],
                          8: [0.833, 1.0, 0.25, 0.5],
                          9: [0.833, 1.0, 0.0, 0.25],
                          10: [0.667, 0.833, 0.0, 0.25],
                          11: [0.5, 0.667, 0.0, 0.25],
                          12: [0.333, 0.5, 0.0, 0.25],
                          13: [0.167, 0.333, 0.0, 0.25],
                          14: [0.0, 0.167, 0.05, 0.3]},
                     16: {0: [0.0, 0.167, 0.5, 0.75],
                          1: [0.0, 0.167, 0.75, 1.0],
                          2: [0.167, 0.333, 0.75, 1.0],
                          3: [0.333, 0.5, 0.75, 1.0],
                          4: [0.5, 0.667, 0.75, 1.0],
                          5: [0.667, 0.833, 0.75, 1.0],
                          6: [0.833, 1.0, 0.75, 1.0],
                          7: [0.833, 1.0, 0.5, 0.75],
                          8: [0.833, 1.0, 0.25, 0.5],
                          9: [0.833, 1.0, 0.0, 0.25],
                          10: [0.667, 0.833, 0.0, 0.25],
                          11: [0.5, 0.667, 0.0, 0.25],
                          12: [0.333, 0.5, 0.0, 0.25],
                          13: [0.167, 0.333, 0.0, 0.25],
                          14: [0.0, 0.167, 0.25, 0.5],
                          15: [0.0, 0.167, 0.0, 0.25]},
                     17: {0: [0.0, 0.167, 0.5, 0.7],
                          1: [0.0, 0.167, 0.7, 0.9],
                          2: [0.167, 0.333, 0.8, 1.0],
                          3: [0.333, 0.5, 0.8, 1.0],
                          4: [0.5, 0.667, 0.8, 1.0],
                          5: [0.667, 0.833, 0.8, 1.0],
                          6: [0.833, 1.0, 0.8, 1.0],
                          7: [0.833, 1.0, 0.6, 0.8],
                          8: [0.833, 1.0, 0.4, 0.6],
                          9: [0.833, 1.0, 0.2, 0.4],
                          10: [0.833, 1.0, 0.0, 0.2],
                          11: [0.667, 0.833, 0.0, 0.2],
                          12: [0.5, 0.667, 0.0, 0.2],
                          13: [0.333, 0.5, 0.0, 0.2],
                          14: [0.167, 0.333, 0.0, 0.2],
                          15: [0.0, 0.167, 0.1, 0.3],
                          16: [0.0, 0.167, 0.3, 0.5]},
                     18: {0: [0.0, 0.167, 0.6, 0.8],
                          1: [0.0, 0.167, 0.8, 1.0],
                          2: [0.167, 0.333, 0.8, 1.0],
                          3: [0.333, 0.5, 0.8, 1.0],
                          4: [0.5, 0.667, 0.8, 1.0],
                          5: [0.667, 0.833, 0.8, 1.0],
                          6: [0.833, 1.0, 0.8, 1.0],
                          7: [0.833, 1.0, 0.6, 0.8],
                          8: [0.833, 1.0, 0.4, 0.6],
                          9: [0.833, 1.0, 0.2, 0.4],
                          10: [0.833, 1.0, 0.0, 0.2],
                          11: [0.667, 0.833, 0.0, 0.2],
                          12: [0.5, 0.667, 0.0, 0.2],
                          13: [0.333, 0.5, 0.0, 0.2],
                          14: [0.167, 0.333, 0.0, 0.2],
                          15: [0.0, 0.167, 0.0, 0.2],
                          16: [0.0, 0.167, 0.2, 0.4],
                          17: [0.0, 0.167, 0.4, 0.6]}
		}
		center_isle = {2: [0.4, 0.6, 0.4, 0.6],
                       3: [0.4, 0.7, 0.35, 0.65],
                       4: [0.4, 0.6, 0.4, 0.6],
                       5: [0.32, 0.68, 0.4, 0.6],
                       6: [0.28, 0.72, 0.4, 0.6],
                       7: [0.28, 0.72, 0.35, 0.65],
                       8: [0.4, 0.6, 0.4, 0.6],
                       9: [0.3, 0.7, 0.35, 0.65],
                       10: [0.3, 0.7, 0.3, 0.7],
                       11: [0.28, 0.72, 0.4, 0.6],
                       12: [0.28, 0.72, 0.4, 0.6],
                       13: [0.3, 0.7, 0.35, 0.65],
                       14: [0.3, 0.7, 0.35, 0.65],
                       15: [0.25, 0.75, 0.35, 0.65],
                       16: [0.25, 0.75, 0.35, 0.65],
                       17: [0.25, 0.75, 0.3, 0.7],
                       18: [0.25, 0.75, 0.3, 0.7]
		}
		# The wheel is a series of coordinates, line segments drawn from one to the next. [Lon0, Lat0], [Lon1, Lat1], [Lon2, Lat2], etc.
		the_wheel = {2: [[0.167, 0.3], [0.167, 0.7], [0.833, 0.7], [0.833, 0.3]],
                     3: [[0.125, 0.375], [0.125, 0.625], [0.625, 0.875], [0.875, 0.875], [0.875, 0.125], [0.625, 0.125]],
                     4: [[0.167, 0.5], [0.5, 0.833], [0.833, 0.5], [0.5, 0.167]],
                     5: [[0.125, 0.167], [0.125, 0.833], [0.875, 0.833], [0.875, 0.167]],
                     6: [[0.1, 0.167], [0.1, 0.833], [0.9, 0.833], [0.9, 0.167]],
                     7: [[0.1, 0.125], [0.1, 0.875], [0.9, 0.875], [0.9, 0.125]],
                     8: [[0.167, 0.167], [0.167, 0.833], [0.833, 0.833], [0.833, 0.167]],
                     9: [[0.083, 0.1], [0.083, 0.9], [0.917, 0.9], [0.917, 0.1]],
                     10: [[0.083, 0.083], [0.083, 0.917], [0.917, 0.917], [0.917, 0.083]],
                     11: [[0.1, 0.333], [0.1, 0.667], [0.3, 0.833], [0.9, 0.833], [0.9, 0.167], [0.3, 0.167]],
                     12: [[0.1, 0.167], [0.1, 0.833], [0.9, 0.833], [0.9, 0.167]],
                     13: [[0.1, 0.175], [0.1, 0.825], [0.3, 0.875], [0.9, 0.875], [0.9, 0.125], [0.3, 0.125]],
                     14: [[0.1, 0.125], [0.1, 0.875], [0.9, 0.875], [0.9, 0.125]],
                     15: [[0.083, 0.175], [0.083, 0.825], [0.25, 0.875], [0.917, 0.875], [0.917, 0.125], [0.25, 0.125]],
                     16: [[0.083, 0.125], [0.083, 0.875], [0.917, 0.875], [0.917, 0.125]],
                     17: [[0.083, 0.2], [0.083, 0.8], [0.25, 0.9], [0.917, 0.9], [0.917, 0.1], [0.25, 0.1]],
                     18: [[0.083, 0.1], [0.083, 0.9], [0.917, 0.9], [0.917, 0.1]]
		}
		# End of Templates data.

	# List region_data: [WestX, EastX, SouthY, NorthY]
	global region_data
	region_data = []
	for regionLoop in range(iPlayers):
		# Region dimensions
		[fWestLon, fEastLon, fSouthLat, fNorthLat] = templates[iPlayers][regionLoop]
		iWestX = int((iW - 1) * fWestLon)
		iEastX = int((iW - 1) * fEastLon) - 1
		iSouthY = int((iH - 1) * fSouthLat)
		iNorthY = int((iH - 1) * fNorthLat) -1
		region_data.append([iWestX, iEastX, iSouthY, iNorthY])

	# If Buffered, list buffer_data: [WestX, EastX, SouthY, NorthY]
	if userInputLandmass == 1 and iPlayers >= 2 and iPlayers <= 10:
		global buffer_data
		buffer_data = []
		for bufferLoop in range(iPlayers):
			# Buffer dimensions
			[fWestLon, fEastLon, fSouthLat, fNorthLat] = buffers[iPlayers][bufferLoop]
			iWestX = int((iW - 1) * fWestLon)
			iEastX = int((iW - 1) * fEastLon) - 1
			iSouthY = int((iH - 1) * fSouthLat)
			iNorthY = int((iH - 1) * fNorthLat) -1
			buffer_data.append([iWestX, iEastX, iSouthY, iNorthY])
	else:
		pass

	# center_data: [WestX, EastX, SouthY, NorthY]
	global center_data
	[fWestLon, fEastLon, fSouthLat, fNorthLat] = center_isle[iPlayers]
	iWestX = int((iW - 1) * fWestLon)
	iEastX = int((iW - 1) * fEastLon) - 1
	iSouthY = int((iH - 1) * fSouthLat)
	iNorthY = int((iH - 1) * fNorthLat) -1
	center_data = [iWestX, iEastX, iSouthY, iNorthY]

	# List wheel_data: [[X0, Y0], [X1, Y1], [X2, Y2], etc]
	global wheel_data
	wheel_data = []
	for [fLon, fLat] in the_wheel[iPlayers]:
		iX = int((iW - 1) * fLon)
		iY = int((iH - 1) * fLat)
		wheel_data.append([iX, iY])
	# Return to start plot, completing the wheel.
	[fLon, fLat] = the_wheel[iPlayers][0]
	iX = int((iW - 1) * fLon)
	iY = int((iH - 1) * fLat)
	wheel_data.append([iX, iY])

	# Successfully generated regions, continue back to C++
	return 0

class RingMultilayeredFractal(CvMapGeneratorUtil.MultilayeredFractal):
	# Override InRegion to increase hills, make up for those lost to the ring path.
	def generatePlotsInRegion(self, iWaterPercent, iRegionWidth, iRegionHeight, iRegionWestX, iRegionSouthY, iRegionGrain, iRegionHillsGrain, iRegionPlotFlags, iRegionTerrainFlags, iRegionFracXExp = -1, iRegionFracYExp = -1, bShift = True, iStrip = 15, rift_grain = -1, has_center_rift = False, invert_heights = False):
		# This is the code to generate each fractal.
		# Determine and pass in the appropriate arguments from the controlling function.
		#
		# Init local variables
		water = iWaterPercent
		iWestX = iRegionWestX
		# Note: if you pass bad regional dimensions so that iEastX > self.iW, BOOM! So don't do that. I could close out that possibility, but better that I not, so that you get an error to warn you of erroneous regional parameters. - Sirian
		iSouthY = iRegionSouthY

		# Init the plot types array and the regional fractals
		self.plotTypes = [] # reinit the array for each pass
		self.plotTypes = [PlotTypes.PLOT_OCEAN] * (iRegionWidth*iRegionHeight)
		regionContinentsFrac = CyFractal()
		regionHillsFrac = CyFractal()
		regionPeaksFrac = CyFractal()
		regionContinentsFrac.fracInit(iRegionWidth, iRegionHeight, iRegionGrain, self.dice, iRegionPlotFlags, iRegionFracXExp, iRegionFracYExp)
		regionHillsFrac.fracInit(iRegionWidth, iRegionHeight, iRegionHillsGrain, self.dice, iRegionTerrainFlags, iRegionFracXExp, iRegionFracYExp)
		regionPeaksFrac.fracInit(iRegionWidth, iRegionHeight, iRegionHillsGrain+1, self.dice, iRegionTerrainFlags, iRegionFracXExp, iRegionFracYExp)

		iWaterThreshold = regionContinentsFrac.getHeightFromPercent(water)
		iHillsBottom1 = regionHillsFrac.getHeightFromPercent(max((23 - self.gc.getClimateInfo(self.map.getClimate()).getHillRange()), 0))
		iHillsTop1 = regionHillsFrac.getHeightFromPercent(min((30 + self.gc.getClimateInfo(self.map.getClimate()).getHillRange()), 100))
		iHillsBottom2 = regionHillsFrac.getHeightFromPercent(max((70 - self.gc.getClimateInfo(self.map.getClimate()).getHillRange()), 0))
		iHillsTop2 = regionHillsFrac.getHeightFromPercent(min((77 + self.gc.getClimateInfo(self.map.getClimate()).getHillRange()), 100))
		iPeakThreshold = regionPeaksFrac.getHeightFromPercent(self.gc.getClimateInfo(self.map.getClimate()).getPeakPercent())

		# Loop through the region's plots
		for x in range(iRegionWidth):
			for y in range(iRegionHeight):
				i = y*iRegionWidth + x
				val = regionContinentsFrac.getHeight(x,y)
				if val <= iWaterThreshold:
					pass
				else:
					hillVal = regionHillsFrac.getHeight(x,y)
					if ((hillVal >= iHillsBottom1 and hillVal <= iHillsTop1) or (hillVal >= iHillsBottom2 and hillVal <= iHillsTop2)):
						peakVal = regionPeaksFrac.getHeight(x,y)
						if (peakVal <= iPeakThreshold):
							self.plotTypes[i] = PlotTypes.PLOT_PEAK
						else:
							self.plotTypes[i] = PlotTypes.PLOT_HILLS
					else:
						self.plotTypes[i] = PlotTypes.PLOT_LAND

		if bShift:
			# Shift plots to obtain a more natural shape.
			self.shiftRegionPlots(iRegionWidth, iRegionHeight, iStrip)

		# Once the plot types for the region have been generated, they must be
		# applied to the global plot array.
		#
		# Default approach is to ignore water and layer the lands over one another.
		# If you want to layer the water, too, or some other combination, then
		# create a subclass and override this function. Customize in your override.
		#
		# Apply the region's plots to the global plot array.
		for x in range(iRegionWidth):
			wholeworldX = x + iWestX
			for y in range(iRegionHeight):
				i = y*iRegionWidth + x
				if self.plotTypes[i] == PlotTypes.PLOT_OCEAN:
					continue
				wholeworldY = y + iSouthY
				iWorld = wholeworldY*self.iW + wholeworldX
				self.wholeworldPlotTypes[iWorld] = self.plotTypes[i]

		# This region is done.
		return

	def generatePlotsByRegion(self, ring_type):
		# Sirian's MultilayeredFractal class, controlling function.
		# You -MUST- customize this function for each use of the class.
		iPlayers = self.gc.getGame().countCivPlayersEverAlive()
		global region_data
		iPlayers = min(iPlayers, len(region_data))
		userInputAreaType = self.map.getCustomMapOption(1)
		area_grain = 3 - userInputAreaType
		userInputBufferType = self.map.getCustomMapOption(2)
		buffer_one = [0, 58, 64, 70]
		bufWater = buffer_one[userInputBufferType]
		grainType = userInputBufferType

		# Sea Level adjustment (from user input), limited to value of 5%.
		sea = self.gc.getSeaLevelInfo(self.map.getSeaLevel()).getSeaLevelChange()
		sea = min(sea, 5)
		sea = max(sea, -5)
		bufWater += sea

		# If Buffered map, add buffers (one fractal each).

		if ring_type == 1 and iPlayers < 11 and iPlayers > 1: # Buffered
			# One buffer per player.
			global buffer_data
			for buffer_loop in range(iPlayers):
				[bufWestX, bufEastX, bufSouthY, bufNorthY] = buffer_data[buffer_loop]
				if grainType == 0:
					thisBuffer = 1 + self.dice.get(3, "Random Buffer Type - Ring PYTHON")
					bufWater = sea + buffer_one[thisBuffer]
					bufGrain = thisBuffer + 1
				else:
					bufGrain = grainType + 1

				# Generate the buffer.
				bufWidth = bufEastX - bufWestX + 1
				bufHeight = bufNorthY - bufSouthY + 1
				self.generatePlotsInRegion(bufWater, bufWidth, bufHeight, bufWestX, bufSouthY, bufGrain, 3, self.iRoundFlags, self.iTerrainFlags, 6, 6, True, 3, -1, False, False)
		else:
			pass

		# Add players' regions (two fractals each to ensure cohesion).
		for region_loop in range(iPlayers):
			[regWestX, regEastX, regSouthY, regNorthY] = region_data[region_loop]
			# Main fractal
			regWidth = regEastX - regWestX + 1
			regHeight = regNorthY - regSouthY + 1
			self.generatePlotsInRegion(55 + sea, regWidth, regHeight, regWestX, regSouthY, area_grain, 4, self.iRoundFlags, self.iTerrainFlags, 6, 6, True, 3, -1, False, False)

			# Core fractal to increase cohesion
			coreWestX = regWestX + int(regWidth * 0.25)
			coreEastX = regEastX - int(regWidth * 0.25)
			coreSouthY = regSouthY + int(regHeight * 0.25)
			coreNorthY = regNorthY - int(regHeight * 0.25)
			coreWidth = coreEastX - coreWestX + 1
			coreHeight = coreNorthY - coreSouthY + 1
			self.generatePlotsInRegion(65, coreWidth, coreHeight, coreWestX, coreSouthY, 1, 3, self.iHorzFlags, self.iTerrainFlags, 5, 5, True, 3, -1, False, False)

		# Add center isle(s).
		global center_data
		[cenWestX, cenEastX, cenSouthY, cenNorthY] = center_data
		cenWidth = cenEastX - cenWestX + 1
		cenHeight = cenNorthY - cenSouthY + 1
		if grainType == 0:
			thisBuffer = 1 + self.dice.get(3, "Random Buffer Type - Ring PYTHON")
			bufWater = sea + buffer_one[thisBuffer]
			bufGrain = thisBuffer + 1
		else:
			bufGrain = grainType + 1
		self.generatePlotsInRegion(bufWater, cenWidth, cenHeight, cenWestX, cenSouthY, bufGrain, 4, self.iRoundFlags, self.iTerrainFlags, -1, -1, True, 5, -1, False, False)

		# Draw the wheel to connect all players.
		global wheel_data
		linewidth = 1 + self.map.getCustomMapOption(3)
		iWH = self.iW * self.iH
		offsetstart = 0 - int(linewidth/2)
		offsetrange = range(offsetstart, offsetstart + linewidth)

		for wheel_loop in range(len(wheel_data) - 1):
			[startx, starty] = wheel_data[wheel_loop]
			[endx, endy] = wheel_data[wheel_loop + 1]

			if abs(endy-starty) < abs(endx-startx):
				# line is closer to horizontal
				if startx > endx:
					startx, starty, endx, endy = endx, endy, startx, starty # swap start and end
				dx = endx-startx
				dy = endy-starty
				if dx == 0 or dy == 0:
					slope = 0
				else:
					slope = float(dy)/float(dx)
				y = starty
				for x in range(startx, endx+1):
					for offset in offsetrange:
						if map.isPlot(x, int(round(y+offset))):
							i = map.plotNum(x, int(round(y+offset)))
							self.wholeworldPlotTypes[i] = PlotTypes.PLOT_LAND
					y += slope
			else:
				# line is closer to vertical
				if starty > endy:
					startx, starty, endx, endy = endx, endy, startx, starty # swap start and end
				dx, dy = endx-startx, endy-starty
				if dx == 0 or dy == 0:
					slope = 0
				else:
					slope = float(dx)/float(dy)
				x = startx
				for y in range(starty, endy+1):
					for offset in offsetrange:
						if map.isPlot(int(round(x+offset)), y):
							i = map.plotNum(int(round(x+offset)), y)
							self.wholeworldPlotTypes[i] = PlotTypes.PLOT_LAND
					x += slope

		# All regions have been processed. Plot Type generation completed.
		return self.wholeworldPlotTypes

'''
Regional Variables Key:

iWaterPercent,
iRegionWidth, iRegionHeight,
iRegionWestX, iRegionSouthY,
iRegionGrain, iRegionHillsGrain,
iRegionPlotFlags, iRegionTerrainFlags,
iRegionFracXExp, iRegionFracYExp,
bShift, iStrip,
rift_grain, has_center_rift,
invert_heights
'''

def generatePlotTypes():
	NiTextOut("Setting Plot Types (Python Ring) ...")
	gc = CyGlobalContext()
	global map
	map = CyMap()

	# Check for valid number of players.
	global bSuccessFlag
	if bSuccessFlag:
		pass
	else:
		fractal_world = FractalWorld()
		fractal_world.initFractal(polar = True)
		return fractal_world.generatePlotTypes()

	# Player count is valid, produce the map.
	fractal_world = RingMultilayeredFractal()
	userInputLandmass = map.getCustomMapOption(0)
	if userInputLandmass == 1: # Buffered.
		ring_type = 1
	else: # Normal
		ring_type = 0
	return fractal_world.generatePlotsByRegion(ring_type)

class RingTerrainGenerator(CvMapGeneratorUtil.TerrainGenerator):
	"If iDesertPercent=35, then about 35% of all land will be desert. Plains is similar. \
	Note that all percentages are approximate, as values have to be roughened to achieve a natural look."

	def __init__(self, iDesertPercent=20, iPlainsPercent=25, iIcePercent=50, iTundraPercent=35, fracXExp=-1, fracYExp=-1, grain_amount=5):

		self.gc = CyGlobalContext()
		self.map = CyMap()

		global center_data
		[self.cenWestX, self.cenEastX, self.cenSouthY, self.cenNorthY] = center_data

		grain_amount += self.gc.getWorldInfo(self.map.getWorldSize()).getTerrainGrainChange()

		self.grain_amount = grain_amount

		self.iWidth = self.map.getGridWidth()
		self.iHeight = self.map.getGridHeight()

		self.mapRand = self.gc.getGame().getMapRand()

		self.iFlags = 0  # Disallow FRAC_POLAR flag, to prevent "zero row" problems.
		if self.map.isWrapX():
			self.iFlags += CyFractal.FracVals.FRAC_WRAP_X
		if self.map.isWrapY():
			self.iFlags += CyFractal.FracVals.FRAC_WRAP_Y

		self.main=CyFractal()
		self.center=CyFractal()

		iDesertPercent += self.gc.getClimateInfo(self.map.getClimate()).getDesertPercentChange()
		if self.gc.getClimateInfo(self.map.getClimate()).getRandIceLatitude() > 0.25: # Cold Climate
			iIcePercent += 10
			iTundraPercent += 5
			iPlainsPercent += 25
		if self.gc.getClimateInfo(self.map.getClimate()).getJungleLatitude() < 5: # Tropical Climate
			iIcePercent = 15
			iTundraPercent = 20

		self.iDesertBottomPercent = 100 - iDesertPercent
		self.iPlainsBottomPercent = self.iDesertBottomPercent - iPlainsPercent
		self.iIceBottomPercent = 100 - iIcePercent
		self.iTundraBottomPercent = self.iIceBottomPercent - iTundraPercent

		self.fracXExp = fracXExp
		self.fracYExp = fracYExp

		self.initFractals()

	def initFractals(self):
		self.main.fracInit(self.iWidth, self.iHeight, self.grain_amount, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
		self.iDesertBottom = self.main.getHeightFromPercent(self.iDesertBottomPercent)
		self.iPlainsBottom = self.main.getHeightFromPercent(self.iPlainsBottomPercent)

		self.center.fracInit(self.iWidth, self.iHeight, self.grain_amount, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
		self.iIceBottom = self.center.getHeightFromPercent(self.iIceBottomPercent)
		self.iTundraBottom = self.center.getHeightFromPercent(self.iTundraBottomPercent)

		self.terrainDesert = getInfoTypeOrFail("TERRAIN_DESERT")
		self.terrainPlains = getInfoTypeOrFail("TERRAIN_PLAINS")
		self.terrainIce = getInfoTypeOrFail("TERRAIN_SNOW")
		self.terrainTundra = getInfoTypeOrFail("TERRAIN_TUNDRA")
		self.terrainGrass = getInfoTypeOrFail("TERRAIN_GRASS")

	def generateTerrainAtPlot(self,iX,iY):
		if (self.map.plot(iX, iY).isWater()):
			return self.map.plot(iX, iY).getTerrainType()

		if ((iX >= self.cenWestX and iX <= self.cenEastX) and (iY >= self.cenSouthY and iY <= self.cenNorthY)):
			# Center region
			val = self.center.getHeight(iX, iY)
			if val >= self.iIceBottom:
				terrainVal = self.terrainIce
			elif val >= self.iTundraBottom:
				terrainVal = self.terrainTundra
			else:
				terrainVal = self.terrainPlains
		else:
			# Main region
			val = self.main.getHeight(iX, iY)
			if val >= self.iDesertBottom:
				terrainVal = self.terrainDesert
			elif val >= self.iPlainsBottom:
				terrainVal = self.terrainPlains
			else:
				terrainVal = self.terrainGrass

		if (terrainVal == TerrainTypes.NO_TERRAIN):
			return self.map.plot(iX, iY).getTerrainType()

		return terrainVal

def generateTerrainTypes():
	global bSuccessFlag
	if bSuccessFlag == False:
		CyPythonMgr().allowDefaultImpl()
		return
	NiTextOut("Generating Terrain (Python Ring) ...")
	terraingen = RingTerrainGenerator()
	terrainTypes = terraingen.generateTerrain()
	return terrainTypes

class RingFeatureGenerator(CvMapGeneratorUtil.FeatureGenerator):
	def __init__(self, iJunglePercent=25, iForestPercent=40, forest_grain=6, fracXExp=-1, fracYExp=-1):

		self.gc = CyGlobalContext()
		self.map = CyMap()
		self.mapRand = self.gc.getGame().getMapRand()
		self.forests = CyFractal()
		global center_data
		[self.cenWestX, self.cenEastX, self.cenSouthY, self.cenNorthY] = center_data

		self.iFlags = 0  # Disallow FRAC_POLAR flag, to prevent "zero row" problems.
		if self.map.isWrapX():
			self.iFlags += CyFractal.FracVals.FRAC_WRAP_X
		if self.map.isWrapY():
			self.iFlags += CyFractal.FracVals.FRAC_WRAP_Y

		self.iGridW = self.map.getGridWidth()
		self.iGridH = self.map.getGridHeight()
		# <!-- custom: Inherited east/north formulas mixed the center coordinate with the full far-edge coordinate, placing those transition boundaries inside the Snowy center and eliminating the Evergreen bands.
		# Mirror the west/south 70% distance calculation from the far edges. See KI#287. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		iMaxX = self.iGridW - 1
		iMaxY = self.iGridH - 1
		self.pineWestX = int(self.cenWestX * 0.7)
		self.pineEastX = iMaxX - int((iMaxX - self.cenEastX) * 0.7)
		self.pineSouthY = int(self.cenSouthY * 0.7)
		self.pineNorthY = iMaxY - int((iMaxY - self.cenNorthY) * 0.7)

		self.iJunglePercent = iJunglePercent
		self.iForestPercent = iForestPercent
		self.iIceChance = 15
		if self.gc.getClimateInfo(self.map.getClimate()).getJungleLatitude() < 5:
			self.iJunglePercent += 25 # Double jungles if Tropical Climate
			self.iForestPercent -= 10 # Reduce forests if Tropical
			self.iIceChance = 30

		if self.gc.getClimateInfo(self.map.getClimate()).getRandIceLatitude() > 0.25:
			self.iIceChance = 6 # Add more ice if Cold Climate
			self.iJunglePercent -= 10 # Reduce jungles if Cold Climate
			self.iForestPercent += 20 # Add forests if Cold

		if self.gc.getClimateInfo(self.map.getClimate()).getDesertPercentChange() >= 10:
			self.iJunglePercent = 15 # Reduce foliage if Arid Climate
			self.iForestPercent = 20

		forest_grain += self.gc.getWorldInfo(self.map.getWorldSize()).getFeatureGrainChange()

		self.forest_grain = forest_grain

		self.fracXExp = fracXExp
		self.fracYExp = fracYExp

		self.__initFractals()
		self.__initFeatureTypes()

	def __initFractals(self):
		self.forests.fracInit(self.iGridW, self.iGridH, self.forest_grain, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)

		self.iJungleBottom = self.forests.getHeightFromPercent(100 - self.iJunglePercent)
		self.iForestLevel = self.forests.getHeightFromPercent(self.iForestPercent)

	def __initFeatureTypes(self):
		self.featureIce = getInfoTypeOrFail("FEATURE_ICE")
		self.featureJungle = getInfoTypeOrFail("FEATURE_JUNGLE")
		self.featureForest = getInfoTypeOrFail("FEATURE_FOREST")
		self.featureOasis = getInfoTypeOrFail("FEATURE_OASIS")

	def addFeaturesAtPlot(self, iX, iY):
		"adds any appropriate features at the plot (iX, iY) where (0,0) is in the SW"
		pPlot = self.map.sPlot(iX, iY)

		for iI in range(self.gc.getNumFeatureInfos()):
			if pPlot.canHaveFeature(iI):
				if self.mapRand.get(10000, "Add Feature PYTHON") < self.gc.getFeatureInfo(iI).getAppearanceProbability():
					pPlot.setFeatureType(iI, -1)

		if (pPlot.getFeatureType() == FeatureTypes.NO_FEATURE):
			self.addIceAtPlot(pPlot, iX, iY)

		if (pPlot.getFeatureType() == FeatureTypes.NO_FEATURE):
			self.addJunglesAtPlot(pPlot, iX, iY)

		if (pPlot.getFeatureType() == FeatureTypes.NO_FEATURE):
			self.addForestsAtPlot(pPlot, iX, iY)

	def addIceAtPlot(self, pPlot, iX, iY):
		if pPlot.isWater() and ((iX >= self.cenWestX and iX <= self.cenEastX) and (iY >= self.cenSouthY and iY <= self.cenNorthY)):
			rand = self.mapRand.get(self.iIceChance, "Add Ice PYTHON")
			if rand == 4:
				pPlot.setFeatureType(self.featureIce, -1)

	def addJunglesAtPlot(self, pPlot, iX, iY):
		# Warning! This version of Jungles is using the forest fractal!
		if pPlot.canHaveFeature(self.featureJungle) and not ((iX >= self.cenWestX and iX <= self.cenEastX) and (iY >= self.cenSouthY and iY <= self.cenNorthY)):
			iJungleHeight = self.forests.getHeight(iX, iY)
			if iJungleHeight >= self.iJungleBottom:
				pPlot.setFeatureType(self.featureJungle, -1)

	def addForestsAtPlot(self, pPlot, iX, iY):
		if pPlot.canHaveFeature(self.featureForest):
			if self.forests.getHeight(iX, iY) <= self.iForestLevel:
				if ((iX >= self.cenWestX and iX <= self.cenEastX) and (iY >= self.cenSouthY and iY <= self.cenNorthY)):
					forestVar = 2
				elif (iX <= self.pineWestX or iX >= self.pineEastX or iY <= self.pineSouthY or iY >= self.pineNorthY):
					forestVar = 0
				else:
					forestVar = 1
				pPlot.setFeatureType(self.featureForest, forestVar)

def addFeatures():
	global bSuccessFlag
	if bSuccessFlag == False:
		CyPythonMgr().allowDefaultImpl()
		return
	NiTextOut("Adding Features (Python Ring) ...")
	featuregen = RingFeatureGenerator()
	featuregen.addFeatures()
	return 0

def assignStartingPlots():
	global bSuccessFlag
	global bUseDefaultStartPlacement
	if bSuccessFlag == False or bUseDefaultStartPlacement == True:
		CyPythonMgr().allowDefaultImpl()
		return

	global start_plots
	global iPlotShift
	global shuffledPlayers
	global region_data
	gc = CyGlobalContext()
	map = CyMap()
	dice = gc.getGame().getMapRand()
	iPlayers = gc.getGame().countCivPlayersEverAlive()
	start_plots = []
	shuffledPlayers = []

	# Set variance for start plots according to map size vs number of players.
	map_size = map.getWorldSize()
	# <!-- custom: Use SAS worldsizes so Ring supports ARENA/SAS sizes and unknown future sizes without KeyError. (GPT-5.3-Codex; ChatGPT-5.5) -->
	sizevalues = {
		WorldSizeTypes.WORLDSIZE_ARENA: (2, 3),
		WorldSizeTypes.WORLDSIZE_DUEL: (2, 3),
		WorldSizeTypes.WORLDSIZE_TINY: (2, 4),
		WorldSizeTypes.WORLDSIZE_SMALL: (3, 6),
		WorldSizeTypes.WORLDSIZE_STANDARD: (4, 10),
		WorldSizeTypes.WORLDSIZE_LARGE: (6, 12),
		WorldSizeTypes.WORLDSIZE_HUGE: (6, 18),
		WorldSizeTypes.WORLDSIZE_SAS24: (7, 20),
		WorldSizeTypes.WORLDSIZE_SAS32: (8, 24),
		WorldSizeTypes.WORLDSIZE_SAS40: (9, 28),
		WorldSizeTypes.WORLDSIZE_SAS48: (10, 32)
		}
	(threeVar, twoVar) = sas_lookup_world_size(map_size, sizevalues)
	if iPlayers <= threeVar:
		iPlotShift = 3
	elif iPlayers <= twoVar:
		iPlotShift = 2
	else:
		iPlotShift = 1

	# Assign start plots for each player area.
	for region_loop in range(iPlayers):
		[regWestX, regEastX, regSouthY, regNorthY] = region_data[region_loop]
		# Main fractal
		regWidth = regEastX - regWestX + 1
		regHeight = regNorthY - regSouthY + 1
		iStartX = regWestX + int(regWidth / 2)
		iStartY = regSouthY + int(regHeight / 2)
		start_plots.append([iStartX, iStartY])

	# <!-- custom: Ring used compact list positions as real PlayerTypes, so sparse/high-ID players lost their handcrafted regions.
	# Assign shuffled template IDs directly to the actual ever-alive player IDs. See KI#267. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	player_list = range(iPlayers)
	shuffledPlayers = {}
	for playerLoop in range(gc.getMAX_CIV_PLAYERS()):
		if gc.getPlayer(playerLoop).isEverAlive():
			iChoosePlayer = dice.get(len(player_list), "Shuffling Template IDs - Ring PYTHON")
			shuffledPlayers[playerLoop] = player_list[iChoosePlayer]
			del player_list[iChoosePlayer]

	# Done setting up variables. Proceed to normal process.
	CyPythonMgr().allowDefaultImpl()

def minStartingDistanceModifier():
	global bSuccessFlag
	global bUseDefaultStartPlacement
	if bSuccessFlag == True and bUseDefaultStartPlacement == False:
		return -95
	else:
		return -50

def findStartingPlot(argsList):
	[playerID] = argsList

	# Set up for maximum of 18 players! If more, use default implementation.
	global bSuccessFlag
	global bUseDefaultStartPlacement
	if bSuccessFlag == False or bUseDefaultStartPlacement == True:
		return CvMapGeneratorUtil.findStartingPlot(playerID)

	def isValid(playerID, x, y):
		gc = CyGlobalContext()
		map = CyMap()
		pPlot = map.plot(x, y)
		# Check to ensure the plot is on the main landmass.
		if (pPlot.getArea() != map.findBiggestArea(False).getID()):
			return false

		iW = map.getGridWidth()
		iH = map.getGridHeight()
		iPlayers = gc.getGame().countCivPlayersEverAlive()
		global start_plots
		global iPlotShift
		global shuffledPlayers
		# <!-- custom: The repaired mapping is keyed by actual PlayerTypes rather than compact positions; reject only a genuinely unmapped player. See KI#267. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if not shuffledPlayers.has_key(playerID):
			return false
		playerTemplateAssignment = shuffledPlayers[playerID]
		if playerTemplateAssignment < 0 or playerTemplateAssignment >= len(start_plots):
			return false
		[iStartX, iStartY] = start_plots[playerTemplateAssignment]

		# Now check for eligibility according to the defintions found in the template.
		westX = max(2, iStartX - iPlotShift)
		eastX = min(iW - 3, iStartX + iPlotShift)
		southY = max(2, iStartY - iPlotShift)
		northY = min(iH - 3, iStartY + iPlotShift)
		if x < westX or x > eastX or y < southY or y > northY:
			return false
		else:
			return true

	return CvMapGeneratorUtil.findStartingPlot(playerID, isValid)

def normalizeRemovePeaks():
	return None

def normalizeRemoveBadTerrain():
	return None

def normalizeAddGoodTerrain():
	return None
