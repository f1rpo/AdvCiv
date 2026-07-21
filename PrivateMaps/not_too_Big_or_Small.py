## "not too Big or Small". A modified version of "big and small" to scale better with larger maps.
## by Karadoc. version 1.4
# advc: Some tweaks; see in-inline comments. Changes tagged with "advc.mxc" were ported from Mixed_Continents.

from CvPythonExtensions import *
import CvUtil
import CvMapGeneratorUtil
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator

def getDescription():
	return "A modified version of Big and Small, designed to scale better for large maps."

# advc.165:
def getNumPlotsPercent(argsList):
	[iWorldSize] = argsList
	if iWorldSize < 0:
		return 100
	sizeModifiers = {
		WorldSizeTypes.WORLDSIZE_DUEL:		100,
		WorldSizeTypes.WORLDSIZE_TINY:		98,
		WorldSizeTypes.WORLDSIZE_SMALL:		92,
		WorldSizeTypes.WORLDSIZE_STANDARD:	87,
		WorldSizeTypes.WORLDSIZE_LARGE:		83,
		WorldSizeTypes.WORLDSIZE_HUGE:		79
	}
	if iWorldSize >= len(sizeModifiers):
		return 75
	return sizeModifiers[iWorldSize]

def isAdvancedMap():
	"This map should not show up in simple mode"
	return 0

def getNumCustomMapOptions():
	return 3 # kekm.32

def getCustomMapOptionName(argsList):
	[iOption] = argsList
	option_names = {
		0:	"TXT_KEY_MAP_SCRIPT_CONTINENTS_SIZE",
		1:	"TXT_KEY_MAP_SCRIPT_ISLANDS_SIZE",
		2:	"TXT_KEY_MAP_WORLD_WRAP" # kekm.32
		}
	translated_text = unicode(CyTranslator().getText(option_names[iOption], ()))
	return translated_text

def getNumCustomMapOptionValues(argsList):
	[iOption] = argsList
	option_values = {
		0:	3,
		1:	2,
		2:	3 # kekm.32
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
		# <kekm.32>
		2:	{
			0: "TXT_KEY_MAP_WRAP_FLAT",
			1: "TXT_KEY_MAP_WRAP_CYLINDER",
			2: "TXT_KEY_MAP_WRAP_TOROID"
			} # </kekm.32>
		}
	translated_text = unicode(CyTranslator().getText(selection_names[iOption][iSelection], ()))
	return translated_text

def getCustomMapOptionDefault(argsList):
	[iOption] = argsList
	option_defaults = {
		0:	1,
		1:	0,
		2:	1 # kekm.32
		}
	return option_defaults[iOption]
# <kekm.32>
def getWrapX():
	return (CyMap().getCustomMapOption(2) >= 1)

def getWrapY():
	return (CyMap().getCustomMapOption(2) == 2) # </kekm.32>

# advc.027: Leave this up to the DLL entirely
#def minStartingDistanceModifier():
#	return -12

def beforeGeneration():
	#global xShiftRoll # advc (comment): Overlap option disabled by K-Mod
	gc = CyGlobalContext()
	dice = gc.getGame().getMapRand()

	# Binary shift roll (for horizontal shifting if Island Region Separate).
	#xShiftRoll = dice.get(2, "Region Shift, Horizontal - Big and Small PYTHON")
	#print xShiftRoll

class BnSMultilayeredFractal(CvMapGeneratorUtil.MultilayeredFractal):
	def generatePlotsByRegion(self):
		#global xShiftRoll
		iContinentsGrain = 1 + self.map.getCustomMapOption(0)
		iIslandsGrain = 4 + self.map.getCustomMapOption(1)
		# advc.mxc: Take into account sea level (Big/Medium&Small also don't do that)
		iSeaLevelChange = CyGlobalContext().getSeaLevelInfo(self.map.getSeaLevel()).getSeaLevelChange()

		# <K-Mod>
		iTargetSize = 30 + self.dice.get(min(36, self.iW/3), "zone target size (horiz)")
		iHorizontalZones = max(1, (self.iW+iTargetSize/2) / iTargetSize)
		iTargetSize = 30 + self.dice.get(min(34, self.iH/2), "zone target size (vert)")
		iVerticalZones = max(1, (self.iH+iTargetSize/2) / iTargetSize)

		# if iHorizontalZones == 1 and iVerticalZones == 1:
			# iHorizontalZones = 1 + self.dice.get(2, "Saving throw vs. Pangaea")

		iTotalZones = iHorizontalZones * iVerticalZones
		iContinentZones = (iTotalZones+1)/2 + self.dice.get(1+(iTotalZones-1)/2, "number of 'big' zones")
		iIslandZones = iTotalZones - iContinentZones
		# Add a few random patches of Tiny Islands first. (originally 1 + r(4))
		numTinies = iContinentZones + self.dice.get(2 + iTotalZones, "number of Tiny Islands")
		# </K-Mod>
		print("Patches of Tiny Islands: ", numTinies)
		if numTinies:
			iWater = 80
			# <advc.mxc> Let's adjust the tiny islands' water a little too
			iWater += iSeaLevelChange // 2
			iWater = min(iWater, 90) # </advc.mxc>
			for tiny_loop in range(numTinies):
				tinyWestLon = 0.01 * self.dice.get(85, "Tiny Longitude - Custom Continents PYTHON")
				tinyWestX = int(self.iW * tinyWestLon)
				tinySouthLat = 0.01 * self.dice.get(85, "Tiny Latitude - Custom Continents PYTHON")
				tinySouthY = int(self.iH * tinyWestLon)
				tinyWidth = int(self.iW * 0.15)
				tinyHeight = int(self.iH * 0.15)
				iHillGrain = 5 # advc.mxc: was 3
				self.generatePlotsInRegion(iWater,
				                           tinyWidth, tinyHeight,
				                           tinyWestX, tinySouthY,
				                           4, iHillGrain,
				                           0, self.iTerrainFlags,
				                           6, 5,
				                           True, 3)
		# <K-Mod>
		zone_types = [0] * iTotalZones
		i = 0
		while i < iContinentZones:
			x = self.dice.get(iTotalZones - i, "zone placement")
			j = 0
			while j <= x:
				if (zone_types[j] == 1):
					x = x + 1
				j += 1
			zone_types[x] = 1
			i += 1

		iZoneWidth = int(self.iW / iHorizontalZones)
		iZoneHeight = int(self.iH / iVerticalZones)

		xExp = 6
		iMaxOverLap = 5
		#iWater = 74
		# <advc.mxc> Moved down.
		# Use a slightly higher base percentage to match Fractal more closely (but not quite - island maps should have a little extra land and I'm also shrinking the dimensions a little through advc.165).
		iWater = 75
		# This is what FractalWorld does and it seems to work well enough here as well:
		iWater += iSeaLevelChange
		iWater = min(iWater, 90) # </advc.mxc>
		for i in range(iTotalZones):
			iWestX = max(0, (i % iHorizontalZones) * iZoneWidth - self.dice.get(iMaxOverLap, "zone overlap (west)"))
			iEastX = min(self.iW - 1, (i % iHorizontalZones + 1) * iZoneWidth + self.dice.get(iMaxOverLap, "zone overlap (east)"))
			iSouthY = max(0, max(3, (i / iHorizontalZones) * iZoneHeight) - self.dice.get(iMaxOverLap, "zone overlap (south)"))
			iNorthY = min(self.iH - 1, min(self.iH - 4, (i / iHorizontalZones + 1) * iZoneHeight) + self.dice.get(iMaxOverLap, "zone overlap (north)"))

			iWidth = iEastX - iWestX + 1
			iHeight = iNorthY - iSouthY + 1

			if (zone_types[i] == 1):
				# continent zone </K-Mod>
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
			# <K-Mod>
			else:
				# islands zone # </K-Mod>
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
