# Game speed chart page for Sevopedia (AdvCiv-SAS)
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Mirrors the handicap chart layout, but for GameSpeedInfo.
#

from CvPythonExtensions import *
import CvUtil
from _sevopedia_helpers import *

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class SevoPediaGameSpeedChart:
	def __init__(self, main):
		self.top = main
		self._cachedTable = None
		self.szCsvLogButton = ""

		self.MARGIN = CHART_TABLE_MARGIN
		self.ROW_H = CHART_TABLE_ROW_H
		self.W_ICON = CHART_TABLE_W_ICON
		iLabelFont = getSASUIFontLabel()
		if iLabelFont <= 1:
			self.W_FIELD = gc.getDefineINT("SAS_SEVOPEDIA_GAME_SPEED_CHART_FIELD_COL_WIDTH_FONT_1")
		elif iLabelFont == 2:
			self.W_FIELD = gc.getDefineINT("SAS_SEVOPEDIA_GAME_SPEED_CHART_FIELD_COL_WIDTH_FONT_2")
		elif iLabelFont == 3:
			self.W_FIELD = gc.getDefineINT("SAS_SEVOPEDIA_GAME_SPEED_CHART_FIELD_COL_WIDTH_FONT_3")
		else:
			self.W_FIELD = gc.getDefineINT("SAS_SEVOPEDIA_GAME_SPEED_CHART_FIELD_COL_WIDTH_FONT_4")
		if self.W_FIELD <= 0:
			raise ValueError("[FATAL] SAS_SEVOPEDIA_GAME_SPEED_CHART_FIELD_COL_WIDTH_FONT_* must be >= 1.")
		iNoReduceMinWidth = gc.getDefineINT("SAS_SEVOPEDIA_GAME_SPEED_CHART_FIELD_NO_REDUCE_MIN_WIDTH")
		if (iNoReduceMinWidth > 0) and (self.top.getScreen().getXResolution() >= iNoReduceMinWidth):
			self.W_FIELD = gc.getDefineINT("SAS_SEVOPEDIA_GAME_SPEED_CHART_FIELD_COL_WIDTH_NO_REDUCE")
			if self.W_FIELD <= 0:
				raise ValueError("[FATAL] SAS_SEVOPEDIA_GAME_SPEED_CHART_FIELD_COL_WIDTH_NO_REDUCE must be >= 1.")
		self.IS_SAS_SEVOPEDIA_GAME_SPEED_CHART_HEADER_ICONS = (gc.getDefineINT("SAS_SEVOPEDIA_GAME_SPEED_CHART_HEADER_ICONS") > 0)
		self.TABLE_FILL_PERCENT = gc.getDefineINT("SAS_SEVOPEDIA_GAME_SPEED_CHART_TABLE_FILL_PERCENT")
		if self.TABLE_FILL_PERCENT <= 0:
			raise ValueError("[FATAL] SAS_SEVOPEDIA_GAME_SPEED_CHART_TABLE_FILL_PERCENT must be >= 1.")

	def interfaceScreen(self):
		screen = self.top.getScreen()
		self._drawTable(screen)
		place_new_concept_legend_link(self.top, "CONCEPT_SAS_SEVOPEDIA_SPEED_CHART_LEGEND")

	def _drawTable(self, screen):
		x = self.top.X_ITEMS
		y = self.top.Y_PEDIA_PAGE
		w = self.top.W_SCREEN - self.top.X_ITEMS - self.MARGIN
		h = self.top.H_PEDIA_PAGE

		screen.addPanel(self.top.getNextWidgetName(), "", "", True, True, x, y, w, h, PanelStyles.PANEL_STYLE_BLUE50)
		screen.addPanel(self.top.getNextWidgetName(), "", "", True, True, x + self.MARGIN, y + self.MARGIN, w - (self.MARGIN * 2), h - (self.MARGIN * 2), PanelStyles.PANEL_STYLE_BLUE50)
		self.szCsvLogButton = chart_add_csv_log_button(screen, self.top, x, y, w)

		table = self.top.getNextWidgetName()
		tableX = x + self.MARGIN
		tableY = y + self.MARGIN + 4
		tableW = w - (self.MARGIN * 2)
		tableH = h - (self.MARGIN * 2) - 4

		data = self._getTableData()
		if not data:
			raise RuntimeError("[FATAL] SevoPediaGameSpeedChart received no table data; this should never happen.")
		header = data[0]
		rows = data[1:]
		nCols = len(header)
		if nCols < 2:
			return

		totalW = (tableW * self.TABLE_FILL_PERCENT) / 100
		if self.IS_SAS_SEVOPEDIA_GAME_SPEED_CHART_HEADER_ICONS:
			remainingW = max(0, totalW - self.W_ICON - self.W_FIELD)
			value_cols = nCols - 2
		else:
			remainingW = max(0, totalW - self.W_FIELD)
			value_cols = nCols - 1
		if value_cols > 0:
			wNum = remainingW / value_cols
		else:
			wNum = 0

		screen.addTableControlGFC(table, nCols, tableX, tableY, tableW, tableH, True, False, self.ROW_H, self.ROW_H, CHART_TABLE_STYLE)
		screen.enableSort(table)

		# Minor Python-level micro-opt: bind methods used in hot loops.
		setHeader = screen.setTableColumnHeader
		appendRow = screen.appendTableRow
		setCell = screen.setTableText

		for iCol in range(nCols):
			if self.IS_SAS_SEVOPEDIA_GAME_SPEED_CHART_HEADER_ICONS and iCol == 0:
				colW = self.W_ICON
			elif (self.IS_SAS_SEVOPEDIA_GAME_SPEED_CHART_HEADER_ICONS and iCol == 1) or (not self.IS_SAS_SEVOPEDIA_GAME_SPEED_CHART_HEADER_ICONS and iCol == 0):
				colW = self.W_FIELD
			else:
				colW = wNum
			setHeader(table, iCol, header[iCol], colW)

		for row in rows:
			iRow = appendRow(table)
			for iCol in range(nCols):
				if iCol < len(row):
					cell = row[iCol]
				else:
					cell = u""

				# Icon column: cell is (text, buttonPath) tuple
				if self.IS_SAS_SEVOPEDIA_GAME_SPEED_CHART_HEADER_ICONS and iCol == 0:
					if isinstance(cell, tuple):
						text = cell[0]
						icon_button = cell[1]
					else:
						text = cell
						icon_button = ""
					if icon_button:
						icon_button = CvUtil.convertToStr(icon_button)
					setCell(table, iCol, iRow, text, icon_button, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

				# Field column (left)
				elif (self.IS_SAS_SEVOPEDIA_GAME_SPEED_CHART_HEADER_ICONS and iCol == 1) or (not self.IS_SAS_SEVOPEDIA_GAME_SPEED_CHART_HEADER_ICONS and iCol == 0):
					setCell(table, iCol, iRow, cell, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

				# Value columns (center)
				else:
					setCell(table, iCol, iRow, cell, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def handleInput(self, inputClass):
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED and self.szCsvLogButton and inputClass.getFunctionName() == self.szCsvLogButton:
			chart_dump_table_csv("SAS_SEVOPEDIA_GAME_SPEED_CHART", self._getTableData())
			return 1
		return 0

	def dumpCsvLog(self):
		chart_dump_table_csv("SAS_SEVOPEDIA_GAME_SPEED_CHART", self._getTableData())

	def _getTableData(self):
		if self._cachedTable is not None:
			return self._cachedTable

		data = self._buildTableFromGameData()
		self._cachedTable = data
		return data

	def _buildTableFromGameData(self):
		# Central configuration (single source of truth)
		#
		# row_specs drives:
		# - which XML fields we display (and in what order)
		# - which getter to call for each field (DLL GameSpeedInfo getters)
		# - which icon (glyph/button) is shown in the left icon column
		#
		# This intentionally replaces the older split setup: field_getters + field_order + icon_token_by_key + glyph_sort_code so future edits only need to touch ONE list.
		#
		# Each entry:
		#   (field_key, getter_name_or_None, icon_spec)
		#
		# icon_spec:
		#   None
		# <!-- custom: ("btn", <name>) -> ArtFileMgr interface art (SAS_EMOJI_*). (GPT-5.2-Codex (summarized)) -->
		#   ("glyph", <name>)   -> GameFont glyph (yield/commerce/symbol char)
		row_specs = (
			("iGrowthPercent",              "getGrowthPercent",              ("glyph", "food")),
			("iTrainPercent",               "getTrainPercent",               ("btn",   "swords")),
			("iConstructPercent",           "getConstructPercent",           ("glyph", "prod")),
			("iCreatePercent",              "getCreatePercent",              ("glyph", "prod")),
			("iResearchPercent",            "getResearchPercent",            ("glyph", "research")),
			("iBuildPercent",               "getBuildPercent",               ("glyph", "citizen")),
			("iImprovementPercent",         "getImprovementPercent",         ("glyph", "citizen")),
			("iGreatPeoplePercent",         "getGreatPeoplePercent",         ("glyph", "great_people")),
			("iCulturePercent",             "getCulturePercent",             ("glyph", "culture")),
			("iAnarchyPercent",             "getAnarchyPercent",             ("btn",   "fire")),
			("iBarbPercent",                "getBarbPercent",                ("btn",   "skull")),
			("iFeatureProductionPercent",   "getFeatureProductionPercent",   ("glyph", "citizen")),
			("iUnitDiscoverPercent",        "getUnitDiscoverPercent",        ("glyph", "research")),
			("iUnitHurryPercent",           "getUnitHurryPercent",           ("glyph", "prod")),
			("iUnitTradePercent",           "getUnitTradePercent",           ("btn",   "swords")),
			("iUnitGreatWorkPercent",       "getUnitGreatWorkPercent",       ("glyph", "culture")),
			("iGoldenAgePercent",           "getGoldenAgePercent",           ("glyph", "golden_age")),
			("iHurryPercent",               "getHurryPercent",               ("glyph", "prod")),
			("iHurryConscriptAngerPercent", "getHurryConscriptAngerPercent", ("glyph", "happy")),
			("iInflationPercent",           "getInflationPercent",           ("glyph", "gold")),
			("iInflationOffset",            "getInflationOffset",            ("glyph", "gold")),
			("iUnitCostPercent",            "getUnitCostPercent",            ("btn",   "swords")),
			("iExtraFreeOutsideUnits",      "getExtraFreeOutsideUnits",      ("btn",   "swords")),
			("iVictoryDelayPercent",        "getVictoryDelayPercent",        ("btn",   "trophy")),
			("iAIMemoryRandPercent",        "getAIMemoryRandPercent",        ("btn",   "brain")),
			("iAIContactRandPercent",       "getAIContactRandPercent",       ("btn",   "dove")),
			("iAIContactDelayPercent",      "getAIContactDelayPercent",      ("btn",   "hourglass")),
			("iFullTradeCreditPercent",     "getFullTradeCreditPercent",     ("glyph", "gold")),
			("iRevoltDivPercent",           "getRevoltDivPercent",           ("btn",   "fire")),
			("iReligionSpreadDivPercent",   "getReligionSpreadDivPercent",   ("btn",   "dove")),
			("iEventRollSidesPercent",      "getEventRollSidesPercent",      ("btn",   "gear")),
			("iVoteIntervalPercent",        "getVoteIntervalPercent",        ("btn",   "dove")),
			("NumTurnIncrements",           None,                            ("btn",   "hourglass")),
			("TotalTurns",                  None,                            ("btn",   "hourglass")),
		)

		# Derived rows have no getter (getter_name is None): values are computed from per-increment data, not read from a single XML field. Mark them with '*' in the display label.
		derived_field_keys = {}
		for (k, getter_name, _icon_spec) in row_specs:
			if getter_name is None:
				derived_field_keys[k] = True

		# Icon libraries
		# Each icon definition includes a "sort group" so sorting by the icon column is meaningful. (The group is also embedded into an invisible tie-breaker so ordering is fully deterministic.)
		game = CyGame()

		# Keep icon libraries lean: define only icons we actually use in this chart.
		# If you add new row_specs icons later, extend the *_defs tuples below.
		#
		# Sorting note: each icon gets a "sort group" used only when sorting by the icon column. Groups are spaced by 10 so you can insert new icons between existing groups without renumbering everything; only relative order matters.

		btn_by_name = {}

		_btn_defs = (("dove", "SAS_EMOJI_DOVE"), ("swords", "SAS_EMOJI_CROSSED_SWORDS"), ("skull", "SAS_EMOJI_SKULL"), ("gear", "SAS_EMOJI_GEAR"), ("brain", "SAS_EMOJI_BRAIN"), ("hourglass", "SAS_EMOJI_HOURGLASS_NOT_DONE"), ("fire", "SAS_EMOJI_FIRE"), ("trophy", "SAS_EMOJI_TROPHY"),)
		for i, (name, artKey) in enumerate(_btn_defs):
			btn_by_name[name] = (ArtFileMgr.getInterfaceArtInfo(artKey).getPath(), (i + 1) * 10)

		glyph_by_name = {}
		# GameFont glyph icons (yields/commerce/symbols).
		_glyph_defs = (
			# (name, glyph_char)
			("food",         (u"%c" % (gc.getYieldInfo(YieldTypes.YIELD_FOOD).getChar()))),
			("prod",         (u"%c" % (gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar()))),
			("gold",         (u"%c" % (gc.getCommerceInfo(CommerceTypes.COMMERCE_GOLD).getChar()))),
			("research",     (u"%c" % (gc.getCommerceInfo(CommerceTypes.COMMERCE_RESEARCH).getChar()))),
			("culture",      (u"%c" % (gc.getCommerceInfo(CommerceTypes.COMMERCE_CULTURE).getChar()))),
			("happy",        (u"%c" % (game.getSymbolID(FontSymbols.HAPPY_CHAR)))),
			("defense",      (u"%c" % (game.getSymbolID(FontSymbols.DEFENSE_CHAR)))),
			("citizen",      (u"%c" % (game.getSymbolID(FontSymbols.CITIZEN_CHAR)))),
			("great_people", (u"%c" % (game.getSymbolID(FontSymbols.GREAT_PEOPLE_CHAR)))),
			("golden_age",   (u"%c" % (game.getSymbolID(getattr(FontSymbols, "GOLDEN_AGE_CHAR", FontSymbols.GREAT_PEOPLE_CHAR))))),
		)
		for i, (name, glyph) in enumerate(_glyph_defs):
			glyph_by_name[name] = (glyph, (i + 1) * 10)

		# Build a quick lookup dict from the central row_specs list.
		# (Still "centralized": this dict is derived from row_specs rather than hand-maintained.)
		icon_spec_by_key = {}
		for (k, _getter, icon_spec) in row_specs:
			icon_spec_by_key[k] = icon_spec

		def icon_cell_for_key(key, iRowIndex):
			icon_spec = icon_spec_by_key.get(key)

			# Calendar rows are derived fields and not part of row_specs: treat them consistently.
			if key.startswith("Calendar_"):
				icon_spec = ("btn", "hourglass")
			if key.startswith("Summary_"):
				icon_spec = ("glyph", "defense")
			if key.startswith("IncrementsYears_") or key.startswith("IncrementsMonths_"):
				icon_spec = ("btn", "hourglass")

			if not icon_spec:
				# No icon: still return a stable, invisible tie-breaker so sorting can't shuffle.
				return (chart_sort_key(0, iRowIndex), "")

			(kind, name) = icon_spec
			if kind == "btn":
				(path, iGroup) = btn_by_name.get(name, ("", 0))
				return (chart_sort_key(iGroup, iRowIndex), path)

			# kind == "glyph"
			(glyph, iGroup) = glyph_by_name.get(name, ("", 0))
			return (chart_font2(glyph) + chart_sort_key(iGroup, iRowIndex), "")

		# Extract data from DLL infos
		# Collect available game speed types in their DLL order.
		speed_types = []
		for i in xrange(gc.getNumGameSpeedInfos()):
			info = gc.getGameSpeedInfo(i)
			speed_types.append(info.getType())

		# Column labels: "Normal (660 / 100%)"
		speed_labels = []
		for i in xrange(gc.getNumGameSpeedInfos()):
			info = gc.getGameSpeedInfo(i)
			szName = chart_get_info_display_name(info)
			iNumInc = info.getNumTurnIncrements()
			iTurns = 0
			for iInc in xrange(iNumInc):
				iTurns += info.getGameTurnInfo(iInc).iNumGameTurnsPerIncrement
			iGrowth = info.getGrowthPercent()
			speed_labels.append("%s (%d / %d%%)" % (szName, iTurns, iGrowth))

		# Fill base fields from getters.
		parsed_data = {}
		for i in xrange(gc.getNumGameSpeedInfos()):
			info = gc.getGameSpeedInfo(i)
			speed_type = info.getType()
			speed_dict = {}

			for (field, getter, _icon_spec) in row_specs:
				if field.startswith("i") and getter:
					speed_dict[field] = str(getattr(info, getter)())

			parsed_data[speed_type] = speed_dict

		# Derived summary rows: NumTurnIncrements / TotalTurns; plus Calendar rows (Calendar_01 .. Calendar_NN).
		start_year = gc.getDefineINT("START_YEAR")  # not cached: read once only
		max_increments = 0
		total_turns_by_speed = {}

		for i in xrange(gc.getNumGameSpeedInfos()):
			info = gc.getGameSpeedInfo(i)
			speed_type = info.getType()

			iNumInc = info.getNumTurnIncrements()
			parsed_data[speed_type]["NumTurnIncrements"] = str(iNumInc)
			if iNumInc > max_increments:
				max_increments = iNumInc

			iTotal = 0
			for iInc in xrange(iNumInc):
				iTotal += info.getGameTurnInfo(iInc).iNumGameTurnsPerIncrement
			parsed_data[speed_type]["TotalTurns"] = str(iTotal)
			total_turns_by_speed[speed_type] = iTotal

		years_chunk_size = 3
		months_chunk_size = 3
		years_chunk_count = (max_increments + years_chunk_size - 1) / years_chunk_size
		months_chunk_count = (max_increments + months_chunk_size - 1) / months_chunk_size

		years_increment_fields = []
		months_increment_fields = []
		display_label_by_field = {}
		for iChunk in xrange(years_chunk_count):
			szField = "IncrementsYears_%02d" % (iChunk + 1)
			years_increment_fields.append(szField)
			display_label_by_field[szField] = "Inc Y %02d" % (iChunk + 1)
		for iChunk in xrange(months_chunk_count):
			szField = "IncrementsMonths_%02d" % (iChunk + 1)
			months_increment_fields.append(szField)
			display_label_by_field[szField] = "Inc M %02d" % (iChunk + 1)

		# Additional derived increment rows split across chunks for readability.
		for i in xrange(gc.getNumGameSpeedInfos()):
			info = gc.getGameSpeedInfo(i)
			speed_type = info.getType()
			iNumInc = info.getNumTurnIncrements()
			compact_incs = []
			raw_incs = []
			for iInc in xrange(iNumInc):
				turn_info = info.getGameTurnInfo(iInc)
				iMonthInc = turn_info.iMonthIncrement
				compact_incs.append(self._format_increment_compact(iMonthInc))
				raw_incs.append(str(iMonthInc))

			for iChunk in xrange(years_chunk_count):
				iStart = iChunk * years_chunk_size
				iEnd = iStart + years_chunk_size
				szField = years_increment_fields[iChunk]
				parsed_data[speed_type][szField] = ", ".join(compact_incs[iStart:iEnd])
			for iChunk in xrange(months_chunk_count):
				iStart = iChunk * months_chunk_size
				iEnd = iStart + months_chunk_size
				szField = months_increment_fields[iChunk]
				parsed_data[speed_type][szField] = ", ".join(raw_incs[iStart:iEnd])

		# Calendar rows: Calendar_01 .. Calendar_NN
		calendar_fields = []
		for iRow in xrange(max_increments):
			# %02d keeps the displayed "Calendar 01" order stable and nicely aligned.
			szField = "Calendar_%02d" % (iRow + 1)
			calendar_fields.append(szField)
			display_label_by_field[szField] = "Cal %02d" % (iRow + 1)

		for i in xrange(gc.getNumGameSpeedInfos()):
			info = gc.getGameSpeedInfo(i)
			speed_type = info.getType()

			iNumInc = info.getNumTurnIncrements()
			cum_months = 0
			# Era labels are omitted for compactness (BC is implied before the transition, AD after).
			# To still make the BC->AD boundary obvious, we mark the *first* segment that reaches AD
			# with an arrow ("->") instead of "=".
			bReachedAD = (start_year >= 0)
			for iInc in xrange(iNumInc):
				turn_info = info.getGameTurnInfo(iInc)
				iTurns = turn_info.iNumGameTurnsPerIncrement
				iMonthInc = turn_info.iMonthIncrement

				iSegMonths = iTurns * iMonthInc
				iStartYear = start_year + (cum_months / 12)
				iEndTotal = cum_months + iSegMonths
				iEndYear = start_year + (iEndTotal / 12)
				iEndMonth = (iEndTotal % 12) + 1

				bEraFlip = (not bReachedAD) and (iStartYear < 0) and (iEndYear >= 0)
				szCell = self._format_calendar_segment(iEndYear, iEndMonth, iTurns, iMonthInc, bEraFlip)
				parsed_data[speed_type][calendar_fields[iInc]] = szCell
				cum_months = iEndTotal
				if iEndYear >= 0:
					bReachedAD = True

		summary_step_percent = 5
		summary_percents = []
		iPct = summary_step_percent
		while iPct <= 100:
			summary_percents.append(iPct)
			iPct += summary_step_percent
		if not summary_percents or summary_percents[-1] != 100:
			summary_percents.append(100)

		summary_fields = []
		for iSummary in xrange(len(summary_percents)):
			iPercent = summary_percents[iSummary]
			szField = "Summary_%02d" % (iSummary + 1)
			summary_fields.append(szField)
			display_label_by_field[szField] = "S %02d (%d%%)" % (iSummary + 1, iPercent)

		for i in xrange(gc.getNumGameSpeedInfos()):
			info = gc.getGameSpeedInfo(i)
			speed_type = info.getType()
			iTotalTurns = total_turns_by_speed.get(speed_type, 0)
			for iSummary in xrange(len(summary_percents)):
				iPercent = summary_percents[iSummary]
				szField = summary_fields[iSummary]
				iTargetTurn = self._percent_to_turn(iTotalTurns, iPercent)
				(iYear, iMonth, iMonthInc) = self._turn_to_date_with_increment(info, start_year, iTargetTurn)
				parsed_data[speed_type][szField] = self._format_summary_cell(iTargetTurn, iYear, iMonth, iMonthInc)

		# Display order:
		# - base rows (row_specs order)
		# - calendar rows appended right after TotalTurns
		field_order = []
		for (k, _getter, _icon_spec) in row_specs:
				field_order.append(k)
				if k == "TotalTurns":
					field_order.extend(years_increment_fields)
					field_order.extend(months_increment_fields)
					field_order.extend(calendar_fields)
					field_order.extend(summary_fields)

		# Build table
		table = []
		header = [""]
		if self.IS_SAS_SEVOPEDIA_GAME_SPEED_CHART_HEADER_ICONS:
			header.append("")
		for sz in speed_labels:
			header.append(chart_font2(sz))
		table.append(header)

		row_index = 0
		for field in field_order:
			szFieldName = display_label_by_field.get(field, chart_beautify_field_name(field))
			if derived_field_keys.get(field):
				szFieldName += u"*"

			if self.IS_SAS_SEVOPEDIA_GAME_SPEED_CHART_HEADER_ICONS:
				row = [icon_cell_for_key(field, row_index), chart_font2(szFieldName)]
			else:
				row = [chart_font2(szFieldName)]

			for speed_type in speed_types:
				row.append(chart_font2(parsed_data.get(speed_type, {}).get(field, "")))

			table.append(row)
			row_index += 1

		# Culture level thresholds (scaled by iCulturePercent) appended at bottom.
		for iLevel in xrange(gc.getNumCultureLevelInfos()):
			info = gc.getCultureLevelInfo(iLevel)
			szName = localText.getText(str(info.getTextKey()), ())

			if self.IS_SAS_SEVOPEDIA_GAME_SPEED_CHART_HEADER_ICONS:
				row = [icon_cell_for_key("iCulturePercent", row_index), chart_font2(szName)]
			else:
				row = [chart_font2(szName)]

			for iSpeed in xrange(len(speed_types)):
				row.append(chart_font2(str(info.getSpeedThreshold(iSpeed))))

			table.append(row)
			row_index += 1

		return table

	def _format_year_magnitude(self, iAbsYear):
		# Use k notation only for clean multiples >= 10,000 (so 1,480 BC stays 1480 BC).
		if iAbsYear >= 10000 and (iAbsYear % 1000) == 0:
			return "%dk" % (iAbsYear / 1000)
		return str(iAbsYear)

	def _format_year_label(self, iYear):
		# Compact year label without era text.
		# In the calendar table we omit "BC"/"AD" everywhere:
		# - Before the BC->AD transition, years are implicitly BC.
		# - The first segment that reaches AD is marked with "->" in the cell.
		# - After that, years are implicitly AD.
		if iYear < 0:
			return self._format_year_magnitude(-iYear)
		return self._format_year_magnitude(iYear)

	def _format_date_label(self, iYear, iMonth):
		# Date label: <year>[m2..m12]
		# <!-- custom: hide m1 so January matches in-game year display, but keep m2..m12 visible to diagnose calendar drift. (GPT-5.3-Codex) -->
		sz = self._format_year_label(iYear)
		if iMonth == 1:
			return sz
		return "%sm%d" % (sz, iMonth)

	def _format_years_per_turn(self, iYearsPerTurn):
		# Compact: 80 -> "80", 1250 -> "1.2k", 10000 -> "10k"
		if iYearsPerTurn < 1000:
			return str(iYearsPerTurn)
		if (iYearsPerTurn % 1000) == 0:
			return "%dk" % (iYearsPerTurn / 1000)
		if (iYearsPerTurn % 100) == 0:
			sz = "%.1f" % (iYearsPerTurn / 1000.0)
			if sz.endswith(".0"):
				sz = sz[:-2]
			return sz + "k"
		return str(iYearsPerTurn)

	def _format_rate_per_turn(self, month_inc):
		# Returns (op, rate_str):
		# - Exact years: op='*', rate='5' or '10k'
		# - Months < 12: op='*', rate='m6' (so "2*m6" means 2 turns of 6 months)
		# - Year+month: op='*', rate='187m6' (so "2*187m6" means 2 turns of 187 years + 6 months)
		if month_inc <= 0:
			return ("*", "0")
		if month_inc < 12:
			return ("*", "m%d" % month_inc)
		if (month_inc % 12) == 0:
			years_per_turn = month_inc / 12
			return ("*", self._format_years_per_turn(years_per_turn))
		years_per_turn = month_inc / 12
		rem_months = month_inc % 12
		szYears = self._format_years_per_turn(years_per_turn)
		return ("*", "%sm%d" % (szYears, rem_months))

	def _format_increment_compact(self, month_inc):
		if month_inc <= 0:
			return "0"
		if month_inc < 12:
			return "m%d" % month_inc
		if (month_inc % 12) == 0:
			return self._format_years_per_turn(month_inc / 12)
		iYears = month_inc / 12
		iRemMonths = month_inc % 12
		return "%sm%d" % (self._format_years_per_turn(iYears), iRemMonths)

	def _format_calendar_segment(self, end_year, end_month, turns, month_inc, bEraFlip):
		# Calendar cell format is intentionally compact:
		#   "+<turns>*<rate><sep><endDate>"
		# We omit the segment start date entirely to save horizontal space. (The start date is implied by the previous segment; for the first segment, players already know START_YEAR / can look it up in XML.)
		#
		# Era handling:
		# - We omit "BC"/"AD" text entirely (BC is implied before the transition, AD after).
		# - We mark the *first* segment that reaches AD with "->" (instead of "=") so the BC->AD boundary is obvious without repeating era text in every cell. (This is also where years are shortest, so the marker doesn't bloat the row.)
		#
		# Month handling:
		# - Month suffix is shown for m2..m12; m1 is hidden so January matches in-game year display.
		#
		# Examples:
		# - "+2*10k=30k"              (still BC; era implied)
		# - "+3*m6->0"                (first segment that reaches AD)
		# - "+3*m6=1901m7"            (later AD; era implied)
		# - "+1*m3=2069m12"           (December is shown; it is not the same as 2070)
		szEnd = self._format_date_label(end_year, end_month)
		(szOp, szRate) = self._format_rate_per_turn(month_inc)
		if bEraFlip:
			szSep = "->"
		else:
			szSep = "="
		return "+%d%s%s%s%s" % (turns, szOp, szRate, szSep, szEnd)

	def _percent_to_turn(self, iTotalTurns, iPercent):
		if iTotalTurns <= 0:
			return 0
		if iPercent <= 0:
			return 0
		if iPercent >= 100:
			return iTotalTurns
		iTarget = (iTotalTurns * iPercent + 99) / 100
		if iTarget < 1:
			return 1
		if iTarget > iTotalTurns:
			return iTotalTurns
		return iTarget

	def _turn_to_date(self, info, iStartYear, iTurn):
		if iTurn <= 0:
			return (iStartYear, 1)

		iRemainingTurns = iTurn
		iTotalMonths = 0
		for iInc in xrange(info.getNumTurnIncrements()):
			turn_info = info.getGameTurnInfo(iInc)
			iTurnsInInc = turn_info.iNumGameTurnsPerIncrement
			iMonthInc = turn_info.iMonthIncrement

			if iRemainingTurns <= iTurnsInInc:
				iTotalMonths += iRemainingTurns * iMonthInc
				break

			iTotalMonths += iTurnsInInc * iMonthInc
			iRemainingTurns -= iTurnsInInc

		iYear = iStartYear + (iTotalMonths / 12)
		iMonth = (iTotalMonths % 12) + 1
		return (iYear, iMonth)

	def _turn_to_date_with_increment(self, info, iStartYear, iTurn):
		if iTurn <= 0:
			return (iStartYear, 1, 0)

		iRemainingTurns = iTurn
		iTotalMonths = 0
		iCurrentMonthInc = 0
		for iInc in xrange(info.getNumTurnIncrements()):
			turn_info = info.getGameTurnInfo(iInc)
			iTurnsInInc = turn_info.iNumGameTurnsPerIncrement
			iMonthInc = turn_info.iMonthIncrement

			if iRemainingTurns <= iTurnsInInc:
				iTotalMonths += iRemainingTurns * iMonthInc
				iCurrentMonthInc = iMonthInc
				break

			iTotalMonths += iTurnsInInc * iMonthInc
			iRemainingTurns -= iTurnsInInc
			iCurrentMonthInc = iMonthInc

		iYear = iStartYear + (iTotalMonths / 12)
		iMonth = (iTotalMonths % 12) + 1
		return (iYear, iMonth, iCurrentMonthInc)

	def _format_summary_cell(self, iTurn, iYear, iMonth, iMonthInc):
		# <!-- custom: use signed year (+/-) plus active month increment to make checkpoint tuning easier across speeds while keeping cells compact. (GPT-5.3-Codex) -->
		return "T%d=%s (%d)" % (iTurn, self._format_date_label_signed(iYear, iMonth), iMonthInc)

	def _format_date_label_signed(self, iYear, iMonth):
		if iYear < 0:
			szYear = "-" + self._format_year_magnitude(-iYear)
		else:
			szYear = "+" + self._format_year_magnitude(iYear)
		if iMonth != 1:
			szYear = "%sm%d" % (szYear, iMonth)
		return szYear

	def _format_date_label_with_era(self, iYear, iMonth):
		iAbsYear = iYear
		szEra = "AD"
		if iYear < 0:
			iAbsYear = -iYear
			szEra = "BC"

		szYear = self._format_year_magnitude(iAbsYear)
		if iMonth != 1:
			szYear = "%sm%d" % (szYear, iMonth)
		return "%s %s" % (szYear, szEra)
