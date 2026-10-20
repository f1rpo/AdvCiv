# Handicap chart page for Sevopedia (AdvCiv-SAS)
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Based on the Middle-earth mod's PlatyPedia handicap approach: pull values from
# CvHandicapInfo getters (rather than XML parsing) for Python 2.4 compatibility.
#
# <!-- custom: note: pending issue of img being left-aligned but couldn't fix it for now, i believe the advantage of being able to sort by emoji i.e. theme outweighs this downside so left as such for now. -->

from CvPythonExtensions import *
# <!-- custom: Consolidate this dependency at the shared SASVisualUtils/SASTextScale boundary instead of retaining the retired overlapping visual-resource helper imports. (ChatGPT-5.6-Sol) -->
import SASVisualUtils as SASVisuals
import CvUtil
import re
from _sevopedia_helpers import *

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class SevoPediaHandicapChart:
	def __init__(self, main):
		self.top = main
		self._cachedTable = None
		self.szCsvLogButton = ""

		self.MARGIN = CHART_TABLE_MARGIN
		self.ROW_H = CHART_TABLE_ROW_H
		self.W_ICON = CHART_TABLE_W_ICON
		self.W_FIELD = 380
		self.IS_SAS_SEVOPEDIA_HANDICAP_CHART_HEADER_ICONS = (gc.getDefineINT("SAS_SEVOPEDIA_HANDICAP_CHART_HEADER_ICONS") > 0)
		self.TABLE_FILL_PERCENT = gc.getDefineINT("SAS_SEVOPEDIA_HANDICAP_CHART_TABLE_FILL_PERCENT")
		if self.TABLE_FILL_PERCENT <= 0:
			raise ValueError("[FATAL] SAS_SEVOPEDIA_HANDICAP_CHART_TABLE_FILL_PERCENT must be >= 1.")

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

		table = self.top.getNextWidgetName()
		tableX = x + self.MARGIN
		tableY = y + self.MARGIN + 4
		tableW = w - (self.MARGIN * 2)
		tableH = h - (self.MARGIN * 2) - 4

		data = self._getTableData()
		if not data:
			raise RuntimeError("[FATAL] SevoPediaHandicapChart received no table data; this should never happen.")
		header = data[0]
		rows = data[1:]
		nCols = len(header)
		if nCols < 2:
			return

		totalW = (tableW * self.TABLE_FILL_PERCENT) / 100
		if self.IS_SAS_SEVOPEDIA_HANDICAP_CHART_HEADER_ICONS:
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
			if self.IS_SAS_SEVOPEDIA_HANDICAP_CHART_HEADER_ICONS and iCol == 0:
				colW = self.W_ICON
			elif (self.IS_SAS_SEVOPEDIA_HANDICAP_CHART_HEADER_ICONS and iCol == 1) or (not self.IS_SAS_SEVOPEDIA_HANDICAP_CHART_HEADER_ICONS and iCol == 0):
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
				if self.IS_SAS_SEVOPEDIA_HANDICAP_CHART_HEADER_ICONS and iCol == 0:
					if isinstance(cell, tuple):
						text = cell[0]
						icon_button = cell[1]
					else:
						text = cell
						icon_button = ""
					setCell(table, iCol, iRow, text, icon_button, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

				# Field column (left)
				elif (self.IS_SAS_SEVOPEDIA_HANDICAP_CHART_HEADER_ICONS and iCol == 1) or (not self.IS_SAS_SEVOPEDIA_HANDICAP_CHART_HEADER_ICONS and iCol == 0):
					setCell(table, iCol, iRow, cell, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

				# Value columns (center)
				else:
					setCell(table, iCol, iRow, cell, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def handleInput(self, inputClass):
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED and self.szCsvLogButton and inputClass.getFunctionName() == self.szCsvLogButton:
			chart_dump_table_csv("SAS_SEVOPEDIA_HANDICAP_CHART", self._getTableData())
			return 1
		return 0

	def dumpCsvLog(self):
		chart_dump_table_csv("SAS_SEVOPEDIA_HANDICAP_CHART", self._getTableData())

	def _getTableData(self):
		if self._cachedTable is not None:
			return self._cachedTable

		data = self._buildTableFromGameData()
		self._cachedTable = data
		return data

	def _buildTableFromGameData(self):
		# Centralized chart spec
		# One place to maintain BOTH:
		#   - which CvHandicapInfo Python getters we read
		#   - which icon token (btn:* / glyph:*) we show for that row
		#
		# This avoids the old duplication where:
		#   field_getters = (...)   and   icon_token_by_key = {...}
		# had to be kept in sync manually.
		field_specs = (
			# <!-- custom: aligned with the help of ChatGPT-5.2 Thinking thanks a lot! -->
			# (field_name, getter_name, icon_token)
			("iAIAttitudeChangePercent",           "getAIAttitudeChangePercent",           "btn:dove"),
			("iAIAdvancedStartPercent",            "getAIAdvancedStartPercent",            "glyph:defense"),
			("iAIAnimalBonus",                     "getAIAnimalCombatModifier",            "btn:lion"),
			("iAIBarbarianBonus",                  "getAIBarbarianCombatModifier",         "btn:skull"),
			("iAICivicUpkeepPercent",              "getAICivicUpkeepPercent",              "glyph:gold"),
			("iAIConstructPercent",                "getAIConstructPercent",                "glyph:prod"),
			("iAICreatePercent",                   "getAICreatePercent",                   "glyph:prod"),
			("iAIDeclareWarProb",                  "getAIDeclareWarProb",                  "btn:swords"),
			("iAIGPThresholdPercent",              "getAIGPThresholdPercent",              "glyph:great_people"),
			("iAIGrowthPercent",                   "getAIGrowthPercent",                   "glyph:food"),
			("iAIHandicapIncrementTurns",          "getAIHandicapIncrementTurns",          "glyph:defense"),
			("iAIInflationPercent",                "getAIInflationPercent",                "glyph:gold"),
			("iAIResearchPercent",                 "getAIResearchPercent",                 "glyph:research"),
			("iAIStartingDefenseUnits",            "getAIStartingDefenseUnits",            "btn:swords"),
			("iAIStartingExploreUnits",            "getAIStartingExploreUnits",            "btn:swords"),
			("iAIStartingUnitMultiplier",          "getAIStartingUnitMultiplier",          "btn:swords"),
			("iAIStartingWorkerUnits",             "getAIStartingWorkerUnits",             "glyph:citizen"),
			("iAITrainPercent",                    "getAITrainPercent",                    "btn:swords"),
			("iAIUnitCostPercent",                 "getAIUnitCostPercent",                 "btn:swords"),
			("iAIUnitSupplyPercent",               "getAIUnitSupplyPercent",               "btn:swords"),
			("iAIUnitUpgradePercent",              "getAIUnitUpgradePercent",              "btn:swords"),
			("iAIWarWearinessPercent",             "getAIWarWearinessPercent",             "btn:swords"),
			("iAIWorkRateModifier",                "getAIWorkRateModifier",                "glyph:citizen"),
			("iAIWorldConstructPercent",           "getAIWorldConstructPercent",           "glyph:prod"),
			("iAIWorldCreatePercent",              "getAIWorldCreatePercent",              "glyph:prod"),
			("iAIWorldTrainPercent",               "getAIWorldTrainPercent",               "btn:swords"),
			("iAdvancedStartPointsMod",            "getAdvancedStartPointsMod",            "glyph:defense"),
			("iAnimalAttackProb",                  "getAnimalAttackProb",                  "btn:lion"),
			("iAnimalBonus",                       "getAnimalCombatModifier",              "btn:lion"),
			("iAttitudeChange",                    "getAttitudeChange",                    "btn:dove"),
			("iBarbarianBonus",                    "getBarbarianCombatModifier",           "btn:skull"),
			("iBarbarianCityAttackBonus",          "getBarbarianCityAttackBonus",          "btn:skull"),
			("iBarbarianCityCreationProb",         "getBarbarianCityCreationProb",         "btn:skull"),
			("iBarbarianCityCreationTurnsElapsed", "getBarbarianCityCreationTurnsElapsed", "btn:skull"),
			("iBarbarianCreationTurnsElapsed",     "getBarbarianCreationTurnsElapsed",     "btn:skull"),
			("iBarbarianDefenders",                "getBarbarianInitialDefenders",         "btn:skull"),
			("iBaseGrowthThresholdPercent",        "getBaseGrowthThresholdPercent",        "glyph:food"),
			("iBuildTimePercent",                  "getBuildTimePercent",                  "glyph:citizen"),
			("iCivicUpkeepPercent",                "getCivicUpkeepPercent",                "glyph:gold"),
			("iColonyMaintenancePercent",          "getColonyMaintenancePercent",          "glyph:gold"),
			("iConstructPercent",                  "getConstructPercent",                  "glyph:prod"),
			("iCorporationMaintenancePercent",     "getCorporationMaintenancePercent",     "glyph:gold"),
			("iCreatePercent",                     "getCreatePercent",                     "glyph:prod"),
			("iCultureLevelPercent",               "getCultureLevelPercent",               "glyph:culture"),
			("iDifficulty",                        "getDifficulty",                        "glyph:defense"),
			("iDistanceMaintenancePercent",        "getDistanceMaintenancePercent",        "glyph:gold"),
			("iForeignCultureStrength",            "getForeignCultureStrength",            "glyph:culture"),
			("iFreeUnits",                         "getFreeUnits",                         "btn:swords"),
			("iFreeWinsVsBarbs",                   "getFreeWinsVsBarbs",                   "btn:skull"),
			("iGold",                              "getStartingGold",                      "glyph:gold"),
			("iGPThresholdPercent",                "getGPThresholdPercent",                "glyph:great_people"),
			("iHappyBonus",                        "getHappyBonus",                        "glyph:happy"),
			("iHealthBonus",                       "getHealthBonus",                       "glyph:health"),
			("iInflationPercent",                  "getInflationPercent",                  "glyph:gold"),
			("iMaxColonyMaintenance",              "getMaxColonyMaintenance",              "glyph:gold"),
			("iMaxNumCitiesMaintenance",           "getMaxNumCitiesMaintenance",           "glyph:gold"),
			("iNoTechTradeModifier",               "getNoTechTradeModifier",               "glyph:research"),
			("iNumCitiesMaintenancePercent",       "getNumCitiesMaintenancePercent",       "glyph:gold"),
			("iResearchPercent",                   "getResearchPercent",                   "glyph:research"),
			("iSeaBarbarianBonus",                 "getSeaBarbarianBonus",                 "btn:skull"),
			("iSeaBarbarianExtraMoves",            "getSeaBarbarianExtraMoves",            "btn:skull"),
			("iStartingDefenseUnits",              "getStartingDefenseUnits",              "btn:swords"),
			("iStartingExploreUnits",              "getStartingExploreUnits",              "btn:swords"),
			("iStartingLocPercent",                "getStartingLocationPercent",           "glyph:defense"),
			("iStartingWorkerUnits",               "getStartingWorkerUnits",               "glyph:citizen"),
			("iTechTradeKnownModifier",            "getTechTradeKnownModifier",            "glyph:research"),
			("iTrainPercent",                      "getTrainPercent",                      "btn:swords"),
			("iUnitCostPercent",                   "getUnitCostPercent",                   "btn:swords"),
			("iUnownedTilesPerBarbarianCity",      "getUnownedTilesPerBarbarianCity",      "btn:skull"),
			("iUnownedTilesPerBarbarianUnit",      "getUnownedTilesPerBarbarianUnit",      "btn:skull"),
			("iUnownedTilesPerGameAnimal",         "getUnownedTilesPerGameAnimal",         "btn:lion"),
			("iUnownedWaterTilesPerBarbarianUnit", "getUnownedWaterTilesPerBarbarianUnit", "btn:skull"),
		)

		# Goody huts: define the DISPLAYED grouped rows ONCE, then derive:
		#   - which GOODY_* types we need to count
		#   - which grouped rows we show in the chart
		# Each group can contain any number of goody types (1, 2, 3+). Today we mostly use pairs, but this structure keeps it flexible for future tweaks.
		goody_group_specs = (
			# (label, goody_types_tuple, icon_token)
			("Goody Gold (Low / High)", ("GOODY_LOW_GOLD", "GOODY_HIGH_GOLD"), "glyph:gold"),
			("Goody (Experience / Healing)", ("GOODY_EXPERIENCE", "GOODY_HEALING"), "btn:swords"),
			("Goody (Map / Tech)", ("GOODY_MAP", "GOODY_TECH"), "glyph:research"),
			("Goody (Scout / Warrior)", ("GOODY_SCOUT", "GOODY_WARRIOR"), "btn:swords"),
			("Goody (Worker / Settler)", ("GOODY_WORKER", "GOODY_SETTLER"), "glyph:citizen"),
			("Goody Barbarians (Weak / Strong)", ("GOODY_BARBARIANS_WEAK", "GOODY_BARBARIANS_STRONG"), "btn:skull"),
		)

		# Build (field, getter) pairs from the centralized spec.
		field_getters_list = []
		for field_name, getter_name, _icon_token in field_specs:
			field_getters_list.append((field_name, getter_name))
		field_getters = tuple(field_getters_list)

		# <!-- custom: fail fast if the DLL doesn't expose required CvHandicapInfo getters for the handicap chart. (GPT-5.2-Codex) -->
		if gc.getNumHandicapInfos() > 0:
			info = gc.getHandicapInfo(0)
			missing = []
			for field_name, getter_name in field_getters:
				if not hasattr(info, getter_name):
					missing.append(getter_name)
			if missing:
				raise RuntimeError("[FATAL] Your mod DLL does not expose the required CvHandicapInfo Python getters: %s. Please expose them in CyInfoInterface2.cpp and rebuild the DLL." % ", ".join(missing))

		# Derive goody_types + goody_groups from goody_group_specs, so there is no
		# separate "types" list to maintain.
		goody_types = []
		_seen = {}
		goody_groups_list = []
		for label, group_types, _icon_token in goody_group_specs:
			# Store the group as-is (tuple of 1, 2, 3+ GOODY_* types).
			goody_groups_list.append((label, tuple(group_types)))
			# Flatten all grouped types into a unique ordered list for counting.
			for gt in group_types:
				if gt not in _seen:
					_seen[gt] = True
					goody_types.append(gt)
		goody_groups = tuple(goody_groups_list)

		anchor_field = gc.getDefineSTRING("SAS_SEVOPEDIA_HANDICAP_CHART_ANCHOR_FIELD")  # not cached: read once only (table is cached via _cachedTable)
		if not anchor_field:
			raise ValueError("[FATAL] Missing SAS_SEVOPEDIA_HANDICAP_CHART_ANCHOR_FIELD define.")

		abbrev_tech_names = {
			"Animal Husbandry": "Animal H.",
			"Bronze Working": "Bronze W.",
			"Iron Working": "Iron W.",
			"Industrialism": "Industr.",
		}
		techs_per_cell = 1
		none_text = localText.getText("TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_NONE", ())

		# Collect tech types once.
		tech_types = []
		for iTech in xrange(gc.getNumTechInfos()):
			tech_types.append(gc.getTechInfo(iTech).getType())

		all_fields = {}
		parsed_data = {}
		difficulty_types = []
		difficulty_labels = {}

		for iHandicap in xrange(gc.getNumHandicapInfos()):
			info = gc.getHandicapInfo(iHandicap)
			handicap_type = info.getType()
			difficulty_types.append(handicap_type)
			difficulty_labels[handicap_type] = chart_get_info_display_name(info)
			handicap_dict = {}

			for field_name, getter_name in field_getters:
				getter = getattr(info, getter_name)
				value = getter()
				handicap_dict[field_name] = str(value)
				all_fields[field_name] = 1

			self._appendGoodyFields(handicap_dict, info, all_fields, goody_types)
			self._appendFreeTechFields(handicap_dict, info, all_fields, tech_types)

			parsed_data[handicap_type] = handicap_dict

		fields = all_fields.keys()
		fields.sort()

		goody_fields = []
		for gt in goody_types:
			if gt in all_fields:
				goody_fields.append(gt)

		rows = []
		for field in fields:
			row = {"Field": field, "IconKey": field}
			for difficulty in difficulty_types:
				value = ""
				if difficulty in parsed_data:
					value = parsed_data[difficulty].get(field, "")
				row[difficulty] = value
			rows.append(row)

		rows = self._mergeHumanAiRows(rows, difficulty_types, none_text)

		before = []
		goody_rows = []
		nested = []
		for row in rows:
			if row["Field"] in goody_fields:
				goody_rows.append(row)
			elif row["Field"] in ("AIFreeTechs", "FreeTechs"):
				nested.append(row)
			else:
				before.append(row)

		if goody_rows:
			goody_rows_by_field = {}
			for grow in goody_rows:
				goody_rows_by_field[grow["Field"]] = grow

			grouped_goody_rows = []
			skipped_goody_fields = {}
			for label, group_types in goody_groups:
				# group_types can have 1, 2, 3+ GOODY_* entries.
				rows_for_group = []
				ok = True
				for gt in group_types:
					r = goody_rows_by_field.get(gt)
					if r is None:
						ok = False
						break
					rows_for_group.append(r)
				if not ok:
					continue
				new_row = {"Field": label, "IconKey": label}
				for difficulty in difficulty_types:
					vals = []
					for r in rows_for_group:
						vals.append(r.get(difficulty, "0"))
					new_row[difficulty] = " / ".join(vals)
				grouped_goody_rows.append(new_row)
				for gt in group_types:
					skipped_goody_fields[gt] = True

			for grow in goody_rows:
				if grow["Field"] not in skipped_goody_fields:
					grouped_goody_rows.append(grow)

			goody_rows = grouped_goody_rows

		# <!-- custom: keep FreeTechs/AIFreeTechs at the end to avoid long multi-row blocks splitting the chart; goody rows keep the anchor placement. (GPT-5.2-Codex) -->
		new_rows = []
		inserted_goody = False
		for row in before:
			new_rows.append(row)
			if row["Field"] == anchor_field:
				for grow in goody_rows:
					new_rows.append(grow)
				inserted_goody = True
		if not inserted_goody:
			for grow in goody_rows:
				new_rows.append(grow)
		for nrow in nested:
			new_rows.append(nrow)

		rows = self._expandTechRows(new_rows, difficulty_types, techs_per_cell, none_text, abbrev_tech_names)

		# One-time render prep (kept local; cache stores only final table cells)
		icon_cell_for_key = None
		if self.IS_SAS_SEVOPEDIA_HANDICAP_CHART_HEADER_ICONS:
			game = CyGame()
			btn_by_name = {}

			_btn_defs = (("dove", "SAS_EMOJI_DOVE"), ("lion", "SAS_EMOJI_LION_FACE"), ("skull", "SAS_EMOJI_SKULL"), ("swords", "SAS_EMOJI_CROSSED_SWORDS"),)
			for i, (name, artKey) in enumerate(_btn_defs):
				# Group id only affects sorting by the icon column.
				# Reorder _btn_defs to change the icon-theme sort order.
				btn_by_name[name] = (SASVisuals.InterfaceArt.getPath(artKey), (i + 1) * 10)

			glyph_by_name = {}
			# GameFont glyph icons (yields/commerce/symbols).
			_glyph_defs = (
				# (name, glyph_char)
				("great_people", (u"%s" % (SASVisuals.Font.GREAT_PEOPLE_CHAR))),

				("research",     (u"%s" % (SASVisuals.Commerce.RESEARCH_CHAR))),
				("gold",         (u"%s" % (SASVisuals.Commerce.GOLD_CHAR))),
				("culture",      (u"%s" % (SASVisuals.Commerce.CULTURE_CHAR))),

				("happy",        (u"%s" % (SASVisuals.Font.HAPPY_CHAR))),
				("health",       (u"%s" % (SASVisuals.Font.HEALTHY_CHAR))),
				("food",         (u"%s" % (SASVisuals.Yield.FOOD_CHAR))),
				("prod",         (u"%s" % (SASVisuals.Yield.PRODUCTION_CHAR))),
				("defense",      (u"%s" % (SASVisuals.Font.DEFENSE_CHAR))),
				("citizen",      (u"%s" % (SASVisuals.Font.CITIZEN_CHAR))),
			)
			for _name, _glyph in _glyph_defs:
				glyph_by_name[_name] = _glyph

				# Token map: key -> "btn:name" or "glyph:name"
			# Built from the centralized specs above, so there is only ONE place to edit.
			icon_token_by_key = {}
			for field_name, _getter_name, icon_token in field_specs:
				if icon_token:
					icon_token_by_key[field_name] = icon_token
			for label, _group_types, icon_token in goody_group_specs:
				if icon_token:
					icon_token_by_key[label] = icon_token
			# Synthetic rows.
			icon_token_by_key["FreeTechs"] = "glyph:research"
			icon_token_by_key["AIFreeTechs"] = "glyph:research"

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
					# Preserve the visible glyph, but add an invisible tie-breaker.
					return (chart_font2(glyph) + chart_sort_key(0, iRowIndex), "")
				return (chart_sort_key(0, iRowIndex), "")

		# Build header (pre-fonted)
		if self.IS_SAS_SEVOPEDIA_HANDICAP_CHART_HEADER_ICONS:
			header = [u"", chart_font2("Field")]
		else:
			header = [chart_font2("Field")]
		for difficulty in difficulty_types:
			header.append(chart_font2(difficulty_labels[difficulty]))

		data = [header]

		# Build rows (pre-fonted)
		row_index = 0
		for row in rows:
			field_name = row["Field"]
			display_field = field_name

			if self.IS_SAS_SEVOPEDIA_HANDICAP_CHART_HEADER_ICONS:
				icon_key = row.get("IconKey", "")
				icon_cell = (u"", "")
				if icon_cell_for_key is not None:
					icon_cell = icon_cell_for_key(icon_key, row_index)
				out_row = [icon_cell, chart_font2(display_field)]
			else:
				out_row = [chart_font2(display_field)]

			for difficulty in difficulty_types:
				value = row.get(difficulty, "")
				if row["Field"] in ("FreeTechs", "AIFreeTechs"):
					value = chart_format_tech_list(value, False, none_text, abbrev_tech_names)
				out_row.append(chart_font2(value))

			data.append(out_row)
			row_index += 1

		return data

	def _appendGoodyFields(self, handicap_dict, info, all_fields, goody_types):
		goody_counts = {}
		for gt in goody_types:
			goody_counts[gt] = 0

		try:
			num_goodies = info.getNumGoodies()
		except:
			num_goodies = 0

		for i in xrange(num_goodies):
			try:
				i_goody = info.getGoodies(i)
			except:
				i_goody = -1
			if i_goody >= 0:
				goody_info = gc.getGoodyInfo(i_goody)
				goody_type = goody_info.getType()
				if goody_type in goody_counts:
					goody_counts[goody_type] = goody_counts[goody_type] + 1

		for gt in goody_types:
			handicap_dict[gt] = str(goody_counts.get(gt, 0))
			all_fields[gt] = 1

	def _appendFreeTechFields(self, handicap_dict, info, all_fields, tech_types):
		free_list = []
		ai_free_list = []
		for iTech in xrange(len(tech_types)):
			tech_type = tech_types[iTech]
			if info.isFreeTechs(iTech):
				free_list.append(tech_type)
			if info.isAIFreeTechs(iTech):
				ai_free_list.append(tech_type)

		handicap_dict["FreeTechs"] = ", ".join(free_list)
		handicap_dict["AIFreeTechs"] = ", ".join(ai_free_list)
		all_fields["FreeTechs"] = 1
		all_fields["AIFreeTechs"] = 1

	def _mergeHumanAiRows(self, rows, difficulty_types, none_text):
		row_by_field = {}
		for row in rows:
			row_by_field[row["Field"]] = row

		merged = []
		skip_fields = {}
		for row in rows:
			field = row["Field"]
			if field in skip_fields:
				continue
			if field.startswith("iAI"):
				human_field = "i" + field[3:]
				if human_field in row_by_field:
					continue
				merged.append(row)
				continue
			if not field.startswith("i"):
				merged.append(row)
				continue

			ai_field = "iAI" + field[1:]
			ai_row = row_by_field.get(ai_field)
			if ai_row is None:
				merged.append(row)
				continue

			label = self._humanAiLabel(field)
			new_row = {"Field": label, "IconKey": row.get("IconKey", "")}
			for difficulty in difficulty_types:
				human_val = row.get(difficulty, "")
				ai_val = ai_row.get(difficulty, "")
				if not human_val:
					human_val = none_text
				if not ai_val:
					ai_val = none_text
				new_row[difficulty] = "%s / %s" % (human_val, ai_val)
			merged.append(new_row)
			skip_fields[ai_field] = True
		return merged

	def _humanAiLabel(self, field):
		base = field
		if base.startswith("i"):
			base = base[1:]
		base = re.sub(r"([a-z])([A-Z])", r"\1 \2", base)
		return base + " (Human / AI)"

	def _expandTechRows(self, rows, difficulty_types, techs_per_cell, none_text, abbrev_tech_names):
		new_rows = []
		for row in rows:
			field = row["Field"]
			if field not in ("FreeTechs", "AIFreeTechs"):
				new_rows.append(row)
				continue

			per_diff_chunks = {}
			max_chunks = 1
			for difficulty in difficulty_types:
				raw = row.get(difficulty, "")
				items = chart_format_tech_list(raw, True, none_text, abbrev_tech_names)
				chunks = chart_chunk_list(items, techs_per_cell)
				if not chunks:
					chunks = [[none_text]]
				per_diff_chunks[difficulty] = chunks
				if len(chunks) > max_chunks:
					max_chunks = len(chunks)

			# <!-- custom: use numbered field labels so sorting by the Field column keeps tech rows together; blank labels can jump on sort. (GPT-5.2-Codex) -->
			for i in xrange(max_chunks):
				new_row = {}
				if field == "AIFreeTechs":
					label_base = "AI Free Tech"
				else:
					label_base = "Free Tech"
				new_row["Field"] = "%s %02d" % (label_base, (i + 1))
				new_row["IconKey"] = field
				for difficulty in difficulty_types:
					chunks = per_diff_chunks[difficulty]
					if i < len(chunks):
						new_row[difficulty] = ", ".join(chunks[i])
					else:
						new_row[difficulty] = ""
				new_rows.append(new_row)
		return new_rows
