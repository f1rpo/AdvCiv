#
#	FILE:	 Rainforest.py
#	AUTHOR:  Bob Thomas (Sirian)
#	PURPOSE: Regional map script - Tropical rainforest
#-----------------------------------------------------------------------------
#	Copyright (c) 2007 Firaxis Games, Inc. All rights reserved.
#-----------------------------------------------------------------------------
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from CvPythonExtensions import *
import CvUtil
import CvMapGeneratorUtil
import random
import sys
from math import sqrt
from CvMapGeneratorUtil import FractalWorld
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator
from SAS_WorldSizeUtils import *
from SASUtils import getInfoTypeOrFail

def getDescription():
	return "TXT_KEY_MAP_SCRIPT_RAINFOREST_DESCR"

def isAdvancedMap():
	"This map should not show up in simple mode"
	return 1

def isClimateMap():
	return 0

def isSeaLevelMap():
	return 0

def getNumCustomMapOptions():
	return 1

def getNumHiddenCustomMapOptions():
	return 1

def getCustomMapOptionName(argsList):
	translated_text = unicode(CyTranslator().getText("TXT_KEY_MAP_WORLD_WRAP", ()))
	return translated_text

def getNumCustomMapOptionValues(argsList):
	return 3

def getCustomMapOptionDescAt(argsList):
	iSelection = argsList[1]
	selection_names = ["TXT_KEY_MAP_WRAP_FLAT", "TXT_KEY_MAP_WRAP_CYLINDER", "TXT_KEY_MAP_WRAP_TOROID"]
	translated_text = unicode(CyTranslator().getText(selection_names[iSelection], ()))
	return translated_text

def getCustomMapOptionDefault(argsList):
	return 0

def isRandomCustomMapOption(argsList):
	return false

def getWrapX():
	map = CyMap()
	return (map.getCustomMapOption(0) == 1 or map.getCustomMapOption(0) == 2)

def getWrapY():
	map = CyMap()
	return (map.getCustomMapOption(0) == 2)

def getTopLatitude():
	return 15

def getBottomLatitude():
	return -15

def getGridSize(argsList):
	"Because this is almost all land, override getGridSize() to keep spacing compact"
	if (argsList[0] == -1): # (-1,) is passed to function on loads
		return []
	[eWorldSize] = argsList
	# <!-- custom: The shared 2x2 Arena grid left too little viable land for two civilizations.
	# Arena 5x3 produced viable starts. See KI#278.2. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	# <!-- custom: During the KI#278.2 repair, restoring BTS's whole original profile made Huge 21x13 almost as large as an empirically oversized SAS24 22x14 despite having only 16 expected players instead of 24.
	# Keep the validated Arena correction, but tune Duel-Huge between the former shared profile and BTS sizes and set integer SAS tiers from the tuned Huge tiles-per-player anchor without generic rounding drift. See KI#278.3. (GPT-5.6-Sol) -->
	grid_sizes = {
		WorldSizeTypes.WORLDSIZE_ARENA: (5,3),
		WorldSizeTypes.WORLDSIZE_DUEL: (5,4),
		WorldSizeTypes.WORLDSIZE_TINY: (7,4),
		WorldSizeTypes.WORLDSIZE_SMALL: (9,5),
		WorldSizeTypes.WORLDSIZE_STANDARD: (11,7),
		WorldSizeTypes.WORLDSIZE_LARGE: (14,9),
		WorldSizeTypes.WORLDSIZE_HUGE: (18,11),
		WorldSizeTypes.WORLDSIZE_SAS24: (22,14),
		WorldSizeTypes.WORLDSIZE_SAS32: (25,16),
		WorldSizeTypes.WORLDSIZE_SAS40: (29,17),
		WorldSizeTypes.WORLDSIZE_SAS48: (31,19),
	}
	return sas_lookup_world_size(eWorldSize, grid_sizes)

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
class RainforestFractalWorld(CvMapGeneratorUtil.FractalWorld):
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
	NiTextOut("Setting Plot Types (Python Rainforest) ...")
	global fractal_world
	fractal_world = RainforestFractalWorld()
	fractal_world.initFractal(continent_grain=3, rift_grain = -1, has_center_rift = False, polar = False)
	plot_types = fractal_world.generatePlotTypes(water_percent = 5)
	return plot_types

# subclass TerrainGenerator to redefine everything. This is a regional map.
class RainforestTerrainGenerator(CvMapGeneratorUtil.TerrainGenerator):
	def __init__(self, fracXExp=-1, fracYExp=-1):
		self.gc = CyGlobalContext()
		self.map = CyMap()

		self.iWidth = self.map.getGridWidth()
		self.iHeight = self.map.getGridHeight()

		self.mapRand = self.gc.getGame().getMapRand()

		self.iFlags = 0  # Disallow FRAC_POLAR flag, to prevent "zero row" problems.

		self.desert=CyFractal()
		self.plains=CyFractal()

		self.fracXExp = fracXExp
		self.fracYExp = fracYExp

		self.initFractals()

	def initFractals(self):
		self.desert.fracInit(self.iWidth, self.iHeight, 1, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
		self.plains.fracInit(self.iWidth, self.iHeight, 4, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)

		self.iDesert = self.desert.getHeightFromPercent(86)
		self.iDesertEdge = self.desert.getHeightFromPercent(82)
		self.iPlains = self.plains.getHeightFromPercent(95)

		self.terrainDesert = getInfoTypeOrFail("TERRAIN_DESERT")
		self.terrainPlains = getInfoTypeOrFail("TERRAIN_PLAINS")
		self.terrainGrass = getInfoTypeOrFail("TERRAIN_GRASS")

	def generateTerrainAtPlot(self,iX,iY):
		if (self.map.plot(iX, iY).isWater()):
			return self.map.plot(iX, iY).getTerrainType()
		else:
			desertVal = self.desert.getHeight(iX, iY)
			plainsVal = self.plains.getHeight(iX, iY)
			if desertVal >= self.iDesert:
				terrainVal = self.terrainDesert
			elif desertVal >= self.iDesertEdge:
				terrainVal = self.terrainPlains
			elif plainsVal >= self.iPlains:
				terrainVal = self.terrainPlains
			else:
				terrainVal = self.terrainGrass

		if (terrainVal == TerrainTypes.NO_TERRAIN):
			return self.map.plot(iX, iY).getTerrainType()

		return terrainVal

def generateTerrainTypes():
	NiTextOut("Generating Terrain (Python Rainforest) ...")
	terraingen = RainforestTerrainGenerator()
	terrainTypes = terraingen.generateTerrain()
	return terrainTypes

class RainforestFeatureGenerator(CvMapGeneratorUtil.FeatureGenerator):
	def __init__(self, iJunglePercent=85, iForestPercent=15, forest_grain=6, fracXExp=-1, fracYExp=-1):
		self.gc = CyGlobalContext()
		self.map = CyMap()
		self.mapRand = self.gc.getGame().getMapRand()
		self.forests = CyFractal()

		self.iFlags = 0 

		self.iGridW = self.map.getGridWidth()
		self.iGridH = self.map.getGridHeight()

		self.iJunglePercent = iJunglePercent
		self.iForestPercent = iForestPercent

		self.forest_grain = forest_grain + self.gc.getWorldInfo(self.map.getWorldSize()).getFeatureGrainChange()

		self.fracXExp = fracXExp
		self.fracYExp = fracYExp

		self.__initFractals()
		self.__initFeatureTypes()

	def __initFractals(self):
		self.forests.fracInit(self.iGridW, self.iGridH, self.forest_grain, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)

		self.iJungleLevel = self.forests.getHeightFromPercent(self.iJunglePercent)
		self.iForestLevel = self.forests.getHeightFromPercent(self.iForestPercent)

	def __initFeatureTypes(self):
		self.featureJungle = getInfoTypeOrFail("FEATURE_JUNGLE")
		self.featureForest = getInfoTypeOrFail("FEATURE_FOREST")
		self.featureOasis = getInfoTypeOrFail("FEATURE_OASIS")
		self.featureFlood = getInfoTypeOrFail("FEATURE_FLOOD_PLAINS")

	def addFeaturesAtPlot(self, iX, iY):
		pPlot = self.map.sPlot(iX, iY)

		if pPlot.isPeak() or pPlot.isWater():
			pass

		else:
			if pPlot.isRiverSide() and pPlot.isFlatlands():
				self.addFloodAtPlot(pPlot, iX, iY)

			if (pPlot.getFeatureType() == FeatureTypes.NO_FEATURE):
				self.addOasisAtPlot(pPlot, iX, iY)

			if (pPlot.getFeatureType() == FeatureTypes.NO_FEATURE):
				self.addForestsAtPlot(pPlot, iX, iY)

			if (pPlot.getFeatureType() == FeatureTypes.NO_FEATURE):
				self.addJunglesAtPlot(pPlot, iX, iY)

	def addFloodAtPlot(self, pPlot, iX, iY):
		if pPlot.getTerrainType() == getInfoTypeOrFail("TERRAIN_DESERT"):
			pPlot.setFeatureType(self.featureFlood, -1)

	def addOasisAtPlot(self, pPlot, iX, iY):
		if not pPlot.isFreshWater():
			if pPlot.getTerrainType() == getInfoTypeOrFail("TERRAIN_DESERT"):
				if self.mapRand.get(25, "Add Feature PYTHON") == 23:
					pPlot.setFeatureType(self.featureOasis, -1)

	def addJunglesAtPlot(self, pPlot, iX, iY):
		# Warning: this version of JunglesAtPlot is using the forest fractal!
		if pPlot.getTerrainType() == getInfoTypeOrFail("TERRAIN_GRASS"):
			if (self.forests.getHeight(iX, iY) <= self.iJungleLevel):
				pPlot.setFeatureType(self.featureJungle, -1)

	def addForestsAtPlot(self, pPlot, iX, iY):
		if pPlot.getTerrainType() != getInfoTypeOrFail("TERRAIN_DESERT"):
			if self.forests.getHeight(iX, iY) <= self.iForestLevel:
				pPlot.setFeatureType(self.featureForest, -1)

def addFeatures():
	NiTextOut("Adding Features (Python Rainforest) ...")
	featuregen = RainforestFeatureGenerator()
	featuregen.addFeatures()
	return 0

def assignStartingPlots():
	# This function borrowed from Highlands. Just as insurance against duds.
	# Lake fill-ins changed to grass instead of plains.
	# - Sirian - May 16, 2007
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

	terrainGrass = getInfoTypeOrFail("TERRAIN_GRASS")

	# Obtain player numbers. (Account for possibility of Open slots!)
	player_list = []
	for plrCheckLoop in range(gc.getMAX_CIV_PLAYERS()):
		if gc.getPlayer(plrCheckLoop).isEverAlive():
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
						pPlot.setTerrainType(terrainGrass, true, true)
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
						pPlot.setTerrainType(terrainGrass, true, true)
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
resourcesToEliminate = ("BONUS_WHALE", "BONUS_MOLLUSCS", "BONUS_FISH", "BONUS_CRAB")

jungleFood = ("BONUS_BANANA", "BONUS_PIG", "BONUS_RICE")
banana = ("BONUS_BANANA")
pig = ("BONUS_PIG")
rice = ("BONUS_RICE")

resourcesToForce = ("BONUS_FUR", "BONUS_SILVER", "BONUS_DEER")
forcePlacementOnFlats = ("BONUS_FUR", "BONUS_DEER")
forcePlacementOnHills = ("BONUS_SILVER")

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
	elif not ((type_string in resourcesToForce) or (type_string in jungleFood)):
		CyPythonMgr().allowDefaultImpl() # Let C handle this bonus in the default way.
	else: # Current bonus type is custom-handled. Assignments to follow.
		print("+++", type_string, "+++")
		iW = map.getGridWidth()
		iH = map.getGridHeight()

		# init forced-eligibility flags
		if (type_string not in resourcesToForce):
			unforced = True
		else:
			unforced = False
		forceFlats = False
		forceHills = False
		if (type_string in forcePlacementOnFlats):
			forceFlats = True
		if (type_string in forcePlacementOnHills):
			forceHills = True

		# Generate jungle food
		# Note: any fractal assignment of bonuses, like this one, must come before determining the count for forced bonuses.
		if (type_string in jungleFood):
			print("---", type_string, "---")
			global food
			NiTextOut("Placing Jungle Food (Jungle Food - Python Rainforest) ...")
			iRiceBottom = food.getHeightFromPercent(24)
			iRiceTop = food.getHeightFromPercent(27)
			iPigBottom = food.getHeightFromPercent(49)
			iPigTop = food.getHeightFromPercent(52)
			iBananaBottom = food.getHeightFromPercent(73)
			iBananaTop = food.getHeightFromPercent(76)

			for y in range(iH):
				for x in range(iW):
					# Fractalized placement
					pPlot = map.plot(x,y)
					if pPlot.isWater() or pPlot.isPeak() or pPlot.getFeatureType() != getInfoTypeOrFail("FEATURE_JUNGLE"):
						continue
					if pPlot.getBonusType(-1) == -1:
						foodVal = food.getHeight(x,y)
						if (type_string in banana):
							if (foodVal >= iBananaBottom and foodVal <= iBananaTop):
								map.plot(x,y).setBonusType(iBonusType)
						if (type_string in pig):
							if (foodVal >= iPigBottom and foodVal <= iPigTop):
								map.plot(x,y).setBonusType(iBonusType)
						if (type_string in rice):
							if (foodVal >= iRiceBottom and foodVal <= iRiceTop):
								map.plot(x,y).setBonusType(iBonusType)

			return None

		# determine number of bonuses to place (defined as count)
		# size modifier is a fixed component based on world size
		sizekey = map.getWorldSize()
		# <!-- custom: Rainforest retained only the legacy Duel-Huge fixed bonus-count table, so missing Arena fell upward to Huge while SAS24-48 stayed capped there.
		# Use the shared complete 1-7 progression already used by sibling Sirian bonus systems. See KI#278. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		sizevalues = sas_default_sizevalues()
		sizemodifier = sas_lookup_world_size(sizekey, sizevalues)
		# playermodifier involves two layers of randomnity.
		players = gc.getGame().countCivPlayersEverAlive()
		plrcomponent1 = int(players / 3.0) # Bonus Method Fixed Component
		plrcomponent2 = dice.get(players, "Bonus Method Abundant Component - Rainforest PYTHON") + 1
		plrcomponent3 = dice.get(int(players / 1.6), "Bonus Method Medium Component - Rainforest PYTHON") - 1
		plrmethods = [plrcomponent1, plrcomponent2, plrcomponent3]

		playermodifier = plrmethods[dice.get(3, "Forced Bonus NoRarity - Rainforest PYTHON")]
		count = sizemodifier + playermodifier
		if count <= 0:
			return None # This bonus drew a short straw. None will be placed!

		# Set plot eligibility for current bonus.
		# Begin by initiating the list, into which eligible plots will be recorded.
		eligible = []
		# Loop through all plots on the map, adding eligible plots to the list.
		for x in range(iW):
			for y in range(iH):
				# First check the plot for an existing bonus.
				pPlot = map.plot(x,y)
				if pPlot.getBonusType(-1) != -1:
					continue # to next plot.
				if pPlot.getFeatureType() == getInfoTypeOrFail("FEATURE_OASIS"):
					continue # Soren wants no bonuses in oasis plots. So mote it be.
				# Check plot type and features for eligibility.
				if forceHills and pPlot.isHills():
					pass
				elif (forceFlats and pPlot.isFlatlands()) and (pPlot.getFeatureType() == getInfoTypeOrFail("FEATURE_FOREST")):
					pass
				else:
					continue # to next plot.
				# Finally we have run all the checks.
				# 1. The plot has no bonus.
				# 2. The plot has an eligible terrain and feature type.
				# Now we append this plot to the eligible list.
				eligible.append([x,y])

		# Now we assign the bonuses to eligible plots chosen completely at random.
		while count > 0:
			if eligible == []:
				break # No eligible plots left!
			index = dice.get(len(eligible), "Bonus Placement - Rainforest PYTHON")
			[x,y] = eligible[index]
			map.plot(x,y).setBonusType(iBonusType)
			del eligible[index] # Remove this plot from the eligible list.
			count = count - 1  # Reduce number of bonuses left to place.
		# This bonus type is done.

		return None

