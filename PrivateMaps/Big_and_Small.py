#
#	FILE:	 Big_and_Small.py
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
	return "TXT_KEY_MAP_SCRIPT_BIG_AND_SMALL_DESCR"

def isAdvancedMap():
	# <!-- custom: hide this map from Simple Game to keep that list curated; still available in Custom Game. (GPT-5.3-Codex) -->
	return 1

def getNumPlotsPercent(argsList):
	# <!-- custom: Huge tested roughly 10-15% too large after the shared world-size helper leak was fixed; reduce larger tiers without changing the generator's continent/island logic. (GPT-5.5?) -->
	[eWorldSize] = argsList
	if eWorldSize < 0:
		return 100
	sizeModifiers = {
		WorldSizeTypes.WORLDSIZE_ARENA: 100,
		WorldSizeTypes.WORLDSIZE_DUEL: 100,
		WorldSizeTypes.WORLDSIZE_TINY: 100,
		WorldSizeTypes.WORLDSIZE_SMALL: 100,
		WorldSizeTypes.WORLDSIZE_STANDARD: 100,
		WorldSizeTypes.WORLDSIZE_LARGE: 95,
		WorldSizeTypes.WORLDSIZE_HUGE: 88
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
		0:	3,
		1:	2,
		2:	2
		}
	return option_values[iOption]

def getCustomMapOptionDescAt(argsList):
	[iOption, iSelection] = argsList
	selection_names = {
		0:	{
			0: "TXT_KEY_MAP_SCRIPT_MASSIVE_CONTINENTS",
			1: "TXT_KEY_MAP_SCRIPT_NORMAL_CONTINENTS",
			2: "TXT_KEY_MAP_SCRIPT_SNAKY_CONTINENTS"
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
		0:	1,
		1:	0,
		2:	0
		}
	return option_defaults[iOption]

def minStartingDistanceModifier():
	return -12

def beforeGeneration():
	global xShiftRoll
	gc = CyGlobalContext()
	dice = gc.getGame().getMapRand()

	# Binary shift roll (for horizontal shifting if Island Region Separate).
	xShiftRoll = dice.get(2, "Region Shift, Horizontal - Big and Small PYTHON")
	print(xShiftRoll)

class BnSMultilayeredFractal(CvMapGeneratorUtil.MultilayeredFractal):
	def generatePlotsByRegion(self):
		# Sirian's MultilayeredFractal class, controlling function.
		# You -MUST- customize this function for each use of the class.
		global xShiftRoll
		iContinentsGrain = 1 + self.map.getCustomMapOption(0)
		iIslandsGrain = 4 + self.map.getCustomMapOption(1)
		userInputOverlap = self.map.getCustomMapOption(2)

		# Water variables need to differ if Overlap is set. Defining default here.
		iWater = 74

		# Add a few random patches of Tiny Islands first.
		numTinies = 1 + self.dice.get(4, "Tiny Islands - Custom Continents PYTHON")
		print("Patches of Tiny Islands: ", numTinies)
		if numTinies:
			for tiny_loop in range(numTinies):
				tinyWestLon = 0.01 * self.dice.get(85, "Tiny Longitude - Custom Continents PYTHON")
				tinyWestX = int(self.iW * tinyWestLon)
				tinySouthLat = 0.01 * self.dice.get(85, "Tiny Latitude - Custom Continents PYTHON")
				# <!-- custom: The stock script rolled an independent latitude but derived Y from longitude, diagonally correlating supposedly random tiny-island patches.
				# Use the latitude roll while preserving RNG order. See KI#262. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				tinySouthY = int(self.iH * tinySouthLat)
				tinyWidth = int(self.iW * 0.15)
				tinyHeight = int(self.iH * 0.15)

				self.generatePlotsInRegion(80, tinyWidth, tinyHeight, tinyWestX, tinySouthY, 4, 3, 0, self.iTerrainFlags, 6, 5, True, 3, -1, False, False)

		# North and South dimensions always fill the entire vertical span for this script.
		iSouthY = 0
		iNorthY = self.iH - 1
		iHeight = iNorthY - iSouthY + 1
		iWestX = 0
		iEastX = self.iW - 1
		iWidth = iEastX - iWestX + 1
		print("Cont South: ", iSouthY, "Cont North: ", iNorthY, "Cont Height: ", iHeight)

		# Add the Continents.
		# Horizontal dimensions may be affected by overlap and/or shift.
		if userInputOverlap: # Then both regions fill the entire map and overlap each other.
			# The west and east boundaries are already set (to max values).
			# Set X exponent to normal setting:
			xExp = 7
			# Also need to reduce amount of land plots, since there will be two layers in all areas.
			iWater = 82
		else: # The regions are separate, with continents only in one part, islands only in the other.
			# Set X exponent to square setting:
			xExp = 6
			# Handle horizontal shift for the Continents layer.
			# (This will choose one side or the other for this region then fit it properly in its space).
			if xShiftRoll:
				westShift = int(0.4 * self.iW)
				eastShift = 0
			else:
				westShift = 0
				eastShift = int(0.4 * self.iW)

			iWestX += westShift
			iEastX -= eastShift
			iWidth = iEastX - iWestX + 1
		print("Cont West: ", iWestX, "Cont East: ", iEastX, "Cont Width: ", iWidth)

		self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iContinentsGrain, 4, self.iRoundFlags, self.iTerrainFlags, xExp, 6, True, 15, -1, False, False)

		# Add the Islands.
		iWestX = 0
		iEastX = self.iW - 1
		iWidth = iEastX - iWestX + 1

		# Horizontal dimensions may be affected by overlap and/or shift.
		if userInputOverlap: # Then both regions fill the entire map and overlap each other.
			# The west and east boundaries are already set (to max values).
			# Set X exponent to normal setting:
			xExp = 7
			# Also need to reduce amount of land plots, since there will be two layers in all areas.
			iWater = 82
		else: # The regions are separate, with continents only in one part, islands only in the other.
			# Set X exponent to square setting:
			xExp = 6
			# Handle horizontal shift for the Continents layer.
			# (This will choose one side or the other for this region then fit it properly in its space).
			if xShiftRoll:
				westShift = 0
				eastShift = int(0.4 * self.iW)
			else:
				westShift = int(0.4 * self.iW)
				eastShift = 0

			iWestX += westShift
			iEastX -= eastShift
			iWidth = iEastX - iWestX + 1
		print("Island West: ", iWestX, "Island East: ", iEastX, "Isl Width: ", iWidth)

		self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iIslandsGrain, 5, self.iRoundFlags, self.iTerrainFlags, xExp, 6, True, 15, -1, False, False)

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
	NiTextOut("Setting Plot Types (Python Custom Continents) ...")
	fractal_world = BnSMultilayeredFractal()
	plotTypes = fractal_world.generatePlotsByRegion()
	return plotTypes

def generateTerrainTypes():
	NiTextOut("Generating Terrain (Python Custom Continents) ...")
	terraingen = TerrainGenerator()
	terrainTypes = terraingen.generateTerrain()
	return terrainTypes

def addFeatures():
	NiTextOut("Adding Features (Python Custom Continents) ...")
	featuregen = FeatureGenerator()
	featuregen.addFeatures()
	return 0
