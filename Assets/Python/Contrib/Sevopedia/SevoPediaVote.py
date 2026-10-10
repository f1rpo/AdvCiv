# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from CvPythonExtensions import *
from SASMagicNumbers import *
import CvUtil
import SevoScreenEnums
import SASTextScale
from _sevopedia_helpers import *

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class SevoPediaVote:

	def __init__(self, main):
		self.iVote = -1
		self.top = main
		self.bHistoryExpanded = False

		self.X_VOTE_PANE = self.top.X_PEDIA_PAGE
		self.Y_VOTE_PANE = self.top.Y_PEDIA_PAGE
		self.W_VOTE_PANE = 200
		self.H_VOTE_PANE = 230

		self.W_ICON = PANE_ICON_SIZE
		self.H_ICON = PANE_ICON_SIZE
		self.X_ICON = self.X_VOTE_PANE + (PANE_ICON_FRAME_SIZE - PANE_ICON_SIZE) / 2 + 19
		self.Y_ICON = self.Y_VOTE_PANE + (self.H_VOTE_PANE - self.H_ICON) / 2 + 3

		self.X_RIGHT = self.X_VOTE_PANE + self.W_VOTE_PANE + MEDIUM_MARGIN

		self.W_SOURCE = get_panel_width_for_buttons(1, MULTILIST_BUTTON_SIZE, HYPOTHESIZED_NON_MULTILIST_PANEL_EDGE_PADDING, HYPOTHESIZED_NON_MULTILIST_PANEL_INTER_BUTTON_SPACING)

		# <!-- custom: top row order is V.Source/Civic -> Vote Source Info -> Free.Sp, while F.Civics is shown on the mid row to reflect vote-level scope. (GPT-5.4?) -->
		self.X_SOURCE = self.X_RIGHT
		self.Y_SOURCE = self.Y_VOTE_PANE
		self.H_SOURCE = (self.H_VOTE_PANE - SMALL_MARGIN) / 2

		self.X_SOURCE_INFO = self.X_SOURCE + self.W_SOURCE + MEDIUM_MARGIN
		self.Y_SOURCE_INFO = self.Y_VOTE_PANE
		self.X_SOURCE_SPECIALIST = self.top.R_PEDIA_PAGE - self.W_SOURCE
		self.Y_SOURCE_SPECIALIST = self.Y_VOTE_PANE
		self.W_SOURCE_SPECIALIST = self.W_SOURCE
		self.H_SOURCE_SPECIALIST = self.H_SOURCE

		self.X_SOURCE_CIVIC = self.X_SOURCE
		self.Y_SOURCE_CIVIC = self.Y_SOURCE + self.H_SOURCE + SMALL_MARGIN
		self.W_SOURCE_CIVIC = self.W_SOURCE
		self.H_SOURCE_CIVIC = self.H_VOTE_PANE - self.H_SOURCE - SMALL_MARGIN

		self.W_SOURCE_INFO = self.X_SOURCE_SPECIALIST - MEDIUM_MARGIN - self.X_SOURCE_INFO
		self.H_SOURCE_INFO = self.H_VOTE_PANE

		self.X_VOTE_NAME = self.X_VOTE_PANE
		self.Y_VOTE_NAME = self.Y_VOTE_PANE + self.H_VOTE_PANE + SMALL_MARGIN
		self.W_VOTE_NAME = self.top.R_PEDIA_PAGE - self.X_VOTE_NAME
		self.H_VOTE_NAME = 75

		# <!-- custom: mid row is split into Requirements (left) and Effects (right), with the
		# top row dedicated to vote-source panels and the bottom row to Background text. (GPT-5.4?) -->
		self.X_REQUIREMENTS = self.X_VOTE_PANE
		self.Y_REQUIREMENTS = self.Y_VOTE_NAME + self.H_VOTE_NAME + SMALL_MARGIN
		self.W_REQUIREMENTS = (self.top.R_PEDIA_PAGE - self.X_REQUIREMENTS - MEDIUM_MARGIN) / 2
		self.H_REQUIREMENTS = 200

		self.X_FORCE_CIVICS = self.X_SOURCE_SPECIALIST
		self.Y_FORCE_CIVICS = self.Y_REQUIREMENTS
		self.W_FORCE_CIVICS = self.W_SOURCE
		self.H_FORCE_CIVICS = self.H_SOURCE

		self.X_SPECIAL = self.X_REQUIREMENTS + self.W_REQUIREMENTS + MEDIUM_MARGIN
		self.Y_SPECIAL = self.Y_REQUIREMENTS
		self.W_SPECIAL = self.X_FORCE_CIVICS - MEDIUM_MARGIN - self.X_SPECIAL
		self.H_SPECIAL = 200

		self.X_HISTORY = self.X_VOTE_PANE
		self.Y_HISTORY = self.Y_REQUIREMENTS + self.H_REQUIREMENTS + SMALL_MARGIN
		self.W_HISTORY = self.top.R_PEDIA_PAGE - self.X_VOTE_PANE
		self.H_HISTORY = self.top.B_PEDIA_PAGE - self.Y_HISTORY

		self.BULLET_PREFIX = u"%c " % CyGame().getSymbolID(FontSymbols.BULLET_CHAR)
		self.TRADE_CHAR = u"%c" % CyGame().getSymbolID(FontSymbols.TRADE_CHAR)
		self.YIELD_CHARS = []
		for iYield in range(YieldTypes.NUM_YIELD_TYPES):
			self.YIELD_CHARS.append(gc.getYieldInfo(iYield).getChar())
		self.COMMERCE_CHARS = []
		for iCommerce in range(CommerceTypes.NUM_COMMERCE_TYPES):
			self.COMMERCE_CHARS.append(gc.getCommerceInfo(iCommerce).getChar())

	def interfaceScreen(self, iVote):
		self.iVote = iVote

		self.placeVotePane()
		self.placeRequirements()
		self.placeSourceInfo()
		self.placeSource()
		self.placeForceCivics()
		self.placeSourceSpecialist()
		self.placeSourceCivic()
		self.placeVoteNameLegend()
		self.placeSpecial()
		self.placeHistory()
		place_new_concept_legend_link(self.top, "CONCEPT_SAS_SEVOPEDIA_VOTES_LEGEND")

	def _getVoteSourceType(self):
		# Return the first VoteSource this vote belongs to, or -1 if none.
		voteInfo = gc.getVoteInfo(self.iVote)
		if not voteInfo:
			return -1
		for i in range(gc.getNumVoteSourceInfos()):
			if voteInfo.isVoteSourceType(i):
				return i
		return -1

	def _getHostingBuilding(self, iVoteSource):
		# Find the building that hosts this vote source (reverse lookup).
		if iVoteSource < 0:
			return -1
		for iBuilding in range(gc.getNumBuildingInfos()):
			bi = gc.getBuildingInfo(iBuilding)
			if bi and bi.getVoteSourceType() == iVoteSource:
				return iBuilding
		return -1

	def placeVotePane(self):
		screen = self.top.getScreen()
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_VOTE_PANE, self.Y_VOTE_PANE, self.W_VOTE_PANE, self.H_VOTE_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_EMPTY)
		iVoteSource = self._getVoteSourceType()
		iBuilding = self._getHostingBuilding(iVoteSource)
		szButton = ""
		if iBuilding > -1:
			szButton = gc.getBuildingInfo(iBuilding).getButton()
		if szButton:
			screen.addDDSGFC(self.top.getNextWidgetName(), szButton, self.X_ICON + self.W_ICON/2 - PANE_ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - PANE_ICON_SIZE/2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def placeSource(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_VOTE_SOURCE", ()), "", False, True, self.X_SOURCE, self.Y_SOURCE, self.W_SOURCE, self.H_SOURCE, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		iVoteSource = self._getVoteSourceType()
		iBuilding = self._getHostingBuilding(iVoteSource)
		if iBuilding > -1:
			bi = gc.getBuildingInfo(iBuilding)
			screen.attachImageButton(panelName, "", bi.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, 1, False)
		else:
			draw_none_text(screen, self.top, self.X_SOURCE, self.Y_SOURCE, self.W_SOURCE, self.H_SOURCE)

	def placeRequirements(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_VOTE_REQUIREMENTS", ()), "", True, False, self.X_REQUIREMENTS, self.Y_REQUIREMENTS, self.W_REQUIREMENTS, self.H_REQUIREMENTS, PanelStyles.PANEL_STYLE_BLUE50)

		voteInfo = gc.getVoteInfo(self.iVote)
		lines = []
		if voteInfo:
			lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_POPULATION_THRESHOLD", ()) + u": %d%%" % voteInfo.getPopulationThreshold())
			lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_MIN_VOTERS", ()) + u": %d" % voteInfo.getMinVoters())
			iStateRelPct = voteInfo.getStateReligionVotePercent()
			if iStateRelPct > 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_STATE_RELIGION_VOTE", ()) + u": %d%%" % iStateRelPct)

		textName = self.top.getNextWidgetName()
		if lines:
			szText = u"\n".join(lines)
		else:
			szText = localText.getText("TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_NONE", ())
		# <!-- custom: keep a wider inner margin so text is less cramped against panel borders. (GPT-5.4?) -->
		screen.addMultilineText(textName, SASTextScale.labelText(szText), self.X_REQUIREMENTS + 14, self.Y_REQUIREMENTS + 34, self.W_REQUIREMENTS - 28, self.H_REQUIREMENTS - 46, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeSourceInfo(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_VOTE_SOURCE_INFO", ()), "", True, False, self.X_SOURCE_INFO, self.Y_SOURCE_INFO, self.W_SOURCE_INFO, self.H_SOURCE_INFO, PanelStyles.PANEL_STYLE_BLUE50)

		iVoteSource = self._getVoteSourceType()
		lines = []
		if iVoteSource > -1:
			srcInfo = gc.getVoteSourceInfo(iVoteSource)
			if srcInfo:
				szNone = localText.getText("TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_NONE", ())
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_SOURCE_INTERVAL", ()) + u": %d" % srcInfo.getVoteInterval())
				szSecretaryText = srcInfo.getSecretaryGeneralText()
				if not szSecretaryText:
					szSecretaryText = szNone
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_SOURCE_SECRETARY_GENERAL_TEXT", ()) + u": " + szSecretaryText)

				listYieldParts = []
				for iYield in range(YieldTypes.NUM_YIELD_TYPES):
					iChange = srcInfo.getReligionYield(iYield)
					if iChange != 0:
						listYieldParts.append(u"%+d%c" % (iChange, self.YIELD_CHARS[iYield]))
				if len(listYieldParts) == 0:
					szYields = szNone
				else:
					szYields = u", ".join(listYieldParts)
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_SOURCE_RELIGION_YIELDS", ()) + u": " + szYields)

				listCommerceParts = []
				for iCommerce in range(CommerceTypes.NUM_COMMERCE_TYPES):
					iChange = srcInfo.getReligionCommerce(iCommerce)
					if iChange != 0:
						listCommerceParts.append(u"%+d%c" % (iChange, self.COMMERCE_CHARS[iCommerce]))
				if len(listCommerceParts) == 0:
					szCommerces = szNone
				else:
					szCommerces = u", ".join(listCommerceParts)
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_SOURCE_RELIGION_COMMERCES", ()) + u": " + szCommerces)

		textName = self.top.getNextWidgetName()
		if lines:
			szText = u"\n".join(lines)
		else:
			szText = localText.getText("TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_NONE", ())
		screen.addMultilineText(textName, SASTextScale.labelText(szText), self.X_SOURCE_INFO + 14, self.Y_SOURCE_INFO + 34, self.W_SOURCE_INFO - 28, self.H_SOURCE_INFO - 46, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeSourceSpecialist(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_VOTE_SOURCE_FREE_SPECIALIST_PANEL", ()), "", False, True, self.X_SOURCE_SPECIALIST, self.Y_SOURCE_SPECIALIST, self.W_SOURCE_SPECIALIST, self.H_SOURCE_SPECIALIST, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		iVoteSource = self._getVoteSourceType()
		if iVoteSource > -1:
			srcInfo = gc.getVoteSourceInfo(iVoteSource)
			if srcInfo:
				iSpecialist = srcInfo.getFreeSpecialist()
				if iSpecialist > -1:
					screen.attachImageButton(panelName, "", gc.getSpecialistInfo(iSpecialist).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_SPECIALIST, iSpecialist, 1, False)
					return
		draw_none_text(screen, self.top, self.X_SOURCE_SPECIALIST, self.Y_SOURCE_SPECIALIST, self.W_SOURCE_SPECIALIST, self.H_SOURCE_SPECIALIST)

	def placeSourceCivic(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_VOTE_SOURCE_CIVIC_PANEL", ()), "", False, True, self.X_SOURCE_CIVIC, self.Y_SOURCE_CIVIC, self.W_SOURCE_CIVIC, self.H_SOURCE_CIVIC, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		iVoteSource = self._getVoteSourceType()
		if iVoteSource > -1:
			srcInfo = gc.getVoteSourceInfo(iVoteSource)
			if srcInfo:
				iCivic = srcInfo.getCivic()
				if iCivic > -1:
					screen.attachImageButton(panelName, "", gc.getCivicInfo(iCivic).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, iCivic, 1, False)
					return
		draw_none_text(screen, self.top, self.X_SOURCE_CIVIC, self.Y_SOURCE_CIVIC, self.W_SOURCE_CIVIC, self.H_SOURCE_CIVIC)

	def placeForceCivics(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_VOTE_FORCE_CIVICS", ()), "", False, True, self.X_FORCE_CIVICS, self.Y_FORCE_CIVICS, self.W_FORCE_CIVICS, self.H_FORCE_CIVICS, PanelStyles.PANEL_STYLE_BLUE50)
		screen.attachLabel(panelName, "", "  ")

		voteInfo = gc.getVoteInfo(self.iVote)
		bFound = False
		if voteInfo:
			# <!-- custom: panel is sized for one civic button in AdvCiv-SAS because votes are expected to force at most one civic; if modders add multiple forced civics later, extra buttons are still appended in sequence. (GPT-5.4?) -->
			for iCivic in range(gc.getNumCivicInfos()):
				if voteInfo.isForceCivic(iCivic):
					ci = gc.getCivicInfo(iCivic)
					screen.attachImageButton(panelName, "", ci.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, iCivic, 1, False)
					bFound = True
		if not bFound:
			draw_none_text(screen, self.top, self.X_FORCE_CIVICS, self.Y_FORCE_CIVICS, self.W_FORCE_CIVICS, self.H_FORCE_CIVICS)

	def placeVoteNameLegend(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_SAS_VOTE_NAME", ()), "", True, False, self.X_VOTE_NAME, self.Y_VOTE_NAME, self.W_VOTE_NAME, self.H_VOTE_NAME, PanelStyles.PANEL_STYLE_BLUE50)

		voteInfo = gc.getVoteInfo(self.iVote)
		szVoteName = localText.getText("TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_NONE", ())
		if voteInfo:
			szVoteName = voteInfo.getDescription()
		textName = self.top.getNextWidgetName()
		screen.addMultilineText(textName, SASTextScale.labelText(self.BULLET_PREFIX + szVoteName), self.X_VOTE_NAME + 14, self.Y_VOTE_NAME + 34, self.W_VOTE_NAME - 28, self.H_VOTE_NAME - 40, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeSpecial(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_EFFECTS", ()), "", True, False, self.X_SPECIAL, self.Y_SPECIAL, self.W_SPECIAL, self.H_SPECIAL, PanelStyles.PANEL_STYLE_BLUE50)

		voteInfo = gc.getVoteInfo(self.iVote)
		lines = []
		if voteInfo:
			# Effect flags — emit one line per enabled flag. Keys reused where possible.
			if voteInfo.isCityVoting():
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_EFFECT_CITY_VOTING", ()))
			if voteInfo.isCivVoting():
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_EFFECT_CIV_VOTING", ()))
			if voteInfo.isSecretaryGeneral():
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_EFFECT_SECRETARY_GENERAL", ()))
			if voteInfo.isVictory():
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_EFFECT_VICTORY", ()))
			if voteInfo.isFreeTrade():
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_EFFECT_FREE_TRADE", ()))
			if voteInfo.isNoNukes():
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_EFFECT_NO_NUKES", ()))
			if voteInfo.isDefensivePact():
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_EFFECT_DEFENSIVE_PACT", ()))
			if voteInfo.isOpenBorders():
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_EFFECT_OPEN_BORDERS", ()))
			if voteInfo.isForcePeace():
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_EFFECT_FORCE_PEACE", ()))
			if voteInfo.isForceNoTrade():
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_EFFECT_FORCE_NO_TRADE", ()))
			if voteInfo.isForceWar():
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_EFFECT_FORCE_WAR", ()))
			if voteInfo.isAssignCity():
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_EFFECT_ASSIGN_CITY", ()))
			# <!-- custom: CvGame::processVote applies getTradeRoutes globally when the resolution passes; it is an effect, not a prerequisite.
			# Keeping it only here prevents Single Currency from showing the same trade-route reward under both Requirements and Effects. See KI#227. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			iTradeRoutesEffect = voteInfo.getTradeRoutes()
			if iTradeRoutesEffect != 0:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_EFFECT_TRADE_ROUTES_ALL_CITIES", ()) + u": %+d%s" % (iTradeRoutesEffect, self.TRADE_CHAR))
			bHasForcedCivic = False
			for iCivic in range(gc.getNumCivicInfos()):
				if voteInfo.isForceCivic(iCivic):
					bHasForcedCivic = True
					break
			if bHasForcedCivic:
				lines.append(self.BULLET_PREFIX + localText.getText("TXT_KEY_PEDIA_SAS_VOTE_EFFECT_FORCE_CIVICS_SEE_PANEL", ()))

		listName = self.top.getNextWidgetName()
		if lines:
			szText = u"\n".join(lines)
		else:
			szText = localText.getText("TXT_KEY_PEDIA_SAS_NO_BUTTON_FOUND_NONE", ())
		screen.addMultilineText(listName, SASTextScale.labelText(szText), self.X_SPECIAL + 14, self.Y_SPECIAL + 34, self.W_SPECIAL - 28, self.H_SPECIAL - 46, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def setHistoryExpanded(self, bExpanded):
		self.bHistoryExpanded = bExpanded

	def placeHistory(self):
		screen = self.top.getScreen()
		szText = SASTextScale.normalizeLabelText(gc.getVoteInfo(self.iVote).getCivilopedia())
		szTitle = localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ())
		draw_expandable_text_panel(screen, self.top, szTitle, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, szText, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)

	def handleInput(self, inputClass):
		return 0
