## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
from CvPythonExtensions import *
import CvUtil
import ScreenInput
import CvScreenEnums
import BugUtil
import PlayerUtil
import TradeUtil
from SASFontUtils import *
from SASUtils import *
import SASTextScale
# <!-- custom: AdvCiv-SAS readability pass: use LABEL as the base domestic-advisor text tag (instead of BODY) for clearer upscaled UI text. (GPT-5.3-Codex) -->
#
#	IMPORTANT INFORMATION
#	
#	All widget names MUST be unique when creating screens.  If you create a widget named 'Hello', and then try to create another named 'Hello', it will modify the first hello.
#
#	Also, when attaching widgets, 'Background' is a reserve word meant for the background widget.  Do NOT use 'Background' to name any widget, but when attaching to the background, please use the 'Background' keyword.
#
#  Thanks to Lee Reeves, AKA Taelis on civfanatics.com
#  Thanks to Solver

# globals
gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class CvDomesticAdvisor:
	# Domestic Advisor Screen
	#
	def __init__(self):
		self.listSelectedCities = []
		self.iLanguageLoaded = -1
		self.iActivePlayer = CyGame().getActivePlayer()
		self.DEBUG_DROPDOWN_ID = "DomesticAdvisorDropdownWidget"
		self.FREE_COLONY_BUTTON_ID = "DomesticSplit"
		self.FREE_COLONY_BUTTON_SIZE = 28
		# <!-- custom: minimal shared-advisor widget plumbing only for placeAdvisorLegendLink; Domestic otherwise uses fixed widget IDs and finance-only dynamic widgets. (GPT-5.5) -->
		self.WIDGET_ID = "DomesticAdvisorWidget"
		self.nWidgetCount = 0
		self.Z_CONTROLS = -0.1
		# <!-- custom: keep screen-independent advisor edge constants in init; runtime resolution-dependent geometry is computed in interfaceScreen through shared SASUtils helpers. (GPT-5.3-Codex) -->
		self.W_LEFT_SPACE_FOR_COMMERCE_SLIDERS = SAS_ADVISOR_LEFT_SPACE_FOR_COMMERCE_SLIDERS
		self.W_RIGHT_SPACE_FOR_SCOREBOARD = SAS_ADVISOR_RIGHT_SPACE_FOR_SCOREBOARD
		self.H_TOP_SPACE_FOR_TECH_BAR = SAS_ADVISOR_TOP_SPACE_FOR_TECH_BAR
		self.H_BOTTOM_SPACE = SAS_ADVISOR_BOTTOM_SPACE
		self.nNormalizedTableWidth = 970
		self.nFirstSpecialistX = 30
		self.nSpecialistWidth = 32
		self.nSpecialistLength = 32
		self.nSpecialistDistance = 100
		self.nPlusOffsetX = -4
		self.nMinusOffsetX = 16
		self.nPlusOffsetY = self.nMinusOffsetY = 30
		self.nPlusWidth = self.nPlusHeight = self.nMinusWidth = self.nMinusHeight = 20
		self.nSpecTextOffsetX = 40
		self.nSpecTextOffsetY = 10
		self.OVERVIEW3_SPECIALIST_ICON_SIZE = 24
		self.Y_TITLE = SAS_ADVISOR_TITLE_Y
		self.PAGE_OVERVIEW1 = 0
		self.PAGE_OVERVIEW2 = 1
		self.PAGE_OVERVIEW3 = 2
		self.PAGE_OVERVIEW4 = 3
		self.PAGE_FINANCE = 4
		# <!-- custom: add Overview 2/3/4 tabs for BUG-style per-city status, culture/GP progress, and people/organization data in the AdvCiv-SAS non-BUG Domestic Advisor. (Claude code Opus 4.7; cleaned/extended by GPT-5.5) -->
		self.PAGE_IDS = [self.PAGE_OVERVIEW1, self.PAGE_OVERVIEW2, self.PAGE_OVERVIEW3, self.PAGE_OVERVIEW4, self.PAGE_FINANCE]
		self.OVERVIEW_PAGE_IDS = [self.PAGE_OVERVIEW1, self.PAGE_OVERVIEW2, self.PAGE_OVERVIEW3, self.PAGE_OVERVIEW4]
		self.iPage = self.PAGE_OVERVIEW1
		self.PAGE_TAB_IDS = []
		for iPage in self.PAGE_IDS:
			self.PAGE_TAB_IDS.append("DomesticTabButton" + str(iPage))
		self.PAGE_LINK_WIDTH = []
		self.TABLE_OVERVIEW1 = "CityListBackground1"
		self.TABLE_OVERVIEW2 = "CityListBackground2"
		self.TABLE_OVERVIEW3 = "CityListBackground3"
		self.TABLE_OVERVIEW4 = "CityListBackground4"
		self.OVERVIEW_CITY_COL_WIDTH = 100
		self.OVERVIEW_RELIGION_BASE_COL_WIDTH = 55
		self.OVERVIEW_CORPORATION_BASE_COL_WIDTH = 50
		self.OVERVIEW_PRODUCING_BASE_COL_WIDTH = 127
		# <!-- custom: 40px empirically fits upscaled signed modifiers like "+150" with some leeway; keep it modest because the overview tables are otherwise nearly full. (GPT-5.5) -->
		self.OVERVIEW_SIGNED_MODIFIER_COL_WIDTH = 40
		self.bOverview1TableCreated = False
		self.bOverview2TableCreated = False
		self.bOverview3TableCreated = False
		self.bOverview4TableCreated = False
		self.nFinanceWidgetCount = 0
		self.FINANCE_WIDGET_ID = "DomesticFinanceWidget"
		# <!-- custom: finance-tab layout constants are screen-independent; keep them in init and only compute runtime positions/sizes in configureFinanceLayout. (GPT-5.3-Codex) -->
		self.FINANCE_INNER_MARGIN = 24
		self.FINANCE_COLUMN_GAP = 20
		self.FINANCE_TOP_PANEL_HEIGHT = 90
		self.FINANCE_TEXT_MARGIN = 15
		self.FINANCE_Y_SPACING = 30
		self.FINANCE_Z_CONTROLS = -2.3
		self.FINANCE_DZ = -0.2
		# <!-- custom: tab labels that do not depend on runtime geometry stay in init/initText; only link widths are runtime-dependent. (GPT-5.3-Codex) -->
		self.TEXT_TAB_OVERVIEW1 = "OVERVIEW 1"
		self.TEXT_TAB_OVERVIEW2 = "OVERVIEW 2"
		self.TEXT_TAB_OVERVIEW3 = "OVERVIEW 3"
		self.TEXT_TAB_OVERVIEW4 = "OVERVIEW 4"
		self.aOverview4Specialists = []
		self.aOverview4SpecialistHeaders = []
		# <!-- custom: cached vanilla engine defines. (Claude code Sonnet 4.6) -->
		self.iCOMMERCE_PERCENT_CHANGE_INCREMENTS = None
		self.iFOOD_CONSUMPTION_PER_POPULATION = None

	def initText(self):
		# <!-- custom: cache Domestic Advisor header texts/icons once per language to avoid repeated translation/symbol lookups on redraw.
		# Keep column widths runtime-based because they depend on current screen size. (GPT-5.3-Codex) -->
		if self.iLanguageLoaded == CyGame().getCurrentLanguage() or not CyGame().isFinalInitialized():
			return
		self.iLanguageLoaded = CyGame().getCurrentLanguage()

		# <!-- custom: cache only raw header text/symbols in init; apply font tags at draw time so runtime label scaling always takes effect. (GPT-5.3-Codex) -->
		self.HEADER_NAME = localText.getText("TXT_KEY_DOMESTIC_ADVISOR_NAME", ())
		self.HEADER_POPULATION = u"Pop"
		self.HEADER_HAPPINESS = (u"%c" % CyGame().getSymbolID(FontSymbols.HAPPY_CHAR))
		self.HEADER_HEALTH = (u"%c" % CyGame().getSymbolID(FontSymbols.HEALTHY_CHAR))
		self.HEADER_FOOD = (u"%c" % gc.getYieldInfo(YieldTypes.YIELD_FOOD).getChar())
		self.HEADER_PRODUCTION = (u"%c" % gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar())
		self.HEADER_GOLD = (u"%c" % gc.getCommerceInfo(CommerceTypes.COMMERCE_GOLD).getChar())
		self.HEADER_RESEARCH = (u"%c" % gc.getCommerceInfo(CommerceTypes.COMMERCE_RESEARCH).getChar())
		self.HEADER_ESPIONAGE = (u"%c" % gc.getCommerceInfo(CommerceTypes.COMMERCE_ESPIONAGE).getChar())
		self.HEADER_CULTURE = (u"%c" % gc.getCommerceInfo(CommerceTypes.COMMERCE_CULTURE).getChar())
		self.HEADER_TRADE = (u"%c" % CyGame().getSymbolID(FontSymbols.TRADE_CHAR))
		self.HEADER_COMMERCE_YIELD_MODIFIER = (u"%c%%" % gc.getYieldInfo(YieldTypes.YIELD_COMMERCE).getChar())
		self.HEADER_MAINTENANCE = (u"%c" % CyGame().getSymbolID(FontSymbols.BAD_GOLD_CHAR))
		self.HEADER_GREAT_PERSON = (u"%c" % CyGame().getSymbolID(FontSymbols.GREAT_PEOPLE_CHAR))
		self.HEADER_GARRISON = (u"%c" % CyGame().getSymbolID(FontSymbols.STRENGTH_CHAR))
		self.HEADER_PRODUCING = localText.getText("TXT_KEY_DOMESTIC_ADVISOR_PRODUCING", ())
		self.HEADER_REVOLT = (u"%c" % CyGame().getSymbolID(FontSymbols.OCCUPATION_CHAR))
		self.HEADER_UNHAPPINESS = (u"%c" % CyGame().getSymbolID(FontSymbols.UNHAPPY_CHAR))
		# <!-- custom: use the angry-pop font icon here; the city-screen angry-citizen texture is too similar to specialist portraits in compact table headers. (GPT-5.5) -->
		self.HEADER_ANGRY_POPULATION = (u"%c" % CyGame().getSymbolID(FontSymbols.ANGRY_POP_CHAR))
		self.HEADER_UNHEALTH = (u"%c" % CyGame().getSymbolID(FontSymbols.UNHEALTHY_CHAR))
		self.HEADER_EATEN_FOOD = (u"%c" % CyGame().getSymbolID(FontSymbols.EATEN_FOOD_CHAR))
		self.HEADER_DEFENSE = (u"%c" % CyGame().getSymbolID(FontSymbols.DEFENSE_CHAR))
		self.HEADER_DELTA = u"d"
		self.HEADER_FOUNDED = u"Founded"
		self.HEADER_REAL_POPULATION = u"POP#"
		self.HEADER_COMMERCE_MODIFIERS = []
		for eCommerce in range(CommerceTypes.NUM_COMMERCE_TYPES):
			self.HEADER_COMMERCE_MODIFIERS.append(u"%c%%" % gc.getCommerceInfo(eCommerce).getChar())
		self.HEADER_RELIGIONS = u"Rel"
		for iRel in range(gc.getNumReligionInfos()):
			self.HEADER_RELIGIONS = (u"%c" % gc.getReligionInfo(iRel).getChar())
			break
		self.HEADER_CORPORATIONS = u"Corp"
		for iCorp in range(gc.getNumCorporationInfos()):
			self.HEADER_CORPORATIONS = (u"%c" % gc.getCorporationInfo(iCorp).getChar())
			break
		self.aOverview4Specialists = []
		self.aOverview4SpecialistHeaders = []
		for eSpec in range(gc.getNumSpecialistInfos()):
			# <!-- custom: include non-visible settled Great Specialists here; the footer controls still only use visible assignable specialists. (GPT-5.5) -->
			self.aOverview4Specialists.append(eSpec)
			self.aOverview4SpecialistHeaders.append(SASTextScale.labelImageText(gc.getSpecialistInfo(eSpec).getButton(), self.OVERVIEW3_SPECIALIST_ICON_SIZE))
		self.SCREEN_TITLE = sasFontTagTitle.bold + localText.getText("TXT_KEY_DOMESTIC_ADVISOR_TITLE", ()).upper() + SAS_FONT_TAG_CLOSE
		self.TEXT_EXIT = sasFontTagTitle + localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT", ()).upper() + SAS_FONT_TAG_CLOSE
		self.TEXT_TAB_FINANCE = localText.getText("TXT_KEY_ECONOMICS_ADVISOR_FINANCE_TAB", ()).upper()
		self.PAGE_NAME_LIST = [self.TEXT_TAB_OVERVIEW1, self.TEXT_TAB_OVERVIEW2, self.TEXT_TAB_OVERVIEW3, self.TEXT_TAB_OVERVIEW4, self.TEXT_TAB_FINANCE]
		self.COLOR_YELLOW = getInfoTypeOrFail("COLOR_YELLOW")
		self.PROCESS_WEALTH = getInfoTypeOrFail("PROCESS_WEALTH")
		self.COLOR_POSITIVE_PREFIX = localText.getText("TXT_KEY_COLOR_POSITIVE", ())
		self.COLOR_NEGATIVE_PREFIX = localText.getText("TXT_KEY_COLOR_NEGATIVE", ())
		self.COLOR_REVERT_SUFFIX = localText.getText("TXT_KEY_COLOR_REVERT", ())
		self.ART_SCREEN_BG_OPAQUE = ArtFileMgr.getInterfaceArtInfo("SCREEN_BG_OPAQUE").getPath()
		self.ART_CITY_SELECTION_BUTTON = ArtFileMgr.getInterfaceArtInfo("INTERFACE_BUTTONS_CITYSELECTION").getPath()

		# <!-- custom: cache finance-tab deterministic labels in initText (language-dependent only) to avoid repeated lookup on each redraw. (GPT-5.3-Codex) -->
		self.TEXT_FIN_COMMERCE = localText.getText("TXT_KEY_CONCEPT_COMMERCE", ())
		self.TEXT_FIN_INCOME_HEADER = localText.getText("TXT_KEY_FINANCIAL_ADVISOR_INCOME_HEADER", ())
		self.TEXT_FIN_EXPENSES_HEADER = localText.getText("TXT_KEY_FINANCIAL_ADVISOR_EXPENSES_HEADER", ())
		self.TEXT_FIN_DOMESTIC_TRADE = localText.getText("TXT_KEY_CONCEPT_DOMESTIC_TRADE", ())
		self.TEXT_FIN_TRADE = localText.getText("TXT_KEY_CONCEPT_TRADE", ())
		self.TEXT_FIN_FOREIGN_TRADE = localText.getText("TXT_KEY_CONCEPT_FOREIGN_TRADE", ())
		self.TEXT_FIN_CORPORATIONS = localText.getText("TXT_KEY_CONCEPT_CORPORATIONS", ())
		self.TEXT_FIN_SPECIALISTS = localText.getText("TXT_KEY_CONCEPT_SPECIALISTS", ())
		self.TEXT_FIN_BUILDINGS = localText.getText("TXT_KEY_CONCEPT_BUILDINGS", ())
		self.TEXT_FIN_CIVICS_MULTIPLIERS = localText.getText("TXT_KEY_MISC_CIVICS_MULTIPLIERS", ())
		self.TEXT_FIN_BUG_COMMERCE = localText.getText("TXT_KEY_BUG_FINANCIAL_ADVISOR_COMMERCE", ())
		self.TEXT_FIN_TAXES = localText.getText("TXT_KEY_FINANCIAL_ADVISOR_TAXES", ())
		self.TEXT_FIN_HQ = localText.getText("TXT_KEY_CORPORATION_HEADQUARTERS", ())
		self.TEXT_FIN_SHRINES = localText.getText("TXT_KEY_CONCEPT_RELIGIOUS_SHRINES", ())
		self.TEXT_FIN_WEALTH = localText.getText("TXT_KEY_PROCESS_WEALTH", ())
		self.TEXT_FIN_PER_TURN = localText.getText("TXT_KEY_FINANCIAL_ADVISOR_PER_TURN", ())
		self.TEXT_FIN_INCOME = localText.getText("TXT_KEY_FINANCIAL_ADVISOR_INCOME", ())
		self.TEXT_FIN_UNIT_COST = localText.getText("TXT_KEY_FINANCIAL_ADVISOR_UNITCOST", ())
		self.TEXT_FIN_UNIT_SUPPLY = localText.getText("TXT_KEY_FINANCIAL_ADVISOR_UNITSUPPLY", ())
		self.TEXT_FIN_MAINTENANCE = localText.getText("TXT_KEY_FINANCIAL_ADVISOR_MAINTENANCE", ())
		self.TEXT_FIN_CIVICS = localText.getText("TXT_KEY_FINANCIAL_ADVISOR_CIVICS", ())
		self.TEXT_FIN_COST_PER_TURN = localText.getText("TXT_KEY_FINANCIAL_ADVISOR_COST_PER_TURN", ())
		self.TEXT_FIN_EXPENSES = localText.getText("TXT_KEY_FINANCIAL_ADVISOR_EXPENSES", ())

	def updateRuntimeLayout(self, screen):
		self.X_SCREEN, self.Y_SCREEN, self.nScreenWidth, self.nScreenHeight = getAdvisorRuntimeBounds(screen, self.W_LEFT_SPACE_FOR_COMMERCE_SLIDERS, self.W_RIGHT_SPACE_FOR_SCOREBOARD, self.H_TOP_SPACE_FOR_TECH_BAR, self.H_BOTTOM_SPACE)
		self.X_TITLE, self.X_EXIT, self.Y_EXIT, self.Y_LINK, self.Y_BOTTOM_PANEL = getAdvisorRuntimeAnchors(self.nScreenWidth, self.nScreenHeight)
		(self.nMainPanelX, self.nMainPanelY, self.nMainPanelWidth, self.nMainPanelHeight), (self.nTableX, self.nTableY, self.nTableWidth, self.nTableHeight) = getAdvisorMaximizedPanelLayout(self.nScreenWidth, self.Y_BOTTOM_PANEL)
		# <!-- custom: keep specialist controls in the footer strip so the table can use symmetric margins inside the main panel. (GPT-5.3-Codex) -->
		self.nSpecialistY = self.Y_BOTTOM_PANEL + 2

	def updateRuntimeTabLinkWidths(self):
		# <!-- custom: reserve a footer slot for the restored Free Colony/Liberate action so it cannot overlap the five Domestic Advisor tabs. See KI#233. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		self.X_FREE_COLONY_BUTTON = self.X_EXIT - CyInterface().determineWidth(self.TEXT_EXIT) - self.FREE_COLONY_BUTTON_SIZE - 20
		self.PAGE_LINK_WIDTH[:] = getAdvisorRuntimeLinkWidths(CyInterface(), self.PAGE_NAME_LIST, self.TEXT_EXIT, self.X_FREE_COLONY_BUTTON - 10)

	# Screen construction function
	def interfaceScreen(self, argsList=None):
		self.iActivePlayer = getAdvisorValidPerspectivePlayer(self.iActivePlayer, bIncludeBarbarians=True, bAllowVassalPerspective=True)
		if argsList is not None:
			if isinstance(argsList, (list, tuple)) and len(argsList) > 0:
				if argsList[0] in self.PAGE_IDS:
					self.iPage = argsList[0]
			elif argsList in self.PAGE_IDS:
				self.iPage = argsList

		# Create a new screen, called DomesticAdvisur, using the file CvDomesticAdvisor.py for input
		screen = CyGInterfaceScreen( "DomesticAdvisor", CvScreenEnums.DOMESTIC_ADVISOR )
		self.updateRuntimeLayout(screen)
		self.initText()
		self.updateRuntimeTabLinkWidths()

		if screen.isActive():
			self.drawContents()
			return

		screen.setRenderInterfaceOnly(True)
		screen.setDimensions(self.X_SCREEN, self.Y_SCREEN, self.nScreenWidth, self.nScreenHeight)
		screen.showScreen(PopupStates.POPUPSTATE_IMMEDIATE, False)

		# Here we set the background widget and exit button, and we show the screen
		screen.addDrawControl("DomesticAdvisorBackground", self.ART_SCREEN_BG_OPAQUE, 0, 0, self.nScreenWidth, self.nScreenHeight, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		screen.addPanel( "DomesticTopPanel", u"", u"", True, False, 0, 0, self.nScreenWidth, 55, PanelStyles.PANEL_STYLE_TOPBAR )
		screen.addPanel( "DomesticBottomPanel", u"", u"", True, False, 0, self.Y_BOTTOM_PANEL, self.nScreenWidth, 55, PanelStyles.PANEL_STYLE_BOTTOMBAR )
		screen.showWindowBackground(False)
		screen.setLabel("DomesticTitleHeader", "Background", self.SCREEN_TITLE, CvUtil.FONT_CENTER_JUSTIFY, self.X_TITLE, self.Y_TITLE, -0.1, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.setText("DomesticExit", "Background", self.TEXT_EXIT, CvUtil.FONT_RIGHT_JUSTIFY, self.X_EXIT, self.Y_EXIT, -0.1, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_CLOSE_SCREEN, -1, -1 )
		screen.addPanel("DomesticMainPanel", "", "", True, True, self.nMainPanelX, self.nMainPanelY, self.nMainPanelWidth, self.nMainPanelHeight, PanelStyles.PANEL_STYLE_BLUE50)
		self.drawDebugDropdown()

		# Erase the flag?
		CyInterface().setDirty(InterfaceDirtyBits.MiscButtons_DIRTY_BIT, True)

		self.drawContents()

	def drawDebugDropdown(self):
		# <!-- custom: add the debug player dropdown missing from the old Domestic Advisor layout; the reworked tabbed advisor now mirrors other advisors and can inspect another player's cities without changing the real active player. (GPT-5.5) -->
		addAdvisorDebugDropdown(self.getScreen(), self.DEBUG_DROPDOWN_ID, self.iActivePlayer, bIncludeBarbarians=True, bAllowVassalPerspective=True)

	def drawTabs(self):
		screen = CyGInterfaceScreen( "DomesticAdvisor", CvScreenEnums.DOMESTIC_ADVISOR )
		self.nWidgetCount = 0
		drawAdvisorFooterTabs(screen, self.PAGE_TAB_IDS, self.PAGE_NAME_LIST, self.PAGE_LINK_WIDTH, self.iPage, self.Y_LINK, -0.1, self.COLOR_YELLOW)
		self.drawFreeColonyButton(screen)
		placeAdvisorLegendLink(self, "CONCEPT_SAS_DOMESTIC_ADVISOR_LEGEND", self.nScreenWidth - 12, self.Y_TITLE)

	def drawFreeColonyButton(self, screen):
		screen.deleteWidget(self.FREE_COLONY_BUTTON_ID)
		# <!-- custom: The native Free Colony action always executes for the real active player and carries no selected-player identity.
		# Do not show eligibility computed for a different Debug-selected perspective. See KI#1067. (GPT-5.6-Sol) -->
		if self.iActivePlayer != CyGame().getActivePlayer():
			return
		if isAdvisorReadOnlyPerspective(self.iActivePlayer):
			return
		player = gc.getPlayer(self.iActivePlayer)
		bCanLiberate = player.canSplitEmpire()
		(loopCity, iter) = player.firstCity(false)
		while loopCity and not bCanLiberate:
			iLiberationPlayer = loopCity.getLiberationPlayer(false)
			if iLiberationPlayer != -1 and not gc.getTeam(gc.getPlayer(iLiberationPlayer).getTeam()).isAtWar(CyGame().getActiveTeam()):
				bCanLiberate = true
			(loopCity, iter) = player.nextCity(iter, false)
		if not bCanLiberate:
			return
		# <!-- custom: The SAS tabbed Domestic Advisor shell lost base AdvCiv's visible Free Colony/Liberate action even though the native control remained available.
		# Restore its original WIDGET_ACTION and style for the real active-player perspective; the native action supplies the tooltip and popup. See KI#233. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		iY = self.Y_BOTTOM_PANEL + (55 - self.FREE_COLONY_BUTTON_SIZE) / 2
		screen.setImageButton(self.FREE_COLONY_BUTTON_ID, "", self.X_FREE_COLONY_BUTTON, iY, self.FREE_COLONY_BUTTON_SIZE, self.FREE_COLONY_BUTTON_SIZE, WidgetTypes.WIDGET_ACTION, gc.getControlInfo(ControlTypes.CONTROL_FREE_COLONY).getActionInfoIndex(), -1)
		screen.setStyle(self.FREE_COLONY_BUTTON_ID, "Button_HUDAdvisorVictory_Style")

	def getScreen(self):
		return CyGInterfaceScreen("DomesticAdvisor", CvScreenEnums.DOMESTIC_ADVISOR)

	def getNextWidgetName(self):
		szName = self.WIDGET_ID + str(self.nWidgetCount)
		self.nWidgetCount += 1
		return szName

	def configureFinanceLayout(self):
		iInnerMargin = self.FINANCE_INNER_MARGIN
		iColumnGap = self.FINANCE_COLUMN_GAP
		self.FIN_X_LEFT_PANEL = self.nMainPanelX + iInnerMargin
		self.FIN_PANE_WIDTH = (self.nMainPanelWidth - (2 * iInnerMargin) - (2 * iColumnGap)) / 3
		self.FIN_X_MIDDLE_PANEL = self.FIN_X_LEFT_PANEL + self.FIN_PANE_WIDTH + iColumnGap
		self.FIN_X_RIGHT_PANEL = self.FIN_X_MIDDLE_PANEL + self.FIN_PANE_WIDTH + iColumnGap
		self.FIN_Y_TOP_PANEL = self.nMainPanelY + iInnerMargin
		self.FIN_H_TOP_PANEL = self.FINANCE_TOP_PANEL_HEIGHT
		self.FIN_Y_LOCATION = self.FIN_Y_TOP_PANEL + self.FIN_H_TOP_PANEL + iInnerMargin
		self.FIN_PANE_HEIGHT = self.nMainPanelY + self.nMainPanelHeight - self.FIN_Y_LOCATION - iInnerMargin

	def getNextFinanceWidgetName(self):
		szName = self.FINANCE_WIDGET_ID + str(self.nFinanceWidgetCount)
		self.nFinanceWidgetCount += 1
		return szName

	def clearOverviewWidgets(self):
		screen = self.getScreen()
		if self.bOverview1TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW1)
			self.bOverview1TableCreated = False
		if self.bOverview2TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW2)
			self.bOverview2TableCreated = False
		if self.bOverview3TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW3)
			self.bOverview3TableCreated = False
		if self.bOverview4TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW4)
			self.bOverview4TableCreated = False
		self.hideSpecialists()

	def clearFinanceWidgets(self):
		if self.nFinanceWidgetCount <= 0:
			return
		screen = self.getScreen()
		for i in range(self.nFinanceWidgetCount - 1, -1, -1):
			screen.deleteWidget(self.FINANCE_WIDGET_ID + str(i))
		self.nFinanceWidgetCount = 0

	def getOverviewHeaderLabel(self, szText):
		# <!-- custom: draw-time label wrapping for Domestic Overview table headers; this is the effective upscale point for header text. (GPT-5.3-Codex) -->
		return sasFontTagLabel + szText + SAS_FONT_TAG_CLOSE

	def getSignedText(self, iValue, bShowZeroAsDash):
		if iValue > 0:
			return self.COLOR_POSITIVE_PREFIX + u"+" + unicode(iValue) + self.COLOR_REVERT_SUFFIX
		if iValue < 0:
			return self.COLOR_NEGATIVE_PREFIX + unicode(iValue) + self.COLOR_REVERT_SUFFIX
		if bShowZeroAsDash:
			return u"-"
		return unicode(iValue)

	def getSignedModifierText(self, iValue):
		if iValue == 0:
			return u""
		return self.getSignedText(iValue, False)

	def getInvertedSignedModifierText(self, iValue):
		if iValue == 0:
			return u""
		# <!-- custom: some city modifiers are penalties, so positive values should read as bad and negative values as good. (GPT-5.5) -->
		if iValue > 0:
			return self.COLOR_NEGATIVE_PREFIX + u"+" + unicode(iValue) + self.COLOR_REVERT_SUFFIX
		return self.COLOR_POSITIVE_PREFIX + unicode(iValue) + self.COLOR_REVERT_SUFFIX

	def getBlankZeroText(self, iValue):
		if iValue == 0:
			return u""
		return unicode(iValue)

	def getOverviewReligionColumnWidth(self):
		iMaxCityReligions = 0
		player = gc.getPlayer(self.iActivePlayer)
		(pLoopCity, iter) = player.firstCity(false)
		while(pLoopCity):
			iCityReligions = 0
			for iRel in range(gc.getNumReligionInfos()):
				if pLoopCity.isHasReligion(iRel):
					iCityReligions += 1
			iMaxCityReligions = max(iMaxCityReligions, iCityReligions)
			(pLoopCity, iter) = player.nextCity(iter, false)
		# <!-- custom: ~55px fits roughly three religion glyphs with the current upscaled Domestic Advisor font; size linearly from the inspected player's widest city, capped by XML religion count, and borrow only from Producing where clipped text is acceptable. (GPT-5.5) -->
		iReligionGlyphs = min(gc.getNumReligionInfos(), iMaxCityReligions)
		return max(self.OVERVIEW_RELIGION_BASE_COL_WIDTH, (self.OVERVIEW_RELIGION_BASE_COL_WIDTH * iReligionGlyphs + 2) / 3)

	def getOverviewProducingColumnWidth(self):
		return self.OVERVIEW_PRODUCING_BASE_COL_WIDTH - (self.getOverviewReligionColumnWidth() - self.OVERVIEW_RELIGION_BASE_COL_WIDTH)

	def getOverviewCorporationColumnWidth(self):
		iMaxCityCorporations = 0
		player = gc.getPlayer(self.iActivePlayer)
		(pLoopCity, iter) = player.firstCity(false)
		while(pLoopCity):
			iCityCorporations = 0
			for iCorp in range(gc.getNumCorporationInfos()):
				if pLoopCity.isHeadquartersByType(iCorp) or pLoopCity.isActiveCorporation(iCorp):
					iCityCorporations += 1
			iMaxCityCorporations = max(iMaxCityCorporations, iCityCorporations)
			(pLoopCity, iter) = player.nextCity(iter, false)
		# <!-- custom: mirror religion-column sizing for corporations; Overview 4 has spare width, so this does not need to borrow from another column. (GPT-5.5) -->
		iCorporationGlyphs = min(gc.getNumCorporationInfos(), iMaxCityCorporations)
		return max(self.OVERVIEW_CORPORATION_BASE_COL_WIDTH, (self.OVERVIEW_CORPORATION_BASE_COL_WIDTH * iCorporationGlyphs + 2) / 3)

	def setupOverviewTable(self, szTable, iColumns):
		screen = self.getScreen()
		screen.addTableControlGFC(szTable, iColumns, self.nTableX, self.nTableY, self.nTableWidth, self.nTableHeight, True, False, 24, 24, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSelect(szTable, True)
		screen.enableSort(szTable)
		screen.setStyle(szTable, "Table_StandardCiv_Style")
		return screen

	def getCityReligionText(self, pCity):
		szRels = u""
		for iRel in range(gc.getNumReligionInfos()):
			if pCity.isHasReligion(iRel):
				if pCity.isHolyCityByType(iRel):
					szRels += (u"%c" % gc.getReligionInfo(iRel).getHolyCityChar())
				else:
					szRels += (u"%c" % gc.getReligionInfo(iRel).getChar())
		return szRels

	def getCityCorporationText(self, pCity):
		szCorps = u""
		for iCorp in range(gc.getNumCorporationInfos()):
			if pCity.isHeadquartersByType(iCorp):
				szCorps += (u"%c" % gc.getCorporationInfo(iCorp).getHeadquarterChar())
			elif pCity.isActiveCorporation(iCorp):
				szCorps += (u"%c" % gc.getCorporationInfo(iCorp).getChar())
		return szCorps

	# headers...
	def drawHeaders1( self ):
		self.initText()

		# Get the screen and the player
		screen = CyGInterfaceScreen( "DomesticAdvisor", CvScreenEnums.DOMESTIC_ADVISOR )

		# Zoom to City
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 0, "", (24 * self.nTableWidth) / self.nNormalizedTableWidth )

		# Name Column (advc.193: Font size increased; was 2.)
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 1, self.getOverviewHeaderLabel(self.HEADER_NAME), (self.OVERVIEW_CITY_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )

		# Population Column
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 2, self.getOverviewHeaderLabel(self.HEADER_POPULATION), (35 * self.nTableWidth) / self.nNormalizedTableWidth )

		# <!-- custom: keep religions in a compact aligned column; appending glyphs to city names was harder to scan.
		# Corporations stay on Overview 3 because they matter less often and would crowd Overview 1. (GPT-5.5) -->
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 3, self.getOverviewHeaderLabel(self.HEADER_RELIGIONS), (self.getOverviewReligionColumnWidth() * self.nTableWidth) / self.nNormalizedTableWidth )

		# Happiness Column  advc.ctr: width was 40
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 4, self.getOverviewHeaderLabel(self.HEADER_HAPPINESS), (33 * self.nTableWidth) / self.nNormalizedTableWidth )

		# Health Column  advc.ctr: width was 40
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 5, self.getOverviewHeaderLabel(self.HEADER_HEALTH), (33 * self.nTableWidth) / self.nNormalizedTableWidth )

		# Food Column
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 6, self.getOverviewHeaderLabel(self.HEADER_FOOD), (40 * self.nTableWidth) / self.nNormalizedTableWidth )

		# Production Column
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 7, self.getOverviewHeaderLabel(self.HEADER_PRODUCTION), (40 * self.nTableWidth) / self.nNormalizedTableWidth )

		# Gold Column
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 8, self.getOverviewHeaderLabel(self.HEADER_GOLD), (40 * self.nTableWidth) / self.nNormalizedTableWidth )

		# Research Column
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 9, self.getOverviewHeaderLabel(self.HEADER_RESEARCH), (40 * self.nTableWidth) / self.nNormalizedTableWidth )

		# Espionage Column
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 10, self.getOverviewHeaderLabel(self.HEADER_ESPIONAGE), (40 * self.nTableWidth) / self.nNormalizedTableWidth )

		# Culture Column
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 11, self.getOverviewHeaderLabel(self.HEADER_CULTURE), (70 * self.nTableWidth) / self.nNormalizedTableWidth )

		# Great Person Column
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 12, self.getOverviewHeaderLabel(self.HEADER_GREAT_PERSON), (70 * self.nTableWidth) / self.nNormalizedTableWidth )

		# Trade Column
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 13, self.getOverviewHeaderLabel(self.HEADER_TRADE), (35 * self.nTableWidth) / self.nNormalizedTableWidth )

		# Maintenance Column  advc.ctr: width was 40
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 14, self.getOverviewHeaderLabel(self.HEADER_MAINTENANCE), (33 * self.nTableWidth) / self.nNormalizedTableWidth )

		# Production Column (advc.193: Font size increased; was 2.)
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 15, self.getOverviewHeaderLabel(self.HEADER_PRODUCING), (self.getOverviewProducingColumnWidth() * self.nTableWidth) / self.nNormalizedTableWidth )

		# <!-- custom: keep production turns in a short sortable column so long production names can clip without hiding the more important timer at the tail. (GPT-5.5) -->
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 16, self.getOverviewHeaderLabel(u"T"), (35 * self.nTableWidth) / self.nNormalizedTableWidth )

		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 17, self.getOverviewHeaderLabel(self.HEADER_DEFENSE + u"%"), (45 * self.nTableWidth) / self.nNormalizedTableWidth )

		# Garrison Column
		# advc.004: Use STRENGTH_CHAR instead of DEFENSE_CHAR
		screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 18, self.getOverviewHeaderLabel(self.HEADER_GARRISON), (35 * self.nTableWidth) / self.nNormalizedTableWidth )

		# Liberate Column
		#screen.setTableColumnHeader( self.TABLE_OVERVIEW1, 19, "", (25 * self.nTableWidth) / self.nNormalizedTableWidth )
		# <advc.ctr> Liberation now shown on "Cities" tab. Instead show revolt probability.
		screen.setTableColumnHeader(self.TABLE_OVERVIEW1, 19, self.getOverviewHeaderLabel(self.HEADER_REVOLT + u"%"), (45 * self.nTableWidth) / self.nNormalizedTableWidth)

	# Function to draw the contents of the cityList passed in
	def drawContents (self):
		self.drawTabs()
		if self.iPage == self.PAGE_FINANCE:
			self.drawFinanceContents()
			return
		if self.iPage == self.PAGE_OVERVIEW2:
			self.drawOverview2Contents()
			return
		if self.iPage == self.PAGE_OVERVIEW3:
			self.drawOverview3Contents()
			return
		if self.iPage == self.PAGE_OVERVIEW4:
			self.drawOverview4Contents()
			return
		self.drawOverview1Contents()

	def drawOverview1Contents(self):
		self.clearFinanceWidgets()
		screen = CyGInterfaceScreen( "DomesticAdvisor", CvScreenEnums.DOMESTIC_ADVISOR )
		if self.bOverview2TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW2)
			self.bOverview2TableCreated = False
		if self.bOverview3TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW3)
			self.bOverview3TableCreated = False
		if self.bOverview4TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW4)
			self.bOverview4TableCreated = False

		# Get the screen and the player
		player = gc.getPlayer(self.iActivePlayer)
		if self.bOverview1TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW1)
			self.bOverview1TableCreated = False

		screen = self.setupOverviewTable(self.TABLE_OVERVIEW1, 20)
		self.bOverview1TableCreated = True

		# Loop through the cities
		i = 0
		(pLoopCity, iter) = player.firstCity(false)
		while(pLoopCity):
			screen.appendTableRow( self.TABLE_OVERVIEW1 )
			if (pLoopCity.getName() in self.listSelectedCities):
				screen.selectRow( self.TABLE_OVERVIEW1, i, True )
			self.updateTable1(pLoopCity, i)
			i += 1
			(pLoopCity, iter) = player.nextCity(iter, false)

		self.drawHeaders1()

		if isAdvisorReadOnlyPerspective(self.iActivePlayer):
			self.hideSpecialists()
		else:
			self.drawSpecialists()
			self.updateAppropriateCitySelection()

		CyInterface().setDirty(InterfaceDirtyBits.Domestic_Advisor_DIRTY_BIT, true)

	def drawFinanceContents(self):
		self.clearOverviewWidgets()
		self.clearFinanceWidgets()
		self.configureFinanceLayout()
		self.iFinanceActiveLeader = self.iActivePlayer
		self.drawFinanceTab()
		screen = self.getScreen()
		screen.setLabel("DomesticTitleHeader", "Background", self.SCREEN_TITLE, CvUtil.FONT_CENTER_JUSTIFY, self.X_TITLE, self.Y_TITLE, -0.1, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def drawFinanceTab(self):
		screen = self.getScreen()
		player = gc.getPlayer(self.iFinanceActiveLeader)
		# <!-- custom: this is tab content inside Domestic Advisor; standalone EconomicsAdvisor-only chrome (separate header text, debug dropdown, own screen widgets) is intentionally not migrated. (GPT-5.3-Codex) -->

		X_LEFT_PANEL = self.FIN_X_LEFT_PANEL
		X_MIDDLE_PANEL = self.FIN_X_MIDDLE_PANEL
		X_RIGHT_PANEL = self.FIN_X_RIGHT_PANEL
		Y_TOP_PANEL = self.FIN_Y_TOP_PANEL
		H_TOP_PANEL = self.FIN_H_TOP_PANEL
		Y_LOCATION = self.FIN_Y_LOCATION
		PANE_WIDTH = self.FIN_PANE_WIDTH
		PANE_HEIGHT = self.FIN_PANE_HEIGHT
		TEXT_MARGIN = self.FINANCE_TEXT_MARGIN
		Y_SPACING = self.FINANCE_Y_SPACING
		Z_CONTROLS = self.FINANCE_Z_CONTROLS
		DZ = self.FINANCE_DZ

		# K-Mod - I've changed these costs to include inflation.
		# advc.001 (comment): CvPlayer::calculateInflatedCosts applies inflation (and rounding) only once in the end.
		# Therefore, the inflated and rounded subtotals here often don't add up correctly. Not easy to fix.
		inflationFactor = 100 + player.calculateInflationRate()
		totalUnitCost = (player.calculateUnitCost() * inflationFactor + 50) / 100
		totalUnitSupply = (player.calculateUnitSupply() * inflationFactor + 50) / 100
		totalMaintenance = (player.getTotalMaintenance() * inflationFactor + 50) / 100
		totalCivicUpkeep = (player.getCivicUpkeep([], False) * inflationFactor + 50) / 100

		totalInflatedCosts = player.calculateInflatedCosts()
		goldCommerce = player.getCommerceRate(CommerceTypes.COMMERCE_GOLD)
		gold = player.getGold()
		goldFromCivs = player.getGoldPerTurn()
		goldPerTurn = player.calculateGoldRate()

		szTreasuryPanel = self.getNextFinanceWidgetName()
		screen.addPanel(szTreasuryPanel, u"", "", True, True, X_LEFT_PANEL, Y_TOP_PANEL, X_RIGHT_PANEL + PANE_WIDTH - X_LEFT_PANEL, H_TOP_PANEL, PanelStyles.PANEL_STYLE_MAIN )
		szText = localText.getText("TXT_KEY_FINANCIAL_ADVISOR_TREASURY", (gold, )).upper()
		if gold < 0:
			if goldPerTurn != 0:
				if gold + goldPerTurn >= 0:
					szText += BugUtil.getText("TXT_KEY_MISC_POS_GOLD_PER_TURN", goldPerTurn)
				elif goldPerTurn >= 0:
					szText += BugUtil.getText("TXT_KEY_MISC_POS_WARNING_GOLD_PER_TURN", goldPerTurn)
				else:
					szText += BugUtil.getText("TXT_KEY_MISC_NEG_GOLD_PER_TURN", goldPerTurn)
		else:
			if goldPerTurn != 0:
				if goldPerTurn >= 0:
					szText += BugUtil.getText("TXT_KEY_MISC_POS_GOLD_PER_TURN", goldPerTurn)
				elif gold + goldPerTurn >= 0:
					szText += BugUtil.getText("TXT_KEY_MISC_NEG_WARNING_GOLD_PER_TURN", goldPerTurn)
				else:
					szText += BugUtil.getText("TXT_KEY_MISC_NEG_GOLD_PER_TURN", goldPerTurn)
		screen.setLabel(self.getNextFinanceWidgetName(), szTreasuryPanel, sasFontTagTitle + szText + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, (X_LEFT_PANEL + PANE_WIDTH + X_RIGHT_PANEL) / 2, Y_TOP_PANEL + H_TOP_PANEL / 2 - Y_SPACING / 2, Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_HELP_FINANCE_GOLD_RESERVE, -1, -1 )

		szCommercePanel = self.getNextFinanceWidgetName()
		screen.addPanel(szCommercePanel, u"", "", True, True, X_LEFT_PANEL, Y_LOCATION, PANE_WIDTH, PANE_HEIGHT, PanelStyles.PANEL_STYLE_MAIN )
		screen.setLabel(self.getNextFinanceWidgetName(), "Background",  sasFontTagLabel + self.TEXT_FIN_COMMERCE.upper() + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH / 2, Y_LOCATION + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		szIncomePanel = self.getNextFinanceWidgetName()
		screen.addPanel(szIncomePanel, u"", "", True, True, X_MIDDLE_PANEL, Y_LOCATION, PANE_WIDTH, PANE_HEIGHT, PanelStyles.PANEL_STYLE_MAIN )
		screen.setLabel(self.getNextFinanceWidgetName(), "Background",  sasFontTagLabel + self.TEXT_FIN_INCOME_HEADER.upper() + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, X_MIDDLE_PANEL + PANE_WIDTH / 2, Y_LOCATION + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		szExpensePanel = self.getNextFinanceWidgetName()
		screen.addPanel(szExpensePanel, u"", "", True, True, X_RIGHT_PANEL, Y_LOCATION, PANE_WIDTH, PANE_HEIGHT, PanelStyles.PANEL_STYLE_MAIN )
		screen.setLabel(self.getNextFinanceWidgetName(), "Background",  sasFontTagLabel + self.TEXT_FIN_EXPENSES_HEADER.upper() + SAS_FONT_TAG_CLOSE, CvUtil.FONT_CENTER_JUSTIFY, X_RIGHT_PANEL + PANE_WIDTH / 2, Y_LOCATION + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		# Commerce
		yLocation = Y_LOCATION
		iCommerce = 0
		iWorkedTileCount = 0
		iWorkedTiles = 0
		bMultipliers = False # advc.004: in Bureaucracy (or sth. similar)?
		for city in PlayerUtil.playerCities(player):
			if not city.isDisorder():
				for i in range(gc.getNUM_CITY_PLOTS()):
					plot = city.getCityIndexPlot(i)
					if plot and not plot.isNone() and plot.hasYield():
						if city.isWorkingPlot(plot):
							iWorkedTileCount += 1
							iWorkedTiles += plot.getYield(YieldTypes.YIELD_COMMERCE)
				if city.getBaseYieldRateModifier(YieldTypes.YIELD_COMMERCE, 0) != 100:
					bMultipliers = True

		yLocation += 1.5 * Y_SPACING
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + localText.getText("TXT_KEY_CONCEPT_WORKED_TILES", (iWorkedTileCount,)) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(iWorkedTiles) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		iCommerce += iWorkedTiles

		iDomesticTrade, _, iForeignTrade, _ = TradeUtil.calculateTradeRoutes(player)

		# advc.086: Always show this to avoid confusion
		if TradeUtil.isFractionalTrade():
			iDomesticTrade //= 100
		yLocation += Y_SPACING
		szDomesticTrade = self.TEXT_FIN_DOMESTIC_TRADE
		# advc.086: Call it just "Trade" if there is no foreign trade to show
		if iForeignTrade <= 0:
			szDomesticTrade = self.TEXT_FIN_TRADE
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + szDomesticTrade + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_DOMESTIC_TRADE, self.iFinanceActiveLeader, 1)
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(iDomesticTrade) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_DOMESTIC_TRADE, self.iFinanceActiveLeader, 1)
		iCommerce += iDomesticTrade

		if iForeignTrade > 0:
			if TradeUtil.isFractionalTrade():
				iForeignTrade //= 100
			yLocation += Y_SPACING
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_FOREIGN_TRADE + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_FOREIGN_TRADE, self.iFinanceActiveLeader, 1)
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(iForeignTrade) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_FOREIGN_TRADE, self.iFinanceActiveLeader, 1)
			iCommerce += iForeignTrade

		iCorporations = 0
		for city in PlayerUtil.playerCities(player):
			if not city.isDisorder():
				iCorporations += city.getCorporationYield(YieldTypes.YIELD_COMMERCE)

		if iCorporations > 0:
			yLocation += Y_SPACING
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_CORPORATIONS + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(iCorporations) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
			iCommerce += iCorporations

		iSpecialists = 0
		for city in PlayerUtil.playerCities(player):
			if not city.isDisorder():
				for eSpec in range(gc.getNumSpecialistInfos()):
					iSpecialists += player.specialistYield(eSpec, YieldTypes.YIELD_COMMERCE) * (city.getSpecialistCount(eSpec) + city.getFreeSpecialistCount(eSpec))

		if iSpecialists > 0:
			yLocation += Y_SPACING
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_SPECIALISTS + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(iSpecialists) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
			iCommerce += iSpecialists

		# advc.001: "Current" total yield - i.e. excluding cities in disorder
		iTotalCommerce = player.calculateCurrentTotalYield(YieldTypes.YIELD_COMMERCE)
		iBuildings = iTotalCommerce - iCommerce
		if iBuildings != 0:
			yLocation += Y_SPACING
			suLabel = sasFontTagLabel + self.TEXT_FIN_BUILDINGS
			if bMultipliers:
				suLabel += u", " + self.TEXT_FIN_CIVICS_MULTIPLIERS
			suLabel += SAS_FONT_TAG_CLOSE
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", suLabel, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(iBuildings) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
			iCommerce += iBuildings

		yLocation += 1.5 * Y_SPACING
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_BUG_COMMERCE + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(iCommerce) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		yLocation += 0.5 * Y_SPACING
		if self.iCOMMERCE_PERCENT_CHANGE_INCREMENTS is None:
			self.iCOMMERCE_PERCENT_CHANGE_INCREMENTS = gc.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS")
		for iI in range(CommerceTypes.NUM_COMMERCE_TYPES):
			eCommerce = (iI + 1) % CommerceTypes.NUM_COMMERCE_TYPES
			if player.isCommerceFlexible(eCommerce):
				yLocation += Y_SPACING
				screen.setButtonGFC(self.getNextFinanceWidgetName(), u"", "", X_LEFT_PANEL + TEXT_MARGIN, int(yLocation) + TEXT_MARGIN, 20, 20, WidgetTypes.WIDGET_CHANGE_PERCENT, eCommerce, self.iCOMMERCE_PERCENT_CHANGE_INCREMENTS, ButtonStyles.BUTTON_STYLE_CITY_PLUS )
				screen.setButtonGFC(self.getNextFinanceWidgetName(), u"", "", X_LEFT_PANEL + TEXT_MARGIN + 24, int(yLocation) + TEXT_MARGIN, 20, 20, WidgetTypes.WIDGET_CHANGE_PERCENT, eCommerce, -self.iCOMMERCE_PERCENT_CHANGE_INCREMENTS, ButtonStyles.BUTTON_STYLE_CITY_MINUS )
				szText = sasFontTagLabel + gc.getCommerceInfo(eCommerce).getDescription() + u" (" + unicode(player.getCommercePercent(eCommerce)) + u"%)" + SAS_FONT_TAG_CLOSE
				screen.setLabel(self.getNextFinanceWidgetName(), "Background",  szText, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + TEXT_MARGIN + 50, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
				szRate = sasFontTagLabel + unicode(player.getCommerceRate(CommerceTypes(eCommerce))) + SAS_FONT_TAG_CLOSE
				screen.setLabel(self.getNextFinanceWidgetName(), "Background", szRate, CvUtil.FONT_RIGHT_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# K-Mod. Show gold rate if it hasn't been shown already
		if not player.isCommerceFlexible(CommerceTypes.COMMERCE_GOLD):
			yLocation += Y_SPACING
			szText = sasFontTagLabel + gc.getCommerceInfo(CommerceTypes.COMMERCE_GOLD).getDescription() + u" (" + unicode(player.getCommercePercent(CommerceTypes.COMMERCE_GOLD)) + u"%)" + SAS_FONT_TAG_CLOSE
			screen.setLabel(self.getNextFinanceWidgetName(), "Background",  szText, CvUtil.FONT_LEFT_JUSTIFY, X_LEFT_PANEL + TEXT_MARGIN + 50, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
			szCommerce = sasFontTagLabel + unicode(goldCommerce) + SAS_FONT_TAG_CLOSE
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", szCommerce, CvUtil.FONT_RIGHT_JUSTIFY, X_LEFT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# Income
		yLocation = Y_LOCATION
		iTaxRate = player.getCommercePercent(CommerceTypes.COMMERCE_GOLD)
		multipliers = []
		for eBldg in range(gc.getNumBuildingInfos()):
			info = gc.getBuildingInfo(eBldg)
			iMultiplier = info.getCommerceModifier(CommerceTypes.COMMERCE_GOLD)
			if iMultiplier > 0:
				multipliers.append([eBldg, iMultiplier, 0, 0.0])

		iBuildingCount = 0
		iHeadquartersCount = 0
		iShrinesCount = 0
		fTaxes = 0.0
		fBuildings = 0.0
		fHeadquarters = 0.0
		fShrines = 0.0
		fCorporations = 0.0
		fSpecialists = 0.0
		iWealthCount = 0
		fWealth = 0.0
		eWealth = self.PROCESS_WEALTH
		for city in PlayerUtil.playerCities(player):
			if not city.isDisorder():
				fCityTaxes = city.getYieldRate(YieldTypes.YIELD_COMMERCE) * iTaxRate / 100.0
				fTaxes += fCityTaxes
				fCityBuildings = 0.0
				fCityHeadquarters = 0.0
				fCityShrines = 0.0
				for eBldg in range(gc.getNumBuildingInfos()):
					iCount = city.getNumRealBuilding(eBldg)
					if iCount > 0:
						iBuildingGold = city.getBuildingCommerceByBuilding(CommerceTypes.COMMERCE_GOLD, eBldg)
						if iBuildingGold > 0:
							info = gc.getBuildingInfo(eBldg)
							if info.getFoundsCorporation() != -1:
								fCityHeadquarters += iBuildingGold
								iHeadquartersCount += 1
							elif info.getGlobalReligionCommerce() != -1:
								fCityShrines += iBuildingGold
								iShrinesCount += 1
							else:
								fCityBuildings += iBuildingGold
								iBuildingCount += iCount
				fBuildings += fCityBuildings
				fHeadquarters += fCityHeadquarters
				fShrines += fCityShrines
				fCityCorporations = city.getCorporationCommerce(CommerceTypes.COMMERCE_GOLD)
				fCorporations += fCityCorporations
				fCitySpecialists = city.getSpecialistCommerce(CommerceTypes.COMMERCE_GOLD)
				fSpecialists += fCitySpecialists
				fCityWealth = 0.0
				if city.isProductionProcess() and city.getProductionProcess() == eWealth:
					fCityWealth = city.getProductionToCommerceModifier(CommerceTypes.COMMERCE_GOLD) * city.getYieldRate(YieldTypes.YIELD_PRODUCTION) / 100.0
					fWealth += fCityWealth
					iWealthCount += 1
				# buildings don't multiply wealth
				fCityTotel = fCityTaxes + fCityBuildings + fCityHeadquarters + fCityCorporations + fCitySpecialists
				for entry in multipliers:
					eBldg, iMultiplier, _, _ = entry
					iCount = city.getNumRealBuilding(eBldg)
					if iCount > 0:
						entry[2] += iCount
						entry[3] += iCount * fCityTotel * iMultiplier / 100.0

		# K-Mod, karadoc
		# The 'total minus taxes' was wrong. We don't need to use that anyway
		# I've changed the 'taxes' output to use fTaxes instead of goldcommerce - totalminustaxes
		yLocation += 1.5 * Y_SPACING
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_TAXES + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_MIDDLE_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(int(fTaxes)) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_MIDDLE_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		if fBuildings > 0.0:
			yLocation += Y_SPACING
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_BUILDINGS + " (%d)" % iBuildingCount + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_MIDDLE_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(int(fBuildings)) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_MIDDLE_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		if fHeadquarters > 0.0:
			yLocation += Y_SPACING
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_HQ + " (%d)" % iHeadquartersCount + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_MIDDLE_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(int(fHeadquarters)) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_MIDDLE_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		if fCorporations > 0.0:
			yLocation += Y_SPACING
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_CORPORATIONS + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_MIDDLE_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(int(fCorporations)) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_MIDDLE_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		if fShrines > 0.0:
			yLocation += Y_SPACING
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_SHRINES + " (%d)" % iShrinesCount + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_MIDDLE_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(int(fShrines)) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_MIDDLE_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		if fSpecialists > 0.0:
			yLocation += Y_SPACING
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_SPECIALISTS + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_MIDDLE_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_SPECIALISTS, self.iFinanceActiveLeader, 1)
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(int(fSpecialists)) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_MIDDLE_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_SPECIALISTS, self.iFinanceActiveLeader, 1)

		for eBldg, iMultiplier, iCount, fGold in multipliers:
			if iCount > 0 and fGold > 0.0:
				fAverage = fGold / iCount
				szDescription = gc.getBuildingInfo(eBldg).getDescription() + u" " + localText.getText("TXT_KEY_BUG_FINANCIAL_ADVISOR_BUILDING_COUNT_AVERAGE", (iCount, BugUtil.formatFloat(fAverage, 2)))
				yLocation += Y_SPACING
				screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + szDescription + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_MIDDLE_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
				screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(int(fGold)) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_MIDDLE_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		if fWealth > 0.0 and iWealthCount > 0:
			yLocation += Y_SPACING
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_WEALTH + " (%d)" % iWealthCount + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_MIDDLE_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(int(fWealth)) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_MIDDLE_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		iIncome = goldCommerce
		if goldFromCivs > 0:
			yLocation += Y_SPACING
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_PER_TURN + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_MIDDLE_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_FOREIGN_INCOME, self.iFinanceActiveLeader, 1)
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(goldFromCivs) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_MIDDLE_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_FOREIGN_INCOME, self.iFinanceActiveLeader, 1)
			iIncome += goldFromCivs

		yLocation += 1.5 * Y_SPACING
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_INCOME + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_MIDDLE_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(iIncome) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_MIDDLE_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		# Expenses
		yLocation = Y_LOCATION
		yLocation += 1.5 * Y_SPACING
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_UNIT_COST + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_RIGHT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_UNIT_COST, self.iFinanceActiveLeader, 1)
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(totalUnitCost) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_RIGHT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_UNIT_COST, self.iFinanceActiveLeader, 1)
		yLocation += Y_SPACING
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_UNIT_SUPPLY + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_RIGHT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_AWAY_SUPPLY, self.iFinanceActiveLeader, 1)
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(totalUnitSupply) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_RIGHT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_AWAY_SUPPLY, self.iFinanceActiveLeader, 1)
		yLocation += Y_SPACING
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_MAINTENANCE + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_RIGHT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_CITY_MAINT, self.iFinanceActiveLeader, 1)
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(totalMaintenance) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_RIGHT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_CITY_MAINT, self.iFinanceActiveLeader, 1)
		yLocation += Y_SPACING
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_CIVICS + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_RIGHT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_CIVIC_UPKEEP, self.iFinanceActiveLeader, 1)
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(totalCivicUpkeep) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_RIGHT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_CIVIC_UPKEEP, self.iFinanceActiveLeader, 1)

		if goldFromCivs < 0:
			yLocation += Y_SPACING
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_COST_PER_TURN + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_RIGHT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_FOREIGN_INCOME, self.iFinanceActiveLeader, 1)
			screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(-goldFromCivs) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_RIGHT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_FOREIGN_INCOME, self.iFinanceActiveLeader, 1)

		# advc: Unmarked K-Mod 1.45 change; inflation now already included
		yLocation += 1.5 * Y_SPACING
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + self.TEXT_FIN_EXPENSES + SAS_FONT_TAG_CLOSE, CvUtil.FONT_LEFT_JUSTIFY, X_RIGHT_PANEL + TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		screen.setLabel(self.getNextFinanceWidgetName(), "Background", sasFontTagLabel + unicode(totalInflatedCosts) + SAS_FONT_TAG_CLOSE, CvUtil.FONT_RIGHT_JUSTIFY, X_RIGHT_PANEL + PANE_WIDTH - TEXT_MARGIN, yLocation + TEXT_MARGIN, Z_CONTROLS + DZ, FontTypes.GAME_FONT, WidgetTypes.WIDGET_HELP_FINANCE_INFLATED_COSTS, self.iFinanceActiveLeader, 1 )

	def updateTable1(self, pLoopCity, i):

		screen = CyGInterfaceScreen( "DomesticAdvisor", CvScreenEnums.DOMESTIC_ADVISOR )

		screen.setTableText( self.TABLE_OVERVIEW1, 0, i, "", self.ART_CITY_SELECTION_BUTTON, WidgetTypes.WIDGET_ZOOM_CITY, pLoopCity.getOwner(), pLoopCity.getID(), CvUtil.FONT_LEFT_JUSTIFY)

		# <advc.193>
		# <!-- custom: preserve base AdvCiv per-row font-tag reuse, but use shared LABEL (not BODY) in AdvCiv-SAS for readability. (GPT-5.3-Codex) -->
		# (Uses of these tags in the code below aren't tagged w/ comments)
		szFontTagOpen = sasFontTagLabel
		szFontTagClose = SAS_FONT_TAG_CLOSE
		# </advc.193>

		szName = pLoopCity.getName()
		if pLoopCity.isCapital():
			szName += (u"%c" % CyGame().getSymbolID(FontSymbols.STAR_CHAR))
		elif pLoopCity.isGovernmentCenter():
			szName += (u"%c" % CyGame().getSymbolID(FontSymbols.SILVER_STAR_CHAR))

		# City name...
		#WidgetTypes.WIDGET_GENERAL, -1, -1,
		# advc.186b: BULL attaches this to the zoom button. I like it better on the city name b/c, that way, it doesn't obscure the button that the player may want to click.
		screen.setTableText( self.TABLE_OVERVIEW1, 1, i, szFontTagOpen + szName + szFontTagClose, "", WidgetTypes.WIDGET_EXAMINE_CITY, pLoopCity.getOwner(), pLoopCity.getID(), CvUtil.FONT_LEFT_JUSTIFY )

		# Population
		screen.setTableInt( self.TABLE_OVERVIEW1, 2, i, szFontTagOpen + unicode(pLoopCity.getPopulation()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		screen.setTableText( self.TABLE_OVERVIEW1, 3, i, szFontTagOpen + self.getCityReligionText(pLoopCity) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		# Happiness...
		iNetHappy = pLoopCity.happyLevel() - pLoopCity.unhappyLevel(0)
		szText = unicode(iNetHappy)
		if iNetHappy > 0:
			szText = self.COLOR_POSITIVE_PREFIX + szText + self.COLOR_REVERT_SUFFIX
		elif iNetHappy < 0:
			szText = self.COLOR_NEGATIVE_PREFIX + szText + self.COLOR_REVERT_SUFFIX
		screen.setTableInt( self.TABLE_OVERVIEW1, 4, i, szFontTagOpen + szText + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		# Health...
		iNetHealth = pLoopCity.goodHealth() - pLoopCity.badHealth(0)
		szText = unicode(iNetHealth)
		if iNetHealth > 0:
			szText = self.COLOR_POSITIVE_PREFIX + szText + self.COLOR_REVERT_SUFFIX
		elif iNetHealth < 0:
			szText = self.COLOR_NEGATIVE_PREFIX + szText + self.COLOR_REVERT_SUFFIX
		screen.setTableInt( self.TABLE_OVERVIEW1, 5, i, szFontTagOpen + szText + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		# Food status...
		iNetFood = pLoopCity.foodDifference(true)
		szText = unicode(iNetFood)
		if iNetFood > 0:
			szText = self.COLOR_POSITIVE_PREFIX + szText + self.COLOR_REVERT_SUFFIX
		elif iNetFood < 0:
			szText = self.COLOR_NEGATIVE_PREFIX + szText + self.COLOR_REVERT_SUFFIX
		elif pLoopCity.isFoodProduction():
			# <!-- custom: leave food-to-production cities blank instead of showing 0 net food; they still sort with the zero-surplus group. (GPT-5.5) -->
			szText = u""
		screen.setTableInt( self.TABLE_OVERVIEW1, 6, i, szFontTagOpen + szText + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		# Production status...
		screen.setTableInt( self.TABLE_OVERVIEW1, 7, i, szFontTagOpen+ unicode(pLoopCity.getYieldRate(YieldTypes.YIELD_PRODUCTION)) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		# Gold status...
		screen.setTableInt( self.TABLE_OVERVIEW1, 8, i, szFontTagOpen + unicode(pLoopCity.getCommerceRate(CommerceTypes.COMMERCE_GOLD)) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		# Science rate...
		screen.setTableInt( self.TABLE_OVERVIEW1, 9, i, szFontTagOpen + unicode(pLoopCity.getCommerceRate(CommerceTypes.COMMERCE_RESEARCH)) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		# Espionage rate...
		screen.setTableInt( self.TABLE_OVERVIEW1, 10, i, szFontTagOpen + unicode(pLoopCity.getCommerceRate(CommerceTypes.COMMERCE_ESPIONAGE)) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		# Culture status...
		szCulture = unicode(pLoopCity.getCommerceRate(CommerceTypes.COMMERCE_CULTURE))
		iCultureTimes100 = pLoopCity.getCultureTimes100(self.iActivePlayer)
		iCultureRateTimes100 = pLoopCity.getCommerceRateTimes100(CommerceTypes.COMMERCE_CULTURE)
		if iCultureRateTimes100 > 0:
			iCultureLeftTimes100 = 100 * pLoopCity.getCultureThreshold() - iCultureTimes100
			if iCultureLeftTimes100 > 0:
				szCulture += u" (" + unicode((iCultureLeftTimes100  + iCultureRateTimes100 - 1) / iCultureRateTimes100) + u")"

		screen.setTableInt( self.TABLE_OVERVIEW1, 11, i, szFontTagOpen + szCulture + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		# Great Person
		iGreatPersonRate = pLoopCity.getGreatPeopleRate()
		szGreatPerson = unicode(iGreatPersonRate)
		if iGreatPersonRate > 0:
			iGPPLeft = gc.getPlayer(self.iActivePlayer).greatPeopleThreshold(false) - pLoopCity.getGreatPeopleProgress()
			if iGPPLeft > 0:
				iTurnsLeft = iGPPLeft / pLoopCity.getGreatPeopleRate()
				if iTurnsLeft * pLoopCity.getGreatPeopleRate() <  iGPPLeft:
					iTurnsLeft += 1
				szGreatPerson += u" (" + unicode(iTurnsLeft) + u")"

		screen.setTableInt( self.TABLE_OVERVIEW1, 12, i, szFontTagOpen + szGreatPerson + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		# Trade
		screen.setTableInt( self.TABLE_OVERVIEW1, 13, i, szFontTagOpen + unicode(pLoopCity.getTradeYield(YieldTypes.YIELD_COMMERCE)) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		# Maintenance...
		#iMaintenance = pLoopCity.getMaintenance()
		# advc.004: Based on K-Mod code in CvMainInterface.py
		iMaintenance = pLoopCity.getMaintenanceTimes100() * (100+gc.getPlayer(pLoopCity.getOwner()).calculateInflationRate()) // 10000
		screen.setTableInt( self.TABLE_OVERVIEW1, 14, i, szFontTagOpen + unicode(iMaintenance) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		# Producing
		szProducing = pLoopCity.getProductionName()
		iProductionTurns = pLoopCity.getGeneralProductionTurnsLeft()

		# <!-- custom: also show the button of the current thing we are building. Credit: Gemini 3 Pro. (GPT-5.2-Codex (summarized)) -->
		# add the button (icon) to the "Producing" column.
		# Get the button art for the current item
		szButton = ""
		iWidget = WidgetTypes.WIDGET_GENERAL
		iData1 = -1
		iData2 = -1

		if pLoopCity.isProductionUnit():
			iUnit = pLoopCity.getProductionUnit()
			szButton = gc.getUnitInfo(iUnit).getButton()
			iWidget = WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT
			iData1 = iUnit
		elif pLoopCity.isProductionBuilding():
			iBuilding = pLoopCity.getProductionBuilding()
			szButton = gc.getBuildingInfo(iBuilding).getButton()
			iWidget = WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING
			iData1 = iBuilding
		elif pLoopCity.isProductionProject():
			iProject = pLoopCity.getProductionProject()
			szButton = gc.getProjectInfo(iProject).getButton()
			iWidget = WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROJECT
			iData1 = iProject
		elif pLoopCity.isProductionProcess():
			iProcess = pLoopCity.getProductionProcess()
			szButton = gc.getProcessInfo(iProcess).getButton()

		# Pass szButton as the 5th argument to display the icon next to the text
		#screen.setTableText( self.TABLE_OVERVIEW1, 15, i, szFontTagOpen + szProducing + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableText( self.TABLE_OVERVIEW1, 15, i, szFontTagOpen + szProducing + szFontTagClose, szButton, iWidget, iData1, iData2, CvUtil.FONT_LEFT_JUSTIFY )
		# End - add the button (icon) to the "Producing" column.

		szProductionTurns = u""
		if iProductionTurns > 0: # advc.004x
			szProductionTurns = unicode(iProductionTurns)
		screen.setTableInt( self.TABLE_OVERVIEW1, 16, i, szFontTagOpen + szProductionTurns + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		screen.setTableInt( self.TABLE_OVERVIEW1, 17, i, szFontTagOpen + unicode(pLoopCity.getDefenseModifier(False)) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		# Garrison
		screen.setTableInt( self.TABLE_OVERVIEW1, 18, i, szFontTagOpen + unicode(pLoopCity.plot().getNumDefenders(pLoopCity.getOwner())) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		# Liberation
		# advc.004: Mark potential independent colonies too (bCanSplit check added to the two conditions below)
		#bCanSplit = gc.getPlayer(gc.getGame().getActivePlayer()).canSplitArea(pLoopCity.area().getID())
		#if bCanSplit or pLoopCity.getLiberationPlayer(false) != -1:
			# UNOFFICIAL_PATCH begin
		#	if bCanSplit or not gc.getTeam(gc.getPlayer(pLoopCity.getLiberationPlayer(false)).getTeam()).isAtWar(CyGame().getActiveTeam()) :
		#		screen.setTableText( self.TABLE_OVERVIEW1, 19, i, sasFontTagBody + (u"%c" % CyGame().getSymbolID(FontSymbols.OCCUPATION_CHAR)) + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
			# UNOFFICIAL_PATCH end
		# <advc.ctr> Replace this column with revolt probability
		revoltPr = pLoopCity.revoltProbability()
		if revoltPr > 0:
			revoltPr = max(revoltPr, 0.001) # Show all near-0 values as "0.1%"
			szRevoltPr = ("%.1f" % (100 * revoltPr)) + "%"
			szColorTag = "COLOR_NEGATIVE_TEXT"
			if not pLoopCity.canCultureFlip():
				szColorTag = "COLOR_YIELD_FOOD"
			szRevoltPr = localText.changeTextColor(szRevoltPr, getInfoTypeOrFail(szColorTag))
			screen.setTableText(self.TABLE_OVERVIEW1, 19, i, szFontTagOpen + szRevoltPr + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# </advc.ctr>

	def drawOverview2Contents(self):
		self.clearFinanceWidgets()
		screen = self.getScreen()
		if self.bOverview1TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW1)
			self.bOverview1TableCreated = False
		self.hideSpecialists()
		if self.bOverview3TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW3)
			self.bOverview3TableCreated = False
		if self.bOverview4TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW4)
			self.bOverview4TableCreated = False
		if self.bOverview2TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW2)
			self.bOverview2TableCreated = False

		player = gc.getPlayer(self.iActivePlayer)
		screen = self.setupOverviewTable(self.TABLE_OVERVIEW2, 20 + CommerceTypes.NUM_COMMERCE_TYPES)
		self.bOverview2TableCreated = True

		i = 0
		(pLoopCity, iter) = player.firstCity(false)
		while(pLoopCity):
			screen.appendTableRow( self.TABLE_OVERVIEW2 )
			self.updateTable2(pLoopCity, i)
			i += 1
			(pLoopCity, iter) = player.nextCity(iter, false)

		self.drawHeaders2()
		CyInterface().setDirty(InterfaceDirtyBits.Domestic_Advisor_DIRTY_BIT, true)

	def drawHeaders2(self):
		self.initText()
		screen = self.getScreen()

		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 0, "", (24 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 1, self.getOverviewHeaderLabel(self.HEADER_NAME), (self.OVERVIEW_CITY_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 2, self.getOverviewHeaderLabel(u"Pop"), (35 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 3, self.getOverviewHeaderLabel(self.HEADER_FOOD), (35 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 4, self.getOverviewHeaderLabel(self.HEADER_EATEN_FOOD), (35 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 5, self.getOverviewHeaderLabel(self.HEADER_FOOD + self.HEADER_DELTA), (35 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 6, self.getOverviewHeaderLabel(self.HEADER_FOOD + u"/"), (60 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 7, self.getOverviewHeaderLabel(self.HEADER_PRODUCTION), (35 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 8, self.getOverviewHeaderLabel(self.HEADER_PRODUCTION + u"/"), (60 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 9, self.getOverviewHeaderLabel(self.HEADER_HAPPINESS), (33 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 10, self.getOverviewHeaderLabel(self.HEADER_UNHAPPINESS), (33 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 11, self.getOverviewHeaderLabel(self.HEADER_HAPPINESS + self.HEADER_DELTA), (33 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 12, self.getOverviewHeaderLabel(self.HEADER_HEALTH), (33 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 13, self.getOverviewHeaderLabel(self.HEADER_UNHEALTH), (33 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 14, self.getOverviewHeaderLabel(self.HEADER_HEALTH + self.HEADER_DELTA), (33 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 15, self.getOverviewHeaderLabel(self.HEADER_FOOD + u"%"), (self.OVERVIEW_SIGNED_MODIFIER_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 16, self.getOverviewHeaderLabel(self.HEADER_PRODUCTION + u"%"), (self.OVERVIEW_SIGNED_MODIFIER_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 17, self.getOverviewHeaderLabel(self.HEADER_COMMERCE_YIELD_MODIFIER), (self.OVERVIEW_SIGNED_MODIFIER_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )
		for eCommerce in range(CommerceTypes.NUM_COMMERCE_TYPES):
			screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 18 + eCommerce, self.getOverviewHeaderLabel(self.HEADER_COMMERCE_MODIFIERS[eCommerce]), (self.OVERVIEW_SIGNED_MODIFIER_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 18 + CommerceTypes.NUM_COMMERCE_TYPES, self.getOverviewHeaderLabel(self.HEADER_TRADE + u"R%"), (self.OVERVIEW_SIGNED_MODIFIER_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW2, 19 + CommerceTypes.NUM_COMMERCE_TYPES, self.getOverviewHeaderLabel(self.HEADER_TRADE + u"FR%"), (self.OVERVIEW_SIGNED_MODIFIER_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )

	def updateTable2(self, pLoopCity, i):
		screen = self.getScreen()
		szFontTagOpen = sasFontTagLabel
		szFontTagClose = SAS_FONT_TAG_CLOSE

		screen.setTableText( self.TABLE_OVERVIEW2, 0, i, "", self.ART_CITY_SELECTION_BUTTON, WidgetTypes.WIDGET_ZOOM_CITY, pLoopCity.getOwner(), pLoopCity.getID(), CvUtil.FONT_LEFT_JUSTIFY)
		szName = pLoopCity.getName()
		if pLoopCity.isCapital():
			szName += (u"%c" % CyGame().getSymbolID(FontSymbols.STAR_CHAR))
		elif pLoopCity.isGovernmentCenter():
			szName += (u"%c" % CyGame().getSymbolID(FontSymbols.SILVER_STAR_CHAR))
		screen.setTableText( self.TABLE_OVERVIEW2, 1, i, szFontTagOpen + szName + szFontTagClose, "", WidgetTypes.WIDGET_EXAMINE_CITY, pLoopCity.getOwner(), pLoopCity.getID(), CvUtil.FONT_LEFT_JUSTIFY )

		screen.setTableInt( self.TABLE_OVERVIEW2, 2, i, szFontTagOpen + unicode(pLoopCity.getPopulation()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		iFoodRate = pLoopCity.getYieldRate(YieldTypes.YIELD_FOOD)
		screen.setTableInt( self.TABLE_OVERVIEW2, 3, i, szFontTagOpen + unicode(iFoodRate) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		if self.iFOOD_CONSUMPTION_PER_POPULATION is None:
			self.iFOOD_CONSUMPTION_PER_POPULATION = gc.getDefineINT("FOOD_CONSUMPTION_PER_POPULATION")
		iFoodConsumed = pLoopCity.getPopulation() * self.iFOOD_CONSUMPTION_PER_POPULATION
		screen.setTableInt( self.TABLE_OVERVIEW2, 4, i, szFontTagOpen + unicode(iFoodConsumed) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		iFoodNet = pLoopCity.foodDifference(true)
		szFoodNet = self.getSignedText(iFoodNet, False)
		if iFoodNet == 0 and pLoopCity.isFoodProduction():
			# <!-- custom: use the same blank zero-net food-to-production convention as Overview 1. (GPT-5.5) -->
			szFoodNet = u""
		screen.setTableInt( self.TABLE_OVERVIEW2, 5, i, szFontTagOpen + szFoodNet + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		szFoodStored = unicode(pLoopCity.getFood()) + u"/" + unicode(pLoopCity.growthThreshold())
		screen.setTableText( self.TABLE_OVERVIEW2, 6, i, szFontTagOpen + szFoodStored + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		screen.setTableInt( self.TABLE_OVERVIEW2, 7, i, szFontTagOpen + unicode(pLoopCity.getCurrentProductionDifference(True, False)) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		iProductionNeeded = pLoopCity.getProductionNeeded()
		szProductionStored = u""
		# <!-- custom: production processes have no finite hammer target; getProductionNeeded returns MAX_INT, which displayed as clipped "0/2147..." in this table. (GPT-5.5) -->
		if not pLoopCity.isProductionProcess() and iProductionNeeded >= 0:
			szProductionStored = unicode(pLoopCity.getProduction()) + u"/" + unicode(iProductionNeeded)
		screen.setTableText( self.TABLE_OVERVIEW2, 8, i, szFontTagOpen + szProductionStored + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		iHappy = pLoopCity.happyLevel()
		screen.setTableInt( self.TABLE_OVERVIEW2, 9, i, szFontTagOpen + unicode(iHappy) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		iUnhappy = pLoopCity.unhappyLevel(0)
		szUnhappy = unicode(iUnhappy)
		if iUnhappy > 0:
			szUnhappy = self.COLOR_NEGATIVE_PREFIX + szUnhappy + self.COLOR_REVERT_SUFFIX
		screen.setTableInt( self.TABLE_OVERVIEW2, 10, i, szFontTagOpen + szUnhappy + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		iHappyNet = iHappy - iUnhappy
		szHappyNet = self.getSignedText(iHappyNet, True)
		screen.setTableInt( self.TABLE_OVERVIEW2, 11, i, szFontTagOpen + szHappyNet + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		iHealth = pLoopCity.goodHealth()
		screen.setTableInt( self.TABLE_OVERVIEW2, 12, i, szFontTagOpen + unicode(iHealth) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		iUnhealth = pLoopCity.badHealth(0)
		szUnhealth = unicode(iUnhealth)
		if iUnhealth > 0:
			szUnhealth = self.COLOR_NEGATIVE_PREFIX + szUnhealth + self.COLOR_REVERT_SUFFIX
		screen.setTableInt( self.TABLE_OVERVIEW2, 13, i, szFontTagOpen + szUnhealth + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		iHealthNet = iHealth - iUnhealth
		szHealthNet = self.getSignedText(iHealthNet, True)
		screen.setTableInt( self.TABLE_OVERVIEW2, 14, i, szFontTagOpen + szHealthNet + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		iFoodMod = pLoopCity.getBaseYieldRateModifier(YieldTypes.YIELD_FOOD, 0) - 100
		iProdMod = pLoopCity.getBaseYieldRateModifier(YieldTypes.YIELD_PRODUCTION, 0) - 100
		iCommMod = pLoopCity.getBaseYieldRateModifier(YieldTypes.YIELD_COMMERCE, 0) - 100
		# <!-- custom: leave zero modifier cells blank; nonzero modifiers remain signed and sortable through setTableInt. (GPT-5.5) -->
		screen.setTableInt( self.TABLE_OVERVIEW2, 15, i, szFontTagOpen + self.getSignedModifierText(iFoodMod) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW2, 16, i, szFontTagOpen + self.getSignedModifierText(iProdMod) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW2, 17, i, szFontTagOpen + self.getSignedModifierText(iCommMod) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		for eCommerce in range(CommerceTypes.NUM_COMMERCE_TYPES):
			iCommerceMod = pLoopCity.getTotalCommerceRateModifier(eCommerce) - 100
			screen.setTableInt( self.TABLE_OVERVIEW2, 18 + eCommerce, i, szFontTagOpen + self.getSignedModifierText(iCommerceMod) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW2, 18 + CommerceTypes.NUM_COMMERCE_TYPES, i, szFontTagOpen + self.getSignedModifierText(pLoopCity.getTradeRouteModifier()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW2, 19 + CommerceTypes.NUM_COMMERCE_TYPES, i, szFontTagOpen + self.getSignedModifierText(pLoopCity.getForeignTradeRouteModifier()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

	def drawOverview3Contents(self):
		self.clearFinanceWidgets()
		screen = self.getScreen()
		if self.bOverview1TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW1)
			self.bOverview1TableCreated = False
		self.hideSpecialists()
		if self.bOverview2TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW2)
			self.bOverview2TableCreated = False
		if self.bOverview3TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW3)
			self.bOverview3TableCreated = False
		if self.bOverview4TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW4)
			self.bOverview4TableCreated = False

		player = gc.getPlayer(self.iActivePlayer)

		screen = self.setupOverviewTable(self.TABLE_OVERVIEW3, 23)
		self.bOverview3TableCreated = True

		i = 0
		(pLoopCity, iter) = player.firstCity(false)
		while(pLoopCity):
			screen.appendTableRow( self.TABLE_OVERVIEW3 )
			self.updateTable3(pLoopCity, i)
			i += 1
			(pLoopCity, iter) = player.nextCity(iter, false)

		self.drawHeaders3()
		CyInterface().setDirty(InterfaceDirtyBits.Domestic_Advisor_DIRTY_BIT, true)

	def drawHeaders3(self):
		self.initText()
		screen = self.getScreen()
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 0, "", (24 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 1, self.getOverviewHeaderLabel(self.HEADER_NAME), (self.OVERVIEW_CITY_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 2, self.getOverviewHeaderLabel(self.HEADER_POPULATION), (35 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 3, self.getOverviewHeaderLabel(self.HEADER_CULTURE), (45 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 4, self.getOverviewHeaderLabel(self.HEADER_CULTURE + u"thr"), (45 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 5, self.getOverviewHeaderLabel(self.HEADER_CULTURE + u"R"), (35 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 6, self.getOverviewHeaderLabel(self.HEADER_CULTURE + u"T"), (35 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 7, self.getOverviewHeaderLabel(self.HEADER_GREAT_PERSON), (45 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 8, self.getOverviewHeaderLabel(self.HEADER_GREAT_PERSON + u"thr"), (45 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 9, self.getOverviewHeaderLabel(self.HEADER_GREAT_PERSON + u"base"), (40 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 10, self.getOverviewHeaderLabel(self.HEADER_GREAT_PERSON + u"R"), (35 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 11, self.getOverviewHeaderLabel(self.HEADER_GREAT_PERSON + u"%"), (self.OVERVIEW_SIGNED_MODIFIER_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 12, self.getOverviewHeaderLabel(self.HEADER_GREAT_PERSON + u"T"), (35 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 13, self.getOverviewHeaderLabel(self.HEADER_GREAT_PERSON + u"#"), (35 * self.nTableWidth) / self.nNormalizedTableWidth )
		# <!-- custom: keep remaining city-level modifiers on Overview 3; Overview 2 already covers yield, commerce, and trade-route modifiers, and zero cells stay blank for scanning. (GPT-5.5) -->
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 14, self.getOverviewHeaderLabel(self.HEADER_MAINTENANCE + u"%"), (self.OVERVIEW_SIGNED_MODIFIER_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 15, self.getOverviewHeaderLabel(u"WW%"), (self.OVERVIEW_SIGNED_MODIFIER_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 16, self.getOverviewHeaderLabel(u"HA%"), (self.OVERVIEW_SIGNED_MODIFIER_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 17, self.getOverviewHeaderLabel(self.HEADER_UNHEALTH + u"%"), (self.OVERVIEW_SIGNED_MODIFIER_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 18, self.getOverviewHeaderLabel(u"Mil%"), (self.OVERVIEW_SIGNED_MODIFIER_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 19, self.getOverviewHeaderLabel(u"Space%"), (self.OVERVIEW_SIGNED_MODIFIER_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 20, self.getOverviewHeaderLabel(u"Air%"), (self.OVERVIEW_SIGNED_MODIFIER_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 21, self.getOverviewHeaderLabel(u"Nuke%"), (self.OVERVIEW_SIGNED_MODIFIER_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW3, 22, self.getOverviewHeaderLabel(self.HEADER_ESPIONAGE + u"D%"), (self.OVERVIEW_SIGNED_MODIFIER_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )

	def updateTable3(self, pLoopCity, i):
		screen = self.getScreen()
		szFontTagOpen = sasFontTagLabel
		szFontTagClose = SAS_FONT_TAG_CLOSE

		screen.setTableText( self.TABLE_OVERVIEW3, 0, i, "", self.ART_CITY_SELECTION_BUTTON, WidgetTypes.WIDGET_ZOOM_CITY, pLoopCity.getOwner(), pLoopCity.getID(), CvUtil.FONT_LEFT_JUSTIFY)

		szName = pLoopCity.getName()
		if pLoopCity.isCapital():
			szName += (u"%c" % CyGame().getSymbolID(FontSymbols.STAR_CHAR))
		elif pLoopCity.isGovernmentCenter():
			szName += (u"%c" % CyGame().getSymbolID(FontSymbols.SILVER_STAR_CHAR))
		screen.setTableText( self.TABLE_OVERVIEW3, 1, i, szFontTagOpen + szName + szFontTagClose, "", WidgetTypes.WIDGET_EXAMINE_CITY, pLoopCity.getOwner(), pLoopCity.getID(), CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW3, 2, i, szFontTagOpen + unicode(pLoopCity.getPopulation()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		iCultureTimes100 = pLoopCity.getCultureTimes100(self.iActivePlayer)
		iCultureTotal = iCultureTimes100 / 100
		iCultureThreshold = pLoopCity.getCultureThreshold()
		screen.setTableInt( self.TABLE_OVERVIEW3, 3, i, szFontTagOpen + unicode(iCultureTotal) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW3, 4, i, szFontTagOpen + unicode(iCultureThreshold) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		iCultureRate = pLoopCity.getCommerceRate(CommerceTypes.COMMERCE_CULTURE)
		screen.setTableInt( self.TABLE_OVERVIEW3, 5, i, szFontTagOpen + unicode(iCultureRate) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		iCultureRateTimes100 = pLoopCity.getCommerceRateTimes100(CommerceTypes.COMMERCE_CULTURE)
		szCultTurns = u"-"
		if iCultureRateTimes100 > 0:
			iCultureLeftTimes100 = 100 * iCultureThreshold - iCultureTimes100
			if iCultureLeftTimes100 > 0:
				szCultTurns = unicode((iCultureLeftTimes100 + iCultureRateTimes100 - 1) / iCultureRateTimes100)
		screen.setTableInt( self.TABLE_OVERVIEW3, 6, i, szFontTagOpen + szCultTurns + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		iGPProgress = pLoopCity.getGreatPeopleProgress()
		iGPThreshold = gc.getPlayer(self.iActivePlayer).greatPeopleThreshold(False)
		iGPBaseRate = pLoopCity.getBaseGreatPeopleRate()
		iGPR = pLoopCity.getGreatPeopleRate()
		iGPModifier = pLoopCity.getTotalGreatPeopleRateModifier() - 100
		screen.setTableInt( self.TABLE_OVERVIEW3, 7, i, szFontTagOpen + unicode(iGPProgress) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW3, 8, i, szFontTagOpen + unicode(iGPThreshold) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW3, 9, i, szFontTagOpen + unicode(iGPBaseRate) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW3, 10, i, szFontTagOpen + unicode(iGPR) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW3, 11, i, szFontTagOpen + self.getSignedModifierText(iGPModifier) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		szGPTurns = u"-"
		if iGPR > 0:
			iGPPLeft = iGPThreshold - iGPProgress
			if iGPPLeft > 0:
				iTurnsLeft = iGPPLeft / iGPR
				if iTurnsLeft * iGPR < iGPPLeft:
					iTurnsLeft += 1
				szGPTurns = unicode(iTurnsLeft)
		screen.setTableInt( self.TABLE_OVERVIEW3, 12, i, szFontTagOpen + szGPTurns + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW3, 13, i, szFontTagOpen + unicode(pLoopCity.getNumGreatPeople()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW3, 14, i, szFontTagOpen + self.getInvertedSignedModifierText(pLoopCity.getMaintenanceModifier()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW3, 15, i, szFontTagOpen + self.getInvertedSignedModifierText(pLoopCity.getWarWearinessModifier()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW3, 16, i, szFontTagOpen + self.getInvertedSignedModifierText(pLoopCity.getHurryAngerModifier()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW3, 17, i, szFontTagOpen + self.getInvertedSignedModifierText(pLoopCity.getUnhealthyPopulationModifier()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW3, 18, i, szFontTagOpen + self.getSignedModifierText(pLoopCity.getMilitaryProductionModifier()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW3, 19, i, szFontTagOpen + self.getSignedModifierText(pLoopCity.getSpaceProductionModifier()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW3, 20, i, szFontTagOpen + self.getSignedModifierText(pLoopCity.getAirModifier()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW3, 21, i, szFontTagOpen + self.getSignedModifierText(pLoopCity.getNukeModifier()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW3, 22, i, szFontTagOpen + self.getSignedModifierText(pLoopCity.getEspionageDefenseModifier()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

	def drawOverview4Contents(self):
		self.clearFinanceWidgets()
		screen = self.getScreen()
		if self.bOverview1TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW1)
			self.bOverview1TableCreated = False
		self.hideSpecialists()
		if self.bOverview2TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW2)
			self.bOverview2TableCreated = False
		if self.bOverview3TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW3)
			self.bOverview3TableCreated = False
		if self.bOverview4TableCreated:
			screen.deleteWidget(self.TABLE_OVERVIEW4)
			self.bOverview4TableCreated = False

		player = gc.getPlayer(self.iActivePlayer)
		iFixedCols = 9
		iTotalCols = iFixedCols + len(self.aOverview4Specialists)

		screen = self.setupOverviewTable(self.TABLE_OVERVIEW4, iTotalCols)
		self.bOverview4TableCreated = True

		i = 0
		(pLoopCity, iter) = player.firstCity(false)
		while(pLoopCity):
			screen.appendTableRow( self.TABLE_OVERVIEW4 )
			self.updateTable4(pLoopCity, i)
			i += 1
			(pLoopCity, iter) = player.nextCity(iter, false)

		self.drawHeaders4()
		CyInterface().setDirty(InterfaceDirtyBits.Domestic_Advisor_DIRTY_BIT, true)

	def drawHeaders4(self):
		self.initText()
		screen = self.getScreen()
		screen.setTableColumnHeader( self.TABLE_OVERVIEW4, 0, "", (24 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW4, 1, self.getOverviewHeaderLabel(self.HEADER_NAME), (self.OVERVIEW_CITY_COL_WIDTH * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW4, 2, self.getOverviewHeaderLabel(self.HEADER_POPULATION), (35 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW4, 3, self.getOverviewHeaderLabel(self.HEADER_FOUNDED), (75 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW4, 4, self.getOverviewHeaderLabel(self.HEADER_REAL_POPULATION), (80 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW4, 5, self.getOverviewHeaderLabel(self.HEADER_CORPORATIONS), (self.getOverviewCorporationColumnWidth() * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW4, 6, self.getOverviewHeaderLabel(u"XP"), (35 * self.nTableWidth) / self.nNormalizedTableWidth )
		screen.setTableColumnHeader( self.TABLE_OVERVIEW4, 7, self.getOverviewHeaderLabel(u"SXP"), (35 * self.nTableWidth) / self.nNormalizedTableWidth )
		iSpecColW = 33
		for k in range(len(self.aOverview4Specialists)):
			screen.setTableColumnHeader( self.TABLE_OVERVIEW4, 8 + k, self.aOverview4SpecialistHeaders[k], (iSpecColW * self.nTableWidth) / self.nNormalizedTableWidth )
		# <!-- custom: mirror city-screen order by placing angry citizens after specialist and settled Great Specialist counts. (GPT-5.5) -->
		screen.setTableColumnHeader( self.TABLE_OVERVIEW4, 8 + len(self.aOverview4Specialists), self.getOverviewHeaderLabel(self.HEADER_ANGRY_POPULATION), (35 * self.nTableWidth) / self.nNormalizedTableWidth )

	def updateTable4(self, pLoopCity, i):
		screen = self.getScreen()
		szFontTagOpen = sasFontTagLabel
		szFontTagClose = SAS_FONT_TAG_CLOSE

		screen.setTableText( self.TABLE_OVERVIEW4, 0, i, "", self.ART_CITY_SELECTION_BUTTON, WidgetTypes.WIDGET_ZOOM_CITY, pLoopCity.getOwner(), pLoopCity.getID(), CvUtil.FONT_LEFT_JUSTIFY)

		szName = pLoopCity.getName()
		if pLoopCity.isCapital():
			szName += (u"%c" % CyGame().getSymbolID(FontSymbols.STAR_CHAR))
		elif pLoopCity.isGovernmentCenter():
			szName += (u"%c" % CyGame().getSymbolID(FontSymbols.SILVER_STAR_CHAR))
		screen.setTableText( self.TABLE_OVERVIEW4, 1, i, szFontTagOpen + szName + szFontTagClose, "", WidgetTypes.WIDGET_EXAMINE_CITY, pLoopCity.getOwner(), pLoopCity.getID(), CvUtil.FONT_LEFT_JUSTIFY )

		szCorps = self.getCityCorporationText(pLoopCity)
		screen.setTableInt( self.TABLE_OVERVIEW4, 2, i, szFontTagOpen + unicode(pLoopCity.getPopulation()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		# <!-- custom: Founded used localized BC/AD text in a lexical table cell, so sorting did not follow chronology across differently sized years or the BC/AD boundary.
		# Match World Advisor by using the raw signed founding year in a numeric cell. See KI#223. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		iFoundYear = CyGame().getTurnYear(pLoopCity.getGameTurnFounded())
		SASTextScale.setTableIntLabel(screen, self.TABLE_OVERVIEW4, 3, i, iFoundYear, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.setTableInt( self.TABLE_OVERVIEW4, 4, i, szFontTagOpen + unicode(pLoopCity.getRealPopulation()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableText( self.TABLE_OVERVIEW4, 5, i, szFontTagOpen + szCorps + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW4, 6, i, szFontTagOpen + self.getBlankZeroText(pLoopCity.getFreeExperience()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.setTableInt( self.TABLE_OVERVIEW4, 7, i, szFontTagOpen + self.getBlankZeroText(pLoopCity.getSpecialistFreeExperience()) + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		for k in range(len(self.aOverview4Specialists)):
			eSpec = self.aOverview4Specialists[k]
			iCount = pLoopCity.getSpecialistCount(eSpec) + pLoopCity.getFreeSpecialistCount(eSpec)
			if iCount > 0:
				szText = unicode(iCount)
			else:
				szText = u"-"
			screen.setTableInt( self.TABLE_OVERVIEW4, 8 + k, i, szFontTagOpen + szText + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		iAngryPopulation = pLoopCity.angryPopulation(0)
		if iAngryPopulation > 0:
			szAngryPopulation = unicode(iAngryPopulation)
		else:
			szAngryPopulation = u""
		screen.setTableInt( self.TABLE_OVERVIEW4, 8 + len(self.aOverview4Specialists), i, szFontTagOpen + szAngryPopulation + szFontTagClose, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

	# Draw the specialist and their increase and decrease buttons
	def drawSpecialists(self):
		screen = CyGInterfaceScreen( "DomesticAdvisor", CvScreenEnums.DOMESTIC_ADVISOR )

		for i in range( gc.getNumSpecialistInfos() ):
			if (gc.getSpecialistInfo(i).isVisible()):			
				szName = "SpecialistImage" + str(i)
				screen.setImageButton( szName, gc.getSpecialistInfo(i).getTexture(), self.nFirstSpecialistX + (self.nSpecialistDistance * i), self.nSpecialistY, self.nSpecialistWidth, self.nSpecialistLength, WidgetTypes.WIDGET_CITIZEN, i, -1 )
				screen.hide(szName)

				szName = "SpecialistPlus" + str(i)
				screen.setButtonGFC( szName, u"", "", self.nFirstSpecialistX + (self.nSpecialistDistance * i) + self.nPlusOffsetX, self.nSpecialistY + self.nPlusOffsetY, self.nPlusWidth, self.nPlusHeight, WidgetTypes.WIDGET_CHANGE_SPECIALIST, i, 1, ButtonStyles.BUTTON_STYLE_CITY_PLUS )
				screen.hide(szName)

				szName = "SpecialistMinus" + str(i)
				screen.setButtonGFC( szName, u"", "", self.nFirstSpecialistX + (self.nSpecialistDistance * i) + self.nMinusOffsetX, self.nSpecialistY + self.nMinusOffsetY, self.nMinusWidth, self.nMinusHeight, WidgetTypes.WIDGET_CHANGE_SPECIALIST, i, -1, ButtonStyles.BUTTON_STYLE_CITY_MINUS )
				screen.hide(szName)

				szName = "SpecialistText" + str(i)
				screen.setLabel(szName, "Background", "", CvUtil.FONT_LEFT_JUSTIFY, self.nFirstSpecialistX + (self.nSpecialistDistance * i) + self.nSpecTextOffsetX, self.nSpecialistY + self.nSpecTextOffsetY, 0, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				screen.hide(szName)

	def hideSpecialists(self):
		screen = CyGInterfaceScreen( "DomesticAdvisor", CvScreenEnums.DOMESTIC_ADVISOR )
		for i in range( gc.getNumSpecialistInfos() ):
			if (gc.getSpecialistInfo(i).isVisible()):			
				screen.hide("SpecialistImage" + str(i))
				screen.hide("SpecialistPlus" + str(i))
				screen.hide("SpecialistMinus" + str(i))
				screen.hide("SpecialistText" + str(i))

	def updateSpecialists(self):
		# Function which shows the specialists.
		#
		screen = CyGInterfaceScreen( "DomesticAdvisor", CvScreenEnums.DOMESTIC_ADVISOR )

		if self.iActivePlayer != CyGame().getActivePlayer():
			self.hideSpecialists()
			return

		if (CyInterface().isOneCitySelected()):

			city = CyInterface().getHeadSelectedCity()
			nPopulation = city.getPopulation()
			nFreeSpecial = city.totalFreeSpecialists()

			for i in range( gc.getNumSpecialistInfos() ):
				if (gc.getSpecialistInfo(i).isVisible()):	
					szName = "SpecialistImage" + str(i)
					screen.show(szName)

					szName = "SpecialistText" + str(i)
					screen.setLabel(szName, "Background", str (city.getSpecialistCount(i)) + "/" + str(city.getMaxSpecialistCount(i)), CvUtil.FONT_LEFT_JUSTIFY, self.nFirstSpecialistX + (self.nSpecialistDistance * i) + self.nSpecTextOffsetX, self.nSpecialistY + self.nSpecTextOffsetY, 0, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
					screen.show(szName)

					# If the specialist is valid and we can increase it
					szName = "SpecialistPlus" + str(i)
					if (city.isSpecialistValid(i, 1) and (city.getForceSpecialistCount(i) < (nPopulation + nFreeSpecial))):
						screen.show(szName)
					else:
						screen.hide(szName)

					# if we HAVE specialists already and they're not forced.
					szName = "SpecialistMinus" + str(i)
					if (city.getSpecialistCount(i) > 0 or city.getForceSpecialistCount(i) > 0):
						screen.show(szName)
					else:
						screen.hide(szName)
		else:
			self.hideSpecialists()

	# Will handle the input for this screen...
	def handleInput (self, inputClass):
		# Calls function mapped in DomesticAdvisorInputMap
		#
		# only get from the map if it has the key
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED and inputClass.getButtonType() == WidgetTypes.WIDGET_GENERAL:
			iData1 = inputClass.getData1()
			if iData1 in self.PAGE_IDS and iData1 != self.iPage:
				self.iPage = iData1
				self.drawContents()
				return 1
		if self.iPage == self.PAGE_FINANCE and inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED and inputClass.getButtonType() == WidgetTypes.WIDGET_CHANGE_PERCENT:
			self.drawFinanceContents()
			return 1
		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED and inputClass.getFunctionName() == self.FREE_COLONY_BUTTON_ID:
			self.getScreen().hideScreen()

		if ( inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED ):
			if inputClass.getFunctionName() == self.DEBUG_DROPDOWN_ID:
				screen = self.getScreen()
				self.iActivePlayer = getAdvisorDebugDropdownSelectedPlayer(screen, self.DEBUG_DROPDOWN_ID)
				self.listSelectedCities = []
				self.drawContents()
				return 1
			if self.iPage not in self.OVERVIEW_PAGE_IDS:
				return 0
			if (inputClass.getMouseX() == 0):
				if isAdvisorReadOnlyPerspective(self.iActivePlayer):
					return 1
				screen = CyGInterfaceScreen( "DomesticAdvisor", CvScreenEnums.DOMESTIC_ADVISOR )
				screen.hideScreen()

				CyInterface().selectCity(gc.getPlayer(inputClass.getData1()).getCity(inputClass.getData2()), true)

				popupInfo = CyPopupInfo()
				popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON_SCREEN)
				popupInfo.setText(u"showDomesticAdvisor")
				popupInfo.addPopup(inputClass.getData1())
			elif self.iPage == self.PAGE_OVERVIEW1:
				if isAdvisorReadOnlyPerspective(self.iActivePlayer):
					return 1
				self.updateAppropriateCitySelection()
				self.updateSpecialists()
		return 0

	def updateAppropriateCitySelection(self):
		if self.iPage != self.PAGE_OVERVIEW1 or isAdvisorReadOnlyPerspective(self.iActivePlayer):
			return
		if self.iActivePlayer != CyGame().getActivePlayer():
			self.listSelectedCities = []
			return
		nCities = gc.getPlayer(self.iActivePlayer).getNumCities()
		screen = CyGInterfaceScreen( "DomesticAdvisor", CvScreenEnums.DOMESTIC_ADVISOR )
		screen.updateAppropriateCitySelection( self.TABLE_OVERVIEW1, nCities, 1 )
		self.listSelectedCities = []
		for i in range(nCities):
			if screen.isRowSelected(self.TABLE_OVERVIEW1, i):
				self.listSelectedCities.append(screen.getTableText(self.TABLE_OVERVIEW1, 1, i))

	def update(self, fDelta):
		if self.iPage == self.PAGE_FINANCE:
			if (CyInterface().isDirty(InterfaceDirtyBits.Financial_Screen_DIRTY_BIT)):
				CyInterface().setDirty(InterfaceDirtyBits.Financial_Screen_DIRTY_BIT, False)
				self.drawFinanceContents()
			return

		if (CyInterface().isDirty(InterfaceDirtyBits.Domestic_Advisor_DIRTY_BIT)):
			CyInterface().setDirty(InterfaceDirtyBits.Domestic_Advisor_DIRTY_BIT, False)

			player = gc.getPlayer(self.iActivePlayer)

			if self.iPage == self.PAGE_OVERVIEW2:
				updater = self.updateTable2
			elif self.iPage == self.PAGE_OVERVIEW3:
				updater = self.updateTable3
			elif self.iPage == self.PAGE_OVERVIEW4:
				updater = self.updateTable4
			else:
				updater = self.updateTable1

			i = 0
			(pLoopCity, iter) = player.firstCity(false)
			while(pLoopCity):
				updater(pLoopCity, i)
				i += 1
				(pLoopCity, iter) = player.nextCity(iter, false)

			if self.iPage == self.PAGE_OVERVIEW1:
				self.updateSpecialists()

		return
