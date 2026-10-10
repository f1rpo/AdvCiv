# SevoPediaTrait
#
# Copyright (c) 2008 The BUG Mod.
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#

from CvPythonExtensions import *
from SASMagicNumbers import *
import CvUtil
import ScreenInput
import SevoScreenEnums
import SASTextScale

import re
import TraitUtil
from _sevopedia_helpers import *

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

IS_SHOW_ZERO_TRAIT_PAIRS = (gc.getDefineINT("SAS_SEVOPEDIA_TRAIT_SHOW_ZERO_PAIRS") > 0)
IS_SHOW_ZERO_TRAIT_PAIRS_LEFT = (gc.getDefineINT("SAS_SEVOPEDIA_TRAIT_SHOW_ZERO_PAIRS_LEFT") > 0)
TRAIT_STATS_FIXED_LEADER_COLS = 6
TRAIT_STATS_ICON_COL_W = INCHART_ICON_SIZE + INCHART_ICON_SPACING
TRAIT_STATS_RIGHT_TABLE_SCROLLBAR_TAIL_W = 18
TRAIT_STATS_MOVE_LEFT_TO_RIGHT_W = 30

# <!-- custom: Module-level cache for trait statistics.
# Computed once on first Traits category click, then reused for the entire session.
# Similar pattern to SevoPediaLeader's LEADERS_INFO_CACHED. (Claude Opus 4.5) -->
# traitLeaders: dict of traitId -> [leaderIds] - used to quickly compute per-trait pairing data
# allPairsData: list of (trait1, trait2, count, [leaderIds]) - all trait combinations globally
# allPairsMinMax: (minCount, maxCount) - for ranking bar normalization
TRAIT_STATISTICS_CACHE = None

def precomputeTraitStatisticsCache():
	# Precompute and cache trait statistics data. Called once from SevoPediaMain.placeTraits()
	# on first Traits category click. Returns the cache dict.
	global TRAIT_STATISTICS_CACHE

	if TRAIT_STATISTICS_CACHE is not None:
		return TRAIT_STATISTICS_CACHE

	numTraits = gc.getNumTraitInfos()
	numLeaders = gc.getNumLeaderHeadInfos()

	# Build traitId -> [leaderIds] mapping (used for per-trait pairing lookups)
	traitLeaders = {}
	for iTrait in range(numTraits):
		traitLeaders[iTrait] = []
	for iLeader in range(numLeaders):
		leader = gc.getLeaderHeadInfo(iLeader)
		for iTrait in range(numTraits):
			if leader.hasTrait(iTrait):
				traitLeaders[iTrait].append(iLeader)

	# Build leader -> traits mapping (only leaders with 2+ traits) for all-pairs computation
	leaderTraits = {}
	for iLeader in range(numLeaders):
		leader = gc.getLeaderHeadInfo(iLeader)
		traits = set()
		for iTrait in range(numTraits):
			if leader.hasTrait(iTrait):
				traits.add(iTrait)
		if len(traits) >= 2:
			leaderTraits[iLeader] = traits

	# Count all trait pairs
	pairCounts = {}
	for iLeader, traits in leaderTraits.items():
		traitList = sorted(traits)
		for i in range(len(traitList)):
			for j in range(i + 1, len(traitList)):
				pair = (traitList[i], traitList[j])
				if pair not in pairCounts:
					pairCounts[pair] = []
				pairCounts[pair].append(iLeader)

	# Build sorted all-pairs data
	allPairsData = []
	def ordered_pair_for_display(t1, t2):
		sort1 = re.sub(r"[^A-Za-z0-9]+", "", gc.getTraitInfo(t1).getDescription() or "")
		sort2 = re.sub(r"[^A-Za-z0-9]+", "", gc.getTraitInfo(t2).getDescription() or "")
		if sort2 < sort1:
			return t2, t1
		return t1, t2
	if IS_SHOW_ZERO_TRAIT_PAIRS:
		for t1 in range(numTraits):
			for t2 in range(t1 + 1, numTraits):
				ids = pairCounts.get((t1, t2), [])
				o1, o2 = ordered_pair_for_display(t1, t2)
				allPairsData.append((o1, o2, len(ids), sorted(ids)))
	else:
		for (t1, t2), ids in pairCounts.items():
			o1, o2 = ordered_pair_for_display(t1, t2)
			allPairsData.append((o1, o2, len(ids), sorted(ids)))
	def clean_sort_name(text):
		# <!-- custom: strip non-alphanumeric chars so tie-break sorts ignore icons/punctuation. (GPT-5.2-Codex) -->
		return re.sub(r"[^A-Za-z0-9]+", "", text or "")
	def all_pairs_sort_key(entry):
		t1, t2, count, ids = entry
		name1 = clean_sort_name(gc.getTraitInfo(t1).getDescription())
		name2 = clean_sort_name(gc.getTraitInfo(t2).getDescription())
		return (-count, name1, name2)
	allPairsData.sort(key=all_pairs_sort_key)
	allPairsMaxLeaders = 0
	if allPairsData:
		allPairsMaxLeaders = max([len(d[3]) for d in allPairsData])

	# Precompute min/max for ranking bar normalization
	if allPairsData:
		counts = [d[2] for d in allPairsData]
		allPairsMinMax = (min(counts), max(counts))
	else:
		allPairsMinMax = (0, 0)

	TRAIT_STATISTICS_CACHE = {
		"traitLeaders": traitLeaders,
		"allPairsData": tuple(allPairsData),
		"allPairsMinMax": allPairsMinMax,
		"allPairsMaxLeaders": allPairsMaxLeaders,
	}

	print("Sevopedia Trait statistics cache prebuilt. This should appear only once per gaming session.")
	return TRAIT_STATISTICS_CACHE

class SevoPediaTrait:

	def __init__(self, main):
		self.iTrait = -1
		self.bHistoryExpanded = False
		self.top = main

		# <!-- custom: Enhanced layout - Leaders on right, Statistics center, Effects bottom-left, History bottom-right.
		# Precompute all layout values once in __init__ for performance. (Claude code Opus 4.5) (GPT-5.2-Codex (summarized)) -->

		MARGIN = 10
		self.BOTTOM_PANELS_H = 291

		# Leaders panel - right side, full height
		self.W_LEADERS = get_multilist_panel_width_for_buttons(4, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_MULTI_LIST_LEFT_EDGE_PADDING, HYPOTHESIZED_MULTI_LIST_RIGHT_EDGE_PADDING, HYPOTHESIZED_MULTI_LIST_INTER_BUTTON_SPACING)
		self.X_LEADERS = self.top.R_PEDIA_PAGE - self.W_LEADERS
		self.Y_LEADERS = self.top.Y_PEDIA_PAGE
		self.H_LEADERS = self.top.B_PEDIA_PAGE - self.Y_LEADERS
		self.R_CONTENT = self.top.R_PEDIA_PAGE - self.W_LEADERS - MARGIN

		# Statistics panel - wide center panel
		self.X_STATISTICS = self.top.X_PEDIA_PAGE
		self.Y_STATISTICS = self.top.Y_PEDIA_PAGE
		self.W_STATISTICS = self.R_CONTENT - self.X_STATISTICS
		self.H_STATISTICS = self.top.B_PEDIA_PAGE - self.top.Y_PEDIA_PAGE - self.BOTTOM_PANELS_H

		# Statistics sub-panels layout (precomputed)
		STATS_MARGIN = 10
		STATS_INNER = 8
		self.STATS_ROW_H = 24
		self.STATS_PANEL_Y = self.Y_STATISTICS + 30
		self.STATS_PANEL_H = self.H_STATISTICS - 40
		self.STATS_PANEL_X = self.X_STATISTICS + STATS_MARGIN
		self.STATS_PANEL_W = self.W_STATISTICS - (2 * STATS_MARGIN)
		self.STATS_INNER = STATS_INNER
		self.STATS_MARGIN = STATS_MARGIN

		# Left panel dimensions
		self.STATS_LEFT_W = int(self.STATS_PANEL_W * 0.44) - TRAIT_STATS_MOVE_LEFT_TO_RIGHT_W
		self.STATS_LEFT_TABLE_X = self.STATS_PANEL_X + STATS_INNER
		self.STATS_LEFT_TABLE_W = self.STATS_LEFT_W - (2 * STATS_INNER)
		self.STATS_LEFT_TABLE_Y = self.STATS_PANEL_Y + STATS_INNER
		self.STATS_LEFT_TABLE_H = self.STATS_PANEL_H - (2 * STATS_INNER)

		# Left table column widths
		leftPairW = 50
		leftTotalW = 50
		leftLeadersW = TRAIT_STATS_FIXED_LEADER_COLS * TRAIT_STATS_ICON_COL_W
		leftTraitW = self.STATS_LEFT_TABLE_W - leftPairW - leftTotalW - leftLeadersW
		self.STATS_LEFT_COL_TRAIT = leftTraitW
		self.STATS_LEFT_COL_PAIR = leftPairW
		self.STATS_LEFT_COL_TOTAL = leftTotalW
		self.STATS_LEFT_COL_LEADERS = leftLeadersW

		# Right panel dimensions
		self.STATS_RIGHT_X = self.STATS_PANEL_X + self.STATS_LEFT_W + STATS_MARGIN
		self.STATS_RIGHT_W = self.STATS_PANEL_X + self.STATS_PANEL_W - self.STATS_RIGHT_X
		self.STATS_RIGHT_TABLE_X = self.STATS_RIGHT_X + STATS_INNER
		self.STATS_RIGHT_TABLE_W = self.STATS_RIGHT_W - (2 * STATS_INNER)
		self.STATS_RIGHT_TABLE_Y = self.STATS_PANEL_Y + STATS_INNER
		self.STATS_RIGHT_TABLE_H = self.STATS_PANEL_H - (2 * STATS_INNER)

		# Right table column widths
		rightCountW = 50
		rightRankW = 80
		rightLeadersW = TRAIT_STATS_FIXED_LEADER_COLS * TRAIT_STATS_ICON_COL_W
		rightTraitsTotalW = self.STATS_RIGHT_TABLE_W - rightCountW - rightRankW - rightLeadersW - TRAIT_STATS_RIGHT_TABLE_SCROLLBAR_TAIL_W
		rightTrait1W = int(rightTraitsTotalW / 2)
		rightTrait2W = rightTraitsTotalW - rightTrait1W
		self.STATS_RIGHT_COL_TRAIT1 = rightTrait1W
		self.STATS_RIGHT_COL_TRAIT2 = rightTrait2W
		self.STATS_RIGHT_COL_COUNT = rightCountW
		self.STATS_RIGHT_COL_RANK = rightRankW
		self.STATS_RIGHT_COL_LEADERS = rightLeadersW

		# Bottom row: Effects (left) and History (right)
		Y_BOTTOM_ROW = self.Y_STATISTICS + self.H_STATISTICS + MARGIN
		H_BOTTOM_ROW = self.top.B_PEDIA_PAGE - Y_BOTTOM_ROW

		# Effects panel - bottom left
		self.X_SPECIAL = self.top.X_PEDIA_PAGE
		self.Y_SPECIAL = Y_BOTTOM_ROW
		self.W_SPECIAL = (self.R_CONTENT - self.X_SPECIAL - MARGIN) / 2
		self.H_SPECIAL = H_BOTTOM_ROW

		# History panel - bottom right
		self.X_HISTORY = self.X_SPECIAL + self.W_SPECIAL + MARGIN
		self.Y_HISTORY = Y_BOTTOM_ROW
		self.W_HISTORY = self.R_CONTENT - self.X_HISTORY
		self.H_HISTORY = H_BOTTOM_ROW

		# <!-- custom: Row height for statistics tables.
		# Icon size and spacing now use centralized INCHART_* constants. (Claude Opus 4.5) -->
		# LEADER_ICON_SIZE and LEADER_BUTTON_COLUMN_SPACING replaced by INCHART_ICON_SIZE and INCHART_ICON_SPACING from _sevopedia_helpers

	# <!-- custom: Updated to receive trait ID directly via WIDGET_PYTHON approach (no longer concept-based). (Claude code Opus 4.5) -->
	def interfaceScreen(self, iTrait):
		self.iLeader = -1
		if self.iTrait != iTrait:
			self.bHistoryExpanded = False
		self.iTrait = iTrait

		self.placeLeaders()
		self.placeStatistics()
		self.placeSpecial()
		self.placeHistory()

	def placeLeaders(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		leaderIds, leaderToCiv, totalRealLeaders = get_real_leader_maps_and_count(EXCLUDED_LEADER_TYPES_FROM_SEVOPEDIA)
		leadersWithTrait = []
		for iLeader in leaderIds:
			if gc.getLeaderHeadInfo(iLeader).hasTrait(self.iTrait):
				leadersWithTrait.append(iLeader)

		headerLabel = localText.getText("TXT_KEY_CONCEPT_LEADERS", ())
		headerText = format_leaders_header_text(len(leadersWithTrait), totalRealLeaders, headerLabel)
		screen.addPanel( panelName, headerText, "", False, True, self.X_LEADERS, self.Y_LEADERS, self.W_LEADERS, self.H_LEADERS, PanelStyles.PANEL_STYLE_BLUE50 )
		rowListName = self.top.getNextWidgetName()
		multiListX = self.X_LEADERS + MULTI_LIST_PANEL_OFFSET_X
		multiListY = self.Y_LEADERS + MULTI_LIST_PANEL_OFFSET_Y
		multiListW = self.W_LEADERS + MULTI_LIST_PANEL_ADDITIONAL_W
		multiListH = self.H_LEADERS + MULTI_LIST_PANEL_ADDITIONAL_H
		screen.addMultiListControlGFC(rowListName, "", multiListX, multiListY, multiListW, multiListH, SEVOPEDIA_MULTILIST_NUM_LISTS_AUTO_CALCULATE, MULTILIST_BUTTON_SIZE, MULTILIST_BUTTON_SIZE, TableStyles.TABLE_STYLE_STANDARD)
		for iLeader in leadersWithTrait:
			self.iLeader = iLeader
			iCiv = leaderToCiv.get(iLeader, -1)
			if iCiv == -1:
				continue
			screen.appendMultiListButton(rowListName, gc.getLeaderHeadInfo(iLeader).getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_LEADER, iLeader, iCiv, False)

	# <!-- custom: Statistics panel with two charts:
	# Left: Trait pairings for current trait (trait, paired count, total, leaders)
	# Right: All trait combinations globally with pair count, ranking bar, and leaders
	# Inspired by README_Assets_Rebalancing.md tables. (Claude code Opus 4.5) -->
	def placeStatistics(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_STATISTICS", ()), "", True, True, self.X_STATISTICS, self.Y_STATISTICS, self.W_STATISTICS, self.H_STATISTICS, PanelStyles.PANEL_STYLE_BLUE50)

		leaderToCiv = get_real_leader_maps_and_count(EXCLUDED_LEADER_TYPES_FROM_SEVOPEDIA)[1]

		pairingData = self._buildTraitPairingData()

		# === LEFT TABLE: Pairings for current trait ===
		if pairingData:
			leftPanelName = self.top.getNextWidgetName()
			screen.addPanel(leftPanelName, "", "", True, True, self.STATS_PANEL_X, self.STATS_PANEL_Y, self.STATS_LEFT_W, self.STATS_PANEL_H, PanelStyles.PANEL_STYLE_BLUE50)

			leftTableName = self.top.getNextWidgetName()
			screen.addTableControlGFC(leftTableName, 3 + TRAIT_STATS_FIXED_LEADER_COLS, self.STATS_LEFT_TABLE_X, self.STATS_LEFT_TABLE_Y, self.STATS_LEFT_TABLE_W, self.STATS_LEFT_TABLE_H, True, False, self.STATS_ROW_H, self.STATS_ROW_H, CHART_TABLE_STYLE)
			screen.enableSort(leftTableName)

			screen.setTableColumnHeader(leftTableName, 0, SASTextScale.labelText(localText.getText("TXT_KEY_PEDIA_SAS_TRAIT_HEADER", ())), self.STATS_LEFT_COL_TRAIT)
			screen.setTableColumnHeader(leftTableName, 1, SASTextScale.labelText(localText.getText("TXT_KEY_PEDIA_SAS_PAIR_COUNT", ())), self.STATS_LEFT_COL_PAIR)
			screen.setTableColumnHeader(leftTableName, 2, SASTextScale.labelText(localText.getText("TXT_KEY_PEDIA_SAS_TOTAL_COUNT", ())), self.STATS_LEFT_COL_TOTAL)
			inchart_set_icon_column_headers(screen, leftTableName, 3, TRAIT_STATS_FIXED_LEADER_COLS, TRAIT_STATS_ICON_COL_W)

			for otherTrait, pairCount, totalCount, leaderIds in pairingData:
				iRow = screen.appendTableRow(leftTableName)
				otherTraitInfo = gc.getTraitInfo(otherTrait)

				traitText = SASTextScale.labelText(u"%c %s" % (TraitUtil.getIcon(otherTrait), otherTraitInfo.getDescription()))
				screen.setTableText(leftTableName, 0, iRow, traitText, "", WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_TRAIT, otherTrait, CvUtil.FONT_LEFT_JUSTIFY)
				screen.setTableText(leftTableName, 1, iRow, SASTextScale.labelText(u"%d" % pairCount), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
				screen.setTableText(leftTableName, 2, iRow, SASTextScale.labelText(u"%d" % totalCount), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

				inchart_set_icon_cells(screen, leftTableName, iRow, leaderIds, 3, TRAIT_STATS_FIXED_LEADER_COLS, INCHART_ICON_TYPE_LEADER, {"leaderToCiv": leaderToCiv})

		# === RIGHT TABLE: All trait combinations globally ===
		# <!-- custom: Use cached all-pairs data for efficiency. (Claude Opus 4.5) -->
		allPairsData, (minCount, maxCount), _ = self._getAllTraitPairsData()
		if allPairsData:
			tableWRight = self.STATS_RIGHT_TABLE_W

			rightPanelName = self.top.getNextWidgetName()
			screen.addPanel(rightPanelName, "", "", True, True, self.STATS_RIGHT_X, self.STATS_PANEL_Y, self.STATS_RIGHT_W, self.STATS_PANEL_H, PanelStyles.PANEL_STYLE_BLUE50)

			rightTableName = self.top.getNextWidgetName()
			screen.addTableControlGFC(rightTableName, 4 + TRAIT_STATS_FIXED_LEADER_COLS, self.STATS_RIGHT_TABLE_X, self.STATS_RIGHT_TABLE_Y, tableWRight, self.STATS_RIGHT_TABLE_H, True, False, self.STATS_ROW_H, self.STATS_ROW_H, CHART_TABLE_STYLE)
			screen.enableSort(rightTableName)

			screen.setTableColumnHeader(rightTableName, 0, SASTextScale.labelText(localText.getText("TXT_KEY_PEDIA_SAS_TRAIT_HEADER", ()) + u" 1"), self.STATS_RIGHT_COL_TRAIT1)
			screen.setTableColumnHeader(rightTableName, 1, SASTextScale.labelText(localText.getText("TXT_KEY_PEDIA_SAS_TRAIT_HEADER", ()) + u" 2"), self.STATS_RIGHT_COL_TRAIT2)
			screen.setTableColumnHeader(rightTableName, 2, SASTextScale.labelText(localText.getText("TXT_KEY_PEDIA_SAS_PAIR_COUNT", ())), self.STATS_RIGHT_COL_COUNT)
			screen.setTableColumnHeader(rightTableName, 3, SASTextScale.labelText(localText.getText("TXT_KEY_PEDIA_SAS_RANKING", ())), self.STATS_RIGHT_COL_RANK)
			inchart_set_icon_column_headers(screen, rightTableName, 4, TRAIT_STATS_FIXED_LEADER_COLS, TRAIT_STATS_ICON_COL_W)

			for trait1, trait2, pairCount, leaderIds in allPairsData:
				iRow = screen.appendTableRow(rightTableName)

				trait1Info = gc.getTraitInfo(trait1)
				trait2Info = gc.getTraitInfo(trait2)
				trait1Text = SASTextScale.labelText(u"%c %s" % (TraitUtil.getIcon(trait1), trait1Info.getDescription()))
				trait2Text = SASTextScale.labelText(u"%c %s" % (TraitUtil.getIcon(trait2), trait2Info.getDescription()))
				screen.setTableText(rightTableName, 0, iRow, trait1Text, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				screen.setTableText(rightTableName, 1, iRow, trait2Text, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				screen.setTableText(rightTableName, 2, iRow, SASTextScale.labelText(u"%d" % pairCount), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

				# Ranking bar using centralized helper
				rankingBar = inchart_calc_ranking_bar(pairCount, minCount, maxCount)
				screen.setTableText(rightTableName, 3, iRow, SASTextScale.labelText(rankingBar), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

				inchart_set_icon_cells(screen, rightTableName, iRow, leaderIds, 4, TRAIT_STATS_FIXED_LEADER_COLS, INCHART_ICON_TYPE_LEADER, {"leaderToCiv": leaderToCiv})

	# <!-- custom: _setLeaderIconCells removed - now uses centralized inchart_set_icon_cells from _sevopedia_helpers -->

	def _buildTraitPairingData(self):
		# <!-- custom: Build per-trait pairing data using cached traitLeaders mapping.
		# Much faster than recomputing the mapping each time. (Claude Opus 4.5) -->
		if self.iTrait < 0:
			return []

		cache = TRAIT_STATISTICS_CACHE
		if cache is None:
			return []  # Cache not built yet (shouldn't happen if called after placeTraits)

		traitLeaders = cache["traitLeaders"]
		leadersWithThisTrait = set(traitLeaders.get(self.iTrait, []))
		if not leadersWithThisTrait:
			return []

		pairingData = []
		if IS_SHOW_ZERO_TRAIT_PAIRS_LEFT:
			for otherTrait, leadersWithOther in traitLeaders.items():
				if otherTrait == self.iTrait:
					continue
				leadersWithBoth = leadersWithThisTrait.intersection(set(leadersWithOther))
				pairingData.append((otherTrait, len(leadersWithBoth), len(leadersWithOther), sorted(leadersWithBoth)))
		else:
			for otherTrait, leadersWithOther in traitLeaders.items():
				if otherTrait == self.iTrait:
					continue
				if not leadersWithOther:
					continue
				leadersWithBoth = leadersWithThisTrait.intersection(set(leadersWithOther))
				if leadersWithBoth:
					pairingData.append((otherTrait, len(leadersWithBoth), len(leadersWithOther), sorted(leadersWithBoth)))

		pairingData.sort(key=lambda x: gc.getTraitInfo(x[0]).getDescription())
		return pairingData

	def _getAllTraitPairsData(self):
		# <!-- custom: Return cached all-pairs data.
		# Computed once per session. (Claude Opus 4.5) -->
		cache = TRAIT_STATISTICS_CACHE
		if cache is None:
			return [], (0, 0), 0
		return cache["allPairsData"], cache["allPairsMinMax"], cache["allPairsMaxLeaders"]

	def placeSpecial(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_EFFECTS", ()), "", True, False, self.X_SPECIAL, self.Y_SPECIAL, self.W_SPECIAL, self.H_SPECIAL, PanelStyles.PANEL_STYLE_BLUE50 )
		listName = self.top.getNextWidgetName()
		screen.attachListBoxGFC( panelName, listName, "", TableStyles.TABLE_STYLE_EMPTY )
		screen.enableSelect(listName, False)

		if self.iLeader != -1:
			leader = gc.getLeaderHeadInfo(self.iLeader)
			trait = gc.getTraitInfo(self.iTrait)
			szText = CyGameTextMgr().parseLeaderTraits(self.iLeader, -1, False, True)
			szSpecial = u""
			bFirst = True
			bFound = False
			bSkip = True
			for line in szText.splitlines():
				if not line.startswith(" "):
					# leader or trait
					if line.find(">%s<" % leader.getDescription()) != -1:
						continue # leader, ignore
					elif line.find(">%s<" % trait.getDescription()) != -1:
						# the trait we want
						bFound = True
						bSkip = False
					else:
						# some other trait
						if bFound:
							break
						bSkip = True
				else:
					if not bSkip:
						if bFirst:
							bFirst = False
						else:
							szSpecial += "\n"
						szSpecial += line[2:]  # strip first two spaces
			if bFound:
				szSpecial = SASTextScale.labelText(szSpecial)
				screen.addMultilineText(listName, szSpecial, self.X_SPECIAL+5, self.Y_SPECIAL+32, self.W_SPECIAL-10, self.H_SPECIAL-40, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def setHistoryExpanded(self, bExpanded):
		self.bHistoryExpanded = bExpanded

	# <!-- custom: History panel showing civilopedia text for the trait, similar to SevoPediaImprovement. (Claude code Opus 4.5) -->
	def placeHistory(self):
		screen = self.top.getScreen()
		szText = gc.getTraitInfo(self.iTrait).getCivilopedia()
		szTitle = localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ())
		draw_expandable_text_panel(screen, self.top, szTitle, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, szText, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)
	def handleInput (self, inputClass):
		return 0
