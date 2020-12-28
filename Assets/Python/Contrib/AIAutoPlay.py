# AI_AUTO_PLAY_MOD
#
# by jdog5000
# version 2.0

from CvPythonExtensions import *
import CvScreenEnums
import CvTopCivs
import CvUtil
import PyHelpers
import Popup as PyPopup

import ChangePlayer
import time # Erik (BM1)

# globals
gc = CyGlobalContext()
PyPlayer = PyHelpers.PyPlayer
PyInfo = PyHelpers.PyInfo
game = CyGame()
localText = CyTranslator()

class AIAutoPlay :

	def __init__(self, customEM ) :

		print "Initializing AIAutoPlay Mod"

		self.LOG_DEBUG = False
		self.blockPopups = True
		self.refortify = False
		self.bSaveAllDeaths = True

		self.DefaultTurnsToAuto = 1
		# Erik <BM1>
		self.benchmarkStartTime = 0
		self.numTurns = 0
		self.bBenchmark = False # advc
		# </BM1>
		self.customEM = customEM

		self.customEM.addEventHandler( "kbdEvent", self.onKbdEvent )
		# advc: Disable unused handler
		#self.customEM.addEventHandler( "EndGameTurn", self.onEndGameTurn )
		#self.customEM.addEventHandler( 'OnLoad', self.onGameLoad )
		#self.customEM.addEventHandler( 'GameStart', self.onGameStart )
		self.customEM.addEventHandler( 'BeginPlayerTurn', self.onBeginPlayerTurn )
		self.customEM.addEventHandler( 'EndPlayerTurn', self.onEndPlayerTurn )
		self.customEM.addEventHandler( 'victory', self.onVictory )
		self.customEM.addEventHandler( 'AutoPlayComplete', self.onAutoPlayComplete ) # Erik (BM1)

		self.customEM.setPopupHandler( 7050, ["toAIChooserPopup",self.AIChooserHandler,self.blankHandler] )
		self.customEM.setPopupHandler( 7052, ["pickHumanPopup",self.pickHumanHandler,self.blankHandler] )

		# Keep game from showing messages about handling these popups
		CvUtil.SilentEvents.extend([7050,7052])
		
		if( self.blockPopups ) :
			print "Removing some event handlers"
			try :
				self.customEM.removeEventHandler( "cityBuilt", customEM.onCityBuilt )
				self.customEM.addEventHandler( "cityBuilt", self.onCityBuilt )
			except ValueError :
				print "Failed to remove 'onCityBuilt', perhaps not registered"
				self.customEM.setEventHandler( "cityBuilt", self.onCityBuilt )
			
			try :
				self.customEM.removeEventHandler( "BeginGameTurn", customEM.onBeginGameTurn )
				self.customEM.addEventHandler( "BeginGameTurn", self.onBeginGameTurn )
			except ValueError :
				print "Failed to remove 'onBeginGameTurn', perhaps not registered"
				self.customEM.setEventHandler( "BeginGameTurn", self.onBeginGameTurn )


	def removeEventHandlers( self ) :
		print "Removing event handlers from AIAutoPlay"
		
		self.customEM.removeEventHandler( "kbdEvent", self.onKbdEvent )
		# advc: Disable unused handlers
		#self.customEM.removeEventHandler( "EndGameTurn", self.onEndGameTurn )
		#self.customEM.removeEventHandler( 'OnLoad', self.onGameLoad )
		#self.customEM.removeEventHandler( 'GameStart', self.onGameStart )
		self.customEM.removeEventHandler( 'BeginPlayerTurn', self.onBeginPlayerTurn )
		self.customEM.removeEventHandler( 'EndPlayerTurn', self.onEndPlayerTurn )
		self.customEM.removeEventHandler( 'victory', self.onVictory )
		self.customEM.addEventHandler( 'AutoPlayComplete', self.onAutoPlayComplete ) # Erik (BM1)

		self.customEM.setPopupHandler( 7050, ["toAIChooserPopup",self.blankHandler,self.blankHandler] )
		self.customEM.setPopupHandler( 7052, ["pickHumanPopup",self.blankHandler,self.blankHandler] )
		
		if( self.blockPopups ) :
			self.customEM.removeEventHandler( "cityBuilt", self.onCityBuilt )
			self.customEM.addEventHandler( "cityBuilt", self.customEM.onCityBuilt )
			
			self.customEM.removeEventHandler( "BeginGameTurn", self.onBeginGameTurn )
			self.customEM.addEventHandler( "BeginGameTurn", self.customEM.onBeginGameTurn )
	
	def blankHandler( self, playerID, netUserData, popupReturn ) :
		# Dummy handler to take the second event for popup
		return

	# advc: Disable unused handlers
	#def onGameStart( self, argsList ) :
	#	self.onGameLoad([])

	#def onGameLoad( self, argsList ) :
	#	# Init things which require a game object or other game data to exist
	#	pass

	#def onEndGameTurn( self, argsList ) :
	#	if( game.getAIAutoPlay() == 1 ) :
	#		# About to turn off automation
	#		pass

	def onVictory( self, argsList ) :
		self.checkPlayer()
		game.setAIAutoPlay(0)

	def pickHumanHandler( self, iPlayerID, netUserData, popupReturn ) :

		CvUtil.pyPrint('Handling pick human popup')

		if( popupReturn.getButtonClicked() == 0 ):  # if you pressed cancel
			CyInterface().addImmediateMessage("Kill your remaining units if you'd like to see end game screens","")
			return

		toKillPlayer = gc.getActivePlayer()

		newHumanIdx = popupReturn.getSelectedPullDownValue( 1 )
		newPlayer = gc.getPlayer(newHumanIdx)

		# game.setActivePlayer( newHumanIdx, False )
		# newPlayer.setIsHuman(True)

		# CvUtil.pyPrint("You now control the %s"%(newPlayer.getCivilizationDescription(0)))
		# CyInterface().addImmediateMessage("You now control the %s"%(newPlayer.getCivilizationDescription(0)),"")
		
		ChangePlayer.changeHuman( newHumanIdx, toKillPlayer.getID() )

		if( toKillPlayer.getNumCities() == 0 ) :
			# Kills off the lion in the ice field
			CvUtil.pyPrint("Killing off player %d"%(toKillPlayer.getID()))
			toKillPlayer.killUnits()
			toKillPlayer.setIsHuman(False)
			#success = game.changePlayer( toKillPlayer.getID(), toKillPlayer.getCivilizationType(), toKillPlayer.getLeaderType(), -1, False, False )
			#toKillPlayer.setNewPlayerAlive(False)
			#toKillPlayer.setFoundedFirstCity(True)

	def onBeginPlayerTurn( self, argsList ) :
		iGameTurn, iPlayer = argsList

		if( game.getAIAutoPlay() == 1 and iPlayer > game.getActivePlayer() and gc.getActivePlayer().isAlive() ) :
			# Forces isHuman checks to come through positive for everything after human players turn
			# <advc.127> Commented out; return instead. (The setAIAutoPlay
			# call might be superfluous in any case.)
			#self.checkPlayer()
			#game.setAIAutoPlay(0)
			return # </advc.127>
		
		elif( self.bSaveAllDeaths ) :
			if( game.getAIAutoPlay() == 0 and not gc.getActivePlayer().isAlive() and iPlayer > game.getActivePlayer() ) :
				self.checkPlayer()
				game.setAIAutoPlay(0)

	def onEndPlayerTurn( self, argsList ) :
		iGameTurn, iPlayer = argsList

		# Can't use isHuman as isHuman has been deactivated by automation
		if( self.refortify and iPlayer == game.getActivePlayer() and game.getAIAutoPlay() == 1 ) :
			doRefortify( game.getActivePlayer() )
		#if (iPlayer == gc.getBARBARIAN_PLAYER() and game.getAIAutoPlay() == 1):
		# <advc.127> ^That only works if there's a human in slot 0.
		turnsLeft = game.getAIAutoPlay()
		if turnsLeft > 1:
			return
		# Find the closest player preceding the disabled human in the turn order
		# (normally the barbarians)
		disabledHuman = -1
		for x in range(gc.getMAX_CIV_PLAYERS()):
			if gc.getPlayer(x).isHumanDisabled():
				disabledHuman = x
				break
		if disabledHuman == -1:
			return
		preceding = iPlayer
		turnsLeftTarget = 0
		m = gc.getMAX_PLAYERS()
		for x in range(disabledHuman - 1, disabledHuman - m, -1):
			y = x % m
			if gc.getPlayer(y).isAlive():
				preceding = y
				break
		if preceding > disabledHuman:
			# This wrap-around check doesn't work correctly with simultaneous turns; I guess b/c multiple players execute it.
			if not game.isMPOption(MultiplayerOptionTypes.MPOPTION_SIMULTANEOUS_TURNS):
				turnsLeftTarget = 1
		if turnsLeft <= turnsLeftTarget and iPlayer == preceding:
			# </advc.127>
			# About to turn off automation
			self.checkPlayer() # advc.127: Un-commented this line.

	def checkPlayer( self ) :
		
		pPlayer = gc.getActivePlayer()
		# advc.127: Unit check added to avoid multiple popups. Note however: Normally, CvPlayer::verifyAlive automatically transfers human control to a different player when the active player is defeated. The popup here remains only as a fallback. Also added not-human check - should obviously only affect AI Auto Play. Don't know if and how that was working previously.
		if not pPlayer.isAlive() and pPlayer.getNumUnits() <= 0 and not pPlayer.isHuman():
			popup = PyPopup.PyPopup(7052,contextType = EventContextTypes.EVENTCONTEXT_ALL)
			popup.setHeaderString( localText.getText("TXT_KEY_AIAUTOPLAY_PICK_CIV", ()) )
			popup.setBodyString( localText.getText("TXT_KEY_AIAUTOPLAY_CIV_DIED", ()) )
			popup.addSeparator()

			popup.createPythonPullDown( localText.getText("TXT_KEY_AIAUTOPLAY_TAKE_CONTROL_CIV", ()), 1 )
			for i in range(0,gc.getMAX_CIV_PLAYERS()) :
				player = PyPlayer(i)
				if( not player.isNone() and not i == pPlayer.getID() ) :
					if( player.isAlive() ) :
						popup.addPullDownString( localText.getText("TXT_KEY_AIAUTOPLAY_OF_THE", ())%(player.getName(),player.getCivilizationName()), i, 1 )

			activePlayerIdx = gc.getActivePlayer().getID()
			popup.popup.setSelectedPulldownID( activePlayerIdx, 1 )

			popup.addSeparator()

			popup.addButton( localText.getText("TXT_KEY_AIAUTOPLAY_NONE", ()) )
			CvUtil.pyPrint('Launching pick human popup')
			popup.launch()

			#gc.getActivePlayer().setNewPlayerAlive( True )
			iSettler = CvUtil.findInfoTypeNum(gc.getUnitInfo,gc.getNumUnitInfos(),'UNIT_SETTLER')
			gc.getActivePlayer().initUnit( iSettler, 0, 0, UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH )
			#gc.getActivePlayer().setFoundedFirstCity( False )
			gc.getActivePlayer().setIsHuman( True )

		CvUtil.pyPrint('CDP: Setting autoplay to 0')
		# advc.127 (comment): Re-enables humans
		game.setAIAutoPlay(0)
		
		if( not pPlayer.isHuman() ) :
			CvUtil.pyPrint('Returning human player to control of %s'%(pPlayer.getCivilizationDescription(0)))
			game.setActivePlayer( pPlayer.getID(), False )
			pPlayer.setIsHuman( True )

		#for idx in range(0,gc.getMAX_CIV_TEAMS()) :
		#	pPlayer.setEspionageSpendingWeightAgainstTeam(idx, pPlayer.getEspionageSpendingWeightAgainstTeam(idx)/10)


	def onKbdEvent( self, argsList ) :
		'keypress handler'
		eventType,key,mx,my,px,py = argsList
		# <advc.001>
		if not self.customEM.isCheatsEnabled():
			return # </advc.001>
		# Get it?  Shift ... control ... to the AI
		if eventType != 6 or not self.customEM.bShift or not self.customEM.bCtrl:
			return # advc
		self.bBenchmark = False # advc (for BM1)
		theKey=int(key)
		# advc: B for benchmark added (for BM1)
		if theKey == int(InputTypes.KB_X) or theKey == int(InputTypes.KB_B):
			if game.getAIAutoPlay() > 0:
				if self.refortify:
					doRefortify(game.getActivePlayer())
				game.setAIAutoPlay(0)
				self.checkPlayer()
			else:
				# <advc> for BM1
				if theKey == int(InputTypes.KB_B):
					self.bBenchmark = True # </advc>
				self.toAIChooser()
		# advc.127: Disable this feature
		#if theKey == int(InputTypes.KB_M):
			# Toggle auto moves
		#	if self.LOG_DEBUG:
		#		CyInterface().addImmediateMessage("Moving your units...","")
		#	game.setAIAutoPlay(1)
		if theKey == int(InputTypes.KB_O):
			doRefortify( game.getActivePlayer() )


	def onBeginGameTurn( self, argsList):
		'Called at the beginning of the end of each turn'
		iGameTurn = argsList[0]
#		if( game.getAIAutoPlay() == 0 ) :
#			CvTopCivs.CvTopCivs().turnChecker(iGameTurn)
		# K-Mod. This should be handled by the original onBeginGameTurn event.
		if (game.getAIAutoPlay() == 0):
			self.customEM.onBeginGameTurn(argsList)
		# K-Mod end

	def onCityBuilt(self, argsList):
		'City Built'
		city = argsList[0]
		#if (city.getOwner() == CyGame().getActivePlayer() and game.getAIAutoPlay() == 0 ):
		# K-Mod. Some mods may use the event for other player's cities too.
		if (game.getAIAutoPlay() == 0):
			self.customEM.onCityBuilt(argsList)
		else:
			try :
				CvUtil.pyPrint('City Built Event: %s' %(city.getName()))
			except :
				CvUtil.pyPrint('City Built Event: Error processing city name' )


	def toAIChooser( self ) :
		'Chooser window for when user switches to AI auto play'

		screen = CyGInterfaceScreen( "MainInterface", CvScreenEnums.MAIN_INTERFACE )
		xResolution = screen.getXResolution()
		yResolution = screen.getYResolution()
		popupSizeX = 400
		popupSizeY = 250
		# <advc> Based on Erik's code (BM1)
		if self.bBenchmark:
			popupSizeX += 100
			popupSizeY += 80
		# </advc>
		popup = PyPopup.PyPopup(7050,contextType = EventContextTypes.EVENTCONTEXT_ALL)
		popup.setPosition((xResolution - popupSizeX )/2, (yResolution-popupSizeY)/2-50)
		popup.setSize(popupSizeX,popupSizeY)
		popup.setHeaderString( localText.getText("TXT_KEY_AIAUTOPLAY_TURN_ON", ()) )
		popup.setBodyString( localText.getText("TXT_KEY_AIAUTOPLAY_TURNS", ()) )
		popup.addSeparator()
		popup.createPythonEditBox( '%d'%(self.DefaultTurnsToAuto), 'Number of turns to turn over to AI', 0)
		popup.setEditBoxMaxCharCount( 4, 2, 0 )
		if self.bBenchmark: # advc
			# Erik <BM1>
			popup.addSeparator()
			# In-game benchmark RNG seed input
			popup.setBodyString(localText.getText("Enter seed (or leave empty to use the existing seed)", ()) )
			popup.createPythonEditBox('', 'RNG seed', 1)
			# </BM1>
		popup.addSeparator()
		popup.addButton("OK")
		popup.addButton(localText.getText("TXT_KEY_AIAUTOPLAY_CANCEL", ()))

		popup.launch(False, PopupStates.POPUPSTATE_IMMEDIATE)

	def AIChooserHandler( self, playerID, netUserData, popupReturn ) :
		'Handles AIChooser popup'
		if( popupReturn.getButtonClicked() == 1 ):  # if you pressed cancel
			return

		self.numTurns = 0
		if( popupReturn.getEditBoxString(0) != '' ) :
			self.numTurns = int( popupReturn.getEditBoxString(0) )
		# Erik <BM1>
			if self.bBenchmark: # advc
				self.benchmarkStartTime = time.clock()
		if self.bBenchmark: # advc
			if( popupReturn.getEditBoxString(1) != '' ) :
				seed = int(popupReturn.getEditBoxString(1))
				# advc: CvRandom::reseed isn't exposed to Python, but init does the same thing.
				gc.getGame().getSorenRand().init(seed)
		# </BM1>
		if( self.numTurns > 0 ) :
			if( self.LOG_DEBUG ) : CyInterface().addImmediateMessage("Fully automating for %d turns"%(self.numTurns),"")
			game.setAIAutoPlay(self.numTurns)
	# Erik <BM1>
	def onAutoPlayComplete(self, argsList):
		# <advc>
		if not self.bBenchmark:
			return # </advc>
		benchmarkEndTime = time.clock()
		timeElapsed = benchmarkEndTime - self.benchmarkStartTime	
		popupInfo = PyPopup.PyPopup(game.getActivePlayer())
		popupInfo.setHeaderString('Benchmark complete!') # advc: Put this in the header
		popupInfo.setBodyString('Elapsed time: ' + str(timeElapsed) + ' seconds. turns: ' + str(self.numTurns))	
		popupInfo.launch(true, PopupStates.POPUPSTATE_QUEUED)
	# </BM1>

########################## Utility functions ###########################################
			
def doRefortify( iPlayer ) :
	 #pyPlayer = PyPlayer( iPlayer )
	pPlayer = gc.getPlayer(iPlayer)
	
	CvUtil.pyPrint( "Refortifying units for player %d"%(iPlayer))

	for groupID in range(0,pPlayer.getNumSelectionGroups()) :
		pGroup = pPlayer.getSelectionGroup(groupID)
		if( pGroup.getNumUnits() > 0 ) :

			headUnit = pGroup.getHeadUnit()
			#CvUtil.pyPrint( "%s fortTurns %d"%(headUnit.getName(),headUnit.getFortifyTurns()) )
			if( headUnit.getFortifyTurns() > 0 and headUnit.getDomainType() == DomainTypes.DOMAIN_LAND ) :
				if( headUnit.isHurt() ) :
					#CvUtil.pyPrint( "%s is hurt"%(headUnit.getName()) )
					#pGroup.setActivityType(ActivityTypes.ACTIVITY_HEAL)
					pass
				else :
					#CvUtil.pyPrint( "Starting mission ..." )
					#pGroup.pushMission( MissionTypes.MISSION_FORTIFY, 0, 0, 0, False, True, MissionAITypes.MISSIONAI_GUARD_CITY, pGroup.plot(), pGroup.getHeadUnit() )
					pGroup.setActivityType(ActivityTypes.ACTIVITY_SLEEP)
					headUnit.NotifyEntity( MissionTypes.MISSION_FORTIFY )
					pass
