from CvPythonExtensions import *
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

		iMax = FontSymbols.MAX_NUM_SYMBOLS + CyGame().getSymbolID(FontSymbols.HAPPY_CHAR) - 8193 + 49 # advc: was 8483+10
		for iLine in range(iMax):
			# <advc> Include letters. Their ID go up to the mid 250s; far lower than the icon IDs.
			iID = iLine
			if iID > 280:
				iID += 8193 # was +8483 </advc>
			screen.appendTableRow(szTableName)
			screen.setTableInt(szTableName, 0, iLine , "<font=2>" + unicode(iID) + "<font/>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
			screen.setTableInt(szTableName, 1, iLine , "<font=2>" + (u" %c" %  (iID)) + "<font/>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
			screen.setTableInt(szTableName, 2, iLine , "<font=4>" + (u" %c" %  (iID)) + "<font/>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

			infoPointer = self.getInfoPointerFromGameFont(iID)

			if infoPointer != None:
				if isinstance(infoPointer, str):
					screen.setTableInt(szTableName, 4, iLine , "<font=2>" + infoPointer + "<font/>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
				else:
					szButton = infoPointer.getButton()
					if szButton != "" and szButton != "Art/Interface/Buttons/Buildings/BombShelters.dds":
						screen.setTableRowHeight(szTableName, iLine, iButtonSize)
						screen.setTableText(szTableName, 3, iLine, "", szButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY);
					screen.setTableInt(szTableName, 4, iLine , "<font=2>" + infoPointer.getType() + "<font/>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )



	def getInfoPointerFromGameFont(self, iIndex):
		for iLoopIndex in range(gc.getNumReligionInfos()):
			infoPointer = gc.getReligionInfo(iLoopIndex)
			if infoPointer.getChar() == iIndex:
				return infoPointer

		for iLoopIndex in range(gc.getNumCorporationInfos()):
			infoPointer = gc.getCorporationInfo(iLoopIndex)
			if infoPointer.getChar() == iIndex:
				return infoPointer

		for iLoopIndex in range(gc.getNumBonusInfos()):
			infoPointer = gc.getBonusInfo(iLoopIndex)
			if infoPointer.getChar() == iIndex:
				return infoPointer
		# advc.002f: BULL symbols (CITIZEN, GREAT_GENERAL, AIRPORT) added
		if iIndex >= CyGame().getSymbolID(FontSymbols.HAPPY_CHAR) and iIndex <= (CyGame().getSymbolID(FontSymbols.AIRPORT_CHAR)):

				list = [ "HAPPY_CHAR",
				"UNHAPPY_CHAR",
				"HEALTHY_CHAR",
				"UNHEALTHY_CHAR",
				"BULLET_CHAR",
				"STRENGTH_CHAR",
				"MOVES_CHAR",
				"RELIGION_CHAR",
				"STAR_CHAR",
				"SILVER_STAR_CHAR",
				"TRADE_CHAR",
				"DEFENSE_CHAR",
				"GREAT_PEOPLE_CHAR",
				"BAD_GOLD_CHAR",
				"BAD_FOOD_CHAR",
				"EATEN_FOOD_CHAR",
				"GOLDEN_AGE_CHAR",
				"ANGRY_POP_CHAR",
				"OPEN_BORDERS_CHAR",
				"DEFENSIVE_PACT_CHAR",
				"MAP_CHAR",
				"OCCUPATION_CHAR",
				"POWER_CHAR",
				"CITIZEN_CHAR",
				"GREAT_GENERAL_CHAR",
				"AIRPORT_CHAR"
				]

				return list[iIndex - CyGame().getSymbolID(FontSymbols.HAPPY_CHAR)]

		return None

	def handleInput (self, inputClass):
		return 0

	def update(self, fDelta):
		return 1
