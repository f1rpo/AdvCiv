#	FILE:	 Continents_and_Islands.py
#	VERSION: 1.1
#	DATE: 08-30-07
#	AUTHOR:  James Morton (Jam3)
#	PURPOSE: Global map script - Mixed islands and continents.
#       A modification of the included script "Big and Small" included with Civ4 beyond the Sword"
#	I found that using larger "Islands" and adding more water to the iWater value
# 	made some very interesting Organic Maps. This is an Enhancement to the orginal provided "BiG and Small"
#	Map Script and uses the default terrain generator.
#
#	v1.0 Initial Release
#	v1.1
#	Fixed
#		Call to TerrainGenerator
#			*was incorrectly called in v1.0
#	Added Menu option
#		Reduce desert
#		Add plains
#		Terrain Clumping
#	Changed
#		Names of Islands in the Island Menu to the continent names
#			*Just felt this was more clear
#



#	FILE:	 Big_and_Small.py
#	AUTHOR:  Bob Thomas (Sirian)
#	PURPOSE: Global map script - Mixed islands and continents.
#-----------------------------------------------------------------------------
#	Copyright (c) 2007 Firaxis Games, Inc. All rights reserved.
#-----------------------------------------------------------------------------
#


from CvPythonExtensions import *
import CvUtil
import CvMapGeneratorUtil
from CvMapGeneratorUtil import FractalWorld
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator

#Jam3 Global Variables
iGlobalDesertMenu = 0
iGlobalPlainsMenu = 0
iGlobalClumpingMenu = 2

#JAM3 Menu options added but commented in the menu functions

def getDescription():
	return "TXT_KEY_MAP_SCRIPT_BIG_AND_SMALL_DESCR"

def isAdvancedMap():
	"This map should not show up in simple mode"
	return 0

def getNumCustomMapOptions():
	return 7

def getCustomMapOptionName(argsList):
	[iOption] = argsList
	option_names = {
		0:	"TXT_KEY_MAP_SCRIPT_CONTINENTS_SIZE",
		1:	"TXT_KEY_MAP_SCRIPT_ISLANDS_SIZE",
		2:	"TXT_KEY_MAP_SCRIPT_ISLAND_OVERLAP",
		3:      "Add Water",
		4:	"Reduce Desert",
		5:	"Add Plains",
		6:	"Terrain Clumping"
		}
	translated_text = unicode(CyTranslator().getText(option_names[iOption], ()))
	return translated_text

def getNumCustomMapOptionValues(argsList):
	[iOption] = argsList
	option_values = {
		0:	3,
		1:	5,
		2:	2,
		3:	4,
		4:	6,
		5:	6,
		6:	4
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
			0: "Massive Continents",
			1: "Normal Continents",
			2: "Snaky Continents",
			3: "Normal Islands",
			4: "Tiny Islands"
			},
		2:	{
			0: "TXT_KEY_MAP_SCRIPT_ISLAND_REGION_SEPARATE",
			1: "TXT_KEY_MAP_SCRIPT_ISLANDS_MIXED_IN"
			},
		3:	{
			0: "0%",
			1: "5%",
			2: "10%",
			3: "15%"
			},
		4:	{
			0: "0%",
			1: "25%",
			2: "33%",
			3: "50%",
			4: "66%",
			5: "75%"
			},
		5:	{
			0: "0%",
			1: "25%",
			2: "33%",
			3: "50%",
			4: "66%",
			5: "75%"
			},
		6:	{
			0: "Very Clumped",
			1: "Clumped",
			2: "Normal",
			3: "Varied"
			}
		}
	translated_text = unicode(CyTranslator().getText(selection_names[iOption][iSelection], ()))
	return translated_text

def getCustomMapOptionDefault(argsList):
	[iOption] = argsList
	option_defaults = {
		0:	1,
		1:	3,
		2:	0,
		3:	0,
		4:	0,
		5:	0,
		6:	2
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

	print xShiftRoll



class BnSMultilayeredFractal(CvMapGeneratorUtil.MultilayeredFractal):
	def generatePlotsByRegion(self):
		# Sirian's MultilayeredFractal class, controlling function.
		# You -MUST- customize this function for each use of the class.
		global xShiftRoll
		global iGlobalDesertMenu
		global iGlobalPlainsMenu
		global iGlobalClumpingMenu


		iContinentsGrain = 1 + self.map.getCustomMapOption(0)
		iIslandsGrain = 1 + self.map.getCustomMapOption(1)
		userInputOverlap = self.map.getCustomMapOption(2)

		#JAM3 Add Water the 74 and 82 values are close enough and the % is calculated from % of 82
		addWater = 0
		if self.map.getCustomMapOption(3) == 1:
			addWater = 4
		elif self.map.getCustomMapOption(3) == 2:
			addWater = 8
		elif self.map.getCustomMapOption(3) == 3:
			addWater = 12
		# Water variables need to differ if Overlap is set. Defining default here.
		iWater = 74 + addWater

		#JAM3 Assign Globals
		iGlobalDesertMenu = self.map.getCustomMapOption(4)
		iGlobalPlainsMenu = self.map.getCustomMapOption(5)
		iGlobalClumpingMenu = 2 + self.map.getCustomMapOption(6)

		# Add a few random patches of Tiny Islands first.
		numTinies = 1 + self.dice.get(4, "Tiny Islands - Custom Continents PYTHON")
		print("Patches of Tiny Islands: ", numTinies)
		if numTinies:
			for tiny_loop in range(numTinies):
				tinyWestLon = 0.01 * self.dice.get(85, "Tiny Longitude - Custom Continents PYTHON")
				tinyWestX = int(self.iW * tinyWestLon)
				tinySouthLat = 0.01 * self.dice.get(85, "Tiny Latitude - Custom Continents PYTHON")
				tinySouthY = int(self.iH * tinyWestLon)
				tinyWidth = int(self.iW * 0.15)
				tinyHeight = int(self.iH * 0.15)

				self.generatePlotsInRegion(80,
				                           tinyWidth, tinyHeight,
				                           tinyWestX, tinySouthY,
				                           4, 3,
				                           0, self.iTerrainFlags,
				                           6, 5,
				                           True, 3,
				                           -1, False,
				                           False
				                           )

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
			iWater = 82 + addWater
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

		self.generatePlotsInRegion(iWater,
		                           iWidth, iHeight,
		                           iWestX, iSouthY,
		                           iContinentsGrain, 4,
		                           self.iRoundFlags, self.iTerrainFlags,
		                           xExp, 6,
		                           True, 15,
		                           -1, False,
		                           False
		                           )

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


		self.generatePlotsInRegion(iWater,
		                           iWidth, iHeight,
		                           iWestX, iSouthY,
		                           iIslandsGrain, 5,
		                           self.iRoundFlags, self.iTerrainFlags,
		                           xExp, 6,
		                           True, 15,
		                           -1, False,
		                           False
		                           )

		# All regions have been processed. Plot Type generation completed.
		print "Done"
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

	#JAM3 Declare Globals and pass them to the TerrainGenerator
	global iGlobalDesertMenu
	global iGlobalPlainsMenu
	global iGlobalClumpingMenu

	NiTextOut("Generating Terrain (Python Custom Continents) ...")

	iLocalDesertPercent = 32
	iLocalPlainsPercent = 18
	iLocalClumpingMenu = iGlobalClumpingMenu

	if iGlobalDesertMenu == 0:
		iLocalDesertPercent = 32
	elif iGlobalDesertMenu == 1:
		iLocalDesertPercent = 24
	elif iGlobalDesertMenu == 2:
		iLocalDesertPercent = 21
	elif iGlobalDesertMenu == 3:
		iLocalDesertPercent = 16
	elif iGlobalDesertMenu == 4:
		iLocalDesertPercent = 11
	elif iGlobalDesertMenu == 5:
		iLocalDesertPercent = 8


	if iGlobalPlainsMenu == 0:
		iLocalPlainsPercent = 18
	elif iGlobalPlainsMenu == 1:
		iLocalPlainsPercent = 22
	elif iGlobalPlainsMenu == 2:
		iLocalPlainsPercent = 24
	elif iGlobalPlainsMenu == 3:
		iLocalPlainsPercent = 27
	elif iGlobalPlainsMenu == 4:
		iLocalPlainsPercent = 30
	elif iGlobalPlainsMenu == 5:
		iLocalPlainsPercent = 32


	terraingen = TerrainGenerator(iLocalDesertPercent, iLocalPlainsPercent, 0.8, 0.7, 0.1, 0.2, 0.5, -1, -1, iLocalClumpingMenu)
	terrainTypes = terraingen.generateTerrain()
	return terrainTypes

def addFeatures():
	NiTextOut("Adding Features (Python Custom Continents) ...")
	featuregen = FeatureGenerator()
	featuregen.addFeatures()
	return 0
