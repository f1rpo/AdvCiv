from SAS_WorldSizeUtils import *

# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: Playable simple flat-grass sandbox wrapper for the compact almost-all-land profile in Assets/Python/SAS_WorldSizeUtils.py.
# Keep reusable helper logic out of this PrivateMaps file: Civ4 treats top-level functions here as map-script callbacks, and the old helper-map/import-star design leaked this file's compact getGridSize into unrelated scripts.
# RandomScriptMap Huge was empirically too small (60 x 44) until the split restored 120 x 84, visually matching base AdvCiv Huge at a glance; the same leak likely affected other scripts such as Archipelago. (GPT-5.5?) -->

def getDescription():
	return "TXT_KEY_MAP_SCRIPT_SAS_SIMPLE_FLAT_GRASS_DESCR"

# <!-- custom: Use compact almost-all-land sizing to avoid oversized flat-grass starts. (GPT-5.3-Codex + GPT-5.5?) -->
def getGridSize(argsList):
	if (argsList[0] == -1): # (-1,) is passed to function on loads
		return []
	[eWorldSize] = argsList
	return sas_get_compact_almost_all_land_grid_size(eWorldSize)
