# advc.mxc: Renamed to "Mixed Continents" because I'm enabling the "Mixed In" option by default, because the old name was a bit long and because I don't want it to be listed above Fractal in alphabetic order. Also disabled some options; see the comments throughout this file. The script is still fully self-contained.
# The K-Mod changes in not_too_Big_or_Small.py "to scale better with larger maps" can't easily be combined with the "Mixed In" option, so I haven't tried to adopt those changes.

#	FILE:	 Continents_and_Islands.py
#	VERSION: 1.1
#	DATE: 08-30-07
#	AUTHOR:  James Morton (Jam3)
#	PURPOSE: Global map script - Mixed islands and continents.
#   	A modification of the script "Big and Small" included with
#		"Civ4 Beyond the Sword". I found that using larger "Islands" and adding
# 		more water to the iWater value made some very interesting Organic Maps.
#
#	v1.0 Initial Release
#	v1.1
#	Fixed
#		Call to TerrainGenerator (was incorrectly called in v1.0)
#	Added options
#		Reduce Desert
#		Add Plains
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
	#return "TXT_KEY_MAP_SCRIPT_BIG_AND_SMALL_DESCR"
	# advc.mxc:
	return "Variant of Jam3's \"Continents and Islands\", which, in turn, is based on Sirian's \"Big and Small\". Allows the \"small\" landmasses to be continents as well. Customized for the AdvCiv mod."

# <advc.165>
def getGridSize(argsList):
	[iWorldSize] = argsList
	if iWorldSize < 0:
		return ()
	sizeModifiers = {
		WorldSizeTypes.WORLDSIZE_DUEL:		( 0, 0),
		WorldSizeTypes.WORLDSIZE_TINY:		( 0,-1),
		WorldSizeTypes.WORLDSIZE_SMALL:		(-1, 0),
		WorldSizeTypes.WORLDSIZE_STANDARD:	(-1,-1),
		WorldSizeTypes.WORLDSIZE_LARGE:		(-2,-1),
		WorldSizeTypes.WORLDSIZE_HUGE:		(-2,-2)
	}
	wi = CyGlobalContext().getWorldInfo(iWorldSize)
	return (sizeModifiers[iWorldSize][0] + wi.getGridWidth(), sizeModifiers[iWorldSize][1] + wi.getGridHeight())
# </advc.165>

def isAdvancedMap():
	"This map should not show up in simple mode"
	return 0

def getNumCustomMapOptions():
	return 5 # advc.mxc


def getCustomMapOptionName(argsList):
	[iOption] = argsList
	option_names = {
		#0:	"TXT_KEY_MAP_SCRIPT_CONTINENTS_SIZE",
		#1:	"TXT_KEY_MAP_SCRIPT_ISLANDS_SIZE",
		#2:	"TXT_KEY_MAP_SCRIPT_ISLAND_OVERLAP",
		# <advc.mxc> There's no fundamental difference between the two size parameters.
		0:	"Region 1",
		1:	"Region 2",
		2:	"Overlap", # </advc.mxc>
		3:	"TXT_KEY_MAP_WORLD_WRAP", # advc.mxc
		4:	"Terrain Clumping"
		#,5:  "Add Water",
		#6:	"Reduce Desert",
		#7:	"Add Plains",
		}
	translated_text = unicode(CyTranslator().getText(option_names[iOption], ()))
	return translated_text


def getNumCustomMapOptionValues(argsList):
	[iOption] = argsList
	option_values = {
		0:	5, # advc.mxc: was 3
		1:	5,
		2:	2,
		3:	3, # advc.mxc: world wrap
		4:	4
		#,5:4,
		#6:	6,
		#7:	6
		}
	return option_values[iOption]


def getCustomMapOptionDescAt(argsList):
	[iOption, iSelection] = argsList
	selection_names = {
		0:	{
			0: "TXT_KEY_MAP_SCRIPT_MASSIVE_CONTINENTS",
			1: "TXT_KEY_MAP_SCRIPT_NORMAL_CONTINENTS",
			2: "TXT_KEY_MAP_SCRIPT_SNAKY_CONTINENTS",
			# <advc.mxc> Might as well allow islands in region 1 as well (though that won't play any better than Archipelago).
			3: "TXT_KEY_MAP_SCRIPT_ISLANDS",
			4: "TXT_KEY_MAP_SCRIPT_TINY_ISLANDS" # </advc.mxc>
			},
		1:	{ # advc.mxc: Use (BtS) TXT_KEYs instead of English-only string literals
			0: "TXT_KEY_MAP_SCRIPT_MASSIVE_CONTINENTS",
			1: "TXT_KEY_MAP_SCRIPT_NORMAL_CONTINENTS",
			2: "TXT_KEY_MAP_SCRIPT_SNAKY_CONTINENTS",
			3: "TXT_KEY_MAP_SCRIPT_ISLANDS",
			4: "TXT_KEY_MAP_SCRIPT_TINY_ISLANDS"
			},
		2:	{
			#0: "TXT_KEY_MAP_SCRIPT_ISLAND_REGION_SEPARATE",
			#1: "TXT_KEY_MAP_SCRIPT_ISLANDS_MIXED_IN"
			0: "Separate regions",
			1: "Regions mixed together"
			},
		# <advc.mxc>
		3:	{
			0: "TXT_KEY_MAP_WRAP_FLAT",
			1: "TXT_KEY_MAP_WRAP_CYLINDER",
			2: "TXT_KEY_MAP_WRAP_TOROID"
			}, # </advc.mxc>
		4:	{
			0: "Very Clumped",
			1: "Clumped",
			2: "Normal",
			3: "Varied"
			}
		# advc.mxc: If these are re-enabled, then the labels should arguably say what is being added, e.g. "Some extra water (5%)", so that the (AdvCiv) Settings tab will not just say "5%".
		#,5:{
		#	0: "0%",
		#	1: "5%",
		#	2: "10%",
		#	3: "15%"
		#	},
		#6:	{
		#	0: "0%",
		#	1: "25%",
		#	2: "33%",
		#	3: "50%",
		#	4: "66%",
		#	5: "75%"
		#	},
		#7:	{
		#	0: "0%",
		#	1: "25%",
		#	2: "33%",
		#	3: "50%",
		#	4: "66%",
		#	5: "75%"
		#	}
		}
	translated_text = unicode(CyTranslator().getText(selection_names[iOption][iSelection], ()))
	return translated_text


def getCustomMapOptionDefault(argsList):
	[iOption] = argsList
	option_defaults = {
		0:	1,
		1:	2, # advc.mxc: was 3 (islands), now snaky
		2:	1, # advc.mxc: was 2 (separate regions), now mixed
		3:	1, # advc.mxc: cylinder
		4:	2
		#,5:0,
		#6:	0,
		#7:	0
		}
	return option_defaults[iOption]


# <advc.mxc>
def getWrapX():
	return (CyMap().getCustomMapOption(3) >= 1)


def getWrapY():
	return (CyMap().getCustomMapOption(3) == 2) # </advc.mxc>


def minStartingDistanceModifier():
	return -12


def beforeGeneration():
	global xShiftRoll
	gc = CyGlobalContext()
	dice = gc.getGame().getMapRand()
	# Binary shift roll (for horizontal shifting if Island Region Separate).
	xShiftRoll = dice.get(2, "Region Shift, Horizontal - Big and Small PYTHON")
	print("xShiftRoll", xShiftRoll)


class BnSMultilayeredFractal(CvMapGeneratorUtil.MultilayeredFractal):
	def generatePlotsByRegion(self):
		global xShiftRoll
		# advc.mxc: disable these two
		#global iGlobalDesertMenu
		#global iGlobalPlainsMenu
		global iGlobalClumpingMenu

		#JAM3 Assign Globals # advc.mxc: Moved up (do the globals in one place)
		#iGlobalDesertMenu = self.map.getCustomMapOption(6)
		#iGlobalPlainsMenu = self.map.getCustomMapOption(7)
		iGlobalClumpingMenu = 2 + self.map.getCustomMapOption(4)
		aiLandmassGrain = [ 1 + self.map.getCustomMapOption(0), 1 + self.map.getCustomMapOption(1) ] # advc.mxc: instead of two primitive variables
		userInputOverlap = self.map.getCustomMapOption(2)

		#JAM3 Add Water: the 74 and 82 values are close enough [...]
		# <advc.mxc> # ^Disagree. Moved up:
		iWater = 80 # was 74
		# The sea level setting previously had no effect (same as in the other Big&Small-based scripts - shockingly). I guess that's why the "Add water" option was added; I'm disabling that option.
		iSeaLevelChange = CyGlobalContext().getSeaLevelInfo(self.map.getSeaLevel()).getSeaLevelChange()
		# Normally, this is how the sea level is applied - but that doesn't work here at all.
		#iWater += iSeaLevelChange
		# Through trial and error. For a land ratio that is mostly independent from userInputOverlap and resembles Fractal.
		if userInputOverlap:
			if iSeaLevelChange < 0:
				iSeaLevelChange = (3 * iSeaLevelChange) / 5
			if iSeaLevelChange > 0:
				iSeaLevelChange = (3 * iSeaLevelChange) / 4
		iWater = (iWater * (100 + iSeaLevelChange)) / 100

		if userInputOverlap:
			iWater += 7 # was (effectively) +8 </advc.mxc>
		#JAM3: the 74 and 82 values are close enough and the % is calculated from % of 82
		# advc.mxc: I think the above means that the increments in the if/elif below were calculated by taking the percent labels of the add-water option times 82. Let's make that explicit (just in case that the extra water option is re-enabled):
		#if self.map.getCustomMapOption(5) == 1:
		#	iWater = (iWater * 105) / 100 # advc.mxc: was +=4
		#elif self.map.getCustomMapOption(5) == 2:
		#	iWater = (iWater * 110) / 100 # advc.mxc: was +=8
		#elif self.map.getCustomMapOption(5) == 3:
		#	iWater = (iWater * 115) / 100 # advc.mxc: was +=12

		# Add a few random patches of Tiny Islands first.
		iMaxTinies = 1 + self.map.getWorldSize() # advc.mxc: was 4 flat
		numTinies = 1 + self.dice.get(iMaxTinies, "Tiny Islands - Custom Continents PYTHON")
		print("Patches of Tiny Islands: ", numTinies)
		if numTinies:
			for tiny_loop in range(numTinies):
				tinyWestLon = 0.01 * self.dice.get(85, "Tiny Longitude - Custom Continents PYTHON")
				tinyWestX = int(self.iW * tinyWestLon)
				tinySouthLat = 0.01 * self.dice.get(85, "Tiny Latitude - Custom Continents PYTHON")
				tinySouthY = int(self.iH * tinyWestLon)
				tinyWidth = int(self.iW * 0.15)
				tinyHeight = int(self.iH * 0.15)
				# advc.mxc: was 3 - which is an unusually coarse grain. Probably the opposite was intended.
				iHillGrain = 5
				self.generatePlotsInRegion(80, tinyWidth, tinyHeight, tinyWestX, tinySouthY, 4, iHillGrain, 0, self.iTerrainFlags, 6, 5, True, 3)

		# North and South dimensions always fill the entire vertical span for this script.
		iSouthY = 0
		iNorthY = self.iH - 1
		iHeight = iNorthY - iSouthY + 1
		iWestX = 0
		iEastX = self.iW - 1
		iWidth = iEastX - iWestX + 1
		print("Regions South: ", iSouthY, "North: ", iNorthY, "Height: ", iHeight)

		# Add the Continents.
		# ...
		# Add the Islands.
		# advc.mxc: Use a loop to get rid of duplicate code
		for iPass in range(2):
			if iPass == 1:
				iWestX = 0
				iEastX = self.iW - 1
				iWidth = iEastX - iWestX + 1
			# Horizontal dimensions may be affected by overlap and/or shift.
			if userInputOverlap: # Then both regions fill the entire map and overlap each other.
				# The west and east boundaries are already set (to max values).
				# Set X exponent to normal setting:
				xExp = 7
				# Also need to reduce amount of land plots, since there will be two layers in all areas.
				#iWater = 82 + addWater # advc.mxc: Handled higher up
			else: # The regions are separate
				# Set X exponent to square setting:
				xExp = 6
				# Handle horizontal shift
				# (This will choose one side or the other for this region then fit it properly in its space).
				if xShiftRoll:
					westShift = int(0.4 * self.iW)
					eastShift = 0
				else:
					westShift = 0
					eastShift = int(0.4 * self.iW)
				# <advc.mxc> Swap for the 2nd region
				if iPass == 1:
					westShift, eastShift = eastShift, westShift
				# </advc.mxc>
				iWestX += westShift
				iEastX -= eastShift
				iWidth = iEastX - iWestX + 1

			print("Region ", iPass + 1, " West: ", iWestX, "East: ", iEastX, "Width: ", iWidth)
			# <advc.mxc> This was 4 for the "continents" region and 5 for the "islands" region.
			iHillGrain = 4
			#  Could set it based on aiLandmassGrain[iPass] ...
			#if aiLandmassGrain[iPass] <= 0: # Massive
			#	iHillGrain = 3
			#elif aiLandmassGrain[iPass] >= 3: # Islands or tiny islands
			#	iHillGrain = 5
			# ... but I'm not convinced that the hill distribution should depend on the landmass size much.
			# </advc.mxc>
			self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, aiLandmassGrain[iPass], iHillGrain, self.iRoundFlags, self.iTerrainFlags, xExp, 6)#, True, 15, -1, False, False) # advc.mxc: Last params are the same as the defaults; better use the defaults then.

		# All regions have been processed. Plot Type generation completed.
		#print "Done"
		return self.wholeworldPlotTypes


'''
Regional Variables Key:   (params of generatePlotsInRegion)

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

	# advc.mxc (note): These two if/elif blocks don't do anything b/c the global values are 0. I'm leaving them alone to make it easy to re-enable the plains and desert options.
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

	#terraingen = TerrainGenerator(iLocalDesertPercent, iLocalPlainsPercent, 0.8, 0.7, 0.1, 0.2, 0.5, -1, -1, iLocalClumpingMenu)
	# advc.mxc: Was using fSnowLatitude=0.8, fTundraLatitude=0.7. I agree that the Warlords values (0.7 and 0.6 respectively) are too low, but the new values are too high. advc.tsl sets realistic latitude thresholds in CvMapGeneratorUtil.py, so we should just use those default values.
	terraingen = TerrainGenerator(iDesertPercent=iLocalDesertPercent, iPlainsPercent=iLocalPlainsPercent, grain_amount=iLocalClumpingMenu)

	terrainTypes = terraingen.generateTerrain()
	return terrainTypes


def addFeatures():
	NiTextOut("Adding Features (Python Custom Continents) ...")
	featuregen = FeatureGenerator()
	featuregen.addFeatures()
	return 0
