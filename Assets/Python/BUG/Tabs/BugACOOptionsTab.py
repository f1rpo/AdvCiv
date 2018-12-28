## BugGeneralOptionsTab
##
## Tab for the BUG Advanced Combat Odds Options.
##
## Copyright (c) 2007-2008 The BUG Mod.
##
## Author: EmperorFool

import BugOptionsTab

class BugACOOptionsTab(BugOptionsTab.BugOptionsTab):
	"BUG ACO Options Screen Tab"
	
	def __init__(self, screen):
		BugOptionsTab.BugOptionsTab.__init__(self, "ACO", "Advanced Combat Odds")

	def create(self, screen):
		tab = self.createTab(screen)
		panel = self.createMainPanel(screen)
		column = self.addOneColumnLayout(screen, panel)
		# advc.048: No separator (last param)
		left, right = self.addTwoColumnLayout(screen, column, "Page", False)
		
		#self.addLabel(screen, left, "ACO", "Advanced Combat Odds:")
		self.addCheckbox(screen, left, "ACO__Enabled")
		# <advc.048> Put all the standard/alternate stuff at the top to make sure players get it before looking into the other options
		self.addCheckbox(screen, left, "ACO__SwapViews")
		#self.addSpacer(screen, left, "ACO_Tab0")
		#self.addSpacer(screen, left, "ACO_Tab1")
		leftL, leftR =  self.addTwoColumnLayout(screen, left, "ACO1")
		self.addTextDropdown(screen, leftL, leftR, "ACO__ShowShiftInstructions")
		# </advc.048>
		
		self.addSpacer(screen, leftL, "ACO_Tab1.1")
		self.addSpacer(screen, leftR, "ACO_Tab1.2")
		
		self.addTextDropdown(screen, leftL, leftR, "ACO__ShowBasicInfo")
		self.addTextDropdown(screen, leftL, leftR, "ACO__ShowAttackerInfo")
		self.addTextDropdown(screen, leftL, leftR, "ACO__ShowDefenderInfo")
		
		self.addSpacer(screen, leftL, "ACO_Tab2.1")
		self.addSpacer(screen, leftR, "ACO_Tab2.2")
		
		# advc.048: Moved to the left column: Odds, health bars and avg. health
		self.addTextDropdown(screen, leftL, leftR, "ACO__ShowSurvivalOdds")
		self.addTextDropdown(screen, leftL, leftR, "ACO__ShowUnharmedOdds")
		# Don't have quite that much space, and it's all closely related anyway.
		#self.addSpacer(screen, leftL, "ACO_Tab3.1")
		#self.addSpacer(screen, leftR, "ACO_Tab3.2")
		self.addTextDropdown(screen, leftL, leftR, "ACO__ShowAttackerHealthBars")
		self.addTextDropdown(screen, leftL, leftR, "ACO__ShowDefenderHealthBars")
		self.addTextDropdown(screen, leftL, leftR, "ACO__ShowAverageHealth")
		
		self.addSpacer(screen, right, "ACO_Tab2")
		self.addSpacer(screen, right, "ACO_Tab3") # advc.048: another spacer
		rightL, rightR =  self.addTwoColumnLayout(screen, right, "ACO2")
		
		self.addTextDropdown(screen, rightL, rightR, "ACO__ShowUnroundedExperience")
		self.addTextDropdown(screen, rightL, rightR, "ACO__ShowExperienceRange")
		
		self.addSpacer(screen, rightL, "ACO_Tab4.1")
		self.addSpacer(screen, rightR, "ACO_Tab4.2")
		self.addTextDropdown(screen, rightL, rightR, "ACO__ShowDefenseModifiers")
		self.addTextDropdown(screen, rightL, rightR, "ACO__ShowTotalDefenseModifier")
		
		self.addSpacer(screen, rightL, "ACO_Tab4.1")
		self.addSpacer(screen, rightR, "ACO_Tab4.2")
		
		# advc.048: Checkboxes that always apply moved to the end, ForceOriginalOdds to the very end. Put a heading in front:
		self.addLabel(screen, rightL, "ACO_Always", "Always ...")
		self.addCheckbox(screen, rightL, "ACO__IgnoreBarbFreeWins")
		self.addCheckbox(screen, rightL, "ACO__MergeShortBars")
		self.addCheckbox(screen, rightL, "ACO__ShowModifierLabels")
		self.addCheckbox(screen, rightL, "ACO__ForceOriginalOdds")
