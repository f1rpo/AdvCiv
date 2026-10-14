# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: SASGameRecord's session-configuration timeline records user/config settings that are not part of game/save state.
# A full snapshot is emitted after GameStart/OnLoad handlers finish; later BUG/options-screen callbacks resample only their small setting families and emit changed values.
# This keeps benchmark/support context verifiable without per-turn polling or repeated INI parsing.
# Configured profile/file values are labelled as such and are not claimed to be adopted immediately by restart-cached consumers. (ChatGPT-5.6-Sol) -->

from CvPythonExtensions import CyGame, CyGlobalContext, CyUserProfile, PlayerOptionTypes, GraphicOptionTypes
import SASGameRecordConfig # <!-- custom: Share lazy None-initialized level caches across recorder callbacks; disabled runs still avoid importing the settings collectors. (GPT-6.1-Sol) -->
import BugOptions
import BugPath
import os

try:
	unicode
except NameError:
	unicode = str

_SAS_NUMERIC_STRING_BUG_OPTIONS = {
	"MainInterface__FieldOfView_Value": True,
}

_SAS_CIV4_INI_KEYS = (
	"FullScreen",
	"ScreenWidth",
	"ScreenHeight",
	"SetMaxFrameRate",
	"D3D9Query",
	"DynamicAnimPaging",
	"MemSaver",
	"DisableFileCaching",
	"DisableCaching",
	"MinimapTrilinearFilter",
	"MouseScrolling",
	# <!-- custom: Ctrl+Alt+F requests were accepted without a Flying-state change while allowflying=0; enabling it and restarting produced both transitions. Preserve this startup setting beside the camera evidence. (GPT-6.1-Sol) -->
	"AllowFlying",
	"HidePythonExceptions",
	"MaxAutoSaves",
	"AutoSaveInterval",
	"LoggingEnabled",
	"OverwriteLogs",
	"RandLog",
	"SynchLog",
	"MessageLog",
	"PlayIntro",
	"BinkNoSkip",
	"HideMinSpecWarning",
	"NoSound",
	"MaxFileHandles",
	"SyncInput",
)

_SAS_MOD_INI_KEYS = (
	"ModularLoading",
	"SkipMainMenu",
	"NoCustomArt",
	"NoCustomAssets",
	"NoCustomScenario",
	"NoTeams",
	"ForceGameOptions",
	"ForceStandardEra",
	"ForceScenario",
	"SinglePlayerOnly",
	"AllowPublicMaps",
)

_g_profileGroups = {}
_g_bugGroups = {}
_g_sessionActive = False


def _plain(value):
	if value is None:
		return "-"
	if value is True:
		return "1"
	if value is False:
		return "0"
	# <!-- custom: Fixed 12-digit formatting hid small float changes; repr preserves round-trip float precision for the settings timeline. (GPT-6.1-Sol) -->
	if isinstance(value, float):
		return repr(value)
	if isinstance(value, unicode):
		return value.encode("utf-8")
	return str(value)


def _safeConfiguredScalar(value):
	value = _plain(value)
	try:
		text = value.decode("utf-8")
	# <!-- custom: Python 2.4 includes these shutdown/interrupt signals under Exception; explicitly re-raise them before handling ordinary diagnostic errors. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
	except (KeyboardInterrupt, SystemExit):
		raise
	except Exception:
		text = value
	if len(text) > 32:
		return "<NONSCALAR>"
	lower = text.lower()
	if lower in ("true", "false", "yes", "no", "on", "off"):
		return value
	if text == "":
		return value
	for char in text:
		if char not in "0123456789-+.":
			return "<NONSCALAR>"
	return value


def _safeColorValue(value):
	value = _plain(value)
	try:
		text = value.decode("utf-8")
	# <!-- custom: Python 2.4 includes these shutdown/interrupt signals under Exception; explicitly re-raise them before handling ordinary diagnostic errors. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
	except (KeyboardInterrupt, SystemExit):
		raise
	except Exception:
		text = value
	if not text.startswith("COLOR_") or len(text) > 64:
		return "<CUSTOM_COLOR>"
	for char in text[6:]:
		if not ((char >= "A" and char <= "Z") or (char >= "0" and char <= "9") or char == "_"):
			return "<CUSTOM_COLOR>"
	return value


def _numericConfiguredString(value):
	try:
		return _plain(float(value))
	# <!-- custom: Python 2.4 includes these shutdown/interrupt signals under Exception; explicitly re-raise them before handling ordinary diagnostic errors. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
	except (KeyboardInterrupt, SystemExit):
		raise
	except Exception:
		return "<NON_NUMERIC>"


def _escape(value):
	value = _plain(value)
	if isinstance(value, unicode):
		value = value.encode("utf-8")
	result = []
	for char in value:
		if isinstance(char, int):
			code = char
			char = chr(char)
		else:
			code = ord(char)
		if ((code >= ord("a") and code <= ord("z")) or
				(code >= ord("A") and code <= ord("Z")) or
				(code >= ord("0") and code <= ord("9")) or char in "-_."):
			result.append(char)
		else:
			result.append("%%%02X" % code)
	return "".join(result)


def _serialize(values):
	parts = []
	for key in sorted(values.keys()):
		parts.append("%s=%s" % (_escape(key), _escape(values[key])))
	return ";".join(parts)


def _serializeChanges(oldValues, newValues):
	changes = {}
	keys = {}
	for key in oldValues.keys():
		keys[key] = True
	for key in newValues.keys():
		keys[key] = True
	for key in sorted(keys.keys()):
		oldValue = oldValues.get(key, "<MISSING>")
		newValue = newValues.get(key, "<MISSING>")
		if oldValue != newValue:
			changes[key] = "%s>%s" % (_escape(oldValue), _escape(newValue))
	parts = []
	for key in sorted(changes.keys()):
		parts.append("%s=%s" % (_escape(key), changes[key]))
	return len(changes), ";".join(parts)


def _emit(event, reason, source, group, values, serialized=None, count=None):
	if serialized is None:
		serialized = _serialize(values)
	if count is None:
		count = len(values)
	CyGame().logSASGameRecordSessionSettings(event, reason, source, group, count, serialized)


def _effectivePlayerOptionValues(gc, game):
	values = {}
	iActivePlayer = game.getActivePlayer()
	if iActivePlayer < 0:
		return values
	for iOption in range(PlayerOptionTypes.NUM_PLAYEROPTION_TYPES):
		key = gc.getPlayerOptionsInfoByIndex(iOption).getType()
		values[key] = _plain(gc.getPlayer(iActivePlayer).isOption(iOption))
	return values


def _configuredPlayerOptionValues(gc, profile):
	values = {}
	for iOption in range(PlayerOptionTypes.NUM_PLAYEROPTION_TYPES):
		key = gc.getPlayerOptionsInfoByIndex(iOption).getType()
		values[key] = _plain(profile.getPlayerOption(iOption))
	return values


def _graphicOptionValues(gc, profile):
	values = {}
	for iOption in range(GraphicOptionTypes.NUM_GRAPHICOPTION_TYPES):
		key = gc.getGraphicOptionsInfoByIndex(iOption).getType()
		values[key] = _plain(profile.getGraphicOption(iOption))
	return values


def _graphicsProfileValues(profile):
	iAntiAliasing = profile.getAntiAliasing()
	iSamples = -1
	if iAntiAliasing == 0:
		iSamples = 0
	elif iAntiAliasing > 0 and iAntiAliasing < 31:
		iSamples = 1 << iAntiAliasing
	iResolution = profile.getResolution()
	values = {
		"antiAliasingIndex": _plain(iAntiAliasing),
		"antiAliasingSamples": _plain(iSamples),
		"globeViewRenderLevel": _plain(profile.getGlobeViewRenderLevel()),
		"movieQualityLevel": _plain(profile.getMovieQualityLevel()),
		"renderQualityLevel": _plain(profile.getRenderQualityLevel()),
		"resolutionIndex": _plain(iResolution),
	}
	try:
		values["resolution"] = _plain(profile.getResolutionString(iResolution))
	# <!-- custom: Python 2.4 includes these shutdown/interrupt signals under Exception; explicitly re-raise them before handling ordinary diagnostic errors. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
	except (KeyboardInterrupt, SystemExit):
		raise
	except Exception:
		values["resolution"] = "-"
	return values


def _audioProfileValues(profile):
	return {
		"ambienceNoSound": _plain(profile.isAmbienceNoSound()),
		"ambienceVolume": _plain(profile.getAmbienceVolume()),
		"captureDeviceIndex": _plain(profile.getCaptureDeviceIndex()),
		"captureVolume": _plain(profile.getCaptureVolume()),
		"customMusicConfigured": _plain(profile.getMusicPath() != ""),
		"interfaceNoSound": _plain(profile.isInterfaceNoSound()),
		"interfaceVolume": _plain(profile.getInterfaceVolume()),
		"masterNoSound": _plain(profile.isMasterNoSound()),
		"masterVolume": _plain(profile.getMasterVolume()),
		"musicNoSound": _plain(profile.isMusicNoSound()),
		"musicVolume": _plain(profile.getMusicVolume()),
		"playbackDeviceIndex": _plain(profile.getPlaybackDeviceIndex()),
		"playbackVolume": _plain(profile.getPlaybackVolume()),
		"soundEffectsNoSound": _plain(profile.isSoundEffectsNoSound()),
		"soundEffectsVolume": _plain(profile.getSoundEffectsVolume()),
		"speakerConfig": _plain(profile.getSpeakerConfig()),
		"speechNoSound": _plain(profile.isSpeechNoSound()),
		"speechVolume": _plain(profile.getSpeechVolume()),
		"useVoice": _plain(profile.useVoice()),
	}


def _viewProfileValues(profile):
	# <!-- custom: These are saved profile preferences, not proof of current engine/UI visibility.
	# Live state is recorded separately from native getters at recorder level 2+. (ChatGPT-5.6-Sol) -->
	return {
		"bareMap": _plain(profile.getMap()),
		"grid": _plain(profile.getGrid()),
		"scores": _plain(profile.getScores()),
		"yields": _plain(profile.getYields()),
	}


def _miscProfileValues(game, profile):
	return {
		"clock24Hours": _plain(profile.is24Hours()),
		"clockOn": _plain(profile.isClockOn()),
		"languageIndex": _plain(game.getCurrentLanguage()),
		"modem": _plain(game.isModem()),
	}


def _collectProfileGroups(bIncludeEffectivePlayerOptions=True):
	gc = CyGlobalContext()
	game = CyGame()
	iSystemContextLevel = SASGameRecordConfig.getSystemContextLevel()
	groups = {}
	# <!-- custom: CvPlayer options can still be awaiting EXE profile synchronization at GameStart.
	# Keep the instantaneous effective baseline for chronology, but native CvPlayer::setOption logs every later application exactly; options-screen resampling deliberately excludes this group. (ChatGPT-5.6-Sol) -->
	if bIncludeEffectivePlayerOptions and game.getActivePlayer() >= 0:
		groups["CIV4_PLAYER_OPTIONS_EFFECTIVE"] = ("ACTIVE_PLAYER_EFFECTIVE", _effectivePlayerOptionValues(gc, game))
	# <!-- custom: Preserve the existing system-context privacy contract: level 0 does not newly expose configured profile values merely because the session-settings timeline is enabled. (ChatGPT-5.6-Sol) -->
	if iSystemContextLevel > 0:
		profile = CyUserProfile()
		groups["CIV4_PLAYER_OPTIONS_CONFIGURED"] = ("USER_PROFILE_CONFIGURED", _configuredPlayerOptionValues(gc, profile))
		groups["CIV4_GRAPHIC_OPTIONS"] = ("USER_PROFILE_CONFIGURED", _graphicOptionValues(gc, profile))
		groups["CIV4_GRAPHICS_PROFILE"] = ("USER_PROFILE_CONFIGURED", _graphicsProfileValues(profile))
		groups["CIV4_AUDIO_PROFILE"] = ("USER_PROFILE_CONFIGURED", _audioProfileValues(profile))
		groups["CIV4_MISC_PROFILE"] = ("USER_PROFILE_CONFIGURED", _miscProfileValues(game, profile))
		groups["CIV4_VIEW_PROFILE"] = ("USER_PROFILE_CONFIGURED", _viewProfileValues(profile))
	return groups


def _collectBugGroups():
	options = BugOptions.getOptions().options
	groups = {}
	iLinkedSkipped = 0
	iUnsavedSkipped = 0
	iParameterizedSkipped = 0
	iStringSkipped = 0
	iRecorded = 0
	for optionID in sorted(options.keys()):
		option = options[optionID]
		if option.__class__.__name__.startswith("Linked"):
			iLinkedSkipped += 1
			continue
		# <!-- custom: BUG UnsavedOption values are transient interface/game state, not persistent user configuration.
		# Keep the settings timeline focused on INI-backed options rather than turning every internal UI state change into configuration noise. (ChatGPT-5.6-Sol) -->
		if not hasattr(option, "getFile"):
			iUnsavedSkipped += 1
			continue
		if hasattr(option, "isParameterized") and option.isParameterized():
			iParameterizedSkipped += 1
			continue
		if option.getType() == "string" and optionID not in _SAS_NUMERIC_STRING_BUG_OPTIONS:
			iStringSkipped += 1
			continue
		group = optionID.split("__", 1)[0]
		if group not in groups:
			groups[group] = {}
		try:
			value = option.getValue()
		# <!-- custom: Python 2.4 includes these shutdown/interrupt signals under Exception; explicitly re-raise them before handling ordinary diagnostic errors. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
		except (KeyboardInterrupt, SystemExit):
			raise
		except Exception:
			value = "<UNREADABLE>"
		if optionID in _SAS_NUMERIC_STRING_BUG_OPTIONS:
			groups[group][optionID] = _numericConfiguredString(value)
		elif option.getType() == "color":
			groups[group][optionID] = _safeColorValue(value)
		else:
			groups[group][optionID] = _plain(value)
		iRecorded += 1
	groups["BUG_META"] = {
		"linkedSkipped": _plain(iLinkedSkipped),
		"parameterizedSkipped": _plain(iParameterizedSkipped),
		"recorded": _plain(iRecorded),
		"registered": _plain(len(options)),
		"stringSkipped": _plain(iStringSkipped),
		"unsavedSkipped": _plain(iUnsavedSkipped),
	}
	result = {}
	for group in groups.keys():
		source = "BUG_RUNTIME_VALUE"
		if group == "BUG_META":
			source = "RECORDER_META"
		result[group] = (source, groups[group])
	return result


def _caseInsensitiveGet(mapping, key):
	try:
		keys = mapping.keys()
	# <!-- custom: Python 2.4 includes these shutdown/interrupt signals under Exception; explicitly re-raise them before handling ordinary diagnostic errors. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
	except (KeyboardInterrupt, SystemExit):
		raise
	except Exception:
		return None
	lower = key.lower()
	for candidate in keys:
		if str(candidate).lower() == lower:
			return mapping[candidate]
	return None


def _readIniValues(path, keys):
	values = {"readable": "0"}
	if not path or not os.path.isfile(path):
		return values
	try:
		from configobj import ConfigObj
		config = ConfigObj(path)
		section = _caseInsensitiveGet(config, "CONFIG")
		if section is None:
			section = config
		values["readable"] = "1"
		for key in keys:
			value = _caseInsensitiveGet(section, key)
			if value is not None:
				values[key] = _safeConfiguredScalar(value)
	# <!-- custom: Python 2.4 includes these shutdown/interrupt signals under Exception; explicitly re-raise them before handling ordinary diagnostic errors. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
	except (KeyboardInterrupt, SystemExit):
		raise
	except Exception:
		values["readable"] = "0"
	return values


def _findCiv4Ini():
	game = CyGame()
	argument = game.getSASGameRecordIniFileArgument()
	if argument:
		candidates = []
		if os.path.isabs(argument) or (len(argument) >= 2 and argument[1] == ":"):
			candidates.append(argument)
		else:
			candidates.append(os.path.join(BugPath.getAppDir(), argument))
			candidates.append(os.path.join(BugPath.getRootDir(), argument))
			candidates.append(argument)
		for path in candidates:
			if os.path.isfile(path):
				return path, "COMMAND_LINE_OVERRIDE"
		return None, "COMMAND_LINE_OVERRIDE_UNRESOLVED"
	return os.path.join(BugPath.getRootDir(), "CivilizationIV.ini"), "DEFAULT"


def _collectConfiguredFiles():
	groups = {}
	civ4Path, civ4Source = _findCiv4Ini()
	civ4Values = _readIniValues(civ4Path, _SAS_CIV4_INI_KEYS)
	civ4Values["activeIniSource"] = civ4Source
	if civ4Values.get("readable") == "1":
		try:
			from configobj import ConfigObj
			config = ConfigObj(civ4Path)
			section = _caseInsensitiveGet(config, "CONFIG")
			if section is None:
				section = config
			cheat = _caseInsensitiveGet(section, "CheatCode")
			mod = _caseInsensitiveGet(section, "Mod")
			civ4Values["cheatCodeConfigured"] = _plain(cheat not in (None, "", "0", "false", "False"))
			civ4Values["modOverrideConfigured"] = _plain(mod not in (None, "", "0"))
		# <!-- custom: Python 2.4 includes these shutdown/interrupt signals under Exception; explicitly re-raise them before handling ordinary diagnostic errors. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
		except (KeyboardInterrupt, SystemExit):
			raise
		except Exception:
			pass
	groups["CIV4_INI_CONFIGURED"] = ("CONFIGURED_STARTUP_FILE", civ4Values)

	try:
		modPath = os.path.join(BugPath.getModDir(), BugPath.getModFolder() + ".ini")
	# <!-- custom: Python 2.4 includes these shutdown/interrupt signals under Exception; explicitly re-raise them before handling ordinary diagnostic errors. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
	except (KeyboardInterrupt, SystemExit):
		raise
	except Exception:
		modPath = None
	modValues = _readIniValues(modPath, _SAS_MOD_INI_KEYS)
	groups["MOD_INI_CONFIGURED"] = ("CONFIGURED_STARTUP_FILE", modValues)
	return groups


def _collectSafely(collector):
	try:
		return collector(), True
	# <!-- custom: Python 2.4 includes these shutdown/interrupt signals under Exception; explicitly re-raise them before handling ordinary diagnostic errors. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
	except (KeyboardInterrupt, SystemExit):
		raise
	except Exception:
		return {}, False


def _snapshotGroups(groups, reason):
	state = {}
	for group in sorted(groups.keys()):
		source, values = groups[group]
		_emit("SNAPSHOT", reason, source, group, values)
		state[group] = (source, values)
	return state


def _changeGroups(previous, current, reason):
	for group in sorted(current.keys()):
		source, values = current[group]
		oldSource, oldValues = previous.get(group, (source, {}))
		count, serialized = _serializeChanges(oldValues, values)
		if count > 0:
			_emit("CHANGE", reason, source, group, values, serialized, count)
		previous[group] = (source, values)


def logSessionSettings(reason):
	global _g_profileGroups, _g_bugGroups, _g_sessionActive
	profileGroups, bProfileReadable = _collectSafely(_collectProfileGroups)
	bugGroups, bBugReadable = _collectSafely(_collectBugGroups)
	configuredGroups, bConfiguredCollectionSucceeded = _collectSafely(_collectConfiguredFiles)
	_g_profileGroups = _snapshotGroups(profileGroups, reason)
	_g_bugGroups = _snapshotGroups(bugGroups, reason)
	for group, pair in configuredGroups.items():
		source, values = pair
		_emit("SNAPSHOT", reason, source, group, values)
	meta = {
		"bugGroups": _plain(len(bugGroups)),
		"bugReadable": _plain(bBugReadable),
		"configuredFileGroups": _plain(len(configuredGroups)),
		# <!-- custom: Completing the collector does not prove any INI was readable; each configured-file group reports its own readable flag. (GPT-6.1-Sol) -->
		"configuredFileCollectionSucceeded": _plain(bConfiguredCollectionSucceeded),
		"profileGroups": _plain(len(profileGroups)),
		"profileReadable": _plain(bProfileReadable),
	}
	_emit("SNAPSHOT", reason, "RECORDER_META", "SESSION_SETTINGS_META", meta)
	# <!-- custom: Transient map-view state is diagnostic detail rather than core configuration.
	# Snapshot it only at recorder level 2+, then native C++ hooks record exact later changes without polling. (ChatGPT-5.6-Sol) -->
	if SASGameRecordConfig.getLogLevel() >= 2:
		CyGame().logSASGameRecordRuntimeViewStateSnapshot(reason)
	_g_sessionActive = True


def _sessionCanRecord():
	global _g_sessionActive
	if not _g_sessionActive:
		return False
	try:
		if not CyGame().isFinalInitialized():
			_g_sessionActive = False
			return False
	# <!-- custom: Python 2.4 includes these shutdown/interrupt signals under Exception; explicitly re-raise them before handling ordinary diagnostic errors. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
	except (KeyboardInterrupt, SystemExit):
		raise
	except Exception:
		return False
	return True


def checkCiv4ProfileSettingsChanges(reason):
	if not _sessionCanRecord():
		return
	# <!-- custom: Resample configured profile values only.
	# Effective player-option transitions are logged natively by CvPlayer::setOption, so delayed startup synchronization cannot be misattributed to closing this screen. (ChatGPT-5.6-Sol) -->
	current, bReadable = _collectSafely(lambda: _collectProfileGroups(False))
	if bReadable:
		_changeGroups(_g_profileGroups, current, reason)


def checkBugSettingsChanges(reason):
	if not _sessionCanRecord():
		return
	current, bReadable = _collectSafely(_collectBugGroups)
	if bReadable:
		_changeGroups(_g_bugGroups, current, reason)
