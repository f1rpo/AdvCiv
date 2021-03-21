## BugCityScreenOptionsTab
##
## Tab for the BUG City Screen Options.
##
## Copyright (c) 2007-2008 The BUG Mod.
##
## Author: EmperorFool

import BugOptionsTab

class BugCityScreenOptionsTab(BugOptionsTab.BugOptionsTab):
	"BUG City Screen Options Screen Tab"
	
	def __init__(self, screen):
		BugOptionsTab.BugOptionsTab.__init__(self, "CityScreen", "City Screen")

	def create(self, screen):
		tab = self.createTab(screen)
		panel = self.createMainPanel(screen)
		column = self.addOneColumnLayout(screen, panel)
		
		#left, right = self.addTwoColumnLayout(screen, column, "Page", True)
		left, middle, right = self.addThreeColumnLayout(screen, column, "Page", True) # K-Mod
		
		self.createRawYieldsPanel(screen, left)
		self.addSpacer(screen, left, "CityScreen1")
		self.createHurryDetailPanel(screen, left)
		self.addSpacer(screen, left, "CityScreen2")
		self.createBuildingActualEffectsPanel(screen, middle) # was left
		#self.addSpacer(screen, left, "CityScreen3")
		self.createGreatPersonBarPanel(screen, left)
		self.addSpacer(screen, middle, "CityScreen4") # was left
		self.createProductionQueuePanel(screen, middle) # was left
		
		#self.createCityBarPanel(screen, right)
		#self.addSpacer(screen, right, "CityScreen6")
		self.createMiscellaneousPanel(screen, right)
		
	def createRawYieldsPanel(self, screen, panel):
		###self.addCheckboxTextDropdown(screen, left, left, "CityScreen__RawYields", "CityScreen__RawYields_View")
		self.addCheckbox(screen, panel, "CityScreen__RawYields")
		self.addTextDropdown(screen, panel, panel, "CityScreen__RawYieldsView", True)
		# advc.004: I really don't see anyone selecting anything other than "worked tiles" as the default counting method
		#self.addTextDropdown(screen, panel, panel, "CityScreen__RawYieldsTiles", True)
		
	def createHurryDetailPanel(self, screen, panel):
		self.addLabel(screen, panel, "HurryDetail", "Hurry Detail:")
		# advc.064: Moved from Misc
		self.addCheckbox(screen, panel, "CityScreen__Anger_Counter")
		left, right = self.addTwoColumnLayout(screen, panel, "HurryDetail", False)
		#self.addCheckbox(screen, left, "CityBar__HurryAssist")
		#self.addCheckbox(screen, right, "CityBar__HurryAssistIncludeCurrent")
		# <advc.064> Restored (and BULL code merged)
		self.addCheckbox(screen, left, "MiscHover__HurryOverflow")
		self.addCheckbox(screen, right, "MiscHover__HurryOverflowIncludeCurrent")
		# </advc.064>
		self.addCheckbox(screen, left, "CityScreen__WhipAssist")
		self.addCheckbox(screen, right, "CityScreen__WhipAssistOverflowCountCurrentProduction")
		# advc.064: New option
		self.addCheckbox(screen, left, "CityScreen__HurryTickMarks")
		
	def createBuildingActualEffectsPanel(self, screen, panel):
		self.addLabel(screen, panel, "BuildingEffects", "Building Actual Effects in Hovers:")
		left, right = self.addTwoColumnLayout(screen, panel, "BuildingEffects", False)
		self.addCheckbox(screen, left, "MiscHover__BuildingActualEffects")
		# BULL - Food Rate Hover:
		self.addCheckbox(screen, left, "MiscHover__BuildingAdditionalFood")
		self.addCheckbox(screen, left, "MiscHover__BuildingAdditionalProduction")
		self.addCheckbox(screen, left, "MiscHover__BuildingAdditionalCommerce")
		self.addCheckbox(screen, left, "MiscHover__BuildingSavedMaintenance")
		#self.addSpacer(screen, right, "CityScreen2a")
		# advc.063 (instead of the spacer):
		self.addCheckbox(screen, right, "MiscHover__SpecialistActualEffects")
		self.addCheckbox(screen, right, "MiscHover__BuildingAdditionalHealth")
		self.addCheckbox(screen, right, "MiscHover__BuildingAdditionalHappiness")
		self.addCheckbox(screen, right, "MiscHover__BuildingAdditionalGreatPeople")
		#self.addCheckbox(screen, right, "MiscHover__BuildingAdditionalDefense")
		
	def createGreatPersonBarPanel(self, screen, panel):
		self.addLabel(screen, panel, "GreatPersonBar", "Great Person Bar:")
		self.addCheckbox(screen, panel, "CityScreen__GreatPersonTurns")
		self.addCheckbox(screen, panel, "CityScreen__GreatPersonInfo")
		#self.addCheckbox(screen, panel, "MiscHover__GreatPeopleRateBreakdown")
		
	def createProductionQueuePanel(self, screen, panel):
		self.addLabel(screen, panel, "ProductionQueue", "Production Queue:")
		self.addCheckbox(screen, panel, "CityScreen__ProductionStarted")
		#left, center, right = self.addThreeColumnLayout(screen, panel, "ProductionDecay")
		#self.addLabel(screen, left, "ProductionDecay", "Decay:")
		#self.addCheckbox(screen, center, "CityScreen__ProductionDecayQueue")
		#self.addCheckbox(screen, right, "CityScreen__ProductionDecayHover")
		#self.addIntDropdown(screen, left, center, "CityScreen__ProductionDecayQueueUnitThreshold", True)
		#self.addIntDropdown(screen, left, center, "CityScreen__ProductionDecayQueueBuildingThreshold", True)
		#self.addIntDropdown(screen, None, right, "CityScreen__ProductionDecayHoverUnitThreshold")
		#self.addIntDropdown(screen, None, right, "CityScreen__ProductionDecayHoverBuildingThreshold")
		
	#def createCityBarPanel(self, screen, panel):
		#self.addLabel(screen, panel, "CitybarHover", "City Bar Hover:")
		#left, right = self.addTwoColumnLayout(screen, panel, "CityBarHover", False)
		
		#self.addCheckbox(screen, left, "CityBar__BaseValues")
		#self.addCheckbox(screen, left, "CityBar__Health")
		#self.addCheckbox(screen, left, "CityBar__Happiness")
		#self.addCheckbox(screen, left, "CityBar__FoodAssist")
		#self.addCheckbox(screen, left, "CityBar__BaseProduction")
		#self.addCheckbox(screen, left, "CityBar__TradeDetail")
		#self.addCheckbox(screen, left, "CityBar__Commerce")
		#self.addCheckbox(screen, left, "CityBar__CultureTurns")
		#self.addCheckbox(screen, left, "CityBar__GreatPersonTurns")
		
		#self.addLabel(screen, right, "Cityanger", "City Anger:")
		#self.addCheckbox(screen, right, "CityBar__HurryAnger")
		#self.addCheckbox(screen, right, "CityBar__DraftAnger")
		
		#self.addSpacer(screen, right, "CityScreen5")
		#self.addCheckbox(screen, right, "CityBar__BuildingActualEffects")
		#self.addCheckbox(screen, right, "CityBar__BuildingIcons")
		#self.addCheckbox(screen, right, "CityBar__Specialists")
		#self.addCheckbox(screen, right, "CityBar__RevoltChance")
		#self.addCheckbox(screen, right, "CityBar__HideInstructions")
		# EF: Airport Icons option is on Map tab
		###self.addCheckbox(screen, right, "CityBar__AirportIcons")
		
	def createMiscellaneousPanel(self, screen, panel):
		self.addLabel(screen, panel, "Misc", "Miscellaneous:")
		# advc.004t: New option
		self.addCheckbox(screen, panel, "CityScreen__ClickMapToExit")
		# advc.004: Moved up
		self.addTextDropdown(screen, panel, panel, "CityScreen__Specialists", True)
		#self.addCheckbox(screen, panel, "MiscHover__RemoveSpecialist")
		
		# advc.065: No longer optional
		#self.addCheckbox(screen, panel, "MiscHover__BaseCommerce")
		self.addCheckbox(screen, panel, "CityScreen__FoodAssist")
		
		# (advc.064: Anger_Counter moved to HurryDetail)
		
		# advc.065: No longer optional
		#self.addCheckbox(screen, panel, "CityScreen__CultureTurns")
		# advc.004: Already shown on the General tab, once should be enough.
		#self.addCheckbox(screen, panel, "MainInterface__ProgressBarsTickMarks")
		self.addCheckbox(screen, panel, "CityScreen__OnlyPresentReligions")
		self.addCheckbox(screen, panel, "CityScreen__OnlyPresentCorporations")
		
		#self.addCheckbox(screen, panel, "MiscHover__UnitExperience")
		#self.addCheckbox(screen, panel, "MiscHover__UnitExperienceModifiers")
		#self.addCheckbox(screen, panel, "MiscHover__ConscriptUnit")
		#self.addCheckbox(screen, panel, "MiscHover__ConscriptLimit")
		#self.addCheckbox(screen, panel, "CityScreen__ProductionPopupTrainCivilianUnitsForever")
		#self.addCheckbox(screen, panel, "CityScreen__ProductionPopupTrainMilitaryUnitsForever")
