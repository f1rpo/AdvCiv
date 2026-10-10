#-----------------------------------------------------------------------------
#	Copyright (c) 2008 Firaxis Games, Inc. All rights reserved.
#-----------------------------------------------------------------------------
#
#
#	FILE:	 RandomScriptMap.py
#	AUTHOR:  Oleg Giwodiorow / Refar
#                lord_refar@yahoo.de
#       CONTRIB: Examples and Code Snipplets from original CivIV scripts
#	PURPOSE: Map Script for Civ4. Will generate a map using one of
#                the games Scripts, choosen at random and using some
#                additional parameters to add diversity.
#                Archipelago, Fractal, Pangea, Hemispheres, Big&Small
#                and Terra are the included scripts.
#       
#-----------------------------------------------------------------------------
#
#-----------------------------------------------------------------------------
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#

from CvPythonExtensions import *
import CvUtil
import CvMapGeneratorUtil
from CvMapGeneratorUtil import FractalWorld
from CvMapGeneratorUtil import MultilayeredFractal
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator
from SAS_WorldSizeUtils import *
from SASUtils import getInfoTypeOrFail

#-----------------------------------------------------------------------------
# User Editable Constants.
# Edit here to have the options you like preselected and to change some
# generation parameters / probabilities
#
# 1.) MENU DEFAULTS:
# Used World Creation Method 
# 0 Any
# 1 Fractal Continental;
# 2 Terra;
# 3 Hemispheres
# 4 Big And Small;
# 5 Medium And Small;
# 6 Archipelago;
# 7 Pangaea
c_genMethodDefault = 0   # Range: 0 - 7; Default: 0 = Any, chosen at random
#
# Methods to exlude
# NOTE: Exclude Override Favourite
c_exclude1Default = 0    # Range: 0 - 7; Default: 0 = Exclude None
c_exclude2Default = 0    # Range: 0 - 7; Default: 0 = Exclude None
c_exclude3Default = 0    # Range: 0 - 7; Default: 0 = Exclude None 
#
# Additional Conditions for Starting Plots. The AI's :
# 0 - No Conditions
# 1 - Script Default Will Start in Old World on Terra and Pangaea. Other Maps
#     might start in Old World (Random, but only if big landmass exists).
# 2 - Force Old World - All Will Start on the Biggest Land. This may override
#     The Script Selection: Archipelage defaults to Big And Small
c_aiStartDefault = 0   # Range: 0 - 2; Default: 0
# Additional Conditions For Human Player/-s
# 0 - Use Same Rules as AI
# 1 - Start near Coast
# 2 - Start on Biggest Landmass Avaiable (Old World)
# 3 - Start near Coast in Old World
c_plStartDefault = 0   # Range: 0 - 3; Default: 0
#
# 2.) PROBABILITIES:
# How much weight the different creation Methods shall recieve, when
# using random Choice. Here you can enter any number equal or bigger than 0
# allwing to finetune the probabilities of getting differnt worlds.
c_fracWeight = 1      # Default: 1; Fractal Continents
c_terraWeight = 1     # Default: 1; Terra-Type World
c_hemiWeight = 1      # Default: 1; Hemispheres. 2 or 3 big Lands + some Islands 
c_bnsWeight = 1       # Default: 1; Big and Small. Big Continent + many Islands
c_mnsWeight = 1       # Default: 1; Medium and Small. 
c_archiWeight = 1     # Default: 1; Fractal Archipelago
c_pangWeight = 1      # Default: 1; Natural Coast Line Pangaea
c_favWeight = 0       # Default: 0; Value not used. Change Weight Directly
#
# 3.) MISC
# Islands smaller than c_minStartArea can not be starting spots. The script
# will try to provide double or more minSize, whenever possible. 
# c_spacePerCiv for some dicisions regarding start plot placement.
# NOTE: Dont Choose both numbers too big - the conditions will be ignored, if
# there is not enought space.
# <!-- custom: RandomScriptMap still has legacy 0..5 Duel..Huge tuning arrays below; convert XML-aligned WorldSizeTypes to those legacy bucket indexes before indexing them.
# Arena reuses Duel tuning and SAS sizes reuse Huge tuning. (GPT-5.5? + ChatGPT-5.5) -->
c_minStartAreaSize = [6, 6, 8, 8, 10, 10]   # Default [6, 6, 8, 8, 10, 10]
c_spacePerCiv = [40, 50, 50, 75, 75, 90]    # Default [40,50,50,75,75,90]
#
# Distance from Top/Bottom of the map where no players shall start
# (polar regions). Can be set to 0 to allow all plots.
# NOTE: A value > 0.5 will make the entire map invalid. On Small maps this 
# also could happen with values lower but very close to 0.5 (Due to rounding)
# Values 0 - 0.25 seem to make sense and are definitely safe on all maps.
c_polar_lat_limit = [0.16, 0.16, 0.14, 0.14, 0.12, 0.12]
# Default [0.16, 0.16, 0.14, 0.14, 0.12, 0.12]
#
# 4.) INTERNAL FLAG CONSTANTS
# Do not change.
MAP_RAND = 0
MAP_FRACT = 1
MAP_TERRA = 2
MAP_HEMIS = 3
MAP_BIGNS = 4
MAP_MANDS = 5
MAP_ARCHI = 6
MAP_PANGA = 7
STA_NOCOND = -1
STA_DEFAULT = 0
STA_COASTAL = 1
STA_OLDWORLD = 2
STA_OLDCOAST = 3

def _sas_world_size_base_index(eWorldSize):
	# <!-- custom: Convert XML-aligned WorldSizeTypes to RandomScriptMap's legacy 0..5 Duel..Huge tuning buckets.
	# Arena reuses Duel tuning and SAS sizes reuse Huge tuning. (GPT-5.5? + ChatGPT-5.5) -->
	size_index_values = {
		WorldSizeTypes.WORLDSIZE_ARENA: 0,
		WorldSizeTypes.WORLDSIZE_DUEL: 0,
		WorldSizeTypes.WORLDSIZE_TINY: 1,
		WorldSizeTypes.WORLDSIZE_SMALL: 2,
		WorldSizeTypes.WORLDSIZE_STANDARD: 3,
		WorldSizeTypes.WORLDSIZE_LARGE: 4,
		WorldSizeTypes.WORLDSIZE_HUGE: 5
	}
	return sas_lookup_world_size(eWorldSize, size_index_values)

#-----------------------------------------------------------------------------
#
#
def getDescription():
	return "Generates a Random Map, using one of the games Scripts"

#-----------------------------------------------------------------------------
#
#	
def isAdvancedMap():
	"This map should not show up in simple mode"
	return 1

#-----------------------------------------------------------------------------
#
#
def getNumPlotsPercent(argsList):
	# <!-- custom: Huge tested slightly too large after fixing the leaked helper getGridSize; trim only larger tiers a little while leaving RandomScriptMap's internal generator choice logic intact. (GPT-5.5?) -->
	[eWorldSize] = argsList
	if eWorldSize < 0:
		return 100
	sizeModifiers = {
		WorldSizeTypes.WORLDSIZE_ARENA: 100,
		WorldSizeTypes.WORLDSIZE_DUEL: 100,
		WorldSizeTypes.WORLDSIZE_TINY: 100,
		WorldSizeTypes.WORLDSIZE_SMALL: 100,
		WorldSizeTypes.WORLDSIZE_STANDARD: 100,
		WorldSizeTypes.WORLDSIZE_LARGE: 99,
		WorldSizeTypes.WORLDSIZE_HUGE: 96
	}
	return sas_lookup_world_size(eWorldSize, sizeModifiers)

#-----------------------------------------------------------------------------
#
#
def getNumCustomMapOptions():
	return 6

#-----------------------------------------------------------------------------
#
#
def getCustomMapOptionName(argsList):
	dummy = argsList[0]
	if (dummy == 0)  :
		szName = "TXT_KEY_MAP_SCRIPT_CREATION_METHOD"
	elif (dummy == 1):
		szName = "TXT_KEY_MAP_SCRIPT_AI_START"
	elif (dummy == 2):
		szName = "TXT_KEY_MAP_SCRIPT_PLAYER_START"
	else:
		szName = "TXT_KEY_MAP_SCRIPT_EXCLUDE_METHOD"

	return unicode(CyTranslator().getText(szName, ()))

#-----------------------------------------------------------------------------
#
#
def getNumCustomMapOptionValues(argsList):
	dummy = argsList[0]
	if (dummy == 0)  :
		return 8 # Force Script
	elif (dummy == 1):
		return 3 # AI Start
	elif (dummy == 2):
		return 4 # Player Start
	else:
		return 8             # Excludes

#-----------------------------------------------------------------------------
# Menu entries. Many are non standard, therefore we do not use the default
# translated keys. English only.
#
def getCustomMapOptionDescAt(argsList):
	dummy = argsList[0]
	iSelection = argsList[1]
	genMethod_names = ["TXT_KEY_MAP_SCRIPT_RANDOM", "Fractal", "Terra", "Hemispheres", "Big_and_Small", "Medium_and_Small", "Archipelago", "Pangaea"]
	exclude_names = ["TXT_KEY_MAIN_MENU_NONE", "Fractal", "Terra", "Hemispheres", "Big_and_Small", "Medium_and_Small", "Archipelago", "Pangaea"]
	startAi_names = ["TXT_KEY_MAP_SCRIPT_NO_CONDITIONS", "TXT_KEY_MAP_SCRIPT_DEFAULT", "TXT_KEY_MAP_SCRIPT_ALL_IN_OLD_WORLD"]
	# <!-- custom: The combined validator requires coastal land on the largest continent; use an SAS key instead of BTS's misleading "or" label. See KI#275.2. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	startPl_names = ["TXT_KEY_MAP_SCRIPT_SAME_AS_AI", "TXT_KEY_MAP_SCRIPT_NEAR_COAST", "TXT_KEY_MAP_SCRIPT_LARGEST_CONTINENT", "TXT_KEY_MAP_SCRIPT_SAS_COASTAL_LARGEST_CONTINENT"]
	if (dummy == 0)  :
		szName = genMethod_names[iSelection]
	elif (dummy == 1):
		szName = startAi_names[iSelection]
	elif (dummy == 2):
		szName = startPl_names[iSelection]
	else:
		szName = exclude_names[iSelection]
	return unicode(CyTranslator().getText(szName, ()))

#-----------------------------------------------------------------------------
# Disable games standard "Random" for some of the options menus
#
def isRandomCustomMapOption(argsList):
	dummy = argsList[0]
	if (dummy == 0)   :
		return False
	elif (dummy == 1) :
		return False
	elif (dummy == 2) :
		return False
	else:
		return True
	return True
#-----------------------------------------------------------------------------
# Assign default values to the Options, using the constants defined in the
# beginning of this file
#
def getCustomMapOptionDefault(argsList):
	dummy = argsList[0]
	if (dummy == 0)  :
		return c_genMethodDefault
	elif (dummy == 1):
		return c_aiStartDefault
	elif (dummy == 2):
		return c_plStartDefault
	elif (dummy == 3):
		return c_exclude1Default
	elif (dummy == 4):
		return c_exclude2Default
	elif (dummy == 5):
		return c_exclude3Default
	else:
		return 0
	return 0

#-----------------------------------------------------------------------------
# Here we read out the User Options and break the many possible
# combinations down to something we can handle.
#
def beforeGeneration():
	global gl_mapType
	global gl_aiStartType
	global gl_plStartType
	global gl_startPlot_firstrun
	gl_startPlot_firstrun = True

	gc = CyGlobalContext()
	map = CyMap()
	dice = gc.getGame().getMapRand()
	# Removed User Selction for Favourite, so set to None.
	favMethod = 0

	genMethod = map.getCustomMapOption(0)
	aiStartMethod = map.getCustomMapOption(1)
	plStartMethod = map.getCustomMapOption(2)
	exclude1 = map.getCustomMapOption(3)		
	exclude2 = map.getCustomMapOption(4)
	exclude3 = map.getCustomMapOption(5) 

	# Deciding on what kind of World we shall get.
	#
	# Calculate the Weights and Probabilities of Methods with respect
	# to Favourite Choice and Excludes.
	# NOTE: Exclude overrides Favourite.
	diceNumber = 0
	fracWeight = c_fracWeight
	if( favMethod == MAP_FRACT ):
		fracWeight += c_favWeight
	if(exclude1 != MAP_FRACT and exclude2 != MAP_FRACT and exclude3 != MAP_FRACT) :
		diceNumber += fracWeight
	else:
		fracWeight = 0
	terraWeight = c_terraWeight
	if( favMethod == MAP_TERRA ):
		terraWeight += c_favWeight	
	if(exclude1 != MAP_TERRA and exclude2 != MAP_TERRA and exclude3 != MAP_TERRA) :
		diceNumber += terraWeight
	else:
		terraWeight = 0
	hemiWeight = c_hemiWeight
	if( favMethod == MAP_HEMIS ):
		hemiWeight += c_favWeight
	if(exclude1 != MAP_HEMIS and exclude2 != MAP_HEMIS and exclude3 != MAP_HEMIS) :
		diceNumber += hemiWeight
	else:
		hemiWeight = 0
	bnsWeight = c_bnsWeight
	if( favMethod == MAP_BIGNS ):
		bnsWeight += c_favWeight
	if(exclude1 != MAP_BIGNS and exclude2 != MAP_BIGNS and exclude3 != MAP_BIGNS) :
		diceNumber += bnsWeight
	else:
		bnsWeight = 0
	mnsWeight = c_mnsWeight
	if( favMethod == MAP_MANDS ):
		mnsWeight += c_favWeight
	if(exclude1 != MAP_MANDS and exclude2 != MAP_MANDS and exclude3 != MAP_MANDS) :
		diceNumber += mnsWeight
	else:
		mnsWeight = 0
	archiWeight = c_archiWeight
	if( favMethod == MAP_ARCHI ):
		archiWeight += c_favWeight
	if(exclude1 != MAP_ARCHI and exclude2 != MAP_ARCHI and exclude3 != MAP_ARCHI) :
		diceNumber += archiWeight
	else:
		archiWeight = 0	
	pangWeight = c_pangWeight
	if( favMethod == MAP_PANGA ):
		pangWeight += c_favWeight
	if(exclude1 != MAP_PANGA and exclude2 != MAP_PANGA and exclude3 != MAP_PANGA) :
		diceNumber += pangWeight
	else:
		pangWeight = 0
	# Now make Random Choice according to Calculated Weights
	if( genMethod == MAP_RAND ) :
		rnd = dice.get(diceNumber, "Python RandomMapScript" )
		threshold = 0
		if( rnd >= threshold and rnd < (threshold + fracWeight) ) :
			gl_mapType = MAP_FRACT 
		threshold += fracWeight
		if( rnd >= threshold and rnd < (threshold + terraWeight) ) :
			gl_mapType = MAP_TERRA 
		threshold += terraWeight
		if( rnd >= threshold and rnd < (threshold + hemiWeight) ) :
			gl_mapType = MAP_HEMIS
		threshold += hemiWeight
		if( rnd >= threshold and rnd < (threshold + bnsWeight) ) :
			gl_mapType = MAP_BIGNS
		threshold += bnsWeight
		if( rnd >= threshold and rnd < (threshold + mnsWeight) ) :
			gl_mapType = MAP_MANDS
		threshold += mnsWeight
		if( rnd >= threshold and rnd < (threshold + archiWeight) ) :
			gl_mapType = MAP_ARCHI
		threshold += archiWeight
		if( rnd >= threshold and rnd < (threshold + pangWeight) ) :
			gl_mapType = MAP_PANGA
		threshold += pangWeight
	else :
		gl_mapType = genMethod
	# Alea iacta est

	# Now for Start Conditions
	gl_plStartType = int(plStartMethod)

	if( aiStartMethod == 1 and gl_mapType == MAP_PANGA ) :
		gl_aiStartType = STA_OLDWORLD
	elif(aiStartMethod == 1 and gl_mapType == MAP_TERRA and 0 == dice.get(3, "Python RandomMapScript")) :
		gl_aiStartType = STA_OLDWORLD
	elif (aiStartMethod == 1 and 0 == dice.get(4, "Python RandomMapScript")) :
		gl_aiStartType = STA_DEFAULT
	elif( aiStartMethod == 2 ) :
		gl_aiStartType = STA_OLDWORLD
	else :
		gl_aiStartType = STA_NOCOND
	# Make Archipelago to Big&Small, if Old World is required.	
	if( gl_aiStartType == STA_OLDWORLD and gl_mapType == MAP_ARCHI ) :
		gl_mapType = MAP_BIGNS

	print("Creating World using Method: ", gl_mapType, "  AI Start: ", gl_aiStartType, "  Player Start: ", gl_plStartType)

#-----------------------------------------------------------------------------
# Changes the minimal distance between starting points. 
#
def minStartingDistanceModifier():
	return -10

#-----------------------------------------------------------------------------
# Look how many Areas of different size are on the Map. The information will
# Later be used by the isValid method to decide, which conditions cn or can
# not be enforced.
#
def measureWorld():
	global gl_numAreas_min
	global gl_numAreas_min_05
	global gl_numAreas_min_15
	global gl_numAreas_min_2
	global gl_numAreas_min_25
	global gl_numAreas_conti
	global gl_numAreas_contiBig

	sk = _sas_world_size_base_index(CyMap().getWorldSize())
	gl_numAreas_min = 0
	gl_numAreas_min_05 = 0
	gl_numAreas_min_15 = 0
	gl_numAreas_min_2 = 0
	gl_numAreas_min_25 = 0
	gl_numAreas_conti = 0
	gl_numAreas_contiBig = 0

	areas = CvMapGeneratorUtil.getAreas()
	for area in areas:
		if not area.isWater() : 
			sa = area.getNumTiles()
			dummy = int(sa / c_spacePerCiv[sk])
			gl_numAreas_conti += dummy
			if ( sa >= c_minStartAreaSize[sk] * 0.5 ) :
				gl_numAreas_min_05 += 1
			if ( sa >= c_minStartAreaSize[sk] ) :
				gl_numAreas_min += 1
			if ( sa >= c_minStartAreaSize[sk] * 1.5 ) :
				gl_numAreas_min_15 += 1
			if ( sa >= c_minStartAreaSize[sk] * 2 ) :
				gl_numAreas_min_2 += 1
			if ( sa >= c_minStartAreaSize[sk] * 2.5 ) :
				gl_numAreas_min_25 += 1

#-----------------------------------------------------------------------------
# isValid Function, preventing playes to start on islands to small
# to accommodate at least one City. The threshold-constant can be set in the
# beginning of this file. There is also a Check, if the map can accommodate all
# civs given this condition. If not enought place is found, the Function will
# accept smaller islands as well, so we shall not end up discarding
# all plots.
#
def isValidStd(playerID, x, y):
	map = CyMap()
	startingAreaSize = map.plot(x, y).area().getNumTiles()
	gc = CyGlobalContext()
	numCivs = gc.getGame().countCivPlayersEverAlive()
	sk = _sas_world_size_base_index(map.getWorldSize())

	# Don't Start in Polar Regions.
	height = map.getGridHeight()
	north_pol = (1 - c_polar_lat_limit[sk]) * height
	south_pol = c_polar_lat_limit[sk] * height
	if( y > north_pol or y < south_pol ) :
		return False

	# If getting a rather small staring island, at least make sure,
	# we can have a coastal capital.
	if(startingAreaSize < (c_minStartAreaSize[sk] * 3.5) and not map.plot(x, y).isCoastalLand()) :
		return False
	# On Large and Huge maps and if there is actually plenty of space in
	# the world, we want have a bigger landmass to start on.
	if(startingAreaSize < (c_minStartAreaSize[sk] * 2.5) and (gl_numAreas_min_25 + gl_numAreas_conti * 0.7) >= numCivs and not (gl_mapType == MAP_ARCHI)) :
		return False
	if(startingAreaSize < (c_minStartAreaSize[sk] * 2) and (gl_numAreas_min_2 + gl_numAreas_conti) >= numCivs and not (gl_mapType == MAP_ARCHI)) :
		return False
	# Everyone can at least get a island of c_minStartAreaSize, or slightly
	# bigger.
	if(startingAreaSize < (c_minStartAreaSize[sk] * 1.5) and (gl_numAreas_min_15 + gl_numAreas_conti) >= numCivs and not (gl_mapType == MAP_ARCHI)) :
		return False
	if(startingAreaSize < c_minStartAreaSize[sk] and (gl_numAreas_min + gl_numAreas_conti) >= numCivs) :
		return False
	# It does not look good, but lets at least try getting something bigger
	# than 1 plot.
	if(startingAreaSize < (c_minStartAreaSize[sk] * 0.5) and (gl_numAreas_min_05 + gl_numAreas_conti) >= numCivs) :
		return False

	# If we are here, we either got a acceptable starting plot, or there
	# is no hope of getting it at all, so we have to accept wat we got.
	return True

#-----------------------------------------------------------------------------
# isValid Function to enforce all Civs starting on the biggest Landmass
#	
def isValidOldW(playerID, x, y):
	map = CyMap()
	pPlot = map.plot(x, y)
	# Don't Start in Polar Regions.
	sk = _sas_world_size_base_index(map.getWorldSize())
	height = map.getGridHeight()
	north_pol = (1 - c_polar_lat_limit[sk]) * height
	south_pol = c_polar_lat_limit[sk] * height
	if( y > north_pol or y < south_pol ) :
		return False
	if (pPlot.getArea() != map.findBiggestArea(False).getID()):
		return False
	return True

#-----------------------------------------------------------------------------
# isValid Function to enforce all Civs starting near a coast.
# Since this will often be used on arhcipelago, a additional check for very
# Small start Area is performed.
#
def isValidCoast(playerID, x, y):
	pPlot = CyMap().plot(x, y)
	pWaterArea = pPlot.waterArea()
	if (pWaterArea.isNone()):
		return False
	# Polar latitude check in isValidStd
	return (not pWaterArea.isLake()) and isValidStd(playerID, x, y) 

#-----------------------------------------------------------------------------
# isValid Function to enforce both coastal and oldworld start conditions.
# not sure if it will cause problems or overcrowding the coastal lines.
#
def isValidBoth(playerID, x, y):
	# <!-- custom: Coastal on Biggest Land raised UnboundLocalError before testing any plot because this function incremented an uninitialized, otherwise-unused local counter.
	# Removing that obsolete increment restores the requested coastal/Old-World validator. See KI#275. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	map = CyMap()
	pPlot = map.plot(x, y)
	# Don't Start in Polar Regions.
	sk = _sas_world_size_base_index(map.getWorldSize())
	height = map.getGridHeight()
	north_pol = (1 - c_polar_lat_limit[sk]) * height
	south_pol = c_polar_lat_limit[sk] * height
	if( y > north_pol or y < south_pol ) :
		return False
	pWaterArea = pPlot.waterArea()
	if(pPlot.getArea() != map.findBiggestArea(False).getID() or pWaterArea.isNone()) :
		return False
	return not pWaterArea.isLake()

#-----------------------------------------------------------------------------
# Checks if a special start condition is appropriate (or forced) and calls
# the default findStartingPlot function, using the according isValid condition
#
def findStartingPlot(argsList):
	[playerID] = argsList
	global gl_startPlot_firstrun
	gc = CyGlobalContext()
	map = CyMap()
	numCivs = gc.getGame().countCivPlayersEverAlive()
	biggestContinent = map.findBiggestArea(False)
	bcSize = biggestContinent.getNumTiles()
	worldLandSize = map.getLandPlots()
	sk = _sas_world_size_base_index(map.getWorldSize())

	if ( gl_startPlot_firstrun ) :
		measureWorld()
		gl_startPlot_firstrun = False

	goodForOldWorld = False
	if ((bcSize * 1.6) > worldLandSize and bcSize > numCivs * c_spacePerCiv[sk]) :
		goodForOldWorld = True

	if(gc.getPlayer(playerID).isHuman() and gl_plStartType != STA_DEFAULT) :
		if( gl_plStartType == STA_COASTAL ) :
			print("Player ", playerID, "(Human) ", "Start near Coast")
			return CvMapGeneratorUtil.findStartingPlot(playerID, isValidCoast)
		elif( gl_plStartType == STA_OLDWORLD) :
			print("Player ", playerID, "(Human) ", "Start on Biggest Landmass")
			return CvMapGeneratorUtil.findStartingPlot(playerID, isValidOldW)
		elif( gl_plStartType == STA_OLDCOAST) :
			print("Player ", playerID, "(Human) ", "Start on Biggest Landmass near Coast")
			return CvMapGeneratorUtil.findStartingPlot(playerID, isValidBoth)
		# Unexpected Defaults to "Same as AI"

	if(gl_aiStartType == STA_OLDWORLD or ((gl_aiStartType == STA_DEFAULT) and goodForOldWorld)) :
		print("Player ", playerID, " - Start In Old World")
		return CvMapGeneratorUtil.findStartingPlot(playerID, isValidOldW)
	else :
		print("Player ", playerID, " - No Start Conditions")
		return CvMapGeneratorUtil.findStartingPlot(playerID, isValidStd)	

	# Personal Note. Keep forgeting it.
	# CyPythonMgr().allowDefaultImpl()
	# CyMap().getWorldSize()

#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
# IMPORTED SCRIPTS BEGIN
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
# AUTHORS: Oleg Giwodiorow (Refar)
# Subclassing Multilayered fractal, in order to overwrite the
# generatePlotsInRegion() function, so thait it uses the Rift_Grain
# parameter now.
# The goal is being able to create regions usig the same settings, as
# the normal fractal map genetaror does.
# This to be able to make one layer covering all map, and being the same
# as the normal 'Fractal' map type, but with the possibility to add
# more layers for islands or specific features.
#-----------------------------------------------------------------------------
class R_MultilayeredFractal(CvMapGeneratorUtil.MultilayeredFractal):
    #-------------------------------------------------------------------------
    # Overwriting to include Wrap Check - better results if onr Region 
    # covers the entire map.
    #
    def shiftRegionPlots(self, iRegionWidth, iRegionHeight, iStrip=15):
        stripRadius = min(15, iStrip)
        stripRadius = max(3, iStrip)
        best_split_x = 0
        best_split_y = 0

        if self.map.isWrapX() :
            best_split_x = self.findBestRegionSplitX(iRegionWidth, iRegionHeight, stripRadius)
        if self.map.isWrapY() :
            best_split_y = self.findBestRegionSplitY(iRegionWidth, iRegionHeight, stripRadius)

        self.shiftRegionPlotsBy(best_split_x, best_split_y, iRegionWidth, iRegionHeight)

    #-------------------------------------------------------------------------
    # Overwriting to change the parameters of the regional fractals.
    # Main Change - the RegionalFractal now can have a RiftFractal
    def generatePlotsInRegion(self, iWaterPercent, iRegionWidth, iRegionHeight, iRegionWestX, iRegionSouthY, iRegionGrain, iRegionHillsGrain, iRegionPlotFlags, iRegionTerrainFlags, iRegionFracXExp = -1, iRegionFracYExp = -1, bShift = True, iStrip = 15, rift_grain = -1, has_center_rift = False, invert_heights = False):
        # This is the code to generate each fractal.
        #
        # Init local variables
        water = iWaterPercent
        iWestX = iRegionWestX
        # Note: Do not pass bad regional dimensions so that iEastX > self.iW
        iSouthY = iRegionSouthY
        # Init the plot types array and the regional fractals
        self.plotTypes = [] # reinit the array for each pass
        self.plotTypes = [PlotTypes.PLOT_OCEAN] * (iRegionWidth*iRegionHeight)
        regionContinentsFrac = CyFractal()
        regionHillsFrac = CyFractal()
        regionPeaksFrac = CyFractal()
        if (rift_grain >= 0) :
            rFrac = CyFractal()
            rFrac.fracInit(iRegionWidth, iRegionHeight, rift_grain, self.dice, 0, iRegionFracXExp, iRegionFracYExp)
            if has_center_rift:
                iRegionPlotFlags += CyFractal.FracVals.FRAC_CENTER_RIFT
            regionContinentsFrac.fracInitRifts(iRegionWidth, iRegionHeight, iRegionGrain, self.dice, iRegionPlotFlags, rFrac, iRegionFracXExp, iRegionFracYExp)
        else :   
            regionContinentsFrac.fracInit(iRegionWidth, iRegionHeight, iRegionGrain, self.dice, iRegionPlotFlags, iRegionFracXExp, iRegionFracYExp)

        regionHillsFrac.fracInit(iRegionWidth, iRegionHeight, iRegionHillsGrain, self.dice, iRegionTerrainFlags, iRegionFracXExp, iRegionFracYExp)
        regionPeaksFrac.fracInit(iRegionWidth, iRegionHeight, iRegionHillsGrain+1, self.dice, iRegionTerrainFlags, iRegionFracXExp, iRegionFracYExp)

        iWaterThreshold = regionContinentsFrac.getHeightFromPercent(water)
        iHillsBottom1 = regionHillsFrac.getHeightFromPercent(max((25 - self.gc.getClimateInfo(self.map.getClimate()).getHillRange()), 0))
        iHillsTop1 = regionHillsFrac.getHeightFromPercent(min((25 + self.gc.getClimateInfo(self.map.getClimate()).getHillRange()), 100))
        iHillsBottom2 = regionHillsFrac.getHeightFromPercent(max((75 - self.gc.getClimateInfo(self.map.getClimate()).getHillRange()), 0))
        iHillsTop2 = regionHillsFrac.getHeightFromPercent(min((75 + self.gc.getClimateInfo(self.map.getClimate()).getHillRange()), 100))
        iPeakThreshold = regionPeaksFrac.getHeightFromPercent(self.gc.getClimateInfo(self.map.getClimate()).getPeakPercent())

        # Loop through the region's plots
        for x in range(iRegionWidth):
            for y in range(iRegionHeight):
                i = y*iRegionWidth + x
                val = regionContinentsFrac.getHeight(x,y)
                if val <= iWaterThreshold:
                    pass
                else:
                    hillVal = regionHillsFrac.getHeight(x,y)
                    if ((hillVal >= iHillsBottom1 and hillVal <= iHillsTop1) or (hillVal >= iHillsBottom2 and hillVal <= iHillsTop2)):
                        peakVal = regionPeaksFrac.getHeight(x,y)
                        if (peakVal <= iPeakThreshold):
                            self.plotTypes[i] = PlotTypes.PLOT_PEAK
                        else:
                            self.plotTypes[i] = PlotTypes.PLOT_HILLS
                    else:
                        self.plotTypes[i] = PlotTypes.PLOT_LAND

        if bShift :
            # Shift plots to obtain a more natural shape.
            self.shiftRegionPlots(iRegionWidth, iRegionHeight, iStrip)

        # Once the plot types for the region have been generated, they must be
        # applied to the global plot array.
        # Default approach is to ignore water and layer the lands over 
        for x in range(iRegionWidth):
            wholeworldX = x + iWestX
            for y in range(iRegionHeight):
                i = y*iRegionWidth + x
                if self.plotTypes[i] == PlotTypes.PLOT_OCEAN:
                    continue
                wholeworldY = y + iSouthY
                iWorld = wholeworldY*self.iW + wholeworldX
                self.wholeworldPlotTypes[iWorld] = self.plotTypes[i]

        # This region is done.
        return

    #-------------------------------------------------------------------------
    # MultilayeredFractal class, controlling function.
    #
    # Regional Variables/Defaults for GeneratePlotsInRegion() from
    # CvMapGeneratorUtil.py
    #
    # [self,]
    # iWaterPercent, 
    # iRegionWidth, iRegionHeight, 
    # iRegionWestX, iRegionSouthY, 
    # iRegionGrain, iRegionHillsGrain, 
    # iRegionPlotFlags, iRegionTerrainFlags, 
    # iRegionFracXExp = -1, iRegionFracYExp = -1, 
    # bShift = True, iStrip = 15, 
    # rift_grain = -1, has_center_rift = False, 
    # invert_heights = False
    #
    def generatePlotsByRegion(self, forceCGrain = False):   

        cGrain = 2 + self.dice.get(4, "Python, RandomScriptUtil, Conti")
        if ( cGrain > 2 ) :
            cGrain = 2
        rGrain = 5
        sea = self.gc.getSeaLevelInfo(self.map.getSeaLevel()).getSeaLevelChange()
        sea = min(sea, 5)
        sea = max(sea, -5)
        iWater = 81 + sea
        mainWater = 76 + sea
        if ( forceCGrain ) :
            cGrain = 2

        subSlot1 = 5 + self.dice.get(2, "Python, RndScriptUtil, Conti")
        subSlot2 = 1 + self.dice.get(4, "Python, RndScriptUtil, Conti")
        subSlot3 = self.dice.get(10, "Python, RndScriptUtil, Conti")
        subSlot4 = 0 # self.dice.get(12, "Python, RndScriptUtil, Conti")

        print("Fractal Continental: cGrain: ", cGrain, "Water % ", iWater, "Subslots: ", subSlot1, subSlot2, subSlot3, subSlot4)

        # 0 Duell, 1 Tiny, 2 Small, 3 Standard, 4 Large, 5 Huge
        sizekey = _sas_world_size_base_index(self.map.getWorldSize())
        if ( sizekey > 3 ) :
            xExp = 8
            yExp = 7
            tinyExp = 4
        else :
            xExp = 7
            yExp = 6
            tinyExp = 3

        # xExp = CyFractal.FracVals.DEFAULT_FRAC_X_EXP
        # yExp = CyFractal.FracVals.DEFAULT_FRAC_Y_EXP

        # Islands.
        # Additional Slots can be choosen.
        # Slot Order:
        # |--------|
        # |2 |6 |4 |
        # |--------|
        # |1 |5 |3 |
        # |--------|
        #
        # Slot1
        if( subSlot1 == 1 or subSlot2 == 1 or subSlot3 == 1 or subSlot4 == 1) :
            iSouthY = int(self.iH * (0.15 + 0.01 * self.dice.get(7,"RndScr.")))
            iHeight = int(self.iH * (0.23 + 0.01 * self.dice.get(12,"RndScr."))) 
            iWestX = int(self.iW * (0.05 + 0.01 * self.dice.get(12,"RndScr.")))
            iWidth = int(self.iW * (0.15 + 0.01 * self.dice.get(16,"RndScr.")))
            iGrain = 4 + self.dice.get(2,"RndScr.")
            self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iGrain, 3, self.iRoundFlags, self.iTerrainFlags, tinyExp, tinyExp, True, 10, -1, False, False)

        # Slot2
        if( subSlot1 == 2 or subSlot2 == 2 or subSlot3 == 2 or subSlot4 == 2) :
            iSouthY = int(self.iH * (0.47 + 0.01 * self.dice.get(10,"RndScr.")))
            iHeight = int(self.iH * (0.25 + 0.01 * self.dice.get(7,"RndScr."))) 
            iWestX = int(self.iW * (0.03 + 0.01 * self.dice.get(12,"RndScr.")))
            iWidth = int(self.iW * (0.15 + 0.01 * self.dice.get(14,"RndScr.")))
            iGrain = 4 + self.dice.get(2,"RndScr.")
            self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iGrain, 3, self.iRoundFlags, self.iTerrainFlags, tinyExp, tinyExp, True, 10, -1, False, False)

        # Slot3
        if( subSlot1 == 3 or subSlot2 == 3 or subSlot3 == 3 or subSlot4 == 3) :
            iSouthY = int(self.iH * (0.17 + 0.01 * self.dice.get(7,"RndScr.")))
            iHeight = int(self.iH * (0.23 + 0.01 * self.dice.get(12,"RndScr."))) 
            iWestX = int(self.iW * (0.68 + 0.01 * self.dice.get(9,"RndScr.")))
            iWidth = int(self.iW * (0.13 + 0.01 * self.dice.get(7,"RndScr.")))
            iGrain = 4 + self.dice.get(2,"RndScr.")
            self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iGrain, 3, self.iRoundFlags, self.iTerrainFlags, tinyExp, tinyExp, True, 10, -1, False, False)

        # Slot4
        if( subSlot1 == 4 or subSlot2 == 4 or subSlot3 == 4 or subSlot4 == 4) :
            iSouthY = int(self.iH * (0.43 + 0.01 * self.dice.get(8,"RndScr.")))
            iHeight = int(self.iH * (0.26 + 0.01 * self.dice.get(8,"RndScr."))) 
            iWestX = int(self.iW * (0.65 + 0.01 * self.dice.get(12,"RndScr.")))
            iWidth = int(self.iW * (0.1 + 0.01 * self.dice.get(10,"RndScr.")))
            iGrain = 4 + self.dice.get(2,"RndScr.")
            self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iGrain, 3, self.iRoundFlags, self.iTerrainFlags, tinyExp, tinyExp, True, 10, -1, False, False)

        # Slot5
        if( subSlot1 == 5 or subSlot2 == 5 or subSlot3 == 5 or subSlot4 == 5) :
            iSouthY = int(self.iH * (0.15 + 0.01 * self.dice.get(10,"RndScr.")))
            iHeight = int(self.iH * (0.25 + 0.01 * self.dice.get(18,"RndScr."))) 
            iWestX = int(self.iW * (0.28 + 0.01 * self.dice.get(6,"RndScr.")))
            iWidth = int(self.iW * (0.30 + 0.01 * self.dice.get(14,"RndScr.")))
            iGrain = 4 + self.dice.get(2,"RndScr.")
            self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iGrain, 3, self.iRoundFlags, self.iTerrainFlags, tinyExp, tinyExp, True, 10, -1, False, False)

        # Slot6
        if( subSlot1 == 6 or subSlot2 == 6 or subSlot3 == 6 or subSlot4 == 6) :
            iSouthY = int(self.iH * (0.46 + 0.01 * self.dice.get(9,"RndScr.")))
            iHeight = int(self.iH * (0.21 + 0.01 * self.dice.get(11,"RndScr."))) 
            iWestX = int(self.iW * (0.29 + 0.01 * self.dice.get(6,"RndScr.")))
            iWidth = int(self.iW * (0.27 + 0.01 * self.dice.get(18,"RndScr.")))
            iGrain = 4 + self.dice.get(2,"RndScr.")
            self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iGrain, 10, self.iRoundFlags, self.iTerrainFlags, tinyExp, tinyExp, True, 5, -1, False, False) 

        #Main Landmasses
        iSouthY = int(self.iH * 0.03)
        iNorthY = int(self.iH * 0.97)
        iHeight = iNorthY - iSouthY + 1
        iWestX = 0
        iEastX = self.iW - 1
        iWidth = iEastX - iWestX + 1
        self.generatePlotsInRegion(mainWater, iWidth, iHeight, iWestX, iSouthY, cGrain, 3, self.iRoundFlags, self.iTerrainFlags, xExp, yExp, True, 15, rGrain, False, False)

        return self.wholeworldPlotTypes

#    
# END class R_ContinentalMultilayeredFractal
#
#-----------------------------------------------------------------------------
# AUTHORS: Oleg Giwodiorow (Refar)
# Subclassing my adjusted R_MultilayeredFractal, in order to be able to set
# boundaries for the island region - i dont wont them to reach to far in the
# ice caps.
#-----------------------------------------------------------------------------
class R_ArchipelagoMultilayeredFractal(R_MultilayeredFractal):
    #-------------------------------------------------------------------------
    # MultilayeredFractal class, controlling function.
    #
    def generatePlotsByRegion(self):

        cGrain = 4
        sea = self.gc.getSeaLevelInfo(self.map.getSeaLevel()).getSeaLevelChange()
        sea = min(sea, 5)
        sea = max(sea, -5)
        iWater = 80 + sea

        # 0 Duell, 1 Tiny, 2 Small, 3 Standard, 4 Large, 5 Huge
        sizekey = _sas_world_size_base_index(self.map.getWorldSize())
        if ( sizekey > 3 ) :
            xExp = 8
            yExp = 7
            tinyExp = 4
        else :
            xExp = 7
            yExp = 6
            tinyExp = 3

        iSouthY = int(self.iH * 0.08)
        iNorthY = int(self.iH * 0.92)
        iHeight = iNorthY - iSouthY + 1
        iWestX = 0
        iEastX = self.iW - 1
        iWidth = iEastX - iWestX + 1

        print("Archipelago: [S,N,E,W] = [", iSouthY, iNorthY, iEastX, iWestX, "] Grain: ", cGrain)

        self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, cGrain, 3, self.iRoundFlags, self.iTerrainFlags, xExp, yExp, True, 15, 5, False, False)

        return self.wholeworldPlotTypes
#    
#END class R_ArchipelagoMultilayeredFractal
#
#-----------------------------------------------------------------------------
# From CivIV Pangaea.py
# AUTHORS: Bob Thomas (Sirian)
# Copyright (c) 2005 Firaxis Games
# My Changes: Slightly more randon mesults. Mainly by removing the
# cohesion check, allowing for more bottlenecks or even occasional non-coherent
# main landmass. Also only the 'Natural Coastline'-setting will be used.
#-----------------------------------------------------------------------------
class R_PangaeaMultilayeredFractal(CvMapGeneratorUtil.MultilayeredFractal):
    #-------------------------------------------------------------------------
    # MultilayeredFractal class, controlling function.
    #
    def generatePlotsByRegion(self, pangaea_type):
        # The following grain matrix is specific to Pangaea.py
        sizekey = self.map.getWorldSize()
        sizevalues = {
            WorldSizeTypes.WORLDSIZE_ARENA:     3,
            WorldSizeTypes.WORLDSIZE_DUEL:      3,
            WorldSizeTypes.WORLDSIZE_TINY:      3,
            WorldSizeTypes.WORLDSIZE_SMALL:     4,
            WorldSizeTypes.WORLDSIZE_STANDARD:  4,
            WorldSizeTypes.WORLDSIZE_LARGE:     4,
            WorldSizeTypes.WORLDSIZE_HUGE:      5
            }
        grain = sas_lookup_world_size(sizekey, sizevalues)

        # Sea Level adjustment (from user input), limited to value of 5%.
        sea = self.gc.getSeaLevelInfo(self.map.getSeaLevel()).getSeaLevelChange()
        sea = min(sea, 5)
        sea = max(sea, -5)

        # The following regions are specific to Pangaea.py
        mainWestLon = 0.05 + 0.01 * self.dice.get(12, "Rnd, Pangea, Python")
        mainEastLon = 0.75 + 0.01 * self.dice.get(23, "Rnd, Pangea, Python")
        mainSouthLat = 0.15 + 0.01 * self.dice.get(10, "Rnd, Pangea, Python")
        mainNorthLat = 0.75 + 0.01 * self.dice.get(11, "Rnd, Pangea, Python")
        subcontinentDimension = 0.4 
        bSouthwardShift = False

        # Shift mainland north or south?
        global shiftRoll
        shiftRoll = self.dice.get(2, "RndScriptUtil, Pangea, PYTHON")
        subcontinentDimension = 0.3
        if shiftRoll == 1:
            mainNorthLat += 0.065
            mainSouthLat += 0.065
        else:
            mainNorthLat -= 0.065
            mainSouthLat -= 0.065
            bSouthwardShift = True

        print("Pangaea: Dientions [W-E,S-N] = [", mainWestLon, mainEastLon, mainSouthLat, mainNorthLat, "] Shift South: ", bSouthwardShift)

        # Define potential subcontinent slots (regional definitions).
        numSubcontinents = 4 + self.dice.get(3, "RandomScriptUtil PYTHON")
        # List values: [westLon, southLat, vertRange, horzRange, southShift]
        scValues = [[0.05, 0.575, 0.0, 0.0, 0.15],
                    [0.05, 0.275, 0.0, 0.0, 0.15],
                    [0.2, 0.175, 0.0, 0.0, 0.15],
                    [0.5, 0.175, 0.0, 0.0, 0.15],
                    [0.65, 0.575, 0.0, 0.0, 0.15],
                    [0.65, 0.275, 0.0, 0.0, 0.15],
                    [0.2, 0.675, 0.0, 0.0, 0.15],
                    [0.5, 0.675, 0.0, 0.0, 0.15]
                    ]

        # Generate the main land mass, first pass (to vary shape).
        mainWestX = int(self.iW * mainWestLon)
        mainEastX = int(self.iW * mainEastLon)
        mainNorthY = int(self.iH * mainNorthLat)
        mainSouthY = int(self.iH * mainSouthLat)
        mainWidth = mainEastX - mainWestX + 1
        mainHeight = mainNorthY - mainSouthY + 1

        mainWater = 55+sea

        self.generatePlotsInRegion(mainWater, mainWidth, mainHeight, mainWestX, mainSouthY, 2, grain, self.iHorzFlags, self.iTerrainFlags, -1, -1, True, 15, 2, False, False)

        # Second pass (to ensure cohesion).
        # Will be droped in favor of Randomness in about 80% of cases.
        if ( 0 != self.dice.get(5, "RandomScriptUtil, Pangea, Python") ) :
            second_layerHeight = mainHeight/2
            second_layerWestX = mainWestX + mainWidth/10
            second_layerEastX = mainEastX - mainWidth/10
            second_layerWidth = second_layerEastX - second_layerWestX + 1
            second_layerNorthY = mainNorthY - mainHeight/4
            second_layerSouthY = mainSouthY + mainHeight/4
            second_layerWater = 60+sea
            self.generatePlotsInRegion(second_layerWater, second_layerWidth, second_layerHeight, second_layerWestX, second_layerSouthY, 1, grain, self.iHorzFlags, self.iTerrainFlags, -1, -1, True, 15, 2, False, False)

        # Add subcontinents.
        while numSubcontinents > 0:
            # Choose a slot for this subcontinent.
            if len(scValues) > 1:
                scIndex = self.dice.get(len(scValues), "RndScriptUtil PYTHON")
            else:
                scIndex = 0
            [scWestLon,
             scSouthLat,
             scVertRange,
             scHorzRange,
             scSouthShift] = scValues[scIndex]
            scWidth = int(subcontinentDimension * self.iW)
            scHeight = int(subcontinentDimension * self.iH)
            scHorzShift = 0
            scVertShift = 0
            if scHorzRange > 0.0:
                scHorzShift = self.dice.get(int(self.iW * scHorzRange), "RndScriptUtil PYTHON")
            if scVertRange > 0.0:
                scVertShift = self.dice.get(int(self.iW * scVertRange), "RndScriptUtil PYTHON")
            scWestX = int(self.iW * scWestLon) + scHorzShift
            scEastX = scWestX + scWidth
            if scEastX >= self.iW: # Trouble! Off the right hand edge!
                while scEastX >= self.iW:
                    scWidth -= 1
                    scEastX = scWestX + scWidth
            scSouthY = int(self.iH * scSouthLat) + scVertShift
            # Check for southward shift.
            if bSouthwardShift:
                scSouthY -= int(self.iH * scSouthShift)
            scNorthY = scSouthY + scHeight
            if scNorthY >= self.iH: # Trouble! Off the top edge!
                while scNorthY >= self.iH:
                    scHeight -= 1
                    scNorthY = scSouthY + scHeight

            scShape = self.dice.get(5, "RandomScriptUtil PYTHON")
            if scShape == 0:    # Regular subcontinent.
                scWater = 55+sea
                scGrain = 1
                scRift = -1
            elif scShape > 2:   # Irregular subcontinent.
                scWater = 66+sea
                scGrain = 2
                scRift = 2
            else:               # scShape 1 and 2, Archipelago subcontinent.
                scWater = 77+sea
                scGrain = grain
                scRift = -1

            self.generatePlotsInRegion(scWater, scWidth, scHeight, scWestX, scSouthY, scGrain, grain, self.iRoundFlags, self.iTerrainFlags, 6, 6, True, 7, scRift, False, False)

            del scValues[scIndex]
            numSubcontinents -= 1

        # All regions have been processed. Plot Type generation completed.
        return self.wholeworldPlotTypes
#    
#END class R_PangaeaultilayeredFractal
#
#-----------------------------------------------------------------------------
# From CivIV Terra.py
# AUTHORS: Bob Thomas (Sirian)
# Copyright (c) 2005 Firaxis Games
# My Changes: Slightly more random results. Occasionaly removing the second 
# pass on eurasia. Added some additional random rolls, for slightly less
# earthlike appearance. Also slightly higher chance of getting more additional
# small landmasses.       
#-----------------------------------------------------------------------------
class R_TerraMultilayeredFractal(CvMapGeneratorUtil.MultilayeredFractal):
    #-------------------------------------------------------------------------
    # Sirian's MultilayeredFractal class, controlling function.
    #
    def generatePlotsByRegion(self):
        # The following grain matrix is specific to Terra.py
        sizekey = self.map.getWorldSize()
        sizevalues = {
            WorldSizeTypes.WORLDSIZE_ARENA:     (3,2,1,2),
            WorldSizeTypes.WORLDSIZE_DUEL:      (3,2,1,2),
            WorldSizeTypes.WORLDSIZE_TINY:      (3,2,1,2),
            WorldSizeTypes.WORLDSIZE_SMALL:     (4,2,1,2),
            WorldSizeTypes.WORLDSIZE_STANDARD:  (4,2,1,2),
            WorldSizeTypes.WORLDSIZE_LARGE:     (4,2,1,2),
            WorldSizeTypes.WORLDSIZE_HUGE:      (5,2,1,2)
            }
        (archGrain, contGrain, gaeaGrain, eurasiaGrain) = sas_lookup_world_size(sizekey, sizevalues)

        # Sea Level adjustment (from user input), limited to value of 5%.
        sea = self.gc.getSeaLevelInfo(self.map.getSeaLevel()).getSeaLevelChange()
        sea = min(sea, 7)
        sea = max(sea, -7)

        # The following regions are specific to Terra.py
        newworldWestLon = 0.02 + 0.01 * self.dice.get(6, "Rnd,Terra,Python")
        # <!-- custom: Refar's later quadrant shifts could move randomized regions outside the compositor: New World east reached 1.02 and Eurasia south reached -0.02.
        # Keep the randomized margins but constrain their bases to 0.36-0.39 and 0.40-0.46, so +0.60 remains below 1 and -0.40 remains at or above 0. See KI#277. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
        newworldEastLon = 0.36 + 0.01 * self.dice.get(4, "Rnd,Terra,Python")
        eurasiaWestLon = 0.40 + 0.01 * self.dice.get(7, "Rnd,Terra,Python")  
        eurasiaEastLon = 0.93 + 0.01 * self.dice.get(6, "Rnd,Terra,Python")
        eurasiaNorthLat = 0.95
        eurasiaSouthLat = 0.40 + 0.01 * self.dice.get(7, "Rnd,Terra,Python")
        thirdworldDimension = 0.125
        thirdworldNorthLat = 0.35 + 0.01 * self.dice.get(6, "Rnd,Terra,Python")
        thirdworldSouthLat = 0.03 + 0.01 * self.dice.get(5, "Rnd,Terra,Python")
        subcontinentLargeHorz = 0.2
        subcontinentLargeVert = 0.32
        subcontinentLargeNorthLat = 0.57
        subcontinentLargeSouthLat = 0.28
        subcontinentSmallDimension = 0.125
        subcontinentSmallNorthLat = 0.525
        subcontinentSmallSouthLat = 0.4

        secondPass = self.dice.get(2, "(Python RandomScriptUtil)")

        # Dice rolls to randomize the quadrants (specific to Terra.py's regions)
        roll1 = self.dice.get(2, "RndMapUtil, Terra, PYTHON")
        if roll1 == 1:
            eurasiaNorthLat -= 0.4
            eurasiaSouthLat -= 0.4
            thirdworldNorthLat += 0.6
            thirdworldSouthLat += 0.6
            subcontinentLargeNorthLat += 0.12
            subcontinentLargeSouthLat += 0.12
            subcontinentSmallNorthLat += 0.075
            subcontinentSmallSouthLat += 0.075
        roll2 = self.dice.get(2, "RndMapUtil, Terra, PYTHON")
        if roll2 == 1:
            newworldWestLon += 0.6
            newworldEastLon += 0.6
            eurasiaWestLon -= 0.4
            eurasiaEastLon -= 0.4

        eurasiaWestX = int(self.iW * eurasiaWestLon)
        eurasiaEastX = int(self.iW * eurasiaEastLon)
        eurasiaNorthY = int(self.iH * eurasiaNorthLat)
        eurasiaSouthY = int(self.iH * eurasiaSouthLat)
        eurasiaWidth = eurasiaEastX - eurasiaWestX + 1
        eurasiaHeight = eurasiaNorthY - eurasiaSouthY + 1
        eurasiaWater = 55+sea

        print("Terra: Quadrants Rolls: ", roll1, roll2, " Second Pass Eurasia: ", secondPass)

        # Eurasia, second layer (to increase pangaea-like cohesion).
        # Will be removed 50% of cases for more diversity.
        if (secondPass == 0) :
            twHeight = eurasiaHeight/2
            twWestX = eurasiaWestX + eurasiaWidth/10
            twEastX = eurasiaEastX - eurasiaWidth/10
            twWidth = twEastX - twWestX + 1
            twNorthY = eurasiaNorthY - eurasiaHeight/4
            twSouthY = eurasiaSouthY + eurasiaHeight/4
            twWater = 60+sea
            twGrain = 1
            twRift = 2

            self.generatePlotsInRegion(twWater, twWidth, twHeight, twWestX, twSouthY, twGrain, archGrain, self.iHorzFlags, self.iTerrainFlags, -1, -1, True, 11, twRift, False, False)

        # Simulate the Old World - a large continent akin to Earth's Eurasia.
        # Set dimensions of the Old World region (specific to Terra.py)
        self.generatePlotsInRegion(eurasiaWater, eurasiaWidth, eurasiaHeight, eurasiaWestX, eurasiaSouthY, eurasiaGrain, archGrain, self.iHorzFlags, self.iTerrainFlags, -1, -1, True, 11, 2, False, False)

        # Simulate the New World. Land masses akin to American continents.
        # First simulate North America
        nwWestX = int(self.iW * newworldWestLon)
        nwEastX = int(self.iW * newworldEastLon)
        nwNorthY = int(self.iH * 0.85)
        nwSouthY = int(self.iH * 0.52)
        nwWidth = nwEastX - nwWestX + 1
        nwHeight = nwNorthY - nwSouthY + 1

        nwWater = 61+sea
        nwGrain = 1
        nwRift = -1

        self.generatePlotsInRegion(nwWater, nwWidth, nwHeight, nwWestX, nwSouthY, nwGrain, archGrain, self.iVertFlags, self.iTerrainFlags, 6, 6, True, 7, nwRift, False, False)

        # Now simulate South America
        nwsRoll = self.dice.get(2, "RandomScriptUtil PYTHON")
        nwsVar = 0.0
        if nwsRoll == 1:
            nwsVar = 0.05
        nwsWestX = nwWestX + int(self.iW * (0.08 - nwsVar))
        # Not as wide as the north
        nwsEastX = nwEastX - int(self.iW * (0.03 + nwsVar))
        nwsNorthY = int(self.iH * 0.47)
        nwsSouthY = int(self.iH * 0.25)
        nwsWidth = nwsEastX - nwsWestX + 1
        nwsHeight = nwsNorthY - nwsSouthY + 1

        nwsWater = 55+sea
        nwsGrain = 1
        nwsRift = -1

        self.generatePlotsInRegion(nwsWater, nwsWidth, nwsHeight, nwsWestX, nwsSouthY, nwsGrain, archGrain, self.iRoundFlags, self.iTerrainFlags, 6, 6, True, 5, nwsRift, False, False)

        nwpWestX = nwWestX + int(self.iW * (0.1 - nwsVar))
        # Not as wide as the north
        nwpEastX = nwEastX - int(self.iW * (0.07 + nwsVar))
        nwpNorthY = int(self.iH * 0.3)
        nwpSouthY = int(self.iH * 0.18)
        nwpWidth = nwpEastX - nwpWestX + 1
        nwpHeight = nwpNorthY - nwpSouthY + 1

        nwpWater = 67+sea
        nwpGrain = 1
        nwpRift = -1

        self.generatePlotsInRegion(nwpWater, nwpWidth, nwpHeight, nwpWestX, nwpSouthY, nwpGrain, archGrain, self.iVertFlags, self.iTerrainFlags, 6, 5, True, 3, nwpRift, False, False)

        # Now the Yukon
        twWidth = int(self.iW * 0.15)
        twWestX = nwWestX
        boreal = self.dice.get(2, "RandomScriptUtil PYTHON")
        if boreal == 1:
            twWestX += int(self.iW * 0.15)
        twEastX = twWestX + twWidth
        twNorthY = int(self.iH * 0.93)
        twSouthY = int(self.iH * 0.75)
        twHeight = twNorthY - twSouthY + 1

        twWater = 68+sea
        twGrain = 2
        twRift = -1

        self.generatePlotsInRegion(twWater, twWidth, twHeight, twWestX, twSouthY, twGrain, archGrain, self.iRoundFlags, self.iTerrainFlags, 6, 5, True, 5, twRift, False, False)

        # Now add a random region of arctic islands
        twWidth = int(thirdworldDimension * self.iW)
        twHeight = int(thirdworldDimension * self.iH)
        if boreal == 0: 
            twEastX = nwEastX
            twWestX = twEastX - twWidth
        else:
            twWestX = nwWestX
            twEastX = twWestX + twWidth
        twNorthY = int(self.iH * 0.975)
        twSouthY = int(self.iH * 0.85)

        twWater = 76+sea
        twGrain = archGrain
        twRift = -1

        self.generatePlotsInRegion(twWater, twWidth, twHeight, twWestX, twSouthY, twGrain, archGrain, self.iHorzFlags, self.iTerrainFlags, 6, 5, True, 5, twRift, False, False)

        # Now simulate Central America
        nwcVar = 0.0
        nwcRoll = self.dice.get(2, "RandomScriptUtil PYTHON")
        if nwcRoll == 1:
            nwcVar = 0.04
        nwcWidth = int(self.iW * 0.06)
        nwcWestX = nwWestX + int(self.iW * (0.1 + nwcVar))
        nwcEastX = nwcWestX + nwcWidth
        nwcNorthY = int(self.iH * 0.6)
        nwcSouthY = int(self.iH * 0.42)
        nwcHeight = nwcNorthY - nwcSouthY + 1

        nwcWater = 60+sea
        nwcGrain = 1
        nwcRift = -1

        self.generatePlotsInRegion(nwcWater, nwcWidth, nwcHeight, nwcWestX, nwcSouthY, nwcGrain, archGrain, self.iVertFlags, self.iTerrainFlags, 6, 5, True, 5, nwcRift, False, False)

        # Now the Carribean islands
        carVar = 0.0
        if nwcRoll == 1:
            carVar = 0.15
        twWidth = int(0.15 * self.iW)
        twEastX = nwEastX - int(carVar * self.iW)
        twWestX = twEastX - twWidth
        twNorthY = int(self.iH * 0.55)
        twSouthY = int(self.iH * 0.47)
        twHeight = twNorthY - twSouthY + 1

        twWater = 75+sea
        twGrain = archGrain + 1
        twRift = -1

        self.generatePlotsInRegion(twWater, twWidth, twHeight, twWestX, twSouthY, twGrain, archGrain, 0, self.iTerrainFlags, 6, 5, True, 3, twRift, False, False)

        # Add subcontinents to the Old World, one large, one small. (Terra.py)
        # Subcontinents can be akin to pangaea, continents, or archipelago.
        # The large adds an amount of land akin to subSaharan Africa.
        # The small adds an amount of land akin to South Pacific islands.
        scLargeWidth = int(subcontinentLargeHorz * self.iW)
        scLargeHeight = int(subcontinentLargeVert * self.iH)
        scRoll = self.dice.get((eurasiaWidth - scLargeWidth), "RndMapUtil, Terra, PYTHON")
        scWestX = eurasiaWestX + scRoll
        scEastX = scWestX + scLargeWidth
        scNorthY = int(self.iH * subcontinentLargeNorthLat)
        scSouthY = int(self.iH * subcontinentLargeSouthLat)

        scShape = self.dice.get(4, "RndMapUtil, Terra, PYTHON")
        if scShape > 1:    # Massive subcontinent! (Africa style)
            scWater = 55+sea
            scGrain = 1
            scRift = 2
        elif scShape == 1: # Standard subcontinent.
            scWater = 66+sea
            scGrain = 2
            scRift = 2
        else:              # scShape == 0, Archipelago subcontinent.
            scWater = 77+sea
            scGrain = archGrain
            scRift = -1

        self.generatePlotsInRegion(scWater, scLargeWidth, scLargeHeight, scWestX, scSouthY, scGrain, archGrain, self.iRoundFlags, self.iTerrainFlags, 6, 6, True, 7, scRift, False, False)

        scSmallWidth = int(subcontinentSmallDimension * self.iW)
        scSmallHeight = int(subcontinentSmallDimension * self.iH)
        endless = 1
        while endless == 1:
            # Prevent excessive overlap of the two subcontinents.
            scsRoll = self.dice.get((eurasiaWidth - scSmallWidth), "RandomScriptUtil PYTHON")
            scsWestX = eurasiaWestX + scsRoll
            if abs((scsWestX + self.iW/12) - scWestX) > self.iW/8:
                break
        scsEastX = scsWestX + scSmallWidth
        scsNorthY = int(self.iH * subcontinentSmallNorthLat)
        scsSouthY = int(self.iH * subcontinentSmallSouthLat)

        scsShape = self.dice.get(4, "RandomScriptUtil PYTHON")
        if scsShape == 2:   # Massive subcontinent!
            scsWater = 55+sea
            scsGrain = 1
            scsRift = 2
        elif scsShape == 1: # Standard subcontinent. (India style).
            scsWater = 66+sea
            scsGrain = 2
            scsRift = 2
        else:               # scsShape == 0 or 3, Archipelago subcontinent.
            scsWater = 77+sea
            scsGrain = archGrain
            scsRift = -1

        self.generatePlotsInRegion(scsWater, scSmallWidth, scSmallHeight, scsWestX, scsSouthY, scsGrain, archGrain, self.iHorzFlags, self.iTerrainFlags, 6, 5, True, 5, scsRift, False, False)

        # Now simulate random lands akin to Australia and Antarctica
        extras = 3 + self.dice.get(6, "RandomScriptUtil PYTHON")
        for loop in range(extras):
            # 3 to 6 of these regions.
            twWidth = int(thirdworldDimension * self.iW)
            twHeight = int(thirdworldDimension * self.iH)
            twVertRange = int(0.3 * self.iH) - twHeight
            twRoll = self.dice.get((eurasiaWidth - twWidth), "RandomScriptUtil PYTHON")
            twWestX = eurasiaWestX + twRoll
            twEastX = scWestX + scLargeWidth
            twVertRoll = self.dice.get(twVertRange, "RandomScriptUtil PYTHON")
            twNorthY = int(self.iH * thirdworldNorthLat) + twVertRoll
            twSouthY = int(self.iH * thirdworldSouthLat) + twVertRoll

            twShape = self.dice.get(3, "RandomScriptUtil PYTHON")
            if twShape == 2:   # Massive subcontinent!
                twWater = 60+sea
                twGrain = 1
                twRift = 2
            elif twShape == 1: # Standard subcontinent.
                twWater = 65+sea
                twGrain = 2
                twRift = 2
            else:              # twShape == 0, Archipelago subcontinent.
                twWater = 70+sea
                twGrain = archGrain
                twRift = -1

            self.generatePlotsInRegion(twWater, twWidth, twHeight, twWestX, twSouthY, twGrain, archGrain, self.iHorzFlags, self.iTerrainFlags, 6, 5, True, 5, twRift, False, False)

        # All regions have been processed. Plot Type generation completed.
        return self.wholeworldPlotTypes
#    
#END class R_TerraMultilayeredFractal
#
#-----------------------------------------------------------------------------
# From CivIV Big_and_Small.py
# AUTHORS: Bob Thomas (Sirian)
# Copyright (c) 2005 Firaxis Games
# My Changes: Random Rolls instead of User Input Options. Allways create
# overlaping continental and islands Regions. Removed unused code for
# separate regions. Added check for SeaLevelChange(). Added code to
# Randomly size and place the continental Area filling 40 to 60% of the map in
# West-East direction. Adjusted the bounds of the island region, to prevent it
# from reaching to far into the ice caps.
# Also place islands first, then the Landmass - when the layers will be
# copied over each other, the big land masses plots will be on top now.
#-----------------------------------------------------------------------------
class R_BnSMultilayeredFractal(CvMapGeneratorUtil.MultilayeredFractal):
    #-------------------------------------------------------------------------
    # MultilayeredFractal class, controlling function.
    #
    def generatePlotsByRegion(self):
        global xShiftRoll
        xShiftRoll = self.dice.get(2, "Python, RndMapUtil, Bns")
        iContinentsGrain = 1 + self.dice.get(3, "Python, RndMapUtil, Bns")
        if (iContinentsGrain > 2) :
            iContinentsGrain = 1
        iIslandsGrain = 4
        contiPart = 0.4 + 0.01 * self.dice.get(21, "Python, RndMapUtil, Bns")
        contiShift = 0.0 + 0.01 * self.dice.get(100, "Python, RndMapUtil, Bns")
        if ( contiShift >= 0.69 ) :
            contiShift = ( contiShift - 0.69 ) * 0.3
        if ( contiPart + contiShift > 0.98) :
            contiShift = 0.98 - contiPart

        print("Big And Small: Continental Rate: ", contiPart, "Continental Shift, West Boundary: ", contiShift, " CGrain, IGrain: ", iContinentsGrain, iIslandsGrain)

        # Since there will be two layers in all areas, reduce the amount of
        # Land Plots
        sea = self.gc.getSeaLevelInfo(self.map.getSeaLevel()).getSeaLevelChange()
        sea = min(sea, 5)
        sea = max(sea, -5)
        iWater = 80 + sea

        sizekey = _sas_world_size_base_index(self.map.getWorldSize())
        if ( sizekey > 3 ) :
            xExp = 7
            yExp = 7
        else :
            xExp = 7
            yExp = 6

        # Add the Islands.
        iSouthY = int(self.iH * 0.08)
        iNorthY = int(self.iH * 0.92)
        iHeight = iNorthY - iSouthY + 1
        iWestX = 0
        iEastX = self.iW - 1
        iWidth = iEastX - iWestX + 1
        self.generatePlotsInRegion(min(iWater+5, 90), iWidth, iHeight, iWestX, iSouthY, iIslandsGrain, 5, self.iRoundFlags, self.iTerrainFlags, xExp, yExp, True, 15, -1, False, False)

        # North and South dimensions always fill almost the entire vertical span
        iSouthY = int(self.iH * 0.05)
        iNorthY = int(self.iH * 0.95)
        iHeight = iNorthY - iSouthY + 1
        # East-West dimension will be 40 to 60% of the horizontal span.
        iWidth = int(self.iW * contiPart)
        iWestX = int(self.iW * contiShift)
        self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iContinentsGrain, 4, self.iRoundFlags, self.iTerrainFlags, xExp, yExp, True, 15, -1, False, False)

        # All regions have been processed. Plot Type generation completed.
        return self.wholeworldPlotTypes
#    
#END class R_BnSMultilayeredFractal
#
#-----------------------------------------------------------------------------
# From CivIV Medium_and_Small.py
# AUTHORS: Bob Thomas (Sirian)
# Copyright (c) 2005 Firaxis Games
# My Changes: Random Rolls instead of User Input Options. Separate Overlap
# settings for west and east. Add islands before Continents (big grain terrain
# overrides small grain terrain). Adjusted boundaries to prevent islands in the
# icescaps. Fixed a copy/paste bug in the 'add tinies loop' (The way it was
# before tinies would allways have same X and Y coordinate...)
# Added additional not random patches of tiny islands, to prevent islands
# Regions from looking 'boxed'.
#-----------------------------------------------------------------------------
class R_MnSMultilayeredFractal(CvMapGeneratorUtil.MultilayeredFractal):
    #-------------------------------------------------------------------------
    # MultilayeredFractal class, controlling function.
    #
    def generatePlotsByRegion(self):
        global yShiftRoll1
        global yShiftRoll2
        yShiftRoll1 = self.dice.get(2, "Python, RndMapUtil, Mns")
        yShiftRoll2 = self.dice.get(4, "Python, RndMapUtil, Mns")
        # <!-- custom: Refar used comparison expressions where it meant to coerce rolls 2/3 to the opposite binary orientation.
        # Assigning the intended value fixes the 25% case where both map halves shifted the same way. See KI#276. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
        if ( yShiftRoll2 > 1 ) :
            if ( yShiftRoll1 == 0 ) :
                yShiftRoll2 = 1
            else:
                yShiftRoll2 = 0
        iContinentsGrainWest = 1 + self.dice.get(2, "Python, RndMapUtil, Mns")
        iContinentsGrainEast = 1 + self.dice.get(2, "Python, RndMapUtil, Mns") 
        iIslandsGrainWest = 3 + self.dice.get(2, "Python, RndMapUtil, Mns") 
        iIslandsGrainEast = 3 + self.dice.get(2, "Python, RndMapUtil, Mns")	
        contiPartWest = 0.5 + 0.01 * self.dice.get(16, "Python, RndMapUtil")	
        contiPartEast = 0.5 + 0.01 * self.dice.get(16, "Python, RndMapUtil")

        print("Medium And Small: Continental Rate (W/E): ", contiPartWest, contiPartEast, "ShiftRoll (W/E): ", yShiftRoll1, yShiftRoll2, "CGrain (W/E): ", iContinentsGrainWest, iContinentsGrainEast, "IGrain (W/E): ", iIslandsGrainWest, iIslandsGrainEast)

        # Look up for Sealevel Changes
        sea = self.gc.getSeaLevelInfo(self.map.getSeaLevel()).getSeaLevelChange()
        sea = min(sea, 5)
        sea = max(sea, -5)

        # Add a few random patches of Tiny Islands first.
        numTinies = 2 + self.dice.get(3, "Python, RndMapUtil, Mns")
        if numTinies:
            for tiny_loop in range(numTinies):
                tinyWestLon = 0.01 * self.dice.get(84, "Python, RndMapUtil, Mns")
                tinyWestX = int(self.iW * tinyWestLon )
                tinySouthLat = 0.01 * self.dice.get(66, "Python, RndMapUtil")
                tinySouthLat = max(0.10, tinySouthLat)
                tinySouthY = int(self.iH * tinySouthLat)
                tinyWidth = int(self.iW * 0.15)
                tinyHeight = int(self.iH * 0.15)

                self.generatePlotsInRegion((78 + sea), tinyWidth, tinyHeight, tinyWestX, tinySouthY, 4, 3, 0, self.iTerrainFlags, 6, 5, True, 5, -1, False, False)

        # Western Region. Islands.
        iSouthY = int(self.iH * 0.08)
        iNorthY = int(self.iH * 0.92) 
        yExp = 5
        iWater = 78 + sea
        if yShiftRoll1 :
                iNorthY = int(self.iH * (1.1 - contiPartWest))
        else :
                iSouthY = int(self.iH * (contiPartWest - 0.1))

        iWestX = int(self.iW * 0.10)
        iEastX = int(self.iW * 0.40)
        iWidth = iEastX - iWestX + 1
        iHeight = iNorthY - iSouthY + 1	
        self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iIslandsGrainWest, 5, self.iRoundFlags, self.iTerrainFlags, 6, yExp, True, 15, -1, False, False)

        # Additional Tinies to fight 'Boxed Look'
        tinyWestX = iWestX - int(self.iW * 0.06 )
        tinySouthY = iSouthY + int(self.iH * 0.06)
        tinyWidth = int(self.iW * 0.1)
        tinyHeight = int(self.iH * 0.1)
        self.generatePlotsInRegion((83 + sea), tinyWidth, tinyHeight, tinyWestX, tinySouthY, 4, 3, self.iRoundFlags, self.iTerrainFlags, 6, 5, True, 5, -1, False, False)
        tinyWestX = iEastX - int(self.iW * 0.04 )
        tinySouthY = iSouthY + int(self.iH * 0.03)
        tinyWidth = int(self.iW * 0.13)
        tinyHeight = int(self.iH * 0.1)
        self.generatePlotsInRegion((83 + sea), tinyWidth, tinyHeight, tinyWestX, tinySouthY, 4, 3, self.iRoundFlags, self.iTerrainFlags, 6, 5, True, 5, -1, False, False)

        # Continents
        iSouthY = int(self.iH * 0.05)
        iNorthY = int(self.iH * 0.95)
        if yShiftRoll1 :
                iSouthY = int(self.iH * (0.95 - contiPartWest))
        else :
                iNorthY = int(self.iH * (0.05 + contiPartWest))	

        iWestX = int(self.iW * 0.03)
        iEastX = int(self.iW * 0.47)
        iWidth = iEastX - iWestX + 1
        iHeight = iNorthY - iSouthY + 1
        self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iContinentsGrainWest, 4, self.iRoundFlags, self.iTerrainFlags, 6, yExp, True, 15, -1, False, False)

        # Eastern Region. Islands.
        iSouthY = int(self.iH * 0.08)
        iNorthY = int(self.iH * 0.92)
        yExp = 5
        iWater = 78 + sea
        if yShiftRoll2 :
                iNorthY = int(self.iH * (1.1 - contiPartEast))
        else :
                iSouthY = int(self.iH * (contiPartEast - 0.1))

        iWestX = int(self.iW * 0.50)
        iEastX = int(self.iW * 0.90)
        iWidth = iEastX - iWestX + 1
        iHeight = iNorthY - iSouthY + 1	
        self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iIslandsGrainEast, 5, self.iRoundFlags, self.iTerrainFlags, 6, yExp, True, 15, -1, False, False)

        # Additional Tinies to fight 'Boxed Look'
        tinyWestX = iWestX - int(self.iW * 0.06 )
        tinySouthY = iSouthY + int(self.iH * 0.09)
        tinyWidth = int(self.iW * 0.1)
        tinyHeight = int(self.iH * 0.1)
        self.generatePlotsInRegion((83 + sea), tinyWidth, tinyHeight, tinyWestX, tinySouthY, 4, 3, self.iRoundFlags, self.iTerrainFlags, 6, 5, True, 5, -1, False, False)
        tinyWestX = iEastX - int(self.iW * 0.06 )
        tinySouthY = iSouthY + int(self.iH * 0.03)
        tinyWidth = int(self.iW * 0.14)
        tinyHeight = int(self.iH * 0.1)
        self.generatePlotsInRegion((83 + sea), tinyWidth, tinyHeight, tinyWestX, tinySouthY, 4, 3, self.iRoundFlags, self.iTerrainFlags, 6, 5, True, 5, -1, False, False)

        # Continents
        iSouthY = int(self.iH * 0.05)
        iNorthY = int(self.iH * 0.95)
        if yShiftRoll2:
                iSouthY = int(self.iH * (0.95 - contiPartEast))
        else :
                iNorthY = int(self.iH * (0.05 + contiPartEast))	

        iWestX = int(self.iW * 0.53)
        iEastX = int(self.iW * 0.97)
        iWidth = iEastX - iWestX + 1
        iHeight = iNorthY - iSouthY + 1
        self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iContinentsGrainEast, 4, self.iRoundFlags, self.iTerrainFlags, 6, yExp, True, 15, -1, False, False)

        return self.wholeworldPlotTypes

#   
#END class R_MnSMultilayeredFractal
#
#-----------------------------------------------------------------------------
# From CivIV Hemispheres.py
# AUTHORS: Ben Sarsgard
# Copyright (c) 2005 Firaxis Games
# My Changes: Random Rolls instead of User Iput Options. Allways use
# 'Varied Continents'-Option, choosing different CGrains for the Continents.
# Use 2 or 3 Regions, biased towards 3. Also occasionaly split one of the
# Regions roughly along equator. Removed unused code (4+ Splits and other)
#-----------------------------------------------------------------------------
class R_HemiMultilayeredFractal(CvMapGeneratorUtil.MultilayeredFractal):
    #-------------------------------------------------------------------------
    # Regional Variables/Defaults for GeneratePlotsInRegion() from
    # CvMapGeneratorUtil.py
    #
    # [self,]
    # iWaterPercent, 
    # iRegionWidth, iRegionHeight, 
    # iRegionWestX, iRegionSouthY, 
    # iRegionGrain, iRegionHillsGrain, 
    # iRegionPlotFlags, iRegionTerrainFlags, 
    # iRegionFracXExp = -1, iRegionFracYExp = -1, 
    # bShift = True, iStrip = 15, 
    # rift_grain = -1, has_center_rift = False, 
    # invert_heights = False
    #
    def generateIslandRegion(self, numTinies, iWestX, iSouthY, iWidth, iHeight):
        if ( numTinies == 0 ) :
            return 0
        for tiny_loop in range(numTinies):
            vVariance = 0.01 * self.dice.get(9, "Python, RndMapUtil, Hemi.")
            hVariance = 0.01 * self.dice.get(9, "Python, RndMapUtil, Hemi.")
            iGrain = 3 + self.dice.get(3, "Python, RndMapUtil, Hemi.")
            tinyWidth = int(self.iW * (0.10 + hVariance))
            tinyHeight = int(self.iH * (0.10 + vVariance))
            tinyWestX = iWestX + self.dice.get(iWidth - tinyWidth, "Python, RndMapUtil, Hemi.")
            tinySouthY = iSouthY + self.dice.get(iHeight - tinyHeight, "Python, RndMapUtil, Hemi.")
            self.generatePlotsInRegion(80, tinyWidth, tinyHeight, tinyWestX, tinySouthY, iGrain, 3, 0, self.iTerrainFlags, 6, 5, True, 3, -1, False, False)
        return 0

    #-------------------------------------------------------------------------
    #
    #
    def generateContinentRegion(self, iWater, iWidth, iHeight, iWestX, iSouthY, iGrain, xExp):
        self.generatePlotsInRegion(iWater, iWidth, iHeight, iWestX, iSouthY, iGrain, 4, self.iRoundFlags, self.iTerrainFlags, xExp, 6, True, 15, -1, False, False)
        return 0

    #-------------------------------------------------------------------------
    # MultilayeredFractal class, controlling function.
    #
    def generatePlotsByRegion(self):
        global xShiftRoll
        xShiftRoll = self.dice.get(2, "Python, RndMapUtil, Hemispheres")
        hasVSplit = False

        # Generate 2 or 3 Varied Regions
        iCGrain_1 = 1 + self.dice.get(2, "Python, RndMapUtil, Hemispheres")
        iCGrain_2 = 1 + self.dice.get(3, "Python, RndMapUtil, Hemispheres")
        if (iCGrain_2 > 2) :
            iCGrain_2 = 1
        iCGrain_3 = 1 + self.dice.get(3, "Python, RndMapUtil, Hemispheres")
        iWater_1 = 72 + self.dice.get(5, "Python, RndMapUtil, Hemispheres")
        iWater_2 = 72 + self.dice.get(9, "Python, RndMapUtil, Hemispheres")
        iWater_3 = 72 + self.dice.get(7, "Python, RndMapUtil, Hemispheres")
        sea = self.gc.getSeaLevelInfo(self.map.getSeaLevel()).getSeaLevelChange()
        sea = min(sea, 5)
        sea = max(sea, -5)
        iWater_1 += sea
        iWater_2 += sea
        iWater_3 += sea
        regionsOverlap = 0.01 + 0.01 * self.dice.get(3, "Python, RndMapUtil")
        iIslandVariance_1 = self.dice.get(4, "Python, RndMapHelper, Hemi.")
        iIslandVariance_2 = self.dice.get(5, "Python, RndMapHelper, Hemi.")
        iIslandVariance_3 = self.dice.get(3, "Python, RndMapHelper, Hemi.")

        regions = 2 + self.dice.get(3, "Python, RndMapUtil, Hemisph.")
        if (regions > 3) :
            regions = 3
        if (regions == 2):
            tripleSplit = 0
        else :
            tripleSplit = 1

        vSplitRnd = 1 - (2*tripleSplit)
        iVSplit = 0.4
        vSplitRnd += self.dice.get(3, "Python, RndMapUtil, Hemisph.")
        if( vSplitRnd > 1 ) :
            hasVSplit = True
            iVSplit += 0.01 * self.dice.get(20, "Python, RndMapUtil, Hem.")

        print("Hemispheres: TripleSplit: ", tripleSplit, " VSplit, Value: ", hasVSplit, iVSplit, " Overlap: ", regionsOverlap, " CGrains: ", iCGrain_1, iCGrain_2, iCGrain_3)

        # 1.st Split.
        # Add the Continents.
        # Handle horizontal shift for the Continents layer.
        # While the Continents must feet in the region, thw island regions
        # will be allowed to overlap
        xExp = 6
        if tripleSplit:
            if xShiftRoll:
                westShiftC = int(0.33 * self.iW)
                eastShiftC = int(0.33 * self.iW)
                westShiftI = int((0.33 - regionsOverlap) * self.iW)
                eastShiftI = int((0.33 - regionsOverlap) * self.iW)
            else:
                westShiftC = 0
                eastShiftC = int(0.66 * self.iW)
                westShiftI = 0
                eastShiftI = int((0.66 - regionsOverlap) * self.iW)
        else:
            if xShiftRoll:
                westShiftC = int(0.50 * self.iW)
                eastShiftC = 0
                westShiftI = int((0.50 - regionsOverlap) * self.iW)
                eastShiftI = 0
            else:
                westShiftC = 0
                eastShiftC = int(0.50 * self.iW) 
                westShiftI = 0
                eastShiftI = int((0.50 - regionsOverlap) * self.iW)

        # Set Boundaries for the islands.
        iSouthY = int(self.iH * 0.07)
        iNorthY = int(self.iH * 0.93)
        iHeight = iNorthY - iSouthY + 1
        iWestX = westShiftI
        iEastX = self.iW - eastShiftI - 1
        iWidth = iEastX - iWestX
        self.generateIslandRegion((3 + iIslandVariance_1), iWestX, iSouthY, iWidth, iHeight)

        # Continents
        iSouthY = int(self.iH * 0.02)
        iNorthY = int(self.iH * 0.98)
        iHeight = iNorthY - iSouthY + 1
        iWestX = westShiftC
        iEastX = self.iW - eastShiftC - 1
        iWidth = iEastX - iWestX
        # Make a addittional horizontal Split from time to time
        if not hasVSplit :  
            self.generateContinentRegion(iWater_1, iWidth, iHeight, iWestX, iSouthY, iCGrain_1, xExp)
        else :
            iNorthY = int(self.iH * (1 - iVSplit))
            iHeight = iNorthY - iSouthY
            self.generateContinentRegion(iWater_2, iWidth, iHeight, iWestX, iSouthY, iCGrain_2, xExp)
            iSouthY = iNorthY + 1
            iNorthY = int(self.iH * 0.98)
            iHeight = iNorthY - iSouthY
            self.generateContinentRegion(iWater_2, iWidth, iHeight, iWestX, iSouthY, iCGrain_2, xExp)

        # 2.nd Split
        xExp = 6
        if tripleSplit:
            if xShiftRoll:
                westShiftC = 0
                eastShiftC = int(0.66 * self.iW)
                westShiftI = 0
                eastShiftI = int((0.66 - regionsOverlap) * self.iW)
            else:
                westShiftC = int(0.33 * self.iW)
                eastShiftC = int(0.33 * self.iW)
                westShiftI = int((0.33 - regionsOverlap) * self.iW)
                eastShiftI = int((0.33 - regionsOverlap) * self.iW)
        else:
            if xShiftRoll:
                westShiftC = 0
                eastShiftC = int(0.50 * self.iW)
                westShiftI = 0
                eastShiftI = int((0.50 - regionsOverlap) * self.iW)
            else:
                westShiftC = int(0.50 * self.iW)
                eastShiftC = 0
                westShiftI = int((0.50 - regionsOverlap) * self.iW)
                eastShiftI = 0

        iSouthY = int(self.iH * 0.07)
        iNorthY = int(self.iH * 0.93)
        iHeight = iNorthY - iSouthY + 1
        iWestX = westShiftI
        iEastX = self.iW - eastShiftI - 1
        iWidth = iEastX - iWestX
        self.generateIslandRegion((2 + iIslandVariance_2), iWestX, iSouthY, iWidth, iHeight)
        iWestX = westShiftC
        iEastX = self.iW - eastShiftC - 1
        iWidth = iEastX - iWestX
        iSouthY = int(self.iH * 0.02)
        iNorthY = int(self.iH * 0.98)
        iHeight = iNorthY - iSouthY + 1 
        self.generateContinentRegion(iWater_2, iWidth, iHeight, iWestX, iSouthY, iCGrain_2, xExp)

        # 3.rd Split, only if needed
        if tripleSplit:
            xExp = 6
            westShiftC = int(0.66 * self.iW)
            eastShiftC = 0
            westShiftI = int((0.66 - regionsOverlap) * self.iW)
            eastShiftI = 0
            iSouthY = int(self.iH * 0.07)
            iNorthY = int(self.iH * 0.93)
            iHeight = iNorthY - iSouthY + 1
            iWestX = westShiftI
            iEastX = self.iW - eastShiftI
            iWidth = iEastX - iWestX
            self.generateIslandRegion((3 + iIslandVariance_3), iWestX, iSouthY, iWidth, iHeight)
            iWestX = westShiftC
            iEastX = self.iW - eastShiftC
            iWidth = iEastX - iWestX
            iSouthY = int(self.iH * 0.02)
            iNorthY = int(self.iH * 0.98)
            iHeight = iNorthY - iSouthY + 1
            self.generateContinentRegion(iWater_3, iWidth, iHeight, iWestX, iSouthY, iCGrain_3, xExp)

        # All regions have been processed. Plot Type generation completed.
        return self.wholeworldPlotTypes
#    
#END class R_HemiMultilayeredFractal
#
#-----------------------------------------------------------------------------
# From CivIV CvMapGeneratorUtil.py
# AUTHOR: Oleg Giwodiorow (Refar)
# Subclassing terrain generator, to overwrite init functions.
# The desert fractal schall get a lower grain, to generate more coherent
# desert regions
#-----------------------------------------------------------------------------
class R_TerrainGenerator(CvMapGeneratorUtil.TerrainGenerator):
    #-------------------------------------------------------------------------
    #
    #	
    def initFractals(self):
        self.deserts.fracInit(self.iWidth, self.iHeight, max(2, (self.grain_amount - 1)), self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
        self.iDesertTop = self.deserts.getHeightFromPercent(self.iDesertTopPercent)
        self.iDesertBottom = self.deserts.getHeightFromPercent(self.iDesertBottomPercent)

        self.plains.fracInit(self.iWidth, self.iHeight, self.grain_amount+1, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
        self.iPlainsTop = self.plains.getHeightFromPercent(self.iPlainsTopPercent)
        self.iPlainsBottom = self.plains.getHeightFromPercent(self.iPlainsBottomPercent)

        self.variation.fracInit(self.iWidth, self.iHeight, self.grain_amount, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)

        self.terrainDesert = getInfoTypeOrFail("TERRAIN_DESERT")
        self.terrainPlains = getInfoTypeOrFail("TERRAIN_PLAINS")
        self.terrainIce = getInfoTypeOrFail("TERRAIN_SNOW")
        self.terrainTundra = getInfoTypeOrFail("TERRAIN_TUNDRA")
        self.terrainGrass = getInfoTypeOrFail("TERRAIN_GRASS")
#    
#END class R_TettainGenerator
#

#-----------------------------------------------------------------------------
# IMPORTED SCRIPTS END
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Here the previeously chosen generation method will be initialized.
# The scripts have additional random options.
#
def generatePlotTypes():
	NiTextOut("Setting Plot Types (Python RandomScriptMap) ...")
	gc = CyGlobalContext()
	dice = gc.getGame().getMapRand()

	if ( gl_mapType == MAP_TERRA ) : # Terra
		fractal_world = R_TerraMultilayeredFractal()
		return fractal_world.generatePlotsByRegion()
	elif ( gl_mapType == MAP_HEMIS ) : # Hemispheres
		fractal_world = R_HemiMultilayeredFractal()
		return fractal_world.generatePlotsByRegion()
	elif ( gl_mapType == MAP_BIGNS ) : # Big and Small
		fractal_world = R_BnSMultilayeredFractal()
		return fractal_world.generatePlotsByRegion()
	elif ( gl_mapType == MAP_MANDS ) : # Medium and Small
		fractal_world = R_MnSMultilayeredFractal()
		return fractal_world.generatePlotsByRegion()
	elif ( gl_mapType == MAP_ARCHI ) : # Archipelago
		fractal_world = R_ArchipelagoMultilayeredFractal()
		return fractal_world.generatePlotsByRegion()
	elif ( gl_mapType == MAP_PANGA ) : # Pangaea
		fractal_world = R_PangaeaMultilayeredFractal()
		return fractal_world.generatePlotsByRegion(pangaea_type = 0)
	else : # gl_mapType == MAP_FRACT, Fractal Continents
		fractal_world = R_MultilayeredFractal()
		return fractal_world.generatePlotsByRegion(forceCGrain = False)

#-----------------------------------------------------------------------------
#
#	
def generateTerrainTypes():
	NiTextOut("Generating Terrain (Python RandomScriptMap) ...")
	# Prevent Coastal Peacks from bisecting Landmasses.
	# I choose rahter to add plots, that to remove the peaks.
	# Do it before generating terrain Types/Features to make sure,
	# that the created hills can have Snow/Desert/Plain fitting
	# to the climatic region of the world.
	map = CyMap()
	iW = map.getGridWidth()
	iH = map.getGridHeight()
	for plotIndex in range(iW * iH):
		pPlot = map.plotByIndex(plotIndex)
		if ( pPlot.isPeak() and pPlot.isCoastalLand() ):
			replace = False
			iX = pPlot.getX()
			iY = pPlot.getY()
			if(iY < 3 or iY > (iH - 4)) :
				replace = True
			else :
				pP1 = map.plot(iX, iY+1)
				if ( pP1.isWater() and not pP1.isLake() ) :
					pP1.setPlotType(PlotTypes.PLOT_HILLS, true, true)
				pP1 = map.plot(iX, iY-1)
				if ( pP1.isWater() and not pP1.isLake() ) :
					pP1.setPlotType(PlotTypes.PLOT_HILLS, true, true)
				pP1 = map.plot(iX+1, iY)
				if ( pP1.isWater() and not pP1.isLake() ) :
					pP1.setPlotType(PlotTypes.PLOT_HILLS, true, true)
				pP1 = map.plot(iX-1, iY)
				if ( pP1.isWater() and not pP1.isLake() ) :
					pP1.setPlotType(PlotTypes.PLOT_HILLS, true, true)
			if replace :
				pPlot.setPlotType(PlotTypes.PLOT_HILLS, true, true)
	# def __init__(self, iDesertPercent=32, iPlainsPercent=18,
	#              fSnowLatitude=0.7, fTundraLatitude=0.6,
	#              fGrassLatitude=0.1, fDesertBottomLatitude=0.2,
	#              fDesertTopLatitude=0.5, fracXExp=-1,
	#              fracYExp=-1, grain_amount=4):
	# Adjusted by Users Climate Choices.
	# sizekey = CyMap().getWorldSize()
	# Using customized TerrainGenerator with lower dert grain (more
	# coherent desert areas), hence the reduced amount.
	if( _sas_world_size_base_index(map.getWorldSize()) > 2 ) :
		terraingen = R_TerrainGenerator(iDesertPercent = 27, fSnowLatitude = 0.8, fTundraLatitude = 0.68, fDesertBottomLatitude = 0.27, fDesertTopLatitude = 0.43)
	else :
		terraingen = R_TerrainGenerator(iDesertPercent = 27, fSnowLatitude = 0.78, fTundraLatitude = 0.68, fDesertBottomLatitude = 0.26, fDesertTopLatitude = 0.44)
	terrainTypes = terraingen.generateTerrain()
	return terrainTypes

#-----------------------------------------------------------------------------
#
#
def addFeatures():
	NiTextOut("Adding Features (Python RandomScriptMap) ...")
	featuregen = FeatureGenerator()
	featuregen.addFeatures()
	return 0

