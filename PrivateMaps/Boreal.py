#
#	FILE:	 Boreal.py
#	AUTHOR:  Bob Thomas (Sirian)
#	PURPOSE: Regional map script - Boreal forest region / tundra.
#-----------------------------------------------------------------------------
#	Copyright (c) 2007 Firaxis Games, Inc. All rights reserved.
#-----------------------------------------------------------------------------
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: KI#105 context: this script had first-launch bonus spawning instability (e.g. Deer missing).
# Keep generator state minimal and favor function-local handles where possible; only keep persistent class state required by CvMapGeneratorUtil base flow (map/grid/mapRand/fractals). See KI#105. (GPT-5.3-Codex) -->
#

from CvPythonExtensions import *
from SASUtils import getInfoTypeOrFail
import CvUtil
import CvMapGeneratorUtil
import random
import sys
from math import sqrt
from CvMapGeneratorUtil import FractalWorld
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator
from SAS_WorldSizeUtils import *

def getDescription():
	return "TXT_KEY_MAP_SCRIPT_BOREAL_DESCR"

def isAdvancedMap():
	"This map should not show up in simple mode"
	# <!-- custom: keep Boreal out of Simple Game by design; return 1 hides it there while keeping the script available in advanced/custom setup. (GPT-5.3-Codex) -->
	return 1

def isClimateMap():
	return 0

def isSeaLevelMap():
	return 0

def getNumCustomMapOptions():
	return 2

def getNumHiddenCustomMapOptions():
	return 1

def getCustomMapOptionName(argsList):
	[iOption] = argsList
	option_names = {
		0: "Deer Percentage",
		1: "TXT_KEY_MAP_WORLD_WRAP"
	}
	if not option_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	translated_text = unicode(CyTranslator().getText(option_names[iOption], ()))
	return translated_text

def getNumCustomMapOptionValues(argsList):
	[iOption] = argsList
	option_values = {
		0: 6,
		1: 3
	}
	if not option_values.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	return option_values[iOption]

def getCustomMapOptionDescAt(argsList):
	[iOption, iSelection] = argsList
	selection_names = {
		0: {
			0: "0%",
			1: "10%",
			2: "25%",
			3: "50%",
			4: "75% (recommended)",
			5: "100%"
		},
		1: {
			0: "TXT_KEY_MAP_WRAP_FLAT",
			1: "TXT_KEY_MAP_WRAP_CYLINDER",
			2: "TXT_KEY_MAP_WRAP_TOROID"
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
		0: 4, # 75%
		1: 0
	}
	if not option_defaults.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	return option_defaults[iOption]

def isRandomCustomMapOption(argsList):
	[iOption] = argsList
	option_random = {
		0: false,
		1: false
	}
	if not option_random.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	return option_random[iOption]

def getWrapX():
	map = CyMap()
	return (map.getCustomMapOption(1) == 1 or map.getCustomMapOption(1) == 2)

def getWrapY():
	map = CyMap()
	return (map.getCustomMapOption(1) == 2)

def getDeerPercent():
	map = CyMap()
	deer_percents = (0, 10, 25, 50, 75, 100)
	iSelection = map.getCustomMapOption(0)
	if iSelection < 0 or iSelection >= len(deer_percents):
		return deer_percents[4]
	return deer_percents[iSelection]

def getTopLatitude():
	return 90

def getBottomLatitude():
	return 70

def getGridSize(argsList):
	"Because this is such a land-heavy map, override getGridSize() to make the map smaller"
	grid_sizes = {
		WorldSizeTypes.WORLDSIZE_ARENA: (5,3),
		WorldSizeTypes.WORLDSIZE_DUEL: (6,4),
		WorldSizeTypes.WORLDSIZE_TINY: (8,5),
		WorldSizeTypes.WORLDSIZE_SMALL: (10,6),
		WorldSizeTypes.WORLDSIZE_STANDARD: (13,8),
		WorldSizeTypes.WORLDSIZE_LARGE: (16,10),
		WorldSizeTypes.WORLDSIZE_HUGE: (21,13)
	}

	if (argsList[0] == -1): # (-1,) is passed to function on loads
		return []
	[eWorldSize] = argsList
	return sas_lookup_world_size_with_calibrated_sas(eWorldSize, grid_sizes)

def minStartingDistanceModifier():
	return -27

def beforeGeneration():
	gc = CyGlobalContext()
	map = CyMap()
	dice = gc.getGame().getMapRand()
	iW = map.getGridWidth()
	iH = map.getGridHeight()
	global food
	food = CyFractal()
	food.fracInit(iW, iH, 7, dice, 0, -1, -1)

# Subclass
class BorealFractalWorld(CvMapGeneratorUtil.FractalWorld):
	def generatePlotTypes(self, water_percent=78, shift_plot_types=True, grain_amount=3):
		# Check for changes to User Input variances.
		self.checkForOverrideDefaultUserInputVariances()

		self.hillsFrac.fracInit(self.iNumPlotsX, self.iNumPlotsY, 2, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
		self.peaksFrac.fracInit(self.iNumPlotsX, self.iNumPlotsY, 5, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)

		water_percent += self.seaLevelChange
		water_percent = min(water_percent, self.seaLevelMax)
		water_percent = max(water_percent, self.seaLevelMin)

		iWaterThreshold = self.continentsFrac.getHeightFromPercent(water_percent)
		iHillsBottom1 = self.hillsFrac.getHeightFromPercent(82)
		iHillsBottom2 = self.peaksFrac.getHeightFromPercent(90)
		iPeakThreshold = self.hillsFrac.getHeightFromPercent(90)
		iSecondPeakThreshold = self.peaksFrac.getHeightFromPercent(97)

		for x in range(self.iNumPlotsX):
			for y in range(self.iNumPlotsY):
				i = y*self.iNumPlotsX + x
				val = self.continentsFrac.getHeight(x,y)
				if val <= iWaterThreshold:
					self.plotTypes[i] = PlotTypes.PLOT_OCEAN
				else:
					hillVal = self.hillsFrac.getHeight(x,y)
					peakVal = self.peaksFrac.getHeight(x,y)
					if hillVal >= iHillsBottom1:
						if (hillVal >= iPeakThreshold):
							self.plotTypes[i] = PlotTypes.PLOT_PEAK
						else:
							self.plotTypes[i] = PlotTypes.PLOT_HILLS
					elif peakVal >= iHillsBottom2:
						if peakVal >= iSecondPeakThreshold:
							self.plotTypes[i] = PlotTypes.PLOT_PEAK
						else:
							self.plotTypes[i] = PlotTypes.PLOT_HILLS
					else:
						self.plotTypes[i] = PlotTypes.PLOT_LAND

		if shift_plot_types:
			self.shiftPlotTypes()

		return self.plotTypes

def generatePlotTypes():
	NiTextOut("Setting Plot Types (Python Boreal) ...")
	global fractal_world
	fractal_world = BorealFractalWorld()
	fractal_world.initFractal(continent_grain=3, rift_grain = -1, has_center_rift = False, polar = False)
	plot_types = fractal_world.generatePlotTypes(water_percent = 12)
	return plot_types

# subclass TerrainGenerator to redefine everything. This is a regional map.
class BorealTerrainGenerator(CvMapGeneratorUtil.TerrainGenerator):
	def __init__(self, fracXExp=-1, fracYExp=-1):
		# Keep only fractal objects as persistent state; other handles/params stay local.
		self.map = CyMap()
		self.iWidth = self.map.getGridWidth()
		self.iHeight = self.map.getGridHeight()
		self.mapRand = CyGlobalContext().getGame().getMapRand()
		self.ice=CyFractal()
		self.plains=CyFractal()
		self.initFractals(fracXExp, fracYExp)

	def initFractals(self, fracXExp, fracYExp):
		gc = CyGlobalContext()
		map = CyMap()
		mapRand = gc.getGame().getMapRand()
		iWidth = map.getGridWidth()
		iHeight = map.getGridHeight()
		iFlags = 0  # Disallow FRAC_POLAR flag, to prevent "zero row" problems.
		self.ice.fracInit(iWidth, iHeight, 1, mapRand, iFlags, fracXExp, fracYExp)
		self.plains.fracInit(iWidth, iHeight, 4, mapRand, iFlags, fracXExp, fracYExp)

	def generateTerrainAtPlot(self,iX,iY):
		map = CyMap()
		gc = CyGlobalContext()
		if (map.plot(iX, iY).isWater()):
			return map.plot(iX, iY).getTerrainType()
		else:
			iceVal = self.ice.getHeight(iX, iY)
			plainsVal = self.plains.getHeight(iX, iY)
			iIce = self.ice.getHeightFromPercent(86)
			iIceEdge = self.plains.getHeightFromPercent(94)
			iPlains = self.plains.getHeightFromPercent(21)
			terrainPlains = getInfoTypeOrFail("TERRAIN_PLAINS")
			terrainTundra = getInfoTypeOrFail("TERRAIN_TUNDRA")
			terrainIce = getInfoTypeOrFail("TERRAIN_SNOW")
			if iceVal >= iIce:
				terrainVal = terrainIce
			elif plainsVal >= iIceEdge:
				terrainVal = terrainIce
			elif plainsVal <= iPlains:
				terrainVal = terrainPlains
			else:
				terrainVal = terrainTundra

		if (terrainVal == TerrainTypes.NO_TERRAIN):
			return map.plot(iX, iY).getTerrainType()

		return terrainVal

def generateTerrainTypes():
	NiTextOut("Generating Terrain (Python Boreal) ...")
	terraingen = BorealTerrainGenerator()
	terrainTypes = terraingen.generateTerrain()
	return terrainTypes

class BorealFeatureGenerator(CvMapGeneratorUtil.FeatureGenerator):
	def __init__(self, forest_grain=5, fracXExp=-1, fracYExp=-1):
		gc = CyGlobalContext()
		self.map = CyMap()
		self.iGridW = self.map.getGridWidth()
		self.iGridH = self.map.getGridHeight()
		self.mapRand = gc.getGame().getMapRand()
		# Keep only forest fractal as persistent state; everything else local.
		self.forests = CyFractal()

		iForestGrain = forest_grain + gc.getWorldInfo(self.map.getWorldSize()).getFeatureGrainChange()

		self.__initFractals(iForestGrain, fracXExp, fracYExp)

	def __initFractals(self, iForestGrain, fracXExp, fracYExp):
		gc = CyGlobalContext()
		map = CyMap()
		mapRand = gc.getGame().getMapRand()
		iGridW = map.getGridWidth()
		iGridH = map.getGridHeight()
		iFlags = 0
		self.forests.fracInit(iGridW, iGridH, iForestGrain, mapRand, iFlags, fracXExp, fracYExp)

	def addFeaturesAtPlot(self, iX, iY):
		gc = CyGlobalContext()
		pPlot = CyMap().sPlot(iX, iY)

		if pPlot.isPeak() or pPlot.isWater():
			pass

		else:
			if pPlot.isRiverSide() and pPlot.isFlatlands():
				if pPlot.getTerrainType() == getInfoTypeOrFail("TERRAIN_SNOW"):
					print("Changing River Ice Plot to Tundra")
					terrainTundra = getInfoTypeOrFail("TERRAIN_TUNDRA")
					pPlot.setTerrainType(terrainTundra, true, true)
				elif pPlot.getTerrainType() == getInfoTypeOrFail("TERRAIN_TUNDRA"):
					print("Changing River Tundra Plot to Plains")
					terrainPlains = getInfoTypeOrFail("TERRAIN_PLAINS")
					pPlot.setTerrainType(terrainPlains, true, true)
				elif pPlot.getTerrainType() == getInfoTypeOrFail("TERRAIN_PLAINS"):
					print("Changing River Plains Plot to Grass")
					terrainGrass = getInfoTypeOrFail("TERRAIN_GRASS")
					pPlot.setTerrainType(terrainGrass, true, true)
			self.addForestsAtPlot(pPlot, iX, iY)

	def addForestsAtPlot(self, pPlot, iX, iY):
		gc = CyGlobalContext()
		iForestLevel1 = self.forests.getHeightFromPercent(90)
		iForestLevel2 = self.forests.getHeightFromPercent(15)
		featureForest = getInfoTypeOrFail("FEATURE_FOREST")
		if pPlot.getTerrainType() != getInfoTypeOrFail("TERRAIN_SNOW"):
			if self.forests.getHeight(iX, iY) <= iForestLevel1 and self.forests.getHeight(iX, iY) >= iForestLevel2:
				pPlot.setFeatureType(featureForest, 2)

def addFeatures():
	NiTextOut("Adding Features (Python Boreal) ...")
	featuregen = BorealFeatureGenerator()
	featuregen.addFeatures()
	return 0

def assignStartingPlots():
	# This function borrowed from Highlands. Just as insurance against duds.
	# Lake fill-ins changed to tundra instead of plains.
	# - Sirian - June 2, 2007
	#
	#
	# In order to prevent "pockets" from forming, where civs can be blocked in 
	# by Peaks or lakes, causing a "dud" map, pathing must be checked for each 
	# new start plot before it hits the map. Any pockets that are detected must 
	# be opened. The following process takes care of this need. Soren created a 
	# useful function that already lets you know how far a given plot is from
	# the closest nearest civ already on the board. MinOriginalStartDist is that 
	# function. You can get-- or setMinoriginalStartDist() as a value attached 
	# to each plot. Any value of -1 means no valid land-hills-only path exists to
	# a civ already placed. For Highlands, that means we have found a pocket 
	# and it must be opened. A valid legal path from all civs to all other civs 
	# is required for this map to deliver reliable, fun games every time.
	#
	# - Sirian - 2005
	#
	gc = CyGlobalContext()
	map = CyMap()
	dice = gc.getGame().getMapRand()
	iW = map.getGridWidth()
	iH = map.getGridHeight()
	iPlayers = gc.getGame().countCivPlayersEverAlive()
	iNumStartsAllocated = 0
	start_plots = []
	print("===")
	print("Number of players: %s" % iPlayers)
	print("===")

	terrainTundra = getInfoTypeOrFail("TERRAIN_TUNDRA")

	# Obtain player numbers. (Account for possibility of Open slots!)
	player_list = []
	for plrCheckLoop in range(gc.getMAX_CIV_PLAYERS()):
		if CyGlobalContext().getPlayer(plrCheckLoop).isEverAlive():
			player_list.append(plrCheckLoop)
	# Shuffle players so that who goes first (and gets the best start location) is randomized.
	shuffledPlayers = []
	for playerLoopTwo in range(gc.getGame().countCivPlayersEverAlive()):
		iChoosePlayer = dice.get(len(player_list), "Shuffling Players - Highlands PYTHON")
		shuffledPlayers.append(player_list[iChoosePlayer])
		del player_list[iChoosePlayer]

	# Loop through players, assigning starts for each.
	for assign_loop in range(iPlayers):
		playerID = shuffledPlayers[assign_loop]
		player = gc.getPlayer(playerID)

		# Use the absolute approach for findStart from CvMapGeneratorUtil, which 
		# ignores areaID quality and finds the best local situation on the board.
		findstart = CvMapGeneratorUtil.findStartingPlot(playerID)
		sPlot = map.plotByIndex(findstart)

		# Record the plot number to the data array for use if needed to open a "pocket".
		iStartX = sPlot.getX()
		iStartY = sPlot.getY()

		# If first player placed, no need to check for pathing yet.
		if assign_loop == 0:
			start_plots.append([iStartX, iStartY])
			player.setStartingPlot(sPlot, true) # True flag causes data to be refreshed for MinOriginalStartDist data cells in plots on the same land mass.
			print("-+-+-")
			print("Player")
			print(playerID)
			print("First player assigned.")
			print("-+-+-")
			continue

		# Check the pathing in the start plot.
		if sPlot.getMinOriginalStartDist() != -1:
			start_plots.append([iStartX, iStartY])
			player.setStartingPlot(sPlot, true)
			print("-+-+-")
			print("Player")
			print(playerID)
			print("Open Path, no problems.")
			print("-+-+-")
			continue

		# If the process has reached this point, then this player is stuck 
		# in a "pocket". This could be an island, a valley surrounded by peaks, 
		# or an area blocked off by peaks. Could even be that a major line 
		# of peaks and lakes combined is bisecting the entire map.
		print("-----")
		print("Player")
		print(playerID)
		print("Pocket detected, attempting to resolve...")
		print("-----")
		#
		# First step is to identify which existing start plot is closest.
		print("Pocket Plot")
		print("%s %s" % (iStartX, iStartY))
		print("---")
		[iEndX, iEndY] = start_plots[0]
		fMinDistance = sqrt(((iStartX - iEndX) ** 2) + ((iStartY - iEndY) ** 2))
		for check_loop in range(1, len(start_plots)):
			[iX, iY] = start_plots[check_loop]
			if fMinDistance > sqrt(((iStartX - iX) ** 2) + ((iStartY - iY) ** 2)):
				# Closer start plot found!
				[iEndX, iEndY] = start_plots[check_loop]
				fMinDistance = sqrt(((iStartX - iX) ** 2) + ((iStartY - iY) ** 2))
		print("Nearest player (path destination)")
		print("%s %s" % (iEndX, iEndY))
		print("---")
		print("Absolute distance:")
		print(fMinDistance)
		print("-----")

		# Now we draw an invisible line, plot by plot, one plot wide, from 
		# the current start to the nearest start, converting peaks along the 
		# way in to hills, and lakes in to flatlands, until a path opens.

		# Bulldoze the path until it opens!
		startPlot = map.plot(iStartX, iStartY)
		endPlot = map.plot(iEndX, iEndY)
		if abs(iEndY-iStartY) < abs(iEndX-iStartX):
			# line is closer to horizontal
			if iStartX > iEndX:
				startX, startY, endX, endY = iEndX, iEndY, iStartX, iStartY # swap start and end
				bReverseFlag = True
				print("Path reversed, working from the end plot.")
			else: # don't swap
				startX, startY, endX, endY = iStartX, iStartY, iEndX, iEndY
				bReverseFlag = False
				print("Path not reversed.")
			dx = endX-startX
			dy = endY-startY
			if dx == 0 or dy == 0:
				slope = 0
			else:
				slope = float(dy)/float(dx)
			print("Slope: ", slope)
			y = startY
			for x in range(startX, endX):
				print("Checking plot")
				print("%s %s" % (x, int(round(y))))
				print("---")
				if map.isPlot(x, int(round(y))):
					i = map.plotNum(x, int(round(y)))
					pPlot = map.plotByIndex(i)
					y += slope
					print("y plus slope: ", y)
					if pPlot.isHills() or pPlot.isFlatlands():
						continue # on to next plot!
					if pPlot.isPeak():
						print("Peak found! Bulldozing this plot.")
						print("---")
						pPlot.setPlotType(PlotTypes.PLOT_HILLS, true, true)
						if bReverseFlag:
							currentDistance = map.calculatePathDistance(pPlot, startPlot)
						else:
							currentDistance = map.calculatePathDistance(pPlot, endPlot)
						if currentDistance != -1: # The path has been opened!
							print("Pocket successfully opened!")
							print("-----")
							break
					elif pPlot.isWater():
						print("Lake found! Filling in this plot.")
						print("---")
						pPlot.setPlotType(PlotTypes.PLOT_LAND, true, true)
						pPlot.setTerrainType(terrainTundra, true, true)
						if pPlot.getBonusType(-1) != -1:
							print("########################")
							print("A sea-based Bonus is now present on the land! EEK!")
							print("########################")
							pPlot.setBonusType(-1)
							print("OK, nevermind. The resource has been removed.")
							print("########################")
						if bReverseFlag:
							currentDistance = map.calculatePathDistance(pPlot, startPlot)
						else:
							currentDistance = map.calculatePathDistance(pPlot, endPlot)
						if currentDistance != -1: # The path has been opened!
							print("Pocket successfully opened!")
							print("-----")
							break

		else:
			# line is closer to vertical
			if iStartY > iEndY:
				startX, startY, endX, endY = iEndX, iEndY, iStartX, iStartY # swap start and end
				bReverseFlag = True
				print("Path reversed, working from the end plot.")
			else: # don't swap
				startX, startY, endX, endY = iStartX, iStartY, iEndX, iEndY
				bReverseFlag = False
				print("Path not reversed.")
			dx, dy = endX-startX, endY-startY
			if dx == 0 or dy == 0:
				slope = 0
			else:
				slope = float(dx)/float(dy)
			print("Slope: ", slope)
			x = startX
			for y in range(startY, endY+1):
				print("Checking plot")
				print("%s %s" % (int(round(x)), y))
				print("---")
				if map.isPlot(int(round(x)), y):
					i = map.plotNum(int(round(x)), y)
					pPlot = map.plotByIndex(i)
					x += slope
					print("x plus slope: ", x)
					if pPlot.isHills() or pPlot.isFlatlands():
						continue # on to next plot!
					if pPlot.isPeak():
						print("Peak found! Bulldozing this plot.")
						print("---")
						pPlot.setPlotType(PlotTypes.PLOT_HILLS, true, true)
						if bReverseFlag:
							currentDistance = map.calculatePathDistance(pPlot, startPlot)
						else:
							currentDistance = map.calculatePathDistance(pPlot, endPlot)
						if currentDistance != -1: # The path has been opened!
							print("Pocket successfully opened!")
							print("-----")
							break
					elif pPlot.isWater():
						print("Lake found! Filling in this plot.")
						print("---")
						pPlot.setPlotType(PlotTypes.PLOT_LAND, true, true)
						pPlot.setTerrainType(terrainTundra, true, true)
						if pPlot.getBonusType(-1) != -1:
							print("########################")
							print("A sea-based Bonus is now present on the land! EEK!")
							print("########################")
							pPlot.setBonusType(-1)
							print("OK, nevermind. The resource has been removed.")
							print("########################")
						if bReverseFlag:
							currentDistance = map.calculatePathDistance(pPlot, startPlot)
						else:
							currentDistance = map.calculatePathDistance(pPlot, endPlot)
						if currentDistance != -1: # The path has been opened!
							print("Pocket successfully opened!")
							print("-----")
							break

		# Now that all the pathing for this player is resolved, set the start plot.
		start_plots.append([iStartX, iStartY])
		player.setStartingPlot(sPlot, true)

	# All done!
	print("**********")
	print("All start plots assigned!")
	print("**********")
	return None

def normalizeRemovePeaks():
	return None

def normalizeRemoveBadTerrain():
	return None

def normalizeAddGoodTerrain():
	return None

def normalizeAddExtras():
	return None

# Sirian's "Sahara Regional Bonus Placement" system.

# Init all bonuses. This is your master key.
resourcesToEliminate = ("BONUS_SILK", "BONUS_BANANA", "BONUS_MAIZE", "BONUS_RICE", "BONUS_PIG", "BONUS_INCENSE", "BONUS_MOLLUSCS")

boreal = ("BONUS_GEMSTONES", "BONUS_DEER", "BONUS_SHEEP", "BONUS_WHEAT")
gems = ("BONUS_GEMSTONES")
deer = ("BONUS_DEER")
sheep = ("BONUS_SHEEP")
wheat = ("BONUS_WHEAT")

def addBonusType(argsList):
	print("*******")
	[iBonusType] = argsList
	gc = CyGlobalContext()
	map = CyMap()
	dice = gc.getGame().getMapRand()
	type_string = gc.getBonusInfo(iBonusType).getType()

	if (type_string in resourcesToEliminate):
		print("-NONE-", type_string, "-NONE-")
		return None # These bonus types will not appear, at all.
	elif type_string not in boreal:
		print("Default", type_string, "Default")
		CyPythonMgr().allowDefaultImpl() # Let C handle this bonus in the default way.
	else: # Current bonus type is custom-handled. Assignments to follow.
		iW = map.getGridWidth()
		iH = map.getGridHeight()

		# Generate resources
		if (type_string in boreal):
			print("---", type_string, "---")
			NiTextOut("Placing forest resources (Python Arboria) ...")
			iWheatBottom1 = food.getHeightFromPercent(40)
			iWheatTop1 = food.getHeightFromPercent(45)
			iWheatBottom2 = food.getHeightFromPercent(55)
			iWheatTop2 = food.getHeightFromPercent(60)
			iSheepBottom1 = food.getHeightFromPercent(10)
			iSheepTop1 = food.getHeightFromPercent(17)
			iSheepBottom2 = food.getHeightFromPercent(83)
			iSheepTop2 = food.getHeightFromPercent(90)
			iGemsBottom1 = food.getHeightFromPercent(30)
			iGemsTop1 = food.getHeightFromPercent(36)
			iGemsBottom2 = food.getHeightFromPercent(64)
			iGemsTop2 = food.getHeightFromPercent(70)
			iDeerBottom1 = food.getHeightFromPercent(23)
			iDeerTop1 = food.getHeightFromPercent(27)
			iDeerBottom2 = food.getHeightFromPercent(48)
			iDeerTop2 = food.getHeightFromPercent(52)
			iDeerBottom3 = food.getHeightFromPercent(73)
			iDeerTop3 = food.getHeightFromPercent(77)
			iDeerPercent = getDeerPercent()

			for y in range(iH):
				for x in range(iW):
					# Fractalized placement
					pPlot = map.plot(x,y)
					if pPlot.isWater() or pPlot.isPeak():
						continue
					if pPlot.getTerrainType() == getInfoTypeOrFail("TERRAIN_GRASS"):
						continue
					if pPlot.getTerrainType() == getInfoTypeOrFail("TERRAIN_SNOW"):
						continue
					if pPlot.getBonusType(-1) == -1:
						foodVal = food.getHeight(x,y)
						if (type_string in deer):
							if pPlot.getFeatureType() == getInfoTypeOrFail("FEATURE_FOREST") and pPlot.isFlatlands():
								if iDeerPercent > 0 and ((foodVal >= iDeerBottom1 and foodVal <= iDeerTop1) or (foodVal >= iDeerBottom2 and foodVal <= iDeerTop2) or (foodVal >= iDeerBottom3 and foodVal <= iDeerTop3)):
									if dice.get(100, "Boreal Deer Density PYTHON") >= iDeerPercent:
										continue
									map.plot(x,y).setBonusType(iBonusType)
						if (type_string in gems):
							if pPlot.isHills():
								if (foodVal >= iGemsBottom1 and foodVal <= iGemsTop1) or (foodVal >= iGemsBottom2 and foodVal <= iGemsTop2):
									map.plot(x,y).setBonusType(iBonusType)
						if (type_string in wheat):
							if pPlot.isFlatlands() and pPlot.getFeatureType() != getInfoTypeOrFail("FEATURE_FOREST"):
								if (foodVal >= iWheatBottom1 and foodVal <= iWheatTop1) or (foodVal >= iWheatBottom2 and foodVal <= iWheatTop2):
									map.plot(x,y).setBonusType(iBonusType)
						if (type_string in sheep):
							if pPlot.getFeatureType() != getInfoTypeOrFail("FEATURE_FOREST"):
								if (foodVal >= iSheepBottom1 and foodVal <= iSheepTop1) or (foodVal >= iSheepBottom2 and foodVal <= iSheepTop2):
									map.plot(x,y).setBonusType(iBonusType)

		return None

