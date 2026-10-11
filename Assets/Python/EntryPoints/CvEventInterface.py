# Sid Meier's Civilization 4
# Copyright Firaxis Games 2005
#
# CvEventInterface.py
#
# These functions are App Entry Points from C++
# WARNING: These function names should not be changed
# WARNING: These functions can not be placed into a class
#
# No other modules should import this

# **********************************
# GJD modifications start here
# **********************************

import BugEventManager
import SASDefineGuard
from CvPythonExtensions import CyGlobalContext, CyGame # <!-- custom: Gate and dispatch the session diagnostic observation only after Python lifecycle handlers finalize BUG/Autolog state. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->

eventManager = BugEventManager.BugEventManager()

def getEventManager():
	return eventManager

# **********************************
# GJD modifications end here
# **********************************

def onEvent(argsList):
	# Called when a game event happens - return 1 if the event was consumed.
	#
	tag = argsList[0]
	# <!-- custom: Reuse only the event classification; keep it before the logging lookup so ordinary events never query the Define or construct a CyGame wrapper. (GPT-6.1-Sol) -->
	bSessionEvent = (tag == "OnLoad" or tag == "GameStart")
	if bSessionEvent:
		SASDefineGuard.verify_or_raise("CvEventInterface.onEvent[%s]" % tag)
	result = getEventManager().handleEvent(argsList)
	# <!-- custom: Autolog resets LoggingOn inside GameStart/OnLoad handlers; recording it from the earlier native session header reported pre-handler state.
	# Emit once after all handlers return, with a caller gate before entering the DLL logger. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	if bSessionEvent and CyGlobalContext().getDefineINT("SAS_GAME_RECORD_LOG_LEVEL") > 0:
		CyGame().logSASGameRecordDiagnosticContext()
	return result

def applyEvent(argsList):
	context, playerID, netUserData, popupReturn = argsList
	return getEventManager().applyEvent(argsList)

def beginEvent(context, argsList = -1):
	return getEventManager().beginEvent(context, argsList)

def initAfterReload():
	# Initialize BUG and fires PythonReloaded event after reloading Python modules while game is still running.
	#
	# The first time this module is loaded after the game launches, the global context is not yet ready, and thus BUG cannot be initialized. When the Python modules are reloaded after being changed, however, this will reinitialize BUG and the main interface.
	#
	import BugInit
	import BugPath
	if not BugPath.isMac() and BugInit.init():
		try:
			import CvScreensInterface
			CvScreensInterface.reinitMainInterface()
		except:
			import BugUtil
			BugUtil.error("BugInit - failure rebuilding main interface after reloading Python modules")
		getEventManager().fireEvent("PythonReloaded")

initAfterReload()
