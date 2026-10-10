# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# FILE: SAS_Large_Facing_Islands.py
# PURPOSE: Facing-islands competitive map with fixed island size across all world sizes.
#
# AUTHORS: wonderingabout & GPT-5.3-Codex

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
	return "Each player starts on a large island, with rivals on the east and west sides, and a rival facing north or south (when world size is large enough). Island size stays the same on all world sizes and is fairly large. Supports world sizes up to SAS48. The world revolves horizontally (WrapX on). Inspired by Empire Earth (1) Large Islands, with some ideas adapted from our SAS_Longworld map."

def isAdvancedMap():
	# <!-- custom: Show this map in Simple Game; it has one practical option and stable generation path. (GPT-5.3-Codex) -->
	return 0

def getNumCustomMapOptions():
	return 3

def getNumHiddenCustomMapOptions():
	return 0

def getCustomMapOptionName(argsList):
	[iOption] = argsList
	option_names = {
		0: u"North-South Bands",
		1: u"East-West Bands",
		2: u"Revolving World"
	}
	if not option_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
		return u"North-South Bands"
	return option_names[iOption]

def getNumCustomMapOptionValues(argsList):
	[iOption] = argsList
	option_values = {
		0: 2,
		1: 2,
		2: 2
	}
	if not option_values.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
		return 2
	return option_values[iOption]

def getCustomMapOptionDescAt(argsList):
	[iOption, iSelection] = argsList
	# <!-- custom: Use explicit N-S / E-W labels for value text to stay self-explanatory in our UI rework context, especially when players discuss Simple Game screenshots where option headers are not always visible. (GPT-5.3-Codex) -->
	selection_names = {
		0: {
			0: u"N-S Coast Bands",
			1: u"N-S Ocean Bands (Recommended)"
		},
		1: {
			0: u"E-W Coast Bands (Recommended)",
			1: u"E-W Ocean Bands"
		},
		2: {
			0: u"WrapX (Recommended)",
			1: u"None"
		}
	}
	if not selection_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
		return u"N-S Ocean Bands (Recommended)"
	if not selection_names[iOption].has_key(iSelection):
		return u"N-S Ocean Bands (Recommended)"
	return selection_names[iOption][iSelection]

def getCustomMapOptionDefault(argsList):
	[iOption] = argsList
	option_defaults = {
		0: 1,
		1: 0,
		2: 0
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
		2: false
	}
	if not option_random.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
		return false
	return option_random[iOption]

def getWrapX():
	return _get_wrap_mode() == 0

def getWrapY():
	return False

def getTopLatitude():
	return 70

def getBottomLatitude():
	return -70

def minStartingDistanceModifier():
	# <!-- custom: Dense high-player island layouts need a very low floor to avoid start-placement failures on SAS sizes. (GPT-5.3-Codex) -->
	return -95

def _world_profile(eWorldSize):
	# <!-- custom: world-size-only scaling: island footprint stays identical; only island count increases with world size. (GPT-5.3-Codex) -->
	profiles = {
		# <!-- custom: One player starts per island; Arena and Small previously provided only 1/4 islands for their 2/5 default players. See KI#242. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		WorldSizeTypes.WORLDSIZE_ARENA: (1, 2),
		WorldSizeTypes.WORLDSIZE_DUEL: (1, 2),
		WorldSizeTypes.WORLDSIZE_TINY: (1, 3),
		WorldSizeTypes.WORLDSIZE_SMALL: (1, 5),
		WorldSizeTypes.WORLDSIZE_STANDARD: (2, 4),
		WorldSizeTypes.WORLDSIZE_LARGE: (2, 6),
		WorldSizeTypes.WORLDSIZE_HUGE: (2, 8),
		WorldSizeTypes.WORLDSIZE_SAS24: (2, 12),
		WorldSizeTypes.WORLDSIZE_SAS32: (2, 16),
		WorldSizeTypes.WORLDSIZE_SAS40: (2, 20),
		WorldSizeTypes.WORLDSIZE_SAS48: (2, 24)
	}
	iWorld = int(eWorldSize)
	if profiles.has_key(iWorld):
		return profiles[iWorld]
	return profiles[WorldSizeTypes.WORLDSIZE_SAS48]

def _get_ew_connector_mode():
	# 0 = Coast bands, 1 = Ocean bands
	return _get_option_value(1, 0, 2)

def _get_ns_connector_mode():
	# 0 = Coast bands, 1 = Ocean bands
	return _get_option_value(0, 1, 2)

def _get_wrap_mode():
	# 0 = WrapX, 1 = None
	return _get_option_value(2, 0, 2)

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

def _shape_constants():
	# <!-- custom: Fixed island footprint for all world sizes; tuned larger so each start island can host roughly 5 cities instead of 2-3. (GPT-5.3-Codex) -->
	iIslandWidth = 10
	iIslandHeight = 10
	# <!-- custom: Wider side separation keeps island interactions tactical and avoids immediate coast-to-coast crowding. (GPT-5.3-Codex) -->
	iSideGap = 8
	# <!-- custom: Keep rows near map edges for the "back against the wall" identity. (GPT-5.3-Codex) -->
	iEdgeMarginX = 3
	iEdgeMarginY = 1
	return (iIslandWidth, iIslandHeight, iSideGap, iEdgeMarginX, iEdgeMarginY)

def getGridSize(argsList):
	if argsList[0] == -1:
		return []

	[eWorldSize] = argsList
	(iRows, iCols) = _world_profile(eWorldSize)
	(iIslandWidth, iIslandHeight, iSideGap, iEdgeMarginX, iEdgeMarginY) = _shape_constants()
	# <!-- custom: Keep north-south spacing equal to east-west spacing for consistent geometry across axes. (GPT-5.3-Codex) -->
	iCenterGap = iSideGap

	# <!-- custom: keep wrap-edge island spacing equal to interior spacing (iSideGap), so edge-facing lanes are not tighter than middle lanes. (GPT-5.3-Codex) -->
	iWidth = iCols * iIslandWidth + (iCols - 1) * iSideGap + iSideGap
	iHeight = iRows * iIslandHeight + (iRows - 1) * iCenterGap + 2 * iEdgeMarginY
	# <!-- custom: Civ4 expects grid cells here; the EXE expands by 4x into actual plot dimensions. (GPT-5.3-Codex) -->
	return ((iWidth + 3) / 4, (iHeight + 3) / 4)

def _build_layout():
	global _sas_layout_cache
	if _sas_layout_cache is not None:
		return _sas_layout_cache

	map_obj = CyMap()
	iW = map_obj.getGridWidth()
	iH = map_obj.getGridHeight()
	(iRows, iCols) = _world_profile(map_obj.getWorldSize())
	(iIslandWidth, iIslandHeight, iSideGap, iEdgeMarginX, iEdgeMarginY) = _shape_constants()
	iCenterGap = iSideGap

	iTotalIslandWidth = iCols * iIslandWidth + (iCols - 1) * iSideGap
	iStartX = max(iEdgeMarginX, (iW - iTotalIslandWidth) / 2)

	islands = []
	start_slots = []
	row_y0 = []
	if iRows <= 1:
		row_y0.append(iEdgeMarginY)
	else:
		# <!-- custom: Anchor first row to top edge and last row to bottom edge; any rounding slack is absorbed in the center corridor. (GPT-5.3-Codex) -->
		iTopY0 = iEdgeMarginY
		iBottomY0 = max(iTopY0 + iIslandHeight + iCenterGap, iH - iEdgeMarginY - iIslandHeight)
		row_y0.append(iTopY0)
		row_y0.append(iBottomY0)
		if iRows > 2:
			iSpan = max(1, iBottomY0 - iTopY0)
			for k in range(1, iRows - 1):
				row_y0.append(iTopY0 + (k * iSpan) / (iRows - 1))
			row_y0 = sorted(row_y0)

	for r in range(iRows):
		iY0 = row_y0[r]
		iY1 = iY0 + iIslandHeight - 1
		iYCenter = (iY0 + iY1) / 2
		for c in range(iCols):
			iX0 = iStartX + c * (iIslandWidth + iSideGap)
			iX1 = iX0 + iIslandWidth - 1
			iXCenter = (iX0 + iX1) / 2
			islands.append((r, c, iX0, iX1, iY0, iY1, iXCenter, iYCenter))
			# <!-- custom: Use center-of-island starts for maximum opener stability and fair city placement odds on all rows. (GPT-5.3-Codex) -->
			start_slots.append((iXCenter, iYCenter))

	_sas_layout_cache = {
		"islands": islands,
		"slots": start_slots,
		"rows": iRows,
		"cols": iCols
	}
	return _sas_layout_cache

def beforeGeneration():
	global _sas_layout_cache, _sas_start_assignments
	_sas_layout_cache = None
	_sas_start_assignments = {}
	_build_layout()

def _is_island_land(iLX, iLY, iW, iH):
	fCX = (iW - 1) / 2.0
	fCY = (iH - 1) / 2.0
	fDX = (float(iLX) - fCX) / max(1.0, fCX)
	fDY = (float(iLY) - fCY) / max(1.0, fCY)
	return (fDX * fDX + fDY * fDY) <= 1.05

def generatePlotTypes():
	NiTextOut("Setting Plot Types (Python SAS_Large_Facing_Islands) ...")
	layout = _build_layout()
	map_obj = CyMap()
	iW = map_obj.getGridWidth()
	iH = map_obj.getGridHeight()
	dice = gc.getGame().getMapRand()
	plot_types = [PlotTypes.PLOT_OCEAN] * (iW * iH)

	for (_, _, iX0, iX1, iY0, iY1, _, _) in layout["islands"]:
		iIslandW = iX1 - iX0 + 1
		iIslandH = iY1 - iY0 + 1
		for x in range(iX0, iX1 + 1):
			for y in range(iY0, iY1 + 1):
				if _is_island_land(x - iX0, y - iY0, iIslandW, iIslandH):
					plot_types[map_obj.plotNum(x, y)] = PlotTypes.PLOT_LAND

	# Keep terrain texture light and predictable on starts.
	for i in range(iW * iH):
		if plot_types[i] != PlotTypes.PLOT_LAND:
			continue
		iRoll = dice.get(20, "SAS_Large_Facing_Islands hills")
		if iRoll <= 2:
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
	if iIndex >= len(layout["slots"]):
		CyPythonMgr().allowDefaultImpl()
		return

	(iX, iY) = layout["slots"][iIndex]
	pSlot = map_obj.plot(iX, iY)
	if _is_valid_start_plot(pSlot):
		iPlotNum = map_obj.plotNum(iX, iY)
		_sas_start_assignments[playerID] = iPlotNum
		return iPlotNum

	# Fallback within the same island first.
	(_, _, iX0, iX1, iY0, iY1, _, _) = layout["islands"][iIndex]
	for y in range(iY0, iY1 + 1):
		for x in range(iX0, iX1 + 1):
			pTry = map_obj.plot(x, y)
			if _is_valid_start_plot(pTry):
				iNum = map_obj.plotNum(x, y)
				_sas_start_assignments[playerID] = iNum
				return iNum

	CyPythonMgr().allowDefaultImpl()
	return

def generateTerrainTypes():
	NiTextOut("Generating Terrain (Python SAS_Large_Facing_Islands) ...")
	terrain_gen = TerrainGenerator()
	terrain_types = terrain_gen.generateTerrain()
	_apply_horizontal_coast_lanes(terrain_types)
	_apply_vertical_coast_lanes(terrain_types)
	return terrain_types

def addFeatures():
	NiTextOut("Adding Features (Python SAS_Large_Facing_Islands) ...")
	feature_gen = FeatureGenerator()
	feature_gen.addFeatures()
	return 0

def _apply_horizontal_coast_lanes(terrain_types):
	layout = _build_layout()
	map_obj = CyMap()
	iW = map_obj.getGridWidth()
	iH = map_obj.getGridHeight()
	iConnectorMode = _get_ew_connector_mode()
	# <!-- custom: Optional horizontal connector type; ocean-band mode keeps lanes as ocean (no early coast linkage). (GPT-5.3-Codex) -->
	if iConnectorMode == 1:
		return

	eCoast = getInfoTypeOrFail("TERRAIN_COAST")

	# <!-- custom: Build tapered coast corridors between horizontal neighbors so lane thickness feels natural: 3 near island edges, then 2, then 1 at the center, mirrored back to 2 and 3. (GPT-5.3-Codex) -->
	for r in range(layout["rows"]):
		row_islands = []
		for info in layout["islands"]:
			if info[0] == r:
				row_islands.append(info)
		if len(row_islands) == 0:
			continue

		row_islands.sort(key=lambda t: t[2])
		iYCenter = row_islands[0][7]

		for i in range(len(row_islands)):
			iLeft = row_islands[i]
			iRight = row_islands[(i + 1) % len(row_islands)]
			iLeftX1 = iLeft[3]
			iRightX0 = iRight[2]

			if i < len(row_islands) - 1:
				gap_x = range(iLeftX1 + 1, iRightX0)
			else:
				# wrap-edge gap between last and first island
				gap_x = list(range(iLeftX1 + 1, iW)) + list(range(0, iRightX0))

			iGapLen = len(gap_x)
			if iGapLen <= 0:
				continue
			# <!-- custom: Use proportional taper bands so 3/2/1 widths remain visible on large gaps instead of collapsing to a near-constant 1-tile lane. (GPT-5.3-Codex) -->
			iBand3 = max(1, iGapLen / 6)
			iBand2 = max(iBand3 + 1, iGapLen / 3)
			for iPos in range(iGapLen):
				x = gap_x[iPos]
				iEdgeDist = min(iPos, iGapLen - 1 - iPos)
				if iEdgeDist < iBand3:
					lane_rows = [iYCenter - 1, iYCenter, iYCenter + 1]
				elif iEdgeDist < iBand2:
					lane_rows = [iYCenter - 1, iYCenter]
				else:
					lane_rows = [iYCenter]
				for y in lane_rows:
					if y < 0 or y >= iH:
						continue
					iPlot = map_obj.plotNum(x, y)
					pPlot = map_obj.plotByIndex(iPlot)
					if pPlot.isWater():
						terrain_types[iPlot] = eCoast

def _apply_vertical_coast_lanes(terrain_types):
	layout = _build_layout()
	map_obj = CyMap()
	iW = map_obj.getGridWidth()
	iH = map_obj.getGridHeight()
	iConnectorMode = _get_ns_connector_mode()
	# <!-- custom: Optional vertical connector type; ocean-band mode keeps inter-row lanes as ocean. (GPT-5.3-Codex) -->
	if iConnectorMode == 1:
		return

	eCoast = getInfoTypeOrFail("TERRAIN_COAST")

	# <!-- custom: Mirror the same tapered profile used on east-west lanes, now for north-south facing lanes: 3 near island edges, then 2, then 1 in the center, then back to 2 and 3. (GPT-5.3-Codex) -->
	for c in range(layout["cols"]):
		col_islands = []
		for info in layout["islands"]:
			if info[1] == c:
				col_islands.append(info)
		if len(col_islands) <= 1:
			continue
		col_islands.sort(key=lambda t: t[4])

		for i in range(len(col_islands) - 1):
			iTop = col_islands[i]
			iBottom = col_islands[i + 1]
			iTopY1 = iTop[5]
			iBottomY0 = iBottom[4]
			iXCenter = iTop[6]
			gap_y = range(iTopY1 + 1, iBottomY0)
			iGapLen = len(gap_y)
			if iGapLen <= 0:
				continue

			iBand3 = max(1, iGapLen / 6)
			iBand2 = max(iBand3 + 1, iGapLen / 3)
			for iPos in range(iGapLen):
				y = gap_y[iPos]
				iEdgeDist = min(iPos, iGapLen - 1 - iPos)
				if iEdgeDist < iBand3:
					lane_cols = [iXCenter - 1, iXCenter, iXCenter + 1]
				elif iEdgeDist < iBand2:
					lane_cols = [iXCenter - 1, iXCenter]
				else:
					lane_cols = [iXCenter]

				for x in lane_cols:
					if x < 0 or x >= iW or y < 0 or y >= iH:
						continue
					iPlot = map_obj.plotNum(x, y)
					pPlot = map_obj.plotByIndex(iPlot)
					if pPlot.isWater():
						terrain_types[iPlot] = eCoast
