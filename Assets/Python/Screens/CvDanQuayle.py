## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
from CvPythonExtensions import *
import PyHelpers
import ScreenInput
import CvScreenEnums
import CvUtil
import CvGameUtils
import CvScreensInterface
from SASFontUtils import *
from SASUtils import getInfoTypeOrFail
import SASTextScale

# globals
gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class CvDanQuayle:

	def __init__(self):
		self.SCREEN_NAME = "CvDanQuayle"
		self.WIDGET_ID = "CvDanQuayleWidget"
		self.EXIT_ID = "CvDanQuayleExitWidget"
		self.BACKGROUND_ID = "CvDanQuayleBackground"
		self.LEADERHEAD_ID = "CvDanQuayleLeaderhead"
		self.LIST_ID = "CvDanQuayleList"
		self.TEXT_ID = "CvDanQuayleText"
		self.SCORE_ID = "CvDanQuayleScore"
		self.SCORE_ID = "CvDanQuayleScore"
		self.WIDGET_HEADER = "CvDanQuayleHeader"

		self.X_SCREEN = 500
		self.Y_SCREEN = 396
		self.W_SCREEN = 1024
		self.H_SCREEN = 768
		self.Y_TITLE = 12
		self.X_EXIT = 994
		self.Y_EXIT = 726

		self.X_LEADERHEAD = 120
		self.Y_LEADERHEAD = 70
		self.W_LEADERHEAD = 390
		self.H_LEADERHEAD = 500

		self.X_LIST = 570
		self.Y_LIST = 130
		self.W_LIST = 300
		self.H_LIST = 560

		self.X_SCORE = 570
		self.Y_SCORE = 70
		self.W_SCORE = 300
		self.H_SCORE = 50

		self.X_TEXT = 120
		self.Y_TEXT = 590
		self.W_TEXT = 390
		self.H_TEXT = 100

		self.leaders = ["TXT_KEY_DQ_LEADER_NAME_1", "TXT_KEY_DQ_LEADER_NAME_2", "TXT_KEY_DQ_LEADER_NAME_3", "TXT_KEY_DQ_LEADER_NAME_4", "TXT_KEY_DQ_LEADER_NAME_5", "TXT_KEY_DQ_LEADER_NAME_6", "TXT_KEY_DQ_LEADER_NAME_7", "TXT_KEY_DQ_LEADER_NAME_8", "TXT_KEY_DQ_LEADER_NAME_9", "TXT_KEY_DQ_LEADER_NAME_10", "TXT_KEY_DQ_LEADER_NAME_11", "TXT_KEY_DQ_LEADER_NAME_12", "TXT_KEY_DQ_LEADER_NAME_13", "TXT_KEY_DQ_LEADER_NAME_14", "TXT_KEY_DQ_LEADER_NAME_15", "TXT_KEY_DQ_LEADER_NAME_16", "TXT_KEY_DQ_LEADER_NAME_17", "TXT_KEY_DQ_LEADER_NAME_18", "TXT_KEY_DQ_LEADER_NAME_19", "TXT_KEY_DQ_LEADER_NAME_20"]

		self.nWidgetCount = 0
		# <!-- custom: cached max-score denominator; five defines combined into one formula that never changes. (Claude code Sonnet 4.6) -->
		self.iMaxScore = None

	def getScreen(self):
		return CyGInterfaceScreen(self.SCREEN_NAME, CvScreenEnums.DAN_QUAYLE_SCREEN)

	def interfaceScreen (self):

		replayInfo = CyGame().getReplayInfo()
		if replayInfo.isNone():
			replayInfo = CyReplayInfo()
			replayInfo.createInfo(gc.getGame().getActivePlayer())

		screen = self.getScreen()
		if screen.isActive():
			return
		self.W_SCREEN = screen.getXResolution()
		self.H_SCREEN = screen.getYResolution()
		self.X_SCREEN = self.W_SCREEN / 2
		self.X_EXIT = self.W_SCREEN - 30
		self.Y_EXIT = self.H_SCREEN - 42

		# <!-- custom: Your Place in History layout pass: use fullscreen bounds, keep right score/ranking column anchored, maximize left leaderhead vertically, and place the "during this game ..." panel in the center gap.
		# This avoids ranking-list scroll while giving the NIF more lateral room and keeps text upscaling support from SAS font tags/helpers. (GPT-5.3-Codex) -->
		iLeftMargin = 45
		iRightMargin = 45
		iColumnGap = 25
		self.W_LIST = 290
		self.X_LIST = self.W_SCREEN - iRightMargin - self.W_LIST
		self.X_SCORE = self.X_LIST
		self.W_SCORE = self.W_LIST
		iAvailableLeftAndCenter = self.X_LIST - iLeftMargin - iColumnGap
		self.W_LEADERHEAD = min(max(560, int(iAvailableLeftAndCenter * 0.82)), 780)
		self.X_LEADERHEAD = iLeftMargin
		iGapLeft = self.X_LEADERHEAD + self.W_LEADERHEAD + iColumnGap
		iGapRight = self.X_LIST - iColumnGap

		iContentTop = 60
		iContentBottom = self.H_SCREEN - 58
		self.Y_SCORE = iContentTop
		self.H_SCORE = 50
		self.Y_LIST = self.Y_SCORE + self.H_SCORE + 10
		self.H_LIST = iContentBottom - self.Y_LIST
		self.W_TEXT = min(360, max(260, iGapRight - iGapLeft - 20))
		self.H_TEXT = 200
		iGapHeight = iContentBottom - iContentTop
		self.X_TEXT = iGapLeft + max(0, ((iGapRight - iGapLeft) - self.W_TEXT) / 2)
		self.Y_TEXT = iContentTop + max(0, (iGapHeight - self.H_TEXT) / 2)
		self.Y_LEADERHEAD = iContentTop
		self.H_LEADERHEAD = iContentBottom - iContentTop
		screen.setRenderInterfaceOnly(True)
		screen.showScreen( PopupStates.POPUPSTATE_IMMEDIATE, False)

		# Set the background and exit button, and show the screen
		screen.setDimensions(0, 0, self.W_SCREEN, self.H_SCREEN)

		screen.addDDSGFC(self.BACKGROUND_ID, ArtFileMgr.getInterfaceArtInfo("SCREEN_BG_OPAQUE").getPath(), 0, 0, self.W_SCREEN, self.H_SCREEN, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		screen.addPanel( "TechTopPanel", u"", u"", True, False, 0, 0, self.W_SCREEN, 55, PanelStyles.PANEL_STYLE_TOPBAR )
		screen.addPanel( "TechBottomPanel", u"", u"", True, False, 0, self.H_SCREEN - 55, self.W_SCREEN, 55, PanelStyles.PANEL_STYLE_BOTTOMBAR )

		screen.showWindowBackground(False)
		screen.setText(self.EXIT_ID, "Background", sasFontTagTitle + localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT", ()).upper() + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, self.X_EXIT, self.Y_EXIT, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_CLOSE_SCREEN, -1, -1 )

		# Header...
		screen.setLabel(self.WIDGET_HEADER, "Background", sasFontTagTitle.bold + localText.getText("TXT_KEY_GAME_END_SCREEN_TITLE", ()).upper() + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, self.X_SCREEN, self.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# Leaderhead
		screen.addLeaderheadGFC(self.LEADERHEAD_ID, replayInfo.getLeader(replayInfo.getActivePlayer()), AttitudeTypes.ATTITUDE_PLEASED, self.X_LEADERHEAD, self.Y_LEADERHEAD, self.W_LEADERHEAD, self.H_LEADERHEAD, WidgetTypes.WIDGET_GENERAL, -1, -1)

		iScore = replayInfo.getNormalizedScore()
		if self.iMaxScore is None:
			self.iMaxScore = ((100 + gc.getDefineINT("SCORE_VICTORY_PERCENT")) * (gc.getDefineINT("SCORE_POPULATION_FACTOR") + gc.getDefineINT("SCORE_LAND_FACTOR") + gc.getDefineINT("SCORE_WONDER_FACTOR") + gc.getDefineINT("SCORE_TECH_FACTOR"))) / 100
		iMaxScore = self.iMaxScore
		if iMaxScore > 0:
			iNormalScore = iScore/float(iMaxScore)
		else:
			iNormalScore = 0
		# <advc.043> Changed all the thresholds. And check >= instead of >.
		if iNormalScore >= 10: # was 1.5
			szLeaderText = self.leaders[0]
		elif iNormalScore >= 7.5: # was 1.4
			szLeaderText = self.leaders[1]
		elif iNormalScore >= 5: # was 1.3
			szLeaderText = self.leaders[2]
		elif iNormalScore >= 4: # was 1.2
			szLeaderText = self.leaders[3]
		elif iNormalScore >= 3: # was 1.1
			szLeaderText = self.leaders[4]
		elif iNormalScore >= 2: # was 1.05
			szLeaderText = self.leaders[5]
		elif iNormalScore >= 1.5: # was 1.0
			szLeaderText = self.leaders[6]
		elif iNormalScore >= 1.25: # was 0.95
			szLeaderText = self.leaders[7]
		elif iNormalScore >= 1: # was 0.9
			szLeaderText = self.leaders[8]
		elif iNormalScore >= 0.8: # was 0.85
			szLeaderText = self.leaders[9]
		elif iNormalScore >= 0.6: # was 0.8
			szLeaderText = self.leaders[10]
		elif iNormalScore >= 0.5: # was 0.75
			szLeaderText = self.leaders[11]
		elif iNormalScore >= 0.4: # was 0.7
			szLeaderText = self.leaders[12]
		elif iNormalScore >= 0.35: # was 0.65
			szLeaderText = self.leaders[13]
		elif iNormalScore >= 0.3: # was 0.6
			szLeaderText = self.leaders[14]
		elif iNormalScore >= 0.25: # was 0.55
			szLeaderText = self.leaders[15]
		elif iNormalScore >= 0.2: # was 0.5
			szLeaderText = self.leaders[16]
		elif iNormalScore >= 0.15: # was 0.4
			szLeaderText = self.leaders[17]
		elif iNormalScore >= 0.1: # was 0.3
			szLeaderText = self.leaders[18]
		else:
			szLeaderText = self.leaders[19]
		# </advc.043>
		screen.addPanel("", u"", u"", True, False, self.X_TEXT, self.Y_TEXT, self.W_TEXT, self.H_TEXT, PanelStyles.PANEL_STYLE_IN)
		szBottomText = localText.getText("TXT_KEY_DQ_TEXT_STRING", (replayInfo.getLeaderName(), szLeaderText, ))
		screen.addMultilineText(self.TEXT_ID, SASTextScale.labelText(szBottomText), self.X_TEXT+5, self.Y_TEXT+5, self.W_TEXT-10, self.H_TEXT-10, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		screen.addPanel(self.SCORE_ID, u"", u"", True, False, self.X_SCORE, self.Y_SCORE, self.W_SCORE, self.H_SCORE, PanelStyles.PANEL_STYLE_IN)
		screen.setLabelAt("", self.SCORE_ID, sasFontTagTitle + localText.getObjectText("TXT_KEY_VICTORY_SCORE", 0) + u" : " + unicode(iScore) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, self.W_SCORE/2-10, 5, 0, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		screen.addListBoxGFC(self.LIST_ID, "", self.X_LIST, self.Y_LIST, self.W_LIST, self.H_LIST, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSelect(self.LIST_ID, False)
		# <!-- custom: hoist out of per-leader loop. (Claude code Opus 4.7) -->
		eColorYellow = getInfoTypeOrFail("COLOR_YELLOW")
		for i in range(len(self.leaders)):
			szText = self.leaders[i]
			if (szLeaderText == szText):
				szText = localText.getColorText(szText, (), eColorYellow)
			else:
				szText = localText.getText(szText, ())

			screen.appendListBoxString(self.LIST_ID, sasFontTagLabel + szText + SAS_FONT_TAG_CLOSE, WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

	# returns a unique ID for a widget in this screen
	def getNextWidgetName(self):
		szName = self.WIDGET_ID + str(self.nWidgetCount)
		self.nWidgetCount += 1
		return szName

	# Will handle the input for this screen...
	def handleInput (self, inputClass):
		return 0

	def update(self, fDelta):
		return
