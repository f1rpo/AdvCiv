# SevoPediaIndex
#
# Copyright (c) 2008 The BUG Mod.
#
# EF: Converted from Civilopedia version by fitchn.

from CvPythonExtensions import *
import CvUtil
import ScreenInput
import SevoScreenEnums
import BugUtil
from SASFontUtils import *
from _sevopedia_helpers import place_new_concept_legend_link

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class SevoPediaIndex:

	def __init__(self, main):
		self.top = main

		self.LIST_BUTTON_SIZE = 24
		self.SAS_indexSetLayout(False)

		self.index = None
		self.letterTextIDs = None
		# <!-- custom: filter reads SevoPediaMain.SAS_szSearchString; the shared top-header search bar is drawn by SevoPediaMain.SAS_syncSearchPanel and refreshed via SAS_refreshActiveListView.
		# This page owns no search state of its own. (Claude code Opus 4.7) -->
		self.SAS_indexWidgetNames = []

	def SAS_indexSetLayout(self, bCategory):
		if bCategory:
			self.X_INDEX = self.top.X_ITEMS
			self.Y_INDEX = self.top.Y_ITEMS
			self.W_INDEX = self.top.W_SCREEN - self.top.X_ITEMS
			self.H_INDEX = self.top.H_ITEMS
		else:
			self.X_INDEX = self.top.X_CATEGORIES
			self.Y_INDEX = self.top.Y_CATEGORIES
			self.W_INDEX = self.top.W_SCREEN - 2 * self.top.X_CATEGORIES
			self.H_INDEX = self.top.H_CATEGORIES

		self.X_LETTER = self.X_INDEX + 130  # position of first letter button
		self.Y_LETTER = self.Y_INDEX
		self.W_LETTER = 20

	def interfaceScreen(self, bCategory=False):
		self.SAS_indexSetLayout(bCategory)
		self.buildIndex()
		self.placeIndex()

	def buildIndex(self):
		if self.index:
			return

		techList = self.top.getTechList()
		unitList = self.top.getUnitList()
		unitCombatList = self.top.getUnitCategoryList()
		promotionList = self.top.getPromotionList()

		buildingList = self.top.getBuildingList()
		nationalWonderList = self.top.getNationalWonderList()
		worldWonderList = self.top.getWorldWonderList()
		projectList = self.top.getProjectList()
		specialistList = self.top.getSpecialistList()

		terrainList = self.top.getTerrainList()
		featureList = self.top.getFeatureList()
		bonusList = self.top.getBonusList()
		improvementList = self.top.getImprovementList()

		civList = self.top.getCivilizationList()
		leaderList = self.top.getLeaderList()
		traitList = self.top.getTraitList()

		civicList = self.top.getCivicList()
		religionList = self.top.getReligionList()
		corporationList = self.top.getCorporationList()

		conceptList = self.top.getConceptList()
		newConceptList = self.top.getNewConceptList()
		# <!-- custom: add Builds to index, inspired by Middle-earth mod's PlatyPedia approach (Claude Opus 4.5) -->
		buildList = self.top.getBuildList()

		# <!-- custom: Note: keep Index list/cell handling local and direct instead of sharing Main's per-category widget metadata.
		# Index is one flattened table while Main drives many independent pedia pages, so sharing would push Index-only rules into Main code for no real reuse win. (GPT-5.5) -->
		# <!-- custom: Dropped the legacy TXT_KEY_* prefix-strip and "The X" comma-flip sort-key cleanup here (sorted the same items differently in Index vs the type-specific pedia pages, hurt diagnosis of missing translations, needless per-entry build-time cost in any locale - and especially wasteful in non-English ones where "The X" never matches anyway, and needless code complexity). See KI#133 for full rationale. (Claude code Opus 4.7) -->
		list=[]
		for item in techList:
			list.append([item[0],"Tech",item])
		for item in unitList:
			list.append([item[0],"Unit",item])
		for item in unitCombatList:
			list.append([item[0],"UnitCombat",item])
		for item in promotionList:
			list.append([item[0],"Promo",item])

		for item in buildingList:
			list.append([item[0],"Building",item])
		for item in nationalWonderList:
			list.append([item[0],"Wonder",item])
		for item in worldWonderList:
			list.append([item[0],"Wonder",item])
		for item in projectList:
			list.append([item[0],"Project",item])
		for item in specialistList:
			list.append([item[0],"Specialist",item])

		for item in terrainList:
			list.append([item[0],"Terrain",item])
		for item in featureList:
			list.append([item[0],"Feature",item])
		for item in bonusList:
			list.append([item[0],"Bonus",item])
		for item in improvementList:
			list.append([item[0],"Improv",item])

		for item in civList:
			list.append([item[0],"Civ",item])
		for item in leaderList:
			list.append([item[0],"Leader",item])
		# <!-- custom: traitList entries have a 2-char leading icon prefix unrelated to TXT_KEY/"The"; keep this strip. (Claude code Opus 4.7 + GPT-5.5) -->
		for item in traitList:
			list.append([item[0][2:],"Trait",item])

		for item in religionList:
			list.append([item[0],"Religion",item])
		for item in civicList:
			list.append([item[0],"Civic",item])

		for item in conceptList:
			list.append([item[0],"Concept",item])
		for item in newConceptList:
			list.append([item[0],"NewConcept",item])
		# <!-- custom: add Builds to index (Claude Opus 4.5) -->
		for item in buildList:
			list.append([item[0],"Build",item])

		list.sort()
		self.index = list

	def placeIndex(self):
		screen = self.top.getScreen()
		CONCEPT_CHAR = gc.getYieldInfo(YieldTypes.YIELD_COMMERCE).getChar()
		place_new_concept_legend_link(self.top, "CONCEPT_SAS_SEVOPEDIA_CATEGORIES_LEGEND")

		if self.SAS_indexWidgetNames:
			for szWidget in self.SAS_indexWidgetNames:
				try:
					screen.deleteWidget(szWidget)
				except:
					pass
			self.SAS_indexWidgetNames = []

		# <!-- custom: draw the shared top-header search bar from SevoPediaMain, and register this method as the active refresher so Main's search handlers can invoke it on each keystroke without needing any category-specific branching. (Claude code Opus 4.7) -->
		self.top.SAS_syncSearchPanel()
		self.top.SAS_activeListRefresher = self.placeIndex
		# <!-- custom: register Index's own arrow-key navigator and reset the cell list / cursor.
		# UP/DOWN steps cell-by-cell in reading order (left->right, top->bottom) by re-rendering the previous and current cells with text highlight; the widget has no per-cell focus API. (Claude code Opus 4.7 + GPT-5.5) -->
		self.top.SAS_activeKeyNavigator = self.SAS_navigateIndexTable
		self.SAS_indexCells = []
		self.SAS_indexCursorPos = -1

		nColumns = 3
		self.tableName = self.top.getNextWidgetName()
		self.SAS_rowToBuild = {}
		self.SAS_rowToTrait = {}  # <!-- custom: row-to-trait mapping for WIDGET_PYTHON trait handling. (Claude Opus 4.5) -->
		self.iTableWidgetId = int(self.tableName.replace(self.top.WIDGET_ID, ""))
		# <!-- custom: search bar lives in the top header, so the Index table uses the full Y_INDEX area below it. (Claude code Opus 4.7) -->
		iTableY = self.Y_INDEX
		iTableH = self.H_INDEX
		# <!-- custom: For Build entries, table selection is the only reliable click signal, so keep the table selectable and capture row->Build mapping in handleInput.
		# We previously tried overlay buttons, but they ignored table scrolling and desynced from rows.
		# Also, if the table doesn't have focus on first open, NOTIFY_CHARACTER goes nowhere and the search bar appears "dead" until you navigate away and back.
		# Setting focus here keeps search responsive and avoids the broken first-load behavior. Credit: Claude Opus 4.5 + GPT-5.2-Codex. (GPT-5.2-Codex (summarized)) -->
		screen.addTableControlGFC(self.tableName, nColumns, self.X_INDEX, iTableY, self.W_INDEX, iTableH, True, True, self.LIST_BUTTON_SIZE, self.LIST_BUTTON_SIZE, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSelect(self.tableName, True)
		screen.setFocus(self.tableName)
		self.SAS_indexWidgetNames.append(self.tableName)
		for i in range(nColumns):
			screen.setTableColumnHeader(self.tableName, i, "", (self.W_INDEX - 10) / nColumns)

		iRow = -1
		iColumn = 0
		sLetter = "#"
		iX = self.X_LETTER
		iLetterY = self.Y_INDEX
		self.letterTextIDs = {}
		# <!-- custom: note: while adding leaderhead art_def in AdvCiv-SAS-NIF-Gallery mod we saw the error "UnicodeDecodeError: 'ascii' codec can't decode byte 0xc8 in position 0" and in Sevopedia Leader, fixed by respecting path case sensitivity (e.g. "Art/LeaderHeads").
		# So reverted a previous patch that would workaround that: prefer to fail loudly instead and fix path or asset cause directly rather. See KI#111. (GPT-5.3-Codex) -->
		szFilter = self.top.SAS_szSearchString.strip().lower()
		bFilter = (len(szFilter) > 0)
		for name, type, item in self.index:
			if item[1] < 0:
				continue
			if bFilter and name.lower().find(szFilter) == -1:
				continue
			if (name[:1] != sLetter):
				sLetter = name[:1]
				screen.appendTableRow(self.tableName)
				iRow += 1
				screen.setTableText(self.tableName, 1, iRow, sasFontTagTitle + u"- " + sLetter + u" -" + SAS_FONT_TAG_CLOSE, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
				screen.appendTableRow(self.tableName)
				# create letter button
				textName = self.top.getNextWidgetName()
				letterText = sasFontTagTitle + sLetter + SAS_FONT_TAG_CLOSE
				screen.setText(textName, "Background", letterText, CvUtil.FONT_CENTER_JUSTIFY, iX, iLetterY, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, iRow, -1)
				self.letterTextIDs[textName] = iRow
				self.SAS_indexWidgetNames.append(textName)
				iX += self.W_LETTER
				iRow += 1
				iColumn = 0
			else:
				iColumn += 1
				if (iColumn >= nColumns):
					screen.appendTableRow(self.tableName)
					iRow += 1
					iColumn = 0

			# <!-- custom: refactor, since sText was defined in existing code, it seems we can reuse it instead of hardcoding it again at each call; this also fixes ruff warning and according to chatgpt; similarly removed unused lines `sButton = ""` and `eWidget = None` and as for lines `iData1 = item[1]` and `iData2 = 1` also using them as variables similarly instead of hardcoding them each time (assuming they are not actually useful/executed instructions) -->
			sText = sasFontTagLabel + item[0] + SAS_FONT_TAG_CLOSE
			iData1 = item[1]
			iData2 = 1
			# <!-- custom: setTableText calls now go through _SAS_indexPlaceCell so cell args are also recorded for arrow-key cursor re-render. (Claude code Opus 4.7) -->
			if (type == "Tech"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getTechInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iData1, iData2)
			elif (type == "Unit"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getUnitInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iData1, iData2)
			elif (type == "UnitCombat"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getUnitCombatInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT_COMBAT, iData1, iData2)
			elif (type == "Promo"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getPromotionInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, iData1, iData2)

			elif (type == "Building"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getBuildingInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iData1, iData2)
			elif (type == "Wonder"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getBuildingInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iData1, iData2)
			elif (type == "Project"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getProjectInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROJECT, iData1, iData2)
			elif (type == "Specialist"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getSpecialistInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_SPECIALIST, iData1, iData2)

			elif (type == "Terrain"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getTerrainInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_TERRAIN, iData1, iData2)
			elif (type == "Feature"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getFeatureInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_FEATURE, iData1, iData2)
			elif (type == "Bonus"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getBonusInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iData1, iData2)
			elif (type == "Improv"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getImprovementInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_IMPROVEMENT, iData1, iData2)
			# <!-- custom: Build rows use the normal table cell (icon + text) and rely on table selection to trigger pediaJump.
			# This avoids overlay widgets that don't scroll with the table. Credit: Claude Opus 4.5 + GPT-5.2-Codex. (GPT-5.2-Codex (summarized)) -->
			elif (type == "Build"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getBuildInfo(iData1).getButton(), WidgetTypes.WIDGET_HELP_IMPROVEMENT, gc.getBuildInfo(iData1).getTechPrereq(), iData1)
				self.SAS_rowToBuild[iRow] = iData1

			elif (type == "Civ"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getCivilizationInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIV, iData1, iData2)
			elif (type == "Leader"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getLeaderHeadInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_LEADER, iData1, iData2)
			# <!-- custom: Trait rows use WIDGET_GENERAL and row-to-trait mapping like Builds. (Claude Opus 4.5) -->
			elif (type == "Trait"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getTraitInfo(iData1).getButton(), WidgetTypes.WIDGET_GENERAL, -1, -1)
				self.SAS_rowToTrait[iRow] = iData1

			elif (type == "Civic"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getCivicInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, iData1, iData2)
			elif (type == "Religion"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getReligionInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION, iData1, iData2)
			# <!-- custom: base AdvCiv bugfix GPT-5.2-Codex found thanks, was gc.getReligionInfo(iData1).getButton() -->
			elif (type == "Corporation"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sText, gc.getCorporationInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_CORPORATION, iData1, iData2)

			elif (type == "Concept"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sasFontTagLabel + (u"%c %s" % (CONCEPT_CHAR, item[0])) + SAS_FONT_TAG_CLOSE, gc.getConceptInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_DESCRIPTION, CivilopediaPageTypes.CIVILOPEDIA_PAGE_CONCEPT, iData1)
			elif (type == "NewConcept"):
				self._SAS_indexPlaceCell(screen, iRow, iColumn, sasFontTagLabel + (u"%c %s" % (CONCEPT_CHAR, item[0])) + SAS_FONT_TAG_CLOSE, gc.getConceptInfo(iData1).getButton(), WidgetTypes.WIDGET_PEDIA_DESCRIPTION, CivilopediaPageTypes.CIVILOPEDIA_PAGE_CONCEPT_NEW, iData1)

		self.iLastRow = iRow

	# <!-- custom: helper used during placeIndex so every cell's args are remembered for arrow-key re-render.
	# Keeps the placement loop one-line-per-type while ensuring SAS_indexCells stays in sync with what's actually drawn. (Claude code Opus 4.7) -->
	def _SAS_indexPlaceCell(self, screen, iRow, iColumn, sText, sButton, eWidget, iData1, iData2):
		screen.setTableText(self.tableName, iColumn, iRow, sText, sButton, eWidget, iData1, iData2, CvUtil.FONT_LEFT_JUSTIFY)
		self.SAS_indexCells.append((iRow, iColumn, sText, sButton, eWidget, iData1, iData2))

	# <!-- custom: cell-by-cell UP/DOWN navigation in reading order across the 3-column table.
	# The widget has no per-cell highlight API, so the "cursor" is drawn by re-rendering the previous and current cells via setTableText with COLOR_HIGHLIGHT_TEXT (the same color the items list uses elsewhere).
	# selectRow is called too, and the table is refocused first so its built-in row-selection visual can paint if the search bar had stolen focus. (Claude code Opus 4.7) -->
	def SAS_navigateIndexTable(self, iDirection):
		if not self.SAS_indexCells:
			return False
		iLast = len(self.SAS_indexCells) - 1
		if self.SAS_indexCursorPos < 0:
			if iDirection > 0:
				iNewPos = 0
			else:
				iNewPos = iLast
		else:
			iNewPos = self.SAS_indexCursorPos + iDirection
			if iNewPos < 0:
				iNewPos = 0
			elif iNewPos > iLast:
				iNewPos = iLast
		if iNewPos == self.SAS_indexCursorPos:
			return False
		screen = self.top.getScreen()
		new = self.SAS_indexCells[iNewPos]
		# <!-- custom: row-level visual = native blue via selectRow (deselect-prev-then-select-new, same call pattern SevoPediaMain.placeItems uses; this is the only call sequence that empirically paints addTableControlGFC selections in this widget). Cell-level visual = re-render previous cell with its original text and current cell wrapped in COLOR_HIGHLIGHT_TEXT, since selectRow has no per-cell granularity. Refocus the table first because the search bar steals focus on typing. (Claude code Opus 4.7 + GPT-5.5) -->
		screen.setFocus(self.tableName)
		screen.enableSelect(self.tableName, True)
		if self.SAS_indexCursorPos >= 0:
			prev = self.SAS_indexCells[self.SAS_indexCursorPos]
			screen.selectRow(self.tableName, prev[0], False)
			# Re-render the previous cell with its original (uncolored) text.
			screen.setTableText(self.tableName, prev[1], prev[0], prev[2], prev[3], prev[4], prev[5], prev[6], CvUtil.FONT_LEFT_JUSTIFY)
		screen.selectRow(self.tableName, new[0], True)
		# Re-render the new cell wrapped in COLOR_HIGHLIGHT_TEXT to mark which of the row's cells is "current".
		sHighlighted = localText.changeTextColor(new[2], self.top.COLOR_HIGHLIGHT_TEXT)
		screen.setTableText(self.tableName, new[1], new[0], sHighlighted, new[3], new[4], new[5], new[6], CvUtil.FONT_LEFT_JUSTIFY)
		self.SAS_indexCursorPos = iNewPos
		return True

	def handleInput (self, inputClass):
		BugUtil.debugInput(inputClass)
		# <!-- custom: search typing + CLEAR live in SevoPediaMain.handleInput; this method only handles letter buttons and table-row clicks specific to the Index page. (Claude code Opus 4.7) -->
		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED and inputClass.getFunctionName() + str(inputClass.getID()) in self.letterTextIDs):
			screen = self.top.getScreen()
			screen.selectRow(self.tableName, self.iLastRow, True)
			screen.selectRow(self.tableName, inputClass.getData1(), True)
			return 1

		if inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED and inputClass.getButtonType() == WidgetTypes.WIDGET_HELP_IMPROVEMENT:
			iBuild = inputClass.getData2()
			# <!-- custom: Build rows now use WIDGET_HELP_IMPROVEMENT to restore hover text in Index; route click explicitly to Sevopedia Builds so behavior matches other Build entries. See KI#113. (GPT-5.3-Codex) -->
			if iBuild >= 0 and iBuild < gc.getNumBuildInfos():
				return self.top.pediaJump(SevoScreenEnums.PEDIA_BUILDS, iBuild, True, False)

		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED or inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED):
			if inputClass.getFunctionName() == self.top.WIDGET_ID and inputClass.getID() == self.iTableWidgetId:
				iRow = inputClass.getData()
				iBuild = self.SAS_rowToBuild.get(iRow, None)
				if iBuild is not None:
					return self.top.pediaJump(SevoScreenEnums.PEDIA_BUILDS, iBuild, True, False)
				# <!-- custom: Handle trait clicks via row mapping. (Claude Opus 4.5) -->
				iTrait = self.SAS_rowToTrait.get(iRow, None)
				if iTrait is not None:
					return self.top.pediaJump(SevoScreenEnums.PEDIA_TRAITS, iTrait, True, False)
		return 0
