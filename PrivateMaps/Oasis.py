#
#	FILE:	 Oasis.py
#	AUTHOR:  Bob Thomas (Sirian)
#	PURPOSE: Regional map script - Desert region between two fertile bands.
#-----------------------------------------------------------------------------
#	Copyright (c) 2005 Firaxis Games, Inc. All rights reserved.
#-----------------------------------------------------------------------------
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from CvPythonExtensions import *
import CvUtil
import CvMapGeneratorUtil
import random
import sys
import math
from SAS_WorldSizeUtils import *
from CvMapGeneratorUtil import FractalWorld
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator
from CvMapGeneratorUtil import BonusBalancer
from SASUtils import getInfoTypeOrFail

balancer = BonusBalancer()

def getDescription():
	return "TXT_KEY_MAP_SCRIPT_OASIS_DESCR"

def isAdvancedMap():
	# <!-- custom: hide this map from Simple Game to keep that list curated; still available in Custom Game. (GPT-5.3-Codex) -->
	return 1

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
		0:	3,
		1:	2
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
	return (map.getCustomMapOption(0) == 1 or map.getCustomMapOption(0) == 2)

def getWrapY():
	map = CyMap()
	return (map.getCustomMapOption(0) == 2)

# <!-- custom: These balanced-resource callbacks were shadowed by later Oasis callbacks with the same names, so they never ran.
# Comment them out instead of merging behavior, fixing Ruff F811 while preserving current Oasis behavior exactly; see KI#164. (GPT-5.5) -->
# def normalizeAddExtras():
# 	if (CyMap().getCustomMapOption(1) == 1):
# 		balancer.normalizeAddExtras()
# 	CyPythonMgr().allowDefaultImpl()	# do the rest of the usual normalizeStartingPlots stuff, don't overrride
#
# def addBonusType(argsList):
# 	[iBonusType] = argsList
# 	gc = CyGlobalContext()
# 	type_string = gc.getBonusInfo(iBonusType).getType()
#
# 	if (CyMap().getCustomMapOption(1) == 1):
# 		if (type_string in balancer.resourcesToBalance) or (type_string in balancer.resourcesToEliminate):
# 			return None # don't place any of this bonus randomly
#
# 	CyPythonMgr().allowDefaultImpl() # pretend we didn't implement this method, and let C handle this bonus in the default way

def isClimateMap():
	return 0

def isSeaLevelMap():
	return 0

def getTopLatitude():
	return 40
def getBottomLatitude():
	return 0

def getGridSize(argsList):
	# <!-- custom: The shared compact almost-all-land profile made Arena only 8x8 plots; two players nearly filled the entire Oasis map by turn 11 in runtime testing.
	# Restore Oasis' inherited land-heavy base profile, add Arena below Duel and calibrate SAS tiers from Huge by expected player count. See KI#293.2. (GPT-5.6-Sol) -->
	if (argsList[0] == -1): # (-1,) is passed to function on loads
		return []
	[eWorldSize] = argsList
	grid_sizes = {
		WorldSizeTypes.WORLDSIZE_ARENA: (5, 3),
		WorldSizeTypes.WORLDSIZE_DUEL: (6, 4),
		WorldSizeTypes.WORLDSIZE_TINY: (8, 5),
		WorldSizeTypes.WORLDSIZE_SMALL: (10, 6),
		WorldSizeTypes.WORLDSIZE_STANDARD: (14, 9),
		WorldSizeTypes.WORLDSIZE_LARGE: (18, 11),
		WorldSizeTypes.WORLDSIZE_HUGE: (23, 14)
		}
	return sas_lookup_world_size_with_calibrated_sas(eWorldSize, grid_sizes)

def findStartingPlot(argsList):
	[playerID] = argsList

	def isValid(playerID, x, y):
		teamID = CyGlobalContext().getPlayer(playerID).getTeam()
		iH = CyMap().getGridHeight()

		if int(teamID/2) * 2 == teamID: # Even-numbered team.
			isOdd = False
		else:
			isOdd = True

		if isOdd and y >= iH * 0.7:
			return true

		if not isOdd and y <= iH * 0.3:
			return true

		return false

	return CvMapGeneratorUtil.findStartingPlot(playerID, isValid)

def normalizeStartingPlotLocations():
	return None

def normalizeAddRiver():
	return None

def normalizeRemoveBadTerrain():
	return None

def normalizeAddFoodBonuses():
	return None

def normalizeAddGoodTerrain():
	return None

def normalizeAddExtras():
	return None

def minStartingDistanceModifier():
	return -35

def argmin(list):
	best = None
	best_index = None
	for i in range(len(list)):
		val = list[i]
		if (best is None) or (val < best):
			best_index = i
			best = val
	return (best_index, best)

# Subclass MultilayeredFractal to obtain natural coastline.
class OasisMultilayeredFractal(CvMapGeneratorUtil.MultilayeredFractal):
	def __init__(self, fracXExp=CyFractal.FracVals.DEFAULT_FRAC_X_EXP, fracYExp=CyFractal.FracVals.DEFAULT_FRAC_Y_EXP):
		self.gc = CyGlobalContext()
		self.map = self.gc.getMap()
		self.iW = self.map.getGridWidth()
		self.iH = self.map.getGridHeight()
		self.dice = self.gc.getGame().getMapRand()
		self.iFlags = self.map.getMapFractalFlags() # Defaults for that map type.
		self.iTerrainFlags = self.map.getMapFractalFlags() # Defaults for that map type.
		self.iHorzFlags = CyFractal.FracVals.FRAC_WRAP_X + CyFractal.FracVals.FRAC_POLAR # Use to prevent flat edges to north or south.
		self.iVertFlags = CyFractal.FracVals.FRAC_WRAP_Y + CyFractal.FracVals.FRAC_POLAR # Use to prevent flat edges to east or west.
		self.iRoundFlags = CyFractal.FracVals.FRAC_POLAR # Use to prevent flat edges on all sides.
		self.plotTypes = [] # Regional array
		self.fracXExp = fracXExp
		self.fracYExp = fracYExp

		# NOTE: The following line customized for Oasis, defaulting to land instead of ocean!
		self.wholeworldPlotTypes = [PlotTypes.PLOT_LAND] * (self.iW*self.iH)

	def generatePlotsInRegion(self, iWaterPercent, iRegionWidth, iRegionHeight, iRegionWestX, iRegionSouthY, iRegionGrain, iRegionHillsGrain, iRegionPlotFlags, iRegionTerrainFlags, iRegionFracXExp = -1, iRegionFracYExp = -1, bShift = True, iStrip = 15, rift_grain = -1, has_center_rift = False, invert_heights = False):
		# This function highly customized for Oasis.
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
		iHillsBottom1 = regionHillsFrac.getHeightFromPercent(20)
		iHillsTop1 = regionHillsFrac.getHeightFromPercent(30)
		iHillsBottom2 = regionHillsFrac.getHeightFromPercent(70)
		iHillsTop2 = regionHillsFrac.getHeightFromPercent(80)
		iPeakThreshold = regionPeaksFrac.getHeightFromPercent(25)

		# Loop through the region's plots. Oasis is only concerned with one region.
		for x in range(iRegionWidth):
			for y in range(iRegionHeight):
				i = y*iRegionWidth + x
				val = regionContinentsFrac.getHeight(x,y)
				if val >= iWaterThreshold:
					self.plotTypes[i] = PlotTypes.PLOT_LAND

		if bShift:
			# Shift plots to obtain a more natural shape.
			self.shiftRegionPlots(iRegionWidth, iRegionHeight, iStrip)

		# Apply the region's plots to the global plot array.
		for x in range(iRegionWidth):
			wholeworldX = x + iWestX
			for y in range(iRegionHeight):
				i = y*iRegionWidth + x
				# The following line is customized for Oasis, to layer water plots!
				if self.plotTypes[i] != PlotTypes.PLOT_OCEAN:
					continue
				wholeworldY = y + iSouthY
				iWorld = wholeworldY*self.iW + wholeworldX
				self.wholeworldPlotTypes[iWorld] = self.plotTypes[i]

	def generatePlotsByRegion(self):
		# Sirian's MultilayeredFractal class, controlling function.
		# You -MUST- customize this function for each use of the class.
		#
		# The following grain matrix is specific to Oasis.py
		sizekey = self.map.getWorldSize()
		sizevalues = {
			WorldSizeTypes.WORLDSIZE_ARENA: 2,
			WorldSizeTypes.WORLDSIZE_DUEL: 3,
			WorldSizeTypes.WORLDSIZE_TINY: 3,
			WorldSizeTypes.WORLDSIZE_SMALL: 3,
			WorldSizeTypes.WORLDSIZE_STANDARD: 4,
			WorldSizeTypes.WORLDSIZE_LARGE: 4,
			WorldSizeTypes.WORLDSIZE_HUGE: 5
			}
		grain = sas_lookup_world_size(sizekey, sizevalues)

		# The following regions are specific to Oasis.py
		forcedOceanY = self.iH - grain + 2
		grassSouthLat = 0.66
		grassMidLat = 0.82
		forcedLandY = int(self.iH * grassMidLat)

		# Force lines of ocean at the top
		for x in range(self.iW):
			for y in range(forcedOceanY, self.iH):
				i = y*self.iW + x
				self.wholeworldPlotTypes[i] = PlotTypes.PLOT_OCEAN

		# Fractalize the northern coast.
		NiTextOut("Simulating the Northern Coastline (Python Oasis) ...")
		# Set dimensions of the Old World region (specific to Terra.py)
		grassWestX = 0
		grassEastX = self.iW - 1
		grassNorthY = forcedOceanY - 1
		grassSouthY = int(self.iH * grassSouthLat)
		grassWidth = grassEastX - grassWestX + 1
		grassHeight = grassNorthY - grassSouthY + 1
		grassFlags = CyFractal.FracVals.FRAC_WRAP_X + CyFractal.FracVals.FRAC_POLAR

		self.generatePlotsInRegion(50, grassWidth, grassHeight, grassWestX, grassSouthY, 1, 3, grassFlags, self.iTerrainFlags, 8, 6, True, 15, -1, False, False)

		# Force land south of grassMidLat
		for x in range(self.iW):
			for y in range(forcedLandY):
				i = y*self.iW + x
				if self.wholeworldPlotTypes[i] == PlotTypes.PLOT_OCEAN:
					self.wholeworldPlotTypes[i] = PlotTypes.PLOT_LAND

		# Now add Hills and Peaks to the land mass, customized for Oasis.
		hillsFrac = CyFractal()
		peaksFrac = CyFractal()
		hillsFrac.fracInit(self.iW, self.iH, grain, self.dice, self.iTerrainFlags, self.fracXExp, self.fracYExp)
		peaksFrac.fracInit(self.iW, self.iH, grain+1, self.dice, self.iTerrainFlags, self.fracXExp, self.fracYExp)

		iHillsBottom1 = hillsFrac.getHeightFromPercent(20)
		iHillsTop1 = hillsFrac.getHeightFromPercent(30)
		iHillsBottom2 = hillsFrac.getHeightFromPercent(70)
		iHillsTop2 = hillsFrac.getHeightFromPercent(80)
		iPeakThreshold = peaksFrac.getHeightFromPercent(25)

		for x in range(self.iW):
			for y in range(self.iH):
				i = y*self.iW + x
				if self.wholeworldPlotTypes[i] == PlotTypes.PLOT_OCEAN:
					continue
				else:
					hillVal = hillsFrac.getHeight(x,y)
					if ((hillVal >= iHillsBottom1 and hillVal <= iHillsTop1) or (hillVal >= iHillsBottom2 and hillVal <= iHillsTop2)):
						peakVal = peaksFrac.getHeight(x,y)
						if (peakVal <= iPeakThreshold):
							self.wholeworldPlotTypes[i] = PlotTypes.PLOT_PEAK
						else:
							self.wholeworldPlotTypes[i] = PlotTypes.PLOT_HILLS
					else:
						pass

		return self.wholeworldPlotTypes

def generatePlotTypes():
	NiTextOut("Setting Plot Types (Python Oasis) ...")
	# Call generatePlotsByRegion() function from MultilayeredFractal subclass.
	#
	# To adapt to other scripts, use the original version instead of the one
	# customized for this script.
	global plotgen
	plotgen = OasisMultilayeredFractal()
	return plotgen.generatePlotsByRegion()

# subclass TerrainGenerator to redefine everything. This is a regional map. Ice need not apply!
# Latitudes, ratios, the works... It's all rewired. - Sirian June 20, 2005

class OasisTerrainGenerator(CvMapGeneratorUtil.TerrainGenerator):
	def __init__(self, iGrassPercent=50, iPlainsPercent=35, iNorthernPlainsPercent=40, iOasisGrassPercent=9, iOasisPlainsPercent=16, iOasisTopLatitude=0.69, iJungleLatitude=0.14, iOasisBottomLatitude=0.3, fracXExp=-1, fracYExp=-1, grain_amount=4):

		self.grain_amount = grain_amount

		self.gc = CyGlobalContext()
		self.map = CyMap()

		self.iWidth = self.map.getGridWidth()
		self.iHeight = self.map.getGridHeight()

		self.mapRand = self.gc.getGame().getMapRand()
		self.iFlags = 0

		self.grass=CyFractal()
		self.Oasisgrass=CyFractal()
		self.plains=CyFractal()
		self.Oasisplains=CyFractal()
		self.northernplains=CyFractal()
		self.variation=CyFractal()

		self.iGrassTopPercent = 100
		self.iGrassBottomPercent = max(0,int(100-iGrassPercent))
		self.iPlainsTopPercent = 100
		self.iPlainsBottomPercent = max(0,int(100-iGrassPercent-iPlainsPercent))
		self.iOasisGrassTopPercent = 100
		self.iOasisGrassBottomPercent = max(0,int(100-iOasisGrassPercent))
		self.iOasisPlainsTopPercent = 100
		self.iOasisPlainsBottomPercent = max(0,int(100-iOasisGrassPercent-iOasisPlainsPercent))
		self.iNorthernPlainsBottomPercent = max(0,int(100-iNorthernPlainsPercent))
		self.iMountainTopPercent = 75
		self.iMountainBottomPercent = 60

		self.iOasisBottomLatitude = iOasisBottomLatitude
		self.iOasisTopLatitude = iOasisTopLatitude
		self.iJungleLatitude = iJungleLatitude

		self.iGrassPercent = iGrassPercent
		self.iPlainsPercent = iPlainsPercent
		self.iOasisGrassPercent = iOasisGrassPercent
		self.iOasisPlainsPercent = iOasisPlainsPercent
		self.iNorthernPlainsPercent = iNorthernPlainsPercent

		self.fracXExp = fracXExp
		self.fracYExp = fracYExp

		self.initFractals()

	def initFractals(self):
		self.grass.fracInit(self.iWidth, self.iHeight, self.grain_amount, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
		self.iGrassTop = self.grass.getHeightFromPercent(self.iGrassTopPercent)
		self.iGrassBottom = self.grass.getHeightFromPercent(self.iGrassBottomPercent)

		self.plains.fracInit(self.iWidth, self.iHeight, self.grain_amount+1, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
		self.iPlainsTop = self.plains.getHeightFromPercent(self.iPlainsTopPercent)
		self.iPlainsBottom = self.plains.getHeightFromPercent(self.iPlainsBottomPercent)

		self.Oasisgrass.fracInit(self.iWidth, self.iHeight, self.grain_amount, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
		self.iOasisGrassTop = self.grass.getHeightFromPercent(self.iOasisGrassTopPercent)
		self.iOasisGrassBottom = self.grass.getHeightFromPercent(self.iOasisGrassBottomPercent)

		self.Oasisplains.fracInit(self.iWidth, self.iHeight, self.grain_amount+1, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
		self.iOasisPlainsTop = self.plains.getHeightFromPercent(self.iOasisPlainsTopPercent)
		self.iOasisPlainsBottom = self.plains.getHeightFromPercent(self.iOasisPlainsBottomPercent)

		self.northernplains.fracInit(self.iWidth, self.iHeight, self.grain_amount+1, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
		self.iNorthernPlainsBottom = self.plains.getHeightFromPercent(self.iNorthernPlainsBottomPercent)

		self.variation.fracInit(self.iWidth, self.iHeight, self.grain_amount, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)

		self.terrainDesert = getInfoTypeOrFail("TERRAIN_DESERT")
		self.terrainPlains = getInfoTypeOrFail("TERRAIN_PLAINS")
		self.terrainGrass = getInfoTypeOrFail("TERRAIN_GRASS")

	def getLatitudeAtPlot(self, iX, iY):
		lat = iY/float(self.iHeight) # 0.0 = south edge, 1.0 = north edge

		# Adjust latitude using self.variation fractal, to mix things up:
		lat += (128 - self.variation.getHeight(iX, iY))/(255.0 * 5.0)

		# Limit to the range [0, 1]:
		if lat < 0:
			lat = 0.0
		if lat > 1:
			lat = 1.0

		return lat

	def generateTerrain(self):		
		terrainData = [0]*(self.iWidth*self.iHeight)
		for x in range(self.iWidth):
			for y in range(self.iHeight):
				iI = y*self.iWidth + x
				terrain = self.generateTerrainAtPlot(x, y)
				terrainData[iI] = terrain
		return terrainData

	def generateTerrainAtPlot(self,iX,iY):
		lat = self.getLatitudeAtPlot(iX,iY)

		if (self.map.plot(iX, iY).isWater()):
			return self.map.plot(iX, iY).getTerrainType()

		terrainVal = self.terrainDesert

		if lat > self.iOasisTopLatitude:
			plainsVal = self.plains.getHeight(iX, iY)
			if plainsVal >= self.iNorthernPlainsBottom:
				terrainVal = self.terrainPlains
			else:
				terrainVal = self.terrainGrass
		elif lat < self.iJungleLatitude:
			terrainVal = self.terrainGrass
		elif lat < self.iOasisBottomLatitude and lat >= self.iJungleLatitude:
			grassVal = self.grass.getHeight(iX, iY)
			plainsVal = self.plains.getHeight(iX, iY)
			if ((grassVal >= self.iGrassBottom) and (grassVal <= self.iGrassTop)):
				terrainVal = self.terrainGrass
			elif ((plainsVal >= self.iPlainsBottom) and (plainsVal <= self.iPlainsTop)):
				terrainVal = self.terrainPlains
		else:
			OasisgrassVal = self.grass.getHeight(iX, iY)
			OasisplainsVal = self.plains.getHeight(iX, iY)
			if ((OasisgrassVal >= self.iOasisGrassBottom) and (OasisgrassVal <= self.iOasisGrassTop)):
				terrainVal = self.terrainGrass
			elif ((OasisplainsVal >= self.iOasisPlainsBottom) and (OasisplainsVal <= self.iOasisPlainsTop)):
				terrainVal = self.terrainPlains

		if (terrainVal == TerrainTypes.NO_TERRAIN):
			return self.map.plot(iX, iY).getTerrainType()

		return terrainVal

def generateTerrainTypes():
	NiTextOut("Generating Terrain (Python Oasis) ...")
	terraingen = OasisTerrainGenerator()
	terrainTypes = terraingen.generateTerrain()
	return terrainTypes

class OasisFeatureGenerator(CvMapGeneratorUtil.FeatureGenerator):
	def __init__(self, iJunglePercent=40, iForestPercent=45, jungle_grain=5, forest_grain=6, fracXExp=-1, fracYExp=-1):
		self.gc = CyGlobalContext()
		self.map = CyMap()
		self.mapRand = self.gc.getGame().getMapRand()
		self.jungles = CyFractal()
		self.forests = CyFractal()
		self.iFlags = 0
		self.iGridW = self.map.getGridWidth()
		self.iGridH = self.map.getGridHeight()

		self.iJunglePercent = iJunglePercent
		self.iForestPercent = iForestPercent

		self.jungle_grain = jungle_grain
		self.forest_grain = forest_grain

		self.fracXExp = fracXExp
		self.fracYExp = fracYExp

		self.__initFractals()
		self.__initFeatureTypes()

	def __initFractals(self):
		self.jungles.fracInit(self.iGridW+1, self.iGridH+1, self.jungle_grain, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
		self.forests.fracInit(self.iGridW+1, self.iGridH+1, self.forest_grain, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)

		self.iJungleLevel = self.jungles.getHeightFromPercent(self.iJunglePercent)
		self.iForestLevel = self.forests.getHeightFromPercent(self.iForestPercent)

	def __initFeatureTypes(self):
		self.featureJungle = getInfoTypeOrFail("FEATURE_JUNGLE")
		self.featureForest = getInfoTypeOrFail("FEATURE_FOREST")
		self.featureOasis = getInfoTypeOrFail("FEATURE_OASIS")

	def getLatitudeAtPlot(self, iX, iY):
		# 0.0 = bottom edge, 1.0 = top edge.
		return iY/float(self.iGridH)

	def addFeaturesAtPlot(self, iX, iY):
		"adds any appropriate features at the plot (iX, iY) where (0,0) is in the SW"
		lat = self.getLatitudeAtPlot(iX, iY)
		pPlot = self.map.sPlot(iX, iY)

		for iI in range(self.gc.getNumFeatureInfos()):
			if pPlot.canHaveFeature(iI):
				if self.mapRand.get(10000, "Add Feature PYTHON") < self.gc.getFeatureInfo(iI).getAppearanceProbability():
					pPlot.setFeatureType(iI, -1)

		if (pPlot.getFeatureType() == FeatureTypes.NO_FEATURE):
			# Jungles only in the deep south or in the Oasis!
			if lat < 0.16:
				self.addJunglesAtPlot(pPlot, iX, iY, lat)
			elif lat > 0.32 and lat < 0.65 and (pPlot.getTerrainType() == getInfoTypeOrFail("TERRAIN_GRASS")):
				pPlot.setFeatureType(self.featureJungle, -1)

		if (pPlot.getFeatureType() == FeatureTypes.NO_FEATURE):
			# No forests in the Oasis!
			if lat > 0.71 or lat < 0.3:
				self.addForestsAtPlot(pPlot, iX, iY, lat)

		if pPlot.isFlatlands() and (pPlot.getFeatureType() == FeatureTypes.NO_FEATURE) and (pPlot.getTerrainType() == getInfoTypeOrFail("TERRAIN_DESERT")):
			# Add more Oases!
			if (lat < 0.71 and lat > 0.3) and self.mapRand.get(9, "Add Extra Oases PYTHON") == 0:
				pPlot.setFeatureType(self.featureOasis, -1)

	def addIceAtPlot(self, pPlot, iX, iY, lat):
		# We don' need no steeking ice. M'kay? Alrighty then.
		ice = 0

	def addJunglesAtPlot(self, pPlot, iX, iY, lat):
		if pPlot.canHaveFeature(self.featureJungle):
			if self.jungles.getHeight(iX+1, iY+1) <= self.iJungleLevel:
				pPlot.setFeatureType(self.featureJungle, -1)

	def addForestsAtPlot(self, pPlot, iX, iY, lat):
		# No evergreens.
		if pPlot.canHaveFeature(self.featureForest):
			if self.forests.getHeight(iX+1, iY+1) <= self.iForestLevel:
				pPlot.setFeatureType(self.featureForest, 0)

def addFeatures():
	global featuregen
	NiTextOut("Adding Features (Python Oasis) ...")
	featuregen = OasisFeatureGenerator()
	featuregen.addFeatures()
	return 0

# Sahara Regional Bonus Placement system designed by Sirian.
'''
Sahara Desert was the first specially tailored "regional" map script. This
means the map is not intended to represent an entire planet, but only one area,
so the default handling of bonus resources, which was intended for maps that
represent whole planets, did not fit. A customizable means of setting specific
logic applicable to the Sahara was needed. Within the Sahara Desert there are
sub-regions ("regions") wherein bonus resources are allowed or forbidden.

addBonusType has been customized for Sahara Desert script (by Sirian) to
enable regional assignment of specific bonus types. You may use this template
to define regions for other map scripts as follows:
* Regions are defined using the literal strings that identify each bonus.
* Use the correctly spelled string drawn straight from the XML file in use.
* You may alter the number of regions according to your needs.
* Bonus "type strings" should be added for each object for each region.
* A bonus type can be made to appear in multiple regions.
* All bonuses handled per region are exempt from default functions that balance
the resources, so you will need to figure out how much of each resource of a
given type that you want to assign. This value can be determined in any way you
like, but be aware that this will not be done for you automatically.
* Regions may overlap, if you wish. Each region is merely a group of plots.
* Latitudes were used to specify regions in the original script, but other
means of assigning plots to a given region can be crafted.
* Regional assignment of bonuses can (theoretically) be used with fixed maps
to shuffle bonuses and increase replayability.
* You can force placement in other than default terrain types for any given bonus,
but only if that resource is assigned to one or more regions.
* Including a bonus type in resourcesToForce will disable default eligibility. If
you want to force terrain types in addition to defaults, do not add the bonus to
the list of bonuses to Force. This gives you flexibility of "and" or "or" w/ forcing.

- Sirian, June 22, 2005

Sahara was found to be a bit too realistic in playtesting, so we decided to adapt 
it in to a more stylized map, capable of supporting more life in the desert region. 
I renamed the script to Oasis and somehow it ended up being one of the nine primary 
map scripts. I learned a lot of lessons working on this script that I was later able 
to apply to other scripts. I'm still giving it tweaks and adjustments almost right 
up to the end of development.

- Sirian, Sept 14, 2005
'''

# Init all bonuses. This is your master key.
resourcesInOasis = ("BONUS_ALUMINUM", "BONUS_IRON", "BONUS_OIL", "BONUS_STONE", "BONUS_GOLD", "BONUS_INCENSE", "BONUS_ELEPHANTS")
resourcesInNorth = ("BONUS_HORSE", "BONUS_MARBLE", "BONUS_FUR", "BONUS_SILVER", "BONUS_SPICES", "BONUS_GRAPES", "BONUS_WHALE", "BONUS_MOLLUSCS", "BONUS_CRAB", "BONUS_FISH", "BONUS_SHEEP", "BONUS_WHEAT")
resourcesInSouth = ("BONUS_DYE", "BONUS_FUR", "BONUS_GEMSTONES", "BONUS_SILK", "BONUS_SUGAR", "BONUS_BANANA", "BONUS_DEER", "BONUS_PIG", "BONUS_RICE")
resourcesToEliminate = ()

resourcesToForce = ("BONUS_FUR", "BONUS_SILVER", "BONUS_DEER")
forcePlacementInForest = ("BONUS_FUR")
forcePlacementOnGrass = ("BONUS_DEER")
forcePlacementOnHills = ("BONUS_SILVER")
forceRarity = ("BONUS_SILK", "BONUS_WHALE")
forceAbundance = ("BONUS_FUR", "BONUS_IRON", "BONUS_ELEPHANTS", "BONUS_HORSE", "BONUS_OIL")
oasisCorn = ("BONUS_MAIZE")

def addBonusType(argsList):
	[iBonusType] = argsList
	gc = CyGlobalContext()
	map = CyMap()
	type_string = gc.getBonusInfo(iBonusType).getType()

	if (type_string in resourcesToEliminate):
		return None # These bonus types will not appear, at all.
	elif (type_string not in resourcesInOasis) and (type_string not in resourcesInNorth) and (type_string not in resourcesInSouth) and (type_string not in oasisCorn):
		CyPythonMgr().allowDefaultImpl() # Let C handle this bonus in the default way.
	else: # Current bonus type is meant to be regional. Assignments to follow.
		# init definition of regions. For Oasis, regions are defined by latitude.
		northlat = 0.72
		northoasis = 0.67
		southoasis = 0.33
		southlat = 0.28
		iW = map.getGridWidth()
		iH = map.getGridHeight()
		desert = OasisFeatureGenerator()
		dice = gc.getGame().getMapRand()

		# Generate desert maize (Corn!) all over the Oasis region!
		# Note: any fractal assignment of bonuses, like this one, must come before determining the count for regional bonuses.
		if (type_string not in oasisCorn):
			pass
		else:
			NiTextOut("Placing Desert Maize (Corn - Python Oasis) ...")
			crops = CyFractal()
			crops.fracInit(iW, iH, 7, dice, 0, -1, -1)
			iCropsBottom1 = crops.getHeightFromPercent(24)
			iCropsTop1 = crops.getHeightFromPercent(27)
			iCropsBottom2 = crops.getHeightFromPercent(73)
			iCropsTop2 = crops.getHeightFromPercent(75)
			cropNorth = int(iH * 0.66)
			cropSouth = int(iH * 0.32)
			for y in range(cropSouth, cropNorth):
				for x in range(iW):
					# Fractalized placement of crops
					pPlot = map.plot(x,y)
					if (not pPlot.isFlatlands()) or pPlot.getFeatureType() != -1:
						continue
					cropVal = crops.getHeight(x,y)
					if pPlot.getBonusType(-1) == -1 and ((cropVal >= iCropsBottom1 and cropVal <= iCropsTop1) or (cropVal >= iCropsBottom2 and cropVal <= iCropsTop2)):
						map.plot(x,y).setBonusType(iBonusType)
			return None
			# Corn all done now.
			# Can you say "Now That's Corny"? =)

		# init forced-eligibility flags
		if (type_string not in resourcesToForce):
			unforced = True
		else:
			unforced = False
		forceForest = False
		forceGrass = False
		forceHills = False
		if (type_string in forcePlacementInForest):
			forceForest = True
		if (type_string in forcePlacementOnGrass):
			forceGrass = True
		if (type_string in forcePlacementOnHills):
			forceHills = True

		# determine number of bonuses to place (defined as count)
		# size modifier is a fixed component based on world size
		sizekey = map.getWorldSize()
		sizevalues = {
			WorldSizeTypes.WORLDSIZE_ARENA: 1,
			WorldSizeTypes.WORLDSIZE_DUEL: 1,
			WorldSizeTypes.WORLDSIZE_TINY: 1,
			WorldSizeTypes.WORLDSIZE_SMALL: 1,
			WorldSizeTypes.WORLDSIZE_STANDARD: 2,
			WorldSizeTypes.WORLDSIZE_LARGE: 2,
			WorldSizeTypes.WORLDSIZE_HUGE: 3
			}
		sizemodifier = sas_lookup_world_size(sizekey, sizevalues)
		# playermodifier involves two layers of randomnity.
		players = gc.getGame().countCivPlayersEverAlive()
		plrcomponent1 = int(players / 3.0) # Bonus Method Fixed Component
		plrcomponent2 = dice.get(players, "Bonus Method Abundant Component - Oasis PYTHON") + 1
		plrcomponent3 = dice.get(int(players / 1.6), "Bonus Method Medium Component - Oasis PYTHON") - 1
		plrcomponent4 = dice.get((int(players / 4.0) + 1), "Bonus Method Sparse Component - Oasis PYTHON") - 2
		plrmethods = [plrcomponent1, plrcomponent2, plrcomponent3, plrcomponent4]
		playermodifier = plrmethods[dice.get(4, "Bonus Method - Oasis PYTHON")]

		count = sizemodifier + playermodifier
		if (type_string in forceRarity):
			count = dice.get(sizemodifier + 1, "Forced Bonus Rarity - Oasis PYTHON")
		if (type_string in forceAbundance):
			count = sizemodifier + plrcomponent2
		if count <= 0:
			return None # This bonus drew a short straw. None will be placed!

		# init bonus in regions. NOTE: a bonus can exist in more than one region!
		inNorth = False
		inOasis = False
		inSouth = False
		if (type_string in resourcesInOasis):
			inOasis = True
		if (type_string in resourcesInNorth):
			inNorth = True
		if (type_string in resourcesInSouth):
			inSouth = True

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
				# Check plot type and features for eligibility.
				if (pPlot.canHaveBonus(iBonusType, True) and unforced):
					pass
				elif forceHills and pPlot.isHills():
					pass
				elif forceForest and pPlot.getFeatureType() == getInfoTypeOrFail("FEATURE_FOREST"):
					pass
				elif forceGrass and pPlot.isFlatlands() and pPlot.getTerrainType() == getInfoTypeOrFail("TERRAIN_GRASS") and pPlot.getFeatureType() == -1:
					pass
				else:
					continue # to next plot.
				# re-init regional plot membership tests for each pass.
				plotInNorth = False
				plotInOasis = False
				plotInSouth = False
				# Check regional membership of current plot.
				# Oasis is using latitude to check, but other
				# tests could be designed and used, including lists.
				lat = desert.getLatitudeAtPlot(x, y)
				if lat >= northlat:
					plotInNorth = True
				if lat < northoasis and lat > southoasis:
					plotInOasis = True
				if lat <= southlat:
					plotInSouth = True
				# Check regional bonus eligibility vs plot membership.
				if (inNorth and plotInNorth):
					pass
				elif (inOasis and plotInOasis):
					pass
				elif (inSouth and plotInSouth):
					pass
				else:
					continue # on to next plot. This plot not eligible.
				#
				# Finally we have run all the checks.
				# 1. The plot has no bonus.
				# 2. The plot has an eligible terrain and feature type.
				# 3. The plot is in one or more eligible regions.
				# Now we append this plot to the eligible list.
				eligible.append([x,y])

		# Now we assign the bonuses to eligible plots chosen completely at random.
		while count > 0:
			if eligible == []:
				break # No eligible plots left!
			index = dice.get(len(eligible), "Bonus Placement - Oasis PYTHON")
			[x,y] = eligible[index]
			map.plot(x,y).setBonusType(iBonusType)
			del eligible[index] # Remove this plot from the eligible list.
			count = count - 1  # Reduce number of bonuses left to place.
		# This bonus type is done.
		return None # The default handler is not to place any more of this type.

# "Nile Style" Custom River Placement System designed by Sirian.
'''
The Nile is a mostly straight river flowing north through eastern Africa.

To simulate the Nile for Sahara, I wanted to create a "lane" within which 
a river could be randomly generated, so that the Nile would differ from 
game to game but still be recognizable as a feasible simulation of the Nile.

Nile Style rivers may be useful for any number of purposes, so I have 
designed the functions to be adaptable and readily controllable. I could 
have done more with them, but I only needed to go as far as I went. You 
may find more advanced versions tucked in to other scripts if I go on to 
adapt the concept to more advanced uses at a later date.

- Bob Thomas   July 18, 2005

Additional Note:

Although I haven't done it here, because there are no natural rivers on this map, if
you want to combine your own processes for charting rivers (such as this one) with 
the default processes, and use both, it is MANDATORY that you setRiverID(iRiverNumberHere) 
for each plot of river you place. The default river process needs the riverID values 
to behave properly when it intersects any rivers that you draw first.

Example:

pRiverPlot = map.plot(iX, iY)
pRiverPlot.setRiverID(iRiverNumberHere)
pRiverPlot.setWOfRiver(true, CardinalDirectionTypes.CARDINALDIRECTION_NORTH)

How you determine your river numbers is not vital, but you should use a different 
number for each new river. Each plot can have only one riverID attached to it.

- Bob Thomas	September 23, 2005
'''

def addNileStyleRiverFlowingNorth(center, maxshift, startX, startY, iDirectionOdds):
	# Nile-Style river flowing north, will stop at first water plot.
	#
	# WARNING: This routine does not protect against running off the 
	# left/right edge of the map!
	# Use center and maxshift to ensure that the river range is legal.
	#
	# WARNING: This routine does not detect or handle the presence of 
	# other rivers. You either need to keep its lane clear of other 
	# rivers or add new code (or find a version with the code added) 
	# to handle intersections with other rivers.
	#
	# Please keep these warnings in place if you borrow this function.
	#
	gc = CyGlobalContext()
	map = CyMap()
	dice = gc.getGame().getMapRand()
	iW = map.getGridWidth()
	iH = map.getGridHeight()
	left = center - maxshift
	right = center + maxshift
	# Init river and place first segment.
	iX = startX
	iY = startY
	pRiverPlot = map.plot(iX, iY)
	pRiverPlot.setWOfRiver(true, CardinalDirectionTypes.CARDINALDIRECTION_NORTH)
	# Loop through addition of more segments until the river terminates.
	# <!-- custom: The helper probes both y+1 plots before extending the river, so stop on the penultimate row instead of allowing an out-of-map final-row probe.
	# The direction-specific northward steps below enforce the same headroom before incrementing y. See KI#293. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	while iY < iH - 1:
		northPlotWest = map.plot(iX, iY+1)
		northPlotEast = map.plot(iX+1, iY+1)
		# Checking both plots for water.
		if northPlotEast.isWater() or northPlotWest.isWater():
			break
		pRiverPlot = map.plot(iX, iY)
		direction = dice.get(iDirectionOdds, "River Direction - Oasis PYTHON")
		segmentLength = 1 + dice.get(maxshift, "River Direction - Oasis PYTHON")
		# WEST
		if direction == 1: # Turn to the West, then North again.
			# <!-- custom: This turn increments y before continuing westward, so preserve the helper's y+1 headroom. See KI#293. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			if iY >= iH - 1:
				break
			iY += 1
			pRiverPlot = map.plot(iX, iY)
			# Turn west.
			for segment in range(segmentLength):
				if iX <= left:
					break
				else:
					pRiverPlot.setNOfRiver(true, CardinalDirectionTypes.CARDINALDIRECTION_WEST)
					plotWest = map.plot(iX-1, iY)
					# Only checking North-West Plot for Oasis.
					if plotWest.isWater():
						break
					iX -= 1
					pRiverPlot = map.plot(iX, iY)
			# Now turn back toward the north.
			pRiverPlot.setWOfRiver(true, CardinalDirectionTypes.CARDINALDIRECTION_NORTH)
		# EAST
		elif direction == 2: # Turn to the East, then North again.
			# <!-- custom: This turn increments y before continuing eastward, so preserve the helper's y+1 headroom. See KI#293. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			if iY >= iH - 1:
				break
			if iX >= right:
				continue
			iY += 1
			# Turn east.
			for segment in range(segmentLength):
				if iX >= right:
					break
				else: 
					iX += 1
					pRiverPlot = map.plot(iX, iY)
					pRiverPlot.setNOfRiver(true, CardinalDirectionTypes.CARDINALDIRECTION_EAST)
					plotEast = map.plot(iX+1, iY)
					# Only checking North-East Plot for Oasis.
					if plotEast.isWater():
						break
			# Now turn back toward the north.
			plotEast = map.plot(iX+1, iY)
			if plotEast.isWater():
				break
			pRiverPlot.setWOfRiver(true, CardinalDirectionTypes.CARDINALDIRECTION_NORTH)
		# NORTH
		else: # Run straight North for a segment. Note: 60% chance of North.
			for segment in range(segmentLength):
				# <!-- custom: Each iteration probes the next row before extending northward, so preserve the helper's y+1 headroom. See KI#293. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				if iY >= iH - 1:
					break
				northPlotWest = map.plot(iX, iY+1)
				northPlotEast = map.plot(iX+1, iY+1)
				if northPlotEast.isWater() or northPlotWest.isWater():
					break
				iY += 1
				pRiverPlot = map.plot(iX, iY)
				pRiverPlot.setWOfRiver(true, CardinalDirectionTypes.CARDINALDIRECTION_NORTH)
	# River is finished.
	return (iX, iY)

def addRivers():
	# Four Nile-style Rivers.
	gc = CyGlobalContext()
	map = CyMap()
	dice = gc.getGame().getMapRand()
	iW = map.getGridWidth()
	iH = map.getGridHeight()

	# Set maxshift, numbers of rivers for Oasis according to grid size.
	sizekey = map.getWorldSize()
	shiftvalues = {
		WorldSizeTypes.WORLDSIZE_ARENA: 1,
		WorldSizeTypes.WORLDSIZE_DUEL: 1,
		WorldSizeTypes.WORLDSIZE_TINY: 2,
		WorldSizeTypes.WORLDSIZE_SMALL: 3,
		WorldSizeTypes.WORLDSIZE_STANDARD: 5,
		WorldSizeTypes.WORLDSIZE_LARGE: 7,
		WorldSizeTypes.WORLDSIZE_HUGE: 9
		}
	maxshift = sas_lookup_world_size(sizekey, shiftvalues)

	# Place the Rivers
	NiTextOut("Charting Rivers (Python Oasis) ...")

	# Rivers begin south of the Oasis.
	startRangeBottom = 2 # May start nearly at the bottom edge of the map.
	# <!-- custom: On compact Arena/Duel heights, inherited iH / 6 could be at or below startRangeBottom and produce an invalid nonpositive CvRandom range.
	# Keep at least one legal vertical start offset. See KI#293. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	startRangeTop = max(startRangeBottom + 1, iH / 6) # May start as far north as Latitude 0.17
	firstQuadCenter = iW / 8
	secondQuadCenter = iW / 8 + iW / 4
	thirdQuadCenter = iW / 8 + 2 * (iW / 4)
	fourthQuadCenter = iW / 8 + 3 * (iW / 4)
	# Quadrant Names
	quadrants = [firstQuadCenter, secondQuadCenter, thirdQuadCenter, fourthQuadCenter]

	# Place rivers.
	for center in quadrants:
		# <!-- custom: Compact SAS grids let the last fixed lane reach the east edge although the river helper probes x+1, and Arena/Duel could produce a negative vertical RNG range.
		# Clamp each lane and both RNG ranges to the actual plot dimensions; the helper's north loop is likewise stopped before its y+1 probes leave the map. See KI#293. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		center = min(max(0, center), iW - 2)
		riverMaxShift = min(maxshift, center, iW - 2 - center)
		left = center - riverMaxShift
		right = center + riverMaxShift
		horzRand = max(1, right - left)
		vertRand = startRangeTop - startRangeBottom
		startX = left + dice.get(horzRand, "River Start X - Oasis PYTHON")
		startY = startRangeBottom + dice.get(vertRand, "River Start Y - Oasis PYTHON")
		#
		addNileStyleRiverFlowingNorth(center, riverMaxShift, startX, startY, 5)

	return 0
