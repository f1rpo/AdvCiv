#
#	FILE:	 Arboria.py
#	AUTHOR:  Bob Thomas (Sirian)
#	PURPOSE: Global map script - Forest paradise - Primarily for MP
#-----------------------------------------------------------------------------
#	Copyright (c) 2005, 2007 Firaxis Games, Inc. All rights reserved.
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
import random
import CvMapGeneratorUtil
import sys
from CvMapGeneratorUtil import HintedWorld
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator
from SAS_WorldSizeUtils import *

def getDescription():
	return "TXT_KEY_MAP_SCRIPT_ARBORIA_DESCR"

def isAdvancedMap():
	"This map should not show up in simple mode"
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
	return option_values[iOption]

def getCustomMapOptionDescAt(argsList):
	[iOption, iSelection] = argsList
	selection_names = {
		0: {
			0: "0%",
			1: "10%",
			2: "25%",
			3: "50% (recommended)",
			4: "75%",
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
	translated_text = unicode(CyTranslator().getText(selection_names[iOption][iSelection], ()))
	return translated_text

def getCustomMapOptionDefault(argsList):
	[iOption] = argsList
	option_defaults = {
		0: 3, # 50%
		1: 1
	}
	return option_defaults[iOption]

def isRandomCustomMapOption(argsList):
	[iOption] = argsList
	option_random = {
		0: false,
		1: false
	}
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
	return deer_percents[map.getCustomMapOption(0)]

def getTopLatitude():
	return 50

def getBottomLatitude():
	return -50

def minStartingDistanceModifier():
	return -10

def getGridSize(argsList):
	"Override Grid Size function to make the maps square."
	grid_sizes = {
		WorldSizeTypes.WORLDSIZE_ARENA: (4,4),
		WorldSizeTypes.WORLDSIZE_DUEL: (5,5),
		WorldSizeTypes.WORLDSIZE_TINY: (6,6),
		WorldSizeTypes.WORLDSIZE_SMALL: (8,8),
		WorldSizeTypes.WORLDSIZE_STANDARD: (10,10),
		WorldSizeTypes.WORLDSIZE_LARGE: (13,13),
		WorldSizeTypes.WORLDSIZE_HUGE: (16,16)
	}

	if (argsList[0] == -1): # (-1,) is passed to function on loads
		return []
	[eWorldSize] = argsList
	return sas_lookup_world_size_with_calibrated_sas(eWorldSize, grid_sizes)

def beforeGeneration():
	gc = CyGlobalContext()
	map = CyMap()
	dice = gc.getGame().getMapRand()
	iW = map.getGridWidth()
	iH = map.getGridHeight()
	global food
	food = CyFractal()
	food.fracInit(iW, iH, 7, dice, 0, -1, -1)

def generatePlotTypes():
	NiTextOut("Setting Plot Types (Python Arboria) ...")
	global hinted_world
	hinted_world = HintedWorld(16,8)

	mapRand = CyGlobalContext().getGame().getMapRand()

	numBlocks = hinted_world.w * hinted_world.h
	numBlocksLand = int(numBlocks*0.50)
	cont = hinted_world.addContinent(numBlocksLand,mapRand.get(5, "Generate Plot Types PYTHON")+4,mapRand.get(3, "Generate Plot Types PYTHON")+2)
	if not cont:
		print("Couldn't create continent! Reverting to C implementation.")
		CyPythonMgr().allowDefaultImpl()
	else:		
		for x in range(hinted_world.w):
			for y in (0, hinted_world.h - 1):
				hinted_world.setValue(x,y, 1) # force ocean at poles
		hinted_world.buildAllContinents()
		return hinted_world.generatePlotTypes(shift_plot_types=True)

# subclass TerrainGenerator to create a lush grassland utopia.
class ArboriaTerrainGenerator(CvMapGeneratorUtil.TerrainGenerator):
	def __init__(self, fracXExp=-1, fracYExp=-1, grain_amount=5):
		# Keep only fractal state persistent; handles/derived values stay local.
		self.terrain = CyFractal()
		self.initFractals(grain_amount, fracXExp, fracYExp)

	def initFractals(self, grain_amount, fracXExp, fracYExp):
		gc = CyGlobalContext()
		map = CyMap()
		iWidth = map.getGridWidth()
		iHeight = map.getGridHeight()
		mapRand = gc.getGame().getMapRand()
		iFlags = 0  # Disallow FRAC_POLAR flag, to prevent "zero row" problems.
		iGrain = grain_amount + gc.getWorldInfo(map.getWorldSize()).getTerrainGrainChange()
		self.terrain.fracInit(iWidth, iHeight, iGrain, mapRand, iFlags, fracXExp, fracYExp)
		self.iGrassBottom = self.terrain.getHeightFromPercent(12)

	def getLatitudeAtPlot(self, iX, iY):
		return None

	def generateTerrain(self):		
		map = CyMap()
		iWidth = map.getGridWidth()
		iHeight = map.getGridHeight()
		terrainGrass = getInfoTypeOrFail("TERRAIN_GRASS")
		terrainPlains = getInfoTypeOrFail("TERRAIN_PLAINS")
		terrainData = [0]*(iWidth*iHeight)
		for x in range(iWidth):
			for y in range(iHeight):
				iI = y*iWidth + x
				terrain = self.generateTerrainAtPlot(x, y, terrainGrass, terrainPlains)
				terrainData[iI] = terrain
		return terrainData

	def generateTerrainAtPlot(self,iX,iY,terrainGrass,terrainPlains):
		map = CyMap()

		if (map.plot(iX, iY).isWater()):
			return map.plot(iX, iY).getTerrainType()

		val = self.terrain.getHeight(iX, iY)
		if val >= self.iGrassBottom:
			terrainVal = terrainGrass
		else:
			terrainVal = terrainPlains

		if (terrainVal == TerrainTypes.NO_TERRAIN):
			return map.plot(iX, iY).getTerrainType()

		return terrainVal

def generateTerrainTypes():
	NiTextOut("Generating Terrain (Python Arboria) ...")
	terraingen = ArboriaTerrainGenerator()
	terrainTypes = terraingen.generateTerrain()
	return terrainTypes

class ArboriaFeatureGenerator(CvMapGeneratorUtil.FeatureGenerator):
	def __init__(self, forest_grain=6, fracXExp=-1, fracYExp=-1):
		gc = CyGlobalContext()
		self.map = CyMap()
		self.mapRand = gc.getGame().getMapRand()
		self.forests = CyFractal()

		self.iGridW = self.map.getGridWidth()
		self.iGridH = self.map.getGridHeight()

		iForestGrain = forest_grain + gc.getWorldInfo(self.map.getWorldSize()).getFeatureGrainChange()
		self.__initFractals(iForestGrain, fracXExp, fracYExp)

	def __initFractals(self, iForestGrain, fracXExp, fracYExp):
		iFlags = 0  # Disallow FRAC_POLAR flag, to prevent "zero row" problems.
		self.forests.fracInit(self.iGridW, self.iGridH, iForestGrain, self.mapRand, iFlags, fracXExp, fracYExp)

	def getLatitudeAtPlot(self, iX, iY):
		return 50

	def addFeaturesAtPlot(self, iX, iY):
		"adds any appropriate features at the plot (iX, iY) where (0,0) is in the SW"
		long = iX/float(self.iGridW)
		lat = iY/float(self.iGridH)
		pPlot = self.map.sPlot(iX, iY)

		if (pPlot.getFeatureType() == FeatureTypes.NO_FEATURE):
			self.addJunglesAtPlot(pPlot, iX, iY, lat)

		if (pPlot.getFeatureType() == FeatureTypes.NO_FEATURE):
			self.addForestsAtPlot(pPlot, iX, iY, lat, long)

	def addIceAtPlot(self, pPlot, iX, iY, lat):
		# We don' need no steeking ice. M'kay? Alrighty then.
		ice = 0

	def addJunglesAtPlot(self, pPlot, iX, iY, lat):
		# Warning: this version of JunglesAtPlot is using the forest fractal!
		gc = CyGlobalContext()
		featureJungle = getInfoTypeOrFail("FEATURE_JUNGLE")
		iJungleStart = self.forests.getHeightFromPercent(65)
		iJungleStop = self.forests.getHeightFromPercent(69)
		if pPlot.canHaveFeature(featureJungle):
			if (self.forests.getHeight(iX, iY) >= iJungleStart) and (self.forests.getHeight(iX, iY) <= iJungleStop):
				pPlot.setFeatureType(featureJungle, -1)

	def addForestsAtPlot(self, pPlot, iX, iY, lat, long):
		# Deciduous trees everywhere.
		gc = CyGlobalContext()
		featureForest = getInfoTypeOrFail("FEATURE_FOREST")
		iForestStart = self.forests.getHeightFromPercent(29)
		if pPlot.canHaveFeature(featureForest):
			if self.forests.getHeight(iX, iY) >= iForestStart:
				pPlot.setFeatureType(featureForest, 0)

def addFeatures():
	global featuregen
	NiTextOut("Adding Features (Python Arboria) ...")
	featuregen = ArboriaFeatureGenerator()
	featuregen.addFeatures()
	NiTextOut("Simulate Forest Paradise (Forests - Python Arboria) ...")
	return 0

def normalizeRemovePeaks():
	return None

def normalizeRemoveBadFeatures():
	return None

def normalizeRemoveBadTerrain():
	return None

def normalizeAddGoodTerrain():
	return None

def normalizeAddExtras():
	return None

# Sirian's "Sahara Regional Bonus Placement" system.

# Init all bonuses. This is your master key.
forest = ("BONUS_SILVER", "BONUS_DEER")
silver = ("BONUS_SILVER")
deer = ("BONUS_DEER")

def addBonusType(argsList):
	print("*******")
	[iBonusType] = argsList
	gc = CyGlobalContext()
	map = CyMap()
	dice = gc.getGame().getMapRand()
	type_string = gc.getBonusInfo(iBonusType).getType()

	if type_string not in forest:
		print("Default", type_string, "Default")
		CyPythonMgr().allowDefaultImpl() # Let C handle this bonus in the default way.
	else: # Current bonus type is custom-handled. Assignments to follow.
		iW = map.getGridWidth()
		iH = map.getGridHeight()

		# Generate resources
		if (type_string in forest):
			print("---", type_string, "---")
			NiTextOut("Placing forest resources (Python Arboria) ...")
			featureForest = getInfoTypeOrFail("FEATURE_FOREST")
			iDeerPercent = getDeerPercent()
			iSilverBottom = food.getHeightFromPercent(10)
			iSilverTop = food.getHeightFromPercent(15)
			iDeerBottom1 = food.getHeightFromPercent(24)
			iDeerTop1 = food.getHeightFromPercent(27)
			iDeerBottom2 = food.getHeightFromPercent(49)
			iDeerTop2 = food.getHeightFromPercent(52)
			iDeerBottom3 = food.getHeightFromPercent(73)
			iDeerTop3 = food.getHeightFromPercent(76)

			for y in range(iH):
				for x in range(iW):
					# Fractalized placement
					pPlot = map.plot(x,y)
					if pPlot.isWater() or pPlot.isPeak():
						continue
					if pPlot.getBonusType(-1) == -1:
						foodVal = food.getHeight(x,y)
						if (type_string in deer):
							if pPlot.getFeatureType() == featureForest and pPlot.isFlatlands():
								if iDeerPercent > 0 and ((foodVal >= iDeerBottom1 and foodVal <= iDeerTop1) or (foodVal >= iDeerBottom2 and foodVal <= iDeerTop2) or (foodVal >= iDeerBottom3 and foodVal <= iDeerTop3)):
									if dice.get(100, "Arboria Deer Density PYTHON") >= iDeerPercent:
										continue
									map.plot(x,y).setBonusType(iBonusType)
						if (type_string in silver):
							if pPlot.isHills():
								if (foodVal >= iSilverBottom and foodVal <= iSilverTop):
									map.plot(x,y).setBonusType(iBonusType)

		return None

