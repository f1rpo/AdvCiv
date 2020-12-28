# advc.savem: savemap script by xyx with some changes for BUG integration
# https://forums.civfanatics.com/threads/premade-map-to-mapscript-converter.654299/

# creates a mapscript from the currently shown map.
# this is based on the savemap script by tywiggins
# https://apolyton.net/forum/civilization-iv/civilization-iv-creation/165900-python-save-map
from CvPythonExtensions import *
import os
# <advc.savem>
import BugPath
import BugUtil
import BugCore
import CvEventInterface
import BugEventManager
# </advc.savem>

gc = CyGlobalContext()
# <advc.savem>
pathnames = []

def savemap(argsList=None):
	# <advc.savem>
	if gc.getGame().isNetworkMultiPlayer() and not CvEventInterface.getEventManager().isCheatsEnabled():
		return
	if not BugCore.game.MainInterface.isSavemapEnabled():
		return
	# </advc.savem>
	# First choice: Mod folder
	pathnames.append(str(BugPath.getModDir()) + "\\PrivateMaps\\")
	# Under MyGames. Can't load it from there into AdvCiv w/o toggling NoCustomAssets.
	myGamesPathFromBUG = str(BugPath.getRootDir()) + "\\PublicMaps\\"
	pathnames.append(myGamesPathFromBUG)
	# Try tywiggins's code as a fallback
	pathFromOSEnv = str(os.getenv("HOMEDRIVE") + os.getenv("HOMEPATH") + "\\Documents\\My Games\\Beyond The Sword\\PublicMaps\\")
	if pathFromOSEnv != myGamesPathFromBUG:
		pathnames.append(pathFromOSEnv)
	# As for the file name: Will generate that based on the map settings
	# </advc.savem>

	map = CyMap() # advc.savem
	game = CyGame() # advc.savem
	width = map.getGridWidth()	 # returns num plots, not gridsize
	height = map.getGridHeight()
	# add extra plots if plots-x or plots-y are no multiples of 4 to obtain valid gridsizes (= numplots/4)
	extraWidth = int(width%4)
	extraHeight = int(height%4)
	if(extraWidth != 0):
		extraWidth = 4 - extraWidth
	if(extraHeight != 0):
		extraHeight = 4 - extraHeight
	numPlots = (width + extraWidth) * (height + extraHeight)
	# <advc.savem> Make sure not to create a huge file if the map somehow returns crazy dimensions
	if numPlots < 0 or numPlots > 50000:
		BugUtil.error("savemap: Invalid number of plots: '%d'", numPlots)
		msg = "Savemap failed. Invalid number of tiles: " + str(numPlots)
		_showOnScreenMessage(msg)
		return
	# </advc.savem>
	wrapX = map.isWrapX()
	wrapY = map.isWrapY()
	topLat = map.getTopLatitude()
	bottomLat = map.getBottomLatitude()
	numPlayers = game.countCivPlayersEverAlive()

	# determine starting locations
	civs = []
	civsDesc = []
	startingPlots = []
	startingPlotsXY = []

	for i in range(numPlayers):
		player = gc.getPlayer(i)
		pIndex = 0
		civInfo = gc.getCivilizationInfo(player.getCivilizationType()) # advc.savem
		# determine starting location from first settler found, since player.getStartingPlot() is somewhat unreliable; only do this at gamestart
		if(game.getElapsedGameTurns() == 0 and game.getStartEra() == 0): # advc.savem: was getGameTurn==0
			if(player.getNumUnits() > 0):
				for j in range(player.getNumUnits()):
					unit = player.getUnit(j)
					if(unit.getUnitClassType() == gc.getInfoTypeForString("UNITCLASS_SETTLER")):
						pPlot = unit.plot()
						if(pPlot.isWater() == 0):
							pIndex = map.plotNum(pPlot.getX(), pPlot.getY()) + (extraWidth * pPlot.getY())
							# advc.savem: was print
							BugUtil.debug("savemap: Found settler of civ '%d' ('%s') at plot '%d' ('%d', '%d')", int(player.getCivilizationType()), civInfo.getShortDescription(0), pIndex, map.plotX(pIndex), map.plotY(pIndex))
							startingPlots.append(pIndex)
							civs.append(int(player.getCivilizationType()))
							civsDesc.append(civInfo.getType())
							break

		# fallback; determine starting location from (a) stored location, (b) capital, or (c) first city
		if(pIndex == 0):
			pPlot = player.getStartingPlot()
			if(map.isPlot(pPlot.getX(), pPlot.getY())):
				pIndex = map.plotNum(pPlot.getX(), pPlot.getY()) + (extraWidth * pPlot.getY())
				startingPlots.append(pIndex)
				civs.append(int(player.getCivilizationType()))
				civsDesc.append(civInfo.getType())
			elif(player.getCapitalCity().plot() != None):
				pPlot = player.getCapitalCity().plot()
				pIndex = map.plotNum(pPlot.getX(), pPlot.getY()) + (extraWidth * pPlot.getY())
				startingPlots.append(pIndex)
				civs.append(int(player.getCivilizationType()))
				civsDesc.append(civInfo.getType())
			elif((player.getCapitalCity().plot() == None) and (player.getNumCities() > 0)):
				pPlot = player.getCity(0).plot()
				pIndex = map.plotNum(pPlot.getX(), pPlot.getY()) + (extraWidth * pPlot.getY())
				startingPlots.append(pIndex)
				civs.append(int(player.getCivilizationType()))
				civsDesc.append(civInfo.getType())
			# advc.savem: was print
			BugUtil.debug("savemap: No settler found (or not saving on the initial turn) for civ '%d' ('%s'); setting starting location at plot '%d' ('%d', '%d')", int(player.getCivilizationType()), civInfo.getShortDescription(0), pIndex, map.plotX(pIndex), map.plotY(pIndex))

	# also store starting coords, not actually required, but makes debugging easier
	for i in range(len(startingPlots)):
		pIndex = startingPlots[i]
		pPlotXY = [map.plotX(pIndex), map.plotY(pIndex)]
		startingPlotsXY.append(pPlotXY)

	# determine terrain etc
	plotTypes = {}          # default 3 (PLOT_OCEAN in BTS)
	terrainTypes = {}       # default 6 (TERRAIN_OCEAN in BTS)
	bonuses = {}            # default -1
	features = {}           # default -1
	featureVarieties = {}   # default 0
	improvements = {}       # default -1
	riverwe = {}            # default -1
	riverns = {}            # default -1

	for i in range(0,height):
		for j in range(0,width):
			pPlot = map.plot(j,i)
			pIndex = map.plotNum(pPlot.getX(), pPlot.getY()) + (extraWidth * pPlot.getY())

			if(int(pPlot.getPlotType()) != 3):
				plotTypes[pIndex] = int(pPlot.getPlotType())
			if(pPlot.getTerrainType() != 6):
				terrainTypes[pIndex] = pPlot.getTerrainType()
			if(pPlot.getBonusType(-1) != -1):
				bonuses[pIndex] = pPlot.getBonusType(-1)
			if(pPlot.getFeatureType() != -1):
				features[pIndex] = pPlot.getFeatureType()
				featureVarieties[pIndex] = pPlot.getFeatureVariety()
			if(pPlot.getImprovementType() != -1):
				improvements[pIndex] = pPlot.getImprovementType()
			if(pPlot.isNOfRiver()):
				riverwe[pIndex] = int(pPlot.getRiverWEDirection())
			if(pPlot.isWOfRiver()):
				riverns[pIndex] = int(pPlot.getRiverNSDirection())


	# write mapscript
	# <advc.savem>
	mapScriptName = str(map.getMapScriptName())
	dimensionsInName = True
	wbEnding = ".CivBeyondSwordWBSave"
	if wbEnding in mapScriptName:
		mapScriptName = mapScriptName.replace(wbEnding, "")
		# Scenario dimensions aren't so interesting
		dimensionsInName = False
	# If the script is used recursively (who would do that?) the map script name can get long
	if len(mapScriptName) > 100:
		mapScriptName = mapScriptName[:100]
	filename = mapScriptName + "_"
	if dimensionsInName:
		filename += str(width) + "x" + str(height) + "_"
	filename += str(numPlayers) + "civs"
	# A bit complicated b/c I want to make sure not to overwrite anything
	goodPath = None
	goodName = None
	customAssets = False
	for pathname in pathnames:
		idSuffix = ""
		attempts = 10
		for id in range(attempts):
			if id > 0:
				idSuffix = "_" + str(id)
			goodName = filename + idSuffix
			pathStr = pathname + goodName + ".py"
			try:
				f = open(pathStr)
			except:
				# File doesn't exist yet: good (or pathname is inaccessible; we'll see about that)
				break
			BugUtil.debug("savemap: File '%s' already exists", pathStr)
			if id == attempts - 1:
				BugUtil.error("savemap: Files '%s' already exist", pathname + filename + "[0.." + str(attempts-1) + "].py")
				msg = "Failed to save map to " + pathname + " -- file " + goodName + ".py already exists."
				_showOnScreenMessage(msg)
				return
			f.close()
		try:
			f = open(pathStr, 'w')
		except:
			BugUtil.debug("savemap: Cannot open path '%s' for writing", pathStr)
			customAssets = True
			continue
		goodPath = pathname
		msg = "Saving map to " + pathStr + "\nNote:"
		if customAssets:
			msg += "\n"
		else:
			msg += " "
		# I see no way to make CvDLLPythonIFaceBase aware of the new map script
		# (CvDLLPythonIFaceBase::moduleExists doesn't do the trick either)
		msg += "Playing the saved map will require a restart of Civ 4"
		if customAssets:
			msg += ", and mods that disable CustomAssets cannot load maps from that location"
		msg += "."
		_showOnScreenMessage(msg, customAssets)
		break
	if goodPath is None:
		path1 = pathnames[0]
		path2 = pathnames[1]
		BugUtil.error("savemap: Failed to write to '%s' and '%s'", path1, path2)
		msg = "Unable to save map to either:\n" + path1 + " or \n" + path2
		_showOnScreenMessage(msg)
		return
	# </advc.savem>

	f.write('from CvPythonExtensions import *\n')
	f.write('import CvMapGeneratorUtil\n')
	f.write('from random import random, seed, shuffle\n')
	f.write('\n')
	# advc.savem: Moved up for quick inspection
	f.write('def getDescription():\n')
	#string = '\treturn "Saved Map, based on ' + str(map.getMapScriptName()) + ' ('+str(width)+' x '+str(height)+') with '+str(numPlayers)+' civs"\n'
	# <advc.savem> Use the above for the file name instead
	string = '\treturn '
	string += "\"Originally created with the following settings by:\\n"
	# This isn't portable; based on advc.106h.
	settingsStr = map.getSettingsString()
	settingsStr = settingsStr.replace('\n', '\\n')
	string += settingsStr + "\\n"
	string += "Original players:\\n"
	for playerID in range(numPlayers):
		player = gc.getPlayer(playerID)
		civInfo = gc.getCivilizationInfo(player.getCivilizationType())
		leaderInfo = gc.getLeaderHeadInfo(player.getLeaderType())
		string += str(playerID) + " - " + leaderInfo.getDescription() + " of " + civInfo.getShortDescription(0)
		if player.isHuman():
			string += " (human)"
		string += "\\n"
	string += "Saved on turn " + str(game.getGameTurn()) + "\""
	string += '\n'
	# </advc.savem>
	f.write(string)
	f.write('\n')
	f.write('gc = CyGlobalContext()\n')
	f.write('\n')
	f.write('# seed random generator with MapRand (synchronized source for multiplayer)\n')
	f.write('seedValue = gc.getGame().getMapRand().get(65535, "Seeding mapRand - savemap.py")\n')
	f.write('seed(seedValue)\n')
	f.write('\n')
	string = 'plotTypes = ' + str(plotTypes) + '\n'
	f.write(string)
	string = 'terrainTypes = ' + str(terrainTypes) + '\n'
	f.write(string)
	string = 'bonuses = ' + str(bonuses) + '\n'
	f.write(string)
	string = 'features = ' + str(features) + '\n'
	f.write(string)
	string = 'featureVarieties = ' + str(featureVarieties) + '\n'
	f.write(string)
	string = 'riverwe = ' + str(riverwe) + '\n'
	f.write(string)
	string = 'riverns = ' + str(riverns) + '\n'
	f.write(string)
	string = 'improvements = ' + str(improvements) + '\n'
	f.write(string)
	string = 'numPlots = ' + str(numPlots) + '\n'
	f.write(string)
	f.write('\n')
	f.write('def isAdvancedMap():\n')
	f.write('\treturn 0\n')
	f.write('\n')
	f.write('def isClimateMap():\n')
	f.write('\treturn 0\n')
	f.write('\n')
	f.write('def isSeaLevelMap():\n')
	f.write('\treturn 0\n')
	f.write('\n')
	f.write('def getNumCustomMapOptions():\n')
	f.write('\treturn 3\n')
	f.write('\n')
	f.write('def getCustomMapOptionName(argsList):\n')
	f.write('\t[iOption] = argsList\n')
	f.write('\toption_names = {\n')
	# (advc.savem: Tbd.: Options "Players", "Difficulty")
	f.write('\t\t0:\t"Starting Sites",\n') # advc.savem: was "Starting Locations"
	f.write('\t\t1:\t"Bonus Resources",\n') # advc.savem: was "Bonuses"
	f.write('\t\t2:\t"Goody Huts"\n')
	f.write('\t\t}\n')
	f.write('\ttranslated_text = unicode(CyTranslator().getText(option_names[iOption], ()))\n')
	f.write('\treturn translated_text\n')
	f.write('\n')
	f.write('def getNumCustomMapOptionValues(argsList):\n')
	f.write('\t[iOption] = argsList\n')
	f.write('\toption_values = {\n')
	f.write('\t\t0:\t3,\n')
	f.write('\t\t1:\t2,\n')
	f.write('\t\t2:\t2\n')
	f.write('\t\t}\n')
	f.write('\treturn option_values[iOption]\n')
	f.write('\n')
	f.write('def getCustomMapOptionDescAt(argsList):\n')
	f.write('\t[iOption, iSelection] = argsList\n')
	f.write('\tselection_names = {\n')
	f.write('\t\t0:\t{\n')
	# (advc.savem: Tbd.: "Players" selections: "Original Players" / "Custom [Override?] Players".
	# Also: "Difficulty": "Custom Difficulty" / "Original Difficulty"
	# Could shorten the selection names further; the longer names are better for the in-game Settings tab.)
	f.write('\t\t\t0: "Original Starts",\n') # advc.savem: was "Use Fixed Starting Locations"
	# advc.savem: was "Use Fixed Starting Locations, but assign Civs at Random".
	f.write('\t\t\t1: "Shuffled Starts",\n')
	# advc.savem: Was "Ignore Fixed Locations". Warn that the normalization step will be skipped?
	f.write('\t\t\t2: "New Starts"\n')
	f.write('\t\t\t},\n')
	f.write('\t\t1:\t{\n')
	f.write('\t\t\t0: "Original Bonuses",\n') # advc.savem: was "Use Fixed Bonuses"
	f.write('\t\t\t1: "New Bonuses"\n') # advc.savem: was "Randomize Bonuses"
	f.write('\t\t\t},\n')
	f.write('\t\t2:\t{\n')
	f.write('\t\t\t0: "Original Huts",\n') # advc.savem: was "Use fixed Goody Huts"
	f.write('\t\t\t1: "New Huts"\n') # advc.savem: was "Randomize Goody Huts"
	f.write('\t\t\t}\n')
	f.write('\t\t}\n')
	f.write('\ttranslated_text = unicode(CyTranslator().getText(selection_names[iOption][iSelection], ()))\n')
	f.write('\treturn translated_text\n')
	f.write('\n')
	f.write('def getCustomMapOptionDefault(argsList):\n')
	f.write('\t[iOption] = argsList\n')
	f.write('\toption_defaults = {\n')
	f.write('\t\t0:\t0,\n')
	f.write('\t\t1:\t0,\n')
	f.write('\t\t2:\t0\n')
	f.write('\t\t}\n')
	f.write('\treturn option_defaults[iOption]\n')
	f.write('\n')
	# advc.savem: Rather confusing together with selections like "Random Bonuses"
	#f.write('def isRandomCustomMapOption(argsList):\n')
	#f.write('\t[iOption] = argsList\n')
	#f.write('\toption_random = {\n')
	#f.write('\t\t0:\tTrue,\n')
	#f.write('\t\t1:\tTrue,\n')
	#f.write('\t\t2:\tTrue\n')
	#f.write('\t\t}\n')
	#f.write('\treturn option_random[iOption]\n')
	#f.write('\n')
	f.write('def getWrapX():\n')
	string = '\treturn ' + str(wrapX) + '\n'
	f.write(string)
	f.write('\n')
	f.write('def getWrapY():\n')
	string = '\treturn ' + str(wrapY) + '\n'
	f.write(string)
	f.write('\n')
	f.write('def getTopLatitude():\n')
	string = '\treturn ' + str(topLat) + '\n'
	f.write(string)
	f.write('\n')
	f.write('def getBottomLatitude():\n')
	string = '\treturn ' + str(bottomLat) + '\n'
	f.write(string)
	f.write('\n')
	f.write('def getGridSize(argsList):\n')
	string = '\treturn (' + str((width + extraWidth)/4) + ', ' + str((height + extraHeight)/4) + ')\n'
	f.write(string)
	f.write('\n')
	f.write('def generatePlotTypes():\n')
	f.write('\tplots = []\n')
	f.write('\tfor i in range(numPlots):\n')
	f.write('\t\tif(i in plotTypes):\n')
	f.write('\t\t\tplots.append(plotTypes[i])\n')
	f.write('\t\telse:\n')
	f.write('\t\t\tplots.append(3)\n')
	f.write('\treturn plots\n')
	f.write('\n')
	f.write('def generateTerrainTypes():\n')
	f.write('\tterrain = []\n')
	f.write('\tfor i in range(numPlots):\n')
	f.write('\t\tif(i in terrainTypes):\n')
	f.write('\t\t\tterrain.append(terrainTypes[i]) \n')
	f.write('\t\telse:\n')
	f.write('\t\t\tterrain.append(6)\n')
	f.write('\treturn terrain\n')
	f.write('\n')
	f.write('def beforeInit():\n')
	f.write('\treturn None\n')
	f.write('\n')
	f.write('def beforeGeneration():\n')
	f.write('\treturn None\n')
	f.write('\n')
	f.write('def addRivers():\n')
	f.write('\t# yes, use riverwe for setNOfRiver and riverns for setWOfRiver\n')
	f.write('\tfor plotIdx in riverwe:\n')
	f.write('\t\tpPlot = CyMap().plotByIndex(plotIdx)\n')
	f.write('\t\tpPlot.setNOfRiver(1, CardinalDirectionTypes(riverwe[plotIdx]))\n')
	f.write('\tfor plotIdx in riverns:\n')
	f.write('\t\tpPlot = CyMap().plotByIndex(plotIdx)\n')
	f.write('\t\tpPlot.setWOfRiver(1, CardinalDirectionTypes(riverns[plotIdx]))\n')
	f.write('\treturn None\n')
	f.write('\n')
	f.write('def addLakes():\n')
	f.write('\treturn None\n')
	f.write('\n')
	f.write('def addFeatures():\n')
	f.write('\tfor plotIdx in features:\n')
	f.write('\t\tpPlot = CyMap().plotByIndex(plotIdx)\n')
	f.write('\t\tpPlot.setFeatureType(features[plotIdx], featureVarieties[plotIdx])\n')
	f.write('\treturn 0\n')
	f.write('\n')
	f.write('def addBonuses():\n')
	f.write('\tif CyMap().getCustomMapOption(1) == 0:\n')
	f.write('\t\tfor plotIdx in bonuses:\n')
	f.write('\t\t\tpPlot = CyMap().plotByIndex(plotIdx)\n')
	f.write('\t\t\tpPlot.setBonusType(bonuses[plotIdx])\n')
	f.write('\telse:\n')
	f.write('\t\tCyPythonMgr().allowDefaultImpl()\n')
	f.write('\treturn None\n')
	f.write('\n')
	f.write('def addGoodies():\n')
	f.write('\tif CyMap().getCustomMapOption(2) == 0:\n')
	f.write('\t\tfor plotIdx in improvements:\n')
	f.write('\t\t\tif(improvements[plotIdx] == gc.getInfoTypeForString("IMPROVEMENT_GOODY_HUT")):\n')
	f.write('\t\t\t\tpPlot = CyMap().plotByIndex(plotIdx)\n')
	f.write('\t\t\t\tpPlot.setImprovementType(gc.getInfoTypeForString("IMPROVEMENT_GOODY_HUT"))\n')
	f.write('\telse:\n')
	f.write('\t\tCyPythonMgr().allowDefaultImpl()\n')
	f.write('\treturn None\n')
	f.write('\n')
	f.write('def assignStartingPlots():\n')
	# <advc.027> Allow StartingPositionIteration to assign the plots
	f.write('\tif CyMap().getCustomMapOption(0) == 2:\n')
	f.write('\t\tCyPythonMgr().allowDefaultImpl()\n')
	f.write('\t\treturn None\n')
	# </advc.027>
	string = '\t# civs are ' + str(civsDesc) + '\n'
	f.write(string)
	string = '\tcivs = ' + str(civs) + '\n'
	f.write(string)
	string = '\tstartingPlots = ' + str(startingPlots) + '\n'
	f.write(string)
	string = '\tstartingPlotsXY = ' + str(startingPlotsXY) + '\n'
	f.write(string)
	f.write('\tif CyMap().getCustomMapOption(0) == 1:\n')
	f.write('\t\tshuffle(startingPlots)\n')
	f.write('\tusedstartingPlots = []\n')
	f.write('\tnumPlayers = CyGame().countCivPlayersEverAlive()\n')
	f.write('\tnotinlist = []\n')
	f.write('\tfor i in range(0, numPlayers):\n')
	f.write('\t\tplayer = gc.getPlayer(i)\n')
	f.write('\t\t# partly random assignment to fixed locations\n')
	f.write('\t\tif CyMap().getCustomMapOption(0) == 1:\n')
	f.write('\t\t\tif(i < len(startingPlots)):\n')
	f.write('\t\t\t\tplotindex = startingPlots[i]\n')
	f.write('\t\t\telse:\n')
	f.write('\t\t\t\tplotindex = findStartingPlot(i)\n')
	f.write('\t\t\tplayer.setStartingPlot(CyMap().plotByIndex(plotindex), 1)\n')
	f.write('\t\t# fixed locations\n')
	#f.write('\t\telif CyMap().getCustomMapOption(0) == 0:\n')
	f.write('\t\telse:\n') # advc.027: Replacing the above
	f.write('\t\t\tciv = int(player.getCivilizationType())\n')
	f.write('\t\t\tif(civs.count(civ) == 1):\n')
	f.write('\t\t\t\tpindex = civs.index(civ)\n')
	f.write('\t\t\t\tplotindex = startingPlots[pindex]\n')
	f.write('\t\t\t\tusedstartingPlots.append(plotindex)\n')
	f.write('\t\t\t\tplayer.setStartingPlot(CyMap().plotByIndex(plotindex), 1)\n')
	f.write('\t\t\telse:\n')
	f.write('\t\t\t\tnotinlist.append(i)\n')
	# advc.027: Commented out (now handled upfront)
	#f.write('\t\t# fully random (ignore fixed locations)\n')
	#f.write('\t\telse:\n')
	#f.write('\t\t\tplotindex = findStartingPlot(i)\n')
	#f.write('\t\t\tplayer.setStartingPlot(CyMap().plotByIndex(plotindex), 1)\n')
	f.write('\t\n')
	f.write('\t# handle unassigned civs\n')
	f.write('\topenstartingPlots = list(set(startingPlots) - set(usedstartingPlots))\n')
	f.write('\tshuffle(openstartingPlots) # so that unassigned civs get different position when regenerating a map\n')
	f.write('\tfor i in range(len(notinlist)):\n')
	f.write('\t\tplayer = gc.getPlayer(notinlist[i])\n')
	f.write('\t\t# try to reuse unassigned starting plots\n')
	f.write('\t\tif len(openstartingPlots) > 0:\n')
	f.write('\t\t\tplotindex = openstartingPlots[0]\n')
	f.write('\t\t\topenstartingPlots.remove(plotindex)\n')
	f.write('\t\telse:\n')
	f.write('\t\t\tplotindex = findStartingPlot(notinlist[i])\n')
	f.write('\t\tplayer.setStartingPlot(CyMap().plotByIndex(plotindex), 1)\n')
	f.write('\treturn None\n')
	f.write('\n')
	f.write('def findStartingPlot(argsList):\n')
	f.write('\tplayerID = argsList\n')
	# <advc.027>
	f.write('\tif CyMap().getCustomMapOption(0) == 2:\n')
	f.write('\t\tCyPythonMgr().allowDefaultImpl()\n')
	f.write('\t\treturn None\n')
	# </advc.027>
	f.write('\treturn CvMapGeneratorUtil.findStartingPlot(playerID)\n')
	f.write('\n')
	f.write('def normalizeStartingPlotLocations():\n')
	f.write('\treturn None\n')
	f.write('\n')
	f.write('def normalizeAddRiver():\n')
	f.write('\treturn None\n')
	f.write('\n')
	f.write('def normalizeRemovePeaks():\n')
	f.write('\treturn None\n')
	f.write('\n')
	f.write('def normalizeAddLakes():\n')
	f.write('\treturn None\n')
	f.write('\n')
	f.write('def normalizeRemoveBadFeatures():\n')
	f.write('\treturn None\n')
	f.write('\n')
	f.write('def normalizeRemoveBadTerrain():\n')
	f.write('\treturn None\n')
	f.write('\n')
	f.write('def normalizeAddFoodBonuses():\n')
	f.write('\treturn None\n')
	f.write('\n')
	f.write('def normalizeAddGoodTerrain():\n')
	f.write('\treturn None\n')
	f.write('\n')
	f.write('def normalizeAddExtras():\n')
	f.write('\treturn None\n')
	f.write('\n')
	# advc.savem: AdvCiv doesn't place free AI units in the same tile; doing it only for humans won't really help.
	#f.write('def startHumansOnSameTile():\n')
	#f.write('\treturn True\n')
	f.close()
	# <advc.savem>
	BugUtil.debug("savemap: Done saving map to '%s'", string)

def _showOnScreenMessage(msg, bExtraLong = False):
	szMsgTime = "EVENT_MESSAGE_TIME"
	if bExtraLong:
		szMsgTime += "_LONG"
	CyInterface().addMessage(CyGame().getActivePlayer(), True, gc.getDefineINT(szMsgTime), msg, None, InterfaceMessageTypes.MESSAGE_TYPE_INFO, None, ColorTypes(-1), 0, 0, False, False)
# </advc.savem>
