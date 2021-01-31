## BugGeneralOptionsTab
##
## Tab for the BUG General Options (Main and City Screens).
##
## Copyright (c) 2007-2008 The BUG Mod.
##
## Author: EmperorFool

import BugOptionsTab

class BugGeneralOptionsTab(BugOptionsTab.BugOptionsTab):
	"BUG General Options Screen Tab"
	
	def __init__(self, screen):
		BugOptionsTab.BugOptionsTab.__init__(self, "General", "General")

	def create(self, screen):
		tab = self.createTab(screen)
		panel = self.createMainPanel(screen)
		column = self.addOneColumnLayout(screen, panel)

		left, center, right = self.addThreeColumnLayout(screen, column, "Top", True)
		
		self.createGreatPersonGeneralPanel(screen, left)
		self.addSpacer(screen, left, "GeneralL1")
		self.createTechSplashPanel(screen, left)
		#self.createLeaderheadPanel(screen, left)

		#self.createAutoSavePanel(screen, center)
		self.createInfoPanePanel(screen, center)
		# advc: Don't have space for this spacer anymore
		#self.addSpacer(screen, center, "GeneralC1")
		self.createActionsPanel(screen, center) # advc.004: Restored
		
		self.createSlidersPanel(screen, right) # advc.120c
		self.addSpacer(screen, right, "GeneralR1")
		self.createMiscellaneousPanel(screen, right)
#		if Buffy.isEnabled():
#			self.addSpacer(screen, right, "General5")
#			self.createBuffyPanel(screen, right)

	def createGreatPersonGeneralPanel(self, screen, panel):
		self.addLabel(screen, panel, "ProgressBars", "Progress Bars:")
		# advc.004: Moved from Misc
		self.addCheckbox(screen, panel, "MainInterface__ProgressBarsTickMarks")
		self.addCheckboxTextDropdown(screen, panel, panel, "MainInterface__GPBar", "MainInterface__GPBar_Types")
		#self.addCheckbox(screen, panel, "MainInterface__GPBar")
		#self.addTextDropdown(screen, panel, panel, "MainInterface__GPBar_Types", True)
		self.addCheckbox(screen, panel, "MainInterface__Combat_Counter")
		# advc.078:
		self.addCheckbox(screen, panel, "MainInterface__OnceProgress")
	# advc.sha: Permanently disabled
	#def createLeaderheadPanel(self, screen, panel):
	#	self.addLabel(screen, panel, "Leaderheads", "Leaderheads:")
	#	self.addCheckbox(screen, panel, "MiscHover__LeaderheadHiddenAttitude")
	#	self.addCheckbox(screen, panel, "MiscHover__LeaderheadWorstEnemy")
	#	self.addCheckbox(screen, panel, "MiscHover__LeaderheadDefensivePacts")
		
	def createAutoSavePanel(self, screen, panel):
		self.addLabel(screen, panel, "AutoSave", "AutoSave:")
		self.addCheckbox(screen, panel, "AutoSave__CreateStartSave")
		self.addCheckbox(screen, panel, "AutoSave__CreateEndSave")
		self.addCheckbox(screen, panel, "AutoSave__CreateExitSave")
		self.addCheckbox(screen, panel, "AutoSave__UsePlayerName")
		
	def createActionsPanel(self, screen, panel):
		self.addLabel(screen, panel, "Actions", "Commands:")
		# advc.004l, advc.011b: These five remain disabled
		#self.addCheckbox(screen, panel, "Actions__AskDeclareWarUnits")
		#self.addCheckbox(screen, panel, "Actions__SentryHealing")
		#self.addCheckbox(screen, panel, "Actions__SentryHealingOnlyNeutral", True)
		#self.addCheckbox(screen, panel, "Actions__PreChopForests")
		#self.addCheckbox(screen, panel, "Actions__PreChopImprovements")
		# <advc.004> Put other stuff (from Misc) under Actions
		self.addTextDropdown(screen, panel, panel, "MainInterface__BuildIconSize", True)
		self.addCheckbox(screen, panel, "MainInterface__SimpleSelection")
		self.addCheckbox(screen, panel, "MainInterface__RapidUnitCycling")
		# </advc.004>
		# advc.154:
		self.addTextDropdown(screen, panel, panel, "MainInterface__UnitCyclingButtons", True)
		# advc.004k:
		self.addCheckbox(screen, panel, "MainInterface__SeaPatrol")
		# advc.002m:
		self.addTextDropdown(screen, panel, panel, "MainInterface__NukeMissionTime", True)

	def createTechSplashPanel(self, screen, panel):
		self.addLabel(screen, panel, "TechWindow", "Tech Splash Screen:")
		self.addTextDropdown(screen, panel, panel, "TechWindow__ViewType", True)
		self.addCheckbox(screen, panel, "TechWindow__CivilopediaText")
		self.addCheckbox(screen, panel, "TechWindow__ShowSSScreen") # advc.060

#	def createBuffyPanel(self, screen, panel):
#		self.addLabel(screen, panel, "BUFFY", "BUFFY:")
#		self.addCheckbox(screen, panel, "BUFFY__WarningsDawnOfMan")
#		self.addCheckbox(screen, panel, "BUFFY__WarningsSettings")

	def createInfoPanePanel(self, screen, panel):
		self.addLabel(screen, panel, "InfoPane", "Unit/Stack Info:")
		self.addCheckbox(screen, panel, "MainInterface__UnitMovementPointsFraction")
		self.addCheckbox(screen, panel, "MainInterface__StackMovementPoints")
		self.addCheckbox(screen, panel, "MainInterface__StackPromotions")
		left, center, right = self.addThreeColumnLayout(screen, panel, "StackPromotionColors")
		self.addCheckbox(screen, left, "MainInterface__StackPromotionCounts", True)
		self.addColorDropdown(screen, center, right, "MainInterface__StackPromotionColor", False)
		self.addColorDropdown(screen, center, right, "MainInterface__StackPromotionColorAll", False)
	
	# <advc.120c>
	def createSlidersPanel(self, screen, panel):
		self.addLabel(screen, panel, "Sliders", "Sliders:")
		# Moved these two from createMiscellaneousPanel
		self.addCheckbox(screen, panel, "MainInterface__MinMax_Commerce")
		self.addCheckbox(screen, panel, "MainInterface__Hide_EspSlider")
		# advc.004p:
		self.addCheckbox(screen, panel, "MainInterface__TotalCultureRate")
		#self.addCheckbox(screen, panel, "MainInterface__GoldRateWarning")
		# <advc.070>
		self.addLabel(screen, panel, "GoldRate", "Gold Rate:", None, True)
		panelLeft, panelRight = self.addTwoColumnLayout(screen, panel, "GoldRateOptions")
		self.addColorDropdown(screen, panelLeft, panelRight, "MainInterface__PositiveGoldRateColor", True)
		self.addColorDropdown(screen, panelLeft, panelRight, "MainInterface__NegativeGoldRateColor", True)
		# Disable this again to save space
		#self.addColorDropdown(screen, panelLeft, panelRight, "MainInterface__GoldRateBrokeColor", True)
		# </advc.070>

	# </advc.120c>
		
	def createMiscellaneousPanel(self, screen, panel):
		self.addLabel(screen, panel, "Misc", "Misc:")
		# <advc.106b>
		self.addCheckbox(screen, panel, "MainInterface__AutoOpenEventLog")
		self.addTextDropdown(screen, panel, panel, "MainInterface__MessageLimit", True)
		# </advc.106b>
		# advc.002n:
		self.addCheckbox(screen, panel, "MainInterface__EndTurnMessage")
		self.addCheckbox(screen, panel, "MainInterface__CityArrows")
