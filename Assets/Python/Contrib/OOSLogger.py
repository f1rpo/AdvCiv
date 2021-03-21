import os
from CvPythonExtensions import *
import CvUtil
import BugPath

gc = CyGlobalContext()

#szFilename = "OOSLog.txt"

bWroteLog = False # (advc: This doesn't seem to accomplish anything(?))

SEPERATOR = "-----------------------------------------------------------------\n"

# kekm.27
# DarkLunaPhantom - OOS logging, idea from Fall from Heaven 2 by Kael. This implementation is a slight modification of the one from ExtraModMod by Terkhen. Implemented as a BUG module.

# Simply checks every game turn for OOS. If it finds it, writes the
# info contained in the sync checksum to a log file, then sets the bWroteLog
# variable so that it only happens once.
def onGameUpdate(argsList):
	global bWroteLog
	# <advc> The OOS log reveals some secret info. Require MessageLog (isLogging) so that players receive a popup about logging. Plus a WorldBuilder check for advc.135c (WorldBuilder always leads to OOS)
	if not gc.isLogging() or gc.getGame().GetWorldBuilderMode():
		return # </advc>

	bOOS = CyInterface().isOOSVisible()

	if (bOOS and not bWroteLog):
		writeLog()
		# Automatic OOS detection START
		#gc.getGame().setOOSVisible() # advc: This function doesn't exist in Kek-Mod/AdvCiv
		# Automatic OOS detection END
		bWroteLog = True
	# Make sure that the OOS log will be generated when the game is not restarted (advc: but e.g. reloaded?) after an OOS.
	if (not bOOS):
		bWroteLog = False

def writeLog():

	if CyGame().isPitbossHost():
		playername = "PitBoss"
	else:
		# advc: Prepend id b/c player names can be the same (that happens easily when testing on a single machine)
		activePlayer = gc.getPlayer(gc.getGame().getActivePlayer())
		playername = str(activePlayer.getID()) + CvUtil.convertToStr(activePlayer.getName())
	szNewFilename = BugPath.getRootDir() + "\\Logs\\" + "OOSLog - %s - " % (playername) + "Turn %s" % (gc.getGame().getGameTurn()) + ".log"
	# <advc> Replacement for the bWroteLog mechanism above
	if os.path.isfile(szNewFilename):
		return # </advc>
	pFile = open(szNewFilename, "w")

	# Backup current language
	iLanguage = CyGame().getCurrentLanguage()
	# Force english language for logs
	CyGame().setCurrentLanguage(0)

	#
	# Global data
	#
	pFile.write(SEPERATOR)
	pFile.write(SEPERATOR)

	#pFile.write(CvUtil.convertToStr(CyTranslator().getText("TXT_KEY_VERSION", ())))
	#pFile.write("\n\n")

	# The log follows the order in CvGame::calculateSyncChecksum()

	pFile.write("  GLOBALS  \n")

	pFile.write(SEPERATOR)
	pFile.write(SEPERATOR)
	pFile.write("\n\n")
	# advc: Call getSeed instead of get -- don't want to change the state of the RNGs here.
	pFile.write("Last Map Rand Value: %d\n" % CyGame().getMapRand().getSeed())
	pFile.write("Last Soren Rand Value: %d\n" % CyGame().getSorenRand().getSeed())

	pFile.write("Total cities: %d\n" % CyGame().getNumCities() )
	pFile.write("Total population: %d\n" % CyGame().getTotalPopulation() )
	pFile.write("Total deals: %d\n" % CyGame().getNumDeals() )

	pFile.write("Total owned plots: %d\n" % CyMap().getOwnedPlots() )
	pFile.write("Total number of areas: %d\n" % CyMap().getNumAreas() )

	#pFile.write("Global counter: %d\n" % CyGame().getGlobalCounter() )
	#pFile.write("Total civilization cities: %d\n" % CyGame().getNumCivCities() )

	pFile.write("Turn slice: %d\n" % (CyGame().getTurnSlice() % 8))

	pFile.write("\n\n")

	#
	# Player data
	#
	iPlayer = 0
	for iPlayer in range(gc.getMAX_PLAYERS()):
		pPlayer = gc.getPlayer(iPlayer)
		if (pPlayer.isEverAlive()):
			pFile.write(SEPERATOR)
			pFile.write(SEPERATOR)

			pFile.write("  PLAYER %d: %s  \n" % (iPlayer, CvUtil.convertToStr(pPlayer.getName())))
			#pFile.write("  Civilization: %s  \n" % (CvUtil.convertToStr(pPlayer.getCivilizationDescriptionKey())))

			pFile.write(SEPERATOR)
			pFile.write(SEPERATOR)
			pFile.write("\n\n")

			pFile.write("Basic data:\n")
			pFile.write("-----------\n")
			pFile.write("Player %d Score: %d\n" % (iPlayer, gc.getGame().getPlayerScore(iPlayer) ))

			pFile.write("Player %d Population: %d\n" % (iPlayer, pPlayer.getTotalPopulation() ) )
			pFile.write("Player %d Total Land: %d\n" % (iPlayer, pPlayer.getTotalLand() ) )
			pFile.write("Player %d Gold: %d\n" % (iPlayer, pPlayer.getGold() ) )
			pFile.write("Player %d Assets: %d\n" % (iPlayer, pPlayer.getAssets() ) )
			pFile.write("Player %d Power: %d\n" % (iPlayer, pPlayer.getPower() ) )
			pFile.write("Player %d Num Cities: %d\n" % (iPlayer, pPlayer.getNumCities() ) )
			pFile.write("Player %d Num Units: %d\n" % (iPlayer, pPlayer.getNumUnits() ) )
			pFile.write("Player %d Num Selection Groups: %d\n" % (iPlayer, pPlayer.getNumSelectionGroups() ) )
			#pFile.write("Player %d Difficulty: %d\n" % (iPlayer, pPlayer.getHandicapType() ))
			#pFile.write("Player %d Religion: %s\n" % (iPlayer, CvUtil.convertToStr(pPlayer.getStateReligionKey()) ))
			#.pFile.write("Player %d Total culture: %d\n" % (iPlayer, pPlayer.countTotalCulture() ))

			pFile.write("\n\n")

			pFile.write("Yields:\n")
			pFile.write("-------\n")
			for iYield in range( int(YieldTypes.NUM_YIELD_TYPES) ):
				pFile.write("Player %d %s Total Yield: %d\n" % (iPlayer, gc.getYieldInfo(iYield).getDescription(), pPlayer.calculateTotalYield(iYield) ))

			pFile.write("\n\n")

			pFile.write("Commerce:\n")
			pFile.write("---------\n")
			for iCommerce in range( int(CommerceTypes.NUM_COMMERCE_TYPES) ):
				pFile.write("Player %d %s Total Commerce: %d\n" % (iPlayer, gc.getCommerceInfo(iCommerce).getDescription(), pPlayer.getCommerceRate(CommerceTypes(iCommerce)) ))

			pFile.write("\n\n")

			pFile.write("Bonus Info:\n")
			pFile.write("-----------\n")
			for iBonus in range(gc.getNumBonusInfos()):
				pFile.write("Player %d, %s, Number Available: %d\n" % (iPlayer, gc.getBonusInfo(iBonus).getDescription(), pPlayer.getNumAvailableBonuses(iBonus) ))
				pFile.write("Player %d, %s, Import: %d\n" % (iPlayer, gc.getBonusInfo(iBonus).getDescription(), pPlayer.getBonusImport(iBonus) ))
				pFile.write("Player %d, %s, Export: %d\n" % (iPlayer, gc.getBonusInfo(iBonus).getDescription(), pPlayer.getBonusExport(iBonus) ))
				pFile.write("\n")

			pFile.write("\n\n")

			pFile.write("Improvement Info:\n")
			pFile.write("-----------------\n")
			for iImprovement in range(gc.getNumImprovementInfos()):
				pFile.write("Player %d, %s, Improvement count: %d\n" % (iPlayer, CvUtil.convertToStr(gc.getImprovementInfo(iImprovement).getDescription()), pPlayer.getImprovementCount(iImprovement) ))

			pFile.write("\n\n")

			pFile.write("Building Class Info:\n")
			pFile.write("--------------------\n")
			for iBuildingClass in range(gc.getNumBuildingClassInfos()):
				pFile.write("Player %d, %s, Building class count plus making: %d\n" % (iPlayer, CvUtil.convertToStr(gc.getBuildingClassInfo(iBuildingClass).getDescription()), pPlayer.getBuildingClassCountPlusMaking(iBuildingClass) ))

			pFile.write("\n\n")

			pFile.write("Unit Class Info:\n")
			pFile.write("--------------------\n")
			for iUnitClass in range(gc.getNumUnitClassInfos()):
				pFile.write("Player %d, %s, Unit class count plus training: %d\n" % (iPlayer, CvUtil.convertToStr(gc.getUnitClassInfo(iUnitClass).getDescription()), pPlayer.getUnitClassCountPlusMaking(iUnitClass) ))

			pFile.write("\n\n")

			pFile.write("UnitAI Types Info:\n")
			pFile.write("------------------\n")
			for iUnitAIType in range(int(UnitAITypes.NUM_UNITAI_TYPES)):
				pFile.write("Player %d, %s, Unit AI Type count: %d\n" % (iPlayer, gc.getUnitAIInfo(iUnitAIType).getDescription(), pPlayer.AI_totalUnitAIs(UnitAITypes(iUnitAIType)) ))

			pFile.write("\n\n")

			pFile.write("Unit Info:\n")
			pFile.write("----------\n")
			iNumUnits = pPlayer.getNumUnits()

			if (iNumUnits == 0):
				pFile.write("No Units")
			else:
				pLoopUnitTuple = pPlayer.firstUnit(False)
				while (pLoopUnitTuple[0] != None):
					pUnit = pLoopUnitTuple[0]
					pFile.write("Player %d, Unit ID: %d, %s\n" % (iPlayer, pUnit.getID(), CvUtil.convertToStr(pUnit.getName()) ))

					pFile.write("X: %d, Y: %d\n" % (pUnit.getX(), pUnit.getY()) )
					pFile.write("Damage: %d\n" % pUnit.getDamage() )
					#pFile.write("Experience: %d\n" % pUnit.getExperienceTimes100() )
					pFile.write("Experience: %d\n" % pUnit.getExperience() )
					pFile.write("Level: %d\n" % pUnit.getLevel() )
					#pFile.write("Promotions:\n")
					#for j in range(gc.getNumPromotionInfos()):
					#	if (pUnit.isHasPromotion(j)):
					#		pFile.write("%s\n" % (CvUtil.convertToStr(gc.getPromotionInfo(j).getDescription()) ))

					pLoopUnitTuple = pPlayer.nextUnit(pLoopUnitTuple[1], False)
					pFile.write("\n")

			if not pPlayer.isBarbarian(): # advc.003n
				pFile.write("\n\n")
				pFile.write("Attitude Info:\n")
				pFile.write("----------\n")

				iLoopPlayer = 0
				# <advc.003n> was MAX_PLAYERS
				for iLoopPlayer in range(gc.getMAX_CIV_PLAYERS()): 
					if iPlayer == iLoopPlayer:
						continue # </advc.003n>
					pLoopPlayer = gc.getPlayer(iLoopPlayer)
					pFile.write("Players %d - %d, Attitude: %d (Note, actual attitudeval number is used for the OOS checksum.)\n" % (iPlayer, iLoopPlayer, pPlayer.AI_getAttitude(iLoopPlayer)))

			pFile.write("\n\n")

			pFile.write("City Info:\n")
			pFile.write("----------\n")
			iNumCities = pPlayer.getNumCities()

			if (iNumCities == 0):
				pFile.write("No Cities")
			else:
				# advc: Moved out of the loop - don't print this for each city.
				pFile.write("(Events that have occurred are also used for the checksum.)\n")
				pLoopCityTuple = pPlayer.firstCity(False)
				while (pLoopCityTuple[0] != None):
					pCity = pLoopCityTuple[0]
					pFile.write("Player %d, City ID: %d, %s, X: %d, Y: %d\n"% (iPlayer, pCity.getID(), CvUtil.convertToStr(pCity.getName()), pCity.getX(), pCity.getY()))

					pFile.write("Religions and corporations present are also used for the checksum.\n")
					#pFile.write("Founded: %d\n" % pCity.getGameTurnFounded() )
					#pFile.write("Population: %d\n" % pCity.getPopulation() )
					#pFile.write("Buildings: %d\n" % pCity.getNumBuildings() )
					#pFile.write("Improved Plots: %d\n" % pCity.countNumImprovedPlots() )
					#pFile.write("Producing: %s\n" % pCity.getProductionName() )
					#pFile.write("Turns remaining for production: %d\n" % pCity.getProductionTurnsLeft() )
					pFile.write("%d happiness, %d unhappiness, %d health, %d unhealth, %d food\n" % (pCity.happyLevel(), pCity.unhappyLevel(0), pCity.goodHealth(), pCity.badHealth(False), pCity.getFood()) )
					# <advc.007>
					if not pCity.isBarbarian():
						pFile.write("Needed floating defenders: %d\n" % pCity.AI_neededFloatingDefenders())
					# </ advc.007>
					#pFile.write("%d Tiles Worked, %d Specialists, %d Great People\n" % (pCity.getWorkingPopulation(), pCity.getSpecialistPopulation(), pCity.getNumGreatPeople()) )
					#pFile.write("City radius: %d\n" % pCity.getPlotRadius() )

					pLoopCityTuple = pPlayer.nextCity(pLoopCityTuple[1], False)
					pFile.write("\n")


			# Space at end of player's info
			pFile.write("\n\n")

	# Restore current language
	CyGame().setCurrentLanguage(iLanguage)

	# Close file

	pFile.close()
