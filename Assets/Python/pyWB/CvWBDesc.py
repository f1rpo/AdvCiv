## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
#
# Code for saving and loading a high-level description of the world.
# Used by WorldBuilder
#
# Author -	Mustafa Thamer
#

from CvPythonExtensions import *
import os
import sys
import CvUtil
from array import *

# globals
gc = CyGlobalContext()
version = 11
fileencoding = "latin_1"	# aka "iso-8859-1"

#############
def getPlayer(idx):
	"helper function which wraps get player in case of bad index"
	#if (gc.getPlayer(idx).isAlive()):
	#	return gc.getPlayer(idx)
	#return None
	# <advc.056> Treat invalid civ ids as the Barbarians
	if idx < gc.getMAX_CIV_TEAMS() and gc.getPlayer(idx).isAlive():
		return gc.getPlayer(idx)
	return gc.getPlayer(gc.getBARBARIAN_PLAYER())
	# </advc.056>

#############
class CvWBParser:
	"parser functions for WB desc"
	def getTokens(self, line):
		"return a list of (comma separated) tokens from the line.  Strip whitespace on each token"
		if line==None:
			return list()
		toks=line.split(",")
		toksOut=list()
		for tok in toks:
			toksOut.append(tok.strip())
		return toksOut

	def findToken(self, toks, item):
		"return true if item exists in list of tokens"
		for tok in toks:
			if (tok==item):
				return true
		return false

	def findTokenValue(self, toks, item):
		"Search for a token of the form item=value in the list of toks, and return value, or -1 if not found"
		for tok in toks:
			l=tok.split("=")
			if (item==l[0]):
				if (len(l)==1):
					return item
				return l[1]
		return -1		# failed

	def getNextLine(self, f):
		"return the next line from the list of lines"
		return f.readline()

	def findNextToken(self, f, item):
		"Find the next line that contains the token item, return false if not found"
		while True:
			line = self.getNextLine(f)
			if (not line):
				return false	# EOF
			toks=self.getTokens(line)
			if (self.findToken(toks, item)):
				return true
		return false

	def findNextTokenValue(self, f, item):
		"Find the next line that contains item=value, return value or -1 if not found"
		while True:
			line = self.getNextLine(f)
			if (not line):
				return -1		# EOF
			toks=self.getTokens(line)
			val=self.findTokenValue(toks, item)
			if (val != -1):
				return val
		return -1

#############
class CvGameDesc:
	"class for serializing game data"
	def __init__(self):
		self.eraType = "NONE"
		self.speedType = "NONE"
		self.calendarType = "CALENDAR_DEFAULT"
		self.options = ()
		self.mpOptions = ()
		self.forceControls = ()
		self.victories = ()
		self.gameTurn = 0
		self.maxTurns = 0
		self.maxCityElimination = 0
		self.numAdvancedStartPoints = 0
		self.targetScore = 0
		self.iStartYear = -4000
		self.szDescription = ""
		self.szModPath = ""
		self.iRandom = 0

	def apply(self):
		"after reading, apply the game data"
		gc.getGame().setStartYear(self.iStartYear)

	def write(self, f):
		"write out game data"
		f.write("BeginGame\n")
		f.write("\tEra=%s\n" %(gc.getEraInfo(gc.getGame().getStartEra()).getType(),))
		f.write("\tSpeed=%s\n" %(gc.getGameSpeedInfo(gc.getGame().getGameSpeedType()).getType(),))
		f.write("\tCalendar=%s\n" %(gc.getCalendarInfo(gc.getGame().getCalendar()).getType(),))

		# write options
		for i in range(gc.getNumGameOptionInfos()):
			if (gc.getGame().isOption(i)):
				f.write("\tOption=%s\n" %(gc.getGameOptionInfo(i).getType()))

		# write mp options
		for i in range(gc.getNumMPOptionInfos()):
			if (gc.getGame().isMPOption(i)):
				f.write("\tMPOption=%s\n" %(gc.getMPOptionInfo(i).getType()))

		# write force controls
		for i in range(gc.getNumForceControlInfos()):
			if (gc.getGame().isForcedControl(i)):
				f.write("\tForceControl=%s\n" %(gc.getForceControlInfo(i).getType()))

		# write victories
		for i in range(gc.getNumVictoryInfos()):
			if (gc.getGame().isVictoryValid(i)):
				if (not gc.getVictoryInfo(i).isPermanent()):
					f.write("\tVictory=%s\n" %(gc.getVictoryInfo(i).getType()))

		f.write("\tGameTurn=%d\n" %(gc.getGame().getGameTurn(),))
		f.write("\tMaxTurns=%d\n" %(gc.getGame().getMaxTurns(),))
		f.write("\tMaxCityElimination=%d\n" %(gc.getGame().getMaxCityElimination(),))
		f.write("\tNumAdvancedStartPoints=%d\n" %(gc.getGame().getNumAdvancedStartPoints(),))
		f.write("\tTargetScore=%d\n" %(gc.getGame().getTargetScore(),))

		f.write("\tStartYear=%d\n" %(gc.getGame().getStartYear(),))
		f.write("\tDescription=%s\n" % (self.szDescription,))
		f.write("\tModPath=%s\n" % (self.szModPath,))
		f.write("EndGame\n")

	def read(self, f):
		"read in game data"
		self.__init__()

		parser = CvWBParser()
		if (parser.findNextTokenValue(f, "BeginGame")!=-1):
			while (true):
				nextLine = parser.getNextLine(f)
				toks = parser.getTokens(nextLine)
				if (len(toks)==0):
					break

				v = parser.findTokenValue(toks, "Era")
				if v!=-1:
					self.eraType = v
					continue

				v = parser.findTokenValue(toks, "Speed")
				if v!=-1:
					self.speedType = v
					continue

				v = parser.findTokenValue(toks, "Calendar")
				if v!=-1:
					self.calendarType = v
					continue

				v = parser.findTokenValue(toks, "Option")
				if v!=-1:
					self.options = self.options + (v,)
					continue

				v = parser.findTokenValue(toks, "MPOption")
				if v!=-1:
					self.mpOptions = self.mpOptions + (v,)
					continue

				v = parser.findTokenValue(toks, "ForceControl")
				if v!=-1:
					self.forceControls = self.forceControls + (v,)
					continue

				v = parser.findTokenValue(toks, "Victory")
				if v!=-1:
					self.victories = self.victories + (v,)
					continue

				v = parser.findTokenValue(toks, "GameTurn")
				if v!=-1:
					self.gameTurn = int(v)
					continue

				v = parser.findTokenValue(toks, "MaxTurns")
				if v!=-1:
					self.maxTurns = int(v)
					continue

				v = parser.findTokenValue(toks, "MaxCityElimination")
				if v!=-1:
					self.maxCityElimination = int(v)
					continue

				v = parser.findTokenValue(toks, "NumAdvancedStartPoints")
				if v!=-1:
					self.numAdvancedStartPoints = int(v)
					continue

				v = parser.findTokenValue(toks, "TargetScore")
				if v!=-1:
					self.targetScore = int(v)
					continue

				v = parser.findTokenValue(toks, "StartYear")
				if v!=-1:
					self.iStartYear = int(v)
					continue

				v = parser.findTokenValue(toks, "Description")
				if v!=-1:
					self.szDescription = v
					continue

				v = parser.findTokenValue(toks, "ModPath")
				if v!=-1:
					self.szModPath = v
					continue

				v = parser.findTokenValue(toks, "Random")
				if v!=-1:
					self.iRandom = int(v)
					continue

				if parser.findTokenValue(toks, "EndGame") != -1:
					break

#############
class CvTeamDesc:
	def __init__(self):
		self.techTypes = ()
		self.aaiEspionageAgainstTeams = []
		self.bContactWithTeamList = ()
		self.bWarWithTeamList = ()
		self.bPermanentWarPeaceList = ()
		self.bOpenBordersWithTeamList = ()
		self.bDefensivePactWithTeamList = ()
		self.bVassalOfTeamList = ()
		self.projectType = []
		self.bRevealMap = 0
		self.iMasterPower = 0
		self.iVassalPower = 0
		self.iEspionageEver = 0

	def write(self, f, idx):
		"write out team data"
		f.write("BeginTeam\n")

		# Team ID (to make things easier to mess with in the text)
		f.write("\tTeamID=%d\n" %(idx))

		# write techs
		for i in range(gc.getNumTechInfos()):
			if (gc.getTeam(idx).isHasTech(i)):
				f.write("\tTech=%s\n" %(gc.getTechInfo(i).getType()))
			if gc.getTechInfo(i).isRepeat():
				for j in range(gc.getTeam(idx).getTechCount(i)):
					f.write("\tTech=%s\n" %(gc.getTechInfo(i).getType()))

		# write Espionage against other teams
		for i in range(gc.getMAX_CIV_TEAMS()):
			if (gc.getTeam(idx).getEspionagePointsAgainstTeam(i) > 0):
				f.write("\tEspionageTeam=%d, EspionageAmount=%d\n" %(i, gc.getTeam(idx).getEspionagePointsAgainstTeam(i)))

		# write Espionage Ever against other teams
		if (gc.getTeam(idx).getEspionagePointsEver() > 0):
			f.write("\tEspionageEverAmount=%d\n" %(gc.getTeam(idx).getEspionagePointsEver()))

		# write met other teams
		for i in range(gc.getMAX_CIV_TEAMS()):
			if (gc.getTeam(idx).isHasMet(i)):
				f.write("\tContactWithTeam=%d\n" %(i))

		# write warring teams
		for i in range(gc.getMAX_CIV_TEAMS()):
			if (gc.getTeam(idx).isAtWar(i)):
				f.write("\tAtWar=%d\n" %(i))

		# write permanent war/peace teams
		for i in range(gc.getMAX_CIV_TEAMS()):
			if (gc.getTeam(idx).isPermanentWarPeace(i)):
				f.write("\tPermanentWarPeace=%d\n" %(i))

		# write open borders other teams
		for i in range(gc.getMAX_CIV_TEAMS()):
			if (gc.getTeam(idx).isOpenBorders(i)):
				f.write("\tOpenBordersWithTeam=%d\n" %(i))

		# write defensive pact other teams
		for i in range(gc.getMAX_CIV_TEAMS()):
			if (gc.getTeam(idx).isDefensivePact(i)):
				f.write("\tDefensivePactWithTeam=%d\n" %(i))

		# write vassal state
		for i in range(gc.getMAX_CIV_TEAMS()):
			if (gc.getTeam(idx).isVassal(i)):
				f.write("\tVassalOfTeam=%d\n" %(i))

		for i in range(gc.getNumProjectInfos()):
			for j in range(gc.getTeam(idx).getProjectCount(i)):
				f.write("\tProjectType=%s\n" %(gc.getProjectInfo(i).getType()))

		f.write("\tRevealMap=%d\n" %(0))

		if gc.getTeam(idx).getVassalPower() != 0:
			f.write("\tVassalPower=%d\n" %(gc.getTeam(idx).getVassalPower()))
		if gc.getTeam(idx).getMasterPower() != 0:
			f.write("\tMasterPower=%d\n" %(gc.getTeam(idx).getMasterPower()))

		f.write("EndTeam\n")

	def read(self, f):
		"read in team data"
		self.__init__()

		parser = CvWBParser()
		if (parser.findNextTokenValue(f, "BeginTeam")!=-1):
			while (true):
				nextLine = parser.getNextLine(f)
				toks = parser.getTokens(nextLine)
				if (len(toks)==0):
					break

				v = parser.findTokenValue(toks, "Tech")
				if v!=-1:
					self.techTypes = self.techTypes + (v,)
					continue

				v = parser.findTokenValue(toks, "EspionageTeam")
				if v!=-1:
					iTeam = int(v)

					iExtra = int(parser.findTokenValue(toks, "EspionageAmount"))
					self.aaiEspionageAgainstTeams.append([iTeam,iExtra])
					continue

				v = parser.findTokenValue(toks, "EspionageEverAmount")
				if v!=-1:
					self.iEspionageEver = int(v)
					continue

				v = parser.findTokenValue(toks, "ContactWithTeam")
				if v!=-1:
					self.bContactWithTeamList = self.bContactWithTeamList + (int(v),)
					continue

				v = parser.findTokenValue(toks, "AtWar")
				if v!=-1:
					self.bWarWithTeamList = self.bWarWithTeamList + (int(v),)
					continue

				v = parser.findTokenValue(toks, "PermanentWarPeace")
				if v!=-1:
					self.bPermanentWarPeaceList = self.bPermanentWarPeaceList + (int(v),)
					continue

				v = parser.findTokenValue(toks, "OpenBordersWithTeam")
				if v!=-1:
					self.bOpenBordersWithTeamList = self.bOpenBordersWithTeamList + (int(v),)
					continue

				v = parser.findTokenValue(toks, "DefensivePactWithTeam")
				if v!=-1:
					self.bDefensivePactWithTeamList = self.bDefensivePactWithTeamList + (int(v),)
					continue

				v = parser.findTokenValue(toks, "VassalOfTeam")
				if v!=-1:
					self.bVassalOfTeamList = self.bVassalOfTeamList + (int(v),)
					continue

				v = parser.findTokenValue(toks, "ProjectType")
				if v!=-1:
					self.projectType.append(v)
					continue

				v = parser.findTokenValue(toks, "RevealMap")
				if v!=-1:
					self.bRevealMap = int(v)
					continue

				v = parser.findTokenValue(toks, "VassalPower")
				if v!=-1:
					self.iVassalPower = int(v)
					continue

				v = parser.findTokenValue(toks, "MasterPower")
				if v!=-1:
					self.iMasterPower = int(v)
					continue

				if parser.findTokenValue(toks, "EndTeam") != -1:
					return true		# completed successfully

		return false	# failed

#############
class CvPlayerDesc:
	def __init__(self):
		self.szCivDesc = ""
		self.szCivShortDesc = ""
		self.szLeaderName = ""
		self.szCivAdjective = ""
		self.szFlagDecal = ""
		self.isWhiteFlag = 0

		self.leaderType = "NONE"
		self.civType = "NONE"
		self.handicap = gc.getHandicapInfo(gc.getDefineINT("STANDARD_HANDICAP")).getType()
		self.team = -1		# team index
		self.color = "NONE"
		self.artStyle = "NONE"
		self.isPlayableCiv = 1
		self.isMinorNationCiv = 0
		self.iStartingGold = 0
		self.iStartingX = -1
		self.iStartingY = -1
		self.stateReligion = ""
		self.szStartingEra = ""
		self.bRandomStartLocation = "false"
		self.bNeverAlive = False # advc.056

		self.aaiCivics = []
		self.aaiAttitudeExtras = []
		self.aszCityList = []

	def write(self, f, idx):
		"write out player data"
		f.write("BeginPlayer\n")

		# write team
		f.write("\tTeam=%d\n" %(int(gc.getPlayer(idx).getTeam())))

		# write leader and Civ Description info
		if (gc.getPlayer(idx).getLeaderType() == LeaderHeadTypes.NO_LEADER):
			f.write("\tLeaderType=NONE\n")

		else:
			f.write("\tLeaderType=%s\n" %(gc.getLeaderHeadInfo(gc.getPlayer(idx).getLeaderType()).getType()))

		# write civ, color, artStyle, isPlayableCiv, isMinorNation, StartingGold and StateReligion
		if (gc.getPlayer(idx).getCivilizationType() == CivilizationTypes.NO_CIVILIZATION):
			f.write("\tCivType=NONE\n")
			f.write("\tColor=NONE\n")
			f.write("\tArtStyle=NONE\n")
		else:
			f.write("\tLeaderName=%s\n" %(gc.getPlayer(idx).getNameKey().encode(fileencoding)))
			f.write("\tCivDesc=%s\n" %(gc.getPlayer(idx).getCivilizationDescriptionKey().encode(fileencoding)))
			f.write("\tCivShortDesc=%s\n" %(gc.getPlayer(idx).getCivilizationShortDescriptionKey().encode(fileencoding)))
			f.write("\tCivAdjective=%s\n" %(gc.getPlayer(idx).getCivilizationAdjectiveKey().encode(fileencoding)))
			f.write("\tFlagDecal=%s\n" %(gc.getPlayer(idx).getFlagDecal().encode(fileencoding)))
			f.write("\tWhiteFlag=%d\n" %(gc.getPlayer(idx).isWhiteFlag(),))
			f.write("\tCivType=%s\n" %(gc.getCivilizationInfo(gc.getPlayer(idx).getCivilizationType()).getType()))
			f.write("\tColor=%s\n" %(gc.getPlayerColorInfo(gc.getPlayer(idx).getPlayerColor()).getType()))
			f.write("\tArtStyle=%s\n" %(gc.getArtStyleTypes(gc.getPlayer(idx).getArtStyleType())))
			f.write("\tPlayableCiv=%d\n" %(int(gc.getPlayer(idx).isPlayable())))
			f.write("\tMinorNationStatus=%d\n" %(gc.getPlayer(idx).isMinorCiv()))
			f.write("\tStartingGold=%d\n" %(gc.getPlayer(idx).getGold()))

			if gc.getPlayer(idx).isAlive():
				pPlot = gc.getPlayer(idx).getStartingPlot()
				if (not pPlot.isNone()):
					f.write("\tStartingX=%d, StartingY=%d\n" %(pPlot.getX(), pPlot.getY()))

			pPlayerReligionInfo = gc.getReligionInfo(gc.getPlayer(idx).getStateReligion())
			if pPlayerReligionInfo:
				f.write("\tStateReligion=%s\n" %(pPlayerReligionInfo.getType()))
			else:
				f.write("\tStateReligion=\n")

			f.write("\tStartingEra=%s\n" %(gc.getEraInfo(gc.getPlayer(idx).getCurrentEra()).getType()))

			f.write("\tRandomStartLocation=false\n")

			# write Civics
			for iCivicOptionLoop in range(gc.getNumCivicOptionInfos()):
				for iCivicLoop in range(gc.getNumCivicInfos()):
					iCivic = gc.getPlayer(idx).getCivics(iCivicOptionLoop)
					if (iCivicLoop == iCivic):
						f.write("\tCivicOption=%s, Civic=%s\n" %(gc.getCivicOptionInfo(iCivicOptionLoop).getType(), gc.getCivicInfo(iCivicLoop).getType()))

			# write Attitude Extra
			for i in range(gc.getMAX_CIV_PLAYERS()):
				if (gc.getPlayer(idx).AI_getAttitudeExtra(i) != 0):
					f.write("\tAttitudePlayer=%d, AttitudeExtra=%d\n" %(i, gc.getPlayer(idx).AI_getAttitudeExtra(i)))

			# write City List
			for i in range(gc.getPlayer(idx).getNumCityNames()):
				f.write("\tCityList=%s\n" %(gc.getPlayer(idx).getCityName(i)))

		if (gc.getPlayer(idx).getHandicapType() == HandicapTypes.NO_HANDICAP):
			f.write("\tHandicap=%s\n" %(gc.getHandicapInfo(gc.getDefineINT("STANDARD_HANDICAP")).getType()))
		else:
			f.write("\tHandicap=%s\n" %(gc.getHandicapInfo(gc.getPlayer(idx).getHandicapType()).getType()))

		f.write("EndPlayer\n")

	def read(self, f):
		"read in player data"
		self.__init__()
		parser = CvWBParser()
		if (parser.findNextTokenValue(f, "BeginPlayer")!=-1):
			while (true):
				nextLine = parser.getNextLine(f)
				toks = parser.getTokens(nextLine)
				if (len(toks)==0):
					break

				v = parser.findTokenValue(toks, "CivDesc")
				if v!=-1:
					self.szCivDesc = v.decode(fileencoding)
					continue

				v = parser.findTokenValue(toks, "CivShortDesc")
				if v!=-1:
					self.szCivShortDesc = v.decode(fileencoding)
					continue

				v = parser.findTokenValue(toks, "LeaderName")
				if v!=-1:
					self.szLeaderName = v.decode(fileencoding)
					continue

				v = parser.findTokenValue(toks, "CivAdjective")
				if v!=-1:
					self.szCivAdjective = v.decode(fileencoding)
					continue

				v = parser.findTokenValue(toks, "FlagDecal")
				if v!=-1:
					self.szFlagDecal = v.decode(fileencoding)
					continue

				v = parser.findTokenValue(toks, "WhiteFlag")
				if v!=-1:
					self.isWhiteFlag = int(v)
					continue

				v = parser.findTokenValue(toks, "LeaderType")
				if v!=-1:
					self.leaderType = v
					continue

				v = parser.findTokenValue(toks, "CivType")
				if v!=-1:
					self.civType = v
					continue

				v = parser.findTokenValue(toks, "Team")
				if v!=-1:
					self.team = int(v)
					continue

				v = parser.findTokenValue(toks, "Handicap")
				if v!=-1:
					self.handicap = v
					continue

				v = parser.findTokenValue(toks, "Color")
				if v!=-1:
					self.color = v
					continue

				v = parser.findTokenValue(toks, "ArtStyle")
				if v!=-1:
					self.artStyle = v
					continue

				v = parser.findTokenValue(toks, "PlayableCiv")
				if v!=-1:
					self.isPlayableCiv = int(v)
					continue

				v = parser.findTokenValue(toks, "MinorNationStatus")
				if v!=-1:
					self.isMinorNationCiv = int(v)
					continue

				v = parser.findTokenValue(toks, "StartingGold")
				if v!=-1:
					self.iStartingGold = int(v)
					continue

				vX = parser.findTokenValue(toks, "StartingX")
				vY = parser.findTokenValue(toks, "StartingY")
				if vX!=-1 and vY!=-1:
					self.iStartingX = int(vX)
					self.iStartingY = int(vY)
					continue

				v = parser.findTokenValue(toks, "StateReligion")
				if v!=-1:
					self.stateReligion = v
					continue

				v = parser.findTokenValue(toks, "StartingEra")
				if v!=-1:
					self.szStartingEra = v
					continue

				v = parser.findTokenValue(toks, "RandomStartLocation")
				if v!=-1:
					self.bRandomStartLocation = v
					continue

				v = parser.findTokenValue(toks, "CivicOption")
				if v!=-1:
					iCivicOptionType = gc.getInfoTypeForString(v)

					v = parser.findTokenValue(toks, "Civic")
					if v!=-1:
						iCivicType = gc.getInfoTypeForString(v)
						self.aaiCivics.append([iCivicOptionType,iCivicType])
						continue

				v = parser.findTokenValue(toks, "AttitudePlayer")
				if v!=-1:
					iPlayer = int(v)

					iExtra = int(parser.findTokenValue(toks, "AttitudeExtra"))
					self.aaiAttitudeExtras.append([iPlayer,iExtra])
					continue

				v = parser.findTokenValue(toks, "CityList")
				if v!=-1:
					self.aszCityList.append(v)
					continue

				if parser.findTokenValue(toks, "EndPlayer") != -1:
					#print("Civics:")
					#print self.aaiCivics
					#print("Attitudes:")
					#print self.aaiAttitudeExtras
					#break
					# <advc.056> Same as in CvTeamDesc.read
					return True
		return False # </advc.056>

#############
class CvUnitDesc:
	"unit WB serialization"
	def __init__(self):
		self.plotX = -1
		self.plotY = -1
		self.unitType = None
		self.szName = None
		self.leaderUnitType = None
		self.owner =-1
		self.damage = 0
		self.level = -1
		self.experience = -1
		self.promotionType = []
		self.facingDirection = DirectionTypes.NO_DIRECTION;
		self.isSleep = False
		self.isIntercept = False
		self.isPatrol = False
		self.isPlunder = False
		self.szUnitAIType = "NO_UNITAI"
		self.szScriptData = "NONE"

	def apply(self):
		"after reading, this will actually apply the data"
		player = getPlayer(self.owner)
		if (player):
			# print ("unit apply %d %d" %(self.plotX, self.plotY))
			CvUtil.pyAssert(self.plotX>=0 and self.plotY>=0, "invalid plot coords")
			unitTypeNum = CvUtil.findInfoTypeNum(gc.getUnitInfo, gc.getNumUnitInfos(), self.unitType)
			if (unitTypeNum < 0):
				unit = None
			else:
				if (self.szUnitAIType != "NO_UNITAI"):
					eUnitAI = CvUtil.findInfoTypeNum(gc.getUnitAIInfo, UnitAITypes.NUM_UNITAI_TYPES, self.szUnitAIType) #pUnitAI.getType()
				else:
					eUnitAI = UnitAITypes.NO_UNITAI

				unit = player.initUnit(unitTypeNum, self.plotX, self.plotY, UnitAITypes(eUnitAI), self.facingDirection)
			if (unit):
				if (self.szName != None):
					unit.setName(self.szName)
				#leader unit type
				if(self.leaderUnitType != None):
					leaderUnitTypeNum = CvUtil.findInfoTypeNum(gc.getUnitInfo, gc.getNumUnitInfos(), self.leaderUnitType)
					if leaderUnitTypeNum >= 0:
						unit.setLeaderUnitType(leaderUnitTypeNum);

				#other properties
				if self.damage != 0:
					unit.setDamage(self.damage, PlayerTypes.NO_PLAYER)
				if self.level != -1:
					unit.setLevel(self.level)
				if self.experience != -1:
					unit.setExperience(self.experience, -1)
				for promo in self.promotionType:
					promotionTypeNum = CvUtil.findInfoTypeNum(gc.getPromotionInfo, gc.getNumPromotionInfos(), promo)
					unit.setHasPromotion(promotionTypeNum, True)
				if self.isSleep:
					unit.getGroup().setActivityType(ActivityTypes.ACTIVITY_SLEEP)
				elif self.isIntercept:
					unit.getGroup().setActivityType(ActivityTypes.ACTIVITY_INTERCEPT)
				elif self.isPatrol:
					unit.getGroup().setActivityType(ActivityTypes.ACTIVITY_PATROL)
				elif self.isPlunder:
					unit.getGroup().setActivityType(ActivityTypes.ACTIVITY_PLUNDER)
				if self.szScriptData != "NONE":
					unit.setScriptData(self.szScriptData)

	def read(self, f, pX, pY):
		"read in unit data - at this point the first line 'BeginUnit' has already been read"
		self.__init__()
		self.plotX = pX
		self.plotY = pY
		CvUtil.pyAssert(self.plotX>=0 and self.plotY>=0, "invalid plot coords")

		parser = CvWBParser()
		while (true):
			nextLine = parser.getNextLine(f)
			toks = parser.getTokens(nextLine)
			if (len(toks)==0):
				break

			v = parser.findTokenValue(toks, "UnitType")
			vOwner = parser.findTokenValue(toks, "UnitOwner")
			if (v!=-1 and vOwner != -1):
				self.unitType = v
				self.owner = int(vOwner)
				continue

			v = parser.findTokenValue(toks, "UnitName")
			if (v != -1):
				self.szName = v.decode(fileencoding)
				continue

			v = parser.findTokenValue(toks, "LeaderUnitType")
			if (v != -1):
				self.leaderUnitType = v
				continue

			v = parser.findTokenValue(toks, "Damage")
			if (v != -1):
				self.damage = (int(v))
				continue

			v = parser.findTokenValue(toks, "Level")
			if (v != -1):
				self.level = (int(v))
				self.experience = int(parser.findTokenValue(toks, "Experience"))
				continue

			# Units - Promotions
			v = parser.findTokenValue(toks, "PromotionType")
			if v!=-1:
				self.promotionType.append(v)
				continue

			v = parser.findTokenValue(toks, "FacingDirection")
			if (v != -1):
				self.facingDirection = (DirectionTypes(v))
				continue

			if (parser.findTokenValue(toks, "Sleep"))!=-1:
				self.isSleep = True
				continue

			if (parser.findTokenValue(toks, "Intercept"))!=-1:
				self.isIntercept = True
				continue

			if (parser.findTokenValue(toks, "Patrol"))!=-1:
				self.isPatrol = True
				continue

			if (parser.findTokenValue(toks, "Plunder"))!=-1:
				self.isPlunder = True
				continue

			v = parser.findTokenValue(toks, "UnitAIType")
			if (v != -1):
				self.szUnitAIType = v
				continue

			v = parser.findTokenValue(toks, "ScriptData")
			if v!=-1:
				print("found script data: %s" %(v))
				self.szScriptData = v
				continue

			if parser.findTokenValue(toks, "EndUnit") != -1:
				break

	def write(self, f, unit, plot):
		"save unit desc to a file"
		unitType = gc.getUnitInfo(unit.getUnitType()).getType()
		unitOwner= unit.getOwner()
		f.write("\tBeginUnit\n")
		f.write("\t\tUnitType=%s, UnitOwner=%d\n" %(unitType,unitOwner))
		if (len(unit.getNameNoDesc()) > 0):
			f.write("\t\tUnitName=%s\n" %(unit.getNameNoDesc().encode(fileencoding),))
		if unit.getLeaderUnitType() != -1:
			f.write("\t\tLeaderUnitType=%s\n" %(gc.getUnitInfo(unit.getLeaderUnitType()).getType()))
		f.write("\t\tDamage=%d\n" %(unit.getDamage(),))
		f.write("\t\tLevel=%d, Experience=%d\n" %(unit.getLevel(), unit.getExperience()))
		for i in range(gc.getNumPromotionInfos()):
			if unit.isHasPromotion(i):
				f.write("\t\tPromotionType=%s\n" %(gc.getPromotionInfo(i).getType()))

		f.write("\t\tFacingDirection=%d\n" %(unit.getFacingDirection(),))
		if (unit.getGroup().getActivityType() == ActivityTypes.ACTIVITY_SLEEP):
			f.write("\t\tSleep\n")
		elif (unit.getGroup().getActivityType() == ActivityTypes.ACTIVITY_INTERCEPT):
			f.write("\t\tIntercept\n")
		elif (unit.getGroup().getActivityType() == ActivityTypes.ACTIVITY_PATROL):
			f.write("\t\tPatrol\n")
		elif (unit.getGroup().getActivityType() == ActivityTypes.ACTIVITY_PLUNDER):
			f.write("\t\tPlunder\n")
		f.write("\t\tUnitAIType=%s\n" %(gc.getUnitAIInfo(unit.getUnitAIType()).getType()))
		if unit.getScriptData():
			f.write("\t\tScriptData=%s\n" %unit.getScriptData() )
		f.write("\tEndUnit\n")

############
class CvCityDesc:
	"serializes city data"
	def __init__(self):
		self.city = None
		self.owner = None
		self.name = None
		self.population = 0
		self.productionUnit = "NONE"
		self.productionBuilding = "NONE"
		self.productionProject = "NONE"
		self.productionProcess = "NONE"
		self.culture = 0
		self.bldgType = []
		self.religions = []
		self.holyCityReligions = []
		self.corporations = []
		self.headquarterCorporations = []
		self.freeSpecialists = []
		self.plotX=-1
		self.plotY=-1
		self.szScriptData = "NONE"
		self.aiPlayerCulture = []
		for iPlayerLoop in range(gc.getMAX_CIV_PLAYERS()):
			self.aiPlayerCulture.append(0)

	def apply(self):
		"after reading, this will actually apply the data"
		player = getPlayer(self.owner)
		# <advc.056>
		if player is None:
			CvUtil.pyPrint("Error: City owner %d not alive" %(self.owner))
			return # </advc.056>

		self.city = player.initCity(self.plotX, self.plotY)

		if (self.name != None):
			self.city.setName(self.name, False)

		if (self.population):
			self.city.setPopulation(self.population)

		if (self.culture):
			self.city.setCulture(self.city.getOwner(), self.culture, True)

		for bldg in (self.bldgType):
			bldgTypeNum = CvUtil.findInfoTypeNum(gc.getBuildingInfo, gc.getNumBuildingInfos(), bldg)
			self.city.setNumRealBuilding(bldgTypeNum, 1)

		for religion in (self.religions):
			religionTypeNum = CvUtil.findInfoTypeNum(gc.getReligionInfo, gc.getNumReligionInfos(), religion)
			self.city.setHasReligion(religionTypeNum, true, false, true)

		for holyCityRel in (self.holyCityReligions):
			religionTypeNum = CvUtil.findInfoTypeNum(gc.getReligionInfo, gc.getNumReligionInfos(), holyCityRel)
			gc.getGame().setHolyCity(religionTypeNum, self.city, false)

		for corporation in (self.corporations):
			corporationTypeNum = CvUtil.findInfoTypeNum(gc.getCorporationInfo, gc.getNumCorporationInfos(), corporation)
			self.city.setHasCorporation(corporationTypeNum, true, false, true)

		for headquarters in (self.headquarterCorporations):
			corporationTypeNum = CvUtil.findInfoTypeNum(gc.getCorporationInfo, gc.getNumCorporationInfos(), headquarters)
			gc.getGame().setHeadquarters(corporationTypeNum, self.city, false)

		for iSpecialist in self.freeSpecialists:
			specialistTypeNum = CvUtil.findInfoTypeNum(gc.getSpecialistInfo, gc.getNumSpecialistInfos(), iSpecialist)
			self.city.changeFreeSpecialistCount(specialistTypeNum, 1)

		for iPlayerLoop in range(gc.getMAX_CIV_PLAYERS()):
			if gc.getPlayer(iPlayerLoop).isEverAlive(): # advc.056
				iPlayerCulture = self.aiPlayerCulture[iPlayerLoop]
				if (iPlayerCulture > 0):
					self.city.setCulture(iPlayerLoop, iPlayerCulture, true)

		unitTypeNum = CvUtil.findInfoTypeNum(gc.getUnitInfo, gc.getNumUnitInfos(), self.productionUnit)
		buildingTypeNum = CvUtil.findInfoTypeNum(gc.getBuildingInfo, gc.getNumBuildingInfos(), self.productionBuilding)
		projectTypeNum = CvUtil.findInfoTypeNum(gc.getProjectInfo, gc.getNumProjectInfos(), self.productionProject)
		processTypeNum = CvUtil.findInfoTypeNum(gc.getProcessInfo, gc.getNumProcessInfos(), self.productionProcess)

		if (unitTypeNum != UnitTypes.NO_UNIT):
			self.city.pushOrder(OrderTypes.ORDER_TRAIN, unitTypeNum, -1, False, False, False, True)
		elif (buildingTypeNum != BuildingTypes.NO_BUILDING):
			self.city.pushOrder(OrderTypes.ORDER_CONSTRUCT, buildingTypeNum, -1, False, False, False, True)
		elif (projectTypeNum != ProjectTypes.NO_PROJECT):
			self.city.pushOrder(OrderTypes.ORDER_CREATE, projectTypeNum, -1, False, False, False, True)
		elif (processTypeNum != ProcessTypes.NO_PROCESS):
			self.city.pushOrder(OrderTypes.ORDER_MAINTAIN, processTypeNum, -1, False, False, False, True)

		if (self.szScriptData != "NONE"):
			self.city.setScriptData(self.szScriptData)

	def write(self, f, plot):
		"write out city data"
		city = plot.getPlotCity()
		CvUtil.pyAssert(city.isNone()==0, "null city?")
		f.write("\tBeginCity\n")
		f.write("\t\tCityOwner=%d\n" %(city.getOwner(),))
		f.write("\t\tCityName=%s\n" %(city.getNameKey().encode(fileencoding),))
		f.write("\t\tCityPopulation=%d\n" %(city.getPopulation(),))
		if (city.isProductionUnit()):
			f.write("\t\tProductionUnit=%s\n" %(gc.getUnitInfo(city.getProductionUnit()).getType(),))
		elif (city.isProductionBuilding()):
			f.write("\t\tProductionBuilding=%s\n" %(gc.getBuildingInfo(city.getProductionBuilding()).getType(),))
		elif (city.isProductionProject()):
			f.write("\t\tProductionProject=%s\n" %(gc.getProjectInfo(city.getProductionProject()).getType(),))
		elif (city.isProductionProcess()):
			f.write("\t\tProductionProcess=%s\n" %(gc.getProcessInfo(city.getProductionProcess()).getType(),))
#		f.write("\t\tCityCulture=%d\n" %(city.getCulture(city.getOwner()),))

		for iI in range(gc.getNumBuildingInfos()):
			if city.getNumRealBuilding(iI) > 0:
				f.write("\t\tBuildingType=%s\n" %(gc.getBuildingInfo(iI).getType()))

		for iI in range(gc.getNumReligionInfos()):
			if city.isHasReligion(iI):
				f.write("\t\tReligionType=%s\n" %(gc.getReligionInfo(iI).getType()))
			if (city.isHolyCityByType(iI)):
				f.write("\t\tHolyCityReligionType=%s\n" %(gc.getReligionInfo(iI).getType()))

		for iI in range(gc.getNumCorporationInfos()):
			if city.isHasCorporation(iI):
				f.write("\t\tCorporationType=%s\n" %(gc.getCorporationInfo(iI).getType()))
			if (city.isHeadquartersByType(iI)):
				f.write("\t\tHeadquarterCorporationType=%s\n" %(gc.getCorporationInfo(iI).getType()))

		for iI in range(gc.getNumSpecialistInfos()):
			for iJ in range(city.getAddedFreeSpecialistCount(iI)):
				f.write("\t\tFreeSpecialistType=%s\n" %(gc.getSpecialistInfo(iI).getType()))

		if city.getScriptData():
			f.write("\t\tScriptData=%s\n" %city.getScriptData())

		# Player culture
		for iPlayerLoop in range(gc.getMAX_CIV_PLAYERS()):
			iPlayerCulture = city.getCulture(iPlayerLoop)
			if (iPlayerCulture > 0):
				f.write("\t\tPlayer%dCulture=%d\n" %(iPlayerLoop, iPlayerCulture))

		f.write("\tEndCity\n")

	def read(self, f, iX, iY):
		"read in city data - at this point the first line 'BeginCity' has already been read"
		self.__init__()
		self.plotX=iX
		self.plotY=iY
		parser = CvWBParser()
		while (true):
			nextLine = parser.getNextLine(f)
			toks = parser.getTokens(nextLine)
			if (len(toks)==0):
				break

			# City - Owner
			vOwner=parser.findTokenValue(toks, "CityOwner")
			if (vOwner != -1):
				self.owner = int(vOwner)
				continue

			# City - Name
			vName=parser.findTokenValue(toks, "CityName")
			if (vName != -1):
				self.name = (vName).decode(fileencoding)
				continue

			# City - Population
			v=parser.findTokenValue(toks, "CityPopulation")
			if v!=-1:
				self.population = (int(v))
				continue

			# City - Production Unit
			v=parser.findTokenValue(toks, "ProductionUnit")
			if v!=-1:
				self.productionUnit = v
				continue

			# City - Production Building
			v=parser.findTokenValue(toks, "ProductionBuilding")
			if v!=-1:
				self.productionBuilding = v
				continue

			# City - Production Project
			v=parser.findTokenValue(toks, "ProductionProject")
			if v!=-1:
				self.productionProject = v
				continue

			# City - Production Process
			v=parser.findTokenValue(toks, "ProductionProcess")
			if v!=-1:
				self.productionProcess = v
				continue

			# City - Culture XXX obsolete...
			v=parser.findTokenValue(toks, "CityCulture")
			if v!=-1:
				self.culture = int(v)
				continue

			# City - Buildings
			v=parser.findTokenValue(toks, "BuildingType")
			if v!=-1:
				self.bldgType.append(v)
				continue

			# City - Religions
			v=parser.findTokenValue(toks, "ReligionType")
			if v!=-1:
				self.religions.append(v)
				continue

			# City - HolyCity
			v=parser.findTokenValue(toks, "HolyCityReligionType")
			if v!=-1:
				self.holyCityReligions.append(v)
				continue

			# City - Corporations
			v=parser.findTokenValue(toks, "CorporationType")
			if v!=-1:
				self.corporations.append(v)
				continue

			# City - Headquarters
			v=parser.findTokenValue(toks, "HeadquarterCorporationType")
			if v!=-1:
				self.headquarterCorporations.append(v)
				continue

			# City - Free Specialist
			v=parser.findTokenValue(toks, "FreeSpecialistType")
			if v!=-1:
				self.freeSpecialists.append(v)
				continue

			# City - ScriptData
			v=parser.findTokenValue(toks, "ScriptData")
			if v!=-1:
				self.szScriptData = v
				continue

			# Player Culture
			for iPlayerLoop in range(gc.getMAX_CIV_PLAYERS()):
				szCityTag = ("Player%dCulture" %(iPlayerLoop))
				v = parser.findTokenValue(toks, szCityTag)
				if v!=-1:
					self.aiPlayerCulture[iPlayerLoop] = int(v)
					continue

			if parser.findTokenValue(toks, "EndCity")!=-1:
				break

###########
class CvPlotDesc:
	"serializes plot data"
	def __init__(self):
		self.iX = -1
		self.iY = -1
		self.riverNSDirection = CardinalDirectionTypes.NO_CARDINALDIRECTION
		self.isNOfRiver = 0
		self.riverWEDirection = CardinalDirectionTypes.NO_CARDINALDIRECTION
		self.isWOfRiver = 0
		self.isStartingPlot = 0
		self.bonusType = None
		self.improvementType = None
		self.featureType = None
		self.featureVariety = 0
		self.routeType = None
		self.terrainType = None
		self.plotType = PlotTypes.NO_PLOT
		self.unitDescs = list()
		self.cityDesc = None
		self.szLandmark = ""
		self.szScriptData = "NONE"
		self.abTeamPlotRevealed = [0]*gc.getMAX_CIV_TEAMS()

	def needToWritePlot(self, plot):
		"returns true if this plot needs to be written out."
		return True

	def preApply(self):
		"apply plot and terrain type"
		plot = CyMap().plot(self.iX, self.iY)
		if (self.plotType != PlotTypes.NO_PLOT):
			plot.setPlotType(self.plotType, False, False)
		if (self.terrainType):
			terrainTypeNum = CvUtil.findInfoTypeNum(gc.getTerrainInfo, gc.getNumTerrainInfos(), self.terrainType)
			plot.setTerrainType(terrainTypeNum, False, False)

	def apply(self):
		"after reading, this will actually apply the data"
		#print("apply plot %d %d" %(self.iX, self.iY))
		plot = CyMap().plot(self.iX, self.iY)
		plot.setNOfRiver(self.isNOfRiver, self.riverWEDirection)
		plot.setWOfRiver(self.isWOfRiver, self.riverNSDirection)
		plot.setStartingPlot(self.isStartingPlot)
		if (self.bonusType):
			bonusTypeNum = CvUtil.findInfoTypeNum(gc.getBonusInfo, gc.getNumBonusInfos(), self.bonusType)
			plot.setBonusType(bonusTypeNum)
		if (self.improvementType):
			improvementTypeNum = CvUtil.findInfoTypeNum(gc.getImprovementInfo, gc.getNumImprovementInfos(), self.improvementType)
			plot.setImprovementType(improvementTypeNum)
		if (self.featureType):
			featureTypeNum = CvUtil.findInfoTypeNum(gc.getFeatureInfo, gc.getNumFeatureInfos(), self.featureType)
			plot.setFeatureType(featureTypeNum, self.featureVariety)
		if (self.routeType):
			routeTypeNum = CvUtil.findInfoTypeNum(gc.getRouteInfo, gc.getNumRouteInfos(), self.routeType)
			plot.setRouteType(routeTypeNum)

		if (self.szLandmark != ""):
			CyEngine().addLandmark(CyMap().plot(self.iX, self.iY), "%s" %(self.szLandmark))

		if (self.szScriptData != "NONE"):
			plot.setScriptData(self.szScriptData)

	def applyUnits(self):
		#print "--apply units"
		for u in self.unitDescs:
			u.apply()

	def applyCity(self):
		if self.cityDesc:
			#print "--apply city"
			self.cityDesc.apply()

	def write(self, f, plot):
		"save plot desc to a file"
		f.write("BeginPlot\n")
		f.write("\tx=%d,y=%d\n" %(plot.getX(), plot.getY()))

		# scriptData
		if (plot.getScriptData() != ""):
			f.write("\tScriptData=%s\n" %plot.getScriptData())
		# rivers
		if (plot.getRiverNSDirection() != CardinalDirectionTypes.NO_CARDINALDIRECTION):
			f.write("\tRiverNSDirection=%d\n" %(int(plot.getRiverNSDirection()),) )
		if (plot.isNOfRiver()):
			f.write("\tisNOfRiver\n")
		if (plot.getRiverWEDirection() != CardinalDirectionTypes.NO_CARDINALDIRECTION):
			f.write("\tRiverWEDirection=%d\n" %(int(plot.getRiverWEDirection()),) )
		if (plot.isWOfRiver()):
			f.write("\tisWOfRiver\n")
		# extras
		if (plot.isStartingPlot()):
			f.write("\tStartingPlot\n")
		if (plot.getBonusType(-1)!=-1):
			f.write("\tBonusType=%s\n" %(gc.getBonusInfo(plot.getBonusType(-1)).getType()) )
		if (plot.getImprovementType()!=-1):
			f.write("\tImprovementType=%s\n" %(gc.getImprovementInfo(plot.getImprovementType()).getType()) )
		if (plot.getFeatureType()!=-1):
			f.write("\tFeatureType=%s, FeatureVariety=%d\n"
			%(gc.getFeatureInfo(plot.getFeatureType()).getType(), plot.getFeatureVariety(), ) )
		if (plot.getRouteType()!=-1):
			f.write("\tRouteType=%s\n" %(gc.getRouteInfo(plot.getRouteType()).getType()) )
		if (plot.getTerrainType()!=-1):
			f.write("\tTerrainType=%s\n" %(gc.getTerrainInfo(plot.getTerrainType()).getType()) )
		if (plot.getPlotType()!=PlotTypes.NO_PLOT):
			f.write("\tPlotType=%d\n" %(int(plot.getPlotType()),) )

		# units
		for i in range(plot.getNumUnits()):
			unit=plot.getUnit(i)
			if unit.getUnitType() == -1:
				continue
			CvUnitDesc().write(f, unit, plot)
		# city
		if (plot.isCity()):
			CvCityDesc().write(f, plot)

		# Fog of War
		bFirstReveal=true
		for iTeamLoop in range(gc.getMAX_CIV_TEAMS()):
			if (gc.getTeam(iTeamLoop).isAlive()):
				if (plot.isRevealed(iTeamLoop,0)):
					# Plot is revealed for this Team so write out the fact that it is; if not revealed don't write anything
					if (bFirstReveal):
						f.write("\tTeamReveal=")
						bFirstReveal=false
					f.write("%d," %(iTeamLoop))
		if (bFirstReveal==false):
			f.write("\n")	# terminate reveal line

		f.write("EndPlot\n")

	def read(self, f):
		"read in a plot desc"
		self.__init__()
		parser = CvWBParser()
		if parser.findNextToken(f, "BeginPlot")==false:
			return false	# no more plots
		while (true):
			nextLine = parser.getNextLine(f)
			toks = parser.getTokens(nextLine)
			if (len(toks)==0):
				break

			x = parser.findTokenValue(toks, "x")
			y = parser.findTokenValue(toks, "y")
			if (x!=-1 and y!=-1):
				self.iX = int(x)
				self.iY = int(y)
				# print("plot read %d %d" %(self.iX, self.iY))
				continue

			v = parser.findTokenValue(toks, "Landmark")
			if v!=-1:
				self.szLandmark=v
				continue

			v = parser.findTokenValue(toks, "ScriptData")
			if v!=-1:
				self.szScriptData=v
				continue

			v = parser.findTokenValue(toks, "RiverNSDirection")
			if v!=-1:
				self.riverNSDirection = (CardinalDirectionTypes(v))
				continue

			if (parser.findTokenValue(toks, "isNOfRiver"))!=-1:
				self.isNOfRiver = (true)
				continue

			v = parser.findTokenValue(toks, "RiverWEDirection")
			if v!=-1:
				self.riverWEDirection = (CardinalDirectionTypes(v))
				continue

			if (parser.findTokenValue(toks, "isWOfRiver"))!=-1:
				self.isWOfRiver = (true)
				continue

			if (parser.findTokenValue(toks, "StartingPlot"))!=-1:
				self.isStartingPlot = (true)
				continue

			v = parser.findTokenValue(toks, "BonusType")
			if v!=-1:
				self.bonusType = v
				continue

			v = parser.findTokenValue(toks, "ImprovementType")
			if v!=-1:
				self.improvementType = v
				continue

			v = parser.findTokenValue(toks, "FeatureType")
			if v!=-1:
				self.featureType = v
				v = parser.findTokenValue(toks, "FeatureVariety")
				if v!=-1:
					self.featureVariety = int(v)
				continue

			v = parser.findTokenValue(toks, "RouteType")
			if v!=-1:
				self.routeType = v
				continue

			v = parser.findTokenValue(toks, "TerrainType")
			if v!=-1:
				self.terrainType = v
				continue

			v = parser.findTokenValue(toks, "PlotType")
			if v!=-1:
				self.plotType = PlotTypes(v)
				continue

			# Units
			v = parser.findTokenValue(toks, "BeginUnit")
			if v!=-1:
				unitDesc = CvUnitDesc()
				unitDesc.read(f, self.iX, self.iY)
				self.unitDescs.append(unitDesc)
				continue

			# City
			v = parser.findTokenValue(toks, "BeginCity")
			if v!=-1:
				self.cityDesc = CvCityDesc()
				self.cityDesc.read(f, self.iX, self.iY)
				continue

			# Fog of War

			v = parser.findTokenValue(toks, "TeamReveal")
			if v!=-1:
				for iTeamLoop in toks:
					iTeamLoop = iTeamLoop.lstrip('TeamReveal=')
					if len(iTeamLoop):
						self.abTeamPlotRevealed[int(iTeamLoop)] = true
				continue

			if parser.findTokenValue(toks, "EndPlot")!=-1:
				break
		return True

################
class CvMapDesc:
	"serialize map data"
	def __init__(self):
		self.iGridW = 0
		self.iGridH = 0
		self.iTopLatitude = 0
		self.iBottomLatitude = 0
		self.bWrapX = 0
		self.bWrapY = 0
		self.worldSize = None
		self.climate = None
		self.seaLevel = None
		self.numPlotsWritten = 0
		self.numSignsWritten = 0
		self.bRandomizeResources = "false"

	def write(self, f):
		"write map data"
		map = CyMap()
		iGridW = map.getGridWidth()
		iGridH = map.getGridHeight()
		iNumPlots = iGridW * iGridH
		iNumSigns = CyEngine().getNumSigns()

		f.write("BeginMap\n")
		f.write("\tgrid width=%d\n" %(map.getGridWidth(),))
		f.write("\tgrid height=%d\n" %(map.getGridHeight(),))
		f.write("\ttop latitude=%d\n" %(map.getTopLatitude(),))
		f.write("\tbottom latitude=%d\n" %(map.getBottomLatitude(),))
		f.write("\twrap X=%d\n" %(map.isWrapX(),))
		f.write("\twrap Y=%d\n" %(map.isWrapY(),))
		f.write("\tworld size=%s\n" %(gc.getWorldInfo(map.getWorldSize()).getType(),))
		f.write("\tclimate=%s\n" %(gc.getClimateInfo(map.getClimate()).getType(),))
		f.write("\tsealevel=%s\n" %(gc.getSeaLevelInfo(map.getSeaLevel()).getType(),))
		f.write("\tnum plots written=%d\n" %(iNumPlots,))
		f.write("\tnum signs written=%d\n" %(iNumSigns,))
		f.write("\tRandomize Resources=false\n")
		f.write("EndMap\n")

	def read(self, f):
		"read map data"
		self.__init__()
		parser = CvWBParser()
		if parser.findNextToken(f, "BeginMap")==false:
			print "can't find map"
			return
		while (true):
			nextLine = parser.getNextLine(f)
			toks = parser.getTokens(nextLine)
			if (len(toks)==0):
				break

			v = parser.findTokenValue(toks, "grid width")
			if v!=-1:
				self.iGridW = int(v)
				continue

			v = parser.findTokenValue(toks, "grid height")
			if v!=-1:
				self.iGridH = int(v)
				continue

			v = parser.findTokenValue(toks, "top latitude")
			if v!=-1:
				self.iTopLatitude = int(v)
				continue

			v = parser.findTokenValue(toks, "bottom latitude")
			if v!=-1:
				self.iBottomLatitude = int(v)
				continue

			v = parser.findTokenValue(toks, "wrap X")
			if v!=-1:
				self.bWrapX = int(v)
				continue

			v = parser.findTokenValue(toks, "wrap Y")
			if v!=-1:
				self.bWrapY = int(v)
				continue

			v = parser.findTokenValue(toks, "world size")
			if v!=-1:
				self.worldSize = v
				continue

			v = parser.findTokenValue(toks, "climate")
			if v!=-1:
				self.climate = v
				continue

			v = parser.findTokenValue(toks, "sealevel")
			if v!=-1:
				self.seaLevel = v
				continue

			v = parser.findTokenValue(toks, "num plots written")
			if v!=-1:
				self.numPlotsWritten = int(v)
				continue

			v = parser.findTokenValue(toks, "num signs written")
			if v!=-1:
				self.numSignsWritten = int(v)
				continue

			v = parser.findTokenValue(toks, "Randomize Resources")
			if v!=-1:
				self.bRandomizeResources = v
				continue

			if parser.findTokenValue(toks, "EndMap")!=-1:
				break

################
class CvSignDesc:
	"serialize map data"
	def __init__(self):
		self.iPlotX = 0
		self.iPlotY = 0
		self.playerType = 0
		self.szCaption = ""

	def apply(self):
		plot = CyMap().plot(self.iPlotX, self.iPlotY)
		CyEngine().addSign(plot, self.playerType, self.szCaption)

	def write(self, f, sign):
		"write sign data"
		f.write("BeginSign\n")
		f.write("\tplotX=%d\n" %(sign.getPlot().getX(),))
		f.write("\tplotY=%d\n" %(sign.getPlot().getY(),))
		f.write("\tplayerType=%d\n" %(sign.getPlayerType(),))
		f.write("\tcaption=%s\n" %(sign.getCaption(),))
		f.write("EndSign\n")

	def read(self, f):
		"read sign data"
		self.__init__()
		parser = CvWBParser()
		if parser.findNextToken(f, "BeginSign")==false:
			print "can't find sign"
			return
		while (true):
			nextLine = parser.getNextLine(f)
			toks = parser.getTokens(nextLine)
			if (len(toks)==0):
				break

			v = parser.findTokenValue(toks, "plotX")
			if v!=-1:
				self.iPlotX = int(v)
				continue

			v = parser.findTokenValue(toks, "plotY")
			if v!=-1:
				self.iPlotY = int(v)
				continue

			v = parser.findTokenValue(toks, "playerType")
			if v!=-1:
				self.playerType = int(v)
				continue

			v = parser.findTokenValue(toks, "caption")
			if v!=-1:
				self.szCaption = v
				continue

			if parser.findTokenValue(toks, "EndSign")!=-1:
				break

		return True

class CvWBDesc:
	"handles saving/loading a worldbuilder description file"
	def __init__(self):
		# game data
		self.gameDesc = CvGameDesc()
		self.playersDesc = None
		self.plotDesc = None	# list
		self.signDesc = None	# list
		self.mapDesc = CvMapDesc()

	def getVersion(self):
		return version

	def getDescFileName(self, fileName):
		return fileName+getWBSaveExtension()

	def write(self, fileName):
		"Save out a high-level desc of the world, and height/terrainmaps"
		fileName = os.path.normpath(fileName)
		fileName,ext = os.path.splitext(fileName)
		CvUtil.pyPrint( 'saveDesc:%s, curDir:%s' %(fileName,os.getcwd()) )

		f = file(self.getDescFileName(fileName), "w")		# open text file
		f.write("Version=%d\n" %(self.getVersion(),))
		self.gameDesc.write(f)	# write game info

		for i in range(gc.getMAX_CIV_TEAMS()):
			CvTeamDesc().write(f, i)		# write team info

		for i in range(gc.getMAX_CIV_PLAYERS()):
			CvPlayerDesc().write(f, i)		# write player info

		self.mapDesc.write(f)	# write map info

		f.write("\n### Plot Info ###\n")
		iGridW = CyMap().getGridWidth()
		iGridH = CyMap().getGridHeight()
		for iX in range(iGridW):
			for iY in range(iGridH):
				plot = CyMap().plot(iX, iY)
				pDesc = CvPlotDesc()
				if pDesc.needToWritePlot(plot):
					pDesc.write(f, plot)

		f.write("\n### Sign Info ###\n")
		iNumSigns = CyEngine().getNumSigns()
		for i in range(iNumSigns):
			sign = CyEngine().getSignByIndex(i)
			pDesc = CvSignDesc()
			pDesc.write(f, sign)

		f.close()

		print("WBSave done\n")
		return 0	# success

	def applyMap(self):
		"after reading setup the map"

		self.gameDesc.apply()

		# recreate map
		print("map rebuild. gridw=%d, gridh=%d" %(self.mapDesc.iGridW, self.mapDesc.iGridH))
		worldSizeType = CvUtil.findInfoTypeNum(gc.getWorldInfo, gc.getNumWorldInfos(), self.mapDesc.worldSize)
		climateType = CvUtil.findInfoTypeNum(gc.getClimateInfo, gc.getNumClimateInfos(), self.mapDesc.climate)
		seaLevelType = CvUtil.findInfoTypeNum(gc.getSeaLevelInfo, gc.getNumSeaLevelInfos(), self.mapDesc.seaLevel)
		CyMap().rebuild(self.mapDesc.iGridW, self.mapDesc.iGridH, self.mapDesc.iTopLatitude, self.mapDesc.iBottomLatitude, self.mapDesc.bWrapX, self.mapDesc.bWrapY, WorldSizeTypes(worldSizeType), ClimateTypes(climateType), SeaLevelTypes(seaLevelType), 0, None)

		print "preapply plots"
		for pDesc in self.plotDesc:
			pDesc.preApply()	# set plot type / terrain type

		print("map apply - recalc areas/regions")
		CyMap().recalculateAreas()

		print "apply plots"
		for pDesc in self.plotDesc:
			pDesc.apply()

		print "apply signs"
		for pDesc in self.signDesc:
			pDesc.apply()

		print "Randomize Resources"
		if (self.mapDesc.bRandomizeResources != "false"):
			for iPlotLoop in range(CyMap().numPlots()):
				pPlot = CyMap().plotByIndex(iPlotLoop)
				pPlot.setBonusType(BonusTypes.NO_BONUS)
			CyMapGenerator().addBonuses()

		print ("WB apply done\n")
		return 0	# ok

	def getAssignedStartingPlots(self):
		"add player starting plots if using random civs"

		# Player stuff
		for iPlayerLoop in range(gc.getMAX_CIV_PLAYERS()):

			pPlayer = gc.getPlayer(iPlayerLoop)
			pWBPlayer = self.playersDesc[iPlayerLoop]
			# <advc.056>
			if pWBPlayer.bNeverAlive:
				continue # </advc.056>
			# Random Start Location
			if (pPlayer.getLeaderType() != -1 and pWBPlayer.bRandomStartLocation != "false"):
				#pPlayer.setStartingPlot(pPlayer.findStartingPlot(True), True)
				# advc.027: Let the DLL find a starting plot later; but make sure that the DLL won't simply assign one of the plots marked with "StartingPlot".
				pPlayer.forceRandomWBStart()
			else:

				# Player's starting plot
				if ((pWBPlayer.iStartingX != -1) and (pWBPlayer.iStartingY != -1)):
					pPlayer.setStartingPlot(CyMap().plot(pWBPlayer.iStartingX, pWBPlayer.iStartingY), True)

		return 0	# ok

	def applyInitialItems(self):
		"add player objects in a last pass"

		# Team stuff
		if (len(self.teamsDesc)) :
			for iTeamLoop in range(gc.getMAX_CIV_TEAMS()):

				if (self.teamsDesc[iTeamLoop]):

					pTeam = gc.getTeam(iTeamLoop)
					pWBTeam = self.teamsDesc[iTeamLoop]

					# Techs
					for techTypeTag in self.teamsDesc[iTeamLoop].techTypes:
						techType = CvUtil.findInfoTypeNum(gc.getTechInfo, gc.getNumTechInfos(), techTypeTag)
						gc.getTeam(iTeamLoop).setHasTech(techType, true, PlayerTypes.NO_PLAYER, false, false)

					# Espionage Points against Other Teams
					for iEspionageLoop in range(len(pWBTeam.aaiEspionageAgainstTeams)):
						iTeam = pWBTeam.aaiEspionageAgainstTeams[iEspionageLoop][0]
						iEspionage = pWBTeam.aaiEspionageAgainstTeams[iEspionageLoop][1]
						gc.getTeam(iTeamLoop).setEspionagePointsAgainstTeam(iTeam, iEspionage)

					# Contact with Other Teams
					for meetTeam in self.teamsDesc[iTeamLoop].bContactWithTeamList:
						gc.getTeam(iTeamLoop).meet(meetTeam, false)

					# Wars
					for warTeam in self.teamsDesc[iTeamLoop].bWarWithTeamList:
						gc.getTeam(iTeamLoop).declareWar(warTeam, false, WarPlanTypes.NO_WARPLAN)

					# Permanent War/Peace
					for permanentWarPeaceTeam in self.teamsDesc[iTeamLoop].bPermanentWarPeaceList:
						gc.getTeam(iTeamLoop).setPermanentWarPeace(permanentWarPeaceTeam, true)

					# Open Borders
					for openBordersTeam in self.teamsDesc[iTeamLoop].bOpenBordersWithTeamList:
						gc.getTeam(iTeamLoop).signOpenBorders(openBordersTeam)

					# Defensive Pacts
					for defensivePactTeam in self.teamsDesc[iTeamLoop].bDefensivePactWithTeamList:
						gc.getTeam(iTeamLoop).signDefensivePact(defensivePactTeam)

					# Vassals
					for vassalTeam in self.teamsDesc[iTeamLoop].bVassalOfTeamList:
						gc.getTeam(vassalTeam).assignVassal(iTeamLoop, true)

					# Projects
					for project in (self.teamsDesc[iTeamLoop].projectType):
						projectTypeNum = CvUtil.findInfoTypeNum(gc.getProjectInfo, gc.getNumProjectInfos(), project)
						gc.getTeam(iTeamLoop).changeProjectCount(projectTypeNum, 1)
						projectCount = gc.getTeam(iTeamLoop).getProjectCount(projectTypeNum)
						gc.getTeam(iTeamLoop).setProjectArtType(projectTypeNum, projectCount - 1, 0)

		# Player stuff
		if (len(self.playersDesc)) :
			for iPlayerLoop in range(gc.getMAX_CIV_PLAYERS()):

				if (self.playersDesc[iPlayerLoop]):

					pPlayer = gc.getPlayer(iPlayerLoop)
					pWBPlayer = self.playersDesc[iPlayerLoop]

					# Player's starting gold
					pPlayer.setGold(pWBPlayer.iStartingGold)

					# Player's starting plot
					if ((pWBPlayer.iStartingX != -1) and (pWBPlayer.iStartingY != -1)):
						pPlayer.setStartingPlot(CyMap().plot(pWBPlayer.iStartingX, pWBPlayer.iStartingY), True)

					# State Religion
					if (pWBPlayer.stateReligion != ""):
						iStateReligionID = gc.getInfoTypeForString(pWBPlayer.stateReligion)
						pPlayer.setLastStateReligion(iStateReligionID)

					# Starting Era
					if (pWBPlayer.szStartingEra != ""):
						iStartingEra = gc.getInfoTypeForString(pWBPlayer.szStartingEra)
						pPlayer.setCurrentEra(iStartingEra)

					# Random Start Location
					if (pWBPlayer.bRandomStartLocation != "false"):
						pPlayer.setStartingPlot(pPlayer.findStartingPlot(true), True)
						print("Setting player %d starting location to (%d,%d)" % (pPlayer.getID(), pPlayer.getStartingPlot().getX(), pPlayer.getStartingPlot().getY()))

					# Civics
					for iCivicLoop in range(len(pWBPlayer.aaiCivics)):
						iCivicOption = pWBPlayer.aaiCivics[iCivicLoop][0]
						iCivic = pWBPlayer.aaiCivics[iCivicLoop][1]
						pPlayer.setCivics(iCivicOption,iCivic)

					# Attitude Extras
					for iAttitudeLoop in range(len(pWBPlayer.aaiAttitudeExtras)):
						iPlayer = pWBPlayer.aaiAttitudeExtras[iAttitudeLoop][0]
						iExtra = pWBPlayer.aaiAttitudeExtras[iAttitudeLoop][1]
						pPlayer.AI_setAttitudeExtra(iPlayer,iExtra)

					# City List
					for iCityListLoop in range(len(pWBPlayer.aszCityList)):
						pPlayer.addCityName(pWBPlayer.aszCityList[iCityListLoop])

		# cities
		for pDesc in self.plotDesc:
			pDesc.applyCity()

		# Team stuff
		if (len(self.teamsDesc)) :
			for iTeamLoop in range(gc.getMAX_CIV_TEAMS()):

				if (self.teamsDesc[iTeamLoop]):

					pTeam = gc.getTeam(iTeamLoop)
					pWBTeam = self.teamsDesc[iTeamLoop]

					# Reveal whole map
					if (pWBTeam.bRevealMap == 1):

						for iPlotX in range(CyMap().getGridWidth()):
							for iPlotY in range(CyMap().getGridHeight()):
							    pPlot = CyMap().plot(iPlotX,iPlotY)
							    pPlot.setRevealed(pTeam.getID(), True, False, TeamTypes.NO_TEAM)

					# Vassal
					if (pWBTeam.iVassalPower != 0):
						pTeam.setVassalPower(pWBTeam.iVassalPower)
					if (pWBTeam.iMasterPower != 0):
						pTeam.setMasterPower(pWBTeam.iMasterPower)

					# Espionage Points Ever against All Teams
					if (pWBTeam.iEspionageEver != 0):
						pTeam.setEspionagePointsEver(pWBTeam.iEspionageEver)

		# Per plot stuff
		for iPlotLoop in range(self.mapDesc.numPlotsWritten):

			pWBPlot = self.plotDesc[iPlotLoop]

			# Reveal Fog of War for teams
			for iTeamLoop in range(gc.getMAX_CIV_TEAMS()):
				pTeam = gc.getTeam(iTeamLoop)
				if pTeam.isEverAlive(): # advc.056
					if (pWBPlot.abTeamPlotRevealed[iTeamLoop] == 1):
						pPlot = CyMap().plot(pWBPlot.iX, pWBPlot.iY)
						pPlot.setRevealed(pTeam.getID(), True, False, TeamTypes.NO_TEAM)

		# units
		for pDesc in self.plotDesc:
			pDesc.applyUnits()
		CvUtil.pyPrint("applyInitialItems: done") # advc.056
		return 0	# ok

	def read(self, fileName):
		"Load in a high-level desc of the world, and height/terrainmaps"
		fileName = os.path.normpath(fileName)
		fileName,ext=os.path.splitext(fileName)
		if len(ext) == 0:
			ext = getWBSaveExtension()
		CvUtil.pyPrint( 'loadDesc:%s, curDir:%s' %(fileName,os.getcwd()) )

		if (not os.path.isfile(fileName+ext)):
			CvUtil.pyPrint("Error: file %s does not exist" %(fileName+ext,))
			return -1	# failed

		f=file(fileName+ext, "r")		# open text file

		parser = CvWBParser()
		version = int(parser.findNextTokenValue(f, "Version"))
		if (version != self.getVersion()):
			CvUtil.pyPrint("Error: wrong WorldBuilder save version.  Expected %d, got %d" %(self.getVersion(), version))
			return -1	# failed

		print "Reading game desc"
		self.gameDesc.read(f)	# read game info
		print "Reading teams desc"
		filePos = f.tell()
		self.teamsDesc = []
		for i in range(gc.getMAX_CIV_TEAMS()):
			print ("reading team %d" %(i))
			teamsDesc = CvTeamDesc()
			if teamsDesc.read(f)==False:
				f.seek(filePos)	# abort and backup
				break
			self.teamsDesc.append(teamsDesc)
		# <advc.056>
		for i in range(len(self.teamsDesc), gc.getMAX_CIV_TEAMS()):
			self.teamsDesc.append(CvTeamDesc()) # </advc.056>

		print "Reading players desc"
		filePos = f.tell() # advc.056
		self.playersDesc = []
		for i in range(gc.getMAX_CIV_PLAYERS()):
			playerDesc = CvPlayerDesc()
			# <advc.056> Same as for teamsDesc above
			if playerDesc.read(f)==False:
				f.seek(filePos)
				break # </advc.056>
			self.playersDesc.append(playerDesc)
		# <advc.056>
		for i in range(len(self.playersDesc), gc.getMAX_CIV_PLAYERS()):
			deadPlayer = CvPlayerDesc()
			# (Not sure if it's necessary to assign a team)
			deadPlayer.team = len(self.playersDesc)
			deadPlayer.isPlayableCiv = 0
			deadPlayer.neverAlive = True
			self.playersDesc.append(deadPlayer)
		# </advc.056>
		print "Reading map desc"
		self.mapDesc.read(f)	# read map info

		print("Reading/creating %d plot descs" %(self.mapDesc.numPlotsWritten,))
		self.plotDesc = []
		for i in range(self.mapDesc.numPlotsWritten):
			pDesc = CvPlotDesc()
			if pDesc.read(f)==True:
				self.plotDesc.append(pDesc)
			else:
				break

		print("Reading/creating %d sign descs" %(self.mapDesc.numSignsWritten,))
		self.signDesc = []
		for i in range(self.mapDesc.numSignsWritten):
			pDesc = CvSignDesc()
			if pDesc.read(f)==True:
				self.signDesc.append(pDesc)
			else:
				break

		f.close()
		print ("WB read done\n")
		return 0
