# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# FILE: SAS_Parallel_Lines.py
# PURPOSE: Parallel vertical-lines map (wrapX + wrapY) for symmetric multi-line play.
#
# AUTHORS: wonderingabout & GPT-5.3-Codex
# <!-- custom: inspired by SAS_Longworld geometry and startup patterns. (GPT-5.3-Codex) -->

from CvPythonExtensions import *
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator
from SAS_WorldSizeUtils import *
from SASUtils import getInfoTypeOrFail

gc = CyGlobalContext()
_sas_layout_cache = None
_sas_start_assignments = {}

def getVersion():
	return "1.00"

def getDescription():
	return "Parallel lines map where the world is optionally (default) connected horizontally (WrapX) and vertically (WrapY). Fixed number of players per line (3 or 4), and height per player. Each land line has an adjacent land line to its left and to its right, separated by a water line. Larger world sizes add more lines."

def isAdvancedMap():
	# <!-- custom: keep visible in Simple Game; options are minimal and practical. (GPT-5.3-Codex) -->
	return 0

def getNumCustomMapOptions():
	return 5

def getNumHiddenCustomMapOptions():
	return 0

def getCustomMapOptionName(argsList):
	[iOption] = argsList
	option_names = {
		0: u"Players Per Line",
		1: u"Height Per Player",
		2: u"Inter-Line Bands",
		3: u"Line Thickness",
		4: u"Revolving World"
	}
	if not option_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
		return u"Players Per Line"
	return option_names[iOption]

def getNumCustomMapOptionValues(argsList):
	[iOption] = argsList
	option_values = {
		0: 2,
		1: 9,
		2: 2,
		3: 5,
		4: 4
	}
	if not option_values.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
		return 2
	return option_values[iOption]

def getCustomMapOptionDescAt(argsList):
	[iOption, iSelection] = argsList
	# <!-- custom: keep labels explicit for screenshots/discussion where option headers may not be visible. (GPT-5.3-Codex) -->
	selection_names = {
		0: {
			0: u"3 Players Per Line (Recommended)",
			1: u"4 Players Per Line"
		},
		1: {
			0: u"7",
			1: u"8",
			2: u"9",
			3: u"10 (Recommended)",
			4: u"11",
			5: u"12",
			6: u"13",
			7: u"14",
			8: u"15"
		},
		2: {
			0: u"Coast Bands (Recommended)",
			1: u"Ocean Bands"
		},
		3: {
			0: u"3 Plots",
			1: u"4 Plots",
			2: u"5 Plots (Recommended)",
			3: u"6 Plots",
			4: u"7 Plots"
		},
		4: {
			0: u"WrapX + WrapY (Recommended)",
			1: u"WrapX Only",
			2: u"WrapY Only",
			3: u"None"
		}
	}
	if not selection_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
		return u"3 Players Per Line (Recommended)"
	if not selection_names[iOption].has_key(iSelection):
		return selection_names[iOption][0]
	return selection_names[iOption][iSelection]

def getCustomMapOptionDefault(argsList):
	[iOption] = argsList
	option_defaults = {
		0: 0,
		1: 3,
		2: 0,
		3: 2,
		4: 0
	}
	if not option_defaults.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
		return 0
	return option_defaults[iOption]

def isRandomCustomMapOption(argsList):
	[iOption] = argsList
	option_random = {
		0: false,
		1: false,
		2: false,
		3: false,
		4: false
	}
	if not option_random.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
		return false
	return option_random[iOption]

def getWrapX():
	iWrapMode = _get_option_value(4, 0, 4)
	return (iWrapMode == 0 or iWrapMode == 1)

def getWrapY():
	iWrapMode = _get_option_value(4, 0, 4)
	return (iWrapMode == 0 or iWrapMode == 2)

def getTopLatitude():
	return 50

def getBottomLatitude():
	return -50

def minStartingDistanceModifier():
	# <!-- custom: dense high-player symmetric lines need a low floor to avoid start failures. (GPT-5.3-Codex) -->
	return -95

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

def _players_per_line():
	iSelection = _get_option_value(0, 0, 2)
	if iSelection == 1:
		return 4
	return 3

def _band_mode():
	# 0 = coast, 1 = ocean
	return _get_option_value(2, 0, 2)

def _line_thickness():
	iSelection = _get_option_value(3, 2, 5)
	line_thicknesses = [3, 4, 5, 6, 7]
	return line_thicknesses[iSelection]

def _height_per_player():
	iSelection = _get_option_value(1, 3, 9)
	heights = [7, 8, 9, 10, 11, 12, 13, 14, 15]
	return heights[iSelection]

def _interline_gap():
	# <!-- custom: Keep line-to-line spacing equal to SAS_Large_Facing_Islands center gap for consistent tactical travel times. (GPT-5.3-Codex) -->
	return 8

def _effective_line_and_gap(iLines):
	iLinePlots = _line_thickness()
	iBandPlots = _interline_gap()
	# <!-- custom: This hard-cap fixed a reproducible startup crash on SAS48 world size at the full 48-player setup (SAS_Parallel_Lines only).
	# Root cause was extreme vertical map dimensions from many lines + wide inter-line gaps. Preserve configured line thickness and reduce
	# only inter-line water gap when necessary so normal sizes keep their original layout. (GPT-5.3-Codex) -->
	iMaxLateralSpanPlots = 192
	if iLines > 0:
		iMaxBand = (iMaxLateralSpanPlots / iLines) - iLinePlots
		if iMaxBand < 1:
			iMaxBand = 1
		if iBandPlots > iMaxBand:
			iBandPlots = iMaxBand
	return (iLinePlots, iBandPlots)

def _default_players_for_world():
	map_obj = CyMap()
	iWorld = map_obj.getWorldSize()
	if iWorld >= 0 and iWorld < gc.getNumWorldInfos():
		return max(1, gc.getWorldInfo(iWorld).getDefaultPlayers())
	return 11

def _line_count():
	iPPL = max(1, _players_per_line())
	iPlayers = _default_players_for_world()
	return max(1, (iPlayers + iPPL - 1) / iPPL)

def getGridSize(argsList):
	if argsList[0] == -1:
		return []

	# <!-- custom: Redesign for stability on extreme sizes (e.g., SAS48 with 48 players): scaling vertically caused startup crashes,
	# while SAS40 still worked. Keep lines parallel but scale line count laterally (X) and place players top-to-bottom within each line. (GPT-5.3-Codex) -->
	iPPL = _players_per_line()
	iTopBottomMarginPlots = 6
	iPerPlayerHeightPlots = _height_per_player()
	iInterPlayerGapPlots = 4
	iHeightPlots = 2 * iTopBottomMarginPlots + iPPL * iPerPlayerHeightPlots + max(0, iPPL - 1) * iInterPlayerGapPlots
	iLines = max(1, _line_count())
	(iLinePlots, iBandPlots) = _effective_line_and_gap(iLines)
	iWidthPlots = iLines * (iLinePlots + iBandPlots)
	return ((iWidthPlots + 3) / 4, (iHeightPlots + 3) / 4)

def _build_layout():
	global _sas_layout_cache
	if _sas_layout_cache is not None:
		return _sas_layout_cache

	map_obj = CyMap()
	iW = map_obj.getGridWidth()
	iH = map_obj.getGridHeight()
	iLines = _line_count()
	(iLinePlots, iBandPlots) = _effective_line_and_gap(iLines)
	iBlock = iLinePlots + iBandPlots
	if iBlock <= 0:
		iBlock = 1

	line_x_ranges = []
	line_x_centers = []
	for iLine in range(iLines):
		iLeft = (iLine * iBlock + iBandPlots) % iW
		iRight = iLeft + iLinePlots - 1
		line_x_ranges.append((iLeft, iRight))
		line_x_centers.append(iLeft + iLinePlots / 2)

	iPPL = _players_per_line()
	iDefaultPlayers = _default_players_for_world()
	line_slots = []
	for iLine in range(iLines):
		iSlots = iPPL
		if iLine == iLines - 1:
			iR = iDefaultPlayers % iPPL
			if iR != 0:
				iSlots = iR
		line_slots.append(iSlots)

	_sas_layout_cache = {
		"iW": iW,
		"iH": iH,
		"line_x_ranges": line_x_ranges,
		"line_x_centers": line_x_centers,
		"line_slots": line_slots,
		"line_plots": iLinePlots,
		"band_plots": iBandPlots,
	}
	return _sas_layout_cache

def beforeGeneration():
	global _sas_layout_cache, _sas_start_assignments
	_sas_layout_cache = None
	_sas_start_assignments = {}
	_build_layout()

def generatePlotTypes():
	NiTextOut("Setting Plot Types (Python SAS_Parallel_Lines) ...")
	layout = _build_layout()
	map_obj = CyMap()
	iW = layout["iW"]
	iH = layout["iH"]
	dice = gc.getGame().getMapRand()
	plot_types = [PlotTypes.PLOT_OCEAN] * (iW * iH)

	for (iLeft, iRight) in layout["line_x_ranges"]:
		for x in range(iLeft, iRight + 1):
			iX = x % iW
			for y in range(iH):
				plot_types[map_obj.plotNum(iX, y)] = PlotTypes.PLOT_LAND

	# <!-- custom: add gentle side waviness so lanes look less artificial while preserving readability. (GPT-5.3-Codex) -->
	for (iLeft, iRight) in layout["line_x_ranges"]:
		for y in range(iH):
			if dice.get(100, "SAS_Parallel_Lines left trim") < 22:
				iX = iLeft % iW
				i = map_obj.plotNum(iX, y)
				if plot_types[i] == PlotTypes.PLOT_LAND:
					plot_types[i] = PlotTypes.PLOT_OCEAN
			if dice.get(100, "SAS_Parallel_Lines right trim") < 22:
				iX = iRight % iW
				i = map_obj.plotNum(iX, y)
				if plot_types[i] == PlotTypes.PLOT_LAND:
					plot_types[i] = PlotTypes.PLOT_OCEAN

	# Sparse hills for texture.
	for i in range(iW * iH):
		if plot_types[i] != PlotTypes.PLOT_LAND:
			continue
		if dice.get(20, "SAS_Parallel_Lines hills") <= 2:
			plot_types[i] = PlotTypes.PLOT_HILLS

	return plot_types

def _is_valid_start_plot(pPlot):
	return pPlot is not None and not pPlot.isWater() and not pPlot.isImpassable()

def findStartingPlot(argsList):
	global _sas_start_assignments
	[playerID] = argsList
	layout = _build_layout()
	map_obj = CyMap()

	lPlayers = []
	for iPlayer in range(gc.getMAX_CIV_PLAYERS()):
		if gc.getPlayer(iPlayer).isAlive():
			lPlayers.append(iPlayer)
	if len(lPlayers) == 0:
		CyPythonMgr().allowDefaultImpl()
		return
	lPlayers.sort()

	if playerID not in lPlayers:
		CyPythonMgr().allowDefaultImpl()
		return

	if _sas_start_assignments.has_key(playerID):
		return _sas_start_assignments[playerID]

	iIndex = lPlayers.index(playerID)
	# <!-- custom: The custom slot count follows the world-size default, but Custom Game can contain more alive players. Let the DLL find unique starts for extras instead of reusing the final custom slot. See KI#238. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if iIndex >= sum(layout["line_slots"]):
		CyPythonMgr().allowDefaultImpl()
		return
	iRemaining = iIndex
	iLine = 0
	iPosInLine = 0
	for k in range(len(layout["line_slots"])):
		iSlots = layout["line_slots"][k]
		if iRemaining < iSlots:
			iLine = k
			iPosInLine = iRemaining
			break
		iRemaining -= iSlots
		iLine = k
		iPosInLine = iSlots - 1

	iSlotsThisLine = max(1, layout["line_slots"][iLine])
	iMargin = 2
	iSpan = max(1, layout["iH"] - 2 * iMargin)
	iY = iMargin + ((2 * iPosInLine + 1) * iSpan) / (2 * iSlotsThisLine)
	if iY >= layout["iH"]:
		iY = layout["iH"] - 1
	iX = layout["line_x_centers"][iLine] % layout["iW"]

	pSlot = map_obj.plot(iX, iY)
	if _is_valid_start_plot(pSlot):
		iPlotNum = map_obj.plotNum(iX, iY)
		_sas_start_assignments[playerID] = iPlotNum
		return iPlotNum

	# Fallback: search this line first.
	(iLeft, iRight) = layout["line_x_ranges"][iLine]
	for dy in range(layout["iH"]):
		iTryY = (iY + dy) % layout["iH"]
		for x in range(iLeft, iRight + 1):
			iTryX = x % layout["iW"]
			pTry = map_obj.plot(iTryX, iTryY)
			if _is_valid_start_plot(pTry):
				iNum = map_obj.plotNum(iTryX, iTryY)
				_sas_start_assignments[playerID] = iNum
				return iNum

	CyPythonMgr().allowDefaultImpl()
	return

def generateTerrainTypes():
	NiTextOut("Generating Terrain (Python SAS_Parallel_Lines) ...")
	terrain_gen = TerrainGenerator()
	terrain_types = terrain_gen.generateTerrain()
	_apply_interline_bands(terrain_types)
	return terrain_types

def _apply_interline_bands(terrain_types):
	if _band_mode() == 1:
		return

	layout = _build_layout()
	map_obj = CyMap()
	iW = layout["iW"]
	iH = layout["iH"]
	eCoast = getInfoTypeOrFail("TERRAIN_COAST")

	# <!-- custom: In coast-band mode, convert water between neighboring vertical lines to coast for early cross-line navigation. (GPT-5.3-Codex) -->
	for iLine in range(len(layout["line_x_ranges"])):
		(_, iRight) = layout["line_x_ranges"][iLine]
		(iNextLeft, _) = layout["line_x_ranges"][(iLine + 1) % len(layout["line_x_ranges"])]
		gap_cols = []
		x = (iRight + 1) % iW
		while x != iNextLeft % iW:
			gap_cols.append(x)
			x = (x + 1) % iW
			if len(gap_cols) > iW:
				break

		for iX in gap_cols:
			for y in range(iH):
				i = map_obj.plotNum(iX, y)
				pPlot = map_obj.plotByIndex(i)
				if pPlot.isWater():
					terrain_types[i] = eCoast

def addFeatures():
	NiTextOut("Adding Features (Python SAS_Parallel_Lines) ...")
	feature_gen = FeatureGenerator()
	feature_gen.addFeatures()
	return 0
