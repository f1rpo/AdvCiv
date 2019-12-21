## Scoreboard
##
## Holds the information used to display the scoreboard.
##
## Notes
##   - Must be initialized externally by calling init()
##   - Add 'DealCanceled' event for onDealCanceled()
##
## Copyright (c) 2007-2008 The BUG Mod.
##
## Author: EmperorFool

from CvPythonExtensions import *
import BugCore
#import BugDll # advc.004: unused now
import BugUtil
import DealUtil
import FontUtil
import CvUtil
import re
import string
import MonkeyTools # advc.085: For checking Ctrl key

# Globals
ScoreOpt = BugCore.game.Scores
gc = CyGlobalContext()

# Constants
Z_DEPTH = -0.3

# Columns IDs
NUM_PARTS = 28
(
	ALIVE,
	WAR,
	SCORE,
	SCORE_DELTA,
	RANK,
	ID,
	MASTER,
	NAME,
	NOT_MET,
	POWER,
	RESEARCH,
	RESEARCH_TURNS,
	ESPIONAGE,
	TRADE,
	BORDERS,
	PACT,
	RELIGION,
	ATTITUDE,
	WONT_TALK,
	WORST_ENEMY,
	WHEOOH,
	CITIES,
	WAITING,
	NET_STATS,
	OOS,
	LEADER_BUTTON, CIV_BUTTON, # dlph.30
	GOLDEN_AGE # advc.085
) = range(NUM_PARTS)

# Types
SKIP = 0
FIXED = 1
DYNAMIC = 2
SPECIAL = 3

# Column Definitions
columns = []
columnsByKey = {}

TRADE_TYPES = (
	TradeableItems.TRADE_OPEN_BORDERS,
	TradeableItems.TRADE_DEFENSIVE_PACT,
	TradeableItems.TRADE_PERMANENT_ALLIANCE,
	TradeableItems.TRADE_PEACE_TREATY,
)

WAR_ICON = None
PEACE_ICON = None

MASTER_ICON = None
ACTIVE_MASTER_ICON = None

VASSAL_PREFIX = None
VASSAL_POSTFIX = None

def init():
	"""
	Initializes the strings used to display the scoreboard.
	"""
	global columns
	
	# Used keys:
	# ABCDEFHIKLMNOPQRSTUVWZ*?
	# (unused: XY)
	columns.append(Column('', ALIVE))
	columns.append(Column('S', SCORE, DYNAMIC))
	columns.append(Column('Z', SCORE_DELTA, DYNAMIC))
	columns.append(Column('K', RANK, DYNAMIC))
	columns.append(Column('I', ID, DYNAMIC))
	columns.append(Column('V', MASTER, DYNAMIC))
	columns.append(Column('C', NAME, DYNAMIC))
	columns.append(Column('?', NOT_MET, FIXED, smallText("?")))
	columns.append(Column('W', WAR, DYNAMIC))
	columns.append(Column('P', POWER, DYNAMIC))
	columns.append(Column('T', RESEARCH, SPECIAL))
	columns.append(Column('U', RESEARCH_TURNS, DYNAMIC))
	columns.append(Column('E', ESPIONAGE, FIXED, smallSymbol(FontSymbols.COMMERCE_ESPIONAGE_CHAR)))
	columns.append(Column('N', TRADE, FIXED, smallSymbol(FontSymbols.TRADE_CHAR)))
	columns.append(Column('B', BORDERS, FIXED, smallSymbol(FontSymbols.OPEN_BORDERS_CHAR)))
	columns.append(Column('D', PACT, FIXED, smallSymbol(FontSymbols.DEFENSIVE_PACT_CHAR)))
	columns.append(Column('R', RELIGION, DYNAMIC))
	columns.append(Column('A', ATTITUDE, DYNAMIC))
	columns.append(Column('!', WONT_TALK, FIXED, smallText("!"))) #K-Mod changed this from 'F'
	columns.append(Column('H', WORST_ENEMY, FIXED, smallSymbol(FontSymbols.ANGRY_POP_CHAR)))
	columns.append(Column('M', WHEOOH, FIXED, smallSymbol(FontSymbols.OCCUPATION_CHAR)))
	columns.append(Column('Q', CITIES, DYNAMIC))
	columns.append(Column('*', WAITING, FIXED, smallText("*")))
	columns.append(Column('L', NET_STATS, DYNAMIC))
	columns.append(Column('O', OOS, DYNAMIC))
	# <dlph.30>
	columns.append(Column('F', LEADER_BUTTON, SPECIAL))
	columns.append(Column('G', CIV_BUTTON, SPECIAL))
	# </dlph.30>
	columns.append(Column('J', GOLDEN_AGE, DYNAMIC)) # advc.085
	
	global WAR_ICON, PEACE_ICON
	WAR_ICON = smallSymbol(FontSymbols.WAR_CHAR)
	PEACE_ICON = smallSymbol(FontSymbols.PEACE_CHAR)
	
	global MASTER_ICON, ACTIVE_MASTER_ICON
	MASTER_ICON = smallSymbol(FontSymbols.SILVER_STAR_CHAR)
	ACTIVE_MASTER_ICON = smallSymbol(FontSymbols.STAR_CHAR)
	
	global VASSAL_PREFIX, VASSAL_POSTFIX
	VASSAL_PREFIX = smallSymbol(FontSymbols.BULLET_CHAR)
	VASSAL_POSTFIX = smallText(u" %s" % FontUtil.getChar(FontSymbols.BULLET_CHAR))
	# <advc.085>
	global GOLDEN_AGE_ICON, ANARCHY_ICON
	GOLDEN_AGE_ICON = smallSymbol(FontSymbols.GOLDEN_AGE_CHAR)
	ANARCHY_ICON = smallSymbol(FontSymbols.BAD_GOLD_CHAR) # </advc.085>

def smallText(text):
	return u"<font=2>%s</font>" % text

def smallSymbol(symbol):
	return smallText(FontUtil.getChar(symbol))

def onDealCanceled(argsList):
	"""Sets the scoreboard dirty bit so it will redraw."""
	CyInterface().setDirty(InterfaceDirtyBits.Score_DIRTY_BIT, True)


class Column:
	
	def __init__(self, key, id, type=SKIP, text=None, alt=None):
		self.key = key
		self.id = id
		self.type = type
		self.text = text
		self.alt = alt
		if (type == FIXED):
			self.width = CyInterface().determineWidth( text )
		else:
			self.width = 0
		if (key):
			columnsByKey[key] = self
	
	def isSkip(self):
		return self.type == SKIP
	
	def isFixed(self):
		return self.type == FIXED
	
	def isDynamic(self):
		return self.type == DYNAMIC
	
	def isSpecial(self):
		return self.type == SPECIAL


class Scoreboard:
	"""
	Holds and builds the ScoreCards.
	"""
	
	def __init__(self):
		self._activePlayer = gc.getGame().getActivePlayer()
		self._teamScores = []
		self._playerScores = []
		self._teamScoresByID = {}
		self._anyHas = [ False ] * NUM_PARTS
		self._currTeamScores = None
		self._currPlayerScore = None
		self._deals = DealUtil.findDealsByPlayerAndType(self._activePlayer, TRADE_TYPES)
		
	def addTeam(self, team, rank):
		self._currTeamScores = TeamScores(self, team, rank)
		self._teamScores.append(self._currTeamScores)
		self._teamScoresByID[team.getID()] = self._currTeamScores
		self._currPlayerScore = None
		
	def getTeamScores(self, eTeam):
		return self._teamScoresByID.get(eTeam, None)
		
	def addPlayer(self, player, rank):
		if self._currTeamScores:
			self._currPlayerScore = self._currTeamScores.addPlayer(player, rank)
			self._playerScores.append(self._currPlayerScore)
		
	def size(self):
		return len(self._playerScores)
		
		
	def setAlive(self):
		self._set(ALIVE)
		
	def setMaster(self):
		self._set(MASTER, MASTER_ICON)
		
	def setMasterSelf(self):
		self._set(MASTER, ACTIVE_MASTER_ICON)
		
	def setScore(self, value):
		# <advc.085>
		# Set the contact widget explicitly for Score and Name (no longer the default)
		widgetData = None
		if gc.getPlayer(self._currPlayerScore.getID()).isAlive():
			widgetData = self._getContactWidget()
		# Score breakdown when hovering over the active player's score (no longer provided by WIDGET_CONTACT_CIV) -- or anyone's score in Debug mode w/ Ctrl pressed.
		if self._activePlayer == self._currPlayerScore.getID() or (gc.getGame().isDebugMode() and MonkeyTools.bCtrl()):
			widgetData = (WidgetTypes.WIDGET_SCORE_BREAKDOWN, self._currPlayerScore.getID(), 0)
		# </advc.085>
		self._set(SCORE, smallText(value), widgetData)
		
	def setScoreDelta(self, value):
		self._set(SCORE_DELTA, smallText(value))
		
	def setRank(self, value):
		self._set(RANK, smallText(value))
		
	def setID(self, value):
		self._set(ID, smallText(value))
		
	def setName(self, value):
		# advc.085: See setScore
		widgetData = None
		if gc.getPlayer(self._currPlayerScore.getID()).isAlive():
			widgetData = self._getContactWidget()
		self._set(NAME, smallText(value), widgetData)
		
	def setNotMet(self):
		self._set(NOT_MET)
		
	def setWHEOOH(self):
		self._set(WHEOOH)
		
	def setNumCities(self, value):
		self._set(CITIES, smallText(value))
		
	def setWar(self):
		self._set(WAR, WAR_ICON)
		
	def setPeace(self):
		self._set(WAR, PEACE_ICON, self._getDealWidget(TradeableItems.TRADE_PEACE_TREATY))
	# <advc.085> Widget help added; pass along color.
	def setPower(self, value, color):
		self._set(POWER, smallText(value), (WidgetTypes.WIDGET_POWER_RATIO, self._currPlayerScore.getID(), color)) # </advc.085>
	# <advc.085>
	def setResearch(self, tech, progress): # Third param was 'turns'
		if tech != -1: # No longer guaranteed by caller </advc.085>
			if (ScoreOpt.isShowResearchIcons()):
				self._set(RESEARCH, tech)
			else:
				self._set(RESEARCH, smallText(gc.getTechInfo(tech).getDescription()))
		#if turns >= 0: # advc.004x
		#	self._set(RESEARCH_TURNS, smallText(u"(%d)" % turns))
		# <advc.085> Replacing the two lines above
		szProgress = u" %d%%" % progress
		# Color it green? I guess better not.
		#szProgress = CyTranslator().changeTextColor(szProgress, gc.getInfoTypeForString("COLOR_ALT_HIGHLIGHT_TEXT"))
		self._set(RESEARCH_TURNS, smallText(szProgress)) # </advc.085>
		
	def setEspionage(self):
		self._set(ESPIONAGE)
		
	def setTrade(self): # advc.004: BULL widget help enabled
		self._set(TRADE, True, (WidgetTypes.WIDGET_TRADE_ROUTES, self._activePlayer, self._currPlayerScore.getID()))
		
	def setBorders(self):
		self._set(BORDERS, True, self._getDealWidget(TradeableItems.TRADE_OPEN_BORDERS))
		
	def setPact(self):
		self._set(PACT, True, self._getDealWidget(TradeableItems.TRADE_DEFENSIVE_PACT))
		
	def setReligion(self, value):
		self._set(RELIGION, smallText(value))
		
	def setAttitude(self, value):
		self._set(ATTITUDE, smallText(value))
		
	def setWontTalk(self):
		self._set(WONT_TALK)
		
	def setWorstEnemy(self):
		self._set(WORST_ENEMY)
		
		
	def setWaiting(self):
		self._set(WAITING)
		
	def setNetStats(self, value):
		self._set(NET_STATS, smallText(value))
		
	def setOOS(self, value):
		self._set(OOS, smallText(value))
	# <dlph.30>
	def setLeaderIcon(self, leader):
		self._set(LEADER_BUTTON, leader)

	def setCivIcon(self, civ):
		self._set(CIV_BUTTON, civ)
	# </dlph.30>
	# <advc.085>
	def setGoldenAge(self, bAnarchy):
		eWidget = WidgetTypes.WIDGET_GOLDEN_AGE
		cIcon = GOLDEN_AGE_ICON
		if bAnarchy:
			eWidget = WidgetTypes.WIDGET_ANARCHY
			cIcon = ANARCHY_ICON
		# Add one space b/c the icons are very tiny; difficult to hover over.
		self._set(GOLDEN_AGE, " " + cIcon, (eWidget, self._currPlayerScore.getID(), 0))
	# </advc.085>

	def _getContactWidget(self):
		iData2 = 0 # advc.085: Was -1; tell the DLL to expand the scoreboard.
		return (WidgetTypes.WIDGET_CONTACT_CIV, self._currPlayerScore.getID(), iData2)
		
	def _getDealWidget(self, type):
		iData2 = 0 # advc.085: Was -1; tell the DLL to expand the scoreboard.
		# lookup the Deal containing the given tradeable item type
		deals = self._deals.get(self._currPlayerScore.getID(), None)
		if deals:
			deal = deals.get(type, None)
			if deal:
				return (WidgetTypes.WIDGET_DEAL_KILL, deal.getID(), iData2)
		return (WidgetTypes.WIDGET_DEAL_KILL, -1, iData2)
		
	def _set(self, part, value=True, widget=None):
		self._anyHas[part] = True
		self._currPlayerScore.set(part, value, widget)
		
		
	def assignRanks(self):
		"""
		Assigns a rank from 1 to N based on score.
		As the player scores are currently reversed, this is done in reverse order.
		"""
		rank = 0
		scores = list(self._playerScores)
		scores.reverse()
		for playerScore in scores:
			if not playerScore.has(NOT_MET) or not playerScore.value(NOT_MET):
				rank += 1
				playerScore.set(RANK, smallText(BugUtil.colorText(u"%d" % rank, ScoreOpt.getRankColor())))
		if rank > 0:
			self._anyHas[RANK] = True
		
	def gatherVassals(self):
		for teamScores in self._teamScores:
			teamScores.gatherVassals()
		
	def sort(self):
		"""Sorts the list by pulling any vassals up below their masters."""
		if ScoreOpt.isGroupVassals():
			self._playerScores.sort(lambda x, y: cmp(x.sortKey(), y.sortKey()))
			self._playerScores.reverse()
		maxPlayers = ScoreOpt.getMaxPlayers()
		if maxPlayers > 0 and len(self._playerScores) > maxPlayers:
			self._playerScores = self._playerScores[len(self._playerScores) - maxPlayers:]
		
	def hide(self, screen):
		"""Hides the text from the screen before building the scoreboard."""
		screen.hide( "ScoreBackground" )
		for p in range( gc.getMAX_CIV_PLAYERS() ):
			name = "ScoreText%d" %( p ) # the part that flashes? holds the score and name
			screen.hide( name )
			for c in range( NUM_PARTS ):
				name = "ScoreText%d-%d" %( p, c )
				screen.hide( name )
		
	def draw(self, screen):
		"""Sorts and draws the scoreboard right-to-left, bottom-to-top."""
		timer = BugUtil.Timer("scores")
		self.hide(screen)
		self.assignRanks()
		self.gatherVassals()
		self.sort()
		interface = CyInterface()
		xResolution = screen.getXResolution()
		yResolution = screen.getYResolution()
		
		x = xResolution - 12 # start here and shift left with each column
		if ( interface.getShowInterface() == InterfaceVisibility.INTERFACE_SHOW or interface.isInAdvancedStart()):
			y = yResolution - 188 # advc.106d: was yResolution-206
		else:
			y = yResolution - 88
		totalWidth = 0
		height = ScoreOpt.getLineHeight()
		techIconSize = ScoreOpt.getResearchIconSize()
		
		defaultSpacing = ScoreOpt.getDefaultSpacing()
		spacing = defaultSpacing
		szDisplayOrder = ScoreOpt.getDisplayOrder()
		# <advc.085>
		bExpanded = False
		if gc.getPlayer(self._activePlayer).isScoreboardExpanded():
			bExpanded = True
		else: # Take out the keys preceded by an underscore
			stringsToRemove = []
			for i, c in enumerate(szDisplayOrder):
				if i > 0 and szDisplayOrder[i - 1] == '_':
					stringsToRemove.append('_' + c)
			for s in stringsToRemove:
				szDisplayOrder = szDisplayOrder.replace(s, '')
			# Remove any stray underscores as well
			szDisplayOrder = szDisplayOrder.replace('_', '')
		# </advc.085>
		format = re.findall('(-?[0-9]+|[^0-9])', szDisplayOrder.replace(' ', '').upper())
		format.reverse()
		for k in format:
			if k == '-':
				spacing = 0
				continue
			if k[0] in string.digits or k[0] == '-':
				spacing = int(k)
				continue
			if (not columnsByKey.has_key(k)):
				spacing = defaultSpacing
				continue
			column = columnsByKey[k]
			c = column.id
			if (not self._anyHas[c]):
				#spacing = defaultSpacing # disabled by K-Mod
				continue
			type = column.type
			if (c == RESEARCH and not ScoreOpt.isShowResearchIcons()):
				# switch SPECIAL research icon to DYNAMIC name
				type = DYNAMIC
			# advc.085: For filling gaps so that the scoreboard doesn't collapse. 4 spaces seem to fit almost exactly for columns with a single icon.
			szBlank = "    "
			if (type == SKIP):
				spacing = defaultSpacing
				continue
			elif (type == FIXED):
				width = column.width
				value = column.text
				x -= spacing
				for p, playerScore in enumerate(self._playerScores):
					name = "ScoreText%d-%d" %( p, c ) # advc.085: Moved up
					if (playerScore.has(c) and playerScore.value(c)):
						widget = playerScore.widget(c)
						if widget is None:
							#if (playerScore.value(ALIVE)):
							#	widget = (WidgetTypes.WIDGET_CONTACT_CIV, playerScore.getID(), 0)
							#else:
							#	widget = (WidgetTypes.WIDGET_GENERAL, -1, -1)
							# <advc.085> Contact widget now set explicitly for Score and Name. Default widget: expand the scoreboard.
							if bExpanded:
								widget = (WidgetTypes.WIDGET_EXPAND_SCORES, -1, 0)
							else: # </advc.085>
								widget = (WidgetTypes.WIDGET_GENERAL, -1, -1)
						screen.setText( name, "Background", value, CvUtil.FONT_RIGHT_JUSTIFY, x, y - p * height, Z_DEPTH, FontTypes.SMALL_FONT, *widget )
						screen.show(name)
					# <advc.085>
					elif bExpanded:
						screen.setText(name, "Background", szBlank, CvUtil.FONT_RIGHT_JUSTIFY, x, y - p * height, Z_DEPTH, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_EXPAND_SCORES, -1, 0)
						screen.show(name) # </advc.085>
				x -= width
				totalWidth += width + spacing
				spacing = defaultSpacing
			
			elif (type == DYNAMIC):
				width = 0
				for playerScore in self._playerScores:
					if (playerScore.has(c)):
						value = playerScore.value(c)
						if (c == NAME and playerScore.isVassal() and ScoreOpt.isGroupVassals()):
							if (ScoreOpt.isLeftAlignName()):
								value = VASSAL_PREFIX + value
							else:
								value += VASSAL_POSTFIX
						newWidth = interface.determineWidth( value )
						if (newWidth > width):
							width = newWidth
				if (width == 0):
					spacing = defaultSpacing
					continue
				x -= spacing
				for p, playerScore in enumerate(self._playerScores):
					name = "ScoreText%d-%d" %( p, c ) # advc.085: Moved up
					if (playerScore.has(c)):
						value = playerScore.value(c)
						if (c == NAME and playerScore.isVassal() and ScoreOpt.isGroupVassals()):
							if (ScoreOpt.isLeftAlignName()):
								value = VASSAL_PREFIX + value
							else:
								value += VASSAL_POSTFIX
						align = CvUtil.FONT_RIGHT_JUSTIFY
						adjustX = 0
						if (c == NAME):
							name = "ScoreText%d" % p
							if (ScoreOpt.isLeftAlignName()):
								align = CvUtil.FONT_LEFT_JUSTIFY
								adjustX = width
						widget = playerScore.widget(c)
						if widget is None:
							#if (playerScore.value(ALIVE)):
							#	widget = (WidgetTypes.WIDGET_CONTACT_CIV, playerScore.getID(), 0)
							#else:
							#	widget = (WidgetTypes.WIDGET_GENERAL, -1, -1)
							# <advc.085> See under FIXED above
							if bExpanded:
								widget = (WidgetTypes.WIDGET_EXPAND_SCORES, -1, 0)
							else: # </advc.085>
								widget = (WidgetTypes.WIDGET_GENERAL, -1, -1)
						screen.setText( name, "Background", value, align, 
										x - adjustX, y - p * height, Z_DEPTH, 
										FontTypes.SMALL_FONT, *widget )
						screen.show(name)
					# <advc.085>
					elif bExpanded and c != NAME:
						szBlankLoop = szBlank
						if c == POWER: # Power ratio takes up extra space
							# Mustn't add too much space though: when the power ratio is in the leftmost column, too many spaces will prevent the scoreboard from collapsing when the mouse is moved away to the left.
							szBlankLoop += szBlank + szBlank
						screen.setText(name, "Background", szBlankLoop, CvUtil.FONT_RIGHT_JUSTIFY, x, y - p * height, Z_DEPTH, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_EXPAND_SCORES, -1, 0)
						screen.show(name) # </advc.085>
				x -= width
				totalWidth += width + spacing
				spacing = defaultSpacing
			
			else: # SPECIAL
				if (c == RESEARCH):
					x -= spacing
					for p, playerScore in enumerate(self._playerScores):
						if (playerScore.has(c)):
							tech = playerScore.value(c)
							name = "ScoreTech%d" % p
							info = gc.getTechInfo(tech)
							iData2 = 0 # advc.085: was -1
							screen.addDDSGFC( name, info.getButton(), x - techIconSize, y - p * height - 1, techIconSize, techIconSize, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, tech, iData2 )
					x -= techIconSize
					totalWidth += techIconSize + spacing
					spacing = defaultSpacing
				# <dlph.30>
				elif c == LEADER_BUTTON:
					x -= spacing
					for p, playerScore in enumerate(self._playerScores):
						if (playerScore.has(c)):
							leader = playerScore.value(c)
							name = "ScoreLeader%d" % p
							info = gc.getLeaderHeadInfo(leader)
							screen.addDDSGFC( name, info.getButton(), x - techIconSize, y - p * height - 1, techIconSize, techIconSize, WidgetTypes.WIDGET_PEDIA_JUMP_TO_LEADER, leader, 1 )
					x -= techIconSize
					totalWidth += techIconSize + spacing
					spacing = defaultSpacing
				elif c == CIV_BUTTON:
					x -= spacing
					for p, playerScore in enumerate(self._playerScores):
						if (playerScore.has(c)):
							civ = playerScore.value(c)
							name = "ScoreCiv%d" % p
							info = gc.getCivilizationInfo(civ)
							screen.addDDSGFC( name, info.getButton(), x - techIconSize, y - p * height - 1, techIconSize, techIconSize, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIV, civ, -1 )
					x -= techIconSize
					totalWidth += techIconSize + spacing
					spacing = defaultSpacing
				# </dlph.30>
		
		for playerScore in self._playerScores:
			interface.checkFlashReset( playerScore.getID() )
		
		if ( interface.getShowInterface() == InterfaceVisibility.INTERFACE_SHOW or interface.isInAdvancedStart()):
			y = yResolution - 168 # advc.106d: was yResolution-186
		else:
			y = yResolution - 68
		screen.setPanelSize( "ScoreBackground", xResolution - 21 - totalWidth, y - (height * self.size()) - 4, 
							 totalWidth + 12, (height * self.size()) + 8 )
		screen.show( "ScoreBackground" )
		timer.log()


class TeamScores:
	def __init__(self, scoreboard, team, rank):
		self._scoreboard = scoreboard
		self._team = team
		self._rank = rank
		self._playerScores = []
		#self._isVassal = team.isAVassal()
		self._isVassal = team.isAVassal() and gc.getTeam(gc.getGame().getActiveTeam()).isHasMet(team.getID()) # K-Mod
		self._master = None
		self._vassalTeamScores = []
		
	def team(self):
		return self._team
		
	def rank(self):
		if self.isVassal():
			return self._master.rank()
		else:
			return self._rank
		
	def isVassal(self):
		return self._isVassal
	
	def addPlayer(self, player, rank):
		playerScore = PlayerScore(self, player, rank)
		self._playerScores.append(playerScore)
		return playerScore
	
	def addVassal(self, teamScore):
		self._vassalTeamScores.append(teamScore)
		
	def gatherVassals(self):
		#if self._team.isAVassal():
		if self.isVassal(): # K-Mod
			for eTeam in range( gc.getMAX_TEAMS() ):
				teamScores = self._scoreboard.getTeamScores(eTeam)
				if teamScores and self._team.isVassal(eTeam):
					# teamScores is a master of self
					teamScores.addVassal(self)
					self._master = teamScores
					for playerScore in teamScores._playerScores:
						if playerScore.isActive():
							playerScore.set(MASTER, ACTIVE_MASTER_ICON)
						else:
							playerScore.set(MASTER, MASTER_ICON)
					self._scoreboard._anyHas[MASTER] = True
			# K-Mod (to fix a problem when a human player becomes the vassal of an unmet team)
			if self._master == None:
				self._isVassal = False
			# K-Mod end


class PlayerScore:
	def __init__(self, teamScore, player, rank):
		self._teamScore = teamScore
		self._isVassal = teamScore.isVassal()
		self._player = player
		self._rank = rank
		self._has = [False] * NUM_PARTS
		self._values = [None] * NUM_PARTS
		self._widgets = [None] * NUM_PARTS
		self._sortKey = None
		
	def player(self):
		return self._player
		
	def rank(self):
		return self._rank
		
	def isVassal(self):
		return self._isVassal
	
	def getID(self):
		return self._player.getID()
	
	def isActive(self):
		return self.getID() == gc.getGame().getActivePlayer()
		
	def sortKey(self):
		if self._sortKey is None:
				self._sortKey = (self._teamScore.rank(), self._isVassal, self._rank)
		return self._sortKey
		
	def set(self, part, value=True, widget=None):
		self._has[part] = True
		self._values[part] = value
		self._widgets[part] = widget
		
	def has(self, part):
		return self._has[part]
		
	def value(self, part):
		return self._values[part]
		
	def widget(self, part):
		return self._widgets[part]
