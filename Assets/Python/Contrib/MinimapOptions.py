# advc.002a: New module for minimap options. I want to let the DLL cache those; that makes them a bit more complicated than other BUG options.

from CvPythonExtensions import *
import BugCore

gc = CyGlobalContext()

def init():
	MainOpt = BugCore.game.MainInterface
	gc.getMap().setMinimapShowUnits(MainOpt.isUnitsOnMinimap())
	gc.getMap().setMinimapWaterAlpha(minimapWaterChoiceToAlpha(MainOpt.getMinimapWaterColor()))

def updateUnitsOnMinimap(option, value):
	gc.getMap().setMinimapShowUnits(value)
	gc.getMap().updateMinimapColor()

def updateMinimapWaterColor(option, value):
	gc.getMap().setMinimapWaterAlpha(minimapWaterChoiceToAlpha(value))
	gc.getMap().updateMinimapColor()

def minimapWaterChoiceToAlpha(choice):
	alpha = gc.getMap().getMinimapLandAlpha()
	if choice == 1:
		alpha /= float(2.3)
	elif choice == 2:
		alpha = 0
	return alpha
