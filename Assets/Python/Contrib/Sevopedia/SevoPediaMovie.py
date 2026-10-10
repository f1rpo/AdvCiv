# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: based on the Middle-earth mod's Platypedia's Movies category and adjusted for AdvCiv-SAS then enhanced with the help of GPT-5.2-Codex and Claude code Opus 4.5 thanks a lot. (GPT-5.2-Codex (summarized)) -->
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
UserProfile = CyUserProfile()

class SevoPediaMovie:

	def __init__(self, main):
		self.top = main
		self.iMovie = -1
		self.MOVIE_PLAYER_SCREEN = "SevoPediaMoviePlayer"
		self.MOVIE_PLAYER_EXIT_ID = "SAS_MoviePlayerExit"
		# <!-- custom: bik files don't handle well: For BIK, playMovie() doesn’t behave like the other media widgets, so using a "soft eject + re‑open overlay" is safer. (GPT-5.2-Codex) -->
		self.SAS_savedNoMovies = None
		self.mediaPlayer = SevoPediaMediaPlayer(self.MOVIE_PLAYER_SCREEN, SevoScreenEnums.PEDIA_MOVIES, self.MOVIE_PLAYER_EXIT_ID, "MoviePlayer")
		self.SAS_lastMoviePayload = None
		self.SAS_playableMovies = None
		self.SAS_playableMovieLabels = None
		self.SAS_playableMovieIcons = None
		self.SAS_playableMovieIndex = -1
		self.SAS_playableMovieGroupByIndex = None
		self.SAS_playableMovieGroupLabels = None
		self.SAS_movieGroupFirstIndex = None
		self.SAS_movieGroupLastIndex = None

		self.X_HEADER = self.top.X_PEDIA_PAGE
		self.Y_HEADER = self.top.Y_PEDIA_PAGE
		self.W_HEADER = self.top.W_PEDIA_PAGE
		self.H_HEADER = 120

		self.W_ICON = 100
		self.H_ICON = 100
		self.X_ICON = self.X_HEADER + 10
		self.Y_ICON = self.Y_HEADER + 10

		self.X_TITLE = self.X_ICON + self.W_ICON + 10
		self.Y_TITLE = self.Y_HEADER + 12

		# <!-- custom: changed from text button to image button. (Claude Opus 4.5) -->
		self.W_BUTTON = 64
		self.H_BUTTON = 64
		self.X_BUTTON = self.X_HEADER + self.W_HEADER - self.W_BUTTON - 10
		self.Y_BUTTON = self.Y_HEADER + (self.H_HEADER - self.H_BUTTON) / 2
		self.playButtonPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_PLAY_BUTTON").getPath()

		self.X_HISTORY = self.X_HEADER
		self.Y_HISTORY = self.Y_HEADER + self.H_HEADER + 10
		self.W_HISTORY = self.top.W_PEDIA_PAGE
		self.H_HISTORY = self.top.B_PEDIA_PAGE - self.Y_HISTORY

	def interfaceScreen(self, iVictory):
		self.iMovie = iVictory

		self.placeHeader()
		self.placeHistory()

	def placeHeader(self):
		screen = self.top.getScreen()
		iMovieType, iMovieId = self.top.SAS_unpackMovieKey(self.iMovie)
		info = self.getMovieInfo(iMovieType, iMovieId)

		screen.addPanel(self.top.getNextWidgetName(), "", "", False, False, self.X_HEADER, self.Y_HEADER, self.W_HEADER, self.H_HEADER, PanelStyles.PANEL_STYLE_BLUE50)

		szButton = ""
		if info:
			szButton = info.getButton()
		if szButton:
			widgetType, widgetData1, widgetData2 = self.getPediaJumpWidget(iMovieType, iMovieId)
			if widgetType == WidgetTypes.WIDGET_GENERAL:
				screen.addDDSGFC(self.top.getNextWidgetName(), szButton, self.X_ICON + self.W_ICON / 2 - PANE_ICON_SIZE / 2, self.Y_ICON + self.H_ICON / 2 - PANE_ICON_SIZE / 2, PANE_ICON_SIZE, PANE_ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1)
			else:
				screen.setImageButton(self.top.getNextWidgetName(), szButton, self.X_ICON + self.W_ICON / 2 - PANE_ICON_SIZE / 2, self.Y_ICON + self.H_ICON / 2 - PANE_ICON_SIZE / 2, PANE_ICON_SIZE, PANE_ICON_SIZE, widgetType, widgetData1, widgetData2)

		szTitleText = ""
		if info:
			szTitleText = info.getDescription()
		szTitle = sasFontTagTitle.bold + szTitleText.upper() + SAS_FONT_TAG_CLOSE
		screen.setLabel(self.top.getNextWidgetName(), "Background", szTitle, CvUtil.FONT_LEFT_JUSTIFY, self.X_TITLE, self.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		if self.hasMovie(iMovieType, iMovieId):
			screen.setImageButton(self.top.getNextWidgetName(), self.playButtonPath, self.X_BUTTON, self.Y_BUTTON, self.W_BUTTON, self.H_BUTTON, WidgetTypes.WIDGET_PYTHON, SAS_MAGIC_PEDIA_PYTHON_MOVIE_PLAY, self.iMovie)

	def placeHistory(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, "", "", True, True, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, PanelStyles.PANEL_STYLE_BLUE50)
		iMovieType, iMovieId = self.top.SAS_unpackMovieKey(self.iMovie)
		info = self.getMovieInfo(iMovieType, iMovieId)
		szText = ""
		if info:
			try:
				szText = info.getCivilopedia()
			except:
				szText = ""
		if szText and szText.startswith("TXT_KEY_"):
			szText = ""
		szText = SASTextScale.labelText(szText)
		textName = self.top.getNextWidgetName()
		screen.addMultilineText(textName, szText, self.X_HISTORY + 10, self.Y_HISTORY + 10, self.W_HISTORY - 20, self.H_HISTORY - 20, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def playMovie(self, iVictory):
		game = CyGame()
		if game.isNetworkMultiPlayer() or game.isPitbossHost():
			return

		iMovieType, iMovieId = self.top.SAS_unpackMovieKey(iVictory)
		moviePayload = self.getMoviePayload(iMovieType, iMovieId)
		if moviePayload is None:
			return
		self.showMoviePlayer(iMovieType, iMovieId, moviePayload)

	def showMoviePlayer(self, iMovieType, iMovieId, moviePayload):
		szMovieFile, szMovieKind, szSoundScript = moviePayload
		self.SAS_lastMoviePayload = (iMovieType, iMovieId, szMovieFile, szMovieKind, szSoundScript)

		if self.SAS_savedNoMovies is None:
			self.SAS_savedNoMovies = CyUserProfile().getGraphicOption(GraphicOptionTypes.GRAPHICOPTION_NO_MOVIES)
		CyUserProfile().setGraphicOption(GraphicOptionTypes.GRAPHICOPTION_NO_MOVIES, False)

		screen = self.mediaPlayer.openScreen()
		iScreenW, iScreenH, iMovieX, iMovieY, iMovieW, iMovieH = self.mediaPlayer.setupLayout(screen, "", False)
		bUseFullScreenMovie = (szMovieKind == "movie")
		# <!-- custom: BIK movies render full-screen, so hide the TV panel and use compact prev/next labels to keep navigation readable. (GPT-5.2-Codex) -->
		if not bUseFullScreenMovie:
			_, _, _, _, iMovieX, iMovieY, iMovieW, iMovieH = self.mediaPlayer.placeTvPanel(screen, iScreenW, iScreenH)
		else:
			self.mediaPlayer.clearTvPanel(screen)
		self.mediaPlayer.placeTimerLabel(screen, iScreenW, iScreenH)
		self.mediaPlayer.setCurrentLabel(screen, self.getMovieTitle(iMovieType, iMovieId))
		self.mediaPlayer.startTimer()

		if szMovieKind == "nif":
			screen.addReligionMovieWidgetGFC("MoviePlayerMovie", szMovieFile, iMovieX, iMovieY, iMovieW, iMovieH, WidgetTypes.WIDGET_GENERAL, -1, -1)
		elif szMovieKind == "dds":
			screen.addDDSGFC("MoviePlayerMovie", szMovieFile, iMovieX, iMovieY, iMovieW, iMovieH, WidgetTypes.WIDGET_GENERAL, -1, -1)
		else:
			screen.playMovie(szMovieFile, -1, -1, -1, -1, 0)

		# <!-- custom: Play audio using Play2DSound/Destroy2DSound approach.
		# This works fine for both starting audio on open and stopping it on exit.
		# An alternative cleaner approach exists in CvEraMovieScreen.py using screen.setSound() before showScreen(), but we couldn't make it work in our context, so we stick with this approach. Credit: GPT-5.2-Codex, Claude Opus 4.5. -->
		if szSoundScript:
			self.mediaPlayer.playSound(szSoundScript, -1, False)

		self.mediaPlayer.placeExitButton(screen, iScreenW, iScreenH)
		self.mediaPlayer.placeReplayButton(screen, iScreenW, iScreenH)
		self.mediaPlayer.setToggleButton("SAS_EMOJI_TELEVISION")
		self.mediaPlayer.placeToggleButton(screen, iScreenW, iScreenH)
		self.mediaPlayer.setReplayCallback(self.replayMovie)
		self.mediaPlayer.placePrevNextButtons(screen, iScreenW, iScreenH)
		self.mediaPlayer.placeGroupSkipButtons(screen, iScreenW, iScreenH)
		self.mediaPlayer.setPrevCallback(self.playPrevMovie)
		self.mediaPlayer.setNextCallback(self.playNextMovie)
		self.mediaPlayer.setPrevGroupCallback(self.playPrevMovieGroup)
		self.mediaPlayer.setNextGroupCallback(self.playNextMovieGroup)
		self.mediaPlayer.setToggleCallback(self.switchToMusic)

		self.SAS_setupPlayableMovies(iMovieType, iMovieId)
		if bUseFullScreenMovie:
			self.mediaPlayer.clearQueueList(screen)
			szPrev, szNext = self.SAS_getAdjacentMovieLabels()
			self.mediaPlayer.placePrevNextLabels(screen, iScreenW, iScreenH, szPrev, szNext)
			self.mediaPlayer.setCurrentLabel(screen, self.getMovieTitle(iMovieType, iMovieId))
		else:
			self.mediaPlayer.placeQueueList(screen, iScreenW, iScreenH, self.SAS_playableMovieLabels, self.SAS_playableMovieIndex, self.SAS_playableMovieGroupByIndex, self.SAS_playableMovieGroupLabels, self.SAS_playableMovieIcons)

	def closeMoviePlayer(self):
		if not self.mediaPlayer.isOpen:
			return

		self.restoreNoMoviesOption()

		# <!-- custom: try to stop the specific 2D sound via Destroy2DSound(handle); if that fails, fall back to
		# CyInterface().stop2DSound() as a global 2D stop for stubborn handles. Credit: CIV4BUG API docs.
		# (GPT-5.2-Codex (summarized)) -->
		self.mediaPlayer.stopSound()
		self.mediaPlayer.closeScreen()

		self.top.pediaJump(SevoScreenEnums.PEDIA_MOVIES, self.iMovie, False, False)

	# <!-- custom: Movie playback temporarily disables the user's No Movies option.
	# Ordinary close restored it, but Movie-to-Music bypassed that path and leaked the changed preference for the rest of the session; centralizing restoration fixes every transition that actually leaves Movie mode. See KI#299. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	def restoreNoMoviesOption(self):
		if self.SAS_savedNoMovies is not None:
			CyUserProfile().setGraphicOption(GraphicOptionTypes.GRAPHICOPTION_NO_MOVIES, self.SAS_savedNoMovies)
			self.SAS_savedNoMovies = None

	def isMoviePlayerOpen(self):
		return self.mediaPlayer.isOpen

	def handleOverlayInput(self, inputClass):
		return self.mediaPlayer.handleInput(inputClass, self.closeMoviePlayer, True)

	def updateTimer(self, fDelta):
		self.mediaPlayer.updateTimer(fDelta)

	def replayMovie(self):
		if self.SAS_lastMoviePayload is None:
			return

		iMovieType, iMovieId, szMovieFile, szMovieKind, szSoundScript = self.SAS_lastMoviePayload

		self.mediaPlayer.stopSound()
		self.mediaPlayer.closeScreen()
		self.showMoviePlayer(iMovieType, iMovieId, (szMovieFile, szMovieKind, szSoundScript))

	def playPrevMovie(self):
		if (self.SAS_playableMovies is None) or (self.SAS_playableMovieIndex <= 0):
			return
		self.SAS_playableMovieIndex = self.SAS_playableMovieIndex - 1
		self.SAS_playMovieByIndex(self.SAS_playableMovieIndex)

	def playNextMovie(self):
		if (self.SAS_playableMovies is None) or (self.SAS_playableMovieIndex < 0):
			return
		if (self.SAS_playableMovieIndex + 1) >= len(self.SAS_playableMovies):
			return
		self.SAS_playableMovieIndex = self.SAS_playableMovieIndex + 1
		self.SAS_playMovieByIndex(self.SAS_playableMovieIndex)

	def playPrevMovieGroup(self):
		if (self.SAS_playableMovieGroupByIndex is None) or (self.SAS_playableMovieIndex < 0):
			return
		iGroup = self.SAS_playableMovieGroupByIndex[self.SAS_playableMovieIndex]
		if iGroup <= 0:
			return
		iTarget = self.SAS_movieGroupFirstIndex[iGroup - 1]
		if iTarget is None:
			return
		self.SAS_playableMovieIndex = iTarget
		self.SAS_playMovieByIndex(self.SAS_playableMovieIndex)

	def playNextMovieGroup(self):
		if (self.SAS_playableMovieGroupByIndex is None) or (self.SAS_playableMovieIndex < 0):
			return
		iGroup = self.SAS_playableMovieGroupByIndex[self.SAS_playableMovieIndex]
		if iGroup < 0:
			return
		if (iGroup + 1) >= len(self.SAS_movieGroupFirstIndex):
			return
		iTarget = self.SAS_movieGroupFirstIndex[iGroup + 1]
		if iTarget is None:
			return
		self.SAS_playableMovieIndex = iTarget
		self.SAS_playMovieByIndex(self.SAS_playableMovieIndex)

	def SAS_setupPlayableMovies(self, iMovieType, iMovieId):
		(self.SAS_playableMovies, self.SAS_playableMovieLabels, self.SAS_playableMovieIcons, self.SAS_playableMovieGroupByIndex, self.SAS_playableMovieGroupLabels, self.SAS_movieGroupFirstIndex, self.SAS_movieGroupLastIndex) = self.SAS_buildPlayableMoviesAndLabels()
		iPacked = self.top.SAS_packMovieKey(iMovieType, iMovieId)
		self.SAS_playableMovieIndex = -1
		try:
			self.SAS_playableMovieIndex = self.SAS_playableMovies.index(iPacked)
		except:
			self.SAS_playableMovieIndex = -1

	def SAS_buildPlayableMoviesAndLabels(self):
		r = []
		labels = []
		icons = []
		groupByIndex = []
		groupLabels = []
		groupFirst = []
		groupLast = []
		listEntries = self.top.getMovieList()
		iGroup = -1
		for (szName, iPacked) in listEntries:
			if iPacked == -1:
				if szName and szName.strip():
					iGroup += 1
					while len(groupLabels) <= iGroup:
						groupLabels.append("")
					groupLabels[iGroup] = szName
				continue
			iType, iId = self.top.SAS_unpackMovieKey(iPacked)
			if self.hasMovie(iType, iId):
				r.append(iPacked)
				labels.append(szName)
				szButton = ""
				info = self.getMovieInfo(iType, iId)
				if info:
					try:
						szButton = info.getButton()
					except:
						szButton = ""
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

	def SAS_playMovieByIndex(self, iIndex):
		if (self.SAS_playableMovies is None) or (iIndex < 0) or (iIndex >= len(self.SAS_playableMovies)):
			return
		iPacked = self.SAS_playableMovies[iIndex]
		self.iMovie = iPacked
		iMovieType, iMovieId = self.top.SAS_unpackMovieKey(iPacked)
		moviePayload = self.getMoviePayload(iMovieType, iMovieId)
		if moviePayload is None:
			return
		self.mediaPlayer.stopSound()
		self.mediaPlayer.closeScreen()
		self.showMoviePlayer(iMovieType, iMovieId, moviePayload)

	def SAS_getFirstPlayableMovie(self):
		playable, labels, icons, groupByIndex, groupLabels, groupFirst, groupLast = self.SAS_buildPlayableMoviesAndLabels()
		if not playable:
			return -1
		return playable[0]

	def SAS_getAdjacentMovieLabels(self):
		if (self.SAS_playableMovieLabels is None) or (self.SAS_playableMovieIndex < 0):
			return ("", "")
		szPrev = ""
		szNext = ""
		if self.SAS_playableMovieIndex > 0:
			szPrev = self.SAS_playableMovieLabels[self.SAS_playableMovieIndex - 1]
		if (self.SAS_playableMovieIndex + 1) < len(self.SAS_playableMovieLabels):
			szNext = self.SAS_playableMovieLabels[self.SAS_playableMovieIndex + 1]
		return (szPrev, szNext)

	def switchToMusic(self):
		iFirstMusic = self.top.pediaMusic.SAS_getFirstPlayableMusic()
		if iFirstMusic == -1:
			return
		self.restoreNoMoviesOption()
		self.mediaPlayer.stopSound()
		self.mediaPlayer.closeScreen()
		self.top.pediaJump(SevoScreenEnums.PEDIA_MUSIC, iFirstMusic, True, False)
		self.top.pediaMusic.playMusic(iFirstMusic)

	def getMovieInfo(self, iMovieType, iMovieId):
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_VICTORY:
			return gc.getVictoryInfo(iMovieId)
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_WONDER:
			return gc.getBuildingInfo(iMovieId)
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_PROJECT:
			return gc.getProjectInfo(iMovieId)
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_RELIGION:
			return gc.getReligionInfo(iMovieId)
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_ERA:
			return gc.getEraInfo(iMovieId)
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_CORPORATION:
			return gc.getCorporationInfo(iMovieId)
		return None

	def getMoviePayload(self, iMovieType, iMovieId):
		if not self.hasMovie(iMovieType, iMovieId):
			return None

		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_VICTORY:
			szArtDef = gc.getVictoryInfo(iMovieId).getMovie()
			if not szArtDef:
				return None
			szMovie = CyArtFileMgr().getMovieArtInfo(szArtDef).getPath()
			if not szMovie:
				return None
			return (szMovie, "movie", None)

		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_WONDER:
			szMovie = gc.getBuildingInfo(iMovieId).getMovie()
			if not szMovie:
				return None
			if szMovie:
				if (szMovie.find(".") == -1) or (szMovie.find("ART_DEF") > -1):
					szMovie = CyArtFileMgr().getMovieArtInfo(szMovie).getPath()
			if not szMovie:
				return None
			szMovieKind = "movie"
			if szMovie.find(".dds") > -1:
				szMovieKind = "dds"
			return (szMovie, szMovieKind, None)

		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_PROJECT:
			szArtDef = gc.getProjectInfo(iMovieId).getMovieArtDef()
			if not szArtDef:
				return None
			szMovie = CyArtFileMgr().getMovieArtInfo(szArtDef).getPath()
			if not szMovie:
				return None
			return (szMovie, "movie", None)

		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_RELIGION:
			szMovieFile = gc.getReligionInfo(iMovieId).getMovieFile()
			if not szMovieFile:
				return None
			szMovieKind = "movie"
			if szMovieFile.find(".nif") > -1:
				szMovieKind = "nif"
			szSoundScript = ""
			try:
				szSoundScript = gc.getReligionInfo(iMovieId).getMovieSound()
			except:
				szSoundScript = ""
			if szSoundScript == "NONE":
				szSoundScript = ""
			return (szMovieFile, szMovieKind, szSoundScript)

		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_ERA:
			szMovieFile = gc.getEraInfo(iMovieId).getButton()
			if not szMovieFile:
				return None
			return (szMovieFile, "dds", "AS2D_NEW_ERA")

		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_CORPORATION:
			szMovieFile = gc.getCorporationInfo(iMovieId).getMovieFile()
			if (not szMovieFile) or (szMovieFile == "NONE"):
				return None
			szMovieKind = "movie"
			if szMovieFile.find(".nif") > -1:
				szMovieKind = "nif"
			szSoundScript = ""
			try:
				szSoundScript = gc.getCorporationInfo(iMovieId).getMovieSound()
			except:
				szSoundScript = ""
			if szSoundScript == "NONE":
				szSoundScript = ""
			return (szMovieFile, szMovieKind, szSoundScript)

		return None

	def getMovieTitle(self, iMovieType, iMovieId):
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_ERA:
			info = self.getMovieInfo(iMovieType, iMovieId)
			if info:
				return info.getDescription() + " " + localText.getText("TXT_KEY_PEDIA_ERA", ())
			return u""

		info = self.getMovieInfo(iMovieType, iMovieId)
		if info:
			return info.getDescription()
		return u""

	def hasMovie(self, iMovieType, iMovieId):
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_VICTORY:
			info = gc.getVictoryInfo(iMovieId)
			return (info is not None) and bool(info.getMovie())
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_WONDER:
			info = gc.getBuildingInfo(iMovieId)
			return (info is not None) and bool(info.getMovie())
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_PROJECT:
			info = gc.getProjectInfo(iMovieId)
			return (info is not None) and bool(info.getMovieArtDef())
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_RELIGION:
			info = gc.getReligionInfo(iMovieId)
			return (info is not None) and bool(info.getMovieFile())
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_ERA:
			return bool(gc.getEraInfo(iMovieId).getButton())
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_CORPORATION:
			info = gc.getCorporationInfo(iMovieId)
			if info is None:
				return False
			szMovieFile = ""
			try:
				szMovieFile = info.getMovieFile()
			except:
				szMovieFile = ""
			return bool(szMovieFile) and szMovieFile != "NONE"
		return False

	def getPediaJumpWidget(self, iMovieType, iMovieId):
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_WONDER:
			return (WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iMovieId, 1)
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_PROJECT:
			return (WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROJECT, iMovieId, 1)
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_RELIGION:
			return (WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION, iMovieId, 1)
		# <!-- custom: this successfully works: redirects to Sevopedia Eras Chart category, that has no item and only a chart (like Promotions Tree for example).
		# Done with the very nice help of Claude code Sonnet 4.5 thanks a lot! -->
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_ERA:
			return (WidgetTypes.WIDGET_PEDIA_MAIN, SevoScreenEnums.PEDIA_ERA_CHART, -1)
		if iMovieType == SAS_MAGIC_PEDIA_MOVIE_TYPE_CORPORATION:
			return (WidgetTypes.WIDGET_PEDIA_JUMP_TO_CORPORATION, iMovieId, 1)
		return (WidgetTypes.WIDGET_GENERAL, -1, -1)

	def handleInput (self, inputClass):
		return 0
