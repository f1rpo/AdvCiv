# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: The page treats the trigger as the dominant entity (matches player's mental model: the trigger is "what happens", events are "the choices when it fires"). (Claude code Opus 4.7) -->

from CvPythonExtensions import *
from SASMagicNumbers import *
import CvUtil
import SevoScreenEnums
import SASTextScale

from _sevopedia_helpers import *

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class SevoPediaEventTrigger:

	def __init__(self, main):
		self.iTrigger = -1
		self.top = main
		# <!-- custom: CvEventTriggerInfo.getPlotsType uses the DLL PlotTypes enum order.
		# Cache translated labels once per page object so rendering requirements does not repeat translator lookups, while keeping the rare enum-order audit point obvious. (GPT-5.5) -->
		self.PLOT_TYPE_LABELS = (localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_PLOT_PEAK", ()), localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_PLOT_HILLS", ()), localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_PLOT_LAND", ()), localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_PLOT_OCEAN", ()))
		# <!-- custom: bHistoryExpanded name matches the generic expand/collapse dispatch in SevoPediaMain (SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND → setHistoryExpanded).
		# On this page the expandable panel is actually Texts, not a Civilopedia/History panel — but reusing the shared mechanism avoids adding a new python widget id just for one page. (Claude code Opus 4.7) -->
		self.bHistoryExpanded = False

		self.X_NAME = self.top.X_PEDIA_PAGE
		self.Y_NAME = self.top.Y_PEDIA_PAGE
		self.W_PAGE = self.top.R_PEDIA_PAGE - self.X_NAME
		self.H_NAME = NON_MULTILIST_PANEL_STANDARD_HEIGHT

		iColGap = MEDIUM_MARGIN
		# <!-- custom: Obsolete-with shares the top row with Name and stays sized for 8 buttons because War Chariots is the longest current event trigger obsolete list, with enough room for "or" connector numTxt.
		# Requires Buttons gets the full row below because it usually has more elements and is more likely to scroll. (GPT-5.5) -->
		iObsoleteButtonW = get_multilist_panel_width_for_buttons(8, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_MULTI_LIST_LEFT_EDGE_PADDING, HYPOTHESIZED_MULTI_LIST_RIGHT_EDGE_PADDING, HYPOTHESIZED_MULTI_LIST_INTER_BUTTON_SPACING)
		self.W_NAME = self.W_PAGE - iColGap - iObsoleteButtonW

		self.X_OBSOLETE_BUTTONS = self.X_NAME + self.W_NAME + iColGap
		self.Y_OBSOLETE_BUTTONS = self.Y_NAME
		self.W_OBSOLETE_BUTTONS = iObsoleteButtonW
		self.H_OBSOLETE_BUTTONS = self.H_NAME

		# Trigger info row: Requires (Text) | Summary.
		iRowTop = self.Y_NAME + self.H_NAME + SMALL_MARGIN
		iRowH = 213
		iColW = (self.W_PAGE - iColGap) / 2

		self.X_REQUIRES_TEXT = self.X_NAME
		self.Y_REQUIRES_TEXT = iRowTop
		self.W_REQUIRES_TEXT = iColW
		self.H_REQUIRES_TEXT = iRowH

		self.X_SUMMARY = self.X_REQUIRES_TEXT + self.W_REQUIRES_TEXT + iColGap
		self.Y_SUMMARY = iRowTop
		self.W_SUMMARY = self.top.R_PEDIA_PAGE - self.X_SUMMARY
		self.H_SUMMARY = iRowH

		iBtnRowTop = iRowTop + iRowH + SMALL_MARGIN
		iBtnRowH = NON_MULTILIST_PANEL_STANDARD_HEIGHT
		self.X_REQUIRES_BUTTONS = self.X_NAME
		self.Y_REQUIRES_BUTTONS = iBtnRowTop
		self.W_REQUIRES_BUTTONS = self.W_PAGE
		self.H_REQUIRES_BUTTONS = iBtnRowH

		self.X_TEXTS = self.X_NAME
		self.Y_TEXTS = iBtnRowTop + iBtnRowH + SMALL_MARGIN
		self.W_TEXTS = self.W_PAGE
		self.H_TEXTS = 130

		# <!-- custom: no Civilopedia/History panel here — event triggers rarely set <Civilopedia> in XML, so the bottom panel would nearly always show "None" and just steals space from the Texts variants.
		# The freed space is given to Texts instead, and Events fills the remainder down to the page bottom. (Claude code Opus 4.7) -->
		self.X_EVENTS = self.X_NAME
		self.Y_EVENTS = self.Y_TEXTS + self.H_TEXTS + SMALL_MARGIN
		self.W_EVENTS = self.W_PAGE
		self.H_EVENTS = self.top.B_PEDIA_PAGE - self.Y_EVENTS

		self.EVENT_CARD_PANEL_HEADER_H = 28

		self.BULLET_PREFIX = u"%c " % CyGame().getSymbolID(FontSymbols.BULLET_CHAR)

		self.COMMERCE_CHARS = []
		for iCommerce in range(CommerceTypes.NUM_COMMERCE_TYPES):
			self.COMMERCE_CHARS.append(gc.getCommerceInfo(iCommerce).getChar())
		self.YIELD_CHARS = []
		for iYield in range(YieldTypes.NUM_YIELD_TYPES):
			self.YIELD_CHARS.append(gc.getYieldInfo(iYield).getChar())
		self.HAPPY_CHAR = u"%c" % CyGame().getSymbolID(FontSymbols.HAPPY_CHAR)
		self.UNHAPPY_CHAR = u"%c" % CyGame().getSymbolID(FontSymbols.UNHAPPY_CHAR)
		self.HEALTHY_CHAR = u"%c" % CyGame().getSymbolID(FontSymbols.HEALTHY_CHAR)
		self.UNHEALTHY_CHAR = u"%c" % CyGame().getSymbolID(FontSymbols.UNHEALTHY_CHAR)
		self.GOLDEN_AGE_CHAR = u"%c" % CyGame().getSymbolID(FontSymbols.GOLDEN_AGE_CHAR)
		self.STAR_CHAR = u"%c" % CyGame().getSymbolID(FontSymbols.STAR_CHAR)

	def interfaceScreen(self, iTrigger):
		bTriggerChanged = (self.iTrigger != iTrigger)
		if bTriggerChanged:
			self.bHistoryExpanded = False
		self.iTrigger = iTrigger

		self.placeName()
		self.placeRequiresText()
		self.placeSummary()
		self.placeRequiresButtons()
		self.placeObsoleteButtons()
		self.placeEvents()
		# <!-- custom: Draw order, places Texts LAST so that when Texts is expanded, its full-page overlay covers the Events panel below it (draw order in Civ4 screens is z-order — later calls paint on top). (Claude code Opus 4.7) -->
		self.placeTexts()
		place_new_concept_legend_link(self.top, "CONCEPT_SAS_SEVOPEDIA_EVENT_TRIGGERS_LEGEND")

	def _getTriggerInfo(self):
		if self.iTrigger < 0:
			return None
		return gc.getEventTriggerInfo(self.iTrigger)

	def _getTriggerDisplayName(self):
		info = self._getTriggerInfo()
		if not info:
			return localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_UNKNOWN", ())
		szName = info.getDescription()
		if szName and len(szName.strip()) > 0:
			return szName
		szType = info.getType()
		if szType and szType.startswith("EVENTTRIGGER_"):
			szType = szType[len("EVENTTRIGGER_"):]
		if szType:
			return szType.replace("_", " ").title()
		return localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_UNKNOWN", ())

	def placeName(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_VOTE_NAME", ()), "", True, False, self.X_NAME, self.Y_NAME, self.W_NAME, self.H_NAME, PanelStyles.PANEL_STYLE_BLUE50)

		info = self._getTriggerInfo()
		szDisplayName = self._getTriggerDisplayName()
		szType = ""
		if info:
			szType = info.getType()
		szLine = self.BULLET_PREFIX + szDisplayName
		if szType:
			szLine = szLine + u"  (" + szType + u")"
		screen.addMultilineText(self.top.getNextWidgetName(), SASTextScale.labelText(szLine), self.X_NAME + 14, self.Y_NAME + 30, self.W_NAME - 28, self.H_NAME - 36, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeSummary(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_SUMMARY", ()), "", True, False, self.X_SUMMARY, self.Y_SUMMARY, self.W_SUMMARY, self.H_SUMMARY, PanelStyles.PANEL_STYLE_BLUE50)

		info = self._getTriggerInfo()
		lines = []
		if info:
			lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_IWEIGHT", ()) + u": %d" % info.getProbability())
			lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_ACTIVE_GAMES", ()) + u": %d%%" % info.getPercentGamesActive())
			listFlags = []
			if info.isRecurring():
				listFlags.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_FLAG_RECURRING", ()))
			if info.isGlobal():
				listFlags.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_FLAG_GLOBAL", ()))
			if info.isTeam():
				listFlags.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_FLAG_TEAM", ()))
			if info.isSinglePlayer():
				listFlags.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_FLAG_SINGLE_PLAYER", ()))
			if listFlags:
				szFlags = u", ".join(listFlags)
			else:
				szFlags = localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_NONE", ())
			lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_FLAGS", ()) + u": " + szFlags)

		if lines:
			szText = u"\n".join(lines)
		else:
			szText = localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_NONE", ())
		screen.addMultilineText(self.top.getNextWidgetName(), SASTextScale.labelText(szText), self.X_SUMMARY + 14, self.Y_SUMMARY + 34, self.W_SUMMARY - 28, self.H_SUMMARY - 44, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeRequiresText(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_REQUIRES", ()) + u" (Text)", "", True, False, self.X_REQUIRES_TEXT, self.Y_REQUIRES_TEXT, self.W_REQUIRES_TEXT, self.H_REQUIRES_TEXT, PanelStyles.PANEL_STYLE_BLUE50)

		info = self._getTriggerInfo()
		lines = []
		if info:
			# <!-- custom: only non-linkable numeric thresholds live here as text.
			# All linkable prereqs (direct techs, civic, obsolete techs, OtherPlayerHasTech, required buildings/units/religions/corporations) are icon buttons in the Requires-Buttons panel below, which avoids duplicating every asset name as both an icon and a text line. (Claude code Opus 4.7) -->
			if info.getMinPopulation() > 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_MIN_POP", ()) + u": %d" % info.getMinPopulation())
			if info.getMaxPopulation() > 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_MAX_POP", ()) + u": %d" % info.getMaxPopulation())
			if info.getMinTreasury() > 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_MIN_TREASURY", ()) + u": %d" % info.getMinTreasury())
			if info.getNumUnits() > 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_CITY_UNITS", ()) + u": %d" % info.getNumUnits())
			if info.getNumBuildings() > 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_CITY_BUILDINGS", ()) + u": %d" % info.getNumBuildings())
			if info.getNumUnitsGlobal() > 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_GLOBAL_UNITS", ()) + u": %d" % info.getNumUnitsGlobal())
			if info.getNumBuildingsGlobal() > 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_GLOBAL_BUILDINGS", ()) + u": %d" % info.getNumBuildingsGlobal())
			if info.getMinDifficulty() >= 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_MIN_DIFFICULTY", ()) + u": " + gc.getHandicapInfo(info.getMinDifficulty()).getDescription())
			if info.getAngry() > 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_ANGRY", ()) + u": %d" % info.getAngry())
			if info.getUnhealthy() > 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_UNHEALTHY", ()) + u": %d" % info.getUnhealthy())
			if info.getMinMapLandmass() > 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_MIN_MAP_LANDMASS", ()) + u": %d" % info.getMinMapLandmass())
			if info.getMinOurLandmass() > 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_MIN_OUR_LANDMASS", ()) + u": %d" % info.getMinOurLandmass())
			if info.getMaxOurLandmass() >= 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_MAX_OUR_LANDMASS", ()) + u": %d" % info.getMaxOurLandmass())
			if info.getNumPlotsRequired() > 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_PLOTS_REQUIRED", ()) + u": %d" % info.getNumPlotsRequired())
			if info.isOwnPlot():
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_OWN_PLOT", ()))
			if info.getPlotsType() >= 0:
				iPlotType = info.getPlotsType()
				if iPlotType < len(self.PLOT_TYPE_LABELS):
					lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_PLOT_TYPE", ()) + u": " + self.PLOT_TYPE_LABELS[iPlotType])
				else:
					lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_PLOT_TYPE", ()) + u": %d" % iPlotType)
			if info.getNumReligions() > 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_NUM_RELIGIONS", ()) + u": %d" % info.getNumReligions())
			if info.getNumCorporations() > 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_NUM_CORPORATIONS", ()) + u": %d" % info.getNumCorporations())
			listFlagKeys = []
			if info.isPickReligion():
				listFlagKeys.append("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_PICK_RELIGION")
			if info.isStateReligion():
				listFlagKeys.append("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_STATE_RELIGION")
			if info.isHolyCity():
				listFlagKeys.append("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_HOLY_CITY")
			if info.isPickCorporation():
				listFlagKeys.append("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_PICK_CORPORATION")
			if info.isHeadquarters():
				listFlagKeys.append("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_HEADQUARTERS")
			if info.isPrereqEventCity():
				listFlagKeys.append("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_PREREQ_EVENT_CITY")
			if info.isPickPlayer():
				listFlagKeys.append("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_PICK_PLAYER")
			if info.isOtherPlayerWar():
				listFlagKeys.append("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_OTHER_PLAYER_WAR")
			if info.isOtherPlayerHasReligion():
				listFlagKeys.append("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_OTHER_PLAYER_HAS_RELIGION")
			if info.isOtherPlayerHasOtherReligion():
				listFlagKeys.append("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_OTHER_PLAYER_HAS_OTHER_RELIGION")
			if info.isOtherPlayerAI():
				listFlagKeys.append("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_OTHER_PLAYER_AI")
			if info.isPickCity():
				listFlagKeys.append("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_PICK_CITY")
			if info.isPickOtherPlayerCity():
				listFlagKeys.append("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_PICK_OTHER_PLAYER_CITY")
			if info.isShowPlot():
				listFlagKeys.append("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_SHOW_PLOT")
			if info.isUnitsOnPlot():
				listFlagKeys.append("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_UNITS_ON_PLOT")
			if info.isProbabilityUnitMultiply():
				listFlagKeys.append("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_IWEIGHT_UNIT_MULTIPLY")
			if info.isProbabilityBuildingMultiply():
				listFlagKeys.append("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_IWEIGHT_BUILDING_MULTIPLY")
			for szKey in listFlagKeys:
				lines.append(self.BULLET_PREFIX + localText.getText(szKey, ()))
			if info.getOtherPlayerShareBorders() > 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_OTHER_PLAYER_SHARE_BORDERS", ()) + u": %d" % info.getOtherPlayerShareBorders())
			if info.getCityFoodWeight() != 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_CITY_FOOD_WEIGHT", ()) + u": %d" % info.getCityFoodWeight())
			if info.getUnitDamagedWeight() != 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_UNIT_DAMAGED_WEIGHT", ()) + u": %d" % info.getUnitDamagedWeight())
			if info.getUnitDistanceWeight() != 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_UNIT_DISTANCE_WEIGHT", ()) + u": %d" % info.getUnitDistanceWeight())
			if info.getUnitExperienceWeight() != 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_UNIT_EXPERIENCE_WEIGHT", ()) + u": %d" % info.getUnitExperienceWeight())
			listRoutes = []
			for i in range(info.getNumRoutesRequired()):
				iRoute = info.getRouteRequired(i)
				if iRoute >= 0:
					routeInfo = gc.getRouteInfo(iRoute)
					if routeInfo:
						listRoutes.append(routeInfo.getDescription())
			if listRoutes:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_REQUIRED_ROUTES", ()) + u": " + localText.getText("TXT_KEY_OR", ()).join([u" " + szRoute + u" " for szRoute in listRoutes]).strip())
			listPython = []
			if info.getPythonCanDo() and len(info.getPythonCanDo().strip()) > 0:
				listPython.append(info.getPythonCanDo())
			if info.getPythonCanDoCity() and len(info.getPythonCanDoCity().strip()) > 0:
				listPython.append(info.getPythonCanDoCity())
			if info.getPythonCanDoUnit() and len(info.getPythonCanDoUnit().strip()) > 0:
				listPython.append(info.getPythonCanDoUnit())
			if listPython:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_PYTHON_GATES", ()) + u": " + u", ".join(listPython))

		if lines:
			szText = u"\n".join(lines)
		else:
			szText = localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_NONE", ())
		screen.addMultilineText(self.top.getNextWidgetName(), SASTextScale.labelText(szText), self.X_REQUIRES_TEXT + 14, self.Y_REQUIRES_TEXT + 34, self.W_REQUIRES_TEXT - 28, self.H_REQUIRES_TEXT - 44, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	# <!-- custom: compose a per-event "what actually happens" bullet list.
	# Covers the common numeric/flag fields on CvEventInfo and inlines Civ4 commerce/yield/happy icons so effect values read at a glance.
	# If no known fields are populated we fall back to probing for a Python callback (getPythonCallback) — only in that case do we label the event "Scripted (Python callback)".
	# Otherwise events that truly do nothing directly get a plain "No direct effect" to avoid false "scripted" labels. (Claude code Opus 4.7) -->
	def _formatEventEffectSummary(self, iEvent):
		eventInfo = gc.getEventInfo(iEvent)
		if not eventInfo:
			return localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_NONE", ())

		parts = []
		# Commerce-ish numeric rewards/penalties with their icons.
		if eventInfo.getGold() != 0:
			# <!-- custom: isGoldToPlayer flips the direction — if true, the gold is gifted TO the other player (subtracted from you and handed over) rather than just added to your treasury.
			# Used by "bribe"/"gift" events. (Claude code Opus 4.7) -->
			if eventInfo.isGoldToPlayer():
				szGoldKey = "TXT_KEY_PEDIA_SAS_EVENT_EFFECT_GOLD_TO_PLAYER"
			else:
				szGoldKey = "TXT_KEY_PEDIA_SAS_EVENT_EFFECT_GOLD"
			parts.append(localText.getText(szGoldKey, ()) + u": %+d %c" % (eventInfo.getGold(), self.COMMERCE_CHARS[CommerceTypes.COMMERCE_GOLD]))
		if eventInfo.getRandomGold() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_RANDOM_GOLD", ()) + u": %+d %c" % (eventInfo.getRandomGold(), self.COMMERCE_CHARS[CommerceTypes.COMMERCE_GOLD]))
		if eventInfo.getCulture() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_CULTURE", ()) + u": %+d %c" % (eventInfo.getCulture(), self.COMMERCE_CHARS[CommerceTypes.COMMERCE_CULTURE]))
		if eventInfo.getEspionagePoints() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_ESPIONAGE", ()) + u": %+d %c" % (eventInfo.getEspionagePoints(), self.COMMERCE_CHARS[CommerceTypes.COMMERCE_ESPIONAGE]))

		# Happiness / health (flat + turn-based).
		if eventInfo.getHappy() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_HAPPY", ()) + u": %+d %s" % (eventInfo.getHappy(), self.HAPPY_CHAR))
		if eventInfo.getHappyTurns() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_HAPPY_TURNS", ()) + u": %d %s" % (eventInfo.getHappyTurns(), self.HAPPY_CHAR))
		if eventInfo.getHealth() != 0:
			# Health can be negative (unhealthy) or positive — use the sign of the value to pick the appropriate Civ4 icon rather than having two separate fields.
			if eventInfo.getHealth() > 0:
				szHealthChar = self.HEALTHY_CHAR
			else:
				szHealthChar = self.UNHEALTHY_CHAR
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_HEALTH", ()) + u": %+d %s" % (eventInfo.getHealth(), szHealthChar))

		# City / population changes.
		if eventInfo.getPopulationChange() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_POPULATION", ()) + u": %+d" % eventInfo.getPopulationChange())
		if eventInfo.getRevoltTurns() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_REVOLT_TURNS", ()) + u": %d" % eventInfo.getRevoltTurns())
		if eventInfo.getConvertOwnCities() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_CONVERT_OWN_CITIES", ()) + u": %d" % eventInfo.getConvertOwnCities())
		if eventInfo.getConvertOtherCities() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_CONVERT_OTHER_CITIES", ()) + u": %d" % eventInfo.getConvertOtherCities())

		# Flags.
		if eventInfo.isGlobal():
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_GLOBAL", ()))
		if eventInfo.isTeam():
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_TEAM", ()))
		if eventInfo.isCityEffect():
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_CITY_EFFECT", ()))
		if eventInfo.isOtherPlayerCityEffect():
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_OTHER_PLAYER_CITY_EFFECT", ()))
		if eventInfo.isGraphicalOnly():
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_GRAPHICAL_ONLY", ()))
		if eventInfo.isGoldenAge():
			parts.append(self.GOLDEN_AGE_CHAR + u" " + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_GOLDEN_AGE", ()))
		if eventInfo.isDeclareWar():
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_DECLARE_WAR", ()))
		if eventInfo.isDisbandUnit():
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_DISBAND_UNIT", ()))

		# Tech grant / discount.
		iTech = eventInfo.getTech()
		if iTech >= 0:
			techInfo = gc.getTechInfo(iTech)
			if techInfo:
				parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_FREE_TECH", ()) + u": " + make_pedia_link(techInfo.getDescription()))
		elif eventInfo.getTechPercent() > 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_TECH_PERCENT", ()) + u": %d%%" % eventInfo.getTechPercent())
		if eventInfo.getTechCostPercent() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_TECH_COST_PERCENT", ()) + u": %+d%%" % eventInfo.getTechCostPercent())
		iPrereqTech = eventInfo.getPrereqTech()
		if iPrereqTech >= 0:
			prereqTechInfo = gc.getTechInfo(iPrereqTech)
			if prereqTechInfo:
				parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_PREREQ_TECH_DETAIL", ()) + u": " + make_pedia_link(prereqTechInfo.getDescription()))
		if eventInfo.getTechMinTurnsLeft() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_TECH_MIN_TURNS_LEFT", ()) + u": %d" % eventInfo.getTechMinTurnsLeft())
		listTechFlavors = []
		for iFlavor in range(gc.getNumFlavorTypes()):
			iFlavorValue = eventInfo.getTechFlavorValue(iFlavor)
			if iFlavorValue != 0:
				szFlavor = gc.getFlavorTypes(iFlavor)
				# <!-- custom: trim the repeated "FLAVOR_" prefix because tech flavor lists can contain many entries; saving horizontal space reduces wrapping/scrolling and makes the event-effect bullet easier to scan. (GPT-5.5) -->
				if szFlavor.startswith("FLAVOR_"):
					szFlavor = szFlavor[len("FLAVOR_"):]
				listTechFlavors.append(szFlavor + u" %+d" % iFlavorValue)
		if listTechFlavors:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_TECH_FLAVORS", ()) + u": " + u", ".join(listTechFlavors))

		# Modifiers (%).
		if eventInfo.getInflationModifier() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_INFLATION", ()) + u": %+d%%" % eventInfo.getInflationModifier())
		if eventInfo.getSpaceProductionModifier() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_SPACE_PRODUCTION", ()) + u": %+d%%" % eventInfo.getSpaceProductionModifier())

		# Diplomacy.
		if eventInfo.getOurAttitudeModifier() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_OUR_ATTITUDE", ()) + u": %+d" % eventInfo.getOurAttitudeModifier())
		if eventInfo.getAttitudeModifier() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_THEIR_ATTITUDE", ()) + u": %+d" % eventInfo.getAttitudeModifier())
		if eventInfo.getTheirEnemyAttitudeModifier() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_THEIR_ENEMY_ATTITUDE", ()) + u": %+d" % eventInfo.getTheirEnemyAttitudeModifier())

		# Free units / promotion grants.
		iFreeUnitClass = eventInfo.getUnitClass()
		if iFreeUnitClass >= 0 and eventInfo.getNumUnits() > 0:
			unitClassInfo = gc.getUnitClassInfo(iFreeUnitClass)
			if unitClassInfo:
				parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_FREE_UNITS", ()) + u": %dx %s" % (eventInfo.getNumUnits(), make_pedia_link(unitClassInfo.getDescription())))
		szUnitNameKey = eventInfo.getUnitNameKey()
		if szUnitNameKey and len(szUnitNameKey.strip()) > 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_UNIT_NAME", ()) + u": " + localText.getText(str(szUnitNameKey), ()))
		iFreeBuildingClass = eventInfo.getBuildingClass()
		if iFreeBuildingClass >= 0:
			buildingClassInfo = gc.getBuildingClassInfo(iFreeBuildingClass)
			if buildingClassInfo:
				# <!-- custom: getBuildingChange > 0 grants the building, < 0 destroys it.
				# Same field on CvEventInfo drives both directions — surface them distinctly so destroyed buildings don't show up as "Free Building". (Claude code Opus 4.7) -->
				iBuildingChange = eventInfo.getBuildingChange()
				szLinkedBC = make_pedia_link(buildingClassInfo.getDescription())
				if iBuildingChange > 0:
					parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_FREE_BUILDING", ()) + u": " + szLinkedBC)
				elif iBuildingChange < 0:
					parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_DESTROY_BUILDING", ()) + u": " + szLinkedBC)
				else:
					parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_FREE_BUILDING", ()) + u": " + szLinkedBC)
		iFreePromotion = eventInfo.getUnitPromotion()
		if iFreePromotion >= 0:
			promoInfo = gc.getPromotionInfo(iFreePromotion)
			if promoInfo:
				parts.append(self.STAR_CHAR + u" " + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_FREE_PROMOTION", ()) + u": " + make_pedia_link(promoInfo.getDescription()))

		# Plot / terrain / feature changes.
		iFeature = eventInfo.getFeature()
		if iFeature >= 0:
			featureInfo = gc.getFeatureInfo(iFeature)
			if featureInfo and eventInfo.getFeatureChange() != 0:
				parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_FEATURE_CHANGE", ()) + u": " + make_pedia_link(featureInfo.getDescription()))
		iImprovement = eventInfo.getImprovement()
		if iImprovement >= 0:
			improvementInfo = gc.getImprovementInfo(iImprovement)
			if improvementInfo and eventInfo.getImprovementChange() != 0:
				parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_IMPROVEMENT_CHANGE", ()) + u": " + make_pedia_link(improvementInfo.getDescription()))
		elif eventInfo.getImprovementChange() != 0:
			# <!-- custom: Some events set iImprovementChange while ImprovementType is NONE.
			# Negative values remove whatever improvement is on the target plot; positive values are surfaced as a raw flag because the DLL only applies them when a concrete ImprovementType is provided.
			# Showing this avoids false "No direct effect" cards. (GPT-5.5) -->
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_CURRENT_IMPROVEMENT_CHANGE", ()) + u": %+d" % eventInfo.getImprovementChange())
		iBonus = eventInfo.getBonus()
		if iBonus >= 0:
			bonusInfo = gc.getBonusInfo(iBonus)
			if bonusInfo and eventInfo.getBonusRevealed():
				parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_BONUS_REVEALED", ()) + u": " + make_pedia_link(bonusInfo.getDescription()))
			if bonusInfo and eventInfo.getBonusChange() != 0:
				parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_BONUS_CHANGE", ()) + u": %+d " % eventInfo.getBonusChange() + make_pedia_link(bonusInfo.getDescription()))

		# <!-- custom: <BonusGift> — gifts a bonus resource to the other player in the popup chain. Core effect for events like Brothers In Need (the "help with Copper/Iron/Horses/..." choices). (Claude code Opus 4.7) -->
		iBonusGift = eventInfo.getBonusGift()
		if iBonusGift >= 0:
			bonusGiftInfo = gc.getBonusInfo(iBonusGift)
			if bonusGiftInfo:
				parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_BONUS_GIFT", ()) + u": " + make_pedia_link(bonusGiftInfo.getDescription()))

		# <!-- custom: <RouteType> + <iRouteChange> — add/remove a road/railroad on the target plot. (Claude code Opus 4.7) -->
		iRoute = eventInfo.getRoute()
		if iRoute >= 0 and eventInfo.getRouteChange() != 0:
			routeInfo = gc.getRouteInfo(iRoute)
			if routeInfo:
				parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_ROUTE_CHANGE", ()) + u": %+d " % eventInfo.getRouteChange() + make_pedia_link(routeInfo.getDescription()))
		elif eventInfo.getRouteChange() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_CURRENT_ROUTE_CHANGE", ()) + u": %+d" % eventInfo.getRouteChange())

		# <!-- custom: <OtherPlayerPopup> signals that the *other* player in the chain gets their own popup with response choices — the real consequences of this branch are applied by whichever response they pick. Surfacing this so readers know the choice has downstream effects even if no XML effect fires on this side. (Claude code Opus 4.7) -->
		szOtherPopup = eventInfo.getOtherPlayerPopup()
		if szOtherPopup and len(str(szOtherPopup).strip()) > 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_OTHER_PLAYER_RESPONDS", ()))

		# <!-- custom: per-BuildingClass extras — <BuildingExtraHappies>, <...Healths>, <BuildingExtraCommerces>, <BuildingExtraYields>.
		# These are list-structs in XML and are queried via scalar accessors that take a BuildingClass index (plus a Commerce/Yield index for those two).
		# Scanning all classes is the only way to surface them since there's no getNum... method.
		# This is what finally makes events like Antimonarchists (Palace +3 happy, Cathedrals +2 gold each) show their actual effect instead of falling through to "No direct effect". (Claude code Opus 4.7) -->
		iNumBuildingClass = gc.getNumBuildingClassInfos()
		iNumCommerce = CommerceTypes.NUM_COMMERCE_TYPES
		iNumYield = YieldTypes.NUM_YIELD_TYPES
		for iBC in range(iNumBuildingClass):
			buildingClassInfo = gc.getBuildingClassInfo(iBC)
			if not buildingClassInfo:
				continue
			szBCName = buildingClassInfo.getDescription()
			iDefault = buildingClassInfo.getDefaultBuildingIndex()
			if iDefault >= 0:
				defaultBuilding = gc.getBuildingInfo(iDefault)
				if defaultBuilding:
					szBCName = defaultBuilding.getDescription()

			szLinkedBCName = make_pedia_link(szBCName)
			iHappyBC = eventInfo.getBuildingHappyChange(iBC)
			if iHappyBC != 0:
				parts.append(szLinkedBCName + u": %+d %s" % (iHappyBC, self.HAPPY_CHAR))
			iHealthBC = eventInfo.getBuildingHealthChange(iBC)
			if iHealthBC != 0:
				if iHealthBC > 0:
					szHealthBCChar = self.HEALTHY_CHAR
				else:
					szHealthBCChar = self.UNHEALTHY_CHAR
				parts.append(szLinkedBCName + u": %+d %s" % (iHealthBC, szHealthBCChar))
			for iC in range(iNumCommerce):
				iVal = eventInfo.getBuildingCommerceChange(iBC, iC)
				if iVal != 0:
					parts.append(szLinkedBCName + u": %+d %c" % (iVal, self.COMMERCE_CHARS[iC]))
			for iY in range(iNumYield):
				iVal = eventInfo.getBuildingYieldChange(iBC, iY)
				if iVal != 0:
					parts.append(szLinkedBCName + u": %+d %c" % (iVal, self.YIELD_CHARS[iY]))

		# <!-- custom: per-UnitClass and per-UnitCombat promotion grants.
		# Same list-struct pattern as the per-BuildingClass extras: the XML tags <UnitClassPromotions> and <UnitCombatPromotions> are probed via scalar accessors that take the class/combat index and return -1 for "no promotion".
		# Catches events like Noble Knights Done_1 that grant PROMOTION_RETREAT1 to Knight and Camel Knight classes. (Claude code Opus 4.7) -->
		for iUC in range(gc.getNumUnitClassInfos()):
			iPromo = eventInfo.getUnitClassPromotion(iUC)
			if iPromo >= 0:
				unitClassInfo = gc.getUnitClassInfo(iUC)
				promoInfo = gc.getPromotionInfo(iPromo)
				if unitClassInfo and promoInfo:
					parts.append(self.STAR_CHAR + u" " + make_pedia_link(unitClassInfo.getDescription()) + u": " + make_pedia_link(promoInfo.getDescription()))
		for iComb in range(gc.getNumUnitCombatInfos()):
			iPromo = eventInfo.getUnitCombatPromotion(iComb)
			if iPromo >= 0:
				combatInfo = gc.getUnitCombatInfo(iComb)
				promoInfo = gc.getPromotionInfo(iPromo)
				if combatInfo and promoInfo:
					parts.append(self.STAR_CHAR + u" " + make_pedia_link(combatInfo.getDescription()) + u": " + make_pedia_link(promoInfo.getDescription()))

		# <!-- custom: <FreeSpecialistCounts> — grants N free specialists of a given type in the target city (e.g. Noble Knights Done_3: +1 Great Priest). Probed per Specialist index, same pattern as UnitClass above. (Claude code Opus 4.7) -->
		for iSp in range(gc.getNumSpecialistInfos()):
			iCount = eventInfo.getFreeSpecialistCount(iSp)
			if iCount != 0:
				specInfo = gc.getSpecialistInfo(iSp)
				if specInfo:
					parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_FREE_SPECIALIST", ()) + u": %dx %s" % (iCount, make_pedia_link(specInfo.getDescription())))

		# Other less-common scalar fields that can still carry the whole effect on their own.
		szPythonCanDo = eventInfo.getPythonCanDo()
		if szPythonCanDo and len(szPythonCanDo.strip()) > 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_PYTHON_CAN_DO", ()) + u": " + szPythonCanDo)
		szPythonCallback = eventInfo.getPythonCallback()
		if szPythonCallback and len(szPythonCallback.strip()) > 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_SCRIPTED", ()) + u": " + szPythonCallback)
		szPythonHelp = eventInfo.getPythonHelp()
		if szPythonHelp and len(szPythonHelp.strip()) > 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_PYTHON_HELP", ()) + u": " + szPythonHelp)
		szPythonExpireCheck = eventInfo.getPythonExpireCheck()
		if szPythonExpireCheck and len(szPythonExpireCheck.strip()) > 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_PYTHON_EXPIRE_CHECK", ()) + u": " + szPythonExpireCheck)
		if eventInfo.getHurryAnger() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_HURRY_ANGER", ()) + u": %+d" % eventInfo.getHurryAnger())
		if eventInfo.getFood() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_FOOD", ()) + u": %+d" % eventInfo.getFood())
		if eventInfo.getFoodPercent() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_FOOD_PERCENT", ()) + u": %+d%%" % eventInfo.getFoodPercent())
		if eventInfo.getUnitExperience() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_UNIT_EXPERIENCE", ()) + u": %+d" % eventInfo.getUnitExperience())
		if eventInfo.getUnitImmobileTurns() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_UNIT_IMMOBILE", ()) + u": %d" % eventInfo.getUnitImmobileTurns())
		if eventInfo.isQuest():
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_QUEST", ()))

		if eventInfo.getFreeUnitSupport() != 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_FREE_UNIT_SUPPORT", ()) + u": %+d" % eventInfo.getFreeUnitSupport())
		iMaxReligions = eventInfo.getMaxNumReligions()
		if iMaxReligions >= 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_MAX_RELIGIONS", ()) + u": %d" % iMaxReligions)

		# <!-- custom: getMinPillage/getMaxPillage are the number of improvements destroyed by the event, not treasury gold.
		# Show one count when equal or the possible count range, without a gold-commerce glyph. See KI#226. (Claude code Opus 4.7 + ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		iMinPillage = eventInfo.getMinPillage()
		iMaxPillage = eventInfo.getMaxPillage()
		if iMinPillage != 0 or iMaxPillage != 0:
			if iMinPillage == iMaxPillage:
				szPillageCount = unicode(iMinPillage)
			else:
				szPillageCount = u"%d-%d" % (iMinPillage, iMaxPillage)
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_PILLAGE_IMPROVEMENTS", ()) + u": " + szPillageCount)

		# <!-- custom: <PlotExtraYields> — permanent yield bonus on the target plot, indexed by Yield type. Same list-struct probe pattern as the per-BuildingClass extras. Useful for events like "The Volcano has fertilized this plot: +2 food". (Claude code Opus 4.7) -->
		for iY in range(YieldTypes.NUM_YIELD_TYPES):
			iExtra = eventInfo.getPlotExtraYield(iY)
			if iExtra != 0:
				parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_PLOT_EXTRA_YIELD", ()) + u": %+d %c" % (iExtra, self.YIELD_CHARS[iY]))

		# <!-- custom: <AdditionalEvents> — follow-up events that may fire after this one, each with a %chance and a turn delay. Probed per event index (same pattern as the BuildingClass scan). Surfaces event chains like "This opens a quest that will resolve in N turns". (Claude code Opus 4.7) -->
		for iOtherEvent in range(gc.getNumEventInfos()):
			iChance = eventInfo.getAdditionalEventChance(iOtherEvent)
			if iChance <= 0:
				continue
			otherInfo = gc.getEventInfo(iOtherEvent)
			if not otherInfo:
				continue
			iTime = eventInfo.getAdditionalEventTime(iOtherEvent)
			szOtherName = otherInfo.getDescription()
			if (not szOtherName) or len(szOtherName.strip()) == 0:
				szOtherName = otherInfo.getType()
			if iTime > 0:
				parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_ADDITIONAL_EVENT", ()) + u": %s (%d%%, %d turns)" % (szOtherName, iChance, iTime))
			else:
				parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_ADDITIONAL_EVENT", ()) + u": %s (%d%%)" % (szOtherName, iChance))

		# <!-- custom: Surface event-chain cleanup and presentation/script text hooks too.
		# These are not always direct mechanical rewards, but they are populated CvEventInfo effects/metadata and otherwise some cards misleadingly fall through to "No direct effect".
		# Keep labels explicit so readers know what kind of field it is. (GPT-5.5) -->
		for iOtherEvent in range(gc.getNumEventInfos()):
			iChance = eventInfo.getClearEventChance(iOtherEvent)
			if iChance <= 0:
				continue
			otherInfo = gc.getEventInfo(iOtherEvent)
			if not otherInfo:
				continue
			szOtherName = otherInfo.getDescription()
			if (not szOtherName) or len(szOtherName.strip()) == 0:
				szOtherName = otherInfo.getType()
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_CLEAR_EVENT", ()) + u": %s (%d%%)" % (szOtherName, iChance))

		for iNews in range(eventInfo.getNumWorldNews()):
			szWorldNewsKey = eventInfo.getWorldNews(iNews)
			if szWorldNewsKey and len(szWorldNewsKey.strip()) > 0:
				parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_WORLD_NEWS", ()) + u": " + localText.getText(str(szWorldNewsKey), ()))
		szLocalInfoKey = eventInfo.getLocalInfoTextKey()
		if szLocalInfoKey and len(szLocalInfoKey.strip()) > 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_LOCAL_INFO", ()) + u": " + localText.getText(str(szLocalInfoKey), ()))
		szQuestFailKey = eventInfo.getQuestFailTextKey()
		if szQuestFailKey and len(szQuestFailKey.strip()) > 0:
			parts.append(localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_QUEST_FAIL_TEXT", ()) + u": " + localText.getText(str(szQuestFailKey), ()))

		if not parts:
			return self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_EVENT_EFFECT_NO_DIRECT", ())
		return u"\n".join([self.BULLET_PREFIX + p for p in parts])

	def _getEventDisplayName(self, iEvent):
		eventInfo = gc.getEventInfo(iEvent)
		if not eventInfo:
			return localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_UNKNOWN", ())
		szName = eventInfo.getDescription()
		if szName and len(szName.strip()) > 0:
			return szName
		szType = eventInfo.getType()
		if szType and szType.startswith("EVENT_"):
			szType = szType[len("EVENT_"):]
		if szType:
			return szType.replace("_", " ").title()
		return localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_UNKNOWN", ())

	def setHistoryExpanded(self, bExpanded):
		# Renamed from bTextsExpanded at the method level only — the dispatch layer in SevoPediaMain always calls setHistoryExpanded (it's the shared hook), and on this page the expandable panel happens to be Texts.
		self.bHistoryExpanded = bExpanded

	# <!-- custom: render the trigger's narrator text variants (from getNumTexts() / getText(i), matching the XML <Texts><Text>...</Text></Texts> block).
	# These are the variants the engine picks from at runtime to introduce the event popup — surfacing them here so Sevopedia readers see the same flavor context a player gets when the trigger actually fires.
	# Rendered through draw_expandable_text_panel so very long / multi-variant Texts stay readable when they overflow the panel. (Claude code Opus 4.7) -->
	def placeTexts(self):
		screen = self.top.getScreen()
		info = self._getTriggerInfo()
		# <!-- custom: dedupe identical resolved text variants and append a "(xN)" multiplier to each repeated entry.
		# Many vanilla BtS triggers list the same flavor string several times to weight the random roll (e.g. Joyous Wedding repeats "A ???
		# industrial heiress..."
		# 3 times); showing them as separate bullets just wastes vertical space without telling the reader anything new.
		# First-seen insertion order is preserved so the bullet sequence still matches the XML declaration order. (Claude code Opus 4.7) -->
		order = []
		counts = {}
		if info:
			for i in range(info.getNumTexts()):
				szKey = info.getText(i)
				if szKey and len(szKey.strip()) > 0:
					szResolved = localText.getText(str(szKey), ())
					if szResolved and len(szResolved.strip()) > 0:
						if szResolved not in counts:
							counts[szResolved] = 0
							order.append(szResolved)
						counts[szResolved] += 1

		lines = []
		for szResolved in order:
			iCount = counts[szResolved]
			if iCount > 1:
				lines.append(self.BULLET_PREFIX + szResolved + u" (x%d)" % iCount)
			else:
				lines.append(self.BULLET_PREFIX + szResolved)

		if lines:
			szBody = u"\n".join(lines)
		else:
			szBody = localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_NONE", ())

		szTitle = localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_TEXTS", ())
		draw_expandable_text_panel(screen, self.top, szTitle, self.X_TEXTS, self.Y_TEXTS, self.W_TEXTS, self.H_TEXTS, szBody, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)
	def placeEvents(self):
		screen = self.top.getScreen()
		info = self._getTriggerInfo()

		# Header panel covers the full events strip; per-event "choice cards" live inside.
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_EVENTS_HEADER", ()), "", True, False, self.X_EVENTS, self.Y_EVENTS, self.W_EVENTS, self.H_EVENTS, PanelStyles.PANEL_STYLE_BLUE50)

		if not info:
			return

		events = []
		for i in range(info.getNumEvents()):
			iEvent = info.getEvent(i)
			if iEvent < 0:
				continue
			eventInfo = gc.getEventInfo(iEvent)
			if not eventInfo:
				continue
			events.append((iEvent, eventInfo))

		if not events:
			noneText = localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_NONE", ())
			screen.addMultilineText(self.top.getNextWidgetName(), SASTextScale.labelText(noneText), self.X_EVENTS + 10, self.Y_EVENTS + 30, self.W_EVENTS - 20, self.H_EVENTS - 40, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			return

		cardCount = len(events)
		# Per-card panel geometry — cards sit inside the events strip, offset below its header.
		cardMargin = 4
		cardGap = 4
		cardInnerMargin = 4
		panelGap = 4
		innerX = self.X_EVENTS + cardMargin
		innerY = self.Y_EVENTS + self.EVENT_CARD_PANEL_HEADER_H + 2
		innerW = self.W_EVENTS - (2 * cardMargin)
		innerH = self.H_EVENTS - self.EVENT_CARD_PANEL_HEADER_H - cardMargin - 2

		cardW = (innerW - ((cardCount - 1) * cardGap)) / cardCount
		cardH = innerH

		availableInnerH = cardH - (2 * cardInnerMargin) - panelGap
		# Summary panel needs enough room for a few bullet lines of effects — ~60 px floor keeps it readable when the event name is long and pushes the split down.
		minSummaryPanelH = 60
		namePanelH = 120
		maxNamePanelH = availableInnerH - minSummaryPanelH
		if namePanelH > maxNamePanelH:
			namePanelH = maxNamePanelH
		if namePanelH < 44:
			namePanelH = 44

		for i, (iEvent, eventInfo) in enumerate(events):
			cardX = innerX + (i * (cardW + cardGap))
			cardY = innerY

			namePanelX = cardX + cardInnerMargin
			namePanelY = cardY + cardInnerMargin
			namePanelW = cardW - (2 * cardInnerMargin)

			summaryPanelX = namePanelX
			summaryPanelY = namePanelY + namePanelH + panelGap
			summaryPanelW = namePanelW
			summaryPanelH = availableInnerH - namePanelH

			namePanel = self.top.getNextWidgetName()
			screen.addPanel(namePanel, "", "", False, False, namePanelX, namePanelY, namePanelW, namePanelH, PanelStyles.PANEL_STYLE_BLUE50)

			summaryPanel = self.top.getNextWidgetName()
			screen.addPanel(summaryPanel, "", "", False, False, summaryPanelX, summaryPanelY, summaryPanelW, summaryPanelH, PanelStyles.PANEL_STYLE_BLUE50)

			szName = self._getEventDisplayName(iEvent)
			textMarginX = 10
			textMarginY = 8
			screen.addMultilineText(self.top.getNextWidgetName(), SASTextScale.labelText(szName), namePanelX + textMarginX, namePanelY + textMarginY, namePanelW - (textMarginX * 2), namePanelH - (textMarginY * 2), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			# <!-- custom: effect summary fills the entire card body.
			# Dropped the bottom event icon (eventInfo.getButton) since it eats real estate and the card header already labels the choice; mirrors the "upper name box" pattern where text uses the full panel. (Claude code Opus 4.7) -->
			szSummary = self._formatEventEffectSummary(iEvent)
			summaryTextTop = textMarginY
			summaryTextH = summaryPanelH - (textMarginY * 2)
			if summaryTextH < 20:
				summaryTextH = 20
			screen.addMultilineText(self.top.getNextWidgetName(), SASTextScale.labelText(szSummary), summaryPanelX + textMarginX, summaryPanelY + summaryTextTop, summaryPanelW - (textMarginX * 2), summaryTextH, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeRequiresButtons(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_REQUIRES", ()) + u" (Buttons)", "", False, True, self.X_REQUIRES_BUTTONS, self.Y_REQUIRES_BUTTONS, self.W_REQUIRES_BUTTONS, self.H_REQUIRES_BUTTONS, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		info = self._getTriggerInfo()
		if not info:
			return

		# <!-- custom: multilist layout (a la SevoPediaUnit.placeRequires) so each button gets a consistent grid slot and we can place a numTxt under the ones that need disambiguating.
		# Only the OtherPlayerHasTech button carries a "Oth.P." numTxt — direct AND-tech / OR-tech / civic / required-building/unit/religion/corporation icons are self-identifying by art and don't need extra text.
		# AND/OR separator labels are dropped vs the previous inline flow; the distinction is rare in vanilla triggers and keeping multilist discipline is worth the small loss. (Claude code Opus 4.7) -->
		rowListName = self.top.getNextWidgetName()
		multiListX = self.X_REQUIRES_BUTTONS + MULTI_LIST_PANEL_OFFSET_X
		multiListY = self.Y_REQUIRES_BUTTONS + MULTI_LIST_PANEL_OFFSET_Y
		multiListW = self.W_REQUIRES_BUTTONS + MULTI_LIST_PANEL_ADDITIONAL_W
		multiListH = self.H_REQUIRES_BUTTONS + MULTI_LIST_PANEL_ADDITIONAL_H
		screen.addMultiListControlGFC(rowListName, "", multiListX, multiListY, multiListW, multiListH, SEVOPEDIA_MULTILIST_NUM_LISTS_AUTO_CALCULATE, MULTILIST_BUTTON_SIZE, MULTILIST_BUTTON_SIZE, TableStyles.TABLE_STYLE_STANDARD)

		iButtonIndex = 0
		maxButtonsPerRow = get_multilist_max_buttons_per_row(multiListW, MULTILIST_BUTTON_SIZE)
		szOrLabel = localText.getText("TXT_KEY_OR", ())

		# <!-- custom: OtherPlayerHasTech is emitted FIRST so its "Oth.P." numTxt lands in the earliest grid slot.
		# numTxt is placed at absolute coordinates (no grid scroll support), so keeping the labeled button on the first row guarantees the label stays visible even if later buttons wrap past the panel height. (Claude code Opus 4.7) -->
		iOtherTech = info.getOtherPlayerHasTech()
		if iOtherTech >= 0:
			otherTechInfo = gc.getTechInfo(iOtherTech)
			if otherTechInfo:
				screen.appendMultiListButton(rowListName, otherTechInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iOtherTech, -1, False)
				add_multilist_numTxt_under_button(multiListX, multiListY, -3, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, u"Oth.P.", screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)
				iButtonIndex += 1

		# <!-- custom: direct AND prereqs are intentionally shown as adjacent unlabeled icons.
		# Rendering "and" between every required icon makes dense panels noisy; in Sevopedia icon rows, plain adjacency means all are required, while explicit connector numTxt is reserved for real alternatives such as "A or B". (GPT-5.5) -->
		seenAnd = {}
		for i in range(info.getNumPrereqAndTechs()):
			iTech = info.getPrereqAndTechs(i)
			if iTech < 0 or iTech in seenAnd:
				continue
			seenAnd[iTech] = True
			techInfo = gc.getTechInfo(iTech)
			if techInfo:
				screen.appendMultiListButton(rowListName, techInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, -1, False)
				iButtonIndex += 1

		# <!-- custom: direct OR prereq techs use connector numTxt between buttons, so they read as "A or B" instead of labeling either icon as "or".
		# Single OR entries do not need a connector. (GPT-5.5) -->
		seenOr = {}
		orTechList = []
		for i in range(info.getNumPrereqOrTechs()):
			iTech = info.getPrereqOrTechs(i)
			if iTech >= 0 and iTech not in seenOr:
				seenOr[iTech] = True
				orTechList.append(iTech)
		bFirstOrTech = True
		for iTech in orTechList:
			techInfo = gc.getTechInfo(iTech)
			if techInfo:
				if bFirstOrTech:
					szConnector = None
					bFirstOrTech = False
				else:
					szConnector = szOrLabel
				add_multilist_connector_numTxt_before_button(multiListX, multiListY, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, szConnector, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)
				screen.appendMultiListButton(rowListName, techInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, -1, False)
				iButtonIndex += 1

		# Civic.
		iCivic = info.getCivic()
		if iCivic >= 0:
			civicInfo = gc.getCivicInfo(iCivic)
			if civicInfo:
				screen.appendMultiListButton(rowListName, civicInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, iCivic, 1, False)
				iButtonIndex += 1

		# Required buildings — display default building of the class.
		for i in range(info.getNumBuildingsRequired()):
			iBC = info.getBuildingRequired(i)
			if iBC < 0:
				continue
			bcInfo = gc.getBuildingClassInfo(iBC)
			if not bcInfo:
				continue
			iDefault = bcInfo.getDefaultBuildingIndex()
			if iDefault < 0:
				continue
			buildingInfo = gc.getBuildingInfo(iDefault)
			if buildingInfo:
				screen.appendMultiListButton(rowListName, buildingInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iDefault, 1, False)
				iButtonIndex += 1

		# Required units — default unit of the class.
		for i in range(info.getNumUnitsRequired()):
			iUC = info.getUnitRequired(i)
			if iUC < 0:
				continue
			ucInfo = gc.getUnitClassInfo(iUC)
			if not ucInfo:
				continue
			iDefaultUnit = ucInfo.getDefaultUnitIndex()
			if iDefaultUnit < 0:
				continue
			unitInfo = gc.getUnitInfo(iDefaultUnit)
			if unitInfo:
				screen.appendMultiListButton(rowListName, unitInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iDefaultUnit, 1, False)
				iButtonIndex += 1

		# Required religions.
		for i in range(info.getNumReligionsRequired()):
			iRel = info.getReligionRequired(i)
			if iRel < 0:
				continue
			relInfo = gc.getReligionInfo(iRel)
			if relInfo:
				screen.appendMultiListButton(rowListName, relInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION, iRel, 1, False)
				iButtonIndex += 1

		# Required corporations.
		for i in range(info.getNumCorporationsRequired()):
			iCorp = info.getCorporationRequired(i)
			if iCorp < 0:
				continue
			corpInfo = gc.getCorporationInfo(iCorp)
			if corpInfo:
				screen.appendMultiListButton(rowListName, corpInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CORPORATION, iCorp, 1, False)
				iButtonIndex += 1

		iListButtonIndex = 0
		for i in range(info.getNumFeaturesRequired()):
			iFeature = info.getFeatureRequired(i)
			if iFeature < 0:
				continue
			featureInfo = gc.getFeatureInfo(iFeature)
			if featureInfo:
				if iListButtonIndex > 0:
					add_multilist_connector_numTxt_before_button(multiListX, multiListY, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, szOrLabel, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)
				screen.appendMultiListButton(rowListName, featureInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_FEATURE, iFeature, 1, False)
				iButtonIndex += 1
				iListButtonIndex += 1

		iListButtonIndex = 0
		for i in range(info.getNumTerrainsRequired()):
			iTerrain = info.getTerrainRequired(i)
			if iTerrain < 0:
				continue
			terrainInfo = gc.getTerrainInfo(iTerrain)
			if terrainInfo:
				if iListButtonIndex > 0:
					add_multilist_connector_numTxt_before_button(multiListX, multiListY, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, szOrLabel, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)
				screen.appendMultiListButton(rowListName, terrainInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TERRAIN, iTerrain, 1, False)
				iButtonIndex += 1
				iListButtonIndex += 1

		iListButtonIndex = 0
		for i in range(info.getNumImprovementsRequired()):
			iImprovement = info.getImprovementRequired(i)
			if iImprovement < 0:
				continue
			improvementInfo = gc.getImprovementInfo(iImprovement)
			if improvementInfo:
				if iListButtonIndex > 0:
					add_multilist_connector_numTxt_before_button(multiListX, multiListY, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, szOrLabel, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)
				screen.appendMultiListButton(rowListName, improvementInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_IMPROVEMENT, iImprovement, 1, False)
				iButtonIndex += 1
				iListButtonIndex += 1

		iListButtonIndex = 0
		for i in range(info.getNumBonusesRequired()):
			iBonus = info.getBonusRequired(i)
			if iBonus < 0:
				continue
			bonusInfo = gc.getBonusInfo(iBonus)
			if bonusInfo:
				if iListButtonIndex > 0:
					add_multilist_connector_numTxt_before_button(multiListX, multiListY, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, szOrLabel, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)
				screen.appendMultiListButton(rowListName, bonusInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus, 1, False)
				iButtonIndex += 1
				iListButtonIndex += 1

		iListButtonIndex = 0
		for i in range(info.getNumPrereqEvents()):
			iPrereqEvent = info.getPrereqEvent(i)
			if iPrereqEvent < 0:
				continue
			eventInfo = gc.getEventInfo(iPrereqEvent)
			if not eventInfo:
				continue
			iTargetTrigger = -1
			for iLoopTrigger in range(gc.getNumEventTriggerInfos()):
				loopInfo = gc.getEventTriggerInfo(iLoopTrigger)
				if not loopInfo:
					continue
				for j in range(loopInfo.getNumEvents()):
					if loopInfo.getEvent(j) == iPrereqEvent:
						iTargetTrigger = iLoopTrigger
						break
				if iTargetTrigger >= 0:
					break
			if iTargetTrigger >= 0:
				if iListButtonIndex > 0:
					add_multilist_connector_numTxt_before_button(multiListX, multiListY, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, szOrLabel, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)
				screen.appendMultiListButton(rowListName, eventInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_EVENT_TRIGGER_ENTRY, iTargetTrigger, False)
				iButtonIndex += 1
				iListButtonIndex += 1

		if iButtonIndex == 0:
			szText = localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_NONE", ())
			yCenter = self.Y_REQUIRES_BUTTONS + (self.H_REQUIRES_BUTTONS / 2)
			screen.addMultilineText(self.top.getNextWidgetName(), SASTextScale.labelText(szText), self.X_REQUIRES_BUTTONS + 7, yCenter, self.W_REQUIRES_BUTTONS - 14, self.H_REQUIRES_BUTTONS - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeObsoleteButtons(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_OBSOLETE_TECHS", ()), "", False, True, self.X_OBSOLETE_BUTTONS, self.Y_OBSOLETE_BUTTONS, self.W_OBSOLETE_BUTTONS, self.H_OBSOLETE_BUTTONS, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		info = self._getTriggerInfo()
		if not info:
			return

		rowListName = self.top.getNextWidgetName()
		multiListX = self.X_OBSOLETE_BUTTONS + MULTI_LIST_PANEL_OFFSET_X
		multiListY = self.Y_OBSOLETE_BUTTONS + MULTI_LIST_PANEL_OFFSET_Y
		multiListW = self.W_OBSOLETE_BUTTONS + MULTI_LIST_PANEL_ADDITIONAL_W
		multiListH = self.H_OBSOLETE_BUTTONS + MULTI_LIST_PANEL_ADDITIONAL_H
		screen.addMultiListControlGFC(rowListName, "", multiListX, multiListY, multiListW, multiListH, SEVOPEDIA_MULTILIST_NUM_LISTS_AUTO_CALCULATE, MULTILIST_BUTTON_SIZE, MULTILIST_BUTTON_SIZE, TableStyles.TABLE_STYLE_STANDARD)

		# <!-- custom: obsolete techs are alternatives: any listed tech blocks the trigger.
		# Use connector numTxt between buttons instead of labeling each icon as "or", so this reads as "A or B" and matches Requires OR prereqs. (GPT-5.5) -->
		iButtonIndex = 0
		maxButtonsPerRow = get_multilist_max_buttons_per_row(multiListW, MULTILIST_BUTTON_SIZE)
		szOrLabel = localText.getText("TXT_KEY_OR", ())
		listTechs = []
		seen = {}
		for i in range(info.getNumObsoleteTechs()):
			iTech = info.getObsoleteTech(i)
			if iTech < 0 or iTech in seen:
				continue
			seen[iTech] = True
			listTechs.append(iTech)

		for iTech in listTechs:
			techInfo = gc.getTechInfo(iTech)
			if techInfo:
				add_multilist_connector_numTxt_before_button(multiListX, multiListY, iButtonIndex, MULTILIST_BUTTON_SIZE, maxButtonsPerRow, szOrLabel, screen, self.top, WidgetTypes.WIDGET_GENERAL, CvUtil.FONT_CENTER_JUSTIFY)
				screen.appendMultiListButton(rowListName, techInfo.getButton(), SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, 1, False)
				iButtonIndex += 1

		if iButtonIndex == 0:
			szText = localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_NONE", ())
			yCenter = self.Y_OBSOLETE_BUTTONS + (self.H_OBSOLETE_BUTTONS / 2)
			screen.addMultilineText(self.top.getNextWidgetName(), SASTextScale.labelText(szText), self.X_OBSOLETE_BUTTONS + 7, yCenter, self.W_OBSOLETE_BUTTONS - 14, self.H_OBSOLETE_BUTTONS - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def handleInput(self, inputClass):
		return 0
