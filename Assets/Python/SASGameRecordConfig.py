# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS root README.md)

# <!-- custom: Keep Python caller gates lightweight without importing the settings collectors or repeatedly crossing into XML define lookup.
# Start at None: modules can load before XML is ready, so resolve only when a runtime/session callback first needs the level.
# Like the DLL's fixed loaded-define caches, these values remain constant for the Python session; revision 151 also caches the action-trace subject switch here so disabled UI hooks avoid repeated XML lookups.
# Restart/reload the mod after XML edits. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
from CvPythonExtensions import CyGlobalContext

_g_logLevel = None
_g_systemContextLevel = None
_g_actionTraceEnabled = None


def getLogLevel():
	global _g_logLevel
	if _g_logLevel is None:
		_g_logLevel = max(0, min(3, CyGlobalContext().getDefineINT("SAS_GAME_RECORD_LOG_LEVEL")))
	return _g_logLevel


def getSystemContextLevel():
	global _g_systemContextLevel
	if _g_systemContextLevel is None:
		_g_systemContextLevel = max(0, min(3, CyGlobalContext().getDefineINT("SAS_GAME_RECORD_SYSTEM_CONTEXT_LEVEL")))
	return _g_systemContextLevel


# <!-- custom: Keep the action/interface subject switch independent from detail level so users can retain level-2/3 gameplay diagnostics without UI-action noise. (ChatGPT-5.6-Sol) -->
def getActionTraceEnabled():
	global _g_actionTraceEnabled
	if _g_actionTraceEnabled is None:
		_g_actionTraceEnabled = (CyGlobalContext().getDefineINT("SAS_GAME_RECORD_ACTION_TRACE_ENABLE") > 0)
	return _g_actionTraceEnabled
