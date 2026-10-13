# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS root README.md)

# <!-- custom: Keep Python caller gates lightweight without importing the settings collectors or repeatedly crossing into XML define lookup.
# Start at None: modules can load before XML is ready, so resolve only when a runtime/session callback first needs the level.
# Like the DLL's fixed loaded-define caches, these values remain constant for the Python session; restart/reload the mod after XML edits. (GPT-6.1-Sol) -->
from CvPythonExtensions import CyGlobalContext

_g_logLevel = None
_g_systemContextLevel = None


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
