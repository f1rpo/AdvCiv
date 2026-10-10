#
#	FILE:	 Grid.py
#	AUTHOR:  Axius
#	CONTRIB: Bob Thomas (Sirian)
#	PURPOSE: Global map script - Generates a grid of connected islands.
#-----------------------------------------------------------------------------
#	Portions Copyright (c) 2005 Firaxis Games, Inc.
#-----------------------------------------------------------------------------
#
#
# 2017 - Penny - Expanded into Grid perfect
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Adapted from Beyond the Game mod map script, version 2.43. (GPT-5.3-Codex) -->

from CvPythonExtensions import *
import CvUtil
import CvMapGeneratorUtil
import sys
import random
from CvMapGeneratorUtil import FractalWorld
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator
from CvMapGeneratorUtil import BonusBalancer
from SASUtils import getInfoTypeOrFail
from SAS_WorldSizeUtils import *

balancer = BonusBalancer()
balancer.resourcesToBalance = ("BONUS_ALUMINUM", "BONUS_COPPER", "BONUS_HORSE", "BONUS_IRON", "BONUS_OIL", "BONUS_URANIUM")
balancer.resourcesToEliminate = ("", )

def getDescription():
	return "Modified version of the BTG_Grid map from the Beyond the Game mod by Penny. Adapted for AdvCiv-SAS: upscaled, non-AdvCiv options removed, and SAS48/high player counts supported."

def getDescriptionTitle():
	return "Each player has its own hub protected and linked via a band of land in all 4 cardinal directions to other players"

def getDescriptionTitleTwo():
	return ""

def getDescriptionMain():
	return ""

def getDescriptionSecond():#Script tip : (on TOP)
	return "Your hub shape and starting positon location are mirrored (default), beware to be accurate on ennemy distance"	

def getDescriptionThird():#Option : (at the bottom)"
	return "Know well the amount of lines you're using in the game. Use 1 not 2, if you have an uneven number of players"	

def getDescriptionScenario():#Scenario : (at the bottom)"
	return "Megamap for Ironman. 9 players in 3x3 a must. Other numbers good too. Also used for 3v3 Modern"	

def getDescriptionBalance():#Balance : (at the bottom)"
	return "16 Tiles away is close but 20 is large. When playing with 1 band of land, use 16 of height for sure"	

def isAdvancedMap():
	# <!-- custom: Keep BTG_Grid visible in Simple Game to match other SAS-adapted BTG maps. (GPT-5.3-Codex) -->
	return 0

def getNumCustomMapOptions():
	return 10

def getNumHiddenCustomMapOptions():
	return 0

def getCustomMapOptionName(argsList):
	[iOption] = argsList
	option_names = {
		0:	"TXT_KEY_MAP_WORLD_WRAP",
		1:	"Mirrored Hubs",
		2:	"TXT_KEY_MAP_SCRIPT_SPOKE_WIDTH",
		3:	"Lines Count",
		4:	"Precious Metal",
		5:	"Empty Land",
		6:	"Forest Density",
		7:	"Start Distance",
		8:	"Desert",
		9: "Starting Units"
		}
	if not option_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	translated_text = unicode(CyTranslator().getText(option_names[iOption], ()))
	return translated_text

def getNumCustomMapOptionValues(argsList):
	[iOption] = argsList
	option_values = {
		0:	3,
		1:	4,
		2:	6,
		3:	3,
		4:	2,
		5:	2,
		6:	4,
		7:	3,
		8:	2,
		9:	2
		}
	if not option_values.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	return option_values[iOption]

def getCustomMapOptionDescAt(argsList):
	[iOption, iSelection] = argsList
	selection_names = {
		0:	{
			0: "TXT_KEY_MAP_WRAP_FLAT",
			1: "TXT_KEY_MAP_WRAP_CYLINDER",
			2: "TXT_KEY_MAP_WRAP_TOROID"
			},
		1:	{
			0: "No",
			1: "Yes - Shape only (not position)",
			2: "Yes - Shape and position",
			3: "Yes - Mirrored"
			},
		2:	{
			0: "TXT_KEY_MAIN_MENU_NONE",
			1: "TXT_KEY_MAP_SCRIPT_1_PLOT_WIDE",
			2: "TXT_KEY_MAP_SCRIPT_2_PLOTS_WIDE",
			3: "TXT_KEY_MAP_SCRIPT_3_PLOTS_WIDE",
			4: "TXT_KEY_MAP_SCRIPT_4_PLOTS_WIDE",
			5: "TXT_KEY_MAP_SCRIPT_5_PLOTS_WIDE"			
			},
		3:	{
			0: "1 Line  Flat",
			1: "2 Lines Rectangle",
			2: "3 Lines Tall"
			},
		4:	{
			0: "Standard",
			1: "Within 7 tiles"
			},
		5:	{
			0: "Yes - Leave empty ocean when no player",
			1: "No - Fill in the space as if there would be someone"
			},
		6:	{
			0: "High - 60% (Game base)",
			1: "Standard - 40% (map base)",
			2: "Scarce - 25%",
			3: "Rare - 10%"
			},
		7:	{
			0: "Normal - 16 Tiles away",
			1: "Far - 20 Tiles away",
			2: "Far Mixed - 20 Tiles Horizontal, 16 Vertical"
			},	
		8:	{
			0: "Normal",
			1: "None - replace by grassland"
			},	
		9:	{
			0: "Normal - Scattered",
			1: "Special - Together Same Tile"
			}
		}
	if not selection_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	if not selection_names[iOption].has_key(iSelection):
		return u""
	translated_text = unicode(CyTranslator().getText(selection_names[iOption][iSelection], ()))
	return translated_text

def getCustomMapOptionDefault(argsList):
	[iOption] = argsList
	option_defaults = {
		0:	2,
		1:	2,
		2:	3,
		3:	2,
		4:	0,
		5:	0,
		6:	1,
		7:	0,
		8:	0,
		9: 0
		}
	if not option_defaults.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	return option_defaults[iOption]

def isRandomCustomMapOption(argsList):
	[iOption] = argsList
	option_random = {
		0:	false,
		1:	false,
		2:	false,
		3:	false,
		4:	true,
		5:	true,
		6:	true,
		7:	true,
		8:	true,
		9:	true
		}
	if not option_random.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	return option_random[iOption]

def getWrapX():
	map = CyMap()
	return (map.getCustomMapOption(0) == 1 or map.getCustomMapOption(0) == 2)

def getWrapY():
	map = CyMap()
	return (map.getCustomMapOption(0) == 2)

# <!-- custom: Generic SAS calibration preserves aspect ratio, but Grid's selectable 1/2/3-row geometry needs fixed height and additional horizontal columns as player count grows.
# Use a Grid-specific lookup so its dynamic hubs retain Huge's per-hub area. See KI#253. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
def lookupGridSizeWithSASRows(eWorldSize, grid_sizes):
	key = sas_find_world_size_key(eWorldSize, grid_sizes)
	if key is not None:
		return grid_sizes[key]
	iAnchorWorldSize = sas_largest_world_size_key(grid_sizes)
	iWorldSize = int(eWorldSize)
	if iWorldSize > int(iAnchorWorldSize) and iWorldSize <= int(WorldSizeTypes.WORLDSIZE_SAS48):
		(iAnchorWidth, iAnchorHeight) = grid_sizes[iAnchorWorldSize]
		iAnchorPlayers = sas_world_default_players(iAnchorWorldSize)
		iTargetPlayers = sas_world_default_players(iWorldSize)
		# <!-- custom: Grid keeps a fixed 1/2/3-row layout, so SAS sizes must add horizontal columns rather than change the anchor's aspect ratio; this preserves its per-hub area while dynamic templates add players. See KI#253. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		iTargetWidth = max(1, int((float(iAnchorWidth) * iTargetPlayers / iAnchorPlayers) + 0.5))
		return (iTargetWidth, iAnchorHeight)
	return grid_sizes[iAnchorWorldSize]

def getGridSize(argsList):
	if (argsList[0] == -1): # (-1,) is passed to function on loads
		return []

	#Traditional way
	# <!-- custom: Land Size option removed; keep default size mode only. (GPT-5.3-Codex) -->
	# <!-- custom: Use DLL WorldSizeTypes values aligned with CIV4WorldInfo.xml order. (Claude code Opus 4.7; GPT-5.5) -->
	if True:
		# Section 1 - if  default option "16 tiles" is clicked
		# <!-- custom: Start Distance is option 7; reading Empty Land (option 5) made the advertised distance choices select the wrong grid dimensions. See KI#250. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if (CyMap().getCustomMapOption(7) == 0):
			if (CyMap().getCustomMapOption(3) == 0):
				grid_sizes = {
					WorldSizeTypes.WORLDSIZE_ARENA:		(6,4),
					WorldSizeTypes.WORLDSIZE_DUEL:		(8,4),
					WorldSizeTypes.WORLDSIZE_TINY:		(12,4),
					WorldSizeTypes.WORLDSIZE_SMALL:		(16,4),
					WorldSizeTypes.WORLDSIZE_STANDARD:		(21,4),
					WorldSizeTypes.WORLDSIZE_LARGE:		(23,4),
					WorldSizeTypes.WORLDSIZE_HUGE:		(26,4)
				}
			elif (CyMap().getCustomMapOption(3) == 1):
				grid_sizes = {
					WorldSizeTypes.WORLDSIZE_ARENA:		(6,4),
					WorldSizeTypes.WORLDSIZE_DUEL:		(8,4),
					WorldSizeTypes.WORLDSIZE_TINY:		(8,8),
					WorldSizeTypes.WORLDSIZE_SMALL:		(12,8),
					WorldSizeTypes.WORLDSIZE_STANDARD:		(16,8),
					WorldSizeTypes.WORLDSIZE_LARGE:		(21,8),
					WorldSizeTypes.WORLDSIZE_HUGE:		(23,8)
				}	
			else:
				grid_sizes = {
					WorldSizeTypes.WORLDSIZE_ARENA:		(4,6),
					WorldSizeTypes.WORLDSIZE_DUEL:		(4,8),
					WorldSizeTypes.WORLDSIZE_TINY:		(4,12),
					WorldSizeTypes.WORLDSIZE_SMALL:		(8,12),
					WorldSizeTypes.WORLDSIZE_STANDARD:		(12,12),
					WorldSizeTypes.WORLDSIZE_LARGE:		(16,12),
					WorldSizeTypes.WORLDSIZE_HUGE:		(21,12)# not set up
				}

		elif (CyMap().getCustomMapOption(7) == 1): # Section 2 - if the map is clicked for larger (20)
			if (CyMap().getCustomMapOption(3) == 0):
				grid_sizes = {
					WorldSizeTypes.WORLDSIZE_ARENA:		(8,5),
					WorldSizeTypes.WORLDSIZE_DUEL:		(10,5),
					WorldSizeTypes.WORLDSIZE_TINY:		(15,5),
					WorldSizeTypes.WORLDSIZE_SMALL:		(20,5),
					WorldSizeTypes.WORLDSIZE_STANDARD:		(26,5),
					WorldSizeTypes.WORLDSIZE_LARGE:		(29,5),
					WorldSizeTypes.WORLDSIZE_HUGE:		(32,5)
				}
			elif (CyMap().getCustomMapOption(3) == 1):
				grid_sizes = {
					WorldSizeTypes.WORLDSIZE_ARENA:		(8,5),
					WorldSizeTypes.WORLDSIZE_DUEL:		(10,5),
					WorldSizeTypes.WORLDSIZE_TINY:		(10,10),
					WorldSizeTypes.WORLDSIZE_SMALL:		(15,10),
					WorldSizeTypes.WORLDSIZE_STANDARD:		(20,10),
					WorldSizeTypes.WORLDSIZE_LARGE:		(26,10),
					WorldSizeTypes.WORLDSIZE_HUGE:		(29,10)
				}	
			else:
				grid_sizes = {
					WorldSizeTypes.WORLDSIZE_ARENA:		(5,8),
					WorldSizeTypes.WORLDSIZE_DUEL:		(5,10),
					WorldSizeTypes.WORLDSIZE_TINY:		(5,15),
					WorldSizeTypes.WORLDSIZE_SMALL:		(10,15),
					WorldSizeTypes.WORLDSIZE_STANDARD:		(15,15),
					WorldSizeTypes.WORLDSIZE_LARGE:		(20,15),
					WorldSizeTypes.WORLDSIZE_HUGE:		(26,15)
				}

		else : # Section 3 - if 20x16
			if (CyMap().getCustomMapOption(3) == 0):
				grid_sizes = {
					WorldSizeTypes.WORLDSIZE_ARENA:		(8,4),
					WorldSizeTypes.WORLDSIZE_DUEL:		(10,4),
					WorldSizeTypes.WORLDSIZE_TINY:		(15,4),
					WorldSizeTypes.WORLDSIZE_SMALL:		(20,4),
					WorldSizeTypes.WORLDSIZE_STANDARD:		(26,4),
					WorldSizeTypes.WORLDSIZE_LARGE:		(29,4),
					WorldSizeTypes.WORLDSIZE_HUGE:		(32,4)
				}
			elif (CyMap().getCustomMapOption(3) == 1):
				grid_sizes = {
					WorldSizeTypes.WORLDSIZE_ARENA:		(8,4),
					WorldSizeTypes.WORLDSIZE_DUEL:		(10,4),
					WorldSizeTypes.WORLDSIZE_TINY:		(10,8),
					WorldSizeTypes.WORLDSIZE_SMALL:		(15,8),
					WorldSizeTypes.WORLDSIZE_STANDARD:		(20,8),
					WorldSizeTypes.WORLDSIZE_LARGE:		(26,8),
					WorldSizeTypes.WORLDSIZE_HUGE:		(29,8)
				}	
			else:
				grid_sizes = {
					WorldSizeTypes.WORLDSIZE_ARENA:		(4,8),
					WorldSizeTypes.WORLDSIZE_DUEL:		(4,10),
					WorldSizeTypes.WORLDSIZE_TINY:		(4,15),
					WorldSizeTypes.WORLDSIZE_SMALL:		(10,12),
					WorldSizeTypes.WORLDSIZE_STANDARD:		(15,12),
					WorldSizeTypes.WORLDSIZE_LARGE:		(20,12),
					WorldSizeTypes.WORLDSIZE_HUGE:		(26,12)
				}			

		[eWorldSize] = argsList
		# <!-- custom: Keep existing BTG base-size tables and extend SAS tiers from Huge according to the fixed row layout. See KI#253. (GPT-5.3-Codex; ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		grid_size = lookupGridSizeWithSASRows(eWorldSize, grid_sizes)

	return grid_size

# <!-- custom: Static BTG templates stop below the player counts advertised by AdvCiv-SAS.
# Build equal rectangular rows for every missing capacity so each supported player can still receive a distinct hub. See KI#253. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
def buildGridTemplate(iRegions, iRows):
	iColumns = iRegions // iRows
	template = {}
	iRegion = 0
	for iRow in range(iRows):
		fSouth = float(iRow) / iRows
		fNorth = float(iRow + 1) / iRows
		for iColumn in range(iColumns):
			fWest = float(iColumn) / iColumns
			fEast = float(iColumn + 1) / iColumns
			template[iRegion] = [fWest, fEast, fSouth, fNorth]
			iRegion += 1
	return template

def beforeGeneration():
	global iNumRegions
	global iNumRegionsForShape
	global regions_in_use
	global remaining_regions
	global remaining_regionsTwo
	# <!-- custom: These mirroring globals persist with the loaded map-script module and must be shared with generation/normalization callbacks. See KI#252. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	global bGridRegionalGenerationCompleted
	global region_duplicated
	global other_regions

	# <!-- custom: Reset all generated-region and mirroring state before each map so fallback paths cannot read undefined or earlier-map geometry. See KI#252. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	bGridRegionalGenerationCompleted = False
	region_duplicated = []
	other_regions = []
	regions_in_use = []
	remaining_regions = []
	remaining_regionsTwo = []

	#2.22
	global isBTPon
	try:
		isBTPon = CvMapGeneratorUtil.BTGInfo().BTG_Version() > 0
	except:
		isBTPon = False

	gc = CyGlobalContext()
	map = CyMap()
	dice = gc.getGame().getMapRand()
	iW = map.getGridWidth()
	iH = map.getGridHeight()
	iPlayersCount = gc.getGame().countCivPlayersEverAlive()
	global iPlayers

	# <!-- custom: Derive row capacity from the real player count instead of clamping at 18; missing static layouts are generated below. See KI#253. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (CyMap().getCustomMapOption(3) == 0):
		iRows = 1
		iNumRegions = iPlayersCount
	elif (CyMap().getCustomMapOption(3) == 1):	
		iRows = 2
		iNumRegions = ((iPlayersCount + 1) / 2) * 2
	else:	
		iRows = max(1, min(iPlayersCount, 3))
		if iPlayersCount < 3:
			iNumRegions = iPlayersCount
		else:
			iNumRegions = ((iPlayersCount + 2) / 3) * 3

	# Do the real player count now, trick for spoiling number of player / region
	if (CyMap().getCustomMapOption(5) == 0):
		iPlayers = iNumRegions # 2.22 - Imagine it's the Template 3x2 for 5 players.
		iNumRegionsForShape = iNumRegions
		# 2.22 - then You need to override to have an empty region
		iNumRegions = min(gc.getGame().countCivPlayersEverAlive(), iNumRegionsForShape)
	else:
		iPlayers = gc.getGame().countCivPlayersEverAlive()
		iNumRegionsForShape = iNumRegions
	iNumRegions = min(iNumRegions, iNumRegionsForShape)
	# <!-- custom: Dynamic templates removed the former 18-player ceiling; only a generation with no players needs the generic fallback. See KI#253. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if iPlayers < 1:
		return None

	# Some regions may go unused. We need to track the ones that have been used.
	for loopy in range(iNumRegionsForShape):
		remaining_regions.append(loopy)
		remaining_regionsTwo.append(loopy)

	# Templates are nested by keys: {NumRegions: {RegionID: [WestLon, EastLon, SouthLat, NorthLat]}}
	if (CyMap().getCustomMapOption(3) == 0):
		templates = {2: {0: [0.0, 0.5, 0.0, 1.0],
					1: [0.5, 1.0, 0.0, 1.0]},
				3: {0: [0.0, 0.333, 0.0, 1.0],
					1: [0.333, 0.666, 0.0, 1.0],
					2: [0.666, 1.0, 0.0, 1.0]},
				4: {0: [0.0, 0.25, 0.0, 1.0],
					1: [0.25, 0.5, 0.0, 1.0],
					2: [0.5, 0.75, 0.0, 1.0],
					3: [0.75, 1.0, 0.0, 1.0]},
				5: {0: [0.0, 0.2, 0.0, 1.0],
					1: [0.2, 0.4, 0.0, 1.0],
					2: [0.4, 0.6, 0.0, 1.0],
					3: [0.6, 0.8, 0.0, 1.0],
					4: [0.8, 1.0, 0.0, 1.0]},
				6: {0: [0, 0.166, 0.0, 1.0],
					1: [0.166, 0.333, 0.0, 1.0],
					2: [0.333, 0.5, 0.0, 1.0],
					3: [0.5, 0.666, 0.0, 1.0],
					4: [0.666, 0.833, 0.0, 1.0],
					5: [0.833, 1.0, 0.0, 1.0]},
				7: {0: [0, 0.142, 0.0, 1.0],
					1: [0.141, 0.285, 0.0, 1.0],
					2: [0.282, 0.428, 0.0, 1.0],
					3: [0.428, 0.571, 0.0, 1.0],
					4: [0.571, 0.705, 0.0, 1.0],
					5: [0.705, 0.857, 0.0, 1.0],
					6: [0.857, 1.0, 0.0, 1.0]},
				8: {0: [0.000, 0.125, 0.0, 1.0],
					1: [0.125, 0.250, 0.0, 1.0],
					2: [0.250, 0.375, 0.0, 1.0],
					3: [0.375, 0.500, 0.0, 1.0],
					4: [0.500, 0.625, 0.0, 1.0],
					5: [0.625, 0.750, 0.0, 1.0],
					6: [0.750, 0.875, 0.0, 1.0],
					7: [0.875, 1.000, 0.0, 1.0]},
				9: {0: [0.000, 0.111, 0.0, 1.0],
					1: [0.111, 0.222, 0.0, 1.0],
					2: [0.222, 0.333, 0.0, 1.0],
					3: [0.333, 0.444, 0.0, 1.0],
					4: [0.444, 0.555, 0.0, 1.0],
					# <!-- custom: BTG's west boundary was 0.755, producing the only inverted rectangle in the template set. See KI#248. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
					5: [0.555, 0.666, 0.0, 1.0],
					6: [0.666, 0.777, 0.0, 1.0],
					7: [0.777, 0.888, 0.0, 1.0],
					8: [0.888, 1.000, 0.0, 1.0]},
				10: {0: [0.000, 0.100, 0.0, 1.0],
					1: [0.100, 0.200, 0.0, 1.0],
					2: [0.200, 0.300, 0.0, 1.0],
					3: [0.300, 0.400, 0.0, 1.0],
					4: [0.400, 0.500, 0.0, 1.0],
					5: [0.500, 0.600, 0.0, 1.0],
					6: [0.600, 0.700, 0.0, 1.0],
					7: [0.700, 0.800, 0.0, 1.0],
					8: [0.800, 0.900, 0.0, 1.0],
					9: [0.900, 1.000, 0.0, 1.0]},
		}
	elif (CyMap().getCustomMapOption(3) == 1):	
		templates = {2: {0: [0.0, 0.5, 0.0, 1.0],
					1: [0.5, 1.0, 0.0, 1.0]},
				4: {0: [0.0, 0.5, 0.0, 0.5],
					1: [0.5, 1.0, 0.0, 0.5],
					2: [0.0, 0.5, 0.5, 1.0],
					3: [0.5, 1.0, 0.5, 1.0]},
				6: {0: [0.0, 0.333, 0.0, 0.5],
					1: [0.333, 0.667, 0.0, 0.5],
					2: [0.667, 1.0, 0.0, 0.5],
					3: [0.0, 0.333, 0.5, 1.0],
					4: [0.333, 0.667, 0.5, 1.0],
					5: [0.667, 1.0, 0.5, 1.0]},
				8: {0: [0.0, 0.25, 0.0, 0.5],
					1: [0.25, 0.5, 0.0, 0.5],
					2: [0.5, 0.75, 0.0, 0.5],
					3: [0.75, 1.0, 0.0, 0.5],
					4: [0.0, 0.25, 0.5, 1.0],
					5: [0.25, 0.5, 0.5, 1.0],
					6: [0.5, 0.75, 0.5, 1.0],
					7: [0.75, 1.0, 0.5, 1.0]},
				10: {0:[0.0, 0.2, 0.0, 0.5],
					1: [0.2, 0.4, 0.0, 0.5],
					2: [0.4, 0.6, 0.0, 0.5],
					3: [0.6, 0.8, 0.0, 0.5],
					4: [0.8, 1.0, 0.0, 0.5],
					5: [0.0, 0.2, 0.5, 1.0],
					6: [0.2, 0.4, 0.5, 1.0],
					7: [0.4, 0.6, 0.5, 1.0],
					8: [0.6, 0.8, 0.5, 1.0],
					9: [0.8, 1.0, 0.5, 1.0]},
				12: {0: [0.0, 0.166, 0.0, 0.5],
						1: [0.166, 0.333, 0.0, 0.5],
						2: [0.333, 0.5, 0.0, 0.5],
						3: [0.5, 0.666, 0.0, 0.5],
						4: [0.666, 0.833, 0.0, 0.5],
						5: [0.833, 1.0, 0.0, 0.5],
						6: [0.0, 0.166, 0.50, 1.0],
						7: [0.166, 0.333, 0.50, 1.0],
						8: [0.333, 0.5, 0.50, 1.0],
						9: [0.5, 0.666, 0.50, 1.0],
						10: [0.666, 0.833, 0.50, 1.0],
						11: [0.833, 1.0, 0.50, 1.0]},
		}
	else:
		templates =  {2: {0: [0.0, 1.0, 0.0, 0.5],
					1: [0.0, 1.0, 0.5, 1.0]},
				3: {0: [0.0, 1.0, 0.0, 0.334],
					1: [0.0, 1.0, 0.334, 0.667],
					2: [0.0, 1.0, 0.667, 1.0]},
				6: {0: [0.0, 0.5, 0.0, 0.334],
					1: [0.5, 1.0, 0.0, 0.334],
					2: [0.0, 0.5, 0.334, 0.667],
					3: [0.5, 1.0, 0.334, 0.667],
					4: [0.0, 0.5, 0.667, 1.0],
					5: [0.5, 1.0, 0.667, 1.0]},
				9: {0: [0.0, 0.334, 0.0, 0.334],
					1: [0.334, 0.667, 0.0, 0.334],
					2: [0.667, 1.0, 0.0, 0.334],
					3: [0.0, 0.334, 0.334, 0.667],
					4: [0.334, 0.667, 0.334, 0.667],
					5: [0.667, 1.0, 0.334, 0.667],
					6: [0.0, 0.334, 0.667, 1.0],
					7: [0.334, 0.667, 0.667, 1.0],
					8: [0.667, 1.0, 0.667, 1.0]},
				12: {0: [0.0, 0.25, 0.0, 0.334],
						1: [0.25, 0.50, 0.0, 0.334],
						2: [0.50, 0.75, 0.0, 0.334],
						3: [0.75, 1.0, 0.0, 0.334],
						4: [0.0, 0.25, 0.334, 0.667],
						5: [0.25, 0.50, 0.334, 0.667],
						6: [0.50, 0.75, 0.334, 0.667],
						7: [0.75, 1.0, 0.334, 0.667],
						8: [0.0, 0.25, 0.667, 1.0],
						9: [0.25, 0.50, 0.667, 1.0],
						10: [0.50, 0.75, 0.667, 1.0],
						11: [0.75, 1.0, 0.667, 1.0]},
				15: {0: [0.0, 0.2, 0.0, 0.334],
						1: [0.2, 0.4, 0.0, 0.334],
						2: [0.4, 0.6, 0.0, 0.334],
						3: [0.6, 0.8, 0.0, 0.334],
						4: [0.8, 1.0, 0.0, 0.334],
						5: [0.0, 0.2, 0.334, 0.667],
						6: [0.2, 0.4, 0.334, 0.667],
						7: [0.4, 0.6, 0.334, 0.667],
						8: [0.6, 0.8, 0.334, 0.667],
						9: [0.8, 1.0, 0.334, 0.667],
						10: [0.0, 0.2, 0.667, 1.0],
						11: [0.2, 0.4, 0.667, 1.0],
						12: [0.4, 0.6, 0.667, 1.0],
						13: [0.6, 0.8, 0.667, 1.0],
						14: [0.8, 1.0, 0.667, 1.0]},
		}
	# End of template data.

	# List region_coords: [WestLon, EastLon, SouthLat, NorthLat]
	global region_coords
	#region_coords = templates[iNumRegions]#2.22
	if templates.has_key(iNumRegionsForShape):
		region_coords = templates[iNumRegionsForShape]
	else:
		# <!-- custom: Preserve tuned BTG layouts where present and synthesize only missing capacities, including Huge/SAS high-player layouts. See KI#253. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		region_coords = buildGridTemplate(iNumRegionsForShape, iRows)

class GridMultilayeredFractal(CvMapGeneratorUtil.MultilayeredFractal):
	def addLandPlot(self, i):
		if self.wholeworldPlotTypes[i] == PlotTypes.PLOT_OCEAN:
			if self.dice.get(5, "Hills on spokes - Grid PYTHON") == 0:
				self.wholeworldPlotTypes[i] = PlotTypes.PLOT_HILLS
			else:
				self.wholeworldPlotTypes[i] = PlotTypes.PLOT_LAND

	def generatePlotsByRegion(self):
		# Sirian's MultilayeredFractal class, controlling function.
		# You -MUST- customize this function for each use of the class.
		# iPlayers = self.gc.getGame().countCivPlayersEverAlive()
		# I Want this as a global definition

		# Sea Level adjustment (from user input), limited to value of 5%.
		sea = self.gc.getSeaLevelInfo(self.map.getSeaLevel()).getSeaLevelChange()
		sea = min(sea, 5)
		sea = max(sea, -5)

		global region_duplicated
		global other_regions#
		# <!-- custom: Share an explicit completion flag with normalization; it must remain false unless this regional generator reaches its successful end. See KI#252. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		global bGridRegionalGenerationCompleted
		# Add the land (two fractals per region to ensure cohesion).
		#global region_coords
		#global regions_in_use
		#global remaining_regions
		#global remaining_regionsTwo#2.21
		region_duplicated = [0 for i in range(5)]
		for region_loop in range(len(remaining_regions)):
			[fWestLon, fEastLon, fSouthLat, fNorthLat] = region_coords[region_loop]
			iWestX = int(self.iW * fWestLon)
			iEastX = int(self.iW * fEastLon) - 1
			iSouthY = int(self.iH * fSouthLat)
			iNorthY = int(self.iH * fNorthLat) -1
			iWidth = iEastX - iWestX + 1
			iHeight = iNorthY - iSouthY + 1

			if region_duplicated[3] == 0 : 
				region_duplicated = [region_loop, iWestX, iSouthY, iWidth, iHeight]
			elif (iWidth > region_duplicated[3]) and ((iWidth > region_duplicated[3]) < region_duplicated[3]) : 
				region_duplicated = [region_loop, iWestX, iSouthY, iWidth, iHeight]

		region_roll = region_duplicated[0]
		thisRegion = remaining_regions[region_roll]
		regions_in_use.append(thisRegion)
		del remaining_regions[region_roll]
		# <!-- custom: BTG's second region pool still contained the already-generated mirror source, so open maps generated that hub twice and all modes recorded it twice.
		# Consume the source in both pools. See KI#274. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		remaining_regionsTwo.remove(thisRegion)

		# Region dimensions
		[fWestLon, fEastLon, fSouthLat, fNorthLat] = region_coords[thisRegion]
		iWestX = int(self.iW * fWestLon)
		iEastX = int(self.iW * fEastLon) - 1
		iSouthY = int(self.iH * fSouthLat)
		iNorthY = int(self.iH * fNorthLat) -1
		iWidth = iEastX - iWestX + 1
		iHeight = iNorthY - iSouthY + 1

		# Each landmass only takes up approximately 64% of the space in its region.
		# This space is further divided between land and water.
		# Choose a pattern for this region
		landPattern = self.dice.get(3, "Land Pattern - Grid PYTHON")
		if landPattern == 1: # Tall
			regWestX = iWestX + int(iWidth * 0.18)
			regSouthY = iSouthY
			regWidth = int(iWidth * 0.64)
			regHeight = iHeight
		elif landPattern == 2: # Wide
			regWestX = iWestX
			regSouthY = iSouthY + int(iHeight * 0.18)
			regWidth = iWidth
			regHeight = int(iHeight * 0.64)
		else: # landPattern == 0, Square
			regWestX = iWestX + int(iWidth * 0.1)
			regSouthY = iSouthY + int(iHeight * 0.1)
			regWidth = int(iWidth * 0.8)
			regHeight = int(iHeight * 0.8)

		self.generatePlotsInRegion(45 + sea, regWidth, regHeight, regWestX, regSouthY, 1, 4, self.iRoundFlags, self.iTerrainFlags, 6, 6, True, 3, -1, False, False)

		# Core fractal to increase cohesion
		coreWestX = regWestX + int(regWidth * 0.25)
		coreEastX = coreWestX + int(regWidth * 0.5)
		coreSouthY = regSouthY + int(regHeight * 0.25)
		coreNorthY = coreSouthY + int(regHeight * 0.5)
		coreWidth = coreEastX - coreWestX + 1
		coreHeight = coreNorthY - coreSouthY + 1

		self.generatePlotsInRegion(25, coreWidth, coreHeight, coreWestX, coreSouthY, 1, 3, self.iHorzFlags, self.iTerrainFlags, 5, 5, True, 3, -1, False, False)

		#2.21z - the 'Open part'
		if (CyMap().getCustomMapOption(1) == 0):

			for region_loop in range(iNumRegions - 1):#2.15
				# Choose an unused region
				region_roll = self.dice.get(len(remaining_regionsTwo), "Region Roll - Grid PYTHON")
				thisRegion = remaining_regionsTwo[region_roll]
				regions_in_use.append(thisRegion)
				del remaining_regionsTwo[region_roll]

				# Region dimensions
				[fWestLon, fEastLon, fSouthLat, fNorthLat] = region_coords[thisRegion]
				iWestX = int(self.iW * fWestLon)
				iEastX = int(self.iW * fEastLon) - 1
				iSouthY = int(self.iH * fSouthLat)
				iNorthY = int(self.iH * fNorthLat) -1
				iWidth = iEastX - iWestX + 1
				iHeight = iNorthY - iSouthY + 1

				# Each landmass only takes up approximately 64% of the space in its region.
				# This space is further divided between land and water.
				# Choose a pattern for this region
				landPattern = self.dice.get(3, "Land Pattern - Grid PYTHON")
				if landPattern == 1: # Tall
					regWestX = iWestX + int(iWidth * 0.18)
					regSouthY = iSouthY
					regWidth = int(iWidth * 0.64)
					regHeight = iHeight
				elif landPattern == 2: # Wide
					regWestX = iWestX
					regSouthY = iSouthY + int(iHeight * 0.18)
					regWidth = iWidth
					regHeight = int(iHeight * 0.64)
				else: # landPattern == 0, Square
					regWestX = iWestX + int(iWidth * 0.1)
					regSouthY = iSouthY + int(iHeight * 0.1)
					regWidth = int(iWidth * 0.8)
					regHeight = int(iHeight * 0.8)

				self.generatePlotsInRegion(45 + sea, regWidth, regHeight, regWestX, regSouthY, 1, 4, self.iRoundFlags, self.iTerrainFlags, 6, 6, True, 3, -1, False, False)

				# Core fractal to increase cohesion
				coreWestX = regWestX + int(regWidth * 0.25)
				coreEastX = coreWestX + int(regWidth * 0.5)
				coreSouthY = regSouthY + int(regHeight * 0.25)
				coreNorthY = coreSouthY + int(regHeight * 0.5)
				coreWidth = coreEastX - coreWestX + 1
				coreHeight = coreNorthY - coreSouthY + 1

				self.generatePlotsInRegion(25, coreWidth, coreHeight, coreWestX, coreSouthY, 1, 3, self.iHorzFlags, self.iTerrainFlags, 5, 5, True, 3, -1, False, False)		

		#2.21z - Back to common part

		if (CyMap().getCustomMapOption(1) >= 1):

			#duplicate land for other used regions
			other_regions = []
			# <!-- custom: Record each physical mirrored hub once, including the source once, then crop every copy to the largest common in-bounds footprint.
			# BTG duplicated the source ID and assumed rounded source/target dimensions were equal. See KI#274. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			for region_loop in range(len(region_coords)):
				if len(regions_in_use) >= iNumRegions:
					break
				if region_loop not in regions_in_use:
					regions_in_use.append(region_loop)

			for thisRegion in regions_in_use:
				[fWestLon, fEastLon, fSouthLat, fNorthLat] = region_coords[thisRegion]
				iWestX = int(self.iW * fWestLon)
				iEastX = int(self.iW * fEastLon) - 1
				iSouthY = int(self.iH * fSouthLat)
				iNorthY = int(self.iH * fNorthLat) -1
				iWidth = iEastX - iWestX + 1
				iHeight = iNorthY - iSouthY + 1

				other_regions.append([thisRegion, iWestX, iSouthY, iWidth, iHeight])

			copyWidth = min([item[3] for item in other_regions])
			copyHeight = min([item[4] for item in other_regions])
			iD, iWestXD, iSouthYD, iWidthD, iHeightD = region_duplicated
			for thisRegion, iWestX, iSouthY, iWidth, iHeight in other_regions:
				for x in range(copyWidth):
					wholeworldX = x + iWestX
					wholeworldXD = x + iWestXD
					for y in range(copyHeight):
						wholeworldY = y + iSouthY
						iWorld = wholeworldY*self.iW + wholeworldX
						wholeworldYD = y + iSouthYD
						iWorldD = wholeworldYD*self.iW + wholeworldXD
						self.wholeworldPlotTypes[iWorld] = self.wholeworldPlotTypes[iWorldD]

		# Generate spokes
		map = CyMap()
		spoke_width = map.getCustomMapOption(2)
		if spoke_width > 0:

			for regionLoop in range(len(regions_in_use)):
				thisRegion = regions_in_use[regionLoop]
				# Region dimensions
				[iWestLon, iEastLon, iSouthLat, iNorthLat] = region_coords[thisRegion]
				iWestX = int(self.iW * iWestLon)
				iEastX = int(self.iW * iEastLon) - 1
				iSouthY = int(self.iH * iSouthLat)
				iNorthY = int(self.iH * iNorthLat) -1
				iCenterX = int((iWestX + iEastX) / 2)
				iCenterY = int((iSouthY + iNorthY) / 2)

				for x in range(iWestX, iEastX+1):
					i = iCenterY*self.iW + x
					self.addLandPlot(i)
					if spoke_width > 1:
						self.addLandPlot(i + self.iW)
					if spoke_width > 2:
						self.addLandPlot(i - self.iW)
					if spoke_width > 3:#2.21z
						self.addLandPlot(i + self.iW + self.iW)
					if spoke_width > 4:#2.21z
						self.addLandPlot(i - self.iW - self.iW)						

				for y in range(iSouthY, iNorthY+1):
					i = y*self.iW + iCenterX
					self.addLandPlot(i)
					if spoke_width > 1:
						self.addLandPlot(i + 1)
					if spoke_width > 2:
						self.addLandPlot(i - 1)
					if spoke_width > 3:#2.21z
						self.addLandPlot(i + 2)
					if spoke_width > 4:#2.21z
						self.addLandPlot(i - 2)				

		# <!-- custom: Mark regional geometry usable only after hub and spoke generation completed, preventing later mirroring after generic/failed generation. See KI#252. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		bGridRegionalGenerationCompleted = True
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
	#iPlayers = CyGlobalContext().getGame().countCivPlayersEverAlive()
	#I've already declared this

	# <!-- custom: Dynamic templates support the advertised SAS player range, so high-player games retain Grid geometry instead of becoming an ordinary fractal. See KI#253. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if iPlayers > 0:
		pass
	else: # Error catching.
		fractal_world = FractalWorld()
		fractal_world.initFractal(polar = True)
		plotTypes = fractal_world.generatePlotTypes()
		return plotTypes

	fractal_world = GridMultilayeredFractal()
	plotTypes = fractal_world.generatePlotsByRegion()
	return plotTypes

def generateTerrainTypes():
	terraingen = TerrainGenerator()
	terraingen.__init__(iDesertPercent=8, iPlainsPercent=25, fSnowLatitude=2.0, fTundraLatitude=2.0, fGrassLatitude=0.0, fDesertBottomLatitude=0.0, fDesertTopLatitude=2.0)
	terrainTypes = terraingen.generateTerrain()

	# Eliminate snow and tundra completely (they still get placed sometimes at extreme latitudes)
	for i in range(len(terrainTypes)):
		if (terrainTypes[i] == terraingen.terrainIce) or (terrainTypes[i] == terraingen.terrainTundra):
			terrainTypes[i] = terraingen.terrainPlains

	return terrainTypes

def addFeatures():
	# Remove all peaks along the coasts, before adding Features, Bonuses, Goodies, etc.
	map = CyMap()
	iW = map.getGridWidth()
	iH = map.getGridHeight()
	for plotIndex in range(iW * iH):
		pPlot = map.plotByIndex(plotIndex)
		if pPlot.isPeak() and pPlot.isCoastalLand():
			# If a peak is along the coast, change to hills and recalc.
			pPlot.setPlotType(PlotTypes.PLOT_HILLS, true, true)

	# Now add the features.
	featuregen = FeatureGenerator()

	if (CyMap().getCustomMapOption(6) == 3):
		featuregen.__init__(iJunglePercent=0, iForestPercent=90, jungle_grain=5, forest_grain=6)
	elif (CyMap().getCustomMapOption(6) == 2):
		featuregen.__init__(iJunglePercent=0, iForestPercent=75, jungle_grain=5, forest_grain=6)
	elif (CyMap().getCustomMapOption(6) == 1):
		featuregen.__init__(iJunglePercent=0, iForestPercent=60, jungle_grain=5, forest_grain=6)
	else: 		
		featuregen.__init__(iJunglePercent=0, iForestPercent=40, jungle_grain=5, forest_grain=6)	

	featuregen.addFeatures()
	return 0

def minStartingDistanceModifier():
	return 20

def assignStartingPlots():

	# Custom start plot finder
	global iNumRegions
	global region_coords
	global regions_in_use
	gc = CyGlobalContext()
	map = CyMap()
	dice = gc.getGame().getMapRand()
	iW = map.getGridWidth()
	iH = map.getGridHeight()
	#iPlayers = gc.getGame().countCivPlayersEverAlive()
	#I've already declared this

	# <!-- custom: High-player regional starts remain valid now that region capacity grows with the player count. See KI#253. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if iPlayers < 1:
		CyPythonMgr().allowDefaultImpl()
		return
	iActualPlayers = gc.getGame().countCivPlayersEverAlive()
	if len(regions_in_use) < iActualPlayers:
		# <!-- custom: If actual players exceed distinct scripted region slots, fall back before assigning any custom start.
		# Removing BTG's duplicate source-region bookkeeping makes this guard detect one-over-template cases; compare the actual count because Grid's legacy iPlayers can instead mean rounded capacity for an intentionally empty hub. See KI#247 and KI#274. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		CyPythonMgr().allowDefaultImpl()
		return

	# Obtain the minimum crow-flies distance figures [minX, minY] for this map size and number of players.
	minimums = {2: [0.15, 0.3],
				3: [0.15, 0.17], #added penny
				4: [0.15, 0.15],
				5: [0.125, 0.16], #added penny
				6: [0.1, 0.15],
				7: [0.08, 0.15], #added penny
				8: [0.07, 0.15],
				9: [0.1, 0.1],
				10: [0.09, 0.1], #added penny
				11: [0.08, 0.1], #added penny
				12: [0.07, 0.1],
				15: [0.06, 0.1],
				16: [0.07, 0.07],
				18: [0.05, 0.1]}
	# <!-- custom: Supported Grid capacities can omit a tuned legacy key; use Cross's conservative fallback instead of raising and silently abandoning regional starts. See KI#251. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if iNumRegions in minimums:
		[minLon, minLat] = minimums[iNumRegions]
	else:
		[minLon, minLat] = [0.05, 0.08]
	minX = max(3, int(minLon * iW))
	minY = max(3, int(minLat * iH))

	# region_data: [WestX, EastX, SouthY, NorthY, 
	# numLandPlotsinRegion, numCoastalPlotsinRegion,
	# numOceanPlotsinRegion, iRegionNetYield, 
	# iNumLandAreas, iNumPlotsinRegion]
	global region_data
	region_data = {}
	region_best_areas = {}
	region_yields = []
	sorting_regions = []
	for regionLoop in range(len(regions_in_use)):
		thisRegion = regions_in_use[regionLoop]
		# Region dimensions
		[iWestLon, iEastLon, iSouthLat, iNorthLat] = region_coords[thisRegion]
		iWestX = int(iW * iWestLon)
		iEastX = int(iW * iEastLon) - 1
		iSouthY = int(iH * iSouthLat)
		iNorthY = int(iH * iNorthLat) -1
		# Plot and Area info.
		iNumLandPlots = 0
		iNumCoastalPlots = 0
		iNumOceanPlots = 0
		iRegionNetYield = 0
		iNumLandAreas = 0
		iNumPlotsinRegion = 0
		land_areas = []
		land_area_plots = {}
		land_area_yield = {}
		# Cycle through all plots in the region.
		for x in range(iWestX, iEastX + 1):
			for y in range(iSouthY, iNorthY + 1):
				iNumPlotsinRegion += 1
				i = y * iW + x
				pPlot = map.plot(x, y)
				if pPlot.getBonusType(-1) != -1: # Count any bonus resource as added value
					iRegionNetYield += 2
				if pPlot.isWater(): # Water plot
					iFertileCheck = pPlot.calculateBestNatureYield(YieldTypes.YIELD_FOOD, TeamTypes.NO_TEAM)
					if iFertileCheck > 1: # If the plot has extra food, count it.
						iRegionNetYield += (2 * (iFertileCheck - 1))
					if pPlot.isAdjacentToLand(): # Coastal plot
						# <!-- custom: The inherited scripts tested bound method objects, which are always truthy, instead of each plot's freshwater result. See KI#249. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
						if pPlot.isFreshWater():
							iNumCoastalPlots += 1
							iRegionNetYield += 2
						else:
							iNumCoastalPlots += 1
							iRegionNetYield += 1
					else:
						iNumOceanPlots += 1
				else: # Land plot
					iNumLandPlots += 1
					iArea = pPlot.getArea()
					iPlotYield = pPlot.calculateTotalBestNatureYield(TeamTypes.NO_TEAM)
					iFertileCheck = pPlot.calculateBestNatureYield(YieldTypes.YIELD_FOOD, TeamTypes.NO_TEAM)
					if iFertileCheck > 1: # If the plot has extra food, count the extra as double value!
						iPlotYield += (iFertileCheck - 1)
					iRegionNetYield += iPlotYield
					if pPlot.isHills():
						iRegionNetYield += 1 # Add a bonus point for Hills plots.
					if iArea not in land_areas: # This plot is the first detected in its AreaID.
						iNumLandAreas += 1
						land_areas.append(iArea)
						land_area_plots[iArea] = 1
						land_area_yield[iArea] = iPlotYield
					else: # This AreaID already known.
						land_area_plots[iArea] += 1
						land_area_yield[iArea] += iPlotYield
		# Sort areas, achieving a list of AreaIDs with best areas first.
		area_yields = land_area_yield.values()
		area_yields.sort()
		area_yields.reverse()
		best_areas = []
		for areaTestLoop in range(iNumLandAreas):
			for landLoop in range(len(land_areas)):
				if area_yields[areaTestLoop] == land_area_yield[land_areas[landLoop]]:
					best_areas.append(land_areas[landLoop])
					del land_areas[landLoop]
					break
		# Store infos to regional lists.
		region_data[thisRegion] = [iWestX, iEastX, iSouthY, iNorthY, iNumLandPlots, iNumCoastalPlots, iNumOceanPlots, iRegionNetYield, iNumLandAreas, iNumPlotsinRegion]
		region_best_areas[thisRegion] = best_areas
		region_yields.append(iRegionNetYield)
		sorting_regions.append(iRegionNetYield)

	# Now sort the regions
	best_regions = []
	# <!-- custom: BTG aliased and consumed the global region list while ranking; rank a copy of the now-unique physical IDs so later callbacks retain their bookkeeping. See KI#274. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	region_numbers = regions_in_use[:]
	sorting_regions.sort()
	sorting_regions.reverse()
	for regionTestLoop in range(iNumRegions):
		for yieldLoop in range(len(region_numbers)):
			if sorting_regions[regionTestLoop] == region_yields[yieldLoop]:
				best_regions.append(region_numbers[yieldLoop])
				del region_numbers[yieldLoop]
				del region_yields[yieldLoop]
				break

	# Need to discard the worst regions and then reverse the region order.
	# Of the regions that will be used, the worst will be assigned first.
	best_regions.reverse()

	# Obtain player numbers. (Account for possibility of Open slots!)
	player_list = []
	for plrCheckLoop in range(gc.getMAX_CIV_PLAYERS()):#2.35 when you reduce to 12 players... cannot hardcode this
		#2.22 Protecting for BTS
		if isBTPon:
			if CyGlobalContext().getPlayer(plrCheckLoop).isEverAlive() and not CyGlobalContext().getPlayer(plrCheckLoop).isSpectator():
				player_list.append(plrCheckLoop)
		else:
			if CyGlobalContext().getPlayer(plrCheckLoop).isEverAlive():
				player_list.append(plrCheckLoop)

	# Shuffle start points so that players are assigned regions at random.
	shuffledPlayers = []
	for playerLoopTwo in range(gc.getGame().countCivPlayersEverAlive()):
		iChoosePlayer = dice.get(len(player_list), "Shuffling Regions - Grid PYTHON")
		shuffledPlayers.append(player_list[iChoosePlayer])
		del player_list[iChoosePlayer]

	# Now assign the start plots!
	plot_assignments = {}
	min_dist = []

	if (CyMap().getCustomMapOption(1) == 0 or CyMap().getCustomMapOption(1) == 1):

		# Loop through players/regions.

		#2.18 A funky way, you double and remove the num of Spec : now the specs are on the left and players on the right
		#iNumSpectators
		'''if iNumSpectators > 0:#Hang on, don't even need this, since it's reverse in C++
			iTotLoop = iPlayers + iNumSpectators
			iStartLoop = iNumSpectators			
		#else :'''
		if (CyMap().getCustomMapOption(5) == 0):
			iTotLoop = iNumRegions #2.22 - Because would crash for when you leave Water on a 3x3 Map of 5 players		
		else:
			iTotLoop = iPlayers

		iStartLoop = 0
		iDidLoop = 0

		#for assignLoop in range(iPlayers):
		for assignLoop in range(iStartLoop,min(iTotLoop, len(shuffledPlayers))):
			#playerID = shuffledPlayers[assignLoop]############ PYTHON ERROR #####################
			playerID = shuffledPlayers[iDidLoop]#2.18
			iDidLoop += 1#2.18

			#reg = best_regions[assignLoop]#Particularity of the Americas Map
			reg = assignLoop#Americas - Need to be in order, so that all the region on same line are assigned first
			if not region_data.has_key(reg):
				if assignLoop < len(best_regions):
					reg = best_regions[assignLoop]
				elif len(region_data.keys()) > 0:
					reg = region_data.keys()[0]
				else:
					continue

			[westX, eastX, southY, northY] = region_data[reg][0:4]
			# Only consider the inner part of the region
			iWidth = eastX - westX + 1
			iHeight = northY - southY + 1
			westX = westX + int(iWidth * 0.2)
			eastX = eastX - int(iWidth * 0.2)
			southY = southY + int(iHeight * 0.2)
			northY = northY - int(iHeight * 0.2)

			iNumAreas = region_data[reg][8]
			area_list = region_best_areas[reg]
			# Print Data for debugging
			# Error Handling (if valid start plot not found, reduce MinDistance)
			# <!-- custom: Keep iPass outside the retry loop so a failed regional search reaches the finite default-placement fallback instead of repeating pass 0 forever.
			# Resolve the player/range before the area loop so an empty area list also falls back safely.
			# No supported trigger was reproduced; this defensively restores the retry contract used by current BTG_Cross. See KI#329. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			player = gc.getPlayer(playerID)
			player.AI_updateFoundValues(True)
			iRange = player.startingPlotRange()
			iPass = 0
			validFn = None
			while (true):
				iBestValue = 0
				pBestPlot = None
				# Loop through best areas in this region
				for areaLoop in range(iNumAreas):
					areaID = area_list[areaLoop]
					# Loop through all plots in the region.
					for iX in range(westX, eastX + 1):
						for iY in range(southY, northY + 1):
							pPlot = map.plot(iX, iY)
							if pPlot.isWater():
								continue
							# if not pPlot.isCoastalLand(): continue
							if areaID != pPlot.getArea():
								continue
							if validFn is not None and not validFn(playerID, iX, iY):
								continue
							val = pPlot.getFoundValue(playerID)
							# <!-- custom: Apply the corrected freshwater result to this first custom-start branch too. See KI#249. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
							if pPlot.isFreshWater():
								val += 1000
							if val > iBestValue:
								valid = True
								for invalid in min_dist:
									[invalidX, invalidY] = invalid
									if abs(invalidX - iX) < minX and abs(invalidY - iY) < minY:
										valid = False
										break
								if valid:
									for iI in range(gc.getMAX_CIV_PLAYERS()):
										if (gc.getPlayer(iI).isAlive()):
											if (iI != playerID):
												if gc.getPlayer(iI).startingPlotWithinRange(pPlot, playerID, iRange, iPass):
													valid = False
													break
								if valid:
									iBestValue = val
									pBestPlot = pPlot

					if pBestPlot is not None:
						min_dist.append([pBestPlot.getX(), pBestPlot.getY()])
						sPlot = map.plot(pBestPlot.getX(), pBestPlot.getY())
						plrID = gc.getPlayer(playerID)
						plrID.setStartingPlot(sPlot, true)
						break # Valid start found, stop checking areas and plots.
					else:
						pass # This area too close to somebody, try the next area.

				# Check to see if a valid start was found in ANY areaID.
				if pBestPlot is None:
					print("player %s pass %s failed" % (playerID, iPass))
					iPass += 1
					if iPass <= max(player.startingPlotRange() + eastX - westX, player.startingPlotRange() + northY - southY):
						continue
					else: # A region has failed to produce any valid starts!
						bSuccessFlag = False
						print("---")
						print("A region has failed")
						print("---")
						# Regional start plot assignment has failed. Reverting to default.
						CyPythonMgr().allowDefaultImpl()
						return
				else:
					break # This player has been assigned a start plot.

		# Successfully assigned start plots, continue back to C++
		return None

	elif (CyMap().getCustomMapOption(1) >= 2):	

		# Loop through players/regions.
		dX = 0
		dY = 0

		if region_duplicated[0] in best_regions :
					tpR = [region_duplicated[0]]
					for item in best_regions :
							if item not in tpR : tpR.append(item)
					best_regions = tpR
		else :
			print("you missed something there O.o")

		#2.18 A funky way, you double and remove the num of Spec : now the specs are on the left and players on the right
		#iNumSpectators
		'''if iNumSpectators > 0:#Hang on, don't even need this, since it's reverse in C++
			iTotLoop = iPlayers + iNumSpectators
			iStartLoop = iNumSpectators			
		#else :'''
		if (CyMap().getCustomMapOption(5) == 0):
			iTotLoop = iNumRegions #2.22 - Because would crash for when you leave Water on a 3x3 Map of 5 players		
		else:
			iTotLoop = iPlayers

		iStartLoop = 0
		iDidLoop = 0
		#for assignLoop in range(iPlayers):
		for assignLoop in range(iStartLoop,min(iTotLoop, len(shuffledPlayers))):	
		#for assignLoop in range(iPlayers):# -Penny ; Not necessary for condition
		# for assignLoop in range(iNumRegions):
			#playerID = shuffledPlayers[assignLoop]############ PYTHON ERROR #####################
			playerID = shuffledPlayers[iDidLoop]#2.18
			iDidLoop += 1#2.18
			#reg = best_regions[assignLoop]#Particularity of the Americas Map
			reg = assignLoop#Americas - Need to be in order, so that all the region on same line are assigned first
			if not region_data.has_key(reg):
				if assignLoop < len(best_regions):
					reg = best_regions[assignLoop]
				elif len(region_data.keys()) > 0:
					reg = region_data.keys()[0]
				else:
					continue

			[wX, eX, sY, nY] = region_data[reg][0:4]
			# Only consider the inner part of the region
			iWidth = eX - wX + 1
			iHeight = nY - sY + 1

			westX = wX + int(iWidth * 0.2)
			eastX = eX - int(iWidth * 0.2)
			southY = sY + int(iHeight * 0.2)
			northY = nY - int(iHeight * 0.2)

			if dX == 0 :

				iNumAreas = region_data[reg][8]
				area_list = region_best_areas[reg]
				# Print Data for debugging
				# Error Handling (if valid start plot not found, reduce MinDistance)
				# <!-- custom: Apply the same finite retry/fallback state to the mirrored-positioning branch. See KI#329. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				player = gc.getPlayer(playerID)
				player.AI_updateFoundValues(True)
				iRange = player.startingPlotRange()
				iPass = 0
				validFn = None
				while (true):
					iBestValue = 0
					pBestPlot = None
					# Loop through best areas in this region
					for areaLoop in range(iNumAreas):
						areaID = area_list[areaLoop]
						# Loop through all plots in the region.
						for iX in range(westX, eastX + 1):
							for iY in range(southY, northY + 1):
								pPlot = map.plot(iX, iY)
								if pPlot.isWater():
									continue
								# if not pPlot.isCoastalLand(): continue
								if areaID != pPlot.getArea():
									continue
								if validFn is not None and not validFn(playerID, iX, iY):
									continue
								val = pPlot.getFoundValue(playerID)
								# <!-- custom: Apply the corrected freshwater result to this mirrored custom-start branch too. See KI#249. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
								if pPlot.isFreshWater():
									val += 1000
								if val > iBestValue:
									valid = True
									for invalid in min_dist:
										[invalidX, invalidY] = invalid
										if abs(invalidX - iX) < minX and abs(invalidY - iY) < minY:
											valid = False
											break
									if valid:
										for iI in range(gc.getMAX_CIV_PLAYERS()):
											if (gc.getPlayer(iI).isAlive()):
												if (iI != playerID):
													if gc.getPlayer(iI).startingPlotWithinRange(pPlot, playerID, iRange, iPass):
														valid = False
														break
									if valid:
										iBestValue = val
										pBestPlot = pPlot

						if pBestPlot is not None:
							min_dist.append([pBestPlot.getX(), pBestPlot.getY()])
							sPlot = map.plot(pBestPlot.getX(), pBestPlot.getY())
							plrID = gc.getPlayer(playerID)
							plrID.setStartingPlot(sPlot, true)
							dX = pBestPlot.getX() - wX
							dY = pBestPlot.getY() - sY
							break # Valid start found, stop checking areas and plots.
						else:
							pass # This area too close to somebody, try the next area.

					# Check to see if a valid start was found in ANY areaID.
					if pBestPlot is None:
							print("player %s pass %s failed" % (playerID, iPass))
							iPass += 1
							if iPass <= max(player.startingPlotRange() + eastX - westX, player.startingPlotRange() + northY - southY):
								continue
							else: # A region has failed to produce any valid starts!
								bSuccessFlag = False
								print("---")
								print("A region has failed")
								print("---")
								# Regional start plot assignment has failed. Reverting to default.
								CyPythonMgr().allowDefaultImpl()
								return
					else:
						break # This player has been assigned a start plot.

			else :
				sPlot = map.plot(wX + dX, sY + dY)
				plrID = gc.getPlayer(playerID)
				plrID.setStartingPlot(sPlot, true)

		# Successfully assigned start plots, continue back to C++'''
		return None

def normalizeRemovePeaks():
	return None

def normalizeAddExtras():

	#if (CyMap().getCustomMapOption(1) == 1): # Applies in all cases now, no balanced option always balanced
	balancer.normalizeAddExtras()

	# V3 by Axius: Give a land oil and aluminum to each player

	gc = CyGlobalContext()
	map = CyMap()
	oil = getInfoTypeOrFail("BONUS_OIL")
	alu = getInfoTypeOrFail("BONUS_ALUMINUM")
	ivory = getInfoTypeOrFail("BONUS_ELEPHANTS")
	gold = getInfoTypeOrFail("BONUS_GOLD")
	silver = getInfoTypeOrFail("BONUS_SILVER")
	gems = getInfoTypeOrFail("BONUS_GEMSTONES")
	desert = getInfoTypeOrFail("TERRAIN_DESERT")
	grass = getInfoTypeOrFail("TERRAIN_GRASS")
	random.seed(gc.getGame().getMapRand().get(30000, "Shuffle Plots - PYTHON"))

	for i in range(0,gc.getMAX_CIV_PLAYERS()):
		#if gc.getPlayer(i).isAlive():
		if gc.getPlayer(i).isEverAlive():#2.22 - Very important - nothing was working with Spectator

			start_plot = gc.getPlayer(i).getStartingPlot()
			startx, starty = start_plot.getX(), start_plot.getY()
			plotsclose = []
			plotsfurther = []
			plotsboundaries = []
			has_oil = false
			has_alu = false
			has_ivory = false
			has_horse = false
			has_precious = false
			for dx in range(-5,5):
				for dy in range(-5,5):
					p = map.plot(startx+dx,starty+dy)
					if ((dx != 0) or (dy != 0)) and (not p.isNone()) and (not p.isImpassable()) and (not p.isWater()):
						plotsclose.append(p)
						if p.getBonusType(-1) == oil:
							has_oil = True
						if p.getBonusType(-1) == alu:
							has_alu = True

			for dx in range(-7,7):
				for dy in range(-7,7):
					p = map.plot(startx+dx,starty+dy)
					if ((dx != 0) or (dy != 0)) and (not p.isNone()) and (not p.isImpassable()) and (not p.isWater()):
						plotsfurther.append(p)	
						if p.getBonusType(-1) == ivory:
							has_ivory = True
						if p.getBonusType(-1) == gold:
							has_precious = True
						if p.getBonusType(-1) == silver:
							has_precious = True
						if p.getBonusType(-1) == gems:
							has_precious = True				

			for dx in range(-6,6):
				for dy in range(-6,6):
					p = map.plot(startx+dx,starty+dy)
					#2.22 This is too restrictive for this map
					if (( abs(dx) >= 5 or abs(dy) >= 5) and not p.isNone()) and (not p.isImpassable()) and (not p.isWater()):#1 notch closer than other maps
						if ((abs(dx) >= 3 and abs(dy) >= 3)):#too tight otherwise on this map
							if (p.getBonusType(-1) == BonusTypes.NO_BONUS):
								plotsboundaries.append(p)						

			if (CyMap().getCustomMapOption(4) == 1):
				if not has_precious:

					random.shuffle(plotsboundaries) 
					for p in plotsboundaries:
						if (p.getBonusType(-1) == BonusTypes.NO_BONUS) and p.canHaveBonus(silver, True):
							p.setBonusType(silver)
							has_precious = True
							break
					# <!-- custom: The inherited forced-Silver fallback indexed an unproven nonempty boundary list.
					# If narrow/tiny geometry has no diagonal boundary candidate, use an empty legal land plot already collected within seven tiles; if neither list has a plot, safely leave the optional normalization unmet. See KI#329. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
					if not has_precious:
						plotsforced = plotsboundaries
						if len(plotsforced) == 0:
							plotsforced = [pLoop for pLoop in plotsfurther if pLoop.getBonusType(-1) == BonusTypes.NO_BONUS]
					if not has_precious and len(plotsforced) > 0:
						p = plotsforced[0]
						p.setPlotType(PlotTypes.PLOT_LAND, True, True)
						p.setTerrainType(desert, True, True)
						p.setFeatureType(-1,-1)#2.34 -- Because Sometimes Oil is on floodplain
						p.setBonusType(silver)		

	#2020 04 - After player's all stuff
	iW = map.getGridWidth()
	iH = map.getGridHeight()

	if (CyMap().getCustomMapOption(8) >= 1):
		for iX in range(iW):
			for iY in range(iH):
				pPlot = map.plot(iX, iY)
				#if pPlot.getTerrainType() == desert:
				if pPlot.getTerrainType() == desert and pPlot.getBonusType(-1) == -1 and pPlot.getFeatureType() == -1:
				#if pPlot.getTerrainType() == desert and pPlot.getBonusType(-1) == -1:
				#if pPlot.getTerrainType() == desert and pPlot.getBonusType(-1) == BonusTypes.NO_BONUS and pPlot.getFeatureType(-1) == FeatureTypes.NO_FEATURE:
					pPlot.setTerrainType(grass, True, True)

	# BTG Resources option removed: keep default behavior (no BTG extra-resource injections).

	# <!-- custom: Only mirror layers when this generation completed the regional Grid path; generic/failure fallbacks have no valid regional geometry. See KI#252. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (CyMap().getCustomMapOption(1) == 3 and bGridRegionalGenerationCompleted):
		mirrorizeMap() #2020 06 - BTP 2.15 - Restart feature

	# BTG Resources option removed: no sulphur-on-capital mode.

	CyMap().recalculateAreas()#2.21zz Mirror looks weird				

	return None

def addBonusType(argsList):
	[iBonusType] = argsList
	gc = CyGlobalContext()
	type_string = gc.getBonusInfo(iBonusType).getType()

	#2.21y
	if isBTPon:
		# BTG Resources option removed: keep default exclusion behavior.
		if (type_string in balancer.newResourcesBTP):
			return None
		if (type_string in balancer.newStrategicBTP):
			return None

	#if (CyMap().getCustomMapOption(1) == 1): # Applies in all cases now, no balanced option always balanced
	if (type_string in balancer.resourcesToBalance) or (type_string in balancer.resourcesToEliminate):
		return None # don't place any of this bonus randomly

	CyPythonMgr().allowDefaultImpl() # pretend we didn't implement this method, and let C handle this bonus in the default way

def isBonusIgnoreLatitude():
	return True

def mirrorizeMap():
	global region_duplicated
	global other_regions
	gc = CyGlobalContext()
	map = CyMap()
	iW = map.getGridWidth()
	iH = map.getGridHeight()

	region_duplicated_ID, iWestX, iSouthY, iWidth, iHeight = region_duplicated
	# <!-- custom: Use the same common source/target footprint chosen during plot generation so every later mirrored layer stays within both rectangles. See KI#274. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	minW = min([item[3] for item in other_regions])
	minH = min([item[4] for item in other_regions])

	for iX in range(iW):
		for iY in range(iH):
			if iX >= iWestX and iX < iWestX + minW and iY >= iSouthY and iY < iSouthY + minH :
				continue
			pPlot = map.plot(iX, iY)

			pPlot.setImprovementType(-1)
			pPlot.setBonusType(-1)	
			pPlot.setFeatureType(-1, -1)
			pPlot.setNOfRiver(False, CardinalDirectionTypes.CARDINALDIRECTION_WEST)
			pPlot.setWOfRiver(False, CardinalDirectionTypes.CARDINALDIRECTION_NORTH)
			pPlot.setRiverID (-1)

	#reflect plot types (addlake)
	for region_ID, wX, sY, rW, rH in other_regions:
		for dX in range(minW):
			for dY in range(minH):
				pPlot = map.plot(wX + dX, sY + dY)
				rPlot = map.plot(iWestX + dX, iSouthY + dY)
				pPlot.setPlotType(rPlot.getPlotType(), True, True)

	map.recalculateAreas()

	#reflect terrain
	for region_ID, wX, sY, rW, rH in other_regions:
		for dX in range(minW):
			for dY in range(minH):
				pPlot = map.plot(wX + dX, sY + dY)
				rPlot = map.plot(iWestX + dX, iSouthY + dY)
				pPlot.setTerrainType(rPlot.getTerrainType(), True, True)

	map.recalculateAreas()

	#rearrange river IDs
	initRiverID = 0
	riverID = {}
	for dX in range(minW):
		for dY in range(minH):
			pPlot = map.plot(iWestX + dX, iSouthY + dY)
			rID = pPlot.getRiverID()
			if rID != -1 :
				if rID in riverID.keys() :
					pPlot.setRiverID(riverID[rID])
				else :
					riverID[rID] = initRiverID
					pPlot.setRiverID(riverID[rID])
					initRiverID += 1

	iRivers = len(riverID.keys())
	incr = 0

	#mirrorize rivers
	for region_ID, wX, sY, rW, rH in other_regions:
		incr += 1
		for dX in range(minW):
			for dY in range(minH):
				pPlot = map.plot(wX + dX, sY + dY)
				rPlot = map.plot(iWestX + dX, iSouthY + dY)

				if rPlot.isNOfRiver():
					pPlot.setNOfRiver(True, rPlot.getRiverWEDirection())
				if rPlot.isWOfRiver():
					pPlot.setWOfRiver(True, rPlot.getRiverNSDirection())

				rID = rPlot.getRiverID()
				if rID != -1 :
					pPlot.setRiverID(rID + incr * iRivers)

	map.recalculateAreas()

	# mirrorize features
	for region_ID, wX, sY, rW, rH in other_regions:
		for dX in range(minW):
			for dY in range(minH):
				pPlot = map.plot(wX + dX, sY + dY)
				rPlot = map.plot(iWestX + dX, iSouthY + dY)
				pPlot.setFeatureType(rPlot.getFeatureType(), -1)

	map.recalculateAreas()

	# mirrorize bonuses
	for region_ID, wX, sY, rW, rH in other_regions:
		for dX in range(minW):
			for dY in range(minH):
				pPlot = map.plot(wX + dX, sY + dY)
				rPlot = map.plot(iWestX + dX, iSouthY + dY)
				pPlot.setBonusType(rPlot.getBonusType(-1))

	map.recalculateAreas()

	# mirrorize goodies
	for region_ID, wX, sY, rW, rH in other_regions:
		for dX in range(minW):
			for dY in range(minH):
				pPlot = map.plot(wX + dX, sY + dY)
				rPlot = map.plot(iWestX + dX, iSouthY + dY)
				pPlot.setImprovementType(rPlot.getImprovementType())

	map.recalculateAreas()

	return None

def startHumansOnSameTile():
	# BTG Forest Type option removed: keep default forest behavior.
	if (CyMap().getCustomMapOption(9) == 1):
		return True

def normalizeStartingPlotLocations():#2.21z

	gc = CyGlobalContext()	
	dice = gc.getGame().getMapRand()	

	# BTG Start Position option removed: keep default normalization flow.
	CyPythonMgr().allowDefaultImpl()#this is the bit that puts team together and is normal case			

''' 11 - BTG local/map Redclaration of MapGeneratorUtil functions for logic '''
def BTPTopBottomTwoTeams(isBTG):							
	gc = CyGlobalContext()	

	#2.19 debug		
	random.seed(gc.getGame().getMapRand().get(30000, "Shuffle Plots - PYTHON"))	

	iEverAliveTeamCount = 0

	for iI in range(gc.getMAX_CIV_TEAMS()):	
		if isBTG:
			if gc.getTeam(iI).isEverAlive() and not gc.getTeam(iI).isSpectator() and not gc.getTeam(iI).isBarbarian():
				iEverAliveTeamCount += 1
		else:
			if gc.getTeam(iI).isEverAlive() and not gc.getTeam(iI).isBarbarian():
				iEverAliveTeamCount += 1			

	if gc.getGame().countCivPlayersEverAlive() <= 3:
		return None			

	if (gc.getGame().countCivPlayersEverAlive() % 2) != 0 :#verify it's even number, if not don't apply
		return None	

	elif not iEverAliveTeamCount == 2:
		return None

	else:

		#############################
		#Spectator bit - not amazing if spectator is middle team

		listTeams = []
		#2.23 Improve for spectators
		for iI in range(gc.getMAX_CIV_TEAMS()):	
			if isBTG:
				if not gc.getTeam(iI).isSpectator():
					if gc.getTeam(iI).isEverAlive():				
						listTeams.append(gc.getTeam(iI).getID())
			else:
				if gc.getTeam(iI).isEverAlive():				
					listTeams.append(gc.getTeam(iI).getID())				

		random.shuffle(listTeams)		
		teamOne = listTeams[0]
		teamTwo = listTeams[1]
		###########################

		listPlot = []
		listPlayer = []
		iH = CyMap().getGridHeight()
		halfHeight = iH / 2
		for iI in range(gc.getMAX_CIV_PLAYERS()):
			if isBTG:
				if (gc.getPlayer(iI).isAlive() and not gc.getPlayer(iI).isSpectator()):		
					listPlot.append(gc.getPlayer(iI).getStartingPlot())
					listPlayer.append(gc.getPlayer(iI).getID())
			else:
				if (gc.getPlayer(iI).isAlive()):		
					listPlot.append(gc.getPlayer(iI).getStartingPlot())
					listPlayer.append(gc.getPlayer(iI).getID())				

		#only do team one it will be good 		
		listCurrentPlayer = listPlayer
		for iI in range(gc.getMAX_CIV_PLAYERS()):
			bDoThis = False
			if isBTG:
				if (gc.getPlayer(iI).isAlive() and not gc.getPlayer(iI).isSpectator()):
					bDoThis = True
			else:
				if (gc.getPlayer(iI).isAlive()):
					bDoThis = True				
			if bDoThis:
				if (gc.getPlayer(iI).getTeam() == teamOne and gc.getPlayer(iI).getStartingPlot().getY() >= halfHeight):						
					random.shuffle(listCurrentPlayer)
					iRoll = listCurrentPlayer[0]
					#while ((gc.getPlayer(iRoll).getStartingPlot().getY() >= halfHeight) or (iRoll == iI)):#I roll until it's a bottom tile
					#2.23 - Problem is, on the last "fix" you can send a teammate back on top
					while ((gc.getPlayer(iRoll).getStartingPlot().getY() >= halfHeight) or (iRoll == iI) or gc.getPlayer(iRoll).getTeam() == teamOne):#I roll until it's a bottom tile
						random.shuffle(listCurrentPlayer)
						iRoll = listCurrentPlayer[0]

					spotA = gc.getPlayer(iI).getStartingPlot()
					spotB = gc.getPlayer(iRoll).getStartingPlot()
					gc.getPlayer(iI).setStartingPlot(spotB,True)
					gc.getPlayer(iRoll).setStartingPlot(spotA,True)	
