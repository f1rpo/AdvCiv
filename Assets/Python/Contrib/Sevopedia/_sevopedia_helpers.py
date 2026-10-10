# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from CvPythonExtensions import *
from SASMagicNumbers import *

import CvUtil
import re
from SASFontUtils import *
import SASTextScale
from SASUtils import getNewConceptID

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()
IS_SAS_CV_INFO_SCREEN_TIMELINE_LOG_BUTTON_ENABLE = (gc.getDefineINT("SAS_CV_INFO_SCREEN_TIMELINE_LOG_BUTTON_ENABLE") > 0)
IS_SAS_SHOW_LEGEND_LINK = (gc.getDefineINT("SAS_SHOW_LEGEND_LINK") > 0)

# <!-- custom: constants useful for numTxt under button placement in a grid-like manner, i got the idea to move them here rather to enhance reuse and remove redundance thanks to chatgpt general comment about them hehe thanks thanks chatgpt etc and me toot thanks; note: these are for non-multilist panels, commented-out if we don't need them but kept for reference still if may serve someday -->
#HYPOTHESIZED_FIRST_BUTTON_LEFT_PADDING = 9
#HYPOTHESIZED_INTER_BUTTON_SPACING = 4
# <!-- custom: "non-multilist panel" means simple horizontal panels where we place buttons directly with attachImageButton/setImageButtonAt and compute width manually.
# This is distinct from multilist panels (addMultiListControlGFC), which use their own layout constants below (HYPOTHESIZED_MULTI_LIST_*).
# Keep these constant families separate. (GPT-5.3-Codex) -->
# <!-- custom: edge padding is inferred from the classic 84px mini-panel that fits one 64px button: 84 - 64 = 20 total horizontal slack, so 10px per side.
# This keeps computed panel widths aligned with existing Sevopedia panel/button visuals. (GPT-5.3-Codex) -->
HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING = 10
HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING = 4
# <!-- custom: shared standard height for Sevopedia single-row non-multilist panels. (GPT-5.3-Codex) -->
NON_MULTILIST_PANEL_STANDARD_HEIGHT = 110
# <!-- custom: shared Y offset for history/background text panels that sit below an animation but have no panel header.
# Used by Bonus, Improvement, Building, Tech, Unit and the expandable text panel helper. (Claude code Sonnet 4.6) -->
H_ADJUST_Y_AFTER_ANIMATION_NO_HEADER = 22
# <!-- custom: Y offset to place the EXPAND button above a headerless animation panel.
# Smaller than H_ADJUST_Y_AFTER_ANIMATION_NO_HEADER because animation panels sit near the top of the pedia area, leaving less room above them. (Claude code Sonnet 4.6) -->
H_ADJUST_ANIMATION_NO_HEADER_EXPAND_BUTTON = 28
# <!-- custom: Y offset for the CLOSE button inside the expanded overlay header bar.
# Shared by both draw_expandable_text_panel and draw_expandable_content_panel_container. (Claude code Sonnet 4.6) -->
EXPANDED_OVERLAY_CLOSE_BUTTON_Y_OFFSET = 4
# <!-- custom: leaderhead expanded overlay: width = height * PCT / 100, keeping portrait proportions. Tune this value to taste.
# Empirically, 80 shows most of the animation height (head to torso/clothes) and most of the width (background included). (Claude code Sonnet 4.6) -->
EXPANDED_LEADERHEAD_WIDTH_PCT = 80
# <!-- custom: Y offset of the EXPAND button inside the collapsed leaderhead pane (placed inside the panel, not above it). (Claude code Sonnet 4.6) -->
LEADERHEAD_EXPAND_BUTTON_Y_OFFSET = -16
SAS_PEDIA_BG_NAME_PREFIXES = ("SCREEN_BG", "SCREEN_UN_BG", "MAINMENU_SLIDESHOW")
SAS_PEDIA_PANEL_STYLE_NAMES = None
SAS_PEDIA_PANEL_STYLE_VALUES = None
SAS_PEDIA_BG_NAMES = None
g_iSasPediaPanelStyleIdx = None
g_iSasPediaBgIdx = None
IS_SAS_PEDIA_PANEL_STYLE_PLAYGROUND_ENABLE = None
# <!-- custom: index 0 is reserved as a "(default)" sentinel with RGB None so draw_expandable_text_panel can skip the <color=...> wrap and leave any inline color tags inside szText intact. See KI#128 for the deferred-discovery rationale. (Claude code Opus 4.7) -->
SAS_PEDIA_TEXT_COLOR_NAMES = None
SAS_PEDIA_TEXT_COLOR_RGBS = None
g_iSasPediaTextColorIdx = None

def _sas_pedia_collect_bg_names():
    aNames = []
    iCount = gc.getNumInterfaceArtInfos()
    for iIdx in range(iCount):
        pInfo = gc.getInterfaceArtInfo(iIdx)
        if pInfo is None:
            continue
        # <!-- custom: keep this playground to DDS backgrounds only because draw_expandable_text_panel renders them with addDDSGFC.
        # MAINMENU_SCENE* art points to NIF/KFM scene files; cycling those through addDDSGFC empirically correlated with native EXE critical-section corruption/crashes on shutdown after browsing Sevopedia/style options. See KI#128. (GPT-5.5) -->
        sPath = pInfo.getPath().lower()
        if not sPath.endswith(".dds"):
            continue
        # <!-- custom: use getTag() rather than getType() because the Python binding for CvArtInfoInterface (CyInfoInterface3.cpp:348) inherits only from CvAssetInfoBase, which is bound as a standalone Python class without CvInfoBase as a base — so getType() is not accessible from Python on art infos even though it exists in C++.
        # getTag() and getType() return the same string per the C++ comment "'tag' is the same as 'type'" in CvInfo_Asset.h. (Claude code Opus 4.7) -->
        sName = pInfo.getTag()
        for sPrefix in SAS_PEDIA_BG_NAME_PREFIXES:
            if sName.startswith(sPrefix):
                aNames.append(sName)
                break
    aNames.sort()
    # <!-- custom: prepend "(none)" sentinel — index 0 means "skip the addDDSGFC call entirely" so we can preview a panel style with no background art behind it. (Claude code Opus 4.7) -->
    return tuple([u"(none)"] + aNames)

def _sas_pedia_collect_text_colors():
    # <!-- custom: no Python getNumColorInfos() binding exists, but CyGlobalContext::getColorInfo returns NULL once i is out of range, so we stop on the first None. (Claude code Opus 4.7) -->
    aNames = [u"(default)"]
    aRgbs = [None]
    iIdx = 0
    while True:
        pInfo = gc.getColorInfo(iIdx)
        if pInfo is None:
            break
        sType = pInfo.getType()
        if sType.startswith(u"COLOR_"):
            sLabel = sType[len(u"COLOR_"):]
        else:
            sLabel = sType
        # <!-- custom: NiColorA channels are floats in [0, 1] but the <color=...> tag expects 0..255. (Claude code Opus 4.7) -->
        kRgba = pInfo.getColor()
        aRgbs.append((int(kRgba.r * 255), int(kRgba.g * 255), int(kRgba.b * 255), int(kRgba.a * 255)))
        aNames.append(sLabel)
        iIdx += 1
    return (tuple(aNames), tuple(aRgbs))

def init_sas_pedia_playground_cache():
    # <!-- custom: lazy-init avoids import-time UI/DLL probing after an empirical native crash while cycling style/background options.
    # Define changes still require a game restart. See KI#128. (GPT-5.5 + Claude code Opus 4.7) -->
    global SAS_PEDIA_PANEL_STYLE_NAMES, SAS_PEDIA_PANEL_STYLE_VALUES, SAS_PEDIA_BG_NAMES
    global SAS_PEDIA_TEXT_COLOR_NAMES, SAS_PEDIA_TEXT_COLOR_RGBS
    global g_iSasPediaPanelStyleIdx, g_iSasPediaBgIdx, g_iSasPediaTextColorIdx
    global IS_SAS_PEDIA_PANEL_STYLE_PLAYGROUND_ENABLE
    if IS_SAS_PEDIA_PANEL_STYLE_PLAYGROUND_ENABLE is None:
        IS_SAS_PEDIA_PANEL_STYLE_PLAYGROUND_ENABLE = (gc.getDefineINT("SAS_PEDIA_PANEL_STYLE_PLAYGROUND_ENABLE") > 0)
    if not IS_SAS_PEDIA_PANEL_STYLE_PLAYGROUND_ENABLE:
        if SAS_PEDIA_PANEL_STYLE_NAMES is None:
            SAS_PEDIA_PANEL_STYLE_NAMES = ()
            SAS_PEDIA_PANEL_STYLE_VALUES = ()
            SAS_PEDIA_BG_NAMES = ()
            SAS_PEDIA_TEXT_COLOR_NAMES = ()
            SAS_PEDIA_TEXT_COLOR_RGBS = ()
            g_iSasPediaPanelStyleIdx = 0
            g_iSasPediaBgIdx = 0
            g_iSasPediaTextColorIdx = 0
        return
    if SAS_PEDIA_PANEL_STYLE_NAMES is None:
        # <!-- custom: auto-discover all PanelStyles enum values exposed by the DLL via boost::python (CvEnums.h PanelStyles).
        # The selected style is applied to the expanded overlay and shown in the header so it can be copied into target call sites.
        # Python 2.4 tuples have no .index(), so default lookup is a manual scan. (Claude code Opus 4.7 + GPT-5.5) -->
        SAS_PEDIA_PANEL_STYLE_NAMES = tuple(sorted(sName for sName in dir(PanelStyles) if sName.startswith("PANEL_STYLE_")))
        SAS_PEDIA_PANEL_STYLE_VALUES = tuple(getattr(PanelStyles, sName) for sName in SAS_PEDIA_PANEL_STYLE_NAMES)
        g_iSasPediaPanelStyleIdx = 0
        for _iIdx in range(len(SAS_PEDIA_PANEL_STYLE_NAMES)):
            if SAS_PEDIA_PANEL_STYLE_NAMES[_iIdx] == "PANEL_STYLE_MAIN":
                g_iSasPediaPanelStyleIdx = _iIdx
                break
    if SAS_PEDIA_BG_NAMES is None:
        # <!-- custom: companion playground for the underlying full-screen background DDS that an expanded overlay draws first.
        # Keep only plausible DDS backgrounds so the cycle list stays short and safe; NIF/KFM scene assets are not valid for addDDSGFC. (Claude code Opus 4.7 + GPT-5.5) -->
        SAS_PEDIA_BG_NAMES = _sas_pedia_collect_bg_names()
        g_iSasPediaBgIdx = 0
        for _iIdx in range(len(SAS_PEDIA_BG_NAMES)):
            if SAS_PEDIA_BG_NAMES[_iIdx] == "SCREEN_BG_OPAQUE":
                g_iSasPediaBgIdx = _iIdx
                break
    if SAS_PEDIA_TEXT_COLOR_NAMES is None:
        SAS_PEDIA_TEXT_COLOR_NAMES, SAS_PEDIA_TEXT_COLOR_RGBS = _sas_pedia_collect_text_colors()
        g_iSasPediaTextColorIdx = 0

def cycle_sas_pedia_panel_style(iDelta):
    # <!-- custom: small setter so SevoPediaMain (which uses `from _sevopedia_helpers import *`) can mutate the module global without falling into Python 2's local-binding trap on rebinding. (Claude code Opus 4.7) -->
    global g_iSasPediaPanelStyleIdx
    init_sas_pedia_playground_cache()
    iCount = len(SAS_PEDIA_PANEL_STYLE_VALUES)
    if iCount > 0:
        g_iSasPediaPanelStyleIdx = (g_iSasPediaPanelStyleIdx + iDelta) % iCount

def cycle_sas_pedia_background(iDelta):
    # <!-- custom: same Python-2-safe pattern as cycle_sas_pedia_panel_style. (Claude code Opus 4.7) -->
    global g_iSasPediaBgIdx
    init_sas_pedia_playground_cache()
    iCount = len(SAS_PEDIA_BG_NAMES)
    if iCount > 0:
        g_iSasPediaBgIdx = (g_iSasPediaBgIdx + iDelta) % iCount

def cycle_sas_pedia_text_color(iDelta):
    global g_iSasPediaTextColorIdx
    init_sas_pedia_playground_cache()
    iCount = len(SAS_PEDIA_TEXT_COLOR_NAMES)
    if iCount > 0:
        g_iSasPediaTextColorIdx = (g_iSasPediaTextColorIdx + iDelta) % iCount
# <!-- custom: some constants often used in sevopedia -->
LARGE_MARGIN = 20
MEDIUM_MARGIN = 15
SMALL_MARGIN = 10
X_ROTATION_ANIMATION = -20
Z_ROTATION_ANIMATION = 30
# <!-- custom: based on iIconFrameSize from sevopediaunit in base advciv code -->
PANE_ICON_SIZE = 64
PANE_ICON_FRAME_SIZE = 164
if PANE_ICON_SIZE > PANE_ICON_FRAME_SIZE:
	raise ValueError(u"[FATAL] PANE_ICON_SIZE=%d cannot be bigger/higher than PANE_ICON_FRAME_SIZE=%d, PANE_ICON_SIZE must fit within the frame, please adjust PANE_ICON_SIZE or PANE_ICON_FRAME_SIZE so that 0 < PANE_ICON_SIZE < PANE_ICON_FRAME_SIZE" % (PANE_ICON_SIZE, PANE_ICON_FRAME_SIZE))

# <!-- custom: for multilist panels -->
MULTILIST_BUTTON_SIZE = 64
MULTI_LIST_PANEL_OFFSET_X = 9
MULTI_LIST_PANEL_OFFSET_Y = 36
MULTI_LIST_PANEL_ADDITIONAL_W = -1 * (MULTI_LIST_PANEL_OFFSET_X * 2)
MULTI_LIST_PANEL_ADDITIONAL_H = -1 * (MULTI_LIST_PANEL_OFFSET_Y)
# <!-- custom: trial symmetric multilist side insets (10/10), keeping inter-spacing at 2.
# This keeps 4-button leader width at 282 and lets us verify if numTxt alignment remains stable. (GPT-5.3-Codex) -->
HYPOTHESIZED_MULTI_LIST_LEFT_EDGE_PADDING = 10
HYPOTHESIZED_MULTI_LIST_RIGHT_EDGE_PADDING = 10
# <!-- custom: backward-compat alias for existing placement code; maps to left edge padding. (GPT-5.3-Codex) -->
HYPOTHESIZED_MULTI_LIST_EDGE_PADDING = HYPOTHESIZED_MULTI_LIST_LEFT_EDGE_PADDING
# <!-- custom: inter-button spacing in multilist math (also used by numTxt placement).
# Verified by visual test: setting this to 3 made numTxt labels drift; 2 keeps labels aligned. (GPT-5.3-Codex) -->
HYPOTHESIZED_MULTI_LIST_INTER_BUTTON_SPACING = 2
# <!-- custom: note: below line not yet tested. -->
HYPOTHESIZED_MULTI_LIST_INTER_LINE_VERTICAL_SPACING = 4
# <!-- custom: global numTxt horizontal nudge for multilist labels.
# With symmetric 10/10 side insets, shifting labels 1px left visually re-centers most numTxt strings under buttons. (GPT-5.3-Codex) -->
MULTILIST_NUMTXT_GLOBAL_X_ADJUST = -1
# <!-- custom: when displaying only one row, adjust height to hide the 2nd row's buttons so it is prettier/clearer to read -->
HIDE_SECOND_ROW_MULTI_LIST = - 4
# Per documentation, the numLists parameter (7th) is actually number of columns
# Setting to 1 means the engine will auto-calculate how many buttons fit per row
# Using 1 for auto-calculation of buttons per row
SEVOPEDIA_MULTILIST_NUM_LISTS_AUTO_CALCULATE = 1
# Column index (always 0 when numLists=1)
SEVOPEDIA_MULTILIST_COLUMN_INDEX_AUTO = 0

# <!-- custom: wrap an asset description in Civ4's <link=literal>...</link> markup so addMultilineText renders it as a clickable link.
# When clicked, the engine calls SevoPediaMain.link(szLink), which reverse-looks-up the text through SAS_linkMatchDefs (techs, civics, buildings, units, promotions, specialists, features, improvements, bonuses, religions, corporations, traits, concepts, etc.) via each info's isMatchForLink(...) and pedia-jumps to the right page.
# No DLL change needed — this piggybacks on the mechanism already used by Sevopedia Building/Unit/Trait/Leader pages and by CvGameTextMgr for inline pedia links.
# Pass the exact localized description string (whatever info.getDescription() returned) so the reverse lookup finds a match. (Claude code Opus 4.7) -->
def make_pedia_link(szText):
	if not szText:
		return szText
	return u"<link=literal>%s</link>" % szText

def get_multilist_panel_width_for_buttons(iNumButtons, iButtonSize, iLeftEdgePadding, iRightEdgePadding, iInterButtonSpacing):
	# <!-- custom: width formula for a horizontal multilist panel:
	# width = (n * buttonSize) + leftEdgePadding + rightEdgePadding + ((n - 1) * interSpacing)
	# Example: n=4, button=64, left=10, right=10, spacing=2 => 256 + 20 + 6 = 282 px. (GPT-5.3-Codex) -->
	if iNumButtons <= 0:
		raise ValueError("get_multilist_panel_width_for_buttons requires iNumButtons > 0, got %d" % iNumButtons)
	return (iNumButtons * iButtonSize) + iLeftEdgePadding + iRightEdgePadding + ((iNumButtons - 1) * iInterButtonSpacing)

def get_panel_width_for_buttons(iNumButtons, iButtonSize, iEdgePadding, iInterButtonSpacing):
	# <!-- custom: width formula for a horizontal single-row button panel:
	# width = (n * buttonSize) + (2 * edgePadding) + ((n - 1) * interSpacing)
	# Example 1: n=1, button=64, edge=10, spacing=4 => 64 + 20 + 0 = 84 px.
	# Example 2: n=4, button=64, edge=10, spacing=4 => 256 + 20 + 12 = 288 px. (GPT-5.3-Codex) -->
	if iNumButtons <= 0:
		raise ValueError("get_panel_width_for_buttons requires iNumButtons > 0, got %d" % iNumButtons)
	return (iNumButtons * iButtonSize) + (2 * iEdgePadding) + ((iNumButtons - 1) * iInterButtonSpacing)

# <!-- custom: shared row helper for bonus-yields-style panels (one attached child panel per row, spacer + image button + label). (Claude code Opus 4.7) -->
def attach_button_label_row(screen, top, panelName, buttonPath, widgetType, widgetID1, widgetID2, szText):
	childPanelName = top.getNextWidgetName()
	screen.attachPanel(panelName, childPanelName, "", "", False, False, PanelStyles.PANEL_STYLE_EMPTY)
	screen.attachLabel(childPanelName, "", "  ")
	screen.attachImageButton(childPanelName, "", buttonPath, GenericButtonSizes.BUTTON_SIZE_CUSTOM, widgetType, widgetID1, widgetID2, False)
	screen.attachLabel(childPanelName, "", SASTextScale.titleText(szText))

# <!-- custom: Broad Sevopedia leader-exclusion list for non-AIP leader/civ association views such as Traits, Improvements, Religions, Civics, and groupings.
# LEADER_BARBARIAN is excluded because it's not a playable/selectable leader.
# LEADER_DEFAULTS is not listed because it has no index (it's an XML template).
# Keep this separate from get_aip_excluded_leader_types() in ai_utils_shared_with_civ4.py, which is shared by the runtime AIP cache and workflow predump checker. (ChatGPT-5.5) -->
EXCLUDED_LEADER_TYPES_FROM_SEVOPEDIA = ("LEADER_BARBARIAN",)

# <!-- custom: favorite leader type selectors for get_favorite_leader_counts. (GPT-5.2-Codex) -->
FAVORITE_LEADER_TYPE_RELIGION = 0
FAVORITE_LEADER_TYPE_CIVIC = 1

# <!-- custom: Sevopedia Chart Helpers (shared across Handicap/GameSpeed/WorldSize/Era charts). (Claude code Opus 4.5) -->
#
# <!-- custom: Common constants for chart tables -->
CHART_TABLE_MARGIN = 4
CHART_TABLE_ROW_H = 15
CHART_TABLE_W_ICON = 24
CHART_TABLE_STYLE = TableStyles.TABLE_STYLE_STANDARD
# <!-- custom: All known enum prefixes to strip.
# Grouped here for simplicity across all charts. -->
CHART_ENUM_PREFIXES = ("TECH_", "HANDICAP_", "GOODY_", "GAMESPEED_", "ERA_", "WORLDSIZE_")
# <!-- custom: # Stable icon sorting (fixes "emoji order changes / ties shuffle")
#
# Civ4 table sorting uses the raw cell text. If multiple rows share the same icon,
# tie ordering can shuffle between ascending/descending clicks.
# We fix ties by appending an invisible sort key (icon_group + row_index) to the icon cell text.
# IMPORTANT: We avoid ASCII control chars (0x01..0x1F) here. Some Civ4 builds can fail to render
# *any* glyph in a table cell if such chars are present. This was the root cause of:
#   "buttons show, but GameFont glyphs (food/citizen/gold/etc) are blank".
# Instead we use Unicode zero-width/formatting marks (U+200B..U+200F), which are invisible
# and safe to include in Civ4's UI strings. (GPT-5.2-Codex + ChatGPT-5.2 Thinking) -->
CHART_SORT_DIGITS = (u"\u200b", u"\u200c", u"\u200d", u"\u200e", u"\u200f")  # 5 invisible marks
def get_leaders_index_to_type_map():
	# Returns a dictionary mapping each leader index (int) to its string type (e.g., "LEADER_GANDHI").
	# Excluded leaders (like BARBARIAN) must be filtered by caller if needed.

	leaders_indexes_to_types = {}
	for iLeader in range(gc.getNumLeaderHeadInfos()):
		leader_type = gc.getLeaderHeadInfo(iLeader).getType()
		leaders_indexes_to_types[iLeader] = leader_type
	return leaders_indexes_to_types

def get_leader_index_from_leader_type(leader_type):
	# Given a leader_type string (e.g. "LEADER_ALEXANDER"), return its iLeader index.
	# Raises ValueError if not found. Compatible with Python 2.4 and Civ4 DLL.
	# <!-- custom: note: LEADER_DEFAULTS doesn't seem to have a leader index at all, so don't use it for this function -->

	for iLeader in range(gc.getNumLeaderHeadInfos()):
		current_leader_type = gc.getLeaderHeadInfo(iLeader).getType()
		if current_leader_type == leader_type:
			return iLeader

	raise ValueError("[FATAL] leader_type=%s not found in gc.getLeaderHeadInfo list. Note: this can also happen but not only in particular with LEADER_DEFAULTS, which does not seem to have a leader index at all in gc of base advciv code, that we use as well in our/this mod very similarly if not identically, so make sure to address the edge case of LEADER_DEFAULTS in another way to not get caught/stuck in this edge case error message in particular, possibly for other leaders as well." % leader_type)

def get_leader_index_safe(leader_type):
	# Returns leader index for a leader_type string, or -1 if not found.
	# Unlike get_leader_index_from_leader_type, this does not raise - useful for
	# exclusion lists that may include non-indexed types like LEADER_DEFAULTS.
	for iLeader in range(gc.getNumLeaderHeadInfos()):
		if gc.getLeaderHeadInfo(iLeader).getType() == leader_type:
			return iLeader
	return -1

def get_leader_indexes_from_leader_types(leader_types):
	# Given a list/tuple of leader types, return tuple of valid leader indexes.
	# Silently skips types that don't exist (e.g. LEADER_DEFAULTS has no index).
	result = []
	for leader_type in leader_types:
		idx = get_leader_index_safe(leader_type)
		if idx != -1:
			result.append(idx)
	return tuple(result)

def get_excluded_leader_indexes(leader_types):
	return get_leader_indexes_from_leader_types(leader_types)

def get_real_leader_maps_and_count(excluded_leader_types):
	# Returns (leaderIds, leaderToCiv, numRealLeaders).
	excluded_leader_indexes = set(get_leader_indexes_from_leader_types(excluded_leader_types))
	leader_ids = []
	leader_to_civ = {}
	num_real_leaders = 0

	for iLeader in range(gc.getNumLeaderHeadInfos()):
		if iLeader in excluded_leader_indexes:
			continue
		iCivForLeader = -1
		for iCiv in range(gc.getNumCivilizationInfos()):
			if gc.getCivilizationInfo(iCiv).isLeaders(iLeader):
				iCivForLeader = iCiv
				break
		if iCivForLeader == -1:
			continue
		num_real_leaders += 1
		leader_ids.append(iLeader)
		leader_to_civ[iLeader] = iCivForLeader

	return leader_ids, leader_to_civ, num_real_leaders

# <!-- custom: Build leaders header text with X/Y (Z%) (example "Leaders 12/53 (22%)") for any Sevopedia panel. (GPT-5.2-Codex (summarized)) -->
def format_leaders_header_text(num_with, total, headerLabel):
	percent = 0
	if total > 0:
		percent = (100 * num_with) / total
	return u"%s %d/%d (%d%%)" % (headerLabel, num_with, total, percent,)

# <!-- custom: Count leaders whose favorite type matches iFavoriteId; favoriteType is "RELIGION" or "CIVIC". (GPT-5.2-Codex) -->
def get_favorite_leader_counts(favoriteType, iFavoriteId, excluded_leader_types):
	leader_ids, unused_leader_to_civ, total_real_leaders = get_real_leader_maps_and_count(excluded_leader_types)
	num_with_favorite = 0
	for iLeader in leader_ids:
		leaderInfo = gc.getLeaderHeadInfo(iLeader)
		if favoriteType == FAVORITE_LEADER_TYPE_RELIGION:
			if leaderInfo.getFavoriteReligion() == iFavoriteId:
				num_with_favorite += 1
		elif favoriteType == FAVORITE_LEADER_TYPE_CIVIC:
			if leaderInfo.getFavoriteCivic() == iFavoriteId:
				num_with_favorite += 1
		else:
			raise ValueError("favoriteType=%d is not supported; use FAVORITE_LEADER_TYPE_RELIGION or FAVORITE_LEADER_TYPE_CIVIC" % favoriteType)
	return num_with_favorite, total_real_leaders

def get_leader_type_from_leader_index(iLeader):
	# Given an iLeader index (e.g. 1), return the leader_type string (e.g. "LEADER_ALEXANDER").
	# Raises IndexError if the index is out of bounds.
	# Compatible with Python 2.4 and Civ4 DLL.
	# <!-- custom: some entries like LEADER_DEFAULTS may not exist at any valid iLeader index and must be handled elsewhere -->

	if iLeader < 0 or iLeader >= gc.getNumLeaderHeadInfos():
		raise IndexError("[FATAL] iLeader=%d is out of bounds for gc.getNumLeaderHeadInfos()=%d. This might indicate a misindexed value or mod bug." % (iLeader, gc.getNumLeaderHeadInfos()))

	return gc.getLeaderHeadInfo(iLeader).getType()

def check_overlapping_keys_between_dicts(d1, d2):
	# Raises ValueError if any key exists in both d1 and d2.
	overlap = []
	for k in d1:
		if k in d2:
			overlap.append(k)
	if overlap:
		raise ValueError("Overlapping keys between dictionaries: %s" % str(overlap))

def get_feature_production_modifier_techs():
	techModifiers = []
	for iTech in xrange(gc.getNumTechInfos()):
		iModifier = gc.getTechInfo(iTech).getFeatureProductionModifier()
		if iModifier != 0:
			techModifiers.append((iTech, iModifier))
	return techModifiers

# <!-- custom: added with the help of chatgpt 5.2 thanks to help separate Land features in sevopedia into as of now Land (Removable) and Land (Other) -->
# A feature is "removable" if there exists a Build that removes it (e.g. Chop Forest, Clear Jungle, Remove Fallout).
# We do this via CvBuildInfo because CvFeatureInfo in our DLL does not expose getRemoveTech().
def SAS_isFeatureRemovable(iFeature):
	for iBuild in range(gc.getNumBuildInfos()):
		buildInfo = gc.getBuildInfo(iBuild)
		if not buildInfo or buildInfo.isGraphicalOnly():
			continue

		# In BtS/AdvCiv-style DLLs, isFeatureRemove is per-feature (takes iFeature).
		# If a modmod changes this API, it should be adjusted here (centralized).
		if buildInfo.isFeatureRemove(iFeature):
			# Some builds may have 0 time for some features in some mods, so we treat any "remove" as removable.
			return True

	return False

def get_concept_id(concept_type):
	# <!-- custom: Find the concept ID, for example for concept_type "CONCEPT_CITIES" (LLM) -->
	for i in range(gc.getNumConceptInfos()):
		if gc.getConceptInfo(i).getType() == concept_type:
			return i

	return -1

def place_new_concept_legend_link(top, new_concept_type):
	if not IS_SAS_SHOW_LEGEND_LINK:
		return
	iConcept = getNewConceptID(new_concept_type)
	if iConcept < 0:
		return
	screen = top.getScreen()
	# <!-- custom: sasFontTagTitle is fine for the legend link here: Sevopedia pages are roomy and the link sits with other footer/header page chrome. (Claude code Opus 4.7; GPT-5.5) -->
	szLegendText = sasFontTagTitle + localText.getText("TXT_KEY_PEDIA_SAS_LEGEND_LINK_SHORT", ()) + SAS_FONT_TAG_CLOSE
	screen.setText(top.getNextWidgetName(), "Background", szLegendText, CvUtil.FONT_LEFT_JUSTIFY, top.SAS_getFooterLegendX(), top.Y_BOT_PANEL + 16, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_DESCRIPTION, CivilopediaPageTypes.CIVILOPEDIA_PAGE_CONCEPT_NEW, iConcept)

def draw_none_text(screen, selfTop, panelX, panelY, panelW, panelH, txtKey=None):
	# <!-- custom: place a centered "None" (or custom-keyed) message inside a panel that has no buttons/content to show.
	# Replaces the ~5-line addMultilineText boilerplate scattered across pedia files. (Claude code Opus 4.7) -->
	if txtKey is None:
		txtKey = "TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_NONE"

	textName = selfTop.getNextWidgetName()
	szText = localText.getText(txtKey, ())
	yPanelCenter = panelY + (panelH / 2)
	screen.addMultilineText(textName, SASTextScale.labelText(szText), panelX + 7, yPanelCenter, panelW - 14, panelH - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

def draw_expandable_text_panel(screen, top, panelTitle, panelX, panelY, panelW, panelH, szText, bExpanded, iPythonWidgetData1, iCollapsedTextYOffset=None):
	# <!-- custom: reusable Sevopedia expandable text panel helper (collapsed panel + opaque full-page overlay). Use WIDGET_PYTHON routing via iPythonWidgetData1. data2=1 expands, data2=0 closes. (GPT-5.3-Codex) -->
	if iCollapsedTextYOffset is None:
		if panelTitle:
			iCollapsedTextYOffset = H_ADJUST_Y_AFTER_ANIMATION_NO_HEADER
		else:
			iCollapsedTextYOffset = 0
	iCloseButtonW = 85
	iExpandButtonW = 107
	if bExpanded:
		iOverlayX = top.X_PEDIA_PAGE
		iOverlayY = top.Y_PEDIA_PAGE
		iOverlayW = top.R_PEDIA_PAGE - top.X_PEDIA_PAGE
		iOverlayH = top.B_PEDIA_PAGE - top.Y_PEDIA_PAGE
		init_sas_pedia_playground_cache()

		# <!-- custom: pick the underlying full-screen background DDS from the playground cache; index 0 is the "(none)" sentinel which means "skip drawing a background entirely" so a transparent panel style reveals whatever is behind the pedia screen.
		# When the playground is off, fall back to the previous hardcoded SCREEN_BG_OPAQUE so default behavior is identical to before this feature. (Claude code Opus 4.7) -->
		if IS_SAS_PEDIA_PANEL_STYLE_PLAYGROUND_ENABLE and len(SAS_PEDIA_BG_NAMES) > 0:
			sBgArtKey = SAS_PEDIA_BG_NAMES[g_iSasPediaBgIdx]
		else:
			sBgArtKey = u"SCREEN_BG_OPAQUE"
		if sBgArtKey != u"(none)":
			screen.addDDSGFC(top.getNextWidgetName(), ArtFileMgr.getInterfaceArtInfo(sBgArtKey).getPath(), iOverlayX, iOverlayY, iOverlayW, iOverlayH, WidgetTypes.WIDGET_GENERAL, -1, -1)
		# <!-- custom: when the caller passed no title, use "(Background)" so the expanded overlay always has a header bar for the CLOSE button to sit on. (Claude code Sonnet 4.6) -->
		if panelTitle:
			expandedTitle = panelTitle
		else:
			expandedTitle = u"(" + localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ()) + u")"
		panelName = top.getNextWidgetName()
		# <!-- custom: read the current playground style from the module-level cache so the overlay reflects the user's last PREV/NEXT click; falls back to PANEL_STYLE_MAIN when the playground is disabled or the cache is somehow empty (defensive, not expected). (Claude code Opus 4.7) -->
		if IS_SAS_PEDIA_PANEL_STYLE_PLAYGROUND_ENABLE and len(SAS_PEDIA_PANEL_STYLE_VALUES) > 0:
			ePanelStyle = SAS_PEDIA_PANEL_STYLE_VALUES[g_iSasPediaPanelStyleIdx]
		else:
			ePanelStyle = PanelStyles.PANEL_STYLE_MAIN
		screen.addPanel(panelName, expandedTitle, "", True, True, iOverlayX, iOverlayY, iOverlayW, iOverlayH, ePanelStyle)
		screen.setButtonGFC(top.getNextWidgetName(), SASTextScale.labelText(u"CLOSE"), "", iOverlayX + iOverlayW - (iCloseButtonW + 10), iOverlayY + EXPANDED_OVERLAY_CLOSE_BUTTON_Y_OFFSET, iCloseButtonW, 26, WidgetTypes.WIDGET_PYTHON, iPythonWidgetData1, 0, ButtonStyles.BUTTON_STYLE_STANDARD)
		# <!-- custom: panel style playground header strip: "[label] [PREV] [NEXT] [CLOSE]" sits on the same row as CLOSE inside the header bar.
		# iData2 is the signed cycle delta (-1 / +1); a separate widget id (SAS_MAGIC_PEDIA_PYTHON_PANEL_STYLE_CYCLE) keeps the existing iData2 dispatch on HISTORY_EXPAND (0=close, 1=expand) untouched.
		# The label uses RIGHT_JUSTIFY so it grows leftward without us having to measure the variable-length style name width. (Claude code Opus 4.7) -->
		if IS_SAS_PEDIA_PANEL_STYLE_PLAYGROUND_ENABLE and len(SAS_PEDIA_PANEL_STYLE_VALUES) > 0:
			iCycleBtnW = 28
			iCycleBtnSpacing = 6
			iCycleY = iOverlayY + EXPANDED_OVERLAY_CLOSE_BUTTON_Y_OFFSET
			iNextX = iOverlayX + iOverlayW - (iCloseButtonW + 10) - iCycleBtnSpacing - iCycleBtnW
			iPrevX = iNextX - iCycleBtnSpacing - iCycleBtnW
			iLabelRightX = iPrevX - iCycleBtnSpacing
			# <!-- custom: align the label baseline with the buttons' inner text by sitting just inside the header bar's top edge; +4 puts it visually level with the CLOSE/-/+ button labels. (Claude code Opus 4.7); when upscaled, a small - 2 offset allows to see text entirely without overflowing vertically onto the top of the text panel -->
			iLabelY = iCycleY - 2
			szStyleLabel = SASTextScale.labelText(u"STYLE: %s (%d/%d)" % (SAS_PEDIA_PANEL_STYLE_NAMES[g_iSasPediaPanelStyleIdx], g_iSasPediaPanelStyleIdx + 1, len(SAS_PEDIA_PANEL_STYLE_NAMES)))
			screen.setText(top.getNextWidgetName(), "Background", szStyleLabel, CvUtil.FONT_RIGHT_JUSTIFY, iLabelRightX, iLabelY, 0, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			# <!-- custom: use "-" / "+" rather than "<" / ">" because angle brackets are parsed as font/markup tag delimiters by the Civ4 text renderer (see agents.md), which breaks the button label rendering. (Claude code Opus 4.7) -->
			screen.setButtonGFC(top.getNextWidgetName(), SASTextScale.labelText(u"-"), "", iPrevX, iCycleY, iCycleBtnW, 26, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_PANEL_STYLE_CYCLE, -1, ButtonStyles.BUTTON_STYLE_STANDARD)
			screen.setButtonGFC(top.getNextWidgetName(), SASTextScale.labelText(u"+"), "", iNextX, iCycleY, iCycleBtnW, 26, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_PANEL_STYLE_CYCLE, 1, ButtonStyles.BUTTON_STYLE_STANDARD)
			# <!-- custom: background DDS playground row, sits one row ABOVE the style row (the user said "we have plenty room up" so it lives in the gutter above the panel header).
			# Same x columns as the style row so the two stacked groups align visually.
			# Routing uses a separate widget id (SAS_MAGIC_PEDIA_PYTHON_BACKGROUND_CYCLE) so the iData2 cycle delta does not collide with the style row's dispatch. (Claude code Opus 4.7) -->
			if len(SAS_PEDIA_BG_NAMES) > 0:
				iBgRowYBtn = iCycleY - 30
				iBgRowYLabel = iLabelY - 30
				szBgLabel = SASTextScale.labelText(u"BG: %s (%d/%d)" % ( SAS_PEDIA_BG_NAMES[g_iSasPediaBgIdx], g_iSasPediaBgIdx + 1, len(SAS_PEDIA_BG_NAMES)))
				screen.setText(top.getNextWidgetName(), "Background", szBgLabel, CvUtil.FONT_RIGHT_JUSTIFY, iLabelRightX, iBgRowYLabel, 0, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				screen.setButtonGFC(top.getNextWidgetName(), SASTextScale.labelText(u"-"), "", iPrevX, iBgRowYBtn, iCycleBtnW, 26, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_BACKGROUND_CYCLE, -1, ButtonStyles.BUTTON_STYLE_STANDARD)
				screen.setButtonGFC(top.getNextWidgetName(), SASTextScale.labelText(u"+"), "", iNextX, iBgRowYBtn, iCycleBtnW, 26, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_BACKGROUND_CYCLE, 1, ButtonStyles.BUTTON_STYLE_STANDARD)
			iTextRowYBtn = iCycleY - 60
			iTextRowYLabel = iLabelY - 60
			tColorPreview = SAS_PEDIA_TEXT_COLOR_RGBS[g_iSasPediaTextColorIdx]
			if tColorPreview is None:
				szColorCode = u"-"
			else:
				szColorCode = u"%d,%d,%d,%d" % tColorPreview
			szTextColorLabel = SASTextScale.labelText(u"TEXT: %s (%s) (%d/%d)" % (SAS_PEDIA_TEXT_COLOR_NAMES[g_iSasPediaTextColorIdx], szColorCode, g_iSasPediaTextColorIdx + 1, len(SAS_PEDIA_TEXT_COLOR_NAMES)))
			screen.setText(top.getNextWidgetName(), "Background", szTextColorLabel, CvUtil.FONT_RIGHT_JUSTIFY, iLabelRightX, iTextRowYLabel, 0, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			screen.setButtonGFC(top.getNextWidgetName(), SASTextScale.labelText(u"-"), "", iPrevX, iTextRowYBtn, iCycleBtnW, 26, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_TEXT_COLOR_CYCLE, -1, ButtonStyles.BUTTON_STYLE_STANDARD)
			screen.setButtonGFC(top.getNextWidgetName(), SASTextScale.labelText(u"+"), "", iNextX, iTextRowYBtn, iCycleBtnW, 26, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_TEXT_COLOR_CYCLE, 1, ButtonStyles.BUTTON_STYLE_STANDARD)
		# <!-- custom: <color=R,G,B,A> wrap syntax matches BUG/RawYields.py:246. (Claude code Opus 4.7) -->
		szTextForOverlay = szText
		if IS_SAS_PEDIA_PANEL_STYLE_PLAYGROUND_ENABLE and len(SAS_PEDIA_TEXT_COLOR_RGBS) > 0:
			tColor = SAS_PEDIA_TEXT_COLOR_RGBS[g_iSasPediaTextColorIdx]
			if tColor is not None:
				szTextForOverlay = u"<color=%d,%d,%d,%d>%s</color>" % (tColor[0], tColor[1], tColor[2], tColor[3], szText)
		screen.addMultilineText(top.getNextWidgetName(), SASTextScale.labelText(szTextForOverlay), iOverlayX + 10, iOverlayY + 40, iOverlayW - 20, iOverlayH - 50, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		return 1

	panelName = top.getNextWidgetName()
	screen.addPanel(panelName, panelTitle, "", True, True, panelX, panelY, panelW, panelH, PanelStyles.PANEL_STYLE_BLUE50)
	# <!-- custom: when the panel has no title, Civ4 renders no header bar so the button at panelY+6 would overlap the content.
	# Place it just above the panel instead. (Claude code Sonnet 4.6) -->
	iExpandButtonY = panelY + 6
	if not panelTitle:
		iExpandButtonY -= H_ADJUST_Y_AFTER_ANIMATION_NO_HEADER
	screen.setButtonGFC(top.getNextWidgetName(), SASTextScale.labelText(u"EXPAND"), "", panelX + panelW - (iExpandButtonW + 8), iExpandButtonY, iExpandButtonW, 24, WidgetTypes.WIDGET_PYTHON, iPythonWidgetData1, 1, ButtonStyles.BUTTON_STYLE_STANDARD)
	# <!-- custom: note: in some panels replace old code's screen.attachMultilineText with screen.addMultilineText to allow padding for text -->
	screen.addMultilineText(top.getNextWidgetName(), SASTextScale.labelText(szText), panelX + 7, panelY + 10 + iCollapsedTextYOffset, panelW - 5, panelH - (10 + iCollapsedTextYOffset) - 25, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
	return 0

def draw_expandable_content_panel_container(screen, top, panelTitle, panelX, panelY, panelW, panelH, bExpanded, iPythonWidgetData1, iPythonReloadData1):
	# <!-- custom: reusable expandable panel container helper for non-text content (animations, 3D previews, etc.).
	# Draws expand/collapse chrome and returns content rect (x, y, w, h).
	# Always shows a bracketed title when none given, mirroring draw_expandable_text_panel. (Claude code Sonnet 4.6 + GPT-5.3-Codex) -->
	iCloseButtonW = 85
	iReloadButtonW = 106
	iExpandButtonW = 107
	# <!-- custom: for the expanded overlay, use a bracketed fallback title so it always has a visible header bar; for collapsed, keep panelTitle as-is (animation panels look cleaner without a header). (Claude code Sonnet 4.6) -->
	if panelTitle:
		expandedTitle = panelTitle
	else:
		expandedTitle = u"(Content)"

	if bExpanded:
		iOverlayX = top.X_PEDIA_PAGE
		iOverlayY = top.Y_PEDIA_PAGE
		iOverlayW = top.R_PEDIA_PAGE - top.X_PEDIA_PAGE
		iOverlayH = top.B_PEDIA_PAGE - top.Y_PEDIA_PAGE
		screen.addDDSGFC(top.getNextWidgetName(), ArtFileMgr.getInterfaceArtInfo("SCREEN_BG_OPAQUE").getPath(), iOverlayX, iOverlayY, iOverlayW, iOverlayH, WidgetTypes.WIDGET_GENERAL, -1, -1)
		panelName = top.getNextWidgetName()
		screen.addPanel(panelName, expandedTitle, "", True, True, iOverlayX, iOverlayY, iOverlayW, iOverlayH, PanelStyles.PANEL_STYLE_MAIN)
		screen.setButtonGFC(top.getNextWidgetName(), SASTextScale.labelText(u"RELOAD"), "", iOverlayX + iOverlayW - (iCloseButtonW + 10 + iReloadButtonW), iOverlayY + EXPANDED_OVERLAY_CLOSE_BUTTON_Y_OFFSET, iReloadButtonW, 26, WidgetTypes.WIDGET_PYTHON, iPythonReloadData1, -1, ButtonStyles.BUTTON_STYLE_STANDARD)
		screen.setButtonGFC(top.getNextWidgetName(), SASTextScale.labelText(u"CLOSE"), "", iOverlayX + iOverlayW - (iCloseButtonW + 10), iOverlayY + EXPANDED_OVERLAY_CLOSE_BUTTON_Y_OFFSET, iCloseButtonW, 26, WidgetTypes.WIDGET_PYTHON, iPythonWidgetData1, 0, ButtonStyles.BUTTON_STYLE_STANDARD)
		return (iOverlayX + 10, iOverlayY + 40, iOverlayW - 20, iOverlayH - 50)

	panelName = top.getNextWidgetName()
	screen.addPanel(panelName, panelTitle, "", True, True, panelX, panelY, panelW, panelH, PanelStyles.PANEL_STYLE_BLUE50)
	iExpandButtonY = panelY + 6
	if not panelTitle:
		iExpandButtonY -= H_ADJUST_ANIMATION_NO_HEADER_EXPAND_BUTTON
	screen.setButtonGFC(top.getNextWidgetName(), SASTextScale.labelText(u"EXPAND"), "", panelX + panelW - (iExpandButtonW + 8), iExpandButtonY, iExpandButtonW, 24, WidgetTypes.WIDGET_PYTHON, iPythonWidgetData1, 1, ButtonStyles.BUTTON_STYLE_STANDARD)
	if panelTitle:
		return (panelX + 4, panelY + 30, panelW - 8, panelH - 35)
	return (panelX, panelY, panelW, panelH)

def draw_expandable_leaderhead_panel(screen, top, panelX, panelY, panelW, panelH, leaderX, leaderY, leaderW, leaderH, bExpanded, iPythonWidgetData1):
	# <!-- custom: expandable panel helper for the leaderhead.
	# Like draw_expandable_content_panel_container but has no RELOAD button (leaderhead re-renders only on attitude/action button clicks, not on reload — use the attitude/action buttons in the right column instead) and reserves a right column for those buttons.
	# Returns ((lhX, lhY, lhW, lhH), (attX, attY, attW, attH)).
	# Collapsed returns the passed leader coords unchanged. (Claude code Sonnet 4.6) -->
	iCloseButtonW = 85
	iAttColW = 260
	iExpandButtonW = 107
	if bExpanded:
		iOverlayX = top.X_PEDIA_PAGE
		iOverlayY = top.Y_PEDIA_PAGE
		iOverlayW = top.R_PEDIA_PAGE - top.X_PEDIA_PAGE
		iOverlayH = top.B_PEDIA_PAGE - top.Y_PEDIA_PAGE
		screen.addDDSGFC(top.getNextWidgetName(), ArtFileMgr.getInterfaceArtInfo("SCREEN_BG_OPAQUE").getPath(), iOverlayX, iOverlayY, iOverlayW, iOverlayH, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.addPanel(top.getNextWidgetName(), u"(Content)", "", True, True, iOverlayX, iOverlayY, iOverlayW, iOverlayH, PanelStyles.PANEL_STYLE_MAIN)
		screen.setButtonGFC(top.getNextWidgetName(), SASTextScale.labelText(u"CLOSE"), "", iOverlayX + iOverlayW - (iCloseButtonW + 10), iOverlayY + EXPANDED_OVERLAY_CLOSE_BUTTON_Y_OFFSET, iCloseButtonW, 26, WidgetTypes.WIDGET_PYTHON, iPythonWidgetData1, 0, ButtonStyles.BUTTON_STYLE_STANDARD)
		# attitude/action column: right-aligned, same width as old RELOAD+CLOSE combined
		iAttX = iOverlayX + iOverlayW - (iAttColW + 10)
		iAttW = iAttColW
		iAttY = iOverlayY + EXPANDED_OVERLAY_CLOSE_BUTTON_Y_OFFSET + 26 + 4
		iAttH = NON_MULTILIST_PANEL_STANDARD_HEIGHT
		# leaderhead: full height, width capped by ratio to preserve portrait proportions
		iLhX = iOverlayX + 10
		iLhY = iOverlayY + 40
		iLhH = iOverlayH - 50
		iLhW = iLhH * EXPANDED_LEADERHEAD_WIDTH_PCT / 100
		return (iLhX, iLhY, iLhW, iLhH), (iAttX, iAttY, iAttW, iAttH)
	screen.addPanel(top.getNextWidgetName(), u"", "", True, True, panelX, panelY, panelW, panelH, PanelStyles.PANEL_STYLE_BLUE50)
	screen.setButtonGFC(top.getNextWidgetName(), SASTextScale.labelText(u"EXPAND"), "", panelX + panelW - (iExpandButtonW + 8), panelY + LEADERHEAD_EXPAND_BUTTON_Y_OFFSET, iExpandButtonW, 24, WidgetTypes.WIDGET_PYTHON, iPythonWidgetData1, 1, ButtonStyles.BUTTON_STYLE_STANDARD)
	return (leaderX, leaderY, leaderW, leaderH), (0, 0, 0, 0)

def get_concept_widgetType_widgetID1_widgetID2(conceptID, widgetTypes, civilopediaPageTypes):
	if conceptID != -1:
		# For concepts, use WIDGET_PEDIA_DESCRIPTION with proper page type and ID
		widgetType = widgetTypes.WIDGET_PEDIA_DESCRIPTION
		widgetID1 = civilopediaPageTypes.CIVILOPEDIA_PAGE_CONCEPT
		widgetID2 = conceptID
		return widgetType, widgetID1, widgetID2
	else:
		# Default to WIDGET_GENERAL with no click action
		widgetType = widgetTypes.WIDGET_GENERAL
		widgetID1 = -1
		widgetID2 = -1
		return widgetType, widgetID1, widgetID2

def get_numTxt_attack_defense_modifiers(iModAttack, iModDefense):
	# Handle class modifiers with x/y format
	# Use %+d to always show the sign (+ or -)
	if iModAttack != 0 and iModDefense != 0:
		# Both attack and defense: x/y format
		return "%+d/%+d" % (iModAttack, iModDefense)
	elif iModAttack != 0:
		# Only attack: x/* format
		return "%+d/_" % (iModAttack)
	elif iModDefense != 0:
		# Only defense: */y format
		return "_/%+d" % (iModDefense)
	else:
		return "_/_"

def get_numTxt_combat_type_modifiers(iModCombat):
	return "%+d%%" % iModCombat

def get_numTxt_promotion_tier_compact(prefix, bHasTier1, bHasTier2, bHasTier3):
	tierIds = []
	if bHasTier1:
		tierIds.append(1)
	if bHasTier2:
		tierIds.append(2)
	if bHasTier3:
		tierIds.append(3)
	if len(tierIds) == 0:
		return ""
	if len(tierIds) == 1:
		return "%s%d" % (prefix, tierIds[0])

	iMin = min(tierIds)
	iMax = max(tierIds)
	bContiguous = (len(tierIds) == (iMax - iMin + 1))
	if bContiguous:
		return "%s%d-%d" % (prefix, iMin, iMax)

	szText = "%s%d" % (prefix, tierIds[0])
	for iTier in tierIds[1:]:
		szText += "+%d" % iTier
	return szText

def get_numTxt_num_free_bonus_or_random_map(iNumFreeBonuses):
	# <!-- custom: note: for freebonus, done according to kujira's website, in https://gforestshade.github.io/kujira/post/civ4buildinginfos/#inumfreebonuses (translated to english with google chrome), see also "for freebonus, done according to kujira's website" note/code comment at top of sevopedia building py file if needed: based on this, displaying free bonus if >= 1 or if == -1, adjusting display depending on this -->
	if iNumFreeBonuses == -1:
		return "FBBR"
	elif iNumFreeBonuses >= 1:
		return "%d" % (iNumFreeBonuses)
	else:
		raise ValueError("[FATAL] Unexpected iNumFreeBonuses=%d value out of bounds of iNumFreeBonuses == -1 or iNumFreeBonuses >=1, please verify the code and iNumFreeBonuses are behaving as intended and adjust this sevopedia code or your mod code based on this as you want/prefer." % iNumFreeBonuses)

def get_extra_correction_x(numTxt):
	# <!-- custom: shorter custom abbreviations (InC, ACs, AUC, AC N+R, etc.) looked slightly left after shortening, so nudge them right without changing generic modifier labels. (GPT-5.3-Codex) -->
	if numTxt in ("InC", "ACs", "AUC"):
		return -3
	if numTxt in ("(!)Cvc", "FBBR"):
		return -2
	if numTxt.startswith("AC ") and numTxt.endswith("+R"):
		return -4

	if len(numTxt) <= 3:
		# <!-- custom: example "_/_", "1", etc -->
		return -6
	elif len(numTxt) == 4:
		# <!-- custom: example "+50%" -->
		return -6
	elif len(numTxt) == 5:
		# <!-- custom: example "+50/_", "+100%", etc -->
		return -7
	elif len(numTxt) == 6:
		return -8
	else:
		# <!-- custom: example "+25/+25" -->
		return -8

def get_extra_correction_x_inbetween_buttons(button_size):
	# <!-- custom: offset numTxt from the current button center into the gap before it, for connector labels such as "or" that describe the relationship between adjacent buttons rather than either button itself.
	# The small extra nudge keeps the connector visually centered in the gap. (GPT-5.5) -->
	extraCorrectionOffFromTheCenter = -3

	return (-1 * (int(button_size / 2))) + extraCorrectionOffFromTheCenter

def get_multilist_max_buttons_per_row(panelWidth, buttonsize):
	totalButtonWidth = buttonsize + HYPOTHESIZED_MULTI_LIST_INTER_BUTTON_SPACING

	return int((panelWidth - MULTI_LIST_PANEL_OFFSET_X) / totalButtonWidth)

def add_multilist_numTxt_under_button(multiListX, multiListY, extraCorrectionX, iButtonIndex, button_size, maxButtonsPerRow, numTxt, screen, selfTop, widgetType, font):
	textName = selfTop.getNextWidgetName()
	buttonColumn = iButtonIndex % maxButtonsPerRow
	buttonRow = iButtonIndex // maxButtonsPerRow

	# <!-- custom: note: since we start at 0 in this nice system chatgpt and claude ai if i may say helped me design and adjust thanks, we don't need to handle the 1th no spacing with next item of the list, as anything multilied by 0 negates spacing, this applies to both column and row calculation(s); however we just add a X correction to start not at leftmost part of the button anyways but instead at center/middle of button to place our first and onwards numTxT -->
	startAtMiddleOfButtonCorrectionX = +1 * (int(button_size / 2))

	# <!-- custom: note: in this code, it seems we are still slightly off vs an ideally centered label, i don't know what the exact cause is, but maybe we can use this as a parameter to control more precisely label positioning based on/depending on numTxt and such as we prefer (center more or less aggressively depending on whether numTxt is expected to be long (like "+25/+100" for example) vs short (for example"+25%") in trying it as such, so we add a tiny bit of in this case extra x correction (extraCorrectionX) etc -->
	textX = multiListX + HYPOTHESIZED_MULTI_LIST_EDGE_PADDING + startAtMiddleOfButtonCorrectionX + extraCorrectionX + ((buttonColumn - 1) * button_size) + ((buttonColumn - 1) * HYPOTHESIZED_MULTI_LIST_INTER_BUTTON_SPACING) + MULTILIST_NUMTXT_GLOBAL_X_ADJUST
	iLabelFont = getSASUIFontLabel()
	if iLabelFont > 2:
		textX += (-3 * (iLabelFont - 2))

	# <!-- custom: similarly to extraCorrectionX, we are slightly off for some reason here so adjust Y position of the numTxt multine text, but since this is the same for all buttons unlike in extraCorrectionX (i.e. regardless of numTxt length), then it is fine i think to hardcode it here for convenience and efficiency at least i want to do so hopefully convenient or helpful or not or yes or etc to do so -->
	extraCorrectionY = -9
	textY = multiListY + button_size + extraCorrectionY + (buttonRow * button_size) + (buttonRow * HYPOTHESIZED_MULTI_LIST_INTER_LINE_VERTICAL_SPACING)

	textW = 2 * button_size
	textH = 30

	szNumTxt = sasFontTagLabel + numTxt + SAS_FONT_TAG_CLOSE
	screen.addMultilineText(textName, szNumTxt, textX, textY, textW, textH, widgetType, -1, -1, font)

def add_multilist_connector_numTxt_before_button(multiListX, multiListY, iButtonIndex, button_size, maxButtonsPerRow, numTxt, screen, selfTop, widgetType, font):
	# <!-- custom: draw a connector numTxt before the current button, e.g. "A or B" for OR prereqs/obsolete alternatives.
	# It deliberately skips the first button and row-start buttons because there is no previous same-row button to connect to; callers should use normal add_multilist_numTxt_under_button when the text labels one specific icon instead. (GPT-5.5) -->
	if not numTxt:
		return
	if iButtonIndex <= 0:
		return
	if (iButtonIndex % maxButtonsPerRow) == 0:
		return
	add_multilist_numTxt_under_button(multiListX, multiListY, get_extra_correction_x_inbetween_buttons(button_size), iButtonIndex, button_size, maxButtonsPerRow, numTxt, screen, selfTop, widgetType, font)

def chart_font2(szText):
	# Wrap chart text using SAS dynamic scaling instead of hardcoded <font=2>.
	return sasFontTagLabel + unicode(szText) + SAS_FONT_TAG_CLOSE

def chart_encode_base5(iValue, iDigits):
	# Encode an integer as a base-5 string using invisible Unicode chars.
	out = []
	for _ in xrange(iDigits):
		out.append(CHART_SORT_DIGITS[iValue % 5])
		iValue //= 5
	out.reverse()
	return u"".join(out)

def chart_sort_key(iGroup, iRowIndex):
	# Generate an invisible sort key for stable icon column sorting.
	# 4 base-5 digits cover up to 624, enough for our "group" numbering (<= 140).
	# 3 digits cover up to 124 rows.
	return u"<font=1>" + chart_encode_base5(iGroup, 4) + chart_encode_base5(iRowIndex, 3) + u"</font>"

def chart_beautify_field_name(raw_name):
	# Convert camelCase/underscore field names to readable labels.
	# Strips leading 'i' or 'b' prefix, replaces _ with spaces, splits camelCase, replaces Percent with %.
	name = raw_name
	if name.startswith("i") and len(name) > 1 and name[1].isupper():
		name = name[1:]
	if name.startswith("b") and len(name) > 1 and name[1].isupper():
		name = name[1:]
	name = re.sub(r"_", " ", name)
	name = re.sub(r"([a-z])([A-Z])", r"\1 \2", name)
	if name.endswith("Percent"):
		name = name[:-len("Percent")] + "%"
	return name

def chart_beautify_enum_name(raw_name):
	# Convert ENUM_STYLE_NAMES to Title Case Labels.
	# Strips known prefixes, replaces _ with spaces, converts to title case.
	name = raw_name
	for prefix in CHART_ENUM_PREFIXES:
		if name.startswith(prefix):
			name = name[len(prefix):]
			break
	name = re.sub(r"_", " ", name)
	return name.title()

def chart_get_info_display_name(info):
	# <!-- custom: Use XML/text descriptions for player-facing chart labels so custom punctuation/wording is preserved; missing text should be fixed in XML/text, not hidden by enum-name fallback. (ChatGPT-5.5) -->
	return info.getDescription()

def chart_chunk_list(items, size):
	# Split a list into chunks of given size. Returns list of lists.
	if size <= 0:
		return [items]
	chunks = []
	current = []
	for item in items:
		current.append(item)
		if len(current) >= size:
			chunks.append(current)
			current = []
	if current:
		chunks.append(current)
	return chunks

def chart_format_tech_list(value, return_list, none_text, abbrev_tech_names=None):
	# Parse a comma-separated tech list string into formatted output.
	# value: comma-separated string of tech names/types
	# return_list: if True, return list; if False, return joined string
	# none_text: text to return/use when list is empty
	# abbrev_tech_names: optional dict of {full_name: abbreviated_name}
	if not value:
		if return_list:
			return []
		return none_text
	parts = value.split(",")
	out = []
	for part in parts:
		p = part.strip()
		if not p:
			continue
		# Beautify if it looks like an enum (e.g. TECH_AGRICULTURE)
		if p.startswith("TECH_"):
			p = chart_beautify_enum_name(p)
		# Apply abbreviations if provided
		if abbrev_tech_names and p in abbrev_tech_names:
			p = abbrev_tech_names[p]
		out.append(p)
	if return_list:
		return out
	if len(out) == 0:
		return none_text
	return ", ".join(out)

def chart_add_csv_log_button(screen, top, x, y, w, xPos=None, yPos=None):
	if not IS_SAS_CV_INFO_SCREEN_TIMELINE_LOG_BUTTON_ENABLE:
		return ""

	# <!-- custom: mirror Timeline LOG sizing: keep compact default, slightly enlarge only when label font is upscaled (4) so "LOG" remains readable. (GPT-5.3-Codex) -->
	if getSASUIFontLabel() > 3:
		chart_log_button_w = 66
		chart_log_button_h = 32
	else:
		chart_log_button_w = 52
		chart_log_button_h = 28
	chart_log_button_margin_x = 10
	chart_log_button_margin_y = 8
	chart_log_button_footer_bottom_padding = 9

	widget = top.getNextWidgetName()
	label = sasFontTagLabel + localText.getText("TXT_KEY_SAS_INFO_TIMELINE_LOG_BUTTON", ()).upper() + SAS_FONT_TAG_CLOSE
	if xPos is None:
		if hasattr(top, "X_EXIT"):
			xPos = top.X_EXIT - chart_log_button_w - 90
		else:
			xPos = x + w - chart_log_button_w - chart_log_button_margin_x
	if yPos is None:
		if hasattr(top, "Y_BOT_PANEL") and hasattr(top, "H_BOT_PANEL"):
			yPos = top.Y_BOT_PANEL + top.H_BOT_PANEL - chart_log_button_h - chart_log_button_footer_bottom_padding
		else:
			yPos = y + chart_log_button_margin_y
	# <!-- custom: use WIDGET_PYTHON + data1 routing for chart log clicks because function-name matching can fail on generated widget IDs in Sevopedia; this mirrors the robust Music/Movie click pattern in SevoPediaMain.handleInput. (GPT-5.3-Codex) -->
	screen.setButtonGFC(widget, label, "", xPos, yPos, chart_log_button_w, chart_log_button_h, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_CHART_LOG, -1, ButtonStyles.BUTTON_STYLE_STANDARD)
	return widget

def chart_dump_table_csv(log_tag, table_data):
	if not table_data:
		print("%s_BEGIN" % log_tag)
		print("%s_EMPTY" % log_tag)
		print("%s_END" % log_tag)
		return

	print("%s_BEGIN" % log_tag)
	for row in table_data:
		cells = []
		for cell in row:
			cells.append(_chart_to_csv_cell(_chart_clean_cell_text(cell)))
		print(u",".join(cells).encode("latin-1", "replace"))
	print("%s_END" % log_tag)

def _chart_clean_cell_text(cell):
	if isinstance(cell, tuple):
		text = cell[0]
	else:
		text = cell
	if text is None:
		text = u""
	text = unicode(text)
	text = re.sub(r"</?font[^>]*>", u"", text)
	text = re.sub(r"</?color[^>]*>", u"", text)
	for digit in CHART_SORT_DIGITS:
		text = text.replace(digit, u"")
	text = text.replace(u"\r", u" ").replace(u"\n", u" ")
	return text.strip()

def _chart_to_csv_cell(text):
	if text.find(u'"') != -1:
		text = text.replace(u'"', u'""')
	if text.find(u",") != -1 or text.find(u'"') != -1 or text.find(u"\n") != -1:
		return '"' + text + '"'
	return text

# <!-- custom: In-Category Chart Helpers (InChart) - shared across Traits, Tech, Improvement charts.
# These are charts that appear WITHIN a category item page (e.g. leader pairing table in Traits), as opposed to "Chart as category" pages like Handicap Chart where the chart IS the category.
# Centralized here to reduce code duplication and ensure consistent styling. (Claude Opus 4.5) -->

# <!-- custom: In-Category Chart Constants -->
INCHART_ICON_SIZE = 24              # Icon button size in table cells (leaders, civs, techs)
INCHART_ICON_SPACING = 2            # Spacing between icon columns
INCHART_ROW_HEIGHT = 24             # Table row height
INCHART_TABLE_MARGIN = 8            # Margin around table inside panel
INCHART_TABLE_INNER = 6             # Inner padding for tables
INCHART_MIN_ICON_COL_WIDTH = 16     # Minimum icon column width after scaling

def inchart_calc_icon_col_width(tableW, fixedColsW, maxIcons, iconSize=None, iconSpacing=None):
	# Calculate icon column width, scaling down if necessary to fit.
	# tableW: total table width
	# fixedColsW: sum of fixed column widths (non-icon columns)
	# maxIcons: maximum number of icon columns needed
	# iconSize: icon size (defaults to INCHART_ICON_SIZE)
	# iconSpacing: spacing between icons (defaults to INCHART_ICON_SPACING)
	# Returns: (iconColW, maxIcons) - column width and adjusted max icons (at least 1)
	if iconSize is None:
		iconSize = INCHART_ICON_SIZE
	if iconSpacing is None:
		iconSpacing = INCHART_ICON_SPACING

	if maxIcons < 1:
		maxIcons = 1

	iconColW = iconSize + iconSpacing
	availableW = tableW - fixedColsW

	if availableW < iconColW * maxIcons:
		iconColW = max(INCHART_MIN_ICON_COL_WIDTH, int(availableW / maxIcons))

	return iconColW, maxIcons

def inchart_set_icon_column_headers(screen, tableName, startCol, numCols, colWidth):
	# Set empty column headers for icon columns (common pattern in all in-category charts).
	# screen: CyGInterfaceScreen
	# tableName: name of the table widget
	# startCol: column index to start at
	# numCols: number of icon columns to add
	# colWidth: width for each column
	for iCol in xrange(numCols):
		screen.setTableColumnHeader(tableName, startCol + iCol, "", colWidth)

def inchart_calc_ranking_bar(value, minVal, maxVal, maxPlus=5):
	# Calculate ranking bar string (+ symbols) for a value within a range.
	# value: the value to rank
	# minVal: minimum value in range
	# maxVal: maximum value in range
	# maxPlus: maximum number of + symbols (default 5)
	# Returns: unicode string of + symbols
	if maxVal > minVal:
		normalized = (value - minVal) * 100 / (maxVal - minVal)
	else:
		normalized = 50

	# Convert normalized (0-100) to plus count (0-maxPlus)
	# Formula: (normalized + 10) / 20 gives 0-5 range for normalized 0-100
	numPlus = (normalized + 10) / (100 / maxPlus)
	if numPlus < 0:
		numPlus = 0
	if numPlus > maxPlus:
		numPlus = maxPlus

	return u"+" * numPlus

# <!-- custom: Icon type constants for inchart_set_icon_cells -->
INCHART_ICON_TYPE_LEADER = 0
INCHART_ICON_TYPE_CIV = 1
INCHART_ICON_TYPE_TECH = 2

def inchart_set_icon_cells(screen, tableName, row, itemIds, colStart, colCount, iconType, extraData=None):
	# Generic helper to set icon cells in a table row.
	# screen: CyGInterfaceScreen
	# tableName: name of the table widget
	# row: row index
	# itemIds: list of item IDs (leader IDs, civ IDs, or tech IDs)
	# colStart: column index to start placing icons
	# colCount: total number of icon columns (fills empty cells if itemIds is shorter)
	# iconType: INCHART_ICON_TYPE_LEADER, INCHART_ICON_TYPE_CIV, or INCHART_ICON_TYPE_TECH
	# extraData: dict with extra data (e.g. {"leaderToCiv": {...}} for leaders)
	#
	# For leaders: extraData should contain "leaderToCiv" mapping iLeader -> iCiv
	# For civs/techs: extraData is not required

	# <!-- custom: In-category chart icon cells are standalone image columns, so give them the same invisible text sort key as full chart-category icon cells. (GPT-5.5) -->
	for iCol in xrange(colCount):
		if iCol >= len(itemIds):
			# Empty cell
			screen.setTableText(tableName, colStart + iCol, row, chart_sort_key(0, row), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			continue

		itemId = itemIds[iCol]
		buttonPath = ""
		widgetType = WidgetTypes.WIDGET_GENERAL
		widgetData1 = -1
		widgetData2 = -1

		if iconType == INCHART_ICON_TYPE_LEADER:
			leaderInfo = gc.getLeaderHeadInfo(itemId)
			buttonPath = leaderInfo.getButton()
			if extraData:
				leaderToCiv = extraData.get("leaderToCiv", {})
			else:
				leaderToCiv = {}
			iCiv = leaderToCiv.get(itemId, -1)
			if iCiv != -1 and buttonPath:
				widgetType = WidgetTypes.WIDGET_PEDIA_JUMP_TO_LEADER
				widgetData1 = itemId
				widgetData2 = iCiv
			else:
				buttonPath = ""

		elif iconType == INCHART_ICON_TYPE_CIV:
			civInfo = gc.getCivilizationInfo(itemId)
			buttonPath = civInfo.getButton()
			if buttonPath:
				widgetType = WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIV
				widgetData1 = itemId
				widgetData2 = -1

		elif iconType == INCHART_ICON_TYPE_TECH:
			techInfo = gc.getTechInfo(itemId)
			buttonPath = techInfo.getButton()
			if buttonPath:
				widgetType = WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH
				widgetData1 = itemId
				widgetData2 = -1

		if buttonPath:
			screen.setTableText(tableName, colStart + iCol, row, chart_sort_key(itemId + 1, row), buttonPath, widgetType, widgetData1, widgetData2, CvUtil.FONT_LEFT_JUSTIFY)
		else:
			screen.setTableText(tableName, colStart + iCol, row, chart_sort_key(0, row), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
