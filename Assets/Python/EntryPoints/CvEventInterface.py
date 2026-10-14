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
from CvPythonExtensions import CyGame, CyUserProfile # <!-- custom: Shared lazy caches replace the former CyGlobalContext define reads; retain the finalized diagnostic bridge and raw graphics-profile sampling. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
import SASGameRecordConfig # <!-- custom: Share lazy None-initialized level caches across recorder callbacks; disabled runs still avoid importing the settings collectors. (GPT-6.1-Sol) -->

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
	# <!-- custom: Reset action-baseline identity before session handlers can draw persistent UI objects; revision 151 also checks the cached action subject before importing its helper.
	# Initialization checks still delay actual emission until the native session is ready. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
	if bSessionEvent and SASGameRecordConfig.getLogLevel() >= 2 and SASGameRecordConfig.getActionTraceEnabled():
		import SASGameRecordActions
		SASGameRecordActions.resetSession()
	result = getEventManager().handleEvent(argsList)
	# <!-- custom: Autolog resets LoggingOn inside GameStart/OnLoad handlers; recording it from the earlier native session header reported pre-handler state.
	# Emit once after all handlers return, with a caller gate before entering the DLL logger.
	# UserProfile owns several active graphics dropdowns that the DLL cannot query; pass their raw values plus the derived AA sample count so reports can distinguish High/Medium/Low render behavior without per-turn polling. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	if bSessionEvent:
		if SASGameRecordConfig.getLogLevel() > 0:
			iRenderQuality = -1
			iAntiAliasing = -1
			iAntiAliasingSamples = -1
			iGlobeViewRenderQuality = -1
			iMovieQuality = -1
			if SASGameRecordConfig.getSystemContextLevel() > 0:
				profile = CyUserProfile()
				iRenderQuality = profile.getRenderQualityLevel()
				iAntiAliasing = profile.getAntiAliasing()
				if iAntiAliasing == 0:
					iAntiAliasingSamples = 0
				elif iAntiAliasing > 0 and iAntiAliasing < 31:
					iAntiAliasingSamples = 1 << iAntiAliasing
				iGlobeViewRenderQuality = profile.getGlobeViewRenderLevel()
				iMovieQuality = profile.getMovieQualityLevel()
			CyGame().logSASGameRecordDiagnosticContext(iRenderQuality, iAntiAliasing, iAntiAliasingSamples, iGlobeViewRenderQuality, iMovieQuality)
			# <!-- custom: Build the broader user/configuration baseline only after session handlers settle BUG/Autolog state.
			# Lazy import plus the master caller gate means disabled runs pay no settings-module import/collection cost. (ChatGPT-5.6-Sol) -->
			import SASGameRecordSettings
			szSettingsReason = "ON_LOAD"
			if tag == "GameStart":
				szSettingsReason = "GAME_START"
			SASGameRecordSettings.logSessionSettings(szSettingsReason)
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
		# <!-- custom: A Python reload replaces the main interface within the same native session; request a fresh resolved scoreboard baseline only when the action subject is enabled, as well as the independent settings baseline below. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
		if SASGameRecordConfig.getLogLevel() >= 2 and SASGameRecordConfig.getActionTraceEnabled():
			import SASGameRecordActions
			SASGameRecordActions.resetSession()
		try:
			import CvScreensInterface
			CvScreensInterface.reinitMainInterface()
		except:
			import BugUtil
			BugUtil.error("BugInit - failure rebuilding main interface after reloading Python modules")
		getEventManager().fireEvent("PythonReloaded")
		# <!-- custom: Python reload resets/replaces module state while a game may remain active.
		# Re-establish one explicit configuration baseline so later change-only callbacks do not compare against stale/pre-reload caches. (ChatGPT-5.6-Sol) -->
		if SASGameRecordConfig.getLogLevel() > 0 and CyGame().isFinalInitialized():
			import SASGameRecordSettings
			SASGameRecordSettings.logSessionSettings("PYTHON_RELOAD")

initAfterReload()
