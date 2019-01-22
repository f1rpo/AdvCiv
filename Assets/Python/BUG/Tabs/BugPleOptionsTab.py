## BugPleOptionsTab
##
## Tab for the BUG PLE Options.
##
## Copyright (c) 2008 The BUG Mod.
##
## Author: EmperorFool

import BugOptionsTab

class BugPleOptionsTab(BugOptionsTab.BugOptionsTab):
	"Plot List Enhancement Tab"
	
	def __init__(self, screen):
		BugOptionsTab.BugOptionsTab.__init__(self, "PLE", "Plot List")

	def create(self, screen):
		tab = self.createTab(screen)
		panel = self.createMainPanel(screen)
		# advc.069: was a single ThreeColumnLayout
		left, rest = self.addTwoColumnLayout(screen, panel, panel, True)
		center, spaceCR, right = self.addThreeColumnLayout(screen, panel, rest)
		self.addSpacer(screen, spaceCR, "Extra_Space_CR", 5)
		
		# advc.069: Indicator settings moved to the top (not dependent on draw method)
		self.addLabel(screen, left, "PLE_Indicators")
		self.addCheckbox(screen, left, "PLE__Wounded_Indicator")
		self.addCheckbox(screen, left, "PLE__Lead_By_GG_Indicator")
		self.addCheckbox(screen, left, "PLE__Upgrade_Indicator")
		self.addCheckbox(screen, left, "PLE__Mission_Info")
		self.addCheckbox(screen, left, "PLE__Promotion_Indicator")
		# advc.002e (and moved the Promotion_Indicator option down):
		self.addCheckbox(screen, left, "PLE__ShowPromotionGlow")
		#self.addSpacer(screen, left, "PLE_Indicators", 1)
		# advc.069: Heading instead of spacer
		self.addLabel(screen, left, "PLE_Advanced")
		
		# advc.069: New checkbox replacing the Draw_Method dropdown
		self.addCheckbox(screen, left, "PLE__BUG_Style")
		
		# advc.069: Health bar settings moved to the left column b/c these work with BUG or PLE.
		self.addCheckbox(screen, left, "PLE__Health_Bar")
		leftL, leftR = self.addTwoColumnLayout(screen, left, "Health_Bar_Column")
		# advc.069: Indentation params (True) added, except to the Fighting one b/c it looked strange.
		self.addColorDropdown(screen, leftL, leftR, "PLE__Healthy_Color", True)
		self.addColorDropdown(screen, leftL, leftR, "PLE__Wounded_Color", True)
		self.addCheckbox(screen, left, "PLE__Hide_Health_Fighting")


		
		#self.addSpacer(screen, left, "PLE_Tab") # advc.069: Not needed
		#self.addSpacer(screen, left, "PLE__Spacing")
		#self.addTextEdit(screen, left, left, "PLE__Horizontal_Spacing")
		#self.addTextEdit(screen, left, left, "PLE__Vertical_Spacing")
		#self.addCheckbox(screen, left, "PLE__Enabled")
		
		# advc.069: PLE-only functions now on the center and right column
		self.addCheckbox(screen, center, "PLE__PLE_Style")
		self.addCheckbox(screen, center, "PLE__Show_Buttons")
		centerL, centerR = self.addTwoColumnLayout(screen, center, "Show_Buttons_Column")
		# Replaced by checkbox BUG_Style
		#self.addTextDropdown(screen, leftL, leftR, "PLE__Draw_Method", True, "LAYOUT_LEFT")
		# Moved up; much more important than the defaults.
		self.addTextDropdown(screen, centerL, centerR, "PLE__Filter_Behavior", True, "LAYOUT_LEFT")
		self.addTextDropdown(screen, centerL, centerR, "PLE__Default_View_Mode", True, "LAYOUT_LEFT")
		self.addTextDropdown(screen, centerL, centerR, "PLE__Default_Group_Mode", True, "LAYOUT_LEFT")
		
		self.addSpacer(screen, center, "PLE__Bars", 1)
		self.addCheckbox(screen, center, "PLE__Move_Bar")
		centerL, centerR = self.addTwoColumnLayout(screen, center, "Move_Bar_Column")
		# advc.069: Indent params added (True)
		self.addColorDropdown(screen, centerL, centerR, "PLE__Full_Movement_Color", True)
		self.addColorDropdown(screen, centerL, centerR, "PLE__Has_Moved_Color", True)
		self.addColorDropdown(screen, centerL, centerR, "PLE__No_Movement_Color", True)
		
		# advc.069: Highlighter moved to center column (was right column)
		self.addSpacer(screen, center, "PLE_Move_Highlighter", 1)
 		self.addCheckbox(screen, center, "PLE__Move_Highlighter")
		
		# advc.069: To make the PLE_STYLE option in the center column stand out
		self.addSpacer(screen, right, "PLE_Right_Top")
		
		self.addLabel(screen, right, "PLE_Unit_Info_Tooltip")
		#self.addCheckbox(screen, right, "PLE__Info_Pane")  # EF: Can't get it to work
		#self.addTextEdit(screen, right, right, "PLE__Info_Pane_X")
		#self.addTextEdit(screen, right, right, "PLE__Info_Pane_Y")
		
		self.addColorDropdown(screen, right, right, "PLE__Unit_Name_Color")
		
		# advc.069: Moved up b/c the upgrade cost colors are even more unimportant. Indented.
		self.addLabel(screen, right, "PLE_Specialties", None, None, True)
		rightL, rightR = self.addTwoColumnLayout(screen, right, "PLE_Specialties_Column")
		self.addColorDropdown(screen, rightL, rightR, "PLE__Unit_Type_Specialties_Color")
		self.addColorDropdown(screen, rightL, rightR, "PLE__Promotion_Specialties_Color")
		# advc.069: Moved under the promotions heading
		self.addIntDropdown(screen, right, right, "PLE__Info_Pane_Promo_Icon_Offset_Y")
		
		# advc.069: Moved down; label indented.
		self.addLabel(screen, right, "PLE_Upgrade_Cost", None, None, True)
		rightL, rightR = self.addTwoColumnLayout(screen, right, "PLE_Upgrade_Cost_Column")
		self.addColorDropdown(screen, rightL, rightR, "PLE__Upgrade_Possible_Color")
		self.addColorDropdown(screen, rightL, rightR, "PLE__Upgrade_Not_Possible_Color")
