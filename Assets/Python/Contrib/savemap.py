# creates a mapscript from the currently shown map.
# this is based on the savemap script by tywiggins https://apolyton.net/forum/civilization-iv/civilization-iv-creation/165900-python-save-map
from CvPythonExtensions import *
import os

gc = CyGlobalContext()
pathname = os.getenv("HOMEDRIVE") + os.getenv("HOMEPATH") + "\\Documents\\My Games\\Beyond The Sword\\PublicMaps\\"

def savemap(filename = 'civ4map'):

	newfilename = str(filename)
	if(type(filename) == type(25)):
		newfilename = 'civ4map' + newfilename

	width = CyMap().getGridWidth()	 # returns num plots, not gridsize
	height = CyMap().getGridHeight()
	# add extra plots if plots-x or plots-y are no multiples of 4 to obtain valid gridsizes (= numplots/4)
	extraWidth = int(width%4)
	extraHeight = int(height%4)
	if(extraWidth != 0):
		extraWidth = 4 - extraWidth
	if(extraHeight != 0):
		extraHeight = 4 - extraHeight
	numPlots = (width + extraWidth) * (height + extraHeight)

	wrapX = CyMap().isWrapX()
	wrapY = CyMap().isWrapY()
	topLat = CyMap().getTopLatitude()
	bottomLat = CyMap().getBottomLatitude()
	numPlayers = CyGame().countCivPlayersEverAlive()

	# determine starting locations
	civs = []
	civsDesc = []
	startingPlots = []
	startingPlotsXY = []

	for i in range(numPlayers):
		player = gc.getPlayer(i)
		pIndex = 0
		# determine starting location from first settler found, since player.getStartingPlot() is somewhat unreliable; only do this at gamestart
		if(CyGame().getGameTurn () == 0):
			if(player.getNumUnits() > 0):
				for j in range(player.getNumUnits()):
					unit = player.getUnit(j)
					if(unit.getUnitClassType() == gc.getInfoTypeForString("UNITCLASS_SETTLER")):
						pPlot = unit.plot()
						if(pPlot.isWater() == 0):
							pIndex = CyMap().plotNum(pPlot.getX(), pPlot.getY()) + (extraWidth * pPlot.getY())
							print "saveMap: found settler of civ %d (%s) at plot %d (%d, %d)" % (int(player.getCivilizationType()), CyGlobalContext().getCivilizationInfo(int(player.getCivilizationType())).getType(), pIndex, CyMap().plotX(pIndex), CyMap().plotY(pIndex))
							startingPlots.append(pIndex)
							civs.append(int(player.getCivilizationType()))
							civsDesc.append(CyGlobalContext().getCivilizationInfo(int(player.getCivilizationType())).getType())
							break

		# fallback; determine starting location from (a) stored location, (b) capital, or (c) first city
		if(pIndex == 0):
			pPlot = player.getStartingPlot()
			if(CyMap().isPlot(pPlot.getX(), pPlot.getY())):
				pIndex = CyMap().plotNum(pPlot.getX(), pPlot.getY()) + (extraWidth * pPlot.getY())
				startingPlots.append(pIndex)
				civs.append(int(player.getCivilizationType()))
				civsDesc.append(CyGlobalContext().getCivilizationInfo(int(player.getCivilizationType())).getType())
			elif(player.getCapitalCity().plot() != None):
				pPlot = player.getCapitalCity().plot()
				pIndex = CyMap().plotNum(pPlot.getX(), pPlot.getY()) + (extraWidth * pPlot.getY())
				startingPlots.append(pIndex)
				civs.append(int(player.getCivilizationType()))
				civsDesc.append(CyGlobalContext().getCivilizationInfo(int(player.getCivilizationType())).getType())
			elif((player.getCapitalCity().plot() == None) and (player.getNumCities() > 0)):
				pPlot = player.getCity(0).plot()
				pIndex = CyMap().plotNum(pPlot.getX(), pPlot.getY()) + (extraWidth * pPlot.getY())
				startingPlots.append(pIndex)
				civs.append(int(player.getCivilizationType()))
				civsDesc.append(CyGlobalContext().getCivilizationInfo(int(player.getCivilizationType())).getType())
			print "saveMap: no settler found or gameturn > 0 for civ %d (%s); setting starting location at plot %d (%d, %d)" % (int(player.getCivilizationType()), CyGlobalContext().getCivilizationInfo(int(player.getCivilizationType())).getType(), pIndex, CyMap().plotX(pIndex), CyMap().plotY(pIndex))

	# also store starting coords, not actually required, but makes debugging easier
	for i in range(len(startingPlots)):
		pIndex = startingPlots[i]
		pPlotXY = [CyMap().plotX(pIndex), CyMap().plotY(pIndex)]
		startingPlotsXY.append(pPlotXY)

	# determine terrain etc
	plotTypes = {}		# default 3 (PLOT_OCEAN in BTS)
	terrainTypes = {}	 # default 6 (TERRAIN_OCEAN in BTS)
	bonuses = {}		  # default -1
	features = {}		 # default -1
	featureVarieties = {} # default 0
	improvements = {}	 # default -1
	riverwe = {}		  # default -1
	riverns = {}		  # default -1

	for i in range(0,height):
		for j in range(0,width):
			pPlot = CyMap().plot(j,i)
			pIndex = CyMap().plotNum(pPlot.getX(), pPlot.getY()) + (extraWidth * pPlot.getY())

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
	string = pathname + newfilename + '.py'
	f=open(string, 'w')
	f.write('from CvPythonExtensions import *\n')
	f.write('import CvMapGeneratorUtil\n')
	f.write('from random import random, seed, shuffle\n')
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
	f.write('def getDescription():\n')
	string = '\treturn "Saved Map, based on ' + str(CyMap().getMapScriptName()) + ' ('+str(width)+' x '+str(height)+') with '+str(numPlayers)+' civs"\n'
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
	f.write('\t\t0:\t"Starting Locations",\n')
	f.write('\t\t1:\t"Bonuses",\n')
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
	f.write('\t\t\t0: "Use Fixed Starting Locations",\n')
	f.write('\t\t\t1: "Use Fixed Starting Locations, but assign Civs at Random",\n')
	f.write('\t\t\t2: "Ignore Fixed Locations"\n')
	f.write('\t\t\t},\n')
	f.write('\t\t1:\t{\n')
	f.write('\t\t\t0: "Use fixed Bonuses",\n')
	f.write('\t\t\t1: "Randomize Bonuses"\n')
	f.write('\t\t\t},\n')
	f.write('\t\t2:\t{\n')
	f.write('\t\t\t0: "Use fixed Goody Huts",\n')
	f.write('\t\t\t1: "Randomize Goody Huts"\n')
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
	f.write('def isRandomCustomMapOption(argsList):\n')
	f.write('\t[iOption] = argsList\n')
	f.write('\toption_random = {\n')
	f.write('\t\t0:\tTrue,\n')
	f.write('\t\t1:\tTrue,\n')
	f.write('\t\t2:\tTrue\n')
	f.write('\t\t}\n')
	f.write('\treturn option_random[iOption]\n')
	f.write('\n')
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
	f.write('\t\tplayer = CyGlobalContext().getPlayer(i)\n')
	f.write('\t\t# partly random assignment to fixed locations\n')
	f.write('\t\tif CyMap().getCustomMapOption(0) == 1:\n')
	f.write('\t\t\tif(i < len(startingPlots)):\n')
	f.write('\t\t\t\tplotindex = startingPlots[i]\n')
	f.write('\t\t\telse:\n')
	f.write('\t\t\t\tplotindex = findStartingPlot(i)\n')
	f.write('\t\t\tplayer.setStartingPlot(CyMap().plotByIndex(plotindex), 1)\n')
	f.write('\t\t# fixed locations\n')
	f.write('\t\telif CyMap().getCustomMapOption(0) == 0:\n')
	f.write('\t\t\tciv = int(player.getCivilizationType())\n')
	f.write('\t\t\tif(civs.count(civ) == 1):\n')
	f.write('\t\t\t\tpindex = civs.index(civ)\n')
	f.write('\t\t\t\tplotindex = startingPlots[pindex]\n')
	f.write('\t\t\t\tusedstartingPlots.append(plotindex)\n')
	f.write('\t\t\t\tplayer.setStartingPlot(CyMap().plotByIndex(plotindex), 1)\n')
	f.write('\t\t\telse:\n')
	f.write('\t\t\t\tnotinlist.append(i)\n')
	f.write('\t\t# fully random (ignore fixed locations)\n')
	f.write('\t\telse:\n')
	f.write('\t\t\tplotindex = findStartingPlot(i)\n')
	f.write('\t\t\tplayer.setStartingPlot(CyMap().plotByIndex(plotindex), 1)\n')
	f.write('\t\n')
	f.write('\t# handle unassigned civs\n')
	f.write('\topenstartingPlots = list(set(startingPlots) - set(usedstartingPlots))\n')
	f.write('\tshuffle(openstartingPlots) # so that unassigned civs get different position when regenerating a map\n')
	f.write('\tfor i in range(len(notinlist)):\n')
	f.write('\t\tplayer = CyGlobalContext().getPlayer(notinlist[i])\n')
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
	f.write('def startHumansOnSameTile():\n')
	f.write('\treturn True\n')
	f.write('\n')
	f.close()
	return None
