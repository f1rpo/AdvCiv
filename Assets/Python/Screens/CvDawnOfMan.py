## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005

import CvUtil
from CvPythonExtensions import *

ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()
gc = CyGlobalContext()

class CvDawnOfMan:
	"Dawn of man screen"
	def __init__(self, iScreenID):
		self.iScreenID = iScreenID
		
		self.X_SCREEN = 0
		self.Y_SCREEN = 0
		self.W_SCREEN = 1024
		self.H_SCREEN = 768
		
		self.W_TECH = 425
		self.H_TECH = 80

		self.W_MAIN_PANEL = 750 # Was 550
		
		self.H_MAIN_PANEL = 500
		
		self.X_MAIN_PANEL = (self.W_SCREEN/2) - (self.W_MAIN_PANEL/2)# Was 250
		
		self.Y_MAIN_PANEL = 70
		
		self.iMarginSpace = 15
		
		self.X_HEADER_PANEL = self.X_MAIN_PANEL + self.iMarginSpace
		self.Y_HEADER_PANEL = self.Y_MAIN_PANEL + self.iMarginSpace
		self.W_HEADER_PANEL = self.W_MAIN_PANEL - (self.iMarginSpace * 2)
		self.H_HEADER_PANEL = int(self.H_MAIN_PANEL * (2.0 / 5.0))
		
		self.X_LEADER_ICON = self.X_HEADER_PANEL + self.iMarginSpace
		self.Y_LEADER_ICON = self.Y_HEADER_PANEL + self.iMarginSpace
		self.H_LEADER_ICON = self.H_HEADER_PANEL - (15 * 2)#140
		self.W_LEADER_ICON = int(self.H_LEADER_ICON / 1.272727)#110
		
		
		self.X_FANCY_ICON1 = self.X_HEADER_PANEL + 170
		self.X_FANCY_ICON2 = self.X_HEADER_PANEL + (self.W_MAIN_PANEL - 120) # Was 430
		self.Y_FANCY_ICON = (self.Y_HEADER_PANEL + self.iMarginSpace + 6) - 6
		self.WH_FANCY_ICON = 64
		
		self.X_LEADER_TITLE_TEXT = (self.X_FANCY_ICON1+self.WH_FANCY_ICON)+((self.X_FANCY_ICON2 - (self.X_FANCY_ICON1+self.WH_FANCY_ICON))/2) - ((self.W_HEADER_PANEL / 3)/2)
		
		self.Y_LEADER_TITLE_TEXT = self.Y_HEADER_PANEL + self.iMarginSpace + 6
		self.W_LEADER_TITLE_TEXT = self.W_HEADER_PANEL / 3
		self.H_LEADER_TITLE_TEXT = self.H_HEADER_PANEL / 2
		
		self.X_STATS_TEXT = self.X_FANCY_ICON1# + self.W_LEADER_ICON + (self.iMarginSpace * 2) + 5
		self.Y_STATS_TEXT = self.Y_LEADER_TITLE_TEXT + 75
		self.W_STATS_TEXT = int(self.W_HEADER_PANEL * (5 / 7.0)) + (self.iMarginSpace * 2)
		self.H_STATS_TEXT = int(self.H_HEADER_PANEL * (3 / 5.0)) - (self.iMarginSpace * 2)
		
		self.X_TEXT_PANEL = self.X_HEADER_PANEL
		self.Y_TEXT_PANEL = self.Y_HEADER_PANEL + self.H_HEADER_PANEL + self.iMarginSpace - 10 #10 is the fudge factor
		self.W_TEXT_PANEL = self.W_HEADER_PANEL
		self.H_TEXT_PANEL = self.H_MAIN_PANEL - self.H_HEADER_PANEL - (self.iMarginSpace * 3) + 10 #10 is the fudge factor
		self.iTEXT_PANEL_MARGIN = 35
		
		self.W_EXIT = 120
		self.H_EXIT = 30
		
		self.X_EXIT = (self.W_SCREEN/2) - (self.W_EXIT/2) # Was 460
		self.Y_EXIT = self.Y_MAIN_PANEL + 440
				
				
	def interfaceScreen(self):
		'Use a popup to display the opening text'
		if ( CyGame().isPitbossHost() ):
			return
		
		self.calculateSizesAndPositions()
		
		self.player = gc.getPlayer(gc.getGame().getActivePlayer())
		self.EXIT_TEXT = localText.getText("TXT_KEY_SCREEN_CONTINUE", ())
		
		# Create screen
		
		screen = CyGInterfaceScreen( "CvDawnOfMan", self.iScreenID )		
		screen.showScreen(PopupStates.POPUPSTATE_QUEUED, False)
		screen.showWindowBackground( False )
		screen.setDimensions(self.X_SCREEN, screen.centerY(self.Y_SCREEN), self.W_SCREEN, self.H_SCREEN)
		screen.enableWorldSounds( false )
		
		# Create panels
		
		# Main
		szMainPanel = "DawnOfManMainPanel"
		screen.addPanel( szMainPanel, "", "", true, true,
			self.X_MAIN_PANEL, self.Y_MAIN_PANEL, self.W_MAIN_PANEL, self.H_MAIN_PANEL, PanelStyles.PANEL_STYLE_MAIN )
		
		# <advc.704>
		riseFall = gc.getGame().isOption(GameOptionTypes.GAMEOPTION_RISE_FALL)
		rfChapter = -1
		if riseFall:
			rfChapter = gc.getGame().getCurrentChapter()
		# </advc.704>
		
		# Top
		# advc.250c: Title "The Dawn of Man" doesn't make sense when starting in a later era.
		# advc.704: Treat later chapters like later-era start.
		isLaterEraStart = (gc.getGame().getStartEra() > 0 or gc.getGame().getCurrentEra() > 0 or rfChapter > 0)
		if not isLaterEraStart:
			szHeaderPanel = "DawnOfManHeaderPanel"
			screen.addPanel( szHeaderPanel, "", "", true, false,
				self.X_HEADER_PANEL, self.Y_HEADER_PANEL, self.W_HEADER_PANEL, self.H_HEADER_PANEL, PanelStyles.PANEL_STYLE_DAWNTOP )
		
		# Bottom
		szTextPanel = "DawnOfManTextPanel"
		screen.addPanel( szTextPanel, "", "", true, true,
			self.X_TEXT_PANEL, self.Y_TEXT_PANEL, self.W_TEXT_PANEL, self.H_TEXT_PANEL, PanelStyles.PANEL_STYLE_DAWNBOTTOM )
		
		# Add contents
		
		# Leaderhead graphic
		szLeaderPanel = "DawnOfManLeaderPanel"
		screen.addPanel( szLeaderPanel, "", "", true, false,
			self.X_LEADER_ICON - 3, self.Y_LEADER_ICON - 5, self.W_LEADER_ICON + 6, self.H_LEADER_ICON + 8, PanelStyles.PANEL_STYLE_DAWNTOP )
		screen.addLeaderheadGFC("LeaderHead", self.player.getLeaderType(), AttitudeTypes.ATTITUDE_PLEASED,
			self.X_LEADER_ICON + 5, self.Y_LEADER_ICON + 5, self.W_LEADER_ICON - 10, self.H_LEADER_ICON - 10, WidgetTypes.WIDGET_GENERAL, -1, -1)
		
		# Info/"Stats" text
		
		szNameText = "<color=255,255,0,255>" + u"<font=3b>" + gc.getLeaderHeadInfo(self.player.getLeaderType()).getDescription().upper() + u"</font>"
		szNameText += "\n- " + self.player.getCivilizationDescription(0) + " -\n"
		szNameText += u"<font=2>" + CyGameTextMgr().parseLeaderTraits(self.player.getLeaderType(), self.player.getCivilizationType(), True, False) + u"</font>"
		screen.addMultilineText( "NameText", szNameText, self.X_LEADER_TITLE_TEXT, self.Y_LEADER_TITLE_TEXT, self.W_LEADER_TITLE_TEXT, self.H_LEADER_TITLE_TEXT, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
		
		if not isLaterEraStart: # advc.250c: No free tech
			screen.addMultilineText( "HeaderText2", localText.getText("TXT_KEY_FREE_TECHS", ()) + ":", self.X_STATS_TEXT, self.Y_STATS_TEXT+15, self.W_STATS_TEXT, self.H_STATS_TEXT, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			screen.addPanel( "HeaderText3", "", "", false, true,
				 self.X_STATS_TEXT, self.Y_STATS_TEXT+30, self.W_TECH, self.H_TECH, PanelStyles.PANEL_STYLE_EMPTY )
		
			for iTech in range(gc.getNumTechInfos()):
				if (gc.getCivilizationInfo(self.player.getCivilizationType()).isCivilizationFreeTechs(iTech)):
					screen.attachImageButton( "HeaderText3", "", gc.getTechInfo(iTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, 1, False )
			
		self.Text_BoxText = CyGameTextMgr().parseCivInfos(self.player.getCivilizationType(), True)
		
		screen.addMultilineText( "HeaderText4", self.Text_BoxText, self.X_STATS_TEXT, self.Y_STATS_TEXT+30+self.H_TECH, self.W_STATS_TEXT - (self.iMarginSpace * 3), self.H_STATS_TEXT - (self.iMarginSpace * 4), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
							
		# Fancy icon things
		screen.addDDSGFC( "IconLeft", ArtFileMgr.getCivilizationArtInfo(gc.getCivilizationInfo(self.player.getCivilizationType()).getArtDefineTag()).getButton(), self.X_FANCY_ICON1 , self.Y_FANCY_ICON , self.WH_FANCY_ICON, self.WH_FANCY_ICON, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		screen.addDDSGFC( "IconRight", ArtFileMgr.getCivilizationArtInfo(gc.getCivilizationInfo(self.player.getCivilizationType()).getArtDefineTag()).getButton(), self.X_FANCY_ICON2 , self.Y_FANCY_ICON , self.WH_FANCY_ICON, self.WH_FANCY_ICON, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		
		# Main Body text
		if not isLaterEraStart: # advc.250c
			szDawnTitle = u"<font=3>" + localText.getText("TXT_KEY_DAWN_OF_MAN_SCREEN_TITLE", ()).upper() + u"</font>"
			screen.setLabel("DawnTitle", "Background", szDawnTitle, CvUtil.FONT_CENTER_JUSTIFY, self.X_TEXT_PANEL + (self.W_TEXT_PANEL / 2), self.Y_TEXT_PANEL + 15, -2.0, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		# <advc.250c> Show the first text block only for Ancient start
		bodyString1 = localText.getText("TXT_KEY_DAWN_OF_MAN_TEXT1", (CyGameTextMgr().getTimeStr(gc.getGame().getGameTurn(), false), self.player.getCivilizationAdjectiveKey()))
		bodyString2 = localText.getText("TXT_KEY_DAWN_OF_MAN_TEXT2", (self.player.getNameKey(),))
		bodyString = ""
		# <advc.704>
		if riseFall:
			len = gc.getGame().getChapterEnd(rfChapter) - gc.getGame().getChapterStart(rfChapter) + 1;
			bodyString2 = localText.getText("TXT_KEY_RF_DOM_FOUND", (len, self.player.getCivilizationAdjectiveKey()))
			chapterNumber = rfChapter + 1 # Start at 1
			chp = localText.getText("TXT_KEY_RF_CHAPTER", ())
			chapterString = localText.getText("TXT_KEY_RF_CHAPTER", ()) + " " + str(chapterNumber)
			if gc.getGame().getCurrentChapter() == gc.getGame().getMaxChapters():
				chapterString = localText.getText("TXT_KEY_RF_DOM_FINAL", ()) + " " + chp
			chapterString += ". "
			bodyString = chapterString
			if rfChapter > 0: # not the first chapter
				bodyString += localText.getText("TXT_KEY_RF_DOM", (CyGameTextMgr().getTimeStr(gc.getGame().getGameTurn(), false), len, self.player.getCivilizationAdjectiveKey()))
		if rfChapter <= 0:
		# </advc.704>
			if not isLaterEraStart:
				bodyString += bodyString1
			bodyString += bodyString2
		# </advc.250c>
		screen.addMultilineText( "BodyText", bodyString, self.X_TEXT_PANEL + self.iMarginSpace, self.Y_TEXT_PANEL + self.iMarginSpace + self.iTEXT_PANEL_MARGIN, self.W_TEXT_PANEL - (self.iMarginSpace * 2), self.H_TEXT_PANEL - (self.iMarginSpace * 2) - 75, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		
		screen.setButtonGFC("Exit", self.EXIT_TEXT, "", self.X_EXIT, self.Y_EXIT, self.W_EXIT, self.H_EXIT, WidgetTypes.WIDGET_CLOSE_SCREEN, -1, -1, ButtonStyles.BUTTON_STYLE_STANDARD )
		
		pActivePlayer = gc.getPlayer(CyGame().getActivePlayer())
		pLeaderHeadInfo = gc.getLeaderHeadInfo(pActivePlayer.getLeaderType())
		screen.setSoundId(CyAudioGame().Play2DSoundWithId(pLeaderHeadInfo.getDiploPeaceMusicScriptIds(0)))
		
	def handleInput( self, inputClass ):
		return 0
	
	def update(self, fDelta):
		return
		
	def onClose(self):
		CyInterface().setSoundSelectionReady(true)		
		return 0
			
	def calculateSizesAndPositions(self):
		self.X_SCREEN = 0
		self.Y_SCREEN = 0
		
		screen = CyGInterfaceScreen( "CvDawnOfMan", self.iScreenID )
		
		self.W_SCREEN = screen.getXResolution()
		self.H_SCREEN = screen.getYResolution()		
		
		self.W_TECH = 425
		self.H_TECH = 80

		self.W_MAIN_PANEL = 750 # Was 550
		
		self.H_MAIN_PANEL = 575 # advc.002b: Was 525.
		self.X_MAIN_PANEL = (self.W_SCREEN/2) - (self.W_MAIN_PANEL/2)# Was 250
		
		self.Y_MAIN_PANEL = 70
		
		self.iMarginSpace = 15
		
		self.X_HEADER_PANEL = self.X_MAIN_PANEL + self.iMarginSpace
		self.Y_HEADER_PANEL = self.Y_MAIN_PANEL + self.iMarginSpace
		self.W_HEADER_PANEL = self.W_MAIN_PANEL - (self.iMarginSpace * 2)
		self.H_HEADER_PANEL = int(self.H_MAIN_PANEL * (2.0 / 5.0)) + 60
		
		self.X_LEADER_ICON = self.X_HEADER_PANEL + self.iMarginSpace
		self.Y_LEADER_ICON = self.Y_HEADER_PANEL + self.iMarginSpace
		self.H_LEADER_ICON = self.H_HEADER_PANEL - (15 * 2)#140
		self.W_LEADER_ICON = int(self.H_LEADER_ICON / 1.272727)#110
		
		
		self.WH_FANCY_ICON = 64
		self.X_FANCY_ICON1 = self.X_LEADER_ICON + self.W_LEADER_ICON + self.iMarginSpace
		self.X_FANCY_ICON2 = self.X_LEADER_ICON + (self.W_HEADER_PANEL - (self.iMarginSpace * 2) - self.WH_FANCY_ICON) # Was 430
		self.Y_FANCY_ICON = (self.Y_HEADER_PANEL + self.iMarginSpace + 6) - 6
		
		self.X_LEADER_TITLE_TEXT = (self.X_FANCY_ICON1+self.WH_FANCY_ICON)+((self.X_FANCY_ICON2 - (self.X_FANCY_ICON1+self.WH_FANCY_ICON))/2) - ((self.W_HEADER_PANEL / 3)/2)
		
		self.Y_LEADER_TITLE_TEXT = self.Y_HEADER_PANEL + self.iMarginSpace + 6
		self.W_LEADER_TITLE_TEXT = self.W_HEADER_PANEL / 3
		self.H_LEADER_TITLE_TEXT = self.H_HEADER_PANEL / 2
		
		self.X_STATS_TEXT = self.X_FANCY_ICON1# + self.W_LEADER_ICON + (self.iMarginSpace * 2) + 5
		
		self.Y_STATS_TEXT = self.Y_LEADER_TITLE_TEXT + 60
		self.W_STATS_TEXT = int(self.W_HEADER_PANEL * (5 / 7.0)) + (self.iMarginSpace * 2)
		self.H_STATS_TEXT = int(self.H_HEADER_PANEL * (3 / 5.0)) - (self.iMarginSpace * 2)
		
		self.X_TEXT_PANEL = self.X_HEADER_PANEL
		self.Y_TEXT_PANEL = self.Y_HEADER_PANEL + self.H_HEADER_PANEL + self.iMarginSpace - 10 #10 is the fudge factor
		self.W_TEXT_PANEL = self.W_HEADER_PANEL
		self.H_TEXT_PANEL = self.H_MAIN_PANEL - self.H_HEADER_PANEL - (self.iMarginSpace * 3) + 10 #10 is the fudge factor
		self.iTEXT_PANEL_MARGIN = 35
		
		self.W_EXIT = 120
		self.H_EXIT = 30
		
		self.X_EXIT = (self.W_SCREEN/2) - (self.W_EXIT/2) # Was 460
		self.Y_EXIT = self.Y_TEXT_PANEL + self.H_TEXT_PANEL - (self.iMarginSpace * 3)
