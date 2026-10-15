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

# <!-- custom: CivilizationIV.ini is sampled once per session as configured-file evidence, never as proof that the EXE accepted or currently applies a value.
# Privacy is decided by reviewed key identity rather than by whether a value looks numeric; direct keys preserve the parsed configured text, including unusual/invalid values. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
_SAS_CIV4_INI_DIRECT_KEYS = (
	# <!-- custom: CheatCode is a documented public debug/autoplay switch (normally chipotle), not an account secret; retain its configured text so invalid values remain diagnosable. (GPT-6.1-Sol) -->
	"CheatCode",
	"FullScreen",
	"ScreenWidth",
	"ScreenHeight",
	"SetMaxFrameRate",
	"MapRandSeed",
	"SyncRandSeed",
	"QuickStart",
	"GenerateCrashDumps",
	"EnableVoice",
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
	"hapdebugger",
	"ModularLoading",
	"disablepakmemorymapping",
	"guimanagedtextures",
	"dynamicunitpaging",
	"field_of_view",
	"gameupdatetime",
	"mainmenumods",
	"nointromovie",
	"audioenable",
	"bink16bit",
	"forcenumlock",
	"binkinterlace",
	"noeventmessages",
	"playmusic",
	"hidemoviebackground",
	"notechsplash",
	"cuttrees",
	"endslides",
	"binkcopyall",
	"QuickHandicap",
	"gametype",
	"GameOptions",
	"VictoryConditions",
	"SeaLevel",
	"Climate",
	"WorldSize",
	"GameSpeed",
	"Era",
	"maxturns",
	"Language",
	"autorunturnlimit",
	"breakonalloc",
	"autorun",
	"port",
	"PitbossTurnTime",
	"pitbosssleep",
	"Bandwidth",
	"selectip",
	"screenshot format",
)

# <!-- custom: These free-text/identity/path-like values can disclose personal/local context.
# Keep their key presence visible but gate raw values behind a default-off INI-specific opt-in. (ChatGPT-5.6-Sol) -->
_SAS_CIV4_INI_PRIVATE_KEYS = (
	"GameName",
	"filename",
	"Nickname",
	"Alias",
	"UserProfile",
	"Map",
	"mod",
)

# <!-- custom: Network/contact/account identifiers get a separate default-off opt-in because they are more sensitive than ordinary local preferences.
# No password/secret value is approved here. (ChatGPT-5.6-Sol) -->
_SAS_CIV4_INI_NETWORK_KEYS = (
	"Email",
	"DirectIP",
	"PitbossSMTPHost",
	"PitbossSMTPLogin",
	"PitbossEmail",
)

_SAS_MOD_INI_DIRECT_KEYS = (
	"ModularLoading",
	"SkipMainMenu",
	"NoCustomArt",
	"NoCustomAssets",
	"NoCustomScenario",
	"NoTeams",
	"ForceGameOptions",
	"ForceStandardEra",
	"SinglePlayerOnly",
	"AllowPublicMaps",
)

# <!-- custom: AdvCiv-SAS.ini can carry filenames/free text as well as Boolean-style switches; reuse the private-value opt-in instead of assuming a nonnumeric value is harmless. (ChatGPT-5.6-Sol) -->
_SAS_MOD_INI_PRIVATE_KEYS = (
	"ForceScenario",
	"ImageFile",
	"Name",
	"Description",
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


def _keyPolicy(keys):
	result = {}
	for key in keys:
		result[str(key).lower()] = key
	return result


def _walkIniScalars(section, path, entries):
	for key in getattr(section, "scalars", []):
		entries.append((path, key, section[key]))
	for sectionName in getattr(section, "sections", []):
		_walkIniScalars(section[sectionName], path + (str(sectionName),), entries)


def _iniFieldName(path, key):
	if path:
		return ".".join(path) + "." + str(key)
	return "ROOT." + str(key)


def _configuredFlag(values, falseValues):
	for value in values:
		text = _plain(value)
		try:
			lower = text.decode("utf-8").strip().lower()
		except (KeyboardInterrupt, SystemExit):
			raise
		except Exception:
			lower = str(text).strip().lower()
		if lower not in falseValues:
			return "1"
	return "0"


# <!-- custom: Replace the former single selected-key list with separate direct/private/network policies and their opt-ins; return both report fields and parsed occurrences so presence flags reuse the same section-aware read. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
def _readIniValues(path, directKeys, privateKeys=(), networkKeys=(),
		bPrivateValuesEnabled=False, bNetworkValuesEnabled=False, bReportUnknown=False):
	values = {"readable": "0"}
	bPolicySummary = bool(privateKeys or networkKeys or bReportUnknown)
	if privateKeys:
		values["privateValuesEnabled"] = _plain(bPrivateValuesEnabled)
	if networkKeys:
		values["networkValuesEnabled"] = _plain(bNetworkValuesEnabled)
	found = {}
	if not path or not os.path.isfile(path):
		return values, found
	try:
		from configobj import ConfigObj
		# <!-- custom: Preserve configured scalar text instead of ConfigObj's list/interpolation semantics; section identity is retained in emitted field names so [CONFIG]/[DEBUG]/other keys cannot silently shadow one another. (ChatGPT-5.6-Sol) -->
		config = ConfigObj(path, list_values=False, interpolation=False)
		entries = []
		_walkIniScalars(config, (), entries)
		direct = _keyPolicy(directKeys)
		private = _keyPolicy(privateKeys)
		network = _keyPolicy(networkKeys)
		iDirect = 0
		iPrivate = 0
		iNetwork = 0
		iUnknown = 0
		for sectionPath, key, value in entries:
			lower = str(key).lower()
			found.setdefault(lower, []).append(value)
			field = _iniFieldName(sectionPath, key)
			if lower in direct:
				values[field] = _plain(value)
				iDirect += 1
			elif lower in private:
				if bPrivateValuesEnabled:
					values[field] = _plain(value)
				else:
					values[field] = "<WITHHELD_PRIVATE>"
				iPrivate += 1
			elif lower in network:
				if bNetworkValuesEnabled:
					values[field] = _plain(value)
				else:
					values[field] = "<WITHHELD_NETWORK>"
				iNetwork += 1
			elif bReportUnknown:
				# <!-- custom: Preserve unknown key identity for diagnostics without copying an unreviewed raw value that could contain personal data or secrets. (ChatGPT-5.6-Sol) -->
				values[field] = "<UNCLASSIFIED>"
				iUnknown += 1
		values["readable"] = "1"
		if bPolicySummary:
			known = {}
			for policy in (direct, private, network):
				known.update(policy)
			missing = [known[key] for key in known.keys() if key not in found]
			missing.sort(key=lambda value: str(value).lower())
			values["entryCount"] = _plain(len(entries))
			values["directValueCount"] = _plain(iDirect)
			values["privateValueCount"] = _plain(iPrivate)
			values["networkValueCount"] = _plain(iNetwork)
			values["unclassifiedValueCount"] = _plain(iUnknown)
			values["missingKnownCount"] = _plain(len(missing))
			# <!-- custom: Civ4 uses Python 2.4, which cannot parse conditional expressions; use an explicit branch even for this logging-only summary. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
			if missing:
				values["missingKnownKeys"] = ",".join(missing)
			else:
				values["missingKnownKeys"] = "-"
	# <!-- custom: Python 2.4 includes these shutdown/interrupt signals under Exception; explicitly re-raise them before handling ordinary diagnostic errors. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
	except (KeyboardInterrupt, SystemExit):
		raise
	except Exception:
		values["readable"] = "0"
	return values, found


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
	gc = CyGlobalContext()
	bPrivateValuesEnabled = gc.getDefineINT("SAS_GAME_RECORD_INI_PRIVATE_VALUES_ENABLE") != 0
	bNetworkValuesEnabled = gc.getDefineINT("SAS_GAME_RECORD_INI_NETWORK_VALUES_ENABLE") != 0

	civ4Path, civ4Source = _findCiv4Ini()
	civ4Values, civ4Found = _readIniValues(
		civ4Path,
		_SAS_CIV4_INI_DIRECT_KEYS,
		_SAS_CIV4_INI_PRIVATE_KEYS,
		_SAS_CIV4_INI_NETWORK_KEYS,
		bPrivateValuesEnabled,
		bNetworkValuesEnabled,
		True)
	civ4Values["activeIniSource"] = civ4Source
	# <!-- custom: Remove the second ConfigObj import/read formerly used only for CheatCode and Mod presence; reuse civ4Found from the reader above to avoid reparsing and the former CONFIG-only omission. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	if civ4Values.get("readable") == "1":
		civ4Values["cheatCodeConfigured"] = _configuredFlag(civ4Found.get("cheatcode", []), ("", "0", "false", "no", "off"))
		civ4Values["modOverrideConfigured"] = _configuredFlag(civ4Found.get("mod", []), ("", "0"))
	groups["CIV4_INI_CONFIGURED"] = ("CONFIGURED_STARTUP_FILE", civ4Values)

	try:
		modPath = os.path.join(BugPath.getModDir(), BugPath.getModFolder() + ".ini")
	# <!-- custom: Python 2.4 includes these shutdown/interrupt signals under Exception; explicitly re-raise them before handling ordinary diagnostic errors. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
	except (KeyboardInterrupt, SystemExit):
		raise
	except Exception:
		modPath = None
	modValues, unused = _readIniValues(
		modPath,
		_SAS_MOD_INI_DIRECT_KEYS,
		_SAS_MOD_INI_PRIVATE_KEYS,
		(),
		bPrivateValuesEnabled,
		False,
		True)
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
