## BugHelp
##
## Opens BUG's help file, "BUG Mod Help.chm", or the online version, for the user's language.
##
## TODO:
##   Move to configuration XML
##   Support multiple help files and shortcuts
##
## Copyright (c) 2008 The BUG Mod.
##
## Author: EmperorFool
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#

from CvPythonExtensions import *
import Popup as PyPopup
import BugPath
import BugUtil

def launch(argsList=None):
	# Opens the mod's help file or web page externally if it can be found or displays an error alert.
	#	
	# On Windows this opens the compiled HTML help file (CHM).
	# On Mac this opens a browser window to the online help file.
	#
	if BugPath.isMac():
		sLang = ["ENG", "ENG", "DEU", "ITA", "ENG"]
		url = "http://civ4bug.sourceforge.net/BUGModHelp/%s/index.htm" % sLang[CyGame().getCurrentLanguage()]
		try:
			import webbrowser
			showLaunchMessage()
			webbrowser.open(url, new=1, autoraise=1)
			return True
		except:
			showErrorAlert(BugUtil.getPlainText("TXT_KEY_BUG_HELP_CANNOT_OPEN_BROWSER_TITLE"), BugUtil.getText("TXT_KEY_BUG_HELP_CANNOT_OPEN_BROWSER_BODY", (url,)))
	else: # advc.009: There is no FRA and ESP translation
		sLang = ["ENG", "ENG", "DEU", "ITA", "ENG"]
		name = "BUG Mod Help-%s.chm" % (sLang[CyGame().getCurrentLanguage()])
		#file = BugPath.findInfoFile(name)

		file, debug = _SAS_findBugDocFile(name)
		if file:
			import os
			message = BugUtil.getPlainText("TXT_KEY_BUG_HELP_OPENING")
			CyInterface().addImmediateMessage(message, "")
			os.startfile(file)
			return True
		else:
			showErrorAlert(BugUtil.getPlainText("TXT_KEY_BUG_HELP_MISSING_TITLE"), debug + BugUtil.getText("TXT_KEY_BUG_HELP_MISSING_BODY", (name,)))
	return False

def showLaunchMessage():
	# Shows an "opening..." alert message in the event log.
	BugUtil.alert(BugUtil.getPlainText("TXT_KEY_BUG_HELP_OPENING"))

def showErrorAlert(title, body):
	# Opens a popup window showing the given error message.
	#
	popup = PyPopup.PyPopup()
	popup.setHeaderString(title)
	popup.setBodyString(body)
	popup.launch()

# <!-- custom: on Windows, the BUG Menu's help does not open for some reason ingame whe clicking on the "Bug Mod Help" button in the Bug Menu.
# Fixed by adding this with the help of chatgpt 5.2 thanks.
# Note: based on testing, it looks like cwd's path is: C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\, considering the error message we got with a faulty path if we don't add the rest of the path to our mod, and considering adding such a remaining path fixes the issue ingame and successfully allows to display the BUG Menu's help. -->
def _SAS_findBugDocFile(name):
	# Returns (fullpath_or_None, debug_string)
	try:
		import os

		debugLines = []
		try:
			cwd = os.getcwd()
		except:
			cwd = "<cwd?>"
		debugLines.append("BugHelp debug")
		debugLines.append("cwd=%s" % cwd)

		tries = []

		def addTry(p):
			if p and p not in tries:
				tries.append(p)

		# <!-- custom: 3) AdvCiv/BtS already knows the actual loaded mod directory. Use BugPath's detected directory instead of hardcoding or reconstructing the folder name from the branded display name, so renamed installs keep working. (ChatGPT-5.6-Sol) -->
		try:
			addTry(os.path.join(BugPath.getModDir(), "_0_Common_Docs", "BUG_Doc", name))
		except:
			pass
		# Decide
		for p in tries:
			try:
				if os.path.isfile(p):
					debugLines.append("FOUND=%s" % p)
					return p, "\n".join(debugLines) + "\n\n"
			except:
				pass

		# Not found: show first few attempts in the popup
		for i in range(3):
			if i < len(tries):
				debugLines.append("try%d=%s" % (i+1, tries[i]))
		return None, "\n".join(debugLines) + "\n\n"
	except:
		return None, "BugHelp debug failed completely.\n\n"
