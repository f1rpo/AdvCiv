# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from CvPythonExtensions import *
import CvUtil
import SASTextScale
from SASFontUtils import sasFontTagLabel, SAS_FONT_TAG_CLOSE

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

def _SAS_getAudio3DScriptName(iSoundId):
	# <!-- custom: The fix is only for Sevopedia's media player.
	# The observed problem was that civilization Select/Order sounds previewed correctly from the main menu, but after entering a game the same Sevopedia previews became much quieter; returning to the main menu did not fix them, and the game had to be restarted.
	# For 3D sounds, iSoundId is a 3D script index, not a normal 2D/general sound ID.
	# CyAudioGame().Play3DSoundWithId(iSoundId, 0, 0, 0) plays the right 3D script but very quietly, while CyInterface().playGeneralSoundByID(iSoundId) plays the wrong sound because that ID belongs to a different audio namespace.
	# AdvCiv-SAS DLL exposes gc.getAudio3DScriptName(iSoundId), which caches Audio3DScripts.xml once in the DLL and returns the AS3D_... script name for any 3D script ID.
	# playSound then calls CyAudioGame().Play3DSound(szScript, -1, -1, -1), which testing showed plays at normal volume and returns a Destroy3DSound-compatible handle; if the helper is unavailable or returns empty, playSound falls back to the old quiet 3D path. See KI#141. (GPT-5.5) -->
	try:
		return gc.getAudio3DScriptName(iSoundId)
	except:
		return ""

class SevoPediaMediaPlayer:

	def __init__(self, screenId, screenEnum, exitId, clickPrefix):
		self.screenId = screenId
		self.screenEnum = screenEnum
		self.exitId = exitId
		self.replayId = clickPrefix + "Replay"
		self.prevId = clickPrefix + "Prev"
		self.nextId = clickPrefix + "Next"
		self.toggleId = clickPrefix + "Toggle"
		self.prevGroupId = clickPrefix + "PrevGroup"
		self.nextGroupId = clickPrefix + "NextGroup"
		self.timerLabelId = clickPrefix + "Timer"
		self.currentLabelId = clickPrefix + "CurrentLabel"
		self.tvPanelId = clickPrefix + "TvPanel"
		self.queuePanelId = clickPrefix + "QueuePanel"
		self.queueListId = clickPrefix + "QueueList"
		self.clickPrefix = clickPrefix
		self.isOpen = False
		self.soundId = None
		self.is3DSound = False
		self.exitButtonPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_EJECT_BUTTON").getPath()
		self.replayButtonPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_PLAY_BUTTON").getPath()
		self.prevButtonPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_LAST_TRACK_BUTTON").getPath()
		self.nextButtonPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_NEXT_TRACK_BUTTON").getPath()
		self.prevGroupButtonPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_FAST_UP_BUTTON").getPath()
		self.nextGroupButtonPath = ArtFileMgr.getInterfaceArtInfo("SAS_EMOJI_FAST_DOWN_BUTTON").getPath()
		self.replayCallback = None
		self.prevCallback = None
		self.nextCallback = None
		self.prevGroupCallback = None
		self.nextGroupCallback = None
		self.toggleButtonPath = None
		self.toggleCallback = None
		self.screen = None
		self.timerSeconds = 0
		self.timerRemainder = 0.0
		self.timerRunning = False
		self.timerLabelX = 0
		self.timerLabelY = 0
		self.currentLabelX = 0
		self.currentLabelY = 0

	def openScreen(self):
		screen = CyGInterfaceScreen(self.screenId, self.screenEnum)

		# <!-- custom: (ChatGPT-5.2 Thinking) -->
		try:
			screen.enableWorldSounds(True)
		except:
			pass
		try:
			screen.setScreenGroup(1)
		except:
			pass

		self.isOpen = True
		self.screen = screen

		screen.showScreen(PopupStates.POPUPSTATE_IMMEDIATE, False)
		screen.setRenderInterfaceOnly(True)
		screen.enableWorldSounds(False)
		screen.addPanel(self.clickPrefix + "BG", u"", u"", True, False, -10, -10, screen.getXResolution() + 20, screen.getYResolution() + 20, PanelStyles.PANEL_STYLE_MAIN)
		return screen

	def setupLayout(self, screen, szTitleText, bShowTitle=True):
		iScreenW = screen.getXResolution()
		iScreenH = screen.getYResolution()
		iMediaW = iScreenW
		iMediaH = iScreenW * 2 / 3
		if iMediaH > iScreenH:
			iMediaH = iScreenH
			iMediaW = iMediaH * 3 / 2
		iMediaX = (iScreenW - iMediaW) / 2
		iMediaY = (iScreenH - iMediaH) / 2

		if bShowTitle and szTitleText:
			screen.setLabel(self.clickPrefix + "Title", "Background", SASTextScale.titleText(szTitleText.upper()), CvUtil.FONT_CENTER_JUSTIFY, iScreenW / 2, 8, -0.1, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		return (iScreenW, iScreenH, iMediaX, iMediaY, iMediaW, iMediaH)

	def placeExitButton(self, screen, iScreenW, iScreenH):
		iSize, iGap, iBaseY = self._getTransportLayout(iScreenW, iScreenH)
		iExitX = self._getTransportButtonX(iScreenW, 4)
		screen.setImageButton(self.exitId, self.exitButtonPath, iExitX, iBaseY, iSize, iSize, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# <!-- custom: (ChatGPT-5.2 Thinking) -->
		try:
			screen.setEscapeKey(self.exitId)
		except:
			pass
		try:
			screen.setReturnKey(self.exitId)
		except:
			pass

	# <!-- custom: useful to replay files with variants such as _ORDER or _SELECT civilization sounds that play a different variant on replay (e.g. "Yes", "Agreed", "Right Away" (imaginary examples)), or for convenience so we don't exit the screen to replay.
	# Added with the very nice help of GPT-5.2-Codex thanks. -->
	def placeReplayButton(self, screen, iScreenW, iScreenH):
		iSize, iGap, iBaseY = self._getTransportLayout(iScreenW, iScreenH)
		iReplayX = self._getTransportButtonX(iScreenW, 2)
		screen.setImageButton(self.replayId, self.replayButtonPath, iReplayX, iBaseY, iSize, iSize, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def placePrevNextButtons(self, screen, iScreenW, iScreenH):
		iSize, iGap, iBaseY = self._getTransportLayout(iScreenW, iScreenH)
		iPrevX = self._getTransportButtonX(iScreenW, 1)
		iNextX = self._getTransportButtonX(iScreenW, 5)
		screen.setImageButton(self.prevId, self.prevButtonPath, iPrevX, iBaseY, iSize, iSize, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.setImageButton(self.nextId, self.nextButtonPath, iNextX, iBaseY, iSize, iSize, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def placeGroupSkipButtons(self, screen, iScreenW, iScreenH):
		iSize, iGap, iBaseY = self._getTransportLayout(iScreenW, iScreenH)
		iPrevGroupX = self._getTransportButtonX(iScreenW, 0)
		iNextGroupX = self._getTransportButtonX(iScreenW, 6)
		screen.setImageButton(self.prevGroupId, self.prevGroupButtonPath, iPrevGroupX, iBaseY, iSize, iSize, WidgetTypes.WIDGET_GENERAL, -1, -1)
		screen.setImageButton(self.nextGroupId, self.nextGroupButtonPath, iNextGroupX, iBaseY, iSize, iSize, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def placeToggleButton(self, screen, iScreenW, iScreenH):
		if not self.toggleButtonPath:
			return
		iSize, iGap, iBaseY = self._getTransportLayout(iScreenW, iScreenH)
		iFlipX = self._getTransportButtonX(iScreenW, 3)
		screen.setImageButton(self.toggleId, self.toggleButtonPath, iFlipX, iBaseY, iSize, iSize, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def _getTransportLayout(self, iScreenW, iScreenH):
		iSize = 64
		iGap = 8
		iBaseY = iScreenH - 74
		return (iSize, iGap, iBaseY)

	def _getTransportButtonX(self, iScreenW, iIndex):
		iSize = 64
		iGap = 8
		iCount = 7
		iTotalW = (iCount * iSize) + ((iCount - 1) * iGap)
		iLeftX = iScreenW / 2 - iTotalW / 2
		return iLeftX + iIndex * (iSize + iGap)

	def placeQueueList(self, screen, iScreenW, iScreenH, items, currentIndex, groupByIndex, groupLabels, itemIcons):
		if (items is None) or (len(items) == 0):
			return

		iPanelW = 320
		iPanelX = iScreenW - iPanelW - 20
		iPanelY = 50
		iPanelH = iScreenH - 140
		if iPanelH <= 0:
			return

		try:
			screen.deleteWidget(self.queuePanelId)
		except:
			pass
		try:
			screen.deleteWidget(self.queueListId)
		except:
			pass

		screen.addPanel(self.queuePanelId, "", "", True, False, iPanelX, iPanelY, iPanelW, iPanelH, PanelStyles.PANEL_STYLE_BLUE50)
		iIconColW = 24
		iTableX = iPanelX + 6
		iTableY = iPanelY + 6
		iTableW = iPanelW - 12
		iTableH = iPanelH - 12
		screen.addTableControlGFC(self.queueListId, 2, iTableX, iTableY, iTableW, iTableH, False, False, 22, 22, TableStyles.TABLE_STYLE_STANDARD)
		screen.setTableColumnHeader(self.queueListId, 0, "", iIconColW)
		screen.setTableColumnHeader(self.queueListId, 1, "", iTableW - iIconColW - 4)
		screen.enableSelect(self.queueListId, False)

		iRowH = 24
		iMaxRows = (iTableH - 4) / iRowH
		if iMaxRows <= 0:
			iMaxRows = 1

		iStart = 0
		if currentIndex >= 0:
			iStart = currentIndex - (iMaxRows / 2)
			if iStart < 0:
				iStart = 0
		if iStart >= len(items):
			iStart = max(0, len(items) - 1)

		i = iStart
		iRows = 0
		iLastGroup = -1
		iCurrentGroup = -1
		if (groupByIndex is not None) and (currentIndex >= 0) and (currentIndex < len(groupByIndex)):
			iCurrentGroup = groupByIndex[currentIndex]

		while i < len(items) and iRows < iMaxRows:
			iGroup = -1
			if (groupByIndex is not None) and (i < len(groupByIndex)):
				iGroup = groupByIndex[i]

			if iGroup != iLastGroup and (groupLabels is not None) and (iGroup >= 0) and (iGroup < len(groupLabels)) and iRows < iMaxRows:
				# <!-- custom: Avoid orphan group headers at the bottom of the visible list.
				# This media table inserts spacer + header rows when changing era/group; without a row-budget check, scrolling can end with a header as the last visible row (e.g. "Tech Quotes (Medieval)"), which looks rough.
				# Only add the group transition rows when there is still room for at least one tech item row after them. (GPT-5.3-Codex) -->
				iRowsNeededBeforeItem = 0
				if iLastGroup != -1:
					iRowsNeededBeforeItem += 1
				if groupLabels[iGroup]:
					iRowsNeededBeforeItem += 1
				if iRows + iRowsNeededBeforeItem + 1 > iMaxRows:
					iLastGroup = iGroup
				else:
					if iLastGroup != -1 and iRows < iMaxRows:
						iRow = screen.appendTableRow(self.queueListId)
						screen.setTableText(self.queueListId, 1, iRow, SASTextScale.labelText(u" "), "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						iRows += 1
					szHeader = groupLabels[iGroup]
					if szHeader:
						if iGroup == iCurrentGroup:
							szHeader = u">> " + szHeader
						szHeader = SASTextScale.labelText(szHeader)
						iRow = screen.appendTableRow(self.queueListId)
						screen.setTableText(self.queueListId, 1, iRow, szHeader, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
						iRows += 1
				iLastGroup = iGroup
				if iRows >= iMaxRows:
					break

			szLabel = items[i]
			if i == currentIndex:
				# <!-- custom: keep selected queue row visually distinct (bold), not only prefixed by ">", so active media is easy to track while browsing groups. (GPT-5.3-Codex) -->
				szLabel = u"> " + szLabel
				szLabel = sasFontTagLabel.bold + szLabel + SAS_FONT_TAG_CLOSE
			else:
				szLabel = SASTextScale.labelText(szLabel)
			szButton = ""
			if (itemIcons is not None) and (i < len(itemIcons)):
				szButton = itemIcons[i]
			iRow = screen.appendTableRow(self.queueListId)
			screen.setTableText(self.queueListId, 0, iRow, "", szButton, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableText(self.queueListId, 1, iRow, szLabel, "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			iRows += 1
			i += 1

	def clearQueueList(self, screen):
		try:
			screen.deleteWidget(self.queuePanelId)
		except:
			pass
		try:
			screen.deleteWidget(self.queueListId)
		except:
			pass

	def placeTvPanel(self, screen, iScreenW, iScreenH):
		iPanelX = 20
		iPanelY = 50
		iPanelW = iScreenW - 320 - 60
		iPanelH = iScreenH - 140
		if iPanelW <= 200 or iPanelH <= 120:
			iPanelX = 20
			iPanelY = 50
			iPanelW = iScreenW - 40
			iPanelH = iScreenH - 140

		try:
			screen.deleteWidget(self.tvPanelId)
		except:
			pass

		screen.addPanel(self.tvPanelId, "", "", True, False, iPanelX, iPanelY, iPanelW, iPanelH, PanelStyles.PANEL_STYLE_BLUE50)

		iPad = 10
		iBoxW = iPanelW - iPad * 2
		iBoxH = iPanelH - iPad * 2
		if iBoxW <= 0 or iBoxH <= 0:
			return (iPanelX, iPanelY, iPanelW, iPanelH, iPanelX, iPanelY, iPanelW, iPanelH)

		iMediaW = iBoxW
		iMediaH = iMediaW * 2 / 3
		if iMediaH > iBoxH:
			iMediaH = iBoxH
			iMediaW = iMediaH * 3 / 2
		iMediaX = iPanelX + (iPanelW - iMediaW) / 2
		iMediaY = iPanelY + (iPanelH - iMediaH) / 2
		return (iPanelX, iPanelY, iPanelW, iPanelH, iMediaX, iMediaY, iMediaW, iMediaH)

	def clearTvPanel(self, screen):
		try:
			screen.deleteWidget(self.tvPanelId)
		except:
			pass

	def placeTextPanel(self, screen, iScreenW, iScreenH, szText):
		if not szText:
			return

		iSize, iGap, iBaseY = self._getTransportLayout(iScreenW, iScreenH)
		# Get position of last button (index 6)
		iLastButtonX = self._getTransportButtonX(iScreenW, 6)

		# Text panel starts after the last button
		iTextX = iLastButtonX + iSize + iGap * 2
		iTextY = iBaseY
		iTextW = iScreenW - iTextX - 10
		iTextH = iSize

		# Only show if there's enough space
		if iTextW <= 100:
			return

		szTextPanelId = self.clickPrefix + "TextPanel"
		try:
			screen.deleteWidget(szTextPanelId)
		except:
			pass

		screen.addPanel(szTextPanelId, "", "", True, True, iTextX, iTextY, iTextW, iTextH, PanelStyles.PANEL_STYLE_EMPTY)
		screen.attachMultilineText(szTextPanelId, "Text", SASTextScale.labelText(szText), WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placePrevNextLabels(self, screen, iScreenW, iScreenH, szPrev, szNext):
		iSize, iGap, iBaseY = self._getTransportLayout(iScreenW, iScreenH)
		iPrevX = self._getTransportButtonX(iScreenW, 1)
		iNextX = self._getTransportButtonX(iScreenW, 5)
		iLabelY = iBaseY - 26
		if szPrev:
			screen.setLabel(self.prevId + "Label", "Background", SASTextScale.labelText(szPrev), CvUtil.FONT_CENTER_JUSTIFY, iPrevX + iSize / 2, iLabelY, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		if szNext:
			screen.setLabel(self.nextId + "Label", "Background", SASTextScale.labelText(szNext), CvUtil.FONT_CENTER_JUSTIFY, iNextX + iSize / 2, iLabelY, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def setReplayCallback(self, replayCallback):
		self.replayCallback = replayCallback

	def setPrevCallback(self, prevCallback):
		self.prevCallback = prevCallback

	def setNextCallback(self, nextCallback):
		self.nextCallback = nextCallback

	def setPrevGroupCallback(self, prevGroupCallback):
		self.prevGroupCallback = prevGroupCallback

	def setNextGroupCallback(self, nextGroupCallback):
		self.nextGroupCallback = nextGroupCallback

	def setToggleButton(self, szArtInfoType):
		self.toggleButtonPath = ArtFileMgr.getInterfaceArtInfo(szArtInfoType).getPath()

	def setToggleCallback(self, toggleCallback):
		self.toggleCallback = toggleCallback

	def playSound(self, szSoundScript, iSoundId, bForce3D):
		self.soundId = None
		self.is3DSound = False

		if (not szSoundScript) and iSoundId == -1:
			return

		try:
			if iSoundId != -1:
				if bForce3D:
					sz3DScript = _SAS_getAudio3DScriptName(iSoundId)
					if sz3DScript:
						self.soundId = CyAudioGame().Play3DSound(sz3DScript, -1, -1, -1)
						# <!-- custom: Python receives 3D audio script IDs for civ Select/Order previews.
						# The old positional `Play3DSoundWithId(iSoundId, 0, 0, 0)` played very quietly after entering a game, while `playGeneralSound(sz3DScript)` fixed volume but returned no stop handle.
						# `Play3DSound(sz3DScript, -1, -1, -1)` fixed volume too and returns a handle for `Destroy3DSound`, so use it as the general 3D preview path. See KI#141. (GPT-5.5) -->
						self.is3DSound = True
						return
					self.soundId = CyAudioGame().Play3DSoundWithId(iSoundId, 0, 0, 0)
					self.is3DSound = True
					return
				else:
					self.soundId = CyAudioGame().Play2DSoundWithId(iSoundId)
			else:
				if szSoundScript.startswith("AS3D_"):
					self.soundId = CyAudioGame().Play3DSound(szSoundScript, -1, -1, -1)
					# <!-- custom: Raw AS3D_ script-name previews use the same non-positional 3D playback as civ Select/Order IDs above.
					# This fixed normal volume and returns a Destroy3DSound-compatible handle, avoiding the playGeneralSound no-handle persistence issue. See KI#141. (GPT-5.5) -->
					self.is3DSound = True
					return
				else:
					self.soundId = CyAudioGame().Play2DSound(szSoundScript)
		except:
			self.soundId = None

		if self.soundId is None or self.soundId == -1:
			if szSoundScript:
				CyInterface().playGeneralSound(szSoundScript)
			self.soundId = None
			self.is3DSound = False

	def stopSound(self):
		if self.soundId is not None and self.soundId != -1:
			try:
				if self.is3DSound:
					CyAudioGame().Destroy3DSound(self.soundId)
				else:
					CyAudioGame().Destroy2DSound(self.soundId)
			except:
				try:
					CyInterface().stop2DSound()
				except:
					pass
			self.soundId = None
			self.is3DSound = False
		elif self.soundId is not None:
			try:
				CyInterface().stop2DSound()
			except:
				pass
			self.soundId = None
			self.is3DSound = False

	def closeScreen(self):
		screen = CyGInterfaceScreen(self.screenId, self.screenEnum)
		screen.hideScreen()
		self.isOpen = False
		self.screen = None
		self.timerRunning = False

	def getScreen(self):
		return self.screen

	def placeTimerLabel(self, screen, iScreenW, iScreenH):
		self.timerLabelX = 20
		self.timerLabelY = iScreenH - 34
		self.currentLabelX = self.timerLabelX + 80
		self.currentLabelY = self.timerLabelY
		self._updateTimerLabel()

	def setCurrentLabel(self, screen, szLabel):
		if not szLabel:
			return
		if self.currentLabelX <= 0:
			return
		screen.setLabel(self.currentLabelId, "Background", SASTextScale.labelText(szLabel), CvUtil.FONT_LEFT_JUSTIFY, self.currentLabelX, self.currentLabelY, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def startTimer(self):
		self.timerSeconds = 0
		self.timerRemainder = 0.0
		self.timerRunning = True
		self._updateTimerLabel()

	def resetTimer(self):
		self.timerSeconds = 0
		self.timerRemainder = 0.0
		self.timerRunning = True
		self._updateTimerLabel()

	def updateTimer(self, fDelta):
		if (not self.timerRunning) or (self.screen is None):
			return
		self.timerRemainder += fDelta
		if self.timerRemainder < 1.0:
			return
		iTicks = int(self.timerRemainder)
		self.timerRemainder = self.timerRemainder - iTicks
		self.timerSeconds += iTicks
		self._updateTimerLabel()

	def _updateTimerLabel(self):
		if self.screen is None:
			return
		szText = self._formatElapsed(self.timerSeconds)
		self.screen.setLabel(self.timerLabelId, "Background", SASTextScale.labelText(szText), CvUtil.FONT_LEFT_JUSTIFY, self.timerLabelX, self.timerLabelY, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def _formatElapsed(self, iSeconds):
		iSec = iSeconds % 60
		iMin = (iSeconds / 60) % 60
		iHour = iSeconds / 3600
		if iHour > 0:
			return u"%d:%02d:%02d" % (iHour, iMin, iSec)
		return u"%02d:%02d" % (iMin, iSec)

	def handleInput(self, inputClass, closeCallback, bCloseOnMovieDone):
		if not self.isOpen:
			return False

		# <!-- custom: While the full-screen music player overlay is open, consume input here so Civ4 does not treat ESC/ENTER as "close civilopedia" and bounce to the main menu. (ChatGPT-5.2 Thinking) -->
		iNotify = inputClass.getNotifyCode()

		if bCloseOnMovieDone and iNotify == NotifyCode.NOTIFY_MOVIE_DONE:
			closeCallback()
			return True

		if iNotify == NotifyCode.NOTIFY_CHARACTER:
			iKey = inputClass.getData()
			if iKey == int(InputTypes.KB_ESCAPE) or iKey == int(InputTypes.KB_RETURN):
				closeCallback()
				return True
			# Some keyboards send a distinct numpad enter key code.
			try:
				if iKey == int(InputTypes.KB_NUMPADENTER):
					closeCallback()
					return True
			except:
				pass

		if iNotify == NotifyCode.NOTIFY_CLICKED:
			szName = inputClass.getFunctionName()
			if szName == self.replayId:
				if self.replayCallback is not None:
					self.replayCallback()
				return True
			if szName == self.prevId:
				if self.prevCallback is not None:
					self.prevCallback()
				return True
			if szName == self.nextId:
				if self.nextCallback is not None:
					self.nextCallback()
				return True
			if szName == self.prevGroupId:
				if self.prevGroupCallback is not None:
					self.prevGroupCallback()
				return True
			if szName == self.nextGroupId:
				if self.nextGroupCallback is not None:
					self.nextGroupCallback()
				return True
			if szName == self.toggleId:
				if self.toggleCallback is not None:
					self.toggleCallback()
				return True
			# Exit button, or click anywhere on the overlay (image/title/background or movie/nif/dds/title/background) to close.
			if szName == self.exitId or szName.startswith(self.clickPrefix):
				closeCallback()
				return True

		return True
