# Sid Meier's Civilization 4
# Copyright Firaxis Games 2005
#
# Sevopedia 2.3
#   sevotastic.blogspot.com
#   sevotastic@yahoo.com
#
# additional work by Gaurav, Progor, Ket, Vovan, Fitchn, LunarMongoose
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from CvPythonExtensions import *
import CvUtil
import ScreenInput
import SevoScreenEnums
import CvModName # <!-- custom: Build the dynamic AdvCiv-SAS Version / Source Mods Info page from the centralized runtime resolver and saved game lineage. (ChatGPT-5.6-Sol) -->
from SASFontUtils import *
import SASTextScale

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class SevoPediaHistory:

	def __init__(self, main):
		self.top = main

		self.X_HISTORY = self.top.X_PEDIA_PAGE
		self.Y_HISTORY = self.top.Y_PEDIA_PAGE
		self.H_HISTORY = self.top.H_PEDIA_PAGE
		self.W_HISTORY = self.top.W_PEDIA_PAGE

	def interfaceScreen(self, iEntry):
		self.placeHistory(iEntry)

	def placeHistory(self, iEntry):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, "", "", True, True, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, PanelStyles.PANEL_STYLE_BLUE50)
		szText = self.getCivilopedia(iEntry)
		# <!-- custom: some engine-provided text can carry embedded <font=...>; use normalize* helper (strip + reapply) so SAS scaling is consistent.
		# See SASTextScale normalize comments. (GPT-5.3-Codex) -->
		szText = SASTextScale.normalizeLabelText(szText)
		textName = self.top.getNextWidgetName()
		screen.addMultilineText(textName, szText, self.X_HISTORY + 10, self.Y_HISTORY + 10, self.W_HISTORY - 20, self.H_HISTORY - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def getCivilopedia(self, iEntry):
		if (self.top.iCategory == SevoScreenEnums.PEDIA_CONCEPTS):
			info = gc.getConceptInfo(iEntry)
		else:
			info = gc.getNewConceptInfo(iEntry)
		szText = info.getCivilopedia()
		# <!-- custom: This Mods Info entry is intentionally dynamic: static XML explains the semantics, while current runtime provenance and this game's persisted revision history are appended when the page is opened. (ChatGPT-5.6-Sol) -->
		if info.getType() == "CONCEPT_SAS_VERSION_SOURCE_INFO":
			szText += self.getSASVersionSourceText()
		return szText

	def getSASVersionSourceText(self):
		szUnknown = localText.getText("TXT_KEY_SAS_VERSION_UNKNOWN", ())
		szNone = localText.getText("TXT_KEY_SAS_VERSION_NONE", ())

		def knownText(szValue):
			if szValue:
				return szValue
			return szUnknown

		def dirtyText(iDirty):
			if iDirty < 0:
				return localText.getText("TXT_KEY_SAS_VERSION_DIRTY_UNAVAILABLE", ())
			if iDirty == 0:
				return localText.getText("TXT_KEY_SAS_VERSION_DIRTY_CLEAN", ())
			return localText.getText("TXT_KEY_SAS_VERSION_DIRTY_DIRTY", ())

		iDirty = CvModName.getSourceDirtyState()
		iDirtyFiles = CvModName.getSourceDirtyFileCount()
		szDirtyFiles = CvModName.getSourceDirtyFiles()
		if iDirty < 0:
			szDirtyFiles = szUnknown
		elif iDirty == 0:
			szDirtyFiles = szNone
		elif not szDirtyFiles:
			szDirtyFiles = szUnknown
		szDirtyFileCount = szUnknown
		if iDirtyFiles >= 0:
			szDirtyFileCount = str(iDirtyFiles)

		szText = localText.getText("TXT_KEY_SAS_VERSION_PEDIA_RUNTIME_DETAILS", (
			CvModName.getDisplayNameAndVersion(),
			knownText(CvModName.getVersion()),
			knownText(CvModName.getCommitHash()),
			knownText(CvModName.getBranch()),
			knownText(CvModName.getCommitDate()),
			knownText(CvModName.getSourceMetadataType()),
			dirtyText(iDirty),
			szDirtyFileCount))
		szText += localText.getText("TXT_KEY_SAS_VERSION_PEDIA_RUNTIME_PATH_DETAILS", (
			knownText(CvModName.getFolderName()),
			knownText(CvModName.getPathInRoot()),
			szDirtyFiles))

		g = gc.getGame()
		iEntries = g.getNumSASVersionHistoryEntries()
		if iEntries <= 0:
			return szText + localText.getText("TXT_KEY_SAS_VERSION_PEDIA_NO_GAME_HISTORY", ())

		szText += localText.getText("TXT_KEY_SAS_VERSION_PEDIA_GAME_HISTORY_HEADER", ())
		for iEntry in range(iEntries):
			szRole = localText.getText("TXT_KEY_SAS_VERSION_ROLE_CREATION", ())
			if iEntry > 0:
				szRole = localText.getText("TXT_KEY_SAS_VERSION_ROLE_TRANSITION", ())
			szText += localText.getText("TXT_KEY_SAS_VERSION_PEDIA_GAME_HISTORY_ENTRY", (
				szRole,
				g.getSASVersionHistoryTurn(iEntry),
				knownText(g.getSASVersionHistoryVersion(iEntry)),
				knownText(g.getSASVersionHistoryCommitHash(iEntry)),
				dirtyText(g.getSASVersionHistoryDirtyState(iEntry))))
		return szText

	def handleInput (self, inputClass):
		return 0
