#
#	FILE:	 Medium_and_Small.py
#	AUTHOR:  Bob Thomas (Sirian)
#	PURPOSE: Global map script - Mixed islands and continents.
#-----------------------------------------------------------------------------
#	Copyright (c) 2007 Firaxis Games, Inc. All rights reserved.
#-----------------------------------------------------------------------------
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: added in PrivateMaps so maps in Simple Game or Custom Game are ordered alphabetically -->

from CvPythonExtensions import *
from SAS_WorldSizeUtils import *
import CvUtil
import CvMapGeneratorUtil
from CvMapGeneratorUtil import FractalWorld
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator

def getDescription():
	return "TXT_KEY_MAP_SCRIPT_MEDIUM_AND_SMALL_DESCR"

def isAdvancedMap():
	"This map should show up in simple mode"
	return 0

def getNumPlotsPercent(argsList):
	# <!-- custom: Huge tested too crowded because bottom islets usually are not assigned to players, leaving the top continent-style area cramped; increase plot count modestly from Large onward. (GPT-5.5?) -->
	[eWorldSize] = argsList
	if eWorldSize < 0:
		return 100
	sizeModifiers = {
		WorldSizeTypes.WORLDSIZE_ARENA: 100,
		WorldSizeTypes.WORLDSIZE_DUEL: 100,
		WorldSizeTypes.WORLDSIZE_TINY: 100,
		WorldSizeTypes.WORLDSIZE_SMALL: 100,
		WorldSizeTypes.WORLDSIZE_STANDARD: 100,
		WorldSizeTypes.WORLDSIZE_LARGE: 105,
		WorldSizeTypes.WORLDSIZE_HUGE: 110
	}
	return sas_lookup_world_size(eWorldSize, sizeModifiers)

def getNumCustomMapOptions():
	return 3

def getCustomMapOptionName(argsList):
	[iOption] = argsList
	option_names = {
		0:	"TXT_KEY_MAP_SCRIPT_CONTINENTS_SIZE",
		1:	"TXT_KEY_MAP_SCRIPT_ISLANDS_SIZE",
		2:	"TXT_KEY_MAP_SCRIPT_ISLAND_OVERLAP"
		}
	if not option_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	translated_text = unicode(CyTranslator().getText(option_names[iOption], ()))
	return translated_text

def getNumCustomMapOptionValues(argsList):
	[iOption] = argsList
	option_values = {
		0:	5,
		1:	2,
		2:	2
		}
	return option_values[iOption]

def getCustomMapOptionDescAt(argsList):
	[iOption, iSelection] = argsList
	selection_names = {
		0:	{
			0: "TXT_KEY_MAP_SCRIPT_NORMAL_CONTINENTS",
			1: "TXT_KEY_MAP_SCRIPT_UNPREDICTABLE",
			2: "TXT_KEY_MAP_SCRIPT_SNAKY_CONTINENTS",
			3: "TXT_KEY_MAP_SCRIPT_ISLANDS",
			4: "TXT_KEY_MAP_SCRIPT_RANDOM"
			},
		1:	{
			0: "TXT_KEY_MAP_SCRIPT_ISLANDS",
			1: "TXT_KEY_MAP_SCRIPT_TINY_ISLANDS"
			},
		2:	{
			0: "TXT_KEY_MAP_SCRIPT_ISLAND_REGION_SEPARATE",
			1: "TXT_KEY_MAP_SCRIPT_ISLANDS_MIXED_IN"
			}
		}
	if not selection_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	translated_text = unicode(CyTranslator().getText(selection_names[iOption][iSelection], ()))
	return translated_text

def getCustomMapOptionDefault(argsList):
	[iOption] = argsList
	option_defaults = {
		0:	4,
		1:	1,
		2:	0
		}
	return option_defaults[iOption]

def isRandomCustomMapOption(argsList):
	[iOption] = argsList
	option_random = {
		0:	false,
		1:	true,
		2:	true
		}
	return option_random[iOption]

def beforeGeneration():
	global yShiftRoll1
	global yShiftRoll2
	gc = CyGlobalContext()
	dice = gc.getGame().getMapRand()

	# Binary shift roll (for horizontal shifting if Island Region Separate).
	yShiftRoll1 = dice.get(2, "West Region Shift, Vertical - Medium and Small PYTHON")
	print(yShiftRoll1)
	yShiftRoll2 = dice.get(2, "East Region Shift, Vertical - Medium and Small PYTHON")
	print(yShiftRoll2)

class MnSMultilayeredFractal(CvMapGeneratorUtil.MultilayeredFractal):
	def generatePlotsByRegion(self):
		# Sirian's MultilayeredFractal class, controlling function.
		# You -MUST- customize this function for each use of the class.
		global yShiftRoll1
		global yShiftRoll2
		if self.map.getCustomMapOption(0) != 4:
			iContinentsGrainWest = 1 + self.map.getCustomMapOption(0)
			iContinentsGrainEast = 1 + self.map.getCustomMapOption(0)
		else:
			iContinentsGrainWest = 1 + self.dice.get(4, "West Continent Grain - Medium and Small PYTHON")
			iContinentsGrainEast = 1 + self.dice.get(4, "East Continent Grain - Medium and Small PYTHON")
		iIslandsGrain = 4 + self.map.getCustomMapOption(1)
		userInputOverlap = self.map.getCustomMapOption(2)

		# Add a few random patches of Tiny Islands first.
		numTinies = 1 + self.dice.get(4, "Tiny Islands - Medium and Small PYTHON")
		print("Patches of Tiny Islands: ", numTinies)
		if numTinies:
			for tiny_loop in range(numTinies):
				tinyWestLon = 0.01 * self.dice.get(85, "Tiny Longitude - Medium and Small PYTHON")
				tinyWestX = int(self.iW * tinyWestLon)
				tinySouthLat = 0.01 * self.dice.get(85, "Tiny Latitude - Medium and Small PYTHON")
				# <!-- custom: The stock script rolled an independent latitude but derived Y from longitude, diagonally correlating supposedly random tiny-island patches.
				# Use the latitude roll while preserving RNG order. See KI#262. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				tinySouthY = int(self.iH * tinySouthLat)
				tinyWidth = int(self.iW * 0.15)
				tinyHeight = int(self.iH * 0.15)

				self.generatePlotsInRegion(80, tinyWidth, tinyHeight, tinyWestX, tinySouthY, 4, 3, 0, self.iTerrainFlags, 6, 5, True, 3, -1, False, False)

		# Add the contental regions.

		# Add the Western Continent(s).
		iSouthY = 0
		iNorthY = self.iH - 1
		iHeight = iNorthY - iSouthY + 1
		iWestX = int(self.iW / 20)
		iEastX = int(self.iW * 0.45) - 1
		iWidth = iEastX - iWestX + 1
		print("Cont West: ", iWestX, "Cont East: ", iEastX, "Cont Width: ", iWidth)

		# Vertical dimensions may be affected by overlap and/or shift.
		if userInputOverlap: # Then both layers fill the entire region and overlap each other.
			# The north and south boundaries are already set (to max values).
			# Set Y exponent:
			yExp = 5
			# Also need to reduce amount of land plots, since there will be two layers in all areas.
			iWater = 80
		else: # The regions are separate, with continents only in one part, islands only in the other.
			iWater = 74
			# Set X exponent to square setting:
			yExp = 6
			# Handle horizontal shift for the Continents layer.
			# (This will choose one side or the other for this region then fit it properly in its space).
			if yShiftRoll1:
				southShift = int(0.4 * self.iH)
				northShift = 0
			else:
				southShift = 0
				northShift = int(0.4 * self.iH)

			iSouthY += southShift
			iNorthY -= northShift
			iHeight = iNorthY - iSouthY + 1
		print("Cont South: ", iSouthY, "Cont North: ", iNorthY, "Cont Height: ", iHeight)

		self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iContinentsGrainWest, 4, self.iRoundFlags, self.iTerrainFlags, 6, yExp, True, 15, -1, False, False)

		# Add the Western Islands.
		iWestX = int(self.iW * 0.12)
		iEastX = int(self.iW * 0.38) - 1
		iWidth = iEastX - iWestX + 1
		iSouthY = 0
		iNorthY = self.iH - 1
		iHeight = iNorthY - iSouthY + 1

		# Vertical dimensions may be affected by overlap and/or shift.
		if userInputOverlap: # Then both layers fill the entire region and overlap each other.
			# The north and south boundaries are already set (to max values).
			# Set Y exponent:
			yExp = 5
			# Also need to reduce amount of land plots, since there will be two layers in all areas.
			iWater = 80
		else: # The regions are separate, with continents only in one part, islands only in the other.
			iWater = 74
			# Set X exponent to square setting:
			yExp = 6
			# Handle horizontal shift for the Continents layer.
			# (This will choose one side or the other for this region then fit it properly in its space).
			if yShiftRoll1:
				southShift = 0
				northShift = int(0.4 * self.iH)
			else:
				southShift = int(0.4 * self.iH)
				northShift = 0

			iSouthY += southShift
			iNorthY -= northShift
			iHeight = iNorthY - iSouthY + 1
		print("Cont South: ", iSouthY, "Cont North: ", iNorthY, "Cont Height: ", iHeight)

		self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iIslandsGrain, 5, self.iRoundFlags, self.iTerrainFlags, 6, yExp, True, 15, -1, False, False)

		# Add the Eastern Continent(s).
		iSouthY = 0
		iNorthY = self.iH - 1
		iHeight = iNorthY - iSouthY + 1
		iWestX = int(self.iW * 0.55)
		iEastX = int(self.iW * 0.95) - 1
		iWidth = iEastX - iWestX + 1
		print("Cont West: ", iWestX, "Cont East: ", iEastX, "Cont Width: ", iWidth)

		# Vertical dimensions may be affected by overlap and/or shift.
		if userInputOverlap: # Then both layers fill the entire region and overlap each other.
			# The north and south boundaries are already set (to max values).
			# Set Y exponent:
			yExp = 5
			# Also need to reduce amount of land plots, since there will be two layers in all areas.
			iWater = 80
		else: # The regions are separate, with continents only in one part, islands only in the other.
			iWater = 74
			# Set X exponent to square setting:
			yExp = 6
			# Handle horizontal shift for the Continents layer.
			# (This will choose one side or the other for this region then fit it properly in its space).
			if yShiftRoll2:
				southShift = int(0.4 * self.iH)
				northShift = 0
			else:
				southShift = 0
				northShift = int(0.4 * self.iH)

			iSouthY += southShift
			iNorthY -= northShift
			iHeight = iNorthY - iSouthY + 1
		print("Cont South: ", iSouthY, "Cont North: ", iNorthY, "Cont Height: ", iHeight)

		# <!-- custom: The stock script independently rolled eastern continent grain but discarded it and reused the western grain, making both halves identical in grain 75% of the time that their rolls differed.
		# Use the east roll for the east region. See KI#263. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iContinentsGrainEast, 4, self.iRoundFlags, self.iTerrainFlags, 6, yExp, True, 15, -1, False, False)

		# Add the Eastern Islands.
		iWestX = int(self.iW * 0.62)
		iEastX = int(self.iW * 0.88) - 1
		iWidth = iEastX - iWestX + 1
		iSouthY = 0
		iNorthY = self.iH - 1
		iHeight = iNorthY - iSouthY + 1

		# Vertical dimensions may be affected by overlap and/or shift.
		if userInputOverlap: # Then both layers fill the entire region and overlap each other.
			# The north and south boundaries are already set (to max values).
			# Set Y exponent:
			yExp = 5
			# Also need to reduce amount of land plots, since there will be two layers in all areas.
			iWater = 80
		else: # The regions are separate, with continents only in one part, islands only in the other.
			iWater = 74
			# Set X exponent to square setting:
			yExp = 6
			# Handle horizontal shift for the Continents layer.
			# (This will choose one side or the other for this region then fit it properly in its space).
			if yShiftRoll2:
				southShift = 0
				northShift = int(0.4 * self.iH)
			else:
				southShift = int(0.4 * self.iH)
				northShift = 0

			iSouthY += southShift
			iNorthY -= northShift
			iHeight = iNorthY - iSouthY + 1
		print("Cont South: ", iSouthY, "Cont North: ", iNorthY, "Cont Height: ", iHeight)

		self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iIslandsGrain, 5, self.iRoundFlags, self.iTerrainFlags, 6, yExp, True, 15, -1, False, False)

		# All regions have been processed. Plot Type generation completed.
		print("Done")
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
	NiTextOut("Setting Plot Types (Python Medium and Small) ...")
	fractal_world = MnSMultilayeredFractal()
	plotTypes = fractal_world.generatePlotsByRegion()
	return plotTypes

def generateTerrainTypes():
	NiTextOut("Generating Terrain (Python Medium and Small) ...")
	terraingen = TerrainGenerator()
	terrainTypes = terraingen.generateTerrain()
	return terrainTypes

def addFeatures():
	NiTextOut("Adding Features (Python Medium and Small) ...")
	featuregen = FeatureGenerator()
	featuregen.addFeatures()
	return 0
