## Sid Meier's Civilization 4 - Copyright Firaxis Games 2005
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: note: for comparison with Base AdvCiv purposes, if needed, as of now we also store a single-line version of Base AdvCiv's CvMainInterface.py.
# See: /LLM_Helpers/examples/CvMainInterface_1_12_singleline.py and [the corresponding readme section] (/LLM_Helpers/README.md#comparison-with-base-advciv-112s-main-interface-processed-similarly) -->
from CvPythonExtensions import *
# <advc.092>
from RectLayout import *
from LayoutDict import *
# Will import this one locally
del globals()["gSetScaleFactors"]
# </advc.092>
import CvUtil
import ScreenInput
import CvScreenEnums
from SASFontUtils import *
from SASUtils import formatExactHundredths, getInfoTypeOrFail
# <!-- custom: in main interface, use LABEL as the base text font instead of BODY because it is more readable. (GPT-5.3-Codex) -->
import CvEventInterface
import time
import BugDll # BUG - DLL
# BUG - Options - start
import BugCore
import BugOptions
import BugOptionsScreen
import BugPath
import BugUtil
import CityUtil
ClockOpt = BugCore.game.NJAGC
ScoreOpt = BugCore.game.Scores
MainOpt = BugCore.game.MainInterface
CityScreenOpt = BugCore.game.CityScreen
# BUG - Options - end
import ReligionUtil # BUG - Limit/Extra Religions
# BUG - Align Icons - start
import Scoreboard
import PlayerUtil
# BUG - Align Icons - end
import AttitudeUtil # BUG - Worst Enemy, Refuses to Talk
import DiplomacyUtil # BUG - Refuses to Talk
import TradeUtil # BUG - Fractional Trade
#import BugUnitPlot # advc.092: Moved to avoid circular dependency
# globals
gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()
PleOpt = BugCore.game.PLE # BUG - PLE
import GameUtil # BUG - 3.17 No Espionage
import ReminderEventManager # BUG - Reminders
import GGUtil # BUG - Great General Bar
import GPUtil # BUG - Great Person Bar
import ProgressBarUtil # BUG - Progress Bar
import PLE # PLE Code
import MinimapOptions # advc.002a
import RawYields # BUG - Raw Yields
import WidgetUtil
# <!-- custom: Register custom widget types once at module load so scoreboard-control and City Breakdown hover text works. (Claude code Sonnet 4.6 + GPT-5.6-Sol) -->
WIDGET_SCORE_SCROLL_UP = WidgetUtil.createWidget("WIDGET_SCORE_SCROLL_UP")
WIDGET_SCORE_SCROLL_DOWN = WidgetUtil.createWidget("WIDGET_SCORE_SCROLL_DOWN")
WIDGET_SCORE_SCROLL_UP_FAST = WidgetUtil.createWidget("WIDGET_SCORE_SCROLL_UP_FAST")
WIDGET_SCORE_SCROLL_DOWN_FAST = WidgetUtil.createWidget("WIDGET_SCORE_SCROLL_DOWN_FAST")
WIDGET_SCORE_BG_STYLE = WidgetUtil.createWidget("WIDGET_SCORE_BG_STYLE")
WIDGET_SCORE_EXPAND_TOGGLE = WidgetUtil.createWidget("WIDGET_SCORE_EXPAND_TOGGLE")
WIDGET_SCORE_SCROLL_CENTER_ACTIVE_PLAYER = WidgetUtil.createWidget("WIDGET_SCORE_SCROLL_CENTER_ACTIVE_PLAYER")
WIDGET_SCORE_SCROLL_AUTOCENTER_ACTIVE_PLAYER_TOGGLE = WidgetUtil.createWidget("WIDGET_SCORE_SCROLL_AUTOCENTER_ACTIVE_PLAYER_TOGGLE")
WIDGET_ANNOTATIONS_TOGGLE = WidgetUtil.createWidget("WIDGET_ANNOTATIONS_TOGGLE")
WIDGET_CITY_BREAKDOWNS = WidgetUtil.createWidget("WIDGET_CITY_BREAKDOWNS")
g_szCityBreakdownsHelp = u""
def _scoreHelp(szText):
	return lambda *_: sasFontTagHover + szText + SAS_FONT_TAG_CLOSE
def _cityBreakdownsHelp(*_):
	return g_szCityBreakdownsHelp
WidgetUtil.setWidgetHelpFunction(WIDGET_SCORE_SCROLL_UP, _scoreHelp(u"Scroll scoreboard up"))
WidgetUtil.setWidgetHelpFunction(WIDGET_SCORE_SCROLL_DOWN, _scoreHelp(u"Scroll scoreboard down"))
WidgetUtil.setWidgetHelpFunction(WIDGET_SCORE_SCROLL_UP_FAST, _scoreHelp(u"Scroll scoreboard up (fast)"))
WidgetUtil.setWidgetHelpFunction(WIDGET_SCORE_SCROLL_DOWN_FAST, _scoreHelp(u"Scroll scoreboard down (fast)"))
WidgetUtil.setWidgetHelpFunction(WIDGET_SCORE_BG_STYLE, _scoreHelp(u"Cycle scoreboard background style"))
WidgetUtil.setWidgetHelpFunction(WIDGET_SCORE_EXPAND_TOGGLE, _scoreHelp(u"Toggle lock scoreboard hover"))
WidgetUtil.setWidgetHelpFunction(WIDGET_SCORE_SCROLL_CENTER_ACTIVE_PLAYER, _scoreHelp(u"Center scoreboard scroll on the active player"))
WidgetUtil.setWidgetHelpFunction(WIDGET_SCORE_SCROLL_AUTOCENTER_ACTIVE_PLAYER_TOGGLE, _scoreHelp(u"Toggle autocenter scoreboard scroll on the active player"))
WidgetUtil.setWidgetHelpFunction(WIDGET_ANNOTATIONS_TOGGLE, _scoreHelp(u"Toggle show map annotations"))
WidgetUtil.setWidgetHelpFunction(WIDGET_CITY_BREAKDOWNS, _cityBreakdownsHelp)
# <advc.090>
import math
def floor(f):
	return int(math.floor(f))
def ceil(f):
	return int(math.ceil(f))
def iround(f):
	return int(round(f))
# </advc.090>
# BUG - city specialist - start
g_iMaxEverFreeSpecialists = 0
g_iMaxEverAdjustableSpecialists = 0
g_iMaxEverAngry = 0
# BUG - city specialist - end
# advc: unused
#iCityCenterRow1Xa = 347
#iCityCenterRow2Xa = 482
#g_iNumTradeRoutes = 0
#g_iNumBuildings = 0
g_iNumLeftBonus = 0
g_iNumCenterBonus = 0
g_iNumRightBonus = 0
# advc: Most of this stuff is unused
#MAX_SELECTED_TEXT = 5
#MAX_DISPLAYABLE_BUILDINGS = 15
#MAX_DISPLAYABLE_TRADE_ROUTES = 4
#MAX_BONUS_ROWS = 10
MAX_CITIZEN_BUTTONS = 8
#SELECTION_BUTTON_COLUMNS = 8
#SELECTION_BUTTON_ROWS = 2
#NUM_SELECTION_BUTTONS = SELECTION_BUTTON_ROWS * SELECTION_BUTTON_COLUMNS
#g_iNumBuildingWidgets = MAX_DISPLAYABLE_BUILDINGS
#g_iNumTradeRouteWidgets = MAX_DISPLAYABLE_TRADE_ROUTES

g_NumEmphasizeInfos = 0
g_NumCityTabTypes = 0
g_NumHurryInfos = 0
g_NumUnitClassInfos = 0
g_NumBuildingClassInfos = 0
g_NumProjectInfos = 0
g_NumProcessInfos = 0
g_NumActionInfos = 0
g_eEndTurnButtonState = -1
g_pSelectedUnit = 0
g_szTimeText = ""
gAlignedScoreboard = None # advc.085
# BUG - NJAGC - start
g_bShowTimeTextAlt = False
g_iTimeTextCounter = -1
# BUG - NJAGC - end
# BUG - Raw Yields - start
g_bRawShowing = False
g_bYieldView, g_iYieldType = RawYields.getViewAndType(0)
g_iYieldTiles = RawYields.WORKED_TILES
RAW_YIELD_HELP = (	"TXT_KEY_RAW_YIELD_VIEW_TRADE", "TXT_KEY_RAW_YIELD_VIEW_FOOD", "TXT_KEY_RAW_YIELD_VIEW_PRODUCTION", "TXT_KEY_RAW_YIELD_VIEW_COMMERCE", "TXT_KEY_RAW_YIELD_TILES_WORKED", "TXT_KEY_RAW_YIELD_TILES_CITY", "TXT_KEY_RAW_YIELD_TILES_OWNED", "TXT_KEY_RAW_YIELD_TILES_ALL"	)
# BUG - Raw Yields - end
# BUG - field of view slider:
#DEFAULT_FIELD_OF_VIEW = 42 # disabled (replaced) by K-Mod

class CvMainInterface:
	"Main Interface Screen"
	def _cacheMapViewRightPanelRects(self):
		self.bMapViewRightPanelNarrow = False

	def _setPanelSizeFromRect(self, szName):
		lRect = gRect(szName)
		self.screen.setPanelSize(szName, lRect.x(), lRect.y(), lRect.width(), lRect.height())

	def _scoreboardFastScrollStep(self):
		if self.iSAS_SCOREBOARD_SCROLL_INCREMENT_FAST is None:
			self.iSAS_SCOREBOARD_SCROLL_INCREMENT_FAST = gc.getDefineINT("SAS_SCOREBOARD_SCROLL_INCREMENT_FAST")
		return self.iSAS_SCOREBOARD_SCROLL_INCREMENT_FAST

	# <!-- custom: shared scoreboard traversal for normal drawing, scroll slicing, and active-player scroll centering.
	# Passing scores also preserves the old BUG aligned-scoreboard side effects by filling teams/players and calling playerScoreString; the isShowTeamScore/isShowPlayerScore checks are the advc.085 static helpers that used to be called inline in updateScoreStrings. (ChatGPT-5.5 + GPT-5.5) -->
	def _scoreboardVisiblePlayers(self, scores=None):
		aeVisiblePlayers = []
		bAlignIcons = (scores is not None)
		for iTeamRank in range(gc.getMAX_CIV_TEAMS() - 1, -1, -1):
			eTeam = gc.getGame().getRankTeam(iTeamRank)
			if not Scoreboard.Scoreboard.isShowTeamScore(eTeam):
				continue
			if bAlignIcons:
				scores.addTeam(gc.getTeam(eTeam), iTeamRank)
			for iPlayerRank in range(gc.getMAX_CIV_PLAYERS() - 1, -1, -1):
				ePlayer = gc.getGame().getRankPlayer(iPlayerRank)
				if Scoreboard.Scoreboard.isShowPlayerScore(ePlayer) and gc.getPlayer(ePlayer).getTeam() == eTeam:
					aeVisiblePlayers.append(ePlayer)
					if bAlignIcons:
						scores.addPlayer(gc.getPlayer(ePlayer), iPlayerRank)
						# AdvCiv/BUG aligned path fills per-part fields through playerScoreString side effects.
						self.playerScoreString(ePlayer, scores, True)
		return aeVisiblePlayers

	def _scoreboardMaxRows(self):
		iBtnHeight = VLEN(22)
		iSScrollBtnSz = gRect("ScoreScrollUp").width()
		iSScrollY = gRect("GlobeToggle").y() - iSScrollBtnSz
		iScoreBottom = iSScrollY - VSPACE(2) - VSPACE(9)
		iScoreTopMin = gRect("AdvisorButtons").yBottom()
		return max(1, (iScoreBottom - iScoreTopMin) // iBtnHeight)

	def _centerScoreboardScrollOnActivePlayer(self, aeVisiblePlayers=None, iMaxRows=None):
		if aeVisiblePlayers is None:
			aeVisiblePlayers = self._scoreboardVisiblePlayers()
		if iMaxRows is None:
			iMaxRows = self._scoreboardMaxRows()
		iTotalCount = len(aeVisiblePlayers)
		if iTotalCount <= iMaxRows:
			self.iScoreScrollOffset = 0
			return
		eActivePlayer = gc.getGame().getActivePlayer()
		if eActivePlayer < 0:
			return
		try:
			iActiveIndex = aeVisiblePlayers.index(eActivePlayer)
		except ValueError:
			return
		self.iScoreScrollOffset = max(0, min(iActiveIndex - (iMaxRows / 2), iTotalCount - iMaxRows))

	def _appendSelectedInfoPaneTextRow(self, screen, szTable, iRow, szLeftBuffer, szRightBuffer, szButton="", iData1=-1):
		screen.appendTableRow(szTable)
		screen.setTableText(szTable, 0, iRow, sasFontTagLabel + szLeftBuffer + SAS_FONT_TAG_CLOSE, szButton, WidgetTypes.WIDGET_HELP_SELECTED, iData1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.setTableText(szTable, 1, iRow, sasFontTagLabel + szRightBuffer + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_HELP_SELECTED, iData1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		screen.show(szTable)
		screen.show("SelectedUnitPanel")
		return iRow + 1

	def _applyMapViewRightPanelNarrow(self, bEnable):
		self._cacheMapViewRightPanelRects()
		bCityScreen = CyInterface().isCityScreenUp()
		if (not bCityScreen) and bEnable == self.bMapViewRightPanelNarrow:
			return
		if bCityScreen:
			iPanelX = gRect("LowerRightCornerPanel").x()
		else:
			if bEnable:
				iPanelW = gRect("MiniMapPanel").width() + HSPACE(6)
			else:
				iPanelW = gRect("LowerLeftCornerPanel").width()
			iPanelX = gRect("Top").xRight() - iPanelW
			gRect("LowerRightCornerPanel").moveTo(iPanelX, gRect("LowerRightCornerPanel").y())
			gRect("LowerRightCornerPanel").adjustSize(iPanelW - gRect("LowerRightCornerPanel").width(), 0)
			gRect("LowerRightCorner").moveTo(iPanelX, gRect("LowerRightCorner").y())
			gRect("LowerRightCorner").adjustSize(iPanelW - gRect("LowerRightCorner").width(), 0)
		# <!-- custom: center bottom panel should stop before right panel in city screen, and extend slightly in map view to avoid a gap. (GPT-5.2-Codex) -->
		# <!-- custom: controls the city screen as intended (e.g. 150 successfully reduces it only in the city screen) -->
		if bCityScreen:
			iCenterMargin = 10
		# <!-- custom: controls the map view (i.e. outside city screen as intended, e.g. -500 successfully reduces it only in map view) -->
		else:
			iCenterMargin = 10
		iCenterW = iPanelX - gRect("LowerLeftCorner").xRight() + iCenterMargin
		gRect("CenterBottomPanel").adjustSize(iCenterW - gRect("CenterBottomPanel").width(), 0)
		gRect("CenterBottom").adjustSize(iCenterW - gRect("CenterBottom").width(), 0)
		self.bMapViewRightPanelNarrow = bEnable
		# <!-- custom: shrink right panel in map view so the map starts closer to it. (GPT-5.2-Codex) -->
		self._setPanelSizeFromRect("LowerRightCornerPanel")
		self._setPanelSizeFromRect("CenterBottomPanel")
	def cityScreenBottomLift(self):
		# <!-- custom: base city-screen lift used by side panels/bars; keep shared behavior stable. (GPT-5.3-Codex) -->
		return self.bottomListBtnSize() + VSPACE(16)

	def _cacheCityScreenSideLiftRects(self):
		if hasattr(self, "cityScreenSideLiftRects"):
			return
		self.cityScreenSideLiftRects = {}
		self.cityScreenUpperSideRects = {}
		for szName in self._cityScreenSideLiftRectNames():
			self.cityScreenSideLiftRects[szName] = gRect(szName).copy()
		for szName in self._cityScreenUpperSideRectNames():
			self.cityScreenUpperSideRects[szName] = gRect(szName).copy()
		self.bCityScreenSideLiftApplied = False

	def _cityScreenSideLiftRectNames(self):
		aRects = ["LowerLeftCornerPanel", "LowerLeftCornerBackgr", "LowerLeftCorner", "LowerRightCornerPanel", "LowerRightCorner", "CenterBottomPanel", "CenterBottom", "CultureBars", "CultureBar", "GreatPeopleBar",]
		return aRects

	def _cityScreenUpperSideRectNames(self):
		aRects = ["CityLeftPanel", "CityRightPanel", "CityLeftPanelStagger", "CityRightPanelStagger",]
		return aRects

	def _applyCityScreenSideLift(self, bEnable):
		self._cacheCityScreenSideLiftRects()
		if bEnable == self.bCityScreenSideLiftApplied:
			return
		iLift = self.cityScreenBottomLift()
		iRibbonExtraLift = self.iCityScreenBlueRibbonExtraLift
		for szName in self._cityScreenSideLiftRectNames():
			lBase = self.cityScreenSideLiftRects[szName].copy()
			if bEnable:
				if szName == "CenterBottomPanel":
					# <!-- custom: decorative ribbon gets the full lift (base + ribbon-extra). (Claude code Opus 4.7) -->
					iCenterLift = iLift + iRibbonExtraLift
					lBase.move(0, -iCenterLift)
					lBase.adjustSize(0, iCenterLift)
				elif szName == "CenterBottom":
					# <!-- custom: bar container gets base lift only, so the bar rect (and iMaxHeight feeding updateBottomButtonList) stays independent of the ribbon-extra-lift knob. (Claude code Opus 4.7) -->
					lBase.move(0, -iLift)
					lBase.adjustSize(0, iLift)
				elif szName in ("LowerLeftCornerPanel", "LowerRightCornerPanel"):
					lBase.move(0, -iLift)
					lBase.adjustSize(0, iLift)
				else:
					lBase.move(0, -iLift)
			gRect(szName).moveTo(lBase.x(), lBase.y())
			gRect(szName).adjustSize(lBase.width() - gRect(szName).width(), lBase.height() - gRect(szName).height())
		for szName in self._cityScreenUpperSideRectNames():
			lBase = self.cityScreenUpperSideRects[szName].copy()
			if bEnable:
				lBase.adjustSize(0, -iLift)
			gRect(szName).moveTo(lBase.x(), lBase.y())
			gRect(szName).adjustSize(lBase.width() - gRect(szName).width(), lBase.height() - gRect(szName).height())
		self.bCityScreenSideLiftApplied = bEnable
		# <!-- custom: city screen only - lift side bottom panels by 2 build rows. (GPT-5.2-Codex) -->
		for szName in ("LowerLeftCornerPanel", "LowerRightCornerPanel", "CenterBottomPanel", "CityLeftPanel", "CityRightPanel", "CityLeftPanelStagger", "CityRightPanelStagger"):
			self._setPanelSizeFromRect(szName)
		gOffSetPoint("GreatPeopleText", "GreatPeopleBar", RectLayout.CENTER, self.stackBarDefaultTextOffset())
		gOffSetPoint("NationalityText", "NationalityBar", RectLayout.CENTER, self.stackBarDefaultTextOffset())
		gOffSetPoint("CultureText", "CultureBar", RectLayout.CENTER, self.stackBarDefaultTextOffset())
		self._rebuildCityScreenBottomBars()
		CyInterface().setDirty(InterfaceDirtyBits.InfoPane_DIRTY_BIT, True)
		CyInterface().setDirty(InterfaceDirtyBits.CityScreen_DIRTY_BIT, True)

	def _rebuildCityScreenBottomBars(self):
		screen = self.screen
		for szName in ("GreatPeopleBar", "CultureBar", "NationalityBar"):
			try:
				screen.deleteWidget(szName)
			except:
				pass
		# <!-- custom: rebuild city-screen stacked bars after rect moves so the bars follow the new positions. (GPT-5.2-Codex) -->
		self.addStackedBar("GreatPeopleBar", WidgetTypes.WIDGET_HELP_GREAT_PEOPLE)
		screen.setStackedBarColors("GreatPeopleBar", InfoBarTypes.INFOBAR_STORED, self.colorGreatPeopleStored)
		screen.setStackedBarColors("GreatPeopleBar", InfoBarTypes.INFOBAR_RATE, self.colorGreatPeopleRate)
		screen.setStackedBarColors("GreatPeopleBar", InfoBarTypes.INFOBAR_RATE_EXTRA, self.colorEmpty)
		screen.setStackedBarColors("GreatPeopleBar", InfoBarTypes.INFOBAR_EMPTY, self.colorEmpty)
		screen.hide("GreatPeopleBar")

		self.addStackedBar("CultureBar", WidgetTypes.WIDGET_HELP_CULTURE)
		screen.setStackedBarColors("CultureBar", InfoBarTypes.INFOBAR_STORED, self.colorGreatCultureStored)
		screen.setStackedBarColors("CultureBar", InfoBarTypes.INFOBAR_RATE, self.colorGreatCultureRate)
		screen.setStackedBarColors("CultureBar", InfoBarTypes.INFOBAR_RATE_EXTRA, self.colorEmpty)
		screen.setStackedBarColors("CultureBar", InfoBarTypes.INFOBAR_EMPTY, self.colorEmpty)
		screen.hide("CultureBar")

		self.addStackedBar("NationalityBar", WidgetTypes.WIDGET_HELP_NATIONALITY)
		screen.hide("NationalityBar")
	# <advc.092> CyGInterfaceScreen wrappers that look up global layout data
	def addPanel(self, szName, eStyle = None):
		if eStyle is None:
			eStyle = PanelStyles.PANEL_STYLE_STANDARD
		lRect = gRect(szName)
		self.screen.addPanel(szName, u"", u"", True, False, lRect.x(), lRect.y(), lRect.width(), lRect.height(), eStyle)
	def addScrollPanel(self, szName, eStyle = None):
		if eStyle is None:
			eStyle = PanelStyles.PANEL_STYLE_EXTERNAL
		lRect = gRect(szName)
		self.screen.addScrollPanel(szName, u"", lRect.x(), lRect.y(), lRect.width(), lRect.height(), eStyle)
	# Can be either an interface art info string or a CvInfo object with a getButton method
	def addDDS(self, szName, textureInfo, eWidgetType = None, iData1 = -1, iData2 = -1, szAttachTo = None):
		if eWidgetType is None:
			eWidgetType = WidgetTypes.WIDGET_GENERAL
		lRect = gRect(szName)
		if isinstance(textureInfo, basestring):
			szTexture = ArtFileMgr.getInterfaceArtInfo(textureInfo).getPath()
		else:
			szTexture = textureInfo.getButton()
		if szAttachTo:
			lParent = gRect(szAttachTo)
			self.screen.addDDSGFCAt(szName, szAttachTo, szTexture, lRect.x() - lParent.x(), lRect.y() - lParent.y(), lRect.width(), lRect.height(), eWidgetType, iData1, iData2, False)
		else:
			self.screen.addDDSGFC(szName, szTexture, lRect.x(), lRect.y(), lRect.width(), lRect.height(), eWidgetType, iData1, iData2)
	def addDDSAt(self, szName, szAttachTo, textureInfo, eWidgetType = None, iData1 = -1, iData2 = -1):
		self.addDDS(szName, textureInfo, eWidgetType, iData1, iData2, szAttachTo)
	def _setStyledImageButton(self, szName, szAttachTo, szTexture, style, eWidgetType, iData1, iData2, szText = ""):
		if eWidgetType is None:
			eWidgetType = WidgetTypes.WIDGET_GENERAL
		lRect = gRect(szName)
		if isinstance(style, basestring):
			szStyle = style
		else:
			szStyle = None
		if szAttachTo:
			lParent = gRect(szAttachTo)
			assert szStyle
			self.screen.setImageButtonAt(szName, szAttachTo, szTexture, lRect.x() - lParent.x(), lRect.y() - lParent.y(), lRect.width(), lRect.height(), eWidgetType, iData1, iData2)
		else:
			if (szStyle or style is None) and not szText:
				self.screen.setImageButton(szName, szTexture, lRect.x(), lRect.y(), lRect.width(), lRect.height(), eWidgetType, iData1, iData2)
			else:
				if szStyle:
					style = ButtonStyles.BUTTON_STYLE_STANDARD
				self.screen.setButtonGFC(szName, szText, "", lRect.x(), lRect.y(), lRect.width(), lRect.height(), eWidgetType, iData1, iData2, style)
				if szStyle:
					self.screen.setStyle(szName, szStyle)
		if szStyle:
			self.screen.setStyle(szName, szStyle)
	# Style can be a key string or ID. Text label; rarely needed.
	def setStyledButton(self, szName, style, eWidgetType = None, iData1 = -1, iData2 = -1, szAttachTo = None, szText = ""):
		self._setStyledImageButton(szName, szAttachTo, "", style, eWidgetType, iData1, iData2, szText)
	def setStyledButtonAt(self, szName, szAttachTo, szStyle, eWidgetType = None, iData1 = -1, iData2 = -1):
		self.setStyledButton(szName, szStyle, eWidgetType, iData1, iData2, szAttachTo)
	@staticmethod
	def _isInterfaceArtKey(s):
		# for BUG - Raw Yields
		# for BUG - PLE
		return (s.startswith("INTERFACE_") or s.startswith("BUTTON_") or s.startswith("RAW_YIELDS_") or s.startswith("PLE_"))
	# Path or InterfaceArtInfo key
	def setImageButton(self, szName, szTexture, eWidgetType = None, iData1 = -1, iData2 = -1, szAttachTo = None):
		if CvMainInterface._isInterfaceArtKey(szTexture):
			szTexture = ArtFileMgr.getInterfaceArtInfo(szTexture).getPath()
		self._setStyledImageButton(szName, szAttachTo, szTexture, None, eWidgetType, iData1, iData2)
	def setImageButtonAt(self, szName, szAttachTo, szTexture, eWidgetType = None, iData1 = -1, iData2 = -1):
		self.setImageButton(szName, szTexture, eWidgetType, iData1, iData2, szAttachTo)
	# Paths or InterfaceArtInfo keys
	def addCheckBox(self, szName, szTexture, szHLTexture, eStyle, eWidgetType = None, iData1 = -1, iData2 = -1, szAttachTo = None):
		if eWidgetType is None:
			eWidgetType = WidgetTypes.WIDGET_GENERAL
		lRect = gRect(szName)
		if CvMainInterface._isInterfaceArtKey(szTexture):
			szTexture = ArtFileMgr.getInterfaceArtInfo(szTexture).getPath()
		if CvMainInterface._isInterfaceArtKey(szHLTexture):
			szHLTexture = ArtFileMgr.getInterfaceArtInfo(szHLTexture).getPath()
		if szAttachTo:
			lParent = gRect(szAttachTo)
			self.screen.addCheckBoxGFCAt(szAttachTo, szName, szTexture, szHLTexture, lRect.x() - lParent.x(), lRect.y() - lParent.y(), lRect.width(), lRect.height(), eWidgetType, iData1, iData2, eStyle, True)
		else:
			self.screen.addCheckBoxGFC(szName, szTexture, szHLTexture, lRect.x(), lRect.y(), lRect.width(), lRect.height(), eWidgetType, iData1, iData2, eStyle)
	def addCheckBoxAt(self, szName, szAttachTo, szTexture, szHLTexture, eStyle, eWidgetType = None, iData1 = -1, iData2 = -1):
		self.addCheckBox(szName, szTexture, szHLTexture, eStyle, eWidgetType, iData1, iData2, szAttachTo)
	def addSlider(self, szName, iDefault, iMin = 0, iMax = -1, bVertical = False, eWidgetType = None, iData1 = -1, iData2 = -1):
		if eWidgetType is None:
			eWidgetType = WidgetTypes.WIDGET_GENERAL
		lRect = gRect(szName)
		if iMax < 0:
			iMax = lRect.width()
		self.screen.addSlider(szName, lRect.x(), lRect.y(), lRect.width(), lRect.height(), iDefault, iMin, iMax, eWidgetType, iData1, iData2, bVertical)
	def addTable(self, szName, iCols, szStyle, iBtnSize = None):
		if iBtnSize is None:
			# Seems to work well for rows at font size 1 (BAT uses only 24)
			iBtnSize = 26
		lRect = gRect(szName)
		self.screen.addTableControlGFC(szName, iCols, lRect.x(), lRect.y(), lRect.width(), lRect.height(), False, False, iBtnSize, iBtnSize, TableStyles.TABLE_STYLE_STANDARD)
		self.screen.setStyle(szName, szStyle)
	def setLabel(self, szName, szAttachTo, szText, uiFlags, eFont, fZ = 0, eWidgetType = None, iData1 = -1, iData2 = -1):
		if eWidgetType is None:
			eWidgetType = WidgetTypes.WIDGET_GENERAL
		lPoint = gPoint(szName)
		self.screen.setLabel(szName, szAttachTo, szText, uiFlags, lPoint.x(), lPoint.y(), fZ, eFont, eWidgetType, iData1, iData2)
	def setText(self, szName, szAttachTo, szText, uiFlags, eFont, fZ = 0, eWidgetType = None, iData1 = -1, iData2 = -1):
		if eWidgetType is None:
			eWidgetType = WidgetTypes.WIDGET_GENERAL
		lPoint = gPoint(szName)
		self.screen.setText(szName, szAttachTo, szText, uiFlags, lPoint.x(), lPoint.y(), fZ, eFont, eWidgetType, iData1, iData2)
	# <!-- custom: create a one-line, horizontally centered icon+text row; place the icon first and the text immediately to its right, with optional vertical offsets to auto lift/shift the icon without changing the text baseline.
	# Uses setImageButton for atlas/tech buttons (fixes purple missing icons from <img>), and falls back to <img> text when given an inline tag.
	# Note: image buttons are absolute-positioned and won’t auto-scroll with table rows, so use <img> for scrollable lists.
	# Keeps centering stable as text length changes. (GPT-5.2-Codex (summarized)) -->
	def setSASCenteredImageButtonAtLeftOfTextRow(self, szTextName, szIconName, szAttachTo, szText, szIcon, iIconSize, iX, iY, iMargin = None, szRectName = None, iTextYOffset = 0, iIconYOffset = 0, eFont = None, fZ = 0, eWidgetType = None, iData1 = -1, iData2 = -1):
		if eWidgetType is None:
			eWidgetType = WidgetTypes.WIDGET_GENERAL
		if eFont is None:
			eFont = FontTypes.GAME_FONT
		if iMargin is None:
			iMargin = HSPACE(2)
		iTextWidth = CyInterface().determineWidth(szText)
		iTotalWidth = iIconSize + iMargin + iTextWidth
		if iX is None and szRectName:
			lRect = gRect(szRectName)
			iX = lRect.x() + (lRect.width() - iTotalWidth) / 2
			# <!-- custom: keep icon+text row within the target bar rect so larger upscaled fonts don't push the icon outside and make it look missing. (GPT-5.3-Codex) -->
			iX = max(lRect.x(), min(iX, lRect.xRight() - iTotalWidth))
		iTextX = iX + iIconSize + iMargin
		gSetPoint(szTextName, PointLayout(iTextX, iY + iTextYOffset))
		if szIcon.startswith("<img="):
			gSetPoint(szIconName, PointLayout(iX, iY + iIconYOffset))
			self.setText(szIconName, szAttachTo, szIcon, CvUtil.FONT_LEFT_JUSTIFY, eFont, fZ, eWidgetType, iData1, iData2)
		else:
			# Avoid gRect lookup; we have explicit coords and size.
			self.screen.setImageButton(szIconName, szIcon, iX, iY + iIconYOffset, iIconSize, iIconSize, eWidgetType, iData1, iData2)
		self.setText(szTextName, szAttachTo, szText, CvUtil.FONT_LEFT_JUSTIFY, eFont, fZ, eWidgetType, iData1, iData2)
	# color can be either a key string or a color id
	def createProgressBar(self, szName, color, uiMarks, bForward):
		lRect = gRect(szName)
		if isinstance(color, basestring):
			eColor = getInfoTypeOrFail(color)
		else:
			eColor = color
		return ProgressBarUtil.ProgressBar(szName + "-Canvas", lRect.x(), lRect.y(), lRect.width(), lRect.height(), eColor, uiMarks, bForward)
	def addStackedBar(self, szName, eWidgetType = None, iData1 = -1, iData2 = -1, szAttachTo = None):
		if eWidgetType is None:
			eWidgetType = WidgetTypes.WIDGET_GENERAL
		lRect = gRect(szName)
		if szAttachTo:
			lParent = gRect(szAttachTo)
			self.screen.addStackedBarGFCAt(szName, szAttachTo, lRect.x() - lParent.x(), lRect.y() - lParent.y(), lRect.width(), lRect.height(), InfoBarTypes.NUM_INFOBAR_TYPES, eWidgetType, iData1, iData2)
		else:
			self.screen.addStackedBarGFC(szName, lRect.x(), lRect.y(), lRect.width(), lRect.height(), InfoBarTypes.NUM_INFOBAR_TYPES, eWidgetType, iData1, iData2)
	def addStackedBarAt(self, szName, szAttachTo, eWidgetType = None, iData1 = -1, iData2 = -1):
		self.addStackedBar(szName, eWidgetType, iData1, iData2, szAttachTo)
	# NB: I've moved the scale param toward the front
	def _addEntityGraphic(self, szName, iEntity, eWidgetType, iData1, iData2, fScale, fxRotation, fzRotation, bUnit = True):
		if eWidgetType is None:
			eWidgetType = WidgetTypes.WIDGET_GENERAL
		lRect = gRect(szName)
		if bUnit:
			addEntityGraphicGFC = self.screen.addUnitGraphicGFC
		else:
			addEntityGraphicGFC = self.screen.addBuildingGraphicGFC
		addEntityGraphicGFC(szName, iEntity, lRect.x(), lRect.y(), lRect.width(), lRect.height(), eWidgetType, iData1, iData2, fxRotation, fzRotation, fScale, False)
	# (UnitTypes id),
	def addUnitGraphic(self, szName, iUnit, eWidgetType = None, iData1 = -1, iData2 = 1, fScale = None, fxRotation = -20, fzRotation = 30):
		if fScale is None:
			fScale = 1.0
			# <advc.002j>
			if gc.getUnitInfo(iUnit).getDomainType() == DomainTypes.DOMAIN_LAND:
				fScale *= 0.9 # </advc.002j>
		self._addEntityGraphic(szName, iUnit, eWidgetType, iData1, iData2, fScale, fxRotation, fzRotation)
	# (BuildingTypes id),
	def addBuildingGraphic(self, szName, iBuilding, eWidgetType = None, iData1 = -1, iData2 = 1, fScale = 0.8, fxRotation = -20, fzRotation = 30):
		self._addEntityGraphic(szName, iBuilding, eWidgetType, iData1, iData2, fScale, fxRotation, fzRotation, False)
	def setDefaultHelpTextArea(self, bMinMargin = False):
		if bMinMargin:
			lRect = gRect("DefaultHelpAreaMin")
		else:
			lRect = gRect("DefaultHelpArea")
		# advc.092c: Since the EXE apparently ignores the iMinWidth param (as well as fWidth), let the DLL handle the matter - which it will do by hacking the EXE. And this hack needs to happen before creating the help text area.
		gc.getGame().setHelpTextAreaWidth(lRect.width())
		self.screen.setHelpTextArea(HLEN(350), FontTypes.SMALL_FONT, lRect.x(), lRect.y(), -0.1, False, "", True, False, CvUtil.FONT_LEFT_JUSTIFY, lRect.width())
	def stackBarDefaultHeight(self):
		return VLEN(27, 0.85)
	def stackBarDefaultTextOffset(self):
		# Was 3 or 4 in BtS. Not sure whether the text is intended to be aligned at the bottom or center - the height of the game font is almost the same as the height of the bars. Let's assume that we want to center the text. I would've thought that making the offset a fraction of the bar height would accomplish that, but, somehow, it's not enough.
		return VLEN((3.15 * self.stackBarDefaultHeight()) / 27)
	def cityScreenHeadingBackgrHeight(self):
		return 30
	def cityScreenHeadingOffset(self):
		# advc.002b: Was 8 in BtS - too much at our font size.
		return (6 * self.cityScreenHeadingBackgrHeight()) / 30
	# </advc.092>

	def __init__(self):
		# advc.009b: This hack is no longer needed. I've moved the global onSwitchHotSeatPlayer function to BugUtil.py.
		# BUG - start
		#global g_mainInterface
		#g_mainInterface = self
		# BUG - end

# BUG - draw method
		self.DRAW_METHOD_PLE = "DRAW_METHOD_PLE"
		self.DRAW_METHOD_VAN = "DRAW_METHOD_VAN"
		self.DRAW_METHOD_BUG = "DRAW_METHOD_BUG"
		# advc.069: No longer used
		#self.DRAW_METHODS = (self.DRAW_METHOD_PLE, self.DRAW_METHOD_VAN, self.DRAW_METHOD_BUG)
#		self.sDrawMethod = self.DRAW_METHOD_PLE
# BUG - draw method

# BUG - PLE - start
		self.PLE = PLE.PLE()
#		self.PLE.PLE_initialize()

		self.MainInterfaceInputMap = {
			self.PLE.PLOT_LIST_BUTTON_NAME	: self.PLE.getPlotListButtonName,
			self.PLE.PLOT_LIST_MINUS_NAME	: self.PLE.getPlotListMinusName,
			self.PLE.PLOT_LIST_PLUS_NAME	: self.PLE.getPlotListPlusName,
			self.PLE.PLOT_LIST_UP_NAME		: self.PLE.getPlotListUpName,
			self.PLE.PLOT_LIST_DOWN_NAME 	: self.PLE.getPlotListDownName,

			"PleViewModeStyle1"				: self.PLE.onClickPLEViewMode,
			self.PLE.PLE_VIEW_MODE			: self.PLE.onClickPLEViewMode,
			self.PLE.PLE_MODE_STANDARD		: self.PLE.onClickPLEModeStandard,
			self.PLE.PLE_MODE_MULTILINE		: self.PLE.onClickPLEModeMultiline,
			self.PLE.PLE_MODE_STACK_VERT	: self.PLE.onClickPLEModeStackVert,
			self.PLE.PLE_MODE_STACK_HORIZ	: self.PLE.onClickPLEModeStackHoriz,

			self.PLE.PLOT_LIST_PROMO_NAME	: self.PLE.unitPromotion,
			self.PLE.PLOT_LIST_UPGRADE_NAME	: self.PLE.unitUpgrade,

			self.PLE.PLE_RESET_FILTERS		: self.PLE.onClickPLEResetFilters,
			self.PLE.PLE_FILTER_CANMOVE		: self.PLE.onClickPLEFilterCanMove,
			self.PLE.PLE_FILTER_CANTMOVE	: self.PLE.onClickPLEFilterCantMove,
			self.PLE.PLE_FILTER_NOTWOUND	: self.PLE.onClickPLEFilterNotWound,
			self.PLE.PLE_FILTER_WOUND		: self.PLE.onClickPLEFilterWound,
			self.PLE.PLE_FILTER_LAND		: self.PLE.onClickPLEFilterLand,
			self.PLE.PLE_FILTER_SEA			: self.PLE.onClickPLEFilterSea,
			self.PLE.PLE_FILTER_AIR			: self.PLE.onClickPLEFilterAir,
			self.PLE.PLE_FILTER_MIL			: self.PLE.onClickPLEFilterMil,
			self.PLE.PLE_FILTER_DOM			: self.PLE.onClickPLEFilterDom,
			self.PLE.PLE_FILTER_OWN			: self.PLE.onClickPLEFilterOwn,
			self.PLE.PLE_FILTER_FOREIGN		: self.PLE.onClickPLEFilterForeign,

			self.PLE.PLE_GRP_UNITTYPE		: self.PLE.onClickPLEGrpUnittype,
			self.PLE.PLE_GRP_GROUPS			: self.PLE.onClickPLEGrpGroups,
			self.PLE.PLE_GRP_PROMO			: self.PLE.onClickPLEGrpPromo,
			self.PLE.PLE_GRP_UPGRADE		: self.PLE.onClickPLEGrpUpgrade,
		}

#		self.iVisibleUnits 			= 0
		self.iMaxPlotListIcons 		= 0

		self.bPLECurrentlyShowing	= False
		self.bVanCurrentlyShowing	= False
# BUG - draw method
		self.bBUGCurrentlyShowing	= False
# BUG - draw method

		self.xResolution = 0
		self.yResolution = 0
# BUG - PLE - end

# BUG - field of view slider - start
		self.sFieldOfView_Text = ""
		self.iField_View_Prev = -1
		self.iFoVPos_Prev = -1 # advc.090
# BUG - field of view slider - end

		# <!-- custom: use more side space for side panels rather than city plots we do not need to show beyond BFC. Credit: Gemini 3 Pro. (GPT-5.2-Codex (summarized)) -->
		# <!-- custom: extend past the current inner edge of the bottom panels so the bonus panels fit; even columns were overlapping text.
		# Ideally also widen that panel, but start here. (GPT-5.2-Codex (summarized)); was 297 -->
		self.SIDE_PANELS_WIDTH = 341
		# <!-- custom: extra width added rightward to the city-screen trade-route and building-list tables so their right-justified yield columns reach toward the commerce value column.
		# The tables intentionally overflow CityLeftPanelContents (fine: unused room before the map, and text overflows when upscaled anyway). (Claude code Opus 4.7); was 0 -->
		self.iCityScreenYieldTableRightExtra = 10

		# <!-- custom: initialize cheaply once. -->
		# <!-- custom: unlike in other files, setting this as a global and reading from gc in global scope doesn't work; regardless of the SAS define value, extra rows for the city screen production chooser stay disabled in-game.
		# So set it here. (GPT-5.2-Codex (summarized)) -->
		self.iBarExtraRows = None
		self.iBarExtraRowsExtraManualAdjust = None
		self.iBarExtraHeightExtraManualAdjust = None
		self.iCityScreenBlueRibbonExtraLift = None
		# <!-- custom: cached resolution-scaled width adjustment for the left-side default help text area; positive widens, negative narrows. (Claude code Opus 4.7) -->
		self.iSAS_CV_MAIN_INTERFACE_DEFAULTHELPAREA_WIDTH_ADJUSTMENT = None
		# <!-- custom: cached resolution-scaled offsets for the map-view unit selection bar (PlotListPanel). (Claude code Opus 4.7) -->
		self.iSAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_X_OFFSET = None
		self.iSAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_Y_OFFSET = None
		# <!-- custom: cached HARD-px width adjustment for the unit bar's available row width; negative shrinks (fewer buttons per row, avoids overflow into scoreboard / off-screen). (Claude code Opus 4.7) -->
		self.iSAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_WIDTH_PIXEL_ADJUSTMENT = None
		# <!-- custom: fix production chooser bar auto-scrolling when clicking lower rows; it is distracting and unnecessary since the player can scroll. Credit: ChatGPT 5.2. (GPT-5.2-Codex (summarized)) -->
		# When your BottomButtonList is tall enough to show multiple rows, clicking the lower visible row changes CityTabSelectionRow, and then the selectMultiList() call scrolls the control so that row becomes the top row.
		# Minimal fix: "pin" the top visible row, and only change it via the tab/scroll buttons
		# 1) Add a pinned-row member (once)
		self.iCityBuildBarPinnedRow = None
		# <!-- custom: building list filter state (0=All, 1=Buildings, 2=National, 3=World). (GPT-5.2-Codex) -->
		self.iBuildingFilter = 0
		self.buildFilterBtnAll = "BuildFilter0"
		self.buildFilterBtnBuildings = "BuildFilter1"
		self.buildFilterBtnNational = "BuildFilter2"
		self.buildFilterBtnWorld = "BuildFilter3"
		# <!-- custom: add buttons in the city screen production queue elements. Credit: Claude Opus 4.5. (GPT-5.2-Codex (summarized)) -->
		self.IS_SAS_CV_MAIN_INTERFACE_PRODUCTION_QUEUE_BUTTONS = None
		# <!-- custom: optional UI convenience: hide the unit plot list bar while a city is selected to avoid overlapping with multi-row build bars. (ChatGPT-5.2 Thinking) -->
		self.IS_SAS_CV_MAIN_INTERFACE_HIDE_PLOT_LIST_PANEL_IN_CITY_SCREEN = None
		# <!-- custom: city-screen bonus rendering mode: 0 = old text glyphs, 1 = base AdvCiv-style framed image buttons, 2 = frameless DDS icons. (GPT-5.5) -->
		self.SAS_CV_MAIN_INTERFACE_CITY_BONUS_ICON_MODE = None
		self.iCityBonusPreferredBtnSize = BTNSZ(40)
		self.iCityBonusMinBtnSize = BTNSZ(20)
		self.iCityBonusShrinkStep = BTNSZ(5)
		# <!-- custom: optional unit info button in the map view unit panel. (GPT-5.2-Codex) -->
		self.IS_SAS_CV_MAIN_INTERFACE_UNIT_INFO_BUTTON = None
		# <!-- custom: optional top-left flag and left-side text spacing shift (e.g. treasury) for the main interface. (GPT-5.3-Codex (summarized)) -->
		self.IS_SAS_CV_MAIN_INTERFACE_TOP_LEFT_FLAG_AND_LEFT_SIDE_TEXT_SHIFT = None
		# <!-- custom: cached commerce block position and alignment defines for city-screen and map views. (Claude code Sonnet 4.6) -->
		self.iSAS_COMMERCE_CITY_X_OFFSET = None
		self.iSAS_COMMERCE_CITY_Y_OFFSET = None
		self.IS_SAS_COMMERCE_CITY_ANCHOR_RIGHT = None
		self.iSAS_COMMERCE_MAP_X_OFFSET = None
		self.iSAS_COMMERCE_MAP_Y_OFFSET = None
		self.IS_SAS_COMMERCE_MAP_ANCHOR_RIGHT = None
		# <!-- custom: cached vanilla engine defines used across city-screen update methods. (Claude code Sonnet 4.6) -->
		self.iCOMMERCE_PERCENT_CHANGE_INCREMENTS = None
		# <!-- custom: the optional second (Min/Max) pair now does a fast +/- step from a SAS define instead of snapping to 0/100; lazily cached. (Claude code Opus 4.7) -->
		self.iSAS_COMMERCE_SLIDERS_FAST_INCREMENT_PERCENT = None
		self.iSAS_SCOREBOARD_SCROLL_INCREMENT_FAST = None
		self.iDEFAULT_SPECIALIST = None
		self.iMAX_TRADE_ROUTES = None
		self.IS_USE_KMOD_TRADE_CULTURE = None
		self.bSASLastCommerceRectsCityScreen = None
		# <!-- custom: scoreboard scroll offset; 0 = show bottom of list (highest-ranked), increases to show lower-ranked players.
		# Reset when the scoreboard is rebuilt. (Claude code Sonnet 4.6) -->
		self.iScoreScrollOffset = 0
		self.iSAS_SCOREBOARD_LOCK_HOVER_DEFAULT_ENABLED = None
		# <!-- custom: always-expand scoreboard toggle; when True, expanded columns shown without hovering.
		# Initial state is seeded once, and the in-game lock button still toggles it normally. (Claude code Sonnet 4.6 + ChatGPT-5.5) -->
		self.bScoreAlwaysExpand = None
		self.iSAS_SCOREBOARD_SCROLL_AUTOCENTER_ACTIVE_PLAYER_DEFAULT_ENABLED = None
		# <!-- custom: scoreboard scroll autocenter toggle; when True, redraws keep the scroll slice centered on the active player.
		# Manual scroll buttons turn it off so deliberate strongest/weakest watching is preserved. (ChatGPT-5.5) -->
		self.bScoreScrollAutocenterActivePlayer = None
		# <!-- custom: scoreboard background style cycle (not just opacity: PanelStyles also include plain/colored panels e.g. blue/tan, not only opaque/translucent).
		# Discrete PanelStyle levels in cycle order.
		# 0 HUD_HELP is vanilla look.
		# If you add or reorder styles here, update the matching SAS_SCOREBOARD_BG_DEFAULT_STYLE legend comment in GlobalDefines_advciv_sas.xml to keep them in sync.
		# Starting level comes from the SAS_SCOREBOARD_BG_DEFAULT_STYLE define. (Claude code Opus 4.7) -->
		self.iScoreBgStyleLevel = None
		self.iSAS_SCOREBOARD_BG_DEFAULT_STYLE = None
		self.aScoreBgStyleLevels = [PanelStyles.PANEL_STYLE_HUD_HELP, PanelStyles.PANEL_STYLE_SOLID, PanelStyles.PANEL_STYLE_MAIN, PanelStyles.PANEL_STYLE_MAIN_BLACK50, PanelStyles.PANEL_STYLE_MAIN_BLACK25, PanelStyles.PANEL_STYLE_EMPTY]
		self._eScoreBgStyleApplied = PanelStyles.PANEL_STYLE_HUD_HELP
		# <!-- custom: main-map annotation visibility toggle; signs/landmarks have no Python visibility flag, so hide caches them and removes engine billboards until restored. (GPT-5.5) -->
		self.bAnnotationsVisible = True
		self.aHiddenAnnotationSigns = []

############## Basic operational functions ###################

	# advc.092 (note): Called externally (from PLE)
	def numPlotListButtonsPerRow(self):
		try: # advc.009b
			return self.m_iNumPlotListButtonsPerRow
		# <advc.009b>
		except AttributeError:
			return 0 # </advc.009b>
	# advc.092: So that PLE can access these
	def plotListUnitButtonSize(self):
		return BTNSZ(34)
	def unitButtonOverlaySize(self):
		return (12 * self.plotListUnitButtonSize()) / 34
	def plotListUnitFrameThickness(self):
		return 2

# I know that this is redundant, but CyInterface().getPlotListOffset() (and probably the column one too) uses this function
# it is also used in "...\EntryPoints\CvScreensInterface.py"
	def numPlotListButtons(self):
		return self.numPlotListButtonsPerRow()

	def numPlotListRows(self):
		return gc.getMAX_PLOT_LIST_ROWS()

	def numPlotListButtons_Total(self):
		return self.numPlotListButtonsPerRow() * self.numPlotListRows()

	def initState (self, screen=None):
		# Initialize screen instance (self.foo) and global variables.
		#
		# This function is called before drawing the screen (from interfaceScreen() below) and anytime the Python modules are reloaded (from CvEventInterface).
		#
		# THIS FUNCTION MUST NOT ALTER THE SCREEN -- screen.foo()
		#
		if screen is None:
			screen = CyGInterfaceScreen("MainInterface", CvScreenEnums.MAIN_INTERFACE)
		self.xResolution = screen.getXResolution()
		self.yResolution = screen.getYResolution()

		# <!-- custom: cache for perf opt with the help of ChatGPT-5.2 Thinking thanks.
		# Note: putting them in init function causes python errors, putting them here in initState seemingly solves them. -->
		self.colorResearchStored = getInfoTypeOrFail("COLOR_RESEARCH_STORED")
		self.colorResearchRate = getInfoTypeOrFail("COLOR_RESEARCH_RATE")
		self.colorEmpty = getInfoTypeOrFail("COLOR_EMPTY")
		self.colorNegativeRate = getInfoTypeOrFail("COLOR_NEGATIVE_RATE")
		self.colorGreatPeopleStored = getInfoTypeOrFail("COLOR_GREAT_PEOPLE_STORED")
		self.colorGreatPeopleRate = getInfoTypeOrFail("COLOR_GREAT_PEOPLE_RATE")
		self.colorGreatCultureStored = getInfoTypeOrFail("COLOR_CULTURE_STORED")
		self.colorGreatCultureRate = getInfoTypeOrFail("COLOR_CULTURE_RATE")
		self.colorRed = getInfoTypeOrFail("COLOR_RED")
		self.colorYellow = getInfoTypeOrFail("COLOR_YELLOW")
		self.colorGreen = getInfoTypeOrFail("COLOR_GREEN")
		self.colorAltHighlightText = getInfoTypeOrFail("COLOR_ALT_HIGHLIGHT_TEXT")
		self.colorBuildingText = getInfoTypeOrFail("COLOR_BUILDING_TEXT")
		#
		self.hurryPopulation = getInfoTypeOrFail("HURRY_POPULATION")
		self.hurryGold = getInfoTypeOrFail("HURRY_GOLD")
		#
		self.overlayFortifyPath = ArtFileMgr.getInterfaceArtInfo("OVERLAY_FORTIFY").getPath()
		self.overlayHasmovedPath = ArtFileMgr.getInterfaceArtInfo("OVERLAY_HASMOVED").getPath()
		self.overlayMovePath = ArtFileMgr.getInterfaceArtInfo("OVERLAY_MOVE").getPath()
		self.overlayNomovePath = ArtFileMgr.getInterfaceArtInfo("OVERLAY_NOMOVE").getPath()
		self.backgroundTransparentPath = ArtFileMgr.getInterfaceArtInfo("POPUPS_BACKGROUND_TRANSPARENT").getPath()
		self.buttonsCreategroupPath = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTONS_CREATEGROUP").getPath()
		self.buttonsSplitgroupPath = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTONS_SPLITGROUP").getPath()
		self.szScoreExpandTogglePath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_LOCKED_2").getPath()
		self.szScoreExpandToggleOffPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_UNLOCKED_2").getPath()
		self.szScoreScrollCenterActivePlayerPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_DIRECT_HIT_3").getPath()
		# <!-- custom: since it is yellowish like the toggle locked/unlocked scroll emojis as of now next to it, it helps making it less distracting and smoother to scan visually for the player perhaps. -->
		self.szScoreScrollAutocenterActivePlayerPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_COMPASS_3").getPath()
		self.szScoreScrollAutocenterActivePlayerOffPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_COMPASS_2").getPath()
		# <!-- custom: building filter button art paths using BUG icons (moved here from updateCityScreen for perf). (Claude Opus 4.5) -->
		self.buildFilterAllBuildingsOn = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTONS_CITYSELECTION").getPath()
		self.buildFilterAllBuildingsOff = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTONS_CITYSELECTION").getPath()
		self.buildFilterRegularBuildingsOn = ArtFileMgr.getInterfaceArtInfo("OVERLAY_ACTION_BUILD").getPath()
		self.buildFilterRegularBuildingsOff = ArtFileMgr.getInterfaceArtInfo("OVERLAY_ACTION_BUILD").getPath()
		self.buildFilterNatWonderOn = ArtFileMgr.getInterfaceArtInfo("BUG_NATWONDER_OFF").getPath()
		self.buildFilterNatWonderOff = ArtFileMgr.getInterfaceArtInfo("BUG_NATWONDER_OFF").getPath()
		self.buildFilterWorldWonderOn = ArtFileMgr.getInterfaceArtInfo("BUG_WORLDWONDER_OFF").getPath()
		self.buildFilterWorldWonderOff = ArtFileMgr.getInterfaceArtInfo("BUG_WORLDWONDER_OFF").getPath()
		# <!-- custom: building filter tooltip XML keys precomputed for efficiency. (Claude code Sonnet 4.5) -->
		self.szBuildFilterTooltipAll = "TXT_KEY_BUILDING_FILTER_ALL"
		self.szBuildFilterTooltipRegular = "TXT_KEY_BUILDING_FILTER_REGULAR"
		self.szBuildFilterTooltipNational = "TXT_KEY_BUILDING_FILTER_NATIONAL"
		self.szBuildFilterTooltipWorld = "TXT_KEY_BUILDING_FILTER_WORLD"
		#
		# <advc.092>
		gSetRectangle("Top", RectLayout(None, 0, 0, self.xResolution, self.yResolution))
		self.bScaleHUD = MainOpt.isEnlargeHUD()
		if self.bScaleHUD:
			# Divide by the aspects that the original HUD was (presumably) optimized for.
			xRatio = math.pow(self.xResolution / 1024.0, 0.4)
			yRatio = math.pow(self.yResolution / 768.0, 0.4)
			if xRatio > 1 and yRatio > 1:
				fHorizontalScaleFactor = min(xRatio, yRatio)
				# More is gained imo by vertical scaling; in part, due to change advc.137, which changes the minimap aspect ratio, but the original HUD also seems quite stretched horizontally. Apart from the main map height, vertical scaling happens at the expense of the sidebar height on the city screen.
				fVerticalScaleFactor = max(yRatio, min(fHorizontalScaleFactor * 1.05, max(xRatio, yRatio)))
				fSquareButtonScaleFactor = ((2 * min(fHorizontalScaleFactor, fVerticalScaleFactor) + 1) / 3)
				# Space should not be expanded quite as much as button sizes.
				# Space is also subject to vertical or horizontal scaling.
				# This division should ensure that space doesn't get expanded too much overall.
				fSpaceScaleFactor = (((2 * fSquareButtonScaleFactor + 1) / 3) / min(fHorizontalScaleFactor, fVerticalScaleFactor))
				# Already imported through import *, but need to call this setter on the LayoutDict module so that its state actually changes.
				import LayoutDict
				LayoutDict.gSetScaleFactors(fHorizontalScaleFactor, fVerticalScaleFactor, fSquareButtonScaleFactor, fSpaceScaleFactor)
				del LayoutDict
		# </advc.092>
		# advc.061:
		gc.getGame().setScreenDimensions(self.xResolution, self.yResolution)
# BUG - Raw Yields - begin
		global g_bYieldView
		global g_iYieldType
		g_bYieldView, g_iYieldType = RawYields.getViewAndType(CityScreenOpt.getRawYieldsDefaultView())
# BUG - Raw Yields - end

		# Set up our global variables...
		global g_NumEmphasizeInfos
		global g_NumCityTabTypes
		global g_NumHurryInfos
		global g_NumUnitClassInfos
		global g_NumBuildingClassInfos
		global g_NumProjectInfos
		global g_NumProcessInfos
		global g_NumActionInfos

		g_NumEmphasizeInfos = gc.getNumEmphasizeInfos()
		g_NumCityTabTypes = CityTabTypes.NUM_CITYTAB_TYPES
		g_NumHurryInfos = gc.getNumHurryInfos()
		g_NumUnitClassInfos = gc.getNumUnitClassInfos()
		g_NumBuildingClassInfos = gc.getNumBuildingClassInfos()
		g_NumProjectInfos = gc.getNumProjectInfos()
		g_NumProcessInfos = gc.getNumProcessInfos()
		g_NumActionInfos = gc.getNumActionInfos()

		# advc.004z: Moved ResourceIcons from last position to second and swapped BareMap and Yields. And moved this whole thing out of setMiniMapButtonVisibility, and storing some associated data too.
		self.aMiniMapMainButtons = [
			MiniMapButton("UnitIcons", ControlTypes.CONTROL_UNIT_ICONS, "Button_HUDGlobeUnit_Style"),
			MiniMapButton("ResourceIcons", ControlTypes.CONTROL_RESOURCE_ALL, "Button_HUDBtnResources_Style"),
			MiniMapButton("Grid", ControlTypes.CONTROL_GRID, "Button_HUDBtnGrid_Style"),
			MiniMapButton("Yields", ControlTypes.CONTROL_YIELDS, "Button_HUDBtnTileAssets_Style"),
			MiniMapButton("BareMap", ControlTypes.CONTROL_BARE_MAP, "Button_HUDBtnClearMap_Style"),
			# <!-- custom: annotation visibility toggle uses Civ4's turn-log notebook/pencil art; only the widget handler differs from the built-in minimap buttons. (GPT-5.5) -->
			MiniMapButton("AnnotationsVisible", None, "Button_HUDLog_Style"),
			MiniMapButton("ScoresVisible", ControlTypes.CONTROL_SCORES, "Button_HUDBtnRank_Style")
		]

		# advc.092: Static positional data
		# Start with the mini map because that's the number one thing that I would like to be adjusted to the screen resolution. Then base the size of the corner panels on the mini map size etc.
		# Will first need a template for the minimap buttons (their size affects the thickness of the frame around the minimap).
		gSetSquare("MiniMapButton", "Top", 0, 0, BTNSZ(28))
		# While we're at it; this one gets treated similarly.
		gSetSquare("GlobeToggle", "Top", 0, 0, BTNSZ(36))
		self.setMiniMapRects()

		# <!-- custom: enlarge the bottom panels now that we increased self.SIDE_PANELS_WIDTH to fit more info, so that the bottom panel is aligned with the enlarged side panels (that have bonuses, etc.), as chatgpt 5.2 confirmed thanks a lot.
		# Note: somehow width is slightly too low and not enough with just self.SIDE_PANELS_WIDTH, so adding some extra manual adjustment.
		# Doing it this way i could confirm chatgpt 5.2's explanation seems correct (check if accurate) -->
		# also ensure CenterBottomPanel starts to the right of CityLeftPanel (needs +8 because LowerLeftCorner trims -8).
		iEstimatedBottomPanelsExtraWidth = 8
		iEstimatedBottomPanelsWidth = self.SIDE_PANELS_WIDTH + iEstimatedBottomPanelsExtraWidth
		# HLEN(304), gRect("MiniMapPanel").height() + 17)
		gSetRect("LowerLeftCornerPanel", "Top", RectLayout.LEFT, RectLayout.BOTTOM, HLEN(iEstimatedBottomPanelsWidth), gRect("MiniMapPanel").height() + 17)
		gSetRect("LowerRightCornerPanel", "Top", RectLayout.RIGHT, RectLayout.BOTTOM, gRect("LowerLeftCornerPanel").width(), gRect("LowerLeftCornerPanel").height())

		gSetRect("LowerLeftCornerBackgr", "LowerLeftCornerPanel", 0, 4, RectLayout.MAX, RectLayout.MAX)
		# The panel dimensions include decorations. The rest of the HUD is mostly based on the undecorated size, so let's define ...
		gSetRect("LowerLeftCorner", "LowerLeftCornerPanel", 0, 19, -8, RectLayout.MAX)
		# <!-- custom: reduce left margin to minimize gap between draft bar and minimap (matching C2C-style tight layout) (Claude code Opus 4.5) -->
		# was 8 - set to 0 to eliminate gap completely
		gSetRect("LowerRightCorner", "LowerRightCornerPanel", 0, 19, RectLayout.MAX, RectLayout.MAX)
		# <!-- custom: add some HSPACE to fix the weird ugly empty gap until we reach the right side panel's left edge with the help of GPT-5.2-Codex thanks -->
		# Hardly gain anything from scaling this up
		gSetRect("CenterBottomPanel", "Top", gRect("LowerLeftCorner").xRight(), RectLayout.BOTTOM, -gRect("LowerRightCornerPanel").width() + HSPACE(10), HLEN(133, 0.5))
		# Sans the decoration on top
		gSetRect("CenterBottom", "CenterBottomPanel", 0, 17, RectLayout.MAX, RectLayout.MAX)

		# <!-- custom: use more space on the sides for the side panels rather than city plots that we don't need to show really beyond BFC.
		# Done with the help of gemini 3 pro thanks -->
		# HLEN(258), -gRect("LowerLeftCorner").height())
		gSetRect("CityLeftPanel", "Top", 0, 0, HLEN(self.SIDE_PANELS_WIDTH), -gRect("LowerLeftCorner").height())
		gSetRect("CityRightPanel", "Top", RectLayout.RIGHT, 0, gRect("CityLeftPanel").width(), -gRect("LowerRightCorner").height())
		self.setTopBarRects()
		gSetRect("CityCenterColumn", "Top", gRect("CityLeftPanel").xRight(), 0, -gRect("CityRightPanel").width(), RectLayout.MAX)
		gSetRect("CityScreenTopWidget", "Top", 0, -2, RectLayout.MAX, gRect("TopBarsOneLineContainer").yBottom() + VLEN(12), True)

		self.setCityNameRects()

		gSetRect("LeftHalf", "Top", 0, 0, gRect("Top").width() / 2, RectLayout.MAX)
		gSetRect("RightHalf", "Top", gRect("LeftHalf").xRight(), 0, RectLayout.MAX, RectLayout.MAX)
		# Background for top bars
		gSetRect("TopCityPanelLeft", "LeftHalf", gRect("CityNameBackground").x(), gRect("CityNameBackground").yBottom() + 1, RectLayout.MAX, 2 * (self.stackBarDefaultHeight() + VSPACE(3)))
		gSetRect("TopCityPanelRight", "RightHalf", 0, gRect("TopCityPanelLeft").y(), gRect("CityNameBackground").width() / 2, gRect("TopCityPanelLeft").height())
		# The two panels above really form a single panel; they're separate only so that the color fades away toward the middle. Let's put them together:
		gSetRect("TopCityPanelLeftAndRight", "Top", gRect("TopCityPanelLeft").x(), gRect("TopCityPanelLeft").y(), gRect("TopCityPanelLeft").width() + gRect("TopCityPanelRight").width(), gRect("TopCityPanelLeft").height())
		gSetRect("InterfaceTopCenter", "Top", gRect("CityLeftPanel").xRight() + 1, -2, -(gRect("CityRightPanel").width() - 1), gRect("TopBarsOneLineContainer").yBottom() + 19, True)
		gSetRect("InterfaceTopLeft", "Top", RectLayout.LEFT, gRect("InterfaceTopCenter").y(), gRect("CityLeftPanel").width() + 9, gRect("InterfaceTopCenter").height() + 12, True)
		gSetRect("InterfaceTopRight", "Top", RectLayout.RIGHT, gRect("InterfaceTopLeft").y(), gRect("InterfaceTopLeft").width(), gRect("InterfaceTopLeft").height(), True)
		if self.bScaleHUD:
			# Needs to be a little slimmer because I can't change the (default) position of the Turn Log
			gRect("InterfaceTopLeft").adjustSize(0, -3)
		# I can only get this panel to work correctly at its original height
		iTopCityPanelCenterHeight = 140
		gSetRect("TopCityPanelCenter", "CityCenterColumn", 0, 0, RectLayout.MAX, iTopCityPanelCenterHeight)
		# To work around this limitation, I'm adding a copy of the troublesome panel that will be displaced toward the bottom when scaling the HUD height.
		iTopCityPanelCenterHeightTarget = gRect("TopCityPanelLeftAndRight").yBottom() + VSPACE(5)
		iVStagger = max(0, iTopCityPanelCenterHeightTarget - iTopCityPanelCenterHeight)
		gSetRect("TopCityPanelCenterStagger", "TopCityPanelCenter", 0, iVStagger, RectLayout.MAX, iTopCityPanelCenterHeight)
		# Left and Right panel have a similar issue, affecting the borders at their inner edge. Need those to line up with TopCityPanelCenterStagger.
		# Doesn't seem to hurt to just place the Left and Right panel (a bit) farther to the bottom, I guess because their upper edge overlaps with a different panel, i.e. copies aren't really needed - but I think this more explicit approach will be less confusing.
		gSetRect("CityLeftPanelStagger", "CityLeftPanel", 0, iVStagger, RectLayout.MAX, gRect("CityLeftPanel").height() - iVStagger)
		gSetRect("CityRightPanelStagger", "CityRightPanel", 0, iVStagger, RectLayout.MAX, gRect("CityRightPanel").height() - iVStagger)

		self.setCityCenterBarRects()
		self.setAdvisorButtonRects()
		self.setTopRightButtonRects()
		gOffSetPoint("TimeText", "TopRightButtons", -HSPACE(1), (4 * 24) / gRect("TopRightButtons").height())
		# advc: BtS had used 27, BUG (always) 10.
		if self.bScaleHUD or MainOpt.isShowOptionsButton():
			iTurnLogBtnMargin = 10
		else:
			iTurnLogBtnMargin = 18
		gSetSquare("TurnLogButton", "Top", iTurnLogBtnMargin, gRect("AdvisorButtons").y() - 1, gRect("AdvisorButtons").height())
		# <!-- custom: score-scroll button placeholder rects; actual position is set at runtime in updateScoreStrings via moveItem because GlobeToggle.y() is still 0 at init time. (Claude code Sonnet 4.6) -->
		iSScrollBtnSz = BTNSZ(20)
		gSetRect("ScoreScrollUp", "Top", 0, 0, iSScrollBtnSz, iSScrollBtnSz)
		gSetRect("ScoreScrollDown", "Top", 0, 0, iSScrollBtnSz, iSScrollBtnSz)
		gSetRect("ScoreScrollUpFast", "Top", 0, 0, iSScrollBtnSz, iSScrollBtnSz)
		gSetRect("ScoreScrollDownFast", "Top", 0, 0, iSScrollBtnSz, iSScrollBtnSz)
		gSetRect("ScoreBgStyle", "Top", 0, 0, iSScrollBtnSz, iSScrollBtnSz)
		gSetRect("ScoreExpandToggle", "Top", 0, 0, iSScrollBtnSz, iSScrollBtnSz)
		gSetRect("ScoreScrollCenterActivePlayer", "Top", 0, 0, iSScrollBtnSz, iSScrollBtnSz)
		gSetRect("ScoreScrollAutocenterActivePlayerToggle", "Top", 0, 0, iSScrollBtnSz, iSScrollBtnSz)
		if self.bScaleHUD:
			# To make room for the Turn Log, whose (default) position I can't change.
			gRect("TurnLogButton").move(6, -3)
		lBUGOptBtn = gRect("TurnLogButton").copy()
		lBUGOptBtn.move(gRect("TurnLogButton").size() + HSPACE(4), 0)
		gSetRectangle("BUGOptionsScreenWidget", lBUGOptBtn)
		iLeftTopTextX = (gRect("TurnLogButton").x() * 2) / 3
		bTopLeftFlagAndLeftSideTextShift = self.IS_SAS_CV_MAIN_INTERFACE_TOP_LEFT_FLAG_AND_LEFT_SIDE_TEXT_SHIFT
		if bTopLeftFlagAndLeftSideTextShift is None:
			bTopLeftFlagAndLeftSideTextShift = (gc.getDefineINT("SAS_CV_MAIN_INTERFACE_TOP_LEFT_FLAG_AND_LEFT_SIDE_TEXT_SHIFT") > 0)
		if bTopLeftFlagAndLeftSideTextShift:
			# <!-- custom: when this define is off, no flag is drawn here, so the left text (e.g. treasury) keeps its base position (no extra room needed). (GPT-5.3-Codex) -->
			iLeftTopTextX += HSPACE(18)
		gSetPoint("TimeText", PointLayout(iLeftTopTextX, gPoint("TimeText").y()))
		gSetPoint("EraText", PointLayout(gRect("CityLeftPanel").xRight() - HSPACE(8), gPoint("TimeText").y()))
		gOffSetPoint("GoldText", "TopRightButtons", -HSPACE(1), (4 * 24) / gRect("TopRightButtons").height())
		self.setCommerceAdjustRects()
		self.bSASLastCommerceRectsCityScreen = CyInterface().isCityScreenUp()

		# (gets centered through text alignment)
		# <!-- custom: collided with the diplo screen and was rendered under it (z-order); moved it lower visually, under it (y) instead. See KI#135. (Claude code Opus 4.7); was -VSPACE(25). -->
		iEndTurnTextYAdjustment = VSPACE(25)
		gSetPoint("EndTurnText", PointLayout(0, max(gRect("LowerLeftCornerPanel").y(), gRect("LowerRightCornerPanel").y()) + iEndTurnTextYAdjustment))
		# Gets moved to a lower position when interface is minimized
		gSetPoint("EndTurnTextMin", PointLayout(0, gRect("Top").yBottom() - VSPACE(86)))
		iEndTurnBtnSz = BTNSZ(32)
		iEndTurnBtnX = gRect("MiniMapPanel").x() - iEndTurnBtnSz - HSPACE(6)
		iEndTurnBtnY = gRect("LowerRightCorner").yBottom() - iEndTurnBtnSz - VSPACE(4)
		if not CyInterface().isCityScreenUp():
			iEndTurnBtnY = gRect("MiniMapPanel").y()
		gSetSquare("EndTurnButton", "Top", iEndTurnBtnX, iEndTurnBtnY, iEndTurnBtnSz)
		# <!-- custom: compact BtS-ratio top-left flag slot; runtime drawing is toggled in updateFlag. (GPT-5.3-Codex) -->
		iFlagWidth = 24
		iFlagHeight = iFlagWidth * 250 / 68
		# <!-- custom: clamp to visible screen bounds; negative top-left offsets can hide the flag widget entirely on some HUD layouts. (GPT-5.3-Codex) -->
		iFlagX = max(0, gRect("InterfaceTopLeft").x() + HSPACE(2))
		iFlagY = max(0, gRect("InterfaceTopLeft").y() + VSPACE(0))
		gSetRect("CivilizationFlag", "Top", iFlagX, iFlagY, iFlagWidth, iFlagHeight)
		# CivFlagArea kept for compatibility, sized to fit the flag.
		gSetRect("CivFlagArea", "CivilizationFlag", 0, 0, RectLayout.MAX, RectLayout.MAX)
# BUG - City Arrows - start
		lMainCityScrollButtons = RowLayout(gRect("Top"), gRect("EndTurnButton").x() + gRect("EndTurnButton").width() / 2, gRect("EndTurnButton").y() + VSPACE(24), 2, HSPACE(-12), BTNSZ(32))
		lMainCityScrollButtons.move(-lMainCityScrollButtons.width() / 2, 0)
		gSetRectangle("MainCityScrollButtons", lMainCityScrollButtons)
		gSetRectangle("MainCityScrollMinus", lMainCityScrollButtons.next())
		gSetRectangle("MainCityScrollPlus", lMainCityScrollButtons.next())

		# <!-- custom: position UnitCycleButton to the left of city arrows (Claude code Opus 4.5) -->
		iUnitCycleBtnSz = BTNSZ(32)
		iUnitCycleBtnX = gRect("MainCityScrollButtons").x() - iUnitCycleBtnSz - HSPACE(4)
		iUnitCycleBtnY = gRect("MainCityScrollButtons").y()
		gSetSquare("UnitCycleButtonPos", "Top", iUnitCycleBtnX, iUnitCycleBtnY, iUnitCycleBtnSz)

		# <!-- custom: wrap end turn and city change arrows with a panel with border effects with the help of GPT-5.2-Codex thanks a lot -->
		iEndTurnPanelX = min(gRect("EndTurnButton").x(), gRect("MainCityScrollButtons").x()) - HSPACE(8)
		iEndTurnPanelY = min(gRect("EndTurnButton").y(), gRect("MainCityScrollButtons").y()) - VSPACE(6)
		iEndTurnPanelRight = max(gRect("EndTurnButton").xRight(), gRect("MainCityScrollButtons").xRight()) + HSPACE(8)
		iEndTurnPanelBottom = max(gRect("EndTurnButton").yBottom(), gRect("MainCityScrollButtons").yBottom()) - VSPACE(2)
		gSetRect("EndTurnButtonPanel", "Top", iEndTurnPanelX, iEndTurnPanelY, iEndTurnPanelRight - iEndTurnPanelX, iEndTurnPanelBottom - iEndTurnPanelY)
		# <!-- custom: add left edge panel to complete the border frame (Claude code Opus 4.5) -->
		gSetRect("EndTurnButtonPanelLeft", "Top", iEndTurnPanelX - HSPACE(28), iEndTurnPanelY, HSPACE(60), iEndTurnPanelBottom - iEndTurnPanelY)

# BUG - City Arrows - end

		self.setCityTabRects()
		self.setCityTaskRects()
		# The height of the bottom button list isn't known here, but the width is - and that width is needed in other places. So let's define a helper rectangle with maximal height.
		# <!-- custom: expand production chooser fairly close to the left edge of CityTaskBtns. (GPT-5.2-Codex) -->
		iLMargin = HSPACE(10)
		iLeft = gRect("CenterBottom").x() + iLMargin
		iRight = gRect("MiniMapPanel").x() - HSPACE(2)
		iWidth = iRight - iLeft
		iMinWidth = gRect("CenterBottom").width() - 2 * iLMargin
		if iWidth < iMinWidth:
			iWidth = iMinWidth
			iLeft = gRect("CenterBottom").x() + iLMargin
		gSetRect("BottomButtonMaxSpace", "Top", iLeft, gRect("CenterBottom").y(), iWidth, gRect("CenterBottom").height())
		self.setInfoPaneRects()
# BUG - field of view slider - start
		# <advc.090>
		self.iFoVLabelLower = 10
		self.iFoVLabelUpper = 100
		#self.iW_FoVSlider = 100
		iW_FoVSlider = min(max(100, self.xResolution / 12), 200)
		iW_FoVSlider -= (iW_FoVSlider % 2)
		#self.iX_FoVSlider = self.xResolution - 120
		gSetRect("FoVSlider", "Top", -HSPACE(18), gRect("AdvisorButtons").yBottom() + VSPACE(4), iW_FoVSlider, VLEN(15))
		gOffSetPoint("FoVSliderText", "FoVSlider", -HSPACE(6), VSPACE(3))
		# </advc.090>
		# K-Mod (bigger FoW for bigger monitors. They'll appreciate it. Trust me.)
		#self.DEFAULT_FIELD_OF_VIEW = max(40, min(80, self.xResolution / 30))
		#if MainOpt.isRememberFieldOfView():
			#self.iField_View = int(MainOpt.getFieldOfView())
			# K-Mod
			#if self.iField_View < 0:
				#self.iField_View = self.DEFAULT_FIELD_OF_VIEW
			# K-Mod end
		#else:
			#self.iField_View = self.DEFAULT_FIELD_OF_VIEW

		# <advc.004m> Replacing the above. Will have to ignore the XML setting (like K-Mod does) to avoid recursion - BUG stores the value computed for self.DEFAULT_FIELD_OF_VIEW in CvGlobals, overwriting the FIELD_OF_VIEW set through XML until the game is restarted.
		# 42.0 originally (FIELD_OF_VIEW in GlobalDefines)
		self.DEFAULT_FIELD_OF_VIEW = 37.0
		#aspectFactor = pow((0.8 * self.xResolution) / self.yResolution, 0.72)
		if ((not MainOpt.isRememberFieldOfView() and not MainOpt.isShowFieldOfView()) or int(MainOpt.getFieldOfView())) < 0:
		#	self.DEFAULT_FIELD_OF_VIEW = int(max(self.DEFAULT_FIELD_OF_VIEW, min(2 * self.DEFAULT_FIELD_OF_VIEW, (aspectFactor * self.xResolution) / max(70 - self.DEFAULT_FIELD_OF_VIEW, 10))))
			# Better to adjust only to the smaller aspect.
			# Bigger screen also tends to mean that the player sits farther away.
			if self.yResolution > 1024:
				self.DEFAULT_FIELD_OF_VIEW = int((self.DEFAULT_FIELD_OF_VIEW * self.yResolution) / 1024)
			self.iField_View = self.DEFAULT_FIELD_OF_VIEW
		else:
			self.iField_View = int(MainOpt.getFieldOfView())
		# </advc.004m>
# BUG - field of view slider - end
# BUG - Progress Bar - Tick Marks - start
		self.pTwoLineResearchBar = self.createProgressBar("TwoLineResearchBar", "COLOR_RESEARCH_RATE", ProgressBarUtil.TICK_MARKS, True)
		self.pTwoLineResearchBar.addBarItem("TwoLineResearchBar")
		self.pTwoLineResearchBar.addBarItem("ResearchText")
		self.pTwoLineResearchBar.addBarItem("ResearchIconText")
		# BUG - Bars on single line for higher resolution screens - start
		self.pOneLineResearchBar = self.createProgressBar("OneLineResearchBar", "COLOR_RESEARCH_RATE", ProgressBarUtil.TICK_MARKS, True)
		self.pOneLineResearchBar.addBarItem("OneLineResearchBar")
		self.pOneLineResearchBar.addBarItem("ResearchText")
		self.pOneLineResearchBar.addBarItem("ResearchIconText")
		# BUG - Bars on single line for higher resolution screens - end
		self.pBarPopulationBar = self.createProgressBar("PopulationBar", gc.getYieldInfo(YieldTypes.YIELD_FOOD).getColorType(), ProgressBarUtil.SOLID_MARKS, True)
		self.pBarPopulationBar.addBarItem("PopulationBar")
		self.pBarPopulationBar.addBarItem("PopulationText")
		self.pBarProductionBar = self.createProgressBar("ProductionBar", gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getColorType(), ProgressBarUtil.TICK_MARKS, True)
		self.pBarProductionBar.addBarItem("ProductionBar")
		self.pBarProductionBar.addBarItem("ProductionText")
		self.pBarProductionBar.addBarItem("ProductionIconText")
		self.pBarProductionBar_Whip = self.createProgressBar("ProductionBar-Whip", "COLOR_YELLOW", ProgressBarUtil.CENTER_MARKS, False)
		self.pBarProductionBar_Whip.addBarItem("ProductionBar")
		self.pBarProductionBar_Whip.addBarItem("ProductionText")
		self.pBarProductionBar_Whip.addBarItem("ProductionIconText")
# BUG - Progress Bar - Tick Marks - end

		# Helper rects for uniform widget placement inside the side panels
		# <!-- custom: note: old CityOrgArea code removed since we don't put religions and corporations in the right side panel anymore -->
		# <!-- custom: move the Bonuses up, but after deleting the old religions and corporations, somehow they are too high, and somehow the higher this value is the lower bonuses are so increasing it to put bonuses down; was +VSPACE(4) -->
		hMoveBonusesDownTooHigh = +VSPACE(35)
		gSetRect("CityLeftPanelContents", "CityLeftPanel", RectLayout.CENTER, gRect("CityAdjustPanel").yBottom(), -HSPACE(9), gRect("LowerLeftCornerPanel").y() - gRect("LowerLeftCorner").y() + 4)
		gSetRect("CityRightPanelContents", "CityRightPanel", RectLayout.CENTER, hMoveBonusesDownTooHigh, -HSPACE(9), gRect("LowerRightCornerPanel").y() - gRect("LowerRightCorner").y() + 4)
		# <!-- custom: move culture bars to the right panel under Great People. (GPT-5.2-Codex) -->
		iBarH = self.stackBarDefaultHeight()
		iBarGap = VSPACE(-1)
		iCultY = gRect("CityRightPanelContents").yBottom() - iBarH
		iGPY = iCultY - iBarH - iBarGap
		gSetRect("GreatPeopleBar", "CityRightPanelContents", 0, iGPY - gRect("CityRightPanelContents").y(), RectLayout.MAX, iBarH)
		gSetRect("CultureBar", "CityRightPanelContents", 0, iCultY - gRect("CityRightPanelContents").y(), RectLayout.MAX, iBarH)
		gSetRect("NationalityBar", "Top", gRect("MiniMapPanel").x(), gRect("MiniMapPanel").y() - iBarH, gRect("MiniMapPanel").width(), iBarH)
		gSetRect("CultureBars", "CityRightPanelContents", 0, iGPY - gRect("CityRightPanelContents").y(), RectLayout.MAX, 2 * iBarH + iBarGap)
		gOffSetPoint("GreatPeopleText", "GreatPeopleBar", RectLayout.CENTER, self.stackBarDefaultTextOffset())
		self.setCitizenButtonRects()
		self.setCityBonusRects()

		self.setTradeRouteRects()
		gOffSetPoint("NationalityText", "NationalityBar", RectLayout.CENTER, self.stackBarDefaultTextOffset())
		gOffSetPoint("CultureText", "CultureBar", RectLayout.CENTER, self.stackBarDefaultTextOffset())
		self.setBuildingListRects()

		iPlotListUnitBtnSz = self.plotListUnitButtonSize()
		# <!-- custom: lazy-cache HARD-px unit-bar width adjustment; no resolution scaling because scoreboard width is font-driven, not resolution-scaled.
		# Negative shrinks row (fewer buttons per row), helping avoid scoreboard / off-screen overflow on multi-row stacks. See KI#124. (Claude code Opus 4.7) -->
		if self.iSAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_WIDTH_PIXEL_ADJUSTMENT is None:
			self.iSAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_WIDTH_PIXEL_ADJUSTMENT = gc.getDefineINT("SAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_WIDTH_PIXEL_ADJUSTMENT")
		self.m_iNumPlotListButtonsPerRow = ((gRect("BottomButtonMaxSpace").width() + self.iSAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_WIDTH_PIXEL_ADJUSTMENT - 2 * iPlotListUnitBtnSz) / iPlotListUnitBtnSz)
		# <!-- custom: cache resolution-scaled width adjustment for the left-side help text area; HLEN asserts on negatives, so dispatch via sign. (Claude code Opus 4.7) -->
		if self.iSAS_CV_MAIN_INTERFACE_DEFAULTHELPAREA_WIDTH_ADJUSTMENT is None:
			iHelpAreaWidthAdjustRaw = gc.getDefineINT("SAS_CV_MAIN_INTERFACE_DEFAULTHELPAREA_WIDTH_ADJUSTMENT")
			if iHelpAreaWidthAdjustRaw > 0:
				self.iSAS_CV_MAIN_INTERFACE_DEFAULTHELPAREA_WIDTH_ADJUSTMENT = HLEN(iHelpAreaWidthAdjustRaw)
			elif iHelpAreaWidthAdjustRaw < 0:
				self.iSAS_CV_MAIN_INTERFACE_DEFAULTHELPAREA_WIDTH_ADJUSTMENT = -HLEN(-iHelpAreaWidthAdjustRaw)
			else:
				self.iSAS_CV_MAIN_INTERFACE_DEFAULTHELPAREA_WIDTH_ADJUSTMENT = 0
		# (Also used for the PLE Info Pane)
		# (The center bottom panel doesn't fully scale up, so we shouldn't use that as our point of reference.)
		#-(VSPACE(5) + gRect("CenterBottomPanel").height() + self.plotListUnitButtonSize())
		# BtS leaves some space here, but we want the text area as low as possible to maximize the amount of text we can display (while also having tall corners for the sake of a large minimap).
		# The subtrahend should account for decorations on the corner panel; for visual alignment.
		#gRect("LowerLeftCornerPanel").width() - 21,
		# The help text really doesn't use that much width; it's been kept short with the original width in mind. So ...
		gSetRect("DefaultHelpArea", "Top", HSPACE(7), gRect("LowerLeftCornerBackgr").y(), max(280, gRect("LowerLeftCornerPanel").width() - HLEN(0.12 * gRect("LowerLeftCornerPanel").width())) + self.iSAS_CV_MAIN_INTERFACE_DEFAULTHELPAREA_WIDTH_ADJUSTMENT, 0)
		# As in BtS; pretty arbitrary and overlaps the unit pane.
		gSetRect("DefaultHelpAreaMin", "Top", HSPACE(7), -VSPACE(50), gRect("DefaultHelpArea").width(), 0)
		# advc.092: Moved down so that PLE can access the above
		self.PLE.PLE_CalcConstants(screen) # BUG - PLE
		self.setPromoButtonRects()
		# (BUG - unit plot draw method - advc.092: Moved into interfaceScreen method)

		# <!-- custom: precompute commonly used icon chars, strings, and constants for performance (claude opus 4.5) -->
		g = CyGame()
		# Integer icon codes (needed for localText.getText() calls and offset calculations)
		self.iDefenseIcon = g.getSymbolID(FontSymbols.DEFENSE_CHAR)
		self.iGreatPeopleIcon = g.getSymbolID(FontSymbols.GREAT_PEOPLE_CHAR)
		self.iWorstAttitudeIcon = g.getSymbolID(FontSymbols.WORST_ATTITUDE_CHAR)
		# Precomputed unicode icon strings (eliminates repeated u"%c" % formatting)
		self.szStarIcon = u"%c" % g.getSymbolID(FontSymbols.STAR_CHAR)
		self.szSilverStarIcon = u"%c" % g.getSymbolID(FontSymbols.SILVER_STAR_CHAR)
		self.szPowerIcon = u"%c" % g.getSymbolID(FontSymbols.POWER_CHAR)
		self.szOccupationIcon = u"%c" % g.getSymbolID(FontSymbols.OCCUPATION_CHAR)
		self.szEatenFoodIcon = u"%c" % g.getSymbolID(FontSymbols.EATEN_FOOD_CHAR)
		self.szAngryPopIcon = u"%c" % g.getSymbolID(FontSymbols.ANGRY_POP_CHAR)
		self.szHappyIcon = u"%c" % g.getSymbolID(FontSymbols.HAPPY_CHAR)
		self.szUnhappyIcon = u"%c" % g.getSymbolID(FontSymbols.UNHAPPY_CHAR)
		self.szHealthyIcon = u"%c" % g.getSymbolID(FontSymbols.HEALTHY_CHAR)
		self.szUnhealthyIcon = u"%c" % g.getSymbolID(FontSymbols.UNHEALTHY_CHAR)
		self.szGreatPeopleIcon = u"%c" % self.iGreatPeopleIcon
		self.szDefenseIcon = u"%c" % self.iDefenseIcon
		self.szMapIcon = u"%c" % g.getSymbolID(FontSymbols.MAP_CHAR)
		self.szReligionIcon = u"%c" % g.getSymbolID(FontSymbols.RELIGION_CHAR)
		self.szCitizenIcon = u"%c" % g.getSymbolID(FontSymbols.CITIZEN_CHAR)
		self.szMovesIcon = u"%c" % g.getSymbolID(FontSymbols.MOVES_CHAR)
		self.szStrengthIcon = u"%c" % g.getSymbolID(FontSymbols.STRENGTH_CHAR)
		self.szTradeIcon = u"%c" % g.getSymbolID(FontSymbols.TRADE_CHAR)
		self.szOpenBordersIcon = u"%c" % g.getSymbolID(FontSymbols.OPEN_BORDERS_CHAR)
		self.szDefensivePactIcon = u"%c" % g.getSymbolID(FontSymbols.DEFENSIVE_PACT_CHAR)
		# Precomputed yield/commerce icon strings (frequently used in city screen)
		self.szFoodIcon = u"%c" % gc.getYieldInfo(YieldTypes.YIELD_FOOD).getChar()
		self.szProductionIcon = u"%c" % gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar()
		self.szCultureIcon = u"%c" % gc.getCommerceInfo(CommerceTypes.COMMERCE_CULTURE).getChar()
		self.szGoldIcon = u"%c" % gc.getCommerceInfo(CommerceTypes.COMMERCE_GOLD).getChar()
		self.szEspionageIcon = u"%c" % gc.getCommerceInfo(CommerceTypes.COMMERCE_ESPIONAGE).getChar()
		# Precomputed constants
		self.iMoveDenominator = gc.getMOVE_DENOMINATOR()
		# <!-- custom: K-Mod/Base AdvCiv already key Info and Foreign Advisor text caches by language, and SAS correctly retained/extended that pattern in other advisor initText paths.
		# These Main Interface translations were instead cached only when initState ran, so changing language during a game left them in the previous language.
		# Retain the cache but key it by current language and refresh it from updateScreen. See KI#307. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		self.iLanguageLoaded = -1
		self.updateCachedLocalizedText()

	def updateCachedLocalizedText(self):
		iCurrentLanguage = CyGame().getCurrentLanguage()
		if self.iLanguageLoaded == iCurrentLanguage:
			return
		self.iLanguageLoaded = iCurrentLanguage
		self.szTextStrength = localText.getText("INTERFACE_PANE_STRENGTH", ())
		self.szTextAirStrength = localText.getText("INTERFACE_PANE_AIR_STRENGTH", ())
		self.szTextMovement = localText.getText("INTERFACE_PANE_MOVEMENT", ())
		self.szTextRange = localText.getText("INTERFACE_PANE_RANGE", ())
		self.szTextLevel = localText.getText("INTERFACE_PANE_LEVEL", ())
		self.szTextExperience = localText.getText("INTERFACE_PANE_EXPERIENCE", ())
		self.szTextStarving = localText.getText("INTERFACE_CITY_STARVING", ())
		self.szTextStagnant = localText.getText("INTERFACE_CITY_STAGNANT", ())
		self.szTextMaintenance = localText.getText("INTERFACE_CITY_MAINTENANCE", ())
		self.szTextDeadCiv = localText.getText("TXT_KEY_BUG_DEAD_CIV", ())
		# <!-- custom: KI#307's live-language refresh omitted four other persistent Main Interface translations; keep their cached lookup under the same language guard; preserve the Label wrappers added for waiting-state readability. See KI#1062. (GPT-5.6-Sol) -->
		self.szTextWaiting = sasFontTagLabel + localText.getText("SYSTEM_WAITING", ()) + SAS_FONT_TAG_CLOSE
		self.szTextEndTurn = localText.getText("SYSTEM_END_TURN", ())
		self.szTextWaitingForYou = sasFontTagLabel + localText.getText("SYSTEM_WAITING_FOR_YOU", ()) + SAS_FONT_TAG_CLOSE
		self.sFieldOfView_Text = localText.getText("TXT_KEY_BUG_OPT_MAININTERFACE__FIELDOFVIEW_TEXT", ())

	def setMiniMapRects(self):
		# <advc.137>
		iMiniMapHeight = VLEN(128) # was 122
		iDefaultMiniMapPanelWidth = iMiniMapHeight * 1625 / 1000 # BtS value
		# We can go a little wider
		iMaxMiniMapPanelWidth = iDefaultMiniMapPanelWidth + 8
		iMiniMapPanelWidth = iDefaultMiniMapPanelWidth
		# <!-- custom: move minimap to the right edge like C2C mod for cleaner look and more space. Credit: C2C mod (Caveman2Cosmos). (Claude code Opus 4.5) -->
		# Use RectLayout.RIGHT to flush minimap to right edge (was HSPACE(6) margin)
		if gc.getMap().isWrapX():
			# Avoid black bars on the sides
			iMiniMapPanelWidth = iMiniMapHeight * gc.getMap().getGridWidth()
			iMiniMapPanelWidth /= gc.getMap().getGridHeight()
			iMiniMapPanelWidth = min(iMiniMapPanelWidth, iMaxMiniMapPanelWidth)
			# Need enough space atop the panel for the minimap buttons (would be better to calculate here how much space those actually take up ...)
			iMiniMapPanelWidth = max(iMiniMapPanelWidth, 204)
		# </advc.137>
		iMiniMapVMargin = HSPACE(3)
		# Ideally should be placed inside the LowerRightCorner rect, but it's more intuitive to me to set the LowerRightCorner size based on the mini map (panel) size than vice versa, so I'm setting the mini map rects first.
		# Flush to right edge
		gSetRect("MiniMapPanel", "Top", RectLayout.RIGHT, RectLayout.BOTTOM, iMiniMapPanelWidth, iMiniMapHeight + gRect("MiniMapButton").size() / 2 + 3 + 2 * iMiniMapVMargin)
		# As in BtS - and 3 indeed looks too thin on the left.
		# advc.137: was 9
		gSetRect("MiniMap", "MiniMapPanel", 4, -5, -3, iMiniMapHeight)

# BUG - Great Person Bar - start
# Also BUG - Great General Bar,
# BUG - Bars on single line for higher resolution screens
	def setTopBarRects(self):
		gSetRect("TopBarsMaxSpace", "Top", gRect("CityLeftPanel").xRight() + HSPACE(8), VSPACE(2), -(gRect("CityRightPanel").width() + HSPACE(8)), 2 * self.stackBarDefaultHeight())
		# <!-- custom: on upscaled 1080p, shift one-line top-bar space from research to GP so GP text fits while research keeps enough room. (GPT-5.3-Codex) -->
		fResearchBarWidth = fThinResearchBarWidth = 415
		fGGBarWidth = 100
		fThinGGBarWidth = 84
		iSpacing = HSPACE(7)
		iThinSpacing = HSPACE(6)
		fThinGPBarWidth = 392
		fOneLineTotalWidth = float(fResearchBarWidth + fThinGPBarWidth + fThinGGBarWidth + 2 * iThinSpacing)
		if self.bScaleHUD: # Scale to use up the available space
			fMultOneLine = gRect("TopBarsMaxSpace").width() / fOneLineTotalWidth
			fResearchBarWidth *= fMultOneLine
			fThinGGBarWidth *= fMultOneLine
			fThinGPBarWidth *= fMultOneLine
			# Update total width
			fOneLineTotalWidth = (fResearchBarWidth + fThinGPBarWidth + fThinGGBarWidth + 2 * iThinSpacing)
			# Leave some room to make sure we don't overlap the Advisor buttons.
			# Not scaled because, if anything, the buttons protrude more on smaller res.
			fMultTwoLines = (gRect("TopBarsMaxSpace").width() - 15) / float(fThinResearchBarWidth)
			fThinResearchBarWidth *= fMultTwoLines
			fGGBarWidth *= fMultTwoLines
		fGPBarWidth = fThinResearchBarWidth - fGGBarWidth - iSpacing
		gSetRect("TopBarsTwoLineContainer", "Top", RectLayout.CENTER, VSPACE(2), fThinResearchBarWidth, gRect("TopBarsMaxSpace").height())
		gSetRect("TopBarsOneLineContainer", "Top", RectLayout.CENTER, gRect("TopBarsTwoLineContainer").y(), fOneLineTotalWidth, gRect("TopBarsMaxSpace").height() / 2)
		gSetRect("OneLineGGBar", "TopBarsOneLineContainer", 0, 0, fThinGGBarWidth, RectLayout.MAX)
		gSetRect("OneLineResearchBar", "TopBarsOneLineContainer", gRect("OneLineGGBar").width() + iThinSpacing, 0, fResearchBarWidth, RectLayout.MAX)
		gSetRect("OneLineGPBar", "TopBarsOneLineContainer", RectLayout.RIGHT, 0, fThinGPBarWidth, RectLayout.MAX)
		gSetRect("TwoLineResearchBar", "TopBarsTwoLineContainer", 0, 0, fThinResearchBarWidth, gRect("TopBarsTwoLineContainer").height() / 2)
		gSetRect("TwoLineGGBar", "TopBarsTwoLineContainer", 0, RectLayout.BOTTOM, fGGBarWidth, gRect("TopBarsTwoLineContainer").height() / 2)
		gSetRect("TwoLineGPBar", "TopBarsTwoLineContainer", RectLayout.RIGHT, RectLayout.BOTTOM, fGPBarWidth, gRect("TopBarsTwoLineContainer").height() / 2)
# BUG - Great Person Bar, BUG - Great General Bar - end

	def setCityNameRects(self):
		gSetRect("CityNameBackground", "CityCenterColumn", RectLayout.CENTER, gRect("CityScreenTopWidget").yBottom() - 8, -HSPACE(2), VSPACE(38, 0.25))
		gOffSetPoint("CityNameText", "CityNameBackground", RectLayout.CENTER, VLEN(3))
		gOffSetPoint("DefenseText", "CityNameBackground", RectLayout.RIGHT, VLEN(7, 0.7))
		gPoint("DefenseText").move(-HSPACE(2), 0)
		lCityScrollButtons = RowLayout(gRect("CityNameBackground"), HSPACE(16), RectLayout.CENTER, 2, HSPACE(-18, 0.5), BTNSZ(32))
		gSetRectangle("CityScrollButtons", lCityScrollButtons)
		gSetRectangle("CityScrollMinus", lCityScrollButtons.next())
		gSetRectangle("CityScrollPlus", lCityScrollButtons.next())

	def setCityCenterBarRects(self):
		# <!-- custom: shorten top city food/production bars so upscaled text does not overflow at 1080p when text is upscaled; keep both bars matched by sharing the same inset. (GPT-5.3-Codex); also since we increased base font size from body to label, this preserves the original distance from food/production bars' outside text to side panels feel -->
		iYieldTextWidth = HLEN(160)
		gSetRect("PopulationBar", "TopCityPanelLeftAndRight", iYieldTextWidth, VSPACE(4), -iYieldTextWidth, self.stackBarDefaultHeight())
		gSetRect("ProductionBar", "TopCityPanelLeftAndRight", RectLayout.CENTER, -VSPACE(3), gRect("PopulationBar").width(), self.stackBarDefaultHeight())
		gSetRectangle("ProductionBar-Whip", gRect("ProductionBar")) # BUG - Progress Bar - Tick Marks
		gOffSetPoint("PopulationText", "PopulationBar", RectLayout.CENTER, self.stackBarDefaultTextOffset())
		gOffSetPoint("ProductionText", "ProductionBar", RectLayout.CENTER, self.stackBarDefaultTextOffset())
		iTextLMargin = HSPACE(3)
		# Left margin somehow looks bigger. Also need more space on the left (for the BUG food help)
		iTextRMargin = iTextLMargin + 3
		# NB: The "input texts" to the left will get right-aligned, the happy and health text to the right will get left-aligned.
		# BUG - Food Rate Hover - start
		gOffSetPoint("PopulationInputText", "PopulationBar", -iTextLMargin, self.stackBarDefaultTextOffset())
		# BUG - Food Rate Hover - end
		gOffSetPoint("ProductionInputText", "ProductionBar", -iTextLMargin, self.stackBarDefaultTextOffset())
		gOffSetPoint("HealthText", "PopulationBar", iTextRMargin + gRect("PopulationBar").width(), self.stackBarDefaultTextOffset())
		gOffSetPoint("HappinessText", "ProductionBar", iTextRMargin + gRect("ProductionBar").width(), self.stackBarDefaultTextOffset())

	def setAdvisorButtonRects(self):
		iSize = BTNSZ(28)
		# <!-- custom: advisor button order follows active shortcuts (F1/F2/F3...); Religion/Corporation buttons removed, and World Advisor uses F7. (GPT-5.5) -->
		advisorNames = [ "Domestic", "Policy", "ForeignDiplomacy", "Foreign", "Military", "Tech", "World", "Info", "Victory" ]
		if GameUtil.isEspionage():
			advisorNames.append("Espionage")
		iButtons = len(advisorNames)
		iSpacing = -iround(iSize / 10.0)
		if iButtons <= 10:
			iSpacing += 1
		lButtons = RowLayout(gRect("Top"), RectLayout.RIGHT, gRect("InterfaceTopRight").yBottom() - iSize - 3, iButtons, iSpacing, iSize)
		lButtons.move(-iSpacing / 3, 0)
		gSetRectangle("AdvisorButtons", lButtons)
		for szAdvisorName in advisorNames:
			gSetRectangle(szAdvisorName + "AdvisorButton", lButtons.next())

	def setTopRightButtonRects(self):
		lButtons = RowLayout(gRect("Top"), -HSPACE(4), VSPACE(2), 2, HSPACE(2), BTNSZ(24))
		gSetRectangle("TopRightButtons", lButtons)
		gSetRectangle("MainMenuButton", lButtons.next())
		gSetRectangle("InterfaceHelpButton", lButtons.next())

	def setCommerceAdjustRects(self):
		iMaxRows = CommerceTypes.NUM_COMMERCE_TYPES
		iBtnSize = BTNSZ(20)
		iVSpacing = VSPACE(-1)
		iHSpacing = HSPACE(0)
		iRowH = iBtnSize + iVSpacing
		iColumnW = iBtnSize + iHSpacing
		if CyInterface().isCityScreenUp():
			if self.iSAS_COMMERCE_CITY_X_OFFSET is None:
				self.iSAS_COMMERCE_CITY_X_OFFSET = gc.getDefineINT("SAS_CV_MAIN_INTERFACE_RIGHT_COMMERCE_BLOCK_CITY_SCREEN_X_OFFSET")
				self.iSAS_COMMERCE_CITY_Y_OFFSET = gc.getDefineINT("SAS_CV_MAIN_INTERFACE_RIGHT_COMMERCE_BLOCK_CITY_SCREEN_Y_OFFSET")
				self.IS_SAS_COMMERCE_CITY_ANCHOR_RIGHT = (gc.getDefineINT("SAS_CV_MAIN_INTERFACE_COMMERCE_BLOCK_CITY_SCREEN_ANCHOR_RIGHT") > 0)
			iRightCommerceBlockXOffset = self.iSAS_COMMERCE_CITY_X_OFFSET
			iRightCommerceBlockYOffset = self.iSAS_COMMERCE_CITY_Y_OFFSET
			bCommerceBlockAnchorRight = self.IS_SAS_COMMERCE_CITY_ANCHOR_RIGHT
		else:
			if self.iSAS_COMMERCE_MAP_X_OFFSET is None:
				self.iSAS_COMMERCE_MAP_X_OFFSET = gc.getDefineINT("SAS_CV_MAIN_INTERFACE_RIGHT_COMMERCE_BLOCK_X_OFFSET")
				self.iSAS_COMMERCE_MAP_Y_OFFSET = gc.getDefineINT("SAS_CV_MAIN_INTERFACE_RIGHT_COMMERCE_BLOCK_Y_OFFSET")
				self.IS_SAS_COMMERCE_MAP_ANCHOR_RIGHT = (gc.getDefineINT("SAS_CV_MAIN_INTERFACE_COMMERCE_BLOCK_MAP_ANCHOR_RIGHT") > 0)
			iRightCommerceBlockXOffset = self.iSAS_COMMERCE_MAP_X_OFFSET
			iRightCommerceBlockYOffset = self.iSAS_COMMERCE_MAP_Y_OFFSET
			bCommerceBlockAnchorRight = self.IS_SAS_COMMERCE_MAP_ANCHOR_RIGHT
		if bCommerceBlockAnchorRight:
			iPercentTextBaseX = gRect("InterfaceTopRight").x()
		else:
			iPercentTextBaseX = gRect("InterfaceTopLeft").x()
		iPercentTextX = iPercentTextBaseX + HSPACE(iRightCommerceBlockXOffset)
		# <!-- custom: Do not reserve two full button columns by shifting the map-view commerce sliders left: with expanded advisors, that hid useful +/- buttons under the advisor even when upscaled and with Min/Max buttons enabled.
		# Move the green commerce-rate values to a compact row below instead, so the slider block can shift right enough for the percent text and Min/Max controls to stay visible inside Tech Chooser and in normal map view. (GPT-5.5) -->
		iCommerceRowsTopOffset = VSPACE(iRightCommerceBlockYOffset)
		for i in range(iMaxRows):
			gSetPoint("PercentText" + str(i), PointLayout(iPercentTextX, gRect("InterfaceTopLeft").yBottom() + VSPACE(-8) + i * iRowH + iCommerceRowsTopOffset))
			if self.bScaleHUD:
				# Make room for the Turn Log, whose (default) position I can't change.
				gPoint("PercentText" + str(i)).move(0, -3)
		iPercentTextToButtonsX = 36 + HSPACE(56) - HSPACE(14)
		iX = iPercentTextX + iPercentTextToButtonsX # Space for the PercentText label
		for i in range(4): # Up to 4 buttons per row (2 for BUG - Min/Max Sliders)
			lSliderBtns = ColumnLayout(gRect("Top"), iX, gPoint("PercentText0").y(), iMaxRows, iVSpacing, iBtnSize)
			gSetRectangle("CommerceSliderBtns" + str(i), lSliderBtns)
			iX += iColumnW
		for i in range(iMaxRows):
			szPrefix = "RateText" + str(i)
			iY = gPoint("PercentText" + str(i)).y()
			gSetPoint(szPrefix + "BtS", PointLayout(gRect("CommerceSliderBtns1").xRight() + HSPACE(3), iY))
			# for BUG - Min/Max Sliders
			gSetPoint(szPrefix + "BUG", PointLayout(gRect("CommerceSliderBtns3").xRight() + HSPACE(3), iY))

			# <!-- custom: after we have changed our width (as of now dynamically adjusted and wider, in self.SIDE_PANELS_WIDTH), we need to anchor the commerce values to the right like the yields in trade routes and buildings nicely already were, but not the commerce values in the commerce panel.
			# Done with the help of chatgpt 5.1 (or was it 5.2?
			# Not sure as version changed just at end of prompt in chatgpt website ui.
			# thanks) thanks and also my own ideas as AIs had quite the trouble to find this but chatgpt 5.1 (or was it 5.2?)
			# pointed this point thanks which i then found more precisely how it could fix.
			# Make it so it is dynamic even if we change side width later and account for margins empirically -->
			iEstimatedCommerceRightMargin = 28
			iEstimatedLeftSideSize = 152
			# <!-- custom: was 70 -->
			iCommercePanelHorizontalSpacingToTheRight = self.SIDE_PANELS_WIDTH - iEstimatedLeftSideSize - iEstimatedCommerceRightMargin
			# (The horizontal spacing here seems pretty arbitrary)
			gSetPoint("CityPercentText" + str(i), PointLayout(gRect("CommerceSliderBtns3").xRight() + HSPACE(iCommercePanelHorizontalSpacingToTheRight, 3), iY))
		gSetPoint("CommerceRateSummaryText", PointLayout(gPoint("PercentText0").x(), gPoint("PercentText0").y() + iMaxRows * iRowH))

		# <advc.002b> Placing the buttons at just the same y coord as the labels
		# doesn't quite work out
		iSliderButtonsVerticalOffset = 25 - iBtnSize
		for i in range(4):
			gRect("CommerceSliderBtns" + str(i)).move(0, iSliderButtonsVerticalOffset)
		# </advc.002b>
		gSetPoint("MaintenanceText", PointLayout(gPoint("PercentText0").x() + 1, gRect("CommerceSliderBtns0").yBottom()))
		gSetPoint("MaintenanceAmountText", PointLayout(gPoint("CityPercentText0").x(), gPoint("MaintenanceText").y()))
		# The panel, which only appears on the city screen, needs to encompass all the widgets above - and be centered on the left city panel.
		iVSpace = VSPACE(5)
		gSetRect("CityAdjustPanel", "CityLeftPanel", RectLayout.CENTER, gPoint("PercentText0").y() - gRect("CityLeftPanel").y() - iVSpace, -1, gPoint("MaintenanceText").y() + iBtnSize + 1 - gPoint("PercentText0").y() + 2 * iVSpace)

	def setTradeRouteRects(self):
		gSetRect("TradeRouteListBackground", "CityLeftPanelContents", 0, gRect("CityAdjustPanel").yBottom() - gRect("CityLeftPanelContents").y() + VSPACE(4), RectLayout.MAX, self.cityScreenHeadingBackgrHeight())
		gOffSetPoint("TradeRouteListLabel", "TradeRouteListBackground", RectLayout.CENTER, self.cityScreenHeadingOffset())
		# One table row (at font size 1) takes up ca. 24 pixels.
		# BtS will show a scrollbar when there are > 4 trade cities.
		# (The upper limit in XML is 8 and the practical limit 5.)
		# At the lowest resolution, showing 5 would indeed leave too little room for buildings, but, if we scale up, we should aim at at least 5 rows.
		iRowH = 24
		iHeightForRows = VLEN(4 * iRowH, 2)

		# <!-- custom: remove the 3 gray bars ("Trade Routes", "Buildings", "Specialists") as they take a lot of room and are unneeded, and we don't have a "Bonuses" bar for example so no reason to have these as well either.
		# Change with the help of gemini 3 pro thanks -->
		# Step 2: Move the Tables Up (Reclaim Space)
		# 1. Trade Routes List Search for def setTradeRouteRects. Find the line defining TradeRouteTable. We will change .yBottom() to .y() so it starts at the top of the (now hidden) header space.
		# 1 + gRect("TradeRouteListBackground").yBottom() - gRect("CityLeftPanelContents").y(),
		# No point in making space for only half a row
		gSetRect("TradeRouteTable", "CityLeftPanelContents", RectLayout.CENTER, 1 + gRect("TradeRouteListBackground").y() - gRect("CityLeftPanelContents").y(), -1, 2 + iHeightForRows - iHeightForRows % iRowH)
		gRect("TradeRouteTable").adjustSize(HSPACE(self.iCityScreenYieldTableRightExtra), 0)

# BUG - Raw Yields - start
		# Can't really scale these up because the background doesn't scale
		iBtnSize = BTNSZ(24, 0.3)
		iHSpaceSmall = HSPACE(0)
		iHSpaceBig = HSPACE(10)

		lRawYieldBtns = RowLayout(gRect("TradeRouteListBackground"), 0, RectLayout.CENTER, YieldTypes.NUM_YIELD_TYPES, iHSpaceSmall, iBtnSize)
		lPlotSelBtns = RowLayout(gRect("TradeRouteListBackground"), 0, RectLayout.CENTER, 3, iHSpaceSmall, iBtnSize)
		iVOffset = 2 # Centering on the background widget doesn't look quite right
		# Buttons rows at x=0 until we've placed the RawYieldMenu container
		lRawYieldBtns.move(-gRect("TradeRouteListBackground").x(), iVOffset)
		lPlotSelBtns.move(-gRect("TradeRouteListBackground").x(), iVOffset)
		gSetRect("RawYieldMenu", "TradeRouteListBackground", RectLayout.CENTER, RectLayout.CENTER, lRawYieldBtns.width() + lPlotSelBtns.width() + 2 * iHSpaceBig + iBtnSize, lRawYieldBtns.height())
		gSetSquare("RawYieldsTrade0", "RawYieldMenu", 0, iVOffset, iBtnSize)
		lRawYieldBtns.move(gRect("RawYieldsTrade0").xRight() + iHSpaceBig, 0)
		lPlotSelBtns.move(lRawYieldBtns.xRight() + iHSpaceBig, 0)
		gSetRectangle("RawYieldsFood1", lRawYieldBtns.next())
		gSetRectangle("RawYieldsProduction2", lRawYieldBtns.next())
		gSetRectangle("RawYieldsCommerce3", lRawYieldBtns.next())
		gSetRectangle("RawYieldsWorkedTiles4", lPlotSelBtns.next())
		gSetRectangle("RawYieldsCityTiles5", lPlotSelBtns.next())
		gSetRectangle("RawYieldsOwnedTiles6", lPlotSelBtns.next())
# BUG - Raw Yields - end

	def setBuildingListRects(self):
		gSetRect("BuildingListBackground", "CityLeftPanelContents", 0, gRect("TradeRouteTable").yBottom() - gRect("CityLeftPanelContents").y() + VSPACE(4), RectLayout.MAX, self.cityScreenHeadingBackgrHeight())
		gOffSetPoint("BuildingListLabel", "BuildingListBackground", RectLayout.CENTER, self.cityScreenHeadingOffset())

		iTopAbs = gRect("BuildingListBackground").y() + VSPACE(5)
		iBottomAbs = gRect("LowerLeftCornerPanel").y()
		iRowH = BTNSZ(26)
		iAvailH = max(0, iBottomAbs - iTopAbs)
		iTableH = iAvailH - (iAvailH % iRowH) - VSPACE(3)
		gSetRect("BuildingListTable", "CityLeftPanelContents", RectLayout.CENTER, iTopAbs - gRect("CityLeftPanelContents").y(), -1, iTableH)
		gRect("BuildingListTable").adjustSize(HSPACE(self.iCityScreenYieldTableRightExtra), 0)

	def setCityTabRects(self):
		iButtons = 3
		# <!-- custom: These row-jump buttons were 24x24 in base AdvCiv when stacked vertically.
		# Now that we restored them beside the city task/building-filter row, use the same 32x28 medium city-button size as that row. (GPT-5.5) -->
		iMediumBtnW = 32
		iMediumBtnH = 28
		iCityTabSpacing = HSPACE(2)
		iBuildFilterSpacing = HSPACE(2)
		iBuildFilterRowW = 4 * iMediumBtnW + 3 * iBuildFilterSpacing
		iHurryRowW = 3 * iMediumBtnW + 2 * iBuildFilterSpacing
		iHurryRowX = gRect("LowerLeftCornerPanel").xRight() - iHurryRowW - HSPACE(16)
		iBuildFilterRowX = iHurryRowX - iBuildFilterRowW - HSPACE(6)
		iCityTabRowW = iButtons * iMediumBtnW + (iButtons - 1) * iCityTabSpacing
		iCityTabRowX = iBuildFilterRowX - iCityTabRowW - HSPACE(6)
		iCityTabRowY = gRect("LowerLeftCornerPanel").y() + VSPACE(2)
		gSetRect("CityTabs", "Top", iCityTabRowX, iCityTabRowY, iCityTabRowW, iMediumBtnH)
		lButtons = RowLayout(gRect("CityTabs"), 0, 0, iButtons, iCityTabSpacing, iMediumBtnW, iMediumBtnH)
		for i in range(iButtons):
			gSetRectangle("CityTab" + str(i), lButtons.next())

	def setCityTaskRects(self):
		# Liberate button (was removed by the expansions)
		#iBtnX = xResolution - 284
		#iBtnY = yResolution - 177
		#iBtnW = 64
		#iBtnH = 30

		# <!-- custom: move city screen config buttons above the map, and hurry buttons above the production queue. (GPT-5.2-Codex) -->
		iSmallBtnW = 22
		iSmallBtnH = 24
		iHighBtnH = 27
		iMediumBtnW = 32
		iMediumBtnH = 28
		iLargeBtnW = 64
		iLargeBtnH = 30
		iBarH = self.stackBarDefaultHeight()
		iConfigBtns = 8
		iConfigBtnSpacing = HSPACE(-1)
		iConfigRowW = iConfigBtns * iSmallBtnW + (iConfigBtns - 1) * iConfigBtnSpacing
		iCityTaskBtnsX = gRect("MiniMapPanel").xRight() - iConfigRowW + HSPACE(2)
		iCityTaskBtnsY = gRect("MiniMapPanel").y() - iBarH - iSmallBtnH - VSPACE(2)
		gSetRect("CityTaskBtns", "Top", iCityTaskBtnsX, iCityTaskBtnsY, iConfigRowW, iSmallBtnH)
		# CityTaskArea is kept for compatibility but sized to fit CityTaskBtns
		gSetRect("CityTaskArea", "CityTaskBtns", 0, 0, RectLayout.MAX, RectLayout.MAX)
		lConfigBtns = RowLayout(gRect("CityTaskBtns"), 0, 0, iConfigBtns, iConfigBtnSpacing, iSmallBtnW, iSmallBtnH)
		gSetRectangle("AutomateProduction", lConfigBtns.next())
		gSetRectangle("AutomateCitizens", lConfigBtns.next())
		for i in range(6):
			gSetRectangle("Emphasize" + str(i), lConfigBtns.next())

		iCityArrowBtnSz = BTNSZ(32)
		iCityArrowSpacing = HSPACE(-12)
		iCityArrowRowW = 2 * iCityArrowBtnSz + iCityArrowSpacing
		iCityArrowRowX = gRect("CityTaskBtns").x() - iCityArrowRowW - HSPACE(6)
		iCityArrowRowY = gRect("CityTaskBtns").y() - VSPACE(4)
		gSetRect("CityScrollButtons", "Top", iCityArrowRowX, iCityArrowRowY, iCityArrowRowW, iCityArrowBtnSz)
		lCityScrollButtons = RowLayout(gRect("CityScrollButtons"), 0, 0, 2, iCityArrowSpacing, iCityArrowBtnSz, iCityArrowBtnSz)
		gSetRectangle("CityScrollMinus", lCityScrollButtons.next())
		gSetRectangle("CityScrollPlus", lCityScrollButtons.next())

		iHurrySpacing = HSPACE(2)
		iHurryRowW = 3 * iMediumBtnW + 2 * iHurrySpacing
		iHurryRowX = gRect("LowerLeftCornerPanel").xRight() - iHurryRowW - HSPACE(16)
		iHurryRowY = gRect("LowerLeftCornerPanel").y() + VSPACE(2)
		iBuildFilterSpacing = HSPACE(2)
		iBuildFilterRowW = 4 * iMediumBtnW + 3 * iBuildFilterSpacing
		iBuildFilterRowX = iHurryRowX - iBuildFilterRowW - HSPACE(6)
		iBuildFilterRowY = iHurryRowY
		gSetRect("CityBuildFilterBtns", "Top", iBuildFilterRowX, iBuildFilterRowY, iBuildFilterRowW, iMediumBtnH)
		lBuildFilterBtns = RowLayout(gRect("CityBuildFilterBtns"), 0, 0, 4, iBuildFilterSpacing, iMediumBtnW, iMediumBtnH)
		gSetRectangle("BuildFilter0", lBuildFilterBtns.next())  # All
		gSetRectangle("BuildFilter1", lBuildFilterBtns.next())  # Buildings
		gSetRectangle("BuildFilter2", lBuildFilterBtns.next())  # National
		gSetRectangle("BuildFilter3", lBuildFilterBtns.next())  # World
		gSetRect("CityHurryBtns", "Top", iHurryRowX, iHurryRowY, iHurryRowW, iMediumBtnH)
		lHurryBtns = RowLayout(gRect("CityHurryBtns"), 0, 0, 3, iHurrySpacing, iMediumBtnW, iMediumBtnH)
		gSetRectangle("Hurry0", lHurryBtns.next())
		gSetRectangle("Hurry1", lHurryBtns.next())
		gSetRectangle("Conscript", lHurryBtns.next())
		gRect("Conscript").adjustSize(2, 4)
		gRect("Conscript").move(-4, -2)

	def setInfoPaneRects(self):
		# and the movement of promo buttons (incl. stack promos) is still to be revised
		gSetRect("SelectedUnitPanel", "LowerLeftCorner", RectLayout.CENTER, RectLayout.CENTER, -HSPACE(8), -VSPACE(9))
		# Doesn't quite look properly centered
		gRect("SelectedUnitPanel").move(-2, 2)
		iUnitInfoBtnSize = BTNSZ(24)
		if self.IS_SAS_CV_MAIN_INTERFACE_UNIT_INFO_BUTTON is None or self.IS_SAS_CV_MAIN_INTERFACE_UNIT_INFO_BUTTON:
			gSetSquare("SelectedUnitInfoButton", "SelectedUnitPanel", HSPACE(2), VSPACE(2), iUnitInfoBtnSize)
		else:
			iUnitInfoBtnSize = 0
		# Not going to add a fifth row, not going to increase the font size, so nothing is gained by making the table higher.
		gSetRect("SelectedUnitText", "SelectedUnitPanel", HSPACE(2), VLEN(30, 0.7), (gRect("SelectedUnitPanel").width() * 183) / 280, 102)
		gOffSetPoint("SelectedUnitLabel", "SelectedUnitPanel", HSPACE(8) + iUnitInfoBtnSize, VSPACE(2))
		gSetRect("SelectedCityText", "SelectedUnitPanel", HSPACE(2), RectLayout.CENTER, gRect("SelectedUnitText").width(), -1)
		# Use available height, maintain the BtS aspect ratio. Well, BtS had used 123x132. Mounted units and ships tend to be wider than tall, and there's no real harm in overlapping, with the table to the left, so I'll actually make it wider than high.
		# These dimensions only affect the 3D space in which the model is shown; won't distort the model itself.
		gSetRect("InterfaceUnitModel", "SelectedUnitPanel", gRect("SelectedUnitText").width() - HLEN(8), VLEN(2), 0, -HLEN(6))
		gRect("InterfaceUnitModel").adjustSize((gRect("InterfaceUnitModel").height() * 150) / 132, 0)
		# Align right if protruding too much
		gRect("InterfaceUnitModel").move(min(0, gRect("LowerLeftCorner").xRight() + HLEN(2) - gRect("InterfaceUnitModel").xRight()), 0)

	# These are for the info pane, so this function ties in with the above; however, mustn't be executed until the PLE constants have been set.
	def setPromoButtonRects(self):
		# Final positions will be set by calculatePromotionButtonPosition. I don't think the preliminary positions matter at all.
		iBtnSize = BTNSZ(24)
		for iPromo in range(gc.getNumPromotionInfos()):
			gSetSquare("PromotionButton" + str(iPromo), "Top", 180, -18, iBtnSize)
# BUG - PLE - begin
			gSetSquare(self.PLE.PLE_PROMO_BUTTONS_UNITINFO + str(iPromo), "Top", 180, -18, self.PLE.CFG_INFOPANE_BUTTON_SIZE)
# BUG - PLE - end
# BUG - Stack Promotions - start
			x, y = self.calculatePromotionButtonPosition(iPromo)
			gSetSquare("PromotionButtonCircle" + str(iPromo), "Top", x + (10 * iBtnSize) / 24, y + (10 * iBtnSize) / 24, (16 * iBtnSize) / 24)
# BUG - Stack Promotions - end

	def setCitizenButtonRects(self):
		# (This will be a bit ugly because the stuff is right-aligned and because it's really a 2D grid. The RectLayout classes are for left-aligned single-file layouts.)
		self.visibleSpecialists = []
		for i in range(gc.getNumSpecialistInfos() - 1, -1, -1):
			if (gc.getSpecialistInfo(i).isVisible()):
				self.visibleSpecialists.append(i)
		# Lots of buttons on the right panel; need to be careful about scaling those up.
		iCitizenBtnSize = BTNSZ(24, 0.3)
		iAdjustBtnSize = (20 * iCitizenBtnSize) / 24
		iSmallVSpace = VSPACE(3) # (one greater than in BtS)
		iBigVSpace = iCitizenBtnSize - iAdjustBtnSize + iSmallVSpace
		iSmallHSpace = HSPACE(2)
		iBigHSpace = HSPACE(4)

		# This value needs to be consistent with the FirstFreeSpecialist and AngryCitizenButtons rects
		# <!-- custom: make room for a new specialist breakdown, with the help of gemini 3 pro thanks -->
		# This is a great idea. Since you want to insert a "Specialist Breakdown" later, you need to create a gap between the Citizen Buttons and the Great Person Bar at the bottom.
		# To do this, we simply tell the game to position the Citizen/Specialist buttons higher up (further away from the bottom). This will naturally push them up towards the Bonus panel and leave empty space below them.
		# Add + self.cityScreenHeadingBackgrHeight() to the end of the calculation.
		# By adding self.cityScreenHeadingBackgrHeight() (which is the height of the gray bar you removed, ~30 pixels), you lift the entire specialist section up by that amount.
		# Since you hid the Specialist Header but didn't expand the Bonus Panel (Step 2.3), there was a "gap" at the top. Moving the specialists up will fill that top gap and open up a new 30-pixel gap at the bottom, right above the Great Person Bar, which is perfect for your future breakdown widget.
		# <!-- custom: add a bit more for beautification and nicer spacing -->
		# <!-- custom: raise the city-screen specialist stack by about one and a half citizen rows so SAS upscaling and the two-stack rows stay above the specialist breakdown without drifting too high. (GPT-5.5) -->
		iHeightRoomForSpecialistBreakdown = (self.cityScreenHeadingBackgrHeight() + 10 + ((3 * (iCitizenBtnSize + iSmallVSpace)) / 2) + self.cityScreenBottomLift())
		iBarsStackHeight = gRect("CultureBars").height()
		iNonAdjustablesOffset = (3 * iSmallVSpace + 2 * iCitizenBtnSize + iBarsStackHeight + iHeightRoomForSpecialistBreakdown)

		lDecreaseSpecialistButtons = ColumnLayout(gRect("CityRightPanelContents"), RectLayout.RIGHT, RectLayout.BOTTOM, len(self.visibleSpecialists), iBigVSpace, iAdjustBtnSize)
		lDecreaseSpecialistButtons.move(0, -iNonAdjustablesOffset)
		gSetRectangle("DecreaseSpecialistButtons", lDecreaseSpecialistButtons)
		for iSpecialist in self.visibleSpecialists:
			gSetRectangle("DecreaseSpecialist" + str(iSpecialist), lDecreaseSpecialistButtons.next())
		lIncreaseSpecialistButtons = ColumnLayout(gRect("CityRightPanelContents"), RectLayout.RIGHT, RectLayout.BOTTOM, len(self.visibleSpecialists), iBigVSpace, iAdjustBtnSize)
		lIncreaseSpecialistButtons.move(-lDecreaseSpecialistButtons.width() - iSmallHSpace, -iNonAdjustablesOffset)
		gSetRectangle("IncreaseSpecialistButtons", lIncreaseSpecialistButtons)
		for iSpecialist in self.visibleSpecialists:
			gSetRectangle("IncreaseSpecialist" + str(iSpecialist), lIncreaseSpecialistButtons.next())

		gSetRect("AdjustSpecialistButtons", "Top", lIncreaseSpecialistButtons.x(), lIncreaseSpecialistButtons.y(), lIncreaseSpecialistButtons.width() + lDecreaseSpecialistButtons.width(), lIncreaseSpecialistButtons.height())

		# Only a placeholder; the free-specialist row gets placed later.
		# <!-- custom: make room for a new specialist breakdown, with the help of gemini 3 pro thanks -->
		# Since you moved the sliders up by adding self.cityScreenHeadingBackgrHeight() to iNonAdjustablesOffset, you need to apply that same vertical shift to the Free Specialists and Angry Citizens so they move up with the rest of the group.
		# Update the .move() commands for both FirstFreeSpecialist and lAngryCitizenButtons by subtracting self.cityScreenHeadingBackgrHeight().
		gSetSquare("FirstFreeSpecialist", "CityRightPanelContents", RectLayout.RIGHT, RectLayout.BOTTOM, iCitizenBtnSize)
		gRect("FirstFreeSpecialist").move(-gRect("AdjustSpecialistButtons").width() - iBigHSpace, -iBarsStackHeight - iCitizenBtnSize - iSmallVSpace - iHeightRoomForSpecialistBreakdown)
		lAngryCitizenButtons = RowLayout(gRect("CityRightPanelContents"), RectLayout.RIGHT, RectLayout.BOTTOM, MAX_CITIZEN_BUTTONS, iSmallHSpace, iCitizenBtnSize)
		lAngryCitizenButtons.move(-gRect("AdjustSpecialistButtons").width() - iBigHSpace, -iBarsStackHeight - iHeightRoomForSpecialistBreakdown)
		gSetRectangle("AngryCitizenButtons", lAngryCitizenButtons)

		iChevronSize = BTNSZ(10)
		for i in range(MAX_CITIZEN_BUTTONS - 1, -1, -1):
			gSetRectangle("AngryCitizen" + str(i), lAngryCitizenButtons.next())
			gSetSquare("AngryCitizenChevron" + str(i), "AngryCitizen" + str(i), 0, 0, iChevronSize)
		lSpecialistDisabledButtons = ColumnLayout(gRect("CityRightPanelContents"), RectLayout.RIGHT, RectLayout.BOTTOM, len(self.visibleSpecialists), iSmallVSpace, iCitizenBtnSize)
		lSpecialistDisabledButtons.move(-gRect("AdjustSpecialistButtons").width() - iBigHSpace, -iNonAdjustablesOffset)
		gSetRectangle("SpecialistDisabledButtons", lSpecialistDisabledButtons)
		for iSpecialist in self.visibleSpecialists:
			gSetRectangle("SpecialistDisabledButton" + str(iSpecialist), lSpecialistDisabledButtons.next())

		for i in range(len(self.visibleSpecialists)):
			iSpecialist = self.visibleSpecialists[i]
			lCitizenButtonRow = RowLayout(gRect("CityRightPanelContents"), RectLayout.RIGHT, RectLayout.BOTTOM, MAX_CITIZEN_BUTTONS, iSmallHSpace, iCitizenBtnSize)
			lCitizenButtonRow.move(-gRect("AdjustSpecialistButtons").width() - iBigHSpace, -iNonAdjustablesOffset - (len(self.visibleSpecialists) - i - 1) * (iCitizenBtnSize + iSmallVSpace))
			gSetRectangle("CitizenButtonRow" + str(iSpecialist), lCitizenButtonRow)
			for j in range(MAX_CITIZEN_BUTTONS - 1, -1, -1):
				szNum = str((iSpecialist * 100) + j)
				lBtn = lCitizenButtonRow.next()
				gSetRectangle("CitizenButton" + szNum, lBtn)
				gSetRectangle("CitizenButtonHighlight" + szNum, lBtn)
				gSetSquare("CitizenChevron" + szNum, "CitizenButton" + szNum, 0, 0, iChevronSize)
# BUG - city specialist - start
		gSetRect("SpecialistLabelBackground", "CityRightPanelContents", 0, lSpecialistDisabledButtons.y() - gRect("CityRightPanelContents").yBottom() - iBigVSpace, RectLayout.MAX, self.cityScreenHeadingBackgrHeight())
		gOffSetPoint("SpecialistLabel", "SpecialistLabelBackground", RectLayout.CENTER, self.cityScreenHeadingOffset())
# BUG - city specialist - end

	def setCityBonusRects(self):
		iMaxRMargin = gRect("Top").xRight() - gRect("CityRightPanelContents").x()
		iBonusBackrOverhang = min(VSPACE(16), gRect("SpecialistLabelBackground").height())

		# <!-- custom: make it even between the 3 bonus columns, since we have more room and cleaner as such and in case the strategy bonuses use some effects or such, they'd need some room to show them.
		# Done with the help of gemini 3 pro, check if accurate.
		# Also use a variable so it dynamically adjusts to total side width in case we want to change it later.
		# Also account for the margins, the total width seems to be effectively quite a bit less than max, empirically reduce it a bit to adjust for that.
		# Note: i tried 15 instead of 2 * 8 but it seems to give same result than 2 * 7 (i.e. 14 and 15 are not different somehow if i'm not mistaken, only 14 or 16 show a difference visually ingame) so went with 2 * 8 rather than 2 * 7 as it seems prettier to me as such even though a bit too wide but the other is too tight -->
		iBonusTableTableWidth = self.SIDE_PANELS_WIDTH - (2 * 8)
		iBonusTableColumnWidth = iBonusTableTableWidth / 3
		# advc.004: Was 57; don't need quite this much space.
		# HLEN(53),
		gSetRect("BonusPane0", "CityRightPanelContents", 0, 0, HLEN(iBonusTableColumnWidth), -(gRect("CityRightPanelContents").yBottom() - (gRect("SpecialistLabelBackground").yBottom() - iBonusBackrOverhang)))
		# Width was 157 in BtS. Perhaps the idea was to get all the scrollbars to appear on top of each other at the right edge. If so, then that's a pretty poor kludge. I'm going to push all scrollbars off-screen.
		# That will still allow scrolling via mouse wheel - though the player may not even realize that not all resources are shown.
		# Scrolling is only relevant when the height is insufficient for the list of bonus resources. That should never be the case with the BtS/AdvCiv rules, even at the lowest resolution.
		gSetRect("BonusBack0", "CityRightPanelContents", 0, 0, iMaxRMargin, gRect("BonusPane0").height() + iBonusBackrOverhang)
		# advc.002b: Was 68 in BtS; need less space in the third col now because the plus signs are gone.
		# HLEN(78),
		gSetRect("BonusPane1", "CityRightPanelContents", gRect("BonusPane0").width(), 0, HLEN(iBonusTableColumnWidth), gRect("BonusPane0").height())
		# width was 184
		gSetRect("BonusBack1", "CityRightPanelContents", gRect("BonusPane1").x() - gRect("CityRightPanelContents").x(), 0, iMaxRMargin, gRect("BonusBack0").height())
		# RectLayout.MAX,
		gSetRect("BonusPane2", "CityRightPanelContents", gRect("BonusPane0").width() + gRect("BonusPane1").width(), 0, HLEN(iBonusTableColumnWidth), gRect("BonusPane0").height())
		# width was 205
		gSetRect("BonusBack2", "CityRightPanelContents", gRect("BonusPane2").x() - gRect("CityRightPanelContents").x(), 0, iMaxRMargin, gRect("BonusBack0").height())

		self.bCityBonusButtons = True
		# <!-- custom: City-screen bonus columns now usually have ample vertical room and side-panel geometry is controlled elsewhere, so prefer a fixed size over the old resolution-ratio formula.
		# Old formula: iround(32 * (0.8 + gRect("Top").height() / float(gRect("Top").width())) / 1.43). (GPT-5.5) -->
		self.useCityBonusButtonSize(self.iCityBonusMinBtnSize)

	def cityBonusRowsPerColumn(self, iBtnSize):
		return ColumnLayout(gRect("BonusBack0"), 0, VSPACE(2), RectLayout.MAX, 1, RectLayout.MAX, iBtnSize).numWidgets()

	def useCityBonusButtonSize(self, iBtnSize):
		for i in range(3):
			lRows = ColumnLayout(gRect("BonusBack" + str(i)), 0, VSPACE(2), RectLayout.MAX, 1, RectLayout.MAX, iBtnSize)
			gSetRectangle("CityBonusColumn" + str(i), lRows)
			for j in range(lRows.numWidgets()):
				lRow = lRows.next()
				szIndex = str(i) + "_" + str(j)
				gSetRectangle("CityBonusCell" + szIndex, lRow)
				lBtn = SquareLayout(lRow, 0, 0, iBtnSize)
				gSetRectangle("CityBonusBtn" + szIndex, lBtn)
				gSetRectangle("CityBonusCircle" + szIndex, SquareLayout(lBtn, RectLayout.RIGHT, RectLayout.BOTTOM, 16))
				gOffSetPoint("CityBonusAmount" + szIndex, "CityBonusCircle" + szIndex, RectLayout.CENTER, -2)
				if i != 0:
					gOffSetPoint("CityBonusEffect" + szIndex, lRow, gRect("BonusPane" + str(i)).width() - HSPACE(4, 4), iBtnSize / 4)

	def interfaceScreen (self):
		# Draw all of the screen elements.
		# This function is called once after starting or loading a game.
		# THIS FUNCTION MUST NOT CREATE ANY INSTANCE OR GLOBAL VARIABLES.
		# It may alter existing ones created in __init__() or initState(), however.
		#
		if (CyGame().isPitbossHost()):
			return

		# This is the main interface screen, create it as such
		screen = CyGInterfaceScreen("MainInterface", CvScreenEnums.MAIN_INTERFACE)
		self.screen = screen # advc.092
		self.initState(screen)
		screen.setForcedRedraw(True)
		screen.setDimensions(0, 0, self.xResolution, self.yResolution)

		xResolution = self.xResolution
		yResolution = self.yResolution

		# <!-- custom: compute cheaply once so it is cheaper.
		# Note: done here rather than in init since it somehow doesn't work unlike in some other files -->
		if self.iBarExtraRows is None:
			self.iBarExtraRows = gc.getDefineINT("SAS_CV_MAIN_INTERFACE_CITY_SCREEN_BAR_IEXTRAROWS")
			self.iBarExtraRowsExtraManualAdjust = gc.getDefineINT("SAS_CV_MAIN_INTERFACE_CITY_SCREEN_BAR_IEXTRAROWS_EXTRA_MANUAL_ADJUST")
		if self.iBarExtraHeightExtraManualAdjust is None:
			self.iBarExtraHeightExtraManualAdjust = VSPACE(gc.getDefineINT("SAS_CV_MAIN_INTERFACE_CITY_SCREEN_BAR_HEIGHT_EXTRA_MANUAL_ADJUST"))
		if self.iCityScreenBlueRibbonExtraLift is None:
			self.iCityScreenBlueRibbonExtraLift = VSPACE(gc.getDefineINT("SAS_CV_MAIN_INTERFACE_CITY_SCREEN_BLUE_RIBBON_EXTRA_LIFT"))
		if self.IS_SAS_CV_MAIN_INTERFACE_PRODUCTION_QUEUE_BUTTONS is None:
			self.IS_SAS_CV_MAIN_INTERFACE_PRODUCTION_QUEUE_BUTTONS = (gc.getDefineINT("SAS_CV_MAIN_INTERFACE_PRODUCTION_QUEUE_BUTTONS") > 0)
		if self.IS_SAS_CV_MAIN_INTERFACE_HIDE_PLOT_LIST_PANEL_IN_CITY_SCREEN is None:
			self.IS_SAS_CV_MAIN_INTERFACE_HIDE_PLOT_LIST_PANEL_IN_CITY_SCREEN = (gc.getDefineINT("SAS_CV_MAIN_INTERFACE_HIDE_PLOT_LIST_PANEL_IN_CITY_SCREEN") > 0)
		if self.SAS_CV_MAIN_INTERFACE_CITY_BONUS_ICON_MODE is None:
			self.SAS_CV_MAIN_INTERFACE_CITY_BONUS_ICON_MODE = gc.getDefineINT("SAS_CV_MAIN_INTERFACE_CITY_BONUS_ICON_MODE")
			if self.SAS_CV_MAIN_INTERFACE_CITY_BONUS_ICON_MODE < 0 or self.SAS_CV_MAIN_INTERFACE_CITY_BONUS_ICON_MODE > 2:
				raise RuntimeError("SAS_CV_MAIN_INTERFACE_CITY_BONUS_ICON_MODE must be 0, 1, or 2")
		if self.IS_SAS_CV_MAIN_INTERFACE_UNIT_INFO_BUTTON is None:
			self.IS_SAS_CV_MAIN_INTERFACE_UNIT_INFO_BUTTON = (gc.getDefineINT("SAS_CV_MAIN_INTERFACE_UNIT_INFO_BUTTON") > 0)
		if self.IS_SAS_CV_MAIN_INTERFACE_TOP_LEFT_FLAG_AND_LEFT_SIDE_TEXT_SHIFT is None:
			self.IS_SAS_CV_MAIN_INTERFACE_TOP_LEFT_FLAG_AND_LEFT_SIDE_TEXT_SHIFT = (gc.getDefineINT("SAS_CV_MAIN_INTERFACE_TOP_LEFT_FLAG_AND_LEFT_SIDE_TEXT_SHIFT") > 0)
		lTop = gRect("Top")

		self.setDefaultHelpTextArea()

		self.addPanel("CityLeftPanel")
		screen.setStyle("CityLeftPanel", "Panel_City_Left_Style")
		screen.hide("CityLeftPanel")

		self.addPanel("CityRightPanel")
		screen.setStyle("CityRightPanel", "Panel_City_Right_Style")
		screen.hide("CityRightPanel")

		self.addPanel("TopCityPanelCenter")
		screen.setStyle("TopCityPanelCenter", "Panel_City_Top_Style")
		screen.hide("TopCityPanelCenter")
		# <advc.092>
		self.addPanel("TopCityPanelCenterStagger")
		screen.setStyle("TopCityPanelCenterStagger", "Panel_City_Top_Style")
		screen.hide("TopCityPanelCenterStagger")
		self.addPanel("CityLeftPanelStagger")
		screen.setStyle("CityLeftPanelStagger", "Panel_City_Left_Style")
		screen.hide("CityLeftPanelStagger")
		self.addPanel("CityRightPanelStagger")
		screen.setStyle("CityRightPanelStagger", "Panel_City_Right_Style")
		screen.hide("CityRightPanelStagger")
		# </advc.092>

		self.addPanel("CityAdjustPanel")
		screen.setStyle("CityAdjustPanel", "Panel_City_Info_Style")
		screen.hide("CityAdjustPanel")

		self.addPanel("TopCityPanelLeft")
		screen.setStyle("TopCityPanelLeft", "Panel_City_TanTL_Style")
		screen.hide("TopCityPanelLeft")
		self.addPanel("TopCityPanelRight")
		screen.setStyle("TopCityPanelRight", "Panel_City_TanTR_Style")
		screen.hide("TopCityPanelRight")

		# Top Bar
		# SF CHANGE  (advc, note: This panel and the other marked by Firaxis with "SF CHANGE" deal with decorations around the research bar. This one here draws a line between the non-city info on top and the proper city screen.
		# The other one draws brackets around the research bar.)
		self.addPanel("CityScreenTopWidget")
		screen.setStyle("CityScreenTopWidget", "Panel_TopBar_Style")
		screen.hide("CityScreenTopWidget")

		self.addPanel("CityNameBackground")
		screen.setStyle("CityNameBackground", "Panel_City_Title_Style")
		screen.hide("CityNameBackground")

		self.addPanel("CenterBottomPanel")
		screen.setStyle("CenterBottomPanel", "Panel_Game_HudBC_Style")
		screen.hide("CenterBottomPanel")

		self.addPanel("LowerLeftCornerPanel")
		screen.setStyle("LowerLeftCornerPanel", "Panel_Game_HudBL_Style")
		screen.hide("LowerLeftCornerPanel")
		# advc: The BtS name for this graphic had clashed with the panel above and, therefore, had had no effect. Fixed, and I've also added show and hide calls throughout the module. It thickens the edges of the corner panel. I don't ultimately like how that looks, so I've commented out the show calls.
		# (There's also an unused texture INTERFACE_BOTTOM_RIGHT, but that one doesn't align with the Globe button, perhaps because the layout changed a bit at some point.)
		self.addDDS("LowerLeftCornerBackgr", "INTERFACE_BOTTOM_LEFT")
		screen.hide("LowerLeftCornerBackgr")

		self.addPanel("LowerRightCornerPanel")
		screen.setStyle("LowerRightCornerPanel", "Panel_Game_HudBR_Style")
		screen.hide("LowerRightCornerPanel")

		self.addPanel("EndTurnButtonPanel")
		screen.setStyle("EndTurnButtonPanel", "Panel_Game_HudBR_Style")
		screen.hide("EndTurnButtonPanel")

		# <!-- custom: left edge panel to complete the border frame (Claude code Opus 4.5) -->
		self.addPanel("EndTurnButtonPanelLeft")
		screen.setStyle("EndTurnButtonPanelLeft", "Panel_Game_HudBL_Style")
		screen.hide("EndTurnButtonPanelLeft")

		self.addPanel("InterfaceTopCenter")
		screen.setStyle("InterfaceTopCenter", "Panel_Game_HudTC_Style")
		screen.hide("InterfaceTopCenter")

		self.addPanel("InterfaceTopLeft")
		screen.setStyle("InterfaceTopLeft", "Panel_Game_HudTL_Style")
		screen.hide("InterfaceTopLeft")

		self.addPanel("InterfaceTopRight")
		screen.setStyle("InterfaceTopRight", "Panel_Game_HudTR_Style")
		screen.hide("InterfaceTopRight")

		# <!-- custom: give the turn log the diplomatic-advisor icon so the log notebook/pencil icon can represent map annotations in the minimap row. (GPT-5.5) -->
		self.setStyledButton("TurnLogButton", "Button_HUDAdvisorForeign_Style", WidgetTypes.WIDGET_ACTION, gc.getControlInfo(ControlTypes.CONTROL_TURN_LOG).getActionInfoIndex())
		screen.hide("TurnLogButton")
		# Advisor Buttons...
		self.setStyledButton("DomesticAdvisorButton", "Button_HUDAdvisorDomestic_Style", WidgetTypes.WIDGET_ACTION, gc.getControlInfo(ControlTypes.CONTROL_DOMESTIC_SCREEN).getActionInfoIndex())
		screen.hide("DomesticAdvisorButton")
		self.setStyledButton("PolicyAdvisorButton", "Button_HUDAdvisorCivics_Style", WidgetTypes.WIDGET_ACTION, gc.getControlInfo(ControlTypes.CONTROL_CIVICS_SCREEN).getActionInfoIndex())
		screen.hide("PolicyAdvisorButton")
		# <!-- custom: use the corporation-advisor icon for foreign diplomacy after moving the diplomatic-advisor icon to the turn log; thematically close enough for relationship/network management. (GPT-5.5) -->
		self.setStyledButton("ForeignDiplomacyAdvisorButton", "Button_HUDAdvisorCorporation_Style", WidgetTypes.WIDGET_ACTION, gc.getControlInfo(ControlTypes.CONTROL_FOREIGN_DIPLOMACY_SCREEN).getActionInfoIndex())
		screen.hide("ForeignDiplomacyAdvisorButton")
		# <!-- custom: F4 (Foreign Trade slot) uses the HUD Globe Trade style: more directly evocative of "foreign trade" than the generic Finance icon.
		# The Finance icon would otherwise read as plain treasury rather than international trade. (Claude code Opus 4.7) -->
		self.setStyledButton("ForeignAdvisorButton", "Button_HUDGlobeTrade_Style", WidgetTypes.WIDGET_ACTION, gc.getControlInfo(ControlTypes.CONTROL_FOREIGN_SCREEN).getActionInfoIndex())
		screen.hide("ForeignAdvisorButton")
		self.setStyledButton("MilitaryAdvisorButton", "Button_HUDAdvisorMilitary_Style", WidgetTypes.WIDGET_ACTION, gc.getControlInfo(ControlTypes.CONTROL_MILITARY_SCREEN).getActionInfoIndex())
		screen.hide("MilitaryAdvisorButton")
		self.setStyledButton("TechAdvisorButton", "Button_HUDAdvisorTechnology_Style", WidgetTypes.WIDGET_ACTION, gc.getControlInfo(ControlTypes.CONTROL_TECH_CHOOSER).getActionInfoIndex())
		screen.hide("TechAdvisorButton")
		self.setStyledButton("WorldAdvisorButton", "Button_HUDGlobeStrategy_Style", WidgetTypes.WIDGET_ACTION, gc.getControlInfo(ControlTypes.CONTROL_WORLD_ADVISOR_SCREEN).getActionInfoIndex())
		screen.hide("WorldAdvisorButton")
		self.setStyledButton("VictoryAdvisorButton", "Button_HUDAdvisorVictory_Style", WidgetTypes.WIDGET_ACTION, gc.getControlInfo(ControlTypes.CONTROL_VICTORY_SCREEN).getActionInfoIndex())
		screen.hide("VictoryAdvisorButton")
		self.setStyledButton("InfoAdvisorButton", "Button_HUDAdvisorRecord_Style", WidgetTypes.WIDGET_ACTION, gc.getControlInfo(ControlTypes.CONTROL_INFO).getActionInfoIndex())
		screen.hide("InfoAdvisorButton")
# BUG - 3.17 No Espionage - start
		if GameUtil.isEspionage():
			self.setStyledButton("EspionageAdvisorButton", "Button_HUDAdvisorEspionage_Style", WidgetTypes.WIDGET_ACTION, gc.getControlInfo(ControlTypes.CONTROL_ESPIONAGE_SCREEN).getActionInfoIndex())
			screen.hide("EspionageAdvisorButton")
# BUG - 3.17 No Espionage - end
# BUG - field of view slider - start
		self.setFieldofView_Text(screen)
		self.addSlider("FoVSlider", self.FoVToSliderPos(self.iField_View))
		screen.hide("FoVSliderText")
		screen.hide("FoVSlider")
# BUG - field of view slider - end
		# City Tabs ...
		self.setStyledButton("CityTab0", "Button_HUDJumpUnit_Style", WidgetTypes.WIDGET_CITY_TAB, 0)
		screen.hide("CityTab0")
		self.setStyledButton("CityTab1", "Button_HUDJumpBuilding_Style", WidgetTypes.WIDGET_CITY_TAB, 1)
		screen.hide("CityTab1")
		self.setStyledButton("CityTab2", "Button_HUDJumpWonder_Style", WidgetTypes.WIDGET_CITY_TAB, 2)
		screen.hide("CityTab2")

		screen.setMainInterface(True)
		# Minimap initialization ...
		self.addPanel("MiniMapPanel")
		screen.setStyle("MiniMapPanel", "Panel_Game_HudMap_Style")
		screen.hide("MiniMapPanel")
		MinimapOptions.init() # advc.002a: Let DLL cache minimap options
		self.initMinimap() # advc: Moved into new function
		gc.getMap().updateMinimapColor()
		self.createMinimapButtons()

		# Help button (always visible), main menu button
		self.setImageButton("InterfaceHelpButton", "INTERFACE_GENERAL_CIVILOPEDIA_ICON", WidgetTypes.WIDGET_ACTION, gc.getControlInfo(ControlTypes.CONTROL_CIVILOPEDIA).getActionInfoIndex())
		screen.hide("InterfaceHelpButton")
		self.setImageButton("MainMenuButton", "INTERFACE_GENERAL_MENU_ICON", WidgetTypes.WIDGET_MENU_ICON)
		screen.hide("MainMenuButton")

		self.createGlobeviewButtons()
		self.updateBottomButtonList()
		screen.hide("BottomButtonList")
		self.createPlotListFrames() # advc: Moved into subroutine
# BUG - unit plot draw method - start (advc.092: Moved from initState method)
		from BugUnitPlot import BupPanel
		# advc.092: BupPanel will access some global rectangles instead
		#gRect("Top").width(), gRect("Top").height(),
		#gRect("BottomButtonMaxSpace").width(),
		self.BupPanel = BupPanel(screen, self.numPlotListButtonsPerRow(), self.numPlotListRows())
# BUG - unit plot draw method - end

		self.setLabel("EndTurnText", "Background", u"", CvUtil.FONT_CENTER_JUSTIFY, FontTypes.GAME_FONT, -0.1)
		screen.setHitTest("EndTurnText", HitTestTypes.HITTEST_NOHIT)
		# Three states for end turn button...
		self.setStyledButton("EndTurnButton", "Button_HUDEndTurn_Style", WidgetTypes.WIDGET_END_TURN)
		screen.setEndTurnState("EndTurnButton", "Red")
		screen.hide("EndTurnButton")

		# Tech buttons (to be positioned later)
		self.iTechBtnSize = BTNSZ(32)
		for i in range(gc.getNumTechInfos()):
			szName = "ResearchButton" + str(i)
			screen.setImageButton(szName, gc.getTechInfo(i).getButton(), 0, 0, self.iTechBtnSize, self.iTechBtnSize, WidgetTypes.WIDGET_RESEARCH, i, -1)
			screen.hide(szName)
		iReligionBtnSize = BTNSZ(32)
		for i in range(gc.getNumReligionInfos()):
			szName = "ReligionButton" + str(i)
			if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_PICK_RELIGION):
				szButton = gc.getReligionInfo(i).getGenericTechButton()
			else:
				szButton = gc.getReligionInfo(i).getTechButton()
			screen.setImageButton(szName, szButton, 0, 0, iReligionBtnSize, iReligionBtnSize, WidgetTypes.WIDGET_RESEARCH, gc.getReligionInfo(i).getTechPrereq(), -1)
			screen.hide(szName)

		# Citizen buttons ...
		#szHideCitizenList = [] # advc: unused
		for i in range(MAX_CITIZEN_BUTTONS):
			szName = "AngryCitizen" + str(i)
			self.setImageButton(szName, "INTERFACE_ANGRYCITIZEN_TEXTURE", WidgetTypes.WIDGET_ANGRY_CITIZEN)
			screen.hide(szName)
		for iSpecialist in self.visibleSpecialists:
			szName = "IncreaseSpecialist" + str(iSpecialist)
			self.setStyledButton(szName, ButtonStyles.BUTTON_STYLE_CITY_PLUS, WidgetTypes.WIDGET_CHANGE_SPECIALIST, iSpecialist, 1)
			screen.hide(szName)
		for iSpecialist in self.visibleSpecialists:
			szName = "DecreaseSpecialist" + str(iSpecialist)
			self.setStyledButton(szName, ButtonStyles.BUTTON_STYLE_CITY_MINUS, WidgetTypes.WIDGET_CHANGE_SPECIALIST, iSpecialist, -1)
			screen.hide(szName)
		for iSpecialist in self.visibleSpecialists:
			szName = "SpecialistDisabledButton" + str(iSpecialist)
			self.setImageButton(szName, gc.getSpecialistInfo(iSpecialist).getTexture(), WidgetTypes.WIDGET_DISABLED_CITIZEN, iSpecialist)
			screen.enable(szName, False)
			screen.hide(szName)
			for j in range(MAX_CITIZEN_BUTTONS):
				szName = "CitizenButton" + str(iSpecialist * 100 + j)
				self.addCheckBox(szName, gc.getSpecialistInfo(iSpecialist).getTexture(), "", ButtonStyles.BUTTON_STYLE_LABEL, WidgetTypes.WIDGET_CITIZEN, iSpecialist, j)
				screen.hide(szName)
# BUG - city specialist - start
		self.addPanel("SpecialistLabelBackground")
		screen.setStyle("SpecialistLabelBackground", "Panel_City_Header_Style")
		screen.hide("SpecialistLabelBackground")
		self.setLabel("SpecialistLabel", "Background", localText.getText("TXT_KEY_CONCEPT_SPECIALISTS", ()), CvUtil.FONT_CENTER_JUSTIFY, FontTypes.SMALL_FONT)
		screen.hide("SpecialistLabel")
# BUG - city specialist - end

		# **********************************************************
		# GAME DATA STRINGS
		# **********************************************************
		szGameDataList = []

		self.addStackedBar("TwoLineResearchBar", WidgetTypes.WIDGET_RESEARCH)
		screen.setStackedBarColors("TwoLineResearchBar", InfoBarTypes.INFOBAR_STORED, self.colorResearchStored)
		screen.setStackedBarColors("TwoLineResearchBar", InfoBarTypes.INFOBAR_RATE, self.colorResearchRate)
		screen.setStackedBarColors("TwoLineResearchBar", InfoBarTypes.INFOBAR_RATE_EXTRA, self.colorEmpty)
		screen.setStackedBarColors("TwoLineResearchBar", InfoBarTypes.INFOBAR_EMPTY, self.colorEmpty)
		screen.hide("TwoLineResearchBar")
# BUG - Great General Bar - start
		self.addStackedBar("TwoLineGGBar", WidgetTypes.WIDGET_HELP_GREAT_GENERAL)
		screen.setStackedBarColors("TwoLineGGBar", InfoBarTypes.INFOBAR_STORED, self.colorNegativeRate)
		screen.setStackedBarColors("TwoLineGGBar", InfoBarTypes.INFOBAR_RATE, self.colorEmpty)
		screen.setStackedBarColors("TwoLineGGBar", InfoBarTypes.INFOBAR_RATE_EXTRA, self.colorEmpty)
		screen.setStackedBarColors("TwoLineGGBar", InfoBarTypes.INFOBAR_EMPTY, self.colorEmpty)
		screen.hide("TwoLineGGBar")
# BUG - Great General Bar - end
# BUG - Great Person Bar - start
		self.addStackedBar("TwoLineGPBar", WidgetTypes.WIDGET_GP_PROGRESS_BAR)
		screen.setStackedBarColors("TwoLineGPBar", InfoBarTypes.INFOBAR_STORED, self.colorGreatPeopleStored)
		screen.setStackedBarColors("TwoLineGPBar", InfoBarTypes.INFOBAR_RATE, self.colorGreatPeopleRate)
		screen.setStackedBarColors("TwoLineGPBar", InfoBarTypes.INFOBAR_RATE_EXTRA, self.colorEmpty)
		screen.setStackedBarColors("TwoLineGPBar", InfoBarTypes.INFOBAR_EMPTY, self.colorEmpty)
		screen.hide("TwoLineGPBar")
# BUG - Great Person Bar - end
# BUG - Bars on single line for higher resolution screens - start
		self.addStackedBar("OneLineGGBar", WidgetTypes.WIDGET_HELP_GREAT_GENERAL)
		#self.colorGreatPeopleStored)
		screen.setStackedBarColors("OneLineGGBar", InfoBarTypes.INFOBAR_STORED, self.colorNegativeRate)
		screen.setStackedBarColors("OneLineGGBar", InfoBarTypes.INFOBAR_RATE, self.colorEmpty)
		screen.setStackedBarColors("OneLineGGBar", InfoBarTypes.INFOBAR_RATE_EXTRA, self.colorEmpty)
		screen.setStackedBarColors("OneLineGGBar", InfoBarTypes.INFOBAR_EMPTY, self.colorEmpty)
		screen.hide("OneLineGGBar")

		self.addStackedBar("OneLineResearchBar", WidgetTypes.WIDGET_RESEARCH)
		screen.setStackedBarColors("OneLineResearchBar", InfoBarTypes.INFOBAR_STORED, self.colorResearchStored)
		screen.setStackedBarColors("OneLineResearchBar", InfoBarTypes.INFOBAR_RATE, self.colorResearchRate)
		screen.setStackedBarColors("OneLineResearchBar", InfoBarTypes.INFOBAR_RATE_EXTRA, self.colorEmpty)
		screen.setStackedBarColors("OneLineResearchBar", InfoBarTypes.INFOBAR_EMPTY, self.colorEmpty)
		screen.hide("OneLineResearchBar")

		self.addStackedBar("OneLineGPBar", WidgetTypes.WIDGET_GP_PROGRESS_BAR)
		screen.setStackedBarColors("OneLineGPBar", InfoBarTypes.INFOBAR_STORED, self.colorGreatPeopleStored)
		screen.setStackedBarColors("OneLineGPBar", InfoBarTypes.INFOBAR_RATE, self.colorGreatPeopleRate)
		screen.setStackedBarColors("OneLineGPBar", InfoBarTypes.INFOBAR_RATE_EXTRA, self.colorEmpty)
		screen.setStackedBarColors("OneLineGPBar", InfoBarTypes.INFOBAR_EMPTY, self.colorEmpty)
		screen.hide("OneLineGPBar")
# BUG - Bars on single line for higher resolution screens - end

		# *********************************************************************************
		# SELECTION DATA BUTTONS/STRINGS
		# *********************************************************************************

		szHideSelectionDataList = []

		self.addStackedBar("PopulationBar", WidgetTypes.WIDGET_HELP_POPULATION)
		screen.setStackedBarColors("PopulationBar", InfoBarTypes.INFOBAR_STORED, gc.getYieldInfo(YieldTypes.YIELD_FOOD).getColorType())
		screen.setStackedBarColorsAlpha("PopulationBar", InfoBarTypes.INFOBAR_RATE, gc.getYieldInfo(YieldTypes.YIELD_FOOD).getColorType(), 0.8)
		screen.setStackedBarColors("PopulationBar", InfoBarTypes.INFOBAR_RATE_EXTRA, self.colorNegativeRate)
		screen.setStackedBarColors("PopulationBar", InfoBarTypes.INFOBAR_EMPTY, self.colorEmpty)
		screen.hide("PopulationBar")

		self.addStackedBar("ProductionBar", WidgetTypes.WIDGET_HELP_PRODUCTION)
		screen.setStackedBarColors("ProductionBar", InfoBarTypes.INFOBAR_STORED, gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getColorType())
		screen.setStackedBarColorsAlpha("ProductionBar", InfoBarTypes.INFOBAR_RATE, gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getColorType(), 0.8)
		screen.setStackedBarColors("ProductionBar", InfoBarTypes.INFOBAR_RATE_EXTRA, gc.getYieldInfo(YieldTypes.YIELD_FOOD).getColorType())
		screen.setStackedBarColors("ProductionBar", InfoBarTypes.INFOBAR_EMPTY, self.colorEmpty)
		screen.hide("ProductionBar")

		self.addStackedBar("GreatPeopleBar", WidgetTypes.WIDGET_HELP_GREAT_PEOPLE)
		screen.setStackedBarColors("GreatPeopleBar", InfoBarTypes.INFOBAR_STORED, self.colorGreatPeopleStored)
		screen.setStackedBarColors("GreatPeopleBar", InfoBarTypes.INFOBAR_RATE, self.colorGreatPeopleRate)
		screen.setStackedBarColors("GreatPeopleBar", InfoBarTypes.INFOBAR_RATE_EXTRA, self.colorEmpty)
		screen.setStackedBarColors("GreatPeopleBar", InfoBarTypes.INFOBAR_EMPTY, self.colorEmpty)
		screen.hide("GreatPeopleBar")

		self.addStackedBar("CultureBar", WidgetTypes.WIDGET_HELP_CULTURE)
		screen.setStackedBarColors("CultureBar", InfoBarTypes.INFOBAR_STORED, self.colorGreatCultureStored)
		screen.setStackedBarColors("CultureBar", InfoBarTypes.INFOBAR_RATE, self.colorGreatCultureRate)
		screen.setStackedBarColors("CultureBar", InfoBarTypes.INFOBAR_RATE_EXTRA, self.colorEmpty)
		screen.setStackedBarColors("CultureBar", InfoBarTypes.INFOBAR_EMPTY, self.colorEmpty)
		screen.hide("CultureBar")

		# (BUG - Limit/Extra Religions: Removed BtS code for adding ReligionHolyCityDDS, CorporationHeadquarterDDS to screen)

		self.addStackedBar("NationalityBar", WidgetTypes.WIDGET_HELP_NATIONALITY)
		screen.hide("NationalityBar")

		# <!-- custom: score scroll buttons; shown only when player count exceeds available scoreboard rows. (Claude code Sonnet 4.6) -->
		self.setStyledButton("ScoreScrollUp", ButtonStyles.BUTTON_STYLE_CITY_PLUS, WIDGET_SCORE_SCROLL_UP, -1, -1)
		screen.hide("ScoreScrollUp")
		self.setStyledButton("ScoreScrollDown", ButtonStyles.BUTTON_STYLE_CITY_MINUS, WIDGET_SCORE_SCROLL_DOWN, -1, -1)
		screen.hide("ScoreScrollDown")
		self.setStyledButton("ScoreScrollUpFast", ButtonStyles.BUTTON_STYLE_CITY_PLUS, WIDGET_SCORE_SCROLL_UP_FAST, -1, -1)
		screen.hide("ScoreScrollUpFast")
		self.setStyledButton("ScoreScrollDownFast", ButtonStyles.BUTTON_STYLE_CITY_MINUS, WIDGET_SCORE_SCROLL_DOWN_FAST, -1, -1)
		screen.hide("ScoreScrollDownFast")
		# <!-- custom: scoreboard bg style cycle button; plain styled button, hover explains it. (Claude code Opus 4.7) -->
		self.setStyledButton("ScoreBgStyle", ButtonStyles.BUTTON_STYLE_STANDARD, WIDGET_SCORE_BG_STYLE, -1, -1)
		screen.hide("ScoreBgStyle")
		# <!-- custom: always-expand toggle; resolve SAS emoji path explicitly (addCheckBox only auto-resolves INTERFACE_/BUTTON_/RAW_YIELDS_/PLE_ prefixes). (Claude code Sonnet 4.6) -->
		self.addCheckBox("ScoreExpandToggle", self.szScoreExpandTogglePath, self.szScoreExpandTogglePath, ButtonStyles.BUTTON_STYLE_IMAGE, WIDGET_SCORE_EXPAND_TOGGLE)
		screen.setState("ScoreExpandToggle", False)
		screen.hide("ScoreExpandToggle")
		self.setImageButton("ScoreScrollCenterActivePlayer", self.szScoreScrollCenterActivePlayerPath, WIDGET_SCORE_SCROLL_CENTER_ACTIVE_PLAYER, -1, -1)
		screen.hide("ScoreScrollCenterActivePlayer")
		self.addCheckBox("ScoreScrollAutocenterActivePlayerToggle", self.szScoreScrollAutocenterActivePlayerPath, self.szScoreScrollAutocenterActivePlayerPath, ButtonStyles.BUTTON_STYLE_IMAGE, WIDGET_SCORE_SCROLL_AUTOCENTER_ACTIVE_PLAYER_TOGGLE)
		screen.setState("ScoreScrollAutocenterActivePlayerToggle", False)
		screen.hide("ScoreScrollAutocenterActivePlayerToggle")
		self.setStyledButton("CityScrollMinus", ButtonStyles.BUTTON_STYLE_ARROW_LEFT, WidgetTypes.WIDGET_CITY_SCROLL, -1)
		screen.hide("CityScrollMinus")
		self.setStyledButton("CityScrollPlus", ButtonStyles.BUTTON_STYLE_ARROW_RIGHT, WidgetTypes.WIDGET_CITY_SCROLL, 1)
		screen.hide("CityScrollPlus")

# BUG - City Arrows - start
		self.setStyledButton("MainCityScrollMinus", ButtonStyles.BUTTON_STYLE_ARROW_LEFT, WidgetTypes.WIDGET_CITY_SCROLL, -1)
		screen.hide("MainCityScrollMinus")
		self.setStyledButton("MainCityScrollPlus", ButtonStyles.BUTTON_STYLE_ARROW_RIGHT, WidgetTypes.WIDGET_CITY_SCROLL, 1)
		screen.hide("MainCityScrollPlus")
# BUG - City Arrows - end

		self.addPanel("TradeRouteListBackground")
		screen.setStyle("TradeRouteListBackground", "Panel_City_Header_Style")
		screen.hide("TradeRouteListBackground")

		self.setLabel("TradeRouteListLabel", "Background", localText.getText("TXT_KEY_HEADING_TRADEROUTE_LIST", ()), CvUtil.FONT_CENTER_JUSTIFY, FontTypes.SMALL_FONT, -0.1)
		screen.hide("TradeRouteListLabel")

# BUG - Raw Yields - start
		# Trade
		self.addCheckBox("RawYieldsTrade0", "RAW_YIELDS_TRADE", "RAW_YIELDS_HIGHLIGHT", ButtonStyles.BUTTON_STYLE_LABEL)
		screen.hide("RawYieldsTrade0")

		# Yields
		self.addCheckBox("RawYieldsFood1", "RAW_YIELDS_FOOD", "RAW_YIELDS_HIGHLIGHT", ButtonStyles.BUTTON_STYLE_LABEL)
		screen.hide("RawYieldsFood1")
		self.addCheckBox("RawYieldsProduction2", "RAW_YIELDS_PRODUCTION", "RAW_YIELDS_HIGHLIGHT", ButtonStyles.BUTTON_STYLE_LABEL)
		screen.hide("RawYieldsProduction2")
		self.addCheckBox("RawYieldsCommerce3", "RAW_YIELDS_COMMERCE", "RAW_YIELDS_HIGHLIGHT", ButtonStyles.BUTTON_STYLE_LABEL)
		screen.hide("RawYieldsCommerce3")

		# Tile Selection
		self.addCheckBox("RawYieldsWorkedTiles4", "RAW_YIELDS_WORKED_TILES", "RAW_YIELDS_HIGHLIGHT", ButtonStyles.BUTTON_STYLE_LABEL)
		screen.hide("RawYieldsWorkedTiles4")
		self.addCheckBox("RawYieldsCityTiles5", "RAW_YIELDS_CITY_TILES", "RAW_YIELDS_HIGHLIGHT", ButtonStyles.BUTTON_STYLE_LABEL)
		screen.hide("RawYieldsCityTiles5")
		self.addCheckBox("RawYieldsOwnedTiles6", "RAW_YIELDS_OWNED_TILES", "RAW_YIELDS_HIGHLIGHT", ButtonStyles.BUTTON_STYLE_LABEL)
		screen.hide("RawYieldsOwnedTiles6")
# BUG - Raw Yields - end
# BUG - BUG Option Button - Start
		#self.setImageButton("BUGOptionsScreenWidget", ArtFileMgr.getInterfaceArtInfo("BUG_OPTIONS_SCREEN_BUTTON").getPath(), WidgetTypes.WIDGET_BUG_OPTION_SCREEN)
		# <K-Mod>
		# </K-Mod>
		self.setStyledButton("BUGOptionsScreenWidget", "Button_HUDAdvisorCorporation_Style", WidgetTypes.WIDGET_BUG_OPTION_SCREEN)
		screen.hide("BUGOptionsScreenWidget")
# BUG - BUG Option Button - End

		self.addPanel("BuildingListBackground")
		screen.setStyle("BuildingListBackground", "Panel_City_Header_Style")
		screen.hide("BuildingListBackground")

		self.setLabel("BuildingListLabel", "Background", localText.getText("TXT_KEY_CONCEPT_BUILDINGS", ()), CvUtil.FONT_CENTER_JUSTIFY, FontTypes.SMALL_FONT, -0.1)
		screen.hide("BuildingListLabel")

		# *********************************************************************************
		# UNIT INFO ELEMENTS
		# *********************************************************************************

		for iPromo in range(gc.getNumPromotionInfos()):
			szName = "PromotionButton" + str(iPromo)
			self.addDDS(szName, gc.getPromotionInfo(iPromo), WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, iPromo)
			screen.hide(szName)
# BUG - Stack Promotions - start
			szName = "PromotionButtonCircle" + str(iPromo)
			x, y = self.calculatePromotionButtonPosition(iPromo)
			gRect(szName).moveTo(x, y)
			self.addDDS(szName, "WHITE_CIRCLE_40", WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, iPromo)
			screen.hide(szName)
# BUG - Stack Promotions - end
# BUG - PLE - begin
			szName = self.PLE.PLE_PROMO_BUTTONS_UNITINFO + str(iPromo)
			self.addDDS(szName, gc.getPromotionInfo(iPromo), WidgetTypes.WIDGET_ACTION, gc.getPromotionInfo(iPromo).getActionInfoIndex())
			screen.hide(szName)
# BUG - PLE - end

		# *********************************************************************************
		# SCORES
		# *********************************************************************************

		screen.addPanel("ScoreBackground", u"", u"", True, False, 0, 0, 0, 0, PanelStyles.PANEL_STYLE_HUD_HELP)
		screen.hide("ScoreBackground")

		for i in range(gc.getMAX_CIV_PLAYERS()):
			szName = "ScoreText" + str(i)
			# advc.092 (note): I think these coordinates are only preliminary
			screen.setText(szName, "Background", u"", CvUtil.FONT_RIGHT_JUSTIFY, 996, 622, -0.3, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_CONTACT_CIV, i, -1)
			screen.hide(szName)

		# This should be a forced redraw screen
		screen.setForcedRedraw(True)

		# This should show the screen immidiately and pass input to the game
		screen.showScreen(PopupStates.POPUPSTATE_IMMEDIATE, True)

		szHideList = []

		szHideList.append("CreateGroup")
		szHideList.append("DeleteGroup")

		# City Tabs
		for i in range(g_NumCityTabTypes):
			szButtonID = "CityTab" + str(i)
			szHideList.append(szButtonID)

		for i in range(g_NumHurryInfos):
			szButtonID = "Hurry" + str(i)
			szHideList.append(szButtonID)

		szHideList.append("Hurry0")
		szHideList.append("Hurry1")

		szHideList.append(self.buildFilterBtnAll)
		szHideList.append(self.buildFilterBtnBuildings)
		szHideList.append(self.buildFilterBtnNational)
		szHideList.append(self.buildFilterBtnWorld)

		screen.registerHideList(szHideList, len(szHideList), 0)

		return 0

	def initMinimap(self): # advc (needed in two places)
		try: # advc.009b: Work around Python exceptions upon reloading scripts
			lRect = gRect("MiniMap")
		except KeyError:
			return
		try:
			# This should recreate the minimap on load games and returns if already exists -JW
			self.screen.initMinimap(lRect.x(), lRect.xRight(), lRect.y(), lRect.yBottom(), -0.1)
		except AttributeError:
			pass

	# advc.004k Cut from updateBottomButtonList
	def bottomListBtnSize(self):
# BUG - Build/Action Icon Size - start
		if MainOpt.isBuildIconSizeLarge():
			iSize = 64
		elif MainOpt.isBuildIconSizeMedium():
			iSize = 48
		else:
			iSize = 36
		# advc.092: Also apply some HUD scaling
		return BTNSZ(iSize, 0.7)
# BUG - Build/Action Icon Size - end

	def isWorldWonderClass(self, eBuildingClass):
		kClassInfo = gc.getBuildingClassInfo(eBuildingClass)
		return (kClassInfo.getMaxGlobalInstances() > -1 or kClassInfo.getMaxTeamInstances() > -1)

	def isNationalWonderClass(self, eBuildingClass):
		kClassInfo = gc.getBuildingClassInfo(eBuildingClass)
		return (kClassInfo.getMaxPlayerInstances() > -1 and not self.isWorldWonderClass(eBuildingClass))

	def isRegularBuildingClass(self, eBuildingClass):
		kClassInfo = gc.getBuildingClassInfo(eBuildingClass)
		return (kClassInfo.getMaxGlobalInstances() == -1 and kClassInfo.getMaxTeamInstances() == -1 and kClassInfo.getMaxPlayerInstances() == -1)

	def updateBottomButtonList(self):
# BUG - Build/Action Icon Size - start
		iButtonSize = self.bottomListBtnSize()
		# EF: minimum icon size for disabled buttons to work is 33 so these sizes won't fly
		# iButtonSize=32, iHeight=102
		# iButtonSize=24, iHeight=104
		# advc: I don't know why the BUG heights were chosen.
		# I think they should be multiples of the button size. That was almost exactly the case for the three button sizes above 33.
		iMaxHeight = gRect("CenterBottom").height() - 2 * VSPACE(1)
		# A little extra to avoid a vertical slider
		iHeight = max(iButtonSize, iMaxHeight - (iMaxHeight % iButtonSize) + 4)
		# <!-- custom: optional city-screen build-bar height tweak; raw pixel add so the last row can fit (e.g. 3.9 -> 4.0 rows) without growing the decorative ribbon.
		# Safe because CenterBottom (and thus iMaxHeight) no longer varies with the ribbon-extra-lift knob. (GPT-5.3-Codex + Claude code Opus 4.7) -->
		if CyInterface().isCityScreenUp() and self.iBarExtraHeightExtraManualAdjust != 0:
			iHeight += self.iBarExtraHeightExtraManualAdjust
		iVMargin = iMaxHeight - iHeight + 2 * VSPACE(1)
		# <advc.004> Lower position goes better with the filter buttons
		if PleOpt.isPLE_Style() and PleOpt.isShowButtons:
			iTMarginThresh = 8
		else: # </advc.004>
			iTMarginThresh = 3
		iTMargin = min(VSPACE(iTMarginThresh), iVMargin / 2)
		iBMargin = iVMargin - iTMargin

		# <!-- custom: optionally set how many extra rows of things to build (units, buildings, processes (research, wealth, culture)) you want to show in the city screen's production chooser bar.
		# 0 disables this feature entirely.
		# 1 Adds an extra row, 2 adds 2 extra rows, etc. Code added with the help of chatgpt 5.2 thanks -->
		# City screen: show more build rows in production chooser
		if self.iBarExtraRows > 0:
			if CyInterface().isCityScreenUp():
				iManualAdjust = self.iBarExtraRowsExtraManualAdjust
				# <!-- custom: scale manual pixel tweak across resolutions; keep sign, but scale magnitude like regular vertical spacing. (GPT-5.3-Codex) -->
				if iManualAdjust < 0:
					iManualAdjust = -VSPACE(-iManualAdjust)
				else:
					iManualAdjust = VSPACE(iManualAdjust)
				iTMargin += - (self.iBarExtraRows * iButtonSize) + iManualAdjust # extend upward; keep bottom aligned
		# End - City screen: show more build rows in production chooser

# BUG - Build/Action Icon Size - end
		lRect = RectLayout(gRect("BottomButtonMaxSpace"), 0, iTMargin, RectLayout.MAX, -iBMargin)
		gSetRectangle("BottomButtonList", lRect)
		# numLists, defaultWidth, defaultHeight
		self.screen.addMultiListControlGFC("BottomButtonList", u"", lRect.x(), lRect.y(), lRect.width(), lRect.height(), 4, iButtonSize, iButtonSize, TableStyles.TABLE_STYLE_STANDARD)

	def createPlotListFrames(self): # advc: Cut from interfaceScreen
		screen = self.screen
# BUG - PLE - begin
		iBtnSize = self.plotListUnitButtonSize()
		iRows = self.numPlotListRows()
		iCols = self.numPlotListButtonsPerRow()
		# <!-- custom: lazy-cache resolution-scaled unit-bar offsets; HLEN/VLEN assert on negatives, so dispatch via sign. (Claude code Opus 4.7) -->
		if self.iSAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_X_OFFSET is None:
			iRawPLPX = gc.getDefineINT("SAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_X_OFFSET")
			if iRawPLPX > 0:
				self.iSAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_X_OFFSET = HLEN(iRawPLPX)
			elif iRawPLPX < 0:
				self.iSAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_X_OFFSET = -HLEN(-iRawPLPX)
			else:
				self.iSAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_X_OFFSET = 0
		if self.iSAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_Y_OFFSET is None:
			iRawPLPY = gc.getDefineINT("SAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_Y_OFFSET")
			if iRawPLPY > 0:
				self.iSAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_Y_OFFSET = VLEN(iRawPLPY)
			elif iRawPLPY < 0:
				self.iSAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_Y_OFFSET = -VLEN(-iRawPLPY)
			else:
				self.iSAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_Y_OFFSET = 0
		for j in range(iRows):
			szPanelName = "PlotListPanel" + str(j)
			# <!-- custom: move unit bar ("PlotListPanel") more to the left so it is as of now aligned with where production chooser buttons start.
			# Done with the help of GPT-5.2-Codex thanks a lot, was 6 -->
			plotListPanelWGap = 0
			# (Don't want these margins to scale fully)
			gSetRect(szPanelName, "Top", gRect("LowerLeftCornerPanel").xRight() + HSPACE(2) + plotListPanelWGap + self.iSAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_X_OFFSET, gRect("CenterBottomPanel").y() - VSPACE(2) - 4 + (j - iRows) * iBtnSize + self.iSAS_CV_MAIN_INTERFACE_PLOTLISTPANEL_Y_OFFSET, iCols * iBtnSize + HSPACE(3), iBtnSize + VSPACE(1))
			self.addPanel(szPanelName, PanelStyles.PANEL_STYLE_EMPTY)
			for i in range(iCols):
				k = j * iCols + i
				xOffset = i * iBtnSize
				szBtn = "PlotListButton" + str(k)
# BUG - plot list - start
				szBtnPromoFrame = szBtn + "PromoFrame"
				iSizeDiff = self.plotListUnitFrameThickness()
				iFrameSize = iBtnSize - iSizeDiff
				gSetRectangle(szBtnPromoFrame, SquareLayout(gRect(szPanelName), xOffset + iSizeDiff, iSizeDiff, iFrameSize))
				self.addDDSAt(szBtnPromoFrame, szPanelName, "OVERLAY_PROMOTION_FRAME", WidgetTypes.WIDGET_PLOT_LIST, k)
				screen.hide(szBtnPromoFrame)
# BUG - plot list - end
				gSetRectangle(szBtn, SquareLayout(gRect(szPanelName), xOffset + iSizeDiff + 1, iSizeDiff + 1, iFrameSize))
				self.addCheckBoxAt(szBtn, szPanelName, "INTERFACE_BUTTONS_GOVERNOR", "BUTTON_HILITE_SQUARE", ButtonStyles.BUTTON_STYLE_LABEL, WidgetTypes.WIDGET_PLOT_LIST, k)
				screen.hide(szBtn)
				szBtnHealth = szBtn + "Health"
				gSetRectangle(szBtnHealth, RectLayout(gRect(szPanelName), xOffset + iSizeDiff + 1, (26 * iFrameSize) / 32, iFrameSize, (11 * iFrameSize) / 32))
				self.addStackedBarAt(szBtnHealth, szPanelName, WidgetTypes.WIDGET_GENERAL, k)
				screen.hide(szBtnHealth)
				szBtnIcon = szBtn + "Icon"
				iOverlaySize = self.unitButtonOverlaySize()
				gSetRectangle(szBtnIcon, SquareLayout(gRect(szPanelName), xOffset, 0, iOverlaySize))
				self.addDDSAt(szBtnIcon, szPanelName, "OVERLAY_MOVE", WidgetTypes.WIDGET_PLOT_LIST, k)
				screen.hide(szBtnIcon)

		self.PLE.preparePlotListObjects(screen)
		lBottommostPanel = gRect("PlotListPanel" + str(iRows - 1))
		lPlusMinusButtons = RowLayout(gRect("Top"), lBottommostPanel.xRight() + HSPACE(8), lBottommostPanel.y() + VSPACE(3), 2, HSPACE(5), BTNSZ(32))
		gSetRectangle("PlotListPlusMinus", lPlusMinusButtons)
		gSetRectangle("PlotListMinus", lPlusMinusButtons.next())
		gSetRectangle("PlotListPlus", lPlusMinusButtons.next())
		lUpDownButtons = ColumnLayout(gRect("Top"), gRect("PlotListPlusMinus").xCenter(), gRect("PlotListPlusMinus").y(), 2, VSPACE(5), BTNSZ(24))
		lUpDownButtons.move(-lUpDownButtons.width() / 2.0, -lUpDownButtons.height() / 4.0)
		gSetRectangle("PlotListUpDown", lUpDownButtons)
		gSetRectangle(self.PLE.PLOT_LIST_UP_NAME, lUpDownButtons.next())
		gSetRectangle(self.PLE.PLOT_LIST_DOWN_NAME, lUpDownButtons.next())
		# Copies for input handlers in PLE module
		gSetRectangle(self.PLE.PLOT_LIST_MINUS_NAME, gRect("PlotListMinus"))
		gSetRectangle(self.PLE.PLOT_LIST_PLUS_NAME, gRect("PlotListPlus"))

		self.setStyledButton("PlotListMinus", ButtonStyles.BUTTON_STYLE_ARROW_LEFT, WidgetTypes.WIDGET_PLOT_LIST_SHIFT, -1)
		screen.hide("PlotListMinus")
		self.setStyledButton(self.PLE.PLOT_LIST_MINUS_NAME, ButtonStyles.BUTTON_STYLE_ARROW_LEFT)
		screen.hide(self.PLE.PLOT_LIST_MINUS_NAME)

		self.setStyledButton("PlotListPlus", ButtonStyles.BUTTON_STYLE_ARROW_RIGHT, WidgetTypes.WIDGET_PLOT_LIST_SHIFT, 1)
		screen.hide("PlotListPlus")
		self.setStyledButton(self.PLE.PLOT_LIST_PLUS_NAME, ButtonStyles.BUTTON_STYLE_ARROW_RIGHT)
		screen.hide(self.PLE.PLOT_LIST_PLUS_NAME)

		self.setImageButton(self.PLE.PLOT_LIST_UP_NAME, "PLE_ARROW_UP")
		screen.hide(self.PLE.PLOT_LIST_UP_NAME)
		self.setImageButton(self.PLE.PLOT_LIST_DOWN_NAME, "PLE_ARROW_DOWN")
		screen.hide(self.PLE.PLOT_LIST_DOWN_NAME)
# BUG - PLE - end

	# Will update the screen (every 250 MS)
	def updateScreen(self):
		global g_szTimeText
		global g_iTimeTextCounter
		# <advc.009b> Work around crash upon reloading scripts
		try:
			gPoint("EndTurnText")
		except KeyError:
			return # </advc.009b>
		self.updateCachedLocalizedText()

#		BugUtil.debug("update - Turn %d, Player %d, Interface %d, End Turn Button %d ===", gc.getGame().getGameTurn(), gc.getGame().getActivePlayer(), CyInterface().getShowInterface(), CyInterface().getEndTurnState())

# BUG - Options - start
		BugOptions.write()
# BUG - Options - end
		# advc.009b: Set screen attribute after reloading scripts
		screen = self.screen = CyGInterfaceScreen("MainInterface", CvScreenEnums.MAIN_INTERFACE)
#		self.m_iNumPlotListButtons = (screen.getXResolution() - (iMultiListXL+iMultiListXR) - 2 * iPlotListUnitBtnSz) / iPlotListUnitBtnSz

		self.initMinimap()

		messageControl = CyMessageControl()

		bShow = False

		# Hide all interface widgets
		#screen.hide("EndTurnText")

		if (CyInterface().getShowInterface() != InterfaceVisibility.INTERFACE_HIDE_ALL and CyInterface().getShowInterface() != InterfaceVisibility.INTERFACE_MINIMAP_ONLY):
			if (gc.getGame().isPaused()):
				# Pause overrides other messages
				acOutput = localText.getText("SYSTEM_GAME_PAUSED", (gc.getPlayer(gc.getGame().getPausePlayer()).getNameKey(),))
				#screen.modifyLabel("EndTurnText", acOutput, CvUtil.FONT_CENTER_JUSTIFY)
				screen.setEndTurnState("EndTurnText", acOutput)
				bShow = True
			# advc.706: Side effect of switching active player
			elif (messageControl.GetFirstBadConnection() != -1 and not gc.getGame().isOption(GameOptionTypes.GAMEOPTION_RISE_FALL)):
				# Waiting on a bad connection to resolve
				if (messageControl.GetConnState(messageControl.GetFirstBadConnection()) == 1):
					if (gc.getGame().isMPOption(MultiplayerOptionTypes.MPOPTION_ANONYMOUS)):
						acOutput = localText.getText("SYSTEM_WAITING_FOR_PLAYER", (gc.getPlayer(messageControl.GetFirstBadConnection()).getNameKey(), 0))
					else:
						acOutput = localText.getText("SYSTEM_WAITING_FOR_PLAYER", (gc.getPlayer(messageControl.GetFirstBadConnection()).getNameKey(), (messageControl.GetFirstBadConnection() + 1)))
					#screen.modifyLabel("EndTurnText", acOutput, CvUtil.FONT_CENTER_JUSTIFY)
					screen.setEndTurnState("EndTurnText", acOutput)
					bShow = True
				elif (messageControl.GetConnState(messageControl.GetFirstBadConnection()) == 2):
					if (gc.getGame().isMPOption(MultiplayerOptionTypes.MPOPTION_ANONYMOUS)):
						acOutput = localText.getText("SYSTEM_PLAYER_JOINING", (gc.getPlayer(messageControl.GetFirstBadConnection()).getNameKey(), 0))
					else:
						acOutput = localText.getText("SYSTEM_PLAYER_JOINING", (gc.getPlayer(messageControl.GetFirstBadConnection()).getNameKey(), (messageControl.GetFirstBadConnection() + 1)))
					#screen.modifyLabel("EndTurnText", acOutput, CvUtil.FONT_CENTER_JUSTIFY)
					screen.setEndTurnState("EndTurnText", acOutput)
					bShow = True
			else:
				# Flash select messages if no popups are present
				if (CyInterface().shouldDisplayReturn()):
					# advc.004t: Disable flashing "Press Esc to return"
					acOutput = ""#localText.getText("SYSTEM_RETURN", ())
					#screen.modifyLabel("EndTurnText", acOutput, CvUtil.FONT_CENTER_JUSTIFY)
					screen.setEndTurnState("EndTurnText", acOutput)
					bShow = True
				# advc.127: No "Waiting" message during Auto Play
				elif (CyInterface().shouldDisplayWaitingOthers() and not gc.getPlayer(gc.getGame().getActivePlayer()).isHumanDisabled()):
					acOutput = self.szTextWaiting
					#screen.modifyLabel("EndTurnText", acOutput, CvUtil.FONT_CENTER_JUSTIFY)
					screen.setEndTurnState("EndTurnText", acOutput)
					bShow = True
				elif (CyInterface().shouldDisplayEndTurn()):
# BUG - Reminders - start
					if (ReminderEventManager.g_turnReminderTexts):
						acOutput = u"%s" % ReminderEventManager.g_turnReminderTexts
					elif MainOpt.isShowEndTurnMessage(): # advc.002n
						acOutput = self.szTextEndTurn
					# advc.002n: So that toggling the option immediately hides the message
					else:
						acOutput = ""
# BUG - Reminders - end
					#screen.modifyLabel("EndTurnText", acOutput, CvUtil.FONT_CENTER_JUSTIFY)
					screen.setEndTurnState("EndTurnText", acOutput)
					bShow = True
				elif (CyInterface().shouldDisplayWaitingYou()):
					acOutput = self.szTextWaitingForYou
					#screen.modifyLabel("EndTurnText", acOutput, CvUtil.FONT_CENTER_JUSTIFY)
					screen.setEndTurnState("EndTurnText", acOutput)
					bShow = True
# BUG - Options - start
				# advc.004: Option disabled
				# elif MainOpt.isShowOptionsKeyReminder()
				# 	if BugPath.isMac():
				# 		acOutput = localText.getText("TXT_KEY_BUG_OPTIONS_KEY_REMINDER_MAC", (BugPath.getModName(),))
				# 	else:
				# 		acOutput = localText.getText("TXT_KEY_BUG_OPTIONS_KEY_REMINDER", (BugPath.getModName(),))
				# 	#screen.modifyLabel("EndTurnText", acOutput, CvUtil.FONT_CENTER_JUSTIFY)
				# 	screen.setEndTurnState("EndTurnText", acOutput)
				# 	bShow = True
# BUG - Options - end
		if bShow:
			screen.showEndTurn("EndTurnText")
			if (CyInterface().getShowInterface() == InterfaceVisibility.INTERFACE_SHOW or CyInterface().isCityScreenUp()):
				szTextPosKey = "EndTurnText"
			else:
				szTextPosKey = "EndTurnTextMin"			
			screen.moveItem("EndTurnText", gPoint(szTextPosKey).x(), gPoint(szTextPosKey).y(), -0.1)
		else:
			screen.hideEndTurn("EndTurnText")

		self.updateEndTurnButton()

# BUG - NJAGC - start
		global g_bShowTimeTextAlt
		if (CyInterface().getShowInterface() != InterfaceVisibility.INTERFACE_HIDE_ALL and CyInterface().getShowInterface() != InterfaceVisibility.INTERFACE_MINIMAP_ONLY and CyInterface().getShowInterface() != InterfaceVisibility.INTERFACE_ADVANCED_START):
			# <advc.067> Moved out of the isEnabled block
			if ClockOpt.isShowEra():
				screen.show("EraText")
			else:
				screen.hide("EraText")
			# </advc.067>
			if ClockOpt.isEnabled():
				if (ClockOpt.isAlternateTimeText()):
					if (CyUserProfile().wasClockJustTurnedOn() or g_iTimeTextCounter <= 0):
						# reset timer, display primary
						g_bShowTimeTextAlt = False
						# advc.067: was getAlternatePeriod
						g_iTimeTextCounter = ClockOpt.getPrimaryPeriod() * 1000
						CyUserProfile().setClockJustTurnedOn(False)
					else:
						# countdown timer
						g_iTimeTextCounter -= 250
						if g_iTimeTextCounter <= 0:
							# <advc.067>
							if g_bShowTimeTextAlt:
								iPeriod = ClockOpt.getPrimaryPeriod()
							else: # </advc.067>
								iPeriod = ClockOpt.getAlternatePeriod()
							# timer elapsed, toggle between primary and alternate
							g_iTimeTextCounter = iPeriod * 1000
							g_bShowTimeTextAlt = not g_bShowTimeTextAlt
				else:
					g_bShowTimeTextAlt = False
			#else:
			#	screen.hide("EraText") # advc.067
			self.updateTimeText()
			self.setLabel("TimeText", "Background", sasFontTagLabel + g_szTimeText + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, FontTypes.GAME_FONT, -0.3)
			screen.show("TimeText")

		else:
			screen.hide("TimeText")
			screen.hide("EraText")
# BUG - NJAGC - end

# BUG - PLE - start
		# this ensures that the info pane is closed after a greater mouse pos change
		self.PLE.checkInfoPane(CyInterface().getMousePos())
# BUG - PLE - end

		return 0

	# Will redraw the interface
	def redraw(self):
		# <advc.706> Freeze main interface during interlude
		if gc.getGame().isRFInterlude():
			return 0 # </advc.706>
#		BugUtil.debug("redraw - Turn %d, Player %d, Interface %d, End Turn Button %d", gc.getGame().getGameTurn(), gc.getGame().getActivePlayer(), CyInterface().getShowInterface(), CyInterface().getEndTurnState())
		# advc.009b: Set screen attribute after reloading scripts
		screen = self.screen = CyGInterfaceScreen("MainInterface", CvScreenEnums.MAIN_INTERFACE)

# BUG - Field of View - start
		#self.setFieldofView(screen, CyInterface().isCityScreenUp())
		# K-Mod. Using the default for the city screen is an ok idea, but it doesn't work properly because the screen is drawn before the value is changed.
		self.setFieldofView(screen, False)
# BUG - Field of View - end

		# Check Dirty Bits, see what we need to redraw...
		if (CyInterface().isDirty(InterfaceDirtyBits.PercentButtons_DIRTY_BIT)):
			# Percent Buttons
			self.updatePercentButtons()
			CyInterface().setDirty(InterfaceDirtyBits.PercentButtons_DIRTY_BIT, False)
		if (CyInterface().isDirty(InterfaceDirtyBits.Flag_DIRTY_BIT)):
			# <!-- custom: changed from unconditional refresh to define-gated refresh so this optional UI feature stays no-op when disabled. (GPT-5.3-Codex) -->
			if self.IS_SAS_CV_MAIN_INTERFACE_TOP_LEFT_FLAG_AND_LEFT_SIDE_TEXT_SHIFT:
				self.updateFlag()
			CyInterface().setDirty(InterfaceDirtyBits.Flag_DIRTY_BIT, False)
		if (CyInterface().isDirty(InterfaceDirtyBits.MiscButtons_DIRTY_BIT)):
			# Miscellaneous buttons (policy advisor screen, etc)
			self.updateMiscButtons()
			CyInterface().setDirty(InterfaceDirtyBits.MiscButtons_DIRTY_BIT, False)
		if (CyInterface().isDirty(InterfaceDirtyBits.InfoPane_DIRTY_BIT)):
			# Info Pane Dirty Bit. This must come before updatePlotListButtons so that the entity widget appears in front of the stats
			self.updateInfoPaneStrings()
			CyInterface().setDirty(InterfaceDirtyBits.InfoPane_DIRTY_BIT, False)
		if (CyInterface().isDirty(InterfaceDirtyBits.PlotListButtons_DIRTY_BIT)):
#			BugUtil.debug("dirty PlotListButtons end - %s %s %s", self.bVanCurrentlyShowing, self.bPLECurrentlyShowing, self.bBUGCurrentlyShowing)
			# Plot List Buttons Dirty
			self.updatePlotListButtons()
			CyInterface().setDirty(InterfaceDirtyBits.PlotListButtons_DIRTY_BIT, False)
#			BugUtil.debug("dirty PlotListButtons start - %s %s %s", self.bVanCurrentlyShowing, self.bPLECurrentlyShowing, self.bBUGCurrentlyShowing)
		if (CyInterface().isDirty(InterfaceDirtyBits.SelectionButtons_DIRTY_BIT)):
			# Selection Buttons Dirty
			self.updateSelectionButtons()
			CyInterface().setDirty(InterfaceDirtyBits.SelectionButtons_DIRTY_BIT, False)
		if (CyInterface().isDirty(InterfaceDirtyBits.ResearchButtons_DIRTY_BIT)):
			# Research Buttons Dirty
			self.updateResearchButtons()
			CyInterface().setDirty(InterfaceDirtyBits.ResearchButtons_DIRTY_BIT, False)
		if (CyInterface().isDirty(InterfaceDirtyBits.CitizenButtons_DIRTY_BIT)):
			# Citizen Buttons Dirty

# BUG - city specialist - start
			self.updateCitizenButtons_hide()
			pHeadSelectedCity = CyInterface().getHeadSelectedCity()
			if (pHeadSelectedCity and CyInterface().isCityScreenUp() and CyInterface().getShowInterface() == InterfaceVisibility.INTERFACE_SHOW):
				if (CityScreenOpt.isCitySpecialist_Stacker()):
					self.updateCitizenButtons_Stacker(pHeadSelectedCity)
				elif (CityScreenOpt.isCitySpecialist_Chevron()):
					self.updateCitizenButtons_Chevron(pHeadSelectedCity)
				else:
					self.updateCitizenButtons(pHeadSelectedCity)
				# <advc.004> Show the SpecialistLabel regardless of BUG options
				# <!-- custom: remove the 3 gray bars ("Trade Routes", "Buildings", "Specialists") as they take a lot of room and are unneeded, and we don't have a "Bonuses" bar for example so no reason to have these as well either.
				# Change with the help of gemini 3 pro thanks -->
				# Step 1: Hide the Header Widgets (Gray Bars)
				# if self.isShowSpecialistLabel():
				# 	# Cut from updateCitizenButtons_Stacker ...
				# 	screen.show("SpecialistLabelBackground")
				# 	screen.show("SpecialistLabel") # </advc.004>
# BUG - city specialist - end

			CyInterface().setDirty(InterfaceDirtyBits.CitizenButtons_DIRTY_BIT, False)
		if (CyInterface().isDirty(InterfaceDirtyBits.GameData_DIRTY_BIT)):
			# Game Data Strings Dirty
			self.updateGameDataStrings()
			CyInterface().setDirty(InterfaceDirtyBits.GameData_DIRTY_BIT, False)
		if (CyInterface().isDirty(InterfaceDirtyBits.Help_DIRTY_BIT)):
			# Help Dirty bit
			self.updateHelpStrings()
			CyInterface().setDirty(InterfaceDirtyBits.Help_DIRTY_BIT, False)
		if (CyInterface().isDirty(InterfaceDirtyBits.CityScreen_DIRTY_BIT)):
			# Selection Data Dirty Bit
			self.updateCityScreen()
			CyInterface().setDirty(InterfaceDirtyBits.Domestic_Advisor_DIRTY_BIT, True)
			CyInterface().setDirty(InterfaceDirtyBits.CityScreen_DIRTY_BIT, False)
		bScoreStringsUpdated = False # advc.004z
		if ((CyInterface().isDirty(InterfaceDirtyBits.Score_DIRTY_BIT)) or CyInterface().checkFlashUpdate()):
			# Scores!
			self.updateScoreStrings()
			CyInterface().setDirty(InterfaceDirtyBits.Score_DIRTY_BIT, False)
			bScoreStringsUpdated = True # advc.004z
		# <advc.085>
		if CyInterface().isDirty(InterfaceDirtyBits.ScoreHelp_DIRTY_BIT):
			if gAlignedScoreboard:
				gAlignedScoreboard.hide(screen, True)
			CyInterface().setDirty(InterfaceDirtyBits.ScoreHelp_DIRTY_BIT, False)
		# </advc.085>
		# advc.004z: Need to determine whether there are options to show even if GlobeInfo hasn't changed. And hide the scores accordingly.
		if (CyInterface().isDirty(InterfaceDirtyBits.GlobeInfo_DIRTY_BIT) or (bScoreStringsUpdated and CyEngine().isGlobeviewUp())):
			# Globeview and Globelayer buttons
			CyInterface().setDirty(InterfaceDirtyBits.GlobeInfo_DIRTY_BIT, False)
			#self.updateGlobeviewButtons()
			# <advc.004z> Show/hide scoreboard depending on whether the layer has options
			bHasOptions = self.updateGlobeviewButtons()
			if CyEngine().isGlobeviewUp() and MainOpt.isScoresInGlobeView():
				if bHasOptions:
					self.hideScoreStrings()
				else:
					# Need this call even if already updated, just to show the background - which updateGlobeviewButtons has just hidden because there were no options to display. Awkward that options and score use the same background widget.
					self.updateScoreStrings(bScoreStringsUpdated)
			# </advc.004z>
		return 0

	# <!-- custom: remove the 3 gray bars ("Trade Routes", "Buildings", "Specialists") as they take a lot of room and are unneeded, and we don't have a "Bonuses" bar for example so no reason to have these as well either.
	# Change with the help of gemini 3 pro thanks -->
	# # advc.004:
	# def isShowSpecialistLabel(self):
	# 	# Not quite enough space on low res. (Fixme: The Stacker layout does leave
	# 	# enough space, but then we'd need a separate label and background panel
	# 	# placed by updateCitizenButton_Stacker; currently, the position is set
	# 	# statically, ignoring options.)
	# 	return (gRect("Top").height() > 900)

	# K-Mod: There are some special rules for which buttons should be shown and when.
	# I'd rather have all those rules in one place. ie. here.
	def showCommercePercent(self, eCommerce, ePlayer):
		player = gc.getPlayer(ePlayer)
		if not player.isFoundedFirstCity():
			return False
		if eCommerce == CommerceTypes.COMMERCE_GOLD and not CyInterface().isCityScreenUp():
			return False
		if player.getCommercePercent(eCommerce) > 0:
			return True
		# advc.120c
		if (eCommerce == CommerceTypes.COMMERCE_ESPIONAGE and (gc.getGame().isOption(GameOptionTypes.GAMEOPTION_NO_ESPIONAGE) or gc.getTeam(player.getTeam()).getHasMetCivCount(True) == 0 or (gc.getPlayer(ePlayer).getCommercePercent(eCommerce) == 0 and not CyInterface().isCityScreenUp() and MainOpt.isHideEspSlider()))):
			return False
		return player.isCommerceFlexible(eCommerce)

	def updatePercentButtons(self):
		screen = self.screen
		for iCommerce in range(CommerceTypes.NUM_COMMERCE_TYPES):
			szIndex = str(iCommerce)
			screen.hide("IncreasePercent" + szIndex)
			screen.hide("DecreasePercent" + szIndex)
# BUG - Min/Max Sliders - start
			screen.hide("MaxPercent" + szIndex)
			screen.hide("MinPercent" + szIndex)
# BUG - Min/Max Sliders - start
		pHeadSelectedCity = CyInterface().getHeadSelectedCity()
		if (CyInterface().isCityScreenUp() and pHeadSelectedCity.getOwner() != gc.getGame().getActivePlayer()):
				# K-Mod: Debug mode doesn't allow us to use the buttons
				#and not gc.getGame().isDebugMode()
			return
		if (CyInterface().getShowInterface() == InterfaceVisibility.INTERFACE_HIDE_ALL or CyInterface().getShowInterface() == InterfaceVisibility.INTERFACE_MINIMAP_ONLY or CyInterface().getShowInterface() == InterfaceVisibility.INTERFACE_ADVANCED_START):
			return
		if self.iCOMMERCE_PERCENT_CHANGE_INCREMENTS is None:
			self.iCOMMERCE_PERCENT_CHANGE_INCREMENTS = gc.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS")
		if self.iSAS_COMMERCE_SLIDERS_FAST_INCREMENT_PERCENT is None:
			self.iSAS_COMMERCE_SLIDERS_FAST_INCREMENT_PERCENT = gc.getDefineINT("SAS_COMMERCE_SLIDERS_FAST_INCREMENT_PERCENT")
		# <advc.092>
		for i in range(4):
			gRect("CommerceSliderBtns" + str(i)).resetIter() # </advc.092>
		for i in range(CommerceTypes.NUM_COMMERCE_TYPES):
			# Intentional offset...
			eCommerce = (i + 1) % CommerceTypes.NUM_COMMERCE_TYPES
			#if (not gc.getActivePlayer().isCommerceFlexible(eCommerce) and (not CyInterface().isCityScreenUp() or eCommerce != CommerceTypes.COMMERCE_GOLD)):
			if not self.showCommercePercent(eCommerce, gc.getGame().getActivePlayer()): # K-Mod
				continue
# BUG - Min/Max Sliders - start
			bEnable = gc.getActivePlayer().isCommerceFlexible(eCommerce)
			iCol = 0 # advc.092
			# <!-- custom: Added Min/Max +/- pair on the city screen (because map-view commerce sliders are hidden there).
			# Not added to the Finance/Espionage advisor tabs: those are windowed so the right-side map-view sliders stay reachable. (Claude code Opus 4.7) -->
			if MainOpt.isShowMinMaxCommerceButtons():
				szString = "MaxPercent" + str(eCommerce)
				gSetRectangle(szString, gRect("CommerceSliderBtns0").next())
				self.setStyledButton(szString, ButtonStyles.BUTTON_STYLE_CITY_PLUS, WidgetTypes.WIDGET_CHANGE_PERCENT, eCommerce, self.iSAS_COMMERCE_SLIDERS_FAST_INCREMENT_PERCENT)
				screen.show(szString)
				screen.enable(szString, bEnable)
				szString = "MinPercent" + str(eCommerce)
				gSetRectangle(szString, gRect("CommerceSliderBtns3").next())
				self.setStyledButton(szString, ButtonStyles.BUTTON_STYLE_CITY_MINUS, WidgetTypes.WIDGET_CHANGE_PERCENT, eCommerce, -self.iSAS_COMMERCE_SLIDERS_FAST_INCREMENT_PERCENT)
				screen.show(szString)
				screen.enable(szString, bEnable)
				iCol = 1
			szString = "IncreasePercent" + str(eCommerce)
			gSetRectangle(szString, gRect("CommerceSliderBtns" + str(iCol)).next())
			self.setStyledButton(szString, ButtonStyles.BUTTON_STYLE_CITY_PLUS, WidgetTypes.WIDGET_CHANGE_PERCENT, eCommerce, self.iCOMMERCE_PERCENT_CHANGE_INCREMENTS)
			screen.show(szString)
			screen.enable(szString, bEnable)
			szString = "DecreasePercent" + str(eCommerce)
			gSetRectangle(szString, gRect("CommerceSliderBtns" + str(iCol + 1)).next())
			self.setStyledButton(szString, ButtonStyles.BUTTON_STYLE_CITY_MINUS, WidgetTypes.WIDGET_CHANGE_PERCENT, eCommerce, -self.iCOMMERCE_PERCENT_CHANGE_INCREMENTS)
			screen.show(szString)
			screen.enable(szString, bEnable)
			# moved enabling above
# BUG - Min/Max Sliders - end

	def resetEndTurnObjects(self): # BUG helper
		# Clears the end turn text and hides it and the button.
		#
		screen = CyGInterfaceScreen("MainInterface", CvScreenEnums.MAIN_INTERFACE)
		screen.setEndTurnState("EndTurnText", u"")
		screen.hideEndTurn("EndTurnText")
		screen.hideEndTurn("EndTurnButton")
		screen.hide("EndTurnButtonPanel")
		screen.hide("EndTurnButtonPanelLeft")

	def updateEndTurnButton(self):
		global g_eEndTurnButtonState
		screen = self.screen
		if (CyInterface().shouldDisplayEndTurnButton() and CyInterface().getShowInterface() == InterfaceVisibility.INTERFACE_SHOW):
			eState = CyInterface().getEndTurnState()
			bShow = False
			if (eState == EndTurnButtonStates.END_TURN_OVER_HIGHLIGHT):
				screen.setEndTurnState("EndTurnButton", u"Red")
				bShow = True
			elif (eState == EndTurnButtonStates.END_TURN_OVER_DARK):
				screen.setEndTurnState("EndTurnButton", u"Red")
				bShow = True
			elif (eState == EndTurnButtonStates.END_TURN_GO):
				screen.setEndTurnState("EndTurnButton", u"Green")
				bShow = True

			if bShow:
				screen.showEndTurn("EndTurnButton")
				if not CyInterface().isCityScreenUp():
					self._setPanelSizeFromRect("EndTurnButtonPanel")
					screen.show("EndTurnButtonPanel")
					self._setPanelSizeFromRect("EndTurnButtonPanelLeft")
					screen.show("EndTurnButtonPanelLeft")
				else:
					screen.hide("EndTurnButtonPanel")
					screen.hide("EndTurnButtonPanelLeft")
			else:
				screen.hideEndTurn("EndTurnButton")
				screen.hide("EndTurnButtonPanel")
				screen.hide("EndTurnButtonPanelLeft")

			if g_eEndTurnButtonState == eState:
				return

			g_eEndTurnButtonState = eState
		else:
			screen.hideEndTurn("EndTurnButton")
			screen.hide("EndTurnButtonPanel")
			screen.hide("EndTurnButtonPanelLeft")

	def updateMiscButtons(self):
		screen = self.screen
		kInterface = CyInterface()
		bCityScreen = kInterface.isCityScreenUp()
		eUIVis = kInterface.getShowInterface()
		self._applyCityScreenSideLift(bCityScreen)
		self._applyMapViewRightPanelNarrow(not bCityScreen)
		if self.bSASLastCommerceRectsCityScreen is None or self.bSASLastCommerceRectsCityScreen != bCityScreen:
			self.setCommerceAdjustRects()
			self.bSASLastCommerceRectsCityScreen = bCityScreen
			kInterface.setDirty(InterfaceDirtyBits.PercentButtons_DIRTY_BIT, True)
# BUG - Great Person Bar - start
		if (eUIVis != InterfaceVisibility.INTERFACE_HIDE_ALL and eUIVis != InterfaceVisibility.INTERFACE_MINIMAP_ONLY and eUIVis != InterfaceVisibility.INTERFACE_ADVANCED_START):
			self.updateGreatPersonBar(screen)
# BUG - Great Person Bar - end
		#CyInterface().shouldDisplayFlag() and
		# <advc.004y> Don't check shouldDisplayFlag for the Civilopedia button, but do check if the city screen is up.
		# <!-- custom: we also show the flag in city screen, unlike base AdvCiv.
		# Because this mod moves it to a small top-left slot, it no longer interferes with city-action buttons; therefore keeping it visible in city screen is useful (traits hover + capital jump) rather than intrusive.
		# This remains an optional behavior. (GPT-5.3-Codex) -->
		if (eUIVis == InterfaceVisibility.INTERFACE_SHOW):
			if self.IS_SAS_CV_MAIN_INTERFACE_TOP_LEFT_FLAG_AND_LEFT_SIDE_TEXT_SHIFT:
				self.updateFlag()
			if bCityScreen:
				screen.hide("InterfaceHelpButton")
				screen.hide("MainMenuButton")
			else:
				screen.show("InterfaceHelpButton")
				if kInterface.shouldDisplayFlag():
					screen.show("MainMenuButton")
				else:
					screen.hide("MainMenuButton")
			# </advc.004y>
		else:
			screen.hide("CivilizationFlag")
			screen.hide("InterfaceHelpButton")
			screen.hide("MainMenuButton")

		if (eUIVis == InterfaceVisibility.INTERFACE_HIDE_ALL or eUIVis == InterfaceVisibility.INTERFACE_MINIMAP_ONLY):
			screen.hide("LowerLeftCornerPanel")
			screen.hide("LowerLeftCornerBackgr")
			screen.hide("InterfaceTopBackgroundWidget")
			screen.hide("CenterBottomPanel")
			screen.hide("LowerRightCornerPanel")
			screen.hide("MiniMapPanel")
			screen.hide("InterfaceTopLeft")
			screen.hide("InterfaceTopCenter")
			screen.hide("InterfaceTopRight")
			screen.hide("TurnLogButton")
			screen.hide("EspionageAdvisorButton")
			screen.hide("DomesticAdvisorButton")
			screen.hide("ForeignAdvisorButton")
			screen.hide("TechAdvisorButton")
			screen.hide("WorldAdvisorButton")
			screen.hide("PolicyAdvisorButton")
			screen.hide("ForeignDiplomacyAdvisorButton")
			screen.hide("MilitaryAdvisorButton")
			screen.hide("VictoryAdvisorButton")
			screen.hide("InfoAdvisorButton")
# BUG - City Arrows - start
			screen.hide("MainCityScrollMinus")
			screen.hide("MainCityScrollPlus")
# BUG - City Arrows - end
			screen.hide("BUGOptionsScreenWidget") # BUG - BUG Option Button
# BUG - field of view slider - start
			screen.hide("FoVSliderText")
			screen.hide("FoVSlider")
# BUG - field of view slider - end
		elif (bCityScreen):
			screen.show("LowerLeftCornerPanel")
			#screen.show("LowerLeftCornerBackgr")
			screen.show("InterfaceTopBackgroundWidget")
			screen.show("CenterBottomPanel")
			screen.show("LowerRightCornerPanel")
			screen.show("MiniMapPanel")
			screen.hide("InterfaceTopLeft")
			screen.hide("InterfaceTopCenter")
			screen.hide("InterfaceTopRight")
			screen.hide("TurnLogButton")
			screen.hide("EspionageAdvisorButton")
			screen.hide("DomesticAdvisorButton")
			screen.hide("ForeignAdvisorButton")
			screen.hide("TechAdvisorButton")
			screen.hide("WorldAdvisorButton")
			screen.hide("PolicyAdvisorButton")
			screen.hide("ForeignDiplomacyAdvisorButton")
			screen.hide("MilitaryAdvisorButton")
			screen.hide("VictoryAdvisorButton")
			screen.hide("InfoAdvisorButton")
# BUG - City Arrows - start
			screen.hide("MainCityScrollMinus")
			screen.hide("MainCityScrollPlus")
# BUG - City Arrows - end
			screen.hide("BUGOptionsScreenWidget") # BUG - BUG Option Button
# BUG - field of view slider - start
			screen.hide("FoVSliderText")
			screen.hide("FoVSlider")
# BUG - field of view slider - end
		elif (eUIVis == InterfaceVisibility.INTERFACE_HIDE):
			screen.hide("LowerLeftCornerPanel")
			screen.hide("LowerLeftCornerBackgr")
			screen.show("InterfaceTopBackgroundWidget")
			screen.hide("CenterBottomPanel")
			screen.hide("LowerRightCornerPanel")
			screen.hide("MiniMapPanel")
			screen.show("InterfaceTopLeft")
			if self.isShowTopBarsFrame(): # advc.092
				screen.show("InterfaceTopCenter")
			screen.show("InterfaceTopRight")
			screen.show("TurnLogButton")
			screen.show("EspionageAdvisorButton")
			screen.show("DomesticAdvisorButton")
			screen.show("ForeignAdvisorButton")
			screen.show("TechAdvisorButton")
			screen.show("WorldAdvisorButton")
			screen.show("PolicyAdvisorButton")
			screen.show("ForeignDiplomacyAdvisorButton")
			screen.show("MilitaryAdvisorButton")
			screen.show("VictoryAdvisorButton")
			screen.show("InfoAdvisorButton")
# BUG - City Arrows - start
			screen.hide("MainCityScrollMinus")
			screen.hide("MainCityScrollPlus")
# BUG - City Arrows - end
# BUG - BUG Option Button - Start
			if MainOpt.isShowOptionsButton():
				screen.show("BUGOptionsScreenWidget")
# BUG - BUG Option Button - End
# BUG - field of view slider - start
			screen.hide("FoVSliderText")
			screen.hide("FoVSlider")
# BUG - field of view slider - end
			screen.moveToFront("TurnLogButton")
			screen.moveToFront("EspionageAdvisorButton")
			screen.moveToFront("DomesticAdvisorButton")
			screen.moveToFront("ForeignAdvisorButton")
			screen.moveToFront("TechAdvisorButton")
			screen.moveToFront("WorldAdvisorButton")
			screen.moveToFront("PolicyAdvisorButton")
			screen.moveToFront("ForeignDiplomacyAdvisorButton")
			screen.moveToFront("MilitaryAdvisorButton")
			screen.moveToFront("VictoryAdvisorButton")
			screen.moveToFront("InfoAdvisorButton")
#			screen.moveToFront("BUGOptionsScreenWidget") # BUG - BUG Option Button
		elif (eUIVis == InterfaceVisibility.INTERFACE_ADVANCED_START):
			screen.hide("LowerLeftCornerPanel")
			screen.hide("LowerLeftCornerBackgr")
			screen.hide("InterfaceTopBackgroundWidget")
			screen.hide("CenterBottomPanel")
			screen.hide("LowerRightCornerPanel")
			screen.show("MiniMapPanel")
			screen.hide("InterfaceTopLeft")
			screen.hide("InterfaceTopCenter")
			screen.hide("InterfaceTopRight")
			screen.hide("TurnLogButton")
			screen.hide("EspionageAdvisorButton")
			screen.hide("DomesticAdvisorButton")
			screen.hide("ForeignAdvisorButton")
			screen.hide("TechAdvisorButton")
			screen.hide("WorldAdvisorButton")
			screen.hide("PolicyAdvisorButton")
			screen.hide("ForeignDiplomacyAdvisorButton")
			screen.hide("MilitaryAdvisorButton")
			screen.hide("VictoryAdvisorButton")
			screen.hide("InfoAdvisorButton")
# BUG - City Arrows - start
			screen.hide("MainCityScrollMinus")
			screen.hide("MainCityScrollPlus")
# BUG - City Arrows - end
			screen.hide("BUGOptionsScreenWidget") # BUG - BUG Option Button
		elif (CyEngine().isGlobeviewUp()):
			screen.hide("LowerLeftCornerPanel")
			screen.hide("LowerLeftCornerBackgr")
			screen.hide("InterfaceTopBackgroundWidget")
			screen.hide("CenterBottomPanel")
			screen.show("LowerRightCornerPanel")
			screen.show("MiniMapPanel")
			screen.show("InterfaceTopLeft")
			if self.isShowTopBarsFrame(): # advc.092
				screen.show("InterfaceTopCenter")
			screen.show("InterfaceTopRight")
			screen.show("TurnLogButton")
			screen.show("EspionageAdvisorButton")
			screen.show("DomesticAdvisorButton")
			screen.show("ForeignAdvisorButton")
			screen.show("TechAdvisorButton")
			screen.show("WorldAdvisorButton")
			screen.show("PolicyAdvisorButton")
			screen.show("ForeignDiplomacyAdvisorButton")
			screen.show("MilitaryAdvisorButton")
			screen.show("VictoryAdvisorButton")
			screen.show("InfoAdvisorButton")
# BUG - City Arrows - start
			screen.hide("MainCityScrollMinus")
			screen.hide("MainCityScrollPlus")
# BUG - City Arrows - end
# BUG - BUG Option Button - Start
			if MainOpt.isShowOptionsButton():
				screen.show("BUGOptionsScreenWidget")
# BUG - BUG Option Button - End
# BUG - field of view slider - start
			screen.hide("FoVSliderText")
			screen.hide("FoVSlider")
# BUG - field of view slider - end
			screen.moveToFront("TurnLogButton")
			screen.moveToFront("EspionageAdvisorButton")
			screen.moveToFront("DomesticAdvisorButton")
			screen.moveToFront("ForeignAdvisorButton")
			screen.moveToFront("TechAdvisorButton")
			screen.moveToFront("WorldAdvisorButton")
			screen.moveToFront("PolicyAdvisorButton")
			screen.moveToFront("ForeignDiplomacyAdvisorButton")
			screen.moveToFront("MilitaryAdvisorButton")
			screen.moveToFront("VictoryAdvisorButton")
			screen.moveToFront("InfoAdvisorButton")
#			screen.moveToFront("BUGOptionsScreenWidget") # BUG - BUG Option Button
		else:
			screen.show("LowerLeftCornerPanel")
			#screen.show("LowerLeftCornerBackgr")
			screen.show("InterfaceTopBackgroundWidget")
			screen.show("CenterBottomPanel")
			screen.show("LowerRightCornerPanel")
			screen.show("MiniMapPanel")
			screen.show("InterfaceTopLeft")
			if self.isShowTopBarsFrame(): # advc.092
				screen.show("InterfaceTopCenter")
			screen.show("InterfaceTopRight")
			screen.show("TurnLogButton")
			screen.show("EspionageAdvisorButton")
			screen.show("DomesticAdvisorButton")
			screen.show("ForeignAdvisorButton")
			screen.show("TechAdvisorButton")
			screen.show("WorldAdvisorButton")
			screen.show("PolicyAdvisorButton")
			screen.show("ForeignDiplomacyAdvisorButton")
			screen.show("MilitaryAdvisorButton")
			screen.show("VictoryAdvisorButton")
			screen.show("InfoAdvisorButton")
# BUG - City Arrows - start
			if (MainOpt.isShowCityCycleArrows()):
				screen.show("MainCityScrollMinus")
				screen.show("MainCityScrollPlus")
			else:
				screen.hide("MainCityScrollMinus")
				screen.hide("MainCityScrollPlus")
# BUG - City Arrows - end
# BUG - BUG Option Button - Start
			if MainOpt.isShowOptionsButton():
				screen.show("BUGOptionsScreenWidget")
# BUG - BUG Option Button - End
# BUG - field of view slider - start
			if (MainOpt.isShowFieldOfView()):
				screen.show("FoVSliderText")
				screen.show("FoVSlider")
			else:
				screen.hide("FoVSliderText")
				screen.hide("FoVSlider")
# BUG - field of view slider - end
			screen.moveToFront("TurnLogButton")
			screen.moveToFront("EspionageAdvisorButton")
			screen.moveToFront("DomesticAdvisorButton")
			screen.moveToFront("ForeignAdvisorButton")
			screen.moveToFront("TechAdvisorButton")
			screen.moveToFront("WorldAdvisorButton")
			screen.moveToFront("PolicyAdvisorButton")
			screen.moveToFront("ForeignDiplomacyAdvisorButton")
			screen.moveToFront("MilitaryAdvisorButton")
			screen.moveToFront("VictoryAdvisorButton")
			screen.moveToFront("InfoAdvisorButton")
#			screen.moveToFront("BUGOptionsScreenWidget") # BUG - BUG Option Button
		screen.updateMinimapVisibility()
		return 0

	# Update plot List Buttons
	def updatePlotListButtons(self):
#		BugUtil.debug("updatePlotListButtons start - %s %s %s", self.bVanCurrentlyShowing, self.bPLECurrentlyShowing, self.bBUGCurrentlyShowing)
		screen = self.screen
		kInterface = CyInterface()
		self.updatePlotListButtons_Hide(screen)
		self.updatePlotListButtons_Common(screen)

		# <!-- custom: optionally hide the unit plot list bar only while a city is selected (ChatGPT-5.2 Thinking) -->
		# This avoids overlap when the city build bar is configured to show extra rows.
		if (self.IS_SAS_CV_MAIN_INTERFACE_HIDE_PLOT_LIST_PANEL_IN_CITY_SCREEN and kInterface.isCitySelection()):
			# Make sure any navigation arrows are hidden too (they are shown only in the draw methods).
			screen.hide("PlotListMinus")
			screen.hide("PlotListPlus")
			screen.hide(self.PLE.PLOT_LIST_MINUS_NAME)
			screen.hide(self.PLE.PLOT_LIST_PLUS_NAME)
			screen.hide(self.PLE.PLOT_LIST_UP_NAME)
			screen.hide(self.PLE.PLOT_LIST_DOWN_NAME)
			return 0

# BUG - draw methods
		#sDrawMethod = self.DRAW_METHODS[PleOpt.getDrawMethod()]
		# <advc.069> Replacing the above (getDrawMethod removed)
		if PleOpt.isPLE_Style():
			sDrawMethod = self.DRAW_METHOD_PLE
		elif PleOpt.isBUG_Style():
			sDrawMethod = self.DRAW_METHOD_BUG
		else:
			sDrawMethod = self.DRAW_METHOD_VAN
		# </advc.069>
		if sDrawMethod == self.DRAW_METHOD_PLE:
			self.PLE.updatePlotListButtons_PLE(screen)
			self.bPLECurrentlyShowing = True
		elif sDrawMethod == self.DRAW_METHOD_VAN:
			self.updatePlotListButtons_Orig(screen)
			self.bVanCurrentlyShowing = True
		else: # self.DRAW_METHOD_BUG
			self.updatePlotListButtons_BUG(screen)
			self.bBUGCurrentlyShowing = True
# BUG - draw methods
#		BugUtil.debug("updatePlotListButtons end - %s %s %s", self.bVanCurrentlyShowing, self.bPLECurrentlyShowing, self.bBUGCurrentlyShowing)
		return 0
#		if PleOpt.isPLE_Style():
#			self.updatePlotListButtons_PLE(screen)
#			self.bPLECurrentlyShowing = True
#		else:
#			self.updatePlotListButtons_Orig(screen)
#			self.bVanCurrentlyShowing = True
#		return 0

	def updatePlotListButtons_Hide(self, screen):
#		BugUtil.debug("updatePlotListButtons_Hide - %s %s %s", self.bVanCurrentlyShowing, self.bPLECurrentlyShowing, self.bBUGCurrentlyShowing)

		# hide all buttons
		if self.bPLECurrentlyShowing:
#			BugUtil.debug("updatePlotListButtons_Hide - hiding PLE")
			self.PLE.hidePlotListButtonPLEObjects(screen)
			self.PLE.hideUnitInfoPane()
			self.bPLECurrentlyShowing = False

		if self.bVanCurrentlyShowing:
#			BugUtil.debug("updatePlotListButtons_Hide - hiding Vanilla")
			self.hidePlotListButton_Orig(screen)
			self.bVanCurrentlyShowing = False

# BUG - BUG unit plot draw method - start
		if self.bBUGCurrentlyShowing:
#			BugUtil.debug("updatePlotListButtons_Hide - hiding BUG")
			self.hidePlotListButton_BUG(screen)
			self.bBUGCurrentlyShowing = False
# BUG - BUG unit plot draw method - end

	def updatePlotListButtons_Common(self, screen):
		xResolution = screen.getXResolution()
		yResolution = screen.getYResolution()
		# Capture these for looping over the plot's units
		self.PLE.UnitPlotList_BUGOptions()
		bHandled = False
		kInterface = CyInterface()
		if (kInterface.shouldDisplayUnitModel() and not CyEngine().isGlobeviewUp() and kInterface.getShowInterface() != InterfaceVisibility.INTERFACE_HIDE_ALL):
			if (kInterface.isCitySelection()):
				iOrders = kInterface.getNumOrdersQueued()
				for i in range(iOrders):
					if (not bHandled):
						eOrderNodeType = kInterface.getOrderNodeType(i)
						iOrderNodeData = kInterface.getOrderNodeData1(i)
						if (eOrderNodeType == OrderTypes.ORDER_TRAIN):
							self.addUnitGraphic("InterfaceUnitModel", iOrderNodeData, WidgetTypes.WIDGET_HELP_SELECTED, 0)
							bHandled = True
						elif (eOrderNodeType == OrderTypes.ORDER_CONSTRUCT):
							self.addBuildingGraphic("InterfaceUnitModel", iOrderNodeData, WidgetTypes.WIDGET_HELP_SELECTED, 0)
							bHandled = True
						elif (eOrderNodeType == OrderTypes.ORDER_CREATE):
							if(gc.getProjectInfo(iOrderNodeData).isSpaceship()):
								modelType = 0
								lRect = gRect("InterfaceUnitModel")
								screen.addSpaceShipWidgetGFC("InterfaceUnitModel", lRect.x(), lRect.y(), lRect.width(), lRect.height(), iOrderNodeData, modelType, WidgetTypes.WIDGET_HELP_SELECTED, 0, -1)
							else:
								screen.hide("InterfaceUnitModel")
							bHandled = True
						elif (eOrderNodeType == OrderTypes.ORDER_MAINTAIN):
							screen.hide("InterfaceUnitModel")
							bHandled = True
				if (not bHandled):
					screen.hide("InterfaceUnitModel")
					bHandled = True
				screen.moveToFront("SelectedCityText")
			elif (CyInterface().getHeadSelectedUnit()):
				self.addUnitGraphic("InterfaceUnitModel", CyInterface().getHeadSelectedUnit().getUnitType(), WidgetTypes.WIDGET_UNIT_MODEL, CyInterface().getHeadSelectedUnit().getUnitType())
#				screen.addSpecificUnitGraphicGFC("InterfaceUnitModel", CyInterface().getHeadSelectedUnit(), 175, yResolution - 138, 123, 132, WidgetTypes.WIDGET_UNIT_MODEL, CyInterface().getHeadSelectedUnit().getUnitType(), -1, -20, 30, 1, False)
				screen.moveToFront("SelectedUnitText")
			else:
				screen.hide("InterfaceUnitModel")
		else:
			screen.hide("InterfaceUnitModel")

	# hides all plot list objects
	def hidePlotListButton_Orig(self, screen):
#		BugUtil.debug("hidePlotListButton_Orig - %i", self.numPlotListButtons_Total())
		# hides all unit button objects
		for i in range(self.numPlotListButtons_Total()):
			szString = "PlotListButton" + str(i)
			screen.hide(szString)
			screen.hide(szString + "Icon")
			screen.hide(szString + "Health")
			screen.hide(szString + "MoveBar")
			screen.hide(szString + "PromoFrame")
			screen.hide(szString + "ActionIcon")
			screen.hide(szString + "Upgrade")

# BUG - draw method
	def hidePlotListButton_BUG(self, screen):
		#if self.DRAW_METHODS[PleOpt.getDrawMethod()] != self.DRAW_METHOD_BUG:
		# advc.069: Replacing the above (getDrawMethod removed)
		if PleOpt.isPLE_Style() or not PleOpt.isBUG_Style():
			self.BupPanel.clearUnits()
			self.BupPanel.Hide()
		return
		# hides all unit button objects
#		for i in range(self.iMaxPlotListIcons):
#			szString = "PlotListButton" + str(i)
#			screen.hide(szString)
#			screen.hide(szString + "Icon")
#			screen.hide(szString + "Health")
#			screen.hide(szString + "MoveBar")
#			screen.hide(szString + "PromoFrame")
#			screen.hide(szString + "ActionIcon")
#			screen.hide(szString + "Upgrade")
# BUG - draw method

	def updatePlotListButtons_Orig(self, screen):
# need to put in something similar to 	def displayUnitPlotListObjects(self, screen, pLoopUnit, nRow, nCol):
		kInterface = CyInterface()
		pPlot = kInterface.getSelectionPlot()
		for i in range(gc.getNumPromotionInfos()):
			szName = "PromotionButton" + str(i)
			screen.moveToFront(szName)
# BUG - Stack Promotions - start
		for i in range(gc.getNumPromotionInfos()):
			szName = "PromotionButtonCircle" + str(i)
			screen.moveToFront(szName)
		for i in range(gc.getNumPromotionInfos()):
			szName = "PromotionButtonCount" + str(i)
			screen.moveToFront(szName)
# BUG - Stack Promotions - end
		screen.hide("PlotListMinus")
		screen.hide("PlotListPlus")
		BugUtil.debug("updatePlotListButtons_Orig - column %i, offset %i", kInterface.getPlotListColumn(), kInterface.getPlotListOffset())
		if (pPlot and kInterface.getShowInterface() != InterfaceVisibility.INTERFACE_HIDE_ALL and (not CyEngine().isGlobeviewUp())):

			iVisibleUnits = kInterface.getNumVisibleUnits()
			iPlotListColumn = kInterface.getPlotListColumn()
			iCount = -iPlotListColumn
			bLeftArrow = False
			bRightArrow = False
			if (kInterface.isCityScreenUp()):
				iMaxRows = 1
				iSkipped = (self.numPlotListRows() - 1) * self.numPlotListButtonsPerRow()
				iCount += iSkipped
			else:
				iMaxRows = self.numPlotListRows()
				iCount += kInterface.getPlotListOffset()
				iSkipped = 0
			BugUtil.debug("updatePlotListButtons_Orig - iCount(%i), iSkipped(%i)", iCount, iSkipped)
			# <advc.069>
			bSimultaneousTurns = gc.getGame().isMPOption(MultiplayerOptionTypes.MPOPTION_SIMULTANEOUS_TURNS)
			bWoundedIndicator = PleOpt.isShowWoundedIndicator()
			bGGIndicator = PleOpt.isShowGreatGeneralIndicator()
			# </advc.069>
			kInterface.cacheInterfacePlotUnits(pPlot)
			for i in range(kInterface.getNumCachedInterfacePlotUnits()):
				pLoopUnit = kInterface.getCachedInterfacePlotUnit(i)
				if (pLoopUnit):
					# advc.004n: Allow going back down to a single row on the city screen
					#iCount == 0 and
					if (iPlotListColumn > 0):
						bLeftArrow = True
					#elif
					if ((iCount == (self.numPlotListRows() * self.numPlotListButtonsPerRow() - 1)) and ((iVisibleUnits - iCount - iPlotListColumn + iSkipped) > 1)):
						bRightArrow = True

					if ((iCount >= 0) and (iCount < self.numPlotListButtonsPerRow() * self.numPlotListRows())):
						# <advc.069>
						# </advc.069>
						bShowMoveOverlay = (bSimultaneousTurns or pLoopUnit.getOwner() == gc.getGame().getActivePlayer() or (bWoundedIndicator and pLoopUnit.isHurt()) or (bGGIndicator and pLoopUnit.getLeaderUnitType() >= 0))
						if ((pLoopUnit.getTeam() != gc.getGame().getActiveTeam()) or pLoopUnit.isWaiting()):
							szFileName = self.overlayFortifyPath
						elif (pLoopUnit.canMove()):
							if (pLoopUnit.hasMoved()):
								szFileName = self.overlayHasmovedPath
							else:
								szFileName = self.overlayMovePath
						else:
							szFileName = self.overlayNomovePath
						szString = "PlotListButton" + str(iCount)
						#screen.changeImageButton(szString, gc.getUnitInfo(pLoopUnit.getUnitType()).getButton())
						# advc.003l: Replacing the above
						screen.changeImageButton(szString, gc.getPlayer(pLoopUnit.getOwner()).getUnitButton(pLoopUnit.getUnitType()))
						if (pLoopUnit.getOwner() == gc.getGame().getActivePlayer()):
							bEnable = True
						else:
							bEnable = False
						screen.enable(szString, bEnable)

						if (pLoopUnit.IsSelected()):
							screen.setState(szString, True)
						else:
							screen.setState(szString, False)
						screen.show(szString)

						# place the health bar
						if (pLoopUnit.isFighting()):
							bShowHealth = False
						elif (pLoopUnit.getDomainType() == DomainTypes.DOMAIN_AIR):
							bShowHealth = pLoopUnit.canAirAttack()
						else:
							bShowHealth = pLoopUnit.canFight()

						if bShowHealth:
							szStringHealth = szString + "Health"
							screen.setBarPercentage(szStringHealth, InfoBarTypes.INFOBAR_STORED, float(pLoopUnit.currHitPoints()) / float(pLoopUnit.maxHitPoints()))
							if (pLoopUnit.getDamage() >= ((pLoopUnit.maxHitPoints() * 2) / 3)):
								screen.setStackedBarColors(szStringHealth, InfoBarTypes.INFOBAR_STORED, self.colorRed)
							elif (pLoopUnit.getDamage() >= (pLoopUnit.maxHitPoints() / 3)):
								screen.setStackedBarColors(szStringHealth, InfoBarTypes.INFOBAR_STORED, self.colorYellow)
							else:
								screen.setStackedBarColors(szStringHealth, InfoBarTypes.INFOBAR_STORED, self.colorGreen)
							screen.show(szStringHealth)

						# Adds the overlay first
						if bShowMoveOverlay: # advc.069
							szStringIcon = szString + "Icon"
							screen.changeDDSGFC(szStringIcon, szFileName)
							screen.show(szStringIcon)

						if bEnable:
							iPlotListFrameSize = self.plotListUnitButtonSize()  - self.plotListUnitFrameThickness()
							try: # advc.009b (work around crash upon reloading scripts)
								lPlotListBtn = gRect("PlotListButton" + str(iCount))
							except KeyError:
								return 0
							x = lPlotListBtn.x()
							y = lPlotListBtn.y()
							# advc.092
							self.PLE._displayUnitPlotList_Dot(screen, pLoopUnit, szString, iCount, x, y + (4 * iPlotListFrameSize) / 32)
							self.PLE._displayUnitPlotList_Promo(screen, pLoopUnit, szString)
							self.PLE._displayUnitPlotList_Upgrade(screen, pLoopUnit, szString, iCount, x, y)
							# advc.092
							self.PLE._displayUnitPlotList_Mission(screen, pLoopUnit, szString, iCount, x, y - (22 * iPlotListFrameSize) / 32, self.unitButtonOverlaySize())

					iCount += 1
#			BugUtil.debug("updatePlotListButtons_Orig - vis units(%i), buttons per row(%i), max rows(%i)", iVisibleUnits, self.numPlotListButtonsPerRow(), iMaxRows)
			if (iVisibleUnits > self.numPlotListButtonsPerRow() * iMaxRows):
#				BugUtil.debug("updatePlotListButtons_Orig - show arrows %s %s", bLeftArrow, bRightArrow)
				screen.enable("PlotListMinus", bLeftArrow)
				screen.show("PlotListMinus")

				screen.enable("PlotListPlus", bRightArrow)
				screen.show("PlotListPlus")

		return 0

# BUG - BUG unit plot draw method - start
	def updatePlotListButtons_BUG(self, screen):
# need to put in something similar to 	def displayUnitPlotListObjects(self, screen, pLoopUnit, nRow, nCol):
#		xResolution = screen.getXResolution()
#		yResolution = screen.getYResolution()
		kInterface = CyInterface()
		pPlot = kInterface.getSelectionPlot()
		# this moves the promotions for the unit shown in the bottom left so that they sit on top of the unit picture
		for i in range(gc.getNumPromotionInfos()):
			szName = "PromotionButton" + str(i)
			screen.moveToFront(szName)
# BUG - Stack Promotions - start
		for i in range(gc.getNumPromotionInfos()):
			szName = "PromotionButtonCircle" + str(i)
			screen.moveToFront(szName)
		for i in range(gc.getNumPromotionInfos()):
			szName = "PromotionButtonCount" + str(i)
			screen.moveToFront(szName)
# BUG - Stack Promotions - end
#		screen.hide("PlotListMinus")
#		screen.hide("PlotListPlus")
#		BugUtil.debug("updatePlotListButtons_BUG - A")
#		if (pPlot and CyInterface().getShowInterface() != InterfaceVisibility.INTERFACE_HIDE_ALL and CyEngine().isGlobeviewUp() == False):
		# skip this if we don't need to display any units
#		if not (pPlot and CyInterface().getShowInterface() != InterfaceVisibility.INTERFACE_HIDE_ALL and CyEngine().isGlobeviewUp() == False):
		if (not pPlot or kInterface.getShowInterface() == InterfaceVisibility.INTERFACE_HIDE_ALL or (CyEngine().isGlobeviewUp())):
			self.BupPanel.clearUnits()
			self.BupPanel.Hide()
			return 0
#		BugUtil.debug("updatePlotListButtons_BUG - B")
#		self.BupPanel.clearUnits()
		self.BupPanel.addPlot(pPlot.getX(), pPlot.getY())
		kInterface.cacheInterfacePlotUnits(pPlot)
		for i in range(kInterface.getNumCachedInterfacePlotUnits()):
			pLoopUnit = kInterface.getCachedInterfacePlotUnit(i)
			if (pLoopUnit):
				self.BupPanel.addUnit(pLoopUnit)
#		BugUtil.debug("updatePlotListButtons_BUG - C")
#		self.BupPanel.UpdateBUGOptions()
		timer = BugUtil.Timer("draw plot list")
		self.BupPanel.Draw()
		timer.log()
		# (advc: Deleted a big chunk of code commented out. Largely the same as in updatePlotListButtons_Orig I think.)
		return 0
# BUG - BUG unit plot draw method - end

	# <!-- custom: restored flag-widget update path (was disabled before our UI option) so hover/click flag behavior can be toggled by define-gated callers. (GPT-5.3-Codex) -->
	def updateFlag(self):
		eIFaceVis = CyInterface().getShowInterface()
		if (eIFaceVis != InterfaceVisibility.INTERFACE_HIDE_ALL and eIFaceVis != InterfaceVisibility.INTERFACE_MINIMAP_ONLY and eIFaceVis != InterfaceVisibility.INTERFACE_ADVANCED_START):
			lRect = gRect("CivilizationFlag")
			self.screen.addFlagWidgetGFC("CivilizationFlag", lRect.x(), lRect.y(), lRect.width(), lRect.height(), gc.getGame().getActivePlayer(), WidgetTypes.WIDGET_FLAG, gc.getGame().getActivePlayer(), -1)
			self.screen.show("CivilizationFlag")
		else:
			self.screen.hide("CivilizationFlag")

	# Will hide and show the selection buttons and their associated buttons
	def updateSelectionButtons(self):
		# advc: unused
		#global SELECTION_BUTTON_COLUMNS
		#global MAX_SELECTION_BUTTONS
		global g_pSelectedUnit
		# <advc.009b> Workaround to avoid exceptions while reloading scripts
		if not hasattr(self, "screen"):
			return # </advc.009b>
		screen = self.screen

		pHeadSelectedCity = CyInterface().getHeadSelectedCity()
		pHeadSelectedUnit = CyInterface().getHeadSelectedUnit()

		global g_NumEmphasizeInfos
		global g_NumCityTabTypes
		global g_NumHurryInfos
		global g_NumUnitClassInfos
		global g_NumBuildingClassInfos
		global g_NumProjectInfos
		global g_NumProcessInfos
		global g_NumActionInfos

		# Find out our resolution
		xResolution = screen.getXResolution()
		yResolution = screen.getYResolution()

		self._applyCityScreenSideLift(CyInterface().isCityScreenUp())
		self.updateBottomButtonList()
		screen.clearMultiList("BottomButtonList")
		screen.hide("BottomButtonList")

		# All of the hides...
		self.setMinimapButtonVisibility(False)

		screen.hideList(0)

		for i in range (g_NumEmphasizeInfos):
			szButtonID = "Emphasize" + str(i)
			screen.hide(szButtonID)

		# Hurry button show...
		for i in range(g_NumHurryInfos):
			szButtonID = "Hurry" + str(i)
			screen.hide(szButtonID)

		# Conscript Button Show
		screen.hide("Conscript")
		#screen.hide("Liberate")
		screen.hide("AutomateProduction")
		screen.hide("AutomateCitizens")

		# <advc.154>
		self.hideUnitCyclingButtons()
		# Moved up:
		bHeadSelectionChanged = (g_pSelectedUnit != pHeadSelectedUnit)
		g_pSelectedUnit = pHeadSelectedUnit
		# </advc.154>

		if not CyEngine().isGlobeviewUp() and pHeadSelectedCity:

			self.setMinimapButtonVisibility(True)

			if (pHeadSelectedCity.getOwner() == gc.getGame().getActivePlayer() or gc.getGame().isDebugMode()):
				# advc.154: Moved up a bit (needed at all?)
				g_pSelectedUnit = 0

				# Liberate button
				#szText = sasFontTagTiny + localText.getText("TXT_KEY_LIBERATE_CITY", ()) + SAS_FONT_TAG_CLOSE
				#screen.setButtonGFC("Liberate", szText, "", iBtnX, iBtnY, iBtnW, iBtnH, WidgetTypes.WIDGET_LIBERATE_CITY, -1, -1, ButtonStyles.BUTTON_STYLE_STANDARD)
				#screen.setStyle("Liberate", "Button_CityT1_Style")
				#screen.hide("Liberate")

				# <!-- custom: use custom DDS for draft button; placeholder paths. (GPT-5.2-Codex) -->
				self.addCheckBox("Conscript", "INTERFACE_CITY_SCREEN_DRAFT", "INTERFACE_CITY_SCREEN_DRAFT", ButtonStyles.BUTTON_STYLE_IMAGE, WidgetTypes.WIDGET_CONSCRIPT, -1, -1)
				screen.hide("Conscript")

				for i in range(2):
					szName = "Hurry" + str(i)
					self.setStyledButton(szName, "Button_CityC" + str(i+1) + "_Style", WidgetTypes.WIDGET_HURRY, i)
					screen.hide(szName)

				# <!-- custom: building list filter buttons using BUG art (paths cached in initState). (GPT-5.2-Codex, Claude Opus 4.5) -->
				self.addCheckBox(self.buildFilterBtnAll, self.buildFilterAllBuildingsOn, self.buildFilterAllBuildingsOff, ButtonStyles.BUTTON_STYLE_IMAGE, WidgetTypes.WIDGET_GENERAL, 0, -1)
				screen.hide(self.buildFilterBtnAll)
				self.addCheckBox(self.buildFilterBtnBuildings, self.buildFilterRegularBuildingsOn, self.buildFilterRegularBuildingsOff, ButtonStyles.BUTTON_STYLE_IMAGE, WidgetTypes.WIDGET_GENERAL, 1, -1)
				screen.hide(self.buildFilterBtnBuildings)
				self.addCheckBox(self.buildFilterBtnNational, self.buildFilterNatWonderOn, self.buildFilterNatWonderOff, ButtonStyles.BUTTON_STYLE_IMAGE, WidgetTypes.WIDGET_GENERAL, 2, -1)
				screen.hide(self.buildFilterBtnNational)
				self.addCheckBox(self.buildFilterBtnWorld, self.buildFilterWorldWonderOn, self.buildFilterWorldWonderOff, ButtonStyles.BUTTON_STYLE_IMAGE, WidgetTypes.WIDGET_GENERAL, 3, -1)
				screen.hide(self.buildFilterBtnWorld)

				self.addCheckBox("AutomateProduction", "", "", ButtonStyles.BUTTON_STYLE_STANDARD, WidgetTypes.WIDGET_AUTOMATE_PRODUCTION)
				screen.setStyle("AutomateProduction", "Button_CityC3_Style")
				self.addCheckBox("AutomateCitizens", "", "", ButtonStyles.BUTTON_STYLE_STANDARD, WidgetTypes.WIDGET_AUTOMATE_CITIZENS)
				screen.setStyle("AutomateCitizens", "Button_CityC4_Style")

				for i in range(6):
					szName = "Emphasize" + str(i)
					self.addCheckBox(szName, "", "", ButtonStyles.BUTTON_STYLE_LABEL, WidgetTypes.WIDGET_EMPHASIZE, i)
					screen.setStyle(szName, "Button_CityB" + str(i+1) + "_Style")
					screen.hide(szName)

				screen.setState("AutomateCitizens", pHeadSelectedCity.isCitizensAutomated())
				screen.setState("AutomateProduction", pHeadSelectedCity.isProductionAutomated())

				if CyInterface().isCityScreenUp():
					for i in range (g_NumEmphasizeInfos):
						szButtonID = "Emphasize" + str(i)
						screen.show(szButtonID)
						if (pHeadSelectedCity.AI_isEmphasize(i)):
							screen.setState(szButtonID, True)
						else:
							screen.setState(szButtonID, False)
				else:
					# <!-- custom: hide city config buttons in map view; hurry stays visible for convenience. (GPT-5.2-Codex) -->
					screen.hide("AutomateProduction")
					screen.hide("AutomateCitizens")
					for i in range (g_NumEmphasizeInfos):
						screen.hide("Emphasize" + str(i))

				# <!-- custom: Restore production chooser row-jump buttons next to the building-list filters; they select/jump to Units, Buildings, or Wonders rows, unlike the separate building-list filter buttons.
				# Our enlarged production chooser has room again, and the buttons make jumping between rows less tedious. (GPT-5.5) -->
				if CyInterface().isCityScreenUp():
					for i in range(g_NumCityTabTypes):
						screen.show("CityTab" + str(i))
				else:
					for i in range(g_NumCityTabTypes):
						screen.hide("CityTab" + str(i))

				# Hurry button show...
				for i in range(g_NumHurryInfos):
					szButtonID = "Hurry" + str(i)
					screen.show(szButtonID)
					screen.enable(szButtonID, pHeadSelectedCity.canHurry(i, False))

				# Conscript Button Show
				bCanConscript = pHeadSelectedCity.canConscript()
				screen.show("Conscript")
				screen.enable("Conscript", bCanConscript)
				screen.setState("Conscript", bCanConscript)

				# <!-- custom: show building filters only on city screen; hurry stays visible in map view for convenience. (GPT-5.2-Codex) -->
				if CyInterface().isCityScreenUp():
					iBuildAll = 0
					iBuildBuildings = 0
					iBuildNational = 0
					iBuildWorld = 0
					for iBuilding in range(gc.getNumBuildingInfos()):
						if pHeadSelectedCity.getNumBuilding(iBuilding) > 0:
							iBuildAll += 1
							eBuildingClass = gc.getBuildingInfo(iBuilding).getBuildingClassType()
							if self.isRegularBuildingClass(eBuildingClass):
								iBuildBuildings += 1
							elif self.isNationalWonderClass(eBuildingClass):
								iBuildNational += 1
							elif self.isWorldWonderClass(eBuildingClass):
								iBuildWorld += 1
					screen.show(self.buildFilterBtnAll)
					screen.show(self.buildFilterBtnBuildings)
					screen.show(self.buildFilterBtnNational)
					screen.show(self.buildFilterBtnWorld)
					screen.enable(self.buildFilterBtnAll, True)
					screen.enable(self.buildFilterBtnBuildings, True)
					screen.enable(self.buildFilterBtnNational, True)
					screen.enable(self.buildFilterBtnWorld, True)
					if iBuildAll > 0:
						screen.setState(self.buildFilterBtnAll, self.iBuildingFilter == 0)
					else:
						screen.setState(self.buildFilterBtnAll, False)
					if iBuildBuildings > 0:
						screen.setState(self.buildFilterBtnBuildings, self.iBuildingFilter == 1)
					else:
						screen.setState(self.buildFilterBtnBuildings, False)
					if iBuildNational > 0:
						screen.setState(self.buildFilterBtnNational, self.iBuildingFilter == 2)
					else:
						screen.setState(self.buildFilterBtnNational, False)
					if iBuildWorld > 0:
						screen.setState(self.buildFilterBtnWorld, self.iBuildingFilter == 3)
					else:
						screen.setState(self.buildFilterBtnWorld, False)
				else:
					screen.hide(self.buildFilterBtnAll)
					screen.hide(self.buildFilterBtnBuildings)
					screen.hide(self.buildFilterBtnNational)
					screen.hide(self.buildFilterBtnWorld)

				# Liberate Button Show
				#screen.show("Liberate")
				#if (-1 != pHeadSelectedCity.getLiberationPlayer()):
				#	screen.enable("Liberate", True)
				#else:
				#	screen.enable("Liberate", False)

				iCount = 0
				iRow = 0
				bFound = False

				# Units to construct
				for i in range (g_NumUnitClassInfos):
					eLoopUnit = gc.getCivilizationInfo(pHeadSelectedCity.getCivilizationType()).getCivilizationUnits(i)
					# <advc.001>
					if eLoopUnit == UnitTypes.NO_UNIT:
						continue # </advc.001>
					if (pHeadSelectedCity.canTrain(eLoopUnit, False, True)):
						szButton = gc.getPlayer(pHeadSelectedCity.getOwner()).getUnitButton(eLoopUnit)
						screen.appendMultiListButton("BottomButtonList", szButton, iRow, WidgetTypes.WIDGET_TRAIN, i, -1, False)
						screen.show("BottomButtonList")
						if (not pHeadSelectedCity.canTrain(eLoopUnit, False, False)):
							screen.disableMultiListButton("BottomButtonList", iRow, iCount, szButton)
						iCount += 1
						bFound = True

				iCount = 0
				if (bFound):
					iRow = iRow + 1
				bFound = False

				# Buildings to construct
				for i in range (g_NumBuildingClassInfos):
					if (not isLimitedWonderClass(i)):
						eLoopBuilding = gc.getCivilizationInfo(pHeadSelectedCity.getCivilizationType()).getCivilizationBuildings(i)
						# <advc.001>
						if eLoopBuilding == BuildingTypes.NO_BUILDING:
							continue # </advc.001>
						if (pHeadSelectedCity.canConstruct(eLoopBuilding, False, True, False)):
							screen.appendMultiListButton("BottomButtonList", gc.getBuildingInfo(eLoopBuilding).getButton(), iRow, WidgetTypes.WIDGET_CONSTRUCT, i, -1, False)
							screen.show("BottomButtonList")
							if (not pHeadSelectedCity.canConstruct(eLoopBuilding, False, False, False)):
								screen.disableMultiListButton("BottomButtonList", iRow, iCount, gc.getBuildingInfo(eLoopBuilding).getButton())
							iCount += 1
							bFound = True

				iCount = 0
				if (bFound):
					iRow = iRow + 1
				bFound = False

				# Wonders to construct
				i = 0
				for i in range(g_NumBuildingClassInfos):
					if (isLimitedWonderClass(i)):
						eLoopBuilding = gc.getCivilizationInfo(pHeadSelectedCity.getCivilizationType()).getCivilizationBuildings(i)
						# <advc.001>
						if eLoopBuilding == BuildingTypes.NO_BUILDING:
							continue # </advc.001>
						if (pHeadSelectedCity.canConstruct(eLoopBuilding, False, True, False)):
							screen.appendMultiListButton("BottomButtonList", gc.getBuildingInfo(eLoopBuilding).getButton(), iRow, WidgetTypes.WIDGET_CONSTRUCT, i, -1, False)
							screen.show("BottomButtonList")
							if (not pHeadSelectedCity.canConstruct(eLoopBuilding, False, False, False)):
								screen.disableMultiListButton("BottomButtonList", iRow, iCount, gc.getBuildingInfo(eLoopBuilding).getButton())
							iCount += 1
							bFound = True

				iCount = 0
				if (bFound):
					iRow = iRow + 1
				bFound = False

				# Projects
				i = 0
				for i in range(g_NumProjectInfos):
					if (pHeadSelectedCity.canCreate(i, False, True)):
						screen.appendMultiListButton("BottomButtonList", gc.getProjectInfo(i).getButton(), iRow, WidgetTypes.WIDGET_CREATE, i, -1, False)
						screen.show("BottomButtonList")
						if (not pHeadSelectedCity.canCreate(i, False, False)):
							screen.disableMultiListButton("BottomButtonList", iRow, iCount, gc.getProjectInfo(i).getButton())
						iCount += 1
						bFound = True

				# Processes
				i = 0
				for i in range(g_NumProcessInfos):
					if (pHeadSelectedCity.canMaintain(i, False)):
						screen.appendMultiListButton("BottomButtonList", gc.getProcessInfo(i).getButton(), iRow, WidgetTypes.WIDGET_MAINTAIN, i, -1, False)
						screen.show("BottomButtonList")
						iCount += 1
						bFound = True

				# <!-- custom: fix production chooser bar auto-scrolling when we click on one of the lower rows (distracting and annoying and not necessary; the player can scroll if they want rather).
				# Fix with the help of chatgpt 5.2 thanks -->
				# 3) Replace the selectMultiList(...getCityTabSelectionRow()) in the city-screen block
				# <!-- custom: note: after applying all the 3 steps of this fix, when clicking on lower rows while we are in the top rows sections, we successfully prevent auto-scrolling down as we want, however it seems that when we click on the upper rows while we are in the bottom rows sections, then we auto-scroll back to top.
				# It may not be ideal; or, maybe this is a nice side effect we can keep, as bottom rows mostly only have wonders and processes, and we don't want to build too many of them anyway, so kept as such -->
				# screen.selectMultiList("BottomButtonList", CyInterface().getCityTabSelectionRow())
				if self.iBarExtraRows > 0:
					if self.iCityBuildBarPinnedRow is None:
						self.iCityBuildBarPinnedRow = CyInterface().getCityTabSelectionRow()

					# Clamp to the last non-empty row we actually filled
					if bFound:
						iLastRowUsed = iRow
					else:
						iLastRowUsed = max(0, iRow - 1)
					if self.iCityBuildBarPinnedRow < 0:
						self.iCityBuildBarPinnedRow = 0
					elif self.iCityBuildBarPinnedRow > iLastRowUsed:
						self.iCityBuildBarPinnedRow = iLastRowUsed

					screen.selectMultiList("BottomButtonList", self.iCityBuildBarPinnedRow)
				else:
					screen.selectMultiList("BottomButtonList", CyInterface().getCityTabSelectionRow())

		elif (not CyEngine().isGlobeviewUp() and CyInterface().getShowInterface() != InterfaceVisibility.INTERFACE_HIDE_ALL and CyInterface().getShowInterface() != InterfaceVisibility.INTERFACE_MINIMAP_ONLY):
			self.setMinimapButtonVisibility(True)
			if CyInterface().getInterfaceMode() == InterfaceModeTypes.INTERFACEMODE_SELECTION:
				# advc.154: pHeadSelectedUnit moved from above. (Handling of g_pSelectedUnit moved up.)
				if (pHeadSelectedUnit and pHeadSelectedUnit.getOwner() == gc.getGame().getActivePlayer() and bHeadSelectionChanged):
					# <advc.004k>
					bGroupButtonsDone = False
					# (Safety margin - seems that MultiListControl can't use its entire width.)
					iMaxCols = int((gRect("BottomButtonList").width() -10) / self.bottomListBtnSize())
					# Can display any number of them, but we don't want a scrollbar.
					iMaxRows = int(gRect("BottomButtonList").height() / self.bottomListBtnSize())
					iRow0Count = 0
					iRow1Count = 0
					#iCount = 0 # </advc.004k>
					actions = CyInterface().getActionsToShow()
					for i in actions:
						# <advc.004k>
						# (I don't think these options are needed after all)
						#
						# if (gc.getActionInfo(i).getMissionType() == MissionTypes.MISSION_SEAPATROL and not MainOpt.isShowSeaPatrol()):
						# 	continue
						# if (gc.getActionInfo(i).getAutomateType() == AutomateTypes.AUTOMATE_EXPLORE and not MainOpt.isShowAutoExplore()):
						# 	continue
						#
						# Show grouping commands before promotions, upgrades
						if (not bGroupButtonsDone and (gc.getActionInfo(i).getCommandType() == CommandTypes.COMMAND_PROMOTION or gc.getActionInfo(i).getCommandType() == CommandTypes.COMMAND_UPGRADE)):
							bGroupButtonsDone = True
							iRow0Count += self.appendGroupBottomButtons()
						# Put promotions, upgrades on a separate row - if we haven't exceeded a full row already and can fit all promos and upgrades in the 2nd row.
						# When space is very limited, then there's a danger of about half of the units using a single (wrapped) row and half using separate rows, which is awkward.
						if (bGroupButtonsDone and iMaxRows >= 2 and iRow0Count <= iMaxCols and len(actions) - iRow0Count <= iMaxCols and iMaxCols >= 8):
							iRow = 1
							iCount = iRow1Count
						else:
							iRow = 0
							iCount = iRow0Count
							# And changed listId args below from 0 to iRow
						# </advc.004k>
						screen.appendMultiListButton("BottomButtonList", gc.getActionInfo(i).getButton(), iRow, WidgetTypes.WIDGET_ACTION, i, -1, False)
						screen.show("BottomButtonList")
						if not CyInterface().canHandleAction(i, False):
							screen.disableMultiListButton("BottomButtonList", iRow, iCount, gc.getActionInfo(i).getButton())
						#or gc.getActionInfo(i).getCommandType() == CommandTypes.COMMAND_PROMOTION
						if (pHeadSelectedUnit.isActionRecommended(i)):
							screen.enableMultiListPulse("BottomButtonList", True, iRow, iCount)
						else:
							screen.enableMultiListPulse("BottomButtonList", False, iRow, iCount)
						#iCount += 1
						# <advc.004k>
						if iRow == 0:
							iRow0Count += 1
						else:
							iRow1Count += 1
					if not bGroupButtonsDone: # If no promotions, upgrades.
						# Moved into new method
						iRow0Count += self.appendGroupBottomButtons()
						screen.show("BottomButtonList") # </advc.004k>
				# <advc.154>
				pUnit = None
				if bHeadSelectionChanged:
					pUnit = pHeadSelectedUnit
				self.updateUnitCyclingButtons(pUnit) # </advc.154>
		elif (CyInterface().getShowInterface() != InterfaceVisibility.INTERFACE_HIDE_ALL and CyInterface().getShowInterface() != InterfaceVisibility.INTERFACE_MINIMAP_ONLY):
			self.setMinimapButtonVisibility(True)

		return 0
	# advc.004k: Moved from updateSelectionButtons above
	def appendGroupBottomButtons(self):
		iAppended = 0
		if CyInterface().canCreateGroup():
			self.screen.appendMultiListButton("BottomButtonList", self.buttonsCreategroupPath, 0, WidgetTypes.WIDGET_CREATE_GROUP, -1, -1, False)
			iAppended += 1
		if CyInterface().canDeleteGroup():
			self.screen.appendMultiListButton("BottomButtonList", self.buttonsSplitgroupPath, 0, WidgetTypes.WIDGET_DELETE_GROUP, -1, -1, False)
			iAppended += 1
		return iAppended

	# <advc.154>
	def hideUnitCyclingButtons(self):
		self.hideUnitCycleButtonGFC("UnitCycle")
		self.hideUnitCycleButtonGFC("WorkerCycle")
		self.hideUnitCycleButtonGFC("Unselect")

	def updateUnitCyclingButtons(self, pHeadSelectedUnit):
		if MainOpt.isUnitCyclingButtonsDisabled():
			return
		# <!-- custom: position UnitCycle button to left of city arrows in the EndTurnButtonPanel (Claude code Opus 4.5) -->
		lButtons = RowLayout(gRect("UnitCycleButtonPos"), 0, 0, 2, HSPACE(-4), BTNSZ(32))
		gSetRectangle("UnitCycleButtons", lButtons)
		pNextUnit = gc.getGame().getNextUnitInCycle(True, False)
		if pNextUnit:
			# The button looks weird when the HUD is partly hidden and no unit selected. Can be a nuisance when taking screenshots.
			if (not pHeadSelectedUnit and CyInterface().getShowInterface() == InterfaceVisibility.INTERFACE_HIDE):
				return
			if pNextUnit.hasMoved():
				szOverlay = "OVERLAY_HASMOVED"
			else:
				szOverlay = "OVERLAY_MOVE"
			self.showUnitCycleButtonGFC("UnitCycle", pNextUnit, lButtons.next(), False, False, ArtFileMgr.getInterfaceArtInfo(szOverlay).getPath())
			if (not MainOpt.isSingleUnitCyclingButton() and not pNextUnit.isWorker()):
				pNextWorker = gc.getGame().getNextUnitInCycle(True, True)
				if pNextWorker and pNextWorker.getID() != pNextUnit.getID():
					if pNextWorker.hasMoved():
						szOverlay = "OVERLAY_HASMOVED"
					else:
						szOverlay = "OVERLAY_MOVE"
					self.showUnitCycleButtonGFC("WorkerCycle", pNextWorker, lButtons.next(), True, False, ArtFileMgr.getInterfaceArtInfo(szOverlay).getPath())
		# If selected unit is not expecting orders, show unselect button (advc.088).
		elif (pHeadSelectedUnit and (pHeadSelectedUnit.isWaiting() or not pHeadSelectedUnit.canMove())):
			if pHeadSelectedUnit.canMove():
				szOverlay = "OVERLAY_FORTIFY"
			else:
				szOverlay = "OVERLAY_NOMOVE"
			self.showUnitCycleButtonGFC("Unselect", pHeadSelectedUnit, lButtons.next(), False, True, ArtFileMgr.getInterfaceArtInfo(szOverlay).getPath())

	def showUnitCycleButtonGFC(self, szName, kUnit, lRect, bWorkers, bUnselect, szOverlayPath):
		iWidgetData2 = -1
		if not bUnselect:
			iWidgetData2 = kUnit.getID()
		screen = self.screen
		screen.setButtonGFC(szName + "Button", "", gc.getPlayer(kUnit.getOwner()).getUnitButton(kUnit.getUnitType()), lRect.x(), lRect.y(), lRect.width(), lRect.height(), WidgetTypes.WIDGET_CYCLE_UNIT, bWorkers, iWidgetData2, ButtonStyles.BUTTON_STYLE_IMAGE)
		iOverlaySize = self.unitButtonOverlaySize()
		screen.addDDSGFCAt(szName + "Overlay", szName + "Button", szOverlayPath, 0, 0, iOverlaySize, iOverlaySize, WidgetTypes.WIDGET_CYCLE_UNIT, bWorkers, iWidgetData2, False)
		screen.show(szName + "Button")
		screen.show(szName + "Overlay")

	def hideUnitCycleButtonGFC(self, szName):
		self.screen.hide(szName + "Button")
		self.screen.hide(szName + "Overlay") # </advc.154>

	# Will update the research buttons
	def updateResearchButtons(self):
		screen = self.screen
		for iTech in range(gc.getNumTechInfos()):
			screen.hide("ResearchButton" + str(iTech))
		#screen.hide("InterfaceOrnamentLeftLow")
		#screen.hide("InterfaceOrnamentRightLow")
		for iReligion in range(gc.getNumReligionInfos()):
			screen.hide("ReligionButton" + str(iReligion))
		if (not CyInterface().shouldShowResearchButtons() or CyInterface().getShowInterface() != InterfaceVisibility.INTERFACE_SHOW):
			return
		# advc.092: Collect the button ids before moving them
		aResearchBtns = []
		for iTech in range(gc.getNumTechInfos()):
			if not gc.getActivePlayer().canResearch(iTech, False):
				continue
			szName = "ResearchButton" + str(iTech)
			for iReligion in range(gc.getNumReligionInfos()):
				if gc.getReligionInfo(iReligion).getTechPrereq() == iTech:
					if not gc.getGame().isReligionSlotTaken(iReligion):
						szName = "ReligionButton" + str(iReligion)
						break
			aResearchBtns.append(szName)
		# advc.092: Based on code from deleted setResearchButtonPosition method
# BUG - Bars on single line for higher resolution screens - start
		if self.isShowTopBarsOnSingleLine():
			szResearchBar = "OneLineResearchBar"
			iHMargin = HSPACE(0)
		else:
			szResearchBar = "TwoLineResearchBar"
			# I think this is as in BtS; not sure if it's a good idea.
			# I guess using some extra space for research buttons makes sense
			# for low resolutions.)
			iHMargin = HSPACE(-3)
		xCoord = gRect(szResearchBar).x() + iHMargin
		yCoord = max(0, gRect(szResearchBar).y() + (gRect(szResearchBar).height() - self.iTechBtnSize) / 2)
		iColW = self.iTechBtnSize + HSPACE(2)
		iRowH = self.iTechBtnSize + VSPACE(2)
		iAvailW = gRect(szResearchBar).xRight() - iHMargin - xCoord
		iBtnPerRow = iAvailW / iColW # was 15 flat in BUG/BtS
		xCoord += (iAvailW - min(iBtnPerRow, len(aResearchBtns)) * iColW) / 2 # center-align
		iMaxCount = iBtnPerRow # was 20 flat in BtS
		if (self.isShowTopBarsOnSingleLine() or (not self.isShowGGProgressBar() and not self.isShowGPProgressBar())):
			iMaxCount += iBtnPerRow
		iCount = 0
		for szButtonID in aResearchBtns:
			screen.moveItem(szButtonID, xCoord + iColW * (iCount % iBtnPerRow), yCoord + iRowH * (iCount / iBtnPerRow), -0.3)
			screen.show(szButtonID)
			iCount += 1
			if iCount >= iMaxCount:
				break
# BUG - Bars on single line for higher resolution screens - end

# BUG - city specialist - start
	def updateCitizenButtons_hide(self):
		screen = self.screen

		for i in range(MAX_CITIZEN_BUTTONS):
			screen.hide("FreeSpecialist" + str(i))
			screen.hide("AngryCitizen" + str(i))
			screen.hide("AngryCitizenChevron" + str(i))
			screen.hide("Stacker_FreeSpecialist" + str(i))
			screen.hide("Stacker_AngryCitizen" + str(i))

		for iSpecialist in self.visibleSpecialists:
			szName = "IncreaseSpecialist" + str(iSpecialist)
			screen.hide(szName)
			szName = "DecreaseSpecialist" + str(iSpecialist)
			screen.hide(szName)
			szName = "SpecialistDisabledButton" + str(iSpecialist)
			screen.hide(szName)
			for j in range(MAX_CITIZEN_BUTTONS):
				szIndex = str(iSpecialist * 100 + j)
				screen.hide("CitizenButton" + szIndex)
				screen.hide("CitizenButtonHighlight" + szIndex)
				screen.hide("CitizenChevron" + szIndex)

				screen.hide("IncrCitizenButton" + szIndex)
				screen.hide("IncrCitizenBanner" + szIndex)
				screen.hide("DecrCitizenButton" + szIndex)
				screen.hide("CitizenButtonHighlight" + szIndex)

		screen.hide("SpecialistLabelBackground")
		screen.hide("SpecialistLabel")

		for i in range(g_iMaxEverFreeSpecialists):
			screen.hide("FreeSpecialist" + str(i))
			screen.hide("Stacker_FreeSpecialist" + str(i))
		for i in range(g_iMaxEverAngry):
			screen.hide("AngryCitizen" + str(i))
			screen.hide("Stacker_AngryCitizen" + str(i))

		for iSpecialist in self.visibleSpecialists:
			for j in range(g_iMaxEverAdjustableSpecialists):
				szIndex = str(iSpecialist * 100 + j)
				screen.hide("IncrCitizenBanner" + szIndex)
				screen.hide("IncrCitizenButton" + szIndex)
				screen.hide("DecrCitizenButton" + szIndex)
# BUG - city specialist - end

	# advc.092: Refactored this and updateCitizenButtons_Chevron to reduce redundancy
	def updateCitizenButtons(self, pHeadSelectedCity):
		screen = self.screen

		for i in range(min(MAX_CITIZEN_BUTTONS, pHeadSelectedCity.angryPopulation(0))):
			szName = "AngryCitizen" + str(i)
			screen.show(szName)

		self.updateFreeSpecialistButtons(pHeadSelectedCity)

		for iSpecialist in self.visibleSpecialists:

			self.updateAdjustableSpecialistButtons(pHeadSelectedCity, iSpecialist)

			bHandled = False
			for j in range(min(MAX_CITIZEN_BUTTONS, pHeadSelectedCity.getSpecialistCount(iSpecialist))):
				bHandled = True
				szIndex = str(iSpecialist * 100 + j)
				szName = "CitizenButton" + szIndex
				# advc: Redundant I think
				#
				# self.addCheckBox(szName, gc.getSpecialistInfo(iSpecialist).getTexture(), "", ButtonStyles.BUTTON_STYLE_LABEL, WidgetTypes.WIDGET_CITIZEN, iSpecialist, j)
				#
				screen.show(szName)
				szName = "CitizenButtonHighlight" + szIndex
				self.addDDS(szName, "BUTTON_HILITE_SQUARE", WidgetTypes.WIDGET_CITIZEN, iSpecialist, j)
				if pHeadSelectedCity.getForceSpecialistCount(iSpecialist) > j:
					screen.show(szName)
				else:
					screen.hide(szName)
			if not bHandled:
				szName = "SpecialistDisabledButton" + str(iSpecialist)
				screen.show(szName)

	def updateFreeSpecialistButtons(self, pHeadSelectedCity):
		screen = self.screen
		iFreeSpecialistCount = 0
		for iSpecialist in range(gc.getNumSpecialistInfos()):
			iFreeSpecialistCount += pHeadSelectedCity.getFreeSpecialistCount(iSpecialist)

		iCount = 0
		lFirstFreeSpecialist = gRect("FirstFreeSpecialist")
		if iFreeSpecialistCount > MAX_CITIZEN_BUTTONS:
			for iSpecialist in range(gc.getNumSpecialistInfos()):
				if pHeadSelectedCity.getFreeSpecialistCount(iSpecialist) > 0:
					if iCount < MAX_CITIZEN_BUTTONS:
						szName = "FreeSpecialist" + str(iCount)
						screen.setImageButton(szName, gc.getSpecialistInfo(iSpecialist).getTexture(), lFirstFreeSpecialist.x() -(lFirstFreeSpecialist.size() + HSPACE(2)) * iCount, lFirstFreeSpecialist.y(), lFirstFreeSpecialist.size(), lFirstFreeSpecialist.size(), WidgetTypes.WIDGET_FREE_CITIZEN, iSpecialist, 1)
						screen.show(szName)
					iCount += 1
		else:
			for iSpecialist in range(gc.getNumSpecialistInfos()):
				for j in range(pHeadSelectedCity.getFreeSpecialistCount(iSpecialist)):
					if iCount < MAX_CITIZEN_BUTTONS:
						szName = "FreeSpecialist" + str(iCount)
						screen.setImageButton(szName, gc.getSpecialistInfo(iSpecialist).getTexture(), lFirstFreeSpecialist.x() - (lFirstFreeSpecialist.size() + HSPACE(2)) * iCount, lFirstFreeSpecialist.y(), lFirstFreeSpecialist.size(), lFirstFreeSpecialist.size(), WidgetTypes.WIDGET_FREE_CITIZEN, iSpecialist, -1)
						screen.show(szName)
					iCount += 1

	def updateAdjustableSpecialistButtons(self, pHeadSelectedCity, iSpecialist):
		if (pHeadSelectedCity.getOwner() != gc.getGame().getActivePlayer() and not gc.getGame().isDebugMode()):
			return
		if pHeadSelectedCity.isCitizensAutomated():
			iSpecialistCount = max(pHeadSelectedCity.getSpecialistCount(iSpecialist), pHeadSelectedCity.getForceSpecialistCount(iSpecialist))
		else:
			iSpecialistCount = pHeadSelectedCity.getSpecialistCount(iSpecialist)
		screen = self.screen
		if (pHeadSelectedCity.isSpecialistValid(iSpecialist, 1) and (pHeadSelectedCity.isCitizensAutomated() or iSpecialistCount < (pHeadSelectedCity.getPopulation() + pHeadSelectedCity.totalFreeSpecialists()))):
			szName = "IncreaseSpecialist" + str(iSpecialist)
			screen.show(szName)
			szName = "SpecialistDisabledButton" + str(iSpecialist)
			screen.show(szName)
		if iSpecialistCount > 0:
			szName = "SpecialistDisabledButton" + str(iSpecialist)
			screen.hide(szName)
			szName = "DecreaseSpecialist" + str(iSpecialist)
			screen.show(szName)

	# BUG - city specialist - start  (advc.092: refactored)
	def updateCitizenButtons_Chevron(self, pHeadSelectedCity):
		screen = self.screen

		iCount = pHeadSelectedCity.angryPopulation(0)
		j = 0
		while iCount > 0:
			screen.show("AngryCitizen" + str(j))
			szArtKey = None
			if iCount >= 20:
				szArtKey = "OVERLAY_CHEVRON20"
				iCount -= 20
			elif iCount >= 10:
				szArtKey = "OVERLAY_CHEVRON10"
				iCount -= 10
			elif iCount >= 5:
				szArtKey = "OVERLAY_CHEVRON5"
				iCount -= 5
			else:
				iCount -= 1
			if szArtKey:
				szName = "AngryCitizenChevron" + str(j)
				# <!-- custom: This overlay belongs to the underlying angry citizen, not to SpecialistTypes j; matching its widget prevents specialist help/Pedia actions over the chevron. See KI#395. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				self.addDDS(szName, szArtKey, WidgetTypes.WIDGET_ANGRY_CITIZEN)
				screen.show(szName)
			j += 1

		self.updateFreeSpecialistButtons(pHeadSelectedCity)

		for iSpecialist in self.visibleSpecialists:

			self.updateAdjustableSpecialistButtons(pHeadSelectedCity, iSpecialist)

			iCount = pHeadSelectedCity.getSpecialistCount(iSpecialist)
			if iCount <= 0:
				szName = "SpecialistDisabledButton" + str(iSpecialist)
				screen.show(szName)
				continue
			j = 0
			while iCount > 0:
				szBtnIndex = str(iSpecialist * 100 + j)
				screen.show("CitizenButton" + szBtnIndex)
				szArtKey = None
				if iCount >= 20:
					szArtKey = "OVERLAY_CHEVRON20"
					iCount -= 20
				elif iCount >= 10:
					szArtKey = "OVERLAY_CHEVRON10"
					iCount -= 10
				elif iCount >= 5:
					szArtKey = "OVERLAY_CHEVRON5"
					iCount -= 5
				else:
					iCount -= 1
				if szArtKey:
					szName = "CitizenChevron" + szBtnIndex
					self.addDDS(szName, szArtKey, WidgetTypes.WIDGET_CITIZEN, iSpecialist, 0)
					screen.show(szName)
				j += 1
# BUG - city specialist - end

# BUG - city specialist - start
	def updateCitizenButtons_Stacker(self, pHeadSelectedCity):
		global g_iMaxEverFreeSpecialists
		global g_iMaxEverAdjustableSpecialists
		global g_iMaxEverAngry
		screen = self.screen
		iBtnSize = BTNSZ(30)
		# advc.092: Nice that this can be quite compact, but looks rather strange on higher resolutions.
		if gRect("Top").height() > 900:
			iVSpace = 6
		else:
			iVSpace = 4
		iVSpace = VSPACE(iVSpace)
		SPECIALIST_ROW_HEIGHT = iBtnSize + iVSpace
		MAX_SPECIALIST_BUTTON_SPACING = iBtnSize
		gSetRect("StackerCitizenButtons", "CityRightPanelContents", RectLayout.CENTER, 0, -HSPACE(4), -gRect("GreatPeopleBar").height())
		# For some reason, a smaller width is used for the iStackWidth calculations.
		gSetRect("StackerRows", "StackerCitizenButtons", RectLayout.CENTER, 0, -HSPACE(6), RectLayout.MAX)

		iFreeSpecialists = 0
		for iSpecialist in range(gc.getNumSpecialistInfos()):
			iFreeSpecialists += pHeadSelectedCity.getFreeSpecialistCount(iSpecialist)
		if iFreeSpecialists > 0:
			iStackWidth = min(gRect("StackerRows").width() / iFreeSpecialists, MAX_SPECIALIST_BUTTON_SPACING)
			lFreeSpecialistButtons = RowLayout(gRect("StackerCitizenButtons"), RectLayout.RIGHT, RectLayout.BOTTOM, iFreeSpecialists, iStackWidth - iBtnSize, iBtnSize)
			# advc.001: Don't re-use the BtS name because that leads to problems when toggling the stacker option.
			gSetRectangle("Stacker_FreeSpecialistButtons", lFreeSpecialistButtons)
			g_iMaxEverFreeSpecialists = max(g_iMaxEverFreeSpecialists, iFreeSpecialists)
		iCount = 0
		for iSpecialist in range(gc.getNumSpecialistInfos() - 1, -1, -1):
			for j in range(pHeadSelectedCity.getFreeSpecialistCount(iSpecialist)):
				szName = "Stacker_FreeSpecialist" + str(iCount)
				gSetRectangle(szName, lFreeSpecialistButtons.next())
				self.setImageButton(szName, gc.getSpecialistInfo(iSpecialist).getTexture(), WidgetTypes.WIDGET_FREE_CITIZEN, iSpecialist, 1)
				screen.show(szName)
				iCount += 1

		iAngry = pHeadSelectedCity.angryPopulation(0)
		if iAngry > 0:
			iStackWidth = min(gRect("StackerRows").width() / iAngry, MAX_SPECIALIST_BUTTON_SPACING)
			lAngryCitizenButtons = RowLayout(gRect("StackerCitizenButtons"), RectLayout.RIGHT, RectLayout.BOTTOM, iAngry, iStackWidth - iBtnSize, iBtnSize)
			lAngryCitizenButtons.move(0, -SPECIALIST_ROW_HEIGHT)
			gSetRectangle("Stacker_AngryCitizenButtons", lAngryCitizenButtons)
			g_iMaxEverAngry = max(g_iMaxEverAngry, iAngry)
		for i in range(iAngry):
			szName = "Stacker_AngryCitizen" + str(i)
			gSetRectangle(szName, lAngryCitizenButtons.next())
			self.setImageButton(szName, "INTERFACE_ANGRYCITIZEN_TEXTURE", WidgetTypes.WIDGET_ANGRY_CITIZEN)
			screen.show(szName)

		iVisibleSpecialists = 0
		if self.iDEFAULT_SPECIALIST is None:
			self.iDEFAULT_SPECIALIST = gc.getDefineINT("DEFAULT_SPECIALIST")
		iDefaultSpecialist = self.iDEFAULT_SPECIALIST
		for iSpecialist in range(gc.getNumSpecialistInfos()):
			SPECIALIST_ROWS = 3
			if iVisibleSpecialists > SPECIALIST_ROWS:
				iXShift = gRect("StackerRows").width() / 2 + HSPACE(5)
				iYShift = (iVisibleSpecialists % SPECIALIST_ROWS) + 1
			else:
				iXShift = 0
				iYShift = iVisibleSpecialists
			iYShift += 2
			if gc.getSpecialistInfo(iSpecialist).isVisible():
				iVisibleSpecialists += 1
			if (gc.getPlayer(pHeadSelectedCity.getOwner()).isSpecialistValid(iSpecialist) or iSpecialist == iDefaultSpecialist):
				iCount = (pHeadSelectedCity.getPopulation() - pHeadSelectedCity.angryPopulation(0) + pHeadSelectedCity.totalFreeSpecialists())
			else:
				iCount = pHeadSelectedCity.getMaxSpecialistCount(iSpecialist)
			if iCount > 0:
				iRowLength = gRect("StackerRows").width()
				if iSpecialist != iDefaultSpecialist:
					iRowLength /= 2
				iStackWidth = min(iRowLength / iCount, MAX_SPECIALIST_BUTTON_SPACING)
				lSpecialistButtons = RowLayout(gRect("StackerCitizenButtons"), RectLayout.RIGHT, RectLayout.BOTTOM, iCount, iStackWidth - iBtnSize, iBtnSize)
				lSpecialistButtons.move(-iXShift, -(SPECIALIST_ROW_HEIGHT * iYShift))
				gSetRectangle("Stacker_SpecialistButtons", lSpecialistButtons)
				g_iMaxEverAdjustableSpecialists = max(g_iMaxEverAdjustableSpecialists, iCount)
			for k in range(iCount):
				szIndex = str(iSpecialist * 100 + k)
				#SPECIALIST_AREA_MARGIN = 45 #gRect("StackerCitizenButtons") 4(extra margin)+9(original margin)+2(?)+iBtnSize
				#iNonAdjustablesOffset = (3 * iSmallVSpace + 2 * iCitizenBtnSize +
				#	gRect("GreatPeopleBar").height()#27  164  | 91-34=57  57-34=23  23-34=-11
				#iYOffset = 282
				lRect = lSpecialistButtons.next()
				if k >= pHeadSelectedCity.getSpecialistCount(iSpecialist):
					szName = "IncrCitizenBanner" + szIndex
					gSetRectangle(szName, lRect)
					self.addCheckBox(szName, gc.getSpecialistInfo(iSpecialist).getTexture(), "", ButtonStyles.BUTTON_STYLE_LABEL, WidgetTypes.WIDGET_CHANGE_SPECIALIST, iSpecialist, 1)
					screen.enable(szName, False)
					screen.show(szName)
					szName = "IncrCitizenButton" + szIndex
					gSetRectangle(szName, lRect)
					self.addCheckBox(szName, "", "", ButtonStyles.BUTTON_STYLE_LABEL, WidgetTypes.WIDGET_CHANGE_SPECIALIST, iSpecialist, 1)
					screen.show(szName)
				else:
					szName = "DecrCitizenButton" + szIndex
					gSetRectangle(szName, lRect)
					self.addCheckBox(szName, gc.getSpecialistInfo(iSpecialist).getTexture(), "", ButtonStyles.BUTTON_STYLE_LABEL, WidgetTypes.WIDGET_CHANGE_SPECIALIST, iSpecialist, -1)
					screen.show(szName)
# BUG - city specialist - end

	# Will update the game data strings
	def updateGameDataStrings(self):
		# <advc.009b> Work around exceptions upon reloading scripts
		if not hasattr(self, "screen"):
			return # </advc.009b>
		screen = self.screen

		screen.hide("ResearchText")
		screen.hide("ResearchIconText")
		screen.hide("GoldText")
		screen.hide("TimeText")
		screen.hide("TwoLineResearchBar")
# BUG - NJAGC - start
		screen.hide("EraText")
# BUG - NJAGC - end
# BUG - Great Person Bar - start
		screen.hide("TwoLineGPBar")
		screen.hide("GreatPersonBarText")
# BUG - Great Person Bar - end
# BUG - Great General Bar - start
		screen.hide("TwoLineGGBar")
		screen.hide("GreatGeneralBarText")
# BUG - Great General Bar - end
# BUG - Bars on single line for higher resolution screens - start
		screen.hide("OneLineGGBar")
		screen.hide("OneLineResearchBar")
		screen.hide("OneLineGPBar")
# BUG - Bars on single line for higher resolution screens - end
# BUG - Progress Bar - Tick Marks - start
		self.pTwoLineResearchBar.hide(screen)
		self.pOneLineResearchBar.hide(screen)
# BUG - Progress Bar - Tick Marks - end
		kInterface = CyInterface()
		kGame = gc.getGame()
		pHeadSelectedCity = kInterface.getHeadSelectedCity()
		if pHeadSelectedCity:
			ePlayer = pHeadSelectedCity.getOwner()
		else:
			ePlayer = kGame.getActivePlayer()
		if ePlayer < 0 or ePlayer >= gc.getMAX_PLAYERS():
			return
		pPlayer = gc.getPlayer(ePlayer)

		for iCommerce in range(CommerceTypes.NUM_COMMERCE_TYPES):
			szIndex = str(iCommerce)
			screen.hide("PercentText" + szIndex)
			screen.hide("RateText" + szIndex)
		screen.hide("CommerceRateSummaryText")

		eUIVis = kInterface.getShowInterface()
		if (eUIVis == InterfaceVisibility.INTERFACE_HIDE_ALL or eUIVis == InterfaceVisibility.INTERFACE_MINIMAP_ONLY or eUIVis == InterfaceVisibility.INTERFACE_ADVANCED_START):
			return
		bCityScreen = kInterface.isCityScreenUp()

		# Percent of commerce
		if pPlayer.isAlive():
			iCount = 0
			aCommerceRateSummary = []
			bUseCompactCommerceRates = ((not bCityScreen) and MainOpt.isShowMinMaxCommerceButtons())
			for i in range(CommerceTypes.NUM_COMMERCE_TYPES):
				eCommerce = (i + 1) % CommerceTypes.NUM_COMMERCE_TYPES
				#if (gc.getPlayer(ePlayer).isCommerceFlexible(eCommerce) or (CyInterface().isCityScreenUp() and (eCommerce == CommerceTypes.COMMERCE_GOLD))):
				if not self.showCommercePercent(eCommerce, ePlayer): # K-Mod
					continue
				szOutText = sasFontTagLabel + u"%c:%d%%" %(gc.getCommerceInfo(eCommerce).getChar(), pPlayer.getCommercePercent(eCommerce)) + SAS_FONT_TAG_CLOSE
				szString = "PercentText" + str(iCount)
				self.setLabel(szString, "Background", szOutText, CvUtil.FONT_LEFT_JUSTIFY, FontTypes.SMALL_FONT, -0.1)
				screen.show(szString)
				# <advc.004p>
				# </advc.004p>
				if (not bCityScreen and (eCommerce != CommerceTypes.COMMERCE_CULTURE or MainOpt.isShowTotalCultureRate())):
					szOutText = sasFontTagLabel + localText.getText("TXT_KEY_MISC_POS_GOLD_PER_TURN", (pPlayer.getCommerceRate(CommerceTypes(eCommerce)),))
					szOutText += SAS_FONT_TAG_CLOSE
					# <!-- custom: BUG Min/Max buttons add two extra commerce button columns in map view, so the green commerce-rate values (e.g. +297/Turn) overflow off the right edge or under expanded advisors.
					# Keep the useful percent text and +/- controls visible; collapse the rate values into one compact row below the sliders.
					# The compact row omits ": " so all visible commerce rates are more likely to fit on one line, especially in late-game 4-5 digit cases; city screen keeps the regular per-row rate labels. (GPT-5.5) -->
					if bUseCompactCommerceRates:
						aCommerceRateSummary.append(u"%c%d" %(gc.getCommerceInfo(eCommerce).getChar(), pPlayer.getCommerceRate(CommerceTypes(eCommerce))))
						iCount += 1
						continue
					szString = "RateText" + str(iCount)
# BUG - Min/Max Sliders - start
					szPointKey = szString
					if MainOpt.isShowMinMaxCommerceButtons():
						szPointKey += "BUG"
					else:
						szPointKey += "BtS"
					gSetPoint(szString, gPoint(szPointKey))
# BUG - Min/Max Sliders - end
					self.setLabel(szString, "Background", szOutText, CvUtil.FONT_LEFT_JUSTIFY, FontTypes.SMALL_FONT, -0.1)
					screen.show(szString)
				iCount += 1
			if bUseCompactCommerceRates and aCommerceRateSummary:
				self.setLabel("CommerceRateSummaryText", "Background", sasFontTagLabel + u"; ".join(aCommerceRateSummary) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, FontTypes.SMALL_FONT, -0.1)
				screen.show("CommerceRateSummaryText")
		self.updateTimeText()
		self.setLabel("TimeText", "Background", sasFontTagLabel + g_szTimeText + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, FontTypes.GAME_FONT, -0.3)
		screen.show("TimeText")
		# advc.103: Don't show gold rate and current research when investigating
		if (not pPlayer.isAlive() or (ePlayer != kGame.getActivePlayer() and not kGame.isDebugMode())):
			return
# BUG - Gold Rate Warning - start
		iGold = pPlayer.getGold()
		iGoldRate = pPlayer.calculateGoldRate()
		# advc.070 (comment): Only relevant for mod-mods I think, so I'm not changing anything here.
		if iGold < 0:
			szText = BugUtil.getText("TXT_KEY_MISC_NEG_GOLD", iGold)
			if iGoldRate != 0:
				if iGold + iGoldRate >= 0:
					szText += BugUtil.getText("TXT_KEY_MISC_POS_GOLD_PER_TURN", iGoldRate)
				elif iGoldRate >= 0:
					szText += BugUtil.getText("TXT_KEY_MISC_POS_WARNING_GOLD_PER_TURN", iGoldRate)
				else:
					szText += BugUtil.getText("TXT_KEY_MISC_NEG_GOLD_PER_TURN", iGoldRate)
		else:
			szText = BugUtil.getText("TXT_KEY_MISC_POS_GOLD", iGold)
			if iGoldRate != 0:
				# <advc.070>
				szRateText = " ("
				szRateText += BugUtil.getText("TXT_KEY_MISC_PER_TURN", iGoldRate)
				szRateText += ")"
				# (I've removed the broke color option again in order to make room on the BUG menu)
				#iRateColor = MainOpt.getGoldRateBrokeColor()
				if iGoldRate >= 0:
					iRateColor = MainOpt.getPositiveGoldRateColor()
					#szText += BugUtil.getText("TXT_KEY_MISC_POS_GOLD_PER_TURN", iGoldRate)
				#elif iGold + iGoldRate >= 0:
				else:
					iRateColor = MainOpt.getNegativeGoldRateColor()
					#szText += BugUtil.getText("TXT_KEY_MISC_NEG_WARNING_GOLD_PER_TURN", iGoldRate)
				#else:
				#szText += BugUtil.getText("TXT_KEY_MISC_NEG_GOLD_PER_TURN", iGoldRate)
				szText += localText.changeTextColor(szRateText, iRateColor)
				# </advc.070>

		if pPlayer.isStrike():
			szText += BugUtil.getPlainText("TXT_KEY_MISC_STRIKE")
		# advc.070: Always show the warning
		#if not MainOpt.isGoldRateWarning():
		#	szText = CyGameTextMgr().getGoldStr(ePlayer)
# BUG - Gold Rate Warning - end
		self.setLabel("GoldText", "Background", sasFontTagLabel + szText + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, FontTypes.GAME_FONT, -0.3)
		screen.show("GoldText")

		if ((iGoldRate != 0 and not pPlayer.isAnarchy()) or pPlayer.getGold() != 0):
			screen.show("GoldText")
# BUG - NJAGC - start
		#if (ClockOpt.isEnabled() and ClockOpt.isShowEra()):
		# advc.067: Replacing the above
		if ClockOpt.isShowEra():
			# <advc.067>
			if ClockOpt.isShowGameEra():
				iEra = kGame.getCurrentEra()
			else: # </advc.067>
				iEra = pPlayer.getCurrentEra()
			# <!-- custom: skip TXT_KEY_BUG_ERA ("%s1 Era") and use the description directly — "Renaissance" is shorter and self-explanatory without the "Era" suffix, and avoids overflow with left-side text when upscaled (e.g. during a golden age). (Claude code Sonnet 4.6) -->
			szText = gc.getEraInfo(iEra).getDescription()
			if ClockOpt.isUseEraColor():
				iEraColor = ClockOpt.getEraColor(gc.getEraInfo(iEra).getType())
				if iEraColor >= 0:
					szText = localText.changeTextColor(szText, iEraColor)
			self.setLabel("EraText", "Background", sasFontTagLabel + szText + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, FontTypes.GAME_FONT, -0.3)
			screen.show("EraText")
# BUG - NJAGC - end
# BUG - Bars on single line for higher resolution screens - start
		if self.isShowTopBarsOnSingleLine():
			szResearchBar = "OneLineResearchBar"
		else:
			szResearchBar = "TwoLineResearchBar"
		iResearchTextOffset = self.stackBarDefaultTextOffset() - VSPACE(2)
		gOffSetPoint("ResearchText", szResearchBar, RectLayout.CENTER, iResearchTextOffset)
		bAnarchy = pPlayer.isAnarchy()
		szText = None
		szIconText = None
		iImgSize = -1
		iTech = -1 # advc.001: Need tech id for jump to Pedia
		if bAnarchy:
			szText = localText.getText("INTERFACE_ANARCHY", (pPlayer.getAnarchyTurns(),))
		elif pPlayer.getCurrentResearch() != -1:
			iTech = pPlayer.getCurrentResearch() # advc.001
			kTeam = gc.getTeam(pPlayer.getTeam())
			researchProgress = kTeam.getResearchProgress(iTech)
			overflowResearch = (pPlayer.getOverflowResearch() * pPlayer.calculateResearchModifier(iTech)) / 100
			researchCost = kTeam.getResearchCost(iTech)
			bTickMarks = MainOpt.isShowBarTickMarks() # advc.078
			researchRate = pPlayer.calculateResearchRate(-1)
			if self.IS_SAS_CV_MAIN_INTERFACE_PRODUCTION_QUEUE_BUTTONS:
				szTechName = gc.getTechInfo(iTech).getDescription()
				iTurns = pPlayer.getResearchTurnsLeft(iTech, True)
				szTechBtn = gc.getTechInfo(iTech).getButton()
				iImgSize = BTNSZ(25)
				szIconText = "<img=%s size=%d/>" % (szTechBtn, iImgSize)
				szText = "%s: %d / %d (%d)" % (szTechName, researchProgress + overflowResearch, researchCost, iTurns)
			else:
				szText = CyGameTextMgr().getResearchStr(ePlayer)
		if szText:
			szText = sasFontTagLabel + szText + SAS_FONT_TAG_CLOSE
			if (self.IS_SAS_CV_MAIN_INTERFACE_PRODUCTION_QUEUE_BUTTONS and not bAnarchy and iTech != -1 and szTechBtn):
				# <!-- custom: setImageButton pins its top-left to the given Y, unlike <img> tags which the engine centers automatically.
				# Offset so the icon's center matches the bar's center rather than sitting pinned near the bar's top edge. (Claude code Sonnet 4.6) -->
				iResearchIconYDownlift = VSPACE(1)
				iResearchIconYOffset = (gRect(szResearchBar).height() - iImgSize) / 2 - iResearchTextOffset + iResearchIconYDownlift
				self.setSASCenteredImageButtonAtLeftOfTextRow("ResearchText", "ResearchIconText", "Background", szText, szTechBtn, iImgSize, None, gPoint("ResearchText").y(), HSPACE(2), szResearchBar, 0, iResearchIconYOffset, FontTypes.GAME_FONT, -0.4, WidgetTypes.WIDGET_RESEARCH, iTech)
			else:
				# advc.001: Pass along tech id
				self.setText("ResearchText", "Background", szText, CvUtil.FONT_CENTER_JUSTIFY, FontTypes.GAME_FONT, -0.4, WidgetTypes.WIDGET_RESEARCH, iTech)
			# advc.004x: Always show ResearchText (i.e. anarchy turns)
			#if not bAnarchy:
			screen.show("ResearchText")
			if (self.IS_SAS_CV_MAIN_INTERFACE_PRODUCTION_QUEUE_BUTTONS and not bAnarchy and iTech != -1 and szTechBtn):
				screen.show("ResearchIconText")
			else:
				screen.hide("ResearchIconText")
			#else:
			#	screen.hide("ResearchText")
		if szText and not bAnarchy:
# BUG - Bars on single line for higher resolution screens - end
			# <advc.078>
			# Meaning that overflow is shown as part of the current progress
			overflowProgress = overflowResearch
			overflowRate = 0
			if bTickMarks:
				# Meaning overflow is shown as part of the next turn's research rate
				overflowRate = overflowResearch
				overflowProgress = 0
			# Mostly BtS code from here
			progressPortion = float(researchProgress + overflowProgress) / researchCost
			screen.setBarPercentage(szResearchBar, InfoBarTypes.INFOBAR_STORED, progressPortion)
			if (researchCost > researchProgress + overflowProgress):
				ratePortion = float(researchRate + overflowRate) / researchCost
				# I don't understand why, but setBarPercentage seems to multiply its argument by the remaining portion of the bar. Cancel that out:
				ratePortion /= (1 - progressPortion)
				screen.setBarPercentage(szResearchBar, InfoBarTypes.INFOBAR_RATE, ratePortion)
			# </advc.078>
			else:
				screen.setBarPercentage(szResearchBar, InfoBarTypes.INFOBAR_RATE, 0.0)
			screen.show(szResearchBar)
# BUG - Progress Bar - Tick Marks - start
			# advc.004x: researchRate condition added
			if bTickMarks and researchRate > 0:
				if szResearchBar == "TwoLineResearchBar":
					self.pTwoLineResearchBar.drawTickMarks(screen, researchProgress + overflowResearch, researchCost, researchRate, researchRate, False)
				else:
					self.pOneLineResearchBar.drawTickMarks(screen, researchProgress + overflowResearch, researchCost, researchRate, researchRate, False)
# BUG - Progress Bar - Tick Marks - end
		self.updateGreatPersonBar(screen) # BUG - Great Person Bar
		self.updateGreatGeneralBar(screen) # BUG - Great General Bar
		return

# BUG - Great Person Bar - start
	def updateGreatPersonBar(self, screen):
		if not self.isShowGPProgressBar() or CyInterface().isCityScreenUp():
			return
		if self.isShowTopBarsOnSingleLine():
			szGPBar = "OneLineGPBar"
		else:
			szGPBar = "TwoLineGPBar"
		gOffSetPoint("GreatPersonBarText", szGPBar, RectLayout.CENTER, self.stackBarDefaultTextOffset())
		pGPCity, iGPTurns = GPUtil.getDisplayCity()
		szText = GPUtil.getGreatPeopleText(pGPCity, iGPTurns, gRect(szGPBar).width(), MainOpt.isGPBarTypesNone(), MainOpt.isGPBarTypesOne(), True)
		szText = sasFontTagLabel + u"%s" % (szText) + SAS_FONT_TAG_CLOSE
		if pGPCity:
			iCityID = pGPCity.getID()
		else:
			iCityID = -1
# BUG - Bars on single line for higher resolution screens - start
		# <!-- custom: attach displayed GP city id so clicking the GP text can open that exact city screen. (GPT-5.3-Codex) -->
		self.setText("GreatPersonBarText", "Background", szText, CvUtil.FONT_CENTER_JUSTIFY, FontTypes.GAME_FONT, -0.4, WidgetTypes.WIDGET_GP_PROGRESS_BAR, iCityID, -1)
		if not pGPCity:
			screen.setHitTest("GreatPersonBarText", HitTestTypes.HITTEST_NOHIT)
		screen.show("GreatPersonBarText")
# BUG - Bars on single line for higher resolution screens - end
		if pGPCity:
			fThreshold = float(gc.getPlayer(pGPCity.getOwner()).greatPeopleThreshold(False))
			fRate = float(pGPCity.getGreatPeopleRate())
			fFirst = float(pGPCity.getGreatPeopleProgress()) / fThreshold
			screen.setBarPercentage(szGPBar, InfoBarTypes.INFOBAR_STORED, fFirst)
			if (fFirst == 1):
				screen.setBarPercentage(szGPBar, InfoBarTypes.INFOBAR_RATE, fRate / fThreshold)
			else:
				screen.setBarPercentage(szGPBar, InfoBarTypes.INFOBAR_RATE, fRate / fThreshold / (1 - fFirst))
		else:
			screen.setBarPercentage(szGPBar, InfoBarTypes.INFOBAR_STORED, 0)
			screen.setBarPercentage(szGPBar, InfoBarTypes.INFOBAR_RATE, 0)
		screen.show(szGPBar)
# BUG - Great Person Bar - end

# BUG - Great General Bar - start
	def updateGreatGeneralBar(self, screen):
		if not self.isShowGGProgressBar() or CyInterface().isCityScreenUp():
			return
		if self.isShowTopBarsOnSingleLine():
			szGGBar = "OneLineGGBar"
		else:
			szGGBar = "TwoLineGGBar"
		gOffSetPoint("GreatGeneralBarText", szGGBar, RectLayout.CENTER, self.stackBarDefaultTextOffset())
		pPlayer = gc.getActivePlayer()
		iCombatExp = pPlayer.getCombatExperience()
		iThresholdExp = pPlayer.greatPeopleThreshold(True)
		iNeededExp = iThresholdExp - iCombatExp
		szText = sasFontTagLabel + u"%s" %(GGUtil.getGreatGeneralText(iNeededExp)) + SAS_FONT_TAG_CLOSE
# BUG - Bars on single line for higher resolution screens - start
		self.setLabel("GreatGeneralBarText", "Background", szText, CvUtil.FONT_CENTER_JUSTIFY, FontTypes.GAME_FONT, -0.4, WidgetTypes.WIDGET_HELP_GREAT_GENERAL)
		screen.show("GreatGeneralBarText")
# BUG - Bars on single line for higher resolution screens - end
		fProgress = float(iCombatExp) / float(iThresholdExp)
		screen.setBarPercentage(szGGBar, InfoBarTypes.INFOBAR_STORED, fProgress)
		screen.show(szGGBar)
# BUG - Great General Bar - end

	# <advc.078> I've replaced all calls to MainOpt.isShowGGProgressBar and MainOpt.isShowGPProgressBar with these functions
	def isShowGGProgressBar(self):
		if not MainOpt.isShowGGProgressBar():
			return False
		if not MainOpt.isShowOnlyOnceProgress():
			return True
		iActivePlayer = gc.getGame().getActivePlayer()
		if iActivePlayer < 0:
			return False
		activePlayer = gc.getPlayer(iActivePlayer)
		return (activePlayer.getCombatExperience() > 0 or activePlayer.getGreatGeneralsCreated() > 0)

	def isShowGPProgressBar(self):
		if not MainOpt.isShowGPProgressBar():
			return False
		if not MainOpt.isShowOnlyOnceProgress():
			return True
		iActivePlayer = gc.getGame().getActivePlayer()
		if iActivePlayer < 0:
			return False
		activePlayer = gc.getPlayer(iActivePlayer)
		# Don't want to check all cities here over and over. Let the DLL keep track of this (new function: isAnyGPPEver). The second condition is only for old savegames.
		return (activePlayer.isAnyGPPEver() or activePlayer.getGreatPeopleCreated() > 0)
	# </advc.078>
	# advc.092:
	def isShowTopBarsOnSingleLine(self):
		# Show the potentially longer non-single-line research bar on the city screen
		return (gRect("TopBarsMaxSpace").encloses(gRect("TopBarsOneLineContainer")) and (self.isShowGGProgressBar() or self.isShowGPProgressBar()) and not CyInterface().isCityScreenUp())

	# advc.092: Don't show that frame when the bars take up two lines
	def isShowTopBarsFrame(self):
		return (self.isShowTopBarsOnSingleLine() or (not self.isShowGGProgressBar() and not self.isShowGPProgressBar()))

	def updateTimeText(self):

		global g_szTimeText

		ePlayer = gc.getGame().getActivePlayer()

# BUG - NJAGC - start
		# <advc.067> Moved out of the isEnabled block
		iEraColor = -1
		if(ClockOpt.isUseEraColor()):
			if ClockOpt.isShowGameEra():
				iEra = gc.getGame().getCurrentEra()
			else: # </advc.067>
				iEra = gc.getPlayer(ePlayer).getCurrentEra()
				iEraColor = ClockOpt.getEraColor(gc.getEraInfo(iEra).getType())
		# advc.067: End of moved code
		if (ClockOpt.isEnabled()):
			# Format: Time - GameTurn/Total Percent - GA (TurnsLeft) Date
			#
			# Ex: 10:37 - 220/660 33% - GA (3) 1925
			#
			if (g_bShowTimeTextAlt):
				bShowTime = ClockOpt.isShowAltTime()
				bShowGameTurn = ClockOpt.isShowAltGameTurn()
				bShowTotalTurns = ClockOpt.isShowAltTotalTurns()
				# advc.067: Option removed
				bShowPercentComplete = False #ClockOpt.isShowAltPercentComplete()
				bShowDateGA = ClockOpt.isShowAltDateGA()
			else:
				bShowTime = ClockOpt.isShowTime()
				bShowGameTurn = ClockOpt.isShowGameTurn()
				bShowTotalTurns = ClockOpt.isShowTotalTurns()
				# advc.067: Option removed
				bShowPercentComplete = False#ClockOpt.isShowPercentComplete()
				bShowDateGA = ClockOpt.isShowDateGA()

			if (not gc.getGame().getMaxTurns() > 0):
				bShowTotalTurns = False
				bShowPercentComplete = False

			bFirst = True
			g_szTimeText = ""

			if (bShowTime):
				bFirst = False
				g_szTimeText += getClockText()

			if (bShowGameTurn):
				if (bFirst):
					bFirst = False
				else:
					g_szTimeText += u" - "
				g_szTimeText += u"%d" %(gc.getGame().getElapsedGameTurns())
				if (bShowTotalTurns):
					g_szTimeText += u"/%d" %(gc.getGame().getMaxTurns())

			if (bShowPercentComplete):
				if (bFirst):
					bFirst = False
				else:
					if (not bShowGameTurn):
						g_szTimeText += u" - "
					else:
						g_szTimeText += u" "
				g_szTimeText += u"%2.2f%%" %(100 *(float(gc.getGame().getElapsedGameTurns()) / float(gc.getGame().getMaxTurns())))

			if (bShowDateGA):
				if (bFirst):
					bFirst = False
				else:
					g_szTimeText += u" - "
				szDateGA = unicode(CyGameTextMgr().getInterfaceTimeStr(ePlayer))
				if iEraColor >= 0: # advc.067: Replacing code that has moved up
					szDateGA = localText.changeTextColor(szDateGA, iEraColor)
				g_szTimeText += szDateGA
		else:
			# Original Clock
			# Format: Time - 'Turn' GameTurn - GA (TurnsLeft) Date
			#
			# Ex: 10:37 - Turn 220 - GA (3) 1925
			#
			g_szTimeText = localText.getText("TXT_KEY_TIME_TURN", (CyGame().getGameTurn(),)) + u" - "
			# <advc.067> Based on NJAGC code above
			szDate = unicode(CyGameTextMgr().getInterfaceTimeStr(ePlayer))
			if iEraColor >= 0:
				szDate = localText.changeTextColor(szDate, iEraColor)
			g_szTimeText += szDate
			# </advc.067>
			if (CyUserProfile().isClockOn()):
				g_szTimeText = getClockText() + u" - " + g_szTimeText
# BUG - NJAGC - end

	# Will update the selection Data Strings
	def updateCityScreen(self):
		# advc: unused
		#global MAX_DISPLAYABLE_BUILDINGS
		#global MAX_DISPLAYABLE_TRADE_ROUTES
		#global MAX_BONUS_ROWS
		#global g_iNumTradeRoutes
		#global g_iNumBuildings
		global g_iNumLeftBonus
		global g_iNumCenterBonus
		global g_iNumRightBonus
		global g_szCityBreakdownsHelp

		screen = self.screen
		pHeadSelectedCity = CyInterface().getHeadSelectedCity()
		xResolution = screen.getXResolution()
		yResolution = screen.getYResolution()
		bShift = CyInterface().shiftKey()

		screen.hide("PopulationBar")
		screen.hide("ProductionBar")
		screen.hide("GreatPeopleBar")
		screen.hide("CultureBar")
		screen.hide("MaintenanceText")
		screen.hide("MaintenanceAmountText")
# BUG - Progress Bar - Tick Marks - start
		self.pBarPopulationBar.hide(screen)
		self.pBarProductionBar.hide(screen)
		self.pBarProductionBar_Whip.hide(screen)
# BUG - Progress Bar - Tick Marks - end
# BUG - Raw Commerce - start
		screen.hide("RawYieldsTrade0")
		screen.hide("RawYieldsFood1")
		screen.hide("RawYieldsProduction2")
		screen.hide("RawYieldsCommerce3")
		screen.hide("RawYieldsWorkedTiles4")
		screen.hide("RawYieldsCityTiles5")
		screen.hide("RawYieldsOwnedTiles6")
# BUG - Raw Commerce - end
		screen.hide("NationalityText")
		screen.hide("NationalityBar")
		screen.hide("DefenseText")
		screen.hide("CityScrollMinus")
		screen.hide("CityScrollPlus")
		screen.hide("CityNameText")
		screen.hide("PopulationText")
		screen.hide("PopulationInputText")
		screen.hide("HealthText")
		screen.hide("ProductionText")
		screen.hide("ProductionIconText")
		screen.hide("ProductionInputText")
		screen.hide("HappinessText")
		screen.hide("CultureText")
		screen.hide("GreatPeopleText")

		# <!-- custom: fix the new specialist breakdown not disappearing after we exit the city screen, with the help of chatgpt 5.2 thanks -->
		# Specialist Breakdown widget (city screen only)
		screen.hide("SpecBreakdownLabel1")
		screen.hide("SpecBreakdownLabel2")
		screen.hide("CultureBreakdownLabel1")
		screen.hide("CultureBreakdownLabel2")

		for i in range(gc.getNumReligionInfos()):
			szName = "ReligionHolyCityDDS" + str(i)
			screen.hide(szName)
			szName = "ReligionDDS" + str(i)
			screen.hide(szName)
		for i in range(gc.getNumCorporationInfos()):
			szName = "CorporationHeadquarterDDS" + str(i)
			screen.hide(szName)
			szName = "CorporationDDS" + str(i)
			screen.hide(szName)
		for i in range(CommerceTypes.NUM_COMMERCE_TYPES):
			screen.hide("CityPercentText" + str(i))

		self.addPanel("BonusPane0", PanelStyles.PANEL_STYLE_CITY_COLUMNL)
		screen.hide("BonusPane0")
		self.addScrollPanel("BonusBack0")
		screen.hide("BonusBack0")

		self.addPanel("BonusPane1", PanelStyles.PANEL_STYLE_CITY_COLUMNC)
		screen.hide("BonusPane1")
		self.addScrollPanel("BonusBack1")
		screen.hide("BonusBack1")

		self.addPanel("BonusPane2", PanelStyles.PANEL_STYLE_CITY_COLUMNR)
		screen.hide("BonusPane2")
		self.addScrollPanel("BonusBack2")
		screen.hide("BonusBack2")

		screen.hide("TradeRouteTable")
		screen.hide("BuildingListTable")
		screen.hide("BuildingListBackground")
		screen.hide("TradeRouteListBackground")
		screen.hide("BuildingListLabel")
		screen.hide("TradeRouteListLabel")
		for i in range(g_iNumLeftBonus):
			szName = "LeftBonusItem" + str(i)
			screen.hide(szName)
			# <advc.092>
			if self.bCityBonusButtons:
				szIndex = "0_" + str(i)
				screen.hide("CityBonusBtn" + szIndex)
				screen.hide("CityBonusCircle" + szIndex)
				screen.hide("CityBonusAmount" + szIndex)
			# </advc.092>
		for i in range(g_iNumCenterBonus):
			szName = "CenterBonusItemLeft" + str(i)
			screen.hide(szName)
			szName = "CenterBonusItemRight" + str(i)
			screen.hide(szName)
			# <advc.092>
			if self.bCityBonusButtons:
				szIndex = "1_" + str(i)
				screen.hide("CityBonusBtn" + szIndex)
				screen.hide("CityBonusCircle" + szIndex)
				screen.hide("CityBonusAmount" + szIndex)
				screen.hide("CityBonusEffect" + szIndex)
			# </advc.092>
		for i in range(g_iNumRightBonus):
			szName = "RightBonusItemLeft" + str(i)
			screen.hide(szName)
			szName = "RightBonusItemRight" + str(i)
			screen.hide(szName)
			# <advc.092>
			if self.bCityBonusButtons:
				szIndex = "2_" + str(i)
				screen.hide("CityBonusBtn" + szIndex)
				screen.hide("CityBonusCircle" + szIndex)
				screen.hide("CityBonusAmount" + szIndex)
				screen.hide("CityBonusEffect" + szIndex)
			# </advc.092>
		for i in range(3):
			szName = "BonusPane" + str(i)
			screen.hide(szName)
			szName = "BonusBack" + str(i)
			screen.hide(szName)

		# advc: Deal with the non-city branch first (to reduce indentation)
		if not CyInterface().isCityScreenUp():
			screen.hide(self.buildFilterBtnAll)
			screen.hide(self.buildFilterBtnBuildings)
			screen.hide(self.buildFilterBtnNational)
			screen.hide(self.buildFilterBtnWorld)
			self.setDefaultHelpTextArea(CyInterface().getShowInterface() != InterfaceVisibility.INTERFACE_SHOW)

			screen.hide("TopCityPanelCenter")
			# advc: Doesn't exist
			#screen.hide("InterfaceTopRightBackgroundWidget")
			screen.hide("CityLeftPanel")
			screen.hide("CityScreenTopWidget")
			screen.hide("CityNameBackground")
			screen.hide("TopCityPanelLeft")
			screen.hide("TopCityPanelRight")
			screen.hide("CityAdjustPanel")
			screen.hide("CityRightPanel")
			# <advc.092>
			screen.hide("TopCityPanelCenterStagger")
			screen.hide("CityLeftPanelStagger")
			screen.hide("CityRightPanelStagger") # </advc.092>

			if (CyInterface().getShowInterface() == InterfaceVisibility.INTERFACE_SHOW):
				self.setMinimapButtonVisibility(True)
			return 0
		if not pHeadSelectedCity:
			return 0
		# <advc.092>
		screen.show("TopCityPanelCenterStagger")
		screen.show("CityLeftPanelStagger")
		screen.show("CityRightPanelStagger") # </advc.092>
		screen.show("TopCityPanelCenter")
		#screen.show("InterfaceTopRightBackgroundWidget")
		screen.show("CityLeftPanel")
		screen.show("CityScreenTopWidget")
		screen.show("CityNameBackground")
		screen.show("TopCityPanelLeft")
		screen.show("TopCityPanelRight")
		screen.show("CityAdjustPanel")
		screen.show("CityRightPanel")

		if (pHeadSelectedCity.getTeam() == gc.getGame().getActiveTeam()):
			if (gc.getActivePlayer().getNumCities() > 1):
				screen.show("CityScrollMinus")
				screen.show("CityScrollPlus")

		screen.setHelpTextArea(HLEN(390), FontTypes.SMALL_FONT, 0, 0, -2.2, True, self.backgroundTransparentPath, True, True, CvUtil.FONT_LEFT_JUSTIFY, 0)

		iFoodDifference = pHeadSelectedCity.foodDifference(True)
		iProductionDiffNoFood = pHeadSelectedCity.getCurrentProductionDifference(True, True)
		iProductionDiffJustFood = (pHeadSelectedCity.getCurrentProductionDifference(False, True) - iProductionDiffNoFood)

		szBuffer = sasFontTagTitle

		if (pHeadSelectedCity.isCapital()):
			szBuffer += self.szStarIcon
		elif (pHeadSelectedCity.isGovernmentCenter()):
			szBuffer += self.szSilverStarIcon

		if (pHeadSelectedCity.isPower()):
			szBuffer += self.szPowerIcon

		iPop = pHeadSelectedCity.getPopulation()
		iCultureTotal = pHeadSelectedCity.getCulture(pHeadSelectedCity.getOwner())
		iFoundTurn = pHeadSelectedCity.getGameTurnFounded()
		iFoundYear = CyGame().getTurnYear(iFoundTurn)
		szFoundYear = BugUtil.getDisplayYear(iFoundYear)
		szBuffer += u"%s: %d - %d (%s)" % (pHeadSelectedCity.getName(), iPop, iCultureTotal, szFoundYear)

		if (pHeadSelectedCity.isOccupation()):
			szBuffer += u" (%s:%d)" %(self.szOccupationIcon, pHeadSelectedCity.getOccupationTimer())

		iDefenseModifier = pHeadSelectedCity.getDefenseModifier(False)
		if (iDefenseModifier != 0):
			szDefBuffer = localText.getText("TXT_KEY_MAIN_CITY_DEFENSE", (self.iDefenseIcon, iDefenseModifier))
			if (pHeadSelectedCity.getDefenseDamage() > 0):
				szTempBuffer = u" (%d%%)" %(((gc.getMAX_CITY_DEFENSE_DAMAGE() - pHeadSelectedCity.getDefenseDamage()) * 100) / gc.getMAX_CITY_DEFENSE_DAMAGE())
				szDefBuffer = szDefBuffer + szTempBuffer
			szBuffer += u"  " + szDefBuffer

		szBuffer += SAS_FONT_TAG_CLOSE

		self.setText("CityNameText", "Background", szBuffer, CvUtil.FONT_CENTER_JUSTIFY, FontTypes.GAME_FONT, -0.3, WidgetTypes.WIDGET_CITY_NAME)
		screen.setStyle("CityNameText", "Button_Stone_Style")
		screen.show("CityNameText")

# BUG - Food Assist - start
		if (CityUtil.willGrowThisTurn(pHeadSelectedCity) or (iFoodDifference != 0) or not (pHeadSelectedCity.isFoodProduction())):
			iFoodNow = pHeadSelectedCity.getFood()
			iFoodThresh = pHeadSelectedCity.growthThreshold()
			# K-Mod disabled this. I think 'Growth!' sounds lame.
#			if (CityUtil.willGrowThisTurn(pHeadSelectedCity)):
				#was elif on next line
#				szBuffer = localText.getText("INTERFACE_CITY_GROWTH", ())
			if (iFoodDifference > 0):
				# <advc.002f>
				if CityUtil.avoidingGrowth(pHeadSelectedCity):
					szBuffer = "Avoiding Growth - %d / %d (%d Turns)" % (iFoodNow, iFoodThresh, pHeadSelectedCity.getFoodTurnsLeft())
				else: # </advc.002f>
					szBuffer = "Growing - %d / %d (%d Turns)" % (iFoodNow, iFoodThresh, pHeadSelectedCity.getFoodTurnsLeft())
			elif (iFoodDifference < 0):
				#iTurnsToStarve = pHeadSelectedCity.getFood() / -iFoodDifference + 1
				# advc.189: The DLL can compute this now
				iTurnsToStarve = -pHeadSelectedCity.getFoodTurnsLeft()
				if iTurnsToStarve > 1:
					szBuffer = "Shrinking - %d / %d (%d Turns)" % (iFoodNow, iFoodThresh, iTurnsToStarve)
				else:
					szBuffer = "Starving - %d / %d (1 Turn)" % (iFoodNow, iFoodThresh)
# BUG - Food Assist - end
			else:
				szBuffer = "Stagnant - %d / %d" % (iFoodNow, iFoodThresh)

			self.setLabel("PopulationText", "Background", sasFontTagLabel + szBuffer + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, FontTypes.GAME_FONT, -1.3)
			screen.setHitTest("PopulationText", HitTestTypes.HITTEST_NOHIT)
			screen.show("PopulationText")

		if (not pHeadSelectedCity.isDisorder() and not pHeadSelectedCity.isFoodProduction()):
# BUG - Food Assist - start
			if (CityScreenOpt.isShowFoodAssist()):
				iFoodYield = pHeadSelectedCity.getYieldRate(YieldTypes.YIELD_FOOD)
				iFoodEaten = pHeadSelectedCity.foodConsumption(False, 0)
				if iFoodYield == iFoodEaten:
					szBuffer = localText.getText("INTERFACE_CITY_FOOD_STAGNATE", (iFoodYield, iFoodEaten))
				elif iFoodYield > iFoodEaten:
					szBuffer = localText.getText("INTERFACE_CITY_FOOD_GROW", (iFoodYield, iFoodEaten, iFoodYield - iFoodEaten))
				else:
					szBuffer = localText.getText("INTERFACE_CITY_FOOD_SHRINK", (iFoodYield, iFoodEaten, iFoodYield - iFoodEaten))
			else:
				szBuffer = u"%d%s - %d%s" %(pHeadSelectedCity.getYieldRate(YieldTypes.YIELD_FOOD), self.szFoodIcon, pHeadSelectedCity.foodConsumption(False, 0), self.szEatenFoodIcon)
# BUG - Food Assist - end
# BUG - Food Rate Hover - start
			# draw label below
		else:
			szBuffer = u"%d%s" %(iFoodDifference, self.szFoodIcon)
			# draw label below
		# advc.004: BULL widget help enabled
		self.setLabel("PopulationInputText", "Background", sasFontTagLabel + szBuffer + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, FontTypes.GAME_FONT, -0.3, WidgetTypes.WIDGET_FOOD_MOD_HELP)
		screen.show("PopulationInputText")
# BUG - Food Rate Hover - end
		if (pHeadSelectedCity.badHealth(False) > 0 or pHeadSelectedCity.goodHealth() >= 0):
			if (pHeadSelectedCity.healthRate(False, 0) < 0):
				szBuffer = localText.getText("INTERFACE_CITY_HEALTH_BAD", (pHeadSelectedCity.goodHealth(), pHeadSelectedCity.badHealth(False), pHeadSelectedCity.healthRate(False, 0)))
			elif (pHeadSelectedCity.badHealth(False) > 0):
				szBuffer = localText.getText("INTERFACE_CITY_HEALTH_GOOD", (pHeadSelectedCity.goodHealth(), pHeadSelectedCity.badHealth(False)))
			else:
				szBuffer = localText.getText("INTERFACE_CITY_HEALTH_GOOD_NO_BAD", (pHeadSelectedCity.goodHealth(),))
			self.setLabel("HealthText", "Background", sasFontTagLabel + szBuffer + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, FontTypes.GAME_FONT, -0.3, WidgetTypes.WIDGET_HELP_HEALTH)
			screen.show("HealthText")
		if (iFoodDifference < 0):
			if (pHeadSelectedCity.getFood() + iFoodDifference > 0):
				iDeltaFood = pHeadSelectedCity.getFood() + iFoodDifference
			else:
				iDeltaFood = 0
			if (-iFoodDifference < pHeadSelectedCity.getFood()):
				iExtraFood = -iFoodDifference
			else:
				iExtraFood = pHeadSelectedCity.getFood()
			screen.setBarPercentage("PopulationBar", InfoBarTypes.INFOBAR_STORED, float(iDeltaFood) / pHeadSelectedCity.growthThreshold())
			screen.setBarPercentage("PopulationBar", InfoBarTypes.INFOBAR_RATE, 0.0)
			if (pHeadSelectedCity.growthThreshold() > iDeltaFood):
				screen.setBarPercentage("PopulationBar", InfoBarTypes.INFOBAR_RATE_EXTRA, float(iExtraFood) / (pHeadSelectedCity.growthThreshold() - iDeltaFood))
			else:
				screen.setBarPercentage("PopulationBar", InfoBarTypes.INFOBAR_RATE_EXTRA, 0.0)
		else:
			screen.setBarPercentage("PopulationBar", InfoBarTypes.INFOBAR_STORED, float(pHeadSelectedCity.getFood()) / pHeadSelectedCity.growthThreshold())
			if (pHeadSelectedCity.growthThreshold() > pHeadSelectedCity.getFood()):
				screen.setBarPercentage("PopulationBar", InfoBarTypes.INFOBAR_RATE, (float(iFoodDifference) / (pHeadSelectedCity.growthThreshold() - pHeadSelectedCity.getFood())))
			else:
				screen.setBarPercentage("PopulationBar", InfoBarTypes.INFOBAR_RATE, 0.0)
			screen.setBarPercentage("PopulationBar", InfoBarTypes.INFOBAR_RATE_EXTRA, 0.0)
		screen.show("PopulationBar")
# BUG - Progress Bar - Tick Marks - start
		if MainOpt.isShowBarTickMarks():
			self.pBarPopulationBar.drawTickMarks(screen, pHeadSelectedCity.getFood(), pHeadSelectedCity.growthThreshold(), iFoodDifference, iFoodDifference, False)
# BUG - Progress Bar - Tick Marks - end
		if (pHeadSelectedCity.getOrderQueueLength() > 0):
			if (pHeadSelectedCity.isProductionProcess()):
				szBuffer = pHeadSelectedCity.getProductionName()
# BUG - Whip Assist - start
			else:
				iProductionTurns = pHeadSelectedCity.getProductionTurnsLeft() # advc.004x
				iProdCur = pHeadSelectedCity.getProduction()
				iProdNeed = pHeadSelectedCity.getProductionNeeded()
				bProdDetailAdded = False
				if iProductionTurns > 0:
					szProdBase = "%s - %d / %d (%d Turns)" % (pHeadSelectedCity.getProductionName(), iProdCur, iProdNeed, iProductionTurns)
					bProdDetailAdded = True
				else:
					szProdBase = pHeadSelectedCity.getProductionName()
				HURRY_WHIP = self.hurryPopulation
				HURRY_BUY = self.hurryGold
				if (CityScreenOpt.isShowWhipAssist() and pHeadSelectedCity.canHurry(HURRY_WHIP, False)):
					iHurryPop = pHeadSelectedCity.hurryPopulation(HURRY_WHIP)
					bCountCurrentOverflow = CityScreenOpt.isWhipAssistOverflowCountCurrentProduction()
					iOverflow = pHeadSelectedCity.getHurryOverflow(HURRY_WHIP, True, bCountCurrentOverflow)
					iOverflowGold = pHeadSelectedCity.getHurryOverflow(HURRY_WHIP, False, bCountCurrentOverflow)
					szBuffer = "%s   %d%s %d%s" % (szProdBase, iHurryPop, self.szAngryPopIcon, iOverflow, self.szProductionIcon)
					if iOverflowGold > 0: # </advc.064>
						szBuffer += " %d%s" % (iOverflowGold, self.szGoldIcon)
				elif (CityScreenOpt.isShowWhipAssist() and pHeadSelectedCity.canHurry(HURRY_BUY, False)):
					iHurryCost = pHeadSelectedCity.hurryGold(HURRY_BUY)
					szBuffer = "%s   -%d%s" % (szProdBase, iHurryCost, self.szGoldIcon)
				else:
					# <advc.004x>
					if iProductionTurns <= 0:
						szBuffer = szProdBase
					else: # </advc.004x>
						szBuffer = szProdBase
						bProdDetailAdded = True
# BUG - Whip Assist - end
				if not bProdDetailAdded:
					if iProdNeed > 0:
						szBuffer = "%s - %d / %d" % (szBuffer, iProdCur, iProdNeed)
			# <!-- custom: show current production icon in the production text (SAS define gated). (GPT-5.2-Codex) -->
			if self.IS_SAS_CV_MAIN_INTERFACE_PRODUCTION_QUEUE_BUTTONS:
				szProdBtn = ""
				iOrderType = CyInterface().getOrderNodeType(0)
				iOrderData = CyInterface().getOrderNodeData1(0)
				if iOrderType == OrderTypes.ORDER_TRAIN:
					szProdBtn = gc.getUnitInfo(iOrderData).getButton()
				elif iOrderType == OrderTypes.ORDER_CONSTRUCT:
					szProdBtn = gc.getBuildingInfo(iOrderData).getButton()
				elif iOrderType == OrderTypes.ORDER_CREATE:
					szProdBtn = gc.getProjectInfo(iOrderData).getButton()
				elif iOrderType == OrderTypes.ORDER_MAINTAIN:
					szProdBtn = gc.getProcessInfo(iOrderData).getButton()
			if (self.IS_SAS_CV_MAIN_INTERFACE_PRODUCTION_QUEUE_BUTTONS and szProdBtn):
				iImgSize = BTNSZ(25)
				iProductionTextOffset = self.stackBarDefaultTextOffset()
				iProductionIconYDownlift = VSPACE(1)
				iProductionIconYOffset = (gRect("ProductionBar").height() - iImgSize) / 2 - iProductionTextOffset + iProductionIconYDownlift
				szProdText = sasFontTagLabel + szBuffer + SAS_FONT_TAG_CLOSE
				self.setSASCenteredImageButtonAtLeftOfTextRow("ProductionText", "ProductionIconText", "Background", szProdText, szProdBtn, iImgSize, None, gPoint("ProductionText").y(), HSPACE(2), "ProductionBar", 0, iProductionIconYOffset, FontTypes.GAME_FONT, -1.3, WidgetTypes.WIDGET_GENERAL)
				screen.show("ProductionIconText")
			else:
				self.setLabel("ProductionText", "Background", sasFontTagLabel + szBuffer + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, FontTypes.GAME_FONT, -1.3, WidgetTypes.WIDGET_GENERAL)
				screen.hide("ProductionIconText")
			screen.setHitTest("ProductionText", HitTestTypes.HITTEST_NOHIT)
			screen.show("ProductionText")
		if (pHeadSelectedCity.isProductionProcess()):
			szBuffer = u"%d%s" %(pHeadSelectedCity.getYieldRate(YieldTypes.YIELD_PRODUCTION), self.szProductionIcon)
		elif (pHeadSelectedCity.isFoodProduction() and iProductionDiffJustFood > 0):
			szBuffer = u"%d%s + %d%s" %(iProductionDiffJustFood, self.szFoodIcon, iProductionDiffNoFood, self.szProductionIcon)
		else:
			szBuffer = u"%d%s" %(iProductionDiffNoFood, self.szProductionIcon)
		self.setLabel("ProductionInputText", "Background", sasFontTagLabel + szBuffer + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, FontTypes.GAME_FONT, -0.3, WidgetTypes.WIDGET_PRODUCTION_MOD_HELP)
		screen.show("ProductionInputText")
		if (pHeadSelectedCity.happyLevel() >= 0 or pHeadSelectedCity.unhappyLevel(0) > 0):
			if (pHeadSelectedCity.isDisorder()):
				szBuffer = u"%d%s" %(pHeadSelectedCity.angryPopulation(0), self.szAngryPopIcon)
			elif (pHeadSelectedCity.angryPopulation(0) > 0):
				szBuffer = localText.getText("INTERFACE_CITY_UNHAPPY", (pHeadSelectedCity.happyLevel(), pHeadSelectedCity.unhappyLevel(0), pHeadSelectedCity.angryPopulation(0)))
			elif (pHeadSelectedCity.unhappyLevel(0) > 0):
				szBuffer = localText.getText("INTERFACE_CITY_HAPPY", (pHeadSelectedCity.happyLevel(), pHeadSelectedCity.unhappyLevel(0)))
			else:
				szBuffer = localText.getText("INTERFACE_CITY_HAPPY_NO_UNHAPPY", (pHeadSelectedCity.happyLevel(),))
# BUG - Anger Display - start
			# K-Mod
			if (CityScreenOpt.isShowAngerCounter() and (pHeadSelectedCity.getTeam() == gc.getGame().getActiveTeam() or gc.getGame().isDebugMode())):
				iAngerTimer = max(pHeadSelectedCity.getHurryAngerTimer(), pHeadSelectedCity.getConscriptAngerTimer())
				# <advc.188> Cover all temporary unhappiness (but not getHappinessTimer)
				# </advc.188>
				iAngerTimer = max(iAngerTimer, pHeadSelectedCity.getDefyResolutionAngerTimer())
				if iAngerTimer > 0:
					szBuffer += u" (%i)" % iAngerTimer
# BUG - Anger Display - end
			self.setLabel("HappinessText", "Background", sasFontTagLabel + szBuffer + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, FontTypes.GAME_FONT, -0.3, WidgetTypes.WIDGET_HELP_HAPPINESS)
			screen.show("HappinessText")
		if (not pHeadSelectedCity.isProductionProcess()):
			# advc.064: Moved up
			HURRY_WHIP = self.hurryPopulation
			iNeeded = pHeadSelectedCity.getProductionNeeded()
			iStored = pHeadSelectedCity.getProduction()
			screen.setBarPercentage("ProductionBar", InfoBarTypes.INFOBAR_STORED, float(iStored) / iNeeded)
			if iNeeded > iStored:
				screen.setBarPercentage("ProductionBar", InfoBarTypes.INFOBAR_RATE, float(iProductionDiffNoFood) / (iNeeded - iStored))
			else:
				screen.setBarPercentage("ProductionBar", InfoBarTypes.INFOBAR_RATE, 0.0)
			if iNeeded > iStored + iProductionDiffNoFood:
				screen.setBarPercentage("ProductionBar", InfoBarTypes.INFOBAR_RATE_EXTRA, float(iProductionDiffJustFood) / (iNeeded - iStored - iProductionDiffNoFood))
			else:
				screen.setBarPercentage("ProductionBar", InfoBarTypes.INFOBAR_RATE_EXTRA, 0.0)
			screen.show("ProductionBar")
# BUG - Progress Bar - Tick Marks - start
			# advc.064: Check moved down so that HurryTickMarks can be enabled independently
			if True:#MainOpt.isShowBarTickMarks():
				if (pHeadSelectedCity.isProductionProcess()):
					iFirst = 0
					iRate = 0
				elif (pHeadSelectedCity.isFoodProduction() and iProductionDiffJustFood > 0):
					iFirst = pHeadSelectedCity.getCurrentProductionDifference(False, True)
					iRate = pHeadSelectedCity.getCurrentProductionDifference(False, False)
				else:
					iFirst = pHeadSelectedCity.getCurrentProductionDifference(True, True)
					iRate = pHeadSelectedCity.getCurrentProductionDifference(True, False)
			if MainOpt.isShowBarTickMarks():
				self.pBarProductionBar.drawTickMarks(screen, pHeadSelectedCity.getProduction(), pHeadSelectedCity.getProductionNeeded(), iFirst, iRate, False)
			# advc.064: Now optional and independent from ShowBarTick. K-Mod changed from False to True.
			if (CityScreenOpt.isShowHurryTickMarks() and pHeadSelectedCity.canHurry(HURRY_WHIP, True)):
				iRate = (pHeadSelectedCity.hurryProduction(HURRY_WHIP) / pHeadSelectedCity.hurryPopulation(HURRY_WHIP))
				# <advc.064b> Subtract guaranteed production
				# </advc.064b>
				iMinProduction = (pHeadSelectedCity.minPlotProduction() + pHeadSelectedCity.getFeatureProduction())
				self.pBarProductionBar_Whip.drawTickMarks(screen, pHeadSelectedCity.getProduction(), pHeadSelectedCity.getProductionNeeded() - iMinProduction, iFirst, iRate, True)
# BUG - Progress Bar - Tick Marks - end
		iCount = 0
		for i in range(CommerceTypes.NUM_COMMERCE_TYPES):
			eCommerce = (i + 1) % CommerceTypes.NUM_COMMERCE_TYPES
			#if ((gc.getPlayer(pHeadSelectedCity.getOwner()).isCommerceFlexible(eCommerce)) or (eCommerce == CommerceTypes.COMMERCE_GOLD)):
			if self.showCommercePercent(eCommerce, pHeadSelectedCity.getOwner()): # K-Mod
				szBuffer = u"%d.%02d %c" %(pHeadSelectedCity.getCommerceRate(eCommerce), pHeadSelectedCity.getCommerceRateTimes100(eCommerce)%100, gc.getCommerceInfo(eCommerce).getChar())
				iHappiness = pHeadSelectedCity.getCommerceHappinessByType(eCommerce)
				if (iHappiness != 0):
					if (iHappiness > 0):
						szTempBuffer = u", %d%s" %(iHappiness, self.szHappyIcon)
					else:
						szTempBuffer = u", %d%s" %(-iHappiness, self.szUnhappyIcon)
					szBuffer = szBuffer + szTempBuffer
				szName = "CityPercentText" + str(iCount)
				self.setLabel(szName, "Background", sasFontTagLabel + szBuffer + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, FontTypes.SMALL_FONT, -0.3, WidgetTypes.WIDGET_COMMERCE_MOD_HELP, eCommerce)
				screen.show(szName)
				iCount += 1
		# <advc.092>
		# Slightly larger font actually hardly affects the amount of displayable info and imo doesn't make the table look too crammed either.
		# This means that the city screen uses size 1 only for the BUG yield breakdowns ("Raw Yields").
		if self.bScaleHUD and gRect("Top").height() > 900:
			# <!-- custom: use LABEL (not BODY) for city building/trade tables so label-font upscaling applies consistently when SAS_UI_FONT_LABEL is raised. (GPT-5.4) -->
			self.bldgFontTag = sasFontTagLabel
			self.trFontTag = sasFontTagLabel
			# Table rows don't or barely get higher, so the button size shouldn't be increased here.
			iBldgBtnSize = None
		else: # </advc.092>
			self.bldgFontTag = sasFontTagTiny
			self.trFontTag = sasFontTagTiny
			iBldgBtnSize = None
		self.addTable("BuildingListTable", 3, "Table_City_Style", iBldgBtnSize)
# BUG - Raw Yields - start
		bShowRawYields = g_bYieldView and CityScreenOpt.isShowRawYields()
		if bShowRawYields:
			iCols = 4
		else:
			iCols = 3
		self.addTable("TradeRouteTable", iCols, "Table_City_Style")
		# advc.092: The BUG and BtS column widths don't quite add up to the table's width. Don't know if that's as it should be.
		iAvailW = gRect("TradeRouteTable").width() - 2
		# advc.092: I think the width of the 3rd (empty) column should be consistent between TR and building table. Was 10 in BtS.
		iColW2 = 2
		if bShowRawYields:
			# advc.002b: Increased the (non-scaled) width of the first column by 15 and decreased the other three by ca. 5 each.
			screen.setTableColumnHeader("TradeRouteTable", 0, u"", (126 * iAvailW) / 236)
			screen.setTableColumnHeader("TradeRouteTable", 1, u"", (56 * iAvailW) / 236)
			screen.setTableColumnHeader("TradeRouteTable", 2, u"", (52 * iAvailW) / 236)
			screen.setTableColumnHeader("TradeRouteTable", 3, u"", (iColW2 * iAvailW) / 236)
			screen.setTableColumnRightJustify("TradeRouteTable", 1)
			screen.setTableColumnRightJustify("TradeRouteTable", 2)
		else:
# K-Mod: Trade culture
			screen.setTableColumnHeader("TradeRouteTable", 0, u"", (132 * iAvailW) / 236)
			screen.setTableColumnHeader("TradeRouteTable", 1, u"", (102 * iAvailW) / 236)
# K-Mod: Trade culture end
			screen.setTableColumnHeader("TradeRouteTable", 2, u"", (iColW2 * iAvailW) / 236)
			screen.setTableColumnRightJustify("TradeRouteTable", 1)
# BUG - Raw Yields - end
		# <advc.097>
		if CityScreenOpt.isBuildings_IconOnly():
			iColW0 = 40
		elif CityScreenOpt.isBuildings_IconAndText():
			iColW0 = 132
		else:
			iColW0 = 105 # 108 in BtS
		iColW1 = 236 - iColW0 - iColW2
		# </advc.097>
		iAvailW = gRect("BuildingListTable").width() - 2
		screen.setTableColumnHeader("BuildingListTable", 0, u"", (iColW0 * iAvailW) / 236)
		screen.setTableColumnHeader("BuildingListTable", 1, u"", (iColW1 * iAvailW) / 236)
		screen.setTableColumnHeader("BuildingListTable", 2, u"", (iColW2 * iAvailW) / 236)
		screen.setTableColumnRightJustify("BuildingListTable", 1)

		# <!-- custom: remove the 3 gray bars ("Trade Routes", "Buildings", "Specialists") as they take a lot of room and are unneeded, and we don't have a "Bonuses" bar for example so no reason to have these as well either.
		# Change with the help of gemini 3 pro thanks -->
		# Step 1: Hide the Header Widgets (Gray Bars)
		# screen.show("BuildingListBackground")
		# screen.show("TradeRouteListBackground")
		# screen.show("BuildingListLabel")
# BUG - Raw Yields - start
		if (CityScreenOpt.isShowRawYields()):
			screen.setState("RawYieldsTrade0", not g_bYieldView)
			screen.show("RawYieldsTrade0")

			screen.setState("RawYieldsFood1", g_bYieldView and g_iYieldType == YieldTypes.YIELD_FOOD)
			screen.show("RawYieldsFood1")
			screen.setState("RawYieldsProduction2", g_bYieldView and g_iYieldType == YieldTypes.YIELD_PRODUCTION)
			screen.show("RawYieldsProduction2")
			screen.setState("RawYieldsCommerce3", g_bYieldView and g_iYieldType == YieldTypes.YIELD_COMMERCE)
			screen.show("RawYieldsCommerce3")

			screen.setState("RawYieldsWorkedTiles4", g_iYieldTiles == RawYields.WORKED_TILES)
			screen.show("RawYieldsWorkedTiles4")
			screen.setState("RawYieldsCityTiles5", g_iYieldTiles == RawYields.CITY_TILES)
			screen.show("RawYieldsCityTiles5")
			screen.setState("RawYieldsOwnedTiles6", g_iYieldTiles == RawYields.OWNED_TILES)
			screen.show("RawYieldsOwnedTiles6")
		# <!-- custom: remove the 3 gray bars ("Trade Routes", "Buildings", "Specialists") as they take a lot of room and are unneeded, and we don't have a "Bonuses" bar for example so no reason to have these as well either.
		# Change with the help of gemini 3 pro thanks -->
		# Step 1: Hide the Header Widgets (Gray Bars)
		# else:
		# 	screen.show("TradeRouteListLabel")
# BUG - Raw Yields - end
		for i in range(3):
			szName = "BonusPane" + str(i)
			screen.show(szName)
			szName = "BonusBack" + str(i)
			screen.show(szName)
		iNumBuildings = 0
# BUG - Raw Yields - start
		self.yields = RawYields.Tracker()
# BUG - Raw Yields - end
		# <advc.097> Sort the building list?
		aCityBldgs = []
		for iBuilding in range(gc.getNumBuildingInfos()):
			if pHeadSelectedCity.getNumBuilding(iBuilding) > 0:
				eBuildingClass = gc.getBuildingInfo(iBuilding).getBuildingClassType()
				if self.iBuildingFilter == 1:
					if not self.isRegularBuildingClass(eBuildingClass):
						continue
				elif self.iBuildingFilter == 2:
					if not self.isNationalWonderClass(eBuildingClass):
						continue
				elif self.iBuildingFilter == 3:
					if not self.isWorldWonderClass(eBuildingClass):
						continue
				aCityBldgs.append(iBuilding)
		# I don't know ... This will result in a different order in every city.
		# Something chronological based on tech reqs (but also taking special care of wonders and free buildings) should work better than the XML ordering - tbd.?
		#
		# aCityBldgs = sorted(aCityBldgs, key=lambda iBuilding:
		# 		# AdvCiv returns -32768 for free buildings. Let's just treat
		# 		# all strange year numbers as 10000, which will move them
		# 		# to the end of the list. The or is a hack for a
		# 		# conditional expression (properly added in Python 2.5).
		# 		(abs(pHeadSelectedCity.getBuildingOriginalTime(iBuilding)) >= 10000 and 10000
		# 		or pHeadSelectedCity.getBuildingOriginalTime(iBuilding))
		# 		+ iBuilding) # id as tiebreaker
		#
		# Let's at least put free and obsolete buildings at the end
		aCityBldgs = sorted(aCityBldgs, key=lambda iBuilding: iBuilding - 2000 * pHeadSelectedCity.getNumActiveBuilding(iBuilding) + 1000 * pHeadSelectedCity.getNumFreeBuilding(iBuilding))
		# </advc.097>
		iTotalBuildingRows = 0
		for iBuilding in aCityBldgs:
			iTotalBuildingRows += pHeadSelectedCity.getNumBuilding(iBuilding)
		screen.setTableNumRows("BuildingListTable", iTotalBuildingRows)
		for iBuilding in aCityBldgs:
			for k in range(pHeadSelectedCity.getNumBuilding(iBuilding)):
				# <advc.097>
				if CityScreenOpt.isBuildings_IconOnly():
					szLeftBuffer = ""
				else:
					szLeftBuffer = gc.getBuildingInfo(iBuilding).getDescription()
				# </advc.097>
				szRightBuffer = u""
				bFirst = True
				if pHeadSelectedCity.getNumActiveBuilding(iBuilding) > 0:
					# K-Mod. I've just swapped the order of health / happiness, to match the DLL hover-text.
					iHappiness = pHeadSelectedCity.getBuildingHappiness(iBuilding)
					if iHappiness != 0:
						if not bFirst:
							szRightBuffer = szRightBuffer + ", "
						else:
							bFirst = False
						if iHappiness > 0:
							szTempBuffer = u"+%d%s" %(iHappiness, self.szHappyIcon)
							szRightBuffer = szRightBuffer + szTempBuffer
						else:
							szTempBuffer = u"+%d%s" %(-iHappiness, self.szUnhappyIcon)
							szRightBuffer = szRightBuffer + szTempBuffer
					iHealth = pHeadSelectedCity.getBuildingHealth(iBuilding)
					if iHealth != 0:
						if not bFirst:
							szRightBuffer = szRightBuffer + ", "
						else:
							bFirst = False
						if iHealth > 0:
							szTempBuffer = u"+%d%s" %(iHealth, self.szHealthyIcon)
							szRightBuffer = szRightBuffer + szTempBuffer
						else:
							szTempBuffer = u"+%d%s" %(-iHealth, self.szUnhealthyIcon)
							szRightBuffer = szRightBuffer + szTempBuffer
					# K-Mod end
					for iYield in range(YieldTypes.NUM_YIELD_TYPES):
						iChange = gc.getBuildingInfo(iBuilding).getYieldChange(iYield)
						iChange += (pHeadSelectedCity.getNumBuilding(iBuilding) * pHeadSelectedCity.getBuildingYieldChange(gc.getBuildingInfo(iBuilding).getBuildingClassType(), iYield))
						if iChange == 0:
							continue
						if (not bFirst):
							szRightBuffer = szRightBuffer + ", "
						else:
							bFirst = False
						if iChange > 0:
							szTempBuffer = u"%s%d%c" %("+", iChange, gc.getYieldInfo(iYield).getChar())
							szRightBuffer = szRightBuffer + szTempBuffer
						else:
							szTempBuffer = u"%s%d%c" %("", iChange, gc.getYieldInfo(iYield).getChar())
							szRightBuffer = szRightBuffer + szTempBuffer
						self.yields.addBuilding(iYield, iChange) # BUG - Raw Yields

				# <advc.097> Gray out names of obsolete buildings
				else:
					szLeftBuffer = u"<color=%d,%d,%d,%d>%s</color>" %(160, 160, 160, 255, szLeftBuffer)
				# </advc.097>
				# <!-- custom: add the +1 great person icon in buildings list as it is handy to have and tedious to check every time, with the help of gemini 3 pro thanks.
				# Update: also show GPP from buildings even if obsolete (e.g., Oracle) to fix this information being missing, since it is accounted by total it seems. See KI#101. (GPT-5.2-Codex) -->
				iGPRate = gc.getBuildingInfo(iBuilding).getGreatPeopleRateChange()
				if iGPRate > 0:
					if not bFirst:
						szRightBuffer = szRightBuffer + ", "
					else:
						bFirst = False
					szRightBuffer = szRightBuffer + u"+%d%s" % (iGPRate, self.szGreatPeopleIcon)
				for iCommerce in range(CommerceTypes.NUM_COMMERCE_TYPES):
					iChange = pHeadSelectedCity.getBuildingCommerceByBuilding(iCommerce, iBuilding)
					iChange /= pHeadSelectedCity.getNumBuilding(iBuilding)
					if iChange == 0:
						continue
					if not bFirst:
						szRightBuffer = szRightBuffer + ", "
					else:
						bFirst = False
					if iChange > 0:
						szTempBuffer = u"%s%d%c" %("+", iChange, gc.getCommerceInfo(iCommerce).getChar())
						szRightBuffer = szRightBuffer + szTempBuffer
					else:
						szTempBuffer = u"%s%d%c" %("", iChange, gc.getCommerceInfo(iCommerce).getChar())
						szRightBuffer = szRightBuffer + szTempBuffer
				#szBuffer = szLeftBuffer + "  " + szRightBuffer # advc: unused
				# <advc.097>
				if CityScreenOpt.isBuildings_TextOnly():
					szIcon = ""
				else:
					szIcon = gc.getBuildingInfo(iBuilding).getButton() # from BAT mod
				# </advc.097>
				szFontStart = self.bldgFontTag # advc.092
				screen.setTableText("BuildingListTable", 0, iNumBuildings, szFontStart + szLeftBuffer + SAS_FONT_TAG_CLOSE, szIcon, WidgetTypes.WIDGET_HELP_BUILDING, iBuilding, -1, CvUtil.FONT_LEFT_JUSTIFY)
				screen.setTableText("BuildingListTable", 1, iNumBuildings, szFontStart + szRightBuffer + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_HELP_BUILDING, iBuilding, -1, CvUtil.FONT_RIGHT_JUSTIFY)
				iNumBuildings = iNumBuildings + 1
		# advc: unused
		#if (iNumBuildings > g_iNumBuildings):
		#	g_iNumBuildings = iNumBuildings
		iNumTradeRoutes = 0
		if self.iMAX_TRADE_ROUTES is None:
			self.iMAX_TRADE_ROUTES = gc.getDefineINT("MAX_TRADE_ROUTES")
			self.IS_USE_KMOD_TRADE_CULTURE = (gc.getDefineINT("USE_KMOD_TRADE_CULTURE") != 0)
		for i in range(self.iMAX_TRADE_ROUTES):
			pLoopCity = pHeadSelectedCity.getTradeCity(i)
			if (pLoopCity and pLoopCity.getOwner() >= 0):
				player = gc.getPlayer(pLoopCity.getOwner())
				szLeftBuffer = u"<color=%d,%d,%d,%d>%s</color>" %(player.getPlayerTextColorR(), player.getPlayerTextColorG(), player.getPlayerTextColorB(), player.getPlayerTextColorA(), pLoopCity.getName())
				szRightBuffer = u""
				for j in range(YieldTypes.NUM_YIELD_TYPES):
# BUG - Fractional Trade - start
					iTradeProfit = TradeUtil.calculateTradeRouteYield(pHeadSelectedCity, i, j)
					if (iTradeProfit != 0):
						if (iTradeProfit > 0):
							if TradeUtil.isFractionalTrade():
								szTempBuffer = u"%s%d.%02d%c" %("+", iTradeProfit // 100, iTradeProfit % 100, gc.getYieldInfo(j).getChar())
							else:
								szTempBuffer = u"%s%d%c" %("+", iTradeProfit, gc.getYieldInfo(j).getChar())
							szRightBuffer = szRightBuffer + szTempBuffer
						else:
							if TradeUtil.isFractionalTrade():
								szTempBuffer = u"%s%d.%02d%c" %("", iTradeProfit // 100, iTradeProfit % 100, gc.getYieldInfo(j).getChar())
							else:
								szTempBuffer = u"%s%d%c" %("", iTradeProfit, gc.getYieldInfo(j).getChar())
							szRightBuffer = szRightBuffer + szTempBuffer
# BUG - Fractional Trade - end
# BUG - Raw Yields - start
						if (j == YieldTypes.YIELD_COMMERCE):
							if pHeadSelectedCity.getTeam() == pLoopCity.getTeam():
								self.yields.addDomesticTrade(iTradeProfit)
							else:
								self.yields.addForeignTrade(iTradeProfit)
# K-Mod: Trade culture
				iTradeCultureTimes100 = pLoopCity.getTradeCultureRateTimes100(pHeadSelectedCity.getCultureLevel())
				# advc.125:
				if (iTradeCultureTimes100 >= 20 and self.IS_USE_KMOD_TRADE_CULTURE):
					szTradeCultureBuffer = u"%s%.1f%c" %("+", iTradeCultureTimes100/100.0, gc.getCommerceInfo(CommerceTypes.COMMERCE_CULTURE).getChar())
					szRightBuffer = szRightBuffer + szTradeCultureBuffer
# K-Mod: Trade culture end
				if not bShowRawYields:
					screen.appendTableRow("TradeRouteTable")
					szFontStart = self.trFontTag # advc.092
					screen.setTableText("TradeRouteTable", 0, iNumTradeRoutes, szFontStart + szLeftBuffer + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_HELP_TRADE_ROUTE_CITY, i, -1, CvUtil.FONT_LEFT_JUSTIFY)
					screen.setTableText("TradeRouteTable", 1, iNumTradeRoutes, szFontStart + szRightBuffer + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_HELP_TRADE_ROUTE_CITY, i, -1, CvUtil.FONT_RIGHT_JUSTIFY)
# BUG - Raw Yields - end
				iNumTradeRoutes = iNumTradeRoutes + 1
		# advc: unused
		#if (iNumTradeRoutes > g_iNumTradeRoutes):
		#	g_iNumTradeRoutes = iNumTradeRoutes
		# <advc.004> Sort the resources by effect, amount, id
		aCityBonuses = []
		for iBonus in range(gc.getNumBonusInfos()):
			aCityBonuses.append(iBonus)
		# </advc.004>
		aCityBonuses = sorted(aCityBonuses, key=lambda iBonus: - 10000 * pHeadSelectedCity.getBonusHappiness(iBonus) - 9000 * pHeadSelectedCity.getBonusHealth(iBonus) - 1000 * pHeadSelectedCity.getNumBonuses(iBonus) + iBonus)
		aCityBonusColumnCounts = [0, 0, 0]
		for iBonus in aCityBonuses:
			if not pHeadSelectedCity.hasBonus(iBonus):
				continue
			iHealth = pHeadSelectedCity.getBonusHealth(iBonus)
			iHappiness = pHeadSelectedCity.getBonusHappiness(iBonus)
			if iHappiness != 0:
				aCityBonusColumnCounts[2] += 1
			elif iHealth != 0:
				aCityBonusColumnCounts[1] += 1
			else:
				aCityBonusColumnCounts[0] += 1
		bUseCityBonusButtons = (self.bCityBonusButtons and self.SAS_CV_MAIN_INTERFACE_CITY_BONUS_ICON_MODE > 0)
		iMaxCityBonusRows = 0
		for iCount in aCityBonusColumnCounts:
			if iCount > iMaxCityBonusRows:
				iMaxCityBonusRows = iCount
		iCityBonusBtnSize = self.iCityBonusPreferredBtnSize
		while iCityBonusBtnSize >= self.iCityBonusMinBtnSize:
			if iMaxCityBonusRows <= self.cityBonusRowsPerColumn(iCityBonusBtnSize):
				break
			iCityBonusBtnSize -= self.iCityBonusShrinkStep
		if iCityBonusBtnSize < self.iCityBonusMinBtnSize:
			bUseCityBonusButtons = False
		if bUseCityBonusButtons:
			self.useCityBonusButtonSize(iCityBonusBtnSize)
		# <advc.092>
		# <!-- custom: The old button fallback used total XML bonus count, which made AdvCiv-SAS fall back to small font icons even when the selected city's actual bonus rows fit. Check the selected city's per-column row counts instead; use the preferred 40px-ish buttons when they fit, shrink in explicit 5px-ish DDS steps only for real selected-city overflow, and keep text glyphs as legacy/debug mode or last-resort overflow fallback. Mode 0 forces at least label-size text so bonus glyphs are not tinier than nearby UI, but does not hardcode font=4 because that also enlarges amount/effect text; testing showed even font=4 text glyphs remain much smaller than image-button rendering, so larger bonus icons need mode 1 or 2. (GPT-5.5) -->
		if (bUseCityBonusButtons or gRect("Top").height() > 900 + max(0, gc.getNumBonusInfos() - 35) * 12) or self.SAS_CV_MAIN_INTERFACE_CITY_BONUS_ICON_MODE == 0:
			szFontStart = sasFontTagLabel
		else:
			szFontStart = sasFontTagTiny # (2 doesn't really increase the size of icons)
		# </advc.092>
		iLeftCount = 0
		iCenterCount = 0
		iRightCount = 0
		for iBonus in aCityBonuses:
			bHandled = False
			if not pHeadSelectedCity.hasBonus(iBonus):
				continue
			iHealth = pHeadSelectedCity.getBonusHealth(iBonus)
			iHappiness = pHeadSelectedCity.getBonusHappiness(iBonus)
			szBuffer = u""
			szLeadBuffer = u""
			szTempBuffer = szFontStart + (u"%c" %(gc.getBonusInfo(iBonus).getChar()))
			szLeadBuffer = szLeadBuffer + szTempBuffer
			iAmount = pHeadSelectedCity.getNumBonuses(iBonus)
			if iAmount > 1:
				szTempBuffer = u"(%d)" %(iAmount)
				szLeadBuffer = szLeadBuffer + szTempBuffer
			szLeadBuffer = szLeadBuffer + SAS_FONT_TAG_CLOSE
			iVSpace = VSPACE(4)
			iRowHeight = BTNSZ(22) # advc.002b: 20 in BtS
			# advc.002b: Removed the plus signs and commas before the amounts of (bad) health and (un-)happiness
			if iHappiness != 0:
				if iHappiness > 0:
					szTempBuffer = szFontStart + (u"%d%s</font>" %(iHappiness, self.szHappyIcon))
				else:
					szTempBuffer = szFontStart + (u"%d%s</font>" %(-iHappiness, self.szUnhappyIcon))
				if iHealth > 0:
					szTempBuffer += szFontStart + (u" %d%s</font>" %(iHealth, self.szHealthyIcon))
				# <advc.092>
				if bUseCityBonusButtons:
					self.fillCityBonusRow(2, iRightCount, iBonus, iAmount, szTempBuffer)
				else: # </advc.092>
					szName = "RightBonusItemLeft" + str(iRightCount)
					screen.setLabelAt(szName, "BonusBack2", szLeadBuffer, CvUtil.FONT_LEFT_JUSTIFY, 0, iRightCount * iRowHeight + iVSpace, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus, -1)
					szName = "RightBonusItemRight" + str(iRightCount)
					screen.setLabelAt(szName, "BonusBack2", szTempBuffer, CvUtil.FONT_RIGHT_JUSTIFY, gRect("BonusPane2").width() - HSPACE(8), iRightCount * iRowHeight + iVSpace, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus, -1)
				iRightCount = iRightCount + 1
				bHandled = True
			if iHealth != 0 and (not bHandled):
				if iHealth > 0:
					szTempBuffer = szFontStart + (u"%d%s</font>" %(iHealth, self.szHealthyIcon))
				else:
					szTempBuffer = szFontStart + (u"%d%s</font>" %(-iHealth, self.szUnhealthyIcon))
				# <advc.092>
				if bUseCityBonusButtons:
					self.fillCityBonusRow(1, iCenterCount, iBonus, iAmount, szTempBuffer)
				else: # </advc.092>
					szName = "CenterBonusItemLeft" + str(iCenterCount)
					screen.setLabelAt(szName, "BonusBack1", szLeadBuffer, CvUtil.FONT_LEFT_JUSTIFY, 0, iCenterCount * iRowHeight + iVSpace, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus, -1)
					szName = "CenterBonusItemRight" + str(iCenterCount)
					screen.setLabelAt(szName, "BonusBack1", szTempBuffer, CvUtil.FONT_RIGHT_JUSTIFY, gRect("BonusPane1").width() - HSPACE(8), iCenterCount * iRowHeight + iVSpace, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus, -1)
				iCenterCount = iCenterCount + 1
				bHandled = True
			szBuffer = u""
			if not bHandled:
				# <advc.092>
				if bUseCityBonusButtons:
					self.fillCityBonusRow(0, iLeftCount, iBonus, iAmount)
				else: # </advc.092>
					szName = "LeftBonusItem" + str(iLeftCount)
					screen.setLabelAt(szName, "BonusBack0", szLeadBuffer, CvUtil.FONT_LEFT_JUSTIFY, 0, (iLeftCount * iRowHeight) + iVSpace, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus, -1)
				iLeftCount = iLeftCount + 1
				bHandled = True
		g_iNumLeftBonus = iLeftCount
		g_iNumCenterBonus = iCenterCount
		g_iNumRightBonus = iRightCount
		iMaintenance = pHeadSelectedCity.getMaintenanceTimes100()
		# <K-Mod>
		iMaintenance *= 100 + gc.getPlayer(pHeadSelectedCity.getOwner()).calculateInflationRate()
		iMaintenance /= 100 # </K-Mod>
		szBuffer = self.szTextMaintenance
		self.setLabel("MaintenanceText", "Background", sasFontTagLabel + szBuffer + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, FontTypes.SMALL_FONT, -0.3, WidgetTypes.WIDGET_HELP_MAINTENANCE)
		screen.show("MaintenanceText")
		szBuffer = u"-%d.%02d %c" %(iMaintenance/100, iMaintenance%100, gc.getCommerceInfo(CommerceTypes.COMMERCE_GOLD).getChar())
		self.setLabel("MaintenanceAmountText", "Background", sasFontTagLabel + szBuffer + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, FontTypes.SMALL_FONT, -0.3, WidgetTypes.WIDGET_HELP_MAINTENANCE)
		screen.show("MaintenanceAmountText")
# BUG - Raw Yields - start
		if bShowRawYields:
			self.yields.processCity(pHeadSelectedCity)
			self.yields.fillTable(screen, "TradeRouteTable", g_iYieldType, g_iYieldTiles)
# BUG - Raw Yields - end
		szBuffer = u""
		# <advc.092>
		if self.bScaleHUD:
			iReligionCorpsMargin = VSPACE(4)
		else:
			iReligionCorpsMargin = 0
		# </advc.092>
# BUG - Limit/Extra Religions - start
		# <!-- custom: religion art is clipped at top and bottom edges for some reason, but reducing icon size does not improve it; corporation icons seem unaffected because they are square.
		# Keep the larger icon size for readability even though it makes religion clipping more visible. (GPT-5.5) -->
		iOrgIconSize = BTNSZ(25)
		iOrgSpacing = HSPACE(2)
		iOrgMargin = HSPACE(6)
		if CityScreenOpt.isShowOnlyPresentReligions():
			aReligions = ReligionUtil.getCityReligions(pHeadSelectedCity)
			if len(aReligions) > 0:
				lReligions = RowLayout(gRect("CityNameBackground"), iOrgMargin, RectLayout.CENTER, len(aReligions), iOrgSpacing, iOrgIconSize)
				gSetRectangle("CityReligions", lReligions)
			else:
				lReligions = None
			for i in range(len(aReligions)):
				iReligion = aReligions[i]
				szButton = gc.getReligionInfo(iReligion).getButton()
				szName = "ReligionDDS" + str(iReligion)
				lReligion = lReligions.next()
				gSetRectangle(szName, lReligion)
				self.setImageButton(szName, szButton, WidgetTypes.WIDGET_HELP_RELIGION_CITY, iReligion)
				screen.enable(szName, True)
				screen.show(szName)
				# Holy City Overlay
				if pHeadSelectedCity.isHolyCityByType(iReligion):
					szName = "ReligionHolyCityDDS" + str(iReligion)
					gSetRectangle(szName, lReligion)
					self.addDDS(szName, "INTERFACE_HOLYCITY_OVERLAY", WidgetTypes.WIDGET_HELP_RELIGION_CITY, iReligion)
					# <!-- custom: make holy-city overlay visual-only so it does not block left-click Sevopedia routing on the underlying religion icon; before this, right-click worked on yellow icons but left-click did not, and in-game testing confirmed this fixes left-click. (GPT-5.5) -->
					screen.setHitTest(szName, HitTestTypes.HITTEST_NOHIT)
					screen.show(szName)
		else: # (show all religions)
			iReligions = gc.getNumReligionInfos()
			if iReligions > 0:
				lReligions = RowLayout(gRect("CityNameBackground"), iOrgMargin, RectLayout.CENTER, iReligions, iOrgSpacing, iOrgIconSize)
				gSetRectangle("CityReligions", lReligions)
			else:
				lReligions = None
			for iReligion in range(gc.getNumReligionInfos()):
				lReligion = lReligions.next()
				bEnable = True
				if (pHeadSelectedCity.isHasReligion(iReligion)):
					# <advc>
					if (pHeadSelectedCity.isHolyCityByType(iReligion)):
						#szTempBuffer = u"%c" %(gc.getReligionInfo(iReligion).getHolyCityChar())
						szName = "ReligionHolyCityDDS" + str(iReligion)
						screen.show(szName)
					# </advc> szBuffer code commented out (see advc comment above)
					#
					# else:
					# 	szTempBuffer = u"%c" %(gc.getReligionInfo(iReligion).getChar())
					# szBuffer = szBuffer + szTempBuffer
					# for iCommerce in range(CommerceTypes.NUM_COMMERCE_TYPES):
					# 	iRate = pHeadSelectedCity.getReligionCommerceByReligion(iCommerce, iReligion)
					# 	if iRate != 0:
					# 		if iRate > 0:
					# 			szTempBuffer = u",%s%d%c" %("+", iRate, gc.getCommerceInfo(iCommerce).getChar())
					# 			szBuffer = szBuffer + szTempBuffer
					# 		else:
					# 			szTempBuffer = u",%s%d%c" %("", iRate, gc.getCommerceInfo(iCommerce).getChar())
					# 			szBuffer = szBuffer + szTempBuffer
					# iHappiness = pHeadSelectedCity.getReligionHappiness(iReligion)
					# if (iHappiness != 0):
					# 	if (iHappiness > 0):
					# 		szTempBuffer = u",+%d%c" %(iHappiness, CyGame().getSymbolID(FontSymbols.HAPPY_CHAR))
					# 		szBuffer = szBuffer + szTempBuffer
					# 	else:
					# 		szTempBuffer = u",+%d%c" %(-(iHappiness), CyGame().getSymbolID(FontSymbols.UNHAPPY_CHAR))
					# 		szBuffer = szBuffer + szTempBuffer
					# szBuffer = szBuffer + " "
					#
					szButton = gc.getReligionInfo(iReligion).getButton()
				else:
					bEnable = False
					szButton = gc.getReligionInfo(iReligion).getButton()
				szName = "ReligionDDS" + str(iReligion)
				gSetRectangle(szName, lReligion)
				self.setImageButton(szName, szButton, WidgetTypes.WIDGET_HELP_RELIGION_CITY, iReligion)
				screen.enable(szName, bEnable)
				screen.show(szName)
				if (pHeadSelectedCity.isHolyCityByType(iReligion)):
					szName = "ReligionHolyCityDDS" + str(iReligion)
					gSetRectangle(szName, lReligion)
					self.addDDS(szName, "INTERFACE_HOLYCITY_OVERLAY", WidgetTypes.WIDGET_HELP_RELIGION_CITY, iReligion)
					# <!-- custom: make holy-city overlay visual-only so it does not block left-click Sevopedia routing on the underlying religion icon; before this, right-click worked on yellow icons but left-click did not, and in-game testing confirmed this fixes left-click. (GPT-5.5) -->
					screen.setHitTest(szName, HitTestTypes.HITTEST_NOHIT)
					screen.show(szName)
# BUG - Limit/Extra Religions - end
# BUG - Limit/Extra Corporations - start
		#if CityScreenOpt.isShowOnlyPresentCorporations():
		# advc.004: Now controlled by a single option
		if CityScreenOpt.isShowOnlyPresentReligions():
			aCorporations = []
			for iCorp in range(gc.getNumCorporationInfos()):
				if not pHeadSelectedCity.isHasCorporation(iCorp):
					continue
				aCorporations += [iCorp]
			if len(aCorporations) > 0:
				lCorporations = RowLayout(gRect("CityNameBackground"), RectLayout.RIGHT, RectLayout.CENTER, len(aCorporations), iOrgSpacing, iOrgIconSize)
				lCorporations.move(-iOrgMargin, 0)
			else:
				lCorporations = None
			for i in range(len(aCorporations)):
				iCorp = aCorporations[i]
				# advc: szBuffer code commented out (see advc comment above)
				#
				# if pHeadSelectedCity.isHeadquartersByType(iCorp):
				# 	szTempBuffer = u"%c" %(gc.getCorporationInfo(iCorp).getHeadquarterChar())
				# 	szName = "CorporationHeadquarterDDS" + str(iCorp)
				# 	screen.show(szName)
				# else:
				# 	szTempBuffer = u"%c" %(gc.getCorporationInfo(iCorp).getChar())
				# szBuffer = szBuffer + szTempBuffer
				# for iYield in range(YieldTypes.NUM_YIELD_TYPES):
				# 	iRate = pHeadSelectedCity.getCorporationYieldByCorporation(iYield, iCorp)
				# 	if iRate != 0:
				# 		if iRate > 0:
				# 			szTempBuffer = u",%s%d%c" %("+", iRate, gc.getYieldInfo(iYield).getChar())
				# 			szBuffer = szBuffer + szTempBuffer
				# 		else:
				# 			szTempBuffer = u",%s%d%c" %("", iRate, gc.getYieldInfo(iYield).getChar())
				# 			szBuffer = szBuffer + szTempBuffer
				# for iCommerce in range(CommerceTypes.NUM_COMMERCE_TYPES):
				# 	iRate = pHeadSelectedCity.getCorporationCommerceByCorporation(iCommerce, iCorp)
				# 	if iRate != 0:
				# 		if iRate > 0:
				# 			szTempBuffer = u",%s%d%c" %("+", iRate, gc.getCommerceInfo(iCommerce).getChar())
				# 			szBuffer = szBuffer + szTempBuffer
				# 		else:
				# 			szTempBuffer = u",%s%d%c" %("", iRate, gc.getCommerceInfo(iCommerce).getChar())
				# 			szBuffer = szBuffer + szTempBuffer
				# szBuffer += " "
				#
				szButton = gc.getCorporationInfo(iCorp).getButton()
				szName = "CorporationDDS" + str(iCorp)
				lCorp = lCorporations.next()
				gSetRectangle(szName, lCorp)
				self.setImageButton(szName, szButton, WidgetTypes.WIDGET_HELP_CORPORATION_CITY, iCorp)
				screen.enable(szName, True)
				screen.show(szName)
				# HQ Overlay
				if pHeadSelectedCity.isHeadquartersByType(iCorp):
					szName = "CorporationHeadquarterDDS" + str(iCorp)
					gSetRectangle(szName, lCorp)
					self.addDDS(szName, "INTERFACE_HOLYCITY_OVERLAY", WidgetTypes.WIDGET_HELP_CORPORATION_CITY, iCorp)
					# <!-- custom: make headquarters overlay visual-only so it does not block left-click Sevopedia routing on the underlying corporation icon; before this, right-click worked on yellow icons but left-click did not, and in-game testing confirmed this fixes left-click. (GPT-5.5) -->
					screen.setHitTest(szName, HitTestTypes.HITTEST_NOHIT)
					screen.show(szName)
		else: # (show all corps)
			iCorporations = gc.getNumCorporationInfos()
			if iCorporations > 0:
				lCorporations = RowLayout(gRect("CityNameBackground"), RectLayout.RIGHT, RectLayout.CENTER, iCorporations, iOrgSpacing, iOrgIconSize)
				lCorporations.move(-iOrgMargin, 0)
			else:
				lCorporations = None
			for iCorp in range(gc.getNumCorporationInfos()):
				lCorp = lCorporations.next()
				bEnable = True
				if pHeadSelectedCity.isHasCorporation(iCorp):
					# <advc>
					if pHeadSelectedCity.isHeadquartersByType(iCorp):
						#szTempBuffer = u"%c" %(gc.getCorporationInfo(iCorp).getHeadquarterChar())
						szName = "CorporationHeadquarterDDS" + str(iCorp)
						screen.show(szName)
					# </advc> szBuffer code commented out (see advc comment above)
					#
					# else:
					# 	szTempBuffer = u"%c" %(gc.getCorporationInfo(iCorp).getChar())
					# szBuffer = szBuffer + szTempBuffer
					# for iYield in range(YieldTypes.NUM_YIELD_TYPES):
					# 	iRate = pHeadSelectedCity.getCorporationYieldByCorporation(iYield, iCorp)
					# 	if iRate != 0:
					# 		if iRate > 0:
					# 			szTempBuffer = u",%s%d%c" %("+", iRate, gc.getYieldInfo(iYield).getChar())
					# 			szBuffer = szBuffer + szTempBuffer
					# 		else:
					# 			szTempBuffer = u",%s%d%c" %("", iRate, gc.getYieldInfo(iYield).getChar())
					# 			szBuffer = szBuffer + szTempBuffer
					# for iCommerce in range(CommerceTypes.NUM_COMMERCE_TYPES):
					# 	iRate = pHeadSelectedCity.getCorporationCommerceByCorporation(iCommerce, iCorp)
					# 	if iRate != 0:
					# 		if iRate > 0:
					# 			szTempBuffer = u",%s%d%c" %("+", iRate, gc.getCommerceInfo(iCommerce).getChar())
					# 			szBuffer = szBuffer + szTempBuffer
					# 		else:
					# 			szTempBuffer = u",%s%d%c" %("", iRate, gc.getCommerceInfo(iCommerce).getChar())
					# 			szBuffer = szBuffer + szTempBuffer
					# szBuffer += " "
					#
					szButton = gc.getCorporationInfo(iCorp).getButton()
				else:
					bEnable = False
					szButton = gc.getCorporationInfo(iCorp).getButton()
				szName = "CorporationDDS" + str(iCorp)
				gSetRectangle(szName, lCorp)
				self.setImageButton(szName, szButton, WidgetTypes.WIDGET_HELP_CORPORATION_CITY, iCorp)
				screen.enable(szName, bEnable)
				screen.show(szName)
				if pHeadSelectedCity.isHeadquartersByType(iCorp):
					szName = "CorporationHeadquarterDDS" + str(iCorp)
					gSetRectangle(szName, lCorp)
					self.addDDS(szName, "INTERFACE_HOLYCITY_OVERLAY", WidgetTypes.WIDGET_HELP_CORPORATION_CITY, iCorp)
					# <!-- custom: make headquarters overlay visual-only so it does not block left-click Sevopedia routing on the underlying corporation icon; before this, right-click worked on yellow icons but left-click did not, and in-game testing confirmed this fixes left-click. (GPT-5.5) -->
					screen.setHitTest(szName, HitTestTypes.HITTEST_NOHIT)
					screen.show(szName)
# BUG - Limit/Extra Corporations - end
		szBuffer = u"%d%% %s" %(pHeadSelectedCity.plot().calculateCulturePercent(pHeadSelectedCity.getOwner()), gc.getPlayer(pHeadSelectedCity.getOwner()).getCivilizationAdjective(0))
		self.setLabel("NationalityText", "Background", sasFontTagLabel + szBuffer + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, FontTypes.SMALL_FONT, -0.3)
		screen.setHitTest("NationalityText", HitTestTypes.HITTEST_NOHIT)
		screen.show("NationalityText")
		iRemainder = 100
		iWhichBar = 0
		players = list()
		# <advc.099> Replaced "Alive" with "EverAlive"
		# Moreover, it turns out that setStackedBarColorsRGB will stack at most 4 bars, and that happens outside the SDK.
		# The best one can do is to start with the longest bars.
		for h in range(gc.getMAX_PLAYERS()):
			if not gc.getPlayer(h).isEverAlive():
				continue
			iPercent = pHeadSelectedCity.plot().calculateCulturePercent(h)
			if iPercent <= 0:
				continue
			players.append((h, iPercent))
		players = sorted(players, key=lambda x: x[1], reverse=True)
		for (h, iPercent) in players: # </advc.099>
			screen.setStackedBarColorsRGB("NationalityBar", iWhichBar, gc.getPlayer(h).getPlayerTextColorR(), gc.getPlayer(h).getPlayerTextColorG(), gc.getPlayer(h).getPlayerTextColorB(), gc.getPlayer(h).getPlayerTextColorA())
			if (iRemainder <= 0):
				screen.setBarPercentage("NationalityBar", iWhichBar, 0.0)
			else:
				screen.setBarPercentage("NationalityBar", iWhichBar, float(iPercent) / iRemainder)
			iRemainder -= iPercent
			iWhichBar += 1
		screen.show("NationalityBar")
		# Defense text now appended to CityNameText to avoid overlapping religions/corps.
		# advc.001: Left side was missing empty parentheses
		if (pHeadSelectedCity.getCultureLevel() != CultureLevelTypes.NO_CULTURELEVEL):
			iRate = pHeadSelectedCity.getCommerceRateTimes100(CommerceTypes.COMMERCE_CULTURE)
			if (iRate%100 == 0):
				szBuffer = localText.getText("INTERFACE_CITY_COMMERCE_RATE", (gc.getCommerceInfo(CommerceTypes.COMMERCE_CULTURE).getChar(), gc.getCultureLevelInfo(pHeadSelectedCity.getCultureLevel()).getTextKey(), iRate/100))
			else:
				szRate = u"+%d.%02d" % (iRate/100, iRate%100)
				szBuffer = localText.getText("INTERFACE_CITY_COMMERCE_RATE_FLOAT", (gc.getCommerceInfo(CommerceTypes.COMMERCE_CULTURE).getChar(), gc.getCultureLevelInfo(pHeadSelectedCity.getCultureLevel()).getTextKey(), szRate))
# BUG - Culture Turns - start
			#if CityScreenOpt.isShowCultureTurns() and iRate > 0:
			if iRate > 0: # advc.065: No longer optional
				iCultureTimes100 = pHeadSelectedCity.getCultureTimes100(pHeadSelectedCity.getOwner())
				iCultureLeftTimes100 = 100 * pHeadSelectedCity.getCultureThreshold() - iCultureTimes100
				# K-Mod: (don't display a negative countdown when we pass legendary culture)
				if iCultureLeftTimes100 > 0:
					szBuffer += u" " + localText.getText("INTERFACE_CITY_TURNS", (((iCultureLeftTimes100 + iRate - 1) / iRate),))
# BUG - Culture Turns - end
			self.setLabel("CultureText", "Background", sasFontTagLabel + szBuffer + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, FontTypes.GAME_FONT, -1.3)
			screen.setHitTest("CultureText", HitTestTypes.HITTEST_NOHIT)
			screen.show("CultureText")
		if (pHeadSelectedCity.getGreatPeopleProgress() > 0 or pHeadSelectedCity.getGreatPeopleRate() > 0):
# BUG - Great Person Turns - start
			iRate = pHeadSelectedCity.getGreatPeopleRate()
			if CityScreenOpt.isShowCityGreatPersonInfo():
				iGPTurns = GPUtil.getCityTurns(pHeadSelectedCity)
				# <!-- custom: Base AdvCiv's scalable HUD made the city GP bar runtime-sized but retained K-Mod/BUG's fixed 230px text budget, prematurely hiding a fitting GP type at wider resolutions.
				# Pass the actual bar width like the map GP bar. See KI#237.2. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				szBuffer = GPUtil.getGreatPeopleText(pHeadSelectedCity, iGPTurns, gRect("GreatPeopleBar").width(), MainOpt.isGPBarTypesNone(), MainOpt.isGPBarTypesOne(), False)
			else:
				szBuffer = localText.getText("INTERFACE_CITY_GREATPEOPLE_RATE", (self.iGreatPeopleIcon, pHeadSelectedCity.getGreatPeopleRate()))
				if CityScreenOpt.isShowGreatPersonTurns() and iRate > 0:
					iGPTurns = GPUtil.getCityTurns(pHeadSelectedCity)
					szBuffer += u" " + localText.getText("INTERFACE_CITY_TURNS", (iGPTurns,))
# BUG - Great Person Turns - end
			self.setLabel("GreatPeopleText", "Background", sasFontTagLabel + szBuffer + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, FontTypes.GAME_FONT, -1.3)
			screen.setHitTest("GreatPeopleText", HitTestTypes.HITTEST_NOHIT)
			screen.show("GreatPeopleText")
			# <advc> Refactored a bit
			fFirst = float(pHeadSelectedCity.getGreatPeopleProgress())
			fFirst /= gc.getPlayer(pHeadSelectedCity.getOwner()).greatPeopleThreshold(False)
			screen.setBarPercentage("GreatPeopleBar", InfoBarTypes.INFOBAR_STORED, fFirst)
			fProgress = float(pHeadSelectedCity.getGreatPeopleRate())
			fProgress /= gc.getPlayer(pHeadSelectedCity.getOwner()).greatPeopleThreshold(False)
			if fFirst < 1:
				fProgress /= (1 - fFirst)
			screen.setBarPercentage("GreatPeopleBar", InfoBarTypes.INFOBAR_RATE, fProgress)
			screen.show("GreatPeopleBar")
		fFirst = float(pHeadSelectedCity.getCultureTimes100(pHeadSelectedCity.getOwner()))
		fFirst /= 100 * pHeadSelectedCity.getCultureThreshold()
		screen.setBarPercentage("CultureBar", InfoBarTypes.INFOBAR_STORED, fFirst)
		fProgress = float(pHeadSelectedCity.getCommerceRate(CommerceTypes.COMMERCE_CULTURE))
		fProgress /= pHeadSelectedCity.getCultureThreshold()
		if fFirst < 1:
			fProgress /= (1 - fFirst) # </advc>
		screen.setBarPercentage("CultureBar", InfoBarTypes.INFOBAR_RATE, fProgress)
		screen.show("CultureBar")

		# <!-- custom: add a new specialist breakdown.
		# We have removed a few elements and moved up the specialists panel, so now we can use this space for that.
		# Code added with the help of gemini 3 pro thanks -->
		# To add the Specialist Breakdown in the empty space you created on the bottom right (below the specialists), we will add a block of code to updateCityScreen.
		# This code will:
		# 	- Calculate the GP points coming from Buildings vs Specialists.
		# 	- Create a detailed string with icons (e.g., Hammers for Engineers, Beakers for Scientists).
		# 	- Place this text in the bottom-right corner of the city pane.
		# Step 1: Add the Logic to CvMainInterface.py
		# --- START: Specialist Breakdown Widget (2-Row / Split Labels Fix) ---
		if pHeadSelectedCity:

			# 1. Calculate Raw Values (Buildings vs Specialists)
			iBldgRaw = 0
			for iBuilding in range(gc.getNumBuildingInfos()):
				if pHeadSelectedCity.getNumBuilding(iBuilding) > 0:
					iRate = gc.getBuildingInfo(iBuilding).getGreatPeopleRateChange()
					if iRate > 0:
						iBldgRaw += (iRate * pHeadSelectedCity.getNumBuilding(iBuilding))

			iSpecRaw = 0
			for iSpecialist in range(gc.getNumSpecialistInfos()):
				iCount = pHeadSelectedCity.getSpecialistCount(iSpecialist) + pHeadSelectedCity.getFreeSpecialistCount(iSpecialist)
				if iCount > 0:
					iRate = gc.getSpecialistInfo(iSpecialist).getGreatPeopleRateChange()
					iSpecRaw += (iCount * iRate)

			# 2. Get Totals & Progress
			iTotalRate = pHeadSelectedCity.getGreatPeopleRate()

			# <!-- custom: Before KI#308, this breakdown reverse-engineered its modifier from the already integer-truncated final GPP rate.
			# It displayed +33% for 3 raw GPP at +50%, and omitted +25% when 3 raw GPP still rounded to 3.
			# CyCity exposes the exact total modifier, including Traits and Golden Ages; using it directly fixed both cases. See KI#308. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			iModPercent = pHeadSelectedCity.getTotalGreatPeopleRateModifier() - 100

			iProgress = pHeadSelectedCity.getGreatPeopleProgress()
			pPlayer = gc.getPlayer(pHeadSelectedCity.getOwner())
			iThreshold = pPlayer.greatPeopleThreshold(False)

			# Calculate Turns Remaining
			if iTotalRate > 0:
				iTurns = (iThreshold - iProgress + iTotalRate - 1) / iTotalRate
				szTurns = u"(%d)" % iTurns
			else:
				szTurns = u"(-)"

			# 3. Construct ROW 1 (Top Line): "(5 [Map] 3 [Cit]) +25% [GP]"
			szRow1 = sasFontTagLabel + u"(%d%s %d%s)" % (iBldgRaw, self.szMapIcon, iSpecRaw, self.szCitizenIcon)
			if iModPercent != 0:
				szRow1 += u" %+d%%" % (iModPercent)
			szRow1 += SAS_FONT_TAG_CLOSE

			# 4. Construct ROW 2 (Bottom Line): "10 [GP]: 109/249 (14)"
			szRow2 = sasFontTagLabel + u"%d%s: %d/%d %s" % (iTotalRate, self.szGreatPeopleIcon, iProgress, iThreshold, szTurns) + SAS_FONT_TAG_CLOSE
			# <!-- custom: The compact glyph-only breakdown is hard to interpret without already knowing its legend.
			# Give both rendered rows the same live hover with matching glyphs, named sources, colored key results, modifier, rate, progress and turn estimate. (GPT-5.6-Sol) -->
			szSpecHoverTitle = localText.changeTextColor(u"Great Person Points breakdown", self.colorYellow)
			szSpecHoverModifier = localText.changeTextColor(u"%+d%%" % iModPercent, self.colorGreen)
			szSpecHoverRate = localText.changeTextColor(u"%d%s" % (iTotalRate, self.szGreatPeopleIcon), self.colorGreen)
			# <!-- custom: Keep the exact progress fraction and add an at-a-glance percentage rounded to the nearest hundredth; fixed one-decimal formatting would discard useful precision. (GPT-5.6-Sol) -->
			iSpecProgressPercentTimes100 = (iProgress * 10000 + iThreshold / 2) / iThreshold
			szSpecHoverProgress = localText.changeTextColor(u"%d/%d (%s%%)" % (iProgress, iThreshold, formatExactHundredths(iSpecProgressPercentTimes100)), self.colorYellow)
			szSpecHoverSources = u"Buildings: %d%s\nSpecialists: %d%s" % (iBldgRaw, self.szMapIcon, iSpecRaw, self.szCitizenIcon)
			szSpecHoverResult = u"Modifier: %s\nRate: %s per turn\nProgress: %s\nTurns: %s" % (szSpecHoverModifier, szSpecHoverRate, szSpecHoverProgress, szTurns.strip(u"()"))
			szSpecialistBreakdownHelp = u"%s\n%s\n%s" % (szSpecHoverTitle, szSpecHoverSources, szSpecHoverResult)

			# 5. Positioning & Rendering (Split into TWO Labels)
			# screen.setLabel ignores \n, so we must draw two separate text widgets.
			iX = gRect("CityRightPanelContents").x() + 2

			# Position specialist rows above culture rows
			iY2 = gRect("GreatPeopleBar").y() - 62
			iY1 = gRect("GreatPeopleBar").y() - 82

			# Draw Label 1 (Top)
			screen.setLabel("SpecBreakdownLabel1", "Background", szRow1, CvUtil.FONT_LEFT_JUSTIFY, iX, iY1, -0.1, FontTypes.SMALL_FONT, WIDGET_CITY_BREAKDOWNS, -1, -1)

			# Draw Label 2 (Bottom)
			screen.setLabel("SpecBreakdownLabel2", "Background", szRow2, CvUtil.FONT_LEFT_JUSTIFY, iX, iY2, -0.1, FontTypes.SMALL_FONT, WIDGET_CITY_BREAKDOWNS, -1, -1)
			# --- END: Specialist Breakdown Widget ---

			# <!-- custom: Culture breakdown summary (per-source base culture and total/turns), placed in the right half below specialists to mirror the specialist breakdown while keeping all other panels unchanged. (GPT-5.2-Codex) -->
			if pHeadSelectedCity:
				if hasattr(pHeadSelectedCity, "getReligionCommerce"):
					iRelCulture = pHeadSelectedCity.getReligionCommerce(CommerceTypes.COMMERCE_CULTURE)
				else:
					iRelCulture = 0
					for iReligion in range(gc.getNumReligionInfos()):
						iRelCulture += pHeadSelectedCity.getReligionCommerceByReligion(CommerceTypes.COMMERCE_CULTURE, iReligion)
				if hasattr(pHeadSelectedCity, "getCorporationCommerce"):
					iCorpCulture = pHeadSelectedCity.getCorporationCommerce(CommerceTypes.COMMERCE_CULTURE)
				else:
					iCorpCulture = 0
					for iCorp in range(gc.getNumCorporationInfos()):
						iCorpCulture += pHeadSelectedCity.getCorporationCommerceByCorporation(CommerceTypes.COMMERCE_CULTURE, iCorp)
				iBldgCulture = 0
				for iBuilding in range(gc.getNumBuildingInfos()):
					if pHeadSelectedCity.getNumBuilding(iBuilding) > 0:
						iBldgCulture += pHeadSelectedCity.getBuildingCommerceByBuilding(CommerceTypes.COMMERCE_CULTURE, iBuilding)
				iTraitCulture = 0
				for iTrait in range(gc.getNumTraitInfos()):
					if pPlayer.hasTrait(iTrait):
						iTraitCulture += gc.getTraitInfo(iTrait).getCommerceChange(CommerceTypes.COMMERCE_CULTURE)
				iSpecCulture = 0
				for iSpecialist in range(gc.getNumSpecialistInfos()):
					iCount = (pHeadSelectedCity.getSpecialistCount(iSpecialist) + pHeadSelectedCity.getFreeSpecialistCount(iSpecialist))
					if iCount > 0:
						iSpecCulture += (gc.getSpecialistInfo(iSpecialist).getCommerceChange(CommerceTypes.COMMERCE_CULTURE) * iCount)
				# Add civic/tech per-specialist extras (e.g. civics/techs) if exposed in Python.
				iSpecExtraPer = 0
				if hasattr(pPlayer, "getSpecialistExtraCommerce"):
					iSpecExtraPer = pPlayer.getSpecialistExtraCommerce(CommerceTypes.COMMERCE_CULTURE)
				if iSpecExtraPer != 0:
					iSpecPop = pHeadSelectedCity.getSpecialistPopulation()
					iSpecGreat = pHeadSelectedCity.getNumGreatPeople()
					iSpecCulture += iSpecExtraPer * (iSpecPop + iSpecGreat)
				# <!-- custom: Complete the SAS Culture Breakdown from the same exact native partition as CvCity::updateCommerce: slider commerce joins the base sources, the modifier applies only to that base, Build Culture is added afterward, and No Espionage contributes the remaining transferred Espionage rate. See KI#1063. (GPT-5.6-Sol) -->
				iSliderCultureTimes100 = pHeadSelectedCity.getCommerceFromPercent(CommerceTypes.COMMERCE_CULTURE, pHeadSelectedCity.getYieldRate(YieldTypes.YIELD_COMMERCE) * 100)
				iBaseCultureTimes100 = pHeadSelectedCity.getBaseCommerceRateTimes100(CommerceTypes.COMMERCE_CULTURE)
				iTotalCultureRateTimes100 = pHeadSelectedCity.getCommerceRateTimes100(CommerceTypes.COMMERCE_CULTURE)
				# <!-- custom: Before KI#308.2, the Culture Breakdown inferred its modifier by dividing the precise final rate by getBaseCommerceRate(), which had already truncated fractional slider commerce.
				# CyCity exposes the exact combined commerce modifier; using it directly prevents the displayed percentage from depending on that rounding. See KI#308.2. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				iModPercent = pHeadSelectedCity.getTotalCommerceRateModifier(CommerceTypes.COMMERCE_CULTURE) - 100
				if pHeadSelectedCity.isDisorder():
					iModifiedBaseCultureTimes100 = 0
					iProcessCultureTimes100 = 0
				else:
					iModifiedBaseCultureTimes100 = iBaseCultureTimes100 * (iModPercent + 100) / 100
					iProcessCultureTimes100 = pHeadSelectedCity.getYieldRate(YieldTypes.YIELD_PRODUCTION) * pHeadSelectedCity.getProductionToCommerceModifier(CommerceTypes.COMMERCE_CULTURE)
				iEspionageTransferTimes100 = 0
				if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_NO_ESPIONAGE):
					iEspionageTransferTimes100 = iTotalCultureRateTimes100 - iModifiedBaseCultureTimes100 - iProcessCultureTimes100

				# <!-- custom: Match the adjacent Great Person legend by using MAP_CHAR for building-derived points, leaving the Production glyph to identify Build Culture unambiguously. See KI#1063. (GPT-5.6-Sol) -->
				szRow1 = sasFontTagLabel + u"(%s%s %d%s %d%s %d%s %d%s %d%s)" % (formatExactHundredths(iSliderCultureTimes100), self.szCultureIcon, iRelCulture, self.szReligionIcon, iCorpCulture, self.szTradeIcon, iBldgCulture, self.szMapIcon, iTraitCulture, self.szStarIcon, iSpecCulture, self.szCitizenIcon)
				if iModPercent != 0:
					szRow1 += u" %+d%%" % (iModPercent)
				szRow1 += SAS_FONT_TAG_CLOSE

				iCultureProgressTimes100 = pHeadSelectedCity.getCultureTimes100(pHeadSelectedCity.getOwner())
				iCultureThreshold = pHeadSelectedCity.getCultureThreshold()
				iCultureThresholdTimes100 = 100 * iCultureThreshold
				if iTotalCultureRateTimes100 > 0 and iCultureProgressTimes100 < iCultureThresholdTimes100:
					iTurns = ((iCultureThresholdTimes100 - iCultureProgressTimes100 + iTotalCultureRateTimes100 - 1) / iTotalCultureRateTimes100)
					szTurns = u"(%d)" % iTurns
				else:
					szTurns = u"(-)"
				szRate = formatExactHundredths(iTotalCultureRateTimes100)
				szProgress = formatExactHundredths(iCultureProgressTimes100)
				# <!-- custom: The Culture modifier above applies only to the upper-row base sources.
				# Placing both post-modifier additions on this roomier lower row mirrors the native formula and keeps the legend stable: Build Culture is added afterward, while No Espionage transfers an already-calculated Espionage rate without applying the Culture modifier again. See KI#1063. (GPT-5.6-Sol) -->
				# <!-- custom: When label text is upscaled above its base font size 3 at widths below the usual 2560px of 1440p, remove only spacing/punctuation from this longer row; retain every value, and keep the complete named breakdown in the combined hover. (GPT-5.6-Sol) -->
				if xResolution < 2560 and getSASUIFontLabel() > 3:
					szRow2 = sasFontTagLabel + u"%s%s%s%s|%s%s:%s/%d%s" % (formatExactHundredths(iProcessCultureTimes100), self.szProductionIcon, formatExactHundredths(iEspionageTransferTimes100), self.szEspionageIcon, szRate, self.szCultureIcon, szProgress, iCultureThreshold, szTurns) + SAS_FONT_TAG_CLOSE
				else:
					szPostModifierSources = u"+%s%s +%s%s = " % (formatExactHundredths(iProcessCultureTimes100), self.szProductionIcon, formatExactHundredths(iEspionageTransferTimes100), self.szEspionageIcon)
					szRow2 = sasFontTagLabel + u"%s%s%s: %s/%d %s" % (szPostModifierSources, szRate, self.szCultureIcon, szProgress, iCultureThreshold, szTurns) + SAS_FONT_TAG_CLOSE
				# <!-- custom: Name every compact Culture glyph in a live hover, reuse those glyphs for direct visual matching, and preserve the native calculation order: base sources, Culture modifier, then Build Culture and No Espionage's transferred Espionage. See KI#1063. (GPT-5.6-Sol) -->
				szCultureHoverTitle = localText.changeTextColor(u"Culture breakdown", self.colorYellow)
				szCultureHoverModifier = localText.changeTextColor(u"%+d%%" % iModPercent, self.colorGreen)
				szCultureHoverRate = localText.changeTextColor(u"%s%s" % (szRate, self.szCultureIcon), self.colorGreen)
				iCultureProgressPercentTimes100 = (iCultureProgressTimes100 * 100 + iCultureThreshold / 2) / iCultureThreshold
				szCultureHoverProgress = localText.changeTextColor(u"%s/%d (%s%%)" % (szProgress, iCultureThreshold, formatExactHundredths(iCultureProgressPercentTimes100)), self.colorYellow)
				szCultureHoverSources = u"Slider: %s%s\nReligions: %d%s\nCorporations: %d%s\nBuildings: %d%s\nTraits: %d%s\nSpecialists: %d%s" % (formatExactHundredths(iSliderCultureTimes100), self.szCultureIcon, iRelCulture, self.szReligionIcon, iCorpCulture, self.szTradeIcon, iBldgCulture, self.szMapIcon, iTraitCulture, self.szStarIcon, iSpecCulture, self.szCitizenIcon)
				szCultureHoverPostModifier = u"Build Culture: %s%s\nNo Espionage transfer: %s%s" % (formatExactHundredths(iProcessCultureTimes100), self.szProductionIcon, formatExactHundredths(iEspionageTransferTimes100), self.szEspionageIcon)
				szCultureHoverResult = u"Modifier: %s\n%s\nRate: %s per turn\nProgress: %s\nTurns: %s" % (szCultureHoverModifier, szCultureHoverPostModifier, szCultureHoverRate, szCultureHoverProgress, szTurns.strip(u"()"))
				szCultureBreakdownHelp = u"%s\n%s\n%s" % (szCultureHoverTitle, szCultureHoverSources, szCultureHoverResult)
				# <!-- custom: Hovering either compact breakdown shows both related city-output summaries, avoiding mouse movement (aims to reduce user tedium) between adjacent rows while retaining separate readable sections. (GPT-5.6-Sol) -->
				g_szCityBreakdownsHelp = sasFontTagHover + szSpecialistBreakdownHelp + u"\n\n" + szCultureBreakdownHelp + SAS_FONT_TAG_CLOSE

				iXRight = iX
				iY2 = gRect("GreatPeopleBar").y() - 22
				iY1 = gRect("GreatPeopleBar").y() - 42
				screen.setLabel("CultureBreakdownLabel1", "Background", szRow1, CvUtil.FONT_LEFT_JUSTIFY, iXRight, iY1, -0.1, FontTypes.SMALL_FONT, WIDGET_CITY_BREAKDOWNS, -1, -1)
				screen.setLabel("CultureBreakdownLabel2", "Background", szRow2, CvUtil.FONT_LEFT_JUSTIFY, iXRight, iY2, -0.1, FontTypes.SMALL_FONT, WIDGET_CITY_BREAKDOWNS, -1, -1)

		return 0

	def fillCityBonusRow(self, iColumn, iRow, iBonus, iAmount, szEffect = None):
		szIndex = str(iColumn) + "_" + str(iRow)
		szButtonName = "CityBonusBtn" + szIndex
		# <!-- custom: Keep both large button renderers: mode 2 addDDS removes distracting button-frame edges, while mode 1 restores base AdvCiv-style framed image buttons. Mode 0 uses old AdvCiv-SAS text glyphs, and text glyphs remain the overflow fallback when selected-city bonus rows do not fit. Mode 0 did not redirect to Sevopedia in testing, including after trying WIDGET_PEDIA_JUMP_TO_BONUS_TRADE with data2=-1. Use the trade widget with data2=-1 only for plain bonus hover in button modes, then route button-mode clicks in Python like religion/corporation city icons because direct bonus-widget clicks were not reliable in testing. Empirically, mode 1 supports left/right click, while mode 2 supports right-click but not left-click in our Python-only attempts so far. (Claude code Opus 4.7; GPT-5.5) -->
		# <!-- custom: Use plain bonus widget data here, not obsolete-bonus widget data.
		# We considered obsolete bonuses because of the previous Tech Chooser obsolete-widget issue, but testing Germany with Environmentalism and an obsolete Elephant resource in Munich showed Elephants do not appear in this panel.
		# The code path matches that result because the city list is filtered by pHeadSelectedCity.hasBonus, and connected map bonuses are added through CvPlot::updatePlotGroupBonus using getNonObsoleteBonusType; therefore this row does not need obsolete-bonus support. (GPT-5.5) -->
		if self.SAS_CV_MAIN_INTERFACE_CITY_BONUS_ICON_MODE == 2:
			self.addDDS(szButtonName, gc.getBonusInfo(iBonus), WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS_TRADE, iBonus, -1)
		else:
			self.setImageButton(szButtonName, gc.getBonusInfo(iBonus).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS_TRADE, iBonus, -1)
		if iAmount > 1:
			self.addDDS("CityBonusCircle" + szIndex, "WHITE_CIRCLE_40", WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus)
			szAmount = (sasFontTagLabel + localText.changeTextColor(str(iAmount), self.colorYellow) + SAS_FONT_TAG_CLOSE)
			self.setLabel("CityBonusAmount" + szIndex, "BonusBack0", szAmount, CvUtil.FONT_CENTER_JUSTIFY, FontTypes.SMALL_FONT, -0.1, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus)
		if szEffect:
			self.setLabel("CityBonusEffect" + szIndex, "BonusBack" + str(iColumn), szEffect, CvUtil.FONT_RIGHT_JUSTIFY, FontTypes.SMALL_FONT, -0.1, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus)

	def updateInfoPaneStrings(self):
		screen = self.screen
		iRow = 0
		#bShift = CyInterface().shiftKey() # advc: unused

		self.addPanel("SelectedUnitPanel")
		screen.setStyle("SelectedUnitPanel", "Panel_Game_HudStat_Style")
		screen.hide("SelectedUnitPanel")

		self.setImageButton("SelectedUnitInfoButton", "INTERFACE_GENERAL_CIVILOPEDIA_ICON", WidgetTypes.WIDGET_HELP_SELECTED, -1, -1)
		if self.IS_SAS_CV_MAIN_INTERFACE_UNIT_INFO_BUTTON:
			screen.hide("SelectedUnitInfoButton")
		else:
			screen.hide("SelectedUnitInfoButton")

		self.addTable("SelectedUnitText", 3, "Table_EmptyScroll_Style")
		screen.hide("SelectedUnitText")
		screen.hide("SelectedUnitLabel")

		self.addTable("SelectedCityText", 3, "Table_EmptyScroll_Style")
		screen.hide("SelectedCityText")

		for i in range(gc.getNumPromotionInfos()):
			szName = "PromotionButton" + str(i)
			screen.hide(szName)
# BUG - Stack Promotions - start
			szName = "PromotionButtonCircle" + str(i)
			screen.hide(szName)
			szName = "PromotionButtonCount" + str(i)
			screen.hide(szName)
# BUG - Stack Promotions - end

		if CyEngine().isGlobeviewUp():
			return

		# advc.092: Empty column in both unit and city text table.
		# Shouldn't scale up I think. Was 10, but that exceeds the total width of the table by 2, and my guess is that the total of the column width is supposed to be 2 _less_ than the table width.
		iThirdColW = 6
		kInterface = CyInterface()
		pHeadSelectedCity = kInterface.getHeadSelectedCity()
		if pHeadSelectedCity:
			iOrders = kInterface.getNumOrdersQueued()
			iFlexW = gRect("SelectedCityText").width() - 2 - iThirdColW
			screen.setTableColumnHeader("SelectedCityText", 0, u"", (121 * iFlexW) / 175)
			screen.setTableColumnHeader("SelectedCityText", 1, u"", (54 * iFlexW) / 175)
			screen.setTableColumnHeader("SelectedCityText", 2, u"", iThirdColW)
			screen.setTableColumnRightJustify("SelectedCityText", 1)
			for i in range(iOrders):
				szLeftBuffer = u""
				szRightBuffer = u""
				# <!-- custom: add buttons city screen in the production queue's elements with the help of claude opus 4.5 -->
				szButton = ""
				eOrderNodeType = kInterface.getOrderNodeType(i)
				iOrderNodeData = kInterface.getOrderNodeData1(i)
				if (eOrderNodeType == OrderTypes.ORDER_TRAIN):
					szLeftBuffer = gc.getUnitInfo(iOrderNodeData).getDescription()
					if self.IS_SAS_CV_MAIN_INTERFACE_PRODUCTION_QUEUE_BUTTONS:
						szButton = gc.getUnitInfo(iOrderNodeData).getButton()
					iProductionTurns = pHeadSelectedCity.getUnitProductionTurnsLeft(iOrderNodeData, i)
					if iProductionTurns > 0: # advc.004x
						szRightBuffer = "(" + str(iProductionTurns) + ")"
					if (kInterface.getOrderNodeSave(i)):
						szLeftBuffer = u"*" + szLeftBuffer
# BUG - Production Started - start
					if CityScreenOpt.isShowProductionStarted():
						eUnit = iOrderNodeData
						if pHeadSelectedCity.getUnitProduction(eUnit) > 0:
							szRightBuffer = BugUtil.colorText(szRightBuffer, "COLOR_CYAN")
# BUG - Production Started - end
# BUG - Production Decay - start
					# advc.094: BugDll.isPresent check removed; active player check added (replacing a is-human check in the DLL).
					if (CityScreenOpt.isShowProductionDecayQueue() and pHeadSelectedCity.getOwner() == gc.getGame().getActivePlayer()):
						eUnit = iOrderNodeData
						if pHeadSelectedCity.getUnitProduction(eUnit) > 0:
							if pHeadSelectedCity.isUnitProductionDecay(eUnit):
								szLeftBuffer = BugUtil.getText("TXT_KEY_BUG_PRODUCTION_DECAY_THIS_TURN", (szLeftBuffer,))
							elif pHeadSelectedCity.getUnitProductionTime(eUnit) > 0:
								iDecayTurns = pHeadSelectedCity.getUnitProductionDecayTurns(eUnit)
								if iDecayTurns <= CityScreenOpt.getProductionDecayQueueUnitThreshold():
									szLeftBuffer = BugUtil.getText("TXT_KEY_BUG_PRODUCTION_DECAY_WARNING", (szLeftBuffer,))
# BUG - Production Decay - end
				elif (eOrderNodeType == OrderTypes.ORDER_CONSTRUCT):
					szLeftBuffer = gc.getBuildingInfo(iOrderNodeData).getDescription()
					if self.IS_SAS_CV_MAIN_INTERFACE_PRODUCTION_QUEUE_BUTTONS:
						szButton = gc.getBuildingInfo(iOrderNodeData).getButton()
					iProductionTurns = pHeadSelectedCity.getBuildingProductionTurnsLeft(iOrderNodeData, i)
					if iProductionTurns > 0: # advc.004x
						szRightBuffer = "(" + str(iProductionTurns) + ")"
# BUG - Production Started - start
					if CityScreenOpt.isShowProductionStarted():
						eBuilding = iOrderNodeData
						if pHeadSelectedCity.getBuildingProduction(eBuilding) > 0:
							szRightBuffer = BugUtil.colorText(szRightBuffer, "COLOR_CYAN")
# BUG - Production Started - end
# BUG - Production Decay - start
					# advc.094: BugDll.isPresent check removed; active player check added.
					if (CityScreenOpt.isShowProductionDecayQueue() and pHeadSelectedCity.getOwner() == gc.getGame().getActivePlayer()):
						eBuilding = iOrderNodeData
						if pHeadSelectedCity.getBuildingProduction(eBuilding) > 0:
							if pHeadSelectedCity.isBuildingProductionDecay(eBuilding):
								szLeftBuffer = BugUtil.getText("TXT_KEY_BUG_PRODUCTION_DECAY_THIS_TURN", (szLeftBuffer,))
							elif pHeadSelectedCity.getBuildingProductionTime(eBuilding) > 0:
								iDecayTurns = pHeadSelectedCity.getBuildingProductionDecayTurns(eBuilding)
								if iDecayTurns <= CityScreenOpt.getProductionDecayQueueBuildingThreshold():
									szLeftBuffer = BugUtil.getText("TXT_KEY_BUG_PRODUCTION_DECAY_WARNING", (szLeftBuffer,))
# BUG - Production Decay - end
				elif (eOrderNodeType == OrderTypes.ORDER_CREATE):
					szLeftBuffer = gc.getProjectInfo(iOrderNodeData).getDescription()
					if self.IS_SAS_CV_MAIN_INTERFACE_PRODUCTION_QUEUE_BUTTONS:
						szButton = gc.getProjectInfo(iOrderNodeData).getButton()
					iProductionTurns = pHeadSelectedCity.getProjectProductionTurnsLeft(iOrderNodeData, i)
					if iProductionTurns > 0: # advc.004x
						szRightBuffer = "(" + str(iProductionTurns) + ")"
# BUG - Production Started - start
					if BugDll.isVersion(3) and CityScreenOpt.isShowProductionStarted():
						eProject = iOrderNodeData
						if pHeadSelectedCity.getProjectProduction(eProject) > 0:
							szRightBuffer = BugUtil.colorText(szRightBuffer, "COLOR_CYAN")
# BUG - Production Started - end
				elif (eOrderNodeType == OrderTypes.ORDER_MAINTAIN):
					szLeftBuffer = gc.getProcessInfo(iOrderNodeData).getDescription()
					if self.IS_SAS_CV_MAIN_INTERFACE_PRODUCTION_QUEUE_BUTTONS:
						szButton = gc.getProcessInfo(iOrderNodeData).getButton()
				# <!-- custom: add buttons city screen in the production queue's elements with the help of claude opus 4.5 -->
				iRow = self._appendSelectedInfoPaneTextRow(screen, "SelectedCityText", iRow, szLeftBuffer, szRightBuffer, szButton, i)
			# <advc> Reduce indentation
			return
		pHeadSelectedUnit = kInterface.getHeadSelectedUnit()
		if (not pHeadSelectedUnit or kInterface.getShowInterface() != InterfaceVisibility.INTERFACE_SHOW):
			screen.hide("SelectedUnitInfoButton")
			return # </advc>
		if self.IS_SAS_CV_MAIN_INTERFACE_UNIT_INFO_BUTTON:
			self.setImageButton("SelectedUnitInfoButton", pHeadSelectedUnit.getButton(), WidgetTypes.WIDGET_HELP_SELECTED, -1, -1)
			screen.show("SelectedUnitInfoButton")
		else:
			screen.hide("SelectedUnitInfoButton")
		iFlexW = gRect("SelectedUnitText").width() - 2 - iThirdColW
		screen.setTableColumnHeader("SelectedUnitText", 0, u"", (100 * iFlexW) / 175)
		screen.setTableColumnHeader("SelectedUnitText", 1, u"", (75 * iFlexW) / 175)
		screen.setTableColumnHeader("SelectedUnitText", 2, u"", iThirdColW)
		screen.setTableColumnRightJustify("SelectedUnitText", 1)
		if (kInterface.mirrorsSelectionGroup()):
			pSelectedGroup = pHeadSelectedUnit.getGroup()
		else:
			pSelectedGroup = 0
		iSelectionListLength = kInterface.getLengthSelectionList()
		if (iSelectionListLength > 1):
# BUG - Stack Movement Display - start
			szBuffer = localText.getText("TXT_KEY_UNIT_STACK", (iSelectionListLength,))
			if MainOpt.isShowStackMovementPoints():
				iMinMoves = 100000
				iMaxMoves = 0
				for i in range(iSelectionListLength):
					pUnit = kInterface.getSelectionUnit(i)
					if (pUnit is not None):
						iLoopMoves = pUnit.movesLeft()
						if (iLoopMoves > iMaxMoves):
							iMaxMoves = iLoopMoves
						if (iLoopMoves < iMinMoves):
							iMinMoves = iLoopMoves
				if (iMinMoves == iMaxMoves):
					fMinMoves = float(iMinMoves) / self.iMoveDenominator
					szBuffer += u" %.1f%s" % (fMinMoves, self.szMovesIcon)
				else:
					fMinMoves = float(iMinMoves) / self.iMoveDenominator
					fMaxMoves = float(iMaxMoves) / self.iMoveDenominator
					szBuffer += u" %.1f - %.1f%s" % (fMinMoves, fMaxMoves, self.szMovesIcon)
			if (szBuffer.find("<font=") == -1):
				szBuffer = sasFontTagLabel + szBuffer + SAS_FONT_TAG_CLOSE
			self.setText("SelectedUnitLabel", "Background", szBuffer, CvUtil.FONT_LEFT_JUSTIFY, FontTypes.SMALL_FONT, -0.1, WidgetTypes.WIDGET_UNIT_NAME)
# BUG - Stack Movement Display - end
# BUG - Stack Promotions - start
			if MainOpt.isShowStackPromotions():
				iNumPromotions = gc.getNumPromotionInfos()
				lPromotionCounts = [0] * iNumPromotions
				iNumUnits = iSelectionListLength
				for i in range(iNumUnits):
					pUnit = kInterface.getSelectionUnit(i)
					if (pUnit is not None):
						for j in range(iNumPromotions):
							if (pUnit.isHasPromotion(j)):
								lPromotionCounts[j] += 1
				iSPColor = MainOpt.getStackPromotionColor()
				iSPColorAll = MainOpt.getStackPromotionColorAll()
				iPromotionCount = 0
				bShowCount = MainOpt.isShowStackPromotionCounts()
				iPromoBtnSize = gRect("PromotionButton0").size() # advc.092
				for i, iCount in enumerate(lPromotionCounts):
					if (iCount > 0):
						szName = "PromotionButton" + str(i)
						x, y = self.setPromotionButtonPosition(szName, iPromotionCount)
						screen.moveToFront(szName)
						screen.show(szName)
						if (bShowCount and iCount > 1):
							szName = "PromotionButtonCircle" + str(i)
							screen.moveItem(szName, x + (10 * iPromoBtnSize) / 24, y + (10 * iPromoBtnSize) / 24, -0.3)
							screen.moveToFront(szName)
							screen.show(szName)
							szName = "PromotionButtonCount" + str(iPromotionCount)
							szText = sasFontTagLabel + u"%d" % iCount + SAS_FONT_TAG_CLOSE
							if iCount == iNumUnits:
								szText = BugUtil.colorText(szText, iSPColorAll)
							else:
								szText = BugUtil.colorText(szText, iSPColor)
							screen.setText(szName, "Background", szText, CvUtil.FONT_CENTER_JUSTIFY, x + (17 * iPromoBtnSize) / 24, y + (7 * iPromoBtnSize) / 24, -0.2, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, i, -1)
							screen.setHitTest(szName, HitTestTypes.HITTEST_NOHIT)
							screen.moveToFront(szName)
							screen.show(szName)
						iPromotionCount += 1
# BUG - Stack Promotions - end
			if (pSelectedGroup == 0 or pSelectedGroup.getLengthMissionQueue() <= 1):
				if (pHeadSelectedUnit):
					for i in range(gc.getNumUnitInfos()):
						iCount = kInterface.countEntities(i)
						if (iCount > 0):
							szRightBuffer = u""
							szLeftBuffer = gc.getUnitInfo(i).getDescription()
							# <!-- custom: add buttons to unit selection panel in the main map view (Claude code Sonnet 4.5) -->
							szButton = ""
							if self.IS_SAS_CV_MAIN_INTERFACE_PRODUCTION_QUEUE_BUTTONS:
								szButton = gc.getUnitInfo(i).getButton()
							if (iCount > 1):
								szRightBuffer = u"(" + str(iCount) + u")"
							# <!-- custom: End - add buttons to unit selection panel in the main map view (Claude code Sonnet 4.5) -->
							iRow = self._appendSelectedInfoPaneTextRow(screen, "SelectedUnitText", iRow, szLeftBuffer, szRightBuffer, szButton, i)
		else: # (selection list length less than 1)
			if (pHeadSelectedUnit.getHotKeyNumber() == -1):
				szBuffer = localText.getText("INTERFACE_PANE_UNIT_NAME", (pHeadSelectedUnit.getName(),))
			else:
				szBuffer = localText.getText("INTERFACE_PANE_UNIT_NAME_HOT_KEY", (pHeadSelectedUnit.getHotKeyNumber(), pHeadSelectedUnit.getName()))
			if (len(szBuffer) > 60):
				szBuffer = sasFontTagLabel + szBuffer + SAS_FONT_TAG_CLOSE
			if (szBuffer.find("<font=") == -1):
				szBuffer = sasFontTagLabel + szBuffer + SAS_FONT_TAG_CLOSE
			self.setText("SelectedUnitLabel", "Background", szBuffer, CvUtil.FONT_LEFT_JUSTIFY, FontTypes.SMALL_FONT, -0.1, WidgetTypes.WIDGET_UNIT_NAME)
			if ((pSelectedGroup == 0) or (pSelectedGroup.getLengthMissionQueue() <= 1)):
				screen.show("SelectedUnitText")
				screen.show("SelectedUnitPanel")
				szBuffer = u""
				szLeftBuffer = u""
				szRightBuffer = u""
				if (pHeadSelectedUnit.getDomainType() == DomainTypes.DOMAIN_AIR):
					if (pHeadSelectedUnit.airBaseCombatStr() > 0):
						szLeftBuffer = self.szTextAirStrength
						if (pHeadSelectedUnit.isFighting()):
							szRightBuffer = u"?/%d%s" %(pHeadSelectedUnit.airBaseCombatStr(), self.szStrengthIcon)
						elif (pHeadSelectedUnit.isHurt()):
							# <advc.004> Replacing Python implementation
							szRightBuffer = u"%s" %(CyGameTextMgr().getHurtUnitStrength(pHeadSelectedUnit))
							# </advc.004>
						else:
							szRightBuffer = u"%d%s" %(pHeadSelectedUnit.airBaseCombatStr(), self.szStrengthIcon)
				else:
					if (pHeadSelectedUnit.canFight()):
						szLeftBuffer = self.szTextStrength
						if (pHeadSelectedUnit.isFighting()):
							szRightBuffer = u"?/%d%s" %(pHeadSelectedUnit.baseCombatStr(), self.szStrengthIcon)
						elif (pHeadSelectedUnit.isHurt()):
							# <advc.004> Replacing Python implementation
							szRightBuffer = u"%s" %(CyGameTextMgr().getHurtUnitStrength(pHeadSelectedUnit))
							# </advc.004>
						else:
							szRightBuffer = u"%d%s" %(pHeadSelectedUnit.baseCombatStr(), self.szStrengthIcon)
				szBuffer = szLeftBuffer + szRightBuffer
				if (szBuffer):
					iRow = self._appendSelectedInfoPaneTextRow(screen, "SelectedUnitText", iRow, szLeftBuffer, szRightBuffer)
				szLeftBuffer = u""
				szRightBuffer = u""
				# <advc.004w> Don't show "Movement" row if it can't move
				eDomain = pHeadSelectedUnit.getDomainType()
				if eDomain == DomainTypes.DOMAIN_AIR: # Show range instead for air units
					szLeftBuffer = self.szTextRange
					szRightBuffer = u"%d" %(pHeadSelectedUnit.airRange(),)
				elif pHeadSelectedUnit.getDomainType() != DomainTypes.DOMAIN_IMMOBILE:
				# </advc.004w>
# BUG - Unit Movement Fraction - start
					szLeftBuffer = self.szTextMovement
					if MainOpt.isShowUnitMovementPointsFraction():
						szRightBuffer = u"%d%s" %(pHeadSelectedUnit.baseMoves(), self.szMovesIcon)
						if (pHeadSelectedUnit.movesLeft() == 0):
							szRightBuffer = u"0/" + szRightBuffer
						elif (pHeadSelectedUnit.movesLeft() == pHeadSelectedUnit.baseMoves() * self.iMoveDenominator):
							pass
						else:
							fCurrMoves = float(pHeadSelectedUnit.movesLeft())
							fCurrMoves /= self.iMoveDenominator
							szRightBuffer = (u"%.1f/" % fCurrMoves) + szRightBuffer
					else:
						if ((pHeadSelectedUnit.movesLeft() % self.iMoveDenominator) > 0):
							iDenom = 1
						else:
							iDenom = 0
						iCurrMoves = ((pHeadSelectedUnit.movesLeft() / self.iMoveDenominator) + iDenom)
						if (pHeadSelectedUnit.baseMoves() == iCurrMoves or eDomain == DomainTypes.DOMAIN_AIR):
							szRightBuffer = u"%d%s" %(pHeadSelectedUnit.baseMoves(), self.szMovesIcon)
						else:
							szRightBuffer = u"%d/%d%s" %(iCurrMoves, pHeadSelectedUnit.baseMoves(), self.szMovesIcon)
# BUG - Unit Movement Fraction - end
				iRow = self._appendSelectedInfoPaneTextRow(screen, "SelectedUnitText", iRow, szLeftBuffer, szRightBuffer)
				# advc.004w
				if (pHeadSelectedUnit.getLevel() > 0 and pHeadSelectedUnit.getExperience() > 0):
					szLeftBuffer = self.szTextLevel
					szRightBuffer = u"%d" %(pHeadSelectedUnit.getLevel())
					iRow = self._appendSelectedInfoPaneTextRow(screen, "SelectedUnitText", iRow, szLeftBuffer, szRightBuffer)
				if ((pHeadSelectedUnit.getExperience() > 0) and not pHeadSelectedUnit.isFighting()):
					szLeftBuffer = self.szTextExperience
					szRightBuffer = u"(%d/%d)" %(pHeadSelectedUnit.getExperience(), pHeadSelectedUnit.experienceNeeded())
					iRow = self._appendSelectedInfoPaneTextRow(screen, "SelectedUnitText", iRow, szLeftBuffer, szRightBuffer)
				iPromotionCount = 0
				for i in range(gc.getNumPromotionInfos()):
					if (pHeadSelectedUnit.isHasPromotion(i)):
						szName = "PromotionButton" + str(i)
						self.setPromotionButtonPosition(szName, iPromotionCount)
						screen.moveToFront(szName)
						screen.show(szName)
						iPromotionCount = iPromotionCount + 1
		if (pSelectedGroup):
			iNodeCount = pSelectedGroup.getLengthMissionQueue()
			if (iNodeCount > 1):
				for i in range(iNodeCount):
					szLeftBuffer = u""
					szRightBuffer = u""
					if (gc.getMissionInfo(pSelectedGroup.getMissionType(i)).isBuild()):
						if (i == 0):
							szLeftBuffer = gc.getBuildInfo(pSelectedGroup.getMissionData1(i)).getDescription()
							# advc.251: Pass group owner as param
							szRightBuffer = localText.getText("INTERFACE_CITY_TURNS", (pSelectedGroup.plot().getBuildTurnsLeft(pSelectedGroup.getMissionData1(i), pSelectedGroup.getOwner(), 0, 0),))
						else:
							szLeftBuffer = u"%s..." %(gc.getBuildInfo(pSelectedGroup.getMissionData1(i)).getDescription())
					else:
						szLeftBuffer = u"%s..." %(gc.getMissionInfo(pSelectedGroup.getMissionType(i)).getDescription())
					iRow = self._appendSelectedInfoPaneTextRow(screen, "SelectedUnitText", iRow, szLeftBuffer, szRightBuffer, "", i)
		return

	# advc.004z: Cut from updateScoreStrings
	def hideScoreStrings(self):
		screen = self.screen
# BUG - Align Icons - start
		for iPlayer in range(gc.getMAX_CIV_PLAYERS()): # advc: Was MAX_PLAYERS
			szIndex = str(iPlayer)
			screen.hide("ScoreText" + szIndex)
			screen.hide("ScoreTech" + szIndex)
			# <kekm.30>
			screen.hide("ScoreLeader" + szIndex)
			screen.hide("ScoreCiv" + szIndex) # </kekm.30>
			for i in range(Scoreboard.NUM_PARTS):
				screen.hide("ScoreText%d-%d" %(iPlayer, i))
# BUG - Align Icons - end
	# <!-- custom: recreate ScoreBackground with the selected PanelStyle; only on change to avoid per-refresh churn.
	# No runtime setPanelStyle in Civ4, hence delete+addPanel (same pattern as _rebuildCityScreenBottomBars).
	# PANEL_STYLE_EMPTY level reads as no background. (Claude code Opus 4.7) -->
	def _applyScoreBackgroundStyle(self):
		if self.iScoreBgStyleLevel is None:
			if self.iSAS_SCOREBOARD_BG_DEFAULT_STYLE is None:
				self.iSAS_SCOREBOARD_BG_DEFAULT_STYLE = gc.getDefineINT("SAS_SCOREBOARD_BG_DEFAULT_STYLE")
			self.iScoreBgStyleLevel = self.iSAS_SCOREBOARD_BG_DEFAULT_STYLE
		eStyle = self.aScoreBgStyleLevels[self.iScoreBgStyleLevel]
		if eStyle == self._eScoreBgStyleApplied:
			return
		screen = self.screen
		try:
			screen.deleteWidget("ScoreBackground")
		except:
			pass
		screen.addPanel("ScoreBackground", u"", u"", True, False, 0, 0, 0, 0, eStyle)
		self._eScoreBgStyleApplied = eStyle
	# advc.092: Reconciling redundant BtS code from updateScoreStrings with BUG code from Scoreboard.draw.
	def updateScoreBackgrSize(self, iTextWidth, iTextHeight):
		iRMargin = gRect("Top").xRight() - gPoint("ScoreTextLowerRight").x()
		# <!-- custom: fixed symmetric panel margin matching original (3*iBMargin/2 with iBMargin=VSPACE(6)) so text is always centered; no longer derived from iBMargin which blows up when iScoreBottom moves. (Claude code Sonnet 4.6) -->
		iSidePad = VSPACE(9)
		iScoreBottomY = gPoint("ScoreTextLowerRight").y()
		self.screen.setPanelSize("ScoreBackground", gRect("Top").xRight() - iTextWidth - 2 * iRMargin, iScoreBottomY - iTextHeight - iSidePad, iTextWidth + (7 * iRMargin) / 4, iTextHeight + 2 * iSidePad)

	def updateScoreStrings(self, bOnlyBackgr = False): # advc.004z: new param
		screen = self.screen
		# <advc.004z>
		if not bOnlyBackgr:
			self.hideScoreStrings() # </advc.004z>
		screen.hide("ScoreBackground")
		screen.hide("ScoreScrollUp")
		screen.hide("ScoreScrollDown")
		screen.hide("ScoreScrollUpFast")
		screen.hide("ScoreScrollDownFast")
		screen.hide("ScoreBgStyle")
		screen.hide("ScoreExpandToggle")
		screen.hide("ScoreScrollCenterActivePlayer")
		screen.hide("ScoreScrollAutocenterActivePlayerToggle")
		eUIVis = CyInterface().getShowInterface()
		if (eUIVis == InterfaceVisibility.INTERFACE_HIDE_ALL or eUIVis == InterfaceVisibility.INTERFACE_MINIMAP_ONLY or not CyInterface().isScoresVisible() or CyInterface().isCityScreenUp()):
			return
		# <advc.004z>
		# </advc.004z>
		if (CyEngine().isGlobeviewUp() and (not MainOpt.isScoresInGlobeView() or eUIVis == InterfaceVisibility.INTERFACE_HIDE)):
			return
		# <advc.004z>
		# <!-- custom: ensure ScoreBackground uses the currently selected PanelStyle (recreated only on change). (Claude code Opus 4.7) -->
		self._applyScoreBackgroundStyle()
		screen.show("ScoreBackground") # Moved up
		if bOnlyBackgr:
			return # </advc.004z>
		xResolution = screen.getXResolution()
		yResolution = screen.getYResolution()
		iWidth = 0
		iCount = 0
		iBtnHeight = VLEN(22)
		# <advc.092> The Scoreboard module will use this too
		# <!-- custom: compute available scoreboard height and cap visible rows; show scroll buttons when player count exceeds capacity. (Claude code Sonnet 4.6) -->
		# <!-- custom: buttons sit just above GlobeToggle; iScoreBottom is panel text-bottom = buttons_y - gap - iSidePad, so panel is exactly symmetric (9px each side) matching original. (Claude code Sonnet 4.6) -->
		iSScrollBtnSz = gRect("ScoreScrollUp").width()
		iSScrollY = gRect("GlobeToggle").y() - iSScrollBtnSz  # buttons flush above GlobeToggle
		iScoreBottom = iSScrollY - VSPACE(2) - VSPACE(9)  # 2px gap + 9px panel bottom margin
		# </advc.092>
		gSetPoint("ScoreTextLowerRight", PointLayout(gRect("MiniMap").xRight(), iScoreBottom))
		iMaxRows = self._scoreboardMaxRows()
		# <!-- custom: position scroll buttons at runtime below the panel.
		# GlobeToggle.y() is 0 at init so buttons must be placed here instead. (Claude code Sonnet 4.6) -->
		# <!-- custom: row layout right->left is [BgStyle][Lock][AutoCenter] | gap | [-fast][-][Center][+][+fast].
		# Normal scroll moves one row; fast scroll uses SAS_SCOREBOARD_SCROLL_INCREMENT_FAST; manual scroll disables autocenter so deliberate strongest/weakest watching is preserved. (Claude code Opus 4.7 + ChatGPT-5.5 + GPT-5.5) -->
		iSRight = gPoint("ScoreTextLowerRight").x()
		iSGap = HSPACE(6)  # gap separating the fixed buttons from the scroll/center cluster
		screen.moveItem("ScoreBgStyle", iSRight - 1 * iSScrollBtnSz, iSScrollY, -0.3)
		screen.moveItem("ScoreExpandToggle", iSRight - 2 * iSScrollBtnSz, iSScrollY, -0.3)
		screen.moveItem("ScoreScrollAutocenterActivePlayerToggle", iSRight - 3 * iSScrollBtnSz, iSScrollY, -0.3)
		screen.moveItem("ScoreScrollDownFast", iSRight - 4 * iSScrollBtnSz - iSGap, iSScrollY, -0.3)
		screen.moveItem("ScoreScrollDown", iSRight - 5 * iSScrollBtnSz - iSGap, iSScrollY, -0.3)
		screen.moveItem("ScoreScrollCenterActivePlayer", iSRight - 6 * iSScrollBtnSz - iSGap, iSScrollY, -0.3)
		screen.moveItem("ScoreScrollUp", iSRight - 7 * iSScrollBtnSz - iSGap, iSScrollY, -0.3)
		screen.moveItem("ScoreScrollUpFast", iSRight - 8 * iSScrollBtnSz - iSGap, iSScrollY, -0.3)
		# <!-- custom: seed the lock-hover default once, close to the first real scoreboard use; handleInput only toggles after this button exists. (ChatGPT-5.5) -->
		if self.bScoreAlwaysExpand is None:
			if self.iSAS_SCOREBOARD_LOCK_HOVER_DEFAULT_ENABLED is None:
				self.iSAS_SCOREBOARD_LOCK_HOVER_DEFAULT_ENABLED = gc.getDefineINT("SAS_SCOREBOARD_LOCK_HOVER_DEFAULT_ENABLED")
			self.bScoreAlwaysExpand = (self.iSAS_SCOREBOARD_LOCK_HOVER_DEFAULT_ENABLED > 0)
		screen.setState("ScoreExpandToggle", self.bScoreAlwaysExpand)
		# <!-- custom: swap emoji per state. (Claude code Opus 4.7) -->
		if self.bScoreAlwaysExpand:
			screen.changeImageButton("ScoreExpandToggle", self.szScoreExpandTogglePath)
		else:
			screen.changeImageButton("ScoreExpandToggle", self.szScoreExpandToggleOffPath)
		screen.show("ScoreExpandToggle")
		# <!-- custom: seed the scoreboard scroll autocenter default once.
		# The in-game toggle controls runtime behavior afterward; manual scroll buttons turn it off. (ChatGPT-5.5) -->
		if self.bScoreScrollAutocenterActivePlayer is None:
			if self.iSAS_SCOREBOARD_SCROLL_AUTOCENTER_ACTIVE_PLAYER_DEFAULT_ENABLED is None:
				self.iSAS_SCOREBOARD_SCROLL_AUTOCENTER_ACTIVE_PLAYER_DEFAULT_ENABLED = gc.getDefineINT("SAS_SCOREBOARD_SCROLL_AUTOCENTER_ACTIVE_PLAYER_DEFAULT_ENABLED")
			self.bScoreScrollAutocenterActivePlayer = (self.iSAS_SCOREBOARD_SCROLL_AUTOCENTER_ACTIVE_PLAYER_DEFAULT_ENABLED > 0)
		screen.setState("ScoreScrollAutocenterActivePlayerToggle", self.bScoreScrollAutocenterActivePlayer)
		if self.bScoreScrollAutocenterActivePlayer:
			screen.changeImageButton("ScoreScrollAutocenterActivePlayerToggle", self.szScoreScrollAutocenterActivePlayerPath)
		else:
			screen.changeImageButton("ScoreScrollAutocenterActivePlayerToggle", self.szScoreScrollAutocenterActivePlayerOffPath)
		screen.show("ScoreBgStyle")
# BUG - Align Icons - start
		bAlignIcons = ScoreOpt.isAlignIcons()
		if bAlignIcons:
			scores = Scoreboard.Scoreboard()
			# <advc.085>
			global gAlignedScoreboard
			gAlignedScoreboard = scores # </advc.085>
		# <advc>
		else:
			scores = None # </advc>
# BUG - Align Icons - end
		# (BUG - Power Rating)  advc: Moved into the loop
		# <!-- custom: use one ranked visible-player traversal for scroll slicing, aligned BUG drawing, and active-player centering so those behaviors cannot drift apart. (GPT-5.3-Codex + ChatGPT-5.5) -->
		aeVisiblePlayers = self._scoreboardVisiblePlayers(scores)
		# <!-- custom: The aligned BUG scoreboard may sort vassals and apply its positive Max Players cap.
		# Use that exact effective population for scrolling and centering; using the uncapped traversal made the buttons change while the same capped rows remained visible. See KI#228. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if bAlignIcons:
			aeVisiblePlayers = scores.prepare()
		iTotalCount = len(aeVisiblePlayers)
		if self.bScoreScrollAutocenterActivePlayer:
			self._centerScoreboardScrollOnActivePlayer(aeVisiblePlayers, iMaxRows)
		if not bAlignIcons:
			if iTotalCount <= iMaxRows:
				self.iScoreScrollOffset = 0
			else:
				self.iScoreScrollOffset = max(0, min(self.iScoreScrollOffset, iTotalCount - iMaxRows))
			aeVisibleSlice = aeVisiblePlayers[self.iScoreScrollOffset : self.iScoreScrollOffset + iMaxRows]
			iCount = 0
			for ePlayer in aeVisibleSlice:
				szBuffer = sasFontTagLabel + self.playerScoreString(ePlayer, None, False) + SAS_FONT_TAG_CLOSE
				iTextW = CyInterface().determineWidth(szBuffer)
				if iTextW > iWidth:
					iWidth = iTextW
				szName = "ScoreText" + str(ePlayer)
# BUG - Dead Civs - start
				if gc.getPlayer(ePlayer).isAlive():
					iWidgetType = WidgetTypes.WIDGET_CONTACT_CIV
					eContactPlayer = ePlayer
				else:
					iWidgetType = WidgetTypes.WIDGET_GENERAL
					eContactPlayer = -1
				yCoord = gPoint("ScoreTextLowerRight").y() - iBtnHeight
				screen.setText(szName, "Background", szBuffer, CvUtil.FONT_RIGHT_JUSTIFY, gPoint("ScoreTextLowerRight").x(), yCoord - iCount * iBtnHeight, -0.3, FontTypes.SMALL_FONT, iWidgetType, eContactPlayer, -1)
# BUG - Dead Civs - end
				screen.show(szName)
				CyInterface().checkFlashReset(ePlayer)
				iCount += 1
			self.updateScoreBackgrSize(iWidth, iBtnHeight * iCount) # advc.092
		else:
			# BUG aligned path: let Scoreboard.draw handle slicing.
			if iTotalCount <= iMaxRows:
				self.iScoreScrollOffset = 0
			else:
				self.iScoreScrollOffset = max(0, min(self.iScoreScrollOffset, iTotalCount - iMaxRows))
# BUG - Align Icons - start
			scores.draw(screen, self.iScoreScrollOffset, iMaxRows, self.bScoreAlwaysExpand, True)
# BUG - Align Icons - end
		# <!-- custom: show scroll buttons when there are more players than visible rows; + (ScoreScrollUp) shows more lower-ranked players, - (ScoreScrollDown) goes back toward rank 1. (Claude code Sonnet 4.6 + ChatGPT-5.5 + GPT-5.5) -->
		if iTotalCount > iMaxRows:
			screen.show("ScoreScrollCenterActivePlayer")
			screen.show("ScoreScrollAutocenterActivePlayerToggle")
			if self.iScoreScrollOffset < iTotalCount - iMaxRows:
				screen.show("ScoreScrollUp")
				screen.show("ScoreScrollUpFast")
			if self.iScoreScrollOffset > 0:
				screen.show("ScoreScrollDown")
				screen.show("ScoreScrollDownFast")

	# <advc> Body cut from updateScoreStrings in order to reduce indentation
	def playerScoreString(self, ePlayer, scores, bAlignIcons):
		pPlayer = gc.getPlayer(ePlayer)
		eTeam = pPlayer.getTeam()
		pTeam = gc.getTeam(eTeam)
		g = gc.getGame()
		eActivePlayer = g.getActivePlayer()
		eActiveTeam = g.getActiveTeam()
		pActivePlayer = gc.getPlayer(eActivePlayer)
		pActiveTeam = gc.getTeam(eActiveTeam)
		szBuffer = "" # </advc>
# BUG: Align Icons throughout -- if (bAlignIcons): scores.setFoo(foo)
		if g.isGameMultiPlayer():
			if not pPlayer.isTurnActive():
				szBuffer = szBuffer + "*"
				if bAlignIcons:
					scores.setWaiting()
# BUG - Dead Civs - start
		# <advc.190d>
		bConcealCiv = False
		bConcealLeader = False
		# Note: Not much of a point in concealing anything in HotSeat mode
		if (not pActiveTeam.isHasMet(eTeam) and not g.isDebugMode() and pPlayer.isAlive() and g.isNetworkMultiPlayer()):
			# (not all uses of these variables are tagged with comments)
			bConcealCiv = pPlayer.wasCivRandomlyChosen()
			bConcealLeader = pPlayer.wasLeaderRandomlyChosen()
			if bConcealCiv:
				kGPColor = gc.getColorInfo(self.colorGreatPeopleStored)
				rgba = kGPColor.getColor()
				iUnmetR = int(255 * rgba.r)
				iUnmetG = int(255 * rgba.g)
				iUnmetB = int(255 * rgba.b)
				iUnmetA = int(255 * rgba.a)
		# </advc.190d>
		if ScoreOpt.isUsePlayerName():
			szPlayerName = pPlayer.getName()
		else:
			szPlayerName = gc.getLeaderHeadInfo(pPlayer.getLeaderType()).getDescription()
		if not bConcealCiv:
			if ScoreOpt.isShowBothNames():
				szCivName = pPlayer.getCivilizationShortDescription(0)
				szPlayerName = szPlayerName + "/" + szCivName
			elif ScoreOpt.isShowBothNamesShort():
				szCivName = pPlayer.getCivilizationDescription(0)
				szPlayerName = szPlayerName + "/" + szCivName
			elif ScoreOpt.isShowLeaderName():
				pass
			elif ScoreOpt.isShowCivName():
				szCivName = pPlayer.getCivilizationShortDescription(0)
				szPlayerName = szCivName
			else:
				szCivName = pPlayer.getCivilizationDescription(0)
				szPlayerName = szCivName
		if not pPlayer.isAlive() and ScoreOpt.isShowDeadTag():
			szPlayerScore = self.szTextDeadCiv
			if bAlignIcons:
				scores.setScore(szPlayerScore)
		else:
			iScore = g.getPlayerScore(ePlayer)
			szPlayerScore = u"%d" % iScore
			# <advc.155> (To allow this option without the Advanced/Tabular layout option, simply remove the bAlignIcons check.)
			if (bAlignIcons and gc.getTeam(eTeam).getAliveCount() > 1 and ScoreOpt.isColorCodeTeamScore() and not bConcealCiv):
				pTeamLeader = gc.getPlayer(gc.getTeam(eTeam).getLeaderID())
				szPlayerScore = u"<color=%d,%d,%d,%d>%s</color>" %(pTeamLeader.getPlayerTextColorR(), pTeamLeader.getPlayerTextColorG(), pTeamLeader.getPlayerTextColorB(), pTeamLeader.getPlayerTextColorA(), szPlayerScore)
			# </advc.155>
			if bAlignIcons:
				scores.setScore(szPlayerScore)
# BUG - Score Delta - start
			if ScoreOpt.isShowScoreDelta():
				iGameTurn = g.getGameTurn()
				if ePlayer >= eActivePlayer:
					iGameTurn -= 1
				if ScoreOpt.isScoreDeltaIncludeCurrentTurn():
					iScoreDelta = iScore
				elif iGameTurn >= 0:
					iScoreDelta = pPlayer.getScoreHistory(iGameTurn)
				else:
					iScoreDelta = 0
				iPrevGameTurn = iGameTurn - 1
				if iPrevGameTurn >= 0:
					iScoreDelta -= pPlayer.getScoreHistory(iPrevGameTurn)
				if iScoreDelta != 0:
					if iScoreDelta > 0:
						# advc.085: To match the color I (might) use for tech progress. Was just GREEN.
						iColorType = self.colorAltHighlightText
					elif iScoreDelta < 0:
						iColorType = self.colorRed
					szScoreDelta = "%+d" % iScoreDelta
					if iColorType >= 0:
						szScoreDelta = localText.changeTextColor(szScoreDelta, iColorType)
					szPlayerScore += szScoreDelta + u" "
					if bAlignIcons:
						scores.setScoreDelta(szScoreDelta)
# BUG - Score Delta - end
		if (not CyInterface().isFlashingPlayer(ePlayer) or CyInterface().shouldFlash(ePlayer)):
			if ePlayer == eActivePlayer:
				szPlayerName = u"[<color=%d,%d,%d,%d>%s</color>]" %(pPlayer.getPlayerTextColorR(), pPlayer.getPlayerTextColorG(), pPlayer.getPlayerTextColorB(), pPlayer.getPlayerTextColorA(), szPlayerName)
			else:
				if not pPlayer.isAlive() and ScoreOpt.isGreyOutDeadCivs():
					szPlayerName = u"<color=%d,%d,%d,%d>%s</color>" %(175, 175, 175, pPlayer.getPlayerTextColorA(), szPlayerName)
				# <advc.190d>
				elif bConcealCiv:
					szPlayerName = u"<color=%d,%d,%d,%d>%s</color>" %(iUnmetR, iUnmetG, iUnmetB, iUnmetA, szPlayerName)
				# </ advc.190d>
				else:
					szPlayerName = u"<color=%d,%d,%d,%d>%s</color>" %(pPlayer.getPlayerTextColorR(), pPlayer.getPlayerTextColorG(), pPlayer.getPlayerTextColorB(), pPlayer.getPlayerTextColorA(), szPlayerName)
		szTempBuffer = u"%s: %s" %(szPlayerScore, szPlayerName)
		szBuffer = szBuffer + szTempBuffer
		if bAlignIcons:
			scores.setName(szPlayerName)
			# <advc.190d>
			if bConcealCiv:
				scores.setID(u"<color=%d,%d,%d,%d>%d</color>" %(iUnmetR, iUnmetG, iUnmetB, iUnmetA, ePlayer))
			else: # </advc.190d>
				scores.setID(u"<color=%d,%d,%d,%d>%d</color>" %(pPlayer.getPlayerTextColorR(), pPlayer.getPlayerTextColorG(), pPlayer.getPlayerTextColorB(), pPlayer.getPlayerTextColorA(), ePlayer))
		if pPlayer.isAlive():
			if bAlignIcons:
				scores.setAlive()
			# BUG: Rest of Dead Civs change is merely indentation by 1 level ...
			#if pTeam.isAlive():
				# if not pActiveTeam.isHasMet(eTeam):
					# szBuffer = szBuffer + (" ?")
					# if bAlignIcons:
						# scores.setNotMet()
			# K-Mod
			if pTeam.isAlive() and not pActiveTeam.isHasMet(eTeam):
				szBuffer = szBuffer + (" ?")
				if bAlignIcons:
					scores.setNotMet()
			# K-Mod end
			# <kekm.30>
			if bAlignIcons:
				scores.setLeaderIcon(pPlayer.getLeaderType())
				scores.setCivIcon(pPlayer.getCivilizationType())
			# </kekm.30>
			# K-Mod
			if (pTeam.isAlive() and (pActiveTeam.isHasMet(eTeam) or g.isDebugMode())):
			# K-Mod end
				if pTeam.isAtWar(eActiveTeam):
					szBuffer += "("
					szBuffer += localText.getColorText("TXT_KEY_CONCEPT_WAR", (), self.colorRed).upper()
					szBuffer += ")"
					if bAlignIcons:
						scores.setWar()
				elif pActiveTeam.isForcePeace(eTeam):
					if bAlignIcons:
						scores.setPeace()
				elif pTeam.isAVassal():
					for eMasterTeam in range(gc.getMAX_TEAMS()):
						if (pTeam.isVassal(eMasterTeam) and pActiveTeam.isForcePeace(eMasterTeam)):
							if bAlignIcons:
								scores.setPeace()
							break
				if (pPlayer.canTradeNetworkWith(eActivePlayer) and ePlayer != eActivePlayer):
					szBuffer = szBuffer + self.szTradeIcon
					if bAlignIcons:
						scores.setTrade()
				if pTeam.isOpenBorders(eActiveTeam):
					szBuffer = szBuffer + self.szOpenBordersIcon
					if bAlignIcons:
						scores.setBorders()
				if pTeam.isDefensivePact(eActiveTeam):
					szBuffer = szBuffer + self.szDefensivePactIcon
					if bAlignIcons:
						scores.setPact()
				eStateReligion = pPlayer.getStateReligion()
				if eStateReligion != -1:
					if pPlayer.hasHolyCity(eStateReligion):
						szTempBuffer = u"%c" %(gc.getReligionInfo(eStateReligion).getHolyCityChar())
					else:
						szTempBuffer = u"%c" %(gc.getReligionInfo(eStateReligion).getChar())
					szBuffer = szBuffer + szTempBuffer
					if bAlignIcons:
						scores.setReligion(szTempBuffer)
				# BUG - 3.17: isEspionage check
				# advc.120h: Second condition was pTeam.getEspionagePointsAgainstTeam(eActiveTeam) < pActiveTeam.getEspionagePointsAgainstTeam(eTeam)
				if (GameUtil.isEspionage() and pActivePlayer.getEspionageSpendingWeightAgainstTeam(eTeam) > 0):
					szTempBuffer = u"%c" %(gc.getCommerceInfo(CommerceTypes.COMMERCE_ESPIONAGE).getChar())
					szBuffer = szBuffer + szTempBuffer
					if bAlignIcons:
						scores.setEspionage()
				# <advc.085>
				if (ePlayer != eActivePlayer and (pPlayer.isGoldenAge() or pPlayer.isAnarchy())):
					# szBuffer += # Perhaps tbd. (non-tabular layout)
					if bAlignIcons:
						scores.setGoldenAge(pPlayer.isAnarchy())
				# </advc.085>
			# K-Mod (original code deleted)
			if (g.isDebugMode() or (pActivePlayer.canSeeResearch(ePlayer) and (eTeam != eActiveTeam or pActiveTeam.getNumMembers() > 1))):
			# K-Mod end
				eCurrentResearch = pPlayer.getCurrentResearch() # advc
				iProgressPercent = 0 # advc.085: Show that even when no current research
				if eCurrentResearch != -1:
					szTempBuffer = u"-%s" %gc.getTechInfo(eCurrentResearch).getDescription()
					# <advc.004x>
					#iTurnsLeft = pPlayer.getResearchTurnsLeft(eCurrentResearch, True)
					#if iTurnsLeft >= 0:
					# </advc.004x>
						#szTempBuffer = u"-%s (%d)" %(gc.getTechInfo(eCurrentResearch).getDescription(), iTurnsLeft)
					# <advc.085> Replacing the above
					iCurrentCost = pTeam.getResearchCost(eCurrentResearch)
					if iCurrentCost > 0:
						iProgressPercent = pTeam.getResearchProgress(eCurrentResearch) * 100
						iProgressPercent /= iCurrentCost
						iProgressPercent = min(iProgressPercent, 99)
						iProgressPercent = max(iProgressPercent, 0)
				szTempBuffer += u" %d%%" % iProgressPercent
				# Don't req. eCurrentResearch!=-1 unless in Debug mode
				if eCurrentResearch != -1 or not gc.getGame().isDebugMode():
					szBuffer += szTempBuffer
					if bAlignIcons:
						# Pass along iProgressPercent instead of iTurnsLeft
						scores.setResearch(eCurrentResearch, iProgressPercent)
					# </advc.085>
			# BUG (Dead Civs): ...end of indentation
# BUG - Dead Civs - end
# BUG - Power Rating - start
			# if on, show according to espionage "see demographics" mission
			# K-Mod (original BUG condition deleted)
			if (ScoreOpt.isShowPower() and eActivePlayer != ePlayer and (g.isDebugMode() or pActivePlayer.canSeeDemographics(ePlayer))):
				iPlayerPower = pActivePlayer.getPower()
				iPowerColor = ScoreOpt.getPowerColor()
				iHighPowerColor = ScoreOpt.getHighPowerColor()
				iLowPowerColor = ScoreOpt.getLowPowerColor()
				iPower = pPlayer.getPower()
				if iPower > 0: # avoid divide by zero
					fPowerRatio = float(iPlayerPower) / float(iPower)
					if ScoreOpt.isPowerThemVersusYou():
						if fPowerRatio > 0:
							fPowerRatio = 1.0 / fPowerRatio
						else:
							fPowerRatio = 999.0
					szTempBuffer = BugUtil.formatFloat(fPowerRatio, ScoreOpt.getPowerDecimals())
					szTempBuffer += self.szStrengthIcon
					# <advc.085> Don't color the power ratios of teammates
					bAlly = (eTeam == eActiveTeam)
					iColor = 0 # Want to pass that to widget help </advc.085>
					if (iHighPowerColor >= 0 and not bAlly and fPowerRatio >= ScoreOpt.getHighPowerRatio()):
						iColor = iHighPowerColor
					elif (iLowPowerColor >= 0 and not bAlly and fPowerRatio <= ScoreOpt.getLowPowerRatio()):
						iColor = iLowPowerColor
					elif (iPowerColor >= 0):
						iColor = iPowerColor
					szTempBuffer = localText.changeTextColor(szTempBuffer, iColor)
					szBuffer = szBuffer + u" " + szTempBuffer
					if bAlignIcons:
						scores.setPower(szTempBuffer, iColor)
# BUG - Power Rating - end
# BUG - Attitude Icons - start
			if ScoreOpt.isShowAttitude():
				if not pPlayer.isHuman() and eActivePlayer != ePlayer:
					iAtt = pPlayer.AI_getAttitude(eActivePlayer)
					#cAtt = unichr(ord(unichr(g.getSymbolID(FontSymbols.POWER_CHAR) + 4)) + iAtt)
					# advc.187: I've added the airport icon as a GameFont_75 symbol and that breaks the offset used above. No cells are left for further insertions, so, I guess, at this point, the offset from POWER_CHAR can't break again - but let's do it a bit more cleanly anyway by exposing the leftmost attitude char to Python.
					cAtt = unichr(ord(unichr(self.iWorstAttitudeIcon + iAtt)))
					szBuffer += cAtt
					if bAlignIcons:
						scores.setAttitude(cAtt)
# BUG - Attitude Icons - end
# BUG - Refuses to Talk - start
			if not DiplomacyUtil.isWillingToTalk(ePlayer, eActivePlayer):
				cRefusesToTalk = u"!"
				szBuffer += cRefusesToTalk
				if bAlignIcons:
					scores.setWontTalk()
# BUG - Refuses to Talk - end
# BUG - Worst Enemy - start
			if ScoreOpt.isShowWorstEnemy():
				if AttitudeUtil.isWorstEnemy(ePlayer, eActivePlayer):
					szBuffer += self.szAngryPopIcon
					if bAlignIcons:
						scores.setWorstEnemy()
# BUG - Worst Enemy - end
# BUG - WHEOOH - start
			#if ScoreOpt.isShowWHEOOH():
			# advc.104: Enable iff UWAI is disabled. But the icon still won't show up unless it gets added to the scoreboard string.
			if CyGame().useKModAI():
				if PlayerUtil.isWHEOOH(ePlayer, PlayerUtil.getActivePlayerID()):
					szBuffer = szBuffer + self.szOccupationIcon
					if bAlignIcons:
						scores.setWHEOOH()
# BUG - WHEOOH - end
# BUG - Num Cities - start
			if ScoreOpt.isShowCountCities():
				# if PlayerUtil.canSeeCityList(ePlayer):
					# szTempBuffer = u"%d" % PlayerUtil.getNumCities(ePlayer)
				# else:
					# szTempBuffer = BugUtil.colorText(u"%d" % PlayerUtil.getNumRevealedCities(ePlayer), "COLOR_CYAN")
				# K-Mod. count revealed cities only.
				if g.isDebugMode():
					szTempBuffer = u"%d" % pPlayer.getNumCities()
				else:
					szTempBuffer = u"%d" % PlayerUtil.getNumRevealedCities(ePlayer)
				# K-Mod end
				# advc.085: Make city counts gray
				szTempBuffer = localText.changeTextColor(szTempBuffer, self.colorBuildingText)
				szBuffer = szBuffer + " " + szTempBuffer
				if bAlignIcons:
					scores.setNumCities(szTempBuffer)
# BUG - Num Cities - end
		if g.isNetworkMultiPlayer():
			szTempBuffer = CyGameTextMgr().getNetStats(ePlayer)
			szBuffer = szBuffer + szTempBuffer
			if bAlignIcons:
				scores.setNetStats(szTempBuffer)
		if pPlayer.isHuman() and CyInterface().isOOSVisible():
			# K-Mod, added TurnSlice
			szTempBuffer = u" <color=255,0,0>* %s *</color> (%d)" %(CyGameTextMgr().getOOSSeeds(ePlayer), g.getTurnSlice()%8)
			szBuffer = szBuffer + szTempBuffer
			if bAlignIcons:
				scores.setNetStats(szTempBuffer)
		return szBuffer

	# Will update the help Strings
	def updateHelpStrings(self):
		screen = self.screen
		if (CyInterface().getShowInterface() == InterfaceVisibility.INTERFACE_HIDE_ALL):
			screen.setHelpTextString("")
		else:
			szHelpText = CyInterface().getHelpString()
			if (szHelpText.find("<font=") == -1):
				szTag = sasFontTagHover._build()
				if szTag:
					szHelpText = szTag + szHelpText + SAS_FONT_TAG_CLOSE
			screen.setHelpTextString(szHelpText)
		return 0

	# Will set the promotion button position
	def setPromotionButtonPosition(self, szName, iPromotionCount):
# BUG - Stack Promotions - start
		x, y = self.calculatePromotionButtonPosition(iPromotionCount)
		gRect(szName).moveTo(x, y)
		self.screen.moveItem(szName, x, y, -0.3)
		return x, y
# BUG - Stack Promotions - end

	def calculatePromotionButtonPosition(self, iPromotionCount):
		iBtnSize = gRect("PromotionButton0").size()
		iAvailH = max(6 * iBtnSize, gRect("Top").yBottom() - gRect("SelectedUnitPanel").y())
		iBtnPerCol = iAvailH / iBtnSize # was hardcoded to 6
		return (gRect("SelectedUnitPanel").xRight() - iBtnSize - HSPACE(-2) - (iBtnSize * (iPromotionCount / iBtnPerCol)), gRect("Top").yBottom() - iAvailH + (iBtnSize * (iPromotionCount % iBtnPerCol)))

	# advc.092: Merged into updateResearchButtons
	#def setResearchButtonPosition(self, szButtonID, iCount)

	# advc: Unused; already unused in Civ4 v1.0.
	#
	# def setScoreTextPosition(self, szButtonID, iWhichLine):
	# 	screen = self.screen
	# 	yResolution = screen.getYResolution()
	# 	if (CyInterface().getShowInterface() == InterfaceVisibility.INTERFACE_SHOW):
	# 		yCoord = yResolution - 180
	# 	else:
	# 		yCoord = yResolution - 88
	# 	screen.moveItem(szButtonID, 996, yCoord - (iWhichLine * 18), -0.3)
	#

	# Will build the globeview UI
	# advc.004z: Returns True iff the current layer has options
	def updateGlobeviewButtons(self):
		kInterface = CyInterface()
		screen = self.screen
		kEngine = CyEngine()
		kGLM = CyGlobeLayerManager()
		#iNumLayers = kGLM.getNumLayers() # advc: unused
		iCurrentLayerID = kGLM.getCurrentLayerID()
		# <advc.004m>
		# The layer id is meaningless to the DLL. Translate to enum type.
		eCurrentLayerType = GlobeLayerTypes.NO_GLOBE_LAYER
		if iCurrentLayerID >= 0:
			szLayerName = kGLM.getLayer(iCurrentLayerID).getName()[:3]
			if szLayerName == "STR":
				eCurrentLayerType = GlobeLayerTypes.GLOBE_LAYER_STRATEGY
			elif szLayerName == "TRA":
				eCurrentLayerType = GlobeLayerTypes.GLOBE_LAYER_TRADE
			elif szLayerName == "UNI":
				eCurrentLayerType = GlobeLayerTypes.GLOBE_LAYER_UNIT
			elif szLayerName == "RES":
				eCurrentLayerType = GlobeLayerTypes.GLOBE_LAYER_RESOURCE
			elif szLayerName == "REL":
				eCurrentLayerType = GlobeLayerTypes.GLOBE_LAYER_RELIGION
			elif szLayerName == "CUL":
				eCurrentLayerType = GlobeLayerTypes.GLOBE_LAYER_CULTURE
		gc.getGame().reportCurrentLayer(eCurrentLayerType)
		# </advc.004m>

		# Positioning things based on the visibility of the globe

		self.setDefaultHelpTextArea(kEngine.isGlobeviewUp() or CyInterface().getShowInterface() != InterfaceVisibility.INTERFACE_SHOW)

		# Hide the layer options ... all of them
		for i in range(20):
			screen.hide("GlobeLayerOption" + str(i))

		# Setup the GlobeLayer panel
		#iNumLayers = kGLM.getNumLayers() # advc: unused
		if (not kEngine.isGlobeviewUp() or CyInterface().getShowInterface() == InterfaceVisibility.INTERFACE_HIDE_ALL):
			# advc: Do this branch first (to reduce indentation)
			if iCurrentLayerID != -1:
				kLayer = kGLM.getLayer(iCurrentLayerID)
				if kLayer.getName() == "RESOURCES":
					screen.setState("ResourceIcons", True)
				else:
					screen.setState("ResourceIcons", False)
				if kLayer.getName() == "UNITS":
					screen.setState("UnitIcons", True)
				else:
					screen.setState("UnitIcons", False)
			else:
				screen.setState("ResourceIcons", False)
				screen.setState("UnitIcons", False)

			screen.setState("Grid", CyUserProfile().getGrid())
			screen.setState("BareMap", CyUserProfile().getMap())
			screen.setState("Yields", CyUserProfile().getYields())
			screen.setState("ScoresVisible", CyUserProfile().getScores())

			screen.hide("InterfaceGlobeLayerPanel")
			screen.setState("GlobeToggle", False)
			return False

		# set up panel
		# <advc.004z>
		bUnitLayer = (eCurrentLayerType == GlobeLayerTypes.GLOBE_LAYER_UNIT)
		bResourceLayer = (eCurrentLayerType == GlobeLayerTypes.GLOBE_LAYER_RESOURCE)
		# Could instead set NumOptions to 0 in CvGame::getGlobeLayers but then the resource layer wouldn't update properly when ResourceIconOptions is toggled on the BUG menu.
		if (iCurrentLayerID >= 0 and kGLM.getLayer(iCurrentLayerID).getNumOptions() > 0 and (not bResourceLayer or MainOpt.isResourceIconOptions())):
			# </advc.004z>
			bHasOptions = True
		else:
			bHasOptions = False
			screen.hide("ScoreBackground")

		# set up toggle button
		screen.setState("GlobeToggle", True)

		# Set GlobeLayer indicators correctly
		for i in range(kGLM.getNumLayers()):
			screen.setState("GlobeLayer" + str(i), iCurrentLayerID == i)

		# Set up options pane
		if not bHasOptions:
			return False
		iGlobeLayerOptionHeight = 25 # advc.002b: was 24
		iHMargin = gRect("Top").xRight() - gRect("MiniMap").xRight()
		iCurY = gRect("GlobeToggle").y()
		kLayer = kGLM.getLayer(iCurrentLayerID)
		iNumOptions = kLayer.getNumOptions()
		iCurOption = kLayer.getCurrentOption()
		iMaxTextWidth = -1
		for iTmp in range(iNumOptions):
			iOption = iTmp # iNumOptions - iTmp - 1
			# <advc.004z> Skip dummy option (see CvEnums.h)
			if bUnitLayer and iOption == 2:
				continue # </advc.004z>
			szName = "GlobeLayerOption" + str(iOption)
			szCaption = kLayer.getOptionName(iOption)
			# advc.004z: Highlight "All Units" option when the default (2) is selected.
			# This is the case when none of the options has been clicked yet.
			if (iOption == iCurOption or (bUnitLayer and iCurOption == 2 and iOption == 0)):
				szBuffer = "  <color=0,255,0>%s</color>  " % (szCaption)
			else:
				szBuffer = "  %s  " % (szCaption)
			iCurY -= iGlobeLayerOptionHeight
			# advc.092: was iCurY-10
			screen.setText(szName, "Background", szBuffer, CvUtil.FONT_RIGHT_JUSTIFY, gRect("Top").xRight() - iHMargin, iCurY - iGlobeLayerOptionHeight / 3, -0.3, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GLOBELAYER_OPTION, iOption, -1)
			screen.show(szName)

			iTextWidth = CyInterface().determineWidth(szBuffer)
			if iTextWidth > iMaxTextWidth:
				iMaxTextWidth = iTextWidth

		iCurY -= iGlobeLayerOptionHeight #make extra space
		iPanelWidth = iMaxTextWidth + iHMargin # advc.092: was ...+32
		# (advc.092: Not a good idea after all I think)
		#iPanelWidth = max(iPanelWidth, gRect("Top").xRight() - gRect("MiniMapPanel").x() - iHMargin)
		# advc.092: was ...-14
		screen.setPanelSize("ScoreBackground", gRect("Top").xRight() - iPanelWidth - iHMargin, iCurY, iPanelWidth, gRect("GlobeToggle").y() - iCurY)
		screen.show("ScoreBackground")
		return True

	# Update minimap buttons
	def setMinimapButtonVisibility(self, bVisible):
		screen = self.screen
		kInterface = CyInterface()
		kGLM = CyGlobeLayerManager()
		if (CyInterface().isCityScreenUp()):
			bVisible = False
		# <advc>
		aMainButtons = []
		for btn in self.aMiniMapMainButtons:
			aMainButtons.append(btn.szName) # </advc>
		aGlobeButtons = []
		for i in range(kGLM.getNumLayers()):
			aGlobeButtons.append("GlobeLayer" + str(i))

		if bVisible:
			if CyEngine().isGlobeviewUp():
				aHide = aMainButtons
				aShow = aGlobeButtons
			else:
				aHide = aGlobeButtons
				aShow = aMainButtons
			screen.show("GlobeToggle")

		else:
			aHide = aMainButtons + aGlobeButtons
			aShow = []
			screen.hide("GlobeToggle")

		for szButton in aHide:
			screen.hide(szButton)

		if CyInterface().getShowInterface() == InterfaceVisibility.INTERFACE_HIDE:
			iGlobeY = gRect("Top").yBottom() - gRect("GlobeToggle").size() - VSPACE(4)
			iY = gRect("Top").yBottom() - gRect("MiniMapButton").size() - VSPACE(4)
		else:
			iGlobeY = gRect("LowerRightCornerPanel").y()
			iY = iGlobeY + gRect("GlobeToggle").size() - gRect("MiniMapButton").size()
		iGlobeX = gRect("MiniMapPanel").xRight() + 1 - gRect("GlobeToggle").size()
		# Update the layout data so that we can refer to it elsewhere
		gSetSquare("GlobeToggle", "Top", iGlobeX, iGlobeY, gRect("GlobeToggle").size())
		screen.moveItem("GlobeToggle", iGlobeX, iGlobeY, 0.0)

		# <!-- custom: adding the map-annotation button made the strip one wider, so its last button collided with the Globe toggle.
		# There was a lot of unused dead space between buttons, so step by less than the full cell to tighten the strip enough to fit the extra button without colliding. (Claude code Opus 4.7); was HSPACE(0).
		# Note: -HSPACE(5) is enough for pangea, but for some squarish maps like as of now BTG_Cross, minimap is horizontally shorter and toggle scoreboard button still collides with globe view button.
		# However this is rare, and increasing the button horizontal inter-spacing reduction would make toggle scoreboard and globe view buttons too spaced in Pangea, plus they don't fully prevent colliding in BTG_Cross, even in BTG_Cross, globe view remains fairly accessible, so not increased further. -->
		iStripButtonOverlapX = -HSPACE(5)
		iStep = gRect("MiniMapButton").size() + iStripButtonOverlapX
		#iBtnX = iGlobeX - len(aShow) * iStep - HSPACE(10)
		# advc.092: Better align left and leave any remaining space between the Globe Toggle and the smaller minimap buttons
		iX = gRect("MiniMapPanel").x()
		i = 0
		for szButton in aShow:
			screen.moveItem(szButton, iX, iY, 0.0)
			screen.moveToFront(szButton)
			if szButton == "AnnotationsVisible":
				screen.setState(szButton, self.bAnnotationsVisible)
			screen.show(szButton)
			iX += iStep
			i += 1

	def createGlobeviewButtons(self):
		screen = self.screen
		kGLM = CyGlobeLayerManager()
		for i in range(kGLM.getNumLayers()):
			szButtonID = "GlobeLayer" + str(i)
			kLayer = kGLM.getLayer(i)
			szStyle = kLayer.getButtonStyle()
			if szStyle == 0 or szStyle == "":
				szStyle = "Button_HUDSmall_Style"
			# advc (note): Same preliminary position for all of them
			lRect = gRect("MiniMapButton")
			screen.addCheckBoxGFC(szButtonID, "", "", lRect.x(), lRect.y(), lRect.size(), lRect.size(), WidgetTypes.WIDGET_GLOBELAYER, i, -1, ButtonStyles.BUTTON_STYLE_LABEL)
			screen.setStyle(szButtonID, szStyle)
			screen.hide(szButtonID)

	def createMinimapButtons(self):
		screen = self.screen
		lRect = gRect("MiniMapButton")
		# advc: Replacing a lot of redundant code
		for btn in self.aMiniMapMainButtons:
			if btn.eControl is None:
				# <!-- custom: annotations toggle has no BtS ControlInfo action, so route it through a custom widget while keeping the standard styled HUD checkbox path. (GPT-5.5) -->
				screen.addCheckBoxGFC(btn.szName, "", "", lRect.x(), lRect.y(), lRect.size(), lRect.size(), WIDGET_ANNOTATIONS_TOGGLE, -1, -1, ButtonStyles.BUTTON_STYLE_LABEL)
				screen.setStyle(btn.szName, btn.szStyle)
			else:
				# advc (note): Same preliminary position for all of them
				screen.addCheckBoxGFC(btn.szName, "", "", lRect.x(), lRect.y(), lRect.size(), lRect.size(), WidgetTypes.WIDGET_ACTION, gc.getControlInfo(btn.eControl).getActionInfoIndex(), -1, ButtonStyles.BUTTON_STYLE_LABEL)
				screen.setStyle(btn.szName, btn.szStyle)
			screen.setState(btn.szName, False)
			screen.hide(btn.szName)

		lToggle = gRect("GlobeToggle")
		screen.addCheckBoxGFC("GlobeToggle", "", "", lToggle.x(), lToggle.y(), lToggle.size(), lToggle.size(), WidgetTypes.WIDGET_ACTION, gc.getControlInfo(ControlTypes.CONTROL_GLOBELAYER).getActionInfoIndex(), -1, ButtonStyles.BUTTON_STYLE_LABEL)
		screen.setStyle("GlobeToggle", "Button_HUDZoom_Style")
		screen.setState("GlobeToggle", False)
		screen.hide("GlobeToggle")
	# <!-- custom: note: old CityOrgArea code removed since we don't put religions and corporations in the right side panel anymore -->

	def update(self, fDelta):
		return

	def forward(self):
		if (not CyInterface().isFocused() or CyInterface().isCityScreenUp()):
			if (CyInterface().isCitySelection()):
				CyGame().doControl(ControlTypes.CONTROL_NEXTCITY)
			else:
				CyGame().doControl(ControlTypes.CONTROL_NEXTUNIT)

	def back(self):
		if (not CyInterface().isFocused() or CyInterface().isCityScreenUp()):
			if (CyInterface().isCitySelection()):
				CyGame().doControl(ControlTypes.CONTROL_PREVCITY)
			else:
				CyGame().doControl(ControlTypes.CONTROL_PREVUNIT)

# BUG - Raw Yields - start
	def handleRawYieldsButtons(self, inputClass):
		iButton = inputClass.getID()
		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CURSOR_MOVE_ON):
			self.PLE.displayHelpHover(RAW_YIELD_HELP[iButton])
		elif (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CURSOR_MOVE_OFF):
			self.PLE.hideInfoPane()
		elif (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED):
			global g_bYieldView
			global g_iYieldType
			global g_iYieldTiles
			if iButton == 0:
				g_bYieldView = False
			elif iButton in (1, 2, 3):
				g_bYieldView = True
				g_iYieldType = RawYields.YIELDS[iButton - 1]
			elif iButton in (4, 5, 6):
				g_bYieldView = True
				g_iYieldTiles = RawYields.TILES[iButton - 4]
			else:
				return 0
			CyInterface().setDirty(InterfaceDirtyBits.CityScreen_DIRTY_BIT, True)
			return 1
		return 0
# BUG - Raw Yields - end

	# <!-- custom: building filter button hover tooltips based on how the C2C mod does (Claude code Sonnet 4.5 + Claude code Opus 4.5) -->
	def handleBuildingFilterButtons(self, inputClass):
		iFilter = inputClass.getID()
		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CURSOR_MOVE_ON):
			if iFilter == 0:
				self.PLE.displayHelpHover(self.szBuildFilterTooltipAll)
			elif iFilter == 1:
				self.PLE.displayHelpHover(self.szBuildFilterTooltipRegular)
			elif iFilter == 2:
				self.PLE.displayHelpHover(self.szBuildFilterTooltipNational)
			elif iFilter == 3:
				self.PLE.displayHelpHover(self.szBuildFilterTooltipWorld)
		elif (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CURSOR_MOVE_OFF):
			self.PLE.hideInfoPane()
		return 0

	# <!-- custom: map-annotation toggle helpers: hide/show dotmap overlays and cached sign/landmark billboards together. (GPT-5.5) -->
	def toggleMapAnnotations(self):
		self.bAnnotationsVisible = not self.bAnnotationsVisible
		if self.bAnnotationsVisible:
			self.showMapAnnotations()
		else:
			self.hideMapAnnotations()
		self.screen.setState("AnnotationsVisible", self.bAnnotationsVisible)

	def hideMapAnnotations(self):
		self.hideMapAnnotationSigns()
		try:
			import CvStrategyOverlay
			# <!-- custom: reuse DotMap because we both call its normal hide() and then force-clear its visual layers.
			# In-game testing showed drawings can be visible while DotMap.visible is already false; without clearCityLayers(), the first toggle hides signs but leaves Alt+X BFC drawings until toggled again. (GPT-5.5) -->
			kDotMap = CvStrategyOverlay.getDotMap()
			kDotMap.hide()
			kDotMap.clearCityLayers()
			CvStrategyOverlay.StratLayerOpt.setShowDotMap(False)
		except:
			BugUtil.warn("CvMainInterface.hideMapAnnotations failed to hide dotmap overlay")

	def showMapAnnotations(self):
		self.showMapAnnotationSigns()
		try:
			import CvStrategyOverlay
			CvStrategyOverlay.StratLayerOpt.setShowDotMap(True)
			CvStrategyOverlay.getDotMap().show()
		except:
			BugUtil.warn("CvMainInterface.showMapAnnotations failed to show dotmap overlay")

	# <!-- custom: save/load support for hidden annotations.
	# Empirically, the previous save hook re-added hidden captions during save, so captions appeared immediately while BFC drawings stayed hidden; reloading that save then showed only BFC annotations and lost the caption annotations.
	# Persisting this cache keeps the hidden state stable and lets the toggle restore captions later. (GPT-5.5) -->
	def isMapAnnotationsHiddenForSave(self):
		return not self.bAnnotationsVisible

	def getHiddenMapAnnotationSignsForSave(self):
		if not self.bAnnotationsVisible and self.aHiddenAnnotationSigns:
			return self.aHiddenAnnotationSigns
		return []

	def loadHiddenMapAnnotationSigns(self, aHiddenAnnotationSigns):
		if aHiddenAnnotationSigns is None:
			aHiddenAnnotationSigns = []
		self.aHiddenAnnotationSigns = aHiddenAnnotationSigns
		self.bAnnotationsVisible = False
		try:
			self.screen.setState("AnnotationsVisible", self.bAnnotationsVisible)
		except:
			BugUtil.warn("CvMainInterface.loadHiddenMapAnnotationSigns could not update the annotation toggle button state")

	def loadVisibleMapAnnotations(self):
		self.aHiddenAnnotationSigns = []
		self.bAnnotationsVisible = True
		try:
			self.screen.setState("AnnotationsVisible", self.bAnnotationsVisible)
		except:
			BugUtil.warn("CvMainInterface.loadVisibleMapAnnotations could not update the annotation toggle button state")

	def hideMapAnnotationSigns(self):
		kEngine = CyEngine()
		self.aHiddenAnnotationSigns = []
		for iSign in range(kEngine.getNumSigns()):
			kSign = kEngine.getSignByIndex(iSign)
			kPlot = kSign.getPlot()
			if kPlot and not kPlot.isNone():
				self.aHiddenAnnotationSigns.append((kPlot.getX(), kPlot.getY(), int(kSign.getPlayerType()), kSign.getCaption()))
		for iX, iY, ePlayer, szCaption in self.aHiddenAnnotationSigns:
			kPlot = gc.getMap().plot(iX, iY)
			if ePlayer == -1:
				kEngine.removeLandmark(kPlot)
			else:
				kEngine.removeSign(kPlot, ePlayer)

	def showMapAnnotationSigns(self):
		kEngine = CyEngine()
		for iX, iY, ePlayer, szCaption in self.aHiddenAnnotationSigns:
			kPlot = gc.getMap().plot(iX, iY)
			if isinstance(szCaption, unicode):
				# <!-- custom: this fixes the below error with the help of GPT-5.5 thanks -->
				# Traceback (most recent call last):
				#
				# File "CvScreensInterface", line 961, in handleInput
				# File "CvMainInterface", line 8203, in handleInput
				# File "CvMainInterface", line 8101, in toggleMapAnnotations
				# File "CvMainInterface", line 8116, in showMapAnnotations
				# File "CvMainInterface", line 8154, in showMapAnnotationSigns
				# ArgumentError: Python argument types in
				# 	CyEngine.addSign(CyEngine, CyPlot, CvPythonExtensions.PlayerTypes, unicode)
				# did not match C++ signature:
				# 	addSign(class CyEngine {lvalue}, class CyPlot *, int, char const *)
				# ERR: Python function handleInput failed, module CvScreensInterface
				#
				szCaption = szCaption.encode('latin_1')
			if ePlayer == -1:
				kEngine.addLandmark(kPlot, szCaption)
			else:
				kEngine.addSign(kPlot, ePlayer, szCaption)
		self.aHiddenAnnotationSigns = []

	# Will handle the input for this screen...
	def handleInput (self, inputClass):
#		BugUtil.debugInput(inputClass)
# BUG - PLE - start
		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CURSOR_MOVE_ON or inputClass.getNotifyCode() == NotifyCode.NOTIFY_CURSOR_MOVE_OFF or inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED):
			if (self.MainInterfaceInputMap.has_key(inputClass.getFunctionName())):
				return self.MainInterfaceInputMap.get(inputClass.getFunctionName())(inputClass)
			if (self.MainInterfaceInputMap.has_key(inputClass.getFunctionName() + "1")):
				return self.MainInterfaceInputMap.get(inputClass.getFunctionName() + "1")(inputClass)
# BUG - PLE - end

			# <!-- custom: fix production chooser bar auto-scrolling when we click on one of the lower rows (distracting and annoying and not necessary; the player can scroll if they want rather).
			# Fix with the help of chatgpt 5.2 thanks -->
			# 2) Update that pin only when the user clicks the city tab / scroll arrows
			# Prevent BottomButtonList from auto-jumping when showing multiple build rows
			if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED:
				fn = inputClass.getFunctionName()
				if fn.startswith("CityTab"):
					self.iCityBuildBarPinnedRow = inputClass.getData1()
					return 0
				elif fn == "CityScrollMinus" or fn == "MainCityScrollMinus":
					if self.iCityBuildBarPinnedRow is None:
						self.iCityBuildBarPinnedRow = CyInterface().getCityTabSelectionRow()
					self.iCityBuildBarPinnedRow -= 1
					return 0
				elif fn == "CityScrollPlus" or fn == "MainCityScrollPlus":
					if self.iCityBuildBarPinnedRow is None:
						self.iCityBuildBarPinnedRow = CyInterface().getCityTabSelectionRow()
					self.iCityBuildBarPinnedRow += 1
					return 0
				# <!-- custom: score scroll buttons; + scrolls down to more lower-ranked players, - scrolls back up toward rank 1.
				# updateScoreStrings clamps the offset to [0, total - rows]. (Claude code Sonnet 4.6 + ChatGPT-5.5 + GPT-5.5) -->
				elif fn == "ScoreScrollUp":
					self.bScoreScrollAutocenterActivePlayer = False
					self.iScoreScrollOffset += 1
					self.updateScoreStrings()
					return 1
				elif fn == "ScoreScrollDown":
					self.bScoreScrollAutocenterActivePlayer = False
					self.iScoreScrollOffset = max(0, self.iScoreScrollOffset - 1)
					self.updateScoreStrings()
					return 1
				# <!-- custom: fast scoreboard scroll keeps the useful medium jump but drops the too-niche fastest tier.
				# Manual scroll disables autocenter before redraw; updateScoreStrings clamps overshoot to [0, total - rows]. (Claude code Opus 4.7 + ChatGPT-5.5 + GPT-5.5) -->
				elif fn == "ScoreScrollUpFast":
					self.bScoreScrollAutocenterActivePlayer = False
					self.iScoreScrollOffset += self._scoreboardFastScrollStep()
					self.updateScoreStrings()
					return 1
				elif fn == "ScoreScrollDownFast":
					self.bScoreScrollAutocenterActivePlayer = False
					self.iScoreScrollOffset = max(0, self.iScoreScrollOffset - self._scoreboardFastScrollStep())
					self.updateScoreStrings()
					return 1
				# <!-- custom: one-click centering around the active player; unlike the persistent autocenter toggle, it does not change later manual scroll behavior. (ChatGPT-5.5) -->
				elif fn == "ScoreScrollCenterActivePlayer":
					self._centerScoreboardScrollOnActivePlayer()
					self.updateScoreStrings()
					return 1
				# <!-- custom: persistent scoreboard scroll autocenter toggle.
				# When turned on, center immediately and keep centering on later score redraws until a manual scroll button turns it off. (ChatGPT-5.5) -->
				elif fn == "ScoreScrollAutocenterActivePlayerToggle":
					self.bScoreScrollAutocenterActivePlayer = not self.bScoreScrollAutocenterActivePlayer
					if self.bScoreScrollAutocenterActivePlayer:
						self._centerScoreboardScrollOnActivePlayer()
					self.updateScoreStrings()
					return 1
				# <!-- custom: scoreboard bg style cycle; advances PanelStyle level (lazily seeded from SAS_SCOREBOARD_BG_DEFAULT_STYLE on first use), redraw recreates the panel. (Claude code Opus 4.7) -->
				elif fn == "ScoreBgStyle":
					self.iScoreBgStyleLevel = (self.iScoreBgStyleLevel + 1) % len(self.aScoreBgStyleLevels)
					self.updateScoreStrings()
					return 1
				# <!-- custom: always-expand toggle; flips bScoreAlwaysExpand so expanded columns are shown permanently. (Claude code Sonnet 4.6) -->
				elif fn == "ScoreExpandToggle":
					self.bScoreAlwaysExpand = not self.bScoreAlwaysExpand
					self.updateScoreStrings()
					return 1
				elif fn == "AnnotationsVisible":
					self.toggleMapAnnotations()
					return 1
				elif fn.startswith("BuildFilter"):
					iNewFilter = inputClass.getData1()
					if iNewFilter != self.iBuildingFilter:
						self.iBuildingFilter = iNewFilter
						CyInterface().setDirty(InterfaceDirtyBits.CityScreen_DIRTY_BIT, True)
					return 1
				elif fn.startswith("ReligionDDS") or fn.startswith("ReligionHolyCityDDS"):
					iReligion = inputClass.getData1()
					if iReligion >= 0:
						# <!-- custom: keep WIDGET_HELP_RELIGION_CITY for city-specific hover, then route clicks to Sevopedia in Python.
						# Catch the holy-city overlay too because it sits over the base icon and otherwise only hover works.
						# Import CvScreensInterface locally because it imports CvMainInterface during startup; top-level import here would be a needless circular-load risk. (Claude code Opus 4.7; GPT-5.5) -->
						import CvScreensInterface
						CvScreensInterface.pediaJumpToReligion([iReligion])
					return 1
				elif fn.startswith("CorporationDDS") or fn.startswith("CorporationHeadquarterDDS"):
					iCorp = inputClass.getData1()
					if iCorp >= 0:
						# <!-- custom: same pattern as religions: preserve city-specific corporation hover while making the icon/headquarters overlay clickable to Sevopedia. (Claude code Opus 4.7; GPT-5.5) -->
						import CvScreensInterface
						CvScreensInterface.pediaJumpToCorporation([iCorp])
					return 1
				elif fn.startswith("CityBonusBtn") or fn.startswith("CityBonusCircle") or fn.startswith("CityBonusAmount") or fn.startswith("CityBonusEffect"):
					iBonus = inputClass.getData1()
					if iBonus >= 0:
						# <!-- custom: same pattern as city religion/corporation icons: keep bonus hover on the widget, then route clicks to Sevopedia in Python because direct bonus-widget clicks were not reliable for the city-screen bonus rows. (GPT-5.5) -->
						import CvScreensInterface
						CvScreensInterface.pediaJumpToBonus([iBonus])
					return 1

# BUG - BUG Option Button - Start
			if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED:
				if inputClass.getFunctionName() == "BUGOptionsScreenWidget":
					BugOptionsScreen.showOptionsScreen()
					return 1
# BUG - BUG Option Button - End

# BUG - Raw Yields - start
		if (inputClass.getFunctionName().startswith("RawYields")):
			return self.handleRawYieldsButtons(inputClass)
# BUG - Raw Yields - end

		# <!-- custom: building filter button hover tooltips (Claude code Sonnet 4.5) -->
		if (inputClass.getFunctionName().startswith("BuildFilter")):
			return self.handleBuildingFilterButtons(inputClass)
		# <!-- custom: end - building filter button hover tooltips -->

# BUG - Great Person Bar - start
		# <!-- custom: support GP bar clicks in both one-line/two-line layouts and on GP text itself. (GPT-5.3-Codex) -->
		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED and (inputClass.getFunctionName().startswith("TwoLineGPBar") or inputClass.getFunctionName().startswith("OneLineGPBar") or inputClass.getFunctionName().startswith("GreatPersonBarText"))):
			iCity = inputClass.getData1()
			# <!-- custom: prefer widget city id when available for direct city resolution. (GPT-5.3-Codex) -->
			if (iCity != -1):
				pCity = gc.getActivePlayer().getCity(iCity)
			else:
				# <!-- custom: fallback to the currently displayed GP city if widget data is missing. (GPT-5.3-Codex) -->
				pCity, _ = GPUtil.getDisplayCity()
			if pCity and not pCity.isNone():
				# <!-- custom: true opens the selected city's city screen from map view. (GPT-5.3-Codex) -->
				CyInterface().selectCity(pCity, true)
			return 1
# BUG - Great Person Bar - end

# BUG - field of view slider - start
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_SLIDER_NEWSTOP:
			if inputClass.getFunctionName() == "FoVSlider":
				screen = self.screen
				#self.iField_View = inputClass.getData() + 1
				# <advc.090>
				# SLIDER_NEWSTOP triggers both on moving the slider and on hovering over the slider.
				# I don't think the two cases can be told apart.
				#BugUtil.debugInput(inputClass, True)
				iPos = inputClass.getData()
				iPrevPos = self.iFoVPos_Prev
				self.iFoVPos_Prev = iPos
				#print "slider new stop: pos=" + str(iPos) + ", prevPos=" + str(iPrevPos)
				# For some reason, the position jumps a bit upon releasing the slider.
				# I've tried to compensate, and it works somewhat, but can also make the slider move back and forth when hovering. Not usable.
				#if iPrevPos > 0:
				#	if iPrevPos < iPos:
				#		iPos += 1
				#	elif iPrevPos > iPos:
				#		iPos -= 1
				self.iField_View = self.sliderPosToFoV(iPos)
				#print "iField_View=" + str(self.iField_View)
				# </advc.090>
				self.setFieldofView(screen, False)
				self.setFieldofView_Text(screen)
				MainOpt.setFieldOfView(self.iField_View)
		return 0

	# <advc.090>
	def sliderPosToFoVPercent(self, iPos): # iPos is between 0 and iW - 1
		fInterval = float(self.iFoVLabelUpper - self.iFoVLabelLower)
		return floor((iPos * fInterval) / gRect("FoVSlider").width() + self.iFoVLabelLower)
	# Inverse of the above
	def FoVPercentToSliderPos(self, iPercent):
		r = (iPercent - self.iFoVLabelLower) * gRect("FoVSlider").width()
		fInterval = float(self.iFoVLabelUpper - self.iFoVLabelLower)
		return ceil(r / fInterval)

	def PercentToFoV(self, iPercent):
		r = iPercent
		if r <= 75:
			return r
		return 2 * r - 75

	def FoVToPercent(self, iFoV):
		r = iFoV
		if r <= 75:
			return r
		return floor((75 + r) / 2)

	def sliderPosToFoV(self, iPos):
		return self.PercentToFoV(self.sliderPosToFoVPercent(iPos))

	def FoVToSliderPos(self, iFoV):
		return self.FoVPercentToSliderPos(self.FoVToPercent(iFoV))
	# </advc.090>

	def setFieldofView(self, screen, bDefault):
		try: # advc.009b
			# K-Mod
			#if bDefault or not MainOpt.isShowFieldOfView():
			# advc.004m: Only remember sensible values
			if (bDefault or (not MainOpt.isShowFieldOfView() and not MainOpt.isRememberFieldOfView()) or int(MainOpt.getFieldOfView()) < 10):
				self._setFieldofView(screen, self.DEFAULT_FIELD_OF_VIEW)
			else:
				self._setFieldofView(screen, self.iField_View)
		# <advc.009b>
		except AttributeError:
			pass # </advc.009b>

	def _setFieldofView(self, screen, iFoV):
		if self.iField_View_Prev != iFoV:
			gc.setDefineFLOAT("FIELD_OF_VIEW", float(iFoV))
			self.iField_View_Prev = iFoV

	def setFieldofView_Text(self, screen):
		# advc.090: was self.iField_View
		zsFieldOfView_Text = "%s [%i]" % (self.sFieldOfView_Text, self.FoVToPercent(self.iField_View))
		self.setLabel("FoVSliderText", "", zsFieldOfView_Text, CvUtil.FONT_RIGHT_JUSTIFY, FontTypes.GAME_FONT)
# BUG - field of view slider - end

# advc:
class MiniMapButton:
	def __init__(self, szName, eControl, szStyle):
		self.szName = szName
		self.eControl = eControl
		self.szStyle = szStyle
