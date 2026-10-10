# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#

from CvPythonExtensions import *
from SASMagicNumbers import *
import CvUtil
import SevoScreenEnums
from SASFontUtils import *
import SASTextScale

from _sevopedia_helpers import *
from SevoPediaMediaPlayer import SevoPediaMediaPlayer

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class SevoPediaMusic:

	def __init__(self, main):
		self.top = main
		self.iMusic = -1
		self.MUSIC_PLAYER_SCREEN = "SevoPediaMusicPlayer"
		self.MUSIC_PLAYER_EXIT_ID = "SAS_MusicPlayerExit"
		self.mediaPlayer = SevoPediaMediaPlayer(self.MUSIC_PLAYER_SCREEN, SevoScreenEnums.PEDIA_MUSIC, self.MUSIC_PLAYER_EXIT_ID, "MusicPlayer")
		self.SAS_lastMusicSound = None
		self.SAS_playableMusic = None
		self.SAS_playableMusicLabels = None
		self.SAS_playableMusicIcons = None
		self.SAS_playableMusicIndex = -1
		self.SAS_playableMusicGroupByIndex = None
		self.SAS_playableMusicGroupLabels = None
		self.SAS_musicGroupFirstIndex = None
		self.SAS_musicGroupLastIndex = None

		# Offset for the extra width of Music items list vs base width
		I_SAS_SEVOPEDIA_MUSIC_ITEMS_WIDTH = gc.getDefineINT("SAS_SEVOPEDIA_MUSIC_ITEMS_WIDTH")
		if I_SAS_SEVOPEDIA_MUSIC_ITEMS_WIDTH <= 0:
			I_SAS_SEVOPEDIA_MUSIC_ITEMS_WIDTH = self.top.W_ITEMS
		iExtraWidth = I_SAS_SEVOPEDIA_MUSIC_ITEMS_WIDTH - self.top.W_ITEMS

		self.X_HEADER = self.top.X_PEDIA_PAGE + iExtraWidth
		self.Y_HEADER = self.top.Y_PEDIA_PAGE
		self.W_HEADER = self.top.W_PEDIA_PAGE - iExtraWidth
		self.H_HEADER = 120

		self.W_ICON = 100
		self.H_ICON = 100
		self.X_ICON = self.X_HEADER + 10
		self.Y_ICON = self.Y_HEADER + 10

		self.X_TITLE = self.X_ICON + self.W_ICON + 10
		self.Y_TITLE = self.Y_HEADER + 12

		self.W_BUTTON = 64
		self.H_BUTTON = 64
		self.X_BUTTON = self.X_HEADER + self.W_HEADER - self.W_BUTTON - 10
		self.Y_BUTTON = self.Y_HEADER + (self.H_HEADER - self.H_BUTTON) / 2
		self.playButtonPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_PLAY_BUTTON").getPath()

		self.X_HISTORY = self.X_HEADER
		self.Y_HISTORY = self.Y_HEADER + self.H_HEADER + 10
		self.W_HISTORY = self.top.W_PEDIA_PAGE - iExtraWidth
		self.H_HISTORY = self.top.B_PEDIA_PAGE - self.Y_HISTORY

	def interfaceScreen(self, iMusic):
		self.iMusic = iMusic

		self.placeHeader()
		self.placeHistory()

	def placeHeader(self):
		screen = self.top.getScreen()
		iMusicType, iMusicId = self.top.SAS_unpackMusicKey(self.iMusic)
		info = self.getMusicInfo(self.iMusic)

		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_HEADER, self.Y_HEADER, self.W_HEADER, self.H_HEADER, PanelStyles.PANEL_STYLE_BLUE50)

		szButton = ""
		if info:
			szButton = self.top.SAS_getMusicButton(self.iMusic)
		if szButton:
			if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_TECH:
				screen.setImageButton(self.top.getNextWidgetName(), szButton, self.X_ICON + self.W_ICON / 2 - PANE_ICON_SIZE / 2, self.Y_ICON + self.H_ICON / 2 - PANE_ICON_SIZE / 2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iMusicId, 1)
			elif iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_ERA:
				screen.setImageButton(self.top.getNextWidgetName(), szButton, self.X_ICON + self.W_ICON / 2 - PANE_ICON_SIZE / 2, self.Y_ICON + self.H_ICON / 2 - PANE_ICON_SIZE / 2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_PEDIA_MAIN, SevoScreenEnums.PEDIA_ERA_CHART, -1)
			elif iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_LEADER:
				iLeaderId = self.top.SAS_getMusicLeaderId(self.iMusic)
				if iLeaderId != -1:
					# <!-- custom: Music's generic leader links use the Leader page's canonical unique civilization for trait production help; literal data2=1 incorrectly meant Arabia. See KI#332. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
					screen.setImageButton(self.top.getNextWidgetName(), szButton, self.X_ICON + self.W_ICON / 2 - PANE_ICON_SIZE / 2, self.Y_ICON + self.H_ICON / 2 - PANE_ICON_SIZE / 2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_PEDIA_JUMP_TO_LEADER, iLeaderId, self.top.pediaLeader.getCiv(iLeaderId))
				else:
					screen.addDDSGFC(self.top.getNextWidgetName(), szButton, self.X_ICON + self.W_ICON / 2 - PANE_ICON_SIZE / 2, self.Y_ICON + self.H_ICON / 2 - PANE_ICON_SIZE / 2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1)
			elif iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_CIV:
				iCivId = self.top.SAS_getMusicCivId(self.iMusic)
				if iCivId != -1:
					screen.setImageButton(self.top.getNextWidgetName(), szButton, self.X_ICON + self.W_ICON / 2 - PANE_ICON_SIZE / 2, self.Y_ICON + self.H_ICON / 2 - PANE_ICON_SIZE / 2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIV, iCivId, 1)
				else:
					screen.addDDSGFC(self.top.getNextWidgetName(), szButton, self.X_ICON + self.W_ICON / 2 - PANE_ICON_SIZE / 2, self.Y_ICON + self.H_ICON / 2 - PANE_ICON_SIZE / 2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1)
			else:
				screen.addDDSGFC(self.top.getNextWidgetName(), szButton, self.X_ICON + self.W_ICON / 2 - PANE_ICON_SIZE / 2, self.Y_ICON + self.H_ICON / 2 - PANE_ICON_SIZE / 2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1)

		szTitleText = ""
		szTitleText = self.top.SAS_getMusicTitle(self.iMusic)
		szTitle = sasFontTagTitle.bold + szTitleText.upper() + SAS_FONT_TAG_CLOSE
		screen.setLabel(self.top.getNextWidgetName(), "Background", szTitle, CvUtil.FONT_LEFT_JUSTIFY, self.X_TITLE, self.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		if self.hasMusic(self.iMusic):
			screen.setImageButton(self.top.getNextWidgetName(), self.playButtonPath, self.X_BUTTON, self.Y_BUTTON, self.W_BUTTON, self.H_BUTTON, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_MUSIC_PLAY, self.iMusic)

	def placeHistory(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, "", "", True, True, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, PanelStyles.PANEL_STYLE_BLUE50)
		iMusicType, iMusicId = self.top.SAS_unpackMusicKey(self.iMusic)
		info = self.getMusicInfo(self.iMusic)
		szText = ""
		if info and iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_TECH:
			try:
				szText = info.getQuote()
			except:
				szText = ""
			if szText and szText.startswith("TXT_KEY_"):
				szText = ""
			if not szText:
				try:
					szText = info.getCivilopedia()
				except:
					szText = ""
			if szText and szText.startswith("TXT_KEY_"):
				szText = ""
		elif iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_ERA:
			szEraText = u""
			if info:
				szEraText = info.getDescription() + u" " + localText.getText("TXT_KEY_PEDIA_ERA", ())

			szExtra = u""
			if (self.top.SAS_musicEraTracks is not None) and (iMusicId >= 0) and (iMusicId < len(self.top.SAS_musicEraTracks)):
				_, iTrackId, iTrack = self.top.SAS_musicEraTracks[iMusicId]
				szTrackName = u""
				try:
					szTrackName = info.getSoundtrackScriptName(iTrack)
				except:
					szTrackName = u""
				szExtra = u"\nTrack %02d" % (iTrack + 1)
				if szTrackName:
					szExtra += u" - " + unicode(szTrackName)
				if iTrackId != -1:
					szExtra += u"\nTrack ID: %d" % iTrackId

			szText = szEraText + szExtra
		elif iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_LEADER:
			szTitleText = self.top.SAS_getMusicTitle(self.iMusic)
			iTrackId = self.top.SAS_getMusicSoundId(self.iMusic)
			if iTrackId != -1:
				szText = szTitleText + u"\nTrack ID: %d" % iTrackId
			else:
				szText = szTitleText
		elif iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_CIV:
			szTitleText = self.top.SAS_getMusicTitle(self.iMusic)
			szScript = self.top.SAS_getMusicSoundScript(self.iMusic)
			iSoundId = self.top.SAS_getMusicSoundId(self.iMusic)
			szNote = localText.getText("TXT_KEY_PEDIA_SAS_MUSIC_CIV_VARIANT_NOTE", ())
			if iSoundId != -1:
				szText = szTitleText + u"\nSound ID: %d" % iSoundId
			elif szScript:
				szText = szTitleText + u"\n" + szScript
			else:
				szText = szTitleText
			if szNote:
				szText = szText + u"\n\n" + szNote
		elif iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_SCRIPT:
			szTitleText = self.top.SAS_getMusicTitle(self.iMusic)
			szScript = self.top.SAS_getMusicSoundScript(self.iMusic)
			szSoundId = ""
			iMusicType, iMusicId = self.top.SAS_unpackMusicKey(self.iMusic)
			if (self.top.SAS_musicScriptTracks is not None) and (iMusicId >= 0) and (iMusicId < len(self.top.SAS_musicScriptTracks)):
				_, szSoundId, _ = self.top.SAS_musicScriptTracks[iMusicId]
			if szSoundId:
				if szTitleText == szSoundId:
					szText = szTitleText + u"\n" + szScript
				else:
					szText = szTitleText + u"\n" + szSoundId + u"\n" + szScript
			else:
				szText = szTitleText + u"\n" + szScript
		elif iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_SCRIPT_3D:
			szTitleText = self.top.SAS_getMusicTitle(self.iMusic)
			szScript = self.top.SAS_getMusicSoundScript(self.iMusic)
			szSoundId = ""
			iMusicType, iMusicId = self.top.SAS_unpackMusicKey(self.iMusic)
			if (self.top.SAS_musicScript3DTracks is not None) and (iMusicId >= 0) and (iMusicId < len(self.top.SAS_musicScript3DTracks)):
				_, szSoundId, _ = self.top.SAS_musicScript3DTracks[iMusicId]
			if szSoundId:
				if szTitleText == szSoundId:
					szText = szTitleText + u"\n" + szScript
				else:
					szText = szTitleText + u"\n" + szSoundId + u"\n" + szScript
			else:
				szText = szTitleText + u"\n" + szScript
			# <!-- custom: Some 3D script previews are EXE/audio-asset flaky even when Sevopedia passes the correct AS3D_ script.
			# Improvement animal scripts such as elephant empirically sometimes played no sound or another animal; ambience 3D scripts can have delayed starts, e.g. dog ambience began around 6 seconds.
			# Add focused notes only for those 3D script groups. See KI#141. (GPT-5.5) -->
			if szScript.startswith("AS3D_IMPROV"):
				szText = szText + u"\n\n" + localText.getText("TXT_KEY_PEDIA_SAS_MUSIC_3D_IMPROVEMENT_NOTE", ())
			elif szScript.startswith("AS3D_SS_"):
				szText = szText + u"\n\n" + localText.getText("TXT_KEY_PEDIA_SAS_MUSIC_3D_AMBIENCE_NOTE", ())
		szText = SASTextScale.labelText(szText)
		textName = self.top.getNextWidgetName()
		screen.addMultilineText(textName, szText, self.X_HISTORY + 10, self.Y_HISTORY + 10, self.W_HISTORY - 20, self.H_HISTORY - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def playMusic(self, iMusic):
		game = CyGame()
		if game.isNetworkMultiPlayer() or game.isPitbossHost():
			return

		if not self.hasMusic(iMusic):
			return
		self.showMusicPlayer(iMusic)

	def showMusicPlayer(self, iMusic):
		iMusicType, iMusicId = self.top.SAS_unpackMusicKey(iMusic)
		info = self.getMusicInfo(iMusic)
		if (iMusicType != SAS_MAGIC_PEDIA_MUSIC_TYPE_SCRIPT) and (iMusicType != SAS_MAGIC_PEDIA_MUSIC_TYPE_SCRIPT_3D) and (not info):
			return
		szSoundScript = self.top.SAS_getMusicSoundScript(iMusic)
		iSoundId = self.top.SAS_getMusicSoundId(iMusic)
		if szSoundScript == "NONE":
			szSoundScript = ""

		screen = self.mediaPlayer.openScreen()
		iScreenW, iScreenH, _, _, _, _ = self.mediaPlayer.setupLayout(screen, "", False)
		iPanelX, iPanelY, iPanelW, iPanelH, iImageX, iImageY, iImageW, iImageH = self.mediaPlayer.placeTvPanel(screen, iScreenW, iScreenH)
		self.mediaPlayer.placeTimerLabel(screen, iScreenW, iScreenH)
		self.mediaPlayer.setCurrentLabel(screen, self.top.SAS_getMusicTitle(iMusic))
		self.mediaPlayer.setCurrentLabel(screen, self.top.SAS_getMusicTitle(iMusic))
		self.mediaPlayer.startTimer()

		iLeaderId = -1
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_LEADER:
			iLeaderId = self.top.SAS_getMusicLeaderId(iMusic)
		if iLeaderId != -1:
			screen.addLeaderheadGFC("MusicPlayerLeaderhead", iLeaderId, AttitudeTypes.ATTITUDE_PLEASED, iImageX, iImageY, iImageW, iImageH, WidgetTypes.WIDGET_GENERAL, -1, -1)
		elif iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_CIV:
			self.placeCivDisplay(screen, iMusic, iImageX, iImageY, iImageW, iImageH)
		elif iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_TECH:
			self.placeTechDisplay(screen, iMusic, iImageX, iImageY, iImageW, iImageH)
		elif iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_SCRIPT or iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_SCRIPT_3D:
			self.placeScriptDisplay(screen, iMusic, iImageX, iImageY, iImageW, iImageH)
		else:
			iEra = self.top.SAS_getMusicEra(iMusic)
			szImagePath = ""
			if iEra != -1:
				szImagePath = gc.getEraInfo(iEra).getButton()
			if szImagePath:
				screen.setImageButton("MusicPlayerImage", szImagePath, iImageX, iImageY, iImageW, iImageH, WidgetTypes.WIDGET_PEDIA_MAIN, SevoScreenEnums.PEDIA_ERA_CHART, -1)

		# Place text panel in bottom right if available
		szText = self.getTextForMusic(iMusic)
		if szText:
			self.mediaPlayer.placeTextPanel(screen, iScreenW, iScreenH, szText)

		if szSoundScript or (iSoundId != -1):
			self.mediaPlayer.playSound(szSoundScript, iSoundId, self.top.SAS_isMusicSound3D(iMusic))

		self.mediaPlayer.placeExitButton(screen, iScreenW, iScreenH)
		self.mediaPlayer.placeReplayButton(screen, iScreenW, iScreenH)
		self.mediaPlayer.setToggleButton("SAS_EMOJI_MUSICAL_NOTES")
		self.mediaPlayer.placeToggleButton(screen, iScreenW, iScreenH)
		self.mediaPlayer.placePrevNextButtons(screen, iScreenW, iScreenH)
		self.mediaPlayer.placeGroupSkipButtons(screen, iScreenW, iScreenH)
		self.mediaPlayer.setReplayCallback(self.replayMusic)
		self.mediaPlayer.setPrevCallback(self.playPrevMusic)
		self.mediaPlayer.setNextCallback(self.playNextMusic)
		self.mediaPlayer.setPrevGroupCallback(self.playPrevMusicGroup)
		self.mediaPlayer.setNextGroupCallback(self.playNextMusicGroup)
		self.mediaPlayer.setToggleCallback(self.switchToMovies)
		self.SAS_lastMusicSound = (szSoundScript, iSoundId, self.top.SAS_isMusicSound3D(iMusic))
		self.SAS_setupPlayableMusic(iMusic)
		self.mediaPlayer.placeQueueList(screen, iScreenW, iScreenH, self.SAS_playableMusicLabels, self.SAS_playableMusicIndex, self.SAS_playableMusicGroupByIndex, self.SAS_playableMusicGroupLabels, self.SAS_playableMusicIcons)

	def closeMusicPlayer(self):
		if not self.mediaPlayer.isOpen:
			return

		self.mediaPlayer.stopSound()
		self.mediaPlayer.closeScreen()

		self.top.pediaJump(SevoScreenEnums.PEDIA_MUSIC, self.iMusic, False, False)

	def isMusicPlayerOpen(self):
		return self.mediaPlayer.isOpen

	def handleOverlayInput(self, inputClass):
		return self.mediaPlayer.handleInput(inputClass, self.closeMusicPlayer, False)

	def replayMusic(self):
		if self.SAS_lastMusicSound is None:
			return
		szSoundScript, iSoundId, bForce3D = self.SAS_lastMusicSound
		iMusicType, iMusicId = self.top.SAS_unpackMusicKey(self.iMusic)
		if iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_LEADER:
			self.mediaPlayer.stopSound()
			self.mediaPlayer.closeScreen()
			self.showMusicPlayer(self.iMusic)
			return
		self.mediaPlayer.resetTimer()
		self.mediaPlayer.stopSound()
		self.mediaPlayer.playSound(szSoundScript, iSoundId, bForce3D)

	def updateTimer(self, fDelta):
		self.mediaPlayer.updateTimer(fDelta)

	def playPrevMusic(self):
		if (self.SAS_playableMusic is None) or (self.SAS_playableMusicIndex <= 0):
			return
		self.SAS_playableMusicIndex = self.SAS_playableMusicIndex - 1
		self.SAS_playMusicByIndex(self.SAS_playableMusicIndex)

	def playNextMusic(self):
		if (self.SAS_playableMusic is None) or (self.SAS_playableMusicIndex < 0):
			return
		if (self.SAS_playableMusicIndex + 1) >= len(self.SAS_playableMusic):
			return
		self.SAS_playableMusicIndex = self.SAS_playableMusicIndex + 1
		self.SAS_playMusicByIndex(self.SAS_playableMusicIndex)

	def playPrevMusicGroup(self):
		if (self.SAS_playableMusicGroupByIndex is None) or (self.SAS_playableMusicIndex < 0):
			return
		iGroup = self.SAS_playableMusicGroupByIndex[self.SAS_playableMusicIndex]
		if iGroup <= 0:
			return
		iTarget = self.SAS_musicGroupFirstIndex[iGroup - 1]
		if iTarget is None:
			return
		self.SAS_playableMusicIndex = iTarget
		self.SAS_playMusicByIndex(self.SAS_playableMusicIndex)

	def playNextMusicGroup(self):
		if (self.SAS_playableMusicGroupByIndex is None) or (self.SAS_playableMusicIndex < 0):
			return
		iGroup = self.SAS_playableMusicGroupByIndex[self.SAS_playableMusicIndex]
		if iGroup < 0:
			return
		if (iGroup + 1) >= len(self.SAS_musicGroupFirstIndex):
			return
		iTarget = self.SAS_musicGroupFirstIndex[iGroup + 1]
		if iTarget is None:
			return
		self.SAS_playableMusicIndex = iTarget
		self.SAS_playMusicByIndex(self.SAS_playableMusicIndex)

	def SAS_setupPlayableMusic(self, iMusic):
		(self.SAS_playableMusic, self.SAS_playableMusicLabels, self.SAS_playableMusicIcons, self.SAS_playableMusicGroupByIndex, self.SAS_playableMusicGroupLabels, self.SAS_musicGroupFirstIndex, self.SAS_musicGroupLastIndex) = self.SAS_buildPlayableMusicAndLabels()
		self.SAS_playableMusicIndex = -1
		try:
			self.SAS_playableMusicIndex = self.SAS_playableMusic.index(iMusic)
		except:
			self.SAS_playableMusicIndex = -1

	def SAS_buildPlayableMusicAndLabels(self):
		r = []
		labels = []
		icons = []
		groupByIndex = []
		groupLabels = []
		groupFirst = []
		groupLast = []
		listEntries = self.top.getMusicList()
		iGroup = -1
		for (szName, iPacked) in listEntries:
			if iPacked == -1:
				if szName and szName.strip():
					iGroup += 1
					while len(groupLabels) <= iGroup:
						groupLabels.append("")
					groupLabels[iGroup] = szName
				continue
			if self.hasMusic(iPacked):
				r.append(iPacked)
				labels.append(szName)
				szButton = self.top.SAS_getMusicButton(iPacked)
				icons.append(szButton)
				if iGroup < 0:
					iGroup = 0
				groupByIndex.append(iGroup)
				while len(groupLabels) <= iGroup:
					groupLabels.append("")
				while len(groupFirst) <= iGroup:
					groupFirst.append(None)
					groupLast.append(None)
				if groupFirst[iGroup] is None:
					groupFirst[iGroup] = len(r) - 1
					groupLast[iGroup] = len(r) - 1
				else:
					groupLast[iGroup] = len(r) - 1
		return (r, labels, icons, groupByIndex, groupLabels, groupFirst, groupLast)

	def SAS_playMusicByIndex(self, iIndex):
		if (self.SAS_playableMusic is None) or (iIndex < 0) or (iIndex >= len(self.SAS_playableMusic)):
			return
		iPacked = self.SAS_playableMusic[iIndex]
		self.iMusic = iPacked
		self.mediaPlayer.stopSound()
		self.mediaPlayer.closeScreen()
		self.showMusicPlayer(iPacked)

	def SAS_getFirstPlayableMusic(self):
		playable, labels, icons, groupByIndex, groupLabels, groupFirst, groupLast = self.SAS_buildPlayableMusicAndLabels()
		if not playable:
			return -1
		return playable[0]

	def switchToMovies(self):
		iFirstMovie = self.top.pediaMovies.SAS_getFirstPlayableMovie()
		if iFirstMovie == -1:
			return
		self.mediaPlayer.stopSound()
		self.mediaPlayer.closeScreen()
		self.top.pediaJump(SevoScreenEnums.PEDIA_MOVIES, iFirstMovie, True, False)
		self.top.pediaMovies.playMovie(iFirstMovie)

	def getMusicInfo(self, iPacked):
		return self.top.getMusicInfo(iPacked)

	def hasMusic(self, iPacked):
		szSound = self.top.SAS_getMusicSoundScript(iPacked)
		if szSound and (szSound != "NONE"):
			return True
		return self.top.SAS_getMusicSoundId(iPacked) != -1

	def getTextForMusic(self, iPacked):
		iMusicType, iMusicId = self.top.SAS_unpackMusicKey(iPacked)
		info = self.getMusicInfo(iPacked)

		# For tech music: show the tech quote
		if info and iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_TECH:
			try:
				szQuote = info.getQuote()
				if szQuote and not szQuote.startswith("TXT_KEY_"):
					return szQuote
			except:
				pass

		# For civilization music: show the variant note
		elif iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_CIV:
			szNote = localText.getText("TXT_KEY_PEDIA_SAS_MUSIC_CIV_VARIANT_NOTE", ())
			if szNote:
				return szNote
		elif iMusicType == SAS_MAGIC_PEDIA_MUSIC_TYPE_SCRIPT_3D:
			szScript = self.top.SAS_getMusicSoundScript(iPacked)
			if szScript.startswith("AS3D_IMPROV"):
				return localText.getText("TXT_KEY_PEDIA_SAS_MUSIC_3D_IMPROVEMENT_NOTE", ())
			if szScript.startswith("AS3D_SS_"):
				return localText.getText("TXT_KEY_PEDIA_SAS_MUSIC_3D_AMBIENCE_NOTE", ())

		return ""

	def placeCivDisplay(self, screen, iMusic, iImageX, iImageY, iImageW, iImageH):
		iCivId = self.top.SAS_getMusicCivId(iMusic)
		if iCivId == -1:
			return

		civInfo = gc.getCivilizationInfo(iCivId)
		if not civInfo:
			return

		# Calculate left and right halves
		iLeftW = iImageW / 2 - 5
		iRightW = iImageW / 2 - 5
		iLeftX = iImageX
		iRightX = iImageX + iImageW / 2 + 5

		# Place civ button on the left half (square and centered)
		szCivButton = civInfo.getButton()
		if szCivButton:
			iCivButtonSize = min(iLeftW, iImageH)
			iCivButtonX = iLeftX + (iLeftW - iCivButtonSize) / 2
			iCivButtonY = iImageY + (iImageH - iCivButtonSize) / 2
			screen.setImageButton("MusicPlayerCivButton", szCivButton, iCivButtonX, iCivButtonY, iCivButtonSize, iCivButtonSize, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIV, iCivId, 1)

		# Get all leaders for this civilization
		aLeaders = []
		for iLeader in range(gc.getNumLeaderHeadInfos()):
			leaderInfo = gc.getLeaderHeadInfo(iLeader)
			if leaderInfo:
				# Check if this leader is associated with this civ
				for iCiv in range(gc.getNumCivilizationInfos()):
					if iCiv == iCivId:
						# Check if this leader can lead this civ
						try:
							if gc.getCivilizationInfo(iCiv).isLeaders(iLeader):
								aLeaders.append(iLeader)
								break
						except:
							pass

		# Display leaders on the right half
		if aLeaders:
			iNumLeaders = len(aLeaders)
			# Start with 3 columns, reduce if needed for many leaders
			iCols = 3
			iRows = (iNumLeaders + iCols - 1) / iCols

			# Calculate max button size to keep them square
			iGap = 4
			iMaxW = (iRightW - (iCols - 1) * iGap) / iCols
			iMaxH = (iImageH - (iRows - 1) * iGap) / iRows
			iButtonSize = min(iMaxW, iMaxH)

			# If buttons get too small (less than 32px), reduce columns
			if iButtonSize < 32 and iCols > 2:
				iCols = 2
				iRows = (iNumLeaders + iCols - 1) / iCols
				iMaxW = (iRightW - (iCols - 1) * iGap) / iCols
				iMaxH = (iImageH - (iRows - 1) * iGap) / iRows
				iButtonSize = min(iMaxW, iMaxH)

			if iButtonSize < 24 and iCols > 1:
				iCols = 1
				iRows = iNumLeaders
				iMaxW = iRightW
				iMaxH = (iImageH - (iRows - 1) * iGap) / iRows
				iButtonSize = min(iMaxW, iMaxH)

			# Calculate grid dimensions and center it
			iGridW = iCols * iButtonSize + (iCols - 1) * iGap
			iGridH = iRows * iButtonSize + (iRows - 1) * iGap
			iStartX = iRightX + (iRightW - iGridW) / 2
			iStartY = iImageY + (iImageH - iGridH) / 2

			for i, iLeaderId in enumerate(aLeaders):
				iRow = i / iCols
				iCol = i % iCols
				iX = iStartX + iCol * (iButtonSize + iGap)
				iY = iStartY + iRow * (iButtonSize + iGap)
				szButtonName = "MusicPlayerLeader" + str(i)
				leaderInfo = gc.getLeaderHeadInfo(iLeaderId)
				szLeaderButton = leaderInfo.getButton()
				if szLeaderButton:
					# <!-- custom: The separate Music Player leader buttons need the same canonical civilization context as the main Music page link above. See KI#332. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
					screen.setImageButton(szButtonName, szLeaderButton, iX, iY, iButtonSize, iButtonSize, WidgetTypes.WIDGET_PEDIA_JUMP_TO_LEADER, iLeaderId, self.top.pediaLeader.getCiv(iLeaderId))

	def placeTechDisplay(self, screen, iMusic, iImageX, iImageY, iImageW, iImageH):
		iMusicType, iTechId = self.top.SAS_unpackMusicKey(iMusic)
		if iTechId == -1:
			return

		techInfo = gc.getTechInfo(iTechId)
		if not techInfo:
			return

		# Calculate left and right halves
		iLeftW = iImageW / 2 - 5
		iRightW = iImageW / 2 - 5
		iLeftX = iImageX
		iRightX = iImageX + iImageW / 2 + 5

		# Get era for this tech
		iEra = -1
		try:
			iEra = techInfo.getEra()
		except:
			pass

		# Place era button on the left half (square and centered)
		if iEra != -1:
			eraInfo = gc.getEraInfo(iEra)
			if eraInfo:
				szEraButton = eraInfo.getButton()
				if szEraButton:
					iEraButtonSize = min(iLeftW, iImageH)
					iEraButtonX = iLeftX + (iLeftW - iEraButtonSize) / 2
					iEraButtonY = iImageY + (iImageH - iEraButtonSize) / 2
					screen.setImageButton("MusicPlayerEraButton", szEraButton, iEraButtonX, iEraButtonY, iEraButtonSize, iEraButtonSize, WidgetTypes.WIDGET_PEDIA_MAIN, SevoScreenEnums.PEDIA_ERA_CHART, -1)

		# Place tech button on the right half (square and centered)
		szTechButton = techInfo.getButton()
		if szTechButton:
			iTechButtonSize = min(iRightW, iImageH)
			iTechButtonX = iRightX + (iRightW - iTechButtonSize) / 2
			iTechButtonY = iImageY + (iImageH - iTechButtonSize) / 2
			screen.setImageButton("MusicPlayerTechButton", szTechButton, iTechButtonX, iTechButtonY, iTechButtonSize, iTechButtonSize, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTechId, 1)

	def placeScriptDisplay(self, screen, iMusic, iImageX, iImageY, iImageW, iImageH):
		# Find the current music's group and get the first item in that group
		playable, labels, icons, groupByIndex, groupLabels, groupFirst, groupLast = self.SAS_buildPlayableMusicAndLabels()

		iCurrentGroup = -1
		iFirstInGroup = -1
		szGroupIcon = ""

		try:
			iCurrentIndex = playable.index(iMusic)
			if iCurrentIndex >= 0 and iCurrentIndex < len(groupByIndex):
				iCurrentGroup = groupByIndex[iCurrentIndex]
				if iCurrentGroup >= 0 and iCurrentGroup < len(groupFirst):
					iFirstInGroup = groupFirst[iCurrentGroup]
					if iFirstInGroup >= 0 and iFirstInGroup < len(icons):
						szGroupIcon = icons[iFirstInGroup]
		except:
			pass

		# If we couldn't find a group icon, try to use the current item's icon
		if not szGroupIcon:
			szGroupIcon = self.top.SAS_getMusicButton(iMusic)

		# Use musical notes icon as fallback
		if not szGroupIcon:
			szGroupIcon = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_MUSICAL_NOTES").getPath()

		# Place the icon centered and square
		if szGroupIcon:
			iButtonSize = min(iImageW, iImageH)
			iButtonX = iImageX + (iImageW - iButtonSize) / 2
			iButtonY = iImageY + (iImageH - iButtonSize) / 2

			# If we found a first item in group, make it clickable to jump to that
			if iFirstInGroup >= 0 and iFirstInGroup < len(playable):
				iTargetMusic = playable[iFirstInGroup]
				screen.setImageButton("MusicPlayerScriptButton", szGroupIcon, iButtonX, iButtonY, iButtonSize, iButtonSize, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_MUSIC_PLAY, iTargetMusic)
			else:
				# Otherwise just show the icon without action
				screen.addDDSGFC("MusicPlayerScriptButton", szGroupIcon, iButtonX, iButtonY, iButtonSize, iButtonSize, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def handleInput(self, inputClass):
		return 0
