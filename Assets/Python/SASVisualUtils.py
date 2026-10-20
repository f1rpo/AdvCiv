# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md).
#
# <!-- custom: Canonical AdvCiv-SAS visual-resource boundary. Import this module as `SASVisuals` rather than importing its generic collection names into caller globals.
# Typical use: SASVisuals.Font.HAPPY_CHAR; SASVisuals.Yield.FOOD_CHAR; SASVisuals.Commerce.chars(); SASVisuals.Religion.holyCityChar(iReligion); SASVisuals.Color.RED;
# SASVisuals.DynamicFont.WAR_CHAR; SASVisuals.InterfaceArt.SAS_EMOJI_TROPHY; SASVisuals.MovieArt.getPath(szArtDef); SASVisuals.CivilizationArt.button(iCivilization);
# SASVisuals.StaticArt.FOUND_CITY_BUTTON; SASVisuals.Trait.getIcon(iTrait); SASVisuals.GreatPeople.getUnitIcon(iUnit); SASVisuals.Specialist.icons(); SASVisuals.CityEmphasis.icons().
# `init()` eagerly resolves the deterministic early GameFont/yield/commerce families once; `initGameResources()` resolves later XML-backed collections. Lazy accessors remain safe fallbacks.
# Resource lookup/caching belongs here; presentation derived from those resources (for example a screen-size-specific <img> tag or translated label) stays with its UI owner. (ChatGPT-5.6-Sol) -->

# <!-- custom: CvPythonExtensions supplies resource enums and engine access; BugConfig supplies symbol handlers, BugDll checks their DLL requirements, BugUtil supplies diagnostics/errors, and re parses icon markup. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
from CvPythonExtensions import *
# <!-- custom: Loading SASVisualUtils from BugEventManager reached BugConfig before BugCore; BugConfig then imported BugCore/BugOptions, whose OptionsHandler inherited from the still-incomplete BugConfig.Handler and startup failed with AttributeError.
# Import BugCore first so BugOptions drives the inherited configuration initialization order before this module defines SymbolHandler. (GPT-6.1-Sol) -->
import BugCore
import BugConfig
import BugDll
import BugUtil
import re
# <!-- custom: Reuse SASUtils.getInfoTypeOrFail for cached color/unit/specialist XML identities so centralizing resources preserves the existing fail-loud contract and raw-lookup CI guard. (GPT-6.1-Sol) -->
from SASUtils import getInfoTypeOrFail

gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()


def _char(iOrdinal):
	return u"%c" % iOrdinal


def _toLegacyKey(szText):
	# <!-- custom: Match CvUtil.convertToStr's inherited latin-1 behavior without importing CvUtil and creating an early-startup dependency cycle. (ChatGPT-5.6-Sol) -->
	if isinstance(szText, unicode):
		return szText.encode("latin_1")
	return szText


# <!-- custom: Built-in GameFont symbols. (ChatGPT-5.6-Sol) -->

# <!-- custom: Keep the complete DLL FontSymbols inventory explicit and code-native.
# CI compares this tuple with CvEnums.h so a future DLL symbol addition cannot silently miss the shared collection. (ChatGPT-5.6-Sol) -->
BUILTIN_FONT_SYMBOL_NAMES = (
	"HAPPY_CHAR",
	"UNHAPPY_CHAR",
	"HEALTHY_CHAR",
	"UNHEALTHY_CHAR",
	"BULLET_CHAR",
	"STRENGTH_CHAR",
	"MOVES_CHAR",
	"RELIGION_CHAR",
	"STAR_CHAR",
	"SILVER_STAR_CHAR",
	"TRADE_CHAR",
	"DEFENSE_CHAR",
	"GREAT_PEOPLE_CHAR",
	"BAD_GOLD_CHAR",
	"BAD_FOOD_CHAR",
	"EATEN_FOOD_CHAR",
	"GOLDEN_AGE_CHAR",
	"ANGRY_POP_CHAR",
	"OPEN_BORDERS_CHAR",
	"DEFENSIVE_PACT_CHAR",
	"MAP_CHAR",
	"OCCUPATION_CHAR",
	"POWER_CHAR",
	"CITIZEN_CHAR",
	"GREAT_GENERAL_CHAR",
	"AIRPORT_CHAR",
	"WORST_ATTITUDE_CHAR",
)


class _FontSymbolResources:
	def __init__(self):
		self._ordinalCache = {}
		self._charCache = {}
		self._chars = None

	def init(self):
		if self._chars is not None:
			return
		chars = []
		for szName in BUILTIN_FONT_SYMBOL_NAMES:
			eSymbol = getattr(FontSymbols, szName)
			szChar = self.char(eSymbol)
			setattr(self, szName, szChar)
			chars.append(szChar)
		self._chars = tuple(chars)

	def ordinal(self, eSymbol):
		if eSymbol not in self._ordinalCache:
			self._ordinalCache[eSymbol] = gc.getGame().getSymbolID(eSymbol)
		return self._ordinalCache[eSymbol]

	def char(self, eSymbol):
		if eSymbol not in self._charCache:
			self._charCache[eSymbol] = _char(self.ordinal(eSymbol))
		return self._charCache[eSymbol]

	def chars(self):
		if self._chars is None:
			self.init()
		return self._chars

	def __getattr__(self, szName):
		if szName.startswith("_") or szName not in BUILTIN_FONT_SYMBOL_NAMES:
			raise AttributeError(szName)
		szChar = self.char(getattr(FontSymbols, szName))
		setattr(self, szName, szChar)
		return szChar


Font = _FontSymbolResources()


# <!-- custom: XML-info-backed font glyph collections. (ChatGPT-5.6-Sol) -->

class _IndexedInfoChars:
	def __init__(self, getInfo, getCount):
		self._getInfo = getInfo
		self._getCount = getCount
		self._ordinalCache = {}
		self._charCache = {}
		self._ordinals = None
		self._chars = None

	def init(self):
		if self._chars is not None:
			return
		ordinals = []
		chars = []
		for iInfo in range(self._getCount()):
			iOrdinal = self.ordinal(iInfo)
			ordinals.append(iOrdinal)
			chars.append(self.char(iInfo))
		self._ordinals = tuple(ordinals)
		self._chars = tuple(chars)

	def ordinal(self, iInfo):
		if iInfo not in self._ordinalCache:
			self._ordinalCache[iInfo] = self._getInfo(iInfo).getChar()
		return self._ordinalCache[iInfo]

	def char(self, iInfo):
		if iInfo not in self._charCache:
			self._charCache[iInfo] = _char(self.ordinal(iInfo))
		return self._charCache[iInfo]

	def ordinals(self):
		if self._ordinals is None:
			self.init()
		return self._ordinals

	def chars(self):
		if self._chars is None:
			self.init()
		return self._chars


class _YieldResources(_IndexedInfoChars):
	_NAMED = {
		"FOOD_CHAR": YieldTypes.YIELD_FOOD,
		"PRODUCTION_CHAR": YieldTypes.YIELD_PRODUCTION,
		"COMMERCE_CHAR": YieldTypes.YIELD_COMMERCE,
	}

	def __init__(self):
		_IndexedInfoChars.__init__(self, gc.getYieldInfo, lambda: YieldTypes.NUM_YIELD_TYPES)
		self._colorCache = {}
		self._colors = None

	def init(self):
		if self._colors is not None:
			return
		_IndexedInfoChars.init(self)
		colors = []
		for iYield in range(YieldTypes.NUM_YIELD_TYPES):
			colors.append(self.color(iYield))
		self._colors = tuple(colors)
		for szName, iYield in self._NAMED.iteritems():
			setattr(self, szName, self.char(iYield))

	def color(self, iYield):
		if iYield not in self._colorCache:
			self._colorCache[iYield] = gc.getYieldInfo(iYield).getColorType()
		return self._colorCache[iYield]

	def colors(self):
		if self._colors is None:
			self.init()
		return self._colors

	def __getattr__(self, szName):
		try:
			iYield = self._NAMED[szName]
		except KeyError:
			raise AttributeError(szName)
		szChar = self.char(iYield)
		setattr(self, szName, szChar)
		return szChar


class _CommerceResources(_IndexedInfoChars):
	_NAMED = {
		"GOLD_CHAR": CommerceTypes.COMMERCE_GOLD,
		"RESEARCH_CHAR": CommerceTypes.COMMERCE_RESEARCH,
		"CULTURE_CHAR": CommerceTypes.COMMERCE_CULTURE,
		"ESPIONAGE_CHAR": CommerceTypes.COMMERCE_ESPIONAGE,
	}

	def __init__(self):
		_IndexedInfoChars.__init__(self, gc.getCommerceInfo, lambda: CommerceTypes.NUM_COMMERCE_TYPES)

	def init(self):
		if self._chars is not None:
			return
		_IndexedInfoChars.init(self)
		for szName, iCommerce in self._NAMED.iteritems():
			setattr(self, szName, self.char(iCommerce))

	def __getattr__(self, szName):
		try:
			iCommerce = self._NAMED[szName]
		except KeyError:
			raise AttributeError(szName)
		szChar = self.char(iCommerce)
		setattr(self, szName, szChar)
		return szChar


class _ReligionResources(_IndexedInfoChars):
	def __init__(self):
		_IndexedInfoChars.__init__(self, gc.getReligionInfo, gc.getNumReligionInfos)
		self._holyOrdinalCache = {}
		self._holyCharCache = {}
		self._holyOrdinals = None
		self._holyChars = None

	def init(self):
		if self._holyChars is not None:
			return
		_IndexedInfoChars.init(self)
		holyOrdinals = []
		holyChars = []
		for iReligion in range(gc.getNumReligionInfos()):
			holyOrdinals.append(self.holyCityOrdinal(iReligion))
			holyChars.append(self.holyCityChar(iReligion))
		self._holyOrdinals = tuple(holyOrdinals)
		self._holyChars = tuple(holyChars)

	def holyCityOrdinal(self, iReligion):
		if iReligion not in self._holyOrdinalCache:
			self._holyOrdinalCache[iReligion] = gc.getReligionInfo(iReligion).getHolyCityChar()
		return self._holyOrdinalCache[iReligion]

	def holyCityChar(self, iReligion):
		if iReligion not in self._holyCharCache:
			self._holyCharCache[iReligion] = _char(self.holyCityOrdinal(iReligion))
		return self._holyCharCache[iReligion]

	def holyCityOrdinals(self):
		if self._holyOrdinals is None:
			self.init()
		return self._holyOrdinals

	def holyCityChars(self):
		if self._holyChars is None:
			self.init()
		return self._holyChars


class _CorporationResources(_IndexedInfoChars):
	def __init__(self):
		_IndexedInfoChars.__init__(self, gc.getCorporationInfo, gc.getNumCorporationInfos)
		self._headquarterOrdinalCache = {}
		self._headquarterCharCache = {}
		self._headquarterOrdinals = None
		self._headquarterChars = None

	def init(self):
		if self._headquarterChars is not None:
			return
		_IndexedInfoChars.init(self)
		headquarterOrdinals = []
		headquarterChars = []
		for iCorporation in range(gc.getNumCorporationInfos()):
			headquarterOrdinals.append(self.headquarterOrdinal(iCorporation))
			headquarterChars.append(self.headquarterChar(iCorporation))
		self._headquarterOrdinals = tuple(headquarterOrdinals)
		self._headquarterChars = tuple(headquarterChars)

	def headquarterOrdinal(self, iCorporation):
		if iCorporation not in self._headquarterOrdinalCache:
			self._headquarterOrdinalCache[iCorporation] = gc.getCorporationInfo(iCorporation).getHeadquarterChar()
		return self._headquarterOrdinalCache[iCorporation]

	def headquarterChar(self, iCorporation):
		if iCorporation not in self._headquarterCharCache:
			self._headquarterCharCache[iCorporation] = _char(self.headquarterOrdinal(iCorporation))
		return self._headquarterCharCache[iCorporation]

	def headquarterOrdinals(self):
		if self._headquarterOrdinals is None:
			self.init()
		return self._headquarterOrdinals

	def headquarterChars(self):
		if self._headquarterChars is None:
			self.init()
		return self._headquarterChars


Yield = _YieldResources()
Commerce = _CommerceResources()
Religion = _ReligionResources()
Corporation = _CorporationResources()
Bonus = _IndexedInfoChars(gc.getBonusInfo, gc.getNumBonusInfos)


# <!-- custom: Interface art, literal art, and named colors. (ChatGPT-5.6-Sol) -->

class _ArtPathResources:
	def __init__(self, getArtInfo):
		self._getArtInfo = getArtInfo
		self._pathCache = {}

	def getPath(self, szArtDefine):
		if szArtDefine not in self._pathCache:
			self._pathCache[szArtDefine] = self._getArtInfo(szArtDefine).getPath()
		return self._pathCache[szArtDefine]

	def __getattr__(self, szArtDefine):
		if szArtDefine.startswith("_"):
			raise AttributeError(szArtDefine)
		szPath = self.getPath(szArtDefine)
		setattr(self, szArtDefine, szPath)
		return szPath


InterfaceArt = _ArtPathResources(ArtFileMgr.getInterfaceArtInfo)
MovieArt = _ArtPathResources(ArtFileMgr.getMovieArtInfo)


class _CivilizationArtResources:
	def __init__(self):
		self._buttonCache = {}

	def button(self, iCivilization):
		if iCivilization not in self._buttonCache:
			szArtDefine = gc.getCivilizationInfo(iCivilization).getArtDefineTag()
			self._buttonCache[iCivilization] = ArtFileMgr.getCivilizationArtInfo(szArtDefine).getButton()
		return self._buttonCache[iCivilization]


CivilizationArt = _CivilizationArtResources()


class _StaticArtResources:
	# <!-- custom: Literal paths live here only when they are reused, represent a compatibility sentinel, or were previously cached by a screen.
	# One-off art stays local. (ChatGPT-5.6-Sol) -->
	FOUND_CITY_BUTTON = "Art/Interface/Buttons/Actions/FoundCity.dds"
	CONVERT_RELIGION_BUTTON = "Art/Interface/Buttons/General/ConvertReligion.dds"
	GOLDEN_AGE_BUTTON = "Art/Interface/Buttons/Actions/GoldenAge.dds"
	WARNING_POPUP_BUTTON = "Art/Interface/Buttons/General/Warning_popup.dds"
	BOMB_SHELTERS_BUTTON = "Art/Interface/Buttons/Buildings/BombShelters.dds"
	FOOD_SYMBOL = "Art/Interface/Symbols/Food/food05.dds"
	HEALTHY_PERSON = "Art/Interface/Buttons/General/unhealthy_person.dds"
	UNHEALTHY_PERSON = "Art/Interface/Buttons/General/unhealthy_person.dds"
	HAPPY_PERSON = "Art/Interface/Buttons/General/happy_person.dds"
	UNHAPPY_PERSON = "Art/Interface/mainscreen/cityscreen/angry_citizen.dds"
	TECH_TREE_BUTTONS = "Art/Interface/Buttons/TechTree/"


StaticArt = _StaticArtResources()


class _ColorResources:
	def __init__(self):
		self._cache = {}

	def get(self, szColorType):
		if not szColorType.startswith("COLOR_"):
			szColorType = "COLOR_" + szColorType
		if szColorType not in self._cache:
			iColor = getInfoTypeOrFail(szColorType)
			self._cache[szColorType] = iColor
		return self._cache[szColorType]

	def __getattr__(self, szName):
		if szName.startswith("_"):
			raise AttributeError(szName)
		iColor = self.get(szName)
		setattr(self, szName, iColor)
		return iColor


Color = _ColorResources()


# <!-- custom: BUG dynamic FontSymbols and [ICON_*] compatibility. (ChatGPT-5.6-Sol) -->

# Copyright (c) 2008 The BUG Mod. Author: EmperorFool.
# Inherited FontUtil compatibility API, moved here with its behavior rather than discarded:
# - getSymbol/getOrdinal/getChar accept a FontSymbols value or registered key.
# - replaceSymbols replaces [ICON_XXX] tags in translated/XML text.
# - <symbol> entries in BUG config may register built-in, offset, or DLL-gated symbols through SymbolHandler.
# - BUG initializes this externally through Assets/Config/init.xml; callers should use the code-native SASVisuals collections when a static symbol is known.
# <!-- custom: This section absorbs BUG FontUtil.py instead of retaining a facade module whose only state already lived here.
# Human keys such as "happy" and "war" are compatibility aliases only; new Python UI code should use code-native collections/attributes above whenever possible. (ChatGPT-5.6-Sol) -->

UNKNOWN_CHAR = "?"
SYMBOL_REGEXP = re.compile(r"\[ICON_([a-zA-Z0-9_]+)\]")
_nextSymbolID = int(FontSymbols.MAX_NUM_SYMBOLS)


class _DynamicFontResources:
	def __init__(self):
		self.keySymbols = {}
		self.symbolPrimaryKeys = {}
		self.symbolOrdinals = {}
		self.symbolChars = {}

	def register(self, szKey, eSymbol, iOrdinal):
		self.keySymbols[szKey] = eSymbol
		self.symbolPrimaryKeys[eSymbol] = szKey
		self.symbolOrdinals[eSymbol] = iOrdinal
		self.symbolChars[eSymbol] = _char(iOrdinal)

	def registerSynonym(self, szSynonym, eSymbol):
		self.keySymbols[szSynonym] = eSymbol

	def getSymbol(self, symbolOrKey):
		if isinstance(symbolOrKey, FontSymbols):
			return symbolOrKey
		try:
			return self.keySymbols[symbolOrKey]
		except KeyError:
			return self.keySymbols[symbolOrKey.upper() + "_CHAR"]

	def getOrdinal(self, symbolOrKey):
		return self.symbolOrdinals[self.getSymbol(symbolOrKey)]

	def getChar(self, symbolOrKey):
		return self.symbolChars[self.getSymbol(symbolOrKey)]

	def __getattr__(self, szName):
		if szName.startswith("_") or not szName.endswith("_CHAR"):
			raise AttributeError(szName)
		try:
			eSymbol = getattr(FontSymbols, szName)
		except AttributeError:
			raise AttributeError(szName)
		try:
			szChar = self.getChar(eSymbol)
		except KeyError:
			raise AttributeError(szName)
		setattr(self, szName, szChar)
		return szChar


DynamicFont = _DynamicFontResources()

# <!-- custom: These human-readable keys preserve CvUtil.getIcon and BUG [ICON_*] behavior only.
# This is intentionally not the canonical new Python API. (ChatGPT-5.6-Sol) -->
LEGACY_BUILTIN_FONT_SYMBOLS = (
	("happy", FontSymbols.HAPPY_CHAR),
	("unhappy", FontSymbols.UNHAPPY_CHAR),
	("healthy", FontSymbols.HEALTHY_CHAR),
	("unhealthy", FontSymbols.UNHEALTHY_CHAR),
	("bullet", FontSymbols.BULLET_CHAR),
	("strength", FontSymbols.STRENGTH_CHAR),
	("moves", FontSymbols.MOVES_CHAR),
	("religion", FontSymbols.RELIGION_CHAR),
	("star", FontSymbols.STAR_CHAR),
	("silver star", FontSymbols.SILVER_STAR_CHAR),
	("trade", FontSymbols.TRADE_CHAR),
	("defense", FontSymbols.DEFENSE_CHAR),
	("greatpeople", FontSymbols.GREAT_PEOPLE_CHAR),
	("badgold", FontSymbols.BAD_GOLD_CHAR),
	("badfood", FontSymbols.BAD_FOOD_CHAR),
	("eatenfood", FontSymbols.EATEN_FOOD_CHAR),
	("goldenage", FontSymbols.GOLDEN_AGE_CHAR),
	("angrypop", FontSymbols.ANGRY_POP_CHAR),
	("openBorders", FontSymbols.OPEN_BORDERS_CHAR),
	("defensivePact", FontSymbols.DEFENSIVE_PACT_CHAR),
	("map", FontSymbols.MAP_CHAR),
	("occupation", FontSymbols.OCCUPATION_CHAR),
	("power", FontSymbols.POWER_CHAR),
)


def _initDynamicSymbols():
	symbolNames = {}
	for szName, eSymbol in FontSymbols.__dict__.iteritems():
		if szName.endswith("_CHAR") and isinstance(eSymbol, FontSymbols):
			symbolNames[eSymbol] = szName
	for szKey, eSymbol in LEGACY_BUILTIN_FONT_SYMBOLS:
		addBuiltinSymbol(szKey, eSymbol)
		if eSymbol in symbolNames:
			szName = symbolNames[eSymbol]
			registerSymbolSynonym(szKey, eSymbol, szName[:-5])
			registerSymbolSynonym(szKey, eSymbol, szName)
	for iYield in range(YieldTypes.NUM_YIELD_TYPES):
		info = gc.getYieldInfo(iYield)
		addSymbol(info.getType().lower().replace("_", " "), Yield.ordinal(iYield), info.getType())
	for iCommerce in range(CommerceTypes.NUM_COMMERCE_TYPES):
		info = gc.getCommerceInfo(iCommerce)
		addSymbol(info.getType().lower().replace("_", " "), Commerce.ordinal(iCommerce), info.getType())


def addBuiltinSymbol(szKey, eSymbol):
	registerSymbol(szKey, eSymbol, Font.ordinal(eSymbol))


def addOffsetSymbol(szKey, symbolOrKey, iOffset, szName=None):
	return addSymbol(szKey, getOrdinal(symbolOrKey) + iOffset, szName)


def addSymbol(szKey, iOrdinal, szName=None):
	if not szName:
		szName = szKey.upper().replace(" ", "_")
	else:
		szName = szName.upper().replace(" ", "_")
	szSymbolName = szName + "_CHAR"
	eSymbol = findOrCreateSymbol(szSymbolName)
	registerSymbol(szKey, eSymbol, iOrdinal)
	registerSymbolSynonym(szKey, eSymbol, szName)
	registerSymbolSynonym(szKey, eSymbol, szSymbolName)
	return eSymbol


def findOrCreateSymbol(szName):
	try:
		eSymbol = getattr(FontSymbols, szName)
		if isinstance(eSymbol, FontSymbols):
			BugUtil.debug("SASVisualUtils - found FontSymbols name %s", szName)
			return eSymbol
	except AttributeError:
		pass
	global _nextSymbolID
	eSymbol = FontSymbols(_nextSymbolID)
	_nextSymbolID += 1
	BugUtil.debug("SASVisualUtils - created FontSymbols.%s", szName)
	setattr(FontSymbols, szName, eSymbol)
	return eSymbol


def registerSymbol(szKey, eSymbol, iOrdinal):
	BugUtil.info("SASVisualUtils - registering symbol '%s' for %d", szKey, iOrdinal)
	if szKey in DynamicFont.keySymbols:
		raise BugUtil.ConfigError("duplicate font symbol key '%s'" % szKey)
	if eSymbol in DynamicFont.symbolPrimaryKeys:
		raise BugUtil.ConfigError("duplicate font symbol for key '%s'" % szKey)
	DynamicFont.register(szKey, eSymbol, iOrdinal)


def registerSymbolSynonym(szKey, eSymbol, szSynonym):
	if szSynonym in DynamicFont.keySymbols:
		BugUtil.warn("SASVisualUtils - ignoring duplicate synonym '%s' for key '%s'", szSynonym, szKey)
	else:
		BugUtil.debug("SASVisualUtils - registering synonym '%s'", szSynonym)
		DynamicFont.registerSynonym(szSynonym, eSymbol)


def getSymbol(symbolOrKey):
	try:
		return DynamicFont.getSymbol(symbolOrKey)
	except (KeyError, AttributeError):
		raise BugUtil.ConfigError("unknown font symbol or key '%s'" % str(symbolOrKey))


def getOrdinal(symbolOrKey):
	try:
		return DynamicFont.getOrdinal(symbolOrKey)
	except (KeyError, AttributeError):
		raise BugUtil.ConfigError("unknown font symbol or key '%s'" % str(symbolOrKey))


def getChar(symbolOrKey):
	try:
		return DynamicFont.getChar(symbolOrKey)
	except (KeyError, AttributeError):
		raise BugUtil.ConfigError("unknown font symbol or key '%s'" % str(symbolOrKey))


def replaceSymbols(szText, szUnknownReplacement=""):
	def replace(match):
		try:
			return getChar(match.group(1))
		except BugUtil.ConfigError:
			return szUnknownReplacement
	return SYMBOL_REGEXP.sub(replace, szText)


class SymbolHandler(BugConfig.Handler):
	TAG = "symbol"

	def __init__(self):
		BugConfig.Handler.__init__(self, SymbolHandler.TAG, "id name from offset dll")
		self.addAttribute("id", True)
		self.addAttribute("name")
		self.addAttribute("from")
		self.addAttribute("offset")
		self.addAttribute("dll")
		self.lastSymbol = None

	def handle(self, element, id, name, fromKey, offset, dll):
		dll = BugDll.decode(dll)
		if self.isDllOkay(element, dll):
			if not fromKey:
				if not self.lastSymbol:
					raise BugUtil.ConfigError("<%s> %s requires an offset symbol" % (element.tag, id))
				fromKey = self.lastSymbol
			if offset is None:
				offset = 1
			else:
				offset = int(offset)
			self.lastSymbol = addOffsetSymbol(id, fromKey, offset, name)
		else:
			BugUtil.info("SASVisualUtils - ignoring <%s> %s, requires dll version %s", element.tag, id, self.resolveDll(element, dll))


# <!-- custom: Legacy CvUtil description-keyed icon map. (ChatGPT-5.6-Sol) -->

_legacyIconMap = {}


def initLegacyIconMap():
	# <!-- custom: Civ4's CvAppInterface.preGameStart still calls CvUtil.initDynamicFontIcons().
	# CvUtil now forwards that legacy entry point here so its mutable FontIconMap cache can live in the same shared resource boundary as the glyphs it contains. (ChatGPT-5.6-Sol) -->
	for iCommerce in range(CommerceTypes.NUM_COMMERCE_TYPES):
		info = gc.getCommerceInfo(iCommerce)
		_addLegacyIcon(Commerce.ordinal(iCommerce), info.getDescription().lower(), "commerce", info.getType())
	for iYield in range(YieldTypes.NUM_YIELD_TYPES):
		info = gc.getYieldInfo(iYield)
		_addLegacyIcon(Yield.ordinal(iYield), info.getDescription().lower(), "yield", info.getType())
	for iReligion in range(gc.getNumReligionInfos()):
		info = gc.getReligionInfo(iReligion)
		szDescription = info.getDescription().lower()
		_addLegacyIcon(Religion.ordinal(iReligion), szDescription, "religion", info.getType())
		_addLegacyIcon(Religion.holyCityOrdinal(iReligion), szDescription, "holy-city", info.getType())
	for szKey, eSymbol in LEGACY_BUILTIN_FONT_SYMBOLS:
		_legacyIconMap[szKey] = Font.char(eSymbol)


# <!-- custom: Retain the committed record=FONT_ICON label while moving CvUtil's icon-map writer here; resource consolidation must not revert searchable diagnostic fields. (GPT-6.1-Sol) -->
def _addLegacyIcon(iOrdinal, szDescription, szRole, szType):
	szDescription = _toLegacyKey(szDescription)
	print("record=FONT_ICON role=%s type=%s charCode=%d description=%s" % (szRole, szType, iOrdinal, szDescription))
	if iOrdinal >= 0:
		_legacyIconMap[szDescription] = _char(iOrdinal)


def getLegacyIcon(szKey):
	szKey = szKey.lower()
	if szKey in _legacyIconMap:
		return _legacyIconMap[szKey]
	return _char(191)


# <!-- custom: Attitude visual resources. (ChatGPT-5.6-Sol) -->

ATTITUDE_SYMBOL_NAMES = (
	"ATTITUDE_FURIOUS_CHAR",
	"ATTITUDE_ANNOYED_CHAR",
	"ATTITUDE_CAUTIOUS_CHAR",
	"ATTITUDE_PLEASED_CHAR",
	"ATTITUDE_FRIENDLY_CHAR",
)


class _AttitudeResources:
	def __init__(self):
		self._icons = None
		self._colors = None

	def configure(self, colorNames):
		if len(colorNames) != len(ATTITUDE_SYMBOL_NAMES):
			raise BugUtil.ConfigError("Expected %d attitude colors" % len(ATTITUDE_SYMBOL_NAMES))
		icons = []
		for szName in ATTITUDE_SYMBOL_NAMES:
			icons.append(getattr(DynamicFont, szName))
		colors = []
		invalid = []
		for szColor in colorNames:
			try:
				colors.append(Color.get(szColor))
			except ValueError:
				invalid.append(szColor)
		if invalid:
			raise BugUtil.ConfigError("Given %d invalid colors: %s" % (len(invalid), str(invalid)))
		self._icons = tuple(icons)
		self._colors = tuple(colors)

	def icons(self):
		if self._icons is None:
			raise RuntimeError("SASVisualUtils.Attitude not configured")
		return self._icons

	def colors(self):
		if self._colors is None:
			raise RuntimeError("SASVisualUtils.Attitude not configured")
		return self._colors

	def getIcon(self, iAttitude):
		return self.icons()[iAttitude]

	def getColor(self, iAttitude):
		return self.colors()[iAttitude]


Attitude = _AttitudeResources()


# <!-- custom: Trait visuals. (ChatGPT-5.6-Sol) -->

# Copyright (c) 2008 The BUG Mod. Author: EmperorFool.
# <!-- custom: TraitUtil.py has been absorbed here because its remaining job was entirely visual-resource mapping/formatting.
# Keep historical disabled/changed mappings beside the live exhaustive map so their provenance is not lost. (ChatGPT-5.6-Sol) -->
_TRAIT_RESOURCE_SPECS = {
	"TRAIT_AGGRESSIVE": (Font, FontSymbols.STRENGTH_CHAR, "Art/Interface/Buttons/Promotions/Combat1.dds"),
	"TRAIT_CHARISMATIC": (Font, FontSymbols.HAPPY_CHAR, "Art/Interface/Buttons/TechTree/MassMedia.dds"),
	"TRAIT_CREATIVE": (Commerce, CommerceTypes.COMMERCE_CULTURE, "Art/Interface/Buttons/TechTree/Music.dds"),
	"TRAIT_FINANCIAL": (Commerce, CommerceTypes.COMMERCE_GOLD, "Art/Interface/Buttons/TechTree/Banking.dds"),
	"TRAIT_IMPERIALIST": (Font, FontSymbols.TRADE_CHAR, StaticArt.FOUND_CITY_BUTTON),
	"TRAIT_INDUSTRIOUS": (Yield, YieldTypes.YIELD_PRODUCTION, "Art/Interface/Buttons/TechTree/Industrialism.dds"),
	"TRAIT_ORGANIZED": (Font, FontSymbols.OCCUPATION_CHAR, "Art/Interface/Buttons/Buildings/Courthouse.dds"),
	"TRAIT_PHILOSOPHICAL": (Font, FontSymbols.GREAT_PEOPLE_CHAR, "Art/Interface/Buttons/TechTree/Philosophy.dds"),
	"TRAIT_PROTECTIVE": (Font, FontSymbols.DEFENSE_CHAR, "Art/Interface/Buttons/Promotions/CityGarrison1.dds"),
	"TRAIT_SPIRITUAL": (Font, FontSymbols.RELIGION_CHAR, "Art/Interface/Buttons/TechTree/Meditation.dds"),
}
# <!-- custom: Preserve the historical disabled BUG/SAS mappings below without making them live; the XML trait no longer exists or the glyph was intentionally changed. (ChatGPT-5.6-Sol) -->
# <!-- custom: Historical disabled mapping example: TRAIT_EXPANSIVE used Font.HEALTHY_CHAR with the Heal button. (ChatGPT-5.6-Sol) -->
# <!-- custom: Historical disabled mapping: TRAIT_IMPERIALIST formerly used FontSymbols.OCCUPATION_CHAR. (ChatGPT-5.6-Sol) -->
# <!-- custom: Historical disabled mapping: TRAIT_ORGANIZED formerly used FontSymbols.TRADE_CHAR. (ChatGPT-5.6-Sol) -->


class _TraitResources:
	GENERIC_BUTTON = StaticArt.TECH_TREE_BUTTONS

	def __init__(self):
		self._iconCache = {}
		self._buttonCache = {}
		self._icons = None
		self._buttons = None

	def init(self):
		if self._icons is not None and self._buttons is not None:
			return
		icons = []
		buttons = []
		for eTrait in range(gc.getNumTraitInfos()):
			szType = gc.getTraitInfo(eTrait).getType()
			if szType not in _TRAIT_RESOURCE_SPECS:
				raise ValueError("Missing SAS trait visual mapping: '%s'" % szType)
			icons.append(self.getIcon(eTrait))
			buttons.append(self.getButton(eTrait))
		self._icons = tuple(icons)
		self._buttons = tuple(buttons)

	def _resolve(self, eTrait):
		szType = gc.getTraitInfo(eTrait).getType()
		resource, eValue, szButton = _TRAIT_RESOURCE_SPECS[szType]
		return (resource.char(eValue), szButton)

	def getIcon(self, eTrait):
		if eTrait not in self._iconCache:
			self._iconCache[eTrait], self._buttonCache[eTrait] = self._resolve(eTrait)
		return self._iconCache[eTrait]

	def getButton(self, eTrait):
		if eTrait not in self._buttonCache:
			self._iconCache[eTrait], self._buttonCache[eTrait] = self._resolve(eTrait)
		return self._buttonCache[eTrait]

	def icons(self):
		if self._icons is None:
			self.init()
		return self._icons

	def buttons(self):
		if self._buttons is None:
			self.init()
		return self._buttons

	def getLeaderTraitsWithIcons(self, eLeader, eCiv):
		szTraits = CyGameTextMgr().parseLeaderTraits(eLeader, eCiv, True, False)
		leaderInfo = gc.getLeaderHeadInfo(eLeader)
		for eTrait in range(gc.getNumTraitInfos()):
			if leaderInfo.hasTrait(eTrait):
				szDescription = gc.getTraitInfo(eTrait).getDescription()
				szTraits = szTraits.replace(szDescription, self.getIcon(eTrait) + u" " + szDescription, 1)
		return szTraits


Trait = _TraitResources()


# <!-- custom: Great Person / Great General visuals. (ChatGPT-5.6-Sol) -->

# Copyright (c) 2007-2009 The BUG Mod. Author: EmperorFool.
# <!-- custom: GPUtil keeps gameplay/progress behavior; the complete live GP unit/icon/color inventory is centralized here.
# GGUtil's only live Great-General text helper is absorbed here too; its unused promotion-button cache/getters are intentionally retired. (ChatGPT-5.6-Sol) -->
GREAT_PERSON_UNIT_TYPES = (
	"UNIT_GREAT_SPY",
	"UNIT_GREAT_ENGINEER",
	"UNIT_GREAT_MERCHANT",
	"UNIT_GREAT_SCIENTIST",
	"UNIT_GREAT_ARTIST",
	"UNIT_GREAT_PROPHET",
	"UNIT_GREAT_GENERAL",
)

_GP_UNIT_ICON_SPECS = {
	"UNIT_GREAT_SPY": (Commerce, CommerceTypes.COMMERCE_ESPIONAGE),
	"UNIT_GREAT_ENGINEER": (Yield, YieldTypes.YIELD_PRODUCTION),
	"UNIT_GREAT_MERCHANT": (Commerce, CommerceTypes.COMMERCE_GOLD),
	"UNIT_GREAT_SCIENTIST": (Commerce, CommerceTypes.COMMERCE_RESEARCH),
	"UNIT_GREAT_ARTIST": (Commerce, CommerceTypes.COMMERCE_CULTURE),
	"UNIT_GREAT_PROPHET": (Font, FontSymbols.RELIGION_CHAR),
	"UNIT_GREAT_GENERAL": (Font, FontSymbols.GREAT_GENERAL_CHAR),
}

_GP_COLOR_SPECS = {
	"UNIT_GREAT_SPY": (Color, "WHITE"),
	"UNIT_GREAT_ENGINEER": (Yield, YieldTypes.YIELD_PRODUCTION),
	"UNIT_GREAT_MERCHANT": (Color, "YELLOW"),
	"UNIT_GREAT_SCIENTIST": (Color, "RESEARCH_STORED"),
	"UNIT_GREAT_ARTIST": (Color, "CULTURE_STORED"),
	"UNIT_GREAT_PROPHET": (Color, "BLUE"),
	"UNIT_GREAT_GENERAL": (Color, "RED"),
}
# <!-- custom: Preserve the historical BUG extension example below as inactive documentation because AdvCiv-SAS has no UNIT_DOCTOR. (ChatGPT-5.6-Sol) -->
# <!-- custom: Historical BUG extension example: UNIT_DOCTOR used Font.HEALTHY_CHAR. (ChatGPT-5.6-Sol) -->


class _GreatPeopleResources:
	def __init__(self):
		self._unitTypeCache = {}
		self._unitIconCache = {}
		self._unitColorCache = {}
		self._unitTypes = None
		self._unitIcons = None
		self._unitColors = None

	def init(self):
		if self._unitTypes is not None:
			return
		unitTypes = []
		unitIcons = []
		unitColors = []
		for szType in GREAT_PERSON_UNIT_TYPES:
			iUnit = getInfoTypeOrFail(szType)
			self._unitTypeCache[szType] = iUnit
			unitTypes.append(iUnit)
			unitIcons.append(self.getUnitIcon(iUnit))
			unitColors.append(self.getUnitColor(iUnit))
		self._unitTypes = tuple(unitTypes)
		self._unitIcons = tuple(unitIcons)
		self._unitColors = tuple(unitColors)

	def _type(self, iUnit):
		return gc.getUnitInfo(iUnit).getType()

	def getDefaultIcon(self):
		return Font.GREAT_PEOPLE_CHAR

	def hasUnitIcon(self, iUnit):
		try:
			return self._type(iUnit) in _GP_UNIT_ICON_SPECS
		except AttributeError:
			return False

	def getUnitIcon(self, iUnit):
		if iUnit not in self._unitIconCache:
			try:
				resource, eValue = _GP_UNIT_ICON_SPECS[self._type(iUnit)]
				szIcon = resource.char(eValue)
			except (KeyError, AttributeError):
				szIcon = self.getDefaultIcon()
			self._unitIconCache[iUnit] = szIcon
		return self._unitIconCache[iUnit]

	def getUnitColor(self, iUnit):
		if iUnit not in self._unitColorCache:
			szType = self._type(iUnit)
			resource, value = _GP_COLOR_SPECS[szType]
			if resource is Color:
				iColor = Color.get(value)
			else:
				iColor = resource.color(value)
			self._unitColorCache[iUnit] = iColor
		return self._unitColorCache[iUnit]

	def unitTypes(self):
		if self._unitTypes is None:
			self.init()
		return self._unitTypes

	def unitIcons(self):
		if self._unitIcons is None:
			self.init()
		return self._unitIcons

	def unitColors(self):
		if self._unitColors is None:
			self.init()
		return self._unitColors

	def getGreatGeneralText(self, iNeededExp):
		return BugUtil.getText("INTERFACE_NEXT_GREAT_GENERAL_XP", (Font.GREAT_GENERAL_CHAR, iNeededExp))


GreatPeople = _GreatPeopleResources()


# <!-- custom: Shared advisor icon collections. (ChatGPT-5.6-Sol) -->

# <!-- custom: The Customizable Domestic Advisor previously rebuilt these invariant specialist/emphasis glyph dictionaries per screen.
# Keep the full current specialist XML inventory and the inherited six emphasize slots centrally instead. (ChatGPT-5.6-Sol) -->
_SPECIALIST_ICON_SPECS = {
	"SPECIALIST_CITIZEN": (Font, FontSymbols.BULLET_CHAR),
	"SPECIALIST_PRIEST": (Font, FontSymbols.HAPPY_CHAR),
	"SPECIALIST_ARTIST": (Commerce, CommerceTypes.COMMERCE_CULTURE),
	"SPECIALIST_SCIENTIST": (Commerce, CommerceTypes.COMMERCE_RESEARCH),
	"SPECIALIST_MERCHANT": (Commerce, CommerceTypes.COMMERCE_GOLD),
	"SPECIALIST_ENGINEER": (Yield, YieldTypes.YIELD_PRODUCTION),
	"SPECIALIST_SPY": (Commerce, CommerceTypes.COMMERCE_ESPIONAGE),
	"SPECIALIST_GREAT_PRIEST": (Font, FontSymbols.RELIGION_CHAR),
	"SPECIALIST_GREAT_ARTIST": (Commerce, CommerceTypes.COMMERCE_CULTURE),
	"SPECIALIST_GREAT_SCIENTIST": (Commerce, CommerceTypes.COMMERCE_RESEARCH),
	"SPECIALIST_GREAT_MERCHANT": (Commerce, CommerceTypes.COMMERCE_GOLD),
	"SPECIALIST_GREAT_ENGINEER": (Yield, YieldTypes.YIELD_PRODUCTION),
	"SPECIALIST_GREAT_GENERAL": (Font, FontSymbols.GREAT_GENERAL_CHAR),
	"SPECIALIST_GREAT_SPY": (Commerce, CommerceTypes.COMMERCE_ESPIONAGE),
}


class _SpecialistResources:
	CITY_SPECIALIST_TYPES = (
		"SPECIALIST_CITIZEN",
		"SPECIALIST_PRIEST",
		"SPECIALIST_ARTIST",
		"SPECIALIST_SCIENTIST",
		"SPECIALIST_MERCHANT",
		"SPECIALIST_ENGINEER",
		"SPECIALIST_SPY",
	)

	def __init__(self):
		self._iconCache = {}
		self._icons = None
		self._cityIcons = None

	def init(self):
		if self._icons is not None:
			return
		icons = []
		for iSpecialist in range(gc.getNumSpecialistInfos()):
			szType = gc.getSpecialistInfo(iSpecialist).getType()
			if szType not in _SPECIALIST_ICON_SPECS:
				raise ValueError("Missing SAS specialist visual mapping: '%s'" % szType)
			icons.append(self.getIcon(iSpecialist))
		self._icons = tuple(icons)

	def getIcon(self, iSpecialist):
		if iSpecialist not in self._iconCache:
			resource, eValue = _SPECIALIST_ICON_SPECS[gc.getSpecialistInfo(iSpecialist).getType()]
			self._iconCache[iSpecialist] = resource.char(eValue)
		return self._iconCache[iSpecialist]

	def icons(self):
		if self._icons is None:
			self.init()
		return self._icons

	def cityIcons(self):
		if self._cityIcons is None:
			icons = []
			for szType in self.CITY_SPECIALIST_TYPES:
				iSpecialist = getInfoTypeOrFail(szType)
				icons.append(self.getIcon(iSpecialist))
			self._cityIcons = tuple(icons)
		return self._cityIcons


Specialist = _SpecialistResources()


class _CityEmphasisResources:
	# <!-- custom: Preserve the inherited CvCustomizableDomesticAdvisor emphasis order; Civ4 exposes these slots by index rather than a useful Python enum/type list. (ChatGPT-5.6-Sol) -->
	FOOD = 0
	PRODUCTION = 1
	GOLD = 2
	RESEARCH = 3
	GREAT_PEOPLE = 4
	AVOID_GROWTH = 5

	def __init__(self):
		self._icons = None

	def icons(self):
		if self._icons is None:
			self._icons = (
				Yield.FOOD_CHAR,
				Yield.PRODUCTION_CHAR,
				Commerce.GOLD_CHAR,
				Commerce.RESEARCH_CHAR,
				Font.GREAT_PEOPLE_CHAR,
				Font.BAD_FOOD_CHAR,
			)
		return self._icons

	def getIcon(self, iEmphasis):
		return self.icons()[iEmphasis]


CityEmphasis = _CityEmphasisResources()


# <!-- custom: Initialization stages. (ChatGPT-5.6-Sol) -->

def init():
	# <!-- custom: BUG loads this immediate stage where FontUtil.init used to run.
	# Preserve that timing for built-in/yield/commerce glyphs and dynamic [ICON_*] registration. (ChatGPT-5.6-Sol) -->
	Font.init()
	Yield.init()
	Commerce.init()
	_initDynamicSymbols()


def initGameResources():
	# <!-- custom: Later BUG init stage, replacing separate TraitUtil/GGUtil visual caches.
	# These XML-backed collections are safe to exhaustively resolve once the game infos are fully available. (ChatGPT-5.6-Sol) -->
	Religion.init()
	Corporation.init()
	Bonus.init()
	Trait.init()
	GreatPeople.init()
	Specialist.init()
	CityEmphasis.icons()
