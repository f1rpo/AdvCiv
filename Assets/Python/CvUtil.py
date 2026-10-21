## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md).
#
# for error reporting
import traceback

# for file ops
import os
import sys

# For Civ game code access
from CvPythonExtensions import *
# <!-- custom: Install shared row timestamps before UI imports emit startup diagnostics; SASPythonLogging avoids importing CvUtil/BUG so this early hook has no dependency cycle. (GPT-6.1-Sol) -->
import SASPythonLogging
# <!-- custom: CvUtil keeps text scaling in SASTextScale while its former shared visual-cache dependency is retired; legacy getIcon/initDynamicFontIcons now forward lazily to SASVisualUtils. (ChatGPT-5.6-Sol) -->
import SASTextScale

# For exception handling
SHOWEXCEPTIONS = 1

# for C++ compatibility
false=False
true=True

# globals
gc = CyGlobalContext()
localText = CyTranslator()

#
# Popup context enums, values greater than 999 are reserved for events
#

# DEBUG TOOLS
PopupTypeEntityEventTest = 4
PopupTypeEffectViewer = 5

# HELP SCREENS
PopupTypeMilitaryAdvisor = 103
PopupTypePlayerSelect = 104

# WORLD BUILDER
PopupTypeWBContextStart = 200
PopupTypeWBEditCity = PopupTypeWBContextStart 
PopupTypeWBEditUnit = 201
PopupTypeWBContextEnd	= 299

# EVENT ID VALUES (also used in popup contexts)
EventGetEspionageTarget = 4999
EventEditCityName = 5000
EventEditCity = 5001
EventPlaceObject = 5002
EventAwardTechsAndGold = 5003
EventEditUnitName = 5006
EventCityWarning = 5007
EventWBAllPlotsPopup = 5008
EventWBLandmarkPopup = 5009
EventWBScriptPopup = 5010
EventWBStartYearPopup = 5011
EventShowWonder = 5012

EventLButtonDown=1
EventLcButtonDblClick=2
EventRButtonDown=3
EventBack=4
EventForward=5
EventKeyDown=6
EventKeyUp=7

# List of unreported Events
SilentEvents = [EventEditCityName, EventEditUnitName]

# BUG - Core - start

# Event IDs
BUG_FIRST_EVENT = 5050
g_nextEventID = BUG_FIRST_EVENT
g_bugEvents = {}
def getNewEventID(name=None, silent=True):
	# Defines a new event and returns its unique ID to be passed to BugEventManager.beginEvent(id).
	# If name is given, it is stored in a map for lookup by ID later for debugging.

	global g_nextEventID
	id = g_nextEventID
	g_nextEventID += 1
	if name:
		g_bugEvents[id] = name
	if silent:
		addSilentEvent(id)
	return id

def getEventName(id):
	return g_bugEvents[id]

def addSilentEvent(id):
	if id not in SilentEvents:
		SilentEvents.append(id)

# Screen IDs
BUG_FIRST_SCREEN = 1000
g_nextScreenID = BUG_FIRST_SCREEN
def getNewScreenID():
	# Returns the next unique screen ID to be used with CyGInterfaceScreen.

	global g_nextScreenID
	id = g_nextScreenID
	g_nextScreenID += 1
	return id
# BUG - Core - end

# Popup defines (TODO: Expose these from C++)
FONT_CENTER_JUSTIFY=1<<2
FONT_RIGHT_JUSTIFY=1<<1
FONT_LEFT_JUSTIFY=1<<0

def convertToUnicode(s):
	# if the string is non unicode, convert it to unicode by decoding it using 8859-1, latin_1
	#
	if (isinstance(s, str)):
		return s.decode("latin_1")
	return s

def convertToStr(s):
	# if the string is unicode, convert it to str by encoding it using 8859-1, latin_1
	#
	if (isinstance(s, unicode)):
		return s.encode("latin_1")
	return s

# <!-- custom: Retain the engine's existing redirect entry points and byte/unicode sinks, but give both the shared row formatter too.
# The EXE can replace sys.stdout/stderr with these classes after importing CvUtil; inheriting the wrapper preserves coverage across that replacement. (GPT-6.1-Sol) -->
class RedirectDebug(SASPythonLogging.TimestampedStream):
	def __init__(self):
		self.m_PythonMgr = CyPythonMgr()
		SASPythonLogging.TimestampedStream.__init__(self, SASPythonLogging.NativeSink(self.m_PythonMgr.debugMsg, self.m_PythonMgr.debugMsgWide))

class RedirectError(SASPythonLogging.TimestampedStream):
	def __init__(self):
		self.m_PythonMgr = CyPythonMgr()
		SASPythonLogging.TimestampedStream.__init__(self, SASPythonLogging.NativeSink(self.m_PythonMgr.errorMsg, self.m_PythonMgr.errorMsgWide))

def myExceptHook(type, value, tb):
	lines=traceback.format_exception(type, value, tb)
	#pre= "---------------------Traceback lines-----------------------\n"
	mid="\n".join(lines)
	#post="-----------------------------------------------------------"
	#total = pre+mid+post
	total=mid
	if SHOWEXCEPTIONS:
		sys.stderr.write(total)
	else:
		sys.stdout.write(total)

def pyPrint(stuff):
	stuff = 'PY:' + stuff + "\n"
	# advc.001: Added encoding; used to throw an exception when printing
	# city names with non-ASCII characters
	stuff = stuff.encode("UTF-8")
	sys.stdout.write(stuff)

# <!-- custom: Keep inherited pyPrint prose semantics intact while giving existing CvUtil callers a concise path to the shared structured diagnostic writer.
# szRecord and optional szFields pass through unchanged; SASPythonLogging owns Unicode handling, the row newline and UTC prefix. (ChatGPT-5.6-Sol) -->
def pyPrintRecord(szRecord, szFields=""):
	SASPythonLogging.writeRecord(szRecord, szFields)

def pyAssert(cond, msg):
	if not cond:
		sys.stderr.write(msg)
	assert cond, msg # advc.001 (from MNAI)

def getScoreComponent(iRawScore, iInitial, iMax, iFactor, bExponential, bFinal, bVictory):

	if gc.getGame().getEstimateEndTurn() == 0:
		return 0

	if bFinal and bVictory:
		fTurnRatio = float(gc.getGame().getGameTurn()) / float(gc.getGame().getEstimateEndTurn())
		if bExponential and (iInitial != 0):
			fRatio = iMax / iInitial
			iMax = iInitial * pow(fRatio, fTurnRatio)
		else:
			iMax = iInitial + fTurnRatio * (iMax - iInitial)

	iFree = (gc.getDefineINT("SCORE_FREE_PERCENT") * iMax) / 100
	if (iFree + iMax) != 0:
		iScore = (iFactor * (iRawScore + iFree)) / (iFree + iMax)
	else:
		iScore = iFactor

	if bVictory:
		iScore = ((100 + gc.getDefineINT("SCORE_VICTORY_PERCENT")) * iScore) / 100

	if bFinal:
		# <advc.250a> Let C++ handle difficulty. Raise the fraction to per-mill.
		diffic = gc.getGame().getDifficultyForEndScore()
		iScore = ((1000 + 10 * gc.getDefineINT("SCORE_HANDICAP_PERCENT_OFFSET") + diffic * gc.getDefineINT("SCORE_HANDICAP_PERCENT_PER")) * iScore) / 1000
		# </advc.250a>

	return int(iScore)

def getOppositeCardinalDirection(dir):
	return (dir + 2) % CardinalDirectionTypes.NUM_CARDINALDIRECTION_TYPES

def shuffle(num, rand):
	"returns a tuple of size num of shuffled numbers"	
	piShuffle = [0]*num
	shuffleList(num, rand, piShuffle)	# implemented in C for speed
	return piShuffle

def spawnUnit(iUnit, pPlot, pPlayer):
	pPlayer.initUnit(iUnit, pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.NO_DIRECTION)
	return 1

def findInfoTypeNum(infoGetter, numInfos, typeStr):
	# advc.001: Also tolerate empty string - now that the bug in pyAssert has been fixed. Raising an error (that no one catches) would render WB saves unplayable that load w/o a hitch in BtS.
	if (not typeStr or typeStr == 'NONE'):
		return -1
	idx = gc.getInfoTypeForString(typeStr)
	pyAssert(idx != -1, "Can't find type enum for type tag %s" %(typeStr,))
	return idx

def getInfo(strInfoType, strInfoName):	# returns info for InfoType
	#set Type to lowercase
	strInfoType = strInfoType.lower()
	strInfoName = strInfoName.capitalize()

	#get the appropriate dictionary item
	infoDict = GlobalInfosMap.get(strInfoType)
	#get the number of infos
	numInfos = infoDict['NUM']()
	#loop through each info
	for i in range(numInfos):
		loopInfo = infoDict['GET'](i)

		if loopInfo.getDescription() == strInfoName:
			#and return the one requested
			return loopInfo

def AdjustBuilding(add, all, BuildingIdx, pCity): # adds/removes buildings from a city
	"Function for toggling buildings in cities"
	if (BuildingIdx!= -1):  
		if (all):                #Add/Remove ALL
			for i in range(BuildingIdx):
				pCity.setNumRealBuildingIdx(i,add)
		else:
			pCity.setNumRealBuildingIdx(BuildingIdx,add)
	return 0

def getIcon(iconEntry):
	# <!-- custom: Preserve CvUtil's inherited public API while keeping the actual description-keyed icon cache in the single SAS visual-resource boundary.
	# Import lazily because CvUtil is loaded before BUG configuration. (ChatGPT-5.6-Sol) -->
	# <!-- custom: Consolidate this dependency at the shared SASVisualUtils/SASTextScale boundary instead of retaining the retired overlapping visual-resource helper imports. (ChatGPT-5.6-Sol) -->
	import SASVisualUtils as SASVisuals
	return SASVisuals.getLegacyIcon(iconEntry)

# advc: Helper for combatDetailMessageBuilder
def addCombatMessageScaled(ePlayer, szText):
	# <!-- custom: upscale Combat Log entries via SAS label font; combat lines are emitted through CyInterface().addCombatMessage from CvUtil/CvEventManager paths. (GPT-5.3-Codex) -->
	CyInterface().addCombatMessage(ePlayer, SASTextScale.labelText(szText))

def _addCombatMsg(ePlayer, iModifier, szTextKey):
	if iModifier != 0:
		addCombatMessageScaled(ePlayer, localText.getText(szTextKey, (iModifier,)))

def combatDetailMessageBuilder(cdUnit, ePlayer, iChange):
	_addCombatMsg(ePlayer, cdUnit.iExtraCombatPercent * iChange, "TXT_KEY_COMBAT_MESSAGE_EXTRA_COMBAT_PERCENT")
	_addCombatMsg(ePlayer, cdUnit.iAnimalCombatModifierTA * iChange, "TXT_KEY_COMBAT_MESSAGE_ANIMAL_COMBAT")
	_addCombatMsg(ePlayer, cdUnit.iAIAnimalCombatModifierTA * iChange, "TXT_KEY_COMBAT_MESSAGE_AI_ANIMAL_COMBAT")
	_addCombatMsg(ePlayer, cdUnit.iAnimalCombatModifierAA * iChange, "TXT_KEY_COMBAT_MESSAGE_ANIMAL_COMBAT")
	_addCombatMsg(ePlayer, cdUnit.iAIAnimalCombatModifierAA * iChange, "TXT_KEY_COMBAT_MESSAGE_AI_ANIMAL_COMBAT")
	_addCombatMsg(ePlayer, cdUnit.iBarbarianCombatModifierTB * iChange, "TXT_KEY_COMBAT_MESSAGE_BARBARIAN_COMBAT")
	_addCombatMsg(ePlayer, cdUnit.iAIBarbarianCombatModifierTB * iChange, "TXT_KEY_COMBAT_MESSAGE_BARBARIAN_AI_COMBAT")
	_addCombatMsg(ePlayer, cdUnit.iBarbarianCombatModifierAB * iChange, "TXT_KEY_COMBAT_MESSAGE_BARBARIAN_COMBAT")
	_addCombatMsg(ePlayer, cdUnit.iAIBarbarianCombatModifierAB * iChange, "TXT_KEY_COMBAT_MESSAGE_BARBARIAN_AI_COMBAT")
	# <advc.313>
	_addCombatMsg(ePlayer, cdUnit.iSeaBarbarianModifierTB * iChange, "TXT_KEY_COMBAT_MESSAGE_BARBARIAN_SEA")
	_addCombatMsg(ePlayer, cdUnit.iSeaBarbarianModifierAB * iChange, "TXT_KEY_COMBAT_MESSAGE_BARBARIAN_SEA")
	# </advc.313>
	_addCombatMsg(ePlayer, cdUnit.iPlotDefenseModifier * iChange, "TXT_KEY_COMBAT_MESSAGE_PLOT_DEFENSE")
	_addCombatMsg(ePlayer, cdUnit.iFortifyModifier * iChange, "TXT_KEY_COMBAT_MESSAGE_FORTIFY")
	_addCombatMsg(ePlayer, cdUnit.iCityDefenseModifier * iChange, "TXT_KEY_COMBAT_MESSAGE_CITY_DEFENSE")
	_addCombatMsg(ePlayer, cdUnit.iHillsAttackModifier * iChange, "TXT_KEY_COMBAT_MESSAGE_HILLS_ATTACK")
	_addCombatMsg(ePlayer, cdUnit.iHillsDefenseModifier * iChange, "TXT_KEY_COMBAT_MESSAGE_HILLS")
	_addCombatMsg(ePlayer, cdUnit.iFeatureAttackModifier * iChange, "TXT_KEY_COMBAT_MESSAGE_FEATURE_ATTACK")
	_addCombatMsg(ePlayer, cdUnit.iFeatureDefenseModifier * iChange, "TXT_KEY_COMBAT_MESSAGE_FEATURE")
	_addCombatMsg(ePlayer, cdUnit.iTerrainAttackModifier * iChange, "TXT_KEY_COMBAT_MESSAGE_TERRAIN_ATTACK")
	_addCombatMsg(ePlayer, cdUnit.iTerrainDefenseModifier * iChange, "TXT_KEY_COMBAT_MESSAGE_TERRAIN")
	_addCombatMsg(ePlayer, cdUnit.iCityAttackModifier * iChange, "TXT_KEY_COMBAT_MESSAGE_CITY_ATTACK")
	_addCombatMsg(ePlayer, cdUnit.iDomainDefenseModifier * iChange, "TXT_KEY_COMBAT_MESSAGE_CITY_DOMAIN_DEFENSE")
	#_addCombatMsg(ePlayer, cdUnit.iCityBarbarianDefenseModifier * iChange, "TXT_KEY_COMBAT_MESSAGE_CITY_BARBARIAN_DEFENSE")
	# <advc.313>
	_addCombatMsg(ePlayer, cdUnit.iBarbarianCityAttackModifier * iChange, "TXT_KEY_COMBAT_MESSAGE_BARBARIAN_CITY") # </advc.313>
	_addCombatMsg(ePlayer, cdUnit.iClassDefenseModifier * iChange, "TXT_KEY_COMBAT_MESSAGE_CLASS_DEFENSE")
	_addCombatMsg(ePlayer, cdUnit.iClassAttackModifier * iChange, "TXT_KEY_COMBAT_MESSAGE_CLASS_ATTACK")
	_addCombatMsg(ePlayer, cdUnit.iCombatModifierT * iChange, "TXT_KEY_COMBAT_MESSAGE_CLASS_COMBAT")
	_addCombatMsg(ePlayer, cdUnit.iCombatModifierA * iChange, "TXT_KEY_COMBAT_MESSAGE_CLASS_COMBAT")
	_addCombatMsg(ePlayer, cdUnit.iDomainModifierA * iChange, "TXT_KEY_COMBAT_MESSAGE_CLASS_DOMAIN")
	_addCombatMsg(ePlayer, cdUnit.iDomainModifierT * iChange, "TXT_KEY_COMBAT_MESSAGE_CLASS_DOMAIN")
	_addCombatMsg(ePlayer, cdUnit.iAnimalCombatModifierA * iChange, "TXT_KEY_COMBAT_MESSAGE_CLASS_ANIMAL_COMBAT")
	_addCombatMsg(ePlayer, cdUnit.iAnimalCombatModifierT * iChange, "TXT_KEY_COMBAT_MESSAGE_CLASS_ANIMAL_COMBAT")
	_addCombatMsg(ePlayer, cdUnit.iRiverAttackModifier * iChange, "TXT_KEY_COMBAT_MESSAGE_CLASS_RIVER_ATTACK")
	_addCombatMsg(ePlayer, cdUnit.iAmphibAttackModifier * iChange, "TXT_KEY_COMBAT_MESSAGE_CLASS_AMPHIB_ATTACK")

def combatMessageBuilder(cdAttacker, cdDefender, iCombatOdds):
	combatMessage = ""
	if (cdAttacker.eOwner == cdAttacker.eVisualOwner):
		combatMessage += "%s's " %(gc.getPlayer(cdAttacker.eOwner).getName(),)
	combatMessage += "%s (%.2f)" %(cdAttacker.sUnitName,cdAttacker.iCurrCombatStr/100.0,)
	combatMessage += " " + localText.getText("TXT_KEY_COMBAT_MESSAGE_VS", ()) + " "
	if (cdDefender.eOwner == cdDefender.eVisualOwner):
		combatMessage += "%s's " %(gc.getPlayer(cdDefender.eOwner).getName(),)
	combatMessage += "%s (%.2f)" %(cdDefender.sUnitName,cdDefender.iCurrCombatStr/100.0,)
	addCombatMessageScaled(cdAttacker.eOwner, combatMessage)
	addCombatMessageScaled(cdDefender.eOwner, combatMessage)
	combatMessage = "%s %.1f%%" %(localText.getText("TXT_KEY_COMBAT_MESSAGE_ODDS", ()),iCombatOdds/10.0,)
	addCombatMessageScaled(cdAttacker.eOwner, combatMessage)
	addCombatMessageScaled(cdDefender.eOwner, combatMessage)
	combatDetailMessageBuilder(cdAttacker,cdAttacker.eOwner,-1)
	combatDetailMessageBuilder(cdDefender,cdAttacker.eOwner,1)
	combatDetailMessageBuilder(cdAttacker,cdDefender.eOwner,-1)
	combatDetailMessageBuilder(cdDefender,cdDefender.eOwner,1)

def initDynamicFontIcons():
	# <!-- custom: Civ4's CvAppInterface.preGameStart calls this exact CvUtil entry point.
	# Keep the API, but let SASVisualUtils own/populate the compatibility map and its cached glyphs. (ChatGPT-5.6-Sol) -->
	import SASVisualUtils as SASVisuals
	SASVisuals.initLegacyIconMap()

GlobalInfosMap = {	'bonus': {'NUM': gc.getNumBonusInfos, 'GET': gc.getBonusInfo},
					'improvement': {'NUM': gc.getNumImprovementInfos, 'GET': gc.getImprovementInfo},
					'yield': {'NUM': YieldTypes.NUM_YIELD_TYPES, 'GET': gc.getYieldInfo},
					'religion': {'NUM': gc.getNumReligionInfos, 'GET': gc.getReligionInfo},
					'tech': {'NUM': gc.getNumTechInfos, 'GET': gc.getTechInfo},
					'unit': {'NUM': gc.getNumUnitInfos, 'GET': gc.getUnitInfo},
					'civic': {'NUM': gc.getNumCivicInfos, 'GET': gc.getCivicInfo},
					'building': {'NUM': gc.getNumBuildingInfos, 'GET': gc.getBuildingInfo},
					'terrain': {'NUM': gc.getNumTerrainInfos, 'GET': gc.getTerrainInfo},
					'trait': {'NUM': gc.getNumTraitInfos, 'GET': gc.getTraitInfo},
					'feature' : {'NUM': gc.getNumFeatureInfos, 'GET': gc.getFeatureInfo},
					'route': {'NUM': gc.getNumRouteInfos, 'GET': gc.getRouteInfo},
					'promotion': {'NUM':gc.getNumPromotionInfos, 'GET': gc.getPromotionInfo},
				}
