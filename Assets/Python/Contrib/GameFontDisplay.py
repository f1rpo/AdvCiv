
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from CvPythonExtensions import *
# <!-- custom: Route shared visual resources through the centralized SASVisualUtils namespace after retiring overlapping helper/cache imports; keep one canonical visual-resource boundary. (ChatGPT-5.6-Sol) -->
import SASVisualUtils as SASVisuals
import CvUtil
import ScreenInput
import CvScreenEnums
gc = CyGlobalContext()

# advc.gfd: Main class of Nightinggale's GameFont Display component
class GameFontDisplay:
	def __init__(self):
		return

	def interfaceScreen(self):
		screen = CyGInterfaceScreen("GameFontDisplayScreen", CvScreenEnums.GAMEFONT_DISPLAY_SCREEN)

		nScreenWidth = screen.getXResolution()
		nScreenHeight = screen.getYResolution()

		screen.addPanel( "MainBG", u"", u"", True, False, -10, -10, screen.getXResolution() + 20, screen.getYResolution() + 20, PanelStyles.PANEL_STYLE_MAIN )
		screen.showScreen(PopupStates.POPUPSTATE_IMMEDIATE, False)

		szTableName = "GameFontTable"

		nTableWidth = 56 * 4 + 500 + 20
		nTableHeight = nScreenHeight
		iButtonSize = 20

		screen.addTableControlGFC(szTableName, 5, ((nScreenWidth - nTableWidth) / 2) + 10, 0, nTableWidth, nTableHeight, True, False, iButtonSize, iButtonSize, TableStyles.TABLE_STYLE_STANDARD )
		screen.setTableColumnHeader( szTableName, 0, "<font=2>ID</font>", 56)
		screen.setTableColumnHeader( szTableName, 1, "<font=2>Small</font>", 56)
		screen.setTableColumnHeader( szTableName, 2, "<font=2>Big</font>", 56)
		screen.setTableColumnHeader( szTableName, 3, "<font=2>Button</font>", 56)
		screen.setTableColumnHeader( szTableName, 4, "<font=2>Type</font>", 500)

		iMax = FontSymbols.MAX_NUM_SYMBOLS + SASVisuals.Font.ordinal(FontSymbols.HAPPY_CHAR) - 8193 + 49 # advc: was 8483+10
		for iLine in range(iMax):
			# <advc> Include letters. Their ID go up to the mid 250s; far lower than the icon IDs.
			iID = iLine
			if iID > 280:
				iID += 8193 # was +8483 </advc>
			screen.appendTableRow(szTableName)
			screen.setTableInt(szTableName, 0, iLine, "<font=2>" + unicode(iID) + "<font/>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
			screen.setTableInt(szTableName, 1, iLine, "<font=2>" + (u" %c" %  (iID)) + "<font/>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
			screen.setTableInt(szTableName, 2, iLine, "<font=4>" + (u" %c" %  (iID)) + "<font/>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

			infoPointer = self.getInfoPointerFromGameFont(iID)

			if infoPointer is not None:
				if isinstance(infoPointer, str):
					screen.setTableInt(szTableName, 4, iLine, "<font=2>" + infoPointer + "<font/>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
				else:
					szButton = infoPointer.getButton()
					if szButton != "" and szButton != SASVisuals.StaticArt.BOMB_SHELTERS_BUTTON:
						screen.setTableRowHeight(szTableName, iLine, iButtonSize)
						screen.setTableText(szTableName, 3, iLine, "", szButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					screen.setTableInt(szTableName, 4, iLine, "<font=2>" + infoPointer.getType() + "<font/>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

	def getInfoPointerFromGameFont(self, iIndex):
		try:
			iLoopIndex = SASVisuals.Religion.ordinals().index(iIndex)
			return gc.getReligionInfo(iLoopIndex)
		except ValueError:
			pass
		try:
			iLoopIndex = SASVisuals.Corporation.ordinals().index(iIndex)
			return gc.getCorporationInfo(iLoopIndex)
		except ValueError:
			pass
		try:
			iLoopIndex = SASVisuals.Bonus.ordinals().index(iIndex)
			return gc.getBonusInfo(iLoopIndex)
		except ValueError:
			pass
		# advc.002f: BULL symbols (CITIZEN, GREAT_GENERAL, AIRPORT) added
		try:
			# <!-- custom: Font exposes its cached glyph tuple, not the indexed XML collections' ordinals() method; search that existing tuple using the requested character. (GPT-6.1-Sol) -->
			iSymbol = SASVisuals.Font.chars().index(unichr(iIndex))
			return SASVisuals.BUILTIN_FONT_SYMBOL_NAMES[iSymbol]
		except ValueError:
			pass

		return None

	def handleInput (self, inputClass):
		return 0

	def update(self, fDelta):
		return 1
