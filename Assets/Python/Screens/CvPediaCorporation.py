## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
from CvPythonExtensions import *
import CvUtil
import ScreenInput
import CvScreenEnums
import string

# globals
gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class CvPediaCorporation:
	"Civilopedia Screen for Corporations"

	def __init__(self, main):
		self.iCorporation = -1
		self.top = main
		
		self.X_MAIN_PANE = 50
		self.Y_MAIN_PANE = 90
		self.W_MAIN_PANE = 250
		self.H_MAIN_PANE = 250

		self.X_ICON = 98
		self.Y_ICON = 125
		self.W_ICON = 150
		self.H_ICON = 190
		self.ICON_SIZE = 64
		
		self.X_REQUIRES = 330
		self.Y_REQUIRES = 70
		self.W_REQUIRES = 425
		self.H_REQUIRES = 110

		self.X_SPECIAL = 330
		self.Y_SPECIAL = 190
		self.W_SPECIAL = 425
		self.H_SPECIAL = 150

		self.X_TEXT = self.X_MAIN_PANE
		self.Y_TEXT = self.Y_MAIN_PANE + self.H_MAIN_PANE + 20
		self.W_TEXT = 705
		self.H_TEXT = 310
		
	# Screen construction function
	def interfaceScreen(self, iCorporation):	
			
		self.iCorporation = iCorporation
	
		self.top.deleteAllWidgets()						
							
		screen = self.top.getScreen()
		
		bNotActive = (not screen.isActive())
		if bNotActive:
			self.top.setPediaCommonWidgets()

		# Header...
		szHeader = u"<font=4b>" + gc.getCorporationInfo(self.iCorporation).getDescription().upper() + u"</font>"
		szHeaderId = self.top.getNextWidgetName()
		screen.setLabel(szHeaderId, "Background", szHeader, CvUtil.FONT_CENTER_JUSTIFY, self.top.X_SCREEN, self.top.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		
		# Top
		screen.setText(self.top.getNextWidgetName(), "Background", self.top.MENU_TEXT, CvUtil.FONT_LEFT_JUSTIFY, self.top.X_MENU, self.top.Y_MENU, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_MAIN, CivilopediaPageTypes.CIVILOPEDIA_PAGE_CORPORATION, -1)

		if self.top.iLastScreen	!= CvScreenEnums.PEDIA_CORPORATION or bNotActive:		
			self.placeLinks(true)
			self.top.iLastScreen = CvScreenEnums.PEDIA_CORPORATION
		else:
			self.placeLinks(false)

		# Icon
		screen.addPanel( self.top.getNextWidgetName(), "", "", False, False,
		    self.X_MAIN_PANE, self.Y_MAIN_PANE, self.W_MAIN_PANE, self.H_MAIN_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		screen.addPanel(self.top.getNextWidgetName(), "", "", false, false,
		    self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_MAIN)
		screen.addDDSGFC(self.top.getNextWidgetName(), gc.getCorporationInfo(self.iCorporation).getButton(),
		    self.X_ICON + self.W_ICON/2 - self.ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - self.ICON_SIZE/2, self.ICON_SIZE, self.ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		self.placeSpecial()
		self.placeRequires()
		self.placeText()

	def placeRequires(self):
		
		screen = self.top.getScreen()
		
		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_REQUIRES", ()), "", false, true,
				 self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES, PanelStyles.PANEL_STYLE_BLUE50 )
		screen.attachLabel(panelName, "", "  ")
		
		iTech = gc.getCorporationInfo(self.iCorporation).getTechPrereq()
		if (iTech > -1):
			screen.attachImageButton( panelName, "", gc.getTechInfo(iTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, 1, False )
			
		for iBuilding in range(gc.getNumBuildingInfos()):
			if (gc.getBuildingInfo(iBuilding).getFoundsCorporation() == self.iCorporation):
				screen.attachImageButton( panelName, "", gc.getBuildingInfo(iBuilding).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, 1, False )
				
		for iUnit in range(gc.getNumUnitInfos()):
			bRequired = false
			for iBuilding in range(gc.getNumBuildingInfos()):
				if (gc.getBuildingInfo(iBuilding).getFoundsCorporation() == self.iCorporation):
					if gc.getUnitInfo(iUnit).getBuildings(iBuilding) or gc.getUnitInfo(iUnit).getForceBuildings(iBuilding):
						bRequired = true
						break

			if bRequired:
				szButton = gc.getUnitInfo(iUnit).getButton()
				if self.top.iActivePlayer != -1:
					szButton = gc.getPlayer(self.top.iActivePlayer).getUnitButton(iUnit)
				screen.attachImageButton( panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False )
			
	def placeSpecial(self):
		
		screen = self.top.getScreen()
		
		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_EFFECTS", ()), "", true, false,
				 self.X_SPECIAL, self.Y_SPECIAL, self.W_SPECIAL, self.H_SPECIAL, PanelStyles.PANEL_STYLE_BLUE50 )
				
		listName = self.top.getNextWidgetName()

		szSpecialText = CyGameTextMgr().parseCorporationInfo(self.iCorporation, True)[1:]
		screen.addMultilineText(listName, szSpecialText, self.X_SPECIAL+5, self.Y_SPECIAL+30, self.W_SPECIAL-10, self.H_SPECIAL-35, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)	
				
	def placeText(self):
		
		screen = self.top.getScreen()
		
		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, "", "", true, true,
				 self.X_TEXT, self.Y_TEXT, self.W_TEXT, self.H_TEXT, PanelStyles.PANEL_STYLE_BLUE50 )
 
		szText = gc.getCorporationInfo(self.iCorporation).getCivilopedia()
		screen.attachMultilineText( panelName, "Text", szText, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		
	def placeLinks(self, bRedraw):

		screen = self.top.getScreen()
        
		if bRedraw:
			screen.clearListBoxGFC(self.top.LIST_ID)
		
		# sort Improvements alphabetically
		listSorted=[(0,0)]*gc.getNumCorporationInfos()
		for j in range(gc.getNumCorporationInfos()):
			listSorted[j] = (gc.getCorporationInfo(j).getDescription(), j)
		listSorted.sort()	
			
		iSelected = 0
		i = 0
		for iI in range(gc.getNumCorporationInfos()):
			if (not gc.getCorporationInfo(listSorted[iI][1]).isGraphicalOnly()):
				if bRedraw:
					screen.appendListBoxStringNoUpdate( self.top.LIST_ID, listSorted[iI][0], WidgetTypes.WIDGET_PEDIA_JUMP_TO_CORPORATION, listSorted[iI][1], 0, CvUtil.FONT_LEFT_JUSTIFY )
				if listSorted[iI][1] == self.iCorporation:
					iSelected = i
				i += 1
				
		if bRedraw:
			screen.updateListBox(self.top.LIST_ID)

		screen.setSelectedListBoxStringGFC(self.top.LIST_ID, iSelected)
		
	# Will handle the input for this screen...
	def handleInput (self, inputClass):
		return 0


