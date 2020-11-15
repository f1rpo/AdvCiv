## BugAdvisorOptionsTab
##
## Tab for the BUG Advisor Options.
##
## Copyright (c) 2007-2008 The BUG Mod.
##
## Author: EmperorFool

import BugOptionsTab

class BugAdvisorOptionsTab(BugOptionsTab.BugOptionsTab):
	"BUG General Options Screen Tab"

	def __init__(self, screen):
		BugOptionsTab.BugOptionsTab.__init__(self, "Advisors", "Advisors")

	def create(self, screen):
		tab = self.createTab(screen)
		panel = self.createMainPanel(screen)
		left, center, right = self.addThreeColumnLayout(screen, panel, panel, True)

		self.addLabel(screen, left, "Domestic_Advisor", "Domestic [F1]:")
		self.addCheckbox(screen, left, "CustDomAdv__Enabled")
		leftL, leftR = self.addTwoColumnLayout(screen, left, "Advisors__CustDomAdv")
		self.addTextDropdown(screen, leftL, leftR, "CustDomAdv__SpaceTop", True)
		self.addTextDropdown(screen, leftL, leftR, "CustDomAdv__SpaceSides", True)
		self.addTextDropdown(screen, leftL, leftR, "CustDomAdv__ProductionGrouping", True)
		#self.addCheckbox(screen, left, "MiscHover__CDAZoomCityDetails")
		#self.addLabel(screen, left, "Finance_Advisor", "Finance [F2]:")
		#self.addCheckbox(screen, left, "Advisors__BugFinanceAdvisor")
		
		# advc.004: Can afford some space
		self.addSpacer(screen, left, "Before_Foreign_Advisor")

		self.addLabel(screen, left, "Foreign_Advisor", "Foreign [F4]:")
		
		#self.addCheckbox(screen, left, "Advisors__EFADealTurnsLeft")
		# <advc.072> Replacing the above
		leftL, leftR = self.addTwoColumnLayout(screen, left, "Advisors__ForeignAdv")
		self.addTextDropdown(screen, leftL, leftR, "Advisors__DealTurnsLeft", True)
		# </advc.072>
		self.addCheckbox(screen, leftL, "Advisors__EFAImprovedInfo")
		#self.addCheckbox(screen, leftL, "MiscHover__TechTradeDenial")
		#self.addCheckbox(screen, leftL, "MiscHover__BonusTradeDenial")

		# advc.152: Moved down to allow the War Trades option to be placed between the Glance Tab option and the Military Advisor options
		#comboBox = "Advisors_ComboBoxEFA"
		#screen.attachHBox(left, comboBox)
		#self.addCheckbox(screen, leftL, "Advisors__EFAGlanceTab")
		#self.addTextDropdown(screen, leftL, leftR, "Advisors__EFAGlanceAttitudes")
		# advc.072: Align through leftL,leftR instead of ComboBox
		self.addCheckboxTextDropdown(screen, leftL, leftR, "Advisors__EFAGlanceTab", "Advisors__EFAGlanceAttitudes")
		# advc.152:
		self.addCheckbox(screen, leftL, "Advisors__EFAWarTrades")
		# advc.ctr:
		self.addCheckbox(screen, leftL, "Advisors__EFACityTrades")
		
		# <advc.004> Moved to center column
		self.addLabel(screen, center, "Military_Advisor", "Military [F5]:")
		self.addCheckbox(screen, center, "Advisors__BugMA")
		self.addSpacer(screen, center, "Before_Technology_Advisor")
		# </advc.004>

		self.addLabel(screen, center, "Technology_Advisor", "Technology [F6]:")
		self.addCheckbox(screen, center, "Advisors__GPTechPrefs")
		#self.addCheckbox(screen, center, "MiscHover__SpedUpTechs")
		# advc.004: No longer optional
		#self.addCheckbox(screen, center, "Advisors__WideTechScreen")
		self.addCheckbox(screen, center, "Advisors__ShowTechEra")

		self.addSpacer(screen, center, "Before_Religious_Advisor") # advc.004
		
		self.addLabel(screen, center, "Religious_Advisor", "Religion [F7]:")
		self.addCheckbox(screen, center, "Advisors__BugReligiousTab")
		self.addTextDropdown(screen, center, center, "Advisors__ShowReligions", True)

		#self.addLabel(screen, center, "Victory_Conditions", "Victory [F8]:")
		#self.addCheckbox(screen, center, "Advisors__BugVictoriesTab")
		#self.addCheckbox(screen, center, "Advisors__BugMembersTab")

		# K-Mod, info stuff moved from center panel to right
		self.addLabel(screen, right, "Info_Screens", "Info [F9]:")
		# <advc.004>
		# Moved up b/c the sub-option looked strange at the end of the block
		self.addCheckbox(screen, right, "Advisors__BugInfoWonders")
		self.addCheckbox(screen, right, "Advisors__BugInfoWondersPlayerColor", True)
		# Put this one before GraphsTab to make clear that GraphsTab isn't a prereq
		self.addCheckbox(screen, right, "Advisors__BugGraphsLogScale")
		# </advc.004>
		self.addCheckbox(screen, right, "Advisors__PartialScoreGraphs") # advc.091
		self.addCheckbox(screen, right, "Advisors__BugGraphsTab")
		self.addCheckbox(screen, right, "Advisors__BugStatsTab")
		# advc.004: No longer optional
		#self.addCheckbox(screen, right, "Advisors__NonZeroStatsOnly") # K-Mod

		self.addSpacer(screen, right, "Before_Sevopedia") # advc.004

		self.addLabel(screen, right, "Sevopedia", "Sevopedia [F12]:")
		self.addCheckbox(screen, right, "Advisors__Sevopedia")
		self.addCheckbox(screen, right, "Advisors__SevopediaSortItemList")

		#self.addLabel(screen, right, "Espionage_Screen", "Espionage [CTRL + E]:")
		#self.addCheckbox(screen, right, "BetterEspionage__Enabled")
		#self.addCheckbox(screen, right, "BetterEspionage__ShowCalculatedInformation")

		#self.addLabel(screen, right, "Espionage_Ratio", "Ratio:")
		#rightL, rightR = self.addTwoColumnLayout(screen, right, "Espionage_Screen_Column")
		#self.addColorDropdown(screen, rightL, rightR, "BetterEspionage__DefaultRatioColor", True)
		#self.addFloatDropdown(screen, rightL, rightR, "BetterEspionage__GoodRatioCutoff", True, "LAYOUT_LEFT")
		#self.addColorDropdown(screen, rightL, rightR, "BetterEspionage__GoodRatioColor", True)
		#self.addFloatDropdown(screen, rightL, rightR, "BetterEspionage__BadRatioCutoff", True, "LAYOUT_LEFT")
		#self.addColorDropdown(screen, rightL, rightR, "BetterEspionage__BadRatioColor", True)

		#self.addLabel(screen, rightL, "Espionage_Missions", "Missions:")
		#self.addSpacer(screen, rightR, "Espionage_Missions")
		#self.addColorDropdown(screen, rightL, rightR, "BetterEspionage__PossibleMissionColor", True)
		#self.addFloatDropdown(screen, rightL, rightR, "BetterEspionage__CloseMissionPercent", True, "LAYOUT_LEFT")
		#self.addColorDropdown(screen, rightL, rightR, "BetterEspionage__CloseMissionColor", True)

		self.addSpacer(screen, right, "Advisors_Tab")
