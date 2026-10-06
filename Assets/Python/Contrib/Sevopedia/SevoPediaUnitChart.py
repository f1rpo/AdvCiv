# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: originally derived from RFC Dawn of Civilization's CvPediaUnitChart.py; subsequently adjusted and extended for AdvCiv-SAS. (ChatGPT-5.6-Sol) -->
#
# parts of this code have also been imported from base advciv:
# https://github.com/f1rpo/AdvCiv/blob/master/Assets/Python/Contrib/Sevopedia/SevoPediaUnitChart.py
# and modified or not for AdvCiv-SAS -->

from CvPythonExtensions import *
import CvUtil
from SASFontUtils import getSASUIFontLabel
from SASUtils import getInfoTypeOrFail

from _sevopedia_helpers import *

gc = CyGlobalContext()
localText = CyTranslator()

class SevoPediaUnitChart:
	def __init__(self, main):
		self.iGroup = -1
		self.top = main
		self.I_UNITCOMBAT_AIR_BOMBER = getInfoTypeOrFail("UNITCOMBAT_AIR_BOMBER")
		self.I_UNITCOMBAT_AIR_FIGHTER = getInfoTypeOrFail("UNITCOMBAT_AIR_FIGHTER")

		self.X_TABLE = self.top.X_PEDIA_PAGE
		self.Y_TABLE = self.top.Y_PEDIA_PAGE
		self.H_TABLE = self.top.H_PEDIA_PAGE

		self.MARGIN = 20
		self.N_COLUMNS = 0
		self.W_NAME = 270
		# <!-- custom: 129 is enough, but add a bit more margin to avoid truncation if digits are wider. (GPT-5.2-Codex (summarized)) -->
		self.W_NUM = 130

		self.W_TABLE = ((self.N_COLUMNS - 2 - 1) * self.W_NUM) + (2 * self.MARGIN)

	def interfaceScreen(self, iGroup):
		self.iGroup = iGroup

		self.placeUnitTable()

	# <!-- custom: i did not know about this ChatGPT told me about this or made me understand it and solve it, so adding this explanation in case it helps others or me:
	# in python, here for placeUnitTable function, when we call it using self.placeUnitTable(), self is passed automatically as an argument so no need to write it (else there would be 2 arguments) at function.
	# However, in function definition, self is not known, so it needs to be defined as a parameter. This is why there is an apparent mismatch in the number of parameters vs arguments in placeUnitTable(self) vs self.placeUnitTable(), but this is how it should be done else it does not work.
	# Thanks to ChatGPT for the guidance; adding this in case it helps others. (GPT-5.2-Codex (summarized)) -->
	def placeUnitTable(self):
		screen = self.top.getScreen()
		table = self.top.getNextWidgetName()

		isAirCombatType = (self.iGroup == self.I_UNITCOMBAT_AIR_BOMBER or self.iGroup == self.I_UNITCOMBAT_AIR_FIGHTER)

		if not isAirCombatType:
			self.N_COLUMNS = 8
		else:
			self.N_COLUMNS = 10

		# <!-- custom: simple per-column width tuning (no extra structures):
		# tighter numeric columns free room for collateral text; air evasion/intercept stay equal. (GPT-5.3-Codex) -->
		wTight = self.W_NUM - 7
		wDefault = self.W_NUM
		wWide = self.W_NUM + 28

		if not isAirCombatType:
			self.W_TABLE = (self.W_NAME + wTight + wTight + wDefault + wTight + wWide + wTight + wDefault) + (2 * self.MARGIN)
		else:
			self.W_TABLE = (self.W_NAME + wTight + wTight + wDefault + wTight + wWide + wDefault + wDefault + wDefault + wDefault) + (2 * self.MARGIN)

		# <!-- custom: blue is more readable than standard i find, imported from base AdvCiv and modified with a similar kind of purpose -->
		#screen.addTableControlGFC(table, self.N_COLUMNS, self.X_TABLE, self.Y_TABLE, self.W_TABLE, self.H_TABLE, True, False, 24, 24, TableStyles.TABLE_STYLE_STANDARD)
		#screen.setStyle(table, "Table_StandardCiv_Style")
		#screen.enableSort(table)
		screen.addPanel(self.top.getNextWidgetName(), "", "", True, True, self.X_TABLE, self.Y_TABLE, self.W_TABLE, self.H_TABLE, PanelStyles.PANEL_STYLE_BLUE50)
		screen.addPanel(self.top.getNextWidgetName(), "", "", True, True, self.X_TABLE + self.MARGIN, self.Y_TABLE + self.MARGIN, self.W_TABLE - (self.MARGIN * 2), self.H_TABLE - (self.MARGIN * 2), PanelStyles.PANEL_STYLE_BLUE50)

		table = self.top.getNextWidgetName()
		tableX = self.X_TABLE + self.MARGIN
		tableY = self.Y_TABLE + self.MARGIN + 5
		tableW = self.W_TABLE - (self.MARGIN * 2)
		tableH = self.H_TABLE - (self.MARGIN * 2) - 5
		screen.addTableControlGFC(table, self.N_COLUMNS, tableX, tableY, tableW, tableH, True, False, 32, 32, CHART_TABLE_STYLE)
		screen.enableSort(table)

		szName = chart_get_info_display_name(gc.getUnitCombatInfo(self.iGroup))
		szStrength = u"%c" % CyGame().getSymbolID(FontSymbols.STRENGTH_CHAR)

		# <!-- custom: display the icon instead of text -->
		#szMove = "Moves"
		szMove = u"%c" % CyGame().getSymbolID(FontSymbols.MOVES_CHAR)

		# <!-- custom: display more potentially useful information, useful in AdvCiv-SAS or maybe other mods as well, also so that i don't have to maintain it everytime i make a micro (or big) change, more versatile and flexible this way too. -->
		#if self.iGroup == getInfoTypeOrFail("UNITCOMBAT_AIR") or self.iGroup == getInfoTypeOrFail("UNITCOMBAT_NAVAL") or self.iGroup == getInfoTypeOrFail("UNITCOMBAT_SIEGE"):
			# <!-- custom: imported and modified from base advciv in a similar way -->
			#szSpecial = "Bombard"
		#else:
		#	szSpecial = "1st Strike"
		# <!-- custom: Chance first strikes now make this a minimum-to-maximum range rather than a guaranteed count alone. See KI#323. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		szFirstStrike = u"1st Strike Range"
		szBombard = u"%c" % CyGame().getSymbolID(FontSymbols.DEFENSE_CHAR)
		szCollateral = u"Collateral"

		szCost = u"%c" % gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar()

		# <!-- custom: trick (to center the headers text too) taught to me by ChatGPT, quoting it: "screen.setTableColumnHeader(...) don't use FONT_*_JUSTIFY like the cell contents, but there's a workaround." + "Workaround: Pad the header label string manually" thanks, that i adjusted too or not for advciv-sas. -->
		# <!-- custom: table headers do not honor text-justify in this widget.
		# Keep per-header legacy spacing (worked well at font 2), then reduce spaces gradually
		# when label font > 2 to avoid right-shift at larger scales. (GPT-5.3-Codex) -->
		iLabelFont = getSASUIFontLabel()
		iShrink = 0
		if iLabelFont > 2:
			iShrink = (iLabelFont - 2) * 3

		def _headerPad(iBaseSpaces):
			iSpaces = iBaseSpaces - iShrink
			if iSpaces < 0:
				iSpaces = 0
			return u" " * iSpaces

		szNameText = chart_font2(_headerPad(4) + szName)
		# <!-- custom: keep these base pads tuned for the small-font look; high-font uses iShrink above.
		# Avoid extra micro-retuning here to reduce tedium. (GPT-5.3-Codex) -->
		szStrengthtext = chart_font2(_headerPad(15) + szStrength)
		szMoveText = chart_font2(_headerPad(15) + szMove)
		szFirstStrikeText = chart_font2(_headerPad(2) + szFirstStrike)
		szBombardText = chart_font2(_headerPad(14) + szBombard)
		szCollateralText = chart_font2(_headerPad(12) + szCollateral)
		szCostText = chart_font2(_headerPad(15) + szCost)

		if isAirCombatType:
			szAirEvasion = u"Air Evasion"
			szAirIntercept = u"Air Intercept"
			szAirRange = u"Air Range"

			szAirEvasionText = chart_font2(_headerPad(5) + szAirEvasion)
			szAirInterceptText = chart_font2(_headerPad(4) + szAirIntercept)
			szAirRangeText = chart_font2(_headerPad(6) + szAirRange)

			screen.setTableColumnHeader(table, 0, szNameText, self.W_NAME)
			screen.setTableColumnHeader(table, 1, szStrengthtext, wTight)
			screen.setTableColumnHeader(table, 2, szMoveText, wTight)
			screen.setTableColumnHeader(table, 3, szFirstStrikeText, wDefault)
			screen.setTableColumnHeader(table, 4, szBombardText, wTight)
			screen.setTableColumnHeader(table, 5, szCollateralText, wWide)
			screen.setTableColumnHeader(table, 6, szAirEvasionText, wDefault)
			screen.setTableColumnHeader(table, 7, szAirInterceptText, wDefault)
			screen.setTableColumnHeader(table, 8, szAirRangeText, wDefault)
			screen.setTableColumnHeader(table, 9, szCostText, wDefault)

			for iUnit in xrange(gc.getNumUnitInfos()):
				UnitInfo = gc.getUnitInfo(iUnit)
				if self.iGroup == UnitInfo.getUnitCombatType():
					iRow = screen.appendTableRow(table)

					self.placeTableName(screen, table, 0, iRow, UnitInfo, iUnit)
					self.placeTableCombat(screen, table, 1, iRow, UnitInfo)
					self.placeTableMovement(screen, table, 2, iRow, UnitInfo)
					self.placeTableFirstStrike(screen, table, 3, iRow, UnitInfo)
					self.placeTableBombard(screen, table, 4, iRow, UnitInfo)
					self.placeTableCollateral(screen, table, 5, iRow, UnitInfo)
					self.placeTableAirEvasion(screen, table, 6, iRow, UnitInfo)
					self.placeTableAirInterception(screen, table, 7, iRow, UnitInfo)
					self.placeTableAirRange(screen, table, 8, iRow, UnitInfo)
					self.placeTableCost(screen, table, 9, iRow, UnitInfo)

		else:
			szWithdraw = u"Withdraw"

			szWithdrawText = chart_font2(_headerPad(6) + szWithdraw)

			screen.setTableColumnHeader(table, 0, szNameText, self.W_NAME)
			screen.setTableColumnHeader(table, 1, szStrengthtext, wTight)
			screen.setTableColumnHeader(table, 2, szMoveText, wTight)
			screen.setTableColumnHeader(table, 3, szFirstStrikeText, wDefault)
			screen.setTableColumnHeader(table, 4, szBombardText, wTight)
			screen.setTableColumnHeader(table, 5, szCollateralText, wWide)
			screen.setTableColumnHeader(table, 6, szWithdrawText, wTight)
			screen.setTableColumnHeader(table, 7, szCostText, wDefault)

			for iUnit in xrange(gc.getNumUnitInfos()):
				UnitInfo = gc.getUnitInfo(iUnit)
				if self.iGroup == UnitInfo.getUnitCombatType():
					iRow = screen.appendTableRow(table)

					self.placeTableName(screen, table, 0, iRow, UnitInfo, iUnit)
					self.placeTableCombat(screen, table, 1, iRow, UnitInfo)
					self.placeTableMovement(screen, table, 2, iRow, UnitInfo)
					self.placeTableFirstStrike(screen, table, 3, iRow, UnitInfo)
					self.placeTableBombard(screen, table, 4, iRow, UnitInfo)
					self.placeTableCollateral(screen, table, 5, iRow, UnitInfo)
					self.placeTableWithdraw(screen, table, 6, iRow, UnitInfo)
					self.placeTableCost(screen, table, 7, iRow, UnitInfo)

	def placeTableName(self, screen, table, iCol, iRow, UnitInfo, iUnit):
		# Name
		screen.setTableText(table, iCol, iRow, chart_font2(chart_get_info_display_name(UnitInfo)), UnitInfo.getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeTableCombat(self, screen, table, iCol, iRow, UnitInfo):
		# Combat Strength
		if UnitInfo.getAirCombat() > 0:
			szCombatNum = u"%d" % UnitInfo.getAirCombat()
		else:
			szCombatNum = u"%d" % UnitInfo.getCombat()

		screen.setTableInt(table, iCol, iRow, chart_font2(szCombatNum), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def placeTableMovement(self, screen, table, iCol, iRow, UnitInfo):
		# Movement
		szMovesNum = u"%d" % UnitInfo.getMoves()

		screen.setTableInt(table, iCol, iRow, chart_font2(szMovesNum), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def placeTableFirstStrike(self, screen, table, iCol, iRow, UnitInfo):
		# First Strikes
		# <!-- custom: The RFC/DoC chart implementation adopted by AdvCiv-SAS did not include chance first strikes, leaving chance-only units blank and understating mixed units.
		# Match DLL help: show guaranteed strikes alone, or guaranteed-to-maximum when chance strikes exist. See KI#323. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		iFirstStrikes = UnitInfo.getFirstStrikes()
		iChanceFirstStrikes = UnitInfo.getChanceFirstStrikes()
		if iChanceFirstStrikes > 0:
			szFirstStrikesNum = u"%d-%d" % (iFirstStrikes, iFirstStrikes + iChanceFirstStrikes)
		elif iFirstStrikes > 0:
			szFirstStrikesNum = u"%d" % iFirstStrikes
		# <!-- custom: keep this beautification; it's more readable than many 0s. (GPT-5.2-Codex (summarized)) -->
		else:
			szFirstStrikesNum = u""

		screen.setTableInt(table, iCol, iRow, chart_font2(szFirstStrikesNum), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def placeTableBombard(self, screen, table, iCol, iRow, UnitInfo):
		# Bombard
		if UnitInfo.getBombardRate() > 0:
			szBombardRate = u"%d%%" % UnitInfo.getBombardRate()
		elif UnitInfo.getBombRate() > 0:
			szBombardRate = u"%d%%" % UnitInfo.getBombRate()
		else:
			szBombardRate = u""

		screen.setTableInt(table, iCol, iRow, chart_font2(szBombardRate), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def placeTableCollateral(self, screen, table, iCol, iRow, UnitInfo):
		# Collateral
		# <!-- custom: Keep zero-base-damage military rows when their limit is nonzero: SAS uses them to expose collateral capacity enabled by promotions.
		# KI#324.2 repairs the inherited promotion-validity guard that prevented this displayed capability from being acquired. See KI#324.2. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if UnitInfo.getCollateralDamage() > 0 or UnitInfo.getCollateralDamageLimit():
			szCollateralRate = u"%d%%/%d%% (%d)" % (UnitInfo.getCollateralDamage(), UnitInfo.getCollateralDamageLimit(), UnitInfo.getCollateralDamageMaxUnits())
		else:
			szCollateralRate = u""

		screen.setTableInt(table, iCol, iRow, chart_font2(szCollateralRate), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def placeTableWithdraw(self, screen, table, iCol, iRow, UnitInfo):
		# Withdrawal
		if UnitInfo.getWithdrawalProbability() > 0:
			szWithdrawalRate = u"%d%%" % UnitInfo.getWithdrawalProbability()
		else:
			szWithdrawalRate = u""

		screen.setTableInt(table, iCol, iRow, chart_font2(szWithdrawalRate), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def placeTableAirEvasion(self, screen, table, iCol, iRow, UnitInfo):
		# Air Evasion
		if UnitInfo.getEvasionProbability() > 0:
			szAirEvasionRate = u"%d%%" % UnitInfo.getEvasionProbability()
		else:
			szAirEvasionRate = u""

		screen.setTableInt(table, iCol, iRow, chart_font2(szAirEvasionRate), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def placeTableAirInterception(self, screen, table, iCol, iRow, UnitInfo):
		# Air Interception
		if UnitInfo.getInterceptionProbability() > 0:
			szAirInterceptionRate = u"%d%%" % (UnitInfo.getInterceptionProbability())
		else:
			szAirInterceptionRate = u""

		screen.setTableInt(table, iCol, iRow, chart_font2(szAirInterceptionRate), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def placeTableAirRange(self, screen, table, iCol, iRow, UnitInfo):
		# Air Range
		if UnitInfo.getAirRange() > 0:
			szAirRangeNum = u"%d" % UnitInfo.getAirRange()
		else:
			szAirRangeNum = u""

		screen.setTableInt(table, iCol, iRow, chart_font2(szAirRangeNum), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def placeTableCost(self, screen, table, iCol, iRow, UnitInfo):
		# Cost
		if UnitInfo.getProductionCost() < 0:
			szCostNum = localText.getText("TXT_KEY_NON_APPLICABLE", ())
		else:
			szCostNum = u"%d" % UnitInfo.getProductionCost()

		screen.setTableInt(table, iCol, iRow, chart_font2(szCostNum), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

	def handleInput (self, inputClass):
		return 0
