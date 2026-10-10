## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
from CvPythonExtensions import *
import PyHelpers
import CvUtil
import ScreenInput
import CvScreenEnums
import CvScreensInterface
import BugCore
import PlayerUtil
import ReligionUtil
from SASFontUtils import *
from SASUtils import *

# globals
gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()
PyPlayer = PyHelpers.PyPlayer
AdvisorOpt = BugCore.game.Advisors

class CvPolicyAdvisorScreen:
	"Policy Advisor Screen"

	def __init__(self):
		self.SCREEN_NAME = "PolicyAdvisorScreen"
		self.CANCEL_NAME = "PolicyCancel"
		self.EXIT_NAME = "PolicyExit"
		self.CONVERT_NAME = "PolicyConvert"
		self.RELIGION_CANCEL_NAME = "ReligionCancel"
		self.TITLE_NAME = "PolicyTitleHeader"
		self.BUTTON_NAME = "PolicyAdvisorScreenButton"
		self.TEXT_NAME = "PolicyAdvisorScreenText"
		self.AREA_NAME = "PolicyAdvisorScreenArea"
		self.HELP_AREA_NAME = "PolicyAdvisorScreenHelpArea"
		self.HELP_IMAGE_NAME = "PolicyAdvisorScreenCivicOptionImage"
		self.DEBUG_DROPDOWN_ID =  "PolicyDropdownWidget"
		self.BACKGROUND_ID = "PolicyBackground"
		self.HELP_HEADER_NAME = "PolicyAdvisorScreenHeaderName"
		self.POLICY_OPTION_HEADER_NAME = "PolicyAdvisorScreenOptionHeader"
		self.PAGE_POLICY = 0
		self.PAGE_RELIGION = 1
		self.PAGE_CORPORATION = 2
		self.iPage = self.PAGE_POLICY
		self.PAGE_TAB_IDS = ["PolicyTabButton0", "PolicyTabButton1", "PolicyTabButton2"]
		self.PAGE_LINK_WIDTH = []

		# <!-- custom: keep screen-independent advisor edge constants in init; compute runtime resolution-dependent bounds in interfaceScreen via shared SASUtils helpers. (GPT-5.3-Codex) -->
		self.W_LEFT_SPACE_FOR_COMMERCE_SLIDERS = SAS_ADVISOR_LEFT_SPACE_FOR_COMMERCE_SLIDERS
		self.W_RIGHT_SPACE_FOR_SCOREBOARD = SAS_ADVISOR_RIGHT_SPACE_FOR_SCOREBOARD
		self.H_TOP_SPACE_FOR_TECH_BAR = SAS_ADVISOR_TOP_SPACE_FOR_TECH_BAR
		self.H_BOTTOM_SPACE = SAS_ADVISOR_BOTTOM_SPACE

		self.HEADINGS_TOP = 70
		self.HEADINGS_SPACING = 5
		self.HEADINGS_BOTTOM = 280
		self.HELP_TOP = 350
		self.HELP_BOTTOM = 610
		self.TEXT_MARGIN = 15
		self.BUTTON_SIZE = 24
		self.BIG_BUTTON_SIZE = 64
		self.BOTTOM_LINE_TOP = 630
		self.BOTTOM_LINE_HEIGHT = 60

		self.Y_EXIT = 726
		self.Y_CANCEL = 726

		self.Y_SCREEN = 396
		self.Z_SCREEN = -6.1
		self.Y_TITLE = SAS_ADVISOR_TITLE_Y
		self.Z_TEXT = self.Z_SCREEN - 0.2

		self.PolicyAdvisorScreenInputMap = {
			self.BUTTON_NAME		: self.PolicyButton,
			self.TEXT_NAME			: self.PolicyButton,
			self.EXIT_NAME			: self.Revolution,
			self.CANCEL_NAME		: self.Cancel,
			}

		self.iActivePlayer = -1
		self.iLanguageLoaded = -1

		self.m_paeCurrentPolicies = []
		self.m_paeDisplayPolicies = []
		self.m_paeOriginalPolicies = []

		# <!-- custom: Religion advisor tab integration state/constants kept in Policy advisor init so no standalone Religion screen state is required anymore.
		# Removed unused legacy Religion-screen members during migration to keep this tab-host class clean.
		# Legacy credit preserved from CvReligionScreen: scrolling aspect by johny smith (CFC thread 260697), inspiration from zappara for extended religion handling, then BUG/K-Mod/advc integration layers. (GPT-5.3-Codex) -->
		self.RELIGION_NAME = "ReligionText"
		self.RELIGION_BUTTON_NAME = "ReligionScreenButton"
		self.RELIGION_TABLE_ID = "ReligionTableWidget"
		self.RELIGION_AREA1_ID = "ReligionAreaWidget1"
		self.RELIGION_AREA2_ID = "ReligionAreaWidget2"
		self.RELIGION_PANEL_ID = "ReligionPanel"
		self.RELIGION_ANARCHY_WIDGET = "ReligionAnarchyWidget"
		self.RELIGION_STATUS_PANEL_ID = "ReligionStatusPanel"
		self.RELIGION_SCROLL_PANEL_ID = "ReligionList"
		self.RELIGION_HEADER_FOUNDED_ID = "ReligionHelpFoundedHeader"
		self.RELIGION_HEADER_HOLY_CITY_ID = "ReligionHelpHolyCityHeader"
		self.RELIGION_HEADER_OWNER_ID = "ReligionHelpOwnerHeader"
		self.RELIGION_HEADER_INFLUENCE_ID = "ReligionHelpInfluenceHeader"
		self.DZ = -0.2
		self.LEFT_EDGE_TEXT = 10
		self.RELIGION_BUTTON_SIZE = 48
		self.X_RELIGION_START = 180
		self.DX_RELIGION = 98
		self.RELIGION_LEFT_LABEL_WIDTH = 180
		self.RELIGION_RIGHT_PADDING = 16
		# <!-- custom: use one uniform Religion row rhythm for top info rows and BUG summary rows. (GPT-5.3-Codex) -->
		self.RELIGION_ROW_START_Y = 90
		self.RELIGION_ROW_STEP_Y = 26
		self.Y_FOUNDED = self.RELIGION_ROW_START_Y
		self.Y_HOLY_CITY = self.RELIGION_ROW_START_Y + self.RELIGION_ROW_STEP_Y
		self.Y_OWNER = self.RELIGION_ROW_START_Y + 2 * self.RELIGION_ROW_STEP_Y
		self.Y_INFLUENCE = self.RELIGION_ROW_START_Y + 3 * self.RELIGION_ROW_STEP_Y
		self.RELIGION_BUG_SUMMARY_FIRST_OFFSET_Y = self.RELIGION_ROW_STEP_Y
		self.Y_RELIGION_NAME = 58
		# <!-- custom: keep separate max lengths for non-ID vs ID rows; non-ID fields can use a bit more room. (GPT-5.3-Codex) -->
		self.RELIGION_HOLY_CITY_CITY_MAX_CHARS_NO_ID = 22
		# <!-- custom: keep Leader (ID) values short enough to stay inside each Religion column at common upscaled UI sizes. (GPT-5.3-Codex) -->
		self.RELIGION_HOLY_CITY_OWNER_MAX_CHARS_WITH_ID = 16
		# <!-- custom: Religions tab fixed layout constants; screen-dependent geometry is derived from these in updateRuntimeLayout. (GPT-5.3-Codex) -->
		self.RELIGION_AREA_MARGIN_X = -4
		self.RELIGION_PANEL_Y_EXPANDED = 44
		self.RELIGION_PANEL_H_EXPANDED = 309
		self.RELIGION_PANEL_Y_COMPACT = 84
		self.RELIGION_PANEL_H_COMPACT = 210
		# <!-- custom: zero/near-zero gaps here are intentional final values (not placeholders) for a contiguous stacked religion layout similar to espionage tab density. (GPT-5.3-Codex) -->
		self.RELIGION_CITY_TOP_GAP = 0
		self.RELIGION_CITY_BOTTOM_GAP = -2
		self.RELIGION_STATUS_TOP_GAP = 0
		self.RELIGION_STATUS_BOTTOM_GAP = 0
		self.RELIGION_STATUS_STRIP_H = 30
		self.RELIGION_STATUS_TEXT_Y_OFFSET = -10
		self.RELIGION_STATUS_RIGHT_PAD = 10
		self.RELIGION_STATUS_CANCEL_GAP = 150
		self.RELIGION_STACK_GAP = -6

		self.NUM_RELIGIONS = -1
		self.COL_ZOOM_CITY = 0
		self.COL_CITY_NAME = 1
		self.COL_FIRST_RELIGION = 2
		self.COL_FIRST_UNIT = 9
		self.COL_FIRST_BUILDING = 10
		self.COL_EFFECTS = 14
		self.TABLE_COLUMNS = 15
		self.iReligionExamined = -1
		self.iReligionSelected = -1
		self.iReligionOriginal = -1
		self.RELIGIONS = []
		self.bBUGConstants = False

		# <!-- custom: Corporation advisor tab integration constants/state (ported from BTS CvCorporationScreen) now live in Policy advisor for a single tabbed screen flow. (GPT-5.3-Codex) -->
		self.CORPORATION_NAME = "CorporationText"
		self.CORPORATION_BUTTON_NAME = "CorporationScreenButton"
		self.CORPORATION_AREA1_ID = "CorporationAreaWidget1"
		self.CORPORATION_AREA2_ID = "CorporationAreaWidget2"
		self.CORPORATION_PANEL_ID = "CorporationPanel"
		self.CORPORATION_HEADER_BUILT_BY_ID = "CorporationHelpBuiltByHeader"
		self.CORPORATION_HEADER_GENERATES_ID = "CorporationHelpGeneratesHeader"
		self.CORPORATION_HEADER_GENERATES_YIELDS_ID = "CorporationHelpGeneratesYieldsHeader"
		self.CORPORATION_HEADER_CONSUMES_ID = "CorporationHelpConsumesHeader"
		self.CORPORATION_HEADER_FOUNDED_ID = "CorporationHelpFoundedHeader"
		self.CORPORATION_HEADER_HEADQUARTERS_ID = "CorporationHelpHeadquartersHeader"
		self.NUM_CORPORATION_PREREQ_BONUSES = None
		self.CORPORATION_LEFT_LABEL_WIDTH = 150
		self.CORPORATION_RIGHT_PADDING = 16
		# <!-- custom: keep a distinct HQ (ID) cap constant for future independent tuning, but currently match Religion's ID-row cap. (GPT-5.3-Codex) -->
		self.CORPORATION_HQ_MAX_CHARS_WITH_ID = self.RELIGION_HOLY_CITY_OWNER_MAX_CHARS_WITH_ID
		self.CORPORATION_BUTTON_SIZE = 48
		self.X_CORPORATION_START = 155
		self.DX_CORPORATION = 116
		self.Y_CORPORATION_BUTTONS = 35
		# <!-- custom: keep corporation row spacing aligned with Religion now that Headquarters is single-line. (GPT-5.3-Codex) -->
		self.CORPORATION_ROW_START_Y = 58
		self.CORPORATION_ROW_STEP_Y = self.RELIGION_ROW_STEP_Y
		self.Y_CORPORATION_GREAT_PERSON = self.CORPORATION_ROW_START_Y
		self.Y_CORPORATION_CONSUMES = self.CORPORATION_ROW_START_Y + self.CORPORATION_ROW_STEP_Y
		self.Y_CORPORATION_GENERATES_BONUS = self.CORPORATION_ROW_START_Y + 2 * self.CORPORATION_ROW_STEP_Y
		self.Y_CORPORATION_GENERATES = self.CORPORATION_ROW_START_Y + 3 * self.CORPORATION_ROW_STEP_Y
		self.Y_CORPORATION_FOUNDED = self.CORPORATION_ROW_START_Y + 4 * self.CORPORATION_ROW_STEP_Y
		self.Y_CORPORATION_HEADQUARTERS = self.CORPORATION_ROW_START_Y + 5 * self.CORPORATION_ROW_STEP_Y
		# <!-- custom: Corporations tab fixed layout constants; screen-dependent geometry is derived in updateRuntimeLayout. (GPT-5.3-Codex) -->
		self.CORPORATION_AREA_MARGIN_X = 45
		self.CORPORATION_PANEL_Y = 84
		# <!-- custom: top panel height expanded to fit Built by + Consumes + Generates + Founded + Headquarters rows in one panel without overlaps. (GPT-5.3-Codex) -->
		self.CORPORATION_PANEL_H = 229
		self.CORPORATION_CITY_TOP_GAP = 18
		# <!-- custom: empirically keep corporations tab outer margins consistent; bottom margin matches side margin. (GPT-5.3-Codex) -->
		self.CORPORATION_CITY_BOTTOM_GAP = 29
		self.iCorporationSelected = -1

	def initText(self):
		# <!-- custom: cache Policy Advisor static UI text once per language to reduce repeated translator work and keep future multi-tab expansion centralized.
		# Dynamic gameplay-state text remains computed at draw/update time. (GPT-5.3-Codex) -->
		if self.iLanguageLoaded == CyGame().getCurrentLanguage() or not CyGame().isFinalInitialized():
			return
		self.iLanguageLoaded = CyGame().getCurrentLanguage()

		self.TEXT_CANCEL = sasFontTagTitle + localText.getText("TXT_KEY_SCREEN_CANCEL", ()).upper() + SAS_FONT_TAG_CLOSE
		self.TEXT_TITLE = sasFontTagTitle.bold + localText.getText("TXT_KEY_CIVICS_SCREEN_TITLE", ()).upper() + SAS_FONT_TAG_CLOSE
		self.TEXT_EXIT = sasFontTagTitle + localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT", ()).upper() + SAS_FONT_TAG_CLOSE
		self.TEXT_REVOLUTION = sasFontTagTitle + localText.getText("TXT_KEY_CONCEPT_REVOLUTION", ()).upper() + SAS_FONT_TAG_CLOSE
		self.TEXT_NO_UPKEEP = localText.getText("TXT_KEY_CIVICS_SCREEN_NO_UPKEEP", ())
		# <!-- custom: tab captions should name subsections, not advisor screens; keep them concise like other merged advisors. (GPT-5.3-Codex) -->
		self.TEXT_TAB_POLICY = localText.getText("TXT_KEY_CONCEPT_CIVICS", ()).upper()
		# <!-- custom: use dedicated plural tab text keys for subsection clarity and consistency with merged-advisor naming (not singular/abbreviated). (GPT-5.3-Codex) -->
		self.TEXT_TAB_RELIGION = localText.getText("TXT_KEY_POLICY_TAB_RELIGIONS", ()).upper()
		self.TEXT_TAB_CORPORATION = localText.getText("TXT_KEY_POLICY_TAB_CORPORATIONS", ()).upper()
		self.PAGE_NAME_LIST = [self.TEXT_TAB_POLICY, self.TEXT_TAB_RELIGION, self.TEXT_TAB_CORPORATION]
		self.EXIT_TEXT = self.TEXT_EXIT
		self.CONVERT_TEXT = sasFontTagTitle + localText.getText("TXT_KEY_RELIGION_CONVERT", ()).upper() + SAS_FONT_TAG_CLOSE
		self.TEXT_RELIGION_NO_STATE = localText.getText("TXT_KEY_RELIGION_SCREEN_NO_STATE", ())
		self.TEXT_RELIGION_DATE_FOUNDED = localText.getText("TXT_KEY_RELIGION_SCREEN_DATE_FOUNDED", ())
		self.TEXT_RELIGION_HOLY_CITY = localText.getText("TXT_KEY_RELIGION_SCREEN_HOLY_CITY", ())
		self.TEXT_RELIGION_INFLUENCE = localText.getText("TXT_KEY_RELIGION_SCREEN_INFLUENCE", ())
		self.TEXT_CORP_ROW_BUILT_BY = localText.getText("TXT_KEY_POLICY_CORP_ROW_BUILT_BY", ())
		self.TEXT_CORP_ROW_CONSUMES = localText.getText("TXT_KEY_POLICY_CORP_ROW_CONSUMES", ())
		self.TEXT_CORP_ROW_GENERATES = localText.getText("TXT_KEY_POLICY_CORP_ROW_GENERATES", ())
		self.TEXT_CORP_ROW_GENERATES_2 = localText.getText("TXT_KEY_POLICY_CORP_ROW_GENERATES_2", ())
		self.TEXT_CORP_ROW_FOUNDED = localText.getText("TXT_KEY_POLICY_CORP_ROW_FOUNDED", ())
		self.TEXT_CORP_ROW_HQ_ID = localText.getText("TXT_KEY_POLICY_CORP_ROW_HQ_ID", ())
		self.TEXT_NOT_FOUNDED = localText.getText("TXT_KEY_RELIGION_SCREEN_NOT_FOUNDED", ())
		self.TEXT_NONE = localText.getText("TXT_KEY_NONE", ())
		self.TEXT_UNKNOWN = localText.getText("TXT_KEY_UNKNOWN", ())
		self.TEXT_BUG_RELIGIOUS_CITY = localText.getText("TXT_KEY_BUG_RELIGIOUS_CITY", ())
		self.TEXT_BUG_RELIGIOUS_TEMPLE = localText.getText("TXT_KEY_BUG_RELIGIOUS_TEMPLE", ())
		self.TEXT_BUG_RELIGIOUS_MONASTARY = localText.getText("TXT_KEY_BUG_RELIGIOUS_MONASTARY", ())
		self.TEXT_BUG_RELIGIOUS_MISSIONARY = localText.getText("TXT_KEY_BUG_RELIGIOUS_MISSIONARY", ())
		self.TEXT_WONDER_CITY = localText.getText("TXT_KEY_WONDER_CITY", ())
		self.LABEL_RELIGION_NO_STATE = sasFontTagLabel + self.TEXT_RELIGION_NO_STATE + SAS_FONT_TAG_CLOSE
		self.LABEL_RELIGION_DATE_FOUNDED = sasFontTagLabel + self.TEXT_RELIGION_DATE_FOUNDED + SAS_FONT_TAG_CLOSE
		self.LABEL_RELIGION_HOLY_CITY = sasFontTagLabel + self.TEXT_RELIGION_HOLY_CITY + SAS_FONT_TAG_CLOSE
		self.LABEL_RELIGION_INFLUENCE = sasFontTagLabel + self.TEXT_RELIGION_INFLUENCE + SAS_FONT_TAG_CLOSE
		self.LABEL_CORP_ROW_BUILT_BY = sasFontTagLabel + self.TEXT_CORP_ROW_BUILT_BY + SAS_FONT_TAG_CLOSE
		self.LABEL_CORP_ROW_CONSUMES = sasFontTagLabel + self.TEXT_CORP_ROW_CONSUMES + SAS_FONT_TAG_CLOSE
		self.LABEL_CORP_ROW_GENERATES = sasFontTagLabel + self.TEXT_CORP_ROW_GENERATES + SAS_FONT_TAG_CLOSE
		self.LABEL_CORP_ROW_GENERATES_2 = sasFontTagLabel + self.TEXT_CORP_ROW_GENERATES_2 + SAS_FONT_TAG_CLOSE
		self.LABEL_CORP_ROW_FOUNDED = sasFontTagLabel + self.TEXT_CORP_ROW_FOUNDED + SAS_FONT_TAG_CLOSE
		self.LABEL_CORP_ROW_HQ_ID = sasFontTagLabel + self.TEXT_CORP_ROW_HQ_ID + SAS_FONT_TAG_CLOSE
		self.LABEL_NONE = sasFontTagLabel + self.TEXT_NONE + SAS_FONT_TAG_CLOSE
		self.LABEL_UNKNOWN = sasFontTagLabel + self.TEXT_UNKNOWN + SAS_FONT_TAG_CLOSE
		self.NO_STATE_BUTTON_ART = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTONS_CANCEL").getPath()
		self.BUTTON_HILITE_SQUARE_ART = ArtFileMgr.getInterfaceArtInfo("BUTTON_HILITE_SQUARE").getPath()
		self.BACKGROUND_ART = ArtFileMgr.getInterfaceArtInfo("MAINMENU_SLIDESHOW_LOAD").getPath()
		self.ZOOM_ART = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTONS_CITYSELECTION").getPath()
		self.COLOR_GREEN = getInfoTypeOrFail("COLOR_GREEN")
		self.COLOR_RED = getInfoTypeOrFail("COLOR_RED")
		self.COLOR_BLUE = getInfoTypeOrFail("COLOR_BLUE")
		self.COLOR_WHITE = getInfoTypeOrFail("COLOR_WHITE")
		self.COLOR_YELLOW = getInfoTypeOrFail("COLOR_YELLOW")

	def updateRuntimeLayout(self, screen):
		self.L_SCREEN, self.T_SCREEN, self.W_SCREEN, self.H_SCREEN = getAdvisorRuntimeBounds(screen, self.W_LEFT_SPACE_FOR_COMMERCE_SLIDERS, self.W_RIGHT_SPACE_FOR_SCOREBOARD, self.H_TOP_SPACE_FOR_TECH_BAR, self.H_BOTTOM_SPACE)
		self.X_TITLE, self.X_EXIT, self.Y_EXIT, self.Y_LINK, self.Y_BOTTOM_PANEL = getAdvisorRuntimeAnchors(self.W_SCREEN, self.H_SCREEN)
		self.X_CANCEL = self.W_SCREEN / 2
		self.X_SCREEN = self.W_SCREEN / 2
		self.Y_CANCEL = self.Y_EXIT
		self.BOTTOM_LINE_WIDTH = self.W_SCREEN - 10
		self.BOTTOM_LINE_TOP = self.Y_BOTTOM_PANEL - self.BOTTOM_LINE_HEIGHT - 23
		self.HELP_BOTTOM = self.BOTTOM_LINE_TOP - 20
		self.HEADINGS_WIDTH = (self.W_SCREEN - self.HEADINGS_SPACING) / gc.getNumCivicOptionInfos() - self.HEADINGS_SPACING
		self.PAGE_LINK_WIDTH[:] = getAdvisorRuntimeLinkWidths(CyInterface(), self.PAGE_NAME_LIST, self.TEXT_EXIT, self.X_EXIT)

		# <!-- custom: Religions tab layout follows runtime Policy advisor width/height so integrated tab scales with resolution like other migrated advisors. (GPT-5.3-Codex) -->
		# <!-- custom: this is runtime (not init) because BUG advisor options can be toggled during a session; layout must react without recreating the screen object. (GPT-5.3-Codex) -->
		bBugReligious = self.isBugReligiousEnabled()
		self.X_RELIGION_AREA = self.RELIGION_AREA_MARGIN_X
		self.W_RELIGION_AREA = self.W_SCREEN - 2 * self.X_RELIGION_AREA
		# <!-- custom: spread religion columns across the full top panel width (after left header labels), instead of fixed legacy spacing that left unused horizontal space. (GPT-5.3-Codex) -->
		iReligionColumns = gc.getNumReligionInfos() + 1
		self.DX_RELIGION = (self.W_RELIGION_AREA - self.RELIGION_LEFT_LABEL_WIDTH - self.RELIGION_RIGHT_PADDING) / iReligionColumns
		self.X_RELIGION_START = self.RELIGION_LEFT_LABEL_WIDTH + self.DX_RELIGION / 2
		if bBugReligious:
			self.Y_RELIGION_AREA, self.H_RELIGION_AREA = self.RELIGION_PANEL_Y_EXPANDED, self.RELIGION_PANEL_H_EXPANDED
		else:
			self.Y_RELIGION_AREA, self.H_RELIGION_AREA = self.RELIGION_PANEL_Y_COMPACT, self.RELIGION_PANEL_H_COMPACT
		self.Y_CITY_AREA = self.Y_RELIGION_AREA + self.H_RELIGION_AREA + self.RELIGION_CITY_TOP_GAP
		self.H_CITY_AREA = self.Y_BOTTOM_PANEL - self.Y_CITY_AREA - self.RELIGION_CITY_BOTTOM_GAP
		self.W_CITY_AREA = (self.W_SCREEN - 3 * self.X_RELIGION_AREA) / 2
		self.X_CITY1_AREA = self.X_RELIGION_AREA
		self.X_CITY2_AREA = self.X_CITY1_AREA + self.W_CITY_AREA + self.X_RELIGION_AREA
		if bBugReligious:
			self.H_RELIGION_STATUS = self.RELIGION_STATUS_STRIP_H
		else:
			self.H_RELIGION_STATUS = self.Y_BOTTOM_PANEL - (self.Y_CITY_AREA + self.H_CITY_AREA + self.RELIGION_STATUS_TOP_GAP) - self.RELIGION_STATUS_BOTTOM_GAP
		self.Y_RELIGION_STATUS = self.Y_CITY_AREA + self.H_CITY_AREA - self.H_RELIGION_STATUS

		# <!-- custom: Corporations tab layout follows runtime Policy advisor bounds so the integrated tab scales with resolution like other migrated advisors. (GPT-5.3-Codex) -->
		self.X_CORPORATION_AREA = self.CORPORATION_AREA_MARGIN_X
		self.W_CORPORATION_AREA = self.W_SCREEN - 2 * self.X_CORPORATION_AREA
		iCorporationColumns = gc.getNumCorporationInfos()
		if iCorporationColumns > 0:
			self.DX_CORPORATION = (self.W_CORPORATION_AREA - self.CORPORATION_LEFT_LABEL_WIDTH - self.CORPORATION_RIGHT_PADDING) / iCorporationColumns
			self.X_CORPORATION_START = self.CORPORATION_LEFT_LABEL_WIDTH + self.DX_CORPORATION / 2
		self.Y_CORPORATION_AREA = self.CORPORATION_PANEL_Y
		self.H_CORPORATION_AREA = self.CORPORATION_PANEL_H
		self.Y_CORPORATION_CITY_AREA = self.Y_CORPORATION_AREA + self.H_CORPORATION_AREA + self.CORPORATION_CITY_TOP_GAP
		self.H_CORPORATION_CITY_AREA = self.Y_BOTTOM_PANEL - self.Y_CORPORATION_CITY_AREA - self.CORPORATION_CITY_BOTTOM_GAP
		self.W_CORPORATION_CITY_AREA = (self.W_SCREEN - 3 * self.X_CORPORATION_AREA) / 2
		self.X_CORPORATION_CITY1_AREA = self.X_CORPORATION_AREA
		self.X_CORPORATION_CITY2_AREA = self.X_CORPORATION_CITY1_AREA + self.W_CORPORATION_CITY_AREA + self.X_CORPORATION_AREA

	def getScreen(self):
		return CyGInterfaceScreen(self.SCREEN_NAME, CvScreenEnums.POLICY_ADVISOR_SCREEN)

	def setActivePlayer(self, iPlayer):

		self.iActivePlayer = iPlayer
		activePlayer = gc.getPlayer(iPlayer)

		self.m_paeCurrentPolicies = []
		self.m_paeDisplayPolicies = []
		self.m_paeOriginalPolicies = []
		for i in range (gc.getNumCivicOptionInfos()):
			self.m_paeCurrentPolicies.append(activePlayer.getCivics(i))
			self.m_paeDisplayPolicies.append(activePlayer.getCivics(i))
			self.m_paeOriginalPolicies.append(activePlayer.getCivics(i))

	def interfaceScreen (self, argsList=None):

		if argsList is not None:
			if type(argsList) in [list, tuple]:
				if len(argsList) > 0 and argsList[0] in [self.PAGE_POLICY, self.PAGE_RELIGION, self.PAGE_CORPORATION]:
					self.iPage = argsList[0]
			elif argsList in [self.PAGE_POLICY, self.PAGE_RELIGION, self.PAGE_CORPORATION]:
				self.iPage = argsList

		screen = self.getScreen()
		if self.NUM_CORPORATION_PREREQ_BONUSES is None:
			# <!-- custom: initialize constant define cache once at screen entry (not per-tab draw); avoids early __init__ timing issues while staying stable at runtime. (GPT-5.3-Codex) -->
			self.NUM_CORPORATION_PREREQ_BONUSES = gc.getDefineINT("NUM_CORPORATION_PREREQ_BONUSES")
		if screen.isActive():
			self.drawContents()
			return
		self.initText()
		screen.setRenderInterfaceOnly(True)
		screen.showScreen( PopupStates.POPUPSTATE_IMMEDIATE, False)

		# <!-- custom: remove dependency on EndTurnButton geometry (old gRect-based margin).
		# We now use shared advisor runtime bounds/anchors so policy layout follows the same resolution logic as other advisors and avoids coupling to HUD widget positions. (GPT-5.3-Codex) -->
		self.updateRuntimeLayout(screen)

		# Set the background and exit button, and show the screen
		screen.setDimensions(self.L_SCREEN, self.T_SCREEN, self.W_SCREEN, self.H_SCREEN)
		# advc.002b (note): The background image has 2:1 dimensions, so the increased W_SCREEN value makes it less distorted than in BtS. (Except maybe on very high resolutions.)
		screen.addDDSGFC(self.BACKGROUND_ID, self.BACKGROUND_ART, 0, 0, self.W_SCREEN, self.H_SCREEN, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		screen.addPanel( "PolicyTopPanel", u"", u"", True, False, 0, 0, self.W_SCREEN, 55, PanelStyles.PANEL_STYLE_TOPBAR )
		screen.addPanel( "PolicyBottomPanel", u"", u"", True, False, 0, self.Y_BOTTOM_PANEL, self.W_SCREEN, 55, PanelStyles.PANEL_STYLE_BOTTOMBAR )
		screen.showWindowBackground(False)
		screen.setText(self.CANCEL_NAME, "Background", self.TEXT_CANCEL, CvUtil.FONT_CENTER_JUSTIFY, self.X_CANCEL, self.Y_CANCEL, self.Z_TEXT, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, 1, 0)

		# Header...
		# <!-- custom: use setLabel with X_TITLE/-0.1 to match Domestic/Foreign advisor title alignment exactly; setText/X_SCREEN rendered slightly off. (GPT-5.3-Codex) -->
		screen.setLabel(self.TITLE_NAME, "Background", self.TEXT_TITLE, CvUtil.FONT_CENTER_JUSTIFY, self.X_TITLE, self.Y_TITLE, -0.1, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		self.setActivePlayer(getAdvisorValidPerspectivePlayer(self.iActivePlayer, bIncludeBarbarians=True, bAllowVassalPerspective=True))

		self.szDropdownName = self.DEBUG_DROPDOWN_ID
		# <!-- custom: preserve the selected debug/vassal perspective when reopening the advisor; bSelectActive=False rebuilt the dropdown with no selected row in debug mode. (GPT-5.5) -->
		addAdvisorDebugDropdown(screen, self.szDropdownName, self.iActivePlayer, bIncludeBarbarians=True, bAllowVassalPerspective=True)

		# Draw Contents
		self.drawContents()

		return 0

	# Draw the contents...
	def drawTabs(self):
		screen = self.getScreen()
		drawAdvisorFooterTabs(screen, self.PAGE_TAB_IDS, self.PAGE_NAME_LIST, self.PAGE_LINK_WIDTH, self.iPage, self.Y_LINK, -0.1, self.COLOR_YELLOW)

	def clearPolicyTabWidgets(self):
		screen = self.getScreen()
		screen.deleteWidget("PolicyBottomLine")
		screen.deleteWidget("PolicyRevText")
		screen.deleteWidget("PolicyUpkeepText")
		for i in range(gc.getNumCivicOptionInfos()):
			screen.deleteWidget(self.AREA_NAME + str(i))
			screen.deleteWidget(self.POLICY_OPTION_HEADER_NAME + str(i))
			screen.deleteWidget("PolicyHelpTextBackground" + str(i))
			screen.deleteWidget(self.HELP_HEADER_NAME + str(i))
			screen.deleteWidget(self.HELP_IMAGE_NAME + str(i))
			screen.deleteWidget(self.HELP_AREA_NAME + str(i))
		for j in range(gc.getNumCivicInfos()):
			screen.deleteWidget(self.getPolicyButtonName(j))
			screen.deleteWidget(self.getPolicyTextName(j))

	def clearReligionTabWidgets(self):
		screen = self.getScreen()
		screen.deleteWidget(self.RELIGION_PANEL_ID)
		screen.deleteWidget(self.RELIGION_SCROLL_PANEL_ID)
		screen.deleteWidget(self.RELIGION_AREA1_ID)
		screen.deleteWidget(self.RELIGION_AREA2_ID)
		screen.deleteWidget(self.RELIGION_TABLE_ID)
		screen.deleteWidget(self.CONVERT_NAME)
		screen.deleteWidget(self.RELIGION_CANCEL_NAME)
		screen.deleteWidget(self.RELIGION_STATUS_PANEL_ID)
		screen.deleteWidget(self.RELIGION_ANARCHY_WIDGET)
		screen.deleteWidget(self.RELIGION_HEADER_FOUNDED_ID)
		screen.deleteWidget(self.RELIGION_HEADER_HOLY_CITY_ID)
		screen.deleteWidget(self.RELIGION_HEADER_OWNER_ID)
		screen.deleteWidget(self.RELIGION_HEADER_INFLUENCE_ID)
		for iRel in range(gc.getNumReligionInfos() + 1):
			screen.deleteWidget(self.getReligionButtonName(iRel))
			screen.deleteWidget(self.getReligionTextName(iRel))

	def clearCorporationTabWidgets(self):
		screen = self.getScreen()
		screen.deleteWidget(self.CORPORATION_PANEL_ID)
		screen.deleteWidget(self.CORPORATION_AREA1_ID)
		screen.deleteWidget(self.CORPORATION_AREA2_ID)
		# <!-- custom: fix observed tab-switch leak from Corporation second-panel multiline entries ("Child..."); remove them explicitly when leaving the tab. (GPT-5.3-Codex) -->
		screen.deleteWidget("Child" + self.CORPORATION_AREA1_ID)
		screen.deleteWidget("Child" + self.CORPORATION_AREA2_ID)
		screen.deleteWidget(self.CORPORATION_HEADER_BUILT_BY_ID)
		screen.deleteWidget(self.CORPORATION_HEADER_GENERATES_ID)
		screen.deleteWidget(self.CORPORATION_HEADER_GENERATES_YIELDS_ID)
		screen.deleteWidget(self.CORPORATION_HEADER_CONSUMES_ID)
		screen.deleteWidget(self.CORPORATION_HEADER_FOUNDED_ID)
		screen.deleteWidget(self.CORPORATION_HEADER_HEADQUARTERS_ID)
		# <!-- custom: also clear dynamic top-panel corporation labels so no stale text survives cross-tab redraws. (GPT-5.3-Codex) -->
		iMaxBonusRows = self.NUM_CORPORATION_PREREQ_BONUSES
		for iCorp in range(gc.getNumCorporationInfos()):
			screen.deleteWidget(self.getCorporationButtonName(iCorp))
			screen.deleteWidget(self.getCorporationTextName(iCorp))
			screen.deleteWidget(self.getCorporationTextName(iCorp) + "GreatPersonIcon")
			screen.deleteWidget(self.getCorporationTextName(iCorp) + "GreatPerson")
			screen.deleteWidget(self.getCorporationTextName(iCorp) + "Generates")
			screen.deleteWidget(self.getCorporationTextName(iCorp) + "GeneratesYields")
			screen.deleteWidget(self.getCorporationTextName(iCorp) + "Consumes")
			screen.deleteWidget(self.getCorporationTextName(iCorp) + "BonusList")
			# <!-- custom: legacy cleanup for old wrapped bonus row widgets kept for one-way compatibility with earlier UI iterations. (GPT-5.3-Codex) -->
			for iRow in range(iMaxBonusRows + 1):
				screen.deleteWidget(self.getCorporationTextName(iCorp) + "BonusRow" + str(iRow))
			screen.deleteWidget(self.getCorporationTextName(iCorp) + "Founded")
			screen.deleteWidget(self.getCorporationTextName(iCorp) + "HeadquartersOwner")
			screen.deleteWidget(self.getCorporationTextName(iCorp) + "HeadquartersCity")

	def drawContents(self):
		self.drawTabs()
		if self.iPage == self.PAGE_POLICY:
			self.clearReligionTabWidgets()
			self.clearCorporationTabWidgets()
			self.drawPolicyTabContents()
		elif self.iPage == self.PAGE_RELIGION:
			self.clearPolicyTabWidgets()
			self.clearCorporationTabWidgets()
			self.drawReligionTabContents()
		else:
			self.clearPolicyTabWidgets()
			self.clearReligionTabWidgets()
			self.drawCorporationTabContents()

	def drawPolicyTabContents(self):
		screen = self.getScreen()
		screen.setLabel(self.TITLE_NAME, "Background", self.TEXT_TITLE, CvUtil.FONT_CENTER_JUSTIFY, self.X_TITLE, self.Y_TITLE, -0.1, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.addPanel("PolicyBottomLine", "", "", True, True, self.HEADINGS_SPACING, self.BOTTOM_LINE_TOP, self.BOTTOM_LINE_WIDTH, self.BOTTOM_LINE_HEIGHT, PanelStyles.PANEL_STYLE_MAIN)
		# Draw the radio buttons
		self.drawAllButtons()
		# Draw Help Text
		self.drawAllHelpText()
		# Update Maintenance/anarchy/etc.
		self.updateAnarchy()

	def drawReligionTabContents(self):
		screen = self.getScreen()
		# <!-- custom: keep advisor header stable across tabs (Policy Advisor), like Domestic/Foreign tabbed screens; tab labels already indicate the active subsection. (GPT-5.3-Codex) -->
		screen.setLabel(self.TITLE_NAME, "Background", self.TEXT_TITLE, CvUtil.FONT_CENTER_JUSTIFY, self.X_TITLE, self.Y_TITLE, -0.1, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		self.refreshReligionTabData()
		self.drawReligionInfo()
		self.drawHelpInfo()
		self.drawCityInfo(self.iReligionSelected)

	def drawCorporationTabContents(self):
		screen = self.getScreen()
		# <!-- custom: keep advisor header stable across tabs (Policy Advisor), like Domestic/Foreign tabbed screens; tab labels already indicate the active subsection. (GPT-5.3-Codex) -->
		screen.setLabel(self.TITLE_NAME, "Background", self.TEXT_TITLE, CvUtil.FONT_CENTER_JUSTIFY, self.X_TITLE, self.Y_TITLE, -0.1, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		self.refreshCorporationTabData()
		self.drawCorporationInfo()
		self.drawCorporationCityInfo(self.iCorporationSelected)

	def refreshReligionTabData(self):
		self.iActivePlayer = getAdvisorValidPerspectivePlayer(self.iActivePlayer, bIncludeBarbarians=True, bAllowVassalPerspective=True)
		if self.NUM_RELIGIONS == -1:
			self.NUM_RELIGIONS = ReligionUtil.getNumReligions()
			self.COL_FIRST_UNIT = self.COL_FIRST_RELIGION + self.NUM_RELIGIONS
			self.COL_FIRST_BUILDING = self.COL_FIRST_UNIT + ReligionUtil.getNumUnitTypes()
			self.COL_EFFECTS = self.COL_FIRST_BUILDING + ReligionUtil.getNumBuildingTypes()
			self.TABLE_COLUMNS = self.COL_EFFECTS + 1
		if self.isBugReligiousEnabled():
			if AdvisorOpt.isShowAllReligions():
				self.RELIGIONS = ReligionUtil.getAllReligions()
			elif AdvisorOpt.isShowFoundedReligions():
				self.RELIGIONS = ReligionUtil.getFoundedReligions()
			else:
				self.RELIGIONS = ReligionUtil.getPlayerReligions(gc.getPlayer(self.iActivePlayer))
		else:
			# <!-- custom: preserve K-Mod/advc behavior from legacy Religion screen: in non-BUG mode, use all religions directly (the old BUG branch/condition mismatch was fixed in K-Mod and later BUG 4.5). (GPT-5.3-Codex) -->
			self.RELIGIONS = range(gc.getNumReligionInfos())
		self.iReligionSelected = gc.getPlayer(self.iActivePlayer).getStateReligion()
		if self.iReligionSelected == -1:
			self.iReligionSelected = gc.getNumReligionInfos()
		self.iReligionExamined = self.iReligionSelected
		self.iReligionOriginal = self.iReligionSelected

	def refreshCorporationTabData(self):
		self.iActivePlayer = getAdvisorValidPerspectivePlayer(self.iActivePlayer, bIncludeBarbarians=True, bAllowVassalPerspective=True)
		if self.iCorporationSelected >= gc.getNumCorporationInfos():
			self.iCorporationSelected = -1

	def drawCivicOptionButtons(self, iCivicOption):

		activePlayer = gc.getPlayer(self.iActivePlayer)
		screen = self.getScreen()

		for j in range(gc.getNumCivicInfos()):

			if (gc.getCivicInfo(j).getCivicOptionType() == iCivicOption):										
				screen.setState(self.getPolicyButtonName(j), self.m_paeCurrentPolicies[iCivicOption] == j)

				if (self.m_paeDisplayPolicies[iCivicOption] == j):
					#screen.setState(self.getPolicyButtonName(j), True)
					screen.show(self.getPolicyButtonName(j))
				elif (activePlayer.canDoCivics(j)):
					#screen.setState(self.getPolicyButtonName(j), False)
					screen.show(self.getPolicyButtonName(j))
				else:
					screen.hide(self.getPolicyButtonName(j))

	# Will draw the radio buttons (and revolution)
	def drawAllButtons(self):				

		for i in range(gc.getNumCivicOptionInfos()):

			fX = self.HEADINGS_SPACING + (self.HEADINGS_WIDTH + self.HEADINGS_SPACING) * i
			fY = self.HEADINGS_TOP

			szAreaID = self.AREA_NAME + str(i)
			screen = self.getScreen()
			screen.addPanel(szAreaID, "", "", True, True, fX, fY, self.HEADINGS_WIDTH, self.HEADINGS_BOTTOM - self.HEADINGS_TOP, PanelStyles.PANEL_STYLE_MAIN)

			screen.setLabel(self.POLICY_OPTION_HEADER_NAME + str(i), "Background", sasFontTagLabel + gc.getCivicOptionInfo(i).getDescription().upper() + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, fX + self.HEADINGS_WIDTH / 2, self.HEADINGS_TOP + self.TEXT_MARGIN, 0, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

			fY += self.TEXT_MARGIN

			for j in range(gc.getNumCivicInfos()):
				if (gc.getCivicInfo(j).getCivicOptionType() == i):										
					fY += 2 * self.TEXT_MARGIN

					screen.addCheckBoxGFC(self.getPolicyButtonName(j), gc.getCivicInfo(j).getButton(), self.BUTTON_HILITE_SQUARE_ART, fX + self.BUTTON_SIZE/2, fY, self.BUTTON_SIZE, self.BUTTON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1, ButtonStyles.BUTTON_STYLE_LABEL)

					screen.setText(self.getPolicyTextName(j), "", sasFontTagLabel + gc.getCivicInfo(j).getDescription() + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, fX + self.BUTTON_SIZE + self.TEXT_MARGIN, fY, 0, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

			self.drawCivicOptionButtons(i)

	def highlight(self, iCivic):
		iCivicOption = gc.getCivicInfo(iCivic).getCivicOptionType()
		if self.m_paeDisplayPolicies[iCivicOption] != iCivic:
			self.m_paeDisplayPolicies[iCivicOption] = iCivic
			self.drawCivicOptionButtons(iCivicOption)
			return True
		return False

	def unHighlight(self, iCivic):		
		iCivicOption = gc.getCivicInfo(iCivic).getCivicOptionType()
		if self.m_paeDisplayPolicies[iCivicOption] != self.m_paeCurrentPolicies[iCivicOption]:
			self.m_paeDisplayPolicies[iCivicOption] = self.m_paeCurrentPolicies[iCivicOption]
			self.drawCivicOptionButtons(iCivicOption)
			return True
		return False

	def select(self, iCivic):
		activePlayer = gc.getPlayer(self.iActivePlayer)
		if (not activePlayer.canDoCivics(iCivic)):
			# If you can't even do this, get out....
			return 0

		iCivicOption = gc.getCivicInfo(iCivic).getCivicOptionType()

		# Set the previous widget
		iCivicPrev = self.m_paeCurrentPolicies[iCivicOption]

		# Switch the widgets
		self.m_paeCurrentPolicies[iCivicOption] = iCivic

		# Unighlight the previous widget
		self.unHighlight(iCivicPrev)
		self.getScreen().setState(self.getPolicyButtonName(iCivicPrev), False)

		# highlight the new widget
		self.highlight(iCivic)		
		self.getScreen().setState(self.getPolicyButtonName(iCivic), True)

		return 0

	def PolicyButton(self, inputClass):

		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED) :
			if (inputClass.getFlags() & MouseFlags.MOUSE_RBUTTONUP):
				CvScreensInterface.pediaJumpToCivic((inputClass.getID(), ))
			else:
				# Select button
				self.select(inputClass.getID())
				self.drawHelpText(gc.getCivicInfo(inputClass.getID()).getCivicOptionType())
				self.updateAnarchy()
		elif (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CURSOR_MOVE_ON) :
			# Highlight this button
			if self.highlight(inputClass.getID()):
				self.drawHelpText(gc.getCivicInfo(inputClass.getID()).getCivicOptionType())
				self.updateAnarchy()
		elif (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CURSOR_MOVE_OFF) :
			if self.unHighlight(inputClass.getID()):
				self.drawHelpText(gc.getCivicInfo(inputClass.getID()).getCivicOptionType())
				self.updateAnarchy()

		return 0

	def drawHelpText(self, iCivicOption):

		activePlayer = gc.getPlayer(self.iActivePlayer)
		iCivic = self.m_paeDisplayPolicies[iCivicOption]

		szPaneID = "PolicyHelpTextBackground" + str(iCivicOption)
		screen = self.getScreen()

		szHelpText = u""

		# Upkeep string
		if ((gc.getCivicInfo(iCivic).getUpkeep() != -1) and not activePlayer.isNoCivicUpkeep(iCivicOption)):
			szHelpText = gc.getUpkeepInfo(gc.getCivicInfo(iCivic).getUpkeep()).getDescription()
		else:
			szHelpText = self.TEXT_NO_UPKEEP

		szHelpText += CyGameTextMgr().parseCivicInfo(iCivic, False, True, True)

		fX = self.HEADINGS_SPACING  + (self.HEADINGS_WIDTH + self.HEADINGS_SPACING) * iCivicOption

		screen.setLabel(self.HELP_HEADER_NAME + str(iCivicOption), "Background",  sasFontTagLabel + gc.getCivicInfo(self.m_paeDisplayPolicies[iCivicOption]).getDescription().upper() + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, fX + self.HEADINGS_WIDTH/2, self.HELP_TOP + self.TEXT_MARGIN, 0, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		fY = self.HELP_TOP - self.BIG_BUTTON_SIZE
		szHelpImageID = self.HELP_IMAGE_NAME + str(iCivicOption)		
		screen.setImageButton(szHelpImageID, gc.getCivicInfo(iCivic).getButton(), fX + self.HEADINGS_WIDTH/2 - self.BIG_BUTTON_SIZE/2, fY, self.BIG_BUTTON_SIZE, self.BIG_BUTTON_SIZE, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, iCivic, 1)

		fY = self.HELP_TOP + 3 * self.TEXT_MARGIN
		szHelpAreaID = self.HELP_AREA_NAME + str(iCivicOption)		
		screen.addMultilineText(szHelpAreaID, sasFontTagLabel + szHelpText + SAS_FONT_TAG_CLOSE, fX+5, fY, self.HEADINGS_WIDTH-7, self.HELP_BOTTOM - fY-2, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	# Will draw the help text
	def drawAllHelpText(self):
		for i in range (gc.getNumCivicOptionInfos()):		

			fX = self.HEADINGS_SPACING  + (self.HEADINGS_WIDTH + self.HEADINGS_SPACING) * i

			szPaneID = "PolicyHelpTextBackground" + str(i)
			screen = self.getScreen()
			screen.addPanel(szPaneID, "", "", True, True, fX, self.HELP_TOP, self.HEADINGS_WIDTH, self.HELP_BOTTOM - self.HELP_TOP, PanelStyles.PANEL_STYLE_MAIN)

			self.drawHelpText(i)

	# Will Update the maintenance/anarchy/etc
	def updateAnarchy(self):

		screen = self.getScreen()
		self.initText()

		activePlayer = gc.getPlayer(self.iActivePlayer)

		bChange = False
		i = 0
		while (i  < gc.getNumCivicOptionInfos() and not bChange):
			if (self.m_paeCurrentPolicies[i] != self.m_paeOriginalPolicies[i]):
				bChange = True
			i += 1		

		# Make the revolution button
		screen.deleteWidget(self.EXIT_NAME)
		if (not isAdvisorReadOnlyPerspective(self.iActivePlayer) and activePlayer.canRevolution(0) and bChange):
			screen.setText(self.EXIT_NAME, "Background", self.TEXT_REVOLUTION, CvUtil.FONT_RIGHT_JUSTIFY, self.X_EXIT, self.Y_EXIT, self.Z_TEXT, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_REVOLUTION, 1, 0)
			screen.show(self.CANCEL_NAME)
		else:
			screen.setText(self.EXIT_NAME, "Background", self.TEXT_EXIT, CvUtil.FONT_RIGHT_JUSTIFY, self.X_EXIT, self.Y_EXIT, self.Z_TEXT, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, 1, -1)
			screen.hide(self.CANCEL_NAME)

		# Anarchy
		iTurns = activePlayer.getCivicAnarchyLength(self.m_paeDisplayPolicies)

		if (activePlayer.canRevolution(0)):
			szText = localText.getText("TXT_KEY_ANARCHY_TURNS", (iTurns, ))
		else:
			szText = CyGameTextMgr().setRevolutionHelp(self.iActivePlayer)

		screen.setLabel("PolicyRevText", "Background", sasFontTagLabel + szText + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, self.X_SCREEN, self.BOTTOM_LINE_TOP + self.TEXT_MARGIN//2, 0, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# Maintenance		
		#szText = localText.getText("TXT_KEY_CIVIC_SCREEN_UPKEEP", (activePlayer.getCivicUpkeep(self.m_paeDisplayPolicies, True), ))
		szText = localText.getText("TXT_KEY_CIVIC_SCREEN_UPKEEP", (activePlayer.getCivicUpkeep(self.m_paeDisplayPolicies, True)*(100+activePlayer.calculateInflationRate())/100, )) # K-Mod
		screen.setLabel("PolicyUpkeepText", "Background", sasFontTagLabel + szText + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, self.X_SCREEN, self.BOTTOM_LINE_TOP + self.BOTTOM_LINE_HEIGHT - 2 * self.TEXT_MARGIN, 0, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

	# Revolution!!!
	def Revolution(self, inputClass):

		activePlayer = gc.getPlayer(self.iActivePlayer)

		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED) :
			# advc.001d: Clause added to prevent revolution when viewing another civ's civics through the Debug menu
			if self.iActivePlayer == gc.getGame().getActivePlayer() and activePlayer.canRevolution(0):
				messageControl = CyMessageControl()
				messageControl.sendUpdateCivics(self.m_paeDisplayPolicies)			
			screen = self.getScreen()
			screen.hideScreen()

	def Cancel(self, inputClass):
		screen = self.getScreen()
		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED) :
			for i in range (gc.getNumCivicOptionInfos()):
				self.m_paeCurrentPolicies[i] = self.m_paeOriginalPolicies[i]
				self.m_paeDisplayPolicies[i] = self.m_paeOriginalPolicies[i]

			self.drawContents()

	def getPolicyButtonName(self, iCivic):
		szName = self.BUTTON_NAME + str(iCivic)
		return szName

	def getPolicyTextName(self, iCivic):
		szName = self.TEXT_NAME + str(iCivic)
		return szName

	# Draws the religion buttons and information
	def drawReligionInfo(self):
		screen = self.getScreen()
		szArea = self.RELIGION_SCROLL_PANEL_ID

		screen.addPanel(self.RELIGION_PANEL_ID, "", "", False, True, self.X_RELIGION_AREA, self.Y_RELIGION_AREA, self.W_RELIGION_AREA, self.H_RELIGION_AREA + 5, PanelStyles.PANEL_STYLE_MAIN)
		screen.addScrollPanel(self.RELIGION_SCROLL_PANEL_ID, u"", self.X_RELIGION_AREA, self.Y_RELIGION_AREA, self.W_RELIGION_AREA, self.H_RELIGION_AREA, PanelStyles.PANEL_STYLE_EXTERNAL)
		screen.setActivation(self.RELIGION_SCROLL_PANEL_ID, ActivationTypes.ACTIVATE_NORMAL)

		xLoop = self.X_RELIGION_START
		iButtonOffset = self.RELIGION_BUTTON_SIZE / 2
		for iRel in self.RELIGIONS:
			szButtonName = self.getReligionButtonName(iRel)
			if gc.getGame().getReligionGameTurnFounded(iRel) >= 0:
				screen.addCheckBoxGFCAt(szArea, szButtonName, gc.getReligionInfo(iRel).getButton(), self.BUTTON_HILITE_SQUARE_ART, xLoop - iButtonOffset, 5, self.RELIGION_BUTTON_SIZE, self.RELIGION_BUTTON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1, ButtonStyles.BUTTON_STYLE_LABEL, False)
			else:
				screen.setImageButtonAt(szButtonName, szArea, gc.getReligionInfo(iRel).getButtonDisabled(), xLoop - iButtonOffset, 5, self.RELIGION_BUTTON_SIZE, self.RELIGION_BUTTON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1)
			screen.setLabelAt(self.getReligionTextName(iRel), szArea, sasFontTagLabel + gc.getReligionInfo(iRel).getDescription() + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, self.Y_RELIGION_NAME, self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			xLoop += self.DX_RELIGION

		szButtonName = self.getReligionButtonName(gc.getNumReligionInfos())
		screen.addCheckBoxGFCAt(szArea, szButtonName, self.NO_STATE_BUTTON_ART, self.BUTTON_HILITE_SQUARE_ART, xLoop - iButtonOffset, 5, self.RELIGION_BUTTON_SIZE, self.RELIGION_BUTTON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1, ButtonStyles.BUTTON_STYLE_LABEL, False)
		screen.setLabelAt(self.getReligionTextName(gc.getNumReligionInfos()), szArea, self.LABEL_RELIGION_NO_STATE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, self.Y_RELIGION_NAME, self.DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def drawHelpInfo(self):
		screen = self.getScreen()
		szArea = self.RELIGION_SCROLL_PANEL_ID
		iMapObserverTeam = self.getMapObserverTeam()

		screen.setLabelAt(self.RELIGION_HEADER_FOUNDED_ID, szArea, self.LABEL_RELIGION_DATE_FOUNDED, CvUtil.FONT_LEFT_JUSTIFY, self.LEFT_EDGE_TEXT, self.Y_FOUNDED, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		xLoop = self.X_RELIGION_START
		for iRel in self.RELIGIONS:
			if gc.getGame().getReligionGameTurnFounded(iRel) >= 0:
				szFounded = CyGameTextMgr().getTimeStr(gc.getGame().getReligionGameTurnFounded(iRel), false)
				screen.setLabelAt("ReligionFoundedValue" + str(iRel), szArea, sasFontTagLabel + szFounded + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, self.Y_FOUNDED, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			xLoop += self.DX_RELIGION

		screen.setLabelAt(self.RELIGION_HEADER_HOLY_CITY_ID, szArea, self.LABEL_RELIGION_HOLY_CITY, CvUtil.FONT_LEFT_JUSTIFY, self.LEFT_EDGE_TEXT, self.Y_HOLY_CITY, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.setLabelAt(self.RELIGION_HEADER_OWNER_ID, szArea, sasFontTagLabel + u"Leader (ID)" + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, self.LEFT_EDGE_TEXT, self.Y_OWNER, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		xLoop = self.X_RELIGION_START
		for iRel in self.RELIGIONS:
			if gc.getGame().getReligionGameTurnFounded(iRel) >= 0:
				pHolyCity = gc.getGame().getHolyCity(iRel)
				if pHolyCity.isNone():
					screen.setLabelAt("ReligionHolyCityValue" + str(iRel), szArea, self.LABEL_NONE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, self.Y_HOLY_CITY, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
					screen.setLabelAt("ReligionHolyCityOwner" + str(iRel), szArea, self.LABEL_NONE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, self.Y_OWNER, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				elif not pHolyCity.isRevealed(iMapObserverTeam, True): # advc.001d
					screen.setLabelAt("ReligionHolyCityValue" + str(iRel), szArea, self.LABEL_UNKNOWN, CvUtil.FONT_CENTER_JUSTIFY, xLoop, self.Y_HOLY_CITY, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
					screen.setLabelAt("ReligionHolyCityOwner" + str(iRel), szArea, self.LABEL_UNKNOWN, CvUtil.FONT_CENTER_JUSTIFY, xLoop, self.Y_OWNER, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				else:
					# <!-- custom: keep Holy City and Leader as separate rows so upscaled text stays readable; include player ID for precise debugging/reference in duplicate-civ setups. (GPT-5.3-Codex) -->
					szHolyCityName = self.trimReligionColumnText(pHolyCity.getName(), self.RELIGION_HOLY_CITY_CITY_MAX_CHARS_NO_ID)
					iOwner = pHolyCity.getOwner()
					szHolyCityOwner = self.trimReligionOwnerWithId(gc.getPlayer(iOwner).getName(), iOwner, self.RELIGION_HOLY_CITY_OWNER_MAX_CHARS_WITH_ID)
					screen.setLabelAt("ReligionHolyCityValue" + str(iRel), szArea, sasFontTagLabel + szHolyCityName + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, self.Y_HOLY_CITY, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
					screen.setLabelAt("ReligionHolyCityOwner" + str(iRel), szArea, sasFontTagLabel + szHolyCityOwner + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, self.Y_OWNER, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			xLoop += self.DX_RELIGION

		screen.setLabelAt(self.RELIGION_HEADER_INFLUENCE_ID, szArea, self.LABEL_RELIGION_INFLUENCE, CvUtil.FONT_LEFT_JUSTIFY, self.LEFT_EDGE_TEXT, self.Y_INFLUENCE, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		xLoop = self.X_RELIGION_START
		for iRel in self.RELIGIONS:
			if gc.getGame().getReligionGameTurnFounded(iRel) >= 0:
				szFounded = str(gc.getGame().calculateReligionPercent(iRel)) + "%"
				screen.setLabelAt("ReligionInfluenceValue" + str(iRel), szArea, sasFontTagLabel + szFounded + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, self.Y_INFLUENCE, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			xLoop += self.DX_RELIGION

		if self.isBugReligiousEnabled():
			self.BUGConstants()
			iPlayer = PyPlayer(self.iActivePlayer)
			cityList = iPlayer.getCityList()
			iCities = [0] * self.NUM_RELIGIONS
			iTemple = [0] * self.NUM_RELIGIONS
			iMonastery = [0] * self.NUM_RELIGIONS
			iMissionaries_Active = [0] * self.NUM_RELIGIONS
			iMissionaries_Construct = [0] * self.NUM_RELIGIONS

			for pLoopCity in cityList:
				lReligions = pLoopCity.getReligions()
				for iRel in self.RELIGIONS:
					if iRel in lReligions:
						iCities[iRel] += 1
					iBldg = ReligionUtil.getBuilding(iRel, ReligionUtil.BUILDING_TEMPLE)
					if self.calculateBuilding(pLoopCity, iBldg) == self.objectHave:
						iTemple[iRel] += 1
					iBldg = ReligionUtil.getBuilding(iRel, ReligionUtil.BUILDING_MONASTERY)
					if self.calculateBuilding(pLoopCity, iBldg) == self.objectHave:
						iMonastery[iRel] += 1
					iUnit = ReligionUtil.getUnit(iRel, ReligionUtil.UNIT_MISSIONARY)
					if pLoopCity.GetCy().getFirstUnitOrder(iUnit) != -1:
						iMissionaries_Construct[iRel] += 1

			for iUnit in PlayerUtil.playerUnits(self.iActivePlayer):
				for iRel in self.RELIGIONS:
					if iUnit.getUnitType() == ReligionUtil.getUnit(iRel, ReligionUtil.UNIT_MISSIONARY):
						iMissionaries_Active[iRel] += 1

			# <!-- custom: keep BUG summary rows on the same vertical rhythm as the main Religion rows for consistent line spacing in this integrated tab. (GPT-5.3-Codex) -->
			iY = self.Y_INFLUENCE + self.RELIGION_BUG_SUMMARY_FIRST_OFFSET_Y
			sCities = "%s [%i]:" % (self.szCities, len(cityList))
			screen.setLabelAt("ReligionCityCountHeader", szArea, sasFontTagLabel + sCities + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, self.LEFT_EDGE_TEXT, iY, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			xLoop = self.X_RELIGION_START
			for iRel in self.RELIGIONS:
				if gc.getGame().getReligionGameTurnFounded(iRel) >= 0:
					screen.setLabelAt("ReligionCityCount" + str(iRel), szArea, sasFontTagLabel + ("%i" % iCities[iRel]) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, iY, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				xLoop += self.DX_RELIGION

			iY += self.RELIGION_ROW_STEP_Y
			screen.setLabelAt("ReligionTempleHeader", szArea, sasFontTagLabel + self.szTemples + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, self.LEFT_EDGE_TEXT, iY, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			xLoop = self.X_RELIGION_START
			for iRel in self.RELIGIONS:
				if gc.getGame().getReligionGameTurnFounded(iRel) >= 0:
					screen.setLabelAt("ReligionTempleCount" + str(iRel), szArea, sasFontTagLabel + ("%i" % iTemple[iRel]) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, iY, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				xLoop += self.DX_RELIGION

			iY += self.RELIGION_ROW_STEP_Y
			screen.setLabelAt("ReligionMonasteryHeader", szArea, sasFontTagLabel + self.szMonastaries + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, self.LEFT_EDGE_TEXT, iY, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			xLoop = self.X_RELIGION_START
			for iRel in self.RELIGIONS:
				if gc.getGame().getReligionGameTurnFounded(iRel) >= 0:
					screen.setLabelAt("ReligionMonasteryCount" + str(iRel), szArea, sasFontTagLabel + ("%i" % iMonastery[iRel]) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, iY, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				xLoop += self.DX_RELIGION

			iY += self.RELIGION_ROW_STEP_Y
			screen.setLabelAt("ReligionMissionaryHeader", szArea, sasFontTagLabel + self.szMissionaries + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, self.LEFT_EDGE_TEXT, iY, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			xLoop = self.X_RELIGION_START
			for iRel in self.RELIGIONS:
				if gc.getGame().getReligionGameTurnFounded(iRel) >= 0:
					szFounded = "%i [%i]" % (iMissionaries_Active[iRel], iMissionaries_Construct[iRel])
					screen.setLabelAt("ReligionMissionaryCount" + str(iRel), szArea, sasFontTagLabel + szFounded + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, iY, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				xLoop += self.DX_RELIGION

	# BUG constants
	def BUGConstants(self):
		if self.bBUGConstants:
			return
		self.bBUGConstants = True
		self.hammerIcon = u"%c" % (gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar())
		self.objectIsPresent = "x"
		self.objectIsNotPresent = "-"
		self.objectCanBeBuild = "o"
		self.objectUnderConstruction = self.hammerIcon
		self.objectHave = localText.changeTextColor(self.objectIsPresent, self.COLOR_GREEN)
		self.objectNotPossible = localText.changeTextColor(self.objectIsNotPresent, self.COLOR_RED)
		self.objectPossible = localText.changeTextColor(self.objectCanBeBuild, self.COLOR_BLUE)
		self.objectHaveObsolete = localText.changeTextColor(self.objectIsPresent, self.COLOR_WHITE)
		self.objectNotPossibleConcurrent = localText.changeTextColor(self.objectIsNotPresent, self.COLOR_YELLOW)
		self.objectPossibleConcurrent = localText.changeTextColor(self.objectCanBeBuild, self.COLOR_YELLOW)
		self.szCities = self.TEXT_BUG_RELIGIOUS_CITY
		self.szTemples = self.TEXT_BUG_RELIGIOUS_TEMPLE
		self.szMonastaries = self.TEXT_BUG_RELIGIOUS_MONASTARY
		self.szMissionaries = self.TEXT_BUG_RELIGIOUS_MISSIONARY
		self.zoomArt = self.ZOOM_ART
		self.sCity = self.TEXT_WONDER_CITY

	def drawCityInfo(self, iReligion):
		screen = self.getScreen()
		bBugReligious = self.isBugReligiousEnabled()
		if iReligion == gc.getNumReligionInfos():
			iLinkReligion = -1
		else:
			iLinkReligion = iReligion

		screen.deleteWidget(self.RELIGION_AREA1_ID)
		screen.deleteWidget(self.RELIGION_AREA2_ID)
		screen.deleteWidget(self.RELIGION_TABLE_ID)
		screen.deleteWidget(self.CONVERT_NAME)
		screen.deleteWidget(self.RELIGION_CANCEL_NAME)

		if bBugReligious:
			# <!-- custom: +17px here is intentional to remove the visible seam above the footer and make the second panel reach the footer like the top panel reaches the header. (GPT-5.3-Codex) -->
			screen.addPanel(self.RELIGION_AREA1_ID, "", "", True, True, self.X_RELIGION_AREA, self.Y_CITY_AREA + self.RELIGION_STACK_GAP, self.W_RELIGION_AREA, self.H_CITY_AREA + 17, PanelStyles.PANEL_STYLE_MAIN)
		else:
			screen.addPanel(self.RELIGION_AREA1_ID, "", "", True, True, self.X_CITY1_AREA, self.Y_CITY_AREA, self.W_CITY_AREA, self.H_CITY_AREA, PanelStyles.PANEL_STYLE_MAIN)
			screen.addPanel(self.RELIGION_AREA2_ID, "", "", True, True, self.X_CITY2_AREA, self.Y_CITY_AREA, self.W_CITY_AREA, self.H_CITY_AREA, PanelStyles.PANEL_STYLE_MAIN)

		for iRel in self.RELIGIONS:
			screen.setState(self.getReligionButtonName(iRel), self.iReligionSelected == iRel)
		screen.setState(self.getReligionButtonName(gc.getNumReligionInfos()), self.iReligionSelected == gc.getNumReligionInfos())

		iPlayer = PyPlayer(self.iActivePlayer)
		cityList = iPlayer.getCityList()

		if bBugReligious:
			iTableW = self.W_RELIGION_AREA - 2 * 15
			screen.addTableControlGFC(self.RELIGION_TABLE_ID, self.TABLE_COLUMNS, self.X_RELIGION_AREA + 15, self.Y_CITY_AREA + self.RELIGION_STACK_GAP + 15, iTableW, self.H_CITY_AREA - self.H_RELIGION_STATUS - 20, True, True, 24, 24, TableStyles.TABLE_STYLE_STANDARD)
			screen.enableSort(self.RELIGION_TABLE_ID)
			iZoomW = 34
			iCityW = 185
			iReligionW = 28
			iUnitW = 32
			iBuildingW = 32
			iFixedW = iZoomW + iCityW + self.NUM_RELIGIONS * iReligionW + ReligionUtil.getNumUnitTypes() * iUnitW + ReligionUtil.getNumBuildingTypes() * iBuildingW
			iEffectsW = iTableW - iFixedW
			if iEffectsW < 175:
				iEffectsW = 175
				iCityW = iTableW - iEffectsW - iZoomW - self.NUM_RELIGIONS * iReligionW - ReligionUtil.getNumUnitTypes() * iUnitW - ReligionUtil.getNumBuildingTypes() * iBuildingW
			screen.setTableColumnHeader(self.RELIGION_TABLE_ID, self.COL_ZOOM_CITY, "", iZoomW)
			screen.setTableColumnHeader(self.RELIGION_TABLE_ID, self.COL_CITY_NAME, sasFontTagLabel + self.sCity + SAS_FONT_TAG_CLOSE, iCityW)

			for iRel in range(self.NUM_RELIGIONS):
				if gc.getGame().getReligionGameTurnFounded(iRel) >= 0:
					szRelHeader = sasFontTagLabel + (u"%c" % (gc.getReligionInfo(iRel).getChar())) + SAS_FONT_TAG_CLOSE
				else:
					szRelHeader = ""
				screen.setTableColumnHeader(self.RELIGION_TABLE_ID, self.COL_FIRST_RELIGION + iRel, szRelHeader, iReligionW)
			for type in ReligionUtil.getUnitTypes():
				screen.setTableColumnHeader(self.RELIGION_TABLE_ID, self.COL_FIRST_UNIT + type.index, sasFontTagLabel + (u"%s" % type.icon) + SAS_FONT_TAG_CLOSE, iUnitW)
			for type in ReligionUtil.getBuildingTypes():
				screen.setTableColumnHeader(self.RELIGION_TABLE_ID, self.COL_FIRST_BUILDING + type.index, sasFontTagLabel + (u"%s" % type.icon) + SAS_FONT_TAG_CLOSE, iBuildingW)
			screen.setTableColumnHeader(self.RELIGION_TABLE_ID, self.COL_EFFECTS, "", iEffectsW)

			for iCity in range(len(cityList)):
				pLoopCity = cityList[iCity]
				screen.appendTableRow(self.RELIGION_TABLE_ID)
				screen.setTableText(self.RELIGION_TABLE_ID, self.COL_ZOOM_CITY, iCity, "", self.zoomArt, WidgetTypes.WIDGET_ZOOM_CITY, pLoopCity.getOwner(), pLoopCity.getID(), CvUtil.FONT_LEFT_JUSTIFY)
				screen.setTableText(self.RELIGION_TABLE_ID, self.COL_CITY_NAME, iCity, sasFontTagLabel + pLoopCity.getName() + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

				lHolyCity = pLoopCity.getHolyCity()
				lReligions = pLoopCity.getReligions()
				for iRel in range(self.NUM_RELIGIONS):
					if gc.getGame().getReligionGameTurnFounded(iRel) >= 0:
						szReligionIcon = ""
						if iRel in lHolyCity:
							szReligionIcon = sasFontTagLabel + (u"%c" % (gc.getReligionInfo(iRel).getHolyCityChar())) + SAS_FONT_TAG_CLOSE
						elif iRel in lReligions:
							szReligionIcon = sasFontTagLabel + (u"%c" % (gc.getReligionInfo(iRel).getChar())) + SAS_FONT_TAG_CLOSE
						screen.setTableText(self.RELIGION_TABLE_ID, self.COL_FIRST_RELIGION + iRel, iCity, szReligionIcon, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

				if ReligionUtil.isValid(iReligion):
					for i in range(ReligionUtil.getNumUnitTypes()):
						iUnit = ReligionUtil.getUnit(iReligion, i)
						if pLoopCity.GetCy().getFirstUnitOrder(iUnit) != -1:
							sUnit = self.objectUnderConstruction
						elif pLoopCity.GetCy().canTrain(iUnit, False, False):
							sUnit = self.objectPossible
						else:
							sUnit = self.objectNotPossible
						screen.setTableText(self.RELIGION_TABLE_ID, self.COL_FIRST_UNIT + i, iCity, sasFontTagLabel + sUnit + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

					for i in range(ReligionUtil.getNumBuildingTypes()):
						iBldg = ReligionUtil.getBuilding(iReligion, i)
						sBldg = self.calculateBuilding(pLoopCity, iBldg)
						screen.setTableText(self.RELIGION_TABLE_ID, self.COL_FIRST_BUILDING + i, iCity, sasFontTagLabel + sBldg + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

				sHelp = self.cityHelp(lReligions, pLoopCity.GetCy(), iLinkReligion)
				screen.setTableText(self.RELIGION_TABLE_ID, self.COL_EFFECTS, iCity, sasFontTagLabel + sHelp + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		else:
			szLeftCities = u""
			szRightCities = u""
			for i in range(len(cityList)):
				bFirstColumn = (i % 2 == 0)
				pLoopCity = cityList[i]
				szCityName = u""
				if pLoopCity.isCapital():
					szCityName += u"%c" % CyGame().getSymbolID(FontSymbols.STAR_CHAR)
				lHolyCity = pLoopCity.getHolyCity()
				if lHolyCity:
					for iI in range(len(lHolyCity)):
						szCityName += u"%c" % (gc.getReligionInfo(lHolyCity[iI]).getHolyCityChar())
				lReligions = pLoopCity.getReligions()
				if lReligions:
					for iI in range(len(lReligions)):
						if lReligions[iI] not in lHolyCity:
							szCityName += u"%c" % (gc.getReligionInfo(lReligions[iI]).getChar())
				szCityName += pLoopCity.getName()[0:17] + "  "
				szCityName += self.cityHelp(lReligions, pLoopCity.GetCy(), iLinkReligion)
				if bFirstColumn:
					szLeftCities += u"<font=3>" + szCityName + u"</font>\n"
				else:
					szRightCities += u"<font=3>" + szCityName + u"</font>\n"

			screen.addMultilineText("Child" + self.RELIGION_AREA1_ID, szLeftCities, self.X_CITY1_AREA + 5, self.Y_CITY_AREA + 5, self.W_CITY_AREA - 10, self.H_CITY_AREA - 10, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.addMultilineText("Child" + self.RELIGION_AREA2_ID, szRightCities, self.X_CITY2_AREA + 5, self.Y_CITY_AREA + 5, self.W_CITY_AREA - 10, self.H_CITY_AREA - 10, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		if not self.canConvert(iLinkReligion) or iLinkReligion == self.iReligionOriginal:
			screen.setText(self.EXIT_NAME, "Background", self.EXIT_TEXT, CvUtil.FONT_RIGHT_JUSTIFY, self.X_EXIT, self.Y_EXIT, self.Z_TEXT, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, 1, 0)
			screen.hide(self.CANCEL_NAME)
			szAnarchyTime = CyGameTextMgr().setConvertHelp(self.iActivePlayer, iLinkReligion)
		else:
			# <!-- custom: keep Exit always at the rightmost footer slot; draw Convert on the in-panel status strip so footer tab labels never overlap it. (GPT-5.3-Codex) -->
			screen.setText(self.EXIT_NAME, "Background", self.EXIT_TEXT, CvUtil.FONT_RIGHT_JUSTIFY, self.X_EXIT, self.Y_EXIT, self.Z_TEXT, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, 1, 0)
			# <!-- custom: place Convert on the same in-panel status line as the convert-help text instead of footer, preventing overlap with bottom tab links. (GPT-5.3-Codex) -->
			screen.setText(self.CONVERT_NAME, "Background", self.CONVERT_TEXT, CvUtil.FONT_RIGHT_JUSTIFY, self.X_RELIGION_AREA + self.W_RELIGION_AREA - self.RELIGION_STATUS_RIGHT_PAD, self.Y_RELIGION_STATUS + self.H_RELIGION_STATUS / 2 + self.RELIGION_STATUS_TEXT_Y_OFFSET, self.Z_TEXT, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_CONVERT, iLinkReligion, 1)
			screen.setText(self.RELIGION_CANCEL_NAME, "Background", self.TEXT_CANCEL, CvUtil.FONT_RIGHT_JUSTIFY, self.X_RELIGION_AREA + self.W_RELIGION_AREA - self.RELIGION_STATUS_CANCEL_GAP, self.Y_RELIGION_STATUS + self.H_RELIGION_STATUS / 2 + self.RELIGION_STATUS_TEXT_Y_OFFSET, self.Z_TEXT, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			screen.hide(self.CANCEL_NAME)
			szAnarchyTime = localText.getText("TXT_KEY_ANARCHY_TURNS", (gc.getPlayer(self.iActivePlayer).getReligionAnarchyLength(), ))

		# <!-- custom: in BUG-religious mode, place status text in the last strip of the city panel (no separate third panel) to keep the entire religion body contiguous. (GPT-5.3-Codex) -->
		if bBugReligious:
			screen.setLabel(self.RELIGION_ANARCHY_WIDGET, "Background", sasFontTagLabel + szAnarchyTime + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, self.X_RELIGION_AREA + self.W_RELIGION_AREA / 2, self.Y_RELIGION_STATUS + self.H_RELIGION_STATUS / 2 + self.RELIGION_STATUS_TEXT_Y_OFFSET, self.Z_TEXT, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		else:
			screen.addPanel(self.RELIGION_STATUS_PANEL_ID, "", "", True, True, self.X_RELIGION_AREA, self.Y_RELIGION_STATUS, self.W_RELIGION_AREA, self.H_RELIGION_STATUS, PanelStyles.PANEL_STYLE_MAIN)
			screen.setLabel(self.RELIGION_ANARCHY_WIDGET, "Background", sasFontTagLabel + szAnarchyTime + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, self.X_RELIGION_AREA + self.W_RELIGION_AREA / 2, self.Y_RELIGION_STATUS + self.H_RELIGION_STATUS / 2, self.Z_TEXT, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def cityHelp(self, lReligions, kCity, eHoverReligion):
		# <!-- custom: preserve advc.172 legacy semantics from CvReligionScreen for CyGameTextMgr.getReligionHelpCity (bForceState behavior adjustment via hover-vs-state religion comparison). (GPT-5.3-Codex) -->
		sHelp = ""
		bFirst = True
		eCurrStateReligion = gc.getPlayer(self.iActivePlayer).getStateReligion()
		for eReligion in lReligions:
			sLoopHelp = CyGameTextMgr().getReligionHelpCity(eReligion, kCity, False, False, eHoverReligion != -1 and (eReligion == eHoverReligion) != (eReligion == eCurrStateReligion), eHoverReligion == -1)
			if sLoopHelp:
				if not bFirst:
					sHelp += u", "
				bFirst = False
				sHelp += sLoopHelp
		return sHelp

	def isBugReligiousEnabled(self):
		return not AdvisorOpt.isReligious()

	def getReligionButtonName(self, iReligion):
		return self.RELIGION_BUTTON_NAME + str(iReligion)

	def getReligionTextName(self, iReligion):
		return self.RELIGION_NAME + str(iReligion)

	def canConvert(self, iReligion):
		if isAdvisorReadOnlyPerspective(self.iActivePlayer):
			return False
		iCurrentReligion = gc.getPlayer(self.iActivePlayer).getStateReligion()
		if iReligion == gc.getNumReligionInfos():
			iConvertReligion = -1
		else:
			iConvertReligion = iReligion
		return (iConvertReligion != iCurrentReligion and gc.getPlayer(self.iActivePlayer).canConvert(iConvertReligion))

	def trimReligionColumnText(self, szText, iMaxChars):
		if len(szText) <= iMaxChars:
			return szText
		return szText[:iMaxChars - 1] + u"."

	def trimReligionOwnerWithId(self, szLeaderName, iOwner, iMaxChars):
		return self.trimTextWithId(szLeaderName, iOwner, iMaxChars)

	def trimTextWithId(self, szText, iOwner, iMaxChars):
		szSuffix = u" (%d)" % iOwner
		iMaxLeaderChars = iMaxChars - len(szSuffix)
		if iMaxLeaderChars <= 1:
			return szSuffix
		if len(szText) <= iMaxLeaderChars:
			return szText + szSuffix
		return szText[:iMaxLeaderChars - 1] + u"." + szSuffix

	def ReligionScreenButton(self, inputClass):
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED:
			if inputClass.getID() == gc.getNumReligionInfos() or gc.getGame().getReligionGameTurnFounded(inputClass.getID()) >= 0:
				self.iReligionSelected = inputClass.getID()
				self.iReligionExamined = self.iReligionSelected
				self.drawCityInfo(self.iReligionSelected)
		elif inputClass.getNotifyCode() == NotifyCode.NOTIFY_CURSOR_MOVE_ON:
			if inputClass.getID() == gc.getNumReligionInfos() or gc.getGame().getReligionGameTurnFounded(inputClass.getID()) >= 0:
				self.iReligionExamined = inputClass.getID()
				self.drawCityInfo(self.iReligionExamined)
		elif inputClass.getNotifyCode() == NotifyCode.NOTIFY_CURSOR_MOVE_OFF:
			self.iReligionExamined = self.iReligionSelected
			self.drawCityInfo(self.iReligionSelected)
		return 0

	def ReligionConvert(self, inputClass):
		screen = self.getScreen()
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED:
			screen.hideScreen()

	def ReligionCancel(self, inputClass):
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED:
			self.iReligionSelected = self.iReligionOriginal
			if -1 == self.iReligionSelected:
				self.iReligionSelected = gc.getNumReligionInfos()
			self.drawCityInfo(self.iReligionSelected)

	def calculateBuilding(self, city, bldg):
		if city.getNumBuilding(bldg) > 0:
			return self.objectHave
		elif city.GetCy().getFirstBuildingOrder(bldg) != -1:
			return self.objectUnderConstruction
		elif city.GetCy().canConstruct(bldg, False, False, False):
			return self.objectPossible
		elif city.GetCy().canConstruct(bldg, True, False, False):
			return self.objectPossibleConcurrent
		else:
			return self.objectNotPossible

	def getCorporationButtonName(self, iCorporation):
		return self.CORPORATION_BUTTON_NAME + str(iCorporation)

	def getCorporationTextName(self, iCorporation):
		return self.CORPORATION_NAME + str(iCorporation)

	def drawCorporationInfo(self):
		screen = self.getScreen()
		iMapObserverTeam = self.getMapObserverTeam()
		screen.addPanel(self.CORPORATION_PANEL_ID, "", "", False, True, self.X_CORPORATION_AREA, self.Y_CORPORATION_AREA, self.W_CORPORATION_AREA, self.H_CORPORATION_AREA, PanelStyles.PANEL_STYLE_MAIN)

		xLoop = self.X_CORPORATION_START
		iButtonOffset = self.CORPORATION_BUTTON_SIZE / 2
		for iCorp in range(gc.getNumCorporationInfos()):
			szButtonName = self.getCorporationButtonName(iCorp)
			kCorpInfo = gc.getCorporationInfo(iCorp)
			screen.addCheckBoxGFCAt(self.CORPORATION_PANEL_ID, szButtonName, kCorpInfo.getButton(), self.BUTTON_HILITE_SQUARE_ART, xLoop - iButtonOffset, self.Y_CORPORATION_BUTTONS - iButtonOffset, self.CORPORATION_BUTTON_SIZE, self.CORPORATION_BUTTON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1, ButtonStyles.BUTTON_STYLE_LABEL, False)
			xLoop += self.DX_CORPORATION

		xLoop = self.X_CORPORATION_START
		for iCorp in range(gc.getNumCorporationInfos()):
			szGreatPerson = u""
			iFoundingUnit = -1
			for iBuilding in range(gc.getNumBuildingInfos()):
				if gc.getBuildingInfo(iBuilding).getFoundsCorporation() == iCorp:
					break
			for iUnit in range(gc.getNumUnitInfos()):
				kUnitInfo = gc.getUnitInfo(iUnit)
				if kUnitInfo.getBuildings(iBuilding) or kUnitInfo.getForceBuildings(iBuilding):
					iFoundingUnit = iUnit
					szGreatPerson = kUnitInfo.getDescription()
					break
			iBuiltByLeftX = xLoop - 44
			if iFoundingUnit != -1:
				# <!-- custom: render Built by as a true inline pair (icon + text) by using one shared left anchor per column. (GPT-5.3-Codex) -->
				screen.setImageButtonAt(self.getCorporationTextName(iCorp) + "GreatPersonIcon", self.CORPORATION_PANEL_ID, kUnitInfo.getButton(), iBuiltByLeftX, self.Y_CORPORATION_GREAT_PERSON, 24, 24, WidgetTypes.WIDGET_GENERAL, -1, -1)
			screen.setLabelAt(self.getCorporationTextName(iCorp) + "GreatPerson", self.CORPORATION_PANEL_ID, sasFontTagLabel + szGreatPerson + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, iBuiltByLeftX + 22, self.Y_CORPORATION_GREAT_PERSON, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			xLoop += self.DX_CORPORATION

		xLoop = self.X_CORPORATION_START
		for iCorp in range(gc.getNumCorporationInfos()):
			# <!-- custom: cache corporation info once per column and reuse across all row renderers to avoid repeated lookups in the same draw pass. (GPT-5.3-Codex) -->
			kCorpInfo = gc.getCorporationInfo(iCorp)
			screen.setLabelAt(self.getCorporationTextName(iCorp) + "Consumes", self.CORPORATION_PANEL_ID, sasFontTagLabel + self.getCorporationConsumedText(kCorpInfo) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, self.Y_CORPORATION_CONSUMES, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			screen.setLabelAt(self.getCorporationTextName(iCorp) + "Generates", self.CORPORATION_PANEL_ID, sasFontTagLabel + self.getCorporationGeneratedBonusText(kCorpInfo) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, self.Y_CORPORATION_GENERATES_BONUS, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			screen.setLabelAt(self.getCorporationTextName(iCorp) + "GeneratesYields", self.CORPORATION_PANEL_ID, sasFontTagLabel + self.getCorporationGeneratedText(kCorpInfo) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, self.Y_CORPORATION_GENERATES, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			xLoop += self.DX_CORPORATION

		# <!-- custom: explicit row labels for corporation effects to keep top-panel semantics clear after tab integration and row reordering. (GPT-5.3-Codex) -->
		screen.setLabelAt(self.CORPORATION_HEADER_BUILT_BY_ID, self.CORPORATION_PANEL_ID, self.LABEL_CORP_ROW_BUILT_BY, CvUtil.FONT_LEFT_JUSTIFY, self.LEFT_EDGE_TEXT, self.Y_CORPORATION_GREAT_PERSON, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.setLabelAt(self.CORPORATION_HEADER_CONSUMES_ID, self.CORPORATION_PANEL_ID, self.LABEL_CORP_ROW_CONSUMES, CvUtil.FONT_LEFT_JUSTIFY, self.LEFT_EDGE_TEXT, self.Y_CORPORATION_CONSUMES, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.setLabelAt(self.CORPORATION_HEADER_GENERATES_ID, self.CORPORATION_PANEL_ID, self.LABEL_CORP_ROW_GENERATES, CvUtil.FONT_LEFT_JUSTIFY, self.LEFT_EDGE_TEXT, self.Y_CORPORATION_GENERATES_BONUS, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.setLabelAt(self.CORPORATION_HEADER_GENERATES_YIELDS_ID, self.CORPORATION_PANEL_ID, self.LABEL_CORP_ROW_GENERATES_2, CvUtil.FONT_LEFT_JUSTIFY, self.LEFT_EDGE_TEXT, self.Y_CORPORATION_GENERATES, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.setLabelAt(self.CORPORATION_HEADER_FOUNDED_ID, self.CORPORATION_PANEL_ID, self.LABEL_CORP_ROW_FOUNDED, CvUtil.FONT_LEFT_JUSTIFY, self.LEFT_EDGE_TEXT, self.Y_CORPORATION_FOUNDED, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		xLoop = self.X_CORPORATION_START
		for iCorp in range(gc.getNumCorporationInfos()):
			if gc.getGame().getCorporationGameTurnFounded(iCorp) < 0:
				szFounded = self.TEXT_NOT_FOUNDED
			else:
				szFounded = CyGameTextMgr().getTimeStr(gc.getGame().getCorporationGameTurnFounded(iCorp), false)
			screen.setLabelAt(self.getCorporationTextName(iCorp) + "Founded", self.CORPORATION_PANEL_ID, sasFontTagLabel + szFounded + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, self.Y_CORPORATION_FOUNDED, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			xLoop += self.DX_CORPORATION

		# <!-- custom: use one-line HQ values with owner ID (no separate nationality line) to match Religion-style concise ownership display and reduce vertical crowding. (GPT-5.3-Codex) -->
		screen.setLabelAt(self.CORPORATION_HEADER_HEADQUARTERS_ID, self.CORPORATION_PANEL_ID, self.LABEL_CORP_ROW_HQ_ID, CvUtil.FONT_LEFT_JUSTIFY, self.LEFT_EDGE_TEXT, self.Y_CORPORATION_HEADQUARTERS, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		xLoop = self.X_CORPORATION_START
		for iCorp in range(gc.getNumCorporationInfos()):
			pHeadquarters = gc.getGame().getHeadquarters(iCorp)
			if pHeadquarters.isNone():
				screen.setLabelAt(self.getCorporationTextName(iCorp) + "HeadquartersCity", self.CORPORATION_PANEL_ID, sasFontTagLabel + u"-" + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, self.Y_CORPORATION_HEADQUARTERS, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			elif not pHeadquarters.isRevealed(iMapObserverTeam, False):
				screen.setLabelAt(self.getCorporationTextName(iCorp) + "HeadquartersCity", self.CORPORATION_PANEL_ID, self.LABEL_UNKNOWN, CvUtil.FONT_CENTER_JUSTIFY, xLoop, self.Y_CORPORATION_HEADQUARTERS, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			else:
				iOwner = pHeadquarters.getOwner()
				szHeadquarters = self.trimTextWithId(pHeadquarters.getName(), iOwner, self.CORPORATION_HQ_MAX_CHARS_WITH_ID)
				screen.setLabelAt(self.getCorporationTextName(iCorp) + "HeadquartersCity", self.CORPORATION_PANEL_ID, sasFontTagLabel + szHeadquarters + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, xLoop, self.Y_CORPORATION_HEADQUARTERS, self.DZ, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			xLoop += self.DX_CORPORATION

	def getCorporationGeneratedText(self, kCorpInfo):
		szText = u""
		iWorldPercent = gc.getWorldInfo(gc.getMap().getWorldSize()).getCorporationMaintenancePercent()
		for iYield in range(YieldTypes.NUM_YIELD_TYPES):
			iValue = self.getScaledCorporationGeneratedValue(kCorpInfo.getYieldProduced(iYield), iWorldPercent)
			if iValue != 0:
				if len(szText) > 0:
					szText += u", "
				szText += self.getCorporationGeneratedValueText(iValue) + u"%c" % gc.getYieldInfo(iYield).getChar()
		for iCommerce in range(CommerceTypes.NUM_COMMERCE_TYPES):
			iValue = self.getScaledCorporationGeneratedValue(kCorpInfo.getCommerceProduced(iCommerce), iWorldPercent)
			if iValue != 0:
				if len(szText) > 0:
					szText += u", "
				szText += self.getCorporationGeneratedValueText(iValue) + u"%c" % gc.getCommerceInfo(iCommerce).getChar()
		if len(szText) <= 0:
			return u"-"
		return szText

	def getScaledCorporationGeneratedValue(self, iStoredValue, iWorldPercent):
		# <!-- custom: Corporation XML yield/commerce values are stored in hundredths.
		# Match native CvGameTextMgr truncation after world-size scaling, including C++ truncation toward zero for negative values. See KI#231. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		iScaledValueX100 = iStoredValue * iWorldPercent
		if iScaledValueX100 >= 0:
			return iScaledValueX100 / 100
		return -((-iScaledValueX100) / 100)

	def getCorporationGeneratedValueText(self, iValueX100):
		# <!-- custom: Display scaled corporation rates as exact signed units (+1, +0.50, +1.50) instead of exposing the raw hundredths (100, 50, 150).
		# Use integer text assembly to avoid floating-point approximation. See KI#231. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		iAbsoluteValue = abs(iValueX100)
		szSign = u"+"
		if iValueX100 < 0:
			szSign = u"-"
		szText = szSign + unicode(iAbsoluteValue / 100)
		iFraction = iAbsoluteValue % 100
		if iFraction != 0:
			szFraction = unicode(iFraction)
			if iFraction < 10:
				szFraction = u"0" + szFraction
			szText += u"." + szFraction
		return szText

	def getMapObserverTeam(self):
		# <!-- custom: A selected vassal remains the Policy Advisor's subject, but outside debug its Holy City and Corporation Headquarters rows must use the real active player's map knowledge so the perspective selector cannot reveal hidden locations.
		# Debug mode deliberately retains the selected player's map knowledge. See KI#232. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if CyGame().isDebugMode():
			return gc.getPlayer(self.iActivePlayer).getTeam()
		return gc.getPlayer(CyGame().getActivePlayer()).getTeam()

	def getCorporationConsumedText(self, kCorpInfo):
		# <!-- custom: keep consumed prerequisites as a single natural comma-separated bonus-char list; no row splitting needed at current panel width. (GPT-5.3-Codex) -->
		szText = u""
		for iRequired in range(self.NUM_CORPORATION_PREREQ_BONUSES):
			eBonus = kCorpInfo.getPrereqBonus(iRequired)
			if eBonus != -1:
				if len(szText) > 0:
					szText += u", "
				szText += u"%c" % (gc.getBonusInfo(eBonus).getChar(),)
		if len(szText) <= 0:
			return u"-"
		return szText

	def getCorporationGeneratedBonusText(self, kCorpInfo):
		eBonus = kCorpInfo.getBonusProduced()
		if eBonus == -1:
			return u"-"
		return u"%c" % (gc.getBonusInfo(eBonus).getChar(),)

	def drawCorporationCityInfo(self, iCorporation):
		screen = self.getScreen()
		screen.deleteWidget(self.CORPORATION_AREA1_ID)
		screen.deleteWidget(self.CORPORATION_AREA2_ID)

		if iCorporation == gc.getNumCorporationInfos():
			iLinkCorporation = -1
		else:
			iLinkCorporation = iCorporation

		screen.addPanel(self.CORPORATION_AREA1_ID, "", "", True, True, self.X_CORPORATION_CITY1_AREA, self.Y_CORPORATION_CITY_AREA, self.W_CORPORATION_CITY_AREA, self.H_CORPORATION_CITY_AREA, PanelStyles.PANEL_STYLE_MAIN)
		screen.addPanel(self.CORPORATION_AREA2_ID, "", "", True, True, self.X_CORPORATION_CITY2_AREA, self.Y_CORPORATION_CITY_AREA, self.W_CORPORATION_CITY_AREA, self.H_CORPORATION_CITY_AREA, PanelStyles.PANEL_STYLE_MAIN)

		for iCorp in range(gc.getNumCorporationInfos()):
			screen.setState(self.getCorporationButtonName(iCorp), self.iCorporationSelected == iCorp)

		iPlayer = PyPlayer(self.iActivePlayer)
		cityList = iPlayer.getCityList()
		szLeftCities = u""
		szRightCities = u""
		for iCity in range(len(cityList)):
			bFirstColumn = (iCity % 2 == 0)
			pLoopCity = cityList[iCity]
			szCityName = u""
			if pLoopCity.isCapital():
				szCityName += u"%c" % CyGame().getSymbolID(FontSymbols.STAR_CHAR)

			lHeadquarters = pLoopCity.getHeadquarters()
			if lHeadquarters:
				for iHeadquarter in range(len(lHeadquarters)):
					szCityName += u"%c" % (gc.getCorporationInfo(lHeadquarters[iHeadquarter]).getHeadquarterChar())

			lCorporations = pLoopCity.getCorporations()
			if lCorporations:
				for iCorporationInCity in range(len(lCorporations)):
					if lCorporations[iCorporationInCity] not in lHeadquarters:
						szCityName += u"%c" % (gc.getCorporationInfo(lCorporations[iCorporationInCity]).getChar())

			szCityName += pLoopCity.getName()[0:17] + "  "
			if iLinkCorporation == -1:
				bFirst = True
				for iCorporationInCity in range(len(lCorporations)):
					szTempBuffer = CyGameTextMgr().getCorporationHelpCity(lCorporations[iCorporationInCity], pLoopCity.GetCy(), False, False)
					if szTempBuffer:
						if not bFirst:
							szCityName += u", "
						szCityName += szTempBuffer
						bFirst = False
			else:
				szCityName += CyGameTextMgr().getCorporationHelpCity(iLinkCorporation, pLoopCity.GetCy(), False, True)

			if bFirstColumn:
				szLeftCities += sasFontTagLabel + szCityName + SAS_FONT_TAG_CLOSE + u"\n"
			else:
				szRightCities += sasFontTagLabel + szCityName + SAS_FONT_TAG_CLOSE + u"\n"

		screen.addMultilineText("Child" + self.CORPORATION_AREA1_ID, szLeftCities, self.X_CORPORATION_CITY1_AREA + 5, self.Y_CORPORATION_CITY_AREA + 5, self.W_CORPORATION_CITY_AREA - 10, self.H_CORPORATION_CITY_AREA - 10, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.addMultilineText("Child" + self.CORPORATION_AREA2_ID, szRightCities, self.X_CORPORATION_CITY2_AREA + 5, self.Y_CORPORATION_CITY_AREA + 5, self.W_CORPORATION_CITY_AREA - 10, self.H_CORPORATION_CITY_AREA - 10, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.hide(self.CANCEL_NAME)
		screen.setText(self.EXIT_NAME, "Background", self.EXIT_TEXT, CvUtil.FONT_RIGHT_JUSTIFY, self.X_EXIT, self.Y_EXIT, self.Z_TEXT, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, 1, 0)

	def CorporationScreenButton(self, inputClass):
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED:
			if self.iCorporationSelected == inputClass.getID():
				self.iCorporationSelected = -1
			else:
				self.iCorporationSelected = inputClass.getID()
			self.drawCorporationCityInfo(self.iCorporationSelected)
		elif inputClass.getNotifyCode() == NotifyCode.NOTIFY_CURSOR_MOVE_ON:
			self.drawCorporationCityInfo(inputClass.getID())
		elif inputClass.getNotifyCode() == NotifyCode.NOTIFY_CURSOR_MOVE_OFF:
			self.drawCorporationCityInfo(self.iCorporationSelected)
		return 0

	def closePolicyScreen(self, inputClass):
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED:
			self.getScreen().hideScreen()

	# Will handle the input for this screen...
	def handleInput(self, inputClass):
		szWidgetName = inputClass.getFunctionName()
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED and szWidgetName.startswith("PolicyTabButton"):
			iData1 = inputClass.getData1()
			if iData1 in [self.PAGE_POLICY, self.PAGE_RELIGION, self.PAGE_CORPORATION] and iData1 != self.iPage:
				self.iPage = iData1
				self.drawContents()
			return 1

		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED):
			screen = self.getScreen()
			if self.iPage == self.PAGE_POLICY:
				self.setActivePlayer(getAdvisorDebugDropdownSelectedPlayer(screen, self.DEBUG_DROPDOWN_ID))
				self.drawContents()
			elif self.iPage == self.PAGE_RELIGION:
				szWidgetName = inputClass.getFunctionName()
				if szWidgetName != self.RELIGION_TABLE_ID:
					self.iActivePlayer = getAdvisorDebugDropdownSelectedPlayer(screen, self.DEBUG_DROPDOWN_ID)
					self.drawReligionInfo()
					self.drawHelpInfo()
					self.drawCityInfo(self.iReligionSelected)
			else:
				self.iActivePlayer = getAdvisorDebugDropdownSelectedPlayer(screen, self.DEBUG_DROPDOWN_ID)
				self.drawCorporationInfo()
				self.drawCorporationCityInfo(self.iCorporationSelected)
			return 1

		if self.iPage == self.PAGE_POLICY:
			if self.PolicyAdvisorScreenInputMap.has_key(inputClass.getFunctionName()):
				self.PolicyAdvisorScreenInputMap.get(inputClass.getFunctionName())(inputClass)
				return 1
		else:
			if self.iPage == self.PAGE_RELIGION:
				if szWidgetName == self.RELIGION_TABLE_ID:
					if inputClass.getMouseX() == 0:
						if isAdvisorReadOnlyPerspective(self.iActivePlayer):
							return 1
						screen = self.getScreen()
						screen.hideScreen()
						pPlayer = gc.getPlayer(inputClass.getData1())
						pCity = pPlayer.getCity(inputClass.getData2())
						CyInterface().selectCity(pCity, true)
					return 1
				if szWidgetName == self.CANCEL_NAME:
					self.ReligionCancel(inputClass)
					return 1
				if szWidgetName == self.RELIGION_NAME or szWidgetName == self.RELIGION_BUTTON_NAME:
					self.ReligionScreenButton(inputClass)
					return 1
				if szWidgetName == self.RELIGION_CANCEL_NAME and inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED:
					self.ReligionCancel(inputClass)
					return 1
				if szWidgetName == self.CONVERT_NAME and inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED:
					self.ReligionConvert(inputClass)
					return 1
				if szWidgetName == self.EXIT_NAME and inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED:
					self.closePolicyScreen(inputClass)
					return 1
				return 0
			if self.iPage == self.PAGE_CORPORATION:
				if szWidgetName == self.CORPORATION_NAME or szWidgetName == self.CORPORATION_BUTTON_NAME:
					self.CorporationScreenButton(inputClass)
					return 1
				if szWidgetName == self.EXIT_NAME and inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED:
					self.closePolicyScreen(inputClass)
					return 1
				return 0
			return 0
		return 0

	def update(self, fDelta):
		# <!-- custom: no Religion dirty-bit enum exists in this branch; keep tab refresh event-driven via input/actions. (GPT-5.3-Codex) -->
		return
