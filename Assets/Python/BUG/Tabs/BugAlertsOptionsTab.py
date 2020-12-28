## BugAlertsOptionsTab
##
## Tab for the BUG Civ4lerts and Reminders Options.
##
## Copyright (c) 2007-2008 The BUG Mod.
##
## Author: EmperorFool

import BugOptionsTab

class BugAlertsOptionsTab(BugOptionsTab.BugOptionsTab):
	"BUG NJAGC Options Screen Tab"
	
	def __init__(self, screen):
		BugOptionsTab.BugOptionsTab.__init__(self, "Alerts", "Alerts")

	def create(self, screen):
		tab = self.createTab(screen)
		panel = self.createMainPanel(screen)
		column = self.addOneColumnLayout(screen, panel)
		
		# Civ4lerts
		self.addCheckbox(screen, column, "Civ4lerts__Enabled")
		left, center, right = self.addThreeColumnLayout(screen, column, "Civ4lerts", True)
		
		# City management
		self.addLabel(screen, left, "Alerts_City", "Cities:")
		leftL, leftR = self.addTwoColumnLayout(screen, left, "Civ4lerts_TableGrowth")
		self.addCheckbox(screen, leftL, "Civ4lerts__CityPendingGrowth")
		self.addCheckbox(screen, leftR, "Civ4lerts__CityGrowth")		
		self.addCheckbox(screen, leftL, "Civ4lerts__CityPendingHealthiness")
		self.addCheckbox(screen, leftR, "Civ4lerts__CityHealthiness")		
		self.addCheckbox(screen, leftL, "Civ4lerts__CityPendingHappiness")
		self.addCheckbox(screen, leftR, "Civ4lerts__CityHappiness")
		# advc.106d:
		self.addCheckbox(screen, left, "Civ4lerts__CityPendingPositive")
		# <advc.210b> Replacing these two left/right boxes with a single one
		# for the revolt alert.
		#self.addCheckbox(screen, leftL, "Civ4lerts__CityPendingOccupation")
		#self.addCheckbox(screen, leftR, "Civ4lerts__CityOccupation")
		self.addCheckbox(screen, left, "Civ4lerts__Revolt")	# </advc.210b>
		# advc.210: Disabled
		#self.addCheckbox(screen, left, "MoreCiv4lerts__CityPendingBorderExpansion")
		self.addCheckbox(screen, left, "Civ4lerts__CityCanHurryPop")
		self.addCheckbox(screen, left, "Civ4lerts__CityCanHurryGold")
		# advc.210c: Disabled
		#self.addCheckbox(screen, left, "MoreCiv4lerts__CityFounded")
		
		# Diplomacy
		self.addLabel(screen, center, "Alerts_Diplomacy", "Diplomacy:")
		self.addCheckbox(screen, center, "Civ4lerts__RefusesToTalk")
		self.addCheckbox(screen, center, "Civ4lerts__WorstEnemy")
		self.addCheckbox(screen, center, "MoreCiv4lerts__OpenBordersTrade")
		self.addCheckbox(screen, center, "MoreCiv4lerts__DefensivePactTrade")
		self.addCheckbox(screen, center, "MoreCiv4lerts__PermanentAllianceTrade")
		self.addCheckbox(screen, center, "MoreCiv4lerts__VassalTrade")
		# advc.210: Disabled; use the RTT alert instead.
		#self.addCheckbox(screen, center, "MoreCiv4lerts__PeaceTrade")
		self.addCheckbox(screen, center, "MoreCiv4lerts__SurrenderTrade")
		
		# Trades
		self.addLabel(screen, right, "Alerts_Trade", "Trading:")
		self.addCheckbox(screen, right, "MoreCiv4lerts__TechTrade")
		# advc.ctr:
		self.addCheckbox(screen, right, "Civ4lerts__CityTrade")
		self.addCheckbox(screen, right, "MoreCiv4lerts__MapTrade")
		# advc.210a:
		self.addCheckbox(screen, right, "Civ4lerts__WarTrade")
	
		rightL, rightR = self.addTwoColumnLayout(screen, right, "Alerts_Trade_Column")
		# <advc.210d> Moved down b/c now part of the two-column layout
		self.addCheckbox(screen, rightL, "MoreCiv4lerts__BonusTrade")
		self.addCheckbox(screen, rightR, "Civ4lerts__BonusThirdParties")
		# </advc.210d>
		self.addCheckboxIntDropdown(screen, rightL, rightR, "Civ4lerts__GoldTrade", "Civ4lerts__GoldTradeThresh", "LAYOUT_LEFT")
		self.addCheckboxIntDropdown(screen, rightL, rightR, "Civ4lerts__GoldPerTurnTrade", "Civ4lerts__GoldPerTurnTradeThresh", "LAYOUT_LEFT")
		
		# advc.210a: Removing these alerts
		# Victories
		#self.addLabel(screen, right, "Alerts_Victory", "Victory:")
		#rightL, rightR = self.addTwoColumnLayout(screen, right, "Alerts_Victory_Column")
		#self.addCheckboxFloatDropdown(screen, rightL, rightR, "MoreCiv4lerts__DomPop", "MoreCiv4lerts__DomPopThresh", "LAYOUT_LEFT")
		#self.addCheckboxFloatDropdown(screen, rightL, rightR, "MoreCiv4lerts__DomLand", "MoreCiv4lerts__DomLandThresh", "LAYOUT_LEFT")
		
		screen.attachHSeparator(column, column + "Sep")
		
		# Reminders
		left, right = self.addTwoColumnLayout(screen, column, "Main")
		# advc.071: And put Reminder options in lLeft
		lLeft, lRight = self.addTwoColumnLayout(screen, left, "Bottom", True)
		# advc.210: Put the "enabled" checkbox and the display method dropdown on the same line
		self.addCheckboxTextDropdown(screen, lLeft, lLeft, "Reminder__Enabled", "Reminder__DisplayMethod")
		self.addCheckbox(screen, lLeft, "Reminder__Autolog")
		# <advc.071>
		self.addLabel(screen, lRight, "OnFirstContact", "On First Contact:") 
		self.addTextDropdown(screen, lRight, lRight, "Civ4lerts__OnFirstContact")
		# (Leave the 'right' column empty)
		# </advc.071>
