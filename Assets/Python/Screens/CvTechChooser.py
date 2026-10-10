## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
from CvPythonExtensions import *
import CvUtil
import ScreenInput
import CvScreenEnums
from SASFontUtils import *
from SASUtils import *
from SASMagicNumbers import *
# <!-- custom: AdvCiv-SAS readability pass: use LABEL as the base tech-chooser text tag (instead of BODY) for clearer upscaled UI text. (GPT-5.3-Codex) -->
import CvScreensInterface

TEXTURE_SIZE = 24
X_START = 6
X_INCREMENT = 27
Y_ROW = 32

CIV_HAS_TECH = 0
CIV_IS_RESEARCHING = 1
CIV_NO_RESEARCH = 2
CIV_TECH_AVAILABLE = 3

# globals
gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

# BUG - GP Tech Prefs - start
import TechPrefs
import BugCore
BugOpt = BugCore.game.Advisors
ClockOpt = BugCore.game.NJAGC

import BugUtil

PREF_ICON_SIZE = 24
#PREF_ICON_TOP = 168
# <advc.004a>
# <!-- custom: GP pref icon anchor is runtime-derived from the Exit baseline; keep only a screen-independent offset here for fast tuning. (GPT-5.3-Codex) -->
PREF_ICON_BOTTOM_FROM_EXIT_OFFSET = 10
PREF_ICON_LEFT_OFFSET = 10
# </advc.004a>
FLAVORS = [TechPrefs.FLAVOR_PRODUCTION, TechPrefs.FLAVOR_GOLD, TechPrefs.FLAVOR_SCIENCE, TechPrefs.FLAVOR_CULTURE, TechPrefs.FLAVOR_RELIGION]
UNIT_CLASSES = ["UNITCLASS_GREAT_ENGINEER", "UNITCLASS_GREAT_MERCHANT", "UNITCLASS_GREAT_SCIENTIST", "UNITCLASS_GREAT_ARTIST", "UNITCLASS_GREAT_PROPHET"]
# BUG - GP Tech Prefs - end

# BUG - 3.19 No Espionage - start
import GameUtil
# BUG - 3.19 No Espionage - end

# BUG - Mac Support - start
BugUtil.fixSets(globals())
# BUG - Mac Support - end

# BUG - Tech Era Colors - start
def getEraDescription(eWidgetType, iData1, iData2, bOption):
	return gc.getEraInfo(iData1).getDescription()
# BUG - Tech Era Colors - end

# BUG - GP Tech Prefs - start
def resetTechPrefs(args=[]):
	CvScreensInterface.techChooser.resetTechPrefs()

def getAllTechPrefsHover(widgetType, iData1, iData2, bOption):
	#return buildTechPrefsHover("TXT_KEY_BUG_TECH_PREFS_ALL", CvScreensInterface.techChooser.pPrefs.getAllFlavorTechs(iData1))
	# <advc.004a> Use slightly different text and name the GP
	msg = BugUtil.getPlainText("TXT_KEY_ADVC_TECH_PREFS_ALL")
	iUnitClass = getInfoTypeOrFail(UNIT_CLASSES[iData1])
	pUnitInfo = gc.getUnitInfo(gc.getUnitClassInfo(iUnitClass).getDefaultUnitIndex())
	msg += " (" + pUnitInfo.getDescription() + ")" # </advc.004a>
	return buildTechPrefsHover(msg, CvScreensInterface.techChooser.pPrefs.getRemainingFlavorTechs(FLAVORS[iData1])) # K-Mod

def getCurrentTechPrefsHover(widgetType, iData1, iData2, bOption):
	#return buildTechPrefsHover("TXT_KEY_BUG_TECH_PREFS_CURRENT", CvScreensInterface.techChooser.pPrefs.getCurrentFlavorTechs(iData1))
	# <advc.004a> CurrentFlavorTechs isn't useful info. Instead, show all techs that have higher priority than iData2 (the current discover tech) and don't require iData2.
	sTechs = set()
	if iData1 >= 0 and iData2 >= 0:
		bCurrentTechFound = False
		for pTech in CvScreensInterface.techChooser.pPrefs.getRemainingFlavorTechs(iData1):
			if pTech.getID() == iData2:
				bCurrentTechFound = True
				break # RemainingFlavorTechs is sorted by flavor
			if pTech.getInfo().getEra() <= gc.getPlayer(gc.getGame().getActivePlayer()).getCurrentEra() + 1 and not CvScreensInterface.techChooser.pPrefs.getTech(iData2).isInevitableReq(pTech):
				bValid = True
				for pAlreadyAdded in sTechs:
					if pAlreadyAdded.isInevitableReq(pTech):
						bValid = False
						break
				if bValid:
					sTechs.add(pTech)
					for pAlreadyAdded in sTechs.copy():
						if pTech.isInevitableReq(pAlreadyAdded):
							sTechs.remove(pAlreadyAdded)
		assert bCurrentTechFound
	msg = BugUtil.getPlainText("TXT_KEY_ADVC_TECH_PREFS_CURRENT")
	if len(sTechs) <= 0:
		return msg
	sCurrentTech = set()
	sCurrentTech.add(CvScreensInterface.techChooser.pPrefs.getTech(iData2))
	return msg + ":" + buildTechPrefsHover("", sCurrentTech) + "\n" + buildTechPrefsHover(BugUtil.getPlainText("TXT_KEY_ADVC_TECH_PREFS_MISSING_REQ"), sTechs)
	# </advc.004a>

def getFutureTechPrefsHover(widgetType, iData1, iData2, bOption):
	pPlayer = gc.getPlayer(CvScreensInterface.techChooser.iCivSelected)
	sTechs = set()
	for i in range(gc.getNumTechInfos()):
		if (pPlayer.isResearchingTech(i)):
			sTechs.add(CvScreensInterface.techChooser.pPrefs.getTech(i))
	#return buildTechPrefsHover("TXT_KEY_BUG_TECH_PREFS_FUTURE", CvScreensInterface.techChooser.pPrefs.getCurrentWithFlavorTechs(iData1, sTechs))
	# advc.004a: Show the currently queued techs instead
	return buildTechPrefsHover(BugUtil.getPlainText("TXT_KEY_ADVC_TECH_PREFS_FUTURE"), sTechs)

# <advc.004a>
def getTechPrefsHeadingHover(widgetType, iData1, iData2, bOption):
	return BugUtil.getPlainText("TXT_KEY_ADVC_TECH_PREFS_HEADING")
# </advc.004a>

def buildTechPrefsHover(msg, lTechs):
	#szText = BugUtil.getPlainText(msg) + "\n" # advc.004a: Let the caller do this
	szText = msg + "\n"
	for pTech in lTechs:
		szText += "<img=%s size=24></img>" % pTech.getInfo().getButton().replace(" ", "_")
	return szText
# BUG - GP Tech Prefs - end

class CvTechChooser:
	"Tech Chooser Screen"

	def __init__(self):
		self.nWidgetCount = 0
		self.iCivSelected = -1
		self.aiCurrentState = []

		# Advanced Start
		self.m_iSelectedTech = -1
		self.m_bSelectedTechDirty = false
		self.m_bTechRecordsDirty = false

# BUG - GP Tech Prefs - start
		self.bPrefsShowing = False
		self.resetTechPrefs()
# BUG - GP Tech Prefs - end

		self.PIXEL_INCREMENT = 7
		self.BOX_INCREMENT_WIDTH = 27 # Used to be 33 #Should be a multiple of 3...
		self.BOX_INCREMENT_HEIGHT = 9 #Should be a multiple of 3...
		self.BOX_INCREMENT_Y_SPACING = 6 #Should be a multiple of 3...
		self.BOX_INCREMENT_X_SPACING = 9 #Should be a multiple of 3...

		# <!-- custom: parametrize properly the X starting position of the first column; value was hardcoded repeatedly. Credit: Gemini 3 Pro. (GPT-5.2-Codex (summarized)) -->
		# <!-- custom: at 1080p, the old first tech-grid column padding 30 partially clipped the last Ancient-era column to the right on the default opening view; 0 keeps all current rows fully and symmetrically visible without having to tediously nudge the horizontal scrollbar at game start, so prefer it.
		# Also replace the old manual 30 / 24 pair with one start value and a derived start - 6 OR-prereq value so they cannot drift. (GPT-5.5) -->
		self.iSAS_TECH_GRID_LEFT_START = 0
		self.iX_LEFT_START = self.iSAS_TECH_GRID_LEFT_START
		self.iX_LEFT_START_OR_PREREQS = self.iX_LEFT_START - 6

		self.iSAS_CV_TECH_CHOOSER_LOW_RES_SCOREBOARD_SWAP_WIDTH = None
		self.iSAS_CV_TECH_CHOOSER_RIGHT_SPACE_FOR_SCOREBOARD_LOW_RES = None
		self.iSAS_CV_TECH_CHOOSER_TOP_SPACE_FOR_TECH_BAR_LOW_RES = None
		self.PREF_ICON_LEFT = self.iX_LEFT_START + PREF_ICON_LEFT_OFFSET
		self.PREF_ICON_BOTTOM = 0
		self.iLanguageLoaded = -1
		self.bTechChooserArtCached = False
		# <!-- custom: cached vanilla engine define; looked up techs x terrains times during tech tree draw. (Claude code Sonnet 4.6) -->
		self.iDEEP_WATER_TERRAIN = None

	def initText(self):
		# <!-- custom: this file had no shared text/art cache before; add one-time language/art caching and reuse it in hot tech-tree loops to avoid repeated lookup work on redraw. (GPT-5.3-Codex) -->
		# <!-- custom: keep first-call cache init available even before final init because Tech Chooser can be entered early; only skip repeated pre-final-init calls once language cache fields already exist. (GPT-5.3-Codex) -->
		if (not CyGame().isFinalInitialized() and self.iLanguageLoaded != -1):
			return
		if self.iLanguageLoaded != CyGame().getCurrentLanguage():
			self.iLanguageLoaded = CyGame().getCurrentLanguage()
			self.TEXT_WB_AS_ADD_TECH = localText.getText("TXT_KEY_WB_AS_ADD_TECH", ())
			self.TEXT_TECH_CHOOSER_TITLE = localText.getText("TXT_KEY_TECH_CHOOSER_TITLE", ()).upper()
			self.TEXT_PEDIA_SCREEN_EXIT = localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT", ()).upper()
			self.TEXT_TECH_CHOOSER_EXIT = sasFontTagTitle + self.TEXT_PEDIA_SCREEN_EXIT + SAS_FONT_TAG_CLOSE
			self.TEXT_TECH_CHOOSER_TITLE_FMT = sasFontTagTitle.bold + self.TEXT_TECH_CHOOSER_TITLE + SAS_FONT_TAG_CLOSE

		if self.bTechChooserArtCached:
			return
		self.bTechChooserArtCached = True
		# BUG - GP Tech Prefs - start
		self.NO_TECH_ART = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTONS_CANCEL").getPath()
		# BUG - GP Tech Prefs - end
		self.ART_SCREEN_BG_OPAQUE = ArtFileMgr.getInterfaceArtInfo("SCREEN_BG_OPAQUE").getPath()
		self.ART_RED_X = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTONS_RED_X").getPath()
		self.ART_TECH_MOVE_BONUS = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_MOVE_BONUS").getPath()
		self.ART_TECH_FEATURE_PRODUCTION = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_FEATURE_PRODUCTION").getPath()
		self.ART_TECH_WORKER_SPEED = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_WORKER_SPEED").getPath()
		self.ART_TECH_TRADE_ROUTES = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_TRADE_ROUTES").getPath()
		self.ART_TECH_HEALTH = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_HEALTH").getPath()
		self.ART_TECH_HAPPINESS = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_HAPPINESS").getPath()
		self.ART_TECH_CULTURE = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_CULTURE").getPath()
		self.ART_TECH_ESPIONAGE = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_ESPIONAGE").getPath()
		self.ART_GENERAL_QUESTIONMARK = ArtFileMgr.getInterfaceArtInfo("INTERFACE_GENERAL_QUESTIONMARK").getPath()
		self.ART_TECH_FREETECH = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_FREETECH").getPath()
		self.ART_TECH_LOS = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_LOS").getPath()
		self.ART_TECH_MAPCENTER = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_MAPCENTER").getPath()
		self.ART_TECH_MAPREVEAL = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_MAPREVEAL").getPath()
		self.ART_TECH_MAPTRADING = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_MAPTRADING").getPath()
		self.ART_TECH_TECHTRADING = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_TECHTRADING").getPath()
		self.ART_TECH_GOLDTRADING = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_GOLDTRADING").getPath()
		self.ART_TECH_OPENBORDERS = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_OPENBORDERS").getPath()
		self.ART_TECH_DEFENSIVEPACT = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_DEFENSIVEPACT").getPath()
		self.ART_TECH_PERMALLIANCE = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_PERMALLIANCE").getPath()
		self.ART_TECH_VASSAL = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_VASSAL").getPath()
		self.ART_TECH_BRIDGEBUILDING = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_BRIDGEBUILDING").getPath()
		self.ART_TECH_IRRIGATION = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_IRRIGATION").getPath()
		self.ART_TECH_NOIRRIGATION = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_NOIRRIGATION").getPath()
		self.ART_TECH_WATERWORK = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_WATERWORK").getPath()
		self.ART_TECH_WATERMOVES = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_WATERMOVES").getPath()
		self.ART_TECH_RIVERTRADE = ArtFileMgr.getInterfaceArtInfo("INTERFACE_TECH_RIVERTRADE").getPath()
		self.ART_POPUPBUTTON_RELIGION = ArtFileMgr.getInterfaceArtInfo("INTERFACE_POPUPBUTTON_RELIGION").getPath()
		self.ARROW_X = ArtFileMgr.getInterfaceArtInfo("ARROW_X").getPath()
		self.ARROW_Y = ArtFileMgr.getInterfaceArtInfo("ARROW_Y").getPath()
		self.ARROW_MXMY = ArtFileMgr.getInterfaceArtInfo("ARROW_MXMY").getPath()
		self.ARROW_XY = ArtFileMgr.getInterfaceArtInfo("ARROW_XY").getPath()
		self.ARROW_MXY = ArtFileMgr.getInterfaceArtInfo("ARROW_MXY").getPath()
		self.ARROW_XMY = ArtFileMgr.getInterfaceArtInfo("ARROW_XMY").getPath()
		self.ARROW_HEAD = ArtFileMgr.getInterfaceArtInfo("ARROW_HEAD").getPath()

	def getScreen(self):
		return CyGInterfaceScreen( "TechChooser", CvScreenEnums.TECH_CHOOSER )

	def hideScreen (self):
		# Get the screen
		screen = self.getScreen()

		# Hide the screen
		screen.hideScreen()
		# <!-- custom: force scoreboard redraw when closing Tech Chooser so any Tech-Chooser-specific alignment/layout rules revert immediately. (GPT-5.3-Codex) -->
		CyInterface().setDirty(InterfaceDirtyBits.Score_DIRTY_BIT, True)

	def updateRuntimeLayout(self, screen):
		# <!-- custom: runtime geometry wrapper to mirror Info/Foreign advisor structure: keep resolution-dependent bounds in one place and out of init-time setup. (GPT-5.3-Codex) -->
		if self.iSAS_CV_TECH_CHOOSER_LOW_RES_SCOREBOARD_SWAP_WIDTH is None:
			self.iSAS_CV_TECH_CHOOSER_LOW_RES_SCOREBOARD_SWAP_WIDTH = gc.getDefineINT("SAS_CV_TECH_CHOOSER_LOW_RES_SCOREBOARD_SWAP_WIDTH")
		if self.iSAS_CV_TECH_CHOOSER_RIGHT_SPACE_FOR_SCOREBOARD_LOW_RES is None:
			self.iSAS_CV_TECH_CHOOSER_RIGHT_SPACE_FOR_SCOREBOARD_LOW_RES = gc.getDefineINT("SAS_CV_TECH_CHOOSER_RIGHT_SPACE_FOR_SCOREBOARD_LOW_RES")
		if self.iSAS_CV_TECH_CHOOSER_TOP_SPACE_FOR_TECH_BAR_LOW_RES is None:
			self.iSAS_CV_TECH_CHOOSER_TOP_SPACE_FOR_TECH_BAR_LOW_RES = gc.getDefineINT("SAS_CV_TECH_CHOOSER_TOP_SPACE_FOR_TECH_BAR_LOW_RES")
		iRightSpaceForScoreboard = SAS_ADVISOR_RIGHT_SPACE_FOR_SCOREBOARD
		iTopSpaceForTechBar = SAS_ADVISOR_TOP_SPACE_FOR_TECH_BAR
		# <!-- custom: optional right-space replacement for scoreboard; on lower resolutions this lets users render a larger advisor so one more tech-tree column can be visible. (GPT-5.3-Codex) -->
		if self.iSAS_CV_TECH_CHOOSER_LOW_RES_SCOREBOARD_SWAP_WIDTH > 0 and screen.getXResolution() < self.iSAS_CV_TECH_CHOOSER_LOW_RES_SCOREBOARD_SWAP_WIDTH:
			iRightSpaceForScoreboard = self.iSAS_CV_TECH_CHOOSER_RIGHT_SPACE_FOR_SCOREBOARD_LOW_RES
			# <!-- custom: optional low-res top-space replacement paired with the low-res width swap; useful to keep map-view advisor buttons accessible when Tech Chooser is widened. (GPT-5.3-Codex) -->
			iTopSpaceForTechBar = self.iSAS_CV_TECH_CHOOSER_TOP_SPACE_FOR_TECH_BAR_LOW_RES
		if iRightSpaceForScoreboard < 0:
			iRightSpaceForScoreboard = 0
		if iTopSpaceForTechBar < 0:
			iTopSpaceForTechBar = 0
		self.X_SCREEN, self.Y_SCREEN, self.W_SCREEN, self.H_SCREEN = getAdvisorRuntimeBounds(screen, SAS_ADVISOR_LEFT_SPACE_FOR_COMMERCE_SLIDERS, iRightSpaceForScoreboard, iTopSpaceForTechBar, SAS_ADVISOR_BOTTOM_SPACE)
		self.iX_LEFT_START = self.iSAS_TECH_GRID_LEFT_START
		self.iX_LEFT_START_OR_PREREQS = self.iX_LEFT_START - 6
		# <!-- custom: keep GP prefs aligned with the tech grid and Exit/footer anchors at any resolution. (GPT-5.3-Codex) -->
		self.PREF_ICON_LEFT = self.iX_LEFT_START + PREF_ICON_LEFT_OFFSET
		self.PREF_ICON_BOTTOM = self.H_SCREEN - SAS_ADVISOR_EXIT_Y_OFFSET + PREF_ICON_BOTTOM_FROM_EXIT_OFFSET

	# Screen construction function
	def interfaceScreen(self):
#		BugUtil.debug("CvTechChooser: interfacescreen")
#		self.timer = BugUtil.Timer("CvTechChooser")

		if ( CyGame().isPitbossHost() ):
			return

		# Create a new screen, called TechChooser, using the file CvTechChooser.py for input
		screen = self.getScreen()

		screen.setRenderInterfaceOnly(True)
		screen.showScreen(PopupStates.POPUPSTATE_IMMEDIATE, False)
		# <!-- custom: force scoreboard redraw when opening Tech Chooser so Tech-Chooser-specific scoreboard alignment applies immediately (not only after hover-triggered redraw). (GPT-5.3-Codex) -->
		CyInterface().setDirty(InterfaceDirtyBits.Score_DIRTY_BIT, True)

		screen.hide("AddTechButton")
		screen.hide("ASPointsLabel")
		screen.hide("SelectedTechLabel")
		self.initText()

		self.iCivSelected = getAdvisorValidPerspectivePlayer(self.iCivSelected)
		if ( CyGame().isDebugMode() ):
			screen.addDropDownBoxGFC( "CivDropDown", 22, 12, 192, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.SMALL_FONT )
			screen.setActivation( "CivDropDown", ActivationTypes.ACTIVATE_MIMICPARENTFOCUS )
			for j in range(gc.getMAX_PLAYERS()):
				if (gc.getPlayer(j).isAlive()):
					screen.addPullDownString( "CivDropDown", gc.getPlayer(j).getName(), j, j, j == self.iCivSelected )
		else:
			screen.hide( "CivDropDown" )

		# advc.068: Dirty-bit check added
		if screen.isPersistent() and not CyInterface().isDirty(InterfaceDirtyBits.Tech_Screen_DIRTY_BIT):
			self.updateTechRecords(false)
			return
		# advc.068: Dirty no more
		CyInterface().setDirty(InterfaceDirtyBits.Tech_Screen_DIRTY_BIT, False)

		self.nWidgetCount = 0
		self.sWidgets = []

		# <!-- custom: preserve the selected debug perspective when reopening Tech Chooser; base code reset to the active player during full rebuild, unlike the persistent shortcut path. (GPT-5.5) -->
		self.iCivSelected = getAdvisorValidPerspectivePlayer(self.iCivSelected)
		self.aiCurrentState = []
		screen.setPersistent( True )

		# Advanced Start
		if (gc.getPlayer(self.iCivSelected).getAdvancedStartPoints() >= 0):

			self.m_bSelectedTechDirty = true

			self.X_ADD_TECH_BUTTON = 10
			self.Y_ADD_TECH_BUTTON = 731
			self.W_ADD_TECH_BUTTON = 150
			self.H_ADD_TECH_BUTTON = 30
			self.X_ADVANCED_START_TEXT = self.X_ADD_TECH_BUTTON + self.W_ADD_TECH_BUTTON + 20

			szText = self.TEXT_WB_AS_ADD_TECH
			screen.setButtonGFC( "AddTechButton", szText, "", self.X_ADD_TECH_BUTTON, self.Y_ADD_TECH_BUTTON, self.W_ADD_TECH_BUTTON, self.H_ADD_TECH_BUTTON, WidgetTypes.WIDGET_GENERAL, -1, -1, ButtonStyles.BUTTON_STYLE_STANDARD )
			screen.hide("AddTechButton")

# BUG - Tech Screen Resolution - start
		# <!-- custom: Tech Chooser now follows the same runtime-layout template as Foreign/Info screens: compute all screen-dependent bounds in interfaceScreen via shared SAS helper formulas. (GPT-5.3-Codex) -->
		self.updateRuntimeLayout(screen)
		xPanelWidth = self.W_SCREEN
		yPanelHeight = self.H_SCREEN

		screen.showWindowBackground( False )

		# <!-- custom: no longer center it; adjust dimensions like the military advisor and related reworks. (GPT-5.2-Codex (summarized)) -->
		# screen.setDimensions((screen.getXResolution() - xPanelWidth) / 2, screen.centerY(0), xPanelWidth, yPanelHeight)
		screen.setDimensions(self.X_SCREEN, self.Y_SCREEN, xPanelWidth, yPanelHeight)
# BUG - Tech Screen Resolution - end

		screen.addPanel( "TechTopPanel", u"", u"", True, False, 0, 0, xPanelWidth, 55, PanelStyles.PANEL_STYLE_TOPBAR )
		screen.addDDSGFC("TechBG", self.ART_SCREEN_BG_OPAQUE, 0, 51, xPanelWidth, yPanelHeight - 96, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		screen.addPanel( "TechBottomPanel", u"", u"", True, False, 0, yPanelHeight - 55, xPanelWidth, 55, PanelStyles.PANEL_STYLE_BOTTOMBAR )
		screen.setText( "TechChooserExit", "Background", self.TEXT_TECH_CHOOSER_EXIT, CvUtil.FONT_RIGHT_JUSTIFY, xPanelWidth - 30, yPanelHeight - 42, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_CLOSE_SCREEN, -1, -1 )
		screen.setActivation( "TechChooserExit", ActivationTypes.ACTIVATE_MIMICPARENTFOCUS )

		# Header...
		# <!-- custom: keep Tech Chooser title styling/vertical anchor aligned with migrated advisor screens (bold title + shared screen-independent Y title constant). (GPT-5.3-Codex) -->
		szText = self.TEXT_TECH_CHOOSER_TITLE_FMT
		screen.setLabel( "TechTitleHeader", "Background", szText, CvUtil.FONT_CENTER_JUSTIFY, xPanelWidth / 2, SAS_ADVISOR_TITLE_Y, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		# <!-- custom: simplify code since this was disabled anyway. (GPT-5.2-Codex (summarized)) -->
		# # Make the scrollable area for the city list...
		# # advc.004a: What cities? Anyway, no scrollable area needed.
		# if False and BugOpt.isShowGPTechPrefs():
		# 	iX = 80
		# 	iW = xPanelWidth - 80
		# else:
		# 	iX = 0
		# 	iW = xPanelWidth
		iX = 0
		iW = xPanelWidth

		self.TabPanels = ["TechList", "TechTrade"]

#		screen.addScrollPanel(self.TabPanels[0], u"", iX, 64, iW, yPanelHeight - 142, PanelStyles.PANEL_STYLE_EXTERNAL )
		screen.addScrollPanel(self.TabPanels[0], u"", iX, 56, iW, yPanelHeight - 134, PanelStyles.PANEL_STYLE_EXTERNAL )
		screen.setActivation(self.TabPanels[0], ActivationTypes.ACTIVATE_NORMAL )

		screen.addScrollPanel(self.TabPanels[1], u"", 80, 64, xPanelWidth - 80, yPanelHeight - 142, PanelStyles.PANEL_STYLE_EXTERNAL )
		screen.setActivation(self.TabPanels[1], ActivationTypes.ACTIVATE_NORMAL )

# BUG - GP Tech Prefs - start
		# advc.004a: Commented out
		#if BugOpt.isShowGPTechPrefs():
		#	screen.addPanel("GPTechPref", u"", u"", True, False, 0, 51, 80, #yPanelHeight - 95, PanelStyles.PANEL_STYLE_MAIN_WHITE )
# BUG - GP Tech Prefs - end

		# Add the Highlight
		#screen.addDDSGFC( "TechHighlight", ArtFileMgr.getInterfaceArtInfo("TECH_HIGHLIGHT").getPath(), 0, 0, self.getXStart() + 6, 12 + ( self.BOX_INCREMENT_HEIGHT * self.PIXEL_INCREMENT ), WidgetTypes.WIDGET_GENERAL, -1, -1 )
		#screen.hide( "TechHighlight" )

		self.X_SELECT_TAB = 30
		self.X_TRADE_TAB = 165
		self.Y_TABS = 730

		self.sTechSelectTab = self.getNextWidgetName("TechSelectTab")
		self.sTechTradeTab = self.getNextWidgetName("TechTradeTab")
		self.sTechTabID = self.sTechSelectTab

		# reset widget array so that the above never get deleted
		self.nWidgetCount = 0
		self.sWidgets = []

		self.ConstructTabs()

		self.ShowTab()

		return

	def ConstructTabs(self):
#		BugUtil.debug("cvTechChooser: ConstructTabs")

		screen = self.getScreen()

		self.BOX_INCREMENT_WIDTH = 27 # Used to be 33 #Should be a multiple of 3...
		self.DrawTechChooser(screen, self.TabPanels[0], True, True, True, True, True, True)

		self.BOX_INCREMENT_WIDTH = 12 # Used to be 33 #Should be a multiple of 3...
		self.DrawTechChooser(screen, self.TabPanels[1], True, False, True, False, False, True)
		self.BOX_INCREMENT_WIDTH = 27 # Used to be 33 #Should be a multiple of 3...

	def ShowTab(self):
#		BugUtil.debug("cvTechChooser: ShowTab")

		screen = self.getScreen()

		for tp in self.TabPanels:
			screen.hide(tp)

		# remove these 2 lines when we return to multi-tab screen and uncomment out the 10 below.
		screen.show(self.TabPanels[0])
		screen.setFocus(self.TabPanels[0])

#		if(self.sTechTabID == self.sTechSelectTab):
#			screen.setText(self.sTechSelectTab, "", "Tech Select", CvUtil.FONT_LEFT_JUSTIFY, self.X_SELECT_TAB, self.Y_TABS, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
#			screen.setText(self.sTechTradeTab, "", "Tech Trade - under development", CvUtil.FONT_LEFT_JUSTIFY, self.X_TRADE_TAB, self.Y_TABS, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
#			screen.show(self.TabPanels[0])
#			screen.setFocus(self.TabPanels[0])

#		elif(self.sTechTabID == self.sTechTradeTab):
#			screen.setText(self.sTechSelectTab, "", "Tech Select", CvUtil.FONT_LEFT_JUSTIFY, self.X_SELECT_TAB, self.Y_TABS, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
#			screen.setText(self.sTechTradeTab, "", "Tech Trade - under development", CvUtil.FONT_LEFT_JUSTIFY, self.X_TRADE_TAB, self.Y_TABS, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
#			screen.show(self.TabPanels[1])
#			screen.setFocus(self.TabPanels[1])

	def DrawTechChooser(self, screen, sPanel, bTechPanel, bTechName, bTechIcon, bTechDetails, bANDPreReq, bORPreReq):
#		BugUtil.debug("cvTechChooser: DrawTechChooser (%s)", sPanel)
#		self.timer.reset()
#		self.timer.start()

		# Place the tech blocks
		self.placeTechs(screen, sPanel, bTechPanel, bTechName, bTechIcon, bTechDetails)

		# Draw the arrows
		self.drawArrows(screen, sPanel, bANDPreReq, bORPreReq)

		# advc.004a: Adding this guard b/c the new code somehow can't handle calls via preGameStart (CvAppInterface) if the map is very large. Still seems to get updated properly if the player opens the Tech Advisor on turn 0.
		# <!-- custom: tried removing the turn > 0 guard to show indicators at turn 0, but it does error.
		# The base AdvCiv comment seems slightly mistaken: I reproduced the issue on a new game (Noble, Pangaea, Normal, Standard).
		# Gemini 3 Pro confirmed the cause, so we keep base AdvCiv behavior.
		# I wanted the BUG bulbing indicators noted for players; see known issue 85. (GPT-5.2-Codex (summarized)) -->
		# <!-- custom: keeping this feature disabled is safer; turn 0 bulbing indicators are not critical, so revert to base AdvCiv behavior. (GPT-5.2-Codex (summarized)) -->
		if CyGame().getElapsedGameTurns() > 0:
			self.updateTechPrefs()

		screen.moveToFront( "CivDropDown" )

		screen.moveToFront( "AddTechButton" )

#		self.timer.logSpan("total")

	def placeTechs(self, screen, sPanel, bTechPanel, bTechName, bTechIcon, bTechDetails):
#		BugUtil.debug("cvTechChooser: placeTechs")

		iMaxX = 0
		iMaxY = 0

		if sPanel == self.TabPanels[0]:
			sPanelWidget = ""
		else:
			sPanelWidget = sPanel

		# If we are the Pitboss, we don't want to put up an interface at all
		if ( CyGame().isPitbossHost() ):
			return

		# Go through all the techs
		for i in range(gc.getNumTechInfos()):

			# Create and place a tech in its proper location
			iX = self.iX_LEFT_START + ( (gc.getTechInfo(i).getGridX() - 1) * ( ( self.BOX_INCREMENT_X_SPACING + self.BOX_INCREMENT_WIDTH ) * self.PIXEL_INCREMENT ) )
			iY = ( gc.getTechInfo(i).getGridY() - 1 ) * ( self.BOX_INCREMENT_Y_SPACING * self.PIXEL_INCREMENT ) + 5
			szTechRecord = sPanelWidget + "TechRecord" + str(i)

			if ( iMaxX < iX + self.getXStart() ):
				iMaxX = iX + self.getXStart()
			if ( iMaxY < iY + ( self.BOX_INCREMENT_HEIGHT * self.PIXEL_INCREMENT ) ):
				iMaxY = iY + ( self.BOX_INCREMENT_HEIGHT * self.PIXEL_INCREMENT )

# BUG - Tech Era Colors - start
			szTechRecordShadow = sPanelWidget + "TechRecordShadow" + str(i)
			iShadowOffset = 9
			screen.attachPanelAt( sPanel, szTechRecordShadow, u"", u"", True, False, PanelStyles.PANEL_STYLE_TECH, iX - 6 + iShadowOffset, iY - 6 + iShadowOffset, self.getXStart() + 6, 12 + ( self.BOX_INCREMENT_HEIGHT * self.PIXEL_INCREMENT ), WidgetTypes.WIDGET_TECH_CHOOSER_ERA, gc.getTechInfo(i).getEra(), -1 )
			self.setTechPanelShadowColor(screen, szTechRecordShadow, gc.getTechInfo(i).getEra())
			screen.hide( szTechRecordShadow )
# BUG - Tech Era Colors - end

			screen.attachPanelAt( sPanel, szTechRecord, u"", u"", True, False, PanelStyles.PANEL_STYLE_TECH, iX - 6, iY - 6, self.getXStart() + 6, 12 + ( self.BOX_INCREMENT_HEIGHT * self.PIXEL_INCREMENT ), WidgetTypes.WIDGET_TECH_TREE, i, -1 )
			screen.setActivation( szTechRecord, ActivationTypes.ACTIVATE_MIMICPARENTFOCUS)
			screen.hide( szTechRecord )

			iTechX = iX
			iTechY = iY

			#reset so that it offsets from the tech record's panel
			iX = 6
			iY = 6

			if ( gc.getTeam(gc.getPlayer(self.iCivSelected).getTeam()).isHasTech(i) ):
				screen.setPanelColor(szTechRecord, 85, 150, 87)
				self.aiCurrentState.append(CIV_HAS_TECH)
			elif ( gc.getPlayer(self.iCivSelected).getCurrentResearch() == i ):
				screen.setPanelColor(szTechRecord, 104, 158, 165)
				self.aiCurrentState.append(CIV_IS_RESEARCHING)
			elif ( gc.getPlayer(self.iCivSelected).isResearchingTech(i) ):
				screen.setPanelColor(szTechRecord, 104, 158, 165)
				self.aiCurrentState.append(CIV_IS_RESEARCHING)
			elif ( gc.getPlayer(self.iCivSelected).canEverResearch(i) ):
				screen.setPanelColor(szTechRecord, 100, 104, 160)
				self.aiCurrentState.append(CIV_NO_RESEARCH)
			else:
				screen.setPanelColor(szTechRecord, 206, 65, 69)
				self.aiCurrentState.append(CIV_TECH_AVAILABLE)

			if bTechName:
				szTechID = sPanelWidget + "TechID" + str(i)
				szTechString = sasFontTagLabel # advc.002b: was 1
				if ( gc.getPlayer(self.iCivSelected).isResearchingTech(i) ):
					szTechString = szTechString + str(gc.getPlayer(self.iCivSelected).getQueuePosition(i)) + ". "
				szTechString += gc.getTechInfo(i).getDescription()
				# <!-- custom: include turns-left on first Tech Advisor draw too; before this, placeTechs only showed queue position and missed "(N)" until a later refresh path (e.g. clicking another tech) rebuilt labels via updateTechRecords, so load-from-save could hide the timer on the initially selected tech. See KI#114. (GPT-5.3-Codex) -->
				if ( gc.getPlayer(self.iCivSelected).isResearchingTech(i) ):
					iTurnsLeft = gc.getPlayer(self.iCivSelected).getResearchTurnsLeft(i, ( gc.getPlayer(self.iCivSelected).getCurrentResearch() == i ))
					if iTurnsLeft > 0:
						# <!-- custom: minor tweak only: keep "(N)" append as one formatted string for readability; no behavior change relative to the timer/clipping fix. (GPT-5.3-Codex) -->
						szTechString += " (%d)" % iTurnsLeft
				szTechString = szTechString + SAS_FONT_TAG_CLOSE
				# <!-- custom: anchor tech labels to the scroll panel (same as updateTechRecords) instead of the tech panel child so first-draw and refresh rendering paths stay consistent. See KI#114. (GPT-5.3-Codex) -->
				screen.setTextAt( szTechID, sPanel, szTechString, CvUtil.FONT_LEFT_JUSTIFY, iTechX + 6 + X_INCREMENT, iTechY + 6, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_TECH_TREE, i, -1 )
				screen.setActivation( szTechID, ActivationTypes.ACTIVATE_MIMICPARENTFOCUS )

			if bTechIcon:
				szTechButtonID = sPanelWidget + "TechButtonID" + str(i)
				szButton = gc.getTechInfo(i).getButton()
				# <advc.096> Show religion tech button - consistent with CvMainInterface.updateResearchButtons and CvDLLButtonPopup::launchChooseTechPopup.
				# Don't do this after all, I think. Difficult to make out. See also CvGame::setReligionSlotTaken.
				#iReligions = 0
				#for j in range(gc.getNumReligionInfos()):
				#	if gc.getReligionInfo(j).getTechPrereq() == i and not gc.getGame().isReligionSlotTaken(j):
				#		iReligions += 1
				#		if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_PICK_RELIGION):
				#			szTechReligionButton = gc.getReligionInfo(j).getGenericTechButton()
				#		else:
				#			szTechReligionButton = gc.getReligionInfo(j).getTechButton()
				#if iReligions == 1:
				#	szButton = szTechReligionButton
				# </advc.096>
				screen.addDDSGFCAt( szTechButtonID, szTechRecord, szButton, iX + 6, iY + 6, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_TECH_TREE, i, -1, False )

			if bTechDetails:
				self.addIconsToTechPanel(screen, i, X_START, iX, iY, szTechRecord)

			if bTechPanel:
				if BugOpt.isShowTechEra():
					screen.show( szTechRecordShadow )
				else:
					screen.hide( szTechRecordShadow )
				screen.show( szTechRecord )
			else:
				screen.hide( szTechRecordShadow )
				screen.hide( szTechRecord )

		screen.setViewMin( sPanel, iMaxX + 20, iMaxY + 20 )

		return

	def addIconsToTechPanel(self, screen, i, fX, iX, iY, szTechRecord):
#		BugUtil.debug("cvTechChooser: addIconsToTechPanel")

		j = 0
		k = 0

		# Unlockable units...
		for j in range( gc.getNumUnitClassInfos() ):
			eLoopUnit = gc.getCivilizationInfo(gc.getGame().getActiveCivilizationType()).getCivilizationUnits(j)
			if (eLoopUnit != -1):
				if (gc.getUnitInfo(eLoopUnit).getPrereqAndTech() == i):
					szUnitButton = self.getNextWidgetName("Unit")
					screen.addDDSGFCAt( szUnitButton, szTechRecord, gc.getPlayer(gc.getGame().getActivePlayer()).getUnitButton(eLoopUnit), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, eLoopUnit, 1, True )
					fX += X_INCREMENT

		j = 0
		k = 0

		# Unlockable Buildings...
		for j in range(gc.getNumBuildingClassInfos()):
			bTechFound = 0
			eLoopBuilding = gc.getCivilizationInfo(gc.getGame().getActiveCivilizationType()).getCivilizationBuildings(j)

			if (eLoopBuilding != -1):
				if (gc.getBuildingInfo(eLoopBuilding).getPrereqAndTech() == i):
					szBuildingButton = self.getNextWidgetName("Building")
					screen.addDDSGFCAt( szBuildingButton, szTechRecord, gc.getBuildingInfo(eLoopBuilding).getButton(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, eLoopBuilding, 1, True )
					fX += X_INCREMENT

		j = 0
		k = 0

		# Obsolete Buildings...
		for j in range(gc.getNumBuildingClassInfos()):
			eLoopBuilding = gc.getCivilizationInfo(gc.getPlayer(self.iCivSelected).getCivilizationType()).getCivilizationBuildings(j)

			if (eLoopBuilding != -1):
				if (gc.getBuildingInfo(eLoopBuilding).getObsoleteTech() == i):
					# Add obsolete picture here...
					szObsoleteButton = self.getNextWidgetName("Obsolete")
					szObsoleteX = self.getNextWidgetName("ObsoleteX")
					screen.addDDSGFCAt( szObsoleteButton, szTechRecord, gc.getBuildingInfo(eLoopBuilding).getButton(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_OBSOLETE, eLoopBuilding, -1, False )
					screen.addDDSGFCAt( szObsoleteX, szTechRecord, self.ART_RED_X, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_OBSOLETE, eLoopBuilding, -1, False )
					fX += X_INCREMENT

		j = 0
		k = 0

		# Obsolete Bonuses...
		for j in range(gc.getNumBonusInfos()):
			if (gc.getBonusInfo(j).getTechObsolete() == i):
				# Add obsolete picture here...
				szObsoleteButton = self.getNextWidgetName("ObsoleteBonus")
				szObsoleteX = self.getNextWidgetName("ObsoleteXBonus")
				screen.addDDSGFCAt( szObsoleteButton, szTechRecord, gc.getBonusInfo(j).getButton(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_OBSOLETE_BONUS, j, -1, False )
				screen.addDDSGFCAt( szObsoleteX, szTechRecord, self.ART_RED_X, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_OBSOLETE_BONUS, j, -1, False )
				fX += X_INCREMENT

		j = 0
		k = 0

		# Obsolete Monasteries...
		for j in range (gc.getNumSpecialBuildingInfos()):
			if (gc.getSpecialBuildingInfo(j).getObsoleteTech() == i):
					# Add obsolete picture here...
					szObsoleteSpecialButton = self.getNextWidgetName("ObsoleteSpecial")
					szObsoleteSpecialX = self.getNextWidgetName("ObsoleteSpecialX")
					screen.addDDSGFCAt( szObsoleteSpecialButton, szTechRecord, gc.getSpecialBuildingInfo(j).getButton(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_OBSOLETE_SPECIAL, j, -1, False )
					screen.addDDSGFCAt( szObsoleteSpecialX, szTechRecord, self.ART_RED_X, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_OBSOLETE_SPECIAL, j, -1, False )
					fX += X_INCREMENT

		j = 0
		k = 0

		# Route movement change
		for j in range(gc.getNumRouteInfos()):
			if ( gc.getRouteInfo(j).getTechMovementChange(i) != 0 ):
				szMoveButton = self.getNextWidgetName("Move")
				screen.addDDSGFCAt( szMoveButton, szTechRecord, self.ART_TECH_MOVE_BONUS, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_MOVE_BONUS, i, -1, False )
				fX += X_INCREMENT

		j = 0
		k = 0

		# Promotion Info
		for j in range( gc.getNumPromotionInfos() ):
			if ( gc.getPromotionInfo(j).getTechPrereq() == i ):
				szPromotionButton = self.getNextWidgetName("Promotion")
				screen.addDDSGFCAt( szPromotionButton, szTechRecord, gc.getPromotionInfo(j).getButton(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, j, -1, False )
				fX += X_INCREMENT

		j = 0
		k = 0

		# Free unit
		if ( gc.getTechInfo(i).getFirstFreeUnitClass() != UnitClassTypes.NO_UNITCLASS ):
			szFreeUnitButton = self.getNextWidgetName("FreeUnit")
			eLoopUnit = gc.getCivilizationInfo(gc.getGame().getActiveCivilizationType()).getCivilizationUnits(gc.getTechInfo(i).getFirstFreeUnitClass())
			if (eLoopUnit != -1):
# BUG - 3.19 No Espionage - start
				# CvUnitInfo.getEspionagePoints() was added in 319
				if (GameUtil.getVersion() < 319 or gc.getUnitInfo(eLoopUnit).getEspionagePoints() == 0 or not gc.getGame().isOption(GameOptionTypes.GAMEOPTION_NO_ESPIONAGE)):				
# BUG - 3.19 No Espionage - end
					screen.addDDSGFCAt( szFreeUnitButton, szTechRecord, gc.getPlayer(gc.getGame().getActivePlayer()).getUnitButton(eLoopUnit), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_FREE_UNIT, eLoopUnit, i, False )
					fX += X_INCREMENT

		j = 0
		k = 0

		# Feature production modifier
		if ( gc.getTechInfo(i).getFeatureProductionModifier() != 0 ):
			szFeatureProductionButton = self.getNextWidgetName("FeatureProduction")
			screen.addDDSGFCAt( szFeatureProductionButton, szTechRecord, self.ART_TECH_FEATURE_PRODUCTION, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_FEATURE_PRODUCTION, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Worker speed
		if ( gc.getTechInfo(i).getWorkerSpeedModifier() != 0 ):
			szWorkerModifierButton = self.getNextWidgetName("Worker")
			screen.addDDSGFCAt( szWorkerModifierButton, szTechRecord, self.ART_TECH_WORKER_SPEED, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_WORKER_RATE, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Trade Routes per City change
		if ( gc.getTechInfo(i).getTradeRoutes() != 0 ):
			szTradeRouteButton = self.getNextWidgetName("TradeRoutes")
			screen.addDDSGFCAt( szTradeRouteButton, szTechRecord, self.ART_TECH_TRADE_ROUTES, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_TRADE_ROUTES, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Health Rate bonus from this tech...
		if ( gc.getTechInfo(i).getHealth() != 0 ):
			szHealthRateButton = self.getNextWidgetName("HealthRate")
			screen.addDDSGFCAt( szHealthRateButton, szTechRecord, self.ART_TECH_HEALTH, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_HEALTH_RATE, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Happiness Rate bonus from this tech...
		if ( gc.getTechInfo(i).getHappiness() != 0 ):
			szHappinessRateButton = self.getNextWidgetName("HappinessRate")
			screen.addDDSGFCAt( szHappinessRateButton, szTechRecord, self.ART_TECH_HAPPINESS, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_HAPPINESS_RATE, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Adjustments  (advc.120g: Moved up so that the icon appears before the tech trading icon)
		for j in range( CommerceTypes.NUM_COMMERCE_TYPES ):
			# advc.120g: The second condition said (I paraphrase) "not team.isCommerceFlexible". This hides the icon once the tech is dicovered, which I don't like. If there were multiple techs unlocking the same slider it would make more sense. Actually, buildings can - in theory - unlock a slider for a player. So I'm going to check if the player already has the slider, but the team doesn't (meaning that the player must have it through a building).
			if (gc.getTechInfo(i).isCommerceFlexible(j) and (not gc.getPlayer(self.iCivSelected).isCommerceFlexible(j) or gc.getTeam(gc.getPlayer(self.iCivSelected).getTeam()).isCommerceFlexible(j))):
				szAdjustButton = self.getNextWidgetName("AdjustButton")
				if ( j == CommerceTypes.COMMERCE_CULTURE ):
					szFileName = self.ART_TECH_CULTURE
				elif ( j == CommerceTypes.COMMERCE_ESPIONAGE ):
					szFileName = self.ART_TECH_ESPIONAGE
				else:
					szFileName = self.ART_GENERAL_QUESTIONMARK
				screen.addDDSGFCAt( szAdjustButton, szTechRecord, szFileName, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_ADJUST, i, j, False )
				fX += X_INCREMENT

		j = 0
		k = 0

		# Free Techs
		if ( gc.getTechInfo(i).getFirstFreeTechs() > 0 ):
			szFreeTechButton = self.getNextWidgetName("FreeTech")
			screen.addDDSGFCAt( szFreeTechButton, szTechRecord, self.ART_TECH_FREETECH, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_FREE_TECH, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0
		# <advc.500c>
		# No Fear for Safety
		if ( gc.getTechInfo(i).isNoFearForSafety() ):
			screen.addDDSGFCAt("FearForSafety" + str(i), szTechRecord, gc.getMissionInfo(MissionTypes.MISSION_FORTIFY).getButton(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_NO_FEAR_FOR_SAFETY, i, -1, False )
			fX += X_INCREMENT
		# </advc.550c>
		# Line of Sight bonus...
		if ( gc.getTechInfo(i).isExtraWaterSeeFrom() ):
			szLOSButton = self.getNextWidgetName("LOS")
			screen.addDDSGFCAt( szLOSButton, szTechRecord, self.ART_TECH_LOS, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_LOS_BONUS, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Map Center Bonus...
		if ( gc.getTechInfo(i).isMapCentering() ):
			szMapCenterButton = self.getNextWidgetName("MapCenter")
			screen.addDDSGFCAt( szMapCenterButton, szTechRecord, self.ART_TECH_MAPCENTER, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_MAP_CENTER, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Map Reveal...
		if ( gc.getTechInfo(i).isMapVisible() ):
			szMapRevealButton = self.getNextWidgetName("MapReveal")
			screen.addDDSGFCAt( szMapRevealButton, szTechRecord, self.ART_TECH_MAPREVEAL, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_MAP_REVEAL, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Map Trading
		if (gc.getTechInfo(i).isMapTrading()):
			szMapTradeButton = self.getNextWidgetName("MapTrade")
			screen.addDDSGFCAt( szMapTradeButton, szTechRecord, self.ART_TECH_MAPTRADING, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_MAP_TRADE, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Tech Trading
		if ( gc.getTechInfo(i).isTechTrading() ):
			szTechTradeButton = self.getNextWidgetName("TechTrade")
			screen.addDDSGFCAt( szTechTradeButton, szTechRecord, self.ART_TECH_TECHTRADING, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_TECH_TRADE, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Gold Trading
		if ( gc.getTechInfo(i).isGoldTrading() ):
			szGoldTradeButton = self.getNextWidgetName("GoldTrade")
			screen.addDDSGFCAt( szGoldTradeButton, szTechRecord, self.ART_TECH_GOLDTRADING, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_GOLD_TRADE, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Open Borders
		if ( gc.getTechInfo(i).isOpenBordersTrading() ):
			szOpenBordersButton = self.getNextWidgetName("OpenBorders")
			screen.addDDSGFCAt( szOpenBordersButton, szTechRecord, self.ART_TECH_OPENBORDERS, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_OPEN_BORDERS, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Defensive Pact
		if ( gc.getTechInfo(i).isDefensivePactTrading() ):
			szDefensivePactButton = self.getNextWidgetName("DefensivePact")
			screen.addDDSGFCAt( szDefensivePactButton, szTechRecord, self.ART_TECH_DEFENSIVEPACT, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_DEFENSIVE_PACT, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Permanent Alliance
		if ( gc.getTechInfo(i).isPermanentAllianceTrading() ):
			szPermanentAllianceButton = self.getNextWidgetName("PermanentAlliance")
			screen.addDDSGFCAt( szPermanentAllianceButton, szTechRecord, self.ART_TECH_PERMALLIANCE, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_PERMANENT_ALLIANCE, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Vassal States
		if ( gc.getTechInfo(i).isVassalStateTrading() ):
			szVassalStateButton = self.getNextWidgetName("VassalState")
			screen.addDDSGFCAt( szVassalStateButton, szTechRecord, self.ART_TECH_VASSAL, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_VASSAL_STATE, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Bridge Building
		if ( gc.getTechInfo(i).isBridgeBuilding() ):
			szBuildBridgeButton = self.getNextWidgetName("BuildBridge")
			screen.addDDSGFCAt( szBuildBridgeButton, szTechRecord, self.ART_TECH_BRIDGEBUILDING, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_BUILD_BRIDGE, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Irrigation unlocked...
		if ( gc.getTechInfo(i).isIrrigation() ):
			szIrrigationButton = self.getNextWidgetName("Irrigation")
			screen.addDDSGFCAt( szIrrigationButton, szTechRecord, self.ART_TECH_IRRIGATION, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_IRRIGATION, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Ignore Irrigation unlocked...
		if ( gc.getTechInfo(i).isIgnoreIrrigation() ):
			szIgnoreIrrigationButton = self.getNextWidgetName("IgnoreIrrigation")
			screen.addDDSGFCAt( szIgnoreIrrigationButton, szTechRecord, self.ART_TECH_NOIRRIGATION, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_IGNORE_IRRIGATION, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Coastal Work unlocked...
		if ( gc.getTechInfo(i).isWaterWork() ):
			szWaterWorkButton = self.getNextWidgetName("WaterWork")
			screen.addDDSGFCAt( szWaterWorkButton, szTechRecord, self.ART_TECH_WATERWORK, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_WATER_WORK, i, -1, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Improvements
		for j in range(gc.getNumBuildInfos()):
			bTechFound = 0

			if (gc.getBuildInfo(j).getTechPrereq() == -1):
				bTechFound = 0
				for k in range(gc.getNumFeatureInfos()):
					if (gc.getBuildInfo(j).getFeatureTech(k) == i):
						bTechFound = 1
			else:
				if (gc.getBuildInfo(j).getTechPrereq() == i):
					bTechFound = 1

			if (bTechFound == 1):
				szImprovementButton = self.getNextWidgetName("Improvement")
				iImprovement = gc.getBuildInfo(j).getImprovement()
				# <!-- custom: use base AdvCiv behavior here (improvement icon + WIDGET_HELP_IMPROVEMENT), and only fallback to the Builds pedia redirect for builds without an improvement (routes/feature removal).
				# This split is intentionally more useful for players: improvement-style behavior where expected, and direct Builds-page routing when the build itself is the key info. See KI#113. (GPT-5.3-Codex) -->
				if (iImprovement != -1):
					screen.addDDSGFCAt( szImprovementButton, szTechRecord, gc.getImprovementInfo(iImprovement).getButton(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_IMPROVEMENT, i, j, False )
				else:
					screen.addDDSGFCAt( szImprovementButton, szTechRecord, gc.getBuildInfo(j).getButton(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_IMPROVEMENT, i, j, False )
				fX += X_INCREMENT

		j = 0
		k = 0

		# Domain Extra Moves
		for j in range( DomainTypes.NUM_DOMAIN_TYPES ):
			if (gc.getTechInfo(i).getDomainExtraMoves(j) != 0):
				szDomainExtraMovesButton = self.getNextWidgetName("DomainExtraMoves")
				screen.addDDSGFCAt( szDomainExtraMovesButton, szTechRecord, self.ART_TECH_WATERMOVES, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_DOMAIN_EXTRA_MOVES, i, j, False )
				fX += X_INCREMENT

		j = 0
		k = 0

		# K-Mod. Commerce modifiers
		for j in range(CommerceTypes.NUM_COMMERCE_TYPES):
			if (gc.getTechInfo(i).getCommerceModifier(j) > 0):
				szCommerceModifierButton = self.getNextWidgetName("CommerceModifierButton")
				screen.addDDSGFCAt( szCommerceModifierButton, szTechRecord, gc.getCommerceInfo(j).getButton(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_GLOBAL_COMMERCE_MODIFIER, i, j, False )
				fX += X_INCREMENT

		j = 0
		k = 0

		# K-Mod. Extra specialist commerce
		for j in range(CommerceTypes.NUM_COMMERCE_TYPES):
			if (gc.getTechInfo(i).getSpecialistExtraCommerce(j) > 0):
				# <advc.002d>
				bestFitSpecialist = SpecialistTypes.NO_SPECIALIST
				bestFitValue = -1
				for k in range(gc.getNumSpecialistInfos()):
					sp = gc.getSpecialistInfo(k)
					commChange = sp.getCommerceChange(j)
					if commChange > 0 and commChange > bestFitValue:
						bestFitValue = commChange
						bestFitSpecialist = k
				if (bestFitSpecialist != SpecialistTypes.NO_SPECIALIST):
					# Below: using bestFitSpecialist instead of gc.getDefineINT("DEFAULT_SPECIALIST")</advc.002d>
					szSpecialistCommerceButtonButton = self.getNextWidgetName("SpecialistCommerceButton")
					screen.addDDSGFCAt( szSpecialistCommerceButtonButton, szTechRecord, gc.getSpecialistInfo(bestFitSpecialist).getButton(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_EXTRA_SPECIALIST_COMMERCE, i, j, False )
					fX += X_INCREMENT
				break

		j = 0
		k = 0
		# K-Mod end.

		# Terrain opens up as a trade route
		if self.iDEEP_WATER_TERRAIN is None:
			self.iDEEP_WATER_TERRAIN = gc.getDefineINT("DEEP_WATER_TERRAIN")
		for j in range( gc.getNumTerrainInfos() ):
			if gc.getTechInfo(i).isTerrainTrade(j):
				# advc.124: This hides the icon in the tech tree once the
				# the player has the tech. Confusing.
				#and not (gc.getTeam(gc.getPlayer(self.iCivSelected).getTeam()).isTerrainTrade(j)):
				szTerrainTradeButton = self.getNextWidgetName("TerrainTradeButton")
				# <advc.002d> Use the ocean trade icon for Ocean. For all other terrains, keep using the coastal trade icon.
				szArtInfoType = "INTERFACE_TECH_WATERTRADE"
				if j == self.iDEEP_WATER_TERRAIN:
					szArtInfoType = "INTERFACE_TECH_DEEPWATERTRADE"
				# </advc.002d>
				screen.addDDSGFCAt( szTerrainTradeButton, szTechRecord, ArtFileMgr.getInterfaceArtInfo(szArtInfoType).getPath(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_TERRAIN_TRADE, i, j, False )
				fX += X_INCREMENT

		j = gc.getNumTerrainInfos()	
		if gc.getTechInfo(i).isRiverTrade():
			# advc.124: Ability not used currently, but still, for consistency:
			#and not (gc.getTeam(gc.getPlayer(self.iCivSelected).getTeam()).isRiverTrade()):
			szTerrainTradeButton = self.getNextWidgetName("TerrainTradeButton")
			screen.addDDSGFCAt( szTerrainTradeButton, szTechRecord, self.ART_TECH_RIVERTRADE, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_TERRAIN_TRADE, i, j, False )
			fX += X_INCREMENT

		j = 0
		k = 0

		# Special buildings like monasteries...
		for j in range( gc.getNumSpecialBuildingInfos() ):
			if (gc.getSpecialBuildingInfo(j).getTechPrereq() == i):
				szSpecialBuilding = self.getNextWidgetName("SpecialBuildingButton")
				screen.addDDSGFCAt( szSpecialBuilding, szTechRecord, gc.getSpecialBuildingInfo(j).getButton(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_SPECIAL_BUILDING, i, j, False )
				fX += X_INCREMENT

		j = 0
		k = 0

		# Yield change
		for j in range( gc.getNumImprovementInfos() ):
			bFound = False
			for k in range( YieldTypes.NUM_YIELD_TYPES ):
				if (gc.getImprovementInfo(j).getTechYieldChanges(i, k)):
					if (not bFound):
						szYieldChange = self.getNextWidgetName("YieldChangeButton")
						screen.addDDSGFCAt( szYieldChange, szTechRecord, gc.getImprovementInfo(j).getButton(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_YIELD_CHANGE, i, j, False )
						fX += X_INCREMENT
						bFound = True

		j = 0
		k = 0

		# Bonuses revealed
		for j in range( gc.getNumBonusInfos() ):
			if (gc.getBonusInfo(j).getTechReveal() == i):
				szBonusReveal = self.getNextWidgetName("BonusRevealButton")
				screen.addDDSGFCAt( szBonusReveal, szTechRecord, gc.getBonusInfo(j).getButton(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_BONUS_REVEAL, i, j, False )
				fX += X_INCREMENT

		j = 0
		k = 0

		# Civic options
		for j in range( gc.getNumCivicInfos() ):
			if (gc.getCivicInfo(j).getTechPrereq() == i):
				szCivicReveal = self.getNextWidgetName("CivicRevealButton")
				screen.addDDSGFCAt( szCivicReveal, szTechRecord, gc.getCivicInfo(j).getButton(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_CIVIC_REVEAL, i, j, False )
				fX += X_INCREMENT

		j = 0
		k = 0

		# Projects possible
		for j in range( gc.getNumProjectInfos() ):
			if (gc.getProjectInfo(j).getTechPrereq() == i):
				szProjectInfo = self.getNextWidgetName("ProjectInfoButton")
				screen.addDDSGFCAt( szProjectInfo, szTechRecord, gc.getProjectInfo(j).getButton(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROJECT, j, 1, False )
				fX += X_INCREMENT

		j = 0
		k = 0

		# Processes possible
		for j in range( gc.getNumProcessInfos() ):
			if (gc.getProcessInfo(j).getTechPrereq() == i):
				szProcessInfo = self.getNextWidgetName("ProcessInfoButton")
				screen.addDDSGFCAt( szProcessInfo, szTechRecord, gc.getProcessInfo(j).getButton(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_PROCESS_INFO, i, j, False )
				fX += X_INCREMENT

		j = 0
		k = 0

		# Religions unlocked
		for j in range( gc.getNumReligionInfos() ):
			if ( gc.getReligionInfo(j).getTechPrereq() == i ):
				szFoundReligion = self.getNextWidgetName("FoundReligionButton")
				if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_PICK_RELIGION):
					szButton = self.ART_POPUPBUTTON_RELIGION
				else:
					szButton = gc.getReligionInfo(j).getButton()
				screen.addDDSGFCAt( szFoundReligion, szTechRecord, szButton, iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_FOUND_RELIGION, i, j, False )
				fX += X_INCREMENT

		for j in range( gc.getNumCorporationInfos() ):
			if ( gc.getCorporationInfo(j).getTechPrereq() == i ):
				szFoundCorporation = self.getNextWidgetName("FoundCorporationButton")
				screen.addDDSGFCAt( szFoundCorporation, szTechRecord, gc.getCorporationInfo(j).getButton(), iX + fX, iY + Y_ROW, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_FOUND_CORPORATION, i, j, False )
				fX += X_INCREMENT

	# Will update the tech records based on color, researching, researched, queued, etc.
	def updateTechRecords (self, bForce):
#		BugUtil.debug("cvTechChooser: updateTechRecords")

		# If we are the Pitboss, we don't want to put up an interface at all
		if ( CyGame().isPitbossHost() ):
			return

		if (self.sTechTabID == self.sTechSelectTab):
			bTechName = True
			sPanel = self.TabPanels[0]
			self.BOX_INCREMENT_WIDTH = 27 # Used to be 33 #Should be a multiple of 3...
		else:
			bTechName = False
			sPanel = self.TabPanels[1]
			self.BOX_INCREMENT_WIDTH = 12 # Used to be 33 #Should be a multiple of 3...

		# Get the screen
		screen = self.getScreen()

		abChanged = []
		bAnyChanged = 0

		# Go through all the techs
		for i in range(gc.getNumTechInfos()):

			abChanged.append(0)

			if ( gc.getTeam(gc.getPlayer(self.iCivSelected).getTeam()).isHasTech(i) ):
				if ( self.aiCurrentState[i] != CIV_HAS_TECH ):
					self.aiCurrentState[i] = CIV_HAS_TECH
					abChanged[i] = 1
					bAnyChanged = 1
			elif ( gc.getPlayer(self.iCivSelected).getCurrentResearch() == i ):
				if ( self.aiCurrentState[i] != CIV_IS_RESEARCHING ):
					self.aiCurrentState[i] = CIV_IS_RESEARCHING
					abChanged[i] = 1
					bAnyChanged = 1
			elif ( gc.getPlayer(self.iCivSelected).isResearchingTech(i) ):
				if ( self.aiCurrentState[i] != CIV_IS_RESEARCHING ):
					self.aiCurrentState[i] = CIV_IS_RESEARCHING
					abChanged[i] = 1
					bAnyChanged = 1
			elif ( gc.getPlayer(self.iCivSelected).canEverResearch(i) ):
				if ( self.aiCurrentState[i] != CIV_NO_RESEARCH ):
					self.aiCurrentState[i] = CIV_NO_RESEARCH
					abChanged[i] = 1
					bAnyChanged = 1
			else:
				if ( self.aiCurrentState[i] != CIV_TECH_AVAILABLE ):
					self.aiCurrentState[i] = CIV_TECH_AVAILABLE
					abChanged[i] = 1
					bAnyChanged = 1

		for i in range(gc.getNumTechInfos()):
			if (abChanged[i] or bForce or (bAnyChanged and gc.getPlayer(self.iCivSelected).isResearchingTech(i))):
				# Create and place a tech in its proper location
				szTechRecord = "TechRecord" + str(i)
				szTechID = "TechID" + str(i)
				szTechString = sasFontTagLabel # advc.002b: was 1

				if ( gc.getPlayer(self.iCivSelected).isResearchingTech(i) ):
					szTechString = szTechString + unicode(gc.getPlayer(self.iCivSelected).getQueuePosition(i)) + ". "

				iX = self.iX_LEFT_START + ( (gc.getTechInfo(i).getGridX() - 1) * ( ( self.BOX_INCREMENT_X_SPACING + self.BOX_INCREMENT_WIDTH ) * self.PIXEL_INCREMENT ) )
				iY = ( gc.getTechInfo(i).getGridY() - 1 ) * ( self.BOX_INCREMENT_Y_SPACING * self.PIXEL_INCREMENT ) + 5

				if bTechName:
					szTechString += gc.getTechInfo(i).getDescription()
					if ( gc.getPlayer(self.iCivSelected).isResearchingTech(i) ):
						iTurnsLeft = gc.getPlayer(self.iCivSelected).getResearchTurnsLeft(i, ( gc.getPlayer(self.iCivSelected).getCurrentResearch() == i ))
						if iTurnsLeft > 0: # advc.004x: Don't show turns left during anarchy
							szTechString += " (%d)" % iTurnsLeft
					szTechString = szTechString + SAS_FONT_TAG_CLOSE
					screen.setTextAt( szTechID, sPanel, szTechString, CvUtil.FONT_LEFT_JUSTIFY, iX + 6 + X_INCREMENT, iY + 6, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_TECH_TREE, i, -1 )
					screen.setActivation( szTechID, ActivationTypes.ACTIVATE_MIMICPARENTFOCUS )

				if ( gc.getTeam(gc.getPlayer(self.iCivSelected).getTeam()).isHasTech(i) ):
					screen.setPanelColor(szTechRecord, 85, 150, 87)
				elif ( gc.getPlayer(self.iCivSelected).getCurrentResearch() == i ):
					screen.setPanelColor(szTechRecord, 104, 158, 165)
				elif ( gc.getPlayer(self.iCivSelected).isResearchingTech(i) ):
					screen.setPanelColor(szTechRecord, 104, 158, 165)
				elif ( gc.getPlayer(self.iCivSelected).canEverResearch(i) ):
					screen.setPanelColor(szTechRecord, 100, 104, 160)
				else:
					screen.setPanelColor(szTechRecord, 206, 65, 69)

# BUG - GP Tech Prefs - start
		self.updateTechPrefs()
# BUG - GP Tech Prefs - end

# BUG - Tech Era Colors - start
	def setTechPanelShadowColor(self, screen, sPanel, iEra):
		szEra = gc.getEraInfo(iEra).getType()
		iColor = ClockOpt.getEraColor(szEra)
		if iColor != -1:
			color = gc.getColorInfo(iColor)
			if color:
				rgb = color.getColor() # NiColorA object
				if rgb:
					screen.setPanelColor(sPanel, int(100 * rgb.r), int(100 * rgb.g), int(100 * rgb.b))
# BUG - Tech Era Colors - end

	# Will draw the arrows
	def drawArrows(self, screen, sPanel, bANDPreReq, bORPreReq):
#		BugUtil.debug("cvTechChooser: drawArrows")

		iLoop = 0

		ARROW_X = self.ARROW_X
		ARROW_Y = self.ARROW_Y
		ARROW_MXMY = self.ARROW_MXMY
		ARROW_XY = self.ARROW_XY
		ARROW_MXY = self.ARROW_MXY
		ARROW_XMY = self.ARROW_XMY
		ARROW_HEAD = self.ARROW_HEAD

		for i in range(gc.getNumTechInfos()):
			bFirst = 1
			fX = (self.BOX_INCREMENT_WIDTH * self.PIXEL_INCREMENT) - 8

			if bANDPreReq:
				for j in range( gc.getNUM_AND_TECH_PREREQS() ):
					eTech = gc.getTechInfo(i).getPrereqAndTechs(j)
					if ( eTech > -1 ):
						fX = fX - X_INCREMENT
						iX = self.iX_LEFT_START + ( (gc.getTechInfo(i).getGridX() - 1) * ( ( self.BOX_INCREMENT_X_SPACING + self.BOX_INCREMENT_WIDTH ) * self.PIXEL_INCREMENT ) )
						iY = ( gc.getTechInfo(i).getGridY() - 1 ) * ( self.BOX_INCREMENT_Y_SPACING * self.PIXEL_INCREMENT ) + 5

						szTechPrereqID = "TechPrereqID" + str((i * 1000) + j)
						screen.addDDSGFCAt( szTechPrereqID, sPanel, gc.getTechInfo(eTech).getButton(), iX + fX, iY + 6, TEXTURE_SIZE, TEXTURE_SIZE, WidgetTypes.WIDGET_HELP_TECH_PREPREQ, eTech, -1, False )

						#szTechPrereqBorderID = "TechPrereqBorderID" + str((i * 1000) + j)
						#screen.addDDSGFCAt( szTechPrereqBorderID, sPanel, ArtFileMgr.getInterfaceArtInfo("TECH_TREE_BUTTON_BORDER").getPath(), iX + fX + 4, iY + 22, 32, 32, WidgetTypes.WIDGET_HELP_TECH_PREPREQ, eTech, -1, False )

			j = 0

			if bORPreReq:
				for j in range( gc.getNUM_OR_TECH_PREREQS() ):
					eTech = gc.getTechInfo(i).getPrereqOrTechs(j)
					if ( eTech > -1 ):
						iX = self.iX_LEFT_START_OR_PREREQS + ( (gc.getTechInfo(eTech).getGridX() - 1) * ( ( self.BOX_INCREMENT_X_SPACING + self.BOX_INCREMENT_WIDTH ) * self.PIXEL_INCREMENT ) )
						iY = ( gc.getTechInfo(eTech).getGridY() - 1 ) * ( self.BOX_INCREMENT_Y_SPACING * self.PIXEL_INCREMENT ) + 5

						# j is the pre-req, i is the tech...
						xDiff = gc.getTechInfo(i).getGridX() - gc.getTechInfo(eTech).getGridX()
						yDiff = gc.getTechInfo(i).getGridY() - gc.getTechInfo(eTech).getGridY()

						if (yDiff == 0):
							screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_X, iX + self.getXStart(), iY + self.getYStart(3), self.getWidth(xDiff), 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
							screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_HEAD, iX + self.getXStart() + self.getWidth(xDiff), iY + self.getYStart(3), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
						elif (yDiff < 0):
							if ( yDiff == -6 ):
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_X, iX + self.getXStart(), iY + self.getYStart(1), self.getWidth(xDiff) / 2, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_XY, iX + self.getXStart() + ( self.getWidth(xDiff) / 2 ), iY + self.getYStart(1), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_Y, iX + self.getXStart() + ( self.getWidth(xDiff) / 2 ), iY + self.getYStart(1) + 8 - self.getHeight(yDiff, 0), 8, self.getHeight(yDiff, 0) - 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_XMY, iX + self.getXStart() + ( self.getWidth(xDiff) / 2 ), iY + self.getYStart(1) - self.getHeight(yDiff, 0), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_X, iX + 8 + self.getXStart() + ( self.getWidth(xDiff) / 2 ), iY + self.getYStart(1) - self.getHeight(yDiff, 0), ( self.getWidth(xDiff) / 2 ) - 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_HEAD, iX + self.getXStart() + self.getWidth(xDiff), iY + self.getYStart(1) - self.getHeight(yDiff, 0), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
							elif ( yDiff == -2 and xDiff == 2 ):
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_X, iX + self.getXStart(), iY + self.getYStart(2), self.getWidth(xDiff) * 5 / 6, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_XY, iX + self.getXStart() + ( self.getWidth(xDiff) * 5 / 6 ), iY + self.getYStart(2), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_Y, iX + self.getXStart() + ( self.getWidth(xDiff) * 5 / 6 ), iY + self.getYStart(2) + 8 - self.getHeight(yDiff, 3), 8, self.getHeight(yDiff, 3) - 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_XMY, iX + self.getXStart() + ( self.getWidth(xDiff) * 5 / 6 ), iY + self.getYStart(2) - self.getHeight(yDiff, 3), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_X, iX + 8 + self.getXStart() + ( self.getWidth(xDiff) * 5 / 6 ), iY + self.getYStart(2) - self.getHeight(yDiff, 3), ( self.getWidth(xDiff) / 6 ) - 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_HEAD, iX + self.getXStart() + self.getWidth(xDiff), iY + self.getYStart(2) - self.getHeight(yDiff, 3), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
							else:
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_X, iX + self.getXStart(), iY + self.getYStart(2), self.getWidth(xDiff) / 2, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_XY, iX + self.getXStart() + ( self.getWidth(xDiff) / 2 ), iY + self.getYStart(2), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_Y, iX + self.getXStart() + ( self.getWidth(xDiff) / 2 ), iY + self.getYStart(2) + 8 - self.getHeight(yDiff, 3), 8, self.getHeight(yDiff, 3) - 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_XMY, iX + self.getXStart() + ( self.getWidth(xDiff) / 2 ), iY + self.getYStart(2) - self.getHeight(yDiff, 3), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_X, iX + 8 + self.getXStart() + ( self.getWidth(xDiff) / 2 ), iY + self.getYStart(2) - self.getHeight(yDiff, 3), ( self.getWidth(xDiff) / 2 ) - 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_HEAD, iX + self.getXStart() + self.getWidth(xDiff), iY + self.getYStart(2) - self.getHeight(yDiff, 3), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
						elif (yDiff > 0):
							if ( yDiff == 2 and xDiff == 2):
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_X, iX + self.getXStart(), iY + self.getYStart(4), self.getWidth(xDiff) / 6, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_MXMY, iX + self.getXStart() + ( self.getWidth(xDiff) / 6 ), iY + self.getYStart(4), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_Y, iX + self.getXStart() + ( self.getWidth(xDiff) / 6 ), iY + self.getYStart(4) + 8, 8, self.getHeight(yDiff, 3) - 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_MXY, iX + self.getXStart() + ( self.getWidth(xDiff) / 6 ), iY + self.getYStart(4) + self.getHeight(yDiff, 3), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_X, iX + 8 + self.getXStart() + ( self.getWidth(xDiff) / 6 ), iY + self.getYStart(4) + self.getHeight(yDiff, 3), ( self.getWidth(xDiff) * 5 / 6 ) - 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_HEAD, iX + self.getXStart() + self.getWidth(xDiff), iY + self.getYStart(4) + self.getHeight(yDiff, 3), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
							elif ( yDiff == 4 and xDiff == 1):
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_X, iX + self.getXStart(), iY + self.getYStart(5), self.getWidth(xDiff) / 3, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_MXMY, iX + self.getXStart() + ( self.getWidth(xDiff) / 3 ), iY + self.getYStart(5), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_Y, iX + self.getXStart() + ( self.getWidth(xDiff) / 3 ), iY + self.getYStart(5) + 8, 8, self.getHeight(yDiff, 0) - 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_MXY, iX + self.getXStart() + ( self.getWidth(xDiff) / 3 ), iY + self.getYStart(5) + self.getHeight(yDiff, 0), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_X, iX + 8 + self.getXStart() + ( self.getWidth(xDiff) / 3 ), iY + self.getYStart(5) + self.getHeight(yDiff, 0), ( self.getWidth(xDiff) * 2 / 3 ) - 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_HEAD, iX + self.getXStart() + self.getWidth(xDiff), iY + self.getYStart(5) + self.getHeight(yDiff, 0), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
							else:
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_X, iX + self.getXStart(), iY + self.getYStart(4), self.getWidth(xDiff) / 2, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_MXMY, iX + self.getXStart() + ( self.getWidth(xDiff) / 2 ), iY + self.getYStart(4), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_Y, iX + self.getXStart() + ( self.getWidth(xDiff) / 2 ), iY + self.getYStart(4) + 8, 8, self.getHeight(yDiff, 3) - 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_MXY, iX + self.getXStart() + ( self.getWidth(xDiff) / 2 ), iY + self.getYStart(4) + self.getHeight(yDiff, 3), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_X, iX + 8 + self.getXStart() + ( self.getWidth(xDiff) / 2 ), iY + self.getYStart(4) + self.getHeight(yDiff, 3), ( self.getWidth(xDiff) / 2 ) - 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )
								screen.addDDSGFCAt( self.getNextWidgetName("TechArrow"), sPanel, ARROW_HEAD, iX + self.getXStart() + self.getWidth(xDiff), iY + self.getYStart(4) + self.getHeight(yDiff, 3), 8, 8, WidgetTypes.WIDGET_GENERAL, -1, -1, False )

		return

# BUG - GP Tech Prefs - start
	def resetTechPrefs (self):
		self.pPrefs = TechPrefs.TechPrefs()

	def updateTechPrefs (self):
#		BugUtil.debug("cvTechChooser: updateTechPrefs")

		# If we are the Pitboss, we don't want to put up an interface at all
		if ( CyGame().isPitbossHost() ):
			return

		# These don't seem to be setup when screen is first opened
		if (gc.getNumTechInfos() <= 0 or gc.getNumFlavorTypes() <= 0):
			return

		# Get the screen and player
		screen = self.getScreen()
		pPlayer = gc.getPlayer(self.iCivSelected)

		# Don't show tech prefs during advanced start setup
		if (pPlayer.getAdvancedStartPoints() >= 0):
			return

		# Check to see if option is disabled
		if (not self.isGpTechPrefsEnabled()):
			if (self.bPrefsShowing):
				# ... and if so, remove icons if they are currently showing
				screen.hide( "GreatPersonHeading")
				for i, f in enumerate(FLAVORS):
					screen.hide( "GreatPerson" + str(f) )
					screen.hide( "GreatPersonTech" + str(f) )
					screen.hide( "GreatPersonTechNext" + str(f) )
				self.bPrefsShowing = False
			return
		# <advc.004a>
		iX = self.PREF_ICON_LEFT 
		iY = self.PREF_ICON_BOTTOM
		# </advc.004a>

		# Always redraw the GP icons because otherwise they are prone to disappearing

		# discover icon heading
		# <advc.004a> Not sure if the big light bulb really helps
		bShowIconHeading = True
		if bShowIconHeading: 
			iIconSize = 36 # was 48 </advc.004a>
			# advc.004a: Position handled above
			#iX = self.PREF_ICON_LEFT + 5 * PREF_ICON_SIZE / 4 - iIconSize / 2
			#iY = PREF_ICON_TOP - iIconSize - 40
			# advc.004a: WIDGET added
			screen.addDDSGFC( "GreatPersonHeading", ArtFileMgr.getInterfaceArtInfo("DISCOVER_TECHNOLOGY_BUTTON").getPath(), iX, iY-iIconSize/5, iIconSize, iIconSize, WidgetTypes.WIDGET_TECH_PREFS_HEADING, -1, -1 )
			iX += 3 * PREF_ICON_SIZE # advc.004a: Continue to the right

		# advc.004a: Merged into the code below
		#for i, f in enumerate(FLAVORS):
		#	# GP icon
		#	iUnitClass = getInfoTypeOrFail(UNIT_CLASSES[i])
		#	iUnitType = gc.getUnitClassInfo(iUnitClass).getDefaultUnitIndex()
		#	pUnitInfo = gc.getUnitInfo(iUnitType)
		#	iX = self.PREF_ICON_LEFT
		#	iY = PREF_ICON_TOP + 4 * i * PREF_ICON_SIZE
		#	screen.addDDSGFC( "GreatPerson" + str(f), pUnitInfo.getButton(), iX, iY, PREF_ICON_SIZE, PREF_ICON_SIZE, WidgetTypes.WIDGET_TECH_PREFS_ALL, f, -1 )
		self.bPrefsShowing = True

		# Remove any techs researched since last call, creating tree if necessary
		if not self.pPrefs:
			self.resetTechPrefs()
		self.pPrefs.removeKnownTechs()
		bAnyResearch = False # advc.004a: Don't show the second button then
		# Add all techs in research queue to set of soon-to-be-known techs
		sTechs = set()
		for i in range(gc.getNumTechInfos()):
			if (pPlayer.isResearchingTech(i)):
				sTechs.add(self.pPrefs.getTech(i))
				bAnyResearch = True # advc.004a

		# advc.004a: This loop is based on the placeGreatPeople function of the RFC:DoC mod
		for i, f in enumerate(FLAVORS):
			iUnitClass = getInfoTypeOrFail(UNIT_CLASSES[i])
			iUnitType = gc.getUnitClassInfo(iUnitClass).getDefaultUnitIndex()
			pUnitInfo = gc.getUnitInfo(iUnitType)
			screen.addDDSGFC( "GreatPerson" + str(f), pUnitInfo.getButton(), iX, iY, PREF_ICON_SIZE, PREF_ICON_SIZE, WidgetTypes.WIDGET_TECH_PREFS_ALL, i, -1 ) # advc.004a: pass i instead of f
			screen.show( "GreatPerson" + str(f) )
			iX += PREF_ICON_SIZE # advc.004a
			# Current tech GP will pop
			szButtonName = "GreatPersonTech" + str(f)
			pTech = self.pPrefs.getNextResearchableFlavorTech(f)
			if (pTech):
				screen.addDDSGFC( szButtonName, pTech.getInfo().getButton(), iX, iY, PREF_ICON_SIZE, PREF_ICON_SIZE, WidgetTypes.WIDGET_TECH_PREFS_CURRENT, f, pTech.getID() ) # advc.004a: Pass pTech to widget
			else:
				screen.addDDSGFC( szButtonName, self.NO_TECH_ART, iX, iY, PREF_ICON_SIZE, PREF_ICON_SIZE, WidgetTypes.WIDGET_TECH_PREFS_CURRENT, f, -1 )
			screen.show( szButtonName )
			iX += PREF_ICON_SIZE  # advc.004a
			# Tech GP will pop once selected techs are researched
			szButtonName = "GreatPersonTechNext" + str(f)
			# <advc.004a> Not sure whether to show the button when the queued tech doesn't make a difference. The two identical icons look strange, but should make it easier to learn how the current/ queued research affects the GP info.
			pTech2 = self.pPrefs.getNextResearchableWithFlavorTech(f, sTechs)
			if bAnyResearch and pTech2: #and pTech2 != pTech: # </advc.004a>
				screen.addDDSGFC( szButtonName, pTech2.getInfo().getButton(), iX, iY, PREF_ICON_SIZE, PREF_ICON_SIZE, WidgetTypes.WIDGET_TECH_PREFS_FUTURE, f, -1 )
				screen.show( szButtonName )
			else:
			#	screen.addDDSGFC( szButtonName, self.NO_TECH_ART, iX, iY, PREF_ICON_SIZE, PREF_ICON_SIZE, WidgetTypes.WIDGET_TECH_PREFS_FUTURE, f, -1 )
			# advc.004a: Rather show no button
				screen.hide( szButtonName )

# BUG - GP Tech Prefs - end
			iX += 2 * PREF_ICON_SIZE # advc.004a

	def isGpTechPrefsEnabled(self):
		# <!-- custom: invert meaning so the checkbox disables the bulbing indicators; keep defaults visible unless players opt out. (GPT-5.2-Codex) -->
		return not BugOpt.isShowGPTechPrefs()

	def TechRecord (self, inputClass):
		return 0

	# Clicked the parent?
	def ParentClick (self, inputClass):
		return 0

	def CivDropDown( self, inputClass ):
		if ( inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED ):
			screen = self.getScreen()
			iIndex = screen.getSelectedPullDownID("CivDropDown")
			self.iCivSelected = screen.getPullDownData("CivDropDown", iIndex)
			self.updateTechRecords(false)

	# Will handle the input for this screen...
	def handleInput (self, inputClass):
#		BugUtil.debug("cvTechChooser: handleInput")
#		BugUtil.debugInput(inputClass)

		# Get the screen
		screen = self.getScreen()

		szWidgetName = inputClass.getFunctionName() + str(inputClass.getID())

		# Advanced Start Stuff
		pPlayer = gc.getPlayer(self.iCivSelected)
		if (pPlayer.getAdvancedStartPoints() >= 0):
#			BugUtil.debug("cvTechChooser: handleInput - advancedstart")
			# Add tech button
			if (inputClass.getFunctionName() == "AddTechButton"):
				if (pPlayer.getAdvancedStartTechCost(self.m_iSelectedTech, true) != -1):
					CyMessageControl().sendAdvancedStartAction(AdvancedStartActionTypes.ADVANCEDSTARTACTION_TECH, self.iCivSelected, -1, -1, self.m_iSelectedTech, true)	#Action, Player, X, Y, Data, bAdd
					self.m_bTechRecordsDirty = true
					self.m_bSelectedTechDirty = true

			# Tech clicked on
			elif (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED):
				if (inputClass.getButtonType() == WidgetTypes.WIDGET_TECH_TREE):
					self.m_iSelectedTech = inputClass.getData1()
					self.updateSelectedTech()

		' Calls function mapped in TechChooserInputMap'
		# only get from the map if it has the key
		if ( inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED ):
#			BugUtil.debug("cvTechChooser: handleInput - dropdown")
			self.CivDropDown( inputClass )
			return 1

		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED):
			if szWidgetName == self.sTechSelectTab:
				self.sTechTabID = self.sTechSelectTab
				self.ShowTab()

			elif szWidgetName == self.sTechTradeTab:
				self.sTechTabID = self.sTechTradeTab
				self.ShowTab()
			elif inputClass.getButtonType() == WidgetTypes.WIDGET_PYTHON:
				if inputClass.getData1() == SAS_MAGIC_PEDIA_PYTHON_BUILD:
					import CvScreensInterface
					CvScreensInterface.pediaJumpToBuild((inputClass.getData2(),))
					return 1
			# <!-- custom: reason for this hybrid path: in this Tech Advisor context, WIDGET_PYTHON gives our custom redirect to Sevopedia Builds but does not provide the base build hover/help text; by using WIDGET_HELP_IMPROVEMENT we restore "can build ..." hover text, then reroute clicks for non-improvement builds (routes/feature removal) to the Builds page here. (GPT-5.3-Codex) -->
			elif inputClass.getButtonType() == WidgetTypes.WIDGET_HELP_IMPROVEMENT:
				iBuild = inputClass.getData2()
				if iBuild >= 0 and iBuild < gc.getNumBuildInfos():
					if gc.getBuildInfo(iBuild).getImprovement() == -1:
						import CvScreensInterface
						CvScreensInterface.pediaJumpToBuild((iBuild,))
						return 1

		return 0

	def getNextWidgetName(self, sName):
#		BugUtil.debug("cvTechChooser: getNextWidgetName %i %i", self.nWidgetCount, len(self.sWidgets))
		szName = sName + str(self.nWidgetCount)
		self.nWidgetCount += 1
		self.sWidgets.append(szName)
		return szName

	def deleteWidgets(self):
#		BugUtil.debug("cvTechChooser: deleteWidgets %i %i", self.nWidgetCount, len(self.sWidgets))
		screen = self.getScreen()
		for w in self.sWidgets:
#			BugUtil.debug("cvTechChooser: deleteWidgets '%s'", w)
			screen.deleteWidget(w)

		self.nWidgetCount = 0
		self.sWidgets = []
		return

	def getXStart(self):
		return ( self.BOX_INCREMENT_WIDTH * self.PIXEL_INCREMENT )

	def getXSpacing(self):
		return ( self.BOX_INCREMENT_X_SPACING * self.PIXEL_INCREMENT )

	def getYStart(self, iY):
		return int((((self.BOX_INCREMENT_HEIGHT * self.PIXEL_INCREMENT ) / 6.0) * iY) - self.PIXEL_INCREMENT )

	def getWidth(self, xDiff):
		return ( ( xDiff * self.getXSpacing() ) + ( ( xDiff - 1 ) * self.getXStart() ) )

	def getHeight(self, yDiff, nFactor):
		return ( ( nFactor + ( ( abs( yDiff ) - 1 ) * 6 ) ) * self.PIXEL_INCREMENT )

	def update(self, fDelta):
		if (CyInterface().isDirty(InterfaceDirtyBits.Advanced_Start_DIRTY_BIT)):
			CyInterface().setDirty(InterfaceDirtyBits.Advanced_Start_DIRTY_BIT, false)

			if (self.m_bSelectedTechDirty):
				self.m_bSelectedTechDirty = false
				self.updateSelectedTech()

			if (self.m_bTechRecordsDirty):
				self.m_bTechRecordsDirty = false
				self.updateTechRecords(true)

			if (gc.getPlayer(self.iCivSelected).getAdvancedStartPoints() < 0):
				# hide the screen
				screen = self.getScreen()
				screen.hide("AddTechButton")
				screen.hide("ASPointsLabel")
				screen.hide("SelectedTechLabel")

		return

	def updateSelectedTech(self):
		pPlayer = gc.getPlayer(CyGame().getActivePlayer())

		# Get the screen
		screen = self.getScreen()

		szName = ""
		iCost = 0

		if (self.m_iSelectedTech != -1):
			szName = gc.getTechInfo(self.m_iSelectedTech).getDescription()
			iCost = gc.getPlayer(CyGame().getActivePlayer()).getAdvancedStartTechCost(self.m_iSelectedTech, true)

		if iCost > 0:
			szText = sasFontTagTitle + localText.getText("TXT_KEY_WB_AS_SELECTED_TECH_COST", (iCost, pPlayer.getAdvancedStartPoints())) + SAS_FONT_TAG_CLOSE
			screen.setLabel( "ASPointsLabel", "Background", szText, CvUtil.FONT_LEFT_JUSTIFY, self.X_ADVANCED_START_TEXT, self.Y_ADD_TECH_BUTTON + 3, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		else:
			screen.hide("ASPointsLabel")

		szText = sasFontTagTitle
		szText += localText.getText("TXT_KEY_WB_AS_SELECTED_TECH", (szName,))
		szText += SAS_FONT_TAG_CLOSE
		screen.setLabel( "SelectedTechLabel", "Background", szText, CvUtil.FONT_LEFT_JUSTIFY, self.X_ADVANCED_START_TEXT + 250, self.Y_ADD_TECH_BUTTON + 3, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		# Want to add
		if (pPlayer.getAdvancedStartTechCost(self.m_iSelectedTech, true) != -1):
			screen.show("AddTechButton")
		else:
			screen.hide("AddTechButton")

	def onClose(self):
		# <!-- custom: ensure scoreboard alignment state resets even when Tech Chooser closes through onClose (not hideScreen). (GPT-5.3-Codex) -->
		CyInterface().setDirty(InterfaceDirtyBits.Score_DIRTY_BIT, True)
		pPlayer = gc.getPlayer(self.iCivSelected)
		if (pPlayer.getAdvancedStartPoints() >= 0):
			CyInterface().setDirty(InterfaceDirtyBits.Advanced_Start_DIRTY_BIT, true)
		return 0

class TechChooserMaps:

	TechChooserInputMap = {
		'TechRecord'			: CvTechChooser().TechRecord,
		'TechID'				: CvTechChooser().ParentClick,
		'TechPane'				: CvTechChooser().ParentClick,
		'TechButtonID'			: CvTechChooser().ParentClick,
		'TechButtonBorder'		: CvTechChooser().ParentClick,
		'Unit'					: CvTechChooser().ParentClick,
		'Building'				: CvTechChooser().ParentClick,
		'Obsolete'				: CvTechChooser().ParentClick,
		'ObsoleteX'				: CvTechChooser().ParentClick,
		'Move'					: CvTechChooser().ParentClick,
		'FreeUnit'				: CvTechChooser().ParentClick,
		'FeatureProduction'			: CvTechChooser().ParentClick,
		'Worker'				: CvTechChooser().ParentClick,
		'TradeRoutes'			: CvTechChooser().ParentClick,
		'HealthRate'			: CvTechChooser().ParentClick,
		'HappinessRate'			: CvTechChooser().ParentClick,
		'FreeTech'				: CvTechChooser().ParentClick,
		'LOS'					: CvTechChooser().ParentClick,
		'MapCenter'				: CvTechChooser().ParentClick,
		'MapReveal'				: CvTechChooser().ParentClick,
		'MapTrade'				: CvTechChooser().ParentClick,
		'TechTrade'				: CvTechChooser().ParentClick,
		'OpenBorders'		: CvTechChooser().ParentClick,
		'BuildBridge'			: CvTechChooser().ParentClick,
		'Irrigation'			: CvTechChooser().ParentClick,
		'Improvement'			: CvTechChooser().ParentClick,
		'DomainExtraMoves'			: CvTechChooser().ParentClick,
		'AdjustButton'			: CvTechChooser().ParentClick,
		'TerrainTradeButton'	: CvTechChooser().ParentClick,
		'SpecialBuildingButton'	: CvTechChooser().ParentClick,
		'YieldChangeButton'		: CvTechChooser().ParentClick,
		'BonusRevealButton'		: CvTechChooser().ParentClick,
		'CivicRevealButton'		: CvTechChooser().ParentClick,
		'ProjectInfoButton'		: CvTechChooser().ParentClick,
		'ProcessInfoButton'		: CvTechChooser().ParentClick,
		'FoundReligionButton'	: CvTechChooser().ParentClick,
		'CivDropDown'			: CvTechChooser().CivDropDown,
		}

