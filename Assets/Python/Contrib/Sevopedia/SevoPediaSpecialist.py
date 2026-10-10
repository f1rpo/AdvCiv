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
# <!-- custom: enhanced in AdvCiv-SAS with new Unlocked with and Yields panel based on the Middle-earth mod's Platypedi's specialists page ("C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\Middle-earth\Assets\Python\Screens\PlatyPedia\PlatyPediaSpecialist.py"), with the help of GPT-5.2-Codex thanks a lot! -->
#

from CvPythonExtensions import *
from SASMagicNumbers import *
import CvUtil
import ScreenInput
import SevoScreenEnums
import SASTextScale

from _sevopedia_helpers import *

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class SevoPediaSpecialist:

	def __init__(self, main):
		self.iSpecialist = -1
		self.bHistoryExpanded = False
		self.top = main

		self.H_TOP_PANEL = 200
		self.W_ICON_PANEL = 200

		self.X_ICON_PANEL = self.top.X_PEDIA_PAGE
		self.Y_ICON_PANEL = self.top.Y_PEDIA_PAGE

		self.X_LEFT = self.X_ICON_PANEL
		self.W_LEFT = (self.top.R_PEDIA_PAGE - self.X_LEFT - MEDIUM_MARGIN) / 2
		self.X_RIGHT = self.X_LEFT + self.W_LEFT + MEDIUM_MARGIN
		self.W_RIGHT = self.top.R_PEDIA_PAGE - self.X_RIGHT

		self.X_EFFECTS_PANEL = self.X_ICON_PANEL + self.W_ICON_PANEL + MEDIUM_MARGIN
		self.Y_EFFECTS_PANEL = self.Y_ICON_PANEL
		self.W_EFFECTS_PANEL = self.W_LEFT - self.W_ICON_PANEL - MEDIUM_MARGIN

		self.W_EXTRA_SLOTS = (self.W_RIGHT - SMALL_MARGIN) / 2
		self.W_EXTRA_YIELDS = self.W_EXTRA_SLOTS

		self.X_EXTRA_SLOTS = self.X_RIGHT
		self.Y_EXTRA_SLOTS = self.top.Y_PEDIA_PAGE
		self.H_EXTRA_SLOTS = self.top.B_PEDIA_PAGE - self.Y_EXTRA_SLOTS

		self.X_EXTRA_YIELDS = self.X_EXTRA_SLOTS + self.W_EXTRA_SLOTS + SMALL_MARGIN
		self.Y_EXTRA_YIELDS = self.Y_EXTRA_SLOTS
		self.H_EXTRA_YIELDS = self.H_EXTRA_SLOTS

		self.X_HISTORY = self.X_ICON_PANEL
		self.Y_HISTORY = self.Y_ICON_PANEL + self.H_TOP_PANEL + SMALL_MARGIN
		self.W_HISTORY = self.W_LEFT
		self.H_HISTORY = self.top.B_PEDIA_PAGE - self.Y_HISTORY

		self.W_ICON = 150
		self.H_ICON = 150
		self.X_ICON = self.X_ICON_PANEL + (self.W_ICON_PANEL - self.W_ICON) / 2
		self.Y_ICON = self.Y_ICON_PANEL + (self.H_TOP_PANEL - self.H_ICON) / 2

	def interfaceScreen(self, iSpecialist):
		if self.iSpecialist != iSpecialist:
			self.bHistoryExpanded = False
		self.iSpecialist = iSpecialist

		self.placeSpecialistPane()
		self.placeEffects()
		self.placeExtraSlots()
		self.placeYields()
		self.placeHistory()

	def placeSpecialistPane(self):
		screen = self.top.getScreen()

		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON_PANEL, self.Y_ICON_PANEL, self.W_ICON_PANEL, self.H_TOP_PANEL, PanelStyles.PANEL_STYLE_BLUE50)
		# <!-- custom: no need for the blue frame on blue background, use transparent instead -->
		#screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_MAIN)
		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_EMPTY)
		screen.addDDSGFC(self.top.getNextWidgetName(), gc.getSpecialistInfo(self.iSpecialist).getButton(), self.X_ICON + self.W_ICON/2 - PANE_ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - PANE_ICON_SIZE/2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def placeEffects(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		# <!-- custom: no header for more compact and prettier display -->
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_EFFECTS_SPECIALISTS", ()), "", True, False, self.X_EFFECTS_PANEL, self.Y_EFFECTS_PANEL, self.W_EFFECTS_PANEL, self.H_TOP_PANEL, PanelStyles.PANEL_STYLE_BLUE50)
		textName = self.top.getNextWidgetName()
		szSpecialText = CyGameTextMgr().getSpecialistHelp(self.iSpecialist, True)
		szSpecialText = SASTextScale.normalizeLabelText(szSpecialText)
		# <!-- custom: reduce top padding now that the traits header is removed (GPT-5.2-Codex).
		# Was Y + headerExtraHeight (i.e. + 10) -->
		headerExtraHeight = 10
		screen.addMultilineText(textName, szSpecialText, self.X_EFFECTS_PANEL + 5, self.Y_EFFECTS_PANEL - headerExtraHeight, self.W_EFFECTS_PANEL - 10, self.H_TOP_PANEL - headerExtraHeight, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def _append_change_text(self, szText, iChange, iChar):
		if iChange != 0:
			if szText:
				szText += u", "
			szText += u"%+d%c" % (iChange, iChar)
		return szText

	def _format_slot_label(self, iCount, iFree):
		szText = u""
		if iCount > 0:
			szText = u"+%d" % iCount
		if iFree > 0:
			if szText:
				szText += u", "
			szText += u"+%d Free" % iFree
		return szText

	# <!-- custom: use the bonus-yields row template for consistency. (Claude code Opus 4.7) -->
	def placeExtraSlots(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, u"Extra Slots", "", True, True, self.X_EXTRA_SLOTS, self.Y_EXTRA_SLOTS, self.W_EXTRA_SLOTS, self.H_EXTRA_SLOTS, PanelStyles.PANEL_STYLE_BLUE50)

		bAnyFound = False
		specialSlots = {}

		# <!-- custom: note: unlike temples, monasteries, and cathedrals, shrines are not a special building, so showing the first entry (as of now Pagan Shrine) only instead for concision since the effect is always the same anyway -->
		for iBuilding in xrange(gc.getNumBuildingInfos()):
			buildingInfo = gc.getBuildingInfo(iBuilding)
			iCount = buildingInfo.getSpecialistCount(self.iSpecialist)
			iFree = buildingInfo.getFreeSpecialistCount(self.iSpecialist)
			if iCount > 0 or iFree > 0:
				szText = self._format_slot_label(iCount, iFree)
				if szText:
					iSpecialBuilding = buildingInfo.getSpecialBuildingType()
					if iSpecialBuilding > -1:
						key = ("special", iSpecialBuilding)
						if key not in specialSlots:
							# <!-- custom: This representative group icon has no concrete building or tech context; neutral help avoids interpreting NO_TECH as TechPrereqAnyone and claiming that any player can construct it. See KI#389. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
							specialSlots[key] = [0, 0, gc.getSpecialBuildingInfo(iSpecialBuilding).getButton(), WidgetTypes.WIDGET_GENERAL, -1, -1]
						if iCount > specialSlots[key][0]:
							specialSlots[key][0] = iCount
						if iFree > specialSlots[key][1]:
							specialSlots[key][1] = iFree
					elif buildingInfo.getHolyCity() != -1:
						key = ("shrine", -1)
						if key not in specialSlots:
							specialSlots[key] = [0, 0, buildingInfo.getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, 1]
						if iCount > specialSlots[key][0]:
							specialSlots[key][0] = iCount
						if iFree > specialSlots[key][1]:
							specialSlots[key][1] = iFree
					else:
						attach_button_label_row(screen, self.top, panelName, buildingInfo.getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, 1, szText)
						bAnyFound = True

		for key in sorted(specialSlots.keys()):
			szText = self._format_slot_label(specialSlots[key][0], specialSlots[key][1])
			if szText:
				attach_button_label_row(screen, self.top, panelName, specialSlots[key][2], specialSlots[key][3], specialSlots[key][4], specialSlots[key][5], szText)
				bAnyFound = True

		for iCivic in xrange(gc.getNumCivicInfos()):
			civicInfo = gc.getCivicInfo(iCivic)
			if civicInfo.isSpecialistValid(self.iSpecialist):
				attach_button_label_row(screen, self.top, panelName, civicInfo.getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, iCivic, 1, "Unlimited")
				bAnyFound = True

		for iVoteSource in xrange(gc.getNumVoteSourceInfos()):
			voteSourceInfo = gc.getVoteSourceInfo(iVoteSource)
			if voteSourceInfo.getFreeSpecialist() == self.iSpecialist:
				iVoteTarget = -1
				for iVote in xrange(gc.getNumVoteInfos()):
					voteInfo = gc.getVoteInfo(iVote)
					if voteInfo and voteInfo.isVoteSourceType(iVoteSource):
						iVoteTarget = iVote
						break
				for iBuilding in xrange(gc.getNumBuildingInfos()):
					buildingInfo = gc.getBuildingInfo(iBuilding)
					if buildingInfo.getVoteSourceType() == iVoteSource:
						# <!-- custom: jump to Votes (not Building) because this free specialist comes from VoteSourceInfo, while the building button is only the host of that vote source. (GPT-5.3-Codex) -->
						if iVoteTarget > -1:
							attach_button_label_row(screen, self.top, panelName, buildingInfo.getButton(), WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_VOTE_ENTRY, iVoteTarget, "+1 Free (V.Sources)")
						else:
							attach_button_label_row(screen, self.top, panelName, buildingInfo.getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, 1, "+1 Free (V.Sources)")
						bAnyFound = True
						break

		if not bAnyFound:
			yPanelCenter = self.Y_EXTRA_SLOTS + (self.H_EXTRA_SLOTS / 2)
			textName = self.top.getNextWidgetName()
			screen.addMultilineText(textName, SASTextScale.labelText("No extra slots gained"), self.X_EXTRA_SLOTS + 7, yPanelCenter, self.W_EXTRA_SLOTS - 14, self.H_EXTRA_SLOTS - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeYields(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, u"Extra Yields", "", True, True, self.X_EXTRA_YIELDS, self.Y_EXTRA_YIELDS, self.W_EXTRA_YIELDS, self.H_EXTRA_YIELDS, PanelStyles.PANEL_STYLE_BLUE50)

		entries = []

		for iCivic in xrange(gc.getNumCivicInfos()):
			civicInfo = gc.getCivicInfo(iCivic)
			szText = u""
			for k in xrange(CommerceTypes.NUM_COMMERCE_TYPES):
				szText = self._append_change_text(szText, civicInfo.getSpecialistExtraCommerce(k), gc.getCommerceInfo(k).getChar())
			if szText:
				entries.append((civicInfo.getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, iCivic, szText))

		for iTech in xrange(gc.getNumTechInfos()):
			techInfo = gc.getTechInfo(iTech)
			szText = u""
			for k in xrange(CommerceTypes.NUM_COMMERCE_TYPES):
				szText = self._append_change_text(szText, techInfo.getSpecialistExtraCommerce(k), gc.getCommerceInfo(k).getChar())
			if szText:
				entries.append((techInfo.getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, szText))

		for iBuilding in xrange(gc.getNumBuildingInfos()):
			buildingInfo = gc.getBuildingInfo(iBuilding)
			szText = u""
			for k in xrange(YieldTypes.NUM_YIELD_TYPES):
				szText = self._append_change_text(szText, buildingInfo.getSpecialistYieldChange(self.iSpecialist, k), gc.getYieldInfo(k).getChar())
			# <!-- custom: Building SpecialistExtraCommerce applies to every specialist, independently of the per-specialist yield map already shown here.
			# Include both effects in the same Building row. See KI#303. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			for k in xrange(CommerceTypes.NUM_COMMERCE_TYPES):
				szText = self._append_change_text(szText, buildingInfo.getSpecialistExtraCommerce(k), gc.getCommerceInfo(k).getChar())
			if szText:
				entries.append((buildingInfo.getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, szText))

		for buttonPath, widgetType, widgetID, szText in entries:
			attach_button_label_row(screen, self.top, panelName, buttonPath, widgetType, widgetID, 1, szText)

		if not entries:
			yPanelCenter = self.Y_EXTRA_YIELDS + (self.H_EXTRA_YIELDS / 2)
			textName = self.top.getNextWidgetName()
			screen.addMultilineText(textName, SASTextScale.labelText("No extra yields gained"), self.X_EXTRA_YIELDS + 7, yPanelCenter, self.W_EXTRA_YIELDS - 14, self.H_EXTRA_YIELDS - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def setHistoryExpanded(self, bExpanded):
		self.bHistoryExpanded = bExpanded

	def placeHistory(self):
		screen = self.top.getScreen()
		# <!-- custom: pre-normalize so the helper's labelText call is a no-op (specialist civilopedia entries need normalizeLabelText like leaders). -->
		szText = SASTextScale.normalizeLabelText(gc.getSpecialistInfo(self.iSpecialist).getCivilopedia())
		szTitle = localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ())
		draw_expandable_text_panel(screen, self.top, szTitle, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, szText, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)
	def handleInput (self, inputClass):
		return 0

