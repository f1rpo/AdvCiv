#
#   FILE:       Peirce.py
#   AUTHOR:     LPlate
#   BASIS:      Discworld.py by Terkhen
#   PURPOSE:    Generates a tidally locked planet
#-----------------------------------------------------------------------------
# CHANGELOG
#-----------------------------------------------------------------------------
#   1.0: Original Version
#   2.0: Amended to avoid starting on single tile islands
#   3.0: Including toroidal map option, without the tile frame.  Some other minor tidies.
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from CvPythonExtensions import *
import CvMapGeneratorUtil
import math
import sys
from SASUtils import getInfoTypeOrFail
from SAS_WorldSizeUtils import sas_warn_simple_game_stale_option_once
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator

gc = CyGlobalContext()
# <!-- custom: PlotTypes are engine enums (not XML Type tags), so use PlotTypes.* constants here; `getInfoTypeOrFail(...)` is for XML tags such as TERRAIN_*/FEATURE_*/BONUS_* only. (GPT-5.3-Codex) -->
iPeak = PlotTypes.PLOT_PEAK
iLand = PlotTypes.PLOT_LAND
iOcean = PlotTypes.PLOT_OCEAN
iDesert = getInfoTypeOrFail("TERRAIN_DESERT")

# Global values that determine how the MapScript works.
iTerrainGrain = 3
# Grain used for terrainVarFractal.

iFeatureGrain = 4
# Grain used for featuresVarFractal.

lStartingPlotAreas = list()
# List of map area polygons (see MapAreaPolygon class definition in this file) in which civilizations can start playing
# the game.

map = CyGlobalContext().getMap()
# Access to map global context.

game = CyGlobalContext().getGame()
# Access to game global context.

terrainVarFractal = None
# Fractal used to introduce random variations to terrain types depending on their distance to the center of the disc.

featuresVarFractal = None
# Fractal used to introduce random variations to feature types depending on their distance to the center of the disc.
### Peirce Options ###
def getDescription():
	return "Tidally locked quincuncial world with optional toroidal/no-wrap layout and multiple continent patterns. Based on Peirce by LPlate2. Modified for AdvCiv-SAS: world-size scaling, larger player-count compatibility, and script-fix updates."

def getNumCustomMapOptions():
	return 2

def getCustomMapOptionName(argsList):
	index = argsList[0]
	option_names = {
		0: "Map Type",
		1: "Peirce Wrap",
	}
	if not option_names.has_key(index):
		sas_warn_simple_game_stale_option_once(index, getNumCustomMapOptions())
	translated_text = unicode(CyTranslator().getText(option_names[index], ()))
	return translated_text

def getNumCustomMapOptionValues(argsList):
	index = argsList[0]
	if (index == 0):
		return 5
	else:
		return 2

def getCustomMapOptionDescAt(argsList):
	iOption = argsList[0]
	iSelection = argsList[1]
	selection_names = {
		0: ["Pangaea",
			"2 N-S Continents",
			"3 N-S Continents",
			"4 N-S Continents",
			"E-W Continents"],
		1: ["Toroidal Wrap",
			"No Wrap"],
	}
	if not selection_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	return unicode(CyTranslator().getText(selection_names[iOption][iSelection], ()))

def getCustomMapOptionDefault(argsList):
	return -1

def isAdvancedMap():
	# <!-- custom: Keep Peirce visible in Simple Game; this script is SAS-adapted and now uses guarded options/grid scaling for safer normal selection flow. (GPT-5.3-Codex) -->
	return 0

def isClimateMap():
	# Peirce uses the climate options.
	# :return: True
	return True

def isSeaLevelMap():
	# Peirce uses the sea level options.
	# :return: True
	return True

def getGridSize(argsList):
	# On the Peirce projection, the grid size is adjusted based on the chosen map type to aim to give ~ 7 cities per player and allow for the frame around the playing area
	# :param argsList: List containing the chosen world size as its single element. This element can be -1 on loads.
	# :return: tuple with the chosen map width and height.
	if (argsList[0] == -1): # (-1,) is passed to function on loads
		return []

	map = CyGlobalContext().getMap()

	# Get user input.
	iPeirceType = map.getCustomMapOption(0)
	iPeirceWrap = map.getCustomMapOption(1)

	[eWorldSize] = argsList
	cgc = CyGlobalContext()
	if eWorldSize < 0 or eWorldSize >= cgc.getNumWorldInfos():
		eWorldSize = cgc.getNumWorldInfos() - 1
	kWorld = cgc.getWorldInfo(eWorldSize)
	iMinWorldSide = min(kWorld.getGridWidth(), kWorld.getGridHeight())
	# <!-- custom: Peirce uses square grids; scale from selected world-size min side with a global define so Arena..SAS48 can be tuned centrally like Planet Generator. (GPT-5.3-Codex) -->
	iGridPercent = cgc.getDefineINT("SAS_PEIRCE_GRID_PERCENT")
	iGridPercent = max(1, min(200, iGridPercent))
	iBaseSide = max(8, int((iMinWorldSide * iGridPercent + 50) / 100))
	# Keep original Peirce-type relative sizing (normalized from old Huge tables).
	if iPeirceWrap == 0: # Toroidal
		type_scale = {0: 1.0, 1: 1.4, 2: 1.2667, 3: 1.0, 4: 1.0667}
	else: # No Wrap
		type_scale = {0: 1.0, 1: 1.4286, 2: 1.2857, 3: 1.0, 4: 1.0714}
	fScale = type_scale.get(iPeirceType, 1.0)
	iSide = max(8, int(iBaseSide * fScale + 0.5))
	return (iSide, iSide)

def getWrapX():
	map = CyGlobalContext().getMap()
	iPeirceWrap = map.getCustomMapOption(1)
	if iPeirceWrap == 0: # Toroidal
		return True
	else:
		return False

def getWrapY():
	map = CyGlobalContext().getMap()
	iPeirceWrap = map.getCustomMapOption(1)
	if iPeirceWrap == 0: # Toroidal
		return True
	else:
		return False

def isBonusIgnoreLatitude():
	# The latitude calculations made in Civilization IV are not appropiate for placing bonuses on Peirce, where the eauators are diagonals linking the midpoints of the sides.
	# :return:
# Not used in Tilted_Axis, so changed to False, as it seems more appropriate.
	return False

def generatePlotTypes():
	# Generates the PlotTypes for all plots in the map. See PeirceMultilayeredFractal for details. This method also
	# creates the border of the Peirce.  It should ensure that there is Ocean where the crease in the map is.
	# :return: List of the PlotTypes generated for each plot of the map.
	print("[PEIRCE] -- generatePlotTypes()")

	plotGenerator = PeirceMultilayeredFractal()

	plotTypes = plotGenerator.generatePlotsByRegion()

	map = CyGlobalContext().getMap()
	iPeirceWrap = map.getCustomMapOption(1)
	if iPeirceWrap == 1: # No wrap
		iQuadW = (map.getGridWidth()-4)/2
		iPeirceType = map.getCustomMapOption(0)
		for iX in range(map.getGridWidth()):
			for iY in range(map.getGridHeight()):

				if isPeirceCrease(iX, iY):
					plotTypes[iY * map.getGridWidth() + iX] = PlotTypes.PLOT_OCEAN

				if iX == 0 or iX == (map.getGridWidth() - 1):
					if (iY > 1 and iY < map.getGridWidth() - 2):
						plotTypes[iY * map.getGridWidth() + iX] = PlotTypes.PLOT_PEAK
					else:
						plotTypes[iY * map.getGridWidth() + iX] = PlotTypes.PLOT_OCEAN
				elif iX == 1:
					if iY > 1 and iY < (map.getGridWidth() - 2):
						plotTypes[iY * map.getGridWidth() + iX] = plotTypes[(map.getGridWidth() - iY - 1) * map.getGridWidth() + iX + 1]
				elif iX == (map.getGridWidth() - 2):
					if iY > 1 and iY < (map.getGridWidth() - 2):
						plotTypes[iY * map.getGridWidth() + iX] = plotTypes[(map.getGridWidth() - iY - 1) * map.getGridWidth() + iX - 1]

				if iY == 0 or iY == (map.getGridWidth() - 1):
					if (iX > 1 and iX < map.getGridWidth() - 2):
						plotTypes[iY * map.getGridWidth() + iX] = PlotTypes.PLOT_PEAK
					else:
						plotTypes[iY * map.getGridWidth() + iX] = PlotTypes.PLOT_OCEAN
				elif iY == 1:
					if iX > 1 and iX < (map.getGridWidth() - 2):
						plotTypes[iY * map.getGridWidth() + iX] = plotTypes[(iY+1) * map.getGridWidth() + (map.getGridWidth() - iX - 1)]
				elif iY == (map.getGridWidth() - 2):
					if iX > 1 and iX < (map.getGridWidth() - 2):
						plotTypes[iY * map.getGridWidth() + iX] = plotTypes[(iY-1) * map.getGridWidth() + (map.getGridWidth() - iX - 1)]

	# Sort far corners
				plotTypes[1 * map.getGridWidth() + 1] = plotTypes[(map.getGridWidth() - 3) * map.getGridWidth() + (map.getGridWidth() - 3)]
				plotTypes[1 * map.getGridWidth() + (map.getGridWidth() - 2)] = plotTypes[(map.getGridWidth() - 3) * map.getGridWidth() + 2]
				plotTypes[(map.getGridWidth() - 2) * map.getGridWidth() + (map.getGridWidth() - 2)] = plotTypes[2 * map.getGridWidth() + 2]
				plotTypes[(map.getGridWidth() - 2) * map.getGridWidth() + 1] = plotTypes[2 * map.getGridWidth() + (map.getGridWidth() - 3)]

				if isPeirceCrease(iX, iY):
					plotTypes[iY * map.getGridWidth() + iX] = PlotTypes.PLOT_OCEAN

				if iPeirceType == 4: # E-W (ensure ocean at equator)
					if isEquatorialBand(iX, iY):
						plotTypes[iY * map.getGridWidth() + iX] = PlotTypes.PLOT_OCEAN

	else:
		iQuadW = (map.getGridWidth())/2
		iPeirceType = map.getCustomMapOption(0)
		for iX in range(map.getGridWidth()):
			for iY in range(map.getGridHeight()):

	# 02Mar - comment out ##
				if isPeirceCrease(iX, iY): 
					plotTypes[iY * map.getGridWidth() + iX] = PlotTypes.PLOT_OCEAN

				if iPeirceType == 4: # E-W (ensure ocean at equator)
					if isEquatorialBand(iX, iY):
						plotTypes[iY * map.getGridWidth() + iX] = PlotTypes.PLOT_OCEAN

	return plotTypes

def generateTerrainTypes():
	# Generates terrain types for all the plots of the map. The terrain generation is based on the equators being the diagonals linking the midpoints of the sides.
	# :return: List of generated terrain types.
	print("[PEIRCE] -- generateTerrainTypes()")

	global terrainVarFractal
	terrainVarFractal = getVariationFractal(iTerrainGrain)
	terrainGen = PeirceTerrainGenerator()
	terrainTypes = terrainGen.generateTerrain()

	iOceanTerrain = getInfoTypeOrFail("TERRAIN_OCEAN")
	iPeakTerrain = getInfoTypeOrFail("TERRAIN_PEAK")

	map = CyGlobalContext().getMap()
	iPeirceWrap = map.getCustomMapOption(1)

	if iPeirceWrap == 1: # No wrap
		iQuadW = (map.getGridWidth()-4)/2
		iPeirceType = map.getCustomMapOption(0)
		for iX in range(map.getGridWidth()):
			for iY in range(map.getGridHeight()):

				if isPeirceCrease(iX, iY): # Peircecrease is not working as wanted
					terrainTypes[iY * map.getGridWidth() + iX] = iOceanTerrain

				if iX == 0 or iX == (map.getGridWidth() - 1):
					if (iY > 1 and iY < map.getGridWidth() - 2):
						iTemp = 1 # if use terrainTypes[iY * map.getGridWidth() + iX] = iPeakTerrain get flat peaks
	#					if terrainTypes[iY * map.getGridWidth() + iX] != iPeakTerrain:
	#						terrainTypes[iY * map.getGridWidth() + iX] = iPeakTerrain
					else:
						terrainTypes[iY * map.getGridWidth() + iX] = iOceanTerrain
				elif iX == 1:
					if iY > 1 and iY < (map.getGridWidth() - 2):
						iMirrorX = 2
						iMirrorY = map.getGridWidth() - iY - 1
						terrainTypes[iY * map.getGridWidth() + iX] = terrainTypes[iMirrorY * map.getGridWidth() + iMirrorX]
				elif iX == (map.getGridWidth() - 2):
					if iY > 1 and iY < (map.getGridWidth() - 2):
						iMirrorX = map.getGridWidth() - 3
						iMirrorY = map.getGridWidth() - iY - 1
						terrainTypes[iY * map.getGridWidth() + iX] = terrainTypes[iMirrorY * map.getGridWidth() + iMirrorX]

				if iY == 0 or iY == (map.getGridWidth() - 1):
					if (iX > 1 and iX < map.getGridWidth() - 2):
						iTemp = 1 # if use terrainTypes[iY * map.getGridWidth() + iX] = iPeakTerrain get flat peaks
	#					if terrainTypes[iY * map.getGridWidth() + iX] != iPeakTerrain:
	#						terrainTypes[iY * map.getGridWidth() + iX] = iPeakTerrain
					else:
						terrainTypes[iY * map.getGridWidth() + iX] = iOceanTerrain
				elif iY == 1:
					if iX > 1 and iX < (map.getGridWidth() - 2):
						iMirrorX = map.getGridWidth() - iX - 1
						iMirrorY = 2
						terrainTypes[iY * map.getGridWidth() + iX] = terrainTypes[iMirrorY * map.getGridWidth() + iMirrorX]
				elif iY == (map.getGridWidth() - 2):
					if iX > 1 and iX < (map.getGridWidth() - 2):
						iMirrorX = map.getGridWidth() - iX - 1
						iMirrorY = map.getGridWidth() - 3
						terrainTypes[iY * map.getGridWidth() + iX] = terrainTypes[iMirrorY * map.getGridWidth() + iMirrorX]

				if isPeirceCrease(iX, iY):
					terrainTypes[iY * map.getGridWidth() + iX] = iOceanTerrain

				if iPeirceType == 4: # E-W (ensure ocean at equator)
					if isEquatorialBand(iX, iY):
						terrainTypes[iY * map.getGridWidth() + iX] = iOceanTerrain

	# Sort far corners
				terrainTypes[1 * map.getGridWidth() + 1] = terrainTypes[(map.getGridWidth() - 3) * map.getGridWidth() + (map.getGridWidth() - 3)]
				terrainTypes[1 * map.getGridWidth() + (map.getGridWidth() - 2)] = terrainTypes[(map.getGridWidth() - 3) * map.getGridWidth() + 2]
				terrainTypes[(map.getGridWidth() - 2) * map.getGridWidth() + (map.getGridWidth() - 2)] = terrainTypes[2 * map.getGridWidth() + 2]
				terrainTypes[(map.getGridWidth() - 2) * map.getGridWidth() + 1] = terrainTypes[2 * map.getGridWidth() + (map.getGridWidth() - 3)]

	else:
		iQuadW = (map.getGridWidth())/2
		iPeirceType = map.getCustomMapOption(0)
		for iX in range(map.getGridWidth()):
			for iY in range(map.getGridHeight()):

				if iPeirceType == 4: # E-W (ensure ocean at equator)
					if isEquatorialBand(iX, iY):
						terrainTypes[iY * map.getGridWidth() + iX] = iOceanTerrain

	return terrainTypes

def addFeatures():
	# Generates feature types for all the plots of the map. These reflect the equator on the diagonals linking the midpoints of the edges
	print("[PEIRCE] -- addFeatures()")

	# Add other features.
	global featuresVarFractal
	featuresVarFractal = getVariationFractal(iFeatureGrain)
	featureGen = PeirceFeatureGenerator()
	featureGen.addFeatures()

	map = CyGlobalContext().getMap()
	iPeirceWrap = map.getCustomMapOption(1)

	if iPeirceWrap == 1: # No wrap
# The repeated "wrap tiles"
		iQuadW = (map.getGridWidth()-4)/2
		iPeirceType = map.getCustomMapOption(0)

		for iX in range(map.getGridWidth()):
			for iY in range(map.getGridHeight()):

				if iX == 1:
					if iY > 1 and iY < (map.getGridWidth() - 2):
						iMirrorX = 2
						iMirrorY = map.getGridWidth() - iY - 1
						pMirrorPlot = map.plot(iX, iY)
						iFeature = pMirrorPlot.getFeatureType()
						map.plot(iX,iY).setFeatureType(iFeature,-1)
				elif iX == (map.getGridWidth() - 2):
					if iY > 1 and iY < (map.getGridWidth() - 2):
						iMirrorX = map.getGridWidth() - 3
						iMirrorY = map.getGridWidth() - iY - 1
						pMirrorPlot = map.plot(iX, iY)
						iFeature = pMirrorPlot.getFeatureType()
						map.plot(iX,iY).setFeatureType(iFeature,-1)
				if iY == 1:
					if iX > 1 and iX < (map.getGridWidth() - 2):
						iMirrorX =  map.getGridWidth() - iX - 1
						iMirrorY = 2
						pMirrorPlot = map.plot(iX, iY)
						iFeature = pMirrorPlot.getFeatureType()
						map.plot(iX,iY).setFeatureType(iFeature,-1)
				elif iY == (map.getGridWidth() - 2):
					if iX > 1 and iX < (map.getGridWidth() - 2):
						iMirrorX =  map.getGridWidth() - iX - 1
						iMirrorY = map.getGridWidth() - iY - 2
						pMirrorPlot = map.plot(iX, iY)
						iFeature = pMirrorPlot.getFeatureType()
						map.plot(iX,iY).setFeatureType(iFeature,-1)

	# Sort far corners
		pMirrorPlot = map.plot(map.getGridWidth() - 3, map.getGridWidth() - 3)
		iFeature = pMirrorPlot.getFeatureType()
		map.plot(1,1).setFeatureType(iFeature,-1)
		pMirrorPlot = map.plot(map.getGridWidth() - 3, 2)
		iFeature = pMirrorPlot.getFeatureType()
		map.plot(1,map.getGridWidth() - 2).setFeatureType(iFeature,-1)
		pMirrorPlot = map.plot(2, map.getGridWidth() - 3)
		iFeature = pMirrorPlot.getFeatureType()
		map.plot(map.getGridWidth() - 2,1).setFeatureType(iFeature,-1)
		pMirrorPlot = map.plot(2, 2)
		iFeature = pMirrorPlot.getFeatureType()
		map.plot(map.getGridWidth() - 2,map.getGridWidth() - 2).setFeatureType(iFeature,-1)

	return 0

def findStartingPlot(argsList):
	# Find starting plot for a certain player.
	# :param argsList: List that contains the playerID of the player.
	# :return: Starting plot
	[playerID] = argsList

	def isInsidePlayableRegion(pID, iX, iY):
		global lStartingPlotAreas

# Avoid starting on islands, Rev 2
		pPlot = CyMap().plot(iX, iY)
		pArea = pPlot.area()
		if pArea.getNumTiles() < 4:
			return False

		for mapAreaPolygon in lStartingPlotAreas:
			if mapAreaPolygon.isInside(iX, iY):
				return True

		return False

	return CvMapGeneratorUtil.findStartingPlot(playerID, isInsidePlayableRegion)

def afterGeneration():
	map = CyGlobalContext().getMap()
	iQuart = (map.getGridWidth())/4
	iCentre = (map.getGridWidth())/2
	iPeirceType = map.getCustomMapOption(0)
	iPeirceWrap = map.getCustomMapOption(1)
	iFeatureIce = getInfoTypeOrFail("FEATURE_ICE")
	iPeakTerrain = getInfoTypeOrFail("TERRAIN_PEAK")

	for iX in range(map.getGridWidth()):
		for iY in range(map.getGridHeight()):

			if isPeirceCrease(iX, iY):
				map.plot(iX,iY).setPlotType(PlotTypes.PLOT_OCEAN, True, True)
				map.plot(iX,iY).setImprovementType(-1)
				map.plot(iX,iY).setBonusType(-1)

			if iPeirceWrap == 0: # Toroidal
				if ((iX == 0 or iX == map.getGridWidth()-1) and (iY > iQuart and iY <= 3*iQuart)) or ((iY == 0 or iY == map.getGridWidth()-1) and (iX > iQuart and iX <= 3*iQuart)):
					map.plot(iX, iY).setFeatureType(iFeatureIce, -1)
				if (((iX >= iCentre - 2 and iX <= iCentre + 1) and (iY >= iCentre - 2 and iY <= iCentre + 1)) or ((iX <= 1) and (iY <= 1)) or ((iX <= 1) and (iY >= map.getGridHeight() - 2)) or ((iX >= map.getGridHeight() - 2) and (iY <= 1))):
					if map.plot(iX,iY).isWater():
						iPolarIce = CyGame().getSorenRandNum(100, "Chance of Icy Ocean")
						if iPolarIce < 70:
							map.plot(iX, iY).setFeatureType(iFeatureIce, -1)

			else:
				if ((iX == 0 or iX == map.getGridWidth()-1) and (iY > 4 and iY <= map.getGridWidth()-5)) or ((iY == 0 or iY == map.getGridWidth()-1) and (iX > 4 and iX <= map.getGridWidth()-5)):
					map.plot(iX,iY).setPlotType(PlotTypes.PLOT_PEAK, False, True)
					map.plot(iX,iY).setImprovementType(-1)
					map.plot(iX,iY).setBonusType(-1)
					map.plot(iX,iY).setFeatureType(-1,-1)
					map.plot(iX,iY).setTerrainType(iPeakTerrain,False,True)
					map.plot(iX,iY).setPlotType(PlotTypes.PLOT_PEAK, False, True)

# All utility classes and methods used by the MapScript are implemented below.

class PeirceMultilayeredFractal(CvMapGeneratorUtil.MultilayeredFractal):
	# Multilayered fractal customized for Peirce.
	#
	# Along with MapAreaPolygon, this implementation of MultilayeredFractal allows to place regions with arbitrary
	# polygonal shapes. These shapes are distorted and
	# randomized slightly to make them appear more natural (see MapAreaPolygon).

	def generatePlotsByRegion(self):
		# Generate all of the regions of the Peirce map.
		# :return: Plots generated.
		# Remove all elements from the starting plot areas list.
		del lStartingPlotAreas[:]
		iBaseSeaLevel = 70 + self.gc.getSeaLevelInfo(self.map.getSeaLevel()).getSeaLevelChange()

		map = CyGlobalContext().getMap()

		# Get user input.
		iPeirceType = map.getCustomMapOption(0)

		map = CyGlobalContext().getMap()
		iPeirceWrap = map.getCustomMapOption(1)

		if iPeirceWrap == 1: # No wrap

			iQuadW = (map.getGridWidth()-4)/2
			iTRCentreX = map.getGridWidth()/2 - 1
			iTRCentreY = map.getGridWidth()/2 - 1

			iPrim = CyGame().getSorenRandNum(4, "Picking Primary Quad")
			iSec = CyGame().getSorenRandNum(4, "Picking Secondary Quad")

			if iPeirceType == 0: # Pangaea
				iParallelLength = 3*iQuadW/4 - 4
				iParallelOffset = iQuadW/4
				iParallelHeight = iQuadW/2 - 5

	# Top Right
				if iPrim != 0:
					iX1 = iTRCentreX
					iY1 = iTRCentreY + iQuadW/6
					iX2 = iTRCentreX - iQuadW/4
					iY2 = iTRCentreY + iQuadW/2 - 1
					iX3 = iTRCentreX + 3*iQuadW/4
					iY3 = iTRCentreY + iQuadW/2 - 1
					iX4 = iTRCentreX + iQuadW/2 - 1
					iY4 = iTRCentreY + iQuadW/2 - 1
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = iTRCentreX + iQuadW/6
					iY1 = iTRCentreY 
					iX2 = iTRCentreX + iQuadW/6
					iY2 = iTRCentreY + iQuadW/4
					iX3 = iTRCentreX + iQuadW/2 - 1
					iY3 = iTRCentreY + 3*iQuadW/4
					iX4 = iTRCentreX + iQuadW/2 - 1
					iY4 = iTRCentreY - iQuadW/4
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = iTRCentreX + iQuadW/2 - 1
					iY1 = iTRCentreY + iQuadW/4
					iX2 = iTRCentreX + iQuadW/2 - 1
					iY2 = map.getGridWidth() - 3
					iX3 = map.getGridWidth() - iQuadW/6
					iY3 = map.getGridWidth() - 3
					iX4 = map.getGridWidth() - iQuadW/6
					iY4 = iTRCentreY + iQuadW/2 - 1
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = iTRCentreX + iQuadW/4
					iY1 = iTRCentreY + iQuadW/2 - 1
					iX2 = iTRCentreX + iQuadW/2
					iY2 = map.getGridWidth() - iQuadW/6
					iX3 = map.getGridWidth() - 3
					iY3 = map.getGridWidth() - iQuadW/6
					iX4 = map.getGridWidth() - 3
					iY4 = iTRCentreY + iQuadW/2 - 1
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = iTRCentreX + iQuadW/2 - 1
					iY1 = map.getGridWidth() - 5
					iX2 = iTRCentreX + iQuadW/2 - 1
					iY2 = map.getGridWidth() - 3
					iX3 = map.getGridWidth() - iQuadW/6
					iY3 = map.getGridWidth() - 3
					iX4 = map.getGridWidth() - iQuadW/6
					iY4 = map.getGridWidth() - 5
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = map.getGridWidth() - 5
					iY1 = iTRCentreY + iQuadW/2 - 1
					iX2 = map.getGridWidth() - 5
					iY2 = map.getGridWidth() - iQuadW/6
					iX3 = map.getGridWidth() - 3
					iY3 = map.getGridWidth() - iQuadW/6
					iX4 = map.getGridWidth() - 3
					iY4 = iTRCentreY + iQuadW/2 - 1
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iWidth2 = (iQuadW/2)
					iRefX2 = iTRCentreX + (iQuadW/4)
					iRefY2 = iTRCentreY + (iQuadW/4) + 1

					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2)

	# Bottom Left
				if iPrim != 1:
					iX1 = iTRCentreX - 1
					iY1 = iTRCentreY - iQuadW/6 - 1
					iX2 = iTRCentreX + iQuadW/4 - 1
					iY2 = iTRCentreY - iQuadW/2 - 2
					iX3 = iTRCentreX - 3*iQuadW/4 - 1
					iY3 = iTRCentreY - iQuadW/2 - 2
					iX4 = iTRCentreX - iQuadW/2 - 2
					iY4 = iTRCentreY - iQuadW/2 - 2
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = iTRCentreX - iQuadW/6 - 1
					iY1 = iTRCentreY - 1
					iX2 = iTRCentreX - iQuadW/6 - 1
					iY2 = iTRCentreY - iQuadW/4 - 1
					iX3 = iTRCentreX - iQuadW/2 - 2
					iY3 = iTRCentreY - 3*iQuadW/4 - 1
					iX4 = iTRCentreX - iQuadW/2 - 2
					iY4 = iTRCentreY + iQuadW/4 - 1
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = iTRCentreX - iQuadW/2 - 2
					iY1 = iTRCentreY - iQuadW/4 - 1
					iX2 = iTRCentreX - iQuadW/2 - 2
					iY2 = 2
					iX3 = iQuadW/6
					iY3 = 2
					iX4 = iQuadW/6
					iY4 = iTRCentreY - iQuadW/2 - 2
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = iTRCentreX - iQuadW/4 - 1
					iY1 = iTRCentreY - iQuadW/2 - 2
					iX2 = iTRCentreX - iQuadW/2
					iY2 = iQuadW/6
					iX3 = 2
					iY3 = iQuadW/6
					iX4 = 2
					iY4 = iTRCentreY - iQuadW/2 - 2
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = iTRCentreX - iQuadW/2 - 2
					iY1 = 4
					iX2 = iTRCentreX - iQuadW/2 - 2
					iY2 = 2
					iX3 = iQuadW/6
					iY3 = 2
					iX4 = iQuadW/6
					iY4 = 4
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = 4
					iY1 = iTRCentreY - iQuadW/2 - 2
					iX2 = 4
					iY2 = iQuadW/6
					iX3 = 2
					iY3 = iQuadW/6
					iX4 = 2
					iY4 = iTRCentreY - iQuadW/2 - 2
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iWidth2 = (iQuadW/2)
					iRefX2 = iTRCentreX - (3* iQuadW/4) -1
					iRefY2 = iTRCentreY - (3* iQuadW/4) -1

					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2)

	# Top Left
				if iPrim != 2:
					iX1 = iTRCentreX - 1
					iY1 = iTRCentreY + iQuadW/6
					iX2 = iTRCentreX + iQuadW/4 - 1
					iY2 = iTRCentreY + iQuadW/2 - 1
					iX3 = iTRCentreX - 3*iQuadW/4 - 1
					iY3 = iTRCentreY + iQuadW/2 - 1
					iX4 = iTRCentreX - iQuadW/2 - 2
					iY4 = iTRCentreY + iQuadW/2 - 1
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = iTRCentreX - iQuadW/6 - 1
					iY1 = iTRCentreY 
					iX2 = iTRCentreX - iQuadW/6 - 1
					iY2 = iTRCentreY + iQuadW/4
					iX3 = iTRCentreX - iQuadW/2 - 2
					iY3 = iTRCentreY + 3*iQuadW/4
					iX4 = iTRCentreX - iQuadW/2 - 2
					iY4 = iTRCentreY - iQuadW/4
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = iTRCentreX - iQuadW/2 - 2
					iY1 = iTRCentreY + iQuadW/4
					iX2 = iTRCentreX - iQuadW/2 - 2
					iY2 = map.getGridWidth() - 3
					iX3 = iQuadW/6
					iY3 = map.getGridWidth() - 3
					iX4 = iQuadW/6
					iY4 = iTRCentreY + iQuadW/2 - 1
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = iTRCentreX - iQuadW/4 - 1
					iY1 = iTRCentreY + iQuadW/2 - 1
					iX2 = iTRCentreX - iQuadW/2
					iY2 = map.getGridWidth() - iQuadW/6
					iX3 = 2
					iY3 = map.getGridWidth() - iQuadW/6
					iX4 = 2
					iY4 = iTRCentreY + iQuadW/2 - 1
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = iTRCentreX - iQuadW/2 - 2
					iY1 = map.getGridWidth() - 5
					iX2 = iTRCentreX - iQuadW/2 - 2
					iY2 = map.getGridWidth() - 3
					iX3 = iQuadW/6
					iY3 = map.getGridWidth() - 3
					iX4 = iQuadW/6
					iY4 = map.getGridWidth() - 5
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = 4
					iY1 = iTRCentreY + iQuadW/2 - 1
					iX2 = 4
					iY2 = map.getGridWidth() - iQuadW/6
					iX3 = 2
					iY3 = map.getGridWidth() - iQuadW/6
					iX4 = 2
					iY4 = iTRCentreY + iQuadW/2 - 1
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iWidth2 = (iQuadW/2)
					iRefX2 = iTRCentreX - (3* iQuadW/4) -1
					iRefY2 = iTRCentreY + (iQuadW/4) + 1
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2)

	# Bottom Right
				if iPrim != 3:
					iX1 = iTRCentreX
					iY1 = iTRCentreY - iQuadW/6 - 1
					iX2 = iTRCentreX - iQuadW/4
					iY2 = iTRCentreY - iQuadW/2 - 2
					iX3 = iTRCentreX + 3*iQuadW/4
					iY3 = iTRCentreY - iQuadW/2 - 2
					iX4 = iTRCentreX + iQuadW/2 - 1
					iY4 = iTRCentreY - iQuadW/2 - 2
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = iTRCentreX + iQuadW/6
					iY1 = iTRCentreY - 1
					iX2 = iTRCentreX + iQuadW/6
					iY2 = iTRCentreY - iQuadW/4 - 1
					iX3 = iTRCentreX + iQuadW/2 - 1
					iY3 = iTRCentreY - 3*iQuadW/4 - 1
					iX4 = iTRCentreX + iQuadW/2 - 1
					iY4 = iTRCentreY + iQuadW/4 - 1
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = iTRCentreX + iQuadW/2 - 1
					iY1 = iTRCentreY - iQuadW/4 - 1
					iX2 = iTRCentreX + iQuadW/2 - 1
					iY2 = 2
					iX3 = map.getGridWidth() - iQuadW/6
					iY3 = 2
					iX4 = map.getGridWidth() - iQuadW/6
					iY4 = iTRCentreY - iQuadW/2 - 2
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = iTRCentreX + iQuadW/4
					iY1 = iTRCentreY - iQuadW/2 - 2
					iX2 = iTRCentreX + iQuadW/2
					iY2 = iQuadW/6
					iX3 = map.getGridWidth() - 3
					iY3 = iQuadW/6
					iX4 = map.getGridWidth() - 3
					iY4 = iTRCentreY - iQuadW/2 - 2
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = iTRCentreX + iQuadW/2 - 1
					iY1 = 4
					iX2 = iTRCentreX + iQuadW/2 - 1
					iY2 = 2
					iX3 = map.getGridWidth() - iQuadW/6
					iY3 = 2
					iX4 = map.getGridWidth() - iQuadW/6
					iY4 = 4
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iX1 = map.getGridWidth() - 5
					iY1 = iTRCentreY - iQuadW/2 - 2
					iX2 = map.getGridWidth() - 5
					iY2 = iQuadW/6
					iX3 = map.getGridWidth() - 3
					iY3 = iQuadW/6
					iX4 = map.getGridWidth() - 3
					iY4 = iTRCentreY - iQuadW/2 - 2
					self.generatePlotsPoly4Continent(iBaseSeaLevel, iX1,iY1, iX2,iY2, iX3,iY3, iX4,iY4)

					iWidth2 = (iQuadW/2)
					iRefX2 = iTRCentreX + (iQuadW/4)
					iRefY2 = iTRCentreY - (3* iQuadW/4) -1

					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2)

			elif iPeirceType == 1 or iPeirceType == 2 or iPeirceType == 3: # 2 N-S
				if iPrim == 0 or iPrim == 2: # Top Right and Bottom Left
			# Primary Continent
					iWidth1 = (iQuadW/2)-3
					iWidth2 = (iQuadW/2)
					iWidth3 = (iQuadW/2)-2
					iRefX1 = iTRCentreX + 1
					iRefX2 = iTRCentreX + (iQuadW/4)
					iRefX3 = iTRCentreX +(iQuadW/2) +1
					iRefY1 = iTRCentreY
					iRefY2 = iTRCentreY + (iQuadW/4) + 1
					iRefY3 = iTRCentreY + (iQuadW/2) + 1

					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX1,iRefY1,iWidth1)
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2)
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX3,iRefY3,iWidth3)
			# Secondary (Opposite) Continent
					iWidth1 = (iQuadW/2)-3
					iWidth2 = (iQuadW/2)
					iWidth3 = (iQuadW/2)-2
					iRefX1 = iTRCentreX - (iQuadW/2)
					iRefX2 = iTRCentreX - (3* iQuadW/4) -1
					iRefX3 = iTRCentreX - (iQuadW) - 1
					iRefY1 = iTRCentreY - (iQuadW/2)
					iRefY2 = iTRCentreY - (3* iQuadW/4) -1
					iRefY3 = iTRCentreY - (iQuadW) - 1

					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX1,iRefY1,iWidth1)
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # This is to be replaced with the diagonal continent
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX3,iRefY3,iWidth3)
				else: # Top Left and Bottom Right
			# Primary Continent
					iWidth1 = (iQuadW/2) -3
					iWidth2 = (iQuadW/2)
					iWidth3 = (iQuadW/2)-2
					iRefX1 = iTRCentreX - (iQuadW/2)
					iRefX2 = iTRCentreX - (3* iQuadW/4) -1
					iRefX3 = iTRCentreX - (iQuadW) - 1
					iRefY1 = iTRCentreY
					iRefY2 = iTRCentreY + (iQuadW/4) + 1
					iRefY3 = iTRCentreY + (iQuadW/2) + 1

					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX1,iRefY1,iWidth1)
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2)
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX3,iRefY3,iWidth3)
			# Secondary (Opposite) Continent
					iWidth1 = (iQuadW/2)-3
					iWidth2 = (iQuadW/2)
					iWidth3 = (iQuadW/2)-2
					iRefX1 = iTRCentreX + 1
					iRefX2 = iTRCentreX + (iQuadW/4)
					iRefX3 = iTRCentreX + (iQuadW/2) + 1
					iRefY1 = iTRCentreY - (iQuadW/2)
					iRefY2 = iTRCentreY - (3* iQuadW/4) -1
					iRefY3 = iTRCentreY - (iQuadW) - 1

					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX1,iRefY1,iWidth1)
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # This is to be replaced with the diagonal continent
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX3,iRefY3,iWidth3)

	# Third and fourth continents
				if iPeirceType == 2 or iPeirceType == 3:
	# TR
					if (iPrim == 1 or iPrim == 3):
						if iSec < 2 or iPeirceType == 3:
							iWidth1 = (iQuadW/2)-5
							iWidth2 = (iQuadW/2)-3
							iWidth3 = (iQuadW/2)-4
							iRefX1 = iTRCentreX + 3
							iRefX2 = iTRCentreX + (iQuadW/2) - 2
							iRefX3 = iTRCentreX + (iQuadW/4) + 2
							iRefY1 = iTRCentreY + 3
							iRefY2 = iTRCentreY + (iQuadW/2) - 2
							iRefY3 = iTRCentreY + (iQuadW/4) + 2

							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX1,iRefY1,iWidth1)
							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2)
							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX3,iRefY3,iWidth3)
	# BL
						if iSec >= 2 or iPeirceType == 3:
							iWidth1 = (iQuadW/2)-5
							iWidth2 = (iQuadW/2)-3
							iWidth3 = (iQuadW/2)-4
							iRefX1 = iTRCentreX - (iQuadW/2) + 1
							iRefX2 = iTRCentreX - (iQuadW) + 2
							iRefX3 = iTRCentreX - (iQuadW/2) - 4
							iRefY1 = iTRCentreY - (iQuadW/2) + 1
							iRefY2 = iTRCentreY - (iQuadW) + 2
							iRefY3 = iTRCentreY - (iQuadW/2) - 4

							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX1,iRefY1,iWidth1)
							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2)
							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX3,iRefY3,iWidth3)

					else:
	#TL
						if iSec < 2 or iPeirceType == 3:
							iWidth1 = (iQuadW/2)-5
							iWidth2 = (iQuadW/2)-3
							iWidth3 = (iQuadW/2)-4
							iRefX1 = iTRCentreX - (iQuadW/2) + 1
							iRefX2 = iTRCentreX - (iQuadW) + 2
							iRefX3 = iTRCentreX - (iQuadW/2) - 4
							iRefY1 = iTRCentreY + 3
							iRefY2 = iTRCentreY + (iQuadW/2) - 2
							iRefY3 = iTRCentreY + (iQuadW/2) + 2

							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX1,iRefY1,iWidth1)
							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2)
							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX3,iRefY3,iWidth3)
	#BR
						if iSec >= 2 or iPeirceType == 3:
							iWidth1 = (iQuadW/2)-5
							iWidth2 = (iQuadW/2)-3
							iWidth3 = (iQuadW/2)-4
							iRefX1 = iTRCentreX + 3
							iRefX2 = iTRCentreX + (iQuadW/2) - 2
							iRefX3 = iTRCentreX + (iQuadW/4) + 2
							iRefY1 = iTRCentreY - (iQuadW/2) + 1
							iRefY2 = iTRCentreY - (iQuadW) + 2
							iRefY3 = iTRCentreY - (iQuadW/4) - 4

							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX1,iRefY1,iWidth1)
							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2)
							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX3,iRefY3,iWidth3)

			elif iPeirceType == 4: # E-W 
				iParallelLength = 3*iQuadW/4 - 5
				iParallelOffset = iQuadW/4
				iParallelHeight = iQuadW/2 - 6

	# <!-- custom: The original Peirce script computed a distinct anchor for each portrait companion arm but passed the preceding landscape arm's anchor instead, shifting and overlapping the intended E-W continent pieces.
	# Use each freshly computed second anchor. See KI#255. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	# Top Right North Continent
				if iPrim != 0:
					iRefX1 = iTRCentreX - 1
					iRefY1 = iTRCentreY + 2
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX1,iRefY1,iParallelLength,iParallelHeight,-iParallelOffset,0)

					iRefX2 = iTRCentreX + 1 + iParallelHeight
					iRefY2 = iTRCentreY - iQuadW/4
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX2,iRefY2,iParallelLength,iParallelHeight,-iParallelOffset,1)

	# Bottom Left North Continent
				if iPrim != 1:
					iRefX1 = iTRCentreX - 4 - iParallelHeight
					iRefY1 = iTRCentreY - 4 - iParallelHeight
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX1,iRefY1,iParallelLength,iParallelHeight,-iParallelOffset,0)

					iRefX2 = iTRCentreX - 4
					iRefY2 = iTRCentreY - 2 - iParallelLength
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX2,iRefY2,iParallelLength,iParallelHeight,iParallelOffset,1)

	# Top Left North Continent
				if iPrim != 2:
					iRefX1 = iTRCentreX - 2 - iParallelLength
					iRefY1 = iTRCentreY + 2
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX1,iRefY1,iParallelLength,iParallelHeight,iParallelOffset,0)

					iRefX2 = iTRCentreX - 4
					iRefY2 = iTRCentreY - 1
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX2,iRefY2,iParallelLength,iParallelHeight,-iParallelOffset,1)

	# Bottom Right North Continent
				if iPrim != 3:
					iRefX1 = iTRCentreX - 1 - iParallelOffset
					iRefY1 = iTRCentreY - 4 - iParallelHeight
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX1,iRefY1,iParallelLength,iParallelHeight,iParallelOffset,0)

					iRefX2 = iTRCentreX + 2 + iParallelHeight
					iRefY2 = iTRCentreY - 1 - iParallelLength
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX2,iRefY2,iParallelLength,iParallelHeight,-iParallelOffset,1)

	# Top Right South Continent
				if iSec != 0:
					iRefX1 = map.getGridWidth() - iParallelLength
					iRefY1 = map.getGridWidth() - 3
					iRefX2 = map.getGridWidth() - 3
					iRefY2 = map.getGridWidth() - 3
					iRefX3 = map.getGridWidth() - 3
					iRefY3 = map.getGridWidth() - iParallelHeight
					iRefX4 = map.getGridWidth() - iParallelLength + iParallelOffset
					iRefY4 = map.getGridWidth() - iParallelHeight

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = map.getGridWidth() - 3
					iRefY1 = map.getGridWidth() - 3
					iRefX2 = map.getGridWidth() - iParallelHeight
					iRefY2 = map.getGridWidth() - 3
					iRefX3 = map.getGridWidth() - iParallelHeight
					iRefY3 = map.getGridWidth() - 1 - (iParallelLength - iParallelOffset)
					iRefX4 = map.getGridWidth() - 3
					iRefY4 = map.getGridWidth() - 1 - iParallelLength

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)
			# 
					iRefX1 = map.getGridWidth() - 5
					iRefY1 = map.getGridWidth() - 3
					iRefX2 = map.getGridWidth() - 3
					iRefY2 = map.getGridWidth() - 3
					iRefX3 = map.getGridWidth() - 3
					iRefY3 = map.getGridWidth() - iParallelHeight
					iRefX4 = map.getGridWidth() - 5
					iRefY4 = map.getGridWidth() - iParallelHeight

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = map.getGridWidth() - 3
					iRefY1 = map.getGridWidth() - 3
					iRefX2 = map.getGridWidth() - iParallelHeight
					iRefY2 = map.getGridWidth() - 3
					iRefX3 = map.getGridWidth() - iParallelHeight
					iRefY3 = map.getGridWidth() - 5
					iRefX4 = map.getGridWidth() - 3
					iRefY4 = map.getGridWidth() - 5

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

	# Top Left South Continent
				if iSec != 1:
					iRefX1 = 2
					iRefY1 = map.getGridWidth() - 6
					iRefX2 = iParallelLength
					iRefY2 = map.getGridWidth() - 6
					iRefX3 = iParallelLength - iParallelOffset
					iRefY3 = map.getGridWidth() - 5 - iParallelHeight
					iRefX4 = 2
					iRefY4 = map.getGridWidth() - 5 - iParallelHeight

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = 5
					iRefY1 = map.getGridWidth() - 3
					iRefX2 = 4 + iParallelHeight
					iRefY2 = map.getGridWidth() - 3
					iRefX3 = 4 + iParallelHeight
					iRefY3 = map.getGridWidth() - 1 - (iParallelLength - iParallelOffset)
					iRefX4 = 5
					iRefY4 = map.getGridWidth() - 1 - iParallelLength

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = 2
					iRefY1 = map.getGridWidth() - 6
					iRefX2 = 4
					iRefY2 = map.getGridWidth() - 6
					iRefX3 = 4
					iRefY3 = map.getGridWidth() - 5 - iParallelHeight
					iRefX4 = 2
					iRefY4 = map.getGridWidth() - 5 - iParallelHeight

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = 5
					iRefY1 = map.getGridWidth() - 3
					iRefX2 = 4 + iParallelHeight
					iRefY2 = map.getGridWidth() - 3
					iRefX3 = 4 + iParallelHeight
					iRefY3 = map.getGridWidth() - 5
					iRefX4 = 5
					iRefY4 = map.getGridWidth() - 5

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

	# Bottom Left South Continent
				if iSec != 3:
					iRefX1 = 2
					iRefY1 = 4
					iRefX2 = iParallelLength
					iRefY2 = 4
					iRefX3 = iParallelLength - iParallelOffset
					iRefY3 = 4 + iParallelHeight
					iRefX4 = 2
					iRefY4 = 4 + iParallelHeight

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = 5
					iRefY1 = 2
					iRefX2 = 4 + iParallelHeight
					iRefY2 = 2
					iRefX3 = 4 + iParallelHeight
					iRefY3 = iParallelLength - iParallelOffset
					iRefX4 = 5
					iRefY4 = iParallelLength

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = 2
					iRefY1 = 4
					iRefX2 = 4
					iRefY2 = 4
					iRefX3 = 4
					iRefY3 = 4 + iParallelHeight
					iRefX4 = 2
					iRefY4 = 4 + iParallelHeight

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = 5
					iRefY1 = 2
					iRefX2 = 4 + iParallelHeight
					iRefY2 = 2
					iRefX3 = 4 + iParallelHeight
					iRefY3 = 4
					iRefX4 = 5
					iRefY4 = 4

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

	# Bottom Right South Continent
				if iSec != 0:
					iRefX1 = map.getGridWidth() - iParallelLength
					iRefY1 = 2
					iRefX2 = map.getGridWidth() - 3
					iRefY2 = 2
					iRefX3 = map.getGridWidth() - 3
					iRefY3 = 4 + iParallelHeight
					iRefX4 = map.getGridWidth() - (iParallelLength - iParallelOffset)
					iRefY4 = 4 + iParallelHeight

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = map.getGridWidth() - 3
					iRefY1 = 2
					iRefX2 = map.getGridWidth() - (iParallelHeight)
					iRefY2 = 2
					iRefX3 = map.getGridWidth() - (iParallelHeight)
					iRefY3 = (iParallelLength - iParallelOffset)
					iRefX4 = map.getGridWidth() - 3
					iRefY4 = iParallelLength

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = map.getGridWidth() - 5
					iRefY1 = 2
					iRefX2 = map.getGridWidth() - 3
					iRefY2 = 2
					iRefX3 = map.getGridWidth() - 3
					iRefY3 = 4 + iParallelHeight
					iRefX4 = map.getGridWidth() - 5
					iRefY4 = 4 + iParallelHeight

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = map.getGridWidth() - 3
					iRefY1 = 2
					iRefX2 = map.getGridWidth() - (iParallelHeight)
					iRefY2 = 2
					iRefX3 = map.getGridWidth() - (iParallelHeight)
					iRefY3 = 4
					iRefX4 = map.getGridWidth() - 3
					iRefY4 = 4

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

		else:
			iQuadW = (map.getGridWidth())/2
			iTRCentreX = map.getGridWidth()/2 + 1
			iTRCentreY = map.getGridWidth()/2 + 1

			iPrim = CyGame().getSorenRandNum(4, "Picking Primary Quad")
			iSec = CyGame().getSorenRandNum(4, "Picking Secondary Quad")

			if iPeirceType == 0: # Pangaea
				iParallelLength = iQuadW/2
				iParallelOffset = iQuadW/4
				iParallelHeight = iQuadW/2 - 4
				iRectWidth = iQuadW/2
				iRectLength = iQuadW/2 - 3

	# Top Right
				if iPrim != 0:
					iRefX1 = iTRCentreX
					iRefY1 = iTRCentreY + 3
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX1,iRefY1,iParallelLength,iParallelHeight,-iParallelOffset,0) # North

					iRefX1 = iTRCentreX + 3
					iRefY1 = iTRCentreY
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX1,iRefY1,iParallelLength,iParallelHeight,-iParallelOffset,1) # North

					iRefX = iTRCentreX + iQuadW/2
					iRefY = iTRCentreY + iQuadW/2
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX, iRefY, iRectWidth, iRectLength, 0) # South
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX, iRefY, iRectWidth, iRectLength, 1) # South

					iWidth2 = iQuadW/2
					iRefX2 = iTRCentreX + iQuadW/4
					iRefY2 = iTRCentreY + iQuadW/4
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Central

					iDisplace = CyGame().getSorenRandNum(iQuadW/6, "Displacement of part of wrapped continent")
					iWidth2 = (iQuadW/4)
					iRefX2 = 0
					iRefY2 = iTRCentreY + iQuadW/2 + iDisplace
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Top Left Quad
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Top Left Quad
					iRectX = map.getGridWidth() - 2
					self.generatePlotsRectContinent(iBaseSeaLevel, iRectX, iRefY2, 2, iWidth2, 1)
					self.generatePlotsRectContinent(iBaseSeaLevel, iRectX, iRefY2, 2, iWidth2, 1)

					iDisplace = CyGame().getSorenRandNum(iQuadW/6, "Displacement of part of wrapped continent")
					iWidth2 = (iQuadW/4)
					iRefX2 = iTRCentreX + iQuadW/2 + iDisplace
					iRefY2 = 0
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Bottom Right Quad
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Bottom Right Quad
					iRectY = map.getGridWidth() - 2
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX2, iRectY, 2, iWidth2, 0)
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX2, iRectY, 2, iWidth2, 0)

	# Bottom Left
				if iPrim != 1:
					iRefX1 = iTRCentreX - iQuadW/4
					iRefY1 = iTRCentreY - iQuadW/2
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX1,iRefY1,iParallelLength,iParallelHeight,-iParallelOffset,0) # North

					iRefX1 = iTRCentreX - iQuadW/2
					iRefY1 = iTRCentreY - iQuadW/4
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX1,iRefY1,iParallelLength,iParallelHeight,-iParallelOffset,1) # North

					iRefX = 0
					iRefY = 2
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX, iRefY, iRectWidth, iRectLength, 0) # South
					iRefX = 2
					iRefY = 0
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX, iRefY, iRectWidth, iRectLength, 1) # South

					iWidth2 = iQuadW/2
					iRefX2 = iQuadW/4
					iRefY2 = iQuadW/4
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Central

					iDisplace = CyGame().getSorenRandNum(iQuadW/6, "Displacement of part of wrapped continent")
					iWidth2 = (iQuadW/4)
					iRefX2 = map.getGridWidth() - iWidth2
					iRefY2 = iDisplace
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Bottom Right Quad
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Bottom Right Quad
					iRectX = 0
					self.generatePlotsRectContinent(iBaseSeaLevel, iRectX, iRefY2, 2, iWidth2, 1)
					self.generatePlotsRectContinent(iBaseSeaLevel, iRectX, iRefY2, 2, iWidth2, 1)

					iDisplace = CyGame().getSorenRandNum(iQuadW/6, "Displacement of part of wrapped continent")
					iWidth2 = (iQuadW/4)
					iRefX2 = iDisplace
					iRefY2 = map.getGridWidth() - iWidth2
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Top Left Quad
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Top Left Quad
					iRectY = 0
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX2, iRectY, 2, iWidth2, 0)
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX2, iRectY, 2, iWidth2, 0)

	# Top Left
				if iPrim != 2:
					iRefX1 = iTRCentreX - iQuadW/2
					iRefY1 = iTRCentreY + 3
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX1,iRefY1,iParallelLength,iParallelHeight,iParallelOffset,0) # North

					iRefX1 = iTRCentreX - iQuadW/2
					iRefY1 = iTRCentreY - iQuadW/4
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX1,iRefY1,iParallelLength,iParallelHeight,iParallelOffset,1) # North

					iRefX = 0
					iRefY = iTRCentreY + iQuadW/2
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX, iRefY, iRectWidth, iRectLength, 0) # South
					iRefX = 2
					iRefY = iTRCentreY + iQuadW/2
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX, iRefY, iRectWidth, iRectLength, 1) # South

					iWidth2 = iQuadW/2
					iRefX2 = iTRCentreX - 3*iQuadW/4
					iRefY2 = iTRCentreY + iQuadW/4
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Central

					iDisplace = CyGame().getSorenRandNum(iQuadW/6, "Displacement of part of wrapped continent")
					iWidth2 = (iQuadW/4)
					iRefX2 = map.getGridWidth() - iWidth2
					iRefY2 = iTRCentreY + iQuadW/2 + iDisplace
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Top Right Quad
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Top Right Quad
					iRectX = 0
					self.generatePlotsRectContinent(iBaseSeaLevel, iRectX, iRefY2, 2, iWidth2, 1)
					self.generatePlotsRectContinent(iBaseSeaLevel, iRectX, iRefY2, 2, iWidth2, 1)

					iDisplace = CyGame().getSorenRandNum(iQuadW/6, "Displacement of part of wrapped continent")
					iWidth2 = (iQuadW/4)
					iRefX2 = iDisplace
					iRefY2 = 0
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Bottom Left Quad
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Bottom Left Quad
					iRectY = map.getGridWidth() - 2
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX2, iRectY, 2, iWidth2, 0)
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX2, iRectY, 2, iWidth2, 0)

	# Bottom Right
				if iPrim != 3:
					iRefX1 = iTRCentreX - iQuadW/4
					iRefY1 = iTRCentreY - iQuadW/2
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX1,iRefY1,iParallelLength,iParallelHeight,iParallelOffset,0) # North

					iRefX1 = iTRCentreX + 3
					iRefY1 = iTRCentreY - iQuadW/2
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX1,iRefY1,iParallelLength,iParallelHeight,iParallelOffset,1) # North

					iRefX = iTRCentreX + iQuadW/2
					iRefY = 2
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX, iRefY, iRectWidth, iRectLength, 0) # South
					iRefX = iTRCentreX + iQuadW/2
					iRefY = 0
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX, iRefY, iRectWidth, iRectLength, 1) # South

					iWidth2 = iQuadW/2
					iRefX2 = iTRCentreX + iQuadW/4
					iRefY2 = iQuadW/4 + 1
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Central

					iDisplace = CyGame().getSorenRandNum(iQuadW/6, "Displacement of part of wrapped continent")
					iWidth2 = (iQuadW/4)
					iRefX2 = 0
					iRefY2 = iDisplace
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Bottom Left Quad
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Bottom Left Quad
					iRectX = map.getGridWidth() - 2
					self.generatePlotsRectContinent(iBaseSeaLevel, iRectX, iRefY2, 2, iWidth2, 1)
					self.generatePlotsRectContinent(iBaseSeaLevel, iRectX, iRefY2, 2, iWidth2, 1)

					iDisplace = CyGame().getSorenRandNum(iQuadW/6, "Displacement of part of wrapped continent")
					iWidth2 = (iQuadW/4)
					iRefX2 = iTRCentreX + iQuadW/2 + iDisplace
					iRefY2 = map.getGridWidth() - 2
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Top Right Quad
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Top Right Quad
					iRectY = 0
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX2, iRectY, 2, iWidth2, 0)
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX2, iRectY, 2, iWidth2, 0)

			elif iPeirceType == 1 or iPeirceType == 2 or iPeirceType == 3: # 2 N-S
				if iPrim == 0 or iPrim == 2: # Top Right and Bottom Left
			# Primary Continent
					iWidth1 = (iQuadW/2)-3
					iWidth2 = (iQuadW/2)
					iWidth3 = (iQuadW/2)-2
					iRefX1 = iTRCentreX + 1
					iRefX2 = iTRCentreX + (iQuadW/4)
					iRefX3 = iTRCentreX +(iQuadW/2) +1
					iRefY1 = iTRCentreY
					iRefY2 = iTRCentreY + (iQuadW/4) + 1
					iRefY3 = iTRCentreY + (iQuadW/2) + 1

					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX1,iRefY1,iWidth1)
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2)
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX3,iRefY3,iWidth3)
			# Secondary (Opposite) Continent
					iWidth1 = (iQuadW/2)-3
					iWidth2 = (iQuadW/2)
					iWidth3 = (iQuadW/2)-2
					iRefX1 = iTRCentreX - (iQuadW/2)
					iRefX2 = iTRCentreX - (3* iQuadW/4) -1
					iRefX3 = iTRCentreX - (iQuadW) - 1
					iRefY1 = iTRCentreY - (iQuadW/2)
					iRefY2 = iTRCentreY - (3* iQuadW/4) -1
					iRefY3 = iTRCentreY - (iQuadW) - 1

					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX1,iRefY1,iWidth1)
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # This is to be replaced with the diagonal continent
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX3,iRefY3,iWidth3)
				else: # Top Left and Bottom Right
			# Primary Continent
					iWidth1 = (iQuadW/2) -3
					iWidth2 = (iQuadW/2)
					iWidth3 = (iQuadW/2)-2
					iRefX1 = iTRCentreX - (iQuadW/2)
					iRefX2 = iTRCentreX - (3* iQuadW/4) -1
					iRefX3 = iTRCentreX - (iQuadW) - 1
					iRefY1 = iTRCentreY
					iRefY2 = iTRCentreY + (iQuadW/4) + 1
					iRefY3 = iTRCentreY + (iQuadW/2) + 1

					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX1,iRefY1,iWidth1)
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2)
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX3,iRefY3,iWidth3)
			# Secondary (Opposite) Continent
					iWidth1 = (iQuadW/2)-3
					iWidth2 = (iQuadW/2)
					iWidth3 = (iQuadW/2)-2
					iRefX1 = iTRCentreX + 1
					iRefX2 = iTRCentreX + (iQuadW/4)
					iRefX3 = iTRCentreX + (iQuadW/2) + 1
					iRefY1 = iTRCentreY - (iQuadW/2)
					iRefY2 = iTRCentreY - (3* iQuadW/4) -1
					iRefY3 = iTRCentreY - (iQuadW) - 1

					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX1,iRefY1,iWidth1)
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # This is to be replaced with the diagonal continent
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX3,iRefY3,iWidth3)

	# Third and fourth continents
				if iPeirceType == 2 or iPeirceType == 3:
	# TR
					if (iPrim == 1 or iPrim == 3):
						if iSec < 2 or iPeirceType == 3:
							iWidth1 = (iQuadW/2)-5
							iWidth2 = (iQuadW/2)-3
							iWidth3 = (iQuadW/2)-4
							iRefX1 = iTRCentreX + 3
							iRefX2 = iTRCentreX + (iQuadW/2) - 2
							iRefX3 = iTRCentreX + (iQuadW/4) + 2
							iRefY1 = iTRCentreY + 3
							iRefY2 = iTRCentreY + (iQuadW/2) - 2
							iRefY3 = iTRCentreY + (iQuadW/4) + 2

							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX1,iRefY1,iWidth1)
							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2)
							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX3,iRefY3,iWidth3)
	# BL
						if iSec >= 2 or iPeirceType == 3:
							iWidth1 = (iQuadW/2)-5
							iWidth2 = (iQuadW/2)-3
							iWidth3 = (iQuadW/2)-4
							iRefX1 = iTRCentreX - (iQuadW/2) + 1
							iRefX2 = iTRCentreX - (iQuadW) + 2
							iRefX3 = iTRCentreX - (iQuadW/2) - 4
							iRefY1 = iTRCentreY - (iQuadW/2) + 1
							iRefY2 = iTRCentreY - (iQuadW) + 2
							iRefY3 = iTRCentreY - (iQuadW/2) - 4

							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX1,iRefY1,iWidth1)
							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2)
							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX3,iRefY3,iWidth3)

					else:
	#TL
						if iSec < 2 or iPeirceType == 3:
							iWidth1 = (iQuadW/2)-5
							iWidth2 = (iQuadW/2)-3
							iWidth3 = (iQuadW/2)-4
							iRefX1 = iTRCentreX - (iQuadW/2) + 1
							iRefX2 = iTRCentreX - (iQuadW) + 2
							iRefX3 = iTRCentreX - (iQuadW/2) - 4
							iRefY1 = iTRCentreY + 3
							iRefY2 = iTRCentreY + (iQuadW/2) - 2
							iRefY3 = iTRCentreY + (iQuadW/2) + 2

							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX1,iRefY1,iWidth1)
							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2)
							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX3,iRefY3,iWidth3)
	#BR
						if iSec >= 2 or iPeirceType == 3:
							iWidth1 = (iQuadW/2)-5
							iWidth2 = (iQuadW/2)-3
							iWidth3 = (iQuadW/2)-4
							iRefX1 = iTRCentreX + 3
							iRefX2 = iTRCentreX + (iQuadW/2) - 2
							iRefX3 = iTRCentreX + (iQuadW/4) + 2
							iRefY1 = iTRCentreY - (iQuadW/2) + 1
							iRefY2 = iTRCentreY - (iQuadW) + 2
							iRefY3 = iTRCentreY - (iQuadW/4) - 4

							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX1,iRefY1,iWidth1)
							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2)
							self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX3,iRefY3,iWidth3)

			elif iPeirceType == 4: # E-W 
				iParallelLength = 3*iQuadW/4 - 5
				iParallelOffset = iQuadW/4
				iParallelHeight = iQuadW/2 - 6

	# <!-- custom: Apply the KI#255 second-anchor repair to the original script's duplicated E-W implementation for the other wrap mode too. See KI#255. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	# Top Right North Continent
				if iPrim != 0:
					iRefX1 = iTRCentreX - 1
					iRefY1 = iTRCentreY + 2
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX1,iRefY1,iParallelLength,iParallelHeight,-iParallelOffset,0)

					iRefX2 = iTRCentreX + 1 + iParallelHeight
					iRefY2 = iTRCentreY - iQuadW/4
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX2,iRefY2,iParallelLength,iParallelHeight,-iParallelOffset,1)

	# Bottom Left North Continent
				if iPrim != 1:
					iRefX1 = iTRCentreX - 4 - iParallelHeight
					iRefY1 = iTRCentreY - 4 - iParallelHeight
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX1,iRefY1,iParallelLength,iParallelHeight,-iParallelOffset,0)

					iRefX2 = iTRCentreX - 4
					iRefY2 = iTRCentreY - 2 - iParallelLength
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX2,iRefY2,iParallelLength,iParallelHeight,iParallelOffset,1)

	# Top Left North Continent
				if iPrim != 2:
					iRefX1 = iTRCentreX - 2 - iParallelLength
					iRefY1 = iTRCentreY + 2
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX1,iRefY1,iParallelLength,iParallelHeight,iParallelOffset,0)

					iRefX2 = iTRCentreX - 4
					iRefY2 = iTRCentreY - 1
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX2,iRefY2,iParallelLength,iParallelHeight,-iParallelOffset,1)

	# Bottom Right North Continent
				if iPrim != 3:
					iRefX1 = iTRCentreX - 1 - iParallelOffset
					iRefY1 = iTRCentreY - 4 - iParallelHeight
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX1,iRefY1,iParallelLength,iParallelHeight,iParallelOffset,0)

					iRefX2 = iTRCentreX + 2 + iParallelHeight
					iRefY2 = iTRCentreY - 1 - iParallelLength
					self.generatePlotsParallelContinent(iBaseSeaLevel, iRefX2,iRefY2,iParallelLength,iParallelHeight,-iParallelOffset,1)

	# Top Right South Continent
				if iSec != 0:
					iRefX1 = map.getGridWidth() - iParallelLength
					iRefY1 = map.getGridWidth() - 1
					iRefX2 = map.getGridWidth() - 1
					iRefY2 = map.getGridWidth() - 1
					iRefX3 = map.getGridWidth() - 1
					iRefY3 = map.getGridWidth() - iParallelHeight
					iRefX4 = map.getGridWidth() - iParallelLength + iParallelOffset
					iRefY4 = map.getGridWidth() - iParallelHeight

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = map.getGridWidth() - 1
					iRefY1 = map.getGridWidth() - 1
					iRefX2 = map.getGridWidth() - iParallelHeight
					iRefY2 = map.getGridWidth() - 1
					iRefX3 = map.getGridWidth() - iParallelHeight
					iRefY3 = map.getGridWidth() + 1 - (iParallelLength - iParallelOffset)
					iRefX4 = map.getGridWidth() - 3
					iRefY4 = map.getGridWidth() + 1 - iParallelLength

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)
			# 
					iRefX1 = map.getGridWidth() - 3
					iRefY1 = map.getGridWidth() - 1
					iRefX2 = map.getGridWidth() - 1
					iRefY2 = map.getGridWidth() - 1
					iRefX3 = map.getGridWidth() - 1
					iRefY3 = map.getGridWidth() - iParallelHeight
					iRefX4 = map.getGridWidth() - 3
					iRefY4 = map.getGridWidth() - iParallelHeight

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = map.getGridWidth() - 1
					iRefY1 = map.getGridWidth() - 1
					iRefX2 = map.getGridWidth() - iParallelHeight
					iRefY2 = map.getGridWidth() - 1
					iRefX3 = map.getGridWidth() - iParallelHeight
					iRefY3 = map.getGridWidth() - 3
					iRefX4 = map.getGridWidth() - 1
					iRefY4 = map.getGridWidth() - 3

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

# added
					iDisplace = CyGame().getSorenRandNum(iQuadW/6, "Displacement of part of wrapped continent")
					iWidth2 = (iQuadW/4)
					iRefX2 = 0
					iRefY2 = iTRCentreY + iQuadW/2 + iDisplace
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Top Left Quad
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Top Left Quad
					iRectX = map.getGridWidth() - 2
					self.generatePlotsRectContinent(iBaseSeaLevel, iRectX, iRefY2, 2, iWidth2, 1)
					self.generatePlotsRectContinent(iBaseSeaLevel, iRectX, iRefY2, 2, iWidth2, 1)

					iDisplace = CyGame().getSorenRandNum(iQuadW/6, "Displacement of part of wrapped continent")
					iWidth2 = (iQuadW/4)
					iRefX2 = iTRCentreX + iQuadW/2 + iDisplace
					iRefY2 = 0
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Bottom Right Quad
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Bottom Right Quad
					iRectY = map.getGridWidth() - 2
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX2, iRectY, 2, iWidth2, 0)
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX2, iRectY, 2, iWidth2, 0)

	# Top Left South Continent
				if iSec != 1:
					iRefX1 = 2
					iRefY1 = map.getGridWidth() - 4
					iRefX2 = iParallelLength
					iRefY2 = map.getGridWidth() - 4
					iRefX3 = iParallelLength - iParallelOffset
					iRefY3 = map.getGridWidth() - 3 - iParallelHeight
					iRefX4 = 2
					iRefY4 = map.getGridWidth() - 3 - iParallelHeight

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = 3
					iRefY1 = map.getGridWidth() - 1
					iRefX2 = 2 + iParallelHeight
					iRefY2 = map.getGridWidth() - 1
					iRefX3 = 2 + iParallelHeight
					iRefY3 = map.getGridWidth() + 1 - (iParallelLength - iParallelOffset)
					iRefX4 = 3
					iRefY4 = map.getGridWidth() + 1 - iParallelLength

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = 0
					iRefY1 = map.getGridWidth() - 4
					iRefX2 = 2
					iRefY2 = map.getGridWidth() - 4
					iRefX3 = 2
					iRefY3 = map.getGridWidth() - 3 - iParallelHeight
					iRefX4 = 0
					iRefY4 = map.getGridWidth() - 3 - iParallelHeight

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = 3
					iRefY1 = map.getGridWidth() - 1
					iRefX2 = 2 + iParallelHeight
					iRefY2 = map.getGridWidth() - 1
					iRefX3 = 2 + iParallelHeight
					iRefY3 = map.getGridWidth() - 3
					iRefX4 = 3
					iRefY4 = map.getGridWidth() - 3

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iDisplace = CyGame().getSorenRandNum(iQuadW/6, "Displacement of part of wrapped continent")
					iWidth2 = (iQuadW/4)
					iRefX2 = map.getGridWidth() - iWidth2
					iRefY2 = iTRCentreY + iQuadW/2 + iDisplace
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Top Right Quad
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Top Right Quad
					iRectX = 0
					self.generatePlotsRectContinent(iBaseSeaLevel, iRectX, iRefY2, 2, iWidth2, 1)
					self.generatePlotsRectContinent(iBaseSeaLevel, iRectX, iRefY2, 2, iWidth2, 1)

					iDisplace = CyGame().getSorenRandNum(iQuadW/6, "Displacement of part of wrapped continent")
					iWidth2 = (iQuadW/4)
					iRefX2 = iDisplace
					iRefY2 = 0
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Bottom Left Quad
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Bottom Left Quad
					iRectY = map.getGridWidth() - 2
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX2, iRectY, 2, iWidth2, 0)
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX2, iRectY, 2, iWidth2, 0)

	# Bottom Left South Continent
				if iSec != 3:
					iRefX1 = 0
					iRefY1 = 2
					iRefX2 = iParallelLength
					iRefY2 = 2
					iRefX3 = iParallelLength - iParallelOffset
					iRefY3 = 2 + iParallelHeight
					iRefX4 = 0
					iRefY4 = 2 + iParallelHeight

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = 3
					iRefY1 = 0
					iRefX2 = 2 + iParallelHeight
					iRefY2 = 0
					iRefX3 = 2 + iParallelHeight
					iRefY3 = iParallelLength - iParallelOffset
					iRefX4 = 3
					iRefY4 = iParallelLength

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = 0
					iRefY1 = 2
					iRefX2 = 2
					iRefY2 = 2
					iRefX3 = 2
					iRefY3 = 2 + iParallelHeight
					iRefX4 = 0
					iRefY4 = 2 + iParallelHeight

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = 3
					iRefY1 = 0
					iRefX2 = 2 + iParallelHeight
					iRefY2 = 0
					iRefX3 = 2 + iParallelHeight
					iRefY3 = 2
					iRefX4 = 3
					iRefY4 = 2

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iDisplace = CyGame().getSorenRandNum(iQuadW/6, "Displacement of part of wrapped continent")
					iWidth2 = (iQuadW/4)
					iRefX2 = map.getGridWidth() - iWidth2
					iRefY2 = iDisplace
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Bottom Right Quad
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Bottom Right Quad
					iRectX = 0
					self.generatePlotsRectContinent(iBaseSeaLevel, iRectX, iRefY2, 2, iWidth2, 1)
					self.generatePlotsRectContinent(iBaseSeaLevel, iRectX, iRefY2, 2, iWidth2, 1)

					iDisplace = CyGame().getSorenRandNum(iQuadW/6, "Displacement of part of wrapped continent")
					iWidth2 = (iQuadW/4)
					iRefX2 = iDisplace
					iRefY2 = map.getGridWidth() - iWidth2
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Top Left Quad
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Top Left Quad
					iRectY = 0
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX2, iRectY, 2, iWidth2, 0)
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX2, iRectY, 2, iWidth2, 0)

	# Bottom Right South Continent
				if iSec != 0:
					iRefX1 = map.getGridWidth() - iParallelLength
					iRefY1 = 0
					iRefX2 = map.getGridWidth() - 1
					iRefY2 = 0
					iRefX3 = map.getGridWidth() - 1
					iRefY3 = 2 + iParallelHeight
					iRefX4 = map.getGridWidth() - (iParallelLength - iParallelOffset)
					iRefY4 = 2 + iParallelHeight

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = map.getGridWidth() - 1
					iRefY1 = 0
					iRefX2 = map.getGridWidth() - (iParallelHeight)
					iRefY2 = 0
					iRefX3 = map.getGridWidth() - (iParallelHeight)
					iRefY3 = (iParallelLength - iParallelOffset)
					iRefX4 = map.getGridWidth() - 1
					iRefY4 = iParallelLength

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = map.getGridWidth() - 3
					iRefY1 = 0
					iRefX2 = map.getGridWidth() - 1
					iRefY2 = 0
					iRefX3 = map.getGridWidth() - 1
					iRefY3 = 2 + iParallelHeight
					iRefX4 = map.getGridWidth() - 3
					iRefY4 = 2 + iParallelHeight

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iRefX1 = map.getGridWidth() - 1
					iRefY1 = 0
					iRefX2 = map.getGridWidth() - (iParallelHeight)
					iRefY2 = 0
					iRefX3 = map.getGridWidth() - (iParallelHeight)
					iRefY3 = 2
					iRefX4 = map.getGridWidth() - 1
					iRefY4 = 2

					self.generatePlotsPoly4Continent(iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4)

					iDisplace = CyGame().getSorenRandNum(iQuadW/6, "Displacement of part of wrapped continent")
					iWidth2 = (iQuadW/4)
					iRefX2 = 0
					iRefY2 = iDisplace
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Bottom Left Quad
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Bottom Left Quad
					iRectX = map.getGridWidth() - 2
					self.generatePlotsRectContinent(iBaseSeaLevel, iRectX, iRefY2, 2, iWidth2, 1)
					self.generatePlotsRectContinent(iBaseSeaLevel, iRectX, iRefY2, 2, iWidth2, 1)

					iDisplace = CyGame().getSorenRandNum(iQuadW/6, "Displacement of part of wrapped continent")
					iWidth2 = (iQuadW/4)
					iRefX2 = iTRCentreX + iQuadW/2 + iDisplace
					iRefY2 = map.getGridWidth() - 2
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Top Right Quad
					self.generatePlotsSquareContinent(iBaseSeaLevel, iRefX2,iRefY2,iWidth2) # Into Top Right Quad
					iRectY = 0
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX2, iRectY, 2, iWidth2, 0)
					self.generatePlotsRectContinent(iBaseSeaLevel, iRefX2, iRectY, 2, iWidth2, 0)

		return self.wholeworldPlotTypes

	def generatePlotsSquareContinent(self, iBaseSeaLevel, iRefX,iRefY,iWidth):
		iSquareContinentGrain = 2
		iSquareContinentHillsGrain = 4
		iQuadW = (map.getGridWidth()-4)/2

		lSquareContinentPolygon = [
			[iRefX + 1, iRefY + 1],
			[iRefX + 1, iRefY + iWidth],
			[iRefX + iWidth, iRefY + iWidth],
			[iRefX + iWidth, iRefY + 1],
		]

		# Create the map area polygon.
		SquareContinentMapArea = MapAreaPolygon("Square Continent", lSquareContinentPolygon, 0)

		# Add the area to the list of regions in which civilizations can start.
		global lStartingPlotAreas
		lStartingPlotAreas.append(SquareContinentMapArea)

		self.generatePlotsInMapAreaPolygon(iBaseSeaLevel - 15, SquareContinentMapArea, iSquareContinentGrain, iSquareContinentHillsGrain, self.iRoundFlags, self.iTerrainFlags, CyFractal.FracVals.DEFAULT_FRAC_Y_EXP, CyFractal.FracVals.DEFAULT_FRAC_Y_EXP)

		return

	def generatePlotsRectContinent(self, iBaseSeaLevel, iRefX, iRefY, iWidth, iLength, bPortrait):
		iRectContinentGrain = 2
		iRectContinentHillsGrain = 4
		iQuadW = (map.getGridWidth()-4)/2

		if bPortrait:
			lRectContinentPolygon = [
				[iRefX + 1, iRefY + 1],
				[iRefX + 1, iRefY + iLength],
				[iRefX + iWidth, iRefY + iLength],
				[iRefX + iWidth, iRefY + 1],
			]
		else:
			lRectContinentPolygon = [
				[iRefX + 1, iRefY + 1],
				[iRefX + 1, iRefY + iWidth],
				[iRefX + iLength, iRefY + iWidth],
				[iRefX + iLength, iRefY + 1],
			]

		# Create the map area polygon.
		RectContinentMapArea = MapAreaPolygon("Rect Continent", lRectContinentPolygon, 0)

		# Add the area to the list of regions in which civilizations can start.
		global lStartingPlotAreas
		lStartingPlotAreas.append(RectContinentMapArea)

		self.generatePlotsInMapAreaPolygon(iBaseSeaLevel - 15, RectContinentMapArea, iRectContinentGrain, iRectContinentHillsGrain, self.iRoundFlags, self.iTerrainFlags, CyFractal.FracVals.DEFAULT_FRAC_Y_EXP, CyFractal.FracVals.DEFAULT_FRAC_Y_EXP)

		return

	def generatePlotsParallelContinent(self, iBaseSeaLevel, iRefX, iRefY, iLength, iHeight, iOffset, bPortrait):
	# Rem adding offset is first action and goes in clockwise direction
		iParallelContinentGrain = 2
		iParallelContinentHillsGrain = 4
		iQuadW = (map.getGridWidth()-4)/2

		if bPortrait:
			lParallelContinentPolygon = [
				[iRefX + 1, iRefY + 1],
				[iRefX + iHeight, iRefY + iOffset],
				[iRefX + iHeight, iRefY + iLength + iOffset],
				[iRefX + 1, iRefY + iLength],
			]
		else:
			lParallelContinentPolygon = [
				[iRefX + 1, iRefY + 1],
				[iRefX + 1 + iOffset, iRefY + iHeight],
				[iRefX + iLength + iOffset, iRefY + iHeight],
				[iRefX + iLength, iRefY+1],
			]

		# Create the map area polygon.
		ParallelContinentMapArea = MapAreaPolygon("Parallel Continent", lParallelContinentPolygon, 0)

		# Add the area to the list of regions in which civilizations can start.
		global lStartingPlotAreas
		lStartingPlotAreas.append(ParallelContinentMapArea)

		self.generatePlotsInMapAreaPolygon(iBaseSeaLevel - 15, ParallelContinentMapArea, iParallelContinentGrain, iParallelContinentHillsGrain, self.iRoundFlags, self.iTerrainFlags, CyFractal.FracVals.DEFAULT_FRAC_Y_EXP, CyFractal.FracVals.DEFAULT_FRAC_Y_EXP)

		return

	def generatePlotsPoly4Continent(self, iBaseSeaLevel, iRefX1,iRefY1, iRefX2,iRefY2, iRefX3,iRefY3, iRefX4,iRefY4):
		iPoly4ContinentGrain = 2
		iPoly4ContinentHillsGrain = 4
		iQuadW = (map.getGridWidth()-4)/2

		lPoly4ContinentPolygon = [
			[iRefX1, iRefY1],
			[iRefX2, iRefY2],
			[iRefX3, iRefY3],
			[iRefX4, iRefY4],
		]

		# Create the map area polygon.
		Poly4ContinentMapArea = MapAreaPolygon("Poly4 Continent", lPoly4ContinentPolygon, 0)

		# Add the area to the list of regions in which civilizations can start.
		global lStartingPlotAreas
		lStartingPlotAreas.append(Poly4ContinentMapArea)

		self.generatePlotsInMapAreaPolygon(iBaseSeaLevel - 15, Poly4ContinentMapArea, iPoly4ContinentGrain, iPoly4ContinentHillsGrain, self.iRoundFlags, self.iTerrainFlags, CyFractal.FracVals.DEFAULT_FRAC_Y_EXP, CyFractal.FracVals.DEFAULT_FRAC_Y_EXP)

		return

	def generatePlotsInMapAreaPolygon(self, iWaterPercent, mapArea, iRegionGrain, iRegionHillsGrain, iRegionPlotFlags, iRegionTerrainFlags, iRegionFracXExp = -1, iRegionFracYExp = -1):
		#
		# Generate plots in a region that is not a rectangle, but an arbitrary polygon. See MapAreaPolygon for details.
		# :param iWaterPercent:
		# :param mapArea: Polygonal shape inside which the region will be created.
		# :type mapArea: MapAreaPolygon
		# :param iRegionGrain: Fractal grain used for generating the terrain.
		# :param iRegionHillsGrain: Fractal grain used for generating hills and peaks.
		# :param iRegionPlotFlags: Flags used for the plot fractal.
		# :param iRegionTerrainFlags: Flags used for the hills and peaks fractals.
		# :param iRegionFracXExp:
		# :param iRegionFracYExp:
		# :return:
		#
		# Obtain size and position from the map area.
		iRegionWidth = mapArea.iRegionWidth
		iRegionHeight = mapArea.iRegionHeight
		fMinX = mapArea.fMinX
		fMinY = mapArea.fMinY

		# Init the plot types array and the regional fractals
		self.plotTypes = [] # reinit the array for each pass
		self.plotTypes = [PlotTypes.PLOT_OCEAN] * (iRegionWidth * iRegionHeight)
		regionContinentsFrac = CyFractal()
		regionHillsFrac = CyFractal()
		regionPeaksFrac = CyFractal()
		regionContinentsFrac.fracInit(iRegionWidth, iRegionHeight, iRegionGrain, self.dice, iRegionPlotFlags, iRegionFracXExp, iRegionFracYExp)
		regionHillsFrac.fracInit(iRegionWidth, iRegionHeight, iRegionHillsGrain, self.dice, iRegionTerrainFlags, iRegionFracXExp, iRegionFracYExp)
		regionPeaksFrac.fracInit(iRegionWidth, iRegionHeight, iRegionHillsGrain+1, self.dice, iRegionTerrainFlags, iRegionFracXExp, iRegionFracYExp)

		iWaterThreshold = regionContinentsFrac.getHeightFromPercent(iWaterPercent)
		iHillsBottom1 = regionHillsFrac.getHeightFromPercent(max((25 - self.gc.getClimateInfo(self.map.getClimate()).getHillRange()), 0))
		iHillsTop1 = regionHillsFrac.getHeightFromPercent(min((25 + self.gc.getClimateInfo(self.map.getClimate()).getHillRange()), 100))
		iHillsBottom2 = regionHillsFrac.getHeightFromPercent(max((75 - self.gc.getClimateInfo(self.map.getClimate()).getHillRange()), 0))
		iHillsTop2 = regionHillsFrac.getHeightFromPercent(min((75 + self.gc.getClimateInfo(self.map.getClimate()).getHillRange()), 100))
		iPeakThreshold = regionPeaksFrac.getHeightFromPercent(self.gc.getClimateInfo(self.map.getClimate()).getPeakPercent())

		# Loop through the region's plots
		for iRegionX in range(iRegionWidth):
			for iRegionY in range(iRegionHeight):
				val = regionContinentsFrac.getHeight(iRegionX, iRegionY)
				if val <= iWaterThreshold:
					pass
				else:
					# Checking if the plot is inside the polygon is expensive, so it is done here at the last possible
					# chance.
					if mapArea.isInside(iRegionX + fMinX, iRegionY + fMinY):
						iPlotIndex = iRegionY * iRegionWidth + iRegionX
						hillVal = regionHillsFrac.getHeight(iRegionX, iRegionY)
						if hillVal >= iHillsBottom1 and hillVal <= iHillsTop1 or hillVal >= iHillsBottom2 and hillVal <= iHillsTop2:
							peakVal = regionPeaksFrac.getHeight(iRegionX, iRegionY)
							if peakVal <= iPeakThreshold:
								self.plotTypes[iPlotIndex] = PlotTypes.PLOT_PEAK
							else:
								self.plotTypes[iPlotIndex] = PlotTypes.PLOT_HILLS
						else:
							self.plotTypes[iPlotIndex] = PlotTypes.PLOT_LAND

		# Apply the region's plots to the global plot array.
		for iRegionX in range(iRegionWidth):
			iWholeworldX = int(iRegionX + fMinX)
			if iWholeworldX < 0 or iWholeworldX > (self.iW - 1):
				continue
			for iRegionY in range(iRegionHeight):
				iWholeworldY = int(iRegionY + fMinY)
				if iWholeworldY < 0 or iWholeworldY > (self.iH - 1):
					continue
				iPlotIndex = iRegionY * iRegionWidth + iRegionX
				if self.plotTypes[iPlotIndex] == PlotTypes.PLOT_OCEAN:
					continue
				iWorld = iWholeworldY * self.iW + iWholeworldX
				self.wholeworldPlotTypes[iWorld] = self.plotTypes[iPlotIndex]

		# This region is done.
		return

class PeirceTerrainGenerator(CvMapGeneratorUtil.TerrainGenerator):
	# Terrain generator customized for Peirce map. This means creating terrain with latitude based on equator on diagonal linking midpoints of sides and poles in centre and in corners.

	def getLatitudeAtPlot(self, iX, iY):
		# Given a plot (iX,iY), returns a value between 0.0 (tropical) and 1.0 (polar). In
		# Peirce map, latitude based on equator on diagonal linking midpoints of sides and poles in centre and in corners.
		# :param iX: x coordinate of the plot
		# :param iY: y coordinate of the plot.
		# :return: Calculated latitude.
		#
		return getLimitedLatitude(iX, iY, terrainVarFractal)

class PeirceFeatureGenerator(CvMapGeneratorUtil.FeatureGenerator):
	# Feature generator customized for Peirce. This means placing features with latitude based on equator on diagonal linking midpoints of sides and poles in centre and in corners.

	def getLatitudeAtPlot(self, iX, iY):
		# Given a plot (iX,iY), returns a value between 0.0 (tropical) and 1.0 (polar). In
		# Peirce map, latitude based on equator on diagonal linking midpoints of sides and poles in centre and in corners.
		# :param iX: x coordinate of the plot.
		# :param iY: y coordinate of the plot.
		# :return: Calculated latitude.
		#
		return getLimitedLatitude(iX, iY, featuresVarFractal)

class MapAreaPolygon:
	# Class that defines a map area that can have any polygonal shape. Randomized distortion using both fractals and
	# coordinate changes is applied, to make sure that the final area shape is not too regular and unpredictable, while
	# still following roughly the desired shape.
	# Uses the PNPOLY algorithm. See: https://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html

	__DISPLACEMENT_FRACTAL_GRAIN = 2
	# Grain used for the displacement fractals.

	def __init__(self, sRegionName, lOriginalPolygonPoints, fAngle):
		# Initializes the polygonal map area.
		# :param lOriginalPolygonPoints: List of tuples that contain the x and y coordinates of each of the points.
		# :param fAngle: The polygon will be rotated by this angle.
		if len(lOriginalPolygonPoints) < 3:
			raise Exception("[Peirce] - " + sRegionName + " - A polygon must have at least three vertices.")

		# Rotate the polygon and apply random displacement.
		lPolygonPoints = list()

		self.__iRandomDisplacement = int(max(2.0, map.getGridWidth() / 12.0))
		fMiddleX = map.getGridWidth() / 2.0
		fMiddleY = map.getGridHeight() / 2.0
		fSinAngle = math.sin(fAngle)
		fCosAngle = math.cos(fAngle)

		for pX, pY in lOriginalPolygonPoints:
			pXInitial = pX - fMiddleX
			pYInitial = pY - fMiddleY
			pXRotated = pXInitial * fCosAngle - pYInitial * fSinAngle + fMiddleX + self.__getRandomDisplacement()
			pYRotated = pXInitial * fSinAngle + pYInitial * fCosAngle + fMiddleY + self.__getRandomDisplacement()
			lPolygonPoints.append([pXRotated, pYRotated])

		# Calculate the rest of the values that depend on the shape of the polygon.
		self.__fMinX = sys.maxint
		self.__fMinY = sys.maxint
		self.__fMaxX = -sys.maxint - 1
		self.__fMaxY = -sys.maxint - 1

		for pX, pY in lPolygonPoints:
			self.__fMinX = min(self.__fMinX, pX)
			self.__fMinY = min(self.__fMinY, pY)
			self.__fMaxX = max(self.__fMaxX, pX)
			self.__fMaxY = max(self.__fMaxY, pY)

		# Give room for fractal displacement.
		self.__fMinX -= 4.0
		self.__fMinY -= 4.0
		self.__fMaxX += 4.0
		self.__fMaxY += 4.0

		# Used for creating displacement fractals and initial isInside checks.
		self.__iRegionWidth = int(self.__fMaxX - self.__fMinX + 1)
		self.__iRegionHeight = int(self.__fMaxY - self.__fMinY + 1)

		# Perfect polygons are boring. These fractals are used to distort the shape of the resulting landmass slightly.
		self.__horizontalDisplacementFrac = CyFractal()
		self.__horizontalDisplacementFrac.fracInit(self.__iRegionWidth, self.__iRegionHeight, self.__DISPLACEMENT_FRACTAL_GRAIN, game.getMapRand(), CyFractal.FracVals.FRAC_POLAR, CyFractal.FracVals.DEFAULT_FRAC_Y_EXP, CyFractal.FracVals.DEFAULT_FRAC_Y_EXP)

		self.__verticalDisplacementFrac = CyFractal()
		self.__verticalDisplacementFrac.fracInit(self.__iRegionWidth, self.__iRegionHeight, self.__DISPLACEMENT_FRACTAL_GRAIN, game.getMapRand(), CyFractal.FracVals.FRAC_POLAR, CyFractal.FracVals.DEFAULT_FRAC_Y_EXP, CyFractal.FracVals.DEFAULT_FRAC_Y_EXP)

		# Since all points need to be accessed at least once, they can be calculated on init.
		self.__bInsideMatrix = [[False for iY in range(self.__iRegionHeight)] for iX in range(self.__iRegionWidth)]

		# PNPOLY algorithm for determining if a given plot is inside of the polygon or not.
		for iX in range(self.__iRegionWidth):
			for iY in range(self.__iRegionHeight):
				# Apply displacement values between -4.0 and 4.0.
				fHorizontalDisp = self.__horizontalDisplacementFrac.getHeight(iX, iY) / 32.0 - 4.0
				fVerticalDisp = self.__verticalDisplacementFrac.getHeight(iX, iY) / 32.0 - 4.0

				fRealX = self.__fMinX + iX + fHorizontalDisp
				fRealY = self.__fMinY + iY + fVerticalDisp

				iPoint = 0
				jPoint = len(lPolygonPoints) - 1
				bInside = False

				while iPoint < len(lPolygonPoints):
					lFirstPoint = lPolygonPoints[iPoint]
					lSecondPoint = lPolygonPoints[jPoint]
					if (lFirstPoint[1] > fRealY) != (lSecondPoint[1] > fRealY):
						fValue = float(lSecondPoint[0] - lFirstPoint[0])
						fValue *= fRealY - lFirstPoint[1]
						fValue /= lSecondPoint[1] - lFirstPoint[1]
						fValue += lFirstPoint[0]
						if fRealX < fValue:
							bInside = not bInside

					# Prepare the next pair of points.
					jPoint = iPoint
					iPoint += 1

				if bInside:
					self.__bInsideMatrix[iX][iY] = True

		# Uncommenting this code displays all regions in the log.
		# print "[Peirce] - " + sRegionName + " - MapAreaPolygon map area:"
		# for iY in range(self.__iRegionHeight - 1, -1, -1):
		# sLine = ""
		# for iX in range(self.__iRegionWidth):
		# if self.__bInsideMatrix[iX][iY]:
		# sLine += "#"
		# else:
		# sLine += " "
		# print sLine

	def __getRandomDisplacement(self):
		# Allows to apply a random displacement to one of the coordinates of one of the points of the polygon.
		# :return: Calculated displacement.
		return self.__iRandomDisplacement / 2 - game.getMapRand().get(self.__iRandomDisplacement, "[Peirce] - Randomization of the points of one of the areas.")

	@property
	def iRegionWidth(self):
		return self.__iRegionWidth

	@property
	def iRegionHeight(self):
		return self.__iRegionHeight

	@property
	def fMinX(self):
		return self.__fMinX

	@property
	def fMinY(self):
		return self.__fMinY

	def isInside(self, fX, fY):
		# Checks if the point (fX, fY) is inside of the polygon.
		# :param fX: x coordinate of the point.
		# :param fY: y coordinate of the point.
		# :return: True if the point is inside of the polygon, false otherwise.
		iRealX = int(fX - self.__fMinX)
		iRealY = int(fY - self.__fMinY)

		if iRealX < 0 or iRealX >= self.__iRegionWidth:
			return False
		if iRealY < 0 or iRealY >= self.__iRegionHeight:
			return False

		return self.__bInsideMatrix[iRealX][iRealY]

def getVariationFractal(iGrain):
	# Initializes a fractal that can be used to introduce random variations.
	# :return: New fractal.
	varFractal = CyFractal()
	iFlags = 0  # Disallow FRAC_POLAR flag, to prevent "zero row" problems.

	# The Peirce map has the same width and height, so both frac Y-exp args use DEFAULT_FRAC_Y_EXP
	varFractal.fracInit(map.getGridWidth(), map.getGridHeight(), iGrain, game.getMapRand(), iFlags, CyFractal.FracVals.DEFAULT_FRAC_Y_EXP, CyFractal.FracVals.DEFAULT_FRAC_Y_EXP)

	return varFractal

def getPeirceLatitude(iX, iY, varFractal=None):
	# Calculates an approximate distance from the point to the center of the disc.
	# :param iX: x coordinate of the plot
	# :param iY: y coordinate of the plot.
	# :param varFractal: Fractal used to introduce random variations in the calculated distance.
	# :return: Calculated distance.
	map = CyGlobalContext().getMap()
	iPeirceWrap = map.getCustomMapOption(1)
	if iPeirceWrap == 1: # No wrap
		fLatitude = 0.5
		iCentreX = map.getGridWidth()/2 - 1
		iCentreY = map.getGridWidth()/2 - 1
		iQuadW = (map.getGridWidth()-4)/2
		fQuadW = iQuadW * 1.0

		# Scaling to allow for increased size of cold poles
	#	iLatitudeAdjust = 100
	#	iPeircePoles = map.getCustomMapOption(1)
	#	if iPeircePoles == 1:
	#		iLatitudeAdjust = 130

	#lat = abs((self.iWidth / 2) - iX)/float(self.iWidth/2)
	# line used in Tilted Axis

		if iX >= iCentreX and iY >= iCentreY:
			fLatitude = abs((iX + iY) - (iCentreX + iCentreY + iQuadW))/fQuadW
			# Make sure the "out of playing area tiles match the appropriate latitude"
			if iX == map.getGridWidth() - 2:
				fLatitude = abs((iX + iY - 1) - (iCentreX + iCentreY + iQuadW))/fQuadW
			if iY == map.getGridWidth() - 2:
				fLatitude = abs((iX + iY - 1) - (iCentreX + iCentreY + iQuadW))/fQuadW
		elif iX < iCentreX and iY < iCentreY:
			fLatitude = abs((iX + iY) - (iCentreX + iCentreY - iQuadW))/fQuadW
			# Make sure the "out of playing area tiles match the appropriate latitude"
			if iX == 1:
				fLatitude = abs((iX + iY + 1) - (iCentreX + iCentreY - iQuadW))/fQuadW
			if iY == 1:
				fLatitude = abs((iX + iY + 1) - (iCentreX + iCentreY - iQuadW))/fQuadW
		elif iX < iCentreX:
			fLatitude = abs(iY - iX - iQuadW)/fQuadW
			# Make sure the "out of playing area tiles match the appropriate latitude"
			if iX == 1:
				fLatitude = abs(iY - iX + 1 - iQuadW)/fQuadW
			if iY == map.getGridWidth() - 2:
				fLatitude = abs(iY - iX - 1 - iQuadW)/fQuadW
		elif iY < iCentreY:
			fLatitude = abs(iX - iY - iQuadW)/fQuadW
			# Make sure the "out of playing area tiles match the appropriate latitude"
			if iX == map.getGridWidth() - 2:
				fLatitude = abs(iX - iY -1 - iQuadW)/fQuadW
			if iY == 1:
				fLatitude = abs(iX - iY + 1 - iQuadW)/fQuadW

		if varFractal is not None:
			fLatitude += (128 - varFractal.getHeight(iX, iY)) / (255.0 * 5.0)

	else: # Toroidal
		fLatitude = 0.5
	# Amended to allow for wrapping - i.e. remove frame
		iCentreX = map.getGridWidth()/2 + 1
		iCentreY = map.getGridWidth()/2 + 1
		iQuadW = (map.getGridWidth())/2
		fQuadW = iQuadW * 1.0

		if iX >= iCentreX and iY >= iCentreY:
			fLatitude = abs((iX + iY) - (iCentreX + iCentreY + iQuadW))/fQuadW
		elif iX < iCentreX and iY < iCentreY:
			fLatitude = abs((iX + iY) - (iCentreX + iCentreY - iQuadW))/fQuadW
		elif iX < iCentreX:
			fLatitude = abs(iY - iX - iQuadW)/fQuadW
		elif iY < iCentreY:
			fLatitude = abs(iX - iY - iQuadW)/fQuadW

		if varFractal is not None:
			fLatitude += (128 - varFractal.getHeight(iX, iY)) / (255.0 * 5.0)
	return fLatitude

def getLimitedLatitude(iX, iY, varFractal):
	# :param iX: x coordinate of the plot
	# :param iY: y coordinate of the plot.
	# :param varFractal: Fractal used to introduce random variations in the calculated distance.
	# :return: Calculated distance, limited between 0.0 and 1.0.
	#
	fDistance = getPeirceLatitude(iX, iY, varFractal)

	# Limit to the range [0, 1]:
	if fDistance < 0.0:
		fDistance = 0.0
	elif fDistance > 1.0:
		fDistance = 1.0
	return fDistance

def isPeirceFrame(iX, iY):
	# Checks if a specific plot is on the frame around the true playable area.  N/A for wrapped map.
	# :param iX: x coordinate of the plot
	# :param iY: y coordinate of the plot.
	#
	bPeirceFrame = 0
	map = CyGlobalContext().getMap()
	iPeirceWrap = map.getCustomMapOption(1)
	if iPeirceWrap == 1: # No wrap
		if iX == 0 or iX == 1 or iX == (map.getGridWidth() - 1) or iX == (map.getGridWidth() - 2):
			bPeirceFrame = 1
		elif iY == 0 or iY == 1 or iY == (map.getGridWidth() - 1) or iY == (map.getGridWidth() - 2):
			bPeirceFrame = 1
	return bPeirceFrame

def isPeirceCrease(iX, iY):
	# Checks if a specific plot is inside, central desert disc, i.e. impassable due to heat of sun.
	# :param iX: x coordinate of the plot
	# :param iY: y coordinate of the plot.
	# :return: True if the plot is outside of the disc, False otherwise.
	map = CyGlobalContext().getMap()
	iPeirceWrap = map.getCustomMapOption(1)
	if iPeirceWrap == 1: # No wrap
		iQuadW = (map.getGridWidth() - 4)/2
		iRadius = iQuadW/2 - 2
		iRadius2 = iRadius*iRadius
		iTRCentreX = map.getGridWidth()/2
		iTRCentreY = map.getGridWidth()/2
		iLAX = iQuadW/2 - 1
		iBBY = iQuadW/2 + 2
		iRAX = 3*iQuadW/2 + 1
		iTBY = 3*iQuadW/2 + 2
	else:
		iQuadW = (map.getGridWidth())/2
		iRadius = iQuadW/2
		iRadius2 = iRadius*iRadius
		iRadiusa2 = (iRadius-1)*(iRadius-1)
		iTRCentreX = map.getGridWidth()/2 + 1
		iTRCentreY = map.getGridWidth()/2 + 1
		iLAX = iQuadW/2
		iBBY = iQuadW/2
		iRAX = 3*iQuadW/2
		iTBY = 3*iQuadW/2

	bPeirceCrease = 0

	# <!-- custom: The original Peirce script used Python's bitwise XOR operator as if it squared each coordinate delta, so crease classification did not measure circular distance and materially changed map topology.
	# Explicit multiplication fixed the four quadrant tests. See KI#254. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if iX < iTRCentreX and iY < iTRCentreY: # Bottom Left Quad
		if (iX >= iLAX and iY <= iBBY) or (iX <= iLAX and iY >= iBBY):
			if ((iX - iLAX)*(iX - iLAX) + (iY - iBBY)*(iY - iBBY) >= iRadius2):
				bPeirceCrease = 1
	elif iX >= iTRCentreX and iY >= iTRCentreY: # Top Right Quad
		if (iX <= iRAX and iY >= iTBY) or (iX >= iRAX and iY <= iTBY):
			if ((iX - iRAX)*(iX - iRAX) + (iY - iTBY)*(iY - iTBY) >= iRadius2):
				bPeirceCrease = 1
	elif iY >= iTRCentreY: # Top Left Quad
		if (iX <= iLAX and iY <= iTBY) or (iX >= iLAX and iY >= iTBY):
			if ((iX - iLAX)*(iX - iLAX) + (iY - iTBY)*(iY - iTBY) >= iRadius2):
				bPeirceCrease = 1
	elif iX >= iTRCentreX: # Bottom Right Quad
		if (iX <= iRAX and iY <= iBBY) or (iX >= iRAX and iY >= iBBY):
			if ((iX - iRAX)*(iX - iRAX) + (iY - iBBY)*(iY - iBBY) >= iRadius2):
				bPeirceCrease = 1

	return bPeirceCrease

def isEquatorialBand(iX, iY):
	# Checks if a specific plot is around the equator diagonals, which will be ocean in the E-W continent map
	map = CyGlobalContext().getMap()
	iPeirceWrap = map.getCustomMapOption(1)
	if iPeirceWrap == 1: # No wrap
		iQuadW = (map.getGridWidth() - 4)/2
		iCentreX = map.getGridWidth()/2
		iCentreY = map.getGridWidth()/2
	else:
		iQuadW = (map.getGridWidth())/2
		iCentreX = map.getGridWidth()/2
		iCentreY = map.getGridWidth()/2

	bEquatorialBand = 0

	if iX >= iCentreX and iY >= iCentreY:
		if abs((iX + iY) - (iCentreX + iCentreY + iQuadW)) < 2:
			bEquatorialBand = 1
	elif iX < iCentreX and iY < iCentreY:
		if abs((iX + iY) - (iCentreX + iCentreY - iQuadW)) < 2:
			bEquatorialBand = 1
	elif iX < iCentreX:
		if abs(iY - iX - iQuadW) < 2:
			bEquatorialBand = 1
	elif iY < iCentreY:
		if abs(iX - iY - iQuadW) < 2:
			bEquatorialBand = 1

	return bEquatorialBand
