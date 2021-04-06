# Sid Meier's Civilization 4
# Copyright Firaxis Games 2005
#
# CvRandomEventInterface.py
#
# These functions are App Entry Points from C++
# WARNING: These function names should not be changed
# WARNING: These functions can not be placed into a class
#
# No other modules should import this
#
import CvUtil
from CvPythonExtensions import *
from EventSigns import placeLandmark # K-Mod
from operator import itemgetter # K-Mod (used to avoid OOS when sorting)

gc = CyGlobalContext()
localText = CyTranslator()


# advc.137: All event preconditions that use DefaultPlayers in BtS are going to use this function instead. Using default player counts was a bad idea to begin with, and now that I've increased the default player count on Deity, it's totally out of whack.
def worldSizeTarget():
	# 5 adjusted by building class prereq modifier, rounded to nearest.
	return ((5 * (100 + gc.getWorldInfo(gc.getMap().getWorldSize()).getBuildingClassPrereqModifier())) + 50) // 100


######## BLESSED SEA ###########

def getHelpBlessedSea1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	map = gc.getMap()
	# iOurMinLandmass = (3 * gc.getWorldInfo(map.getWorldSize()).getDefaultPlayers()) / 2
	iOurMinLandmass = gc.getWorldInfo(map.getWorldSize()).getTargetNumCities() # K-Mod
	
	szHelp = localText.getText("TXT_KEY_EVENT_BLESSED_SEA_HELP", (iOurMinLandmass, ))	

	return szHelp

def canTriggerBlessedSea(argsList):
	kTriggeredData = argsList[0]
	map = gc.getMap()
		
	if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_ONE_CITY_CHALLENGE) and gc.getPlayer(kTriggeredData.ePlayer).isHuman():
		return false

	# iMapMinLandmass = 2 * gc.getWorldInfo(map.getWorldSize()).getDefaultPlayers()
	# iOurMaxLandmass = gc.getWorldInfo(map.getWorldSize()).getDefaultPlayers() / 2
	# K-Mod
	iMapMinLandmass = 4 * gc.getWorldInfo(map.getWorldSize()).getTargetNumCities() / 3
	iOurMaxLandmass = 2 * gc.getWorldInfo(map.getWorldSize()).getTargetNumCities() / 3
	# K-Mod end
	
	if (map.getNumLandAreas() < iMapMinLandmass):
		return false

	iOurLandmasses = 0
	for i in range(map.getIndexAfterLastArea()):
		area = map.getArea(i)
		if not area.isNone() and not area.isWater() and area.getCitiesPerPlayer(kTriggeredData.ePlayer) > 0:
			iOurLandmasses += 1
			
	if (iOurLandmasses > iOurMaxLandmass):
		return false

	player = gc.getPlayer(kTriggeredData.ePlayer)
	if player.getUnitClassCount(CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_GALLEY')) == 0:
		if player.getUnitClassCount(CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_CARAVEL')) == 0:
			if player.getUnitClassCount(CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_GALLEON')) == 0:
				return false
			
	return true

def canTriggerBlessedSea2(argsList):

	kTriggeredData = argsList[0]
	map = gc.getMap()
	# iOurMinLandmass = (3 * gc.getWorldInfo(map.getWorldSize()).getDefaultPlayers()) / 2
	iOurMinLandmass = gc.getWorldInfo(map.getWorldSize()).getTargetNumCities() # K-Mod

	iOurLandmasses = 0
	for i in range(map.getIndexAfterLastArea()):
		area = map.getArea(i)
		if not area.isNone() and not area.isWater() and area.getCitiesPerPlayer(kTriggeredData.ePlayer) > 0:
			iOurLandmasses += 1
			
	if (iOurLandmasses < iOurMinLandmass):
		return false
	
	return true

def applyBlessedSea2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
		
	iBuilding = -1
	
	if (-1 != kTriggeredData.eReligion):
		for i in range(gc.getNumBuildingInfos()):
			if (gc.getBuildingInfo(i).getSpecialBuildingType() == CvUtil.findInfoTypeNum(gc.getSpecialBuildingInfo,gc.getNumSpecialBuildingInfos(),'SPECIALBUILDING_TEMPLE')):
				if (gc.getBuildingInfo(i).getReligionType() == kTriggeredData.eReligion):
					iBuilding = i
					break
		
		
	if (iBuilding == -1):
		return
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	(loopCity, iter) = player.firstCity(false)

	while(loopCity):

		if (loopCity.getPopulation() >= 5):
			if (loopCity.canConstruct(iBuilding, false, false, true)):
				loopCity.setNumRealBuilding(iBuilding, 1)
				
		(loopCity, iter) = player.nextCity(iter, false)
		

def canApplyBlessedSea2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
		
	iBuilding = -1
	
	if (-1 != kTriggeredData.eReligion):
		for i in range(gc.getNumBuildingInfos()):
			if (gc.getBuildingInfo(i).getSpecialBuildingType() == CvUtil.findInfoTypeNum(gc.getSpecialBuildingInfo,gc.getNumSpecialBuildingInfos(),'SPECIALBUILDING_TEMPLE')):
				if (gc.getBuildingInfo(i).getReligionType() == kTriggeredData.eReligion):
					iBuilding = i
					break
		
		
	if (iBuilding == -1):
		return false
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	(loopCity, iter) = player.firstCity(false)
	bFound = false

	while(loopCity):

		if (loopCity.getPopulation() >= 5):
			if (loopCity.canConstruct(iBuilding, false, false, true)):
				bFound = true
				break
				
		(loopCity, iter) = player.nextCity(iter, false)

	return bFound


######## HOLY MOUNTAIN ###########

def getHelpHolyMountain1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	map = gc.getMap()
	iMinPoints = 2 * worldSizeTarget()
	
	iBuilding = -1
	iReligion = gc.getPlayer(kTriggeredData.ePlayer).getStateReligion()
	
	if (-1 != iReligion):
		for i in range(gc.getNumBuildingInfos()):
			if (gc.getBuildingInfo(i).getSpecialBuildingType() == CvUtil.findInfoTypeNum(gc.getSpecialBuildingInfo, gc.getNumSpecialBuildingInfos(), 'SPECIALBUILDING_CATHEDRAL')):
				if (gc.getBuildingInfo(i).getReligionType() == iReligion):
					iBuilding = i
					break


		szHelp = localText.getText("TXT_KEY_EVENT_HOLY_MOUNTAIN_HELP", ( gc.getBuildingInfo(iBuilding).getTextKey(), gc.getBuildingInfo(iBuilding).getTextKey(), iMinPoints))

	return szHelp

def canTriggerHolyMountain(argsList):
	kTriggeredData = argsList[0]
	map = gc.getMap()
	
	if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_ONE_CITY_CHALLENGE) and gc.getPlayer(kTriggeredData.ePlayer).isHuman():
		return false

	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	if (plot.getOwner() == -1):
		return true
	
	return false

def expireHolyMountain1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	if (plot == None):
		return true
	
	if (plot.getOwner() != kTriggeredData.ePlayer and plot.getOwner() != -1):
		return true
		
	return false

def canTriggerHolyMountainDone(argsList):

	kTriggeredData = argsList[0]
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	kOrigTriggeredData = player.getEventOccured(trigger.getPrereqEvent(0))
	
	if (kOrigTriggeredData == None):
		return false
		
	plot = gc.getMap().plot(kOrigTriggeredData.iPlotX, kOrigTriggeredData.iPlotY)
	if (plot == None):
		return false
	
	if (plot.getOwner() != kTriggeredData.ePlayer):
		return false
	
	return true

def canTriggerHolyMountainRevealed(argsList):

	kTriggeredData = argsList[0]
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	kOrigTriggeredData = player.getEventOccured(trigger.getPrereqEvent(0))
	
	if (kOrigTriggeredData == None):
		return false

	iNumPoints = 0		
	
	for i in range(gc.getNumBuildingInfos()):
		if (gc.getBuildingInfo(i).getReligionType() == kOrigTriggeredData.eReligion):
			if (gc.getBuildingInfo(i).getSpecialBuildingType() == CvUtil.findInfoTypeNum(gc.getSpecialBuildingInfo,gc.getNumSpecialBuildingInfos(),'SPECIALBUILDING_CATHEDRAL')):
				iNumPoints += 4 * player.countNumBuildings(i)
			elif (gc.getBuildingInfo(i).getSpecialBuildingType() == CvUtil.findInfoTypeNum(gc.getSpecialBuildingInfo,gc.getNumSpecialBuildingInfos(),'SPECIALBUILDING_TEMPLE')):
				iNumPoints += player.countNumBuildings(i)
			elif (gc.getBuildingInfo(i).getSpecialBuildingType() == CvUtil.findInfoTypeNum(gc.getSpecialBuildingInfo,gc.getNumSpecialBuildingInfos(),'SPECIALBUILDING_MONASTERY')):
				iNumPoints += player.countNumBuildings(i)
				
	if iNumPoints < 2 * worldSizeTarget():
		return false

	plot = gc.getMap().plot(kOrigTriggeredData.iPlotX, kOrigTriggeredData.iPlotY)
	if (plot == None):
		return false
		
	plot.setRevealed(player.getTeam(), true, true, -1)

	kActualTriggeredDataObject = player.getEventTriggered(kTriggeredData.iId)
	kActualTriggeredDataObject.iPlotX = kOrigTriggeredData.iPlotX
	kActualTriggeredDataObject.iPlotY = kOrigTriggeredData.iPlotY
	
	return true

def doHolyMountainRevealed(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	if (kTriggeredData.ePlayer == gc.getGame().getActivePlayer()):
		CyCamera().JustLookAtPlot( CyMap().plot( kTriggeredData.iPlotX, kTriggeredData.iPlotY ) )

	return 1

######## MARATHON ###########

def canTriggerMarathon(argsList):	
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	otherPlayer = gc.getPlayer(kTriggeredData.eOtherPlayer)
	team = gc.getTeam(player.getTeam())
	
	if (team.AI_getAtWarCounter(otherPlayer.getTeam()) == 1):
		(loopUnit, iter) = otherPlayer.firstUnit(false)
		while( loopUnit ):
			plot = loopUnit.plot()
			if (not plot.isNone()):
				if (plot.getOwner() == kTriggeredData.ePlayer):
					return true
			(loopUnit, iter) = otherPlayer.nextUnit(iter, false)

	return false

######## WEDDING FEUD ###########

def doWeddingFeud2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	(loopCity, iter) = player.firstCity(false)

	while(loopCity):
		if loopCity.isHasReligion(kTriggeredData.eReligion):
			loopCity.changeHappinessTimer(30)
		(loopCity, iter) = player.nextCity(iter, false)
		
	return 1

def getHelpWeddingFeud2(argsList):
	iEvent = argsList[0]
	event = gc.getEventInfo(iEvent)
	kTriggeredData = argsList[1]
	religion = gc.getReligionInfo(kTriggeredData.eReligion)

	szHelp = localText.getText("TXT_KEY_EVENT_WEDDING_FEUD_2_HELP", (gc.getDefineINT("TEMP_HAPPY"), 30, religion.getChar()))

	return szHelp

def canDoWeddingFeud3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	if player.getGold() - 10 * player.getNumCities() < 0:
		return false
				
	return true


def doWeddingFeud3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	destPlayer = gc.getPlayer(kTriggeredData.eOtherPlayer)
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	for iLoopPlayer in range(gc.getMAX_CIV_PLAYERS()):
		loopPlayer = gc.getPlayer(iLoopPlayer)
		if loopPlayer.isAlive() and loopPlayer.getStateReligion() == player.getStateReligion():
			loopPlayer.AI_changeAttitudeExtra(kTriggeredData.ePlayer, 1)
			player.AI_changeAttitudeExtra(iLoopPlayer, 1)

	if gc.getTeam(destPlayer.getTeam()).canDeclareWar(player.getTeam()):			
		if destPlayer.isHuman():
			# this works only because it's a single-player only event
			popupInfo = CyPopupInfo()
			popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON)
			popupInfo.setText(localText.getText("TXT_KEY_EVENT_WEDDING_FEUD_OTHER_3", (gc.getReligionInfo(kTriggeredData.eReligion).getAdjectiveKey(), player.getCivilizationShortDescriptionKey())))
			popupInfo.setData1(kTriggeredData.eOtherPlayer)
			popupInfo.setData2(kTriggeredData.ePlayer)
			popupInfo.setPythonModule("CvRandomEventInterface")
			popupInfo.setOnClickedPythonCallback("weddingFeud3Callback")
			popupInfo.addPythonButton(localText.getText("TXT_KEY_POPUP_YES", ()), "")
			popupInfo.addPythonButton(localText.getText("TXT_KEY_POPUP_NO", ()), "")
			popupInfo.addPopup(kTriggeredData.eOtherPlayer)
		else: # advc.106g: Let the DLL know that this is through an event
			gc.getTeam(destPlayer.getTeam()).declareWarEvent(player.getTeam(), false, WarPlanTypes.WARPLAN_LIMITED)
			
	return 1


def weddingFeud3Callback(argsList):
	iButton = argsList[0]
	iData1 = argsList[1]
	iData2 = argsList[2]
	iData3 = argsList[3]
	szText = argsList[4]
	bOption1 = argsList[5]
	bOption2 = argsList[6]
	
	if iButton == 0:
		destPlayer = gc.getPlayer(iData1)
		player = gc.getPlayer(iData2)
		# advc.106g: Let the DLL know that this is through an event
		gc.getTeam(destPlayer.getTeam()).declareWarEvent(player.getTeam(), false, WarPlanTypes.WARPLAN_LIMITED)
	
	return 0

def getHelpWeddingFeud3(argsList):
	iEvent = argsList[0]
	event = gc.getEventInfo(iEvent)
	kTriggeredData = argsList[1]
	religion = gc.getReligionInfo(kTriggeredData.eReligion)

	szHelp = localText.getText("TXT_KEY_EVENT_WEDDING_FEUD_3_HELP", (1, religion.getChar()))

	return szHelp

######## SPICY ###########

def canTriggerSpicy(argsList):

	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iSpice = CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_SPICES')
	iHappyBonuses = 0
	bSpices = false
	for i in range(gc.getNumBonusInfos()):
		bonus = gc.getBonusInfo(i)
		iNum = player.getNumAvailableBonuses(i)
		if iNum > 0 :
			if bonus.getHappiness() > 0:
				iHappyBonuses += 1
				if iHappyBonuses > 4:
					return false
			if i == iSpice:
				return false

	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	if not plot.canHaveBonus(iSpice, false):
		return false
	
	return true

def doSpicy2(argsList):
#	need this because plantations are notmally not allowed unless there are already spices
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	
	if not plot.isNone():
		plot.setImprovementType(CvUtil.findInfoTypeNum(gc.getImprovementInfo,gc.getNumImprovementInfos(),'IMPROVEMENT_PLANTATION'))
	
	return 1

def getHelpSpicy2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	iPlantation = CvUtil.findInfoTypeNum(gc.getImprovementInfo,gc.getNumImprovementInfos(),'IMPROVEMENT_PLANTATION')
	szHelp = localText.getText("TXT_KEY_EVENT_IMPROVEMENT_GROWTH", ( gc.getImprovementInfo(iPlantation).getTextKey(), ))

	return szHelp

######## BABY BOOM ###########

def canTriggerBabyBoom(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	team = gc.getTeam(player.getTeam())

	if team.getAtWarCount(true) > 0:
		return false

	for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
		if iLoopTeam != player.getTeam():
			if team.AI_getAtPeaceCounter(iLoopTeam) == 1:
				return true

	return false

######## BARD TALE ###########

def applyBardTale3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	player.changeGold(-10 * player.getNumCities())
	
def canApplyBardTale3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	if player.getGold() - 10 * player.getNumCities() < 0:
		return false
		
	return true
	

def getHelpBardTale3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	szHelp = localText.getText("TXT_KEY_EVENT_GOLD_LOST", (10 * player.getNumCities(), ))	

	return szHelp

######## LOOTERS ###########

def getHelpLooters3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	otherPlayer = gc.getPlayer(kTriggeredData.eOtherPlayer)
	city = otherPlayer.getCity(kTriggeredData.iOtherPlayerCityId)

	szHelp = localText.getText("TXT_KEY_EVENT_LOOTERS_3_HELP", (1, 2, city.getNameKey()))

	return szHelp

def canApplyLooters3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	otherPlayer = gc.getPlayer(kTriggeredData.eOtherPlayer)
	city = otherPlayer.getCity(kTriggeredData.iOtherPlayerCityId)

	iNumBuildings = 0	
	for iBuilding in range(gc.getNumBuildingInfos()):
		if (city.getNumRealBuilding(iBuilding) > 0 and gc.getBuildingInfo(iBuilding).getProductionCost() <= 100 and gc.getBuildingInfo(iBuilding).getProductionCost() > 0  and not isLimitedWonderClass(gc.getBuildingInfo(iBuilding).getBuildingClassType())):
			iNumBuildings += 1
		
	return (iNumBuildings > 0)
	

def applyLooters3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	otherPlayer = gc.getPlayer(kTriggeredData.eOtherPlayer)
	city = otherPlayer.getCity(kTriggeredData.iOtherPlayerCityId)
	
	iNumBuildings = gc.getGame().getSorenRandNum(2, "Looters event number of buildings destroyed")
	iNumBuildingsDestroyed = 0

	listBuildings = []	
	for iBuilding in range(gc.getNumBuildingInfos()):
		if (city.getNumRealBuilding(iBuilding) > 0 and gc.getBuildingInfo(iBuilding).getProductionCost() <= 100 and gc.getBuildingInfo(iBuilding).getProductionCost() > 0  and not isLimitedWonderClass(gc.getBuildingInfo(iBuilding).getBuildingClassType())):
			listBuildings.append(iBuilding)

	for i in range(iNumBuildings+1):
		if len(listBuildings) > 0:
			iBuilding = listBuildings[gc.getGame().getSorenRandNum(len(listBuildings), "Looters event building destroyed")]
			szBuffer = localText.getText("TXT_KEY_EVENT_CITY_IMPROVEMENT_DESTROYED", (gc.getBuildingInfo(iBuilding).getTextKey(), ))
			CyInterface().addMessage(kTriggeredData.eOtherPlayer, false, gc.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_BOMBARDED", InterfaceMessageTypes.MESSAGE_TYPE_INFO, gc.getBuildingInfo(iBuilding).getButton(), gc.getInfoTypeForString("COLOR_RED"), city.getX(), city.getY(), true, true)
			city.setNumRealBuilding(iBuilding, 0)
			iNumBuildingsDestroyed += 1
			listBuildings.remove(iBuilding)
				
	if iNumBuildingsDestroyed > 0:
		szBuffer = localText.getText("TXT_KEY_EVENT_NUM_BUILDINGS_DESTROYED", (iNumBuildingsDestroyed, gc.getPlayer(kTriggeredData.eOtherPlayer).getCivilizationAdjectiveKey(), city.getNameKey()))
		CyInterface().addMessage(kTriggeredData.ePlayer, false, gc.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_BOMBARDED", InterfaceMessageTypes.MESSAGE_TYPE_INFO, None, gc.getInfoTypeForString("COLOR_WHITE"), -1, -1, true, true)

######## BROTHERS IN NEED ###########

def canTriggerBrothersInNeed(argsList):
	kTriggeredData = argsList[0]
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	player = gc.getPlayer(kTriggeredData.ePlayer)
	otherPlayer = gc.getPlayer(kTriggeredData.eOtherPlayer)
	
	if not player.canTradeNetworkWith(kTriggeredData.eOtherPlayer):
		return false
	
	listResources = []
	listResources.append(CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_COPPER'))
	listResources.append(CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_IRON'))
	listResources.append(CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_HORSE'))
	listResources.append(CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_IVORY'))
	listResources.append(CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_OIL'))
	listResources.append(CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_URANIUM'))

	bFound = false
	for iResource in listResources: 
		if (player.getNumTradeableBonuses(iResource) > 1 and otherPlayer.getNumAvailableBonuses(iResource) <= 0):
			bFound = true
			break
		
	if not bFound:
		return false
		
	for iTeam in range(gc.getMAX_CIV_TEAMS()):
		if iTeam != player.getTeam() and iTeam != otherPlayer.getTeam() and gc.getTeam(iTeam).isAlive():
			if gc.getTeam(iTeam).isAtWar(otherPlayer.getTeam()) and not gc.getTeam(iTeam).isAtWar(player.getTeam()):
				return true
			
	return false
	
def canDoBrothersInNeed1(argsList):
	kTriggeredData = argsList[1]
	newArgs = (kTriggeredData, )
	
	return canTriggerBrothersInNeed(newArgs)
	
######## HURRICANE ###########

def canTriggerHurricaneCity(argsList):
	eTrigger = argsList[0]
	ePlayer = argsList[1]
	iCity = argsList[2]
	
	player = gc.getPlayer(ePlayer)
	city = player.getCity(iCity)
	
	if city.isNone():
		return false
		
	if not city.isCoastal(gc.getMIN_WATER_SIZE_FOR_OCEAN()):
		return false
		
	if city.plot().getLatitude() <= 0:
		return false
		
	if city.getPopulation() < 2:
		return false
		
	return true

def canApplyHurricane1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	city = player.getCity(kTriggeredData.iCityId)
	
	listBuildings = []	
	for iBuilding in range(gc.getNumBuildingInfos()):
		if (city.getNumRealBuilding(iBuilding) > 0 and gc.getBuildingInfo(iBuilding).getProductionCost() > 0 and not isLimitedWonderClass(gc.getBuildingInfo(iBuilding).getBuildingClassType())):
			listBuildings.append(iBuilding)
			
	return (len(listBuildings) > 0)

def canApplyHurricane2(argsList):			
	return (not canApplyHurricane1(argsList))

		
def applyHurricane1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	city = player.getCity(kTriggeredData.iCityId)
	
	listCheapBuildings = []	
	listExpensiveBuildings = []	
	for iBuilding in range(gc.getNumBuildingInfos()):
		if (city.getNumRealBuilding(iBuilding) > 0 and gc.getBuildingInfo(iBuilding).getProductionCost() <= 100 and gc.getBuildingInfo(iBuilding).getProductionCost() > 0 and not isLimitedWonderClass(gc.getBuildingInfo(iBuilding).getBuildingClassType())):
			listCheapBuildings.append(iBuilding)
		if (city.getNumRealBuilding(iBuilding) > 0 and gc.getBuildingInfo(iBuilding).getProductionCost() > 100 and gc.getBuildingInfo(iBuilding).getProductionCost() > 0 and not isLimitedWonderClass(gc.getBuildingInfo(iBuilding).getBuildingClassType())):
			listExpensiveBuildings.append(iBuilding)

	if len(listCheapBuildings) > 0:
		iBuilding = listCheapBuildings[gc.getGame().getSorenRandNum(len(listCheapBuildings), "Hurricane event cheap building destroyed")]
		szBuffer = localText.getText("TXT_KEY_EVENT_CITY_IMPROVEMENT_DESTROYED", (gc.getBuildingInfo(iBuilding).getTextKey(), ))
		CyInterface().addMessage(kTriggeredData.ePlayer, false, gc.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_BOMBARDED", InterfaceMessageTypes.MESSAGE_TYPE_INFO, gc.getBuildingInfo(iBuilding).getButton(), gc.getInfoTypeForString("COLOR_RED"), city.getX(), city.getY(), true, true)
		city.setNumRealBuilding(iBuilding, 0)

	if len(listExpensiveBuildings) > 0:
		iBuilding = listExpensiveBuildings[gc.getGame().getSorenRandNum(len(listExpensiveBuildings), "Hurricane event expensive building destroyed")]
		szBuffer = localText.getText("TXT_KEY_EVENT_CITY_IMPROVEMENT_DESTROYED", (gc.getBuildingInfo(iBuilding).getTextKey(), ))
		CyInterface().addMessage(kTriggeredData.ePlayer, false, gc.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_BOMBARDED", InterfaceMessageTypes.MESSAGE_TYPE_INFO, gc.getBuildingInfo(iBuilding).getButton(), gc.getInfoTypeForString("COLOR_RED"), city.getX(), city.getY(), true, true)
		city.setNumRealBuilding(iBuilding, 0)

		
######## CYCLONE ###########

def canTriggerCycloneCity(argsList):
	eTrigger = argsList[0]
	ePlayer = argsList[1]
	iCity = argsList[2]
	
	player = gc.getPlayer(ePlayer)
	city = player.getCity(iCity)
	
	if city.isNone():
		return false
		
	if not city.isCoastal(gc.getMIN_WATER_SIZE_FOR_OCEAN()):
		return false
		
	if city.plot().getLatitude() >= 0:
		return false
		
	return true

######## TSUNAMI ###########

def canTriggerTsunamiCity(argsList):
	eTrigger = argsList[0]
	ePlayer = argsList[1]
	iCity = argsList[2]
	
	player = gc.getPlayer(ePlayer)
	city = player.getCity(iCity)
	
	if city.isNone():
		return false
		
	if not city.isCoastal(gc.getMIN_WATER_SIZE_FOR_OCEAN()):
		return false
				
	return true

def canApplyTsunami1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	city = player.getCity(kTriggeredData.iCityId)
	
	return (city.getPopulation() < 6)

def canApplyTsunami2(argsList):			
	return (not canApplyTsunami1(argsList))

		
def applyTsunami1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	city = player.getCity(kTriggeredData.iCityId)
	
	city.kill()
	
def applyTsunami2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	city = player.getCity(kTriggeredData.iCityId)
	
	listBuildings = []	
	for iBuilding in range(gc.getNumBuildingInfos()):
		if (city.getNumRealBuilding(iBuilding) > 0 and gc.getBuildingInfo(iBuilding).getProductionCost() > 0  and not isLimitedWonderClass(gc.getBuildingInfo(iBuilding).getBuildingClassType())):
			listBuildings.append(iBuilding)

	for i in range(5):
		if len(listBuildings) > 0:
			iBuilding = listBuildings[gc.getGame().getSorenRandNum(len(listBuildings), "Tsunami event building destroyed")]
			szBuffer = localText.getText("TXT_KEY_EVENT_CITY_IMPROVEMENT_DESTROYED", (gc.getBuildingInfo(iBuilding).getTextKey(), ))
			CyInterface().addMessage(kTriggeredData.ePlayer, false, gc.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_BOMBARDED", InterfaceMessageTypes.MESSAGE_TYPE_INFO, gc.getBuildingInfo(iBuilding).getButton(), gc.getInfoTypeForString("COLOR_RED"), city.getX(), city.getY(), true, true)
			city.setNumRealBuilding(iBuilding, 0)
			listBuildings.remove(iBuilding)
					

def getHelpTsunami2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	city = player.getCity(kTriggeredData.iCityId)

	szHelp = localText.getText("TXT_KEY_EVENT_TSUNAMI_2_HELP", (5, city.getNameKey()))

	return szHelp

		
######## MONSOON ###########

def canTriggerMonsoonCity(argsList):
	eTrigger = argsList[0]
	ePlayer = argsList[1]
	iCity = argsList[2]
	
	player = gc.getPlayer(ePlayer)
	city = player.getCity(iCity)
	
	if city.isNone():
		return false
		
	if city.isCoastal(gc.getMIN_WATER_SIZE_FOR_OCEAN()):
		return false
		
	iJungleType = CvUtil.findInfoTypeNum(gc.getFeatureInfo, gc.getNumFeatureInfos(),'FEATURE_JUNGLE')
		
	for iDX in range(-3, 4):
		for iDY in range(-3, 4):
			pLoopPlot = plotXY(city.getX(), city.getY(), iDX, iDY)
			if not pLoopPlot.isNone() and pLoopPlot.getFeatureType() == iJungleType:
				return true
				
	return false

######## VOLCANO ###########

def getHelpVolcano1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	szHelp = localText.getText("TXT_KEY_EVENT_VOLCANO_1_HELP", ())

	return szHelp

def canApplyVolcano1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	iNumImprovements = 0
	for iDX in range(-1, 2):
		for iDY in range(-1, 2):
			loopPlot = plotXY(kTriggeredData.iPlotX, kTriggeredData.iPlotY, iDX, iDY)
			if not loopPlot.isNone():
				if (iDX != 0 or iDY != 0):
					if loopPlot.getImprovementType() != -1:
						iNumImprovements += 1

	return (iNumImprovements > 0)

def applyVolcano1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	listPlots = []
	for iDX in range(-1, 2):
		for iDY in range(-1, 2):
			loopPlot = plotXY(kTriggeredData.iPlotX, kTriggeredData.iPlotY, iDX, iDY)
			if not loopPlot.isNone():
				if (iDX != 0 or iDY != 0):
					if loopPlot.getImprovementType() != -1:
						listPlots.append(loopPlot)
					
	listRuins = []
	## K-Mod: I don't think cottages and hamlets are big enough to leave "city ruins"
	#listRuins.append(CvUtil.findInfoTypeNum(gc.getImprovementInfo,gc.getNumImprovementInfos(),'IMPROVEMENT_COTTAGE'))
	#listRuins.append(CvUtil.findInfoTypeNum(gc.getImprovementInfo,gc.getNumImprovementInfos(),'IMPROVEMENT_HAMLET'))
	listRuins.append(CvUtil.findInfoTypeNum(gc.getImprovementInfo,gc.getNumImprovementInfos(),'IMPROVEMENT_VILLAGE'))
	listRuins.append(CvUtil.findInfoTypeNum(gc.getImprovementInfo,gc.getNumImprovementInfos(),'IMPROVEMENT_TOWN'))
	
	iRuins = CvUtil.findInfoTypeNum(gc.getImprovementInfo,gc.getNumImprovementInfos(),'IMPROVEMENT_CITY_RUINS')

	for i in range(3):
		if len(listPlots) > 0:
			plot = listPlots[gc.getGame().getSorenRandNum(len(listPlots), "Volcano event improvement destroyed")]
			iImprovement = plot.getImprovementType()
			szBuffer = localText.getText("TXT_KEY_EVENT_CITY_IMPROVEMENT_DESTROYED", (gc.getImprovementInfo(iImprovement).getTextKey(), ))
			#CyInterface().addMessage(kTriggeredData.ePlayer, false, gc.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_BOMBARDED", InterfaceMessageTypes.MESSAGE_TYPE_INFO, gc.getImprovementInfo(iImprovement).getButton(), gc.getInfoTypeForString("COLOR_RED"), plot.getX(), plot.getY(), true, true)
			# K-Mod
			if plot.getOwner() != PlayerTypes.NO_PLAYER:
				plot_owner = plot.getOwner()
			else:
				plot_owner = kTriggeredData.ePlayer
			if (gc.getPlayer(plot_owner).isHuman()):
				CyInterface().addMessage(plot_owner, false, gc.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_BOMBARDED", InterfaceMessageTypes.MESSAGE_TYPE_INFO, gc.getImprovementInfo(iImprovement).getButton(), gc.getInfoTypeForString("COLOR_RED"), plot.getX(), plot.getY(), true, true)
			# K-Mod end
			if iImprovement in listRuins:
				plot.setImprovementType(iRuins)
			else:
				plot.setImprovementType(-1)
			listPlots.remove(plot)
			
			if i == 1 and gc.getGame().getSorenRandNum(100, "Volcano event num improvements destroyed") < 50:
				break
##
# K-Mod, karadoc, 26/Jun/2011
# Volcanic ash improves some tiles
##
	sEventType = gc.getEventInfo(iEvent).getType(); # Event name string
	listPlots = []
	for iDX in range(-1, 2):
		for iDY in range(-1, 2):
			loopPlot = plotXY(kTriggeredData.iPlotX, kTriggeredData.iPlotY, iDX, iDY)
			if not loopPlot.isNone() and not loopPlot.isWater():
				if (iDX != 0 or iDY != 0):
					if gc.getGame().getPlotExtraYield(loopPlot.getX(), loopPlot.getY(), YieldTypes.YIELD_FOOD) == 0:
						listPlots.append(loopPlot)

	# zero - 50%, one - 25%, two - 25%
	iCount = gc.getGame().getSorenRandNum(4, "Volcanic ash fertility bonus") - 1
							
	for i in range(iCount):
		if len(listPlots) <= 0:
			break
		plot = listPlots[gc.getGame().getSorenRandNum(len(listPlots), "Volcano event increased yield")]
		gc.getGame().setPlotExtraYield(plot.getX(), plot.getY(), YieldTypes.YIELD_FOOD, 1)
		placeLandmark(plot, sEventType, 1, 0, 0, True, -1) # event sign
		if plot.getOwner() != PlayerTypes.NO_PLAYER:
			plot_owner = plot.getOwner()
		else:
			plot_owner = kTriggeredData.ePlayer
		if (gc.getPlayer(plot_owner).isHuman()):
			CyInterface().addMessage(plot_owner, false, gc.getEVENT_MESSAGE_TIME(), localText.getText("TXT_KEY_EVENT_VOLCANO_FERTILITY", ()), "", InterfaceMessageTypes.MESSAGE_TYPE_INFO, None, gc.getInfoTypeForString("COLOR_WHITE"), plot.getX(), plot.getY(), true, true)
		listPlots.remove(plot)
# K-Mod end

######## DUSTBOWL ###########

def canTriggerDustbowlCont(argsList):
	kTriggeredData = argsList[0]

	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	kOrigTriggeredData = player.getEventOccured(trigger.getPrereqEvent(0))
	
	if (kOrigTriggeredData == None):
		return false

	iFarmType = CvUtil.findInfoTypeNum(gc.getImprovementInfo,gc.getNumImprovementInfos(),'IMPROVEMENT_FARM')
	iPlainsType = CvUtil.findInfoTypeNum(gc.getTerrainInfo,gc.getNumTerrainInfos(),'TERRAIN_PLAINS')
	
	map = gc.getMap()
	iBestValue = map.getGridWidth() + map.getGridHeight()
	bestPlot = None
	for i in range(map.numPlots()):
		plot = map.plotByIndex(i)
		if (plot.getOwner() == kTriggeredData.ePlayer and plot.getImprovementType() == iFarmType and plot.getTerrainType() == iPlainsType):
			iValue = plotDistance(kOrigTriggeredData.iPlotX, kOrigTriggeredData.iPlotY, plot.getX(), plot.getY())
			if iValue < iBestValue:
				iBestValue = iValue
				bestPlot = plot
				
	if bestPlot != None:
		kActualTriggeredDataObject = player.getEventTriggered(kTriggeredData.iId)
		kActualTriggeredDataObject.iPlotX = bestPlot.getX()
		kActualTriggeredDataObject.iPlotY = bestPlot.getY()
	else:
		player.resetEventOccured(trigger.getPrereqEvent(0))
		return false
		
	return true

def getHelpDustBowl2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	szHelp = localText.getText("TXT_KEY_EVENT_DUSTBOWL_2_HELP", ())

	return szHelp

######## SALTPETER ###########

def getSaltpeterNumExtraPlots():
	map = gc.getMap()	
	if map.getWorldSize() <= 1:
		return 1
	elif map.getWorldSize() <= 3:
		return 2
	elif map.getWorldSize() <= 4:
		return 3
	else:
		return 4

def getHelpSaltpeter(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	szHelp = localText.getText("TXT_KEY_EVENT_SALTPETER_HELP", (getSaltpeterNumExtraPlots(), ))

	return szHelp

def canApplySaltpeter(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	map = gc.getMap()
	
	player = gc.getPlayer(kTriggeredData.ePlayer)

	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	if (plot == None):
		return false
		
	iForest = CvUtil.findInfoTypeNum(gc.getFeatureInfo,gc.getNumFeatureInfos(),'FEATURE_FOREST')
	
	iNumPlots = 0
	for i in range(map.numPlots()):
		loopPlot = map.plotByIndex(i)
		if (loopPlot.getOwner() == kTriggeredData.ePlayer and loopPlot.getFeatureType() == iForest and loopPlot.isHills()):
			iDistance = plotDistance(kTriggeredData.iPlotX, kTriggeredData.iPlotY, loopPlot.getX(), loopPlot.getY())
			if iDistance > 0:
				iNumPlots += 1
	
	return (iNumPlots >= getSaltpeterNumExtraPlots())

# K-Mod. EventSigns originally overwrote applySaltpeter with an equivalent function in EventSigns.py.
# I think it makes much more sense to just apply the changes here directly - so that's what I've done.
def applySaltpeter(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	# EventSigns start -- setup
	event = gc.getEventInfo(iEvent)
	iFood = event.getPlotExtraYield(YieldTypes.YIELD_FOOD)
	iProd = event.getPlotExtraYield(YieldTypes.YIELD_PRODUCTION)
	iComm = event.getPlotExtraYield(YieldTypes.YIELD_COMMERCE)
	sEventType = event.getType()
	# EventSigns end

	map = gc.getMap()
	
	player = gc.getPlayer(kTriggeredData.ePlayer)

	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	if (plot == None):
		return
	# EventSigns start -- Add landmark for initial plot, if there is still a yield change
	placeLandmark(plot, sEventType, iFood, iProd, iComm, True, -1)
	# EventSigns end
		
	iForest = gc.getInfoTypeForString('FEATURE_FOREST')
	
	listPlots = []
	for i in range(map.numPlots()):
		loopPlot = map.plotByIndex(i)
		if (loopPlot.getOwner() == kTriggeredData.ePlayer and loopPlot.getFeatureType() == iForest and loopPlot.isHills()):
			iDistance = plotDistance(kTriggeredData.iPlotX, kTriggeredData.iPlotY, loopPlot.getX(), loopPlot.getY())
			if iDistance > 0:
				listPlots.append((iDistance, loopPlot))

	#listPlots.sort()
	listPlots.sort(key=itemgetter(0)) # K-Mod. Sorting by pointers can cause OOS.
	
	iCount = getSaltpeterNumExtraPlots()
	for loopPlot in listPlots:
		if iCount == 0:
			break
		iCount -= 1
		gc.getGame().setPlotExtraYield(loopPlot[1].getX(), loopPlot[1].getY(), YieldTypes.YIELD_COMMERCE, 1)
		CyInterface().addMessage(kTriggeredData.ePlayer, false, gc.getEVENT_MESSAGE_TIME(), localText.getText("TXT_KEY_EVENT_SALTPETER_DISCOVERED", ()), "", InterfaceMessageTypes.MESSAGE_TYPE_INFO, None, gc.getInfoTypeForString("COLOR_WHITE"), loopPlot[1].getX(), loopPlot[1].getY(), true, true)
		# EventSigns start -- Add landmark for other plots, if there is still a yield change
		placeLandmark(loopPlot[1], sEventType, 0, 0, 1, True, -1) # K-Mod. (originally it used iFood, iProd, iComm -- and that's not correct.)
		# EventSigns end

######## GREAT DEPRESSION ###########

def applyGreatDepression(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	corporation = gc.getCorporationInfo(kTriggeredData.eCorporation)
	
	for iPlayer in range(gc.getMAX_CIV_PLAYERS()):			
		loopPlayer = gc.getPlayer(iPlayer)
		if loopPlayer.isAlive():
			loopPlayer.changeGold(-loopPlayer.getGold()/4)	
			
			if iPlayer != kTriggeredData.ePlayer:
				szText = localText.getText("TXT_KEY_EVENTTRIGGER_GREAT_DEPRESSION", (player.getCivilizationAdjectiveKey(), u"", u"", u"", u"", corporation.getTextKey()))
				szText += u"\n\n" + localText.getText("TXT_KEY_EVENT_GREAT_DEPRESSION_HELP", (25, ))
				popupInfo = CyPopupInfo()
				popupInfo.setText(szText)
				popupInfo.addPopup(iPlayer)
			
	
def getHelpGreatDepression(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	szHelp = localText.getText("TXT_KEY_EVENT_GREAT_DEPRESSION_HELP", (25, ))	

	return szHelp
	
######## CHAMPION ###########

def canTriggerChampion(argsList):	
	kTriggeredData = argsList[0]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	team = gc.getTeam(player.getTeam())

	if team.getAtWarCount(true) > 0:
		return false
				
	return true
	
def canTriggerChampionUnit(argsList):
	eTrigger = argsList[0]
	ePlayer = argsList[1]
	iUnit = argsList[2]
	
	player = gc.getPlayer(ePlayer)
	unit = player.getUnit(iUnit)
	
	if unit.isNone():
		return false
		
	if unit.getDamage() > 0:
		return false
		
	if unit.getExperience() < 3:
		return false

	iLeadership = CvUtil.findInfoTypeNum(gc.getPromotionInfo,gc.getNumPromotionInfos(),'PROMOTION_LEADERSHIP')
	if unit.isHasPromotion(iLeadership):
		return false

	return true
	
def applyChampion(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	player = gc.getPlayer(kTriggeredData.ePlayer)
	unit = player.getUnit(kTriggeredData.iUnitId)
	
	iLeadership = CvUtil.findInfoTypeNum(gc.getPromotionInfo,gc.getNumPromotionInfos(),'PROMOTION_LEADERSHIP')
	
	unit.setHasPromotion(iLeadership, true)
	
def getHelpChampion(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	unit = player.getUnit(kTriggeredData.iUnitId)
	
	iLeadership = CvUtil.findInfoTypeNum(gc.getPromotionInfo,gc.getNumPromotionInfos(),'PROMOTION_LEADERSHIP')

	szHelp = localText.getText("TXT_KEY_EVENT_CHAMPION_HELP", (unit.getNameKey(), gc.getPromotionInfo(iLeadership).getTextKey()))	

	return szHelp
	
######## ELECTRIC COMPANY ###########

def canTriggerElectricCompany(argsList):	
	kTriggeredData = argsList[0]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	(loopCity, iter) = player.firstCity(false)

	while(loopCity):

		if (loopCity.angryPopulation(0) > 0):
			return false
				
		(loopCity, iter) = player.nextCity(iter, false)
						
	return true
	
######## GOLD RUSH ###########

def canTriggerGoldRush(argsList):	
	kTriggeredData = argsList[0]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iIndustrial = CvUtil.findInfoTypeNum(gc.getEraInfo,gc.getNumEraInfos(),'ERA_INDUSTRIAL')
	
	if player.getCurrentEra() != iIndustrial:
		return false
	
						
	return true
	
######## INFLUENZA ###########

def canTriggerInfluenza(argsList):	
	kTriggeredData = argsList[0]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	team = gc.getTeam(player.getTeam())
	
	iIndustrial = CvUtil.findInfoTypeNum(gc.getEraInfo,gc.getNumEraInfos(),'ERA_INDUSTRIAL')
	
	if player.getCurrentEra() <= iIndustrial:
		return false
	
	iMedicine = CvUtil.findInfoTypeNum(gc.getTechInfo,gc.getNumTechInfos(),'TECH_MEDICINE')
	
	if team.isHasTech(iMedicine):
		return false
						
	return true
	
def applyInfluenza2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
		
	player = gc.getPlayer(kTriggeredData.ePlayer)
	eventCity = player.getCity(kTriggeredData.iCityId)

	iNumCities = 2 + gc.getGame().getSorenRandNum(3, "Influenza event number of cities")

	listCities = []	
	(loopCity, iter) = player.firstCity(false)
	while(loopCity):
		if loopCity.getPopulation() > 2:
			iDistance = plotDistance(eventCity.getX(), eventCity.getY(), loopCity.getX(), loopCity.getY())
			if iDistance > 0:
				listCities.append((iDistance, loopCity))
		(loopCity, iter) = player.nextCity(iter, false)
		
	#listCities.sort()
	listCities.sort(key=itemgetter(0)) # K-Mod. Sorting by pointers can cause OOS.
	
	if iNumCities > len(listCities): 
		iNumCities = len(listCities)
				
	for i in range(iNumCities):
		(iDist, loopCity) = listCities[i]
		loopCity.changePopulation(-2)
		szBuffer = localText.getText("TXT_KEY_EVENT_INFLUENZA_HIT_CITY", (loopCity.getNameKey(), ))
		CyInterface().addMessage(kTriggeredData.ePlayer, false, gc.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_PILLAGE", InterfaceMessageTypes.MESSAGE_TYPE_INFO, None, gc.getInfoTypeForString("COLOR_RED"), loopCity.getX(), loopCity.getY(), true, true)
				

def getHelpInfluenza2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	szHelp = localText.getText("TXT_KEY_EVENT_INFLUENZA_HELP_2", (2, ))	

	return szHelp

######## SOLO FLIGHT ###########


def canTriggerSoloFlight(argsList):	
	kTriggeredData = argsList[0]

	map = gc.getMap()	
	if map.getWorldSize() == CvUtil.findInfoTypeNum(gc.getWorldInfo, gc.getNumWorldInfos(), 'WORLDSIZE_DUEL'):
		iMinLandmass  = 3
	elif map.getWorldSize() == CvUtil.findInfoTypeNum(gc.getWorldInfo, gc.getNumWorldInfos(), 'WORLDSIZE_TINY'):
		iMinLandmass  = 4
	elif map.getWorldSize() == CvUtil.findInfoTypeNum(gc.getWorldInfo, gc.getNumWorldInfos(), 'WORLDSIZE_SMALL'):
		iMinLandmass  = 6
	elif map.getWorldSize() == CvUtil.findInfoTypeNum(gc.getWorldInfo, gc.getNumWorldInfos(), 'WORLDSIZE_STANDARD'):
		iMinLandmass  = 8
	elif map.getWorldSize() == CvUtil.findInfoTypeNum(gc.getWorldInfo, gc.getNumWorldInfos(), 'WORLDSIZE_LARGE'):
		iMinLandmass  = 10
	else:
		iMinLandmass  = 12
	
	if (map.getNumLandAreas() < iMinLandmass):
		return false
		
	if gc.getGame().isGameMultiPlayer():
		return false
	
	return true

def getHelpSoloFlight(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	szHelp = localText.getText("TXT_KEY_EVENT_SOLO_FLIGHT_HELP_1", (1, ))	

	return szHelp

def applySoloFlight(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	for iPlayer in range(gc.getMAX_CIV_PLAYERS()):			
		loopPlayer = gc.getPlayer(iPlayer)
		if loopPlayer.isAlive() and iPlayer != kTriggeredData.ePlayer:
			loopTeam = gc.getTeam(loopPlayer.getTeam())
			if loopTeam.isHasMet(gc.getPlayer(kTriggeredData.ePlayer).getTeam()):
				loopPlayer.AI_changeAttitudeExtra(kTriggeredData.ePlayer, 1)
						

######## ANTELOPE ###########

def canTriggerAntelope(argsList):

	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iDeer = CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_DEER')
	iHappyBonuses = 0
	bDeer = false
	for i in range(gc.getNumBonusInfos()):
		bonus = gc.getBonusInfo(i)
		iNum = player.getNumAvailableBonuses(i)
		if iNum > 0 :
			if bonus.getHappiness() > 0:
				iHappyBonuses += 1
				if iHappyBonuses > 5:
					return false
			if i == iDeer:
				return false	

	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	if not plot.canHaveBonus(iDeer, false):
		return false
				
	return true

def doAntelope2(argsList):
#	Need this because camps are not normally allowed unless there is already deer.
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	
	if not plot.isNone():
		plot.setImprovementType(CvUtil.findInfoTypeNum(gc.getImprovementInfo,gc.getNumImprovementInfos(),'IMPROVEMENT_CAMP'))
	
	return 1

def getHelpAntelope2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	iCamp = CvUtil.findInfoTypeNum(gc.getImprovementInfo,gc.getNumImprovementInfos(),'IMPROVEMENT_CAMP')
	szHelp = localText.getText("TXT_KEY_EVENT_IMPROVEMENT_GROWTH", ( gc.getImprovementInfo(iCamp).getTextKey(), ))

	return szHelp

######## WHALEOFATHING ###########

def canTriggerWhaleOfAThing(argsList):

	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iWhale = CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_WHALE')
	iHappyBonuses = 0
	bWhale = false
	for i in range(gc.getNumBonusInfos()):
		bonus = gc.getBonusInfo(i)
		iNum = player.getNumAvailableBonuses(i)
		if iNum > 0 :
			if bonus.getHappiness() > 0:
				iHappyBonuses += 1
				if iHappyBonuses > 5:
					return false
			if i == iWhale:
				return false

	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	if not plot.canHaveBonus(iWhale, false):
		return false
		
	return true


######## HIYOSILVER ###########

def canTriggerHiyoSilver(argsList):

	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iSilver = CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_SILVER')
	iHappyBonuses = 0
	bSilver = false
	for i in range(gc.getNumBonusInfos()):
		bonus = gc.getBonusInfo(i)
		iNum = player.getNumAvailableBonuses(i)
		if iNum > 0 :
			if bonus.getHappiness() > 0:
				iHappyBonuses += 1
				if iHappyBonuses > 5:
					return false
			if i == iSilver:
				return false

	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	if not plot.canHaveBonus(iSilver, false):
		return false
				
	return true

######## WININGMONKS ###########

def canTriggerWiningMonks(argsList):

	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	if player.getNumAvailableBonuses(CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_WINE')) > 0:
		return false
				
	return true


def doWiningMonks2(argsList):
#	Need this because wineries are not normally allowed unless there is already wine.
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	
	if not plot.isNone():
		plot.setImprovementType(CvUtil.findInfoTypeNum(gc.getImprovementInfo,gc.getNumImprovementInfos(),'IMPROVEMENT_WINERY'))
	
	return 1

def getHelpWiningMonks2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	iImp = CvUtil.findInfoTypeNum(gc.getImprovementInfo,gc.getNumImprovementInfos(),'IMPROVEMENT_WINERY')
	szHelp = localText.getText("TXT_KEY_EVENT_IMPROVEMENT_GROWTH", ( gc.getImprovementInfo(iImp).getTextKey(), ))

	return szHelp


######## INDEPENDENTFILMS ###########

def canTriggerIndependentFilms(argsList):

	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	for i in range(gc.getNumBuildingInfos()):
		if gc.getBuildingInfo(i).getFreeBonus() == CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_MOVIES'):
			if player.countNumBuildings(i) > 0:
				return false
				
	return true

def doIndependentFilms(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	city = player.getCity(kTriggeredData.iCityId)

	iBonus = CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_MOVIES')

	city.changeFreeBonus(iBonus, 1)
	
	return 1

def getHelpIndependentFilms(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	city = player.getCity(kTriggeredData.iCityId)

	iBonus = CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_MOVIES')
	
	szHelp = localText.getText("TXT_KEY_EVENT_INDEPENDENTFILMS_HELP_1", ( 1, gc.getBonusInfo(iBonus).getChar(), city.getNameKey()))

	return szHelp

######## ANCIENT OLYMPICS ###########

def canTriggerAncientOlympics(argsList):

	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	stateReligion = player.getStateReligion()
	
	if stateReligion == CvUtil.findInfoTypeNum(gc.getReligionInfo,gc.getNumReligionInfos(),'RELIGION_JUDAISM'):
		return false

	if stateReligion == CvUtil.findInfoTypeNum(gc.getReligionInfo,gc.getNumReligionInfos(),'RELIGION_CHRISTIANITY'):
		return false

	if stateReligion == CvUtil.findInfoTypeNum(gc.getReligionInfo,gc.getNumReligionInfos(),'RELIGION_ISLAM'):
		return false

	return true

def doAncientOlympics2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	map = gc.getMap()

	for j in range(gc.getMAX_CIV_PLAYERS()):
		loopPlayer = gc.getPlayer(j)
		if j != kTriggeredData.ePlayer and loopPlayer.isAlive() and not loopPlayer.isMinorCiv():

			for i in range(map.numPlots()):
				plot = map.plotByIndex(i)
				if not plot.isWater() and plot.getOwner() == kTriggeredData.ePlayer and plot.isAdjacentPlayer(j, true):
					loopPlayer.AI_changeMemoryCount(kTriggeredData.ePlayer, MemoryTypes.MEMORY_EVENT_GOOD_TO_US, 1)
					break
		
	return 1

def getHelpAncientOlympics2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
		
	szHelp = localText.getText("TXT_KEY_EVENT_ANCIENTOLYMPICS_HELP_2", ( 1, ))

	return szHelp


######## MODERN OLYMPICS ###########

def canTriggerModernOlympics(argsList):

	kTriggeredData = argsList[0]
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	kOrigTriggeredData = player.getEventOccured(trigger.getPrereqEvent(0))
	
	if (kOrigTriggeredData == None):
		return false

	kActualTriggeredDataObject = player.getEventTriggered(kTriggeredData.iId)
	kActualTriggeredDataObject.iCityId = kOrigTriggeredData.iCityId
	kActualTriggeredDataObject.iPlotX = kOrigTriggeredData.iPlotX
	kActualTriggeredDataObject.iPlotY = kOrigTriggeredData.iPlotY
	
	return true

def getHelpModernOlympics(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	szHelp = localText.getText("TXT_KEY_EVENT_SOLO_FLIGHT_HELP_1", (1, ))	

	return szHelp

def applyModernOlympics(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	for iPlayer in range(gc.getMAX_CIV_PLAYERS()):			
		loopPlayer = gc.getPlayer(iPlayer)
		if loopPlayer.isAlive() and iPlayer != kTriggeredData.ePlayer:
			loopTeam = gc.getTeam(loopPlayer.getTeam())
			if loopTeam.isHasMet(gc.getPlayer(kTriggeredData.ePlayer).getTeam()):
				loopPlayer.AI_changeAttitudeExtra(kTriggeredData.ePlayer, 1)
						

######## INTERSTATE ###########

def canTriggerInterstate(argsList):

	kTriggeredData = argsList[0]
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	if not player.isCivic(CvUtil.findInfoTypeNum(gc.getCivicInfo,gc.getNumCivicInfos(),'CIVIC_EMANCIPATION')):
		return false
	
	return true

def getHelpInterstate(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	szHelp = localText.getText("TXT_KEY_UNIT_MOVEMENT", (1, gc.getRouteInfo(CvUtil.findInfoTypeNum(gc.getRouteInfo,gc.getNumRouteInfos(),'ROUTE_ROAD')).getTextKey()))	

	return szHelp

def applyInterstate(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	team = gc.getTeam(player.getTeam())
	
	iRoad = CvUtil.findInfoTypeNum(gc.getRouteInfo,gc.getNumRouteInfos(),'ROUTE_ROAD')
						
	team.changeRouteChange(iRoad, -5)
	
######## EARTH DAY ###########

def getHelpEarthDay2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	szHelp = localText.getText("TXT_KEY_EVENT_EARTHDAY_HELP_2", ())	

	return szHelp

def canApplyEarthDay2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iCivic = CvUtil.findInfoTypeNum(gc.getCivicInfo,gc.getNumCivicInfos(),'CIVIC_ENVIRONMENTALISM')
	
	for iPlayer in range(gc.getMAX_CIV_PLAYERS()):			
		loopPlayer = gc.getPlayer(iPlayer)
		if loopPlayer.isAlive() and iPlayer != kTriggeredData.ePlayer and not loopPlayer.isHuman():
			loopTeam = gc.getTeam(loopPlayer.getTeam())
			if loopTeam.isHasMet(gc.getPlayer(kTriggeredData.ePlayer).getTeam()):
				tradeData = TradeData()
				tradeData.ItemType = TradeableItems.TRADE_CIVIC
				tradeData.iData = iCivic
				if loopPlayer.canTradeItem(kTriggeredData.ePlayer, tradeData, False):
					if (loopPlayer.getTradeDenial(kTriggeredData.ePlayer, tradeData) == DenialTypes.NO_DENIAL):
						return true
	return false
	
		
def applyEarthDay2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iCivic = CvUtil.findInfoTypeNum(gc.getCivicInfo,gc.getNumCivicInfos(),'CIVIC_ENVIRONMENTALISM')
	iCivicOption = CvUtil.findInfoTypeNum(gc.getCivicOptionInfo,gc.getNumCivicOptionInfos(),'CIVICOPTION_ECONOMY')
	
	listPlayers = []
	for iPlayer in range(gc.getMAX_CIV_PLAYERS()):			
		loopPlayer = gc.getPlayer(iPlayer)
		if loopPlayer.isAlive() and iPlayer != kTriggeredData.ePlayer and not loopPlayer.isHuman():
			loopTeam = gc.getTeam(loopPlayer.getTeam())
			if loopTeam.isHasMet(gc.getPlayer(kTriggeredData.ePlayer).getTeam()):
				tradeData = TradeData()
				tradeData.ItemType = TradeableItems.TRADE_CIVIC
				tradeData.iData = iCivic
				if loopPlayer.canTradeItem(kTriggeredData.ePlayer, tradeData, False):
					if (loopPlayer.getTradeDenial(kTriggeredData.ePlayer, tradeData) == DenialTypes.NO_DENIAL):
						listPlayers.append((-loopPlayer.AI_civicValue(iCivic), iPlayer))
						
	#listPlayers.sort()	
	listPlayers.sort(key=itemgetter(0)) # K-Mod. For consistent usage of 'sort', ignore the player id.
	
	if len(listPlayers) > 3:
		listPlayers = listPlayers[:2]
	
	for (iValue, iPlayer) in listPlayers:
		gc.getPlayer(iPlayer).setCivics(iCivicOption, iCivic)
		
######## FREEDOM CONCERT ###########

def getHelpFreedomConcert2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	szHelp = localText.getText("TXT_KEY_EVENT_FREEDOMCONCERT_HELP_2", ())	

	return szHelp

def canApplyFreedomConcert2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	eventCity = player.getCity(kTriggeredData.iCityId)
	
	for iReligion in range(gc.getNumReligionInfos()):
		if eventCity.isHasReligion(iReligion):		
			(loopCity, iter) = player.firstCity(false)
			while(loopCity):
				if not loopCity.isHasReligion(iReligion):
					for jReligion in range(gc.getNumReligionInfos()):
						if loopCity.isHasReligion(jReligion):
							return true
				(loopCity, iter) = player.nextCity(iter, false)

	return false
		
def applyFreedomConcert2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	eventCity = player.getCity(kTriggeredData.iCityId)
	
	for iReligion in range(gc.getNumReligionInfos()):
		if eventCity.isHasReligion(iReligion):
		
			bestCity = None
			iBestDistance = 0
			(loopCity, iter) = player.firstCity(false)
			while(loopCity):
				if not loopCity.isHasReligion(iReligion):
					bValid = false
					for jReligion in range(gc.getNumReligionInfos()):
						if loopCity.isHasReligion(jReligion):
							bValid = true
							break
					
					if bValid:				
						iDistance = plotDistance(eventCity.getX(), eventCity.getY(), loopCity.getX(), loopCity.getY())
						
						if iDistance < iBestDistance or bestCity == None:
							bestCity = loopCity
							iBestDistance = iDistance
						
				(loopCity, iter) = player.nextCity(iter, false)
				

			if bestCity != None:									
				bestCity.setHasReligion(iReligion, true, true, true)

######## HEROIC_GESTURE ###########

def canTriggerHeroicGesture(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	destPlayer = gc.getPlayer(kTriggeredData.eOtherPlayer)

	if not gc.getTeam(destPlayer.getTeam()).canChangeWarPeace(player.getTeam()):
		return false
		
	if gc.getTeam(destPlayer.getTeam()).AI_getWarSuccess(player.getTeam()) <= 0:
		return false

	if gc.getTeam(player.getTeam()).AI_getWarSuccess(destPlayer.getTeam()) <= 0:
		return false

	# K-Mod. an ugly hack to prevent the AI from signing a peace deal when it really doesn't want to.
	tradeData = TradeData()
	tradeData.ItemType = TradeableItems.TRADE_PEACE_TREATY

	if player.getTradeDenial(kTriggeredData.eOtherPlayer, tradeData) != DenialTypes.NO_DENIAL:
		return false

	if destPlayer.getTradeDenial(kTriggeredData.ePlayer, tradeData) != DenialTypes.NO_DENIAL:
		return false
	# K-Mod end

	return true

def doHeroicGesture2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	destPlayer = gc.getPlayer(kTriggeredData.eOtherPlayer)
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	if destPlayer.isHuman():
		# this works only because it's a single-player only event
		popupInfo = CyPopupInfo()
		popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON)
		popupInfo.setText(localText.getText("TXT_KEY_EVENT_HEROIC_GESTURE_OTHER_3", (player.getCivilizationAdjectiveKey(), )))
		popupInfo.setData1(kTriggeredData.eOtherPlayer)
		popupInfo.setData2(kTriggeredData.ePlayer)
		popupInfo.setPythonModule("CvRandomEventInterface")
		popupInfo.setOnClickedPythonCallback("heroicGesture2Callback")
		popupInfo.addPythonButton(localText.getText("TXT_KEY_POPUP_YES", ()), "")
		popupInfo.addPythonButton(localText.getText("TXT_KEY_POPUP_NO", ()), "")
		popupInfo.addPopup(kTriggeredData.eOtherPlayer)
	else:
		#destPlayer.forcePeace(kTriggeredData.ePlayer)
		# advc.106g: I don't think there's a difference between CvPlayer::forcePeace and CvTeam::makePeace, so I'm going to use the makePeaceEvent function to let the DLL know that peace happens through an event. 
		gc.getTeam(destPlayer.getTeam()).makePeaceEvent(player.getTeam())
		destPlayer.AI_changeAttitudeExtra(kTriggeredData.ePlayer, 1)
		player.AI_changeAttitudeExtra(kTriggeredData.eOtherPlayer, 1)

	return

def heroicGesture2Callback(argsList):
	iButton = argsList[0]
	iData1 = argsList[1]
	iData2 = argsList[2]
	iData3 = argsList[3]
	szText = argsList[4]
	bOption1 = argsList[5]
	bOption2 = argsList[6]
	
	if iButton == 0:
		destPlayer = gc.getPlayer(iData1)
		player = gc.getPlayer(iData2)
		destPlayer.forcePeace(iData2)
		destPlayer.AI_changeAttitudeExtra(iData2, 1)
		player.AI_changeAttitudeExtra(iData1, 1)		

	return 0
	
def getHelpHeroicGesture2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	destPlayer = gc.getPlayer(kTriggeredData.eOtherPlayer)

	# Get help text
	szHelp = localText.getText("TXT_KEY_EVENT_ATTITUDE_GOOD", (1, destPlayer.getNameKey()));

	return szHelp

######## GREAT_MEDIATOR ###########

def canTriggerGreatMediator(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	destPlayer = gc.getPlayer(kTriggeredData.eOtherPlayer)

	if not gc.getTeam(player.getTeam()).canChangeWarPeace(destPlayer.getTeam()):
		return false
		
	if gc.getTeam(player.getTeam()).AI_getAtWarCounter(destPlayer.getTeam()) < 10:
		return false

	# K-Mod. an ugly hack to prevent the AI from signing a peace deal when it really doesn't want to.
	tradeData = TradeData()
	tradeData.ItemType = TradeableItems.TRADE_PEACE_TREATY

	if player.getTradeDenial(kTriggeredData.eOtherPlayer, tradeData) != DenialTypes.NO_DENIAL:
		return false

	if destPlayer.getTradeDenial(kTriggeredData.ePlayer, tradeData) != DenialTypes.NO_DENIAL:
		return false
	# K-Mod end

	return true

def doGreatMediator2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	destPlayer = gc.getPlayer(kTriggeredData.eOtherPlayer)
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	if destPlayer.isHuman():
		# this works only because it's a single-player only event
		popupInfo = CyPopupInfo()
		popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON)
		popupInfo.setText(localText.getText("TXT_KEY_EVENT_GREAT_MEDIATOR_OTHER_3", (player.getCivilizationAdjectiveKey(), )))
		popupInfo.setData1(kTriggeredData.eOtherPlayer)
		popupInfo.setData2(kTriggeredData.ePlayer)
		popupInfo.setPythonModule("CvRandomEventInterface")
		popupInfo.setOnClickedPythonCallback("greatMediator2Callback")
		popupInfo.addPythonButton(localText.getText("TXT_KEY_POPUP_YES", ()), "")
		popupInfo.addPythonButton(localText.getText("TXT_KEY_POPUP_NO", ()), "")
		popupInfo.addPopup(kTriggeredData.eOtherPlayer)
	else:
		# advc.106g: Let the DLL know that this is through an event
		gc.getTeam(player.getTeam()).makePeaceEvent(destPlayer.getTeam())
		destPlayer.AI_changeAttitudeExtra(kTriggeredData.ePlayer, 1)
		player.AI_changeAttitudeExtra(kTriggeredData.eOtherPlayer, 1)

	return

def greatMediator2Callback(argsList):
	iButton = argsList[0]
	iData1 = argsList[1]
	iData2 = argsList[2]
	iData3 = argsList[3]
	szText = argsList[4]
	bOption1 = argsList[5]
	bOption2 = argsList[6]
	
	if iButton == 0:
		destPlayer = gc.getPlayer(iData1)
		player = gc.getPlayer(iData2)
		gc.getTeam(destPlayer.getTeam()).makePeace(player.getTeam())
		destPlayer.AI_changeAttitudeExtra(iData2, 1)
		player.AI_changeAttitudeExtra(iData1, 1)		

	return 0
	
def getHelpGreatMediator2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	destPlayer = gc.getPlayer(kTriggeredData.eOtherPlayer)

	# Get help text
	szHelp = localText.getText("TXT_KEY_EVENT_ATTITUDE_GOOD", (1, destPlayer.getNameKey()));

	return szHelp

######## ANCIENT_TEXTS ###########

def doAncientTexts2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	for iPlayer in range(gc.getMAX_CIV_PLAYERS()):			
		loopPlayer = gc.getPlayer(iPlayer)
		if loopPlayer.isAlive() and iPlayer != kTriggeredData.ePlayer:
			loopTeam = gc.getTeam(loopPlayer.getTeam())
			if loopTeam.isHasMet(gc.getPlayer(kTriggeredData.ePlayer).getTeam()):
				loopPlayer.AI_changeAttitudeExtra(kTriggeredData.ePlayer, 1)

	return

def getHelpAncientTexts2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	szHelp = localText.getText("TXT_KEY_EVENT_SOLO_FLIGHT_HELP_1", (1, ))	

	return szHelp


######## IMPACT_CRATER ###########

def canTriggerImpactCrater(argsList):

	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iUranium = CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_URANIUM')
	if player.getNumAvailableBonuses(iUranium) > 0:
		return false
	
	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	if not plot.canHaveBonus(iUranium, false):
		return false
	
	return true

def doImpactCrater2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	
	if not plot.isNone():
		plot.setImprovementType(CvUtil.findInfoTypeNum(gc.getImprovementInfo,gc.getNumImprovementInfos(),'IMPROVEMENT_MINE'))
	
	return 1

def getHelpImpactCrater2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	iMine = CvUtil.findInfoTypeNum(gc.getImprovementInfo,gc.getNumImprovementInfos(),'IMPROVEMENT_MINE')
	szHelp = localText.getText("TXT_KEY_EVENT_IMPROVEMENT_GROWTH", ( gc.getImprovementInfo(iMine).getTextKey(), ))

	return szHelp


# <advc.311>
def numUnitsUprising(playerId):
	# BtS code (deleted at the call locations):
	#map = gc.getMap()
	#if map.getWorldSize() == CvUtil.findInfoTypeNum(gc.getWorldInfo, gc.getNumWorldInfos(), 'WORLDSIZE_DUEL'):
	#	iNumUnits  = 1
	#elif map.getWorldSize() == CvUtil.findInfoTypeNum(gc.getWorldInfo, gc.getNumWorldInfos(), 'WORLDSIZE_TINY'):
	#	iNumUnits  = 2
	#elif map.getWorldSize() == CvUtil.findInfoTypeNum(gc.getWorldInfo, gc.getNumWorldInfos(), 'WORLDSIZE_SMALL'):
	#	iNumUnits  = 3
	#elif map.getWorldSize() == CvUtil.findInfoTypeNum(gc.getWorldInfo, gc.getNumWorldInfos(), 'WORLDSIZE_STANDARD'):
	#	iNumUnits  = 4
	#elif map.getWorldSize() == CvUtil.findInfoTypeNum(gc.getWorldInfo, gc.getNumWorldInfos(), 'WORLDSIZE_LARGE'):
	#	iNumUnits  = 5
	#else: 
	#	iNumUnits  = 6

	#r = 2;
	#for i in range(gc.getPlayer(playerId).getCurrentEra() + 1):
	#	r += i
	#return r
	# Next attempt (5 in the Medieval era seems one too many after all)
	return 2 + gc.getPlayer(playerId).getCurrentEra()

# Based (loosely) on BtS code deleted from the five canTriggerThe... functions
def hasAnyCivTeamTech(techTag):
	techId = gc.getInfoTypeForString(techTag)
	if techId < 0:
		return False
	for i in range(gc.getMAX_CIV_TEAMS()):
		t = gc.getTeam(i)
		if t.isAlive() and t.isHasTech(techId):
			return True
	return False

def isUprisingPlot(p, playerId):
	# Cut from the uprising canTrigger and apply functions (was repeated there 10 times)
	#return (not p.isOwned() and not p.isWater() and not p.isImpassable() and p.area().getCitiesPerPlayer(playerId) > 0 and p.isAdjacentPlayer(playerId, True))
	if p.isWater(): # Redundant, but may save time.
		return False
	# Let's not take Barbarian territory off limits
	if p.isOwned() and (not p.isBarbarian() or p.isCity()):
		return False
	# As above:
	if p.isImpassable() or p.area().getCitiesPerPlayer(playerId) <= 0:
		return False
	# Additional checks:
	if p.isUnit() or p.area().isBorderObstacle(gc.getPlayer(playerId).getTeam()):
		return False
	valid = False
	for dx in range(-1, 2):
		for dy in range(-1, 2):
			if dx == 0 and dy == 0:
				continue
			adj = plotXY(p.getX(), p.getY(), dx, dy)
			if not adj:
				continue
			# Adjacent is too close
			if adj.isOwned() and not adj.isBarbarian():
				return False
			# Two plots away is what I want
			if adj.isAdjacentPlayer(playerId, True):
				valid = True
	return valid

# Based on BtS code deleted from the five canTriggerThe... functions
def canTriggerUprising(playerId, techReqTag1, techReqTag2, counterUnitClassTag):
	player = gc.getPlayer(playerId)

	if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_NO_BARBARIANS):
		return False
	if (techReqTag1 and not hasAnyCivTeamTech(techReqTag1)) or (techReqTag2 and not hasAnyCivTeamTech(techReqTag2)):
		return False
	if counterUnitClassTag:
		counterClassId = gc.getInfoTypeForString(counterUnitClassTag)
		if counterClassId < 0:
			return False
		counterUnitId = gc.getCivilizationInfo(player.getCivilizationType()).getCivilizationUnits(counterClassId)
		if counterUnitId < 0:
			return False
		canTrain = False
		(c, iter) = player.firstCity(False)
		while c:
			if c.canTrain(counterUnitId, False, False):
				canTrain = True
				break
			(c, iter) = player.nextCity(iter, False)
		if not canTrain:
			return False
	map = gc.getMap()	
	for i in range(map.numPlots()):
		if isUprisingPlot(map.plotByIndex(i), playerId):
			return True
	return False
# </advc.311>


######## THE_HUNS ###########

def canTriggerTheHuns(argsList):
	# advc.311: Moved into new function
	return canTriggerUprising(argsList[0].ePlayer, 'TECH_HORSEBACK_RIDING', 'TECH_IRON_WORKING', 'UNITCLASS_SPEARMAN')

def getHelpTheHuns1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	szHelp = localText.getText("TXT_KEY_EVENT_THE_HUNS_HELP_1", ())
	return szHelp

def applyTheHuns1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	listPlots = []
	map = gc.getMap()	
	for i in range(map.numPlots()):
		if isUprisingPlot(map.plotByIndex(i), kTriggeredData.ePlayer): # advc.311
			listPlots.append(i)
	if 0 == len(listPlots):
		return
	plot = map.plotByIndex(listPlots[gc.getGame().getSorenRandNum(len(listPlots), "Hun event location")])
	iNumUnits = numUnitsUprising(kTriggeredData.ePlayer) # advc.311
	iUnitType = CvUtil.findInfoTypeNum(gc.getUnitInfo, gc.getNumUnitInfos(), 'UNIT_HORSE_ARCHER')
	barbPlayer = gc.getPlayer(gc.getBARBARIAN_PLAYER())
	for i in range(iNumUnits):
		barbPlayer.initUnit(iUnitType, plot.getX(), plot.getY(), UnitAITypes.UNITAI_ATTACK_CITY_LEMMING, DirectionTypes.DIRECTION_SOUTH)

######## THE_VANDALS ###########

def canTriggerTheVandals(argsList):
	# advc.311: Moved into new function
	return canTriggerUprising(argsList[0].ePlayer, 'TECH_METAL_CASTING', 'TECH_IRON_WORKING', 'UNITCLASS_AXEMAN')

def getHelpTheVandals1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	szHelp = localText.getText("TXT_KEY_EVENT_THE_VANDALS_HELP_1", ())
	return szHelp

def applyTheVandals1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	listPlots = []
	map = gc.getMap()	
	for i in range(map.numPlots()):
		if isUprisingPlot(map.plotByIndex(i), kTriggeredData.ePlayer): # advc.311
			listPlots.append(i)
	if 0 == len(listPlots):
		return
	plot = map.plotByIndex(listPlots[gc.getGame().getSorenRandNum(len(listPlots), "Vandal event location")])
	iNumUnits = numUnitsUprising(kTriggeredData.ePlayer) # advc.311
	iUnitType = CvUtil.findInfoTypeNum(gc.getUnitInfo, gc.getNumUnitInfos(), 'UNIT_SWORDSMAN')
	barbPlayer = gc.getPlayer(gc.getBARBARIAN_PLAYER())
	for i in range(iNumUnits):
		barbPlayer.initUnit(iUnitType, plot.getX(), plot.getY(), UnitAITypes.UNITAI_ATTACK_CITY_LEMMING, DirectionTypes.DIRECTION_SOUTH)

######## THE_GOTHS ###########

def canTriggerTheGoths(argsList):
	# advc.311: Moved into new function
	return canTriggerUprising(argsList[0].ePlayer, 'TECH_MATHEMATICS', 'TECH_IRON_WORKING', 'UNITCLASS_CHARIOT')

def getHelpTheGoths1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	szHelp = localText.getText("TXT_KEY_EVENT_THE_GOTHS_HELP_1", ())
	return szHelp

def applyTheGoths1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	listPlots = []
	map = gc.getMap()	
	for i in range(map.numPlots()):
		if isUprisingPlot(map.plotByIndex(i), kTriggeredData.ePlayer): # advc.311
			listPlots.append(i)
	if 0 == len(listPlots):
		return
	plot = map.plotByIndex(listPlots[gc.getGame().getSorenRandNum(len(listPlots), "Goth event location")])
	iNumUnits = numUnitsUprising(kTriggeredData.ePlayer) # advc.311
	iUnitType = CvUtil.findInfoTypeNum(gc.getUnitInfo, gc.getNumUnitInfos(), 'UNIT_AXEMAN')
	barbPlayer = gc.getPlayer(gc.getBARBARIAN_PLAYER())
	for i in range(iNumUnits):
		barbPlayer.initUnit(iUnitType, plot.getX(), plot.getY(), UnitAITypes.UNITAI_ATTACK_CITY_LEMMING, DirectionTypes.DIRECTION_SOUTH)

######## THE_PHILISTINES ###########

def canTriggerThePhilistines(argsList):
	# advc.311: Moved into new function
	return canTriggerUprising(argsList[0].ePlayer, 'TECH_MONOTHEISM', 'TECH_BRONZE_WORKING', 'UNITCLASS_AXEMAN')

def getHelpThePhilistines1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	szHelp = localText.getText("TXT_KEY_EVENT_THE_PHILISTINES_HELP_1", ())
	return szHelp

def applyThePhilistines1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	listPlots = []
	map = gc.getMap()	
	for i in range(map.numPlots()):
		if isUprisingPlot(map.plotByIndex(i), kTriggeredData.ePlayer): # advc.311
			listPlots.append(i)
	if 0 == len(listPlots):
		return
	plot = map.plotByIndex(listPlots[gc.getGame().getSorenRandNum(len(listPlots), "Philistine event location")])
	iNumUnits = numUnitsUprising(kTriggeredData.ePlayer) # advc.311
	iUnitType = CvUtil.findInfoTypeNum(gc.getUnitInfo, gc.getNumUnitInfos(), 'UNIT_SPEARMAN')
	barbPlayer = gc.getPlayer(gc.getBARBARIAN_PLAYER())
	for i in range(iNumUnits):
		barbPlayer.initUnit(iUnitType, plot.getX(), plot.getY(), UnitAITypes.UNITAI_ATTACK_CITY_LEMMING, DirectionTypes.DIRECTION_SOUTH)

######## THE_VEDIC_ARYANS ###########

def canTriggerTheVedicAryans(argsList):
	# advc.311: Moved into new function; replace Polytheism with Priesthood (from BUFFY 3.19.003).
	return canTriggerUprising(argsList[0].ePlayer, 'TECH_PRIESTHOOD', 'TECH_ARCHERY', 'UNITCLASS_ARCHER')

def getHelpTheVedicAryans1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	szHelp = localText.getText("TXT_KEY_EVENT_THE_VEDIC_ARYANS_HELP_1", ())
	return szHelp

def applyTheVedicAryans1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	listPlots = []
	map = gc.getMap()	
	for i in range(map.numPlots()):
		if isUprisingPlot(map.plotByIndex(i), kTriggeredData.ePlayer): # advc.311
			listPlots.append(i)
	if 0 == len(listPlots):
		return
	plot = map.plotByIndex(listPlots[gc.getGame().getSorenRandNum(len(listPlots), "Vedic Aryan event location")])
	iNumUnits = numUnitsUprising(kTriggeredData.ePlayer) # advc.311
	iUnitType = CvUtil.findInfoTypeNum(gc.getUnitInfo, gc.getNumUnitInfos(), 'UNIT_ARCHER')
	barbPlayer = gc.getPlayer(gc.getBARBARIAN_PLAYER())
	for i in range(iNumUnits):
		barbPlayer.initUnit(iUnitType, plot.getX(), plot.getY(), UnitAITypes.UNITAI_ATTACK_CITY_LEMMING, DirectionTypes.DIRECTION_SOUTH)

######## SECURITY_TAX ###########

def canTriggerSecurityTax(argsList):

	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iWalls = CvUtil.findInfoTypeNum(gc.getBuildingClassInfo, gc.getNumBuildingClassInfos(), 'BUILDINGCLASS_WALLS')
	if player.getNumCities() > player.getBuildingClassCount(iWalls):
		return false
	
	return true


######## LITERACY ###########

def canTriggerLiteracy(argsList):

	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iLibrary = CvUtil.findInfoTypeNum(gc.getBuildingClassInfo, gc.getNumBuildingClassInfos(), 'BUILDINGCLASS_LIBRARY')
	if player.getNumCities() > player.getBuildingClassCount(iLibrary):
		return false
	
	return true

######## TEA ###########

def canTriggerTea(argsList):

	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	if player.isCivic(CvUtil.findInfoTypeNum(gc.getCivicInfo,gc.getNumCivicInfos(),'CIVIC_MERCANTILISM')):
		return false

	bCanTrade = false		
	for iLoopPlayer in range(gc.getMAX_CIV_PLAYERS()):
		if player.canHaveTradeRoutesWith(iLoopPlayer):
			bCanTrade = true	
			break
			
	if not bCanTrade:
		return false
	
	return true

######## HORSE WHISPERING ###########

def canTriggerHorseWhispering(argsList):
	kTriggeredData = argsList[0]

	if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_ONE_CITY_CHALLENGE) and gc.getPlayer(kTriggeredData.ePlayer).isHuman():
		return false
	
	return true

def getHelpHorseWhispering1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	map = gc.getMap()
	
	iNumStables = worldSizeTarget()
	szHelp = localText.getText("TXT_KEY_EVENT_HORSE_WHISPERING_HELP", (iNumStables, ))

	return szHelp

def canTriggerHorseWhisperingDone(argsList):
	kTriggeredData = argsList[0]
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iStable = CvUtil.findInfoTypeNum(gc.getBuildingClassInfo, gc.getNumBuildingClassInfos(), 'BUILDINGCLASS_STABLE')
	if worldSizeTarget() > player.getBuildingClassCount(iStable):
		return false
	
	return true

def getHelpHorseWhisperingDone1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	map = gc.getMap()
	
	iNumUnits = worldSizeTarget()
	szHelp = localText.getText("TXT_KEY_EVENT_HORSE_WHISPERING_DONE_HELP_1", (iNumUnits, ))

	return szHelp

def applyHorseWhisperingDone1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	map = gc.getMap()
	plot = map.plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	iNumUnits = worldSizeTarget()
	iUnitClassType = CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_HORSE_ARCHER')
	iUnitType = gc.getCivilizationInfo(player.getCivilizationType()).getCivilizationUnits(iUnitClassType)
	
	if iUnitType != -1:
		for i in range(iNumUnits):
			player.initUnit(iUnitType, plot.getX(), plot.getY(), UnitAITypes.UNITAI_ATTACK, DirectionTypes.DIRECTION_SOUTH)

######## HARBORMASTER ###########

def getHelpHarbormaster1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
		
	iHarborsRequired = worldSizeTarget()
	iCaravelsRequired = iHarborsRequired / 2 + 1

	szHelp = localText.getText("TXT_KEY_EVENT_HARBORMASTER_HELP", (iHarborsRequired, iCaravelsRequired))

	return szHelp


def canTriggerHarbormaster(argsList):
	kTriggeredData = argsList[0]
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	player = gc.getPlayer(kTriggeredData.ePlayer)

	if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_ONE_CITY_CHALLENGE) and gc.getPlayer(kTriggeredData.ePlayer).isHuman():
		return false

	map = gc.getMap()

	iNumWater = 0
	
	for i in range(map.numPlots()):
		plot = map.plotByIndex(i)
		
		if plot.isWater():
			iNumWater += 1
			
		if 100 * iNumWater >= 40 * map.numPlots():
			return true
		
	return false

def canTriggerHarbormasterDone(argsList):
	kTriggeredData = argsList[0]
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iHarbor = CvUtil.findInfoTypeNum(gc.getBuildingClassInfo, gc.getNumBuildingClassInfos(), 'BUILDINGCLASS_HARBOR')
	iHarborsRequired = worldSizeTarget()
	if iHarborsRequired > player.getBuildingClassCount(iHarbor):
		return false

	iCaravel = CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_CARAVEL')
	iCaravelsRequired = iHarborsRequired / 2 + 1
	if iCaravelsRequired > player.getUnitClassCount(iCaravel):
		return false
	
	return true
	
######## CLASSIC LITERATURE ###########

def canTriggerClassicLiterature(argsList):
	kTriggeredData = argsList[0]
	
	if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_ONE_CITY_CHALLENGE) and gc.getPlayer(kTriggeredData.ePlayer).isHuman():
		return false

	return true

def getHelpClassicLiterature1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
		
	iLibrariesRequired = worldSizeTarget()
	szHelp = localText.getText("TXT_KEY_EVENT_CLASSIC_LITERATURE_HELP_1", (iLibrariesRequired, ))

	return szHelp


def canTriggerClassicLiteratureDone(argsList):
	kTriggeredData = argsList[0]
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iLibrary = CvUtil.findInfoTypeNum(gc.getBuildingClassInfo, gc.getNumBuildingClassInfos(), 'BUILDINGCLASS_LIBRARY')
	iBuildingsRequired = worldSizeTarget()
	if iBuildingsRequired > player.getBuildingClassCount(iLibrary):
		return false
	
	return true

def getHelpClassicLiteratureDone2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	szHelp = localText.getText("TXT_KEY_EVENT_CLASSIC_LITERATURE_DONE_HELP_2", ( ))

	return szHelp

def canApplyClassicLiteratureDone2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iEraAncient = CvUtil.findInfoTypeNum(gc.getEraInfo, gc.getNumEraInfos(), 'ERA_ANCIENT')

	for iTech in range(gc.getNumTechInfos()):
		if gc.getTechInfo(iTech).getEra() == iEraAncient and player.canResearch(iTech, false):
			return true
			
	return false
		
def applyClassicLiteratureDone2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	iEraAncient = CvUtil.findInfoTypeNum(gc.getEraInfo, gc.getNumEraInfos(), 'ERA_ANCIENT')

	listTechs = []
	for iTech in range(gc.getNumTechInfos()):
		if gc.getTechInfo(iTech).getEra() == iEraAncient and player.canResearch(iTech, false):
			listTechs.append(iTech)
			
	if len(listTechs) > 0:
		iTech = listTechs[gc.getGame().getSorenRandNum(len(listTechs), "Classic Literature Event Tech selection")]
		gc.getTeam(player.getTeam()).setHasTech(iTech, true, kTriggeredData.ePlayer, true, true)
		
def getHelpClassicLiteratureDone3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	iSpecialist = CvUtil.findInfoTypeNum(gc.getSpecialistInfo, gc.getNumSpecialistInfos(), 'SPECIALIST_SCIENTIST', )
	iGreatLibrary = CvUtil.findInfoTypeNum(gc.getBuildingInfo, gc.getNumBuildingInfos(), 'BUILDING_GREAT_LIBRARY')

	szCityName = u""
	(loopCity, iter) = player.firstCity(false)
	while(loopCity):
		if (loopCity.isHasBuilding(iGreatLibrary)):
			szCityName = loopCity.getNameKey()
			break
				
		(loopCity, iter) = player.nextCity(iter, false)
	
	szHelp = localText.getText("TXT_KEY_EVENT_FREE_SPECIALIST", (1, gc.getSpecialistInfo(iSpecialist).getTextKey(), szCityName))

	return szHelp

def canApplyClassicLiteratureDone3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iGreatLibrary = CvUtil.findInfoTypeNum(gc.getBuildingInfo, gc.getNumBuildingInfos(), 'BUILDING_GREAT_LIBRARY')

	(loopCity, iter) = player.firstCity(false)
	while(loopCity):
		if (loopCity.isHasBuilding(iGreatLibrary)):
			return true
				
		(loopCity, iter) = player.nextCity(iter, false)
			
	return false
		
def applyClassicLiteratureDone3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	iSpecialist = CvUtil.findInfoTypeNum(gc.getSpecialistInfo, gc.getNumSpecialistInfos(), 'SPECIALIST_SCIENTIST', )
	iGreatLibrary = CvUtil.findInfoTypeNum(gc.getBuildingInfo, gc.getNumBuildingInfos(), 'BUILDING_GREAT_LIBRARY')

	(loopCity, iter) = player.firstCity(false)
	while(loopCity):
		if (loopCity.isHasBuilding(iGreatLibrary)):
			loopCity.changeFreeSpecialistCount(iSpecialist, 1)
			return
				
		(loopCity, iter) = player.nextCity(iter, false)

######## MASTER BLACKSMITH ###########

def canTriggerMasterBlacksmith(argsList):
	kTriggeredData = argsList[0]
	
	if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_ONE_CITY_CHALLENGE) and gc.getPlayer(kTriggeredData.ePlayer).isHuman():
		return false

	return true

def getHelpMasterBlacksmith1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
		
	iRequired = worldSizeTarget()
	szHelp = localText.getText("TXT_KEY_EVENT_MASTER_BLACKSMITH_HELP_1", (iRequired, player.getCity(kTriggeredData.iCityId).getNameKey()))

	return szHelp

def expireMasterBlacksmith1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	city = player.getCity(kTriggeredData.iCityId)	
	if city == None or city.getOwner() != kTriggeredData.ePlayer:
		return true
				
	return false

def canTriggerMasterBlacksmithDone(argsList):
	kTriggeredData = argsList[0]
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	player = gc.getPlayer(kTriggeredData.ePlayer)
		
	iForge = CvUtil.findInfoTypeNum(gc.getBuildingClassInfo, gc.getNumBuildingClassInfos(), 'BUILDINGCLASS_FORGE')
	iBuildingsRequired = worldSizeTarget()
	if iBuildingsRequired > player.getBuildingClassCount(iForge):
		return false

	kOrigTriggeredData = player.getEventOccured(trigger.getPrereqEvent(0))

	city = player.getCity(kOrigTriggeredData.iCityId)	
	if city == None or city.getOwner() != kTriggeredData.ePlayer:
		return false

	kActualTriggeredDataObject = player.getEventTriggered(kTriggeredData.iId)
	kActualTriggeredDataObject.iCityId = kOrigTriggeredData.iCityId
		
	return true

def canApplyMasterBlacksmithDone1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iBonus = CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_COPPER')
	city = player.getCity(kTriggeredData.iCityId)
	
	if city == None:
		return false
	
	map = gc.getMap()
	iBestValue = map.getGridWidth() + map.getGridHeight()
	bestPlot = None
	for i in range(map.numPlots()):
		plot = map.plotByIndex(i)
		if (plot.getOwner() == kTriggeredData.ePlayer and plot.canHaveBonus(iBonus, false)):
			iValue = plotDistance(city.getX(), city.getY(), plot.getX(), plot.getY())
			if iValue < iBestValue:
				iBestValue = iValue
				bestPlot = plot
				
	if bestPlot == None:
		return false

	kActualTriggeredDataObject = player.getEventTriggered(kTriggeredData.iId)
	kActualTriggeredDataObject.iPlotX = bestPlot.getX()
	kActualTriggeredDataObject.iPlotY = bestPlot.getY()
		
	return true

def applyMasterBlacksmithDone1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	city = player.getCity(kTriggeredData.iCityId)
	
	iBonus = CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_COPPER')
	plot.setBonusType(iBonus)

	szBuffer = localText.getText("TXT_KEY_MISC_DISCOVERED_NEW_RESOURCE", (gc.getBonusInfo(iBonus).getTextKey(), city.getNameKey()))
	CyInterface().addMessage(kTriggeredData.ePlayer, false, gc.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_DISCOVERBONUS", InterfaceMessageTypes.MESSAGE_TYPE_MINOR_EVENT, gc.getBonusInfo(iBonus).getButton(), gc.getInfoTypeForString("COLOR_WHITE"), plot.getX(), plot.getY(), true, true)

def canApplyMasterBlacksmithDone3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	if player.getStateReligion() == -1:		
		return false
		
	return true

######## THE BEST DEFENSE ###########

def canTriggerBestDefense(argsList):
	kTriggeredData = argsList[0]
	
	if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_ONE_CITY_CHALLENGE) and gc.getPlayer(kTriggeredData.ePlayer).isHuman():
		return false

	return true

def getHelpBestDefense1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
		
	iRequired = worldSizeTarget()
	szHelp = localText.getText("TXT_KEY_EVENT_BEST_DEFENSE_HELP_1", (iRequired, ))

	return szHelp

def canTriggerBestDefenseDone(argsList):
	kTriggeredData = argsList[0]
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	player = gc.getPlayer(kTriggeredData.ePlayer)
		
	iCastle = CvUtil.findInfoTypeNum(gc.getBuildingClassInfo, gc.getNumBuildingClassInfos(), 'BUILDINGCLASS_CASTLE')
	iBuildingsRequired = worldSizeTarget()
	if iBuildingsRequired > player.getBuildingClassCount(iCastle):
		return false
		
	return true

def getHelpBestDefenseDone2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	szHelp = localText.getText("TXT_KEY_EVENT_BEST_DEFENSE_DONE_HELP_2", (3, ))	

	return szHelp

def applyBestDefenseDone2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	for iPlayer in range(gc.getMAX_CIV_PLAYERS()):			
		loopPlayer = gc.getPlayer(iPlayer)
		if loopPlayer.isAlive() and iPlayer != kTriggeredData.ePlayer:
			loopTeam = gc.getTeam(loopPlayer.getTeam())
			if loopTeam.isHasMet(gc.getPlayer(kTriggeredData.ePlayer).getTeam()):
				loopPlayer.AI_changeAttitudeExtra(kTriggeredData.ePlayer, 3)
						

def canApplyBestDefenseDone3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iGreatWall = CvUtil.findInfoTypeNum(gc.getBuildingInfo, gc.getNumBuildingInfos(), 'BUILDING_GREAT_WALL')

	(loopCity, iter) = player.firstCity(false)
	while(loopCity):
		if (loopCity.isHasBuilding(iGreatWall)):
			return true
				
		(loopCity, iter) = player.nextCity(iter, false)
			
	return false

######## NATIONAL SPORTS LEAGUE ###########

def canTriggerSportsLeague(argsList):
	kTriggeredData = argsList[0]
	
	if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_ONE_CITY_CHALLENGE) and gc.getPlayer(kTriggeredData.ePlayer).isHuman():
		return false

	return true
def getHelpSportsLeague1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
		
	iRequired = worldSizeTarget()
	iBuilding = CvUtil.findInfoTypeNum(gc.getBuildingInfo, gc.getNumBuildingInfos(), 'BUILDING_STATUE_OF_ZEUS')
	
	szHelp = localText.getText("TXT_KEY_EVENT_SPORTS_LEAGUE_HELP_1", (iRequired, gc.getBuildingInfo(iBuilding).getTextKey()))

	return szHelp

def canTriggerSportsLeagueDone(argsList):
	kTriggeredData = argsList[0]
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	player = gc.getPlayer(kTriggeredData.ePlayer)
		
	iCastle = CvUtil.findInfoTypeNum(gc.getBuildingClassInfo, gc.getNumBuildingClassInfos(), 'BUILDINGCLASS_COLOSSEUM')
	iBuildingsRequired = worldSizeTarget()
	if iBuildingsRequired > player.getBuildingClassCount(iCastle):
		return false
		
	return true

def canApplySportsLeagueDone3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iZeus = CvUtil.findInfoTypeNum(gc.getBuildingInfo, gc.getNumBuildingInfos(), 'BUILDING_STATUE_OF_ZEUS')

	(loopCity, iter) = player.firstCity(false)
	while(loopCity):
		if (loopCity.isHasBuilding(iZeus)):
			return true
				
		(loopCity, iter) = player.nextCity(iter, false)
			
	return false

######## CRUSADE ###########

def canTriggerCrusade(argsList):
	kTriggeredData = argsList[0]
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	player = gc.getPlayer(kTriggeredData.ePlayer)
	otherPlayer = gc.getPlayer(kTriggeredData.eOtherPlayer)
	holyCity = gc.getGame().getHolyCity(kTriggeredData.eReligion)

	if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_ONE_CITY_CHALLENGE) and gc.getPlayer(kTriggeredData.ePlayer).isHuman():
		return false

	if holyCity.getOwner() != kTriggeredData.eOtherPlayer:
		return false
		
	kActualTriggeredDataObject = player.getEventTriggered(kTriggeredData.iId)
	kActualTriggeredDataObject.iOtherPlayerCityId = holyCity.getID()	
			
	return true

def getHelpCrusade1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	holyCity = gc.getGame().getHolyCity(kTriggeredData.eReligion)
	
	szHelp = localText.getText("TXT_KEY_EVENT_CRUSADE_HELP_1", (holyCity.getNameKey(), ))

	return szHelp

def expireCrusade1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	otherPlayer = gc.getPlayer(kTriggeredData.eOtherPlayer)
	holyCity = gc.getGame().getHolyCity(kTriggeredData.eReligion)

	if holyCity.getOwner() == kTriggeredData.ePlayer:
		return false

	if player.getStateReligion() != kTriggeredData.eReligion:
		return true

	if holyCity.getOwner() != kTriggeredData.eOtherPlayer:
		return true

	if not gc.getTeam(player.getTeam()).isAtWar(otherPlayer.getTeam()):
		return true	
					
	return false

def canTriggerCrusadeDone(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)

	kOrigTriggeredData = player.getEventOccured(trigger.getPrereqEvent(0))
	holyCity = gc.getGame().getHolyCity(kOrigTriggeredData.eReligion)

	if holyCity.getOwner() != kTriggeredData.ePlayer:
		return false
					
	kActualTriggeredDataObject = player.getEventTriggered(kTriggeredData.iId)
	kActualTriggeredDataObject.iCityId = holyCity.getID()
	kActualTriggeredDataObject.eOtherPlayer = kOrigTriggeredData.eOtherPlayer
	kActualTriggeredDataObject.eReligion = kOrigTriggeredData.eReligion
	
	for iBuilding in range(gc.getNumBuildingInfos()):
		if gc.getBuildingInfo(iBuilding).getHolyCity() == kOrigTriggeredData.eReligion:
			kActualTriggeredDataObject.eBuilding = BuildingTypes(iBuilding)
			break	
			
	return true

def getHelpCrusadeDone1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	holyCity = gc.getGame().getHolyCity(kTriggeredData.eReligion)
	szUnit = gc.getUnitInfo(holyCity.getConscriptUnit()).getTextKey()
	iNumUnits = worldSizeTarget() / 2 + 1
	szHelp = localText.getText("TXT_KEY_EVENT_CRUSADE_DONE_HELP_1", (iNumUnits, szUnit, holyCity.getNameKey()))	

	return szHelp

def canApplyCrusadeDone1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	holyCity = gc.getGame().getHolyCity(kTriggeredData.eReligion)
	if -1 == holyCity.getConscriptUnit():
		return false
	
	return true

def applyCrusadeDone1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	holyCity = gc.getGame().getHolyCity(kTriggeredData.eReligion)						
	iUnitType = holyCity.getConscriptUnit()
	iNumUnits = worldSizeTarget() / 2 + 1
	
	if iUnitType != -1:
		for i in range(iNumUnits):
			player.initUnit(iUnitType, holyCity.getX(), holyCity.getY(), UnitAITypes.UNITAI_CITY_DEFENSE, DirectionTypes.DIRECTION_SOUTH)

def getHelpCrusadeDone2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	holyCity = gc.getGame().getHolyCity(kTriggeredData.eReligion)
	
	szHelp = localText.getText("TXT_KEY_EVENT_CRUSADE_DONE_HELP_2", (gc.getBuildingInfo(kTriggeredData.eBuilding).getTextKey(), holyCity.getNameKey()))	

	return szHelp

def canApplyCrusadeDone2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	holyCity = gc.getGame().getHolyCity(kTriggeredData.eReligion)
	
	if -1 == kTriggeredData.eBuilding or holyCity.isHasBuilding(kTriggeredData.eBuilding):
		return false			
	
	return true

def applyCrusadeDone2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	holyCity = gc.getGame().getHolyCity(kTriggeredData.eReligion)
	holyCity.setNumRealBuilding(kTriggeredData.eBuilding, 1)
						
	if (not gc.getGame().isNetworkMultiPlayer() and kTriggeredData.ePlayer == gc.getGame().getActivePlayer()):
		popupInfo = CyPopupInfo()
		popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON_SCREEN)
		popupInfo.setData1(kTriggeredData.eBuilding)
		popupInfo.setData2(holyCity.getID())
		popupInfo.setData3(0)
		popupInfo.setText(u"showWonderMovie")
		popupInfo.addPopup(kTriggeredData.ePlayer)

def getHelpCrusadeDone3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	iNumCities = worldSizeTarget()
	holyCity = gc.getGame().getHolyCity(kTriggeredData.eReligion)
	
	szHelp = localText.getText("TXT_KEY_EVENT_CRUSADE_DONE_HELP_3", (gc.getReligionInfo(kTriggeredData.eReligion).getTextKey(), iNumCities))	

	return szHelp

def canApplyCrusadeDone3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	iNumCities = worldSizeTarget()
	if gc.getGame().getNumCities() == gc.getGame().countReligionLevels(kTriggeredData.eReligion):
		return false
		
	return true

def applyCrusadeDone3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	holyCity = gc.getGame().getHolyCity(kTriggeredData.eReligion)

	listCities = []	
	for iPlayer in range(gc.getMAX_CIV_PLAYERS()):			
		loopPlayer = gc.getPlayer(iPlayer)
		if loopPlayer.isAlive():			
			(loopCity, iter) = loopPlayer.firstCity(false)

			while(loopCity):
				if (not loopCity.isHasReligion(kTriggeredData.eReligion)):
					iDistance = plotDistance(holyCity.getX(), holyCity.getY(), loopCity.getX(), loopCity.getY())
					listCities.append((iDistance, loopCity))
						
				(loopCity, iter) = loopPlayer.nextCity(iter, false)
	
	#listCities.sort()
	listCities.sort(key=itemgetter(0)) # K-Mod. Sorting by pointers can cause OOS.
	
	iNumCities = min(worldSizeTarget(), len(listCities))
	
	for i in range(iNumCities):
		iDistance, loopCity = listCities[i]
		loopCity.setHasReligion(kTriggeredData.eReligion, true, true, true)	

######## ESTEEMEED_PLAYWRIGHT ###########

def canTriggerEsteemedPlaywright(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	# If source civ is operating this Civic, disallow the event to trigger.
	if player.isCivic(CvUtil.findInfoTypeNum(gc.getCivicInfo,gc.getNumCivicInfos(),'CIVIC_SLAVERY')):
		return false

	return true


######## SECRET_KNOWLEDGE ###########
	
def getHelpSecretKnowledge2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	szHelp = localText.getText("TXT_KEY_EVENT_YIELD_CHANGE_BUILDING", (gc.getBuildingInfo(kTriggeredData.eBuilding).getTextKey(), u"+4[ICON_CULTURE]"))	

	return szHelp

def applySecretKnowledge2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	city = player.getCity(kTriggeredData.iCityId)
	city.setBuildingCommerceChange(gc.getBuildingInfo(kTriggeredData.eBuilding).getBuildingClassType(), CommerceTypes.COMMERCE_CULTURE, 4)

######## HIGH_WARLORD ###########

def canTriggerHighWarlord(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	# If source civ is operating this Civic, disallow the event to trigger.
	if player.isCivic(CvUtil.findInfoTypeNum(gc.getCivicInfo,gc.getNumCivicInfos(),'CIVIC_EMANCIPATION')):
		return false

	return true


######## EXPERIENCED_CAPTAIN ###########

def canTriggerExperiencedCaptain(argsList):
	kTriggeredData = argsList[0]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	unit = player.getUnit(kTriggeredData.iUnitId)
	
	if unit.isNone():
		return false
		
	if unit.getExperience() < 7:
		return false

	return true

######## PARTISANS ###########

def getNumPartisanUnits(plot, iPlayer):
	# <advc.003y> Let the DLL handle this so that it's consistent with the code moved from CvEventManager.onCityRazed.
	if not plot.isCity():
		return 1 # (shouldn't happen)
	return plot.getPlotCity().getNumPartisanUnits(iPlayer)
	# </advc.003y>

def getHelpPartisans1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	capital = player.getCapitalCity()
	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	
	if None != capital and not capital.isNone():
		iNumUnits = getNumPartisanUnits(plot, kTriggeredData.ePlayer)
		szUnit = gc.getUnitInfo(capital.getConscriptUnit()).getTextKey()
		
		szHelp = localText.getText("TXT_KEY_EVENT_PARTISANS_HELP_1", (iNumUnits, szUnit))	

	return szHelp

def canApplyPartisans1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)

	if getNumPartisanUnits(plot, kTriggeredData.ePlayer) <= 0:
		return false

	for i in range(3):
		for j in range(3):
			loopPlot = gc.getMap().plot(kTriggeredData.iPlotX + i - 1, kTriggeredData.iPlotY + j - 1)
			if None != loopPlot and not loopPlot.isNone():
				if not (loopPlot.isVisibleEnemyUnit(kTriggeredData.ePlayer) or loopPlot.isWater() or loopPlot.isImpassable() or loopPlot.isCity()):
					return true
	return false
	

def applyPartisans1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)	
	capital = player.getCapitalCity()
	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	
	if None != capital and not capital.isNone():
		iNumUnits = getNumPartisanUnits(plot, kTriggeredData.ePlayer)

		listPlots = []
		for i in range(3):
			for j in range(3):
				loopPlot = gc.getMap().plot(kTriggeredData.iPlotX + i - 1, kTriggeredData.iPlotY + j - 1)
				if None != loopPlot and not loopPlot.isNone() and (i != 1 or j != 1):
					if not (loopPlot.isVisibleEnemyUnit(kTriggeredData.ePlayer) or loopPlot.isWater() or loopPlot.isImpassable()):
						listPlots.append(loopPlot)
		
		if len(listPlots) > 0:
			for i in range(iNumUnits):
				iPlot = gc.getGame().getSorenRandNum(len(listPlots), "Partisan event placement")
				player.initUnit(capital.getConscriptUnit(), listPlots[iPlot].getX(), listPlots[iPlot].getY(), UnitAITypes.UNITAI_ATTACK, DirectionTypes.DIRECTION_SOUTH)

def getHelpPartisans2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	capital = player.getCapitalCity()
	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	
	if None != capital and not capital.isNone():
		iNumUnits = max(1, getNumPartisanUnits(plot, kTriggeredData.ePlayer) / 2)
		szUnit = gc.getUnitInfo(capital.getConscriptUnit()).getTextKey()
		
		szHelp = localText.getText("TXT_KEY_EVENT_PARTISANS_HELP_2", (iNumUnits, szUnit, capital.getNameKey()))	

	return szHelp

def canApplyPartisans2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	
	return (max(1, getNumPartisanUnits(plot, kTriggeredData.ePlayer) / 2) > 0)
	
def applyPartisans2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)	
	capital = player.getCapitalCity()
	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)

	if None != capital and not capital.isNone():
		iNumUnits = max(1, getNumPartisanUnits(plot, kTriggeredData.ePlayer) / 2)
		for i in range(iNumUnits):
			player.initUnit(capital.getConscriptUnit(), capital.getX(), capital.getY(), UnitAITypes.UNITAI_ATTACK, DirectionTypes.DIRECTION_SOUTH)

######## GREED ###########

def canTriggerGreed(argsList):
	kTriggeredData = argsList[0]
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	player = gc.getPlayer(kTriggeredData.ePlayer)
	otherPlayer = gc.getPlayer(kTriggeredData.eOtherPlayer)
	
	if not gc.getTeam(player.getTeam()).canChangeWarPeace(otherPlayer.getTeam()):
		return false

	listBonuses = []
	iOil = CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_OIL')
	if 0 == player.getNumAvailableBonuses(iOil):
		listBonuses.append(iOil)
	iIron = CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_IRON')
	if 0 == player.getNumAvailableBonuses(iIron):
		listBonuses.append(iIron)
	iHorse = CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_HORSE')
	if 0 == player.getNumAvailableBonuses(iHorse):
		listBonuses.append(iHorse)
	iCopper = CvUtil.findInfoTypeNum(gc.getBonusInfo,gc.getNumBonusInfos(),'BONUS_COPPER')
	if 0 == player.getNumAvailableBonuses(iCopper):
		listBonuses.append(iCopper)

	map = gc.getMap()
	bFound = false
	listPlots = []
	for iBonus in listBonuses:
		for i in range(map.numPlots()):
			loopPlot = map.plotByIndex(i)
			if loopPlot.getOwner() == kTriggeredData.eOtherPlayer and loopPlot.getBonusType(player.getTeam()) == iBonus and loopPlot.isRevealed(player.getTeam(), false) and not loopPlot.isWater():
				listPlots.append(loopPlot)
				bFound = true
		if bFound:
			break
			
	if not bFound:
		return false

	plot = listPlots[gc.getGame().getSorenRandNum(len(listPlots), "Greed event plot selection")]
	
	if -1 == getGreedUnit(player, plot):
		return false
	
	kActualTriggeredDataObject = player.getEventTriggered(kTriggeredData.iId)
	kActualTriggeredDataObject.iPlotX = plot.getX()
	kActualTriggeredDataObject.iPlotY = plot.getY()
				
	return true

def getHelpGreed1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	otherPlayer = gc.getPlayer(kTriggeredData.eOtherPlayer)
	iBonus = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY).getBonusType(player.getTeam())
	
	iTurns = gc.getGameSpeedInfo(gc.getGame().getGameSpeedType()).getGrowthPercent()
			
	szHelp = localText.getText("TXT_KEY_EVENT_GREED_HELP_1", (otherPlayer.getCivilizationShortDescriptionKey(), gc.getBonusInfo(iBonus).getTextKey(), iTurns))

	return szHelp

def expireGreed1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	otherPlayer = gc.getPlayer(kTriggeredData.eOtherPlayer)
	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)

	if plot.getOwner() == kTriggeredData.ePlayer or plot.getOwner() == -1:
		return false
	
	if gc.getGame().getGameTurn() >= kTriggeredData.iTurn + gc.getGameSpeedInfo(gc.getGame().getGameSpeedType()).getGrowthPercent():
		return true
	
	if plot.getOwner() != kTriggeredData.eOtherPlayer:
		return true
				
	return false

def canTriggerGreedDone(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	kOrigTriggeredData = player.getEventOccured(trigger.getPrereqEvent(0))
	plot = gc.getMap().plot(kOrigTriggeredData.iPlotX, kOrigTriggeredData.iPlotY)

	if plot.getOwner() != kOrigTriggeredData.ePlayer:
		return false
		
	if -1 == getGreedUnit(player, plot):
		return false

	kActualTriggeredDataObject = player.getEventTriggered(kTriggeredData.iId)
	kActualTriggeredDataObject.iPlotX = kOrigTriggeredData.iPlotX
	kActualTriggeredDataObject.iPlotY = kOrigTriggeredData.iPlotY
	kActualTriggeredDataObject.eOtherPlayer = kOrigTriggeredData.eOtherPlayer
	
	return true

def getGreedUnit(player, plot):
	iBonus = plot.getBonusType(player.getTeam())
	iBestValue = 0
	iBestUnit = -1
	for iUnitClass in range(gc.getNumUnitClassInfos()):
		iUnit = gc.getCivilizationInfo(player.getCivilizationType()).getCivilizationUnits(iUnitClass)
		if -1 != iUnit and player.canTrain(iUnit, false, false) and (gc.getUnitInfo(iUnit).getDomainType() == DomainTypes.DOMAIN_LAND):
			iValue = 0
			if gc.getUnitInfo(iUnit).getPrereqAndBonus() == iBonus:
				iValue = player.AI_unitValue(iUnit, UnitAITypes.UNITAI_ATTACK, plot.area())
			else:
				for j in range(gc.getNUM_UNIT_PREREQ_OR_BONUSES()):
					if (gc.getUnitInfo(iUnit).getPrereqOrBonuses(j) == iBonus):
						iValue = player.AI_unitValue(iUnit, UnitAITypes.UNITAI_ATTACK, plot.area())
						break
			if iValue > iBestValue:
				iBestValue = iValue
				iBestUnit = iUnit
				
	return iBestUnit
	

def getHelpGreedDone1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
	
	iNumUnits = worldSizeTarget() / 2 + 1
	iUnitType = getGreedUnit(player, plot)
	
	if iUnitType != -1:	
		szHelp = localText.getText("TXT_KEY_EVENT_GREED_DONE_HELP_1", (iNumUnits, gc.getUnitInfo(iUnitType).getTextKey()))	

	return szHelp

def applyGreedDone1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	plot = gc.getMap().plot(kTriggeredData.iPlotX, kTriggeredData.iPlotY)
		
	iUnitType = getGreedUnit(player, plot)
	iNumUnits = worldSizeTarget() / 2 + 1
	
	if iUnitType != -1:
		for i in range(iNumUnits):
			player.initUnit(iUnitType, plot.getX(), plot.getY(), UnitAITypes.UNITAI_ATTACK, DirectionTypes.DIRECTION_SOUTH)


######## WAR CHARIOTS ###########

def canTriggerWarChariots(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	kActualTriggeredDataObject = player.getEventTriggered(kTriggeredData.iId)
	kActualTriggeredDataObject.eReligion = ReligionTypes(player.getStateReligion())
	
	return true

def getHelpWarChariots1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
		
	iNumUnits = worldSizeTarget() + 1
	szHelp = localText.getText("TXT_KEY_EVENT_WAR_CHARIOTS_HELP_1", (iNumUnits, ))

	return szHelp

def canTriggerWarChariotsDone(argsList):
	kTriggeredData = argsList[0]
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	player = gc.getPlayer(kTriggeredData.ePlayer)
	iNumUnits = worldSizeTarget() + 1
	iUnitClassType = CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_CHARIOT')
	if player.getUnitClassCount(iUnitClassType) < iNumUnits:
		return false
	
	kOrigTriggeredData = player.getEventOccured(trigger.getPrereqEvent(0))
	kActualTriggeredDataObject = player.getEventTriggered(kTriggeredData.iId)
	kActualTriggeredDataObject.eReligion = kOrigTriggeredData.eReligion
		
	return true

######## ELITE SWORDSMEN ###########

def getHelpEliteSwords1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
		
	iNumUnits = worldSizeTarget() + 1
	szHelp = localText.getText("TXT_KEY_EVENT_ELITE_SWORDS_HELP_1", (iNumUnits, ))

	return szHelp

def canTriggerEliteSwordsDone(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iNumUnits = worldSizeTarget() + 1
	iUnitClassType = CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_SWORDSMAN')
	if player.getUnitClassCount(iUnitClassType) < iNumUnits:
		return false
			
	return true


def canApplyEliteSwordsDone2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
		
	iCivic = CvUtil.findInfoTypeNum(gc.getCivicInfo,gc.getNumCivicInfos(),'CIVIC_HEREDITARY_RULE')
	
	if not player.isCivic(iCivic):
		return false
	
	return true	

######## WARSHIPS ###########

def canTriggerWarships(argsList):
	kTriggeredData = argsList[0]
	
	map = gc.getMap()
	iNumWater = 0
	
	for i in range(map.numPlots()):
		plot = map.plotByIndex(i)
		
		if plot.isWater():
			iNumWater += 1
			
		if 100 * iNumWater >= 55 * map.numPlots():
			return true
			
	return false

def getHelpWarships1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
		
	iNumUnits = worldSizeTarget()
	iBuilding = CvUtil.findInfoTypeNum(gc.getBuildingInfo, gc.getNumBuildingInfos(), 'BUILDING_GREAT_LIGHTHOUSE')
	szHelp = localText.getText("TXT_KEY_EVENT_WARSHIPS_HELP_1", (iNumUnits, gc.getBuildingInfo(iBuilding).getTextKey()))

	return szHelp

def canTriggerWarshipsDone(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iNumUnits = worldSizeTarget()
	iUnitClassType = CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_TRIREME')

	if player.getUnitClassCount(iUnitClassType) < iNumUnits:
		return false
			
	return true


def canApplyWarshipsDone2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
		
	iBuilding = CvUtil.findInfoTypeNum(gc.getBuildingInfo, gc.getNumBuildingInfos(), 'BUILDING_GREAT_LIGHTHOUSE')
	if player.getBuildingClassCount(gc.getBuildingInfo(iBuilding).getBuildingClassType()) == 0:
		return false

	return true	

######## GUNS NOT BUTTER ###########

def getHelpGunsButter1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
		
	iNumUnits = worldSizeTarget() + 1
	iBuilding = CvUtil.findInfoTypeNum(gc.getBuildingInfo, gc.getNumBuildingInfos(), 'BUILDING_TAJ_MAHAL')
	
	szHelp = localText.getText("TXT_KEY_EVENT_GUNS_BUTTER_HELP_1", (iNumUnits, gc.getBuildingInfo(iBuilding).getTextKey()))

	return szHelp

def canTriggerGunsButterDone(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iNumUnits = worldSizeTarget() + 1
	iUnitClassType = CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_MUSKETMAN')

	if player.getUnitClassCount(iUnitClassType) < iNumUnits:
		return false
			
	return true


def canApplyGunsButterDone2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
		
	iCivic = CvUtil.findInfoTypeNum(gc.getCivicInfo,gc.getNumCivicInfos(),'CIVIC_VASSALAGE')
	
	if not player.isCivic(iCivic):
		return false
	
	return true	

def canApplyGunsButterDone3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
		
	iBuilding = CvUtil.findInfoTypeNum(gc.getBuildingInfo, gc.getNumBuildingInfos(), 'BUILDING_TAJ_MAHAL')
	if player.getBuildingClassCount(gc.getBuildingInfo(iBuilding).getBuildingClassType()) == 0:
		return false

	return true	

######## NOBLE KNIGHTS ###########

def canTriggerNobleKnights(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	kActualTriggeredDataObject = player.getEventTriggered(kTriggeredData.iId)
	kActualTriggeredDataObject.eReligion = ReligionTypes(player.getStateReligion())
	
	return true

def getHelpNobleKnights1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
		
	iNumUnits = worldSizeTarget() + 1
	iBuilding = CvUtil.findInfoTypeNum(gc.getBuildingInfo, gc.getNumBuildingInfos(), 'BUILDING_ORACLE')
	
	szHelp = localText.getText("TXT_KEY_EVENT_NOBLE_KNIGHTS_HELP_1", (iNumUnits, gc.getBuildingInfo(iBuilding).getTextKey()))

	return szHelp

def canTriggerNobleKnightsDone(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iNumUnits = worldSizeTarget() + 1
	iUnitClassType = CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_KNIGHT')

	if player.getUnitClassCount(iUnitClassType) < iNumUnits:
		return false
			
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	kOrigTriggeredData = player.getEventOccured(trigger.getPrereqEvent(0))
	kActualTriggeredDataObject = player.getEventTriggered(kTriggeredData.iId)
	kActualTriggeredDataObject.eReligion = kOrigTriggeredData.eReligion

	iBuilding = CvUtil.findInfoTypeNum(gc.getBuildingInfo, gc.getNumBuildingInfos(), 'BUILDING_ORACLE')
	
	(loopCity, iter) = player.firstCity(false)
	while(loopCity):
		if (loopCity.isHasBuilding(iBuilding)):
			kActualTriggeredDataObject.iPlotX = loopCity.getX()
			kActualTriggeredDataObject.iPlotY = loopCity.getY()
			kActualTriggeredDataObject.iCityId = loopCity.getID()
			break
				
		(loopCity, iter) = player.nextCity(iter, false)

	return true

def canApplyNobleKnightsDone2(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
		
	iCivic = CvUtil.findInfoTypeNum(gc.getCivicInfo,gc.getNumCivicInfos(),'CIVIC_ORGANIZED_RELIGION')
	
	if not player.isCivic(iCivic):
		return false
	
	return true	

######## OVERWHELM DOCTRINE ###########

def canTriggerOverwhelm(argsList):
	kTriggeredData = argsList[0]
	
	map = gc.getMap()
	iNumWater = 0
	
	for i in range(map.numPlots()):
		plot = map.plotByIndex(i)
		if plot.isWater():
			iNumWater += 1
		if 100 * iNumWater >= 55 * map.numPlots():
			return true			
	return false

def getHelpOverwhelm1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	iDestroyer = CvUtil.findInfoTypeNum(gc.getUnitInfo, gc.getNumUnitInfos(), 'UNIT_DESTROYER')
	iNumDestroyers = 4
	iBattleship = CvUtil.findInfoTypeNum(gc.getUnitInfo, gc.getNumUnitInfos(), 'UNIT_BATTLESHIP')
	iNumBattleships = 2
	iCarrier = CvUtil.findInfoTypeNum(gc.getUnitInfo, gc.getNumUnitInfos(), 'UNIT_CARRIER')
	iNumCarriers = 3
	iFighter = CvUtil.findInfoTypeNum(gc.getSpecialUnitInfo, gc.getNumSpecialUnitInfos(), 'SPECIALUNIT_FIGHTER')
	iNumFighters = 9
	iProject = CvUtil.findInfoTypeNum(gc.getProjectInfo, gc.getNumProjectInfos(), 'PROJECT_MANHATTAN_PROJECT')
			
	szHelp = localText.getText("TXT_KEY_EVENT_OVERWHELM_HELP_1", (iNumDestroyers, gc.getUnitInfo(iDestroyer).getTextKey(), iNumBattleships, gc.getUnitInfo(iBattleship).getTextKey(), iNumCarriers, gc.getUnitInfo(iCarrier).getTextKey(), iNumFighters, gc.getSpecialUnitInfo(iFighter).getTextKey(), gc.getProjectInfo(iProject).getTextKey()))

	return szHelp

def canTriggerOverwhelmDone(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iDestroyer = CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_DESTROYER')
	iNumDestroyers = 4
	if player.getUnitClassCount(iDestroyer) < iNumDestroyers:
		return false
			
	iBattleship = CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_BATTLESHIP')
	iNumBattleships = 2
	if player.getUnitClassCount(iBattleship) < iNumBattleships:
		return false

	iCarrier = CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_CARRIER')
	iNumCarriers = 3
	if player.getUnitClassCount(iCarrier) < iNumCarriers:
		return false

	iFighter = CvUtil.findInfoTypeNum(gc.getSpecialUnitInfo, gc.getNumSpecialUnitInfos(), 'SPECIALUNIT_FIGHTER')
	iNumFighters = 9
	iNumPlayerFighters = 0
	(loopUnit, iter) = player.firstUnit(false)
	while (loopUnit):
		if loopUnit.getSpecialUnitType() == iFighter:
			iNumPlayerFighters += 1
		(loopUnit, iter) = player.nextUnit(iter, false)

	if iNumPlayerFighters < iNumFighters:
		return false
			
	return true

def getHelpOverwhelmDone3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	szHelp = localText.getText("TXT_KEY_EVENT_OVERWHELM_DONE_HELP_3", ())
	
	return szHelp

def canApplyOverwhelmDone3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iProject = CvUtil.findInfoTypeNum(gc.getProjectInfo, gc.getNumProjectInfos(), 'PROJECT_MANHATTAN_PROJECT')
	if gc.getTeam(player.getTeam()).getProjectCount(iProject) == 0:
		return false
	
	return true

def applyOverwhelmDone3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	gc.getGame().changeNoNukesCount(1)
		
######## CORPORATE EXPANSION ###########

def canTriggerCorporateExpansion(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	city = gc.getGame().getHeadquarters(kTriggeredData.eCorporation)
	if None == city or city.isNone():
		return false

	# Hack to remember the number of cities you already have with the Corporation
	kActualTriggeredDataObject = player.getEventTriggered(kTriggeredData.iId)
	kActualTriggeredDataObject.iOtherPlayerCityId = gc.getGame().countCorporationLevels(kTriggeredData.eCorporation)	
	kActualTriggeredDataObject.iCityId = city.getID()
	kActualTriggeredDataObject.iPlotX = city.getX()
	kActualTriggeredDataObject.iPlotY = city.getY()

	bFound = false
	for iBuilding in range(gc.getNumBuildingInfos()):
		if gc.getBuildingInfo(iBuilding).getFoundsCorporation() == kTriggeredData.eCorporation:
			kActualTriggeredDataObject.eBuilding = BuildingTypes(iBuilding)
			bFound = true
			break
			
	if not bFound:
		return false
	
	return true

def expireCorporateExpansion1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	city = player.getCity(kTriggeredData.iCityId)
	if None == city or city.isNone():
		return true

	return false

def getHelpCorporateExpansion1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	
	iNumCities = worldSizeTarget() + 1
	szHelp = localText.getText("TXT_KEY_EVENT_CORPORATE_EXPANSION_HELP_1", (gc.getCorporationInfo(kTriggeredData.eCorporation).getTextKey(), iNumCities))

	return szHelp

def canTriggerCorporateExpansionDone(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	kOrigTriggeredData = player.getEventOccured(trigger.getPrereqEvent(0))
	
	iNumCitiesRequired = worldSizeTarget() + 1 + kOrigTriggeredData.iOtherPlayerCityId
	
	if iNumCitiesRequired > gc.getGame().countCorporationLevels(kOrigTriggeredData.eCorporation):
		return false

				
	kActualTriggeredDataObject = player.getEventTriggered(kTriggeredData.iId)
	kActualTriggeredDataObject.eCorporation = kOrigTriggeredData.eCorporation
	kActualTriggeredDataObject.eBuilding = kOrigTriggeredData.eBuilding
	kActualTriggeredDataObject.iCityId = kOrigTriggeredData.iCityId
	kActualTriggeredDataObject.iPlotX = kOrigTriggeredData.iPlotX
	kActualTriggeredDataObject.iPlotY = kOrigTriggeredData.iPlotY
			
	return true

def getHelpCorporateExpansionDone1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	szHelp = localText.getText("TXT_KEY_EVENT_YIELD_CHANGE_BUILDING", (gc.getBuildingInfo(kTriggeredData.eBuilding).getTextKey(), u"+10[ICON_GOLD]"))

	return szHelp

def applyCorporateExpansionDone1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	city = player.getCity(kTriggeredData.iCityId)
	if None != city and not city.isNone():
		city.setBuildingCommerceChange(gc.getBuildingInfo(kTriggeredData.eBuilding).getBuildingClassType(), CommerceTypes.COMMERCE_GOLD, 10)
		
######## HOSTILE TAKEOVER ###########

def canTriggerHostileTakeover(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_ONE_CITY_CHALLENGE) and gc.getPlayer(kTriggeredData.ePlayer).isHuman():
		return false

	city = gc.getGame().getHeadquarters(kTriggeredData.eCorporation)
	if None == city or city.isNone():
		return false

	# Hack to remember the number of cities you already have with the Corporation
	kActualTriggeredDataObject = player.getEventTriggered(kTriggeredData.iId)
	kActualTriggeredDataObject.iCityId = city.getID()
	kActualTriggeredDataObject.iPlotX = city.getX()
	kActualTriggeredDataObject.iPlotY = city.getY()

	bFound = false
	for iBuilding in range(gc.getNumBuildingInfos()):
		if gc.getBuildingInfo(iBuilding).getFoundsCorporation() == kTriggeredData.eCorporation:
			kActualTriggeredDataObject.eBuilding = BuildingTypes(iBuilding)
			bFound = true
			break
			
	if not bFound:
		return false

	listResources = getHostileTakeoverListResources(gc.getCorporationInfo(kTriggeredData.eCorporation), player)
	if len(listResources) == 0:
		return false
		
	return true

def expireHostileTakeover1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	city = player.getCity(kTriggeredData.iCityId)
	if None == city or city.isNone():
		return true

	return false

def getHostileTakeoverListResources(corporation, player):
	map = gc.getMap()
	listHave = []
	for i in range(map.numPlots()):
		plot = map.plotByIndex(i)
		if plot.getOwner() == player.getID():
			iBonus = plot.getBonusType(player.getTeam())
			if iBonus != -1:
				if not iBonus in listHave:
					listHave.append(iBonus)
	listNeed = []
	for i in range(gc.getNUM_CORPORATION_PREREQ_BONUSES()):
		iBonus = corporation.getPrereqBonus(i)
		if iBonus != -1:
			if not iBonus in listHave:
				listNeed.append(iBonus)
	return listNeed
	
def getHelpHostileTakeover1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	listResources = getHostileTakeoverListResources(gc.getCorporationInfo(kTriggeredData.eCorporation), player)
	szList = u""
	bFirst = true
	for iBonus in listResources:
		if not bFirst:
			szList += u", "
		else:
			bFirst = false
		szList += u"[COLOR_HIGHLIGHT_TEXT]" + gc.getBonusInfo(iBonus).getDescription() + u"[COLOR_REVERT]"
		
	szHelp = localText.getText("TXT_KEY_EVENT_HOSTILE_TAKEOVER_HELP_1", (len(listResources), szList))

	return szHelp

def canTriggerHostileTakeoverDone(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	trigger = gc.getEventTriggerInfo(kTriggeredData.eTrigger)
	kOrigTriggeredData = player.getEventOccured(trigger.getPrereqEvent(0))
	
	listResources = getHostileTakeoverListResources(gc.getCorporationInfo(kOrigTriggeredData.eCorporation), player)
	
	if len(listResources) > 0:
		return false
				
	kActualTriggeredDataObject = player.getEventTriggered(kTriggeredData.iId)
	kActualTriggeredDataObject.eCorporation = kOrigTriggeredData.eCorporation
	kActualTriggeredDataObject.eBuilding = kOrigTriggeredData.eBuilding
	kActualTriggeredDataObject.iCityId = kOrigTriggeredData.iCityId
	kActualTriggeredDataObject.iPlotX = kOrigTriggeredData.iPlotX
	kActualTriggeredDataObject.iPlotY = kOrigTriggeredData.iPlotY
			
	return true

def getHelpHostileTakeoverDone1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	szHelp = localText.getText("TXT_KEY_EVENT_YIELD_CHANGE_BUILDING", (gc.getBuildingInfo(kTriggeredData.eBuilding).getTextKey(), u"+20[ICON_GOLD]"))

	return szHelp

def applyHostileTakeoverDone1(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	city = player.getCity(kTriggeredData.iCityId)
	if None != city and not city.isNone():
		city.setBuildingCommerceChange(gc.getBuildingInfo(kTriggeredData.eBuilding).getBuildingClassType(), CommerceTypes.COMMERCE_GOLD, 20)
		
		
######## Great Beast ########

def doGreatBeast3(argsList):
	kTriggeredData = argsList[1]
	
	player = gc.getPlayer(kTriggeredData.ePlayer)
	(loopCity, iter) = player.firstCity(false)

	while(loopCity):
		if loopCity.isHasReligion(kTriggeredData.eReligion):
			loopCity.changeHappinessTimer(40)
		(loopCity, iter) = player.nextCity(iter, false)

def getHelpGreatBeast3(argsList):
	kTriggeredData = argsList[1]
	religion = gc.getReligionInfo(kTriggeredData.eReligion)

	szHelp = localText.getText("TXT_KEY_EVENT_GREAT_BEAST_3_HELP", (gc.getDefineINT("TEMP_HAPPY"), 40, religion.getChar()))

	return szHelp

####### Comet Fragment ########

def canDoCometFragment(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	if (player.getSpaceProductionModifier()) > 10:
		return false
	
	return true

####### Immigrants ########

def canTriggerImmigrantCity(argsList):
	ePlayer = argsList[1]
	iCity = argsList[2]
	
	player = gc.getPlayer(ePlayer)
	city = player.getCity(iCity)

	if city.isNone():
		return false

	if ((city.happyLevel() - city.unhappyLevel(0) < 1) or (city.goodHealth() - city.badHealth(true) < 1)):
		return false

	if (city.getCommerceRateTimes100(CommerceTypes.COMMERCE_CULTURE) < 5500):
		return false

####### Controversial Philosopher ######

def canTriggerControversialPhilosopherCity(argsList):
	ePlayer = argsList[1]
	iCity = argsList[2]
	
	player = gc.getPlayer(ePlayer)
	city = player.getCity(iCity)
	
	if city.isNone():
		return false
	if (not city.isCapital()):
		return false
	if (city.getCommerceRateTimes100(CommerceTypes.COMMERCE_RESEARCH) < 3500):
		return false

	return true

####### Spy Discovered #######


def canDoSpyDiscovered3(argsList):
	iEvent = argsList[0]
	kTriggeredData = argsList[1]

	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	if player.getNumCities() < 4:
		return false
	
	if player.getCapitalCity().isNone():
		return false
				
	return true

def doSpyDiscovered3(argsList):
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	plot = player.getCapitalCity().plot()
	iNumUnits = player.getNumCities() / 4
	iUnitClassType = CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_TANK')
	iUnitType = gc.getCivilizationInfo(player.getCivilizationType()).getCivilizationUnits(iUnitClassType)
	
	if iUnitType != -1:
		for i in range(iNumUnits):
			player.initUnit(iUnitType, plot.getX(), plot.getY(), UnitAITypes.UNITAI_ATTACK, DirectionTypes.DIRECTION_SOUTH)

def getHelpSpyDiscovered3(argsList):
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	iNumUnits = player.getNumCities() / 4
	szHelp = localText.getText("TXT_KEY_EVENT_SPY_DISCOVERED_3_HELP", (iNumUnits, ))

	return szHelp

####### Nuclear Protest #######

def canTriggerNuclearProtest(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)
	
	iICBMClass = CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_ICBM')
	iTacNukeClass = CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_TACTICAL_NUKE')
	if player.getUnitClassCount(iICBMClass) + player.getUnitClassCount(iTacNukeClass) < 10:
		return false

	return true

def doNuclearProtest1(argsList):
	kTriggeredData = argsList[1]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	iICBMClass = CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_ICBM')
	iTacNukeClass = CvUtil.findInfoTypeNum(gc.getUnitClassInfo, gc.getNumUnitClassInfos(), 'UNITCLASS_TACTICAL_NUKE')

	(loopUnit, iter) = player.firstUnit(false)
	while (loopUnit):
		if loopUnit.getUnitClassType() == iICBMClass or loopUnit.getUnitClassType() == iTacNukeClass:
			loopUnit.kill(false, -1)
		(loopUnit, iter) = player.nextUnit(iter, false)

def getHelpNuclearProtest1(argsList):
	szHelp = localText.getText("TXT_KEY_EVENT_NUCLEAR_PROTEST_1_HELP", ())
	return szHelp


######## Preaching Researcher #######

def canTriggerPreachingResearcherCity(argsList):
	iCity = argsList[2]
	
	player = gc.getPlayer(argsList[1])
	city = player.getCity(iCity)

	if city.isHasBuilding(gc.getInfoTypeForString("BUILDING_UNIVERSITY")):
		return true
	return false

######## Toxcatl (Aztec event) #########

def canTriggerToxcatl(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	if (player.getCivilizationType() == gc.getInfoTypeForString("CIVILIZATION_AZTEC")):
		return true
	return false

######## Dissident Priest (Egyptian event) #######

def canTriggerDissidentPriest(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	if (player.getCivilizationType() == gc.getInfoTypeForString("CIVILIZATION_EGYPT")):
		return true
	return false

def canTriggerDissidentPriestCity(argsList):
	iCity = argsList[2]
	
	player = gc.getPlayer(argsList[1])
	city = player.getCity(iCity)

	if city.isGovernmentCenter():
		return false
	if (city.getCommerceRateTimes100(CommerceTypes.COMMERCE_CULTURE) < 3000):
		return false
	if (player.getStateReligion() != -1):
		return false

	return true

######## Rogue Station  (Russian event) ###########

def canTriggerRogueStation(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	if (player.getCivilizationType() == gc.getInfoTypeForString("CIVILIZATION_RUSSIA")):
		return true
	return false

######## Antimonarchists (French event) #########

def canTriggerAntiMonarchists(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	if (player.getCivilizationType() == gc.getInfoTypeForString("CIVILIZATION_FRANCE")):
		return true
	return false

######## Impeachment (American event) ########

def canTriggerImpeachment(argsList):
	kTriggeredData = argsList[0]
	player = gc.getPlayer(kTriggeredData.ePlayer)

	if (player.getCivilizationType() == gc.getInfoTypeForString("CIVILIZATION_AMERICA")):
		return true
	return false

def canTriggerImpeachmentCity(argsList):
	iCity = argsList[2]
	
	player = gc.getPlayer(argsList[1])
	city = player.getCity(iCity)

	if city.isCapital():
		return true
	return false

