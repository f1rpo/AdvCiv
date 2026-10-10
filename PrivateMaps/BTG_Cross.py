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
#	BugUtil.debug("Team_Battleground: getDescription")
	return "Modified version of the BTG_Cross map from the Beyond the Game mod by Penny. Adapted for AdvCiv-SAS: upscaled, non-AdvCiv options removed, and SAS48/high player counts supported."

def getDescriptionTitle():
	return "The map present each player with it's own little hub of land but with a 'prize' hub in the middle, players in the same team are lined up to take it"

def getDescriptionTitleTwo():
	return "The underlying design of the map is based on Grid and the little hub looks as such"

def getDescriptionMain():
	return "Default design keeps one more hub than players (empty central prize hub), with AdvCiv-SAS scaling for large player counts and SAS48 world size."

def getDescriptionSecond():#Script tip : (on TOP)
	return "Make sure to gage how good the middle hub is (2 level of options to improve it) and how exposed you are to sea attacks"	

def getDescriptionThird():#Option : (at the bottom)"
	return ""

def getDescriptionScenario():#Scenario : (at the bottom)"
	return "Best for 2v2 Teamers, mostly Renaissance. Best if the threat of sea attack is present quickly but not immediate"

def getDescriptionBalance():#Balance : (at the bottom)"
	return "If teamers don't improve land of empty hub too much (positioning too good), but definitely do for CTON to maximise appetite"		

def isAdvancedMap():
	"This map should not show up in simple mode"
	# <!-- custom: keep this at 0 so BTG_Cross appears in Simple Game mode map lists; return 1 hides it from simple mode. (GPT-5.3-Codex (summarized)) -->
	return 0

def getNumCustomMapOptions():
	return 12

def getNumHiddenCustomMapOptions():
	return 0

def getCustomMapOptionName(argsList):
	[iOption] = argsList
	option_names = {
		0:	"TXT_KEY_MAP_WORLD_WRAP",
		1:	"Mirrored Hubs",
		2:	"TXT_KEY_MAP_SCRIPT_SPOKE_WIDTH",
		3:	"Regions",
		4:	"Oil and Aluminium",
		5:	"Elephant",
		6:	"Precious Metal",
		7:	"Positioning",
		8:	"Forest Density",
		9:	"Start Distance",
		10: "Central Hub",
		11:	"Desert"
		}
	if not option_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	translated_text = unicode(CyTranslator().getText(option_names[iOption], ()))
	return translated_text

def getNumCustomMapOptionValues(argsList):
	[iOption] = argsList
	# <!-- custom: SAS exposed BTG's dormant fourth Positioning label, but Crossed executed the same mapping as Together.
	# Restore the original three selectable modes until a distinct Crossed policy exists. See KI#272. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	option_values = {
		0:	3,
		1:	4,
		2:	6,
		3:	4,
		4:	2,
		5:	2,
		6:	2,
		7:	3,
		8:	4,
		9:	4,
		10: 3,
		11: 2
		}
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
			0: "4 Regions",
			1: "5 Regions",
			2: "9 Regions",
			3: "7 Regions",			
			},
		4:	{
			0: "Standard",
			1: "Within 5 tiles"
			},
		5:	{
			0: "Standard",
			1: "Within 7 tiles"
			},
		6:	{
			0: "Standard",
			1: "Within 7 tiles"
			},
		7:	{
			0: "Numerical Order",
			1: "Full position shuffle",
			2: "Outside positions, together"
			},
		8:	{
			0: "High - 60% (Game base)",
			1: "Standard - 40% (map base)",
			2: "Scarce - 25%",
			3: "Rare - 10%"
			},
		9:	{
			0: "Normal - 16 Tiles away",
			1: "Far - 20 Tiles away",
			2: "Close - 12 Tiles away",
			3: "Super Close - 8 Tiles away"
			},	
		10:	{
			0: "Normal Hub",
			1: "Wealthy Hub",			
			2: "Amazing Hub"
			},
		11:	{
			0: "Normal",
			1: "None - replace by grassland"
			}
		}
	if not selection_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	translated_text = unicode(CyTranslator().getText(selection_names[iOption][iSelection], ()))
	return translated_text

def getCustomMapOptionDefault(argsList):
	[iOption] = argsList
	option_defaults = {
		0:	2,
		1:	0,
		2:	5,
		3:	1,
		4:	1,
		5:	0,
		6:	0,
		7:	2,
		8:	1,
		9:	2,
		10:	0,
		11:	0
		}
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
		9:	true,
		10:	true,
		11: true
		}
	return option_random[iOption]

def getWrapX():
	map = CyMap()
	return (map.getCustomMapOption(0) == 1 or map.getCustomMapOption(0) == 2)

def getWrapY():
	map = CyMap()
	return (map.getCustomMapOption(0) == 2)

def getGridSize(argsList):
	# 2.23 - Simplified, enhanced
	if (argsList[0] == -1): # (-1,) is passed to function on loads
		return []
	[eWorldSize] = argsList
	iDistanceOption = CyMap().getCustomMapOption(9)
	if (iDistanceOption == 0):
		# <!-- custom: BTG Cross dimensions were too small for each base world size in current tuning.
		# Use direct grid rows instead of hiding the change behind a shifted calibration formula, so each base size is visible and tunable; SAS24+ calibrates from the explicit Huge row. (GPT-5.5) -->
		grid_sizes = {
			WorldSizeTypes.WORLDSIZE_ARENA: (7, 7),
			WorldSizeTypes.WORLDSIZE_DUEL: (8, 8),
			WorldSizeTypes.WORLDSIZE_TINY: (11, 11),
			WorldSizeTypes.WORLDSIZE_SMALL: (13, 13),
			WorldSizeTypes.WORLDSIZE_STANDARD: (16, 16),
			WorldSizeTypes.WORLDSIZE_LARGE: (19, 19),
			WorldSizeTypes.WORLDSIZE_HUGE: (24, 24),
		}
	elif (iDistanceOption == 1):
		grid_sizes = {
			WorldSizeTypes.WORLDSIZE_ARENA: (8, 8),
			WorldSizeTypes.WORLDSIZE_DUEL: (10, 10),
			WorldSizeTypes.WORLDSIZE_TINY: (13, 13),
			WorldSizeTypes.WORLDSIZE_SMALL: (17, 17),
			WorldSizeTypes.WORLDSIZE_STANDARD: (20, 20),
			WorldSizeTypes.WORLDSIZE_LARGE: (24, 24),
			WorldSizeTypes.WORLDSIZE_HUGE: (30, 30),
		}
	elif (iDistanceOption == 2):
		grid_sizes = {
			WorldSizeTypes.WORLDSIZE_ARENA: (5, 5),
			WorldSizeTypes.WORLDSIZE_DUEL: (6, 6),
			WorldSizeTypes.WORLDSIZE_TINY: (8, 8),
			WorldSizeTypes.WORLDSIZE_SMALL: (10, 10),
			WorldSizeTypes.WORLDSIZE_STANDARD: (12, 12),
			WorldSizeTypes.WORLDSIZE_LARGE: (14, 14),
			WorldSizeTypes.WORLDSIZE_HUGE: (18, 18),
		}
	else:
		grid_sizes = {
			WorldSizeTypes.WORLDSIZE_ARENA: (3, 3),
			WorldSizeTypes.WORLDSIZE_DUEL: (4, 4),
			WorldSizeTypes.WORLDSIZE_TINY: (5, 5),
			WorldSizeTypes.WORLDSIZE_SMALL: (7, 7),
			WorldSizeTypes.WORLDSIZE_STANDARD: (8, 8),
			WorldSizeTypes.WORLDSIZE_LARGE: (9, 9),
			WorldSizeTypes.WORLDSIZE_HUGE: (12, 12),
		}

	iWorld = int(eWorldSize)
	# <!-- custom: BTG Cross used the Huge grid for every world size of Huge and below, including our SAS ARENA size, so they collapsed to one effective map size such as 60x60 plots.
	# Scale them from the Huge anchor and keep the existing SAS24+ formula below. See KI#137. (GPT-5.5) -->
	if iWorld <= WorldSizeTypes.WORLDSIZE_HUGE:
		return sas_lookup_world_size(eWorldSize, grid_sizes)

	iDefaultPlayers = sas_world_default_players(iWorld)
	iTargetPlayers = int(math.ceil(float(iDefaultPlayers) * 1.5))
	(iHugeW, iHugeH) = grid_sizes[WorldSizeTypes.WORLDSIZE_HUGE]
	return sas_calibrate_grid_from_anchor(iHugeW, iHugeH, sas_world_default_players(WorldSizeTypes.WORLDSIZE_HUGE), iTargetPlayers)

def beforeGeneration():
	global iNumRegions
	global regions_in_use
	global remaining_regions
	global remaining_regionsTwo#2.21z Cheezy	

	#2.22
	global isBTPon
	# BTG-only blocks are disabled in AdvCiv-SAS.
	isBTPon = False

	gc = CyGlobalContext()
	map = CyMap()
	dice = gc.getGame().getMapRand()
	iW = map.getGridWidth()
	iH = map.getGridHeight()
	iPlayersCount = gc.getGame().countCivPlayersEverAlive()
	global iPlayers

	#2.18 - 2.22
	global iNumSpectators 
	if isBTPon:
		iNumSpectators = gc.getGame().countCivPlayersEverSpectator()
	else:
		iNumSpectators = 0		

	# Number of regions
	iRegionsBase = 5
	if (CyMap().getCustomMapOption(3) == 0):
		iRegionsBase = 4
	elif (CyMap().getCustomMapOption(3) == 1):	
		iRegionsBase = 5
	elif (CyMap().getCustomMapOption(3) == 2):	
		iRegionsBase = 9
	elif (CyMap().getCustomMapOption(3) == 3):#Bigger grid for 7
		iRegionsBase = 7

	# Do the real player count now, trick for spoiling number of player / region
	#if (CyMap().getCustomMapOption(7) == 1):
	#	iPlayers = iNumRegions
	#else:

	iPlayers = gc.getGame().countCivPlayersEverAlive()
	# Error catching.
	if iPlayers < 1 or iPlayers > gc.getMAX_CIV_PLAYERS():
		return None
	# Keep legacy cross-region templates to preserve original map identity.
	iNumRegions = iRegionsBase

	# Some regions may go unused. We need to track the ones that have been used.
	regions_in_use = []
	remaining_regions = []
	remaining_regionsTwo = []#2.21z Cheezey	
	for loopy in range(iNumRegions):
		remaining_regions.append(loopy)
		remaining_regionsTwo.append(loopy)#2.21z Cheezey		

	# Templates are nested by keys: {NumRegions: {RegionID: [WestLon, EastLon, SouthLat, NorthLat]}}
	templates =  {4: {0: [0.0, 0.334, 0.334, 0.667],
				1: [0.667, 1.0, 0.334, 0.667],
				2: [0.334, 0.667, 0.0, 0.334],
				3: [0.334, 0.667, 0.667, 1.0]},
			5: {0: [0.0, 0.334, 0.334, 0.667],
				1: [0.667, 1.0, 0.334, 0.667],
				2: [0.334, 0.667, 0.0, 0.334],
				3: [0.334, 0.667, 0.667, 1.0],
				4: [0.334, 0.667, 0.334, 0.667]},
			7: {0: [0.000, 0.250, 0.500, 0.750],
				1: [0.250, 0.500, 0.500, 0.750],
				2: [0.750, 1.000, 0.500, 0.750],
				3: [0.500, 0.750, 0.000, 0.250],
				4: [0.500, 0.750, 0.250, 0.500],
				5: [0.500, 0.750, 0.750, 1.000],
				6: [0.500, 0.750, 0.500, 0.750]},#Trick is to finish by the one you want to fill in last
			9: {0: [0.0, 0.334, 0.334, 0.667],
				1: [0.667, 1.0, 0.334, 0.667],
				2: [0.334, 0.667, 0.0, 0.334],
				3: [0.334, 0.667, 0.667, 1.0],
				4: [0.334, 0.667, 0.334, 0.667],
				5: [0.0, 0.334, 0.0, 0.334],
				6: [0.667, 1.0, 0.0, 0.334],
				7: [0.0, 0.334, 0.667, 1.0],
				8: [0.667, 1.0, 0.667, 1.0]},
		}
	# End of template data.

	# List region_coords: [WestLon, EastLon, SouthLat, NorthLat]
	global region_coords
	region_coords = templates[iNumRegions]

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

		# Add the land (two fractals per region to ensure cohesion).
		global region_coords
		global regions_in_use
		global remaining_regions
		global region_duplicated
		global other_regions

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

		# Generate spokes
		map = CyMap()
		spoke_width = map.getCustomMapOption(2)
		if spoke_width > 0:

			for regionLoop in range(len(regions_in_use)):

				#2.10o
				#if regionLoop >= 0:

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

		# Add the land (two fractals per region to ensure cohesion).
		if (CyMap().getCustomMapOption(1) >= 1):

			#duplicate land for other used regions
			other_regions = []
			# <!-- custom: Record each physical mirrored hub once, including the source once, then crop every copy to the largest common in-bounds footprint.
			# BTG duplicated the source ID and assumed rounded source/target dimensions were equal, reading neighboring map data when they differed. See KI#273 and KI#274. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
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

	# Check for valid number of players.
	if iPlayers > 0 and iPlayers <= CyGlobalContext().getMAX_CIV_PLAYERS():
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

	#2.10 -- Adding this --	featuregen.__init__(iIcePercent=0)	
	if (CyMap().getCustomMapOption(8) == 3):
		featuregen.__init__(iJunglePercent=0, iForestPercent=90, jungle_grain=5, forest_grain=6)#, iIcePercent=0)
	elif (CyMap().getCustomMapOption(8) == 2):
		featuregen.__init__(iJunglePercent=0, iForestPercent=75, jungle_grain=5, forest_grain=6)#, iIcePercent=0)
	elif (CyMap().getCustomMapOption(8) == 1):
		featuregen.__init__(iJunglePercent=0, iForestPercent=60, jungle_grain=5, forest_grain=6)#, iIcePercent=0)
	else: 		
		featuregen.__init__(iJunglePercent=0, iForestPercent=40, jungle_grain=5, forest_grain=6)#, iIcePercent=0)	

	featuregen.addFeatures()

	## 2.10 do it after the general calc otherwise doesn't work
	for plotIndex in range(iW * iH):
		pPlot = map.plotByIndex(plotIndex)
		if pPlot.isWater():
			pPlot.setFeatureType(-1, -1)# -1 is nothing
	#2.10 end

	return 0

def minStartingDistanceModifier():
	return 20

def assignStartingPlots():

	# Custom start plot finder
	global iNumRegions
	global region_coords
	global regions_in_use
	global region_duplicated#2.15
	global other_regions#2.15
	gc = CyGlobalContext()
	map = CyMap()
	dice = gc.getGame().getMapRand()
	iW = map.getGridWidth()
	iH = map.getGridHeight()
	#iPlayers = gc.getGame().countCivPlayersEverAlive()
	#I've already declared this

	# Error catching.
	if iPlayers < 1 or iPlayers > gc.getMAX_CIV_PLAYERS():
		CyPythonMgr().allowDefaultImpl()
		return
	# <!-- custom: SAS reused Cross's fixed 4/5/7/9 physical hubs with modulo assignment for larger player counts, producing duplicate regions and exact duplicate starts under positional mirroring.
	# Fall back before assigning any custom start when unique hub capacity is insufficient. See KI#271. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if len(regions_in_use) < iPlayers:
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
	# <!-- custom: Preserve the actual ordered player IDs before consuming the list for shuffling.
	# BTG's Numerical/Together branches substituted compact loop positions, assigning dead open slots and omitting sparse alive IDs. See KI#267. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	orderedPlayers = player_list[:]
	shuffledPlayers = []
	for playerLoopTwo in range(gc.getGame().countCivPlayersEverAlive()):
		iChoosePlayer = dice.get(len(player_list), "Shuffling Regions - Grid PYTHON")
		shuffledPlayers.append(player_list[iChoosePlayer])
		del player_list[iChoosePlayer]

	# Now assign the start plots!
	plot_assignments = {}
	min_dist = []

	dX = 0
	dY = 0

	if (CyMap().getCustomMapOption(1) == 1 or CyMap().getCustomMapOption(1) == 0):#2.21z

		# Loop through players/regions.
		for assignLoop in range(iPlayers):
		# for assignLoop in range(iNumRegions):
			#---- Below 2 lines normal code, full shuffle ----#
			#playerID = shuffledPlayers[assignLoop]#2.10test
			#reg = best_regions[assignLoop]

			iNumTeams = gc.getGame().countCivTeamsEverAlive()

			if (CyMap().getCustomMapOption(7) == 0):#Numerical Order
				playerID = orderedPlayers[assignLoop]
				reg = assignLoop % iNumRegions

			elif (CyMap().getCustomMapOption(7) == 1):#Full shuffle / normal
				playerID = shuffledPlayers[assignLoop]
				reg = best_regions[assignLoop % len(best_regions)]

			elif iNumTeams == iPlayers:#Shuffle but not slot 5 in 5 players CTON
				playerID = shuffledPlayers[assignLoop]
				reg = assignLoop % iNumRegions

			elif iNumTeams == 2 and iNumSpectators > 0:	
				#2.18 -- It wasn't working with Spectator, I kinda force the scenario for when spectator watches 2 teams
				playerID = shuffledPlayers[assignLoop]
				reg = assignLoop % iNumRegions			

			else:#Together
				playerID = orderedPlayers[assignLoop]
				reg = assignLoop % iNumRegions

			if reg not in region_data:
				if len(best_regions) > 0:
					reg = best_regions[assignLoop % len(best_regions)]
				else:
					reg = region_data.keys()[0]

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
				iPass = 0
				while (true):
					iBestValue = 0
					pBestPlot = None
					# Loop through best areas in this region
					for areaLoop in range(iNumAreas):
						areaID = area_list[areaLoop]
						player = gc.getPlayer(playerID)
						player.AI_updateFoundValues(True)
						iRange = player.startingPlotRange()
						validFn = None
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

		for assignLoop in range(iPlayers):# -Penny ; Not necessary for condition
		# for assignLoop in range(iNumRegions):
			#---- Below 2 lines normal code, full shuffle ----#
			#playerID = shuffledPlayers[assignLoop]#2.10test
			#reg = best_regions[assignLoop]

			iNumTeams = gc.getGame().countCivTeamsEverAlive()

			if (CyMap().getCustomMapOption(7) == 0):#Numerical Order
				playerID = orderedPlayers[assignLoop]
				reg = assignLoop % iNumRegions

			elif (CyMap().getCustomMapOption(7) == 1):#Full shuffle / normal
				playerID = shuffledPlayers[assignLoop]
				reg = best_regions[assignLoop % len(best_regions)]

			elif iNumTeams == iPlayers:#Shuffle but not slot 5 in 5 players CTON
				playerID = shuffledPlayers[assignLoop]
				reg = assignLoop % iNumRegions

			elif iNumTeams == 2 and iNumSpectators > 0:	
				#2.18 -- It wasn't working with Spectator, I kinda force the scenario for when spectator watches 2 teams
				playerID = shuffledPlayers[assignLoop]
				reg = assignLoop % iNumRegions			

			else:#Together
				playerID = orderedPlayers[assignLoop]
				reg = assignLoop % iNumRegions

			if reg not in region_data:
				if len(best_regions) > 0:
					reg = best_regions[assignLoop % len(best_regions)]
				else:
					reg = region_data.keys()[0]

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
				iPass = 0
				while (true):
					iBestValue = 0
					pBestPlot = None
					# Loop through best areas in this region
					for areaLoop in range(iNumAreas):
						areaID = area_list[areaLoop]
						player = gc.getPlayer(playerID)
						player.AI_updateFoundValues(True)
						iRange = player.startingPlotRange()
						validFn = None
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

	balancer.normalizeAddExtras()

	# V3 by Axius: Give a land oil and aluminum to each player

	gc = CyGlobalContext()
	map = CyMap()
	oil = getInfoTypeOrFail("BONUS_OIL")
	alu = getInfoTypeOrFail("BONUS_ALUMINUM")
	elephantsBonus = getInfoTypeOrFail("BONUS_ELEPHANTS")
	gold = getInfoTypeOrFail("BONUS_GOLD")
	silver = getInfoTypeOrFail("BONUS_SILVER")
	gemstonesBonus = getInfoTypeOrFail("BONUS_GEMSTONES")
	random.seed(gc.getGame().getMapRand().get(30000, "Shuffle Plots - PYTHON"))

	for i in range(0,gc.getMAX_CIV_PLAYERS()):
		#if gc.getPlayer(i).isAlive():
		if gc.getPlayer(i).isEverAlive():#2.18 Exclude Spectator

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
						if p.getBonusType(-1) == elephantsBonus:
							has_ivory = True
						if p.getBonusType(-1) == gold:
							has_precious = True
						if p.getBonusType(-1) == silver:
							has_precious = True
						if p.getBonusType(-1) == gemstonesBonus:
							has_precious = True				

			for dx in range(-5,5):#1 notch closer than other maps
				for dy in range(-5,5):
					p = map.plot(startx+dx,starty+dy)
					#2.22 This is too restrictive for this map
					if (( abs(dx) >= 4 or abs(dy) >=4) and not p.isNone()) and (not p.isImpassable()) and (not p.isWater()):#1 notch closer than other maps
						if ((abs(dx) >= 1 and abs(dy) >= 1)):#too tight otherwise on this map
							if (p.getBonusType(-1) == BonusTypes.NO_BONUS):
								plotsboundaries.append(p)					

			if (CyMap().getCustomMapOption(4) == 1):
				if not has_oil:

					random.shuffle(plotsclose) 
					for p in plotsclose:
						if (p.getBonusType(-1) == BonusTypes.NO_BONUS) and p.canHaveBonus(oil, True):
							p.setBonusType(oil)
							has_oil = True
							break
					if not has_oil:
						p = plotsclose[0]
						p.setPlotType(PlotTypes.PLOT_LAND, True, True)
						p.setTerrainType(getInfoTypeOrFail("TERRAIN_GRASS"), True, True)
						p.setBonusType(oil)

				if not has_alu:

					random.shuffle(plotsclose) 
					for p in plotsclose:
						if (p.getBonusType(-1) == BonusTypes.NO_BONUS) and p.canHaveBonus(alu, True):
							p.setBonusType(alu)
							has_alu = True or CyMap().getCustomMapOption(4) == 0
							break
					if not has_alu:
						p = plotsclose[0]
						p.setPlotType(PlotTypes.PLOT_HILLS, True, True)
						p.setTerrainType(getInfoTypeOrFail("TERRAIN_GRASS"), True, True)
						p.setBonusType(alu)

			if (CyMap().getCustomMapOption(5) == 1):
				if not has_ivory:

					random.shuffle(plotsfurther) 
					for p in plotsfurther:
						if (p.getBonusType(-1) == BonusTypes.NO_BONUS) and p.canHaveBonus(elephantsBonus, True):
							p.setBonusType(elephantsBonus)
							has_ivory = True
							break
					if not has_ivory:
						p = plotsfurther[0]
						p.setPlotType(PlotTypes.PLOT_LAND, True, True)
						p.setTerrainType(getInfoTypeOrFail("TERRAIN_GRASS"), True, True)
						p.setBonusType(elephantsBonus)

			if (CyMap().getCustomMapOption(6) == 1):
				if not has_precious:

					random.shuffle(plotsboundaries) 
					for p in plotsboundaries:
						if (p.getBonusType(-1) == BonusTypes.NO_BONUS) and p.canHaveBonus(silver, True):
							p.setBonusType(silver)
							has_precious = True
							break
					# <!-- custom: The inherited forced-Silver fallback indexed an unproven nonempty boundary list.
					# If the boundary list is empty, use an empty legal land plot already collected within seven tiles; if neither list has a plot, safely leave the optional normalization unmet. See KI#329. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
					if not has_precious:
						plotsforced = plotsboundaries
						if len(plotsforced) == 0:
							plotsforced = [pLoop for pLoop in plotsfurther if pLoop.getBonusType(-1) == BonusTypes.NO_BONUS]
					if not has_precious and len(plotsforced) > 0:
						p = plotsforced[0]
						p.setPlotType(PlotTypes.PLOT_LAND, True, True)
						p.setTerrainType(getInfoTypeOrFail("TERRAIN_DESERT"), True, True)
						p.setBonusType(silver)

	if (CyMap().getCustomMapOption(1) == 3):
		mirrorizeMap() #2020 06 - BTP 2.15 - Restart feature

	if (CyMap().getCustomMapOption(10)>= 1):#middle is great Akira
		'''p = map.plot(map.getGridWidth()/2,map.getGridHeight()/2)
		p.setBonusType(gold)'''

		pig = getInfoTypeOrFail("BONUS_PIG")	
		wheat = getInfoTypeOrFail("BONUS_WHEAT")	
		bronze = getInfoTypeOrFail("BONUS_COPPER")	
		corn = getInfoTypeOrFail("BONUS_MAIZE")	

		iW = map.getGridWidth()
		iH = map.getGridHeight()

		midW = iW/2
		midH = iH /2		
		if (CyMap().getCustomMapOption(3) == 3):#Bigger grid for 7	- 2.23
			midW = iW * 625 / 1000
			midH = iH * 625 / 1000

		plotsrange = []
		for dx in range(-4,4):
			for dy in range(-4,4):
				p = map.plot(midW+dx,midH+dy)	
				if ((dx != 0) or (dy != 0)) and (not p.isNone()) and (not p.isImpassable()) and (not p.isWater()):
					if (p.getBonusType(-1) == BonusTypes.NO_BONUS):
						plotsrange.append(p)				

		random.shuffle(plotsrange) 
		for p in plotsrange:
			if (p.canHaveBonus(pig, True)):
				p.setBonusType(pig)
				plotsrange.remove(p)
				break
		for p in plotsrange:
			if (p.canHaveBonus(bronze, True)):
				p.setBonusType(bronze)
				plotsrange.remove(p)
				break
		for p in plotsrange:
			p.setBonusType(silver)
			plotsrange.remove(p)
			break

		if (CyMap().getCustomMapOption(10)>= 2):

			for p in plotsrange:
				if (p.canHaveBonus(corn, True)):
					p.setBonusType(corn)
					plotsrange.remove(p)
					break		
			for p in plotsrange:
				p.setBonusType(gemstonesBonus)
				plotsrange.remove(p)
				break						

		'''p = map.plot(map.getGridWidth()/2+2,map.getGridHeight()/2-1)
		p.setBonusType(gems)'''

	#2.15
	iW = map.getGridWidth()
	iH = map.getGridHeight()		

	if (CyMap().getCustomMapOption(11) >= 1):
		for iX in range(iW):
			for iY in range(iH):
				pPlot = map.plot(iX, iY)
				if pPlot.getTerrainType() == getInfoTypeOrFail("TERRAIN_DESERT") and pPlot.getBonusType(-1) == -1 and pPlot.getFeatureType() == -1:
					pPlot.setTerrainType(getInfoTypeOrFail("TERRAIN_GRASS"), True, True)

	return None

def addBonusType(argsList):
	[iBonusType] = argsList
	gc = CyGlobalContext()
	type_string = gc.getBonusInfo(iBonusType).getType()

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
	# <!-- custom: Mirror only the common source/target footprint chosen during plot generation.
	# Rounded third-based Cross regions can differ by one row/column; the inherited full-target loops read outside the source hub. See KI#273. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
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
	return False
