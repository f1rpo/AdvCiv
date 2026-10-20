# Era chart page for Sevopedia (AdvCiv-SAS)
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Mirrors the handicap chart layout, but for EraInfo.
#

from CvPythonExtensions import *
# <!-- custom: Route shared visual resources through the centralized SASVisualUtils namespace after retiring overlapping helper/cache imports; keep one canonical visual-resource boundary. (ChatGPT-5.6-Sol) -->
import SASVisualUtils as SASVisuals
from SASMagicNumbers import *
import CvUtil
# <!-- custom: SevoScreenEnums.py was removed; import its unchanged page IDs from the SevoScreenEnums class in CvScreenEnums.py, keeping them distinct from vanilla Civilopedia pages. (GPT-6.1-Sol) -->
from CvScreenEnums import SevoScreenEnums
from _sevopedia_helpers import *

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class SevoPediaEraChart:
	def __init__(self, main):
		self.top = main
		self._cachedTable = None
		self.szCsvLogButton = ""

		self.MARGIN = CHART_TABLE_MARGIN
		self.ROW_H = CHART_TABLE_ROW_H
		self.W_ICON = CHART_TABLE_W_ICON
		self.W_FIELD = 290
		self.IS_SAS_SEVOPEDIA_ERA_CHART_HEADER_ICONS = (gc.getDefineINT("SAS_SEVOPEDIA_ERA_CHART_HEADER_ICONS") > 0)
		self.TABLE_FILL_PERCENT = gc.getDefineINT("SAS_SEVOPEDIA_ERA_CHART_TABLE_FILL_PERCENT")
		if self.TABLE_FILL_PERCENT <= 0:
			raise ValueError("[FATAL] SAS_SEVOPEDIA_ERA_CHART_TABLE_FILL_PERCENT must be >= 1.")
		# <!-- custom: Toggle to show/hide era button row above the table. (Claude Opus 4.5) -->
		self.SHOW_ERA_BUTTONS = (gc.getDefineINT("SAS_SEVOPEDIA_ERA_CHART_SHOW_ERA_BUTTONS") > 0)

		# <!-- custom: Cache emoji icon path for performance. (GPT-5.2-Codex (summarized)) -->

	def interfaceScreen(self):
		screen = self.top.getScreen()
		self._drawTable(screen)

	def _drawTable(self, screen):
		x = self.top.X_ITEMS
		y = self.top.Y_PEDIA_PAGE
		w = self.top.W_SCREEN - self.top.X_ITEMS - self.MARGIN
		h = self.top.H_PEDIA_PAGE

		screen.addPanel(self.top.getNextWidgetName(), "", "", True, True, x, y, w, h, PanelStyles.PANEL_STYLE_BLUE50)
		screen.addPanel(self.top.getNextWidgetName(), "", "", True, True, x + self.MARGIN, y + self.MARGIN, w - (self.MARGIN * 2), h - (self.MARGIN * 2), PanelStyles.PANEL_STYLE_BLUE50)
		self.szCsvLogButton = chart_add_csv_log_button(screen, self.top, x, y, w)

		data = self._getTableData()
		if not data:
			raise RuntimeError("[FATAL] SevoPediaEraChart received no table data; this should never happen.")
		header = data[0]
		rows = data[1:]
		nCols = len(header)
		if nCols < 2:
			return

		# Calculate column widths
		tableX = x + self.MARGIN
		tableW = w - (self.MARGIN * 2)
		totalW = (tableW * self.TABLE_FILL_PERCENT) / 100
		if self.IS_SAS_SEVOPEDIA_ERA_CHART_HEADER_ICONS:
			remainingW = max(0, totalW - self.W_ICON - self.W_FIELD)
			value_cols = nCols - 2
		else:
			remainingW = max(0, totalW - self.W_FIELD)
			value_cols = nCols - 1
		if value_cols > 0:
			wNum = remainingW / value_cols
		else:
			wNum = 0

		# <!-- custom: Draw era button icons above the table; size matches column width. (Claude Opus 4.5) -->
		buttonRowH = 0
		if self.SHOW_ERA_BUTTONS:
			buttonSize = wNum
			buttonRowH = buttonSize + 8

			# Position for button row: inside inner panel
			buttonRowY = y + self.MARGIN + 8
			# Calculate X offset for first era column
			if self.IS_SAS_SEVOPEDIA_ERA_CHART_HEADER_ICONS:
				buttonRowStartX = tableX + self.W_ICON + self.W_FIELD
			else:
				buttonRowStartX = tableX + self.W_FIELD

			if SASVisuals.InterfaceArt.SAS_EMOJI_PLAY_BUTTON:
				playButtonX = buttonRowStartX - wNum
				# <!-- custom: redirect to first era track (Track 01 of Ancient era) in Sevopedia Music category -->
				iFirstEraMusicKey = self.top.SAS_getFirstEraMusicKey()
				if iFirstEraMusicKey != -1:
					screen.setImageButton(self.top.getNextWidgetName(), SASVisuals.InterfaceArt.SAS_EMOJI_PLAY_BUTTON, playButtonX, buttonRowY, buttonSize, buttonSize, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_MUSIC_ENTRY, iFirstEraMusicKey)
				else:
					screen.setImageButton(self.top.getNextWidgetName(), SASVisuals.InterfaceArt.SAS_EMOJI_PLAY_BUTTON, playButtonX, buttonRowY, buttonSize, buttonSize, WidgetTypes.WIDGET_PEDIA_MAIN, SevoScreenEnums.PEDIA_MUSIC, -1)

			era_count = gc.getNumEraInfos()
			for iEra in xrange(era_count):
				if iEra >= value_cols:
					break
				info = gc.getEraInfo(iEra)
				button = info.getButton()
				if button:
					btnX = buttonRowStartX + (iEra * wNum)
					btnY = buttonRowY
					# <!-- custom: each era button redirects to its movie entry if movie exists. (Claude Opus 4.5) -->
					if self.top.pediaMovies.hasMovie(SAS_MAGIC_PEDIA_MOVIE_TYPE_ERA, iEra):
						iPackedEraMovie = self.top.SAS_packMovieKey(SAS_MAGIC_PEDIA_MOVIE_TYPE_ERA, iEra)
						screen.setImageButton(self.top.getNextWidgetName(), button, btnX, btnY, buttonSize, buttonSize, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_MOVIE_ENTRY, iPackedEraMovie)
					else:
						screen.addDDSGFC(self.top.getNextWidgetName(), button, btnX, btnY, buttonSize, buttonSize, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# Table starts below the button row (or at top if buttons disabled)
		tableY = y + self.MARGIN + 4 + buttonRowH
		tableH = h - (self.MARGIN * 2) - buttonRowH - 4

		table = self.top.getNextWidgetName()
		screen.addTableControlGFC(table, nCols, tableX, tableY, tableW, tableH, True, False, self.ROW_H, self.ROW_H, CHART_TABLE_STYLE)
		screen.enableSort(table)

		# Minor Python-level micro-opt: bind methods used in hot loops.
		setHeader = screen.setTableColumnHeader
		appendRow = screen.appendTableRow
		setCell = screen.setTableText

		for iCol in range(nCols):
			if self.IS_SAS_SEVOPEDIA_ERA_CHART_HEADER_ICONS and iCol == 0:
				colW = self.W_ICON
			elif (self.IS_SAS_SEVOPEDIA_ERA_CHART_HEADER_ICONS and iCol == 1) or (not self.IS_SAS_SEVOPEDIA_ERA_CHART_HEADER_ICONS and iCol == 0):
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
				if self.IS_SAS_SEVOPEDIA_ERA_CHART_HEADER_ICONS and iCol == 0:
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
				elif (self.IS_SAS_SEVOPEDIA_ERA_CHART_HEADER_ICONS and iCol == 1) or (not self.IS_SAS_SEVOPEDIA_ERA_CHART_HEADER_ICONS and iCol == 0):
					setCell(table, iCol, iRow, cell, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

				# Value columns (center)
				else:
					setCell(table, iCol, iRow, cell, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def handleInput(self, inputClass):
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED and self.szCsvLogButton and inputClass.getFunctionName() == self.szCsvLogButton:
			chart_dump_table_csv("SAS_SEVOPEDIA_ERA_CHART", self._getTableData())
			return 1
		return 0

	def dumpCsvLog(self):
		chart_dump_table_csv("SAS_SEVOPEDIA_ERA_CHART", self._getTableData())

	def _getTableData(self):
		if self._cachedTable is not None:
			return self._cachedTable

		data = self._buildTableFromGameData()
		self._cachedTable = data
		return data

	def _buildTableFromGameData(self):
		row_specs = (
			# (field_key, display_label_or_None, getter_name_or_None, icon_token, value_type)
			("bNoGoodies",                 "No Goodies",                 "isNoGoodies",                 "btn:skull",        "bool"),
			("bAllGoodyTechs",             "All Goody Techs",             "isAllGoodyTechs",             "glyph:research",   "bool"),
			("bNoAnimals",                 "No Animals",                 "isNoAnimals",                 "btn:lion",         "bool"),
			("bNoBarbUnits",               "No Barb Units",              "isNoBarbUnits",               "btn:skull",        "bool"),
			("bNoBarbCities",              "No Barb Cities",             "isNoBarbCities",              "btn:skull",        "bool"),
			("iAdvancedStartPoints",       "Advanced Start Points",      "getAdvancedStartPoints",      "glyph:defense",    "int"),
			("iStartingUnitMultiplier",    "Starting Unit Multiplier",   "getStartingUnitMultiplier",   "btn:swords",       "int"),
			("iStartingDefenseUnits",      "Starting Defense Units",     "getStartingDefenseUnits",     "btn:swords",       "int"),
			("iStartingWorkerUnits",       "Starting Worker Units",      "getStartingWorkerUnits",      "glyph:citizen",    "int"),
			("iStartingExploreUnits",      "Starting Explore Units",     "getStartingExploreUnits",     "btn:swords",       "int"),
			("iStartingGold",              "Starting Gold",              "getStartingGold",             "glyph:gold",       "int"),
			("iFreePopulation",            "Free Population",            "getFreePopulation",           "glyph:citizen",    "int"),
			("iStartPercent",              "Start Percent",              "getStartPercent",             "glyph:defense",    "int"),
			("iGrowthPercent",             None,                         "getGrowthPercent",            "glyph:food",       "int"),
			("iTrainPercent",              None,                         "getTrainPercent",             "btn:swords",       "int"),
			("iConstructPercent",          None,                         "getConstructPercent",         "glyph:prod",       "int"),
			("iCreatePercent",             None,                         "getCreatePercent",            "glyph:prod",       "int"),
			("iResearchPercent",           None,                         "getResearchPercent",          "glyph:research",   "int"),
			("iTechCostModifier",          "Tech Cost Modifier",         "getTechCostModifier",         "glyph:research",   "int"),
			("iAIMaxGroundbreakingPenalty","AI Max Groundbreaking Pen.", "getAIMaxGroundbreakingPenalty","btn:brain",        "int"),
			("iHumanMaxGroundbreakingPenalty","Human Max Groundbreaking Pen.","getHumanMaxGroundbreakingPenalty","btn:brain",  "int"),
			("iBuildPercent",              None,                         "getBuildPercent",             "glyph:citizen",    "int"),
			("iImprovementPercent",        None,                         "getImprovementPercent",       "glyph:citizen",    "int"),
			("iGreatPeoplePercent",        None,                         "getGreatPeoplePercent",       "glyph:great_people","int"),
			("iCulturePercent",            None,                         "getCulturePercent",           "glyph:culture",    "int"),
			("iAnarchyPercent",            None,                         "getAnarchyPercent",           "btn:fire",         "int"),
			("iEventChancePerTurn",        "Event Chance Per Turn",      "getEventChancePerTurn",       "btn:gear",         "int"),
			("bAIAgeOfPestilence",          "AI Age Of Pestilence",        "isAIAgeOfPestilence",          "btn:brain",        "bool"),
			("bAIAgeOfExploration",         "AI Age Of Exploration",       "isAIAgeOfExploration",         "btn:brain",        "bool"),
			("bAIAgeOfPollution",           "AI Age Of Pollution",         "isAIAgeOfPollution",           "btn:brain",        "bool"),
			("bAIAgeOfFertility",           "AI Age Of Fertility",         "isAIAgeOfFertility",           "btn:brain",        "bool"),
			("bAIAgeOfGuns",                "AI Age Of Guns",              "isAIAgeOfGuns",                "btn:brain",        "bool"),
			("bAIAgeOfProduction",          "AI Age Of Production",        "isAIAgeOfProduction",          "btn:brain",        "bool"),
			("bAIAtomicAge",                "AI Atomic Age",               "isAIAtomicAge",                "btn:brain",        "bool"),
			("Techs",                       "Techs",                       None,                            "glyph:research",   "techs"),
		)

		field_getters_list = []
		for field_name, _display_label, getter_name, _icon_token, _value_type in row_specs:
			if getter_name:
				field_getters_list.append((field_name, getter_name))
		field_getters = tuple(field_getters_list)

		if gc.getNumEraInfos() > 0:
			info = gc.getEraInfo(0)
			missing = []
			for field_name, getter_name in field_getters:
				if not hasattr(info, getter_name):
					missing.append(getter_name)
			if missing:
				raise RuntimeError("[FATAL] Your mod DLL does not expose the required CvEraInfo Python getters: %s. Please expose them in CyInfoInterface2.cpp and rebuild the DLL." % ", ".join(missing))

		game = CyGame()
		btn_by_name = {}

		_btn_defs = (("lion", "SAS_EMOJI_LION_FACE"), ("skull", "SAS_EMOJI_SKULL"), ("swords", "SAS_EMOJI_CROSSED_SWORDS"), ("fire", "SAS_EMOJI_FIRE"), ("gear", "SAS_EMOJI_GEAR"), ("brain", "SAS_EMOJI_BRAIN"),)
		for i, (name, artKey) in enumerate(_btn_defs):
			btn_by_name[name] = (SASVisuals.InterfaceArt.getPath(artKey), (i + 1) * 10)

		glyph_by_name = {}
		_glyph_defs = (
			# (name, glyph_char)
			("food",         (u"%s" % (SASVisuals.Yield.FOOD_CHAR))),
			("prod",         (u"%s" % (SASVisuals.Yield.PRODUCTION_CHAR))),
			("gold",         (u"%s" % (SASVisuals.Commerce.GOLD_CHAR))),
			("research",     (u"%s" % (SASVisuals.Commerce.RESEARCH_CHAR))),
			("culture",      (u"%s" % (SASVisuals.Commerce.CULTURE_CHAR))),
			("citizen",      (u"%s" % (SASVisuals.Font.CITIZEN_CHAR))),
			("defense",      (u"%s" % (SASVisuals.Font.DEFENSE_CHAR))),
			("great_people", (u"%s" % (SASVisuals.Font.GREAT_PEOPLE_CHAR))),
		)
		for name, glyph in _glyph_defs:
			glyph_by_name[name] = glyph

		icon_token_by_key = {}
		display_label_by_key = {}
		for field_name, display_label, _getter_name, icon_token, _value_type in row_specs:
			if icon_token:
				icon_token_by_key[field_name] = icon_token
			if display_label:
				display_label_by_key[field_name] = display_label

		def icon_cell_for_key(icon_key, iRowIndex):
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

		none_text = localText.getText("TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_NONE", ())
		yes_text = localText.getText("TXT_KEY_YES", ())
		no_text = localText.getText("TXT_KEY_NO", ())

		era_types = []
		era_labels = []
		for iEra in xrange(gc.getNumEraInfos()):
			info = gc.getEraInfo(iEra)
			era_types.append(info.getType())
			era_labels.append(chart_get_info_display_name(info))

		parsed_data = {}
		for iEra in xrange(gc.getNumEraInfos()):
			info = gc.getEraInfo(iEra)
			era_type = info.getType()
			era_dict = {}

			for field_name, getter_name in field_getters:
				getter = getattr(info, getter_name)
				era_dict[field_name] = str(getter())

			parsed_data[era_type] = era_dict

		techs_by_era = {}
		for era_type in era_types:
			techs_by_era[era_type] = []

		for iTech in xrange(gc.getNumTechInfos()):
			tech_info = gc.getTechInfo(iTech)
			iEra = tech_info.getEra()
			if iEra < 0 or iEra >= len(era_types):
				continue
			tech_name = chart_get_info_display_name(tech_info)
			techs_by_era[era_types[iEra]].append(tech_name)

		for era_type in era_types:
			tech_list = techs_by_era.get(era_type, [])
			if tech_list:
				parsed_data[era_type]["Techs"] = ", ".join(tech_list)
			else:
				parsed_data[era_type]["Techs"] = ""

		header = []
		if self.IS_SAS_SEVOPEDIA_ERA_CHART_HEADER_ICONS:
			header.append(u"")
			header.append(chart_font2("Field"))
		else:
			header.append(chart_font2("Field"))
		for label in era_labels:
			header.append(chart_font2(label))

		table = [header]

		row_index = 0
		techs_per_cell = 1
		for field_name, display_label, _getter_name, _icon_token, value_type in row_specs:
			if field_name == "Techs":
				tech_rows = self._expandTechRows(field_name, era_types, parsed_data, techs_per_cell, none_text)
				for tech_row in tech_rows:
					display_text = tech_row["Field"]
					if self.IS_SAS_SEVOPEDIA_ERA_CHART_HEADER_ICONS:
						icon_key = tech_row.get("IconKey", "")
						row = [icon_cell_for_key(icon_key, row_index), chart_font2(display_text)]
					else:
						row = [chart_font2(display_text)]
					for era_type in era_types:
						row.append(chart_font2(tech_row.get(era_type, "")))
					table.append(row)
					row_index += 1
				continue

			szFieldName = display_label_by_key.get(field_name, chart_beautify_field_name(field_name))

			if self.IS_SAS_SEVOPEDIA_ERA_CHART_HEADER_ICONS:
				row = [icon_cell_for_key(field_name, row_index), chart_font2(szFieldName)]
			else:
				row = [chart_font2(szFieldName)]

			for era_type in era_types:
				val = parsed_data.get(era_type, {}).get(field_name, "")
				if value_type == "bool":
					val = self._format_bool(val, yes_text, no_text)
				row.append(chart_font2(val))

			table.append(row)
			row_index += 1

		return table

	def _format_bool(self, value, yes_text, no_text):
		# <!-- custom: normalize bool strings from Cy* getters so chart uses + / empty instead of True/False. (GPT-5.2-Codex) -->
		if value in (True, "1", 1):
			return "+"
		if value in (False, "0", 0):
			return ""
		if isinstance(value, basestring):
			if value.lower() == "true":
				return "+"
			if value.lower() == "false":
				return ""
		return value

	def _expandTechRows(self, field_name, era_types, parsed_data, techs_per_cell, none_text):
		per_era_chunks = {}
		max_chunks = 1
		for era_type in era_types:
			raw = parsed_data.get(era_type, {}).get(field_name, "")
			items = chart_format_tech_list(raw, True, none_text)
			chunks = chart_chunk_list(items, techs_per_cell)
			if not chunks:
				chunks = [[none_text]]
			per_era_chunks[era_type] = chunks
			if len(chunks) > max_chunks:
				max_chunks = len(chunks)

		rows = []
		# <!-- custom: use numbered field labels so sorting by the Field column keeps Tech rows together; blank labels can jump on sort. (GPT-5.2-Codex) -->
		for i in xrange(max_chunks):
			row = {}
			row["Field"] = "Tech %02d" % (i + 1)
			row["IconKey"] = field_name
			for era_type in era_types:
				chunks = per_era_chunks[era_type]
				if i < len(chunks):
					row[era_type] = ", ".join(chunks[i])
				else:
					row[era_type] = ""
			rows.append(row)
		return rows
