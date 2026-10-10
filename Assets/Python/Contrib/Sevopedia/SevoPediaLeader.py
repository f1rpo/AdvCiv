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
# AI Utilities or Personality Panel for normalization and general helpers for the SevopediaLeader category
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: Long_Comments_py.txt #3 -->

from CvPythonExtensions import *
import CvUtil
import ScreenInput
import SevoScreenEnums
import SASTextScale
from SASFontUtils import sasFontTagLabel
from SASMagicNumbers import *
from _sevopedia_helpers import *
# <!-- custom: import to display chars before Traits -->
import TraitUtil

# <!-- custom: AI personality cache/value computation moved to a dedicated module to keep this file lean. (ChatGPT-5.2 Thinking) -->
import SevoPediaLeaderAIPValues as _SAS_LeaderAIPValues

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

# <!-- custom: Leader page display toggle (not part of AI cache module). -->
IS_SHOW_TRAIT_ICONS_IN_LEADER = (gc.getDefineINT("SAS_SEVOPEDIA_LEADER_TRAITS_SHOW_ICONS") > 0)
IS_SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_ENABLE = (gc.getDefineINT("SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_ENABLE") > 0)
IS_SAS_SEVOPEDIA_LEADER_ATTITUDE_EMOJI_ENABLE = (gc.getDefineINT("SAS_SEVOPEDIA_LEADER_ATTITUDE_EMOJI_ENABLE") > 0)
IS_SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_PANEL_SHOW_EMOJI = (gc.getDefineINT("SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_PANEL_SHOW_EMOJI") > 0)
# <!-- custom: era art feature gate — mirrors the C++ XML-load define.
# When False, the era art row in the Sevopedia leader attitude panel is never shown. (Claude code Sonnet 4.6) -->
IS_SAS_LEADERHEAD_ERA_ART = (gc.getDefineINT("SAS_CV_LEADER_HEAD_INFO_ENABLE_XML_ERA_ART_DEFS") > 0)
SAS_LEADER_ATTITUDE_PREVIEW_ORDER = (AttitudeTypes.ATTITUDE_FURIOUS, AttitudeTypes.ATTITUDE_ANNOYED, AttitudeTypes.ATTITUDE_CAUTIOUS, AttitudeTypes.ATTITUDE_PLEASED, AttitudeTypes.ATTITUDE_FRIENDLY)
SAS_LEADER_ACTION_PREVIEW_ORDER = ((LeaderheadAction.NO_LEADERANIM, u"no"), (LeaderheadAction.LEADERANIM_GREETING, u"gr"), (LeaderheadAction.LEADERANIM_AGREE, u"ag"), (LeaderheadAction.LEADERANIM_DISAGREE, u"dg"))

# <!-- custom: keep debug flag available in this module for existing debug print sites. -->
IS_DEBUG_LEADER = _SAS_LeaderAIPValues.IS_DEBUG_LEADER

# <!-- custom: shared exclusion list used by both cache computation and page rendering. -->
EXCLUDED_LEADER_INDEXES_FROM_CALCULATIONS = _SAS_LeaderAIPValues.EXCLUDED_LEADER_INDEXES_FROM_CALCULATIONS

# <!-- custom: cache containers (populated once per session from SevoPediaMain). -->
LEADERS_INFO_CACHED = {}

def getPrecomputedCacheOnceOnlyFromSevopediaMainInSevopediaLeaderForEntireSession():
	# Called once (from SevoPediaMain) to prebuild the AI Personality Panel cache.
	return _SAS_LeaderAIPValues.getPrecomputedCacheOnceOnlyFromSevopediaMainInSevopediaLeaderForEntireSession()

class SevoPediaLeader:

	def __init__(self, main):
		self.iLeader = -1
		self.bHistoryExpanded = False
		self.bContentExpanded = False
		self.top = main
		self.iSelectedAttitude = AttitudeTypes.ATTITUDE_PLEASED
		self.ATTITUDE_SELECTED_EMOJI_SIZE = 24
		self.ATTITUDE_UNSELECTED_EMOJI_SIZE = 12
		self.attitudeButtonLabelCache = {}
		self.attitudeButtonLabelCache[AttitudeTypes.ATTITUDE_FURIOUS] = (u"fu", u"FU")
		self.attitudeButtonLabelCache[AttitudeTypes.ATTITUDE_ANNOYED] = (u"an", u"AN")
		self.attitudeButtonLabelCache[AttitudeTypes.ATTITUDE_CAUTIOUS] = (u"ca", u"CA")
		self.attitudeButtonLabelCache[AttitudeTypes.ATTITUDE_PLEASED] = (u"pl", u"PL")
		self.attitudeButtonLabelCache[AttitudeTypes.ATTITUDE_FRIENDLY] = (u"fr", u"FR")
		self.attitudeSelectedEmojiPathByAttitude = {}
		self.attitudeSelectedEmojiPathByAttitude[AttitudeTypes.ATTITUDE_FURIOUS] = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_ATTITUDE_FURIOUS").getPath()
		self.attitudeSelectedEmojiPathByAttitude[AttitudeTypes.ATTITUDE_ANNOYED] = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_ATTITUDE_ANNOYED").getPath()
		self.attitudeSelectedEmojiPathByAttitude[AttitudeTypes.ATTITUDE_CAUTIOUS] = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_ATTITUDE_CAUTIOUS").getPath()
		self.attitudeSelectedEmojiPathByAttitude[AttitudeTypes.ATTITUDE_PLEASED] = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_ATTITUDE_PLEASED").getPath()
		self.attitudeSelectedEmojiPathByAttitude[AttitudeTypes.ATTITUDE_FRIENDLY] = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_ATTITUDE_FRIENDLY").getPath()
		self.ATTITUDES_PANEL_ID = "SevoPediaLeaderAttitudesPanel"
		self.ATTITUDE_BUTTON_WIDGET_BY_ATTITUDE = {}
		for iAttitude in SAS_LEADER_ATTITUDE_PREVIEW_ORDER:
			self.ATTITUDE_BUTTON_WIDGET_BY_ATTITUDE[iAttitude] = "SevoPediaLeaderAttitudeBtn%d" % iAttitude
		self.ACTION_BUTTON_WIDGET_BY_ACTION = {}
		for iAction, _ in SAS_LEADER_ACTION_PREVIEW_ORDER:
			self.ACTION_BUTTON_WIDGET_BY_ACTION[iAction] = "SevoPediaLeaderActionBtn%d" % iAction
		# <!-- custom: era art preview state: -1 = default (no override), else the selected era index. (Claude code Sonnet 4.6) -->
		self.iSelectedEra = -1
		self.ERA_DEFAULT_BUTTON_WIDGET = "SevoPediaLeaderEraDefaultBtn"
		self.ERA_BUTTON_WIDGET_BY_ERA = {}
		for iEra in xrange(gc.getNumEraInfos()):
			self.ERA_BUTTON_WIDGET_BY_ERA[iEra] = "SevoPediaLeaderEraBtn%d" % iEra
		self.eraDefaultIconPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_LOCKED_2").getPath()
		self.eraIconPathByEra = {}
		for iEra in xrange(gc.getNumEraInfos()):
			self.eraIconPathByEra[iEra] = gc.getEraInfo(iEra).getButton()
		self.ERA_ICON_SIZE = 16
		# <!-- custom: cache resolved AI categories once at init for this screen instance (runtime Y-resolution aware); no separate header/symbol cache is needed because this already avoids per-draw rebuild cost.
		# Also only do this when AIP is enabled; otherwise skip needless setup.
		# We intentionally don't re-resolve on in-session resolution changes: main interface still needs restart after such changes for correct layout, so dynamic Sevopedia-only redraw adds complexity for little practical gain. (GPT-5.3-Codex) -->
		if IS_SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_ENABLE:
			iScreenHeight = self.top.getScreen().getYResolution()
			self.aiRightCategories, self.aiMiddleCategories, self.aiLeftCategories = self.buildAICategoriesForCurrentResolution(iScreenHeight)
		else:
			self.aiRightCategories, self.aiMiddleCategories, self.aiLeftCategories = (), (), ()

		self.N_AI_TABLE_NUM = 3
		iLeaderItemsWidthFont2 = gc.getDefineINT("SAS_SEVOPEDIA_LEADER_ITEMS_WIDTH_FONT_2")
		if iLeaderItemsWidthFont2 <= 0:
			iLeaderItemsWidthFont2 = self.top.SAS_W_ITEMS_BASE
		iLabelFont = getSASUIFontLabel()
		if iLabelFont <= 1:
			iLeaderItemsWidthCurrentForGain = gc.getDefineINT("SAS_SEVOPEDIA_LEADER_ITEMS_WIDTH_FONT_1")
		elif iLabelFont == 2:
			iLeaderItemsWidthCurrentForGain = gc.getDefineINT("SAS_SEVOPEDIA_LEADER_ITEMS_WIDTH_FONT_2")
		elif iLabelFont == 3:
			iLeaderItemsWidthCurrentForGain = gc.getDefineINT("SAS_SEVOPEDIA_LEADER_ITEMS_WIDTH_FONT_3")
		else:
			iLeaderItemsWidthCurrentForGain = gc.getDefineINT("SAS_SEVOPEDIA_LEADER_ITEMS_WIDTH_FONT_4")
		if iLeaderItemsWidthCurrentForGain <= 0:
			iLeaderItemsWidthCurrentForGain = self.top.SAS_W_ITEMS_BASE
		# <!-- custom: keep AIP panel expansion tied to upscaled font widths, but when item-list reduction is disabled on wide screens, keep page-left anchored at base item width so the rest of the page absorbs the squeeze. (GPT-5.3-Codex) -->
		iLeaderItemsWidthCurrent = iLeaderItemsWidthCurrentForGain
		iNoReduceMinWidth = gc.getDefineINT("SAS_SEVOPEDIA_LEADER_ITEMS_NO_REDUCE_MIN_WIDTH")
		if (iNoReduceMinWidth > 0) and (self.top.getScreen().getXResolution() >= iNoReduceMinWidth):
			iLeaderItemsWidthCurrent = self.top.SAS_W_ITEMS_BASE
		iLeaderItemsWidthGain = iLeaderItemsWidthFont2 - iLeaderItemsWidthCurrentForGain
		if iLeaderItemsWidthGain < 0:
			iLeaderItemsWidthGain = 0
		iAIPPanelGain = iLeaderItemsWidthGain / self.N_AI_TABLE_NUM
		iAIPLabelGain = (iAIPPanelGain * 70) / 100
		iAIPRemainingGain = iAIPPanelGain - iAIPLabelGain
		iAIPValueGain = iAIPRemainingGain / 2
		iAIPScaleGain = iAIPRemainingGain - iAIPValueGain

		# <!-- custom: for leader page, compute page-left from leader item width directly so layout is deterministic at init. (GPT-5.3-Codex) -->
		self.X_LEADERHEAD_PANE = self.top.X_ITEMS + iLeaderItemsWidthCurrent + 18
		self.Y_LEADERHEAD_PANE = self.top.Y_PEDIA_PAGE
		# <!-- custom: for the ratio of the portrait, aim to match closely the ingame diplomacy portrait ratio; Long_Comments_py.txt #2 -->
		self.W_LEADERHEAD_PANE = 327
		self.H_LEADERHEAD_PANE = 400

		# <!-- custom: 1) (most) absolute dimensions first -->
		self.W_AI_PERSONALITY = 290 + iAIPPanelGain
		# <!-- custom: we also need this information sooner, move it here with the more absolute dimensions of some elements
		self.W_CIV = 64
		self.H_CIV = 64
		self.CIV_MARGIN = 0
		self.CIV_DISELEVATION = 38
		self.H_FAVORITES = NON_MULTILIST_PANEL_STANDARD_HEIGHT

		# <!-- custom: 2) (most) relative dimensions or positions then -->

		self.W_LEADERHEAD = self.W_LEADERHEAD_PANE - 30
		self.H_LEADERHEAD = self.H_LEADERHEAD_PANE - 34
		self.X_LEADERHEAD = self.X_LEADERHEAD_PANE + (self.W_LEADERHEAD_PANE - self.W_LEADERHEAD) / 2
		self.Y_LEADERHEAD = self.Y_LEADERHEAD_PANE + (self.H_LEADERHEAD_PANE - self.H_LEADERHEAD) / 2 + 3

		self.W_AI_TOTAL_TABLES_WIDTH = self.N_AI_TABLE_NUM * self.W_AI_PERSONALITY + self.N_AI_TABLE_NUM * LARGE_MARGIN

		self.Y_FAVORITES = self.Y_LEADERHEAD_PANE + self.H_LEADERHEAD_PANE + SMALL_MARGIN

		# <!-- custom: we need self.W_HISTORY before the favourites coordinates, (even though the history panel is placed under/after the favourites panel when i constructed the page's "spacing" and dimensions of (and between) panels, anyways) because/as the favourites panel uses/needs/is based on the history panel's (relative) position, anyways -->
		# <!-- custom: might as well put the other/rest/remaining HISTORY coordinates if doesn't harm and they are perhaps needed too, anyways -->
		self.X_HISTORY = self.X_LEADERHEAD_PANE
		self.Y_HISTORY = self.Y_FAVORITES + self.H_FAVORITES + SMALL_MARGIN
		self.W_HISTORY = self.top.R_PEDIA_PAGE - self.W_AI_TOTAL_TABLES_WIDTH - self.X_LEADERHEAD_PANE
		self.H_HISTORY = self.top.B_PEDIA_PAGE - self.Y_HISTORY

		# Music panel (helper-computed one-button width)
		self.W_MUSIC = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)

		# <!-- custom: Favorites panel only needs two icons (favorite civic/religion), so size it with shared helper for a strict two-button layout. (GPT-5.3-Codex) -->
		self.W_FAVORITES = get_panel_width_for_buttons(2, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)
		# <!-- custom: layout order tweak: Favorites at left, then Music, then civ icon at far right. (GPT-5.3-Codex) -->
		self.X_FAVORITES = self.X_LEADERHEAD_PANE
		self.X_MUSIC = self.X_HISTORY + self.W_HISTORY - self.W_CIV - SMALL_MARGIN - self.W_MUSIC
		self.Y_MUSIC = self.Y_FAVORITES
		self.H_MUSIC = self.H_FAVORITES
		self.X_ATTITUDES = self.X_FAVORITES + self.W_FAVORITES + SMALL_MARGIN
		self.Y_ATTITUDES = self.Y_FAVORITES
		self.W_ATTITUDES = self.X_MUSIC - self.X_ATTITUDES - SMALL_MARGIN
		self.H_ATTITUDES = self.H_FAVORITES
		self.playButtonPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_PLAY_BUTTON").getPath()

		# <!-- custom: the rest of the coordinates here, as it is dependent on other coordinates we need first that (i.e. before being able to add these) -->
		self.X_AI_PERSONALITY = self.top.R_PEDIA_PAGE - self.W_AI_PERSONALITY 
		self.Y_AI_PERSONALITY = self.Y_LEADERHEAD_PANE
		self.H_AI_PERSONALITY = self.H_LEADERHEAD_PANE + SMALL_MARGIN + self.H_FAVORITES + SMALL_MARGIN + self.H_HISTORY

		# <!-- custom: AI Personality Panel(s) column widths -->
		self.W_AI_VALUE = 35 + iAIPValueGain
		self.W_AI_SCALE = 100 + iAIPScaleGain
		self.W_AI_LABEL = self.W_AI_PERSONALITY - self.W_AI_VALUE - self.W_AI_SCALE
		self.H_AI_LINE_HEIGHT = 22
		self.H_AI_CATEGORY_SPACING = 10
		self.W_AI_LEFT_SIDE_PADDING = 12
		# <!-- custom: Long_Comments_py.txt #4 -->
		#self.H_AI_UPPER_PADDING = 36
		self.H_AI_UPPER_PADDING = 15

		self.AI_PANEL_RIGHT_TXT_KEY = "TXT_KEY_AI_PERSONALITY_RIGHT_PANEL"
		self.AI_PANEL_MIDDLE_TXT_KEY = "TXT_KEY_AI_PERSONALITY_MIDDLE_PANEL"
		self.AI_PANEL_LEFT_TXT_KEY = "TXT_KEY_AI_PERSONALITY_LEFT_PANEL"

		self.X_TRAITS = self.X_LEADERHEAD_PANE + self.W_LEADERHEAD_PANE + SMALL_MARGIN
		self.Y_TRAITS = self.Y_LEADERHEAD_PANE
		self.W_TRAITS = self.W_HISTORY - self.W_LEADERHEAD_PANE - SMALL_MARGIN
		self.H_TRAITS = self.H_LEADERHEAD_PANE

		self.X_CIV = self.X_HISTORY + self.W_HISTORY - self.CIV_MARGIN - self.W_CIV
		# <!-- custom: put the flag/civ at the middle Y of the favourites panel -->
		# <!-- custom: quite high as compared to favourites panel's lowest point -->
		self.Y_CIV = self.Y_FAVORITES + self.CIV_DISELEVATION

	def interfaceScreen(self, iLeader):
		if self.iLeader != iLeader:
			self.bHistoryExpanded = False
			self.bContentExpanded = False
			self.iSelectedEra = -1
		self.iLeader = iLeader

		if self.bContentExpanded:
			self.placeLeaderHeadPane()
			return
		# <!-- custom: when history is expanded, render only the overlay to avoid underlying panel bleed-through. (GPT-5.3-Codex) -->
		if self.bHistoryExpanded:
			self.placeHistory()
			return

		self.placeLeaderHeadPane()
		self.placeFavorites()
		self.placeMusic()
		self.placeAttitudes()
		self.placeCiv()
		self.placeTraits()
		place_new_concept_legend_link(self.top, "CONCEPT_SAS_SEVOPEDIA_LEADER_LEGEND")

		# <!-- custom: for excluded leader indexes from calculations, leave the zone/space where the AI personality panel was supposed to be especially empty, instead of getting a key error or missing leader from leaders_info_cached; Long_Comments_py.txt #10 -->
		#
		if IS_SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_ENABLE and (iLeader not in EXCLUDED_LEADER_INDEXES_FROM_CALCULATIONS):
			self.placeAIPersonalityPanel(iLeader)
		else:
			if IS_DEBUG_LEADER and IS_SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_ENABLE:
				print("[DEBUG] Leader index iLeader=%d in EXCLUDED_LEADER_INDEXES_FROM_CALCULATIONS=%s is skipped, leave the place where AI Personality panel was supposed to be entirely empty so we don't get a missing key in leaders_info_cached Error, while signifying clearly enough hopefully that the excluded leader currently selected doesn't have an item in leaders_info_cached and AI Personality Panel at all/is not part of it." % (iLeader, str(EXCLUDED_LEADER_INDEXES_FROM_CALCULATIONS)))
		# <!-- custom: normal-mode panels are drawn first; placeHistory() is called last so its expandable overlay remains top-most. (Claude code Sonnet 4.6 + GPT-5.3-Codex) -->
		self.placeHistory()

	# <!-- custom: wrap leader placement in a specific function for clarity or flexibility or not anyways, -->
	def placeLeaderHeadPane(self):
		screen = self.top.getScreen()
		(iLhX, iLhY, iLhW, iLhH), (iAttX, iAttY, iAttW, iAttH) = draw_expandable_leaderhead_panel(screen, self.top, self.X_LEADERHEAD_PANE, self.Y_LEADERHEAD_PANE, self.W_LEADERHEAD_PANE, self.H_LEADERHEAD_PANE, self.X_LEADERHEAD, self.Y_LEADERHEAD, self.W_LEADERHEAD, self.H_LEADERHEAD, self.bContentExpanded, SAS_MAGIC_PEDIA_PYTHON_CONTENT_EXPAND)
		self.leaderWidget = self.top.getNextWidgetName()
		iRenderLeader = self._getEraRenderLeader(self.iLeader, self.iSelectedEra)
		screen.addLeaderheadGFC(self.leaderWidget, iRenderLeader, self.iSelectedAttitude, iLhX, iLhY, iLhW, iLhH, WidgetTypes.WIDGET_GENERAL, -1, -1)
		if self.bContentExpanded:
			self._drawAttitudeRowAt(screen, iAttX, iAttY, iAttW, iAttH)

	# <!-- custom: originally derived in part from RFC Dawn of Civilization's CvPediaLeader.py; subsequently modified for AdvCiv-SAS. (ChatGPT-5.6-Sol) -->
	def placeFavorites(self):
		screen = self.top.getScreen()
		panel = self.top.getNextWidgetName()
		screen.addPanel(panel, localText.getText("TXT_KEY_PEDIA_FAVOURITE_CIVICS_AND_RELIGIONS", ()), "", False, True, self.X_FAVORITES, self.Y_FAVORITES, self.W_FAVORITES, self.H_FAVORITES, PanelStyles.PANEL_STYLE_BLUE50)
		screen.enableSelect(panel, False)
		screen.attachLabel(panel, "", "  ")

		# Civic
		iCivic = gc.getLeaderHeadInfo(self.iLeader).getFavoriteCivic()
		if iCivic > -1:
			screen.attachImageButton(panel, "", gc.getCivicInfo(iCivic).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, iCivic, 1, False)

		# Religion
		iReligion = gc.getLeaderHeadInfo(self.iLeader).getFavoriteReligion()
		if iReligion > -1:
			screen.attachImageButton(panel, "", gc.getReligionInfo(iReligion).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION, iReligion, 1, False)

	def placeMusic(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_MUSIC_PANEL", ()), "", False, True, self.X_MUSIC, self.Y_MUSIC, self.W_MUSIC, self.H_MUSIC, PanelStyles.PANEL_STYLE_BLUE50)

		# <!-- custom: use attachLabel for padding similar to other 84px panels -->
		screen.attachLabel(panelName, "", "  ")

		buttonSize = 64
		buttonX = (self.W_MUSIC - buttonSize) / 2
		buttonY = 10
		# <!-- custom: redirect to leader's peace music entry in Sevopedia Music category -->
		iMusicKey = self.top.SAS_getLeaderPeaceMusicKey(self.iLeader)
		if iMusicKey != -1:
			screen.setImageButtonAt(self.top.getNextWidgetName(), panelName, self.playButtonPath, buttonX, buttonY, buttonSize, buttonSize, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_MUSIC_ENTRY, iMusicKey)
		else:
			screen.setImageButtonAt(self.top.getNextWidgetName(), panelName, self.playButtonPath, buttonX, buttonY, buttonSize, buttonSize, WidgetTypes.WIDGET_PEDIA_MAIN, SevoScreenEnums.PEDIA_MUSIC, -1)

	def placeAttitudes(self):
		if self.W_ATTITUDES <= 0:
			return

		screen = self.top.getScreen()
		self.deleteAttitudeWidgets(screen)
		# <!-- custom: add direct mood buttons in the center gap (between Favorites and Music) so clicking previews leader attitude animations without keyboard-only hotkeys. (GPT-5.3-Codex) -->
		screen.addPanel(self.ATTITUDES_PANEL_ID, "", "", False, True, self.X_ATTITUDES, self.Y_ATTITUDES, self.W_ATTITUDES, self.H_ATTITUDES, PanelStyles.PANEL_STYLE_BLUE50)

		# <!-- custom: 3 rows when this leader has era art, 2 rows otherwise. (Claude code Sonnet 4.6) -->
		leaderEraArts = []
		if IS_SAS_LEADERHEAD_ERA_ART:
			leaderEraArts = self._getLeaderEraArts(self.iLeader)
		iNumRows = 2
		if leaderEraArts:
			iNumRows = 3

		iRowGap = 4
		iTopPadding = 8
		iBottomPadding = 8
		# <!-- custom: simple manual Y nudge for the two attitude/action rows as one block; increase to move both rows lower. (GPT-5.3-Codex) -->
		iVerticalNudgeY = 3
		iButtonH = (self.H_ATTITUDES - iTopPadding - iBottomPadding - iRowGap * (iNumRows - 1)) / iNumRows
		if iButtonH < 16:
			iButtonH = 16

		attitudeOrder = SAS_LEADER_ATTITUDE_PREVIEW_ORDER
		iAttitudeCount = len(attitudeOrder)
		iAttitudeSpacing = 3
		iAttitudePadding = 6
		iAttitudeButtonW = (self.W_ATTITUDES - 2 * iAttitudePadding - iAttitudeSpacing * (iAttitudeCount - 1)) / iAttitudeCount
		if iAttitudeButtonW < 16:
			iAttitudeSpacing = 2
			iAttitudePadding = 4
			iAttitudeButtonW = (self.W_ATTITUDES - 2 * iAttitudePadding - iAttitudeSpacing * (iAttitudeCount - 1)) / iAttitudeCount
		if iAttitudeButtonW < 14:
			iAttitudeButtonW = 14
		iAttitudeTotalW = iAttitudeCount * iAttitudeButtonW + (iAttitudeCount - 1) * iAttitudeSpacing
		iAttitudeX = self.X_ATTITUDES + (self.W_ATTITUDES - iAttitudeTotalW) / 2
		iAttitudeY = self.Y_ATTITUDES + iTopPadding + iVerticalNudgeY
		iSelectedEmojiSize = self.ATTITUDE_SELECTED_EMOJI_SIZE
		iUnselectedEmojiSize = self.ATTITUDE_UNSELECTED_EMOJI_SIZE

		# <!-- custom: We tested the city-screen filter-style GFC checkbox DDS route here (similar to CvMainInterface building filter buttons), but icon sizing could not be controlled independently enough in this attitude row.
		# Keeping inline <img> gives reliable per-button size control despite the minor known vertical lift side effect. (GPT-5.3-Codex) -->
		for iAttitude in attitudeOrder:
			szWidget = self.ATTITUDE_BUTTON_WIDGET_BY_ATTITUDE[iAttitude]
			if IS_SAS_SEVOPEDIA_LEADER_ATTITUDE_EMOJI_ENABLE:
				szEmojiPath = self.attitudeSelectedEmojiPathByAttitude[iAttitude]
				iEmojiSize = iUnselectedEmojiSize
				if iAttitude == self.iSelectedAttitude:
					iEmojiSize = iSelectedEmojiSize
				szLabel = u"<img=%s size=%d></img>" % (szEmojiPath, iEmojiSize)
				screen.setButtonGFC(szWidget, szLabel, "", iAttitudeX, iAttitudeY, iAttitudeButtonW, iButtonH, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_LEADER_ATTITUDE, iAttitude, ButtonStyles.BUTTON_STYLE_STANDARD)
			else:
				szLabel = SASTextScale.labelText(self.getAttitudeButtonLabel(iAttitude))
				screen.setButtonGFC(szWidget, szLabel, "", iAttitudeX, iAttitudeY, iAttitudeButtonW, iButtonH, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_LEADER_ATTITUDE, iAttitude, ButtonStyles.BUTTON_STYLE_STANDARD)
			iAttitudeX += iAttitudeButtonW + iAttitudeSpacing

		iActionCount = len(SAS_LEADER_ACTION_PREVIEW_ORDER)
		iActionSpacing = 3
		iActionPadding = 6
		iActionButtonW = (self.W_ATTITUDES - 2 * iActionPadding - iActionSpacing * (iActionCount - 1)) / iActionCount
		if iActionButtonW < 16:
			iActionSpacing = 2
			iActionPadding = 4
			iActionButtonW = (self.W_ATTITUDES - 2 * iActionPadding - iActionSpacing * (iActionCount - 1)) / iActionCount
		if iActionButtonW < 14:
			iActionButtonW = 14
		iActionTotalW = iActionCount * iActionButtonW + (iActionCount - 1) * iActionSpacing
		iActionX = self.X_ATTITUDES + (self.W_ATTITUDES - iActionTotalW) / 2
		iActionY = iAttitudeY + iButtonH + iRowGap

		for iAction, szLabel in SAS_LEADER_ACTION_PREVIEW_ORDER:
			szWidget = self.ACTION_BUTTON_WIDGET_BY_ACTION[iAction]
			screen.setButtonGFC(szWidget, SASTextScale.labelText(szLabel), "", iActionX, iActionY, iActionButtonW, iButtonH, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_LEADER_ACTION, iAction, ButtonStyles.BUTTON_STYLE_STANDARD)
			iActionX += iActionButtonW + iActionSpacing

		# <!-- custom: era art preview row — only when IS_SAS_LEADERHEAD_ERA_ART and this leader has era art entries. (Claude code Sonnet 4.6) -->
		if leaderEraArts:
			self._placeEraRowAt(screen, self.X_ATTITUDES, iActionY + iButtonH + iRowGap, self.W_ATTITUDES, iButtonH, leaderEraArts, named=True)

	def deleteAttitudeWidgets(self, screen):
		screen.deleteWidget(self.ATTITUDES_PANEL_ID)
		for iAttitude in SAS_LEADER_ATTITUDE_PREVIEW_ORDER:
			screen.deleteWidget(self.ATTITUDE_BUTTON_WIDGET_BY_ATTITUDE[iAttitude])
		for iAction, _ in SAS_LEADER_ACTION_PREVIEW_ORDER:
			screen.deleteWidget(self.ACTION_BUTTON_WIDGET_BY_ACTION[iAction])
		screen.deleteWidget(self.ERA_DEFAULT_BUTTON_WIDGET)
		for iEra in xrange(gc.getNumEraInfos()):
			screen.deleteWidget(self.ERA_BUTTON_WIDGET_BY_ERA[iEra])

	def getAttitudeButtonLabel(self, iAttitude):
		szLabelLower, szLabelUpper = self.attitudeButtonLabelCache[iAttitude]
		if iAttitude == self.iSelectedAttitude:
			return szLabelUpper
		return szLabelLower

	# <!-- custom: era art helpers. (Claude code Sonnet 4.6) -->

	def _getLeaderEraArts(self, iLeader):
		# Returns [(iEra, szArtTag), ...] for eras that have art defined for this leader.
		result = []
		leaderInfo = gc.getLeaderHeadInfo(iLeader)
		for iEra in xrange(gc.getNumEraInfos()):
			szTag = leaderInfo.getEraArtDefineTag(iEra)
			if szTag:
				result.append((iEra, szTag))
		return result

	def _getEraRenderLeader(self, iLeader, iSelectedEra):
		# Returns the leader index to pass to addLeaderheadGFC for the given era selection.
		# iSelectedEra == -1 means default (no override). Otherwise finds the leader whose
		# base ArtDefineTag matches the era art tag, so the right 3D model is rendered.
		if iSelectedEra < 0:
			return iLeader
		szEraTag = gc.getLeaderHeadInfo(iLeader).getEraArtDefineTag(iSelectedEra)
		if not szEraTag:
			return iLeader
		for i in xrange(gc.getNumLeaderHeadInfos()):
			if gc.getLeaderHeadInfo(i).getArtDefineTag() == szEraTag:
				return i
		return iLeader

	def _placeEraRowAt(self, screen, iX, iY, iW, iH, leaderEraArts, named):
		# Draws default + era buttons. named=True uses stable widget IDs (for deleteAttitudeWidgets);
		# named=False uses anonymous widget names (expanded overlay, cleaned up by pediaJump).
		iSpacing = -3
		iPadding = 0
		iEraCount = 1 + len(leaderEraArts)  # 1 for default button
		iButtonW = (iW - 2 * iPadding - iSpacing * (iEraCount - 1)) / iEraCount
		if iButtonW < 14:
			iButtonW = 14
		iTotalW = iEraCount * iButtonW + iSpacing * (iEraCount - 1)
		iDrawX = iX + (iW - iTotalW) / 2

		if self.iSelectedEra < 0:
			szDefaultLabel = u"<img=%s size=%d></img>" % (self.eraDefaultIconPath, self.ERA_ICON_SIZE)
		else:
			szDefaultLabel = SASTextScale.labelText(u"d")
		if named:
			szDefaultWidget = self.ERA_DEFAULT_BUTTON_WIDGET
		else:
			szDefaultWidget = self.top.getNextWidgetName()
		screen.setButtonGFC(szDefaultWidget, szDefaultLabel, "", iDrawX, iY, iButtonW, iH, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_LEADER_ERA, -1, ButtonStyles.BUTTON_STYLE_STANDARD)
		iDrawX += iButtonW + iSpacing

		for iEra, _ in leaderEraArts:
			if self.iSelectedEra == iEra:
				szEraLabel = u"<img=%s size=%d></img>" % (self.eraIconPathByEra[iEra], self.ERA_ICON_SIZE)
			else:
				szEraLabel = SASTextScale.labelText(u"%d" % iEra)
			if named:
				szEraWidget = self.ERA_BUTTON_WIDGET_BY_ERA[iEra]
			else:
				szEraWidget = self.top.getNextWidgetName()
			screen.setButtonGFC(szEraWidget, szEraLabel, "", iDrawX, iY, iButtonW, iH, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_LEADER_ERA, iEra, ButtonStyles.BUTTON_STYLE_STANDARD)
			iDrawX += iButtonW + iSpacing

	def applyLeaderEra(self, iEra):
		# iEra == -1 resets to default art; otherwise selects that era's art.
		self.iSelectedEra = iEra
		if self.bContentExpanded:
			self.top.pediaJump(self.top.iCategory, self.top.iItem, False, False)
			return 1
		self.refreshLeaderheadWidget()
		self.placeAttitudes()
		return 1

	def refreshLeaderheadWidget(self):
		if self.iLeader < 0:
			return 0
		screen = self.top.getScreen()
		screen.deleteWidget(self.leaderWidget)
		iRenderLeader = self._getEraRenderLeader(self.iLeader, self.iSelectedEra)
		screen.addLeaderheadGFC(self.leaderWidget, iRenderLeader, self.iSelectedAttitude, self.X_LEADERHEAD, self.Y_LEADERHEAD, self.W_LEADERHEAD, self.H_LEADERHEAD, WidgetTypes.WIDGET_GENERAL, -1, -1)
		return 1

	def setHistoryExpanded(self, bExpanded):
		self.bHistoryExpanded = bExpanded

	def setContentExpanded(self, bExpanded):
		self.bContentExpanded = bExpanded

	def _drawAttitudeRowAt(self, screen, iX, iY, iW, iH):
		# <!-- custom: draws attitude+action (+ optional era) buttons in the expanded overlay right column.
		# iW and iH are fixed by the helper (191x100), so no need for the dynamic fallback resizing that placeAttitudes uses for variable screen widths. (Claude code Sonnet 4.6) -->
		iSpacing = 3
		iPadding = 6
		leaderEraArts = []
		if IS_SAS_LEADERHEAD_ERA_ART:
			leaderEraArts = self._getLeaderEraArts(self.iLeader)
		iNumRows = 2
		if leaderEraArts:
			iNumRows = 3
		iButtonH = (iH - 20 - 4 * (iNumRows - 2)) / iNumRows
		iAttitudeY = iY + 11

		iAttitudeCount = len(SAS_LEADER_ATTITUDE_PREVIEW_ORDER)
		iAttitudeButtonW = (iW - 2 * iPadding - iSpacing * (iAttitudeCount - 1)) / iAttitudeCount
		iAttitudeX = iX + (iW - (iAttitudeCount * iAttitudeButtonW + (iAttitudeCount - 1) * iSpacing)) / 2

		for iAttitude in SAS_LEADER_ATTITUDE_PREVIEW_ORDER:
			if IS_SAS_SEVOPEDIA_LEADER_ATTITUDE_EMOJI_ENABLE:
				iEmojiSize = self.ATTITUDE_UNSELECTED_EMOJI_SIZE
				if iAttitude == self.iSelectedAttitude:
					iEmojiSize = self.ATTITUDE_SELECTED_EMOJI_SIZE
				szLabel = u"<img=%s size=%d></img>" % (self.attitudeSelectedEmojiPathByAttitude[iAttitude], iEmojiSize)
			else:
				szLabel = SASTextScale.labelText(self.getAttitudeButtonLabel(iAttitude))
			screen.setButtonGFC(self.top.getNextWidgetName(), szLabel, "", iAttitudeX, iAttitudeY, iAttitudeButtonW, iButtonH, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_LEADER_ATTITUDE, iAttitude, ButtonStyles.BUTTON_STYLE_STANDARD)
			iAttitudeX += iAttitudeButtonW + iSpacing

		iActionCount = len(SAS_LEADER_ACTION_PREVIEW_ORDER)
		iActionButtonW = (iW - 2 * iPadding - iSpacing * (iActionCount - 1)) / iActionCount
		iActionX = iX + (iW - (iActionCount * iActionButtonW + (iActionCount - 1) * iSpacing)) / 2
		iActionY = iAttitudeY + iButtonH + 4

		for iAction, szLabel in SAS_LEADER_ACTION_PREVIEW_ORDER:
			screen.setButtonGFC(self.top.getNextWidgetName(), SASTextScale.labelText(szLabel), "", iActionX, iActionY, iActionButtonW, iButtonH, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_LEADER_ACTION, iAction, ButtonStyles.BUTTON_STYLE_STANDARD)
			iActionX += iActionButtonW + iSpacing

		if leaderEraArts:
			self._placeEraRowAt(screen, iX, iActionY + iButtonH + 4, iW, iButtonH, leaderEraArts, named=False)

	def placeHistory(self):
		screen = self.top.getScreen()
		# <!-- custom: use normalizeLabelText here because many leader Civilopedia entries already include embedded <font=...> tags; simple labelText then leaves text at legacy small size instead of applying SAS upscaling. (Claude code Sonnet 4.6 + GPT-5.3-Codex) -->
		szText = SASTextScale.normalizeLabelText(gc.getLeaderHeadInfo(self.iLeader).getCivilopedia())
		szTitle = u""
		draw_expandable_text_panel(screen, self.top, szTitle, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, szText, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)
	# <!-- custom: logo / flag of the civ -->
	def placeCiv(self):
		screen = self.top.getScreen()
		for iCiv in xrange(gc.getNumCivilizationInfos()):
			civ = gc.getCivilizationInfo(iCiv)
			if civ.isLeaders(self.iLeader):
				screen.setImageButton(self.top.getNextWidgetName(), civ.getButton(), self.X_CIV, self.Y_CIV, self.W_CIV, self.H_CIV, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIV, iCiv, 1)

	# advc.001 (from Taurus): Static for use by SevoPediaMain; body cut from placeTraits.
	@staticmethod
	def getCiv(iLeader):
		iNumCivs = 0
		for iCiv in xrange(gc.getNumCivilizationInfos()):
			if gc.getCivilizationInfo(iCiv).isLeaders(iLeader):
				iNumCivs += 1
				iLeaderCiv = iCiv
		# <advc.001> (No functional change here)
		if iNumCivs != 1:
			return -1
		return iLeaderCiv # </advc.001>

	def placeTraits(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		# <!-- custom: no header for more compact and prettier display -->
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_LEADER_TRAITS", ()), "", True, False, self.X_TRAITS, self.Y_TRAITS, self.W_TRAITS, self.H_TRAITS, PanelStyles.PANEL_STYLE_BLUE50)
		listName = self.top.getNextWidgetName()
		# advc.001: Civ search moved into a static method
		szSpecialText = CyGameTextMgr().parseLeaderTraits(self.iLeader, SevoPediaLeader.getCiv(self.iLeader), False, True)
		szSpecialText = szSpecialText[1:]

		# <!-- custom: add trait icons by replacing the trait header text once per leader trait (linked or plain); avoids per-line scans and keeps only traits the leader actually has. (GPT-5.2-Codex) -->
		if IS_SHOW_TRAIT_ICONS_IN_LEADER:
			leader = gc.getLeaderHeadInfo(self.iLeader)
			for iTrait in xrange(gc.getNumTraitInfos()):
				if leader.hasTrait(iTrait):
					traitDesc = gc.getTraitInfo(iTrait).getDescription()
					traitLink = make_pedia_link(traitDesc)
					traitIcon = TraitUtil.getIcon(iTrait)
					# <!-- custom: prefer replacing the linked trait label; if traits are plain text, replace the first matching trait name only. (GPT-5.2-Codex) -->
					szReplaced = szSpecialText.replace(traitLink, traitIcon + u" " + traitLink)
					if szReplaced == szSpecialText:
						szSpecialText = szSpecialText.replace(traitDesc, traitIcon + u" " + traitDesc, 1)
					else:
						szSpecialText = szReplaced

		# <!-- custom: reduce top padding now that the traits header is removed (GPT-5.2-Codex).
		# Was headerExtraHeight 30 -->
		headerExtraHeight = 10
		screen.addMultilineText(listName, SASTextScale.normalizeLabelText(szSpecialText), self.X_TRAITS + 5, self.Y_TRAITS + headerExtraHeight, self.W_TRAITS - 10, self.H_TRAITS - headerExtraHeight - 5, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def getXAIPanelCoordinate(self, tableId):
		return self.X_AI_PERSONALITY - tableId * self.W_AI_PERSONALITY - tableId * LARGE_MARGIN

	def getAIPersonalityLeftX(self):
		return self.getXAIPanelCoordinate(self.N_AI_TABLE_NUM - 1)

	def setupAIPanel(self, screen, txtKey, xPanel):
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText(txtKey, ()),"", True, True, xPanel, self.Y_AI_PERSONALITY, self.W_AI_PERSONALITY, self.H_AI_PERSONALITY, PanelStyles.PANEL_STYLE_BLUE50)

	def fillAITableRow(self, screen, label, value, scale, xLabel, xValue, xScale, y):
		labelText = SASTextScale.labelText(label)
		valueText = SASTextScale.applyFontTag(u"%d" % value, sasFontTagLabel.bold)
		scaleText = SASTextScale.labelText(scale)

		screen.setText(self.top.getNextWidgetName(), "", labelText, CvUtil.FONT_LEFT_JUSTIFY, xLabel, y, 0, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.setText(self.top.getNextWidgetName(), "", valueText, CvUtil.FONT_LEFT_JUSTIFY, xValue, y, 0, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.setText(self.top.getNextWidgetName(), "", scaleText, CvUtil.FONT_LEFT_JUSTIFY, xScale, y, 0, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def renderAICategories(self, screen, ai_categories, xPanel, yPanel, leader_info_cached):
		xLabel = xPanel + self.W_AI_LEFT_SIDE_PADDING
		xValue = xLabel + self.W_AI_LABEL
		xScale = xValue + self.W_AI_VALUE
		y = yPanel + self.H_AI_UPPER_PADDING

		for ai_category in ai_categories:
			ai_category_header_line, ai_category_x_offset, ai_category_key_order = ai_category

			# AI Category Header Line
			if ai_category_header_line is not None:
				xOffsetButton = xLabel + ai_category_x_offset
				screen.setText(self.top.getNextWidgetName(), "", SASTextScale.applyFontTag(ai_category_header_line, sasFontTagLabel.bold), CvUtil.FONT_LEFT_JUSTIFY, xOffsetButton, y, 0, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				y += self.H_AI_LINE_HEIGHT

			# <!-- custom: AI Category items in their predefined order -->
			for key in ai_category_key_order:
				label, norm_value, scale = leader_info_cached[key]
				self.fillAITableRow(screen, label, norm_value, scale, xLabel, xValue, xScale, y)
				y += self.H_AI_LINE_HEIGHT

			# <!-- custom: space for next ai_category if any are there (else still space but not used more efficient this way i think i mean than rechecking each time and we have some tables that overflow vertically too so maybe fine this way too if not broken in this case i mean maybe -->
			y += self.H_AI_CATEGORY_SPACING

	# <!-- custom: build categories once at init using runtime resolution (Y), with named category variables so layout tuning stays readable. (GPT-5.3-Codex) -->
	def buildAICategoriesForCurrentResolution(self, iScreenHeight):
		def get_header_text(header_key_or_text):
			if header_key_or_text is None:
				return None
			if header_key_or_text.startswith("TXT_KEY_"):
				return localText.getText(header_key_or_text, ())
			return header_key_or_text

		def get_ai_category(icon_button_art_key, header_key_or_text, ai_category_key_order):
			if header_key_or_text is None:
				ai_category_header_line = None
				ai_category_x_offset = 0
			else:
				header_text = get_header_text(header_key_or_text)
				if IS_SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_PANEL_SHOW_EMOJI and icon_button_art_key:
					button_size = 16
					line_button_txt = u"<img=%s size=%s></img>" % (ArtFileMgr.getInterfaceArtInfo(icon_button_art_key).getPath(), str(button_size))
					ai_category_header_line = u"%s %s" % (line_button_txt, header_text)
					# <!-- custom: keep a small negative header offset so emoji+title rows align with table body and waste less horizontal room. (GPT-5.3-Codex) -->
					ai_category_x_offset = -7
				else:
					ai_category_header_line = header_text
					ai_category_x_offset = 0
			return (ai_category_header_line, ai_category_x_offset, ai_category_key_order)

		def build_memory_order(positive_negative, affection_resentment, suffixes):
			return tuple("iAggregated%sMemory%s%s" % (positive_negative, suffix, affection_resentment) for suffix in suffixes)

		positive_memory_suffixes = ("GiveHelp", "AcceptDemand", "AcceptedReligion", "AcceptedCivic", "AcceptedJoinWar", "AcceptedStopTrading", "VotedForUs", "EventGoodToUs", "LiberatedCities", "Independence", "TradedTechToUs")
		negative_memory_suffixes = ["DeclaredWar", "DeclaredWarOnFriend", "HiredWarAlly", "NukedUs", "NukedFriend", "RazedCity", "RazedHolyCity", "SpyCaught", "RefusedHelp", "RejectedDemand", "DeniedReligion", "DeniedCivic", "DeniedJoinWar", "DeniedStopTrading", "StoppedTrading", "HiredTradeEmbargo", "MadeDemand", "VotedAgainstUs", "EventBadToUs", "CancelledVassalAgreement", "DeclaredWarRecent", "ReceivedTechFromAny", "MadeDemandRecent", "CancelledOpenBorders"]
		if iScreenHeight >= 1440:
			negative_memory_suffixes.append("StoppedTradingRecent")
		# <!-- custom: at 1080p/1440p, keep "StoppedTradingRecent" hidden (lower-priority tail field) to avoid overflow in tighter panel layouts. (GPT-5.3-Codex) -->
		negative_memory_suffixes.append("CancelledDefensivePact")
		negative_memory_suffixes = tuple(negative_memory_suffixes)

		positive_memory_affections_category = get_ai_category("SAS_EMOJI_RED_HEART", "TXT_KEY_LEADER_AI_PANEL_POSITIVE_MEMORY_AFFECTIONS", build_memory_order("Positive", "Affection", positive_memory_suffixes))
		negative_memory_resentments_category = get_ai_category("SAS_EMOJI_SKULL", "TXT_KEY_LEADER_AI_PANEL_NEGATIVE_MEMORY_RESENTMENTS", build_memory_order("Negative", "Resentment", negative_memory_suffixes))
		contact_offer_probabilities_category = get_ai_category("SAS_EMOJI_DOVE", "TXT_KEY_LEADER_AI_PANEL_CONTACT_OFFER_PROBABILITIES", ("iAggregatedContactProbPeaceTreaty", "iAggregatedContactProbOpenBorders", "iAggregatedContactProbTradeMap", "iAggregatedContactProbTradeTech", "iAggregatedContactProbTradeBonus", "iAggregatedContactProbGiveHelp", "iAggregatedContactProbDefensivePact", "iAggregatedContactProbPermanentAlliance"))
		contact_demand_probabilities_category = get_ai_category("SAS_EMOJI_MEGAPHONE", "TXT_KEY_LEADER_AI_PANEL_CONTACT_DEMAND_PROBABILITIES", ("iAggregatedContactProbReligionPressure", "iAggregatedContactProbCivicPressure", "iAggregatedContactProbStopTrading", "iAggregatedContactProbDemandTribute", "iAggregatedContactProbAskForHelp", "iAggregatedContactProbJoinWar"))
		refusal_thresholds_offer_category = get_ai_category("SAS_EMOJI_NO_ENTRY", "TXT_KEY_LEADER_AI_PANEL_REFUSAL_THRESHOLDS_OFFER", ("getOpenBordersRefuseAttitudeThreshold", "getMapRefuseAttitudeThreshold", "getTechRefuseAttitudeThreshold", "getStrategicBonusRefuseAttitudeThreshold", "getHappinessBonusRefuseAttitudeThreshold", "getHealthBonusRefuseAttitudeThreshold", "getNoGiveHelpAttitudeThreshold", "getDefensivePactRefuseAttitudeThreshold"))
		# <!-- custom: higher threshold means harder to convince, so refusal_thresholds_demand sits in demand-refusal section with the other attitude gates. (GPT-5.3-Codex) -->
		refusal_thresholds_demand_category = get_ai_category("SAS_EMOJI_AXE", "TXT_KEY_LEADER_AI_PANEL_REFUSAL_THRESHOLDS_DEMAND", ("getConvertReligionRefuseAttitudeThreshold", "getAdoptCivicRefuseAttitudeThreshold", "getDeclareWarRefuseAttitudeThreshold", "getDeclareWarThemRefuseAttitudeThreshold", "getStopTradingRefuseAttitudeThreshold", "getStopTradingThemRefuseAttitudeThreshold", "getDemandTributeAttitudeThreshold", "getCityRefuseAttitudeThreshold", "getNativeCityRefuseAttitudeThreshold", "getVassalRefuseAttitudeThreshold"))
		no_war_at_category = get_ai_category("SAS_EMOJI_HERB", "TXT_KEY_LEADER_AI_PANEL_NO_WAR_AT", ("iNoWarAttitudeProbFurious", "iNoWarAttitudeProbAnnoyed", "iNoWarAttitudeProbCautious", "iNoWarAttitudeProbPleased", "iNoWarAttitudeProbFriendly"))
		attitude_changes_category = get_ai_category("SAS_EMOJI_CHART_DECREASING", "TXT_KEY_LEADER_AI_PANEL_ATTITUDE_CHANGES", ("getSameReligionAttitudeChange", "getSameReligionAttitudeDivisor", "getDifferentReligionAttitudeChange", "getDifferentReligionAttitudeDivisor", "getFavoriteCivicAttitudeChange", "getFavoriteCivicAttitudeDivisor", "getLostWarAttitudeChange", "getAtWarAttitudeDivisor", "getAtWarAttitudeChangeLimit", "getAtPeaceAttitudeDivisor", "getAtPeaceAttitudeChangeLimit", "getShareWarAttitudeChange", "getShareWarAttitudeDivisor", "getBonusTradeAttitudeDivisor", "getBonusTradeAttitudeChangeLimit", "getOpenBordersAttitudeDivisor", "getOpenBordersAttitudeChangeLimit", "getDefensivePactAttitudeDivisor", "getDefensivePactAttitudeChangeLimit"))
		misc_modifiers_category = get_ai_category("SAS_EMOJI_WRENCH", "TXT_KEY_LEADER_AI_PANEL_MISC_MODIFIERS", ("getFreedomAppreciation",))
		# <!-- custom: ACL limits stay in core_personality intentionally; thematic purity is secondary to keeping this column compact/readable on constrained widths. (GPT-5.3-Codex) -->
		core_personality_category = get_ai_category("SAS_EMOJI_BRAIN", "TXT_KEY_LEADER_AI_PANEL_CORE_PERSONALITY", ("getBaseAttitude", "getBasePeaceWeight", "getPeaceWeightRand", "getWorseRankDifferenceAttitudeChange", "getBetterRankDifferenceAttitudeChange", "getWarmongerRespect", "getCloseBordersAttitudeChange", "getSameReligionAttitudeChangeLimit", "getDifferentReligionAttitudeChangeLimit", "getFavoriteCivicAttitudeChangeLimit"))
		victory_weights_category = get_ai_category("SAS_EMOJI_TROPHY", "TXT_KEY_LEADER_AI_PANEL_BBAI_VICTORY_WEIGHTS", ("getConquestVictoryWeight", "getDominationVictoryWeight", "getCultureVictoryWeight", "getDiplomacyVictoryWeight", "getSpaceVictoryWeight"))
		flavors_category = get_ai_category("SAS_EMOJI_GEAR", "TXT_KEY_LEADER_AI_PANEL_FLAVORS", ("iFlavorMilitary", "iFlavorReligion", "iFlavorProduction", "iFlavorGold", "iFlavorScience", "iFlavorCulture", "iFlavorGrowth", "iFlavorEspionage"))
		# <!-- custom: keep ShareWar ACL in war_strategy so war heuristics remain visible together instead of introducing another micro-section. (GPT-5.3-Codex) -->
		war_strategy_category = get_ai_category("SAS_EMOJI_CROSSED_SWORDS", "TXT_KEY_LEADER_AI_PANEL_WAR_STRATEGY", ("getMaxWarRand", "getMaxWarNearbyPowerRatio", "getMaxWarDistantPowerRatio", "getMaxWarMinAdjacentLandPercent", "getLimitedWarRand", "getLimitedWarPowerRatio", "getBaseAttackOddsChange", "getAttackOddsChangeRand", "getRazeCityProb", "getDemandRebukedSneakProb", "getDemandRebukedWarProb", "getDogpileWarRand", "getDeclareWarTradeRand", "getShareWarAttitudeChangeLimit", "getVassalPowerModifier", "getRefuseToTalkWarThreshold", "getMakePeaceRand"))
		economic_preferences_category = get_ai_category("SAS_EMOJI_MONEY_BAG", "TXT_KEY_LEADER_AI_PANEL_ECONOMIC_PREFERENCES", ("getMaxGoldTradePercent", "getMaxGoldPerTurnTradePercent", "getTechTradeKnownPercent", "getNoTechTradeThreshold", "getBuildUnitProb", "getWonderConstructRand", "getEspionageWeight"))

		if iScreenHeight >= 1440:
			# <!-- custom: at 1440p and above, move "No War At" to the left column so the middle column can fully show ACL-heavy rows. (GPT-5.3-Codex) -->
			middle_categories = (contact_offer_probabilities_category, contact_demand_probabilities_category, refusal_thresholds_offer_category, refusal_thresholds_demand_category, attitude_changes_category, misc_modifiers_category)
			left_categories = (core_personality_category, victory_weights_category, flavors_category, war_strategy_category, no_war_at_category)
		else:
			# <!-- custom: below 1440p, keep "No War At" in middle to preserve the tighter legacy ordering. (GPT-5.3-Codex) -->
			middle_categories = (contact_offer_probabilities_category, contact_demand_probabilities_category, refusal_thresholds_offer_category, refusal_thresholds_demand_category, no_war_at_category, attitude_changes_category, misc_modifiers_category)
			left_categories = (core_personality_category, victory_weights_category, flavors_category, war_strategy_category)

		right_categories = (economic_preferences_category, positive_memory_affections_category, negative_memory_resentments_category)

		return right_categories, middle_categories, left_categories

	# Place AI Personality Panel (using precomputed scales)
	# Renders the full AI Personality panel in the Sevopedia Leader page using precomputed <!-- custom: leader info tuples in leaders_info_cached --> for the given leader.
	def placeAIPersonalityPanel(self, iLeader):
		screen = self.top.getScreen()

		xPanelRight = self.getXAIPanelCoordinate(self.N_AI_TABLE_NUM - 3)
		xPanelMiddle = self.getXAIPanelCoordinate(self.N_AI_TABLE_NUM - 2)
		xPanelLeft = self.getXAIPanelCoordinate(self.N_AI_TABLE_NUM - 1)

		self.setupAIPanel(screen, self.AI_PANEL_RIGHT_TXT_KEY, xPanelRight)
		self.setupAIPanel(screen, self.AI_PANEL_MIDDLE_TXT_KEY, xPanelMiddle)
		self.setupAIPanel(screen, self.AI_PANEL_LEFT_TXT_KEY, xPanelLeft)

		# <!-- custom: cache for performance optimization. -->
		leader_info_cached = LEADERS_INFO_CACHED[iLeader]
		self.renderAICategories(screen, self.aiRightCategories, xPanelRight, self.Y_AI_PERSONALITY, leader_info_cached)
		self.renderAICategories(screen, self.aiMiddleCategories, xPanelMiddle, self.Y_AI_PERSONALITY, leader_info_cached)
		self.renderAICategories(screen, self.aiLeftCategories, xPanelLeft, self.Y_AI_PERSONALITY, leader_info_cached)

	def handleInput (self, inputClass):
		if inputClass.getButtonType() == WidgetTypes.WIDGET_PYTHON:
			if inputClass.getData1() == SAS_MAGIC_PEDIA_PYTHON_LEADER_ATTITUDE:
				return self.applyLeaderAttitude(inputClass.getData2())
			if inputClass.getData1() == SAS_MAGIC_PEDIA_PYTHON_LEADER_ACTION:
				return self.applyLeaderAction(inputClass.getData2())
			if inputClass.getData1() == SAS_MAGIC_PEDIA_PYTHON_LEADER_ERA:
				return self.applyLeaderEra(inputClass.getData2())

		# <!-- custom: leaderhead hotkeys (animations/moods) are cosmetic; if they conflict with search, consider removing or remapping here. (GPT-5.2-Codex) -->
		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CHARACTER):
			if (inputClass.getData() == int(InputTypes.KB_0)):
				self.top.getScreen().performLeaderheadAction(self.leaderWidget, LeaderheadAction.LEADERANIM_GREETING)
			elif (inputClass.getData() == int(InputTypes.KB_6)):
				self.top.getScreen().performLeaderheadAction(self.leaderWidget, LeaderheadAction.LEADERANIM_DISAGREE)
			elif (inputClass.getData() == int(InputTypes.KB_7)):
				self.top.getScreen().performLeaderheadAction(self.leaderWidget, LeaderheadAction.LEADERANIM_AGREE)
			elif (inputClass.getData() == int(InputTypes.KB_1)):
				return self.applyLeaderAttitude(AttitudeTypes.ATTITUDE_FRIENDLY)
			elif (inputClass.getData() == int(InputTypes.KB_2)):
				return self.applyLeaderAttitude(AttitudeTypes.ATTITUDE_PLEASED)
			elif (inputClass.getData() == int(InputTypes.KB_3)):
				return self.applyLeaderAttitude(AttitudeTypes.ATTITUDE_CAUTIOUS)
			elif (inputClass.getData() == int(InputTypes.KB_4)):
				return self.applyLeaderAttitude(AttitudeTypes.ATTITUDE_ANNOYED)
			elif (inputClass.getData() == int(InputTypes.KB_5)):
				return self.applyLeaderAttitude(AttitudeTypes.ATTITUDE_FURIOUS)
			else:
				self.top.getScreen().leaderheadKeyInput(self.leaderWidget, inputClass.getData())
		return 0

	def applyLeaderAttitude(self, iAttitude):
		if iAttitude not in SAS_LEADER_ATTITUDE_PREVIEW_ORDER:
			return 0
		self.iSelectedAttitude = iAttitude
		if self.bContentExpanded:
			# <!-- custom: in expanded mode a full re-render is needed so the leaderhead and attitude row both redraw at expanded coords. (Claude code Sonnet 4.6) -->
			self.top.pediaJump(self.top.iCategory, self.top.iItem, False, False)
			return 1
		# <!-- custom: force-refresh the leaderhead widget so attitude changes show immediately on click; mood-only updates can be visually ignored while another anim is still running. (GPT-5.3-Codex) -->
		self.refreshLeaderheadWidget()
		self.placeAttitudes()
		return 1

	def applyLeaderAction(self, iAction):
		self.top.getScreen().performLeaderheadAction(self.leaderWidget, iAction)
		return 1
