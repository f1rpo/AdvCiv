## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
# Author - Jon Shafer
# Top Civilizations screen

import PyHelpers
import CvUtil
import CvScreenEnums
import random
from CvPythonExtensions import *

PyPlayer = PyHelpers.PyPlayer
gc = CyGlobalContext()
localText = CyTranslator()

NUM_CIVILIZATIONS = 8

class CvTopCivs:
	"The Greatest Civilizations screen"
	
	def __init__(self):
		
		self.X_SCREEN = 0#205
		self.Y_SCREEN = 0#27
		self.W_SCREEN = 1024#426
		self.H_SCREEN = 768#470

		self.X_MAIN_PANEL = 250
		self.Y_MAIN_PANEL = 70
		self.W_MAIN_PANEL = 550
		self.H_MAIN_PANEL = 500
		
		self.iMarginSpace = 15
		
		self.X_HEADER_PANEL = self.X_MAIN_PANEL + self.iMarginSpace
		self.Y_HEADER_PANEL = self.Y_MAIN_PANEL + self.iMarginSpace
		self.W_HEADER_PANEL = self.W_MAIN_PANEL - (self.iMarginSpace * 2)
		self.H_HEADER_PANEL = self.H_MAIN_PANEL - (self.iMarginSpace * 2)
		
#		iWHeaderPanelRemainingAfterLeader = self.W_HEADER_PANEL - self.W_LEADER_ICON + (self.iMarginSpace * 3)
#		iXHeaderPanelRemainingAfterLeader = self.X_LEADER_ICON + self.W_LEADER_ICON + self.iMarginSpace
		self.X_LEADER_TITLE_TEXT = 500#iXHeaderPanelRemainingAfterLeader + (iWHeaderPanelRemainingAfterLeader / 2)
		self.Y_LEADER_TITLE_TEXT = self.Y_HEADER_PANEL + self.iMarginSpace
		self.W_LEADER_TITLE_TEXT = self.W_HEADER_PANEL / 3
		self.H_LEADER_TITLE_TEXT = self.H_HEADER_PANEL / 3
		
		self.X_TEXT_PANEL = self.X_HEADER_PANEL + self.iMarginSpace
		self.Y_TEXT_PANEL = self.Y_HEADER_PANEL + 132
		self.W_TEXT_PANEL = self.W_HEADER_PANEL - (self.iMarginSpace * 2)
		self.H_TEXT_PANEL = 265#self.H_MAIN_PANEL - self.H_HEADER_PANEL - (self.iMarginSpace * 3) + 10 #10 is the fudge factor
		self.iTEXT_PANEL_MARGIN = 35
		
		self.X_RANK_TEXT = 430
		self.Y_RANK_TEXT = 230
		self.W_RANK_TEXT = 300
		self.H_RANK_TEXT = 30
		
		self.X_EXIT = 460
		self.Y_EXIT = self.Y_MAIN_PANEL + 440
		self.W_EXIT = 120
		self.H_EXIT = 30

	def showScreen(self):
			  
		'Use a popup to display the opening text'
		if ( CyGame().isPitbossHost() ):
			return

		# Text
		self.TITLE_TEXT = u"<font=3>" + localText.getText("TXT_KEY_TOPCIVS_TITLE", ()).upper() + u"</font>"
		self.EXIT_TEXT = localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT", ()).upper()
		# advc.038: Moved up
		self.TypeList =    [	localText.getText("TXT_KEY_TOPCIVS_WEALTH", ()),
					localText.getText("TXT_KEY_TOPCIVS_POWER", ()),
					localText.getText("TXT_KEY_TOPCIVS_TECH", ()),
					localText.getText("TXT_KEY_TOPCIVS_CULTURE", ()),
					localText.getText("TXT_KEY_TOPCIVS_SIZE", ()),
				    ]
		# <advc.038>
		randIndex = gc.getGame().getSorenRandNum(len(self.TypeList), "advc.038")
		if(randIndex == 0): # Wealth: Herodotus, Thucydides
			self.HistorianList = [	localText.getText("TXT_KEY_TOPCIVS_HISTORIAN1", ()), localText.getText("TXT_KEY_TOPCIVS_HISTORIAN2", ()) ]
		elif(randIndex == 1): # Power: Tacitus, Machiavelli
			self.HistorianList = [	localText.getText("TXT_KEY_TOPCIVS_HISTORIAN9", ()), localText.getText("TXT_KEY_TOPCIVS_HISTORIAN11", ()) ]
		elif(randIndex == 2): # Tech: Gibbon, Pliny
			self.HistorianList = [	localText.getText("TXT_KEY_TOPCIVS_HISTORIAN3", ()), localText.getText("TXT_KEY_TOPCIVS_HISTORIAN5", ()) ]
		elif(randIndex == 3): # Culture: Bede, McCauley
			self.HistorianList = [	localText.getText("TXT_KEY_TOPCIVS_HISTORIAN7", ()), localText.getText("TXT_KEY_TOPCIVS_HISTORIAN10", ()) ]
		else: # Size: Livy, Toynbee
			self.HistorianList = [	localText.getText("TXT_KEY_TOPCIVS_HISTORIAN6", ()), localText.getText("TXT_KEY_TOPCIVS_HISTORIAN8", ()) ]
		# Unassigned: St. Augustine - is he really considered a historian? He's also in the game as a Great Prophet.
		# </advc.038>
		#self.HistorianList = [	localText.getText("TXT_KEY_TOPCIVS_HISTORIAN1", ()),
					#localText.getText("TXT_KEY_TOPCIVS_HISTORIAN2", ()),
					#localText.getText("TXT_KEY_TOPCIVS_HISTORIAN3", ()),
					#localText.getText("TXT_KEY_TOPCIVS_HISTORIAN4", ()),
					#localText.getText("TXT_KEY_TOPCIVS_HISTORIAN5", ()),
					#localText.getText("TXT_KEY_TOPCIVS_HISTORIAN6", ()),
					#localText.getText("TXT_KEY_TOPCIVS_HISTORIAN7", ()),
					#localText.getText("TXT_KEY_TOPCIVS_HISTORIAN8", ()),
					#localText.getText("TXT_KEY_TOPCIVS_HISTORIAN9", ()),
					#localText.getText("TXT_KEY_TOPCIVS_HISTORIAN10", ()),
					#localText.getText("TXT_KEY_TOPCIVS_HISTORIAN11", ())
				    #]
				
		self.RankList =     [	localText.getText("TXT_KEY_TOPCIVS_RANK1", ()),
					localText.getText("TXT_KEY_TOPCIVS_RANK2", ()),
					localText.getText("TXT_KEY_TOPCIVS_RANK3", ()),
					localText.getText("TXT_KEY_TOPCIVS_RANK4", ()),
					localText.getText("TXT_KEY_TOPCIVS_RANK5", ()),
					localText.getText("TXT_KEY_TOPCIVS_RANK6", ()),
					localText.getText("TXT_KEY_TOPCIVS_RANK7", ()),
					localText.getText("TXT_KEY_TOPCIVS_RANK8", ())
				    ]

		#self.TypeList =    [	localText.getText("TXT_KEY_TOPCIVS_WEALTH", ()),
					#localText.getText("TXT_KEY_TOPCIVS_POWER", ()),
					#localText.getText("TXT_KEY_TOPCIVS_TECH", ()),
					#localText.getText("TXT_KEY_TOPCIVS_CULTURE", ()),
					#localText.getText("TXT_KEY_TOPCIVS_SIZE", ()),
				    #]

		# Randomly choose what category and what historian will be used
		#szTypeRand = random.choice(self.TypeList)
		#szHistorianRand = random.choice(self.HistorianList)
		# <advc.038> Replacing the two lines above
		szTypeRand = self.TypeList[randIndex]
		szHistorianRand = self.HistorianList[gc.getGame().getSorenRandNum(len(self.HistorianList), "advc.038")]
		# </advc.038>
		# Create screen
		
		self.screen = CyGInterfaceScreen( "CvTopCivs", CvScreenEnums.TOP_CIVS )

		self.screen.setSound("AS2D_TOP_CIVS")
		self.screen.showScreen(PopupStates.POPUPSTATE_QUEUED, False)
		self.screen.showWindowBackground( False )
		self.screen.setDimensions(self.screen.centerX(self.X_SCREEN), self.screen.centerY(self.Y_SCREEN), self.W_SCREEN, self.H_SCREEN)
		
		# Create panels
		
		# Main
		szMainPanel = "TopCivsMainPanel"
		self.screen.addPanel( szMainPanel, "", "", true, true,
			self.X_MAIN_PANEL, self.Y_MAIN_PANEL, self.W_MAIN_PANEL, self.H_MAIN_PANEL, PanelStyles.PANEL_STYLE_MAIN )
		
		# Top
		szHeaderPanel = "TopCivsHeaderPanel"
		szHeaderText = ""#gc.getLeaderHeadInfo(self.player.getLeaderType()).getDescription() + "\n-" + self.player.getCivilizationDescription(0) + "-"
		self.screen.addPanel( szHeaderPanel, szHeaderText, "", true, true,
			self.X_HEADER_PANEL, self.Y_HEADER_PANEL, self.W_HEADER_PANEL, self.H_HEADER_PANEL, PanelStyles.PANEL_STYLE_DAWNBOTTOM )
		
		# Bottom
		szTextPanel = "TopCivsTextPanel"
		szHeaderText = ""#self.Text_Title
		self.screen.addPanel( szTextPanel, szHeaderText, "", true, true,
			self.X_TEXT_PANEL, self.Y_TEXT_PANEL, self.W_TEXT_PANEL, self.H_TEXT_PANEL, PanelStyles.PANEL_STYLE_DAWNTOP )
		
		self.screen.setButtonGFC("Exit", self.EXIT_TEXT, "", self.X_EXIT,self.Y_EXIT, self.W_EXIT, self.H_EXIT, WidgetTypes.WIDGET_CLOSE_SCREEN, -1, -1, ButtonStyles.BUTTON_STYLE_STANDARD )
		
		# Title Text
		self.X_TITLE_TEXT = self.X_HEADER_PANEL + (self.W_HEADER_PANEL / 2)
		self.Y_TITLE_TEXT = self.Y_HEADER_PANEL + 15
		self.screen.setLabel("DawnTitle", "Background", self.TITLE_TEXT, CvUtil.FONT_CENTER_JUSTIFY,
				self.X_TITLE_TEXT, self.Y_TITLE_TEXT, -2.0, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		
		# 1 Text
		self.X_INFO_TEXT = self.X_TITLE_TEXT - 260#self.X_HEADER_PANEL + (self.W_HEADER_PANEL / 2)
		self.Y_INFO_TEXT = self.Y_TITLE_TEXT + 50
		self.W_INFO_TEXT = self.W_HEADER_PANEL
		self.H_INFO_TEXT = 70
		szText = localText.getText("TXT_KEY_TOPCIVS_TEXT1", (szHistorianRand, )) + u"\n" + localText.getText("TXT_KEY_TOPCIVS_TEXT2", (szTypeRand, ))
		self.screen.addMultilineText( "InfoText1", szText, self.X_INFO_TEXT, self.Y_INFO_TEXT, self.W_INFO_TEXT, self.H_INFO_TEXT, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
		
		self.makeList(szTypeRand)
		
	def makeList(self, szType):

		# Determine the list of top civs
		
		# Will eventually Store [iValue, iPlayerID]
		self.aiTopCivsValues = []
		
		# Loop through all players except the barbs
		for iPlayerLoop in range(gc.getMAX_PLAYERS()-1):
				
			if (gc.getPlayer(iPlayerLoop).isAlive()):
				
				if (szType == localText.getText("TXT_KEY_TOPCIVS_WEALTH", ())):
					# <advc.038> Replacing the two lines below
					gnp = gc.getPlayer(iPlayerLoop).estimateYieldRate(YieldTypes.YIELD_COMMERCE)
					self.aiTopCivsValues.append([gnp, iPlayerLoop])
					# </advc.038>
					#self.aiTopCivsValues.append([gc.getPlayer(iPlayerLoop).getGold(), iPlayerLoop])
					#print("Player %d Num Gold: %d" %(iPlayerLoop, gc.getPlayer(iPlayerLoop).getGold()))
					
				if (szType == localText.getText("TXT_KEY_TOPCIVS_POWER", ())):

					self.aiTopCivsValues.append([gc.getPlayer(iPlayerLoop).getPower(), iPlayerLoop])

				if (szType == localText.getText("TXT_KEY_TOPCIVS_TECH", ())):

					iPlayerNumTechs = 0
					iPlayerTechCosts = 0 # advc.038
					iNumTotalTechs = gc.getNumTechInfos()

					for iTechLoop in range(iNumTotalTechs):

						bPlayerHasTech = gc.getTeam(gc.getPlayer(iPlayerLoop).getTeam()).isHasTech(iTechLoop)

						if (bPlayerHasTech):
							iPlayerNumTechs = iPlayerNumTechs + 1
							# advc.038:
							iPlayerTechCosts += gc.getTechInfo(iTechLoop).getResearchCost()
					#self.aiTopCivsValues.append([iPlayerNumTechs, iPlayerLoop])
					#advc.038: Replacing the above
					self.aiTopCivsValues.append([iPlayerTechCosts, iPlayerLoop])
				if (szType == localText.getText("TXT_KEY_TOPCIVS_CULTURE", ())):

					self.aiTopCivsValues.append([gc.getPlayer(iPlayerLoop).countTotalCulture(), iPlayerLoop])

				if (szType == localText.getText("TXT_KEY_TOPCIVS_SIZE", ())):

					self.aiTopCivsValues.append([gc.getPlayer(iPlayerLoop).getTotalLand(), iPlayerLoop])

		# Lowest to Highest
		self.aiTopCivsValues.sort()
		# Switch it around - want the best to be first
		self.aiTopCivsValues.reverse()

		self.printList(szType)
		
	def printList(self, szType):
		
		# Print out the list
		# <advc.038> Confusing when all civs but 1 are ranked
		iRanks = 8
		if gc.getGame().countCivPlayersAlive() == iRanks + 1:
			iRanks = iRanks - 1
		for iRankLoop in range(iRanks): # </advc.038>
			
			if (iRankLoop > len(self.aiTopCivsValues)-1):
				return
			
			iPlayer = self.aiTopCivsValues[iRankLoop][1]
			iValue = self.aiTopCivsValues[iRankLoop][0]
			
			szPlayerName = gc.getPlayer(iPlayer).getNameKey()
			
			if (szPlayerName != ""):
				
				pActivePlayerTeam = gc.getTeam(gc.getPlayer(CyGame().getActivePlayer()).getTeam())
				iPlayerTeam = gc.getPlayer(iPlayer).getTeam()
				szCivText = ""
				
				# Does the Active player know this player exists?
				if (iPlayer == CyGame().getActivePlayer() or pActivePlayerTeam.isHasMet(iPlayerTeam)):
					szCivText = localText.getText("TXT_KEY_TOPCIVS_TEXT3", (szPlayerName, self.RankList[iRankLoop]))
					
				else:
					szCivText = localText.getText("TXT_KEY_TOPCIVS_UNKNOWN", ())
					
				szWidgetName = "Text" + str(iRankLoop)
				szWidgetDesc = "%d) %s" % (iRankLoop + 1, szCivText)
				iXLoc = self.X_RANK_TEXT
				iYLoc = self.Y_RANK_TEXT + (iRankLoop * self.H_RANK_TEXT)
				#self.screen.setText(szWidgetName, "Background", szWidgetDesc, CvUtil.FONT_LEFT_JUSTIFY, iXLoc, iYLoc, TEXT_Z, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				self.screen.addMultilineText( szWidgetName, unicode(szWidgetDesc), iXLoc, iYLoc, self.W_RANK_TEXT, self.H_RANK_TEXT, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				
	def turnChecker(self, iTurnNum):

		# Check to see if this is a turn when the screen should pop up
		if (not CyGame().isNetworkMultiPlayer() and CyGame().getActivePlayer()>=0):
			# <advc.038>
			interval = gc.getDefineINT("TOP_CIVS_INTERVAL") # 50 in BtS
			minTurn = 75 # 2 in BtS
			speedAdjustPercent = gc.getGameSpeedInfo(gc.getGame().getGameSpeedType()).getGrowthPercent()
			interval = (interval * speedAdjustPercent) / 100
			minTurn = (minTurn * speedAdjustPercent) / 100
			otherTeamsMet = gc.getTeam(gc.getPlayer(CyGame().getActivePlayer()).getTeam()).getHasMetCivCount(true)
			otherTeamsTotal = gc.getGame().countCivTeamsAlive() - 1
			# Subtract 1 from turn num b/c the popup is triggered one turn before it appears. Nicer to have it show up on turn 50 than on turn 51
			if ((iTurnNum-1) % interval == 0 and iTurnNum+1 >= minTurn and gc.getPlayer(CyGame().getActivePlayer()).isAlive() and otherTeamsTotal > 0 and otherTeamsMet <= otherTeamsTotal / 2):
				# </advc.038>
				self.showScreen()

	#####################################################################################################################################
	      
	def handleInput( self, inputClass ):
		self.screen = CyGInterfaceScreen( "CvTopCivs", CvScreenEnums.TOP_CIVS )		

		if ( inputClass.getFunctionName() == "Exit" and inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED ):
			self.screen.hideScreen()
			return 1
		elif ( inputClass.getData() == int(InputTypes.KB_RETURN) ):
			self.screen.hideScreen()
			return 1
		return 0

	def update(self, fDelta):
		return
