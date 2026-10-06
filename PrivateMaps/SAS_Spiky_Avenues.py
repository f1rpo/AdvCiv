# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# FILE: SAS_Spiky_Avenues.py
# PURPOSE: Competitive avenue map with streets, houses (spikes), and bridges.
#
# AUTHORS: wonderingabout & GPT-5.3-Codex & Claude code Opus 4.6
#
# LAYOUT OVERVIEW:
# Picture a real-world residential street seen from above: a road with houses on
# both sides, separated by driveways. This map replicates that pattern in Civ4 tiles.
#
# Terminology (smallest to largest):
#   house  = rectangular land block where one player starts (5 tiles wide).
#   spike  = one column of two facing houses: one above the ribbon, one below.
#            So 1 spike = 2 houses = 2 player starts.
#   ribbon = the full continuous horizontal land strip spanning the entire map width.
#   avenue = the section of the ribbon between the first and last spike
#            (the shared land lane that all houses connect to).
#   bridge = the ribbon sections beyond the avenue on each side, plus vertical land
#            connectors at the left/right map edges (which wrap in X) that link
#            adjacent streets so units can walk between them in just a few tiles.
#   street = one complete unit: ribbon + its spikes (houses above and below).
#
# Streets are stacked vertically (along Y). Houses within a street are separated
# by water gaps one house wide for culture separation. The profile (streets, spikes)
# scales with world size to match default player counts.
# Example: LARGE  = 2 streets x 3 spikes =  6 houses/street, 12 total for 11 players.
#          SAS48  = 4 streets x 6 spikes = 12 houses/street, 48 total for 48 players.

from CvPythonExtensions import *
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator
from SAS_WorldSizeUtils import *

sas_spiky_layout = None
sas_slot_assignments = {}
sas_used_start_plots = set()

def getVersion():
	return "1.00"

def getDescription():
	return "Compact tactical map with repeated streets: each street has facing houses (spikes), a central avenue, and bridges on left/right edges to connect streets. One player starts per house. Bigger world sizes increase street/spike counts. Examples: in Large worldsize (11 default players), 3 spikes per street (6 houses) and 2 streets (12 houses total); in SAS48 worldsize (48 default players), 6 spikes per street (12 houses) and 4 streets (48 houses total). Recommended to play this map on SAS48. This is an AdvCiv-SAS original map."

def isAdvancedMap():
	"This map should show up in Simple Game."
	# <!-- custom: Keep visible in Simple Game; options are practical and stale-option safe. (GPT-5.3-Codex) -->
	return 0

def getNumCustomMapOptions():
	return 2

def getNumHiddenCustomMapOptions():
	return 0

def getCustomMapOptionName(argsList):
	[iOption] = argsList
	option_names = {
		0: u"Ribbon Width",
		1: u"House Size"
	}
	if not option_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
		return u"Ribbon Width"
	return option_names[iOption]

def getNumCustomMapOptionValues(argsList):
	[iOption] = argsList
	option_values = {
		0: 5,
		1: 6
	}
	if not option_values.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
		return 2
	return option_values[iOption]

def getCustomMapOptionDescAt(argsList):
	[iOption, iSelection] = argsList
	selection_names = {
		0: {
			0: u"-2",
			1: u"-1",
			2: u"Recommended",
			3: u"+1",
			4: u"+2"
		},
		1: {
			0: u"3",
			1: u"4",
			2: u"5 (Recommended)",
			3: u"6",
			4: u"7",
			5: u"8"
		}
	}
	if not selection_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
		return u"Recommended"
	if not selection_names[iOption].has_key(iSelection):
		return selection_names[iOption][0]
	return selection_names[iOption][iSelection]

def getCustomMapOptionDefault(argsList):
	[iOption] = argsList
	option_defaults = {
		0: 2,
		1: 2
	}
	if not option_defaults.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
		return 0
	return option_defaults[iOption]

def isRandomCustomMapOption(argsList):
	[iOption] = argsList
	option_random = {
		0: false,
		1: false
	}
	if not option_random.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
		return false
	return option_random[iOption]

def _get_option_value(iOption, iDefault, iNumValues):
	map_obj = CyMap()
	iValue = iDefault
	try:
		if iOption >= getNumCustomMapOptions():
			sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
			return iDefault
		iValue = map_obj.getCustomMapOption(iOption)
	except:
		return iDefault
	if iValue < 0 or iValue >= iNumValues:
		return iDefault
	return iValue

def _ribbon_width():
	iSelection = _get_option_value(0, 2, 5)
	widths = [2, 3, 4, 5, 6]
	return widths[iSelection]

def _house_size():
	iSelection = _get_option_value(1, 2, 6)
	sizes = [3, 4, 5, 6, 7, 8]
	return sizes[iSelection]

def getWrapX():
	return True

def getWrapY():
	return False

def getTopLatitude():
	return 70

def getBottomLatitude():
	return -70

def minStartingDistanceModifier():
	# <!-- custom: Dense tactical layouts need a low start-distance floor to avoid turn-0 defeats at high player counts. (GPT-5.3-Codex) -->
	return -95

def getGridSize(argsList):
	if argsList[0] == -1:
		return []

	[eWorldSize] = argsList
	(iStreets, iSpikesPerStreet) = _profile_for_world_size(eWorldSize)
	if iStreets <= 0 or iSpikesPerStreet <= 0:
		return []

	iAvenueHeight = _ribbon_width()
	iSpikeDepth = _house_size()
	iHouseWidth = _house_size()
	# <!-- custom: widen inter-house spacing to one house-width and keep it as water lanes for culture separation. (GPT-5.3-Codex) -->
	iHouseGap = iHouseWidth
	iBridgeWidth = 1
	iBridgeBuffer = 2
	# <!-- custom: keep one-house vertical spacing between streets to avoid early culture collisions. (GPT-5.3-Codex) -->
	iStreetGap = iHouseWidth
	# <!-- custom: leave at least 3 polar-edge tiles so starts near top/bottom are still usable even with ice,
	# while keeping the no-top-enemy pole advantage controlled. (GPT-5.3-Codex) -->
	iEdgeMarginY = 3

	iHouseNeedWidth = iSpikesPerStreet * iHouseWidth + (iSpikesPerStreet - 1) * iHouseGap
	iAvenueWidth = iHouseNeedWidth + 2 * iBridgeBuffer
	iWidth = iAvenueWidth + 2 * iBridgeWidth + 4
	iStreetBand = iAvenueHeight + 2 * iSpikeDepth
	iHeight = iStreets * iStreetBand + (iStreets - 1) * iStreetGap + 2 * iEdgeMarginY
	# <!-- custom: Civ4 multiplies grid-cell counts by 4 to get actual plot dimensions.
	# Return grid cells so actual map is tight around the content. (Claude code Opus 4.6) -->
	return ((iWidth + 3) / 4, (iHeight + 3) / 4)

def _profile_for_world_size(eWorldSize):
	profiles = {
		WorldSizeTypes.WORLDSIZE_ARENA: (1, 1),
		WorldSizeTypes.WORLDSIZE_DUEL: (1, 1),
		# <!-- custom: One player starts per house; Tiny and Small previously provided only 2/4 houses for their 3/5 default players. Symmetric spikes require the next even capacities, 4/6. See KI#245. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		WorldSizeTypes.WORLDSIZE_TINY: (1, 2),
		WorldSizeTypes.WORLDSIZE_SMALL: (1, 3),
		WorldSizeTypes.WORLDSIZE_STANDARD: (2, 2),
		WorldSizeTypes.WORLDSIZE_LARGE: (2, 3), # 11 players -> 3 spikes * 2 streets = 12 houses
		WorldSizeTypes.WORLDSIZE_HUGE: (2, 4), # 16 players -> 4 spikes * 2 streets = 16 houses
		WorldSizeTypes.WORLDSIZE_SAS24: (3, 4), # 24 players -> 4 spikes * 3 streets = 24 houses
		WorldSizeTypes.WORLDSIZE_SAS32: (4, 4), # 32 players -> 4 spikes * 4 streets = 32 houses
		WorldSizeTypes.WORLDSIZE_SAS40: (4, 5), # 40 players -> 5 spikes * 4 streets = 40 houses
		WorldSizeTypes.WORLDSIZE_SAS48: (4, 6), # 48 players -> 6 spikes * 4 streets = 48 houses
	}
	iWorldSize = int(eWorldSize)
	if iWorldSize in profiles:
		return profiles[iWorldSize]
	return profiles[max(profiles.keys())]

def _build_layout():
	global sas_spiky_layout
	if "sas_spiky_layout" in globals() and sas_spiky_layout is not None:
		return sas_spiky_layout

	map_obj = CyMap()
	iW = map_obj.getGridWidth()
	iH = map_obj.getGridHeight()
	(iStreets, iSpikesPerStreet) = _profile_for_world_size(map_obj.getWorldSize())

	iAvenueHeight = _ribbon_width()
	iSpikeDepth = _house_size()
	iHouseWidth = _house_size()
	iHouseGap = iHouseWidth
	iStreetGap = iHouseWidth
	iBridgeWidth = 1
	iBridgeBuffer = 2

	iStreetTopMargin = 3
	iStreetBand = iAvenueHeight + 2 * iSpikeDepth
	iTotalHeight = iStreets * iStreetBand + (iStreets - 1) * iStreetGap
	iTopBase = max(iStreetTopMargin, (iH - iTotalHeight) / 2)

	iLeftBridgeStart = 0
	iRightBridgeStart = iW - iBridgeWidth
	iAvenueLeft = iLeftBridgeStart + iBridgeWidth + 2
	iAvenueRight = iRightBridgeStart - 3

	iHouseNeedWidth = iSpikesPerStreet * iHouseWidth + (iSpikesPerStreet - 1) * iHouseGap
	iHouseZoneLeft = iAvenueLeft + iBridgeBuffer
	iHouseZoneRight = iAvenueRight - iBridgeBuffer
	iHouseZoneWidth = iHouseZoneRight - iHouseZoneLeft + 1
	if iHouseNeedWidth > iHouseZoneWidth:
		iHouseNeedWidth = iHouseZoneWidth

	iHouseStart = iHouseZoneLeft + max(0, (iHouseZoneWidth - iHouseNeedWidth) / 2)
	house_rects = []
	for i in range(iSpikesPerStreet):
		iX0 = iHouseStart + i * (iHouseWidth + iHouseGap)
		iX1 = min(iHouseZoneRight, iX0 + iHouseWidth - 1)
		iXC = (iX0 + iX1) / 2
		house_rects.append((iX0, iX1, iXC))

	start_slots = []
	street_rows = []
	for t in range(iStreets):
		iStreetTop = iTopBase + t * (iStreetBand + iStreetGap)
		iAvenueTop = iStreetTop + iSpikeDepth
		iAvenueBottom = iAvenueTop + iAvenueHeight - 1
		iStreetBottom = iStreetTop + iStreetBand - 1
		street_rows.append((iStreetTop, iAvenueTop, iAvenueBottom, iStreetBottom))
		for (_, _, iXC) in house_rects:
			# <!-- custom: one start slot per house, at center X and outer Y edge (farthest from avenue). (GPT-5.3-Codex) -->
			start_slots.append((iXC, iStreetTop))
			start_slots.append((iXC, iStreetBottom))

	sas_spiky_layout = {
		"streets": iStreets,
		"street_rows": street_rows,
		"left_bridge_start": iLeftBridgeStart,
		"right_bridge_start": iRightBridgeStart,
		"bridge_width": iBridgeWidth,
		"avenue_left": iAvenueLeft,
		"avenue_right": iAvenueRight,
		"house_rects": house_rects,
		"slots": start_slots,
	}
	return sas_spiky_layout

def beforeGeneration():
	global sas_spiky_layout, sas_slot_assignments, sas_used_start_plots
	sas_spiky_layout = None
	sas_slot_assignments = {}
	sas_used_start_plots = set()
	_build_layout()

def _is_valid_start_plot(pPlot):
	return pPlot is not None and not pPlot.isWater() and not pPlot.isImpassable()

def generatePlotTypes():
	NiTextOut("Setting Plot Types (Python SAS_Spiky_Avenues) ...")
	layout = _build_layout()
	map_obj = CyMap()
	dice = CyGlobalContext().getGame().getMapRand()
	iW = map_obj.getGridWidth()
	iH = map_obj.getGridHeight()
	plot_types = [PlotTypes.PLOT_OCEAN] * (iW * iH)

	# <!-- custom: Build every configured street fully from world-size profile, independent of alive player count. (GPT-5.3-Codex) -->
	for iStreet in range(len(layout["street_rows"])):
		(iStreetTop, iAvenueTop, iAvenueBottom, iStreetBottom) = layout["street_rows"][iStreet]
		# Main central avenue
		for x in range(layout["avenue_left"], layout["avenue_right"] + 1):
			for y in range(iAvenueTop, iAvenueBottom + 1):
				plot_types[map_obj.plotNum(x, y)] = PlotTypes.PLOT_LAND
		# Spikes: facing houses on both sides of avenue.
		for (iX0, iX1, _) in layout["house_rects"]:
			for x in range(iX0, iX1 + 1):
				for y in range(iStreetTop, iAvenueTop):
					plot_types[map_obj.plotNum(x, y)] = PlotTypes.PLOT_LAND
				for y in range(iAvenueBottom + 1, iStreetBottom + 1):
					plot_types[map_obj.plotNum(x, y)] = PlotTypes.PLOT_LAND
		# <!-- custom: keep house-gap columns as water in house bands only; preserve avenue land lanes. (GPT-5.3-Codex) -->
		if len(layout["house_rects"]) > 1:
			for i in range(len(layout["house_rects"]) - 1):
				(_, iLeftX1, _) = layout["house_rects"][i]
				(iRightX0, _, _) = layout["house_rects"][i + 1]
				for x in range(iLeftX1 + 1, iRightX0):
					for y in range(iStreetTop, iAvenueTop):
						plot_types[map_obj.plotNum(x, y)] = PlotTypes.PLOT_OCEAN
					for y in range(iAvenueBottom + 1, iStreetBottom + 1):
						plot_types[map_obj.plotNum(x, y)] = PlotTypes.PLOT_OCEAN
		# Soft avenue end-caps near bridges for smoother side movement.
		for y in range(iAvenueTop, iAvenueBottom + 1):
			plot_types[map_obj.plotNum(layout["avenue_left"] - 1, y)] = PlotTypes.PLOT_LAND
			plot_types[map_obj.plotNum(layout["avenue_right"] + 1, y)] = PlotTypes.PLOT_LAND
			if y == iAvenueTop or y == iAvenueBottom:
				plot_types[map_obj.plotNum(layout["avenue_left"] - 2, y)] = PlotTypes.PLOT_LAND
				plot_types[map_obj.plotNum(layout["avenue_right"] + 2, y)] = PlotTypes.PLOT_LAND
		# Bridge endpoints on both map edges for this street.
		for y in range(iAvenueTop, iAvenueBottom + 1):
			for x in range(layout["left_bridge_start"], layout["left_bridge_start"] + layout["bridge_width"]):
				plot_types[map_obj.plotNum(x, y)] = PlotTypes.PLOT_LAND
			for x in range(layout["right_bridge_start"], layout["right_bridge_start"] + layout["bridge_width"]):
				plot_types[map_obj.plotNum(x, y)] = PlotTypes.PLOT_LAND
		# Edge bridges connect this street to next street; movement between streets is routed via avenue edge.
		if iStreet < len(layout["street_rows"]) - 1:
			(_, nextAvenueTop, nextAvenueBottom, _) = layout["street_rows"][iStreet + 1]
			iStartY = (iAvenueTop + iAvenueBottom) / 2
			iEndY = (nextAvenueTop + nextAvenueBottom) / 2
			iConnTop = min(iStartY, iEndY)
			iConnBottom = max(iStartY, iEndY)
			for x in range(layout["left_bridge_start"], layout["left_bridge_start"] + layout["bridge_width"]):
				for y in range(iConnTop, iConnBottom + 1):
					plot_types[map_obj.plotNum(x, y)] = PlotTypes.PLOT_LAND
			for x in range(layout["right_bridge_start"], layout["right_bridge_start"] + layout["bridge_width"]):
				for y in range(iConnTop, iConnBottom + 1):
					plot_types[map_obj.plotNum(x, y)] = PlotTypes.PLOT_LAND

	# Texture: sparse hills only on land.
	for i in range(iW * iH):
		if plot_types[i] != PlotTypes.PLOT_LAND:
			continue
		iChance = dice.get(20, "SAS_Spiky_Avenues hills")
		if iChance <= 2:
			plot_types[i] = PlotTypes.PLOT_HILLS

	return plot_types

def findStartingPlot(argsList):
	global sas_slot_assignments, sas_used_start_plots
	[playerID] = argsList
	layout = _build_layout()
	map_obj = CyMap()
	gc = CyGlobalContext()

	player_ids = []
	for iPlayer in range(gc.getMAX_CIV_PLAYERS()):
		if gc.getPlayer(iPlayer).isAlive():
			player_ids.append(iPlayer)
	if len(player_ids) == 0:
		CyPythonMgr().allowDefaultImpl()
		return
	player_ids.sort()
	if playerID not in player_ids:
		CyPythonMgr().allowDefaultImpl()
		return

	if playerID in sas_slot_assignments:
		return sas_slot_assignments[playerID]

	slots = layout["slots"]
	if len(slots) == 0:
		CyPythonMgr().allowDefaultImpl()
		return

	iIndex = player_ids.index(playerID)
	if iIndex >= len(slots):
		CyPythonMgr().allowDefaultImpl()
		return

	(iX, iY) = slots[iIndex]
	pSlot = map_obj.plot(iX, iY)
	if _is_valid_start_plot(pSlot):
		iNum = map_obj.plotNum(iX, iY)
		if iNum not in sas_used_start_plots:
			sas_used_start_plots.add(iNum)
			sas_slot_assignments[playerID] = iNum
			return iNum

	# Safety search only near this house slot; never steal distant slots and never reuse an already-used start plot.
	for iRadius in range(1, 3):
		for dx in range(-iRadius, iRadius + 1):
			for dy in range(-iRadius, iRadius + 1):
				iNX = iX + dx
				iNY = iY + dy
				if iNX < 0 or iNX >= map_obj.getGridWidth() or iNY < 0 or iNY >= map_obj.getGridHeight():
					continue
				pTry = map_obj.plot(iNX, iNY)
				if not _is_valid_start_plot(pTry):
					continue
				iNum = map_obj.plotNum(iNX, iNY)
				if iNum in sas_used_start_plots:
					continue
				sas_used_start_plots.add(iNum)
				sas_slot_assignments[playerID] = iNum
				return iNum

	CyPythonMgr().allowDefaultImpl()
	return

def generateTerrainTypes():
	NiTextOut("Generating Terrain (Python SAS_Spiky_Avenues) ...")
	terrain_gen = TerrainGenerator()
	return terrain_gen.generateTerrain()

def addFeatures():
	NiTextOut("Adding Features (Python SAS_Spiky_Avenues) ...")
	feature_gen = FeatureGenerator()
	feature_gen.addFeatures()
	return 0
