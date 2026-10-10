#
#	FILE:	 Mirror.py
#	AUTHOR:  Bob Thomas (Sirian)
#	PURPOSE: Regional map script - Generates half a map, then mirrors it.
#-----------------------------------------------------------------------------
#	Copyright (c) 2005 Firaxis Games, Inc. All rights reserved.
#-----------------------------------------------------------------------------
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from CvPythonExtensions import *
import CvUtil
import CvMapGeneratorUtil
import sys
from SAS_WorldSizeUtils import *
from CvMapGeneratorUtil import FractalWorld
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator
from CvMapGeneratorUtil import BonusBalancer

balancer = BonusBalancer()

def getDescription():
	return "TXT_KEY_MAP_SCRIPT_MIRROR_DESCR"

def getNumCustomMapOptions():
	return 4

def getNumHiddenCustomMapOptions():
	return 1

def getCustomMapOptionName(argsList):
	[iOption] = argsList
	option_names = {
		0:	"TXT_KEY_MAP_SCRIPT_MIRROR_TYPE",
		1:	"TXT_KEY_MAP_SCRIPT_TEAM_SETTING",
		2:	"TXT_KEY_MAP_SCRIPT_LANDMASS_TYPE",
		3:  "TXT_KEY_MAP_WORLD_WRAP"
		}
	if not option_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
	translated_text = unicode(CyTranslator().getText(option_names[iOption], ()))
	return translated_text

def getNumCustomMapOptionValues(argsList):
	[iOption] = argsList
	option_values = {
		0:	4,
		1:	3,
		2:	6,
		3:  3
		}
	return option_values[iOption]

def getCustomMapOptionDescAt(argsList):
	[iOption, iSelection] = argsList
	selection_names = {
		0:	{
			0: "TXT_KEY_MAP_SCRIPT_REFLECTION",
			1: "TXT_KEY_MAP_SCRIPT_INVERSION",
			2: "TXT_KEY_MAP_SCRIPT_COPY",
			3: "TXT_KEY_MAP_SCRIPT_OPPOSITE"
			},
		1:	{
			0: "TXT_KEY_MAP_SCRIPT_START_TOGETHER",
			1: "TXT_KEY_MAP_SCRIPT_START_SEPARATED",
			2: "TXT_KEY_MAP_SCRIPT_START_ANYWHERE"
			},
		2:	{
			0: "TXT_KEY_MAP_SCRIPT_SMALL_LAKES",
			1: "TXT_KEY_MAP_SCRIPT_WIDE_CONTINENTS",
			2: "TXT_KEY_MAP_SCRIPT_SNAKY_CONTINENTS",
			3: "TXT_KEY_MAP_SCRIPT_ARCHIPELAGO",
			4: "TXT_KEY_MAP_SCRIPT_TINY_ISLANDS",
			5: "TXT_KEY_MAP_SCRIPT_VARIED"
			},
		3:	{
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
		0:	0,
		1:	0,
		2:	0,
		3:  0
		}
	return option_defaults[iOption]

def isRandomCustomMapOption(argsList):
	[iOption] = argsList
	option_random = {
		0:	true,
		1:	false,
		2:	false,
		3:  false
		}
	return option_random[iOption]

def isAdvancedMap():
	"This map should not show up in simple mode"
	return 1

def isSeaLevelMap():
	return 0

def getWrapX():
	map = CyMap()
	return (map.getCustomMapOption(3) == 1 or map.getCustomMapOption(3) == 2)

def getWrapY():
	map = CyMap()
	return (map.getCustomMapOption(3) == 2)

def minStartingDistanceModifier():
	return -65

def beforeGeneration():
	global team_num
	team_num = []
	team_index = 0
	gc = CyGlobalContext()
	for teamCheckLoop in range(gc.getMAX_CIV_TEAMS()):
		if CyGlobalContext().getTeam(teamCheckLoop).isEverAlive():
			team_num.append(team_index)
			team_index += 1
		else:
			team_num.append(-1)
	return None

def getGridSize(argsList):
	# <!-- custom: After repairing actual two-team IDs, a two-player Arena Start Together test reached Hall of Fame immediately because the shared 8x8 compact grid supplied no valid special-path start.
	# Restore Mirror's inherited land-heavy base profile, add Arena below Duel and calibrate SAS tiers from Huge by expected player count. See KI#294.2. (GPT-5.6-Sol) -->
	if (argsList[0] == -1): # (-1,) is passed to function on loads
		return []
	[eWorldSize] = argsList
	grid_sizes = {
		WorldSizeTypes.WORLDSIZE_ARENA: (5, 3),
		WorldSizeTypes.WORLDSIZE_DUEL: (6, 4),
		WorldSizeTypes.WORLDSIZE_TINY: (8, 5),
		WorldSizeTypes.WORLDSIZE_SMALL: (10, 6),
		WorldSizeTypes.WORLDSIZE_STANDARD: (13, 8),
		WorldSizeTypes.WORLDSIZE_LARGE: (16, 10),
		WorldSizeTypes.WORLDSIZE_HUGE: (21, 13)
		}
	return sas_lookup_world_size_with_calibrated_sas(eWorldSize, grid_sizes)

class MirrorMultilayeredFractal(CvMapGeneratorUtil.MultilayeredFractal):
	# Subclass. Only the controlling function overridden in this case.
	def generatePlotsByRegion(self, terrain_type):
		# Sirian's MultilayeredFractal class, controlling function.
		# You -MUST- customize this function for each use of the class.
		#
		# The following grain matrix is specific to Pangaea.py
		sizekey = self.map.getWorldSize()
		sizevalues = {
			WorldSizeTypes.WORLDSIZE_ARENA: 2,
			WorldSizeTypes.WORLDSIZE_DUEL: 3,
			WorldSizeTypes.WORLDSIZE_TINY: 3,
			WorldSizeTypes.WORLDSIZE_SMALL: 4,
			WorldSizeTypes.WORLDSIZE_STANDARD: 4,
			WorldSizeTypes.WORLDSIZE_LARGE: 4,
			WorldSizeTypes.WORLDSIZE_HUGE: 5
			}
		grain = sas_lookup_world_size(sizekey, sizevalues)

		# The following regions are specific to Mirror.py
		westX = self.iW / 2
		eastX = self.iW - 1
		southY = 0
		northY = self.iH - 1
		iWidth = self.iW / 2
		iHeight = self.iH

		# Set the grain for the primary layer.
		if terrain_type == 1:
			cont_grain = terrain_type
		else:
			cont_grain = terrain_type + 1

		# Set the water level for the primary layer.
		water = 50 + (5 * cont_grain)

		# Generate the main land mass.
		self.generatePlotsInRegion(water, iWidth, iHeight, westX, southY, cont_grain, grain, self.iRoundFlags, self.iTerrainFlags, 6, 6, True, 7, -1, False, False)

		# Second pass (to ensure cohesion), but only for Continents choices.
		if terrain_type < 3:
			second_layerHeight = iHeight/2
			second_layerWestX = westX + 1
			second_layerEastX = eastX - 1
			second_layerWidth = second_layerEastX - second_layerWestX + 1
			second_layerNorthY = northY - int(iHeight/5)
			second_layerSouthY = southY + int(iHeight/5)

			self.generatePlotsInRegion(60, second_layerWidth, second_layerHeight, second_layerWestX, second_layerSouthY, 1, grain, self.iHorzFlags, self.iTerrainFlags, 6, 5, True, 5, -1, False, False)

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
bStrip, strip,
rift_grain, has_center_rift,
invert_heights
'''

def generatePlotTypes():
	NiTextOut("Setting Plot Types (Python Mirror) ...")
	gc = CyGlobalContext()
	map = CyMap()
	userInputLandmass = map.getCustomMapOption(2)
	dice = gc.getGame().getMapRand()
	global terrainRoll
	terrainRoll = dice.get(8, "PlotGen Chooser - Mirror PYTHON")
	# 0-3 = LvR
	# 4 = Wide Conts
	# 5 = Snaky Conts
	# 6 = 'Pelago
	# 7 = Tiny Islands

	# Teams together on an all land map.
	if userInputLandmass == 0 or (userInputLandmass == 5 and terrainRoll < 4): # LvR
		land_world = FractalWorld()
		land_world.initFractal(continent_grain = 4, rift_grain = -1, has_center_rift = False, invert_heights = True)
		return land_world.generatePlotTypes(water_percent = 8)

	# Teams separated by water, use MultilayeredFractal.
	water_world = MirrorMultilayeredFractal()
	if userInputLandmass == 5: # Varied setting gives 50% land maps (LvR) and 50% water maps (random type).
		terrain_type = terrainRoll - 3
	else:
		terrain_type = userInputLandmass
	return water_world.generatePlotsByRegion(terrain_type)

def generateTerrainTypes():
	# MIRRORIZE PLOTS
	gc = CyGlobalContext()
	map = CyMap()
	userInputPlots = map.getCustomMapOption(0)
	iW = map.getGridWidth()
	iH = map.getGridHeight()

	if userInputPlots == 0: # Reflection
		reflect_x = lambda x: iW - iX - 1
		reflect_y = lambda y: iY
	elif userInputPlots == 1: # Inversion
		reflect_x = lambda x: iW - iX - 1
		reflect_y = lambda y: iH - iY - 1
	elif userInputPlots == 2: # Copy
		reflect_x = lambda x: iX + (iW / 2)
		reflect_y = lambda y: iY
	else: # userInputPlots == 3: Opposite
		reflect_x = lambda x: iX + (iW / 2)
		reflect_y = lambda y: iH - iY - 1

	for iX in range(iW / 2):
		for iY in range(iH):
			pPlot = map.plot(iX, iY)
			rPlot = map.plot(reflect_x(iX), reflect_y(iY))
			pPlot.setPlotType(rPlot.getPlotType(), false, false)

	# Smooth any graphical glitches these changes may have produced.
	map.recalculateAreas()

	# Now generate the terrain.	
	NiTextOut("Generating Terrain (Python Mirror) ...")
	terraingen = CvMapGeneratorUtil.TerrainGenerator()
	terrainTypes = terraingen.generateTerrain()
	return terrainTypes

def addLakes():
	# MIRRORIZE TERRAIN
	gc = CyGlobalContext()
	map = CyMap()
	userInputPlots = map.getCustomMapOption(0)
	iW = map.getGridWidth()
	iH = map.getGridHeight()

	if userInputPlots == 0: # Reflection
		reflect_x = lambda x: iW - iX - 1
		reflect_y = lambda y: iY
	elif userInputPlots == 1: # Inversion
		reflect_x = lambda x: iW - iX - 1
		reflect_y = lambda y: iH - iY - 1
	elif userInputPlots == 2: # Copy
		reflect_x = lambda x: iX + (iW / 2)
		reflect_y = lambda y: iY
	else: # userInputPlots == 3: Opposite
		reflect_x = lambda x: iX + (iW / 2)
		reflect_y = lambda y: iH - iY - 1

	for iX in range(iW / 2):
		for iY in range(iH):
			pPlot = map.plot(iX, iY)
			rPlot = map.plot(reflect_x(iX), reflect_y(iY))
			pPlot.setTerrainType(rPlot.getTerrainType(), false, false)

	# MIRRORIZE RIVERS
	if userInputPlots == 0: # Reflection
		reflect_x = lambda x: iW - iX - 1
		reflect_y = lambda y: iY
		reflect_z = lambda x: iW - iX - 2
		for iX in range(iW / 2):
			for iY in range(iH):
				pPlot = map.plot(iX, iY)
				pPlot.setNOfRiver(false, CardinalDirectionTypes.CARDINALDIRECTION_WEST)
				if iX != (iW / 2) - 1:
					pPlot.setWOfRiver(false, CardinalDirectionTypes.CARDINALDIRECTION_NORTH)
				rPlot = map.plot(reflect_x(iX), reflect_y(iY))
				sPlot = map.plot(reflect_z(iX), reflect_y(iY))
				if rPlot.isNOfRiver():
					if rPlot.getRiverWEDirection() == CardinalDirectionTypes.CARDINALDIRECTION_EAST:
						rivDir = CardinalDirectionTypes.CARDINALDIRECTION_WEST
					else:
						rivDir = CardinalDirectionTypes.CARDINALDIRECTION_EAST
					pPlot.setNOfRiver(true, rivDir)
				if sPlot.isWOfRiver():
					rivDir = sPlot.getRiverNSDirection()
					pPlot.setWOfRiver(true, rivDir)

	elif userInputPlots == 1: # Inversion
		reflect_x = lambda x: iW - iX - 1
		reflect_y = lambda y: iH - iY - 1
		reflect_z = lambda x: iW - iX - 2
		reflect_w = lambda y: iH - iY
		for iX in range(iW / 2):
			for iY in range(iH):
				pPlot = map.plot(iX, iY)
				pPlot.setNOfRiver(false, CardinalDirectionTypes.CARDINALDIRECTION_WEST)
				if iX != (iW / 2) - 1:
					pPlot.setWOfRiver(false, CardinalDirectionTypes.CARDINALDIRECTION_NORTH)
				sPlot = map.plot(reflect_z(iX), reflect_y(iY))
				if iY > 0:
					rPlot = map.plot(reflect_x(iX), reflect_w(iY))
					if rPlot.isNOfRiver():
						if rPlot.getRiverWEDirection() == CardinalDirectionTypes.CARDINALDIRECTION_EAST:
							rivDir = CardinalDirectionTypes.CARDINALDIRECTION_WEST
						else:
							rivDir = CardinalDirectionTypes.CARDINALDIRECTION_EAST
						pPlot.setNOfRiver(true, rivDir)
				if sPlot.isWOfRiver():
					if sPlot.getRiverNSDirection() == CardinalDirectionTypes.CARDINALDIRECTION_NORTH:
						rivDir = CardinalDirectionTypes.CARDINALDIRECTION_SOUTH
					else:
						rivDir = CardinalDirectionTypes.CARDINALDIRECTION_NORTH
					pPlot.setWOfRiver(true, rivDir)

	elif userInputPlots == 2: # Copy
		reflect_x = lambda x: iX + (iW / 2)
		reflect_y = lambda y: iY
		for iX in range(iW / 2):
			for iY in range(iH):
				pPlot = map.plot(iX, iY)
				pPlot.setNOfRiver(false, CardinalDirectionTypes.CARDINALDIRECTION_WEST)
				pPlot.setWOfRiver(false, CardinalDirectionTypes.CARDINALDIRECTION_NORTH)
				rPlot = map.plot(reflect_x(iX), reflect_y(iY))
				sPlot = map.plot(reflect_x(iX), reflect_y(iY))
				if iY > 0:
					rPlot = map.plot(reflect_x(iX), reflect_y(iY))
					if rPlot.isNOfRiver():
						rivDir = rPlot.getRiverWEDirection()
						if iX == (iW / 2) - 1 and rivDir == CardinalDirectionTypes.CARDINALDIRECTION_EAST:
							rPlot.setNOfRiver(false, CardinalDirectionTypes.CARDINALDIRECTION_WEST)
							tPlot = map.plot(reflect_x(iX - 1), reflect_y(iY - 1))
							if tPlot.isWOfRiver():
								tPlot.setWOfRiver(false, CardinalDirectionTypes.CARDINALDIRECTION_NORTH)
								uPlot = map.plot((iX + 1), (iY - 1))
								if uPlot.isWOfRiver():
									uPlot.setWOfRiver(false, CardinalDirectionTypes.CARDINALDIRECTION_NORTH)
							rPlot.setPlotType(PlotTypes.PLOT_OCEAN, false, false)
						else:
							pPlot.setNOfRiver(true, rivDir)
				if sPlot.isWOfRiver():
					rivDir = sPlot.getRiverNSDirection()
					pPlot.setWOfRiver(true, rivDir)

	else: # userInputPlots == 3: Opposite
		reflect_x = lambda x: iX + (iW / 2)
		reflect_y = lambda y: iH - iY - 1
		reflect_w = lambda y: iH - iY
		for iX in range(iW / 2):
			for iY in range(iH):
				pPlot = map.plot(iX, iY)
				pPlot.setNOfRiver(false, CardinalDirectionTypes.CARDINALDIRECTION_WEST)
				pPlot.setWOfRiver(false, CardinalDirectionTypes.CARDINALDIRECTION_NORTH)
				sPlot = map.plot(reflect_x(iX), reflect_y(iY))
				if iY > 0:
					rPlot = map.plot(reflect_x(iX), reflect_w(iY))
					if rPlot.isNOfRiver():
						rivDir = rPlot.getRiverWEDirection()
						if iX == (iW / 2) - 1 and rivDir == CardinalDirectionTypes.CARDINALDIRECTION_EAST:
							rPlot.setNOfRiver(false, CardinalDirectionTypes.CARDINALDIRECTION_WEST)
							tPlot = map.plot(reflect_x(iX - 1), reflect_w(iY - 1))
							if tPlot.isWOfRiver():
								tPlot.setWOfRiver(false, CardinalDirectionTypes.CARDINALDIRECTION_NORTH)
							rPlot.setPlotType(PlotTypes.PLOT_OCEAN, false, false)
						else:
							pPlot.setNOfRiver(true, rivDir)
				if sPlot.isWOfRiver():
					if sPlot.getRiverNSDirection() == CardinalDirectionTypes.CARDINALDIRECTION_NORTH:
						rivDir = CardinalDirectionTypes.CARDINALDIRECTION_SOUTH
					else:
						rivDir = CardinalDirectionTypes.CARDINALDIRECTION_NORTH
					pPlot.setWOfRiver(true, rivDir)

	# Smooth any graphical glitches these changes may have produced.
	map.recalculateAreas()

	# Now add the lakes.	
	return CyPythonMgr().allowDefaultImpl()

def addFeatures():
	# MIRRORIZE LAKES
	gc = CyGlobalContext()
	map = CyMap()
	userInputPlots = map.getCustomMapOption(0)
	iW = map.getGridWidth()
	iH = map.getGridHeight()

	if userInputPlots == 0: # Reflection
		reflect_x = lambda x: iW - iX - 1
		reflect_y = lambda y: iY
	elif userInputPlots == 1: # Inversion
		reflect_x = lambda x: iW - iX - 1
		reflect_y = lambda y: iH - iY - 1
	elif userInputPlots == 2: # Copy
		reflect_x = lambda x: iX + (iW / 2)
		reflect_y = lambda y: iY
	else: # userInputPlots == 3: Opposite
		reflect_x = lambda x: iX + (iW / 2)
		reflect_y = lambda y: iH - iY - 1

	for iX in range(iW / 2):
		for iY in range(iH):
			pPlot = map.plot(iX, iY)
			rPlot = map.plot(reflect_x(iX), reflect_y(iY))
			if pPlot.getPlotType() != rPlot.getPlotType():
				pPlot.setPlotType(rPlot.getPlotType(), false, false)

	# If one of the oceanic landmass types, remove all peaks along the coasts.
	userInputLandmass = map.getCustomMapOption(2)
	global terrainRoll
	if userInputLandmass == 0 or (userInputLandmass == 5 and terrainRoll < 4): # "Lakes" terrain, leave all peaks intact.
		pass
	else: # Oceanic option.
		for plotIndex in range(iW * iH):
			pPlot = map.plotByIndex(plotIndex)
			if pPlot.isPeak() and pPlot.isCoastalLand():
				# If a peak is along the coast, change to hills and recalc.
				pPlot.setPlotType(PlotTypes.PLOT_HILLS, false, false)

	# Smooth any graphical glitches these changes may have produced.
	map.recalculateAreas()

	# Now add the features.
	NiTextOut("Adding Features (Python Mirror) ...")
	featuregen = FeatureGenerator()
	featuregen.addFeatures()
	return 0

def addGoodies():
	# MIRRORIZE FEATURES AND BONUSES
	gc = CyGlobalContext()
	map = CyMap()
	userInputPlots = map.getCustomMapOption(0)
	iW = map.getGridWidth()
	iH = map.getGridHeight()

	if userInputPlots == 0: # Reflection
		reflect_x = lambda x: iW - iX - 1
		reflect_y = lambda y: iY
	elif userInputPlots == 1: # Inversion
		reflect_x = lambda x: iW - iX - 1
		reflect_y = lambda y: iH - iY - 1
	elif userInputPlots == 2: # Copy
		reflect_x = lambda x: iX + (iW / 2)
		reflect_y = lambda y: iY
	else: # userInputPlots == 3: Opposite
		reflect_x = lambda x: iX + (iW / 2)
		reflect_y = lambda y: iH - iY - 1

	for iX in range(iW / 2):
		for iY in range(iH):
			pPlot = map.plot(iX, iY)
			rPlot = map.plot(reflect_x(iX), reflect_y(iY))
			pPlot.setFeatureType(rPlot.getFeatureType(), -1)
			pPlot.setBonusType(rPlot.getBonusType(-1))

	# Now add the goodies.
	return CyPythonMgr().allowDefaultImpl()

def afterGeneration():
	# MIRRORIZE GOODIES
	gc = CyGlobalContext()
	map = CyMap()
	userInputPlots = map.getCustomMapOption(0)
	iW = map.getGridWidth()
	iH = map.getGridHeight()

	if userInputPlots == 0: # Reflection
		reflect_x = lambda x: iW - iX - 1
		reflect_y = lambda y: iY
	elif userInputPlots == 1: # Inversion
		reflect_x = lambda x: iW - iX - 1
		reflect_y = lambda y: iH - iY - 1
	elif userInputPlots == 2: # Copy
		reflect_x = lambda x: iX + (iW / 2)
		reflect_y = lambda y: iY
	else: # userInputPlots == 3: Opposite
		reflect_x = lambda x: iX + (iW / 2)
		reflect_y = lambda y: iH - iY - 1

	for iX in range(iW / 2):
		for iY in range(iH):
			pPlot = map.plot(iX, iY)
			rPlot = map.plot(reflect_x(iX), reflect_y(iY))
			pPlot.setImprovementType(rPlot.getImprovementType())

	# All done!
	return None

def assignStartingPlots():
	gc = CyGlobalContext()
	dice = gc.getGame().getMapRand()
	global shuffle
	global shuffledTeams
	global assignedPlayers
	assignedPlayers = [0] * gc.getGame().countCivTeamsEverAlive()
	print(assignedPlayers)
	shuffle = gc.getGame().getMapRand().get(2, "Start Location Shuffle - PYTHON")
	if gc.getGame().countCivTeamsEverAlive() < 5:
		team_list = [0, 1, 2, 3]
		shuffledTeams = []
		for teamLoop in range(gc.getGame().countCivTeamsEverAlive()):
			iChooseTeam = dice.get(len(team_list), "Shuffling Regions - TBG PYTHON")
			shuffledTeams.append(team_list[iChooseTeam])
			del team_list[iChooseTeam]

	# For Lakes and Continents settings, ensure that starts are all placed on the biggest landmass on each side.
	global biggest_areas
	biggest_areas = []
	areas = CvMapGeneratorUtil.getAreas()
	area_sizes = [(area.getNumTiles(), area.getID()) for area in areas if not area.isWater()]
	area_sizes.sort() # sort by size -- biggest areas last.

	# pop the biggest two areas off the list.
	area_size, area_ID = area_sizes.pop()
	biggest_areas.append(area_ID)
	if area_sizes != []:
		area_size, area_ID = area_sizes.pop()
		biggest_areas.append(area_ID)

	# First check to see if teams chose to "Start Separated" or "Start Anywhere".
	map = CyMap()
	userInputProximity = map.getCustomMapOption(1)
	if userInputProximity == 1: # Teams set to Start Separated. Use default impl.
		CyPythonMgr().allowDefaultImpl()
		return

	# Shuffle the players.
	global playersOnTeamOne
	global playersOnTeamTwo
	iPlayers = gc.getGame().countCivPlayersEverAlive()
	playersOnTeamOne = []
	playersOnTeamTwo = []

	player_list = []
	for plrCheckLoop in range(gc.getMAX_CIV_PLAYERS()):
		if CyGlobalContext().getPlayer(plrCheckLoop).isEverAlive():
			player_list.append(plrCheckLoop)
	shuffledPlayers = []
	for playerLoopTwo in range(iPlayers):
		iChoosePlayer = dice.get(len(player_list), "Shuffling Player Order - Mirror PYTHON")
		shuffledPlayers.append(player_list[iChoosePlayer])
		del player_list[iChoosePlayer]

	if userInputProximity == 2: # Teams set to Start Anywhere!
		def isValidToStartAnywhere(playerID, x, y):
			global biggest_areas
			global terrainRoll
			userInputTerrain = CyMap().getCustomMapOption(2)
			if userInputTerrain < 3 or (userInputTerrain == 5 and terrainRoll < 6):
				pPlot = CyMap().plot(x, y)
				areaID = pPlot.getArea()
				if areaID not in biggest_areas:
					return false
			return true

		# Since the default alternates by team, must use the shuffled players list to assign starting locs.
		# This will provide a truly random order, which may or may not be "fair". But hey, starting anywhere means ANYwhere. OK?
		for playerID in shuffledPlayers:
			player = gc.getPlayer(playerID)
			startPlot = CvMapGeneratorUtil.findStartingPlot(playerID, isValidToStartAnywhere)
			sPlot = map.plotByIndex(startPlot)
			player.setStartingPlot(sPlot, true)
		# All done.
		return None

	# OK, so the teams have chosen to Start Together.
	#
	# Check for the special case of two teams with even players.
	# If found, force perfect mirrorization of start plots!
	#
	# (This is necessary because the default start plot process 
	# resolves "ties" differently on each side due to minor
	# differences in the order of operations. Odd but true!)
	#
	iTeams = gc.getGame().countCivTeamsEverAlive()
	if iTeams != 2:
		CyPythonMgr().allowDefaultImpl()
		return
	# We are dealing with two teams who are evenly matched.
	# Assign all start plots for the first team, then mirrorize the locations for the second team!
	# <!-- custom: The inherited special path assumed that the two participating TeamTypes were literal IDs 0 and 1.
	# Derive the actual two team IDs from the shuffled participating players, then verify equal membership before assigning either side. See KI#294. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	teamIDs = []
	for thisPlayerID in shuffledPlayers:
		teamID = gc.getPlayer(thisPlayerID).getTeam()
		if teamID not in teamIDs:
			teamIDs.append(teamID)
	if len(teamIDs) != 2:
		CyPythonMgr().allowDefaultImpl()
		return
	# Start by determining which players are on which teams.
	for iLoop in range(iPlayers):
		thisPlayerID = shuffledPlayers[iLoop]
		this_player = gc.getPlayer(thisPlayerID)
		teamID = this_player.getTeam()
		print("Player: ", thisPlayerID, " Team: ", teamID)
		if teamID == teamIDs[1]:
			playersOnTeamTwo.append(shuffledPlayers[iLoop])
		else:
			playersOnTeamOne.append(shuffledPlayers[iLoop])
	if len(playersOnTeamOne) != len(playersOnTeamTwo):
		CyPythonMgr().allowDefaultImpl()
		return

	# Now we pick a team to assign to the left side and assign them there.
	userInputPlots = map.getCustomMapOption(0)
	iW = map.getGridWidth()
	iH = map.getGridHeight()

	def isValidForMirror(playerID, x, y):
		global biggest_areas
		global terrainRoll
		userInputTerrain = CyMap().getCustomMapOption(2)
		if userInputTerrain < 3 or (userInputTerrain == 5 and terrainRoll < 6):
			pPlot = CyMap().plot(x, y)
			areaID = pPlot.getArea()
			if areaID not in biggest_areas:
				return false

		userInputPlots = CyMap().getCustomMapOption(0)
		iPlayers = CyGlobalContext().getGame().countCivPlayersEverAlive()
		teamID = CyGlobalContext().getPlayer(playerID).getTeam()
		iW = CyMap().getGridWidth()

		# Two Evenly-Matched Teams, Start Together
		if iPlayers > 2 and userInputPlots <= 1 and x <= iW * 0.4:
			return true
		if iPlayers > 2 and userInputPlots >= 2 and x >= iW * 0.1 and x <= iW * 0.4:
			return true
		# 1 vs 1 game, so make sure the players start farther apart!
		if iPlayers == 2 and userInputPlots <= 1 and x <= iW * 0.2:
			return true
		if iPlayers == 2 and userInputPlots >= 2 and x >= iW * 0.2 and x <= iW * 0.3:
			return true
		# if not true, then false! (Duh? Well, the program still has to be told.)
		return false

	# <!-- custom: The inherited code duplicated the complete assignment loop for each shuffled side and did not retain provisional pairs for rollback.
	# Select primary/opposite lists once so both sides use the same validated path and every provisional start can be cleared before delegating a failed assignment. See KI#294.2. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if shuffle:
		primaryPlayers = playersOnTeamTwo
		oppositePlayers = playersOnTeamOne
	else:
		primaryPlayers = playersOnTeamOne
		oppositePlayers = playersOnTeamTwo
	assignedPlayerIDs = []
	for playerIndex in range(len(primaryPlayers)):
		thisPlayer = primaryPlayers[playerIndex]
		player = gc.getPlayer(thisPlayer)
		startPlot = CvMapGeneratorUtil.findStartingPlot(thisPlayer, isValidForMirror)
		if startPlot < 0:
			# <!-- custom: The special path previously converted a failed -1 search to a NULL plot, returned Python success and could eliminate every player immediately.
			# Roll back any provisional mirrored pairs and delegate the complete assignment instead. See KI#294.2. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			nullPlot = map.plotByIndex(-1)
			for assignedPlayerID in assignedPlayerIDs:
				gc.getPlayer(assignedPlayerID).setStartingPlot(nullPlot, false)
			CyPythonMgr().allowDefaultImpl()
			return
		sPlot = map.plotByIndex(startPlot)
		iX = sPlot.getX()
		iY = sPlot.getY()
		# <!-- custom: Preserve the inherited option formulas (0 Reflection, 1 Inversion, 2 Copy, 3 Opposite), but calculate the counterpart directly from this validated primary plot instead of duplicated shuffle branches. See KI#294.2. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if userInputPlots == 0:
			mirror_x = iW - iX - 1
			mirror_y = iY
		elif userInputPlots == 1:
			mirror_x = iW - iX - 1
			mirror_y = iH - iY - 1
		elif userInputPlots == 2:
			mirror_x = iX + (iW / 2)
			mirror_y = iY
		else:
			mirror_x = iX + (iW / 2)
			mirror_y = iH - iY - 1
		# <!-- custom: Validate the computed counterpart before assignment.
		# If it is outside the map, clear every provisional pair and delegate the complete assignment instead of returning Python success with invalid starts. See KI#294.2. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if not map.isPlot(mirror_x, mirror_y):
			nullPlot = map.plotByIndex(-1)
			for assignedPlayerID in assignedPlayerIDs:
				gc.getPlayer(assignedPlayerID).setStartingPlot(nullPlot, false)
			CyPythonMgr().allowDefaultImpl()
			return
		oppositePlayerID = oppositePlayers[playerIndex]
		player.setStartingPlot(sPlot, true)
		gc.getPlayer(oppositePlayerID).setStartingPlot(map.plot(mirror_x, mirror_y), true)
		assignedPlayerIDs.append(thisPlayer)
		assignedPlayerIDs.append(oppositePlayerID)

	# All done.
	return None

def findStartingPlot(argsList):
	[playerID] = argsList
	global assignedPlayers
	global team_num
	thisTeamID = CyGlobalContext().getPlayer(playerID).getTeam()
	teamID = team_num[thisTeamID]

	assignedPlayers[teamID] += 1

	def isValid(playerID, x, y):
		global biggest_areas
		global terrainRoll
		userInputTerrain = CyMap().getCustomMapOption(2)
		if userInputTerrain < 3 or (userInputTerrain == 5 and terrainRoll < 6):
			pPlot = CyMap().plot(x, y)
			areaID = pPlot.getArea()
			if areaID not in biggest_areas:
				return false

		map = CyMap()
		numTeams = CyGlobalContext().getGame().countCivTeamsAlive()
		if numTeams > 4 or numTeams < 2: # Put em anywhere, and let the normalizer sort em out.
			return true
		userInputProximity = map.getCustomMapOption(1)
		if userInputProximity == 2: # Start anywhere!
			return true
		global shuffle
		global shuffledTeams
		global team_num
		thisTeamID = CyGlobalContext().getPlayer(playerID).getTeam()
		teamID = team_num[thisTeamID]
		iW = map.getGridWidth()
		iH = map.getGridHeight()

		# Two Teams, Start Together
		if numTeams == 2 and userInputProximity == 0: # Two teams, Start Together
			if teamID == 0 and shuffle and x >= iW * 0.6:
				return true
			if teamID == 1 and not shuffle and x >= iW * 0.6:
				return true
			if teamID == 0 and not shuffle and x <= iW * 0.4:
				return true
			if teamID == 1 and shuffle and x <= iW * 0.4:
				return true
			return false

		# Three or Four Teams
		elif (numTeams == 3 or numTeams == 4) and userInputProximity == 0: # 3 or 4 teams, Start Together
			corner = shuffledTeams[teamID]
			if corner == 0 and x <= iW * 0.4 and y <= iH * 0.4:
				return true
			if corner == 1 and x >= iW * 0.6 and y <= iH * 0.4:
				return true
			if corner == 2 and x <= iW * 0.4 and y >= iH * 0.6:
				return true
			if corner == 3 and x >= iW * 0.6 and y >= iH * 0.6:
				return true
			return false
		elif (numTeams == 3 or numTeams == 4) and userInputProximity == 1: # 3 or 4 teams, Start Separated
			corner = shuffledTeams[teamID] + assignedPlayers[teamID]
			while corner >= 4:
				corner -= 4
			if corner == 0 and x <= iW * 0.4 and y <= iH * 0.4:
				return true
			if corner == 1 and x >= iW * 0.6 and y <= iH * 0.4:
				return true
			if corner == 2 and x <= iW * 0.4 and y >= iH * 0.6:
				return true
			if corner == 3 and x >= iW * 0.6 and y >= iH * 0.6:
				return true
			return false

		# Two Teams, Start Separated
		elif numTeams == 2 and userInputProximity == 1: # Two teams, Start Separated
			if (shuffle and teamID == 0) or (not shuffle and teamID == 1):
				side = assignedPlayers[teamID]
			else:
				side = 1 + assignedPlayers[teamID]
			while side >= 2:
				side -= 2
			if teamID == 0 and side and x >= iW * 0.6:
				return true
			if teamID == 1 and not side and x >= iW * 0.6:
				return true
			if teamID == 0 and not side and x <= iW * 0.4:
				return true
			if teamID == 1 and side and x <= iW * 0.4:
				return true
			return false

		# All conditions have failed? Wow. Is that even possible? :)
		return true

	return CvMapGeneratorUtil.findStartingPlot(playerID, isValid)

def normalizeStartingPlotLocations():
	numTeams = CyGlobalContext().getGame().countCivTeamsAlive()
	userInputProximity = CyMap().getCustomMapOption(1)
	if (numTeams > 4 or numTeams < 2) and userInputProximity == 0:
		CyPythonMgr().allowDefaultImpl()
	else:
		return None

def normalizeAddRiver():
	return None

def normalizeRemovePeaks():
	return None

def normalizeAddLakes():
	return None

def normalizeRemoveBadFeatures():
	return None

def normalizeRemoveBadTerrain():
	return None

def normalizeAddFoodBonuses():
	return None

def normalizeAddGoodTerrain():
	return None
