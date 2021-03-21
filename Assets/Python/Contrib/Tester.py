# Tester.py
#
# by jdog5000
#
# Provides event debug output and other testing faculties

from CvPythonExtensions import *
import CvUtil
import sys
import Popup as PyPopup
from PyHelpers import PyPlayer, PyInfo
import CvEventManager


# globals
gc = CyGlobalContext()
game = CyGame()
localText = CyTranslator()

class Tester :

	def __init__(self, customEM):

		print "Initializing Tester"

		self.LOG_DEBUG = True

		customEM.addEventHandler( "kbdEvent", self.onKbdEvent )

		self.customEM = customEM

	def removeEventHandlers( self ) :
		print "Removing event handlers from Tester"
		
		self.customEM.removeEventHandler( "kbdEvent", self.onKbdEvent )

	
	def blankHandler( self, playerID, netUserData, popupReturn ) :
		# Dummy handler to take the second event for popup
		return


	def onKbdEvent(self, argsList ):
		'keypress handler'
		eventType,key,mx,my,px,py = argsList
		# <advc.001>
		if not self.customEM.isCheatsEnabled():
			return # </advc.001>
		if ( eventType == 6 ):
			theKey=int(key)
				
			if( theKey == int(InputTypes.KB_S) and self.customEM.bShift and self.customEM.bCtrl ) :
				self.showStrandedPopup()


	def showStrandedPopup( self ) :

		bodStr = "Stranded units by player:\n"
		for iPlayer in range(0,gc.getMAX_PLAYERS()) :
			pPlayer = gc.getPlayer(iPlayer)
			
			if( pPlayer.isAlive() ) :
				
				bodStr += "\n\n%d: %s"%(iPlayer, pPlayer.getCivilizationDescription(0))
				
				unitList = PyPlayer(iPlayer).getUnitList()
				
				for pUnit in unitList :
					
					pGroup = pUnit.getGroup()
					if( pGroup.getHeadUnit().getID() == pUnit.getID() ) :
						if( pGroup.isStranded() ) :
							bodStr += "\n   %s (%d units) at (%d, %d)"%(pUnit.getName(),pGroup.getNumUnits(),pUnit.getX(),pUnit.getY())
		
		popup = PyPopup.PyPopup()
		popup.setBodyString(bodStr)
		popup.launch()
