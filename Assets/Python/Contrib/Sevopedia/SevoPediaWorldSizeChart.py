# World size chart page for Sevopedia (AdvCiv-SAS)
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Mirrors the handicap chart layout, but for WorldInfo.
#

from CvPythonExtensions import *
import CvUtil
from _sevopedia_helpers import *

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class SevoPediaWorldSizeChart:
	def __init__(self, main):
		self.top = main
		self._cachedTable = None
		self.szCsvLogButton = ""

		self.MARGIN = CHART_TABLE_MARGIN
		self.ROW_H = CHART_TABLE_ROW_H
		self.W_ICON = CHART_TABLE_W_ICON
		self.W_FIELD = 280
		self.IS_SAS_SEVOPEDIA_WORLD_SIZE_CHART_HEADER_ICONS = (gc.getDefineINT("SAS_SEVOPEDIA_WORLD_SIZE_CHART_HEADER_ICONS") > 0)
		self.TABLE_FILL_PERCENT = gc.getDefineINT("SAS_SEVOPEDIA_WORLD_SIZE_CHART_TABLE_FILL_PERCENT")
		if self.TABLE_FILL_PERCENT <= 0:
			raise ValueError("[FATAL] SAS_SEVOPEDIA_WORLD_SIZE_CHART_TABLE_FILL_PERCENT must be >= 1.")

	def interfaceScreen(self):
		screen = self.top.getScreen()
		self._drawTable(screen)
		place_new_concept_legend_link(self.top, "CONCEPT_SAS_SEVOPEDIA_WORLD_SIZE_CHART_LEGEND")

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
			raise RuntimeError("[FATAL] SevoPediaWorldSizeChart received no table data; this should never happen.")
		header = data[0]
		rows = data[1:]
		nCols = len(header)
		if nCols < 2:
			return

		totalW = (tableW * self.TABLE_FILL_PERCENT) / 100
		if self.IS_SAS_SEVOPEDIA_WORLD_SIZE_CHART_HEADER_ICONS:
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
			if self.IS_SAS_SEVOPEDIA_WORLD_SIZE_CHART_HEADER_ICONS and iCol == 0:
				colW = self.W_ICON
			elif (self.IS_SAS_SEVOPEDIA_WORLD_SIZE_CHART_HEADER_ICONS and iCol == 1) or (not self.IS_SAS_SEVOPEDIA_WORLD_SIZE_CHART_HEADER_ICONS and iCol == 0):
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
				if self.IS_SAS_SEVOPEDIA_WORLD_SIZE_CHART_HEADER_ICONS and iCol == 0:
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
				elif (self.IS_SAS_SEVOPEDIA_WORLD_SIZE_CHART_HEADER_ICONS and iCol == 1) or (not self.IS_SAS_SEVOPEDIA_WORLD_SIZE_CHART_HEADER_ICONS and iCol == 0):
					setCell(table, iCol, iRow, cell, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

				# Value columns (center)
				else:
					setCell(table, iCol, iRow, cell, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def handleInput(self, inputClass):
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED and self.szCsvLogButton and inputClass.getFunctionName() == self.szCsvLogButton:
			chart_dump_table_csv("SAS_SEVOPEDIA_WORLD_SIZE_CHART", self._getTableData())
			return 1
		return 0

	def dumpCsvLog(self):
		chart_dump_table_csv("SAS_SEVOPEDIA_WORLD_SIZE_CHART", self._getTableData())

	def _getTableData(self):
		if self._cachedTable is not None:
			return self._cachedTable

		data = self._buildTableFromGameData()
		self._cachedTable = data
		return data

	def _buildTableFromGameData(self):
		row_specs = (
			# NOTE: Some rows are direct XML fields, others are convenience composites.
			# Use a trailing '*' in the *display label* when the displayed value is not a single XML tag.
			# <!-- custom: CvWorldInfo GridWidth/GridHeight are terrain-cell units.
			# Ordinary map generation doubles each axis into plots, while custom map scripts can override the dimensions and scaling; label their product as cells rather than tiles/plots. See KI#305. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			# <!-- custom: use "Usual" to clarify the grid size may change (e.g., depending on mapscripts like SAS_Longworld being low height and high width) -->
			# (field_name, display_label_or_None, getter_name_or_None, icon_token)
			("GridSize",                       "Usual Grid Cells W x H",        None,                               "glyph:map"),
			("GridRatio",                      "W/H Ratio*",                 None,                               "glyph:map"),
			("RatioToStandard",                "Ratio to Standard*",        None,                               "glyph:map"),
			("RatioToLargest",                 "Ratio to Largest*",         None,                               "glyph:map"),
			("GridCells",                      "WorldInfo Grid Cells*",     None,                               "glyph:map"),
			("CellsPerDefaultPlayer",          "Cells Per Default Player*", None,                               "glyph:map"),
			("iDefaultPlayers",                "Default Players",           "getDefaultPlayers",                "glyph:citizen"),
			("iTargetNumCities",               "Target Num Cities",         "getTargetNumCities",               "glyph:citizen"),
			("iNumFreeBuildingBonuses",        "Free Building Bonuses",     "getNumFreeBuildingBonuses",        "glyph:prod"),
			("iBuildingClassPrereqModifier",   "Building Class Prereq Mod", "getBuildingClassPrereqModifier",   "glyph:prod"),
			("iUnitNameModifier",              "Unit Name Modifier",        "getUnitNameModifier",              "btn:swords"),
			("iMaxConscriptModifier",          "Max Conscript Mod",         "getMaxConscriptModifier",          "btn:swords"),
			("iWarWearinessModifier",          "War Weariness Mod",         "getWarWearinessModifier",          "glyph:unhappy"),
			("iUnitCostPercent",               None,                        "getUnitCostPercent",               "btn:swords"),
			("iTerrainGrainChange",            None,                        "getTerrainGrainChange",            "btn:herb"),
			("iFeatureGrainChange",            None,                        "getFeatureGrainChange",            "btn:herb"),
			("iResearchPercent",               None,                        "getResearchPercent",               "glyph:research"),
			("iTradeProfitPercent",            None,                        "getTradeProfitPercent",            "glyph:gold"),
			("iDistanceMaintenancePercent",    None,                        "getDistanceMaintenancePercent",    "glyph:gold"),
			("iNumCitiesMaintenancePercent",   None,                        "getNumCitiesMaintenancePercent",   "glyph:gold"),
			("iColonyMaintenancePercent",      None,                        "getColonyMaintenancePercent",      "glyph:gold"),
			("iCorporationMaintenancePercent", None,                        "getCorporationMaintenancePercent", "glyph:gold"),
			("iNumCitiesAnarchyPercent",       None,                        "getNumCitiesAnarchyPercent",       "btn:fire"),
			("iAdvancedStartPointsMod",        "Advanced Start Points Mod", "getAdvancedStartPointsMod",        "glyph:defense"),
		)

		field_getters_list = []
		for field_name, _display_label, getter_name, _icon_token in row_specs:
			if getter_name:
				field_getters_list.append((field_name, getter_name))
		field_getters = tuple(field_getters_list)

		if gc.getNumWorldInfos() > 0:
			info = gc.getWorldInfo(0)
			missing = []
			for field_name, getter_name in field_getters:
				if not hasattr(info, getter_name):
					missing.append(getter_name)
			if not hasattr(info, "getGridWidth"):
				missing.append("getGridWidth")
			if not hasattr(info, "getGridHeight"):
				missing.append("getGridHeight")
			if missing:
				raise RuntimeError("[FATAL] Your mod DLL does not expose the required CvWorldInfo Python getters: %s. Please expose them in CyInfoInterface2.cpp and rebuild the DLL." % ", ".join(missing))

		game = CyGame()
		btn_by_name = {}

		_btn_defs = (
			# <!-- custom: (name, SAS_EMOJI_* interface art key). (GPT-5.2-Codex (summarized)) -->
			("swords", "SAS_EMOJI_CROSSED_SWORDS"),
			("herb",   "SAS_EMOJI_HERB"),
			("fire",   "SAS_EMOJI_FIRE"),
		)
		for i, (name, artKey) in enumerate(_btn_defs):
			# Group id only affects sorting by the icon column.
			# Reorder _btn_defs to change the icon-theme sort order.
			btn_by_name[name] = (ArtFileMgr.getInterfaceArtInfo(artKey).getPath(), (i + 1) * 10)

		glyph_by_name = {}
		# GameFont glyph icons (yields/commerce/symbols).
		_glyph_defs = (
			# (name, glyph_char)
			("map",      (u"%c" % (game.getSymbolID(FontSymbols.MAP_CHAR)))),
			("citizen",  (u"%c" % (game.getSymbolID(FontSymbols.CITIZEN_CHAR)))),
			("prod",     (u"%c" % (gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar()))),
			("gold",     (u"%c" % (gc.getCommerceInfo(CommerceTypes.COMMERCE_GOLD).getChar()))),
			("research", (u"%c" % (gc.getCommerceInfo(CommerceTypes.COMMERCE_RESEARCH).getChar()))),
			("defense",  (u"%c" % (game.getSymbolID(FontSymbols.DEFENSE_CHAR)))),
			("unhappy",  (u"%c" % (game.getSymbolID(FontSymbols.UNHAPPY_CHAR)))),
		)
		for name, glyph in _glyph_defs:
			glyph_by_name[name] = glyph

		icon_token_by_key = {}
		for field_name, _display_label, _getter_name, icon_token in row_specs:
			if icon_token:
				icon_token_by_key[field_name] = icon_token

		display_label_by_key = {}
		for field_name, display_label, _getter_name, _icon_token in row_specs:
			if display_label:
				display_label_by_key[field_name] = display_label

		def icon_cell_for_key(icon_key, iRowIndex):
			# Always return a stable invisible tie-breaker, even if no icon is found.
			if not icon_key:
				return (chart_sort_key(0, iRowIndex), "")
			token = icon_token_by_key.get(icon_key, "")
			if not token:
				return (chart_sort_key(0, iRowIndex), "")
			if token.startswith("btn:"):
				name = token[4:]
				btn = btn_by_name.get(name)
				if btn is None:
					return (chart_sort_key(0, iRowIndex), "")
				path, iGroup = btn[0], btn[1]
				if not path:
					return (chart_sort_key(0, iRowIndex), "")
				return (chart_sort_key(iGroup, iRowIndex), path)
			if token.startswith("glyph:"):
				name = token[6:]
				glyph = glyph_by_name.get(name, u"")
				if not glyph:
					return (chart_sort_key(0, iRowIndex), "")
				return (chart_font2(glyph) + chart_sort_key(0, iRowIndex), "")
			return (chart_sort_key(0, iRowIndex), "")

		world_types = []
		world_labels = []
		parsed_data = {}

		for iWorld in xrange(gc.getNumWorldInfos()):
			info = gc.getWorldInfo(iWorld)
			world_type = info.getType()
			world_types.append(world_type)
			world_labels.append(chart_get_info_display_name(info))

			world_dict = {}
			for field_name, getter_name in field_getters:
				getter = getattr(info, getter_name)
				world_dict[field_name] = str(getter())

			grid_w = info.getGridWidth()
			grid_h = info.getGridHeight()
			world_dict["GridSize"] = "%d x %d" % (grid_w, grid_h)
			world_dict["GridRatio"] = self._format_ratio(grid_w, grid_h)
			world_dict["GridCells"] = str(grid_w * grid_h)

			parsed_data[world_type] = world_dict

		# Derived rows (AdvCiv-SAS: XXL-inspired extra world sizes, adjusted for SAS; e.g. SAS24/SAS32/SAS40/SAS48)
		# Computed fields are marked in the UI with a trailing \"*\".
		# Added with help from ChatGPT (GPT-5.2 Thinking)
		iStandardCells = None
		if parsed_data.has_key("WORLDSIZE_STANDARD"):
			try:
				iStandardCells = int(parsed_data["WORLDSIZE_STANDARD"].get("GridCells", "0"))
			except:
				iStandardCells = None

		iLargestCells = None
		if world_types:
			try:
				iLargestCells = int(parsed_data.get(world_types[-1], {}).get("GridCells", "0"))
			except:
				iLargestCells = None

		# Ratio to Standard: GridCells / Standard GridCells (3 decimals).
		# Ratio to Largest: GridCells / Largest GridCells (3 decimals).
		# Cells Per Default Player: GridCells / iDefaultPlayers (rounded to int).
		for world_type in world_types:
			try:
				iCells = int(parsed_data.get(world_type, {}).get("GridCells", "0"))
			except:
				iCells = 0

			# Ratio
			if iStandardCells is not None and iStandardCells > 0:
				parsed_data[world_type]["RatioToStandard"] = ("%.3f" % (float(iCells) / float(iStandardCells)))
			else:
				parsed_data[world_type]["RatioToStandard"] = ""

			if iLargestCells is not None and iLargestCells > 0:
				parsed_data[world_type]["RatioToLargest"] = ("%.3f" % (float(iCells) / float(iLargestCells)))
			else:
				parsed_data[world_type]["RatioToLargest"] = ""

			# Cells per default player
			try:
				iDefaultPlayers = int(parsed_data.get(world_type, {}).get("iDefaultPlayers", "0"))
			except:
				iDefaultPlayers = 0
			if iCells > 0 and iDefaultPlayers > 0:
				parsed_data[world_type]["CellsPerDefaultPlayer"] = str(int(round(float(iCells) / float(iDefaultPlayers))))
			else:
				parsed_data[world_type]["CellsPerDefaultPlayer"] = ""

		header = []
		if self.IS_SAS_SEVOPEDIA_WORLD_SIZE_CHART_HEADER_ICONS:
			header.append(u"")
			header.append(chart_font2("Field"))
		else:
			header.append(chart_font2("Field"))
		for label in world_labels:
			header.append(chart_font2(label))

		table = [header]

		row_index = 0
		for field_name, display_label, _getter_name, _icon_token in row_specs:
			szFieldName = display_label_by_key.get(field_name, chart_beautify_field_name(field_name))

			if self.IS_SAS_SEVOPEDIA_WORLD_SIZE_CHART_HEADER_ICONS:
				row = [icon_cell_for_key(field_name, row_index), chart_font2(szFieldName)]
			else:
				row = [chart_font2(szFieldName)]

			for world_type in world_types:
				row.append(chart_font2(parsed_data.get(world_type, {}).get(field_name, "")))

			table.append(row)
			row_index += 1

		return table

	def _format_ratio(self, width, height):
		if width <= 0 or height <= 0:
			return ""
		return "%.4f" % (float(width) / float(height))
