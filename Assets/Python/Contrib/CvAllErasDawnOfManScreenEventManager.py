## All Eras Dawn Of Man Screen Mod by Jeckel.
##
## Displays the Dawn of Man screen regardless of which era you start in.
##
## Notes
##  - Must be initialized externally by calling init()
##  - Rewritten to demonstrate a simple no-class event handling mod.

from CvPythonExtensions import *

START_YEAR = None

gc = CyGlobalContext()

def init():
	global START_YEAR
	START_YEAR = gc.getDefineINT("START_YEAR")

def onGameStart(argsList):
	# advc.250c: Show DoM also when human starts advanced - actually always
	# show DoM on game start, but for ordinary games, CvEventManager.py
	# already show it.
	# advc.704: RiseFall isn't initialized onGameStart; show the screen from the DLL instead
	if (gc.getGame().getGameTurnYear() != START_YEAR or (gc.getGame().isOption(GameOptionTypes.GAMEOPTION_ADVANCED_START) and not gc.getGame().isOption(GameOptionTypes.GAMEOPTION_SPAH))) and not gc.getGame().isOption(GameOptionTypes.GAMEOPTION_RISE_FALL):
		if (gc.getGame().getGameTurn() == gc.getGame().getStartTurn()) or (gc.getGame().countNumHumanGameTurnActive() == 0):
			for iPlayer in range(gc.getMAX_PLAYERS()):
				pPlayer = gc.getPlayer(iPlayer)
				if (pPlayer.isAlive()) and (pPlayer.isHuman()):
					popupInfo = CyPopupInfo()
					popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON_SCREEN)
					popupInfo.setText(u"showDawnOfMan")
					popupInfo.addPopup(iPlayer)
