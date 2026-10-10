# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md).
#
# <!-- custom: Purpose: Python reads defines through CyGlobalContext (DLL runtime define table), not by parsing XML directly.
# Our DLL loads xml\GlobalDefines_advciv_sas.xml in CvXMLLoadUtility::SetGlobalDefines.
# If that file is not loaded (wrong mod/DLL/load path), these keys resolve to defaults/missing values instead of our sentinels, so this guard fails fast and surfaces the launch/config issue. (GPT-5.3-Codex) -->

from CvPythonExtensions import CyGlobalContext

gc = CyGlobalContext()

SAS_LAUNCH_GUARD_TEST_INT_DEFINE = "SAS_LAUNCH_GUARD_TEST_INT"
SAS_LAUNCH_GUARD_TEST_STRING_DEFINE = "SAS_LAUNCH_GUARD_TEST_STRING"
SAS_LAUNCH_GUARD_EXPECTED_INT = 137531
SAS_LAUNCH_GUARD_EXPECTED_STRING = "AdvCiv-SAS defines test string"

def _raise_guard_error(szContext, szDetail):
	raise RuntimeError("Missing or incorrect SAS defines. This could indicate a mod configuration error. Make sure you start this mod from a Windows shortcut. See the Quick Install Guide documentation. Context: %s. %s" % (szContext, szDetail))

def verify_or_raise(szContext):
	# <!-- custom: launch guard to detect unexpected define state (e.g., missing defines) and fail fast with explicit context. (GPT-5.3-Codex) -->
	iValue = gc.getDefineINT(SAS_LAUNCH_GUARD_TEST_INT_DEFINE)
	szValue = gc.getDefineSTRING(SAS_LAUNCH_GUARD_TEST_STRING_DEFINE)

	if iValue != SAS_LAUNCH_GUARD_EXPECTED_INT:
		_raise_guard_error(szContext, "%s actual=%d expected=%d" % (SAS_LAUNCH_GUARD_TEST_INT_DEFINE, iValue, SAS_LAUNCH_GUARD_EXPECTED_INT))

	if szValue != SAS_LAUNCH_GUARD_EXPECTED_STRING:
		_raise_guard_error(szContext, "%s actual='%s' expected='%s'" % (SAS_LAUNCH_GUARD_TEST_STRING_DEFINE, szValue, SAS_LAUNCH_GUARD_EXPECTED_STRING))
