# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# FILE: SAS_Longworld.py
# PURPOSE: Lightweight long-format world map for AdvCiv-SAS.
#
# AUTHORS: wonderingabout & GPT-5.3-Codex
# <!-- custom: inspired by the Ringworld3 v1.02 map -->

from CvPythonExtensions import *
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator
from SAS_WorldSizeUtils import *

# <!-- custom: Keep stable per-generation Longworld assignments so trimmed-edge fallback cannot return an already-used starting plot. See KI#246. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
longworldStartByPlayer = {}
longworldUsedStarts = {}

def getVersion():
	return "1.00"

def getDescription():
	return "A lightweight long-format world for AdvCiv-SAS: one connected horizontal landmass with wavy borders, inspired by Ringworld3_102_mst.py but heavily simplified."

def isAdvancedMap():
	"This map should show up in Simple Game."
	# <!-- custom: Keep visible in Simple Game; option set is compact and stale-option safe. (GPT-5.3-Codex) -->
	return 0

def getNumCustomMapOptions():
	return 3

def getNumHiddenCustomMapOptions():
	return 0

def getCustomMapOptionName(argsList):
	[iOption] = argsList
	option_names = {
		0: u"LongWorld Height",
		1: u"Width Per Player",
		2: u"Revolving World"
	}
	if not option_names.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
		return u"LongWorld Height"
	return option_names[iOption]

def getNumCustomMapOptionValues(argsList):
	[iOption] = argsList
	option_values = {
		0: 6,
		1: 6,
		2: 2
	}
	if not option_values.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
		return 3
	return option_values[iOption]

def getCustomMapOptionDescAt(argsList):
	[iOption, iSelection] = argsList
	selection_names = {
		0: {
			0: u"2 Plots",
			1: u"3 Plots",
			2: u"4 Plots (Recommended)",
			3: u"5 Plots",
			4: u"6 Plots",
			5: u"7 Plots"
		},
		1: {
			0: u"10",
			1: u"11",
			2: u"12 (Recommended)",
			3: u"13",
			4: u"14",
			5: u"15"
		},
		2: {
			0: u"WrapX (Recommended)",
			1: u"None"
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
		1: 2,
		2: 0
	}
	if not option_defaults.has_key(iOption):
		sas_warn_simple_game_stale_option_once(iOption, getNumCustomMapOptions())
		return 2
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

def _strip_height():
	iSelection = _get_option_value(0, 2, 6)
	heights = [2, 3, 4, 5, 6, 7]
	return heights[iSelection]

def _width_per_player():
	iSelection = _get_option_value(1, 2, 6)
	widths = [10, 11, 12, 13, 14, 15]
	return widths[iSelection]

def getWrapX():
	iWrapMode = _get_option_value(2, 0, 2)
	return (iWrapMode == 0)

def getWrapY():
	return False

def getTopLatitude():
	return 70

def getBottomLatitude():
	return -70

def minStartingDistanceModifier():
	# <!-- custom: Tight long-strip maps need very low min start distance so high player-count games don't fail start assignment. (GPT-5.3-Codex) -->
	return -95

def getStripBounds(iH):
	# <!-- custom: Use explicit strip heights for clarity in map options; clamp by map height for very small worlds. (GPT-5.3-Codex) -->
	iBandHeight = _strip_height()
	iBandHeight = max(2, iBandHeight)
	iBandHeight = min(iH - 2, iBandHeight)

	iTopRow = max(1, (iH / 2) - (iBandHeight / 2))
	iBottomRow = iTopRow + iBandHeight - 1
	if iBottomRow > iH - 2:
		iBottomRow = iH - 2
		iTopRow = max(1, iBottomRow - iBandHeight + 1)
	return (iTopRow, iBottomRow)

# <!-- custom: Longworld's fallback may move a player onto another player's nominal strip tile.
# Reset the assignment cache and used-plot set for each generation. See KI#246. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
def beforeGeneration():
	global longworldStartByPlayer
	global longworldUsedStarts
	longworldStartByPlayer = {}
	longworldUsedStarts = {}

def findStartingPlot(argsList):
	# <!-- custom: Force starts onto the long central strip with even horizontal spacing.
	# Preserve assignments across repeated callbacks and skip already-used fallback plots so trimmed strip edges cannot give two players the same start. See KI#246. (GPT-5.3-Codex; ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	[playerID] = argsList
	global longworldStartByPlayer
	global longworldUsedStarts
	if longworldStartByPlayer.has_key(playerID):
		return longworldStartByPlayer[playerID]
	gc = CyGlobalContext()
	map_obj = CyMap()
	iW = map_obj.getGridWidth()
	iH = map_obj.getGridHeight()
	(iTopRow, iBottomRow) = getStripBounds(iH)
	strip_rows = range(iTopRow, iBottomRow + 1)

	player_ids = []
	for iPlayer in range(gc.getMAX_CIV_PLAYERS()):
		pPlayer = gc.getPlayer(iPlayer)
		if pPlayer.isAlive():
			player_ids.append(iPlayer)

	if len(player_ids) == 0:
		CyPythonMgr().allowDefaultImpl()
		return

	player_ids.sort()
	if playerID not in player_ids:
		CyPythonMgr().allowDefaultImpl()
		return

	iIndex = player_ids.index(playerID)
	iCount = len(player_ids)
	iMargin = 2
	iSpan = max(1, iW - 2 * iMargin)
	iX = iMargin + (iIndex * iSpan) / iCount
	if iX >= iW:
		iX = iW - 1
	iY = strip_rows[iIndex % len(strip_rows)]

	# <!-- custom: A trimmed or already-used nominal tile enters the nearest-column-first search, which now excludes every plot assigned earlier in this generation. See KI#246. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	iPlot = map_obj.plotNum(iX, iY)
	pPlot = map_obj.plot(iX, iY)
	if pPlot.isWater() or pPlot.isImpassable() or longworldUsedStarts.has_key(iPlot):
		for dx in range(iW):
			iTryX = (iX + dx) % iW
			for iTryY in strip_rows:
				pTry = map_obj.plot(iTryX, iTryY)
				iTryPlot = map_obj.plotNum(iTryX, iTryY)
				if not pTry.isWater() and not pTry.isImpassable() and not longworldUsedStarts.has_key(iTryPlot):
					# <!-- custom: Cache the chosen fallback before returning so repeated callbacks stay stable and later players cannot reuse it. See KI#246. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
					longworldStartByPlayer[playerID] = iTryPlot
					longworldUsedStarts[iTryPlot] = True
					return iTryPlot
		CyPythonMgr().allowDefaultImpl()
		return

	# <!-- custom: Record a valid nominal assignment too, reserving it against later fallbacks and making repeated callbacks return the same plot. See KI#246. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	longworldStartByPlayer[playerID] = iPlot
	longworldUsedStarts[iPlot] = True
	return iPlot

def getGridSize(argsList):
	if argsList[0] == -1:
		return []

	# <!-- custom: Use XML-aligned DLL WorldSizeTypes keys so Huge and SAS sizes use their intended Longworld grid profiles instead of stale/misaligned fallback rows. (Claude code Opus 4.7; GPT-5.5; ChatGPT-5.5) -->
	grid_sizes = {
		WorldSizeTypes.WORLDSIZE_ARENA: (9, 4),
		WorldSizeTypes.WORLDSIZE_DUEL: (12, 4),
		WorldSizeTypes.WORLDSIZE_TINY: (18, 5),
		WorldSizeTypes.WORLDSIZE_SMALL: (27, 6),
		WorldSizeTypes.WORLDSIZE_STANDARD: (34, 6),
		WorldSizeTypes.WORLDSIZE_LARGE: (41, 6),
		WorldSizeTypes.WORLDSIZE_HUGE: (54, 6),
	}

	[eWorldSize] = argsList
	fWidthScale = float(_width_per_player()) / 12.0
	def _scaled_width(iBaseWidth):
		return max(3, int(float(iBaseWidth) * fWidthScale + 0.5))

	key = sas_find_world_size_key(eWorldSize, grid_sizes)
	if key is not None:
		(iBaseWidth, iHeight) = grid_sizes[key]
		return (_scaled_width(iBaseWidth), iHeight)

	# <!-- custom: For post-Huge SAS sizes, keep long-map height and scale width by default player ratio from Huge anchor. (GPT-5.3-Codex) -->
	gc = CyGlobalContext()
	iWorldSize = int(eWorldSize)
	if iWorldSize > int(WorldSizeTypes.WORLDSIZE_HUGE) and iWorldSize < gc.getNumWorldInfos():
		(iAnchorWidth, iAnchorHeight) = grid_sizes[WorldSizeTypes.WORLDSIZE_HUGE]
		iAnchorWidth = _scaled_width(iAnchorWidth)
		iAnchorPlayers = max(1, gc.getWorldInfo(WorldSizeTypes.WORLDSIZE_HUGE).getDefaultPlayers())
		iTargetPlayers = max(1, gc.getWorldInfo(eWorldSize).getDefaultPlayers())
		iTargetWidth = max(iAnchorWidth + 1, int((float(iAnchorWidth) * float(iTargetPlayers)) / float(iAnchorPlayers) + 0.5))
		return (iTargetWidth, iAnchorHeight)

	(iFallbackWidth, iFallbackHeight) = grid_sizes[WorldSizeTypes.WORLDSIZE_HUGE]
	return (_scaled_width(iFallbackWidth), iFallbackHeight)

def generatePlotTypes():
	NiTextOut("Setting Plot Types (Python SAS_Longworld) ...")
	map_obj = CyMap()
	iW = map_obj.getGridWidth()
	iH = map_obj.getGridHeight()
	dice = CyGlobalContext().getGame().getMapRand()
	plot_types = [PlotTypes.PLOT_OCEAN] * (iW * iH)

	# <!-- custom: Build one continuous horizontal land strip (single connected landmass) centered on the map. (GPT-5.3-Codex) -->
	if iH < 3:
		for x in range(iW):
			plot_types[map_obj.plotNum(x, 0)] = PlotTypes.PLOT_LAND
		return plot_types

	(iTopRow, iBottomRow) = getStripBounds(iH)
	iBaseBandHeight = iBottomRow - iTopRow + 1
	iMinBandHeight = max(3, iBaseBandHeight - 2)
	iPrevTop = iTopRow
	iPrevBottom = iBottomRow
	for x in range(iW):
		iTopTrim = 0
		iBottomTrim = 0
		if dice.get(100, "SAS_Longworld border top trim") < 35:
			iTopTrim = 1
			if dice.get(100, "SAS_Longworld border top trim extra") < 10:
				iTopTrim = 2
		if dice.get(100, "SAS_Longworld border bottom trim") < 35:
			iBottomTrim = 1
			if dice.get(100, "SAS_Longworld border bottom trim extra") < 10:
				iBottomTrim = 2

		iLocalTop = iTopRow + iTopTrim
		iLocalBottom = iBottomRow - iBottomTrim
		if iLocalBottom - iLocalTop + 1 < iMinBandHeight:
			iLocalTop = iTopRow
			iLocalBottom = iBottomRow

		iLocalTop = max(iPrevTop - 1, min(iPrevTop + 1, iLocalTop))
		iLocalBottom = max(iPrevBottom - 1, min(iPrevBottom + 1, iLocalBottom))
		if iLocalBottom - iLocalTop + 1 < iMinBandHeight:
			iLocalTop = iPrevTop
			iLocalBottom = iPrevBottom

		for y in range(iLocalTop, iLocalBottom + 1):
			plot_types[map_obj.plotNum(x, y)] = PlotTypes.PLOT_LAND

		iPrevTop = iLocalTop
		iPrevBottom = iLocalBottom

	# Add sparse hills/peaks on the strip for texture, but keep enough flat plots for starts.
	for x in range(iW):
		for y in range(iTopRow, iBottomRow + 1):
			i = map_obj.plotNum(x, y)
			if plot_types[i] != PlotTypes.PLOT_LAND:
				continue
			iChance = dice.get(20, "SAS_Longworld hills")
			if iChance <= 2:
				plot_types[i] = PlotTypes.PLOT_HILLS

	return plot_types

def generateTerrainTypes():
	NiTextOut("Generating Terrain (Python SAS_Longworld) ...")
	terrain_gen = TerrainGenerator()
	return terrain_gen.generateTerrain()

def addFeatures():
	NiTextOut("Adding Features (Python SAS_Longworld) ...")
	feature_gen = FeatureGenerator()
	feature_gen.addFeatures()
	return 0
