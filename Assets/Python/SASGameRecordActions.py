# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: Keep revision-151 Python UI tracing tiny and factual: fixed screen/action labels plus scalar state only.
# Free-text search, names, paths, mouse movement, redraws, and other high-volume/private input deliberately stay outside SASGameRecord. (ChatGPT-5.6-Sol) -->

from CvPythonExtensions import CyGame
import SASGameRecordConfig

# <!-- custom: Python 2.4 has no standard Enum class; shared named constants keep action labels consistent and make misspelled attribute references fail visibly.
# These are existing log tokens, not new numeric IDs or a separate runtime registry. (GPT-6.1-Sol) -->
SHOW_REQUEST = "SHOW_REQUEST"
SHOW = "SHOW"
HIDE = "HIDE"
NAVIGATE = "NAVIGATE"
BACK = "BACK"
FORWARD = "FORWARD"
CLOSE_CALLBACK = "CLOSE_CALLBACK"
SCROLL_UP = "SCROLL_UP"
SCROLL_DOWN = "SCROLL_DOWN"
SCROLL_UP_FAST = "SCROLL_UP_FAST"
SCROLL_DOWN_FAST = "SCROLL_DOWN_FAST"
CENTER_ACTIVE_PLAYER = "CENTER_ACTIVE_PLAYER"
AUTOCENTER_TOGGLE = "AUTOCENTER_TOGGLE"
BACKGROUND_STYLE_CYCLE = "BACKGROUND_STYLE_CYCLE"
EXPAND_LOCK_TOGGLE = "EXPAND_LOCK_TOGGLE"
VISIBILITY_TOGGLE = "VISIBILITY_TOGGLE"

# <!-- custom: Screen/component tokens are diagnostic labels, not new Civ4 screen IDs; retain engine screen IDs separately in the numeric destination fields. (GPT-6.1-Sol) -->
SCREEN_BUG_OPTIONS = "BUG_OPTIONS"
SCREEN_DOMESTIC_ADVISOR = "DOMESTIC_ADVISOR"
SCREEN_FOREIGN_ADVISOR = "FOREIGN_ADVISOR"
SCREEN_FOREIGN_DIPLOMACY_ADVISOR = "FOREIGN_DIPLOMACY_ADVISOR"
SCREEN_INFO_SCREEN = "INFO_SCREEN"
SCREEN_MAP_ANNOTATIONS = "MAP_ANNOTATIONS"
SCREEN_MILITARY_ADVISOR = "MILITARY_ADVISOR"
SCREEN_OPTIONS = "OPTIONS"
SCREEN_POLICY_ADVISOR = "POLICY_ADVISOR"
SCREEN_PYTHON_SCREEN = "PYTHON_SCREEN"
SCREEN_SCOREBOARD = "SCOREBOARD"
SCREEN_SEVOPEDIA = "SEVOPEDIA"
SCREEN_TECH_CHOOSER = "TECH_CHOOSER"
SCREEN_VICTORY_SCREEN = "VICTORY_SCREEN"
SCREEN_WORLD_ADVISOR = "WORLD_ADVISOR"
SCREEN_WORLD_BUILDER = "WORLD_BUILDER"
SCREEN_WORLD_BUILDER_DIPLOMACY = "WORLD_BUILDER_DIPLOMACY"

# <!-- custom: Advance only at explicit new/load/reload boundaries; persistent screen objects must not suppress a later session's baseline.
# Zero also keeps disabled redraws on their cheap integer-comparison path. (GPT-6.1-Sol) -->
g_sessionSerial = 0


def resetSession():
	global g_sessionSerial
	if SASGameRecordConfig.getLogLevel() >= 2 and SASGameRecordConfig.getActionTraceEnabled():
		g_sessionSerial += 1


def isEnabled():
	return SASGameRecordConfig.getLogLevel() >= 2 and SASGameRecordConfig.getActionTraceEnabled() and CyGame().isFinalInitialized()


# <!-- custom: Check the cached detail level and action-trace subject switch before constructing the native wrapper; enabled emitters reuse that one wrapper for the initialization check and output instead of constructing it twice. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
def _getEnabledGame():
	if SASGameRecordConfig.getLogLevel() < 2 or not SASGameRecordConfig.getActionTraceEnabled():
		return None
	game = CyGame()
	if not game.isFinalInitialized():
		return None
	return game


def logScreenAction(szScreen, szAction, iPrimary=-1, iSecondary=-1):
	game = _getEnabledGame()
	if game is None:
		return
	game.logSASGameRecordScreenAction(szScreen, szAction, iPrimary, iSecondary)


def _scoreboardState(iScrollOffset, iBgStyle, bAlwaysExpand, bAutocenter):
	# <!-- custom: Conditional expressions in the first draft require Python 2.5; Civ4 uses Python 2.4.
	# Explicit branches preserve unresolved None values as -1. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	if iBgStyle is None:
		iStyle = -1
	else:
		iStyle = int(iBgStyle)
	if bAlwaysExpand is None:
		iExpand = -1
	else:
		iExpand = int(bool(bAlwaysExpand))
	if bAutocenter is None:
		iCenter = -1
	else:
		iCenter = int(bool(bAutocenter))
	return (int(iScrollOffset), iStyle, iExpand, iCenter)


def logScoreboardSnapshot(iScrollOffset, iBgStyle, bAlwaysExpand, bAutocenter):
	game = _getEnabledGame()
	if game is None:
		return False
	state = _scoreboardState(iScrollOffset, iBgStyle, bAlwaysExpand, bAutocenter)
	szValues = "scrollOffset=%d;bgStyle=%d;alwaysExpand=%d;autocenter=%d" % state
	game.logSASGameRecordInterfaceState(SCREEN_SCOREBOARD, "SNAPSHOT", "FIRST_DRAW", 4, szValues)
	return True


def captureScoreboardState(iScrollOffset, iBgStyle, bAlwaysExpand, bAutocenter):
	if not isEnabled():
		return None
	return _scoreboardState(iScrollOffset, iBgStyle, bAlwaysExpand, bAutocenter)


def logScoreboardChange(szReason, before, iScrollOffset, iBgStyle, bAlwaysExpand, bAutocenter):
	if before is None:
		return
	game = _getEnabledGame()
	if game is None:
		return
	after = _scoreboardState(iScrollOffset, iBgStyle, bAlwaysExpand, bAutocenter)
	if before == after:
		return
	names = ("scrollOffset", "bgStyle", "alwaysExpand", "autocenter")
	changes = []
	for i in range(len(names)):
		if before[i] != after[i]:
			changes.append("%s=%d>%d" % (names[i], before[i], after[i]))
	game.logSASGameRecordInterfaceState(SCREEN_SCOREBOARD, "CHANGE", szReason, len(changes), ";".join(changes))


def logBooleanInterfaceChange(szComponent, szReason, szKey, bOldValue, bNewValue):
	game = _getEnabledGame()
	if game is None:
		return
	bOld = bool(bOldValue)
	bNew = bool(bNewValue)
	if bOld == bNew:
		return
	szValues = "%s=%d>%d" % (szKey, int(bOld), int(bNew))
	game.logSASGameRecordInterfaceState(szComponent, "CHANGE", szReason, 1, szValues)
