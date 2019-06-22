## BugAutologOptionsTab
##
## Tab for the BUG Autolog Options.
##
## Copyright (c) 2007-2008 The BUG Mod.
##
## Author: EmperorFool

import BugOptionsTab

class BugAutologOptionsTab(BugOptionsTab.BugOptionsTab):
	"BUG Autolog Options Screen Tab"
	
	def __init__(self, screen):
		BugOptionsTab.BugOptionsTab.__init__(self, "Autolog", "Logging")

	def create(self, screen):
		tab = self.createTab(screen)
		panel = self.createMainPanel(screen)
		column = self.addOneColumnLayout(screen, panel)
		
		left, right = self.addTwoColumnLayout(screen, column, "Autolog")
		self.addCheckbox(screen, left, "Autolog__Enabled")
		self.addCheckbox(screen, right, "Autolog__Silent")		
		
		# File and Format
		screen.attachHSeparator(column, column + "Sep1")
		# advc.004: Third column with a spacer added
		left, empty, right = self.addThreeColumnLayout(screen, column, "Options")
		self.addSpacer(screen, empty, "Autolog__CenterSpacer", 8)
		
		#self.addIntDropdown(screen, left, left, "Autolog__4000BC")
		# advc.004: Removing the above tentatively b/c I can't think of a good use. Also removed from autolog.py, autologEventManager.py and StatusDumpEventManager.py.
		# Replacing it with a spacer b/c I like how the file name and color coding options align with related options in the right column
		self.addSpacer(screen, left, "Autolog__Spacer0")
		self.addCheckbox(screen, left, "Autolog__DefaultFileName")
		# advc.004: Switched the next two b/c IBT is already about what events get logged
		self.addCheckbox(screen, left, "Autolog__ColorCoding")
		self.addCheckbox(screen, left, "Autolog__IBT")
		
		rightL, rightR = self.addTwoColumnLayout(screen, right, "File_Column")
		self.addTextEdit(screen, rightL, rightR, "Autolog__FilePath")
		self.addTextEdit(screen, rightL, rightR, "Autolog__FileName")
		# advc.004: Switched the next two (see previous comment)
		self.addTextDropdown(screen, rightL, rightR, "Autolog__Format")
		self.addTextEdit(screen, rightL, rightR, "Autolog__Prefix")
		
		# What to Log
		screen.attachHSeparator(column, column + "Sep2")
		#col1, col1, col2, col3, col4 = self.addMultiColumnLayout(screen, column, 4, "Events")
		left, right = self.addTwoColumnLayout(screen, column, "Events", False)
		
		# <advc.004> Rearranged all of these; new labels too.
		self.addLabel(screen, left, "Autolog_TechCivics")
		col1, col2, col3, col4 = self.addMultiColumnLayout(screen, right, 4)
		self.addCheckbox(screen, col1, "Autolog__LogTech")
		self.addCheckbox(screen, col2, "Autolog__LogSliders")
		self.addCheckbox(screen, col3, "Autolog__LogGA")
		self.addCheckbox(screen, col4, "Autolog__LogCivics")
		screen.attachHSeparator(right, right + "Sep3a")
		screen.attachHSeparator(left, left + "Sep3b")
		
		self.addLabel(screen, left, "Autolog_Expansion")
		col1, col2, col3, col4 = self.addMultiColumnLayout(screen, right, 4)
		self.addCheckbox(screen, col1, "Autolog__LogCityFounded")
		self.addCheckbox(screen, col2, "Autolog__LogCityOwner")
		self.addCheckbox(screen, col3, "Autolog__LogCityRazed")
		screen.attachHSeparator(right, right + "Sep4a")
		screen.attachHSeparator(left, left + "Sep4b")
		
		self.addLabel(screen, left, "Autolog_ManageCity")
		self.addSpacer(screen, left, "SpaceManageCity")
		col1, col2, col3, col4 = self.addMultiColumnLayout(screen, right, 4)
		self.addCheckbox(screen, col1, "Autolog__LogBuildStarted")
		self.addCheckbox(screen, col2, "Autolog__LogBuildCompleted")
		self.addCheckbox(screen, col3, "Autolog__LogProjects")
		self.addCheckbox(screen, col1, "Autolog__LogCityWhipStatus")
		self.addCheckbox(screen, col2, "Autolog__LogCityGrowth")
		self.addCheckbox(screen, col3, "Autolog__LogCityBorders")	
		screen.attachHSeparator(right, right + "Sep5a")
		screen.attachHSeparator(left, left + "Sep5b")
		
		self.addLabel(screen, left, "Autolog_Relations")
		col1, col2, col3, col4 = self.addMultiColumnLayout(screen, right, 4)
		self.addCheckbox(screen, col1, "Autolog__LogContact")
		self.addCheckbox(screen, col2, "Autolog__LogWar")
		self.addCheckbox(screen, col3, "Autolog__LogVassals")
		self.addCheckbox(screen, col4, "Autolog__LogAttitude")
		screen.attachHSeparator(right, right + "Sep6a")
		screen.attachHSeparator(left, left + "Sep6b")
		
		self.addLabel(screen, left, "Autolog_AIPropose")
		self.addSpacer(screen, left, "SpaceAIPropose")
		col1, col2, col3, col4 = self.addMultiColumnLayout(screen, right, 4)
		self.addCheckbox(screen, col1, "Autolog__LogTradeOffer")
		self.addCheckbox(screen, col2, "Autolog__LogWarDemand")
		self.addCheckbox(screen, col3, "Autolog__LogCivicDemand")
		# Was commented out already in BUG; haven't checked what it does exactly.
		#self.addCheckbox(screen, col4, "Autolog__LogTradeAll")
		self.addCheckbox(screen, col1, "Autolog__LogTributeDemand")
		self.addCheckbox(screen, col2, "Autolog__LogEmbargoDemand")
		self.addCheckbox(screen, col3, "Autolog__LogReligionDemand")	
		screen.attachHSeparator(right, right + "Sep7a")
		screen.attachHSeparator(left, left + "Sep7b")
		
		self.addLabel(screen, left, "Autolog_Units")
		self.addSpacer(screen, left, "SpaceUnits")
		col1, col2, col3, col4 = self.addMultiColumnLayout(screen, right, 4)
		self.addCheckbox(screen, col1, "Autolog__LogCombat")
		self.addCheckbox(screen, col2, "Autolog__LogPromotions")
		self.addCheckbox(screen, col3, "Autolog__LogImprovements")
		self.addCheckbox(screen, col4, "Autolog__LogPillage")
		self.addCheckbox(screen, col1, "Autolog__LogGoodies")
		self.addCheckbox(screen, col2, "Autolog__LogGP")
		self.addCheckbox(screen, col3, "Autolog__LogReligion")
		self.addCheckbox(screen, col4, "Autolog__LogCorporation")
		# </advc.004>
