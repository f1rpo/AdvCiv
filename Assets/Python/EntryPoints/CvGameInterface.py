## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
# WARNING - The DLL specifically calls the functions as they are named
# advc.003y: I've copied this file from BtS only to add the missing
# cannotSpreadReligion function. For adding new Python callbacks,
# BUG\BugGameUtils.py (or BugEventManager.py) should be used instead.

import CvUtil
import CvGameUtils
import CvGameInterfaceFile
import CvEventInterface
from CvPythonExtensions import *

# globals
gc = CyGlobalContext()
normalGameUtils = CvGameInterfaceFile.GameUtils

def gameUtils():
	' replace normalGameUtils with your mod version'
	return normalGameUtils
		
def isVictoryTest():
	#CvUtil.pyPrint( "CvGameInterface.isVictoryTest" )
	return gameUtils().isVictoryTest()

def isVictory(argsList):
	return gameUtils().isVictory(argsList)

def isPlayerResearch(argsList):
	#CvUtil.pyPrint( "CvGameInterface.isPlayerResearch" )
	return gameUtils().isPlayerResearch(argsList)

def getExtraCost(argsList):
	#CvUtil.pyPrint( "CvGameInterface.getExtraCost" )
	return gameUtils().getExtraCost(argsList)

def createBarbarianCities():
	#CvUtil.pyPrint( "CvGameInterface.createBarbarianCities" )
	return gameUtils().createBarbarianCities()

def createBarbarianUnits():
	#CvUtil.pyPrint( "CvGameInterface.createBarbarianUnits" )
	return gameUtils().createBarbarianUnits()

def skipResearchPopup(argsList):
	#CvUtil.pyPrint( "CvGameInterface.skipResearchPopup" )
	return gameUtils().skipResearchPopup(argsList)

def showTechChooserButton(argsList):
	#CvUtil.pyPrint( "CvGameInterface.showTechChooserButton" )
	return gameUtils().showTechChooserButton(argsList)

def getFirstRecommendedTech(argsList):
	#CvUtil.pyPrint( "CvGameInterface.getFirstRecommendedTech" )
	return gameUtils().getFirstRecommendedTech(argsList)

def getSecondRecommendedTech(argsList):
	#CvUtil.pyPrint( "CvGameInterface.getSecondRecommendedTech" )
	return gameUtils().getSecondRecommendedTech(argsList)

def skipProductionPopup(argsList):
	#CvUtil.pyPrint( "CvGameInterface.skipProductionPopup" )
	return gameUtils().skipProductionPopup(argsList)

def canRazeCity(argsList):
	#CvUtil.pyPrint( "CvGameInterface.canRazeCity" )
	return gameUtils().canRazeCity(argsList)

def canDeclareWar(argsList):
	#CvUtil.pyPrint( "CvGameInterface.canRazeCity" )
	return gameUtils().canDeclareWar(argsList)

def showExamineCityButton(argsList):
	#CvUtil.pyPrint( "CvGameInterface.showExamineCityButton" )
	return gameUtils().showExamineCityButton(argsList)

def getRecommendedUnit(argsList):
	#CvUtil.pyPrint( "CvGameInterface.getRecommendedUnit" )
	return gameUtils().getRecommendedUnit(argsList)

def getRecommendedBuilding(argsList):
	#CvUtil.pyPrint( "CvGameInterface.getRecommendedBuilding" )
	return gameUtils().getRecommendedBuilding(argsList)

def updateColoredPlots():
	#CvUtil.pyPrint( "CvGameInterface.updateColoredPlots" )
	return gameUtils().updateColoredPlots()

def isActionRecommended(argsList):
	#CvUtil.pyPrint( "CvGameInterface.isActionRecommended" )
	return gameUtils().isActionRecommended(argsList)

def unitCannotMoveInto(argsList):
	return gameUtils().unitCannotMoveInto(argsList)

def cannotHandleAction(argsList):
	#CvUtil.pyPrint( "CvGameInterface.cannotHandleAction" )
	return gameUtils().cannotHandleAction(argsList)

def canBuild(argsList):
	#CvUtil.pyPrint( "CvGameInterface.canBuild" )
	return gameUtils().canBuild(argsList)

def cannotFoundCity(argsList):
	#CvUtil.pyPrint( "CvGameInterface.cannotHandleAction" )
	return gameUtils().cannotFoundCity(argsList)

def cannotSelectionListMove(argsList):
	#CvUtil.pyPrint( "CvGameInterface.cannotSelectionListMove" )
	return gameUtils().cannotSelectionListMove(argsList)

def cannotSelectionListGameNetMessage(argsList):
	#CvUtil.pyPrint( "CvGameInterface.cannotSelectionListGameNetMessage" )
	return gameUtils().cannotSelectionListGameNetMessage(argsList)

def cannotDoControl(argsList):
	#CvUtil.pyPrint( "CvGameInterface.cannotDoControl" )
	return gameUtils().cannotDoControl(argsList)

def canResearch(argsList):
	#CvUtil.pyPrint( "CvGameInterface.canResearch" )
	return gameUtils().canResearch(argsList)

def cannotResearch(argsList):
	#CvUtil.pyPrint( "CvGameInterface.cannotResearch" )
	return gameUtils().cannotResearch(argsList)

def canDoCivic(argsList):
	#CvUtil.pyPrint( "CvGameInterface.canDoCivic" )
	return gameUtils().canDoCivic(argsList)

def cannotDoCivic(argsList):
	#CvUtil.pyPrint( "CvGameInterface.cannotDoCivic" )
	return gameUtils().cannotDoCivic(argsList)

def canTrain(argsList):
	#CvUtil.pyPrint( "CvGameInterface.canTrain" )
	return gameUtils().canTrain(argsList)

def cannotTrain(argsList):
	#CvUtil.pyPrint( "CvGameInterface.cannotTrain" )
	return gameUtils().cannotTrain(argsList)

def canConstruct(argsList):
	#CvUtil.pyPrint( "CvGameInterface.canConstruct" )
	return gameUtils().canConstruct(argsList)

def cannotConstruct(argsList):
	#CvUtil.pyPrint( "CvGameInterface.cannotConstruct" )
	return gameUtils().cannotConstruct(argsList)

def canCreate(argsList):
	#CvUtil.pyPrint( "CvGameInterface.canCreate" )
	return gameUtils().canCreate(argsList)

def cannotCreate(argsList):
	#CvUtil.pyPrint( "CvGameInterface.cannotCreate" )
	return gameUtils().cannotCreate(argsList)

def canMaintain(argsList):
	#CvUtil.pyPrint( "CvGameInterface.canMaintain" )
	return gameUtils().canMaintain(argsList)

def cannotMaintain(argsList):
	#CvUtil.pyPrint( "CvGameInterface.cannotMaintain" )
	return gameUtils().cannotMaintain(argsList)

def AI_chooseTech(argsList):
	'AI chooses what to research'
	#CvUtil.pyPrint( "CvGameInterface.AI_chooseTech" )
	return gameUtils().AI_chooseTech(argsList)

def AI_chooseProduction(argsList):
	'AI chooses city production'
	#CvUtil.pyPrint( "CvGameInterface.AI_chooseProduction" )
	return gameUtils().AI_chooseProduction(argsList)

def AI_unitUpdate(argsList):
	'AI moves units - return 0 to let AI handle it, return 1 to say that the move is handled in python '
	#CvUtil.pyPrint( "CvGameInterface.AI_unitUpdate" )
	return gameUtils().AI_unitUpdate(argsList)

def AI_doWar(argsList):
	'AI decides whether to make war or peace - return 0 to let AI handle it, return 1 to say that the move is handled in python '
	#CvUtil.pyPrint( "CvGameInterface.AI_doWar" )
	return gameUtils().AI_doWar(argsList)

def AI_doDiplo(argsList):
	'AI decides does diplomacy for the turn - return 0 to let AI handle it, return 1 to say that the move is handled in python '
	#CvUtil.pyPrint( "CvGameInterface.AI_doDiplo" )
	return gameUtils().AI_doDiplo(argsList)

def calculateScore(argsList):
	return gameUtils().calculateScore(argsList)

def doHolyCity():
	#CvUtil.pyPrint( "CvGameInterface.doHolyCity" )
	return gameUtils().doHolyCity()

def doHolyCityTech(argsList):
	#CvUtil.pyPrint( "CvGameInterface.doHolyCityTech" )
	return gameUtils().doHolyCityTech(argsList)

def doGold(argsList):
	#CvUtil.pyPrint( "CvGameInterface.doGold" )
	return gameUtils().doGold(argsList)

def doResearch(argsList):
	#CvUtil.pyPrint( "CvGameInterface.doResearch" )
	return gameUtils().doResearch(argsList)

def doGoody(argsList):
	#CvUtil.pyPrint( "CvGameInterface.doGoody" )
	return gameUtils().doGoody(argsList)

def doGrowth(argsList):
	#CvUtil.pyPrint( "CvGameInterface.doGrowth" )
	return gameUtils().doGrowth(argsList)

def doProduction(argsList):
	#CvUtil.pyPrint( "CvGameInterface.doProduction" )
	return gameUtils().doProduction(argsList)

def doCulture(argsList):
	#CvUtil.pyPrint( "CvGameInterface.doCulture" )
	return gameUtils().doCulture(argsList)

def doPlotCulture(argsList):
	#CvUtil.pyPrint( "CvGameInterface.doPlotCulture" )
	return gameUtils().doPlotCulture(argsList)

def doReligion(argsList):
	#CvUtil.pyPrint( "CvGameInterface.doReligion" )
	return gameUtils().doReligion(argsList)
# <advc.003y> This one was missing; tagging advc.001 (bugfix).
def cannotSpreadReligion(argsList):
	#CvUtil.pyPrint( "CvGameInterface.cannotSpreadReligion" )
	return gameUtils().cannotSpreadReligion(argsList) # </advc.003y>

def doGreatPeople(argsList):
	#CvUtil.pyPrint( "CvGameInterface.doGreatPeople" )
	return gameUtils().doGreatPeople(argsList)

def doMeltdown(argsList):
	#CvUtil.pyPrint( "CvGameInterface.doMeltdown" )
	return gameUtils().doMeltdown(argsList)

def doReviveActivePlayer(argsList):
	return gameUtils().doReviveActivePlayer(argsList)

def doPillageGold(argsList):
	return gameUtils().doPillageGold(argsList)

def doCityCaptureGold(argsList):
	return gameUtils().doCityCaptureGold(argsList)

def citiesDestroyFeatures(argsList):
	return gameUtils().citiesDestroyFeatures(argsList)

def canFoundCitiesOnWater(argsList):
	return gameUtils().canFoundCitiesOnWater(argsList)

def doCombat(argsList):
	return gameUtils().doCombat(argsList)

def getConscriptUnitType(argsList):
	return gameUtils().getConscriptUnitType(argsList)

def getCityFoundValue(argsList):
	return gameUtils().getCityFoundValue(argsList)

def canPickPlot(argsList):
	return gameUtils().canPickPlot(argsList)

def getUnitCostMod(argsList):
	return gameUtils().getUnitCostMod(argsList)

def getBuildingCostMod(argsList):
	return gameUtils().getBuildingCostMod(argsList)

def canUpgradeAnywhere(argsList):
	return gameUtils().canUpgradeAnywhere(argsList)
	
def getWidgetHelp(argsList):
	return gameUtils().getWidgetHelp(argsList)
	
def getUpgradePriceOverride(argsList):
	return gameUtils().getUpgradePriceOverride(argsList)

def getExperienceNeeded(argsList):
	return gameUtils().getExperienceNeeded(argsList)