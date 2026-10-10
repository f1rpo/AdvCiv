# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: Shared ARENA/SAS world-size helpers for map scripts.
# This used to live in the playable PrivateMaps/SAS_WorldSizes.py helper map; importing it with import-star leaked that map's compact getGridSize callback into scripts that did not define their own.
# RandomScriptMap Huge was empirically far too small (60 x 44) and became 120 x 84 after moving the helpers out of PrivateMaps, visually matching base AdvCiv Huge at a glance; the same leak likely affected other scripts such as Archipelago.
# Keep this utility outside PrivateMaps and do not add Civ4 map-script callback names here. (GPT-5.5?) -->

import math
from CvPythonExtensions import CyGlobalContext, WorldSizeTypes

SAS_SIMPLE_GAME_STALE_OPTION_WARNED = False

def sas_default_sizevalues():
	return {
		WorldSizeTypes.WORLDSIZE_ARENA:  1,
		WorldSizeTypes.WORLDSIZE_DUEL:  1,
		WorldSizeTypes.WORLDSIZE_TINY:  1,
		WorldSizeTypes.WORLDSIZE_SMALL:  1,
		WorldSizeTypes.WORLDSIZE_STANDARD:  2,
		WorldSizeTypes.WORLDSIZE_LARGE:  2,
		WorldSizeTypes.WORLDSIZE_HUGE:  3,
		WorldSizeTypes.WORLDSIZE_SAS24:  4,
		WorldSizeTypes.WORLDSIZE_SAS32:  5,
		WorldSizeTypes.WORLDSIZE_SAS40:  6,
		WorldSizeTypes.WORLDSIZE_SAS48: 7,
	}

# <!-- custom: Shared compact profile for almost-all-land maps.
# Base tiers are fixed; SAS24/32/40/48 are calibrated from the Huge XML default-player anchor so tiles-per-player stay closer to Huge while reducing spacing for underpopulated starts further.
# See also SAS_MAP_SCRIPT_NAMES_ALMOST_ALL_LAND (GPT-5.3-Codex + GPT-5.5) -->
def sas_compact_almost_all_land_grid_sizes():
	return {
		WorldSizeTypes.WORLDSIZE_ARENA:  (2, 2),
		WorldSizeTypes.WORLDSIZE_DUEL:  (3, 2),
		WorldSizeTypes.WORLDSIZE_TINY:  (4, 3),
		WorldSizeTypes.WORLDSIZE_SMALL:  (6, 4),
		WorldSizeTypes.WORLDSIZE_STANDARD:  (8, 6),
		WorldSizeTypes.WORLDSIZE_LARGE:  (11, 8),
		WorldSizeTypes.WORLDSIZE_HUGE:  (15, 11),
	}

def sas_get_compact_almost_all_land_grid_size(eWorldSize):
	return sas_lookup_world_size_with_calibrated_sas(eWorldSize, sas_compact_almost_all_land_grid_sizes())

def sas_find_world_size_key(eWorldSize, values):
	iWorldSize = int(eWorldSize)
	for key in values.keys():
		if int(key) == iWorldSize:
			return key
	return None

def sas_largest_world_size_key(values):
	bestKey = None
	for key in values.keys():
		if bestKey is None or int(key) > int(bestKey):
			bestKey = key
	return bestKey

def sas_lookup_world_size(eWorldSize, values):
	key = sas_find_world_size_key(eWorldSize, values)
	if key is not None:
		return values[key]
	return values[sas_largest_world_size_key(values)]

def sas_grid_ratio(iWidth, iHeight):
	return float(iWidth) / float(max(1, iHeight))

def sas_tiles_per_player(iWidth, iHeight, iMaxPlayers):
	return float(iWidth * iHeight) / float(max(1, iMaxPlayers))

def sas_world_default_players(iWorldSize):
	gc = CyGlobalContext()
	iWorld = int(iWorldSize)
	if iWorld >= 0 and iWorld < gc.getNumWorldInfos():
		return max(1, gc.getWorldInfo(iWorld).getDefaultPlayers())
	raise IndexError("Unknown world-size index %d; missing CIV4WorldInfo default-player entry" % iWorld)

def sas_warn_simple_game_stale_option_once(iOption, iRealCount):
	# <!-- custom: Shared one-time warning for stale Simple Game custom-option cache indices (KI#106); keep implementation minimal with a single boolean flag. (GPT-5.3-Codex) -->
	global SAS_SIMPLE_GAME_STALE_OPTION_WARNED
	if SAS_SIMPLE_GAME_STALE_OPTION_WARNED:
		return
	SAS_SIMPLE_GAME_STALE_OPTION_WARNED = True
	raise KeyError("Notice: if you started this map from Simple Game, you'll likely encounter a stale custom option index KeyError on next Simple Game launch. Workaround: press Esc key many times (or faster keep it pressed several seconds), or play Custom Game to avoid this issue entirely. See KI#106. (requested option %d, real options %d)" % (iOption, iRealCount))

# <!-- custom: Helper for map-size tuning: keep anchor ratio and tiles-per-player roughly stable when extrapolating to larger player caps.
# Intended for script calibration by maintainers. (GPT-5.3-Codex) -->
def sas_calibrate_grid_from_anchor(iAnchorWidth, iAnchorHeight, iAnchorMaxPlayers, iTargetMaxPlayers):
	fRatio = sas_grid_ratio(iAnchorWidth, iAnchorHeight)
	fTilesPerPlayer = sas_tiles_per_player(iAnchorWidth, iAnchorHeight, iAnchorMaxPlayers)
	fTargetTiles = fTilesPerPlayer * float(max(1, iTargetMaxPlayers))
	iTargetHeight = max(1, int(math.sqrt(fTargetTiles / max(0.0001, fRatio)) + 0.5))
	iTargetWidth = max(1, int((float(iTargetHeight) * fRatio) + 0.5))
	return (iTargetWidth, iTargetHeight)

# <!-- custom: For map-size calibration workflows, compute only the requested world size: base tiers return directly; SAS24/32/40/48 are calibrated on demand from the last base tier (usually Huge) and XML default-player counts.
# Fail loudly if a referenced world size is missing instead of using stale fallback player counts. (GPT-5.3-Codex + GPT-5.5) -->
def sas_lookup_world_size_with_calibrated_sas(eWorldSize, base_grid_sizes):
	iWorldSize = int(eWorldSize)
	key = sas_find_world_size_key(eWorldSize, base_grid_sizes)
	if key is not None:
		return base_grid_sizes[key]

	iAnchorWorldSize = sas_largest_world_size_key(base_grid_sizes)
	if iWorldSize > int(iAnchorWorldSize) and iWorldSize <= int(WorldSizeTypes.WORLDSIZE_SAS48):
		(iAnchorWidth, iAnchorHeight) = base_grid_sizes[iAnchorWorldSize]
		iAnchorPlayers = sas_world_default_players(iAnchorWorldSize)
		iTargetPlayers = sas_world_default_players(iWorldSize)
		return sas_calibrate_grid_from_anchor(iAnchorWidth, iAnchorHeight, iAnchorPlayers, iTargetPlayers)
	return base_grid_sizes[iAnchorWorldSize]
