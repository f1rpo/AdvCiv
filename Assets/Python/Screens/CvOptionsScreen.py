## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005

# For Input see CvOptionsScreenCallbackInterface in Python\EntryPoints\

import CvUtil
from CvPythonExtensions import *

# globals
gc = CyGlobalContext()
UserProfile = CyUserProfile()
localText = CyTranslator()

class CvOptionsScreen:
	"Options Screen"
	
	def __init__(self):
		
		self.iScreenHeight = 50
		
		self.iGameOptionsTabID		= 0
		self.iGraphicOptionsTabID	= 1
		self.iAudioOptionsTabID		= 2
		self.iOtherOptionsTabID	        = 3
		
		self.callbackIFace = "CvOptionsScreenCallbackInterface"
		
	def getTabControl(self):
		return self.pTabControl
		
	def getGameOptionsTabName(self):
		return self.szGameOptionsTabName
	def getGraphicOptionsTabName(self):
		return self.szGraphicsOptionsTabName
	def getAudioOptionsTabName(self):
		return self.szAudioOptionsTabName
	def getOtherOptionsTabName(self):
		return self.szOtherOptionsTabName
		
	# Used by Callback Interface to set path via checkbox
	def getMusicPath(self):
		return self.getTabControl().getText("CustomMusicEditBox")
	def getCustomMusicCheckboxName(self):
		return self.szCustomMusicCheckboxName
		
	# Used by Callback Interface to set Alarm time via checkbox
	def getAlarmHour(self):
		return self.getTabControl().getText("AlarmHourEditBox")
	def getAlarmMin(self):
		return self.getTabControl().getText("AlarmMinEditBox")
		
	# Used by Callback Interface to get user defined profile names from editbox
	def setProfileEditCtrlText(self, szProfileName):
		szWideProfName = CvUtil.convertToUnicode(szProfileName)
		self.getTabControl().setText("ProfileNameEditBox", szWideProfName)
	def getProfileEditCtrlText(self):
		return self.getTabControl().getText("ProfileNameEditBox")
		
	# Called from C++ after a custom music path is selected via FileDialogBox
	def updateMusicPath (self, szMusicPath):
		
		self.getTabControl().setText("CustomMusicEditBox", szMusicPath)
		self.getTabControl().setValue(self.getCustomMusicCheckboxName(), true)
		
#########################################################################################
################################## SCREEN CONSTRUCTION ##################################
#########################################################################################
	
	def initText(self):
		
		self.szTabControlName = localText.getText("TXT_KEY_OPTIONS_TITLE", ())
		
		self.szGameOptionsTabName = localText.getText("TXT_KEY_OPTIONS_GAME", ())
		self.szGraphicsOptionsTabName = localText.getText("TXT_KEY_OPTIONS_GRAPHICS", ())
		self.szAudioOptionsTabName = localText.getText("TXT_KEY_OPTIONS_AUDIO", ())
		self.szOtherOptionsTabName = localText.getText("TXT_KEY_OPTIONS_SCREEN_OTHER", ())
		
	def refreshScreen (self):
		
		# advc.076: Active profile may have changed
		UserProfile = CyUserProfile()
		
		#################### Game Options ####################
		
		szTab = self.getGameOptionsTabName()
		for iOptionLoop in range(PlayerOptionTypes.NUM_PLAYEROPTION_TYPES):
			szWidgetName = "GameOptionCheckBox_" + str(iOptionLoop)
			bOptionOn = UserProfile.getPlayerOption(iOptionLoop)
			self.pTabControl.setValue(szWidgetName, bOptionOn)
		
		# Languages Dropdown
		self.getTabControl().setValue("LanguagesDropdownBox", CyGame().getCurrentLanguage())
		
		#################### GRAPHICS ####################
		
		szTab = self.getGraphicOptionsTabName()

		# Graphics Dropdowns
	
		self.getTabControl().setValue(self.szResolutionComboBoxName, UserProfile.getResolution() )
		self.getTabControl().setValue("AntiAliasingDropdownBox", UserProfile.getAntiAliasing() )
		self.getTabControl().setValue("GraphicsLevelDropdownBox", UserProfile.getGraphicsLevel() )
		self.getTabControl().setValue("RenderQualityDropdownBox", UserProfile.getRenderQualityLevel() )
		self.getTabControl().setValue("GlobeViewDropdownBox", UserProfile.getGlobeViewRenderLevel() )
		self.getTabControl().setValue("MovieDropdownBox", UserProfile.getMovieQualityLevel() )
		self.getTabControl().setValue("MainMenuDropdownBox", UserProfile.getMainMenu() )
		
		# Graphic Option Checkboxes
		for iOptionLoop in range(GraphicOptionTypes.NUM_GRAPHICOPTION_TYPES):
			szWidgetName = "GraphicOptionCheckbox_" + str(iOptionLoop)
			bOptionOn = UserProfile.getGraphicOption(iOptionLoop)
			self.pTabControl.setValue(szWidgetName, bOptionOn)
		
		#################### AUDIO ####################
		
		szTab = self.getAudioOptionsTabName()
		
		iMax = UserProfile.getVolumeStops()
		
		# Volume Sliders and No Sound Checkboxes
		for iWidgetNum in range(6):
			if (iWidgetNum == 0):		# Master Volume
				iInitialVal = iMax-UserProfile.getMasterVolume()-1
				bNoSoundTrue = UserProfile.isMasterNoSound()
			elif (iWidgetNum == 1):		# Music Volume
				iInitialVal = iMax-UserProfile.getMusicVolume()-1
				bNoSoundTrue = UserProfile.isMusicNoSound()
			elif (iWidgetNum == 2):		# Sound Effects Volume
				iInitialVal = iMax-UserProfile.getSoundEffectsVolume()-1
				bNoSoundTrue = UserProfile.isSoundEffectsNoSound()
			elif (iWidgetNum == 3):		# Speech Volume
				iInitialVal = iMax-UserProfile.getSpeechVolume()-1
				bNoSoundTrue = UserProfile.isSpeechNoSound()
			elif (iWidgetNum == 4):		# Ambience Volume
				iInitialVal = iMax-UserProfile.getAmbienceVolume()-1
				bNoSoundTrue = UserProfile.isAmbienceNoSound()
			elif (iWidgetNum == 5):		# Interface Volume
				iInitialVal = iMax-UserProfile.getInterfaceVolume()-1
				bNoSoundTrue = UserProfile.isInterfaceNoSound()
				
			# Volume Slider
			szWidgetName = "VolumeSlider_" + str(iWidgetNum)
   			self.getTabControl().setValue(szWidgetName, iInitialVal)
			
			# Volume Checkbox
			szWidgetName = "VolumeNoSoundCheckbox_" + str(iWidgetNum)
			self.pTabControl.setValue(szWidgetName, bNoSoundTrue)
			
		# Voice Capture Dropdown
		self.getTabControl().setValue("CaptureDeviceDropdownBox", UserProfile.getCaptureDeviceIndex() )
		# Voice Capture Slider
#   		self.getTabControl().setValue("CaptureVolumeSlider", UserProfile.getMaxCaptureVolume() - UserProfile.getCaptureVolume())
   		self.getTabControl().setValue("CaptureVolumeSlider", UserProfile.getCaptureVolume())
		
		# Voice Playback Dropdown
		self.getTabControl().setValue("PlaybackDeviceDropdownBox", UserProfile.getPlaybackDeviceIndex() )
		# Voice Playback Slider
#   		self.getTabControl().setValue("PlaybackVolumeSlider", UserProfile.getMaxPlaybackVolume() - UserProfile.getPlaybackVolume())
   		self.getTabControl().setValue("PlaybackVolumeSlider", UserProfile.getPlaybackVolume())
		
		# Voice Chatting Checkbox
		self.getTabControl().setValue("VoiceChatCheckbox", UserProfile.useVoice())
		
		# Speaker config
		iInitialSelection = 0
		for iSpeakerConfigLoop in range(16):
			szActiveConfig = UserProfile.getSpeakerConfigFromList(iSpeakerConfigLoop)
			if (UserProfile.getSpeakerConfig() == szActiveConfig):
				iInitialSelection = iSpeakerConfigLoop
			
		# Speaker Config Dropdown
		self.getTabControl().setValue("SpeakerConfigDropdownBox", iInitialSelection )
		
		# Custom Music Path Checkbox
		bUseCustomMusicPath = false
		if (UserProfile.getMusicPath() != ""):
			bUseCustomMusicPath = true
		self.getTabControl().setValue(self.getCustomMusicCheckboxName(), bUseCustomMusicPath)
		
		# Custom Music Path Editbox
		szEditBoxDesc = ""
		if (UserProfile.getMusicPath() != ""):
			szEditBoxDesc = CvUtil.convertToUnicode(UserProfile.getMusicPath())
		self.getTabControl().setText("CustomMusicEditBox", szEditBoxDesc)
		
		#################### CLOCK ####################
		
		szTab = self.getOtherOptionsTabName()
		
		# Clock On Checkbox
		self.getTabControl().setValue("ClockOnCheckbox", UserProfile.isClockOn())
		
		# 24 Hour Clock Checkbox
		self.getTabControl().setValue("24HourClockCheckbox", UserProfile.is24Hours())
		
		# Alarm On Checkbox
		self.getTabControl().setValue("AlarmOnCheckbox", isAlarmOn())
		
		# Alarm Hours
		self.getTabControl().setText("AlarmHourEditBox", str(getAlarmHourLeft()))
		self.getTabControl().setText("AlarmMinEditBox", str(getAlarmMinLeft()))
		
		#################### PROFILE ####################
		
		# Profile Name Editbox
		self.getTabControl().setText("ProfileNameEditBox", CvUtil.convertToUnicode(UserProfile.getProfileName()))
		
		aszDropdownElements = ()
		for iProfileLoop in range(UserProfile.getNumProfileFiles()):
			szProfileFileName = UserProfile.getProfileFileName(iProfileLoop)
			# Cut off file path and extension
			szProfile = szProfileFileName[szProfileFileName.find("PROFILES\\")+9:-4]
			aszDropdownElements = aszDropdownElements + (szProfile,)
			if (UserProfile.getProfileName() == szProfile):
				iInitialSelection = iProfileLoop
		
		self.getTabControl().changeDropdownContents("ProfilesDropdownBox", aszDropdownElements)
		
		# Profile List Dropdown
		self.getTabControl().setValue("ProfilesDropdownBox", iInitialSelection)
		
		#################### PROFILE ####################
		
		# Broadband Radio Button
   		self.getTabControl().setValue("BroadbandSelection", not gc.getGame().isModem())
		
		# Modem Checkbox
   		self.getTabControl().setValue("ModemSelection", gc.getGame().isModem())
		
		# <advc.076> Update button tooltips b/c profile name may have changed
		# Tbd.: A function updateTooltips to get rid of duplicate code and hardcoded widget names
		szProfileName = UserProfile.getProfileName()
		for iTab in range(4):
			self.getTabControl().setToolTip("OptionsExitButton" + str(iTab), localText.getText("TXT_KEY_OPTIONS_SCREEN_EXIT_HELP", (szProfileName,)))
		self.getTabControl().setToolTip("GameOptionsResetButton", localText.getText("TXT_KEY_OPTIONS_SCREEN_RESET1_HELP", (szProfileName,)))
		self.getTabControl().setToolTip("GraphicOptionsResetButton", localText.getText("TXT_KEY_OPTIONS_SCREEN_RESET2_HELP", (szProfileName,)))
		self.getTabControl().setToolTip("AudioOptionsResetButton", localText.getText("TXT_KEY_OPTIONS_SCREEN_RESET3_HELP", (szProfileName,)))
		# </advc.076>
		
	def interfaceScreen (self):
		"Initial creation of the screen"
		self.initText()
		
		self.pTabControl = CyGTabCtrl(self.szTabControlName, false, false)
		self.pTabControl.setModal(1)
		self.pTabControl.setSize(800,600)
		self.pTabControl.setControlsExpanding(false)
		self.pTabControl.setColumnLength(self.iScreenHeight)
		
		# Set Tabs
		self.pTabControl.attachTabItem("GameForm", self.szGameOptionsTabName)
		self.pTabControl.attachTabItem("GraphicsForm", self.szGraphicsOptionsTabName)
		self.pTabControl.attachTabItem("AudioForm", self.szAudioOptionsTabName)
		self.pTabControl.attachTabItem("OtherForm", self.szOtherOptionsTabName)
		
		# <advc.076> Initialize some data for the draw...Tab functions
		self.graphicOptionsDone = set()
		self.playerOptionsDone = set()
		self.tab1Options = set() # for the reset buttons
		self.tab2Options = set()
		self.iBlank = 0 # for attachVSpace
		self.iHeading = 0 # for attachHeading
		self.iExitButton = 0 # for attachExitButton
		# </advc.076>
		
		self.drawGameOptionsTab()
		self.drawGraphicOptionsTab()
		self.drawAudioOptionsTab()
		self.drawOtherTab()

		
	def drawGameOptionsTab(self):
		
		tab = self.pTabControl
		
		tab.attachVBox("GameForm", "GameVBox")
		
		# Add Game Options
		
		tab.attachPanel("GameVBox", "GamePanelCenter")
		tab.setStyle("GamePanelCenter", "Panel_Tan15_Style")
		tab.setLayoutFlag("GamePanelCenter", "LAYOUT_SIZE_HEXPANDING")
		tab.setLayoutFlag("GamePanelCenter", "LAYOUT_SIZE_VEXPANDING")

		tab.attachScrollPanel("GamePanelCenter", "GamePanel")
		tab.setLayoutFlag("GamePanel", "LAYOUT_SIZE_HEXPANDING")
		tab.setLayoutFlag("GamePanel", "LAYOUT_SIZE_VEXPANDING")
		
		hbox = "GameHBox" # advc.076
		tab.attachHBox("GamePanel", hbox)
		tab.setLayoutFlag(hbox, "LAYOUT_SIZE_HEXPANDING")
		vbox1 = "GameVBox1" # advc.076: Put this in a variable
		tab.attachVBox(hbox, vbox1)
		tab.setLayoutFlag(vbox1, "LAYOUT_SIZE_HEXPANDING")
		# <advc.076> Un-commented, layout flag added b/c there was 0 space to the right of the separator. Now the separator itself gets streched a bit. Oh well.
		tab.attachVSeparator(hbox, "GameHBoxSeparator")
		tab.setLayoutFlag("GameHBoxSeparator", "LAYOUT_SIZE_HEXPANDING")
		vbox2 = "GameVBox2"
		# </advc.076>
		tab.attachVBox(hbox, vbox2)
		tab.setLayoutFlag(vbox2, "LAYOUT_SIZE_HEXPANDING")
		
		# <advc.076>
	# Left column
		self.attachHeading(tab, vbox1, "GENCONTROL")
		self.attachPlayerOption(tab, vbox1, PlayerOptionTypes.PLAYEROPTION_WAIT_END_TURN)
		self.attachPlayerOption(tab, vbox1, PlayerOptionTypes.PLAYEROPTION_MINIMIZE_POP_UPS)
		self.attachHeading(tab, vbox1, "UNITCONTROL")
		self.attachPlayerOption(tab, vbox1, PlayerOptionTypes.PLAYEROPTION_NO_UNIT_CYCLING)
		self.attachPlayerOption(tab, vbox1, PlayerOptionTypes.PLAYEROPTION_RIGHT_CLICK_MENU)
		self.attachPlayerOption(tab, vbox1, PlayerOptionTypes.PLAYEROPTION_STACK_ATTACK)
		self.attachPlayerOption(tab, vbox1, PlayerOptionTypes.PLAYEROPTION_QUICK_ATTACK)
		self.attachHeading(tab, vbox1, "RIVALS")
		self.attachPlayerOption(tab, vbox1, PlayerOptionTypes.PLAYEROPTION_QUICK_DEFENSE)
		self.attachPlayerOption(tab, vbox1, PlayerOptionTypes.PLAYEROPTION_SHOW_ENEMY_MOVES)
		self.attachPlayerOption(tab, vbox1, PlayerOptionTypes.PLAYEROPTION_SHOW_FRIENDLY_MOVES)
	# Right column
		self.attachHeading(tab, vbox2, "AUTOMATE")
		self.attachPlayerOption(tab, vbox2, PlayerOptionTypes.PLAYEROPTION_SAFE_AUTOMATION)
		self.attachPlayerOption(tab, vbox2, PlayerOptionTypes.PLAYEROPTION_LEAVE_FORESTS)
		self.attachPlayerOption(tab, vbox2, PlayerOptionTypes.PLAYEROPTION_START_AUTOMATED)
		self.attachPlayerOption(tab, vbox2, PlayerOptionTypes.PLAYEROPTION_MISSIONARIES_AUTOMATED)
		self.attachPlayerOption(tab, vbox2, PlayerOptionTypes.PLAYEROPTION_AUTO_PROMOTION)
		self.attachHeading(tab, vbox2, "RECOMMEND")
		self.attachPlayerOption(tab, vbox2, PlayerOptionTypes.PLAYEROPTION_NO_UNIT_RECOMMENDATIONS)
		self.attachPlayerOption(tab, vbox2, PlayerOptionTypes.PLAYEROPTION_ADVISOR_HELP)
		self.attachPlayerOption(tab, vbox2, PlayerOptionTypes.PLAYEROPTION_ADVISOR_POPUPS)
		# </advc.076>
		i = 0
		for iOptionLoop in range(PlayerOptionTypes.NUM_PLAYEROPTION_TYPES):
			# <advc.076> They're all placed individually, so this loop does nothing unless someone adds new player options.
			if iOptionLoop in self.playerOptionsDone:
				continue
			# Moved to the graphics tab
			if iOptionLoop == PlayerOptionTypes.PLAYEROPTION_NUMPAD_HELP or iOptionLoop == PlayerOptionTypes.PLAYEROPTION_QUICK_MOVES:
				continue # </advc.076>
			bContinue = true
			
			if (iOptionLoop == PlayerOptionTypes.PLAYEROPTION_MODDER_1):
				if (gc.getDefineINT("USE_MODDERS_PLAYEROPTION_1") == 0):
					bContinue = false
			elif (iOptionLoop == PlayerOptionTypes.PLAYEROPTION_MODDER_2):
				if (gc.getDefineINT("USE_MODDERS_PLAYEROPTION_2") == 0):
					bContinue = false
			elif (iOptionLoop == PlayerOptionTypes.PLAYEROPTION_MODDER_3):
				if (gc.getDefineINT("USE_MODDERS_PLAYEROPTION_3") == 0):
					bContinue = false
			
			if (bContinue):
				
				szOptionDesc = gc.getPlayerOptionsInfoByIndex(iOptionLoop).getDescription()
				szHelp = gc.getPlayerOptionsInfoByIndex(iOptionLoop).getHelp()
				szCallbackFunction = "handleGameOptionsClicked"
				szWidgetName = "GameOptionCheckBox_" + str(iOptionLoop)
				#gc.getPlayer(gc.getGame().getActivePlayer()).isOption(iOptionLoop)
				bOptionOn = UserProfile.getPlayerOption(iOptionLoop)
				#if ((i+1) <= (PlayerOptionTypes.NUM_PLAYEROPTION_TYPES+1)/2): 
				#	vbox = vbox1
				#else: 
				#	vbox = vbox2
				vbox = vbox2 # advc.076: Put any new options in the right column
				tab.attachCheckBox(vbox, szWidgetName, szOptionDesc, self.callbackIFace, szCallbackFunction, szWidgetName, bOptionOn)
				tab.setToolTip(szWidgetName, szHelp)
				i += 1
				
		tab.attachSpacer("GamePanelCenter")

		tab.attachHBox("GamePanelCenter", "LangHBox")
		
		# Languages Dropdown
		tab.attachLabel("LangHBox", "LangLabel", localText.getText("TXT_KEY_OPTIONS_SCREEN_LANGUAGE", ()))
		szDropdownDesc = "LanguagesDropdownBox"

		tab.attachSpacer("LangHBox")
		
		aszDropdownElements = ()
		for i in range(CvGameText().getNumLanguages()):
			szKey = "TXT_KEY_LANGUAGE_%d" % i
			aszDropdownElements = aszDropdownElements + (localText.getText(szKey, ()),)
					
		szCallbackFunction = "handleLanguagesDropdownBoxInput"
		szWidgetName = "LanguagesDropdownBox"
		iInitialSelection = CyGame().getCurrentLanguage()
		# <advc.076> (Not sure if this precaution could ever be needed)
		if iInitialSelection >= CvGameText().getNumLanguages():
			iInitialSelection = 0 # </advc.076>
		tab.attachDropDown("LangHBox", szWidgetName, szDropdownDesc, aszDropdownElements, self.callbackIFace, szCallbackFunction, szWidgetName, iInitialSelection)
		tab.setLayoutFlag(szWidgetName, "LAYOUT_LEFT")
		
		# K-Mod: BUG options button. Unfortunately, the BUG options screen doesn't work from the main menu.
		if CyGame().isFinalInitialized():
			tab.attachVSeparator("LangHBox", "LangHBoxSeparator")
			tab.attachVBox("LangHBox", "KmodBox")
			tab.setLayoutFlag("KmodBox", "LAYOUT_SIZE_HEXPANDING")
			tab.setLayoutFlag("KmodBox", "LAYOUT_HCENTER")
			szOptionDesc = localText.getText("TXT_KEY_BUG_OPT_TITLE", ())
			szWidgetName = "BugOptionsButton"
			tab.attachButton("KmodBox", szWidgetName, szOptionDesc, "CvScreensInterface", "showBugOptionsScreen", szWidgetName)
		# K-Mod end

		########## Lower Panel

		tab.attachHSeparator("GameVBox", "GameExitSeparator")
		
		tab.attachHBox("GameVBox", "LowerHBox")
		tab.setLayoutFlag("LowerHBox", "LAYOUT_HCENTER")
		
		szOptionDesc = localText.getText("TXT_KEY_OPTIONS_RESET", ())
		szCallbackFunction = "handleGameReset"
		szWidgetName = "GameOptionsResetButton"
		tab.attachButton("LowerHBox", szWidgetName, szOptionDesc, self.callbackIFace, szCallbackFunction, szWidgetName)
		# advc.076:
		tab.setToolTip(szWidgetName, localText.getText("TXT_KEY_OPTIONS_SCREEN_RESET1_HELP", (UserProfile.getProfileName(),)))
		# advc.076: Moved into subroutine
		self.attachExitButton(tab)
		
	def drawGraphicOptionsTab(self):
		
		tab = self.pTabControl
		
		tab.attachVBox("GraphicsForm", "GraphicsVBox")		
					
		tab.attachScrollPanel("GraphicsVBox", "GraphicsPanel")
		tab.setLayoutFlag("GraphicsPanel", "LAYOUT_SIZE_HEXPANDING")
		tab.setLayoutFlag("GraphicsPanel", "LAYOUT_SIZE_VEXPANDING")
		
		tab.attachHBox("GraphicsPanel", "GraphicsPanelHBox")
		tab.setLayoutFlag("GraphicsPanelHBox", "LAYOUT_SIZE_HPREFERREDEXPANDING")
		tab.setLayoutFlag("GraphicsPanelHBox", "LAYOUT_SIZE_VPREFERREDEXPANDING")
		
		
		####### RESOLUTION
		
		tab.attachVBox("GraphicsPanelHBox", "ResVBox")
		tab.setLayoutFlag("ResVBox", "LAYOUT_SIZE_HEXPANDING")
		tab.setLayoutFlag("ResVBox", "LAYOUT_SIZE_VEXPANDING")
		
		tab.attachPanel("ResVBox", "ResPanel")
		tab.setStyle("ResPanel", "Panel_Tan15_Style")
		tab.setLayoutFlag("ResPanel", "LAYOUT_SIZE_HEXPANDING")
		tab.setLayoutFlag("ResPanel", "LAYOUT_SIZE_VEXPANDING")

		hbox = "ResPanelHBox"
		tab.attachHBox("ResPanel", hbox)
		tab.setLayoutFlag(hbox, "LAYOUT_SIZE_HEXPANDING")
		tab.setLayoutFlag(hbox, "LAYOUT_SIZE_VEXPANDING")

	
		vbox = "ResPanelVBox"
		tab.attachVBox(hbox, vbox)
		tab.setLayoutFlag(vbox, "LAYOUT_SIZE_HEXPANDING")
		tab.setLayoutFlag(vbox, "LAYOUT_SIZE_VEXPANDING")

		# Screen Image
		tab.attachPanel(vbox, "ResScreenPanel")
		tab.setLayoutFlag("ResScreenPanel", "LAYOUT_SIZE_HEXPANDING")
		tab.setStyle("ResScreenPanel", "Panel_Black25_Style")
		
		tab.attachHBox(vbox, "ResHBox")

		vbox1 = "ResVBox1"
		vbox2 = "ResVBox2"
		tab.attachVBox("ResHBox", vbox1)
		tab.attachVBox("ResHBox", vbox2)
		
		# <advc.076>
		# Fullscreen Checkbox
		self.attachGraphicsOption(tab, vbox1, GraphicOptionTypes.GRAPHICOPTION_FULLSCREEN)
		self.attachVSpace(tab, vbox2)
		# </advc.076>
		# Screen Resolution Dropdown
		tab.attachLabel(vbox1, "ResLabel", localText.getText("TXT_KEY_OPTIONS_SCREEN_RES", ()))
		tab.setControlFlag("ResLabel", "CF_LABEL_DEFAULTSIZE")
		szDropdownDesc = "ResolutionDropdownBox"
		aszDropdownElements = ()
		for iResLoop in range(UserProfile.getResolutionMaxModes()):
			aszDropdownElements = aszDropdownElements + (UserProfile.getResolutionString(iResLoop),)
		szCallbackFunction = "handleResolutionDropdownInput"
		szWidgetName = self.szResolutionComboBoxName = "ResolutionDropdownBox"
		iInitialSelection = UserProfile.getResolution()
		tab.attachDropDown(vbox2, szWidgetName, szDropdownDesc, aszDropdownElements, self.callbackIFace, szCallbackFunction, szWidgetName, iInitialSelection)
		# advc.076:
		tab.setToolTip(szWidgetName, localText.getText("TXT_KEY_OPTIONS_SCREEN_RES_HELP", ()))
		
		# advc.137: Replaced TXT_KEY_SEALEVEL_HIGH with TXT_KEY_HIGH
		# and ditto for MEDIUM and LOW. Also under Render Quality, Globe View
		# and Movies. Cleaner this way.
		# Graphics Level
		#tab.attachLabel(vbox1, "GraphicsLevelLabel", localText.getText("TXT_KEY_OPTIONS_SCREEN_GRAPHICS_LEVEL", ()))
		#tab.setControlFlag("GraphicsLevelLabel", "CF_LABEL_DEFAULTSIZE")
		#szDropdownDesc = "GraphicsLevelDropdownBox"
		#aszDropdownElements = (localText.getText("TXT_KEY_HIGH", ()), localText.getText("TXT_KEY_MEDIUM", ()), localText.getText("TXT_KEY_LOW", ()))
		#szCallbackFunction = "handleGraphicsLevelDropdownBoxInput"
		#szWidgetName = szDropdownDesc
		#iInitialSelection = UserProfile.getGraphicsLevel()
		#tab.attachDropDown(vbox2, szWidgetName, szDropdownDesc, aszDropdownElements, self.callbackIFace, szCallbackFunction, szWidgetName, iInitialSelection)
		# advc.076: Disable GraphicsLevel dropdown. That menu only changes other settings and has no effect of its own - I think. Since I'm not perfectly sure, I'd like to force "high" (0), but if a player doesn't already have it at high, this will cause other settings to change when the player enters the options screen; can't have that.
		#if UserProfile.getGraphicsLevel() != 0:
		#	UserProfile.setGraphicsLevel(0)
		
		# Render Quality level
		tab.attachLabel(vbox1, "GraphicsQualityLabel", localText.getText("TXT_KEY_OPTIONS_SCREEN_RENDER_QUALITY_LEVEL", ()))
		tab.setControlFlag("GraphicsQualityLabel", "CF_LABEL_DEFAULTSIZE")
		szDropdownDesc = "RenderQualityDropdownBox"
		aszDropdownElements = (localText.getText("TXT_KEY_HIGH", ()), localText.getText("TXT_KEY_MEDIUM", ()), localText.getText("TXT_KEY_LOW", ()))
		szCallbackFunction = "handleRenderQualityDropdownBoxInput"
		szWidgetName = self.szRenderQualityDropdownBoxName = "RenderQualityDropdownBox"
		iInitialSelection = UserProfile.getRenderQualityLevel()
		tab.attachDropDown(vbox2, szWidgetName, szDropdownDesc, aszDropdownElements, self.callbackIFace, szCallbackFunction, szWidgetName, iInitialSelection)
		# advc.076:
		tab.setToolTip(szWidgetName, localText.getText("TXT_KEY_OPTIONS_SCREEN_RENDER_QUALITY_LEVEL_HELP", ()))
		
		# Anti-Aliasing Dropdown  advc.076: Switched places with RenderQuality
		tab.attachLabel(vbox1, "AALabel", localText.getText("TXT_KEY_OPTIONS_ANTIALIAS", ()))
		tab.setControlFlag("AALabel", "CF_LABEL_DEFAULTSIZE")
		szDropdownDesc = "AntiAliasingDropdownBox"
		aszDropdownElements = ()
		for iAALoop in range(UserProfile.getAntiAliasingMaxMultiSamples()+1):
			if (iAALoop == 0):
				aszDropdownElements = aszDropdownElements + (u"0",)
			elif (iAALoop == 1):
				aszDropdownElements = aszDropdownElements + (u"2",)
			elif (iAALoop == 2):
				aszDropdownElements = aszDropdownElements + (u"4",)
			elif (iAALoop == 3):
				aszDropdownElements = aszDropdownElements + (u"8",)
			elif (iAALoop == 4):
				aszDropdownElements = aszDropdownElements + (u"16",)
			
		szCallbackFunction = "handleAntiAliasingDropdownInput"
		szWidgetName = "AntiAliasingDropdownBox"
		iInitialSelection = UserProfile.getAntiAliasing()
		tab.attachDropDown(vbox2, szWidgetName, szDropdownDesc, aszDropdownElements, self.callbackIFace, szCallbackFunction, szWidgetName, iInitialSelection)
		tab.setLayoutFlag(szWidgetName, "LAYOUT_LEFT")
		# <advc.076>
		tab.setToolTip(szWidgetName, localText.getText("TXT_KEY_OPTIONS_SCREEN_ANTIALIAS_HELP", ()))
		# Terrain Quality Checkboxes
		self.attachGraphicsOption(tab, vbox1, GraphicOptionTypes.GRAPHICOPTION_HIRES_TERRAIN)
		self.attachGraphicsOption(tab, vbox2, GraphicOptionTypes.GRAPHICOPTION_LOWRES_TEXTURES)
		# </advc.076>
		
		# Globe view rendering level
		tab.attachLabel(vbox1, "GlobeViewLabel", localText.getText("TXT_KEY_OPTIONS_SCREEN_GLOBE", ()))
		tab.setControlFlag("GlobeViewLabel", "CF_LABEL_DEFAULTSIZE")
				
		szDropdownDesc = "GlobeViewDropdownBox"
		aszDropdownElements = (localText.getText("TXT_KEY_HIGH", ()), localText.getText("TXT_KEY_MEDIUM", ()), localText.getText("TXT_KEY_LOW", ()))
		szCallbackFunction = "handleGlobeViewDropdownBoxInput"
		szWidgetName = self.szGlobeViewDropdownBoxName = "GlobeViewDropdownBox"
		iInitialSelection = UserProfile.getGlobeViewRenderLevel()
		tab.attachDropDown(vbox2, szWidgetName, szDropdownDesc, aszDropdownElements, self.callbackIFace, szCallbackFunction, szWidgetName, iInitialSelection)
		# <advc.076>
		tab.setToolTip(szWidgetName, localText.getText("TXT_KEY_OPTIONS_SCREEN_GLOBE_HELP", ()))
		# No Buildings in Globe View Checkbox
		self.attachGraphicsOption(tab, vbox1, GraphicOptionTypes.GRAPHICOPTION_GLOBE_VIEW_BUILDINGS_DISABLED)
		# No Movies Checkbox
		self.attachGraphicsOption(tab, vbox2, GraphicOptionTypes.GRAPHICOPTION_NO_MOVIES)
		# ^It would make more sense to place this _under_ the Movie Quality dropdown menu, but the layout looks weird when it alternates between a dropdown menu and a single checkbox.
		# </advc.076>
		
		# Movies
		tab.attachLabel(vbox1, "MovieLabel", localText.getText("TXT_KEY_GRAPHICS_SETTINGS_MOVIE_QUALITY", ()))
		tab.setControlFlag("MovieLabel", "CF_LABEL_DEFAULTSIZE")
				
		szDropdownDesc = "MovieDropdownBox"
		aszDropdownElements = (localText.getText("TXT_KEY_HIGH", ()), localText.getText("TXT_KEY_MEDIUM", ()), localText.getText("TXT_KEY_LOW", ()))
		szCallbackFunction = "handleMovieDropdownBoxInput"
		szWidgetName = self.szMovieDropdownBoxName = "MovieDropdownBox"
		iInitialSelection = UserProfile.getMovieQualityLevel()
		tab.attachDropDown(vbox2, szWidgetName, szDropdownDesc, aszDropdownElements, self.callbackIFace, szCallbackFunction, szWidgetName, iInitialSelection)
		# advc.076:
		tab.setToolTip(szWidgetName, localText.getText("TXT_KEY_OPTIONS_SCREEN_MOVIE_QUALITY_HELP", ()))

		# Main menu
		tab.attachLabel(vbox1, "MainMenuLabel", localText.getText("TXT_KEY_OPENING_MENU", ()))
		tab.setControlFlag("MainMenuLabel", "CF_LABEL_DEFAULTSIZE")
				
		szDropdownDesc = "MainMenuDropdownBox"
		aszDropdownElements = ()
		for iMainMenuLoop in range(gc.getNumMainMenus()):
			aszDropdownElements = aszDropdownElements + (gc.getMainMenus(iMainMenuLoop).getDescription(),)
		szCallbackFunction = "handleMainMenuDropdownBoxInput"
		szWidgetName = self.szMainMenuDropdownBoxName = "DropdownBox"
		iInitialSelection = UserProfile.getMainMenu()
		# <advc.076> To avoid problems for mod-mods with a single custom menu background
		if iInitialSelection >= gc.getNumMainMenus():
			iInitialSelection = 0 # </advc.076>
		tab.attachDropDown(vbox2, szWidgetName, szDropdownDesc, aszDropdownElements, self.callbackIFace, szCallbackFunction, szWidgetName, iInitialSelection)
		# advc.076:
		tab.setToolTip(szWidgetName, localText.getText("TXT_KEY_OPTIONS_SCREEN_OPENING_MENU_HELP", ()))

		####### GAME GRAPHICS

		tab.attachVSeparator(hbox, "GfxSeparator")
		# advc.076: Was LAYOUT_LEFT. Want a bit more space to the right. Still not ideal: HEXPANDING has the side-effect of stretching the separator.
		tab.setLayoutFlag("GfxSeparator", "LAYOUT_SIZE_HEXPANDING")
		
		vbox = "GfxPanelVBox"
		tab.attachVBox(hbox, vbox)
		tab.setLayoutFlag(vbox, "LAYOUT_SIZE_HEXPANDING")
		tab.setLayoutFlag(vbox, "LAYOUT_SIZE_VEXPANDING")
		tab.setLayoutFlag(vbox, "LAYOUT_SPACING_NONE")
		
		# Checkboxes
		# <advc.076>
		self.attachHeading(tab, vbox, "ANIMATIONS")
		self.attachGraphicsOption(tab, vbox, GraphicOptionTypes.GRAPHICOPTION_FROZEN_ANIMATIONS)
		self.attachGraphicsOption(tab, vbox, GraphicOptionTypes.GRAPHICOPTION_EFFECTS_DISABLED)
		self.attachGraphicsOption(tab, vbox, GraphicOptionTypes.GRAPHICOPTION_NO_COMBAT_ZOOM)
		self.attachPlayerOption(tab, vbox, PlayerOptionTypes.PLAYEROPTION_QUICK_MOVES, True) # Moved from first tab
		self.attachHeading(tab, vbox, "UNITGFX")
		self.attachGraphicsOption(tab, vbox, GraphicOptionTypes.GRAPHICOPTION_SINGLE_UNIT_GRAPHICS)
		self.attachGraphicsOption(tab, vbox, GraphicOptionTypes.GRAPHICOPTION_HEALTH_BARS)
		self.attachHeading(tab, vbox, "VISAIDS")
		self.attachGraphicsOption(tab, vbox, GraphicOptionTypes.GRAPHICOPTION_NO_ENEMY_GLOW)
		self.attachPlayerOption(tab, vbox, PlayerOptionTypes.PLAYEROPTION_NUMPAD_HELP, True) # Moved from first tab
		self.attachGraphicsOption(tab, vbox, GraphicOptionTypes.GRAPHICOPTION_CITY_RADIUS)
		self.attachGraphicsOption(tab, vbox, GraphicOptionTypes.GRAPHICOPTION_CITY_DETAIL)

		for iOptionLoop in range(GraphicOptionTypes.NUM_GRAPHICOPTION_TYPES):
			# <advc.076> They're all placed individually, so this loop does nothing unless someone adds new graphics options. (There's no room left, so let's hope not.)
			if not iOptionLoop in self.graphicOptionsDone:
				# Code moved into subroutine
				self.attachGraphicsOption(tab, vbox, iOptionLoop) # </advc.076>
			
		########## EXIT

		tab.attachHSeparator("GraphicsVBox", "GraphicsExitSeparator")
		
		tab.attachHBox("GraphicsVBox", "LowerHBox")
		tab.setLayoutFlag("LowerHBox", "LAYOUT_HCENTER")
		
		szOptionDesc = localText.getText("TXT_KEY_OPTIONS_RESET", ())
		szCallbackFunction = "handleGraphicsReset"
		szWidgetName = "GraphicOptionsResetButton"
		tab.attachButton("LowerHBox", szWidgetName, szOptionDesc, self.callbackIFace, szCallbackFunction, szWidgetName)
		# advc.076:
		tab.setToolTip(szWidgetName, localText.getText("TXT_KEY_OPTIONS_SCREEN_RESET2_HELP", (UserProfile.getProfileName(),)))
		# advc.076: Moved into subroutine
		self.attachExitButton(tab)

	# <advc.076> 
	def attachGraphicsOption(self, tab, panel, iOption, bAsPlayerOption=False):
		self.attachOption(tab, panel, iOption, False, bAsPlayerOption)
		
	def attachPlayerOption(self, tab, panel, iOption, bAsGfxOption=False):
		self.attachOption(tab, panel, iOption, True, bAsGfxOption)

	# Mix of the code from the "Checkboxes" loop in drawGraphicOptionsTab and drawGameOptionsTab
	def attachOption(self, tab, panel, iOption, bPlayerOption, bWrongTab):
		optionInfo = None
		if bPlayerOption:
			optionInfo = gc.getPlayerOptionsInfoByIndex(iOption)
		else:
			optionInfo = gc.getGraphicOptionsInfoByIndex(iOption)
		szOptionDesc = optionInfo.getDescription()
		szHelp = optionInfo.getHelp()
		iOption = int(iOption) # So that str doesn't return the enum name
		szCallbackFunction = ""
		szWidgetName = ""
		bOptionOn = False
		tabOptions = None
		if bPlayerOption:
			szCallbackFunction = "handleGameOptionsClicked"
			szWidgetName = "GameOptionCheckBox_"
			bOptionOn = UserProfile.getPlayerOption(iOption)
			self.playerOptionsDone.add(iOption)
			if bWrongTab:
				tabOptions = self.tab2Options
			else:
				tabOptions = self.tab1Options
		else:
			szCallbackFunction = "handleGraphicOptionsClicked"
			szWidgetName = "GraphicOptionCheckbox_"
			bOptionOn = UserProfile.getGraphicOption(iOption)
			self.graphicOptionsDone.add(iOption)
			if bWrongTab:
				tabOptions = self.tab1Options
			else:
				tabOptions = self.tab2Options
		tabOptions.add((iOption,bPlayerOption))
		szWidgetName += str(iOption)
		tab.attachCheckBox(panel, szWidgetName, szOptionDesc, self.callbackIFace, szCallbackFunction, szWidgetName, bOptionOn)
		tab.setToolTip(szWidgetName, szHelp)
	
	def attachVSpace(self, tab, panel):
		szWidgetName = "blank" + str(self.iBlank)
		self.iBlank += 1
		tab.attachLabel(panel, szWidgetName, "")
		tab.setControlFlag(szWidgetName, "CF_LABEL_DEFAULTSIZE")
		
	def attachHeading(self, tab, panel, txtKey):
		szWidgetName = "heading" + str(self.iHeading)
		self.iHeading += 1
		tab.attachLabel(panel, szWidgetName, localText.getText("TXT_KEY_OPTIONS_SCREEN_HEADING_" + txtKey, ()) + ":")
		tab.setControlFlag(szWidgetName, "CF_LABEL_DEFAULTSIZE")
	# </advc.076>
		
	def drawAudioOptionsTab(self):
		
		tab = self.pTabControl
		
		tab.attachVBox("AudioForm", "AudioVBox")		
					
		tab.attachScrollPanel("AudioVBox", "AudioPanel")
		tab.setLayoutFlag("AudioPanel", "LAYOUT_SIZE_HEXPANDING")
		tab.setLayoutFlag("AudioPanel", "LAYOUT_SIZE_VEXPANDING")
		
		tab.attachVBox("AudioPanel", "AudioPanelVBox")
		tab.setLayoutFlag("AudioPanelHBox", "LAYOUT_SPACING_FORM")
		tab.setLayoutFlag("AudioPanelHBox", "LAYOUT_SIZE_HEXPANDING")
		tab.setLayoutFlag("AudioPanelHBox", "LAYOUT_SIZE_VEXPANDING")
	
			
		######################### Create the 6 volume slider/checkboxes #########################
		
		tab.attachVBox("AudioPanelVBox", "VolumeVBox")
		tab.setLayoutFlag("VolumeVBox", "LAYOUT_SIZE_HEXPANDING")
		tab.setLayoutFlag("VolumeVBox", "LAYOUT_SIZE_VEXPANDING")
		
		#tab.attachLabel("VolumeVBox", "VolumeLabel", "VOLUME")

		tab.attachPanel("VolumeVBox", "VolumePanel")
		tab.setStyle("VolumePanel", "Panel_Tan15_Style")
		tab.setLayoutFlag("VolumePanel", "LAYOUT_SIZE_HEXPANDING")
		tab.setLayoutFlag("VolumePanel", "LAYOUT_SIZE_VEXPANDING")

		tab.attachVBox("VolumePanel", "VolumePanelVBox")
		tab.setLayoutFlag("VolumePanelVBox", "LAYOUT_SIZE_HEXPANDING")
		tab.setLayoutFlag("VolumePanelVBox", "LAYOUT_SIZE_VEXPANDING")

		tab.attachScrollPanel("VolumePanelVBox", "VolumeScrollPanel")
		tab.setLayoutFlag("VolumeScrollPanel", "LAYOUT_SIZE_HEXPANDING")
		tab.setLayoutFlag("VolumeScrollPanel", "LAYOUT_SIZE_VEXPANDING")
		
		tab.attachHBox("VolumeScrollPanel", "VolumePanelHBox")
		tab.setLayoutFlag("VolumePanelHBox", "LAYOUT_HEVENSTRETCH")
		tab.setLayoutFlag("VolumePanelHBox", "LAYOUT_SIZE_VEXPANDING")
		
		for iWidgetNum in range(6):
						
			# SLIDER
			
			if (iWidgetNum == 0):		# Master Volume
				szWidgetDesc = localText.getText("TXT_KEY_OPTIONS_MASTERVOLUME", ())
				iInitialVal = 20-UserProfile.getMasterVolume()-1
				bNoSoundTrue = UserProfile.isMasterNoSound()
			elif (iWidgetNum == 1):		# Music Volume
				szWidgetDesc = localText.getText("TXT_KEY_OPTIONS_MUSICVOLUME", ())
				iInitialVal = 20-UserProfile.getMusicVolume()-1
				bNoSoundTrue = UserProfile.isMusicNoSound()
			elif (iWidgetNum == 2):		# Sound Effects Volume
				szWidgetDesc = localText.getText("TXT_KEY_OPTIONS_EFFECTSVOLUME", ())
				iInitialVal = 20-UserProfile.getSoundEffectsVolume()-1
				bNoSoundTrue = UserProfile.isSoundEffectsNoSound()
			elif (iWidgetNum == 3):		# Speech Volume
				szWidgetDesc = localText.getText("TXT_KEY_OPTIONS_SPEECHVOLUME", ())
				iInitialVal = 20-UserProfile.getSpeechVolume()-1
				bNoSoundTrue = UserProfile.isSpeechNoSound()
			elif (iWidgetNum == 4):		# Ambience Volume
				szWidgetDesc = localText.getText("TXT_KEY_OPTIONS_AMBIENCEVOLUME", ())
				iInitialVal = 20-UserProfile.getAmbienceVolume()-1
				bNoSoundTrue = UserProfile.isAmbienceNoSound()
			elif (iWidgetNum == 5):		# Interface Volume
				szWidgetDesc = localText.getText("TXT_KEY_OPTIONS_INTERFACEVOLUME", ())
				iInitialVal = 20-UserProfile.getInterfaceVolume()-1
				bNoSoundTrue = UserProfile.isInterfaceNoSound()
			
			islider = str(iWidgetNum)
			
			vbox = "VolumeSliderVBox"+islider
			tab.attachVBox("VolumePanelHBox", vbox)
			
			# Volume Slider
			szSliderDesc = szWidgetDesc
			szWidgetName = "VolumeSliderLabel"+islider
			tab.attachLabel(vbox, szWidgetName, szSliderDesc)
			tab.setLayoutFlag(szWidgetName, "LAYOUT_HCENTER")
			
			szCallbackFunction = "handleVolumeSlidersInput"
			szWidgetName = "VolumeSlider_" + str(iWidgetNum)
			iMin = 0
			iMax = UserProfile.getVolumeStops()
			# iInitialVal set above
			tab.attachVSlider(vbox, szWidgetName, self.callbackIFace, szCallbackFunction, szWidgetName, iMin, iMax, iInitialVal)
			tab.setLayoutFlag(szWidgetName, "LAYOUT_SIZE_VEXPANDING")
			tab.setControlFlag(szWidgetName, "CF_SLIDER_FILL_DOWN")
			
			# CHECKBOX
			# <advc.076>
			szDescKey = ""
			if iWidgetNum == 0: # To make the overall sound switch stand out more
				szDescKey = "TXT_KEY_OPTIONS_NO_SOUND"
			else:
				szDescKey = "TXT_KEY_OPTIONS_MUTE" # </advc.076>
			szOptionDesc = localText.getText(szDescKey, ())
			szCallbackFunction = "handleVolumeCheckboxesInput"
			szWidgetName = "VolumeNoSoundCheckbox_" + str(iWidgetNum)
			# bNoSoundTrue set above
			tab.attachCheckBox(vbox, szWidgetName, szOptionDesc, self.callbackIFace, szCallbackFunction, szWidgetName, bNoSoundTrue)
			tab.setLayoutFlag(szWidgetName, "LAYOUT_HCENTER")

			
		tab.attachHSeparator("VolumePanelVBox", "SoundSeparator")
				
		tab.attachHBox("VolumePanelVBox", "SoundPanelHBox")
		tab.setLayoutFlag("SoundPanelHBox", "LAYOUT_SIZE_HPREFERREDEXPANDING")
		tab.setLayoutFlag("SoundPanelHBox", "LAYOUT_SIZE_VPREFERRED")
								
		######################### Speaker Config Dropdown #########################

		tab.attachVBox("SoundPanelHBox", "SoundConfigVBox")
		# <advc.076>
		# Die, advertisement!
		#tab.attachImage("SoundConfigVBox", "SoundBlasterLogo", CyArtFileMgr().getMiscArtInfo("SOUND_BLASTER_LOGO").getPath())
		# New H-box so that the speaker dropdown appears in the same line as its label
		tab.attachHBox("SoundConfigVBox", "SpeakerConfigHBox")
		tab.setLayoutFlag("SpeakerConfigHBox", "LAYOUT_SIZE_HEXPANDING")
		# Was attached to SoundConfigVBox
		tab.attachLabel("SpeakerConfigHBox", "SpeakerConfigLabel", localText.getText("TXT_KEY_OPTIONS_SPEAKERS", ()))
		# </advc.076>
		szDropdownDesc = "SpeakerConfigDropdownBox"
		aszDropdownElements = ()
		iCharLimit = 27 # advc.076, advc.002b
		iInitialSelection = 0
		for iSpeakerConfigLoop in range(15):
			szActiveConfigKey = UserProfile.getSpeakerConfigFromList(iSpeakerConfigLoop)
			szDesc = localText.getText(szActiveConfigKey, ())
			# <advc.076> Truncate long device names
			if len(szDesc) > iCharLimit:
				szDesc = szDesc[:iCharLimit-3] + '...'
			aszDropdownElements += (szDesc,)
			# </advc.076>
			#aszDropdownElements = aszDropdownElements + (szActiveConfig,)
			if (UserProfile.getSpeakerConfig() == szActiveConfigKey):
				iInitialSelection = iSpeakerConfigLoop
			
		szCallbackFunction = "handleSpeakerConfigDropdownInput"
		szWidgetName = "SpeakerConfigDropdownBox"
		# iInitialSelection set above
		# advc.076: Was attached to SoundConfigVBox
		tab.attachDropDown("SpeakerConfigHBox", szWidgetName, szDropdownDesc, aszDropdownElements, self.callbackIFace, szCallbackFunction, szWidgetName, iInitialSelection)
		tab.setLayoutFlag(szWidgetName, "LAYOUT_SIZE_HFIXEDEXPANDING")
		tab.setLayoutFlag(szWidgetName, "LAYOUT_LEFT")
			
		######################### Custom Audio Path #########################
		# advc.076: Commented out
		#tab.attachHSeparator("SoundConfigVBox", "SoundSeparator")

		tab.attachHBox("SoundConfigVBox", "CustomPanelHBox")
		tab.setLayoutFlag("CustomPanelHBox", "LAYOUT_SIZE_HPREFERREDEXPANDING")
		tab.setLayoutFlag("CustomPanelHBox", "LAYOUT_SIZE_VPREFERRED")
										
		# Checkbox
		szOptionDesc = localText.getText("TXT_KEY_OPTIONS_CUSTOM_MUSIC", ())
		szCallbackFunction = "handleCustomMusicPathCheckboxInput"
		self.szCustomMusicCheckboxName = "CustomMusicPathCheckbox"
		szWidgetName = CvUtil.convertToStr(self.szCustomMusicCheckboxName)
		bUseCustomMusicPath = false
		if (UserProfile.getMusicPath() != ""):
			bUseCustomMusicPath = true
		# <advc.076> New V-box so that there is a newline after the CustomMusic checkbox. (Maybe one of the outer boxes could be removed now.)
		tab.attachVBox("CustomPanelHBox", "CustomMusicVBox")
		# These two were attached to CustomPanelHBox
		tab.attachCheckBox("CustomMusicVBox", szWidgetName, szOptionDesc, self.callbackIFace, szCallbackFunction, szWidgetName, bUseCustomMusicPath)
		tab.attachHBox("CustomMusicVBox", "AudioPathHBox")
		# </advc.076>
		tab.setLayoutFlag("AudioPathHBox", "LAYOUT_SIZE_HFIXEDEXPANDING")
		
		# Browse Button
		szOptionDesc = localText.getText("TXT_KEY_OPTIONS_BROWSE", ())
		szCallbackFunction = "handleCustomMusicPathButtonInput"
		szWidgetName = "CustomMusicPathButton"
		tab.attachButton("AudioPathHBox", szWidgetName, szOptionDesc, self.callbackIFace, szCallbackFunction, szWidgetName)

		# Edit Box
		szEditBoxDesc = u""
		if (UserProfile.getMusicPath() != ""):
			szEditBoxDesc = CvUtil.convertToUnicode(UserProfile.getMusicPath())
		szWidgetName = "CustomMusicEditBox"
		szCallbackFunction = "DummyCallback"
		
		tab.attachEdit("AudioPathHBox", szWidgetName, szEditBoxDesc, self.callbackIFace, szCallbackFunction, szWidgetName)
		# advc.076: Moved along with Voice Config Section
		tab.attachVSeparator("SoundPanelHBox", "SoundVSeparator")
		
		######################### Voice Config Section #########################
		# advc.076: Swapped this section with Speaker Config/ Custom Audio
		
		tab.attachVBox("SoundPanelHBox", "VoiceVBox")

		# Checkbox
		szOptionDesc = localText.getText("TXT_KEY_OPTIONS_VOICE", ())
		szCallbackFunction = "handleVoiceCheckboxInput"
		self.szVoiceCheckboxName = "VoiceCheckbox"
		szWidgetName = "VoiceChatCheckbox"
		bUseVoice = UserProfile.useVoice()
		tab.attachCheckBox("VoiceVBox", szWidgetName, szOptionDesc, self.callbackIFace, szCallbackFunction, szWidgetName, bUseVoice)

		# Capture Device Dropdown
		tab.attachLabel("VoiceVBox", "VoiceCaptureLabel", localText.getText("TXT_KEY_OPTIONS_CAPTURE_DEVICE", ()))
		szDropdownDesc = "CaptureDeviceDropdownBox"
		aszDropdownElements = ()
		iCharLimit = 42 # advc.076, advc.002b
		for iCaptureDevice in range(UserProfile.getNumCaptureDevices()):
			# <advc.076>
			szDesc = UserProfile.getCaptureDeviceDesc(iCaptureDevice)
			if len(szDesc) > iCharLimit:
				szDesc = szDesc[:iCharLimit-3] + '...'
			aszDropdownElements += (szDesc,)
			# </advc.076>
			#aszDropdownElements = aszDropdownElements + (UserProfile.getCaptureDeviceDesc(iCaptureDevice),)
		szCallbackFunction = "handleCaptureDeviceDropdownInput"
		szWidgetName = "CaptureDeviceDropdownBox"
		iInitialSelection = UserProfile.getCaptureDeviceIndex()
		tab.attachDropDown("VoiceVBox", szWidgetName, szDropdownDesc, aszDropdownElements, self.callbackIFace, szCallbackFunction, szWidgetName, iInitialSelection)
		
		# Capture Volume Slider
		szSliderDesc = localText.getText("TXT_KEY_OPTIONS_CAPTUREVOLUME", ())
		szCallbackFunction = "handleCaptureVolumeSliderInput"
		szWidgetName = "CaptureVolumeSlider"
		iMin = 0
		iMax = UserProfile.getMaxCaptureVolume()
#		iInitialVal = iMax - UserProfile.getCaptureVolume()
		iInitialVal = UserProfile.getCaptureVolume()
		tab.attachHSlider("VoiceVBox", szWidgetName, self.callbackIFace, szCallbackFunction, szWidgetName, iMin, iMax, iInitialVal)
		tab.setControlFlag(szWidgetName, "CF_SLIDER_FILL_UP")
		
		# Playback Device Dropdown
		tab.attachLabel("VoiceVBox", "VoicePlaybackLabel", localText.getText("TXT_KEY_OPTIONS_PLAYBACK_DEVICE", ()))
		szDropdownDesc = "PlaybackDeviceDropdownBox"
		aszDropdownElements = ()
		for iPlaybackDevice in range(UserProfile.getNumPlaybackDevices()):
			# <advc.076>
			szDesc = UserProfile.getPlaybackDeviceDesc(iPlaybackDevice)
			if len(szDesc) > iCharLimit:
				szDesc = szDesc[:iCharLimit-3] + '...'
			aszDropdownElements += (szDesc,)
			# </advc.076>
			#aszDropdownElements = aszDropdownElements + (UserProfile.getPlaybackDeviceDesc(iPlaybackDevice),)
		szCallbackFunction = "handlePlaybackDeviceDropdownInput"
		szWidgetName = "PlaybackDeviceDropdownBox"
		iInitialSelection = UserProfile.getPlaybackDeviceIndex()
		tab.attachDropDown("VoiceVBox", szWidgetName, szDropdownDesc, aszDropdownElements, self.callbackIFace, szCallbackFunction, szWidgetName, iInitialSelection)
		
		# Playback Volume Slider
		szSliderDesc = localText.getText("TXT_KEY_OPTIONS_PLAYBACKVOLUME", ())
		szCallbackFunction = "handlePlaybackVolumeSliderInput"
		szWidgetName = "PlaybackVolumeSlider"
		iMin = 0
		iMax = UserProfile.getMaxPlaybackVolume()
#		iInitialVal = iMax - UserProfile.getPlaybackVolume()
		iInitialVal = UserProfile.getPlaybackVolume()
		tab.attachHSlider("VoiceVBox", szWidgetName, self.callbackIFace, szCallbackFunction, szWidgetName, iMin, iMax, iInitialVal)
		tab.setControlFlag(szWidgetName, "CF_SLIDER_FILL_UP")

		########## EXIT

		tab.attachHSeparator("AudioVBox", "AudioExitSeparator")
		
		tab.attachHBox("AudioVBox", "LowerHBox")
		tab.setLayoutFlag("LowerHBox", "LAYOUT_HCENTER")
		
		szOptionDesc = localText.getText("TXT_KEY_OPTIONS_RESET", ())
		szCallbackFunction = "handleAudioReset"
		szWidgetName = "AudioOptionsResetButton"
		tab.attachButton("LowerHBox", szWidgetName, szOptionDesc, self.callbackIFace, szCallbackFunction, szWidgetName)
		# advc.076:
		tab.setToolTip(szWidgetName, localText.getText("TXT_KEY_OPTIONS_SCREEN_RESET3_HELP", (UserProfile.getProfileName(),)))
		# advc.076: Moved into subroutine
		self.attachExitButton(tab)
		
		
	def drawOtherTab(self):

		tab = self.pTabControl

		tab.attachVBox("OtherForm", "OtherVBox")		
					
		tab.attachScrollPanel("OtherVBox", "OtherPanel")
		tab.setLayoutFlag("OtherPanel", "LAYOUT_SIZE_HEXPANDING")
		tab.setLayoutFlag("OtherPanel", "LAYOUT_SIZE_VEXPANDING")
		
		tab.attachHBox("OtherPanel", "OtherPanelHBox")
		tab.setLayoutFlag("OtherPanelHBox", "LAYOUT_SPACING_INNERFORM")
		tab.setLayoutFlag("OtherPanelHBox", "LAYOUT_SIZE_HEXPANDING")
		
		
		########### CLOCK
		
		tab.attachVBox("OtherPanelHBox", "ClockVBox")
		tab.setLayoutFlag("ClockVBox", "LAYOUT_SIZE_HEXPANDING")
		tab.setLayoutFlag("ClockVBox", "LAYOUT_SIZE_VEXPANDING")
		
		tab.attachLabel("ClockVBox", "ClockLabel", localText.getText("TXT_KEY_OPTIONS_CLOCK", ()).upper() )

		tab.attachPanel("ClockVBox", "ClockPanel")
		tab.setStyle("ClockPanel", "Panel_Tan15_Style")
		tab.setLayoutFlag("ClockPanel", "LAYOUT_SIZE_HPREFERREDEXPANDING")
		tab.setLayoutFlag("ClockPanel", "LAYOUT_SIZE_VPREFERREDEXPANDING")
		
		tab.attachVBox("ClockPanel", "ClockPanelVBox")
		tab.setLayoutFlag("ClockPanelVBox", "LAYOUT_SIZE_HPREFERREDEXPANDING")
		tab.setLayoutFlag("ClockPanelVBox", "LAYOUT_SIZE_VPREFERREDEXPANDING")
		
		# Clock On Checkbox
		szOptionDesc = localText.getText("TXT_KEY_OPTIONS_CLOCK_ON", ())
		szCallbackFunction = "handleClockOnCheckboxInput"
		szWidgetName = "ClockOnCheckbox"
		bClockOn = UserProfile.isClockOn()
		tab.attachCheckBox("ClockPanelVBox", szWidgetName, szOptionDesc, self.callbackIFace, szCallbackFunction, szWidgetName, bClockOn)
		
		# 24 Hour Clock Checkbox
		szOptionDesc = localText.getText("TXT_KEY_OPTIONS_24CLOCK", ())
		szCallbackFunction = "handle24HourClockCheckboxInput"
		szWidgetName = "24HourClockCheckbox"
		b24HourClock = UserProfile.is24Hours()
		tab.attachCheckBox("ClockPanelVBox", szWidgetName, szOptionDesc, self.callbackIFace, szCallbackFunction, szWidgetName, b24HourClock)
		
		# Edit Box Hours
		tab.attachLabel("ClockPanelVBox", "HoursLabel", localText.getText("TXT_KEY_OPTIONS_HOURS", ()))
		szEditBoxDesc = str(getAlarmHourLeft())
		szCallbackFunction = "DummyCallback"
		szWidgetName = "AlarmHourEditBox"
		tab.attachEdit("ClockPanelVBox", szWidgetName, szEditBoxDesc, self.callbackIFace, szCallbackFunction, szWidgetName)
		
		# Edit Box Mins
		tab.attachLabel("ClockPanelVBox", "MinsLabel", localText.getText("TXT_KEY_OPTIONS_MINS", ()))
		szEditBoxDesc = str(getAlarmMinLeft())
		szCallbackFunction = "DummyCallback"
		szWidgetName = "AlarmMinEditBox"
		tab.attachEdit("ClockPanelVBox", szWidgetName, szEditBoxDesc, self.callbackIFace, szCallbackFunction, szWidgetName)
		
		# Alarm On Checkbox
		szOptionDesc = localText.getText("TXT_KEY_OPTIONS_ALARMON", ())
		szCallbackFunction = "handleAlarmOnCheckboxInput"
		szWidgetName = "AlarmOnCheckbox"
		bAlarmOn = isAlarmOn()
		tab.attachCheckBox("ClockPanelVBox", szWidgetName, szOptionDesc, self.callbackIFace, szCallbackFunction, szWidgetName, bAlarmOn)
		
		
		########### PROFILE
		
		UserProfile.loadProfileFileNames()
		
		tab.attachVBox("OtherPanelHBox", "ProfileVBox")
		tab.setLayoutFlag("ProfileVBox", "LAYOUT_SIZE_HEXPANDING")
		tab.setLayoutFlag("ProfileVBox", "LAYOUT_SIZE_VEXPANDING")
		# advc.076: Txt key changed to ..._TITLE
		tab.attachLabel("ProfileVBox", "ProfileLabel", localText.getText("TXT_KEY_OPTIONS_SCREEN_PROFILES_TITLE", ()).upper() )

		tab.attachPanel("ProfileVBox", "ProfilePanel")
		tab.setStyle("ProfilePanel", "Panel_Tan15_Style")
		tab.setLayoutFlag("ProfilePanel", "LAYOUT_SIZE_HPREFERREDEXPANDING")
		tab.setLayoutFlag("ProfilePanel", "LAYOUT_SIZE_VPREFERREDEXPANDING")
		
		tab.attachVBox("ProfilePanel", "ProfilePanelVBox")
		tab.setLayoutFlag("ProfilePanelVBox", "LAYOUT_SIZE_HPREFERREDEXPANDING")
		tab.setLayoutFlag("ProfilePanelVBox", "LAYOUT_SIZE_VPREFERREDEXPANDING")


		# Profiles Dropdown
		
		tab.attachLabel("ProfilePanelVBox", "ProfileComboLabel", localText.getText("TXT_KEY_OPTIONS_SCREEN_PROFILES", ()))
		
		szDropdownDesc = "ProfilesDropdownBox"
		aszDropdownElements = ()
		iInitialSelection = 0
		for iProfileLoop in range(UserProfile.getNumProfileFiles()):
			szProfileFileName = UserProfile.getProfileFileName(iProfileLoop)
			
			# Cut off file path and extension
			szProfile = szProfileFileName[szProfileFileName.find("PROFILES\\")+9:-4]
			
			aszDropdownElements = aszDropdownElements + (szProfile,)
						
			if (UserProfile.getProfileName() == szProfile):
				iInitialSelection = iProfileLoop
			
		szCallbackFunction = "handleProfilesDropdownInput"
		szWidgetName = "ProfilesDropdownBox"
		# iInitialSelection set above
		tab.attachDropDown("ProfilePanelVBox",szWidgetName,szDropdownDesc, aszDropdownElements, self.callbackIFace, szCallbackFunction, szWidgetName, iInitialSelection)
		
		# Edit Box ProfileName
		# advc.076: Txt key was TXT_KEY_OPTIONS_SCREEN_PROFILE_NAME
		tab.attachLabel("ProfilePanelVBox","ProfilesName",localText.getText("TXT_KEY_OPTIONS_SCREEN_EDIT_PROFILE", ()))
		
		#szCallbackIFace = ""
		szEditBoxDesc = UserProfile.getProfileName()
		szCallbackFunction = "DummyCallback"
		szWidgetName = "ProfileNameEditBox"
		szWideEditBoxDesc = CvUtil.convertToUnicode(szEditBoxDesc)
		tab.attachEdit("ProfilePanelVBox", szWidgetName, szWideEditBoxDesc, self.callbackIFace, szCallbackFunction, szWidgetName)
		
		# New Profile Button
		szOptionDesc = localText.getText("TXT_KEY_OPTIONS_NEW_PROFILE", ())
		szCallbackFunction = "handleNewProfileButtonInput"
		szWidgetName = "NewProfileButton"
		tab.attachButton("ProfilePanelVBox", szWidgetName, szOptionDesc, self.callbackIFace, szCallbackFunction, szWidgetName)
		
		# Delete Profile Button
		szOptionDesc = localText.getText("TXT_KEY_OPTIONS_DELETE_PROFILE", ())
		szCallbackFunction = "handleDeleteProfileButtonInput"
		szWidgetName = "DeleteProfileButton"
		tab.attachButton("ProfilePanelVBox", szWidgetName, szOptionDesc, self.callbackIFace, szCallbackFunction, szWidgetName)
		
		
		########## NETWORKING		
		# advc.076: Remove the entire network panel
		#tab.attachVBox("OtherPanelHBox", "NetVBox")
		#tab.setLayoutFlag("NetVBox", "LAYOUT_SIZE_HEXPANDING")
		#tab.setLayoutFlag("NetVBox", "LAYOUT_SIZE_VEXPANDING")
		
		#tab.attachLabel("NetVBox", "NetLabel", localText.getText("TXT_KEY_OPTIONS_NETWORK", ()).upper() )

		#tab.attachPanel("NetVBox", "NetPanel")
		#tab.setStyle("NetPanel", "Panel_Tan15_Style")
		#tab.setLayoutFlag("NetPanel", "LAYOUT_SIZE_HPREFERREDEXPANDING")
		#tab.setLayoutFlag("NetPanel", "LAYOUT_SIZE_VPREFERREDEXPANDING")
		
		#tab.attachVBox("NetPanel", "NetPanelVBox")
		#tab.setLayoutFlag("NetPanelVBox", "LAYOUT_SIZE_HPREFERREDEXPANDING")
		#tab.setLayoutFlag("NetPanelVBox", "LAYOUT_SIZE_VPREFERREDEXPANDING")

		# Radio Buttons
		#tab.attachLabel("NetPanelVBox", "NetBandwidthLabel", localText.getText("TXT_KEY_OPTIONS_BANDWIDTH_DESC", ()) )
		
		#bIsModem = gc.getGame().isModem()
		#szCallbackFunction = "handleBroadbandSelected"
		#szWidgetName = "BroadbandSelection"
		#szWidgetLbl = localText.getText("TXT_KEY_OPTIONS_BROADBAND_LBL", ())
		#tab.attachRadioButton("NetPanelVBox", szWidgetName, szWidgetLbl, self.callbackIFace, szCallbackFunction, str(szWidgetName), (not bIsModem))
		
		#szCallbackFunction = "handleModemSelected"
		#szWidgetName = "ModemSelection"
		#szWidgetLbl = localText.getText("TXT_KEY_OPTIONS_MODEM_LBL", ())
		#tab.attachRadioButton("NetPanelVBox", szWidgetName, szWidgetLbl, self.callbackIFace, szCallbackFunction, str(szWidgetName), bIsModem)


		########## EXIT

		tab.attachHSeparator("OtherVBox", "OtherExitSeparator")
		
		tab.attachHBox("OtherVBox", "LowerHBox")
		tab.setLayoutFlag("LowerHBox", "LAYOUT_HCENTER")
		# advc.076: Don't need a reset button just for the clock options. Reset-all would make some sense. Too much work for now.
		#szOptionDesc = localText.getText("TXT_KEY_OPTIONS_RESET", ())
		#szCallbackFunction = "handleOtherReset"
		#szWidgetName = "OtherOptionsResetButton"
		#tab.attachButton("LowerHBox", szWidgetName, szOptionDesc, self.#callbackIFace, szCallbackFunction, szWidgetName)

		# advc.076: Moved into subroutine
		self.attachExitButton(tab)
	
	# advc.076: Moved here to reduce code duplication
	def attachExitButton(self, tab):
		box = "LowerHBox" # That widget name is used by all tabs
		szButtonDesc = localText.getText("TXT_KEY_PEDIA_SCREEN_EXIT", ())
		szCallbackFunction = "handleExitButtonInput"
		szWidgetName = "OptionsExitButton" + str(self.iExitButton)
		self.iExitButton += 1
		tab.attachButton(box, szWidgetName, szButtonDesc, self.callbackIFace, szCallbackFunction, szWidgetName)
		# advc.076:
		tab.setToolTip(szWidgetName, localText.getText("TXT_KEY_OPTIONS_SCREEN_EXIT_HELP", (UserProfile.getProfileName(),)))
