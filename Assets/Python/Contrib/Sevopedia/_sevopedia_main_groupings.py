# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: moved to a separate files these we compute these only once fairly independently from external variables, and return a list at the end anyway, which seems much cleaner to do as a separate file rather, with the help of ChatGPT-5.2 Thinking thanks a lot -->
#
# SevoPedia main-list grouping helpers.
# - Purpose: keep SevoPediaMain.py smaller/cleaner by moving "grouping once" logic here.
# - Design: NO imports from SevoPediaMain (avoids circular deps). Use module-scope `gc` + a few flags.
# - Python: keep Civ4/BUG Python 2.4 compatibility (xrange, cmp sorts, etc.).
#
# Expected usage from SevoPediaMain:
#   import _sevopedia_main_groupings as SAS_MainGroupings
#   ... then call SAS_MainGroupings.<func>(..., self.isSortLists(), ...)
#
# <!-- custom: For grouping helpers that accept a prebuilt baseList, SevoPediaMain already applies BUG 'Sort Lists' ordering once via getSortedList()/getUnfilteredSortedList().
# These helpers therefore preserve the incoming order and do not re-sort, avoiding redundant work when list grouping is enabled. (ChatGPT-5.2 Thinking) -->

from CvPythonExtensions import *
from _sevopedia_helpers import *
import os

gc = CyGlobalContext()
localText = CyTranslator()
# <!-- custom: toggle verbose Sevopedia music path/count debug prints in PythonDbg.log; keep disabled by default to avoid log clutter, but retain for future diagnosis of mod-vs-base audio XML resolution issues. (GPT-5.3-Codex) -->
SAS_SEVO_MUSIC_DEBUG_ENABLE = False

def _SAS_findAssetXmlPath(szFileName, szSubDir):
	# <!-- custom: build absolute candidate paths and prefer the mod copy; for Audio2DScripts specifically, pick the candidate that contains AS2D_OPENING_MENU_01 so Sevopedia grouping uses AdvCiv-SAS variants instead of base BTS opening entries.
	# Important: avoid BugPath.findAssetFile() here because this helper can run very early (before BUG init), which can prematurely freeze BugPath asset search paths to base Assets only and later break BUG MainInterface init. See KI#110. (GPT-5.3-Codex) -->
	def _SAS_addCandidate(cands, szPath):
		if not szPath:
			return
		try:
			szNorm = os.path.normpath(os.path.abspath(szPath))
		except:
			szNorm = szPath
		if os.path.isfile(szNorm) and (szNorm not in cands):
			cands.append(szNorm)

	def _SAS_fileContains(szPath, szNeedle):
		try:
			f = open(szPath, "r")
			for line in f:
				if szNeedle in line:
					f.close()
					return True
			f.close()
		except:
			pass
		return False

	candidates = []
	try:
		# <!-- custom: fallback independent from __file__: derive mod Assets from current BTS cwd + AdvCiv's actual EXE-detected mod folder, exposed through CvModName. Do not use the branded display name here: users may rename the installed folder.
		# This still avoids calling BugPath during early startup. See KI#110. (GPT-5.3-Codex + ChatGPT-5.6-Sol) -->
		import CvModName
		szModName = CvModName.getFolderName()
		szCwd = os.getcwd()
		if szModName and szCwd:
			_SAS_addCandidate(candidates, os.path.join(szCwd, "Mods", szModName, "Assets", szSubDir, szFileName))
	except:
		pass

	_SAS_addCandidate(candidates, os.path.join("Assets", szSubDir, szFileName))

	if szFileName == "Audio2DScripts.xml":
		for szPath in candidates:
			if _SAS_fileContains(szPath, "AS2D_OPENING_MENU_01"):
				return szPath

	if len(candidates) > 0:
		return candidates[0]
	return os.path.join("Assets", szSubDir, szFileName)

def SAS_isFoodYieldImprovement(iImprovement):
	# Check if improvement provides food yields from any bonus.
	info = gc.getImprovementInfo(iImprovement)
	if not info or info.isGraphicalOnly():
		return False

	for iBonus in range(gc.getNumBonusInfos()):
		bInfo = gc.getBonusInfo(iBonus)
		if bInfo and bInfo.isGraphicalOnly():
			continue
		# Check if this improvement gives food yield from this bonus
		if info.getImprovementBonusYield(iBonus, YieldTypes.YIELD_FOOD) > 0:
			return True

	return False

# <!-- custom: in sevopedia improvement list, group improvements by whether their terrain is a water type or not (e.g. Land Improvements -> Farm/Pasture, Water Improvements -> Fishing Boats/Offshore Platform) an idea i got from seeing ingame how it is in the Middle-Earth mod which i find very polished and took ideas from btw  thanks; plus other subgroups we added in advciv-sas. Implemented with chatgpt 5.2's help as for as of now the other ones thanks a lot -->
# Group improvements as:
# - Land (Growth): land improvements in an upgrade chain (e.g. Cottage -> Hamlet -> Village -> Town)
# - Land (Bonus-capable): land improvements that can interact with bonuses (trade/connect or bonus yields)
# - Land (Other): remaining land improvements
# - Water (Food): water improvements that provide food (Fishing Boats, Whaling Boats)
# - Water (Other): other water improvements (Offshore Platform)

def SAS_isBonusCapableImprovement(iImprovement):
	info = gc.getImprovementInfo(iImprovement)
	if not info or info.isGraphicalOnly():
		return False

	# If it connects/trades any bonus, it's bonus-capable.
	for iBonus in range(gc.getNumBonusInfos()):
		bInfo = gc.getBonusInfo(iBonus)
		if bInfo and bInfo.isGraphicalOnly():
			continue

		if hasattr(info, "isBonusTrade"):
			if info.isBonusTrade(iBonus):
				return True
		else:
			if info.isImprovementBonusTrade(iBonus):
				return True

	# Some mods may give bonus yields even if trade flags are odd.
	for iBonus in range(gc.getNumBonusInfos()):
		bInfo = gc.getBonusInfo(iBonus)
		if bInfo and bInfo.isGraphicalOnly():
			continue
		for iYield in range(YieldTypes.NUM_YIELD_TYPES):
			if info.getImprovementBonusYield(iBonus, iYield) != 0:
				return True

	return False

def SAS_isBuildCreatedImprovement(iImprovement):
	for iBuild in xrange(gc.getNumBuildInfos()):
		buildInfo = gc.getBuildInfo(iBuild)
		if buildInfo and buildInfo.getImprovement() == iImprovement:
			return True
	return False

def SAS_isSpecialMapImprovement(iImprovement):
	info = gc.getImprovementInfo(iImprovement)
	if not info or info.isGraphicalOnly():
		return False
	if SAS_isBuildCreatedImprovement(iImprovement):
		return False
	return info.getPillageGold() <= 0 and not info.isRequiresFeature() and not info.isOutsideBorders()

# Helper: get ALL improvements that make a bonus tradable (i.e. "improve/connect" it).
# Shared Sevopedia Main list-grouping helpers; some earlier grouping logic was derived from RFC Dawn of Civilization. (ChatGPT-5.6-Sol)
# Unlike a "primary improvement" heuristic, we keep multi-improvement cases exhaustive by using
# a combined header like: "Well, Offshore Platform".
def SAS_getTradingImprovementsForBonus(iBonus):
	bInfo = gc.getBonusInfo(iBonus)
	if not bInfo or bInfo.isGraphicalOnly():
		return []
	r = []
	for iImprovement in range(gc.getNumImprovementInfos()):
		imprInfo = gc.getImprovementInfo(iImprovement)
		if not imprInfo or imprInfo.isGraphicalOnly():
			continue
		# Support both wrapper styles (RFC often exposes isBonusTrade; BtS/AdvCiv uses isImprovementBonusTrade).
		if hasattr(imprInfo, "isBonusTrade"):
			bTrades = imprInfo.isBonusTrade(iBonus)
		else:
			bTrades = imprInfo.isImprovementBonusTrade(iBonus)
		if bTrades:
			r.append(iImprovement)
	return r

# Group bonuses by the improvement(s) that connect them:
#   Farm -> Corn/Wheat/...
#   Pasture -> Sheep/Horse/...
# For multi-improvement bonuses (e.g. Oil: Well + Offshore Platform), we use a combined header.
# <!-- custom: keep every improvement that can trade a bonus instead of using a water/land tie-breaker.
# This ensures Oil lists both Well and Offshore Platform and remains correct if a future bonus can be traded by multiple land improvements.
# The earlier implementation was derived from RFC Dawn of Civilization and reworked for AdvCiv-SAS with ChatGPT 5.2. (ChatGPT-5.6-Sol) -->

def SAS_getTerrainsGroupedByLandWater_fromBaseList(baseList, bSortLists, highIds):
	r = []
	landFlat = []
	graphicalOnlyHigh = []
	water = []

	# Terrain IDs considered "GraphicalOnly (High)" purely for UI grouping (e.g. Hills/Peaks).

	for (szName, iTerrain) in baseList:
		info = gc.getTerrainInfo(iTerrain)

		# IMPORTANT:
		# In your CIV4TerrainInfos.xml, TERRAIN_HILL and TERRAIN_PEAK have <bWater>1</bWater>. Therefore info.isWater() returns True for them. For Sevopedia grouping we still want them under GraphicalOnly (High), so we treat LAND_HIGH as a deliberate UI override.
		if iTerrain in highIds:
			graphicalOnlyHigh.append((szName, iTerrain))
		elif info and info.isWater():
			water.append((szName, iTerrain))
		else:
			landFlat.append((szName, iTerrain))

	# Emit headers + items in alphabetical order by header name
	if graphicalOnlyHigh:
		r.append(("GraphicalOnly (High)", -1))
		for x in graphicalOnlyHigh:
			r.append(x)

	if graphicalOnlyHigh and landFlat:
		r.append(("", -1))

	if landFlat:
		r.append(("Land", -1))
		for x in landFlat:
			r.append(x)

	if (graphicalOnlyHigh or landFlat) and water:
		r.append(("", -1))

	if water:
		r.append(("Water", -1))
		for x in water:
			r.append(x)

	return r

# <!-- custom: in sevopedia feature list, group features by Land (Removable), Land (Other), and Water. Added with the help of chatgpt 5.2 thanks. -->
# Implementation note:
# - We detect "Water" features by checking if the feature can appear on any true water terrain using FeatureInfo.isTerrain(iTerrain).
# - We detect "Removable" land features by scanning CvBuildInfo for a build that removes that feature.
#
# IMPORTANT MOD-SPECIFIC NOTE:
# - In our CIV4TerrainInfos.xml, TERRAIN_HILL and TERRAIN_PEAK have <bWater>1</bWater>.
# - Many land features are valid on hills, so if we treat every isWater terrain as water here,
#   we'd misclassify many land features as "Water".
# - Therefore we exclude our GraphicalOnly (High) terrains from the water-terrain scan used here. -->
def SAS_getFeaturesGroupedByLandWater_fromBaseList(baseList, bSortLists, graphicalOnlyHighIds):
	r = []
	landRemovable = []
	landOther = []
	water = []

	# Exclude our GraphicalOnly (High) terrains (e.g. Hill/Peak) even if XML marks them as water.

	# Build list of "true water terrains" for feature classification.
	waterTerrainIds = []
	for iTerrain in range(gc.getNumTerrainInfos()):
		tInfo = gc.getTerrainInfo(iTerrain)
		if tInfo and tInfo.isWater() and (iTerrain not in graphicalOnlyHighIds):
			waterTerrainIds.append(iTerrain)

	for (szName, iFeature) in baseList:
		fInfo = gc.getFeatureInfo(iFeature)

		# Water feature if it can appear on at least one true water terrain.
		bIsWaterFeature = False
		for iTerrain in waterTerrainIds:
			if fInfo.isTerrain(iTerrain):
				bIsWaterFeature = True
				break

		if bIsWaterFeature:
			water.append((szName, iFeature))
		else:
			# Land feature: split removable vs other
			if SAS_isFeatureRemovable(iFeature):
				landRemovable.append((szName, iFeature))
			else:
				landOther.append((szName, iFeature))

	# Emit headers + items in alphabetical order by header name
	if landOther:
		r.append(("Land (Other)", -1))
		for x in landOther:
			r.append(x)

	if landOther and landRemovable:
		r.append(("", -1))

	if landRemovable:
		r.append(("Land (Removable)", -1))
		for x in landRemovable:
			r.append(x)

	if (landOther or landRemovable) and water:
		r.append(("", -1))

	if water:
		r.append(("Water", -1))
		for x in water:
			r.append(x)

	return r

# Group bonuses by the improvement(s) that connect them:
#   Farm -> Corn/Wheat/...
#   Pasture -> Sheep/Horse/...
# For multi-improvement bonuses (e.g. Oil: Well + Offshore Platform), we use a combined header.
# <!-- custom: keep every improvement that can trade a bonus instead of using a water/land tie-breaker.
# This ensures Oil lists both Well and Offshore Platform and remains correct if a future bonus can be traded by multiple land improvements.
# The earlier implementation was derived from RFC Dawn of Civilization and reworked for AdvCiv-SAS with ChatGPT 5.2. (ChatGPT-5.6-Sol) -->
def SAS_getBonusesGroupedByImprovement_fromBaseList(baseList, bSortLists):
	bonusesList = []

	noImprovement = []
	groups = {}  # key tuple(iImprovement, ...) -> [(szName, iBonus), ...]

	# One pass over bonuses preserves XML order within each group when Sort Lists is OFF.
	for (szName, iBonus) in baseList:
		lImpr = SAS_getTradingImprovementsForBonus(iBonus)
		if not lImpr:
			noImprovement.append((szName, iBonus))
			continue

		# Keep improvement ids in XML order (already in-order due to the loop in SAS_getTradingImprovementsForBonus).
		key = tuple(lImpr)
		tmp = groups.get(key, None)
		if tmp is None:
			tmp = []
			groups[key] = tmp
		tmp.append((szName, iBonus))

	# Order headers:
	#  - primarily by first improvement id (so this generally follows ImprovementInfos XML order),
	#  - then by key length (single-improvement headers first),
	#  - then by the key itself (stable).
	keys = groups.keys()
	def _cmpKeys(a, b):
		ta = (a[0], len(a), a)
		tb = (b[0], len(b), b)
		if ta < tb:
			return -1
		if ta > tb:
			return 1
		return 0
	keys.sort(_cmpKeys)

	for key in keys:
		tmp = groups.get(key, None)
		if not tmp:
			continue

		if bonusesList:
			bonusesList.append(("", -1))

		# Header text: single improvement name OR "A, B, C" for multi-improvement bonuses.
		parts = []
		for iImprovement in key:
			imprInfo = gc.getImprovementInfo(iImprovement)
			if imprInfo:
				parts.append(imprInfo.getDescription())
		szHeader = "Improvement"
		if parts:
			szHeader = ", ".join(parts)
		bonusesList.append((szHeader, -1))

		for x in tmp:
			bonusesList.append(x)

	# Fallback bucket for bonuses that have no connecting/trade improvement (rare)
	if noImprovement:
		if bonusesList:
			bonusesList.append(("", -1))
		bonusesList.append(("Other", -1))
		for x in noImprovement:
			bonusesList.append(x)

	return bonusesList

# <!-- custom: in sevopedia improvement list, group improvements by whether their terrain is a water type or not (e.g. Land Improvements -> Farm/Pasture, Water Improvements -> Fishing Boats/Offshore Platform) an idea i got from seeing ingame how it is in the Middle-Earth mod which i find very polished and took ideas from btw  thanks; plus other subgroups we added in advciv-sas. Implemented with chatgpt 5.2's help as for as of now the other ones thanks a lot -->
# Group improvements as:
# - Land (Growth): land improvements in an upgrade chain (e.g. Cottage -> Hamlet -> Village -> Town)
# - Land (Bonus-capable): land improvements that can interact with bonuses (trade/connect or bonus yields)
# - Land (Other): remaining land improvements
# - Water (Food): water improvements that provide food (Fishing Boats, Whaling Boats)
# - Water (Other): other water improvements (Offshore Platform)
def SAS_getImprovementsGroupedByTerrain_fromBaseList(baseList, bSortLists):
	r = []
	landGrowth = []
	landBonusCapable = []
	landOther = []
	graphicalOnly = []
	specialMap = []
	waterFood = []
	waterOther = []

	# Build set of improvements that are upgraded *to* by something (to detect chain membership, incl. final node).
	dUpgradeTargets = {}
	for (szName, iImprovement) in baseList:
		info = gc.getImprovementInfo(iImprovement)
		if info and hasattr(info, "getImprovementUpgrade"):
			iUp = info.getImprovementUpgrade()
			if iUp != -1:
				dUpgradeTargets[iUp] = 1

	# Cache bonus-capability per improvement to avoid re-scanning bonuses repeatedly.
	dBonusCapable = {}

	# Build improvement -> build tech prereq era mapping for water improvements sorting
	dImprToEra = {}
	for iBuild in xrange(gc.getNumBuildInfos()):
		buildInfo = gc.getBuildInfo(iBuild)
		if buildInfo:
			iImpr = buildInfo.getImprovement()
			if iImpr >= 0:
				iTech = buildInfo.getTechPrereq()
				if iTech >= 0:
					dImprToEra[iImpr] = gc.getTechInfo(iTech).getEra()
				elif iImpr not in dImprToEra:
					dImprToEra[iImpr] = -1

	for (szName, iImprovement) in baseList:
		info = gc.getImprovementInfo(iImprovement)
		# <!-- custom: Expose graphical-only improvements such as worked-land/water markers in Sevopedia, mirroring how
		# Sevopedia Terrain shows graphical-only Peak/Hill because they provide useful map and rules context. Keep these
		# separate from gameplay improvements because helper predicates intentionally ignore graphical-only infos. (GPT-5.5) -->
		if info and info.isGraphicalOnly():
			graphicalOnly.append((szName, iImprovement))
			continue
		# <!-- custom: Show special map improvements separately from Worker improvements. These are not graphical-only
		# infos; they are map/engine-placed improvements with no creating Build, so they fail AdvCiv's old filter for
		# Worker improvements with pillage/features/outside-border rules. World Advisor Territory and BFC 2 can expose
		# them on real plots, so Sevopedia should document them too. (GPT-5.5) -->
		if SAS_isSpecialMapImprovement(iImprovement):
			specialMap.append((szName, iImprovement))
			continue

		if info and info.isWater():
			# Check if water improvement provides food yields from any bonus
			bFood = SAS_isFoodYieldImprovement(iImprovement)
			iEra = dImprToEra.get(iImprovement, -1)
			if bFood:
				waterFood.append((iEra, szName, iImprovement))
			else:
				waterOther.append((iEra, szName, iImprovement))
			continue

		# Land
		bGrowth = False
		if info and hasattr(info, "getImprovementUpgrade"):
			iUp = info.getImprovementUpgrade()
			if iUp != -1 or dUpgradeTargets.has_key(iImprovement):
				bGrowth = True

		if bGrowth:
			landGrowth.append((szName, iImprovement))
		else:
			if dBonusCapable.has_key(iImprovement):
				bBonusCapable = dBonusCapable[iImprovement]
			else:
				bBonusCapable = SAS_isBonusCapableImprovement(iImprovement)
				dBonusCapable[iImprovement] = bBonusCapable

			if bBonusCapable:
				landBonusCapable.append((szName, iImprovement))
			else:
				landOther.append((szName, iImprovement))

	# Sort Land (Growth) by upgrade chain order, not alphabetically
	# Find chain root(s) and walk the upgrade path
	def sortByUpgradeChain(growthList):
		if not growthList:
			return growthList
		# Build upgrade map: iImprovement -> iUpgrade
		upgradeMap = {}
		allIds = set()
		for (szName, iImprovement) in growthList:
			allIds.add(iImprovement)
			info = gc.getImprovementInfo(iImprovement)
			if info and hasattr(info, "getImprovementUpgrade"):
				iUp = info.getImprovementUpgrade()
				if iUp != -1:
					upgradeMap[iImprovement] = iUp
		# Find roots (improvements not upgraded to by anything in our list)
		roots = []
		for (szName, iImprovement) in growthList:
			isRoot = True
			for tgt in upgradeMap.values():
				if tgt == iImprovement:
					isRoot = False
					break
			if isRoot:
				roots.append(iImprovement)
		# Walk chains from roots
		result = []
		visited = set()
		for root in roots:
			current = root
			while current is not None and current not in visited:
				visited.add(current)
				for (szName, iImprovement) in growthList:
					if iImprovement == current:
						result.append((szName, iImprovement))
						break
				current = upgradeMap.get(current, None)
		# Add any remaining (shouldn't happen, but safety)
		for item in growthList:
			if item[1] not in visited:
				result.append(item)
		return result

	landGrowth = sortByUpgradeChain(landGrowth)

	# Sort water improvements by tech prereq era, then extract (szName, iImprovement)
	waterFood.sort(key=lambda x: (x[0], x[1]))  # sort by (iEra, szName)
	waterFood = [(item[1], item[2]) for item in waterFood]

	waterOther.sort(key=lambda x: (x[0], x[1]))  # sort by (iEra, szName)
	waterOther = [(item[1], item[2]) for item in waterOther]

	# Emit headers + items in alphabetical order by header name
	if graphicalOnly:
		r.append(("GraphicalOnly", -1))
		for x in graphicalOnly:
			r.append(x)

	if graphicalOnly and (specialMap or landBonusCapable or landGrowth or landOther or waterFood or waterOther):
		r.append(("", -1))

	if specialMap:
		r.append(("Special Map", -1))
		for x in specialMap:
			r.append(x)

	if specialMap and (landBonusCapable or landGrowth or landOther or waterFood or waterOther):
		r.append(("", -1))

	if landBonusCapable:
		r.append(("Land (Bonus-capable)", -1))
		for x in landBonusCapable:
			r.append(x)

	if landBonusCapable and (landGrowth or landOther or waterFood or waterOther):
		r.append(("", -1))

	if landGrowth:
		r.append(("Land (Growth)", -1))
		for x in landGrowth:
			r.append(x)

	if landGrowth and (landOther or waterFood or waterOther):
		r.append(("", -1))

	if landOther:
		r.append(("Land (Other)", -1))
		for x in landOther:
			r.append(x)

	if (landBonusCapable or landGrowth or landOther) and (waterFood or waterOther):
		r.append(("", -1))

	if waterFood:
		r.append(("Water (Food)", -1))
		for x in waterFood:
			r.append(x)

	if waterFood and waterOther:
		r.append(("", -1))

	if waterOther:
		r.append(("Water (Other)", -1))
		for x in waterOther:
			r.append(x)

	return r

# <!-- custom: in sevopedia build list, group builds mirroring the improvement/feature categories.
# Implemented with claude opus 4.5's help thanks a lot -->
# Group builds as:
# - Land (Growth): builds that create growth-chain improvements (Cottage->Hamlet->Village->Town) - sorted by upgrade chain order
# - Land (Bonus-capable): builds that create bonus-capable improvements (Farm, Mine, etc.)
# - Land (Other): builds that create other land improvements (Fort, Forest Preserve, etc.)
# - Land (Removable): builds that remove features without creating improvements (Chop Down, Remove Jungle, Scrub Fallout)
# - Land (Route): builds that create routes (Road, Railroad)
# - Water (Food): builds that create food-yielding water improvements (Fishing Boats, Whaling Boats)
# - Water (Other): builds that create other water improvements (Offshore Platform)
def SAS_getBuildsGroupedByType_fromBaseList(baseList, bSortLists):
	r = []
	landGrowth = []
	landBonusCapable = []
	landOther = []
	waterFood = []
	waterOther = []
	landRoute = []
	landRemovable = []

	# Build set of improvements that are upgraded *to* by something (to detect chain membership, incl. final node).
	dUpgradeTargets = {}
	for i in xrange(gc.getNumImprovementInfos()):
		info = gc.getImprovementInfo(i)
		if info and hasattr(info, "getImprovementUpgrade"):
			iUp = info.getImprovementUpgrade()
			if iUp != -1:
				dUpgradeTargets[iUp] = 1

	# Also build improvement -> build mapping for chain ordering
	imprToBuild = {}

	for (szName, iBuild) in baseList:
		buildInfo = gc.getBuildInfo(iBuild)
		if not buildInfo:
			continue

		iImprovement = buildInfo.getImprovement()
		iRoute = buildInfo.getRoute()

		if iRoute > -1:
			# Store tech prereq era for sorting (Road before Railroad)
			iTech = buildInfo.getTechPrereq()
			iEra = -1
			if iTech >= 0:
				iEra = gc.getTechInfo(iTech).getEra()
			landRoute.append((iEra, szName, iBuild))
		elif iImprovement > -1:
			imprToBuild[iImprovement] = (szName, iBuild)
			imprInfo = gc.getImprovementInfo(iImprovement)

			# Check if water improvement
			if imprInfo and imprInfo.isWater():
				bFood = SAS_isFoodYieldImprovement(iImprovement)
				# Store tech prereq era for sorting
				iTech = buildInfo.getTechPrereq()
				iEra = -1
				if iTech >= 0:
					iEra = gc.getTechInfo(iTech).getEra()
				if bFood:
					waterFood.append((iEra, szName, iBuild))
				else:
					waterOther.append((iEra, szName, iBuild))
			else:
				# Check if growth-chain improvement
				bGrowth = False
				if imprInfo and hasattr(imprInfo, "getImprovementUpgrade"):
					iUp = imprInfo.getImprovementUpgrade()
					if iUp != -1 or dUpgradeTargets.has_key(iImprovement):
						bGrowth = True

				if bGrowth:
					landGrowth.append((szName, iBuild, iImprovement))  # include iImprovement for chain sorting
				else:
					# Check if bonus-capable
					bBonusCapable = SAS_isBonusCapableImprovement(iImprovement)
					if bBonusCapable:
						landBonusCapable.append((szName, iBuild))
					else:
						landOther.append((szName, iBuild))
		else:
			landRemovable.append((szName, iBuild))

	# Sort Land (Growth) by upgrade chain order, not alphabetically
	def sortBuildsByUpgradeChain(growthList):
		if not growthList:
			return []
		# Build upgrade map: iImprovement -> iUpgrade
		upgradeMap = {}
		allIds = set()
		for item in growthList:
			iImprovement = item[2]
			allIds.add(iImprovement)
			info = gc.getImprovementInfo(iImprovement)
			if info and hasattr(info, "getImprovementUpgrade"):
				iUp = info.getImprovementUpgrade()
				if iUp != -1:
					upgradeMap[iImprovement] = iUp
		# Find roots (improvements not upgraded to by anything in our list)
		roots = []
		for item in growthList:
			iImprovement = item[2]
			isRoot = True
			for tgt in upgradeMap.values():
				if tgt == iImprovement:
					isRoot = False
					break
			if isRoot:
				roots.append(iImprovement)
		# Walk chains from roots
		result = []
		visited = set()
		for root in roots:
			current = root
			while current is not None and current not in visited:
				visited.add(current)
				for item in growthList:
					if item[2] == current:
						result.append((item[0], item[1]))  # (szName, iBuild)
						break
				current = upgradeMap.get(current, None)
		# Add any remaining (shouldn't happen, but safety)
		for item in growthList:
			if item[2] not in visited:
				result.append((item[0], item[1]))
		return result

	landGrowth = sortBuildsByUpgradeChain(landGrowth)

	# Sort by tech prereq era, then extract (szName, iBuild)
	landRoute.sort(key=lambda x: (x[0], x[1]))  # sort by (iEra, szName)
	landRoute = [(item[1], item[2]) for item in landRoute]

	waterFood.sort(key=lambda x: (x[0], x[1]))  # sort by (iEra, szName)
	waterFood = [(item[1], item[2]) for item in waterFood]

	waterOther.sort(key=lambda x: (x[0], x[1]))  # sort by (iEra, szName)
	waterOther = [(item[1], item[2]) for item in waterOther]

	# Emit headers + items in alphabetical order by header name
	if landBonusCapable:
		r.append(("Land (Bonus-capable)", -1))
		for x in landBonusCapable:
			r.append(x)

	if landBonusCapable and (landGrowth or landOther or landRemovable or landRoute or waterFood or waterOther):
		r.append(("", -1))

	if landGrowth:
		r.append(("Land (Growth)", -1))
		for x in landGrowth:
			r.append(x)

	if landGrowth and (landOther or landRemovable or landRoute or waterFood or waterOther):
		r.append(("", -1))

	if landOther:
		r.append(("Land (Other)", -1))
		for x in landOther:
			r.append(x)

	if landOther and (landRemovable or landRoute or waterFood or waterOther):
		r.append(("", -1))

	if landRemovable:
		r.append(("Land (Removable)", -1))
		for x in landRemovable:
			r.append(x)

	if landRemovable and (landRoute or waterFood or waterOther):
		r.append(("", -1))

	if landRoute:
		r.append(("Land (Route)", -1))
		for x in landRoute:
			r.append(x)

	if landRoute and (waterFood or waterOther):
		r.append(("", -1))

	if waterFood:
		r.append(("Water (Food)", -1))
		for x in waterFood:
			r.append(x)

	if waterFood and waterOther:
		r.append(("", -1))

	if waterOther:
		r.append(("Water (Other)", -1))
		for x in waterOther:
			r.append(x)

	return r

# <!-- custom: Era / category grouping helpers (Techs, Units, Buildings, Projects, Religions, Corporations, Specialists, Civics)
#
# Notes:
# - For era-tiered lists that depend on "availability era" logic, callers pass a callback:
#     getEraFn(itemId, *extraCounts) -> iEra (>=0), -1 for "no tech prereq", or None to skip. (ChatGPT-5.2 Thinking) -->

# # <!-- custom: Availability-era helpers (used by era groupings). These were previously methods on SevoPediaMain; moved here to keep groupings self-contained. (ChatGPT-5.2 Thinking) -->
_SAS_cacheCorporationHQBuildingByCorp = None

# In AdvCiv-SAS, your corporations are effectively gated by the founding building (the BUILDING_CORPORATION_X that has <FoundsCorporation>CORPORATION_X</FoundsCorporation> and has <PrereqTech> / <TechTypes>), while the corresponding CIV4CorporationInfo.xml often has <TechPrereq>NONE</TechPrereq>. So the clean "era" for a corporation should be the availability era of its founding building.
# This reuses your existing SAS_getBuildingAvailabilityEra(iBuilding, iNumAndTechs) helper (the one that already accounts for SpecialBuilding tech + religion founding tech).
# Map each corporation to its founding (HQ) building once, then reuse; used for grouping corporations by era.
def SAS_getCorporationHQBuilding(iCorporation):
	global _SAS_cacheCorporationHQBuildingByCorp
	if _SAS_cacheCorporationHQBuildingByCorp is None:
		m = {}
		for iBuilding in range(gc.getNumBuildingInfos()):
			bInfo = gc.getBuildingInfo(iBuilding)
			if not bInfo or bInfo.isGraphicalOnly():
				continue
			iFounds = bInfo.getFoundsCorporation()
			if iFounds >= 0:
				m[iFounds] = iBuilding
		_SAS_cacheCorporationHQBuildingByCorp = m
	return _SAS_cacheCorporationHQBuildingByCorp.get(iCorporation, -1)

def SAS_getBuildingAvailabilityEra(iBuilding, iNumAndTechs):
	info = gc.getBuildingInfo(iBuilding)
	if not info or info.isGraphicalOnly():
		return None  # caller should skip

	iEra = -1

	# Main AND prereq tech (<PrereqTech>)
	# <!-- custom: it seems getPrereqOrTechs does not exist causing a python error, but with the help of chatgpt 5.2 found the correct way to fetch tech prereqs that addresses and fixes it and that works as intended in displaying the building at latest tech prereq of the building's corresponding era. Seems to run fine in testing/empirically -->
	# When first implementing era-tiered building lists, we hit a crash: AttributeError: 'CvBuildingInfo' object has no attribute 'getPrereqOrTechs' (Python traceback from SevoPediaMain.getBuildingList). We verified the cause by inspecting our exported Cy/Cv infos (see as of now Sevopedia/Debug/): CvBuildingInfo in this DLL exposes getPrereqAndTech() + getPrereqAndTechs(i), but NOT getPrereqOrTechs(i) (unlike some other mods / DLLs). (Mapping to our XML schema: <PrereqTech> maps to getPrereqAndTech(), and <TechTypes><PrereqTech>...</PrereqTech></TechTypes> maps to getPrereqAndTechs(i).)
	# Fix: compute the building "availability era" using only these AND-tech prereqs: start with getPrereqAndTech(), then scan additional prereqs via getPrereqAndTechs(i) for i in range(gc.getNUM_BUILDING_AND_TECH_PREREQS()). We bucket the building into the LATEST (max) era among these prereq techs, so it shows up in the era when it actually becomes buildable.
	# Empirical sanity check: we tested a building with two widely separated PrereqTech and TechTypes (e.g. TECH_MATHEMATICS (Classical) and TECH_FUSION (Future)) in both permutations (A then B, and B then A), and in both cases the building was listed under the later era (Future), confirming the "max era of prereqs" rule behaves correctly in practice.
	# Additional sanity check: we also tested <PrereqTech>NONE</PrereqTech> with an existing TechTypes prereq (i.e. only the <TechTypes> prereq was set), and the building was still listed under the correct era, confirming the fallback scan of getPrereqAndTechs(i) works even when the main <PrereqTech> is NONE. -->
	iTech = info.getPrereqAndTech()
	if iTech >= 0:
		iEra = gc.getTechInfo(iTech).getEra()

	# Extra AND tech prereqs (<TechTypes><PrereqTech>...)
	for j in range(iNumAndTechs):
		iTech2 = info.getPrereqAndTechs(j)
		if iTech2 >= 0:
			iEra2 = gc.getTechInfo(iTech2).getEra()
			if iEra2 > iEra:
				iEra = iEra2

	# SpecialBuilding tech prereq (eg monasteries gated by SPECIALBUILDING tech)
	# <!-- custom: special buildings need also to be checked for a tech prereq. For example the buddhist monastery as of now has no TechPrereq and TechTypes NONE, but the parent specialbuilding_monastery requires TECH_MONARCHY. Yet, without taking special buildings into account, the buddhist monastery is incorrectly listed at the "No Tech Prereq" part instead of at the "Classical Era" part. Fix this by taking their actual tech prereq into account properly with the help of chatgpt 5.2 thanks -->
	# Yep — that’s exactly why: for "special buildings" (Temple/Cathedral/Monastery/etc.), the real tech gate often lives in CIV4SpecialBuildingInfos.xml (e.g. SPECIALBUILDING_MONASTERY has TechPrereq = TECH_MONARCHY). So your era-bucketing currently sees "no building tech prereq" and dumps it into All Eras.
	# Also consider SpecialBuilding tech prereq (e.g. Monastery -> TECH_MONARCHY in CIV4SpecialBuildingInfos.xml)
	iSpecialBuildingType = info.getSpecialBuildingType()
	if iSpecialBuildingType >= 0:
		iSpecialTech = gc.getSpecialBuildingInfo(iSpecialBuildingType).getTechPrereq()
		if iSpecialTech >= 0:
			iSpecialEra = gc.getTechInfo(iSpecialTech).getEra()
			if iSpecialEra > iEra:
				iEra = iSpecialEra

	# Also consider PrereqReligion tech prereq (religion founding tech).
	# Example: a Christian Temple may have no building tech prereq, but RELIGION_CHRISTIANITY is founded by TECH_MONOTHEISM, so the building effectively cannot exist before that technology (except via conquest/trade of a religion-enabled city).
	iPrereqReligion = info.getPrereqReligion()
	if iPrereqReligion >= 0:
		iReligionTech = gc.getReligionInfo(iPrereqReligion).getTechPrereq()
		if iReligionTech >= 0:
			iReligionEra = gc.getTechInfo(iReligionTech).getEra()
			if iReligionEra > iEra:
				iEra = iReligionEra

	return iEra  # -1 means "no tech prereq bucket"

def SAS_getUnitAvailabilityEra(iUnit, iNumUnitAndTechs, iNumBuildingAndTechs):
	info = gc.getUnitInfo(iUnit)
	if not info or info.isGraphicalOnly():
		return None  # caller should skip

	iEra = -1

	# Unit tech prereqs: <PrereqTech> + <TechTypes>
	iTech = info.getPrereqAndTech()
	if iTech >= 0:
		iEra = gc.getTechInfo(iTech).getEra()

	for j in range(iNumUnitAndTechs):
		iTech2 = info.getPrereqAndTechs(j)
		if iTech2 >= 0:
			iEra2 = gc.getTechInfo(iTech2).getEra()
			if iEra2 > iEra:
				iEra = iEra2

	# PrereqBuilding availability era (reuses building-era helper, so it includes SpecialBuilding tech + religion tech, etc.)
	iPrereqBuilding = info.getPrereqBuilding()
	if iPrereqBuilding >= 0:
		iBuildingEra = SAS_getBuildingAvailabilityEra(iPrereqBuilding, iNumBuildingAndTechs)
		if iBuildingEra is not None and iBuildingEra > iEra:
			iEra = iBuildingEra

	# PrereqReligion tech prereq (religion founding tech).
	# (Mostly redundant for missionaries in your setup since they already require a monastery building, but harmless and fixes any unit that uses PrereqReligion without a building gate.)
	iPrereqReligion = info.getPrereqReligion()
	if iPrereqReligion >= 0:
		iReligionTech = gc.getReligionInfo(iPrereqReligion).getTechPrereq()
		if iReligionTech >= 0:
			iReligionEra = gc.getTechInfo(iReligionTech).getEra()
			if iReligionEra > iEra:
				iEra = iReligionEra

	# <!-- custom: note: executives's tech actual requirement not implemented here, as in advciv-sas they also have a prereq tech (see XML code comments or main changes guide or such for rationale). Otherwise the implementation so they are listed at e.g. "Industrial" Era and not "No Tech Prerequisite" (which does not reflect their effective ingame availabilty: not until later eras) would be tedious from what i understand of chatgpt 5.2's explanation and solution (plus we don't need to so better not); check if accurate -->
	# Yes — for your mod, adding a tech prereq directly on Executive units is the simplest and arguably the cleanest fix, and it also matches the design logic you already used for shrines ("captured thing exists locally, but you can’t mass-produce/spread it without understanding the tech").

	return iEra  # -1 means "no tech prereq bucket"

# Compute the "availability era" for a corporation, based on its founding building's prereq tech era.
def SAS_getCorporationAvailabilityEra(iCorporation, iNumBuildingAndTechs):
	cInfo = gc.getCorporationInfo(iCorporation)
	if not cInfo or cInfo.isGraphicalOnly():
		return None  # caller should skip

	iEra = -1

	# In AdvCiv-SAS, corporations are typically gated by the founding building techs (see CIV4BuildingInfos.xml <FoundsCorporation>).
	iHQBuilding = SAS_getCorporationHQBuilding(iCorporation)
	if iHQBuilding >= 0:
		iBuildingEra = SAS_getBuildingAvailabilityEra(iHQBuilding, iNumBuildingAndTechs)
		if iBuildingEra is not None and iBuildingEra > iEra:
			iEra = iBuildingEra

	return iEra  # -1 means "No Tech Prerequisite" bucket

def SAS_getTechsGroupedByEra(bSortLists):
	techsList = []

	iNumEras = gc.getNumEraInfos()
	groups = {}  # iEra -> [(szName, iTech), ...]

	# One pass over TechInfos preserves XML order when Sort Lists is OFF
	iNumTechs = gc.getNumTechInfos()
	for iTech in range(iNumTechs):
		info = gc.getTechInfo(iTech)
		if info.isGraphicalOnly():
			continue

		iEra = info.getEra()
		szName = info.getDescription()

		if iEra not in groups:
			groups[iEra] = []
		groups[iEra].append((szName, iTech))

	# Emit era groups in order
	for iEra in range(iNumEras):
		tmp = groups.get(iEra, None)
		if not tmp:
			continue

		if techsList:
			techsList.append(("", -1))  # spacer between eras
		techsList.append((gc.getEraInfo(iEra).getDescription() + " " + localText.getText("TXT_KEY_PEDIA_ERA", ()), -1))

		for x in tmp:
			techsList.append(x)

	return techsList

def SAS_getUnitsGroupedByEra_fromBaseList(baseList, bSortLists, getUnitAvailabilityEraFn=None):
	# getUnitAvailabilityEraFn signature:
	#   fn(iUnit, iNumUnitAndTechs, iNumBuildingAndTechs) -> iEra / -1 / None

	# Default to the shared availability-era helper in this module (no SevoPediaMain dependency).
	if getUnitAvailabilityEraFn is None:
		def getUnitAvailabilityEraFn(iUnit, iNumUnitAndTechs, iNumBuildingAndTechs):
			return SAS_getUnitAvailabilityEra(iUnit, iNumUnitAndTechs, iNumBuildingAndTechs)

	unitsList = []

	iNumEras = gc.getNumEraInfos()
	iNumUnitAndTechs = gc.getNUM_UNIT_AND_TECH_PREREQS()
	iNumBuildingAndTechs = gc.getNUM_BUILDING_AND_TECH_PREREQS()

	noTech = []
	groups = {}  # iEra -> [(szName, iUnit), ...]

	# One pass: compute era once and bucket
	for (szName, iUnit) in baseList:
		iEra = getUnitAvailabilityEraFn(iUnit, iNumUnitAndTechs, iNumBuildingAndTechs)
		if iEra is None:
			continue

		if iEra == -1:
			noTech.append((szName, iUnit))
		else:
			if iEra not in groups:
				groups[iEra] = []
			groups[iEra].append((szName, iUnit))

	# "No Tech Prerequisite" group first
	if noTech:
		unitsList.append((localText.getText("TXT_KEY_PEDIA_NO_TECH_PREREQUISITE", ()), -1))
		for x in noTech:
			unitsList.append(x)

	# Era groups in order
	for iEraLoop in range(iNumEras):
		tmp = groups.get(iEraLoop, None)
		if not tmp:
			continue

		if unitsList:
			unitsList.append(("", -1))

		unitsList.append((gc.getEraInfo(iEraLoop).getDescription() + " " + localText.getText("TXT_KEY_PEDIA_ERA", ()), -1))
		for x in tmp:
			unitsList.append(x)

	return unitsList

def SAS_getBuildingsGroupedByEra_fromBaseList(baseList, bSortLists, getBuildingAvailabilityEraFn=None):
	# getBuildingAvailabilityEraFn signature:
	#   fn(iBuilding, iNumAndTechs) -> iEra / -1 / None

	# Default to the shared availability-era helper in this module (no SevoPediaMain dependency).
	if getBuildingAvailabilityEraFn is None:
		def getBuildingAvailabilityEraFn(iBuilding, iNumAndTechs):
			return SAS_getBuildingAvailabilityEra(iBuilding, iNumAndTechs)

	buildingsList = []

	iNumEras = gc.getNumEraInfos()
	iNumAndTechs = gc.getNUM_BUILDING_AND_TECH_PREREQS()

	noTech = []
	groups = {}  # iEra -> [(szName, iBuilding), ...]

	# One pass: compute era once and bucket
	for (szName, iBuilding) in baseList:
		iEra = getBuildingAvailabilityEraFn(iBuilding, iNumAndTechs)
		if iEra is None:
			continue  # graphical-only or invalid

		if iEra == -1:
			noTech.append((szName, iBuilding))
		else:
			if iEra not in groups:
				groups[iEra] = []
			groups[iEra].append((szName, iBuilding))

	# "No Tech Prereq" group first
	if noTech:
		buildingsList.append((localText.getText("TXT_KEY_PEDIA_NO_TECH_PREREQUISITE", ()), -1))
		for x in noTech:
			buildingsList.append(x)

	# Era groups in order
	for iEraLoop in range(iNumEras):
		tmp = groups.get(iEraLoop, None)
		if not tmp:
			continue

		if buildingsList:
			buildingsList.append(("", -1))

		buildingsList.append((gc.getEraInfo(iEraLoop).getDescription() + " " + localText.getText("TXT_KEY_PEDIA_ERA", ()), -1))
		for x in tmp:
			buildingsList.append(x)

	return buildingsList

def SAS_getProjectsGroupedByEra_fromBaseList(baseList, bSortLists, getProjectAvailabilityEraFn=None):
	# getProjectAvailabilityEraFn signature:
	#   fn(iProject) -> iEra / -1 / None

	projectsList = []

	iNumEras = gc.getNumEraInfos()

	noTech = []
	groups = {}  # iEra -> [(szName, iProject), ...]

	# One pass: compute era once and bucket
	for (szName, iProject) in baseList:
		iEra = getProjectAvailabilityEraFn(iProject)
		if iEra is None:
			continue

		if iEra == -1:
			noTech.append((szName, iProject))
		else:
			if iEra not in groups:
				groups[iEra] = []
			groups[iEra].append((szName, iProject))

	# "No Tech Prereq" group first
	if noTech:
		projectsList.append((localText.getText("TXT_KEY_PEDIA_NO_TECH_PREREQUISITE", ()), -1))
		for x in noTech:
			projectsList.append(x)

	# Era groups in order
	for iEraLoop in range(iNumEras):
		tmp = groups.get(iEraLoop, None)
		if not tmp:
			continue

		if projectsList:
			projectsList.append(("", -1))

		projectsList.append((gc.getEraInfo(iEraLoop).getDescription() + " " + localText.getText("TXT_KEY_PEDIA_ERA", ()), -1))
		for x in tmp:
			projectsList.append(x)

	return projectsList

def SAS_getReligionsGroupedByEra_fromBaseList(baseList, bSortLists, getReligionAvailabilityEraFn=None):
	# getReligionAvailabilityEraFn signature:
	#   fn(iReligion) -> iEra / -1 / None

	religionsList = []

	iNumEras = gc.getNumEraInfos()

	noTech = []
	groups = {}  # iEra -> [(szName, iReligion), ...]

	# One pass: compute era once and bucket
	for (szName, iReligion) in baseList:
		iEra = getReligionAvailabilityEraFn(iReligion)
		if iEra is None:
			continue

		if iEra == -1:
			noTech.append((szName, iReligion))
		else:
			# Keep has_key style for Py2.4 familiarity / consistency
			if not groups.has_key(iEra):
				groups[iEra] = []
			groups[iEra].append((szName, iReligion))

	# "No Tech Prerequisite" group first
	if noTech:
		religionsList.append((localText.getText("TXT_KEY_PEDIA_NO_TECH_PREREQUISITE", ()), -1))
		for x in noTech:
			religionsList.append(x)

	# Era groups in order
	for iEraLoop in range(iNumEras):
		tmp = groups.get(iEraLoop, None)
		if not tmp:
			continue

		if religionsList:
			religionsList.append(("", -1))

		religionsList.append((gc.getEraInfo(iEraLoop).getDescription() + " " + localText.getText("TXT_KEY_PEDIA_ERA", ()), -1))
		for x in tmp:
			religionsList.append(x)

	return religionsList

# Helper to group corporations by era for Sevopedia lists, mirroring the building/unit/tech patterns.
def SAS_getCorporationsGroupedByEra_fromBaseList(baseList, bSortLists, getCorporationAvailabilityEraFn=None):
	# getCorporationAvailabilityEraFn signature:
	#   fn(iCorporation, iNumBuildingAndTechs) -> iEra / -1 / None

	# Default to the shared availability-era helper in this module (no SevoPediaMain dependency).
	if getCorporationAvailabilityEraFn is None:
		def getCorporationAvailabilityEraFn(iCorporation, iNumBuildingAndTechs):
			return SAS_getCorporationAvailabilityEra(iCorporation, iNumBuildingAndTechs)

	corpsList = []

	iNumEras = gc.getNumEraInfos()
	iNumBuildingAndTechs = gc.getNUM_BUILDING_AND_TECH_PREREQS()

	noTech = []
	groups = {}  # iEra -> [(szName, iCorporation), ...]

	for (szName, iCorporation) in baseList:
		iEra = getCorporationAvailabilityEraFn(iCorporation, iNumBuildingAndTechs)
		if iEra is None:
			continue

		if iEra == -1:
			noTech.append((szName, iCorporation))
		else:
			if iEra not in groups:
				groups[iEra] = []
			groups[iEra].append((szName, iCorporation))

	if noTech:
		corpsList.append((localText.getText("TXT_KEY_PEDIA_NO_TECH_PREREQUISITE", ()), -1))
		for x in noTech:
			corpsList.append(x)

	for iEraLoop in range(iNumEras):
		tmp = groups.get(iEraLoop, None)
		if not tmp:
			continue

		if corpsList:
			corpsList.append(("", -1))

		corpsList.append((gc.getEraInfo(iEraLoop).getDescription() + " " + localText.getText("TXT_KEY_PEDIA_ERA", ()), -1))
		for x in tmp:
			corpsList.append(x)

	return corpsList

# <!-- custom: in Sevopedia specialists, group regular and great specialists (those whose <Type> contains "GREAT_").
# The earlier grouping implementation was derived from RFC Dawn of Civilization and adapted with ChatGPT 5.2. (ChatGPT-5.6-Sol) -->
# Notes (kept conservative)
# 	- "Great specialists" detection is exactly RFC’s convention: getType().find("GREAT_") > -1. 
# 	- I reused TXT_KEY_PEDIA_CATEGORY_SPECIALIST for the regular header (so it’s localized), and I used the literal string "Great Specialists" for the great header (since your mod likely doesn’t have RFC’s TXT_KEY_PEDIA_HEADER_GREAT_SPECIALIST). If you want, you can later add your own TXT_KEY and swap that line to localText.getText("TXT_KEY_PEDIA_HEADER_GREAT_SPECIALIST", ()).
def SAS_getSpecialistsGroupedByType(bSortLists):
	specialistsList = []
	greatSpecialistsList = []

	# One pass: preserve XML order when Sort Lists is OFF.
	for iSpecialist in range(gc.getNumSpecialistInfos()):
		info = gc.getSpecialistInfo(iSpecialist)
		if not info or info.isGraphicalOnly():
			continue

		szName = info.getDescription()
		szType = info.getType()

		if szType and (szType.find("GREAT_") > -1):
			greatSpecialistsList.append((szName, iSpecialist))
		else:
			specialistsList.append((szName, iSpecialist))

	# Optional sorting within each group
	if bSortLists:
		specialistsList.sort()
		greatSpecialistsList.sort()

	outList = []

	if specialistsList:
		# Reuse the category label for a localized "Specialists" header.
		outList.append((localText.getText("TXT_KEY_PEDIA_CATEGORY_SPECIALIST", ()), -1))
		for x in specialistsList:
			outList.append(x)

	if specialistsList and greatSpecialistsList:
		outList.append(("", -1))  # spacer between groups

	if greatSpecialistsList:
		outList.append(("Great Specialists", -1))
		for x in greatSpecialistsList:
			outList.append(x)

	return outList

# <!-- custom: in Sevopedia civics, order civics by civic type (e.g. Government, Economy, etc.).
# The earlier implementation was derived from RFC Dawn of Civilization and adapted with ChatGPT 5.2. (ChatGPT-5.6-Sol) -->
# Step 2: Replace getCivicList() with "category + era tiers" (behind a SAS define)
# Right now your civics list is just getSortedList(gc.getNumCivicInfos(), gc.getCivicInfo) (optionally alphabetical via BUG).
# The adopted grouping approach uses non-clickable header rows for civic-option categories and related grouped lists. (ChatGPT-5.6-Sol)
def SAS_getCivicsGroupedByCivicOption(bSortLists):
	civicsList = []
	iNumCivics = gc.getNumCivicInfos()
	iNumOptions = gc.getNumCivicOptionInfos()

	# One pass: bucket civics by option (preserves XML order naturally)
	groups = [[] for _ in range(iNumOptions)]
	for iCivic in range(iNumCivics):
		info = gc.getCivicInfo(iCivic)
		if info.isGraphicalOnly():
			continue
		iOption = info.getCivicOptionType()
		if iOption >= 0 and iOption < iNumOptions:
			groups[iOption].append((info.getDescription(), iCivic))

	# Emit in option order (Government, Legal, Labor, etc.)
	for iOption in range(iNumOptions):
		tmp = groups[iOption]
		if not tmp:
			continue

		# <!-- custom: do not alphabetize civics inside each civic-option group; keep their CIV4CivicInfos.xml order so it is more intuitive and logical to read (for civics). (GPT-5.3-Codex) -->
		# # If BUG "Sort Lists" is ON, alphabetize within each option group
		# if bSortLists:
		# 	tmp.sort()

		if civicsList:
			civicsList.append(("", -1))  # spacer between groups
		civicsList.append((gc.getCivicOptionInfo(iOption).getDescription(), -1))  # header

		for x in tmp:
			civicsList.append(x)

	return civicsList

# <!-- custom: in sevopedia leaders, group leaders by civilization so same-civ leaders are adjacent
# under each civ header (e.g. Persia -> Cyrus, Darius). (GPT-5.3-Codex) -->
def SAS_getLeadersGroupedByCivilization(bSortLists):
	leadersList = []
	iNumLeaders = gc.getNumLeaderHeadInfos()
	iNumCivs = gc.getNumCivilizationInfos()
	civGroups = []
	excludedLeaderIndexes = set(get_leader_indexes_from_leader_types(EXCLUDED_LEADER_TYPES_FROM_SEVOPEDIA))

	for iCiv in range(iNumCivs):
		civInfo = gc.getCivilizationInfo(iCiv)
		if (not civInfo) or civInfo.isGraphicalOnly():
			continue

		civLeaders = []
		for iLeader in range(iNumLeaders):
			if iLeader in excludedLeaderIndexes:
				continue
			leaderInfo = gc.getLeaderHeadInfo(iLeader)
			if (not leaderInfo) or leaderInfo.isGraphicalOnly():
				continue
			if civInfo.isLeaders(iLeader):
				civLeaders.append((leaderInfo.getDescription(), iLeader))

		if not civLeaders:
			continue

		if bSortLists:
			civLeaders.sort()

		civGroups.append((civInfo.getDescription(), civInfo.getButton(), civLeaders))

	# Keep grouped civ headers alphabetical for easier browsing.
	civGroups.sort(key=lambda x: x[0])

	for civName, civButton, civLeaders in civGroups:
		if leadersList:
			leadersList.append(("", -1))
		# Optional 3rd tuple value is a custom icon/button path used by placeItems for header rows.
		leadersList.append((civName, -1, civButton))
		for x in civLeaders:
			leadersList.append(x)

	return leadersList

# <!-- custom: in sevopedia civilizations, group civs by ArtStyleType so visually related civs are
# adjacent (e.g. European/Asian/Middle East groups). (GPT-5.3-Codex) -->
def SAS_getCivilizationsGroupedByArtStyle(bSortLists):
	grouped = {}  # iArtStyle -> [(civName, iCiv), ...]

	for iCiv in range(gc.getNumCivilizationInfos()):
		civInfo = gc.getCivilizationInfo(iCiv)
		if (not civInfo) or civInfo.isGraphicalOnly():
			continue
		iArtStyle = civInfo.getArtStyleType()
		if iArtStyle not in grouped:
			grouped[iArtStyle] = []
		grouped[iArtStyle].append((civInfo.getDescription(), iCiv))

	# Build deterministic group order by readable header text.
	groupOrder = []
	for iArtStyle in grouped.keys():
		# Prefer runtime artstyle tag from DLL (e.g. "ARTSTYLE_EUROPEAN"), then make it human-readable.
		szTag = gc.getArtStyleTypes(iArtStyle)
		szHeader = szTag.replace("ARTSTYLE_", "").replace("_", " ").title()
		groupOrder.append((szHeader, iArtStyle))
	groupOrder.sort()

	out = []
	for szHeader, iArtStyle in groupOrder:
		items = grouped[iArtStyle]
		if bSortLists:
			items.sort()
		if out:
			out.append(("", -1))
		out.append((szHeader, -1))
		for x in items:
			out.append(x)

	return out

# <!-- custom: Movies + Music list-building helpers
# - Goal: keep SevoPediaMain.py clean by moving clunky grouping/list assembly here.
# - Note: These build "base lists" (flat item lists) and then add sections, mirroring
#   how other categories are handled in SevoPediaMain + this module. (ChatGPT-5.2 Thinking) -->
def _SAS_addSection(listEntries, szHeader, items):
	# Common helper: header + items, with a blank separator between sections.
	if not items:
		return
	if listEntries:
		listEntries.append(("", -1))
	listEntries.append((szHeader, -1))
	for x in items:
		listEntries.append(x)

# <!-- custom: shorten vote labels in the left list so they fit in the item bar without
# changing global list width. We strip source prefixes (already shown by section headers),
# then abbreviate Election -> E: and Resolution # -> R#. Full vote names remain visible on
# the vote page itself. (GPT-5.4?) -->
def _SAS_shortenVoteListLabel(szLabel):
	if not szLabel:
		return szLabel
	szOut = szLabel
	for szPrefix in ("U.N. ", "UN ", "Apostolic Palace "):
		if szOut.startswith(szPrefix):
			szOut = szOut[len(szPrefix):]
			break
	if szOut.startswith("Election"):
		szRest = szOut[len("Election"):].strip()
		if szRest:
			return "E: " + szRest
		return "E"
	if szOut.startswith("Resolution #"):
		szRest = szOut[len("Resolution #"):].strip()
		if szRest:
			return "R#" + szRest
		return "R#"
	return szOut

# <!-- custom: sort vote sources from oldest to newest using the hosting building's
# prerequisite AND tech progression (era, then grid X). This places Apostolic Palace
# before United Nations in default XML. Fallback keeps deterministic order by source id.
# (GPT-5.4?) -->
def _SAS_getVoteSourceOldestFirstSortKey(iVoteSource):
	for iBuilding in range(gc.getNumBuildingInfos()):
		bi = gc.getBuildingInfo(iBuilding)
		if bi and bi.getVoteSourceType() == iVoteSource:
			iTech = bi.getPrereqAndTech()
			if iTech > -1:
				ti = gc.getTechInfo(iTech)
				if ti:
					return (ti.getEra(), ti.getGridX(), iVoteSource)
			return (999, 999, iVoteSource)
	return (999, 999, iVoteSource)

# <!-- custom: Sevopedia Votes grouped by vote source. Each vote in current
# CIV4VoteInfo.xml has exactly one source (isVoteSourceType true for one iVoteSource),
# so we attribute each vote to the first matching source. Modders adding multi-source votes
# would need custom duplication/grouping logic. (Claude code Opus 4.7 + GPT-5.4?) -->
def SAS_getVotesGroupedByVoteSource(bSortLists):
	listEntries = []
	iNumVoteSources = gc.getNumVoteSourceInfos()
	iNumVotes = gc.getNumVoteInfos()
	listVoteSources = []
	for iVoteSource in range(iNumVoteSources):
		srcInfo = gc.getVoteSourceInfo(iVoteSource)
		if srcInfo:
			listVoteSources.append((_SAS_getVoteSourceOldestFirstSortKey(iVoteSource), iVoteSource, srcInfo))
	listVoteSources.sort()
	for _, iVoteSource, srcInfo in listVoteSources:
		items = []
		for iVote in range(iNumVotes):
			voteInfo = gc.getVoteInfo(iVote)
			if not voteInfo:
				continue
			if voteInfo.isVoteSourceType(iVoteSource):
				items.append((_SAS_shortenVoteListLabel(voteInfo.getDescription()), iVote))
		if bSortLists:
			items.sort()
		# VoteSourceInfo.getDescription() resolves to the hosting building's name
		# (TXT_KEY_BUILDING_UNITED_NATIONS / TXT_KEY_BUILDING_APOSTOLIC_PALACE).
		_SAS_addSection(listEntries, srcInfo.getDescription(), items)
	return listEntries

# <!-- custom: earliest era at which an event trigger can fire. The trigger's direct
# <OrPreReqs>/<AndPreReqs> tech lists are only one of several era-gating fields — we also
# check <OtherPlayerHasTech>, <CivicPrereq> (single civic), and <UnitsRequired> /
# <BuildingsRequired> / <ReligionsRequired> / <CorporationsRequired>
# whose required assets each imply a tech-era through their own XML prereqs. Without these
# indirect checks, events like Airliner Crash (gated only by <OtherPlayerHasTech>TECH_FLIGHT</>)
# wrongly land in the "Any era" bucket.
#
# Algorithm: OR-techs allow any single member to satisfy the constraint, so the OR list
# contributes min(eras) only if non-empty; true AND constraints contribute their maximum.
# Required building/unit lists are eligibility pools, and multiple copies of the earliest
# eligible class can satisfy their count, so they contribute the minimum. Religion and
# corporation counts require distinct types at player level, so they contribute the Nth
# earliest eligible type; city triggers require N total types plus any one listed type.
# Returns -1 only when no constraint supplies an era gate. (Claude code Opus 4.7; corrected ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
def _SAS_getTechEra(iTech):
	if iTech < 0:
		return -1
	techInfo = gc.getTechInfo(iTech)
	if not techInfo:
		return -1
	return techInfo.getEra()

def _SAS_getBuildingClassEra(iBuildingClass):
	if iBuildingClass < 0:
		return -1
	buildingClassInfo = gc.getBuildingClassInfo(iBuildingClass)
	if not buildingClassInfo:
		return -1
	iBuilding = buildingClassInfo.getDefaultBuildingIndex()
	if iBuilding < 0:
		return -1
	buildingInfo = gc.getBuildingInfo(iBuilding)
	if not buildingInfo:
		return -1
	return _SAS_getTechEra(buildingInfo.getPrereqAndTech())

def _SAS_getUnitClassEra(iUnitClass):
	if iUnitClass < 0:
		return -1
	unitClassInfo = gc.getUnitClassInfo(iUnitClass)
	if not unitClassInfo:
		return -1
	iUnit = unitClassInfo.getDefaultUnitIndex()
	if iUnit < 0:
		return -1
	unitInfo = gc.getUnitInfo(iUnit)
	if not unitInfo:
		return -1
	return _SAS_getTechEra(unitInfo.getPrereqAndTech())

def _SAS_getThresholdEra(aiEras, iRequiredCount):
	# <!-- custom: Return the earliest era in which the requested number of distinct eligible types can exist. Keep -1 entries: a type without a tech prerequisite is available before any era gate and must win a one-of pool instead of letting a later alternative delay it. See KI#225. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if iRequiredCount <= 0 or len(aiEras) <= 0:
		return -1
	aiSortedEras = aiEras[:]
	aiSortedEras.sort()
	return aiSortedEras[iRequiredCount - 1]

def _SAS_getEventTriggerForEvent(iEvent):
	if iEvent < 0:
		return -1
	for iTrigger in range(gc.getNumEventTriggerInfos()):
		info = gc.getEventTriggerInfo(iTrigger)
		if not info:
			continue
		for i in range(info.getNumEvents()):
			if info.getEvent(i) == iEvent:
				return iTrigger
	return -1

def _SAS_getEventTriggerEarliestEraAndSource(iTrigger, seenTriggers=None):
	# <!-- custom: returns (iEra, bDirect). bDirect is True when the trigger declares
	# at least one entry in <OrPreReqs> or <AndPreReqs> — i.e. the era is visible at
	# a glance on the trigger itself. bDirect is False when the era had to be inferred
	# indirectly via chain lookups (OtherPlayerHasTech, prerequisite event chains,
	# required civic/building/unit/religion/corporation). The grouping function uses this to split each era section
	# into "Ancient" vs "Ancient (indirect)" so readers can tell at-a-glance which
	# triggers are obviously era-gated vs which rely on our indirect inference.
	# iEra is -1 only when NO prereq of any kind gives an era — in that case bDirect
	# is irrelevant (caller puts those in the "Any era" bucket). (Claude code Opus 4.7; GPT-5.5 update) -->
	if seenTriggers is None:
		seenTriggers = {}
	if iTrigger in seenTriggers:
		return (-1, False)
	seenTriggers[iTrigger] = True
	info = gc.getEventTriggerInfo(iTrigger)
	if not info:
		return (-1, False)

	# OR tech list: any single tech satisfies the constraint, take min. Counts as direct.
	iOrMin = -1
	for i in range(info.getNumPrereqOrTechs()):
		iEra = _SAS_getTechEra(info.getPrereqOrTechs(i))
		if iEra >= 0:
			if iOrMin == -1 or iEra < iOrMin:
				iOrMin = iEra
	bHasDirectOr = (iOrMin >= 0)

	# AND tech list: all must be satisfied, take max. Counts as direct.
	iAndMax = -1
	for i in range(info.getNumPrereqAndTechs()):
		iEra = _SAS_getTechEra(info.getPrereqAndTechs(i))
		if iEra >= 0 and iEra > iAndMax:
			iAndMax = iEra
	bHasDirectAnd = (iAndMax >= 0)

	# Indirect constraint families are aggregated according to their engine semantics,
	# then combined as AND gates through the maximum. Collected separately so we can
	# classify direct vs indirect.
	iIndirectMax = -1
	def _bumpIndirect(iEra):
		if iEra < 0:
			return
		if iIndirectHolder[0] == -1 or iEra > iIndirectHolder[0]:
			iIndirectHolder[0] = iEra
	iIndirectHolder = [iIndirectMax]

	_bumpIndirect(_SAS_getTechEra(info.getOtherPlayerHasTech()))

	iCivic = info.getCivic()
	if iCivic >= 0:
		civicInfo = gc.getCivicInfo(iCivic)
		if civicInfo:
			_bumpIndirect(_SAS_getTechEra(civicInfo.getTechPrereq()))

	# <!-- custom: The DLL sums copies across listed building classes and picks any one valid unit, so the earliest eligible class can satisfy these gates; maxing every listed alternative delayed triggers such as Experienced Captain until Modern. See KI#225. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if info.getNumBuildings() > 0 or info.getNumBuildingsGlobal() > 0:
		aiBuildingEras = []
		for i in range(info.getNumBuildingsRequired()):
			iBuildingClass = info.getBuildingRequired(i)
			if iBuildingClass >= 0:
				aiBuildingEras.append(_SAS_getBuildingClassEra(iBuildingClass))
		_bumpIndirect(_SAS_getThresholdEra(aiBuildingEras, 1))
	if info.getNumUnits() > 0 or info.getNumUnitsGlobal() > 0:
		aiUnitEras = []
		for i in range(info.getNumUnitsRequired()):
			iUnitClass = info.getUnitRequired(i)
			if iUnitClass >= 0:
				aiUnitEras.append(_SAS_getUnitClassEra(iUnitClass))
		_bumpIndirect(_SAS_getThresholdEra(aiUnitEras, 1))

	# <!-- custom: Player-level religion/corporation thresholds count distinct eligible types. City triggers instead require the total count in one city and, when a list is present, any one listed type; combine those two gates rather than treating every listed type as mandatory. See KI#225. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if info.getNumReligions() > 0:
		aiReligionEras = []
		for i in range(info.getNumReligionsRequired()):
			iReligion = info.getReligionRequired(i)
			if iReligion >= 0:
				religionInfo = gc.getReligionInfo(iReligion)
				if religionInfo:
					aiReligionEras.append(_SAS_getTechEra(religionInfo.getTechPrereq()))
		if info.isPickCity() or len(aiReligionEras) <= 0:
			aiAllReligionEras = []
			for iReligion in range(gc.getNumReligionInfos()):
				religionInfo = gc.getReligionInfo(iReligion)
				if religionInfo:
					aiAllReligionEras.append(_SAS_getTechEra(religionInfo.getTechPrereq()))
			_bumpIndirect(_SAS_getThresholdEra(aiAllReligionEras, info.getNumReligions()))
		if info.isPickCity():
			_bumpIndirect(_SAS_getThresholdEra(aiReligionEras, 1))
		elif len(aiReligionEras) > 0:
			_bumpIndirect(_SAS_getThresholdEra(aiReligionEras, info.getNumReligions()))

	if info.getNumCorporations() > 0:
		# <!-- custom: AdvCiv-SAS corporations use their HQ/founding building's technology gates rather than CvCorporationInfo.TechPrereq.
		# Reuse the existing availability helper so the repaired threshold semantics classify shipped corporation EventTriggers in their reachable era. See KI#1068. (GPT-5.6-Sol) -->
		iNumBuildingAndTechs = gc.getNUM_BUILDING_AND_TECH_PREREQS()
		aiCorporationEras = []
		for i in range(info.getNumCorporationsRequired()):
			iCorporation = info.getCorporationRequired(i)
			if iCorporation >= 0:
				iCorporationEra = SAS_getCorporationAvailabilityEra(iCorporation, iNumBuildingAndTechs)
				if iCorporationEra is not None:
					aiCorporationEras.append(iCorporationEra)
		if info.isPickCity() or len(aiCorporationEras) <= 0:
			aiAllCorporationEras = []
			for iCorporation in range(gc.getNumCorporationInfos()):
				iCorporationEra = SAS_getCorporationAvailabilityEra(iCorporation, iNumBuildingAndTechs)
				if iCorporationEra is not None:
					aiAllCorporationEras.append(iCorporationEra)
			_bumpIndirect(_SAS_getThresholdEra(aiAllCorporationEras, info.getNumCorporations()))
		if info.isPickCity():
			_bumpIndirect(_SAS_getThresholdEra(aiCorporationEras, 1))
		elif len(aiCorporationEras) > 0:
			_bumpIndirect(_SAS_getThresholdEra(aiCorporationEras, info.getNumCorporations()))
	# <!-- custom: prerequisite event chains inherit the prerequisite trigger's inferred era.
	# This stays indirect because the current trigger does not declare the tech itself; it
	# depends on a prior event outcome that may have its own tech or chain gate. (GPT-5.5) -->
	for i in range(info.getNumPrereqEvents()):
		iPrereqEvent = info.getPrereqEvent(i)
		iPrereqTrigger = _SAS_getEventTriggerForEvent(iPrereqEvent)
		if iPrereqTrigger >= 0:
			iPrereqEra = _SAS_getEventTriggerEarliestEraAndSource(iPrereqTrigger, seenTriggers.copy())[0]
			_bumpIndirect(iPrereqEra)
	# Routes use TechMovementChanges (a list, not a single prereq) so there's no clean
	# single-era answer — skipped. Route-required triggers are rare enough that the
	# other constraints on the same trigger usually give a correct era anyway.

	iIndirectMax = iIndirectHolder[0]

	# Direct era contribution = max(OR min, AND max). Indirect era contribution is what
	# we just computed. Final era = max of both. Classification: direct if the trigger
	# has ANY direct prereq at all (OR or AND list non-empty). Rationale: users see
	# "ancient (indirect)" as "we had to dig into indirect refs to place this", whereas
	# "ancient" means "the trigger XML says so up-front".
	iDirectEra = -1
	if iOrMin >= 0:
		iDirectEra = iOrMin
	if iAndMax >= 0 and iAndMax > iDirectEra:
		iDirectEra = iAndMax

	if iDirectEra == -1 and iIndirectMax == -1:
		return (-1, False)
	if iDirectEra == -1:
		return (iIndirectMax, False)
	if iIndirectMax == -1:
		return (iDirectEra, True)
	# Both contributed; final era = max.
	if iDirectEra >= iIndirectMax:
		return (iDirectEra, True)
	return (iIndirectMax, bHasDirectOr or bHasDirectAnd)

def _SAS_getEventTriggerRowLabel(iTrigger):
	info = gc.getEventTriggerInfo(iTrigger)
	if not info:
		return "Trigger %d" % iTrigger
	szLabel = info.getDescription()
	if (not szLabel) or len(szLabel.strip()) == 0:
		szType = info.getType()
		if szType and szType.startswith("EVENTTRIGGER_"):
			szType = szType[len("EVENTTRIGGER_"):]
		if szType:
			szLabel = szType.replace("_", " ").title()
		else:
			szLabel = "Trigger %d" % iTrigger
	return szLabel

# <!-- custom: Sevopedia Event Triggers grouped by the earliest era at which they can fire.
# - "Any era (no tech requirement)" bucket is placed FIRST because these triggers are
#   active from turn 1 — putting them first matches the player's "when does this fire?"
#   mental model better than appending them at the end.
# - Each real era produces TWO buckets: "Ancient" (triggers whose tech prereq is visible
#   directly in <OrPreReqs> / <AndPreReqs>) and "Ancient (indirect)" (triggers whose era
#   was inferred from <OtherPlayerHasTech>, a prerequisite event chain, or a required
#   civic/building/unit/religion/corporation's own tech prereq). The indirect bucket exists so readers can
#   tell at-a-glance which placements are "the XML says so" vs "our chain-lookup says so".
# - Within each section, triggers are emitted in XML declaration order. This preserves
#   the modder's intended grouping (families like FESTIVAL / FESTIVAL_AGAIN / FESTIVAL_DONE
#   are typically declared adjacently in CIV4EventTriggerInfos.xml, so they stay adjacent
#   in the list for free). If a family is ever scattered in the XML, fix at XML level.
# - bSortLists is honored: when the user enables Sort Lists, entries within each section
#   sort alphabetically by label instead of XML order (same convention as other pedia
#   categories in this module). (Claude code Opus 4.7; GPT-5.5 update) -->
def SAS_getEventTriggersGroupedByEra(bSortLists):
	listEntries = []
	iNumEras = gc.getNumEraInfos()
	# directGroups[iEra] / indirectGroups[iEra] hold row tuples. -1 goes in the shared
	# "any era" bucket (no tech requirement of any kind — direct/indirect distinction
	# isn't meaningful there).
	anyEraItems = []
	directGroups = {}
	indirectGroups = {}
	iNumTriggers = gc.getNumEventTriggerInfos()
	for iTrigger in range(iNumTriggers):
		info = gc.getEventTriggerInfo(iTrigger)
		if not info:
			continue
		iEra, bDirect = _SAS_getEventTriggerEarliestEraAndSource(iTrigger)
		row = (_SAS_getEventTriggerRowLabel(iTrigger), iTrigger)
		if iEra < 0:
			anyEraItems.append(row)
		elif bDirect:
			if iEra not in directGroups:
				directGroups[iEra] = []
			directGroups[iEra].append(row)
		else:
			if iEra not in indirectGroups:
				indirectGroups[iEra] = []
			indirectGroups[iEra].append(row)

	if anyEraItems:
		if bSortLists:
			anyEraItems = sorted(anyEraItems)
		_SAS_addSection(listEntries, localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_GROUP_NO_TECH", ()), anyEraItems)

	szIndirectSuffix = localText.getText("TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_GROUP_INDIRECT_SUFFIX", ())
	for iEra in range(iNumEras):
		szEraName = gc.getEraInfo(iEra).getDescription()
		if iEra in directGroups:
			items = directGroups[iEra]
			if bSortLists:
				items = sorted(items)
			_SAS_addSection(listEntries, szEraName, items)
		if iEra in indirectGroups:
			items = indirectGroups[iEra]
			if bSortLists:
				items = sorted(items)
			_SAS_addSection(listEntries, szEraName + u" " + szIndirectSuffix, items)

	return listEntries

def _SAS_appendSoundLabel(szLabel, szSoundScript, iSoundId):
	if szSoundScript:
		return szLabel + " - " + szSoundScript
	if iSoundId != -1:
		return szLabel + " - Sound ID %d" % iSoundId
	return szLabel

def SAS_getMoviesListGroupedByType(bSortLists, packMovieKey, unpackMovieKey, iTypeVictory, iTypeWonder, iTypeProject, iTypeReligion, iTypeEra, iTypeCorporation):
	# Return the Movies left-list entries with section headers (Victory/Wonder/Project/Religion/Era/Corporation).
	# Implementation detail: we build a single flat base list first, then split into sections.
	#
	baseList = []

	# Victory movies
	for iVictory in range(gc.getNumVictoryInfos()):
		info = gc.getVictoryInfo(iVictory)
		if not info:
			continue
		szMovie = ""
		try:
			szMovie = info.getMovie()
		except:
			szMovie = ""
		if szMovie:
			baseList.append((info.getDescription(), packMovieKey(iTypeVictory, iVictory)))

	# Wonder movies (buildings with a movie)
	for iBuilding in range(gc.getNumBuildingInfos()):
		info = gc.getBuildingInfo(iBuilding)
		if (not info) or info.isGraphicalOnly():
			continue
		szMovie = ""
		try:
			szMovie = info.getMovie()
		except:
			szMovie = ""
		if szMovie:
			baseList.append((info.getDescription(), packMovieKey(iTypeWonder, iBuilding)))

	# Project movies
	for iProject in range(gc.getNumProjectInfos()):
		info = gc.getProjectInfo(iProject)
		if (not info) or info.isGraphicalOnly():
			continue
		szMovie = ""
		try:
			szMovie = info.getMovieArtDef()
		except:
			szMovie = ""
		if szMovie:
			baseList.append((info.getDescription(), packMovieKey(iTypeProject, iProject)))

	# Religion movies
	for iReligion in range(gc.getNumReligionInfos()):
		info = gc.getReligionInfo(iReligion)
		if (not info) or info.isGraphicalOnly():
			continue
		szMovie = ""
		try:
			szMovie = info.getMovieFile()
		except:
			szMovie = ""
		if szMovie:
			baseList.append((info.getDescription(), packMovieKey(iTypeReligion, iReligion)))

	# Era movies (list all eras, even if no movie file exists; the page will hide the Play button)
	for iEra in range(gc.getNumEraInfos()):
		info = gc.getEraInfo(iEra)
		if not info:
			continue
		szEraName = info.getDescription() + " " + localText.getText("TXT_KEY_PEDIA_ERA", ())
		baseList.append((szEraName, packMovieKey(iTypeEra, iEra)))

	# Corporation movies
	for iCorporation in range(gc.getNumCorporationInfos()):
		info = gc.getCorporationInfo(iCorporation)
		if (not info) or info.isGraphicalOnly():
			continue
		szMovie = ""
		try:
			szMovie = info.getMovieFile()
		except:
			szMovie = ""
		if szMovie and szMovie != "NONE":
			baseList.append((info.getDescription(), packMovieKey(iTypeCorporation, iCorporation)))

	if bSortLists:
		baseList.sort()

	# Split into groups (preserves alphabetical ordering inside each group because baseList is already ordered)
	victoryItems = []
	wonderItems = []
	projectItems = []
	religionItems = []
	eraItems = []
	corporationItems = []

	for (szName, iPacked) in baseList:
		iType, _ = unpackMovieKey(iPacked)
		if iType == iTypeVictory:
			victoryItems.append((szName, iPacked))
		elif iType == iTypeWonder:
			wonderItems.append((szName, iPacked))
		elif iType == iTypeProject:
			projectItems.append((szName, iPacked))
		elif iType == iTypeReligion:
			religionItems.append((szName, iPacked))
		elif iType == iTypeEra:
			eraItems.append((szName, iPacked))
		elif iType == iTypeCorporation:
			corporationItems.append((szName, iPacked))

	# <!-- custom: Sort eraItems by era index (chronological order) regardless of bSortLists
	# (Era order should always be Ancient -> Classical -> Medieval -> etc.) (Claude code Sonnet 4.5) -->
	if eraItems:
		eraItems.sort(key=lambda x: unpackMovieKey(x[1])[1])

	listEntries = []
	_SAS_addSection(listEntries, localText.getText("TXT_KEY_PEDIA_SAS_MOVIES_HEADER_VICTORY", ()), victoryItems)
	_SAS_addSection(listEntries, localText.getText("TXT_KEY_PEDIA_SAS_MOVIES_HEADER_WONDER", ()), wonderItems)
	_SAS_addSection(listEntries, localText.getText("TXT_KEY_PEDIA_SAS_MOVIES_HEADER_PROJECT", ()), projectItems)
	_SAS_addSection(listEntries, localText.getText("TXT_KEY_PEDIA_SAS_MOVIES_HEADER_RELIGION", ()), religionItems)
	_SAS_addSection(listEntries, localText.getText("TXT_KEY_PEDIA_SAS_MOVIES_HEADER_ERA", ()), eraItems)
	_SAS_addSection(listEntries, localText.getText("TXT_KEY_PEDIA_SAS_MOVIES_HEADER_CORPORATION", ()), corporationItems)
	return listEntries

def _SAS_extractTagValue(line, tagName):
	# Very lightweight tag extraction (kept compatible with the existing "one-line tag" script xml style).
	openTag = "<" + tagName + ">"
	closeTag = "</" + tagName + ">"
	start = line.find(openTag)
	if start == -1:
		return ""
	start += len(openTag)
	end = line.find(closeTag, start)
	if end == -1:
		return ""
	return line[start:end].strip()

def SAS_getMusicListAndTables(bSortLists, packMusicKey, unpackMusicKey, iTypeTech, iTypeEra, iTypeLeader, iTypeCiv, iTypeScript, iTypeScript3D, bLeaderIntroPeaceFirstOnly, bLeaderPeaceFirstOnly, bLeaderIntroWarFirstLeaderOnly, bLeaderWarFirstLeaderOnly):
	# Return:
	#   (listEntries, musicEraTracks, musicLeaderTracks, musicCivTracks, musicScriptTracks, musicScript3DTracks, firstCivScript3DKey)
	#
	# These tables are required by SevoPediaMusic for Play button behavior and for showing Track IDs.
	#
	listEntries = []
	musicEraTracks = []
	musicLeaderTracks = []
	musicCivTracks = []
	musicScriptTracks = []
	musicScript3DTracks = []

	# Tech quote music (grouped by Era)
	techBase = []  # [(szTechName, packed, iEra), ...]
	for iTech in range(gc.getNumTechInfos()):
		info = gc.getTechInfo(iTech)
		if not info:
			continue
		szSound = ""
		try:
			szSound = info.getSound()
		except:
			szSound = ""
		if not szSound or szSound == "NONE":
			continue
		iEra = info.getEra()
		szLabel = _SAS_appendSoundLabel(info.getDescription(), szSound, -1)
		techBase.append((szLabel, packMusicKey(iTypeTech, iTech), iEra))

	if bSortLists:
		techBase.sort()

	techByEra = {}
	for (szName, iPacked, iEra) in techBase:
		tmp = techByEra.get(iEra, None)
		if tmp is None:
			tmp = []
			techByEra[iEra] = tmp
		tmp.append((szName, iPacked))

	for iEra in range(gc.getNumEraInfos()):
		info = gc.getEraInfo(iEra)
		if not info:
			continue
		items = techByEra.get(iEra, [])
		if not items:
			continue
		szEraName = info.getDescription()
		if szEraName.endswith(" Era"):
			szEraName = szEraName[:-len(" Era")]
		szQuotesHeader = localText.getText("TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_TECH_QUOTES", ())
		_SAS_addSection(listEntries, szQuotesHeader + " (" + szEraName + ")", items)

	# Era soundtrack tracks (grouped by Era)
	for iEra in range(gc.getNumEraInfos()):
		info = gc.getEraInfo(iEra)
		if not info:
			continue
		eraItems = []
		numTracks = 0
		try:
			numTracks = info.getNumSoundtracks()
		except:
			numTracks = 0
		for iTrack in range(numTracks):
			iTrackId = -1
			try:
				iTrackId = info.getSoundtracks(iTrack)
			except:
				iTrackId = -1
			if iTrackId == -1:
				continue

			iPackedTrack = len(musicEraTracks)
			musicEraTracks.append((iEra, iTrackId, iTrack))

			szTrackName = ""
			try:
				szTrackName = info.getSoundtrackScriptName(iTrack)
			except:
				szTrackName = ""
			szLabel = "Track %02d" % (iTrack + 1)
			if szTrackName:
				szLabel = szLabel + " - " + szTrackName
			eraItems.append((szLabel, packMusicKey(iTypeEra, iPackedTrack)))

		if bSortLists:
			eraItems.sort()

		szEraName = info.getDescription() + " " + localText.getText("TXT_KEY_PEDIA_ERA", ())
		_SAS_addSection(listEntries, szEraName, eraItems)

	# Leader music (intro/peace/war buckets)
	leaderIntroPeaceItems = []
	leaderPeaceItems = []
	leaderIntroWarItems = []
	leaderWarItems = []

	iWarIntroLeaderChosen = -1
	iWarLeaderChosen = -1

	for iLeader in range(gc.getNumLeaderHeadInfos()):
		leaderInfo = gc.getLeaderHeadInfo(iLeader)
		if not leaderInfo:
			continue

		szLeaderName = leaderInfo.getDescription()

		# Peace
		bAddedIntroPeace = False
		bAddedPeace = False
		for iEra in range(gc.getNumEraInfos()):
			eraInfo = gc.getEraInfo(iEra)
			szEraName = ""
			if eraInfo:
				szEraName = eraInfo.getDescription()
				if szEraName.endswith(" Era"):
					szEraName = szEraName[:-len(" Era")]

			iPeaceIntroId = -1
			iPeaceId = -1
			try:
				iPeaceIntroId = leaderInfo.getDiploPeaceIntroMusicScriptIds(iEra)
			except:
				iPeaceIntroId = -1
			try:
				iPeaceId = leaderInfo.getDiploPeaceMusicScriptIds(iEra)
			except:
				iPeaceId = -1

			if iPeaceIntroId != -1:
				szLabel = szLeaderName
				if szEraName:
					szLabel += " (" + szEraName + ")"
				if (not bLeaderIntroPeaceFirstOnly) or (not bAddedIntroPeace):
					szLabel = _SAS_appendSoundLabel(szLabel, "", iPeaceIntroId)
					iTrackId = len(musicLeaderTracks)
					musicLeaderTracks.append((iLeader, iEra, "Peace Intro", iPeaceIntroId, szLabel))
					leaderIntroPeaceItems.append((szLabel, packMusicKey(iTypeLeader, iTrackId)))
					bAddedIntroPeace = True

			if iPeaceId != -1:
				szLabel = szLeaderName
				if szEraName:
					szLabel += " (" + szEraName + ")"
				if (not bLeaderPeaceFirstOnly) or (not bAddedPeace):
					szLabel = _SAS_appendSoundLabel(szLabel, "", iPeaceId)
					iTrackId = len(musicLeaderTracks)
					musicLeaderTracks.append((iLeader, iEra, "Peace", iPeaceId, szLabel))
					leaderPeaceItems.append((szLabel, packMusicKey(iTypeLeader, iTrackId)))
					bAddedPeace = True

		# War
		for iEra in range(gc.getNumEraInfos()):
			eraInfo = gc.getEraInfo(iEra)
			szEraName = ""
			if eraInfo:
				szEraName = eraInfo.getDescription()
				if szEraName.endswith(" Era"):
					szEraName = szEraName[:-len(" Era")]

			iWarIntroId = -1
			iWarId = -1
			try:
				iWarIntroId = leaderInfo.getDiploWarIntroMusicScriptIds(iEra)
			except:
				iWarIntroId = -1
			try:
				iWarId = leaderInfo.getDiploWarMusicScriptIds(iEra)
			except:
				iWarId = -1

			if iWarIntroId != -1:
				if bLeaderIntroWarFirstLeaderOnly:
					if iWarIntroLeaderChosen == -1:
						iWarIntroLeaderChosen = iLeader
					elif iWarIntroLeaderChosen != iLeader:
						continue
				szLabel = szLeaderName
				if szEraName:
					szLabel += " (" + szEraName + ")"
				szLabel = _SAS_appendSoundLabel(szLabel, "", iWarIntroId)
				iTrackId = len(musicLeaderTracks)
				musicLeaderTracks.append((iLeader, iEra, "War Intro", iWarIntroId, szLabel))
				leaderIntroWarItems.append((szLabel, packMusicKey(iTypeLeader, iTrackId)))

			if iWarId != -1:
				if bLeaderWarFirstLeaderOnly:
					if iWarLeaderChosen == -1:
						iWarLeaderChosen = iLeader
					elif iWarLeaderChosen != iLeader:
						continue
				szLabel = szLeaderName
				if szEraName:
					szLabel += " (" + szEraName + ")"
				szLabel = _SAS_appendSoundLabel(szLabel, "", iWarId)
				iTrackId = len(musicLeaderTracks)
				musicLeaderTracks.append((iLeader, iEra, "War", iWarId, szLabel))
				leaderWarItems.append((szLabel, packMusicKey(iTypeLeader, iTrackId)))

	_SAS_addSection(listEntries, localText.getText("TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_LEADERS_INTRO_PEACE", ()), leaderIntroPeaceItems)
	_SAS_addSection(listEntries, localText.getText("TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_LEADERS_PEACE", ()), leaderPeaceItems)
	_SAS_addSection(listEntries, localText.getText("TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_LEADERS_INTRO_WAR", ()), leaderIntroWarItems)
	_SAS_addSection(listEntries, localText.getText("TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_LEADERS_WAR", ()), leaderWarItems)

	# <!-- custom: Civilizations (Selection/Action sounds from CIV4CivilizationInfos.xml) (GPT-5.2-Codex) -->
	civItems = []
	szSelectLabel = localText.getText("TXT_KEY_PEDIA_SAS_MUSIC_CIV_SELECT", ())
	szOrderLabel = localText.getText("TXT_KEY_PEDIA_SAS_MUSIC_CIV_ORDER", ())
	for iCiv in range(gc.getNumCivilizationInfos()):
		civInfo = gc.getCivilizationInfo(iCiv)
		if not civInfo or civInfo.isGraphicalOnly():
			continue
		szCivName = civInfo.getDescription()
		selVal = civInfo.getSelectionSoundScriptId()
		if selVal != -1 and selVal != "" and selVal != "NONE":
			iSoundId = -1
			szScript = ""
			try:
				if isinstance(selVal, (int, long)):
					iSoundId = selVal
				else:
					szScript = selVal
			except:
				try:
					if isinstance(selVal, int):
						iSoundId = selVal
					else:
						szScript = selVal
				except:
					szScript = selVal
			iTrackId = len(musicCivTracks)
			szLabel = _SAS_appendSoundLabel(szCivName + " (" + szSelectLabel + ")", szScript, iSoundId)
			bIs3D = True
			if szScript:
				bIs3D = szScript.startswith("AS3D_")
			musicCivTracks.append((iCiv, iSoundId, szScript, szLabel, bIs3D))
			civItems.append((szLabel, packMusicKey(iTypeCiv, iTrackId)))

		actVal = civInfo.getActionSoundScriptId()
		if actVal != -1 and actVal != "" and actVal != "NONE":
			iSoundId = -1
			szScript = ""
			try:
				if isinstance(actVal, (int, long)):
					iSoundId = actVal
				else:
					szScript = actVal
			except:
				try:
					if isinstance(actVal, int):
						iSoundId = actVal
					else:
						szScript = actVal
				except:
					szScript = actVal
			iTrackId = len(musicCivTracks)
			szLabel = _SAS_appendSoundLabel(szCivName + " (" + szOrderLabel + ")", szScript, iSoundId)
			bIs3D = True
			if szScript:
				bIs3D = szScript.startswith("AS3D_")
			musicCivTracks.append((iCiv, iSoundId, szScript, szLabel, bIs3D))
			civItems.append((szLabel, packMusicKey(iTypeCiv, iTrackId)))

	_SAS_addSection(listEntries, localText.getText("TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_CIVILIZATIONS", ()), civItems)

	# Sound scripts (2D) - keep grouping/labels identical to the original SevoPediaMain implementation.
	# <!-- custom: use TXT_KEYs for section groupings (GPT-5.2-Codex) -->
	_SCRIPT_2D_OPENING = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_OPENING_2D"
	_SCRIPT_2D_SONGS = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_SONGS_2D"
	_SCRIPT_2D_DIPLO = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_DIPLO_2D"
	_SCRIPT_2D_TECH = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_TECH_2D"
	_SCRIPT_2D_TUTORIAL = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_TUTORIAL_2D"
	_SCRIPT_2D_BUILDS = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_BUILDS_2D"
	_SCRIPT_2D_UNITS = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_UNITS_2D"
	_SCRIPT_2D_INTERFACE = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_INTERFACE_2D"
	_SCRIPT_2D_AMBIENT = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_AMBIENT_2D"
	_SCRIPT_2D_GOODY = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_GOODY_2D"
	_SCRIPT_2D_EVENTS = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_EVENTS_2D"
	_SCRIPT_2D_SFX = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_SFX_2D"
	_SCRIPT_2D_OTHER = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_OTHER_2D"
	scriptGroups = {
		_SCRIPT_2D_OPENING: [],
		_SCRIPT_2D_SONGS: [],
		_SCRIPT_2D_DIPLO: [],
		_SCRIPT_2D_TECH: [],
		_SCRIPT_2D_TUTORIAL: [],
		_SCRIPT_2D_BUILDS: [],
		_SCRIPT_2D_UNITS: [],
		_SCRIPT_2D_INTERFACE: [],
		_SCRIPT_2D_AMBIENT: [],
		_SCRIPT_2D_GOODY: [],
		_SCRIPT_2D_EVENTS: [],
		_SCRIPT_2D_SFX: [],
		_SCRIPT_2D_OTHER: [],
	}

	szScript = None
	szSound = None
	labelSeenCounts = {}
	iOpeningCount = 0
	iSongCount = 0
	szAudio2DPath = _SAS_findAssetXmlPath("Audio2DScripts.xml", os.path.join("XML", "Audio"))
	try:
		f = open(szAudio2DPath, "r")
		for line in f:
			if "<ScriptID>" in line:
				szScript = _SAS_extractTagValue(line, "ScriptID")
			elif "<SoundID>" in line:
				szSound = _SAS_extractTagValue(line, "SoundID")
			elif "</Script2DSound>" in line:
				if szScript and szSound:
					# <!-- custom: classify opening tracks by either SoundID (SONG_OPENING*) or ScriptID (AS2D_OPENING_MENU*), so custom opening variants still appear in the Opening grouping even when SoundID naming differs. (GPT-5.3-Codex) -->
					if szSound.startswith("SONG_OPENING") or szScript.startswith("AS2D_OPENING_MENU"):
						szGroup = _SCRIPT_2D_OPENING
						iOpeningCount += 1
					elif szSound.startswith("SONG_"):
						szGroup = _SCRIPT_2D_SONGS
						iSongCount += 1
					elif szSound.startswith("DIPLO_") or szScript.startswith("AS2D_DIPLO_"):
						szGroup = _SCRIPT_2D_DIPLO
					elif szSound.startswith("SND_TECH"):
						szGroup = _SCRIPT_2D_TECH
					elif szScript.startswith("AS2D_TUTORIAL"):
						szGroup = _SCRIPT_2D_TUTORIAL
					elif szScript.startswith("AS2D_BUILD"):
						szGroup = _SCRIPT_2D_BUILDS
					elif szScript.startswith("AS2D_UNIT"):
						szGroup = _SCRIPT_2D_UNITS
					elif szScript.startswith("AS2D_IF"):
						szGroup = _SCRIPT_2D_INTERFACE
					elif (szSound.startswith("SND_AMB") or szSound.startswith("SND_OCEAN") or szSound.startswith("SND_CROWD") or szSound.startswith("SND_MARCH") or szSound.startswith("SND_TOWNMUSIC")):
						szGroup = _SCRIPT_2D_AMBIENT
					elif szSound.startswith("SND_GOODY"):
						szGroup = _SCRIPT_2D_GOODY
					elif (szSound.startswith("SND_VICTORY") or szSound.startswith("SND_LOSS") or szSound.startswith("SND_WONDER") or szSound.startswith("SND_CITY") or szSound.startswith("SND_CULTURE") or szSound.startswith("SND_NUKE") or szSound.startswith("SND_GOLDAGEEND") or szSound.startswith("SND_MELTDOWN") or szSound.startswith("SND_ALARM") or szSound.startswith("SND_CONTACT")):
						szGroup = _SCRIPT_2D_EVENTS
					elif szSound.startswith("SND_"):
						szGroup = _SCRIPT_2D_SFX
					else:
						szGroup = _SCRIPT_2D_OTHER

					szLabel = szSound

					groupCounts = labelSeenCounts.get(szGroup)
					if groupCounts is None:
						groupCounts = {}
						labelSeenCounts[szGroup] = groupCounts
					szBaseLabel = szLabel
					iLabelCount = groupCounts.get(szBaseLabel, 0)
					if iLabelCount > 0:
						szLabel = szLabel + " (" + szScript + ")"
					groupCounts[szBaseLabel] = iLabelCount + 1

					iTrackId = len(musicScriptTracks)
					musicScriptTracks.append((szScript, szSound, szLabel))
					scriptGroups[szGroup].append((szLabel, packMusicKey(iTypeScript, iTrackId)))

				szScript = None
				szSound = None
		f.close()
		if SAS_SEVO_MUSIC_DEBUG_ENABLE:
			print("SAS_SEVO_MUSIC_2D_PATH=%s OPENING=%d SONGS=%d") % (szAudio2DPath, iOpeningCount, iSongCount)
	except:
		if SAS_SEVO_MUSIC_DEBUG_ENABLE:
			print("SAS_SEVO_MUSIC_2D_PARSE_FAILED path=%s") % (szAudio2DPath,)
		scriptGroups = {}

	for szHeader in (_SCRIPT_2D_OPENING, _SCRIPT_2D_SONGS, _SCRIPT_2D_DIPLO, _SCRIPT_2D_TECH, _SCRIPT_2D_TUTORIAL, _SCRIPT_2D_BUILDS, _SCRIPT_2D_UNITS, _SCRIPT_2D_INTERFACE, _SCRIPT_2D_AMBIENT, _SCRIPT_2D_GOODY, _SCRIPT_2D_EVENTS, _SCRIPT_2D_SFX, _SCRIPT_2D_OTHER):
		items = scriptGroups.get(szHeader, [])
		if bSortLists and szHeader != _SCRIPT_2D_OTHER:
			items.sort()
		szDisplayHeader = localText.getText(szHeader, ())
		_SAS_addSection(listEntries, szDisplayHeader, items)

	# Sound scripts (3D) - keep grouping/labels identical to the original SevoPediaMain implementation.
	# <!-- custom: use TXT_KEYs for section groupings (GPT-5.2-Codex) -->
	_SCRIPT_3D_UNITS = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_UNITS_3D"
	_SCRIPT_3D_AMBIENCE = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_AMBIENCE_3D"
	_SCRIPT_3D_IMPROVEMENTS = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_IMPROVEMENTS_3D"
	_SCRIPT_3D_CIVILIZATIONS = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_CIVILIZATIONS_3D"
	_SCRIPT_3D_OTHER = "TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_SCRIPTS_OTHER_3D"
	script3DGroups = {
		_SCRIPT_3D_UNITS: [],
		_SCRIPT_3D_AMBIENCE: [],
		_SCRIPT_3D_IMPROVEMENTS: [],
		_SCRIPT_3D_CIVILIZATIONS: [],
		_SCRIPT_3D_OTHER: [],
	}

	szScript3D = None
	szSound3D = None
	labelSeenCounts3D = {}
	szAudio3DPath = _SAS_findAssetXmlPath("Audio3DScripts.xml", os.path.join("XML", "Audio"))
	try:
		f3 = open(szAudio3DPath, "r")
		for line in f3:
			if "<ScriptID>" in line:
				szScript3D = _SAS_extractTagValue(line, "ScriptID")
			elif "<SoundID>" in line:
				szSound3D = _SAS_extractTagValue(line, "SoundID")
			elif "</Script3DSound>" in line:
				if szScript3D and szSound3D:
					if szScript3D.startswith("AS3D_UN_"):
						szGroup3D = _SCRIPT_3D_UNITS
					elif szScript3D.startswith("AS3D_SS_"):
						szGroup3D = _SCRIPT_3D_AMBIENCE
					elif szScript3D.startswith("AS3D_IMPROV"):
						szGroup3D = _SCRIPT_3D_IMPROVEMENTS
					elif szScript3D.endswith("_SELECT") or szScript3D.endswith("_ORDER"):
						szGroup3D = _SCRIPT_3D_CIVILIZATIONS
					else:
						szGroup3D = _SCRIPT_3D_OTHER

					szLabel3D = szSound3D

					groupCounts3D = labelSeenCounts3D.get(szGroup3D)
					if groupCounts3D is None:
						groupCounts3D = {}
						labelSeenCounts3D[szGroup3D] = groupCounts3D
					szBaseLabel3D = szLabel3D
					iLabelCount3D = groupCounts3D.get(szBaseLabel3D, 0)
					if iLabelCount3D > 0:
						szLabel3D = szLabel3D + " (" + szScript3D + ")"
					groupCounts3D[szBaseLabel3D] = iLabelCount3D + 1

					iTrackId3D = len(musicScript3DTracks)
					musicScript3DTracks.append((szScript3D, szSound3D, szLabel3D))
					script3DGroups[szGroup3D].append((szLabel3D, packMusicKey(iTypeScript3D, iTrackId3D)))

				szScript3D = None
				szSound3D = None
		f3.close()
		if SAS_SEVO_MUSIC_DEBUG_ENABLE:
			print("SAS_SEVO_MUSIC_3D_PATH=%s") % (szAudio3DPath,)
	except:
		if SAS_SEVO_MUSIC_DEBUG_ENABLE:
			print("SAS_SEVO_MUSIC_3D_PARSE_FAILED path=%s") % (szAudio3DPath,)
		script3DGroups = {}

	firstCivScript3DKey = -1
	for szHeader3D in (_SCRIPT_3D_UNITS, _SCRIPT_3D_AMBIENCE, _SCRIPT_3D_IMPROVEMENTS, _SCRIPT_3D_CIVILIZATIONS, _SCRIPT_3D_OTHER):
		items3D = script3DGroups.get(szHeader3D, [])
		if bSortLists and szHeader3D != _SCRIPT_3D_OTHER:
			items3D.sort()
		if (szHeader3D == _SCRIPT_3D_CIVILIZATIONS) and items3D and (firstCivScript3DKey == -1):
			firstCivScript3DKey = items3D[0][1]
		szDisplayHeader3D = localText.getText(szHeader3D, ())
		_SAS_addSection(listEntries, szDisplayHeader3D, items3D)

	return (listEntries, musicEraTracks, musicLeaderTracks, musicCivTracks, musicScriptTracks, musicScript3DTracks, firstCivScript3DKey)
