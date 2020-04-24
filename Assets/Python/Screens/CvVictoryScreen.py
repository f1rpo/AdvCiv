## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
from CvPythonExtensions import *
import CvUtil
import ScreenInput
import PyHelpers
import time

# BUG - start
import AttitudeUtil
import BugCore
import BugPath
import BugUtil
import ColorUtil
import GameUtil
import PlayerUtil
import TechUtil

AdvisorOpt = BugCore.game.Advisors
# BUG - end

# BUG - Mac Support - start
BugUtil.fixSets(globals())
# BUG - Mac Support - end

PyPlayer = PyHelpers.PyPlayer

# globals
gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

VICTORY_CONDITION_SCREEN = 0
GAME_SETTINGS_SCREEN = 1
UN_RESOLUTION_SCREEN = 2
UN_MEMBERS_SCREEN = 3
# advc.703
RF_SCORE_SCREEN = 4

class CvVictoryScreen:
	"Keeps track of victory conditions"

	def __init__(self, screenId):
		self.screenId = screenId
		self.SCREEN_NAME = "VictoryScreen"
		self.DEBUG_DROPDOWN_ID =  "VictoryScreenDropdownWidget"
		self.INTERFACE_ART_INFO = "TECH_BG"
		self.EXIT_AREA = "EXIT"
		self.EXIT_ID = "VictoryScreenExit"
		self.BACKGROUND_ID = "VictoryScreenBackground"
		self.HEADER_ID = "VictoryScreenHeader"
		self.WIDGET_ID = "VictoryScreenWidget"
		self.VC_TAB_ID = "VictoryTabWidget"
		self.SETTINGS_TAB_ID = "SettingsTabWidget"
		self.UN_RESOLUTION_TAB_ID = "VotingTabWidget"
		self.UN_MEMBERS_TAB_ID = "MembersTabWidget"
		self.SPACESHIP_SCREEN_BUTTON = 1234
		
		# <advc.703>
		self.RF_SCORE_TAB_ID = "RiseFallTabWidget"
		# Adopted from CvReligionScreen:
		self.AREA1_ID =  "RiseFallAreaWidget1"
		self.AREA2_ID =  "RiseFallAreaWidget2"
		self.X_RF1_AREA = 45
		self.X_RF2_AREA = 522
		self.W_RF_AREA = 457
		# (These two can get adjusted based on the chapter count)
		self.Y_RF_AREA = 350
		self.H_RF_AREA = 340
		# </advc.703>

		self.Z_BACKGROUND = -6.1
		self.Z_CONTROLS = self.Z_BACKGROUND - 0.2
		self.DZ = -0.2

		self.X_SCREEN = 500
		self.Y_SCREEN = 396
		self.W_SCREEN = 1024
		self.H_SCREEN = 768
		self.Y_TITLE = 12

		self.X_EXIT = 994
		self.Y_EXIT = 726

		self.X_AREA = 10
		self.Y_AREA = 60
		self.W_AREA = 1010
		self.H_AREA = 650

		self.TABLE_WIDTH_0 = 350
		self.TABLE_WIDTH_1 = 80
		self.TABLE_WIDTH_2 = 180
		self.TABLE_WIDTH_3 = 100
		self.TABLE_WIDTH_4 = 180
		self.TABLE_WIDTH_5 = 100

		self.TABLE2_WIDTH_0 = 740
		self.TABLE2_WIDTH_1 = 265
		# <advc.703>
		self.RF_TABLEW_0 = 125
		self.RF_TABLEW_1 = 150
		self.RF_TABLEW_2 = 250
		self.RF_TABLEW_3 = 260
		self.RF_TABLEW_4 = 205
		# </advc.703>
# BUG Additions Start
		self.TABLE3_WIDTH_0 = 450
		self.TABLE3_WIDTH_1 = 90
		self.TABLE3_WIDTH_2 = 90
		self.TABLE3_WIDTH_3 = 90
		self.TABLE3_WIDTH_4 = 90
		self.TABLE3_WIDTH_5 = 200

		self.Vote_Pope_ID = "BUGVotePope_Widget"
		self.Vote_DipVic_ID = "BUGVoteDiplomaticVictory_Widget"
		self.Vote_X = 20
		self.Vote_Y = 688
		self.VoteType = 1  # 1 = Pope or GenSec, 2 = Diplomatic Victory
		self.VoteBody = 1  # 1 = AP, 2 = UN

		self.Vote_AP_ID = "BUGVoteAP_Widget"
		self.Vote_UN_ID = "BUGVoteUN_Widget"
# BUG Additions End

		self.X_LINK = 100
		self.DX_LINK = 220
		self.Y_LINK = 726
		self.MARGIN = 20
		
		self.SETTINGS_PANEL_X1 = 50
		self.SETTINGS_PANEL_X2 = 355
		self.SETTINGS_PANEL_X3 = 660
		self.SETTINGS_PANEL_Y = 150
		self.SETTINGS_PANEL_WIDTH = 300
		self.SETTINGS_PANEL_HEIGHT = 500

		self.nWidgetCount = 0
		self.iActivePlayer = -1
		self.bVoteTab = False

		self.iScreen = VICTORY_CONDITION_SCREEN

		self.ApolloTeamsChecked = set()
		self.ApolloTeamCheckResult = {}

	def getScreen(self):
		return CyGInterfaceScreen(self.SCREEN_NAME, self.screenId)

	def hideScreen(self):
		screen = self.getScreen()
		screen.hideScreen()

	def interfaceScreen(self):

		# Create a new screen
		screen = self.getScreen()
		if screen.isActive():
			return
		screen.setRenderInterfaceOnly(True);
		screen.showScreen(PopupStates.POPUPSTATE_IMMEDIATE, False)

		self.iActivePlayer = CyGame().getActivePlayer()
		if self.iScreen == -1:
			self.iScreen = VICTORY_CONDITION_SCREEN

		# Set the background widget and exit button
		screen.addDDSGFC(self.BACKGROUND_ID, ArtFileMgr.getInterfaceArtInfo("MAINMENU_SLIDESHOW_LOAD").getPath(), 0, 0, self.W_SCREEN, self.H_SCREEN, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		screen.addPanel( "TechTopPanel", u"", u"", True, False, 0, 0, self.W_SCREEN, 55, PanelStyles.PANEL_STYLE_TOPBAR )
		screen.addPanel( "TechBottomPanel", u"", u"", True, False, 0, 713, self.W_SCREEN, 55, PanelStyles.PANEL_STYLE_BOTTOMBAR )
		screen.showWindowBackground( False )
		screen.setDimensions(screen.centerX(0), screen.centerY(0), self.W_SCREEN, self.H_SCREEN)
		screen.setText(self.EXIT_ID, "Background", u"<font=4>" + localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT", ()).upper() + "</font>", CvUtil.FONT_RIGHT_JUSTIFY, self.X_EXIT, self.Y_EXIT, self.Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_CLOSE_SCREEN, -1, -1 )

		# Header...
		screen.setLabel(self.HEADER_ID, "Background", u"<font=4b>" + localText.getText("TXT_KEY_VICTORY_SCREEN_TITLE", ()).upper() + u"</font>", CvUtil.FONT_CENTER_JUSTIFY, self.X_SCREEN, self.Y_TITLE, self.Z_CONTROLS, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		if self.iScreen == VICTORY_CONDITION_SCREEN:
			self.showVictoryConditionScreen()
		elif self.iScreen == GAME_SETTINGS_SCREEN:
			self.showGameSettingsScreen()
		elif self.iScreen == UN_RESOLUTION_SCREEN:
			self.showVotingScreen()
		elif self.iScreen == UN_MEMBERS_SCREEN:
			self.showMembersScreen()
		# <advc.703>
		elif self.iScreen == RF_SCORE_SCREEN:
			self.showRiseFall()
		# </advc.703>

	def drawTabs(self):
		screen = self.getScreen()

		xLink = self.X_LINK
		if (self.iScreen != VICTORY_CONDITION_SCREEN):
			screen.setText(self.VC_TAB_ID, "", u"<font=4>" + localText.getText("TXT_KEY_MAIN_MENU_VICTORIES", ()).upper() + "</font>", CvUtil.FONT_CENTER_JUSTIFY, xLink, self.Y_LINK, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		else:
			screen.setText(self.VC_TAB_ID, "", u"<font=4>" + localText.getColorText("TXT_KEY_MAIN_MENU_VICTORIES", (), gc.getInfoTypeForString("COLOR_YELLOW")).upper() + "</font>", CvUtil.FONT_CENTER_JUSTIFY, xLink, self.Y_LINK, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		xLink += self.DX_LINK

		if (self.iScreen != GAME_SETTINGS_SCREEN):
			screen.setText(self.SETTINGS_TAB_ID, "", u"<font=4>" + localText.getText("TXT_KEY_MAIN_MENU_SETTINGS", ()).upper() + "</font>", CvUtil.FONT_CENTER_JUSTIFY, xLink, self.Y_LINK, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		else:
			screen.setText(self.SETTINGS_TAB_ID, "", u"<font=4>" + localText.getColorText("TXT_KEY_MAIN_MENU_SETTINGS", (), gc.getInfoTypeForString("COLOR_YELLOW")).upper() + "</font>", CvUtil.FONT_CENTER_JUSTIFY, xLink, self.Y_LINK, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		xLink += self.DX_LINK

		if self.bVoteTab:
			if (self.iScreen != UN_RESOLUTION_SCREEN):
				screen.setText(self.UN_RESOLUTION_TAB_ID, "", u"<font=4>" + localText.getText("TXT_KEY_VOTING_TITLE", ()).upper() + "</font>", CvUtil.FONT_CENTER_JUSTIFY, xLink, self.Y_LINK, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			else:
				screen.setText(self.UN_RESOLUTION_TAB_ID, "", u"<font=4>" + localText.getColorText("TXT_KEY_VOTING_TITLE", (), gc.getInfoTypeForString("COLOR_YELLOW")).upper() + "</font>", CvUtil.FONT_CENTER_JUSTIFY, xLink, self.Y_LINK, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			xLink += self.DX_LINK
			# advc.703: Merge Members into Resolutions when R&F enabled
			if not gc.getGame().isOption(GameOptionTypes.GAMEOPTION_RISE_FALL):
				if (self.iScreen != UN_MEMBERS_SCREEN):
					screen.setText(self.UN_MEMBERS_TAB_ID, "", u"<font=4>" + localText.getText("TXT_KEY_MEMBERS_TITLE", ()).upper() + "</font>", CvUtil.FONT_CENTER_JUSTIFY, xLink, self.Y_LINK, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				else:
					screen.setText(self.UN_MEMBERS_TAB_ID, "", u"<font=4>" + localText.getColorText("TXT_KEY_MEMBERS_TITLE", (), gc.getInfoTypeForString("COLOR_YELLOW")).upper() + "</font>", CvUtil.FONT_CENTER_JUSTIFY, xLink, self.Y_LINK, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				xLink += self.DX_LINK
		# <advc.703>
		if gc.getGame().isOption(GameOptionTypes.GAMEOPTION_RISE_FALL):
			if self.iScreen != RF_SCORE_SCREEN:
				screen.setText(self.RF_SCORE_TAB_ID, "", u"<font=4>" + localText.getText("TXT_KEY_GAME_SCORE", ()).upper() + "</font>", CvUtil.FONT_CENTER_JUSTIFY, xLink, self.Y_LINK, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			else:
				screen.setText(self.RF_SCORE_TAB_ID, "", u"<font=4>" + localText.getColorText("TXT_KEY_GAME_SCORE", (), gc.getInfoTypeForString("COLOR_YELLOW")).upper() + "</font>", CvUtil.FONT_CENTER_JUSTIFY, xLink, self.Y_LINK, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
			xLink += self.DX_LINK
		# </advc.703>

	def showVotingScreen(self):
		self.deleteAllWidgets()

		activePlayer = gc.getPlayer(self.iActivePlayer)
		iActiveTeam = activePlayer.getTeam()

		aiVoteBuildingClass = []
		for i in range(gc.getNumBuildingInfos()):
			for j in range(gc.getNumVoteSourceInfos()):
				if (gc.getBuildingInfo(i).getVoteSourceType() == j):
					iUNTeam = -1
					bUnknown = true
					for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
						if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
							if (gc.getTeam(iLoopTeam).getBuildingClassCount(gc.getBuildingInfo(i).getBuildingClassType()) > 0):
								iUNTeam = iLoopTeam
								if (iLoopTeam == iActiveTeam or gc.getGame().isDebugMode() or gc.getTeam(activePlayer.getTeam()).isHasMet(iLoopTeam)):
									bUnknown = false
								break

					#aiVoteBuildingClass.append((gc.getBuildingInfo(i).getBuildingClassType(), iUNTeam, bUnknown))
					aiVoteBuildingClass.append((j, gc.getBuildingInfo(i).getBuildingClassType(), iUNTeam, bUnknown)) # K-Mod

		if (len(aiVoteBuildingClass) == 0):
			return

		screen = self.getScreen()

		screen.addPanel(self.getNextWidgetName(), "", "", False, False, self.X_AREA-10, self.Y_AREA-15, self.W_AREA+20, self.H_AREA+30, PanelStyles.PANEL_STYLE_BLUE50)
		szTable = self.getNextWidgetName()
		screen.addTableControlGFC(szTable, 2, self.X_AREA, self.Y_AREA, self.W_AREA, self.H_AREA, False, False, 32,32, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSelect(szTable, False)		
		screen.setTableColumnHeader(szTable, 0, "", self.TABLE2_WIDTH_0)
		screen.setTableColumnHeader(szTable, 1, "", self.TABLE2_WIDTH_1)

		# for (iVoteBuildingClass, iUNTeam, bUnknown) in aiVoteBuildingClass:
			# iRow = screen.appendTableRow(szTable)
			# screen.setTableText(szTable, 0, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_ELECTION", (gc.getBuildingClassInfo(iVoteBuildingClass).getTextKey(), )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# if (iUNTeam != -1):
				# if bUnknown:
					# szName = localText.getText("TXT_KEY_TOPCIVS_UNKNOWN", ())
				# else:
					# szName = gc.getTeam(iUNTeam).getName()
				# screen.setTableText(szTable, 1, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_BUILT", (szName, )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# else:
				# screen.setTableText(szTable, 1, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_NOT_BUILT", ()), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		#for i in range(gc.getNumVoteSourceInfos()):
		for (i, iVoteBuildingClass, iUNTeam, bUnknown) in aiVoteBuildingClass: # K-Mod
			if gc.getGame().isDiploVote(i): # K-Mod (previously no condition)
				# K-Mod
				kVoteSource = gc.getVoteSourceInfo(i)
				iRow = screen.appendTableRow(szTable)
				screen.setTableText(szTable, 0, iRow, u"<font=4b>" + kVoteSource.getDescription().upper() + u"</font>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				if (gc.getGame().getVoteSourceReligion(i) != -1):
					screen.setTableText(szTable, 1, iRow, gc.getReligionInfo(gc.getGame().getVoteSourceReligion(i)).getDescription(), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				# K-Mod end

				#if (gc.getGame().canHaveSecretaryGeneral(i) and -1 != gc.getGame().getSecretaryGeneral(i)):
				if (gc.getGame().canHaveSecretaryGeneral(i) and -1 != gc.getGame().getSecretaryGeneral(i) and (gc.getGame().isDebugMode() or gc.getTeam(activePlayer.getTeam()).isHasMet(gc.getGame().getSecretaryGeneral(i)))):# K-Mod
					iRow = screen.appendTableRow(szTable)
					screen.setTableText(szTable, 0, iRow, gc.getVoteSourceInfo(i).getSecretaryGeneralText(), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					screen.setTableText(szTable, 1, iRow, gc.getTeam(gc.getGame().getSecretaryGeneral(i)).getName(), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

				# K-Mod
				# vote timing
				if activePlayer.isVotingMember(i):
					iRow = screen.appendTableRow(szTable)
					iVoteTimer = gc.getGame().getVoteTimer(i)+1 # the +1 is so that when there will be a vote next turn, this reads "1" rather than "0".
					sString = localText.getText("TXT_KEY_BUG_VICTORY_TURNS_NEXT_VOTE", (iVoteTimer,) )
					#sString = u"<font=2b>" + sString + "</font>"
					sString = BugUtil.colorText(sString, "COLOR_YELLOW")
					screen.setTableText(szTable, 0, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					iSecGenTimer = gc.getGame().getSecretaryGeneralTimer(i)
					sString = localText.getText("TXT_KEY_BUG_VICTORY_VOTES_NEXT_ELECTION", (iSecGenTimer,) )
					#sString = u"<font=2b>" + sString + "</font>"
					sString = BugUtil.colorText(sString, "COLOR_YELLOW")
					screen.setTableText(szTable, 1, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				# K-Mod end

				for iLoop in range(gc.getNumVoteInfos()):
					if gc.getGame().countPossibleVote(iLoop, i) > 0:
						info = gc.getVoteInfo(iLoop)
						# advc.178: Clauses for diplo victory added
						if gc.getGame().isChooseElection(iLoop) and (gc.getGame().isDiploVictoryValid() or not info.isVictory()):
							iRow = screen.appendTableRow(szTable)
							screen.setTableText(szTable, 0, iRow, info.getDescription(), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
							if gc.getGame().isVotePassed(iLoop):
								screen.setTableText(szTable, 1, iRow, localText.getText("TXT_KEY_POPUP_PASSED", ()), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
							else:
								screen.setTableText(szTable, 1, iRow, localText.getText("TXT_KEY_POPUP_ELECTION_OPTION", (u"", gc.getGame().getVoteRequired(iLoop, i), gc.getGame().countPossibleVote(iLoop, i))), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				iRow = screen.appendTableRow(szTable) # empty row between vote sources. (K-Mod)
		# Remove the final empty row (K-Mod)
		if screen.getTableNumRows(szTable) > 0:
			screen.setTableNumRows(szTable, screen.getTableNumRows(szTable)-1)
		#
		# <advc.703> Add info from Members tab
		if not gc.getGame().isOption(GameOptionTypes.GAMEOPTION_RISE_FALL):
			self.drawTabs()
			return
		screen.appendTableRow(szTable) # Empty row as separator
		self.appendMemberRows(screen, szTable)
		# </advc.703>
		self.drawTabs()


# BUG Additions Start
	def showMembersScreen(self):
		if AdvisorOpt.isMembers():
			iRelVote, iRelVoteIdx, iUNVote, iUNVoteIdx  = self.getVoteAvailable()
			if  iRelVote == -1: self.VoteBody = 2 # AP Not active
			elif iUNVote == -1: self.VoteBody = 1 # UN Not active

			if self.VoteBody == 1:
				iVoteBody = iRelVote
				iVoteIdx = iRelVoteIdx
			else:
				iVoteBody = iUNVote
				iVoteIdx = iUNVoteIdx

			self.showMembersScreen_BUG(iRelVote, iUNVote, iVoteBody, iVoteIdx)
		else:
			self.showMembersScreen_NonBUG()

		self.drawTabs()

	def getVoteAvailable(self):

		iRelVote = -1
		iRelVoteIdx = -1
		iUNVote = -1
		iUNVoteIdx = -1

		for i in range(gc.getNumVoteSourceInfos()):
			if gc.getGame().isDiploVote(i):
				if (gc.getGame().getVoteSourceReligion(i) != -1):
					iRelVote = i
				else:
					iUNVote = i

			if (gc.getGame().canHaveSecretaryGeneral(i)
			and gc.getGame().getSecretaryGeneral(i) != -1):
				for j in range(gc.getNumVoteInfos()):
					if gc.getVoteInfo(j).isVoteSourceType(i):
						if gc.getVoteInfo(j).isSecretaryGeneral():
							if (gc.getGame().getVoteSourceReligion(i) != -1):
								iRelVoteIdx = j
							else:
								iUNVoteIdx = j

							break

		BugUtil.debug("CvVictoryScreen: Rel Vote %i, UN Vote %i, Rel Vote Idx %i, UN Vote Idx %i", iRelVote, iRelVoteIdx, iUNVote, iUNVoteIdx)

		return iRelVote, iRelVoteIdx, iUNVote, iUNVoteIdx

	def showMembersScreen_BUG(self, iRelVote, iUNVote, iActiveVote, iVoteIdx):
		self.deleteAllWidgets()

		if (iRelVote == -1 and iUNVote == -1): return  # neither AP or UN are active

		activePlayer = gc.getPlayer(self.iActivePlayer)
		iActiveTeam = activePlayer.getTeam()

		screen = self.getScreen()

		screen.addPanel(self.getNextWidgetName(), "", "", False, False, self.X_AREA-10, self.Y_AREA-15, self.W_AREA+20, self.H_AREA+30, PanelStyles.PANEL_STYLE_BLUE50)

		# set up the header row
		szHeading = self.getNextWidgetName()
		screen.addTableControlGFC(szHeading, 3, self.X_AREA, self.Y_AREA, self.W_AREA, 30, False, False, 32,32, TableStyles.TABLE_STYLE_STANDARD)
		screen.setTableColumnHeader(szHeading, 0, "", self.TABLE3_WIDTH_0)
		screen.setTableColumnHeader(szHeading, 1, "", self.TABLE3_WIDTH_1 + self.TABLE3_WIDTH_2)
		screen.setTableColumnHeader(szHeading, 2, "", self.TABLE3_WIDTH_3 + self.TABLE3_WIDTH_4)
		iRow = screen.appendTableRow(szHeading)

		# heading
		kVoteSource = gc.getVoteSourceInfo(iActiveVote)
		sTableHeader = u"<font=4b>" + kVoteSource.getDescription().upper() + u"</font>"
		if (gc.getGame().getVoteSourceReligion(iActiveVote) != -1):
			sTableHeader += " (" + gc.getReligionInfo(gc.getGame().getVoteSourceReligion(iActiveVote)).getDescription() + ")"

		screen.setTableText(szHeading, 0, iRow, sTableHeader, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		# determine the two candidates, add to header
		iCandTeam1 = -1
		iCandTeam2 = -1
		for j in range(gc.getMAX_TEAMS()):
			BugUtil.debug("CvVictoryScreen: Team %i", j)

			if (gc.getTeam(j).isAlive()
			and gc.getGame().isTeamVoteEligible(j, iActiveVote)):
				BugUtil.debug("CvVictoryScreen: Team %i, %s <- vote eligible ", j, gc.getTeam(j).getName())
				if iCandTeam1 == -1:
					iCandTeam1 = j
				else:
					iCandTeam2 = j

		# get the first player for each team
		# going to use that to calculation attitude - too hard to calc attitude for team
		iCandPlayer1 = self.getPlayerOnTeam(iCandTeam1)
		iCandPlayer2 = self.getPlayerOnTeam(iCandTeam2)

		# candidate known returns -1 if there is no candidate, 0 if not known or 1 if known
		iCand1Known, sCand1Name = self.getCandStatusName(iCandTeam1)
		iCand2Known, sCand2Name = self.getCandStatusName(iCandTeam2)

		screen.setTableText(szHeading, 1, iRow, sCand1Name, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)
		screen.setTableText(szHeading, 2, iRow, sCand2Name, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

		# set up the member table
		szTable = self.getNextWidgetName()
		screen.addTableControlGFC(szTable, 6, self.X_AREA, self.Y_AREA + 30, self.W_AREA, self.H_AREA-20-30, False, False, 32,32, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSelect(szTable, False)
		screen.setTableColumnHeader(szTable, 0, "", self.TABLE3_WIDTH_0)
		screen.setTableColumnHeader(szTable, 1, "", self.TABLE3_WIDTH_1)
		screen.setTableColumnHeader(szTable, 2, "", self.TABLE3_WIDTH_2)
		screen.setTableColumnHeader(szTable, 3, "", self.TABLE3_WIDTH_3)
		screen.setTableColumnHeader(szTable, 4, "", self.TABLE3_WIDTH_4)
		screen.setTableColumnHeader(szTable, 5, "", self.TABLE3_WIDTH_5)
		iRow = screen.appendTableRow(szTable)

		# set up the vote selection texts
		iX = self.X_EXIT
		sText = gc.getVoteSourceInfo(iActiveVote).getSecretaryGeneralText()
		if self.VoteType == 1: sText = BugUtil.colorText(sText, "COLOR_YELLOW")
		screen.setText(self.Vote_Pope_ID, "", sText, CvUtil.FONT_RIGHT_JUSTIFY, iX, self.Vote_Y, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		iX -= 10 + CyInterface().determineWidth(sText)
		sText = localText.getText("TXT_KEY_BUG_VICTORY_DIPLOMATIC", ())
		if self.VoteType == 2: sText = BugUtil.colorText(sText, "COLOR_YELLOW")
		screen.setText(self.Vote_DipVic_ID, "", sText, CvUtil.FONT_RIGHT_JUSTIFY, iX, self.Vote_Y, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		if (iRelVote != -1
		and iUNVote != -1):  # both AP and UN are active
			iX = self.Vote_X
			sText = gc.getVoteSourceInfo(iRelVote).getDescription()
			if iActiveVote == iRelVote: sText = BugUtil.colorText(sText, "COLOR_YELLOW")
			screen.setText(self.Vote_AP_ID, "", sText, CvUtil.FONT_LEFT_JUSTIFY, iX, self.Vote_Y, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

			iX += 10 + CyInterface().determineWidth(sText)
			sText = gc.getVoteSourceInfo(iUNVote).getDescription()
			if iActiveVote == iUNVote: sText = BugUtil.colorText(sText, "COLOR_YELLOW")
			screen.setText(self.Vote_UN_ID, "", sText, CvUtil.FONT_LEFT_JUSTIFY, iX, self.Vote_Y, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		else:
			screen.hide(self.Vote_AP_ID)
			screen.hide(self.Vote_UN_ID)

		# initialize the candidate votes and total vote counter
		iVoteTotal = [0] * 2
		iVoteCand = [0] * 2

		lMembers = []
		iAPUNTeam = self.getAP_UN_OwnerTeam()

		for j in range(gc.getMAX_PLAYERS()):
			pPlayer = gc.getPlayer(j)
			if (pPlayer.isAlive()
			and not pPlayer.isBarbarian()):
				iPlayer = j
				lPlayerName = pPlayer.getName()
				lPlayerVotes = 10000 - pPlayer.getVotes(iVoteIdx, iActiveVote)   # so that it sorts from most votes to least

				if (gc.getGame().canHaveSecretaryGeneral(iActiveVote)
				and iAPUNTeam == pPlayer.getTeam()
				and gc.getGame().getSecretaryGeneral(iActiveVote) == -1):
					lPlayerStatus = 0
					lPlayerLabel = gc.getVoteSourceInfo(iActiveVote).getSecretaryGeneralText()
				elif (gc.getGame().canHaveSecretaryGeneral(iActiveVote)
				and gc.getGame().getSecretaryGeneral(iActiveVote) == pPlayer.getTeam()):
					lPlayerStatus = 1
					lPlayerLabel = gc.getVoteSourceInfo(iActiveVote).getSecretaryGeneralText()
				elif (pPlayer.isFullMember(iActiveVote)):
					lPlayerStatus = 2
					lPlayerLabel = localText.getText("TXT_KEY_VOTESOURCE_FULL_MEMBER", ())
				elif (pPlayer.isVotingMember(iActiveVote)):
					lPlayerStatus = 3
					lPlayerLabel = localText.getText("TXT_KEY_VOTESOURCE_VOTING_MEMBER", ())
				else:
					lPlayerStatus = 4
					lPlayerLabel = localText.getText("TXT_KEY_VOTESOURCE_NON_VOTING_MEMBER", ())

				lMembers.append([lPlayerStatus, lPlayerVotes, iPlayer, lPlayerLabel])

		lMembers.sort()

		for lMember in lMembers:
			lMemberStatus = lMember[0]
			lMemberVotes = 10000 - lMember[1]
			iMember = lMember[2]
			lMemberLabel = lMember[3]

			# player name
			bKnown, szPlayerText = self.getPlayerStatusName(iMember)

			if (lMemberVotes > 0
			and bKnown):
				szPlayerText += localText.getText("TXT_KEY_VICTORY_SCREEN_PLAYER_VOTES", (lMemberVotes, iActiveVote), )

			iRow = screen.appendTableRow(szTable)
			screen.setTableText(szTable, 0, iRow, szPlayerText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			if iMember != self.iActivePlayer:
				# player attitude to candidate #1
				szText = AttitudeUtil.getAttitudeText (iMember, iCandPlayer1, True, True, False, False, False) # advc.152: To match the new signature of getAttitudeText
				if (szText != None
				and iCand1Known == 1
				and bKnown):
					screen.setTableText(szTable, 1, iRow, szText, "", WidgetTypes.WIDGET_LEADERHEAD, iMember, iCandPlayer1, CvUtil.FONT_CENTER_JUSTIFY)

				# player attitude to candidate #2
				szText = AttitudeUtil.getAttitudeText (iMember, iCandPlayer2, True, True, False, False, False) # advc.152: To match the new signature of getAttitudeText
				if (szText != None
				and iCand2Known == 1
				and bKnown):
					screen.setTableText(szTable, 3, iRow, szText, "", WidgetTypes.WIDGET_LEADERHEAD, iMember, iCandPlayer2, CvUtil.FONT_CENTER_JUSTIFY)

			iVote = self.getVotesForWhichCandidate(iMember, iCandTeam1, iCandTeam2, self.VoteType)
			iVote_Column = -1

			if iVote != -1:
				sVote = str(lMemberVotes)
				iVoteTotal[iVote - 1] += lMemberVotes

				# number of votes for Candidate #1
				if (iVote == 1
				and lMemberVotes > 0
				and iCand1Known == 1):
					screen.setTableText(szTable, 2, iRow, sVote, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

				# number of votes for Candidate #2
				if (iVote == 2
				and lMemberVotes > 0
				and iCand2Known == 1):
					screen.setTableText(szTable, 4, iRow, sVote, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

			# player status
			if bKnown:
				screen.setTableText(szTable, 5, iRow, lMemberLabel, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)

			# store the candidates own votes
			if (iCandTeam1 == gc.getPlayer(iMember).getTeam()
			and iCand1Known == 1):
				iVoteCand[0] = lMemberVotes
			if (iCandTeam2 == gc.getPlayer(iMember).getTeam()
			and iCand2Known == 1):
				iVoteCand[1] = lMemberVotes

		# calculate the maximum number of votes
		iMaxVotes = 0
		for iLoop in range(gc.getNumVoteInfos()):
			if gc.getGame().countPossibleVote(iLoop, iActiveVote) > 0:
				iMaxVotes = gc.getGame().countPossibleVote(iLoop, iActiveVote)
				break

		iRow = screen.appendTableRow(szTable)
		iVoteReq = self.getVoteReq(iActiveVote, self.VoteType)
		sVoteReq = "%i" % (iVoteReq)
		sString = u"<font=3b>" + localText.getText("TXT_KEY_BUG_VICTORY_VOTES_TOTAL", ()) + "</font> "
		if (iCand1Known != 0
		and iCand2Known != 0):
			sString +=  localText.getText("TXT_KEY_BUG_VICTORY_VOTES_REQUIRED", (sVoteReq,))
		screen.setTableText(szTable, 0, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		if iCand1Known == 1:
			# color code the totals
			sVoteTotal = str(iVoteTotal[0])
			iColor = self.getVoteTotalColor(iVoteReq, iVoteTotal[0], iVoteCand[0], iVoteTotal[0] > iVoteTotal[1], self.VoteType == 2)
			if iColor != -1:
				sVoteTotal = localText.changeTextColor(sVoteTotal, iColor)
			screen.setTableText(szTable, 2, iRow, sVoteTotal, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

		if iCand2Known == 1:
			sVoteTotal = str(iVoteTotal[1])
			iColor = self.getVoteTotalColor(iVoteReq, iVoteTotal[1], iVoteCand[1], iVoteTotal[1] > iVoteTotal[0], self.VoteType == 2)
			if iColor != -1:
				sVoteTotal = localText.changeTextColor(sVoteTotal, iColor)
			screen.setTableText(szTable, 4, iRow, sVoteTotal, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_CENTER_JUSTIFY)

		# add a blank row
		iRow = screen.appendTableRow(szTable)

		# SecGen vote prediction
		if iVoteTotal[0] > iVoteTotal[1]:
			iWinner = 0
			sWin = sCand1Name
		else:
			iWinner = 1
			sWin = sCand2Name
		iLoser = 1 - iWinner

		fVotePercent = 100.0 * iVoteTotal[iWinner] / iMaxVotes
		fMargin = 100.0 * (iVoteTotal[iWinner] - iVoteTotal[iLoser]) / iMaxVotes
		
		if self.VoteType == 1:
			sSecGen = gc.getVoteSourceInfo(iActiveVote).getSecretaryGeneralText()
		else:
			sSecGen = localText.getText("TXT_KEY_BUG_VICTORY_DIPLOMATIC", ())

		# display SecGen vote prediction
		if (iCand1Known != 0
		and iCand2Known != 0):
			sString = sSecGen + ":"
			iRow = screen.appendTableRow(szTable)
			screen.setTableText(szTable, 0, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			sString = "     " + localText.getText("TXT_KEY_BUG_VICTORY_BUG_POLL_RESULT", (sWin, self.formatPercent(fVotePercent), self.formatPercent(fMargin)))
			iRow = screen.appendTableRow(szTable)
			screen.setTableText(szTable, 0, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			# BUG Poll statistical error
			iRandError = 3.5 + gc.getASyncRand().get(10, "Election Results Statistical Error") / 10.0
			sString = localText.getText("TXT_KEY_BUG_VICTORY_BUG_POLL_ERROR", (self.formatPercent(iRandError), ))
			iRow = screen.appendTableRow(szTable)
			screen.setTableText(szTable, 0, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		iRow = screen.appendTableRow(szTable)

		# add info about vote timing
		iRow = screen.appendTableRow(szTable)
		iVoteTimer = gc.getGame().getVoteTimer(iActiveVote)
		sString = localText.getText("TXT_KEY_BUG_VICTORY_TURNS_NEXT_VOTE", (iVoteTimer,) )
		sString = u"<font=2>" + sString + "</font>"
		screen.setTableText(szTable, 0, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		iRow = screen.appendTableRow(szTable)
		iSecGenTimer = gc.getGame().getSecretaryGeneralTimer(iActiveVote)
		sString = localText.getText("TXT_KEY_BUG_VICTORY_VOTES_NEXT_ELECTION", (iSecGenTimer,) )
		sString = u"<font=2>" + sString + "</font>"
		screen.setTableText(szTable, 0, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	# <advc.703> Now shared by showVotes and showMembersScreen_NonBUG
	# (I don't think the BUG version can be enabled in K-Mod)
	def appendMemberRows(self, screen, szTable):
		activePlayer = gc.getPlayer(self.iActivePlayer)
		iActiveTeam = activePlayer.getTeam()
		for i in range(gc.getNumVoteSourceInfos()):
			if gc.getGame().isDiploVote(i):
				kVoteSource = gc.getVoteSourceInfo(i)
				iRow = screen.appendTableRow(szTable)
				screen.setTableText(szTable, 0, iRow, u"<font=4b>" + kVoteSource.getDescription().upper() + u"</font>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				if (gc.getGame().getVoteSourceReligion(i) != -1):
					screen.setTableText(szTable, 1, iRow, gc.getReligionInfo(gc.getGame().getVoteSourceReligion(i)).getDescription(), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

				iSecretaryGeneralVote = -1
				if (gc.getGame().canHaveSecretaryGeneral(i) and -1 != gc.getGame().getSecretaryGeneral(i)):
					for j in range(gc.getNumVoteInfos()):
						print j
						if gc.getVoteInfo(j).isVoteSourceType(i):
							print "votesource"
							if gc.getVoteInfo(j).isSecretaryGeneral():
								print "secgen"
								iSecretaryGeneralVote = j
								break

				print iSecretaryGeneralVote
				for j in range(gc.getMAX_PLAYERS()):
					if gc.getPlayer(j).isAlive() and not gc.getPlayer(j).isBarbarian() and gc.getTeam(iActiveTeam).isHasMet(gc.getPlayer(j).getTeam()):
						szPlayerText = gc.getPlayer(j).getName()
						if (-1 != iSecretaryGeneralVote):
							szPlayerText += localText.getText("TXT_KEY_VICTORY_SCREEN_PLAYER_VOTES", (gc.getPlayer(j).getVotes(iSecretaryGeneralVote, i), )) 
						if (gc.getGame().canHaveSecretaryGeneral(i) and gc.getGame().getSecretaryGeneral(i) == gc.getPlayer(j).getTeam()):
							iRow = screen.appendTableRow(szTable)
							screen.setTableText(szTable, 0, iRow, szPlayerText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
							screen.setTableText(szTable, 1, iRow, gc.getVoteSourceInfo(i).getSecretaryGeneralText(), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						elif (gc.getPlayer(j).isFullMember(i)):
							iRow = screen.appendTableRow(szTable)
							screen.setTableText(szTable, 0, iRow, szPlayerText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
							screen.setTableText(szTable, 1, iRow, localText.getText("TXT_KEY_VOTESOURCE_FULL_MEMBER", ()), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						elif (gc.getPlayer(j).isVotingMember(i)):
							iRow = screen.appendTableRow(szTable)
							screen.setTableText(szTable, 0, iRow, szPlayerText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
							screen.setTableText(szTable, 1, iRow, localText.getText("TXT_KEY_VOTESOURCE_VOTING_MEMBER", ()), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

				iRow = screen.appendTableRow(szTable)
		# Remove the final empty row (K-Mod)
		if screen.getTableNumRows(szTable) > 0:
			screen.setTableNumRows(szTable, screen.getTableNumRows(szTable)-1)
	# </advc.703>

	def showMembersScreen_NonBUG(self):
		self.deleteAllWidgets()

		activePlayer = gc.getPlayer(self.iActivePlayer)
		iActiveTeam = activePlayer.getTeam()

		screen = self.getScreen()

		screen.addPanel(self.getNextWidgetName(), "", "", False, False, self.X_AREA-10, self.Y_AREA-15, self.W_AREA+20, self.H_AREA+30, PanelStyles.PANEL_STYLE_BLUE50)
		szTable = self.getNextWidgetName()
		screen.addTableControlGFC(szTable, 2, self.X_AREA, self.Y_AREA, self.W_AREA, self.H_AREA, False, False, 32,32, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSelect(szTable, False)
		screen.setTableColumnHeader(szTable, 0, "", self.TABLE2_WIDTH_0)
		screen.setTableColumnHeader(szTable, 1, "", self.TABLE2_WIDTH_1)
		# advc.703: Code moved into subroutine
		self.appendMemberRows(screen, szTable)


	# <advc.703>
	def showRiseFall(self):
		self.deleteAllWidgets()	
		screen = self.getScreen()
		screen.addPanel(self.getNextWidgetName(), "", "", False, False, self.X_AREA-10, self.Y_AREA-15, self.W_AREA+20, self.H_AREA+30, PanelStyles.PANEL_STYLE_BLUE50)
		szTable = self.getNextWidgetName()
		screen.addTableControlGFC(szTable, 5, self.X_AREA, self.Y_AREA, self.W_AREA, self.H_AREA, False, False, 32,32, TableStyles.TABLE_STYLE_STANDARD)
		screen.setTableColumnHeader(szTable, 0, "", self.RF_TABLEW_0)
		screen.setTableColumnHeader(szTable, 1, "", self.RF_TABLEW_1)
		screen.setTableColumnHeader(szTable, 2, "", self.RF_TABLEW_2)
		screen.setTableColumnHeader(szTable, 3, "", self.RF_TABLEW_3)
		screen.setTableColumnHeader(szTable, 4, "", self.RF_TABLEW_4)
		screen.appendTableRow(szTable)
		iNumRows = screen.getTableNumRows(szTable)
		iTitleRow = iNumRows - 1
		szHeading = u"<font=4b>" + localText.getText("TXT_KEY_RF_CHAPTER_PLURAL", ()) + u"</font>"
		screen.setTableText(szTable, 0, iTitleRow, szHeading, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		partialScore = 0
		allScored = true
		extGame = gc.getGame().getGameState() == GameStateTypes.GAMESTATE_EXTENDED
		currentTurn = gc.getGame().getGameTurn()
		maxChapters = gc.getGame().getMaxChapters()
		for i in range(maxChapters):
			startTurn = gc.getGame().getChapterStart(i)
			endTurn = gc.getGame().getChapterEnd(i)
			# Endless chapter for extended play
			extChapter = (endTurn < 0 and gc.getGame().getMaxTurns() > 0)
			ongoing = (currentTurn >= startTurn and (currentTurn <= endTurn or extChapter))
			if ongoing and not extChapter:
				allScored = false
			iRow = screen.appendTableRow(szTable)
			# [0] Chapter index
			s = str(i + 1)
			if ongoing:
				s = self.highlight(s)
			s = "  " + s # Don't want this to align exactly with the heading
			screen.setTableText(szTable, 0, iRow, s, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# [1] Civ description
			chapterCiv = gc.getGame().getChapterCiv(i)
			activeCiv = gc.getGame().getActivePlayer()
			s = "?"
			if currentTurn >= startTurn and chapterCiv >= 0:
				s = gc.getPlayer(chapterCiv).getCivilizationShortDescription(0)
				if ongoing and not extChapter:
					s = self.highlight(s)
			screen.setTableText(szTable, 1, iRow, s, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# [2] Start/end turn
			s = localText.getText("TXT_KEY_RF_CHAPTER_INTERVAL", (startTurn, endTurn))
			if endTurn < 0:
				s = localText.getText("TXT_KEY_RF_TURN", (startTurn,))
			if ongoing and not extChapter:
				s = self.highlight(s)
			if endTurn >= 0: # Don't highlight this part
				s += " (" + BugUtil.getDisplayYear(gc.getGame().getTurnYear(endTurn)) + ")"
			screen.setTableText(szTable, 2, iRow, s, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# [3] Points
			chapterScore = gc.getGame().getChapterScore(i)
			scoreTurn = gc.getGame().getChapterScoreTurn(i)
			s = localText.getText("TXT_KEY_RF_CHAPTER_PTS", (chapterScore,))
			# Highlight the part in parentheses when ongoing, otherwise the pts.
			if currentTurn <= endTurn:
				s += " ("
				remaining = endTurn - currentTurn + 1
				if remaining > 1:
					s += self.highlight(localText.getText("TXT_KEY_RF_CHAPTER_REMAIN", (remaining,)))
				else:
					s += self.highlight(localText.getText("TXT_KEY_RF_CHAPTER_FINAL", ()))
				s += ")"
			elif currentTurn >= scoreTurn:
				# Doesn't actually imply that the chapter's already scored b/c
				# the chapter civ could come later in the turn order than the
				# current civ; but I'm too lazy to export isScored to Python.
				s = self.highlight(s)
			elif chapterCiv < 0 or not gc.getTeam(gc.getPlayer(chapterCiv).getTeam()).isHasMet(gc.getPlayer(activeCiv).getTeam()):
				s += " (" + localText.getText("TXT_KEY_RF_CHAPTER_LAST_KNOWN", (endTurn,)) + ")"
			if currentTurn < startTurn:
				s = "?"
			elif extChapter:
				s = "-"
			else:
				partialScore += chapterScore
			screen.setTableText(szTable, 3, iRow, s, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			# [4] Scoring turn
			s = localText.getText("TXT_KEY_RF_CHAPTER_SCORE_TURN", (scoreTurn,))
			if i == maxChapters - 1 and scoreTurn < 0:
				s = localText.getText("TXT_KEY_RF_CHAPTER_SCORE_END", ())
			elif currentTurn >= scoreTurn:
				s = self.highlight(s)
			if not extGame or currentTurn >= scoreTurn:
				screen.setTableText(szTable, 4, iRow, s, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			if extChapter:
				break # The remaining chapters will never happen
		iRow = screen.appendTableRow(szTable)
		s1 = localText.getText("TXT_KEY_RF_PARTIAL_SUM", ())
		s2 = str(partialScore)
		if allScored: # Only relevant when the game is over
			s1 = self.highlight(localText.getText("TXT_KEY_RF_TOTAL", ()))
			s2 = self.highlight(s2)
		screen.setTableText(szTable, 2, iRow, s1, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_RIGHT_JUSTIFY)
		screen.setTableText(szTable, 3, iRow, s2, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# The panels in the lower half are based on CvReligionScreen
		# By default, let them take up the bottom half of the screen. But if there isn't enough room for the chapters in the upper part ...
		iExcessChapters = min(maxChapters - 10, 3)
		if iExcessChapters > 0:
			iHeightMinus = iExcessChapters * 30 # not sure if 30 per chapter row is exactly right
			self.Y_RF_AREA += iHeightMinus
			self.H_RF_AREA -= iHeightMinus
		screen.addPanel(self.AREA1_ID, "", "", True, True, self.X_RF1_AREA, self.Y_RF_AREA, self.W_RF_AREA, self.H_RF_AREA, PanelStyles.PANEL_STYLE_BLUE50)
		screen.addPanel(self.AREA2_ID, "", "", True, True, self.X_RF2_AREA, self.Y_RF_AREA, self.W_RF_AREA, self.H_RF_AREA, PanelStyles.PANEL_STYLE_BLUE50)
		chapterScoreText = u"<font=3b>"
		chapterScoreText += localText.getText("TXT_KEY_RF_CHAPTER_BREAKDOWN_H", ())
		chapterScoreText += u"</font>"
		chapterScoreText += u"<font=3>" + "\n\n"
		if not extGame: # No breakdown after game end
			chapterScoreText += gc.getGame().chapterScoreBreakdown()
		chapterScoreText += u"</font>"
		riseScoreText = u"<font=3b>"
		if extGame >= 0:
			riseScoreText += localText.getText("TXT_KEY_RF_RISE_FINAL_H", ())
		else:
			riseScoreText += localText.getText("TXT_KEY_RF_RISE_BREAKDOWN_H", ())
		riseScoreText += u"</font>"
		riseScoreText += u"<font=3>" + "\n\n"
		riseScoreText += gc.getGame().riseScoreBreakdown()
		riseScoreText += u"</font>"
		screen.addMultilineText("Child" + self.AREA1_ID, chapterScoreText, self.X_RF1_AREA+20, self.Y_RF_AREA+20, self.W_RF_AREA-20, self.H_RF_AREA-20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.addMultilineText("Child" + self.AREA2_ID, riseScoreText, self.X_RF2_AREA+10, self.Y_RF_AREA+20, self.W_RF_AREA-10, self.H_RF_AREA-20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		self.drawTabs()
	
	def highlight(self, s):
		return u"<font=2b>" + s + u"</font>"
	# </advc.703>

	def formatPercent(self, f):
		return "%.1f%%" % f

	def getVoteReq(self, i, iVote):
		iMaxVotes = 0
		iMinVotes = 999999
		for iLoop in range(gc.getNumVoteInfos()):
			iVoteReq = gc.getGame().getVoteRequired(iLoop, i)
			if iVoteReq > 0:
				if iVoteReq > iMaxVotes:
					iMaxVotes = iVoteReq
				if iVoteReq < iMinVotes:
					iMinVotes = iVoteReq

		if iVote == 1:
			return iMinVotes
		else:
			return iMaxVotes

	def getCandStatusName(self, iCand):
		# iCand is a team
		if iCand == -1: # there is no candidate
			return -1, "-"

		activePlayer = gc.getPlayer(self.iActivePlayer)
		iActiveTeam = activePlayer.getTeam()

		if iActiveTeam == iCand:
			return 1, gc.getTeam(iCand).getName()

		if gc.getTeam(iActiveTeam).isHasMet(iCand):
			return 1, gc.getTeam(iCand).getName()
		else:
			return 0, localText.getText("TXT_KEY_TOPCIVS_UNKNOWN", ())

	def getPlayerStatusName(self, iPlayer):
		if iPlayer == -1: # there is no player
			return False, "-"

		pPlayer = gc.getPlayer(iPlayer)
		iPlayerTeam = pPlayer.getTeam()
		activePlayer = gc.getPlayer(self.iActivePlayer)
		iActiveTeam = activePlayer.getTeam()

		if iActiveTeam == iPlayerTeam:
			return True, pPlayer.getName()

		if gc.getTeam(iActiveTeam).isHasMet(iPlayerTeam):
			return True, pPlayer.getName()
		else:
			return False, localText.getText("TXT_KEY_TOPCIVS_UNKNOWN", ())

	def getVoteTotalColor(self, iVoteReq, iVoteTotal, iVoteCand, bWinner, bVictoryVote):
		print "%i %i %i" % (iVoteReq, iVoteTotal, iVoteCand)
		if not bWinner:
			return -1
		if (iVoteCand > iVoteReq
		and bVictoryVote):
			return ColorUtil.keyToType("COLOR_RED")
		if iVoteTotal > iVoteReq:
			return ColorUtil.keyToType("COLOR_GREEN")
		return -1

	def showGameSettingsScreen(self):
		self.deleteAllWidgets()	
		screen = self.getScreen()

		activePlayer = gc.getPlayer(self.iActivePlayer)

		szSettingsPanel = self.getNextWidgetName()
		screen.addPanel(szSettingsPanel, localText.getText("TXT_KEY_MAIN_MENU_SETTINGS", ()).upper(), "", True, True, self.SETTINGS_PANEL_X1, self.SETTINGS_PANEL_Y - 10, self.SETTINGS_PANEL_WIDTH, self.SETTINGS_PANEL_HEIGHT, PanelStyles.PANEL_STYLE_MAIN)
		szSettingsTable = self.getNextWidgetName()
		screen.addListBoxGFC(szSettingsTable, "", self.SETTINGS_PANEL_X1 + self.MARGIN, self.SETTINGS_PANEL_Y + self.MARGIN, self.SETTINGS_PANEL_WIDTH - 2*self.MARGIN, self.SETTINGS_PANEL_HEIGHT - 2*self.MARGIN, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(szSettingsTable, False)

# K-Mod, disabled the following		
#		if showHOFSettingChecks and BugPath.isMac():
#			failedHOFChecks = True
#			showHOFSettingChecks = False
#			screen.appendListBoxStringNoUpdate(szSettingsTable, self.BuffyWarningMac, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
				
		screen.appendListBoxStringNoUpdate(szSettingsTable, localText.getText("TXT_KEY_LEADER_CIV_DESCRIPTION", (activePlayer.getNameKey(), activePlayer.getCivilizationShortDescriptionKey())), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.appendListBoxStringNoUpdate(szSettingsTable, u"     (" + CyGameTextMgr().parseLeaderTraits(activePlayer.getLeaderType(), activePlayer.getCivilizationType(), True, False) + ")", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.appendListBoxStringNoUpdate(szSettingsTable, " ", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		g = gc.getGame() # advc
		m = gc.getMap() # advc
		#screen.appendListBoxStringNoUpdate(szSettingsTable, localText.getText("TXT_KEY_SETTINGS_DIFFICULTY", (gc.getHandicapInfo(activePlayer.getHandicapType()).getTextKey(), )), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		# K-Mod. In multiplayer games, show both the game difficulty and the player difficulty
		if activePlayer.getHandicapType() == g.getHandicapType():
			screen.appendListBoxStringNoUpdate(szSettingsTable, localText.getText("TXT_KEY_SETTINGS_DIFFICULTY", (gc.getHandicapInfo(activePlayer.getHandicapType()).getTextKey(), )), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		else:
			szBuffer = "%s :\n  %s (%s) / %s (%s)" % (localText.getText("TXT_KEY_PITBOSS_DIFFICULTY", ()), gc.getHandicapInfo(activePlayer.getHandicapType()).getText(), localText.getText("TXT_KEY_MAIN_MENU_PLAYER", ()), gc.getHandicapInfo(g.getHandicapType()).getText(), localText.getText("TXT_KEY_OPTIONS_GAME", ()))
			screen.appendListBoxStringNoUpdate(szSettingsTable, szBuffer, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		# K-Mod end

		screen.appendListBoxStringNoUpdate(szSettingsTable, " ", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.appendListBoxStringNoUpdate(szSettingsTable, m.getMapScriptName(), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		screen.appendListBoxStringNoUpdate(szSettingsTable, localText.getText("TXT_KEY_SETTINGS_MAP_SIZE", (gc.getWorldInfo(m.getWorldSize()).getTextKey(), )), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.appendListBoxStringNoUpdate(szSettingsTable, localText.getText("TXT_KEY_SETTINGS_CLIMATE", (gc.getClimateInfo(m.getClimate()).getTextKey(), )), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		# <advc.137> Use TXT_KEY_LOW/HIGH instead of SEA_LEVEL keys. (The SEA_LEVEL keys include the recommendation for the Custom Game screen - don't want that here.
		seaLvl = gc.getSeaLevelInfo(m.getSeaLevel()).getSeaLevelChange()
		if seaLvl == 0: # Medium sea level - use the BtS code.
			screen.appendListBoxStringNoUpdate(szSettingsTable, localText.getText("TXT_KEY_SETTINGS_SEA_LEVEL", (gc.getSeaLevelInfo(m.getSeaLevel()).getTextKey(), )), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		else:
			szKey = "TXT_KEY_HIGH"
			if seaLvl < 0:
				szKey = "TXT_KEY_LOW"
			s = localText.getText(szKey, ())
			screen.appendListBoxStringNoUpdate(szSettingsTable, localText.getText("TXT_KEY_SETTINGS_SEA_LEVEL", (s,)), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		# </advc.137>
		# <advc.004>
		for i in range(m.getNumCustomMapOptions()):
			# Don't know how to call map script functions in Python, so I've implemented that in the DLL.
			szDesc = m.getNonDefaultCustomMapOptionDesc(i)
			if len(szDesc) <= 0: # Meaning that the option is set to its default value
				continue
			screen.appendListBoxStringNoUpdate(szSettingsTable, szDesc, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		screen.appendListBoxStringNoUpdate(szSettingsTable, str(g.countCivPlayersEverAlive()) + " " + localText.getText("TXT_KEY_MAIN_MENU_PLAYERS", ()), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# </advc.004>
		screen.appendListBoxStringNoUpdate(szSettingsTable, " ", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		screen.appendListBoxStringNoUpdate(szSettingsTable, localText.getText("TXT_KEY_SETTINGS_STARTING_ERA", (gc.getEraInfo(g.getStartEra()).getTextKey(), )), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		# <advc.251>
		iStartTurn = g.getStartTurn()
		if iStartTurn != 0:
			szTurnDate = CyGameTextMgr().getDateStr(iStartTurn, False, g.getCalendar(), g.getStartYear(), g.getGameSpeedType())
			screen.appendListBoxStringNoUpdate(szSettingsTable, localText.getText("TXT_KEY_SETTINGS_START_TURN", (iStartTurn, szTurnDate)), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		# </advc.251>
		screen.appendListBoxStringNoUpdate(szSettingsTable, localText.getText("TXT_KEY_SETTINGS_GAME_SPEED", (gc.getGameSpeedInfo(g.getGameSpeedType()).getTextKey(), )), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
		# advc.106i
		screen.appendListBoxStringNoUpdate(szSettingsTable, BugPath.getModName() + " Mod", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		screen.updateListBox(szSettingsTable)

		szOptionsPanel = self.getNextWidgetName()
		screen.addPanel(szOptionsPanel, localText.getText("TXT_KEY_MAIN_MENU_CUSTOM_SETUP_OPTIONS", ()).upper(), "", True, True, self.SETTINGS_PANEL_X2, self.SETTINGS_PANEL_Y - 10, self.SETTINGS_PANEL_WIDTH, self.SETTINGS_PANEL_HEIGHT, PanelStyles.PANEL_STYLE_MAIN)
		szOptionsTable = self.getNextWidgetName()
		screen.addListBoxGFC(szOptionsTable, "", self.SETTINGS_PANEL_X2 + self.MARGIN, self.SETTINGS_PANEL_Y + self.MARGIN, self.SETTINGS_PANEL_WIDTH - 2*self.MARGIN, self.SETTINGS_PANEL_HEIGHT - 2*self.MARGIN, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(szOptionsTable, False)
		bSPaH = g.isOption(GameOptionTypes.GAMEOPTION_SPAH) # advc.250b
		for i in range(GameOptionTypes.NUM_GAMEOPTION_TYPES):
			if g.isOption(i):
				# <advc.104> Handle Aggressive AI below; skip it here.
				if i == GameOptionTypes.GAMEOPTION_AGGRESSIVE_AI:
					continue # </advc.104>
				# <advc.250b> Handle Advanced Start options below if SPaH.
				if bSPaH:
					if i == GameOptionTypes.GAMEOPTION_SPAH:
						continue
					if i == GameOptionTypes.GAMEOPTION_ADVANCED_START:
						continue
				# </advc.250b>
				# <advc.300> Show earliest turn that barbarians can spawn on
				szDescr = gc.getGameOptionInfo(i).getDescription()
				if i == GameOptionTypes.GAMEOPTION_RAGING_BARBARIANS:
					iBarbarianStartTurn = g.getBarbarianStartTurn()
					# Stop displaying after some time [Since v0.94: Don't show the start turn at all.]
					if False and iBarbarianStartTurn + 25 > g.getGameTurn():
						szDescr += "\n\t("
						szDescr += localText.getText("TXT_KEY_BARB_START", ())
						szDescr += " " + str(iBarbarianStartTurn) + ")"
				# </advc.300>
				screen.appendListBoxStringNoUpdate(szOptionsTable, szDescr, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# <advc.004> Disabled victory conditions. Some overlap with CvReplayInfo::addSettingsMsg
		for i in range(gc.getNumVictoryInfos()):
			if not g.isVictoryValid(i):
				screen.appendListBoxStringNoUpdate(szOptionsTable, gc.getVictoryInfo(i).getDescription() + " " + localText.getText("TXT_KEY_VICTORY_DISABLED",()), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# </advc.004>

		# <advc.104> AI settings
		bLegacyAI = g.useKModAI()
		szAIOption = None
		if g.isOption(GameOptionTypes.GAMEOPTION_AGGRESSIVE_AI) and bLegacyAI:
			szAIOption = localText.getText("TXT_KEY_GAME_OPTION_AGGRESSIVE_AI",())
		elif bLegacyAI: # Only possible if Aggressive AI disabled through XML
			szAIOption = "Non-Aggressive AI"
		if not szAIOption is None:
			screen.appendListBoxStringNoUpdate(szOptionsTable, szAIOption, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		# </advc.104>

		if (g.isOption(GameOptionTypes.GAMEOPTION_ADVANCED_START)):
			if not bSPaH: # advc.250b
				szNumPoints = u"%s %d" % (localText.getText("TXT_KEY_ADVANCED_START_POINTS", ()), g.getNumAdvancedStartPoints())
			# <advc.250b>
			else:
				# Could be done directly in Python, but I also need it for advc.106i
				szNumPoints = g.SPaHPointsForSettingsScreen()
			# </advc.250b>
			screen.appendListBoxStringNoUpdate(szOptionsTable, szNumPoints, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		if g.isGameMultiPlayer():
			for i in range(gc.getNumMPOptionInfos()):
				if (g.isMPOption(i)):
					screen.appendListBoxStringNoUpdate(szOptionsTable, gc.getMPOptionInfo(i).getDescription(), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			if g.getMaxTurns() > 0:
				# <advc.135d> Based on code in CvGame::setStartTurnYear
				iDefaultEndTurn = 0
				for i in range(gc.getGameSpeedInfo(g.getGameSpeedType()).getNumTurnIncrements()):
					iDefaultEndTurn += gc.getGameSpeedInfo(g.getGameSpeedType()).getGameTurnInfo(i).iNumGameTurnsPerIncrement;
				if g.getMaxTurns() != iDefaultEndTurn:
				# </advc.135d>
					szMaxTurns = u"%s %d" % (localText.getText("TXT_KEY_TURN_LIMIT_TAG", ()), g.getMaxTurns())
					screen.appendListBoxStringNoUpdate(szOptionsTable, szMaxTurns, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

			if (g.getMaxCityElimination() > 0):
				szMaxCityElimination = u"%s %d" % (localText.getText("TXT_KEY_CITY_ELIM_TAG", ()), g.getMaxCityElimination())
				screen.appendListBoxStringNoUpdate(szOptionsTable, szMaxCityElimination, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		if (g.hasSkippedSaveChecksum()):
			screen.appendListBoxStringNoUpdate(szOptionsTable, localText.getText("TXT_KEY_BUFFYWARNING_CHECKSUM_SKIPPED", ()), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

		screen.updateListBox(szOptionsTable)

		szCivsPanel = self.getNextWidgetName()
		screen.addPanel(szCivsPanel, localText.getText("TXT_KEY_RIVALS_MET", ()).upper(), "", True, True, self.SETTINGS_PANEL_X3, self.SETTINGS_PANEL_Y - 10, self.SETTINGS_PANEL_WIDTH, self.SETTINGS_PANEL_HEIGHT, PanelStyles.PANEL_STYLE_MAIN)

		szCivsTable = self.getNextWidgetName()
		screen.addListBoxGFC(szCivsTable, "", self.SETTINGS_PANEL_X3 + self.MARGIN, self.SETTINGS_PANEL_Y + self.MARGIN, self.SETTINGS_PANEL_WIDTH - 2*self.MARGIN, self.SETTINGS_PANEL_HEIGHT - 2*self.MARGIN, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(szCivsTable, False)

		for iLoopPlayer in range(gc.getMAX_CIV_PLAYERS()):
			player = gc.getPlayer(iLoopPlayer)
			if (player.isEverAlive() and iLoopPlayer != self.iActivePlayer and (gc.getTeam(player.getTeam()).isHasMet(activePlayer.getTeam()) or g.isDebugMode()) and not player.isBarbarian() and not player.isMinorCiv()):
				screen.appendListBoxStringNoUpdate(szCivsTable, localText.getText("TXT_KEY_LEADER_CIV_DESCRIPTION", (player.getNameKey(), player.getCivilizationShortDescriptionKey())), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
				screen.appendListBoxStringNoUpdate(szCivsTable, u"     (" + CyGameTextMgr().parseLeaderTraits(player.getLeaderType(), player.getCivilizationType(), True, False) + ")", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )
				screen.appendListBoxStringNoUpdate(szCivsTable, " ", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

		screen.updateListBox(szCivsTable)

		self.drawTabs()

	def showVictoryConditionScreen(self):
		activePlayer = PyHelpers.PyPlayer(self.iActivePlayer)
		iActiveTeam = gc.getPlayer(self.iActivePlayer).getTeam()

		# checking if apollo has been built - clear arrays / lists / whatever they are called
		self.ApolloTeamsChecked = set()
		self.ApolloTeamCheckResult = {}

		# Conquest
		nRivals = -1
# BUG Additions Start
		nknown = 0
		nVassaled = 0
# BUG Additions End
		for i in range(gc.getMAX_CIV_TEAMS()):
			if (gc.getTeam(i).isAlive() and not gc.getTeam(i).isMinorCiv() and not gc.getTeam(i).isBarbarian()):
				nRivals += 1
# BUG Additions Start
				if i != iActiveTeam:
					if gc.getTeam(i).isHasMet(iActiveTeam):
						nknown += 1
					if gc.getTeam(i).isVassal(iActiveTeam):
						nVassaled += 1
# BUG Additions End

		# Population
		totalPop = gc.getGame().getTotalPopulation()
		ourPop = activePlayer.getTeam().getTotalPopulation()
		if (totalPop > 0):
			popPercent = (ourPop * 100.0) / totalPop
		else:
			popPercent = 0.0

		iBestPopTeam = -1
		bestPop = 0
		for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
			if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
				if (iLoopTeam != iActiveTeam and (activePlayer.getTeam().isHasMet(iLoopTeam) or gc.getGame().isDebugMode())):
					teamPop = gc.getTeam(iLoopTeam).getTotalPopulation()
					if (teamPop > bestPop):
						bestPop = teamPop
						iBestPopTeam = iLoopTeam

		# Score
		ourScore = gc.getGame().getTeamScore(iActiveTeam)

		iBestScoreTeam = -1
		bestScore = 0
		for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
			if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
				if (iLoopTeam != iActiveTeam and (activePlayer.getTeam().isHasMet(iLoopTeam) or gc.getGame().isDebugMode())):
					teamScore = gc.getGame().getTeamScore(iLoopTeam)
					if (teamScore > bestScore):
						bestScore = teamScore
						iBestScoreTeam = iLoopTeam

		# Land Area
		totalLand = gc.getMap().getLandPlots()
		ourLand = activePlayer.getTeam().getTotalLand()
		if (totalLand > 0):
			landPercent = (ourLand * 100.0) / totalLand
		else:
			landPercent = 0.0

		iBestLandTeam = -1
		bestLand = 0
		for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
			if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
				if (iLoopTeam != iActiveTeam and (activePlayer.getTeam().isHasMet(iLoopTeam) or gc.getGame().isDebugMode())):
					teamLand = gc.getTeam(iLoopTeam).getTotalLand()
					if (teamLand > bestLand):
						bestLand = teamLand
						iBestLandTeam = iLoopTeam

		# Religion
		iOurReligion = -1
		ourReligionPercent = 0
		for iLoopReligion in range(gc.getNumReligionInfos()):
			if (activePlayer.getTeam().hasHolyCity(iLoopReligion)):
				religionPercent = gc.getGame().calculateReligionPercent(iLoopReligion)
				if (religionPercent > ourReligionPercent):
					ourReligionPercent = religionPercent
					iOurReligion = iLoopReligion

		iBestReligion = -1
		bestReligionPercent = 0
		for iLoopReligion in range(gc.getNumReligionInfos()):
			if (iLoopReligion != iOurReligion):
				religionPercent = gc.getGame().calculateReligionPercent(iLoopReligion)
				if (religionPercent > bestReligionPercent):
					bestReligionPercent = religionPercent
					iBestReligion = iLoopReligion

		# Vote
		aiVoteBuildingClass = []
		for i in range(gc.getNumBuildingInfos()):
			for j in range(gc.getNumVoteSourceInfos()):
				if (gc.getBuildingInfo(i).getVoteSourceType() == j):
					iUNTeam = -1
					bUnknown = true 
					for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
						if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
							if (gc.getTeam(iLoopTeam).getBuildingClassCount(gc.getBuildingInfo(i).getBuildingClassType()) > 0):
								iUNTeam = iLoopTeam
								if (iLoopTeam == iActiveTeam or gc.getGame().isDebugMode() or activePlayer.getTeam().isHasMet(iLoopTeam)):
									bUnknown = False
								break

					aiVoteBuildingClass.append((gc.getBuildingInfo(i).getBuildingClassType(), iUNTeam, bUnknown))

		#self.bVoteTab = (len(aiVoteBuildingClass) > 0)
		# K-Mod
		self.bVoteTab = False
		for i in range(gc.getNumVoteSourceInfos()):
			if gc.getGame().isDiploVote(i):
				self.bVoteTab = True
				break
		# K-Mod end

		self.deleteAllWidgets()	
		screen = self.getScreen()

		# Start filling in the table below
		screen.addPanel(self.getNextWidgetName(), "", "", False, False, self.X_AREA-10, self.Y_AREA-15, self.W_AREA+20, self.H_AREA+30, PanelStyles.PANEL_STYLE_BLUE50)
		szTable = self.getNextWidgetName()
		screen.addTableControlGFC(szTable, 6, self.X_AREA, self.Y_AREA, self.W_AREA, self.H_AREA, False, False, 32,32, TableStyles.TABLE_STYLE_STANDARD)
		screen.setTableColumnHeader(szTable, 0, "", self.TABLE_WIDTH_0)
		screen.setTableColumnHeader(szTable, 1, "", self.TABLE_WIDTH_1)
		screen.setTableColumnHeader(szTable, 2, "", self.TABLE_WIDTH_2)
		screen.setTableColumnHeader(szTable, 3, "", self.TABLE_WIDTH_3)
		screen.setTableColumnHeader(szTable, 4, "", self.TABLE_WIDTH_4)
		screen.setTableColumnHeader(szTable, 5, "", self.TABLE_WIDTH_5)
		screen.appendTableRow(szTable)

		for iLoopVC in range(gc.getNumVictoryInfos()):
			victory = gc.getVictoryInfo(iLoopVC)
			if gc.getGame().isVictoryValid(iLoopVC):

				iNumRows = screen.getTableNumRows(szTable)
				szVictoryType = u"<font=4b>" + victory.getDescription().upper() + u"</font>"
				if (victory.isEndScore() and (gc.getGame().getMaxTurns() > gc.getGame().getElapsedGameTurns())):
					szVictoryType += "    (" + localText.getText("TXT_KEY_MISC_TURNS_LEFT", (gc.getGame().getMaxTurns() - gc.getGame().getElapsedGameTurns(), )) + ")"

				iVictoryTitleRow = iNumRows - 1
				screen.setTableText(szTable, 0, iVictoryTitleRow, szVictoryType, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				bSpaceshipFound = False

				bEntriesFound = False

				if (victory.isTargetScore() and gc.getGame().getTargetScore() != 0):

					iRow = screen.appendTableRow(szTable)
					screen.setTableText(szTable, 0, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_TARGET_SCORE", (gc.getGame().getTargetScore(), )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					screen.setTableText(szTable, 2, iRow, activePlayer.getTeam().getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					screen.setTableText(szTable, 3, iRow, (u"%d" % ourScore), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					if (iBestScoreTeam != -1):
						screen.setTableText(szTable, 4, iRow, gc.getTeam(iBestScoreTeam).getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						screen.setTableText(szTable, 5, iRow, (u"%d" % bestScore), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					bEntriesFound = True

				if (victory.isEndScore()):

					szText1 = localText.getText("TXT_KEY_VICTORY_SCREEN_HIGHEST_SCORE", (CyGameTextMgr().getTimeStr(gc.getGame().getStartTurn() + gc.getGame().getMaxTurns(), False), ))

					iRow = screen.appendTableRow(szTable)
					screen.setTableText(szTable, 0, iRow, szText1, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					screen.setTableText(szTable, 2, iRow, activePlayer.getTeam().getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					screen.setTableText(szTable, 3, iRow, (u"%d" % ourScore), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					if (iBestScoreTeam != -1):
						screen.setTableText(szTable, 4, iRow, gc.getTeam(iBestScoreTeam).getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						screen.setTableText(szTable, 5, iRow, (u"%d" % bestScore), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					bEntriesFound = True

				if (victory.isConquest()):
					iRow = screen.appendTableRow(szTable)
					screen.setTableText(szTable, 0, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_ELIMINATE_ALL", ()), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					screen.setTableText(szTable, 2, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_RIVALS_LEFT", ()), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					screen.setTableText(szTable, 3, iRow, unicode(nRivals), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					bEntriesFound = True
# BUG Additions Start
					if AdvisorOpt.isVictories():
						if nVassaled != 0:
							sString = localText.getText("TXT_KEY_BUG_VICTORY_VASSALED", (nVassaled, ))
							screen.setTableText(szTable, 4, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						if nRivals - nknown != 0:
							sString = localText.getText("TXT_KEY_BUG_VICTORY_UNKNOWN", (nRivals - nknown, ))
							screen.setTableText(szTable, 5, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
# BUG Additions End

				if (gc.getGame().getAdjustedPopulationPercent(iLoopVC) > 0):
					iRow = screen.appendTableRow(szTable)
					screen.setTableText(szTable, 0, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_PERCENT_POP", (gc.getGame().getAdjustedPopulationPercent(iLoopVC), )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					screen.setTableText(szTable, 2, iRow, activePlayer.getTeam().getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					screen.setTableText(szTable, 3, iRow, (u"%.2f%%" % popPercent), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					if (iBestPopTeam != -1):
						screen.setTableText(szTable, 4, iRow, gc.getTeam(iBestPopTeam).getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						screen.setTableText(szTable, 5, iRow, (u"%.2f%%" % (bestPop * 100 / totalPop)), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					bEntriesFound = True


				if (gc.getGame().getAdjustedLandPercent(iLoopVC) > 0):
					iRow = screen.appendTableRow(szTable)
					screen.setTableText(szTable, 0, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_PERCENT_LAND", (gc.getGame().getAdjustedLandPercent(iLoopVC), )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					screen.setTableText(szTable, 2, iRow, activePlayer.getTeam().getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					screen.setTableText(szTable, 3, iRow, (u"%.2f%%" % landPercent), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					if (iBestLandTeam != -1):
						screen.setTableText(szTable, 4, iRow, gc.getTeam(iBestLandTeam).getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						screen.setTableText(szTable, 5, iRow, (u"%.2f%%" % (bestLand * 100 / totalLand)), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					bEntriesFound = True

				if (victory.getReligionPercent() > 0):
					iRow = screen.appendTableRow(szTable)
					screen.setTableText(szTable, 0, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_PERCENT_RELIGION", (victory.getReligionPercent(), )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					if (iOurReligion != -1):
						screen.setTableText(szTable, 2, iRow, gc.getReligionInfo(iOurReligion).getDescription() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						screen.setTableText(szTable, 3, iRow, (u"%d%%" % ourReligionPercent), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					else:
						screen.setTableText(szTable, 2, iRow, activePlayer.getTeam().getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						screen.setTableText(szTable, 3, iRow, u"No Holy City", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					if (iBestReligion != -1):
						screen.setTableText(szTable, 4, iRow, gc.getReligionInfo(iBestReligion).getDescription() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						screen.setTableText(szTable, 5, iRow, (u"%d%%" % religionPercent), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					bEntriesFound = True

				if (victory.getTotalCultureRatio() > 0):
					iRow = screen.appendTableRow(szTable)
					# <advc> Moved down to have this stuff in one place
					ourCulture = activePlayer.getTeam().countTotalCulture()
					iBestCultureTeam = -1
					bestCulture = 0
					for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
						if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
							if (iLoopTeam != iActiveTeam and (activePlayer.getTeam().isHasMet(iLoopTeam) or gc.getGame().isDebugMode())):
								teamCulture = gc.getTeam(iLoopTeam).countTotalCulture()
								if (teamCulture > bestCulture):
									bestCulture = teamCulture
									iBestCultureTeam = iLoopTeam
					# </advc>
					#iCulturePercent = int((100.0 * bestCulture) / victory.getTotalCultureRatio())
					# advc.004: The above is neither helpful nor really a percentage. Let's simply show the target ratio for now. What's really needed is, I think, the ratio of bestCulture to secondBestCulture, where secondBestCulture is computed w/o checking isHasMet. Or possibly that ratio divided by victory.getTotalCultureRatio (meaning that 100% is needed for victory). 
					iCulturePercent = victory.getTotalCultureRatio()
					screen.setTableText(szTable, 0, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_PERCENT_CULTURE", (iCulturePercent, )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					screen.setTableText(szTable, 2, iRow, activePlayer.getTeam().getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					screen.setTableText(szTable, 3, iRow, unicode(ourCulture), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					if (iBestLandTeam != -1):
						screen.setTableText(szTable, 4, iRow, gc.getTeam(iBestCultureTeam).getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						screen.setTableText(szTable, 5, iRow, unicode(bestCulture), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
					bEntriesFound = True

				iBestBuildingTeam = -1
				bestBuilding = 0
				for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
					if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
						if (iLoopTeam != iActiveTeam and (activePlayer.getTeam().isHasMet(iLoopTeam) or gc.getGame().isDebugMode())):
							teamBuilding = 0
							for i in range(gc.getNumBuildingClassInfos()):
								if (gc.getBuildingClassInfo(i).getVictoryThreshold(iLoopVC) > 0):
									teamBuilding += gc.getTeam(iLoopTeam).getBuildingClassCount(i)
							if (teamBuilding > bestBuilding):
								bestBuilding = teamBuilding
								iBestBuildingTeam = iLoopTeam

				for i in range(gc.getNumBuildingClassInfos()):
					if (gc.getBuildingClassInfo(i).getVictoryThreshold(iLoopVC) > 0):
						iRow = screen.appendTableRow(szTable)
						szNumber = unicode(gc.getBuildingClassInfo(i).getVictoryThreshold(iLoopVC))
						screen.setTableText(szTable, 0, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_BUILDING", (szNumber, gc.getBuildingClassInfo(i).getTextKey())), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						screen.setTableText(szTable, 2, iRow, activePlayer.getTeam().getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						screen.setTableText(szTable, 3, iRow, activePlayer.getTeam().getBuildingClassCount(i), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						if (iBestBuildingTeam != -1):
							screen.setTableText(szTable, 4, iRow, gc.getTeam(iBestBuildingTeam).getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
							screen.setTableText(szTable, 5, iRow, gc.getTeam(iBestBuildingTeam).getBuildingClassCount(i), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						bEntriesFound = True

				iBestProjectTeam = -1
				bestProject = -1
				for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
					if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
						if (iLoopTeam != iActiveTeam
						and (activePlayer.getTeam().isHasMet(iLoopTeam) or gc.getGame().isDebugMode())
						and self.isApolloBuiltbyTeam(gc.getTeam(iLoopTeam))):
							teamProject = 0
							for i in range(gc.getNumProjectInfos()):
								if (gc.getProjectInfo(i).getVictoryThreshold(iLoopVC) > 0):
									teamProject += gc.getTeam(iLoopTeam).getProjectCount(i)
							if (teamProject > bestProject):
								bestProject = teamProject
								iBestProjectTeam = iLoopTeam

# BUG Additions Start
				if AdvisorOpt.isVictories():
					bApolloShown = False
					for i in range(gc.getNumProjectInfos()):
						if (gc.getProjectInfo(i).getVictoryThreshold(iLoopVC) > 0):
							if not self.isApolloBuilt():
								iRow = screen.appendTableRow(szTable)
								screen.setTableText(szTable, 0, iRow, localText.getText("TXT_KEY_PROJECT_APOLLO_PROGRAM", ()), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
								screen.setTableText(szTable, 2, iRow, activePlayer.getTeam().getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
								screen.setTableText(szTable, 3, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_NOT_BUILT", ()), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
								bEntriesFound = True
								break
							else:
								if not bApolloShown:
									bApolloShown = True
									iRow = screen.appendTableRow(szTable)
									screen.setTableText(szTable, 0, iRow, localText.getText("TXT_KEY_PROJECT_APOLLO_PROGRAM", ()), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

									if self.isApolloBuiltbyTeam(activePlayer.getTeam()):
										screen.setTableText(szTable, 2, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_BUILT", (activePlayer.getTeam().getName(), )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
									else:
										screen.setTableText(szTable, 2, iRow, activePlayer.getTeam().getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
										screen.setTableText(szTable, 3, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_NOT_BUILT", ()), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

									if (iBestProjectTeam != -1):
										screen.setTableText(szTable, 4, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_BUILT", (gc.getTeam(iBestProjectTeam).getName(), )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

								iRow = screen.appendTableRow(szTable)
								iReqTech = gc.getProjectInfo(i).getTechPrereq()

								if (gc.getProjectInfo(i).getVictoryMinThreshold(iLoopVC) == gc.getProjectInfo(i).getVictoryThreshold(iLoopVC)):
									szNumber = unicode(gc.getProjectInfo(i).getVictoryThreshold(iLoopVC))
								else:
									szNumber = unicode(gc.getProjectInfo(i).getVictoryMinThreshold(iLoopVC)) + u"-" + unicode(gc.getProjectInfo(i).getVictoryThreshold(iLoopVC))

								sSSPart = localText.getText("TXT_KEY_VICTORY_SCREEN_BUILDING", (szNumber, gc.getProjectInfo(i).getTextKey()))
								screen.setTableText(szTable, 0, iRow, sSSPart, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

								if self.isApolloBuiltbyTeam(activePlayer.getTeam()):

									bHasTech = gc.getTeam(iActiveTeam).isHasTech(iReqTech)
									sSSPlayer = activePlayer.getTeam().getName() + ":"
									sSSCount = "%i [+%i]" % (activePlayer.getTeam().getProjectCount(i), activePlayer.getTeam().getProjectMaking(i))

									iHasTechColor = -1
									iSSColor = 0
									if activePlayer.getTeam().getProjectCount(i) == gc.getProjectInfo(i).getVictoryThreshold(iLoopVC):
										sSSCount = "%i" % (activePlayer.getTeam().getProjectCount(i))
										iSSColor = ColorUtil.keyToType("COLOR_GREEN")
									elif activePlayer.getTeam().getProjectCount(i) >= gc.getProjectInfo(i).getVictoryMinThreshold(iLoopVC):
										iSSColor = ColorUtil.keyToType("COLOR_YELLOW")

									if iSSColor > 0:
										sSSPlayer = localText.changeTextColor(sSSPlayer, iSSColor)
										sSSCount = localText.changeTextColor(sSSCount, iSSColor)

									screen.setTableText(szTable, 2, iRow, sSSPlayer, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
									if bHasTech:
										screen.setTableText(szTable, 3, iRow, sSSCount, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

									#check if spaceship
									if (gc.getProjectInfo(i).isSpaceship()):
										bSpaceshipFound = True
								
								# add AI space ship info
								if (iBestProjectTeam != -1):
									pTeam = gc.getTeam(iBestProjectTeam)
									sSSPlayer = gc.getTeam(iBestProjectTeam).getName() + ":"
									sSSCount = "%i" % (pTeam.getProjectCount(i))

									Techs = TechUtil.getVisibleKnownTechs(pTeam.getLeaderID(), self.iActivePlayer)
									bHasTech = iReqTech in Techs

									iHasTechColor = -1
									iSSColor = 0
									if pTeam.getProjectCount(i) == gc.getProjectInfo(i).getVictoryThreshold(iLoopVC):
										iSSColor = ColorUtil.keyToType("COLOR_GREEN")
									elif pTeam.getProjectCount(i) >= gc.getProjectInfo(i).getVictoryMinThreshold(iLoopVC):
										iSSColor = ColorUtil.keyToType("COLOR_YELLOW")
									elif bHasTech:
										iSSColor = ColorUtil.keyToType("COLOR_PLAYER_ORANGE")

									if iSSColor > 0:
										sSSPlayer = localText.changeTextColor(sSSPlayer, iSSColor)
										sSSCount = localText.changeTextColor(sSSCount, iSSColor)

									screen.setTableText(szTable, 4, iRow, sSSPlayer, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
									screen.setTableText(szTable, 5, iRow, sSSCount, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

								bEntriesFound = True

				else: # vanilla BtS SShip display
					for i in range(gc.getNumProjectInfos()):
						if (gc.getProjectInfo(i).getVictoryThreshold(iLoopVC) > 0):
							iRow = screen.appendTableRow(szTable)
							if (gc.getProjectInfo(i).getVictoryMinThreshold(iLoopVC) == gc.getProjectInfo(i).getVictoryThreshold(iLoopVC)):
								szNumber = unicode(gc.getProjectInfo(i).getVictoryThreshold(iLoopVC))
							else:
								szNumber = unicode(gc.getProjectInfo(i).getVictoryMinThreshold(iLoopVC)) + u"-" + unicode(gc.getProjectInfo(i).getVictoryThreshold(iLoopVC))
							screen.setTableText(szTable, 0, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_BUILDING", (szNumber, gc.getProjectInfo(i).getTextKey())), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
							screen.setTableText(szTable, 2, iRow, activePlayer.getTeam().getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
							screen.setTableText(szTable, 3, iRow, str(activePlayer.getTeam().getProjectCount(i)), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
							
							#check if spaceship
							if (gc.getProjectInfo(i).isSpaceship()):
								bSpaceshipFound = True

							if (iBestProjectTeam != -1):
								screen.setTableText(szTable, 4, iRow, gc.getTeam(iBestProjectTeam).getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
								screen.setTableText(szTable, 5, iRow, unicode(gc.getTeam(iBestProjectTeam).getProjectCount(i)), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

							bEntriesFound = True
# BUG Additions End
						
				#add spaceship button
				if (bSpaceshipFound):
					screen.setButtonGFC("SpaceShipButton" + str(iLoopVC), localText.getText("TXT_KEY_GLOBELAYER_STRATEGY_VIEW", ()), "", 0, 0, 15, 10, WidgetTypes.WIDGET_GENERAL, self.SPACESHIP_SCREEN_BUTTON, -1, ButtonStyles.BUTTON_STYLE_STANDARD )
					screen.attachControlToTableCell("SpaceShipButton" + str(iLoopVC), szTable, iVictoryTitleRow, 1)
					
					victoryDelay = gc.getTeam(iActiveTeam).getVictoryCountdown(iLoopVC)
					if((victoryDelay > 0) and (gc.getGame().getGameState() != GameStateTypes.GAMESTATE_EXTENDED)):
						victoryDate = CyGameTextMgr().getTimeStr(gc.getGame().getGameTurn() + victoryDelay, False)
						screen.setTableText(szTable, 2, iVictoryTitleRow, localText.getText("TXT_KEY_SPACE_SHIP_SCREEN_ARRIVAL", ()) + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						screen.setTableText(szTable, 3, iVictoryTitleRow, victoryDate, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						screen.setTableText(szTable, 4, iVictoryTitleRow, localText.getText("TXT_KEY_REPLAY_SCREEN_TURNS", ()) + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						screen.setTableText(szTable, 5, iVictoryTitleRow, str(victoryDelay), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						
				if (victory.isDiploVote()):
					for (iVoteBuildingClass, iUNTeam, bUnknown) in aiVoteBuildingClass:
						iRow = screen.appendTableRow(szTable)
						screen.setTableText(szTable, 0, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_ELECTION", (gc.getBuildingClassInfo(iVoteBuildingClass).getTextKey(), )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						if (iUNTeam != -1):
							if bUnknown:
								szName = localText.getText("TXT_KEY_TOPCIVS_UNKNOWN", ())
							else:
								szName = gc.getTeam(iUNTeam).getName()
							screen.setTableText(szTable, 2, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_BUILT", (szName, )), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						else:
							screen.setTableText(szTable, 2, iRow, localText.getText("TXT_KEY_VICTORY_SCREEN_NOT_BUILT", ()), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						bEntriesFound = True
					
				if (victory.getCityCulture() != CultureLevelTypes.NO_CULTURELEVEL and victory.getNumCultureCities() > 0):
					ourBestCities = self.getListCultureCities(iActiveTeam, victory)
					
					# K-Mod - changed to loop through teams rather than players, to match actual victory conditions.
					iBestCultureTeam = -1
					bestCityCulture = 0
					maxCityCulture = GameUtil.getCultureThreshold(victory.getCityCulture()) # BUG

					for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
						if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
							if (iLoopTeam != iActiveTeam and (activePlayer.getTeam().isHasMet(iLoopTeam) or gc.getGame().isDebugMode())):
								theirBestCities = self.getListCultureCities(iLoopTeam, victory)

								iTotalCulture = 0
								for loopCity in theirBestCities:
									if loopCity[0] >= maxCityCulture:
										iTotalCulture += maxCityCulture
									else:
										iTotalCulture += loopCity[0]

								# Note: we could give more weight to the lower cities if we wanted a more accurate gauge of how close to victory the team is.
								if (iTotalCulture >= bestCityCulture):
									bestCityCulture = iTotalCulture
									iBestCultureTeam = iLoopTeam

					if (iBestCultureTeam != -1):
						theirBestCities = self.getListCultureCities(iBestCultureTeam, victory)
					else:
						theirBestCities = []

					iRow = screen.appendTableRow(szTable)
					# <advc.126>
					eVictoryLevel = victory.getCityCulture()
					iCultureThresh = gc.getGame().getCultureThreshold(eVictoryLevel)
					szCultureVictoryText = localText.getText("TXT_KEY_VICTORY_SCREEN_CITY_CULTURE", (victory.getNumCultureCities(), gc.getCultureLevelInfo(eVictoryLevel).getTextKey(), iCultureThresh)) # </advc.126>
					screen.setTableText(szTable, 0, iRow, szCultureVictoryText, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

					for i in range(victory.getNumCultureCities()):
						if (len(ourBestCities) > i):
							screen.setTableText(szTable, 2, iRow, ourBestCities[i][1].getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
# BUG Additions Start
							if AdvisorOpt.isVictories():
								if ourBestCities[i][2] == -1:
									sString = "%i (-)" % (ourBestCities[i][0])
								elif ourBestCities[i][2] > 100:
									sString = "%i (100+)" % (ourBestCities[i][0])
								elif ourBestCities[i][2] < 1:
									sString = "%i (L)" % (ourBestCities[i][0])
								else:
									sString = "%i (%i)" % (ourBestCities[i][0], ourBestCities[i][2])
							else:
								sString = "%i" % (ourBestCities[i][0])

							screen.setTableText(szTable, 3, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
# BUG Additions End

						if (len(theirBestCities) > i):
							screen.setTableText(szTable, 4, iRow, theirBestCities[i][1].getName() + ":", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

# BUG Additions Start
							if AdvisorOpt.isVictories():
								if theirBestCities[i][2] == -1:
									sString = "%i (-)" % (theirBestCities[i][0])
								elif theirBestCities[i][2] > 100:
									sString = "%i (100+)" % (theirBestCities[i][0])
								elif theirBestCities[i][2] < 1:
									sString = "%i (L)" % (theirBestCities[i][0])
								else:
									sString = "%i (%i)" % (theirBestCities[i][0], theirBestCities[i][2])
							else:
								sString = "%i" % (theirBestCities[i][0])

							screen.setTableText(szTable, 5, iRow, sString, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
# BUG Additions End

						if (i < victory.getNumCultureCities()-1):
							iRow = screen.appendTableRow(szTable)
					bEntriesFound = True
					
				if (bEntriesFound):
					screen.appendTableRow(szTable)
					screen.appendTableRow(szTable)

		# Remove the two final empty rows (K-Mod)
		if screen.getTableNumRows(szTable) > 2:
			screen.setTableNumRows(szTable, screen.getTableNumRows(szTable)-2)
		#

		# civ picker dropdown
		if (CyGame().isDebugMode()):
			self.szDropdownName = self.DEBUG_DROPDOWN_ID
			screen.addDropDownBoxGFC(self.szDropdownName, 22, 12, 300, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
			for j in range(gc.getMAX_CIV_PLAYERS()): # advc.007: barbs excluded
				if (gc.getPlayer(j).isAlive()):
					screen.addPullDownString(self.szDropdownName, gc.getPlayer(j).getName(), j, j, False )
		
		self.drawTabs()

#	def getListCultureCities(self, iPlayer):
# rewritten for K-Mod
	def getListCultureCities(self, iTeam, victory):
		maxCityCulture = GameUtil.getCultureThreshold(victory.getCityCulture())

		if iTeam < 0:
			return []
		# else continue

		cultureCityList = [] # item format is (culture, city, turns to threshold)

		for i in range(gc.getMAX_CIV_PLAYERS()):
			loopPlayer = gc.getPlayer(i)
			if (not loopPlayer.isAlive() or loopPlayer.getTeam() != iTeam):
				continue
			# otherwise, loop through their cities
			(loopCity, iter) = loopPlayer.firstCity(false)
			while(loopCity): # advc.001d: isRevealed clause added
				if (not loopCity.isNone() and loopCity.getTeam() == iTeam and loopCity.isRevealed(gc.getGame().getActiveTeam(), True)):
					iRate = loopCity.getCommerceRateTimes100(CommerceTypes.COMMERCE_CULTURE)
					if iRate == 0:
						iTurns = -1
					else:
						iCultureLeftTimes100 = 100 * maxCityCulture - loopCity.getCultureTimes100(loopCity.getOwner())
						iTurns = int((iCultureLeftTimes100 + iRate - 1) / iRate)
					cultureCityList.append((loopCity.getCulture(loopCity.getOwner()), PyHelpers.PyCity(loopCity.getOwner(), loopCity.getID()), iTurns))
					# I don't see the point of PyCity. But that's what's used elsewhere.
				(loopCity, iter) = loopPlayer.nextCity(iter, false)

		cultureCityList.sort()
		cultureCityList.reverse()
		return cultureCityList[0:victory.getNumCultureCities()]
# K-Mod end

# BUG Additions Start
	def getVotesForWhichCandidate(self, iPlayer, iCand1, iCand2, iVote):
		# returns are 1 = vote for candidate 1
		#             2 = vote for candidate 2
		#            -1 = abstain

		# iVote = 1 means vote for SecGen or Pope
		# iVote = 2 means vote for diplomatic victory

		# candidates are teams!!!

		# * AI votes for itself if it can
		# * AI votes for a team member if it can
		# * AI votes for its master, if it is a vassal
		# * if the AI attitude to one of the candidates is 'friendly' and the other is 'pleased' or less, AI votes for 'friend'
		# * if both candidates are at 'friendly' status, votes for one with highest attitude
		# * if neither candidate is at 'friendly', abstains

		iPTeam = gc.getPlayer(iPlayer).getTeam()
		iPCand1 = self.getPlayerOnTeam(iCand1)
		iPCand2 = self.getPlayerOnTeam(iCand2)

		# * player votes for its own team if it can
		if iPTeam == iCand1: return 1
		if iPTeam == iCand2: return 2

		# if player is human, votes for self or abstains
		if iPlayer == self.iActivePlayer: return -1

		# * AI votes for its master, if it is a vassal
		if gc.getTeam(iPTeam).isVassal(iCand1): return 1
		if gc.getTeam(iPTeam).isVassal(iCand2): return 2

		# get player category (friendly) to candidates
		iC1Cat = AttitudeUtil.getAttitudeCategory(iPlayer, iPCand1)
		iC2Cat = AttitudeUtil.getAttitudeCategory(iPlayer, iPCand2)

		# the cut-off for SecGen votes is pleased (3)
		# the cut-off for Diplo victory votes is friendly (4)
		if iVote == 1:  # vote for SecGen or Pope
			iCutOff = 3
		else:
			iCutOff = 4

		# * if neither candidate is at 'friendly', abstains
		# assumes friendly = 4, pleased = 3, etc
		if (iC1Cat < iCutOff
		and iC2Cat < iCutOff):
			return -1

		# * if the AI attitude to one of the candidates is 'friendly' and the other is 'pleased' or less, AI votes for 'friend'
		if (iC1Cat >= iCutOff
		and iC1Cat > iC2Cat):
			return 1

		if (iC2Cat >= iCutOff
		and iC2Cat > iC1Cat):
			return 2

		# if the code gets to here, then both candidates are at or above the cutoff
		# and they are both at the same category (ie both friendly)
		# need to decide on straight attitude (visible only)

		# get player attitude to candidates
		iC1Att = AttitudeUtil.getAttitudeCount(iPlayer, iPCand1)
		iC2Att = AttitudeUtil.getAttitudeCount(iPlayer, iPCand2)

		# * if both candidates are at 'friendly' status, votes for one with highest attitude
		if iC2Att < iC1Att: # ties go to Candidate #1
			return 1
		else:
			return 2

		return -1

	def getPlayerOnTeam(self, iTeam):
		for i in range(gc.getMAX_PLAYERS()):
			if iTeam == gc.getPlayer(i).getTeam():
				return i

		return -1

	def getAP_UN_OwnerTeam(self):
		for i in range(gc.getNumBuildingInfos()):
			for j in range(gc.getNumVoteSourceInfos()):
				if (gc.getBuildingInfo(i).getVoteSourceType() == j):
					for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
						if (gc.getTeam(iLoopTeam).isAlive() and not gc.getTeam(iLoopTeam).isMinorCiv() and not gc.getTeam(iLoopTeam).isBarbarian()):
							if (gc.getTeam(iLoopTeam).getBuildingClassCount(gc.getBuildingInfo(i).getBuildingClassType()) > 0):
								return iLoopTeam
								break
		return -1

	def canBuildSSComponent(self, vTeam, vComponent):
		if(not vTeam.isHasTech(vComponent.getTechPrereq())):
			return False
		else:
			for j in range(gc.getNumProjectInfos()):
				if(vTeam.getProjectCount(j) < vComponent.getProjectsNeeded(j)):
					return False
		return True

	def isApolloBuilt(self):
		activePlayer = gc.getPlayer(self.iActivePlayer)
		iActiveTeam = activePlayer.getTeam()

		# check if anyone has built the apollo project (PROJECT_APOLLO_PROGRAM)
		for iLoopTeam in range(gc.getMAX_CIV_TEAMS()):
			pLoopTeam = gc.getTeam(iLoopTeam)
			if (pLoopTeam.isAlive()
			and not pLoopTeam.isMinorCiv()
			and not pLoopTeam.isBarbarian()):
				if iLoopTeam == iActiveTeam:
					bContact = True
				elif (gc.getTeam(iActiveTeam).isHasMet(iLoopTeam)
				or gc.getGame().isDebugMode()):
					bContact = True
				else:
					bContact = False

				if bContact:
					if self.isApolloBuiltbyTeam(pLoopTeam):
						return True
		return False

	def isApolloBuiltbyTeam(self, vTeam):
		iTeam = vTeam.getID()
#		print vTeam.getName()

		if iTeam in self.ApolloTeamsChecked:
			sString = "1: %s" % (self.ApolloTeamCheckResult[iTeam])
#			print sString
#			return self.ApolloTeamCheckResult[iTeam]

		for i in range(gc.getNumProjectInfos()):
			component = gc.getProjectInfo(i)
			if (component.isSpaceship()):
				bApollo = True
				for j in range(gc.getNumProjectInfos()):
					if(vTeam.getProjectCount(j) < component.getProjectsNeeded(j)):
						bApollo = False
#					sString = "2: %s %s %i %i %s" % (component.getDescription(), gc.getProjectInfo(j).getDescription(), vTeam.getProjectCount(j), component.getProjectsNeeded(j), bApollo)
#					print sString

#				sString = "2: %s %s" % (component.getDescription(), bApollo)
#				print sString
				if bApollo:
					self.ApolloTeamCheckResult[iTeam] = True
					self.ApolloTeamsChecked.add(iTeam)
					return True
				break

		self.ApolloTeamCheckResult[iTeam] = False
		self.ApolloTeamsChecked.add(iTeam)
		return False
# BUG Additions End

	# returns a unique ID for a widget in this screen
	def getNextWidgetName(self):
		szName = self.WIDGET_ID + str(self.nWidgetCount)
		self.nWidgetCount += 1
		return szName

	def deleteAllWidgets(self):
		screen = self.getScreen()
		i = self.nWidgetCount - 1
		while (i >= 0):
			self.nWidgetCount = i
			screen.deleteWidget(self.getNextWidgetName())
			i -= 1

		self.nWidgetCount = 0

		screen.deleteWidget(self.Vote_Pope_ID)
		screen.deleteWidget(self.Vote_DipVic_ID)
		screen.deleteWidget(self.Vote_AP_ID)
		screen.deleteWidget(self.Vote_UN_ID)
		screen.deleteWidget(self.AREA1_ID)
		screen.deleteWidget(self.AREA2_ID)
		screen.deleteWidget("Child" + self.AREA1_ID)
		screen.deleteWidget("Child" + self.AREA2_ID)

	# handle the input for this screen...
	def handleInput (self, inputClass):
		sWidget = inputClass.getFunctionName()
		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED):
			if (sWidget == self.DEBUG_DROPDOWN_ID):
				szName = self.DEBUG_DROPDOWN_ID
				iIndex = self.getScreen().getSelectedPullDownID(szName)
				self.iActivePlayer = self.getScreen().getPullDownData(szName, iIndex)
				self.iScreen = VICTORY_CONDITION_SCREEN
				self.showVictoryConditionScreen()
		elif (inputClass.getNotifyCode() == NotifyCode.NOTIFY_CLICKED):
			if (sWidget == self.VC_TAB_ID):
				self.iScreen = VICTORY_CONDITION_SCREEN
				self.showVictoryConditionScreen()
			elif (sWidget == self.SETTINGS_TAB_ID):
				self.iScreen = GAME_SETTINGS_SCREEN
				self.showGameSettingsScreen()
			elif (sWidget == self.UN_RESOLUTION_TAB_ID):
				self.iScreen = UN_RESOLUTION_SCREEN
				self.showVotingScreen()
			elif (sWidget == self.UN_MEMBERS_TAB_ID):
				self.iScreen = UN_MEMBERS_SCREEN
				self.showMembersScreen()
			# <advc.703>
			elif (sWidget == self.RF_SCORE_TAB_ID):
				self.iScreen = RF_SCORE_SCREEN
				self.showRiseFall()
			# </advc.703>

			elif (sWidget == self.Vote_Pope_ID and self.VoteType == 2):
				self.VoteType = 1
				self.iScreen = UN_MEMBERS_SCREEN
				self.showMembersScreen()

			elif (sWidget == self.Vote_DipVic_ID and self.VoteType == 1):
				self.VoteType = 2
				self.iScreen = UN_MEMBERS_SCREEN
				self.showMembersScreen()

			elif (sWidget == self.Vote_AP_ID and self.VoteBody == 2):
				self.VoteBody = 1
				self.iScreen = UN_MEMBERS_SCREEN
				self.showMembersScreen()

			elif (sWidget == self.Vote_UN_ID and self.VoteBody == 1):
				self.VoteBody = 2
				self.iScreen = UN_MEMBERS_SCREEN
				self.showMembersScreen()

			elif (inputClass.getData1() == self.SPACESHIP_SCREEN_BUTTON):
				#close screen
				screen = self.getScreen()
				screen.setDying(True)
				CyInterface().clearSelectedCities()

				#popup spaceship screen
				popupInfo = CyPopupInfo()
				popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON_SCREEN)
				popupInfo.setData1(-1)
				popupInfo.setText(u"showSpaceShip")
				popupInfo.addPopup(self.iActivePlayer)

	def update(self, fDelta):
		return
