# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Adapted from Beyond the Game mod map script, version 2.43. (GPT-5.3-Codex) -->

from CvPythonExtensions import *
import CvUtil
import CvMapGeneratorUtil
from math import sqrt
from CvMapGeneratorUtil import FractalWorld
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator
from CvMapGeneratorUtil import BonusBalancer
from SASUtils import getInfoTypeOrFail
from SAS_WorldSizeUtils import *

import random #2.25

balancer = BonusBalancer()

def getDescription():#The BTS description, used in general game setup, not the MapPreview screen in game from BTS
	return "Modified BTG_Lagoon map from the Beyond the Game mod (by Penny). Adapted for AdvCiv-SAS: upscaled, non-AdvCiv options removed, and SAS48/high player counts supported."

def getDescriptionTitle():
	return "A map deisgned for two sides, as a top v bottom. There will be 1 player of each team in a very rich central donut"

def getDescriptionTitleTwo():
	return "other players will be on the side of the map and far apart, and will have to choose to come to the middle or not. Middle will be very difficult to reach on foot, easier by boat"

def getDescriptionMain():
	return "Center design keeps early conflict limited while preserving a reachable central objective on larger AdvCiv-SAS world sizes."

def getDescriptionSecond():#Script tip : (on TOP)
	return "The 'size factor' option has a huge impact, it determines if the donut in the center can be reach by a very thin 1 tile layer at the bottom, or if boats will be necessary "

def getDescriptionThird():#Option : (at the bottom)"
	return "Perfect for 4v4, with favoring a 'non-bridge' configuration, so that the bottom and top players will have to decide to build galleys very quickly or not."	

def getDescriptionScenario():#Scenario : (at the bottom)"
	return "On 3v3 also good, will make more sense to leave a band of land in that case. Don't play other player counts"

def getDescriptionBalance():#Balance : (at the bottom)"
	return ""	

def getNumCustomMapOptions():
	return 6

def getNumHiddenCustomMapOptions():
	return 0

def getCustomMapOptionName(argsList):
	[iOption] = argsList
	option_names = {
		0:	"Size Factor",
		1:  "TXT_KEY_CONCEPT_RESOURCES",
		2:	"Types of Start",
		3:	"Center Resources",
		4:	"Start Position",
		5:	"TXT_KEY_MAP_WORLD_WRAP"
		}
	if not option_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	translated_text = unicode(CyTranslator().getText(option_names[iOption], ()))
	return translated_text

def getNumCustomMapOptionValues(argsList):
	[iOption] = argsList
	option_values = {
		0:	2,
		1:  3,
		2:	2,
		3:	2,
		4:	3,
		5:	3
		}
	if not option_values.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	return option_values[iOption]

def getCustomMapOptionDescAt(argsList):
	[iOption, iSelection] = argsList
	selection_names = {
		0:	{
			0: "Normal - Island is separated",
			1: "1 Size small - Creates little bridges"
			},
		1:	{
			0: "TXT_KEY_WORLD_STANDARD",
			1: "TXT_KEY_MAP_BALANCED",
			2: "Balanced - Including Marble"
			},
		2:	{
			0: "1 Player by Team on Island",
			1: "All Outer land"
			},			
		3:	{
			0: "Normal Resources",
			1: "Extra Resources"
		},	
		4:	{
			0: "Normal",
			1: "Top v Bottom",
			2: "Anywhere"
			},				
		5:	{
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
		0:	0,
		1:  2,
		2:	0,
		3:	1,
		4:	1,
		5:	0
		}
	if not option_defaults.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	return option_defaults[iOption]

def isRandomCustomMapOption(argsList):
	[iOption] = argsList
	option_random = {
		0:	false,
		1:	false,
		2:	false,
		3:  false,
		4:	false,
		5:	false
		}
	if not option_random.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	return option_random[iOption]

def getWrapX():
	map = CyMap()
	return (map.getCustomMapOption(5) == 1 or map.getCustomMapOption(5) == 2)	

def getWrapY():
	map = CyMap()
	return (map.getCustomMapOption(5) == 2)

def normalizeAddExtras():

	if (CyMap().getCustomMapOption(1) >= 1):
		balancer.normalizeAddExtras()

	#BTG
	# Moving from 4 to 8 because there are lots of tundras etc.
	BTPForceResourceLand(100,True,getInfoTypeOrFail("BONUS_OIL"),10,False,getInfoTypeOrFail("TERRAIN_DESERT"))
	BTPForceResourceLand(100,False,getInfoTypeOrFail("BONUS_ALUMINUM"),10,True,getInfoTypeOrFail("TERRAIN_PLAINS"))

	if (CyMap().getCustomMapOption(1) >= 2):	
		BTPForceEnrichFood(100,True,getInfoTypeOrFail("BONUS_BANANA"),5,3,False,getInfoTypeOrFail("TERRAIN_PLAINS"))	
		BTPForceEnrichFood(100,True,getInfoTypeOrFail("BONUS_PIG"),5,4,True,getInfoTypeOrFail("TERRAIN_PLAINS"))
		BTPForceEnrichFood(100,True,getInfoTypeOrFail("BONUS_WHEAT"),7,5,True,getInfoTypeOrFail("TERRAIN_PLAINS"))				

	if (CyMap().getCustomMapOption(3) >= 1):
		doUUCenter()

	#End Map Specific		

	CyPythonMgr().allowDefaultImpl()	# do the rest of the usual normalizeStartingPlots stuff, don't overrride

def doUUCenter():
	iExtraTile = 0

	#Center Unique
	# <!-- custom: AdvCiv-SAS has no BONUS_SALT/BONUS_TEA/BONUS_OLIVES; use supported luxuries instead. (GPT-5.3-Codex) -->
	BTPresourceFromCenter(1,5+iExtraTile,getInfoTypeOrFail("BONUS_SPICES"),getInfoTypeOrFail("TERRAIN_PLAINS"))

	BTPresourceFromCenter(2,6+iExtraTile,getInfoTypeOrFail("BONUS_MARBLE"),getInfoTypeOrFail("TERRAIN_PLAINS"))
	BTPresourceFromCenter(2,6+iExtraTile,getInfoTypeOrFail("BONUS_STONE"),getInfoTypeOrFail("TERRAIN_PLAINS"))

	BTPresourceFromCenter(2,7+iExtraTile,getInfoTypeOrFail("BONUS_WHALE"),getInfoTypeOrFail("TERRAIN_COAST"))

	BTPresourceFromCenter(1,5+iExtraTile,getInfoTypeOrFail("BONUS_SILVER"),getInfoTypeOrFail("TERRAIN_PLAINS"))			
	BTPresourceFromCenter(1,5+iExtraTile,getInfoTypeOrFail("BONUS_GEMSTONES"),getInfoTypeOrFail("TERRAIN_PLAINS"))			
	BTPresourceFromCenter(1,5+iExtraTile,getInfoTypeOrFail("BONUS_GOLD"),getInfoTypeOrFail("TERRAIN_PLAINS"))	

	#2.23 Elephants every two players rather than 3
	BTPresourceFromCenter(2,6+iExtraTile,getInfoTypeOrFail("BONUS_ELEPHANTS"),getInfoTypeOrFail("TERRAIN_PLAINS"))		

	BTPresourceFromCenter(2,8+iExtraTile,getInfoTypeOrFail("BONUS_GRAPES"),getInfoTypeOrFail("TERRAIN_PLAINS"))
	BTPresourceFromCenter(2,8+iExtraTile,getInfoTypeOrFail("BONUS_SILK"),getInfoTypeOrFail("TERRAIN_PLAINS"))
	BTPresourceFromCenter(2,8+iExtraTile,getInfoTypeOrFail("BONUS_SPICES"),getInfoTypeOrFail("TERRAIN_PLAINS"))
	BTPresourceFromCenter(2,8+iExtraTile,getInfoTypeOrFail("BONUS_SUGAR"),getInfoTypeOrFail("TERRAIN_PLAINS"))
	BTPresourceFromCenter(2,8+iExtraTile,getInfoTypeOrFail("BONUS_DYE"),getInfoTypeOrFail("TERRAIN_PLAINS"))		
	BTPresourceFromCenter(2,8+iExtraTile,getInfoTypeOrFail("BONUS_INCENSE"),getInfoTypeOrFail("TERRAIN_DESERT"))

	BTPresourceFromCenter(2,6+iExtraTile,getInfoTypeOrFail("BONUS_DEER"),getInfoTypeOrFail("TERRAIN_TUNDRA"))				
	BTPresourceFromCenter(2,6+iExtraTile,getInfoTypeOrFail("BONUS_FUR"),getInfoTypeOrFail("TERRAIN_TUNDRA"))						

def addBonusType(argsList):

	[iBonusType] = argsList
	gc = CyGlobalContext()
	type_string = gc.getBonusInfo(iBonusType).getType()

	if (CyMap().getCustomMapOption(1) >= 1):
		if (type_string in balancer.resourcesToBalance) or (type_string in balancer.resourcesToEliminate):
			return None # don't place any of this bonus randomly

	CyPythonMgr().allowDefaultImpl() # pretend we didn't implement this method, and let C handle this bonus in the default way

def isAdvancedMap():
	"This map should not show up in simple mode"
	# <!-- custom: keep this at 0 so BTG_Lagoon appears in Simple Game mode map lists; return 1 hides it from simple mode. (GPT-5.3-Codex (summarized)) -->
	return 0

def isClimateMap():
	return 0

def isSeaLevelMap():
	return 0

def getGridSize(argsList):
	"Override Grid Size function to make the maps square."

	# <!-- custom: Use DLL WorldSizeTypes values aligned with CIV4WorldInfo.xml order. (Claude code Opus 4.7; GPT-5.5) -->
	grid_sizes = {
		WorldSizeTypes.WORLDSIZE_ARENA: (5,5),
		WorldSizeTypes.WORLDSIZE_DUEL: (6,6),
		WorldSizeTypes.WORLDSIZE_TINY: (7,7),
		WorldSizeTypes.WORLDSIZE_SMALL: (8,8),
		WorldSizeTypes.WORLDSIZE_STANDARD: (9,9),
		WorldSizeTypes.WORLDSIZE_LARGE: (10,10),
		WorldSizeTypes.WORLDSIZE_HUGE: (11,11)
	}			

	if (argsList[0] == -1): # (-1,) is passed to function on loads
		return []
	[eWorldSize] = argsList
	return sas_lookup_world_size_with_calibrated_sas(eWorldSize, grid_sizes)

	#return (10, 10) 

def minStartingDistanceModifier():
	return -12

class DonutFractalWorld(CvMapGeneratorUtil.FractalWorld):
	# <!-- custom: Wrapped Lagoon geography must stay at its handcrafted coordinates, so this override no longer accepts the inherited shift-plot-types switch. See KI#270. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	def generatePlotTypes(self, water_percent=78, grain_amount=3):
		self.hillsFrac.fracInit(self.iNumPlotsX, self.iNumPlotsY, grain_amount, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
		self.peaksFrac.fracInit(self.iNumPlotsX, self.iNumPlotsY, grain_amount+1, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)

		iHillsBottom1 = self.hillsFrac.getHeightFromPercent(max((self.hillGroupOneBase - self.hillGroupOneRange), 0))
		iHillsTop1 = self.hillsFrac.getHeightFromPercent(min((self.hillGroupOneBase + self.hillGroupOneRange), 100))
		iHillsBottom2 = self.hillsFrac.getHeightFromPercent(max((self.hillGroupTwoBase - self.hillGroupTwoRange), 0))
		iHillsTop2 = self.hillsFrac.getHeightFromPercent(min((self.hillGroupTwoBase + self.hillGroupTwoRange), 100))
		iPeakThreshold = self.peaksFrac.getHeightFromPercent(self.peakPercent)

		iCenterX = int(self.iNumPlotsX / 2)
		iCenterY = int(self.iNumPlotsY / 2)

		iRadius = int(self.map.getGridHeight() / 4)
		iHoleRadius = int(self.map.getGridHeight() / 4)
		# <!-- custom: The BTG formula used raw pre-Arena world-size indices, making every Tiny-Huge land band one tier too wide and drifting farther on SAS sizes.
		# Preserve the original BTS widths through an explicit aligned Arena-SAS48 table. See KI#269. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		worldSizeWidths = {
			WorldSizeTypes.WORLDSIZE_ARENA: (4, 3),
			WorldSizeTypes.WORLDSIZE_DUEL: (4, 3),
			WorldSizeTypes.WORLDSIZE_TINY: (4, 3),
			WorldSizeTypes.WORLDSIZE_SMALL: (5, 4),
			WorldSizeTypes.WORLDSIZE_STANDARD: (6, 4),
			WorldSizeTypes.WORLDSIZE_LARGE: (7, 5),
			WorldSizeTypes.WORLDSIZE_HUGE: (8, 6),
			WorldSizeTypes.WORLDSIZE_SAS24: (9, 7),
			WorldSizeTypes.WORLDSIZE_SAS32: (10, 8),
			WorldSizeTypes.WORLDSIZE_SAS40: (11, 9),
			WorldSizeTypes.WORLDSIZE_SAS48: (12, 10),
		}
		(iBaseWidthSize, iBaseSmallWidthSize) = worldSizeWidths[self.map.getWorldSize()]
		iBridgeWidth = self.map.getCustomMapOption(0)
		iWidthSize = iBaseWidthSize + iBridgeWidth
		iSmallWidthSize = iBaseSmallWidthSize + iBridgeWidth

		for x in range(self.iNumPlotsX):
			for y in range(self.iNumPlotsY):
				i = y*self.iNumPlotsX + x
				if x == iCenterX and y == iCenterY:
					fDistance = 0
				else:
					fDistance = sqrt(((x - iCenterX) ** 2) + ((y - iCenterY) ** 2))
				if fDistance > iRadius:#outside
					self.plotTypes[i] = PlotTypes.PLOT_OCEAN
				elif fDistance < iHoleRadius:#inside

					if fDistance == iHoleRadius - 1:#Specific Lagoon map, makes edge blurry and boatable
						iProba = CyGlobalContext().getGame().getMapRandNum(100,"iProba")						
						if (iProba < 60):
							self.plotTypes[i] = PlotTypes.PLOT_LAND					
					elif fDistance == iHoleRadius - 2:#Specific Lagoon map, makes edge blurry and boatable
						iProba = CyGlobalContext().getGame().getMapRandNum(100,"iProba")						
						if (iProba < 80):
							self.plotTypes[i] = PlotTypes.PLOT_LAND						
					else:#that's the normal one
						iProba = CyGlobalContext().getGame().getMapRandNum(100,"iProba")
						if (iProba < 18):
							self.plotTypes[i] = PlotTypes.PLOT_HILLS
						else :
							self.plotTypes[i] = PlotTypes.PLOT_LAND			

				else:
					hillVal = self.hillsFrac.getHeight(x,y)
					if ((hillVal >= iHillsBottom1 and hillVal <= iHillsTop1) or (hillVal >= iHillsBottom2 and hillVal <= iHillsTop2)):
						peakVal = self.peaksFrac.getHeight(x,y)
						if (peakVal <= iPeakThreshold):
							self.plotTypes[i] = PlotTypes.PLOT_PEAK
						else:
							self.plotTypes[i] = PlotTypes.PLOT_HILLS
					else:
						self.plotTypes[i] = PlotTypes.PLOT_LAND

		for x in range(self.iNumPlotsX):
			for y in range(self.iNumPlotsY):
				i = y*self.iNumPlotsX + x	

				#A - Extreme sides solid
				if x <= iSmallWidthSize or x >= self.map.getGridWidth() - iSmallWidthSize or y <= iSmallWidthSize or y >= self.map.getGridWidth() - iSmallWidthSize:
					iProba = CyGlobalContext().getGame().getMapRandNum(100,"iProba")
					if (iProba < 18):
						self.plotTypes[i] = PlotTypes.PLOT_HILLS
					else :
						self.plotTypes[i] = PlotTypes.PLOT_LAND

				#B Next to that, less solid
				if x <= iWidthSize or x >= self.map.getGridWidth() - iWidthSize or y <= iWidthSize or y >= self.map.getGridWidth() - iWidthSize:
					iProba = CyGlobalContext().getGame().getMapRandNum(100,"iProba")
					if (iProba < 40):
						iSecondProba = CyGlobalContext().getGame().getMapRandNum(100,"iProba")
						if (iSecondProba < 18):
							self.plotTypes[i] = PlotTypes.PLOT_HILLS
						else :
							self.plotTypes[i] = PlotTypes.PLOT_LAND

				#C make a bridge for the lagoon
				if (CyMap().getCustomMapOption(0) == 0): #This option is worth 1 if bridge activated
					if x >= iCenterX -1 and x <= iCenterX +1:				
						if ((y >= iWidthSize and y <= iWidthSize + 1) or (y >= self.map.getGridHeight() - iWidthSize - 1 and y <= self.map.getGridHeight() - iWidthSize)):
							iProba = CyGlobalContext().getGame().getMapRandNum(100,"iProba")
							if (iProba < 40):
								self.plotTypes[i] = PlotTypes.PLOT_HILLS
				if (CyMap().getCustomMapOption(0) == 1): #this is WITH the bridge, I want to make sure it's there
					if x >= iCenterX - 2 and x <= iCenterX +2:				
						if ((y >= iWidthSize -1 and y <= iWidthSize + 1) or (y >= self.map.getGridHeight() - iWidthSize - 1 and y <= self.map.getGridHeight() - iWidthSize + 1)):
							iProba = CyGlobalContext().getGame().getMapRandNum(100,"iProba")
							if (iProba < 95):
								self.plotTypes[i] = PlotTypes.PLOT_HILLS					

		# <!-- custom: BTG shifted the handcrafted Lagoon plot array on wrapped maps but kept starting regions, center terrain/rivers and resources at unshifted coordinates.
		# Keeping the designed array fixed makes every downstream coordinate refer to the same geography. See KI#270. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->

		return self.plotTypes

def generatePlotTypes():
	NiTextOut("Setting Plot Types (Python Donut) ...")
	fractal_world = DonutFractalWorld()
	return fractal_world.generatePlotTypes()

# subclass TerrainGenerator to create a lush grassland utopia.
class DonutTerrainGenerator(CvMapGeneratorUtil.TerrainGenerator):
	def __init__(self, fracXExp=-1, fracYExp=-1, grain_amount=5):
		self.gc = CyGlobalContext()
		self.map = CyMap()

		self.grain_amount = grain_amount + self.gc.getWorldInfo(self.map.getWorldSize()).getTerrainGrainChange()

		self.iWidth = self.map.getGridWidth()
		self.iHeight = self.map.getGridHeight()

		self.mapRand = self.gc.getGame().getMapRand()

		self.iFlags = 0  # Disallow FRAC_POLAR flag, to prevent "zero row" problems.

		self.terrain=CyFractal()

		self.fracXExp = fracXExp
		self.fracYExp = fracYExp

		self.initFractals()

		self.iCenterX = int(self.map.getGridWidth() / 2)
		self.iCenterY = int(self.map.getGridHeight() / 2)

		self.iRadius = int(self.map.getGridHeight() / 2)
		self.iHoleRadius = int(self.map.getGridHeight() / 2)

	def initFractals(self):
		self.terrain.fracInit(self.iWidth, self.iHeight, self.grain_amount, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
		self.iGrassBottom = self.terrain.getHeightFromPercent(12)

		self.terrainPlains = getInfoTypeOrFail("TERRAIN_PLAINS")
		self.terrainGrass = getInfoTypeOrFail("TERRAIN_GRASS")
		self.terrainDesert = getInfoTypeOrFail("TERRAIN_DESERT")

	def getLatitudeAtPlot(self, iX, iY):
		return None

	def generateTerrain(self):		
		terrainData = [0]*(self.iWidth*self.iHeight)
		for x in range(self.iWidth):
			for y in range(self.iHeight):
				iI = y*self.iWidth + x
				terrain = self.generateTerrainAtPlot(x, y)
				terrainData[iI] = terrain
		return terrainData

	def generateTerrainAtPlot(self,iX,iY):
		if (self.map.plot(iX, iY).isWater()):
			return self.map.plot(iX, iY).getTerrainType()

		#version B - Simplified
		val = self.terrain.getHeight(iX, iY)
		iProba = CyGlobalContext().getGame().getMapRandNum(100,"iProba")
		if iProba <= 3:
			terrainVal = getInfoTypeOrFail("TERRAIN_DESERT")
		elif iProba <= 30:
			terrainVal = getInfoTypeOrFail("TERRAIN_PLAINS")				
		else:#then normal
			if val >= self.iGrassBottom:
				terrainVal = self.terrainGrass
			else:
				terrainVal = self.terrainPlains	

		if (terrainVal == TerrainTypes.NO_TERRAIN):
			return self.map.plot(iX, iY).getTerrainType()

		return terrainVal

def addRivers():#Lagoon 2.26 -- Adding this because it's direct after Generate plot and before the bonuses

	#Polish the Terrain Type
	for x in range(CyMap().getGridWidth()):
		for y in range(CyMap().getGridHeight()):
			if (x == 0 or y == 0 or x == CyMap().getGridWidth()-1 or y == CyMap().getGridHeight()-1):
				if not (CyMap().getCustomMapOption(5) == 1 or CyMap().getCustomMapOption(5) == 2):#it's not nice when there is cylindrical wrap
					p = CyMap().plot(x,y)
					p.setTerrainType(getInfoTypeOrFail("TERRAIN_TUNDRA"), True, True)
			if (x >= CyMap().getGridWidth()/2 -3 and x <= CyMap().getGridWidth()/2 +2 and y >= CyMap().getGridHeight()/2 -1 and y <= CyMap().getGridHeight()/2):
				p = CyMap().plot(x,y)
				iProba = CyGlobalContext().getGame().getMapRandNum(100,"iProba")							
				if (iProba < 75):
					p.setPlotType(PlotTypes.PLOT_PEAK,True,True)
				else:
					p.setTerrainType(getInfoTypeOrFail("TERRAIN_DESERT"), True, True)	

	#Need to finish by doing normal rivers
	CyPythonMgr().allowDefaultImpl()				

def generateTerrainTypes():
	NiTextOut("Generating Terrain (Python Donut) ...")
	terraingen = DonutTerrainGenerator()
	terrainTypes = terraingen.generateTerrain()
	return terrainTypes

class DonutFeatureGenerator(CvMapGeneratorUtil.FeatureGenerator):
	def addIceAtPlot(self, pPlot, iX, iY, lat):
		# We don' need no steeking ice. M'kay? Alrighty then.
		ice = 0

	def addJunglesAtPlot(self, pPlot, iX, iY, lat):
		jungle = 0	

def addFeatures():
	NiTextOut("Adding Features (Python Donut) ...")
	featuregen = DonutFeatureGenerator()
	featuregen.addFeatures()
	return 0

def beforeGeneration():
	#copy /inspired by inland			
	"Set up global variables for start point templates"
	global templates
	# <!-- custom: Store shuffled template assignments by actual player ID because alive slots need not be compact. See KI#267. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	global playerTemplateByID
	global iTemplateRoll
	gc = CyGlobalContext()
	dice = gc.getGame().getMapRand()
	iW = CyMap().getGridWidth()
	iH = CyMap().getGridHeight()

	# Choose a Template to be used for this game.
	iPlayers = gc.getGame().countCivPlayersEverAlive()
	iTemplateRoll = 0#Because only 1 template for each

	#2.23 - Debug because it crashes if too close

	if (CyMap().getCustomMapOption(2) == 0):#Normal with people inside

		fVar = 2

		templates = {(1,0): {0: [0.50, 0.50, 4, 4]},
                     (2,0): {0: [0.50, 0.10, fVar, fVar],
                             1: [0.50, 0.90, fVar, fVar]},
                     (3,0): {0: [0.50, 0.10, fVar, fVar],
                             1: [0.50, 0.30, fVar, fVar],
                             2: [0.50, 0.90, fVar, fVar]},
                     (4,0): {0: [0.50, 0.30, fVar, fVar],
                             1: [0.50, 0.70, fVar, fVar],
                             2: [0.50, 0.10, fVar, fVar],
                             3: [0.50, 0.90, fVar, fVar]},
                     (5,0): {0: [0.50, 0.30, fVar, fVar],
                             1: [0.50, 0.70, fVar, fVar],
                             2: [0.50, 0.10, fVar, fVar],
                             3: [0.50, 0.90, fVar, fVar],
                             4: [0.10, 0.25, fVar, fVar]},
                     (6,0): {0: [0.10, 0.25, fVar, fVar],
                             1: [0.10, 0.75, fVar, fVar],
                             2: [0.90, 0.25, fVar, fVar],
                             3: [0.90, 0.75, fVar, fVar],
                             4: [0.50, 0.30, fVar, fVar],
                             5: [0.50, 0.70, fVar, fVar]},
                     (7,0): {0: [0.10, 0.25, fVar, fVar],
                             1: [0.10, 0.75, fVar, fVar],
                             2: [0.90, 0.25, fVar, fVar],
                             3: [0.90, 0.75, fVar, fVar],
                             4: [0.50, 0.30, fVar, fVar],
                             5: [0.50, 0.70, fVar, fVar],
                             6: [0.50, 0.10, fVar, fVar]},
                     (8,0): {0: [0.10, 0.25, fVar, fVar],
                             1: [0.10, 0.75, fVar, fVar],
                             2: [0.90, 0.25, fVar, fVar],
                             3: [0.90, 0.75, fVar, fVar],
                             4: [0.50, 0.30, fVar, fVar],
                             5: [0.50, 0.70, fVar, fVar],
                             6: [0.50, 0.10, fVar, fVar],
                             7: [0.50, 0.90, fVar, fVar]},
                     (9,0): {0: [0.33, 0.20, fVar, fVar],
                             1: [0.15, 0.35, fVar, fVar],
                             2: [0.50, 0.75, fVar, fVar],
                             3: [0.66, 0.20, fVar, fVar],
                             4: [0.85, 0.35, fVar, fVar],
                             5: [0.15, 0.65, fVar, fVar],
                             6: [0.85, 0.65, fVar, fVar],
                             7: [0.10, 0.10, fVar, fVar],
                             8: [0.90, 0.10, fVar, fVar]},
                     (10,0): {0: [0.10, 0.25, fVar, fVar],
                             1: [0.10, 0.75, fVar, fVar],
                             2: [0.90, 0.25, fVar, fVar],
                             3: [0.90, 0.75, fVar, fVar],
                             4: [0.50, 0.30, fVar, fVar],
                             5: [0.50, 0.70, fVar, fVar],
                             6: [0.30, 0.10, fVar, fVar],
                             7: [0.30, 0.90, fVar, fVar],
                             8: [0.70, 0.10, fVar, fVar],
                             9: [0.70, 0.90, fVar, fVar]},
		}

	if (CyMap().getCustomMapOption(2) == 1):#Only outside

		fVar = 2

		templates = {(1,0): {0: [0.50, 0.50, 4, 4]},
                     (2,0): {0: [0.10, 0.10, fVar, fVar],
                             1: [0.10, 0.90, fVar, fVar]},
                     (3,0): {0: [0.10, 0.10, fVar, fVar],
                             1: [0.90, 0.10, fVar, fVar],
                             2: [0.10, 0.90, fVar, fVar]},
                     (4,0): {0: [0.10, 0.90, fVar, fVar],
                             1: [0.90, 0.10, fVar, fVar],
                             2: [0.10, 0.10, fVar, fVar],
                             3: [0.90, 0.90, fVar, fVar]},
                     (5,0): {0: [0.10, 0.90, fVar, fVar],
                             1: [0.90, 0.10, fVar, fVar],
                             2: [0.10, 0.10, fVar, fVar],
                             3: [0.90, 0.90, fVar, fVar],
                             4: [0.50, 0.50, fVar, fVar]},
                     (6,0): {0: [0.10, 0.25, fVar, fVar],
                             1: [0.10, 0.75, fVar, fVar],
                             2: [0.90, 0.25, fVar, fVar],
                             3: [0.90, 0.75, fVar, fVar],
                             4: [0.50, 0.10, fVar, fVar],
                             5: [0.50, 0.90, fVar, fVar]},
                     (7,0): {0: [0.10, 0.25, fVar, fVar],
                             1: [0.10, 0.75, fVar, fVar],
                             2: [0.90, 0.25, fVar, fVar],
                             3: [0.90, 0.75, fVar, fVar],
                             4: [0.70, 0.10, fVar, fVar],
                             5: [0.70, 0.90, fVar, fVar],
                             6: [0.30, 0.10, fVar, fVar],
                             7: [0.30, 0.90, fVar, fVar],
                             8: [0.50, 0.50, fVar, fVar]},
                     (8,0): {0: [0.10, 0.25, fVar, fVar],
                             1: [0.10, 0.75, fVar, fVar],
                             2: [0.90, 0.25, fVar, fVar],
                             3: [0.90, 0.75, fVar, fVar],
                             4: [0.70, 0.10, fVar, fVar],
                             5: [0.70, 0.90, fVar, fVar],
                             6: [0.30, 0.10, fVar, fVar],
                             7: [0.30, 0.90, fVar, fVar]},
                     (9,0): {0: [0.10, 0.25, fVar, fVar],
                             1: [0.10, 0.75, fVar, fVar],
                             2: [0.90, 0.25, fVar, fVar],
                             3: [0.90, 0.75, fVar, fVar],
                             4: [0.75, 0.10, fVar, fVar],
                             5: [0.75, 0.90, fVar, fVar],
                             6: [0.25, 0.10, fVar, fVar],
                             7: [0.25, 0.90, fVar, fVar],
                             8: [0.50, 0.50, fVar, fVar]},
                     (10,0): {0: [0.10, 0.25, fVar, fVar],
                             1: [0.10, 0.75, fVar, fVar],
                             2: [0.90, 0.25, fVar, fVar],
                             3: [0.90, 0.75, fVar, fVar],
                             4: [0.75, 0.10, fVar, fVar],
                             5: [0.75, 0.90, fVar, fVar],
                             6: [0.25, 0.10, fVar, fVar],
                             7: [0.25, 0.90, fVar, fVar],
                             8: [0.50, 0.10, fVar, fVar],
                             9: [0.50, 0.90, fVar, fVar]},
		}
	# End of Templates data.

	# Shuffle start points so that players are assigned templateIDs at random.
	# <!-- custom: BTG indexed a compact assignment list with real player IDs, so sparse alive slots lost Lagoon's custom starting region.
	# Map every actual ever-alive civilization player ID directly to one shuffled template ID. See KI#267. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	playerIDs = []
	for playerID in range(gc.getMAX_CIV_PLAYERS()):
		if gc.getPlayer(playerID).isEverAlive():
			playerIDs.append(playerID)
	templateIDs = range(iPlayers)
	playerTemplateByID = {}
	for playerID in playerIDs:
		iChooseTemplate = dice.get(len(templateIDs), "Shuffling Template IDs - Inland Sea PYTHON")
		playerTemplateByID[playerID] = templateIDs[iChooseTemplate]
		del templateIDs[iChooseTemplate]

	return None		

def findStartingPlot(argsList):
	# Set up for maximum of 18 players! If more, use default implementation.
	iPlayers = CyGlobalContext().getGame().countCivPlayersEverAlive()
	if iPlayers > 10:
		CyPythonMgr().allowDefaultImpl()
		return

	[playerID] = argsList

	global plotSuccess
	global plotValue

	def isValid(playerID, x, y):
		gc = CyGlobalContext()
		map = CyMap()
		pPlot = map.plot(x, y)
		iW = map.getGridWidth()
		iH = map.getGridHeight()
		iPlayers = gc.getGame().countCivPlayersEverAlive()

		# Use global data set up via beforeGeneration().
		global templates
		global playerTemplateByID
		global iTemplateRoll
		playerTemplateAssignment = playerTemplateByID[playerID]
		[fLat, fLon, varX, varY] = templates[(iPlayers, iTemplateRoll)][playerTemplateAssignment]

		# Check to ensure the plot is on the main landmass.
		#2.26 Lagoon - This makes it crash !
		#if (pPlot.getArea() != map.findBiggestArea(False).getID()):
		#	return false

		# Now check for eligibility according to the defintions found in the template.
		iX = int(iW * fLat)
		iY = int(iH * fLon)
		westX = max(2, iX - varX)
		eastX = min(iW - 3, iX + varX)
		southY = max(2, iY - varY)
		northY = min(iH - 3, iY + varY)
		if x < westX or x > eastX or y < southY or y > northY:
			return false
		else:
			return true

	return CvMapGeneratorUtil.findStartingPlot(playerID, isValid)#2.22 Simplified this part by calling common logic	

def normalizeStartingPlotLocations():

	gc = CyGlobalContext()	
	dice = gc.getGame().getMapRand()	

	if (CyMap().getCustomMapOption(4) == 1):
		BTPTopBottomTwoTeams(False)
	elif (CyMap().getCustomMapOption(4) == 2):
		return
	else:
		CyPythonMgr().allowDefaultImpl()#this is the bit that puts team together and is normal case	

def startHumansOnSameTile():

	#doing in normalizeAddExtra was too early
	#we do this after because default implement does add forest
	CyPythonMgr().allowDefaultImpl()

def BTGSong():
	return 0

def BTPTopBottomTwoTeams(isBTG):							
	gc = CyGlobalContext()	

	#2.19 debug		
	random.seed(gc.getGame().getMapRand().get(30000, "Shuffle Plots - PYTHON"))	

	iEverAliveTeamCount = 0

	for iI in range(gc.getMAX_CIV_TEAMS()):	
		if isBTG:
			if gc.getTeam(iI).isEverAlive() and not gc.getTeam(iI).isSpectator() and not gc.getTeam(iI).isBarbarian():
				iEverAliveTeamCount += 1
		else:
			if gc.getTeam(iI).isEverAlive() and not gc.getTeam(iI).isBarbarian():
				iEverAliveTeamCount += 1			

	if gc.getGame().countCivPlayersEverAlive() <= 3:
		return None			

	if (gc.getGame().countCivPlayersEverAlive() % 2) != 0 :#verify it's even number, if not don't apply
		return None	

	elif not iEverAliveTeamCount == 2:
		return None

	else:

		#############################
		#Spectator bit - not amazing if spectator is middle team

		listTeams = []
		#2.23 Improve for spectators
		for iI in range(gc.getMAX_CIV_TEAMS()):	
			if isBTG:
				if not gc.getTeam(iI).isSpectator():
					if gc.getTeam(iI).isEverAlive():				
						listTeams.append(gc.getTeam(iI).getID())
			else:
				if gc.getTeam(iI).isEverAlive():				
					listTeams.append(gc.getTeam(iI).getID())				

		random.shuffle(listTeams)		
		teamOne = listTeams[0]
		teamTwo = listTeams[1]
		###########################

		listPlayer = []
		iH = CyMap().getGridHeight()
		halfHeight = iH / 2
		for iI in range(gc.getMAX_CIV_PLAYERS()):
			if isBTG:
				if (gc.getPlayer(iI).isAlive() and not gc.getPlayer(iI).isSpectator()):		
					listPlayer.append(gc.getPlayer(iI).getID())
			else:
				if (gc.getPlayer(iI).isAlive()):		
					listPlayer.append(gc.getPlayer(iI).getID())				

		# <!-- custom: BTG retried random candidates without a bound, hanging when unequal teams made full Top-v-Bottom separation impossible.
		# Require equal teams and pair the finite misplaced-player lists; if the current starts cannot be paired completely, leave normalization unchanged. See KI#268. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		teamOnePlayers = []
		teamTwoPlayers = []
		for playerID in listPlayer:
			if gc.getPlayer(playerID).getTeam() == teamOne:
				teamOnePlayers.append(playerID)
			elif gc.getPlayer(playerID).getTeam() == teamTwo:
				teamTwoPlayers.append(playerID)
		if len(teamOnePlayers) != len(teamTwoPlayers):
			return None

		teamOneTop = []
		teamTwoBottom = []
		for playerID in teamOnePlayers:
			if gc.getPlayer(playerID).getStartingPlot().getY() >= halfHeight:
				teamOneTop.append(playerID)
		for playerID in teamTwoPlayers:
			if gc.getPlayer(playerID).getStartingPlot().getY() < halfHeight:
				teamTwoBottom.append(playerID)
		if len(teamOneTop) != len(teamTwoBottom):
			return None
		random.shuffle(teamOneTop)
		random.shuffle(teamTwoBottom)
		for swapIndex in range(len(teamOneTop)):
			playerA = teamOneTop[swapIndex]
			playerB = teamTwoBottom[swapIndex]
			spotA = gc.getPlayer(playerA).getStartingPlot()
			spotB = gc.getPlayer(playerB).getStartingPlot()
			gc.getPlayer(playerA).setStartingPlot(spotB,True)
			gc.getPlayer(playerB).setStartingPlot(spotA,True)

def BTPForceResourceLand(iProbaTreshold,bMainLandOnly,iResourceType,iDistance,bMakeHill,iForceTerrain):

	gc = CyGlobalContext()
	map = CyMap()
	random.seed(gc.getGame().getMapRand().get(30000, "Shuffle Plots - PYTHON"))

	for i in range(gc.getMAX_CIV_PLAYERS()):

		iProba = CyGlobalContext().getGame().getMapRandNum(100,"iProba")
		if iProba <= iProbaTreshold:

			if gc.getPlayer(i).isEverAlive():
				start_plot = gc.getPlayer(i).getStartingPlot()
				startx, starty = start_plot.getX(), start_plot.getY()
				plotsboundaries = []
				plotsboundariesSafe = []
				plotsboundariesSafeNoRiver = []
				has_resource = false
				for dx in range(-iDistance,iDistance):
					for dy in range(-iDistance,iDistance):
						p = map.plot(startx+dx,starty+dy)
						#if ((dx != 0) or (dy != 0)) and (not p.isNone()) and (not p.isImpassable()) and (not p.isWater()):
						if (bMainLandOnly == True) :
							if ((dx != 0) or (dy != 0)) and (not p.isNone()) and (not p.isImpassable()) and (not p.isWater()) and p.getArea() == CyMap().findBiggestArea(False).getID():	
								iBonusCount = 0
								for tx in range(3):
									for ty in range(3):
										testP = CyMap().plot(startx+dx+tx-1,starty+dy+ty-1)
										if (testP.getBonusType(-1) != -1):
											iBonusCount += 1		
								if iBonusCount >= 1:
									plotsboundaries.append(p)
								elif not p.isRiver():
									plotsboundariesSafeNoRiver.append(p)
								else :
									plotsboundariesSafe.append(p)#all the tiles are no bonus, this has priority
								if p.getBonusType(-1) == iResourceType:
									has_resource = True
						if (bMainLandOnly == False) :								
							if ((dx != 0) or (dy != 0)) and (not p.isNone()) and (not p.isImpassable()) and (not p.isWater()):			
								iBonusCount = 0
								for tx in range(3):
									for ty in range(3):
										testP = CyMap().plot(startx+dx+tx-1,starty+dy+ty-1)
										if (testP.getBonusType(-1) != -1):
											iBonusCount += 1		
								if iBonusCount >= 1:
									plotsboundaries.append(p)
								elif not p.isRiver():
									plotsboundariesSafeNoRiver.append(p)										
								else :
									plotsboundariesSafe.append(p)#all the tiles are no bonus, this has priority
								if p.getBonusType(-1) == iResourceType:
									has_resource = True

				if not has_resource:
					if len(plotsboundariesSafeNoRiver) > 0:	#2.34 new block									
						random.shuffle(plotsboundariesSafeNoRiver)	
						for p in plotsboundariesSafeNoRiver:
							if (p.getBonusType(-1) == BonusTypes.NO_BONUS):
								if bMakeHill:
									p.setPlotType(PlotTypes.PLOT_HILLS, True, True)						
								else:
									p.setPlotType(PlotTypes.PLOT_LAND, True, True)
								p.setTerrainType(iForceTerrain, True, True)
								p.setBonusType(iResourceType)
								p.setFeatureType(-1,-1)#2.34 avoid resource on floodplains transformed into 5F
								has_resource = True
								break						

					elif len(plotsboundariesSafe) > 0:										
						random.shuffle(plotsboundariesSafe)	
						for p in plotsboundariesSafe:
							if (p.getBonusType(-1) == BonusTypes.NO_BONUS):
								if bMakeHill:
									p.setPlotType(PlotTypes.PLOT_HILLS, True, True)						
								else:
									p.setPlotType(PlotTypes.PLOT_LAND, True, True)
								p.setTerrainType(iForceTerrain, True, True)
								p.setBonusType(iResourceType)
								p.setFeatureType(-1,-1)#2.34 avoid resource on floodplains transformed into 5F
								has_resource = True
								break

					else:								
						random.shuffle(plotsboundaries)	
						for p in plotsboundaries:
							if (p.getBonusType(-1) == BonusTypes.NO_BONUS):
								if bMakeHill:
									p.setPlotType(PlotTypes.PLOT_HILLS, True, True)						
								else:
									p.setPlotType(PlotTypes.PLOT_LAND, True, True)
								p.setTerrainType(iForceTerrain, True, True)
								p.setBonusType(iResourceType)
								p.setFeatureType(-1,-1)#2.34 avoid resource on floodplains transformed into 5F
								has_resource = True
								break				

def BTPForceEnrichFood(iProbaTreshold,bMainLandOnly,iResourceType,iMaxDistance,iMinDistance,bMakeHill,iForceTerrain):		

	gc = CyGlobalContext()
	map = CyMap()
	random.seed(gc.getGame().getMapRand().get(30000, "Shuffle Plots - PYTHON"))

	for i in range(gc.getMAX_CIV_PLAYERS()):

		iProba = CyGlobalContext().getGame().getMapRandNum(100,"iProba")
		if iProba <= iProbaTreshold:

			if gc.getPlayer(i).isEverAlive():
				start_plot = gc.getPlayer(i).getStartingPlot()
				startx, starty = start_plot.getX(), start_plot.getY()
				plotsboundaries = []
				plotsboundariesSafe = []
				for dx in range(-iMaxDistance,iMaxDistance):
					for dy in range(-iMaxDistance,iMaxDistance):
						p = map.plot(startx+dx,starty+dy)
						if (bMainLandOnly == True) :
							if ((dx != 0) or (dy != 0)) and (not p.isImpassable()) and (not p.isWater()) and p.getArea() == CyMap().findBiggestArea(False).getID():				
								if ((abs(dx) >= iMinDistance) and (abs(dy) >= iMinDistance)):
									iBonusCount = 0
									for tx in range(3):
										for ty in range(3):
											testP = CyMap().plot(startx+dx+tx-1,starty+dy+ty-1)
											if (testP.getBonusType(-1) != -1):
												iBonusCount += 1		
									if iBonusCount >= 1:
										plotsboundaries.append(p)
									else :
										plotsboundariesSafe.append(p)#all the tiles are no bonus, this has priority

						if (bMainLandOnly == False) :
							if ((dx != 0) or (dy != 0)) and (not p.isImpassable()) and (not p.isWater()):				
								if ((abs(dx) >= iMinDistance) and (abs(dy) >= iMinDistance)):
									iBonusCount = 0
									for tx in range(3):
										for ty in range(3):
											testP = CyMap().plot(startx+dx+tx-1,starty+dy+ty-1)
											if (testP.getBonusType(-1) != -1):
												iBonusCount += 1		
									if iBonusCount >= 1:
										plotsboundaries.append(p)
									else :
										plotsboundariesSafe.append(p)#all the tiles are no bonus, this has priority								

				if len(plotsboundariesSafe) > 0:

					random.shuffle(plotsboundariesSafe)	
					for p in plotsboundariesSafe:
						#if (p.getBonusType(-1) == BonusTypes.NO_BONUS) and p.canHaveBonus(iResourceType, True):
						if (p.getBonusType(-1) == BonusTypes.NO_BONUS):#I don't care, I make my own plot anyway
							if bMakeHill:
								p.setPlotType(PlotTypes.PLOT_HILLS, True, True)						
							else:
								p.setPlotType(PlotTypes.PLOT_LAND, True, True)
							p.setTerrainType(iForceTerrain, True, True)#I Like to see it in this case too					
							p.setBonusType(iResourceType)
							p.setFeatureType(-1, -1)#2.25 -- Need remove floodplains, and forest then	
							break

				else:	
					random.shuffle(plotsboundaries)
					for p in plotsboundaries:
						#if (p.getBonusType(-1) == BonusTypes.NO_BONUS) and p.canHaveBonus(iResourceType, True):
						if (p.getBonusType(-1) == BonusTypes.NO_BONUS):#I don't care, I make my own plot anyway
							if bMakeHill:
								p.setPlotType(PlotTypes.PLOT_HILLS, True, True)						
							else:
								p.setPlotType(PlotTypes.PLOT_LAND, True, True)
							p.setTerrainType(iForceTerrain, True, True)#I Like to see it in this case too					
							p.setBonusType(iResourceType)
							p.setFeatureType(-1, -1)#2.25 -- Need remove floodplains, and forest then	
							break		

def BTPresourceFromCenter(minFromCenter,maxFromCenter,iResourceType,iTerrainType):

	centerX = CyMap().getGridWidth()*50/100
	centerY = CyMap().getGridHeight()*50/100
	plotCenter = CyMap().plot(CyMap().getGridWidth()*50/100,CyMap().getGridHeight()*50/100)
	random.seed(CyGlobalContext().getGame().getMapRand().get(30000, "Shuffle Plots - PYTHON"))

	plotsboundaries = []
	plotsboundariesSafe = []
	for dx in range(-maxFromCenter,maxFromCenter):
		for dy in range(-maxFromCenter,maxFromCenter):
			p = CyMap().plot(centerX+dx,centerY+dy)
			#if (((dx >= minFromCenter) or (dx <= -minFromCenter)) and ((dy >= minFromCenter) or (dy <=-minFromCenter))): 
			if ((abs(dx) >= minFromCenter) and (abs(dy) >= minFromCenter)):
				if (not p.isNone()) and (not p.isImpassable()) and (not p.isWater()):

					#2.21
					iBonusCount = 0
					for tx in range(3):
						for ty in range(3):
							testP = CyMap().plot(centerX+dx+tx-1,centerY+dy+ty-1)
							if (testP.getBonusType(-1) != -1):
								iBonusCount += 1		

					if iBonusCount >= 1:
						plotsboundaries.append(p)
					else :
						plotsboundariesSafe.append(p)#all the tiles are no bonus, this has priority

	if len(plotsboundariesSafe) > 0:

		random.shuffle(plotsboundariesSafe)	
		for p in plotsboundariesSafe:
			if (p.getBonusType(-1) == BonusTypes.NO_BONUS):
				p.setTerrainType(iTerrainType,True,True)
				#p.setTerrainType(getInfoTypeOrFail("TERRAIN_DESERT"),True,True)#for debug
				p.setBonusType(iResourceType)
				p.setFeatureType(-1, -1)
				break	

	else:
		random.shuffle(plotsboundaries)
		for p in plotsboundaries:
			if (p.getBonusType(-1) == BonusTypes.NO_BONUS):
				p.setTerrainType(iTerrainType,True,True)
				p.setBonusType(iResourceType)
				p.setFeatureType(-1, -1)
				break	
