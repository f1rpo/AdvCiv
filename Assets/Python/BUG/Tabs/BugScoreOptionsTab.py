## BugScoreOptionsTab
##
## Tab for the BUG Scoreboard Options.
##
## Copyright (c) 2007-2008 The BUG Mod.
##
## Author: EmperorFool

import BugOptionsTab

class BugScoreOptionsTab(BugOptionsTab.BugOptionsTab):
	"BUG Scores Options Screen Tab"
	
	def __init__(self, screen):
		BugOptionsTab.BugOptionsTab.__init__(self, "Scores", "Scoreboard")

	def create(self, screen):
		tab = self.createTab(screen)
		panel = self.createMainPanel(screen)
		column = self.addOneColumnLayout(screen, panel)
		
		left, right = self.addTwoColumnLayout(screen, column, "Alerts", True)
		self.addLabel(screen, left, "Scores_General", "General:")
		self.addTextDropdown(screen, left, left, "Scores__DisplayName")
		self.addCheckbox(screen, left, "Scores__UsePlayerName")
		self.addCheckbox(screen, left, "Scores__ShowMinor")
		
		# advc.099: Commented out
		#self.addLabel(screen, left, "Scores_Dead_Civs", "Dead Civilizations:")
		self.addCheckbox(screen, left, "Scores__ShowDead")
		# <advc.099> Now sub-options of the above
		self.addCheckbox(screen, left, "Scores__TagDead", True)
		self.addCheckbox(screen, left, "Scores__GreyDead", True)
		# </advc.099>
		
		# <advc.004> I'm switching these columns b/c the dropdowns in the center make it look like the checkboxes on the right refer to the dropboxes. And I'm adding an empty column to the center, where more space is needed.
		empty0, col1, empty2, col3L, col3R = self.addMultiColumnLayout(screen, right, 5, "Scores_Power_Column")
		# Left margin (was width 3 before)
		self.addSpacer(screen, empty0, "Scores_New_Columns", 5)
		# Moved down so that the left margin applies to it
		self.addLabel(screen, col1, "Scores_New_Columns", "Additional Columns:")
		# </advc.004>
		self.addCheckboxTextDropdown(screen, col3L, col3R, "Scores__Power", "Scores__PowerFormula", "LAYOUT_LEFT")
		self.addIntDropdown(screen, col3L, col3R, "Scores__PowerDecimals", True, "LAYOUT_LEFT")
		self.addColorDropdown(screen, col3L, col3R, "Scores__PowerColor", True, "LAYOUT_LEFT")
		self.addFloatDropdown(screen, col3L, col3R, "Scores__PowerHighRatio", True, "LAYOUT_LEFT")
		self.addColorDropdown(screen, col3L, col3R, "Scores__PowerHighColor", True, "LAYOUT_LEFT")
		self.addFloatDropdown(screen, col3L, col3R, "Scores__PowerLowRatio", True, "LAYOUT_LEFT")
		self.addColorDropdown(screen, col3L, col3R, "Scores__PowerLowColor", True, "LAYOUT_LEFT")
		# advc.004: Horizontal space in the center
		self.addSpacer(screen, empty2, "Scores_New_Columns", 8)
		
		self.addCheckbox(screen, col1, "Scores__Delta")
		# advc.004: Now sub-option of the above
		self.addCheckbox(screen, col1, "Scores__DeltaIncludeCurrent", True)
		# advc.004: Moved up b/c this is neither an icon nor related to relations
		self.addCheckbox(screen, col1, "Scores__Cities")
		# advc.004: Was Scores_Icons/"Icons:" and that text key had been set to [SPACE], i.e. just an empty line.
		self.addLabel(screen, col1, "Scores_Relations", "Relations:")
		self.addCheckbox(screen, col1, "Scores__Attitude")
		self.addCheckbox(screen, col1, "Scores__WorstEnemy")
		#self.addCheckbox(screen, col1, "Scores__WHEOOH") # disabled by K-Mod
		
		screen.attachHSeparator(column, column + "Sep")
		
		left, space, center, right = self.addMultiColumnLayout(screen, column, 4, "Advanced_Scores_Column")
		# advc.004: Merge this label with the AlignIcons option
		#self.addLabel(screen, left, "Scores_Grid", "Advanced Layout:")
		# <advc.004> Indent these (True)
		self.addCheckbox(screen, left, "Scores__AlignIcons")
		self.addCheckbox(screen, left, "Scores__GroupVassals", True)
		self.addCheckbox(screen, left, "Scores__ColorCodeTeamScore", True)
		self.addCheckbox(screen, left, "Scores__ExpandOnHover", True)
		self.addCheckbox(screen, left, "Scores__LeftAlignName", True)
		self.addCheckboxIntDropdown(screen, left, left, "Scores__ResearchIcons", "Scores__ResearchIconSize", "right", True)
		# </advc.004>
		# advc.004: Last param was 3 (space between left and center column)
		self.addSpacer(screen, space, "Scores_Grid", 10)
		
		self.addSpacer(screen, center, "Scoreboard_Tab")
		#self.addLabel(screen, center, "Scores_Order", "Column Order:")
		
		centerL, centerR = self.addTwoColumnLayout(screen, center, "Scores")
		# <advc.004> Indentation removed (False); order changed.
		self.addIntDropdown(screen, centerL, centerR, "Scores__MaxPlayers", False, "LAYOUT_LEFT")
		self.addIntDropdown(screen, centerL, centerR, "Scores__LineHeight", False, "LAYOUT_LEFT")
		self.addIntDropdown(screen, centerL, centerR, "Scores__DefaultSpacing", False, "LAYOUT_LEFT")
		self.addTextEdit(screen, center, center, "Scores__DisplayOrder")
		# Label that tells players where to find a legend for the DisplayOrder
		self.addLabel(screen, center, "Scores_DisplayOrderHelp", None, None, True)
		# </advc.004>
