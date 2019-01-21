## BugNJAGCOptionsTab
##
## Tab for the BUG NJAGC Options.
##
## Copyright (c) 2007-2008 The BUG Mod.
##
## Author: EmperorFool

import BugOptionsTab

class BugNJAGCOptionsTab(BugOptionsTab.BugOptionsTab):
	"BUG NJAGC Options Screen Tab"
	
	def __init__(self, screen):
		BugOptionsTab.BugOptionsTab.__init__(self, "NJAGC", "Clock")

	def create(self, screen):
		tab = self.createTab(screen)
		panel = self.createMainPanel(screen)
		upperPanel = self.addOneColumnLayout(screen, panel)
		# <advc.002k>
		self.addTextDropdown(screen, upperPanel, upperPanel, "NJAGC__YearNotation", True)
		self.addSpacer(screen, upperPanel, "Clock_Tab_Top1")
		screen.attachHSeparator(upperPanel, upperPanel + "Sep")
		# </advc.002k>
		
		leftPanel, spaceLC, centerPanel, rightPanel = self.addMultiColumnLayout(screen, upperPanel, 4, "EraColors")
		# advc.067: Adding space between left and center column
		self.addSpacer(screen, spaceLC, "Extra_Space_CR", 20)
		# advc.067: Moved to bottom half
		#self.addCheckbox(screen, leftPanel, "NJAGC__Enabled")
		self.addCheckbox(screen, leftPanel, "NJAGC__ShowEra")
		# advc.067: New sub-option - don't show it after all
		#self.addCheckbox(screen, leftPanel, "NJAGC__ShowGameEra", True)
		# advc.067: was on leftPanel
		self.addCheckbox(screen, centerPanel, "NJAGC__ShowEraColor")
		centerPanelL, centerPanelR = self.addTwoColumnLayout(screen, centerPanel, "ShowEraColor_Column")
		self.addColorDropdown(screen, centerPanelL, centerPanelR, "NJAGC__Color_ERA_ANCIENT", True)
		self.addColorDropdown(screen, centerPanelL, centerPanelR, "NJAGC__Color_ERA_CLASSICAL", True)
		self.addColorDropdown(screen, centerPanelL, centerPanelR, "NJAGC__Color_ERA_MEDIEVAL", True)
		rightPanelL, rightPanelR = self.addTwoColumnLayout(screen, rightPanel, "ShowEraColor_Column")
		# advc.067: Moved from centerPanel
		self.addColorDropdown(screen, rightPanelL, rightPanelR, "NJAGC__Color_ERA_RENAISSANCE", True)
		self.addColorDropdown(screen, rightPanelL, rightPanelR, "NJAGC__Color_ERA_INDUSTRIAL", True)
		self.addColorDropdown(screen, rightPanelL, rightPanelR, "NJAGC__Color_ERA_MODERN", True)
		self.addColorDropdown(screen, rightPanelL, rightPanelR, "NJAGC__Color_ERA_FUTURE", True)
		# advc.067: Space above the separator
		self.addSpacer(screen, centerPanel, "Clock_Tab_Upper")
		screen.attachHSeparator(upperPanel, upperPanel + "Sep")

		leftPanel, rightPanel = self.addTwoColumnLayout(screen, upperPanel, "Views")
		# <advc.067> Instead of heading left and right, I'm placing the main 'Enabled' option above the left column and the Alternate option above the right column; the other options receive params for indentation. And each of the two top options receives a dropdown menu.
		#self.addCheckbox(screen, leftPanel, "NJAGC__Enabled")
		self.addCheckboxTextDropdown(screen, leftPanel, leftPanel, "NJAGC__Enabled", "NJAGC__PrimaryTiming")
		#self.addCheckbox(screen, leftPanel, "NJAGC__AlternateText")
		#self.addIntDropdown(screen, rightPanel, rightPanel, "NJAGC__AltTiming")
		self.addCheckboxTextDropdown(screen, rightPanel, rightPanel, "NJAGC__AlternateText", "NJAGC__AltTiming")
		#self.addLabel(screen, leftPanel, "NJAGC_Regular", "Standard View:")
		self.addCheckbox(screen, leftPanel, "NJAGC__ShowTime", True)
		self.addCheckbox(screen, leftPanel, "NJAGC__ShowCompletedTurns", True)
		self.addCheckbox(screen, leftPanel, "NJAGC__ShowTotalTurns", True)
		#self.addCheckbox(screen, leftPanel, "NJAGC__ShowCompletedPercent")
		self.addCheckbox(screen, leftPanel, "NJAGC__ShowDate", True)
		# removed
		#self.addLabel(screen, rightPanel, "NJAGC_Alternate", "Alternate View:")
		self.addCheckbox(screen, rightPanel, "NJAGC__ShowAltTime", True)
		self.addCheckbox(screen, rightPanel, "NJAGC__ShowAltCompletedTurns", True)
		self.addCheckbox(screen, rightPanel, "NJAGC__ShowAltTotalTurns", True)
		# removed
		#self.addCheckbox(screen, rightPanel, "NJAGC__ShowAltCompletedPercent")
		self.addCheckbox(screen, rightPanel, "NJAGC__ShowAltDate", True)
		# </advc.067>
