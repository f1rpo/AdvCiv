## BugInit
##
## Initializes the BUG core and loads all of the mods.
##
## Called by BugEventManager during "OnLoad" and "PreGameStart" events.
##
## Copyright (c) 2008 The BUG Mod.
##
## Author: EmperorFool

from CvPythonExtensions import *
import BugConfig
import BugCore
import BugPath
import BugUtil
import sys # <!-- custom: Decode byte-valued filesystem roots using Python's filesystem encoding before the Unicode native log-path bridge. (GPT-6.1-Sol) -->

g_initRunning = False
g_initDone = False

g_initQueue = []

def init():
	# Performs the one-time initialization of the BUG core and all mods.
	#
	global g_initDone
	if g_initDone:
		BugUtil.debug("BugInit - init() already complete")
		return
	if not CyGame().isFinalInitialized():
		BugUtil.debug("BugInit - game not fully initialized")
		return
	# K-Mod. Don't use any BUG features if this is a PitBoss server.
	if CyGame().isPitbossHost():
		BugUtil.debug("BugInit - BUG disabled for PitBoss host")
		return
	# K-Mod end
	global g_initRunning
	if g_initRunning:
		BugUtil.warn("BugInit - init() already running")
		return
	g_initRunning = True

	BugUtil.debug("BugInit - initializing...")
	timer = BugUtil.Timer("BUG init")

	BugPath.init()
	# <!-- custom: Give the shared writer BUG's authoritative user root (including CvAltRoot); this one-time handoff also covers BBAI-only runs without duplicating its category gates in Python.
	# The handoff publishes queued startup diagnostics immediately; disabled diagnostics still skip row construction and file I/O. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	szRoot = BugPath.getRootDir()
	if szRoot:
		if not isinstance(szRoot, unicode):
			szRoot = szRoot.decode(sys.getfilesystemencoding())
		CyGame().setSASDiagnosticLogRoot(szRoot)
	timer.log("init paths").start()

	loadMod("init")
	BugCore.initDone()
	timer.log("read configs").start()

	callInits()
	timer.log("call inits/events")

	timer.logTotal()

	g_initDone = True
	g_initRunning = False
	return True

def loadMod(name):
	# Load the given mod from its XML file using a custom parser.
	path = BugPath.findAssetFile(name + ".xml", "Config")
	if path:
		BugUtil.debug("BugInit - loading module %s...", name)
		parser = BugConfig.ConfigParser()
		timer = BugUtil.Timer("load mod")
		try:
			parser.parse(path)
		# <advc.009b> Say which module failed
		# <!-- custom: Avoid old comma-exception binding while staying compatible with Civ4 Python 2.4; sys.exc_info()[1] was already used in BugUtil, so this pattern is deemed safe. (GPT-5.5) -->
		except Exception:
			import sys
			e = sys.exc_info()[1]
			BugUtil.error("BugInit - failed to parse module %s", name)
			timer.log(name)
			raise e # </advc.009b>
		timer.log(name)
	else:
		BugUtil.error("BugInit - cannot find XML file for module %s", name)

def addInit(name, function):
	# Calls function after all mods are loaded.
	#	
	# If all mods have been loaded, the function is called immediately.
	# Modules should use this function to setup a one-time initialization function
	# that requires an initialized CyGlobalContext.
	#	
	# name - short descriptive string used in debug messages, typically the module's name
	# function - the function to call
	#
	# Use BugUtil.getFunction() to pass arguments to your function.
	#
	g_initQueue.append((name, function))
	if g_initDone:
		callInits()

def callInits():
	# Calls all of the stored init functions in the order they were added.
	#
	BugUtil.debug("BugInit - calling init functions...")
	while g_initQueue:
		name, func = g_initQueue.pop(0)
		try:
			func()
		except:
			BugUtil.trace("BugInit - init '%s' failed" % name)
