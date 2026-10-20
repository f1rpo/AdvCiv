## GPUtil
##
## Utilities for dealing with Great People.
##
## MODDERS
##
##   There are four places where you must add information about your new great people.
##   This is also necessary if you assign GP points to buildings that don't normally get them,
##   for example GG points to Heroic Epic.
##
##     1. Unit Type
##     2. Named constant
##     3. Color
##     4. Icon (font glyph or string)
##
## Notes
##   - Must be initialized externally by calling init()
##
## Copyright (c) 2007-2009 The BUG Mod.
##
## Author: EmperorFool
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: AdvCiv-SAS keeps GP gameplay/progress behavior here while the complete live GP unit/icon/color inventory is centralized in SASVisualUtils.GreatPeople. (ChatGPT-5.6-Sol) -->

from CvPythonExtensions import *
import BugUtil
import SASVisualUtils as SASVisuals
import PlayerUtil
import SASTextScale # <!-- custom: Measure Maximum-types candidates in the same configurable Label font used by both GP-bar callers. See KI#1066. (GPT-5.6-Sol) -->

gc = CyGlobalContext()

# <!-- custom: Canonical GP unit type names/order now live in SASVisualUtils; this alias preserves GPUtil's inherited bar/progress API. (ChatGPT-5.6-Sol) -->
g_gpBarList = SASVisuals.GREAT_PERSON_UNIT_TYPES

# Named constants for each great person and total number of GP types
# These must be in the exact same order as the list above
NUM_GP = len(g_gpBarList)
# MOD: define a constant for each new great person in same order as above (2) - e.g. add GP_DOCTOR to this unpack
(GP_GREAT_SPY, GP_GREAT_ENGINEER, GP_GREAT_MERCHANT, GP_GREAT_SCIENTIST, GP_GREAT_ARTIST, GP_GREAT_PROPHET, GP_GREAT_GENERAL) = range(NUM_GP)

# Information

def init():
	# <!-- custom: SASVisualUtils.initGameResources resolves these once before GPUtil.init; touch the tuples here too so the inherited BUG init contract remains fail-loud if initialization order changes. (ChatGPT-5.6-Sol) -->
	SASVisuals.GreatPeople.unitTypes()
	SASVisuals.GreatPeople.unitColors()


def getUnitType(gpType):
	return SASVisuals.GreatPeople.unitTypes()[gpType]


def getColor(gpType):
	return SASVisuals.GreatPeople.unitColors()[gpType]


def getUnitIcon(iUnit):
	if not SASVisuals.GreatPeople.hasUnitIcon(iUnit):
		BugUtil.warn("no GP icon for unit %d", iUnit)
	return SASVisuals.GreatPeople.getUnitIcon(iUnit)

# Getting Progress

def getDisplayCity():
	# Returns the city to display in the progress bar.
	#
	pHeadSelectedCity = CyInterface().getHeadSelectedCity()
	if (pHeadSelectedCity and pHeadSelectedCity.getTeam() == gc.getGame().getActiveTeam()):
		city = pHeadSelectedCity
		iTurns = getCityTurns(city)
	else:
		city, iTurns = findNextCity()
		if not city:
			city, iGPP = findMaxCity()
			iTurns = None
	return (city, iTurns)

def findNextCity():
	iMinTurns = None
	iTurns = 0
	player = gc.getPlayer(gc.getGame().getActivePlayer())
	iThreshold = player.greatPeopleThreshold(False)
	bestCity = None
	for city in PlayerUtil.playerCities(player):
		iRate = city.getGreatPeopleRate()
		if (iRate > 0):
			iProgress = city.getGreatPeopleProgress()
			iTurns = (iThreshold - iProgress + iRate - 1) / iRate
			if (iMinTurns is None or iTurns < iMinTurns):
				iMinTurns = iTurns
				bestCity = city
	return (bestCity, iMinTurns)

def findMaxCity():
	iMaxProgress = 0
	player = gc.getPlayer(gc.getGame().getActivePlayer())
	bestCity = None
	for city in PlayerUtil.playerCities(player):
		iProgress = city.getGreatPeopleProgress()
		if (iProgress > iMaxProgress):
			iMaxProgress = iProgress
			bestCity = city
	return (bestCity, iMaxProgress)

def getCityTurns(city):
	if (city):
		player = gc.getPlayer(city.getOwner())
		iThreshold = player.greatPeopleThreshold(False)
		iRate = city.getGreatPeopleRate()
		if (iRate > 0):
			iProgress = city.getGreatPeopleProgress()
			iTurns = (iThreshold - iProgress + iRate - 1) / iRate
			return iTurns
	return None

def calcPercentages(city):

	# Calc total rate
	#iTotal = 0
	# advc.001c: Total no longer needed, and the >0 check would prevent percentages from being shown when the GP rate is positive but no GP have been accumulated yet.
	iTotal=1
	#for iUnit in g_gpUnitTypes:
	#	iTotal += city.getGreatPeopleUnitProgress(iUnit)
	# Calc individual percentages based on rates and total
	percents = []
	if (iTotal > 0):
		iLeftover = 100
		for iUnit in range(gc.getNumUnitInfos()):
			#iProgress = city.getGreatPeopleUnitProgress(iUnit)
			#if (iProgress > 0):
			#	iPercent = 100 * iProgress / iTotal
			#	iLeftover -= iPercent
			#	percents.append((iPercent, iUnit))
			# <advc.001c> Replacing the above
			# In principle, any unit can be born as a GP if CvBuildingInfos.xml says so, but GPProjection is inefficiently coded, so I don't want to call it more often than necessary, and arbitrary units wouldn't have an icon to display on the GP bar anyway.
			if gc.getUnitInfo(iUnit).getSpecialUnitType() >= 0 and gc.getUnitInfo(iUnit).getProductionCost() < 0:
				iPercent = city.GPProjection(iUnit)
				if iPercent > 0:
					percents.append((iPercent, iUnit))
			# </advc.001c>
		# Add remaining from 100 to first in list to match Civ4
		# advc.001c: No longer needed
		#if (iLeftover > 0):
		#	percents[0] = (percents[0][0] + iLeftover, percents[0][1])
	return percents

# Displaying Progress

def getHoverText(eWidgetType, iData1, iData2, bOption):
	city, iTurns = getDisplayCity()
	if (not city):
		# no rate or progress in any city and no city selected
		return BugUtil.getText("TXT_KEY_MISC_GREAT_PERSON", (0, PlayerUtil.getActivePlayer().greatPeopleThreshold(False)))
	iThreshold = gc.getPlayer(city.getOwner()).greatPeopleThreshold(False)
	iProgress = city.getGreatPeopleProgress()
	iRate = city.getGreatPeopleRate()
	#szText = BugUtil.colorText(city.getName(), "COLOR_HIGHLIGHT_TEXT")
	# advc.004: Don't color the city (I don't think this matches the overall color scheme of the game)
	szText = city.getName()
	szText += u"\n"
	szText += BugUtil.getText("TXT_KEY_MISC_GREAT_PERSON", (iProgress, iThreshold))
	if (iRate > 0):
		szText += u"\n%d%s%s " % (iRate, SASVisuals.GreatPeople.getDefaultIcon(), BugUtil.getPlainText("TXT_KEY_PER_TURN"))
		szText += BugUtil.getText("INTERFACE_CITY_TURNS", (iTurns,))

	percents = calcPercentages(city)
	if (len(percents) > 0):
		percents.sort()
		percents.reverse()
		#szText += u"\n" advc.004: No empty line between total rate and percentages
		for iPercent, iUnit in percents:
#			iUnit = getUnitType(gpType)
			szText += u"\n%s%s - %d%%" % (getUnitIcon(iUnit), gc.getUnitInfo(iUnit).getDescription(), iPercent)
	return szText

def getGreatPeopleText(city, iGPTurns, iGPBarWidth, bGPBarTypesNone, bGPBarTypesOne, bIncludeCityName):
	sGreatPeopleChar = SASVisuals.GreatPeople.getDefaultIcon()
	if (not city):
		szText = BugUtil.getText("INTERFACE_GREAT_PERSON_NONE", (sGreatPeopleChar, ))
	elif (bGPBarTypesNone):
		if (iGPTurns):
			if (bIncludeCityName):
				szText = BugUtil.getText("INTERFACE_GREAT_PERSON_CITY_TURNS", (sGreatPeopleChar, city.getName(), iGPTurns))
			else:
				szText = BugUtil.getText("INTERFACE_GREAT_PERSON_TURNS", (sGreatPeopleChar, iGPTurns))
		else:
			if (bIncludeCityName):
				szText = BugUtil.getText("INTERFACE_GREAT_PERSON_CITY", (sGreatPeopleChar, city.getName()))
			else:
				szText = sGreatPeopleChar
	else:
		lPercents = calcPercentages(city)
		if (len(lPercents) == 0):
			if (iGPTurns):
				if (bIncludeCityName):
					szText = BugUtil.getText("INTERFACE_GREAT_PERSON_CITY_TURNS", (sGreatPeopleChar, city.getName(), iGPTurns))
				else:
					szText = BugUtil.getText("INTERFACE_GREAT_PERSON_TURNS", (sGreatPeopleChar, iGPTurns))
			else:
				if (bIncludeCityName):
					szText = BugUtil.getText("INTERFACE_GREAT_PERSON_CITY", (sGreatPeopleChar, city.getName()))
				else:
					szText = sGreatPeopleChar
		else:
			lPercents.sort()
			lPercents.reverse()
			if (bGPBarTypesOne or len(lPercents) == 1):
				iPercent, iUnit = lPercents[0]
				pInfo = gc.getUnitInfo(iUnit)
				if (iGPTurns):
					if (bIncludeCityName):
						szText = BugUtil.getText("INTERFACE_GREAT_PERSON_CITY_TURNS", (pInfo.getDescription(), city.getName(), iGPTurns))
					else:
						szText = BugUtil.getText("INTERFACE_GREAT_PERSON_TURNS", (pInfo.getDescription(), iGPTurns))
				else:
					if (bIncludeCityName):
						szText = BugUtil.getText("INTERFACE_GREAT_PERSON_CITY", (pInfo.getDescription(), city.getName()))
					else:
						szText = unicode(pInfo.getDescription())
				# <!-- custom: show chars in the GP bar even if 100% (e.g. `Great Scientist: Lyons (3) - [RESEARCH_CHAR] 100%`). (GPT-5.2-Codex) -->
				szText = u"%s - %s100%%" % (szText, getUnitIcon(iUnit))
			else:
				if (iGPTurns):
					if (bIncludeCityName):
						szText = BugUtil.getText("INTERFACE_GREAT_PERSON_CITY_TURNS", (sGreatPeopleChar, city.getName(), iGPTurns))
					else:
						szText = BugUtil.getText("INTERFACE_GREAT_PERSON_TURNS", (sGreatPeopleChar, iGPTurns))
				else:
					if (bIncludeCityName):
						szText = BugUtil.getText("INTERFACE_GREAT_PERSON_CITY", (sGreatPeopleChar, city.getName()))
					else:
						szText = sGreatPeopleChar + u":"
				szTypes = ""
				for iPercent, iUnit in lPercents:
					szNewTypes = szTypes + u" %c%d%%" % (getUnitIcon(iUnit), iPercent)
					# <!-- custom: Maximum-types measured the previously accepted list, so the first candidate exceeding the GP bar width was still appended.
					# Measure the candidate list before accepting it. See KI#237. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
					szNewText = szText + u" -%s" % szNewTypes
					# <!-- custom: SAS moved font ownership to both callers, but untagged measurement could disagree with their configurable Label-font rendering; wrap only the temporary candidate used for width measurement so returned text remains unnested. See KI#1066. (GPT-5.6-Sol) -->
					if (CyInterface().determineWidth(SASTextScale.labelText(szNewText)) > iGPBarWidth - 10):
						# Keep under width
						break
					szTypes = szNewTypes
				if (len(szTypes) > 0):
					szText += u" -%s" % szTypes
	return szText
