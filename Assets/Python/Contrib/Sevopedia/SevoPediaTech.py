# Sid Meier's Civilization 4
# Copyright Firaxis Games 2005
#
#
# Sevopedia 2.3
#   sevotastic.blogspot.com
#   sevotastic@yahoo.com
#
# additional work by Gaurav, Progor, Ket, Vovan, Fitchn, LunarMongoose
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
#
# <!-- custom: uses new buildBTradeString function in CvGameTextMgr.cpp to display in placeSpecial the this technology "Cannot be traded" bullet point, see modding ressources readme at /_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md (or whichever path it may be if changed path or modifications i did or may have done additionally or kept as is).
#

from CvPythonExtensions import *
from SASMagicNumbers import *
import CvUtil
import CvPediaScreen
import ScreenInput
import SevoScreenEnums
import SASTextScale
from SASFontUtils import sasFontTagLabel

from _sevopedia_helpers import *

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

IS_SHOW_OBSOLETES_RED_X = (gc.getDefineINT("SAS_SEVOPEDIA_TECH_SHOW_OBSOLETES_RED_X") > 0)
TECH_STATS_FIXED_LEFT_ICON_COLS = 20
TECH_STATS_FIXED_RIGHT_ICON_COLS = 7
TECH_STATS_RIGHT_ICON_COL_W = INCHART_ICON_SIZE + INCHART_ICON_SPACING
TECH_STATS_RIGHT_SCROLLBAR_TAIL_W = 18
TECH_STATS_MOVE_LEFT_TO_RIGHT_W = 60
TECH_STATS_MOVE_TOPLEFT_TO_BOTTOMLEFT_H = 2

# <!-- custom: Module-level cache for tech statistics.
# Computed once on first Techs category click.
# Similar pattern to SevoPediaTrait's TRAIT_STATISTICS_CACHE. (Claude Opus 4.5) -->
# startingTechData: list of (techId, civCount, [civIds]) - all starting techs sorted by civ count
# startingTechCombos: list of (tech1, tech2, count, [civIds]) - all starting tech combinations globally
# untradeableTechsByEra: dict of eraId -> (techCount, [techIds]) - untradeable techs grouped by era
# totalTechsByEra: dict of eraId -> totalTechCount - all techs per era (for "All" column)
TECH_STATISTICS_CACHE = None

def precomputeTechStatisticsCache():
	# Precompute and cache tech statistics data. Called once from SevoPediaMain.placeTechs()
	# on first Techs category click. Returns the cache dict.
	global TECH_STATISTICS_CACHE

	if TECH_STATISTICS_CACHE is not None:
		return TECH_STATISTICS_CACHE

	numTechs = gc.getNumTechInfos()
	numCivs = gc.getNumCivilizationInfos()
	numEras = gc.getNumEraInfos()

	# Build techId -> [civIds] mapping for starting techs
	startingTechToCivs = {}
	for iTech in xrange(numTechs):
		startingTechToCivs[iTech] = []

	# Build civId -> [techIds] mapping for playable civs
	civToStartingTechs = {}
	playableCivIds = []
	for iCiv in xrange(numCivs):
		civ = gc.getCivilizationInfo(iCiv)
		if not (civ.isPlayable() or civ.isAIPlayable()):
			continue
		playableCivIds.append(iCiv)
		civTechs = []
		for iTech in xrange(numTechs):
			if civ.isCivilizationFreeTechs(iTech):
				civTechs.append(iTech)
				startingTechToCivs[iTech].append(iCiv)
		civToStartingTechs[iCiv] = sorted(civTechs)

	# Build starting tech data for top-left table (all techs that are starting techs)
	startingTechData = []
	for iTech in xrange(numTechs):
		civIds = startingTechToCivs[iTech]
		if civIds:
			startingTechData.append((iTech, len(civIds), sorted(civIds)))

	# Sort by civ count descending, then by tech name
	startingTechData.sort(key=lambda x: (-x[1], gc.getTechInfo(x[0]).getDescription()))

	# Calculate max civs for top-left table column sizing
	startingTechMaxCivs = 0
	if startingTechData:
		startingTechMaxCivs = max([d[1] for d in startingTechData])

	# Build starting tech combinations (for right table - all pairs globally)
	pairToCivs = {}
	for iCiv, techs in civToStartingTechs.items():
		if len(techs) < 2:
			continue
		for i in xrange(len(techs)):
			for j in xrange(i + 1, len(techs)):
				pair = (techs[i], techs[j])
				if pair not in pairToCivs:
					pairToCivs[pair] = []
				pairToCivs[pair].append(iCiv)

	# Build sorted all-pairs data (startingTechCombos)
	startingTechCombos = []
	def ordered_pair_for_display(t1, t2):
		name1 = gc.getTechInfo(t1).getDescription() or ""
		name2 = gc.getTechInfo(t2).getDescription() or ""
		if name2 < name1:
			return t2, t1
		return t1, t2
	for (t1, t2), civIds in pairToCivs.items():
		o1, o2 = ordered_pair_for_display(t1, t2)
		startingTechCombos.append((o1, o2, len(civIds), sorted(civIds)))

	# Sort by count descending, then by tech names
	def combo_sort_key(entry):
		t1, t2, count, civIds = entry
		name1 = gc.getTechInfo(t1).getDescription()
		name2 = gc.getTechInfo(t2).getDescription()
		return (-count, name1, name2)
	startingTechCombos.sort(key=combo_sort_key)

	# Calculate max civs for ranking bar normalization
	combosMaxCivs = 0
	if startingTechCombos:
		combosMaxCivs = max([len(d[3]) for d in startingTechCombos])
	combosMinMax = (0, 0)
	if startingTechCombos:
		counts = [d[2] for d in startingTechCombos]
		combosMinMax = (min(counts), max(counts))

	# Build untradeable techs grouped by era: eraId -> (techCount, [techIds])
	# Also count total techs per era for the "All" column
	untradeableTechsByEra = {}
	totalTechsByEra = {}
	for iEra in xrange(numEras):
		untradeableTechsByEra[iEra] = []
		totalTechsByEra[iEra] = 0

	for iTech in xrange(numTechs):
		techInfo = gc.getTechInfo(iTech)
		iEra = techInfo.getEra()
		if iEra >= 0 and iEra < numEras:
			totalTechsByEra[iEra] += 1
			if not techInfo.isTrade():
				untradeableTechsByEra[iEra].append(iTech)

	# Sort each era's techs by name and convert to (count, techIds) tuple
	untradeableTechsByEraFinal = {}
	maxUntradeableTechs = 0
	for iEra in xrange(numEras):
		techIds = untradeableTechsByEra[iEra]
		techIds.sort(key=lambda t: gc.getTechInfo(t).getDescription())
		untradeableTechsByEraFinal[iEra] = (len(techIds), techIds)
		if len(techIds) > maxUntradeableTechs:
			maxUntradeableTechs = len(techIds)

	TECH_STATISTICS_CACHE = {
		"startingTechData": tuple(startingTechData),
		"startingTechMaxCivs": startingTechMaxCivs,
		"startingTechCombos": tuple(startingTechCombos),
		"combosMinMax": combosMinMax,
		"combosMaxCivs": combosMaxCivs,
		"untradeableTechsByEra": untradeableTechsByEraFinal,
		"maxUntradeableTechs": maxUntradeableTechs,
		"totalTechsByEra": totalTechsByEra,
	}

	print("Sevopedia Tech statistics cache prebuilt. This should appear only once per gaming session.")
	return TECH_STATISTICS_CACHE

class SevoPediaTech(CvPediaScreen.CvPediaScreen):

	def __init__(self, main):
		self.iTech = -1
		self.bHistoryExpanded = False
		self.top = main

		# <!-- custom: SevopediaTech layout rework (starting-tech stats tables)
		# Row 1: Tech Pane (left) | Requires | Leads To
		# Row 2: First to D (one-button) | Obsoletes (wide) | Music (one-button)
		# Row 3: Enables (full width)
		# Row 4: Starting tech statistics (two tables: techs + combinations)
		# Row 5: Special Abilities | Background
		# (GPT-5.2 Thinking)
		# -->

		self.X_TECH_PANE = self.top.X_PEDIA_PAGE
		self.Y_TECH_PANE = self.top.Y_PEDIA_PAGE
		self.W_TECH_PANE = 380
		self.H_TECH_PANE = 116

		self.W_ICON = 100
		self.H_ICON = 100
		self.X_ICON = self.X_TECH_PANE + (self.H_TECH_PANE - self.H_ICON) / 2
		self.Y_ICON = self.Y_TECH_PANE + (self.H_TECH_PANE - self.H_ICON) / 2

		# Row 1 right region
		self.Y_REQUIRES = self.Y_TECH_PANE
		self.H_REQUIRES = self.H_TECH_PANE
		self.Y_LEADS_TO = self.Y_TECH_PANE
		self.H_LEADS_TO = self.H_TECH_PANE

		self.W_LEADS_TO = get_panel_width_for_buttons(4, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.X_LEADS_TO = self.top.R_PEDIA_PAGE - self.W_LEADS_TO
		self.X_REQUIRES = self.X_TECH_PANE + self.W_TECH_PANE + MEDIUM_MARGIN
		self.W_REQUIRES = self.X_LEADS_TO - MEDIUM_MARGIN - self.X_REQUIRES
		if self.W_REQUIRES < 50:
			# Safety fallback if the page is too narrow.
			self.W_REQUIRES = 50

		# Row 2: First to D | Tradeable | Obsoletes | Music
		self.H_ROW = NON_MULTILIST_PANEL_STANDARD_HEIGHT
		self.Y_FIRST_TO_DISCOVER = self.Y_TECH_PANE + self.H_TECH_PANE + SMALL_MARGIN
		self.X_FIRST_TO_DISCOVER = self.X_TECH_PANE

		self.W_FIRST_TO_DISCOVER = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.H_FIRST_TO_DISCOVER = self.H_ROW

		# <!-- custom: Tradeable panel showing if tech can be traded (Claude Opus 4.5) -->
		self.X_TRADEABLE = self.X_FIRST_TO_DISCOVER + self.W_FIRST_TO_DISCOVER + MEDIUM_MARGIN
		self.Y_TRADEABLE = self.Y_FIRST_TO_DISCOVER
		self.W_TRADEABLE = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.H_TRADEABLE = self.H_ROW

		self.Y_MUSIC = self.Y_FIRST_TO_DISCOVER
		self.W_MUSIC = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		self.H_MUSIC = self.H_ROW
		self.X_MUSIC = self.top.R_PEDIA_PAGE - self.W_MUSIC
		self.playButtonPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_PLAY_BUTTON").getPath()
		self.noEntryButtonPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_NO_ENTRY").getPath()

		self.Y_OBSOLETES = self.Y_FIRST_TO_DISCOVER
		self.X_OBSOLETES = self.X_TRADEABLE + self.W_TRADEABLE + MEDIUM_MARGIN
		self.W_OBSOLETES = self.X_MUSIC - MEDIUM_MARGIN - self.X_OBSOLETES
		self.H_OBSOLETES = self.H_ROW

		# <!-- custom: note: Now that we switched to the thinner ChatGPT 5.2 based model, 64 is a bit too small, so extending it to fit buttons -->
		# self.RED_X_BUTTON_SIZE = 64
		self.RED_X_BUTTON_SIZE = 72

		# Row 3: Enables (full width)
		self.X_ENABLES = self.X_TECH_PANE
		self.Y_ENABLES = self.Y_FIRST_TO_DISCOVER + self.H_ROW + SMALL_MARGIN
		self.W_ENABLES = self.top.R_PEDIA_PAGE - self.X_ENABLES
		self.H_ENABLES = self.H_ROW

		# Row 4: Starting tech statistics tables (2 left stacked, 1 right)
		# <!-- custom: Statistics panel - blue panels without headers, tables inside.
		# Left side wider (~200px more than right). (Claude Opus 4.5) -->
		self.X_STATS = self.X_TECH_PANE
		self.Y_STATS = self.Y_ENABLES + self.H_ENABLES + SMALL_MARGIN
		self.W_STATS = self.top.R_PEDIA_PAGE - self.X_STATS
		# Stats height - slightly increased to avoid bottom clipping in stats tables.
		self.H_STATS = 384

		# Stats sub-panels layout - blue panels without headers, margin separation
		STATS_MARGIN = 8
		STATS_INNER = 6
		self.STATS_ROW_H = 24
		self.STATS_MARGIN = STATS_MARGIN
		self.STATS_INNER = STATS_INNER

		# Left side deterministic split with explicit transfer to the right chart.
		self.STATS_LEFT_W = int(self.W_STATS * 0.60) - TECH_STATS_MOVE_LEFT_TO_RIGHT_W
		self.STATS_LEFT_X = self.X_STATS + STATS_MARGIN
		self.STATS_LEFT_TOP_Y = self.Y_STATS + STATS_MARGIN
		# Top-left reduced, bottom-left gets more room (0.44 ratio gives ~1-2px to bottom)
		self.STATS_LEFT_TOP_H = int((self.H_STATS - 3 * STATS_MARGIN) * 0.44) - TECH_STATS_MOVE_TOPLEFT_TO_BOTTOMLEFT_H
		self.STATS_LEFT_BOTTOM_Y = self.STATS_LEFT_TOP_Y + self.STATS_LEFT_TOP_H + STATS_MARGIN
		self.STATS_LEFT_BOTTOM_H = self.H_STATS - 2 * STATS_MARGIN - self.STATS_LEFT_TOP_H - STATS_MARGIN

		# Right side (narrower - remaining width)
		self.STATS_RIGHT_X = self.STATS_LEFT_X + self.STATS_LEFT_W + STATS_MARGIN
		self.STATS_RIGHT_Y = self.Y_STATS + STATS_MARGIN
		self.STATS_RIGHT_W = self.W_STATS - self.STATS_LEFT_W - 3 * STATS_MARGIN
		self.STATS_RIGHT_H = self.H_STATS - 2 * STATS_MARGIN

		# <!-- custom: Button sizes now use centralized INCHART_* constants from _sevopedia_helpers.
		# CIV_ICON_SIZE, TECH_ICON_SIZE, CIV_BUTTON_COLUMN_SPACING replaced by INCHART_ICON_SIZE, INCHART_ICON_SPACING -->

		# Bottom row: Special / History (reduced height by 120px)
		self.X_SPECIAL = self.X_TECH_PANE
		self.Y_SPECIAL = self.Y_STATS + self.H_STATS + SMALL_MARGIN
		self.W_SPECIAL = self.W_TECH_PANE
		self.H_SPECIAL = self.top.B_PEDIA_PAGE - self.Y_SPECIAL

		self.X_HISTORY = self.X_SPECIAL + self.W_SPECIAL + MEDIUM_MARGIN
		self.Y_HISTORY = self.Y_SPECIAL
		self.W_HISTORY = self.top.R_PEDIA_PAGE - self.X_HISTORY
		self.H_HISTORY = self.H_SPECIAL

		# <!-- custom: cached terrain define for trade-route icon selection. (Claude code Sonnet 4.6) -->
		self.iDEEP_WATER_TERRAIN = None

	def interfaceScreen(self, iTech):
		if self.iTech != iTech:
			self.bHistoryExpanded = False
		self.iTech = iTech

		self.placeTechPane()
		self.placePrereqs()
		self.placeLeadsTo()
		self.placeFirstToDiscover()
		self.placeTradeable()
		self.placeObsoletes()
		self.placeMusic()
		self.placeEnables()
		self.placeStartingTechStatistics()

		self.placeSpecial()
		self.placeHistory()

	def placeTechPane(self):
		screen = self.top.getScreen()
		techInfo = gc.getTechInfo(self.iTech)

		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_TECH_PANE, self.Y_TECH_PANE, self.W_TECH_PANE, self.H_TECH_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		# <!-- custom: was PanelStyles.PANEL_STYLE_MAIN -->
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_EMPTY)
		screen.addDDSGFC(self.top.getNextWidgetName(), techInfo.getButton(), self.X_ICON + self.W_ICON/2 - PANE_ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - PANE_ICON_SIZE/2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1)

		listBoxName = self.top.getNextWidgetName()
		szEra = gc.getEraInfo(techInfo.getEra()).getDescription() + " " + localText.getText("TXT_KEY_PEDIA_ERA", ())

		techCost = techInfo.getResearchCost()
		if (self.top.iActivePlayer != -1):
			techCost = gc.getTeam(gc.getGame().getActiveTeam()).getResearchCost(self.iTech)
		szCostText = u"%c %s" % (gc.getCommerceInfo(CommerceTypes.COMMERCE_RESEARCH).getChar(), localText.getText("TXT_KEY_PEDIA_COST_CUSTOM", (techCost,)))

		screen.addListBoxGFC(listBoxName, "", self.X_TECH_PANE + 92, self.Y_TECH_PANE + 14, self.W_TECH_PANE, self.H_TECH_PANE, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(listBoxName, False)
		# <!-- custom: extra space (" ") in some of these listboxstringsto better align with the research icon char starting more on the right, depending on where the space is put, the text is so much better left-aligned between rows i think/feel/see or so it seems to me if i mmay say... -->
		screen.appendListBoxString(listBoxName, SASTextScale.titleText(u" " + techInfo.getDescription()), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)
		screen.appendListBoxString(listBoxName, SASTextScale.labelText(u" " + szEra), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)
		screen.appendListBoxString(listBoxName, SASTextScale.titleText(szCostText), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

	def placeMusic(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_MUSIC_PANEL", ()), "", False, True, self.X_MUSIC, self.Y_MUSIC, self.W_MUSIC, self.H_MUSIC, PanelStyles.PANEL_STYLE_BLUE50)

		# <!-- custom: use attachLabel for padding similar to other 84px panels -->
		screen.attachLabel(panelName, "", "  ")

		# <!-- custom: Music system uses packed keys (unlike Movies which use separate type+id). (Claude code Sonnet 4.5) -->
		iMusicType = SAS_MAGIC_PEDIA_MUSIC_TYPE_TECH
		iPackedMusic = self.top.SAS_packMusicKey(iMusicType, self.iTech)

		if self.top.pediaMusic.hasMusic(iPackedMusic):
			buttonSize = 64
			# <!-- custom: setImageButtonAt positions relative to panel content area (below header).
			# X: Standard centering works correctly.
			# Y: Must be set to 10 (not calculated from panelHeaderHeight) - empirically determined positioning fix. (Claude code Sonnet 4.5) -->
			buttonX = (self.W_MUSIC - buttonSize) / 2
			buttonY = 10
			screen.setImageButtonAt(self.top.getNextWidgetName(), panelName, self.playButtonPath, buttonX, buttonY, buttonSize, buttonSize, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_MUSIC_ENTRY, iPackedMusic)
		else:
			draw_none_text(screen, self.top, self.X_MUSIC, self.Y_MUSIC, self.W_MUSIC, self.H_MUSIC)

	def placeCivilizationsThatStartWithThisTech(self):
		# <advc.004y> Show the box only for starting techs
		civs = []
		for iCiv in range(gc.getNumCivilizationInfos()):
			civ = gc.getCivilizationInfo(iCiv)
			if (civ.isCivilizationFreeTechs(self.iTech) and (civ.isPlayable() or civ.isAIPlayable())): # Exclude Minor civ
				civs.append(iCiv)
		if len(civs) <= 0:
			return
		# </advc.004y>
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_CIVILIZATIONS_THAT_START_WITH_THIS_TECH", ()), "", False, True, self.X_CIVILIZATIONS_THAT_START_WITH_THIS_TECH, self.Y_CIVILIZATIONS_THAT_START_WITH_THIS_TECH, self.W_CIVILIZATIONS_THAT_START_WITH_THIS_TECH, self.H_CIVILIZATIONS_THAT_START_WITH_THIS_TECH, PanelStyles.PANEL_STYLE_BLUE50 )
		screen.attachLabel(panelName, "", "  ")
		for iCiv in civs: # advc.004y: Use the list computed above
			civ = gc.getCivilizationInfo(iCiv)
			#if civ.isCivilizationFreeTechs(self.iTech):
			screen.attachImageButton(panelName, "", civ.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIV, iCiv, 1, False)

	# <!-- custom: First to Discover panel showing religions, corporations, great people, and free techs that can be gained by being first to discover this tech (Claude code Opus 4.5 + GPT-5.2-Codex) -->
	def placeFirstToDiscover(self):
		screen = self.top.getScreen()
		techInfo = gc.getTechInfo(self.iTech)
		iActivePlayer = gc.getGame().getActivePlayer()

		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_FIRST_TO_DISCOVER", ()), "", False, True, self.X_FIRST_TO_DISCOVER, self.Y_FIRST_TO_DISCOVER, self.W_FIRST_TO_DISCOVER, self.H_FIRST_TO_DISCOVER, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		bButtonFound = False

		# Religions founded by first discoverer
		for iReligion in range(gc.getNumReligionInfos()):
			religionInfo = gc.getReligionInfo(iReligion)
			if religionInfo.getTechPrereq() == self.iTech:
				screen.attachImageButton(panelName, "", religionInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION, iReligion, 1, False)
				bButtonFound = True

		# Free unit (great person) for first discoverer
		iFirstFreeUnitClass = techInfo.getFirstFreeUnitClass()
		if iFirstFreeUnitClass != -1:
			# Get civ-specific unit if active player exists
			if iActivePlayer >= 0:
				iUnit = gc.getCivilizationInfo(gc.getGame().getActiveCivilizationType()).getCivilizationUnits(iFirstFreeUnitClass)
			else:
				iUnit = gc.getUnitClassInfo(iFirstFreeUnitClass).getDefaultUnitIndex()
			if iUnit != -1:
				unitInfo = gc.getUnitInfo(iUnit)
				szButton = unitInfo.getButton()
				if iActivePlayer >= 0:
					szButton = gc.getPlayer(iActivePlayer).getUnitButton(iUnit)
				screen.attachImageButton(panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False)
				bButtonFound = True

		# Free tech for first discoverer
		if techInfo.getFirstFreeTechs() > 0:
			szButton = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_FREETECH").getPath()
			# <!-- custom: similarly, as of now upscale the smaller button to 46 px not more -->
			screen.attachImageButton(panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_FREE_TECH, self.iTech, -1, False)
			bButtonFound = True

		if not bButtonFound:
			# No first-to-discover effects - display "None" text
			draw_none_text(screen, self.top, self.X_FIRST_TO_DISCOVER, self.Y_FIRST_TO_DISCOVER, self.W_FIRST_TO_DISCOVER, self.H_FIRST_TO_DISCOVER)

	# <!-- custom: Tradeable panel showing if this tech can be traded.
	# Shows "Yes" if tradeable, or No Entry emoji icon if not tradeable. (Claude Opus 4.5) -->
	def placeTradeable(self):
		screen = self.top.getScreen()
		techInfo = gc.getTechInfo(self.iTech)

		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_TRADEABLE_PANEL", ()), "", False, True, self.X_TRADEABLE, self.Y_TRADEABLE, self.W_TRADEABLE, self.H_TRADEABLE, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		if techInfo.isTrade():
			# Tech is tradeable - show "Yes" text
			txtName = self.top.getNextWidgetName()
			szText = localText.getText("TXT_KEY_PEDIA_SAS_YES", ())
			yPanelCenter = self.Y_TRADEABLE + (self.H_TRADEABLE / 2)
			screen.addMultilineText(txtName, SASTextScale.labelText(szText), self.X_TRADEABLE + 7, yPanelCenter, self.W_TRADEABLE - 14, self.H_TRADEABLE - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		else:
			# Tech is not tradeable - show emoji icon
			buttonSize = 64
			buttonX = (self.W_TRADEABLE - buttonSize) / 2
			buttonY = 10
			screen.setImageButtonAt(self.top.getNextWidgetName(), panelName, self.noEntryButtonPath, buttonX, buttonY, buttonSize, buttonSize, WidgetTypes.WIDGET_GENERAL, -1, -1)

	# <!-- custom: new obsoletes panel showing buildings, bonuses, special buildings, and units obsoleted by this tech with red X overlay (Claude code Sonnet 4.5) -->
	def placeObsoletes(self):
		screen = self.top.getScreen()

		# Collect all obsolete items for this tech
		obsoleteBuildings = []
		obsoleteBonuses = []
		obsoleteSpecialBuildings = []
		obsoleteUnits = []

		# Obsolete Buildings (iterate by building class to get civ-specific buildings)
		for iBuildingClass in range(gc.getNumBuildingClassInfos()):
			iBuilding = gc.getBuildingClassInfo(iBuildingClass).getDefaultBuildingIndex()
			if iBuilding != -1:
				buildingInfo = gc.getBuildingInfo(iBuilding)
				if buildingInfo.getObsoleteTech() == self.iTech:
					obsoleteBuildings.append(iBuilding)

		# Obsolete Bonuses
		for iBonus in range(gc.getNumBonusInfos()):
			bonusInfo = gc.getBonusInfo(iBonus)
			if bonusInfo.getTechObsolete() == self.iTech:
				obsoleteBonuses.append(iBonus)

		# Obsolete Special Buildings
		for iSpecialBuilding in range(gc.getNumSpecialBuildingInfos()):
			specialBuildingInfo = gc.getSpecialBuildingInfo(iSpecialBuilding)
			if specialBuildingInfo.getObsoleteTech() == self.iTech:
				obsoleteSpecialBuildings.append(iSpecialBuilding)

		# Obsolete Units - check that getObsoleteTech is exposed in DLL
		unitInfo = gc.getUnitInfo(0)
		if not hasattr(unitInfo, "getObsoleteTech"):
			raise RuntimeError("[FATAL] Your mod DLL does not expose the required CvUnitInfo Python getter: getObsoleteTech. Please expose it in CyInfoInterface1.cpp and rebuild the DLL.")
		for iUnit in range(gc.getNumUnitInfos()):
			unitInfo = gc.getUnitInfo(iUnit)
			if unitInfo.getObsoleteTech() == self.iTech:
				obsoleteUnits.append(iUnit)

		# Always show panel, display "None" if no obsolete items
		totalObsoleteCount = len(obsoleteBuildings) + len(obsoleteBonuses) + len(obsoleteSpecialBuildings) + len(obsoleteUnits)
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_OBSOLETES", ()), "", False, True, self.X_OBSOLETES, self.Y_OBSOLETES, self.W_OBSOLETES, self.H_OBSOLETES, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		if totalObsoleteCount > 0:
			# Get red X overlay path for obsolete indicator
			# <!-- custom: default Civ4 one was too bold at 64 px making it hard to read; replaced with a thinner one imrpessively generated by ChatGPT 5.2 thanks a lot! -->
			# szRedX = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTONS_RED_X").getPath()
			szRedX = "Art/AdvCiv_SAS/Interface/RedX_Thin/chatgpt_5_2_obsolete_x_edge_thin_1204_preview64.dds"

			# Helper to add an obsolete item with red X overlay
			# We use addDDSGFCAt to overlay red X on top of the button
			# Since attachImageButton doesn't support overlays, we use a workaround:
			# attach the button first, then add the overlay separately using screen coordinates

			# For simplicity, we'll use a different approach: create a child panel for each item
			# Actually, the cleanest approach is to use attachImageButton and then manually
			# add the red X overlay at the same position using addDDSGFC

			# Obsolete Buildings
			# <!-- custom: use iData2=-1 instead of 1 to fix left-click not working (same pattern as SevoPediaBuilding's obsolete tech) -->
			for iBuilding in obsoleteBuildings:
				buildingInfo = gc.getBuildingInfo(iBuilding)
				screen.attachImageButton(panelName, "", buildingInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, -1, False)

			# Obsolete Bonuses
			for iBonus in obsoleteBonuses:
				bonusInfo = gc.getBonusInfo(iBonus)
				screen.attachImageButton(panelName, "", bonusInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus, -1, False)

			# Obsolete Special Buildings - show individual buildings that belong to this special building type
			for iSpecialBuilding in obsoleteSpecialBuildings:
				specialBuildingInfo = gc.getSpecialBuildingInfo(iSpecialBuilding)
				# Find all buildings with this special building type and display them
				for iBuilding in range(gc.getNumBuildingInfos()):
					buildingInfo = gc.getBuildingInfo(iBuilding)
					if buildingInfo.getSpecialBuildingType() == iSpecialBuilding:
						screen.attachImageButton(panelName, "", buildingInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, -1, False)

			# Obsolete Units
			iActivePlayer = gc.getGame().getActivePlayer()
			for iUnit in obsoleteUnits:
				unitInfo = gc.getUnitInfo(iUnit)
				szButton = unitInfo.getButton()
				if iActivePlayer >= 0:
					szButton = gc.getPlayer(iActivePlayer).getUnitButton(iUnit)
				screen.attachImageButton(panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, -1, False)

			# Now add red X overlays on top of all buttons (if enabled via SAS define)
			if IS_SHOW_OBSOLETES_RED_X:
				# Since attachImageButton positions are managed by the panel, we need to use a different approach
				# We'll add the overlays as separate DDS graphics at calculated positions
				# The panel adds a label "  " first, then buttons start after that

				# Calculate total items and add overlays
				# Adjusted values to match actual button positions in the panel
				# Note: actual buttons in panel are 64px (BUTTON_SIZE_CUSTOM), RED_X_BUTTON_SIZE is for the overlay DDS
				# <!-- custom: old values for base INTERFACE_BUTTONS_RED_X (64px, bold):
				#   iOverlaySize = 64
				#   iCurrentX = self.X_OBSOLETES + 10
				#   iOverlayY = self.Y_OBSOLETES + 36
				#   iButtonSpacing = 64 + 4
				# -->
				iButtonSize = 64  # Actual button size in panel (BUTTON_SIZE_CUSTOM)
				iOverlaySize = self.RED_X_BUTTON_SIZE  # Size of the thin red X DDS (128px to fit properly)
				iOverlayOffset = (iButtonSize - iOverlaySize) / 2  # Center overlay on button (will be negative if overlay > button)
				iCurrentX = self.X_OBSOLETES + 10 + iOverlayOffset  # Starting position with centering offset
				# <!-- custom: for some reason after switching from addDDSGFC to addDDSGFCAt to fix linking issues clicking (right click works but left click doesn't which is not consistent with how other panel's clicking works (either left only or both), the RedX has shifted vertically a bit, so we need to reduce the original + 36 added to iOverlayY -->
				iOverlayY = self.Y_OBSOLETES + 8 + iOverlayOffset  # Y position with centering offset
				iButtonSpacing = iButtonSize + 4  # Actual button spacing (64 + 4 = 68)

				# Process buildings
				for iBuilding in obsoleteBuildings:
					szOverlayName = self.top.getNextWidgetName()
					screen.addDDSGFCAt(szOverlayName, panelName, szRedX, iCurrentX - self.X_OBSOLETES, iOverlayY - self.Y_OBSOLETES, iOverlaySize, iOverlaySize, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, -1, False)
					# <!-- custom: overlay should not intercept clicks; disable hit testing so underlying obsolete button handles left/right click (GPT-5.2-Codex) -->
					screen.setHitTest(szOverlayName, HitTestTypes.HITTEST_NOHIT)
					iCurrentX += iButtonSpacing

				# Process bonuses
				for iBonus in obsoleteBonuses:
					szOverlayName = self.top.getNextWidgetName()
					screen.addDDSGFCAt(szOverlayName, panelName, szRedX, iCurrentX - self.X_OBSOLETES, iOverlayY - self.Y_OBSOLETES, iOverlaySize, iOverlaySize, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus, -1, False)
					screen.setHitTest(szOverlayName, HitTestTypes.HITTEST_NOHIT)
					iCurrentX += iButtonSpacing

				# Process special buildings (individual buildings)
				for iSpecialBuilding in obsoleteSpecialBuildings:
					for iBuilding in range(gc.getNumBuildingInfos()):
						buildingInfo = gc.getBuildingInfo(iBuilding)
						if buildingInfo.getSpecialBuildingType() == iSpecialBuilding:
							szOverlayName = self.top.getNextWidgetName()
							screen.addDDSGFCAt(szOverlayName, panelName, szRedX, iCurrentX - self.X_OBSOLETES, iOverlayY - self.Y_OBSOLETES, iOverlaySize, iOverlaySize, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, -1, False)
							screen.setHitTest(szOverlayName, HitTestTypes.HITTEST_NOHIT)
							iCurrentX += iButtonSpacing

				# Process units
				for iUnit in obsoleteUnits:
					szOverlayName = self.top.getNextWidgetName()
					screen.addDDSGFCAt(szOverlayName, panelName, szRedX, iCurrentX - self.X_OBSOLETES, iOverlayY - self.Y_OBSOLETES, iOverlaySize, iOverlaySize, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, -1, False)
					screen.setHitTest(szOverlayName, HitTestTypes.HITTEST_NOHIT)
					iCurrentX += iButtonSpacing

		else:
			# No obsolete items - display "None" text
			draw_none_text(screen, self.top, self.X_OBSOLETES, self.Y_OBSOLETES, self.W_OBSOLETES, self.H_OBSOLETES)

	def placeLeadsTo(self):
		screen = self.top.getScreen()
		szLeadsTo = localText.getText("TXT_KEY_PEDIA_LEADS_TO", ())
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, szLeadsTo, "", False, True, self.X_LEADS_TO, self.Y_LEADS_TO, self.W_LEADS_TO, self.H_LEADS_TO, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")
		bButtonFound = False

		for j in range(gc.getNumTechInfos()):
			for k in range(gc.getNUM_OR_TECH_PREREQS()):
				iPrereq = gc.getTechInfo(j).getPrereqOrTechs(k)
				if (iPrereq == self.iTech):
					screen.attachImageButton(panelName, "", gc.getTechInfo(j).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_DERIVED_TECH, j, self.iTech, False)
					bButtonFound = True
			for k in range(gc.getNUM_AND_TECH_PREREQS()):
				iPrereq = gc.getTechInfo(j).getPrereqAndTechs(k)
				if (iPrereq == self.iTech):
					screen.attachImageButton(panelName, "", gc.getTechInfo(j).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_DERIVED_TECH, j, self.iTech, False)
					bButtonFound = True

		if not bButtonFound:
			draw_none_text(screen, self.top, self.X_LEADS_TO, self.Y_LEADS_TO, self.W_LEADS_TO, self.H_LEADS_TO)

	def placePrereqs(self):
		screen = self.top.getScreen()
		szRequires = localText.getText("TXT_KEY_PEDIA_REQUIRES", ())
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, szRequires, "", False, True, self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")
		bButtonFound = False
		bHasAnd = False
		for j in range(gc.getNUM_AND_TECH_PREREQS()):
			eTech = gc.getTechInfo(self.iTech).getPrereqAndTechs(j)
			if (eTech > -1):
				if bHasAnd:
					screen.attachLabel(panelName, "", SASTextScale.labelText(localText.getText("TXT_KEY_AND", ())))
				else:
					bHasAnd = True
				screen.attachImageButton(panelName, "", gc.getTechInfo(eTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_REQUIRED_TECH, eTech, j, False)
				bButtonFound = True
		nOrTechs = 0
		for j in range(gc.getNUM_OR_TECH_PREREQS()):
			if (gc.getTechInfo(self.iTech).getPrereqOrTechs(j) > -1):
				nOrTechs += 1
		szLeftDelimeter = ""
		szRightDelimeter = ""
		if bHasAnd:
			if (nOrTechs > 1):
				szLeftDelimeter = localText.getText("TXT_KEY_AND", ()) + "("
				szRightDelimeter = ") "
			elif (nOrTechs > 0):
				szLeftDelimeter = localText.getText("TXT_KEY_AND", ())
		if len(szLeftDelimeter) > 0:
			screen.attachLabel(panelName, "", SASTextScale.labelText(szLeftDelimeter))
		bFirst = True
		for j in range(gc.getNUM_OR_TECH_PREREQS()):
			eTech = gc.getTechInfo(self.iTech).getPrereqOrTechs(j)
			if (eTech > -1):
				if (not bFirst):
					screen.attachLabel(panelName, "", SASTextScale.labelText(localText.getText("TXT_KEY_OR", ())))
				else:
					bFirst = False
				screen.attachImageButton(panelName, "", gc.getTechInfo(eTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_REQUIRED_TECH, eTech, j, False)
				bButtonFound = True
		if len(szRightDelimeter) > 0:
			screen.attachLabel(panelName, "", SASTextScale.labelText(szRightDelimeter))

		if not bButtonFound:
			draw_none_text(screen, self.top, self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES)

	# <!-- custom: merged placeUnits and placeBuildings into single placeEnables, expanded to include all tech advisor items (Claude code Opus 4.5 + GPT-5.2-Codex) -->
	def placeEnables(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_ENABLES", ()), "", False, True, self.X_ENABLES, self.Y_ENABLES, self.W_ENABLES, self.H_ENABLES, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		iActivePlayer = gc.getGame().getActivePlayer()
		techInfo = gc.getTechInfo(self.iTech)
		bButtonFound = False

		# Units enabled
		for eLoopUnit in range(gc.getNumUnitInfos()):
			if eLoopUnit != -1:
				if isTechRequiredForUnit(self.iTech, eLoopUnit):
					szButton = gc.getUnitInfo(eLoopUnit).getButton()
					if iActivePlayer >= 0:
						szButton = gc.getPlayer(iActivePlayer).getUnitButton(eLoopUnit)
					screen.attachImageButton(panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, eLoopUnit, 1, False)
					bButtonFound = True

		# Buildings enabled
		for eLoopBuilding in range(gc.getNumBuildingInfos()):
			if eLoopBuilding != -1:
				if isTechRequiredForBuilding(self.iTech, eLoopBuilding):
					screen.attachImageButton(panelName, "", gc.getBuildingInfo(eLoopBuilding).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, eLoopBuilding, 1, False)
					bButtonFound = True

		# Projects enabled
		for eLoopProject in range(gc.getNumProjectInfos()):
			if isTechRequiredForProject(self.iTech, eLoopProject):
				screen.attachImageButton(panelName, "", gc.getProjectInfo(eLoopProject).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROJECT, eLoopProject, 1, False)
				bButtonFound = True

		# Promotions enabled
		for iPromotion in range(gc.getNumPromotionInfos()):
			if gc.getPromotionInfo(iPromotion).getTechPrereq() == self.iTech:
				screen.attachImageButton(panelName, "", gc.getPromotionInfo(iPromotion).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, iPromotion, 1, False)
				bButtonFound = True

		# Civics enabled
		for iCivic in range(gc.getNumCivicInfos()):
			if gc.getCivicInfo(iCivic).getTechPrereq() == self.iTech:
				screen.attachImageButton(panelName, "", gc.getCivicInfo(iCivic).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, iCivic, 1, False)
				bButtonFound = True

		# Bonuses revealed
		for iBonus in range(gc.getNumBonusInfos()):
			if gc.getBonusInfo(iBonus).getTechReveal() == self.iTech:
				screen.attachImageButton(panelName, "", gc.getBonusInfo(iBonus).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus, 1, False)
				bButtonFound = True

		# Builds/Improvements enabled (includes feature-removal builds like Chop Forest, roads, etc.)
		for iBuild in range(gc.getNumBuildInfos()):
			buildInfo = gc.getBuildInfo(iBuild)
			bTechFound = False
			# Check if this build's tech prereq matches
			if buildInfo.getTechPrereq() == -1:
				# No direct tech prereq - check feature-specific techs (e.g. Chop Jungle needs specific tech)
				for iFeature in range(gc.getNumFeatureInfos()):
					if buildInfo.getFeatureTech(iFeature) == self.iTech:
						bTechFound = True
						break
			else:
				if buildInfo.getTechPrereq() == self.iTech:
					bTechFound = True

			if bTechFound:
				iImprovement = buildInfo.getImprovement()
				if iImprovement != -1:
					screen.attachImageButton(panelName, "", buildInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_IMPROVEMENT, iImprovement, 1, False)
				else:
					# Use WIDGET_HELP_IMPROVEMENT to keep the feature-removal tooltip/redirect behavior (DLL change).
					screen.attachImageButton(panelName, "", buildInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_HELP_IMPROVEMENT, self.iTech, iBuild, False)
				bButtonFound = True

		# Special buildings like monasteries
		for iSpecialBuilding in range(gc.getNumSpecialBuildingInfos()):
			if gc.getSpecialBuildingInfo(iSpecialBuilding).getTechPrereq() == self.iTech:
				screen.attachImageButton(panelName, "", gc.getSpecialBuildingInfo(iSpecialBuilding).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_HELP_SPECIAL_BUILDING, self.iTech, iSpecialBuilding, False)
				bButtonFound = True

		# Route movement change (faster roads)
		# <!-- custom: use BUTTON_SIZE_46 for interface art buttons - this is the largest size the engine supports for attachImageButton;
		# unintended but beneficial side effect: the smaller size helps visually distinguish these "ability" icons from the 64px game object buttons above (Claude code Opus 4.5) -->
		for iRoute in range(gc.getNumRouteInfos()):
			if gc.getRouteInfo(iRoute).getTechMovementChange(self.iTech) != 0:
				screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_MOVE_BONUS").getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_MOVE_BONUS, self.iTech, -1, False)
				bButtonFound = True
				break  # Only show once even if multiple routes affected

		# Bridge Building
		if techInfo.isBridgeBuilding():
			screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_BRIDGEBUILDING").getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_BUILD_BRIDGE, self.iTech, -1, False)
			bButtonFound = True

		# Irrigation
		if techInfo.isIrrigation():
			screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_IRRIGATION").getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_IRRIGATION, self.iTech, -1, False)
			bButtonFound = True

		# Ignore Irrigation (farms spread without fresh water)
		if techInfo.isIgnoreIrrigation():
			screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_NOIRRIGATION").getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_IGNORE_IRRIGATION, self.iTech, -1, False)
			bButtonFound = True

		# Water Work (coastal work)
		if techInfo.isWaterWork():
			screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_WATERWORK").getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_WATER_WORK, self.iTech, -1, False)
			bButtonFound = True

		# Domain Extra Moves (e.g. extra naval movement)
		for iDomain in range(DomainTypes.NUM_DOMAIN_TYPES):
			if techInfo.getDomainExtraMoves(iDomain) != 0:
				screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_WATERMOVES").getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_DOMAIN_EXTRA_MOVES, self.iTech, iDomain, False)
				bButtonFound = True

		# Terrain trade routes (coastal/ocean trade)
		if self.iDEEP_WATER_TERRAIN is None:
			self.iDEEP_WATER_TERRAIN = gc.getDefineINT("DEEP_WATER_TERRAIN")
		for iTerrain in range(gc.getNumTerrainInfos()):
			if techInfo.isTerrainTrade(iTerrain):
				szArtInfoType = "INTERFACE_TECH_WATERTRADE"
				if iTerrain == self.iDEEP_WATER_TERRAIN:
					szArtInfoType = "INTERFACE_TECH_DEEPWATERTRADE"
				screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo(szArtInfoType).getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_TERRAIN_TRADE, self.iTech, iTerrain, False)
				bButtonFound = True

		# River trade
		if techInfo.isRiverTrade():
			screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_RIVERTRADE").getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_TERRAIN_TRADE, self.iTech, gc.getNumTerrainInfos(), False)
			bButtonFound = True

		# Commerce slider adjustments (culture, espionage sliders)
		for iCommerce in range(CommerceTypes.NUM_COMMERCE_TYPES):
			if techInfo.isCommerceFlexible(iCommerce):
				if iCommerce == CommerceTypes.COMMERCE_CULTURE:
					szFileName = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_CULTURE").getPath()
				elif iCommerce == CommerceTypes.COMMERCE_ESPIONAGE:
					szFileName = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_ESPIONAGE").getPath()
				else:
					szFileName = ArtFileMgr.getInterfaceArtInfo("INTERFACE_GENERAL_QUESTIONMARK").getPath()
				screen.attachImageButton(panelName, "", szFileName, GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_ADJUST, self.iTech, iCommerce, False)
				bButtonFound = True

		# Map Trading
		if techInfo.isMapTrading():
			screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_MAPTRADING").getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_MAP_TRADE, self.iTech, -1, False)
			bButtonFound = True

		# Tech Trading
		if techInfo.isTechTrading():
			screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_TECHTRADING").getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_TECH_TRADE, self.iTech, -1, False)
			bButtonFound = True

		# Gold Trading
		if techInfo.isGoldTrading():
			screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_GOLDTRADING").getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_GOLD_TRADE, self.iTech, -1, False)
			bButtonFound = True

		# Open Borders
		if techInfo.isOpenBordersTrading():
			screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_OPENBORDERS").getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_OPEN_BORDERS, self.iTech, -1, False)
			bButtonFound = True

		# Defensive Pact
		if techInfo.isDefensivePactTrading():
			screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_DEFENSIVEPACT").getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_DEFENSIVE_PACT, self.iTech, -1, False)
			bButtonFound = True

		# Permanent Alliance
		if techInfo.isPermanentAllianceTrading():
			screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_PERMALLIANCE").getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_PERMANENT_ALLIANCE, self.iTech, -1, False)
			bButtonFound = True

		# Vassal States
		if techInfo.isVassalStateTrading():
			screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_VASSAL").getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_VASSAL_STATE, self.iTech, -1, False)
			bButtonFound = True

		# Extra LOS (line of sight) from water
		if techInfo.isExtraWaterSeeFrom():
			screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_LOS").getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_LOS_BONUS, self.iTech, -1, False)
			bButtonFound = True

		# Map centering
		if techInfo.isMapCentering():
			screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_MAPCENTER").getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_MAP_CENTER, self.iTech, -1, False)
			bButtonFound = True

		# Map reveal
		if techInfo.isMapVisible():
			screen.attachImageButton(panelName, "", ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_MAPREVEAL").getPath(), GenericButtonSizes.BUTTON_SIZE_46, WidgetTypes.WIDGET_HELP_MAP_REVEAL, self.iTech, -1, False)
			bButtonFound = True

		if not bButtonFound:
			draw_none_text(screen, self.top, self.X_ENABLES, self.Y_ENABLES, self.W_ENABLES, self.H_ENABLES)

	# <!-- custom: Statistics panel with three tables wrapped in outer blue panel:
	# Top-Left: All starting techs with civ count and civ buttons
	# Bottom-Left: Untradeable techs grouped by era (era name, tech count, tech buttons)
	# Right: All starting tech combinations globally with count, ranking bar, and civs
	# (Claude Opus 4.5) -->
	def placeStartingTechStatistics(self):
		screen = self.top.getScreen()

		cache = TECH_STATISTICS_CACHE
		if cache is None:
			return

		# Outer background panel wrapping all 3 charts (like SevoPediaTrait)
		outerPanelName = self.top.getNextWidgetName()
		screen.addPanel(outerPanelName, "", "", True, True, self.X_STATS, self.Y_STATS, self.W_STATS, self.H_STATS, PanelStyles.PANEL_STYLE_BLUE50)

		# === TOP-LEFT TABLE: All starting techs with civ buttons ===
		self._placeStartingTechsTable(screen, cache)

		# === BOTTOM-LEFT TABLE: Untradeable techs by era ===
		self._placeUntradeableTechsByEraTable(screen, cache)

		# === RIGHT TABLE: All starting tech combinations ===
		self._placeStartingTechCombosTable(screen, cache)

	def _placeStartingTechsTable(self, screen, cache):
		# Top-left table: shows ALL starting techs with civ count and civ buttons
		# Blue panel without header, table inside
		startingTechData = cache["startingTechData"]
		_ = cache["startingTechMaxCivs"]

		# Panel dimensions
		panelX = self.STATS_LEFT_X
		panelY = self.STATS_LEFT_TOP_Y
		panelW = self.STATS_LEFT_W
		panelH = self.STATS_LEFT_TOP_H

		# Add blue panel without header (empty title)
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, "", "", True, True, panelX, panelY, panelW, panelH, PanelStyles.PANEL_STYLE_BLUE50)

		# Table dimensions inside panel
		tableX = panelX + self.STATS_INNER
		tableY = panelY + self.STATS_INNER
		tableW = panelW - 2 * self.STATS_INNER
		tableH = panelH - 2 * self.STATS_INNER

		# Column widths: Starting Tech (button + name) | Count | Civ buttons...
		# Tech column should be compact - just enough for button + long tech name
		colTechW = 160
		colCountW = 40
		maxCivs = TECH_STATS_FIXED_LEFT_ICON_COLS
		fixedColsW = colTechW + colCountW
		civColW, maxCivs = inchart_calc_icon_col_width(tableW, fixedColsW, maxCivs, INCHART_ICON_SIZE, INCHART_ICON_SPACING)

		# Create table
		tableName = self.top.getNextWidgetName()
		screen.addTableControlGFC(tableName, 2 + maxCivs, tableX, tableY, tableW, tableH, True, False, self.STATS_ROW_H, self.STATS_ROW_H, CHART_TABLE_STYLE)
		screen.enableSort(tableName)

		# Column headers: "Starting Tech" in first column (implicit panel title)
		SASTextScale.setTableColumnHeaderScaled(screen, tableName, 0, localText.getText("TXT_KEY_PEDIA_SAS_STARTING_TECH_HEADER", ()), colTechW, sasFontTagLabel)
		SASTextScale.setTableColumnHeaderScaled(screen, tableName, 1, localText.getText("TXT_KEY_PEDIA_SAS_TOTAL_COUNT", ()), colCountW, sasFontTagLabel)
		inchart_set_icon_column_headers(screen, tableName, 2, maxCivs, civColW)

		for iTech, civCount, civIds in startingTechData:
			iRow = screen.appendTableRow(tableName)
			techInfo = gc.getTechInfo(iTech)

			SASTextScale.setTableTextScaled(screen, tableName, 0, iRow, techInfo.getDescription(), techInfo.getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, -1, CvUtil.FONT_LEFT_JUSTIFY, sasFontTagLabel)
			SASTextScale.setTableTextScaled(screen, tableName, 1, iRow, u"%d" % civCount, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY, sasFontTagLabel)

			inchart_set_icon_cells(screen, tableName, iRow, civIds, 2, maxCivs, INCHART_ICON_TYPE_CIV)

	def _placeUntradeableTechsByEraTable(self, screen, cache):
		# Bottom-left table: untradeable techs grouped by era
		# Blue panel without header, table inside
		# Columns: Era name | Count (untradeable) | All (total techs in era) | Tech buttons...
		untradeableTechsByEra = cache["untradeableTechsByEra"]
		_ = cache["startingTechMaxCivs"]
		totalTechsByEra = cache["totalTechsByEra"]

		# Panel dimensions
		panelX = self.STATS_LEFT_X
		panelY = self.STATS_LEFT_BOTTOM_Y
		panelW = self.STATS_LEFT_W
		panelH = self.STATS_LEFT_BOTTOM_H

		# Add blue panel without header (empty title)
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, "", "", True, True, panelX, panelY, panelW, panelH, PanelStyles.PANEL_STYLE_BLUE50)

		# Table dimensions inside panel
		tableX = panelX + self.STATS_INNER
		tableY = panelY + self.STATS_INNER
		tableW = panelW - 2 * self.STATS_INNER
		tableH = panelH - 2 * self.STATS_INNER

		# Column widths: Era name | Count (untradeable) | All (total) | Tech buttons...
		# Use same width as top-left table's first column (160) for vertical alignment
		colEraW = 160
		colCountW = 40
		colAllW = 40
		maxTechs = TECH_STATS_FIXED_LEFT_ICON_COLS
		fixedColsW = colEraW + colCountW + colAllW
		techColW, maxTechs = inchart_calc_icon_col_width(tableW, fixedColsW, maxTechs, INCHART_ICON_SIZE, INCHART_ICON_SPACING)

		# Create table (3 fixed columns + tech button columns)
		tableName = self.top.getNextWidgetName()
		screen.addTableControlGFC(tableName, 3 + maxTechs, tableX, tableY, tableW, tableH, True, False, self.STATS_ROW_H, self.STATS_ROW_H, CHART_TABLE_STYLE)
		screen.enableSort(tableName)

		# Column headers: "Untradeable" | "Count" | "All" | tech buttons...
		SASTextScale.setTableColumnHeaderScaled(screen, tableName, 0, localText.getText("TXT_KEY_PEDIA_SAS_UNTRADEABLE_HEADER", ()), colEraW, sasFontTagLabel)
		SASTextScale.setTableColumnHeaderScaled(screen, tableName, 1, localText.getText("TXT_KEY_PEDIA_SAS_COUNT", ()), colCountW, sasFontTagLabel)
		SASTextScale.setTableColumnHeaderScaled(screen, tableName, 2, localText.getText("TXT_KEY_PEDIA_SAS_ALL", ()), colAllW, sasFontTagLabel)
		inchart_set_icon_column_headers(screen, tableName, 3, maxTechs, techColW)

		numEras = gc.getNumEraInfos()
		for iEra in xrange(numEras):
			techCount, techIds = untradeableTechsByEra.get(iEra, (0, []))
			if techCount == 0:
				continue

			iRow = screen.appendTableRow(tableName)
			eraInfo = gc.getEraInfo(iEra)

			# Era name column with DDS icon
			# <!-- custom: NOTE: Cannot make this clickable/redirect to Era Chart because setTableText does not support navigation widgets (WIDGET_PEDIA_MAIN, WIDGET_PYTHON, etc.).
			# Navigation widgets only work with setImageButtonAt, not table cells.
			# To make this clickable, would need to restructure from table to individual buttons. (Claude code Sonnet 4.5) -->
			eraButtonPath = eraInfo.getButton()
			SASTextScale.setTableTextScaled(screen, tableName, 0, iRow, eraInfo.getDescription(), eraButtonPath, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY, sasFontTagLabel)

			# Untradeable tech count column
			SASTextScale.setTableTextScaled(screen, tableName, 1, iRow, u"%d" % techCount, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY, sasFontTagLabel)

			# All techs in era column
			totalInEra = totalTechsByEra.get(iEra, 0)
			SASTextScale.setTableTextScaled(screen, tableName, 2, iRow, u"%d" % totalInEra, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY, sasFontTagLabel)

			# Tech button columns (starting at column 3)
			inchart_set_icon_cells(screen, tableName, iRow, techIds, 3, maxTechs, INCHART_ICON_TYPE_TECH)

	def _placeStartingTechCombosTable(self, screen, cache):
		# Right table: all starting tech combinations globally
		# Blue panel without header, table inside
		startingTechCombos = cache["startingTechCombos"]
		combosMinMax = cache["combosMinMax"]
		_ = cache["combosMaxCivs"]

		# Panel dimensions
		panelX = self.STATS_RIGHT_X
		panelY = self.STATS_RIGHT_Y
		panelW = self.STATS_RIGHT_W
		panelH = self.STATS_RIGHT_H

		# Add blue panel without header (empty title)
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, "", "", True, True, panelX, panelY, panelW, panelH, PanelStyles.PANEL_STYLE_BLUE50)

		# Table dimensions inside panel
		tableX = panelX + self.STATS_INNER
		tableY = panelY + self.STATS_INNER
		tableW = panelW - 2 * self.STATS_INNER
		tableH = panelH - 2 * self.STATS_INNER

		# Column widths
		maxCivsRight = TECH_STATS_FIXED_RIGHT_ICON_COLS
		colCountW = 40
		colRankW = 70
		colCivsTotalW = maxCivsRight * TECH_STATS_RIGHT_ICON_COL_W
		colTechsTotalW = tableW - colCountW - colRankW - colCivsTotalW - TECH_STATS_RIGHT_SCROLLBAR_TAIL_W
		colTech1W = int(colTechsTotalW / 2)
		colTech2W = colTechsTotalW - colTech1W

		# Create table
		tableName = self.top.getNextWidgetName()
		screen.addTableControlGFC(tableName, 4 + maxCivsRight, tableX, tableY, tableW, tableH, True, False, self.STATS_ROW_H, self.STATS_ROW_H, CHART_TABLE_STYLE)
		screen.enableSort(tableName)

		SASTextScale.setTableColumnHeaderScaled(screen, tableName, 0, u"Tech 1", colTech1W, sasFontTagLabel)
		SASTextScale.setTableColumnHeaderScaled(screen, tableName, 1, u"Tech 2", colTech2W, sasFontTagLabel)
		SASTextScale.setTableColumnHeaderScaled(screen, tableName, 2, localText.getText("TXT_KEY_PEDIA_SAS_TOTAL_COUNT", ()), colCountW, sasFontTagLabel)
		SASTextScale.setTableColumnHeaderScaled(screen, tableName, 3, localText.getText("TXT_KEY_PEDIA_SAS_RANKING", ()), colRankW, sasFontTagLabel)
		inchart_set_icon_column_headers(screen, tableName, 4, maxCivsRight, TECH_STATS_RIGHT_ICON_COL_W)

		minCount, maxCount = combosMinMax
		for tech1, tech2, civCount, civIds in startingTechCombos:
			iRow = screen.appendTableRow(tableName)

			tech1Info = gc.getTechInfo(tech1)
			tech2Info = gc.getTechInfo(tech2)

			SASTextScale.setTableTextScaled(screen, tableName, 0, iRow, tech1Info.getDescription(), tech1Info.getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, tech1, -1, CvUtil.FONT_LEFT_JUSTIFY, sasFontTagLabel)
			SASTextScale.setTableTextScaled(screen, tableName, 1, iRow, tech2Info.getDescription(), tech2Info.getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, tech2, -1, CvUtil.FONT_LEFT_JUSTIFY, sasFontTagLabel)

			# Count
			SASTextScale.setTableTextScaled(screen, tableName, 2, iRow, u"%d" % civCount, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY, sasFontTagLabel)

			# Ranking bar using centralized helper
			rankingBar = inchart_calc_ranking_bar(civCount, minCount, maxCount)
			SASTextScale.setTableTextScaled(screen, tableName, 3, iRow, rankingBar, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY, sasFontTagLabel)

			# Civ icons using centralized helper
			inchart_set_icon_cells(screen, tableName, iRow, civIds, 4, maxCivsRight, INCHART_ICON_TYPE_CIV)

	# <!-- custom: _setCivIconCells and _setTechIconCells removed - now use centralized inchart_set_icon_cells from _sevopedia_helpers -->

	def placeSpecial(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SPECIAL_ABILITIES", ()), "", True, False, self.X_SPECIAL, self.Y_SPECIAL, self.W_SPECIAL, self.H_SPECIAL, PanelStyles.PANEL_STYLE_BLUE50)
		listName = self.top.getNextWidgetName()

		szSpecialText = CyGameTextMgr().getTechHelp(self.iTech, True, False, False, False, -1)[1:]
		screen.addMultilineText(listName, SASTextScale.normalizeLabelText(szSpecialText), self.X_SPECIAL + 5, self.Y_SPECIAL + 30, self.W_SPECIAL - 3, self.H_SPECIAL - 35, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def setHistoryExpanded(self, bExpanded):
		self.bHistoryExpanded = bExpanded

	def placeHistory(self):
		screen = self.top.getScreen()
		szText = gc.getTechInfo(self.iTech).getQuote()
		szText += u"\n\n" + gc.getTechInfo(self.iTech).getCivilopedia()
		szTitle = localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ())
		draw_expandable_text_panel(screen, self.top, szTitle, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, szText, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)
	def handleInput (self, inputClass):
		return 0
