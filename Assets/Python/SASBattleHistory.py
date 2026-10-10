# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from CvPythonExtensions import *
from SASUtils import getInfoTypeOrFail
import BugData

gc = CyGlobalContext()

_TABLE_ID = "AdvCivSASBattleHistory"
_ENTRIES_KEY = "entries"
_MAX_ENTRIES = None
_PENDING_COMBAT_ACTORS = None
_PENDING_COMBAT_UNIT_CONTEXT = None
_CAPTURE_DATA_START = 13
_END_STRENGTH_DATA_START = 16
_OUTCOME_DATA_START = 18
_OUTCOME_RETREAT = 1
_PLOT_CONTEXT_DATA_START = 19
_PLOT_CONTEXT_DATA_COUNT = 4
_UNIT_CONTEXT_DATA_START = _PLOT_CONTEXT_DATA_START + _PLOT_CONTEXT_DATA_COUNT
_PLOT_CONTEXT_HILL_PEAK_NONE = -1
_PLOT_CONTEXT_CITY_NONE = 0
_PLOT_CONTEXT_CITY_DEFENDED = 1
_PLOT_CONTEXT_CITY_CAPTURED = 2
_TERRAIN_PEAK = None
_TERRAIN_HILL = None

# <!-- custom: BugData persists this table in the save without DLL or save-format changes; old saves simply start with no rows and record battles from the first combat after loading.
# Store rows per player because advisor perspective can change. (GPT-5.5) -->

def _getMaxEntries():
	global _MAX_ENTRIES
	if _MAX_ENTRIES is None:
		_MAX_ENTRIES = gc.getDefineINT("SAS_CV_MILITARY_ADVISOR_BATTLE_HISTORY_MAX_ENTRIES")
	return _MAX_ENTRIES

def _getPeakTerrain():
	global _TERRAIN_PEAK
	if _TERRAIN_PEAK is None:
		_TERRAIN_PEAK = getInfoTypeOrFail("TERRAIN_PEAK")
	return _TERRAIN_PEAK

def _getHillTerrain():
	global _TERRAIN_HILL
	if _TERRAIN_HILL is None:
		_TERRAIN_HILL = getInfoTypeOrFail("TERRAIN_HILL")
	return _TERRAIN_HILL

def _getEntriesByPlayer():
	table = BugData.getTable(_TABLE_ID)
	if table.hasValue(_ENTRIES_KEY):
		entries = table.getValue(_ENTRIES_KEY)
		if isinstance(entries, dict):
			return _sanitizeEntriesByPlayer(entries)
	return {}

def _saveEntriesByPlayer(entriesByPlayer):
	BugData.getTable(_TABLE_ID).setValue(_ENTRIES_KEY, _sanitizeEntriesByPlayer(entriesByPlayer))

def _sanitizeEntry(entry):
	# <!-- custom: BugData pickles this table; Civ4 enum objects such as PlayerTypes are not pickle-safe, so force stored rows to plain Python ints. (GPT-5.5) -->
	#
	# Traceback (most recent call last):
	# File "CvEventInterface", line 35, in onEvent
	# File "BugEventManager", line 346, in handleEvent
	# File "BugEventManager", line 351, in _dispatchEvent
	# File "BugEventManager", line 390, in _handleOnPreSaveEvent
	# File "BugData", line 92, in save
	# File "BugData", line 116, in save
	# File "BugData", line 119, in save
	# File "BugData", line 261, in _save
	#
	aValues = []
	for value in entry:
		aValues.append(int(value))
	return tuple(aValues)

def _sanitizeEntriesByPlayer(entriesByPlayer):
	safeEntriesByPlayer = {}
	for szPlayer, entries in entriesByPlayer.items():
		if isinstance(entries, list):
			safeEntries = []
			for entry in entries:
				safeEntries.append(_sanitizeEntry(entry))
			safeEntriesByPlayer[str(szPlayer)] = safeEntries
	return safeEntriesByPlayer

def _appendEntry(entriesByPlayer, iPlayer, entry, iMaxEntries):
	szPlayer = str(iPlayer)
	entries = entriesByPlayer.get(szPlayer, [])
	entries.append(entry)
	if iMaxEntries > 0 and len(entries) > iMaxEntries:
		entries = entries[-iMaxEntries:]
	entriesByPlayer[szPlayer] = entries

def _getEntryWithPlotContext(entry, iX, iY):
	# <!-- custom: snapshot terrain/feature/peak-or-hill at battle time instead of looking up the plot while drawing the advisor; forests, fallout, or terrain can change later, but the Battles tab should show the historical combat context. (GPT-5.5) -->
	aEntry = list(entry[:_PLOT_CONTEXT_DATA_START])
	while len(aEntry) < _PLOT_CONTEXT_DATA_START:
		aEntry.append(0)
	aTail = []
	if len(entry) > _UNIT_CONTEXT_DATA_START:
		aTail = list(entry[_UNIT_CONTEXT_DATA_START:])
	pPlot = CyMap().plot(iX, iY)
	iHillPeakTerrain = _PLOT_CONTEXT_HILL_PEAK_NONE
	if pPlot.isPeak():
		iHillPeakTerrain = _getPeakTerrain()
	elif pPlot.isHills():
		iHillPeakTerrain = _getHillTerrain()
	iCityContext = _PLOT_CONTEXT_CITY_NONE
	if pPlot.isCity():
		iCityContext = _PLOT_CONTEXT_CITY_DEFENDED
	aEntry.extend([pPlot.getTerrainType(), pPlot.getFeatureType(), iHillPeakTerrain, iCityContext])
	aEntry.extend(aTail)
	return tuple(aEntry)

def _setEntryCityContext(entry, iX, iY, iCityContext):
	aEntry = list(_getEntryWithPlotContext(entry, iX, iY))
	aEntry[5] = int(iX)
	aEntry[6] = int(iY)
	aEntry[_PLOT_CONTEXT_DATA_START + 3] = int(iCityContext)
	return tuple(aEntry)

def _getEntryWithUnitContext(entry):
	aEntry = list(entry)
	while len(aEntry) < _UNIT_CONTEXT_DATA_START:
		aEntry.append(0)
	if _PENDING_COMBAT_UNIT_CONTEXT is not None:
		aEntry.extend(_PENDING_COMBAT_UNIT_CONTEXT)
	return tuple(aEntry)

def _getEntryWithCapture(entry, iCapturingPlayer, iCapturedUnitType):
	# <!-- custom: capture info is stored after the combat-log strength block; pad rare/no-strength rows so older tuple parsing keeps its fixed offsets. (GPT-5.5) -->
	aEntry = list(entry)
	while len(aEntry) < _CAPTURE_DATA_START:
		aEntry.append(0)
	if len(aEntry) >= _CAPTURE_DATA_START + 3:
		if aEntry[_CAPTURE_DATA_START] == iCapturingPlayer and aEntry[_CAPTURE_DATA_START + 1] == iCapturedUnitType:
			aEntry[_CAPTURE_DATA_START + 2] += 1
		else:
			aEntry[_CAPTURE_DATA_START] = iCapturingPlayer
			aEntry[_CAPTURE_DATA_START + 1] = iCapturedUnitType
			aEntry[_CAPTURE_DATA_START + 2] = 1
	else:
		aEntry.extend([iCapturingPlayer, iCapturedUnitType, 1])
	return tuple(aEntry)

def _getUnitEndCombatStr(pUnit, iMaxCombatStr):
	if iMaxCombatStr <= 0 or pUnit.maxHitPoints() <= 0:
		return 0
	return (iMaxCombatStr * pUnit.currHitPoints()) / pUnit.maxHitPoints()

def _matchesCapturedBattle(entry, iCapturingPlayer, iOldOwner, iX, iY):
	if len(entry) < 7:
		return False
	# <!-- custom: Capturable civilian units do not fight, so their old unit type cannot equal the defeated escort type in the preceding combat row.
	# Defended combat records the loser's plot; match the captured unit's plot plus player direction, and reject retreat rows so another same-turn battle cannot receive the capture. See KI#222. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	return (entry[0] == CyGame().getGameTurn() and entry[1] == iCapturingPlayer and entry[2] == iOldOwner and entry[5] == iX and entry[6] == iY and not isRetreatEntry(entry))

def _patchCapturedEntry(entries, iCapturingPlayer, iOldOwner, iX, iY, iCapturedUnitType):
	iIndex = len(entries) - 1
	while iIndex >= 0:
		if _matchesCapturedBattle(entries[iIndex], iCapturingPlayer, iOldOwner, iX, iY):
			entries[iIndex] = _getEntryWithCapture(entries[iIndex], iCapturingPlayer, iCapturedUnitType)
			return True
		iIndex -= 1
	return False

def _matchesCityCapturedBattle(entry, iPreviousOwner, iNewOwner, iX, iY):
	if len(entry) < 7:
		return False
	# <!-- custom: An undefended conquest fires cityAcquired without combat.
	# Require a lethal battle at this city plot so an unrelated same-turn battle between the same players is not relabeled as the city capture. See KI#221. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	return (entry[0] == CyGame().getGameTurn() and entry[1] == iNewOwner and entry[2] == iPreviousOwner and entry[5] == iX and entry[6] == iY and not isRetreatEntry(entry))

def _patchCityCapturedEntry(entries, iPreviousOwner, iNewOwner, iX, iY):
	iIndex = len(entries) - 1
	while iIndex >= 0:
		if _matchesCityCapturedBattle(entries[iIndex], iPreviousOwner, iNewOwner, iX, iY):
			entries[iIndex] = _setEntryCityContext(entries[iIndex], iX, iY, _PLOT_CONTEXT_CITY_CAPTURED)
			return True
		iIndex -= 1
	return False

def noteCombatActors(cdAttacker, cdDefender, iAttackerXP=None, iDefenderXP=None, bAttackerGG=None, bDefenderGG=None):
	global _PENDING_COMBAT_ACTORS
	global _PENDING_COMBAT_UNIT_CONTEXT
	bKeepUnitContext = (_PENDING_COMBAT_ACTORS is not None and _PENDING_COMBAT_UNIT_CONTEXT is not None and iAttackerXP is None and iDefenderXP is None and _PENDING_COMBAT_ACTORS[0] == int(cdAttacker.eOwner) and _PENDING_COMBAT_ACTORS[1] == int(cdDefender.eOwner))
	_PENDING_COMBAT_ACTORS = (int(cdAttacker.eOwner), int(cdDefender.eOwner), int(cdAttacker.iCurrCombatStr), int(cdAttacker.iMaxCombatStr), int(cdDefender.iCurrCombatStr), int(cdDefender.iMaxCombatStr))
	if bKeepUnitContext:
		return
	_PENDING_COMBAT_UNIT_CONTEXT = None
	if iAttackerXP is not None and iDefenderXP is not None and bAttackerGG is not None and bDefenderGG is not None:
		_PENDING_COMBAT_UNIT_CONTEXT = (int(iAttackerXP), int(bAttackerGG), int(iDefenderXP), int(bDefenderGG))

def recordCombatResult(pWinner, pLoser):
	global _PENDING_COMBAT_ACTORS
	global _PENDING_COMBAT_UNIT_CONTEXT
	iMaxEntries = _getMaxEntries()
	if iMaxEntries == 0:
		return
	iWinner = pWinner.getOwner()
	iLoser = pLoser.getOwner()
	if iWinner < 0 or iLoser < 0:
		return
	iBattleX = pWinner.getX()
	iBattleY = pWinner.getY()
	if _PENDING_COMBAT_ACTORS is not None:
		iPendingAttacker = _PENDING_COMBAT_ACTORS[0]
		iPendingDefender = _PENDING_COMBAT_ACTORS[1]
		if iPendingAttacker == iWinner and iPendingDefender == iLoser:
			iBattleX = pLoser.getX()
			iBattleY = pLoser.getY()
	entry = (CyGame().getGameTurn(), iWinner, iLoser, pWinner.getUnitType(), pLoser.getUnitType(), iBattleX, iBattleY)
	if _PENDING_COMBAT_ACTORS is not None:
		iAttacker, iDefender = _PENDING_COMBAT_ACTORS[:2]
		# <!-- custom: only attach combat-log strength data when its attacker/defender owners match this combat result, so a stale pending tuple from an interrupted/odd combat cannot corrupt the next row. Credit: Claude code Opus 4.7 review. (GPT-5.5) -->
		if (iAttacker == iWinner and iDefender == iLoser) or (iAttacker == iLoser and iDefender == iWinner):
			entry += _PENDING_COMBAT_ACTORS
			iAttackerEndStr = 0
			iDefenderEndStr = 0
			# <!-- custom: combatResult fires after combat damage is applied, so store battle-end effective strength here; capture data still lives before this block for compatibility with the previous saved tuple layout. (GPT-5.5) -->
			if iAttacker == iWinner:
				iAttackerEndStr = _getUnitEndCombatStr(pWinner, _PENDING_COMBAT_ACTORS[3])
			else:
				iDefenderEndStr = _getUnitEndCombatStr(pWinner, _PENDING_COMBAT_ACTORS[5])
			while len(entry) < _END_STRENGTH_DATA_START:
				entry += (0,)
			entry += (iAttackerEndStr, iDefenderEndStr)
	_PENDING_COMBAT_ACTORS = None
	entry = _getEntryWithPlotContext(entry, iBattleX, iBattleY)
	entry = _getEntryWithUnitContext(entry)
	_PENDING_COMBAT_UNIT_CONTEXT = None
	entriesByPlayer = _getEntriesByPlayer()
	_appendEntry(entriesByPlayer, iWinner, entry, iMaxEntries)
	if iLoser != iWinner:
		_appendEntry(entriesByPlayer, iLoser, entry, iMaxEntries)
	_saveEntriesByPlayer(entriesByPlayer)

def recordCombatRetreat(iAttacker, iDefender, iAttackerUnit, iDefenderUnit, iX, iY, iAttackerEndStr, iDefenderEndStr):
	global _PENDING_COMBAT_ACTORS
	global _PENDING_COMBAT_UNIT_CONTEXT
	iMaxEntries = _getMaxEntries()
	if iMaxEntries == 0:
		return
	iAttacker = int(iAttacker)
	iDefender = int(iDefender)
	if iAttacker < 0 or iDefender < 0:
		return
	entry = (CyGame().getGameTurn(), iAttacker, iDefender, int(iAttackerUnit), int(iDefenderUnit), int(iX), int(iY))
	if _PENDING_COMBAT_ACTORS is not None and _PENDING_COMBAT_ACTORS[0] == iAttacker and _PENDING_COMBAT_ACTORS[1] == iDefender:
		entry += _PENDING_COMBAT_ACTORS
	while len(entry) < _END_STRENGTH_DATA_START:
		entry += (0,)
	entry += (int(iAttackerEndStr), int(iDefenderEndStr))
	# <!-- custom: retreat rows have no winner/loser, but older rows use entry[1]/entry[2] as winner/loser.
	# Store attacker/defender there for role/unit perspective, and add the outcome flag after capture/end-strength fields so old saved rows keep their offsets. (GPT-5.5) -->
	while len(entry) < _OUTCOME_DATA_START:
		entry += (0,)
	entry += (_OUTCOME_RETREAT,)
	entry = _getEntryWithPlotContext(entry, int(iX), int(iY))
	entry = _getEntryWithUnitContext(entry)
	_PENDING_COMBAT_ACTORS = None
	_PENDING_COMBAT_UNIT_CONTEXT = None
	entriesByPlayer = _getEntriesByPlayer()
	_appendEntry(entriesByPlayer, iAttacker, entry, iMaxEntries)
	if iDefender != iAttacker:
		_appendEntry(entriesByPlayer, iDefender, entry, iMaxEntries)
	_saveEntriesByPlayer(entriesByPlayer)

def recordUnitCaptured(iOldOwner, iOldUnitType, pNewUnit):
	# <!-- custom: cast the old-owner event arg to plain int; PlayerTypes arrives as an enum/SWIG wrapper from CyArgsList, which makes str(iOldOwner) miss the "3"-style per-player key and entry[2] == iOldOwner fail on int-vs-enum comparison, leaving Cap# / Cap blank.
	# Same pattern as noteCombatActors. (Claude code Opus 4.7) -->
	iOldOwner = int(iOldOwner)
	iCapturingPlayer = int(pNewUnit.getOwner())
	iCapturedUnitType = int(pNewUnit.getUnitType())
	iX = int(pNewUnit.getX())
	iY = int(pNewUnit.getY())
	entriesByPlayer = _getEntriesByPlayer()
	bChanged = False
	for iPlayer in (iCapturingPlayer, iOldOwner):
		szPlayer = str(iPlayer)
		entries = entriesByPlayer.get(szPlayer, [])
		if _patchCapturedEntry(entries, iCapturingPlayer, iOldOwner, iX, iY, iCapturedUnitType):
			entriesByPlayer[szPlayer] = entries
			bChanged = True
	if bChanged:
		_saveEntriesByPlayer(entriesByPlayer)

def recordCityCaptured(iPreviousOwner, iNewOwner, pCity, bConquest):
	if not bConquest:
		return
	iPreviousOwner = int(iPreviousOwner)
	iNewOwner = int(iNewOwner)
	iX = int(pCity.getX())
	iY = int(pCity.getY())
	entriesByPlayer = _getEntriesByPlayer()
	bChanged = False
	for iPlayer in (iNewOwner, iPreviousOwner):
		szPlayer = str(iPlayer)
		entries = entriesByPlayer.get(szPlayer, [])
		if _patchCityCapturedEntry(entries, iPreviousOwner, iNewOwner, iX, iY):
			entriesByPlayer[szPlayer] = entries
			bChanged = True
	if bChanged:
		_saveEntriesByPlayer(entriesByPlayer)

def getEntriesForPlayer(iPlayer):
	entriesByPlayer = _getEntriesByPlayer()
	entries = entriesByPlayer.get(str(iPlayer), [])
	if isinstance(entries, list):
		return entries
	return []

def isRetreatEntry(entry):
	return (len(entry) > _OUTCOME_DATA_START and entry[_OUTCOME_DATA_START] == _OUTCOME_RETREAT)

def getPlotContext(entry):
	if len(entry) < _PLOT_CONTEXT_DATA_START + 2:
		return (-1, -1, -1, _PLOT_CONTEXT_CITY_NONE)
	iHillPeakTerrain = -1
	if len(entry) >= _PLOT_CONTEXT_DATA_START + 3:
		iHillPeakTerrain = entry[_PLOT_CONTEXT_DATA_START + 2]
	iCityContext = _PLOT_CONTEXT_CITY_NONE
	if len(entry) >= _PLOT_CONTEXT_DATA_START + 4:
		iCityContext = entry[_PLOT_CONTEXT_DATA_START + 3]
	return (entry[_PLOT_CONTEXT_DATA_START], entry[_PLOT_CONTEXT_DATA_START + 1], iHillPeakTerrain, iCityContext)

def getUnitContext(entry):
	if len(entry) < _UNIT_CONTEXT_DATA_START + 4:
		return (-1, 0, -1, 0)
	return (entry[_UNIT_CONTEXT_DATA_START], entry[_UNIT_CONTEXT_DATA_START + 1], entry[_UNIT_CONTEXT_DATA_START + 2], entry[_UNIT_CONTEXT_DATA_START + 3])

def isCityContextDefended(iCityContext):
	return iCityContext == _PLOT_CONTEXT_CITY_DEFENDED

def isCityContextCaptured(iCityContext):
	return iCityContext == _PLOT_CONTEXT_CITY_CAPTURED
