# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from CvPythonExtensions import *
import CvUtil
from SASFontUtils import sasFontTagLabel, sasFontTagTitle, SAS_FONT_TAG_CLOSE

gc = CyGlobalContext()
localText = CyTranslator()

# <!-- custom: lazy-init cache for SAS_SHOW_LEGEND_LINK GlobalDefine.
# Cannot be resolved at module import time because SASUtils is imported before CyGlobalContext finishes loading XML defines (eager init returns False and the legend link silently disappears).
# Resolved on first use via _isLegendLinkEnabled, matching the lazy-init-to-None approach used by other AdvCiv-SAS advisors. (Claude code Opus 4.7) -->
_IS_SAS_SHOW_LEGEND_LINK = None
def _isLegendLinkEnabled():
	global _IS_SAS_SHOW_LEGEND_LINK
	if _IS_SAS_SHOW_LEGEND_LINK is None:
		_IS_SAS_SHOW_LEGEND_LINK = (gc.getDefineINT("SAS_SHOW_LEGEND_LINK") > 0)
	return _IS_SAS_SHOW_LEGEND_LINK

_IS_SAS_ADVISOR_ALLOW_VASSAL_PERSPECTIVE = None
def _isAdvisorVassalPerspectiveEnabled():
	global _IS_SAS_ADVISOR_ALLOW_VASSAL_PERSPECTIVE
	if _IS_SAS_ADVISOR_ALLOW_VASSAL_PERSPECTIVE is None:
		_IS_SAS_ADVISOR_ALLOW_VASSAL_PERSPECTIVE = (gc.getDefineINT("SAS_ADVISOR_ALLOW_VASSAL_PERSPECTIVE") > 0)
	return _IS_SAS_ADVISOR_ALLOW_VASSAL_PERSPECTIVE

# <advc.077>
# <!-- custom: shared thousands formatter used by Info Screen and Victory Screen.
# The separator is passed by the caller so callers can use XML text, spaces, dots, or another context-specific separator without hidden module state. (Claude Opus 4.5 + GPT-5.5) -->
def separateThousands(iValue, szSeparator):
	# The rest of the function is adopted from this StackOverflow answer by Nadia Alramli: https://stackoverflow.com/posts/1823189/revisions
	s = '%d' % iValue
	groups = []
	while s and s[-1].isdigit():
		groups.append(s[-3:])
		s = s[:-3]
	return s + szSeparator.join(reversed(groups))
# </advc.077>

# <!-- custom: Shared exact times-100 formatter: omit redundant .00 while retaining nonzero hundredths without approximation.
# First used by the Culture Breakdown. See KI#1063. (GPT-5.6-Sol) -->
def formatExactHundredths(iValue):
	szSign = u""
	if iValue < 0:
		szSign = u"-"
	iAbsValue = abs(iValue)
	if iAbsValue % 100 == 0:
		return u"%s%d" % (szSign, iAbsValue / 100)
	return u"%s%d.%02d" % (szSign, iAbsValue / 100, iAbsValue % 100)

# <!-- custom: shared advisor layout constants that are screen-independent (safe to read in __init__); runtime screen-dependent geometry is derived per screen from current resolution in interfaceScreen. (GPT-5.3-Codex) -->
# <!-- custom: as part of upscaling text code changes, now that commerce sliders are on the right-side, reduce left-space (from 172).
# Increase right-space for scoreboard (from 390) to give more room for anarchy and golden age button and just in case here.
# Increase top space (from 28). -->
SAS_ADVISOR_LEFT_SPACE_FOR_COMMERCE_SLIDERS = 0
SAS_ADVISOR_RIGHT_SPACE_FOR_SCOREBOARD = 420
SAS_ADVISOR_TOP_SPACE_FOR_TECH_BAR = 32
SAS_ADVISOR_BOTTOM_SPACE = 0

# <!-- custom: shared advisor title Y is screen-independent and unified for migrated advisors to reduce per-file noise; older per-screen values were close (e.g. 8/12), so use one midpoint default (10).
# Keep screen-dependent anchors computed at runtime. (GPT-5.3-Codex) -->
SAS_ADVISOR_TITLE_Y = 10
# <!-- custom: shared offsets/divisors for runtime screen-dependent anchor formulas (title/exit/link/footer) derived from each advisor's current panel width/height in interfaceScreen. (GPT-5.3-Codex) -->
SAS_ADVISOR_TITLE_X_DIVISOR = 2
SAS_ADVISOR_EXIT_X_OFFSET = 30
SAS_ADVISOR_EXIT_Y_OFFSET = 42
SAS_ADVISOR_BOTTOM_PANEL_Y_OFFSET = 55

# <!-- custom: shared runtime advisor bounds (screen-dependent) and shared title/exit/link anchors; callers pass screen and configured margins from __init__. (GPT-5.3-Codex) -->
def getAdvisorRuntimeBounds(screen, iLeftSpace, iRightSpace, iTopSpace, iBottomSpace):
	iXScreen = iLeftSpace
	iWScreen = screen.getXResolution() - iLeftSpace - iRightSpace
	iYScreen = iTopSpace
	iHScreen = screen.getYResolution() - iTopSpace - iBottomSpace
	return (iXScreen, iYScreen, iWScreen, iHScreen)

def getAdvisorRuntimeAnchors(iWScreen, iHScreen):
	iXTitle = iWScreen / SAS_ADVISOR_TITLE_X_DIVISOR
	iXExit = iWScreen - SAS_ADVISOR_EXIT_X_OFFSET
	iYExit = iHScreen - SAS_ADVISOR_EXIT_Y_OFFSET
	iYLink = iYExit
	iYBottomPanel = iHScreen - SAS_ADVISOR_BOTTOM_PANEL_Y_OFFSET
	return (iXTitle, iXExit, iYExit, iYLink, iYBottomPanel)

# <!-- custom: shared "maximized" panel/table layout for advisor tabs that want to fill the area between top and bottom bars (Domestic Overview tabs, Military Advisor Battles tab).
# The Y bleed lets the panel slide a few pixels under the top/bottom bars to hide thin seams; the inner table margin and top visual adjust account for TABLE_STYLE_STANDARD drawing the header slightly above its anchor.
# Returns two (x, y, w, h) tuples so callers can unpack directly into addPanel / addTableControlGFC. (GPT-5.3-Codex + Claude code Opus 4.7) -->
def getAdvisorMaximizedPanelLayout(iWScreen, iYBottomPanel):
	iMainPanelYBleed = 10
	iMainPanelX = 0
	iMainPanelY = 55 - iMainPanelYBleed
	iMainPanelW = iWScreen
	iMainPanelH = (iYBottomPanel - iMainPanelY) + iMainPanelYBleed
	iTableMargin = 8
	iTableTopVisualAdjust = 6
	iTableX = iMainPanelX + iTableMargin
	iTableY = iMainPanelY + iTableMargin + iTableTopVisualAdjust
	iTableW = iMainPanelW - 2 * iTableMargin
	iTableH = iMainPanelH - 2 * iTableMargin - iTableTopVisualAdjust
	return ((iMainPanelX, iMainPanelY, iMainPanelW, iMainPanelH), (iTableX, iTableY, iTableW, iTableH))

# <!-- custom: shared weighted tab-link width helper used by advisor tab bars; weights by text width and scales to runtime X_EXIT. (GPT-5.3-Codex) -->
def getAdvisorRuntimeLinkWidths(cyInterface, aszLabels, szExitLabel, iXExit):
	aiLabelWidths = []
	for szLabel in aszLabels:
		aiLabelWidths.append(cyInterface.determineWidth(szLabel) + 20)
	iTotalWidth = sum(aiLabelWidths) + cyInterface.determineWidth(szExitLabel) + 20
	aiLinkWidths = []
	for iWidth in aiLabelWidths:
		aiLinkWidths.append((iXExit * iWidth + iTotalWidth/2) / iTotalWidth)
	return aiLinkWidths

# <!-- custom: shared footer-tab renderer for advisor screens.
# Use non-bold title-sized links like Foreign/Info/Sevopedia footer navigation.
# Each row is (widget, label, width, page_id); page_id can be a simple 0/1/2 tab id, a Victory screen id, or a Foreign SCREEN_DICT id, so oddball advisors still use the same reliable pattern.
# Active tabs are colored yellow and emitted as inert WIDGET_GENERAL, -1/-1 labels.
# Use this helper only for screens where active-tab clicks can safely do nothing; inactive tabs keep routing through the caller-supplied ids. (GPT-5.5 + GPT-5.5-Thinking review) -->
def drawAdvisorFooterTabRows(screen, aTabRows, iActivePageID, iYLink, iZ, iColorYellow, eInactiveWidget=WidgetTypes.WIDGET_GENERAL, iInactiveData2=-1, iXStart=0):
	iX = iXStart
	for iPage in range(len(aTabRows)):
		szWidgetID, szLabel, iWidth, iData1 = aTabRows[iPage]
		eWidget = eInactiveWidget
		iData2 = iInactiveData2
		if iData1 == iActivePageID:
			szLabel = localText.changeTextColor(szLabel, iColorYellow)
			eWidget = WidgetTypes.WIDGET_GENERAL
			iData1 = -1
			iData2 = -1
		screen.setText(szWidgetID, "", sasFontTagTitle + szLabel + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, iX + iWidth / 2, iYLink, iZ, FontTypes.TITLE_FONT, eWidget, iData1, iData2)
		iX += iWidth

def drawAdvisorFooterTabs(screen, aWidgetIDs, aLabels, aiWidths, iActivePageID, iYLink, iZ, iColorYellow, eInactiveWidget=WidgetTypes.WIDGET_GENERAL, aiInactiveData1=None, iInactiveData2=-1, iXStart=0):
	aTabRows = []
	for iPage in range(len(aLabels)):
		iData1 = iPage
		if aiInactiveData1 is not None:
			iData1 = aiInactiveData1[iPage]
		aTabRows.append((aWidgetIDs[iPage], aLabels[iPage], aiWidths[iPage], iData1))
	drawAdvisorFooterTabRows(screen, aTabRows, iActivePageID, iYLink, iZ, iColorYellow, eInactiveWidget, iInactiveData2, iXStart)

# <!-- custom: invisible sort key for advisor icon-only table cells. GameFont chars (religion, attitude, trade icons, etc.) already sort as text; this is only for separate icon-button cells where the visible text is otherwise empty. Usually skip this when an icon button is immediately followed by matching text in the next column (e.g. Info Screen Wonders), because that text column already gives the useful sort path. Keep hardcoded font=1 unscaled so the sort key stays invisible and does not affect row height. Pattern based on Sevopedia chart icon sorting. (GPT-5.5) -->
def getAdvisorIconSortKey(iGroup, iRow):
	aDigits = (u"\u200b", u"\u200c", u"\u200d", u"\u200e", u"\u200f")
	aParts = []
	iValue = int(iGroup)
	for _ in xrange(4):
		aParts.insert(0, aDigits[iValue % 5])
		iValue = iValue / 5
	iValue = int(iRow)
	for _ in xrange(6):
		aParts.insert(4, aDigits[iValue % 5])
		iValue = iValue / 5
	return u"<font=1>" + u"".join(aParts) + u"</font>"

# <!-- custom: shared debug/vassal player dropdown for advisors.
# Debug mode keeps each legacy advisor's MAX_PLAYERS vs MAX_CIV_PLAYERS and active-row preselection behavior.
# Outside debug, selected advisors may inspect only the current active player plus alive vassals of the active player's team, not a hardcoded human slot; vassals are subordinate enough to make their domestic/world/policy state relevant, but this remains perspective-only UI access and does not reveal unrelated players or hidden map data. (GPT-5.5) -->
def addAdvisorDebugDropdown(screen, szDropdownName, iActivePlayer, bIncludeBarbarians=False, bSelectActive=True, iX=22, iY=12, iW=300, eFont=FontTypes.GAME_FONT, bAllowVassalPerspective=False):
	aiPlayers = getAdvisorPerspectivePlayerIDs(bIncludeBarbarians, bAllowVassalPerspective)
	if len(aiPlayers) <= 0:
		return False
	bDebug = CyGame().isDebugMode()
	screen.addDropDownBoxGFC(szDropdownName, iX, iY, iW, WidgetTypes.WIDGET_GENERAL, -1, -1, eFont)
	for iPlayer in aiPlayers:
		screen.addPullDownString(szDropdownName, gc.getPlayer(iPlayer).getName(), iPlayer, iPlayer, iPlayer == iActivePlayer and (bSelectActive or not bDebug))
	return True

def getAdvisorPerspectivePlayerIDs(bIncludeBarbarians=False, bAllowVassalPerspective=False):
	if CyGame().isDebugMode():
		aiPlayers = []
		iMaxPlayers = gc.getMAX_CIV_PLAYERS()
		if bIncludeBarbarians:
			iMaxPlayers = gc.getMAX_PLAYERS()
		for iPlayer in range(iMaxPlayers):
			if gc.getPlayer(iPlayer).isAlive():
				aiPlayers.append(iPlayer)
		return aiPlayers
	if not bAllowVassalPerspective or not _isAdvisorVassalPerspectiveEnabled():
		return []
	iActivePlayer = CyGame().getActivePlayer()
	if iActivePlayer < 0 or not gc.getPlayer(iActivePlayer).isAlive():
		return []
	iActiveTeam = gc.getPlayer(iActivePlayer).getTeam()
	aiPlayers = [iActivePlayer]
	for iPlayer in range(gc.getMAX_CIV_PLAYERS()):
		if iPlayer != iActivePlayer and gc.getPlayer(iPlayer).isAlive() and gc.getTeam(gc.getPlayer(iPlayer).getTeam()).isVassal(iActiveTeam):
			aiPlayers.append(iPlayer)
	if len(aiPlayers) <= 1:
		return []
	return aiPlayers

def getAdvisorValidPerspectivePlayer(iPlayer, bIncludeBarbarians=False, bAllowVassalPerspective=False):
	if iPlayer in getAdvisorPerspectivePlayerIDs(bIncludeBarbarians, bAllowVassalPerspective):
		return iPlayer
	return CyGame().getActivePlayer()

def isAdvisorReadOnlyPerspective(iPlayer):
	return not CyGame().isDebugMode() and iPlayer != CyGame().getActivePlayer()

def getAdvisorDebugDropdownSelectedPlayer(screen, szDropdownName):
	iIndex = screen.getSelectedPullDownID(szDropdownName)
	return screen.getPullDownData(szDropdownName, iIndex)

# <!-- custom: shared strict XML lookup helper for maps/screens; raise loudly instead of silently accepting missing tags. (GPT-5.3-Codex) -->
# <!-- custom: handle for example PROMOTION_GUERILLA1 now being renamed to PROMOTION_HILLS_MASTER1, so summoning wrong asset for example as is done in sevopedia bonus's placeRelevantUnits panel as of now should raise an error not silently pass; also useful to access any asset id safely such as hills or peak terrains 's id, or hills's button for example too; is also useful to detect and signal loudly errors such as using wrong "TERRAIN_FOREST" as part of copy pasting terrain code into features code of the placeUnits method there as of now instead of "FEATURE_FOREST", and we get a nice error instead of what i assume would be a silent pass; done with the help of chatgpt thanks -->
def getInfoTypeOrFail(tag):
	iType = gc.getInfoTypeForString(tag)
	if iType == -1:
		raise ValueError("Missing XML tag: '%s'" % tag)
	return iType

# <!-- custom: strict variant of CvUtil.findInfoTypeNum for older Python callsites that pass info getter/count functions instead of using gc.getInfoTypeForString directly.
# Keep missing static XML tags loud like getInfoTypeOrFail. (GPT-5.5) -->
def findInfoTypeNumOrFail(infoGetter, numInfosGetter, tag):
	iType = CvUtil.findInfoTypeNum(infoGetter, numInfosGetter, tag)
	if iType == -1:
		raise ValueError("Missing XML tag: '%s'" % tag)
	return iType

# <!-- custom: shared helper to resolve NewConcept IDs by XML type (e.g. "CONCEPT_SAS_SCORE_TAB_COLUMNS" for the Score-tab Legend clickable Sevopedia/NewConcept entry).
# Returns -1 when missing so callers can skip optional links safely. (GPT-5.3-Codex) -->
def getNewConceptID(szConceptType):
	for i in range(gc.getNumNewConceptInfos()):
		if gc.getNewConceptInfo(i).getType() == szConceptType:
			return i
	return -1

# <!-- custom: shared advisor "Legend" link helper, factored from CvInfoScreen's Score-tab pattern so any AdvCiv-SAS advisor can drop a Sevopedia legend link with one call.
# Advisor legend links use smaller sasFontTagLabel rather than Sevopedia's footer-sized sasFontTagTitle because an in-game screen link should stay quiet.
# Caller passes the screen object (must expose getScreen, getNextWidgetName, Z_CONTROLS), the NewConcept XML type, and the (X,Y) anchor in screen coords.
# Skips silently when the SAS_SHOW_LEGEND_LINK GlobalDefine is off or the concept is missing.
# setText params mirror the Score-tab call exactly: WIDGET_PEDIA_DESCRIPTION + CIVILOPEDIA_PAGE_CONCEPT_NEW + concept ID is the pedia jump that actually resolves through SevoPediaMain.pediaJump. (Claude code Opus 4.7 + GPT-5.5) -->
def placeAdvisorLegendLink(top, szConceptType, iX, iY, eJustify=None):
	if not _isLegendLinkEnabled():
		return
	iConcept = getNewConceptID(szConceptType)
	if iConcept < 0:
		return
	if eJustify is None:
		eJustify = CvUtil.FONT_RIGHT_JUSTIFY
	screen = top.getScreen()
	szLabel = sasFontTagLabel + "Legend" + SAS_FONT_TAG_CLOSE
	screen.setText(top.getNextWidgetName(), "Background", szLabel, eJustify, iX, iY, top.Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_DESCRIPTION, CivilopediaPageTypes.CIVILOPEDIA_PAGE_CONCEPT_NEW, iConcept)
